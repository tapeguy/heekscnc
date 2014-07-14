// Waterline.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#ifndef STABLE_OPS_ONLY
#include "DepthOp.h"
#include "HeeksCNCTypes.h"

class CWaterline;

class CWaterlineParams : public DomainObject
{
private:

    CWaterline * parent;

public:

    CBox m_box; // z values ignored ( use start_depth, final_depth instead )
    PropertyLength m_min_x;
    PropertyLength m_min_y;
    PropertyLength m_max_x;
    PropertyLength m_max_y;
//    PropertyLength m_step_over;
    PropertyLength m_material_allowance;
    PropertyLength m_tolerance;

    CWaterlineParams ( CWaterline * parent );

    void InitializeProperties();
    void GetProperties(std::list<Property *> *list);
    void OnPropertyEdit(Property& prop);

    void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigScope(void)const{return _T("Waterline");}

	bool operator==( const CWaterlineParams & rhs ) const;
	bool operator!=( const CWaterlineParams & rhs ) const { return(! (*this == rhs)); }
};

class CWaterline: public CDepthOp{
public:
	std::list<int> m_solids;
	CWaterlineParams m_params;
	static int number_for_stl_file;

	CWaterline()
	 : CDepthOp(GetTypeString(), 0, WaterlineType), m_params(this)
	{
	}

	CWaterline(const std::list<int> &solids, const int tool_number = -1);
	CWaterline( const CWaterline & rhs );
	CWaterline & operator= ( const CWaterline & rhs );

	bool operator==( const CWaterline & rhs ) const;
	bool operator!=( const CWaterline & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CWaterline *)other)); }

	// HeeksObj's virtual functions
	int GetType()const{return WaterlineType;}
	const wxChar* GetTypeString(void)const{return _T("Waterline");}
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	bool CanAdd(HeeksObj* object);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	void WriteDefaultValues();
	void ReadDefaultValues();
	Python AppendTextToProgram(CMachineState *pMachineState);
	void ReloadPointers();
	void SetDepthOpParamsFromBox();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
#endif
