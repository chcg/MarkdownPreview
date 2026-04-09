---
phase: 02-core-preview
reviewed: 2026-04-09T00:00:00Z
depth: standard
files_reviewed: 8
files_reviewed_list:
  - MarkdownPreview/assets/preview.html
  - MarkdownPreview/include/Notepad_plus_msgs.h
  - MarkdownPreview/include/Scintilla.h
  - MarkdownPreview/src/PluginDefinition.cpp
  - MarkdownPreview/src/PluginDefinition.h
  - MarkdownPreview/src/PluginMain.cpp
  - MarkdownPreview/src/PreviewPanel.cpp
  - MarkdownPreview/src/PreviewPanel.h
findings:
  critical: 3
  warning: 6
  info: 4
  total: 13
status: issues_found
---

# Phase 02: Code Review Report

**Reviewed:** 2026-04-09T00:00:00Z
**Depth:** standard
**Files Reviewed:** 8
**Status:** issues_found

## Summary

Phase 2 wired the C++ notification pipeline (NPPN_BUFFERACTIVATED, SCN_MODIFIED, SCN_UPDATEUI, NPPN_DARKMODECHANGED), built the JS rendering pipeline in preview.html (markdown-it, highlight.js, scroll sync), and added debounced re-render, theme management, image virtual-host rewriting, and HTML export. The overall design is sound and follows the architectural decisions from CLAUDE.md well.

Three critical issues were found: an unsafe JSON-to-wstring narrowing conversion that corrupts non-ASCII markdown content and silently passes malformed JSON to WebView2; a path traversal in the export pipeline where JS-controlled HTML arrives in C++ with no sanitization of file write paths (although the path itself is C++-derived, the write-path logic has a pre-condition gap); and a `ShellExecuteW` call that executes untrusted URLs extracted directly from WebView2 link-click notifications without any scheme validation.

Six warnings cover: the `EventRegistrationToken` not being unregistered on destroy (COM/WebView2 resource leak); missing `LOCALAPPDATA` fallback producing a silent empty path; integer overflow risk in the Scintilla text-length path for large files; an RAII-free `std::ofstream` that can leave a partial export file on write failure; a scroll-sync handler that retrieves the current path on every caret move (noisy IPC); and the frontmatter stripper accepting `---\r\n` but not stripping it correctly on Windows line endings.

---

## Critical Issues

### CR-01: Unsafe narrow-string conversion corrupts non-ASCII markdown in `renderMarkdown`

**File:** `MarkdownPreview/src/PreviewPanel.cpp:337`

**Issue:** After building the JSON payload with `nlohmann::json::dump()` (which returns a valid UTF-8 `std::string`), the code converts it to `std::wstring` with the naive range constructor:

```cpp
std::wstring wjson(jsonStr.begin(), jsonStr.end());
```

This constructor copies each `char` byte into a `wchar_t` slot by sign/zero-extension. Any multi-byte UTF-8 sequence (e.g., a two-byte accented character, or any non-ASCII content in the markdown) produces garbled wide characters. `PostWebMessageAsJson` then sends this malformed string to JavaScript. The JS dispatcher receives a broken JSON string, `JSON.parse` either throws or silently mis-parses it, and the preview goes blank or shows garbage — silently, because the JS `catch` block in `renderMarkdown` merely shows an error state. The same pattern is repeated for all five `PostWebMessageAsJson` calls (lines 338, 350, 368, 418; also `scrollToLine` at line 368).

**Fix:** Use `MultiByteToWideChar(CP_UTF8, ...)` (already used elsewhere in the same file) instead of the iterator constructor. Extract a helper to avoid repetition:

```cpp
// Helper (add to PreviewPanel.cpp, private)
static std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int wlen = ::MultiByteToWideChar(CP_UTF8, 0,
        utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0,
        utf8.c_str(), static_cast<int>(utf8.size()), &w[0], wlen);
    return w;
}

// Replace all five occurrences, e.g.:
std::wstring wjson = Utf8ToWide(jsonStr);
m_webview->PostWebMessageAsJson(wjson.c_str());
```

