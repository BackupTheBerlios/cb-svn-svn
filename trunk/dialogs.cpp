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
#include <wx/textctrl.h>
#include <wx/regex.h>
#include <wx/notebook.h>

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
EVT_BUTTON(XRCID("cvs.fileselect"), CheckoutDialog::OnFileSelect)
END_EVENT_TABLE()


CheckoutDialog::CheckoutDialog(wxWindow* parent, const wxArrayString& repoHist, const wxString& defaultCheckoutDir)
{

  wxXmlResource::Get()->LoadDialog(this, parent, "Checkout");

  wxComboBox* c = XRCCTRL(*this, "repository url", wxComboBox);
  assert(c);

  for(int i = 0; i < repoHist.Count(); i++)
    c->Append(repoHist[i]);

  if(repoHist.Count())
    c->SetSelection(0);

  XRCCTRL(*this, "working dir", wxTextCtrl)->SetValue(defaultCheckoutDir);
  XRCCTRL(*this, "cvs_workingdir", wxTextCtrl)->SetValue(defaultCheckoutDir);
}

void CheckoutDialog::OnFileSelect(wxUpdateUIEvent& event)
{
  if(XRCCTRL(*this, "notebook", wxNotebook)->GetSelection() == 0)
    XRCCTRL(*this, "working dir", wxTextCtrl)->SetValue( ::wxDirSelector("Choose the checkout directory", XRCCTRL(*this, "working dir", wxTextCtrl)->GetValue(), wxDD_NEW_DIR_BUTTON) );
  else
    XRCCTRL(*this, "cvs_workingdir", wxTextCtrl)->SetValue( ::wxDirSelector("Choose the checkout directory", XRCCTRL(*this, "cvs_workingdir", wxTextCtrl)->GetValue(), wxDD_NEW_DIR_BUTTON) );
}

