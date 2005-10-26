// This file is part of the Code::Blocks SVN Plugin
// Copyright (C) 2005 Thomas Denk
//
// This program is licensed under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// $HeadURL$
// $Id$



#ifndef TOOLRUNNER_H
#define TOOLRUNNER_H


#include <wx/wx.h>
#include <wx/file.h>
#include <wx/dir.h>

#include "pipedprocess.h"
#include "sdk_events.h"
#include "manager.h"
#include "pluginmanager.h"
#include "globals.h"

#include "log.h"


#ifdef new
#undef new
#endif
#include <deque>

enum
{
  TRANSACTION_SUCCESS,
  TRANSACTION_FAILURE,
  RUN_AGAIN,
  RUN_NEXT_IN_QUEUE
};

typedef enum
    {
      UNDEFINED,
      SVN,
      CVS,
      DIFF3,
      TKDIFF
    }toolrunner_type;

extern EVTIMPORT const wxEventType EVT_WX_SUCKS;
#define SCREW_THIS_MACRO_ABUSE(id, fn) DECLARE_EVENT_TABLE_ENTRY( EVT_WX_SUCKS, id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) & fn, (wxObject *) NULL ),

class BogusProcess : public wxProcess
  {
  public:
    BogusProcess::BogusProcess()
    {
      Redirect();  // We won't make use of this, but it prevents a DOS window from showing up
      Detach();    // Auto-delete when OnTerminate() returns... hopefully
    }
  };


class Process : public wxProcess
  {
    wxInputStream *stream_stdout;
    wxInputStream *stream_stderr;

    bool running;
  public:
    int exitCode;
    wxArrayString std_out;
    wxArrayString std_err;

    bool Running()
    {
      return running;
    };

    Process::Process() : running(true)
    {
      Redirect();
    };

    void FlushPipe()
    {
      wxString line;
      stream_stdout = GetInputStream();
      stream_stderr = GetErrorStream();
      assert(stream_stdout);
      assert(stream_stderr);
      wxTextInputStream t_stream_stdout(*stream_stdout);
      wxTextInputStream t_stream_stderr(*stream_stderr);

      while(! stream_stdout->Eof() )
        {
          line = t_stream_stdout.ReadLine();
          std_out.Add(line);
        }

      while(! stream_stderr->Eof() )
        {
          line = t_stream_stderr.ReadLine();
          std_err.Add(line);
        }
    };

    virtual void OnTerminate(int pid, int status)
    {
      FlushPipe();
      exitCode = status;
      running = false;
    }
  };


class TempFile
  {
  public:
    wxString name;

    TempFile() : name(wxEmptyString)
    {}

    TempFile(const wxString& comment)
    {
      wxFile f;
      name = wxFileName::CreateTempFileName(wxEmptyString, &f);
      if(!f.Write(comment))
        Log::Instance()->Add("Error: unable to open tempfile.");
    };

    ~TempFile()
    {
      if(!name.IsEmpty())
        ::wxRemoveFile(name);
    }

    static wxString TempFolder()
    {
      TempFile t(wxEmptyString);
      return wxFileName(t.name).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
    };
  };


class Job
  {
  public:
    wxString commandline;
    wxString cwd;
    wxString verb;
    wxString target;
    toolrunner_type  runnerType;
    TempFile *temp;

    Job() : commandline(wxEmptyString), cwd(wxEmptyString), verb(wxEmptyString), target(wxEmptyString), runnerType(UNDEFINED), temp(0)
    {}
    ;

    Job(wxString cmd, wxString dir, wxString the_verb, wxString the_target) : commandline(cmd), cwd(dir), verb(the_verb), target(the_target), runnerType(UNDEFINED), temp(0)
    {}
    ;

    Job(wxString cmd, wxString dir, wxString the_verb, wxString the_target, wxString temp_value) : commandline(cmd), cwd(dir), verb(the_verb), target(the_target), runnerType(UNDEFINED)
    {
    temp = new TempFile(temp_value);
    }
    ;

    Job(wxString cmd, wxString dir, wxString the_verb, wxString the_target, TempFile *temp_file) : commandline(cmd), cwd(dir), verb(the_verb), target(the_target), runnerType(UNDEFINED), temp(temp_file)
    {
    }
    ;

    void SetTarget(wxString t)
    {
      target = t;
    };

    wxString T(wxString str)
    {
      temp = new TempFile(str);
      return temp->name;
    };

    wxString T()
    {
      return temp->name;
    };

    ~Job()
    {
      if(temp)
        delete temp;
    };

    void Clear()
    {
      commandline = cwd = verb = target = "";
    };
  };




