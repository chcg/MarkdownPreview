# Architecture Research

**Domain:** Notepad++ plugin with embedded WebView2 for live markdown preview
**Researched:** 2026-04-08
**Confidence:** HIGH

## System Overview

```
+------------------------------------------------------------------+
|                        Notepad++ Process                         |
|  +------------------------------------------------------------+  |
|  |                    Notepad++ Host (exe)                     |  |
|  |  Scintilla Editor  |  Plugin Manager  |  Dockable Panels   |  |
|  +--------+-----------+--------+---------+---------+----------+  |
|           |                    |                   |             |
|  +--------v--------------------v-------------------v----------+  |
|  |              MarkdownPreview.dll (Plugin DLL)              |  |
|  |                                                            |  |
|  |  +--------------+  +---------------+  +----------------+   |  |
|  |  | Plugin Core  |  | Notification  |  | Dockable Panel |   |  |
|  |  | (Exports &   |  | Handler       |  | Manager        |   |  |
|  |  |  Lifecycle)  |  | (beNotified)  |  | (HWND Host)    |   |  |
|  |  +------+-------+  +-------+-------+  +-------+--------+   |  |
|  |         |                  |                   |            |  |
|  |  +------v------------------v-------------------v--------+   |  |
|  |  |              Preview Controller                      |   |  |
|  |  |  - Debounce timer    - Markdown text extraction      |   |  |
|  |  |  - File type check   - Scroll position mapping       |   |  |
|  |  +---------------------------+---+---------------------+   |  |
|  |                              |   |                          |  |
|  +------------------------------+---+-------------------------+  |
|                                 |   |                            |
|  +------------------------------v---v-------------------------+  |
|  |            WebView2 (Edge Chromium, child HWND)            |  |
|  |                                                            |  |
|  |  +------------------+  +-------------------------------+   |  |
|  |  | HTML Shell Page  |  | JS Rendering Engine           |   |  |
|  |  | (template.html)  |  | - markdown-it (parser)        |   |  |
|  |  |                  |  | - highlight.js (code blocks)  |   |  |
|  |  |                  |  | - KaTeX (math)                |   |  |
|  |  |                  |  | - mermaid (diagrams)          |   |  |
|  |  +------------------+  +-------------------------------+   |  |
|  |                                                            |  |
|  |  +------------------------------------------------------+  |  |
|  |  | CSS Themes (light.css, dark.css, user-custom.css)    |  |  |
|  |  +------------------------------------------------------+  |  |
|  +------------------------------------------------------------+  |
+------------------------------------------------------------------+
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| **Plugin Core** | DLL exports, lifecycle management, menu registration | 6 required C++ exports: `setInfo`, `getName`, `getFuncsArray`, `beNotified`, `messageProc`, `isUnicode` |
| **Notification Handler** | Listens to Npp/Scintilla events, routes to Preview Controller | Switch on notification codes in `beNotified()` |
| **Dockable Panel Manager** | Creates/manages the dockable HWND that hosts WebView2 | Win32 dialog/window registered via `NPPM_DMMREGASDCKDLG` |
| **Preview Controller** | Orchestrates updates: extracts text, debounces, sends to WebView2 | C++ class coordinating between Scintilla and WebView2 |
| **WebView2 Host** | Creates and manages WebView2 environment, controller, and web view | `CreateCoreWebView2EnvironmentWithOptions` + child HWND parenting |
| **HTML Shell Page** | Static HTML loaded once; receives markdown via postMessage | Single `template.html` with JS message listener |
| **JS Rendering Engine** | Parses markdown, renders HTML, applies syntax highlighting | markdown-it + plugins, running inside WebView2 |
| **CSS Themes** | Visual styling of rendered output | Swappable CSS files, user-customizable |

## Recommended Project Structure

```
src/
├── plugin/                    # Notepad++ plugin interface
│   ├── PluginDefinition.h     # Plugin name, menu items, command IDs
│   ├── PluginDefinition.cpp   # Command implementations
│   ├── PluginInterface.h      # Required export declarations
│   ├── PluginInterface.cpp    # setInfo, getName, getFuncsArray, beNotified, messageProc, isUnicode
│   └── NppData.h              # Notepad++ handle storage struct
├── core/                      # Business logic
│   ├── PreviewController.h    # Orchestrates preview updates
│   ├── PreviewController.cpp  # Debounce, text extraction, scroll sync
│   ├── NotificationHandler.h  # Routes Npp/Scintilla notifications
│   ├── NotificationHandler.cpp
│   ├── ScintillaHelper.h      # Scintilla message wrappers (get text, get scroll pos)
│   └── ScintillaHelper.cpp
├── webview/                   # WebView2 integration
│   ├── WebViewHost.h          # WebView2 environment/controller lifecycle
│   ├── WebViewHost.cpp        # CreateCoreWebView2Environment, HWND management
│   ├── WebViewMessages.h      # Native-to-JS and JS-to-native message definitions
│   └── WebViewMessages.cpp
├── panel/                     # Dockable panel UI
│   ├── DockablePanel.h        # Win32 dockable dialog hosting WebView2
│   ├── DockablePanel.cpp      # NPPM_DMMREGASDCKDLG registration, show/hide
│   └── DockablePanel.rc       # Resource file for the dialog
├── settings/                  # Configuration
│   ├── Settings.h             # Theme selection, debounce timing, export options
│   └── Settings.cpp           # JSON-based config file read/write
├── export/                    # Export functionality
│   ├── HtmlExporter.h         # Export to standalone HTML
│   ├── HtmlExporter.cpp
│   ├── PdfExporter.h          # Export to PDF (via WebView2 print API)
│   └── PdfExporter.cpp
├── resources/                 # Embedded web assets
│   ├── template.html          # Shell HTML page loaded into WebView2
│   ├── renderer.js            # Markdown rendering pipeline (markdown-it + plugins)
│   ├── scroll-sync.js         # Scroll synchronization logic
│   ├── themes/
│   │   ├── light.css          # Default light theme
│   │   ├── dark.css           # Default dark theme
│   │   └── github.css         # GitHub-flavored styling
│   └── libs/                  # Vendored JS libraries
│       ├── markdown-it.min.js
│       ├── highlight.min.js
│       ├── katex.min.js
│       └── mermaid.min.js
└── MarkdownPreview.def        # DLL export definitions
```

### Structure Rationale

- **plugin/:** Isolated Notepad++ API surface. Changes to Npp plugin API only affect this folder. Thin layer that delegates to core/.
- **core/:** Plugin logic independent of both Npp API details and WebView2 specifics. Testable in isolation.
- **webview/:** Encapsulates all WebView2 COM complexity. Single point of change if WebView2 API evolves.
- **panel/:** Win32 dockable panel mechanics separated from WebView2 hosting. The panel owns the HWND; WebView2 is a child.
- **resources/:** Web assets are separate files (not compiled in) so they can be edited, themed, and debugged independently. Consider embedding as resources for distribution simplicity.

## Architectural Patterns

### Pattern 1: Message-Based Native/Web Bridge

**What:** All communication between C++ and the WebView2 content uses `PostWebMessageAsJson` (native to web) and `window.chrome.webview.postMessage` (web to native). No host objects.
**When to use:** Always, for this plugin. Host objects add COM complexity and are slower for frequent, simple updates.
**Trade-offs:** Simple and fast for string/JSON data. Cannot expose native object APIs directly to JS (not needed here).

**Example (C++ side):**
```cpp
// Send markdown content to WebView2 for rendering
void PreviewController::UpdatePreview(const std::wstring& markdownText) {
    nlohmann::json msg;
    msg["type"] = "updateMarkdown";
    msg["content"] = WideToUtf8(markdownText);
    msg["scrollRatio"] = GetEditorScrollRatio();
    
    std::wstring jsonStr = Utf8ToWide(msg.dump());
    m_webView->PostWebMessageAsJson(jsonStr.c_str());
}
```

**Example (JS side):**
```javascript
window.chrome.webview.addEventListener('message', (event) => {
    const msg = event.data;
    switch (msg.type) {
        case 'updateMarkdown':
            renderMarkdown(msg.content);
            if (msg.scrollRatio !== undefined) {
                syncScroll(msg.scrollRatio);
            }
            break;
        case 'setTheme':
            applyTheme(msg.theme);
            break;
    }
});
```

### Pattern 2: Debounced Update Pipeline

**What:** Text changes from Scintilla (SCN_MODIFIED) are coalesced via a debounce timer before triggering a WebView2 update. This prevents rendering on every keystroke.
**When to use:** Always for live preview. Without debouncing, large files will freeze the editor.
**Trade-offs:** Adds latency (typically 150-300ms) between typing and preview update, but prevents editor jank.

**Example:**
```cpp
void NotificationHandler::OnTextModified() {
    // Kill any pending timer, restart the debounce
    KillTimer(m_panelHwnd, TIMER_ID_DEBOUNCE);
    SetTimer(m_panelHwnd, TIMER_ID_DEBOUNCE, 200 /* ms */, nullptr);
}

