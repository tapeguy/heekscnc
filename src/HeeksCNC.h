// HeeksCNC.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

// defines global variables and functions

#pragma once

#include "interface/HeeksObj.h"
#include "interface/HeeksCADInterface.h"
#include "interface/PropertyList.h"
#include "CNCPoint.h"
#include "PythonString.h"
#include <list>
#include <wx/string.h>

extern CHeeksCADInterface* heeksCAD;

class Property;
class CProgram;
class CProgramCanvas;
class COutputCanvas;
class CPrintCanvas;
class Tool;
class CSurface;

class CHeeksCNCApp : public DomainObject {
public:
	bool m_draw_cutter_radius; // applies to all operations
	CProgram* m_program;
	CProgramCanvas* m_program_canvas;
	COutputCanvas* m_output_canvas;
	CPrintCanvas* m_print_canvas;
	bool m_run_program_on_new_line;
	wxAuiToolBar* m_machiningBar;
	wxMenu *m_menuMachining;
	bool m_machining_hidden;
	wxString m_dll_path;
	int m_icon_texture_number;
	std::list< void(*)() > m_OnRewritePython_list;
	std::set<int> m_external_op_types;

	PropertyList machining_options;
	PropertyList nc_options;
	PropertyList text_colors;
	PropertyCheck m_use_Clipper_not_Boolean;
	PropertyCheck m_use_DOS_not_Unix;
	PropertyList excellon_options;

	CSurface* m_attached_to_surface;
	int  m_tool_number;
	CNCPoint m_location;
	bool m_settings_restored;

	CHeeksCNCApp();
	~CHeeksCNCApp();

	Python SetTool( const int new_tool );

	void InitializeProperties();
	void OnStartUp(CHeeksCADInterface* h, const wxString& dll_path);
	void OnNewOrOpen(bool open, int res);
	void OnInitDLL();
	void OnDestroyDLL();
	void GetProperties(std::list<Property *> *list);
	void OnFrameDelete();
	wxString GetDllFolder();
	wxString GetResFolder();
	wxString GetBitmapPath(const wxString& name);
	wxString GetDialogBitmapPath(const wxString& name, const wxString& folder);
	void RunPythonScript();

	typedef int SymbolType_t;
	typedef unsigned int SymbolId_t;
	typedef std::pair< SymbolType_t, SymbolId_t > Symbol_t;
	typedef std::list< Symbol_t > Symbols_t;

	std::list<wxString> GetFileNames( const char *p_szRoot ) const;
    static void GetNewToolTools(std::list<Tool*>* t_list);
    static void GetNewPatternTools(std::list<Tool*>* t_list);
    static void GetNewSurfaceTools(std::list<Tool*>* t_list);
    static void GetNewOperationTools(std::list<Tool*>* t_list);
    static void GetNewStockTools(std::list<Tool*>* t_list);

	wxString ConfigScope() const { return(_("Program")); }
};

extern CHeeksCNCApp theApp;

