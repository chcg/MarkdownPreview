---
phase: 04-polish-publication
reviewed: 2026-04-10T00:00:00Z
depth: standard
files_reviewed: 7
files_reviewed_list:
  - MarkdownPreview/assets/preview.html
  - MarkdownPreview/src/PreviewPanel.cpp
  - MarkdownPreview/src/PreviewPanel.h
  - MarkdownPreview/MarkdownPreview.rc
  - MarkdownPreview/MarkdownPreview.vcxproj
  - manifest.json
  - scripts/package.ps1
findings:
  critical: 1
  warning: 4
  info: 4
  total: 9
status: issues_found
---

# Phase 04: Code Review Report

**Reviewed:** 2026-04-10T00:00:00Z
**Depth:** standard
**Files Reviewed:** 7
**Status:** issues_found

## Summary

Seven files were reviewed covering the JS rendering layer (`preview.html`), the C++ WebView2 host (`PreviewPanel.cpp` and `PreviewPanel.h`), the Windows version resource (`MarkdownPreview.rc`), the MSBuild project file (`MarkdownPreview.vcxproj`), the Plugin-Admin manifest (`manifest.json`), and the packaging script (`scripts/package.ps1`).

The code is in generally good shape. The critical finding is a missing `sanitizeZoom` bounds clamp on the JS side: the `zoom` message handler in `preview.html` applies `msg.level` directly to `document.body.style.zoom` with no range validation, so a malformed or replayed message can set arbitrary zoom values (including 0, negative, or enormous values that freeze the renderer). The warnings are: `WideCharToMultiByte` return value unchecked before using the result as a buffer size (silent data truncation), a possible null-pointer dereference when `SCI_GETLENGTH` returns 0 and a buffer of size 1 is sent to `SCI_GETTEXT`, one-way `NavigationCompleted` token that fires on every future navigation (not just the first), and the `manifest.json` shipping with an unresolved placeholder `id` field. The info items are minor: a hardcoded version string in the HTML idle screen, debug-level `console.error` left in `exportHtml`, the `.vcxproj` `PlatformToolset` set to `v145` (VS2022 ships `v143`), and the packaging script not verifying the hash format before printing.

---

## Critical Issues

### CR-01: Unvalidated zoom level applied directly to `document.body.style.zoom`

**File:** `MarkdownPreview/assets/preview.html:753`
**Issue:** The `zoom` message handler sets `document.body.style.zoom = String(msg.level)` with no bounds check. `msg.level` comes from a `PostWebMessageAsJson` call originating in C++ (`postZoomToJs`), but C++ itself clamps only during interactive key-presses — not for the initial zoom replay from settings (`applyInitialZoom`). If the persisted `settings.json` value is corrupted (e.g., `-1`, `0`, `1e308`, or a non-number that survives `nlohmann::json::value("level", <float>)` coercion), the raw value reaches the JS `zoom` property. Setting `zoom: 0` collapses the viewport; setting an enormous float can freeze the renderer. Because `PostWebMessageAsJson` is the only input path, this is a logic-correctness issue rather than a remote-attack surface — but it can produce a non-recoverable blank panel.

**Fix:**
```javascript
case 'zoom':
    // THME-04: clamp to the documented 80%–800% range before applying.
    var level = parseFloat(msg.level);
    if (!isFinite(level) || level < 0.1) level = 1.0;   // reject 0, negative, NaN
    if (level > 8.0) level = 8.0;                        // cap at 800%
    document.body.style.zoom = String(level);
    break;
```

---

## Warnings

### WR-01: `WideCharToMultiByte` return value used as buffer size without checking for 0

**File:** `MarkdownPreview/src/PreviewPanel.cpp:482-486` and `489-493`
**Issue:** The pattern used in `renderMarkdown()` calls `WideCharToMultiByte` to compute the required buffer length, stores the result in `utf8len` / `pathLen` (both `int`), and then immediately constructs `std::string(static_cast<size_t>(utf8len), '\0')` and calls the conversion again. If the conversion fails (returns 0), `utf8len` is 0, the `std::string` is default-constructed empty, and the second call writes nothing — which means `nlohmann` receives an empty markdown or empty filePath. No crash, but silent data loss (render produces blank output with no error). The same pattern repeats for `pathLen`. The `utf8len == 0` case is only guarded further downstream, not at the point of construction.

**Fix:**
```cpp
int utf8len = ::WideCharToMultiByte(CP_UTF8, 0, wtext.c_str(),
    static_cast<int>(wtext.size()), nullptr, 0, nullptr, nullptr);
if (utf8len <= 0) return;  // conversion failed — bail rather than sending empty render
std::string utf8Markdown(static_cast<size_t>(utf8len), '\0');
// ... repeat for pathLen
```

### WR-02: `SCI_GETTEXT` buffer is one byte too small when `SCI_GETLENGTH` returns 0

**File:** `MarkdownPreview/src/PreviewPanel.cpp:447-450`
**Issue:** `getCurrentText()` allocates `std::string utf8Text(len + 1, '\0')` and passes `len + 1` as the buffer length to `SCI_GETTEXT`. When `len == 0` (empty document), this sends a 1-byte buffer. `SCI_GETTEXT` writes a NUL terminator so this is not a buffer overrun, but `len` was checked to be `< 0` only — a zero-length document skips the `len < 0` early return and goes through the full path. The subsequent `utf8Text.resize(len)` call then resizes to 0, which is correct. While not a crash, the separate early-exit path at line 459 (`if (utf8Text.empty()) return L""`) covers this after the resize, meaning the `MultiByteToWideChar` path is never reached for an empty document. This is safe today, but the logic is fragile: if the order of statements ever changes, the `wlen <= 0` guard further down would be the only protection. Clarify the intent with an explicit early return at the top:

