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

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif
#include <cbplugin.h>
#include <settings.h>
#include <messagemanager.h>
#include <wx/dir.h>

#include <manager.h>
#include <sdk_events.h>
#include <projectmanager.h>
#include <projectbuildtarget.h>
#include <simpletextlog.h>

#include "toolrunner.h"


WX_DECLARE_HASH_MAP( int, wxString, wxIntegerHash, wxIntegerEqual, IdToStringHash );


class SubversionPlugin : public cbPlugin
  {
    int tabIndex;

    SimpleTextLog *outputLog;

    IdToStringHash	fileProperties;

    SVNRunner		* svn;
    TortoiseRunner	* tortoise;

  public:
    SubversionPlugin();
    ~SubversionPlugin()
    {}
    ;
    int SubversionPlugin::Configure()
    {
      CodeBlocksEvent e;
      Preferences(e);
      return 0;
    };

    void BuildMenu(wxMenuBar* menuBar)
    {}
    ;
    void BuildModuleMenu(const ModuleType type, wxMenu* menu, const wxString& arg);

    void BuildProjectMenu(wxMenu* menu, wxString name, wxString target);
    void BuildFileMenu(wxMenu* menu, wxString name, wxString target);
    void BuildMgrMenu(wxMenu* menu);
    void AppendCommonMenus(wxMenu *menu, wxString target, bool isProject);

    void SubversionPlugin::BuildToolBar(wxToolBar* toolBar)
    {
      NotImplemented("SubversionPlugin::BuildToolBar()");
    };

    void OnAttach();
    void OnRelease(bool appShutDown);

    void SubversionPlugin::OnFirstRun();
    void SubversionPlugin::Preferences(CodeBlocksEvent& event);
    void SubversionPlugin::SetUser(CodeBlocksEvent& event);

    void SubversionPlugin::Add(CodeBlocksEvent& event);
    void SubversionPlugin::Delete(CodeBlocksEvent& event);

    void SubversionPlugin::PropIgnore(CodeBlocksEvent& event);
    void SubversionPlugin::PropMime(CodeBlocksEvent& event);
    void SubversionPlugin::PropExec(CodeBlocksEvent& event);
    void SubversionPlugin::PropExt(CodeBlocksEvent& event);
    void SubversionPlugin::PropKeywords(CodeBlocksEvent& event);


    void SubversionPlugin::Checkout(CodeBlocksEvent& event);
    void SubversionPlugin::Import(CodeBlocksEvent& event);
    void SubversionPlugin::Commit(CodeBlocksEvent& event);
    void SubversionPlugin::Update(CodeBlocksEvent& event);
    void SubversionPlugin::Revert(CodeBlocksEvent& event);
    void SubversionPlugin::Diff(CodeBlocksEvent& event);
    void SubversionPlugin::OnFatTortoiseFunctionality(CodeBlocksEvent& event);

    void SubversionPlugin::EditProperty(wxEvent& event);

    wxArrayString	SubversionPlugin::ParseStatusOutputForChar(const char what);
    char 			SubversionPlugin::ParseStatusOutputForFile(const wxString& what);

    void SubversionPlugin::TamperWithWindowsRegistry();
    wxString SubversionPlugin::NastyFind(const wxString& name);

    void SubversionPlugin::ReadConfig();
    void SubversionPlugin::WriteConfig();

    wxString SubversionPlugin::Escape(const wxString& s)  // we might use a better func here
    {
      wxString t("\"" + s + "\"");
      t.Replace("\\", "/");
      return t;
    }

    wxString SubversionPlugin::LocalPath(const wxString& target)
    {
      return LocalPath(GetSelectionsProject(), target);
    };

    wxString SubversionPlugin::LocalPath(const wxString& base, const wxString& target)
    {
      wxFileName fn(target);
      fn.MakeRelativeTo(base);
      return fn.GetFullPath();
    };

    wxString SubversionPlugin::GetActiveProject()
    {
      cbProject* currentProject = Manager::Get()->GetProjectManager()->GetActiveProject();
      if (!currentProject)
        return wxEmptyString;
      return currentProject->GetCommonTopLevelPath();
    };

    wxString SubversionPlugin::GetSelection()
    {
      wxTreeCtrl* tree = Manager::Get()->GetProjectManager()->GetTree();

      FileTreeData* ftd	= (FileTreeData*) tree->GetItemData(tree->GetSelection());

      if(!ftd) // please don't crash us if nothing is selected
        return wxEmptyString;

      if(ProjectFile *f = ftd->GetProject()->GetFile(ftd->GetFileIndex()))
        return f->file.GetFullPath();
      else
        return ftd->GetProject()->GetCommonTopLevelPath();
    };

    wxString SubversionPlugin::GetSelectionsProject()
    {
      wxTreeCtrl* tree = Manager::Get()->GetProjectManager()->GetTree();
      FileTreeData* ftd	= (FileTreeData*) tree->GetItemData(tree->GetSelection());

      if(!ftd) // please don't crash us if nothing is selected
        return wxEmptyString;

      return ftd->GetProject()->GetCommonTopLevelPath();
    };


    bool SubversionPlugin::IsProject(const wxString& arg)
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


    bool SubversionPlugin::DirUnderVersionControl(const wxString& arg)
    {
      wxString s =  wxFileName(arg).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + ".svn";
      return wxFileName::DirExists(s);
    }

    void SubversionPlugin::fg()
    {
      Manager::Get()->GetMessageManager()->SwitchTo(tabIndex);
    };


    wxString defaultCheckoutDir;
    wxString svnbinary;
    wxString tortoiseproc;

    bool cascade_menu;
    bool auto_add;
    bool auto_add_only_project;
    bool auto_delete;
    bool force_clean;
    bool require_comments;
    bool prefill_comments;
    bool update_on_conflict;

    bool no_ask_revertable;
    bool never_ask;
    bool warn_revert;
    bool full_status_on_startup;
    bool no_props;
    bool show_resolved;
    wxArrayString repoHistory;

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
#endif // SVN_H
