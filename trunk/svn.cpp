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


#include <wx/wx.h>

#include <wx/xrc/xmlres.h>
#include <wx/fs_zip.h>

#include "svn.h"

#include <manager.h>
#include <sdk_events.h>
#include <editormanager.h>

#include <cbproject.h>
#include <pipedprocess.h>

#include "dialogs.h"
#include "toolrunner.h"

#ifdef __WIN32__
const bool filenames_are_unix = false;
#else
const bool filenames_are_unix = true;
#endif


extern const wxEventType EVT_WX_SUCKS;

BEGIN_EVENT_TABLE(SubversionPlugin, cbPlugin)

EVT_MENU(ID_MENU_PREFS, SubversionPlugin::Preferences)

EVT_MENU(ID_MENU_IMPORT,   SubversionPlugin::Import)
EVT_MENU(ID_MENU_CHECKOUT,   SubversionPlugin::Checkout)

EVT_MENU(ID_MENU_USER,    SubversionPlugin::SetUser)

EVT_MENU(ID_MENU_COMMIT,    SubversionPlugin::Commit)
EVT_MENU(ID_MENU_UPDATE,   SubversionPlugin::Update)
EVT_MENU(ID_MENU_UP_P,    SubversionPlugin::Update)
EVT_MENU(ID_MENU_UP_C,    SubversionPlugin::Update)
EVT_MENU(ID_MENU_UP_B,    SubversionPlugin::Update)
EVT_MENU(ID_MENU_UP_REV,   SubversionPlugin::Update)

EVT_MENU(ID_MENU_D_H,    SubversionPlugin::Diff)
EVT_MENU(ID_MENU_D_P,    SubversionPlugin::Diff)
EVT_MENU(ID_MENU_D_C,    SubversionPlugin::Diff)
EVT_MENU(ID_MENU_D_B,    SubversionPlugin::Diff)
EVT_MENU(ID_MENU_D_REV,    SubversionPlugin::Diff)
EVT_MENU(ID_MENU_PATCH,    SubversionPlugin::Patch)
EVT_MENU(ID_MENU_PATCH_F_CVS,    SubversionPlugin::Patch)

EVT_MENU(ID_MENU_RESTORE,   SubversionPlugin::Update)

EVT_MENU(ID_MENU_REVERT,   SubversionPlugin::Revert)

EVT_MENU(ID_MENU_BRANCH,   SubversionPlugin::OnFatTortoiseFunctionality)
EVT_MENU(ID_MENU_SWITCH,   SubversionPlugin::OnFatTortoiseFunctionality)
EVT_MENU(ID_MENU_MERGE,    SubversionPlugin::OnFatTortoiseFunctionality)
EVT_MENU(ID_MENU_CREATE,   SubversionPlugin::OnFatTortoiseFunctionality)
EVT_MENU(ID_MENU_RELOCATE,   SubversionPlugin::OnFatTortoiseFunctionality)

EVT_MENU(ID_MENU_CVS_BRANCH,  SubversionPlugin::OnFatTortoiseCVSFunctionality)
EVT_MENU(ID_MENU_CVS_TAG,   SubversionPlugin::OnFatTortoiseCVSFunctionality)
EVT_MENU(ID_MENU_CVS_MERGE,   SubversionPlugin::OnFatTortoiseCVSFunctionality)
EVT_MENU(ID_MENU_CVS_PATCH,   SubversionPlugin::OnFatTortoiseCVSFunctionality)

EVT_MENU(ID_MENU_CVS_UPDATE,  SubversionPlugin::CVSUpdate)
EVT_MENU(ID_MENU_CVS_UPDATE_R,  SubversionPlugin::CVSUpdate)
EVT_MENU(ID_MENU_CVS_UPDATE_D,  SubversionPlugin::CVSUpdate)
EVT_MENU(ID_MENU_CVS_COMMIT,  SubversionPlugin::Commit)
EVT_MENU(ID_MENU_RESOLVETOOL,  SubversionPlugin::EditConflicts)
EVT_MENU(ID_MENU_RESOLVED,  SubversionPlugin::Resolved)

EVT_MENU(ID_MENU_ADD,    SubversionPlugin::Add)
EVT_MENU(ID_MENU_DELETE,   SubversionPlugin::Delete)

EVT_MENU(ID_MENU_PROP_IGNORE,  SubversionPlugin::PropIgnore)
EVT_MENU(ID_MENU_PROP_MIME,   SubversionPlugin::PropMime)

EVT_MENU(ID_MENU_PROP_EXECUTABLE, SubversionPlugin::PropExec)
EVT_MENU(ID_MENU_PROP_NEEDSLOCK, SubversionPlugin::PropNeedsLock)
EVT_MENU(ID_MENU_PROP_EXTERNALS, SubversionPlugin::PropExt)

EVT_MENU(ID_MENU_LOCK, SubversionPlugin::Lock)
EVT_MENU(ID_MENU_UNLOCK, SubversionPlugin::Lock)

EVT_MENU(ID_MENU_KW_DATE,   SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_REVISION,  SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_AUTHOR,   SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_HEAD,   SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_ID,    SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_SETALL,   SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_CLEARALL,  SubversionPlugin::PropKeywords)

EVT_MENU(ID_MENU_PROP_NEW,   SubversionPlugin::EditProperty)

EVT_TIMER(-1, SubversionPlugin::OnTimer)

SCREW_THIS_MACRO_ABUSE(TRANSACTION_SUCCESS, SubversionPlugin::TransactionSuccess)
SCREW_THIS_MACRO_ABUSE(TRANSACTION_FAILURE, SubversionPlugin::TransactionFailure)
SCREW_THIS_MACRO_ABUSE(RUN_AGAIN,   SubversionPlugin::ReRun)
SCREW_THIS_MACRO_ABUSE(RUN_NEXT_IN_QUEUE, SubversionPlugin::ReRun)


END_EVENT_TABLE()


cbPlugin* GetPlugin()
{
    return new SubversionPlugin;
}

SubversionPlugin::SubversionPlugin() :     cascade_menu(true), auto_add(true), auto_add_only_project(true),
        auto_delete(false), force_clean(false), require_comments(true), prefill_comments(true), avoid_out_of_date(true),
        no_ask_revertable(true), never_ask(false), warn_revert(true), full_status_on_startup(false), no_props(false),
        show_resolved(false), prompt_reload(false), up_after_co(true), verbose(true)
{
    wxFileSystem::AddHandler(new wxZipFSHandler);
    wxXmlResource::Get()->InitAllHandlers();
    wxXmlResource::Get()->Load(ConfigManager::Get()->Read("data_path", wxEmptyString) + "/svn.zip#zip:*.xrc");
    
    m_PluginInfo.name = "svn";
    m_PluginInfo.title = "Subversion";
    
    {
        wxString rev("$Revision$"); // let svn:keywords fill in the revision number
        wxString date("$Date$");
        rev.Replace("$", "");     // but make it a bit prettier
        wxRegEx reg("(\\(.*\\))", wxRE_ICASE);
        if(reg.Matches(date))
            date = reg.GetMatch(date, 1);
            
        m_PluginInfo.version = "0.3   " + rev + date;
    }
    
    wxString repo("$HeadURL$");
    repo = repo.Mid(repo.Index(' ')+1).BeforeLast('/') + "/";
    
    m_PluginInfo.description = "code::blocks revision control using subversion\n\n"
                               "Subversion is an advanced revision control system intended to replace CVS.\n\n"
                               "If you develop under Windows, do not forget to get TortoiseSVN as well.\n\n"
                               "References:\nhttp://subversion.tigris.org\nhttp://tortoisesvn.tigris.org\n\n"
                               "Subversion access to the cb-svn project is available at:\n"
                               + repo;
                               
    m_PluginInfo.author = "Thomas Denk";
    m_PluginInfo.authorEmail = "cb-svn@users.berlios.de";
    
    m_PluginInfo.authorWebsite = "http://cb-svn.berlios.de";
    m_PluginInfo.thanksTo = wxEmptyString;
    
    m_PluginInfo.license = "This program is licensed under the terms of the\n\n"
                           "GNU General Public License Version 2, June 1991\n"
                           "http://www.gnu.org/copyleft/gpl.htm\n\n";
                           
    m_PluginInfo.hasConfigure = true;
    
}

void SubversionPlugin::OnAttach()
{
    Log::Instance();   // avoid lazy creation
    
    clearTimer.SetOwner(this);
    
    ReadConfig();
    
    svn = new SVNRunner(svnbinary);
    svn->RunBlind(""); // get svn into the file cache asynchronously, as we'll need it soon
    
    /* For some reason, if you do not start Tortoise from cmd, then it won't work. I have no idea why, and neither
    *  have the guys developing Tortoise. There is no good reason why it should not work, really.
    *  It is apparently not specifically my code, though. It rather seems to be that wxExecute is broken somehow,
    *  since Tortoise does work fine if you use *anything* except wxExecute() to call it
    *  - including popen(), CreateProcess(), execv(), and system().
    *  On the other hand, Tortoise does not even run using the original "exec" sample from wxWindows, which we may assume does
    *  implement wxExecute() "correctly".
    *  I would use popen() as the much better alternative (not only does it work, it also reduces code size by 20kB!),
    *  but could not live with a DOS window constantly popping up under Windows.
    */
    tortoise = tortoiseproc.IsEmpty() ? 0 : new TortoiseRunner("cmd /C " + tortoiseproc);
    
    cvs = cvsbinary.IsEmpty() ? 0 : new CVSRunner(cvsbinary);
    tortoisecvs = tortoiseact.IsEmpty() ? 0 : new TortoiseCVSRunner("cmd /C " + tortoiseact);
    
    if(!extdiff.IsEmpty())
    {
#ifdef __WIN32__
        const char* shell = "cmd /C ";
#else
        
        const char* shell = "bash ";
#endif
        
        if(extdiff.Contains(".tcl"))
            diff3 = new DiffRunner("tclsh " + extdiff);
        else
            diff3 = extdiff.IsEmpty() ? 0 : new DiffRunner(shell + extdiff);
    }
    
}

