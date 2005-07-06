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

#include <wx/regex.h>
#include <stdio.h>


int SVNRunner::Run(wxString cmd)
{
  wxString ia(" --non-interactive");

  if(prune_non_interactive) // a few commands will refuse to run "non-interactively", although they are not interactive :S
  {
  ia.Empty();
  prune_non_interactive = false;
  }

  cmd.Replace("\\", "/");

  wxString runCmd(cmd);
  if(username > "" && password > "")
    {
      runCmd << " --username " << username << " --password \"" << password << "\"" << ia;
      username = password = "";
    }
  else
    runCmd << ia;

  ToolRunner::Run(runCmd);

  if(lastExitCode == 0)
    return false;


  if(blob.Contains("Connection is read-only"))
    {
      wxMessageDialog(NULL, "Subversion returned 'Connection is read-only'."
                      " This means you either provided no authentication tokens at all (the likely case), "
                      "or you are correctly logged in but do not have write access enabled (check conf/svnserve.conf).\n"
                      "This is NOT an authentication failure.",
                      "Oops...", wxOK );
    }


  wxRegEx reg("authenti|pass|user", wxRE_ICASE);
  if(reg.Matches(blob))
    {
      do
        {
          PasswordDialog p(Manager::Get()->GetAppWindow());
          p.Centre();
          p.ShowModal();

          if(p.username == "")
            return true;

          runCmd = cmd;
          runCmd << " --username " << p.username << " --password \"" << p.password << "\"" << ia;
          ToolRunner::Run(runCmd);
        }
      while(lastExitCode && reg.Matches(blob));
    }
  return lastExitCode;
}


int SVNRunner::Status(const wxString& file)
{
  return Run("status \"" + file + "\"");
}


int  SVNRunner::Revert(const wxString& file)
{
  if(wxDirExists(file))
    wxRmdir(file);
  else
    wxRemoveFile(file);
  wxSleep(2);

  Run("update -r BASE \"" + file + "\"");

  return lastExitCode;
}


int  SVNRunner::Move(const wxString& selected, const wxString& to)
{
  Run("move \"" + selected + "\" \"" + to + "\"" );
  return lastExitCode;
}

int  SVNRunner::Add(const wxString& selected)
{
NoInteractive();
  Run("add \"" + selected + "\"");
  return lastExitCode;
}

int  SVNRunner::Delete(const wxString& selected)
{
NoInteractive();
  Run("delete \"" + selected + "\"");
  return lastExitCode;
}


int  SVNRunner::Checkout(const wxString& repo, const wxString& dir, const wxString& revision)
{
  Run("checkout \"" + repo + "\" \"" + dir + "\" -r " + revision);
  return lastExitCode;
}

int  SVNRunner::Import(const wxString& repo, const wxString& dir, const wxString &message)
{
  TempFile c(message);
  Run("import \"" + dir + "\" \"" + repo + "\" -F \"" + c.name + "\"");
  return lastExitCode;
}


int  SVNRunner::Update(const wxString& selected, wxString& revision)
{
  Run("update \"" + selected + "\" -r " + revision);
  return lastExitCode;
}


int  SVNRunner::Commit(const wxString& selected, const wxString& message)
{
  Status(selected);
  wxString msg(message);
  msg.Replace("\"", "\\\"");
  TempFile c(msg);
  Run("commit \"" + selected + "\" -F \"" + c.name +"\"");
  return lastExitCode;
}

wxArrayString  SVNRunner::GetPropertyList(const wxString& file)
{
  wxArrayString ret;
  Run("pl \"" + file + "\"");

  int n = std_out.Count();
  for(int i = 0; i < n; ++i)
    if(std_out[i].StartsWith("  "))
      ret.Add(std_out[i].Mid(2));

  return ret;
}

wxString  SVNRunner::PropGet(const wxString& file, const wxString& prop)
{
  Run("pg " + prop + " \"" + file + "\"");

  wxString ret;

  int n = std_out.Count();
  for(int i = 0; i < n; ++i)
    ret << std_out[i] << "\n";

  return ret;
}



