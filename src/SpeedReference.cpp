// SpeedReference.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <math.h>
#include "SpeedReference.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/HeeksColor.h"
#include "tinyxml/tinyxml.h"

#include <sstream>
#include <string>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;


const wxBitmap &CSpeedReference::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)
	    icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/speeds.png")));
	return *icon;
}

void CSpeedReference::InitializeProperties()
{
	{
		CToolParams::MaterialsList_t materials = CToolParams::GetMaterialsList();

		int choice = -1;
		std::list< wxString > choices;
		for (CToolParams::MaterialsList_t::size_type i=0; i<materials.size(); i++)
		{
			choices.push_back(materials[i].second);
			if (m_tool_material == materials[i].first) choice = int(i);

		} // End for
		m_tool_material.Initialize(_("Material"), this);
		m_tool_material.m_choices = choices;
	}

	m_brinell_hardness_of_raw_material.Initialize(_("Brinell hardness of raw material"), this);
    m_surface_speed.Initialize(_("Surface Speed"), this);
    m_material_name.Initialize(_("Raw Material Name"), this);
}

void CSpeedReference::GetProperties(std::list<Property *> *list)
{
	if (theApp.m_program->m_units == 1.0)
	{
		// We're in metric.
	    m_surface_speed.SetTitle(_("Surface Speed (m/min)"));
	} // End if - then
	else
	{
		// We're using imperial measurements.
		double feet_per_metre = 1000 / (25.4 * 12.0);
        m_surface_speed.SetTitle(_("Surface Speed (ft/min)"));
        m_surface_speed = m_surface_speed * feet_per_metre;
	} // End if - else

	HeeksObj::GetProperties(list);
}

void CSpeedReference::OnPropertyEdit(Property& prop)
{
    /**
        This is either expressed in metres per minute (when m_units = 1) or feet per minute (when m_units = 25.4).  These
        are not the normal conversion for the m_units parameter but these seem to be the two standards by which these
        surface speeds are specified.
     */
    if (prop == m_surface_speed)
    {
        if (theApp.m_program->m_units != 1.0)
        {
            // We're using imperial measurements.  Convert from 'feet per minute' into
            // 'metres per minute' for a consistent internal representation.

            double metres_per_foot = (25.4 * 12.0) / 1000.0;
            m_surface_speed = m_surface_speed * metres_per_foot;
        } // End if - else
        this->ResetTitle();
    }
    else if (prop == m_tool_material)
    {
        this->ResetTitle();
        heeksCAD->RefreshProperties();
    }
    HeeksObj::OnPropertyEdit(prop);
}

HeeksObj *CSpeedReference::MakeACopy(void)const
{
	return new CSpeedReference(*this);
}

void CSpeedReference::CopyFrom(const HeeksObj* object)
{
	operator=(*((CSpeedReference*)object));
}

bool CSpeedReference::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == SpeedReferencesType));
}

void CSpeedReference::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "SpeedReference" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetAttribute( "title", m_title.utf8_str());
	element->SetAttribute( "surface_speed", m_surface_speed );
	element->SetAttribute( "tool_material", int(m_tool_material) );
	element->SetAttribute( "brinell_hardness_of_raw_material", m_brinell_hardness_of_raw_material );
	element->SetAttribute( "raw_material_name", ((const wxString&)m_material_name).utf8_str() );

	WriteBaseXML(element);
}

// static member function
HeeksObj* CSpeedReference::ReadFromXMLElement(TiXmlElement* element)
{
	double brinell_hardness_of_raw_material = 15.0;
	if (element->Attribute("brinell_hardness_of_raw_material"))
		 element->Attribute("brinell_hardness_of_raw_material", &brinell_hardness_of_raw_material);

	double surface_speed = 600.0;
	if (element->Attribute("surface_speed")) element->Attribute("surface_speed", &surface_speed);

	wxString material_name;
	if (element->Attribute("raw_material_name")) material_name = Ctt(element->Attribute("raw_material_name"));

	int tool_material = int(CToolParams::eCarbide);
	if (element->Attribute("tool_material")) element->Attribute("tool_material", &tool_material);

	wxString title(Ctt(element->Attribute("title")));
	CSpeedReference* new_object = new CSpeedReference( 	material_name,
								tool_material,
								brinell_hardness_of_raw_material,
								surface_speed );
	new_object->ReadBaseXML(element);
	return new_object;
}


void CSpeedReference::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->Changed();
}

void CSpeedReference::ResetTitle()
{
#ifdef UNICODE
	std::wostringstream l_ossTitle;
#else
	std::ostringstream l_ossTitle;
#endif

	CToolParams::MaterialsList_t materials = CToolParams::GetMaterialsList();
	std::map< int, wxString > materials_map;
	std::copy( materials.begin(), materials.end(), std::inserter( materials_map, materials_map.begin() ) );

	l_ossTitle << ((const wxString&)m_material_name).c_str()
	           << " (" << m_brinell_hardness_of_raw_material << ") with "
	           << materials_map[m_tool_material].c_str();

	OnEditString(l_ossTitle.str().c_str());
} // End ResetTitle() method


bool CSpeedReference::operator== ( const CSpeedReference & rhs ) const
{
    if (m_tool_material != rhs.m_tool_material)
        return(false);
	if ((const wxString&)m_material_name != (const wxString&)rhs.m_material_name)
	    return(false);
	if (m_brinell_hardness_of_raw_material != rhs.m_brinell_hardness_of_raw_material)
	    return(false);
	if (m_surface_speed != rhs.m_surface_speed)
	    return(false);

	return(true);
}