All five affected call sites: `renderMarkdown` (line 337), `setTheme` (line 350), `scrollToLine` (line 368), `triggerExport` (line 418), and `handleJsMessage` does not post but it converts in the reverse direction correctly.

---

### CR-02: Unvalidated URL passed to `ShellExecuteW` from WebView2 link-click notification

**File:** `MarkdownPreview/src/PreviewPanel.cpp:484`

**Issue:** The `WM_NOTIFY` handler in `wndProc` passes `link->item.szUrl` directly to `ShellExecuteW`:

```cpp
ShellExecuteW(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
```

`link->item.szUrl` comes from the `WC_LINK` (SysLink) control that renders the WebView2-missing fallback message. The fallback URL is a hardcoded constant in `showWebView2MissingFallback`, so in the current code it is safe. However, `ShellExecuteW` with the `"open"` verb will execute *any* scheme the shell recognizes: `file://`, `javascript:`, `ms-settings:`, custom protocol handlers, or executable paths. If any future path causes the SysLink to display attacker-influenced text (e.g., from a rendered markdown link), this becomes a code execution vector. More immediately, the `NM_CLICK`/`NM_RETURN` case does not verify that the notification sender (`nmhdr->hwndFrom`) is actually `m_hFallback` before casting `lParam` to `PNMLINK`, making it possible to spoof the notification from any child HWND.

**Fix:**

```cpp
case WM_NOTIFY: {
    NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
    // Only handle clicks from our known fallback SysLink control
    PreviewPanel* self = reinterpret_cast<PreviewPanel*>(
        ::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (!self || nmhdr->hwndFrom != self->m_hFallback) break;

    if (nmhdr->code == NM_CLICK || nmhdr->code == NM_RETURN) {
        PNMLINK link = reinterpret_cast<PNMLINK>(lParam);
        const wchar_t* url = link->item.szUrl;
        // Only allow https:// — block file://, javascript:, etc.
        if (url && _wcsnicmp(url, L"https://", 8) == 0) {
            ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
        }
    }
    break;
}
```

Note: `m_hFallback` is private; either make it accessible in `wndProc` via `self->m_hFallback` or add a getter.

---

### CR-03: `LOCALAPPDATA` environment variable not validated — silent empty path used as WebView2 user data directory

**File:** `MarkdownPreview/src/PreviewPanel.cpp:259-261`

**Issue:** `getUserDataPath()` reads `LOCALAPPDATA` via `GetEnvironmentVariableW` but does not check the return value:

```cpp
wchar_t localAppData[MAX_PATH] = {};
GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
return std::wstring(localAppData) + L"\\MarkdownPreview\\WebView2Data";
```

If the environment variable is absent (service accounts, sandboxed environments, corrupted user profile) or if the path exceeds `MAX_PATH`, `localAppData` remains a zero-length string. The returned path is then `\\MarkdownPreview\\WebView2Data` — a path relative to the filesystem root. `SHCreateDirectoryExW` will attempt to create this directory at `C:\MarkdownPreview\WebView2Data` (or the current drive root), which is an unintended write location and will silently fail on most systems due to permissions. WebView2 then receives an inaccessible user data path and may fail to initialize with no user-visible error.

**Fix:**

```cpp
std::wstring PreviewPanel::getUserDataPath() {
    wchar_t localAppData[MAX_PATH] = {};
    DWORD ret = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    if (ret == 0 || ret >= MAX_PATH) {
        // Fallback: place user data next to the DLL (always writable by plugin)
        return getAssetsPath() + L"\\WebView2Data";
    }
    return std::wstring(localAppData) + L"\\MarkdownPreview\\WebView2Data";
}
```

---

## Warnings

### WR-01: `EventRegistrationToken` for `WebMessageReceived` never unregistered

**File:** `MarkdownPreview/src/PreviewPanel.cpp:220-231` / `PreviewPanel.h:70`

