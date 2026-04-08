# Phase 1: Plugin Foundation - Context

**Gathered:** 2026-04-08
**Status:** Ready for planning

<domain>
## Phase Boundary

A working Notepad++ plugin that loads, creates a dockable panel, initializes WebView2, and can be toggled on/off. Plugin DLL loads in both 32-bit and 64-bit Notepad++. When WebView2 is missing, a clear message is shown instead of a crash.

</domain>

<decisions>
## Implementation Decisions

### Panel Docking Behavior
- **D-01:** Panel docks on the right side by default
- **D-02:** Standard Notepad++ redocking enabled — user can drag the panel to any dock location
- **D-03:** Initial panel width is 50% of the Notepad++ window
- **D-04:** Panel dock position and size persist across sessions via plugin settings

### WebView2 Missing Experience
- **D-05:** When WebView2 runtime is not installed, show a static message with a clickable download link to the Evergreen installer
- **D-06:** WebView2 availability is checked lazily — only when the panel is first opened, not at plugin startup
- **D-07:** The missing-WebView2 message renders inside the docking panel area (Win32 static content), not as a modal dialog

### Toggle & Shortcut Design
- **D-08:** Default keyboard shortcut is Ctrl+Shift+M to toggle the preview panel
- **D-09:** Toggle command appears under Plugins > MarkdownPreview > Toggle Preview (standard plugin menu location only)
- **D-10:** In Phase 1, the panel requires manual toggle only — no auto-open on .md file open (that's Phase 2 / REND-01)
- **D-11:** When WebView2 loads successfully but no markdown content is available, show a simple welcome page with the plugin name, version, and a hint ("Open a .md file to see the preview")

### Plugin Identity & Scaffolding
- **D-12:** Plugin display name is "MarkdownPreview" (no spaces, no Npp prefix)
- **D-13:** Use the NppCppMSVS VS2022 template as the project scaffolding — provides Scintilla interface, docking dialog examples, and settings mechanism
- **D-14:** Plugin settings stored as JSON in %AppData%/Notepad++/plugins/config/MarkdownPreview.json

### Claude's Discretion
- Exact welcome page HTML/styling
- WebView2 user data folder location
- Internal error handling patterns
- Build configuration details (debug/release, warning levels)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

No external specs — requirements fully captured in decisions above.

Phase requirements from REQUIREMENTS.md: INFR-01, INFR-02, INFR-03.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- No existing code — greenfield project. NppCppMSVS template provides the starting scaffolding.

### Established Patterns
- No patterns established yet. Phase 1 will set the foundational patterns for all subsequent phases.

### Integration Points
- Notepad++ Plugin API: DLL exports (isUnicode, getFuncsArray, getName, etc.)
- Scintilla notifications: Will be needed in Phase 2 but plugin must register for them
- WebView2 SDK: NuGet package, WebView2Loader.dll bundling
- nlohmann/json: For settings persistence (via vcpkg or header-only include)

</code_context>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches. Follow NppCppMSVS template conventions where possible.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 01-plugin-foundation*
*Context gathered: 2026-04-08*
