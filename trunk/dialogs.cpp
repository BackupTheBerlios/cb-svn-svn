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
#include <wx/checklst.h>

#include "dialogs.h"
#include "svn.h"

#include <wx/xrc/xmlres.h>
#include <manager.h>
#include <messagemanager.h>


// --- Checkout Dialog ---------------------------------------------------

BEGIN_EVENT_TABLE(CheckoutDialog, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), CheckoutDialog::OnOKClick)
EVT_BUTTON(XRCID("wxID_CANCEL"), CheckoutDialog::OnCancelClick)
EVT_BUTTON(XRCID("fileselect"), CheckoutDialog::OnFileSelect)
END_EVENT_TABLE()


CheckoutDialog::CheckoutDialog(wxWindow* parent, const wxArrayString& repoHist, const wxString& defaultCheckoutDir)
{

  wxXmlResource::Get()->LoadDialog(this, parent, "Checkout");

  wxComboBox* c = XRCCTRL(*this, "repository url", wxComboBox);

  for(int i = 0; i < repoHist.Count(); i++)
    c->Append(repoHist[i]);

  if(repoHist.Count())
    c->SetSelection(0);

  XRCCTRL(*this, "working dir", wxTextCtrl)->SetValue(defaultCheckoutDir);
}

void CheckoutDialog::OnFileSelect(wxUpdateUIEvent& event)
{
  XRCCTRL(*this, "working dir", wxTextCtrl)->SetValue( ::wxDirSelector("Choose the checkout directory", XRCCTRL(*this, "working dir", wxTextCtrl)->GetValue(), wxDD_NEW_DIR_BUTTON) );
}

void CheckoutDialog::OnOKClick(wxCommandEvent& event)
{
  checkoutDir	= XRCCTRL(*this, "working dir", wxTextCtrl)->GetValue();
  repoURL		= XRCCTRL(*this, "repository url", wxComboBox)->GetValue();
  username	= XRCCTRL(*this, "username", wxTextCtrl)->GetValue();
  password	= XRCCTRL(*this, "password", wxTextCtrl)->GetValue();
  revision	= XRCCTRL(*this, "revision", wxComboBox)->GetValue();
  autoOpen	= XRCCTRL(*this, "auto_open", wxCheckBox)->GetValue();

  EndModal(wxID_OK);
}

void CheckoutDialog::OnCancelClick(wxCommandEvent& event)
{
  EndModal(wxID_CANCEL);
}



// --- Import Dialog ---------------------------------------------------

BEGIN_EVENT_TABLE(ImportDialog, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), ImportDialog::OnOKClick)
EVT_BUTTON(XRCID("wxID_CANCEL"), ImportDialog::OnCancelClick)
END_EVENT_TABLE()


ImportDialog::ImportDialog(wxWindow* parent, const wxArrayString& repoHist, const wxString& imp)
{
  wxXmlResource::Get()->LoadDialog(this, parent, "Import");

  wxComboBox* c = XRCCTRL(*this, "repository url", wxComboBox);

  for(int i = 0; i < repoHist.Count(); i++)
    c->Append(repoHist[i]);

  XRCCTRL(*this, "source dir", wxTextCtrl)->SetValue(imp);
}

void ImportDialog::OnOKClick(wxCommandEvent& event)
{
  importDir	= XRCCTRL(*this, "source dir", wxTextCtrl)->GetValue();
  repoURL	= XRCCTRL(*this, "repository url", wxComboBox)->GetValue();
  username	= XRCCTRL(*this, "username", wxTextCtrl)->GetValue();
  password	= XRCCTRL(*this, "password", wxTextCtrl)->GetValue();
  comment	= XRCCTRL(*this, "comment", wxComboBox)->GetValue();
  trunkify	= XRCCTRL(*this, "trunkify", wxCheckBox)->GetValue();

  EndModal(wxID_OK);
}

void ImportDialog::OnCancelClick(wxCommandEvent& event)
{
  EndModal(wxID_CANCEL);
}




// --- Commit Dialog ---------------------------------------------------

BEGIN_EVENT_TABLE(CommitDialog, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), CommitDialog::OnOKClick)
EVT_BUTTON(XRCID("wxID_CANCEL"), CommitDialog::OnCancelClick)

EVT_CHECKBOX(XRCID("select all"),  CommitDialog::SelectAll)
EVT_CHECKBOX(XRCID("select none"), CommitDialog::SelectNone)
EVT_CHECKLISTBOX(-1, CommitDialog::Selected)

