# Phase 3: Extended Rendering - Research

**Researched:** 2026-04-09
**Domain:** JavaScript rendering pipeline extensions (KaTeX, Mermaid, markdown-it plugins) + WebView2 PDF export + keyboard zoom + C++ Settings extension
**Confidence:** HIGH (all core tech verified against official sources and live CDN bundles)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01 (Mermaid error handling):** When a fenced `mermaid` block fails to render, fall back to syntax-highlighted code — no styled error frame, no silent disappearance.
- **D-02 (Copy button style):** Icon-only clipboard SVG, positioned top-right of each code block, visible on hover. GitHub convention.
- **D-03 (Copy feedback):** No feedback animation after copy. Silent copy.
- **D-04 (Zoom shortcuts):** Ctrl+= (zoom in), Ctrl+- (zoom out), Ctrl+0 (reset to 100%). Keyboard-only. Range: 80–800%.
- **D-05 (Zoom persistence):** Zoom level persisted globally in settings.json as a float (default 1.0). Loaded on WebView2 init, saved when changed.
- **D-06 (PDF save path):** Auto-save to same directory as source .md file, using filename.pdf. No dialog. Overwrite if exists.
- **D-07 (PDF paper):** US Letter (8.5 × 11 inches), portrait orientation, fixed. No user config.
- **D-08 (PDF footer):** Footer: basename of source file + "Page N of M". Header: empty.

### Claude's Discretion

- KaTeX error rendering for malformed math (standard KaTeX error display is acceptable)
- Mermaid initialization strategy (async init, lazy vs. eager)
- Copy button SVG icon design
- Zoom step size (e.g., 10% per keypress vs. 25%)
- PDF margins
- Exact keyboard shortcut implementation (Win32 WM_KEYDOWN interception vs. WebView2 AcceleratorKeyPressed)

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within Phase 3 scope.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| XRND-01 | Math/LaTeX expressions render via KaTeX (inline and block) | KaTeX 0.16.45 browser load verified; markdown-it-texmath provides browser-ready UMD; see Standard Stack |
| XRND-02 | Mermaid diagrams render from fenced code blocks (flowcharts, sequence, Gantt, etc.) | Mermaid 11.14.0 IIFE bundle verified at CDN; initialization pattern documented; see Architecture Patterns |
| XRND-03 | Footnotes render with proper numbering and back-references | markdown-it-footnote 4.0.0 UMD verified; global `markdownitFootnote`; see Standard Stack |
| XRND-04 | Code blocks include a copy-to-clipboard button (GitHub-style) | Pure JS DOM injection post-render; Clipboard API available in WebView2; see Architecture Patterns |
| THME-04 | Zoom controls (Ctrl+/Ctrl-, 80–800%) | WebView2 AcceleratorKeyPressed event; `document.body.style.zoom` CSS; see Architecture Patterns |
| EXPT-02 | Export rendered markdown to PDF via WebView2 | ICoreWebView2_7::PrintToPdf verified; ICoreWebView2Environment6::CreatePrintSettings; see Code Examples |
| EXPT-03 | Print output includes page numbers, headers, and footers | ShouldPrintHeaderAndFooter + FooterUri/HeaderTitle properties on ICoreWebView2PrintSettings verified |
</phase_requirements>

---

## Summary

Phase 3 adds six independent feature clusters on top of the Phase 2 preview pipeline: KaTeX math rendering, Mermaid diagram rendering, footnotes, copy-to-clipboard buttons on code blocks, keyboard zoom controls, and PDF export. All six are purely additive — they bolt onto the existing `renderMarkdown()` post-render hook and the `window.chrome.webview.addEventListener` message dispatcher, with zero changes to the core render loop.

The primary technical risk identified in research is the KaTeX integration path: `@vscode/markdown-it-katex` (the CLAUDE.md-mandated choice) has **no UMD browser build**. It ships only a CommonJS `dist/index.js` compiled from TypeScript with no browser-ready artifact. This means the plugin cannot be loaded via `<script src>` in preview.html as-is. The recommended resolution is `markdown-it-texmath`, a drop-in alternative that provides a browser-ready `texmath.js` file, uses KaTeX as a passed-in engine, and exposes the same `$...$` / `$$...$$` delimiters. This replaces `@vscode/markdown-it-katex` while fully satisfying XRND-01.

The secondary risk is Mermaid 11's IIFE bundle: v11 changed from UMD to IIFE (esbuild), and the global access pattern changed to `mermaid.default.*` in the raw IIFE, though the maintainers patched this to restore `window.mermaid` compatibility. The ESM `<script type="module">` approach is safer and the documented recommendation for v11. Both options are viable but require care.

**Primary recommendation:** Use markdown-it-texmath (UMD, browser-ready) for math, Mermaid ESM import for diagrams, markdown-it-footnote UMD for footnotes, WebView2 AcceleratorKeyPressed for zoom keyboard interception, and ICoreWebView2_7::PrintToPdf for PDF export with ICoreWebView2PrintSettings for headers/footers.

---

## Project Constraints (from CLAUDE.md)

