// Tag.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Tag.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CNCConfig.h"

CTag::CTag ( )
        : m_width ( 10.0 ), m_angle ( 45.0 ), m_height ( 4.0 )
{
    m_pos.SetX ( 0.0 );
    m_pos.SetY ( 0.0 );
    ReadDefaultValues ( );
}

CTag::CTag ( const CTag & rhs )
 : HeeksObj ( rhs )
{
    m_pos = rhs.m_pos;
    m_width = rhs.m_width;
    m_angle = rhs.m_angle;
    m_height = rhs.m_height;
}


CTag& CTag::operator= ( const CTag & rhs )
{
    if ( this != &rhs )
    {
        HeeksObj::operator= ( rhs );

        m_pos = rhs.m_pos;
        m_width = rhs.m_width;
        m_angle = rhs.m_angle;
        m_height = rhs.m_height;
    }

    return ( *this );
}

void CTag::InitializeProperties()
{
    m_pos.Initialize(_("position"), this);
    m_width.Initialize(_("width"), this);
    m_angle.Initialize(_("angle"), this);
    m_height.Initialize(_("height"), this);
}

void CTag::OnPropertyEdit(Property *prop)
{
    this->WriteDefaultValues();
}


const wxBitmap &CTag::GetIcon()
{
    static wxBitmap* icon = NULL;
    if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/tag.png")));
    return *icon;
}

static unsigned char cross16[32] = {0x80, 0x01, 0x40, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20, 0x02, 0x40, 0x01, 0x80, 0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01};
static unsigned char bmp16[10] = {0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0};

void CTag::glCommands(bool select, bool marked, bool no_color)
{
	if(marked)
	{
	    double pos[2];
	    pos[0] = m_pos.X();
	    pos[1] = m_pos.Y();
		glColor3ub(0, 0, 0);
		glRasterPos2dv(pos);
		glBitmap(16, 5, 8, 3, 10.0, 0.0, bmp16);

		glColor3ub(0, 0, 255);
		glRasterPos2dv(pos);
		glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
	}
}

static CTag* object_for_tools = NULL;

class PickPos: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick position");}
	void Run(){
		heeksCAD->CreateUndoPoint();
		double pos[3];
		heeksCAD->PickPosition(_("Pick position"), pos);
		object_for_tools->m_pos.SetX(pos[0]);
		object_for_tools->m_pos.SetY(pos[1]);
		heeksCAD->Changed();
	}
	wxString BitmapPath(){ return theApp.GetResFolder() + _T("/bitmaps/tagpos.png"); }
};

static PickPos pick_pos;

void CTag::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_tools = this;
	t_list->push_back(&pick_pos);

}

void CTag::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Tag" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetDoubleAttribute( "x", m_pos.X());
	element->SetDoubleAttribute( "y", m_pos.Y());
	element->SetDoubleAttribute( "width", m_width);
	element->SetDoubleAttribute( "angle", m_angle);
	element->SetDoubleAttribute( "height", m_height);
	WriteBaseXML(element);
}

//static
HeeksObj* CTag::ReadFromXMLElement(TiXmlElement* pElem)
{
    double d;
	CTag* new_object = new CTag;
	new_object->ReadBaseXML(pElem);
	pElem->Attribute("x", &d);
    new_object->m_pos.SetX(d);
    pElem->Attribute("y", &d);
    new_object->m_pos.SetY(d);
	pElem->Attribute("width", &d);
	new_object->m_width = d;
	pElem->Attribute("angle", &d);
	new_object->m_angle = d;
	pElem->Attribute("height", &d);
	new_object->m_height = d;
	return new_object;
}

void CTag::WriteDefaultValues()
{
	CNCConfig config(ConfigScope());
	config.Write(_T("Width"), m_width);
	config.Write(_T("Angle"), m_angle);
	config.Write(_T("Height"), m_height);
}

void CTag::ReadDefaultValues()
{
	CNCConfig config(ConfigScope());
	config.Read(_T("Width"), m_width);
	config.Read(_T("Angle"), m_angle);
	config.Read(_T("Height"), m_height);
}

bool CTag::operator==( const CTag & rhs ) const
{
	if (m_pos.X() != rhs.m_pos.X() || m_pos.Y() != rhs.m_pos.Y()) return(false);
	if (m_width != rhs.m_width) return(false);
	if (m_angle != rhs.m_width) return(false);
	if (m_height != rhs.m_width) return(false);
	// return(HeeksObj::operator==(rhs));
	return(true);
}
