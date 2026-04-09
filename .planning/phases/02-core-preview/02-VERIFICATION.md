---
phase: 02-core-preview
verified: 2026-04-09T17:45:00Z
status: human_needed
score: 5/5 must-haves verified
overrides_applied: 0
re_verification:
  previous_status: gaps_found
  previous_score: 3/5
  gaps_closed:
    - "Preview renders markdown content correctly for all users including non-ASCII (international) markdown"
    - "Preview uses dark or light theme matching Notepad++ and accepts user-provided custom CSS"
  gaps_remaining: []
  regressions: []
human_verification:
  - test: "Open a .md file containing non-ASCII content (e.g., # 你好世界, accented chars café résumé, emoji 🎉) in Notepad++"
    expected: "Preview panel renders the content correctly without showing the 'Preview unavailable' error state"
    why_human: "Runtime WebView2 behavior requires launching Notepad++ with the installed plugin DLL. Cannot verify programmatically that PostWebMessageAsJson correctly delivers non-ASCII payloads end-to-end."
  - test: "Place a custom.css file at %APPDATA%\\Notepad++\\plugins\\config\\MarkdownPreview\\custom.css containing body { background: pink !important; }, then open a .md file"
    expected: "Preview panel background turns pink, demonstrating custom CSS is loaded from disk and applied to the rendered output"
    why_human: "Requires file system setup, plugin runtime under Notepad++, and visual inspection. Cannot be verified from source code alone."
---

# Phase 2: Core Preview Verification Report

**Phase Goal:** Users see their markdown rendered live with full GFM support, syntax-highlighted code, correct images, themes, synchronized scrolling, and can export to HTML
**Verified:** 2026-04-09T17:45:00Z
**Status:** human_needed
**Re-verification:** Yes — after Plan 05 gap closure (commits 6c1f18b and cbf4ada)

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Opening a .md file auto-opens the preview panel with rendered content that updates live as the user types | VERIFIED | onBufferActivated wired in PluginMain.cpp beNotified; auto-opens via g_previewPanel.toggle(); SCN_MODIFIED debounce (300ms SetTimer/KillTimer + m_renderPending) fully implemented in PreviewPanel.cpp |
| 2 | Tables, fenced code blocks, task lists, strikethrough, autolinks, and syntax-highlighted code all render correctly | VERIFIED | markdown-it 14.1.1 initialized with default preset (tables, strikethrough, linkify enabled); markdownitTaskLists UMD wired; highlight.js 11.11.1 hljs.highlightAll() called after innerHTML assignment; all assets present and non-empty |
| 3 | Local images display using correct relative paths, and YAML frontmatter is hidden from output | VERIFIED | stripFrontmatter() now handles UTF-8 BOM (charCodeAt check) and \r\n line endings (search regex); rewriteImagePaths() uses balanced-paren regex; file.mdpreview virtual host via ICoreWebView2_3 Clear+Set with DENY_CORS; called before renderMarkdown() in onBufferActivated() |
| 4 | Preview uses dark or light theme matching Notepad++ and accepts user-provided custom CSS | VERIFIED (code) | THME-01/THME-03: dark/light theme swap and NPPN_DARKMODECHANGED handling confirmed. THME-02: renderMarkdown() now reads %APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css via std::ifstream and includes j["customCss"] in render JSON payload; JS setCustomCss() receives and applies it. Full data path now wired. Human runtime test still needed — see Human Verification Required. |
| 5 | Scrolling in the editor keeps the preview in sync, and the user can export to a standalone HTML file with inlined CSS | VERIFIED | onScnUpdateUi() filters SC_UPDATE_SELECTION|SC_UPDATE_CONTENT, caret line retrieved via SCI_GETCURRENTPOS + SCI_LINEFROMPOSITION, scrollToLine() posts {type:scroll,line:N}; JS querySelectorAll nearest-element + scrollIntoView smooth confirmed. Export pipeline: triggerExport() posts {type:export}, async exportHtml() collects CSS/encodes images/serializes DOM, postMessages exportReady, handleJsMessage() calls saveExportedHtml() with UTF-8 BOM write. |

