# Phase 2: Core Preview - Research

**Researched:** 2026-04-09
**Domain:** Notepad++ plugin — WebView2 messaging, markdown-it JS pipeline, Scintilla notifications, theme detection, scroll sync, HTML export
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01:** Panel auto-opens whenever a .md file is activated (NPPN_BUFFERACTIVATED — see note below), even if previously closed. No sticky-close logic.
- **D-02:** Debounced rendering — update preview on SCN_MODIFIED after 300ms pause. Full re-render every trigger (no incremental diff).
- **D-03:** YAML frontmatter (content between leading `---` delimiters) stripped before passing to markdown-it, not rendered as a table.
- **D-04:** Source-map precision — markdown-it injects `data-line="N"` on block-level elements. C++ sends caret line via PostWebMessageAsString; JS finds closest element and scrolls to it smoothly.
- **D-05:** Live dark/light theme detection via NPPN_DARKMODECHANGED. No restart required.
- **D-06:** Dark/light theme via `github-markdown-css` dark and light variants. Theme class applied on `<body>` — swapped by JS on C++ notification.
- **D-07:** Custom CSS from `%AppData%\Notepad++\plugins\config\MarkdownPreview\custom.css`. Absence silently ignored. Loaded as `<link>` after base theme.
- **D-08:** Local images resolved via second WebView2 virtual host mapping current file's parent directory. Relative image paths rewritten to virtual host URLs before rendering.
- **D-09:** HTML export is fully self-contained — local images base64-encoded, CSS inlined. No companion files.
- **D-10:** Exported file saved to same directory as source `.md`, `.md` extension replaced with `.html`. Overwrite without prompt.

### Claude's Discretion

- Exact debounce timer value (300ms recommended baseline)
- Virtual host name for file directory mapping
- JS scroll behavior (smooth vs. instant)
- Export trigger location (menu item name, keyboard shortcut)
- Syntax highlighting theme for highlight.js (github or github-dark matching current theme)

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within Phase 2 scope.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| REND-01 | Preview panel auto-opens when .md file is activated | NPPN_BUFFERACTIVATED handler, file extension check via NPPM_GETFULLCURRENTPATH, panel show/hide API |
| REND-02 | Preview updates live as user types (debounced) | SCN_MODIFIED + NPPM_ADDSCNMODIFIEDFLAGS, Win32 SetTimer/KillTimer debounce, SCI_GETTEXT retrieval |
| REND-03 | Full GFM — tables, fenced code blocks, task lists, strikethrough, autolinks | markdown-it 14.1.1 with `preset: 'default'`, `options.html=false, tables=true, strikethrough=true, linkify=true` + markdown-it-task-lists plugin |
| REND-04 | Syntax highlighting in fenced code blocks | highlight.js 11.11.1 `hljs.highlightAll()` called after markdown-it renders; theme swapped via CSS link |
| REND-05 | Local images with relative path resolution | Second `SetVirtualHostNameToFolderMapping` for file parent dir; JS rewrites `./img.png` → `https://file.mdpreview/img.png` before render |
| REND-06 | YAML frontmatter hidden | JS regex strip before markdown-it parse: `/^---[\s\S]*?---\n?/` |
| THME-01 | Dark and light themes bundled | `github-markdown-light.css` + `github-markdown-dark.css` bundled in assets; hljs `github.min.css` / `github-dark.min.css` swapped in pair |
| THME-02 | User custom CSS | `<link id="custom-css">` tag; C++ reads file, sends path or content; JS swaps/injects on message |
| THME-03 | Preview theme auto-detects Notepad++ dark/light | NPPM_ISDARKMODEENABLED on startup + NPPN_DARKMODECHANGED listener; sends `{type:"theme", dark:true/false}` message |
| SCRL-01 | Preview scroll syncs with editor caret | SCN_UPDATEUI / SCN_MODIFIED triggers `{type:"scroll", line:N}`; JS finds `[data-line]` element via source map |
| EXPT-01 | Export to standalone HTML with inlined CSS | JS-side: fetch all `<img src>` → base64 dataURIs, collect `<link>` CSS text, serialize DOM; C++ saves to disk path from JS→C++ message |
</phase_requirements>

---

## Summary

Phase 2 builds the entire rendering pipeline on top of the Phase 1 WebView2 host. The C++ side expands to: watch three notification types (NPPN_BUFFERACTIVATED for auto-open + initial render, SCN_MODIFIED for live updates, NPPN_DARKMODECHANGED for theme), retrieve document text from Scintilla, and communicate with the JS side via PostWebMessageAsString/PostWebMessageAsJson. A second virtual host mapping resolves local images without exposing the file system. The JavaScript side (preview.html) receives structured messages and orchestrates markdown-it, highlight.js, and github-markdown-css.

The core implementation has two non-trivial sub-problems: (1) injecting `data-line` source-map attributes into markdown-it's output using renderer rule overrides on the `token.map` property, and (2) the debounce pattern using Win32 SetTimer/KillTimer, which requires the panel HWND to receive WM_TIMER messages. Both are solved patterns — no custom algorithms needed.

