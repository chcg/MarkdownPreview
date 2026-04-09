---
phase: 03-extended-rendering
plan: 03
subsystem: zoom-controls
tags: [zoom, keyboard, settings, webview2, accelerator-key]
dependency_graph:
  requires: []
  provides: [zoom-keyboard-controls, zoom-persistence, webview2-environment-pointer]
  affects: [settings-json, preview-panel, preview-html]
tech_stack:
  added: []
  patterns:
    - AcceleratorKeyPressed handler on ICoreWebView2Controller for keyboard interception
    - put_Handled(TRUE) to suppress WebView2 built-in zoom behavior
    - CSS zoom property on document.body for viewport scaling
    - Immediate settings.json save on each zoom keypress for persistence
key_files:
  created: []
  modified:
    - MarkdownPreview/src/Settings.h
    - MarkdownPreview/src/Settings.cpp
    - MarkdownPreview/src/PreviewPanel.h
    - MarkdownPreview/src/PreviewPanel.cpp
    - MarkdownPreview/src/PluginDefinition.cpp
    - MarkdownPreview/assets/preview.html
decisions:
  - "Zoom step is 10% (0.1f float delta) per keypress — plan specified D-04 discretion"
  - "AcceleratorKeyPressed registered on controller (not webview) to intercept before web content receives key"
  - "g_settings accessed via extern in PreviewPanel.cpp — avoids circular include via PluginDefinition.h"
  - "m_configPath stored in PreviewPanel via setConfigPath() — cleanest pattern matching existing codebase"
  - "applyInitialZoom() called inside controller callback after navigation, reading g_settings.zoomLevel set by onNppReady()"
metrics:
  duration: ~10min
  completed: "2026-04-09T18:15:49Z"
  tasks_completed: 2
  files_modified: 6
---

# Phase 03 Plan 03: Keyboard Zoom Controls Summary

Zoom keyboard controls (Ctrl+=/Ctrl+-/Ctrl+0) wired from WebView2 AcceleratorKeyPressed handler through to CSS zoom on document.body, with 80-800% clamping and immediate settings.json persistence; WebView2 environment pointer stored for Plan 04 PDF export.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Add zoomLevel to Settings and zoom infrastructure to PreviewPanel | fcffa8d | Settings.h, Settings.cpp, PreviewPanel.h, PreviewPanel.cpp, PluginDefinition.cpp |
| 2 | Add 'zoom' message case to preview.html dispatcher | 6b5483d | preview.html |

## What Was Built

### Settings persistence (Settings.h/cpp)
- Added `float zoomLevel = 1.0f` field to `Settings` struct
- `Settings::load()` reads `"zoomLevel"` with default 1.0f (T-03-10: nlohmann `j.value()` uses default on missing/malformed field)
- `Settings::save()` writes `"zoomLevel"` to settings.json

### PreviewPanel zoom infrastructure (PreviewPanel.h/cpp)
- New private members: `m_zoomLevel`, `m_accelKeyToken`, `m_configPath`, `m_environment`
- New public methods: `setConfigPath()` (inline), `applyInitialZoom()`
- New private method: `postZoomToJs(float level)`
- `m_environment = env` stored in `initWebView2()` environment callback — required for Plan 04's `PrintToPdfAsync`
- `add_AcceleratorKeyPressed` registered on controller after `resizeWebView2()`:
  - Intercepts only KEY_DOWN / SYSTEM_KEY_DOWN events
  - Checks `VK_CONTROL` state, then `VK_OEM_PLUS` / `VK_OEM_MINUS` / `0x30` virtual keys
  - Calls `args->put_Handled(TRUE)` to suppress WebView2 built-in Ctrl+/-/0 zoom
  - Clamps zoom to `[0.8f, 8.0f]` (80%–800%)
  - Persists immediately: `g_settings.zoomLevel = m_zoomLevel; g_settings.save(m_configPath)`
  - Calls `postZoomToJs(m_zoomLevel)` to update the live preview
- After pending file path dispatch in controller callback: reads `g_settings.zoomLevel` (set in `onNppReady`) and calls `applyInitialZoom()` to restore persisted zoom
- `destroy()` unregisters `m_accelKeyToken` before `m_controller->Close()`

### PluginDefinition.cpp
- `onNppReady()` calls `g_previewPanel.setConfigPath(g_configPath)` after `g_settings.load()`

### preview.html
- Added `case 'zoom':` after `case 'export':` in the WebView2 message dispatcher
- Sets `document.body.style.zoom = String(msg.level)` — CSS zoom scales the entire rendering viewport

## Deviations from Plan

None — plan executed exactly as written. The plan's "REVISED PluginDefinition.cpp change" and "CHANGE C" wording was followed precisely: `m_zoomLevel = g_settings.zoomLevel; applyInitialZoom(m_zoomLevel)` inside the controller callback, with `setConfigPath()` called from `onNppReady()`.

## Known Stubs

None. All zoom functionality is fully wired: keyboard → C++ handler → settings.json + postZoomToJs → CSS zoom on document.body.

## Threat Flags

No new security-relevant surface beyond what is documented in the plan's threat model. The AcceleratorKeyPressed handler operates on UI thread with O(1) computation; zoom level is a C++-clamped float, not user string input.

## Self-Check: PASSED

Files exist:
- MarkdownPreview/src/Settings.h — FOUND (zoomLevel field present)
- MarkdownPreview/src/Settings.cpp — FOUND (load/save present)
- MarkdownPreview/src/PreviewPanel.h — FOUND (all members and methods present)
- MarkdownPreview/src/PreviewPanel.cpp — FOUND (handler, postZoomToJs, applyInitialZoom, m_environment present)
- MarkdownPreview/src/PluginDefinition.cpp — FOUND (setConfigPath call present)
- MarkdownPreview/assets/preview.html — FOUND (zoom case present)

Commits exist:
- fcffa8d — feat(03-03): add zoom infrastructure to Settings and PreviewPanel
- 6b5483d — feat(03-03): add zoom message case to preview.html dispatcher
