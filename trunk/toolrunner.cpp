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


#include "toolrunner.h"
#include "svn.h"
#include "log.h"
#include "dialogs.h"

#include <wx/regex.h>
#include <wx/event.h>
#include <stdio.h>




const wxEventType EVT_WX_SUCKS = wxNewEventType();

wxString TempFile::oldName = wxEmptyString;


BEGIN_EVENT_TABLE(ToolRunner, wxEvtHandler)
EVT_TIMER(-1, ToolRunner::OnTimer)
EVT_PIPEDPROCESS_STDOUT(ID_PROCESS,  ToolRunner::OnOutput)
EVT_PIPEDPROCESS_STDERR(ID_PROCESS,  ToolRunner::OnError)
EVT_PIPEDPROCESS_TERMINATED(ID_PROCESS, ToolRunner::OnTerminated)
END_EVENT_TABLE()



int ToolRunner::RunBlocking(const wxString& cmd)
{
    assert(!exec.IsEmpty() && !cmd.IsEmpty());
    
    Finish();
    
    lastCommand.Empty();
    process = new Process(); // make Running() return true
    
    wxString runCommand(exec + " " + cmd);
    
    Log::Instance()->Add(runCommand);
    
    if ( wxExecute(runCommand, wxEXEC_ASYNC, process) == -1 )
    {
        Log::Instance()->Add("Execution failed.");
        Log::Instance()->Add("Command: " + runCommand);
        delete process;
        process = 0;
        return -1;
    }
    
    timer.Start(250); // This is a parachute. There should never really be a single timer event.
    
    while(process->Running())
        wxYield();
        
    timer.Stop();
    
    lastExitCode = process->exitCode;
    
    if(lastExitCode)
    {
        blob = ::GetStringFromArray(process->std_err, wxEmptyString);
        out = ::GetStringFromArray(process->std_err, "\n");
    }
    else
    {
        blob = ::GetStringFromArray(process->std_out, wxEmptyString);
        out = ::GetStringFromArray(process->std_out, "\n");
    }
    
    std_out = process->std_out;
    std_err = process->std_err;
    
    delete process;
    process = 0;
    return lastExitCode;
};



int ToolRunner::Run(const wxString& cmd)
{
    wxString runCommand(exec + " " + cmd);
    
    if(Running())
    {
        commandQueue.Add(runCommand);
        return 0;
    }
    
    RunAsync(runCommand);
    Log::Instance()->fg();
    lastCommand = runCommand;
    return 0;
};

int ToolRunner::RunAsync(const wxString& cmd)
{
    assert(!exec.IsEmpty() && !cmd.IsEmpty());
    std_out.Empty();
    std_err.Empty();
    blob.Empty();
    
    Log::Instance()->Add(cmd);
    
    cb_process = new  PipedProcess((void**)&cb_process, this, ID_PROCESS, true);
    
    pid = wxExecute(cmd, wxEXEC_ASYNC, cb_process);
    if ( !pid )
    {
        Log::Instance()->Add("Execution failed.");
        Log::Instance()->Add("Command: " + cmd);
        delete cb_process;
        cb_process = 0;
        wxCommandEvent e(EVT_WX_SUCKS, TRANSACTION_FAILURE);
        plugin->AddPendingEvent(e);
        return -1;
    }
    
    timer.Start(100);
    
    return 0;
};

void ToolRunner::RunAgain()
{
Log::Instance()->Add("rerunning...");
	if(!lastCommand.IsEmpty())
        RunAsync(lastCommand);
};

void ToolRunner::RunQueue()
{
    if(commandQueue.Count())
    {
        wxString runCommand = commandQueue[0];
        commandQueue.Remove((size_t) 0);
        RunAsync(runCommand);
    }
};


void ToolRunner::RunBlind(const wxString& cmd)
{
    BogusProcess *proc = new BogusProcess();
    wxString runCommand(exec + " " + cmd);
    wxExecute(runCommand, wxEXEC_ASYNC, proc);
};

void ToolRunner::OnTimer(wxTimerEvent& event)
{
    if (cb_process)
        ((PipedProcess*)cb_process)->HasInput();
        
    if(process)
        process->FlushPipe();
}

void ToolRunner::OnOutput(CodeBlocksEvent& event)
{
    wxString msg(event.GetString());
    std_out.Add(msg);
    Log::Instance()->Add(msg);
}

void ToolRunner::OnError(CodeBlocksEvent& event)
{
    wxString msg(event.GetString());
    std_err.Add(msg);
    Log::Instance()->Add(msg);
}

void ToolRunner::OnTerminated(CodeBlocksEvent& event)
{
    lastExitCode = event.GetInt();
    timer.Stop();
    
    if(std_err.Count())
    {
        blob = ::GetStringFromArray(std_err, wxEmptyString);
        out = ::GetStringFromArray(std_err, "\n");
    }
    else
    {
        blob = ::GetStringFromArray(std_out, wxEmptyString);
        out = ::GetStringFromArray(std_out, "\n");
    }
    
    OutputHandler();
}


void ToolRunner::Fail()
    {
        wxCommandEvent e(EVT_WX_SUCKS, TRANSACTION_FAILURE);
        e.SetExtraLong(runnerType);
        plugin->AddPendingEvent(e);
    };
    
void ToolRunner::Succeed()
    {
        wxCommandEvent e(EVT_WX_SUCKS, TRANSACTION_SUCCESS);
        e.SetExtraLong(runnerType);
        plugin->AddPendingEvent(e);
    };
    
void ToolRunner::Send(int cmd)
    {
        wxCommandEvent e(EVT_WX_SUCKS, cmd);
        e.SetExtraLong(runnerType);
        plugin->AddPendingEvent(e);
    };