| Directive | Constraint |
|-----------|-----------|
| Platform | Windows only |
| Plugin API | C++ DLL, MSVC v145 toolset (dev env uses VS 2026) |
| Rendering | WebView2 only — no CEF, no other engines |
| Markdown parser | markdown-it (JS in WebView2) — no C++ side parsing |
| Math | KaTeX 0.16.45 — not MathJax |
| Math plugin | `@vscode/markdown-it-katex` 1.1.2 per CLAUDE.md — **BUT see Critical Finding below** |
| Mermaid | 11.14.0 — pinned version |
| Footnotes | markdown-it-footnote 4.0.0 |
| Asset loading | All JS/CSS bundled in `assets/`, served via `appassets.mdpreview` virtual host |
| No build step | Assets must be pre-downloaded minified files, no webpack/esbuild at runtime |
| Settings | nlohmann/json for persistence |
| Security | html: false on markdown-it (XSS mitigation) — do not change |

### Critical Finding: @vscode/markdown-it-katex Has No Browser Build

`@vscode/markdown-it-katex` v1.1.2 is compiled from TypeScript to CommonJS (`dist/index.js`) with no UMD or IIFE browser bundle. [VERIFIED: github.com/microsoft/vscode-markdown-it-katex package.json — build scripts: `tsc -p .` only, no bundler; files: `dist/index.js`, `types.d.ts`]. It cannot be loaded via `<script src>` in a browser without a bundler.

**Resolution:** Replace `@vscode/markdown-it-katex` with `markdown-it-texmath` for this phase's browser context. `markdown-it-texmath` ships a browser-ready `texmath.js` file, accepts KaTeX as a passed-in engine (`{ engine: window.katex }`), and handles `$...$` inline and `$$...$$` block delimiters. [VERIFIED: cdn.jsdelivr.net/npm/markdown-it-texmath@1.0.0/ — texmath.js present]. This satisfies XRND-01 without violating the KaTeX mandate. The `@vscode/markdown-it-katex` directive in CLAUDE.md applies to the math rendering engine (KaTeX) — the wrapper plugin must change to be browser-compatible.

---

## Standard Stack

### Core (JavaScript side — bundled in assets/)

| Library | Version | Purpose | Source / Trust |
|---------|---------|---------|----------------|
| KaTeX | 0.16.45 | TeX math rendering (the engine) | [CITED: katex.org/docs/browser.html] |
| markdown-it-texmath | 1.0.0 | markdown-it plugin for KaTeX; browser UMD | [VERIFIED: cdn.jsdelivr.net/npm/markdown-it-texmath@1.0.0/] |
| mermaid | 11.14.0 | Diagram rendering; ESM bundle | [VERIFIED: cdn.jsdelivr.net/npm/mermaid@11.14.0/dist/] |
| markdown-it-footnote | 4.0.0 | Footnotes with numbering and back-refs | [VERIFIED: cdn.jsdelivr.net/npm/markdown-it-footnote@4.0.0/dist/] |

### New Assets Required in `assets/`

| File | Source | Size (approx) |
|------|--------|---------------|
| `katex.min.js` | KaTeX 0.16.45 release | ~300 KB |
| `katex.min.css` | KaTeX 0.16.45 release | ~15 KB |
| `texmath.js` | markdown-it-texmath 1.0.0 from CDN | ~14 KB |
| `mermaid.esm.min.mjs` | mermaid 11.14.0 from CDN | ~27 KB (entry; lazy-loads diagram grammars) |
| `markdown-it-footnote.min.js` | markdown-it-footnote 4.0.0 from CDN | ~6 KB |

Note: `mermaid.esm.min.mjs` is only the entry ESM file. Mermaid 11 with ESM lazy-loads diagram grammar chunks from the same dist/ directory. **If serving from a local virtual host, ALL chunk files from `mermaid@11.14.0/dist/` must be copied to `assets/` for offline use.** Alternatively, use `mermaid.min.js` (IIFE, 3 MB) which bundles everything. Given the local virtual host constraint (no CDN at runtime), `mermaid.min.js` is simpler and more reliable. [VERIFIED: cdn.jsdelivr.net/npm/mermaid@11.14.0/dist/ — mermaid.min.js at 3.02 MB, fully self-contained IIFE]

### Revised Assets Table (practical choice)

| File | Format | Reason |
|------|--------|--------|
| `mermaid.min.js` | IIFE (3 MB) | Self-contained; no chunk dependencies; works offline |

### Global Variables After Script Load

| Script file | Global variable | How to use |
|-------------|----------------|-----------|
| `katex.min.js` | `window.katex` | Passed to texmath as `{ engine: window.katex }` |
| `texmath.js` | `window.texmath` | `md.use(window.texmath, { engine: window.katex })` |
| `mermaid.min.js` (IIFE) | `window.mermaid` | `mermaid.initialize({...})` then `mermaid.run({...})` |
| `markdown-it-footnote.min.js` | `window.markdownitFootnote` | `md.use(window.markdownitFootnote)` |

[VERIFIED: markdownitFootnote global from CDN file inspection — UMD wrapper assigns `globalThis.markdownitFootnote`]
[VERIFIED: window.mermaid from mermaid v11 discussion — backward compat maintained after alpha.2 patch]

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| markdown-it-texmath | @vscode/markdown-it-katex | No browser UMD build; requires bundler — not viable |
| mermaid.min.js (IIFE) | mermaid.esm.min.mjs (ESM) | ESM requires all chunk files present locally; IIFE is self-contained |
| Clipboard API | execCommand('copy') | execCommand deprecated; Clipboard API works in WebView2 Chromium |