// In WndProc:
case WM_TIMER:
    if (wParam == TIMER_ID_DEBOUNCE) {
        KillTimer(hwnd, TIMER_ID_DEBOUNCE);
        m_previewController->UpdatePreview(GetCurrentDocumentText());
    }
    break;
```

### Pattern 3: Lazy WebView2 Initialization

**What:** WebView2 environment and controller are created only when first needed (first .md file opened), not at plugin load time. The environment is created once and reused.
**When to use:** Always. WebView2 cold start adds 100-500ms. Users who never open .md files should pay zero cost.
**Trade-offs:** First preview has a brief delay. Can mitigate by showing a "Loading..." message in the panel.

**Key implementation detail:** `CreateCoreWebView2EnvironmentWithOptions` is asynchronous (callback-based). The plugin must handle the case where the user opens a .md file but WebView2 is not yet ready -- queue the update and apply it when initialization completes.

### Pattern 4: Single Shell Page with Dynamic Content

**What:** Load a single HTML page (`template.html`) into WebView2 once via `NavigateToString` or `Navigate` to a local file. All subsequent markdown updates are sent via `PostWebMessageAsJson` and the JS renderer updates the DOM in-place.
**When to use:** Always. Avoids the cost of re-navigating on every update. Navigation is expensive; DOM manipulation is cheap.
**Trade-offs:** The shell page must be robust -- if its JS crashes, the preview breaks until the page is reloaded.

## Data Flow

### Primary Rendering Flow

```
User types in Scintilla editor
    |
    v