END_EVENT_TABLE()


CommitDialog::CommitDialog(wxWindow* parent, const wxArrayString& addList)
{
  if(addList.Count() == 0)
    {
      extended = false;
      wxXmlResource::Get()->LoadDialog(this, parent, "Commit");
    }
  else
    {
      extended = true;
      wxXmlResource::Get()->LoadDialog(this, parent, "CommitAdd");
      wxCheckListBox* list = XRCCTRL(*this, "listcontrol", wxCheckListBox);
      assert(list);
      for(int i = 0; i < addList.Count(); ++i)
        {
          list->Append(addList[i]);
          list->Check(i, true);
        }
    }
}

void CommitDialog::Selected(wxCommandEvent& event)
{
  XRCCTRL(*this, "select all", wxCheckBox)->SetValue(false);
  XRCCTRL(*this, "select none", wxCheckBox)->SetValue(false);
}

void CommitDialog::SelectNone(wxCommandEvent& event)
{
  wxCheckListBox* clist = XRCCTRL(*this, "listcontrol", wxCheckListBox);
  for(int i = 0; i < clist->GetCount(); ++i)
    {
      clist->Check(i, false);
    }
  XRCCTRL(*this, "select all", wxCheckBox)->SetValue(false);
}

void CommitDialog::SelectAll(wxCommandEvent& event)
{
  wxCheckListBox* clist = XRCCTRL(*this, "listcontrol", wxCheckListBox);
  for(int i = 0; i < clist->GetCount(); ++i)
    {
      clist->Check(i, true);
    }
  XRCCTRL(*this, "select none", wxCheckBox)->SetValue(false);
}

void CommitDialog::OnOKClick(wxCommandEvent& event)
{
  comment	= XRCCTRL(*this, "comment", wxTextCtrl)->GetValue();
  if(extended)
    {
      wxCheckListBox* clist = XRCCTRL(*this, "listcontrol", wxCheckListBox);
      for(int i = 0; i < clist->GetCount(); ++i)
        {
          if(clist->IsChecked(i))
            finalList.Add(clist->GetString(i));
        }
    }
  else
    {
      finalList.Clear();
    }
  EndModal(wxID_OK);
}

void CommitDialog::OnCancelClick(wxCommandEvent& event)
{
  EndModal(wxID_CANCEL);
}




// --- Preferences Dialog ---------------------------------------------------

BEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), 	PreferencesDialog::OnOKClick)
EVT_BUTTON(XRCID("wxID_CANCEL"), PreferencesDialog::OnCancelClick)
EVT_CHECKBOX(-1,  PreferencesDialog::RadioToggle)
END_EVENT_TABLE()


PreferencesDialog::PreferencesDialog(wxWindow* parent, SubversionPlugin* plugin)
{
  PreferencesDialog::plugin = plugin;
  wxXmlResource::Get()->LoadDialog(this, parent, "Preferences");
}


void PreferencesDialog::OnOKClick(wxCommandEvent& event)
{
  EndModal(wxID_OK);
}

void PreferencesDialog::OnCancelClick(wxCommandEvent& event)
{
  EndModal(wxID_CANCEL);
}

void PreferencesDialog::RadioToggle(wxCommandEvent& event)
{
  XRCCTRL(*this, "autoadd only project", wxCheckBox)->Enable(XRCCTRL(*this, "auto add", wxCheckBox)->GetValue());
  XRCCTRL(*this, "prefill comments", wxCheckBox)->Enable(XRCCTRL(*this, "no emtpy comments", wxCheckBox)->GetValue());
}


// --- Password Dialog ---------------------------------------------------

BEGIN_EVENT_TABLE(PasswordDialog, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), PasswordDialog::OnOKClick)
EVT_BUTTON(XRCID("wxID_CANCEL"), PasswordDialog::OnCancelClick)
END_EVENT_TABLE()


PasswordDialog::PasswordDialog(wxWindow* parent)
{
  wxXmlResource::Get()->LoadDialog(this, parent, "Password");
}


void PasswordDialog::OnOKClick(wxCommandEvent& event)
{
  username	= XRCCTRL(*this, "username", wxTextCtrl)->GetValue();
  password	= XRCCTRL(*this, "password", wxTextCtrl)->GetValue();
  EndModal(wxID_OK);
}

void PasswordDialog::OnCancelClick(wxCommandEvent& event)
{
  username = password = "";
  EndModal(wxID_CANCEL);
}


