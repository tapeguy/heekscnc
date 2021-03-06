// CTool.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include <stdafx.h>
#include <math.h>
#include "CTool.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/HeeksColor.h"
#include "tinyxml/tinyxml.h"
#include "CNCPoint.h"
#include "PythonStuff.h"
#include "Program.h"
#include "Surface.h"

#include <sstream>
#include <string>
#include <algorithm>

#include <gp_Pnt.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Compound.hxx>

#include <TopExp_Explorer.hxx>

#include <BRep_Tool.hxx>
#include <BRepLib.hxx>

#include <GCE2d_MakeSegment.hxx>

#include <Handle_Geom_TrimmedCurve.hxx>
#include <Handle_Geom_CylindricalSurface.hxx>
#include <Handle_Geom2d_TrimmedCurve.hxx>
#include <Handle_Geom2d_Ellipse.hxx>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeRevolution.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>

#include <BRepPrim_Cylinder.hxx>
#include <BRepPrim_Cone.hxx>

#include <BRepFilletAPI_MakeFillet.hxx>

#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>

#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeSegment.hxx>

#include <BRepAlgoAPI_Fuse.hxx>

#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>

#include "Tools.h"
#include "CToolDlg.h"

extern CHeeksCADInterface* heeksCAD;

#ifdef HEEKSCNC
#define PROGRAM theApp.m_program
#define TOOLS (theApp.m_program ? theApp.m_program->Tools() : NULL)
#else
#define PROGRAM heeksCNC->GetProgram()
#define TOOLS heeksCNC->GetTools()
#endif

CToolParams::CToolParams(CTool * parent)
{
    this->parent = parent;
    InitializeProperties();
}

void CToolParams::InitializeProperties()
{
	m_automatically_generate_title.Initialize(_("Automatically generate title"), parent);

	m_material.Initialize(_("Material"), parent);
	CToolParams::MaterialsList_t materials_list = CToolParams::GetMaterialsList();
	for (CToolParams::MaterialsList_t::size_type i = 0; i < materials_list.size(); i++)
	{
		m_material.m_choices.push_back(materials_list[i].second);
	}

	m_type_choice.Initialize(_("Type"), parent);
	ToolTypesList_t tool_types_list = CToolParams::GetToolTypesList();
	for (CToolParams::ToolTypesList_t::size_type i = 0; i < tool_types_list.size(); i++)
	{
	    m_type_choice.m_choices.push_back(tool_types_list[i].second);
	}
	m_type_choice.SetTransient(true);

	m_type.Initialize(_("Type"), parent);
	m_type.SetVisible(false);

	m_max_advance_per_revolution.Initialize(_("Max advance per revolution"), parent);
	m_x_offset.Initialize(_("X offset"), parent);
	m_front_angle.Initialize(_("Front Angle"), parent);
	m_tool_angle.Initialize(_("Tool Angle"), parent);
	m_back_angle.Initialize(_("Back Angle"), parent);

	m_orientation.Initialize(_("Orientation"), parent);
	m_orientation.m_choices.push_back(_("(0) Unused"));
	m_orientation.m_choices.push_back(_("(1) Turning/Back Facing"));
	m_orientation.m_choices.push_back(_("(2) Turning/Facing"));
	m_orientation.m_choices.push_back(_("(3) Boring/Facing"));
	m_orientation.m_choices.push_back(_("(4) Boring/Back Facing"));
	m_orientation.m_choices.push_back(_("(5) Back Facing"));
	m_orientation.m_choices.push_back(_("(6) Turning"));
	m_orientation.m_choices.push_back(_("(7) Facing"));
	m_orientation.m_choices.push_back(_("(8) Boring"));
	m_orientation.m_choices.push_back(_("(9) Centre"));

	m_diameter.Initialize(_("Diameter"), parent);
	m_tool_length_offset.Initialize(_("Tool length offset"), parent);
	m_flat_radius.Initialize(_("Flat radius"), parent);
	m_corner_radius.Initialize(_("Corner radius"), parent);
	m_cutting_edge_angle.Initialize(_("Cutting edge angle"), parent);
	m_cutting_edge_height.Initialize(_("Cutting edge height"), parent);
	m_gradient.Initialize(_("Plunge Gradient"), parent);

	m_probe_offset_x.Initialize(_("Probe offset X"), parent);
	m_probe_offset_y.Initialize(_("Probe offset Y"), parent);

	m_extrusion_material.Initialize(_("Extrusion material"), parent);
	CToolParams::ExtrusionMaterialsList_t extrusion_materials_list = CToolParams::GetExtrusionMaterialsList();
	for (CToolParams::ExtrusionMaterialsList_t::size_type i = 0; i < extrusion_materials_list.size(); i++)
	{
		m_extrusion_material.m_choices.push_back(extrusion_materials_list[i].second);
	}

	m_width_over_thickness.Initialize(_("Width / Thickness"), parent);
	m_feedrate.Initialize(_("Feedrate"), parent);
	m_layer_height.Initialize(_("Layer height"), parent);
	m_temperature.Initialize(_("Temperature (C)"), parent);
	m_flowrate.Initialize(_("Flowrate"), parent);
	m_filament_diameter.Initialize(_("Filament diameter"), parent);
	m_direction.Initialize(_("Tap direction"), parent);
	m_direction.m_choices.push_back(_("right hand"));
	m_direction.m_choices.push_back(_("left hand"));
	m_pitch.Initialize(_("Pitch"), parent);

	m_thread_standard.Initialize(_("Select TAP from standard sizes"), parent);
	m_thread_standard.m_choices.push_back(_("Select size"));
	m_thread_standard.m_choices.push_back(_("Metric"));
	m_thread_standard.m_choices.push_back(_("Unified Thread Standard (UNC/UNF/UNEF)"));
	m_thread_standard.m_choices.push_back(_("British Standard Whitworth"));
}

