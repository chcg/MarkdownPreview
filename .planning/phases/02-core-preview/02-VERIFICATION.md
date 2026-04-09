---
phase: 02-core-preview
verified: 2026-04-09T15:36:10Z
status: gaps_found
score: 3/5 must-haves verified
overrides_applied: 0
gaps:
  - truth: "Preview uses dark or light theme matching Notepad++ and accepts user-provided custom CSS"
    status: partial
    reason: "Dark/light theme sync (THME-01, THME-03) is fully implemented. Custom CSS (THME-02) has JS infrastructure complete (setCustomCss, dispatcher), but C++ never reads the custom.css file or includes it in the render message. renderMarkdown() only sends {type, markdown, filePath} — the customCss field is always absent. JS falls back to null silently, so no crash but feature is non-functional."
    artifacts:
      - path: "MarkdownPreview/src/PreviewPanel.cpp"
        issue: "renderMarkdown() builds JSON with only type/markdown/filePath. No custom.css file read, no j[\"customCss\"] field. Plan 02-02 explicitly required this C++ wiring."
    missing:
      - "In PreviewPanel::renderMarkdown(), read the custom.css file from %APPDATA%\\Notepad++\\plugins\\config\\MarkdownPreview\\custom.css (or %LOCALAPPDATA%\\MarkdownPreview path)"
      - "Convert custom CSS content to UTF-8 and add j[\"customCss\"] = utf8CssContent to the nlohmann JSON payload (or set to null if file absent)"
  - truth: "Preview renders markdown content correctly for all users including non-ASCII (international) markdown"
    status: failed
    reason: "All four PostWebMessageAsJson call sites use the naive std::wstring iterator constructor: std::wstring wjson(jsonStr.begin(), jsonStr.end()). This copies each char byte to a wchar_t slot by zero-extension, corrupting any multi-byte UTF-8 sequence. Non-ASCII markdown (accented characters, CJK, Arabic, emojis) silently produces garbled JSON that WebView2 either fails to parse or mis-parses, blanking the preview. ASCII-only markdown is unaffected. This bug is present in renderMarkdown (line 337), setTheme (line 349), scrollToLine (line 367), and triggerExport (line 417)."
    artifacts:
      - path: "MarkdownPreview/src/PreviewPanel.cpp"
        issue: "Line 337: std::wstring wjson(jsonStr.begin(), jsonStr.end()) — unsafe byte-to-wchar_t copy. Same pattern at lines 349, 367, 417."
    missing:
      - "Replace all four instances of std::wstring wjson(jsonStr.begin(), jsonStr.end()) with MultiByteToWideChar(CP_UTF8, ...) conversion"
      - "Add a helper static std::wstring Utf8ToWide(const std::string& utf8) to PreviewPanel.cpp to avoid repetition (pattern already used elsewhere in the same file for getCurrentText)"
---

# Phase 2: Core Preview Verification Report

**Phase Goal:** Users see their markdown rendered live with full GFM support, syntax-highlighted code, correct images, themes, synchronized scrolling, and can export to HTML
**Verified:** 2026-04-09T15:36:10Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Opening a .md file auto-opens the preview panel with rendered content that updates live as the user types | VERIFIED | onBufferActivated wired in PluginMain.cpp beNotified; auto-opens via g_previewPanel.toggle(); SCN_MODIFIED debounce (300ms SetTimer/KillTimer + m_renderPending) fully implemented in PreviewPanel.cpp |
| 2 | Tables, fenced code blocks, task lists, strikethrough, autolinks, and syntax-highlighted code all render correctly | VERIFIED | markdown-it 14.1.1 initialized with default preset (tables, strikethrough, linkify enabled); markdownitTaskLists UMD global confirmed in downloaded asset and wired in preview.html; highlight.js 11.11.1 hljs.highlightAll() called after innerHTML assignment; all assets present and non-empty |
| 3 | Local images display using correct relative paths, and YAML frontmatter is hidden from output | VERIFIED | stripFrontmatter() strips YAML between leading --- delimiters; rewriteImagePaths() rewrites relative paths to https://file.mdpreview/ before md.render(); PreviewPanel::updateFileVirtualHost() maps file.mdpreview virtual host using ICoreWebView2_3 Clear+Set with DENY_CORS; called before renderMarkdown() in onBufferActivated() |
| 4 | Preview uses dark or light theme matching Notepad++ and accepts user-provided custom CSS | PARTIAL FAIL | THME-01 (dark/light bundled): github-markdown-light.css + github-markdown-dark.css + hljs theme swap implemented in setTheme() — verified. THME-03 (auto-detect NPP mode): NPPM_ISDARKMODEENABLED called on startup and NPPN_DARKMODECHANGED handled — verified. THME-02 (user custom CSS): JS setCustomCss() and msg.customCss dispatcher exist, but C++ renderMarkdown() never sends customCss field — feature non-functional |
| 5 | Scrolling in the editor keeps the preview in sync, and the user can export to a standalone HTML file with inlined CSS | VERIFIED | onScnUpdateUi() filters SC_UPDATE_SELECTION\|SC_UPDATE_CONTENT, retrieves caret line via SCI_GETCURRENTPOS + SCI_LINEFROMPOSITION, calls scrollToLine(); JS querySelectorAll('[data-line]') nearest-element scan + scrollIntoView smooth verified in preview.html; Export pipeline end-to-end: triggerExport() posts {type:export}, async exportHtml() collects CSS/encodes images/serializes DOM, postMessages exportReady, handleJsMessage() calls saveExportedHtml() with UTF-8 BOM write |

