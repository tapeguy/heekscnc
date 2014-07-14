// Reselect.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Reselect.h"
#include "interface/ObjList.h"

static bool GetSketches(std::list<int>& sketches )
{
	// check for at least one sketch selected

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SketchType)
		{
			sketches.push_back(object->GetID());
		}
	}

	if(sketches.size() == 0)return false;

	return true;
}

void ReselectSketches::Run()
{
	std::list<int> sketches;
    MarkingFilter filters[] = { SketchMarkingFilter };
    std::set<MarkingFilter> filterset ( filters, filters + sizeof(filters) / sizeof(MarkingFilter));
	heeksCAD->PickObjects(_("Select Sketches"), filterset);
	if(GetSketches( sketches ))
	{
		heeksCAD->CreateUndoPoint();
		m_sketches->clear();
		*m_sketches = sketches;
		((ObjList*)m_object)->Clear();
		m_object->ReloadPointers();
		heeksCAD->Changed();
	}
	else
	{
		wxMessageBox(_("Select cancelled. No sketches were selected!"));
	}

	// get back to the operation's properties
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(m_object);
}

static bool GetSolids(std::list<int>& solids )
{
	// check for at least one sketch selected

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SolidType || object->GetType() == StlSolidType)
		{
			solids.push_back(object->GetID());
		}
	}

	if(solids.size() == 0)return false;

	return true;
}

void ReselectSolids::Run()
{
	std::list<int> solids;
	MarkingFilter filters[] = { SolidMarkingFilter, StlSolidMarkingFilter };
	std::set<MarkingFilter> filterset (filters, filters + sizeof(filters) / sizeof(MarkingFilter));
	heeksCAD->PickObjects(_("Select Solids"), filterset );
	if(GetSolids( solids ))
	{
		heeksCAD->CreateUndoPoint();
		m_solids->clear();
		*m_solids = solids;
		((ObjList*)m_object)->Clear();
		m_object->ReloadPointers();
		heeksCAD->Changed();
	}
	else
	{
		wxMessageBox(_("Select cancelled. No solids were selected!"));
	}

	// get back to the operation's properties
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(m_object);
}

wxString GetIntListString(const std::list<int> &list)
{
	wxString str;
	int i = 0;
	for(std::list<int>::const_iterator It = list.begin(); It != list.end(); It++, i++)
	{
		if(i > 0)str.Append(_T(" "));
		if(i > 8)
		{
			str.Append(_T("..."));
			break;
		}
		str.Append(wxString::Format(_T("%d"), *It));
	}

	return str;
}

#ifdef OP_SKETCHES_AS_CHILDREN
void AddSolidsProperties(std::list<Property *> *list, HeeksObj* object)
{
	std::list<int> solids;
	for(HeeksObj* child = object->GetFirstChild(); child; child = object->GetNextChild())
	{
		if(child->GetIDGroupType() == SolidType)solids.push_back(child->GetID());
	}
#else
void AddSolidsProperties(std::list<Property *> *list, const std::list<int> &solids)
{
#endif

    Property * prop;
	if(solids.size() == 0)
	{
	    prop = new PropertyString(_("solids"), _("Solids"), NULL);
	    ((PropertyString *)prop)->SetValue ( _("None") );
	}
	else if(solids.size() == 1)
	{
	    prop = new PropertyInt(_("solids"), _("Solids"), NULL);
	    ((PropertyInt *)prop)->SetValue ( solids.front() );
	}
	else
	{
	    prop = new PropertyString(_("solids"), _("Solids"), NULL);
	    ((PropertyString *)prop)->SetValue ( GetIntListString(solids) );
	}
	list->push_back(prop);
}

#ifdef OP_SKETCHES_AS_CHILDREN
void AddSketchesProperties(std::list<Property *> *list, HeeksObj* object)
{
	std::list<int> sketches;
	for(HeeksObj* child = object->GetFirstChild(); child; child = object->GetNextChild())
	{
		if(child->GetIDGroupType() == SketchType)sketches.push_back(child->GetID());
	}
#else
void AddSketchesProperties(std::list<Property *> *list, const std::list<int> &sketches)
{
#endif
    Property * prop;

	if(sketches.size() == 0)
    {
	    prop = new PropertyString(_("sketches"), _("Sketches"), NULL);
	    ((PropertyString *)prop)->SetValue ( _("None") );
    }
	else if(sketches.size() == 1)
    {
	    prop = new PropertyInt(_("sketches"), _("Sketches"), NULL);
        ((PropertyInt *)prop)->SetValue ( sketches.front() );
    }
    else
    {
        prop = new PropertyString(_("sketches"), _("Sketches"), NULL);
        ((PropertyString *)prop)->SetValue ( GetIntListString(sketches) );
    }
    list->push_back(prop);
}