YAML frontmatter stripping is a simple regex applied to the raw text before markdown-it sees it. HTML export is orchestrated entirely on the JS side using async fetch/base64 conversion of image URLs, then a JS→C++ message delivers the finished HTML string for C++ to write to disk.

**Primary recommendation:** Implement the JS message protocol first (render/theme/scroll/export message types), then wire the C++ notification handlers to produce those messages — this order lets you test the JS side independently by manually posting messages via DevTools.

---

## Standard Stack

### Core (JS, bundled in assets/)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| markdown-it | 14.1.1 | Markdown → HTML parser | CLAUDE.md locked; CommonMark + GFM extensible via plugins |
| markdown-it-task-lists | 2.1.1 | Renders `- [x]` checkboxes | Official plugin for GFM task lists |
| highlight.js | 11.11.1 | Code block syntax highlighting | CLAUDE.md locked; runtime auto-detection, zero build step |
| github-markdown-css | 5.9.0 | Base markdown styling (light + dark) | CLAUDE.md specifies 5.8.1; npm registry at 5.9.0 — use 5.9.0 |

**Version note:** `github-markdown-css` is at 5.9.0 [VERIFIED: npm registry, 2026-02-03]. CLAUDE.md references 5.8.1. Plan should use 5.9.0 as the current stable.

### Supporting (JS)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| (none external needed for export) | — | HTML export | Uses native `fetch()` + `FileReader`/`btoa()` in WebView2 JS |

### Supporting (C++)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| nlohmann/json | 3.11.3 | JSON for C++→JS messages | Already in project; use for serializing render/theme/scroll/export messages |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Win32 SetTimer debounce | std::async / thread sleep | SetTimer posts WM_TIMER to message queue — no threading complications; correct choice for Notepad++ plugin |
| JS renderer rule override for data-line | markdown-it-source-map npm package | Package is 9 years old (last published). Implement directly via `md.core.ruler.push` or renderer rules — 15 lines of JS |
| Regex YAML strip | gray-matter npm package | Adding gray-matter for a 3-line regex is unnecessary overhead; no npm build pipeline in this project |

**Installation (download to assets/ — no build pipeline):**
```bash
# Download minified files to MarkdownPreview/assets/
# markdown-it
curl -o assets/markdown-it.min.js "https://cdn.jsdelivr.net/npm/markdown-it@14.1.1/dist/markdown-it.min.js"

# markdown-it-task-lists (UMD build)
curl -o assets/markdown-it-task-lists.min.js "https://cdn.jsdelivr.net/npm/markdown-it-task-lists@2.1.1/dist/markdown-it-task-lists.min.js"

# highlight.js (core only — auto-detection is in highlight.min.js)
curl -o assets/highlight.min.js "https://cdn.jsdelivr.net/gh/highlightjs/cdn-release@11.11.1/build/highlight.min.js"
curl -o assets/hljs-github.min.css "https://cdn.jsdelivr.net/gh/highlightjs/cdn-release@11.11.1/build/styles/github.min.css"
curl -o assets/hljs-github-dark.min.css "https://cdn.jsdelivr.net/gh/highlightjs/cdn-release@11.11.1/build/styles/github-dark.min.css"

# github-markdown-css (light and dark variants)
curl -o assets/github-markdown-light.css "https://cdn.jsdelivr.net/npm/github-markdown-css@5.9.0/github-markdown-light.css"
curl -o assets/github-markdown-dark.css "https://cdn.jsdelivr.net/npm/github-markdown-css@5.9.0/github-markdown-dark.css"
```

---

## Architecture Patterns

### Recommended File Structure
```
MarkdownPreview/
├── src/
│   ├── PreviewPanel.h          # Add: renderMarkdown(), setTheme(), scrollToLine(), exportHtml() public methods
│   ├── PreviewPanel.cpp        # Add: PostWebMessageAsJson helpers, second virtual host, debounce timer
│   ├── PluginMain.cpp          # Add: NPPN_BUFFERACTIVATED, NPPN_DARKMODECHANGED, SCN_MODIFIED cases
│   ├── PluginDefinition.cpp    # Add: Export menu item, NPPM_ADDSCNMODIFIEDFLAGS in onNppReady()
│   ├── PluginDefinition.h      # NB_FUNC = 2 (Toggle + Export)
│   ├── Notepad_plus_msgs.h     # Add missing constants (see below)
│   ├── Scintilla.h             # Add: SCI_GETCURRENTPOS, SCI_LINEFROMPOSITION, SCI_GETCODEPAGE, SCN_ flags
│   └── Settings.h/cpp          # Add: debounceMs, theme tracking
└── assets/
    ├── preview.html            # New — replaces welcome.html when .md file active
    ├── markdown-it.min.js
    ├── markdown-it-task-lists.min.js
    ├── highlight.min.js
    ├── hljs-github.min.css
    ├── hljs-github-dark.min.css
    ├── github-markdown-light.css
    ├── github-markdown-dark.css
    └── welcome.html            # Keep — shown when no .md file is active
```

### Pattern 1: Notepad++ Notification Handler Expansion

**What:** Add three new cases to the `beNotified()` switch in `PluginMain.cpp`.
**When to use:** Every new NPP event the plugin responds to.