**Score:** 3/5 truths verified (SC4 partial, plus implicit SC for non-ASCII content from bug CR-01)

### Critical Bug: Non-ASCII Content Corruption (Blocker)

**File:** MarkdownPreview/src/PreviewPanel.cpp, lines 337, 349, 367, 417

**Pattern:** `std::wstring wjson(jsonStr.begin(), jsonStr.end());`

This pattern copies each `char` byte into a `wchar_t` slot by zero-extension, corrupting any multi-byte UTF-8 sequence. `nlohmann::json::dump()` returns valid UTF-8, but the wstring iterator constructor does not decode it — it just byte-widens. Any non-ASCII character in the markdown source (e.g., `é`, `中文`, `🎉`) produces garbled wide characters, causing `PostWebMessageAsJson` to send malformed JSON to the JS side. The JS `catch` block shows the error state silently.

**Affected call sites:**
- `renderMarkdown()` line 337 — all render messages with non-ASCII content fail
- `setTheme()` line 349 — theme messages unaffected (JSON is pure ASCII), but pattern is inconsistent
- `scrollToLine()` line 367 — scroll messages unaffected (pure integers)
- `triggerExport()` line 417 — export trigger unaffected (pure `{type:export}`)

**Practical impact:** Any user with non-ASCII markdown (international users, emoji-heavy documents, code comments in non-Latin scripts) sees a blank or "Preview unavailable" error state instead of rendered content. This directly contradicts Success Criterion 1 and 2.

