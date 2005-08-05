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


#include <wx/wx.h>



enum {  ID_MENU_USER = 32000,
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
		ID_MENU_CONFLICTS,
        
        ID_MENU_PROP_IGNORE,
        ID_MENU_PROP_MIME,
        ID_MENU_PROP_EXECUTABLE,
        ID_MENU_PROP_NEEDSLOCK,
        ID_MENU_PROP_EXTERNALS,
        ID_MENU_KW_DATE,
        ID_MENU_KW_REVISION,
        ID_MENU_KW_AUTHOR,
        ID_MENU_KW_HEAD,
        ID_MENU_KW_ID,
        ID_MENU_PROP_NEW,
        ID_MENU_RESOLVEPROP,
        ID_MENU_KW_SETALL,
        ID_MENU_KW_CLEARALL,
		ID_MENU_PATCH,
		ID_MENU_LOCK,
		ID_MENU_UNLOCK,
        
        ID_MENU_CVS_BRANCH,
        ID_MENU_CVS_TAG,
        ID_MENU_CVS_MERGE,
        ID_MENU_CVS_PATCH,
        
        ID_MENU_CVS_COMMIT,
        ID_MENU_CVS_UPDATE,
        ID_MENU_CVS_UPDATE_R,
        ID_MENU_CVS_UPDATE_D,
        ID_MENU_CVS_LOGIN,
        
        ID_COMBO_SRC = 32600,
        ID_COMBO_DEST,
        
        ID_TIMER,
        ID_PROCESS,
     };
     
class CheckoutDialog : public wxDialog
{
public:
    CheckoutDialog(wxWindow* parent, const wxArrayString& repoHist, const wxArrayString& repoHistCVS, const wxString & );
    ~CheckoutDialog()
    {}
    ;
    
    wxString checkoutDir;  // too lazy to write accessor funcs, so these be public, not much harm really
    wxString repoURL;
    wxString username;
    wxString password;
    wxString revision;
    bool  autoOpen;
    bool  noExternals;
    
    bool  use_cvs_instead;
    
    wxString cvs_workingdir;
    wxString cvs_repo;
    wxString cvs_proto;
    wxString cvs_module;
    wxString cvs_user;
    wxString cvs_pass;
    bool  cvs_auto_open;
    wxString cvs_revision;
    
private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    void OnFileSelect(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};

class ImportDialog : public wxDialog
{
public:
    ImportDialog::ImportDialog(wxWindow* parent, const wxArrayString& repoHist, const wxString& importDir, bool no_empty);
    
    ~ImportDialog()
    {}
    ;
    
    wxString importDir;
    wxString repoURL;
    wxString username;
    wxString password;
    wxString comment;
    bool  trunkify;
    
    wxString keywords;
    wxString ignore;
    wxString externals;
    wxString copy;
    wxString home;
    wxString docs;
    wxString contact;
    wxString arch;
    
private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    bool no_empty;
    DECLARE_EVENT_TABLE()
};



class CommitDialog : public wxDialog
{
public:
    CommitDialog(wxWindow* parent, const wxArrayString& addList, bool no_empty_c);
    ~CommitDialog()
    {}
    ;
    void SetComment(const wxString& cmt);
    wxString comment;
    wxArrayString finalList;
private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    void SelectNone(wxCommandEvent& event);
    void SelectAll(wxCommandEvent& event);
    void CommitDialog::Selected(wxCommandEvent& event);
    bool extended;
    bool no_empty;
    DECLARE_EVENT_TABLE()
};

class PreferencesDialog : public wxDialog
{
public:
    PreferencesDialog(wxWindow* parent);
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

class PropertyEditorDialog : public wxDialog
{
public:
    PropertyEditorDialog(wxWindow* parent, const wxString& name, const wxString& value);
    ~PropertyEditorDialog()
    {}
    ;
    
    wxString name;
    wxString value;
    bool del;
    
private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    void OnDeleteClick(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};


class IgnoreEditorDialog : public wxDialog
{
    wxString dir;
public:
    IgnoreEditorDialog(wxWindow* parent, const wxString& target, const wxString& value, const wxString& d);
    ~IgnoreEditorDialog()
    {}
    ;
    
    wxString value;
    bool del;
    
private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    void OnSelectClick(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};



class RevertDialog : public wxDialog
{
public:
    RevertDialog(wxWindow* parent, const wxArrayString& revertList, const wxArrayString& files);
    ~RevertDialog()
    {}
    ;
    wxArrayString files;
    wxArrayString finalList;
private:
    void OnOKClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    void SelectNone(wxCommandEvent& event);
    void SelectAll(wxCommandEvent& event);
    void Selected(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};



#endif
