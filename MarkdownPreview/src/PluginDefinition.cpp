// This file is part of MarkdownPreview plugin for Notepad++

#include "PluginDefinition.h"
#include <cstring>

// Global plugin data
NppData nppData;
FuncItem funcItems[NB_FUNC];
HINSTANCE hInstance = nullptr;

// Shortcut key: Ctrl+Shift+M
static ShortcutKey toggleShortcut = { true, false, true, 'M' };

void pluginInit(HANDLE hModule) {
    hInstance = reinterpret_cast<HINSTANCE>(hModule);
}

void pluginCleanUp() {
    // Cleanup will be implemented in later phases
}

void commandMenuInit() {
    // Menu item 0: Toggle Preview (Ctrl+Shift+M)
    wcscpy_s(funcItems[0]._itemName, menuItemSize, L"Toggle Preview");
    funcItems[0]._pFunc = togglePreview;
    funcItems[0]._cmdID = 0;
    funcItems[0]._init2Check = false;
    funcItems[0]._pShKey = &toggleShortcut;
}

void commandMenuCleanUp() {
    // Shortcut cleanup will be implemented if needed
}

void togglePreview() {
    // Stub: will be replaced with docking panel toggle in Plan 02
    MessageBox(nppData._nppHandle,
        L"MarkdownPreview toggle - panel will be implemented in the next phase.",
        L"MarkdownPreview",
        MB_OK | MB_ICONINFORMATION);
}
