# Pitfalls Research

**Domain:** Notepad++ markdown preview plugin (C++ DLL, WebView2, live rendering, scroll sync)
**Researched:** 2026-04-08
**Confidence:** HIGH (verified against official docs, GitHub issues, community reports)

## Critical Pitfalls

### Pitfall 1: NavigateToString 2MB Size Limit

**What goes wrong:**
WebView2's `NavigateToString()` method has a hard 2MB limit on the HTML string. When rendering large markdown files -- especially those with embedded base64 images, extensive code blocks, or Mermaid SVG output -- the rendered HTML easily exceeds 2MB. The call fails with "Value does not fall within the expected range" and the preview goes blank.

**Why it happens:**
Developers start with `NavigateToString()` because it is the simplest API for pushing HTML into WebView2. It works fine during development with small test files but silently breaks on real-world documents.

**How to avoid:**
Never use `NavigateToString()` as the primary rendering path. Instead:
1. Use `SetVirtualHostNameToFolderMapping()` to map a virtual hostname (e.g., `app.local`) to a local folder containing your HTML template.
2. Navigate to that virtual host URL once on initialization.
3. Push markdown content updates via `ExecuteScriptAsync()` / `PostWebMessageAsJson()` to JavaScript that performs incremental DOM updates.

This also solves the related problem that `NavigateToString()` sets the origin to `null`, breaking `localStorage`, `indexedDB`, and same-origin policies for loading local resources like images.

**Warning signs:**
- Preview works on small files but goes blank on large ones
- Console errors mentioning "Value does not fall within the expected range"
- Local image references (`![](./image.png)`) fail to load

**Phase to address:**
Phase 1 (Foundation) -- this is an architectural decision that must be correct from the start. Retrofitting from NavigateToString to virtual host mapping requires rewriting the entire content pipeline.

---

### Pitfall 2: SCN_MODIFIED Notification Flood and Missed Events

**What goes wrong:**
The plugin hooks `SCN_MODIFIED` from Scintilla to detect text changes and trigger re-renders. Two failure modes:
1. **Flood:** Each keystroke and especially Replace All operations fire many `SCN_MODIFIED` events. Without debouncing, the plugin re-renders on every single notification, freezing both the editor and the preview.
2. **Missed events:** Since Notepad++ v8.6.3, `SCN_MODIFIED` is suppressed during Replace All. Since v8.7.6, only 5 default flags are forwarded (`SC_MOD_DELETETEXT`, `SC_MOD_INSERTTEXT`, `SC_PERFORMED_UNDO`, `SC_PERFORMED_REDO`, `SC_MOD_CHANGEINDICATOR`). Plugins that need other flags must call `NPPM_ADDSCNMODIFIEDFLAGS` after `NPPN_READY`. Missing this means the preview silently stops updating after certain operations.

**Why it happens:**
Plugin developers test with manual typing and miss the Replace All edge case. The `NPPM_ADDSCNMODIFIEDFLAGS` API is new (v8.7.7) and poorly documented outside the Notepad++ community forum.

**How to avoid:**
- Always listen for `NPPN_GLOBALMODIFIED` in addition to `SCN_MODIFIED` to catch Replace All completions.
- After receiving `NPPN_READY`, send `NPPM_ADDSCNMODIFIEDFLAGS` with any additional notification flags the plugin requires.
- Implement a debounce timer (200-300ms) that coalesces rapid `SCN_MODIFIED` events into a single re-render.
- For large files (>2000 lines), increase debounce to 500-1000ms.

**Warning signs:**
- Editor becomes laggy while typing in large files
- Preview does not update after Find/Replace All
- Preview stops working after a Notepad++ update

**Phase to address:**
Phase 1 (Foundation) -- the notification handling architecture must be correct from the start. The debounce tuning can be refined in later phases.

---

### Pitfall 3: WebView2 User Data Folder Permissions and Corruption

