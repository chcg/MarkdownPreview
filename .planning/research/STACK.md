# Technology Stack

**Project:** MarkdownPreview (Notepad++ Plugin)
**Researched:** 2026-04-08

## Decision: C++ Native Plugin (Not C#/.NET)

Both existing Notepad++ markdown plugins (NppMarkdownPanel, NppAnotherMarkdown) use C#/.NET. We will use **C++ instead** for these reasons:

1. **No runtime dependency** -- C# plugins require .NET Framework 4.7.2+/.NET runtime, adding a failure mode. C++ DLLs load natively.
2. **ABI compatibility** -- Notepad++ is built with MSVC C++. Docking dialogs require ABI compatibility with the Notepad++ build. C++ avoids the COM interop/P-Invoke layer entirely.
3. **WebView2 is a COM API** -- The Win32 C++ WebView2 SDK is the primary supported path. C# wrappers add indirection.
4. **Performance** -- Direct Scintilla message passing (SendMessage) without marshalling overhead. Matters for high-frequency scroll sync.
5. **Plugin list credibility** -- Most published Notepad++ plugins are native C++. No .NET runtime prerequisite for users.

**Confidence:** HIGH -- This aligns with Notepad++ architecture and avoids the dependency issues reported in existing C# plugins.

## Recommended Stack

### Core: Plugin Framework

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| MSVC (Visual Studio 2022) | v143 toolset | C++ compiler and build system | Notepad++ itself is built with MSVC; ABI compatibility required for docking dialogs |
| NppCppMSVS template | v1.0 (Mar 2025) | Plugin scaffolding | Provides VS2022 project template with Scintilla C++ interface, docking dialog examples, settings JSON mechanism |
| Windows SDK | 10.0.22621+ | Win32 API | Required for HWND management, WM_NOTIFY handling, dockable panel creation |

**Confidence:** HIGH -- NppCppMSVS is the most current C++ template (March 2025), targets VS2022, and includes docking dialog examples which is exactly our use case.

### Core: Rendering Engine

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| Microsoft WebView2 SDK | 1.0.3856.49 (stable, Mar 2026) | Chromium-based HTML/CSS/JS rendering in a dockable panel | Ships with Windows 11, available via Evergreen runtime on Windows 10. Full HTML5/CSS3/ES2022 support for rich markdown rendering. Both existing N++ markdown plugins use WebView2 -- it is the proven approach. |

**Confidence:** HIGH -- WebView2 is the only viable modern rendering engine for this use case. IE11/MSHTML is deprecated. Embedding Chromium directly (CEF) is massive overkill. WebView2 is Microsoft-maintained, auto-updated, and ~1MB SDK.

**Integration pattern:** Create a child HWND for the Notepad++ docking panel, pass it to `CreateCoreWebView2Controller(parentHWND)`. WebView2 renders into the child window. Communication via `ExecuteScriptAsync` (C++ to JS) and `WebMessageReceived` (JS to C++).

### Core: Markdown Rendering (JavaScript in WebView2)

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| markdown-it | 14.1.1 | Markdown-to-HTML parser | CommonMark compliant, extensible via plugins, actively maintained (npm: 5840 dependents). Industry standard for pluggable markdown parsing. |
| markdown-it-footnote | 4.0.0 | Footnotes support | Official markdown-it plugin. Stable. |

**Confidence:** HIGH -- markdown-it is the dominant pluggable markdown parser. Used by VS Code, Mermaid docs, and both existing Notepad++ markdown plugins.

### GFM (GitHub-Flavored Markdown) Support

GFM features are handled by markdown-it configuration and plugins:

| Feature | How | Notes |
|---------|-----|-------|
| Tables | Built-in (`html: true`) | markdown-it supports GFM tables natively when enabled |
| Fenced code blocks | Built-in | Core markdown-it feature |
| Strikethrough | Built-in (enable `strikethrough` option) | Part of markdown-it's GFM preset |
| Task lists | markdown-it-task-lists (2.1.1) | Renders `- [x]` checkboxes |
| Autolinks | Built-in (enable `linkify` option) | URL auto-detection |

**Confidence:** HIGH -- markdown-it's GFM preset covers most needs. Task lists need a small plugin.

### Syntax Highlighting in Code Blocks

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| highlight.js | 11.11.1 | Code syntax highlighting | **Use highlight.js, not Shiki.** Highlight.js runs client-side with zero build step, auto-detects languages, supports 185+ languages, and is 44x faster than Shiki. Shiki is better for SSG (build-time highlighting) but we need runtime highlighting in WebView2. Highlight.js is lighter (no 9MB grammar bundle). |