**Fix available:** `MultiByteToWideChar(CP_UTF8, ...)` is already used in the same file in `getCurrentText()` and other conversion paths. Add a `Utf8ToWide()` helper and replace the four affected call sites.

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `MarkdownPreview/include/Notepad_plus_msgs.h` | NPPN_BUFFERACTIVATED corrected to 1010; 6 missing constants added | VERIFIED | NPPN_BUFFERACTIVATED = (NPPN_FIRST + 10) confirmed; all 6 constants present (lines 33-42) |
| `MarkdownPreview/include/Scintilla.h` | SCN_MODIFIED, SCN_UPDATEUI, SC_MOD_*, SCI_GETCURRENTPOS, SCI_LINEFROMPOSITION, SC_UPDATE_* | VERIFIED | All constants present (lines 49-64) |
| `MarkdownPreview/src/PreviewPanel.h` | renderMarkdown, setTheme, scheduleRender, showIdle, scrollToLine, updateFileVirtualHost, triggerExport public; doRender, handleJsMessage, saveExportedHtml private; members m_isDark, m_currentFilePath, m_exportFilePath, m_webMessageReceivedToken | VERIFIED | All 7 public methods and 3 private methods confirmed at lines 21-28 and 46-47; all members at lines 67-70 |
| `MarkdownPreview/src/PreviewPanel.cpp` | All 7 public methods implemented; 300ms debounce with SetTimer/KillTimer; nlohmann JSON for all messages; add_WebMessageReceived wired; saveExportedHtml with UTF-8 BOM | STUB (partial) | All methods implemented and wired. Critical: std::wstring iterator constructor bug at lines 337/349/367/417 corrupts non-ASCII UTF-8 JSON payload. Correct MultiByteToWideChar pattern already used elsewhere in same file. |
| `MarkdownPreview/src/PluginMain.cpp` | beNotified handles NPPN_BUFFERACTIVATED, NPPN_DARKMODECHANGED, SCN_MODIFIED, SCN_UPDATEUI | VERIFIED | All 4 cases present at lines 54/58/62/66 |
| `MarkdownPreview/src/PluginDefinition.cpp` | onBufferActivated with updateFileVirtualHost before renderMarkdown; onDarkModeChanged; onScnModified; onScnUpdateUi with full scroll logic; exportMarkdown | VERIFIED | All handlers implemented; updateFileVirtualHost called at line 126, renderMarkdown at line 127 (timing correct) |
| `MarkdownPreview/src/PluginDefinition.h` | NB_FUNC = 2; exportMarkdown() declared | VERIFIED | NB_FUNC = 2 at line 11; exportMarkdown declared at line 38 |
| `MarkdownPreview/assets/preview.html` | Full JS pipeline: markdown-it init, source_map rule, YAML strip, renderMarkdown, setTheme, setCustomCss, scrollToLine (full), exportHtml (full async), WebView2 message dispatcher | VERIFIED | All functions present (lines 129-370); message dispatcher with all 4 cases; async exportHtml with blobToDataUrl, cloneNode, CSS collect, base64 images, postMessage exportReady |
| `MarkdownPreview/assets/markdown-it.min.js` | markdown-it 14.1.1 UMD, window.markdownit global | VERIFIED | 123KB; "/*! markdown-it 14.1.1" header; window.markdownit global export confirmed |
| `MarkdownPreview/assets/markdown-it-task-lists.min.js` | markdown-it-task-lists 2.1.0 UMD (CDN serves 2.1.0 for 2.1.1 tag), window.markdownitTaskLists global | VERIFIED | 2.7KB; window.markdownitTaskLists export confirmed in UMD header; matches preview.html reference |
| `MarkdownPreview/assets/highlight.min.js` | highlight.js 11.11.1 core | VERIFIED | 128KB; contains hljs global |
| `MarkdownPreview/assets/hljs-github.min.css` | highlight.js GitHub light theme | VERIFIED | 1.3KB |
| `MarkdownPreview/assets/hljs-github-dark.min.css` | highlight.js GitHub dark theme | VERIFIED | 1.3KB |
| `MarkdownPreview/assets/github-markdown-light.css` | github-markdown-css 5.9.0 light variant | VERIFIED | 23KB; contains .markdown-body rules |
| `MarkdownPreview/assets/github-markdown-dark.css` | github-markdown-css 5.9.0 dark variant | VERIFIED | 23KB |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| PluginMain.cpp beNotified | PreviewPanel.renderMarkdown / scheduleRender / setTheme | onBufferActivated / onScnModified / onDarkModeChanged | WIRED | All 4 beNotified cases call corresponding handlers in PluginDefinition.cpp which call g_previewPanel methods |
| PreviewPanel.doRender | WebView2 PostWebMessageAsJson | nlohmann JSON {type:render, markdown, filePath} | WIRED (with bug) | PostWebMessageAsJson call is wired, but wstring conversion at line 337 corrupts non-ASCII payload |
| PreviewPanel.setTheme | WebView2 PostWebMessageAsJson | nlohmann JSON {type:theme, dark:bool} | WIRED | ASCII-only JSON; conversion bug does not affect correctness here |
| window.chrome.webview message event | renderMarkdown() / setTheme() / scrollToLine() / exportHtml() | switch on msg.type (4 cases) | WIRED | All 4 cases present and wired to correct functions |
| markdown-it md.render() | document.getElementById('preview').innerHTML | direct assignment after YAML strip and image path rewrite | WIRED | rewriteImagePaths called before md.render; result assigned to innerHTML |
| hljs.highlightAll() | pre > code elements in preview div | called after innerHTML assignment | WIRED | Present at preview.html line 227 |
| SCN_UPDATEUI in PluginMain | PreviewPanel.scrollToLine() | onScnUpdateUi() with SC_UPDATE_SELECTION\|SC_UPDATE_CONTENT filter | WIRED | Full implementation in PluginDefinition.cpp lines 172-200 |
| JS scrollToLine() | element with matching data-line | querySelectorAll('[data-line]') nearest-element scan | WIRED | Full implementation at preview.html lines 243-261 |
| onBufferActivated() | file.mdpreview virtual host | updateFileVirtualHost() called BEFORE renderMarkdown() | WIRED | Lines 126-127 in PluginDefinition.cpp — correct ordering |
| Export as HTML menu item | PreviewPanel via triggerExport() | exportMarkdown() in PluginDefinition.cpp | WIRED | funcItems[1] initialized with exportMarkdown; exportMarkdown calls g_previewPanel.triggerExport() |
| JS exportHtml() | C++ handleJsMessage() | window.chrome.webview.postMessage({type:exportReady, html:...}) | WIRED | add_WebMessageReceived wired in initWebView2; handleJsMessage dispatches exportReady to saveExportedHtml |
| C++ renderMarkdown | JS setCustomCss | j["customCss"] field in render message | NOT WIRED | customCss field is never populated in C++; JS infrastructure exists but never receives data |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|--------------|--------|-------------------|--------|
| preview.html renderMarkdown | markdown (from msg.markdown) | C++ PreviewPanel::renderMarkdown → Scintilla SCI_GETTEXT | Yes — real document text | FLOWING (for ASCII); BROKEN for non-ASCII due to wstring conversion bug |
| preview.html setTheme | isDark (from msg.dark) | C++ setTheme → NPPM_ISDARKMODEENABLED | Yes — real NPP dark mode state | FLOWING |
| preview.html scrollToLine | line (from msg.line) | C++ onScnUpdateUi → SCI_GETCURRENTPOS + SCI_LINEFROMPOSITION | Yes — real caret position | FLOWING |
| preview.html setCustomCss | customCss (from msg.customCss) | C++ renderMarkdown JSON payload | No — field never sent from C++ | DISCONNECTED |
| preview.html exportHtml | clone of #preview innerHTML | JS DOM serialization after render | Yes — rendered HTML | FLOWING (async, depends on render) |

