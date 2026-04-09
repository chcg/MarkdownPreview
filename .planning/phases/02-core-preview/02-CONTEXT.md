# Phase 2: Core Preview - Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Live GFM rendering with full syntax-highlighted code, correct local images, YAML frontmatter hiding, themes (dark/light + custom CSS), synchronized editor-to-preview scrolling, and HTML export. Preview auto-opens when .md files are activated. Building on the WebView2 panel and virtual host mapping from Phase 1.

</domain>

<decisions>
## Implementation Decisions

### Auto-open Behavior
- **D-01:** Panel auto-opens whenever a .md file is activated (NPPN_FILEACTIVATED), even if the user previously closed it manually. No sticky-close logic — always surfaces when the user is in a markdown file.

### Live Rendering
- **D-02:** Debounced rendering — update preview on SCN_MODIFIED after a short pause (300ms). Full re-render on every trigger (no incremental diffing in Phase 2).
- **D-03:** REND-06: YAML frontmatter (content between leading `---` delimiters) is stripped before passing to markdown-it, not rendered as a table.

### Scroll Sync
- **D-04:** Source-map precision — markdown-it render pipeline injects `data-line="N"` attributes on block-level elements (headings, paragraphs, code blocks, tables, lists). C++ sends the current editor caret line number via WebView2 PostWebMessageAsString; JS finds the element with the closest `data-line` and scrolls to it smoothly.

### Theme Switching
- **D-05:** Live dark/light theme detection via NPPN_DARKMODECHANGED notification. Preview switches instantly when user changes Notepad++ theme — no restart required.
- **D-06:** Dark/light theme via `github-markdown-css` dark and light variants. Theme class applied on `<body>` — swapped by JS when notified by C++.

### Custom CSS
- **D-07:** User custom CSS loaded from fixed path: `%AppData%\Notepad++\plugins\config\MarkdownPreview\custom.css`. No settings UI required — users drop a file there. Absence of the file is silently ignored. Loaded as a `<link>` tag after the base theme stylesheet.

### Image Path Resolution
- **D-08:** REND-05: Local images resolved by mapping the current file's parent directory to a second WebView2 virtual host (`file.mdpreview` or similar). Relative image paths in markdown are rewritten to use this virtual host before rendering, so `./img.png` becomes `https://file.mdpreview/img.png`.

### HTML Export
- **D-09:** EXPT-01: Exported HTML is fully self-contained — local images are base64-encoded as data URIs. CSS (base theme + custom CSS if present) is inlined. The exported file works anywhere with no companion files required.
- **D-10:** Exported file saved to same directory as the source `.md` file, with `.html` extension replacing `.md`. If file already exists, overwrite (no prompt in Phase 2).

### Claude's Discretion
- Exact debounce timer value (300ms recommended baseline)
- Virtual host name for file directory mapping
- JS scroll behavior (smooth vs. instant)
- Export trigger location (menu item name, keyboard shortcut)
- Syntax highlighting theme for highlight.js (github or github-dark matching current theme)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase requirements
- `.planning/REQUIREMENTS.md` §Core Rendering (REND-01–REND-06) — Auto-open, live update, GFM, syntax highlighting, image resolution, frontmatter
- `.planning/REQUIREMENTS.md` §Themes & Styling (THME-01–THME-03) — Dark/light themes, custom CSS, auto-detection
- `.planning/REQUIREMENTS.md` §Scroll & Navigation (SCRL-01) — Scroll sync (editor → preview only; reverse is Phase 4)
- `.planning/REQUIREMENTS.md` §Export (EXPT-01) — Standalone HTML export with inlined CSS

### Existing implementation (Phase 1)
- `MarkdownPreview/src/PreviewPanel.h` — WebView2 members, virtual host setup, public interface
- `MarkdownPreview/src/PreviewPanel.cpp` — initWebView2(), virtual host mapping, Navigate() call pattern
- `MarkdownPreview/src/PluginMain.cpp` — beNotified() handler (add NPPN_FILEACTIVATED and SCN_MODIFIED here)
- `MarkdownPreview/src/PluginDefinition.h` — Global instances, NB_FUNC count (will increase)
- `MarkdownPreview/src/Settings.h` — Settings struct (will need new fields: panelAutoOpen, debounceMs, etc.)
- `MarkdownPreview/assets/welcome.html` — Existing virtual host HTML pattern to follow for preview.html

### Tech stack decisions (CLAUDE.md)
- markdown-it 14.1.1 with GFM preset (tables, strikethrough, task lists, autolinks)
- highlight.js 11.11.1 for syntax highlighting (auto-detection)
- github-markdown-css 5.8.1 for base styling (light + dark variants)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `PreviewPanel::m_webview` (`ICoreWebView2`): Already initialized. Use `PostWebMessageAsString` to send markdown content and commands (theme switch, scroll target, export trigger) from C++ to JS.
- `PreviewPanel::m_webview` (`ICoreWebView2_3`): Virtual host mapping already configured for `appassets.mdpreview` → `assets/`. Add a second mapping for the current file's directory.
- `PreviewPanel::resizeWebView2()`: Pattern for geometry management — already wired to WM_SIZE.
- `Settings` struct + JSON load/save: Extend with new fields (debounceMs, autoOpen flag if needed later).
- `beNotified()` in PluginMain.cpp: Single dispatch point. Add `NPPN_FILEACTIVATED`, `NPPN_DARKMODECHANGED`, and `SCN_MODIFIED` cases here.

### Established Patterns
- Virtual host + `file:///`-free navigation: Phase 1 uses `https://appassets.mdpreview/` for all asset URLs. Follow the same pattern for a second host mapping the open file's directory.
- WebView2 message passing: C++ → JS via `PostWebMessageAsString(JSON)`. JS → C++ via `add_WebMessageReceived` (not yet wired — needed for export file-save dialog).
- Callback-in-callback: Phase 1 shows the ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler nesting pattern. Follow same structure for any additional async operations.
- Settings JSON at `%AppData%\Notepad++\plugins\config\MarkdownPreview.json` — already loaded in onNppReady().

### Integration Points
- `beNotified()` → add handlers for file activation (trigger auto-open + initial render) and text changes (trigger debounced re-render)
- `PreviewPanel` needs: `renderMarkdown(content, filePath)`, `setTheme(isDark)`, `scrollToLine(lineNum)`, `exportHtml(targetPath)` public methods (or equivalent messaging)
- Second WebView2 virtual host: maps current file's parent directory for image resolution — must be updated when file switches
- JS side (`preview.html`): receives PostWebMessageAsString with `{type: "render", markdown: "...", filePath: "..."}`, `{type: "theme", dark: true}`, `{type: "scroll", line: 42}`, `{type: "export"}`

</code_context>

<specifics>
## Specific Ideas

No specific requirements beyond the decisions above — open to standard approaches for the JS render pipeline and WebView2 messaging protocol.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within Phase 2 scope.

</deferred>

---

*Phase: 02-core-preview*
*Context gathered: 2026-04-09*
