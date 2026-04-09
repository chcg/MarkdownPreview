<!-- GSD:project-start source:PROJECT.md -->
## Project

**MarkdownPreview**

A Notepad++ plugin that provides a live, full-featured markdown preview in a dockable panel. When a user opens a .md file, the preview panel automatically renders the markdown with synchronized scrolling. Aimed at the Notepad++ plugin community — publish-ready quality.

**Core Value:** Open a markdown file, see it beautifully rendered in real-time as you type — no context switching, no external tools.

### Constraints

- **Platform**: Windows only (Notepad++ constraint)
- **Plugin API**: Must conform to Notepad++ plugin architecture (C++ DLL, specific exports)
- **Quality**: Publish-ready for Notepad++ plugin list — needs installer, documentation, proper versioning
- **Dependencies**: WebView2 runtime must be available or bundled; should gracefully handle its absence
- **Performance**: Preview updates must not lag the editor — debounced rendering for large files
<!-- GSD:project-end -->

<!-- GSD:stack-start source:research/STACK.md -->
## Technology Stack

## Decision: C++ Native Plugin (Not C#/.NET)
## Recommended Stack
### Core: Plugin Framework
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| MSVC (Visual Studio 2022) | v143 toolset | C++ compiler and build system | Notepad++ itself is built with MSVC; ABI compatibility required for docking dialogs |
| NppCppMSVS template | v1.0 (Mar 2025) | Plugin scaffolding | Provides VS2022 project template with Scintilla C++ interface, docking dialog examples, settings JSON mechanism |
| Windows SDK | 10.0.22621+ | Win32 API | Required for HWND management, WM_NOTIFY handling, dockable panel creation |
### Core: Rendering Engine
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| Microsoft WebView2 SDK | 1.0.3856.49 (stable, Mar 2026) | Chromium-based HTML/CSS/JS rendering in a dockable panel | Ships with Windows 11, available via Evergreen runtime on Windows 10. Full HTML5/CSS3/ES2022 support for rich markdown rendering. Both existing N++ markdown plugins use WebView2 -- it is the proven approach. |
### Core: Markdown Rendering (JavaScript in WebView2)
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| markdown-it | 14.1.1 | Markdown-to-HTML parser | CommonMark compliant, extensible via plugins, actively maintained (npm: 5840 dependents). Industry standard for pluggable markdown parsing. |
| markdown-it-footnote | 4.0.0 | Footnotes support | Official markdown-it plugin. Stable. |
### GFM (GitHub-Flavored Markdown) Support
| Feature | How | Notes |
|---------|-----|-------|
| Tables | Built-in (`html: true`) | markdown-it supports GFM tables natively when enabled |
| Fenced code blocks | Built-in | Core markdown-it feature |
| Strikethrough | Built-in (enable `strikethrough` option) | Part of markdown-it's GFM preset |
| Task lists | markdown-it-task-lists (2.1.1) | Renders `- [x]` checkboxes |
| Autolinks | Built-in (enable `linkify` option) | URL auto-detection |
### Syntax Highlighting in Code Blocks
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| highlight.js | 11.11.1 | Code syntax highlighting | **Use highlight.js, not Shiki.** Highlight.js runs client-side with zero build step, auto-detects languages, supports 185+ languages, and is 44x faster than Shiki. Shiki is better for SSG (build-time highlighting) but we need runtime highlighting in WebView2. Highlight.js is lighter (no 9MB grammar bundle). |
### Math/LaTeX Rendering
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| KaTeX | 0.16.45 | TeX math rendering | Fastest math renderer for the web. Synchronous rendering (no reflow). Actively maintained (updated April 2026). MathJax is 10x slower and overkill for preview use. |
| markdown-it-texmath | 1.0.0 | markdown-it integration for KaTeX (browser-ready UMD) | **Use markdown-it-texmath, not @vscode/markdown-it-katex.** The VS Code fork (@vscode/markdown-it-katex 1.1.2) has no browser UMD build — it ships only a CommonJS `dist/index.js` compiled from TypeScript and cannot be loaded via `<script src>` in WebView2. markdown-it-texmath ships a browser-ready `texmath.js`, accepts KaTeX as a passed-in engine (`{ engine: window.katex }`), and handles `$...$` inline and `$$...$$` block delimiters. [VERIFIED: github.com/microsoft/vscode-markdown-it-katex package.json — no bundler, TypeScript only] |
### Mermaid Diagram Rendering
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| mermaid | 11.14.0 | Diagram rendering (flowcharts, sequence, gantt, etc.) | Industry standard for text-to-diagram. Actively maintained (April 2026). |
### Themes & CSS
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| github-markdown-css | 5.8.1 | Base markdown styling | GitHub's own markdown CSS. Familiar look. Includes light and dark variants. |
| Custom CSS loader | N/A | User-provided CSS themes | Load from plugin config directory. Simple `<link>` injection. |
### Export: HTML & PDF
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| WebView2 `PrintToPdfAsync` | SDK built-in | PDF export | Native WebView2 API (`ICoreWebView2_7::PrintToPdf`). No third-party dependency. Configurable margins, orientation, scale. |
| HTML serialization | Custom | HTML export | Capture rendered HTML + inline CSS + inline JS assets into standalone .html file. |
## Build System
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| Visual Studio 2022 | 17.x | IDE and build system | NppCppMSVS template is a VS2022 .vcxproj. Notepad++ itself uses VS solution files. |
| MSBuild | (bundled) | Build automation | Standard for MSVC projects. CI-friendly via `msbuild.exe` |
| vcpkg or NuGet | Latest | C++ package management | WebView2 SDK distributed via NuGet. vcpkg for any other native deps (e.g., nlohmann/json for settings). |
## JavaScript Asset Bundling
| Approach | Details |
|----------|---------|
| Bundle method | Pre-download minified JS/CSS files, embed in plugin's `assets/` directory |
| Loading | WebView2 navigates to a local `preview.html` that references bundled assets via `file:///` or WebView2's virtual host mapping (`SetVirtualHostNameToFolderMapping`) |
| Updates | JS libraries updated when plugin is updated. Pin versions. |
## Supporting Libraries (C++ side)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| nlohmann/json | 3.11.3 | JSON parsing for settings and WebView2 messages | Settings persistence, C++/JS message passing |
| WebView2Loader.dll | (bundled with SDK) | WebView2 runtime loader | Required for WebView2 initialization |
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
# C++ / Native
# JavaScript (bundled in assets/)
## Sources
- [NppCppMSVS Plugin Template](https://community.notepad-plus-plus.org/topic/26673/nppcppmsvs-a-visual-studio-project-template-for-a-notepad-c-plugin) -- VS2022 C++ plugin template with docking support
- [WebView2 SDK NuGet](https://www.nuget.org/packages/microsoft.web.webview2) -- Latest stable: 1.0.3856.49
- [WebView2 Win32 Getting Started](https://learn.microsoft.com/en-us/microsoft-edge/webview2/get-started/win32) -- C++ integration guide
- [WebView2 PrintToPdf](https://learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/print) -- Native PDF export API
- [markdown-it on npm](https://www.npmjs.com/package/markdown-it) -- v14.1.1
- [KaTeX on npm](https://www.npmjs.com/package/katex) -- v0.16.45
- [highlight.js on npm](https://www.npmjs.com/package/highlight.js) -- v11.11.1
- [Mermaid releases](https://github.com/mermaid-js/mermaid/releases) -- v11.14.0
- [markdown-it-texmath](https://www.npmjs.com/package/markdown-it-texmath) -- v1.0.0, browser-ready UMD (replaces @vscode/markdown-it-katex which has no browser build)
- [NppAnotherMarkdown](https://github.com/ezyuzin/NppAnotherMarkdown) -- Reference implementation using markdown-it + WebView2
- [NppMarkdownPanel](https://github.com/mohzy83/NppMarkdownPanel) -- Reference implementation using Markdig + WebView2
- [Notepad++ Plugin Communication](https://npp-user-manual.org/docs/plugin-communication/) -- Plugin API and Scintilla notifications
- [highlight.js vs Shiki comparison](https://dev.to/begin/tale-of-the-tape-highlightjs-vs-shiki-27ce) -- Runtime vs build-time tradeoffs
- [Notepad++ Plugin List](https://github.com/notepad-plus-plus/nppPluginList) -- Distribution requirements (x86 + x64 zips)
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

Conventions not yet established. Will populate as patterns emerge during development.
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

Architecture not yet mapped. Follow existing patterns found in the codebase.
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, or `.github/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
