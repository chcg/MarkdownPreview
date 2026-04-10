---
phase: 03-extended-rendering
plan: GAP
subsystem: ui
tags: [mermaid, webview2, zoom, notepad-plus-plus, javascript, cplusplus]

# Dependency graph
requires:
  - phase: 03-extended-rendering
    provides: Mermaid rendering pipeline and AcceleratorKeyPressed zoom handler from prior Phase 3 plans

provides:
  - Mermaid SVG rendering via off-screen getBBox layout container (UAT test 4 fix)
  - Ctrl+= / Ctrl+- / Ctrl+0 zoom hotkeys as NPP plugin FuncItem shortcuts (UAT test 7 fix)
  - zoomIn() / zoomOut() / zoomReset() public methods on PreviewPanel
  - NB_FUNC = 6 with three new zoom FuncItem registrations in PluginDefinition

affects:
  - phase: 04-polish (zoom UX, Mermaid PDF export)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Promise.allSettled for async cleanup after parallel Mermaid render() calls"
    - "Off-screen DOM container (position:absolute;left:-9999px) for getBBox() layout context in WebView2"
    - "NPP FuncItem shortcut registration as focus-independent alternative to AcceleratorKeyPressed"

key-files:
  created: []
  modified:
    - MarkdownPreview/assets/preview.html
    - MarkdownPreview/src/PluginDefinition.h
    - MarkdownPreview/src/PluginDefinition.cpp
    - MarkdownPreview/src/PreviewPanel.h
    - MarkdownPreview/src/PreviewPanel.cpp

key-decisions:
  - "Off-screen container (800x600px, position:absolute, left:-9999px) passed as third arg to mermaid.render() to give getBBox() real layout dimensions inside WebView2 overflow:hidden flex body"
  - "Promise.allSettled used (not Promise.all) so cleanup fires even when individual diagrams fail D-01 fallback"
  - "AcceleratorKeyPressed zoom logic neutralized (put_Handled(TRUE) kept to suppress WebView2 built-in zoom, but custom zoom computation removed) to prevent double-zoom via FuncItem + AcceleratorKeyPressed"
  - "NB_FUNC increased from 3 to 6 — funcItems[3]=ZoomIn(Ctrl+=), funcItems[4]=ZoomOut(Ctrl+-), funcItems[5]=ZoomReset(Ctrl+0)"

patterns-established:
  - "Pattern: Off-screen container for Mermaid rendering — pass as third arg to mermaid.render() when WebView2 body uses overflow:hidden or flex layout that breaks getBBox()"
  - "Pattern: NPP FuncItem shortcuts for keyboard actions that must fire regardless of WebView2 focus state"

requirements-completed: [XRND-02, THME-04]

# Metrics
duration: 20min
completed: 2026-04-10
---

# Phase 03-GAP: Gap Closure Summary

**Mermaid SVG rendering fixed via off-screen layout container; Ctrl+=/−/0 zoom hotkeys wired as NPP FuncItem shortcuts bypassing Scintilla focus constraint**

## Performance

- **Duration:** ~20 min
- **Started:** 2026-04-10T19:28:00Z
- **Completed:** 2026-04-10T19:48:20Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Mermaid fenced code blocks now render as SVG diagrams: `renderMermaidDiagrams()` rewritten to create an off-screen container (800x600px, `position:absolute;left:-9999px`) and pass it as the third argument to `_mermaid.render()`, giving getBBox() real layout dimensions instead of zeros
- Promise.allSettled cleanup removes the off-screen container after all render promises settle, preventing DOM pollution in PDF exports
- Ctrl+=/−/0 zoom hotkeys now fire from Scintilla focus: registered as NPP plugin FuncItem shortcuts (funcItems[3-5]) that directly call `g_previewPanel.zoomIn/zoomOut/zoomReset()`, bypassing the WebView2 focus requirement
- AcceleratorKeyPressed zoom logic neutralized — `put_Handled(TRUE)` retained to suppress WebView2 built-in zoom, but custom zoom computation removed to prevent double-zoom
- Both Debug and Release builds succeed with 0 errors, 0 warnings

## Task Commits

Each task was committed atomically:

1. **Task 1: Fix Mermaid getBBox layout failure with off-screen render container** - `2693ad9` (fix)
2. **Task 2: Register Ctrl+=/−/0 as NPP plugin shortcuts that call PreviewPanel zoom methods** - `7793e1b` (feat)

## Files Created/Modified

- `MarkdownPreview/assets/preview.html` - renderMermaidDiagrams() rewritten with off-screen container + Promise.allSettled cleanup
- `MarkdownPreview/src/PluginDefinition.h` - NB_FUNC 3→6; added zoomInPreview/zoomOutPreview/zoomResetPreview declarations
- `MarkdownPreview/src/PluginDefinition.cpp` - Added zoomInShortcut/zoomOutShortcut/zoomResetShortcut structs; registered funcItems[3-5]; added three callback implementations
- `MarkdownPreview/src/PreviewPanel.h` - Added public zoomIn(), zoomOut(), zoomReset() declarations
- `MarkdownPreview/src/PreviewPanel.cpp` - Implemented zoomIn/zoomOut/zoomReset with 10% step, 80%-800% clamp, settings persistence, WR-02 PDF guard; neutralized AcceleratorKeyPressed zoom computation

## Decisions Made

- Used `Promise.allSettled` rather than `Promise.all` so the off-screen container cleanup runs even when individual Mermaid diagrams fail (D-01 fallback path)
- Kept `args->put_Handled(TRUE)` in AcceleratorKeyPressed after neutralizing the zoom logic — this still correctly suppresses WebView2's built-in Ctrl+/- browser zoom, which is the desired behavior regardless of which code path applies the custom zoom
- `VK_OEM_PLUS`, `VK_OEM_MINUS`, and `0x30` literals used directly in ShortcutKey structs — consistent with virtual key usage in the existing AcceleratorKeyPressed handler

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

- MSBuild required dash-style flags (`-p:Configuration=Debug`) rather than slash-style (`/p:Configuration=Debug`) when invoked from bash due to bash path-stripping behavior. Non-blocking — workaround applied immediately.

## User Setup Required

None — no external service configuration required.

## Known Stubs

None — all Mermaid and zoom functionality is fully wired.

## Threat Flags

No new security-relevant surface introduced beyond what is documented in the plan's threat model (T-03G-01 through T-03G-04). The off-screen container is removed after rendering and does not create a persistent DOM surface.

## Next Phase Readiness

- UAT tests 4 (Mermaid SVG) and 7 (zoom hotkeys) are now fixed; tests 8 (zoom persistence) and 9 (clean PDF) are unblocked
- Phase 4 polish can proceed: zoom UX, Mermaid rendering, and PDF export are all in a clean state
- No blockers or concerns

## Self-Check: PASSED

- `MarkdownPreview/assets/preview.html` — FOUND (contains `left:-9999px`, `Promise.allSettled`)
- `MarkdownPreview/src/PluginDefinition.h` — FOUND (contains `NB_FUNC = 6`, `zoomInPreview`)
- `MarkdownPreview/src/PluginDefinition.cpp` — FOUND (contains `zoomInShortcut`, `funcItems[3]`)
- `MarkdownPreview/src/PreviewPanel.h` — FOUND (contains `void zoomIn()`)
- `MarkdownPreview/src/PreviewPanel.cpp` — FOUND (contains `void PreviewPanel::zoomIn()`)
- Commit `2693ad9` — FOUND
- Commit `7793e1b` — FOUND

---
*Phase: 03-extended-rendering*
*Completed: 2026-04-10*
