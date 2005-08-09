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


#ifndef SVN_H
#define SVN_H


#include <wx/wx.h>

#include <manager.h>
#include <configmanager.h>
#include <sdk_events.h>
#include <projectmanager.h>
#include <projectbuildtarget.h>

#include "toolrunner.h"
#include "svncvsrunner.h"
#include "tortoiserunner.h"


WX_DECLARE_HASH_MAP( int, wxString, wxIntegerHash, wxIntegerEqual, IdToStringHash );


class MRUList : public wxArrayString
{
public:
    void Add(wxString s)
    {
        int idx;
        while((idx = Index(s, false)) != wxNOT_FOUND)
            Remove(idx);
        Insert(s, 0);
    };
};



class SubversionPlugin : public cbPlugin
{
    IdToStringHash fileProperties;
    
    SVNRunner  *svn;
    TortoiseRunner *tortoise;
    TortoiseCVSRunner *tortoisecvs;
    CVSRunner   *cvs;
    DiffRunner *diff3;
    ToolRunner *binutils;
    
public:
    SubversionPlugin();
    ~SubversionPlugin()
    {}
    ;
    int Configure()
    {
        CodeBlocksEvent e;
        Preferences(e);
        return 0;
    };
    void BuildMenu(wxMenuBar* menuBar)
    {}
    ;
    bool SubversionPlugin::BuildToolBar(wxToolBar* toolBar)
    {
        return false;
    };
    
    void BuildModuleMenu(const ModuleType type, wxMenu* menu, const wxString& arg);
    void Build_CVS_ModuleMenu(wxMenu* menu, const wxString& arg);
    
    
    void   BuildProjectMenu(wxMenu* menu, wxString name, wxString target);
    void   BuildFileMenu(wxMenu* menu, wxString name, wxString target);
    void   BuildMgrMenu(wxMenu* menu);
    void   AppendCommonMenus(wxMenu *menu, wxString target, bool isProject, bool isLocked);
    
    void   OnAttach();
    void   OnRelease(bool appShutDown);
    void   OnTimer(wxTimerEvent& event);
    
    void   SearchBinaries();
    void   Preferences(wxCommandEvent& event);
    void   SetUser(wxCommandEvent& event);
    
    void   Add(wxCommandEvent& event);
    void   Delete(wxCommandEvent& event);
    void   Lock(wxCommandEvent& event);
    
    void   PropIgnore(wxCommandEvent& event);
    void   PropMime(wxCommandEvent& event);
    void   PropExec(wxCommandEvent& event);
    void   PropNeedsLock(wxCommandEvent& event);
    void   PropExt(wxCommandEvent& event);
    void   PropKeywords(wxCommandEvent& event);
    
    void   TransactionSuccess(wxCommandEvent& event);
    void   TransactionFailure(wxCommandEvent& event);
    void   ReRun(wxCommandEvent& event);
    
    void   Checkout(wxCommandEvent& event);
    void   Import(wxCommandEvent& event);
    void   Commit(wxCommandEvent& event);
    void   Update(wxCommandEvent& event);
    void   Revert(wxCommandEvent& event);
    void   Diff(wxCommandEvent& event);
    void   Patch(wxCommandEvent& event);
    void   EditConflicts(wxCommandEvent& event);
    
    void   OnFatTortoiseFunctionality(wxCommandEvent& event);
    void   OnFatTortoiseCVSFunctionality(wxCommandEvent& event);
    void   CVSUpdate(wxCommandEvent& event);
    void   CVSLogin(wxCommandEvent& event);
    
    void   EditProperty(wxCommandEvent& event);
    void   DoResolve(const wxString& conflicting);
    void   Resolved(wxCommandEvent& event);
    void   Release(wxCommandEvent& event);
    
    wxArrayString ExtractFilesWithStatus(const char what, unsigned int pos = 0);
    void      ExtractFilesWithStatus(const char what, wxArrayString& ret, unsigned int pos = 0);
    char   ParseStatusOutputForFile(const wxString& what);
    void      AutoOpenProjects(const wxString& rootdir, bool recursive, bool others);
    
    void  ReloadEditors(const wxArrayString& filenames);
    void  ReOpenEditor(const wxString& filenames);
    void  TamperWithWindowsRegistry();
    wxString NastyFind(const wxString& name);
    
    void   ReadConfig();
    void   WriteConfig();
    
    wxString  GetCheckoutDir();
    
    wxString LocalPath(const wxString& target)
    {
        return LocalPath(GetSelectionsProject(), target);
    };
    
    wxString LocalPath(const wxString& base, const wxString& target)
    {
        wxFileName fn(target);
        fn.MakeRelativeTo(base);
        return fn.GetFullPath();
    };
    
