---
status: complete
phase: 03-extended-rendering
source: [03-VERIFICATION.md]
started: 2026-04-09T19:00:00Z
updated: 2026-04-10T00:00:00Z
---

## Current Test

[testing complete]

## Tests

### 1. KaTeX inline and block math renders correctly
expected: `$E=mc^2$` renders as inline math; `$$\int_0^\infty$$` renders as display block
result: pass

### 2. Malformed LaTeX shows inline error (not crash)
expected: Syntax error in `$...$` shows red error span inline, no exception or blank
result: pass

### 3. Footnotes render with superscript links and back-references
expected: `[^1]` in text becomes superscript link; `[^1]: text` at bottom becomes footnote with back-link
result: pass

### 4. Mermaid valid block renders as SVG diagram
expected: ` ```mermaid\ngraph TD\nA-->B\n``` ` renders as an SVG flowchart
result: issue
reported: "it looks like a code block"
severity: major

### 5. Mermaid invalid block falls back to code (D-01)
expected: Invalid Mermaid syntax leaves the `<pre><code>` block intact, no error SVG
result: pass

### 6. Copy button appears on hover; clipboard write works
expected: Hovering over a code block shows copy icon; clicking copies code to clipboard
result: pass

### 7. Ctrl+=/- zoom works visually; Ctrl+0 resets; clamped at bounds
expected: Ctrl+= enlarges preview, Ctrl+- shrinks, Ctrl+0 resets to 100%; bounds clamped at 80%-800%
result: issue
reported: "The preview does not change, only the md file zooms"
severity: major

### 8. Zoom persists across Notepad++ restart
expected: Zoom level saved to settings.json; on restart, preview opens at last-used zoom
result: skipped
reason: untestable — zoom controls not affecting preview (test 7 blocked)

### 9. PDF export produces .pdf file in same directory
expected: Ctrl+Shift+P (or menu) creates a .pdf in the same directory as the .md source
result: issue
reported: "It does, but it is filled with Syntax error in text mermaid version 11.14.0"
severity: major

### 10. PDF has page numbers in footer and filename in header at 100% zoom
expected: Generated PDF shows document basename as header and page numbers as footer
result: pass

### 11. User zoom resets to 100% during export and restores after
expected: If preview is at 150%, PDF exports at 100% zoom, then preview returns to 150%
result: skipped
reason: untestable — zoom controls not affecting preview (test 7 blocked)

## Summary

total: 11
passed: 6
issues: 3
pending: 0
skipped: 2
blocked: 0

## Gaps

- truth: "Mermaid fenced code block renders as SVG diagram"
  status: failed
  reason: "User reported: it looks like a code block"
  severity: major
  test: 4
  artifacts: []
  missing: []

- truth: "Ctrl+=/- zooms the preview panel; Ctrl+0 resets to 100%; bounds clamped at 80%-800%"
  status: failed
  reason: "User reported: The preview does not change, only the md file zooms"
  severity: major
  test: 7
  artifacts: []
  missing: []

- truth: "PDF export produces a clean PDF of the rendered markdown"
  status: failed
  reason: "User reported: It does, but it is filled with Syntax error in text mermaid version 11.14.0"
  severity: major
  test: 9
  artifacts: []
  missing: []
