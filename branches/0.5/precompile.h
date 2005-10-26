#ifndef __PRECOMPILE_H__
#define __PRECOMPILE_H__

#ifdef __WIN32__
const bool filenames_are_unix = false;
#else
const bool filenames_are_unix = true;
#endif

#include <wx/wx.h>

#include <wx/checklst.h>
#include <wx/textctrl.h>
#include <wx/regex.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>
#include <wx/filesys.h>
#include <wx/fs_zip.h>
#include <wx/image.h>
#include <wx/event.h>


#include <sdk_events.h>
#include <manager.h>
#include <messagemanager.h>
#include <configmanager.h>
#include <editormanager.h>
#include <pluginmanager.h>
#include <cbproject.h>
#include <pipedprocess.h>
#include <projectbuildtarget.h>
#include <cbexception.h>

#include "singleton.h"
#include "log.h"
#include "dialogs.h"
#include "svn.h"
#include "svnlog.h"
#include "toolrunner.h"
#include "svnrunner.h"
#include "cvsrunner.h"
#include "tortoiserunner.h"
#include "smart.h"

#ifdef __WIN32__
 #include <shlobj.h>
 #include <wx/msw/registry.h>
#endif


#endif