### Behavioral Spot-Checks

| Behavior | Check | Result | Status |
|----------|-------|--------|--------|
| All 4 message dispatcher cases present | grep -c for all cases in preview.html | 4 | PASS |
| message handler addEventListener wired | grep for window.chrome.webview.addEventListener | 1 match | PASS |
| NPPN_BUFFERACTIVATED corrected to 1010 | grep for (NPPN_FIRST + 10) | Confirmed | PASS |
| All 6 NPP message constants present | grep for each in Notepad_plus_msgs.h | 6 matches | PASS |
| NB_FUNC = 2 for Export menu item | grep for NB_FUNC = 2 | Confirmed | PASS |
| UTF-8 BOM in saveExportedHtml | grep for 0xEF 0xBB 0xBF | Confirmed | PASS |
| customCss field in C++ render message | grep renderMarkdown JSON fields | Not present | FAIL |
| Utf8ToWide safe conversion for PostWebMessageAsJson | grep for wjson(jsonStr.begin(), jsonStr.end()) | 4 occurrences (lines 337/349/367/417) | FAIL |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| REND-01 | 02-01 | Preview panel auto-opens when .md file opened | SATISFIED | onBufferActivated auto-opens panel via toggle(); NPPN_BUFFERACTIVATED corrected to 1010 |
| REND-02 | 02-01 | Live preview updates (debounced) | SATISFIED | SCN_MODIFIED triggers scheduleRender(); 300ms SetTimer/KillTimer debounce with m_renderPending guard |
| REND-03 | 02-02 | Full GFM support | SATISFIED | markdown-it default preset (tables, strikethrough, linkify) + markdown-it-task-lists wired |
| REND-04 | 02-02 | Syntax highlighting with auto-detection | SATISFIED | highlight.js 11.11.1 hljs.highlightAll() called after innerHTML assignment |
| REND-05 | 02-03 | Local images with relative path resolution | SATISFIED | rewriteImagePaths() + file.mdpreview virtual host via ICoreWebView2_3 |
| REND-06 | 02-02 | YAML frontmatter hidden from output | SATISFIED | stripFrontmatter() strips --- delimited block before md.render() |
| THME-01 | 02-02 | Dark and light themes bundled | SATISFIED | github-markdown-light.css + github-markdown-dark.css + hljs theme pair; setTheme() swaps both atomically |
| THME-02 | 02-02 | User custom CSS | BLOCKED | JS setCustomCss() infrastructure present; C++ never reads custom.css or sends customCss field in render message |
| THME-03 | 02-01 | Auto-detect NPP dark/light mode | SATISFIED | NPPM_ISDARKMODEENABLED on startup; NPPN_DARKMODECHANGED handled |
| SCRL-01 | 02-03 | Scroll sync with editor caret | SATISFIED | Full onScnUpdateUi pipeline to JS scrollToLine nearest-element scrollIntoView |
| EXPT-01 | 02-04 | Export to standalone HTML with inlined CSS | SATISFIED | Full async exportHtml pipeline; CSS fetch; base64 image encoding; saveExportedHtml with UTF-8 BOM |

