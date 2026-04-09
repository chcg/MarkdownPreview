---
phase: 02-core-preview
plan: "01"
subsystem: plugin-core
tags: [cpp, webview2, notepad-plus-plus, scintilla, nlohmann-json, win32-timer, debounce]

# Dependency graph
requires:
  - phase: 01-plugin-foundation
    provides: PreviewPanel WebView2 host, initWebView2 with virtual host mapping, toggle/show/hide panel infrastructure

provides:
  - NPPN_BUFFERACTIVATED handler (corrected to 1010) — auto-opens panel for .md files
  - SCN_MODIFIED debounced render pipeline (300ms SetTimer/KillTimer)
  - SCN_UPDATEUI placeholder for Plan 02-03 scroll sync
  - NPPN_DARKMODECHANGED + NPPM_ISDARKMODEENABLED theme detection and initial state
  - renderMarkdown() — Scintilla text retrieval via NPPM_GETCURRENTSCINTILLA + SCI_GETTEXT, nlohmann JSON encode, PostWebMessageAsJson
  - setTheme() — posts {type:theme, dark:bool} JSON to WebView2
  - showIdle() — navigates preview back to welcome.html on non-.md activation
  - add_WebMessageReceived stub — JS->C++ message channel wired, exportReady stub for Plan 02-04
  - All 6 missing Notepad++ message constants added to Notepad_plus_msgs.h
  - Phase 2 Scintilla notification codes added to Scintilla.h

affects:
  - 02-02-PLAN (JS preview.html receives render/theme messages from this pipeline)
  - 02-03-PLAN (onScnUpdateUi placeholder wired here; scroll sync adds implementation)
  - 02-04-PLAN (add_WebMessageReceived stub + handleJsMessage exportReady stub wired here)

# Tech tracking
tech-stack:
  added:
    - nlohmann/json (already in project via vcpkg) — used for C++->JS message construction
  patterns:
    - nlohmann/json used exclusively for all C++->JS PostWebMessageAsJson payloads (T-02-01 mitigation — no string concatenation)
    - Win32 SetTimer/KillTimer debounce pattern (300ms, one-shot via KillTimer in WM_TIMER handler + m_renderPending guard)
    - NppData accessed in PreviewPanel.cpp via extern declaration + PluginInterface.h include (avoids circular dependency with PluginDefinition.h)
    - beNotified switch dispatch pattern for both NPP notifications (NPPN_*) and Scintilla notifications (SCN_*)

key-files:
  created: []
  modified:
    - MarkdownPreview/include/Notepad_plus_msgs.h
    - MarkdownPreview/include/Scintilla.h
    - MarkdownPreview/src/PreviewPanel.h
    - MarkdownPreview/src/PreviewPanel.cpp
    - MarkdownPreview/src/PluginMain.cpp
    - MarkdownPreview/src/PluginDefinition.h
    - MarkdownPreview/src/PluginDefinition.cpp

key-decisions:
  - "Include PluginInterface.h directly in PreviewPanel.cpp (not PluginDefinition.h) to avoid circular dependency; declare extern NppData nppData explicitly"
  - "NPPN_BUFFERACTIVATED corrected from (NPPN_FIRST+9) to (NPPN_FIRST+10) per official Notepad++ source"
  - "SCN_MODIFIED (2008) and SCI_GETCURRENTPOS share value 2008 by design — distinct namespaces (nmhdr.code vs SendMessage wParam)"
  - "Add Scintilla.h include to PluginDefinition.h so SCNotification* parameter in onScnModified/onScnUpdateUi declarations resolves"
  - "Packages directory linked via Windows junction point in worktree (packages dir not present in worktree, main repo packages reused)"

patterns-established:
  - "Pattern: All C++->JS JSON messages built with nlohmann::json, never string concatenation (threat model T-02-01)"
  - "Pattern: debounce via SetTimer(HWND, ID, 300ms) + KillTimer in WM_TIMER + m_renderPending bool guard prevents double-render"
  - "Pattern: handleJsMessage wraps nlohmann::json::parse in try/catch, ignores malformed silently (threat model T-02-04)"