```cpp
// Source: npp-user-manual.org/docs/plugin-communication + VERIFIED: official NPP source
case NPPN_BUFFERACTIVATED:
    // nmhdr.idFrom = BufferID of the newly active buffer
    onBufferActivated(notification->nmhdr.idFrom);
    break;
case NPPN_DARKMODECHANGED:
    onDarkModeChanged();
    break;
// SCN_MODIFIED is forwarded from Scintilla (nmhdr.hwndFrom is Scintilla HWND)
// SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT are forwarded by default in NPP
// NPPM_ADDSCNMODIFIEDFLAGS ensures they arrive even if NPP default set changes
```

**NPPN_BUFFERACTIVATED** fires whenever the user switches tabs or opens a file. `notification->nmhdr.idFrom` carries the BufferID (not a file path). Get the path via:
```cpp
// Source: VERIFIED official NPP Notepad_plus_msgs.h (NPPMSG + 58)
wchar_t path[MAX_PATH] = {};
::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID,
    notification->nmhdr.idFrom, reinterpret_cast<LPARAM>(path));
// Check if path ends with .md before acting
```

### Pattern 2: Retrieving Document Text from Scintilla

**What:** Get the full text of the current document via Scintilla messages.
**When to use:** On every debounced re-render trigger.

```cpp
// Source: VERIFIED Scintilla docs + NppGateway.h patterns
// Step 1: Get the active Scintilla handle
int sciId = 0;
::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0,
    reinterpret_cast<LPARAM>(&sciId));
HWND hSci = (sciId == 0) ? nppData._scintillaMainHandle
                          : nppData._scintillaSecondHandle;

// Step 2: Get text length and retrieve text (UTF-8 from Scintilla)
int len = static_cast<int>(::SendMessage(hSci, SCI_GETLENGTH, 0, 0));
std::string utf8Text(len + 1, '\0');
::SendMessage(hSci, SCI_GETTEXT, len + 1,
    reinterpret_cast<LPARAM>(utf8Text.data()));
utf8Text.resize(len);

// Step 3: Convert to std::wstring for PostWebMessageAsJson
// (use MultiByteToWideChar with CP_UTF8)
```

**Key insight:** Scintilla stores text as UTF-8 (`SCI_GETTEXT` returns `char*` UTF-8, not wide chars). Always use `CP_UTF8` when converting.

### Pattern 3: Debounce with Win32 SetTimer

**What:** Delay re-render until 300ms after last keystroke. Cancel pending timer on new keystrokes.
**When to use:** SCN_MODIFIED handler.

```cpp
// Source: VERIFIED Win32 docs + common Notepad++ plugin pattern
// In PreviewPanel — panel's HWND receives WM_TIMER
static const UINT_PTR DEBOUNCE_TIMER_ID = 1;

void PreviewPanel::scheduleRender() {
    // KillTimer is safe to call even if no timer is running (returns FALSE, not error)
    ::KillTimer(m_hPanel, DEBOUNCE_TIMER_ID);
    ::SetTimer(m_hPanel, DEBOUNCE_TIMER_ID, 300, nullptr);
    // WM_TIMER → wndProc → calls actualRender()
}

// In wndProc:
case WM_TIMER:
    if (wParam == DEBOUNCE_TIMER_ID) {
        ::KillTimer(hWnd, DEBOUNCE_TIMER_ID);  // one-shot: kill before render
        PreviewPanel* self = reinterpret_cast<PreviewPanel*>(
            ::GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (self) self->doRender();
        return 0;
    }
    break;
```

**Pitfall:** `KillTimer` does NOT flush already-queued `WM_TIMER` messages from the queue. If the render takes longer than 300ms, a second WM_TIMER may still fire. Guard `doRender()` with a flag.

### Pattern 4: WebView2 Message Protocol (C++ → JS)

**What:** Typed JSON messages sent from C++ to JS via `PostWebMessageAsJson`.
**When to use:** Render trigger, theme switch, scroll sync, export trigger.

```cpp
// Source: VERIFIED learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/communicate-btwn-web-native
// C++ sends JSON string:
std::wstring msg = L"{\"type\":\"render\",\"markdown\":\"" + escapedMarkdown + L"\",\"filePath\":\"" + escapedPath + L"\"}";
m_webview->PostWebMessageAsJson(msg.c_str());

// Or use nlohmann/json to build the payload safely:
nlohmann::json j;
j["type"] = "render";
j["markdown"] = nlohmann::json(utf8Markdown);  // handles escaping
j["filePath"] = nlohmann::json(utf8FilePath);
std::string jsonStr = j.dump();
std::wstring wjson(jsonStr.begin(), jsonStr.end());
m_webview->PostWebMessageAsJson(wjson.c_str());
```

**CRITICAL: Use nlohmann/json for payload construction.** Do NOT manually concatenate JSON with markdown content — markdown text contains quotes, backslashes, and newlines that will break JSON parsing if not escaped. nlohmann/json handles all escaping correctly.

**JS receives:**
```javascript
// Source: VERIFIED learn.microsoft.com webview2 interop guide
window.chrome.webview.addEventListener('message', (event) => {
    const msg = event.data;  // already parsed to JS object via PostWebMessageAsJson
    if (msg.type === 'render') renderMarkdown(msg.markdown, msg.filePath);
    else if (msg.type === 'theme') setTheme(msg.dark);
    else if (msg.type === 'scroll') scrollToLine(msg.line);
    else if (msg.type === 'export') exportHtml();
});
```

