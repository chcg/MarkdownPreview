# MarkdownPreview

## What This Is

A Notepad++ plugin that provides a live, full-featured markdown preview in a dockable panel. When a user opens a .md file, the preview panel automatically renders the markdown with synchronized scrolling. Aimed at the Notepad++ plugin community — publish-ready quality.

## Core Value

Open a markdown file, see it beautifully rendered in real-time as you type — no context switching, no external tools.

## Requirements

### Validated

- [x] Toggle to show/hide the preview panel — Validated in Phase 1: Plugin Foundation
- [x] WebView2 runtime graceful fallback when missing — Validated in Phase 1: Plugin Foundation

### Active

- [ ] Auto-open dockable preview panel when a .md file is opened
- [ ] Live rendering that updates as the user types
- [ ] Full GitHub-Flavored Markdown support (tables, fenced code blocks, task lists, strikethrough)
- [ ] Syntax highlighting in fenced code blocks
- [ ] Math/LaTeX rendering
- [ ] Mermaid diagram rendering
- [ ] Footnotes support
- [ ] Synchronized scroll position between editor and preview
- [ ] Customizable CSS themes (user-provided CSS or theme selection)
- [ ] Dark and light theme included out of the box
- [ ] Export rendered markdown to standalone HTML file
- [ ] Export rendered markdown to PDF
- [ ] Toggle to show/hide the preview panel (see Validated)
- [ ] Publish-ready quality: proper installer, error handling, Notepad++ plugin list compatible

### Out of Scope

- Markdown editing assistance (autocomplete, snippets, toolbar) — this is a preview plugin, not an editor enhancement
- Collaborative/real-time sharing — single-user desktop tool
- Support for non-Windows platforms — Notepad++ is Windows-only

## Context

- Notepad++ plugins are native C++ DLLs using the Notepad++ Plugin API (Scintilla-based editor)
- Rendering engine: WebView2 (Microsoft Edge) is a strong candidate — ships with Windows 11, available on Windows 10 via runtime
- Markdown parsing will need a C/C++ library or JavaScript library running in the WebView2 context
- The plugin needs to hook into Notepad++ notifications (file open, text changed, scroll) to drive auto-preview and sync
- Target audience: developers and technical writers who use Notepad++ and work with markdown files
- Must handle large markdown files without freezing the editor

## Constraints

- **Platform**: Windows only (Notepad++ constraint)
- **Plugin API**: Must conform to Notepad++ plugin architecture (C++ DLL, specific exports)
- **Quality**: Publish-ready for Notepad++ plugin list — needs installer, documentation, proper versioning
- **Dependencies**: WebView2 runtime must be available or bundled; should gracefully handle its absence
- **Performance**: Preview updates must not lag the editor — debounced rendering for large files

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Dockable panel (not split view) | Follows Notepad++ UX conventions, familiar to users | Validated Phase 1 |
| WebView2 for rendering engine | Modern, ships with Windows, supports full HTML/CSS/JS for rich rendering | Validated Phase 1 |
| Full-featured markdown (GFM + math + diagrams) | User wants comprehensive rendering, not a minimal previewer | — Pending |
| Customizable themes via CSS | Gives users control, reduces opinionated design decisions | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-08 after initialization*
