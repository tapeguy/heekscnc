// TurnRough.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#ifndef STABLE_OPS_ONLY

#include "HeeksCNCTypes.h"
#include "SpeedOp.h"
#include "Drilling.h"

#include <vector>

class CTurnRough;

class CTurnRoughParams : public MutableObject
{
private:

    CTurnRough * parent;

public:

    PropertyCheck m_outside;
    PropertyCheck m_front;
    PropertyCheck m_facing;
    PropertyLength m_clearance;

    CTurnRoughParams(CTurnRough * parent);

    void InitializeProperties();

	void set_initial_values();
	void write_values_to_config();
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	static wxString ConfigScope() { return(_T("TurnRough")); }

	bool operator==( const CTurnRoughParams & rhs ) const;
	bool operator!=( const CTurnRoughParams & rhs ) const { return(! (*this == rhs)); }
};

class CTurnRough: public CSpeedOp{
public:
	std::list<int> m_sketches;
	CTurnRoughParams m_turn_rough_params;

	CTurnRough()
	 : CSpeedOp(GetTypeString(), 0, TurnRoughType), m_turn_rough_params(this)
	{
	}

	CTurnRough(const std::list<int> &sketches, const int tool_number )
	 : CSpeedOp(GetTypeString(), tool_number, TurnRoughType),
			m_sketches(sketches), m_turn_rough_params(this)
	{
		m_turn_rough_params.set_initial_values();
	}

	CTurnRough( const CTurnRough & rhs );
	CTurnRough & operator= ( const CTurnRough & rhs );

	bool operator==( const CTurnRough & rhs ) const;
	bool operator!=( const CTurnRough & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CTurnRough *)other)); }

	// HeeksObj's virtual functions
	int GetType()const{return TurnRoughType;}
	const wxChar* GetTypeString(void)const{return _T("Rough Turning");}
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	Python WriteSketchDefn(HeeksObj* sketch, int id_to_use, CMachineState *pMachineState );
	Python AppendTextForOneSketch(HeeksObj* object, int sketch, CMachineState *pMachineState);
	Python AppendTextToProgram(CMachineState *pMachineState);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};

#endif //#ifndef STABLE_OPS_ONLY