### Pattern 5: markdown-it Source Map Data-Line Injection

**What:** Inject `data-line="N"` on block-level elements for scroll sync.
**When to use:** Applied once during markdown-it initialization, not per-render.

The `token.map` property on block tokens is `[lineStart, lineEnd]` (0-indexed). Override the default `renderToken` to add the attribute when present.

```javascript
// Source: VERIFIED markdown-it docs (token.map property) + pattern from NppAnotherMarkdown
// Register a core rule that injects data-line on all block open tokens:
md.core.ruler.push('source_map', (state) => {
    for (const token of state.tokens) {
        if (token.map && token.type !== 'inline') {
            token.attrSet('data-line', String(token.map[0] + 1)); // 1-indexed
        }
    }
});
// Tokens with token.map: heading_open, paragraph_open, fence, table_open,
// bullet_list_open, ordered_list_open, blockquote_open, code_block, hr
```

**Why `token.map[0] + 1`:** markdown-it uses 0-indexed lines; Scintilla/NPP also use 0-indexed lines internally but the caret line sent from C++ should be 0-indexed to match. Match the indexing convention explicitly in C++ when sending `{type:"scroll", line: caretLine}`.

### Pattern 6: Second Virtual Host for Image Resolution

**What:** Map a second virtual hostname to the active file's parent directory. JS rewrites image src paths.
**When to use:** Every time the active file changes.

```cpp
// Source: VERIFIED ICoreWebView2_3 docs — SetVirtualHostNameToFolderMapping
// Called when file path changes (on buffer activation):
void PreviewPanel::updateFileVirtualHost(const std::wstring& filePath) {
    // Extract parent directory
    std::wstring dir = filePath;
    size_t pos = dir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) dir = dir.substr(0, pos);

    wil::com_ptr<ICoreWebView2_3> webview3;
    m_webview->QueryInterface(IID_PPV_ARGS(&webview3));
    if (webview3) {
        // Clear old mapping first (safe even if no mapping exists)
        webview3->ClearVirtualHostNameToFolderMapping(L"file.mdpreview");
        // Set new mapping
        webview3->SetVirtualHostNameToFolderMapping(
            L"file.mdpreview",
            dir.c_str(),
            COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
    }
}
```

**JS side — rewrite relative image paths before rendering:**
```javascript
// Source: ASSUMED pattern; no official markdown-it docs for this specific rewrite
function rewriteImagePaths(markdownText) {
    // Rewrite ![alt](./path) → ![alt](https://file.mdpreview/path)
    // Handles: ./x, ../x, subdir/x (relative), leaves http/https alone
    return markdownText.replace(
        /!\[([^\]]*)\]\((?!https?:\/\/|\/\/)(\.?\.?[^)]+)\)/g,
        (match, alt, path) => `![${alt}](https://file.mdpreview/${path.replace(/^\.\//, '')})`
    );
}
```

**Important:** The docs say "changes to the mapping might not be applied to the current page and a reload of the page is needed to apply the new mapping." For preview.html (our own page), this means update the mapping BEFORE sending the render message, not after. The preview.html never navigates away, so mappings persist per-load.

### Pattern 7: JS → C++ Message (for Export)

**What:** JS sends the finished HTML string back to C++ for file I/O.
**When to use:** Export flow — JS does the serialization, C++ does the disk write.

```cpp
// Source: VERIFIED learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/communicate-btwn-web-native
// Wire once in initWebView2 (after m_webview is valid):
m_webview->add_WebMessageReceived(
    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
            wil::unique_cotaskmem_string msg;
            args->TryGetWebMessageAsString(&msg);
            std::wstring message = msg.get();
            // Parse with nlohmann: if type == "exportReady", write htmlContent to disk
            handleJsMessage(message);
            return S_OK;
        }).Get(), &m_webMessageReceivedToken);
