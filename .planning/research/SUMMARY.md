# Project Research Summary

**Project:** MarkdownPreview (Notepad++ Plugin)
**Domain:** Native Win32 C++ plugin with embedded Chromium renderer
**Researched:** 2026-04-08
**Confidence:** HIGH

## Executive Summary

MarkdownPreview is a native Notepad++ plugin that delivers a live markdown preview panel using WebView2 (Edge Chromium) as a rendering host. The correct implementation pattern is a C++ DLL that hooks Notepad++ notifications, extracts text from Scintilla, debounces updates, and communicates via JSON messages to a JavaScript rendering pipeline (markdown-it, highlight.js, KaTeX, mermaid) running inside WebView2. Both existing Notepad++ markdown plugins (NppMarkdownPanel, NppAnotherMarkdown) use C# with .NET, which introduces a runtime dependency and ABI complications with docking dialogs. Going native C++ eliminates both problems, aligns with the Notepad++ plugin ecosystem, and enables direct Scintilla messaging without marshalling overhead.

The recommended approach has two clear halves: the C++ plugin host manages the Win32 dockable panel, WebView2 lifecycle, Scintilla notification routing, and settings persistence; a self-contained JavaScript rendering engine inside WebView2 handles all parsing and rendering. All data crosses the boundary as JSON strings via `PostWebMessageAsJson` / `postMessage`. This keeps the C++ side thin and the rendering side debuggable in browser DevTools. JS libraries are bundled as static local files (no CDN, no Node.js at runtime) and loaded via WebView2's virtual host mapping.

The primary risks center on four architectural decisions that are expensive to retrofit if done wrong from the start: (1) never use `NavigateToString` as the update path — use virtual host mapping with incremental DOM updates; (2) set the WebView2 user data folder explicitly to the plugin config directory, not the default; (3) always use `NPPM_GETCURRENTSCINTILLA` to track the active Scintilla handle to avoid dual-view failures; (4) annotate rendered HTML with `data-source-line` attributes from the start even if scroll sync is not built in Phase 1, because retrofitting the annotation into a working rendering pipeline is high-cost. These four decisions collectively define the Phase 1 foundation and must be correct before any feature work begins.

## Key Findings

### Recommended Stack

The plugin is built as a 64-bit (and 32-bit) native DLL using Visual Studio 2022 (v143 toolset) scaffolded from the NppCppMSVS template (March 2025), which provides the correct project structure, Scintilla headers, and docking dialog examples. WebView2 SDK 1.0.3856.49 is the sole rendering engine choice — it ships with Windows 11, is available as an Evergreen runtime on Windows 10, requires only a ~1MB SDK, and is what both reference implementations use. Settings are persisted as JSON using nlohmann/json (single-header, no deps). Both x86 and x64 DLL targets are required for the Notepad++ plugin list.

Inside WebView2, all markdown rendering runs as JavaScript: markdown-it 14.1.1 (CommonMark + GFM, plugin ecosystem) with plugins for footnotes, task lists, and YAML frontmatter stripping; highlight.js 11.11.1 for runtime syntax highlighting (chosen over Shiki for runtime speed — 44x faster, no build step needed); KaTeX 0.16.45 + @vscode/markdown-it-katex 1.1.2 for math (the Microsoft-maintained fork, not the abandoned original); Mermaid 11.14.0 via a post-processing pattern rather than a brittle markdown-it plugin; and github-markdown-css 5.8.1 for base styling with light/dark variants. All JS/CSS assets are vendored as pre-downloaded files in the plugin's `assets/` directory — no npm, no webpack, no CDN calls.

**Core technologies:**
- Visual Studio 2022 / MSVC v143: C++ compiler — ABI compatibility with Notepad++ required for docking dialogs
- NppCppMSVS template: Plugin scaffolding — current (March 2025), includes docking examples and Scintilla interface
- WebView2 SDK 1.0.3856.49: Rendering host — Chromium in a Win32 child HWND, proven by both reference plugins
- markdown-it 14.1.1: Markdown parser (JS) — CommonMark compliant, extensible plugin ecosystem, used by VS Code
- highlight.js 11.11.1: Syntax highlighting (JS) — 185+ languages, auto-detection, 44x faster than Shiki at runtime
- KaTeX 0.16.45 + @vscode/markdown-it-katex 1.1.2: Math rendering — fastest web math renderer, synchronous
- Mermaid 11.14.0: Diagram rendering — industry standard, post-processing pattern avoids brittle wrappers
- github-markdown-css 5.8.1: Preview styling — includes light/dark variants, familiar GitHub appearance
- nlohmann/json 3.11.3: C++ JSON — settings persistence and WebView2 message serialization

### Expected Features

The competitive landscape is weak. The two existing Notepad++ markdown plugins have clear capability gaps (no math, no full Mermaid, no PDF export, inconsistent GFM). Table stakes are achievable in Phase 1-2 and genuine differentiation is reachable in Phase 3.

