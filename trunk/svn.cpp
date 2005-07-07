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


#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif
#include <wx/regex.h>

#include <wx/xrc/xmlres.h>
#include <wx/fs_zip.h>
#include <wx/image.h>

#include "svn.h"
#include <licenses.h>
#include "codeblocks/sdk_events.h"

#include <manager.h>
#include <sdk_events.h>
#include <configmanager.h>
#include <messagemanager.h>
#include <projectmanager.h>
#include <editormanager.h>

#include <cbproject.h>
#include <pipedprocess.h>
#include <wx/fileconf.h>

#include "dialogs.h"
#include "toolrunner.h"

#ifdef __WIN32__
	#include <shlobj.h>
	#include <registry.h>
	#ifndef CSIDL_PROGRAM_FILES
		#define CSIDL_PROGRAM_FILES  0x0026
	#endif
const bool filenames_are_unix = false;
#else
const bool filenames_are_unix = true;
#endif



BEGIN_EVENT_TABLE(SubversionPlugin, cbPlugin)

EVT_MENU(ID_MENU_PREFS, SubversionPlugin::Preferences)

EVT_MENU(ID_MENU_IMPORT,			SubversionPlugin::Import)
EVT_MENU(ID_MENU_CHECKOUT,			SubversionPlugin::Checkout)

EVT_MENU(ID_MENU_COMMIT, 			SubversionPlugin::Commit)
EVT_MENU(ID_MENU_UPDATE,			SubversionPlugin::Update)
EVT_MENU(ID_MENU_UP_P,				SubversionPlugin::Update)
EVT_MENU(ID_MENU_UP_C,				SubversionPlugin::Update)
EVT_MENU(ID_MENU_UP_B,				SubversionPlugin::Update)
EVT_MENU(ID_MENU_UP_REV,			SubversionPlugin::Update)

EVT_MENU(ID_MENU_RESTORE,			SubversionPlugin::Update)

EVT_MENU(ID_MENU_REVERT,			SubversionPlugin::Revert)

EVT_MENU(ID_MENU_BRANCH,			SubversionPlugin::OnFatTortoiseFunctionality)
EVT_MENU(ID_MENU_SWITCH,			SubversionPlugin::OnFatTortoiseFunctionality)
EVT_MENU(ID_MENU_MERGE,				SubversionPlugin::OnFatTortoiseFunctionality)
EVT_MENU(ID_MENU_CREATE,			SubversionPlugin::OnFatTortoiseFunctionality)
EVT_MENU(ID_MENU_RELOCATE,			SubversionPlugin::OnFatTortoiseFunctionality)


EVT_MENU(ID_MENU_ADD,				SubversionPlugin::Add)
EVT_MENU(ID_MENU_DELETE,			SubversionPlugin::Delete)

EVT_MENU(ID_MENU_PROP_IGNORE,		SubversionPlugin::PropIgnore)
EVT_MENU(ID_MENU_PROP_MIME,			SubversionPlugin::PropMime)

EVT_MENU(ID_MENU_PROP_EXECUTABLE,	SubversionPlugin::PropExec)
EVT_MENU(ID_MENU_PROP_EXTERNALS,	SubversionPlugin::PropExt)

EVT_MENU(ID_MENU_KW_DATE,			SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_REVISION,		SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_AUTHOR,			SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_HEAD,			SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_ID,				SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_SETALL,			SubversionPlugin::PropKeywords)
EVT_MENU(ID_MENU_KW_CLEARALL,		SubversionPlugin::PropKeywords)

EVT_MENU(ID_MENU_PROP_NEW,			SubversionPlugin::EditProperty)


END_EVENT_TABLE()


cbPlugin* GetPlugin()
{
  return new SubversionPlugin;
}

