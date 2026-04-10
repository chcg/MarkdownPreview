# Milestones

## v1.0 MVP (Shipped: 2026-04-10)

**Phases completed:** 4 phases, 17 plans, 26 tasks
**Timeline:** 2026-04-08 → 2026-04-10 (3 days)
**Scope:** 110 files changed, ~47,700 insertions

**Key accomplishments:**

- Phase 1: Notepad++ C++ plugin DLL with dual x86/x64 build, six required exports, WebView2 docking panel, Ctrl+Shift+M toggle, JSON settings persistence
- Phase 1: WebView2 lazy init with runtime detection, fallback SysLink UI, and welcome page via virtual host mapping
- Phase 2: Full GFM live preview — markdown-it 14.1.1, highlight.js syntax highlighting, scroll sync, local image resolution, YAML strip, dark/light themes, custom CSS, HTML export (Ctrl+Shift+E)
- Phase 3: KaTeX math rendering, Mermaid diagrams, footnotes, copy-to-clipboard buttons, keyboard zoom (Ctrl+=/−/0, 80-800%), PDF export via WebView2 PrintToPdf (Ctrl+Shift+P)
- Phase 3 gap: Mermaid SVG rendering fixed via off-screen layout container; Ctrl+=/−/0 wired as NPP FuncItem shortcuts bypassing Scintilla focus constraint
- Phase 4: Click-to-editor navigation, TOC sidebar with active heading tracking, DLL version resource (VS_VERSION_INFO 1.0.0.0), manifest.json, automated zip packager with SHA-256

---
