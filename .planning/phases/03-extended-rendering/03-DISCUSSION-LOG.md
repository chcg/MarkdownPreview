# Phase 3: Extended Rendering - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-09
**Phase:** 03-extended-rendering
**Areas discussed:** Diagram error behavior, Copy button UX, Zoom interaction, PDF export UX

---

## Diagram Error Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Raw code fallback | Show fenced block as syntax-highlighted text — user sees source, can fix it | ✓ |
| Styled error in the diagram frame | Red-bordered placeholder box with Mermaid error message | |
| Silent fail | Diagram block disappears entirely | |

**User's choice:** Raw code fallback
**Notes:** None

---

## Copy Button UX

### Appearance

| Option | Description | Selected |
|--------|-------------|----------|
| Icon only, top-right | Clipboard SVG icon, top-right corner, visible on hover. GitHub-style. | ✓ |
| Icon + "Copy" label | Clipboard icon + word "Copy". More discoverable. | |
| You decide | Claude picks implementation. | |

**User's choice:** Icon only, top-right

### Feedback

| Option | Description | Selected |
|--------|-------------|----------|
| Icon flips to checkmark for 2s | Clipboard swaps to ✓ for 2 seconds then reverts | |
| "Copied!" text replaces label | Button text changes for 2 seconds | |
| No feedback | Silent copy | ✓ |

**User's choice:** No feedback

---

## Zoom Interaction

### Trigger

| Option | Description | Selected |
|--------|-------------|----------|
| Keyboard only: Ctrl+/Ctrl- | Standard shortcuts, no toolbar, Ctrl+0 resets | ✓ |
| Toolbar strip + keyboard | +/- buttons with zoom % display, plus keyboard | |
| Ctrl+scroll wheel only | Mouse-heavy; breaks normal page scrolling | |

**User's choice:** Keyboard only: Ctrl+/Ctrl-

### Persistence

| Option | Description | Selected |
|--------|-------------|----------|
| Persist globally in settings | Saved to settings.json, same for all files | ✓ |
| Reset on session/restart | Returns to 100% on Notepad++ restart | |
| Persist per-file | Separate zoom per .md file | |

**User's choice:** Persist globally in settings

---

## PDF Export UX

### Save Location

| Option | Description | Selected |
|--------|-------------|----------|
| Same dir as .md, auto-named | filename.pdf next to source, no dialog — mirrors HTML export | ✓ |
| File save dialog | User chooses location each time (requires Win32 C++ dialog) | |

**User's choice:** Same dir as .md, auto-named

### Paper Setup

| Option | Description | Selected |
|--------|-------------|----------|
| A4 portrait, fixed | Common default for technical docs | |
| Letter portrait, fixed | US-standard | ✓ |
| You decide | Claude picks | |

**User's choice:** Letter portrait, fixed

### Header/Footer

| Option | Description | Selected |
|--------|-------------|----------|
| Filename + page N of M | Footer: filename.md \| Page N of M. Header empty. | ✓ |
| Date + page number | Footer: YYYY-MM-DD \| Page N of M | |
| Page number only | Just "Page N" in footer | |

**User's choice:** Filename + page N of M

---

## Claude's Discretion

- KaTeX error rendering for malformed math
- Mermaid initialization strategy (lazy vs. eager)
- Copy button SVG icon design
- Zoom step size per keypress
- PDF margins
- Zoom keyboard implementation approach (Win32 vs. WebView2 AcceleratorKeyPressed)

## Deferred Ideas

None
