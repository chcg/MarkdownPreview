---
phase: 02-core-preview
plan: "04"
subsystem: plugin-core
tags: [cpp, webview2, javascript, html-export, base64, file-io, nlohmann-json, notepad-plus-plus]

# Dependency graph
requires:
  - phase: 02-core-preview
    provides: "Plan 02-01 handleJsMessage exportReady stub + add_WebMessageReceived channel; Plan 02-02 exportHtml() stub in preview.html"

provides:
  - "exportHtml() fully async: collects link[rel=stylesheet] CSS via fetch, includes custom-css style text, clones #preview div, base64-encodes local images via fetch+FileReader, builds standalone HTML, sends {type:exportReady} to C++"
  - "blobToDataUrl() helper: converts Blob to base64 data URI via FileReader Promise"
  - "NB_FUNC = 2: Export as HTML menu item at index 1 with Ctrl+Shift+E shortcut"
  - "exportMarkdown() menu command: guards on isVisible() + .md extension, calls triggerExport()"
  - "PreviewPanel::triggerExport(): derives .html path from m_currentFilePath (.md replaced), sets m_exportFilePath, posts {type:export} JSON"
  - "PreviewPanel::saveExportedHtml(): writes UTF-8 with BOM (0xEF 0xBB 0xBF) to m_exportFilePath, silent overwrite"
  - "handleJsMessage() exportReady branch: fully implemented — extracts html field, calls saveExportedHtml()"

affects:
  - "Phase 3 (advanced rendering) — no dependencies on export but same files used"

# Tech tracking
tech-stack:
  added:
    - "#include <fstream> added to PreviewPanel.cpp for std::ofstream"
  patterns:
    - "Export round-trip: C++ posts {type:export} JSON -> JS serializes DOM -> JS postMessages {type:exportReady, html:...} -> C++ writes file (no blocking, fully async)"
    - "JS DOM cloneNode() before image mutation: avoids mutating live preview while encoding images"
    - "Base64 image encoding: fetch(src) -> resp.blob() -> FileReader.readAsDataURL() chained with Promise"
    - "UTF-8 BOM written before HTML content (0xEF 0xBB 0xBF) for maximum browser compatibility on Windows"
    - "Export path derivation: rfind('.') on m_currentFilePath, replace suffix with .html — no JS involvement in path (T-02-15 mitigation)"

key-files:
  created: []
  modified:
    - "MarkdownPreview/assets/preview.html"
    - "MarkdownPreview/src/PluginDefinition.h"
    - "MarkdownPreview/src/PluginDefinition.cpp"
    - "MarkdownPreview/src/PreviewPanel.h"
    - "MarkdownPreview/src/PreviewPanel.cpp"

key-decisions:
  - "Export path derived entirely in C++ from m_currentFilePath (set by Notepad++ NPPM_GETFULLCURRENTPATH) — JS never provides the output path (T-02-15 path traversal mitigation)"
  - "Silent overwrite per D-10 — no file-exists check or user prompt in Phase 2"
  - "UTF-8 BOM included for maximum browser compatibility on Windows (Edge, Chrome, and IE all respect BOM)"
  - "JS cloneNode(true) on #preview before image mutation prevents any visible flicker or live DOM corruption during export"

patterns-established:
  - "Pattern: Export trigger is one-way: C++ sends {type:export}, JS responds with {type:exportReady, html:...}. The file path is never sent to JS — only C++ knows where to write."
  - "Pattern: m_exportFilePath set just before trigger and cleared after write — acts as a one-shot output path slot."

requirements-completed: [EXPT-01]

# Metrics
duration: 15min
completed: 2026-04-09
---

# Phase 02 Plan 04: HTML Export End-to-End Summary

**Async JS DOM serialization with inlined CSS and base64 images wired to C++ UTF-8 file write via WebView2 postMessage round-trip; Export as HTML menu item (Ctrl+Shift+E) triggers the full pipeline**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-04-09T15:25:00Z
- **Completed:** 2026-04-09T15:40:00Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Replaced exportHtml() stub in preview.html with full async implementation: collects CSS from all link[rel=stylesheet] elements via fetch, includes custom-css inline style, clones #preview div, converts local image src to base64 data URIs (fetch + FileReader), builds standalone HTML document, sends {type:exportReady, html:...} to C++ via window.chrome.webview.postMessage
- Added Export as HTML menu item at index 1 (NB_FUNC = 2) with Ctrl+Shift+E shortcut; exportMarkdown() guards on panel visibility and .md extension then calls triggerExport()
- Implemented triggerExport() (derives .html export path, sets m_exportFilePath, posts {type:export}), saveExportedHtml() (writes UTF-8 with BOM), and completed handleJsMessage() exportReady branch; both x64 and x86 Release builds succeed with 0 errors

