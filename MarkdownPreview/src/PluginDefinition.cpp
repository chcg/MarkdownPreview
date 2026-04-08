// This file is part of MarkdownPreview plugin for Notepad++

#include "PluginDefinition.h"
#include <cstring>
#include <string>
#include <objbase.h>

// Global plugin data
NppData nppData;
FuncItem funcItems[NB_FUNC];
PreviewPanel g_previewPanel;
Settings g_settings;

static HINSTANCE g_hInstance = nullptr;
static std::wstring g_configPath;

// Shortcut key: Ctrl+Shift+M (D-08)
static ShortcutKey toggleShortcut = { true, false, true, 'M' };

void pluginInit(HANDLE hModule) {
    g_hInstance = reinterpret_cast<HINSTANCE>(hModule);
    // Per Pitfall 2: CoInitializeEx required before WebView2, safe to call early
    // Duplicate calls return S_FALSE which is fine
    ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

void pluginCleanUp() {
    g_previewPanel.destroy();
}

void commandMenuInit() {
    // Menu item 0: Toggle Preview (Ctrl+Shift+M) per D-09
    wcscpy_s(funcItems[0]._itemName, menuItemSize, L"Toggle Preview");
    funcItems[0]._pFunc = togglePreview;
    funcItems[0]._cmdID = 0;
    funcItems[0]._init2Check = false;
    funcItems[0]._pShKey = &toggleShortcut;
}

void commandMenuCleanUp() {
    // Shortcut cleanup will be implemented if needed
}

void onNppReady() {
    // Compute config path using NPPM_GETPLUGINSCONFIGDIR (D-14)
    wchar_t configDir[MAX_PATH] = {};
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, reinterpret_cast<LPARAM>(configDir));
    g_configPath = std::wstring(configDir) + L"\\MarkdownPreview.json";

    // Load persisted settings
    g_settings.load(g_configPath);

    // Initialize preview panel (lazy - window created on first toggle per D-06)
    g_previewPanel.init(g_hInstance, nppData._nppHandle);

    // Restore panel visibility state from previous session (D-04)
    if (g_settings.panelVisible) {
        g_previewPanel.toggle(funcItems[0]._cmdID);
    }
}

void onNppShutdown() {
    // Persist settings before Notepad++ exits
    g_settings.save(g_configPath);
}

void togglePreview() {
    g_previewPanel.toggle(funcItems[0]._cmdID);
    g_settings.panelVisible = g_previewPanel.isVisible();
}
