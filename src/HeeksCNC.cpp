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
#include "ZigZag.h"
#include "Waterline.h"
#include "Drilling.h"
#include "Tapping.h"
#include "Positioning.h"
#include "CTool.h"
#include "CounterBore.h"
#ifndef STABLE_OPS_ONLY
#include "TurnRough.h"
#endif
#include "Fixture.h"
#include "SpeedReference.h"
#include "CuttingRate.h"
#include "Operations.h"
#include "Fixtures.h"
#include "Tools.h"
#include "interface/strconv.h"
#include "CNCPoint.h"
#include "BOM.h"
#include "Probing.h"
#include "Excellon.h"
#include "Chamfer.h"
#include "Contour.h"
#ifndef STABLE_OPS_ONLY
#include "Inlay.h"
#endif
#include "Tags.h"
#include "Tag.h"
#include "ScriptOp.h"
#include "AttachOp.h"

#include <sstream>

CHeeksCADInterface* heeksCAD = NULL;

CHeeksCNCApp theApp;

#ifndef STABLE_OPS_ONLY
extern void ImportFixturesFile( const wxChar *file_path );
#endif
extern void ImportToolsFile( const wxChar *file_path );
extern void ImportSpeedReferencesFile( const wxChar *file_path );

extern CTool::tap_sizes_t metric_tap_sizes[];
extern CTool::tap_sizes_t unified_thread_standard_tap_sizes[];
extern CTool::tap_sizes_t british_standard_whitworth_tap_sizes[];

wxString HeeksCNCType(const int type);

CHeeksCNCApp::CHeeksCNCApp(){
	m_draw_cutter_radius = true;
	m_program = NULL;
	m_run_program_on_new_line = false;
	m_machiningBar = NULL;
	m_icon_texture_number = 0;
	m_machining_hidden = false;
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

static void OnUpdateOutputCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	event.Check(aui_manager->GetPane(theApp.m_output_canvas).IsShown());
}

static bool GetSketches(std::list<int>& sketches, std::list<int> &tools )
{
	// check for at least one sketch selected

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SketchType)
		{
			sketches.push_back(object->m_id);
		} // End if - then

		if ((object != NULL) && (object->GetType() == ToolType))
		{
			tools.push_back( object->m_id );
		} // End if - then
	}

	if(sketches.size() == 0)
	{
		wxMessageBox(_("You must select some sketches, first!"));
		return false;
	}

	return true;
}

static void NewProfileOpMenuCallback(wxCommandEvent &event)
{
	std::list<int> drill_bits;
	std::list<int> tools;
	std::list<int> sketches;
	int milling_tool_number = -1;
	if(GetSketches(sketches, tools))
	{
		// Look through the tools that have been selected.  If any of them
		// are drill or centre bits then create Drilling cycle objects as well.
		// If we find a milling bit then pass that through to the CProfile object.
		std::list<int>::const_iterator l_itTool;
		for (l_itTool = tools.begin(); l_itTool != tools.end(); l_itTool++)
		{
			HeeksObj *ob = heeksCAD->GetIDObject( ToolType, *l_itTool );
			if (ob != NULL)
			{
				switch (((CTool *)ob)->m_params.m_type)
				{
					case CToolParams::eDrill:
					case CToolParams::eCentreDrill:
						// Keep a list for later.  We need to create the Profile object
						// before we create Drilling objects that refer to it.
						drill_bits.push_back( *l_itTool );
						break;

					case CToolParams::eChamfer:
					case CToolParams::eEndmill:
					case CToolParams::eSlotCutter:
					case CToolParams::eBallEndMill:
						// We only want one.  Just keep overwriting this variable.
						milling_tool_number = ((CTool *)ob)->m_tool_number;
						break;

					default:
						break;
				} // End switch
			} // End if - then
		} // End for

		heeksCAD->CreateUndoPoint();
		CProfile *new_object = new CProfile(sketches, milling_tool_number);
		new_object->AddMissingChildren();
		theApp.m_program->Operations()->Add(new_object,NULL);
		heeksCAD->ClearMarkedList();
		heeksCAD->Mark(new_object);

		CDrilling::Symbols_t profiles;
		profiles.push_back( CDrilling::Symbol_t( new_object->GetType(), new_object->m_id ) );

		for (l_itTool = drill_bits.begin(); l_itTool != drill_bits.end(); l_itTool++)
		{
			HeeksObj *ob = heeksCAD->GetIDObject( ToolType, *l_itTool );
			if (ob != NULL)
			{
				CDrilling *new_object = new CDrilling( profiles, ((CTool *)ob)->m_tool_number, -1 );
				theApp.m_program->Operations()->Add(new_object, NULL);
				heeksCAD->ClearMarkedList();
				heeksCAD->Mark(new_object);
			} // End if - then
		} // End for
		heeksCAD->Changed();
	}
}

static void NewPocketOpMenuCallback(wxCommandEvent &event)
{
	std::list<int> tools;
	std::list<int> sketches;
	if(GetSketches(sketches, tools))
	{
		CPocket *new_object = new CPocket(sketches, (tools.size()>0)?(*tools.begin()):-1 );
		if(new_object->Edit())
		{
			heeksCAD->CreateUndoPoint();
			theApp.m_program->Operations()->Add(new_object, NULL);
			heeksCAD->Changed();
		}
		else
			delete new_object;
	}
}

#ifndef STABLE_OPS_ONLY