```

### Pattern 8: Dark Mode Detection at Startup

**What:** Check if NPP is in dark mode when the plugin initializes.
**When to use:** In `onNppReady()` after panel creation.

```cpp
// Source: VERIFIED npp-user-manual.org (NPPM_ISDARKMODEENABLED = NPPMSG + 107)
bool isDark = (BOOL)::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0) != FALSE;
g_previewPanel.setTheme(isDark);
```

### Header Constants to Add

The current `Notepad_plus_msgs.h` and `Scintilla.h` are missing constants needed for Phase 2. All values below are verified from the official NPP repository.

**Notepad_plus_msgs.h additions:**
```cpp
// Source: VERIFIED notepad-plus-plus/PowerEditor/src/MISC/PluginsManager/Notepad_plus_msgs.h
#define NPPM_GETCURRENTSCINTILLA        (NPPMSG + 4)    // returns Scintilla ID (0 or 1)
#define NPPM_GETCURRENTBUFFERID         (NPPMSG + 60)   // returns active BufferID
#define NPPM_GETFULLPATHFROMBUFFERID    (NPPMSG + 58)   // WPARAM=bufferID, LPARAM=wchar_t[MAX_PATH]
#define NPPM_ISDARKMODEENABLED          (NPPMSG + 107)  // returns BOOL
#define NPPM_ADDSCNMODIFIEDFLAGS        (NPPMSG + 117)  // registers SCN_MODIFIED flag interest
#define NPPN_DARKMODECHANGED            (NPPN_FIRST + 27)  // = 1027
// NPPN_BUFFERACTIVATED is already (NPPN_FIRST + 9) in current header — but VERIFY:
// Official source has it as (NPPN_FIRST + 10) = 1010.
// Current header says (NPPN_FIRST + 9) = 1009 — THIS IS A DISCREPANCY (see Pitfall 1).
```

**Scintilla.h additions:**
```cpp
// Source: VERIFIED scintilla.nim (nppnim repo) cross-referenced with Scintilla docs
#define SCI_GETCURRENTPOS       2008    // returns current caret byte position
#define SCI_LINEFROMPOSITION    2166    // WPARAM=position, returns line number (0-indexed)
#define SCI_GETCODEPAGE         2137    // returns encoding code page
// SCN_MODIFIED notification code
#define SCN_MODIFIED            2008    // NOTE: same value as SCI_GETCURRENTPOS — SCN_ codes
                                        // are in nmhdr.code, SCI_ codes are SendMessage IDs;
                                        // they do not conflict
