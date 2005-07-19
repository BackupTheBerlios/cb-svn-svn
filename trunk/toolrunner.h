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



#ifndef TOOLRUNNER_H
#define TOOLRUNNER_H


#include <wx.h>

#include <wx/progdlg.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/stream.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/regex.h>

#include "log.h"


#define LOTS_OF_DEBUG_OUTPUT




class ToolRunner
  {
    /*
    * The implementation of ToolRunner is fucking braindead and inefficient, in particular the spinlock around wxYield()
    * is some nasty stuff. In addition to the SNAFU given by wxWindows, this version also opens and closes several hundred
    * input streams while spinning. I was unable to implement it using only one input stream, as the stream object deletes
    * itself at unpredictable times (apparently when eof is reached?) Some serious rewrite needs to be done there some day.
    * Note: Do not even think about replacing the spinlock with a wxTimer like it is done in the wx sample code and in code::blocks.
    * This is fine if you run a compiler, but not for this purpose here. Remember that svn is invoked at least two times
    * during BuildModuleMenu(). On my machine (Amd64/3500), wxTimer.Start(100) performs closer to 300-350 than to 100ms.
    * With a 600-700ms timer overhead plus 100-250 ms for svn to execute, BuildModuleMenu() would clearly have to
    * be considered non-realtime.
    * Also, do not even think about skipping the spinlock alltogether.
    * Lastly, do not think about using a mutex or semaphore, either. Although this would be the "correct" way, this will
    * not work, as notification is (contrarily to what the docs may make you believe) not asynchronous.
    * Therefore, if you use a mutex or a semaphore, you will deadlock your application's main thread forever. 
    */
  class Process : public wxProcess
      {
        wxArrayString *std_out;
        wxArrayString *std_err;
        wxString *blob;
        wxString *out;

        wxInputStream *stream_stdout;
        wxInputStream *stream_stderr;


      public:
        bool running;
        int exitCode;

        Process::Process(wxArrayString *sout, wxArrayString *err, wxString *blb, wxString *o)
        {
          Redirect();

          std_out = sout;
          std_err = err;
          blob = blb;
          out = o;
          running = true;

          if(std_out == 0)
            Detach();			// Auto-delete a blind Process object, we'll have to trust this is really done
        }

        void FlushPipe()
        {
          wxString line;
          stream_stdout = GetInputStream();
          stream_stderr = GetErrorStream();
          wxTextInputStream t_stream_stdout(*stream_stdout);
          wxTextInputStream t_stream_stderr(*stream_stderr);
          assert(stream_stdout);
          assert(stream_stderr);
          while(! stream_stdout->Eof() )
            {
              line = t_stream_stdout.ReadLine();
              blob->Append(line);
              std_out->Add(line);
            }

          while(! stream_stderr->Eof() )
            {
              line = t_stream_stderr.ReadLine();
              blob->Append(line);
              std_out->Add(line);
            }
        }
        ;

        virtual void OnTerminate(int pid, int status)
        {
          if(std_out == 0) // StevieWonder() needs no output
            {
              running = false;
              return;
            }
          assert(std_err);
          assert(blob);

          FlushPipe();

          int count = std_out->Count() -1;
          out->Append(std_out->Item(0));
          for(int i = 1; i < count; ++i)
            out->Append("\n" + std_out->Item(i));

          Manager::Get()->GetAppWindow()->SetStatusText("");
          exitCode = status;
          running = false;
        }
      };

  protected:
    wxString exec;
    class TempFile
      {
      public:
        wxString name;	// this is evil, but do I care?
        TempFile(const wxString& comment)
        {
          wxFile f;
          name = wxFileName::CreateTempFileName("", &f);
          if(!f.Write(comment))
            Log::Instance()->Add("Error: unable to open tempfile.");
        };

        ~TempFile()
        {
          ::wxRemoveFile(name);
        }
      };

  public:
    wxArrayString		std_out;
    wxArrayString		std_err;
    wxString			blob;			// "blob" concats everything so searching is somewhat easier
    wxString			out;			// "out" likewise, but preserving linebreaks, and empty on errors
    int lastExitCode;
    bool running;

    ToolRunner() :  lastExitCode(0), running(false)
    {}
    ;
    virtual ~ToolRunner()
    {}
    ;

    void SetExecutable(const wxString& executable)
    {
      exec = executable;
    };

    wxString GetExecutable()
    {
      return exec;
    };

    wxString Q(const wxString & in)
    {
      wxString out(" \"");
      out << in << "\" ";
      return out;
    };

    int ToolRunner::Run(wxString cmd)
    {
      running = true;
      std_out.Empty();
      std_err.Empty();
      blob.Empty();
      out.Empty();
      assert(!exec.IsEmpty());

      wxString runCommand(exec + " " + cmd);

      Process *process = new Process(&std_out, &std_err, &blob, &out);

      if ( !wxExecute(runCommand, wxEXEC_ASYNC, process) )
        {
          Log::Instance()->Add("Execution failed.");
          delete process;
          return -1;
        }

      while(process->running)
        {
          wxYield();
          process->FlushPipe();
        }
      lastExitCode = process->exitCode;

#ifdef LOTS_OF_DEBUG_OUTPUT

      Log::Instance()->Add(runCommand);
      for(int i = 0; i < std_out.Count(); ++i)
        Log::Instance()->Add(std_out[i]);
      for(int i = 0; i < std_err.Count(); ++i)
        Log::Instance()->Add(std_err[i]);
#endif

      delete process;
      running = false;
      return lastExitCode;
    };


    /*
     * Even if you don't see what happens around you, you can still do something good.
     * This function asynchronously starts a process and forgets about it. We never see what happens.
     * TortoiseRunner and SubversionPlugin::OnAttach() make use of this.
     * Note that this function possibly leaks a Process object. We cannot simply pass null to wxExectute, as this
     * will create a shell window.
     * The wxProcess documentation says that Detach() will make the object auto-delete. Let's hope that is true. 
     */
    int ToolRunner::StevieWonder(const wxString& cmd)
    {
      Process *process = new Process(0, 0, 0, 0);
      wxString runCommand(exec + " " + cmd);
      wxExecute(runCommand, wxEXEC_ASYNC, process);
      return 0;
    };
  };



