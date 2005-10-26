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

#include <wx/filesys.h>
#include <wx/image.h>

#include <manager.h>
#include <messagemanager.h>
#include <configmanager.h>
#include "svnlog.h"


#include "log.h"

int Log::lastLogTime = 0;

Log::Log()
{
  MessageManager* msgMan = Manager::Get()->GetMessageManager();
  
  wxFileSystem fs;
  wxString zip(ConfigManager::Get()->Read("data_path") + "/svn.zip#zip:\\");
  
  
  if(wxFSFile* file = fs.OpenFile(zip + "skull.png"))
    {
      wxBitmap skull(wxImage(*(file->GetStream()), wxBITMAP_TYPE_PNG));
      delete file;
      m_log = new SVNLog(msgMan, "cb::svn", &skull);
    }
  else
    m_log = new SVNLog(msgMan, "cb::svn");
    
  m_log->GetTextControl()->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxWHITE, wxFont(8, wxMODERN, wxNORMAL, wxNORMAL)));
  
  index = msgMan->AddLog(m_log);
  
  if(wxFSFile* file = fs.OpenFile(zip + "log.png"))
    {
      wxBitmap bmp(wxImage(*(file->GetStream()), wxBITMAP_TYPE_PNG));
      delete file;
      msgMan->SetLogImage(index, bmp);
    }
    
    
}

Log::~Log()
{}

void Log::Add(wxString str)
{
  m_log->AddLog(str);
  lastLogTime = wxGetLocalTime();
}

void Log::Red(wxString str)
{
  m_log->GetTextControl()->SetDefaultStyle(wxTextAttr(*wxRED, *wxWHITE));
  Add(str);
  m_log->GetTextControl()->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxWHITE));
}
void Log::Blue(wxString str)
{
  m_log->GetTextControl()->SetDefaultStyle(wxTextAttr(*wxBLUE, *wxWHITE));
  Add(str);
  m_log->GetTextControl()->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxWHITE));
}
void Log::Grey(wxString str)
{
  m_log->GetTextControl()->SetDefaultStyle(wxTextAttr(*wxLIGHT_GREY, *wxWHITE));
  Add(str);
  m_log->GetTextControl()->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxWHITE));
}

