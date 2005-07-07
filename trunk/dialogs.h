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

#ifndef CHECKOUT_DIALOG_H
#define CHECKOUT_DIALOG_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "svn.h"

enum {  ID_MENU_SVN = 32000,
        ID_MENU_CHECKOUT,
        ID_MENU_IMPORT,
        ID_MENU,
        ID_MENU_COMMIT,
        ID_MENU_UPDATE,
        ID_MENU_UP_P,
        ID_MENU_UP_C,
        ID_MENU_UP_B,
        ID_MENU_UP_REV,
        ID_MENU_REVERT,
        ID_MENU_LOG,
        ID_MENU_D_H,
        ID_MENU_D_P,
        ID_MENU_D_C,
        ID_MENU_D_B,
        ID_MENU_D_REV,
        ID_MENU_PREFS,
        ID_MENU_ED_DIFF,
        ID_MENU_ADD,
        ID_MENU_DELETE,
        ID_MENU_RESTORE,
        ID_MENU_P_HIST,
        ID_MENU_IGNORE_LIST,
        ID_MENU_BRANCH,
        ID_MENU_SWITCH,
        ID_MENU_MERGE,
        ID_MENU_CREATE,
        ID_MENU_RELOCATE,
        ID_MENU_NEW_PROPERTY,
        ID_MENU_RESOLVETOOL,

        ID_MENU_PROP_IGNORE,
        ID_MENU_PROP_MIME,
        ID_MENU_PROP_EXECUTABLE,
        ID_MENU_PROP_EXTERNALS,
        ID_MENU_KW_DATE,
        ID_MENU_KW_REVISION,
        ID_MENU_KW_AUTHOR,
        ID_MENU_KW_HEAD,
        ID_MENU_KW_ID,
        ID_MENU_PROP_NEW,
        ID_MENU_RESOLVEPROP,
        ID_MENU_KW_SETALL,
        ID_MENU_KW_CLEARALL
     };

class CheckoutDialog : public wxDialog
  {
  public:
    CheckoutDialog(wxWindow* parent, const wxArrayString& repoHist, const wxString & );
    ~CheckoutDialog()
    {}
    ;

    wxString	checkoutDir;		// too lazy to write accessor funcs, so these be public, not much harm really
    wxString	repoURL;
    wxString	username;
    wxString	password;
    wxString	revision;
    bool		autoOpen;

  private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    void OnFileSelect(wxUpdateUIEvent& event);
    DECLARE_EVENT_TABLE()
  };

class ImportDialog : public wxDialog
  {
  public:
    ImportDialog::ImportDialog(wxWindow* parent, const wxArrayString& repoHist, const wxString& importDir);

    ~ImportDialog()
    {}
    ;

    wxString	importDir;
    wxString	repoURL;
    wxString	username;
    wxString	password;
    wxString	comment;
    bool		trunkify;

  private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
  };



class CommitDialog : public wxDialog
  {
  public:
    CommitDialog(wxWindow* parent, const wxArrayString& addList);
    ~CommitDialog()
    {}
    ;

    wxString comment;
    wxArrayString finalList;
  private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    void SelectNone(wxCommandEvent& event);
    void SelectAll(wxCommandEvent& event);
    void CommitDialog::Selected(wxCommandEvent& event);
    bool extended;
    DECLARE_EVENT_TABLE()
  };

class PreferencesDialog : public wxDialog
  {
    SubversionPlugin* plugin;

  public:
    PreferencesDialog(wxWindow* parent, SubversionPlugin* plugin);
    ~PreferencesDialog()
    {}
    ;
    void RadioToggle(wxCommandEvent& event);
  private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
  };


class PasswordDialog : public wxDialog
  {
  public:
    PasswordDialog(wxWindow* parent);
    ~PasswordDialog()
    {}
    ;

    wxString username;
    wxString password;

  private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
  };


#endif