void SubversionPlugin::OnRelease(bool appShutDown)
{
    WriteConfig();
    Manager::Get()->GetAppWindow()->SetStatusText("Waiting for in-progress transactions to finish...");
    Log::Instance()->fg();
    if(svn && svn->Running())
        svn->Finish();
    if(cvs && cvs->Running())
        cvs->Finish();
    Manager::Get()->GetAppWindow()->SetStatusText("");
    TempFile::Cleanup();
}

void SubversionPlugin::OnTimer(wxTimerEvent& event)
{
    TempFile::CleanupCheck();
    if (Log::lastLogTime < wxGetLocalTime() - 180)
    {
        Log::Instance()->Reduce();
        clearTimer.Start(360000);
    }
    else
        clearTimer.Start(120000);
}

void SubversionPlugin::BuildModuleMenu(const ModuleType type, wxMenu* menu, const wxString& arg)
{
    wxMenu* cmenu;
    
    if (!menu || !m_IsAttached || type != mtProjectManager)
        return;
        
    menu->AppendSeparator();
    
    if(svn->Running()) // this may be more user-friendly than wxEnableTopLevelWindows(false)
    {
        menu->Append(ID_MENU, "Revision Control");
        menu->Enable(ID_MENU, false);
        return;
    }
    
    if(cascade_menu)
        cmenu = new wxMenu;
    else
        cmenu = menu;
        
        
    if (arg.IsEmpty())
    {
        BuildMgrMenu(cmenu);
        if(menu != cmenu)
            menu->Append( ID_MENU, "Subversion", cmenu );
            
        return;
    }
    
    wxString selected(GetSelection());
    
    if(!DirUnderVersionControl(selected) && !DirUnderCVS(selected))
    {
        cmenu->Append( ID_MENU_IMPORT, "Import Project (currently unversioned)" );
        
        if(menu != cmenu)
            menu->Append( ID_MENU, "Subversion", cmenu );
        return;
    }
    
    if(cvs && DirUnderCVS(selected))
    {
        Build_CVS_ModuleMenu(cmenu, arg);
        if (tortoisecvs && IsProject(arg))
        {
            cmenu->AppendSeparator();
            cmenu->Append( ID_MENU_CVS_BRANCH, "[cvs] Branch..." );
            cmenu->Append( ID_MENU_CVS_TAG,  "[cvs] Tag..." );
            cmenu->Append( ID_MENU_CVS_MERGE,  "[cvs] Merge..." );
            cmenu->Append( ID_MENU_CVS_PATCH,  "[cvs] Create patch..." );
        }
        if(menu != cmenu)
            menu->Append( ID_MENU, "Subversion", cmenu );
        return;
    }
    
    svn->Status(selected);
    
    if (IsProject(arg))
        BuildProjectMenu(cmenu, arg, selected);
    else
        BuildFileMenu(cmenu, arg, selected);
        
        
    if(menu != cmenu)
        menu->Append( ID_MENU, "Subversion", cmenu );
}

void SubversionPlugin::Build_CVS_ModuleMenu(wxMenu* menu, const wxString& arg)
{
    assert(cvs);
    
    menu->Append( ID_MENU_CVS_COMMIT, "[cvs] Commit" );
    menu->AppendSeparator();
    menu->Append( ID_MENU_CVS_UPDATE, "[cvs] Update" );
    menu->Append( ID_MENU_CVS_UPDATE_R, "[cvs] Update to revision..." );
    menu->Append( ID_MENU_CVS_UPDATE_D, "[cvs] Update to date..." );
    if (!IsProject(arg))
    {
        menu->AppendSeparator();
        menu->Append( ID_MENU_PATCH_F_CVS,  "[cvs] Create patch..." );
    }
    else
    {
        menu->AppendSeparator();
        menu->Append( ID_MENU_CVS_LOGIN, "[cvs] Login" );
        menu->Enable(ID_MENU_CVS_LOGIN, false);
    }
}

void SubversionPlugin::BuildMgrMenu(wxMenu* menu)
{
    menu->Append( ID_MENU_CHECKOUT, "Checkout...", "Instantiate a working copy that you can safely modify. This requires a revision controlled project." );
    if(!tortoiseproc.IsEmpty())
    {
        menu->AppendSeparator();
        menu->Append( ID_MENU_CREATE, "Create repository..." );
    }
    menu->AppendSeparator();
    menu->Append( ID_MENU_PREFS, "Preferences..." );
}

void SubversionPlugin::BuildFileMenu(wxMenu* menu, wxString name, wxString target)
{
    char status = 0;
    char pstatus = 0;
    bool locked = false;
    int n = svn->std_out.GetCount();
    for(int i = 0; i < n; ++i)
    {
        if(svn->std_out[i].Mid(7) == target )
        {
            status = (svn->std_out[i])[(size_t)0];
            pstatus = (svn->std_out[i])[(size_t)1];
            locked =  ((svn->std_out[i])[(size_t)5] == 'K');
            break;
        }
    }
    
    if(status == '?')
    {
        menu->Append( ID_MENU_ADD, "Add (currently not versioned)" );
        return;
    }
    
    if(status == '!')
    {
        menu->Append( ID_MENU_RESTORE, "Restore (file is missing)" );
        menu->AppendSeparator();
        menu->Append( ID_MENU_DELETE,  "Remove from version control" );
        return;
    }
    
    if(status == 'M' || status == 'A' || status == 'D' || pstatus == 'M')
    {
        menu->Append( ID_MENU_COMMIT, "Commit", "Copy your local modifications to a new revision in the repository" );
        menu->AppendSeparator();
    }
    
    if(status == 'C' && (tortoise || diff3))
    {
        menu->Append( ID_MENU_RESOLVETOOL, "Edit file conflicts");
        menu->Append( ID_MENU_RESOLVED, "Resolved!");
        menu->AppendSeparator();
    }
    if(pstatus == 'C' && status != 'C') // do not allow to resolve properties if files are in conflict!
    {
        menu->Append( ID_MENU_RESOLVEPROP, "Resolve property conflicts");
        menu->AppendSeparator();
    }
    
    
    AppendCommonMenus(menu, target, false, locked);
    
    menu->AppendSeparator();
    wxMenu *sub = new wxMenu;
    sub->Append( ID_MENU_D_H, "HEAD" );
    sub->Append( ID_MENU_D_P, "PREV" );
    sub->Append( ID_MENU_D_C, "COMMITTED" );
    sub->Append( ID_MENU_D_B, "BASE" );
    sub->Append( ID_MENU_D_REV, "Revision..." );
    menu->Append( ID_MENU, "DIFF against...", sub );
    
    
    if(status == 'M' || status == 'A' || status == 'D' || pstatus == 'M')
    {
        menu->AppendSeparator();
        menu->Append(ID_MENU_REVERT, "Revert");
    }
    
    menu->AppendSeparator();
    menu->Append( ID_MENU_DELETE, "Delete", "Remove from project, delete, and schedule for deletion from repository. Can be reverted." );
}

void SubversionPlugin::BuildProjectMenu(wxMenu* menu, wxString name, wxString target)
{
    int fm = 0;
    int fa = 0;
    int fd = 0;
    int pm = 0;
    int cf = 0;
    int ms = 0;
    int lk = 0;
    int n = svn->std_out.GetCount();
    
    for(int i = 0; i < n; ++i)
    {
        if((svn->std_out[i])[(size_t)0] == 'M' )
            ++fm;
        if((svn->std_out[i])[(size_t)1] == 'M' )
            ++pm;
        if((svn->std_out[i])[(size_t)0] == 'A' )
            ++fa;
        if((svn->std_out[i])[(size_t)0] == 'D' )
            ++fd;
        if((svn->std_out[i])[(size_t)0] == 'C' || (svn->std_out[i])[(size_t)1] == 'C')
            ++cf;
        if((svn->std_out[i])[(size_t)0] == '!')
            ++ms;
        if((svn->std_out[i])[(size_t)5] == 'K')
            ++lk;
    }
    
    if(cf)
    {
        if(tortoise || diff3)
        {
            wxString comstr("Edit conflicts (");
            comstr << cf << " file" << (cf == 1 ? "" : "s") << ")";
            menu->Append( ID_MENU_RESOLVETOOL, comstr );
            menu->Append( ID_MENU_RESOLVED, "Resolved!");
            menu->AppendSeparator();
        }
        menu->Append( ID_MENU_REVERT, "Revert..." );
        menu->AppendSeparator();
    }
    else if(fm || pm || fa)
    {
        wxString comstr("Commit (");
        
        if(fm)
            comstr << fm << " file" << (fm == 1 ? "" : "s");
        if(pm)
            comstr << (fm ? ", " : "") << pm << (pm == 1 ? " property" : " properties");
        comstr << " modified";
        if(fa)
            comstr << ((fm || pm) ? ", " : "") << fa << " file" << (fa == 1 ? "" : "s") << " added";
        if(fd)
            comstr << ((fm || pm || fa) ? ", " : "") << fd << " file" << (fd == 1 ? "" : "s") << " deleted";
        if(ms)
            comstr << ((fm || pm || fa || fd || cf) ? ", " : "") << ms << " file" << (ms == 1 ? "" : "s") << " missing";
        if(lk)
            comstr << ((fm || pm || fa || fd || cf || ms) ? ", " : "") << lk << " lock" << (lk == 1 ? "" : "s") << " held";
            
        comstr << ")";
        
        menu->Append( ID_MENU_COMMIT, comstr );
        menu->AppendSeparator();
        menu->Append( ID_MENU_REVERT, "Revert..." );
        menu->AppendSeparator();
    }
    
    AppendCommonMenus(menu, target, true, false);
    
    if(! tortoiseproc.IsEmpty())
    {
        menu->AppendSeparator();
        menu->Append( ID_MENU_BRANCH,  "Branch..." );
        menu->Append( ID_MENU_SWITCH,  "Switch..." );
        menu->Append( ID_MENU_MERGE,  "Merge..." );
        menu->Append( ID_MENU_RELOCATE, "Relocate..." );
    }
    menu->AppendSeparator();
    menu->Append( ID_MENU_USER, "Set User...", "Set (or change) username and password for the next transaction.");
}


