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

#include "log.h"


#define LOTS_OF_DEBUG_OUTPUT




class ToolRunner
  {
    /*
    * The implementation of ToolRunner is fucking braindead and inefficient, in particular the spinlock around wxYield()
    * is some nasty stuff. In addition to the SNAFU given by wxWindows, this version also opens and closes several hundred
    * input streams while spinning. Some serious rewrite needs to be done here some day.
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

    int ToolRunner::Run(wxString cmd)
    {
running = true;
      std_out.Empty();
      std_err.Empty();
      blob.Empty();
      out.Empty();

      wxString runCommand(exec + " " + cmd);

      Process *process = new Process(&std_out, &std_err, &blob, &out);

      if ( !wxExecute(runCommand, wxEXEC_ASYNC, process) )
        {
          Log::Instance()->Add("Execution failed.");
          delete process;
          return -1;
        }


      //wxEnableTopLevelWindows(FALSE);
      while(process->running)
        {
          wxYield();
          process->FlushPipe();
        }
      //wxEnableTopLevelWindows(TRUE);
      lastExitCode = process->exitCode;

#ifdef LOTS_OF_DEBUG_OUTPUT

      Log::Instance()->Add(runCommand);
      Log::Instance()->Add(wxString("stdout:"));
      for(int i = 0; i < std_out.Count(); ++i)
        Log::Instance()->Add(std_out[i]);
      Log::Instance()->Add(wxString("stderr:"));
      for(int i = 0; i < std_err.Count(); ++i)
        Log::Instance()->Add(std_err[i]);
#endif

      delete process;
      running = false;
      return lastExitCode;
    };


    /*
     * Even if you don't see what happens around you, you can still do something good.
     * This function asynchronously starts svn during OnAttach(). No output is needed or wanted. We don't even see if we succeeded.
     * The sole reason svn is run is to force Windows to get its butt moving, as it will otherwise take a virtually endless time
     * when you first click on the project manager (right, dynamic linkage and stuff). On Linux we probably don't need this at all.
     * Note that this function leaks a Process object. It seems like Detach() will make the object auto-delete. Let's hope that is true. 
     */
    int ToolRunner::StevieWonder(wxString cmd)
    {
      Process *process = new Process(0, 0, 0, 0);
      wxString runCommand(exec + " " + cmd);
      wxExecute(runCommand, wxEXEC_ASYNC, process);
      return 0;
    };

  protected:
    wxString exec;
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
            Log::Instance()->Add("Error: unable to open tempfile.");
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
    SVNRunner(const wxString& executable)
    {
      SetExecutable(executable);
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

    void Force() // work around "file has local modifications, use --force switch" error
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


