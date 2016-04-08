// SpeedOp.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "SpeedOp.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/PropertyList.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CTool.h"

CSpeedOpParams::CSpeedOpParams(CSpeedOp * parent)
{
    this->parent = parent;
    InitializeProperties();
    m_slot_feed_rate = 0.0;
	m_horizontal_feed_rate = 0.0;
	m_vertical_feed_rate = 0.0;
	m_spindle_speed = 0.0;
}

void CSpeedOpParams::InitializeProperties()
{
    m_slot_feed_rate.Initialize(_("slot feed rate"), parent);
    m_horizontal_feed_rate.Initialize(_("horizontal feed rate"), parent);
    m_vertical_feed_rate.Initialize(_("vertical feed rate"), parent);
    m_spindle_speed.Initialize(_("spindle speed"), parent);
}

void CSpeedOp::glCommands(bool select, bool marked, bool no_color)
{
	COp::glCommands(select, marked, no_color);
}

CSpeedOp & CSpeedOp::operator= ( const CSpeedOp & rhs )
{
	if (this != &rhs)
	{
		COp::operator=(rhs);
		m_speed_op_params = rhs.m_speed_op_params;
		// static bool m_auto_set_speeds_feeds;
	}

	return(*this);
}

CSpeedOp::CSpeedOp( const CSpeedOp & rhs )
 : COp(rhs), m_speed_op_params(this)
{
	m_speed_op_params = rhs.m_speed_op_params;
}

void CSpeedOp::WriteDefaultValues()
{
	COp::WriteDefaultValues();

	CNCConfig config(CSpeedOp::ConfigScope());
    config.Write(_T("SpeedOpSlotFeed"), m_speed_op_params.m_slot_feed_rate);
	config.Write(_T("SpeedOpHorizFeed"), m_speed_op_params.m_horizontal_feed_rate);
	config.Write(_T("SpeedOpVertFeed"), m_speed_op_params.m_vertical_feed_rate);
	config.Write(_T("SpeedOpSpindleSpeed"), m_speed_op_params.m_spindle_speed);
}

void CSpeedOp::ReadDefaultValues()
{
	COp::ReadDefaultValues();

	CNCConfig config(CSpeedOp::ConfigScope());
    config.Read(_T("SpeedOpSlotFeed"), m_speed_op_params.m_slot_feed_rate, 25.0);
	config.Read(_T("SpeedOpHorizFeed"), m_speed_op_params.m_horizontal_feed_rate, 100.0);
	config.Read(_T("SpeedOpVertFeed"), m_speed_op_params.m_vertical_feed_rate, 100.0);
	config.Read(_T("SpeedOpSpindleSpeed"), m_speed_op_params.m_spindle_speed, 7000);
}

Python CSpeedOp::AppendTextToProgram()
{
	Python python;

	python << COp::AppendTextToProgram();

	if (m_speed_op_params.m_spindle_speed != 0)
	{
		python << _T("spindle(") << m_speed_op_params.m_spindle_speed << _T(")\n");
	} // End if - then

	double scale = Length::Conversion(theApp.m_program->m_units, UnitTypeMillimeter);
	python << _T("feedrate_slot(") << m_speed_op_params.m_slot_feed_rate / scale << _T(")\n");
	python << _T("feedrate_hv(") << m_speed_op_params.m_horizontal_feed_rate / scale << _T(", ");
    python << m_speed_op_params.m_vertical_feed_rate / scale << _T(")\n");
    python << _T("flush_nc()\n");

    return(python);
}

/* static */ PropertyCheck CSpeedOp::m_auto_set_speeds_feeds = false;


// static
void CSpeedOp::GetOptions(std::list<Property *> *list)
{
	list->push_back ( &m_auto_set_speeds_feeds );
}

// static
void CSpeedOp::ReadFromConfig()
{
	CNCConfig config(ConfigScope());
	config.Read(_T("SpeedOpAutoSetSpeeds"), m_auto_set_speeds_feeds, true);
}

// static
void CSpeedOp::WriteToConfig()
{
	CNCConfig config(ConfigScope());
	config.Write(_T("SpeedOpAutoSetSpeeds"), m_auto_set_speeds_feeds);
}

void CSpeedOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    COp::GetTools(t_list, p);
}

bool CSpeedOpParams::operator== ( const CSpeedOpParams & rhs ) const
{
    if (m_slot_feed_rate != rhs.m_slot_feed_rate) return(false);
	if (m_horizontal_feed_rate != rhs.m_horizontal_feed_rate) return(false);
	if (m_vertical_feed_rate != rhs.m_vertical_feed_rate) return(false);
	if (m_spindle_speed != rhs.m_spindle_speed) return(false);

	return(true);
}

bool CSpeedOp::operator==(const CSpeedOp & rhs) const
{
	if (m_speed_op_params != rhs.m_speed_op_params) return(false);

	return(COp::operator==(rhs));
}
