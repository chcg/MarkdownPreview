---
status: partial
phase: 02-core-preview
source: [02-VERIFICATION.md]
started: 2026-04-09T16:18:33Z
updated: 2026-04-09T16:18:33Z
---

## Current Test

[awaiting human testing]

## Tests

### 1. Non-ASCII rendering
expected: Open a .md file containing CJK characters (e.g., `# 你好世界`), accented characters, and emoji (e.g., `🎉 Done`) — preview renders the content correctly, not "Preview unavailable" or blank content
result: [pending]

### 2. Custom CSS application
expected: Place `custom.css` containing `body { background: pink !important; }` at `%APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css`, then activate a .md file — preview background changes to pink
result: [pending]

## Summary

total: 2
passed: 0
issues: 0
pending: 2
skipped: 0
blocked: 0

## Gaps
