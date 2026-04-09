---
phase: 02-core-preview
reviewed: 2026-04-09T16:11:19Z
depth: standard
files_reviewed: 2
files_reviewed_list:
  - MarkdownPreview/assets/preview.html
  - MarkdownPreview/src/PreviewPanel.cpp
findings:
  critical: 2
  warning: 4
  info: 3
  total: 9
status: issues_found
---

# Phase 02: Code Review Report

**Reviewed:** 2026-04-09T16:11:19Z
**Depth:** standard
**Files Reviewed:** 2
**Status:** issues_found

## Summary

Both files implement the core preview pipeline: `PreviewPanel.cpp` manages the C++ side (WebView2 initialization, Scintilla text retrieval, JSON message dispatch, debounced rendering, HTML export), and `preview.html` implements the JS renderer (markdown-it, highlight.js, scroll sync, export serialization).

The overall structure is sound and security-conscious in several places — JSON is constructed via nlohmann rather than concatenation, `html: false` is set on markdown-it to block raw HTML pass-through, the export file path is derived server-side in C++ (not from JS-controlled content), and `PostWebMessageAsJson` is used correctly. The `Utf8ToWide` helper is correctly implemented with `MultiByteToWideChar`.

Two critical issues were found: an unvalidated URL passed to `ShellExecuteW` in the `WM_NOTIFY` handler (no hwnd source check, no scheme whitelist), and a ReDoS vulnerability in the `rewriteImagePaths` regex. Four warnings cover a scroll-sync algorithm correctness problem, a missing export-failure feedback path, document-size truncation risk in `getCurrentText`, and an unguarded `exportReady` handler that can write to a stale path. Three info items cover `GetModuleFileNameW` truncation, `stripFrontmatter` fragility, and an unused parameter.

---

## Critical Issues

### CR-01: `ShellExecuteW` Called With Unvalidated URL From SysLink Control

**File:** `MarkdownPreview/src/PreviewPanel.cpp:536`

**Issue:** The `WM_NOTIFY` handler extracts `link->item.szUrl` from the SysLink notification and passes it directly to `ShellExecuteW` with the `"open"` verb. Two problems compound:

1. `nmhdr->hwndFrom` is never checked against `m_hFallback`. Any child HWND in the panel that can generate `NM_CLICK` or `NM_RETURN` will reach the `ShellExecuteW` call, and `lParam` is reinterpreted as `PNMLINK` without verifying the notification actually came from a `WC_LINK` control.
2. There is no URL scheme whitelist. `ShellExecuteW` with `"open"` honors `file://`, `cmd:`, `ms-settings:`, custom URI handlers, and executable paths. If any future code path allows user-influenced content into the SysLink text, this becomes a code execution vector.

**Fix:**
```cpp
case WM_NOTIFY: {
    NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
    PreviewPanel* self = reinterpret_cast<PreviewPanel*>(
        ::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    // Guard: only process clicks from our known SysLink fallback control
    if (!self || !self->m_hFallback || nmhdr->hwndFrom != self->m_hFallback) break;
    if (nmhdr->code == NM_CLICK || nmhdr->code == NM_RETURN) {
        PNMLINK link = reinterpret_cast<PNMLINK>(lParam);
        const wchar_t* url = link->item.szUrl;
        // Scheme whitelist: only allow https:// to prevent local program execution
        if (url && ::wcsncmp(url, L"https://", 8) == 0) {
            ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
        }
    }
    break;
}
```

Note: `m_hFallback` is private; `wndProc` is a `static` member and already has access to the class's private members through `self`.

---

### CR-02: ReDoS Risk in `rewriteImagePaths` Regex

**File:** `MarkdownPreview/assets/preview.html:206-213`

**Issue:** The image-path rewrite regex contains a nested quantifier pattern in the path-capture group:

```
[^)\s][^)]*(?:\([^)]*\)[^)]*)*
```

The `(?:\([^)]*\)[^)]*)` group contains `[^)]*` as a repeated subpattern, and the outer `*` repeats the entire group. This creates catastrophic backtracking when given a malformed image reference with no closing `)` — for example `![x](aaaaaaaaaaaaaaaaaaaaa`. The JavaScript regex engine must explore an exponential number of backtrack paths before failing. On a large markdown document with many such sequences (possible from a pasted file or deliberate user input), this hangs the WebView2 JS thread indefinitely, making Notepad++ unresponsive. This is a denial-of-service risk against the user's own session.

**Fix:** Replace the nested quantifier with a non-backtracking alternative that handles one level of balanced parens:

```js
function rewriteImagePaths(markdownText) {
    return markdownText.replace(
        /!\[([^\]]*)\]\((?!https?:\/\/|\/\/|data:)((?:[^()\\]|\\.|\([^()]*\))*)\)/g,
        function(match, alt, path) {
            var normalized = path.replace(/^\.\//, '');
            return '![' + alt + '](https://file.mdpreview/' + normalized + ')';
        }
    );
}
```