requirements-completed: [REND-01, REND-02, THME-03]

# Metrics
duration: 35min
completed: 2026-04-09
---

# Phase 02 Plan 01: C++ Notification Pipeline and WebView2 Messaging Summary

**NPPN_BUFFERACTIVATED/SCN_MODIFIED/NPPN_DARKMODECHANGED notification pipeline wired to WebView2 via nlohmann JSON PostWebMessageAsJson with 300ms SetTimer/KillTimer debounce**

## Performance

- **Duration:** ~35 min
- **Started:** 2026-04-09T11:00:00Z
- **Completed:** 2026-04-09T11:35:00Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Corrected NPPN_BUFFERACTIVATED value to 1010 and added all 6 missing Notepad++ message constants; added SCN_MODIFIED, SCN_UPDATEUI and modifier flags to Scintilla.h
- Expanded PreviewPanel with full render/theme/idle/debounce pipeline — renderMarkdown() retrieves UTF-8 text from active Scintilla view, converts safely, builds JSON with nlohmann, posts to WebView2; 300ms SetTimer/KillTimer debounce with m_renderPending guard
- Wired add_WebMessageReceived stub for JS->C++ channel and onBufferActivated/onDarkModeChanged/onScnModified/onScnUpdateUi handlers in PluginDefinition.cpp; both x64 and x86 Release builds succeed with 0 errors

## Task Commits

Each task was committed atomically:

1. **Task 1: Header constants + C++ notification handlers** - `d4475c1` (feat) — includes Task 2 (both implemented together in one coherent commit)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `MarkdownPreview/include/Notepad_plus_msgs.h` - Fixed NPPN_BUFFERACTIVATED to 1010; added NPPM_GETCURRENTSCINTILLA, NPPM_GETCURRENTBUFFERID, NPPM_GETFULLPATHFROMBUFFERID, NPPM_ISDARKMODEENABLED, NPPM_ADDSCNMODIFIEDFLAGS, NPPN_DARKMODECHANGED
- `MarkdownPreview/include/Scintilla.h` - Added SCI_GETCURRENTPOS, SCI_LINEFROMPOSITION, SCI_GETCODEPAGE, SCN_MODIFIED, SCN_UPDATEUI, SC_MOD_INSERTTEXT, SC_MOD_DELETETEXT, SC_PERFORMED_UNDO, SC_PERFORMED_REDO
- `MarkdownPreview/src/PreviewPanel.h` - Added renderMarkdown, setTheme, scheduleRender, showIdle public methods; doRender, getCurrentText, handleJsMessage private methods; DEBOUNCE_TIMER_ID, m_renderPending, m_isDark, m_currentFilePath, m_webMessageReceivedToken members
- `MarkdownPreview/src/PreviewPanel.cpp` - Implemented all new methods; added WM_TIMER case to wndProc; added add_WebMessageReceived call in initWebView2; included Scintilla.h and PluginInterface.h; extern NppData nppData declaration
- `MarkdownPreview/src/PluginMain.cpp` - Added NPPN_BUFFERACTIVATED, NPPN_DARKMODECHANGED, SCN_MODIFIED, SCN_UPDATEUI cases to beNotified switch
- `MarkdownPreview/src/PluginDefinition.h` - Added Scintilla.h include; declared onBufferActivated, onDarkModeChanged, onScnModified, onScnUpdateUi
- `MarkdownPreview/src/PluginDefinition.cpp` - Added Scintilla.h include; added NPPM_ADDSCNMODIFIEDFLAGS and initial setTheme call in onNppReady; implemented all four Phase 2 notification handlers

## Decisions Made