void SubversionPlugin::AppendCommonMenus(wxMenu *menu, wxString target, bool isProject, bool isLocked)
{
    menu->Append( ID_MENU_UPDATE, "Update (HEAD)", "Update (overwrite) your local copy with a file from the repository" );
    wxMenu *sub = new wxMenu;
    sub->Append( ID_MENU_UP_P, "PREV" );
    sub->Append( ID_MENU_UP_C, "COMMITTED" );
    sub->Append( ID_MENU_UP_B, "BASE" );
    sub->Append( ID_MENU_UP_REV, "Revision..." );
    menu->Append( ID_MENU, "Update to...", sub );
    
    if(!fileProperties.empty())
    {
        IdToStringHash::iterator it;
        for( it = fileProperties.begin(); it != fileProperties.end(); ++it ) // remove all the old rubbish
            Disconnect(it->first);            // from the event tables
        fileProperties.clear();
    }
    
    if(no_props)
        return;
        
    wxArrayString props = svn->GetPropertyList(target);
    
    
    bool has_ignore = false;
    bool has_keywords = false;
    bool has_exec = false;
    bool has_mime = false;
    bool has_externals = false;
    bool has_needslock = false;
    
    int n = props.GetCount();
    for(int i = 0; i < n; ++i)
    {
        has_ignore |= (props[i] == "svn:ignore");
        has_keywords |= (props[i] == "svn:keywords");
        has_exec  |= (props[i] == "svn:executable");
        has_mime  |= (props[i] == "svn:mime-type");
        has_externals |= (props[i] == "svn:externals");
        has_needslock |= (props[i] == "svn:needs-lock");
    }
    
    menu->AppendSeparator();
    
    wxMenu* keywords = new wxMenu;
    wxMenu* svnprops = new wxMenu;
    if(isProject)
    {
        keywords->Append( ID_MENU_KW_SETALL, "Set all" );
        keywords->Append( ID_MENU_KW_CLEARALL, "Clear all" );
        svnprops->Append( ID_MENU, "Global->svn:keywords", keywords );
    }
    else
    {
        keywords->Append( ID_MENU_KW_DATE, "Date", "", wxITEM_CHECK );
        keywords->Append( ID_MENU_KW_REVISION, "Revision", "", wxITEM_CHECK );
        keywords->Append( ID_MENU_KW_AUTHOR, "Author", "", wxITEM_CHECK );
        keywords->Append( ID_MENU_KW_HEAD, "HeadURL", "", wxITEM_CHECK );
        keywords->Append( ID_MENU_KW_ID, "Id", "", wxITEM_CHECK );
        svnprops->Append( ID_MENU, "keywords", keywords );
    }
    
    
    if(isProject)
    {
        svnprops->Append( ID_MENU_PROP_IGNORE, has_ignore ? "svn:ignore" : "svn:ignore [none]" );
        svnprops->Append( ID_MENU_PROP_EXTERNALS, has_externals ? "svn:externals" : "svn:externals [none]" );
    }
    else
    {
        wxFileName fn(target);
        fn.MakeRelativeTo(GetSelectionsProject());
        wxString f = fn.GetPath().IsEmpty() ? "Global->" : fn.GetPath()+"->";
        
        svnprops->Append( ID_MENU_PROP_EXECUTABLE, "executable", "", wxITEM_CHECK );
        svnprops->Append( ID_MENU_PROP_NEEDSLOCK, "needs lock", "", wxITEM_CHECK );
        svnprops->Append( ID_MENU_PROP_MIME, has_mime ? "mime-type" : "mime-type [default]" );
        
        svnprops->Check(ID_MENU_PROP_EXECUTABLE, has_exec);
        svnprops->Check(ID_MENU_PROP_NEEDSLOCK, has_needslock);
        svnprops->AppendSeparator();
        
        svnprops->Append( ID_MENU_PROP_IGNORE, has_ignore ? f+"svn:ignore" : f+"svn:ignore  [none]" );
        svnprops->Append( ID_MENU_PROP_EXTERNALS, has_externals ? f+"svn:externals" : f+"svn:externals  [none]" );
        
    }
    
    if(has_keywords)
    {
        wxString kw = svn->PropGet(target, "svn:keywords");
        keywords->Check(ID_MENU_KW_DATE,  kw.Contains("Date"));
        keywords->Check(ID_MENU_KW_REVISION, kw.Contains("Revision"));
        keywords->Check(ID_MENU_KW_AUTHOR, kw.Contains("Author"));
        keywords->Check(ID_MENU_KW_HEAD,  kw.Contains("HeadURL"));
        keywords->Check(ID_MENU_KW_ID,  kw.Contains("Id"));
    }
    
    wxMenu* propsub = new wxMenu;
    propsub->Append( ID_MENU, "svn", svnprops );
    propsub->Append( ID_MENU_PROP_NEW, "New Property");
    
    
    props.Remove("svn:ignore");
    props.Remove("svn:keywords");
    props.Remove("svn:executable");
    props.Remove("svn:mime-type");
    props.Remove("svn:externals");
    
    if(props.Count())
    {
        propsub->AppendSeparator();
        int tid ;
        for(int i = 0; i < props.Count(); ++i)
        {
            tid = wxNewId();
            propsub->Append(tid, props[i]);
            fileProperties[tid] = props[i];
            Connect(tid, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &SubversionPlugin::EditProperty);
        }
    }
    
    
    menu->Append( ID_MENU, "Properties...", propsub );
    menu->AppendSeparator();
    
    if(isLocked)
    {
        menu->Append(ID_MENU_UNLOCK, "Release Lock");
        menu->AppendSeparator();
    }
    else if(has_needslock)
    {
        menu->Append(ID_MENU_LOCK, "Acquire Lock");
        menu->AppendSeparator();
    }
    
    menu->Append( ID_MENU_PATCH, "Create Patch" );
}



void SubversionPlugin::Update(wxCommandEvent& event)
{
    wxString revision("HEAD");
    wxString selected(GetSelection());
    
    switch(event.GetId())
    {
        case ID_MENU_UPDATE:
        break;
        case ID_MENU_UP_P:
        revision = "PREV";
        break;
        case ID_MENU_UP_C:
        revision = "COMMITTED";
        break;
        case ID_MENU_UP_B:
        revision = "BASE";
        break;
        case ID_MENU_UP_REV:
        {
            wxTextEntryDialog d(NULL,
                                "Please enter:\n"
                                "- a revision number,\n"
                                "- a revision keyword (HEAD, COMMITTED, PREV, BASE),\n"
                                "- a revision date, time, or date-time in curly braces.\n\n"
                                "Examples of valid revision dates are:\n"
                                "{2002-02-17}  {15:30}  {2002-02-17 15:30}  {20020217T1530}",
                                "Update to revision...");
            d.ShowModal();
            
            revision = d.GetValue();
            if(revision == wxEmptyString)
                return;
        }
        break;
    }
    
    DisableCheckExternals();
    svn->Update(selected, revision);
}

void SubversionPlugin::CVSUpdate(wxCommandEvent& event)
{
    wxString revision;
    wxString date;
    
    wxString selected(GetSelection());
    
    if(event.GetId() == ID_MENU_CVS_UPDATE_R)
    {
        wxTextEntryDialog d(NULL,
                            "Please enter a revision tag:",
                            "Update to revision...");
        d.ShowModal();
        revision = d.GetValue();
        if(revision == wxEmptyString)
            return;
    }
    if(event.GetId() == ID_MENU_CVS_UPDATE_D)
    {
        wxTextEntryDialog d(NULL,
                            "Please enter a revision date.\n"
                            "meta dates such as 'yesterday' or 'last week' are allowed.",
                            "Update to date...");
        d.ShowModal();
        date = d.GetValue();
        if(date == wxEmptyString)
            return;
    }
    cvs->Update(selected, revision, date);
}


