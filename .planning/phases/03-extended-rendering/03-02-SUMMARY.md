---
phase: 03-extended-rendering
plan: 02
subsystem: assets/preview
tags: [mermaid, copy-to-clipboard, webview2, assets, diagrams]
dependency_graph:
  requires: [03-01]
  provides: [mermaid-diagram-rendering, copy-to-clipboard-buttons]
  affects: [MarkdownPreview/assets/preview.html, MarkdownPreview/assets/mermaid.min.js]
tech_stack:
  added:
    - mermaid 11.14.0 (mermaid.min.js, IIFE bundle, 3.16 MB, self-contained)
  patterns:
    - Per-diagram mermaid.render() with Promise.catch() for D-01 error fallback
    - _mermaid variable guard (window.mermaid || window.mermaid.default) for IIFE export shape
    - Post-render hook chain: hljs.highlightAll() -> renderMermaidDiagrams() -> injectCopyButtons()
    - Idempotent button injection (pre.querySelector('.copy-btn') guard)
    - Silent clipboard write via navigator.clipboard.writeText() — no execCommand
key_files:
  created:
    - MarkdownPreview/assets/mermaid.min.js
  modified:
    - MarkdownPreview/assets/preview.html
decisions:
  - Used mermaid.min.js IIFE bundle (not mermaid.esm.min.mjs) — ESM entry lazily imports chunk files by URL which would all need to be present locally; IIFE is self-contained
  - Used mermaid.render() per-diagram (not mermaid.run()) — run() inserts error SVGs on invalid syntax; render() returns a Promise that rejects cleanly enabling D-01 fallback
  - renderMermaidDiagrams() runs before injectCopyButtons() — mermaid replaces <pre> elements; copy buttons must only inject into remaining real code blocks
  - Silent clipboard copy (no feedback animation) per D-03 decision — no checkmark swap, no label change
metrics:
  duration: ~8 minutes
  completed: 2026-04-09T18:19:28Z
  tasks_completed: 2
  tasks_total: 2
  files_created: 1
  files_modified: 1
---

# Phase 03 Plan 02: Mermaid Diagrams + Copy Buttons — Summary

Mermaid 11.14.0 IIFE bundle downloaded as a local asset and wired into preview.html with per-diagram render(), D-01 error fallback (leave code block on parse failure), theme-aware initialization, and icon-only clipboard copy buttons on all non-mermaid code blocks.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Download Mermaid IIFE bundle | 9013031 | MarkdownPreview/assets/mermaid.min.js (3.16 MB) |
| 2 | Add Mermaid rendering and copy-to-clipboard buttons to preview.html | 629a2f5 | MarkdownPreview/assets/preview.html |

## What Was Built

**Task 1 — Mermaid IIFE bundle:**
`mermaid@11.14.0/dist/mermaid.min.js` downloaded from jsDelivr CDN (pinned exact version per T-03-05 tampering mitigation). The IIFE bundle is 3.16 MB and fully self-contained — it exposes `window.mermaid` with no chunk file dependencies. File size > 2 MB confirmed the correct artifact (the ESM entry is tiny by comparison).

**Task 2 — preview.html additions (7 changes):**

1. **Script tag in `<head>`:** `<script src="https://appassets.mdpreview/mermaid.min.js">` inserted after `markdown-it-footnote.min.js`, before `</head>`. Served via the existing `appassets.mdpreview` virtual host mapping.

2. **Copy button CSS in `<style>` block:** `pre { position: relative }` + `.copy-btn` positioned absolute top-right, `display:none` by default, revealed via `pre:hover .copy-btn { display: block }`. Full dark-mode variants using `body[data-theme="dark"]` selector.

3. **`_mermaid` initialization block:** Guard pattern `(window.mermaid && typeof window.mermaid.initialize === 'function') ? window.mermaid : window.mermaid?.default` handles both IIFE export shapes. `_mermaid.initialize({ startOnLoad: false, theme: 'default' })` called once on page load.

4. **`renderMermaidDiagrams()` function:** Queries `code.language-mermaid` elements, extracts `textContent` (strips hljs spans), calls `_mermaid.render(uniqueId, source)` per diagram. On resolve: replaces `<pre>` with `<div class="mermaid-diagram">` containing the SVG. On reject (D-01): leaves the `<pre><code>` block untouched — hljs-highlighted code is shown instead of an error SVG.

5. **`injectCopyButtons()` function:** Queries `pre > code`, skips blocks already having `.copy-btn` (idempotent). Appends a `<button class="copy-btn">` with GitHub clipboard SVG. Click handler calls `navigator.clipboard.writeText(codeEl.textContent)` silently — no feedback per D-03.

6. **Post-render calls in `renderMarkdown()`:** After `hljs.highlightAll()`, calls `renderMermaidDiagrams()` then `injectCopyButtons()`. Order is critical: mermaid replaces `<pre>` nodes first, then copy buttons inject into the remaining ones.

7. **`setTheme()` extension:** After the existing CSS link swap, calls `_mermaid.initialize({ startOnLoad: false, theme: isDark ? 'dark' : 'default' })` so the next render cycle uses the correct Mermaid theme.

Security invariants preserved: `html: false` on `markdownit()` constructor (T-02-06 XSS mitigation) untouched. Mermaid runs inside WebView2 sandbox (T-03-06). SVG injected via `innerHTML` on an isolated container div — Mermaid sanitizes its own SVG output.

## Deviations from Plan

None — plan executed exactly as written. All 7 changes applied in the specified order with the specified content.

## Known Stubs

None. All functionality is fully wired:
- `_mermaid` is initialized from the downloaded IIFE bundle
- `renderMermaidDiagrams()` called on every render cycle
- `injectCopyButtons()` called on every render cycle
- `setTheme()` extended to update mermaid theme on dark/light toggle

## Threat Flags

No new trust boundaries beyond those in the plan's threat model.
- T-03-05 mitigated: pinned mermaid@11.14.0 URL, file size > 2 MB verified
- T-03-06 mitigated: WebView2 sandbox contains mermaid execution; `html: false` blocks raw HTML passthrough
- T-03-07 accepted: clipboard writes only user's own code content, user-initiated click
- T-03-08 accepted: async per-diagram rendering, user-authored content
- T-03-09 mitigated: D-01 catch leaves `<pre><code>` in place — no error SVG injected, no XSS vector from Mermaid error content

## Self-Check: PASSED

Files exist:
- MarkdownPreview/assets/mermaid.min.js: FOUND (3,164,970 bytes)
- MarkdownPreview/assets/preview.html: FOUND (modified)
- .planning/phases/03-extended-rendering/03-02-SUMMARY.md: FOUND

Commits exist:
- 9013031: FOUND
- 629a2f5: FOUND
