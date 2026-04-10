# Phase 4: Polish & Publication - Context

**Gathered:** 2026-04-10
**Status:** Ready for planning

<domain>
## Phase Boundary

Bidirectional navigation (clicking in the preview scrolls the editor to the corresponding source line), a clickable table of contents generated from document headings displayed as a fixed left sidebar, and publish-ready packaging as x86/x64 zips for the Notepad++ plugin list. This is the final v1.0 phase.

</domain>

<decisions>
## Implementation Decisions

### Click-to-Editor Navigation (SCRL-02)

- **D-01:** Any click on any element in the preview navigates the editor — not just headings. The existing `data-line` attribute infrastructure from Phase 2 is used. A JS click listener on `#preview` (or `document`) fires `postMessage({type: "lineClick", line: N})` with the nearest ancestor's `data-line` value.
- **D-02:** Silent caret move only — no flash, no highlight, no animation. Editor caret moves to the source line and scrolls into view. Consistent with Phase 2/3 export behavior (no confirmation needed).
- **D-03:** Editor window receives focus (`SetFocus()` on the Scintilla HWND) after navigation. The user can immediately start editing at that location.
- **D-04:** C++ receives the `lineClick` message in the existing `WebMessageReceived` handler (currently handling `exportReady`). It uses `SCI_GOTOPOS`/`SCI_GOTOLINE` + `SCI_ENSUREVISIBLE` + `SCI_SCROLLCARET` to move the caret, then calls `SetFocus(hSci)`.

### Table of Contents (SCRL-03)

- **D-05:** Fixed left sidebar within the WebView2 area. The preview content flows to the right of the sidebar. Width: ~200px. No toggle button needed — it is always visible when headings exist.
- **D-06:** Sidebar is hidden (display: none) when the document has no headings (no h1–h6 elements). In that case, the content takes full width. Re-evaluated on every render.
- **D-07:** TOC is generated in JS from the rendered heading elements after each `renderMarkdown()` call. It uses the same `data-line` attributes on headings to know source line numbers.
- **D-08:** TOC entry clicks navigate both the preview (smooth scroll to the heading) AND the editor (same `lineClick` mechanism as D-01/D-04). All navigation in the preview uses the same editor-jump behavior.
- **D-09:** Active heading tracking — as the editor caret moves (driven by the existing `scroll` message from C++), the TOC highlights the nearest heading above the current viewport position. CSS class `toc-active` applied to the matching TOC entry. This uses the existing `scrollToLine` infrastructure from Phase 2.

### Installer & Packaging (INFR-04)

- **D-10:** Distribution format: two .zip files — `MarkdownPreview_v1.0.0_x64.zip` and `MarkdownPreview_v1.0.0_x86.zip`. Each contains: `MarkdownPreview.dll` + `assets/` directory (all bundled JS/CSS). This is exactly what the Notepad++ plugin list requires.
- **D-11:** Version format: semantic versioning `v1.0.0`. Embedded in the DLL's `VS_VERSION_INFO` resource block (FILEVERSION and PRODUCTVERSION) and in a `manifest.json` file shipped alongside the zip (for plugin list metadata).
- **D-12:** A `scripts/package.ps1` PowerShell script automates zip creation from the Release build outputs. It copies x86/x64 Release DLLs + assets into properly structured directories and zips them. Repeatable, no manual steps.

### Claude's Discretion

- TOC sidebar exact CSS styling (font size, indent per heading level, scroll behavior)
- Active heading highlight CSS style (color, bold, indicator)
- TOC heading truncation if text is very long
- `lineClick` message field names (e.g., `line` vs `lineNumber`)
- Scintilla navigation message sequence (SCI_GOTOLINE vs SCI_GOTOPOS + SCI_ENSUREVISIBLE combination)
- `manifest.json` schema (match Notepad++ plugin list format if documented)
- Scripts directory location (`scripts/` at repo root)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase requirements
- `.planning/REQUIREMENTS.md` §Scroll & Navigation (SCRL-02, SCRL-03) — Click navigation and TOC
- `.planning/REQUIREMENTS.md` §Plugin Infrastructure (INFR-04) — Publish-ready packaging

