---
phase: 02-core-preview
plan: "05"
subsystem: preview-rendering
tags: [utf8, custom-css, resource-cleanup, frontmatter, image-paths, gap-closure]
dependency_graph:
  requires: []
  provides: [THME-02-custom-css, utf8-safe-postmessage, webview2-cleanup]
  affects: [PreviewPanel.cpp, preview.html]
tech_stack:
  added: []
  patterns: [MultiByteToWideChar-CP_UTF8, nlohmann-json-null-field, EventRegistrationToken-cleanup]
key_files:
  created: []
  modified:
    - MarkdownPreview/src/PreviewPanel.cpp
    - MarkdownPreview/assets/preview.html
decisions:
  - "Read custom.css on every render call (not cached) — ensures user edits take effect immediately without restart"
  - "Send customCss as JSON null when absent — allows JS to distinguish 'no CSS' from 'empty CSS'"
  - "Utf8ToWide() as file-static helper — avoids header pollution, accessible to all 4 call sites in same TU"
metrics:
  duration: "~20 minutes"
  completed: "2026-04-09"
  tasks_completed: 2
  tasks_total: 2
  files_changed: 2
---

# Phase 02 Plan 05: Gap Closure — UTF-8 Conversion, THME-02 Custom CSS, and Edge Case Hardening Summary

**One-liner:** UTF-8-safe PostWebMessageAsJson via Utf8ToWide() helper, THME-02 custom CSS wiring from %APPDATA%, WR-01 token cleanup, CR-03 env-var validation, WR-05 BOM/CRLF frontmatter fix, and WR-06 parenthetical image filename support.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Fix C++ UTF-8 conversion, wire custom CSS, fix resource leaks | `6c1f18b` | PreviewPanel.cpp |
| 2 | Fix JS stripFrontmatter and rewriteImagePaths in preview.html | `cbf4ada` | preview.html |

## Changes Made

### PreviewPanel.cpp (Task 1)

**1. Utf8ToWide() static helper added**

A file-static `Utf8ToWide(const std::string& utf8) -> std::wstring` function was inserted after `using namespace Microsoft::WRL;`. It uses `MultiByteToWideChar(CP_UTF8, ...)` — the same approach already used in `getCurrentText()`. All four `PostWebMessageAsJson` call sites (`renderMarkdown`, `setTheme`, `scrollToLine`, `triggerExport`) now use this helper instead of the iterator constructor that zero-extends bytes and corrupts multi-byte sequences.

**2. THME-02 custom CSS wiring**

`renderMarkdown()` now reads `%APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css` via `std::ifstream` before serialising the JSON payload. The file content is sent as `j["customCss"]` (string) when present and non-empty, or as `nullptr` (JSON null) when absent or empty. The JS side already had `setCustomCss()` and the `msg.customCss` read path wired — the C++ side was the missing link. Absence of the file is silently ignored.

**3. CR-03: getUserDataPath() LOCALAPPDATA validation**

`GetEnvironmentVariableW` return value is now checked (`ret == 0 || ret >= MAX_PATH`). On failure, the fallback path `getAssetsPath() + L"\\WebView2Data"` is returned instead of a root-relative path that would fail silently.

**4. WR-01: WebMessageReceived token unregistration**

`destroy()` now calls `m_webview->remove_WebMessageReceived(m_webMessageReceivedToken)` before `m_controller->Close()`. The token is zeroed after unregistration. This prevents use-after-free when the panel is destroyed and re-initialized rapidly.

### preview.html (Task 2)

**5. WR-05: stripFrontmatter BOM and CRLF handling**

`stripFrontmatter()` now:
- Strips UTF-8 BOM (`U+FEFF`) at position 0 before checking for `---`
- Uses `markdown.search(/\r?\n---(\r?\n|$)/)` instead of `indexOf('\n---', 3)` to locate the closing delimiter, correctly handling both `\n` and `\r\n` line endings
- Advances past the delimiter using `indexOf('\n', end + 1) + 1`

**6. WR-06: rewriteImagePaths parenthetical filename support**

The image path regex's path capture group was updated from `[^)]+` to `[^)\s][^)]*(?:\([^)]*\)[^)]*)*)`. This allows single-level balanced parentheses inside the filename (e.g., `image (1).png`, `chart (v2).svg`) while still requiring a non-empty, non-whitespace-starting match.

## Gaps Closed

| Gap | Requirement | Resolution |
|-----|-------------|------------|
| Non-ASCII markdown blanks the preview | REND-01, REND-02 | Utf8ToWide() at all 4 PostWebMessageAsJson call sites |
| Custom CSS never applied | THME-02 | renderMarkdown() reads and sends custom.css content |

## Deviations from Plan

None — plan executed exactly as written. All 5 changes applied, all acceptance criteria met.

## Verification Results

```
grep -c "wjson(jsonStr.begin(), jsonStr.end())" PreviewPanel.cpp  → 0  (all replaced)
grep -c "Utf8ToWide" PreviewPanel.cpp                             → 5  (1 def + 4 sites)
grep "customCss" PreviewPanel.cpp                                 → 4 lines (content + 3 null)
grep "remove_WebMessageReceived" PreviewPanel.cpp                 → 1 line in destroy()
grep "ret == 0 || ret >= MAX_PATH" PreviewPanel.cpp               → 1 line in getUserDataPath()
grep "charCodeAt(0) === 0xFEFF" preview.html                      → 1 match in stripFrontmatter
grep -c "indexOf.*\n---" preview.html                             → 0  (old pattern gone)
balanced-paren regex [^)\s][^)]*(?:\([^)]*\)[^)]*)*              → present in rewriteImagePaths
```

## Known Stubs

None. All changes are fully wired end-to-end.

## Self-Check: PASSED

- `MarkdownPreview/src/PreviewPanel.cpp` — exists and modified
- `MarkdownPreview/assets/preview.html` — exists and modified
- Commit `6c1f18b` — exists (Task 1)
- Commit `cbf4ada` — exists (Task 2)
