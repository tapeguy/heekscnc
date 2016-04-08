// HeeksCNC.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <errno.h>

#ifndef WIN32
	#include <dirent.h>
#endif

#include <wx/stdpaths.h>
#include <wx/dynlib.h>
#include <wx/aui/aui.h>
#include "interface/Observer.h"
#include "interface/ToolImage.h"
#include "PythonStuff.h"
#include "Program.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "CNCConfig.h"
#include "NCCode.h"
#include "Profile.h"
#include "Pocket.h"
#include "Drilling.h"
#include "CTool.h"
#include "Operations.h"
#include "Tools.h"
#include "interface/strconv.h"
#include "CNCPoint.h"
#include "Excellon.h"
#include "Tags.h"
#include "Tag.h"
#include "ScriptOp.h"
#include "Simulate.h"
#include "Pattern.h"
#include "Patterns.h"
#include "Surface.h"
#include "Surfaces.h"
#include "Stock.h"
#include "Stocks.h"

#include <sstream>

CHeeksCADInterface* heeksCAD = NULL;

CHeeksCNCApp theApp;

extern void ImportToolsFile( const wxChar *file_path );

wxString HeeksCNCType(const int type);

CHeeksCNCApp::CHeeksCNCApp()
{
	m_draw_cutter_radius = true;
	m_program = NULL;
	m_run_program_on_new_line = false;
	m_machiningBar = NULL;
	m_icon_texture_number = 0;
	m_machining_hidden = false;
	m_settings_restored = false;
}

CHeeksCNCApp::~CHeeksCNCApp(){
}

void CHeeksCNCApp::OnInitDLL()
{
}

void CHeeksCNCApp::OnDestroyDLL()
{
	// save any settings
	//config.Write("SolidSimWorkingDir", m_working_dir_for_solid_sim);

#if !defined WXUSINGDLL
	wxUninitialize();
#endif

	heeksCAD = NULL;
}

void OnMachiningBar( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(theApp.m_machiningBar);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

void OnUpdateMachiningBar( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	event.Check(aui_manager->GetPane(theApp.m_machiningBar).IsShown());
}

void OnProgramCanvas( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(theApp.m_program_canvas);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

void OnUpdateProgramCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	event.Check(aui_manager->GetPane(theApp.m_program_canvas).IsShown());
}

static void OnOutputCanvas( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(theApp.m_output_canvas);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

static void OnPrintCanvas( wxCommandEvent& event )
{
    wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
    wxAuiPaneInfo& pane_info = aui_manager->GetPane(theApp.m_print_canvas);
    if(pane_info.IsOk()){
        pane_info.Show(event.IsChecked());
        aui_manager->Update();
    }
}

static void OnUpdateOutputCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	event.Check(aui_manager->GetPane(theApp.m_output_canvas).IsShown());
}

static void OnUpdatePrintCanvas( wxUpdateUIEvent& event )
{
    wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
    event.Check(aui_manager->GetPane(theApp.m_print_canvas).IsShown());
}

static void GetSketches(std::list<int>& sketches, std::list<int> &tools )
{
    // check for at least one sketch selected

    const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList ( );
    for ( std::list<HeeksObj*>::const_iterator It = list.begin ( ); It != list.end ( ); It++ )
    {
        HeeksObj* object = *It;
        if ( object->GetIDGroupType ( ) == SketchType )
        {
            sketches.push_back ( object->GetID() );
        } // End if - then

        if ( ( object != NULL ) && ( object->GetType ( ) == ToolType ) )
        {
            tools.push_back ( object->GetID() );
        } // End if - then
    }
}

static void NewProfileOp()
{
    std::list<int> tools;
    std::list<int> sketches;
    GetSketches(sketches, tools);

    int sketch = 0;
    if(sketches.size() > 0)sketch = sketches.front();

    CProfile *new_object = new CProfile(sketch, (tools.size()>0)?(*tools.begin()):-1);
    new_object->SetID(heeksCAD->GetNextID(ProfileType));
    new_object->AddMissingChildren(); // add the tags container

    if(new_object->Edit())
    {
        heeksCAD->StartHistory();
        heeksCAD->AddUndoably(new_object, theApp.m_program->Operations());

        if(sketches.size() > 1)
        {
            for(std::list<int>::iterator It = sketches.begin(); It != sketches.end(); It++)
            {
                if(It == sketches.begin())continue;
                CProfile* copy = (CProfile*)(new_object->MakeACopy());
                copy->m_sketch = *It;
                heeksCAD->AddUndoably(copy, theApp.m_program->Operations());
            }
        }
        heeksCAD->EndHistory();
    }
    else
        delete new_object;
}

static void NewProfileOpMenuCallback(wxCommandEvent &event)
{
    NewProfileOp();
}

static void NewPocketOp()
{
    std::list<int> tools;
    std::list<int> sketches;
    GetSketches(sketches, tools);

    int sketch = 0;
    if(sketches.size() > 0)sketch = sketches.front();

    CPocket *new_object = new CPocket(sketch, (tools.size()>0)?(*tools.begin()):-1 );
    new_object->SetID(heeksCAD->GetNextID(PocketType));

    if(new_object->Edit())
    {
        heeksCAD->StartHistory();
        heeksCAD->AddUndoably(new_object, theApp.m_program->Operations());

        if(sketches.size() > 1)
        {
            for(std::list<int>::iterator It = sketches.begin(); It != sketches.end(); It++)
            {
                if(It == sketches.begin())continue;
                CPocket* copy = (CPocket*)(new_object->MakeACopy());
                copy->m_sketch = *It;
                heeksCAD->AddUndoably(copy, theApp.m_program->Operations());
            }
        }
        heeksCAD->EndHistory();
    }
    else
        delete new_object;
}

static void NewPocketOpMenuCallback(wxCommandEvent &event)
{
    NewPocketOp();
}

static void AddNewObjectUndoablyAndMarkIt(HeeksObj* new_object, HeeksObj* parent)
{
    heeksCAD->StartHistory();
    heeksCAD->AddUndoably(new_object, parent);
    heeksCAD->ClearMarkedList();
    heeksCAD->Mark(new_object);
    heeksCAD->EndHistory();
}

static void NewDrillingOp ( )
{
    std::list<int> points;

    const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList ( );
    for ( std::list<HeeksObj*>::const_iterator It = list.begin ( ); It != list.end ( ); It++ )
    {
        HeeksObj* object = *It;
        if ( object->GetType ( ) == PointType )
        {
            points.push_back ( object->GetID() );
        } // End if - else
    } // End for

    {
        CDrilling *new_object = new CDrilling ( points, 0, -1 );
        new_object->SetID ( heeksCAD->GetNextID ( DrillingType ) );
        if ( new_object->Edit ( ) )
        {
            heeksCAD->StartHistory ( );
            heeksCAD->AddUndoably ( new_object, theApp.m_program->Operations ( ) );
            heeksCAD->EndHistory ( );
        }
        else
            delete new_object;
    }
}

static void NewDrillingOpMenuCallback(wxCommandEvent &event)
{
    NewDrillingOp();
}

static void NewScriptOpMenuCallback(wxCommandEvent &event)
{
    CScriptOp *new_object = new CScriptOp();
    new_object->SetID(heeksCAD->GetNextID(ScriptOpType));
    if(new_object->Edit())
    {
        heeksCAD->StartHistory();
        AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Operations());
        heeksCAD->EndHistory();
    }
    else
        delete new_object;
}

