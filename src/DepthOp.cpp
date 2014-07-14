// DepthOp.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "DepthOp.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CTool.h"
#include "MachineState.h"


CDepthOpParams::CDepthOpParams(CDepthOp * parent)
{
	this->parent = parent;
	m_abs_mode = eAbsolute;
	m_clearance_height = 0.0;
	m_start_depth = 0.0;
	m_step_down = 0.0;
	m_final_depth = 0.0;
	m_rapid_safety_space = 0.0;

}

CDepthOp & CDepthOp::operator= ( const CDepthOp & rhs )
{
	if (this != &rhs)
	{
		CSpeedOp::operator=( rhs );
		m_depth_op_params = rhs.m_depth_op_params;
	}

	return(*this);
}

CDepthOp::CDepthOp(const wxString& title, const std::list<int> *sketches, const int tool_number, const int operation_type)
 : CSpeedOp(title, tool_number, operation_type), m_depth_op_params(this)
{
	ReadDefaultValues();
	SetDepthsFromSketchesAndTool(sketches);
}

CDepthOp::CDepthOp(const wxString& title, const std::list<HeeksObj *> sketches, const int tool_number, const int operation_type)
 : CSpeedOp(title, tool_number, operation_type), m_depth_op_params(this)
{
	ReadDefaultValues();
	SetDepthsFromSketchesAndTool(sketches);
}

CDepthOp::CDepthOp( const CDepthOp & rhs ) : CSpeedOp(rhs), m_depth_op_params(this)
{
	m_depth_op_params = rhs.m_depth_op_params;
}

void CDepthOp::ReloadPointers()
{
	CSpeedOp::ReloadPointers();
}

static double degrees_to_radians( const double degrees )
{
	return( (degrees / 360.0) * (2 * PI) );
} // End degrees_to_radians() routine

void CDepthOp::OnPropertyEdit(Property * prop)
{
	this->WriteDefaultValues();
}

void CDepthOpParams::InitializeProperties()
{
	m_abs_mode.Initialize(_("ABS/INCR mode"), parent);
	m_abs_mode.m_choices.push_back(_("Absolute"));
	m_abs_mode.m_choices.push_back(_("Incremental"));

	m_clearance_height.Initialize(_("Clearance height"), parent);		// Either/Or
	m_clearance_height_string.Initialize(_("Clearance height"), parent);
	m_clearance_height_string.SetReadOnly(true);

	m_rapid_safety_space.Initialize(_("rapid safety space"), parent);

	//My initial thought was that extrusion operatons would always start at z=0 and end at z=top of object.  I'm now thinking it might be desireable to preserve this as an option.
	//It might be good to run an operation that prints the bottom half of the object, pauses to allow insertion of something.  Then another operation could print the top half.

	m_start_depth.Initialize(_("start depth"), parent);
	m_final_depth.Initialize(_("final depth"), parent);

	//Step down doesn't make much sense for extrusion.  The amount the z axis steps up or down is equal to the layer thickness of the slice which
	//is determined by the thickness of an extruded filament.  Step up is very important since it is directly related to the resolution of the final
	//produce.
	m_step_down.Initialize(_("step down"), parent);
}

void CDepthOpParams::GetProperties(std::list<Property *> *list)
{
	switch(theApp.m_program->m_clearance_source)
	{
	case CProgram::eClearanceDefinedByFixture:
		m_clearance_height_string = _("Defined in fixture definition");
		m_clearance_height_string.SetVisible(true);
		m_clearance_height.SetVisible(false);
		break;

	case CProgram::eClearanceDefinedByMachine:
		m_clearance_height_string = _("Defined in Program properties for whole machine");
		m_clearance_height_string.SetVisible(true);
		m_clearance_height.SetVisible(false);
		break;

	case CProgram::eClearanceDefinedByOperation:
	default:
		m_clearance_height_string.SetVisible(false);
		m_clearance_height.SetVisible(true);
		break;
	} // End switch

	DomainObject::GetProperties(list);
}

