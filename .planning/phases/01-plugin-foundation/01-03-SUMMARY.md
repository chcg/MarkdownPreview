---
phase: 01-plugin-foundation
plan: 03
subsystem: infra
tags: [webview2, notepad++, docking-panel, win32]

# Dependency graph
requires:
  - phase: 01-plugin-foundation/01-02
    provides: PreviewPanel class with docking registration and toggle
provides:
  - WebView2 initialization inside docking panel
  - Runtime detection for WebView2 availability
  - Fallback UI (SysLink) when WebView2 is missing
  - Welcome page via virtual host mapping
affects: [02-markdown-rendering]

# Tech tracking
tech-stack:
  added: [WebView2 SDK, WIL COM helpers]
  patterns: [lazy WebView2 init on first toggle, virtual host mapping for local assets]

key-files:
  created:
    - MarkdownPreview/assets/welcome.html
  modified:
    - MarkdownPreview/src/PreviewPanel.cpp
    - MarkdownPreview/src/PreviewPanel.h
    - MarkdownPreview/src/PluginDefinition.cpp
    - MarkdownPreview/src/Settings.cpp
    - MarkdownPreview/include/Docking.h
    - MarkdownPreview/MarkdownPreview.vcxproj

key-decisions:
  - "WebView2 user data stored in %LOCALAPPDATA%\\MarkdownPreview\\WebView2Data"
  - "Virtual host mapping: appassets.mdpreview for local asset loading"
  - "Settings saved immediately on toggle (not just shutdown) for crash resilience"
  - "Panel cleanup moved to NPPN_SHUTDOWN (not DLL_PROCESS_DETACH) to avoid stale WebView2 locks"

patterns-established:
  - "Lazy initialization: WebView2 only created when panel first opens"
  - "Immediate settings persistence on state change"

requirements-completed: [INFR-03]

# Metrics
duration: 45min
completed: 2026-04-08
---

# Plan 01-03: WebView2 Initialization Summary

**WebView2 lazy init with runtime detection, fallback SysLink UI, and welcome page via virtual host mapping**

## Performance

- **Duration:** ~45 min (including debugging cycles)
- **Tasks:** 3
- **Files modified:** 8

## Accomplishments
- WebView2 initializes lazily inside docking panel on first toggle
- Runtime detection via `GetAvailableCoreWebView2BrowserVersionString` — shows SysLink with download URL if missing
- Welcome page (`welcome.html`) served via `SetVirtualHostNameToFolderMapping` at `appassets.mdpreview`
- Settings persistence fully working — panel state survives NPP restarts
- Both x86 and x64 Release builds verified

## Task Commits

1. **Task 1: WebView2 lazy detection and initialization** - `b05cc60` (feat)
2. **Task 2: Welcome page and build verification** - `17cca63` (feat)
3. **Bug fix: Settings persistence and panel lifecycle** - `775740c` (fix)
4. **Bug fix: Correct docking constants** - `e4a1760` (fix)

## Files Created/Modified
- `MarkdownPreview/assets/welcome.html` - Welcome page shown when no .md file is open
- `MarkdownPreview/src/PreviewPanel.cpp` - WebView2 init, runtime detection, fallback UI, resize handling
- `MarkdownPreview/src/PreviewPanel.h` - WebView2 member variables and methods
- `MarkdownPreview/src/PluginDefinition.cpp` - Panel lifecycle wiring, immediate settings save
- `MarkdownPreview/src/Settings.cpp` - CreateDirectoryW for config dir, defensive save
- `MarkdownPreview/include/Docking.h` - Corrected DWS_DF_CONT_* constants to match NPP values
- `MarkdownPreview/MarkdownPreview.vcxproj` - Added new source files and asset copy post-build

## Decisions Made
- WebView2 user data path: `%LOCALAPPDATA%\MarkdownPreview\WebView2Data`
- Settings save on every toggle (crash-resilient) plus on NPPN_SHUTDOWN
- Panel destroy moved from DLL_PROCESS_DETACH to NPPN_SHUTDOWN to avoid corrupting WebView2 state
- Do NOT call DestroyWindow on panel HWND — NPP's docking manager owns it

## Deviations from Plan

### Auto-fixed Issues

**1. [Bug] Settings never persisted to disk**
- **Found during:** User verification
- **Issue:** ofstream silently failed because config directory didn't exist; settings only saved at shutdown
- **Fix:** Added CreateDirectoryW before write; save immediately on toggle
- **Committed in:** `775740c`

**2. [Bug] Panel broken after NPP restart**
- **Found during:** User verification
- **Issue:** pluginCleanUp (DLL_PROCESS_DETACH) destroyed WebView2 and panel HWND after NPP tore down UI, corrupting WebView2 user data
- **Fix:** Moved cleanup to onNppShutdown; stopped destroying NPP-owned panel HWND
- **Committed in:** `775740c`

**3. [Bug] Wrong docking position constants**
- **Found during:** User verification (panel docked at top instead of right)
- **Issue:** DWS_DF_CONT_* used values 1,2,4,8 instead of NPP's 0,1,2,3 container indices
- **Fix:** Corrected to match NPP source: CONT_LEFT=0, CONT_RIGHT=1, CONT_TOP=2, CONT_BOTTOM=3
- **Committed in:** `e4a1760`

---

**Total deviations:** 3 bug fixes found during user verification
**Impact on plan:** All fixes required for correct functionality. No scope creep.

## Issues Encountered
- None beyond the deviations above

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- WebView2 panel fully operational with welcome page
- Ready for Phase 2: markdown-it integration, content rendering pipeline
- Virtual host mapping (`appassets.mdpreview`) established for loading JS/CSS assets

---
*Phase: 01-plugin-foundation*
*Completed: 2026-04-08*
