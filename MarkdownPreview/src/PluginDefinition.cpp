// This file is part of MarkdownPreview plugin for Notepad++

#include "PluginDefinition.h"
#include "Scintilla.h"
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
    // NOTE: Do NOT destroy the panel here.
    // pluginCleanUp is called from DLL_PROCESS_DETACH, which runs after
    // Notepad++ has already torn down its UI. Calling DestroyWindow or
    // WebView2 Close at this point is unsafe and can corrupt WebView2
    // user data (stale lock files), causing failures on next restart.
    // Panel cleanup is done in onNppShutdown() instead.
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

    // Register for SCN_MODIFIED forwarding (defensive — fails silently on older NPP)
    ::SendMessage(nppData._nppHandle, NPPM_ADDSCNMODIFIEDFLAGS,
        SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT, 0);

    // Set initial theme state (THME-03)
    bool isDark = (BOOL)::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0) != FALSE;
    g_previewPanel.setTheme(isDark);
}

void onNppShutdown() {
    // Persist settings before Notepad++ exits
    g_settings.save(g_configPath);

    // Clean up panel resources while NPP is still alive.
    // This is safer than pluginCleanUp (DLL_PROCESS_DETACH) because
    // window handles and COM objects are still valid here.
    g_previewPanel.destroy();
}

void togglePreview() {
    g_previewPanel.toggle(funcItems[0]._cmdID);
    g_settings.panelVisible = g_previewPanel.isVisible();

    // Save immediately so state persists even if NPP crashes or
    // NPPN_SHUTDOWN doesn't fire cleanly
    if (!g_configPath.empty()) {
        g_settings.save(g_configPath);
    }
}

// Phase 2: Called when a buffer is activated (file switched or opened)
void onBufferActivated(UINT_PTR bufferId) {
    wchar_t path[MAX_PATH] = {};
    ::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID,
        static_cast<WPARAM>(bufferId), reinterpret_cast<LPARAM>(path));

    std::wstring filePath(path);
    // Check .md extension (case-insensitive)
    bool isMd = filePath.size() >= 3 &&
        (_wcsicmp(filePath.c_str() + filePath.size() - 3, L".md") == 0);

    if (isMd) {
        // D-01: auto-open panel whenever a .md file is activated, even if previously closed
        if (!g_previewPanel.isVisible()) {
            g_previewPanel.toggle(funcItems[0]._cmdID);
        }
        g_previewPanel.renderMarkdown(filePath);
    } else {
        // Non-.md file: navigate preview back to idle welcome page
        if (g_previewPanel.isVisible()) {
            g_previewPanel.showIdle();
        }
    }
}

// Phase 2: Called when Notepad++ dark/light mode changes (THME-03)
void onDarkModeChanged() {
    bool isDark = (BOOL)::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0) != FALSE;
    g_previewPanel.setTheme(isDark);
}

// Phase 2: Called on SCN_MODIFIED — trigger debounced re-render (REND-02)
void onScnModified(SCNotification* notification) {
    // Only react to text insertions and deletions (not fold/attribute changes)
    if (!(notification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))) return;
    // Only trigger if a .md file is currently active
    wchar_t path[MAX_PATH] = {};
    ::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, reinterpret_cast<LPARAM>(path));
    std::wstring filePath(path);
    bool isMd = filePath.size() >= 3 &&
        (_wcsicmp(filePath.c_str() + filePath.size() - 3, L".md") == 0);
    if (isMd) {
        g_previewPanel.scheduleRender();
    }
}

// Phase 2: Called on SCN_UPDATEUI — placeholder for scroll sync (Plan 02-03)
void onScnUpdateUi(SCNotification* notification) {
    // Scroll sync implementation deferred to Plan 02-03
    // Plan 02-03 will add scrollToLine call here using SC_UPDATE_SELECTION | SC_UPDATE_CONTENT
    (void)notification;
}
