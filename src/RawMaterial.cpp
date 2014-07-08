#include "stdafx.h"
#include "Program.h"
#include "HeeksCNC.h"
#include "RawMaterial.h"

#include "interface/Property.h"
#include "interface/HeeksObj.h"
#include "ProgramCanvas.h"
#include "interface/strconv.h"
#include "PythonStuff.h"

#include <wx/string.h>
#include <sstream>
#include <map>
#include <list>
#include <algorithm>
#include <vector>


/**
	\class CRawMaterial
	\ingroup classes
	\brief Defines material hardness of the raw material being machined.  This value helps
		to determine recommended feed and speed settings.
 */
CRawMaterial::CRawMaterial()
{
	CNCConfig config(ConfigScope());
	config.Read(_T("RawMaterial_BrinellHardness"), &m_brinell_hardness, 0.0);
	config.Read(_T("RawMaterial_MaterialName"), &m_material_name, _T("Please select a material to machine"));
}

double CRawMaterial::Hardness() const
{
	return(m_brinell_hardness);
} // End Hardness() method


void CRawMaterial::InitializeProperties ( )
{
    if ((theApp.m_program == NULL) || (theApp.m_program->SpeedReferences() == NULL))
        return;

    {
        std::list< wxString > choices;
        int choice = -1;
        std::set< wxString > materials = theApp.m_program->SpeedReferences()->GetMaterials();

        for (std::set<wxString>::iterator l_itMaterial = materials.begin();
            l_itMaterial != materials.end(); l_itMaterial++)
        {
            choices.push_back(*l_itMaterial);
            if (*l_itMaterial == m_material_name)
            {
                choice = std::distance( materials.begin(), l_itMaterial );
            } // End if - then
        } // End for
        raw_material_choice.Initialize (_("Raw Material"), this);
        raw_material_choice.m_choices = choices;
    }

    {
        std::set<double> hardness_values = theApp.m_program->SpeedReferences()->GetHardnessForMaterial(m_material_name);
        std::list<wxString> choices;

        int choice = -1;
        for (std::set<double>::iterator l_itChoice = hardness_values.begin(); l_itChoice != hardness_values.end(); l_itChoice++)
        {
    #ifdef UNICODE
            std::wostringstream l_ossChoice;
    #else
            std::ostringstream l_ossChoice;
    #endif
            l_ossChoice << *l_itChoice;
            if (m_brinell_hardness == *l_itChoice)
            {
                choice = int(std::distance( hardness_values.begin(), l_itChoice ));
            } // End if - then

            choices.push_back( l_ossChoice.str().c_str() );
        } // End for
        hardness_choice.Initialize (_("Brinell Hardness of raw material"), this);
        hardness_choice.m_choices = choices;
    }
}

void CRawMaterial::OnPropertyEdit(Property *prop)
{
    if ((theApp.m_program == NULL) || (theApp.m_program->SpeedReferences() == NULL))
        return;

    CProgram *pProgram = theApp.m_program;
    int zero_based_choice;

    if (prop == &raw_material_choice) {

        zero_based_choice = raw_material_choice;
        std::set<wxString> materials = CSpeedReferences::GetMaterials();
        std::vector<wxString> copy;

        std::copy( materials.begin(), materials.end(), std::inserter(copy,copy.end()));

        CNCConfig config(CRawMaterial::ConfigScope());
        if (zero_based_choice <= int(copy.size()-1))
        {
            pProgram->m_raw_material.m_material_name = copy[zero_based_choice];
            config.Write(_T("RawMaterial_MaterialName"), pProgram->m_raw_material.m_material_name );

            std::set<double> choices = CSpeedReferences::GetHardnessForMaterial( copy[zero_based_choice] );
            if (choices.size() > 0)
            {
                pProgram->m_raw_material.m_brinell_hardness = *(choices.begin());
                config.Write(_T("RawMaterial_BrinellHardness"), pProgram->m_raw_material.m_brinell_hardness );
            } // End if - then
        } // End if - then
        heeksCAD->Changed();
    }

    else if (prop == &hardness_choice)
    {
        zero_based_choice = raw_material_choice;

        std::set<double> choices = CSpeedReferences::GetHardnessForMaterial( pProgram->m_raw_material.m_material_name );
        std::vector<double> choice_array;
        std::copy( choices.begin(), choices.end(), std::inserter( choice_array, choice_array.begin() ) );
        if (zero_based_choice <= int(choice_array.size()))
        {
            pProgram->m_raw_material.m_brinell_hardness = choice_array[zero_based_choice];
            heeksCAD->Changed();
        } // End if - then

        CNCConfig config(CRawMaterial::ConfigScope());
        config.Write(_T("RawMaterial_BrinellHardness"), pProgram->m_raw_material.m_brinell_hardness );
    }
}

void CRawMaterial::WriteBaseXML(TiXmlElement *element)
{
	element->SetDoubleAttribute( "brinell_hardness", m_brinell_hardness);
	element->SetAttribute( "raw_material_name", m_material_name.utf8_str());
} // End WriteBaseXML() method

void CRawMaterial::ReadBaseXML(TiXmlElement* element)
{
	if (element->Attribute("brinell_hardness"))
	{
		element->Attribute("brinell_hardness", &m_brinell_hardness);
	} // End if - then

	if (element->Attribute("raw_material_name"))
	{
		m_material_name = wxString(Ctt(element->Attribute("raw_material_name")));
	} // End if - then
} // End ReadBaseXML() method

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
Python CRawMaterial::AppendTextToProgram()
{
	Python python;

	python << _T("comment(") << PythonString(wxString(_("Feeds and Speeds set for machining ")) + m_material_name) << _T(")\n");
	return(python);
}

bool CRawMaterial::operator==( const CRawMaterial & rhs ) const
{
	if (m_material_name != rhs.m_material_name) return(false);
	if (m_brinell_hardness != rhs.m_brinell_hardness) return(false);

	return(true);
}

