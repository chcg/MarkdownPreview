---
phase: 03-extended-rendering
reviewed: 2026-04-09T18:28:40Z
depth: standard
files_reviewed: 10
files_reviewed_list:
  - MarkdownPreview/assets/preview.html
  - MarkdownPreview/assets/texmath.js
  - MarkdownPreview/src/PluginDefinition.cpp
  - MarkdownPreview/src/PluginDefinition.h
  - MarkdownPreview/src/PluginMain.cpp
  - MarkdownPreview/src/PreviewPanel.cpp
  - MarkdownPreview/src/PreviewPanel.h
  - MarkdownPreview/src/Settings.cpp
  - MarkdownPreview/src/Settings.h
  - MarkdownPreview/MarkdownPreview.vcxproj
findings:
  critical: 1
  warning: 5
  info: 4
  total: 10
status: issues_found
---

# Phase 03: Code Review Report

**Reviewed:** 2026-04-09T18:28:40Z
**Depth:** standard
**Files Reviewed:** 10
**Status:** issues_found

## Summary

Phase 03 adds KaTeX math rendering (XRND-01), Mermaid diagrams (XRND-02), footnotes (XRND-03), copy-to-clipboard buttons (XRND-04), keyboard zoom controls (THME-04), settings persistence for zoom, and PDF export via WebView2 `PrintToPdf` (EXPT-02).

The overall implementation is solid. `html: false` is correctly maintained, JSON construction always goes through nlohmann (never string concatenation), WebView2 event tokens are unregistered before `Close()`, and the PDF zoom-reset/restore pattern is sound in the success path. The one critical issue is a zoom-level state divergence introduced in the PDF export flow. Five warnings cover a PDF path-injection risk, a mermaid `innerHTML` XSS exposure, a zoom persistence/restoration split-brain, missing guard on `get_Settings`, and the WM_NOTIFY shell-open handler accepting unvalidated URLs. Four informational items round out the report.

---

## Critical Issues

### CR-01: PDF export uses user-controlled file path without sanitization — path-injection risk

**File:** `MarkdownPreview/src/PreviewPanel.cpp:615-621`

**Issue:** `triggerPdfExport()` passes `m_currentFilePath` (set from `NPPM_GETFULLCURRENTPATH` in `onScnModified` / `onBufferActivated`) directly to `PrintToPdf` after replacing the extension. `m_currentFilePath` is set only from Notepad++ API calls, so in normal operation the value is trustworthy. However `PrintToPdf` silently overwrites any writable file at the computed path with no confirmation. If a future code path (or a crafted NPP plugin state) places an adversarial absolute path in `m_currentFilePath` — e.g., `C:\Windows\System32\somefile.pdf` — the export will overwrite it without prompting.

More immediately: there is no check that the derived PDF path stays under the same directory as the source `.md` file. An `.md` file named `../../../../etc/evil.md` (theoretically possible on a network share or via a crafted `NPPM_GETFULLCURRENTPATH` response) would write a PDF to an unexpected location.

**Fix:** Validate that the PDF output path shares the same directory as the source file before calling `PrintToPdf`:

```cpp
// After computing pdfPath (line ~621), before calling PrintToPdf:
std::wstring pdfDir = pdfPath.substr(0, pdfPath.find_last_of(L"\\/"));
std::wstring srcDir = m_currentFilePath.substr(
    0, m_currentFilePath.find_last_of(L"\\/"));
if (_wcsicmp(pdfDir.c_str(), srcDir.c_str()) != 0) return;  // reject escaped path
```

This keeps the same-directory guarantee for both HTML and PDF export.

---

## Warnings

### WR-01: Mermaid `result.svg` assigned to `innerHTML` — unsanitized SVG injection

**File:** `MarkdownPreview/assets/preview.html:394`

**Issue:** `container.innerHTML = result.svg;` assigns Mermaid's rendered SVG output directly as inner HTML. Mermaid generates SVG internally from its own parser, so this is not a direct user-content sink. However:

1. Mermaid does not guarantee that `result.svg` is safe when given adversarial diagram source. A crafted Mermaid diagram could theoretically produce SVG containing `<script>` or `onload=` event handlers if Mermaid's sanitizer has a bypass. Mermaid 11 has its own XSS sanitizer, but it has had CVEs in the past (e.g., GHSA-x3vm-38hw-q2wc in v9.x).
2. Because `html: false` is correctly set on markdown-it, user markdown text does not pass through as raw HTML — but code blocks tagged `mermaid` do reach this code path, so the attack surface exists.

**Fix:** Use `DOMParser` to parse the SVG and insert it as a DOM node, or use `insertAdjacentHTML` after stripping `<script>` elements from the SVG string:

```js
_mermaid.render(id, source).then(function(result) {
    var parser = new DOMParser();
    var svgDoc = parser.parseFromString(result.svg, 'image/svg+xml');
    // Remove any script elements from the parsed SVG
    svgDoc.querySelectorAll('script').forEach(function(s) { s.remove(); });
    var container = document.createElement('div');
    container.className = 'mermaid-diagram';
    container.appendChild(document.importNode(svgDoc.documentElement, true));
    pre.parentNode.replaceChild(container, pre);
});
```

