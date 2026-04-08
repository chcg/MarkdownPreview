---
phase: 01-plugin-foundation
verified: 2026-04-08T20:30:00Z
status: human_needed
score: 4/4
overrides_applied: 0
human_verification:
  - test: "Load plugin DLL in both 32-bit and 64-bit Notepad++ and confirm no error"
    expected: "Plugin appears in Plugins menu as MarkdownPreview with Toggle Preview submenu item"
    why_human: "Requires running Notepad++ with the DLL installed -- cannot verify plugin load behavior programmatically"
  - test: "Press Ctrl+Shift+M to toggle docking panel visible and hidden"
    expected: "Panel appears docked on right side, menu checkmark toggles, panel hides on second press"
    why_human: "Requires live Notepad++ interaction to verify docking behavior and keyboard shortcut"
  - test: "Verify WebView2 renders welcome page inside panel"
    expected: "Welcome page shows MarkdownPreview heading, v0.1.0, and Open a .md file to see the preview"
    why_human: "WebView2 async initialization inside docking panel requires runtime visual verification"
  - test: "Close and reopen Notepad++ to verify panel state persistence"
    expected: "Panel auto-appears if it was visible before close; MarkdownPreview.json exists in config dir"
    why_human: "Requires full Notepad++ restart cycle to verify settings round-trip"
  - test: "Drag panel to different dock position (e.g. bottom)"
    expected: "Panel redocks successfully (standard Notepad++ docking behavior)"
    why_human: "Drag-and-dock is a visual interaction that cannot be verified programmatically"
  - test: "Resize panel by dragging splitter"
    expected: "WebView2 content resizes correctly to fill the panel"
    why_human: "Visual resize behavior requires runtime verification"
---

# Phase 1: Plugin Foundation Verification Report

**Phase Goal:** A working Notepad++ plugin that loads, creates a dockable panel, initializes WebView2, and can be toggled on/off
**Verified:** 2026-04-08T20:30:00Z
**Status:** human_needed
**Re-verification:** No -- initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Plugin DLL loads without error in both 32-bit and 64-bit Notepad++ | VERIFIED | DLLs exist: bin/x64/Release/MarkdownPreview.dll (160KB), bin/x86/Release/MarkdownPreview.dll (144KB). Six dllexport functions confirmed in PluginMain.cpp (setInfo, getName, getFuncsArray, isUnicode, beNotified, messageProc). WebView2Loader.dll bundled in both output dirs. Solution has all 4 configs: Debug/Release x86/x64. Needs human confirmation of actual NPP loading. |
| 2 | User can toggle a dockable preview panel via menu item and keyboard shortcut | VERIFIED | PluginDefinition.cpp: toggleShortcut = { true, false, true, 'M' } (Ctrl+Shift+M). funcItems[0] registered as "Toggle Preview". togglePreview() calls g_previewPanel.toggle(). PreviewPanel::toggle() uses NPPM_DMMSHOW/DMMHIDE and NPPM_SETMENUITEMCHECK. DWS_DF_CONT_RIGHT used for right-side docking. Needs human confirmation in live NPP. |
| 3 | WebView2 initializes inside the dockable panel and renders a placeholder page | VERIFIED | PreviewPanel.cpp: CreateCoreWebView2EnvironmentWithOptions with async Callback pattern. SetVirtualHostNameToFolderMapping maps "appassets.mdpreview" to assets folder. Navigate to "https://appassets.mdpreview/welcome.html". welcome.html exists (55 lines) with correct content: "MarkdownPreview" h1, "v0.1.0" version, "Open a .md file to see the preview" hint. Post-build xcopy copies assets/ to output. Needs human confirmation of rendering. |
| 4 | When WebView2 runtime is missing, a clear message is shown instead of a crash | VERIFIED | PreviewPanel.cpp: checkWebView2Available() calls GetAvailableCoreWebView2BrowserVersionString. If false, showWebView2MissingFallback() creates WC_LINK (SysLink) with "Download WebView2 Runtime" text and URL. WM_NOTIFY handler opens URL via ShellExecuteW. Needs human confirmation with simulated missing runtime. |

