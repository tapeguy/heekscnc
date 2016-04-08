// Profile.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Profile.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/Geom.h"
#include "interface/HeeksObj.h"
#include "tinyxml/tinyxml.h"
#include "interface/InputMode.h"
#include "interface/LeftAndRight.h"
#include "interface/Tool.h"
#include "CTool.h"
#include "CNCPoint.h"
#include "Reselect.h"
#include "PythonStuff.h"
#include "Tags.h"
#include "Tag.h"
#include "ProfileDlg.h"

#include <gp_Pnt.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

#include <sstream>
#include <iomanip>

// static
PropertyDouble CProfile::max_deviation_for_spline_to_arc = 0.1;

CProfileParams::CProfileParams(CProfile * parent)
{
    this->parent = parent;
    InitializeProperties();
    m_tool_on_side = eOn;
	m_cut_mode = eConventional;
	m_auto_roll_radius = 2.0;
	m_auto_roll_on = true;
	m_auto_roll_off = true;

	m_start_given = false;
	m_end_given = false;

    m_extend_at_start = 0.0;
    m_extend_at_end = 0.0;

    m_lead_in_line_len = 1.0;
    m_lead_out_line_len= 1.0;

	m_end_beyond_full_profile = false;
	m_sort_sketches = 1;
	m_offset_extra = 0.0;
	m_do_finishing_pass = false;
	m_only_finishing_pass = false;
	m_finishing_h_feed_rate = 0.0;
	m_finishing_cut_mode = eConventional;
	m_finishing_step_down = 1.0;
}


void CProfileParams::InitializeProperties()
{
    m_tool_on_side.Initialize(_("tool_on_side"), parent);

    {
        std::list< wxString > choices;
        choices.push_back(_("Conventional"));
        choices.push_back(_("Climb"));
        m_cut_mode.Initialize(_("cut_mode"), parent);
        m_cut_mode.m_choices = choices;
    }

    m_auto_roll_on.Initialize(_("auto_roll_on"), parent);
    m_auto_roll_off.Initialize(_("auto_roll_off"), parent);
    m_auto_roll_radius.Initialize(_("roll_radius"), parent);
    m_lead_in_line_len.Initialize(_("lead_in_line_length"), parent);
    m_lead_out_line_len.Initialize(_("lead_out_line_length"), parent);
    m_roll_on_point.Initialize(_("roll_on_point"), parent);
    m_roll_off_point.Initialize(_("roll_off_point"), parent);
    m_start_given.Initialize(_("use_start_point"), parent);
    m_end_given.Initialize(_("use_end_point"), parent);
    m_start.Initialize(_("start_point"), parent);
    m_end.Initialize(_("end_point"), parent);
    m_extend_at_start.Initialize(_("extend_before_start"), parent);
    m_extend_at_end.Initialize(_("extend_past_end"), parent);
    m_end_beyond_full_profile.Initialize(_("end_beyond_full_profile"), parent);
    m_sort_sketches.Initialize(_("sort_sketches"), parent);

    m_offset_extra.Initialize(_("extra_offset"), parent);
    m_do_finishing_pass.Initialize(_("do_finishing_pass"), parent);
    m_only_finishing_pass.Initialize(_("only_finishing_pass"), parent);
    m_finishing_h_feed_rate.Initialize(_("finishing_feed_rate"), parent);
    {
        std::list< wxString > choices;
        choices.push_back(_("Conventional"));
        choices.push_back(_("Climb"));
        m_finishing_cut_mode.Initialize(_("finish_cut_mode"), parent);
        m_finishing_cut_mode.m_choices = choices;
    }
    m_finishing_step_down.Initialize(_("finishing_step_down"), parent);
}

void CProfileParams::GetProperties(std::list<Property *> *list)
{
    CToolParams::eToolType tool_type = CTool::FindToolType(parent->m_tool_number);

    if(CTool::IsMillingToolType(tool_type)){
        std::list< wxString > choices;

        SketchOrderType order = SketchOrderTypeUnknown;

        {
            HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, parent->m_sketch);
            if((sketch) && (sketch->GetType() == SketchType))
            {
                order = heeksCAD->GetSketchOrder(sketch);
            }
        }

        switch(order)
        {
        case SketchOrderTypeOpen:
            choices.push_back(_("Left"));
            choices.push_back(_("Right"));
            break;

        case SketchOrderTypeCloseCW:
        case SketchOrderTypeCloseCCW:
            choices.push_back(_("Outside"));
            choices.push_back(_("Inside"));
            break;

        default:
            choices.push_back(_("Outside or Left"));
            choices.push_back(_("Inside or Right"));
            break;
        }
        choices.push_back(_("On"));

        int choice = int(eOn);
        switch (m_tool_on_side)
        {
            case eRightOrInside:    choice = 1;
                    break;

            case eOn:   choice = 2;
                    break;

            case eLeftOrOutside:    choice = 0;
                    break;
        } // End switch

        m_tool_on_side.m_choices = choices;
    }
}