class SVNRunner : public ToolRunner
  {
    wxString username;
    wxString password;
    wxString surplusTarget;
    bool do_force;
    bool prune_non_interactive;

  public:
    SVNRunner(const wxString& executable)
    {
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

    int				SVNRunner::Checkout(const wxString& repo, const wxString& dir, const wxString& revision);
    int				SVNRunner::Import(const wxString& repo, const wxString& dir, const wxString &message);

    int				SVNRunner::Status(const wxString& file, bool minusU = false);
    int				SVNRunner::Update(const wxString& file, const wxString& revision = wxString("HEAD"));
    int				SVNRunner::Commit(const wxString& selected, const wxString& message);

    int				SVNRunner::Move(const wxString& selected, const wxString& to);
    int				SVNRunner::Add(const wxString& selected);
    int				SVNRunner::Delete(const wxString& selected);

    wxString		SVNRunner::Cat(const wxString& selected, const wxString& rev);
    wxString		SVNRunner::Diff(const wxString& selected, const wxString& rev);

    int  			SVNRunner::Revert(const wxString& file);

    wxString		SVNRunner::PropGet(const wxString& file, const wxString& prop);
    int  			SVNRunner::PropSet(const wxString& file, const wxString& prop, const wxString& value, bool recursive);
    int				SVNRunner::PropDel(const wxString& file, const wxString& prop);
    int				SVNRunner::Info(const wxString& file, bool minusR);
    wxString		SVNRunner::Info(const wxString& file);

    wxArrayString	SVNRunner::GetPropertyList(const wxString& file);

    virtual int		SVNRunner::Run(wxString cmd);

    void			SVNRunner::DumpErrors()
    {
      for(unsigned int i = 0; i < std_err.Count(); ++i)
        Log::Instance()->Add(std_err[i]);
    };


  private:
  }
;

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

      return ToolRunner::StevieWonder(cmd);
    };


    int TortoiseRunner::Branch(const wxString& path)
    {
      wxString cmd("/command:copy /path:\"");
      cmd << path << "\" /notempfile /closeonend";
      return Run(cmd);
    }


    int TortoiseRunner::Switch(const wxString& path)
    {
      wxString cmd("/command:switch /path:\"");
      cmd << path << "\" /notempfile /closeonend";
      return Run(cmd);
    }

    int TortoiseRunner::Merge(const wxString& path)
    {
      wxString cmd("/command:merge /path:\"");
      cmd << path << "\" /notempfile /closeonend";
      return Run(cmd);
    }
    int TortoiseRunner::Relocate(const wxString& path)
    {
      wxString cmd("/command:relocate /path:\"");
      cmd << path << "\" /notempfile /closeonend";
      return Run(cmd);
    }
    int TortoiseRunner::Create(const wxString& path)
    {
      wxString cmd("/command:create /path:\"");
      cmd << path << "\" /notempfile /closeonend";
      return Run(cmd);
    }
    int TortoiseRunner::ConflictEditor(const wxString& path)
    {
      wxString cmd("/command:conflicteditor /path:\"");
      cmd << path << "\" /notempfile /closeonend";
      return Run(cmd);
    }
    int TortoiseRunner::StatusDialog(const wxString& path)
    {
      wxString cmd("/command:repostatus /path:\"");
      cmd << path << "\" /notempfile /closeonend";
      return Run(cmd);
    }

  };