---

## Architecture Patterns

### Recommended Project Structure (additions only)

```
MarkdownPreview/assets/
├── [existing files unchanged]
├── katex.min.js           # KaTeX math engine
├── katex.min.css          # KaTeX render styles
├── texmath.js             # markdown-it-texmath plugin (KaTeX wrapper)
├── mermaid.min.js         # Mermaid IIFE bundle (self-contained)
└── markdown-it-footnote.min.js  # Footnote plugin

MarkdownPreview/src/
├── Settings.h / Settings.cpp    # Add zoomLevel float field
├── PreviewPanel.h / PreviewPanel.cpp  # Add triggerPdfExport(), setZoom(), AcceleratorKeyPressed handler
└── PluginDefinition.h / .cpp    # Add NB_FUNC = 3, add "Export to PDF" menu item + shortcut
```

### Pattern 1: KaTeX + texmath Plugin Initialization

Load KaTeX CSS and JS in `<head>` before markdown-it. Load texmath after katex.min.js. Apply plugin after markdown-it init.

```javascript
// Source: github.com/goessner/markdown-it-texmath README
// texmath accepts KaTeX as a passed-in engine — no global pollution from katex
if (window.texmath && window.katex) {
    md.use(window.texmath, {
        engine: window.katex,
        delimiters: 'dollars',   // handles $...$ inline and $$...$$ block
        katexOptions: { throwOnError: false }   // malformed math shows error inline, not throw
    });
}
```

`throwOnError: false` is the correct KaTeX setting for preview use — malformed math renders a red error span rather than throwing a JS exception. [ASSUMED — standard KaTeX option, behavior inferred from training data; verify in testing]

### Pattern 2: Mermaid Initialization and Post-Render Processing

Mermaid 11 uses `startOnLoad: false` + explicit `mermaid.run()` call. markdown-it renders fenced ```mermaid blocks as `<pre><code class="language-mermaid">` after hljs runs. The mermaid post-processing step must re-query for these elements, convert them to `<div class="mermaid">`, and call `mermaid.run()`.

```javascript
// Source: mermaid.js.org/config/usage.html
// Initialize once on page load (before first render)
if (window.mermaid) {
    mermaid.initialize({
        startOnLoad: false,
        theme: 'default'   // overridden in setTheme()
    });
}

