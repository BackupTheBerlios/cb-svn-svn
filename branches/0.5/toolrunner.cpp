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


const wxEventType EVT_WX_SUCKS = wxNewEventType();

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
  assert(!exec.IsEmpty());
  assert(!cmd.IsEmpty());

  Finish();

  currentJob.Clear();
  process = new Process(); // make Running() return true

  wxString runCommand(exec + " " + cmd);

  wxString oldLang;
  wxString oldSvnSsh;
  wxString oldCvsRsh;

  SaveEnvironment();
  SetEnvironment();

  if ( wxExecute(runCommand, wxEXEC_ASYNC, process) == 0 )
    {
      Log::Instance()->Red("Execution failed.");
      Log::Instance()->Red("Command: " + runCommand);
      exec_error = true;
      delete process;
      process = 0;
      RestoreEnvironment();
      return -1;
    }
  RestoreEnvironment();

  timer.Start(250); // This is a parachute. There should never really be a single timer event.

  while(process->Running())
    wxTheApp->Yield(true);

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



int ToolRunner::RunJob(Job *theJob, bool insert_first)
{
  if(insert_first)
    {
      jobQueue.push_front(newJob);
    }
  else
    {
      jobQueue.push_back(newJob);
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
  assert(!exec.IsEmpty());
  assert(!cmd.IsEmpty());

  std_out.Empty();
  std_err.Empty();
  blob.Empty();

  Log::Instance()->Add(cmd);

  cb_process = new  PipedProcess((void**)&cb_process, this, ID_PROCESS, true, cwd);

  SaveEnvironment();
  SetEnvironment();
  pid = wxExecute(cmd, wxEXEC_ASYNC, cb_process);
  RestoreEnvironment();

  if (!pid)
    {
      Log::Instance()->Red("Execution failed.");
      Log::Instance()->Red("Command: " + cmd);
      exec_error = true;
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
  RunQueue();
};

void ToolRunner::RunQueue()
{
  if(!jobQueue.empty())
    {
      currentJob = jobQueue[0];
      jobQueue.pop_front();

      RunAsync(currentJob.commandline, currentJob.cwd);
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
  if(((SubversionPlugin*)plugin)->verbose && !currentJob.verb.IsSameAs("info"))
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
  e.SetExtraLong(currentJob.runnerType);
  e.SetString(currentJob.verb);
  plugin->AddPendingEvent(e);
};

void ToolRunner::Succeed()
{
  wxCommandEvent e(EVT_WX_SUCKS, TRANSACTION_SUCCESS);
  e.SetExtraLong(currentJob.runnerType);
  e.SetString(currentJob.verb);
  plugin->AddPendingEvent(e);
};

void ToolRunner::Send(int cmd)
{
  wxCommandEvent e(EVT_WX_SUCKS, cmd);
  e.SetExtraLong(currentJob.runnerType);
  plugin->AddPendingEvent(e);
};
