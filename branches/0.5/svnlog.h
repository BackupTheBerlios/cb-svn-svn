/*
* This file is part of the Code::Blocks SVN Plugin
* Copyright (C) 2005 Thomas Denk
*
* This program is licensed under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2 of the License,
* or (at your option) any later version.
*
* $HeadURL$
* $Id$
*/
#ifndef NOTSOSIMPLETEXTLOG_H
#define NOTSOSIMPLETEXTLOG_H

#include "settings.h"
#include "simpletextlog.h"
#include <wx/textctrl.h>


class SVNLog : public SimpleTextLog
{
	public:
		SVNLog(wxNotebook* parent, const wxString& title, wxBitmap *skull = 0);
		~SVNLog();
		void Forward(wxCommandEvent& event);
private:
    DECLARE_EVENT_TABLE()
};

#endif