The pattern `(?:[^()\\]|\\.|\([^()]*\))*` handles: ordinary characters (no parens, no backslash), escaped characters (backslash + any char), and balanced single-level parentheses — covering all real-world filenames including `image (1).png`. It cannot catastrophically backtrack because each alternative consumes exactly one character or one balanced paren group.

---

## Warnings

### WR-01: `getCurrentText` Truncates Documents Larger Than 2 GB — Potential Heap Corruption

**File:** `MarkdownPreview/src/PreviewPanel.cpp:320`

**Issue:** `SCI_GETLENGTH` returns a `LRESULT` (signed 64-bit on x64 Windows). The result is immediately cast to `int`:

```cpp
int len = static_cast<int>(::SendMessage(hSci, SCI_GETLENGTH, 0, 0));
```

For a document exceeding `INT_MAX` bytes (~2 GB), `len` becomes negative. The subsequent allocation:

```cpp
std::string utf8Text(static_cast<size_t>(len) + 1, '\0');
```

adds 1 to a large negative value after converting to `size_t`, producing a value near `SIZE_MAX`. This allocation either throws `std::bad_alloc` or, on systems with overcommit, returns a buffer that is not actually backed by physical memory and causes a crash when written. While 2 GB markdown files are not common, the type mismatch is a latent defect and the silent truncation path (a very large `WPARAM` passed to `SCI_GETTEXT`) also exists.

**Fix:**
```cpp
LRESULT rawLen = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
if (rawLen <= 0) return L"";
// Cap at 64 MB — generous for markdown, prevents OOM on absurdly large files
constexpr LRESULT MAX_PREVIEW_BYTES = 64LL * 1024 * 1024;
if (rawLen > MAX_PREVIEW_BYTES) rawLen = MAX_PREVIEW_BYTES;
size_t len = static_cast<size_t>(rawLen);
std::string utf8Text(len + 1, '\0');
::SendMessage(hSci, SCI_GETTEXT, static_cast<WPARAM>(len + 1),
    reinterpret_cast<LPARAM>(utf8Text.data()));
utf8Text.resize(len);
```

---

### WR-02: `handleJsMessage` Processes `exportReady` Without Verifying an Export Was Pending

**File:** `MarkdownPreview/src/PreviewPanel.cpp:503-508`

**Issue:** `handleJsMessage` writes a file to `m_exportFilePath` whenever it receives a message with `"type": "exportReady"`, regardless of whether `triggerExport` was ever called:

```cpp
if (type == "exportReady") {
    std::string htmlContent = j.value("html", "");
    if (!htmlContent.empty()) {
        saveExportedHtml(htmlContent);  // writes to m_exportFilePath unconditionally
    }
}
```

`m_exportFilePath` is only cleared inside `saveExportedHtml` after a successful write (line 488). If an export fails before the write (e.g., `f.is_open()` returns false), `m_exportFilePath` remains set. A subsequent spurious or replayed `exportReady` message (from a JS error, a race condition, or future code paths that post such a message) will overwrite the previous export target silently.

**Fix:** Add a boolean guard flag:
```cpp
// In PreviewPanel.h, add:
bool m_exportPending = false;

// In triggerExport(), before posting to JS:
m_exportPending = true;

// In handleJsMessage():
if (type == "exportReady" && m_exportPending) {
    m_exportPending = false;
    std::string htmlContent = j.value("html", "");
    if (!htmlContent.empty()) {
        saveExportedHtml(htmlContent);
    }
}
```

Also move `m_exportFilePath.clear()` in `saveExportedHtml` to before the `f.is_open()` check so it is always reset regardless of whether the file opens successfully.

---

### WR-03: Scroll Sync Algorithm Breaks on Non-Monotonic `data-line` Order

**File:** `MarkdownPreview/assets/preview.html:258-270`

**Issue:** `scrollToLine` uses an early `break` once it finds an element with `data-line > line`:

```js
} else {
    break;
}
```

This assumes `querySelectorAll('[data-line]')` returns elements in strictly ascending `data-line` order (DOM order == source order). markdown-it's block tokenizer emits tokens in document order, but for nested structures (blockquotes, list items, definition lists) the outer container token has a `data-line` equal to or less than its first child's `data-line`. If any outer container token appears after a child token in the DOM with a lower `data-line` than the child, the scan breaks early and returns a stale `best` that is higher in the document than the actual nearest element. Scroll sync would then jump the user to the wrong location.

**Fix:** Remove the early break and track the closest match explicitly:

```js
function scrollToLine(line) {
    var elements = document.querySelectorAll('[data-line]');
    if (!elements.length) return;
    var best = null;
    var bestLine = -1;
    for (var i = 0; i < elements.length; i++) {
        var elLine = parseInt(elements[i].getAttribute('data-line'), 10);
        if (elLine <= line && elLine >= bestLine) {
            best = elements[i];
            bestLine = elLine;
        }
    }
    if (best) {
        best.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
}
```

---

### WR-04: Export Failures Are Silent — No User Feedback Path