void CheckoutDialog::OnOKClick(wxCommandEvent& event)
{
  checkoutDir	= XRCCTRL(*this, "working dir", wxTextCtrl)->GetValue();
  repoURL		= XRCCTRL(*this, "repository url", wxComboBox)->GetValue();
  username		= XRCCTRL(*this, "username", wxTextCtrl)->GetValue();
  password		= XRCCTRL(*this, "password", wxTextCtrl)->GetValue();
  revision		= XRCCTRL(*this, "revision", wxComboBox)->GetValue();
  autoOpen		= XRCCTRL(*this, "auto_open", wxCheckBox)->GetValue();

  cvs_workingdir	= XRCCTRL(*this, "cvs_workingdir", wxTextCtrl)->GetValue();
  cvs_repo			= XRCCTRL(*this, "cvs_repo", wxTextCtrl)->GetValue();
  cvs_module		= XRCCTRL(*this, "cvs_module", wxTextCtrl)->GetValue();
  cvs_user			= XRCCTRL(*this, "cvs_user", wxTextCtrl)->GetValue();
  cvs_pass			= XRCCTRL(*this, "cvs_pass", wxTextCtrl)->GetValue();
  cvs_proto			= XRCCTRL(*this, "cvs_proto", wxComboBox)->GetValue();
  cvs_auto_open		= XRCCTRL(*this, "cvs_auto_open", wxCheckBox)->GetValue();
  cvs_revision		= XRCCTRL(*this, "cvs_revision", wxTextCtrl)->GetValue();
  use_cvs_instead 	= XRCCTRL(*this, "notebook", wxNotebook)->GetSelection();

  if(use_cvs_instead)
    {
      if(cvs_repo.IsEmpty() || cvs_module.IsEmpty())
        {
          wxMessageDialog(Manager::Get()->GetAppWindow(),
                          "You must specify both a repository and a module name to be able to check out.",
                          "CVS Checkout", wxOK).ShowModal();
          return;
        }
    }
  else
  {
  if(repoURL.IsEmpty())
        {
          wxMessageDialog(Manager::Get()->GetAppWindow(),
                          "Please provide a repository URL in one of the following formats:\n\n"
                          "svn://some.server/path/from/svnroot/repo/\n"
                          "svn+ssh://some.server/absolute/path/to/repo/\n"
                          "http://some.server/repo/\n"
                          "https://some.server/repo/",
                          "SVN Checkout", wxOK).ShowModal();
          return;
        }
  }

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


ImportDialog::ImportDialog(wxWindow* parent, const wxArrayString& repoHist, const wxString& imp, bool no_empty_c) : no_empty(no_empty_c)
{
  wxXmlResource::Get()->LoadDialog(this, parent, "Import");

  wxComboBox* c = XRCCTRL(*this, "repository url", wxComboBox);
  assert(c);

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

  keywords.Clear();
  keywords << ( XRCCTRL(*this, "kw_revision", wxCheckBox)->GetValue()	? "Revision\n"	: "" );
  keywords << ( XRCCTRL(*this, "kw_author", wxCheckBox)->GetValue()		? "Author\n"	: "" );
  keywords << ( XRCCTRL(*this, "kw_headurl", wxCheckBox)->GetValue()	? "HeadURL\n"	: "" );
  keywords << ( XRCCTRL(*this, "kw_date", wxCheckBox)->GetValue()		? "Date\n"		: "" );
  keywords << ( XRCCTRL(*this, "kw_id", wxCheckBox)->GetValue()			? "Id\n"		: "" );

  ignore	= XRCCTRL(*this, "ignore", wxTextCtrl)->GetValue();
  externals	= XRCCTRL(*this, "externals", wxTextCtrl)->GetValue();
  copy		= XRCCTRL(*this, "copy", wxTextCtrl)->GetValue();
  home		= XRCCTRL(*this, "home", wxTextCtrl)->GetValue();
  docs		= XRCCTRL(*this, "docs", wxTextCtrl)->GetValue();
  contact	= XRCCTRL(*this, "contact", wxTextCtrl)->GetValue();
  arch		= XRCCTRL(*this, "arch", wxComboBox)->GetValue();

  if(no_empty && comment.IsEmpty())
    {
      wxMessageDialog(Manager::Get()->GetAppWindow(),
                      "According to your preferences, you are required to enter a comment.",
                      "Import", wxOK).ShowModal();
      return;
    }


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


CommitDialog::CommitDialog(wxWindow* parent, const wxArrayString& addList, bool no_empty_c) : no_empty(no_empty_c)
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

void CommitDialog::SetComment(const wxString& cmt)
{
  XRCCTRL(*this, "comment", wxTextCtrl)->SetValue(cmt);
}

void CommitDialog::OnOKClick(wxCommandEvent& event)
{
  comment	= XRCCTRL(*this, "comment", wxTextCtrl)->GetValue();

  if(no_empty && comment.IsEmpty())
    {
      wxMessageDialog(Manager::Get()->GetAppWindow(),
                      "According to your preferences, you are required to enter a comment.",
                      "Commit", wxOK).ShowModal();
      return;
    }

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



// --- Property Dialog ---------------------------------------------------

BEGIN_EVENT_TABLE(PropertyEditorDialog, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), PropertyEditorDialog::OnOKClick)
EVT_BUTTON(XRCID("wxID_CANCEL"), PropertyEditorDialog::OnCancelClick)
EVT_BUTTON(XRCID("delete"), PropertyEditorDialog::OnDeleteClick)
END_EVENT_TABLE()


PropertyEditorDialog::PropertyEditorDialog(wxWindow* parent, const wxString& n, const wxString& v) : del(0)
{
  wxXmlResource::Get()->LoadDialog(this, parent, "PropertyEditor");
  XRCCTRL(*this, "value", wxTextCtrl)->SetValue(v);
  XRCCTRL(*this, "property", wxTextCtrl)->SetValue(n);
}


void PropertyEditorDialog::OnOKClick(wxCommandEvent& event)
{
  value	= XRCCTRL(*this, "value", wxTextCtrl)->GetValue();
  name	= XRCCTRL(*this, "property", wxTextCtrl)->GetValue();
  name.Replace("\"", ""); // sure a validator would be better, if only they worked as you need them
  name.Replace(" ", "");
  EndModal(wxID_OK);
}

void PropertyEditorDialog::OnDeleteClick(wxCommandEvent& event)
{
  del = true;
  name	= XRCCTRL(*this, "property", wxTextCtrl)->GetValue();
  EndModal( wxID_CANCEL );
}

void PropertyEditorDialog::OnCancelClick(wxCommandEvent& event)
{
  EndModal(wxID_CANCEL);
}




// --- Ignore Dialog ---------------------------------------------------

BEGIN_EVENT_TABLE(IgnoreEditorDialog, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), IgnoreEditorDialog::OnOKClick)
EVT_BUTTON(XRCID("wxID_CANCEL"), IgnoreEditorDialog::OnCancelClick)
EVT_BUTTON(XRCID("select"), IgnoreEditorDialog::OnSelectClick)
END_EVENT_TABLE()


IgnoreEditorDialog::IgnoreEditorDialog(wxWindow* parent, const wxString& n, const wxString& v, const wxString& d)
{
  wxXmlResource::Get()->LoadDialog(this, parent, "svnIgnore");
  XRCCTRL(*this, "value", wxTextCtrl)->SetValue(v);
  SetTitle(n);
  dir = d;
}


void IgnoreEditorDialog::OnOKClick(wxCommandEvent& event)
{
  value	= XRCCTRL(*this, "value", wxTextCtrl)->GetValue();
  EndModal(wxID_OK);
}

void IgnoreEditorDialog::OnCancelClick(wxCommandEvent& event)
{
  EndModal(wxID_CANCEL);
}

void IgnoreEditorDialog::OnSelectClick(wxCommandEvent& event)
{
  wxFileDialog fd(this, "Choose files to be added to the ignore list", dir, "", "*.*", wxMULTIPLE);
  if(fd.ShowModal() == wxID_OK)
    {
      wxArrayString p;
      fd.GetPaths(p);

      value	= XRCCTRL(*this, "value", wxTextCtrl)->GetValue();
      for(int i = 0; i < p.Count(); ++i)
        {
          wxFileName fn(p[i]);
          fn.MakeRelativeTo(dir);

          value.Append(fn.GetFullPath()+"\n");
        }
      XRCCTRL(*this, "value", wxTextCtrl)->SetValue(value);
    }
}






// --- Revert Dialog ---------------------------------------------------

BEGIN_EVENT_TABLE(RevertDialog, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), RevertDialog::OnOKClick)
EVT_BUTTON(XRCID("wxID_CANCEL"), RevertDialog::OnCancelClick)

