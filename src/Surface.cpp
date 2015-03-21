// Surface.cpp

#include <stdafx.h>

#include "Surface.h"
#include "Surfaces.h"
#include "Program.h"
#include "CNCConfig.h"
#include "tinyxml/tinyxml.h"
#include "interface/Property.h"
#include "Reselect.h"
#include "SurfaceDlg.h"

int CSurface::number_for_stl_file = 1;

CSurface::CSurface()
 : IdNamedObj(ObjType)
{
	ReadDefaultValues();
	InitializeProperties();
}

CSurface::CSurface(const std::list<int> &solids, double tol, double mat_allowance)
 : IdNamedObj(ObjType), m_solids(solids), m_tolerance(tol),
   m_material_allowance(mat_allowance), m_same_for_each_pattern_position(true)
{
    InitializeProperties();
}

HeeksObj *CSurface::MakeACopy(void)const
{
	return new CSurface(*this);
}

void CSurface::InitializeProperties()
{
    m_tolerance.Initialize(_("tolerance"), this);
    m_material_allowance.Initialize(_("material allowance"), this);
    m_same_for_each_pattern_position.Initialize(_("same for each pattern position"), this);
}

void CSurface::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Surface" );
	heeksCAD->LinkXMLEndChild( root,  element );

	// write solid ids
	for (std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
    {
		int solid = *It;

		TiXmlElement * solid_element = heeksCAD->NewXMLElement( "Solid" );
		heeksCAD->LinkXMLEndChild( element, solid_element );
		solid_element->SetAttribute("id", solid);
	}

	WriteBaseXML(element);
}

// static member function
HeeksObj* CSurface::ReadFromXMLElement(TiXmlElement* element)
{
	CSurface* new_object = new CSurface;

	// read solid ids
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "Solid"){
			for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
			{
				std::string name(a->Name());
				if(name == "id"){
					int id = a->IntValue();
					new_object->m_solids.push_back(id);
				}
			}
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}

void CSurface::WriteDefaultValues()
{
	CNCConfig config;
	config.Write(wxString(GetTypeString()) + _T("Tolerance"), m_tolerance);
	config.Write(wxString(GetTypeString()) + _T("MatAllowance"), m_material_allowance);
	config.Write(wxString(GetTypeString()) + _T("SameForPositions"), m_same_for_each_pattern_position);
}

void CSurface::ReadDefaultValues()
{
	CNCConfig config;
	config.Read(wxString(GetTypeString()) + _T("Tolerance"), m_tolerance, 0.01);
	config.Read(wxString(GetTypeString()) + _T("MatAllowance"), m_material_allowance, 0.0);
	config.Read(wxString(GetTypeString()) + _T("SameForPositions"), m_same_for_each_pattern_position, true);
}

void CSurface::GetProperties(std::list<Property *> *list)
{
	AddSolidsProperties(list, m_solids);

	IdNamedObj::GetProperties(list);
}

void CSurface::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CSurface*)object));
	}
}

bool CSurface::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == SurfacesType));
}

const wxBitmap &CSurface::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/surface.png")));
	return *icon;
}

static bool OnEdit(HeeksObj* object)
{
	return SurfaceDlg::Do((CSurface*)object);
}

void CSurface::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}

HeeksObj* CSurface::PreferredPasteTarget()
{
	return theApp.m_program->Surfaces();
}