void SubversionPlugin::ReloadEditors(const wxArrayString& filenames)
{
    if(prompt_reload && wxMessageDialog(Manager::Get()->GetAppWindow(),
                                        "Do you want to replace the editor's out-of-date versions with their respective updated versions?",
                                        "Reload editor", wxYES_NO).ShowModal() == wxID_NO)
        return;
        
    EditorManager *em = Manager::Get()->GetEditorManager();
    assert(em);
    for(int i = 0; i < filenames.Count(); ++i)
        if(cbEditor *e = em->GetBuiltinEditor(filenames[i]))
        {
#ifdef LOTS_OF_DEBUG_OUTPUT
            Log::Instance()->Add("Reload "+filenames[i]);
#endif
            
            e->Reload();
        }
}

void SubversionPlugin::ReOpenEditor(const wxString& filename)
{
    EditorManager *em = Manager::Get()->GetEditorManager();
    assert(em);
    if(cbEditor *e = em->GetBuiltinEditor(filename))
    {
        e->Close();
        em->Open(filename);
    }
}

void SubversionPlugin::Diff(wxCommandEvent& event)
{
    wxString revision("HEAD");
    wxString selected(GetSelection());
    
    switch(event.GetId())
    {
        case ID_MENU_D_P:
        revision = "PREV";
        break;
        case ID_MENU_D_C:
        revision = "COMMITTED";
        break;
        case ID_MENU_D_B:
        revision = "BASE";
        break;
        case ID_MENU_D_REV:
        {
            wxTextEntryDialog d(NULL,
                                "Please enter:\n"
                                "- a revision number,\n"
                                "- a revision keyword (HEAD, COMMITTED, PREV, BASE),\n"
                                "- a revision date, time, or date-time in curly braces.\n\n"
                                "Examples of valid revision dates are:\n"
                                "{2002-02-17}  {15:30}  {2002-02-17 15:30}  {20020217T1530}",
                                "Compare to revision...");
            d.ShowModal();
            
            revision = d.GetValue();
            if(revision == wxEmptyString)
                return;
        }
        break;
    }
    svn->ExportToTemp(selected, revision, "diff");
}

void SubversionPlugin::Patch(wxCommandEvent& event)
{
    wxString selected(GetSelection());
    
    wxString fn((selected.IsEmpty() ? "patchfile" : selected) + ".patch");
    
    patchFileName = wxFileSelector("Save patch file as...", "", fn,
                                   "patch files (*.patch)|*.patch|text files (*.txt)|*.txt|all files (*.*)|*.*", "*.patch", wxSAVE);
                                   
    if(!patchFileName.IsEmpty())
        if(event.GetId() == ID_MENU_PATCH_F_CVS)
            cvs->Diff(selected);
        else
            svn->Diff(selected, "HEAD");
}

void SubversionPlugin::Lock(wxCommandEvent& event)
{
    wxString selected(GetSelection());
    
    if(wxFileName::DirExists(selected))
        return; // no locking of directories
        
    if(event.GetId() == ID_MENU_LOCK)
        svn->Lock(selected);
    else
        svn->UnLock(selected);
}


void SubversionPlugin::Commit(wxCommandEvent& event)
{
    wxString selected(GetSelection());
    
    if(DirUnderCVS(selected))
    {
        wxArrayString files;
        CommitDialog d(Manager::Get()->GetAppWindow(), files, require_comments);
        d.Centre();
        
        if(d.ShowModal() == wxID_OK)
            cvs->Commit(selected, d.comment);
        return;
    }
    
    svn->Status(selected);
    
    wxArrayString files;
    wxArrayString toAdd;
    
    wxArrayString missing = ExtractFilesWithStatus('?');
    
    if(auto_add)
        files = wxArrayString();
    else
        files = missing;
        
    CommitDialog d(Manager::Get()->GetAppWindow(), files, require_comments);
    if(prefill_comments)
    {
        wxArrayString modified = ExtractFilesWithStatus('M');
        wxString prefill;
        
        for(int i = 0; i < modified.Count(); ++i)
        {
            modified[i] = wxFileName(modified[i]).GetFullName();
            prefill << modified[i] << " : \n";
        }
        d.SetComment(prefill);
    }
    d.Centre();
    
    if(d.ShowModal() == wxID_OK)
    {
        if(auto_delete)
        {
            missing = ExtractFilesWithStatus('!');
            wxString concat;
            for(unsigned int i = 0; i < missing.Count(); ++i)
                concat << " \"" << missing[i] << "\" ";
            concat = concat.Mid(2, concat.Length()-4);
            if(!concat.IsEmpty())
                svn->Delete(concat);
        }
        if(auto_add)
        {
            if(auto_add_only_project)
            {
                ProjectsArray* array = Manager::Get()->GetProjectManager()->GetProjects();
                if (array)
                {
                    int n = array->GetCount();
                    
                    for (unsigned int k = 0; k < missing.Count(); ++k)
                        for (unsigned int i = 0; i < n; ++i)
                        {
                            cbProject* proj = array->Item(i);
                            if (proj && proj->GetFileByFilename(missing[k], false, filenames_are_unix))
                            {
                                toAdd.Add(missing[k]);
                                break;
                            }
                        }
                }
                
            }
            else
                toAdd = missing;
        }
        else
            toAdd = d.finalList;
            
        wxString concat;
        for(unsigned int i = 0; i < toAdd.Count(); ++i) // better call svn with 637 paramters than run svn 637 times...
            concat << " \"" << toAdd[i] << "\" ";
        concat = concat.Mid(2, concat.Length()-4);  // svn->Add() wraps in double quotes already
        
        if(!concat.IsEmpty())
            svn->Add(concat);
            
        DisableCheckExternals();
        
        svn->Commit(selected, d.comment, avoid_out_of_date);
    }
}

void SubversionPlugin::Checkout(wxCommandEvent& event)
{
    ProjectManager *pmgr = Manager::Get()->GetProjectManager();
    CheckoutDialog d(Manager::Get()->GetAppWindow(), repoHistory, repoHistoryCVS, defaultCheckoutDir);
    d.Centre();
    if(d.ShowModal() == wxID_OK)
    {
        if(d.use_cvs_instead)
        {
            if(!d.cvs_pass.IsEmpty())
                cvs->Login(d.cvs_proto, d.cvs_repo, d.cvs_user, d.cvs_pass);
                
            if(d.cvs_workingdir.IsEmpty())
                d.cvs_workingdir = GetCheckoutDir();
                
            repoHistoryCVS.Add(d.cvs_repo);
            cvs->Checkout(d.cvs_proto, d.cvs_repo, d.cvs_module, d.cvs_workingdir, d.cvs_user, d.cvs_revision);
            
            request_autoopen = d.cvs_auto_open;
            return;
        }
        
        if(!d.username.IsEmpty())
            svn->SetPassword(d.username, d.password);
            
        if(d.checkoutDir.IsEmpty())
            d.checkoutDir = GetCheckoutDir();
            
        request_autoopen = d.autoOpen;
        
        svn->Checkout(d.repoURL, d.checkoutDir, (d.revision.IsEmpty() ? wxString("HEAD") : d.revision), d.noExternals );
        svn->Info(d.checkoutDir, false);
        
        repoHistory.Add(d.repoURL);
    }
}

void SubversionPlugin::AutoOpenProjects(const wxString& rootdir, bool recursive, bool others)
{
    ProjectManager *pmgr = Manager::Get()->GetProjectManager();
    wxArrayString dirs;
    wxString f;
    
    dirs.Add(rootdir);
    if(recursive)
    {
        f = wxFindFirstFile(rootdir + "/", wxDIR);
        while ( !f.IsEmpty() )
        {
            dirs.Add(f);
            f = wxFindNextFile();
        }
    }
    for(int i = 0; i < dirs.Count(); ++i)
    {
        f = wxFindFirstFile(dirs[i] + "/*.cbp");
        while ( !f.IsEmpty() )
        {
            if(verbose)
                Log::Instance()->Add("opening " + f);
            pmgr->LoadProject(f);
            f = wxFindNextFile();
        }
        if(others)
        {
            f = wxFindFirstFile(dirs[i] + "/*.dev");
            while ( !f.IsEmpty() )
            {
                if(verbose)
                    Log::Instance()->Add("importing " + f);
                pmgr->LoadProject(f);
                f = wxFindNextFile();
            }
            f = wxFindFirstFile(dirs[i] + "/*.dsp");
            while ( !f.IsEmpty() )
            {
                if(verbose)
                    Log::Instance()->Add("importing " + f);
                pmgr->LoadProject(f);
                f = wxFindNextFile();
            }
        }
    }
    
}



