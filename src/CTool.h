
// CTool.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

#include "Op.h"
#include "HeeksCNCTypes.h"
#include "interface/Property.h"
#include <vector>
#include <algorithm>

class CTool;
class CAttachOp;

class CToolParams : public DomainObject {

private:
	CTool * parent;

public:

    typedef enum {
        eDrill = 0,
        eCentreDrill,
        eEndmill,
        eSlotCutter,
        eBallEndMill,
        eChamfer,
        eTurningTool,
        eTouchProbe,
        eToolLengthSwitch,
        eExtrusion,
        eTapTool,
        eEngravingTool,
        eUndefinedToolType
    } eToolType;

    typedef std::pair< eToolType, wxString > ToolTypeDescription_t;
    typedef std::vector<ToolTypeDescription_t > ToolTypesList_t;

    static ToolTypesList_t GetToolTypesList()
    {
        ToolTypesList_t types_list;

        types_list.push_back( ToolTypeDescription_t( eDrill, wxString(_("Drill Bit")) ));
        types_list.push_back( ToolTypeDescription_t( eCentreDrill, wxString(_("Centre Drill Bit")) ));
        types_list.push_back( ToolTypeDescription_t( eEndmill, wxString(_("End Mill")) ));
        types_list.push_back( ToolTypeDescription_t( eSlotCutter, wxString(_("Slot Cutter")) ));
        types_list.push_back( ToolTypeDescription_t( eBallEndMill, wxString(_("Ball End Mill")) ));
        types_list.push_back( ToolTypeDescription_t( eChamfer, wxString(_("Chamfer")) ));
#ifndef STABLE_OPS_ONLY
        types_list.push_back( ToolTypeDescription_t( eTurningTool, wxString(_("Turning Tool")) ));
#endif
        types_list.push_back( ToolTypeDescription_t( eTouchProbe, wxString(_("Touch Probe")) ));
        types_list.push_back( ToolTypeDescription_t( eToolLengthSwitch, wxString(_("Tool Length Switch")) ));
#ifndef STABLE_OPS_ONLY
        types_list.push_back( ToolTypeDescription_t( eExtrusion, wxString(_("Extrusion")) ));
#endif
#ifndef STABLE_OPS_ONLY
        types_list.push_back( ToolTypeDescription_t( eTapTool, wxString(_("Tapping Tool")) ));
#endif
        types_list.push_back( ToolTypeDescription_t( eEngravingTool, wxString(_("Engraving Tool")) ));
        return(types_list);
    } // End GetToolTypesList() method


    typedef enum {
        eHighSpeedSteel = 0,
        eCarbide,
        eUndefinedMaterialType
    } eMaterial_t;

    typedef std::pair< eMaterial_t, wxString > MaterialDescription_t;
    typedef std::vector<MaterialDescription_t > MaterialsList_t;

    static MaterialsList_t GetMaterialsList()
    {
        MaterialsList_t materials_list;

        materials_list.push_back( MaterialDescription_t( eHighSpeedSteel, wxString(_("High Speed Steel")) ));
        materials_list.push_back( MaterialDescription_t( eCarbide, wxString(_("Carbide")) ));

        return(materials_list);
    } // End Get() method

	PropertyChoice m_type_choice;
	PropertyInt    m_type;
	PropertyChoice m_material;	// eMaterial_t - describes the cutting surface type.

	PropertyLength m_diameter;
	PropertyLength m_tool_length_offset;

	// The following are all for lathe tools.  They become relevant when the m_type = eTurningTool
	PropertyLength m_x_offset;
	PropertyDouble m_front_angle;
	PropertyDouble m_tool_angle;
	PropertyDouble m_back_angle;
	PropertyChoice m_orientation;
	// also m_corner_radius, see below, is used for turning tools and milling tools


