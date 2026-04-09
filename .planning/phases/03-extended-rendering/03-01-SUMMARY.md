---
phase: 03-extended-rendering
plan: 01
subsystem: assets/preview
tags: [katex, math, footnotes, markdown-it, webview2, assets]
dependency_graph:
  requires: []
  provides: [katex-math-rendering, footnote-rendering]
  affects: [MarkdownPreview/assets/preview.html]
tech_stack:
  added:
    - KaTeX 0.16.45 (katex.min.js + katex.min.css + 20 woff2 fonts)
    - markdown-it-texmath 1.0.0 (texmath.js, browser UMD)
    - markdown-it-footnote 4.0.0 (markdown-it-footnote.min.js, UMD)
  patterns:
    - Progressive plugin loading with window.* guard (if (window.plugin) md.use(...))
    - KaTeX CSS loaded before md-theme link for correct cascade
    - throwOnError:false for graceful malformed-LaTeX inline error spans
key_files:
  created:
    - MarkdownPreview/assets/katex.min.js
    - MarkdownPreview/assets/katex.min.css
    - MarkdownPreview/assets/texmath.js
    - MarkdownPreview/assets/markdown-it-footnote.min.js
    - MarkdownPreview/assets/fonts/ (20 KaTeX woff2 font files)
  modified:
    - MarkdownPreview/assets/preview.html
decisions:
  - Used markdown-it-texmath (texmath.js) instead of @vscode/markdown-it-katex — the VS Code fork has no browser UMD build (CommonJS only)
  - KaTeX_Math-Regular.woff2 not available in v0.16.45 dist; removed after confirming CDN 404; 20 other variants sufficient
  - throwOnError:false chosen for T-03-03 mitigation — malformed LaTeX renders red error span, not exception
metrics:
  duration: ~7 minutes
  completed: 2026-04-09T18:11:27Z
  tasks_completed: 2
  tasks_total: 2
  files_created: 25
  files_modified: 1
---

# Phase 03 Plan 01: KaTeX Math + Footnotes — Summary

KaTeX 0.16.45 and markdown-it-footnote 4.0.0 bundled as local assets and wired into the preview.html JS pipeline via markdown-it-texmath, enabling $...$ inline math, $$...$$ block math, and [^n] footnotes with back-references.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Download JS/CSS asset files | 4848e5b | katex.min.js, katex.min.css, texmath.js, markdown-it-footnote.min.js, fonts/ (20 woff2) |
| 2 | Wire KaTeX and footnote plugins into preview.html | b08bc11 | preview.html |

## What Was Built

Four new asset files and 20 KaTeX font files were downloaded from jsDelivr CDN (pinned exact versions per T-03-01 mitigation) and committed into `MarkdownPreview/assets/`. The preview.html JS pipeline was extended with three changes:

1. `<link rel="stylesheet" href=".../katex.min.css">` inserted as the first link in `<head>` (before `<link id="md-theme">`) so KaTeX styles are always present regardless of theme swap.
2. Three `<script>` tags added after `highlight.min.js`: `katex.min.js`, `texmath.js`, `markdown-it-footnote.min.js` — synchronous load, no defer.
3. Two `md.use()` calls added after the existing `markdownitTaskLists` block: `markdownitFootnote` then `texmath` (order matters — footnote anchors must not conflict with math delimiters).

Security invariants preserved: `html: false` on the `markdownit()` constructor (T-02-06 XSS mitigation) untouched. Scroll sync `md.core.ruler.push('source_map', ...)` untouched.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] KaTeX_Math-Regular.woff2 not available in v0.16.45**
- **Found during:** Task 1 font download verification
- **Issue:** CDN returned a 79-byte error message ("Couldn't find the requested file") instead of a valid woff2 for `KaTeX_Math-Regular.woff2` at katex@0.16.45
- **Fix:** Detected the invalid file (79 bytes vs thousands for valid fonts), removed it. The remaining 20 font variants cover all KaTeX glyph needs; KaTeX_Math-Regular is not present in the official v0.16.45 release
- **Files modified:** Removed `MarkdownPreview/assets/fonts/KaTeX_Math-Regular.woff2`
- **Impact:** None — KaTeX renders correctly without this file; acceptance criteria requires ≥10 fonts, we have 20

## Known Stubs

None. All plugin wiring is complete and functional. The `if (window.texmath && window.katex)` and `if (window.markdownitFootnote)` guards are defensive checks for missing assets, not stubs — the assets are present.

## Threat Flags

No new trust boundaries introduced beyond those in the plan's threat model. All CDN downloads use exact pinned versions (T-03-01). Fonts served from `appassets.mdpreview` virtual host (T-03-02 — existing DENY_CORS pattern). KaTeX `throwOnError: false` mitigates T-03-03. Footnote anchor IDs accepted per T-03-04.

## Self-Check: PASSED

Files exist:
- MarkdownPreview/assets/katex.min.js: FOUND
- MarkdownPreview/assets/katex.min.css: FOUND
- MarkdownPreview/assets/texmath.js: FOUND
- MarkdownPreview/assets/markdown-it-footnote.min.js: FOUND
- MarkdownPreview/assets/fonts/ (20 woff2 files): FOUND

Commits exist:
- 4848e5b: FOUND
- b08bc11: FOUND
