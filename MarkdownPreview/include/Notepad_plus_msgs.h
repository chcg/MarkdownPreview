// This file is part of MarkdownPreview plugin for Notepad++
// Notepad++ message constants

#pragma once

#include "PluginInterface.h"

// Docking Manager messages
#define NPPM_DMMSHOW            (NPPMSG + 30)
#define NPPM_DMMHIDE            (NPPMSG + 31)
#define NPPM_DMMUPDATEDISPINFO  (NPPMSG + 32)
#define NPPM_DMMREGASDCKDLG     (NPPMSG + 33)

// Menu and UI messages
#define NPPM_SETMENUITEMCHECK   (NPPMSG + 40)

// Plugin config messages
#define NPPM_GETPLUGINSCONFIGDIR (NPPMSG + 46)
#define NPPM_GETPLUGINHOMEPATH   (NPPMSG + 49)

// File messages
#define NPPM_GETFULLCURRENTPATH  (NPPMSG + 17)
#define NPPM_GETCURRENTLANGTYPE  (NPPMSG + 5)

// Notification constants
#define NPPN_FIRST              1000
#define NPPN_READY              (NPPN_FIRST + 1)
#define NPPN_TBMODIFICATION     (NPPN_FIRST + 2)
#define NPPN_FILEBEFORECLOSE    (NPPN_FIRST + 3)
#define NPPN_FILEOPENED         (NPPN_FIRST + 4)
#define NPPN_FILECLOSED         (NPPN_FIRST + 5)
#define NPPN_FILEBEFOREOPEN     (NPPN_FIRST + 6)
#define NPPN_BUFFERACTIVATED    (NPPN_FIRST + 10)  // corrected from +9; official NPP source = 1010
#define NPPN_SHUTDOWN           (NPPN_FIRST + 11)

// Phase 2 additions — verified against official Notepad++ source
#define NPPM_GETCURRENTSCINTILLA        (NPPMSG + 4)
#define NPPM_GETCURRENTBUFFERID         (NPPMSG + 60)
#define NPPM_GETFULLPATHFROMBUFFERID    (NPPMSG + 58)   // WPARAM=bufferID, LPARAM=wchar_t[MAX_PATH]
#define NPPM_ISDARKMODEENABLED          (NPPMSG + 107)  // returns BOOL
#define NPPM_ADDSCNMODIFIEDFLAGS        (NPPMSG + 117)  // register SCN_MODIFIED interest flags
#define NPPN_DARKMODECHANGED            (NPPN_FIRST + 27)