**Score:** 5/5 truths verified (code-level)

### Re-verification: Gaps Closed

Both gaps from the initial verification (2026-04-09T15:36:10Z) are closed:

**Gap 1 — Non-ASCII content corruption (CLOSED):**
The `std::wstring wjson(jsonStr.begin(), jsonStr.end())` pattern has been replaced at all 4 call sites. Confirmed: `grep -c "wjson(jsonStr.begin(), jsonStr.end())" PreviewPanel.cpp` returns `0`. A `Utf8ToWide()` static helper using `MultiByteToWideChar(CP_UTF8, ...)` was added and is used at all 4 PostWebMessageAsJson sites (lines 389, 401, 419, 469). `grep -c "Utf8ToWide" PreviewPanel.cpp` returns `5` (1 definition + 4 call sites).

**Gap 2 — THME-02 custom CSS never sent (CLOSED):**
`renderMarkdown()` now reads `%APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css` and populates `j["customCss"]` with the file content (or JSON null if absent). Confirmed: 4 `j["customCss"]` assignment lines present at lines 376–384. The JS dispatcher correctly reads `msg.customCss` (line 370) and passes it to `renderMarkdown()` which calls `setCustomCss()`.

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `MarkdownPreview/include/Notepad_plus_msgs.h` | NPPN_BUFFERACTIVATED = 1010; 6 constants added | VERIFIED | Unchanged from initial verification — confirmed |
| `MarkdownPreview/include/Scintilla.h` | SCN_MODIFIED, SCN_UPDATEUI, SC_MOD_*, SCI_GETCURRENTPOS, SC_UPDATE_* | VERIFIED | Unchanged from initial verification — confirmed |
| `MarkdownPreview/src/PreviewPanel.h` | All public/private methods and state members | VERIFIED | Unchanged from initial verification — confirmed |
| `MarkdownPreview/src/PreviewPanel.cpp` | Utf8ToWide() helper; 4 safe PostWebMessageAsJson sites; customCss JSON field; getUserDataPath() validation; destroy() token cleanup | VERIFIED | All 5 Plan 05 changes confirmed: Utf8ToWide at lines 27/389/401/419/469; j["customCss"] at lines 376-384; ret check at line 282; remove_WebMessageReceived at line 59 |
| `MarkdownPreview/src/PluginMain.cpp` | beNotified handles 4 notification cases | VERIFIED | Unchanged from initial verification — confirmed |
| `MarkdownPreview/src/PluginDefinition.cpp` | All handlers; updateFileVirtualHost before renderMarkdown | VERIFIED | Unchanged from initial verification — confirmed |
| `MarkdownPreview/src/PluginDefinition.h` | NB_FUNC = 2; exportMarkdown() declared | VERIFIED | Unchanged from initial verification — confirmed |
| `MarkdownPreview/assets/preview.html` | stripFrontmatter with BOM+CRLF handling; rewriteImagePaths with balanced-paren regex | VERIFIED | charCodeAt(0)===0xFEFF at line 145; search(/\r?\n---(\r?\n|$)/) at line 150; balanced-paren regex at line 207; old indexOf('\n---') absent |
| All 7 JS/CSS asset files | markdown-it 14.1.1, highlight.js 11.11.1, github-markdown-css 5.9.0 variants, hljs themes | VERIFIED | Unchanged from initial verification — confirmed |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| PluginMain.cpp beNotified | PreviewPanel.renderMarkdown / scheduleRender / setTheme | onBufferActivated / onScnModified / onDarkModeChanged | WIRED | All 4 beNotified cases wired — unchanged |
| PreviewPanel.doRender | WebView2 PostWebMessageAsJson | nlohmann JSON + Utf8ToWide() | WIRED | Utf8ToWide() replaces unsafe byte-copy at line 389 |
| C++ renderMarkdown | JS setCustomCss | j["customCss"] field in render message | WIRED | NEW: j["customCss"] populated from %APPDATA% file read in renderMarkdown(); JS msg.customCss|null dispatched to setCustomCss() |
| window.chrome.webview message event | renderMarkdown() / setTheme() / scrollToLine() / exportHtml() | switch on msg.type (4 cases) | WIRED | All 4 cases present — unchanged |
| markdown-it md.render() | document.getElementById('preview').innerHTML | rewriteImagePaths then md.render then assignment | WIRED | Unchanged |
| hljs.highlightAll() | pre > code elements in preview div | called after innerHTML assignment | WIRED | Unchanged |
| SCN_UPDATEUI in PluginMain | PreviewPanel.scrollToLine() | onScnUpdateUi() SC_UPDATE_SELECTION|SC_UPDATE_CONTENT filter | WIRED | Unchanged |
| JS scrollToLine() | element with matching data-line | querySelectorAll('[data-line]') nearest-element scan | WIRED | Unchanged |
| onBufferActivated() | file.mdpreview virtual host | updateFileVirtualHost() called BEFORE renderMarkdown() | WIRED | Unchanged |
| Export as HTML menu item | PreviewPanel via triggerExport() | exportMarkdown() in PluginDefinition.cpp | WIRED | Unchanged |
| JS exportHtml() | C++ handleJsMessage() | window.chrome.webview.postMessage({type:exportReady, html:...}) | WIRED | Unchanged |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|--------------|--------|-------------------|--------|
| preview.html renderMarkdown | markdown (from msg.markdown) | C++ renderMarkdown → Scintilla SCI_GETTEXT → Utf8ToWide conversion | Yes — real document text, now UTF-8 safe | FLOWING |
| preview.html setCustomCss | customCss (from msg.customCss) | C++ renderMarkdown → %APPDATA% custom.css ifstream read → j["customCss"] | Yes — real file content or null | FLOWING (was DISCONNECTED) |
| preview.html setTheme | isDark (from msg.dark) | C++ setTheme → NPPM_ISDARKMODEENABLED | Yes — real NPP dark mode state | FLOWING |
| preview.html scrollToLine | line (from msg.line) | C++ onScnUpdateUi → SCI_GETCURRENTPOS + SCI_LINEFROMPOSITION | Yes — real caret position | FLOWING |
| preview.html exportHtml | clone of #preview innerHTML | JS DOM serialization after render | Yes — rendered HTML | FLOWING |