#define SC_MOD_INSERTTEXT       0x0001
#define SC_MOD_DELETETEXT       0x0002
#define SC_PERFORMED_UNDO       0x0020
#define SC_PERFORMED_REDO       0x0040
```

### Anti-Patterns to Avoid

- **Manual JSON string building with markdown content:** Backslashes, quotes, and newlines in markdown will produce malformed JSON. Always use nlohmann/json.
- **Calling SetVirtualHostNameToFolderMapping after Navigate:** Mapping changes may not apply to the current page. Update the mapping, then send the render message.
- **Using NavigateToString to deliver markdown HTML:** Has a 2MB limit [CITED: weblog.west-wind.com/posts/2024/Jul/22]. Use PostWebMessageAsJson instead — the JS side renders from the raw markdown string.
- **initHighlightingOnLoad():** Deprecated. Use `hljs.highlightAll()`. [VERIFIED: highlight.js docs]
- **Polling SCN_MODIFIED without NPPM_ADDSCNMODIFIEDFLAGS:** Post NPP v8.7.7, the default forwarded flags may change. Call `NPPM_ADDSCNMODIFIEDFLAGS` in `onNppReady()` to explicitly request `SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT`.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSON encoding of markdown string | String concat with escaping | nlohmann/json `j["markdown"] = utf8text` | Backslashes, quotes, newlines, Unicode all handled correctly |
| Markdown parsing | Custom parser | markdown-it 14.1.1 | GFM, extensible, battle-tested |
| Syntax highlighting | Custom colorizer | highlight.js 11.11.1 `hljs.highlightAll()` | 185+ languages, auto-detection |
| YAML frontmatter detection | Custom state machine | Simple regex `/^---[\s\S]*?---\n?/` applied before parse | 3 lines; no library needed |
| Image base64 encoding (JS) | Custom XHR | Native `fetch(src).then(r=>r.blob()).then(blob=>blobToBase64(blob))` | Browser-native, no library |
| Markdown source map | `markdown-it-source-map` npm | `md.core.ruler.push` with `token.map` (15 lines) | The npm package is 9 years old and unsupported |

**Key insight:** The most common mistake is building markdown text delivery via string substitution in HTML. The correct pattern is: C++ sends raw markdown via PostWebMessageAsJson, JS renders it. This completely avoids the 2MB NavigateToString limit.

---

## Common Pitfalls

### Pitfall 1: NPPN_BUFFERACTIVATED Value Discrepancy
**What goes wrong:** Plugin silently never receives buffer activation events.
**Why it happens:** The current project's `Notepad_plus_msgs.h` defines `NPPN_BUFFERACTIVATED` as `(NPPN_FIRST + 9)` = 1009. The official Notepad++ source has it as `(NPPN_FIRST + 10)` = 1010. Using the wrong value means the switch case never matches.
**How to avoid:** Update the project's `Notepad_plus_msgs.h` to match the official source value of 1010 before adding the BUFFERACTIVATED case. [VERIFIED: official NPP source via WebFetch]
**Warning signs:** Auto-open never fires; no render on file switch.

### Pitfall 2: Markdown Text Encoding Mismatch
**What goes wrong:** Non-ASCII characters (accented letters, CJK, emoji) render as garbage.
**Why it happens:** Scintilla returns UTF-8 bytes via SCI_GETTEXT. If these are treated as Windows-1252 or blindly widened character-by-character, non-ASCII characters break.
**How to avoid:** Use `MultiByteToWideChar(CP_UTF8, ...)` to convert SCI_GETTEXT output to wstring before JSON serialization, OR pass the UTF-8 bytes directly to nlohmann/json (which handles UTF-8 correctly).
**Warning signs:** Accented characters show as `?` or boxes in the preview.

### Pitfall 3: Virtual Host Mapping Timing
**What goes wrong:** Images 404 after switching to a new file.
**Why it happens:** If `SetVirtualHostNameToFolderMapping` is called after `PostWebMessageAsJson` (render message), the page may use the old mapping for the first render cycle.
**How to avoid:** Always call `ClearVirtualHostNameToFolderMapping` + `SetVirtualHostNameToFolderMapping` BEFORE sending the render message. [CITED: ICoreWebView2_3 docs — "reload of the page may be needed to apply new mapping"]
**Warning signs:** First image load after file switch shows 404; subsequent renders show image correctly.

### Pitfall 4: WM_TIMER Latent Messages After KillTimer
**What goes wrong:** Double render fires — one from scheduled timer, one from latent WM_TIMER already in queue.
**Why it happens:** KillTimer does not remove WM_TIMER messages already posted to the queue. [VERIFIED: Win32 KillTimer docs]
**How to avoid:** In the WM_TIMER handler, call KillTimer immediately (making it one-shot), and guard `doRender()` with a `m_renderPending` bool flag reset before the render.
**Warning signs:** Two rapid renders with no actual text change; occasional stutter during fast typing.

### Pitfall 5: JSON Payload Size for Large Files
**What goes wrong:** PostWebMessageAsJson silently fails or crashes on very large documents.
**Why it happens:** While PostWebMessageAsJson has no documented hard limit (unlike NavigateToString's 2MB), very large JSON payloads traverse COM/IPC boundary and can cause performance issues. Files >1MB of markdown are unusual but possible.
**How to avoid:** The pattern of sending raw markdown (not rendered HTML) via PostWebMessageAsJson is correct and efficient. Rendering happens in JS, so the payload is just the source text. A 1MB markdown file is ~1MB of JSON string — well within practical limits. No mitigation needed for Phase 2.
**Warning signs:** Preview stops updating on very large files; check file size. If needed, defer large-file handling to Phase 3.

### Pitfall 6: github-markdown-css Theme Class Application
**What goes wrong:** Light/dark theme doesn't switch; all text renders in wrong color.
**Why it happens:** `github-markdown-css` 5.x requires the `.markdown-body` class on the content container AND a `data-color-mode` attribute or the correct CSS file. The auto-switching `github-markdown.css` uses `@media (prefers-color-scheme)` — which in WebView2 follows the *system* preference, not Notepad++'s per-application theme.
**How to avoid:** Use the explicit light/dark CSS files (`github-markdown-light.css` / `github-markdown-dark.css`) and swap the `<link>` href via JS message. Do NOT rely on `prefers-color-scheme` — it reflects Windows system theme, not NPP's setting. [VERIFIED: github-markdown-css repo]
**Warning signs:** Theme always matches system, not NPP's explicit setting.

### Pitfall 7: NPPM_ADDSCNMODIFIEDFLAGS Version Requirement
**What goes wrong:** Plugin doesn't receive SCN_MODIFIED on older Notepad++ versions.
**Why it happens:** `NPPM_ADDSCNMODIFIEDFLAGS` was introduced in NPP v8.7.7. On older NPP, the SendMessage returns a non-zero error or is silently ignored. NPP still forwards `SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT` by default — so the real risk is if NPP changes defaults in future.
**How to avoid:** Call `NPPM_ADDSCNMODIFIEDFLAGS` defensively in `onNppReady()`. On older NPP it will fail silently; on newer NPP it is correct. The default forwarded flags (INSERT + DELETE + UNDO + REDO) already include what Phase 2 needs. [CITED: NPP community forum — introduced v8.7.7]
**Warning signs:** Live preview only works on v8.7.7+ but that is the current target. Minimum supported version should be documented.

---

## Code Examples

### preview.html — Initialization Pattern
```javascript
// Source: VERIFIED markdown-it docs (window.markdownit browser UMD) + highlight.js docs
const md = window.markdownit({
    html: false,        // don't allow raw HTML passthrough (security)
    linkify: true,      // auto-detect URLs
    typographer: false  // keep off for now — Phase 3 optional
}).use(window.markdownitTaskLists, { enabled: true });

// Source map: inject data-line on block-level tokens
// Source: VERIFIED from token.map property in markdown-it architecture docs
md.core.ruler.push('source_map', (state) => {
    for (const token of state.tokens) {
        if (token.map && token.map.length >= 2 && !token.type.endsWith('_close')) {
            token.attrSet('data-line', String(token.map[0])); // 0-indexed to match Scintilla
        }
    }
});

