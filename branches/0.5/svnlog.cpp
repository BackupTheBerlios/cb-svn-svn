// This file is part of the Code::Blocks SVN Plugin
// Copyright (C) 2005 Thomas Denk
//
// This program is licensed under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// $HeadURL$
// $Id$


#include "precompile.h"


BEGIN_EVENT_TABLE(SVNLog, SimpleTextLog)
EVT_BUTTON(ID_SKULL_BUTTON,   SVNLog::Forward)
EVT_CHECKBOX(ID_VERBOSE_CHECK, SVNLog::Forward)
END_EVENT_TABLE()

SVNLog::SVNLog(wxNotebook* parent, const wxString& title, wxBitmap *skull)
    : SimpleTextLog(parent, title)
{
  wxSizer* bs = GetTextControl()->GetContainingSizer();
  if (bs)
    {
      wxFlexGridSizer *grd = new wxFlexGridSizer(3, 0, 0);
      grd->AddGrowableCol(1);
      
      wxCheckBox *c = new wxCheckBox( this, ID_VERBOSE_CHECK, wxT("Show all output (\"verbose logging\")"), wxDefaultPosition, wxDefaultSize, 0 );
      
      SubversionPlugin *plugin = (SubversionPlugin *)(Manager::Get()->GetPluginManager()->FindPluginByName("svn"));
      if(plugin)
        c->SetValue(plugin->verbose);
        
      grd->Add(c, 0, wxALIGN_CENTER|wxRIGHT|wxTOP, 5);
      
      wxButton *b = new wxButton(this, ID_LOG_CLEAR, "C");
      grd->Add(b, 0, wxALIGN_RIGHT, 0);
      
      if(skull)
        {
          wxBitmapButton *bb = new wxBitmapButton( this, ID_SKULL_BUTTON, *skull, wxDefaultPosition, wxDefaultSize );
          grd->Add(bb, 0, wxALIGN_RIGHT, 0);
        }
      bs->Add(grd, 0, wxGROW | wxALL, 5);
    }
}


SVNLog::~SVNLog()
{}


void SVNLog::Forward(wxCommandEvent& event)
{
  SubversionPlugin *plugin = (SubversionPlugin *)(Manager::Get()->GetPluginManager()->FindPluginByName("svn"));
  if(plugin)
    {
      if(event.GetId() == ID_SKULL_BUTTON)
        plugin->Kill();
      if(event.GetId() == ID_VERBOSE_CHECK)
        {
          plugin->Verbose(event.IsChecked());
        }
    }
}
