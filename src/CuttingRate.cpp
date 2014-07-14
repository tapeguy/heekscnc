// CuttingRate.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <math.h>
#include "CuttingRate.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "tinyxml/tinyxml.h"

#include <sstream>
#include <string>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;


void CCuttingRate::InitializeProperties()
{
	m_brinell_hardness_of_raw_material.Initialize(_("m_brinell_hardness_of_raw_material"), this);
	m_brinell_hardness_of_raw_material.SetVisible(false);
	m_brinell_hardness_choice.Initialize(_("Brinell hardness of raw material"), this);
	std::set<double> all_values = CSpeedReferences::GetAllHardnessValues();
	for (std::set<double>::iterator l_itHardness = all_values.begin(); l_itHardness != all_values.end(); l_itHardness++)
	{
		wxString message;
		message << *l_itHardness;
		m_brinell_hardness_choice.m_choices.push_back( message );
	}
	m_max_material_removal_rate.Initialize(_("Maximum Material Removal Rate (mm^3/min)"), this);
}

void CCuttingRate::OnPropertyEdit(Property& prop)
{
	if (prop == m_brinell_hardness_choice) {
		int choice = m_brinell_hardness_choice;
		if (choice < 0)
		    return;	// An error has occured.

		std::set<double> all_values = CSpeedReferences::GetAllHardnessValues();
		for (std::set<double>::iterator l_itHardness = all_values.begin(); l_itHardness != all_values.end(); l_itHardness++)
		{
			if (std::distance( all_values.begin(), l_itHardness) == choice)
			{
				m_brinell_hardness_of_raw_material = *l_itHardness;
				break;
			} // End if - then
		} // End for
	}
	else if (prop == m_max_material_removal_rate) {
		if (theApp.m_program->m_units != 1.0) {
			double cubic_mm_per_cubic_inch = 25.4 * 25.4 * 25.4;
			m_max_material_removal_rate = m_max_material_removal_rate * cubic_mm_per_cubic_inch;
		}
	}
	this->ResetTitle();
}

const wxBitmap &CCuttingRate::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/cutting_rate.png")));
	return *icon;
}

void CCuttingRate::GetProperties(std::list<Property *> *list)
{

	if (theApp.m_program->m_units == 1.0)
	{
		// We're set to metric.  Just present the internal value.
		m_max_material_removal_rate.SetTitle(_("Maximum Material Removal Rate (mm^3/min)"));
	} // End if - then
	else
	{
		// We're set to imperial.  Convert the internal (metric) value for presentation.

		double cubic_mm_per_cubic_inch = 25.4 * 25.4 * 25.4;
		m_max_material_removal_rate.SetTitle(_("Maximum Material Removal Rate (inches^3/min)"));
		m_max_material_removal_rate = m_max_material_removal_rate / cubic_mm_per_cubic_inch;
	} // End if - else

	HeeksObj::GetProperties(list);
}


HeeksObj *CCuttingRate::MakeACopy(void)const
{
	return new CCuttingRate(*this);
}

void CCuttingRate::CopyFrom(const HeeksObj* object)
{
	operator=(*((CCuttingRate*)object));
}

bool CCuttingRate::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == SpeedReferencesType));
}

void CCuttingRate::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "CuttingRate" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetAttribute( "title", m_title.utf8_str());
	element->SetDoubleAttribute( "brinell_hardness_of_raw_material", m_brinell_hardness_of_raw_material );
	element->SetDoubleAttribute( "max_material_removal_rate", m_max_material_removal_rate );

	WriteBaseXML(element);
}

// static member function
HeeksObj* CCuttingRate::ReadFromXMLElement(TiXmlElement* element)
{
	double brinell_hardness_of_raw_material = 15.0;
	if (element->Attribute("brinell_hardness_of_raw_material"))
		 element->Attribute("brinell_hardness_of_raw_material", &brinell_hardness_of_raw_material);

	double max_material_removal_rate = 1.0;
	if (element->Attribute("max_material_removal_rate"))
		 element->Attribute("max_material_removal_rate", &max_material_removal_rate);

	wxString title(Ctt(element->Attribute("title")));
	CCuttingRate* new_object = new CCuttingRate( 	brinell_hardness_of_raw_material,
							max_material_removal_rate );
	new_object->ReadBaseXML(element);
	return new_object;
}


void CCuttingRate::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->Changed();
}

void CCuttingRate::ResetTitle()
{
#ifdef UNICODE
	std::wostringstream l_ossTitle;
#else
	std::ostringstream l_ossTitle;
#endif

	l_ossTitle << "Brinell (" << m_brinell_hardness_of_raw_material << ") at ";
	if (theApp.m_program->m_units == 1.0)
	{
		// We're set to metric.  Just present the internal value.
		l_ossTitle << m_max_material_removal_rate << " (mm^3/min)";
	} // End if - then
	else
	{
		// We're set to imperial.  Convert the internal (metric) value for presentation.

		double cubic_mm_per_cubic_inch = 25.4 * 25.4 * 25.4;
		l_ossTitle << m_max_material_removal_rate / cubic_mm_per_cubic_inch << " (inches^3/min)";
	} // End if - else

	OnEditString(l_ossTitle.str().c_str());
} // End ResetTitle() method

bool CCuttingRate::operator== ( const CCuttingRate & rhs ) const
{
	if (m_title != rhs.m_title) return(false);
	if (m_brinell_hardness_of_raw_material != rhs.m_brinell_hardness_of_raw_material) return(false);
	if (m_max_material_removal_rate != rhs.m_max_material_removal_rate) return(false);

	// return(HeeksObj::operator==(rhs));
	return(true);
}



