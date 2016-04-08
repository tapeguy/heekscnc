// PocketDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "PocketDlg.h"
#include "interface/NiceTextCtrl.h"
#include "Pocket.h"

BEGIN_EVENT_TABLE(PocketDlg, SketchOpDlg)
    EVT_COMBOBOX(ID_STARTING_PLACE,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_CUT_MODE,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_POST_PROCESSOR,PocketDlg::OnPostProcessor)
    EVT_BUTTON(wxID_HELP, PocketDlg::OnHelp)
END_EVENT_TABLE()

PocketDlg::PocketDlg(wxWindow *parent, CPocket* object, const wxString& title, bool top_level)
             : SketchOpDlg(parent, object, title, false)
{
	std::list<HControl> save_leftControls = leftControls;
	leftControls.clear();

	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("Step Over"), m_lgthStepOver = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Material Allowance"), m_lgthMaterialAllowance = new CLengthCtrl(this)));

	wxString starting_place_choices[] = {_("Boundary"), _("Center")};
	leftControls.push_back(MakeLabelAndControl(_("Starting Place"), m_cmbStartingPlace = new wxComboBox(this, ID_STARTING_PLACE, _T(""), wxDefaultPosition, wxDefaultSize, 2, starting_place_choices)));

	wxString cut_mode_choices[] = {_("Conventional"), _("Climb")};
	leftControls.push_back(MakeLabelAndControl(_("Cut Mode"), m_cmbCutMode = new wxComboBox(this, ID_CUT_MODE, _T(""), wxDefaultPosition, wxDefaultSize, 2, cut_mode_choices)));

    wxString post_processor_choices[] = {_("ZigZag"), _("ZigZag Unidirectional"), _("Offsets"), _("Trochoidal")};
    leftControls.push_back(MakeLabelAndControl(_("Post-Processor"), m_cmbPostProcessor = new wxComboBox(this, ID_POST_PROCESSOR, _T(""), wxDefaultPosition, wxDefaultSize, 4, post_processor_choices)));

	leftControls.push_back(MakeLabelAndControl(_("Zig Zag Angle"), m_dblZigAngle = new CDoubleCtrl(this)));

	for(std::list<HControl>::iterator It = save_leftControls.begin(); It != save_leftControls.end(); It++)
	{
		leftControls.push_back(*It);
	}

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_cmbSketch->SetFocus();
	}
}

void PocketDlg::GetDataRaw(HeeksObj* object)
{
    CPocket* pocket = (CPocket*)object;
    pocket->m_pocket_params.m_step_over = m_lgthStepOver->GetValueAsDouble();
    pocket->m_pocket_params.m_material_allowance = m_lgthMaterialAllowance->GetValueAsDouble();
    pocket->m_pocket_params.m_starting_place = (m_cmbStartingPlace->GetValue().CmpNoCase(_("Center")) == 0) ? 1 : 0;
    pocket->m_pocket_params.m_cut_mode = (m_cmbCutMode->GetValue().CmpNoCase(_("climb")) == 0) ? CPocketParams::eClimb : CPocketParams::eConventional;

    wxString value = m_cmbPostProcessor->GetValue();
    if ( value.Cmp(_("ZigZag")) == 0 ) {
        pocket->m_pocket_params.m_post_processor = CPocketParams::eZigZag;
    }
    else if ( value.Cmp(_("ZigZag Unidirectional")) == 0 ) {
        pocket->m_pocket_params.m_post_processor = CPocketParams::eZigZagUnidirectional;
    }
    else if ( value.Cmp(_("Offsets")) == 0 ) {
        pocket->m_pocket_params.m_post_processor = CPocketParams::eOffsets;
    }
    else if ( value.Cmp(_("Trochoidal")) == 0 ) {
        pocket->m_pocket_params.m_post_processor = CPocketParams::eTrochoidal;
    }

	if(pocket->m_pocket_params.IsZigZag()) {
	    pocket->m_pocket_params.m_zig_angle = m_dblZigAngle->GetValueAsDouble();
	}

	SketchOpDlg::GetDataRaw(object);
}

