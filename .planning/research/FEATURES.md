# Feature Landscape

**Domain:** Notepad++ Markdown Preview Plugin
**Researched:** 2026-04-08

## Table Stakes

Features users expect from any markdown preview plugin. Missing any of these and users will switch to a competitor or a different editor entirely.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Live preview in dockable panel | Every competing tool (VS Code, IntelliJ, Markdown Monster) does this. NppMarkdownPanel and MarkdownViewerPlusPlus both offer it. Users will not accept save-then-preview workflows. | Medium | WebView2 panel rendering HTML on text change events. Must debounce for performance. |
| GitHub-Flavored Markdown (GFM) | GFM is the de facto standard. Tables, fenced code blocks, task lists, strikethrough, autolinks are all expected. CommonMark alone is insufficient. | Low | Use a mature parser like markdown-it (JS, runs in WebView2) or cmark-gfm (C). markdown-it is recommended for extensibility. |
| Syntax highlighting in code blocks | Every modern preview tool highlights code. GitHub renders it, VS Code renders it, Markdown Monster renders it. Unformatted code blocks look broken. | Low | highlight.js or Prism.js in the WebView2 context. Both are mature and well-supported. |
| Dark and light theme | NppMarkdownPanel already supports dark mode (since Npp 8.4.1). Users expect the preview to match their editor theme. A light-only preview in a dark editor is jarring. | Low | Two bundled CSS files. Detect Notepad++ theme via plugin API or let users toggle. |
| Toggle show/hide preview panel | Standard dockable panel behavior in Notepad++. Users must be able to dismiss and restore the panel. | Low | Built into Notepad++ dockable panel API. |
| Auto-open on .md file | VS Code does this. Reduces friction to zero. If users have to manually open the panel every time, they will stop using the plugin. | Low | Hook NPPN_BUFFERACTIVATED notification, check file extension. |
| Local image rendering | Markdown files reference local images constantly. A preview that shows broken image icons is useless for real work. | Medium | Resolve relative paths against the file's directory. WebView2 handles file:// URLs but needs proper base path configuration. |
| Basic scroll synchronization | VS Code, IntelliJ, and Markdown Monster all sync scroll. NppMarkdownPanel syncs to caret position. Users expect the preview to follow their editing position. | High | Line-mapping approach (map source lines to rendered elements via data attributes) is more reliable than percentage-based scrolling. This is the hardest table-stakes feature to get right. |
| Export to HTML | NppMarkdownPanel and MarkdownViewerPlusPlus both support this. Users need to share rendered output. | Low | Serialize the WebView2 DOM to a standalone HTML file with inlined CSS. |
| Custom CSS support | Both existing Notepad++ plugins support this. Power users expect to customize the preview appearance. | Low | Load user-provided CSS file path from plugin settings. Apply as additional stylesheet in WebView2. |

## Differentiators