void CToolParams::set_initial_values()
{
	CNCConfig config(ConfigScope());
	config.Read(_T("m_material"), m_material, int(eCarbide));
	config.Read(_T("m_diameter"), m_diameter, 12.7);
	config.Read(_T("m_tool_length_offset"), m_tool_length_offset, (10 * m_diameter));
	config.Read(_T("m_max_advance_per_revolution"), m_max_advance_per_revolution, 0.12 );	// mm
	config.Read(_T("m_automatically_generate_title"), m_automatically_generate_title, 1 );

	config.Read(_T("m_type"), m_type, eDrill);
	config.Read(_T("m_flat_radius"), m_flat_radius, 0);
	config.Read(_T("m_corner_radius"), m_corner_radius, 0);
	config.Read(_T("m_cutting_edge_angle"), m_cutting_edge_angle, 59);
	config.Read(_T("m_cutting_edge_height"), m_cutting_edge_height, 4 * m_diameter);

	// The following are all turning tool parameters
	config.Read(_T("m_orientation"), m_orientation, 6);
	config.Read(_T("m_x_offset"), m_x_offset, 0);
	config.Read(_T("m_front_angle"), m_front_angle, 95);
	config.Read(_T("m_tool_angle"), m_tool_angle, 60);
	config.Read(_T("m_back_angle"), m_back_angle, 25);

	// The following are ONLY for touch probe tools
	config.Read(_T("probe_offset_x"), m_probe_offset_x, 0.0);
	config.Read(_T("probe_offset_y"), m_probe_offset_y, 0.0);

	// The following are ONLY for extrusions
	config.Read(_T("m_extrusion_material"), m_extrusion_material, int(eABS));  //type of plastic or other material to extrude
	config.Read(_T("m_feedrate"), m_feedrate, 50);  //the base feed rate.
	config.Read(_T("m_layer_height"), m_layer_height, .35);  //Distance the extruder moves in Z axis for each layer.
	config.Read(_T("m_width_over_thickness"), m_width_over_thickness, 1.8);  //Ratio expressing the height over width of the extruded filament 1.0 indicates a circular cross section.  Higher numbers indicate an elliptical extrusion.
	config.Read(_T("m_temperature"), m_temperature, 220); //temp in celsius
	config.Read(_T("m_flowrate"), m_flowrate, 255); //speed of the extruder motor.
	config.Read(_T("m_filament_diameter"), m_filament_diameter, 3); //The diameter of the raw filament.  Typically ~3mm

	config.Read(_T("gradient"), m_gradient, 0.0);  // Straight plunge by default.

	// The following are ONLY for tapping tools
	config.Read(_T("m_direction"), m_direction, 0);  // default to right-hand tap
	config.Read(_T("m_pitch"), m_pitch, 1.0);        // mm/rev, this would be for an M6 tap
	}

void CToolParams::write_values_to_config()
{
	CNCConfig config(ConfigScope());

	// We ALWAYS write the parameters into the configuration file in mm (for consistency).
	// If we're now in inches then convert the values.
	// We're in mm already.
	config.Write(_T("m_material"), m_material);
	config.Write(_T("m_diameter"), m_diameter);
	config.Write(_T("m_x_offset"), m_x_offset);
	config.Write(_T("m_tool_length_offset"), m_tool_length_offset);
	config.Write(_T("m_orientation"), m_orientation);
	config.Write(_T("m_max_advance_per_revolution"), m_max_advance_per_revolution );
	config.Write(_T("m_automatically_generate_title"), m_automatically_generate_title );

	config.Write(_T("m_type"), m_type);
	config.Write(_T("m_flat_radius"), m_flat_radius);
	config.Write(_T("m_corner_radius"), m_corner_radius);
	config.Write(_T("m_cutting_edge_angle"), m_cutting_edge_angle);
	config.Write(_T("m_cutting_edge_height"), m_cutting_edge_height);

	config.Write(_T("m_front_angle"), m_front_angle);
	config.Write(_T("m_tool_angle"), m_tool_angle);
	config.Write(_T("m_back_angle"), m_back_angle);

	// The following are ONLY for touch probe tools
	config.Write(_T("probe_offset_x"), m_probe_offset_x);
	config.Write(_T("probe_offset_y"), m_probe_offset_y);

	// The following are ONLY for extrusions
	config.Write(_T("m_extrusion_material"), m_extrusion_material);
	config.Write(_T("m_feedrate"), m_feedrate);
	config.Write(_T("m_layer_height"), m_layer_height);
	config.Write(_T("m_width_over_thickness"), m_width_over_thickness);
	config.Write(_T("m_temperature"), m_temperature);
	config.Write(_T("m_flowrate"), m_flowrate);
	config.Write(_T("m_filament_diameter"), m_filament_diameter);


	config.Write(_T("gradient"), m_gradient);

	// The following are ONLY for tapping tools
	config.Write(_T("m_direction"), m_direction);
	config.Write(_T("m_pitch"), m_pitch);
}

static double degrees_to_radians( const double degrees )
{
	return( (degrees / 360) * 2 * PI );
} // End degrees_to_radians() routine


double CToolParams::ReasonableGradient( const eToolType type ) const
{
    switch(type)
	{
	    case CToolParams::eCentreDrill:
		case CToolParams::eDrill:
		case CToolParams::eTapTool:
				return(0.0);

        case CToolParams::eSlotCutter:
		case CToolParams::eEndmill:
		case CToolParams::eBallEndMill:
				return(-1.0 / 10.0);  // 1 down for 10 across


		case CToolParams::eTouchProbe:
		case CToolParams::eToolLengthSwitch:
				return(0.0);

		case CToolParams::eExtrusion:
				return(0.0);

		case CToolParams::eChamfer:
				return(0.0);

		case CToolParams::eTurningTool:
				return(0.0);

		case CToolParams::eEngravingTool:
				return(0.0);

        default:
            return(0.0);
	} // End switch
}


CTool::CTool ( const CTool & rhs )
 : HeeksObj(rhs), m_params(this)
{
    m_pToolSolid = NULL;
    *this = rhs;    // Call the assignment operator.
}


CTool::~CTool()
{
    DeleteSolid();
}


void CTool::InitializeProperties()
{
    m_tool_number.Initialize(_("Tool number"), this);
}


void CTool::SetAngleAndRadius()
{
    switch (m_params.m_type)
    {
        case CToolParams::eChamfer:
        case CToolParams::eEngravingTool:
        {
            // Recalculate the cutting edge length based on this new diameter
            // and the cutting angle.

            double opposite = m_params.m_diameter - m_params.m_flat_radius;
            double angle = m_params.m_cutting_edge_angle / 360.0 * 2 * PI;

            m_params.m_cutting_edge_height = opposite / tan(angle);
        }
        break;

        case CToolParams::eEndmill:
        case CToolParams::eSlotCutter:
            m_params.m_flat_radius = m_params.m_diameter / 2.0;
            break;

        default:
        break;
    }

    ResetTitle();
    KillGLLists();
    heeksCAD->Repaint();
}

