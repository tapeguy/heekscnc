// Pocket.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Pocket.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/HeeksObj.h"
#include "tinyxml/tinyxml.h"
#include "CTool.h"
#include "CNCPoint.h"
#include "Reselect.h"
#include "PocketDlg.h"

#include <sstream>

/* static */ PropertyDouble CPocket::max_deviation_for_spline_to_arc = 0.1;

CPocketParams::CPocketParams(CPocket* parent)
{
	this->parent = parent;
	InitializeProperties();
	m_step_over = 0.0;
	m_material_allowance = 0.0;
	m_starting_place.SetValue ( true );
	m_post_processor.SetValue ( CPocketParams::eTrochoidal );
	m_zig_angle = 0.0;
	m_entry_move = ePlunge;
}

void CPocketParams::set_initial_values(const CTool::ToolNumber_t tool_number)
{
    if (tool_number > 0)
    {
        CTool *pTool = CTool::Find(tool_number);
        if (pTool != NULL)
        {
            m_step_over = pTool->CuttingRadius() * 3.0 / 5.0;
        }
    }
}

void CPocketParams::InitializeProperties()
{
	m_step_over.Initialize(_("step over"), parent);
	m_material_allowance.Initialize(_("material allowance"), parent);

	m_starting_place.Initialize(_("starting place"), parent);
	m_starting_place.m_choices.push_back(_("boundary"));
	m_starting_place.m_choices.push_back(_("center"));

	m_entry_move.Initialize(_("entry move"), parent);
	m_entry_move.m_choices.push_back(_("plunge"));
	m_entry_move.m_choices.push_back(_("ramp"));
	m_entry_move.m_choices.push_back(_("helical"));

	m_post_processor.Initialize(_("post-processor"), parent);
	m_post_processor.m_choices.push_back(_("ZigZag"));
	m_post_processor.m_choices.push_back(_("ZigZag Unidirectional"));
	m_post_processor.m_choices.push_back(_("Offsets"));
	m_post_processor.m_choices.push_back(_("Trochodial"));

	m_zig_angle.Initialize(_("zig angle"), parent);
}

void CPocketParams::GetProperties(std::list<Property *> *list)
{
    m_zig_angle.SetVisible(IsZigZag());
}

