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


#include <wx/filesys.h>
#include <wx/image.h>

#include <manager.h>
#include <messagemanager.h>
#include <configmanager.h>
#include <simpletextlog.h>


#include "log.h"


Log::Log()
{
  MessageManager* msgMan = Manager::Get()->GetMessageManager();

  m_log = new SimpleTextLog(msgMan, "cb::svn");

  m_log->GetTextControl()->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxWHITE, wxFont(8, wxMODERN, wxNORMAL, wxNORMAL)));

  index = msgMan->AddLog(m_log);

  wxFileSystem fs;
  wxString zip(ConfigManager::Get()->Read("data_path") + "/svn.zip#zip:\\");

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
  Manager::Get()->GetMessageManager()->SwitchTo(index);
}