void CTool::OnPropertySet(Property& prop)
{
    if (prop == m_params.m_type_choice) {
        CToolParams::ToolTypesList_t tool_types_list = CToolParams::GetToolTypesList();
        CToolParams::ToolTypeDescription_t description = tool_types_list[m_params.m_type_choice];
        m_params.m_type = description.first;
    }
    else if (prop == m_params.m_diameter) {
		SetAngleAndRadius();
	}
	else if (prop == m_params.m_flat_radius) {
		double value = m_params.m_flat_radius;
		if (value > m_params.m_diameter / 2.0)
		{
			wxMessageBox(_T("Flat radius cannot be larger than the tool's diameter"));
			return;
		}
		SetAngleAndRadius();
	}
	else if (prop == m_params.m_cutting_edge_angle) {
		double value = m_params.m_cutting_edge_angle;
		if (value < 0)
		{
			wxMessageBox(_T("Cutting edge angle must be zero or positive."));
			return;
		}
		SetAngleAndRadius();
	}
	else if (prop == m_params.m_cutting_edge_height)
	{
		double value = m_params.m_cutting_edge_height;
		if (value <= 0)
		{
			wxMessageBox(_T("Cutting edge height must be positive."));
			return;
		}

		if (m_params.m_type == CToolParams::eChamfer || m_params.m_type == CToolParams::eEngravingTool)
		{
			wxMessageBox(_T("Cutting edge height is generated from diameter, flat radius and cutting edge angle for chamfering bits."));
			return;
		}
	}
	else
	{
	    HeeksObj::OnPropertySet(prop);
	}

	ResetTitle();
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
Python CTool::AppendTextToProgram()
{
    Python python;
    wxString title = GetTitle();

    // The G10 command can be used (within EMC2) to add a tool to the tool
        // table from within a program.
        // G10 L1 P[tool number] R[radius] X[offset] Z[offset] Q[orientation]
    //
    // The radius value must be expressed in MACHINE CONFIGURATION UNITS.  This may be different
    // to this model's drawing units.  The value is interpreted, at lease for EMC2, in terms
    // of the units setup for the machine's configuration (something.ini in EMC2 parlence).  At
    // the moment we don't have a MACHINE CONFIGURATION UNITS parameter so we've got a 50%
    // chance of getting it right.

    if (title.size() > 0)
    {
        python << _T("#(") << title.c_str() << _T(")\n");
    } // End if - then

    python << _T("tool_defn( ") << m_tool_number << _T(", ");

    if (title.size() > 0)
    {
        python << PythonString(title).c_str() << _T(", ");
    } // End if - then
    else
    {
        python << _T("None, ");
    } // End if - else

    // write all the other parameters as a dictionary
    python << _T("{");
    python << _T("'corner radius':") << this->m_params.m_corner_radius;
    python << _T(", ");
    python << _T("'cutting edge angle':") << this->m_params.m_cutting_edge_angle;
    python << _T(", ");
    python << _T("'cutting edge height':") << this->m_params.m_cutting_edge_height;
    python << _T(", ");
    python << _T("'diameter':") << this->m_params.m_diameter;
    python << _T(", ");
    python << _T("'flat radius':") << this->m_params.m_flat_radius;
    python << _T(", ");
    python << _T("'material':") << this->m_params.m_material;
    python << _T(", ");
    python << _T("'tool length offset':") << this->m_params.m_tool_length_offset;
    python << _T(", ");
    python << _T("'type':") << this->m_params.m_type;
    python << _T(", ");
    python << _T("'name':'") << this->GetMeaningfulName(theApp.m_program->m_units) << _T("'");
    python << _T("})\n");

    return(python);
}


void CTool::GetProperties(std::list<Property *> *list)
{
    CToolParams::ToolTypesList_t tool_types_list = CToolParams::GetToolTypesList();
    CToolParams::ToolTypeDescription_t description = tool_types_list[m_params.m_type_choice];
    m_params.m_type = description.first;

    for (CToolParams::ToolTypesList_t::size_type i = 0; i < tool_types_list.size(); i++)
    {
        if (m_params.m_type == tool_types_list[i].first)
            m_params.m_type_choice = i;
    }

	HeeksObj::GetProperties(list);
}


HeeksObj *CTool::MakeACopy(void)const
{
	HeeksObj *duplicate = new CTool(*this);
	((CTool *) duplicate)->m_pToolSolid = NULL;	// We didn't duplicate this so reset the pointer.
	return(duplicate);
}


CTool & CTool::operator= ( const CTool & rhs )
{
    if (this != &rhs)
    {
        m_params = rhs.m_params;
        SetTitle ( rhs.GetTitle() );
        m_tool_number = rhs.m_tool_number;

        if (m_pToolSolid)
        {
            delete m_pToolSolid;
            m_pToolSolid = NULL;
        }

        HeeksObj::operator=( rhs );
    }

    return(*this);
}


void CTool::CopyFrom(const HeeksObj* object)
{
	operator=(*((CTool*)object));
	m_pToolSolid = NULL;	// We didn't duplicate this so reset the pointer.
}

bool CTool::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == ToolsType));
}

const wxBitmap &CTool::GetIcon()
{
	switch(m_params.m_type){
		case CToolParams::eDrill:
			{
				static wxBitmap* drillIcon = NULL;
				if(drillIcon == NULL)drillIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/drill.png")));
				return *drillIcon;
			}
		case CToolParams::eCentreDrill:
			{
				static wxBitmap* centreDrillIcon = NULL;
				if(centreDrillIcon == NULL)centreDrillIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/centredrill.png")));
				return *centreDrillIcon;
			}
		case CToolParams::eEndmill:
			{
				static wxBitmap* endmillIcon = NULL;
				if(endmillIcon == NULL)endmillIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/endmill.png")));
				return *endmillIcon;
			}
		case CToolParams::eSlotCutter:
			{
				static wxBitmap* slotCutterIcon = NULL;
				if(slotCutterIcon == NULL)slotCutterIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/slotdrill.png")));
				return *slotCutterIcon;
			}
		case CToolParams::eBallEndMill:
			{
				static wxBitmap* ballEndMillIcon = NULL;
				if(ballEndMillIcon == NULL)ballEndMillIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons//ballmill.png")));
				return *ballEndMillIcon;
			}
		case CToolParams::eChamfer:
			{
				static wxBitmap* chamferIcon = NULL;
				if(chamferIcon == NULL)chamferIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/chamfmill.png")));
				return *chamferIcon;
			}
		case CToolParams::eTurningTool:
			{
				static wxBitmap* turningToolIcon = NULL;
				if(turningToolIcon == NULL)turningToolIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/turntool.png")));
				return *turningToolIcon;
			}
		case CToolParams::eTouchProbe:
			{
				static wxBitmap* touchProbeIcon = NULL;
				if(touchProbeIcon == NULL)touchProbeIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/probe.png")));
				return *touchProbeIcon;
			}
		case CToolParams::eToolLengthSwitch:
			{
				static wxBitmap* toolLengthSwitchIcon = NULL;
				if(toolLengthSwitchIcon == NULL)toolLengthSwitchIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/probe.png")));
				return *toolLengthSwitchIcon;
			}

		case CToolParams::eExtrusion:
			{
				static wxBitmap* extrusionIcon = NULL;
				if(extrusionIcon == NULL)extrusionIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/extrusion.png")));
				return *extrusionIcon;
			}
		case CToolParams::eTapTool:
			{
				static wxBitmap* tapToolIcon = NULL;
				if(tapToolIcon == NULL)tapToolIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/tap.png")));
				return *tapToolIcon;
			}
		case CToolParams::eEngravingTool:
			{
				static wxBitmap* engraverIcon = NULL;
				if(engraverIcon == NULL)engraverIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/engraver.png")));
				return *engraverIcon;
			}
		default:
			{
				static wxBitmap* toolIcon = NULL;
				if(toolIcon == NULL)toolIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/tool.png")));
				return *toolIcon;
			}
	}
}