	/**
		The next three parameters describe the cutting surfaces of the bit.

		The two radii go from the centre of the bit -> flat radius -> corner radius.
		The vertical_cutting_edge_angle is the angle between the centre line of the
		milling bit and the angle of the outside cutting edges.  For an end-mill, this
		would be zero.  i.e. the cutting edges are parallel to the centre line
		of the milling bit.  For a chamfering bit, it may be something like 45 degrees.
		i.e. 45 degrees from the centre line which has both cutting edges at 2 * 45 = 90
		degrees to each other

		For a ball-nose milling bit we would have;
			- m_corner_radius = m_diameter / 2
			- m_flat_radius = 0;	// No middle bit at the bottom of the cutter that remains flat
						// before the corner radius starts.
			- m_cutting_edge_angle = 0

		For an end-mill we would have;
			- m_corner_radius = 0;
			- m_flat_radius = m_diameter / 2
			- m_cutting_edge_angle = 0

		For a chamfering bit we would have;
			- m_corner_radius = 0;
			- m_flat_radius = 0;	// sharp pointed end.  This may be larger if we can't use the centre point.
			- m_cutting_edge_angle = 45	// degrees from centre line of tool
	 */
	PropertyLength m_corner_radius;
	PropertyLength m_flat_radius;
	PropertyDouble m_cutting_edge_angle;
	PropertyLength m_cutting_edge_height;	// How far, from the bottom of the cutter, do the flutes extend?

	PropertyLength m_max_advance_per_revolution;	// This is the maximum distance a tool should advance during a single
							// revolution.  This value is often defined by the manufacturer in
							// terms of an advance no a per-tooth basis.  This value, however,
							// must be expressed on a per-revolution basis.  i.e. we don't want
							// to maintain the number of cutting teeth so a per-revolution
							// value is easier to use.

	PropertyCheck m_automatically_generate_title;	// Set to true by default but reset to false when the user edits the title.

	// The following coordinates relate ONLY to touch probe tools.  They describe
	// the error the probe tool has in locating an X,Y point.  These values are
	// added to a probed point's location to find the actual point.  The values
	// should come from calibrating the touch probe.  i.e. set machine position
	// to (0,0,0), drill a hole and then probe for the centre of the hole.  The
	// coordinates found by the centre finding operation should be entered into
	// these values verbatim.  These will represent how far off concentric the
	// touch probe's tip is with respect to the quil.  Of course, these only
	// make sense if the probe's body is aligned consistently each time.  I will
	// ASSUME this is correct.

	PropertyLength m_probe_offset_x;
	PropertyLength m_probe_offset_y;

	// The following  properties relate to the extrusions created by a reprap style 3D printer.
	// using temperature, speed, and the height of the nozzle, and the nozzle size it's possible to create
	// many different sizes and shapes of extrusion.

	typedef enum {
		eABS = 0,
		ePLA,
		eHDPE,
		eUndefinedExtrusionMaterialType
	} eExtrusionMaterial_t;
	typedef std::pair< eExtrusionMaterial_t, wxString > ExtrusionMaterialDescription_t;
	typedef std::vector<ExtrusionMaterialDescription_t > ExtrusionMaterialsList_t;

	static ExtrusionMaterialsList_t GetExtrusionMaterialsList()
	{
		ExtrusionMaterialsList_t ExtrusionMaterials_list;

		ExtrusionMaterials_list.push_back( ExtrusionMaterialDescription_t( eABS, wxString(_("ABS Plastic")) ));
		ExtrusionMaterials_list.push_back( ExtrusionMaterialDescription_t( ePLA, wxString(_("PLA Plastic")) ));
		ExtrusionMaterials_list.push_back( ExtrusionMaterialDescription_t( eHDPE, wxString(_("HDPE Plastic")) ));

		return(ExtrusionMaterials_list);
	}
	PropertyChoice m_extrusion_material;
	PropertyLength m_feedrate;
	PropertyLength m_layer_height;
	PropertyLength m_width_over_thickness;
	PropertyLength m_temperature;
	PropertyLength m_flowrate;
	PropertyLength m_filament_diameter;


	// The gradient is the steepest angle at which this tool can plunge into the material.  Many
	// tools behave better if they are slowly ramped down into the material.  This gradient
	// specifies the steepest angle of descent.  This is expected to be a negative number indicating
	// the 'rise / run' ratio.  Since the 'rise' will be downward, it will be negative.
	// By this measurement, a drill bit's straight plunge would have an infinite gradient (all rise, no run).
	// To cater for this, a value of zero will indicate a straight plunge.

