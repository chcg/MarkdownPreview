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
  artifacts: []
  missing: []

- truth: "Custom CSS applied to preview background (pink with !important override)"
  status: failed
  reason: "User reported: preview panel background is white now even though I am in dark mode"
  severity: major
  test: 2
  artifacts: []
  missing: []