void CTool::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Tool" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetAttribute( "title", GetTitle().utf8_str());

	element->SetAttribute( "tool_number", m_tool_number );

	WriteBaseXML(element);
}

// static member function
HeeksObj* CTool::ReadFromXMLElement(TiXmlElement* element)
{

	int tool_number = 0;
	if (element->Attribute("tool_number"))
	    element->Attribute("tool_number", &tool_number);

	wxString title(Ctt(element->Attribute("title")));
	CTool* new_object = new CTool( title.c_str(), CToolParams::eDrill, tool_number);

	new_object->ReadBaseXML(element);
	new_object->ResetTitle();	// reset title to match design units.
	return new_object;
}


void CTool::OnEditString(const wxChar* str)
{
	m_params.m_automatically_generate_title.SetValue ( false );	// It's been manually edited.  Leave it alone now.
	// to do, make undoable properties
}

CTool *CTool::Find( const int tool_number )
{
	int id = FindTool( tool_number );
	if (id <= 0) return(NULL);
	return((CTool *) heeksCAD->GetIDObject( ToolType, id ));
} // End Find() method

CTool::ToolNumber_t CTool::FindFirstByType( const CToolParams::eToolType type )
{

	if (TOOLS)
	{
		HeeksObj* tool_list = TOOLS;
		for(HeeksObj* ob = tool_list->GetFirstChild(); ob; ob = tool_list->GetNextChild())
		{
			if (ob->GetType() != ToolType) continue;
			if ((ob != NULL) && (((CTool *) ob)->m_params.m_type == type))
			{
				return(((CTool *)ob)->m_tool_number);
			} // End if - then
		} // End for
	} // End if - then

	return(-1);
}


/**
 * Find the Tool object whose tool number matches that passed in.
 */
int CTool::FindTool( const int tool_number )
{

	if (TOOLS)
	{
		HeeksObj* tool_list = TOOLS;

		for(HeeksObj* ob = tool_list->GetFirstChild(); ob; ob = tool_list->GetNextChild())
		{
			if (ob->GetType() != ToolType) continue;
			if ((ob != NULL) && (((CTool *) ob)->m_tool_number == tool_number))
			{
				return(ob->GetID());
			} // End if - then
		} // End for
	} // End if - then

	return(-1);

} // End FindTool() method



std::vector< std::pair< int, wxString > > CTool::FindAllTools()
{
    std::vector< std::pair< int, wxString > > tools;

    // Always add a value of zero to allow for an absense of tool use.
    tools.push_back( std::make_pair(0, _T("No Tool") ) );

    if (TOOLS)
    {
        HeeksObj* tool_list = TOOLS;

        for(HeeksObj* ob = tool_list->GetFirstChild(); ob; ob = tool_list->GetNextChild())
        {
            if (ob->GetType() != ToolType)
                continue;

            CTool *pTool = (CTool *)ob;
            if (ob != NULL)
            {
                tools.push_back( std::make_pair( pTool->m_tool_number, pTool->GetTitle() ) );
            } // End if - then
        } // End for
    } // End if - then

    return(tools);

} // End FindAllTools() method

//static
CToolParams::eToolType CTool::FindToolType( const int tool_number )
{
        CTool* pTool = Find(tool_number);
        if(pTool)
        {
            int tool_type = pTool->m_params.m_type;
            return (CToolParams::eToolType)tool_type;
        }
        return CToolParams::eUndefinedToolType;
}

// static
bool CTool::IsMillingToolType( CToolParams::eToolType type )
{
        switch(type)
        {
        case CToolParams::eEndmill:
        case CToolParams::eSlotCutter:
        case CToolParams::eBallEndMill:
        case CToolParams::eDrill:
        case CToolParams::eCentreDrill:
        case CToolParams::eChamfer:
                return true;
        default:
                return false;
        }
}

/**
	Find a fraction that represents this floating point number.  We use this
	purely for readability purposes.  It only looks accurate to the nearest 1/64th

	eg: 0.125 -> "1/8"
	    1.125 -> "1 1/8"
	    0.109375 -> "7/64"
 */
/* static */ wxString CTool::FractionalRepresentation( const double original_value, const int max_denominator /* = 64 */ )
{

    wxString result;
	double _value(original_value);
	double near_enough = 0.00001;

	if (floor(_value) > 0)
	{
		result << floor(_value) << _T(" ");
		_value -= floor(_value);
	} // End if - then

	// We only want even numbers between 2 and 64 for the denominator.  The others just look 'wierd'.
	for (int denominator = 2; denominator <= max_denominator; denominator *= 2)
	{
		for (int numerator = 1; numerator < denominator; numerator++)
		{
			double fraction = double( double(numerator) / double(denominator) );
			if ( ((_value > fraction) && ((_value - fraction) < near_enough)) ||
			     ((_value < fraction) && ((fraction - _value) < near_enough)) ||
			     (_value == fraction) )
			{
				result << numerator << _T("/") << denominator;
				return(result);
			} // End if - then
		} // End for
	} // End for


	result = _T("");	// It's not a recognisable fraction.  Return nothing to indicate such.
	return(result);
} // End FractionalRepresentation() method

/**
 * This method uses the various attributes of the tool to produce a meaningful name.
 * eg: with diameter = 6, units = 1 (mm) and type = 'drill' the name would be '6mm Drill Bit".  The
 * idea is to produce a m_title value that is representative of the tool.  This will
 * make selection in the program list easier.
 */
wxString CTool::GetMeaningfulName(EnumUnitType units) const
{
#ifdef UNICODE
    std::wostringstream l_ossName;
#else
    std::ostringstream l_ossName;
#endif

    {
        double scale = Length::Conversion(theApp.m_program->m_units, UnitTypeMillimeter);
        if (units == UnitTypeMillimeter)
        {
            // We're using metric.  Leave the diameter as a floating point number.  It just looks more natural.
            l_ossName << m_params.m_diameter / scale << " mm ";
        } // End if - then
        else
        {
            // We're using inches.  Find a fractional representation if one matches.
            wxString fraction = FractionalRepresentation(m_params.m_diameter / units);

            if (fraction.Len() > 0)
            {
                l_ossName << fraction.c_str() << " inch ";
            }
            else
            {
                l_ossName << m_params.m_diameter / scale << " inch ";
            }
        } // End if - else
    } // End if - then

    switch (m_params.m_type)
    {
        case CToolParams::eDrill:   l_ossName << (_("Drill Bit"));
            break;

        case CToolParams::eCentreDrill: l_ossName << (_("Centre Drill Bit"));
            break;

        case CToolParams::eEndmill: l_ossName << (_("End Mill"));
            break;

        case CToolParams::eSlotCutter:  l_ossName << (_("Slot Cutter"));
            break;

        case CToolParams::eBallEndMill: l_ossName << (_("Ball End Mill"));
            break;

        case CToolParams::eChamfer: l_ossName.str(_T(""));  // Remove all that we've already prepared.
            l_ossName << m_params.m_cutting_edge_angle << (_T(" degree "));
                    l_ossName << (_("Chamfering Bit"));
            break;
        default:
            break;
    } // End switch

    return( l_ossName.str().c_str() );
} // End GetMeaningfulName() method