Features that set this plugin apart from existing Notepad++ markdown plugins. Not expected given the weak competition, but valued and noticed.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Math/LaTeX rendering | NppMarkdownPanel does NOT support this. Neither does MarkdownViewerPlusPlus. VS Code requires Markdown Preview Enhanced extension. Targets technical writers and academics who use Notepad++. | Medium | KaTeX (faster, smaller) preferred over MathJax. Runs in WebView2. markdown-it-katex plugin integrates with markdown-it parser. |
| Mermaid diagram rendering | NppMarkdownPanel added basic mermaid support, but it is rudimentary. Full mermaid support (flowcharts, sequence diagrams, Gantt, etc.) is a strong differentiator in the Notepad++ ecosystem. | Medium | Load mermaid.js in WebView2. Process fenced code blocks with language "mermaid". |
| Footnotes support | Not supported by either existing Notepad++ plugin. Common in academic and technical writing. | Low | markdown-it-footnote plugin. Straightforward extension. |
| Export to PDF | Only MarkdownViewerPlusPlus supports this (via PDFSharp, limited quality). High-quality PDF export is a gap. | High | WebView2 PrintToPdfAsync API provides browser-quality PDF rendering. Much better than PDFSharp approach. |
| Copy code block button | GitHub, Material for MkDocs, and modern markdown viewers all add a "copy" button to code blocks. Huge quality-of-life feature. | Low | Small JS injection: add a button element to each pre/code block that copies textContent to clipboard. |
| Bidirectional scroll sync | Most Notepad++ plugins only sync editor-to-preview (caret position). VS Code syncs both directions. Clicking a section in the preview could scroll the editor to the corresponding source line. | High | Requires click handlers in WebView2 that map rendered elements back to source line numbers, then send Scintilla scroll commands. |
| Theme auto-detection | Automatically match preview theme to Notepad++ dark/light mode without user intervention. | Low | Query Notepad++ background color via Scintilla messages on panel open and theme change. |
| Zoom controls | NppMarkdownPanel supports 80-800% zoom. Useful for presentations and accessibility. | Low | WebView2 ZoomFactor property. Add toolbar buttons or keyboard shortcuts (Ctrl+/Ctrl-). |
| Print preview header/footer | Page numbers, document title, date in PDF/print output. Professional-quality output. | Medium | CSS @page rules and WebView2 print settings. |
| Table of contents generation | Render a clickable TOC from headings. Useful for long documents. Markdown Monster and IntelliJ both offer outline views. | Medium | Parse heading structure from AST. Render as a sidebar or top-of-document navigation. Clicking entries scrolls preview. |
| YAML frontmatter handling | Do not render YAML frontmatter as visible text. Many markdown files (Jekyll, Hugo, Obsidian) start with frontmatter. Rendering it as text looks broken. | Low | markdown-it-front-matter plugin strips/hides frontmatter block. |

## Anti-Features

Features to explicitly NOT build. These would bloat scope, conflict with Notepad++ philosophy, or create maintenance burden.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Markdown editing toolbar/ribbon | PROJECT.md explicitly scopes this out. This is a PREVIEW plugin, not an editor enhancement. Notepad++ users prefer keyboard-driven workflows. Adding toolbars violates the editor's philosophy. | Focus entirely on rendering quality and preview UX. |
| WYSIWYG/inline rendering (Typora-style) | Fundamentally incompatible with Notepad++'s Scintilla-based editor. Would require replacing the editor component entirely. Typora is a separate product category. | Side-by-side preview in dockable panel is the correct UX for Notepad++. |
| Weblog publishing | Markdown Monster does this. It is a full application feature, not a preview plugin feature. Massive scope creep. | Export to HTML/PDF covers the "share output" use case. |
| AI features (grammar, summarize, translate) | Markdown Monster added these in 2025. They require API keys, cloud services, and ongoing maintenance. Completely out of scope for a preview plugin. | Keep the plugin offline and self-contained. |
| Collaborative editing | PROJECT.md explicitly scopes this out. Notepad++ is a single-user desktop tool. | Not applicable to this product category. |
| PlantUML rendering | Requires a Java runtime or external server. Heavy dependency for a Notepad++ plugin. Mermaid covers most diagram use cases with pure JS. | Support Mermaid diagrams only. If users need PlantUML, they can use external tools. |
| File import/transclusion | Markdown Preview Enhanced supports importing external files into markdown. Niche feature, complex to implement safely, security concerns with arbitrary file reading. | Render the single open file only. |
| Snippet/autocomplete support | Editor enhancement, not preview feature. Out of scope per PROJECT.md. | Existing Notepad++ autocomplete plugins handle this. |

## Feature Dependencies

```
GFM Parsing ─────────────────┬──> Syntax Highlighting (code blocks need language detection)
                              ├──> Task Lists (GFM extension)
                              ├──> Tables (GFM extension)
                              └──> Strikethrough (GFM extension)

WebView2 Panel ──────────────┬──> Live Preview (render HTML in panel)
                              ├──> Custom CSS (load stylesheets in panel)
                              ├──> Dark/Light Theme (CSS in panel)
                              ├──> Scroll Sync (JS in panel + Scintilla API)
                              ├──> Local Image Rendering (file:// URL resolution)
                              ├──> Export to HTML (serialize panel DOM)
                              ├──> Export to PDF (WebView2 PrintToPdfAsync)
                              └──> Zoom Controls (WebView2 ZoomFactor)

markdown-it Parser ──────────┬──> Math/LaTeX (markdown-it-katex plugin)
                              ├──> Footnotes (markdown-it-footnote plugin)
                              ├──> Mermaid (custom fence renderer)
                              └──> YAML Frontmatter (markdown-it-front-matter plugin)

Scroll Sync (basic) ─────────> Bidirectional Scroll Sync (extends basic with reverse mapping)

Live Preview ─────────────────> Copy Code Block Button (JS injection into rendered output)

Heading Parsing ──────────────> Table of Contents Generation
```

