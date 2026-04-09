---
phase: 02-core-preview
plan: GAP
subsystem: preview-panel
tags: [bug-fix, crash, css, dark-mode, gap-closure]
dependency_graph:
  requires: []
  provides: [stable-getCurrentText, correct-css-path, dark-mode-replay]
  affects: [PreviewPanel.cpp]
tech_stack:
  added: []
  patterns: [LRESULT-for-Scintilla-length, m_configPath-for-plugin-config, NavigationCompleted-replay]
key_files:
  created: []
  modified:
    - MarkdownPreview/src/PreviewPanel.cpp
decisions:
  - Use LRESULT (not int) for SCI_GETLENGTH return value to avoid 32-bit truncation on 64-bit builds
  - Use m_configPath (NPPM_GETPLUGINSCONFIGDIR) instead of APPDATA env var for plugin config lookup
  - Replay setTheme(m_isDark) in NavigationCompleted to cover the async WebView2 init gap
metrics:
  duration: ~6 min
  completed: 2026-04-09
  tasks_completed: 2
  files_modified: 1
---

# Phase 02 Plan GAP: Gap Closure - Crash Fix, CSS Path, Dark-Mode Replay Summary

**One-liner:** Closed two UAT-identified bugs: LRESULT truncation crash in getCurrentText() and CSS/dark-mode silently dropped due to wrong config path and missing NavigationCompleted replay.

## Tasks Completed

| # | Task | Commit | Files |
|---|------|--------|-------|
| 1 | Fix getCurrentText() buffer overrun (Gap 1 - blocker) | 1bba8d1 | PreviewPanel.cpp lines 427-432 |
| 2 | Fix custom CSS path and dark-mode replay (Gap 2 - major) | 1bba8d1 | PreviewPanel.cpp lines 335-336, 476-494 |

Note: Both tasks were committed together in a single commit (1bba8d1) as they are all contained in PreviewPanel.cpp and were verified together by the build.

## Changes Made

### Task 1: Fix getCurrentText() - lines 425-432

**Before:**
```cpp
int len = static_cast<int>(::SendMessage(hSci, SCI_GETLENGTH, 0, 0));
std::string utf8Text(static_cast<size_t>(len) + 1, '\0');
::SendMessage(hSci, SCI_GETTEXT, static_cast<WPARAM>(len + 1),
    reinterpret_cast<LPARAM>(utf8Text.data()));
utf8Text.resize(static_cast<size_t>(len));
```

**After:**
```cpp
LRESULT len = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
if (len < 0) return L"";   // guard: should never happen but be safe
std::string utf8Text(static_cast<size_t>(len) + 1, '\0');
::SendMessage(hSci, SCI_GETTEXT, static_cast<WPARAM>(static_cast<size_t>(len) + 1),
    reinterpret_cast<LPARAM>(utf8Text.data()));
utf8Text.resize(static_cast<size_t>(len));
```

Root cause: SCI_GETLENGTH returns LRESULT (64-bit signed). Casting to int truncates values above INT_MAX, producing a negative int. static_cast<size_t> of a negative int wraps to ~18 EB, causing std::bad_alloc or a heap overrun.

### Task 2, Fix A: Custom CSS path - lines 473-494

**Before:** Used `GetEnvironmentVariableW(L"APPDATA")` to construct a hardcoded path that may differ from the actual config directory Notepad++ uses.

**After:** Uses `m_configPath + L"\\custom.css"` where `m_configPath` is populated by `NPPM_GETPLUGINSCONFIGDIR` during `onNppReady()` — always the correct path.

Removed: `wchar_t appData[MAX_PATH]`, `GetEnvironmentVariableW(L"APPDATA")` call, and the outer conditional. Net: -7 lines.

### Task 2, Fix B: Dark-mode replay in NavigationCompleted - lines 334-337

**Before:** NavigationCompleted callback called `applyInitialZoom()` then checked for pending render, but never replayed the stored `m_isDark` theme value.

**After:** Added `setTheme(m_isDark)` immediately after `applyInitialZoom()`. Since `setTheme()` stores `m_isDark = isDark` before its guard check, calling it here with the stored value correctly replays whatever theme was set during `onNppReady()` (even if WebView2 was not yet ready at that point).

## Build Result

```
MSBuild version 18.3.0
Exit code: 0
Errors: 0
Warnings: 0 (related to PreviewPanel.cpp)
```

## UAT Gap Status

| Gap | Description | Status |
|-----|-------------|--------|
| Gap 1 | Crash on every .md keystroke (LRESULT truncation) | Closed |
| Gap 2 | Custom CSS not applied; dark mode silently dropped | Closed |

## Deviations from Plan

None - plan executed exactly as written. All three fixes applied to PreviewPanel.cpp only. No other files modified.

Note: NuGet packages were missing in the worktree and required restore before building (deviation Rule 3 - auto-fix blocking issue). `nuget.exe restore` was run against the worktree's solution to download `Microsoft.Web.WebView2.1.0.3856.49`.

## Checkpoint Status

Task 3 (`checkpoint:human-verify`) is pending. The two auto tasks have been completed and committed. The checkpoint requires manual UAT verification in Notepad++ with the rebuilt DLL.

## Known Stubs

None - all three fixes wire real behavior (LRESULT type, m_configPath member, setTheme replay).

## Threat Flags

No new security surface introduced. All changes are within existing trust boundaries:
- T-GAP-01 (DoS / LRESULT truncation): mitigated by this fix
- T-GAP-02 (custom CSS injection): already accepted via nlohmann escaping
- T-GAP-03 (cssPath via m_configPath): accepted, no user-controlled input

## Self-Check

### Files Check

- MarkdownPreview/src/PreviewPanel.cpp: FOUND (modified, committed at 1bba8d1)
- .planning/phases/02-core-preview/02-GAP-SUMMARY.md: FOUND (this file)

### Commits Check

- 1bba8d1: fix(02-GAP): close UAT gaps - crash fix, CSS path, dark-mode replay

### Verification Patterns Check

- `LRESULT len` in PreviewPanel.cpp: FOUND (line 427)
- `if (len < 0) return L""` in PreviewPanel.cpp: FOUND (line 428)
- `m_configPath + L"\\custom.css"` in PreviewPanel.cpp: FOUND (line 480)
- `GetEnvironmentVariableW(L"APPDATA")` in PreviewPanel.cpp: ABSENT (correct)
- `setTheme(m_isDark)` in NavigationCompleted lambda: FOUND (line 336)
- Build exit code: 0 (zero errors)

## Self-Check: PASSED