static void NewZigZagOpMenuCallback(wxCommandEvent &event)
{
	// check for at least one solid selected
	std::list<int> solids;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
	}

	// if no selected solids,
	if(solids.size() == 0)
	{
		// use all the solids in the drawing
		for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
		{
			if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
		}
	}

	if(solids.size() == 0)
	{
		wxMessageBox(_("There are no solids!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	CZigZag *new_object = new CZigZag(solids);
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewWaterlineOpMenuCallback(wxCommandEvent &event)
{
	// check for at least one solid selected
	std::list<int> solids;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
	}

	// if no selected solids,
	if(solids.size() == 0)
	{
		// use all the solids in the drawing
		for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
		{
			if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
		}
	}

	if(solids.size() == 0)
	{
		wxMessageBox(_("There are no solids!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	CWaterline *new_object = new CWaterline(solids);
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}
#endif


static void NewDrillingOpMenuCallback(wxCommandEvent &event)
{
	std::vector<CNCPoint> intersections;
	CDrilling::Symbols_t symbols;
	CDrilling::Symbols_t Tools;
	int tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object->GetType() == ToolType)
		{
			Tools.push_back( CDrilling::Symbol_t( object->GetType(), object->m_id ) );
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
		else
		{
		    if (CDrilling::ValidType( object->GetType() ))
		    {
                symbols.push_back( CDrilling::Symbol_t( object->GetType(), object->m_id ) );
		    }
		} // End if - else
	} // End for

	double depth = -1;
	CDrilling::Symbols_t ToolsThatMatchCircles;
	CDrilling drill( symbols, -1, -1 );

	intersections = CDrilling::FindAllLocations(&drill);

	if ((Tools.size() == 0) && (ToolsThatMatchCircles.size() > 0))
	{
		// The operator didn't point to a tool object and one of the circles that they
		// did point to happenned to match the diameter of an existing tool.  Use that
		// one as our default.  The operator can always overwrite it later on.

		std::copy( ToolsThatMatchCircles.begin(), ToolsThatMatchCircles.end(),
				std::inserter( Tools, Tools.begin() ));
	} // End if - then

	if (intersections.size() == 0)
	{
		wxMessageBox(_("You must select some points, circles or other intersecting elements first!"));
		return;
	}

	if(Tools.size() > 1)
	{
		wxMessageBox(_("You may only select a single tool for each drilling operation.!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	CDrilling *new_object = new CDrilling( symbols, tool_number, depth );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}


static void NewTappingOpMenuCallback(wxCommandEvent &event)
{
	std::vector<CNCPoint> intersections;
	CTapping::Symbols_t symbols;
	CTapping::Symbols_t Tools;
	int tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object->GetType() == ToolType)
		{
			Tools.push_back( CTapping::Symbol_t( object->GetType(), object->m_id ) );
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
		else
		{
		    if (CTapping::ValidType( object->GetType() ))
		    {
                symbols.push_back( CTapping::Symbol_t( object->GetType(), object->m_id ) );
		    }
		} // End if - else
	} // End for

	double depth = -1;
	CTapping::Symbols_t ToolsThatMatchCircles;
	CTapping tap( symbols, -1, -1 );

	intersections = CDrilling::FindAllLocations(&tap);

	if ((Tools.size() == 0) && (ToolsThatMatchCircles.size() > 0))
	{
		// The operator didn't point to a tool object and one of the circles that they
		// did point to happenned to match the diameter of an existing tool.  Use that
		// one as our default.  The operator can always overwrite it later on.

		std::copy( ToolsThatMatchCircles.begin(), ToolsThatMatchCircles.end(),
				std::inserter( Tools, Tools.begin() ));
	} // End if - then

	if (intersections.size() == 0)
	{
		wxMessageBox(_("You must select some points, circles or other intersecting elements first!"));
		return;
	}

	if(Tools.size() > 1)
	{
		wxMessageBox(_("You may only select a single tool for each tapping operation.!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	//CDrilling *new_object = new CDrilling( symbols, tool_number, depth );
	CTapping *new_object = new CTapping( symbols, tool_number, depth );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

#ifndef STABLE_OPS_ONLY
static void NewChamferOpMenuCallback(wxCommandEvent &event)
{
	CDrilling::Symbols_t symbols;
	CDrilling::Symbols_t Tools;
	int tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object->GetType() == ToolType)
		{
			Tools.push_back( CDrilling::Symbol_t( object->GetType(), object->m_id ) );
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
		else
		{
			symbols.push_back( CChamfer::Symbol_t( object->GetType(), object->m_id ) );
		} // End if - else
	} // End for

	if(Tools.size() > 1)
	{
		wxMessageBox(_("You may only select a single tool for each chamfer operation.!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	CChamfer *new_object = new CChamfer( symbols, tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}
#endif

static void NewAttachOpMenuCallback(wxCommandEvent &event)
{
	// check for at least one solid selected
	std::list<int> solids;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
	}

	// if no selected solids,
	if(solids.size() == 0)
	{
		// use all the solids in the drawing
		for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
		{
			if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
		}
	}

	if(solids.size() == 0)
	{
		wxMessageBox(_("There are no solids!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	CAttachOp *new_object = new CAttachOp(solids, 0.01, 0.0);
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewUnattachOpMenuCallback(wxCommandEvent &event)
{
	heeksCAD->CreateUndoPoint();
	CUnattachOp *new_object = new CUnattachOp();
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewPositioningOpMenuCallback(wxCommandEvent &event)
{
	std::vector<CNCPoint> intersections;
	CDrilling::Symbols_t symbols;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object != NULL)
		{
		    if (CPositioning::ValidType( object->GetType() ))
		    {
                symbols.push_back( CDrilling::Symbol_t( object->GetType(), object->m_id ) );
		    }
		} // End if - then
	} // End for

    if (symbols.size() == 0)
    {
        wxMessageBox(_("You must select some points, circles or other intersecting elements first!"));
        return;
    }

	heeksCAD->CreateUndoPoint();
	CPositioning *new_object = new CPositioning( symbols );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewProbe_Centre_MenuCallback(wxCommandEvent &event)
{
	CTool::ToolNumber_t tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if ((object != NULL) && (object->GetType() == ToolType))
		{
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
	} // End for

	if (tool_number == 0)
	{
		tool_number = CTool::FindFirstByType( CToolParams::eTouchProbe );
	} // End if - then

	heeksCAD->CreateUndoPoint();
	CProbe_Centre *new_object = new CProbe_Centre( tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewProbe_Grid_MenuCallback(wxCommandEvent &event)
{
	CTool::ToolNumber_t tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if ((object != NULL) && (object->GetType() == ToolType))
		{
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
	} // End for

	if (tool_number == 0)
	{
		tool_number = CTool::FindFirstByType( CToolParams::eTouchProbe );
	} // End if - then

	heeksCAD->CreateUndoPoint();
	CProbe_Grid *new_object = new CProbe_Grid( tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}


static void NewProbe_Edge_MenuCallback(wxCommandEvent &event)
{
	CTool::ToolNumber_t tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if ((object != NULL) && (object->GetType() == ToolType))
		{
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
	} // End for

	if (tool_number == 0)
	{
		tool_number = CTool::FindFirstByType( CToolParams::eTouchProbe );
	} // End if - then

	heeksCAD->CreateUndoPoint();
	CProbe_Edge *new_object = new CProbe_Edge( tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

#ifndef STABLE_OPS_ONLY

static void NewFixtureMenuCallback(wxCommandEvent &event)
{
	if (theApp.m_program->Fixtures()->GetNextFixture() > 0)
	{
		CFixture::eCoordinateSystemNumber_t coordinate_system_number = CFixture::eCoordinateSystemNumber_t(theApp.m_program->Fixtures()->GetNextFixture());

		heeksCAD->CreateUndoPoint();
		CFixture *new_object = new CFixture( NULL, coordinate_system_number, theApp.m_program->m_machine.m_safety_height_defined, theApp.m_program->m_machine.m_safety_height );
		theApp.m_program->Fixtures()->Add(new_object, NULL);
		heeksCAD->ClearMarkedList();
		heeksCAD->Mark(new_object);
		heeksCAD->Changed();
	} // End if - then
	else
	{
		wxMessageBox(_T("There are no more coordinate systems available"));
	} // End if - else
}
#endif

static void DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;

	HeeksObj* operations = theApp.m_program->Operations();

	for(HeeksObj* obj = operations->GetFirstChild(); obj; obj = operations->GetNextChild())
	{
		if (COperations::IsAnOperation( obj->GetType() ))
		{
			if (obj != NULL)
			{
				std::list<wxString> change = ((COp *)obj)->DesignRulesAdjustment(apply_changes);
				std::copy( change.begin(), change.end(), std::inserter( changes, changes.end() ));
			} // End if - then
		} // End if - then
	} // End for

	std::wostringstream l_ossChanges;
	for (std::list<wxString>::const_iterator l_itChange = changes.begin(); l_itChange != changes.end(); l_itChange++)
	{
		l_ossChanges << l_itChange->c_str();
	} // End for

	if (l_ossChanges.str().size() > 0)
	{
		wxMessageBox( l_ossChanges.str().c_str() );
	} // End if - then

} // End DesignRulesAdjustmentMenuCallback() routine


static void DesignRulesAdjustmentMenuCallback(wxCommandEvent &event)
{
	DesignRulesAdjustment(true);
} // End DesignRulesAdjustmentMenuCallback() routine

static void DesignRulesCheckMenuCallback(wxCommandEvent &event)
{
	DesignRulesAdjustment(false);
} // End DesignRulesCheckMenuCallback() routine

#ifndef STABLE_OPS_ONLY
static void NewCounterBoreOpMenuCallback(wxCommandEvent &event)
{
	std::vector<CNCPoint> intersections;
	CCounterBore::Symbols_t symbols;
	CCounterBore::Symbols_t Tools;
	int tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object->GetType() == ToolType)
		{
			Tools.push_back( CCounterBore::Symbol_t( object->GetType(), object->m_id ) );
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
		else
		{
		    if (CCounterBore::ValidType( object->GetType() ))
		    {
                symbols.push_back( CCounterBore::Symbol_t( object->GetType(), object->m_id ) );
		    }
		} // End if - else
	} // End for

	CCounterBore::Symbols_t ToolsThatMatchCircles;
	CCounterBore counterbore( symbols, -1 );
	intersections = CDrilling::FindAllLocations( &counterbore );

	if ((Tools.size() == 0) && (ToolsThatMatchCircles.size() > 0))
	{
		// The operator didn't point to a tool object and one of the circles that they
		// did point to happenned to match the diameter of an existing tool.  Use that
		// one as our default.  The operator can always overwrite it later on.

		std::copy( ToolsThatMatchCircles.begin(), ToolsThatMatchCircles.end(),
				std::inserter( Tools, Tools.begin() ));
	} // End if - then

	if (intersections.size() == 0)
	{
		wxMessageBox(_("You must select some points, circles or other intersecting elements first!"));
		return;
	}

	if(Tools.size() > 1)
	{
		wxMessageBox(_("You may only select a single tool for each drilling operation.!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	CCounterBore *new_object = new CCounterBore( symbols, tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewContourOpMenuCallback(wxCommandEvent &event)
{
	CContour::Symbols_t symbols;
	CContour::Symbols_t Tools;
	int tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object == NULL) continue;

		if (object->GetType() == ToolType)
		{
			Tools.push_back( CContour::Symbol_t( object->GetType(), object->m_id ) );
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
		else
		{
		    symbols.push_back( CContour::Symbol_t( object->GetType(), object->m_id ) );
		} // End if - else
	} // End for

	heeksCAD->CreateUndoPoint();
	CContour *new_object = new CContour( symbols, tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewInlayOpMenuCallback(wxCommandEvent &event)
{
	CInlay::Symbols_t symbols;
	CInlay::Symbols_t Tools;
	int tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object->GetType() == ToolType)
		{
			Tools.push_back( CInlay::Symbol_t( object->GetType(), object->m_id ) );
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
		else
		{
		    symbols.push_back( CInlay::Symbol_t( object->GetType(), object->m_id ) );
		} // End if - else
	} // End for

	heeksCAD->CreateUndoPoint();
	CInlay *new_object = new CInlay( symbols, tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

#endif

static void NewSpeedReferenceMenuCallback(wxCommandEvent &event)
{
	heeksCAD->CreateUndoPoint();
	CSpeedReference *new_object = new CSpeedReference(_T("Fill in material name"), int(CToolParams::eCarbide), 0.0, 0.0);
	theApp.m_program->SpeedReferences()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewCuttingRateMenuCallback(wxCommandEvent &event)
{
	heeksCAD->CreateUndoPoint();
	CCuttingRate *new_object = new CCuttingRate(0.0, 0.0);
	theApp.m_program->SpeedReferences()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

#ifndef STABLE_OPS_ONLY

static void NewRoughTurnOpMenuCallback(wxCommandEvent &event)
{
	std::list<int> tools;
	std::list<int> sketches;
	if(GetSketches(sketches, tools))
	{
		heeksCAD->CreateUndoPoint();
		CTurnRough *new_object = new CTurnRough(sketches, (tools.size()>0)?(*tools.begin()):-1 );
		theApp.m_program->Operations()->Add(new_object, NULL);
		heeksCAD->ClearMarkedList();
		heeksCAD->Mark(new_object);
		heeksCAD->Changed();
	}
}

#endif

static void NewScriptOpMenuCallback(wxCommandEvent &event)
{
	heeksCAD->CreateUndoPoint();
	CScriptOp *new_object = new CScriptOp();
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void AddNewTool(CToolParams::eToolType type)
{
	// Add a new tool.
	heeksCAD->CreateUndoPoint();
	CTool *new_object = new CTool(NULL, type, heeksCAD->GetNextID(ToolType));
	theApp.m_program->Tools()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewDrillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eDrill);
}

static void NewMetricTappingToolMenuCallback(wxCommandEvent &event)
{
    wxString message(_("Select tap size"));
    wxString caption(_("Standard Tap Sizes"));

    wxArrayString choices;

    for (::size_t i=0; (metric_tap_sizes[i].diameter > 0.0); i++)
    {
        choices.Add(metric_tap_sizes[i].description);
    }

    wxString choice = ::wxGetSingleChoice( message, caption, choices );

    for (::size_t i=0; (metric_tap_sizes[i].diameter > 0.0); i++)
    {
        if ((choices.size() > 0) && (choice == choices[i]))
        {
            // Add a new tool.
            heeksCAD->CreateUndoPoint();
            CTool *new_object = new CTool(NULL, CToolParams::eTapTool, heeksCAD->GetNextID(ToolType));
            new_object->m_params.m_diameter = metric_tap_sizes[i].diameter;
            new_object->m_params.m_pitch = metric_tap_sizes[i].pitch;
            new_object->m_params.m_direction = 0;    // Right hand thread.
            new_object->ResetTitle();
            theApp.m_program->Tools()->Add(new_object, NULL);
            heeksCAD->ClearMarkedList();
            heeksCAD->Mark(new_object);
            heeksCAD->Changed();
            return;
        }
    }
}

static void NewUnifiedThreadingStandardTappingToolMenuCallback(wxCommandEvent &event)
{
	wxString message(_("Select tap size"));
    wxString caption(_("Standard Tap Sizes"));

    wxArrayString choices;

    for (::size_t i=0; (unified_thread_standard_tap_sizes[i].diameter > 0.0); i++)
    {
        choices.Add(unified_thread_standard_tap_sizes[i].description);
    }

    wxString choice = ::wxGetSingleChoice( message, caption, choices );

    for (::size_t i=0; (unified_thread_standard_tap_sizes[i].diameter > 0.0); i++)
    {
        if ((choices.size() > 0) && (choice == choices[i]))
        {
            // Add a new tool.
            heeksCAD->CreateUndoPoint();
            CTool *new_object = new CTool(NULL, CToolParams::eTapTool, heeksCAD->GetNextID(ToolType));
            new_object->m_params.m_diameter = unified_thread_standard_tap_sizes[i].diameter;
            new_object->m_params.m_pitch = unified_thread_standard_tap_sizes[i].pitch;
            new_object->m_params.m_direction = 0;    // Right hand thread.
            new_object->ResetTitle();
            theApp.m_program->Tools()->Add(new_object, NULL);
            heeksCAD->ClearMarkedList();
            heeksCAD->Mark(new_object);
            heeksCAD->Changed();
            return;
        }
    }
}

static void NewBritishStandardWhitworthTappingToolMenuCallback(wxCommandEvent &event)
{
	wxString message(_("Select tap size"));
    wxString caption(_("Standard Tap Sizes"));

    wxArrayString choices;

    for (::size_t i=0; (british_standard_whitworth_tap_sizes[i].diameter > 0.0); i++)
    {
        choices.Add(british_standard_whitworth_tap_sizes[i].description);
    }

    wxString choice = ::wxGetSingleChoice( message, caption, choices );

    for (::size_t i=0; (british_standard_whitworth_tap_sizes[i].diameter > 0.0); i++)
    {
        if ((choices.size() > 0) && (choice == choices[i]))
        {
            // Add a new tool.
            heeksCAD->CreateUndoPoint();
            CTool *new_object = new CTool(NULL, CToolParams::eTapTool, heeksCAD->GetNextID(ToolType));
            new_object->m_params.m_diameter = british_standard_whitworth_tap_sizes[i].diameter;
            new_object->m_params.m_pitch = british_standard_whitworth_tap_sizes[i].pitch;
            new_object->m_params.m_direction = 0;    // Right hand thread.
            new_object->ResetTitle();
            theApp.m_program->Tools()->Add(new_object, NULL);
            heeksCAD->ClearMarkedList();
            heeksCAD->Mark(new_object);
            heeksCAD->Changed();
            return;
        }
    }
}

static void NewTapToolMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eTapTool);
}

static void NewEngraverToolMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eEngravingTool);
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

#ifndef STABLE_OPS_ONLY
static void NewTurningToolMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eTurningTool);
}
#endif

static void NewTouchProbeMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eTouchProbe);
}

static void NewToolLengthSwitchMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eToolLengthSwitch);
}

#ifndef STABLE_OPS_ONLY
static void NewExtrusionMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eExtrusion);
}
#endif

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
	{
		// clear the backplot file
		wxString backplot_path = m_program->GetOutputFileName() + _T(".nc.xml");
		wxFile f(backplot_path.c_str(), wxFile::write);
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


static void OpenBOMFileMenuCallback(wxCommandEvent& event)
{
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("BOM files")) + _T(" |") + ext_str;
	wxFileDialog dialog(theApp.m_output_canvas, _("Open BOM file"), wxEmptyString, wxEmptyString, wildcard_string);
	dialog.CentreOnParent();

	if (dialog.ShowModal() == wxID_OK)
	{
		theApp.m_program->Owner()->Add(new CBOM(dialog.GetPath()),NULL);
	}
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
#ifndef STABLE_OPS_ONLY
static CCallbackTool new_turn_tool(_("New Turning Tool..."), _T("turntool"), NewTurningToolMenuCallback);
#endif
static CCallbackTool new_touch_probe(_("New Touch Probe..."), _T("probe"), NewTouchProbeMenuCallback);
#ifndef STABLE_OPS_ONLY
static CCallbackTool new_extrusion(_("New Extrusion..."), _T("extrusion"), NewExtrusionMenuCallback);
#endif
static CCallbackTool new_tool_length_switch(_("New Tool Length Switch..."), _T("probe"), NewToolLengthSwitchMenuCallback);
static CCallbackTool new_tap_tool(_("New Tap Tool..."), _T("tap"), NewTapToolMenuCallback);
static CCallbackTool new_engraver_tool(_("New Engraver Tool..."), _T("engraver"), NewEngraverToolMenuCallback);

void CHeeksCNCApp::GetNewToolTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_drill_tool);
	t_list->push_back(&new_centre_drill_tool);
	t_list->push_back(&new_endmill_tool);
	t_list->push_back(&new_slotdrill_tool);
	t_list->push_back(&new_ball_end_mill_tool);
	t_list->push_back(&new_chamfer_mill_tool);
	// t_list->push_back(&new_chamfer_mill_tool);
#ifndef STABLE_OPS_ONLY
	t_list->push_back(&new_turn_tool);
#endif
	t_list->push_back(&new_touch_probe);
#ifndef STABLE_OPS_ONLY
	t_list->push_back(&new_extrusion);
#endif
	t_list->push_back(&new_tool_length_switch);
	t_list->push_back(&new_tap_tool);
	t_list->push_back(&new_engraver_tool);
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
#ifndef STABLE_OPS_ONLY
		heeksCAD->AddFlyoutButton(_("CounterBore"), ToolImage(theApp.GetBitmapPath(_T("counterbore")), true), _("New CounterBore Cycle Operation..."), NewCounterBoreOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Contour"), ToolImage(theApp.GetBitmapPath(_T("opcontour")), true), _("New Contour Operation..."), NewContourOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Inlay"), ToolImage(theApp.GetBitmapPath(_T("opinlay")), true), _("New Inlay Operation..."), NewInlayOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Chamfer"), ToolImage(theApp.GetBitmapPath(_T("opchamfer")), true), _("New Chamfer Operation..."), NewChamferOpMenuCallback);
#endif
		heeksCAD->AddFlyoutButton(_("Tap"), ToolImage(theApp.GetBitmapPath(_T("optap")), true), _("New Tapping Operation..."), NewTappingOpMenuCallback);
		heeksCAD->EndToolBarFlyout(theApp.m_machiningBar);

		heeksCAD->StartToolBarFlyout(_("3D Milling operations"));
#ifndef STABLE_OPS_ONLY
		heeksCAD->AddFlyoutButton(_("ZigZag"), ToolImage(theApp.GetBitmapPath(_T("zigzag")), true), _("New ZigZag Operation..."), NewZigZagOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Waterline"), ToolImage(theApp.GetBitmapPath(_T("waterline")), true), _("New Waterline Operation..."), NewWaterlineOpMenuCallback);
#endif
		heeksCAD->AddFlyoutButton(_("Attach"), ToolImage(theApp.GetBitmapPath(_T("attach")), true), _("New Attach Operation..."), NewAttachOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Unattach"), ToolImage(theApp.GetBitmapPath(_T("unattach")), true), _("New Unattach Operation..."), NewUnattachOpMenuCallback);
		heeksCAD->EndToolBarFlyout(theApp.m_machiningBar);

		heeksCAD->StartToolBarFlyout(_("Other operations"));
		heeksCAD->AddFlyoutButton(_("Positioning"), ToolImage(theApp.GetBitmapPath(_T("locating")), true), _("New Positioning Operation..."), NewPositioningOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Probing"), ToolImage(theApp.GetBitmapPath(_T("probe")), true), _("New Probe Centre Operation..."), NewProbe_Centre_MenuCallback);
		heeksCAD->AddFlyoutButton(_("Probing"), ToolImage(theApp.GetBitmapPath(_T("probe")), true), _("New Probe Edge Operation..."), NewProbe_Edge_MenuCallback);
		heeksCAD->AddFlyoutButton(_("Probing"), ToolImage(theApp.GetBitmapPath(_T("probe")), true), _("New Probe Grid Operation..."), NewProbe_Grid_MenuCallback);
		heeksCAD->AddFlyoutButton(_("ScriptOp"), ToolImage(theApp.GetBitmapPath(_T("scriptop")), true), _("New Script Operation..."), NewScriptOpMenuCallback);
		heeksCAD->EndToolBarFlyout(theApp.m_machiningBar);

		heeksCAD->AddToolBarButton(theApp.m_machiningBar, _("Tool"), ToolImage(theApp.GetBitmapPath(_T("drill")), true), _("New Tool Definition..."), NewDrillMenuCallback);

#ifndef STABLE_OPS_ONLY
		heeksCAD->AddToolBarButton(theApp.m_machiningBar, _("Fixture"), ToolImage(theApp.GetBitmapPath(_T("fixture")), true), _("New Fixture..."), NewFixtureMenuCallback);
#endif
		heeksCAD->StartToolBarFlyout(_("Design Rules"));
		heeksCAD->AddFlyoutButton(_("Design Rules Check"), ToolImage(theApp.GetBitmapPath(_T("design_rules_check")), true), _("Design Rules Check..."), DesignRulesCheckMenuCallback);
		heeksCAD->AddFlyoutButton(_("Design Rules Adjustment"), ToolImage(theApp.GetBitmapPath(_T("design_rules_adjustment")), true), _("Design Rules Adjustment..."), DesignRulesAdjustmentMenuCallback);
		heeksCAD->EndToolBarFlyout(theApp.m_machiningBar);

		heeksCAD->StartToolBarFlyout(_("Speeds"));
		heeksCAD->AddFlyoutButton(_("Speed Reference"), ToolImage(theApp.GetBitmapPath(_T("speed_reference")), true), _("Add Speed Reference..."), NewSpeedReferenceMenuCallback);
		heeksCAD->AddFlyoutButton(_("Cutting Rate"), ToolImage(theApp.GetBitmapPath(_T("cutting_rate")), true), _("Add Cutting Rate Reference..."), NewCuttingRateMenuCallback);
		heeksCAD->EndToolBarFlyout(theApp.m_machiningBar);

		heeksCAD->StartToolBarFlyout(_("Post Processing"));
		heeksCAD->AddFlyoutButton(_("PostProcess"), ToolImage(theApp.GetBitmapPath(_T("postprocess")), true), _("Post-Process"), PostProcessMenuCallback);
		heeksCAD->AddFlyoutButton(_("Make Python Script"), ToolImage(theApp.GetBitmapPath(_T("python")), true), _("Make Python Script"), MakeScriptMenuCallback);
		heeksCAD->AddFlyoutButton(_("Run Python Script"), ToolImage(theApp.GetBitmapPath(_T("runpython")), true), _("Run Python Script"), RunScriptMenuCallback);
		heeksCAD->AddFlyoutButton(_("OpenNC"), ToolImage(theApp.GetBitmapPath(_T("opennc")), true), _("Open NC File"), OpenNcFileMenuCallback);
		heeksCAD->AddFlyoutButton(_("SaveNC"), ToolImage(theApp.GetBitmapPath(_T("savenc")), true), _("Save NC File"), SaveNcFileMenuCallback);
		heeksCAD->AddFlyoutButton(_("Send to Machine"), ToolImage(theApp.GetBitmapPath(_T("tomachine")), true), _("Send to Machine"), SendToMachineMenuCallback);
		heeksCAD->AddFlyoutButton(_("Cancel"), ToolImage(theApp.GetBitmapPath(_T("cancel")), true), _("Cancel Python Script"), CancelMenuCallback);
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

static void UnitsChangedHandler( const double units )
{
    // The view units have changed.  See if the user wants the NC output units to change
    // as well.

    if (units != theApp.m_program->m_units)
    {
        int response;
        response = wxMessageBox( _("Would you like to change the NC code generation units too?"), _("Change Units"), wxYES_NO );
        if (response == wxYES)
        {
            theApp.m_program->ChangeUnits( units );
        }
    }
}


void CHeeksCNCApp::OnStartUp(CHeeksCADInterface* h, const wxString& dll_path)
{
	m_dll_path = dll_path;
	heeksCAD = h;
#if !defined WXUSINGDLL
	wxInitialize();
#endif

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

	// Milling Operations menu
	wxMenu *menuMillingOperations = new wxMenu;
	heeksCAD->AddMenuItem(menuMillingOperations, _("Profile Operation..."), ToolImage(theApp.GetBitmapPath(_T("opprofile")), true), NewProfileOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Pocket Operation..."), ToolImage(theApp.GetBitmapPath(_T("pocket")), true), NewPocketOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Drilling Operation..."), ToolImage(theApp.GetBitmapPath(_T("drilling")), true), NewDrillingOpMenuCallback);
#ifndef STABLE_OPS_ONLY
	heeksCAD->AddMenuItem(menuMillingOperations, _("Chamfer Operation..."), ToolImage(theApp.GetBitmapPath(_T("opchamfer")), true), NewChamferOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("CounterBore Operation..."), ToolImage(theApp.GetBitmapPath(_T("counterbore")), true), NewCounterBoreOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Contour Operation..."), ToolImage(theApp.GetBitmapPath(_T("opcontour")), true), NewContourOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Inlay Operation..."), ToolImage(theApp.GetBitmapPath(_T("opinlay")), true), NewInlayOpMenuCallback);
#endif
	heeksCAD->AddMenuItem(menuMillingOperations, _("Tapping Operation..."), ToolImage(theApp.GetBitmapPath(_T("optap")), true), NewTappingOpMenuCallback);

	wxMenu *menu3dMillingOperations = new wxMenu;
#ifndef STABLE_OPS_ONLY
	heeksCAD->AddMenuItem(menu3dMillingOperations, _("ZigZag Operation..."), ToolImage(theApp.GetBitmapPath(_T("zigzag")), true), NewZigZagOpMenuCallback);
	heeksCAD->AddMenuItem(menu3dMillingOperations, _("Waterline Operation..."), ToolImage(theApp.GetBitmapPath(_T("waterline")), true), NewWaterlineOpMenuCallback);
#endif
	heeksCAD->AddMenuItem(menu3dMillingOperations, _("Attach Operation..."), ToolImage(theApp.GetBitmapPath(_T("attach")), true), NewAttachOpMenuCallback);
	heeksCAD->AddMenuItem(menu3dMillingOperations, _("Unattach Operation..."), ToolImage(theApp.GetBitmapPath(_T("unattach")), true), NewUnattachOpMenuCallback);

	wxMenu *menuOperations = new wxMenu;
#ifndef STABLE_OPS_ONLY
	heeksCAD->AddMenuItem(menuOperations, _("Rough Turning Operation..."), ToolImage(theApp.GetBitmapPath(_T("turnrough")), true), NewRoughTurnOpMenuCallback);
#endif
	heeksCAD->AddMenuItem(menuOperations, _("Script Operation..."), ToolImage(theApp.GetBitmapPath(_T("scriptop")), true), NewScriptOpMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Design Rules Check..."), ToolImage(theApp.GetBitmapPath(_T("design_rules_check")), true), DesignRulesCheckMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Design Rules Adjustment..."), ToolImage(theApp.GetBitmapPath(_T("design_rules_adjustment")), true), DesignRulesAdjustmentMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Speed Reference..."), ToolImage(theApp.GetBitmapPath(_T("speed_reference")), true), NewSpeedReferenceMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Cutting Rate Reference..."), ToolImage(theApp.GetBitmapPath(_T("cutting_rate")), true), NewCuttingRateMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Positioning Operation..."), ToolImage(theApp.GetBitmapPath(_T("locating")), true), NewPositioningOpMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Probe Centre Operation..."), ToolImage(theApp.GetBitmapPath(_T("probe")), true), NewProbe_Centre_MenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Probe Edge Operation..."), ToolImage(theApp.GetBitmapPath(_T("probe")), true), NewProbe_Edge_MenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Probe Grid Operation..."), ToolImage(theApp.GetBitmapPath(_T("probe")), true), NewProbe_Grid_MenuCallback);

    // Tapping tools menu
	wxMenu *menuTappingTools = new wxMenu;
	heeksCAD->AddMenuItem(menuTappingTools, _("Any sized tap..."), ToolImage(theApp.GetBitmapPath(_T("tap")), true), NewTapToolMenuCallback);
	heeksCAD->AddMenuItem(menuTappingTools, _("Pick from Metric standard sizes"), ToolImage(theApp.GetBitmapPath(_T("tap")), true), NewMetricTappingToolMenuCallback);
	heeksCAD->AddMenuItem(menuTappingTools, _("Pick from Unified Threading Standard (UNC, UNF or UNEF)"), ToolImage(theApp.GetBitmapPath(_T("tap")), true), NewUnifiedThreadingStandardTappingToolMenuCallback);
	heeksCAD->AddMenuItem(menuTappingTools, _("Pick from British Standard Whitworth standard sizes"), ToolImage(theApp.GetBitmapPath(_T("tap")), true), NewBritishStandardWhitworthTappingToolMenuCallback);

	// Tools menu
	wxMenu *menuTools = new wxMenu;
	heeksCAD->AddMenuItem(menuTools, _("Drill..."), ToolImage(theApp.GetBitmapPath(_T("drill")), true), NewDrillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Centre Drill..."), ToolImage(theApp.GetBitmapPath(_T("centredrill")), true), NewCentreDrillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("End Mill..."), ToolImage(theApp.GetBitmapPath(_T("endmill")), true), NewEndmillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Slot Drill..."), ToolImage(theApp.GetBitmapPath(_T("slotdrill")), true), NewSlotCutterMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Ball End Mill..."), ToolImage(theApp.GetBitmapPath(_T("ballmill")), true), NewBallEndMillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Chamfer Mill..."), ToolImage(theApp.GetBitmapPath(_T("chamfmill")), true), NewChamferMenuCallback);
#ifndef STABLE_OPS_ONLY
	heeksCAD->AddMenuItem(menuTools, _("Turning Tool..."), ToolImage(theApp.GetBitmapPath(_T("turntool")), true), NewTurningToolMenuCallback);
#endif
	heeksCAD->AddMenuItem(menuTools, _("Touch Probe..."), ToolImage(theApp.GetBitmapPath(_T("probe")), true), NewTouchProbeMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Tool Length Switch..."), ToolImage(theApp.GetBitmapPath(_T("probe")), true), NewToolLengthSwitchMenuCallback);
#ifndef STABLE_OPS_ONLY
	heeksCAD->AddMenuItem(menuTools, _("Extrusion..."), ToolImage(theApp.GetBitmapPath(_T("extrusion")), true), NewExtrusionMenuCallback);
#endif
	heeksCAD->AddMenuItem(menuTools, _("Tap Tool..."), ToolImage(theApp.GetBitmapPath(_T("tap")), true), NULL, NULL, menuTappingTools);

#ifndef STABLE_OPS_ONLY
	// Fixtures menu
	wxMenu *menuFixtures = new wxMenu;
	heeksCAD->AddMenuItem(menuFixtures, _("New Fixture..."), ToolImage(theApp.GetBitmapPath(_T("fixture")), true), NewFixtureMenuCallback);
#endif

	// Machining menu
	wxMenu *menuMachining = new wxMenu;
	heeksCAD->AddMenuItem(menuMachining, _("Add New Milling Operation"), ToolImage(theApp.GetBitmapPath(_T("ops")), true), NULL, NULL, menuMillingOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add New 3D Operation"), ToolImage(theApp.GetBitmapPath(_T("ops")), true), NULL, NULL, menu3dMillingOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add Other Operation"), ToolImage(theApp.GetBitmapPath(_T("ops")), true), NULL, NULL, menuOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add New Tool"), ToolImage(theApp.GetBitmapPath(_T("tools")), true), NULL, NULL, menuTools);
#ifndef STABLE_OPS_ONLY
	heeksCAD->AddMenuItem(menuMachining, _("Fixtures"), ToolImage(theApp.GetBitmapPath(_T("fixtures")), true), NULL, NULL, menuFixtures);
#endif
	heeksCAD->AddMenuItem(menuMachining, _("Make Python Script"), ToolImage(theApp.GetBitmapPath(_T("python")), true), MakeScriptMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Run Python Script"), ToolImage(theApp.GetBitmapPath(_T("runpython")), true), RunScriptMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Post-Process"), ToolImage(theApp.GetBitmapPath(_T("postprocess")), true), PostProcessMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Open NC File"), ToolImage(theApp.GetBitmapPath(_T("opennc")), true), OpenNcFileMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Save NC File"), ToolImage(theApp.GetBitmapPath(_T("savenc")), true), SaveNcFileMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Send to Machine"), ToolImage(theApp.GetBitmapPath(_T("tomachine")), true), SendToMachineMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Open BOM File"), ToolImage(theApp.GetBitmapPath(_T("opennc")), true), OpenBOMFileMenuCallback);
	frame->GetMenuBar()->Append(menuMachining,  _("Machining"));

	// add the program canvas
    m_program_canvas = new CProgramCanvas(frame);
	aui_manager->AddPane(m_program_canvas, wxAuiPaneInfo().Name(_T("Program")).Caption(_T("Program")).Bottom().BestSize(wxSize(600, 200)));

	// add the output canvas
    m_output_canvas = new COutputCanvas(frame);
	aui_manager->AddPane(m_output_canvas, wxAuiPaneInfo().Name(_T("Output")).Caption(_T("Output")).Bottom().BestSize(wxSize(600, 200)));

	bool program_visible;
	bool output_visible;

	config.Read(_T("ProgramVisible"), &program_visible);
	config.Read(_T("OutputVisible"), &output_visible);

	// read other settings
	CNCCode::ReadColorsFromConfig();
	CProfile::ReadFromConfig();
	CPocket::ReadFromConfig();
	CSpeedOp::ReadFromConfig();

	CSendToMachine::ReadFromConfig();
	config.Read(_T("UseClipperNotBoolean"), m_use_Clipper_not_Boolean, false);
	config.Read(_T("UseDOSNotUnix"), m_use_DOS_not_Unix, false);
	aui_manager->GetPane(m_program_canvas).Show(program_visible);
	aui_manager->GetPane(m_output_canvas).Show(output_visible);

	// add tick boxes for them all on the view menu
	wxMenu* window_menu = heeksCAD->GetWindowMenu();
	heeksCAD->AddMenuItem(window_menu, _T("Program"), wxBitmap(), OnProgramCanvas, OnUpdateProgramCanvas, NULL, true);
	heeksCAD->AddMenuItem(window_menu, _T("Output"), wxBitmap(), OnOutputCanvas, OnUpdateOutputCanvas, NULL, true);
	heeksCAD->AddMenuItem(window_menu, _T("Machining"), wxBitmap(), OnMachiningBar, OnUpdateMachiningBar, NULL, true);
	heeksCAD->RegisterHideableWindow(m_program_canvas);
	heeksCAD->RegisterHideableWindow(m_output_canvas);
	heeksCAD->RegisterHideableWindow(m_machiningBar);

	// add object reading functions
	heeksCAD->RegisterReadXMLfunction("Program", CProgram::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("nccode", CNCCode::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Operations", COperations::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tools", CTools::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Profile", CProfile::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Pocket", CPocket::ReadFromXMLElement);
#ifndef STABLE_OPS_ONLY
	heeksCAD->RegisterReadXMLfunction("ZigZag", CZigZag::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Waterline", CWaterline::ReadFromXMLElement);
#endif
	heeksCAD->RegisterReadXMLfunction("Drilling", CDrilling::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Locating", CPositioning::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Positioning", CPositioning::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ProbeCentre", CProbe_Centre::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ProbeEdge", CProbe_Edge::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ProbeGrid", CProbe_Grid::ReadFromXMLElement);
#ifndef STABLE_OPS_ONLY
	heeksCAD->RegisterReadXMLfunction("CounterBore", CCounterBore::ReadFromXMLElement);
#endif
	heeksCAD->RegisterReadXMLfunction("Tool", CTool::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("CuttingTool", CTool::ReadFromXMLElement);
#ifndef STABLE_OPS_ONLY
	heeksCAD->RegisterReadXMLfunction("Fixture", CFixture::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("TurnRough", CTurnRough::ReadFromXMLElement);
#endif
	heeksCAD->RegisterReadXMLfunction("Tapping", CTapping::ReadFromXMLElement);

	heeksCAD->RegisterReadXMLfunction("SpeedReferences", CSpeedReferences::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("SpeedReference", CSpeedReference::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("CuttingRate", CCuttingRate::ReadFromXMLElement);
#ifndef STABLE_OPS_ONLY
	heeksCAD->RegisterReadXMLfunction("Fixtures", CFixtures::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Chamfer", CChamfer::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Contour", CContour::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Inlay", CInlay::ReadFromXMLElement);
#endif
	heeksCAD->RegisterReadXMLfunction("Tags", CTags::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tag", CTag::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ScriptOp", CScriptOp::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("AttachOp", CAttachOp::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("UnattachOp", CUnattachOp::ReadFromXMLElement);

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
#ifndef STABLE_OPS_ONLY
	{
        std::list<wxString> file_extensions;
        file_extensions.push_back(_T("fixture"));
        file_extensions.push_back(_T("fixtures"));
        if (! heeksCAD->RegisterFileOpenHandler( file_extensions, ImportFixturesFile ))
        {
            printf("Failed to register handler for Fixture files\n");
        }
	}
#endif
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
	{
        std::list<wxString> file_extensions;
        file_extensions.push_back(_T("speed"));
        file_extensions.push_back(_T("speeds"));
        file_extensions.push_back(_T("feed"));
        file_extensions.push_back(_T("feeds"));
        file_extensions.push_back(_T("feedsnspeeds"));
        if (! heeksCAD->RegisterFileOpenHandler( file_extensions, ImportSpeedReferencesFile ))
        {
            printf("Failed to register handler for Speed References files\n");
        }
	}

	heeksCAD->RegisterUnitsChangeHandler( UnitsChangedHandler );
	heeksCAD->RegisterHeeksTypesConverter( HeeksCNCType );
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
		heeksCAD->GetMainObject()->Add(m_program, NULL);
		theApp.m_program_canvas->Clear();
		theApp.m_output_canvas->Clear();
		heeksCAD->Changed();

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

				if ((speed_references_found == false) && (lowercase_file_name.Find(_T("speed")) != -1))
				{
					printf("Importing data from %s\n",  Ttc(l_itFile->c_str()));
					heeksCAD->OpenXMLFile( l_itFile->c_str(), theApp.m_program->SpeedReferences() );
					heeksCAD->Changed();
					speed_references_found = true;
				} // End if - then
				else if ((speed_references_found == false) && (lowercase_file_name.Find(_T("feed")) != -1))
				{
					printf("Importing data from %s\n",  Ttc(l_itFile->c_str()));
					heeksCAD->OpenXMLFile( l_itFile->c_str(), theApp.m_program->SpeedReferences() );
					heeksCAD->Changed();
					speed_references_found = true;
				} // End if - then
				else if ((tool_table_found == false) && (lowercase_file_name.Find(_T("tool")) != -1))
				{
					printf("Importing data from %s\n",  Ttc(l_itFile->c_str()));
					heeksCAD->OpenXMLFile( l_itFile->c_str(), theApp.m_program->Tools() );
					heeksCAD->Changed();
					tool_table_found = true;
				}
#ifndef STABLE_OPS_ONLY
				else if ((fixtures_found == false) && (lowercase_file_name.Find(_T("fixture")) != -1))
				{
					printf("Importing data from %s\n",  Ttc(l_itFile->c_str()));
					heeksCAD->OpenXMLFile( l_itFile->c_str(), theApp.m_program->Fixtures() );
					heeksCAD->Changed();
					fixtures_found = true;
				}
#endif
			} // End for
		} // End for
	} // End if - then
}


void CHeeksCNCApp::InitializeProperties()
{
	machining_options.Initialize(_("machining options"), this);
/*
	CNCCode::GetOptions(&(machining_options->m_list));
	CSpeedOp::GetOptions(&(machining_options->m_list));
	CProfile::GetOptions(&(machining_options->m_list));
*/
	CContour::max_deviation_for_spline_to_arc.Initialize( _("Contour spline deviation"), &machining_options);
	CInlay::max_deviation_for_spline_to_arc.Initialize(_("Inlay spline deviation"), &machining_options);
	CPocket::max_deviation_for_spline_to_arc.Initialize(_("Pocket spline deviation"), &machining_options);
/*
	CSendToMachine::GetOptions(&(machining_options->m_list));
*/
	m_use_Clipper_not_Boolean.Initialize(_("Use Clipper not Boolean"), &machining_options);
	m_use_DOS_not_Unix.Initialize(_("Use DOS Line Endings"), &machining_options);

	excellon_options.Initialize(_("Excellon options"), this);
	Excellon::s_allow_dummy_tool_definitions.Initialize(_("Allow dummy tool definitions"), &excellon_options);
}

void CHeeksCNCApp::GetOptions(std::list<Property *> *list){
}

void CHeeksCNCApp::OnFrameDelete()
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	CNCConfig config(ConfigScope());
	config.Write(_T("ProgramVisible"), aui_manager->GetPane(m_program_canvas).IsShown());
	config.Write(_T("OutputVisible"), aui_manager->GetPane(m_output_canvas).IsShown());
	config.Write(_T("MachiningBarVisible"), aui_manager->GetPane(m_machiningBar).IsShown());

	CNCCode::WriteColorsToConfig();
	CProfile::WriteToConfig();
	CPocket::WriteToConfig();
	CSpeedOp::WriteToConfig();
	CSendToMachine::WriteToConfig();
	config.Write(_T("UseClipperNotBoolean"), m_use_Clipper_not_Boolean);
    config.Write(_T("UseDOSNotUnix"), m_use_DOS_not_Unix);
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
    case ProgramType:       return(_("Program"));
	case NCCodeBlockType:       return(_("NCCodeBlock"));
	case NCCodeType:       return(_("NCCode"));
	case OperationsType:       return(_("Operations"));
	case ProfileType:       return(_("Profile"));
	case PocketType:       return(_("Pocket"));
	case ZigZagType:       return(_("ZigZag"));
	case DrillingType:       return(_("Drilling"));
	case ToolType:       return(_("Tool"));
	case ToolsType:       return(_("Tools"));
	case CounterBoreType:       return(_("CounterBore"));
	case TurnRoughType:       return(_("TurnRough"));
	case FixtureType:       return(_("Fixture"));
	case FixturesType:       return(_("Fixtures"));
	case SpeedReferenceType:       return(_("SpeedReference"));
	case SpeedReferencesType:       return(_("SpeedReferences"));
	case CuttingRateType:       return(_("CuttingRate"));
	case PositioningType:       return(_("Positioning"));
	case BOMType:       return(_("BOM"));
	case TrsfNCCodeType:      return(_("TrsfNCCode"));
	case ProbeCentreType:       return(_("ProbeCentre"));
	case ProbeEdgeType:       return(_("ProbeEdge"));
	case ContourType:       return(_("Contour"));
	case ChamferType:       return(_("Chamfer"));
	case InlayType:       return(_("Inlay"));
	case ProbeGridType:       return(_("ProbeGrid"));
	case TagsType:       return(_("Tags"));
	case TagType:       return(_("Tag"));
	case ScriptOpType:       return(_("ScriptOp"));
	case AttachOpType:       return(_("AttachOp"));
	case UnattachOpType:       return(_("UnattachOp"));
	case WaterlineType:       return(_("Waterline"));
	case TappingType:       return(_("Tapping"));

	default:
        return(_T("")); // Indicates that this function could not make the conversion.
    } // End switch
} // End HeeksCNCType() routine