static void NewPatternMenuCallback(wxCommandEvent &event)
{
    CPattern *new_object = new CPattern();

    if(new_object->Edit())
    {
        heeksCAD->StartHistory();
        AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Patterns());
        heeksCAD->EndHistory();
    }
    else
        delete new_object;
}

static void NewSurfaceMenuCallback(wxCommandEvent &event)
{
    // check for at least one solid selected
    std::list<int> solids;

    const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
    for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
    {
        HeeksObj* object = *It;
        if(object->GetIDGroupType() == SolidType)solids.push_back(object->GetID());
    }

    // if no selected solids,
    if(solids.size() == 0)
    {
        // use all the solids in the drawing
        for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
        {
            if(object->GetIDGroupType() == SolidType)solids.push_back(object->GetID());
        }
    }

    {
        CSurface *new_object = new CSurface();
        new_object->m_solids = solids;
        if(new_object->Edit())
        {
            heeksCAD->StartHistory();
            AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Surfaces());
            heeksCAD->EndHistory();
        }
        else
            delete new_object;
    }
}

static void NewStockMenuCallback(wxCommandEvent &event)
{
    // check for at least one solid selected
    std::list<int> solids;

    const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
    for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
    {
        HeeksObj* object = *It;
        if(object->GetIDGroupType() == SolidType)solids.push_back(object->GetID());
    }

    // if no selected solids,
    if(solids.size() == 0)
    {
        // use all the solids in the drawing
        for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
        {
            if(object->GetIDGroupType() == SolidType)solids.push_back(object->GetID());
        }
    }

    {
        CStock *new_object = new CStock();
        new_object->m_solids = solids;
        if(new_object->Edit())
        {
            heeksCAD->StartHistory();
            AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Stocks());
            heeksCAD->EndHistory();
        }
        else
            delete new_object;
    }
}

static void AddNewTool(CToolParams::eToolType type)
{
    // find next available tool number
    int max_tool_number = 0;
    for(HeeksObj* object = theApp.m_program->Tools()->GetFirstChild(); object; object = theApp.m_program->Tools()->GetNextChild())
    {
        if(object->GetType() == ToolType)
        {
            int tool_number = ((CTool*)object)->m_tool_number;
            if(tool_number > max_tool_number)max_tool_number = tool_number;
        }
    }

    // Add a new tool.
    CTool *new_object = new CTool(NULL, type, max_tool_number + 1);
    if(new_object->Edit())
        AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Tools());
    else
        delete new_object;
}

static void NewDrillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eDrill);
}

static void NewCentreDrillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eCentreDrill);
}

static void NewEndmillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eEndmill);
}

static void NewSlotCutterMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eSlotCutter);
}

static void NewBallEndMillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eBallEndMill);
}

static void NewChamferMenuCallback(wxCommandEvent &event)
{
    AddNewTool(CToolParams::eChamfer);
}

static void MakeScriptMenuCallback(wxCommandEvent &event)
{
	// create the Python program
	theApp.m_program->RewritePythonProgram();
}

void CHeeksCNCApp::RunPythonScript()
{
	{
		// clear the output file
		wxFile f(m_program->GetOutputFileName().c_str(), wxFile::write);
		if(f.IsOpened())f.Write(_T("\n"));
	}

	// Check to see if someone has modified the contents of the
	// program canvas manually.  If so, replace the m_python_program
	// with the edited program.  We don't want to do this without
	// this check since the maximum size of m_textCtrl is sometimes
	// a limitation to the size of the python program.  If the first 'n' characters
	// of m_python_program matches the full contents of the m_textCtrl then
	// it's likely that the text control holds as much of the python program
	// as it can hold but more may still exist in m_python_program.
	unsigned int text_control_length = m_program_canvas->m_textCtrl->GetLastPosition();
	if (m_program->m_python_program.substr(0,text_control_length) != m_program_canvas->m_textCtrl->GetValue())
	{
        // copy the contents of the program canvas to the string
        m_program->m_python_program.clear();
        m_program->m_python_program << theApp.m_program_canvas->m_textCtrl->GetValue();
	}

	HeeksPyPostProcess(m_program, m_program->GetOutputFileName(), true );
}

static void RunScriptMenuCallback(wxCommandEvent &event)
{
	theApp.RunPythonScript();
}

static void PostProcessMenuCallback(wxCommandEvent &event)
{
	// write the python program
	theApp.m_program->RewritePythonProgram();

	// run it
	theApp.RunPythonScript();
}

static void CancelMenuCallback(wxCommandEvent &event)
{
	HeeksPyCancel();
}

static void SimulateCallback(wxCommandEvent &event)
{
    RunVoxelcutSimulation();
}

static void OpenNcFileMenuCallback(wxCommandEvent& event)
{
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
    wxFileDialog dialog(theApp.m_output_canvas, _("Open NC file"), wxEmptyString, wxEmptyString, wildcard_string);
    dialog.CentreOnParent();

    if (dialog.ShowModal() == wxID_OK)
    {
		HeeksPyBackplot(theApp.m_program, theApp.m_program, dialog.GetPath().c_str());
	}
}

// create a temporary file for ngc output
// run appropriate command to make 'Machine' read ngc file
// linux/emc/axis: this would typically entail calling axis-remote <filename>

static void SendToMachineMenuCallback(wxCommandEvent& event)
{
	HeeksSendToMachine(theApp.m_output_canvas->m_textCtrl->GetValue());
}

