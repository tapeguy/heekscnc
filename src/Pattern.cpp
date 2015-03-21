// Pattern.cpp

#include <stdafx.h>

#include "Pattern.h"
#include "Patterns.h"
#include "Program.h"
#include "interface/Property.h"
#include "CNCConfig.h"
#include "tinyxml/tinyxml.h"
#include "PatternDlg.h"

CPattern::CPattern()
 : IdNamedObj(ObjType)
{
	//ReadDefaultValues();
    InitializeProperties();
	m_copies1 = 2;
	m_x_shift1 = 50;
	m_y_shift1 = 0;
	m_copies2 = 1;
	m_x_shift2 = 0;
	m_y_shift2 = 50;
}

CPattern::CPattern(int copies1, double x_shift1, double y_shift1, int copies2, double x_shift2, double y_shift2)
 : IdNamedObj(ObjType),
   m_copies1(copies1), m_x_shift1(x_shift1), m_y_shift1(y_shift1),
   m_copies2(copies2), m_x_shift2(x_shift2), m_y_shift2(y_shift2)
{
    InitializeProperties();
}

HeeksObj *CPattern::MakeACopy(void) const
{
	return new CPattern(*this);
}

// static member function
HeeksObj* CPattern::ReadFromXMLElement(TiXmlElement* element)
{
	CPattern* new_object = new CPattern;
	new_object->ReadBaseXML(element);
	return new_object;
}

void CPattern::InitializeProperties()
{
    m_copies1.Initialize(_("number of copies 1"), this);
    m_x_shift1.Initialize(_("x shift 1"), this);
    m_y_shift1.Initialize(_("y shift 1"), this);
    m_copies2.Initialize(_("number of copies 2"), this);
    m_x_shift2.Initialize(_("x shift 2"), this);
    m_y_shift2.Initialize(_("y shift 2"), this);
}

void CPattern::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CPattern*)object));
	}
}

bool CPattern::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == PatternsType));
}

const wxBitmap &CPattern::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/pattern.png")));
	return *icon;
}

void CPattern::GetMatrices(std::list<gp_Trsf> &matrices)
{
	gp_Trsf m2;
	gp_Trsf shift_m2;
	shift_m2.SetTranslationPart(gp_Vec(m_x_shift2, m_y_shift2, 0.0));
	gp_Trsf shift_m1;
	shift_m1.SetTranslationPart(gp_Vec(m_x_shift1, m_y_shift1, 0.0));

	for(int j = 0; j<m_copies2; j++)
	{
		gp_Trsf m = m2;

		for(int i = 0; i<m_copies1; i++)
		{
			matrices.push_back(m);
			m = m * shift_m1;
		}

		m2 = m2 * shift_m2;
	}
}

static bool OnEdit(HeeksObj* object)
{
	PatternDlg dlg(heeksCAD->GetMainFrame(), (CPattern*)object);
	if(dlg.ShowModal() == wxID_OK)
	{
		dlg.GetData((CPattern*)object);
		return true;
	}
	return false;
}

void CPattern::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}

HeeksObj* CPattern::PreferredPasteTarget()
{
	return theApp.m_program->Patterns();
}
