---
status: complete
phase: 02-core-preview
source: [02-VERIFICATION.md]
started: 2026-04-09T16:18:33Z
updated: 2026-04-09T20:05:00Z
---

## Current Test

[testing complete]

## Tests

### 1. Non-ASCII rendering
expected: Open a .md file containing CJK characters (e.g., `# 你好世界`), accented characters, and emoji (e.g., `🎉 Done`) — preview renders the content correctly, not "Preview unavailable" or blank content
result: issue
reported: "When i attempt to edit a md file, npp crashed"
severity: blocker

### 2. Custom CSS application
expected: Place `custom.css` containing `body { background: pink !important; }` at `%APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css`, then activate a .md file — preview background changes to pink
result: issue
reported: "preview panel background is white now even though I am in dark mode"
severity: major

## Summary

total: 2
passed: 0
issues: 2
pending: 0
skipped: 0
blocked: 0

## Gaps

- truth: "Preview renders CJK characters, accented characters, and emoji correctly"
  status: failed
  reason: "User reported: When i attempt to edit a md file, npp crashed"
  severity: blocker
  test: 1
  root_cause: |
    getCurrentText() in PreviewPanel.cpp:425-429 casts SCI_GETLENGTH LRESULT (64-bit) to int (32-bit).
    If len is negative after truncation, size_t wraps to a huge value; SCI_GETTEXT then writes into an
    undersized heap buffer → crash. Triggered on every keystroke via SCN_MODIFIED → scheduleRender → doRender → getCurrentText.
  fix: "Change `int len` to `LRESULT len` (or `Sci_Position len`) and guard against negative values before allocating."
  artifacts: [PreviewPanel.cpp:425-429, PluginDefinition.cpp:167]
  missing: []

- truth: "Custom CSS applied to preview background (pink with !important override)"
  status: failed
  reason: "User reported: preview panel background is white now even though I am in dark mode"
  severity: major
  test: 2
  root_cause: |
    Two bugs:
    1. custom.css path uses hardcoded %APPDATA%\Notepad++ instead of m_configPath (from NPPM_GETPLUGINSCONFIGDIR).
       File not found → cssFile.is_open() false → null CSS sent → setCustomCss(null) removes any style. (PreviewPanel.cpp:476-496)
    2. Dark mode: setTheme(isDark) called in onNppReady() before WebView2 exists; guard `!m_webview2Initialized`
       silently drops it. NavigationCompleted callback replays pending render but never replays m_isDark.
       (PreviewPanel.cpp:505-507, PluginDefinition.cpp:97-98, PreviewPanel.cpp:330-342)
  fix: |
    1. Use m_configPath to build custom.css path instead of hardcoded %APPDATA%\Notepad++.
    2. In NavigationCompleted callback, after applyInitialZoom(), also send the stored m_isDark as a theme message.
  artifacts: [PreviewPanel.cpp:476-496, PreviewPanel.cpp:505-507, PreviewPanel.cpp:330-342, PluginDefinition.cpp:97-98]
  missing: []
