// This file is part of the Code::Blocks SVN Plugin
// Copyright (C) 2005 Thomas Denk
//
// This program is licensed under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// $HeadURL$
// $Id$


#ifndef TORTOISERUNNER_H
#define TORTOISERUNNER_H

#include <wx/wx.h>

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

    wxString QT(const wxString& s)
    {
      return(Q(s).Mid(1));
    }

    virtual ~TortoiseRunner()
    {}
    ;

    virtual int TortoiseRunner::Run(wxString cmd)
    {
      Log::Instance()->Add(cmd);
      ToolRunner::RunBlind(cmd);
      return 0;
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
    void TortoiseRunner::Diff(const wxString& path1, const wxString& path2)
    {
      wxString cmd("/command:diff /path:" + QT(path1) + "/path2:" + QT(path2));
      cmd << "\" /notempfile /closeonend";
      Run(cmd);
    }
    void TortoiseRunner::Patch(const wxString& path)
    {
      wxString cmd("/command:createpatch /path:" + QT(path));
      cmd << "\" /notempfile /closeonend";
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
      ToolRunner::RunBlind(cmd);
      return 0;
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










class DiffRunner : public ToolRunner
  {
  public:
    DiffRunner(const wxString& executable)
    {
      SetExecutable(executable);
      if(executable.Contains("tkdiff.tcl"))
        runnerType = TKDIFF;
      else
        runnerType = DIFF3;
    };

    virtual ~DiffRunner()
  {}
    ;

    virtual int Run(wxString cmd)
    {
      ToolRunner::RunBlind(cmd);
      return 0;
    };


    void Diff(const wxString& path1, const wxString& path2)
    {
      wxString cmd(Q(path1) + Q(path2));
      Run(cmd);
    }

    void Merge(const wxString& path1, const wxString& path2, const wxString& path3)
    {
      wxString cmd("-m" + Q(path1) + Q(path2) + "-o" + Q(path3));
      Run(cmd);
    }

    void Merge(const wxString& path, const wxString xCommand = wxString("-conflict"))
    {
      wxString cmd(xCommand + Q(path));
      Run(cmd);
    }

  };


#endif









