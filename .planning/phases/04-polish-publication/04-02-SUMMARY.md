---
phase: 04-polish-publication
plan: 02
subsystem: ui
tags: [webview2, javascript, toc, sidebar, scroll-sync, notepad-plus-plus]

# Dependency graph
requires:
  - phase: 04-polish-publication
    plan: 01
    provides: click-to-editor navigation with lineClick postMessage mechanism
  - phase: 03-extended-rendering
    provides: preview.html with data-line attributes on all block tokens via source_map rule
provides:
  - TOC sidebar nav#toc with buildToc() generated from rendered headings
  - Active heading tracking via updateTocActive() on editor scroll
  - TOC click navigation to both preview (scrollIntoView) and editor (lineClick postMessage)
  - Flex layout wrapper #layout containing nav#toc and #preview as scrollable flex child
affects:
  - 04-03 and later plans (layout now uses flex; #preview is scroll container, not body)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Flex layout: #layout display:flex with nav#toc (200px fixed) and #preview (flex:1 overflow-y:auto)"
    - "body { overflow:hidden } makes #preview the scroll container instead of body"
    - "TOC generation: querySelectorAll('#preview h1,...,h6') after each renderMarkdown()"
    - "Active heading: updateTocActive(line) walks toc-entry data-line values, applies toc-active class"
    - "TOC click: e.stopPropagation() + scrollIntoView + postMessage lineClick (same as Plan 01)"
    - "XSS guard: a.textContent = text (not innerHTML) per T-04-03"

key-files:
  created: []
  modified:
    - MarkdownPreview/assets/preview.html

key-decisions:
  - "body overflow:hidden chosen so #preview (flex child) is the scroll container — avoids double scrollbar"
  - "min-width:0 on #preview prevents flex blowout from wide pre/table blocks"
  - "TOC anchor uses a.textContent (not innerHTML) for XSS safety (T-04-03 mitigation)"
  - "buildToc() hides #toc when no headings found, allowing full-width content (D-06)"
  - "#idle-state and #error-state kept outside #layout to avoid flex interference with their height:100vh centering"
  - "e.stopPropagation() on TOC entry click as defense-in-depth (TOC is outside #preview so Plan 01 listener would not fire anyway)"

requirements-completed: [SCRL-03]

# Metrics
duration: 10min
completed: 2026-04-10
---

# Phase 04 Plan 02: TOC Sidebar Summary

**Flex layout with nav#toc sidebar generated from rendered headings; buildToc() and updateTocActive() wire TOC visibility, active-heading tracking, and dual-target click navigation (preview scroll + editor lineClick)**

## Performance

- **Duration:** ~10 min
- **Started:** 2026-04-10T16:29:41Z
- **Completed:** 2026-04-10T16:39:38Z
- **Tasks:** 1
- **Files modified:** 1

## Accomplishments

- Added `#layout` flex wrapper containing `nav#toc` (200px fixed sidebar) and `#preview` (flex:1 scroll child)
- `#idle-state` and `#error-state` remain outside `#layout` — no flex interference with their viewport-height centering
- Added `body { overflow:hidden }` to make `#preview` the scroll container (prevents body double-scroll)
- Added `#preview { flex:1; overflow-y:auto; min-width:0 }` — min-width:0 prevents flex blowout on wide code blocks
- Added `.toc-entry`, `.toc-active`, `.toc-h1` through `.toc-h6` CSS rules with dark theme variants
- `buildToc()`: querySelectorAll headings after each render, hides TOC when empty (D-06), builds `<a>` entries with textContent (XSS-safe), click handler calls `scrollIntoView` + posts `lineClick` message
- `updateTocActive(line)`: walks toc-entry data-line values, applies `toc-active` to the nearest heading above current line
- Wired `buildToc()` after `injectCopyButtons()` in `renderMarkdown()`
- Wired `updateTocActive(msg.line || 0)` in the `scroll` case of the message dispatcher

## Task Commits

1. **Task 1: TOC sidebar implementation** - `433010c` (feat)

## Files Created/Modified

- `MarkdownPreview/assets/preview.html` - Added flex layout (#layout, nav#toc), TOC CSS, buildToc(), updateTocActive(), wired both into render pipeline and scroll dispatcher

## Decisions Made

- `body { overflow:hidden }` required so `#preview` (not `body`) is the scroll container; without this, body scrolls and the flex child does not
- `min-width: 0` on `#preview` is a required flex child property to prevent blowout from wide `<pre>` or `<table>` blocks
- `a.textContent = text` (not `innerHTML`) per T-04-03 XSS mitigation — heading text from markdown-it render is treated as plain text in the TOC anchor
- `e.stopPropagation()` on TOC entry click as defense-in-depth; Plan 01 listener is scoped to `#preview` so it would not fire for TOC clicks anyway, but stopPropagation makes the isolation explicit

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None - the TOC sidebar is fully wired end-to-end: headings detected, TOC built, active tracking on scroll, click navigation to both preview and editor.

## Threat Flags

No new threat surface beyond what is documented in the plan's threat model (T-04-03, T-04-04).
- T-04-03 mitigated: `a.textContent = text` used (not innerHTML)
- T-04-04 accepted: data-line values read from rendered heading elements with integer-only source_map values

## Self-Check: PASSED

- FOUND: MarkdownPreview/assets/preview.html
- FOUND: .planning/phases/04-polish-publication/04-02-SUMMARY.md
- FOUND commit: 433010c (Task 1)
- VERIFIED: `id="toc"` present in body HTML
- VERIFIED: `id="layout"` present in body HTML
- VERIFIED: `function buildToc()` in script section
- VERIFIED: `function updateTocActive(` in script section
- VERIFIED: `buildToc();` called after `injectCopyButtons()` in renderMarkdown()
- VERIFIED: `updateTocActive(msg.line || 0)` in scroll case of dispatcher
- VERIFIED: `.toc-active` CSS rule present
- VERIFIED: `.toc-h1` and `.toc-h2` CSS rules present
- VERIFIED: `overflow: hidden` applied to `body`
- VERIFIED: no `document.addEventListener('click'` in file
- VERIFIED: `#preview` still has `class="markdown-body"`
- VERIFIED: `#idle-state` is outside `#layout` (sibling, not child)

---
*Phase: 04-polish-publication*
*Completed: 2026-04-10*
