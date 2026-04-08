# Requirements: MarkdownPreview

**Defined:** 2026-04-08
**Core Value:** Open a markdown file, see it beautifully rendered in real-time as you type — no context switching, no external tools.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Core Rendering

- [ ] **REND-01**: Preview panel auto-opens as a dockable Notepad++ panel when a .md file is opened
- [ ] **REND-02**: Preview updates live as the user types (debounced for performance)
- [ ] **REND-03**: Full GFM support — tables, fenced code blocks, task lists, strikethrough, autolinks
- [ ] **REND-04**: Syntax highlighting in fenced code blocks with language auto-detection
- [ ] **REND-05**: Local images render correctly with relative path resolution against the file's directory
- [ ] **REND-06**: YAML frontmatter is hidden from rendered output

### Extended Rendering

- [ ] **XRND-01**: Math/LaTeX expressions render via KaTeX (inline and block)
- [ ] **XRND-02**: Mermaid diagrams render from fenced code blocks (flowcharts, sequence, Gantt, etc.)
- [ ] **XRND-03**: Footnotes render with proper numbering and back-references
- [ ] **XRND-04**: Code blocks include a copy-to-clipboard button (GitHub-style)

### Themes & Styling

- [ ] **THME-01**: Dark and light themes bundled out of the box
- [ ] **THME-02**: User can provide custom CSS to style the preview
- [ ] **THME-03**: Preview theme auto-detects Notepad++ dark/light mode
- [ ] **THME-04**: Zoom controls (Ctrl+/Ctrl- or toolbar, 80-800%)

### Scroll & Navigation

- [ ] **SCRL-01**: Preview scroll position syncs with editor caret position
- [ ] **SCRL-02**: Clicking in the preview scrolls the editor to the corresponding source line
- [ ] **SCRL-03**: Clickable table of contents generated from document headings

### Export

- [ ] **EXPT-01**: Export rendered markdown to standalone HTML file (with inlined CSS)
- [ ] **EXPT-02**: Export rendered markdown to PDF via WebView2
- [ ] **EXPT-03**: Print output includes page numbers, headers, and footers

### Plugin Infrastructure

- [ ] **INFR-01**: Toggle show/hide preview panel via menu item and keyboard shortcut
- [ ] **INFR-02**: Plugin loads correctly in both 32-bit and 64-bit Notepad++
- [ ] **INFR-03**: Graceful handling when WebView2 runtime is not installed
- [ ] **INFR-04**: Publish-ready quality — proper installer, versioning, plugin list compatible

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Advanced Navigation

- **ANAV-01**: Outline/document map sidebar showing heading hierarchy
- **ANAV-02**: Find-in-preview search functionality

### Advanced Rendering

- **ARND-01**: PlantUML diagram support (requires Java runtime)
- **ARND-02**: Custom markdown-it plugin loading from user directory

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Markdown editing toolbar/ribbon | This is a preview plugin, not an editor enhancement. Notepad++ users prefer keyboard-driven workflows. |
| WYSIWYG/inline rendering | Incompatible with Notepad++ Scintilla-based editor. Different product category. |
| Weblog publishing | Full application feature, not a preview plugin concern. Export covers sharing. |
| AI features (grammar, summarize) | Requires API keys, cloud services, ongoing maintenance. Out of scope. |
| Collaborative editing | Notepad++ is a single-user desktop tool. |
| File import/transclusion | Niche feature, security concerns with arbitrary file reading. |
| Non-Windows platform support | Notepad++ is Windows-only. |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| REND-01 | Phase 2 | Pending |
| REND-02 | Phase 2 | Pending |
| REND-03 | Phase 2 | Pending |
| REND-04 | Phase 2 | Pending |
| REND-05 | Phase 2 | Pending |
| REND-06 | Phase 2 | Pending |
| XRND-01 | Phase 3 | Pending |
| XRND-02 | Phase 3 | Pending |
| XRND-03 | Phase 3 | Pending |
| XRND-04 | Phase 3 | Pending |
| THME-01 | Phase 2 | Pending |
| THME-02 | Phase 2 | Pending |
| THME-03 | Phase 2 | Pending |
| THME-04 | Phase 3 | Pending |
| SCRL-01 | Phase 2 | Pending |
| SCRL-02 | Phase 4 | Pending |
| SCRL-03 | Phase 4 | Pending |
| EXPT-01 | Phase 2 | Pending |
| EXPT-02 | Phase 3 | Pending |
| EXPT-03 | Phase 3 | Pending |
| INFR-01 | Phase 1 | Pending |
| INFR-02 | Phase 1 | Pending |
| INFR-03 | Phase 1 | Pending |
| INFR-04 | Phase 4 | Pending |

**Coverage:**
- v1 requirements: 24 total
- Mapped to phases: 24
- Unmapped: 0

---
*Requirements defined: 2026-04-08*
*Last updated: 2026-04-08 after roadmap creation*