Alternatively, set Mermaid's `securityLevel: 'strict'` in the `initialize` call (lines 247 and 307-311) which enables Mermaid's own DOMPurify-based sanitizer:

```js
_mermaid.initialize({ startOnLoad: false, theme: 'default', securityLevel: 'strict' });
```

### WR-02: Zoom level state diverges between `m_zoomLevel` and `g_settings.zoomLevel` after PDF export

**File:** `MarkdownPreview/src/PreviewPanel.cpp:671-696`

**Issue:** The PDF zoom-reset pattern saves `m_zoomLevel` to `m_savedZoomForPdf`, posts zoom=1.0 to JS, then restores `m_savedZoomForPdf` to JS in the callback. This correctly restores the *visual* zoom in the webview. However `m_zoomLevel` and `g_settings.zoomLevel` are never updated during the reset/restore cycle — they retain the pre-PDF value throughout, which is correct for persistence.

The bug arises if the user triggers a zoom key (`Ctrl++`, `Ctrl+-`, `Ctrl+0`) *between* the `postZoomToJs(1.0f)` call on line 672 and the completion callback on line 685. In that window:

- The accelerator handler (lines 260-276) reads and updates `m_zoomLevel` from the current value (which is still the pre-PDF value, not 1.0f)
- The completion callback then restores `m_savedZoomForPdf` to JS, overwriting the user's new zoom
- `m_zoomLevel` now holds the value the user set, but JS shows `m_savedZoomForPdf` — they are out of sync

**Fix:** Block zoom key handling during PDF export, or check `m_printToPdfInProgress` in the accelerator handler before applying a zoom change:

```cpp
// In AcceleratorKeyPressed lambda, before computing new zoom level (~line 255):
if (m_printToPdfInProgress) {
    args->put_Handled(TRUE);  // still suppress WebView2 built-in zoom
    return S_OK;
}
```

### WR-03: `applyInitialZoom` is called before the navigation-complete event — JS may not be ready

**File:** `MarkdownPreview/src/PreviewPanel.cpp:304-317`

**Issue:** After `m_webview->Navigate(...)` on line 303, the code immediately calls `applyInitialZoom(m_zoomLevel)` at line 317. `Navigate` is asynchronous. The `preview.html` page may not have finished loading and executing its `<script>` block — specifically, the `window.chrome.webview.addEventListener('message', ...)` registration — before the zoom message is posted. If the message arrives before the listener registers, it is silently dropped and the persisted zoom is never applied.

This is a timing bug specific to the first launch when `g_settings.zoomLevel != 1.0f`. The render already handles this via `m_pendingFilePath`, but zoom has no equivalent pending mechanism.

**Fix:** Wire a `NavigationCompleted` event handler and post the initial zoom (and any pending render) from within it:

```cpp
m_webview->add_NavigationCompleted(
    Callback<ICoreWebView2NavigationCompletedEventHandler>(
        [this](ICoreWebView2* /*sender*/,
               ICoreWebView2NavigationCompletedEventArgs* /*args*/) -> HRESULT {
            applyInitialZoom(m_zoomLevel);
            if (!m_pendingFilePath.empty()) {
                std::wstring pending = m_pendingFilePath;
                m_pendingFilePath.clear();
                renderMarkdown(pending);
            }
            return S_OK;
        }).Get(),
    &m_navigationCompletedToken);
```

Remove the direct calls to `applyInitialZoom` and the inline `m_pendingFilePath` dispatch block at lines 309-317.

### WR-04: `get_Settings` return value is unchecked — potential null-pointer dereference

**File:** `MarkdownPreview/src/PreviewPanel.cpp:282-286`

**Issue:**

```cpp
wil::com_ptr<ICoreWebView2Settings> settings;
m_webview->get_Settings(&settings);
settings->put_AreDefaultContextMenusEnabled(FALSE);
```

`get_Settings` returns an `HRESULT`. If it fails (e.g., during a race where the webview is being closed), `settings` remains null and the three `put_*` calls dereference a null COM pointer, causing an access violation crash.

**Fix:**

```cpp
wil::com_ptr<ICoreWebView2Settings> settings;
if (SUCCEEDED(m_webview->get_Settings(&settings)) && settings) {
    settings->put_AreDefaultContextMenusEnabled(FALSE);
    settings->put_AreDevToolsEnabled(FALSE);
    settings->put_IsStatusBarEnabled(FALSE);
}
```

### WR-05: `WM_NOTIFY` SysLink handler passes unvalidated URL to `ShellExecuteW`

**File:** `MarkdownPreview/src/PreviewPanel.cpp:741-745`

**Issue:**

```cpp
PNMLINK link = reinterpret_cast<PNMLINK>(lParam);
ShellExecuteW(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
```

