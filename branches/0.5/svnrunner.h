// This file is part of the Code::Blocks SVN Plugin
// Copyright (C) 2005 Thomas Denk
//
// This program is licensed under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// $HeadURL$
// $Id$


#ifndef SVNRUNNER_H
#define SVNRUNNER_H


#include <wx/wx.h>

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
//    wxString username;
//    wxString password;
    bool do_force;
    bool remoteStatusHandler;

public:
    SVNRunner(const wxString& executable) : do_force(false), remoteStatusHandler(false)
    {
        runnerType = SVN;
        SetExecutable(executable);
    }
    ;

    virtual ~SVNRunner()
    {}
    ;

//    void SetPassword(const wxString& user, const wxString& pass)
//    {
//        username = user;
//        password = pass;
//    };

    void Force() // work around "file has local modifications, use --force switch" error
    {
        do_force = true;
    };

    void   Checkout(const wxString& repo, const wxString& dir, const wxString& revision, bool noExternals = false);
    void   Import(const wxString& repo, const wxString& dir, const wxString &message);

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

#endif