**Confidence:** HIGH -- highlight.js is the standard choice for runtime/client-side syntax highlighting. Shiki's advantage (VS Code-quality grammars) doesn't justify the size/complexity for a desktop plugin.

### Math/LaTeX Rendering

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| KaTeX | 0.16.45 | TeX math rendering | Fastest math renderer for the web. Synchronous rendering (no reflow). Actively maintained (updated April 2026). MathJax is 10x slower and overkill for preview use. |
| @vscode/markdown-it-katex | 1.1.2 | markdown-it integration for KaTeX | **Use the VS Code fork, not the abandoned original.** The original `markdown-it-katex` (2.0.3) hasn't been updated in 9 years. The `@vscode/markdown-it-katex` is maintained by Microsoft and published 7 months ago. Handles `$...$` inline and `$$...$$` block math delimiters. |

**Confidence:** MEDIUM-HIGH -- KaTeX is clearly the right choice. The @vscode/markdown-it-katex plugin is the best maintained fork, but verify it works standalone outside VS Code's extension host.

### Mermaid Diagram Rendering

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| mermaid | 11.14.0 | Diagram rendering (flowcharts, sequence, gantt, etc.) | Industry standard for text-to-diagram. Actively maintained (April 2026). |

**Integration approach:** Do NOT use a markdown-it-mermaid plugin (they are all poorly maintained, outdated, or server-side). Instead, use a **post-processing approach**:

1. markdown-it renders ` ```mermaid ` blocks as `<pre><code class="language-mermaid">` elements
2. After HTML insertion into DOM, run `mermaid.init()` on those elements
3. Mermaid.js replaces the code blocks with rendered SVG diagrams

This is the same approach used by GitHub, GitLab, and the Mermaid documentation site. It avoids brittle markdown-it plugin wrappers and always uses the latest Mermaid API.

**Confidence:** HIGH -- Post-processing is the standard pattern. All markdown-it mermaid plugins are wrappers that call Mermaid anyway.

### Themes & CSS

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| github-markdown-css | 5.8.1 | Base markdown styling | GitHub's own markdown CSS. Familiar look. Includes light and dark variants. |
| Custom CSS loader | N/A | User-provided CSS themes | Load from plugin config directory. Simple `<link>` injection. |

**Confidence:** HIGH -- github-markdown-css is the de facto standard for GitHub-flavored rendering appearance.

### Export: HTML & PDF

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| WebView2 `PrintToPdfAsync` | SDK built-in | PDF export | Native WebView2 API (`ICoreWebView2_7::PrintToPdf`). No third-party dependency. Configurable margins, orientation, scale. |
| HTML serialization | Custom | HTML export | Capture rendered HTML + inline CSS + inline JS assets into standalone .html file. |

**Confidence:** HIGH -- WebView2's built-in PrintToPdf is the obvious choice. It uses the same Chromium print engine that Chrome uses. No need for wkhtmltopdf or Puppeteer.

## Build System

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| Visual Studio 2022 | 17.x | IDE and build system | NppCppMSVS template is a VS2022 .vcxproj. Notepad++ itself uses VS solution files. |
| MSBuild | (bundled) | Build automation | Standard for MSVC projects. CI-friendly via `msbuild.exe` |
| vcpkg or NuGet | Latest | C++ package management | WebView2 SDK distributed via NuGet. vcpkg for any other native deps (e.g., nlohmann/json for settings). |

**Build targets:** Both x86 and x64 DLLs required. Notepad++ plugin architecture must match the Notepad++ binary (32-bit plugin won't load in 64-bit Notepad++ and vice versa).

**Confidence:** HIGH

## JavaScript Asset Bundling

The JS libraries (markdown-it, KaTeX, highlight.js, mermaid) run inside WebView2, not in the C++ plugin. They should be **bundled as local files** shipped with the plugin, not loaded from CDN (offline support is essential for a desktop app).

| Approach | Details |
|----------|---------|
| Bundle method | Pre-download minified JS/CSS files, embed in plugin's `assets/` directory |
| Loading | WebView2 navigates to a local `preview.html` that references bundled assets via `file:///` or WebView2's virtual host mapping (`SetVirtualHostNameToFolderMapping`) |
| Updates | JS libraries updated when plugin is updated. Pin versions. |

**Do NOT use npm/webpack/esbuild in the plugin itself.** The JS assets are static, pre-built files. No Node.js tooling at runtime. Use a simple build script (PowerShell or batch) to download and bundle the specific versions.