- Include `PluginInterface.h` directly in PreviewPanel.cpp rather than `PluginDefinition.h` to avoid circular dependency (PreviewPanel.h is included by PluginDefinition.h). Declare `extern NppData nppData` explicitly.
- NPPN_BUFFERACTIVATED corrected to (NPPN_FIRST + 10) = 1010 per official Notepad++ source. The template had +9 which is wrong.
- SCN_MODIFIED (2008) and SCI_GETCURRENTPOS share value 2008 by design — they live in distinct namespaces (nmhdr.code vs SendMessage wParam) and never conflict.
- Add `#include "Scintilla.h"` to PluginDefinition.h so `SCNotification*` parameter type in the new handler declarations resolves for all translation units that include the header.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added `#include "Scintilla.h"` to PluginDefinition.h**
- **Found during:** Task 1 (first build attempt)
- **Issue:** `onScnModified(SCNotification* notification)` declared in PluginDefinition.h but `SCNotification` undefined there — Scintilla.h not included in the header
- **Fix:** Added `#include "Scintilla.h"` to PluginDefinition.h includes block
- **Files modified:** MarkdownPreview/src/PluginDefinition.h
- **Verification:** Build succeeded after fix
- **Committed in:** d4475c1 (task commit)

**2. [Rule 3 - Blocking] Added `#include "Scintilla.h"` to PreviewPanel.cpp**
- **Found during:** Task 1 (first build attempt)
- **Issue:** PreviewPanel::getCurrentText uses SCI_GETLENGTH and SCI_GETTEXT but Scintilla.h was not included in PreviewPanel.cpp
- **Fix:** Added `#include "Scintilla.h"` after the existing Notepad_plus_msgs.h include
- **Files modified:** MarkdownPreview/src/PreviewPanel.cpp
- **Verification:** Build succeeded after fix
- **Committed in:** d4475c1 (task commit)

**3. [Rule 3 - Blocking] Used PluginInterface.h + extern instead of PluginDefinition.h in PreviewPanel.cpp**
- **Found during:** Task 1 (design analysis)
- **Issue:** PreviewPanel::getCurrentText needs nppData (defined in PluginDefinition.cpp) but including PluginDefinition.h would create a circular dependency since PluginDefinition.h includes PreviewPanel.h
- **Fix:** Include `../include/PluginInterface.h` directly for the NppData struct definition and declare `extern NppData nppData` explicitly in PreviewPanel.cpp
- **Files modified:** MarkdownPreview/src/PreviewPanel.cpp
- **Verification:** Build succeeded; no circular includes
- **Committed in:** d4475c1 (task commit)

**4. [Rule 3 - Blocking] Created packages junction point in worktree**
- **Found during:** Task 1 (first build attempt)
- **Issue:** NuGet packages directory not present in worktree — build error for missing WebView2.targets
- **Fix:** Created Windows junction point: worktree/packages -> main repo packages/
- **Files modified:** (filesystem junction, no source files)
- **Verification:** Build succeeded after junction created
- **Committed in:** n/a (filesystem operation, not tracked in git)

---

**Total deviations:** 4 auto-fixed (all Rule 3 blocking issues)
**Impact on plan:** All fixes required for correct compilation. No scope creep — no new features or architectural changes.

## Issues Encountered

- MSBuild path differs from plan's PLAN.md verify command (plan shows VS 2022 path, actual install is VS 18 at a different path). Discovered and corrected during build verification.
- Forward declaration of `struct NppData;` insufficient — needed complete type for member access. Resolved by including PluginInterface.h instead.

## Known Stubs

- `handleJsMessage()` exportReady branch is a stub — no implementation body yet. Plan 02-04 will add `saveExportedHtml()` call here. The stub is intentional and documented in the plan.
- `onScnUpdateUi()` is a no-op placeholder. Plan 02-03 will add scroll sync implementation.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- C++ messaging backbone complete — Plan 02-02 (JS preview.html) can now receive `{type:"render", markdown:..., filePath:...}` and `{type:"theme", dark:bool}` messages via window.chrome.webview.addEventListener
- Plan 02-03 scroll sync: onScnUpdateUi placeholder is wired; implementation adds `{type:"scroll", line:N}` PostWebMessageAsJson call
- Plan 02-04 export: add_WebMessageReceived and handleJsMessage exportReady stub are wired; implementation adds saveExportedHtml file write logic

---
*Phase: 02-core-preview*
*Completed: 2026-04-09*
