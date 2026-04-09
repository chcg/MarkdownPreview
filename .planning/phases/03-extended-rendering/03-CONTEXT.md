# Phase 3: Extended Rendering - Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Add advanced rendering features to the existing preview pipeline: KaTeX math (inline and block), Mermaid diagrams, footnotes, copy-to-clipboard on code blocks, zoom controls (Ctrl+/Ctrl-, 80–800%), and PDF export via WebView2 PrintToPdf. All new elements integrate with the existing dark/light theme system and the established message-passing architecture from Phase 2.

</domain>

<decisions>
## Implementation Decisions

### Diagram Error Handling
- **D-01:** When a fenced `mermaid` block fails to render (invalid syntax or Mermaid error), fall back to displaying the block as syntax-highlighted code — same treatment as any other fenced language. No styled error frame, no silent disappearance. User sees their source and can fix it.

### Copy Button
- **D-02:** Copy-to-clipboard button is icon-only (clipboard SVG), positioned in the top-right corner of each code block, visible on hover. Matches current GitHub conventions.
- **D-03:** No feedback animation after copy — silent copy. No checkmark swap, no "Copied!" label.

### Zoom Controls
- **D-04:** Keyboard-only zoom: Ctrl+= (zoom in), Ctrl+- (zoom out), Ctrl+0 (reset to 100%). No toolbar or Ctrl+scroll. Range: 80–800% per THME-04.
- **D-05:** Zoom level persisted globally in settings.json. Single zoom applies across all files. Loaded on WebView2 init, saved when changed.

### PDF Export
- **D-06:** Auto-save to same directory as the source .md file, using filename.pdf (replaces .md extension). No save dialog — mirrors HTML export behavior (Phase 2 D-10). If file already exists, overwrite without prompt.
- **D-07:** Paper size: US Letter, portrait orientation, fixed. No user configuration for paper size/orientation.
- **D-08:** Footer: filename (basename of source file) + "Page N of M". Header: empty. Satisfies EXPT-03.

### Claude's Discretion
- KaTeX error rendering for malformed math (standard KaTeX error display is acceptable)
- Mermaid initialization strategy (async init, lazy vs. eager)
- Copy button SVG icon design
- Zoom step size (e.g., 10% per keypress vs. 25%)
- PDF margins
- Exact keyboard shortcut implementation (Win32 WM_KEYDOWN interception vs. WebView2 AcceleratorKeyPressed)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase requirements
- `.planning/REQUIREMENTS.md` §Extended Rendering (XRND-01–XRND-04) — KaTeX math, Mermaid, footnotes, copy-to-clipboard
- `.planning/REQUIREMENTS.md` §Themes & Styling (THME-04) — Zoom controls 80–800%
- `.planning/REQUIREMENTS.md` §Export (EXPT-02–EXPT-03) — PDF export via WebView2, page numbers/headers/footers

### Existing implementation (Phase 2)
- `MarkdownPreview/assets/preview.html` — Full JS pipeline: message dispatch, renderMarkdown(), setTheme(), scrollToLine(), exportHtml(). New features bolt onto the same window.chrome.webview.addEventListener handler.
- `MarkdownPreview/src/PreviewPanel.cpp` — C++ side: PostWebMessageAsString patterns, WebMessageReceived handler, virtual host setup, export file write. PDF export will follow the same exportReady postMessage → C++ file write pattern used for HTML export.
- `MarkdownPreview/src/Settings.h` / `Settings.cpp` — Settings struct and JSON load/save. Extend with `zoomLevel` (float, default 1.0).
- `MarkdownPreview/src/PluginDefinition.h` / `PluginDefinition.cpp` — Menu item registration. Add "Export to PDF" menu item here.

### Tech stack decisions (CLAUDE.md)
- KaTeX 0.16.45 via `@vscode/markdown-it-katex` 1.1.2 (Microsoft-maintained fork — use this, not the abandoned original)
- Mermaid 11.14.0 (runtime diagram rendering)
- markdown-it-footnote 4.0.0 (official plugin)
- WebView2 `ICoreWebView2_7::PrintToPdf` / `PrintToPdfAsync` for PDF export (no third-party dependency)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `window.chrome.webview.addEventListener('message', handler)` in preview.html: The existing message dispatch switch handles `render`, `theme`, `scroll`, `export`. Add `zoom` case and the PDF export trigger here.
- `exportHtml()` async function: Pattern for JS → C++ file write via `postMessage({type:'exportReady', html:...})`. PDF export follows the same postMessage → C++ `PrintToPdf` chain.
- `setTheme(isDark)` and `hljs.highlightAll()` calls in `renderMarkdown()`: Mermaid and KaTeX initialization must hook in after the render call and respect the current theme state.
- Assets directory (`MarkdownPreview/assets/`): All JS/CSS bundles live here and are served via `appassets.mdpreview` virtual host. Add KaTeX (katex.min.js + katex.min.css), @vscode/markdown-it-katex, mermaid.min.js, and markdown-it-footnote here.

### Established Patterns
- Post-render hooks: `hljs.highlightAll()` is called after `preview.innerHTML = html`. KaTeX rendering (via markdown-it integration) happens in the markdown-it pipeline; Mermaid needs a post-render call to process `<pre class="mermaid">` or `<div class="mermaid">` blocks.
- Dark/light theming: Mermaid supports `theme: 'dark'` / `theme: 'default'` init config — must be toggled alongside `setTheme()`. KaTeX renders inline math as SVG/HTML that inherits body color — generally theme-agnostic.
- Virtual host for assets: All new JS/CSS files drop into `assets/` and reference via `https://appassets.mdpreview/`. No webpack or build step.
- C++ keyboard handling: Notepad++ plugins intercept WM_KEYDOWN in the plugin DLL. Alternatively, WebView2 fires `AcceleratorKeyPressed` events for keys caught in the WebView2 compositor — check which approach handles Ctrl+/- cleanly before choosing.

### Integration Points
- `renderMarkdown()` in preview.html: After `hljs.highlightAll()`, add Mermaid diagram processing and copy-button injection.
- `setTheme()` in preview.html: Extend to also update Mermaid theme when dark/light switches.
- `window.chrome.webview.addEventListener` handler: Add `zoom` message type → `document.body.style.zoom = msg.level`.
- C++ `PreviewPanel.cpp`: Add `PrintToPdf` call triggered by a new `exportPdf` message type from JS, or directly from a C++ menu handler.
- `Settings` struct: Add `zoomLevel` field (default 1.0), loaded in `onNppReady()`, saved on change.

</code_context>

<specifics>
## Specific Ideas

No specific requirements beyond the decisions above — open to standard approaches for Mermaid initialization, KaTeX integration, and the copy button implementation.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within Phase 3 scope.

</deferred>

---

*Phase: 03-extended-rendering*
*Context gathered: 2026-04-09*
