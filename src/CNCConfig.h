// CNCConfig.h

#pragma once

#include <wx/fileconf.h>
#include "interface/HeeksConfig.h"


class CNCConfig : public HeeksConfig
{
public:
	CNCConfig(const wxString scope = _T("") ) : HeeksConfig(_T("HeeksCNC")), m_scope(scope)
	{
	}

public:
	bool Read(const wxString& key, wxString *pStr)
	{
		return(HeeksConfig::Read(VerboseKey(key), pStr));
	}
	bool Read(const wxString& key, int *pi)
	{
		return(HeeksConfig::Read(VerboseKey(key), pi));
	}
	bool Read(const wxString& key, long *pl)
	{
		return(HeeksConfig::Read(VerboseKey(key), pl));
	}
	bool Read(const wxString& key, double* val)
	{
		return(HeeksConfig::Read(VerboseKey(key), val));
	}
	bool Read(const wxString& key, bool* val)
	{
		return(HeeksConfig::Read(VerboseKey(key), val));
	}

	bool Read(const wxString& key, wxString *pStr, const wxString& defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), pStr, defVal));
	}
	bool Read(const wxString& key, int *pi, int defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), pi, defVal));
	}
	bool Read(const wxString& key, long *pl, long defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), pl, defVal));
	}
	bool Read(const wxString& key, double* val, double defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), val, defVal));
	}
	bool Read(const wxString& key, bool* val, bool defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), val, defVal));
	}


	bool Read(const wxString& key, PropertyCheck& prop)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop));
	}
	bool Read(const wxString& key, PropertyChoice& prop)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop));
	}
	bool Read(const wxString& key, PropertyDouble& prop)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop));
	}
	bool Read(const wxString& key, PropertyInt& prop)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop));
	}
	bool Read(const wxString& key, PropertyString& prop)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop));
	}
	bool Read(const wxString& key, PropertyFile& prop)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop));
	}
	bool Read(const wxString& key, PropertyLength& prop)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop));
	}

	bool Read(const wxString& key, PropertyCheck& prop, bool defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop, defVal));
	}
	bool Read(const wxString& key, PropertyChoice& prop, int defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop, defVal));
	}
	bool Read(const wxString& key, PropertyDouble& prop, double defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop, defVal));
	}
	bool Read(const wxString& key, PropertyInt& prop, int defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop, defVal));
	}
	bool Read(const wxString& key, PropertyString& prop, const wxString& defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop, defVal));
	}
	bool Read(const wxString& key, PropertyFile& prop, const wxString& defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop, defVal));
	}
	bool Read(const wxString& key, PropertyLength& prop, double defVal)
	{
		return(HeeksConfig::Read(VerboseKey(key), prop, defVal));
	}

	// write the value (return true on success)
	bool Write(const wxString& key, const wxString& value)
	{
		return(HeeksConfig::Write(VerboseKey(key), value));
	}

	bool Write(const wxString& key, long value)
	{
		return(HeeksConfig::Write(VerboseKey(key), value));
	}

	bool Write(const wxString& key, int value)
	{
		return(HeeksConfig::Write(VerboseKey(key), value));
	}

	bool Write(const wxString& key, double value)
	{
		return(HeeksConfig::Write(VerboseKey(key), value));
	}

	bool Write(const wxString& key, bool value)
	{
		return(HeeksConfig::Write(VerboseKey(key), value));
	}

	// we have to provide a separate version for C strings as otherwise they
	// would be converted to bool and not to wxString as expected!
	bool Write(const wxString& key, const wxChar *value)
	{
		return(HeeksConfig::Write(VerboseKey(key), value));
	}

private:
    const wxString VerboseKey( const wxString &key ) const
    {
        wxString _key(m_scope);
        if (_key.Length() > 0)
        {
            _key << wxCONFIG_PATH_SEPARATOR;
        }

        _key << key;

        return(_key);
    }

private:
	wxString      m_scope;

};