**What goes wrong:**
WebView2 requires a "user data folder" for its browser profile (caches, cookies, GPU state). If the plugin does not explicitly set this folder:
1. WebView2 defaults to creating it alongside the executable -- which is `C:\Program Files\Notepad++\`, where the process has no write permission.
2. WebView2 initialization silently fails or crashes.
3. If a previous instance crashed, stale lock files in the user data folder prevent new instances from initializing.
4. If another plugin or application uses the same user data folder with different environment options, initialization fails with an incompatibility error.

**Why it happens:**
WebView2's default user data folder location assumes the application controls its own install directory. Notepad++ plugins run inside a host process they do not control, in a protected directory.

**How to avoid:**
- Explicitly set the user data folder to a plugin-specific subdirectory under `%APPDATA%\Notepad++\plugins\MarkdownPreview\WebView2Data\` (or the plugin config directory Notepad++ provides via `NPPM_GETPLUGINSCONFIGDIR`).
- Implement a startup check: if the lock file exists but the process is stale, delete the lock file before initialization.
- Handle `CreateCoreWebView2EnvironmentWithOptions` failure gracefully with a user-visible error message and retry logic.

**Warning signs:**
- Plugin works when Notepad++ is run as administrator but fails for normal users
- "Initialization failed due to incompatible environment configurations" errors
- WebView2 works once, then fails after a crash until the user manually clears a folder

**Phase to address:**
Phase 1 (Foundation) -- must be decided before the first WebView2 initialization code is written.

---

### Pitfall 4: Scroll Synchronization Between Scintilla and WebView2

**What goes wrong:**
Naive scroll sync uses a percentage-based approach: `editor scroll% = preview scroll%`. This breaks badly because:
1. A single markdown line (e.g., `![](image.png)`) can render as hundreds of pixels of height in the preview.
2. Code blocks, tables, and Mermaid diagrams have wildly different source-vs-rendered heights.
3. Collapsed `<details>` blocks in the preview have zero height but occupy source lines.
4. The preview "jumps" or "drifts" as the user scrolls, making it disorienting rather than helpful.

VS Code has had numerous scroll sync bugs filed over years, including issues where the preview scrolls down and then jumps back up, or where scroll sync at EOF causes excessive scrolling on every keystroke.

**Why it happens:**
Percentage-based sync is the obvious first implementation. It works acceptably for short, text-heavy documents but degrades on real documents with mixed content.

**How to avoid:**
Implement element-based (anchor-based) scroll synchronization:
1. During markdown parsing, annotate rendered HTML elements with `data-source-line` attributes indicating which source line produced them.
2. When the editor scrolls, find the top visible source line number.
3. In the WebView2, find the rendered element with the matching `data-source-line` and scroll it into view.
4. Interpolate between anchors for smooth scrolling (don't snap to discrete elements).
5. When scrolling from the preview back to the editor, reverse the mapping.

**Warning signs:**
- Preview jumps to wrong position when scrolling past images or code blocks
- Scroll position is approximately right for short files but wildly off for long ones
- Users report the preview "bouncing" or "fighting" their scroll input

**Phase to address:**
Phase 2 (after basic rendering works) -- scroll sync requires the rendering pipeline to be stable first. The `data-source-line` annotation should be designed into the markdown parsing approach from Phase 1, even if scroll sync is implemented later.

---

### Pitfall 5: DPI Scaling Mismatch Between Notepad++ and WebView2

**What goes wrong:**
WebView2 requires Per-Monitor V2 DPI awareness for crisp rendering. But the plugin runs inside Notepad++'s process, inheriting its DPI awareness level. If there is a mismatch:
1. WebView2 content appears blurry on high-DPI displays (renders at 96 DPI then upscaled).
2. On multi-monitor setups with different DPI, moving the Notepad++ window causes the WebView2 content to render at the wrong scale.
3. Initialization can fail entirely with "Initialization failed due to a mismatch in DPI awareness (0x8007139F)".

**Why it happens:**
DPI awareness is per-process on Windows, and the plugin cannot change it. WebView2 expects PerMonitorV2 but the host process may be System DPI aware or unaware.

**How to avoid:**
- Check Notepad++'s DPI awareness level at plugin load and log it.
- Use `SetThreadDpiAwarenessContext()` (Windows 10 1607+) to temporarily set Per-Monitor V2 on the thread that creates the WebView2, if the process-level awareness differs.
- Test on 100%, 125%, 150%, and 200% scaling, and on multi-monitor setups with mixed scaling.
- Set CSS `zoom` in the WebView2 content to compensate if native DPI handling is insufficient.

**Warning signs:**
- Preview text looks blurry on high-DPI laptops
- Preview renders at wrong size when Notepad++ is moved between monitors
- WebView2 initialization fails on some machines but not others

**Phase to address:**
Phase 1 (Foundation) -- DPI handling must be part of the WebView2 initialization code from the start.

---

### Pitfall 6: Preview Flicker and Content Flash on Re-render

**What goes wrong:**
On each text change, the plugin re-renders the entire markdown document and pushes new HTML to WebView2. This causes:
1. The preview flashes white/blank momentarily during each update.
2. Images reload and flicker (especially external images).
3. Scroll position resets to the top on each re-render.
4. CSS transitions/animations restart.

This makes the preview unusable for real-time editing -- the constant flashing is distracting and the scroll position loss means the user cannot see what they are typing.

**Why it happens:**
Full-page replacement (`innerHTML = newContent` or repeated `NavigateToString`) is the simplest implementation but destroys the entire DOM tree on every update.

**How to avoid:**
- Use incremental DOM updates: parse the new markdown to HTML, then use a DOM diffing approach (morphdom, or a custom diff) to patch only changed elements.
- Preserve scroll position by saving it before update and restoring after, or better, by using element-based scroll anchoring.
- Cache images: use `data-src` with lazy loading so already-loaded images are not re-fetched.
- Use CSS `contain: content` on the preview container to limit repaint scope.
- Never call `NavigateToString()` or `Navigate()` for content updates after initial page load -- always use `ExecuteScriptAsync()` to update the existing DOM.

**Warning signs:**
- White flash visible between keystrokes
- Images blink when typing anywhere in the document
- Scroll position jumps to top on each keystroke

**Phase to address:**
Phase 2 (Live Rendering) -- this is the core challenge of the live preview feature. The initial Phase 1 can use full replacement for proof-of-concept, but Phase 2 must implement incremental updates.

---

### Pitfall 7: Dual Scintilla View Architecture

**What goes wrong:**
Notepad++ has two editor views (View 1 and View 2), each backed by a different Scintilla instance. The plugin must:
1. Track which view is active and which contains the current markdown file.
2. Handle the case where the same file is open in both views (clone mode).
3. Handle the case where different files are open and the user switches between them.

Plugins that hardcode a reference to a single Scintilla HWND break when the user uses the second view, resulting in the preview showing content from the wrong file or not updating at all.

**Why it happens:**
Most plugin tutorials show getting a single Scintilla handle. The dual-view architecture is not emphasized in basic documentation.

**How to avoid:**
- Always use `NPPM_GETCURRENTSCINTILLA` to determine which Scintilla instance is active before reading content.
- Listen for `NPPN_BUFFERACTIVATED` and `NPPN_LANGCHANGED` to detect when the user switches files or views.
- Store the current buffer ID (not just the Scintilla handle) and re-fetch content when the buffer changes.
- Handle `NPPN_DOCORDERCHANGED` for when documents are moved between views.

**Warning signs:**
- Preview shows wrong file content when using split view
- Preview stops updating when file is moved to the other view
- Preview crashes or shows stale content in clone mode

**Phase to address:**
Phase 1 (Foundation) -- the notification handler must correctly identify the active buffer from the start.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Using NavigateToString for all updates | Simplest API, works immediately | 2MB limit, null origin, full page flash, scroll reset | Never in production -- only for a 1-day prototype |
| Percentage-based scroll sync | Works in 2 hours | Unusable on real documents, generates bug reports | Phase 1 prototype only, must replace in Phase 2 |
| Synchronous Scintilla text reads on UI thread | No threading complexity | Freezes editor on large files (100K+ lines) | Acceptable if combined with debounce; async read needed only for very large files |
| Bundling WebView2 fixed-version runtime | Guarantees version compatibility | 150-300MB distribution size, must manually update for security patches | Never -- use evergreen runtime with graceful absence handling |
| Single .cpp file for entire plugin | Fast to get started | Unmaintainable past 1000 lines | Phase 1 only, refactor in Phase 2 |
| Hardcoding CSS themes | No config system needed | Users cannot customize, every theme change requires a rebuild | Phase 1 prototype, must externalize in Phase 3 |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| Scintilla text retrieval | Using `SCI_GETTEXT` which includes null terminators and may not handle UTF-8 correctly | Use `SCI_GETCHARACTERPOINTER` for zero-copy access to the buffer, handle UTF-8 encoding explicitly |
| WebView2 initialization | Creating WebView2 in `DllMain` or `setInfo` -- too early, no message loop yet | Create WebView2 lazily on first panel show, after `NPPN_READY` has been received |
| WebView2 host communication | Using `NavigateToString` for each update | Use `PostWebMessageAsJson` / `AddWebMessageReceived` for bidirectional communication after initial page load |
| Notepad++ plugin list | Submitting without SHA-256 hash, wrong folder structure, or missing architecture builds | Follow nppPluginList submission requirements exactly: provide both x86 and x64 builds, correct JSON entry, SHA-256 hash of zip file |
| Local file references in markdown | Using `file:///` URLs which WebView2 blocks by default | Use `SetVirtualHostNameToFolderMapping` to map the document's directory to a virtual host, then rewrite image/link URLs to use that host |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Re-parsing entire document on every keystroke | Editor lag, high CPU usage | Debounce (200-300ms), and consider incremental parsing for files >1000 lines | Files >500 lines with fast typing |
| Loading full highlight.js with all languages | 1-2 second initial load, 2MB+ JS payload | Load only common languages initially, lazy-load others on demand via `hljs.registerLanguage()` | Immediately on first load |
| Mermaid diagram re-rendering on every update | Each Mermaid render takes 100-500ms, visible stutter | Cache rendered Mermaid SVGs by source hash, only re-render when the diagram source changes | Any document with 2+ Mermaid diagrams |
| KaTeX/MathJax rendering all math on every update | Slow re-render, flash of unstyled math | Diff which math blocks changed, only re-render those | Documents with >10 math blocks |
| WebView2 process accumulation | Memory grows over session, eventually GB of RAM | Reuse a single WebView2 instance, never create/destroy on file switch | After opening/closing 20+ files in a session |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Rendering untrusted markdown with unrestricted JS execution | XSS via crafted markdown (e.g., `<script>` tags, `javascript:` URLs, event handlers in HTML) | Use a markdown parser with HTML sanitization enabled (e.g., markdown-it with `html: false` or a sanitizer plugin). Even in a local preview, malicious markdown from downloaded repos can execute arbitrary JS in the WebView2 context |
| WebView2 with full host object access | Malicious scripts in markdown could call native functions via `AddHostObjectToScript` | Expose minimal host object API. Never expose file system access or process execution. Use `AddWebMessageReceived` with a strict message schema instead of full host objects |
| Loading external resources without user consent | Privacy leak -- opening a markdown file triggers network requests to external servers (tracking pixels, CDN resources) | Block external requests by default via `WebResourceRequested` filter. Add an opt-in setting for loading external images |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Preview panel steals keyboard focus | User types in editor, keystrokes go to WebView2 instead. Extremely frustrating | Ensure WebView2 never takes focus. Handle `GotFocus` events on the WebView2 to immediately return focus to Scintilla. Use `MoveFocusRequested` event |
| Preview opens for non-markdown files | Panel appears when opening .txt, .json, etc. Wastes screen space | Only auto-open for files with .md/.markdown extensions. Check `NPPM_GETEXTPART` on `NPPN_BUFFERACTIVATED`. Provide manual toggle for other files |
| No graceful degradation when WebView2 runtime is missing | Plugin crashes or shows cryptic error | Detect WebView2 runtime availability at startup. Show a clear dialog: "WebView2 Runtime required. [Download] [Dismiss]". Do not crash |
| Theme mismatch with Notepad++ dark mode | Bright white preview panel next to dark editor. Jarring | Detect Notepad++ dark mode via `NPPM_ISDARKMODEENABLED` (v8.4.1+) and apply matching preview theme automatically |
| Preview panel has no loading state | Large files show blank panel for 1-2 seconds during render | Show a subtle loading indicator or keep the previous content visible until the new render is ready |