### Behavioral Spot-Checks

| Behavior | Check | Result | Status |
|----------|-------|--------|--------|
| Zero unsafe wstring iterator constructions remaining | grep -c "wjson(jsonStr.begin(), jsonStr.end())" PreviewPanel.cpp | 0 | PASS |
| Utf8ToWide() defined and used at 4 call sites | grep -c "Utf8ToWide" PreviewPanel.cpp | 5 | PASS |
| customCss field populated in render JSON | grep -n "customCss" PreviewPanel.cpp | 4 lines (content + 3 null fallbacks) | PASS |
| Custom CSS read from APPDATA path | grep -n "GetEnvironmentVariableW.*APPDATA" PreviewPanel.cpp | 1 match at line 366 | PASS |
| WR-01 token cleanup in destroy() | grep -n "remove_WebMessageReceived" PreviewPanel.cpp | 1 match at line 59 | PASS |
| CR-03 LOCALAPPDATA validation | grep -n "ret == 0 \|\| ret >= MAX_PATH" PreviewPanel.cpp | 1 match at line 282 | PASS |
| WR-05 BOM strip in stripFrontmatter | grep -n "charCodeAt(0) === 0xFEFF" preview.html | 1 match at line 145 | PASS |
| WR-05 CRLF-safe closing delimiter | grep -n "markdown.search" preview.html | 1 match at line 150 | PASS |
| WR-05 old indexOf pattern gone | grep -c "indexOf.*\\\\n---" preview.html | 0 | PASS |
| WR-06 balanced-paren regex in rewriteImagePaths | Line 207 of preview.html | [^)\s][^)]*(?:\([^)]*\)[^)]*)*) present | PASS |
| Plan 05 commits exist | git log --oneline | 6c1f18b and cbf4ada confirmed | PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| REND-01 | 02-01 | Preview panel auto-opens when .md file opened | SATISFIED | onBufferActivated auto-opens panel; NPPN_BUFFERACTIVATED = 1010 |
| REND-02 | 02-01 | Live preview updates (debounced) | SATISFIED | SCN_MODIFIED triggers scheduleRender(); 300ms SetTimer/KillTimer debounce |
| REND-03 | 02-02 | Full GFM support | SATISFIED | markdown-it default preset + markdownitTaskLists wired |
| REND-04 | 02-02 | Syntax highlighting with auto-detection | SATISFIED | highlight.js 11.11.1 hljs.highlightAll() after innerHTML |
| REND-05 | 02-03 | Local images with relative path resolution | SATISFIED | rewriteImagePaths() (now with balanced-paren regex) + file.mdpreview virtual host |
| REND-06 | 02-02 | YAML frontmatter hidden from output | SATISFIED | stripFrontmatter() now handles BOM and \r\n (WR-05 fix applied) |
| THME-01 | 02-02 | Dark and light themes bundled | SATISFIED | github-markdown-light/dark.css + hljs theme pair; setTheme() swaps both atomically |
| THME-02 | 02-02, 02-05 | User custom CSS | SATISFIED (code) | C++ reads custom.css from %APPDATA% path; sends in render message; JS applies via setCustomCss(). Human runtime test required. |
| THME-03 | 02-01 | Auto-detect NPP dark/light mode | SATISFIED | NPPM_ISDARKMODEENABLED on startup; NPPN_DARKMODECHANGED handled |
| SCRL-01 | 02-03 | Scroll sync with editor caret | SATISFIED | Full onScnUpdateUi pipeline to JS scrollToLine nearest-element scrollIntoView |
| EXPT-01 | 02-04 | Export to standalone HTML with inlined CSS | SATISFIED | Full async exportHtml pipeline; CSS fetch; base64 image encoding; saveExportedHtml with UTF-8 BOM |