CProfile::CProfile(int sketch, const int tool_number )
 : CSketchOp(sketch, tool_number, ProfileType), m_tags(NULL), m_profile_params(this)
{
    ReadDefaultValues();
} // End constructor


CProfile::CProfile( const CProfile & rhs )
 : CSketchOp(rhs), m_profile_params(this)
{
	m_tags = new CTags;
	Add( m_tags, NULL );
	if (rhs.m_tags != NULL) *m_tags = *(rhs.m_tags);
	m_sketches.clear();
	std::copy( rhs.m_sketches.begin(), rhs.m_sketches.end(), std::inserter( m_sketches, m_sketches.begin() ) );
	m_profile_params = rhs.m_profile_params;
}

CProfile & CProfile::operator= ( const CProfile & rhs )
{
	if (this != &rhs)
	{
	    CSketchOp::operator=( rhs );
		if ((m_tags != NULL) && (rhs.m_tags != NULL)) *m_tags = *(rhs.m_tags);
		m_sketches.clear();
		std::copy( rhs.m_sketches.begin(), rhs.m_sketches.end(), std::inserter( m_sketches, m_sketches.begin() ) );

		m_profile_params = rhs.m_profile_params;
	}

	return(*this);
}

const wxBitmap &CProfile::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/profile.png")));
	return *icon;
}

bool CProfile::Add(HeeksObj* object, HeeksObj* prev_object)
{
	switch(object->GetType())
	{
	case TagsType:
		m_tags = (CTags*)object;
		break;
	}

	return CSketchOp::Add(object, prev_object);
}

void CProfile::Remove(HeeksObj* object)
{
	// set the m_tags pointer to NULL, when Tags is removed from here
	if(object == m_tags)m_tags = NULL;

	CSketchOp::Remove(object);
}