void SubversionPlugin::Import(wxCommandEvent& event)
{
    ImportDialog d(Manager::Get()->GetAppWindow(), repoHistory, GetSelectionsProject(), require_comments);
    d.Centre();
    if(d.ShowModal() == wxID_OK)
    {
        if(!d.username.IsEmpty())
            svn->SetPassword(d.username, d.password);
            
        wxString target(defaultCheckoutDir + "/" +DirName(d.importDir));
        
        request_autoopen = true;
        repoHistory.Add(d.repoURL);
        
        cbProject *p = GetCBProject();
        if(p)
        {
            ProjectManager *pmgr = Manager::Get()->GetProjectManager();
            pmgr->CloseProject(p);
        }
        
        svn->Import(d.repoURL, d.importDir, d.comment);
        svn->Checkout(d.repoURL, target, "HEAD");
        
        if(!d.keywords.IsEmpty())
            svn->PropSet(target, "svn:keywords", d.keywords, true);
            
        if(!d.ignore.IsEmpty())
            svn->PropSet(target, "svn:ignore", d.ignore, true);
            
        if(!d.externals.IsEmpty())
            svn->PropSet(target, "svn:externals", d.externals, true);
            
        if(!d.copy.IsEmpty())
            svn->PropSet(target, "copyright", d.copy, true);
            
        if(!d.home.IsEmpty())
            svn->PropSet(target, "home", d.home, true);
            
        if(!d.docs.IsEmpty())
            svn->PropSet(target, "documentation", d.docs, true);
            
        if(!d.contact.IsEmpty())
            svn->PropSet(target, "contact", d.contact, true);
            
        if(!d.arch.IsEmpty())
            svn->PropSet(target, "architecture", d.arch, true);
    }
}


void SubversionPlugin::SetUser(wxCommandEvent& event)
{

    PasswordDialog p(Manager::Get()->GetAppWindow());
    p.Centre();
    if(p.ShowModal() == wxID_OK)
        svn->SetPassword(p.username, p.password);
    svn->Status(GetSelection(), true);
}



void SubversionPlugin::Revert(wxCommandEvent& event)
{
    wxString target(GetSelection());
    
    svn->Status(target);
    
    wxArrayString mods;
    wxArrayString files;
    
    int n = svn->std_out.Count();
    for(int i = 0; i < n; ++i)
    {
        if(svn->std_out[i][(size_t)0] == '?')
            continue;
            
        wxString f(LocalPath(svn->std_out[i].Mid(7)));
        if(f.StartsWith(".."))
            f = "[project]";
        wxString s;
        if(svn->std_out[i][(size_t)0] == '!')
            s << "missing";
        if(svn->std_out[i][(size_t)0] == 'D')
            s << "deleted";
        if(svn->std_out[i][(size_t)0] == 'A')
            s << (s.Length()? ", " : "") <<  "added";
        if(svn->std_out[i][(size_t)0] == 'M')
            s << (s.Length()? ", " : "") <<  "modified";
        if(svn->std_out[i][(size_t)0] == 'C')
            s << (s.Length()? ", " : "") <<  "modified and in conflict";
        if(svn->std_out[i][(size_t)1] == 'M')
            s << (s.Length()? ", " : "") <<  "has property changes";
        s = f + "   (" + s + ")";
        
        if(svn->std_out[i].Length())
        {
            mods.Add(s);
            files.Add(svn->std_out[i].Mid(7));
        }
    }
    
    if(::wxDirExists(target)) // seems like we have a project folder here
    {
        RevertDialog d(Manager::Get()->GetAppWindow(), mods, files);
        d.Centre();
        if(d.ShowModal() == wxID_OK)
        {
            wxString toRevert;
            for(int i = 0; i < d.finalList.Count(); i++)
                toRevert << " \"" + d.finalList[i] + "\" ";
            DisableCheckExternals();
            svn->Revert(toRevert.Mid(2, toRevert.Length()-4));
        }
    }
    else
    {
        wxString localMods;
        if(warn_revert && ParseStatusOutputForFile(target) == 'M')
            localMods = "\nWARNING:\n\nThis file has local modifications which you will lose if you revert it.\n\n\n";
            
        if(never_ask || wxMessageDialog(Manager::Get()->GetAppWindow(), localMods + "Do you want to revert the file " + target + "?", (localMods.IsEmpty() ? "Revert" : "Revert over Modifications"), wxYES_NO).ShowModal() == wxID_YES)
        {
            DisableCheckExternals();
            svn->Revert(target);
        }
    }
    
}


char SubversionPlugin::ParseStatusOutputForFile(const wxString& what)
{
    int n = svn->std_out.Count();
    for(int i = 0; i < n; ++i)
    {
        if(svn->std_out[i].Contains(what))
        {
            return (svn->std_out[i])[(size_t)0];
        }
    }
    return 0;
}



/*
* We have to discriminate between lines like
* U    somefile.h
* UU   anotherfile.cpp
* and
* Update failed (details follow)
*
* Luckily, svn status codes are *never* lowercase, so we can use "std_out[i])[(size_t)1] < 'a'" as a condition
* A regex would certainly do even better, but a regex is 30-40 times slower, too.
* We don't know in advance how many files there are in a transaction, it could very easily be 200 (or 5000?)
* I'd rather avoid running wxRegex.Matches() 200 times in a loop unless I have to.
*/
void SubversionPlugin::ExtractFilesWithStatus(const char what, wxArrayString& ret, unsigned int pos)
{
    wxString t;
    for(int i = 0; i < svn->std_out.GetCount(); ++i)
        if( (svn->std_out[i])[(size_t)pos] == what && (svn->std_out[i])[(size_t)1] < 'a')
            ret.Add(svn->std_out[i].Mid(5).Strip(wxString::both));
}

wxArrayString SubversionPlugin::ExtractFilesWithStatus(const char what, unsigned int pos)
{
    wxArrayString ret;
    ExtractFilesWithStatus(what, ret, pos);
    return ret;
}


void SubversionPlugin::Preferences(wxCommandEvent& event)
{
    PreferencesDialog d(Manager::Get()->GetAppWindow());
    d.Centre();
    
    XRCCTRL(d, "svn binary path", wxTextCtrl)->SetValue(wxFileName(svnbinary).GetFullPath());
    XRCCTRL(d, "cvs binary path", wxTextCtrl)->SetValue(wxFileName(cvsbinary).GetFullPath());
    XRCCTRL(d, "std checkout", wxTextCtrl)->SetValue(defaultCheckoutDir);
    
    XRCCTRL(d, "auto add", wxCheckBox)->SetValue(auto_add);
    XRCCTRL(d, "autoadd only project", wxCheckBox)->SetValue(auto_add_only_project);
    XRCCTRL(d, "auto remove missing", wxCheckBox)->SetValue(auto_delete);
    XRCCTRL(d, "auto merge", wxCheckBox)->SetValue(avoid_out_of_date);
    XRCCTRL(d, "no emtpy comments", wxCheckBox)->SetValue(require_comments);
    XRCCTRL(d, "prefill comments", wxCheckBox)->SetValue(prefill_comments);
    XRCCTRL(d, "forceclean", wxCheckBox)->SetValue(force_clean);
    XRCCTRL(d, "no confirm revertable", wxCheckBox)->SetValue(no_ask_revertable);
    XRCCTRL(d, "warn revert", wxCheckBox)->SetValue(warn_revert);
    XRCCTRL(d, "never ask", wxCheckBox)->SetValue(never_ask);
    XRCCTRL(d, "full status", wxCheckBox)->SetValue(full_status_on_startup);
    XRCCTRL(d, "no props", wxCheckBox)->SetValue(no_props);
    XRCCTRL(d, "show resolved", wxCheckBox)->SetValue(show_resolved);
    XRCCTRL(d, "cascade", wxCheckBox)->SetValue(cascade_menu);
    XRCCTRL(d, "prompt reload", wxCheckBox)->SetValue(prompt_reload);
    XRCCTRL(d, "up after co", wxCheckBox)->SetValue(up_after_co);
    XRCCTRL(d, "verbose", wxCheckBox)->SetValue(verbose);
    
    d.RadioToggle(event);
    
    if(d.ShowModal() == wxID_OK)
    {
        svnbinary     = XRCCTRL(d, "svn binary path", wxTextCtrl)->GetValue();
        cvsbinary  = XRCCTRL(d, "cvs binary path", wxTextCtrl)->GetValue();
        defaultCheckoutDir  = XRCCTRL(d, "std checkout", wxTextCtrl)->GetValue();
        
        if(!svnbinary.IsEmpty())
            if(!svn)
                svn = new SVNRunner(svnbinary);
                
        if(!cvsbinary.IsEmpty())
            if(!cvs)
                cvs = new CVSRunner(cvsbinary);
                
        svn->SetExecutable(svnbinary);
        cvs->SetExecutable(cvsbinary);
        
        auto_add     = XRCCTRL(d, "auto add", wxCheckBox)->GetValue();
        auto_add_only_project  = XRCCTRL(d, "autoadd only project", wxCheckBox)->GetValue();
        auto_delete    = XRCCTRL(d, "auto remove missing", wxCheckBox)->GetValue();
        avoid_out_of_date  = XRCCTRL(d, "auto merge", wxCheckBox)->GetValue();
        require_comments   = XRCCTRL(d, "no emtpy comments", wxCheckBox)->GetValue();
        prefill_comments   = XRCCTRL(d, "prefill comments", wxCheckBox)->GetValue();
        force_clean    = XRCCTRL(d, "forceclean", wxCheckBox)->GetValue();
        no_ask_revertable   = XRCCTRL(d, "no confirm revertable", wxCheckBox)->GetValue();
        warn_revert    = XRCCTRL(d, "warn revert", wxCheckBox)->GetValue();
        never_ask      = XRCCTRL(d, "never ask", wxCheckBox)->GetValue();
        full_status_on_startup = XRCCTRL(d, "full status", wxCheckBox)->GetValue();
        no_props     = XRCCTRL(d, "no props", wxCheckBox)->GetValue();
        show_resolved    = XRCCTRL(d, "show resolved", wxCheckBox)->GetValue();
        cascade_menu    = XRCCTRL(d, "cascade", wxCheckBox)->GetValue();
        prompt_reload    = XRCCTRL(d, "prompt reload", wxCheckBox)->GetValue();
        up_after_co    = XRCCTRL(d, "up after co", wxCheckBox)->GetValue();
        verbose    = XRCCTRL(d, "verbose", wxCheckBox)->GetValue();
        
        WriteConfig();
    }
}