## MVP Recommendation

**Phase 1 - Core Preview (ship something usable):**
1. WebView2 dockable panel with toggle
2. GFM markdown rendering via markdown-it (tables, code blocks, task lists, strikethrough)
3. Syntax highlighting via highlight.js
4. Auto-open on .md file
5. Local image rendering
6. Dark and light theme (bundled CSS)

**Phase 2 - Competitive Parity (match existing Notepad++ plugins):**
1. Basic scroll synchronization (editor caret to preview position)
2. Custom CSS support
3. Export to HTML
4. Zoom controls
5. YAML frontmatter handling

**Phase 3 - Differentiation (surpass existing plugins):**
1. Math/LaTeX rendering (KaTeX)
2. Mermaid diagram rendering
3. Footnotes support
4. Copy code block button
5. Export to PDF (WebView2 PrintToPdfAsync)
6. Theme auto-detection

**Phase 4 - Polish (publish-ready quality):**
1. Bidirectional scroll sync
2. Table of contents generation
3. Print preview formatting (headers, footers, page numbers)

**Defer indefinitely:** PlantUML, weblog publishing, AI features, WYSIWYG editing, collaborative features.

## Competitive Landscape Summary

| Feature | NppMarkdownPanel | MarkdownViewerPlusPlus | VS Code (built-in) | VS Code (MPE) | This Plugin (target) |
|---------|-----------------|----------------------|-------------------|--------------|---------------------|
| Live preview | Yes | Yes | Yes | Yes | Yes |
| GFM support | Partial | CommonMark 0.28 | Yes | Yes | Yes |
| Code highlighting | Yes | No | Yes | Yes | Yes |
| Dark mode | Yes | No | Yes | Yes | Yes |
| Scroll sync | Caret-based | Yes | Bidirectional | Bidirectional | Bidirectional |
| Math/LaTeX | No | No | No (ext needed) | Yes | Yes |
| Mermaid | Basic | No | No (ext needed) | Yes | Yes |
| Footnotes | No | No | No | Yes | Yes |
| Export HTML | Yes | Yes | No | Yes | Yes |
| Export PDF | No | Yes (basic) | No | Yes (via browser) | Yes (native WebView2) |
| Custom CSS | Yes | Yes | Yes | Yes | Yes |
| Copy code button | No | No | No (ext needed) | No | Yes |
| Zoom | Yes | No | No | No | Yes |
| YAML frontmatter | Yes | No | Yes | Yes | Yes |

## Sources

- [NppMarkdownPanel GitHub](https://github.com/mohzy83/NppMarkdownPanel) - Current leading Notepad++ markdown plugin
- [MarkdownViewerPlusPlus](https://github.com/nea/MarkdownViewerPlusPlus) - Older Notepad++ markdown plugin, dormant
- [VS Code Markdown Docs](https://code.visualstudio.com/docs/languages/markdown) - Built-in VS Code markdown features
- [Markdown Preview Enhanced](https://github.com/shd101wyy/vscode-markdown-preview-enhanced) - Feature-rich VS Code extension, gold standard
- [Markdown Monster](https://markdownmonster.west-wind.com/) - Full-featured Windows markdown editor
- [IntelliJ IDEA Markdown](https://www.jetbrains.com/help/idea/markdown.html) - JetBrains markdown support
- [Synchronized Scrolling Implementation](https://dev.to/woai3c/implementing-synchronous-scrolling-in-a-dual-pane-markdown-editor-5d75) - Technical deep-dive on scroll sync challenges
- [github-markdown-css](https://github.com/sindresorhus/github-markdown-css) - GitHub-style markdown CSS with dark mode support
