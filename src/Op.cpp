
// Op.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include <stdafx.h>
#include "Op.h"
#include "interface/Tool.h"
#include "tinyxml/tinyxml.h"
#include "HeeksCNCTypes.h"
#include "CTool.h"
#include "PythonStuff.h"
#include "CNCConfig.h"
#include "Program.h"

#define FIND_FIRST_TOOL CTool::FindFirstByType
#define FIND_ALL_TOOLS CTool::FindAllTools

#include <iterator>
#include <vector>

const wxBitmap& COp::GetInactiveIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/noentry.png")));
	return *icon;
}

COp::COp(int obj_type, const int tool_number, const int operation_type)
 : IdNamedObjList(obj_type), m_active(true), m_tool_number(tool_number),
   m_operation_type(operation_type), m_pattern(1), m_surface(0)
{
    InitializeProperties();
    ReadDefaultValues();
}

COp::COp( const COp & rhs )
 : IdNamedObjList(rhs)
{
    InitializeProperties();
    *this = rhs;    // Call the assignment operator.
}

COp & COp::operator= ( const COp & rhs )
{
    if (this != &rhs)
    {
            IdNamedObjList::operator=( rhs );

            m_comment = rhs.m_comment;
            m_active = rhs.m_active;
            m_tool_number = rhs.m_tool_number;
            m_operation_type = rhs.m_operation_type;
            m_pattern = rhs.m_pattern;
            m_surface = rhs.m_surface;
    }

    return(*this);
}

void COp::InitializeProperties()
{
    m_comment.Initialize(_("comment"), this);
    m_active.Initialize(_("active"), this);
    m_tool_number_choice.Initialize(_("tool"), this);
    m_pattern.Initialize(_("pattern"), this);
    m_surface.Initialize(_("surface"), this);
}

void COp::OnPropertySet(Property& prop)
{
    if (prop == m_tool_number_choice) {
        std::vector< std::pair< int, wxString > > tools = FIND_ALL_TOOLS();

        if ((m_tool_number_choice >= 0) && (m_tool_number_choice <= int(tools.size()-1))) {
            m_tool_number = tools[m_tool_number_choice].first;       // Convert the choice offset to the tool number for that choice
        } // End if - then

        WriteDefaultValues();
    }
    else {
        IdNamedObjList::OnPropertySet(prop);
    }
}

void COp::ReadBaseXML(TiXmlElement* element)
{
    IdNamedObjList::ReadBaseXML(element);

    std::vector< std::pair< int, wxString > > tools = FIND_ALL_TOOLS();

    if ((m_tool_number_choice >= 0) && (m_tool_number_choice <= int(tools.size()-1))) {
        m_tool_number = tools[m_tool_number_choice].first;       // Convert the choice offset to the tool number for that choice
    } // End if - then
}

void COp::GetProperties(std::list<Property *> *list)
{

    if(UsesTool()) {
        m_tool_number_choice.SetVisible(true);
        m_tool_number_choice.m_choices.clear();
        std::vector< std::pair< int, wxString > > tools = FIND_ALL_TOOLS();
        for (std::vector< std::pair< int, wxString > >::size_type i=0; i<tools.size(); i++) {
            m_tool_number_choice.m_choices.push_back(tools[i].second);

            // Set selection
            if (m_tool_number == tools[i].first) {
                m_tool_number_choice = tools[i].first;
            }
        } // End for
    }
    else {
        m_tool_number_choice.SetVisible(false);
    }

    IdNamedObjList::GetProperties(list);
}

void COp::glCommands(bool select, bool marked, bool no_color)
{
    IdNamedObjList::glCommands(select, marked, no_color);
}


void COp::WriteDefaultValues()
{
	CNCConfig config(GetTypeString());
	config.Write(_T("Tool"), m_tool_number);
    config.Write(_T("Pattern"), m_pattern);
    config.Write(_T("Surface"), m_surface);
}

void COp::ReadDefaultValues()
{
    CNCConfig config(GetTypeString());
	if (m_tool_number <= 0)
	{
		// The tool number hasn't been assigned from above.  Set some reasonable
		// defaults.


		// assume that default.tooltable contains tools with IDs:
		// 1 drill
		// 2 centre drill
		// 3 end mill
		// 4 slot drill
		// 5 ball end mill
		// 6 chamfering bit
		// 7 turn tool

		int default_tool = 0;
		switch(m_operation_type)
		{
        case DrillingType:
            default_tool = FIND_FIRST_TOOL( CToolParams::eDrill );
            if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eCentreDrill );
            break;
        case ProfileType:
        case PocketType:
            default_tool = FIND_FIRST_TOOL( CToolParams::eEndmill );
            if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eSlotCutter );
            if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eBallEndMill );
            break;

		default:
			default_tool = FIND_FIRST_TOOL( CToolParams::eEndmill );
			if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eSlotCutter );
			if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eBallEndMill );
			if (default_tool <= 0) default_tool = 4;
			break;
		}
		config.Read(_T("Tool"), &m_tool_number, default_tool);
	} // End if - then

    config.Read(_T("Pattern"), m_pattern, 0);
    config.Read(_T("Surface"), m_surface, 0);
}

/**
    Change tools (if necessary) and assign any private fixtures.
 */
Python COp::AppendTextToProgram()
{
    Python python;
    wxString comment = m_comment;

    if(comment.Len() > 0)
    {
            python << _T("comment(") << PythonString(comment) << _T(")\n");
    }

    if(UsesTool())
    {
            python << theApp.SetTool(m_tool_number); // Select the correct  tool.
    }

    return(python);
}

void COp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    IdNamedObjList::GetTools( t_list, p );
}

bool COp::operator==(const COp & rhs) const
{
	if ((const wxString&)m_comment != (const wxString&)rhs.m_comment) return(false);
	if (m_active != rhs.m_active) return(false);
	if (m_tool_number != rhs.m_tool_number) return(false);
	if (m_operation_type != rhs.m_operation_type) return(false);

	return(IdNamedObjList::operator==(rhs));
}

//HeeksObj* COp::PreferredPasteTarget()
//{
//    return theApp.m_program->Operations();
//}
