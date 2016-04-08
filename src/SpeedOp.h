// SpeedOp.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

// base class for machining operations which have feedrates and spindle speed

#ifndef SPEED_OP_HEADER
#define SPEED_OP_HEADER

#include "Op.h"

class CSpeedOp;

class CSpeedOpParams : public DomainObject
{
private:
    CSpeedOp * parent;

public:
    PropertyLength m_slot_feed_rate;
    PropertyLength m_horizontal_feed_rate;
    PropertyLength m_vertical_feed_rate;
    PropertyDouble m_spindle_speed;

	CSpeedOpParams(CSpeedOp * parent);
	bool operator== ( const CSpeedOpParams & rhs ) const;
	bool operator!= ( const CSpeedOpParams & rhs ) const { return(! (*this == rhs)); }

	void InitializeProperties();
};

class CSpeedOp : public COp
{
public:
	CSpeedOpParams m_speed_op_params;

	static PropertyCheck m_auto_set_speeds_feeds;

	CSpeedOp(const int tool_number = -1, const int operation_type = UnknownType )
     : COp(tool_number, operation_type), m_speed_op_params(this)
    {
        ReadDefaultValues();
    }

	CSpeedOp & operator= ( const CSpeedOp & rhs );
	CSpeedOp( const CSpeedOp & rhs );

	// COp's virtual functions
	void WriteDefaultValues();
	void ReadDefaultValues();
	Python AppendTextToProgram();

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);
	void ReloadPointers() { COp::ReloadPointers(); }

	static wxString ConfigScope() { return(_T("SpeedOp")); }

	bool operator==( const CSpeedOp & rhs ) const;
	bool operator!=( const CSpeedOp & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent(HeeksObj *other) { return( *this != (*((CSpeedOp *) other))); }
};

#endif
