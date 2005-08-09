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
#include <wx/file.h>
#include <wx/dir.h>

#include "pipedprocess.h"
#include "sdk_events.h"
#include "manager.h"
#include "pluginmanager.h"
#include "globals.h"

#include "log.h"


enum
{
    TRANSACTION_SUCCESS,
    TRANSACTION_FAILURE,
    RUN_AGAIN,
    RUN_NEXT_IN_QUEUE
};



extern EVTIMPORT const wxEventType EVT_WX_SUCKS;
#define SCREW_THIS_MACRO_ABUSE(id, fn) DECLARE_EVENT_TABLE_ENTRY( EVT_WX_SUCKS, id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) & fn, (wxObject *) NULL ),


class BogusProcess : public wxProcess
{
public:
    BogusProcess::BogusProcess()
    {
        Redirect();  // We won't make use of this, but it prevents a DOS window from showing up
        Detach();   // Auto-delete when OnTerminate() returns... hopefully
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
    
    Process::Process()
    {
        Redirect();
        running = true;
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
    static wxArrayString oldName;
public:
    wxString name;
    
    TempFile(const wxString& contents)
    {
        wxFile f;
        name = wxFileName::CreateTempFileName("", &f);
        oldName.Add(name);
        
        if(!f.Write(contents))
            Log::Instance()->Add("Error: unable to open tempfile.");
        f.Close();
    };
    
    static void Cleanup()
    {
        wxString tempFolder = TempFolder();
        
        for(int i = 0; i < oldName.Count() ; ++i)
        {
            if(::wxFileExists(oldName[i]))
                ::wxRemoveFile(oldName[i]);
        }
        
        
        tempFolder = wxFindFirstFile(tempFolder + "cbsvn-*", wxDIR);
        while ( !tempFolder.IsEmpty() )
        {
            // TODO: recursive delete
            tempFolder = wxFindNextFile();
        }
    };
    
    static wxString TempFolder()
    {
        if(oldName.Count() == 0)
        {
            TempFile t("");
        }
        
        if(oldName.Count())
            return wxFileName(oldName[0]).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
            
        return wxEmptyString;
    };
    
    static void CleanupCheck()
    {
        wxArrayString new_oldName;
        wxFile f;
        wxString tname = wxFileName::CreateTempFileName("", &f);
        int cutoff = wxFileModificationTime(tname) - 60*60*4; // files older than 4 hours
        f.Close();
        ::wxRemoveFile(tname);
        
        for(int i = 0; i < oldName.Count() ; ++i)
        {
            if(::wxFileExists(oldName[i]) && wxFileModificationTime(oldName[i]) < cutoff )
                ::wxRemoveFile(oldName[i]);
            else
                new_oldName.Add(oldName[i]);
        }
        oldName = new_oldName;
    };
};





class ToolRunner : public wxEvtHandler
{
public:

    typedef enum
    {
        UNDEFINED,
        SVN,
        CVS,
        DIFF3,
        TKDIFF
    }type;
    
    
    wxArrayString  std_out;
    wxArrayString  std_err;
    wxString   blob;   // "blob" concats everything so searching is somewhat easier
    wxString   out;   // "out" likewise, but preserving linebreaks
    int     lastExitCode;
    
    ToolRunner() :  lastExitCode(0), runnerType(ToolRunner::UNDEFINED), insert_first(0), implicit_run(0)
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
    };
    
    wxString GetExecutable()
    {
        return exec;
    };
    
    void EmptyQueue()
    {
        commandQueue.Empty();
    };
    
    void InsertFirst()
    {
        insert_first = true;
    };
    
    void Implicit()
    {
        implicit_run  = true;
    };
    
    void QueueAgain()
    {
        commandQueue.Insert(lastCommand, 0);
    };
    
    
    wxString GetTarget()
    {
        return target;
    };
    
    wxString LastCommand()
    {
        return lastCommand;
    };
    
    void AddToLastCommand(const wxString& s)
    {
        lastCommand << s;
    };
    
    void SetTarget(const wxString& t)
    {
        target = t;
    };
    
    
    wxString GetQueued()
    {
        if(commandQueue.Count())
            return commandQueue[0];
        else
            return wxEmptyString;
    };
    
    type Type()
    {
        return runnerType;
    };
    
    wxString Q(const wxString & in)
    {
        wxString out(" \"");
        out << in << "\" ";
        out.Replace("\\", "/", true);
        return out;
    };
    
    int ToolRunner::RunBlocking(const wxString& cmd);
    int ToolRunner::Run(const wxString& cmd, const wxString& cwd = wxEmptyString);
    void ToolRunner::RunBlind(const wxString& cmd);
    
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
            wxYield();
        }
    };
    
    bool IsIdle()
    {
        return !Running() && commandQueue.IsEmpty();
    };
    
    bool SetCommand(const wxString& c)
    {
        subcommand = c;
    };
    
protected:
    int ToolRunner::RunAsync(const wxString& cmd, const wxString& cwd);
    cbPlugin *plugin;
    wxArrayString commandQueue;
    wxArrayString cwdQueue;
    static wxString lastCommand;
    wxString target;
    type runnerType;
    wxString  exec;
    wxString  subcommand;
    bool implicit_run;
    
private:
    wxTimer   timer;
    static PipedProcess *cb_process;
    static Process   *process;
    int    pid;
    bool insert_first;
    DECLARE_EVENT_TABLE()
};


#endif