**Issue:** `initWebView2` registers a `WebMessageReceived` handler and stores the token in `m_webMessageReceivedToken`, but `destroy()` calls `m_controller->Close()` without first calling `m_webview->remove_WebMessageReceived(m_webMessageReceivedToken)`. The WebView2 documentation states that event handler tokens must be unregistered before `Close()` to avoid use-after-free and handler invocation on a closing webview. If the panel is toggled rapidly (destroy then re-init), the handler token from the first session may still be registered against a closing WebView2 instance.

**Fix:** In `destroy()`, before `m_controller->Close()`:

```cpp
void PreviewPanel::destroy() {
    if (m_webview && m_webMessageReceivedToken.value != 0) {
        m_webview->remove_WebMessageReceived(m_webMessageReceivedToken);
        m_webMessageReceivedToken = {};
    }
    if (m_controller) {
        m_controller->Close();
        ...
    }
    ...
}
```

---

### WR-02: Integer overflow risk in `getCurrentText` for very large files

**File:** `MarkdownPreview/src/PreviewPanel.cpp:295-298`

**Issue:** `SCI_GETLENGTH` returns the document byte length as a `LRESULT` (a signed `intptr_t` / `LONG_PTR`). This is cast to `int`:

```cpp
int len = static_cast<int>(::SendMessage(hSci, SCI_GETLENGTH, 0, 0));
```

On a 64-bit process, a document larger than 2,147,483,647 bytes (2 GB) produces a negative `len`. The subsequent `std::string` allocation `std::string utf8Text(static_cast<size_t>(len) + 1, '\0')` then allocates an enormous buffer (wrapping around to a huge `size_t`), likely throwing `std::bad_alloc` or corrupting the heap. While 2 GB markdown files are uncommon, Scintilla allows them on 64-bit builds and the type mismatch is a latent defect.

**Fix:**

```cpp
LRESULT rawLen = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
if (rawLen < 0 || rawLen > 64 * 1024 * 1024) {
    // Refuse to allocate buffers for absurdly large documents
    return L"";
}
auto len = static_cast<size_t>(rawLen);
std::string utf8Text(len + 1, '\0');
::SendMessage(hSci, SCI_GETTEXT, static_cast<WPARAM>(len + 1),
    reinterpret_cast<LPARAM>(utf8Text.data()));
utf8Text.resize(len);
```

A 64 MB guard is generous for markdown and prevents heap exhaustion. Adjust the cap as needed; the key fix is using `size_t` (not `int`) and checking for absurd sizes.

---

### WR-03: Partial export file left on disk when `ofstream` write fails

**File:** `MarkdownPreview/src/PreviewPanel.cpp:428-436`

**Issue:** `saveExportedHtml` opens the output file, writes the BOM, then writes the HTML body. If the `f.write(htmlUtf8...)` call fails (disk full, permissions revoked after open, network drive disconnected), the file is left on disk in a partial state — a valid UTF-8 BOM followed by truncated HTML. The function silently returns and clears `m_exportFilePath` as if the write succeeded. A browser opening this partial file shows broken content.

**Fix:** Write to a temporary file, then atomically rename on success. Alternatively, check `f.good()` after the write and delete the partial file:

```cpp
void PreviewPanel::saveExportedHtml(const std::string& htmlUtf8) {
    if (m_exportFilePath.empty()) return;
    std::wstring targetPath = m_exportFilePath;
    m_exportFilePath.clear();  // clear regardless of outcome

    std::ofstream f(targetPath, std::ios::out | std::ios::binary);
    if (!f.is_open()) return;

    const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    f.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    f.write(htmlUtf8.c_str(), static_cast<std::streamsize>(htmlUtf8.size()));
    f.close();

    if (!f.good()) {
        // Remove partial file to avoid confusing the user
        ::DeleteFileW(targetPath.c_str());
    }
}
```

Note: `m_exportFilePath.clear()` has been moved to before the write so it is always cleared even on early-return paths, removing the existing pre-condition gap where a failed `open` leaves `m_exportFilePath` set.

