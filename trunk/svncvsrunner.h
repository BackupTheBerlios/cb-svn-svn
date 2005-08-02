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
    bool prune_non_interactive;
    bool block;
    
public:
    SVNRunner(const wxString& executable) : block(false), do_force(false)
    {
        ToolRunner::runnerType = ToolRunner::SVN;
        SetExecutable(executable);
    }
    ;
    
    virtual ~SVNRunner()
    {}
    ;
    
    void SVNRunner::SetPassword(const wxString& user, const wxString& pass)
    {
        username = user;
        password = pass;
    };
    
    void NoInteractive() // work around "subcommand does not support --non-interactive" error
    {
        prune_non_interactive = true;
    };
    
    void Force() // work around "file has local modifications, use --force switch" error
    {
        do_force = true;
    };
    
    int    SVNRunner::Checkout(const wxString& repo, const wxString& dir, const wxString& revision, bool noExternals = false);
    int    SVNRunner::Import(const wxString& repo, const wxString& dir, const wxString &message);
    
    int    SVNRunner::Status(const wxString& file, bool minusU = false);
    int    SVNRunner::Update(const wxString& file, const wxString& revision = wxString("HEAD"));
    int    SVNRunner::Commit(const wxString& selected, const wxString& message);
    
    int    SVNRunner::Move(const wxString& selected, const wxString& to);
    int    SVNRunner::Add(const wxString& selected);
    int    SVNRunner::Delete(const wxString& selected);
    
    wxString  SVNRunner::Cat(const wxString& selected, const wxString& rev);
    wxString  SVNRunner::Diff(const wxString& selected, const wxString& rev);
    
    int     SVNRunner::Revert(const wxString& file);
    
    wxString  SVNRunner::PropGet(const wxString& file, const wxString& prop);
    int     SVNRunner::PropSet(const wxString& file, const wxString& prop, const wxString& value, bool recursive);
    int    SVNRunner::PropDel(const wxString& file, const wxString& prop);
    int    SVNRunner::Info(const wxString& file, bool minusR);
    wxString  SVNRunner::Info(const wxString& file);
    
    wxArrayString SVNRunner::GetPropertyList(const wxString& file);
    
    virtual int  SVNRunner::Run(wxString cmd);
    virtual void OutputHandler();
    
    void   SVNRunner::DumpErrors()
    {
        for(unsigned int i = 0; i < std_err.Count(); ++i)
            Log::Instance()->Add(std_err[i]);
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
        wxString cmd("-d " + proto + user + ":" + pass + "@" + repo + " login");
        RunBlocking(cmd);
    };
    
    void CVSRunner::Checkout(const wxString& proto, const wxString& repo, const wxString& module, const wxString& workingdir, const wxString& user, const wxString& revision)
    {
        SetTarget(workingdir);
        wxString cmd("-d " + proto + user + "@" + repo + " checkout -d" + Q(workingdir)+ (revision.IsEmpty() ? "" : " -r" + Q(revision)) + module);
        Log::Instance()->Add("cvs " + cmd);
        Run(cmd);
    };
    
    void CVSRunner::Update(const wxString& target, const wxString& revision, const wxString& date)
    {
        SetTarget(target);
        wxFileName fn(target);
        wxFileName::SetCwd(fn.GetPath(wxPATH_GET_VOLUME));
        wxString file = wxDirExists(target) ? "" : Q(fn.GetFullName());
        wxString cmd(" update " + file + (revision.IsEmpty() ? "" : " -r" + Q(revision)) + (revision.IsEmpty() ? "" : " -D" + Q(date)));
        Run(cmd);
    };
    
    void CVSRunner::Commit(const wxString& target, const wxString& message)
    {
        SetTarget(target);
		TempFile msg(message);
        wxFileName fn(target);
        wxFileName::SetCwd(fn.GetPath(wxPATH_GET_VOLUME));
        wxString file = wxDirExists(target) ? "" : Q(fn.GetFullName());
        wxString cmd(" commit " + file + " -F " + Q(msg.name));
        Run(cmd);
    };
    
    
    //    int    CVSRunner::Import(const wxString& repo, const wxString& dir, const wxString &message);
    //    int    CVSRunner::Add(const wxString& selected);
    //    int    CVSRunner::Delete(const wxString& selected);
    
private:
}
;




#endif