## "Looks Done But Isn't" Checklist

- [ ] **Scroll sync:** Works with pure text but breaks with images, code blocks, tables, and Mermaid diagrams -- test with a real-world README.md
- [ ] **Unicode support:** ASCII markdown renders fine, but CJK characters, emoji, RTL text, and combining characters may break line mapping or display
- [ ] **File switch handling:** Preview updates when clicking in the editor, but does not update when switching files via the tab bar -- verify `NPPN_BUFFERACTIVATED` is handled
- [ ] **WebView2 cleanup:** Preview works during the session, but Notepad++ hangs on exit because WebView2 was not properly closed/released -- verify `NPPN_SHUTDOWN` handler calls `Close()` on the WebView2 controller
- [ ] **Large file graceful degradation:** Preview works on 100-line files, but test with 10,000-line files to verify debounce and performance
- [ ] **Multiple Notepad++ instances:** WebView2 user data folder locking when two Notepad++ instances run simultaneously
- [ ] **Plugin update path:** Plugin works on fresh install, but updating via Plugin Admin while Notepad++ is running can fail because the DLL is locked
- [ ] **Portable Notepad++ mode:** Plugin assumes `%APPDATA%` paths, but portable mode uses relative paths -- check `NPPM_GETPLUGINSCONFIGDIR`

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| NavigateToString 2MB limit | MEDIUM | Refactor to virtual host mapping + ExecuteScriptAsync. Requires rewriting content pipeline but not the parsing logic |
| Percentage-based scroll sync | HIGH | Must add data-source-line annotations to parser output, build line mapping infrastructure, rewrite scroll handler. Affects both C++ and JS sides |
| Wrong user data folder location | LOW | Change one string constant and test. May need to handle migration of existing user data |
| Preview flicker / full replacement | MEDIUM | Add DOM diffing library (morphdom) to JS side, change update function from innerHTML replacement to morphdom patch |
| Focus stealing | LOW | Add a single focus redirect handler. Quick fix once identified |
| Missing dual-view support | MEDIUM | Must refactor notification handler to track buffer IDs. Medium because it affects the core event loop |
| DPI mismatch | MEDIUM | Add thread DPI context switching around WebView2 creation. May require testing matrix across DPI configurations |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| NavigateToString 2MB limit | Phase 1 (Foundation) | Render a 5MB markdown file without errors |
| SCN_MODIFIED notification handling | Phase 1 (Foundation) | Do Replace All on a 10K-line file; verify preview updates and editor does not freeze |
| User data folder permissions | Phase 1 (Foundation) | Install to Program Files, run as non-admin, verify WebView2 initializes |
| Scroll synchronization | Phase 2 (Live Preview) | Scroll through a document with mixed images, code blocks, and text; preview tracks correctly |
| DPI scaling | Phase 1 (Foundation) | Test at 100%, 150%, 200% scaling; verify crisp text |
| Preview flicker | Phase 2 (Live Preview) | Type continuously in a document; no white flashes or scroll jumps visible |
| Dual Scintilla views | Phase 1 (Foundation) | Open .md files in both views, switch between them; preview always shows active file |
| Focus stealing | Phase 1 (Foundation) | Click in preview then type; keystrokes go to editor |
| WebView2 runtime missing | Phase 1 (Foundation) | Uninstall WebView2 runtime; plugin shows helpful error, does not crash |
| Security / XSS | Phase 2 (Live Preview) | Open a markdown file containing `<script>alert(1)</script>`; verify it is sanitized |
| Theme mismatch | Phase 3 (Polish) | Enable Notepad++ dark mode; preview automatically matches |
| Export to HTML/PDF | Phase 3 (Polish) | Export a file with images, math, and Mermaid; verify all render correctly in standalone output |