SubversionPlugin::SubversionPlugin()
{
  cascade_menu				= true;
  auto_add					= true;
  auto_add_only_project		= true;
  auto_delete				= false;
  force_clean				= false;
  require_comments			= true;
  prefill_comments			= true;
  update_on_conflict		= true;
  no_ask_revertable			= true;
  never_ask					= false;
  warn_revert				= true;
  full_status_on_startup 	= false;
  no_props					= false;
  show_resolved				= false;

  wxFileSystem::AddHandler(new wxZipFSHandler);
  wxXmlResource::Get()->InitAllHandlers();
  wxXmlResource::Get()->Load(ConfigManager::Get()->Read("data_path", wxEmptyString) + "/svn.zip#zip:*.xrc");

  m_PluginInfo.name = "svn";
  m_PluginInfo.title = "Subversion";

  {
    wxString rev("$Revision$");	// let svn:keywords fill in the revision number
    wxString date("$Date$");
    rev.Replace("$", "");					// but make it a bit prettier
    wxRegEx reg("Date: ([0-9]{4}-[0-9]{2}-[0-9]{2})", wxRE_ICASE);
    if(reg.Matches(date))
      date = reg.GetMatch(date, 1);

    m_PluginInfo.version = "0.2   " + rev + " / " + date;
  }
  m_PluginInfo.description = "code::blocks revision control using subversion\n\n"
                             "Subversion is an advanced revision control system intended to replace CVS.\n\n"
                             "If you develop under Windows, do not forget to get TortoiseSVN as well.\n\n"
                             "References:\nhttp://subversion.tigris.org\nhttp://tortoisesvn.tigris.org\n\n"
                             "Subversion access to the cb-svn project is available at:\n"
                             "svn://svn.berlios.de/svnroot/repos/cb-svn/";

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
  MessageManager* msgMan = Manager::Get()->GetMessageManager();

  outputLog = new SimpleTextLog(msgMan, m_PluginInfo.title);

  outputLog->GetTextControl()->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxWHITE, wxFont(8, wxMODERN, wxNORMAL, wxNORMAL)));

  tabIndex = msgMan->AddLog(outputLog);

  wxBitmap bmp;
  wxFileSystem fs;
  wxString zip(ConfigManager::Get()->Read("data_path") + "/svn.zip#zip:\\");

  if(wxFSFile* file = fs.OpenFile(zip + "log.png"))
    {
      bmp = wxBitmap(wxImage(*(file->GetStream()), wxBITMAP_TYPE_PNG));
      delete file;
    }

  Manager::Get()->GetMessageManager()->SetLogImage(outputLog, bmp);

  ReadConfig();


  svn = new SVNRunner(svnbinary, outputLog);
  svn->StevieWonder("help"); // get svn into the file cache asynchronously, as we'll need it soon


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
  tortoise = tortoiseproc.IsEmpty() ? 0 : new TortoiseRunner("cmd /C " + tortoiseproc, outputLog);

}

void SubversionPlugin::OnRelease(bool appShutDown)
{
  WriteConfig();
}