class ToolRunner : public wxEvtHandler
  {
  public:

    typedef std::deque<Job> JobQueue;

    wxArrayString std_out;
    wxArrayString std_err;
    wxString  blob; // "blob" concats everything so searching is somewhat easier
    wxString  out; // "out" likewise, but preserving linebreaks
    int     lastExitCode;

    ToolRunner() :  lastExitCode(0), implicit_run(0), exec_error(false)
    {
      timer.SetOwner(this);
      plugin = Manager::Get()->GetPluginManager()->FindPluginByName("svn");
      assert(plugin);
    };

    virtual ~ToolRunner()
    {}
    ;

    void SetExecutable(const wxString& executable)
    {
      exec = executable;
      exec_error = false;
    };

    wxString GetExecutable()
    {
      return exec;
    };

    bool ExecError()
    {
      return exec_error;
    };

    void EmptyQueue()
    {
      jobQueue.clear();
    };

    void Implicit()
    {
      implicit_run  = true;
    };

    void QueueAgain()
    {
      jobQueue.push_front(currentJob);
    };





    toolrunner_type Type()
    {
      return currentJob.runnerType;
    };

    wxString Q(const wxString & in)
    {
      wxString out(" \"");
      out << in << "\" ";
      out.Replace("\\", "/", true);
      return out;
    };

    int ToolRunner::RunBlocking(const wxString& cmd);
    void ToolRunner::RunBlind(const wxString& cmd);

    int ToolRunner::RunJob(Job* theJob);

    void ToolRunner::RunAgain();
    void ToolRunner::RunQueue();

    void OnTimer(wxTimerEvent& event);
    void OnOutput(CodeBlocksEvent& event);
    void OnError(CodeBlocksEvent& event);
    void OnTerminated(CodeBlocksEvent& event);

    virtual void OutputHandler()
    {
      if(lastExitCode)
        Fail();
      else
        Succeed();
    };

    void Fail();
    void Succeed();
    void Send(int cmd);

    bool Running()
    {
      return cb_process || process;
    };

    void Finish()
    {
      while(Running())
        {
          if (cb_process)
            ((PipedProcess*)cb_process)->HasInput();
          wxTheApp->Yield(true);
        }
    };

    bool IsIdle()
    {
      return !Running() && jobQueue.empty();
    };

    void SetCommand(const wxString& c)
    {
      newJob.verb = c;
    };

    void SetPlink(const wxString& c)
    {
      plink = c;
    };

    void Kill(wxSignal signal)
    {
      if(pid)
        wxKill(pid, signal);
    };

    void SaveEnvironment()
    {
      wxGetEnv("LANG", &oldLang);
      wxGetEnv("SVN_SSH", &oldSvnSsh);
      wxGetEnv("CVS_RSH", &oldCvsRsh);
    };

    void SetEnvironment()
    {
      wxSetEnv("LANG", "en");
      if(!plink.IsEmpty())
        {
          wxSetEnv("SVN_SSH", plink);
          wxSetEnv("CVS_RSH", plink);
        }
    };

    void RestoreEnvironment()
    {
      wxSetEnv("LANG", oldLang);
      wxSetEnv("SVN_SSH", oldSvnSsh);
      wxSetEnv("CVS_RSH", oldCvsRsh);
    };

  protected:
    int ToolRunner::RunAsync(const wxString& cmd, const wxString& cwd);
    cbPlugin *plugin;

    JobQueue jobQueue;
    Job  newJob;
    Job  currentJob;

    wxString  exec;
    wxString plink;
    toolrunner_type runnerType;

  private:

    wxString oldLang;
    wxString oldSvnSsh;
    wxString oldCvsRsh;

    wxTimer   timer;
    static PipedProcess *cb_process;
    static Process   *process;
    int    pid;
    bool implicit_run;
    bool exec_error;
    DECLARE_EVENT_TABLE()
  };


#endif