static void SaveNcFileMenuCallback(wxCommandEvent& event)
{
    wxStandardPaths& sp = wxStandardPaths::Get();
    wxString user_docs = sp.GetDocumentsDir();
    wxString ncdir;
    //ncdir =  user_docs + _T("/nc");
    ncdir =  user_docs; //I was getting tired of having to start out at the root directory in linux
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
    wxString defaultDir = ncdir;
	wxFileDialog fd(theApp.m_output_canvas, _("Save NC file"), defaultDir, wxEmptyString, wildcard_string, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
	fd.SetFilterIndex(1);
	if (fd.ShowModal() == wxID_OK)
	{
		wxString nc_file_str = fd.GetPath().c_str();
		{
			wxFile ofs(nc_file_str.c_str(), wxFile::write);
			if(!ofs.IsOpened())
			{
				wxMessageBox(wxString(_("Couldn't open file")) + _T(" - ") + nc_file_str);
				return;
			}


          if(theApp.m_use_DOS_not_Unix == true)   //DF -added to get DOS line endings HeeksCNC running on Unix
            {
                long line_num= 0;
                bool ok = true;
                int nLines = theApp.m_output_canvas->m_textCtrl->GetNumberOfLines();
            for ( int nLine = 0; ok && nLine < nLines; nLine ++)
                {
                    ok = ofs.Write(theApp.m_output_canvas->m_textCtrl->GetLineText(line_num) + _T("\r\n") );
                    line_num = line_num+1;
                }
            }

            else
			    ofs.Write(theApp.m_output_canvas->m_textCtrl->GetValue());
		}
		HeeksPyBackplot(theApp.m_program, theApp.m_program, nc_file_str);
	}
}

static void HelpMenuCallback(wxCommandEvent& event)
{
    ::wxLaunchDefaultBrowser(_T("http://heeks.net/help"));
}

// a class to re-use existing "OnButton" functions in a Tool class
class CCallbackTool: public Tool{
public:
	wxString m_title;
	wxString m_bitmap_path;
	void(*m_callback)(wxCommandEvent&);

	CCallbackTool(const wxString& title, const wxString& bitmap_path, void(*callback)(wxCommandEvent&)): m_title(title), m_bitmap_path(bitmap_path), m_callback(callback){}

	// Tool's virtual functions
	const wxChar* GetTitle(){return m_title;}
	void Run(){
		wxCommandEvent dummy_evt;
		(*m_callback)(dummy_evt);
	}
	wxString BitmapPath(){ return theApp.GetBitmapPath(m_bitmap_path); }
};

static CCallbackTool new_drill_tool(_("New Drill..."), _T("drill"), NewDrillMenuCallback);
static CCallbackTool new_centre_drill_tool(_("New Centre Drill..."), _T("centredrill"), NewCentreDrillMenuCallback);
static CCallbackTool new_endmill_tool(_("New End Mill..."), _T("endmill"), NewEndmillMenuCallback);
static CCallbackTool new_slotdrill_tool(_("New Slot Drill..."), _T("slotdrill"), NewSlotCutterMenuCallback);
static CCallbackTool new_ball_end_mill_tool(_("New Ball End Mill..."), _T("ballmill"), NewBallEndMillMenuCallback);
static CCallbackTool new_chamfer_mill_tool(_("New Chamfer Mill..."), _T("chamfmill"), NewChamferMenuCallback);

void CHeeksCNCApp::GetNewToolTools(std::list<Tool*>* t_list)
{
        t_list->push_back(&new_drill_tool);
        t_list->push_back(&new_centre_drill_tool);
        t_list->push_back(&new_endmill_tool);
        t_list->push_back(&new_slotdrill_tool);
        t_list->push_back(&new_ball_end_mill_tool);
        t_list->push_back(&new_chamfer_mill_tool);
}

static CCallbackTool new_pattern_tool(_("New Pattern..."), _T("pattern"), NewPatternMenuCallback);

void CHeeksCNCApp::GetNewPatternTools(std::list<Tool*>* t_list)
{
        t_list->push_back(&new_pattern_tool);
}

static CCallbackTool new_surface_tool(_("New Surface..."), _T("surface"), NewSurfaceMenuCallback);

void CHeeksCNCApp::GetNewSurfaceTools(std::list<Tool*>* t_list)
{
        t_list->push_back(&new_surface_tool);
}

static CCallbackTool new_stock_tool(_("New Stock..."), _T("stock"), NewStockMenuCallback);

void CHeeksCNCApp::GetNewStockTools(std::list<Tool*>* t_list)
{
        t_list->push_back(&new_stock_tool);
}

#define MAX_XML_SCRIPT_OPS 10

std::vector< CXmlScriptOp > script_ops;
int script_op_flyout_index = 0;

static void NewXmlScriptOp(int i)
{
    CScriptOp *new_object = new CScriptOp();
    new_object->m_title_made_from_id = false;
    new_object->SetTitle ( script_ops[i].m_name );
    new_object->m_str = script_ops[i].m_script;
    new_object->m_user_icon = true;
    new_object->m_user_icon_name = script_ops[i].m_icon;

    if(new_object->Edit())
    {
        heeksCAD->StartHistory();
        AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Operations());
        heeksCAD->EndHistory();
    }
    else
        delete new_object;
}

static void NewXmlScriptOpCallback0(wxCommandEvent &event)
{
    NewXmlScriptOp(0);
}

static void NewXmlScriptOpCallback1(wxCommandEvent &event)
{
    NewXmlScriptOp(1);
}

static void NewXmlScriptOpCallback2(wxCommandEvent &event)
{
    NewXmlScriptOp(2);
}

static void NewXmlScriptOpCallback3(wxCommandEvent &event)
{
    NewXmlScriptOp(3);
}

static void NewXmlScriptOpCallback4(wxCommandEvent &event)
{
    NewXmlScriptOp(4);
}

static void NewXmlScriptOpCallback5(wxCommandEvent &event)
{
    NewXmlScriptOp(5);
}

static void NewXmlScriptOpCallback6(wxCommandEvent &event)
{
    NewXmlScriptOp(6);
}

static void NewXmlScriptOpCallback7(wxCommandEvent &event)
{
    NewXmlScriptOp(7);
}

static void NewXmlScriptOpCallback8(wxCommandEvent &event)
{
    NewXmlScriptOp(8);
}

static void NewXmlScriptOpCallback9(wxCommandEvent &event)
{
    NewXmlScriptOp(9);
}

