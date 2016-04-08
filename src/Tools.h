// Tools.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"

#pragma once

class CTools: public ObjList
{
public:

    static const int ObjType = ToolsType;


    typedef enum {
        eGaugeReplacesSize = 0,
        eIncludeGaugeAndSize
	} TitleFormat_t;

    PropertyChoice m_title_format;

public:

	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	const wxChar* GetTypeString(void)const{return _("Tools");}
	HeeksObj *MakeACopy(void)const;

	CTools();
	CTools( const CTools & rhs );
	CTools & operator= ( const CTools & rhs );

	void InitializeProperties();
	void OnPropertySet(Property& prop);

	bool operator==( const CTools & rhs ) const { return(ObjList::operator==(rhs)); }
	bool operator!=( const CTools & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CTools *)other)); }

	const wxBitmap &GetIcon();
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	bool UsesID() { return(false); }
	void CopyFrom(const HeeksObj* object);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	void OnChangeUnits(const double units);

	static wxString ConfigScope() { return(_("Tools")); }
};
