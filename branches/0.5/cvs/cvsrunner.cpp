// This file is part of the Code::Blocks SVN Plugin
// Copyright (C) 2005 Thomas Denk
//
// This program is licensed under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// $HeadURL$
// $Id$


#include "precompile.h"



void CVSRunner::Login(const wxString& proto, const wxString& repo, const wxString& user, const wxString& pass)
    {
    // FIXME: this has to be changed to respect new meaning of repo
        // cvs -d :pserver:bach:p4ss30rd@faun.example.org:/usr/local/cvsroot login
        //SetTarget("");
        SetCommand("CVS-login");
        wxString cmd("-z6 -d " + proto + user + ":" + pass + "@" + repo + " login");
        RunBlocking(cmd);
    }

void CVSRunner::Checkout(const wxString& proto, const wxString& repo, const wxString& module, const wxString& workingdir, const wxString& user, const wxString& revision)
    {
        SetCommand("checkout");
        //SetTarget(workingdir);
        wxString cmd("-z6 -d " + repo + " checkout -d" + Q(workingdir)+ (revision.IsEmpty() ? "" : " -r" + Q(revision)) + Q(module));
        Run(cmd);
    }

void CVSRunner::Update(const wxString& target, const wxString& revision, const wxString& date)
    {
        SetCommand("CVS-update");
        SetTarget(target);
        wxFileName fn(target);
        wxString file = wxDirExists(target) ? "" : Q(fn.GetFullName());
        wxString cmd(" -z6 update " + file + (revision.IsEmpty() ? "" : " -r" + Q(revision)) + (date.IsEmpty() ? "" : " -D" + Q(date)));
        Run(cmd, fn.GetPath(wxPATH_GET_VOLUME));
    }

void CVSRunner::Commit(const wxString& target, const wxString& message)
    {
        SetCommand("CVS-commit");
        //SetTarget(target);
        TempFile msg(message);
        wxFileName fn(target);
        wxString file = wxDirExists(target) ? "" : Q(fn.GetFullName());
        wxString cmd(" -z6 commit " + file + " -F " + Q(msg.name));
        Run(cmd, fn.GetPath(wxPATH_GET_VOLUME));
    }

void CVSRunner::Diff(const wxString& target)
    {
        SetCommand("CVS-diff");
        //SetTarget(target);
        wxFileName fn(target);
        wxString file = Q(fn.GetFullName());
        wxString cmd(" -z6 diff " + file);
        Run(cmd, fn.GetPath(wxPATH_GET_VOLUME));
    }


void CVSRunner::Export(const wxString& repo, const wxString& module, const wxString& dir, const wxString& cwd, const wxString& rev)
    {
        SetCommand("export:release");
        //SetTarget("*" + cwd + dir);
        wxString cmd(" -z6 -d " + Q(repo) + "export -f -r" + Q(rev) + "-d" + Q(dir) + Q(module));
        Run(cmd, cwd);
    }