**Requirement THME-02 BLOCKED:** Mapped to Phase 2 only in REQUIREMENTS.md. Not addressed in any later phase. Must be fixed in Phase 2.

### Anti-Patterns Found

| File | Line(s) | Pattern | Severity | Impact |
|------|---------|---------|----------|--------|
| PreviewPanel.cpp | 337, 349, 367, 417 | `std::wstring wjson(jsonStr.begin(), jsonStr.end())` — unsafe byte-to-wchar_t copy of UTF-8 string | Blocker | Corrupts all non-ASCII markdown content in PostWebMessageAsJson messages; preview goes blank/error for international content |
| PreviewPanel.cpp | 337 | renderMarkdown JSON missing customCss field | Blocker | THME-02 feature non-functional; user custom CSS never sent to JS |
| PreviewPanel.cpp | 259-261 | GetEnvironmentVariableW return value not checked (from code review CR-03) | Warning | If LOCALAPPDATA absent, WebView2 user data path resolves to filesystem root; WebView2 may fail to initialize |
| PreviewPanel.cpp | 220-231 | m_webMessageReceivedToken never unregistered in destroy() (from code review WR-01) | Warning | Potential use-after-free if panel destroyed and re-initialized |
| assets/preview.html | 144-148 | stripFrontmatter does not handle UTF-8 BOM; \r\n edge case (from code review WR-05) | Warning | Files with BOM skip frontmatter stripping and render raw --- block |
| assets/preview.html | 196-203 | rewriteImagePaths regex [^)]+ breaks on filenames with parentheses e.g. image (1).png (from code review WR-06) | Warning | Images with parenthetical filenames (common on Windows) fail to load |

### Human Verification Required

None beyond the automated gaps — the identified failures are code-level and verifiable programmatically.

However, two behaviors should be tested manually once gaps are fixed:

#### 1. Non-ASCII Markdown Rendering

**Test:** Open a .md file containing non-ASCII content (e.g., Chinese characters `# 你好世界`, accented characters `café résumé`, emoji `🎉 Great job!`) in Notepad++
**Expected:** Preview panel renders the content correctly without showing "Preview unavailable" error state
**Why human:** Runtime WebView2 behavior cannot be verified without launching Notepad++

#### 2. Custom CSS Application (after THME-02 fix)

**Test:** Place a `custom.css` file at `%APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css` with a distinctive rule (e.g., `body { background: pink; }`), then open a .md file
**Expected:** Preview panel background turns pink, demonstrating custom CSS is loaded and applied
**Why human:** Requires file system setup, plugin runtime, and visual verification

### Gaps Summary

**2 gaps blocking full goal achievement:**

**Gap 1 — Non-ASCII content corruption (CR-01 from code review):** The `std::wstring wjson(jsonStr.begin(), jsonStr.end())` pattern at 4 call sites in PreviewPanel.cpp corrupts UTF-8 multibyte sequences when passed to `PostWebMessageAsJson`. This is a latent defect confirmed by the code review (CR-01) that was not fixed before completion. Any international user opening a .md file with non-ASCII content sees a broken preview. The fix is a mechanical replacement using `MultiByteToWideChar(CP_UTF8, ...)` — the correct pattern already exists elsewhere in the same file.

**Gap 2 — THME-02 custom CSS never sent (missing C++ wiring):** The JS side has complete infrastructure (`setCustomCss()`, `msg.customCss` dispatcher), but `PreviewPanel::renderMarkdown()` builds the JSON message without a `customCss` field. C++ never reads the `custom.css` file from the plugin config directory. The render message always delivers `msg.customCss === undefined` to JS, which falls back to `null` and clears any injected style. THME-02 is mapped to Phase 2 only and is not addressed in any later phase.

**Root cause relationship:** Both gaps were present at the time of the plan execution. The code review (02-REVIEW.md) identified CR-01 but no corrective action was taken before phase completion. Gap 2 was not flagged in the review — it was silently accepted as out-of-scope when the plan's custom CSS C++ integration note was not implemented.

---

_Verified: 2026-04-09T15:36:10Z_
_Verifier: Claude (gsd-verifier)_