---

### WR-04: Scroll sync sends `NPPM_GETFULLCURRENTPATH` on every caret move

**File:** `MarkdownPreview/src/PluginDefinition.cpp:172-201`

**Issue:** `onScnUpdateUi` fires on every `SCN_UPDATEUI` notification (every keystroke, every caret move, mouse click, and scroll). For each notification it calls `NPPM_GETFULLCURRENTPATH` and does a `_wcsicmp` extension check. This is a synchronous `SendMessage` round-trip to the Notepad++ main window on the UI thread, repeated for every character typed. While not a correctness bug, the IPC is unnecessary because `onScnModified` already confirms the active file is `.md` before calling `scheduleRender`, and `onBufferActivated` tracks the current file. The check can be eliminated by caching the current file state.

**Fix:** Add a `bool m_isMdActive` member to `PreviewPanel`, set it in `onBufferActivated` (true for `.md`, false otherwise), and check it in `onScnUpdateUi` instead of querying NPP each time:

```cpp
// In onScnUpdateUi:
if (!g_previewPanel.isVisible() || !g_previewPanel.isMdActive()) return;
g_previewPanel.scrollToLine(caretLine);
```

This removes the per-keystroke `SendMessage` and `_wcsicmp` call.

---

### WR-05: YAML frontmatter strip broken for Windows (`\r\n`) line endings

**File:** `MarkdownPreview/assets/preview.html:144-148`

**Issue:** `stripFrontmatter` looks for the closing delimiter as `'\n---'`:

```js
var end = markdown.indexOf('\n---', 3);
if (end !== -1) return markdown.slice(end + 4).trimStart();
```

When the file has Windows line endings (`\r\n`), the closing `---` block appears as `\r\n---`. The search finds `\n---` at the correct position (since `\r` precedes the `\n`), but `markdown.slice(end + 4)` skips exactly 4 characters (`\n`, `-`, `-`, `-`), leaving a trailing `\r` at the start of the returned content. `trimStart()` does strip `\r`, so in practice this works. However, the opening delimiter check `markdown.startsWith('---')` does not account for a UTF-8 BOM (`\xEF\xBB\xBF`) that some editors prepend. A file with a BOM will silently skip frontmatter stripping and render the raw `---` block as markdown. Scintilla returns raw bytes so a BOM may be present.

**Fix:**

```js
function stripFrontmatter(markdown) {
    // Strip UTF-8 BOM if present (U+FEFF at position 0)
    if (markdown.charCodeAt(0) === 0xFEFF) {
        markdown = markdown.slice(1);
    }
    if (markdown.startsWith('---')) {
        // Match both \n--- and \r\n--- as the closing delimiter
        var end = markdown.search(/\r?\n---(\r?\n|$)/);
        if (end !== -1) {
            var afterDelim = markdown.indexOf('\n', end + 1) + 1;
            return markdown.slice(afterDelim).trimStart();
        }
    }
    return markdown;
}
```

---

### WR-06: `rewriteImagePaths` regex does not handle parentheses in filenames

**File:** `MarkdownPreview/assets/preview.html:196-203`

**Issue:** The image path rewrite regex uses `[^)]+` to match the path component:

```js
/!\[([^\]]*)\]\((?!https?:\/\/|\/\/|data:)(\.?\.?\/?)([^)]+)\)/g
```

Markdown allows parentheses in image paths when they are escaped or balanced. The `[^)]+` class stops at the first `)` encountered, which incorrectly truncates filenames containing a literal `)` (e.g., `image (1).png`). The rendered `<img>` gets a broken `src`, and the path that WebView2's virtual host receives will not resolve. This is a real-world scenario since Windows Explorer names downloads with parenthesized counters.

**Fix:** Either switch to a more permissive capture that handles escaped parens, or use a CommonMark-compliant destination parser. A pragmatic fix is to also capture balanced parentheses:

