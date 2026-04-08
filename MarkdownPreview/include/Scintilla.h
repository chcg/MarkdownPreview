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