void SubversionPlugin::ReadConfig()
{
    wxConfigBase* c = ConfigManager::Get();
    
    svnbinary    = c->Read("/svn/svnbinary", "unset");
    cvsbinary    = c->Read("/svn/cvsbinary");
    defaultCheckoutDir = c->Read("/svn/defaultCheckoutDir");
    
    c->Read("/svn/auto_add", &auto_add);
    c->Read("/svn/auto_add_only_project", &auto_add_only_project);
    c->Read("/svn/auto_delete", &auto_delete);
    c->Read("/svn/avoid_out_of_date", &avoid_out_of_date);
    c->Read("/svn/require_comments", &require_comments);
    c->Read("/svn/prefill_comments", &prefill_comments);
    c->Read("/svn/force_clean", &force_clean);
    c->Read("/svn/no_ask_revertable", &no_ask_revertable);
    c->Read("/svn/warn_revert", &warn_revert);
    c->Read("/svn/never_ask", &never_ask);
    c->Read("/svn/full_status_on_startup", &full_status_on_startup);
    c->Read("/svn/no_props", &no_props);
    c->Read("/svn/show_resolved", &show_resolved);
    c->Read("/svn/cascade_menu", &cascade_menu);
    c->Read("/svn/prompt_reload", &prompt_reload);
    c->Read("/svn/up_after_co", &up_after_co);
    c->Read("/svn/verbose", &verbose);
    
    TamperWithWindowsRegistry();
    
    SearchBinaries();
    
    {
        wxString ht("/svn/repoHist");
        wxString val;
        for(unsigned int i = 0; i < 16; ++i)
        {
            wxString h = ht;
            val = c->Read(h<<i);
            if(!val.IsEmpty())
                repoHistory.Add(val);
        }
    }
    {
        wxString ht("/svn/repoHistCVS");
        wxString val;
        for(unsigned int i = 0; i < 16; ++i)
        {
            wxString h = ht;
            val = c->Read(h<<i);
            if(!val.IsEmpty())
                repoHistoryCVS.Add(val);
        }
    }
}

void SubversionPlugin::WriteConfig()
{
    wxConfigBase* c = ConfigManager::Get();
    
    c->Write("/svn/svnbinary", svnbinary);
    c->Write("/svn/cvsbinary", cvsbinary);
    c->Write("/svn/defaultCheckoutDir", defaultCheckoutDir);
    
    c->Write("/svn/auto_add", auto_add);
    c->Write("/svn/auto_add_only_project", auto_add_only_project);
    c->Write("/svn/auto_delete", auto_delete);
    c->Write("/svn/avoid_out_of_date", avoid_out_of_date);
    c->Write("/svn/require_comments", require_comments);
    c->Write("/svn/prefill_comments", prefill_comments);
    c->Write("/svn/force_clean", force_clean);
    c->Write("/svn/no_ask_revertable", no_ask_revertable);
    c->Write("/svn/warn_revert", warn_revert);
    c->Write("/svn/never_ask", never_ask);
    c->Write("/svn/full_status_on_startup", full_status_on_startup);
    c->Write("/svn/no_props", no_props);
    c->Write("/svn/show_resolved", show_resolved);
    c->Write("/svn/cascade_menu", cascade_menu);
    c->Write("/svn/prompt_reload", prompt_reload);
    c->Write("/svn/up_after_co", up_after_co);
    c->Write("/svn/verbose", verbose);
    
    {
        unsigned int count = repoHistory.Count() < 16 ? repoHistory.Count() : 16;
        wxString ht("/svn/repoHist");
        for(unsigned int i = 0; i < count; ++i)
        {
            wxString h = ht;
            c->Write(h<<i, repoHistory[i]);
        }
    }
    {
        unsigned int count = repoHistoryCVS.Count() < 16 ? repoHistoryCVS.Count() : 16;
        wxString ht("/svn/repoHistCVS");
        for(unsigned int i = 0; i < count; ++i)
        {
            wxString h = ht;
            c->Write(h<<i, repoHistoryCVS[i]);
        }
    }
}


void SubversionPlugin::Add(wxCommandEvent& event)
{
    svn->Add(GetSelection());
}

void SubversionPlugin::Delete(wxCommandEvent& event)
{
    wxString selected(GetSelection());
    
    if(never_ask)
        svn->Force();
        
    if(never_ask || no_ask_revertable || wxMessageDialog(Manager::Get()->GetAppWindow(), "Issue a 'svn delete' on the file " + selected + "?\n\nThis will not only delete the from disk, but also remove it from revision control.", "Delete File", wxYES_NO).ShowModal() == wxID_YES)
    {
        svn->Delete(selected);
        if(!svn->std_err.Count())
        {
            wxTreeCtrl* tree = Manager::Get()->GetProjectManager()->GetTree();
            FileTreeData* ftd = (FileTreeData*) tree->GetItemData(tree->GetSelection());
            cbProject *p = ftd->GetProject();
            p->RemoveFile(ftd->GetFileIndex());
            Manager::Get()->GetProjectManager()->RebuildTree();
        }
        else
        {
            Log::Instance()->Add("Warning: delete failed.");
            svn->DumpErrors();
        }
    }
}



void SubversionPlugin::PropIgnore(wxCommandEvent& event)
{
    wxString target(GetSelection());
    
    if(!::wxDirExists(target))               // svn:ignore only valid on directories
        target = wxFileName(target).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
        
    wxString value = svn->PropGet(target, "svn:ignore");
    
    IgnoreEditorDialog d(Manager::Get()->GetAppWindow(), target, value, target);
    d.Centre();
    wxString localPath = LocalPath(target);
    d.SetTitle("svn:ignore" + (localPath.IsEmpty() ? "" : " on " + localPath));
    
    if(d.ShowModal() == wxID_OK)
    {
        svn->PropSet(target, "svn:ignore", d.value, false);
    }
}

void SubversionPlugin::PropMime(wxCommandEvent& event)
{
    wxString target = GetSelection();
    wxString mime = svn->PropGet(target, "svn:mime-type");
    
    wxTextEntryDialog d(Manager::Get()->GetAppWindow(),
                        "Please provide a mime-type for the file " + LocalPath(target) +
                        "\n or leave empty to use the default type.\n\n"
                        "Note that a mime type outside text/* will effectively disable all merging capabilities.",
                        "svn:mime-type on " + LocalPath(target), mime);
                        
    if(d.ShowModal() ==  wxID_OK)
    {
        mime = d.GetValue();
        if(mime.IsEmpty())
            svn->PropDel(target, "svn:mime-type");
        else
            svn->PropSet(target, "svn:mime-type", mime, false);
    }
    
}

void SubversionPlugin::PropExec(wxCommandEvent& event)
{
    if(event.IsChecked())
        svn->PropDel(GetSelection(), "svn:executable");
    else
        svn->PropSet(GetSelection(), "svn:executable", "", false);
}

void SubversionPlugin::PropNeedsLock(wxCommandEvent& event)
{
    if(event.IsChecked())
        svn->PropDel(GetSelection(), "svn:needs-lock");
    else
        svn->PropSet(GetSelection(), "svn:needs-lock", "", false);
}

void SubversionPlugin::PropExt(wxCommandEvent& event)
{
    //FIXME: svn:externals deserves its own dialog
    
    wxString file(GetSelection());
    
    if(!::wxDirExists(file))
        file = wxFileName(file).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
        
    wxString value(svn->PropGet(file, "svn:externals"));
    
    PropertyEditorDialog d(Manager::Get()->GetAppWindow(), "svn:externals", value);
    d.Centre();
    wxString loc = LocalPath(file);
    loc = loc == "" ? "[top dir]" : loc;
    d.SetTitle("Property editor: " + loc);
    
    if(d.ShowModal() == wxID_OK)
    {
        svn->PropSet(file, d.name, d.value, false);
    }
    else if(d.del && ( never_ask || no_ask_revertable || wxMessageDialog(Manager::Get()->GetAppWindow(), "Remove the property " + d.name + " from " + file + " ?", "Delete Property", wxYES_NO).ShowModal() == wxID_YES))
    {
        svn->PropDel(file, d.name);
    }
}

void SubversionPlugin::PropKeywords(wxCommandEvent& event)
{
    wxString item;
    
    switch(event.GetId())
    {
        case ID_MENU_KW_DATE:
        item << "Date";
        break;
        case ID_MENU_KW_REVISION:
        item << "Revision";
        break;
        case ID_MENU_KW_AUTHOR:
        item << "Author";
        break;
        case ID_MENU_KW_HEAD:
        item << "HeadURL";
        break;
        case ID_MENU_KW_ID:
        item << "Id";
        break;
        case ID_MENU_KW_SETALL:
        svn->PropSet(GetSelection(), "svn:keywords", "Date Revision Author HeadURL Id", true);
        return;
        case ID_MENU_KW_CLEARALL:
        svn->PropSet(GetSelection(), "svn:keywords", "", true);
        return;
    }
    wxString kw = svn->PropGet(GetSelection(), "svn:keywords");
    
    if(kw.Contains(item))
        kw.Replace(item, "");
    else
        kw << "  " << item;
        
    kw.Replace("\n", " ", true); // this is probably unneeded, but better do it anyway just in case
    kw.Replace("  ", " ", true);
    kw.Trim(false);
    
    svn->PropSet(GetSelection(), "svn:keywords", kw, true);
}


