---
phase: 03-extended-rendering
plan: 04
subsystem: pdf-export
tags: [pdf, export, webview2, print-to-pdf, zoom-reset, menu-item]
dependency_graph:
  requires: [03-03]
  provides: [pdf-export-menu-item, triggerPdfExport, print-settings-us-letter]
  affects: [PreviewPanel, PluginDefinition]
tech_stack:
  added: []
  patterns:
    - ICoreWebView2_7::PrintToPdf for native PDF export without external dependencies
    - ICoreWebView2Environment6::CreatePrintSettings for page layout configuration
    - Zoom reset via postZoomToJs(1.0f) before PrintToPdf + restore in completion callback
    - m_printToPdfInProgress boolean guard for concurrent call prevention
    - exportMarkdownAsPdf() mirrors exportMarkdown() pattern exactly (NB_FUNC=3)
key_files:
  created: []
  modified:
    - MarkdownPreview/src/PreviewPanel.h
    - MarkdownPreview/src/PreviewPanel.cpp
    - MarkdownPreview/src/PluginDefinition.h
    - MarkdownPreview/src/PluginDefinition.cpp
decisions:
  - "ICoreWebView2Environment6 QueryInterface on stored m_environment (from Plan 03) — zero additional infrastructure needed"
  - "Zoom reset to 1.0 via postZoomToJs before PrintToPdf; restore in both completion callback and FAILED(hr) branch"
  - "Silent export (no dialog on success or failure) per D-06"
  - "Footer shows default URI + Page N of M — ICoreWebView2PrintSettings limitation, accepted per D-08"
metrics:
  duration: ~5min
  completed: "2026-04-09T18:23:00Z"
  tasks_completed: 1
  files_modified: 4
---

# Phase 03 Plan 04: PDF Export via WebView2 PrintToPdf Summary

PDF export pipeline added using ICoreWebView2_7::PrintToPdf with US Letter/portrait settings, filename header, Page N of M footer, zoom-reset/restore around the print call, and concurrent-export guard; wired via new "Export as PDF" (Ctrl+Shift+P) menu item as funcItems[2].

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Implement triggerPdfExport() in PreviewPanel and add PDF menu item | 65ca142 | PreviewPanel.h, PreviewPanel.cpp, PluginDefinition.h, PluginDefinition.cpp |

## What Was Built

### PreviewPanel.h
- New public method: `void triggerPdfExport()` (EXPT-02, EXPT-03)
- New private members: `bool m_printToPdfInProgress = false` and `float m_savedZoomForPdf = 1.0f`

### PreviewPanel.cpp — triggerPdfExport()
- Guards: returns immediately if `!m_webview || !m_webview2Initialized || m_currentFilePath.empty()` or `m_printToPdfInProgress`
- PDF output path: `m_currentFilePath` with `.md` extension replaced by `.pdf` (D-06: auto-save, overwrite, no dialog)
- Basename extraction from `m_currentFilePath` for `put_HeaderTitle` (D-08)
- `ICoreWebView2Environment6` QueryInterface on stored `m_environment` (set by Plan 03's `initWebView2`)
- `ICoreWebView2Environment6::CreatePrintSettings` creates the settings object
- Print settings applied: `put_Orientation(PORTRAIT)`, `put_ShouldPrintHeaderAndFooter(TRUE)`, `put_HeaderTitle(basename)`, `put_ShouldPrintBackgrounds(TRUE)`, 0.5/0.5/0.75/0.75 inch margins
- `ICoreWebView2_7` QueryInterface on `m_webview` for `PrintToPdf` method
- Zoom reset: `m_savedZoomForPdf = m_zoomLevel; postZoomToJs(1.0f)` before `PrintToPdf`
- `m_printToPdfInProgress = true` set immediately before `PrintToPdf` call
- Completion callback: resets `m_printToPdfInProgress = false`, calls `postZoomToJs(m_savedZoomForPdf)`, uses `UNREFERENCED_PARAMETER` for `errorCode`/`isSuccessful` (D-06: silent)
- `FAILED(hr)` branch: resets `m_printToPdfInProgress = false` and restores zoom on immediate failure

### PluginDefinition.h
- `const int NB_FUNC = 3` (was 2)
- `void exportMarkdownAsPdf()` declaration added

### PluginDefinition.cpp
- `static ShortcutKey pdfExportShortcut = { true, false, true, 'P' }` (Ctrl+Shift+P)
- `funcItems[2]` set to "Export as PDF" with `exportMarkdownAsPdf` and `pdfExportShortcut`
- `exportMarkdownAsPdf()` implementation: mirrors `exportMarkdown()` exactly — checks `isVisible()`, gets full current path, checks `.md` extension, calls `g_previewPanel.triggerPdfExport()`

## Deviations from Plan

None — plan executed exactly as written. NuGet packages were not pre-restored in the worktree (expected for a fresh worktree); restored via `nuget.exe restore` before build, zero errors confirmed.

## Known Stubs

None. PDF export is fully wired: menu item → exportMarkdownAsPdf() → triggerPdfExport() → ICoreWebView2_7::PrintToPdf → PDF file on disk.

## Threat Flags

No new security-relevant surface beyond the plan's threat model. All mitigations from T-03-15 through T-03-20 are implemented: pdfPath is C++-derived only, m_printToPdfInProgress guards concurrent calls, zoom values are fixed literals or C++-clamped floats.

## Self-Check: PASSED

Files exist:
- MarkdownPreview/src/PreviewPanel.h — FOUND (triggerPdfExport, m_printToPdfInProgress, m_savedZoomForPdf present)
- MarkdownPreview/src/PreviewPanel.cpp — FOUND (triggerPdfExport implementation present)
- MarkdownPreview/src/PluginDefinition.h — FOUND (NB_FUNC=3, exportMarkdownAsPdf declaration present)
- MarkdownPreview/src/PluginDefinition.cpp — FOUND (funcItems[2], pdfExportShortcut, exportMarkdownAsPdf implementation present)

Commits exist:
- 65ca142 — feat(03-04): implement PDF export via WebView2 PrintToPdf (EXPT-02, EXPT-03)

Build: zero errors confirmed (MSBuild Release x64, MarkdownPreview.dll produced successfully)