void CDepthOpParams::WriteXMLAttributes(TiXmlNode* pElem)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "depthop" );
	heeksCAD->LinkXMLEndChild( pElem,  element );
	element->SetDoubleAttribute( "clear", m_clearance_height);
	element->SetDoubleAttribute( "down", m_step_down);
	element->SetDoubleAttribute( "startdepth", m_start_depth);
	element->SetDoubleAttribute( "depth", m_final_depth);
	element->SetDoubleAttribute( "r", m_rapid_safety_space);
	element->SetAttribute( "abs_mode", m_abs_mode);
}

void CDepthOpParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	TiXmlElement* depthop = heeksCAD->FirstNamedXMLChildElement(pElem, "depthop");
	if(depthop)
	{	int int_for_enum;
		double clearance_height, step_down, start_depth, final_depth, rapid_safety_space;
		depthop->Attribute("clear", &clearance_height);
		m_clearance_height = clearance_height;
		depthop->Attribute("down", &step_down);
		m_step_down = step_down;
		depthop->Attribute("startdepth", &start_depth);
		m_start_depth = start_depth;
		depthop->Attribute("depth", &final_depth);
		m_final_depth = final_depth;
		depthop->Attribute("r", &rapid_safety_space);
		m_rapid_safety_space = rapid_safety_space;
		if(pElem->Attribute("abs_mode", &int_for_enum)) {
			m_abs_mode = (eAbsMode)int_for_enum;
		}
		heeksCAD->RemoveXMLChild(pElem, depthop);	// We don't want to interpret this again when
								// the ObjList::ReadBaseXML() method gets to it.
	}
}

void CDepthOp::glCommands(bool select, bool marked, bool no_color)
{
	CSpeedOp::glCommands(select, marked, no_color);
}

void CDepthOp::WriteBaseXML(TiXmlElement *element)
{
	m_depth_op_params.WriteXMLAttributes(element);
	CSpeedOp::WriteBaseXML(element);
}

void CDepthOp::ReadBaseXML(TiXmlElement* element)
{
	m_depth_op_params.ReadFromXMLElement(element);
	CSpeedOp::ReadBaseXML(element);
}

void CDepthOp::GetProperties(std::list<Property *> *list)
{
	m_depth_op_params.GetProperties(list);
	CSpeedOp::GetProperties(list);
}

void CDepthOp::WriteDefaultValues()
{
	CSpeedOp::WriteDefaultValues();

	CNCConfig config(GetTypeString());
	config.Write(_T("ClearanceHeight"), m_depth_op_params.ClearanceHeight());
	config.Write(_T("StartDepth"), m_depth_op_params.m_start_depth);
	config.Write(_T("StepDown"), m_depth_op_params.m_step_down);
	config.Write(_T("FinalDepth"), m_depth_op_params.m_final_depth);
	config.Write(_T("RapidDown"), m_depth_op_params.m_rapid_safety_space);
	config.Write(_T("ABSMode"), m_depth_op_params.m_abs_mode);
}

void CDepthOp::ReadDefaultValues()
{
	CSpeedOp::ReadDefaultValues();

	CNCConfig config(GetTypeString());

	double clearance_height;
	config.Read(_T("ClearanceHeight"), &clearance_height, 5.0);
	m_depth_op_params.ClearanceHeight(clearance_height);

	config.Read(_T("StartDepth"), m_depth_op_params.m_start_depth, 0.0);
	config.Read(_T("StepDown"), m_depth_op_params.m_step_down, 1.0);
	config.Read(_T("FinalDepth"), m_depth_op_params.m_final_depth, -1.0);
	config.Read(_T("RapidDown"), m_depth_op_params.m_rapid_safety_space, 2.0);
	int int_mode = m_depth_op_params.m_abs_mode;
	config.Read(_T("ABSMode"), &int_mode, CDepthOpParams::eAbsolute);
	m_depth_op_params.m_abs_mode = (CDepthOpParams::eAbsMode)int_mode;
}

void CDepthOp::SetDepthsFromSketchesAndTool(const std::list<int> *sketches)
{
	std::list<HeeksObj *> objects;

	if (sketches != NULL)
	{
		for (std::list<int>::const_iterator l_itSketch = sketches->begin(); l_itSketch != sketches->end(); l_itSketch++)
		{
			HeeksObj *pSketch = heeksCAD->GetIDObject( SketchType, *l_itSketch );
			if (pSketch != NULL)
			{
				objects.push_back(pSketch);
			}
		}
	}

	SetDepthsFromSketchesAndTool( objects );
}