/**
	Reset the m_title value with a meaningful name ONLY if it does not look like it was
	automatically generated in the first place.  If someone has reset it manually then leave it alone.

	Return a verbose description of what we've done (if anything) so that we can pop up a
	warning message to the operator letting them know.
 */
wxString CTool::ResetTitle()
{
	if (m_params.m_automatically_generate_title)
	{
	    // It has the default title.  Give it a name that makes sense.
        SetTitle ( GetMeaningfulName(heeksCAD->GetViewUnits()) );
        // to do, make undoable properties

#ifdef UNICODE
		std::wostringstream l_ossChange;
#else
		std::ostringstream l_ossChange;
#endif
		l_ossChange << "Changing name to " << GetTitle().c_str() << "\n";
		return( l_ossChange.str().c_str() );
	} // End if - then

	// Nothing changed, nothing to report
	return(_T(""));
} // End ResetTitle() method



/**
        This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
        routines to paint the tool in the graphics window.

	We want to draw an outline of the tool in 2 dimensions so that the operator
	gets a feel for what the various tool parameter values mean.
 */
void CTool::glCommands(bool select, bool marked, bool no_color)
{
	// draw this tool only if it is selected
	if(heeksCAD->ObjectMarked(this) && !no_color)
	{
		if(!m_pToolSolid)
		{
			try {
				TopoDS_Shape tool_shape = GetShape();
				m_pToolSolid = heeksCAD->NewSolid( *((TopoDS_Solid *) &tool_shape), NULL, HeeksColor(234, 123, 89) );
			} // End try
			catch(Standard_DomainError) { }
			catch(...)  { }
		}

		if(m_pToolSolid)m_pToolSolid->glCommands( true, false, true );
	} // End if - then

} // End glCommands() method

void CTool::KillGLLists(void)
{
	DeleteSolid();
}

void CTool::DeleteSolid()
{
	if(m_pToolSolid)
	    delete m_pToolSolid;
	m_pToolSolid = NULL;
}

/**
	This method produces a "Topology Data Structure - Shape" based on the parameters
	describing the tool's dimensions.  We will (probably) use this, along with the
	NCCode paths to find out what devastation this tool will make when run through
	the program.  We will then (again probably) intersect the resulting solid with
	things like fixture solids to see if we're going to break anything.  We could
	also intersect it with a solid that represents the raw material (i.e before it
	has been machined).  This would give us an idea of what we'd end up with if
	we ran the GCode program.

	Finally, we might use this to 'apply' a GCode operation to existing geometry.  eg:
	where a drilling cycle is defined, the result of drilling a hole from one
	location for a certain depth will be a hole in that green solid down there.  We
	may want to do this so as to use the edges (or other side-effects) to define
	subsequent NC operations.  (just typing out loud)

	NOTE: The shape is always drawn with the tool's tip at the origin.
	It is always drawn along the Z axis.  The calling routine may move and rotate the drawn
	shape if need be but this method returns a standard straight up and down version.
 */