EVT_CHECKBOX(XRCID("select all"),  RevertDialog::SelectAll)
EVT_CHECKBOX(XRCID("select none"), RevertDialog::SelectNone)
EVT_CHECKLISTBOX(-1, RevertDialog::Selected)

END_EVENT_TABLE()


RevertDialog::RevertDialog(wxWindow* parent, const wxArrayString& revertList, const wxArrayString& fileNames)
{
  wxXmlResource::Get()->LoadDialog(this, parent, "Revert");
  wxCheckListBox* list = XRCCTRL(*this, "listcontrol", wxCheckListBox);
  assert(list);
  for(int i = 0; i < revertList.Count(); ++i)
    {
      list->Append(revertList[i]);
    }
  files = fileNames;
}

void RevertDialog::Selected(wxCommandEvent& event)
{
  XRCCTRL(*this, "select all", wxCheckBox)->SetValue(false);
  XRCCTRL(*this, "select none", wxCheckBox)->SetValue(false);
}

void RevertDialog::SelectNone(wxCommandEvent& event)
{
  wxCheckListBox* clist = XRCCTRL(*this, "listcontrol", wxCheckListBox);
  for(int i = 0; i < clist->GetCount(); ++i)
    {
      clist->Check(i, false);
    }
  XRCCTRL(*this, "select all", wxCheckBox)->SetValue(false);
}

void RevertDialog::SelectAll(wxCommandEvent& event)
{
  wxCheckListBox* clist = XRCCTRL(*this, "listcontrol", wxCheckListBox);
  for(int i = 0; i < clist->GetCount(); ++i)
    {
      clist->Check(i, true);
    }
  XRCCTRL(*this, "select none", wxCheckBox)->SetValue(false);
}

void RevertDialog::OnOKClick(wxCommandEvent& event)
{
  wxCheckListBox* clist = XRCCTRL(*this, "listcontrol", wxCheckListBox);
  for(int i = 0; i < clist->GetCount(); ++i)
    {
      if(clist->IsChecked(i))
        finalList.Add(files[i]);
    }
  EndModal(wxID_OK);
}

void RevertDialog::OnCancelClick(wxCommandEvent& event)
{
  EndModal(wxID_CANCEL);
}







// --- Diff Dialog ---------------------------------------------------

