---
status: partial
phase: 03-extended-rendering
source: [03-VERIFICATION.md]
started: 2026-04-09T19:00:00Z
updated: 2026-04-09T19:00:00Z
---

## Current Test

[awaiting human testing]

## Tests

### 1. KaTeX inline and block math renders correctly
expected: `$E=mc^2$` renders as inline math; `$$\int_0^\infty$$` renders as display block
result: [pending]

### 2. Malformed LaTeX shows inline error (not crash)
expected: Syntax error in `$...$` shows red error span inline, no exception or blank
result: [pending]

### 3. Footnotes render with superscript links and back-references
expected: `[^1]` in text becomes superscript link; `[^1]: text` at bottom becomes footnote with back-link
result: [pending]

### 4. Mermaid valid block renders as SVG diagram
expected: ` ```mermaid\ngraph TD\nA-->B\n``` ` renders as an SVG flowchart
result: [pending]

### 5. Mermaid invalid block falls back to code (D-01)
expected: Invalid Mermaid syntax leaves the `<pre><code>` block intact, no error SVG
result: [pending]

### 6. Copy button appears on hover; clipboard write works
expected: Hovering over a code block shows copy icon; clicking copies code to clipboard
result: [pending]

### 7. Ctrl+=/- zoom works visually; Ctrl+0 resets; clamped at bounds
expected: Ctrl+= enlarges preview, Ctrl+- shrinks, Ctrl+0 resets to 100%; bounds clamped at 80%-800%
result: [pending]

### 8. Zoom persists across Notepad++ restart
expected: Zoom level saved to settings.json; on restart, preview opens at last-used zoom
result: [pending]

### 9. PDF export produces .pdf file in same directory
expected: Ctrl+Shift+P (or menu) creates a .pdf in the same directory as the .md source
result: [pending]

### 10. PDF has page numbers in footer and filename in header at 100% zoom
expected: Generated PDF shows document basename as header and page numbers as footer
result: [pending]

### 11. User zoom resets to 100% during export and restores after
expected: If preview is at 150%, PDF exports at 100% zoom, then preview returns to 150%
result: [pending]

## Summary

total: 11
passed: 0
issues: 0
pending: 11
skipped: 0
blocked: 0

## Gaps