void SubversionPlugin::BuildModuleMenu(const ModuleType type, wxMenu* menu, const wxString& arg)
{
  wxMenu* cmenu;

  if (!menu || !m_IsAttached || type != mtProjectManager)
    return;

  menu->AppendSeparator();

  if(cascade_menu)
    cmenu = new wxMenu;
  else
    cmenu = menu;


  if (arg.IsEmpty())
    {
      BuildMgrMenu(cmenu);
    }
  else
    {
      wxString selected(GetSelection());

      if(!DirUnderVersionControl(selected))
        {
          cmenu->Append( ID_MENU_IMPORT, "Import Project (currently unversioned)" );
          if(menu != cmenu)
            menu->Append( ID_MENU, "Subversion", cmenu );
          return;
        }

      svn->Status(selected);

      if (IsProject(arg))
        BuildProjectMenu(cmenu, arg, selected);
      else
        BuildFileMenu(cmenu, arg, selected);
    }

  if(menu != cmenu)
    menu->Append( ID_MENU, "Subversion", cmenu );
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
  int n = svn->std_out.GetCount();
  for(int i = 0; i < n; ++i)
    {
      if(svn->std_out[i].Mid(7) == target )
        {
          status = (svn->std_out[i])[(size_t)0];
          pstatus = (svn->std_out[i])[(size_t)1];
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

  if(status == 'C' && !tortoiseproc.IsEmpty())
    {
      menu->Append( ID_MENU_RESOLVETOOL, "Resolve file conflicts");
      menu->AppendSeparator();
    }
  if(pstatus == 'C' && status != 'C') // do not allow to resolve properties if files are in conflict!
    {
      menu->Append( ID_MENU_RESOLVEPROP, "Resolve property conflicts");
      menu->AppendSeparator();
    }

  AppendCommonMenus(menu, target, false);

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
    }

  if(fm || pm || fa)
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
      if(cf)
        comstr << ((fm || pm || fa || fd) ? ", " : "") << cf << " file" << (cf == 1 ? "" : "s") << " in conflict";
      if(ms)
        comstr << ((fm || pm || fa || fd || cf) ? ", " : "") << ms << " file" << (ms == 1 ? "" : "s") << " missing";

      comstr << ")";


      menu->Append( ID_MENU_COMMIT, comstr );
      menu->AppendSeparator();
      menu->Append( ID_MENU_REVERT, "Revert..." );
      menu->AppendSeparator();
    }

  AppendCommonMenus(menu, target, true);

  if(! tortoiseproc.IsEmpty())
    {
      menu->AppendSeparator();
      menu->Append( ID_MENU_BRANCH,		"Branch..." );
      menu->Append( ID_MENU_SWITCH,		"Switch..." );
      menu->Append( ID_MENU_MERGE,		"Merge..." );
      menu->Append( ID_MENU_RELOCATE,	"Relocate..." );
    }


}


void SubversionPlugin::AppendCommonMenus(wxMenu *menu, wxString target, bool isProject)
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
      for( it = fileProperties.begin(); it != fileProperties.end(); ++it )	// remove all the old rubbish
        Disconnect(it->first);												// from the event tables
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

  int n = props.GetCount();
  for(int i = 0; i < n; ++i)
    {
      has_ignore	|= (props[i] == "svn:ignore");
      has_keywords	|= (props[i] == "svn:keywords");
      has_exec		|= (props[i] == "svn:executable");
      has_mime		|= (props[i] == "svn:mime-type");
      has_externals	|= (props[i] == "svn:externals");
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
      svnprops->Append( ID_MENU, "svn:keywords", keywords );
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

      svnprops->Append( ID_MENU_PROP_IGNORE, has_ignore ? f+"svn:ignore" : f+"svn:ignore  [none]" );
      svnprops->Append( ID_MENU_PROP_EXTERNALS, has_externals ? f+"svn:externals" : f+"svn:externals  [none]" );


      svnprops->Append( ID_MENU_PROP_EXECUTABLE, "svn:executable", "", wxITEM_CHECK );
      svnprops->Append( ID_MENU_PROP_MIME, has_mime ? "svn:mime-type" : "svn:mime-type [default]" );
      svnprops->Check(ID_MENU_PROP_EXECUTABLE, has_exec);
    }

  if(has_keywords)
    {
      wxString kw = svn->PropGet(target, "svn:keywords");
      keywords->Check(ID_MENU_KW_DATE,		kw.Contains("Date"));
      keywords->Check(ID_MENU_KW_REVISION,	kw.Contains("Revision"));
      keywords->Check(ID_MENU_KW_AUTHOR,	kw.Contains("Author"));
      keywords->Check(ID_MENU_KW_HEAD,		kw.Contains("HeadURL"));
      keywords->Check(ID_MENU_KW_ID,		kw.Contains("Id"));
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
  sub = new wxMenu;
  sub->Append( ID_MENU_D_H, "HEAD" );
  sub->Append( ID_MENU_D_P, "PREV" );
  sub->Append( ID_MENU_D_C, "COMMITTED" );
  sub->Append( ID_MENU_D_B, "BASE" );
  sub->Append( ID_MENU_D_REV, "Revision..." );
  menu->Append( ID_MENU, "DIFF against...", sub );
}





