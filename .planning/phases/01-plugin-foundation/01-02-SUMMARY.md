---
phase: 01-plugin-foundation
plan: 02
subsystem: infra
tags: [win32, docking-panel, settings, json, notepad++, c++]

# Dependency graph
requires:
  - phase: 01-plugin-foundation plan 01
    provides: VS2022 solution, plugin DLL skeleton with exports, NuGet packages
provides:
  - Dockable preview panel with toggle via Ctrl+Shift+M
  - PreviewPanel class managing Win32 host window and Notepad++ docking registration
  - Settings persistence to MarkdownPreview.json via nlohmann/json
  - Panel visibility state restored on Notepad++ restart
  - Menu checkmark reflecting panel visibility
affects: [01-03-PLAN]

# Tech tracking
tech-stack:
  added: []
  patterns: [Lazy docking panel registration on first toggle, JSON settings via NPPM_GETPLUGINSCONFIGDIR, CoInitializeEx in pluginInit for WebView2 readiness]

key-files:
  created:
    - MarkdownPreview/src/PreviewPanel.cpp
    - MarkdownPreview/src/PreviewPanel.h
    - MarkdownPreview/src/Settings.cpp
    - MarkdownPreview/src/Settings.h
  modified:
    - MarkdownPreview/src/PluginDefinition.cpp
    - MarkdownPreview/src/PluginDefinition.h
    - MarkdownPreview/src/PluginMain.cpp
    - MarkdownPreview/MarkdownPreview.vcxproj

key-decisions:
  - "onNppReady/onNppShutdown functions exported from PluginDefinition and called by PluginMain beNotified handler"
  - "CoInitializeEx called in pluginInit for WebView2 readiness (Pitfall 2 mitigation)"

patterns-established:
  - "Lazy panel registration: register docking dialog on first toggle, then DMMSHOW/DMMHIDE only"
  - "Settings pattern: load in NPPN_READY, save in NPPN_SHUTDOWN, config path via NPPM_GETPLUGINSCONFIGDIR"
  - "Global instances: g_previewPanel and g_settings as file-scope globals in PluginDefinition.cpp"

requirements-completed: [INFR-01]

# Metrics
duration: 3min
completed: 2026-04-08
---

# Phase 01 Plan 02: Dockable Preview Panel and Settings Summary

**Dockable preview panel toggled via Ctrl+Shift+M with right-side docking, menu checkmark, and JSON settings persistence across Notepad++ restarts**

## Performance

- **Duration:** 3 min
- **Started:** 2026-04-08T18:41:48Z
- **Completed:** 2026-04-08T18:44:40Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments
- PreviewPanel class with Win32 host window, docking registration via NPPM_DMMREGASDCKDLG, and toggle via DMMSHOW/DMMHIDE
- Settings struct with JSON load/save using nlohmann/json, with try/catch for malformed files (T-01-04 mitigation)
- Full wiring: togglePreview() replaces MessageBox stub, NPPN_READY loads settings and restores panel, NPPN_SHUTDOWN saves settings
- Both x86 and x64 Release builds verified passing with all new source files

## Task Commits

Each task was committed atomically:

1. **Task 1: Create PreviewPanel class with docking panel registration and toggle** - `786a1f7` (feat)
2. **Task 2: Implement Settings persistence and wire toggle into PluginDefinition** - `a377706` (feat)

## Files Created/Modified
- `MarkdownPreview/src/PreviewPanel.h` - PreviewPanel class declaration with init, destroy, toggle, isVisible, getHwnd
- `MarkdownPreview/src/PreviewPanel.cpp` - Docking panel creation (50% width, right dock), registration, show/hide toggle, menu checkmark update
- `MarkdownPreview/src/Settings.h` - Settings struct with panelVisible field and load/save methods
- `MarkdownPreview/src/Settings.cpp` - JSON settings persistence via nlohmann/json with error-resilient parsing
- `MarkdownPreview/src/PluginDefinition.h` - Added PreviewPanel/Settings includes, extern declarations, onNppReady/onNppShutdown
- `MarkdownPreview/src/PluginDefinition.cpp` - Replaced MessageBox stub with full panel toggle, settings load/save, CoInitializeEx
- `MarkdownPreview/src/PluginMain.cpp` - beNotified calls onNppReady and onNppShutdown
- `MarkdownPreview/MarkdownPreview.vcxproj` - Added PreviewPanel.cpp/h and Settings.cpp/h to build

## Decisions Made
- Added `onNppReady()` and `onNppShutdown()` as separate functions in PluginDefinition rather than putting logic directly in PluginMain.cpp beNotified handler. Keeps PluginMain.cpp thin (just dispatches) and PluginDefinition.cpp as the logic home.
- Called CoInitializeEx in pluginInit (DLL_PROCESS_ATTACH time) rather than deferring to first panel toggle. This is safe (duplicate calls return S_FALSE) and ensures COM STA is ready before any WebView2 code in Plan 03.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Docking panel infrastructure complete, ready for WebView2 initialization (Plan 03)
- PreviewPanel::wndProc has WM_SIZE stub ready for WebView2 controller resize
- CoInitializeEx already called, removing a potential blocker for WebView2 creation
- Settings infrastructure in place for adding more settings in future phases

## Self-Check: PASSED

All 8 files verified present. Both task commits (786a1f7, a377706) verified in git log.

---
*Phase: 01-plugin-foundation*
*Completed: 2026-04-08*