**Confidence:** HIGH -- This is how NppAnotherMarkdown does it and it's the correct pattern for a native desktop plugin.

## Supporting Libraries (C++ side)

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| nlohmann/json | 3.11.3 | JSON parsing for settings and WebView2 messages | Settings persistence, C++/JS message passing |
| WebView2Loader.dll | (bundled with SDK) | WebView2 runtime loader | Required for WebView2 initialization |

**Confidence:** HIGH -- nlohmann/json is the de facto C++ JSON library. Single-header, no dependencies.

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Language | C++ | C# (.NET) | Adds .NET runtime dependency, ABI compat issues with docking, COM interop overhead |
| Rendering | WebView2 | CEF (Chromium Embedded) | 200MB+ binary, complex build, massive overkill |
| Rendering | WebView2 | Scintilla HTML rendering | Scintilla is a text editor, not a web renderer |
| Markdown parser | markdown-it (JS in WebView2) | Markdig (C#) / cmark (C) | markdown-it runs natively in WebView2's JS engine with zero bridging. cmark lacks GFM plugin ecosystem. Markdig requires .NET. |
| Markdown parser | markdown-it | marked.js | markdown-it has superior plugin ecosystem (footnotes, math, task lists). Marked is faster but less extensible. |
| Math | KaTeX | MathJax | MathJax is 10x slower, larger, and uses async rendering that causes page reflow |
| Syntax highlighting | highlight.js | Shiki | Shiki is 9MB, designed for build-time SSG, not runtime. highlight.js is 44x faster at runtime. |
| Syntax highlighting | highlight.js | Prism.js | Prism requires manual language loading. highlight.js auto-detects and has broader language support. |
| PDF export | WebView2 PrintToPdf | wkhtmltopdf | External binary dependency. WebView2 has it built in. |
| Build system | MSBuild (VS2022) | CMake | Notepad++ and its templates use MSBuild. CMake adds complexity for no benefit here. |

## Version Summary

```
# C++ / Native
Visual Studio 2022 (v143 toolset)
WebView2 SDK: 1.0.3856.49
nlohmann/json: 3.11.3
Windows SDK: 10.0.22621+

# JavaScript (bundled in assets/)
markdown-it: 14.1.1
markdown-it-footnote: 4.0.0
markdown-it-task-lists: 2.1.1
@vscode/markdown-it-katex: 1.1.2
KaTeX: 0.16.45
highlight.js: 11.11.1
mermaid: 11.14.0
github-markdown-css: 5.8.1
```

## Sources

- [NppCppMSVS Plugin Template](https://community.notepad-plus-plus.org/topic/26673/nppcppmsvs-a-visual-studio-project-template-for-a-notepad-c-plugin) -- VS2022 C++ plugin template with docking support
- [WebView2 SDK NuGet](https://www.nuget.org/packages/microsoft.web.webview2) -- Latest stable: 1.0.3856.49
- [WebView2 Win32 Getting Started](https://learn.microsoft.com/en-us/microsoft-edge/webview2/get-started/win32) -- C++ integration guide
- [WebView2 PrintToPdf](https://learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/print) -- Native PDF export API
- [markdown-it on npm](https://www.npmjs.com/package/markdown-it) -- v14.1.1
- [KaTeX on npm](https://www.npmjs.com/package/katex) -- v0.16.45
- [highlight.js on npm](https://www.npmjs.com/package/highlight.js) -- v11.11.1
- [Mermaid releases](https://github.com/mermaid-js/mermaid/releases) -- v11.14.0
- [@vscode/markdown-it-katex](https://www.npmjs.com/package/@vscode/markdown-it-katex) -- v1.1.2, Microsoft-maintained
- [NppAnotherMarkdown](https://github.com/ezyuzin/NppAnotherMarkdown) -- Reference implementation using markdown-it + WebView2
- [NppMarkdownPanel](https://github.com/mohzy83/NppMarkdownPanel) -- Reference implementation using Markdig + WebView2
- [Notepad++ Plugin Communication](https://npp-user-manual.org/docs/plugin-communication/) -- Plugin API and Scintilla notifications
- [highlight.js vs Shiki comparison](https://dev.to/begin/tale-of-the-tape-highlightjs-vs-shiki-27ce) -- Runtime vs build-time tradeoffs
- [Notepad++ Plugin List](https://github.com/notepad-plus-plus/nppPluginList) -- Distribution requirements (x86 + x64 zips)
