// Tag.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "interface/HeeksObj.h"
#include "HeeksCNCTypes.h"

#pragma once


class CTag: public HeeksObj
{
public:

    static const int ObjType = TagType;


    PropertyVertex2d m_pos; // at middle of tag, x and y
    PropertyLength m_width;
    PropertyDouble m_angle; // ramp angle, in degrees
    PropertyLength m_height;

	CTag();
	CTag( const CTag & rhs );
	CTag & operator= ( const CTag & rhs );

	bool operator==( const CTag & rhs ) const;
	bool operator!=( const CTag & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CTag *)other)); }

	// HeeksObj's virtual functions
    void InitializeProperties();
    void OnPropertySet(Property& prop);
	const wxChar* GetTypeString(void)const{return _("Tag");}
	void glCommands(bool select, bool marked, bool no_color);
	HeeksObj *MakeACopy(void)const{ return new CTag(*this);}
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	const wxBitmap &GetIcon();
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == TagsType;}
	bool UsesID() { return(false); }

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void WriteDefaultValues();
	void ReadDefaultValues();

	static wxString ConfigScope() { return(_T("Tag")); }
};