## Sources

- [Notepad++ Plugin Communication](https://npp-user-manual.org/docs/plugin-communication/) -- official notification reference
- [NPPM_ADDSCNMODIFIEDFLAGS API](https://community.notepad-plus-plus.org/topic/26595/new-api-to-fix-eventual-regression-regarding-scn_modified-for-some-plugins) -- v8.7.7 notification fix
- [NPPN_GLOBALMODIFIED notification](https://community.notepad-plus-plus.org/topic/25504/new-nppn_globalmodified-notification) -- Replace All event handling
- [WebView2 NavigateToString 2MB limit](https://weblog.west-wind.com/posts/2024/Jul/22/Work-around-the-WebView2-NavigateToString-2mb-Size-Limit) -- Rick Strahl's detailed workaround
- [WebView2 local content approaches](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/working-with-local-content) -- Microsoft official docs
- [WebView2 user data folder issues](https://github.com/MicrosoftEdge/WebView2Feedback/issues/901) -- community discussion on folder pitfalls
- [WebView2 DPI scaling issues](https://github.com/MicrosoftEdge/WebView2Feedback/issues/1700) -- high DPI rendering problems
- [WebView2 performance best practices](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/performance) -- Microsoft official guidance
- [WebView2 security guidance](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/security) -- secure development practices
- [VS Code markdown scroll sync bugs](https://github.com/microsoft/vscode/pull/251228) -- real-world scroll sync issues
- [Scroll sync implementation approaches](https://dev.to/woai3c/implementing-synchronous-scrolling-in-a-dual-pane-markdown-editor-5d75) -- detailed comparison of approaches
- [NppMarkdownPanel](https://github.com/mohzy83/NppMarkdownPanel) -- existing Notepad++ markdown plugin (reference implementation)
- [WebView2 memory issues](https://github.com/MicrosoftEdge/WebView2Feedback/issues/3678) -- memory leak reports

---
*Pitfalls research for: Notepad++ Markdown Preview Plugin*
*Researched: 2026-04-08*
