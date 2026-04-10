// This file is part of MarkdownPreview plugin for Notepad++
// Minimal Scintilla header for plugin notification handling

#pragma once

#include <windows.h>

// Scintilla position type
#ifndef Sci_Position
typedef intptr_t Sci_Position;
#endif

// SCNotification structure - used by beNotified callback
struct SCNotification {
    NMHDR nmhdr;
    Sci_Position position;      // SCN_STYLENEEDED, SCN_MODIFIED, SCN_DWELLSTART, SCN_DWELLEND
    int ch;                     // SCN_CHARADDED, SCN_KEY
    int modifiers;              // SCN_KEY
    int modificationType;       // SCN_MODIFIED
    const char* text;           // SCN_MODIFIED, SCN_USERLISTSELECTION, SCN_AUTOCSELECTION
    Sci_Position length;        // SCN_MODIFIED
    Sci_Position linesAdded;    // SCN_MODIFIED
    int message;                // cyclic - cycled through notification queue
    uintptr_t wParam;           // cyclic
    intptr_t lParam;            // cyclic
    Sci_Position line;          // cyclic
    int foldLevelNow;           // cyclic
    int foldLevelPrev;          // cyclic
    int margin;                 // cyclic
    int listType;               // cyclic
    int x;                      // cyclic
    int y;                      // cyclic
    int token;                  // cyclic
    Sci_Position annotationLinesAdded; // cyclic
    int updated;                // cyclic
    int listCompletionMethod;   // cyclic
    int characterSource;        // cyclic
};

// Scintilla messages used by the plugin
#define SCI_GETLENGTH       2006
#define SCI_GETTEXT         2182
#define SCI_GETTEXTRANGE    2162
#define SCI_GETFIRSTVISIBLELINE 2152
#define SCI_LINESONSCREEN   2370
#define SCI_GETLINECOUNT    2154

// Phase 2 additions
#define SCI_GETCURRENTPOS       2008
#define SCI_LINEFROMPOSITION    2166
#define SCI_GETCODEPAGE         2137

// Phase 4 additions (click-to-editor navigation)
#define SCI_ENSUREVISIBLE       2232
#define SCI_GOTOLINE            2024
#define SCI_SCROLLCARET         2169
// Notification codes (nmhdr.code in SCNotification) — distinct namespace from SCI_ message IDs
// Note: SCN_MODIFIED (2008) and SCI_GETCURRENTPOS share the same numeric value by design.
// SCN_ codes appear in nmhdr.code; SCI_ codes are SendMessage wParam. They never conflict.
#define SCN_MODIFIED            2008
#define SCN_UPDATEUI            2013
#define SC_MOD_INSERTTEXT       0x0001
#define SC_MOD_DELETETEXT       0x0002
#define SC_PERFORMED_UNDO       0x0020
#define SC_PERFORMED_REDO       0x0040

// SCN_UPDATEUI updated field bitmask values (Plan 02-03 scroll sync)
#define SC_UPDATE_CONTENT      0x01
#define SC_UPDATE_SELECTION    0x02
#define SC_UPDATE_V_SCROLL     0x04
#define SC_UPDATE_H_SCROLL     0x08