static void AddXmlScriptOpMenuItems(wxMenu *menu = NULL)
{
    script_ops.clear();
    CProgram::GetScriptOps(script_ops);

    int i = 0;
    for(std::vector< CXmlScriptOp >::iterator It = script_ops.begin(); It != script_ops.end(); It++, i++)
    {
        CXmlScriptOp &s = *It;
        if(i >= MAX_XML_SCRIPT_OPS)break;
        void(*onButtonFunction)(wxCommandEvent&) = NULL;
        switch(i)
        {
        case 0:
            onButtonFunction = NewXmlScriptOpCallback0;
            break;
        case 1:
            onButtonFunction = NewXmlScriptOpCallback1;
            break;
        case 2:
            onButtonFunction = NewXmlScriptOpCallback2;
            break;
        case 3:
            onButtonFunction = NewXmlScriptOpCallback3;
            break;
        case 4:
            onButtonFunction = NewXmlScriptOpCallback4;
            break;
        case 5:
            onButtonFunction = NewXmlScriptOpCallback5;
            break;
        case 6:
            onButtonFunction = NewXmlScriptOpCallback6;
            break;
        case 7:
            onButtonFunction = NewXmlScriptOpCallback7;
            break;
        case 8:
            onButtonFunction = NewXmlScriptOpCallback8;
            break;
        case 9:
            onButtonFunction = NewXmlScriptOpCallback9;
            break;
        }

        if(menu)
            heeksCAD->AddMenuItem(menu, s.m_name, ToolImage(s.m_bitmap), onButtonFunction);
        else
            heeksCAD->AddFlyoutButton(s.m_name, ToolImage(s.m_bitmap), s.m_name, onButtonFunction);
    }
}

static void AddToolBars()
{
	if(!theApp.m_machining_hidden)
	{
		wxFrame* frame = heeksCAD->GetMainFrame();
		wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
		if(theApp.m_machiningBar) {
			aui_manager->DetachPane(theApp.m_machiningBar);
			heeksCAD->RemoveToolBar(theApp.m_machiningBar);
			delete theApp.m_machiningBar;
		}
		theApp.m_machiningBar = new wxAuiToolBar(frame, -1, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);
		theApp.m_machiningBar->SetToolBitmapSize(wxSize(ToolImage::GetBitmapSize(), ToolImage::GetBitmapSize()));

		heeksCAD->StartToolBarFlyout(_("Milling operations"));
		heeksCAD->AddFlyoutButton(_("Profile"), ToolImage(theApp.GetBitmapPath(_T("opprofile")), true), _("New Profile Operation..."), NewProfileOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Pocket"), ToolImage(theApp.GetBitmapPath(_T("pocket")), true), _("New Pocket Operation..."), NewPocketOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Drill"), ToolImage(theApp.GetBitmapPath(_T("drilling")), true), _("New Drill Cycle Operation..."), NewDrillingOpMenuCallback);
		heeksCAD->EndToolBarFlyout(theApp.m_machiningBar);

		heeksCAD->StartToolBarFlyout(_("Other operations"));
        heeksCAD->AddFlyoutButton(_T("ScriptOp"), ToolImage(theApp.GetBitmapPath(_T("scriptop")), true), _("New Script Operation..."), NewScriptOpMenuCallback);
        heeksCAD->AddFlyoutButton(_T("Pattern"), ToolImage(theApp.GetBitmapPath(_T("pattern")), true), _("New Pattern..."), NewPatternMenuCallback);
        heeksCAD->AddFlyoutButton(_T("Surface"), ToolImage(theApp.GetBitmapPath(_T("surface")), true), _("New Surface..."), NewSurfaceMenuCallback);
        heeksCAD->AddFlyoutButton(_T("Stock"), ToolImage(theApp.GetBitmapPath(_T("stock")), true), _("New Stock..."), NewStockMenuCallback);
        AddXmlScriptOpMenuItems();
        heeksCAD->EndToolBarFlyout(theApp.m_machiningBar);

        heeksCAD->StartToolBarFlyout(_("Tools"));
        heeksCAD->AddFlyoutButton(_T("drill"), ToolImage(theApp.GetBitmapPath(_T("drill")), true), _("Drill..."), NewDrillMenuCallback);
        heeksCAD->AddFlyoutButton(_T("centredrill"), ToolImage(theApp.GetBitmapPath(_T("centredrill")), true), _("Centre Drill..."), NewCentreDrillMenuCallback);
        heeksCAD->AddFlyoutButton(_T("endmill"), ToolImage(theApp.GetBitmapPath(_T("endmill")), true), _("End Mill..."), NewEndmillMenuCallback);
        heeksCAD->AddFlyoutButton(_T("slotdrill"), ToolImage(theApp.GetBitmapPath(_T("slotdrill")), true), _("Slot Drill..."), NewSlotCutterMenuCallback);
        heeksCAD->AddFlyoutButton(_T("ballmill"), ToolImage(theApp.GetBitmapPath(_T("ballmill")), true), _("Ball End Mill..."), NewBallEndMillMenuCallback);
        heeksCAD->AddFlyoutButton(_T("chamfmill"), ToolImage(theApp.GetBitmapPath(_T("chamfmill")), true), _("Chamfer Mill..."), NewChamferMenuCallback);
        heeksCAD->EndToolBarFlyout(theApp.m_machiningBar);

		heeksCAD->StartToolBarFlyout(_("Post Processing"));
		heeksCAD->AddFlyoutButton(_("PostProcess"), ToolImage(theApp.GetBitmapPath(_T("postprocess")), true), _("Post-Process"), PostProcessMenuCallback);
		heeksCAD->AddFlyoutButton(_("Make Python Script"), ToolImage(theApp.GetBitmapPath(_T("python")), true), _("Make Python Script"), MakeScriptMenuCallback);
		heeksCAD->AddFlyoutButton(_("Run Python Script"), ToolImage(theApp.GetBitmapPath(_T("runpython")), true), _("Run Python Script"), RunScriptMenuCallback);
		heeksCAD->AddFlyoutButton(_("OpenNC"), ToolImage(theApp.GetBitmapPath(_T("opennc")), true), _("Open NC File"), OpenNcFileMenuCallback);
		heeksCAD->AddFlyoutButton(_("SaveNC"), ToolImage(theApp.GetBitmapPath(_T("savenc")), true), _("Save NC File"), SaveNcFileMenuCallback);
		heeksCAD->AddFlyoutButton(_("Send to Machine"), ToolImage(theApp.GetBitmapPath(_T("tomachine")), true), _("Send to Machine"), SendToMachineMenuCallback);
		heeksCAD->AddFlyoutButton(_("Cancel"), ToolImage(theApp.GetBitmapPath(_T("cancel")), true), _("Cancel Python Script"), CancelMenuCallback);
        heeksCAD->AddFlyoutButton(_T("Simulate"), ToolImage(theApp.GetBitmapPath(_T("simulate")), true), _("Simulate"), SimulateCallback);
		heeksCAD->EndToolBarFlyout(theApp.m_machiningBar);

		theApp.m_machiningBar->Realize();
		aui_manager->AddPane(theApp.m_machiningBar, wxAuiPaneInfo().Name(_T("MachiningBar")).Caption(_T("Machining Tools")).ToolbarPane().Top());
		heeksCAD->RegisterToolBar(theApp.m_machiningBar);
	}
}

void OnBuildTexture()
{
	wxString filepath = theApp.GetResFolder() + _T("/icons/iconimage.png");
	theApp.m_icon_texture_number = heeksCAD->LoadIconsTexture(filepath.c_str());
}

