# MarkdownPreview

## What This Is

A Notepad++ plugin that provides a live, full-featured markdown preview in a dockable panel. When a user opens a .md file, the preview panel automatically renders the markdown with synchronized scrolling, dark/light theming, math, diagrams, and export. Aimed at the Notepad++ plugin community — publish-ready quality.

## Core Value

Open a markdown file, see it beautifully rendered in real-time as you type — no context switching, no external tools.

## Current State

**Version:** v1.0 MVP — shipped 2026-04-10

- C++ Notepad++ plugin DLL (x86 + x64), MSVC v145 toolset
- WebView2 docking panel with markdown-it 14.1.1, highlight.js, KaTeX, Mermaid 11.14.0
- Full GFM + math + diagrams — surpasses all existing Notepad++ markdown plugins
- ~47,700 LOC across 110 files (C++, HTML/JS/CSS, PowerShell)
- Packaged: DLL version resource, manifest.json, automated zip packager (`scripts/package.ps1`)
- Known issue: `manifest.json` `id` field is a SHA-256 placeholder — must be updated before plugin list PR

## Requirements

### Validated

- ✓ Toggle show/hide preview panel via Ctrl+Shift+M — v1.0 (Phase 1)
- ✓ Plugin loads in both 32-bit and 64-bit Notepad++ — v1.0 (Phase 1)
- ✓ WebView2 runtime graceful fallback when missing — v1.0 (Phase 1)
- ✓ Auto-open dockable preview panel when a .md file is opened — v1.0 (Phase 2)
- ✓ Live rendering that updates as the user types (300ms debounce) — v1.0 (Phase 2)
- ✓ Full GitHub-Flavored Markdown support (tables, fenced code, task lists, strikethrough) — v1.0 (Phase 2)
- ✓ Syntax highlighting in fenced code blocks (highlight.js, auto-detect) — v1.0 (Phase 2)
- ✓ Local images render correctly with relative path resolution — v1.0 (Phase 2)
- ✓ YAML frontmatter hidden from rendered output — v1.0 (Phase 2)
- ✓ Dark and light themes bundled out of the box — v1.0 (Phase 2)
- ✓ Preview theme auto-detects Notepad++ dark/light mode — v1.0 (Phase 2)
- ✓ User can provide custom CSS to style the preview — v1.0 (Phase 2)
- ✓ Synchronized scroll position between editor and preview — v1.0 (Phase 2)
- ✓ Export rendered markdown to standalone HTML file — v1.0 (Phase 2)
- ✓ Math/LaTeX expressions render via KaTeX (inline and block) — v1.0 (Phase 3)
- ✓ Mermaid diagrams render from fenced code blocks — v1.0 (Phase 3)
- ✓ Footnotes render with proper numbering and back-references — v1.0 (Phase 3)
- ✓ Code blocks include copy-to-clipboard button — v1.0 (Phase 3)
- ✓ Zoom controls (Ctrl+=/−/0, 80-800%) with settings persistence — v1.0 (Phase 3)
- ✓ Export rendered markdown to PDF via WebView2 — v1.0 (Phase 3)
- ✓ PDF includes page numbers, headers, and footers — v1.0 (Phase 3)
- ✓ Clicking in preview scrolls editor to corresponding source line — v1.0 (Phase 4)
- ✓ Clickable table of contents generated from document headings — v1.0 (Phase 4)
- ✓ Publish-ready packaging: DLL version resource, manifest.json, zip packager — v1.0 (Phase 4)

### Active (v1.1+)

*(No known gaps — all v1 requirements shipped. v1.1 to be defined via `/gsd-new-milestone`)*

### Out of Scope

- Markdown editing toolbar/ribbon — preview plugin, not an editor enhancement
- WYSIWYG/inline rendering — incompatible with Scintilla-based editor
- Collaborative/real-time sharing — single-user desktop tool
- Support for non-Windows platforms — Notepad++ is Windows-only
- AI features (grammar, summarize) — out of scope for a preview plugin
- PlantUML support — requires Java runtime dependency (v2 candidate)
- Custom markdown-it plugin loading from user directory (v2 candidate)

## Context

- Notepad++ plugin C++ DLL using Scintilla-based editor; MSVC v145 (VS 2026) toolset
- Rendering: WebView2 + markdown-it 14.1.1 in the browser context
- Assets bundled locally — no CDN at runtime (all JS/CSS in `assets/` via virtual host)
- PlatformToolset v145 (VS 2026) instead of v143 (VS 2022) — dev machine constraint
- `Scintilla.h` is a minimal stub — add SCI_* constants manually; never use full header
- To submit to Notepad++ plugin list: run `package.ps1`, copy SHA-256 to `manifest.json` `id` field, open PR at notepad-plus-plus/nppPluginList

## Constraints

- **Platform**: Windows only (Notepad++ constraint)
- **Plugin API**: Must conform to Notepad++ plugin architecture (C++ DLL, specific exports)
- **Quality**: Publish-ready for Notepad++ plugin list — needs installer, documentation, proper versioning
- **Dependencies**: WebView2 runtime must be available or bundled; gracefully handles its absence
- **Performance**: Preview updates must not lag the editor — debounced rendering for large files

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Dockable panel (not split view) | Follows Notepad++ UX conventions, familiar to users | ✓ Good — validated Phase 1 |
| WebView2 for rendering engine | Modern, ships with Windows, supports full HTML/CSS/JS | ✓ Good — zero friction |
| PlatformToolset v145 (VS 2026) | Dev machine has VS 2026, not VS 2022 | ✓ Good — ABI compatible |
| Lazy docking registration | Register on first toggle, then DMMSHOW/DMMHIDE only | ✓ Good — avoids init timing issues |
| CoInitializeEx in pluginInit | Early COM STA readiness before any WebView2 code | ✓ Good — prevents Pitfall 2 |
| Virtual host mapping (`appassets.mdpreview`) | Serve local assets cleanly without file:/// restrictions | ✓ Good — used for all assets |
| markdown-it with `html: false` | XSS mitigation — prevents raw HTML passthrough | ✓ Good — security baseline |
| Paired CSS link swap for theming | Atomic href swap vs body class toggle | ✓ Good — clean cascade separation |
| markdown-it-texmath (not @vscode/markdown-it-katex) | VS Code fork has no browser UMD build | ✓ Good — correct choice |
| Mermaid IIFE bundle (not ESM) | ESM lazily imports chunk files by URL — breaks offline | ✓ Good — IIFE is self-contained |
| mermaid.render() per-diagram (not mermaid.run()) | run() inserts error SVGs; render() rejects cleanly | ✓ Good — clean D-01 fallback |
| AcceleratorKeyPressed for zoom keys | Intercepts keys before WebView2 built-in zoom | ✓ Good — prevents double-zoom |
| Zoom reset before PrintToPdf | Ensures PDF renders at 100% scale | ✓ Good — correct output |
| Data-line attributes for source map | Block token annotation enables scroll sync + click-nav | ✓ Good — reused for both features |
| onNppReady/onNppShutdown pattern | Thin PluginMain.cpp; logic in PluginDefinition.cpp | ✓ Good — clean separation |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-10 after v1.0 milestone*