## Task Commits

Each task was committed atomically:

1. **Task 1: JS exportHtml() — inline CSS + base64 images + DOM serialization** - `91c9f3b` (feat)
2. **Task 2: C++ export menu item + file write from exportReady message** - `c587fce` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `MarkdownPreview/assets/preview.html` - exportHtml() stub replaced with full async implementation; blobToDataUrl() helper added
- `MarkdownPreview/src/PluginDefinition.h` - NB_FUNC changed from 1 to 2; exportMarkdown() declared
- `MarkdownPreview/src/PluginDefinition.cpp` - exportShortcut (Ctrl+Shift+E) added; funcItems[1] initialized with Export as HTML; exportMarkdown() implemented
- `MarkdownPreview/src/PreviewPanel.h` - triggerExport() added to public section; saveExportedHtml() added to private section; m_exportFilePath member added
- `MarkdownPreview/src/PreviewPanel.cpp` - Added #include <fstream>; triggerExport() implemented; saveExportedHtml() implemented with UTF-8 BOM; handleJsMessage() exportReady branch completed

## Decisions Made

- Export path derived entirely in C++ (m_currentFilePath via NPPM_GETFULLCURRENTPATH, .md replaced with .html). JS never provides the path — eliminates T-02-15 path traversal risk.
- Silent overwrite per D-10 — no prompt, no backup. Phase 2 design decision.
- UTF-8 BOM (0xEF 0xBB 0xBF) prepended to output file for maximum compatibility with Windows browsers.
- JS cloneNode(true) on #preview before image src mutation prevents any visible flicker in the live preview panel.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] NuGet package restore required before build**
- **Found during:** Task 2 (first build attempt)
- **Issue:** packages/ directory was empty — WebView2.targets missing. Same root cause as Plans 02-01 and 02-03 (NuGet packages not persisted in dev environment)
- **Fix:** Downloaded nuget.exe to %TEMP% and ran `nuget.exe restore MarkdownPreview.sln` against the packages/ directory in the main repo
- **Files modified:** (filesystem/NuGet operation — no source files)
- **Verification:** WebView2.targets present after restore; both builds succeeded
- **Committed in:** n/a (filesystem operation, not tracked in git)

---

**Total deviations:** 1 auto-fixed (Rule 3 blocking)
**Impact on plan:** Fix required for compilation. Recurring NuGet restore issue across Phase 2 plans — dev environment does not persist packages/. No scope creep.

## Issues Encountered

- NuGet packages not restored (packages/ empty) — same issue as Plans 02-01 and 02-03. Resolved identically by running nuget.exe restore. This is a dev-environment bootstrapping concern, not a code issue.

## Known Stubs

None — all stubs from Plans 02-01 and 02-02 for the export feature are now fully implemented.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- HTML export feature is end-to-end complete: menu item → JS serialization → C++ file write → standalone .html
- Phase 2 (02-core-preview) is now complete — all four plans (notification pipeline, preview.html, scroll sync, HTML export) implemented
- Phase 3 (advanced rendering) can proceed: KaTeX math, Mermaid diagrams, footnotes, PDF export are the next deliverables
- No blockers

## Self-Check: PASSED

- FOUND: MarkdownPreview/assets/preview.html
- FOUND: MarkdownPreview/src/PluginDefinition.h
- FOUND: MarkdownPreview/src/PluginDefinition.cpp
- FOUND: MarkdownPreview/src/PreviewPanel.h
- FOUND: MarkdownPreview/src/PreviewPanel.cpp
- FOUND: .planning/phases/02-core-preview/02-04-SUMMARY.md (worktree root)
- FOUND commit: 91c9f3b (Task 1 - JS exportHtml)
- FOUND commit: c587fce (Task 2 - C++ export menu + file write)

---
*Phase: 02-core-preview*
*Completed: 2026-04-09*
