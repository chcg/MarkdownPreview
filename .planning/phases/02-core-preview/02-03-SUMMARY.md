---
phase: 02-core-preview
plan: "03"
subsystem: plugin-core
tags: [cpp, webview2, scintilla, scroll-sync, virtual-host, image-resolution, javascript]

# Dependency graph
requires:
  - phase: 02-core-preview
    provides: "Plan 02-01 SCN_UPDATEUI stub wired; Plan 02-02 data-line attributes on block tokens; preview.html scrollToLine stub"

provides:
  - "scrollToLine(int line) public method on PreviewPanel — posts {type:scroll,line:N} JSON to JS"
  - "updateFileVirtualHost(const std::wstring& filePath) public method — maps file.mdpreview virtual host to file's parent dir"
  - "onScnUpdateUi() full implementation — SC_UPDATE_SELECTION|SC_UPDATE_CONTENT filter, SCI_GETCURRENTPOS + SCI_LINEFROMPOSITION caret retrieval"
  - "SC_UPDATE_CONTENT/SELECTION/V_SCROLL/H_SCROLL bitmask constants in Scintilla.h"
  - "JS scrollToLine() full implementation — querySelectorAll data-line scan, max-<=target, scrollIntoView smooth"
  - "JS rewriteImagePaths() — rewrites relative image paths to https://file.mdpreview/ before md.render()"
  - "updateFileVirtualHost called before renderMarkdown() in onBufferActivated() (Pitfall 3 timing guaranteed)"

affects:
  - 02-04-PLAN (HTML export — no dependency on scroll/image features, but same files modified)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "ICoreWebView2_3 Clear+Set pattern for virtual host remapping on file switch (avoids stale mapping)"
    - "SC_UPDATE_SELECTION|SC_UPDATE_CONTENT filter prevents scroll-event-triggered re-scroll (avoids feedback loop)"
    - "Regex image path rewrite in JS before md.render() — keeps markdown source clean, rewrite is transparent"
    - "COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS on file.mdpreview prevents cross-origin requests (T-02-10)"

key-files:
  created: []
  modified:
    - "MarkdownPreview/src/PreviewPanel.h"
    - "MarkdownPreview/src/PreviewPanel.cpp"
    - "MarkdownPreview/src/PluginDefinition.cpp"
    - "MarkdownPreview/include/Scintilla.h"
    - "MarkdownPreview/assets/preview.html"

key-decisions:
  - "SC_UPDATE_SELECTION|SC_UPDATE_CONTENT filter (not SC_UPDATE_V_SCROLL) prevents scroll-position-change events from triggering scroll — avoids infinite feedback loop"
  - "COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS chosen for file.mdpreview (T-02-10 mitigation) — images load from virtual host but JS cannot fetch arbitrary files via it"
  - "rewriteImagePaths() runs in JS before md.render() so markdown-it never sees relative paths — simpler than post-render DOM manipulation"
  - "NuGet packages restore required (packages dir was empty in both main repo and worktree) — downloaded nuget.exe and restored before first build"

patterns-established:
  - "Pattern: Virtual host remapping uses Clear-before-Set to avoid stale mappings when switching between .md files in different directories"
  - "Pattern: JS image path rewrite is pre-parse (before md.render) not post-render — prevents markdown-it from seeing relative paths that would fail"

requirements-completed: [SCRL-01, REND-05]

# Metrics
duration: 5min
completed: 2026-04-09
---

# Phase 02 Plan 03: Scroll Sync and Local Image Resolution Summary

**SCN_UPDATEUI caret-to-preview scroll sync via PostWebMessageAsJson + JS data-line nearest-element scrollIntoView, and ICoreWebView2_3 file.mdpreview virtual host for local image resolution with pre-parse regex path rewrite**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-04-09T15:12:12Z
- **Completed:** 2026-04-09T15:17:00Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Implemented full scroll sync pipeline: SCN_UPDATEUI filtered to SC_UPDATE_SELECTION|SC_UPDATE_CONTENT, caret line retrieved via SCI_GETCURRENTPOS + SCI_LINEFROMPOSITION, PreviewPanel::scrollToLine() posts {type:"scroll",line:N}, JS querySelectorAll data-line scan finds nearest element and calls scrollIntoView with behavior:smooth, block:start
- Implemented local image resolution: PreviewPanel::updateFileVirtualHost() uses ICoreWebView2_3 ClearVirtualHostNameToFolderMapping + SetVirtualHostNameToFolderMapping with DENY_CORS; called before renderMarkdown() in onBufferActivated() for correct timing (Pitfall 3); JS rewriteImagePaths() pre-parse regex rewrites relative paths to https://file.mdpreview/ while leaving http/https/data URIs unchanged
- Both x64 and x86 Release builds succeed with 0 errors