**Score:** 4/4 truths verified (code-level). All require human verification in live Notepad++ environment.

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `MarkdownPreview.sln` | VS2022 solution with x86/x64 configs | VERIFIED | Exists, contains all 4 config combos (Debug/Release x86/x64) |
| `MarkdownPreview/MarkdownPreview.vcxproj` | Project with NuGet refs, dual platform | VERIFIED | ConfigurationType=DynamicLibrary, PlatformToolset=v145, WebView2 SDK include/lib paths, post-build copy of WebView2Loader.dll and assets |
| `MarkdownPreview/packages.config` | NuGet package references | VERIFIED | Exists |
| `MarkdownPreview/src/PluginMain.cpp` | DLL entry with 6 exports | VERIFIED | 61 lines, 6 dllexport functions, delegates to PluginDefinition |
| `MarkdownPreview/src/PluginDefinition.h` | Plugin class declaration | VERIFIED | 30 lines, PLUGIN_NAME=L"MarkdownPreview", NB_FUNC=1, extern declarations |
| `MarkdownPreview/src/PluginDefinition.cpp` | Plugin init, toggle, settings wiring | VERIFIED | 86 lines, CoInitializeEx, settings load/save, panel toggle, no MessageBox stub |
| `MarkdownPreview/src/PreviewPanel.cpp` | Docking panel, WebView2, fallback | VERIFIED | 267 lines (exceeds min_lines: 150). Contains docking registration, WebView2 init, SysLink fallback, WM_SIZE resize |
| `MarkdownPreview/src/PreviewPanel.h` | PreviewPanel class declaration | VERIFIED | 46 lines, exports PreviewPanel class with WebView2 members |
| `MarkdownPreview/src/Settings.cpp` | JSON settings load/save | VERIFIED | 39 lines, nlohmann::json parse/dump, try/catch error handling, CreateDirectoryW for config dir |
| `MarkdownPreview/src/Settings.h` | Settings struct declaration | VERIFIED | 13 lines, panelVisible field, load/save methods |
| `MarkdownPreview/assets/welcome.html` | Welcome page with name, version, hint | VERIFIED | 55 lines, contains "MarkdownPreview", "v0.1.0", "Open a .md file to see the preview", correct typography/colors per UI-SPEC |
| `MarkdownPreview/include/PluginInterface.h` | NppData, FuncItem, ShortcutKey structs | VERIFIED | Exists |
| `MarkdownPreview/include/Notepad_plus_msgs.h` | NPPM_DMM* message constants | VERIFIED | Exists |
| `MarkdownPreview/include/Docking.h` | tTbData struct, DWS_DF_CONT_RIGHT | VERIFIED | Exists, DWS_DF_CONT_RIGHT defined |
| `MarkdownPreview/include/Scintilla.h` | SCNotification struct | VERIFIED | Exists |
| `MarkdownPreview/include/nlohmann/json.hpp` | JSON single-header library | VERIFIED | Exists |
| `.gitignore` | Build output exclusions | VERIFIED | Exists |
| `bin/x64/Release/MarkdownPreview.dll` | Built x64 DLL | VERIFIED | 160KB |
| `bin/x86/Release/MarkdownPreview.dll` | Built x86 DLL | VERIFIED | 144KB |
| `bin/x64/Release/WebView2Loader.dll` | Bundled WebView2 loader x64 | VERIFIED | Exists |
| `bin/x86/Release/WebView2Loader.dll` | Bundled WebView2 loader x86 | VERIFIED | Exists |
| `bin/x64/Release/assets/welcome.html` | Assets copied to output | VERIFIED | Exists |
| `bin/x86/Release/assets/welcome.html` | Assets copied to output | VERIFIED | Exists |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| PluginMain.cpp | PluginDefinition.h | #include and delegation | WIRED | Line 4: `#include "PluginDefinition.h"`. All 6 exports delegate to PluginDefinition functions. |
| PluginDefinition.cpp | PreviewPanel.h | togglePreview() calls panel | WIRED | Line 78: `g_previewPanel.toggle(funcItems[0]._cmdID)`. Global `g_previewPanel` declared, init called in onNppReady. |
| PluginDefinition.cpp | Settings.h | pluginInit loads, shutdown saves | WIRED | Line 56: `g_settings.load(g_configPath)`, Line 69: `g_settings.save(g_configPath)`. Immediate save on toggle at line 84. |
| PreviewPanel.cpp | NPPM_DMMREGASDCKDLG | SendMessage to register | WIRED | Line 107: `::SendMessage(m_nppHandle, NPPM_DMMREGASDCKDLG, 0, ...)` |
| PreviewPanel.cpp | GetAvailableCoreWebView2BrowserVersionString | Lazy runtime detection | WIRED | Line 154: called in checkWebView2Available() |
| PreviewPanel.cpp | CreateCoreWebView2EnvironmentWithOptions | Async WebView2 init | WIRED | Line 166: full async callback chain with controller and webview creation |
| PreviewPanel.cpp | SetVirtualHostNameToFolderMapping | Virtual host for assets | WIRED | Line 197: maps "appassets.mdpreview" to assets folder |
| PreviewPanel.cpp | welcome.html | WebView2 navigates to welcome | WIRED | Line 204: `Navigate(L"https://appassets.mdpreview/welcome.html")` |
| MarkdownPreview.vcxproj | PreviewPanel.cpp, Settings.cpp | ClCompile includes | WIRED | Lines 197-198: both files in ClCompile ItemGroup |

