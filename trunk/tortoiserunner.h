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

#ifndef TORTOISERUNNER_H
#define TORTOISERUNNER_H


#include <wx.h>

#include <wx/progdlg.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/stream.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/regex.h>

#include "log.h"




class TortoiseRunner : public ToolRunner
{
public:
    TortoiseRunner(const wxString& executable)
    {
        SetExecutable(executable);
    }
    ;
    
    virtual ~TortoiseRunner()
    {}
    ;
    
    virtual int TortoiseRunner::Run(wxString cmd)
    {
#ifdef LOTS_OF_DEBUG_OUTPUT
        Log::Instance()->Add(exec + " " + cmd);
#endif
        
        ToolRunner::RunBlind(cmd);
    };
    
    
    void TortoiseRunner::Branch(const wxString& path)
    {
        wxString cmd("/command:copy /path:\"");
        cmd << path << "\" /notempfile /closeonend";
        Run(cmd);
    }
    
    
    void TortoiseRunner::Switch(const wxString& path)
    {
        wxString cmd("/command:switch /path:\"");
        cmd << path << "\" /notempfile /closeonend";
        Run(cmd);
    }
    
    void TortoiseRunner::Merge(const wxString& path)
    {
        wxString cmd("/command:merge /path:\"");
        cmd << path << "\" /notempfile /closeonend";
        Run(cmd);
    }
    void TortoiseRunner::Relocate(const wxString& path)
    {
        wxString cmd("/command:relocate /path:\"");
        cmd << path << "\" /notempfile /closeonend";
        Run(cmd);
    }
    void TortoiseRunner::Create(const wxString& path)
    {
        wxString cmd("/command:repocreate /path:\"");
        cmd << path << "\" /notempfile /closeonend";
        Run(cmd);
    }
    void TortoiseRunner::ConflictEditor(const wxString& path)
    {
        wxString cmd("/command:conflicteditor /path:\"");
        cmd << path << "\" /notempfile /closeonend";
        Run(cmd);
    }
    void TortoiseRunner::StatusDialog(const wxString& path)
    {
        wxString cmd("/command:repostatus /path:\"");
        cmd << path << "\" /notempfile /closeonend";
        Run(cmd);
    }
    
};









class TortoiseCVSRunner : public ToolRunner
{
public:
    TortoiseCVSRunner(const wxString& executable)
    {
        SetExecutable(executable);
    }
    ;
    
    virtual ~TortoiseCVSRunner()
    {}
    ;
    
    virtual int TortoiseCVSRunner::Run(wxString cmd)
    {
#ifdef LOTS_OF_DEBUG_OUTPUT
        Log::Instance()->Add(exec + " " + cmd);
#endif
        
        ToolRunner::RunBlind(cmd);
    };
    
    
    void TortoiseCVSRunner::Branch(const wxString& path)
    {
        wxString cmd("cvsbranch -l ");
        cmd << Q(path);
        Run(cmd);
    }
    void TortoiseCVSRunner::Tag(const wxString& path)
    {
        wxString cmd("cvstag -l ");
        cmd << Q(path);
        Run(cmd);
    }
    void TortoiseCVSRunner::Merge(const wxString& path)
    {
        wxString cmd("cvsmerge -l ");
        cmd << Q(path);
        Run(cmd);
    }
    void TortoiseCVSRunner::Patch(const wxString& path)
    {
        wxString cmd("CVSMakePatch -l ");
        cmd << Q(path);
        Run(cmd);
    }
};



#endif