class CVSRunner : public ToolRunner
  {
  public:
    CVSRunner(const wxString& executable)
    {
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
      Run(cmd);
    };

    void CVSRunner::Checkout(const wxString& proto, const wxString& repo, const wxString& module, const wxString& workingdir, const wxString& user, const wxString& revision)
    {
      wxString cmd("-d " + proto + user + "@" + repo + " checkout -d" + Q(workingdir)+ (revision.IsEmpty() ? "" : " -r" + Q(revision)) + module);
      Log::Instance()->Add("cvs " + cmd);
      Run(cmd);
    };

    void CVSRunner::Update(const wxString& target, const wxString& revision, const wxString& date)
    {
      wxFileName fn(target);
      wxFileName::SetCwd(fn.GetPath(wxPATH_GET_VOLUME));
      wxString file = wxDirExists(target) ? "" : Q(fn.GetFullName());
      wxString cmd(" update " + file + (revision.IsEmpty() ? "" : " -r" + Q(revision)) + (revision.IsEmpty() ? "" : " -D" + Q(date)));
      Run(cmd);
    };

    void CVSRunner::Commit(const wxString& target, const wxString& message)
    {
      TempFile msg(message);
      wxFileName fn(target);
      wxFileName::SetCwd(fn.GetPath(wxPATH_GET_VOLUME));
      wxString file = wxDirExists(target) ? "" : Q(fn.GetFullName());
      wxString cmd(" commit " + file + " -F " + Q(msg.name));
      Run(cmd);
    };


    //    int				CVSRunner::Import(const wxString& repo, const wxString& dir, const wxString &message);
    //    int				CVSRunner::Add(const wxString& selected);
    //    int				CVSRunner::Delete(const wxString& selected);

  private:
  }
;



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

      return ToolRunner::StevieWonder(cmd);
    };


    int TortoiseCVSRunner::Branch(const wxString& path)
    {
      wxString cmd("cvsbranch -l ");
      cmd << Q(path);
      return Run(cmd);
    }
    int TortoiseCVSRunner::Tag(const wxString& path)
    {
      wxString cmd("cvstag -l ");
      cmd << Q(path);
      return Run(cmd);
    }
    int TortoiseCVSRunner::Merge(const wxString& path)
    {
      wxString cmd("cvsmerge -l ");
      cmd << Q(path);
      return Run(cmd);
    }
    int TortoiseCVSRunner::Patch(const wxString& path)
    {
      wxString cmd("CVSMakePatch -l ");
      cmd << Q(path);
      return Run(cmd);
    }
  };



#endif // TOOLRUNNER_H