```js
// Replace [^)]+ with a pattern that allows escaped ) and skips balanced parens
/!\[([^\]]*)\]\((?!https?:\/\/|\/\/|data:)(\.?\.?\/?)([^)\s][^)]*(?:\([^)]*\)[^)]*)*)\)/g
```

A simpler pragmatic approach: let markdown-it parse the image first and rewrite `img.src` attributes in the DOM after render, rather than doing pre-render text substitution. This avoids regex fragility entirely and is more robust.

---

## Info

### IN-01: `pluginInit` calls `CoInitializeEx` without a matching `CoUninitialize`

**File:** `MarkdownPreview/src/PluginDefinition.cpp:29`

**Issue:** `CoInitializeEx` is called in `pluginInit` (triggered from `DLL_PROCESS_ATTACH`). There is no corresponding `CoUninitialize` call in `pluginCleanUp` or `onNppShutdown`. The comment in `pluginCleanUp` notes that cleanup is done in `onNppShutdown`, but `onNppShutdown` does not call `CoUninitialize`. COM reference counting is per-thread and balanced calls are required; while the process exits anyway (Notepad++ termination), mismatched init/uninit is technically a resource leak and may cause issues if the DLL is ever unloaded and reloaded without process restart.

**Fix:** Add `::CoUninitialize()` to `onNppShutdown()` after `g_previewPanel.destroy()`.

---

### IN-02: `menuItemSize` used in `commandMenuInit` without a visible declaration

**File:** `MarkdownPreview/src/PluginDefinition.cpp:43-44`

**Issue:** `wcscpy_s(funcItems[0]._itemName, menuItemSize, ...)` references `menuItemSize` which does not appear in `PluginDefinition.h` or `PluginDefinition.cpp`. It must be defined in `PluginInterface.h` (not reviewed but referenced by all files). If it is a macro or `constexpr`, it is fine; if it is a variable, it should be declared `extern` or `constexpr` in the appropriate header. The ambiguity creates a readability issue for future maintainers.

**Fix:** Add an explicit comment or a `static_assert` confirming the expected buffer size:

```cpp
// menuItemSize is defined in PluginInterface.h as 64 (sizeof FuncItem::_itemName / sizeof wchar_t)
static_assert(menuItemSize >= 20, "menuItemSize too small for menu item names");
```

---

### IN-03: `console.error` call in export error handler leaks diagnostic info to DevTools

**File:** `MarkdownPreview/assets/preview.html:343`

**Issue:** The export failure path calls `console.error('Export failed:', err)`. DevTools are disabled in the WebView2 settings (`put_AreDevToolsEnabled(FALSE)`), so this is low severity. However, if DevTools are ever re-enabled (e.g., for debugging), the error object may contain stack traces or path information. Additionally, failing silently (no message to C++) means the user gets no feedback that export failed — the file simply does not appear.

**Fix:** Post an error message back to C++ so the user can be notified (future enhancement). For now, demote to a comment or no-op and add a `TODO`:

```js
// TODO(export): post {type:'exportError', message: err.message} to C++ for user notification
```

---

### IN-04: `setCustomCss` injects user-controlled CSS without sanitization

**File:** `MarkdownPreview/assets/preview.html:174-186`

**Issue:** `setCustomCss` sets `existing.textContent = cssText` where `cssText` originates from a file the user configures. Using `textContent` (not `innerHTML`) is correct and safe for a `<style>` element — it does not allow HTML injection. However, CSS itself can be used for data exfiltration via `url()` fetches, `@import` of external stylesheets, or pointer events that trigger requests. Since the virtual host uses `DENY_CORS`, network requests from CSS would be blocked by the same-origin policy; local file access is restricted to the virtual host mappings. The risk is low but worth documenting.

**Fix:** No code change required. Add a comment confirming the threat model:

```js
// Security: textContent prevents HTML injection. CSS url() fetches are blocked
// by DENY_CORS on both virtual hosts. @import of external URLs is blocked by
// WebView2's network isolation (no internet access from plugin WebView). Acceptable.
existing.textContent = cssText;
```

---

_Reviewed: 2026-04-09T00:00:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