Scintilla sends SCN_MODIFIED notification
    |
    v
Notepad++ forwards to plugin's beNotified()
    |
    v
NotificationHandler checks: is active buffer a .md file?
    |-- NO --> ignore
    |-- YES
    v
NotificationHandler starts/resets debounce timer (200ms)
    |
    v  (timer fires)
PreviewController extracts full text via SCI_GETTEXT
    |
    v
PreviewController calls WebViewHost::PostMessage(json)
    |
    v
WebView2 receives message in JS context
    |
    v
renderer.js: markdown-it parses markdown to HTML
    |
    v
renderer.js: highlight.js processes code blocks
    |
    v
renderer.js: KaTeX processes math blocks
    |
    v
renderer.js: mermaid renders diagrams
    |
    v
DOM updated with rendered HTML
    |
    v
scroll-sync.js scrolls to matching position
```

### File Switch Flow

```
User switches tab / opens new file
    |
    v
NPPN_BUFFERACTIVATED notification
    |
    v
NotificationHandler checks file extension
    |-- .md file --> Show panel, extract text, trigger render
    |-- other   --> Optionally hide panel (configurable)
```

### Scroll Synchronization Flow

```
User scrolls in Scintilla editor
    |
    v
SCN_UPDATEUI notification (SC_UPDATE_V_SCROLL flag)
    |
    v
PreviewController calculates scroll ratio:
  ratio = firstVisibleLine / totalLines
    |
    v
PostMessage({ type: "scrollTo", ratio: 0.42 })
    |
    v
JS: document.documentElement.scrollTop = 
    ratio * document.documentElement.scrollHeight
