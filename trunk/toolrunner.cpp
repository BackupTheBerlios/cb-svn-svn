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
#include "dialogs.h"

#include <wx/event.h>


const wxEventType EVT_WX_SUCKS = wxNewEventType();

wxArrayString TempFile::oldName;
wxString ToolRunner::lastCommand  = wxEmptyString;
PipedProcess* ToolRunner::cb_process = 0;
Process* ToolRunner::process   = 0;

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
    
    wxString oldLang;
    wxGetEnv("LANG", &oldLang);
    wxSetEnv("LANG", "en");
    
    if ( wxExecute(runCommand, wxEXEC_ASYNC, process) == -1 )
    {
        Log::Instance()->Red("Execution failed.");
        Log::Instance()->Red("Command: " + runCommand);
        delete process;
        process = 0;
        wxSetEnv("LANG", oldLang);
        return -1;
    }
    wxSetEnv("LANG", oldLang);
    
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



int ToolRunner::Run(const wxString& cmd, const wxString& cwd)
{
    wxString runCommand(exec + " " + cmd);
    
    if(insert_first)
    {
        commandQueue.Insert(runCommand, 0);
        cwdQueue.Insert(cwd, 0);
    }
    else
    {
        commandQueue.Add(runCommand);
        cwdQueue.Add(cwd);
    }
    
    Finish();
    if(!implicit_run)
        RunQueue();
        
    insert_first = false;
    implicit_run = false;
    return 0;
};

int ToolRunner::RunAsync(const wxString& cmd, const wxString& cwd)
{
    assert(!exec.IsEmpty() && !cmd.IsEmpty());
    std_out.Empty();
    std_err.Empty();
    blob.Empty();
    
    Log::Instance()->Add(cmd);
    
    cb_process = new  PipedProcess((void**)&cb_process, this, ID_PROCESS, true, cwd);
    
    pid = wxExecute(cmd, wxEXEC_ASYNC, cb_process);
    if ( !pid )
    {
        Log::Instance()->Red("Execution failed.");
        Log::Instance()->Red("Command: " + cmd);
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
    QueueAgain();
    Finish();
    RunQueue();
};

void ToolRunner::RunQueue()
{
    if(commandQueue.Count())
    {
        wxString runCommand = commandQueue[0];
        commandQueue.Remove((size_t) 0);
        wxString cwd = cwdQueue[0];
        cwdQueue.Remove((size_t) 0);
        wxString oldLang;
        wxGetEnv("LANG", &oldLang);
        wxSetEnv("LANG", "en");
        RunAsync(runCommand, cwd);
        wxSetEnv("LANG", oldLang);
        lastCommand = runCommand;
    }
};


void ToolRunner::RunBlind(const wxString& cmd)
{
    BogusProcess *proc = new BogusProcess();
    wxString runCommand(exec + " " + cmd);
    wxString oldLang;
    wxGetEnv("LANG", &oldLang);
    wxSetEnv("LANG", "en");
    wxExecute(runCommand, wxEXEC_ASYNC, proc);
    wxSetEnv("LANG", oldLang);
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
    if(((SubversionPlugin*)plugin)->verbose)
        Log::Instance()->Grey(msg);
}

void ToolRunner::OnError(CodeBlocksEvent& event)
{
    wxString msg(event.GetString());
    std_err.Add(msg);
    Log::Instance()->Grey(msg);
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
    e.SetString(subcommand);
    plugin->AddPendingEvent(e);
    subcommand.Empty();
};

void ToolRunner::Succeed()
{
    wxCommandEvent e(EVT_WX_SUCKS, TRANSACTION_SUCCESS);
    e.SetExtraLong(runnerType);
    e.SetString(subcommand);
    plugin->AddPendingEvent(e);
    subcommand.Empty();
};

void ToolRunner::Send(int cmd)
{
    wxCommandEvent e(EVT_WX_SUCKS, cmd);
    e.SetExtraLong(runnerType);
    plugin->AddPendingEvent(e);
};
