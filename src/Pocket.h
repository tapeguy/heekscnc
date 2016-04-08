// Pocket.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "SketchOp.h"
#include "CTool.h"

class CPocket;

class CPocketParams : public DomainObject {
private:
	CPocket * parent;

public:
	PropertyChoice m_starting_place;
	PropertyLength m_material_allowance;
	PropertyLength m_step_over;

    typedef enum {
        eZigZag = 0,
        eZigZagUnidirectional,
        eOffsets,
        eTrochoidal

    } ePostProcessor;
    PropertyChoice m_post_processor;

    bool IsZigZag() const {
        return ( m_post_processor == CPocketParams::eZigZag ||
                 m_post_processor == CPocketParams::eZigZagUnidirectional );
    }

	PropertyDouble m_zig_angle;

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
	static wxString ConfigScope() { return(_T("Pocket")); }

	bool operator== ( const CPocketParams & rhs ) const;
	bool operator!= ( const CPocketParams & rhs ) const { return(! (*this == rhs)); }
};

class CPocket: public CSketchOp{
public:
	CPocketParams m_pocket_params;

	static PropertyDouble max_deviation_for_spline_to_arc;

	CPocket():CSketchOp(0, PocketType), m_pocket_params(this){}
	CPocket(int sketch, const int tool_number );
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
	bool CanAddTo(HeeksObj* owner);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void GetOnEdit(bool(**callback)(HeeksObj*));
	bool Add(HeeksObj* object, HeeksObj* prev_object);

	// COp's virtual functions
	Python AppendTextToProgram();
	void WriteDefaultValues();
	void ReadDefaultValues();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

    void WritePocketPython(Python &python);

    static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};