void PocketDlg::SetFromDataRaw(HeeksObj* object)
{
    CPocket* pocket = (CPocket*)object;
	m_lgthStepOver->SetValueFromDouble(pocket->m_pocket_params.m_step_over);
	m_lgthMaterialAllowance->SetValueFromDouble(pocket->m_pocket_params.m_material_allowance);
	m_cmbStartingPlace->SetValue((pocket->m_pocket_params.m_starting_place == 1) ? _("Center") : _("Boundary"));
	m_cmbCutMode->SetValue((pocket->m_pocket_params.m_cut_mode == CPocketParams::eClimb) ? _("Climb") : _("Conventional"));

    switch ( pocket->m_pocket_params.m_post_processor ) {
    case CPocketParams::eZigZag:
        m_cmbPostProcessor->SetValue(_("ZigZag"));
        break;
    case CPocketParams::eZigZagUnidirectional:
        m_cmbPostProcessor->SetValue(_("ZigZag Unidirectional"));
        break;
    case CPocketParams::eOffsets:
        m_cmbPostProcessor->SetValue(_("Offsets"));
        break;
    case CPocketParams::eTrochoidal:
        m_cmbPostProcessor->SetValue(_("Trochoidal"));
        break;
    }

    m_dblZigAngle->SetValueFromDouble(pocket->m_pocket_params.m_zig_angle);
    if ( pocket->m_pocket_params.IsZigZag() ) {
        EnableZigZagControls();
    }

	SketchOpDlg::SetFromDataRaw(object);
}

void PocketDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(theApp.GetDialogBitmapPath(name, _T("pocket")));
}

void PocketDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_lgthStepOver)
    {
	    SetPicture(_T("step over"));
    }
	else if(w == m_lgthMaterialAllowance)
    {
	    SetPicture(_T("material allowance"));
    }
	else if(w == m_cmbStartingPlace)
	{
		if(m_cmbStartingPlace->GetValue() == _("Boundary"))SetPicture(_T("starting boundary"));
		else SetPicture(_T("starting center"));
	}
	else if(w == m_cmbCutMode)
	{
		if(m_cmbCutMode->GetValue() == _("Climb"))SetPicture(_T("climb milling"));
		else SetPicture(_T("conventional milling"));
	}
	else if(w == m_cmbPostProcessor)
	{
	    wxString value = m_cmbPostProcessor->GetValue();
	    if ( value.Cmp(_("ZigZag")) == 0 ) {
	        SetPicture(_T("use zig zag"));
	    }
	    else if ( value.Cmp(_("ZigZag Unidirectional")) == 0 ) {
	        SetPicture(_T("zig unidirectional"));
	    }
        else if ( value.Cmp(_("Offsets")) == 0 ) {
            SetPicture(_T("general"));
        }
        else if ( value.Cmp(_("Trochoidal")) == 0 ) {
            SetPicture(_T("general"));
        }
	}
	else if(w == m_dblZigAngle)
	{
	    SetPicture(_T("zig angle"));
	}
	else SketchOpDlg::SetPictureByWindow(w);
}

void PocketDlg::OnPostProcessor(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
	EnableZigZagControls();
	HeeksObjDlg::SetPicture();
}

void PocketDlg::EnableZigZagControls()
{
    wxString value = m_cmbPostProcessor->GetValue();
	bool enable = ( value.Cmp(_("ZigZag")) == 0 || value.Cmp(_("ZigZag Unidirectional")) == 0 );

	m_dblZigAngle->Enable(enable);
}

void PocketDlg::OnHelp( wxCommandEvent& event )
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/pocket"));
}

bool PocketDlg::Do(CPocket* object)
{
	PocketDlg dlg(heeksCAD->GetMainFrame(), object);

	while(1)
	{
		int result = dlg.ShowModal();

		if(result == wxID_OK)
		{
			dlg.GetData(object);
			return true;
		}
		else if(result == ID_SKETCH_PICK)
		{
			dlg.PickSketch();
		}
		else
		{
			return false;
		}
	}
}