// Tables require 'tables' option — it's ON by default in markdown-it 14.x
// Strikethrough requires 'strikethrough' option
const md2 = window.markdownit({ tables: true, strikethrough: true });
// Actually for markdown-it 14.x these are enabled in the DEFAULT preset:
// md = markdownit()  — default preset enables tables and strikethrough
// Use markdownit('commonmark') only for strict CommonMark (disables GFM extensions)
```

**Note on GFM options:** markdown-it's default preset (`markdownit()` with no preset argument) includes tables and strikethrough. [ASSUMED — based on markdown-it 14.x behavior; verify via testing]. The `'commonmark'` preset disables them.

### Scroll Sync — JS Side
```javascript
// Source: ASSUMED pattern; verified by NppAnotherMarkdown architecture description
function scrollToLine(line) {
    // Find the element with data-line closest to (<=) the target line
    const elements = document.querySelectorAll('[data-line]');
    let best = null;
    for (const el of elements) {
        const elLine = parseInt(el.dataset.line, 10);
        if (elLine <= line) best = el;
        else break;
    }
    if (best) best.scrollIntoView({ behavior: 'smooth', block: 'start' });
}
```

### YAML Frontmatter Strip — JS
```javascript
// Source: VERIFIED pattern (multiple sources); regex approach confirmed adequate for Phase 2
function stripFrontmatter(markdown) {
    if (markdown.startsWith('---')) {
        const end = markdown.indexOf('\n---', 3);
        if (end !== -1) return markdown.slice(end + 4).trimStart();
    }
    return markdown;
}
```

### HTML Export — JS Side (Outline)
```javascript
// Source: VERIFIED fetch/FileReader pattern from MDN + WebView2 JS→C++ message pattern
async function exportHtml() {
    // 1. Collect CSS text from all <link rel="stylesheet"> elements
    const cssLinks = [...document.querySelectorAll('link[rel=stylesheet]')];
    const cssTexts = await Promise.all(
        cssLinks.map(link => fetch(link.href).then(r => r.text()))
    );
    const inlinedCss = cssTexts.join('\n');

    // 2. Encode local images to base64 dataURIs
    const body = document.getElementById('preview');
    const images = [...body.querySelectorAll('img')];
    await Promise.all(images.map(async img => {
        try {
            const resp = await fetch(img.src);
            const blob = await resp.blob();
            img.src = await blobToDataUrl(blob);
        } catch { /* remote or unresolvable image — leave as-is */ }
    }));

    // 3. Serialize to standalone HTML
    const html = `<!DOCTYPE html><html><head><style>${inlinedCss}</style></head>`
               + `<body class="markdown-body">${body.innerHTML}</body></html>`;

    // 4. Send to C++ for disk write
    window.chrome.webview.postMessage(JSON.stringify({ type: 'exportReady', html }));
}

function blobToDataUrl(blob) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => resolve(reader.result);
        reader.onerror = reject;
        reader.readAsDataURL(blob);
    });
}
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `initHighlightingOnLoad()` | `hljs.highlightAll()` | hljs v11.0 | Old function deprecated, remove in v12 |
| `markdownit('gfm')` preset | `markdownit()` default preset | markdown-it v13+ | Default preset has GFM extensions; 'gfm' preset removed |
| `prefers-color-scheme` for app theme | Explicit CSS file swap + app notification | N/A (always correct for non-browser) | Must not use media query when app controls theme independently of OS |
| NPPN_FILEACTIVATED | NPPN_BUFFERACTIVATED | NPP architecture | The correct notification for tab switches; file-based ones fire less often |

**Deprecated/outdated:**
- `initHighlightingOnLoad()`: removed in hljs 12.0, use `hljs.highlightAll()`
- `markdown-it-source-map` (npm): last published 9 years ago, do not use

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `markdownit()` default preset enables tables and strikethrough | Code Examples, Phase Requirements | If wrong, must pass explicit options `{tables:true}` or use a preset — easy fix in testing |
| A2 | JS regex `rewriteImagePaths` handles all relative path formats correctly | Pattern 6 | If wrong, images with unusual relative paths (e.g., `../img.png`) won't display — test with edge cases |
| A3 | `PostWebMessageAsJson` with markdown content up to ~500KB works reliably | Pitfall 5 | If there's an undocumented size limit lower than expected, need virtual-host-based content delivery |
| A4 | `md.core.ruler.push` with `token.map` correctly annotates all block-level elements | Pattern 5 | If some block tokens don't have `token.map`, those elements won't be scroll targets — acceptable degradation |
| A5 | `ClearVirtualHostNameToFolderMapping` followed by `SetVirtualHostNameToFolderMapping` updates apply before the next render message is processed | Pitfall 3 | Race condition possible if WebView2 processes messages asynchronously — mitigate with short delay or NavigateToString-after-mapping if needed |

**User-confirmable assumption:** A1 should be verified empirically in the first Wave 0 task that sets up preview.html.

---

## Open Questions

1. **SCN_MODIFIED forwarding on older Notepad++ versions**
   - What we know: NPP v8.7.7 introduced `NPPM_ADDSCNMODIFIEDFLAGS`; default forwarding includes INSERT+DELETE
   - What's unclear: Minimum supported NPP version for this plugin — if targeting very old versions, defensive handling needed
   - Recommendation: Target NPP 8.x (current). Call `NPPM_ADDSCNMODIFIEDFLAGS` defensively; no hard dependency on v8.7.7+.

2. **markdown-it task-lists UMD browser build**
   - What we know: `markdown-it-task-lists@2.1.1` has a `dist/` directory with UMD build based on npm package structure
   - What's unclear: Whether the UMD build exposes itself as `window.markdownitTaskLists` or requires different init
   - Recommendation: Confirm by inspecting the downloaded file header. Most markdown-it plugins expose as `window.markdownit[PluginName]`.

