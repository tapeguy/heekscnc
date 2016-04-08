// Tags.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"

#pragma once

class CTags: public ObjList
{
public:

    static const int ObjType = TagsType;


	CTags() : ObjList(TagsType) { }
	CTags( const CTags & rhs );
	CTags & operator= ( const CTags & rhs );

	bool operator==( const CTags & rhs ) const { return(ObjList::operator==(rhs)); }
	bool operator!=( const CTags & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CTags *)other)); }

	// HeeksObj's virtual functions
	const wxChar* GetTypeString(void)const{return _("Tags");}
	bool OneOfAKind(){return true;} // only one in each profile operation
	HeeksObj *MakeACopy(void)const{ return new CTags(*this);}
	const wxBitmap &GetIcon();
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProfileType;}
	bool CanAdd(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	bool AutoExpand(){return true;}
	bool UsesID() { return(false); }

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