void SubversionPlugin::Update(CodeBlocksEvent& event)
{
  wxString	revision("HEAD");
  wxString	selected(GetSelection());

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


  if(svn->Update(selected, revision) == 0)
    {
      wxRegEx r("revision\\ ([0-9]+)\\.$");
      wxString rev;

      if(r.IsValid())
        if(r.Matches(svn->blob))
          {
            rev = r.GetMatch(svn->blob, 1);
            wxFileName fn(selected);
            if(event.GetId() == ID_MENU_RESTORE)
              outputLog->AddLog( "Restored missing file " + fn.GetFullName() + " to revision " + rev);
            else
              outputLog->AddLog( ( fn.IsDir() ? "Project" : fn.GetFullName() )+ " is at revision " + rev);
          }
    }

  Manager::Get()->GetEditorManager()->CheckForExternallyModifiedFiles();
}

void SubversionPlugin::Commit(CodeBlocksEvent& event)
{
  wxString	selected(GetSelection());
  svn->Status(selected);

  wxArrayString files;
  wxArrayString toAdd;

  wxArrayString missing = ParseStatusOutputForChar('?');

  if(auto_add)
    files = wxArrayString();
  else
    files = missing;

  CommitDialog d(Manager::Get()->GetAppWindow(), files);
  d.Centre();

  if(d.ShowModal() == wxID_OK)
    {
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
      for(unsigned int i = 0; i < toAdd.Count(); ++i)	// better call svn with 637 paramters than run svn 637 times...
        concat << " \"" << toAdd[i] << "\" ";
      concat = concat.Mid(2, concat.Length()-4);		// svn->Add() wraps in double quotes already

      svn->Add(concat);
      svn->Commit(selected, d.comment);
    }
}

void SubversionPlugin::Checkout(CodeBlocksEvent& event)
{
  ProjectManager *pmgr = Manager::Get()->GetProjectManager();
  CheckoutDialog d(Manager::Get()->GetAppWindow(), repoHistory, defaultCheckoutDir);
  d.Centre();
  if(d.ShowModal() == wxID_OK)
    {
      if(!d.username.IsEmpty())
        svn->SetPassword(d.username, d.password);

      if(!d.checkoutDir.IsEmpty())
        {
          if(::wxDirExists(d.checkoutDir) || ::wxMkdir(d.checkoutDir))
            if(!svn->Checkout(d.repoURL, d.checkoutDir, (d.revision.IsEmpty() ? wxString("HEAD") : d.revision) ))
              {
                if(d.autoOpen)
                  {
                    outputLog->AddLog("Looking for projects...");

                    wxArrayString dirs;
                    wxString f;

                    f = wxFindFirstFile(d.checkoutDir + "/", wxDIR);
                    dirs.Add(d.checkoutDir);
                    while ( !f.IsEmpty() )
                      {
                        dirs.Add(f);
                        f = wxFindNextFile();
                      }
                    for(int i = 0; i < dirs.Count(); ++i)
                      {
                        f = wxFindFirstFile(dirs[i] + "/*.cbp");
                        while ( !f.IsEmpty() )
                          {
                            outputLog->AddLog("opening " + f);
                            pmgr->LoadProject(f);
                            f = wxFindNextFile();
                          }
                        f = wxFindFirstFile(dirs[i] + "/*.dev");
                        while ( !f.IsEmpty() )
                          {
                            outputLog->AddLog("importing " + f);
                            pmgr->LoadProject(f);
                            f = wxFindNextFile();
                          }
                        f = wxFindFirstFile(dirs[i] + "/*.dsp");
                        while ( !f.IsEmpty() )
                          {
                            outputLog->AddLog("importing " + f);
                            pmgr->LoadProject(f);
                            f = wxFindNextFile();
                          }
                      }

                  }
                repoHistory.Insert(d.repoURL, 0);
              }
        }
    }
}


void SubversionPlugin::Import(CodeBlocksEvent& event)
{
  ImportDialog d(Manager::Get()->GetAppWindow(), repoHistory, GetSelectionsProject());
  d.Centre();
  if(d.ShowModal() == wxID_OK)
    {
      if(!d.username.IsEmpty())
        svn->SetPassword(d.username, d.password);

      svn->Import(d.repoURL, d.importDir, d.comment);
      repoHistory.Insert(d.repoURL, 0);
    }
}