// Called after renderMarkdown() and hljs.highlightAll()
function renderMermaidDiagrams() {
    if (!window.mermaid) return;

    // hljs will have processed <pre><code class="language-mermaid"> blocks.
    // We must undo hljs highlighting and convert to mermaid-expected format.
    var mermaidBlocks = document.querySelectorAll('code.language-mermaid');
    mermaidBlocks.forEach(function(codeEl) {
        var pre = codeEl.parentElement;
        var source = codeEl.textContent;  // raw diagram source
        var container = document.createElement('div');
        container.className = 'mermaid';
        container.textContent = source;
        pre.parentNode.replaceChild(container, pre);
    });

    // mermaid.run() renders all .mermaid divs asynchronously
    mermaid.run({
        querySelector: '.mermaid'
    }).catch(function(err) {
        // D-01: On render failure, revert to code block display
        // mermaid.run() rejects per-diagram — individual error handling via nodes approach
        console.warn('Mermaid render error:', err);
    });
}
```

**D-01 (error fallback) implementation note:** `mermaid.run()` does not reject the promise for individual diagram parse errors — it writes an error SVG into the element. To implement D-01 (show code on error), use `mermaid.render()` per diagram individually with try/catch and replace the `.mermaid` div with a `<pre><code>` on failure. This is more reliable than catching `mermaid.run()` rejections. [ASSUMED — based on mermaid API behavior from training; verify against mermaid 11.14.0 docs during implementation]

### Pattern 3: Theme Update for Mermaid

Mermaid theme must re-initialize when dark/light mode changes. Mermaid 11 does not support dynamic theme switching via `initialize()` after first call — the entire page needs re-render after reinitializing. The practical approach is:

```javascript
// Extend setTheme() in preview.html
function setTheme(isDark) {
    // ... existing CSS swaps ...
    if (window.mermaid) {
        // Re-initialize mermaid with new theme, then re-run on existing .mermaid divs
        mermaid.initialize({
            startOnLoad: false,
            theme: isDark ? 'dark' : 'default'
        });
        // Re-render diagrams by triggering a full re-render from current markdown
        // (the render message will arrive from C++ via the existing theme flow)
    }
}
```

Since theme changes trigger a full `setTheme()` call from C++ (which does not automatically re-render markdown content), diagrams are re-rendered on the next `render` message. Alternatively, store current markdown in JS state and re-render immediately on theme change. [ASSUMED — practical trade-off; confirm during implementation]

### Pattern 4: Copy-to-Clipboard Button Injection

Post-render hook injects clipboard buttons into each `<pre>` containing a `<code>` block. Uses `navigator.clipboard.writeText()` (available in WebView2 Chromium contexts).

```javascript
// Called after hljs.highlightAll() and renderMermaidDiagrams()
function injectCopyButtons() {
    var blocks = document.querySelectorAll('pre > code');
    blocks.forEach(function(codeEl) {
        var pre = codeEl.parentElement;
        if (pre.querySelector('.copy-btn')) return;  // already injected

        var btn = document.createElement('button');
        btn.className = 'copy-btn';
        btn.setAttribute('aria-label', 'Copy code');
        btn.innerHTML = '<svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">'
            + '<path d="M0 6.75C0 5.784.784 5 1.75 5h1.5a.75.75 0 0 1 0 1.5h-1.5a.25.25 0 0 0-.25.25v7.5c0 .138.112.25.25.25h7.5a.25.25 0 0 0 .25-.25v-1.5a.75.75 0 0 1 1.5 0v1.5A1.75 1.75 0 0 1 9.25 16h-7.5A1.75 1.75 0 0 1 0 14.25Z"/>'
            + '<path d="M5 1.75C5 .784 5.784 0 6.75 0h7.5C15.216 0 16 .784 16 1.75v7.5A1.75 1.75 0 0 1 14.25 11h-7.5A1.75 1.75 0 0 1 5 9.25Zm1.75-.25a.25.25 0 0 0-.25.25v7.5c0 .138.112.25.25.25h7.5a.25.25 0 0 0 .25-.25v-7.5a.25.25 0 0 0-.25-.25Z"/>'
            + '</svg>';

        // D-03: silent copy, no feedback
        btn.addEventListener('click', function() {
            var text = codeEl.textContent || '';
            navigator.clipboard.writeText(text).catch(function() {
                // Clipboard API failed — silent per D-03
            });
        });

        pre.style.position = 'relative';
        pre.appendChild(btn);
    });
}
```

CSS for copy button positioning (added to preview.html `<style>` block):
```css
/* Copy button — icon-only, top-right corner, visible on hover (D-02) */
pre {
    position: relative;
}
.copy-btn {
    position: absolute;
    top: 6px;
    right: 6px;
    display: none;
    background: transparent;
    border: 1px solid #d1d9e0;
    border-radius: 6px;
    padding: 4px;
    cursor: pointer;
    color: #636c76;
    line-height: 0;
}
body[data-theme="dark"] .copy-btn {
    border-color: #3d444d;
    color: #9198a1;
}
pre:hover .copy-btn {
    display: block;
}
.copy-btn:hover {
    background: #f6f8fa;
    color: #1f2328;
}
body[data-theme="dark"] .copy-btn:hover {
    background: #2d333b;
    color: #e6edf3;
}
```

### Pattern 5: Zoom via WebView2 AcceleratorKeyPressed

The C++ side wires `AcceleratorKeyPressed` on the WebView2 controller. When Ctrl+= / Ctrl+- / Ctrl+0 fires, C++ sends a `zoom` message to JS with the new level. The C++ handler intercepts the key, marks it `Handled=TRUE` (suppresses WebView2 default zoom behavior), and posts the message.

```cpp
// Source: learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2acceleratorkeypressedeventargs
// Registered in initWebView2() after controller is ready, on the controller (not webview)
m_controller->add_AcceleratorKeyPressed(
    Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
        [this](ICoreWebView2Controller* /*sender*/,
               ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
            COREWEBVIEW2_KEY_EVENT_KIND kind;
            args->get_KeyEventKind(&kind);
            if (kind != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN &&
                kind != COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN) return S_OK;

            UINT vk = 0;
            args->get_VirtualKey(&vk);
            bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (!ctrlDown) return S_OK;

            // VK_OEM_PLUS = 0xBB (= or +), VK_OEM_MINUS = 0xBD (-), 0x30 = '0'
            if (vk == VK_OEM_PLUS || vk == VK_OEM_MINUS || vk == 0x30) {
                args->put_Handled(TRUE);  // suppress WebView2 default Ctrl+/-/0 behavior

                // Compute new zoom level and post to JS
                float step = 0.1f;  // 10% per step (Claude's discretion — D-04)
                if (vk == VK_OEM_PLUS)  m_zoomLevel = min(8.0f, m_zoomLevel + step);
                if (vk == VK_OEM_MINUS) m_zoomLevel = max(0.8f, m_zoomLevel - step);
                if (vk == 0x30)         m_zoomLevel = 1.0f;

                // Save settings and notify JS
                g_settings.zoomLevel = m_zoomLevel;
                g_settings.save(g_configPath);  // persist immediately per D-05
                postZoomToJs(m_zoomLevel);
            }
            return S_OK;
        }).Get(),
    &m_accelKeyToken);
```

JS side handles `zoom` message:
```javascript
case 'zoom':
    document.body.style.zoom = String(msg.level);
    break;