void   SubversionPlugin::EditConflicts(wxCommandEvent& event)
{
    assert(tortoise || diff3);
    
    svn->Status(svn->GetTarget());
    
    wxArrayString conflicting;
    ExtractFilesWithStatus('C', conflicting);
    
    for(int i = 0; i < conflicting.Count(); ++i)
        DoResolve(conflicting[i]);
}

void SubversionPlugin::OnFatTortoiseCVSFunctionality(wxCommandEvent& event)
{
    assert(tortoisecvs);
    
    wxString p(GetSelection());
    
    switch(event.GetId())
    {
        case ID_MENU_CVS_BRANCH:
        tortoisecvs->Branch(p);
        break;
        
        case ID_MENU_CVS_TAG:
        tortoisecvs->Tag(p);
        break;
        
        case ID_MENU_CVS_MERGE:
        tortoisecvs->Merge(p);
        break;
        
        case ID_MENU_CVS_PATCH:
        tortoisecvs->Patch(p);
        break;
    };
}





void SubversionPlugin::OnFatTortoiseFunctionality(wxCommandEvent& event)
{
    assert(event.GetId() >= ID_MENU_BRANCH && event.GetId() <= ID_MENU_RELOCATE);
    assert(tortoise);
    
    wxString p(GetSelection());
    
    switch(event.GetId())
    {
        case ID_MENU_BRANCH:
        tortoise->Branch(p);
        break;
        
        case ID_MENU_SWITCH:
        tortoise->Switch(p);
        break;
        
        case ID_MENU_MERGE:
        tortoise->Merge(p);
        break;
        
        case ID_MENU_CREATE:
        {
            wxString d(::wxDirSelector("Please point to the location where the new repository will be created."));
            if(!d.IsEmpty())
                tortoise->Create(d);
        }
        break;
        
        case ID_MENU_RELOCATE:
        tortoise->Relocate(p);
        break;
    };
}


void SubversionPlugin::EditProperty(wxCommandEvent& event)
{
    wxString name;
    wxString value;
    wxString file(GetSelection());
    
    if(event.GetId() == ID_MENU_PROP_NEW)
    {
        name = "NewPropertyName";
    }
    else
    {
        name = fileProperties[event.GetId()];
        value = svn->PropGet(file, name);
    }
    
    PropertyEditorDialog d(Manager::Get()->GetAppWindow(), name, value);
    d.Centre();
    wxString loc = LocalPath(file);
    loc = loc == "" ? "[top dir]" : loc;
    d.SetTitle("Property editor: " + loc);
    
    if(d.ShowModal() == wxID_OK)
    {
        svn->PropSet(file, d.name, d.value, false);
    }
    else if(d.del && ( never_ask || no_ask_revertable || wxMessageDialog(Manager::Get()->GetAppWindow(), "Remove the property " + d.name + " from " + file + " ?", "Delete Property", wxYES_NO).ShowModal() == wxID_YES))
    {
        svn->PropDel(file, d.name);
    }
}



void SubversionPlugin::TransactionSuccess(wxCommandEvent& event)
{
    wxString cmd(event.GetString());
    if(event.GetExtraLong() == ToolRunner::SVN)
    {
        wxArrayString conflicting;
        ExtractFilesWithStatus('C', conflicting);
        
        if(conflicting.Count())
        {
            svn->EmptyQueue();  // Although the command was successful, files in conflict are still a failure, so break here
            if(tortoise)
            {
                for(int i = 0; i < conflicting.Count(); ++i)
                    DoResolve(conflicting[i]);
                    
            }
            if(verbose)
                Log::Instance()->Blue("Transaction was successful, but conflicts remain.");
            return;
        }
        
    }
    
    wxString lastCommand(svn->LastCommand());
    
    if(cmd.IsSameAs("commit"))
    {
        if(up_after_co)
        {
            wxArrayString changed;
            for(unsigned int i = 0; i < svn->std_out.Count(); ++i)
                if(svn->std_out[i].StartsWith("Adding") || svn->std_out[i].StartsWith("Sending"))
                    changed.Add(svn->std_out[i].Mid(15).Trim());
                    
            ReloadEditors(changed);
        }
    }
    
    if(cmd.Contains("lock"))
    {
        ReOpenEditor(svn->GetTarget());
        
        // this may be a bug in svn? IMO, the transaction should fail on an existing lock
        if(svn->blob.Contains("is already locked by"))
            if(wxMessageDialog(Manager::Get()->GetAppWindow(),
                               "The resource is alredy locked by another user.\n\n"
                               "You can 'break' the existing lock and acquire your own lock on the resource if you wish to do so.\n\n"
                               "However, locking is a mechanism that should not be handled carelessly.\n"
                               "Before you think about breaking a lock, it is STRONGLY recommended that you communicate with the user currently holding that lock.\n\n"
                               "Do you want to break the lock at the risk of possibly breaking more than just a lock?",
                               "Resource locked", wxYES_NO | wxNO_DEFAULT |wxICON_EXCLAMATION).ShowModal() == wxID_YES)
            {
                svn->AddToLastCommand(" --force");
                svn->RunAgain();
                return;
            }
            else
            {
                Log::Instance()->Blue("Aborted.");
                return;
            }
    }
    
    if(cmd.IsSameAs("checkout"))
    {
        if(request_autoopen)
            AutoOpenProjects(svn->GetTarget(), true, true);
    }
    
    if(cmd.IsSameAs("info"))
    {
        wxString s;
        int n = svn->std_out.Count();
        
        for(int i = 0; i < n; ++i)
            if(svn->std_out[i].Contains("UUID") || svn->std_out[i].Contains("Revision") || svn->std_out[i].Contains("Last"))
                s << svn->std_out[i] << "\n";
        Log::Instance()->Add(s);
    }
    
    if(cmd.IsSameAs("cat"))
    {
        meow = svn->out;
    }
    
    if(cmd.IsSameAs("export:diff"))
    {
        wxString dest = svn->GetTarget();
        wxString src = dest.BeforeFirst('*');
        dest = dest.AfterFirst('*');
        
        if(tortoise)
            tortoise->Diff(src, dest);
        else if(diff3)
            diff3->Diff(src, dest);
    }
    
    if(cmd.IsSameAs("diff"))
    {
        if(!patchFileName.IsEmpty())
        {
            wxFile f(patchFileName, wxFile::write);
            f.Write(svn->out);
            patchFileName.Empty();
        }
    }
    
    // Know nothing, assume all is fine:)
    if(verbose && svn->IsIdle())
        Log::Instance()->Blue("All transactions finished.");
        
    if(cmd.IsSameAs("checkout") || cmd.IsSameAs("update") || cmd.IsSameAs("revert"))
        ResetCheckExternals();
        
    svn->RunQueue();
}


void SubversionPlugin::TransactionFailure(wxCommandEvent& event)
{
    wxString cmd(event.GetString());
    if(event.GetExtraLong() == ToolRunner::SVN)
    {
        // svn:run 'svn cleanup' to remove locks (type 'svn help cleanup' for details)
        if(svn->blob.Contains("svn cleanup"))
        {
            svn->QueueAgain();
            svn->InsertFirst();
            svn->Run("cleanup" + svn->Q(svn->GetTarget()));
            return;
        }
        
        
        // svn: Out of date: 'main.cpp' in transaction '43-1'
        if(avoid_out_of_date && svn->blob.Contains("Out of date"))
        {
            svn->QueueAgain();
            svn->InsertFirst();
            svn->Update(svn->GetTarget());
            return;
        }
        
        // svn: Server doesn't support the lock command
        // svn: Unknown command 'lock'
        if(svn->blob.Contains("Unknown command 'lock'"))
        {
            Log::Instance()->Red("Locking is a feature only available in newer versions of subversion.\n"
                                 "You need at least version 1.2.0 both on the server and on the client side.");
        }
    }
    
    if(event.GetExtraLong() == ToolRunner::CVS)
    {
        /*
        *  Now tell me that CVS does not suck...
        *  CVS exits with error if diff finds differences in files.
        *  Seriously, what do you expect if the user runs "diff"? 
        */
        if(cmd.IsSameAs("CVS-diff"))
        {
            if(!patchFileName.IsEmpty())
            {
                wxFile f(patchFileName, wxFile::write);
                f.Write(cvs->out);
                patchFileName.Empty();
            }
            if(verbose && cvs->IsIdle())
                Log::Instance()->Blue("All transactions finished.");
            return;
        }
        
        NotImplemented("Error handler for CVS.");
    }
    
    // We were not able to handle whatever errors occurred
    Log::Instance()->Red("Transaction failed.");
    svn->EmptyQueue();
}

