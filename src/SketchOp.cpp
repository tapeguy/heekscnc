// SketchOp.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "SketchOp.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/Property.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CTool.h"
#include "Reselect.h"


CSketchOp::CSketchOp(int sketch, const int tool_number, const int operation_type)
 : CDepthOp(tool_number, operation_type), m_sketch(sketch)
{
    InitializeProperties();
}

CSketchOp::CSketchOp( const CSketchOp & rhs ) : CDepthOp(rhs)
{
    InitializeProperties();
	m_sketch = rhs.m_sketch;
}

CSketchOp & CSketchOp::operator= ( const CSketchOp & rhs )
{
    if (this != &rhs)
    {
        m_sketch = rhs.m_sketch;
        CDepthOp::operator=( rhs );
    }

    return(*this);
}

void CSketchOp::InitializeProperties()
{
    m_sketch.Initialize(_("sketch_id"), this);
}

void CSketchOp::ReloadPointers()
{
	CDepthOp::ReloadPointers();
}

void CSketchOp::GetBox(CBox &box)
{
	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, m_sketch);
	if (sketch)
	{
	    sketch->GetBox(box);
	}
}

void CSketchOp::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands(select, marked, no_color);

	if (select || marked)
	{
		// allow sketch operations to be selected
		HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, m_sketch);
		if (sketch)sketch->glCommands(select, marked, no_color);
	}
}

Python CSketchOp::AppendTextToProgram()
{
	Python python;
    python << CDepthOp::AppendTextToProgram();
	return(python);
}

static ReselectSketch reselect_sketch;

void CSketchOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	reselect_sketch.m_sketch = m_sketch;
	reselect_sketch.m_object = this;
	t_list->push_back(&reselect_sketch);
    CDepthOp::GetTools( t_list, p );
}

bool CSketchOp::operator== ( const CSketchOp & rhs ) const
{
	return(CDepthOp::operator==(rhs));
}