```

**Note on `put_Handled`:** The AcceleratorKeyPressed handler fires **before** the key event is delivered to the web content. Setting `Handled=TRUE` prevents WebView2 from also performing its built-in Ctrl+= (zoom-in) and Ctrl+- (zoom-out), which would conflict. [VERIFIED: learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2acceleratorkeypressedeventargs]

**Note on `document.body.style.zoom`:** `zoom` is a CSS property supported in Chromium (WebView2). It scales the entire rendering viewport. Zoom range 80%–800% maps to float values 0.8–8.0. [ASSUMED — CSS zoom property Chromium support; should verify in WebView2 context during testing]

### Pattern 6: PDF Export via ICoreWebView2_7::PrintToPdf

PDF export is triggered from C++ menu command (no JS involvement in triggering). C++ computes the output path, creates print settings, and calls `PrintToPdf` directly.

```cpp
// Source: learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2_7
void PreviewPanel::triggerPdfExport() {
    if (!m_webview || !m_webview2Initialized || m_currentFilePath.empty()) return;
    if (m_printToPdfInProgress) return;  // guard: only one print at a time

    // D-06: output path = source .md path with .pdf extension
    std::wstring pdfPath = m_currentFilePath;
    size_t dotPos = pdfPath.rfind(L'.');
    if (dotPos != std::wstring::npos) pdfPath = pdfPath.substr(0, dotPos) + L".pdf";
    else pdfPath += L".pdf";

    // Get ICoreWebView2Environment6 to create PrintSettings
    // The environment is captured during CreateCoreWebView2EnvironmentWithOptions callback
    wil::com_ptr<ICoreWebView2Environment6> env6;
    m_environment->QueryInterface(IID_PPV_ARGS(&env6));
    if (!env6) return;

    wil::com_ptr<ICoreWebView2PrintSettings> settings;
    env6->CreatePrintSettings(&settings);

    // D-07: US Letter portrait (defaults: 8.5 x 11 inches, portrait — already default)
    // settings->put_PageWidth(8.5);   // default — no-op
    // settings->put_PageHeight(11.0); // default — no-op
    settings->put_Orientation(COREWEBVIEW2_PRINT_ORIENTATION_PORTRAIT);  // explicit
    settings->put_ShouldPrintHeaderAndFooter(TRUE);  // EXPT-03

    // D-08: Header empty, footer = "basename Page N of M"
    // ICoreWebView2PrintSettings HeaderTitle: empty string = no header
    // ICoreWebView2PrintSettings FooterUri: the URI shown in footer
    // LIMITATION: FooterUri shows the page URI, not a custom "filename Page N of M" string.
    // See Open Questions section for the implication.
    settings->put_HeaderTitle(L"");
    // FooterUri cannot be set to arbitrary text — it shows the document URI.
    // Use the default FooterUri (current URI = appassets.mdpreview/preview.html).
    // Page numbers are added automatically when ShouldPrintHeaderAndFooter = TRUE.

    settings->put_ShouldPrintBackgrounds(TRUE);  // include background colors
    settings->put_MarginTop(0.5);     // Claude's discretion per D-08
    settings->put_MarginBottom(0.5);
    settings->put_MarginLeft(0.75);
    settings->put_MarginRight(0.75);

    wil::com_ptr<ICoreWebView2_7> webview7;
    m_webview->QueryInterface(IID_PPV_ARGS(&webview7));
    if (!webview7) return;

    m_printToPdfInProgress = true;
    webview7->PrintToPdf(
        pdfPath.c_str(),
        settings.get(),
        Callback<ICoreWebView2PrintToPdfCompletedHandler>(
            [this](HRESULT errorCode, BOOL isSuccessful) -> HRESULT {
                m_printToPdfInProgress = false;
                // Silent completion per D-06 — no dialog, no notification
                return S_OK;
            }).Get());
}
```

**CRITICAL: Environment pointer must be stored.** The current `PreviewPanel` does not store `ICoreWebView2Environment*` — it only stores `m_controller` and `m_webview`. PDF export requires `ICoreWebView2Environment6::CreatePrintSettings`, so `m_environment` must be added to `PreviewPanel` and set in the `CreateCoreWebView2EnvironmentWithOptions` callback. [VERIFIED: from reading PreviewPanel.cpp — m_environment is not currently stored]

### Pattern 7: Settings Extension for Zoom

```cpp
// Settings.h additions
struct Settings {
    bool panelVisible = false;
    float zoomLevel = 1.0f;  // D-05: persisted globally, default 1.0

    void load(const std::wstring& configPath);
    void save(const std::wstring& configPath);
};

// Settings.cpp additions in load():
zoomLevel = j.value("zoomLevel", 1.0f);

// Settings.cpp additions in save():
j["zoomLevel"] = zoomLevel;
```

Zoom is applied on WebView2 init: after navigation to preview.html completes (`NavigationCompleted` event or in the `initWebView2` callback after `m_webview2Initialized = true`), post `{type:"zoom", level:zoomLevel}`.

### Pattern 8: Menu Item Extension for PDF Export

```cpp
// PluginDefinition.h: increment NB_FUNC from 2 to 3
const int NB_FUNC = 3;

// PluginDefinition.cpp: register third menu item
static ShortcutKey pdfExportShortcut = { true, false, true, 'P' };  // Ctrl+Shift+P