Python CProfile::WriteSketchDefn(HeeksObj* sketch, bool reversed)
{
	// write the python code for the sketch
	Python python;

	if ((!sketch->GetTitle().IsEmpty()) && (wxString(sketch->GetTitle()).size() > 0))
	{
		python << (wxString::Format(_T("\ncomment(%s)\n"), PythonString(sketch->GetTitle()).c_str()));
	}

	python << _T("curve = area.Curve()\n");

	bool started = false;
	std::list<HeeksObj*> new_spans;
	for(HeeksObj* span = sketch->GetFirstChild(); span; span = sketch->GetNextChild())
	{
		if(span->GetType() == SplineType)
		{
			std::list<HeeksObj*> new_spans2;
			heeksCAD->SplineToBiarcs(span, new_spans2, CProfile::max_deviation_for_spline_to_arc);
			for(std::list<HeeksObj*>::iterator It2 = new_spans2.begin(); It2 != new_spans2.end(); It2++)
			{
				HeeksObj* s = *It2;
				if(reversed)
				    new_spans.push_front(s);
				else
				    new_spans.push_back(s);
			}
		}
		else
		{
		    if(reversed)
		        new_spans.push_front(span->MakeACopyWithSameID());
		    else
		        new_spans.push_back(span->MakeACopyWithSameID());
		}
	}

	for(std::list<HeeksObj*>::iterator It = new_spans.begin(); It != new_spans.end(); It++)
	{
		HeeksObj* span_object = *It;
		double s[3] = {0, 0, 0};
		double e[3] = {0, 0, 0};
		double c[3] = {0, 0, 0};

		if(span_object) {

		    python << _T("# ");
		    HeeksObj * owner = span_object->GetOwner();
		    if (owner) {
		        python << owner->GetTypeString();
		        python << _T(" ");
		        python << (int)owner->GetID();
		    }
		    else {
                python << span_object->GetTypeString();
                python << _T(" ");
		        python << (int)span_object->GetID();
		    }
		    python << _T("\n");

			int type = span_object->GetType();
			if(type == LineType || type == ArcType || type == CircleType)
			{
				if(!started && type != CircleType)
				{
					if(reversed)
					    span_object->GetEndPoint(s);
					else
					    span_object->GetStartPoint(s);

					CNCPoint start(s);

					python << _T("curve.append(area.Point(");
					python << start.X(true);
					python << _T(", ");
					python << start.Y(true);
					python << _T("))\n");
					started = true;
				}
				if(reversed)
				    span_object->GetStartPoint(e);
				else
				    span_object->GetEndPoint(e);

				CNCPoint end(e);

				if(type == LineType)
				{
					python << _T("curve.append(area.Point(");
					python << end.X(true);
					python << _T(", ");
					python << end.Y(true);
					python << _T("))\n");
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);

					CNCPoint centre(c);

					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = ((pos[2] >=0) != reversed) ? 1: -1;
					python << _T("curve.append(area.Vertex(");
					python << (span_type);
					python << (_T(", area.Point("));
					python << end.X(true);
					python << (_T(", "));
					python << end.Y(true);
					python << (_T("), area.Point("));
					python << centre.X(true);
					python << (_T(", "));
					python << centre.Y(true);
					python << (_T(")))\n"));
				}
				else if(type == CircleType)
				{
					std::list< std::pair<int, gp_Pnt > > points;
					span_object->GetCentrePoint(c);

					double radius = heeksCAD->CircleGetRadius(span_object);

					// Setup the four arcs to make up the full circle using UNadjusted
					// coordinates.  We do this so that the offsets are expressed along the
					// X and Y axes.  We will adjust the resultant points later.

					// The kurve code needs a start point first.
					points.push_back( std::make_pair(0, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					if(reversed)
					{
						points.push_back( std::make_pair(1, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
						points.push_back( std::make_pair(1, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
						points.push_back( std::make_pair(1, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
						points.push_back( std::make_pair(1, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					}
					else
					{
						points.push_back( std::make_pair(-1, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
						points.push_back( std::make_pair(-1, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
						points.push_back( std::make_pair(-1, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
						points.push_back( std::make_pair(-1, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					}

					CNCPoint centre(c);

					for (std::list< std::pair<int, gp_Pnt > >::iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
					{
						CNCPoint pnt( l_itPoint->second );

						python << (_T("curve.append(area.Vertex("));
						python << l_itPoint->first << _T(", area.Point(");
						python << pnt.X(true);
						python << (_T(", "));
						python << pnt.Y(true);
						python << (_T("), area.Point("));
						python << centre.X(true);
						python << (_T(", "));
						python << centre.Y(true);
						python << (_T(")))\n"));
					} // End for
				}
			}
		}
	}

	// delete the spans made
	for(std::list<HeeksObj*>::iterator It = new_spans.begin(); It != new_spans.end(); It++)
	{
		HeeksObj* span = *It;
		delete span;
	}

	double scale = Length::Conversion(theApp.m_program->m_units, UnitTypeMillimeter);

	if(m_profile_params.m_start_given || m_profile_params.m_end_given)
	{
		double startx, starty, finishx, finishy;

		wxString start_string;
		if(m_profile_params.m_start_given)
		{
#ifdef UNICODE
			std::wostringstream ss;
#else
			std::ostringstream ss;
#endif

            gp_Pnt starting(m_profile_params.m_start.X() / scale,
                            m_profile_params.m_start.Y() / scale,
                            0.0 );

			startx = starting.X();
			starty = starting.Y();

			ss.imbue(std::locale("C"));
			ss<<std::setprecision(10);
			ss << ", start = area.Point(" << startx << ", " << starty << ")";
			start_string = ss.str().c_str();
		}


		wxString finish_string;
		wxString beyond_string;
		if(m_profile_params.m_end_given)
		{
#ifdef UNICODE
			std::wostringstream ss;
#else
			std::ostringstream ss;
#endif

			gp_Pnt finish(m_profile_params.m_end.X() / scale,
					      m_profile_params.m_end.Y() / scale,
					      0.0 );

			finishx = finish.X();
			finishy = finish.Y();

			ss.imbue(std::locale("C"));
			ss<<std::setprecision(10);
			ss << ", finish = area.Point(" << finishx << ", " << finishy << ")";
			finish_string = ss.str().c_str();

			if(m_profile_params.m_end_beyond_full_profile)beyond_string = _T(", end_beyond = True");
		}

		python << (wxString::Format(_T("kurve_funcs.make_smaller( curve%s%s%s)\n"), start_string.c_str(), finish_string.c_str(), beyond_string.c_str())).c_str();
	}

	return(python);
}

Python CProfile::AppendTextForSketch(HeeksObj* object, CProfileParams::eCutMode cut_mode)
{
    Python python;
    double scale = Length::Conversion(theApp.m_program->m_units, UnitTypeMillimeter);

	if(object)
	{
		// decide if we need to reverse the kurve
		bool reversed = false;
		bool initially_ccw = false;
		if(m_profile_params.m_tool_on_side != CProfileParams::eOn)
		{
            if(object)
            {
                switch(object->GetType())
                {
                case CircleType:
                case AreaType:
                    initially_ccw = true;
                    break;
                case SketchType:
                    SketchOrderType order = heeksCAD->GetSketchOrder(object);
                    if(order == SketchOrderTypeCloseCCW)
                        initially_ccw = true;
                    break;
                }
            }
            if(m_speed_op_params.m_spindle_speed<0)
                reversed = !reversed;
            if(cut_mode == CProfileParams::eConventional)
                reversed = !reversed;
            if(m_profile_params.m_tool_on_side == CProfileParams::eRightOrInside)
                reversed = !reversed;
		}

		// write the kurve definition
		python << WriteSketchDefn(object, initially_ccw != reversed);

        if((m_profile_params.m_start_given == false) && (m_profile_params.m_end_given == false))
        {
            python << _T("kurve_funcs.set_good_start_point(curve, ") << (reversed ? _T("True") : _T("False")) << _T(")\n");
        }

		// start - assume we are at a suitable clearance height

		// get offset side string
		wxString side_string;
		switch(m_profile_params.m_tool_on_side)
		{
		case CProfileParams::eLeftOrOutside:
			if(reversed)side_string = _T("right");
			else side_string = _T("left");
			break;
		case CProfileParams::eRightOrInside:
			if(reversed)side_string = _T("left");
			else side_string = _T("right");
			break;
		default:
			side_string = _T("on");
			break;
		}

		// roll on
		switch(m_profile_params.m_tool_on_side)
		{
		case CProfileParams::eLeftOrOutside:
		case CProfileParams::eRightOrInside:
			{
				if(m_profile_params.m_auto_roll_on)
				{
					python << wxString(_T("roll_on = 'auto'\n"));
				}
				else if(! m_profile_params.m_roll_on_point.AsPoint().IsEqual(gp_Pnt(), 0.000000001))
				{
					python << wxString(_T("roll_on = area.Point(")) << m_profile_params.m_roll_on_point.X() / scale << wxString(_T(", "))
					       << m_profile_params.m_roll_on_point.Y() / scale << wxString(_T(")\n"));
				}
                else
                {
                    python << _T("roll_off = None\n");
                }
			}
			break;
		default:
			{
				python << _T("roll_on = None\n");
			}
			break;
		}

		// rapid across to it
		//python << wxString::Format(_T("rapid(%s)\n"), roll_on_string.c_str()).c_str();

		switch(m_profile_params.m_tool_on_side)
		{
		case CProfileParams::eLeftOrOutside:
		case CProfileParams::eRightOrInside:
			{
				if(m_profile_params.m_auto_roll_off)
				{
					python << wxString(_T("roll_off = 'auto'\n"));
				}
				else if(! m_profile_params.m_roll_off_point.AsPoint().IsEqual(gp_Pnt(), 0.000000001))
				{
					python << wxString(_T("roll_off = area.Point(")) << m_profile_params.m_roll_off_point.X() / scale << wxString(_T(", "))
					       << m_profile_params.m_roll_off_point.Y() / scale << wxString(_T(")\n"));
				}
				else
				{
				    python << _T("roll_off = None\n");
				}
			}
			break;
		default:
			{
				python << _T("roll_off = None\n");
			}
			break;
		}

		bool tags_cleared = false;
		for(CTag* tag = (CTag*)(m_tags->GetFirstChild()); tag; tag = (CTag*)(m_tags->GetNextChild()))
		{
			if(!tags_cleared)python << _T("kurve_funcs.clear_tags()\n");
			tags_cleared = true;
			python << _T("kurve_funcs.add_tag(area.Point(") << tag->m_pos.X() / scale
			       << _T(", ") << tag->m_pos.Y() / scale
			       << _T("), ") << tag->m_width / scale
			       << _T(", ") << tag->m_angle * PI/180
			       << _T(", ") << tag->m_height / scale << _T(")\n");
		}
        //extend_at_start, extend_at_end
        python << _T("extend_at_start= ") << m_profile_params.m_extend_at_start / scale << _T("\n");
        python << _T("extend_at_end= ") << m_profile_params.m_extend_at_end / scale << _T("\n");

        //lead in lead out line length
        python << _T("lead_in_line_len= ") << m_profile_params.m_lead_in_line_len / scale << _T("\n");
        python << _T("lead_out_line_len= ") << m_profile_params.m_lead_out_line_len / scale << _T("\n");

        // profile the kurve
        python << wxString::Format(_T("kurve_funcs.profile(curve, '%s', tool_diameter/2, offset_extra, roll_radius, roll_on, roll_off, depthparams, extend_at_start, extend_at_end, lead_in_line_len, lead_out_line_len)\n"), side_string.c_str());
	}
	python << _T("absolute()\n");
	return(python);
}

void CProfile::WriteDefaultValues()
{
    CSketchOp::WriteDefaultValues();

	CNCConfig config(CProfileParams::ConfigScope());
	config.Write(_T("ToolOnSide"), m_profile_params.m_tool_on_side);
	config.Write(_T("CutMode"), m_profile_params.m_cut_mode);
	config.Write(_T("RollRadius"), m_profile_params.m_auto_roll_radius);
	config.Write(_T("OffsetExtra"), m_profile_params.m_offset_extra);
	config.Write(_T("DoFinishPass"), m_profile_params.m_do_finishing_pass);
	config.Write(_T("OnlyFinishPass"), m_profile_params.m_only_finishing_pass);
	config.Write(_T("FinishFeedRate"), m_profile_params.m_finishing_h_feed_rate);
	config.Write(_T("FinishCutMode"), m_profile_params.m_finishing_cut_mode);
	config.Write(_T("FinishStepDown"), m_profile_params.m_finishing_step_down);
	config.Write(_T("EndBeyond"), m_profile_params.m_end_beyond_full_profile);

	config.Write(_T("ExtendAtStart"), m_profile_params.m_extend_at_start);
	config.Write(_T("ExtendAtEnd"), m_profile_params.m_extend_at_end);
	config.Write(_T("LeadInLineLen"), m_profile_params.m_lead_in_line_len);
	config.Write(_T("LeadOutLineLen"), m_profile_params.m_lead_out_line_len);

}

void CProfile::ReadDefaultValues()
{
    CSketchOp::ReadDefaultValues();

	CNCConfig config(CProfileParams::ConfigScope());
	int int_side = m_profile_params.m_tool_on_side;
	config.Read(_T("ToolOnSide"), &int_side, CProfileParams::eLeftOrOutside);
	m_profile_params.m_tool_on_side = (CProfileParams::eSide)int_side;
	int int_mode = m_profile_params.m_cut_mode;
	config.Read(_T("CutMode"), &int_mode, CProfileParams::eConventional);
	m_profile_params.m_cut_mode = (CProfileParams::eCutMode)int_mode;
	config.Read(_T("RollRadius"), m_profile_params.m_auto_roll_radius, 2.0);
	config.Read(_T("OffsetExtra"), m_profile_params.m_offset_extra, 0.0);
	config.Read(_T("DoFinishPass"), m_profile_params.m_do_finishing_pass, false);
	config.Read(_T("OnlyFinishPass"), m_profile_params.m_only_finishing_pass, false);
	config.Read(_T("FinishFeedRate"), m_profile_params.m_finishing_h_feed_rate, 100.0);
	config.Read(_T("FinishCutMode"), &int_mode, CProfileParams::eConventional);
	m_profile_params.m_finishing_cut_mode = (CProfileParams::eCutMode)int_mode;
	config.Read(_T("FinishStepDown"), m_profile_params.m_finishing_step_down, 1.0);
	config.Read(_T("EndBeyond"), m_profile_params.m_end_beyond_full_profile, false);

	config.Read(_T("ExtendAtStart"), m_profile_params.m_extend_at_start, 0.0);
	config.Read(_T("ExtendAtEnd"), m_profile_params.m_extend_at_end, 0.0);
	config.Read(_T("LeadInLineLen"), m_profile_params.m_lead_in_line_len, 0.0);
	config.Read(_T("LeadOutLineLen"), m_profile_params.m_lead_out_line_len, 0.0);
}

Python CProfile::AppendTextToProgram()
{
    Python python;

    // only do finish pass for non milling cutters
    if(!CTool::IsMillingToolType(CTool::FindToolType(m_tool_number)))
    {
        this->m_profile_params.m_only_finishing_pass = true;
    }

    // roughing pass
    if(!this->m_profile_params.m_do_finishing_pass || !this->m_profile_params.m_only_finishing_pass)
    {
        python << AppendTextToProgram(false);
    }

    // finishing pass
    if(this->m_profile_params.m_do_finishing_pass)
    {
        python << AppendTextToProgram(true);
    }

    return python;
}

Python CProfile::AppendTextToProgram(bool finishing_pass)
{
    Python python;
    double scale = Length::Conversion(theApp.m_program->m_units, UnitTypeMillimeter);

    CTool *pTool = CTool::Find( m_tool_number );
    if (pTool == NULL)
    {
        if(!finishing_pass)
            wxMessageBox(_T("Cannot generate GCode for profile without a tool assigned"));
        return(python);
    } // End if - then

    if(!finishing_pass || m_profile_params.m_only_finishing_pass)
    {
        python << CSketchOp::AppendTextToProgram();

        if(m_profile_params.m_auto_roll_on || m_profile_params.m_auto_roll_off)
        {
            python << _T("roll_radius = float(");
            python << m_profile_params.m_auto_roll_radius / scale;
            python << _T(")\n");
        }
    }

    if(finishing_pass)
    {
        python << _T("feedrate_hv(") << m_profile_params.m_finishing_h_feed_rate / scale << _T(", ");
        python << m_speed_op_params.m_vertical_feed_rate / scale << _T(")\n");
        python << _T("flush_nc()\n");
        python << _T("offset_extra = 0.0\n");
        python << _T("depthparams.step_down = ") << m_profile_params.m_finishing_step_down << _T("\n");
    }
    else
    {
        python << _T("offset_extra = ") << m_profile_params.m_offset_extra / scale << _T("\n");
    }

    CProfileParams::eCutMode cut_mode = finishing_pass ? CProfileParams::eCutMode((int)m_profile_params.m_finishing_cut_mode)
                                                       : CProfileParams::eCutMode((int)m_profile_params.m_cut_mode);

    HeeksObj* object = heeksCAD->GetIDObject(SketchType, m_sketch);
    if(object)
    {
        HeeksObj* sketch_to_be_deleted = NULL;
        if(object->GetType() == AreaType)
        {
            object = heeksCAD->NewSketchFromArea(object);
            sketch_to_be_deleted = object;
        }

        if(object->GetType() == SketchType)
        {
            std::list<HeeksObj*> new_separate_sketches;
            heeksCAD->ExtractSeparateSketches(object, new_separate_sketches, false);
            for(std::list<HeeksObj*>::iterator It = new_separate_sketches.begin(); It != new_separate_sketches.end(); It++)
            {
                HeeksObj* one_curve_sketch = *It;
                python << AppendTextForSketch(one_curve_sketch, cut_mode).c_str();
                delete one_curve_sketch;
            }
        }
        else
        {
            python << AppendTextForSketch(object, cut_mode).c_str();
        }

        if(sketch_to_be_deleted)
        {
            delete sketch_to_be_deleted;
        }
    }

	return python;
} // End AppendTextToProgram() method

static unsigned char cross16[32] = {0x80, 0x01, 0x40, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20, 0x02, 0x40, 0x01, 0x80, 0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01};

void CProfile::glCommands(bool select, bool marked, bool no_color)
{
    double point[3];
    CSketchOp::glCommands(select, marked, no_color);

	if(marked && !no_color)
	{
        // draw roll on point
        if(!m_profile_params.m_auto_roll_on)
        {
            extract(m_profile_params.m_roll_on_point, point);
            glColor3ub(0, 200, 200);
            glRasterPos3dv(point);
            glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
        }
        // draw roll off point
        if(!m_profile_params.m_auto_roll_on)
        {
            extract(m_profile_params.m_roll_off_point, point);
            glColor3ub(255, 128, 0);
            glRasterPos3dv(point);
            glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
        }
        // draw start point
        if(m_profile_params.m_start_given)
        {
            extract(m_profile_params.m_start, point);
            glColor3ub(128, 0, 255);
            glRasterPos3dv(point);
            glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
        }
        // draw end point
        if(m_profile_params.m_end_given)
        {
            extract(m_profile_params.m_end, point);
            glColor3ub(200, 200, 0);
            glRasterPos3dv(point);
            glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
        }
	}
}

void CProfile::GetProperties(std::list<Property *> *list)
{
	m_profile_params.GetProperties(list);

	CSketchOp::GetProperties(list);
}


static CProfile* object_for_tools = NULL;

class PickStart: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick Start");}
    void Run ( )
    {
        double point[3];
        if ( heeksCAD->PickPosition ( _( "Pick new start point" ), point ) ) {
            object_for_tools->m_profile_params.m_start = make_point ( point );
            object_for_tools->m_profile_params.m_start_given = true;
        }
    }
	wxString BitmapPath(){ return theApp.GetResFolder() + _T("/bitmaps/pickstart.png"); }
};

static PickStart pick_start;

class PickEnd: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick End");}
    void Run ( )
    {
        double point[3];
        if ( heeksCAD->PickPosition ( _( "Pick new end point" ), point ) ) {
            object_for_tools->m_profile_params.m_end = make_point ( point );
            object_for_tools->m_profile_params.m_end_given = true;
        }
        heeksCAD->RefreshProperties ( );
    }
	wxString BitmapPath(){ return theApp.GetResFolder() + _T("/bitmaps/pickend.png"); }
};

static PickEnd pick_end;

class PickRollOn: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick roll on point");}
    void Run ( )
    {
        double point[3];
        if ( heeksCAD->PickPosition ( _( "Pick roll on point" ), point ) ) {
            object_for_tools->m_profile_params.m_roll_on_point = make_point ( point );
            object_for_tools->m_profile_params.m_auto_roll_on = false;
        }
    }
	wxString BitmapPath(){ return theApp.GetResFolder() + _T("/bitmaps/rollon.png"); }
};

static PickRollOn pick_roll_on;

class PickRollOff: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick roll off point");}
    void Run ( )
    {
        double point[3];
        if ( heeksCAD->PickPosition ( _( "Pick roll off point" ), point ) ) {
            object_for_tools->m_profile_params.m_roll_off_point = make_point ( point );
            object_for_tools->m_profile_params.m_auto_roll_off = false;
        }
    }
	wxString BitmapPath(){ return theApp.GetResFolder() + _T("/bitmaps/rolloff.png"); }
};

static PickRollOff pick_roll_off;


class TagAddingMode: public CInputMode, CLeftAndRight {
private:
    wxPoint clicked_point;

public:
    // virtual functions for InputMode
    const wxChar* GetTitle(){return _("Add tags by clicking");}
    void OnMouse( wxMouseEvent& event );
    void OnKeyDown(wxKeyEvent& event);
    void GetTools(std::list<Tool*> *f_list, const wxPoint *p);
};

void TagAddingMode::OnMouse( wxMouseEvent& event )
{
    bool event_used = false;

    if(LeftAndRightPressed(event, event_used))
    {
        heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
    }

    if(!event_used)
    {
        if(event.MiddleIsDown() || event.GetWheelRotation() != 0)
        {
            heeksCAD->GetSelectMode()->OnMouse(event);
        }
        else
        {
            if(event.LeftDown())
            {
                double pos[3];
                heeksCAD->Digitize(event.GetPosition(), pos);
            }
            else if(event.LeftUp())
            {
                double pos[3];
                if(heeksCAD->GetLastDigitizePosition(pos))
                {
                    if (!object_for_tools)
                    {
                        HeeksObj* owner = NULL;
                        HeeksObj* marked = heeksCAD->GetMarkedList().front();
                        if (marked->GetType() == TagsType)
                        {
                            owner = marked->GetOwner();
                        }

                        if (!owner || owner->GetType() != ProfileType)
                        {
                            wxMessageBox(_T("Please select a Tags object"));
                            return;
                        }
                        object_for_tools = (CProfile *)owner;
                    }
                    CTag* new_object = new CTag();
                    new_object->m_pos.SetX(pos[0]);
                    new_object->m_pos.SetY(pos[1]);
                    heeksCAD->StartHistory();
                    heeksCAD->AddUndoably(new_object, object_for_tools->Tags());
                    heeksCAD->EndHistory();
                }
            }
            else if(event.RightUp())
            {
                // do context menu same as select mode
                heeksCAD->GetSelectMode()->OnMouse(event);
            }
        }
    }
}

void TagAddingMode::OnKeyDown(wxKeyEvent& event)
{
        switch(event.GetKeyCode()){
        case WXK_F1:
        case WXK_RETURN:
        case WXK_ESCAPE:
                // end drawing mode
                heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
        }
}

class EndDrawing:public Tool {
public:
        void Run(){heeksCAD->SetInputMode(heeksCAD->GetSelectMode());}
        const wxChar* GetTitle(){return _("Stop adding tags");}
        wxString BitmapPath(){return _T("enddraw");}
};

static EndDrawing end_drawing;

void TagAddingMode::GetTools(std::list<Tool*> *f_list, const wxPoint *p){
        f_list->push_back(&end_drawing);
}


TagAddingMode tag_adding_mode;

class AddTagTool: public Tool
{
public:
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Add Tag");}

	void Run()
	{
	    heeksCAD->SetInputMode(&tag_adding_mode);
	}
	bool CallChangedOnRun(){return false;}
	wxString BitmapPath(){ return theApp.GetResFolder() + _T("/bitmaps/addtag.png"); }
};

static AddTagTool add_tag_tool;

void CProfile::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_tools = this;
	t_list->push_back(&pick_start);
	t_list->push_back(&pick_end);
	t_list->push_back(&pick_roll_on);
	t_list->push_back(&pick_roll_off);
	t_list->push_back(&add_tag_tool);

	CSketchOp::GetTools(t_list, p);
}

HeeksObj *CProfile::MakeACopy(void)const
{
	return new CProfile(*this);
}

void CProfile::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CProfile*)object));
	}
}

bool CProfile::CanAdd(HeeksObj* object)
{
	return ((object != NULL) && (object->GetType() == TagsType || object->GetType() == SketchType || object->GetType() == FixtureType));
}

bool CProfile::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

// static member function
HeeksObj* CProfile::ReadFromXMLElement(TiXmlElement* element)
{
	CProfile* new_object = new CProfile;
	new_object->ReadBaseXML(element);

	new_object->AddMissingChildren();

	return new_object;
}

void CProfile::AddMissingChildren()
{
	// make sure "tags" exists
	if(m_tags == NULL)
	{
	    m_tags = new CTags;
	    Add( m_tags, NULL );
	}
}

static void on_set_spline_deviation(double value, HeeksObj* object){
	CProfile::max_deviation_for_spline_to_arc = value;
	CProfile::WriteToConfig();
}

// static
void CProfile::GetOptions(std::list<Property *> *list)
{
    list->push_back ( &m_auto_set_speeds_feeds );
}
//	list->push_back ( new PropertyDouble ( _("Profile spline deviation"), max_deviation_for_spline_to_arc, NULL, on_set_spline_deviation ) );


// static
void CProfile::ReadFromConfig()
{
	CNCConfig config(CProfileParams::ConfigScope());
	config.Read(_T("ProfileSplineDeviation"), max_deviation_for_spline_to_arc, 0.01);
}

// static
void CProfile::WriteToConfig()
{
	CNCConfig config(CProfileParams::ConfigScope());
	config.Write(_T("ProfileSplineDeviation"), max_deviation_for_spline_to_arc);
}
bool CProfileParams::operator==( const CProfileParams & rhs ) const
{
	if (m_auto_roll_on != rhs.m_auto_roll_on) return(false);
	if (m_auto_roll_off != rhs.m_auto_roll_off) return(false);
	if (m_auto_roll_radius != rhs.m_auto_roll_radius) return(false);
	if (!m_roll_on_point.IsEqual(rhs.m_roll_on_point, heeksCAD->GetTolerance())) return(false);
	if (!m_roll_off_point.IsEqual(rhs.m_roll_off_point, heeksCAD->GetTolerance())) return(false);
	if (m_start_given != rhs.m_start_given) return(false);
	if (m_end_given != rhs.m_end_given) return(false);
	if (m_end_beyond_full_profile != rhs.m_end_beyond_full_profile) return(false);
	if (!m_start.IsEqual(rhs.m_start, heeksCAD->GetTolerance())) return(false);
	if (!m_end.IsEqual(rhs.m_end, heeksCAD->GetTolerance())) return(false);
	if (m_sort_sketches != rhs.m_sort_sketches) return(false);
	if (m_offset_extra != rhs.m_offset_extra) return(false);
	if (m_do_finishing_pass != rhs.m_do_finishing_pass) return(false);
	if (m_finishing_h_feed_rate != rhs.m_finishing_h_feed_rate) return(false);
	if (m_finishing_cut_mode != rhs.m_finishing_cut_mode) return(false);

	return(true);
}

bool CProfile::operator==( const CProfile & rhs ) const
{
    if (m_profile_params != rhs.m_profile_params) return(false);

    if(m_sketch != rhs.m_sketch)return false;

    return(CSketchOp::operator==(rhs));
}


static bool OnEdit(HeeksObj* object)
{
        return ProfileDlg::Do((CProfile*)object);
}

void CProfile::GetOnEdit(bool(**callback)(HeeksObj*))
{
        *callback = OnEdit;
}

void CProfile::Clear()
{
        if (m_tags != NULL)
            m_tags->Clear();
}
