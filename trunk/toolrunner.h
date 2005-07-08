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
#include <simpletextlog.h>


//#define LOTS_OF_DEBUG_OUTPUT




class ToolRunner
  {
    /*
    * The implementation of ToolRunner is fucking braindead and inefficient, in particular the spinlock around wxYield()
    * is some nasty stuff. This, however, is purely a wxWindows SNAFU, so not much one can do really.
    * The spinlock is needed to pass the stdout pipe via the message queue. Unless you spin, you get no output redirection.
    * This is one gigantic con called "asynchronous" notification.
    * You might think you could put a sem->Post() into Process::OnTerminate() and sem->Wait() into ToolRunner::Run().
    * After all we're notified asynchronously, aren't we. Well, if only that was the case... :(
     * This class definitely needs a complete, rewrite some day.
    */
  class Process : public wxProcess
      {
        wxArrayString *std_out;
        wxArrayString *std_err;
        wxString *blob;

      public:
        bool running;

        Process::Process(wxArrayString *out, wxArrayString *err, wxString *blb)
        {
          Redirect();
          std_out = out;
          std_err = err;
          blob = blb;
          running = true;
        }

        virtual void OnTerminate(int pid, int status)
        {
          if(std_out == 0) // StevieWonder() needs no output
            {
              running = false;
              return;
            }
          assert(std_err);
          assert(blob);

          std_out->Empty();
          std_err->Empty();
          blob->Empty();

          if(!status)
            {
              wxInputStream *s = GetInputStream();
              assert(s);
              wxTextInputStream tis(*s);
              wxString line;
              while(! s->Eof() )
                {
                  line = tis.ReadLine();
                  blob->Append(line);
                  std_out->Add(line);
                }
            }
          else
            {
              wxInputStream *s = GetErrorStream();
              assert(s);
              wxTextInputStream tis(*s);

              wxString line;
              while(! s->Eof() )
                {
                  line = tis.ReadLine();
                  blob->Append(line);
                  std_err->Add(line);
                }

            }
          running = false;
        }
      };


  public:
    wxArrayString		std_out;		// Oh yes, we're public once again. Assume we won't do much harm though.
    wxArrayString		std_err;
    wxString			blob;			// "blob" concats everything so searching is somewhat easier
    ToolRunner() :  lastExitCode(0)
    {}
    ;
    virtual ~ToolRunner()
    {}
    ;

    void SetExecutable(const wxString& executable)
    {
      exec = executable;
    };

    int ToolRunner::Run(wxString cmd)
    {
      wxString runCommand(exec + " " + cmd);

      Process *process = new Process(&std_out, &std_err, &blob);

      if ( !wxExecute(runCommand, wxEXEC_ASYNC, process) )
        {
          outputLog->AddLog("Execution failed.");
          delete process;
          return -1;
        }


      wxEnableTopLevelWindows(FALSE);
      while(process->running)
        wxYield();
      wxEnableTopLevelWindows(TRUE);

#ifdef LOTS_OF_DEBUG_OUTPUT
      outputLog->AddLog(runCommand);
      outputLog->AddLog(wxString("stdout:"));
      for(int i = 0; i < std_out.Count(); ++i)
        outputLog->AddLog(std_out[i]);
      outputLog->AddLog(wxString("stderr:"));
      for(int i = 0; i < std_err.Count(); ++i)
        outputLog->AddLog(std_err[i]);
#endif

      delete process;
      return lastExitCode;
    };



    /*
     * Even if you don't see what happens around you, you can still do something good.
     * This function asynchronously starts svn during OnAttach(). No output is needed or wanted. We don't even see if we succeeded.
     * The sole reason svn is run is to force Windows to get its butt moving, as it will otherwise take a virtually endless time
     * when you first click on the project manager (right, dynamic linkage and stuff). On Linux we probably don't need this at all.
     * Note that this function leaks a Process object. THIS IS DELIBERATE as we will otherwise crash the application
     * -- there is no notification from the running child process, so no way of telling when it will be safe to delete the process.
     * Unluckily, we cannot just pass NULL to wxExecute() as this would pop up an annoying DOS window.
     */
    int ToolRunner::StevieWonder(wxString cmd)
    {
      Process *process = new Process(0, 0, 0);
      wxString runCommand(exec + " " + cmd);
      wxExecute(runCommand, wxEXEC_ASYNC, process);
      return 0;
    };




  protected:
    wxString exec;
    SimpleTextLog* outputLog;

    int lastExitCode;

    wxProgressDialog*	progressBar;
    unsigned int		count;
  };



class SVNRunner : public ToolRunner
  {
    class TempFile
      {
      public:
        wxString name;	// this is evil, but do I care?
        TempFile(const wxString& comment)
        {
          wxFile f;
          name = wxFileName::CreateTempFileName("", &f);
          if(!f.Write(comment))
            ::wxBell();	// oh yes, this is truly some good error checking
        };

        ~TempFile()
        {
          ::wxRemoveFile(name);
        }
      };


    wxString username;
    wxString password;
    bool do_force;
    bool prune_non_interactive;
    int wantProgressBar;

  public:
    SVNRunner(const wxString& executable, SimpleTextLog* log)
    {
      SetExecutable(executable);
      ToolRunner::outputLog = log;
      wantProgressBar = false;
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
    void Force() // work around "subcommand does not support --non-interactive" error
    {
      do_force = true;
    };

    wxString Q(const wxString & in)
    {
      wxString out(" \"");
      out << in << "\" ";
      return out;
    };

    int				SVNRunner::Checkout(const wxString& repo, const wxString& dir, const wxString& revision);
    int				SVNRunner::Import(const wxString& repo, const wxString& dir, const wxString &message);

    int				SVNRunner::Status(const wxString& file);
    int				SVNRunner::Update(const wxString& file, wxString& revision);
    int				SVNRunner::Commit(const wxString& selected, const wxString& message);

    int				SVNRunner::Move(const wxString& selected, const wxString& to);
    int				SVNRunner::Add(const wxString& selected);
    int				SVNRunner::Delete(const wxString& selected);

    int  			SVNRunner::Revert(const wxString& file);

    wxString		SVNRunner::PropGet(const wxString& file, const wxString& prop);
    int  			SVNRunner::PropSet(const wxString& file, const wxString& prop, const wxString& value, bool recursive);
    int				SVNRunner::PropDel(const wxString& file, const wxString& prop);

    wxArrayString	SVNRunner::GetPropertyList(const wxString& file);

    virtual int		SVNRunner::Run(wxString cmd);

    void			SVNRunner::DumpErrors()
    {
      for(unsigned int i = 0; i < std_err.Count(); ++i)
        outputLog->AddLog(std_err[i]);
    };


  private:
  }
;

class TortoiseRunner : public ToolRunner
  {
  public:
    TortoiseRunner(const wxString& executable, SimpleTextLog* log)
    {
      SetExecutable(executable);
      outputLog = log;
    }
    ;

    virtual ~TortoiseRunner()
    {}
    ;

    virtual int TortoiseRunner::Run(wxString cmd)
    {
      return ToolRunner::Run(cmd);
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

#endif // TOOLRUNNER_H