const TopoDS_Shape& CTool::GetShape() const
{
   try {
	gp_Dir orientation(0,0,1);	// This method always draws it up and down.  Leave it
					// for other methods to rotate the resultant shape if
					// they need to.
	gp_Pnt tool_tip_location(0,0,0);	// Always from the origin in this method.

	double diameter = m_params.m_diameter;
	if (diameter < 0.01) diameter = 2;

	double tool_length_offset = m_params.m_tool_length_offset;
	if (tool_length_offset <  diameter) tool_length_offset = 10 * diameter;

	double cutting_edge_height = m_params.m_cutting_edge_height;
	if (cutting_edge_height < (2 * diameter)) cutting_edge_height = 2 * diameter;

	switch (m_params.m_type)
	{
		case CToolParams::eCentreDrill:
		{
			// First a cylinder to represent the shaft.
			double tool_tip_length = (diameter / 2) * tan( degrees_to_radians(90.0 - m_params.m_cutting_edge_angle));
			double non_cutting_shaft_length = tool_length_offset - tool_tip_length - cutting_edge_height;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length + cutting_edge_height );
			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );
			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, (diameter / 2) * 2.0, non_cutting_shaft_length );

			gp_Pnt cutting_shaft_start_location( tool_tip_location );
			cutting_shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length );
			gp_Ax2 cutting_shaft_position_and_orientation( cutting_shaft_start_location, orientation );
			BRepPrimAPI_MakeCylinder cutting_shaft( cutting_shaft_position_and_orientation, diameter / 2, cutting_edge_height );

			// And a cone for the tip.
			gp_Ax2 tip_position_and_orientation( cutting_shaft_start_location, gp_Dir(0,0,-1) );
			BRepPrimAPI_MakeCone tool_tip( tip_position_and_orientation,
							diameter/2,
							m_params.m_flat_radius,
							tool_tip_length);

			TopoDS_Shape shafts = BRepAlgoAPI_Fuse(shaft.Shape(), cutting_shaft.Shape() );
			m_tool_shape = BRepAlgoAPI_Fuse(shafts, tool_tip.Shape() );
			return m_tool_shape;
		}

		case CToolParams::eDrill:
		{
			// First a cylinder to represent the shaft.
			double tool_tip_length = (diameter / 2) * tan( degrees_to_radians(90 - m_params.m_cutting_edge_angle));
			double shaft_length = tool_length_offset - tool_tip_length;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );

			// And a cone for the tip.
			gp_Ax2 tip_position_and_orientation( shaft_start_location, gp_Dir(0,0,-1) );

			BRepPrimAPI_MakeCone tool_tip( tip_position_and_orientation,
							diameter/2,
							m_params.m_flat_radius,
							tool_tip_length);

			m_tool_shape = BRepAlgoAPI_Fuse(shaft.Shape() , tool_tip.Shape() );
			return m_tool_shape;
		}

		case CToolParams::eChamfer:
		case CToolParams::eEngravingTool:
		{
			// First a cylinder to represent the shaft.
			double tool_tip_length_a = (diameter / 2) / tan( degrees_to_radians(m_params.m_cutting_edge_angle));
			double tool_tip_length_b = (m_params.m_flat_radius)  / tan( degrees_to_radians(m_params.m_cutting_edge_angle));
			double tool_tip_length = tool_tip_length_a - tool_tip_length_b;

			double shaft_length = tool_length_offset - tool_tip_length;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, (diameter / 2) * ((m_params.m_type == CToolParams::eEngravingTool) ? 1.0 : 0.5), shaft_length );

			// And a cone for the tip.
			// double cutting_edge_angle_in_radians = ((m_params.m_cutting_edge_angle / 2) / 360) * (2 * PI);
			gp_Ax2 tip_position_and_orientation( shaft_start_location, gp_Dir(0,0,-1) );

			BRepPrimAPI_MakeCone tool_tip( tip_position_and_orientation,
							diameter/2,
							m_params.m_flat_radius,
							tool_tip_length);

			m_tool_shape = BRepAlgoAPI_Fuse(shaft.Shape() , tool_tip.Shape() );
			return m_tool_shape;
		}

		case CToolParams::eBallEndMill:
		{
			// First a cylinder to represent the shaft.
			double shaft_length = tool_length_offset - m_params.m_corner_radius;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + m_params.m_corner_radius );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );
			BRepPrimAPI_MakeSphere ball( shaft_start_location, diameter / 2 );

			// TopoDS_Compound tool_shape;
			m_tool_shape = BRepAlgoAPI_Fuse(shaft.Shape() , ball.Shape() );
			return m_tool_shape;
		}

		case CToolParams::eTouchProbe:
		case CToolParams::eToolLengthSwitch:	// TODO: Draw a tool length switch
		{
			// First a cylinder to represent the shaft.
			double shaft_length = tool_length_offset - diameter;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() - shaft_length );

			gp_Ax2 tip_position_and_orientation( tool_tip_location, gp_Dir(0,0,+1) );
			BRepPrimAPI_MakeCone shaft( tip_position_and_orientation,
							diameter/16.0,
							diameter/2,
							shaft_length);

			BRepPrimAPI_MakeSphere ball( tool_tip_location, diameter / 2.0 );

			// TopoDS_Compound tool_shape;
			m_tool_shape = BRepAlgoAPI_Fuse(shaft.Shape() , ball.Shape() );
			return m_tool_shape;
		}

		case CToolParams::eTurningTool:
		{
			// First draw the cutting tip.
			double triangle_radius = 8.0;	// mm
			double cutting_tip_thickness = 3.0;	// mm

			gp_Trsf rotation;

			gp_Pnt p1(0.0, triangle_radius, 0.0);
			rotation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(0) );
			p1.Transform(rotation);

			rotation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(+1 * ((360 - m_params.m_tool_angle)/2)) );
			gp_Pnt p2(p1);
			p2.Transform(rotation);

			rotation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(-1 * ((360 - m_params.m_tool_angle)/2)) );
			gp_Pnt p3(p1);
			p3.Transform(rotation);

			Handle(Geom_TrimmedCurve) line1 = GC_MakeSegment(p1,p2);
			Handle(Geom_TrimmedCurve) line2 = GC_MakeSegment(p2,p3);
			Handle(Geom_TrimmedCurve) line3 = GC_MakeSegment(p3,p1);

			TopoDS_Edge edge1 = BRepBuilderAPI_MakeEdge( line1 );
			TopoDS_Edge edge2 = BRepBuilderAPI_MakeEdge( line2 );
			TopoDS_Edge edge3 = BRepBuilderAPI_MakeEdge( line3 );

			TopoDS_Wire wire = BRepBuilderAPI_MakeWire( edge1, edge2, edge3 );
			TopoDS_Face face = BRepBuilderAPI_MakeFace( wire );
			gp_Vec vec( 0,0, cutting_tip_thickness );
			TopoDS_Shape cutting_tip = BRepPrimAPI_MakePrism( face, vec );

			// Now make the supporting shaft
			gp_Pnt p4(p3); p4.SetZ( p3.Z() + cutting_tip_thickness );
			gp_Pnt p5(p2); p5.SetZ( p2.Z() + cutting_tip_thickness );

			Handle(Geom_TrimmedCurve) shaft_line1 = GC_MakeSegment(p2,p3);
			Handle(Geom_TrimmedCurve) shaft_line2 = GC_MakeSegment(p3,p4);
			Handle(Geom_TrimmedCurve) shaft_line3 = GC_MakeSegment(p4,p5);
			Handle(Geom_TrimmedCurve) shaft_line4 = GC_MakeSegment(p5,p2);

			TopoDS_Edge shaft_edge1 = BRepBuilderAPI_MakeEdge( shaft_line1 );
			TopoDS_Edge shaft_edge2 = BRepBuilderAPI_MakeEdge( shaft_line2 );
			TopoDS_Edge shaft_edge3 = BRepBuilderAPI_MakeEdge( shaft_line3 );
			TopoDS_Edge shaft_edge4 = BRepBuilderAPI_MakeEdge( shaft_line4 );

			TopoDS_Wire shaft_wire = BRepBuilderAPI_MakeWire( shaft_edge1, shaft_edge2, shaft_edge3, shaft_edge4 );
			TopoDS_Face shaft_face = BRepBuilderAPI_MakeFace( shaft_wire );
			gp_Vec shaft_vec( 0, (-1 * tool_length_offset), 0 );
			TopoDS_Shape shaft = BRepPrimAPI_MakePrism( shaft_face, shaft_vec );

			// Aggregate the shaft and cutting tip
			m_tool_shape = BRepAlgoAPI_Fuse(shaft , cutting_tip );

			// Now orient the tool as per its settings.
			gp_Trsf tool_holder_orientation;
			gp_Trsf orient_for_lathe_use;

			switch (m_params.m_orientation)
			{
				case 1: // South East
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(45 - 90) );
					break;

				case 2: // South West
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(123 - 90) );
					break;

				case 3: // North West
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(225 - 90) );
					break;

				case 4: // North East
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(-45 - 90) );
					break;

				case 5: // East
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(0 - 90) );
					break;

				case 6: // South
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(+90 - 90) );
					break;

				case 7: // West
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(180 - 90) );
					break;

				case 8: // North
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(-90 - 90) );
					break;

				case 9: // Boring (straight along Y axis)
				default:
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(1,0,0)), degrees_to_radians(90.0) );
					break;

			} // End switch

			// Rotate from drawing orientation (for easy mathematics in this code) to tool holder orientation.
			m_tool_shape = BRepBuilderAPI_Transform( m_tool_shape, tool_holder_orientation, false );

			// Rotate to use axes typically used for lathe work.
			// i.e. z axis along the bed (from head stock to tail stock as z increases)
			// and x across the bed.
			orient_for_lathe_use.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,1,0) ), degrees_to_radians(-90.0) );
			m_tool_shape = BRepBuilderAPI_Transform( m_tool_shape, orient_for_lathe_use, false );

			orient_for_lathe_use.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1) ), degrees_to_radians(90.0) );
			m_tool_shape = BRepBuilderAPI_Transform( m_tool_shape, orient_for_lathe_use, false );

			return m_tool_shape;
		}

		case CToolParams::eEndmill:
		case CToolParams::eSlotCutter:
		case CToolParams::eExtrusion:
	        case CToolParams::eTapTool:             // reasonable?
		default:
		{
			// First a cylinder to represent the shaft.
			double shaft_length = tool_length_offset;
			gp_Pnt shaft_start_location( tool_tip_location );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );

			m_tool_shape = shaft.Shape();
			return m_tool_shape;
		}
	} // End switch
   } // End try
   // These are due to poor parameter settings resulting in negative lengths and the like.  I need
   // to work through the various parameters to either ensure they're correct or don't try
   // to construct a shape.
   catch (Standard_ConstructionError)
   {
	// printf("Construction error thrown while generating tool shape\n");
	throw;	// Re-throw the exception.
   } // End catch
   catch (Standard_DomainError)
   {
	// printf("Domain error thrown while generating tool shape\n");
	throw;	// Re-throw the exception.
   } // End catch
} // End GetShape() method

