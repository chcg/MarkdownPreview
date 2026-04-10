# Retrospective: MarkdownPreview

---

## Milestone: v1.0 MVP

**Shipped:** 2026-04-10
**Phases:** 4 | **Plans:** 17 | **Timeline:** 3 days (2026-04-08 → 2026-04-10)

### What Was Built

- Phase 1: C++ Notepad++ plugin DLL (x86/x64) with WebView2 docking panel, Ctrl+Shift+M toggle, and JSON settings persistence
- Phase 1: WebView2 lazy init with runtime detection, fallback SysLink UI, and virtual host asset serving
- Phase 2: Full GFM live preview — markdown-it, highlight.js, scroll sync, local image resolution, YAML strip, dark/light themes, custom CSS, HTML export
- Phase 3: KaTeX math, Mermaid diagrams, footnotes, copy-to-clipboard, keyboard zoom (Ctrl+=/−/0), PDF export via WebView2 PrintToPdf
- Phase 4: Click-to-editor navigation, TOC sidebar, DLL version resource, manifest.json, automated zip packager

### What Worked

- **Pre-bundled JS/CSS assets** — downloading pinned versions to `assets/` and serving via virtual host mapping worked flawlessly. No CDN dependencies at runtime.
- **Gap plans** — when UAT revealed real bugs (Mermaid getBBox off-screen failure, zoom key focus race), inserting a focused gap plan rather than expanding the original plan kept commits clean and traceable.
- **Data-line source map** — the `md.core.ruler.push('source_map', ...)` pattern annotating block tokens with `data-line` paid dividends twice: once for scroll sync and again for click-to-editor navigation. Good architecture decision.
- **Stub-then-replace pattern** — `scrollToLine()` and `exportHtml()` as intentional stubs in preview.html let phases proceed independently without coupling. Clean integration.
- **nlohmann/json for all C++→JS messages** — enforced in Phase 2 as the only way to build PostWebMessageAsJson payloads; prevented any injection risk from string concatenation.

### What Was Inefficient

- **Requirements checkboxes not updated during execution** — all 24 requirements were shipped but the REQUIREMENTS.md checkboxes stayed "Pending" throughout. Required bulk update at milestone close. Should update per-phase during transitions.
- **Roadmap progress table not updated** — the `Plans Complete` and `Status` columns stayed at planning-complete state throughout execution. Cosmetic, but created a false impression of zero progress.
- **Worktree NuGet packages junction** — packages directory not present in worktrees required a Windows junction point as a workaround. Not ideal for reproducibility.
- **`summary-extract` CLI misread GAP summaries** — the accomplishment extractor picked up "One-liner:" label lines from non-standard GAP summary format. Required manual fixup in MILESTONES.md.

### Patterns Established

- **`PreviewPanel` as the central WebView2 host** — all messaging, virtual host management, zoom, PDF, and event registration lives here. C++ side is thin wrappers; logic is in PreviewPanel.
- **Defensive stub pattern** — `if (window.plugin)` guards in preview.html prevent silent failure when optional assets aren't loaded.
- **Zoom reset/restore around PrintToPdf** — explicit zoom-to-1.0 before print, restore in both success and failure branches of the completion callback.
- **Lazy docking registration** — register once on first toggle via `NPPM_DMMREGASDCKDLG`, then DMMSHOW/DMMHIDE only. Avoids initialization timing issues.
- **`onNppReady` / `onNppShutdown` delegation** — thin `PluginMain.cpp` dispatches to logic in `PluginDefinition.cpp`. Keeps the entry point clean.

### Key Lessons

1. **Virtual host mapping requires DENY_CORS** — `SetVirtualHostNameToFolderMapping` with `COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS` prevents cross-origin leakage. Apply this to every new virtual host.
2. **NPPN_BUFFERACTIVATED is `NPPN_FIRST + 10`, not `+9`** — the template had the wrong offset. Verify all NPPN_* constants against Notepad++ source, not templates.
3. **mermaid.render() not mermaid.run()** — `run()` inserts error SVGs on invalid syntax; `render()` rejects cleanly, enabling the "leave code block on failure" fallback.
4. **Mermaid needs off-screen DOM container** — `getBBox()` fails on detached nodes. Must attach a hidden container to `document.body` before calling `mermaid.render()`.
5. **Zoom via AcceleratorKeyPressed on controller** — zoom keys must be intercepted on the controller (not webview) with `put_Handled(TRUE)` to suppress WebView2's built-in Ctrl+scroll zoom.
6. **`panel destroy` belongs in `NPPN_SHUTDOWN`, not `DLL_PROCESS_DETACH`** — calling `DestroyWindow` on the docking panel HWND at detach corrupts WebView2 user data. NPP's docking manager owns the HWND.

### Cost Observations

- Sessions: ~6 sessions across 3 days
- All phases completed without human intervention during execution (yolo mode)
- Gap plans needed for 2 phases (Phase 2 and Phase 3) — both caught real bugs not visible during planning

---

## Cross-Milestone Trends

| Metric | v1.0 |
|--------|------|
| Days to ship | 3 |
| Phases | 4 |
| Plans | 17 |
| Gap plans needed | 2 |
| Requirements delivered | 24/24 |
| Blocking deviations auto-fixed | ~10 |