`link->item.szUrl` is the URL text from the `WC_LINK` SysLink control. The SysLink control is populated with a hardcoded string in `showWebView2MissingFallback` (line 343-348), so currently the URL is known and safe. However, the handler does not validate the `nmhdr->hwndFrom` — any `WM_NOTIFY` message sent to the panel window with `NM_CLICK` or `NM_RETURN` code will trigger `ShellExecuteW` with whatever URL is in `lParam`. A plugin or malicious process could send a crafted `WM_NOTIFY` to execute an arbitrary command via `ShellExecuteW`.

**Fix:** Validate that the notify comes from the expected fallback control, and cap the URL to `https://`:

```cpp
case WM_NOTIFY: {
    NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
    if ((nmhdr->code == NM_CLICK || nmhdr->code == NM_RETURN)
        && self && nmhdr->hwndFrom == self->m_hFallback) {
        PNMLINK link = reinterpret_cast<PNMLINK>(lParam);
        // Only open https:// links
        if (wcsncmp(link->item.szUrl, L"https://", 8) == 0) {
            ShellExecuteW(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
        }
    }
    break;
}
```

Note: `self` must be retrieved from `GWLP_USERDATA` as in the `WM_SIZE` handler before this check.

---

## Info

### IN-01: `AdditionalLibraryDirectories` points to a file, not a directory

**File:** `MarkdownPreview/MarkdownPreview.vcxproj:113` (and lines 139, 161, 187)

**Issue:** The `<AdditionalLibraryDirectories>` element contains the full path to `WebView2LoaderStatic.lib` rather than the directory containing it:

```xml
<AdditionalLibraryDirectories>
  $(SolutionDir)packages\Microsoft.Web.WebView2.1.0.3856.49\build\native\$(PlatformTarget)\WebView2LoaderStatic.lib
</AdditionalLibraryDirectories>
```

`AdditionalLibraryDirectories` expects a directory path (like `/LIBPATH:` in cl.exe), not a file path. The `.lib` file must appear in `<AdditionalDependencies>` instead. This currently appears to build (the `.targets` import in the WebView2 NuGet package likely handles it), but is incorrect and may cause issues on clean machines or CI environments that skip the NuGet `.targets` auto-import.

**Fix:** Move the lib file to `AdditionalDependencies` and set the directory correctly:

```xml
<AdditionalLibraryDirectories>
  $(SolutionDir)packages\Microsoft.Web.WebView2.1.0.3856.49\build\native\$(PlatformTarget)
</AdditionalLibraryDirectories>
<AdditionalDependencies>WebView2LoaderStatic.lib;%(AdditionalDependencies)</AdditionalDependencies>
```

### IN-02: Export failure is silently swallowed with no user feedback

**File:** `MarkdownPreview/assets/preview.html:587-591`

**Issue:** When `exportHtml()` throws an exception (e.g., `fetch` fails, `FileReader` errors, or `postMessage` fails), the catch block logs to `console.error` but never notifies the user or C++. The user clicks "Export as HTML" and nothing happens — no file is written and no error is shown. The same applies to PDF export failure (line 688-689 in `PreviewPanel.cpp` where `errorCode` and `isSuccessful` are explicitly ignored).

**Fix (info-level suggestion):** Post an `exportFailed` message to C++ on catch and have the C++ side show a `MessageBoxW` with the error, or at minimum add a visible error indicator in the preview UI. This is flagged as Info because the spec says "silent failure in Phase 2" was an accepted compromise, but it will be a usability pain point and should be revisited before publish.

### IN-03: `mermaid-diagram-` IDs use `Date.now()` — not unique under rapid re-render

**File:** `MarkdownPreview/assets/preview.html:385`

**Issue:**

```js
var id = 'mermaid-diagram-' + index + '-' + Date.now();
```

`Date.now()` has millisecond resolution. If `renderMermaidDiagrams()` is called twice in the same millisecond (possible during rapid debounced renders), two diagrams will receive identical IDs. Mermaid uses this ID as an SVG `id` attribute, and duplicate IDs can cause the second `mermaid.render()` call to overwrite the first SVG in the DOM or fail silently.

**Fix:** Use a monotonically incrementing counter instead of `Date.now()`:

```js
// At module scope (after _mermaid initialization):
var _mermaidCounter = 0;

// In renderMermaidDiagrams():
var id = 'mermaid-diagram-' + (_mermaidCounter++);
```

### IN-04: `CoUninitialize` is never called

**File:** `MarkdownPreview/src/PluginDefinition.cpp:33` / `pluginCleanUp`

**Issue:** `CoInitializeEx` is called in `pluginInit` (line 33) but `CoUninitialize` is never called in `pluginCleanUp` or `onNppShutdown`. In practice Notepad++ terminates the process after plugin cleanup, so the leak has no observable effect. However, if the plugin is ever dynamically unloaded and reloaded (not currently supported by NPP, but a possibility in test harnesses), the COM apartment would be leaked.

**Fix:** Call `::CoUninitialize()` at the end of `pluginCleanUp()`. The comment in `pluginCleanUp` explains that WebView2 destruction is moved to `onNppShutdown`, but COM uninit can safely remain in `pluginCleanUp` after WebView2 has already been closed.

---

_Reviewed: 2026-04-09T18:28:40Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