	PropertyDouble m_gradient;

	// properties for tapping tools
	PropertyChoice m_thread_standard;
	PropertyChoice m_direction;    // 0.. right hand tapping, 1..left hand tapping
        PropertyLength m_pitch;     // in units/rev

	CToolParams(CTool * parent);
	void InitializeProperties();
	void set_initial_values();
	void write_values_to_config();
//	void WriteXMLAttributes(TiXmlNode* pElem);
//	void ReadParametersFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigScope(void)const{return _T("ToolParam_");}
	double ReasonableGradient( const eToolType type ) const;

	bool operator== ( const CToolParams & rhs ) const;
	bool operator!= ( const CToolParams & rhs ) const { return(! (*this == rhs)); }
};

class CTool: public HeeksObj
{
private:

    mutable TopoDS_Shape m_tool_shape;

public:

    static const int ObjType = ToolType;


	//	These are references to the CAD elements whose position indicate where the Tool Cycle begins.
	CToolParams m_params;

	typedef int ToolNumber_t;
	PropertyInt m_tool_number;
	HeeksObj *m_pToolSolid;

	//	Constructors.
	CTool(const wxChar *title, CToolParams::eToolType type, const int tool_number)
	 : HeeksObj(ObjType), m_params(this), m_tool_number(tool_number), m_pToolSolid(NULL)
	{
		m_params.set_initial_values();
		m_params.m_type = type;
		if (title != NULL) {
		    SetTitle ( title );
		}
		else {
		    SetTitle ( GetMeaningfulName(heeksCAD->GetViewUnits()) );
		}
	} // End constructor

	CTool( const CTool & rhs );
	CTool & operator= ( const CTool & rhs );

	~CTool();

	bool operator== ( const CTool & rhs ) const;
	bool operator!= ( const CTool & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CTool *)other)); }

	 // HeeksObj's virtual functions
	const wxChar* GetTypeString(void) const{ return _T("Tool"); }
    void InitializeProperties();
    HeeksObj *MakeACopy(void)const;

    void WriteXML(TiXmlNode *root);
    static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

    // program whose job is to generate RS-274 GCode.
    Python AppendTextToProgram();
    Python OCLDefinition(CSurface* surface)const;

    void GetProperties(std::list<Property *> *list);
    void CopyFrom(const HeeksObj* object);
    bool CanAddTo(HeeksObj* owner);
    const wxBitmap &GetIcon();
    void glCommands(bool select, bool marked, bool no_color);
    void KillGLLists(void);
    bool CanEditString(void)const{return true;}
    void OnEditString(const wxChar* str);
    void WriteDefaultValues();
    void ReadDefaultValues();
    HeeksObj* PreferredPasteTarget();

	static CTool *Find( const int tool_number );
	static int FindTool( const int tool_number );
    static CToolParams::eToolType FindToolType( const int tool_number );
    static std::vector< std::pair< int, wxString > > FindAllTools();
    static bool IsMillingToolType( CToolParams::eToolType type );
	static ToolNumber_t FindFirstByType( const CToolParams::eToolType type );
	wxString GetMeaningfulName(EnumUnitType units) const;
	wxString ResetTitle();
	static wxString FractionalRepresentation( const double original_value, const int max_denominator = 64 );

	const TopoDS_Shape& GetShape() const;
	TopoDS_Face  GetSideProfile() const;

	double CuttingRadius(const bool express_in_drawing_units = false, const double depth = -1) const;
	static CToolParams::eToolType CutterType( const int tool_number );
	static CToolParams::eMaterial_t CutterMaterial( const int tool_number );

	void OnPropertySet(Property& prop);

	void SetAngleAndRadius();
	Python OpenCamLibDefinition(const unsigned int indent = 0)const;
	Python VoxelcutDefinition()const;

	void GetOnEdit(bool(**callback)(HeeksObj*));
	void OnChangeViewUnits(const EnumUnitType units);

private:
	void DeleteSolid();
}; // End CTool class definition.

