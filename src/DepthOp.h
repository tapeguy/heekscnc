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

public:

    PropertyLength m_clearance_height;
    PropertyLength m_rapid_safety_space;
    PropertyLength m_start_depth;
    PropertyLength m_step_down;
    PropertyLength m_z_finish_depth;
    PropertyLength m_z_thru_depth;
    PropertyLength m_final_depth;
    PropertyString m_user_depths;

	CDepthOpParams(CDepthOp * parent);
	bool operator== ( const CDepthOpParams & rhs ) const;
	bool operator!= ( const CDepthOpParams & rhs ) const { return(! (*this == rhs)); }

	void InitializeProperties();
	void set_initial_values(const std::list<int> *sketches = NULL, const int tool_number = -1);
	void write_values_to_config();
};

class CDepthOp : public CSpeedOp
{
public:
	CDepthOpParams m_depth_op_params;

    CDepthOp(const int tool_number = -1, const int operation_type = UnknownType );

	CDepthOp( const CDepthOp & rhs );

	CDepthOp & operator= ( const CDepthOp & rhs );

	// HeeksObj's virtual functions
	void OnPropertySet(Property& prop);
	void ReloadPointers();

	// COp's virtual functions
	void WriteDefaultValues();
	void ReadDefaultValues();
	Python AppendTextToProgram();
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);

	void SetDepthsFromSketchesAndTool(const std::list<int> *sketches);
	void SetDepthsFromSketchesAndTool(const std::list<HeeksObj *> sketches);

	std::list<double> GetDepths() const;

	bool operator== ( const CDepthOp & rhs ) const;
	bool operator!= ( const CDepthOp & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent(HeeksObj *other) { return(*this != (*((CDepthOp *) other))); }
};

#endif
