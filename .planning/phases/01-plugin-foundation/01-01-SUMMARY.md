---
phase: 01-plugin-foundation
plan: 01
subsystem: infra
tags: [msvc, vs2022, notepad++, plugin, webview2, nuget, c++, dll]

# Dependency graph
requires: []
provides:
  - VS2022 solution with dual-platform (x86/x64) build configurations
  - Plugin DLL skeleton with six required Notepad++ exports
  - NuGet package references (WebView2 SDK, WIL)
  - nlohmann/json single-header library
  - Proven build pipeline producing MarkdownPreview.dll for both architectures
affects: [01-02-PLAN, 01-03-PLAN]

# Tech tracking
tech-stack:
  added: [MSVC v145 (VS 2026), WebView2 SDK 1.0.3856.49, WIL 1.0.240803.1, nlohmann/json 3.11.3]
  patterns: [NppCppMSVS-style plugin structure, extern C dllexport pattern, FuncItem menu registration]

key-files:
  created:
    - MarkdownPreview.sln
    - MarkdownPreview/MarkdownPreview.vcxproj
    - MarkdownPreview/packages.config
    - MarkdownPreview/src/PluginMain.cpp
    - MarkdownPreview/src/PluginDefinition.cpp
    - MarkdownPreview/src/PluginDefinition.h
    - MarkdownPreview/include/PluginInterface.h
    - MarkdownPreview/include/Notepad_plus_msgs.h
    - MarkdownPreview/include/Scintilla.h
    - MarkdownPreview/include/Docking.h
    - MarkdownPreview/include/menuCmdID.h
    - .gitignore
  modified: []

key-decisions:
  - "PlatformToolset v145 instead of v143 (dev machine has VS 2026, not VS 2022)"
  - "Plugin display name MarkdownPreview with Toggle Preview menu item and Ctrl+Shift+M shortcut"

patterns-established:
  - "Plugin DLL exports: six extern C dllexport functions in PluginMain.cpp delegating to PluginDefinition"
  - "Header organization: plugin API headers in include/, source in src/"
  - "NuGet packages.config with solution-level packages/ directory"
  - "Build output: bin/{platform}/{configuration}/ with post-build WebView2Loader.dll copy"

requirements-completed: [INFR-02]

# Metrics
duration: 16min
completed: 2026-04-08
---

# Phase 01 Plan 01: VS2022 Solution and Plugin Skeleton Summary

**Notepad++ plugin DLL skeleton with six required exports, dual x86/x64 build, WebView2 SDK and WIL NuGet references, Ctrl+Shift+M toggle shortcut**

## Performance

- **Duration:** 16 min
- **Started:** 2026-04-08T18:23:10Z
- **Completed:** 2026-04-08T18:38:56Z
- **Tasks:** 3
- **Files modified:** 12

## Accomplishments
- VS2022 solution with four build configurations (Debug/Release x86/x64) producing MarkdownPreview.dll
- Plugin DLL exports all six required Notepad++ functions verified by dumpbin
- WebView2Loader.dll correctly bundled in both architecture output directories
- Toggle Preview menu command registered with Ctrl+Shift+M keyboard shortcut

## Task Commits

Each task was committed atomically:

1. **Task 1: Create VS2022 solution with NuGet packages** - `3e1060c` (feat)
2. **Task 2: Implement plugin DLL exports and skeleton** - `085da28` (feat)
3. **Task 3: Restore NuGet packages and verify build** - `f3bf7a9` (chore)

## Files Created/Modified
- `MarkdownPreview.sln` - VS2022 solution with x86/x64 Debug/Release configurations
- `MarkdownPreview/MarkdownPreview.vcxproj` - Project file with v145 toolset, C++17, NuGet imports, post-build copy
- `MarkdownPreview/packages.config` - NuGet package references (WebView2, WIL)
- `MarkdownPreview/src/PluginMain.cpp` - DLL entry point with six exported functions
- `MarkdownPreview/src/PluginDefinition.cpp` - Plugin init, menu setup, toggle stub
- `MarkdownPreview/src/PluginDefinition.h` - Plugin declarations, PLUGIN_NAME, NB_FUNC
- `MarkdownPreview/include/PluginInterface.h` - NppData, FuncItem, ShortcutKey structs
- `MarkdownPreview/include/Notepad_plus_msgs.h` - NPPM_DMM* and NPPN_* message constants
- `MarkdownPreview/include/Scintilla.h` - Minimal SCNotification struct and SCI_* messages
- `MarkdownPreview/include/Docking.h` - tTbData struct and DWS_DF_CONT_* constants
- `MarkdownPreview/include/menuCmdID.h` - IDM base constant
- `MarkdownPreview/include/nlohmann/json.hpp` - nlohmann/json v3.11.3 single-header
- `.gitignore` - Excludes bin/, obj/, packages/, .vs/, nuget.exe

## Decisions Made
- **PlatformToolset v145 instead of v143:** Dev machine has Visual Studio Professional 2026 (v18.3.2) which provides PlatformToolset v145 (not v143 from VS 2022). The plan specified v143 but that toolset is not installed. v145 is fully backward compatible and the compiled DLL is ABI-compatible with Notepad++.
- **Installed C++ desktop workload:** The VS 2026 installation did not have the "Desktop development with C++" workload. Installed it via VS Installer CLI (`setup.exe modify --add Microsoft.VisualStudio.Workload.NativeDesktop`).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Installed missing C++ desktop development workload**
- **Found during:** Task 3 (Build verification)
- **Issue:** Visual Studio Professional 2026 did not have C++ build tools installed (Microsoft.Cpp.Default.props not found)
- **Fix:** Installed NativeDesktop workload via elevated VS Installer CLI
- **Files modified:** None (system-level VS installation)
- **Verification:** MSVC 14.50.35717 compiler and MSBuild C++ targets now available
- **Committed in:** N/A (system change, not a code change)

**2. [Rule 3 - Blocking] Changed PlatformToolset from v143 to v145**
- **Found during:** Task 3 (Build verification)
- **Issue:** VS 2026 provides PlatformToolset v145, not v143. Build would fail with missing toolset error.
- **Fix:** Updated all four configuration blocks in vcxproj from v143 to v145
- **Files modified:** MarkdownPreview/MarkdownPreview.vcxproj
- **Verification:** Both x86 and x64 Release builds succeed
- **Committed in:** f3bf7a9 (Task 3 commit)

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both fixes were necessary to enable building on the available development environment. No scope creep. The PlatformToolset change may need adjustment when targeting VS 2022 for distribution (can add v143 back as an option later).

## Issues Encountered
- NuGet CLI not in PATH: Downloaded nuget.exe directly from dist.nuget.org for package restore
- MSBuild `/p:` flag interpreted as path in bash: Used `-p:` prefix instead

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Build infrastructure proven working for both x86 and x64
- Plugin skeleton ready for docking panel implementation (Plan 02)
- All Notepad++ API headers in place for NPPM_DMMREGASDCKDLG and related messages
- WebView2 SDK headers and libraries available for Plan 02 WebView2 initialization

## Self-Check: PASSED

All 12 created files verified present. All 3 task commits verified in git log.

---
*Phase: 01-plugin-foundation*
*Completed: 2026-04-08*
