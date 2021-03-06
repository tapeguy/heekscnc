// TrsfNCCode.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "PythonStuff.h"
#include "tinyxml/tinyxml.h"
#include "NCCode.h"
#include "TrsfNCCode.h"
#include "interface/Geom.h"


using namespace std;

CTrsfNCCode::CTrsfNCCode()
 : ObjList(ObjType), m_x(0), m_y(0)
{
	Add(new CNCCode());
}

CTrsfNCCode::~CTrsfNCCode()
{

}

const wxBitmap &CTrsfNCCode::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/program.png")));
	return *icon;
}

HeeksObj *CTrsfNCCode::MakeACopy(void)const
{
	return new CTrsfNCCode(*this);
}


void CTrsfNCCode::GetProperties(std::list<Property *> *list)
{
	HeeksObj::GetProperties(list);
}

void CTrsfNCCode::WriteXML(TiXmlNode *root)
{
}

// static member function
HeeksObj* CTrsfNCCode::ReadFromXMLElement(TiXmlElement* pElem)
{
	return NULL;
}

void CTrsfNCCode::glCommands(bool select, bool marked, bool no_color)
{
	glPushMatrix();

	glTranslated(m_x,m_y,0);
	ObjList::glCommands(select,marked,no_color);

	glPopMatrix();
}

void CTrsfNCCode::ModifyByMatrix(const double *m)
{
	gp_Trsf mat = make_matrix(m);
	gp_Pnt pos(m_x,m_y,0);
	pos.Transform(mat);
	m_x = pos.X();
	m_y = pos.Y();
}

void CTrsfNCCode::WriteCode(wxTextFile &f)
{
	CNCCode* code = (CNCCode*)GetFirstChild();

	std::list<CNCCodeBlock*>::iterator it;
	for(it = code->m_blocks.begin(); it != code->m_blocks.end(); it++)
	{
		CNCCodeBlock* block = *it;
		block->WriteNCCode(f,m_x,m_y);
	}
}

