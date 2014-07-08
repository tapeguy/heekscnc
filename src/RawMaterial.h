
#include "interface/HeeksObj.h"
#include "tinyxml/tinyxml.h"
#include "PythonStuff.h"

#include <wx/string.h>
#include <sstream>
#include <map>
#include <list>

#pragma once

class CProgram;

/**
	\class CRawMaterial
	\ingroup classes
	\brief Defines material hardness of the raw material being machined.  This value helps
		to determine recommended feed and speed settings.
 */
class CRawMaterial : public MutableObject
{
public:
	CRawMaterial();

	PropertyChoice raw_material_choice;
	PropertyChoice hardness_choice;

	wxString m_material_name;
	double m_brinell_hardness;

	double Hardness() const;

	void InitializeProperties();
    void OnPropertyEdit(Property *prop);

	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	Python AppendTextToProgram();

	static wxString ConfigScope() { return(_T("RawMaterial")); }

	bool operator== ( const CRawMaterial & rhs ) const;
	bool operator!= ( const CRawMaterial & rhs ) const { return(! (*this == rhs)); }

}; // End CRawMaterial class definition.

