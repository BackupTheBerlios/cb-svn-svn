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


extern const wxEventType EVT_WX_SUCKS;

int SVNRunner::Run(wxString cmd)
{
    wxString ia;
    wxString force;
    
    if(cmd.Contains(" update ") || cmd.Contains(" commit ") || cmd.Contains(" delete ")
            || cmd.Contains(" status ") || cmd.Contains(" lock ") || cmd.Contains(" unlock "))
        ia = " --non-interactive";
        
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
        runCmd << " --username " << Q(username) << " --password \"" << password << "\"" << ia << force;
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
    
    // Transmitting file data .svn: Commit failed (details follow):
    // svn: Cannot verify lock on path '/base.cpp'; no matching lock-token available
    if(blob.Contains("no matching lock-token available"))
    {
        Log::Instance()->Red("Someone else is holding a lock which prevents you from committing your changes.");
        
        EmptyQueue();
        Info(GetTarget(), false);
        Send(RUN_NEXT_IN_QUEUE);
    }
    
    
    wxRegEx reg("authenti|password", wxRE_ICASE);
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
    
    if(remoteStatusHandler)
    {
        remoteStatusHandler = false;
        RemoteStatusHandler();
    }
    
    ToolRunner::OutputHandler();
}

void SVNRunner::RemoteStatusHandler()
{
    //  L     some_dir            # svn left a lock in the .svn area of some_dir
    //M       bar.c               # the content in bar.c has local modifications
    //M      *bar.c               # the content in bar.c has local and remote modifications
    // M      baz.c               # baz.c has property but no content modifications
    //X       3rd_party           # dir is part of an externals definition
    //?       foo.o               # svn doesn't manage foo.o
    //!       some_dir            # svn manages this, but it's missing or incomplete
    //~       qux                 # versioned as file/dir/link, but type has changed
    //I       .screenrc           # svn doesn't manage this, and is set to ignore it
    //A  +    moved_dir           # added with history of where it came from
    //M  +    moved_dir/README    # added with history and has local modifications
    //D       stuff/fish.c        # file is scheduled for deletion
    //A       stuff/loot/bloo.h   # file is scheduled for addition
    //C       stuff/loot/lump.c   # file has textual conflicts from an update
    // C      stuff/loot/glub.c   # file has property conflicts from an update
    //R       xyz.c               # file is scheduled for replacement
    //    S   stuff/squawk        # file or dir has been switched to a branch
    //     K  dog.jpg             # file is locked locally; lock-token present
    //     O  cat.jpg             # file is locked in the repository by other user
    //     B  bird.jpg            # file is locked locally, but lock has been broken
    //     T  fish.jpg            # file is locked locally, but lock has been stolen
    
    bool need_update = 0;
    
    int conflict = 0;
    int broken = 0;
    int stolen = 0;
    int locked = 0;
    
    for(int i = 0; i < std_out.Count(); ++i)
    {
        if(std_out[i][(size_t)7] == '*')
            need_update = true;
        if(std_out[i][(size_t)5] == 'C')
            ++conflict;
        if(std_out[i][(size_t)5] == 'B')
            ++broken;
        if(std_out[i][(size_t)5] == 'T')
            ++stolen;
        if(std_out[i][(size_t)5] == 'O' && ( std_out[i][(size_t)0] == 'M' || std_out[i][(size_t)0] == 'D'))
            ++locked;
            
    }
    
    if(need_update)
    {
        wxBell();
        InsertFirst();
        Implicit();
        Update(GetTarget());
        return;
    }
}

int SVNRunner::Status(const wxString& selected, bool minusU)
{
    SetTarget(selected);
    if(minusU)
    {
        SetCommand("status");
        Run("status" + Q(selected) + " -u");
        return 0;
    }
    return RunBlocking("status" + Q(selected));
}

int  SVNRunner::Revert(const wxString& selected)
{
    SetTarget(selected);
    SetCommand("revert");
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
    return RunBlocking("add" + Q(selected));
}

int  SVNRunner::Delete(const wxString& selected)
{
    SetTarget(selected);
    return RunBlocking("delete" + Q(selected));
}


int  SVNRunner::Checkout(const wxString& repo, const wxString& dir, const wxString& revision, bool noExternals)
{
    SetTarget(dir);
    SetCommand("checkout");
    return Run("checkout" + Q(repo) + Q(dir) + "-r " + revision + (noExternals ? " --ignore-externals" : ""));
}

int  SVNRunner::Import(const wxString& repo, const wxString& dir, const wxString &message)
{
    SetTarget(dir);
    SetCommand("import");
    TempFile c(message);
    return Run("import" + Q(dir) + Q(repo) + "-F" + Q(c.name));
}


int  SVNRunner::Update(const wxString& selected, const wxString& revision)
{
    SetTarget(selected);
    SetCommand("update");
    return Run("update" + Q(selected) + "-r " + revision);
}


int  SVNRunner::Commit(const wxString& selected, const wxString& message, bool safeCast)
{
    SetTarget(selected);
    SetCommand("commit");
    TempFile c(message);
    
    if(safeCast)
    {
        EnableRemoteStatusHandler();
        Status(selected,  true);
    }
    return Run("commit" + Q(selected) + "-F" + Q(c.name));
}

int  SVNRunner::Lock(const wxString& selected, bool force)
{
    SetTarget(selected);
    SetCommand("lock");
    return Run("lock" + Q(selected) + (force ? " --force" : ""));
}

int  SVNRunner::UnLock(const wxString& selected, bool force)
{
    SetTarget(selected);
    SetCommand("unlock");
    return Run("unlock" + Q(selected) + (force ? " --force" : ""));
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
    SetCommand("propset");
    TempFile t(value);
    return  Run("propset" + Q(prop) + "-F" + Q(t.name) + Q(selected) + (recursive ? "-R" : ""));
    
}

int SVNRunner::PropDel(const wxString& selected, const wxString& prop)
{
    SetTarget(selected);
    SetCommand("propdel");
    return  Run("propdel" + Q(prop) + Q(selected));
}

wxString SVNRunner::Cat(const wxString& selected, const wxString& rev)
{
    SetTarget(selected);
    SetCommand("cat");
    if(rev.IsEmpty())
        Run("cat" + Q(selected));
    else
        Run("cat" + Q(selected) + "-r" +Q(rev) );
    return out;
}

wxString SVNRunner::Diff(const wxString& selected, const wxString& rev)
{
    SetTarget(selected);
    SetCommand("diff");
    if(rev.IsEmpty())
        Run("diff" + Q(selected));
    else
        Run("diff" + Q(selected) + "-r" +Q(rev) );
    return out;
}


int SVNRunner::Info(const wxString& selected, bool minusR)
{
    SetTarget(selected);
    SetCommand("info");
    return  Run("info" + Q(selected) + (minusR ? "-R" : ""));
}

int SVNRunner::Resolved(const wxString& selected)
{
    SetTarget(selected);
    SetCommand("resolved");
    return  Run("resolved" + Q(selected));
}

void SVNRunner::Export(const wxString& src, const wxString& dest, const wxString& rev, const wxString why)
{
    assert(!src.IsSameAs(dest));
    
    SetTarget(src + "*" + dest);
    SetCommand(wxString("export:") + why);
    
    Run("export" + Q(src) + Q(dest) + "-r" +Q(rev));
}

void SVNRunner::ExportToTemp(const wxString& src, const wxString& rev, const wxString why)
{
    TempFile dest("");
    
    SetTarget(src + "*" + dest.name);
    SetCommand(wxString("export:") + why);
    
    Run("export" + Q(src) + Q(dest.name) + "-r" +Q(rev));
}




