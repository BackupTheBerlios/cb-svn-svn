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

extern const wxEventType EVT_WX_SUCKS;

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
        
    return lastExitCode;
}



void SVNRunner::OutputHandler()
{
    Manager::Get()->GetAppWindow()->SetStatusText("");
    
    if(blob.Contains("Connection is read-only"))
    {
        Log::Instance()->Red("Subversion returned 'Connection is read-only'.");
        Log::Instance()->Add("  This means you either provided no authentication tokens at all (try 'Set User...'), \n"
                             "  or you are correctly logged in but do not have write access enabled (check conf/svnserve.conf).");
        Fail();
        return;
    }
    
    wxRegEx reg("authenti|pass|user", wxRE_ICASE);
    if(reg.Matches(blob))
    {
        PasswordDialog p(Manager::Get()->GetAppWindow());
        p.Centre();
        if(p.ShowModal() == wxID_CANCEL || p.username == "")
        {
            Log::Instance()->Add("User cancelled authentication.");
            return ;
        }
        
        username = p.username;
        password = p.password;
        std_err.Empty();
        
        if(lastCommand.Contains("--username"))
            lastCommand = lastCommand.Left(lastCommand.Index("--username"));
            
        lastCommand << " --username " << p.username << " --password \"" << p.password;
        
        Send(RUN_AGAIN);
        return;
    }
    
    ToolRunner::OutputHandler();
}

int SVNRunner::Status(const wxString& selected, bool minusU)
{
    SetTarget(selected);
    return RunBlocking("status" + Q(selected) + (minusU ? " -u" : ""));
}


int  SVNRunner::Revert(const wxString& selected)
{
    SetTarget(selected);
    NoInteractive();
    return Run("revert" + Q(selected));
}


int  SVNRunner::Move(const wxString& selected, const wxString& to)
{
    SetTarget(selected);
    return RunBlocking("move" + Q(selected) + Q(to) );
}

int  SVNRunner::Add(const wxString& selected)
{
    SetTarget(selected);
    NoInteractive();
    return RunBlocking("add" + Q(selected));
}

int  SVNRunner::Delete(const wxString& selected)
{
    SetTarget(selected);
    NoInteractive();
    return RunBlocking("delete" + Q(selected));
}


int  SVNRunner::Checkout(const wxString& repo, const wxString& dir, const wxString& revision, bool noExternals)
{
    SetTarget(dir);
    return Run("checkout" + Q(repo) + Q(dir) + "-r " + revision + (noExternals ? " --ignore-externals" : ""));
}

int  SVNRunner::Import(const wxString& repo, const wxString& dir, const wxString &message)
{
    SetTarget(dir);
    TempFile c(message);
    return Run("import" + Q(dir) + Q(repo) + "-F" + Q(c.name));
}


int  SVNRunner::Update(const wxString& selected, const wxString& revision)
{
    SetTarget(selected);
    return Run("update" + Q(selected) + "-r " + revision);
}


int  SVNRunner::Commit(const wxString& selected, const wxString& message)
{
    SetTarget(selected);
    TempFile c(message);
    return Run("commit" + Q(selected) + "-F" + Q(c.name));
}

wxArrayString  SVNRunner::GetPropertyList(const wxString& selected)
{
    SetTarget(selected);
    wxArrayString ret;
    RunBlocking("proplist" + Q(selected));
    
    int n = std_out.Count();
    for(int i = 0; i < n; ++i)
        if(std_out[i].StartsWith("  "))
            ret.Add(std_out[i].Mid(2));
            
    return ret;
}

wxString  SVNRunner::PropGet(const wxString& selected, const wxString& prop)
{
    SetTarget(selected);
    RunBlocking("propget" + Q(prop) + Q(selected));
    return out;
}

int  SVNRunner::PropSet(const wxString& selected, const wxString& prop, const wxString& value, bool recursive)
{
    SetTarget(selected);
    TempFile t(value);
    return  Run("propset" + Q(prop) + "-F" + Q(t.name) + Q(selected) + (recursive ? "-R" : ""));
    
}

int SVNRunner::PropDel(const wxString& selected, const wxString& prop)
{
    SetTarget(selected);
    return  Run("propdel" + Q(prop) + Q(selected));
}

wxString SVNRunner::Cat(const wxString& selected, const wxString& rev)
{
    SetTarget(selected);
    if(rev.IsEmpty())
        Run("cat" + Q(selected));
    else
        Run("cat" + Q(selected) + "-r" +Q(rev) );
    return out;
}

wxString SVNRunner::Diff(const wxString& selected, const wxString& rev)
{
    SetTarget(selected);
    if(rev.IsEmpty())
        Run("diff" + Q(selected));
    else
        Run("diff" + Q(selected) + "-r" +Q(rev) );
    return out;
}


int SVNRunner::Info(const wxString& selected, bool minusR)
{
    SetTarget(selected);
    return  Run("info" + Q(selected) + (minusR ? "-R" : ""));
}