static void ImportExcellonDrillFile( const wxChar *file_path )
{
    Excellon drill;

    wxString message(_("Select how the file is to be interpreted"));
    wxString caption(_("Excellon drill file interpretation"));

    wxArrayString choices;

    choices.Add(_("Produce drill pattern as described"));
    choices.Add(_("Produce mirrored drill pattern"));

    wxString choice = ::wxGetSingleChoice( message, caption, choices );

    if (choice == choices[0])
    {
        drill.Read( Ttc(file_path), false );
    }

    if (choice == choices[1])
    {
        drill.Read( Ttc(file_path), true );
    }
}

static void UnitsChangedHandler( const EnumUnitType units )
{
    // The view units have changed.  See if the user wants the NC output units to change
    // as well.

    if (units != theApp.m_program->m_units)
    {
        int response;
        response = wxMessageBox( _("Would you like to change the NC code generation units too?"), _("Change Units"), wxYES_NO );
        if (response == wxYES)
        {
            theApp.m_program->m_units = units;
        }
    }
}

class SketchBox{
public:
    CBox m_box;
    gp_Vec m_latest_shift;

    SketchBox(const CBox &box);

    SketchBox(const SketchBox &s)
    {
        m_box = s.m_box;
        m_latest_shift = s.m_latest_shift;
    }

    void UpdateBoxAndSetShift(const CBox &new_box)
    {
        // use Centre
        double old_centre[3], new_centre[3];
        m_box.Centre(old_centre);
        new_box.Centre(new_centre);
        m_latest_shift = gp_Vec(new_centre[0] - old_centre[0], new_centre[1] - old_centre[1], 0.0);
        m_box = new_box;
    }
};

SketchBox::SketchBox(const CBox &box)
{
    m_box = box;
    m_latest_shift = gp_Vec(0, 0, 0);
}

class HeeksCADObserver: public Observer
{
public:
    std::map<int, SketchBox> m_box_map;

    void OnChanged(const std::list<HeeksObj*>* added, const std::list<HeeksObj*>* removed, const std::list<HeeksObj*>* modified)
    {
        if(added)
        {
            for(std::list<HeeksObj*>::const_iterator It = added->begin(); It != added->end(); It++)
            {
                HeeksObj* object = *It;
                if(object->GetType() == SketchType)
                {
                    CBox box;
                    object->GetBox(box);
                    m_box_map.insert(std::make_pair(object->GetID(), SketchBox(box)));
                }
            }
        }

        if(modified)
        {
            for(std::list<HeeksObj*>::const_iterator It = modified->begin(); It != modified->end(); It++)
            {
                HeeksObj* object = *It;
                if(object->GetType() == SketchType)
                {
                    CBox new_box;
                    object->GetBox(new_box);
                    std::map<int, SketchBox>::iterator FindIt = m_box_map.find(object->GetID());
                    if(FindIt != m_box_map.end())
                    {
                        SketchBox &sketch_box = FindIt->second;
                        sketch_box.UpdateBoxAndSetShift(new_box);
                    }
                }
            }

            // check all the profile operations, so we can move the tags
            for(HeeksObj* object = theApp.m_program->Operations()->GetFirstChild(); object; object = theApp.m_program->Operations()->GetNextChild())
            {
                if(object->GetType() == ProfileType)
                {
                    CProfile* profile = (CProfile*)object;
                    std::map<int, SketchBox>::iterator FindIt = m_box_map.find(object->GetID());
                    if (FindIt != m_box_map.end())
                    {
                        SketchBox &sketch_box = FindIt->second;
                        for (HeeksObj* tagObj = profile->Tags()->GetFirstChild(); tagObj; tagObj = profile->Tags()->GetNextChild())
                        {
                            CTag * tag = (CTag*)tagObj;
                            tag->m_pos.SetX( tag->m_pos.X() + sketch_box.m_latest_shift.X() );
                            tag->m_pos.SetY( tag->m_pos.Y() + sketch_box.m_latest_shift.Y() );
                        }

                        profile->m_profile_params.m_start = gp_Pnt (
                            profile->m_profile_params.m_start.X() + sketch_box.m_latest_shift.X(),
                            profile->m_profile_params.m_start.Y() + sketch_box.m_latest_shift.Y(),
                            profile->m_profile_params.m_start.Z() + sketch_box.m_latest_shift.Z());

                        profile->m_profile_params.m_end = gp_Pnt (
                            profile->m_profile_params.m_end.X() + sketch_box.m_latest_shift.X(),
                            profile->m_profile_params.m_end.Y() + sketch_box.m_latest_shift.Y(),
                            profile->m_profile_params.m_end.Z() + sketch_box.m_latest_shift.Z());

                        profile->m_profile_params.m_roll_on_point = gp_Pnt (
                            profile->m_profile_params.m_roll_on_point.X() + sketch_box.m_latest_shift.X(),
                            profile->m_profile_params.m_roll_on_point.Y() + sketch_box.m_latest_shift.Y(),
                            profile->m_profile_params.m_roll_on_point.Z() + sketch_box.m_latest_shift.Z());

                        profile->m_profile_params.m_roll_off_point = gp_Pnt (
                            profile->m_profile_params.m_roll_off_point.X() + sketch_box.m_latest_shift.X(),
                            profile->m_profile_params.m_roll_off_point.Y() + sketch_box.m_latest_shift.Y(),
                            profile->m_profile_params.m_roll_off_point.Z() + sketch_box.m_latest_shift.Z());
                    }
                }
            }
            for(std::map<int, SketchBox>::iterator It = m_box_map.begin(); It != m_box_map.end(); It++)
            {
                SketchBox &sketch_box = It->second;
                sketch_box.m_latest_shift = gp_Vec(0, 0, 0);
            }
        }
    }

    void Clear()
    {
        m_box_map.clear();
    }
}heekscad_observer;

class NewProfileOpTool:public Tool
{
    // Tool's virtual functions
    const wxChar* GetTitle(){return _("New Profile Operation");}
    void Run(){
        NewProfileOp();
    }
    wxString BitmapPath(){ return theApp.GetResFolder() + _T("/bitmaps/opprofile.png"); }
};

class NewPocketOpTool:public Tool
{
    // Tool's virtual functions
    const wxChar* GetTitle(){return _("New Pocket Operation");}
    void Run(){
        NewPocketOp();
    }
    wxString BitmapPath(){ return theApp.GetResFolder() + _T("/bitmaps/pocket.png"); }
};