Python CTool::OCLDefinition(CSurface* surface) const
{
	Python python;

	switch (m_params.m_type)
	{
		case CToolParams::eBallEndMill:
			python << _T("ocl.BallCutter(float(") << m_params.m_diameter + surface->m_material_allowance * 2 << _T("), 1000)\n");
			break;

		case CToolParams::eChamfer:
		case CToolParams::eEngravingTool:
			python << _T("ocl.CylConeCutter(float(") << m_params.m_flat_radius * 2 + surface->m_material_allowance << _T("), float(") << m_params.m_diameter + surface->m_material_allowance * 2 << _T("), float(") << m_params.m_cutting_edge_angle * PI/360 << _T("))\n");
			break;

		default:
			if(this->m_params.m_corner_radius > 0.000000001)
			{
				python << _T("ocl.BullCutter(float(") << m_params.m_diameter + surface->m_material_allowance * 2 << _T("), float(") << m_params.m_corner_radius << _T("), 1000)\n");
			}
			else
			{
				python << _T("ocl.CylCutter(float(") << m_params.m_diameter + surface->m_material_allowance * 2 << _T("), 1000)\n");
			}
			break;
	} // End switch

	return python;

} // End GetShape() method

TopoDS_Face CTool::GetSideProfile() const
{
   try {
	gp_Dir orientation(0,0,1);	// This method always draws it up and down.  Leave it
					// for other methods to rotate the resultant shape if
					// they need to.
	gp_Pnt tool_tip_location(0,0,0);	// Always from the origin in this method.

	double diameter = m_params.m_diameter;
	if (diameter < 0.01) diameter = 2;

	double tool_length_offset = m_params.m_tool_length_offset;
	if (tool_length_offset <  diameter) tool_length_offset = 10 * diameter;

	double cutting_edge_height = m_params.m_cutting_edge_height;
	if (cutting_edge_height < (2 * diameter)) cutting_edge_height = 2 * diameter;

    gp_Pnt top_left( tool_tip_location );
    gp_Pnt top_right( tool_tip_location );
    gp_Pnt bottom_left( tool_tip_location );
    gp_Pnt bottom_right( tool_tip_location );

    top_left.SetY( tool_tip_location.Y() - CuttingRadius());
    bottom_left.SetY( tool_tip_location.Y() - CuttingRadius());

    top_right.SetY( tool_tip_location.Y() + CuttingRadius());
    bottom_right.SetY( tool_tip_location.Y() + CuttingRadius());

    top_left.SetZ( tool_tip_location.Z() + cutting_edge_height);
    bottom_left.SetZ( tool_tip_location.Z() );

    top_right.SetZ( tool_tip_location.Z() + cutting_edge_height);
    bottom_right.SetZ( tool_tip_location.Z() );

    Handle(Geom_TrimmedCurve) seg1 = GC_MakeSegment(top_left, bottom_left);
    Handle(Geom_TrimmedCurve) seg2 = GC_MakeSegment(bottom_left, bottom_right);
    Handle(Geom_TrimmedCurve) seg3 = GC_MakeSegment(bottom_right, top_right);
    Handle(Geom_TrimmedCurve) seg4 = GC_MakeSegment(top_right, top_left);

    TopoDS_Edge edge1 = BRepBuilderAPI_MakeEdge(seg1);
    TopoDS_Edge edge2 = BRepBuilderAPI_MakeEdge(seg2);
    TopoDS_Edge edge3 = BRepBuilderAPI_MakeEdge(seg3);
    TopoDS_Edge edge4 = BRepBuilderAPI_MakeEdge(seg4);

    BRepBuilderAPI_MakeWire wire_maker;

    wire_maker.Add(edge1);
    wire_maker.Add(edge2);
    wire_maker.Add(edge3);
    wire_maker.Add(edge4);

    TopoDS_Face face = BRepBuilderAPI_MakeFace(wire_maker.Wire());
    return(face);
   } // End try
   // These are due to poor parameter settings resulting in negative lengths and the like.  I need
   // to work through the various parameters to either ensure they're correct or don't try
   // to construct a shape.
   catch (Standard_ConstructionError)
   {
	// printf("Construction error thrown while generating tool shape\n");
	throw;	// Re-throw the exception.
   } // End catch
   catch (Standard_DomainError)
   {
	// printf("Domain error thrown while generating tool shape\n");
	throw;	// Re-throw the exception.
   } // End catch
} // End GetSideProfile() method




/**
	The CuttingRadius is almost always the same as half the tool's diameter.
	The exception to this is if it's a chamfering bit.  In this case we
	want to use the flat_radius plus a little bit.  i.e. if we're chamfering the edge
	then we want to use the part of the cutting surface just a little way from
	the flat radius.  If it has a flat radius of zero (i.e. it has a pointed end)
	then it will be a small number.  If it is a carbide tipped bit then the
	flat radius will allow for the area below the bit that doesn't cut.  In this
	case we want to cut around the middle of the carbide tip.  In this case
	the carbide tip should represent the full cutting edge height.  We can
	use this method to make all these adjustments based on the tool's
	geometry and return a reasonable value.

	If express_in_drawing_units is true then we need to divide by the drawing
	units value.  We use metric (mm) internally and convert to inches only
	if we need to and only as the last step in the process.  By default, return
	the value in internal (metric) units.

	If the depth value is passed in as a positive number then the radius is given
	for the corresponding depth (from the bottom-most tip of the tool).  This is
	only relevant for chamfering (angled) bits.
 */
