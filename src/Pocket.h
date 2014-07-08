// Pocket.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "DepthOp.h"
#include "CTool.h"

class CPocket;

class CPocketParams : public MutableObject {
private:
	CPocket * parent;

public:
	PropertyChoice m_starting_place;
	PropertyLength m_material_allowance;
	PropertyLength m_step_over;
	PropertyCheck m_keep_tool_down_if_poss;
	PropertyCheck m_use_zig_zag;
	PropertyDouble m_zig_angle;
	PropertyCheck m_zig_unidirectional;

	typedef enum {
		eConventional = 0,
		eClimb
	} eCutMode;
	eCutMode m_cut_mode;

	typedef enum {
		ePlunge = 0,
		eRamp,
		eHelical,
		eUndefinedeDescentStrategy
	} eEntryStyle;
	PropertyChoice m_entry_move;

	CPocketParams(CPocket* parent);

	void InitializeProperties();
	void set_initial_values(const CTool::ToolNumber_t tool_number);
	void GetProperties(std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
	static wxString ConfigScope() { return(_T("Pocket")); }

	bool operator== ( const CPocketParams & rhs ) const;
	bool operator!= ( const CPocketParams & rhs ) const { return(! (*this == rhs)); }
};

class CPocket: public CDepthOp{
public:
	typedef std::list<int> Sketches_t;
	Sketches_t m_sketches;
	CPocketParams m_pocket_params;

	static PropertyDouble max_deviation_for_spline_to_arc;

	CPocket():CDepthOp(GetTypeString(), 0, PocketType), m_pocket_params(this){}
	CPocket(const std::list<int> &sketches, const int tool_number );
	CPocket(const std::list<HeeksObj *> &sketches, const int tool_number );

	CPocket( const CPocket & rhs );
	CPocket & operator= ( const CPocket & rhs );

	bool operator== ( const CPocket & rhs ) const;
	bool operator!= ( const CPocket & rhs ) const { return(! (*this == rhs)); }

	// HeeksObj's virtual functions
	int GetType()const{return PocketType;}
	const wxChar* GetTypeString(void)const{return _T("Pocket");}
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
#ifdef OP_SKETCHES_AS_CHILDREN
	void ReloadPointers();
#endif
	void GetOnEdit(bool(**callback)(HeeksObj*));
	bool Add(HeeksObj* object, HeeksObj* prev_object);

	// COp's virtual functions
	Python AppendTextToProgram(CMachineState *pMachineState);
	void WriteDefaultValues();
	void ReadDefaultValues();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);

	static void ReadFromConfig();
	static void WriteToConfig();
};