### Data-Flow Trace (Level 4)

Not applicable -- this phase produces a native C++ plugin with Win32/WebView2 rendering, not a data-driven web component. Data flow is WebView2 navigating to a static welcome.html, which is verified via the key link above.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| DLL exports correct functions | Verified via grep for dllexport in PluginMain.cpp | 6 matches: setInfo, getName, getFuncsArray, isUnicode, beNotified, messageProc | PASS |
| No stub MessageBox in togglePreview | grep MessageBox across src/ | Zero matches | PASS |
| No TODO/FIXME/placeholder comments | grep -i across src/ | Zero matches | PASS |
| Both platform DLLs built | ls bin/{x64,x86}/Release/MarkdownPreview.dll | Both exist, 160KB and 144KB respectively | PASS |
| Assets copied to output | ls bin/{x64,x86}/Release/assets/welcome.html | Both exist | PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| INFR-01 | 01-02-PLAN | Toggle show/hide preview panel via menu item and keyboard shortcut | SATISFIED | togglePreview() wired to Ctrl+Shift+M, NPPM_DMMSHOW/DMMHIDE, NPPM_SETMENUITEMCHECK for checkmark |
| INFR-02 | 01-01-PLAN | Plugin loads correctly in both 32-bit and 64-bit Notepad++ | SATISFIED | x86 and x64 DLLs built, 6 exports, PlatformToolset v145, all configs present |
| INFR-03 | 01-03-PLAN | Graceful handling when WebView2 runtime is not installed | SATISFIED | GetAvailableCoreWebView2BrowserVersionString check, SysLink fallback with download URL, no crash path |

Note: REQUIREMENTS.md still shows INFR-03 as "Pending" (checkbox unchecked) -- this is a documentation tracking inconsistency but does not affect the actual implementation status.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| (none) | - | - | - | No TODO/FIXME/placeholder/stub patterns found in any source file |

### Human Verification Required

### 1. Plugin Loading in Notepad++

**Test:** Copy bin/x64/Release/ contents (MarkdownPreview.dll, WebView2Loader.dll, assets/) to `C:\Program Files\Notepad++\plugins\MarkdownPreview\`. Restart Notepad++.
**Expected:** Plugins menu shows "MarkdownPreview" > "Toggle Preview" with Ctrl+Shift+M shortcut.
**Why human:** Requires running Notepad++ with the DLL installed to verify plugin discovery, loading, and menu registration.

### 2. Toggle Panel via Keyboard Shortcut

**Test:** Press Ctrl+Shift+M in Notepad++.
**Expected:** Docking panel appears on the right side. Menu item shows checkmark. Press again to hide; checkmark disappears.
**Why human:** Keyboard shortcut handling, docking panel visual behavior, and checkmark state are runtime behaviors.

### 3. WebView2 Welcome Page Rendering

**Test:** With panel visible, observe the content.
**Expected:** Welcome page shows "MarkdownPreview" heading, "v0.1.0" version, "Open a .md file to see the preview" hint, centered vertically and horizontally.
**Why human:** WebView2 async initialization and HTML rendering inside a docking panel requires visual confirmation.

### 4. Settings Persistence Across Restarts

**Test:** Toggle panel visible, close Notepad++, reopen.
**Expected:** Panel auto-appears. Check `%AppData%\Notepad++\plugins\config\MarkdownPreview.json` exists with `"panelVisible": true`.
**Why human:** Full application restart cycle needed to verify settings round-trip.

### 5. Panel Redocking

**Test:** Drag the panel to a different dock position (e.g., bottom).
**Expected:** Panel successfully redocks.
**Why human:** Drag-and-dock is a visual interaction.

### 6. Panel Resize

**Test:** Drag the splitter to resize the panel.
**Expected:** WebView2 content resizes to fill the panel without clipping or blank areas.
**Why human:** Resize behavior with WebView2 requires runtime visual verification.

### Gaps Summary

No code-level gaps found. All artifacts exist, are substantive, and are properly wired. All six required DLL exports are present. WebView2 initialization follows the correct async pattern with lazy runtime detection. Settings persistence uses nlohmann/json with error handling. The welcome page matches the UI-SPEC.

The phase requires human verification because the entire value proposition -- a plugin that loads in Notepad++, creates a docking panel, and renders content via WebView2 -- can only be confirmed by running the plugin in a live Notepad++ instance. The code analysis strongly indicates correct implementation, and the 01-03-SUMMARY.md documents that user verification was performed during development (with 3 bugs found and fixed).

---

_Verified: 2026-04-08T20:30:00Z_
_Verifier: Claude (gsd-verifier)_