void SubversionPlugin::Revert(CodeBlocksEvent& event)
{
  wxString target(GetSelection());

  svn->Status(target);

  wxArrayString mods;

  int n = svn->std_out.Count();
  for(int i = 0; i < n; ++i)
    {
      wxString s;
      if(svn->std_out[i][(size_t)0] == 'D')
        s << "deletion";
      if(svn->std_out[i][(size_t)0] == 'A')
        s << (s.Length()? ", " : "") <<  "addition";
      if(svn->std_out[i][(size_t)0] == 'M' || svn->std_out[i][(size_t)0] == 'C')
        s << (s.Length()? ", " : "") <<  "local modification";
      if(svn->std_out[i][(size_t)1] == 'M')
        s << (s.Length()? ", " : "") <<  "property change";

      s << " of " << svn->std_out[i].Mid(7);

      if(svn->std_out[i].Length() > 8)
        mods.Add(s);
    }

  if(::wxDirExists(target)) // seems like we have a project folder here
    {
      RevertDialog d(Manager::Get()->GetAppWindow(), mods);
      d.Centre();
      if(d.ShowModal() == wxID_OK)
      {}
    }
  else
    {
      wxString localMods;
      if(warn_revert && ParseStatusOutputForFile(target) == 'M')
        localMods = "\nWARNING:\n\nThis file has local modifications which you will lose if you revert it.\n\n\n";

      if(never_ask || wxMessageDialog(Manager::Get()->GetAppWindow(), localMods + "Do you want to revert the file " + target + "?", (localMods.IsEmpty() ? "Revert" : "Revert over Modifications"), wxYES_NO).ShowModal() == wxID_YES)
        svn->Revert(target);
    }

}


char SubversionPlugin::ParseStatusOutputForFile(const wxString& what)
{
  int n = svn->std_out.Count();
  for(int i = 0; i < n; ++i)
    {
      if(svn->std_out[i].Mid(7) == what )
        {
          return (svn->std_out[i])[(size_t)0];
        }
    }
  return 0;
}

wxArrayString SubversionPlugin::ParseStatusOutputForChar(const char what)
{
  wxArrayString ret;
  for(int i = 0; i < svn->std_out.GetCount(); ++i)
    if((svn->std_out[i])[(size_t)0] == what )  // operator overloading abuse of the year...
      ret.Add(svn->std_out[i].Mid(7));
  return ret;
}


void SubversionPlugin::Preferences(CodeBlocksEvent& event)
{
  PreferencesDialog d(Manager::Get()->GetAppWindow(), this);
  d.Centre();

  XRCCTRL(d, "svn binary path", wxTextCtrl)->SetValue(wxFileName(svnbinary).GetFullPath());
  XRCCTRL(d, "tortoise binary path", wxTextCtrl)->SetValue(wxFileName(tortoiseproc).GetFullPath());
  XRCCTRL(d, "std checkout", wxTextCtrl)->SetValue(defaultCheckoutDir);

  XRCCTRL(d, "auto add", wxCheckBox)->SetValue(auto_add);
  XRCCTRL(d, "autoadd only project", wxCheckBox)->SetValue(auto_add_only_project);
  XRCCTRL(d, "auto remove missing", wxCheckBox)->SetValue(auto_delete);
  XRCCTRL(d, "auto merge", wxCheckBox)->SetValue(update_on_conflict);
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
  d.RadioToggle(event);

  if(d.ShowModal() == wxID_OK)	// Great job wxOK and wxID_OK have similar names and different values. Since nothing in wxWindows is typedef'd
    {							// you can spend hours wondering what goes wrong and never get a compiler error.
      svnbinary					= XRCCTRL(d, "svn binary path", wxTextCtrl)->GetValue();
      tortoiseproc				= XRCCTRL(d, "tortoise binary path", wxTextCtrl)->GetValue();
      defaultCheckoutDir		= XRCCTRL(d, "std checkout", wxTextCtrl)->GetValue();

      svn->SetExecutable(svnbinary);
      tortoise->SetExecutable(tortoiseproc);

      auto_add					= XRCCTRL(d, "auto add", wxCheckBox)->GetValue();
      auto_add_only_project		= XRCCTRL(d, "autoadd only project", wxCheckBox)->GetValue();
      auto_delete				= XRCCTRL(d, "auto remove missing", wxCheckBox)->GetValue();
      update_on_conflict		= XRCCTRL(d, "auto merge", wxCheckBox)->GetValue();
      require_comments			= XRCCTRL(d, "no emtpy comments", wxCheckBox)->GetValue();
      prefill_comments			= XRCCTRL(d, "prefill comments", wxCheckBox)->GetValue();
      force_clean				= XRCCTRL(d, "forceclean", wxCheckBox)->GetValue();
      no_ask_revertable			= XRCCTRL(d, "no confirm revertable", wxCheckBox)->GetValue();
      warn_revert				= XRCCTRL(d, "warn revert", wxCheckBox)->GetValue();
      never_ask  				= XRCCTRL(d, "never ask", wxCheckBox)->GetValue();
      full_status_on_startup	= XRCCTRL(d, "full status", wxCheckBox)->GetValue();
      no_props					= XRCCTRL(d, "no props", wxCheckBox)->GetValue();
      show_resolved				= XRCCTRL(d, "show resolved", wxCheckBox)->GetValue();
      cascade_menu				= XRCCTRL(d, "cascade", wxCheckBox)->GetValue();

      WriteConfig();
    }
}



