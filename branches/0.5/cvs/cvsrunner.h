// This file is part of the Code::Blocks SVN Plugin
// Copyright (C) 2005 Thomas Denk
//
// This program is licensed under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// $HeadURL$
// $Id$


#ifndef CVSRUNNER_H
#define CVSRUNNER_H


#include <wx/wx.h>

#include <wx/progdlg.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/stream.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/regex.h>

#include "log.h"


class CVSRunner : public ToolRunner
{
public:
    CVSRunner(const wxString& executable)
    {
        ToolRunner::runnerType = CVS;
        SetExecutable(executable);
    }
    ;

    virtual ~CVSRunner()
    {}
    ;

    void Login(const wxString& proto, const wxString& repo, const wxString& user, const wxString& pass);

    void Checkout(const wxString& proto, const wxString& repo, const wxString& module, const wxString& workingdir, const wxString& user, const wxString& revision);

    void Update(const wxString& target, const wxString& revision, const wxString& date);
    void Commit(const wxString& target, const wxString& message);

    void Diff(const wxString& target);

    void Export(const wxString& repo, const wxString& module, const wxString& dir, const wxString& cwd, const wxString& rev);

};


#endif

