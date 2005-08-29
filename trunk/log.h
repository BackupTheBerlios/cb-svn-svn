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
#include "svnlog.h"

#include <manager.h>
#include <messagemanager.h>

class Log : public Singleton<Log>
{
    friend class Singleton<Log>;
    
    SVNLog *m_log;
    int    index;
    
private:
    Log();
    ~Log();
public:
    void Add(wxString str);
    void Red(wxString str);
    void Blue(wxString str);
    void Grey(wxString str);
    void fg()
    {
        Manager::Get()->GetMessageManager()->SwitchTo(index);
    };
    void Reduce()
    {
        wxTextCtrl *t = m_log->GetTextControl();
        int pos;
        
        if(t->GetNumberOfLines() > 100)
            pos = t->XYToPosition(0, 50);
		else
            pos = t->XYToPosition(0, 10);
		
        t->Remove(0, pos);
        lastLogTime = wxGetLocalTime();
    };
    static int lastLogTime;
};

#endif
