---
phase: 03-extended-rendering
fixed_at: 2026-04-09T19:15:00Z
review_path: .planning/phases/03-extended-rendering/03-REVIEW.md
iteration: 1
findings_in_scope: 6
fixed: 6
skipped: 0
status: all_fixed
---

# Phase 03: Code Review Fix Report

**Fixed at:** 2026-04-09T19:15:00Z
**Source review:** .planning/phases/03-extended-rendering/03-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 6 (1 Critical, 5 Warning)
- Fixed: 6
- Skipped: 0

## Fixed Issues

### CR-01: PDF export uses user-controlled file path without sanitization — path-injection risk

**Files modified:** `MarkdownPreview/src/PreviewPanel.cpp`
**Commit:** `90ca69b`
**Applied fix:** Added a directory-equality check immediately after computing `pdfPath` in `triggerPdfExport()`. Extracts the directory component of both `pdfPath` and `m_currentFilePath` using `find_last_of(L"\\/")` and compares them with `_wcsicmp`. Returns early without calling `PrintToPdf` if they differ, preventing any path traversal from writing a PDF outside the source file's directory.

### WR-01: Mermaid `result.svg` assigned to `innerHTML` — unsanitized SVG injection

**Files modified:** `MarkdownPreview/assets/preview.html`
**Commit:** `9f6c1bf`
**Applied fix:** Three coordinated changes: (1) replaced `container.innerHTML = result.svg` in `renderMermaidDiagrams()` with a `DOMParser`-based approach that parses the SVG, strips any `<script>` elements, then inserts the root SVG element via `document.importNode`; (2) added `securityLevel: 'strict'` to the initial `_mermaid.initialize()` call at startup; (3) added `securityLevel: 'strict'` to the `_mermaid.initialize()` call inside `setTheme()` so it is preserved on every theme change.

### WR-02: Zoom level state diverges between `m_zoomLevel` and `g_settings.zoomLevel` after PDF export

**Files modified:** `MarkdownPreview/src/PreviewPanel.cpp`
**Commit:** `bb7da92`
**Applied fix:** Added an early-return guard inside the `AcceleratorKeyPressed` lambda, placed after `args->put_Handled(TRUE)` (so WebView2's built-in zoom is still suppressed) but before any zoom computation or persistence. The guard checks `m_printToPdfInProgress` and returns `S_OK` immediately if a PDF export is in flight, preventing `m_zoomLevel` from being modified during the zoom-reset/restore window.

### WR-03: `applyInitialZoom` is called before the navigation-complete event — JS may not be ready

**Files modified:** `MarkdownPreview/src/PreviewPanel.cpp`, `MarkdownPreview/src/PreviewPanel.h`
**Commit:** `657430d`
**Applied fix:** Added `EventRegistrationToken m_navigationCompletedToken` to `PreviewPanel.h`. In `initWebView2()`, removed the direct inline calls to `applyInitialZoom` and the `m_pendingFilePath` dispatch block that ran immediately after `Navigate()`; replaced them with a `NavigationCompleted` event handler that posts the initial zoom and dispatches any pending render only after `preview.html` has fully loaded. Added token unregistration in `destroy()` to mirror the pattern used for the existing `AcceleratorKeyPressed` and `WebMessageReceived` tokens.

### WR-04: `get_Settings` return value is unchecked — potential null-pointer dereference

**Files modified:** `MarkdownPreview/src/PreviewPanel.cpp`
**Commit:** `04a634d`
**Applied fix:** Wrapped the three `settings->put_*` calls in a block that first checks `SUCCEEDED(m_webview->get_Settings(&settings)) && settings`. The `settings` pointer is now only dereferenced when the call succeeds and the returned pointer is non-null, eliminating the access violation risk if `get_Settings` fails during a race with webview teardown.

### WR-05: `WM_NOTIFY` SysLink handler passes unvalidated URL to `ShellExecuteW`

**Files modified:** `MarkdownPreview/src/PreviewPanel.cpp`
**Commit:** `a4916d3`
**Applied fix:** Restructured the `WM_NOTIFY` handler to retrieve `self` from `GWLP_USERDATA` (matching the pattern already used in `WM_SIZE` and `WM_TIMER`). Added a guard that only proceeds if `nmhdr->hwndFrom == self->m_hFallback`, ensuring only notifications from the known SysLink control are processed. Added a `wcsncmp` check that restricts `ShellExecuteW` to URLs beginning with `https://`, blocking `file://`, `ms-msdt://`, and other dangerous protocol handlers.

---

_Fixed: 2026-04-09T19:15:00Z_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