void SubversionPlugin::ReadConfig()
{
  wxConfigBase* c = ConfigManager::Get();

  svnbinary 			= c->Read("/svn/svnbinary", "unset");
  tortoiseproc			= c->Read("/svn/tortoiseproc");
  defaultCheckoutDir	= c->Read("/svn/defaultCheckoutDir");

  c->Read("/svn/auto_add", &auto_add);
  c->Read("/svn/auto_add_only_project", &auto_add_only_project);
  c->Read("/svn/auto_delete", &auto_delete);
  c->Read("/svn/update_on_conflict", &update_on_conflict);
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

  if(tortoiseproc.IsEmpty())
    TamperWithWindowsRegistry();

  if(svnbinary == "unset")
    {
      svnbinary = "svn.exe";
      OnFirstRun();
    }

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

void SubversionPlugin::WriteConfig()
{
  wxConfigBase* c = ConfigManager::Get();

  c->Write("/svn/svnbinary", svnbinary);
  c->Write("/svn/tortoiseproc", tortoiseproc);
  c->Write("/svn/defaultCheckoutDir", defaultCheckoutDir);

  c->Write("/svn/auto_add", auto_add);
  c->Write("/svn/auto_add_only_project", auto_add_only_project);
  c->Write("/svn/auto_delete", auto_delete);
  c->Write("/svn/update_on_conflict", update_on_conflict);
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

  unsigned int count = min(16, repoHistory.Count());
  wxString ht("/svn/repoHist");
  for(unsigned int i = 0; i < count; ++i)
    {
      wxString h = ht;
      c->Write(h<<i, repoHistory[i]);
    }
}


void SubversionPlugin::Add(CodeBlocksEvent& event)
{
  svn->Add(GetSelection());
}

void SubversionPlugin::Delete(CodeBlocksEvent& event)
{
  wxString selected(GetSelection());

  if(never_ask)
    svn->Force();

  if(never_ask || no_ask_revertable || wxMessageDialog(Manager::Get()->GetAppWindow(), "Issue a 'svn delete' on the file " + selected + "?\n\nThis will not only delete the from disk, but also remove it from revision control.", "Delete File", wxYES_NO).ShowModal() == wxID_YES)
    {
      wxTreeCtrl* tree = Manager::Get()->GetProjectManager()->GetTree();
      FileTreeData* ftd	= (FileTreeData*) tree->GetItemData(tree->GetSelection());
      cbProject *p = ftd->GetProject();
      p->RemoveFile(ftd->GetFileIndex());
      Manager::Get()->GetProjectManager()->RebuildTree();
      svn->Delete(selected);
      if(svn->std_err.Count())
        ; // display something
    }
}



void SubversionPlugin::PropIgnore(CodeBlocksEvent& event)
{}
void SubversionPlugin::PropMime(CodeBlocksEvent& event)
{}
void SubversionPlugin::PropExec(CodeBlocksEvent& event)
{}
void SubversionPlugin::PropExt(CodeBlocksEvent& event)
{}
void SubversionPlugin::PropKeywords(CodeBlocksEvent& event)
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





void SubversionPlugin::ProjectOpen(CodeBlocksEvent& event)
{}




void SubversionPlugin::OnFatTortoiseFunctionality(CodeBlocksEvent& event)
{
  assert(event.GetId() >= ID_MENU_BRANCH && event.GetId() <= ID_MENU_RELOCATE);

  wxString cmd(" /command:");

  switch(event.GetId())
    {
    case ID_MENU_BRANCH:
      cmd << "copy";
      break;

    case ID_MENU_SWITCH:
      cmd << "switch";
      break;

    case ID_MENU_MERGE:
      cmd << "merge";
      break;

    case ID_MENU_CREATE:
      cmd << "repocreate";
      break;

    case ID_MENU_RELOCATE:
      cmd << "relocate";
      break;
    };


  wxString p(GetSelection());

  if(! p.IsEmpty())
    {
      cmd << " /path:" << Escape(p);
    }
  tortoise->Run(cmd);
}


void SubversionPlugin::EditProperty(wxEvent& event)
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
  file = LocalPath(file);
  file = file == "" ? "[top dir]" : file;
  d.SetTitle("Property editor: " + file);

  if(d.ShowModal() == wxID_OK)
    {
      svn->PropSet(file, d.name, d.value, true);
    }
  else if(d.del && ( never_ask || no_ask_revertable || wxMessageDialog(Manager::Get()->GetAppWindow(), "Remove the property " + name + " from " + file + " ?", "Delete Property", wxYES_NO).ShowModal() == wxID_YES))
    {
      svn->PropDel(file, d.name);
    }
}





































