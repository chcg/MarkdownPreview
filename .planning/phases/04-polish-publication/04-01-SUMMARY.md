---
phase: 04-polish-publication
plan: 01
subsystem: ui
tags: [webview2, scintilla, javascript, click-navigation, notepad-plus-plus]

# Dependency graph
requires:
  - phase: 03-extended-rendering
    provides: preview.html with data-line attributes on all block tokens via source_map rule
provides:
  - JS delegated click listener on #preview that posts lineClick messages to C++
  - C++ navigateEditorToLine() method using SCI_ENSUREVISIBLE + SCI_GOTOLINE + SCI_SCROLLCARET
  - lineClick handler in handleJsMessage() with T-04-01 guard (line >= 0)
affects:
  - 04-02-toc-sidebar (TOC sidebar adds nav outside #preview — click listener correctly scoped)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "JS->C++ message: post JSON via window.chrome.webview.postMessage, handle in handleJsMessage() switch"
    - "Scintilla navigation: SCI_ENSUREVISIBLE then SCI_GOTOLINE then SCI_SCROLLCARET then SetFocus"
    - "Delegated click listener on container element, not document, for TOC isolation"

key-files:
  created: []
  modified:
    - MarkdownPreview/assets/preview.html
    - MarkdownPreview/src/PreviewPanel.cpp
    - MarkdownPreview/src/PreviewPanel.h

key-decisions:
  - "Click listener scoped to #preview element (not document) so Plan 02 TOC sidebar clicks do not trigger editor navigation"
  - "data-line is 0-indexed (from token.map[0]) and SCI_GOTOLINE is 0-indexed — no offset conversion needed"
  - "T-04-01 mitigation: j.value(line,-1) default plus if(line>=0) guard prevents negative line passed to Scintilla"

patterns-established:
  - "IIFE wrapper for DOM-dependent setup code to capture element reference at setup time"
  - "navigateEditorToLine() mirrors getCurrentText() Scintilla handle retrieval pattern exactly"

requirements-completed: [SCRL-02]

# Metrics
duration: 8min
completed: 2026-04-10
---

# Phase 04 Plan 01: Click-to-Editor Navigation Summary

**Delegated JS click listener on #preview posts lineClick to C++; navigateEditorToLine() uses SCI_GOTOLINE + SetFocus for silent caret jump with T-04-01 tamper guard**

## Performance

- **Duration:** ~8 min
- **Started:** 2026-04-10T16:21:00Z
- **Completed:** 2026-04-10T16:29:41Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- JS IIFE click listener delegates on `#preview`, walks ancestors for `data-line`, posts `{type:'lineClick', line:N}` via `window.chrome.webview.postMessage`
- C++ `navigateEditorToLine()` helper calls `SCI_ENSUREVISIBLE`, `SCI_GOTOLINE`, `SCI_SCROLLCARET`, then `SetFocus` — silent, no animation
- `handleJsMessage()` extended with `lineClick` branch; T-04-01 tamper guard (`j.value("line",-1)` + `if (line >= 0)`) prevents negative Scintilla line

## Task Commits

Each task was committed atomically:

1. **Task 1: JS click listener in preview.html** - `2b2643f` (feat)
2. **Task 2: navigateEditorToLine() and lineClick handler in PreviewPanel** - `def9ce1` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified
- `MarkdownPreview/assets/preview.html` - Added IIFE click listener on #preview that posts lineClick to C++
- `MarkdownPreview/src/PreviewPanel.cpp` - Added lineClick else-if in handleJsMessage(); added navigateEditorToLine() definition
- `MarkdownPreview/src/PreviewPanel.h` - Added void navigateEditorToLine(int line) declaration in private section

## Decisions Made
- Click listener scoped to `#preview` (not `document`) so the TOC sidebar Plan 02 will add outside `#preview` does not send spurious lineClick messages
- No line offset conversion: `data-line` values come from `token.map[0]` (0-indexed) and `SCI_GOTOLINE` is also 0-indexed
- T-04-01 tamper mitigation applied inline: `j.value("line", -1)` with `if (line >= 0)` guard — Scintilla never receives a negative line number

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- Worktree has its own file copies separate from the main repo path; initial edit targeted the main repo path and had to be redirected to the worktree path. No code impact.

## Known Stubs

None - the click-to-editor navigation is fully wired end-to-end.

## Threat Flags

No new threat surface beyond what is documented in the plan's threat model (T-04-01, T-04-02).

## Self-Check: PASSED

- FOUND: MarkdownPreview/assets/preview.html
- FOUND: MarkdownPreview/src/PreviewPanel.cpp
- FOUND: MarkdownPreview/src/PreviewPanel.h
- FOUND: .planning/phases/04-polish-publication/04-01-SUMMARY.md
- FOUND commit: 2b2643f (Task 1)
- FOUND commit: def9ce1 (Task 2)

## Next Phase Readiness
- Click-to-editor navigation complete and isolated from TOC sidebar (Plan 02)
- Plan 02 (TOC sidebar) can add `<nav id="toc">` outside `#preview` with no click handler conflict
- No blockers

---
*Phase: 04-polish-publication*
*Completed: 2026-04-10*