class NewDrillingOpTool:public Tool
{
    // Tool's virtual functions
    const wxChar* GetTitle(){return _("New Drilling Operation");}
    void Run(){
        NewDrillingOp();
    }
    wxString BitmapPath(){ return theApp.GetResFolder() + _T("/bitmaps/drilling.png"); }
};

static void GetMarkedListTools(std::list<Tool*>& t_list)
{
    std::set<int> group_types;

    const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
    for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
    {
        HeeksObj* object = *It;
        group_types.insert(object->GetIDGroupType());
    }

    for(std::set<int>::iterator It = group_types.begin(); It != group_types.end(); It++)
    {
        switch(*It)
        {
        case SketchType:
            t_list.push_back(new NewProfileOpTool);
            t_list.push_back(new NewPocketOpTool);
            break;
        case PointType:
            t_list.push_back(new NewDrillingOpTool);
            break;
        }
    }
}

static void OnRestoreDefaults()
{
    CNCConfig config;
    config.DeleteAll();
    theApp.m_settings_restored = true;
}

void CHeeksCNCApp::OnStartUp(CHeeksCADInterface* h, const wxString& dll_path)
{
    InitializeProperties();

	m_dll_path = dll_path;
	heeksCAD = h;
#if !defined WXUSINGDLL
	wxInitialize();
#endif

    // to do, use os_id
    wxOperatingSystemId os_id = wxGetOsVersion();

	CNCConfig config(ConfigScope());

	// About box, stuff
	heeksCAD->AddToAboutBox(wxString(_T("\n\n")) + _("HeeksCNC is the free machining add-on to HeeksCAD")
		+ _T("\n") + _T("          http://code.google.com/p/heekscnc/")
		+ _T("\n") + _("Written by Dan Heeks, Hirutso Enni, Perttu Ahola, David Nicholls")
		+ _T("\n") + _("With help from archivist, crotchet1, DanielFalck, fenn, Sliptonic")
		+ _T("\n\n") + _("geometry code, donated by Geoff Hawkesford, Camtek GmbH http://www.peps.de/")
		+ _T("\n") + _("pocketing code from http://code.google.com/p/libarea/ , derived from the kbool library written by Klaas Holwerda http://boolean.klaasholwerda.nl/bool.html")
		+ _T("\n") + _("Zig zag code from opencamlib http://code.google.com/p/opencamlib/")
		+ _T("\n\n") + _("This HeeksCNC software installation is restricted by the GPL license http://www.gnu.org/licenses/gpl-3.0.txt")
		+ _T("\n") + _("  which means it is free and open source, and must stay that way")
		);

	// add menus and toolbars
	wxFrame* frame = heeksCAD->GetMainFrame();
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();

	// tool bars
	heeksCAD->RegisterAddToolBars(AddToolBars);
	AddToolBars();

    // Help menu
    wxMenu *menuHelp = heeksCAD->GetHelpMenu();
    menuHelp->AppendSeparator();
    heeksCAD->AddMenuItem(menuHelp, _("Online HeeksCNC Manual"), ToolImage(theApp.GetBitmapPath(_T("help")), true), HelpMenuCallback);

	// Milling Operations menu
	wxMenu *menuMillingOperations = new wxMenu;
	heeksCAD->AddMenuItem(menuMillingOperations, _("Profile Operation..."), ToolImage(theApp.GetBitmapPath(_T("opprofile")), true), NewProfileOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Pocket Operation..."), ToolImage(theApp.GetBitmapPath(_T("pocket")), true), NewPocketOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Drilling Operation..."), ToolImage(theApp.GetBitmapPath(_T("drilling")), true), NewDrillingOpMenuCallback);

    // Additive Operations menu
    wxMenu *menuOperations = new wxMenu;
    heeksCAD->AddMenuItem(menuOperations, _("Script Operation..."), ToolImage(theApp.GetBitmapPath(_T("scriptop")), true), NewScriptOpMenuCallback);
    heeksCAD->AddMenuItem(menuOperations, _("Pattern..."), ToolImage(theApp.GetBitmapPath(_T("pattern")), true), NewPatternMenuCallback);
    heeksCAD->AddMenuItem(menuOperations, _("Surface..."), ToolImage(theApp.GetBitmapPath(_T("surface")), true), NewSurfaceMenuCallback);
    heeksCAD->AddMenuItem(menuOperations, _("Stock..."), ToolImage(theApp.GetBitmapPath(_T("stock")), true), NewStockMenuCallback);
    AddXmlScriptOpMenuItems(menuOperations);

	// Tools menu
	wxMenu *menuTools = new wxMenu;
	heeksCAD->AddMenuItem(menuTools, _("Drill..."), ToolImage(theApp.GetBitmapPath(_T("drill")), true), NewDrillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Centre Drill..."), ToolImage(theApp.GetBitmapPath(_T("centredrill")), true), NewCentreDrillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("End Mill..."), ToolImage(theApp.GetBitmapPath(_T("endmill")), true), NewEndmillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Slot Drill..."), ToolImage(theApp.GetBitmapPath(_T("slotdrill")), true), NewSlotCutterMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Ball End Mill..."), ToolImage(theApp.GetBitmapPath(_T("ballmill")), true), NewBallEndMillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Chamfer Mill..."), ToolImage(theApp.GetBitmapPath(_T("chamfmill")), true), NewChamferMenuCallback);

	// Machining menu
	wxMenu *menuMachining = new wxMenu;
	heeksCAD->AddMenuItem(menuMachining, _("Add New Milling Operation"), ToolImage(theApp.GetBitmapPath(_T("ops")), true), NULL, NULL, menuMillingOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add Other Operation"), ToolImage(theApp.GetBitmapPath(_T("ops")), true), NULL, NULL, menuOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add New Tool"), ToolImage(theApp.GetBitmapPath(_T("tools")), true), NULL, NULL, menuTools);
	heeksCAD->AddMenuItem(menuMachining, _("Make Python Script"), ToolImage(theApp.GetBitmapPath(_T("python")), true), MakeScriptMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Run Python Script"), ToolImage(theApp.GetBitmapPath(_T("runpython")), true), RunScriptMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Post-Process"), ToolImage(theApp.GetBitmapPath(_T("postprocess")), true), PostProcessMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Simulate"), ToolImage(theApp.GetBitmapPath(_T("simulate")), true), SimulateCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Open NC File..."), ToolImage(theApp.GetBitmapPath(_T("opennc")), true), OpenNcFileMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Save NC File as..."), ToolImage(theApp.GetBitmapPath(_T("savenc")), true), SaveNcFileMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Send to Machine"), ToolImage(theApp.GetBitmapPath(_T("tomachine")), true), SendToMachineMenuCallback);
	frame->GetMenuBar()->Append(menuMachining,  _("Machining"));

	// add the program canvas
    m_program_canvas = new CProgramCanvas(frame);
	aui_manager->AddPane(m_program_canvas, wxAuiPaneInfo().Name(_T("Program")).Caption(_T("Program")).Bottom().BestSize(wxSize(600, 200)));

	// add the output canvas
    m_output_canvas = new COutputCanvas(frame);
	aui_manager->AddPane(m_output_canvas, wxAuiPaneInfo().Name(_T("Output")).Caption(_T("Output")).Bottom().BestSize(wxSize(600, 200)));

    // add the print canvas
    m_print_canvas = new CPrintCanvas(frame);
    aui_manager->AddPane(m_print_canvas, wxAuiPaneInfo().Name(_T("Print")).Caption(_T("Print")).Bottom().BestSize(wxSize(600, 200)));

	bool program_visible;
	bool output_visible;
	bool print_visible;

	config.Read(_T("ProgramVisible"), &program_visible);
	config.Read(_T("OutputVisible"), &output_visible);
	config.Read(_T("PrintVisible"), &print_visible);

	// read other settings
	CNCCode::ReadColorsFromConfig(&text_colors);
	CProfile::ReadFromConfig();
	CPocket::ReadFromConfig();
	CSpeedOp::ReadFromConfig();
	CSendToMachine::ReadFromConfig();
	config.Read(_T("UseClipperNotBoolean"), m_use_Clipper_not_Boolean, false);
	config.Read(_T("UseDOSNotUnix"), m_use_DOS_not_Unix, false);
	aui_manager->GetPane(m_program_canvas).Show(program_visible);
	aui_manager->GetPane(m_output_canvas).Show(output_visible);
	aui_manager->GetPane(m_print_canvas).Show(output_visible);

	// add tick boxes for them all on the view menu
	wxMenu* window_menu = heeksCAD->GetWindowMenu();
	heeksCAD->AddMenuItem(window_menu, _T("Program"), wxBitmap(), OnProgramCanvas, OnUpdateProgramCanvas, NULL, true);
	heeksCAD->AddMenuItem(window_menu, _T("Output"), wxBitmap(), OnOutputCanvas, OnUpdateOutputCanvas, NULL, true);
	heeksCAD->AddMenuItem(window_menu, _T("Print"), wxBitmap(), OnPrintCanvas, OnUpdatePrintCanvas, NULL, true);
	heeksCAD->AddMenuItem(window_menu, _T("Machining"), wxBitmap(), OnMachiningBar, OnUpdateMachiningBar, NULL, true);
	heeksCAD->RegisterHideableWindow(m_program_canvas);
	heeksCAD->RegisterHideableWindow(m_output_canvas);
	heeksCAD->RegisterHideableWindow(m_print_canvas);
	heeksCAD->RegisterHideableWindow(m_machiningBar);

	// add object reading functions
    heeksCAD->RegisterReadXMLfunction("Program", CProgram::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("nccode", CNCCode::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Operations", COperations::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Tools", CTools::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Profile", CProfile::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Pocket", CPocket::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Drilling", CDrilling::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Tool", CTool::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("CuttingTool", CTool::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Tags", CTags::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Tag", CTag::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("ScriptOp", CScriptOp::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Pattern", CPattern::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Patterns", CPatterns::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Surface", CSurface::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Surfaces", CSurfaces::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Stock", CStock::ReadFromXMLElement);
    heeksCAD->RegisterReadXMLfunction("Stocks", CStocks::ReadFromXMLElement);

	// icons
	heeksCAD->RegisterOnBuildTexture(OnBuildTexture);

	// Import functions.
    {
        std::list<wxString> file_extensions;
        file_extensions.push_back(_T("cnc"));
        file_extensions.push_back(_T("drl"));
        file_extensions.push_back(_T("drill"));
        if (! heeksCAD->RegisterFileOpenHandler( file_extensions, ImportExcellonDrillFile ))
        {
            printf("Failed to register handler for Excellon dril files\n");
        }
    }
	{
		std::list<wxString> file_extensions;
		file_extensions.push_back(_T("tool"));
		file_extensions.push_back(_T("tools"));
		file_extensions.push_back(_T("tooltable"));
		if (! heeksCAD->RegisterFileOpenHandler( file_extensions, ImportToolsFile ))
		{
		    printf("Failed to register handler for Tool Table files\n");
		}
	}

    heeksCAD->RegisterObserver(&heekscad_observer);

    heeksCAD->RegisterUnitsChangeHandler( UnitsChangedHandler );
    heeksCAD->RegisterHeeksTypesConverter( HeeksCNCType );

    heeksCAD->RegisterMarkedListTools(&GetMarkedListTools);
    heeksCAD->RegisterOnRestoreDefaults(&OnRestoreDefaults);
}

std::list<wxString> CHeeksCNCApp::GetFileNames( const char *p_szRoot ) const
#ifdef WIN32
{
	std::list<wxString>	results;

	WIN32_FIND_DATA file_data;
	HANDLE hFind;

	std::string pattern = std::string(p_szRoot) + "\\*";
	hFind = FindFirstFile(Ctt(pattern.c_str()), &file_data);

	// Now recurse down until we find document files within 'current' directories.
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) continue;

			results.push_back( file_data.cFileName );
		} while (FindNextFile( hFind, &file_data));

		FindClose(hFind);
	} // End if - then

	return(results);
} // End of GetFileNames() method.
#else
{
	// We're in UNIX land now.

	std::list<wxString>	results;

	DIR *pdir = opendir(p_szRoot);	// Look in the current directory for files
				// whose names begin with "default."
	if (pdir != NULL)
	{
		struct dirent *pent = NULL;
		while ((pent=readdir(pdir)))
		{
			results.push_back(Ctt(pent->d_name));
		} // End while
		closedir(pdir);
	} // End if - then

	return(results);
} // End of GetFileNames() method
#endif



