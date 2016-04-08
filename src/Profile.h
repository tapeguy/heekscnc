// Profile.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "SketchOp.h"
#include "Drilling.h"
#include "CNCPoint.h"

#include <vector>

class CProfile;
class CTags;

class CProfileParams : public DomainObject
{
private:

    CProfile * parent;

public:
    typedef enum {
        eRightOrInside = -1,
        eOn = 0,
        eLeftOrOutside = +1
    } eSide;
	PropertyChoice m_tool_on_side;

	typedef enum {
        eConventional,
        eClimb
    } eCutMode;
    PropertyChoice m_cut_mode;

	// these are only used when m_sketches.size() == 1
	PropertyCheck m_auto_roll_on;
	PropertyCheck m_auto_roll_off;
	PropertyLength m_auto_roll_radius;
	PropertyLength m_lead_in_line_len;
	PropertyLength m_lead_out_line_len;
	PropertyVertex m_roll_on_point;
	PropertyVertex m_roll_off_point;
	PropertyCheck m_start_given;
	PropertyCheck m_end_given;
	PropertyVertex m_start;
	PropertyVertex m_end;
	PropertyLength m_extend_at_start;
	PropertyLength m_extend_at_end;
	PropertyCheck m_end_beyond_full_profile;
	PropertyCheck m_sort_sketches;

	PropertyLength m_offset_extra; // in mm
	PropertyCheck m_do_finishing_pass;
	PropertyCheck m_only_finishing_pass; // don't do roughing pass
	PropertyLength m_finishing_h_feed_rate;
	PropertyChoice m_finishing_cut_mode;
	PropertyLength m_finishing_step_down;

	CProfileParams(CProfile * parent);

	void InitializeProperties();
	void GetProperties(std::list<Property *> *list);

	static wxString ConfigScope() { return(_T("Profile")); }

	bool operator==(const CProfileParams & rhs ) const;
	bool operator!=(const CProfileParams & rhs ) const { return(! (*this == rhs)); }
};

class CProfile: public CSketchOp{
private:
	CTags* m_tags;				// Access via Tags() method

public:
	typedef std::list<int> Sketches_t;
	Sketches_t	m_sketches;
	CProfileParams m_profile_params;

	static PropertyDouble max_deviation_for_spline_to_arc;

	CProfile()
	 : CSketchOp(0, ProfileType), m_tags(NULL), m_profile_params(this)
	{
	}

	CProfile(int sketch, const int tool_number );
	CProfile( const CProfile & rhs );
	CProfile & operator= ( const CProfile & rhs );

	bool operator==( const CProfile & rhs ) const;
	bool operator!=( const CProfile & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CProfile *)other)); }


	// HeeksObj's virtual functions
	int GetType()const{return ProfileType;}
	const wxChar* GetTypeString(void)const{return _T("Profile");}
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	bool Add(HeeksObj* object, HeeksObj* prev_object);
	void Remove(HeeksObj* object);
	bool CanAdd(HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
    void GetOnEdit(bool(**callback)(HeeksObj*));
    void WriteDefaultValues();
    void ReadDefaultValues();
    void Clear();

	// Data access methods.
	CTags* Tags(){return m_tags;}

	Python WriteSketchDefn(HeeksObj* sketch, bool reversed );
	Python AppendTextForSketch(HeeksObj* object, CProfileParams::eCutMode cut_mode);

	// COp's virtual functions
	Python AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void AddMissingChildren();
	Python AppendTextToProgram(bool finishing_pass);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};
