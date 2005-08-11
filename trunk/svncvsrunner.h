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



#ifndef SVNCVSRUNNER_H
#define SVNCVSRUNNER_H


#include <wx.h>

#include <wx/progdlg.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/stream.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/regex.h>

#include "log.h"




class SVNRunner : public ToolRunner
{
    wxString username;
    wxString password;
    bool do_force;
    bool remoteStatusHandler;
    
public:
    SVNRunner(const wxString& executable) : do_force(false), remoteStatusHandler(false)
    {
        ToolRunner::runnerType = ToolRunner::SVN;
        SetExecutable(executable);
    }
    ;
    
    virtual ~SVNRunner()
    {}
    ;
    
    void SetPassword(const wxString& user, const wxString& pass)
    {
        username = user;
        password = pass;
    };
    
    void Force() // work around "file has local modifications, use --force switch" error
    {
        do_force = true;
    };
    
    int    Checkout(const wxString& repo, const wxString& dir, const wxString& revision, bool noExternals = false);
    int    Import(const wxString& repo, const wxString& dir, const wxString &message);
    
    int    Status(const wxString& file, bool minusU = false);
    int    Update(const wxString& file, const wxString& revision = wxString("HEAD"));
    int    Commit(const wxString& selected, const wxString& message, bool safeCast = false);
    
    int    Move(const wxString& selected, const wxString& to);
    int    Add(const wxString& selected);
    int    Delete(const wxString& selected);
    
    int    Lock(const wxString& selected, bool force = false);
    int    UnLock(const wxString& selected, bool force = false);
    
    wxString  Diff(const wxString& selected, const wxString& rev);
    
    int     Revert(const wxString& file);
    int     Resolved(const wxString& file);
    
    wxString  PropGet(const wxString& file, const wxString& prop);
    int     PropSet(const wxString& file, const wxString& prop, const wxString& value, bool recursive);
    int    PropDel(const wxString& file, const wxString& prop);
    int    Info(const wxString& file, bool minusR);
    wxString  Info(const wxString& file);
    void Export(const wxString& src, const wxString& dest, const wxString& rev = wxString("HEAD"), const wxString why = wxString("diff"));
    void ExportToTemp(const wxString& src, const wxString& rev = wxString("HEAD"), const wxString why = wxString("diff"));
    
    wxArrayString GetPropertyList(const wxString& file);
    
    virtual int  Run(wxString cmd);
    virtual void OutputHandler();
    void RemoteStatusHandler();
    
    void DumpErrors()
    {
        for(unsigned int i = 0; i < std_err.Count(); ++i)
            Log::Instance()->Add(std_err[i]);
    };
    
    void EnableRemoteStatusHandler()
    {
        remoteStatusHandler = true;
    };
    
private:

}
;

















class CVSRunner : public ToolRunner
{
public:
    CVSRunner(const wxString& executable)
    {
        ToolRunner::runnerType = ToolRunner::CVS;
        SetExecutable(executable);
    }
    ;
    
    virtual ~CVSRunner()
    {}
    ;
    
    void CVSRunner::Login(const wxString& proto, const wxString& repo, const wxString& user, const wxString& pass)
    {
        // cvs -d :pserver:bach:p4ss30rd@faun.example.org:/usr/local/cvsroot login
        SetTarget("");
        SetCommand(wxString("CVS-login"));
        wxString cmd("-z6 -d " + proto + user + ":" + pass + "@" + repo + " login");
        RunBlocking(cmd);
    };
    
    void CVSRunner::Checkout(const wxString& proto, const wxString& repo, const wxString& module, const wxString& workingdir, const wxString& user, const wxString& revision)
    {
        SetCommand(wxString("checkout"));
        SetTarget(workingdir);
        wxString cmd("-z6 -d " + proto + user + "@" + repo + " checkout -d" + Q(workingdir)+ (revision.IsEmpty() ? "" : " -r" + Q(revision)) + module);
        Log::Instance()->Add("cvs " + cmd);
        Run(cmd);
    };
    
    void CVSRunner::Update(const wxString& target, const wxString& revision, const wxString& date)
    {
        SetCommand(wxString("CVS-update"));
        SetTarget(target);
        wxFileName fn(target);
        wxString file = wxDirExists(target) ? "" : Q(fn.GetFullName());
        wxString cmd(" -z6 update " + file + (revision.IsEmpty() ? "" : " -r" + Q(revision)) + (revision.IsEmpty() ? "" : " -D" + Q(date)));
        Run(cmd, fn.GetPath(wxPATH_GET_VOLUME));
    };
    
    void CVSRunner::Commit(const wxString& target, const wxString& message)
    {
        SetCommand(wxString("CVS-commit"));
        SetTarget(target);
        TempFile msg(message);
        wxFileName fn(target);
        wxString file = wxDirExists(target) ? "" : Q(fn.GetFullName());
        wxString cmd(" -z6 commit " + file + " -F " + Q(msg.name));
        Run(cmd, fn.GetPath(wxPATH_GET_VOLUME));
    };
    
    void CVSRunner::Diff(const wxString& target)
    {
        SetCommand(wxString("CVS-diff"));
        SetTarget(target);
        wxFileName fn(target);
        wxString file = Q(fn.GetFullName());
        wxString cmd(" -z6 diff " + file);
        Run(cmd, fn.GetPath(wxPATH_GET_VOLUME));
    };

   
    //    int    CVSRunner::Import(const wxString& repo, const wxString& dir, const wxString &message);
    //    int    CVSRunner::Add(const wxString& selected);
    //    int    CVSRunner::Delete(const wxString& selected);
    
private:
}
;




#endif