void CHeeksCNCApp::OnNewOrOpen(bool open, int res)
{
	// check for existance of a program

	bool program_found = false;
	for(HeeksObj* object = heeksCAD->GetFirstObject(); object; object = heeksCAD->GetNextObject())
	{
		if(object->GetType() == ProgramType)
		{
			program_found = true;
			break;
		}
	}

	if(!program_found)
	{
		// add the program
		m_program = new CProgram;

		m_program->AddMissingChildren();
		heeksCAD->GetMainObject()->Add(m_program);
		theApp.m_program_canvas->Clear();
		theApp.m_output_canvas->Clear();
		heeksCAD->EndHistory();

		std::list<wxString> directories;
		wxString directory_separator;

		#ifdef WIN32
			directory_separator = _T("\\");
		#else
			directory_separator = _T("/");
		#endif

                wxStandardPaths& standard_paths = wxStandardPaths::Get();
		directories.push_back( standard_paths.GetUserConfigDir() );	// Look for a user-specific file first
		directories.push_back( GetDllFolder() );	// And then look in the application-delivered directory
#ifdef CMAKE_UNIX
	#ifdef RUNINPLACE
		directories.push_back( GetResFolder() );
	#else
		directories.push_back( _T("/usr/local/lib/heekscnc") ); //Location if installed by CMAKE
	#endif
#endif //CMAKE_UNIX
		bool tool_table_found = false;
		bool speed_references_found = false;
		bool fixtures_found = false;

		for (std::list<wxString>::iterator l_itDirectory = directories.begin();
			l_itDirectory != directories.end(); l_itDirectory++)
		{
 			printf("Looking for default data in '%s'\n", Ttc(l_itDirectory->c_str()));

			// Must be a new file.
			// Read in any default speed reference or tool table data.
			std::list<wxString> all_file_names = GetFileNames( l_itDirectory->utf8_str() );
			std::list<wxString> seed_file_names;

			for (std::list<wxString>::iterator l_itFileName = all_file_names.begin();
					l_itFileName != all_file_names.end(); l_itFileName++)
			{
				if (l_itFileName->Find( _("default") ) != -1)
				{
					wxString path;
					path << *l_itDirectory << directory_separator << *l_itFileName;

					seed_file_names.push_back(path);
				} // End if - then
			} // End for

			seed_file_names.sort();	// Sort them so that the user can assign an order alphabetically if they wish.
			for (std::list<wxString>::const_iterator l_itFile = seed_file_names.begin(); l_itFile != seed_file_names.end(); l_itFile++)
			{
				wxString lowercase_file_name( *l_itFile );
				lowercase_file_name.MakeLower();

				if ((tool_table_found == false) && (lowercase_file_name.Find(_T("tool")) != -1))
				{
					printf("Importing data from %s\n",  Ttc(l_itFile->c_str()));
					heeksCAD->OpenXMLFile( l_itFile->c_str(), theApp.m_program->Tools() );
					heeksCAD->EndHistory();
					tool_table_found = true;
				}
			} // End for
		} // End for
	} // End if - then
}