3. **Virtual host mapping updates and page state**
   - What we know: The docs say mapping changes may require a page reload to apply
   - What's unclear: Whether `ClearVirtualHostNameToFolderMapping` + immediate re-map is reliable for our use case (preview.html stays loaded, only file path changes)
   - Recommendation: If images don't appear after file switch, add a `Navigate(previewUrl)` after the mapping update. This reloads preview.html but is harmless. Alternative: pass the full absolute path via the render message and do URL construction in JS.

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Visual Studio (MSBuild) | C++ compilation | Yes | VS 18 (2026) Professional at `C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\` | — |
| WebView2 Runtime | Plugin rendering | Yes | 146.0.3856.97 / 146.0.3856.109 at `C:\Program Files (x86)\Microsoft\EdgeWebView\Application\` | — |
| Node.js | Asset download scripts | Yes | v23.6.1 | Use PowerShell Invoke-WebRequest |
| npm | Package info lookup | Yes | Bundled with Node | — |

**Note:** Developer machine has VS 18 (2026) Professional, not VS2022 (v17). The project uses `PlatformToolset v145` (confirmed in STATE.md). Build system is functional.

**Missing dependencies with no fallback:** None — all required tools are available.

---

## Project Constraints (from CLAUDE.md)

**Required patterns:**
- C++ native plugin, MSVC toolset v145 (VS 2026 on dev machine)
- WebView2 SDK 1.0.3856.49 via NuGet
- JS libraries: markdown-it 14.1.1, highlight.js 11.11.1, github-markdown-css (5.8.1 per CLAUDE.md, 5.9.0 per npm registry)
- Pre-download minified JS/CSS to `assets/`; no npm build pipeline
- Virtual host pattern: `https://appassets.mdpreview/` for plugin assets
- Settings JSON at `%AppData%\Notepad++\plugins\config\MarkdownPreview.json`
- Custom CSS at `%AppData%\Notepad++\plugins\config\MarkdownPreview\custom.css`
- No context menus, no DevTools, no status bar in WebView2 (already configured in Phase 1)

**Forbidden:**
- C# / .NET dependency
- CEF (Chromium Embedded Framework)
- Shiki for syntax highlighting (use highlight.js)
- MathJax (use KaTeX — Phase 3 concern)
- HTML/PDF via NavigateToString for large content (2MB limit)
- Manual JSON string concatenation with user content

---

## Sources

### Primary (HIGH confidence)
- [Official Notepad_plus_msgs.h — notepad-plus-plus GitHub](https://raw.githubusercontent.com/notepad-plus-plus/notepad-plus-plus/master/PowerEditor/src/MISC/PluginsManager/Notepad_plus_msgs.h) — all NPPM_/NPPN_ constant values
- [WebView2 ICoreWebView2_3 — Microsoft Docs](https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2_3?view=webview2-1.0.3650.58) — SetVirtualHostNameToFolderMapping API
- [WebView2 Interop Guide — Microsoft Docs](https://learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/communicate-btwn-web-native) — PostWebMessageAsJson, add_WebMessageReceived patterns
- [Notepad++ Plugin Communication — npp-user-manual.org](https://npp-user-manual.org/docs/plugin-communication/) — NPPN_ values, NPPM_ISDARKMODEENABLED, NPPM_ADDSCNMODIFIEDFLAGS
- scintilla.nim (nppnim repo) — SCI_GETCURRENTPOS=2008, SCI_LINEFROMPOSITION=2166, SCI_GETCODEPAGE=2137
- npm registry — markdown-it@14.1.1, highlight.js@11.11.1, markdown-it-task-lists@2.1.1, github-markdown-css@5.9.0

### Secondary (MEDIUM confidence)
- [github-markdown-css README](https://github.com/sindresorhus/github-markdown-css) — theme file names, `.markdown-body` class, manual light/dark switching
- [highlight.js docs](https://highlightjs.readthedocs.io/en/latest/) — hljs.highlightAll(), theme names (github, github-dark)
- [Win32 KillTimer docs — Microsoft Docs](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-killtimer) — "does not remove WM_TIMER messages already posted"
- [WebView2 NavigateToString 2MB workaround — weblog.west-wind.com 2024](https://weblog.west-wind.com/posts/2024/Jul/22/Work-around-the-WebView2-NavigateToString-2mb-Size-Limit)
- NppGateway.h (NppLSP) — NPPM_GETCURRENTSCINTILLA, NPPM_GETFULLPATHFROMBUFFERID call patterns
- markdown-it architecture.md — token.map property, renderer.rules override pattern
- NPP community forum — NPPM_ADDSCNMODIFIEDFLAGS introduced v8.7.7
- NppAnotherMarkdown (ezyuzin/NppAnotherMarkdown) — reference implementation architecture

### Tertiary (LOW confidence)
- SCN_MODIFIED / SC_MOD_ flag values from Scintilla.h cross-references — LOW; verify from official Scintilla.h

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all versions verified via npm registry
- Architecture: HIGH — patterns verified from official WebView2 and NPP docs
- Pitfalls: HIGH — most verified; Pitfall 3 (mapping timing) is MEDIUM
- Header constants: HIGH — verified from official NPP source via WebFetch

**Research date:** 2026-04-09
**Valid until:** 2026-05-09 (stable stack; NPP API constants are stable)