**File:** `MarkdownPreview/src/PreviewPanel.cpp:481` and `MarkdownPreview/assets/preview.html:350-354`

**Issue:** Two failure points are silent:

1. C++ `saveExportedHtml`: `if (!f.is_open()) return;` — if the output file cannot be opened (permissions, locked by another process, disk full), the function returns with no notification to the user and no indication to the caller.
2. JS `exportHtml` catch block: `console.error('Export failed:', err)` — DevTools are disabled in production (`put_AreDevToolsEnabled(FALSE)`), so this error is never visible to the user. There is no message posted back to C++.

The user experience is: clicks "Export", nothing happens, no file appears.

**Fix (C++ side):** Show a `MessageBoxW` on file open failure:
```cpp
if (!f.is_open()) {
    ::MessageBoxW(m_nppHandle,
        (L"Export failed: could not write to\n" + m_exportFilePath).c_str(),
        L"Markdown Preview", MB_OK | MB_ICONWARNING);
    m_exportFilePath.clear();
    return;
}
```

**Fix (JS side):** Post an `exportError` message instead of using `console.error`:
```js
} catch (err) {
    window.chrome.webview.postMessage(
        JSON.stringify({ type: 'exportError', message: err ? err.message : 'unknown error' })
    );
}
```
Then handle `exportError` in `handleJsMessage` to show a `MessageBoxW`.

---

## Info

### IN-01: `getAssetsPath` Does Not Check `GetModuleFileNameW` for Truncation

**File:** `MarkdownPreview/src/PreviewPanel.cpp:290-296`

**Issue:** `GetModuleFileNameW` is called with a fixed `MAX_PATH` buffer and the return value is not checked:

```cpp
wchar_t dllPath[MAX_PATH] = {};
GetModuleFileNameW(m_hInst, dllPath, MAX_PATH);
```

When the DLL is installed in a path longer than `MAX_PATH` (possible on Windows with long-path support enabled, UNC paths, or junction points), the function truncates the path silently and sets `GetLastError()` to `ERROR_INSUFFICIENT_BUFFER`. The returned string is then an invalid path, and `SetVirtualHostNameToFolderMapping` will fail to find the assets directory, breaking all asset loading with no diagnostic.

**Fix:**
```cpp
std::wstring PreviewPanel::getAssetsPath() {
    wchar_t dllPath[MAX_PATH] = {};
    DWORD ret = GetModuleFileNameW(m_hInst, dllPath, MAX_PATH);
    if (ret == 0 || ret >= MAX_PATH) {
        // Truncated or failed — return empty; callers should handle gracefully
        return L"";
    }
    std::wstring path(dllPath);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) path = path.substr(0, pos);
    return path + L"\\assets";
}
```

---

### IN-02: `stripFrontmatter` Slice Logic Is Fragile for CRLF Files

**File:** `MarkdownPreview/assets/preview.html:149-154`

**Issue:** The closing-delimiter slice logic on lines 152-154:

```js
var afterDelim = markdown.indexOf('\n', end + 1) + 1;
return markdown.slice(afterDelim).trimStart();
```

`end` is the index of the `\r` (for CRLF files) or `\n` (for LF files) that begins `\n---`. `markdown.indexOf('\n', end + 1)` searches from `end + 1` — for a CRLF file this is the position of the `\n` in `\n---`, not the `\n` after `---`. So `afterDelim` lands at the character after `---`, which is `\r` for `\r\n---\r\n` content. `trimStart()` strips the leading `\r`, so the function produces the correct output. However the logic is accidentally correct: the comment on line 152 says "advance past the matched `\n---`" but the code actually stops after `---` and relies on `trimStart()` to clean up. This is a maintainability hazard — a future change that removes `trimStart()` for a different reason would silently break CRLF frontmatter handling.

**Fix:** Use the regex match object's index and length for a clean, explicit slice:
```js
function stripFrontmatter(markdown) {
    if (markdown.charCodeAt(0) === 0xFEFF) markdown = markdown.slice(1);
    if (!markdown.startsWith('---')) return markdown;
    var closeMatch = markdown.match(/\r?\n---(\r?\n|$)/);
    if (!closeMatch) return markdown;
    // closeMatch.index + closeMatch[0].length lands exactly after the closing ---
    return markdown.slice(closeMatch.index + closeMatch[0].length).trimStart();
}
```

---

### IN-03: `renderMarkdown` (JS) Has an Unused `filePath` Parameter

**File:** `MarkdownPreview/assets/preview.html:219`

**Issue:** `renderMarkdown(markdown, filePath, customCss)` accepts `filePath` but never references it in the function body. `rewriteImagePaths` receives only the markdown text, not the file path. A reader may assume `filePath` is used for image resolution or document title display and be confused when tracing the code.

**Fix:** Either remove the parameter until it is needed, or annotate it:
```js
// filePath: reserved for future use (document title, relative URL resolution); unused in Phase 2
function renderMarkdown(markdown, filePath, customCss) {
```

---

_Reviewed: 2026-04-09T16:11:19Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