void CHeeksCNCApp::InitializeProperties()
{
	machining_options.Initialize(_("machining options"), this);
    nc_options.Initialize(_("nc options"), &machining_options);
    text_colors.Initialize(_("text colors"), &nc_options);

    CNCCode::InitializeColorProperties(&text_colors);
/*
	CSpeedOp::GetOptions(&(machining_options->m_list));
	CProfile::GetOptions(&(machining_options->m_list));
*/

	CPocket::max_deviation_for_spline_to_arc.Initialize(_("Pocket spline deviation"), &machining_options);

	CSendToMachine::m_command.Initialize(_("Send-to-machine command"), &machining_options);

	m_use_Clipper_not_Boolean.Initialize(_("Use Clipper not Boolean"), &machining_options);
	m_use_DOS_not_Unix.Initialize(_("Use DOS Line Endings"), &machining_options);
}

void CHeeksCNCApp::GetProperties(std::list<Property *> *list)
{
    DomainObject::GetProperties(list);
}

void CHeeksCNCApp::OnFrameDelete()
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	CNCConfig config(ConfigScope());
	config.Write(_T("ProgramVisible"), aui_manager->GetPane(m_program_canvas).IsShown());
	config.Write(_T("OutputVisible"), aui_manager->GetPane(m_output_canvas).IsShown());
	config.Write(_T("PrintVisible"), aui_manager->GetPane(m_print_canvas).IsShown());
	config.Write(_T("MachiningBarVisible"), aui_manager->GetPane(m_machiningBar).IsShown());

	CNCCode::WriteColorsToConfig();
	CProfile::WriteToConfig();
	CPocket::WriteToConfig();
	CSpeedOp::WriteToConfig();
	CSendToMachine::WriteToConfig();
	config.Write(_T("UseClipperNotBoolean"), m_use_Clipper_not_Boolean);
    config.Write(_T("UseDOSNotUnix"), m_use_DOS_not_Unix);
}

Python CHeeksCNCApp::SetTool( const int new_tool )
{
    Python python;

    // Select the right tool.
    CTool *pTool = (CTool *) CTool::Find(new_tool);
    if (pTool != NULL)
    {
        if (m_tool_number != new_tool)
        {

            python << _T("tool_change( id=") << new_tool << _T(")\n");
        }

        if(m_attached_to_surface)
        {
            python << _T("nc.creator.set_ocl_cutter(") << pTool->OCLDefinition(m_attached_to_surface) << _T(")\n");
        }
    } // End if - then

    m_tool_number = new_tool;

    return(python);
}

wxString CHeeksCNCApp::GetDllFolder()
{
	return m_dll_path;
}

wxString CHeeksCNCApp::GetResFolder()
{
#if MACOSX
	return m_dll_path;

#elif defined(WIN32) || defined(RUNINPLACE) //compile with 'RUNINPLACE=yes make' then skip 'sudo make install'
	#ifdef CMAKE_UNIX
		return (m_dll_path + _T("/.."));
	#else
		return m_dll_path;
	#endif

#else
#ifdef CMAKE_UNIX
	return (_T("/usr/local/share/heekscnc"));
#else
	return (m_dll_path + _T("/../../share/heekscnc"));
#endif
#endif
}


wxString CHeeksCNCApp::GetBitmapPath(const wxString& name)
{
	return theApp.GetResFolder() + _T("/bitmaps/") + name + _T(".png");
}

wxString CHeeksCNCApp::GetDialogBitmapPath(const wxString& name, const wxString& folder)
{
    return theApp.GetResFolder() + _T("/bitmaps/") + folder + "/" + name + _T(".png");
}


class MyApp : public wxApp
{

 public:

   virtual bool OnInit(void);

 };

 bool MyApp::OnInit(void)

 {

   return true;

 }


 DECLARE_APP(MyApp)

 IMPLEMENT_APP(MyApp)


wxString HeeksCNCType( const int type )
{
    switch (type)
    {
    case NCCodeBlockType:    return(_("NCCodeBlock"));
    case NCCodeType:         return(_("NCCode"));
    case OperationsType:     return(_("Operations"));
    case ProfileType:        return(_("Profile"));
    case PocketType:         return(_("Pocket"));
    case DrillingType:       return(_("Drilling"));
    case ToolType:           return(_("Tool"));
    case ToolsType:          return(_("Tools"));
    case TagsType:           return(_("Tags"));
    case TagType:            return(_("Tag"));
    case ScriptOpType:       return(_("ScriptOp"));

	default:
        return(_T("")); // Indicates that this function could not make the conversion.
    } // End switch
} // End HeeksCNCType() routine