void CDepthOp::SetDepthsFromSketchesAndTool(const std::list<HeeksObj *> sketches)
{
	for (std::list<HeeksObj *>::const_iterator l_itSketch = sketches.begin(); l_itSketch != sketches.end(); l_itSketch++)
	{
		double default_depth = 1.0;	// mm
		HeeksObj *pSketch = *l_itSketch;
		if (pSketch != NULL)
		{
			CBox bounding_box;
			pSketch->GetBox( bounding_box );

			if (l_itSketch == sketches.begin())
			{
				// This is the first cab off the rank.

				m_depth_op_params.m_start_depth = bounding_box.MaxZ();
				m_depth_op_params.m_final_depth = m_depth_op_params.m_start_depth - default_depth;
			} // End if - then
			else
			{
				// We've seen some before.  If this one is higher up then use
				// that instead.

				if (m_depth_op_params.m_start_depth < bounding_box.MaxZ())
				{
					m_depth_op_params.m_start_depth = bounding_box.MaxZ();
				} // End if - then

				if (m_depth_op_params.m_final_depth > bounding_box.MinZ())
				{
					m_depth_op_params.m_final_depth = bounding_box.MinZ() - default_depth;
				} // End if - then
			} // End if - else
		} // End if - then
	} // End for

	// If we've chosen a chamfering bit, calculate the depth required to give a 1 mm wide
	// chamfer.  It's as good as any width to start with.  If it's not a chamfering bit
	// then we can't even guess as to what the operator wants.

	const double default_chamfer_width = 1.0;	// mm
	if (m_tool_number > 0)
	{
		CTool *pTool = CTool::Find( m_tool_number );
		if (pTool != NULL)
		{
			if ((pTool->m_params.m_type == CToolParams::eChamfer) &&
			    (pTool->m_params.m_cutting_edge_angle > 0))
			{
				m_depth_op_params.m_final_depth = m_depth_op_params.m_start_depth - (default_chamfer_width * tan( degrees_to_radians( 90.0 - pTool->m_params.m_cutting_edge_angle ) ));
			} // End if - then
		} // End if - then
	} // End if - then
}

Python CDepthOp::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

    python << CSpeedOp::AppendTextToProgram(pMachineState);

	python << _T("clearance = float(") << m_depth_op_params.ClearanceHeight() / theApp.m_program->m_units << _T(")\n");
	python << _T("rapid_safety_space = float(") << m_depth_op_params.m_rapid_safety_space / theApp.m_program->m_units << _T(")\n");
    python << _T("start_depth = float(") << m_depth_op_params.m_start_depth / theApp.m_program->m_units << _T(")\n");
    python << _T("step_down = float(") << m_depth_op_params.m_step_down / theApp.m_program->m_units << _T(")\n");
    python << _T("final_depth = float(") << m_depth_op_params.m_final_depth / theApp.m_program->m_units << _T(")\n");

	CTool *pTool = CTool::Find( m_tool_number );
	if (pTool != NULL)
	{
		python << _T("tool_diameter = float(") << (pTool->CuttingRadius(true) * 2.0) << _T(")\n");
		python << _T("cutting_edge_angle = float(") << pTool->m_params.m_cutting_edge_angle<< _T(")\n");

	} // End if - then

	if(m_depth_op_params.m_abs_mode == CDepthOpParams::eAbsolute){
		python << _T("#absolute() mode\n");
	}
	else
	{
		python << _T("rapid(z=clearance)\n");
		python << _T("incremental()\n");
	}// End if else - then

	return(python);
}


