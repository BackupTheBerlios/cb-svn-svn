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
#include "svncvsrunner.h"
#include "svn.h"
#include "log.h"
#include "dialogs.h"

#include <wx/regex.h>
#include <stdio.h>


int SVNRunner::Run(wxString cmd)
{
    wxString ia(" --non-interactive");
    wxString force;
    
    
    if(prune_non_interactive) // a few commands will refuse to run "non-interactively", although they are not interactive :S
    {
        ia.Empty();
        prune_non_interactive = false;
    }
    
    if(do_force)
    {
        do_force = false;
        force = " --force";
    }
    cmd.Replace("\\", "/");
    
    Manager::Get()->GetAppWindow()->SetStatusText("svn " +cmd);
    
    wxString runCmd(cmd);
    if(username > "" && password > "")
    {
        runCmd << " --username " << username << " --password \"" << password << "\"" << ia << force;
        username = password = "";
    }
    else
        runCmd << ia << force;
        
    ToolRunner::Run(runCmd);
    
    if(lastExitCode == 0)
        return false;
        
    {
        wxRegEx reg("run.*svn.*cleanup", wxRE_ICASE);    // svn:run 'svn cleanup' to remove locks (type 'svn help cleanup' for details)
        if(reg.Matches(blob) && runCmd.Contains(surplusTarget))  // if surplusTarget is contained in runCmd, it can be assumed valid
        {
            Log::Instance()->Add("Running svn cleanup to remove stale locks...");
            ToolRunner::Run("cleanup" + Q(surplusTarget));
            ToolRunner::Run(runCmd);
            
            if(lastExitCode == 0)
                return false;
        }
    }
    
    if(blob.Contains("Connection is read-only"))
    {
        wxMessageDialog(NULL, "Subversion returned 'Connection is read-only'."
                        " This means you either provided no authentication tokens at all (the likely case), "
                        "or you are correctly logged in but do not have write access enabled (check conf/svnserve.conf).\n"
                        "If you did not authenticate, try 'Set User...' from the project manager menu.\n\n"
                        "Note that this is NOT an authentication failure, so if you already did provide some credentials,\n"
                        "submitting these again will not help.",
                        "Oops...", wxOK );
        return -1;
    }
    
    
    wxRegEx reg("authenti|pass|user", wxRE_ICASE);
    if(reg.Matches(blob))
    {
        do
        {
            PasswordDialog p(Manager::Get()->GetAppWindow());
            p.Centre();
            if(p.ShowModal() == wxID_CANCEL)
            {
                Log::Instance()->Add("User cancelled authentication.");
                return -1;
            }
            if(p.username == "")
                return true;
                
            runCmd = cmd;
            runCmd << " --username " << p.username << " --password \"" << p.password << "\"" << ia << force;
            ToolRunner::Run(runCmd);
        }
        while(lastExitCode && reg.Matches(blob));
    }
    return lastExitCode;
}


int SVNRunner::Status(const wxString& file, bool minusU)
{
    return RunBlocking("status" + Q(file) + (minusU ? " -u" : ""));
}


int  SVNRunner::Revert(const wxString& file)
{
    NoInteractive();
    return Run("revert" + Q(file));
}


int  SVNRunner::Move(const wxString& selected, const wxString& to)
{
    return RunBlocking("move" + Q(selected) + Q(to) );
}

int  SVNRunner::Add(const wxString& selected)
{
    NoInteractive();
    return RunBlocking("add" + Q(selected));
}

int  SVNRunner::Delete(const wxString& selected)
{
    NoInteractive();
    return RunBlocking("delete" + Q(selected));
}


int  SVNRunner::Checkout(const wxString& repo, const wxString& dir, const wxString& revision, bool noExternals)
{
    return Run("checkout" + Q(repo) + Q(dir) + "-r " + revision + (noExternals ? " --ignore-externals" : ""));
}

int  SVNRunner::Import(const wxString& repo, const wxString& dir, const wxString &message)
{
    TempFile c(message);
    return Run("import" + Q(dir) + Q(repo) + "-F" + Q(c.name));
}


int  SVNRunner::Update(const wxString& selected, const wxString& revision)
{
    surplusTarget = selected; // update may fail due to stale locks, this will be used to call svn cleanup
    return Run("update" + Q(selected) + "-r " + revision);
}


int  SVNRunner::Commit(const wxString& selected, const wxString& message)
{
    TempFile c(message);
    return Run("commit" + Q(selected) + "-F" + Q(c.name));
}

wxArrayString  SVNRunner::GetPropertyList(const wxString& file)
{
    wxArrayString ret;
    RunBlocking("proplist" + Q(file));
    
    int n = std_out.Count();
    for(int i = 0; i < n; ++i)
        if(std_out[i].StartsWith("  "))
            ret.Add(std_out[i].Mid(2));
            
    return ret;
}

wxString  SVNRunner::PropGet(const wxString& file, const wxString& prop)
{
    RunBlocking("propget" + Q(prop) + Q(file));
    return out;
}

int  SVNRunner::PropSet(const wxString& file, const wxString& prop, const wxString& value, bool recursive)
{
    TempFile t(value);
    return  RunBlocking("propset" + Q(prop) + "-F" + Q(t.name) + Q(file) + (recursive ? "-R" : ""));
    
}

int SVNRunner::PropDel(const wxString& file, const wxString& prop)
{
    return  RunBlocking("propdel" + Q(prop) + Q(file));
}

wxString SVNRunner::Cat(const wxString& selected, const wxString& rev)
{
    if(rev.IsEmpty())
        Run("cat" + Q(selected));
    else
        Run("cat" + Q(selected) + "-r" +Q(rev) );
    return out;
}

wxString SVNRunner::Diff(const wxString& selected, const wxString& rev)
{
    if(rev.IsEmpty())
        Run("diff" + Q(selected));
    else
        Run("diff" + Q(selected) + "-r" +Q(rev) );
    return out;
}


int SVNRunner::Info(const wxString& file, bool minusR)
{
    return  Run("info" + Q(file) + (minusR ? "-R" : ""));
}


wxString SVNRunner::Info(const wxString& file)
{
    Info(file, false);
    wxString ret;
    for(int i = 0; i < std_out.Count(); ++i)
    {
        if(std_out[i].Contains("not a working copy"))
            return std_out[i];
            
        if(std_out[i].Contains("UUID") || std_out[i].Contains("Revision") || std_out[i].Contains("Last"))
            ret << std_out[i] << "\n";
    }
    return ret;
}