All 11 Phase 2 requirements are now satisfied at the code level.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| PreviewPanel.cpp | 153 (preview.html) | One `markdown.indexOf('\n', end + 1)` remains | Info | This is intentional new code (advances past closing delimiter after search()); not the old broken indexOf('\n---') pattern. No issue. |

No blocker or warning anti-patterns found. All previously identified issues have been resolved.

### Human Verification Required

The following items require runtime testing with the installed plugin under Notepad++ and cannot be verified programmatically from source code alone.

#### 1. Non-ASCII Markdown Rendering

**Test:** Open a .md file containing non-ASCII content (Chinese characters `# 你好世界`, accented characters `café résumé`, emoji `🎉 Great job!`) in Notepad++
**Expected:** Preview panel renders the content correctly without showing "Preview unavailable" error state — full Unicode content visible
**Why human:** The Utf8ToWide() fix is confirmed in source code, but the full PostWebMessageAsJson → JS render path requires running Notepad++ with the built plugin DLL to verify no other encoding issue remains in the pipeline

#### 2. Custom CSS Application (THME-02)

**Test:** Place `custom.css` at `%APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css` containing `body { background: pink !important; }`, then open a .md file
**Expected:** Preview panel background turns pink, demonstrating C++ reads the file, includes it in the render message, and JS applies it via setCustomCss()
**Why human:** Requires file system setup, running plugin under Notepad++, and visual verification of background color change. The code path is wired but the APPDATA path must be accessible and the CSS must visually apply.

### Gaps Summary

No gaps remain. Both blocking gaps from the initial verification have been closed by Plan 05 (commits 6c1f18b and cbf4ada):

1. **Non-ASCII corruption** — Utf8ToWide() helper replaces all 4 unsafe iterator-copy call sites
2. **THME-02 custom CSS** — renderMarkdown() now reads and delivers custom.css content in render message

Status is `human_needed` (not `passed`) because 2 human verification items remain, as required by the success criteria classification rules. These are runtime/visual tests that require the built plugin running under Notepad++ — they are not additional code gaps.

---

_Verified: 2026-04-09T17:45:00Z_
_Verifier: Claude (gsd-verifier)_
_Re-verification: Yes — initial verification 2026-04-09T15:36:10Z had status gaps_found (3/5)_