/*-----------------------------------------------------------------------------------------------------------------
*
*  DO NOT LOOK ANY FURTHER. BEYOND THIS POINT, THINGS ARE REALLY EVIL. YOU HAVE BEEN WARNED.
* 
*/

void SubversionPlugin::OnFirstRun()
{
#ifdef __WIN32__

  if(tortoiseproc.IsEmpty())
    tortoiseproc 	= NastyFind("TortoiseProc.exe");

  svnbinary		= NastyFind("svn.exe");

#else

  svnbinary		= NastyFind("svn");

#endif

  WriteConfig();
}

void SubversionPlugin::TamperWithWindowsRegistry()
{
#ifdef __WIN32__
  wxRegKey* rKey;
  wxString tort_bin;
  wxString tmerg_bin;
  wxString svn_bin;

  rKey = new wxRegKey("HKEY_LOCAL_MACHINE\\SOFTWARE\\TortoiseSVN");
  if( rKey->Exists() )
    {
      rKey->QueryValue("ProcPath", tort_bin);

      if(wxFile::Exists(tort_bin))
        tortoiseproc = tort_bin;
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
    return(name);
#endif

#ifdef __linux__

  wxArrayString location;
  location.Add("/usr/bin");			// this is probably it, anyway
  location.Add("/usr/local/bin");
  location.Add("/usr/share/bin");
  location.Add("/bin");
  location.Add("/opt/bin");
  location.Add("/usr/sbin");		// uhh... hopefully not, but who knows...
  location.Add("/sbin");			// everything is possible
  for(int i = 0; i < location.GetCount(); i++)
    {
      wxString loc = location[i] + "/" + name;
      if(wxFile::Exists(loc))
        return loc;
    }

  // similar to above - if we can't find svn, we will assume (hope) it is in $PATH and just call "svn"
  // - if the user actually works with svn at all, this should be the case
  return(name);

#endif

  return wxEmptyString;
}




