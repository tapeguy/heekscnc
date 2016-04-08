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


CDepthOpParams::CDepthOpParams(CDepthOp * parent)
{
	this->parent = parent;
	InitializeProperties();
	m_clearance_height = 0.0;
    m_start_depth = 0.0;
    m_step_down = 0.0;
    m_z_finish_depth = 0.0;
    m_z_thru_depth = 0.0;
    m_final_depth = 0.0;
    m_rapid_safety_space = 0.0;
}

void CDepthOpParams::InitializeProperties()
{
    m_clearance_height.Initialize(_("clearance_height"), parent);
    m_rapid_safety_space.Initialize(_("rapid_safety_space"), parent);

    //My initial thought was that extrusion operatons would always start at z=0 and end at z=top of object.  I'm now thinking it might be desireable to preserve this as an option.
    //It might be good to run an operation that prints the bottom half of the object, pauses to allow insertion of something.  Then another operation could print the top half.

    m_start_depth.Initialize(_("start_depth"), parent);
    m_final_depth.Initialize(_("final_depth"), parent);

    //Step down doesn't make much sense for extrusion.  The amount the z axis steps up or down is equal to the layer thickness of the slice which
    //is determined by the thickness of an extruded filament.  Step up is very important since it is directly related to the resolution of the final
    //produce.
    m_step_down.Initialize(_("max_step_down"), parent);

    m_z_finish_depth.Initialize(_("z_finish_depth"), parent);
    m_z_thru_depth.Initialize(_("z_thru_depth"), parent);
    m_user_depths.Initialize(_("user_depths"), parent);
}


CDepthOp::CDepthOp(const int tool_number, const int operation_type)
 : CSpeedOp(tool_number, operation_type), m_depth_op_params(this)
{
	ReadDefaultValues();
}

CDepthOp::CDepthOp( const CDepthOp & rhs ) : CSpeedOp(rhs), m_depth_op_params(this)
{
	m_depth_op_params = rhs.m_depth_op_params;
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

void CDepthOp::ReloadPointers()
{
        CSpeedOp::ReloadPointers();
}

static double degrees_to_radians( const double degrees )
{
	return( (degrees / 360.0) * (2 * PI) );
} // End degrees_to_radians() routine

void CDepthOp::OnPropertySet(Property& prop)
{
	this->WriteDefaultValues();
	CSpeedOp::OnPropertySet(prop);
}

void CDepthOp::glCommands(bool select, bool marked, bool no_color)
{
	CSpeedOp::glCommands(select, marked, no_color);
}

void CDepthOp::WriteDefaultValues()
{
	CSpeedOp::WriteDefaultValues();

	CNCConfig config(GetTypeString());
	config.Write(_T("ClearanceHeight"), m_depth_op_params.m_clearance_height);
    config.Write(_T("StartDepth"), m_depth_op_params.m_start_depth);
    config.Write(_T("StepDown"), m_depth_op_params.m_step_down);
    config.Write(_T("ZFinish"), m_depth_op_params.m_z_finish_depth);
    config.Write(_T("ZThru"), m_depth_op_params.m_z_thru_depth);
    config.Write(_T("UserDepths"), (const wxString &)m_depth_op_params.m_user_depths);
    config.Write(_T("FinalDepth"), m_depth_op_params.m_final_depth);
    config.Write(_T("RapidDown"), m_depth_op_params.m_rapid_safety_space);
}

void CDepthOp::ReadDefaultValues()
{
	CSpeedOp::ReadDefaultValues();

	CNCConfig config(GetTypeString());

	config.Read(_T("ClearanceHeight"), m_depth_op_params.m_clearance_height, 5.0);
    config.Read(_T("StartDepth"), m_depth_op_params.m_start_depth, 0.0);
    config.Read(_T("StepDown"), m_depth_op_params.m_step_down, 1.0);
    config.Read(_T("ZFinish"), m_depth_op_params.m_z_finish_depth, 0.0);
    config.Read(_T("ZThru"), m_depth_op_params.m_z_thru_depth, 0.0);
    config.Read(_T("UserDepths"), m_depth_op_params.m_user_depths, _T(""));
    config.Read(_T("FinalDepth"), m_depth_op_params.m_final_depth, -1.0);
    config.Read(_T("RapidDown"), m_depth_op_params.m_rapid_safety_space, 2.0);
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

Python CDepthOp::AppendTextToProgram()
{
	Python python;

    python << CSpeedOp::AppendTextToProgram();

    double scale = Length::Conversion(theApp.m_program->m_units, UnitTypeMillimeter);
    python << _T("depthparams = depth_params(");
    python << _T("float(") << m_depth_op_params.m_clearance_height / scale << _T(")");
    python << _T(", float(") << m_depth_op_params.m_rapid_safety_space / scale << _T(")");
    python << _T(", float(") << m_depth_op_params.m_start_depth / scale << _T(")");
    python << _T(", float(") << m_depth_op_params.m_step_down / scale << _T(")");
    python << _T(", float(") << m_depth_op_params.m_z_finish_depth / scale << _T(")");
    python << _T(", float(") << m_depth_op_params.m_z_thru_depth / scale << _T(")");
    python << _T(", float(") << m_depth_op_params.m_final_depth / scale << _T(")");
    if(((const wxString&)(m_depth_op_params.m_user_depths)).Len() == 0)
        python << _T(", None");
    else
        python << _T(", [") << m_depth_op_params.m_user_depths << _T("]");

    python << _T(")\n");

    CTool *pTool = CTool::Find( m_tool_number );
    if (pTool != NULL)
    {
            python << _T("tool_diameter = float(") << (pTool->CuttingRadius(true) * 2.0) << _T(")\n");
            python << _T("cutting_edge_angle = float(") << pTool->m_params.m_cutting_edge_angle<< _T(")\n");

    } // End if - then

    return(python);
}

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
    if (m_z_finish_depth != rhs.m_z_finish_depth) return(false);
    if (m_z_thru_depth != rhs.m_z_thru_depth) return(false);
    if (m_final_depth != rhs.m_final_depth) return(false);
    if (m_rapid_safety_space != rhs.m_rapid_safety_space) return(false);

	return(true);
}

bool CDepthOp::operator== ( const CDepthOp & rhs ) const
{
	if (m_depth_op_params != rhs.m_depth_op_params) return(false);
	return(CSpeedOp::operator==(rhs));
}