DiffDialog::DiffDialog(wxWindow *parent, SVNRunner *s)
    : wxDialog(parent, -1, "Poor Man's 'diff' viewer", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, ""), svn(s)
{
  wxFlexGridSizer *flex = new wxFlexGridSizer( 1, 0, 0 );
  flex->AddGrowableCol( 0 );
  flex->AddGrowableCol( 1 );
  flex->AddGrowableRow( 1 );

  wxBoxSizer *box = new wxBoxSizer( wxHORIZONTAL );

  wxStaticText *txt = new wxStaticText( this, -1, "Compare", wxDefaultPosition, wxDefaultSize, 0 );
  box->Add( txt, 0, wxALIGN_CENTER|wxALL, 5 );

  wxString strs[] =
    {
      "Local Version",
      "Editor Contents",
      "HEAD Revision"
    };
  wxComboBox *src = new wxComboBox( this, ID_COMBO_SRC, "", wxDefaultPosition, wxSize(100,-1), 3, strs, wxCB_DROPDOWN );
  src->Enable(false);
  box->Add( src, 0, wxALIGN_CENTER|wxALL, 5 );

  txt = new wxStaticText( this, -1, "against", wxDefaultPosition, wxDefaultSize, 0 );
  box->Add( txt, 0, wxALIGN_CENTER|wxALL, 5 );

  wxComboBox *dest = new wxComboBox( this, ID_COMBO_DEST, "", wxDefaultPosition, wxSize(100,-1), 0, NULL, wxCB_DROPDOWN );
  box->Add( dest, 0, wxALIGN_CENTER|wxALL, 5 );
  dest->Enable(false);

  flex->Add( box, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

  diff = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize,
                        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxHSCROLL);

  flex->Add( diff, 0, wxGROW|wxALL, 5 );

  SetSizer( flex );
  flex->SetSizeHints( this );

}


void   DiffDialog::LoadDiff(const wxString& selected, const wxString& revision)
{
  wxArrayString text;

  wxTextAttr black(*wxBLACK);
  wxTextAttr red(*wxRED);
  wxTextAttr blue(*wxBLUE);

  diff->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, wxFont(8, wxMODERN, wxNORMAL, wxNORMAL, false)));

  if(revision.Contains(":"))
    svn->Cat(selected, revision.Mid(0, revision.Index(':')));
  else
    svn->Cat(selected, "");

  if(svn->lastExitCode)
    {
      diff->SetDefaultStyle(red);
      for(int i = 0; i < svn->std_err.Count(); ++i)
        diff->AppendText(svn->std_err[i] + "\n");
      return;
    }
  text = svn->std_out;

  svn->Diff(selected, revision);

  if(!svn->blob.Contains("@@"))
    {
      diff->AppendText("No changes.\n");
      return;
    }

  if(svn->lastExitCode)
    {
      diff->SetDefaultStyle(red);
      for(int i = 0; i < svn->std_err.Count(); ++i)
        diff->AppendText(svn->std_err[i] + "\n");
      return;
    }

  wxRegEx reg("@@.*\\+([0-9]+),.*@@");

  int textPos = 0;

  int count = svn->std_out.Count();
  for(int i = 0; i < count; ++i)
    {
      if(svn->std_out[i].IsEmpty())
        break;
      if(svn->std_out[i].Contains("@@"))
        if(reg.Matches(svn->std_out[i]))
          {
            unsigned long pos = 0;
            reg.GetMatch(svn->std_out[i], 1).ToULong(&pos);

            while(textPos < pos && textPos < text.Count())
              diff->AppendText(text[textPos++] + "\n");

            ++i;
            while(svn->std_out[i][(size_t)0] == ' '
                  || svn->std_out[i][(size_t)0] == '+'
                  || svn->std_out[i][(size_t)0] == '-')
              {
                if(svn->std_out[i][(size_t)0] == ' ')
                  diff->SetDefaultStyle(black);
                else if(svn->std_out[i][(size_t)0] == '+')
                  diff->SetDefaultStyle(red);
                else
                  diff->SetDefaultStyle(blue);

                diff->AppendText(svn->std_out[i].Mid(1) + "\n");

                ++i;
                ++textPos;
              }
          }
    }
  while(textPos < text.Count())
    diff->AppendText(text[textPos++] + "\n");

}




