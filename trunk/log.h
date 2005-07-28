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

#ifndef __LOG_H_
#define __LOG_H_

#include "singleton.h"

#include <wx/wx.h>


#include <manager.h>
#include <messagemanager.h>
#include <simpletextlog.h>


class Log : public Singleton<Log>
{
    friend class Singleton<Log>;
    
    SimpleTextLog *m_log;
    int    index;
    
private:
    Log();
    ~Log();
public:
    void Add(wxString str);
};

#endif