void SubversionPlugin::ReRun(wxCommandEvent& event)
{
    int id = event.GetId();
    assert(id == RUN_AGAIN || id == RUN_NEXT_IN_QUEUE);
    
    if(id = RUN_AGAIN)
        svn->RunAgain();
    else
        svn->RunQueue();
}

void SubversionPlugin::DoResolve(const wxString& conflicting)
{
    wxBell();
    Log::Instance()->Add("resolve...");
    if(tortoise)
        tortoise->ConflictEditor(conflicting);
    else if(diff3)
    {
        /*
        * tkdiff.tcl does not understand the -m switch that is mandatory for all others.
        * Luckily, it has a much easier way to handle conflict files
        */
        if(diff3->Type() == ToolRunner::TKDIFF)
        {
            diff3->Merge(conflicting);
        }
        else
        {
            wxString target(conflicting);
            wxString mine(target + ".mine");
            wxString theirs;
            wxString f = wxFindFirstFile( target+ ".r*");
            while ( !f.IsEmpty() )
            {
                theirs = f;
                f = wxFindNextFile();
            }
            diff3->Merge(mine, theirs, target);
        }
    }
}

void SubversionPlugin::Resolved(wxCommandEvent& event)
{
    wxString target(GetSelection());
    
    if(never_ask || (show_resolved == false) || (wxMessageDialog(Manager::Get()->GetAppWindow(),
            "Do you want to issue a 'svn resolved' command?\n\n"
            "This command does NOT resolve any conflicts, instead it tells the revision control system that you have been "
            "manually editing the conflicting to resolve all conflicts.\n\n"
            "Undoing the effects of a false 'resolved' command is not impossible, but it is painful. Be sure about what you do.",
            "Resolved", wxYES_NO | wxICON_EXCLAMATION).ShowModal() == wxID_YES))
        svn->Resolved(target);
        
}





































/*-----------------------------------------------------------------------------------------------------------------
*
*  DO NOT LOOK ANY FURTHER. BEYOND THIS POINT, THINGS ARE REALLY EVIL. YOU HAVE BEEN WARNED.
* 
*/





#ifdef __WIN32__
 #include <shlobj.h>
 #include <wx/msw/registry.h>
 #ifndef CSIDL_PROGRAM_FILES
  #define CSIDL_PROGRAM_FILES  0x0026
 #endif
#endif


wxString  SubversionPlugin::GetCheckoutDir()
{
    wxFileName home;
    home.AssignHomeDir();
    return home.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "checkout";
}

void SubversionPlugin::SearchBinaries()
{
#ifdef __WIN32__

    if(svnbinary.IsEmpty())
        svnbinary  = NastyFind("svn.exe");
    if(svnbinary.IsEmpty())
        cvsbinary  = NastyFind("cvs.exe");
        
#else
        
    svnbinary  = NastyFind("svn");
    cvsbinary  = NastyFind("cvs");
    extdiff  = NastyFind("kdiff3");
    if(extdiff.IsEmpty())
        extdiff = NastyFind("tkdiff.tcl");
        
#endif
        
#ifdef SCO
        
    long* ptr = 0;
    *ptr = 1L;
#endif
    
    WriteConfig();
}

void SubversionPlugin::TamperWithWindowsRegistry()
{
#ifdef __WIN32__
    wxRegKey* rKey;
    wxString bin;
    
    rKey = new wxRegKey("HKEY_LOCAL_MACHINE\\SOFTWARE\\TortoiseSVN");
    if( rKey->Exists() )
    {
        rKey->QueryValue("ProcPath", bin);
        
        if(wxFile::Exists(bin))
            tortoiseproc = bin;
    }
    delete rKey;
    
    rKey = new wxRegKey("HKEY_CURRENT_USER\\Software\\TortoiseSVN\\History\\repoURLS");
    if( rKey->Exists() )
    {
        long index;
        wxString val;
        wxString result;
        
        if(rKey->GetFirstValue(val, index))
            do
            {
                if(rKey->QueryValue(val, result))
                    repoHistory.Add(result);
            }
            while ( rKey->GetNextValue(val, index) );
    }
    delete rKey;
    
    if(tortoiseproc.IsEmpty())
    {
        if(verbose)
            Log::Instance()->Add("No working installation of TortoiseSVN was found. Although TortoiseSVN is not strictly necessary to use this plugin,\n"
                                 "it offers valuable additional functionality and is highly recommended.\n"
                                 "TortoiseSVN is available under http://tortoisesvn.tigris.org\n");
                                 
        rKey = new wxRegKey("HKEY_CURRENT_USER\\Software\\KDiff3");
        if( rKey->Exists() )
        {
            rKey->QueryValue("", bin);
            bin << "\\kdiff3.exe";
            if(wxFile::Exists(bin))
            {
                extdiff = bin;
                if(verbose)
                    Log::Instance()->Add("KDiff3 detected.");
            }
        }
        delete rKey;
        if(extdiff.IsEmpty())
        {
            rKey = new wxRegKey("HKEY_CURRENT_USER\\Software\\Thingamahoochie\\WinMerge");
            if( rKey->Exists() )
            {
                rKey->QueryValue("Executable", bin);
                if(wxFile::Exists(bin))
                {
                    extdiff = bin;
                    if(verbose)
                        Log::Instance()->Add("WinMerge detected.");
                }
            }
            delete rKey;
        }
    }
    
    rKey = new wxRegKey("HKEY_LOCAL_MACHINE\\SOFTWARE\\TortoiseCVS");
    if( rKey->Exists() )
    {
        rKey->QueryValue("RootDir", bin);
        bin << "TortoiseAct.exe";
        
        if(wxFile::Exists(bin))
            tortoiseact = bin;
    }
    delete rKey;
    
    if(verbose)
    {
        if(!tortoiseproc.IsEmpty())
            Log::Instance()->Add("TortoiseSVN detected.");
        if(repoHistory.Count())
            Log::Instance()->Add("Successfully imported TortoiseSVN history from registry.");
        if(!tortoiseact.IsEmpty())
            Log::Instance()->Add("TortoiseCVS detected.");
            
        clearTimer.Start(12000);
        Log::lastLogTime = 0;
        Log::Instance()->fg();
    }
    
#endif
    
}


wxString SubversionPlugin::NastyFind(const wxString& name)
{
    // NOTE : This hideous beast could maybe be implemented more elegantly using wxPathList
#ifdef __WIN32__
    
    wxArrayString prefix;
    wxArrayString location;
    
    TCHAR szPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0, szPath);
    prefix.Add(szPath);
    prefix.Add("C:");
    prefix.Add("D:");
    prefix.Add("C:\\Program Files");
    prefix.Add("C:\\Programme");
    prefix.Add("C:\\Programa");
    prefix.Add("C:\\Programmes");
    prefix.Add("C:\\Apps");
    prefix.Add("C:\\Tools");
    prefix.Add("E:");
    
    location.Add("\\subversion\\bin");
    location.Add("\\TortoiseSVN\\bin");
    location.Add("\\TortoiseMerge\\bin");
    location.Add("\\subversion");
    location.Add("\\TortoiseSVN");
    location.Add("\\TortoiseMerge");
    location.Add("\\svn\\bin");
    location.Add("\\svn");
    location.Add("\\bin");
    location.Add("\\cvs");
    location.Add("\\TortoiseCVS");
    
    
    int lc = location.GetCount();  // I never trust these are really inline, are they...?
    int pc = prefix.GetCount();
    
    for(int i = 0; i < lc; i++)
    {
        for(int j = 0; j < pc; j++)
        {
            wxString loc = prefix[j] + location[i] + "\\" + name;
            if(wxFile::Exists(loc))
                return loc;
        }
    }
    // In the case of svn, we assume that if we have not found it, then it is (hopefully) in PATH.
    // Tortoise, if not found, is assumed "missing".
    // Hopefully anyone installing a plugin to run svn has enough sense to install svn as well.
    if(name.CompareTo("svn.exe"))
    {
        Log::Instance()->Add("The svn executable could not be found in any of the 'usual' locations.\n"
                             "Please do note that svn is essential for the operation of this plugin.\n"
                             "As this is the only thing that makes sense, I will assume that svn is accessible via the %PATH% environment variable.\n"
                             "You can set the path to the svn executable in the preferences dialog.");
        return(name);
    }
#endif
    
#ifdef __linux__
    
    wxArrayString location;
    location.Add("/usr/bin");   // this is probably it, anyway
    location.Add("/usr/local/bin");
    location.Add("/usr/share/bin");
    location.Add("/bin");
    location.Add("/opt/bin");
    location.Add("/usr/sbin");  // uhh... hopefully not, but who knows...
    location.Add("/sbin");   // everything is possible
    for(int i = 0; i < location.GetCount(); i++)
    {
        wxString loc = location[i] + "/" + name;
        if(wxFile::Exists(loc))
            return loc;
    }
    
    // similar to above - if we can't find svn, we will assume (hope) it is in $PATH and just call "svn"
    // - if the user actually works with svn at all, this should be the case
    if(name.CompareTo("svn"))
    {
        Log::Instance()->Add("The svn executable could not be found in any of the 'usual' locations.\n"
                             "Please do note that svn is essential for the operation of this plugin.\n"
                             "As this is the only thing that makes sense, I will assume that svn is accessible via the $PATH environment variable.\n"
                             "You can set the path to the svn executable in the preferences dialog.");
        return(name);
    }
#endif
    
    return wxEmptyString;
}

