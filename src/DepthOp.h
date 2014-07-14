// DepthOp.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

// base class for machining operations which can have multiple depths

#ifndef DEPTH_OP_HEADER
#define DEPTH_OP_HEADER

#include "SpeedOp.h"
#include <list>

class CDepthOp;

class CDepthOpParams : public DomainObject {
private:
	CDepthOp * parent;
	PropertyLength m_clearance_height;
	PropertyString m_clearance_height_string;

public:
	PropertyLength m_start_depth;
	PropertyLength m_step_down;
	PropertyLength m_final_depth;
	PropertyLength m_rapid_safety_space;
	//check to see if in Absolute or Incremental mode for moves
	typedef enum {
		eAbsolute = 0,
		eIncremental
	}eAbsMode;
	PropertyChoice m_abs_mode;

	CDepthOpParams(CDepthOp * parent);
	bool operator== ( const CDepthOpParams & rhs ) const;
	bool operator!= ( const CDepthOpParams & rhs ) const { return(! (*this == rhs)); }

	void InitializeProperties();
	void set_initial_values(const std::list<int> *sketches = NULL, const int tool_number = -1);
	void write_values_to_config();
	void GetProperties(std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	double ClearanceHeight() const;
	void ClearanceHeight( const double value ) { m_clearance_height = value; }
};

class CDepthOp : public CSpeedOp
{
public:
	CDepthOpParams m_depth_op_params;

	CDepthOp(const wxString& title, const std::list<int> *sketches = NULL, const int tool_number = -1, const int operation_type = UnknownType );

	CDepthOp(const wxString& title, const std::list<HeeksObj *> sketches, const int tool_number = -1, const int operation_type = UnknownType );

	CDepthOp( const CDepthOp & rhs );

	CDepthOp & operator= ( const CDepthOp & rhs );

	// HeeksObj's virtual functions
	void OnPropertyEdit(Property * prop);
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	void ReloadPointers();

	// COp's virtual functions
	void WriteDefaultValues();
	void ReadDefaultValues();
	Python AppendTextToProgram(CMachineState *pMachineState);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);

	void SetDepthsFromSketchesAndTool(const std::list<int> *sketches);
	void SetDepthsFromSketchesAndTool(const std::list<HeeksObj *> sketches);
	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);

	std::list<double> GetDepths() const;

	bool operator== ( const CDepthOp & rhs ) const;
	bool operator!= ( const CDepthOp & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent(HeeksObj *other) { return(*this != (*((CDepthOp *) other))); }
};

#endif
