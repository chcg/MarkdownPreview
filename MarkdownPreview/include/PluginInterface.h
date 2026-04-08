// This file is part of MarkdownPreview plugin for Notepad++
// Based on Notepad++ plugin interface definitions

#pragma once

#include <windows.h>

// Notepad++ message base
#define NPPMSG (WM_USER + 1000)

const int menuItemSize = 64;

typedef void (*PFUNCPLUGINCMD)();

struct ShortcutKey {
    bool _isCtrl;
    bool _isAlt;
    bool _isShift;
    UCHAR _key;
};

struct FuncItem {
    wchar_t _itemName[menuItemSize];
    PFUNCPLUGINCMD _pFunc;
    int _cmdID;
    bool _init2Check;
    ShortcutKey* _pShKey;
};

struct NppData {
    HWND _nppHandle;
    HWND _scintillaMainHandle;
    HWND _scintillaSecondHandle;
};