**Fix:**
```cpp
LRESULT len = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
if (len <= 0) return L"";  // empty or error — nothing to retrieve
```

### WR-03: `NavigationCompleted` event fires on every navigation, not just the first

**File:** `MarkdownPreview/src/PreviewPanel.cpp:341-359`
**Issue:** The `NavigationCompleted` callback is registered without ever being removed (the token `m_navigationCompletedToken` is stored but `remove_NavigationCompleted` is only called in `destroy()`). The callback calls `applyInitialZoom`, `setTheme`, and possibly `renderMarkdown` every time a navigation completes. Under normal usage this fires only once (the initial `Navigate` to `preview.html`). However, if any code ever calls `m_webview->Navigate(...)` again (e.g., on error recovery or a future feature), the callback fires a second time and replays zoom/theme/pending-render, which can cause a duplicate render or a stale `m_pendingFilePath` to be replayed. This is a latent bug whose trigger depends on future code paths.

**Fix:** Unregister the token inside the callback itself after first fire, so only the initial navigation triggers the setup sequence:
```cpp
[this](ICoreWebView2* /*sender*/, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
    // Unregister immediately — setup fires only on the first navigation
    if (m_navigationCompletedToken.value != 0) {
        m_webview->remove_NavigationCompleted(m_navigationCompletedToken);
        m_navigationCompletedToken = {};
    }
    try {
        applyInitialZoom(m_zoomLevel);
        setTheme(m_isDark);
        if (!m_pendingFilePath.empty()) { ... }
    } catch (...) {}
    return S_OK;
}
```

### WR-04: `manifest.json` ships with unresolved placeholder `id` field

**File:** `manifest.json:4`
**Issue:** The `"id"` field contains the literal string `"REPLACE_WITH_SHA256_FROM_PACKAGE_PS1"`. This is the value that the Notepad++ Plugin Admin reads to verify the downloaded ZIP's integrity. If this file is committed and released as-is, Plugin Admin will either reject the package or — if it treats the `id` field as informational — install an unverified package. This is not a runtime bug, but it is a correctness defect in the publication artifact that would cause immediate distribution failure.

**Fix:** Run `scripts/package.ps1`, capture the printed SHA-256, and update `manifest.json`:
```json
{
  "id": "<sha256-from-package-ps1-output>"
}
```
Consider adding a validation step in `package.ps1` that writes the hash back into `manifest.json` automatically to prevent this mistake recurrence.

---

## Info

### IN-01: Hardcoded version string in preview idle screen does not match `manifest.json`

**File:** `MarkdownPreview/assets/preview.html:239`
**Issue:** The idle screen displays `v0.1.0` hardcoded in HTML while `manifest.json` declares `"version": "1.0.0"` and `MarkdownPreview.rc` sets `FILEVERSION 1,0,0,0`. The idle screen version is stale and will diverge further as the plugin version is bumped. Not a bug, but a user-visible inconsistency.

**Fix:** Update the idle screen version to match the release version. For long-term maintenance, consider having `package.ps1` perform a string substitution in the HTML at packaging time, or have C++ post the version to JS at startup via a `version` message type.

### IN-02: `console.error` left in `exportHtml` catch block

**File:** `MarkdownPreview/assets/preview.html:717`
**Issue:** `console.error('Export failed:', err)` is in the catch block of `exportHtml`. WebView2 has DevTools disabled (`settings->put_AreDevToolsEnabled(FALSE)` in `PreviewPanel.cpp:311`), so this output is not visible to end users. However, it is production code with an active logging call that would appear in a DevTools-enabled debug build and adds noise if DevTools are re-enabled during testing.

**Fix:** Remove the `console.error` call or guard it behind a debug flag:
```javascript
} catch (err) {
    // Export failed silently — C++ receives no exportReady message.
}
```

### IN-03: `PlatformToolset` set to `v145` instead of `v143`

**File:** `MarkdownPreview/MarkdownPreview.vcxproj:32,38,44,50`
**Issue:** All four configurations specify `<PlatformToolset>v145</PlatformToolset>`. Visual Studio 2022 ships with toolset `v143`; `v145` is a future/non-existent toolset version. MSBuild silently falls back to the latest installed toolset when the specified version is not found, so this has not caused a build failure. However, if a future VS update installs `v145` with different ABI or behavior, the build could change silently. The correct value for VS2022 is `v143`.

**Fix:** Replace all four occurrences:
```xml
<PlatformToolset>v143</PlatformToolset>
```

### IN-04: `package.ps1` does not validate that computed hash is a 64-character hex string

**File:** `scripts/package.ps1:68`
**Issue:** `Get-FileHash` with `-Algorithm SHA256` always returns a 64-character uppercase hex string in PowerShell 5.1+, so the `.ToLower()` call is safe. However, the script prints the hash and terminates without any assertion that the hash is the expected length. If a future PowerShell version or a different algorithm is mistakenly specified, a short or malformed hash would be silently printed and potentially copied into `manifest.json`. This is a minor robustness gap, not a current bug.

**Fix:**
```powershell
$hash = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToLower()
if ($hash.Length -ne 64) {
    Write-Error "Unexpected hash length $($hash.Length) — expected 64 hex characters."
}
```

---

_Reviewed: 2026-04-10T00:00:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
