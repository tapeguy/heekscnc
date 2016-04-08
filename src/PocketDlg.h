// PocketDlg.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CPocket;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "SketchOpDlg.h"

class PocketDlg : public SketchOpDlg
{
	enum
	{
		ID_STARTING_PLACE = ID_SKETCH_ENUM_MAX,
		ID_CUT_MODE,
		ID_POST_PROCESSOR
	};

	CLengthCtrl *m_lgthStepOver;
	CLengthCtrl *m_lgthMaterialAllowance;
	wxComboBox *m_cmbStartingPlace;
	wxComboBox *m_cmbCutMode;
	wxComboBox *m_cmbEntryMove;
	wxComboBox *m_cmbPostProcessor;
	CDoubleCtrl *m_dblZigAngle;

	void EnableZigZagControls();

public:
    PocketDlg(wxWindow *parent, CPocket* object, const wxString& title = wxString(_T("Pocket Operation")), bool top_level = true);

	static bool Do(CPocket* object);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	void OnPostProcessor(wxCommandEvent& event);
	void OnHelp( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