```

### Key Data Flows

1. **Markdown rendering:** Scintilla text -> C++ extraction -> JSON message -> JS parser -> DOM update. Unidirectional, native-to-web only.
2. **Scroll sync (editor to preview):** Scintilla scroll event -> ratio calculation -> JSON message -> JS scroll. Unidirectional.
3. **Scroll sync (preview to editor, optional):** JS scroll event -> postMessage to native -> Scintilla SCI_SETFIRSTVISIBLELINE. Bidirectional requires care to avoid infinite loops (use a "syncing" guard flag).
4. **Theme change:** Settings UI -> C++ reads CSS file -> PostMessage with CSS content or theme name -> JS applies stylesheet.
5. **Export:** User triggers menu command -> C++ tells WebView2 to get rendered HTML or calls WebView2 print-to-PDF API.

## Scaling Considerations

| Concern | Small files (<1KB) | Medium files (10-100KB) | Large files (>500KB) |
|---------|---------------------|-------------------------|-----------------------|
| **Text extraction** | Direct SCI_GETTEXT | Direct SCI_GETTEXT | Consider SCI_GETTEXT with range; full extraction may cause brief pause |
| **Rendering** | Instant | Sub-100ms | Debounce critical; consider incremental/visible-only rendering |
| **Memory** | Negligible | Negligible | WebView2 DOM can grow large; consider limiting rendered scope |
| **Scroll sync** | Exact line mapping | Line mapping works | Line-to-pixel mapping becomes approximate |

### Scaling Priorities

1. **First bottleneck: Large file rendering.** markdown-it parsing 500KB+ of text takes noticeable time. Mitigation: longer debounce, render only visible portion, or use a Web Worker for parsing.
2. **Second bottleneck: Mermaid diagrams.** Complex diagrams are slow to render. Mitigation: detect mermaid blocks, render them lazily or cache rendered SVGs.

## Anti-Patterns

### Anti-Pattern 1: Re-navigating on Every Update

**What people do:** Call `NavigateToString(fullHtml)` every time the markdown changes.
**Why it's wrong:** Navigation tears down and rebuilds the entire page, including all JS state and the DOM. Causes flickering, loses scroll position, and is orders of magnitude slower than DOM updates.
**Do this instead:** Load the shell page once. Send markdown text via `PostWebMessageAsJson`. Let JS update the DOM in-place.

### Anti-Pattern 2: Synchronous Text Extraction in Notification Handler

**What people do:** Extract the full document text inside `beNotified()` on every SCN_MODIFIED.
**Why it's wrong:** `beNotified()` is called synchronously by Notepad++ on the main thread. Heavy work here blocks the editor UI. SCN_MODIFIED fires on every keystroke (and on undo/redo, paste, etc.).
**Do this instead:** In `beNotified()`, only set a flag or reset a timer. Do the actual text extraction and WebView2 communication in the timer callback.

### Anti-Pattern 3: Creating Multiple WebView2 Environments

**What people do:** Create a new `CoreWebView2Environment` every time the panel is shown or recreated.
**Why it's wrong:** Each environment spawns separate browser processes and allocates separate caches. Memory balloons. Startup is slow.
**Do this instead:** Create one environment at first use. Store it. Reuse for all subsequent controller/view creation.

### Anti-Pattern 4: Blocking on WebView2 Initialization

**What people do:** Wait synchronously for `CreateCoreWebView2EnvironmentWithOptions` to complete before returning from plugin initialization.
**Why it's wrong:** WebView2 creation is asynchronous by design. Blocking the thread freezes Notepad++ during startup.
**Do this instead:** Fire-and-forget the initialization. Queue any pending preview updates. Apply them when the async callback fires.

### Anti-Pattern 5: Bidirectional Scroll Sync Without Guard

**What people do:** Sync scroll in both directions (editor->preview and preview->editor) without preventing feedback loops.
**Why it's wrong:** Editor scrolls -> updates preview scroll -> preview scroll event fires -> updates editor scroll -> infinite loop causing jank.
**Do this instead:** Use a boolean guard (`m_isSyncing`). Set it before programmatic scroll changes, check it before forwarding scroll events.

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| **Notepad++ (host)** | Win32 SendMessage for NPPM_* messages; WM_NOTIFY for notifications | Plugin receives NppData struct with Npp HWND and two Scintilla HWNDs via setInfo() |
| **Scintilla** | Direct SendMessage to Scintilla HWND with SCI_* messages | Use the active Scintilla handle (main or secondary view); track which is active via NPPM_GETCURRENTSCINTILLA |
| **WebView2 Runtime** | COM-based; `CreateCoreWebView2EnvironmentWithOptions` | Requires WebView2 Runtime (Evergreen). Must handle absence gracefully with error message |
| **File System** | Read CSS themes, write exported HTML/PDF | User data folder for WebView2 cache; plugin config directory for settings |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Plugin Core <-> Notification Handler | Direct C++ function calls | Same DLL, same thread. Handler registered in beNotified() |
| Notification Handler <-> Preview Controller | C++ method calls, timer-based decoupling | Debounce timer decouples notification frequency from update frequency |
| Preview Controller <-> WebView2 Host | C++ method calls | Controller tells Host what to send; Host manages COM lifecycle |
| WebView2 Host <-> JS Rendering Engine | PostWebMessageAsJson / postMessage (async, cross-process) | This is the critical boundary: native C++ <-> Chromium renderer process. All data crosses as JSON strings. Keep messages small and infrequent |
| Dockable Panel <-> WebView2 Host | HWND parent-child relationship | Panel provides the parent HWND. WebView2 is sized to fill it. Panel resize -> must call `put_Bounds` on the controller |

## Build Order (Dependency Chain)

The following ordering reflects true dependencies -- each layer requires the one before it.

1. **Plugin skeleton** -- DLL exports, setInfo, beNotified, getName, getFuncsArray, messageProc, isUnicode. Loads into Notepad++ and appears in plugin menu. No functionality yet.
2. **Dockable panel** -- Win32 dialog registered with NPPM_DMMREGASDCKDLG. Toggle show/hide from menu. Empty panel visible.
3. **WebView2 hosting** -- Create environment and controller. Embed WebView2 as child of dockable panel HWND. Load a static "Hello World" HTML page. This validates the WebView2 integration works.
4. **Notification wiring** -- Hook NPPN_BUFFERACTIVATED, SCN_MODIFIED, SCN_UPDATEUI. Detect .md files. Extract text from Scintilla. Log or display raw text to confirm notification pipeline works.
5. **Markdown rendering** -- Load shell template.html with markdown-it. Send markdown text from C++ to JS via PostWebMessageAsJson. Render in WebView2 DOM. This is the core feature.
6. **Debounce and performance** -- Add timer-based debouncing. Test with large files. Optimize text extraction.
7. **Scroll synchronization** -- Calculate scroll ratio from Scintilla. Send to JS. Implement preview-to-editor sync with loop guard.
8. **Extended rendering** -- Add highlight.js for code blocks, KaTeX for math, mermaid for diagrams. Each is additive and independent.
9. **Theming** -- Load CSS themes. Dark/light mode. Custom CSS support. Settings persistence.
10. **Export** -- HTML export (get rendered HTML from WebView2). PDF export (WebView2 print-to-PDF API).

## Sources

- [Notepad++ Plugin Communication (official manual)](https://npp-user-manual.org/docs/plugin-communication/)
- [Notepad++ Plugins (official manual)](https://npp-user-manual.org/docs/plugins/)
- [Microsoft WebView2 Documentation](https://learn.microsoft.com/en-us/microsoft-edge/webview2/)
- [WebView2 Performance Best Practices](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/performance)
- [WebView2 Native/Web Interop](https://learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/communicate-btwn-web-native)
- [WebView2 User Data Folder Management](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/user-data-folder)
- [NppMarkdownPanel (existing plugin, C#)](https://github.com/mohzy83/NppMarkdownPanel)
- [NppAnotherMarkdown (existing plugin, C#)](https://github.com/ezyuzin/NppAnotherMarkdown)
- [NppCppMSVS Template](https://community.notepad-plus-plus.org/topic/26673/nppcppmsvs-a-visual-studio-project-template-for-a-notepad-c-plugin)
- [NPPN_GLOBALMODIFIED for Replace All](https://community.notepad-plus-plus.org/topic/25504/new-nppn_globalmodified-notification)
- [NPPM_ADDSCNMODIFIEDFLAGS (v8.7.7+)](https://community.notepad-plus-plus.org/topic/26595/new-api-to-fix-eventual-regression-regarding-scn_modified-for-some-plugins)

---
*Architecture research for: Notepad++ WebView2 Markdown Preview Plugin*
*Researched: 2026-04-08*