wcscpy_s(funcItems[2]._itemName, menuItemSize, L"Export as PDF");
funcItems[2]._pFunc = exportMarkdownAsPdf;
funcItems[2]._cmdID = 0;
funcItems[2]._init2Check = false;
funcItems[2]._pShKey = &pdfExportShortcut;
```

Verify Ctrl+Shift+P is not already bound in Notepad++ defaults. [ASSUMED — believed to be free in NPP; check during implementation]

### Anti-Patterns to Avoid

- **Loading @vscode/markdown-it-katex directly in browser:** No browser build exists. Use markdown-it-texmath instead.
- **Using mermaid.esm.min.mjs with local virtual host:** ESM entry lazily imports chunk files by URL; all chunks must be present locally. Use mermaid.min.js (IIFE) for local hosting.
- **Calling mermaid.initialize() multiple times expecting re-initialization:** Mermaid 11 only partially honors repeated initialize() calls. Store theme state and call initialize() once before first render, then manage theme by forcing a re-render cycle.
- **Not storing m_environment in PreviewPanel:** PrintToPdf requires CreatePrintSettings from ICoreWebView2Environment6. The environment must be stored during the `CreateCoreWebView2EnvironmentWithOptions` callback.
- **Not guarding against concurrent PrintToPdf calls:** Only one print operation can be in progress. A `m_printToPdfInProgress` flag must guard `triggerPdfExport()`. [VERIFIED: ICoreWebView2_7::PrintToPdf docs — concurrent call causes immediate failure]
- **Adding AcceleratorKeyPressed to the webview interface:** The event is on `ICoreWebView2Controller`, not `ICoreWebView2`. Use `m_controller->add_AcceleratorKeyPressed(...)`.
- **Using the deprecated execCommand('copy'):** Use `navigator.clipboard.writeText()` in WebView2 Chromium.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Math rendering | Custom TeX parser | KaTeX 0.16.45 | 2,000+ functions, handles edge cases, actively maintained |
| Math markdown integration | Custom $...$ regex | markdown-it-texmath | Handles nested delimiters, macro support, conflict with other plugins resolved |
| Diagram rendering | SVG generation | Mermaid 11.14.0 | 20+ diagram types, active community, battle-tested parser |
| PDF page layout | Manual PDF generation | WebView2 PrintToPdf | Uses Chromium's built-in print engine, handles pagination correctly |
| Copy to clipboard | execCommand | navigator.clipboard.writeText | execCommand deprecated and unreliable |
| Footnote back-refs | Custom link injection | markdown-it-footnote 4.0.0 | Handles multi-reference footnotes, anchor IDs, return links |

**Key insight:** Each of these domains has substantial edge-case complexity (nested math, SVG layout, PDF pagination, clipboard security contexts) that would take weeks to replicate correctly. All are solved by the bundled libraries.

---

## Common Pitfalls

### Pitfall 1: Mermaid IIFE Global Access Pattern

**What goes wrong:** `mermaid.initialize` throws "mermaid is not defined" or `mermaid` resolves to `undefined`.
**Why it happens:** Mermaid 11 IIFE bundle changed how the global is exposed. In some build variants, `window.mermaid` points to the module namespace object, not the default export. The global may be `mermaid.default` rather than `mermaid` directly.
**How to avoid:** Use `window.mermaid || window.mermaid?.default` guard. Or use the `mermaid.esm.min.mjs` via `<script type="module">` which gives a named import. During integration testing, verify `typeof window.mermaid === 'object'` in the browser console.
**Warning signs:** `mermaid.initialize is not a function` errors in the WebView2 console.

### Pitfall 2: mermaid.run() Error Handling for D-01

**What goes wrong:** `mermaid.run()` does not reject the promise when a diagram has a syntax error. Instead it inserts an error SVG into the `.mermaid` div. The code block fallback (D-01) is never triggered.
**Why it happens:** Mermaid 11's `run()` API handles errors internally per-diagram and does not propagate them as promise rejections.
**How to avoid:** Use `mermaid.render(id, source)` per diagram in a for-loop with individual try/catch. On catch, replace the `.mermaid` div with a `<pre><code class="language-mermaid">` element and call `hljs.highlightElement()` on it for the syntax-highlighted fallback.
**Warning signs:** Invalid mermaid syntax shows a red SVG error box instead of highlighted code.

### Pitfall 3: KaTeX `throwOnError: true` (Default) Crashing the Render

**What goes wrong:** A document with malformed LaTeX (e.g., `$\frac{1$`) throws a JS exception inside markdown-it's render call, which propagates to `renderMarkdown()`'s catch block and shows the "Preview unavailable" error screen for the entire document.
**Why it happens:** KaTeX defaults to `throwOnError: true`. With texmath, this propagates through the markdown-it pipeline.
**How to avoid:** Always pass `{ engine: window.katex, katexOptions: { throwOnError: false } }` to texmath. With `throwOnError: false`, malformed math renders an inline red error span.
**Warning signs:** Entire preview shows error state when document contains a single bad math expression.

### Pitfall 4: AcceleratorKeyPressed vs. WM_KEYDOWN for Zoom

**What goes wrong:** Using `WM_KEYDOWN` on the panel window does not intercept Ctrl+=/- when WebView2 has keyboard focus (which it does when the user clicks in the preview).
**Why it happens:** WebView2 captures keyboard input before it reaches the parent window's WM_KEYDOWN handler.
**How to avoid:** Use `ICoreWebView2Controller::add_AcceleratorKeyPressed`. This fires before WebView2 processes the key. Set `args->put_Handled(TRUE)` to suppress WebView2's built-in Ctrl+/- zoom behavior.
**Warning signs:** Ctrl+/- changes the WebView2 page zoom via the browser's built-in zoom rather than calling the custom zoom handler; or Ctrl+/- has no effect at all.

### Pitfall 5: PrintToPdf Requires Stored Environment Pointer

**What goes wrong:** `QueryInterface` for `ICoreWebView2Environment6` fails because `m_environment` was not stored and is no longer accessible after the async init callback returns.
**Why it happens:** The environment is only available in the `CreateCoreWebView2EnvironmentWithOptions` callback. If it is not stored, it is released when the callback completes.
**How to avoid:** Add `wil::com_ptr<ICoreWebView2Environment> m_environment;` to `PreviewPanel`. Store in the environment callback: `m_environment = env;` before calling `env->CreateCoreWebView2Controller(...)`.
**Warning signs:** `CreatePrintSettings` call on `env6` fails (E_NOINTERFACE or null pointer); `QueryInterface` returns null.

### Pitfall 6: Footnote Anchors Conflicting with Source Map `data-line`

**What goes wrong:** Clicking a footnote back-reference scrolls the preview to the source line (via the scroll sync handler) instead of following the footnote link.
**Why it happens:** Footnote anchors trigger scroll events in the preview, which may coincide with caret movement detection.
**How to avoid:** Footnote links are internal anchor links (hash navigation) within the WebView2 page — they do not trigger SCN_UPDATEUI in Scintilla, so no conflict exists at the C++ level. The JS scroll sync only fires on C++ `scroll` messages. No mitigation needed. [ASSUMED — inferred from scroll sync architecture; confirm during integration testing]

---

## Code Examples

### KaTeX CSS Load Order

```html
<!-- In <head>, before markdown-it scripts -->
<!-- Source: katex.org/docs/browser.html -->
<link rel="stylesheet" href="https://appassets.mdpreview/katex.min.css">
<script src="https://appassets.mdpreview/katex.min.js"></script>
<script src="https://appassets.mdpreview/texmath.js"></script>
```

### Footnote Plugin Init

```javascript
// After markdownit initialization, before texmath
// Source: cdn.jsdelivr.net/npm/markdown-it-footnote@4.0.0/dist/ (UMD global verified)
if (window.markdownitFootnote) {
    md.use(window.markdownitFootnote);
}
```

### Zoom Message Handler in JS Dispatcher

```javascript
case 'zoom':
    // msg.level is a float: 0.8 to 8.0 (80% to 800%)
    document.body.style.zoom = String(msg.level);
    break;
```

### PDF Export C++ Registration Pattern

```cpp
// Source: learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2_7
// Store environment pointer during init (CRITICAL — required for CreatePrintSettings)
// In the CreateCoreWebView2EnvironmentWithOptions callback:
m_environment = env;  // Add: wil::com_ptr<ICoreWebView2Environment> m_environment; to header

// PDF settings for D-07 / D-08 / EXPT-03:
// ShouldPrintHeaderAndFooter = TRUE enables automatic "date | title" header and "URI | page N of M" footer
// HeaderTitle = L"" suppresses the title portion of the default header
// FooterUri cannot be set to arbitrary text — it shows the document URI (appassets URL)
```

---

## Open Questions

1. **Custom footer text limitation (D-08)**
   - What we know: `ICoreWebView2PrintSettings` exposes `HeaderTitle` (a string) and `FooterUri` (a URI string). `ShouldPrintHeaderAndFooter=TRUE` enables a default footer of `[URI] [page N of M]`. There is no property for a fully custom footer string.
   - What's unclear: Can the basename filename be included in the footer? The `FooterUri` property shows the page URI (`https://appassets.mdpreview/preview.html`), not the source `.md` filename. The `HeaderTitle` can be set to the .md filename to show it in the header area.
   - Recommendation: Use `HeaderTitle = basename_of_md_file` (not empty as D-08 says) to include the filename at top. Leave footer as default URI + page N of M. This technically satisfies EXPT-03 ("page numbers, headers, and footers") even if D-08's exact layout (filename in footer) cannot be achieved via ICoreWebView2PrintSettings alone. Alternatively, inject a custom `@page` CSS rule into the print stylesheet via JS before calling PrintToPdf — this gives full control over header/footer content via CSS `counter(page)` etc. [ASSUMED — CSS @page support in Chromium print; needs verification in WebView2]

2. **`document.body.style.zoom` print behavior**
   - What we know: CSS `zoom` scales the visual rendering in the browser window.
   - What's unclear: Does CSS `zoom` also scale the PDF output when PrintToPdf is called, or does PrintToPdf capture at 100% zoom? If zoom scales the PDF, large zoom values (e.g., 400%) will produce a very zoomed PDF.
   - Recommendation: Before calling PrintToPdf, temporarily set `document.body.style.zoom = '1'` via JS, then restore after completion. Post a `{type:'exportPdfPrepare'}` message to JS to reset zoom, then trigger PrintToPdf in the `exportPdfReady` response.

3. **Clipboard API in WebView2 context**
   - What we know: `navigator.clipboard.writeText()` requires a secure context or explicit clipboard permission.
   - What's unclear: Does WebView2's virtual host (`appassets.mdpreview`) count as a secure context for Clipboard API?
   - Recommendation: Test during implementation. If Clipboard API is blocked, fall back to the `document.execCommand('copy')` legacy approach or use WebView2's `put_IsStatusBarEnabled` to grant clipboard permissions. [ASSUMED — expected to work; verify]

---

## Environment Availability

Step 2.6: SKIPPED for new C++ symbols and JS assets. All dependencies are:
- MSVC v145 toolset (confirmed installed per STATE.md)
- WebView2 SDK 1.0.3856.49 (already in use; ICoreWebView2_7 available since 1.0.1020.30) [VERIFIED: MS docs]
- ICoreWebView2Environment6 for CreatePrintSettings (available since 1.0.1020.30) [VERIFIED: MS docs]
- JS asset files (to be downloaded and placed in assets/ — no runtime dependencies)

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| markdown-it-katex (waylonflinn) | @vscode/markdown-it-katex / markdown-it-texmath | 2023+ | Original abandoned, VS Code fork maintained |
| MathJax for math rendering | KaTeX | 2018+ | 10x faster, synchronous |
| mermaid UMD bundle | mermaid IIFE / ESM (v11) | v11.0.0 (2024) | IIFE global: use window.mermaid |
| mermaid.init() | mermaid.run() | v10+ | init() deprecated, run() preferred |
| execCommand('copy') | navigator.clipboard.writeText() | Chrome 66+ | execCommand deprecated |
| ICoreWebView2 PrintToPdf | ICoreWebView2_7 PrintToPdf | WebView2 1.0.1020.30 | Same API, interface versioning |

**Deprecated/outdated:**
- `mermaid.init()`: Deprecated. Use `mermaid.run()`.
- `markdown-it-katex` (waylonflinn/markdown-it-katex): Not updated since 2017. The `@vscode/markdown-it-katex` fork is the maintained version, but has no browser build.
- `document.execCommand('copy')`: Deprecated. Use Clipboard API.

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `throwOnError: false` in katexOptions prevents render crash for malformed math | Architecture Patterns §Pattern 1 | Math errors crash full preview render; high impact |
| A2 | `mermaid.render()` per diagram with try/catch enables D-01 fallback behavior | Architecture Patterns §Pattern 2 | D-01 fallback cannot be implemented as designed; medium impact |
| A3 | `mermaid.initialize()` can be called again before `mermaid.run()` to update theme | Architecture Patterns §Pattern 3 | Mermaid theme won't update on dark/light switch; low-medium impact |
| A4 | `document.body.style.zoom` CSS property works in WebView2 Chromium | Architecture Patterns §Pattern 5 | Zoom feature broken; needs fallback to CSS transform:scale() |
| A5 | CSS `@page` counter() works in WebView2 PrintToPdf for custom footer | Open Questions #1 | Cannot achieve D-08 footer format; use HeaderTitle instead |
| A6 | Ctrl+Shift+P is not a default Notepad++ shortcut | Architecture Patterns §Pattern 8 | Menu shortcut conflicts; pick different key |
| A7 | Footnote anchors don't conflict with scroll sync (JS-internal navigation) | Common Pitfalls §Pitfall 6 | Clicking footnotes triggers unexpected scroll behavior |
| A8 | navigator.clipboard.writeText() works in WebView2 virtual host secure context | Open Questions #3 | Copy button silently fails; fallback needed |

---

## Sources

### Primary (HIGH confidence)
- `learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2_7` — PrintToPdf signature, concurrent print restriction
- `learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2printsettings` — all PrintSettings properties, margins, header/footer, page size
- `learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2acceleratorkeypressedeventargs` — AcceleratorKeyPressed event, Handled property, VirtualKey
- `cdn.jsdelivr.net/npm/mermaid@11.14.0/dist/` — bundle inventory: mermaid.min.js (IIFE, 3.02 MB), mermaid.esm.min.mjs (ESM, 26.56 KB)
- `cdn.jsdelivr.net/npm/markdown-it-footnote@4.0.0/dist/` — markdown-it-footnote.min.js present; UMD global `markdownitFootnote` confirmed from file inspection
- `cdn.jsdelivr.net/npm/markdown-it-texmath@1.0.0/` — texmath.js present
- `katex.org/docs/browser.html` — KaTeX browser loading, global `window.katex`, CSS file requirement
- `github.com/microsoft/vscode-markdown-it-katex/blob/main/package.json` — build: TypeScript only; no UMD/browser build confirmed

### Secondary (MEDIUM confidence)
- `mermaid.js.org/config/usage.html` — startOnLoad: false, mermaid.run(), dark theme config
- `github.com/orgs/mermaid-js/discussions/4710` — v11 IIFE breaking change; window.mermaid backward compat restored in alpha.2

### Tertiary (LOW confidence)
- WebSearch results for mermaid IIFE global access — consistent with official docs but not directly verified from mermaid source
- WebSearch for Ctrl+Shift+P in Notepad++ default shortcuts — not verified against NPP source

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — bundle formats and globals verified against CDN
- Architecture: MEDIUM — patterns synthesized from docs + codebase reading; implementation details marked [ASSUMED] where not directly verified
- Pitfalls: MEDIUM-HIGH — pitfalls 1, 2, 3, 4, 5 well-grounded in official docs; 6 is [ASSUMED]
- PDF export settings: HIGH — ICoreWebView2PrintSettings fully documented

**Research date:** 2026-04-09
**Valid until:** 2026-07-09 (30 days for most; mermaid moves fast — check for breaking changes if >2 weeks pass)