std::list<wxString> CDepthOp::DesignRulesAdjustment(const bool apply_changes)
{

	std::list<wxString> changes;

	CTool *pTool = CTool::Find( m_tool_number );
	if (pTool == NULL)
	{
#ifdef UNICODE
		std::wostringstream l_ossChange;
#else
		std::ostringstream l_ossChange;
#endif

		l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << GetID() << ") " << _("does not have a tool assigned") << ". " << _("It can not produce GCode without a tool assignment") << ".\n";
		changes.push_back(l_ossChange.str().c_str());
	} // End if - then
	else
	{
		double cutting_depth = m_depth_op_params.m_start_depth - m_depth_op_params.m_final_depth;
		if (cutting_depth > pTool->m_params.m_cutting_edge_height)
		{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << GetID() << ") " << _("is set to cut deeper than the assigned tool will allow") << ".\n";
			changes.push_back(l_ossChange.str().c_str());
		} // End if - then
	} // End if - else

	if (m_depth_op_params.m_start_depth <= m_depth_op_params.m_final_depth)
	{
#ifdef UNICODE
		std::wostringstream l_ossChange;
#else
		std::ostringstream l_ossChange;
#endif
		l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << GetID() << ") " << _("has poor start and final depths") << ". " << _("Can't change this setting automatically") << ".\n";
		changes.push_back(l_ossChange.str().c_str());
	} // End if - then

	if (m_depth_op_params.m_start_depth > m_depth_op_params.ClearanceHeight())
	{
#ifdef UNICODE
		std::wostringstream l_ossChange;
#else
		std::ostringstream l_ossChange;
#endif

		l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << GetID() << ") " << _("Clearance height is below start depth") << ".\n";
		changes.push_back(l_ossChange.str().c_str());

		if (apply_changes)
		{
			l_ossChange << _("Depth Operation") << " (id=" << GetID() << ").  " << _("Raising clearance height up to start depth (+5 mm)") << "\n";
			m_depth_op_params.ClearanceHeight( m_depth_op_params.m_start_depth + 5 );
		} // End if - then
	} // End if - then

    if (m_depth_op_params.m_step_down < 0)
    {
        wxString change;

        change << _("The step-down value for pocket (id=") << GetID() << _(") must be positive");
        changes.push_back(change);

        if (apply_changes)
        {
            m_depth_op_params.m_step_down = m_depth_op_params.m_step_down * -1.0;
        }
    }

	return(changes);

} // End DesignRulesAdjustment() method

void CDepthOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CSpeedOp::GetTools( t_list, p );
}

std::list<double> CDepthOp::GetDepths() const
{
    std::list<double> depths;

    if (m_depth_op_params.m_start_depth <= m_depth_op_params.m_final_depth)
    {
        // Invalid depth values defined.
        return(depths); // Empty list.
    }

    for (double depth=m_depth_op_params.m_start_depth - m_depth_op_params.m_step_down;
                depth > m_depth_op_params.m_final_depth;
                depth -= m_depth_op_params.m_step_down)
    {
        depths.push_back(depth);
    }

    if ((depths.size() == 0) || (*(depths.rbegin()) > m_depth_op_params.m_final_depth))
    {
        depths.push_back( m_depth_op_params.m_final_depth );
    }

    return(depths);
}



bool CDepthOpParams::operator== ( const CDepthOpParams & rhs ) const
{
	if (m_clearance_height != rhs.m_clearance_height) return(false);
	if (m_start_depth != rhs.m_start_depth) return(false);
	if (m_step_down != rhs.m_step_down) return(false);
	if (m_final_depth != rhs.m_final_depth) return(false);
	if (m_rapid_safety_space != rhs.m_rapid_safety_space) return(false);

	return(true);
}

bool CDepthOp::operator== ( const CDepthOp & rhs ) const
{
	if (m_depth_op_params != rhs.m_depth_op_params) return(false);
	return(CSpeedOp::operator==(rhs));
}

double CDepthOpParams::ClearanceHeight() const
{
	switch (theApp.m_program->m_clearance_source)
	{
	case CProgram::eClearanceDefinedByMachine:
		return(theApp.m_program->m_machine.m_clearance_height);

#ifndef STABLE_OPS_ONLY
	case CProgram::eClearanceDefinedByFixture:
		// We need to figure out which is the 'active' fixture and return
		// the clearance height from that fixture.

		if (theApp.m_program->m_active_machine_state != NULL)
		{
			return(theApp.m_program->m_active_machine_state->Fixture().m_params.m_clearance_height);
		}
		else
		{
			// This should not occur.  In any case, use the clearance value from the individual operation.
			return(m_clearance_height);
		}
#endif // STABLE_OPS_ONLY

	case CProgram::eClearanceDefinedByOperation:
	default:
		return(m_clearance_height);
	} // End switch
}
