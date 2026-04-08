// This file is part of MarkdownPreview plugin for Notepad++
// Docking dialog constants and structures

#pragma once

#include <windows.h>

// Container indices (must match Notepad++ internal values)
#define CONT_LEFT    0
#define CONT_RIGHT   1
#define CONT_TOP     2
#define CONT_BOTTOM  3

// Docking position masks (container index in bits 28-31)
#define DWS_DF_CONT_LEFT    (CONT_LEFT << 28)
#define DWS_DF_CONT_RIGHT   (CONT_RIGHT << 28)
#define DWS_DF_CONT_TOP     (CONT_TOP << 28)
#define DWS_DF_CONT_BOTTOM  (CONT_BOTTOM << 28)

// Docking icon flags
#define DWS_ICONTAB         0x00000001
#define DWS_ICONBAR         0x00000002
#define DWS_ADDINFO         0x00000004

// Docking dialog data structure
struct tTbData {
    HWND hClient;               // Client window handle
    const wchar_t* pszName;     // Panel title
    int dlgID;                  // Index of funcItems[] for this dialog
    UINT uMask;                 // Docking position mask (DWS_DF_CONT_*)
    HICON hIconTab;             // Icon for tab (optional)
    const wchar_t* pszAddInfo;  // Additional info (optional)
    RECT rcFloat;               // Floating rectangle
    int iPrevCont;              // Previous container
    const wchar_t* pszModuleName; // Module file name
};