## Task Commits

Each task was committed atomically:

1. **Task 1: Scroll sync — C++ caret tracking + JS nearest-element scroll** - `1abc528` (feat)
2. **Task 2: Local image resolution — file.mdpreview virtual host + JS path rewrite** - `a079bcf` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `MarkdownPreview/src/PreviewPanel.h` - Added `scrollToLine(int line)` and `updateFileVirtualHost(const std::wstring& filePath)` public declarations
- `MarkdownPreview/src/PreviewPanel.cpp` - Implemented scrollToLine() (PostWebMessageAsJson with nlohmann JSON) and updateFileVirtualHost() (ICoreWebView2_3 Clear+Set with DENY_CORS)
- `MarkdownPreview/src/PluginDefinition.cpp` - Replaced onScnUpdateUi() stub with full implementation; added updateFileVirtualHost() call before renderMarkdown() in onBufferActivated()
- `MarkdownPreview/include/Scintilla.h` - Added SC_UPDATE_CONTENT, SC_UPDATE_SELECTION, SC_UPDATE_V_SCROLL, SC_UPDATE_H_SCROLL bitmask constants
- `MarkdownPreview/assets/preview.html` - Replaced scrollToLine() stub with full data-line nearest-element scroll; added rewriteImagePaths() function; wired into renderMarkdown() before md.render()

## Decisions Made

- SC_UPDATE_SELECTION|SC_UPDATE_CONTENT filter (not SC_UPDATE_V_SCROLL) prevents scroll-position-change events from re-triggering scroll, which would create an infinite feedback loop between C++ and JS.
- COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS on file.mdpreview (T-02-10 mitigation) — images load but cross-origin JS fetch to the virtual host is blocked.
- rewriteImagePaths() runs before md.render() (pre-parse) rather than as post-render DOM manipulation — simpler, no need to iterate rendered `<img>` elements.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] NuGet package restore required before first build**
- **Found during:** Task 1 (first build attempt)
- **Issue:** packages/ directory was empty in both the main repo and worktree — WebView2.targets missing, same root cause as Plan 02-01 deviation 4 (packages junction) but the junction pointed to an empty directory
- **Fix:** Downloaded nuget.exe to temp and ran `nuget.exe restore MarkdownPreview.sln` to populate packages/ in the main repo; the existing worktree junction to main repo packages/ then resolved correctly
- **Files modified:** (filesystem/NuGet operation — no source files)
- **Verification:** Build succeeded after restore
- **Committed in:** n/a (filesystem operation, not tracked in git)

---

**Total deviations:** 1 auto-fixed (Rule 3 blocking)
**Impact on plan:** Fix required for compilation. Identical root cause to Plan 02-01 deviation 4 — NuGet restore needed on first use of this development environment. No scope creep.

## Issues Encountered

- NuGet packages not restored in dev environment (packages/ was empty). Resolved by downloading nuget.exe and running restore. This will not recur now that packages/ is populated.
- Worktree junction for packages/ was created pointing to main repo packages/, but main repo packages/ was also empty. Needed to restore at the main repo level first, then the junction resolved correctly.

## Known Stubs

| Stub | File | Line | Reason |
|------|------|------|--------|
| `exportHtml()` no-op | preview.html | ~250 | Intentional — Plan 02-04 implements base64 image inlining and CSS collection |
| `handleJsMessage()` exportReady branch | PreviewPanel.cpp | ~413 | Intentional — Plan 02-04 adds saveExportedHtml() implementation |

These stubs are by prior plan design and do not affect this plan's goals.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Scroll sync complete: caret movement in .md files drives smooth preview scroll to nearest rendered element
- Local images complete: relative image paths in markdown render correctly; virtual host remapped on every file switch
- Plan 02-04 (HTML export) can now implement exportHtml() and handleJsMessage exportReady — the WebView2 message channel is wired and all imaging/scroll infrastructure is in place
- No blockers

---
*Phase: 02-core-preview*
*Completed: 2026-04-09*