### Existing implementation (Phases 1–3)
- `MarkdownPreview/assets/preview.html` — Full JS pipeline: `data-line` injection, `scrollToLine()`, `window.chrome.webview.addEventListener` message handler, `window.chrome.webview.postMessage` (used for exportReady — add lineClick here). TOC and click handler bolt onto the same architecture.
- `MarkdownPreview/src/PreviewPanel.cpp` — `WebMessageReceived` handler (line ~820): add `lineClick` case here. `NPPM_GETCURRENTSCINTILLA` + `SCI_GETLENGTH` pattern shows how to get the Scintilla handle — use same pattern for `SCI_GOTOLINE`.
- `MarkdownPreview/src/PreviewPanel.cpp` `scrollToLine()` (~line 616): existing editor-to-preview scroll message. Click navigation is the reverse flow.
- `MarkdownPreview/src/PluginDefinition.h` — Version constants will be added here for DLL resource embedding.

### Tech stack
- No new JavaScript libraries needed — TOC and click navigation are pure DOM/JS using existing `data-line` attributes.
- No new C++ dependencies — Scintilla navigation uses standard `SendMessage(hSci, SCI_GOTOLINE, ...)` already available.
- PowerShell 5.1+ for `package.ps1` (ships with Windows 10/11, no install needed).

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `data-line` attributes on block elements (h1–h6, p, pre, table, li): Already injected by markdown-it render plugin in `preview.html`. TOC uses these directly — no extra annotation pass needed.
- `window.chrome.webview.postMessage(JSON)`: Already in preview.html for `exportReady`. Add `{type: "lineClick", line: N}` using the same call.
- `WebMessageReceived` handler in `PreviewPanel.cpp` (~line 390): Switch statement currently handles `exportReady`. Add `lineClick` case here.
- `NPPM_GETCURRENTSCINTILLA` + `nppData._scintillaMainHandle/_scintillaSecondHandle`: Already used in `getCurrentText()`. Reuse to get `hSci` for navigation.
- `scrollToLine()` in `PreviewPanel.cpp`: Sends `{type: "scroll", line: N}` to JS. The reverse direction (JS→C++ via lineClick) mirrors this contract.
- `renderMarkdown()` in `preview.html`: Called after every render. TOC generation hooks in here, after `preview.innerHTML = html`.

### Established Patterns
- JS→C++ messaging: `window.chrome.webview.postMessage({type: "...", ...})` then C++ switch on `type`.
- C++→JS messaging: `PostWebMessageAsJson(wjson.c_str())` with `{type, ...}` payload.
- Post-render hooks: `hljs.highlightAll()` and Mermaid processing are called after `preview.innerHTML = html`. TOC generation follows this same pattern.

### Integration Points
- `renderMarkdown()` in preview.html: After setting `preview.innerHTML`, call `buildToc()` to (re)generate the sidebar.
- `setTheme()` in preview.html: TOC sidebar should inherit the dark/light CSS variables already applied to `<body>`.
- `window.chrome.webview.addEventListener('message', ...)` in preview.html: Add handler for `{type: "scroll", line: N}` to update TOC active state (already sent from `scrollToLine()`).
- `PreviewPanel::onWebMessageReceived()`: Add `lineClick` → `SCI_GOTOLINE` + `SetFocus(hSci)`.
- `MarkdownPreview.vcxproj`: Add `VS_VERSION_INFO` resource (`.rc` file) for DLL versioning.

</code_context>

<specifics>
## Specific Ideas

- TOC sidebar CSS: narrow fixed-width panel on the left, flex layout (`display: flex` on `<body>` or outer wrapper, TOC as `<nav>` and content as `<article>`).
- Active TOC entry: determined by which heading has the highest `data-line` ≤ current scroll line. Same logic as `scrollToLine()` nearest-match algorithm.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within Phase 4 scope.

</deferred>

---

*Phase: 04-polish-publication*
*Context gathered: 2026-04-10*
