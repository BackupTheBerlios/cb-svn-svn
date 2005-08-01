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

#include <wx/process.h>
#include <pipedprocess.h>
#include <wx/file.h>
#include <wx/filename.h>

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
    static wxString oldName;
public:
    wxString name;
    
    TempFile(const wxString& contents)
    {
        if(!oldName.IsEmpty() && ::wxFileExists(oldName))
            ::wxRemoveFile(oldName);
            
        wxFile f;
        name = wxFileName::CreateTempFileName("", &f);
        TempFile::oldName = name;
        
        if(!f.Write(contents))
            Log::Instance()->Add("Error: unable to open tempfile.");
    };
    
    static void Cleanup()
    {
        if(!oldName.IsEmpty() && ::wxFileExists(oldName))
            ::wxRemoveFile(oldName);
    };
};





class ToolRunner : public wxEvtHandler
{
public:

    typedef enum
    {
        UNDEFINED,
        SVN,
        CVS
    }type;
    
    
    wxArrayString  std_out;
    wxArrayString  std_err;
    wxString   blob;   // "blob" concats everything so searching is somewhat easier
    wxString   out;   // "out" likewise, but preserving linebreaks
    int     lastExitCode;
    
    ToolRunner() :  lastExitCode(0), runnerType(ToolRunner::UNDEFINED)
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
    
    void PushBack()
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
    int ToolRunner::Run(const wxString& cmd);
    void ToolRunner::RunBlind(const wxString& cmd);
    
    int ToolRunner::RunAsync(const wxString& cmd);
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
    
protected:
    cbPlugin *plugin;
    wxArrayString commandQueue;
    static wxString lastCommand;
    wxString target;
    type runnerType;
    
private:
    wxString  exec;
    wxTimer   timer;
    static PipedProcess *cb_process;
    static Process   *process;
    int    pid;
    
    DECLARE_EVENT_TABLE()
};


#endif

