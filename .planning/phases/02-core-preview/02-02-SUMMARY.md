---
phase: 02-core-preview
plan: 02
subsystem: ui
tags: [markdown-it, highlight.js, github-markdown-css, webview2, preview-html, javascript]

# Dependency graph
requires:
  - phase: 02-core-preview
    provides: "Plan 02-01 C++ side — WebView2 virtual host appassets.mdpreview serving assets/"
provides:
  - "preview.html: complete in-WebView2 JS rendering pipeline for markdown-it GFM + syntax highlighting"
  - "7 bundled JS/CSS assets: markdown-it 14.1.1, highlight.js 11.11.1, github-markdown-css 5.9.0 light+dark, hljs github light+dark"
  - "WebView2 message dispatcher: render/theme/scroll/export message protocol"
  - "setTheme(): atomic swap of github-markdown and hljs CSS link pair"
  - "setCustomCss(): inline style injection for user custom CSS (D-07)"
  - "stripFrontmatter(): YAML frontmatter strip before parse (D-03)"
  - "source_map core rule: data-line attributes on block tokens for scroll sync (D-04)"
  - "Stub scrollToLine() and exportHtml() safe for Plans 02-03/02-04 to replace"
affects:
  - 02-core-preview (plan 03 scroll sync, plan 04 export)

# Tech tracking
tech-stack:
  added:
    - "markdown-it 14.1.1 (UMD, bundled in assets/)"
    - "markdown-it-task-lists 2.1.0 (UMD, bundled — note: CDN serves 2.1.0 for 2.1.1 tag)"
    - "highlight.js 11.11.1 (core bundle with auto-detection, bundled)"
    - "github-markdown-css 5.9.0 light + dark variants (bundled)"
    - "hljs github + github-dark themes (bundled)"
  patterns:
    - "Assets served via WebView2 virtual host appassets.mdpreview (no CDN at runtime)"
    - "Paired CSS link swap for theme switching (md-theme + hljs-theme toggled atomically)"
    - "markdown-it core.ruler.push for source map injection (data-line on block tokens)"
    - "Stub-then-replace pattern: scrollToLine/exportHtml are no-ops until upgraded by later plans"

key-files:
  created:
    - "MarkdownPreview/assets/preview.html"
    - "MarkdownPreview/assets/markdown-it.min.js"
    - "MarkdownPreview/assets/markdown-it-task-lists.min.js"
    - "MarkdownPreview/assets/highlight.min.js"
    - "MarkdownPreview/assets/hljs-github.min.css"
    - "MarkdownPreview/assets/hljs-github-dark.min.css"
    - "MarkdownPreview/assets/github-markdown-light.css"
    - "MarkdownPreview/assets/github-markdown-dark.css"
  modified: []

key-decisions:
  - "markdown-it initialized with html:false (XSS mitigation T-02-06, ASVS L1)"
  - "markdownitTaskLists global confirmed from UMD file header — matches window.markdownitTaskLists"
  - "github-markdown-css 5.9.0 used (research confirms 5.9.0 is current stable; CLAUDE.md references 5.8.1)"
  - "scrollToLine() and exportHtml() are intentional stubs — Plans 02-03/02-04 replace them"

patterns-established:
  - "WebView2 message protocol: event.data is pre-parsed JS object (PostWebMessageAsJson)"
  - "Theme switching: atomic href swap on two <link> elements (no class toggle on body for color)"
  - "Custom CSS: inline <style id=custom-css> injected after base theme links (cascade wins)"

requirements-completed: [REND-03, REND-04, REND-06, THME-01, THME-02]

# Metrics
duration: 3min
completed: 2026-04-09
---

# Phase 2 Plan 02: Preview HTML and JS Asset Pipeline Summary

**markdown-it 14.1.1 + highlight.js 11.11.1 render pipeline in preview.html with GFM, YAML strip, paired theme swap, and custom CSS injection via WebView2 message protocol**

## Performance

- **Duration:** 3 min
- **Started:** 2026-04-09T15:03:10Z
- **Completed:** 2026-04-09T15:06:02Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- Downloaded all 7 JS/CSS assets at pinned versions from jsDelivr to assets/, verified UMD globals and content
- Created preview.html (10.5KB) with complete render pipeline: markdown-it GFM init, source map rule, YAML strip, renderMarkdown(), setTheme(), setCustomCss(), and WebView2 message dispatcher
- XSS mitigation: html:false on markdown-it prevents raw HTML passthrough (T-02-06, ASVS L1)
- Stub scrollToLine() and exportHtml() included — safe no-ops for Plans 02-03/02-04 to replace

## Task Commits

Each task was committed atomically:

1. **Task 1: Download JS/CSS assets** - `4543024` (chore)
2. **Task 2: Create preview.html** - `8b71a4d` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `MarkdownPreview/assets/preview.html` - Full JS rendering pipeline for WebView2
- `MarkdownPreview/assets/markdown-it.min.js` - markdown-it 14.1.1 UMD (123KB)
- `MarkdownPreview/assets/markdown-it-task-lists.min.js` - Task lists plugin 2.1.0 UMD (2.7KB)
- `MarkdownPreview/assets/highlight.min.js` - highlight.js 11.11.1 core (127KB)
- `MarkdownPreview/assets/hljs-github.min.css` - highlight.js GitHub light theme
- `MarkdownPreview/assets/hljs-github-dark.min.css` - highlight.js GitHub dark theme
- `MarkdownPreview/assets/github-markdown-light.css` - github-markdown-css 5.9.0 light (22KB)
- `MarkdownPreview/assets/github-markdown-dark.css` - github-markdown-css 5.9.0 dark (22KB)

## Decisions Made

- Used `html: false` on markdown-it for XSS protection (T-02-06 mitigate disposition)
- github-markdown-css 5.9.0 used (research confirmed 5.9.0 is current stable; CLAUDE.md shows 5.8.1 which is slightly older)
- markdown-it-task-lists CDN resolves 2.1.1 tag to 2.1.0 build — verified UMD global `markdownitTaskLists` present and correct
- Theme switching uses paired href swap on two `<link>` elements (not body class toggle) for clean cascade separation

## Deviations from Plan

None - plan executed exactly as written. The markdown-it-task-lists CDN version note (2.1.0 served for 2.1.1 tag) is cosmetic — functionality is identical.

## Known Stubs

| Stub | File | Line | Reason |
|------|------|------|--------|
| `scrollToLine()` no-op | preview.html | ~170 | Intentional — Plan 02-03 implements full data-line nearest-element scroll logic |
| `exportHtml()` no-op | preview.html | ~178 | Intentional — Plan 02-04 implements base64 image inlining and CSS collection |

These stubs are by plan design and documented in the plan's objective. They do not block this plan's goal (rendering pipeline).

## Issues Encountered

- Assets downloaded to main repo working directory instead of worktree — corrected by copying to worktree path before commit. Files are identical.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- preview.html is ready for Plan 02-01 (C++ side) to navigate WebView2 to it via the appassets.mdpreview virtual host
- Plan 02-03 (scroll sync) can replace scrollToLine() stub with full data-line implementation
- Plan 02-04 (HTML export) can replace exportHtml() stub with base64/CSS inlining implementation
- No blockers

---
*Phase: 02-core-preview*
*Completed: 2026-04-09*
