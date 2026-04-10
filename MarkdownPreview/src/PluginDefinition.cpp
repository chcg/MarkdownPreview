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

// Cached path of the currently active .md file (or empty if not a .md file).
// Updated by onBufferActivated(). Read by onScnModified() to avoid calling back
// into NPP's main window (SendMessage NPPM_GETFULLCURRENTPATH) from within a
// Scintilla notification handler — that call is the confirmed crash trigger:
// NPP's NPPM_GETFULLCURRENTPATH handler dereferences getCurrentBuffer() without
// a null guard, and getCurrentBuffer() can return nullptr during SC_MOD_BEFOREINSERT
// notifications when the document state is transient.
static std::wstring g_currentMdFilePath;

// Shortcut key: Ctrl+Shift+M for Toggle Preview (D-08)
static ShortcutKey toggleShortcut = { true, false, true, 'M' };

// Shortcut key: Ctrl+Shift+E for Export as HTML
// Verified no conflict: Ctrl+Shift+E is not a default Notepad++ shortcut
static ShortcutKey exportShortcut = { true, false, true, 'E' };

// Shortcut key: Ctrl+Shift+P for Export as PDF (Phase 3)
// Ctrl+Shift+P is not a default Notepad++ shortcut (A6: assumed free — verify during testing)
static ShortcutKey pdfExportShortcut = { true, false, true, 'P' };

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

    // Menu item 1: Export as HTML (Ctrl+Shift+E) per EXPT-01
    wcscpy_s(funcItems[1]._itemName, menuItemSize, L"Export as HTML");
    funcItems[1]._pFunc = exportMarkdown;
    funcItems[1]._cmdID = 0;
    funcItems[1]._init2Check = false;
    funcItems[1]._pShKey = &exportShortcut;

    // Menu item 2: Export as PDF (Ctrl+Shift+P) per EXPT-02
    wcscpy_s(funcItems[2]._itemName, menuItemSize, L"Export as PDF");
    funcItems[2]._pFunc = exportMarkdownAsPdf;
    funcItems[2]._cmdID = 0;
    funcItems[2]._init2Check = false;
    funcItems[2]._pShKey = &pdfExportShortcut;
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

    // Phase 3: Pass config path to PreviewPanel for zoom settings persistence (D-05)
    g_previewPanel.setConfigPath(g_configPath);

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

    // Cache the active .md path so onScnModified() can check it without calling
    // SendMessage(NPPM_GETFULLCURRENTPATH) from within a Scintilla notification
    // handler — that call crashes NPP when getCurrentBuffer() returns nullptr
    // during SC_MOD_BEFOREINSERT notifications.
    g_currentMdFilePath = isMd ? filePath : std::wstring();

    if (isMd) {
        // D-01: auto-open panel whenever a .md file is activated, even if previously closed
        if (!g_previewPanel.isVisible()) {
            g_previewPanel.toggle(funcItems[0]._cmdID);
        }
        // Update image virtual host BEFORE sending render message (Pitfall 3: timing)
        // Maps file.mdpreview to the new file's parent directory before images are requested
        g_previewPanel.updateFileVirtualHost(filePath);
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
    // Use the cached path set by onBufferActivated() — do NOT call SendMessage back into NPP
    // from within a Scintilla notification handler. NPPM_GETFULLCURRENTPATH causes NPP to call
    // _pEditView->getCurrentBuffer()->getFullPathName() without a null guard; getCurrentBuffer()
    // returns nullptr during SC_MOD_BEFOREINSERT (before the edit is committed), crashing NPP.
    if (!g_currentMdFilePath.empty()) {
        g_previewPanel.scheduleRender();
    }
}

// Phase 2 Plan 04: Trigger HTML export — check active .md file, call triggerExport()
void exportMarkdown() {
    // Only trigger export if panel is visible and a .md file is active
    if (!g_previewPanel.isVisible()) return;
    wchar_t path[MAX_PATH] = {};
    ::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH,
        reinterpret_cast<LPARAM>(path));
    std::wstring filePath(path);
    bool isMd = filePath.size() >= 3 &&
        (_wcsicmp(filePath.c_str() + filePath.size() - 3, L".md") == 0);
    if (!isMd) return;
    g_previewPanel.triggerExport();
}

// Phase 3: Trigger PDF export — check active .md file, call triggerPdfExport()
// Mirrors exportMarkdown() pattern exactly (EXPT-02)
void exportMarkdownAsPdf() {
    // Only trigger if panel is visible and a .md file is active
    if (!g_previewPanel.isVisible()) return;
    wchar_t path[MAX_PATH] = {};
    ::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH,
        reinterpret_cast<LPARAM>(path));
    std::wstring filePath(path);
    bool isMd = filePath.size() >= 3 &&
        (_wcsicmp(filePath.c_str() + filePath.size() - 3, L".md") == 0);
    if (!isMd) return;
    g_previewPanel.triggerPdfExport();
}

// Phase 2 Plan 03: Called on SCN_UPDATEUI — scroll sync (SCRL-01)
void onScnUpdateUi(SCNotification* notification) {
    // Only sync on caret/selection changes — not on scroll events
    // SC_UPDATE_SELECTION = 0x02; SC_UPDATE_CONTENT = 0x01
    if (!(notification->updated & (SC_UPDATE_SELECTION | SC_UPDATE_CONTENT))) return;

    // Get active Scintilla handle
    int sciId = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0,
        reinterpret_cast<LPARAM>(&sciId));
    HWND hSci = (sciId == 0) ? nppData._scintillaMainHandle
                              : nppData._scintillaSecondHandle;

    // Get byte position of caret, then convert to 0-indexed line number
    Sci_Position caretPos = static_cast<Sci_Position>(
        ::SendMessage(hSci, SCI_GETCURRENTPOS, 0, 0));
    int caretLine = static_cast<int>(
        ::SendMessage(hSci, SCI_LINEFROMPOSITION,
            static_cast<WPARAM>(caretPos), 0));

    // Only send scroll if a .md file is active and panel is visible
    if (!g_previewPanel.isVisible()) return;
    wchar_t path[MAX_PATH] = {};
    ::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH,
        reinterpret_cast<LPARAM>(path));
    std::wstring filePath(path);
    bool isMd = filePath.size() >= 3 &&
        (_wcsicmp(filePath.c_str() + filePath.size() - 3, L".md") == 0);
    if (isMd) {
        g_previewPanel.scrollToLine(caretLine);
    }
}
