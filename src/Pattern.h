// Pattern.h
/*
 * Copyright (c) 2013, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

#include "interface/IdNamedObj.h"
#include "HeeksCNCTypes.h"

class CPattern: public IdNamedObj
{
public:

    static const int ObjType = PatternType;


    PropertyInt m_copies1;
    PropertyDouble m_x_shift1;
    PropertyDouble m_y_shift1;
	PropertyInt m_copies2;
	PropertyDouble m_x_shift2;
	PropertyDouble m_y_shift2;

	//	Constructors.
	CPattern();
	CPattern(int copies1, double x_shift1, double y_shift1, int copies2, double x_shift2, double y_shift2);

	// HeeksObj's virtual functions
	void InitializeProperties();
	const wxChar* GetTypeString(void) const{ return _T("Pattern"); }
    HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	const wxBitmap &GetIcon();
	void GetOnEdit(bool(**callback)(HeeksObj*));
	HeeksObj* PreferredPasteTarget();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void GetMatrices(std::list<gp_Trsf> &matrices);
}; // End CPattern class definition.