    wxString GetActiveProject()
    {
        cbProject* currentProject = Manager::Get()->GetProjectManager()->GetActiveProject();
        if (!currentProject)
            return wxEmptyString;
        return currentProject->GetCommonTopLevelPath();
    };
    
    wxString GetSelection()
    {
        wxTreeCtrl* tree = Manager::Get()->GetProjectManager()->GetTree();
        
        FileTreeData* ftd = (FileTreeData*) tree->GetItemData(tree->GetSelection());
        
        if(!ftd) // please don't crash us if nothing is selected
            return wxEmptyString;
            
        if(ProjectFile *f = ftd->GetProject()->GetFile(ftd->GetFileIndex()))
            return f->file.GetFullPath();
        else
            return ftd->GetProject()->GetCommonTopLevelPath();
    };
    
    wxString GetSelectionsProject()
    {
        wxTreeCtrl* tree = Manager::Get()->GetProjectManager()->GetTree();
        FileTreeData* ftd = (FileTreeData*) tree->GetItemData(tree->GetSelection());
        
        if(!ftd) // please don't crash us if nothing is selected
            return wxEmptyString;
            
        return ftd->GetProject()->GetCommonTopLevelPath();
    };
    
    
    bool IsProject(const wxString& arg)
    {
        ProjectsArray* array = Manager::Get()->GetProjectManager()->GetProjects();
        if (array)
        {
            int n = array->GetCount();
            for (size_t i = 0; i < n; ++i)
            {
                cbProject* cur = array->Item(i);
                if (cur && cur->GetTitle() == arg)
                    return true;
            }
        }
        return false;
    }
    
    void DisableCheckExternals()
    {
        chkmod_status = ConfigManager::Get()->Read("/environment/check_modified_files",1); // evil stuff: tamper with c::b settings
        ConfigManager::Get()->Write("/environment/check_modified_files", false);     // to prevent a race that occurs on lengthy operations
    }
    
    void ResetCheckExternals()
    {
        ConfigManager::Get()->Write("/environment/check_modified_files", chkmod_status); // restore original state
    }
    
    cbProject* GetCBProject()
    {
        wxTreeCtrl* tree = Manager::Get()->GetProjectManager()->GetTree();
        FileTreeData* ftd = (FileTreeData*) tree->GetItemData(tree->GetSelection());
        
        if(!ftd) // please don't crash us if nothing is selected
            return 0;
            
        return ftd->GetProject();
    }
    
    wxString DirName(wxString& d)
    {
        d.Replace("\\", "/");
        if(d[d.Length()-1] == '/')
            d = d.Mid(0, d.Length()-1);
        return d.AfterLast('/') ;
    }
    
    bool DirUnderVersionControl(const wxString& arg)
    {
        wxString s =  wxFileName(arg).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + ".svn";
        return wxFileName::DirExists(s);
    }
    
    /*
    * As it is possible that both .svn and CVS exist (migrated project with leftover folders), DirUnderCVS first checks for subversion's 
    * presence, and returns false if subversion is found. Nobody should migrate backwards from subversion to CVS (at least I think so),
    * thus it is assumed that if both revision control systems are present, the better one should be used.
    * Also, of course, this is a subversion plugin. CVS is only supported as a fallback for the quaint 
    */
    bool DirUnderCVS(const wxString& arg)
    {
        wxString s =  wxFileName(arg).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + ".svn";
        if(wxFileName::DirExists(s))
            return false;
            
        s =  wxFileName(arg).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "CVS";
        return wxFileName::DirExists(s);
    }
    
    
    wxString defaultCheckoutDir;
    wxString svnbinary;
    wxString cvsbinary;
    wxString tortoiseproc;
    wxString tortoiseact;
    wxString extdiff;
    wxString tarbin;
    wxString bzip2bin;
    wxString zipbin;
    
    bool cascade_menu;
    bool auto_add;
    bool auto_add_only_project;
    bool auto_delete;
    bool force_clean;
    bool require_comments;
    bool prefill_comments;
    bool avoid_out_of_date;
    
    bool no_ask_revertable;
    bool never_ask;
    bool warn_revert;
    bool full_status_on_startup;
    bool no_props;
    bool show_resolved;
    bool prompt_reload;
    bool up_after_co;
    bool verbose;
    bool chkmod_status;
    MRUList repoHistory;
    MRUList repoHistoryCVS;
    bool request_autoopen;
    wxTimer clearTimer;
    wxString patchFileName;
    wxString meow;
    bool has_tar_or_zip;
protected:
private:
    DECLARE_EVENT_TABLE()
};
#ifdef __cplusplus
extern "C"
{
#endif
    PLUGIN_EXPORT cbPlugin* GetPlugin();
#ifdef __cplusplus
    
};
#endif
#endif
