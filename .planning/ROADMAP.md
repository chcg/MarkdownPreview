# Roadmap: MarkdownPreview

## Overview

MarkdownPreview delivers a live markdown preview panel for Notepad++ in four phases: first the native plugin shell with WebView2 hosting, then a fully functional GFM preview with themes and scroll sync, then extended rendering (math, diagrams, PDF export) that surpasses all existing Notepad++ markdown plugins, and finally bidirectional navigation polish and publication to the Notepad++ plugin list.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Plugin Foundation** - Native C++ plugin shell with WebView2 hosting in a dockable panel
- [ ] **Phase 2: Core Preview** - Live GFM rendering with themes, scroll sync, and HTML export
- [ ] **Phase 3: Extended Rendering** - Math, diagrams, footnotes, PDF export, and zoom
- [ ] **Phase 4: Polish & Publication** - Bidirectional navigation, TOC, and publish-ready packaging

## Phase Details

### Phase 1: Plugin Foundation
**Goal**: A working Notepad++ plugin that loads, creates a dockable panel, initializes WebView2, and can be toggled on/off
**Depends on**: Nothing (first phase)
**Requirements**: INFR-01, INFR-02, INFR-03
**Success Criteria** (what must be TRUE):
  1. Plugin DLL loads without error in both 32-bit and 64-bit Notepad++
  2. User can toggle a dockable preview panel via menu item and keyboard shortcut
  3. WebView2 initializes inside the dockable panel and renders a placeholder page
  4. When WebView2 runtime is missing, a clear message is shown instead of a crash
**Plans**: TBD

Plans:
(TBD)

### Phase 2: Core Preview
**Goal**: Users see their markdown rendered live with full GFM support, syntax-highlighted code, correct images, themes, synchronized scrolling, and can export to HTML
**Depends on**: Phase 1
**Requirements**: REND-01, REND-02, REND-03, REND-04, REND-05, REND-06, THME-01, THME-02, THME-03, SCRL-01, EXPT-01
**Success Criteria** (what must be TRUE):
  1. Opening a .md file auto-opens the preview panel with rendered content that updates live as the user types
  2. Tables, fenced code blocks, task lists, strikethrough, autolinks, and syntax-highlighted code all render correctly
  3. Local images display using correct relative paths, and YAML frontmatter is hidden from output
  4. Preview uses dark or light theme matching Notepad++ and accepts user-provided custom CSS
  5. Scrolling in the editor keeps the preview in sync, and the user can export to a standalone HTML file with inlined CSS
**Plans**: TBD
**UI hint**: yes

Plans:
(TBD)

### Phase 3: Extended Rendering
**Goal**: Preview supports math equations, Mermaid diagrams, footnotes, copy-code buttons, zoom controls, and PDF export -- surpassing all existing Notepad++ markdown plugins
**Depends on**: Phase 2
**Requirements**: XRND-01, XRND-02, XRND-03, XRND-04, THME-04, EXPT-02, EXPT-03
**Success Criteria** (what must be TRUE):
  1. Inline and block LaTeX expressions render correctly via KaTeX
  2. Mermaid fenced code blocks render as diagrams (flowcharts, sequence diagrams, Gantt charts)
  3. Footnotes render with proper numbering and back-references, and code blocks have a copy-to-clipboard button
  4. User can zoom the preview (80-800%) and export to PDF with page numbers, headers, and footers
**Plans**: TBD
**UI hint**: yes

Plans:
(TBD)

### Phase 4: Polish & Publication
**Goal**: Full bidirectional navigation between editor and preview, table of contents, and publish-ready packaging for the Notepad++ plugin list
**Depends on**: Phase 3
**Requirements**: SCRL-02, SCRL-03, INFR-04
**Success Criteria** (what must be TRUE):
  1. Clicking a location in the preview scrolls the editor to the corresponding source line
  2. A clickable table of contents generated from document headings is available in the preview
  3. Plugin is packaged with a proper installer, versioning, and is compatible with the Notepad++ plugin list
**Plans**: TBD

Plans:
(TBD)

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Plugin Foundation | 0/TBD | Not started | - |
| 2. Core Preview | 0/TBD | Not started | - |
| 3. Extended Rendering | 0/TBD | Not started | - |
| 4. Polish & Publication | 0/TBD | Not started | - |
