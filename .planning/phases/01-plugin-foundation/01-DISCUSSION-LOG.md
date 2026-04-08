# Phase 1: Plugin Foundation - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions captured in CONTEXT.md — this log preserves the Q&A.

**Date:** 2026-04-08
**Phase:** 01-Plugin Foundation
**Mode:** discuss (interactive)
**Areas discussed:** Panel docking behavior, WebView2 missing experience, Toggle & shortcut design, Plugin identity

## Area: Panel Docking Behavior

| Question | Options Presented | Selected |
|----------|-------------------|----------|
| Where should the preview panel dock by default? | Right side, Bottom, Left side | Right side |
| Should the user be able to reposition the panel? | Yes (standard Notepad++ docking), No (lock to right) | Yes, standard Notepad++ docking |
| What initial width should the preview panel use? | 50% of window, One-third, You decide | 50% of Notepad++ window |
| Should the panel remember position/size between sessions? | Yes (persist), No (always reset) | Yes, persist across sessions |

## Area: WebView2 Missing Experience

| Question | Options Presented | Selected |
|----------|-------------------|----------|
| What should the user see when WebView2 is not installed? | Static message with download link, Offer auto-download, Just show error | Static message with download link |
| When should WebView2 availability be checked? | When panel is first opened, On plugin load (startup), You decide | When panel is first opened |
| Where should the missing-WebView2 message appear? | Inside the docking panel, Modal dialog box, You decide | Inside the docking panel |

## Area: Toggle & Shortcut Design

| Question | Options Presented | Selected |
|----------|-------------------|----------|
| What keyboard shortcut should toggle the preview panel? | Ctrl+Shift+M, Ctrl+Alt+P, No default shortcut, You decide | Ctrl+Shift+M |
| Where should the toggle appear in the menu? | Plugins > MarkdownPreview > Toggle Preview, Also add to View menu | Plugins > MarkdownPreview > Toggle Preview |
| Should the panel auto-open on .md files in Phase 1? | Manual toggle only, Auto-open from Phase 1 | Manual toggle only in Phase 1 |
| What should the placeholder page show? | Simple welcome message, Blank white page, You decide | Simple welcome message |

## Area: Plugin Identity

| Question | Options Presented | Selected |
|----------|-------------------|----------|
| What should the plugin be called? | MarkdownPreview, Markdown Preview (with space), NppMarkdownPreview | MarkdownPreview |
| Use NppCppMSVS template or minimal skeleton? | NppCppMSVS template as-is, Minimal custom skeleton, You decide | NppCppMSVS template as-is |
| How should plugin settings be stored? | JSON file in plugin config dir, Windows Registry, INI file | JSON file in plugin config dir |

## Corrections Made

No corrections — all recommendations confirmed.

## Deferred Ideas

None — discussion stayed within phase scope.