**Must have (table stakes):**
- Live preview in dockable panel — users will not accept save-then-preview workflows
- Full GFM support (tables, fenced code, task lists, strikethrough, autolinks) — CommonMark alone is insufficient
- Syntax highlighting in code blocks — unformatted code looks broken; all competing tools offer this
- Dark and light theme — a bright preview panel next to a dark editor is a UX failure
- Toggle show/hide preview panel — standard dockable panel behavior
- Auto-open on .md file activation — reduces friction to zero
- Local image rendering with correct relative path resolution — broken images make the plugin useless for real work
- Basic scroll synchronization (editor caret to preview) — VS Code, IntelliJ, and Markdown Monster all do this
- Export to HTML (standalone, inlined CSS) — both existing Notepad++ plugins support this
- Custom CSS support — power user expectation, both existing plugins support this

**Should have (competitive differentiators):**
- Math/LaTeX rendering (KaTeX) — no existing Notepad++ plugin supports this
- Full Mermaid diagram rendering — genuine gap in ecosystem
- Footnotes — not supported by either existing plugin
- Export to PDF via WebView2 PrintToPdfAsync — browser-quality output
- Copy code block button — GitHub-style UX, low complexity
- Theme auto-detection from Notepad++ dark mode
- YAML frontmatter stripping
- Zoom controls

### Architecture Approach

The plugin follows a layered architecture: a thin `plugin/` layer exposes the six required Notepad++ DLL exports and routes notifications; a `core/` layer containing the `PreviewController` and `NotificationHandler` orchestrates debounced updates and scroll sync; a `webview/` layer encapsulates all WebView2 COM lifecycle; and a `panel/` layer manages the Win32 dockable dialog HWND that hosts WebView2 as a child window.

**Major components:**
1. Plugin Core (plugin/) — DLL exports, lifecycle, menu registration
2. Notification Handler (core/) — routes SCN_MODIFIED, NPPN_BUFFERACTIVATED, SCN_UPDATEUI via debounce timer; handles dual Scintilla view tracking
3. Preview Controller (core/) — debounce logic, Scintilla text extraction, scroll ratio calculation, WebView2 message dispatch
4. WebView2 Host (webview/) — async environment creation, HWND parenting, message bridge
5. Dockable Panel (panel/) — Win32 dialog registered with NPPM_DMMREGASDCKDLG; owns parent HWND; handles resize
6. JS Rendering Engine (resources/) — markdown-it + plugins pipeline inside WebView2; DOM updates in-place
7. Export (export/) — HTML serialization and WebView2 PrintToPdfAsync integration
8. Settings (settings/) — JSON config read/write via nlohmann/json

### Critical Pitfalls

1. **NavigateToString 2MB limit** — Never use `NavigateToString` as the update path. Use virtual host mapping + `PostWebMessageAsJson` for all updates.
2. **SCN_MODIFIED flood and missed events** — Hook `NPPN_GLOBALMODIFIED` in addition to `SCN_MODIFIED`. Use `NPPM_ADDSCNMODIFIEDFLAGS` after `NPPN_READY`. Always debounce (200-300ms).
3. **WebView2 user data folder permissions** — Explicitly set to plugin config directory. Never use the default.
4. **Dual Scintilla view architecture** — Always use `NPPM_GETCURRENTSCINTILLA` to get the active handle.
5. **Scroll sync: percentage-based fails** — Use element-based `data-source-line` annotation from Phase 1.
6. **Preview flicker** — Incremental DOM updates (morphdom), not full innerHTML replacement.
7. **DPI scaling** — Requires Per-Monitor V2 awareness on WebView2 creation thread.

## Implications for Roadmap

### Phase 1: Foundation and Core Infrastructure
Correct architectural decisions and pitfall prevention. Plugin DLL loads, dockable panel shows, WebView2 initializes correctly, notifications hook with dual-view tracking. Not yet rendering markdown — but the pipeline is architecturally correct.

### Phase 2: Core Preview (Usable MVP)
Full GFM rendering, syntax highlighting, scroll sync, themes, HTML export. Competitive with NppMarkdownPanel. `data-source-line` annotation designed in from the start.

### Phase 3: Differentiation Features
Math (KaTeX), Mermaid diagrams, footnotes, PDF export, copy-code button. Surpasses all existing Notepad++ markdown plugins.

### Phase 4: Polish and Publication
Bidirectional scroll sync, TOC, print formatting, portable mode, plugin list submission.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All technology choices verified against official docs, npm registries, and two reference implementations |
| Features | HIGH | Competitive landscape mapped across NppMarkdownPanel, MarkdownViewerPlusPlus, VS Code, Markdown Monster |
| Architecture | HIGH | Patterns verified against official Notepad++ plugin docs, WebView2 Microsoft docs, and working references |
| Pitfalls | HIGH | Each pitfall verified against official docs, GitHub issues, and community posts |

**Gaps to Address:**
- @vscode/markdown-it-katex standalone behavior outside VS Code
- Mermaid 11.x init API confirmation
- WebView2 runtime detection exact API
- Portable mode user data folder on read-only media

---
*Research completed: 2026-04-08*
*Ready for roadmap: yes*