double CTool::CuttingRadius( const bool express_in_drawing_units /* = false */, const double depth /* = -1 */ ) const
{
	double radius;

	switch (m_params.m_type)
	{
		case CToolParams::eChamfer:
			{
			    if (depth < 0.0)
			    {
                    // We want to decide where, along the cutting edge, we want
                    // to machine.  Let's start with 1/3 of the way from the inside
                    // cutting edge so that, as we plunge it into the material, it
                    // cuts towards the outside.  We don't want to run right on
                    // the edge because we don't want to break the top off.

                    // one third from the centre-most point.
                    double proportion_near_centre = 0.3;
                    radius = (((m_params.m_diameter/2) - m_params.m_flat_radius) * proportion_near_centre) + m_params.m_flat_radius;
			    }
			    else
			    {
			        radius = m_params.m_flat_radius + (depth * tan((m_params.m_cutting_edge_angle / 360.0 * 2 * PI)));
			        if (radius > (m_params.m_diameter / 2.0))
			        {
			            // The angle and depth would have us cutting larger than our largest diameter.
			            radius = (m_params.m_diameter / 2.0);
			        }
			    }
			}
			break;

        case CToolParams::eEngravingTool:
                {
            radius = m_params.m_flat_radius;
                }
                break;

		default:
			radius = m_params.m_diameter/2;
	} // End switch

	if (express_in_drawing_units)
	{
	    double scale = Length::Conversion(theApp.m_program->m_units, UnitTypeMillimeter);
	    return(radius / scale);
	}
	else return(radius);

} // End CuttingRadius() method


CToolParams::eToolType CTool::CutterType( const int tool_number )
{
	if (tool_number <= 0) return(CToolParams::eUndefinedToolType);

	CTool *pTool = CTool::Find( tool_number );
	if (pTool == NULL) return(CToolParams::eUndefinedToolType);

	int type = pTool->m_params.m_type;
	return(CToolParams::eToolType(type));
} // End of CutterType() method


CToolParams::eMaterial_t CTool::CutterMaterial( const int tool_number )
{
	if (tool_number <= 0) return(CToolParams::eUndefinedMaterialType);

	CTool *pTool = CTool::Find( tool_number );
	if (pTool == NULL) return(CToolParams::eUndefinedMaterialType);

	int material = pTool->m_params.m_material;
	return(CToolParams::eMaterial_t(material));
} // End of CutterType() method

bool CToolParams::operator==( const CToolParams & rhs ) const
{
	if (m_material != rhs.m_material) return(false);
	if (m_diameter != rhs.m_diameter) return(false);
	if (m_tool_length_offset != rhs.m_tool_length_offset) return(false);
	if (m_x_offset != rhs.m_x_offset) return(false);
	if (m_front_angle != rhs.m_front_angle) return(false);
	if (m_tool_angle != rhs.m_tool_angle) return(false);
	if (m_back_angle != rhs.m_back_angle) return(false);
	if (m_orientation != rhs.m_orientation) return(false);
	if (m_corner_radius != rhs.m_corner_radius) return(false);
	if (m_flat_radius != rhs.m_flat_radius) return(false);
	if (m_cutting_edge_angle != rhs.m_cutting_edge_angle) return(false);
	if (m_cutting_edge_height != rhs.m_cutting_edge_height) return(false);
	if (m_type != rhs.m_type) return(false);
	if (m_max_advance_per_revolution != rhs.m_max_advance_per_revolution) return(false);
	if (m_automatically_generate_title != rhs.m_automatically_generate_title) return(false);
	if (m_probe_offset_x != rhs.m_probe_offset_x) return(false);
	if (m_probe_offset_y != rhs.m_probe_offset_y) return(false);
	if (m_extrusion_material != rhs.m_material) return(false);
	if (m_automatically_generate_title != rhs.m_automatically_generate_title) return(false);
	if (m_layer_height != rhs.m_layer_height) return(false);
	if (m_width_over_thickness != rhs.m_width_over_thickness) return(false);
	if (m_feedrate != rhs.m_feedrate) return(false);
	if (m_temperature != rhs.m_temperature) return(false);
	if (m_flowrate != rhs.m_flowrate) return(false);
	if (m_filament_diameter != rhs.m_filament_diameter) return(false);
	if (m_direction != rhs.m_direction) return(false);
	if (m_pitch != rhs.m_pitch) return(false);
	return(true);
}

bool CTool::operator==( const CTool & rhs ) const
{
	if (m_params != rhs.m_params) return(false);
	if (GetTitle() != rhs.GetTitle()) return(false);
	if (m_tool_number != rhs.m_tool_number) return(false);
	// m_pToolSolid;

	// return(HeeksObj::operator==(rhs));
	return(true);
}

Python CTool::OpenCamLibDefinition(const unsigned int indent /* = 0 */ ) const
{
	Python python;
	Python _indent;

	for (::size_t i=0; i<indent; i++)
	{
		_indent << _T("\040\040\040\040");
	}

	switch (m_params.m_type)
	{
	case CToolParams::eBallEndMill:
		python << _indent << _T("ocl.BallCutter(") << CuttingRadius(false) << _T(", 1000)");
		return(python);

	case CToolParams::eEndmill:
	case CToolParams::eSlotCutter:
		python << _indent << _T("ocl.CylCutter(") << CuttingRadius(false) << _T(", 1000)");
		return(python);

    case CToolParams::eDrill:
    case CToolParams::eCentreDrill:
    case CToolParams::eChamfer:
    case CToolParams::eTurningTool:
    case CToolParams::eTouchProbe:
    case CToolParams::eToolLengthSwitch:
    case CToolParams::eExtrusion:
    case CToolParams::eTapTool:
    case CToolParams::eEngravingTool:
    case CToolParams::eUndefinedToolType:
        return(python); // Avoid the compiler warnings.
	} // End switch

	return(python);
}

static bool OnEdit(HeeksObj* object)
{
    CTool* tool = (CTool*)object;
	CToolDlg dlg(heeksCAD->GetMainFrame(), tool);
	if(dlg.ShowModal() == wxID_OK)
	{
		dlg.GetData(tool);
		tool->m_params.write_values_to_config();
		return true;
	}
	return false;
}

void CTool::GetOnEdit(bool(**callback)(HeeksObj*))
{
#if 1
        *callback = OnEdit;
#else
        *callback = NULL;
#endif
}

void CTool::OnChangeViewUnits(const EnumUnitType units)
{
    if ( m_params.m_automatically_generate_title ) {
        SetTitle ( GetMeaningfulName ( heeksCAD->GetViewUnits ( ) ) );
    }
}

void CTool::WriteDefaultValues()
{
        m_params.write_values_to_config();
}

void CTool::ReadDefaultValues()
{
        m_params.set_initial_values();
}

HeeksObj* CTool::PreferredPasteTarget()
{
        return theApp.m_program->Tools();
}
