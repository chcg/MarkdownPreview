# Phase 2: Core Preview - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions captured in CONTEXT.md — this log preserves the discussion.

**Date:** 2026-04-09
**Phase:** 02-core-preview
**Mode:** discuss
**Areas discussed:** Auto-open behavior, Scroll sync precision, Theme switching, HTML export image handling

## Gray Areas Presented

| Area | Options offered | User choice |
|------|----------------|-------------|
| Auto-open behavior | Always / Once per session / Respect prior state | Always |
| Scroll sync precision | Source-map / Heading-anchored / Ratio-based | Source-map |
| Theme detection timing | Live via NPPN_DARKMODECHANGED / Startup only | Live via notification |
| Custom CSS location | Fixed plugin config dir / Path in settings | Fixed plugin config dir |
| HTML export images | Inline data URIs / Copy alongside / Relative paths | Inline data URIs |

## Discussion Notes

All areas: user selected the recommended option in every case. No corrections or scope creep.

**Auto-open:** Always re-open on .md file activation — no sticky-close behavior.

**Scroll sync:** Source-map via `data-line` attributes — most accurate, worthwhile investment for day-to-day usability.

**Theme:** Live switching preferred; live is low-cost (one notification handler) and avoids stale themes during long sessions.

**Custom CSS:** Fixed path (`%AppData%\Notepad++\plugins\config\MarkdownPreview\custom.css`) — zero configuration burden, fits existing settings pattern.

**HTML export:** Self-contained with data URIs — export file works anywhere without managing companion assets.

## Corrections Made

None — all recommended options confirmed.

## Prior Phase Decisions Applied

From Phase 1 (01-CONTEXT.md):
- D-04: Settings persistence → extended to cover new phase 2 settings fields
- D-06: Lazy WebView2 init → auto-open will trigger first registration if not yet initialized
- D-14: Settings JSON path → custom CSS placed in same plugin config directory tree