static wxString WriteSketchDefn(HeeksObj* sketch)
{
#ifdef UNICODE
	std::wostringstream gcode;
#else
    std::ostringstream gcode;
#endif
    gcode.imbue(std::locale("C"));
	gcode << std::setprecision(10);

	bool started = false;

	double prev_e[3];

	std::list<HeeksObj*> new_spans;
	for(HeeksObj* span = sketch->GetFirstChild(); span; span = sketch->GetNextChild())
	{
		if(span->GetType() == SplineType)
		{
			heeksCAD->SplineToBiarcs(span, new_spans, CPocket::max_deviation_for_spline_to_arc);
		}
		else
		{
			new_spans.push_back(span->MakeACopy());
		}
	}

	for(std::list<HeeksObj*>::iterator It = new_spans.begin(); It != new_spans.end(); It++)
	{
		HeeksObj* span_object = *It;

		double s[3] = {0, 0, 0};
		double e[3] = {0, 0, 0};
		double c[3] = {0, 0, 0};

		if(span_object){
			int type = span_object->GetType();

			if(type == LineType || type == ArcType)
			{
				span_object->GetStartPoint(s);

				CNCPoint start(s);

				if(started && (fabs(s[0] - prev_e[0]) > 0.0001 || fabs(s[1] - prev_e[1]) > 0.0001))
				{
					gcode << _T("a.append(c)\n");
					started = false;
				}

				if(!started)
				{
					gcode << _T("c = area.Curve()\n");
					gcode << _T("c.append(area.Vertex(0, area.Point(") << start.X(true) << _T(", ") << start.Y(true) << _T("), area.Point(0, 0)))\n");
					started = true;
				}
				span_object->GetEndPoint(e);
				CNCPoint end(e);

				if(type == LineType)
				{
					gcode << _T("c.append(area.Vertex(0, area.Point(") << end.X(true) << _T(", ") << end.Y(true) << _T("), area.Point(0, 0)))\n");
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					CNCPoint centre(c);

					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					gcode << _T("c.append(area.Vertex(") << span_type << _T(", area.Point(") << end.X(true) << _T(", ") << end.Y(true);
					gcode << _T("), area.Point(") << centre.X(true) << _T(", ") << centre.Y(true) << _T(")))\n");
				}
				memcpy(prev_e, e, 3*sizeof(double));
			} // End if - then
			else
			{
				if (type == CircleType)
				{
					if(started)
					{
						gcode << _T("a.append(c)\n");
						started = false;
					}

					std::list< std::pair<int, gp_Pnt > > points;
					span_object->GetCentrePoint(c);

					// Setup the four arcs that will make up the circle using UNadjusted
					// coordinates first so that the offsets align with the X and Y axes.
					double radius = heeksCAD->CircleGetRadius(span_object);

					points.push_back( std::make_pair(0, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					points.push_back( std::make_pair(-1, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
					points.push_back( std::make_pair(-1, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
					points.push_back( std::make_pair(-1, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
					points.push_back( std::make_pair(-1, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north

					CNCPoint centre(c);

					gcode << _T("c = area.Curve()\n");
					for (std::list< std::pair<int, gp_Pnt > >::iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
					{
						CNCPoint pnt( l_itPoint->second );

						gcode << _T("c.append(area.Vertex(") << l_itPoint->first << _T(", area.Point(");
						gcode << pnt.X(true) << (_T(", ")) << pnt.Y(true);
						gcode << _T("), area.Point(") << centre.X(true) << _T(", ") << centre.Y(true) << _T(")))\n");
					} // End for
					gcode << _T("a.append(c)\n");
				}
			} // End if - else
		}
	}

	if(started)
	{
		gcode << _T("a.append(c)\n");
		started = false;
	}

	// delete the spans made
	for(std::list<HeeksObj*>::iterator It = new_spans.begin(); It != new_spans.end(); It++)
	{
		HeeksObj* span = *It;
		delete span;
	}

	gcode << _T("\n");
	return(wxString(gcode.str().c_str()));
}

const wxBitmap &CPocket::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/pocket.png")));
	return *icon;
}

void CPocket::WritePocketPython(Python &python)
{
    double scale = Length::Conversion(theApp.m_program->m_units, UnitTypeMillimeter);

    // start - assume we are at a suitable clearance height

    // make a parameter of area_funcs.pocket() eventually
    // 0..plunge, 1..ramp, 2..helical
    python << _T("entry_style = ") <<  m_pocket_params.m_entry_move << _T("\n");

    // Pocket the area
    python << _T("area_funcs.pocket(a, tool_diameter/2, ");
    python << m_pocket_params.m_material_allowance / scale;
    python << _T(", ") << m_pocket_params.m_step_over / scale;
    python << _T(", depthparams, ");
    python << m_pocket_params.m_starting_place;
    python << _T(", ");
    python << (m_pocket_params.m_post_processor == CPocketParams::eZigZag ? _T("'zigzag'") :
               m_pocket_params.m_post_processor == CPocketParams::eZigZagUnidirectional ? _T("'zigzag-unidirectional'") :
               m_pocket_params.m_post_processor == CPocketParams::eOffsets ? _T("'offsets'") : _T("'trochoidal'"));
    python << _T(", ") << m_pocket_params.m_zig_angle;
    python << _T(", None, "); // start point
    python << ((m_pocket_params.m_cut_mode == CPocketParams::eClimb) ? _T("'climb'") : _T("'conventional'"));
    python << _T(")\n");

    // rapid back up to clearance plane
    python << _T("rapid(z = depthparams.clearance_height)\n");
}

Python CPocket::AppendTextToProgram()
{
	Python python;

	CTool *pTool = CTool::Find( m_tool_number );
	if (pTool == NULL)
	{
		wxMessageBox(_T("Cannot generate GCode for pocket without a tool assigned"));
		return(python);
	} // End if - then


	python << CSketchOp::AppendTextToProgram();

    HeeksObj* object = heeksCAD->GetIDObject(SketchType, m_sketch);

    if(object == NULL) {
        wxMessageBox(wxString::Format(_("Pocket operation - Sketch doesn't exist")));
        return python;
    }

    int type = object->GetType();

    // do areas and circles first, separately
    {
        switch(type)
        {
        case CircleType:
        case AreaType:
            {
                heeksCAD->ObjectAreaString(object, python);
                WritePocketPython(python);
            }
            break;
        }
    }

    if(type == SketchType)
    {
        python << _T("a = area.Area()\n");
        python << _T("entry_moves = []\n");

        if (object->GetNumChildren() == 0){
            wxMessageBox(wxString::Format(_("Pocket operation - Sketch %d has no children"), object->GetID()));
            return python;
        }

        HeeksObj* re_ordered_sketch = NULL;
        SketchOrderType order = heeksCAD->GetSketchOrder(object);
        if(     (order != SketchOrderTypeCloseCW) &&
            (order != SketchOrderTypeCloseCCW) &&
            (order != SketchOrderTypeMultipleCurves) &&
            (order != SketchOrderHasCircles))
        {
            re_ordered_sketch = object->MakeACopy();
            heeksCAD->ReOrderSketch(re_ordered_sketch, SketchOrderTypeReOrder);
            object = re_ordered_sketch;
            order = heeksCAD->GetSketchOrder(object);
            if( (order != SketchOrderTypeCloseCW) &&
                (order != SketchOrderTypeCloseCCW) &&
                (order != SketchOrderTypeMultipleCurves) &&
                (order != SketchOrderHasCircles))
            {
                switch(heeksCAD->GetSketchOrder(object))
                {
                case SketchOrderTypeOpen:
                    {
                        wxMessageBox(wxString::Format(_("Pocket operation - Sketch must be a closed shape - sketch %d"), object->GetID()));
                        delete re_ordered_sketch;
                        return python;
                    }
                    break;

                default:
                    {
                        wxMessageBox(wxString::Format(_("Pocket operation - Badly ordered sketch - sketch %d"), object->GetID()));
                        delete re_ordered_sketch;
                        return python;
                    }
                    break;
                }
            }
        }

        if(object)
        {
            python << WriteSketchDefn(object);
        }

        if(re_ordered_sketch)
        {
            delete re_ordered_sketch;
        }

    } // End for

    // reorder the area, the outside curves must be made anti-clockwise and the insides clockwise
    python << _T("a.Reorder()\n");

    WritePocketPython(python);

	return(python);

} // End AppendTextToProgram() method


void CPocket::WriteDefaultValues()
{
    CSketchOp::WriteDefaultValues();

	CNCConfig config(CPocketParams::ConfigScope());
	config.Write(_T("StepOver"), m_pocket_params.m_step_over);
	config.Write(_T("MaterialAllowance"), m_pocket_params.m_material_allowance);
	config.Write(_T("FromCenter"), m_pocket_params.m_starting_place);
	config.Write(_T("PostProcessor"), m_pocket_params.m_post_processor);
	config.Write(_T("ZigAngle"), m_pocket_params.m_zig_angle);
	config.Write(_T("DecentStrategy"), m_pocket_params.m_entry_move);
}

void CPocket::ReadDefaultValues()
{
    CSketchOp::ReadDefaultValues();

	CNCConfig config(CPocketParams::ConfigScope());
	config.Read(_T("StepOver"), m_pocket_params.m_step_over, 1.0);
	config.Read(_T("MaterialAllowance"), m_pocket_params.m_material_allowance, 0.2);
	config.Read(_T("FromCenter"), m_pocket_params.m_starting_place, 1);
	config.Read(_T("PostProcessor"), m_pocket_params.m_post_processor, CPocketParams::eTrochoidal);
	config.Read(_T("ZigAngle"), m_pocket_params.m_zig_angle);
	int int_for_entry_move = CPocketParams::ePlunge;
	config.Read(_T("DecentStrategy"), &int_for_entry_move);
	m_pocket_params.m_entry_move = (CPocketParams::eEntryStyle) int_for_entry_move;
}

// static member function
HeeksObj* CPocket::ReadFromXMLElement(TiXmlElement* element)
{
    CPocket* new_object = new CPocket;
    new_object->ReadBaseXML(element);
    return new_object;
}

void CPocket::glCommands(bool select, bool marked, bool no_color)
{
    CSketchOp::glCommands( select, marked, no_color );
}

void CPocket::GetProperties(std::list<Property *> *list)
{
    m_pocket_params.GetProperties(list);
    CSketchOp::GetProperties(list);
}

HeeksObj *CPocket::MakeACopy(void)const
{
	return new CPocket(*this);
}

void CPocket::CopyFrom(const HeeksObj* object)
{
	operator=(*((CPocket*)object));
}

CPocket::CPocket( const CPocket & rhs ) : CSketchOp(rhs), m_pocket_params(this)
{
	m_pocket_params = rhs.m_pocket_params;
}

CPocket & CPocket::operator= ( const CPocket & rhs )
{
    if (this != &rhs)
    {
        CSketchOp::operator=(rhs);
        m_pocket_params = rhs.m_pocket_params;
    }

    return(*this);
}

bool CPocket::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

CPocket::CPocket(int sketch, const int tool_number )
    : CSketchOp(sketch, tool_number, PocketType ), m_pocket_params(this)
{
    ReadDefaultValues();
    m_pocket_params.set_initial_values(tool_number);
}

// static
void CPocket::ReadFromConfig()
{
	CNCConfig config(CPocketParams::ConfigScope());
	config.Read(_T("PocketSplineDeviation"), max_deviation_for_spline_to_arc, 0.1);
}

// static
void CPocket::WriteToConfig()
{
	CNCConfig config(CPocketParams::ConfigScope());
	config.Write(_T("PocketSplineDeviation"), max_deviation_for_spline_to_arc);
}

void CPocket::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CSketchOp::GetTools( t_list, p );
}

bool CPocketParams::operator==(const CPocketParams & rhs) const
{
	if (m_starting_place != rhs.m_starting_place) return(false);
	if (m_material_allowance != rhs.m_material_allowance) return(false);
	if (m_step_over != rhs.m_step_over) return(false);
	if (m_post_processor != rhs.m_post_processor) return(false);
	if (m_zig_angle != rhs.m_zig_angle) return(false);
	if (m_entry_move != rhs.m_entry_move) return(false);

	return(true);
}

bool CPocket::operator==(const CPocket & rhs) const
{
	if (m_pocket_params != rhs.m_pocket_params) return(false);

	return(CSketchOp::operator==(rhs));
}

static bool OnEdit(HeeksObj* object)
{
    return PocketDlg::Do((CPocket*)object);
}

void CPocket::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}

bool CPocket::Add(HeeksObj* object, HeeksObj* prev_object)
{
	return CSketchOp::Add(object, prev_object);
}
