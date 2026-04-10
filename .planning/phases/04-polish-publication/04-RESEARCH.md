# Phase 4: Polish & Publication - Research

**Researched:** 2026-04-10
**Domain:** WebView2/JS bidirectional navigation, TOC sidebar CSS, Win32 DLL versioning, Notepad++ plugin list packaging
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Click-to-Editor Navigation (SCRL-02)**
- D-01: Any click on any element fires `postMessage({type: "lineClick", line: N})` using the nearest ancestor's `data-line` value. Click listener on `#preview` (or `document`).
- D-02: Silent caret move only — no flash, no highlight, no animation.
- D-03: Editor window receives focus (`SetFocus()` on the Scintilla HWND) after navigation.
- D-04: C++ handles `lineClick` in existing `WebMessageReceived` handler. Uses `SCI_GOTOPOS`/`SCI_GOTOLINE` + `SCI_ENSUREVISIBLE` + `SCI_SCROLLCARET`. Then calls `SetFocus(hSci)`.

**Table of Contents (SCRL-03)**
- D-05: Fixed left sidebar (~200px wide) within the WebView2 area. Always visible when headings exist. No toggle button.
- D-06: Sidebar hidden (`display: none`) when document has no h1-h6 elements. Full width content in that case. Re-evaluated on every render.
- D-07: TOC generated in JS from rendered heading elements after each `renderMarkdown()` call, using `data-line` attributes on headings.
- D-08: TOC entry clicks navigate both preview (smooth scroll) AND editor (same `lineClick` mechanism). All navigation uses same editor-jump behavior.
- D-09: Active heading tracking — as editor caret moves (`scroll` message from C++), TOC highlights nearest heading above current viewport. CSS class `toc-active`. Uses existing `scrollToLine` infrastructure.

**Installer & Packaging (INFR-04)**
- D-10: Two .zip files: `MarkdownPreview_v1.0.0_x64.zip` and `MarkdownPreview_v1.0.0_x86.zip`. Each contains: `MarkdownPreview.dll` + `assets/` directory. This is exactly what the Notepad++ plugin list requires.
- D-11: Semantic versioning `v1.0.0`. Embedded in DLL's `VS_VERSION_INFO` resource (FILEVERSION and PRODUCTVERSION). Also in `manifest.json` shipped alongside zip for plugin list metadata.
- D-12: `scripts/package.ps1` PowerShell script automates zip creation from Release build outputs. Repeatable, no manual steps.

### Claude's Discretion

- TOC sidebar exact CSS styling (font size, indent per heading level, scroll behavior)
- Active heading highlight CSS style (color, bold, indicator)
- TOC heading truncation if text is very long
- `lineClick` message field names (e.g., `line` vs `lineNumber`)
- Scintilla navigation message sequence (SCI_GOTOLINE vs SCI_GOTOPOS + SCI_ENSUREVISIBLE combination)
- `manifest.json` schema (match Notepad++ plugin list format if documented)
- Scripts directory location (`scripts/` at repo root)

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within Phase 4 scope.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SCRL-02 | Clicking in the preview scrolls the editor to the corresponding source line | D-01 through D-04 in CONTEXT.md; existing `data-line` infrastructure and `WebMessageReceived` handler provide the integration points |
| SCRL-03 | Clickable table of contents generated from document headings | D-05 through D-09 in CONTEXT.md; pure DOM/JS using existing `data-line` attributes on headings; flex layout sidebar pattern |
| INFR-04 | Publish-ready quality — proper installer, versioning, plugin list compatible | Notepad++ plugin list requires SHA-256 id of zip, `VS_VERSION_INFO` DLL resource, x86+x64 zips with DLL at root; PowerShell 5.1 confirmed available |
</phase_requirements>

---

## Summary

Phase 4 completes three distinct workstreams: reverse scroll sync (JS clicks navigate the editor), a table of contents sidebar, and publish-ready packaging.

**Click navigation (SCRL-02)** is the reverse of the editor-to-preview scroll sync already implemented in Phase 2/3. The `data-line` attributes are already on all block elements. A JS click event listener reads the nearest ancestor's `data-line` value and posts `{type: "lineClick", line: N}` to C++. On the C++ side, the `handleJsMessage()` switch statement in `PreviewPanel.cpp` adds a `lineClick` case that calls `SCI_GOTOLINE` + `SCI_ENSUREVISIBLE` on the Scintilla handle, then calls `SetFocus(hSci)`. The pattern is identical to how the existing `exportReady` message is handled.

**TOC sidebar (SCRL-03)** is pure DOM/JS added to `preview.html`. A `buildToc()` function runs after each `renderMarkdown()` call, queries all heading elements (`h1`-`h6`), reads their `data-line` attributes, and populates a `<nav id="toc">` sidebar. Layout uses CSS flexbox on `body` or a wrapper element. Active heading tracking wires into the existing `scroll` message already received from C++. No new C++ code is needed for the TOC beyond what SCRL-02 already adds.

**Packaging (INFR-04)** requires three artifacts: (1) a `.rc` file added to the vcxproj that embeds `VS_VERSION_INFO` in the DLL, (2) a `scripts/package.ps1` that copies x86/x64 Release DLLs + assets into the correct zip structure and runs `Get-FileHash` for the SHA-256 id, and (3) a `manifest.json` containing the Notepad++ plugin list JSON entry fields. The zip structure is already clear from the official docs: DLL at root, `assets/` as a subfolder.

**Primary recommendation:** Implement in three sequential plans — Plan 1 (SCRL-02 click navigation), Plan 2 (SCRL-03 TOC sidebar), Plan 3 (INFR-04 versioning + packaging). Each plan is independently testable.

---

## Project Constraints (from CLAUDE.md)

- C++ DLL only — no .NET, no CEF, no external Markdown parser
- MSVC v145 toolset (VS 2026 on dev machine, not v143)
- WebView2 SDK 1.0.3856.49 via NuGet
- No new JS libraries — TOC and click nav use pure DOM with existing `data-line` attributes
- No new C++ dependencies — navigation uses `SCI_GOTOLINE` via `SendMessage` already used in codebase
- PowerShell 5.1 for `package.ps1` (ships with Windows 10/11)
- Security: `html: false` in markdown-it (XSS prevention already in place)
- Build outputs: `bin\x86\Release\` and `bin\x64\Release\` (confirmed from vcxproj)

---

## Standard Stack

### Core (No New Dependencies)
| Component | Version | Purpose | Why Standard |
|-----------|---------|---------|--------------|
| Pure DOM/JS | — | TOC generation + click listener | No new library needed; `data-line` attrs already on all block elements |
| Scintilla `SCI_GOTOLINE` | (Notepad++ built-in) | Navigate editor to line | Standard Scintilla message; already used via `SendMessage` pattern in codebase |
| PowerShell 5.1 | 5.1.26100.7920 (confirmed) | `package.ps1` zip packaging script | Ships with Windows 10/11; no install needed |
| Win32 VERSIONINFO resource | Windows SDK | DLL version embedding | Standard RC resource; required for Notepad++ plugin list version comparison |

### No Alternatives to Consider
Per CONTEXT.md locked decisions: no new C++ libraries, no new JS libraries. Every component in Phase 4 reuses existing infrastructure.

**Installation:** No new packages. All implementation is additive to existing files.

---

## Architecture Patterns

### Pattern 1: JS Click Listener → C++ Scintilla Navigation

**What:** A delegated event listener on `#preview` (or `document`) captures all clicks. It walks up the DOM from the click target to find the nearest ancestor with a `data-line` attribute, then posts a `lineClick` message to C++.

**When to use:** SCRL-02. Handles clicks on headings, paragraphs, code blocks, tables, list items — any element with `data-line`.

**Example (JS in preview.html):**
```javascript
// Source: CONTEXT.md D-01, mirrors exportReady pattern in existing preview.html
document.addEventListener('click', function(evt) {
    var el = evt.target;
    while (el && el !== document.body) {
        var lineAttr = el.getAttribute('data-line');
        if (lineAttr !== null) {
            window.chrome.webview.postMessage(
                JSON.stringify({ type: 'lineClick', line: parseInt(lineAttr, 10) })
            );
            break;
        }
        el = el.parentElement;
    }
});
```

**Wire into existing `switch` in `handleJsMessage()` (C++):**
```cpp
// Source: mirrors existing exportReady case in PreviewPanel.cpp handleJsMessage()
if (type == "lineClick") {
    int line = j.value("line", -1);
    if (line >= 0) {
        navigateEditorToLine(line);
    }
}
```

**New C++ helper `navigateEditorToLine(int line)`:**
```cpp
// Source: Scintilla docs — SCI_GOTOLINE is 0-indexed, moves caret to line start
// SCI_ENSUREVISIBLE expands any collapsed folds, SCI_SCROLLCARET ensures line is in view
void PreviewPanel::navigateEditorToLine(int line) {
    int sciId = 0;
    ::SendMessage(m_nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&sciId));
    HWND hSci = (sciId == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    if (!hSci) return;
    ::SendMessage(hSci, SCI_ENSUREVISIBLE, static_cast<WPARAM>(line), 0);
    ::SendMessage(hSci, SCI_GOTOLINE,      static_cast<WPARAM>(line), 0);
    ::SendMessage(hSci, SCI_SCROLLCARET,   0, 0);
    ::SetFocus(hSci);
}
```

**Key detail:** Scintilla lines are 0-indexed. `data-line` is set from `token.map[0]` (0-indexed) in the existing source-map plugin. The values match directly — no offset conversion needed. [VERIFIED: existing preview.html source_map rule + Scintilla documentation]

---

### Pattern 2: TOC Sidebar — Flex Layout + Post-Render Hook

**What:** `buildToc()` is called at the end of `renderMarkdown()`. It collects h1-h6 elements from `#preview`, reads their `data-line` and `textContent`, and populates `<nav id="toc">`. CSS flexbox makes the TOC a fixed-width left column with `#preview` taking the remaining space.

**When to use:** SCRL-03. Runs on every render. Shows/hides based on whether headings exist (D-06).

**HTML structure (added to preview.html `<body>`):**
```html
<!-- Outer flex wrapper replaces the current direct #preview at body root -->
<div id="layout">
    <nav id="toc" style="display:none"></nav>
    <div id="preview" class="markdown-body" style="display:none"></div>
</div>
```

**CSS:**
```css
/* Source: CONTEXT.md D-05, D-06, Specifics section */
#layout {
    display: flex;
    height: 100%;
}
#toc {
    width: 200px;
    min-width: 200px;
    overflow-y: auto;
    padding: var(--space-md);
    font-size: 13px;
    border-right: 1px solid #d1d9e0;
    flex-shrink: 0;
}
body[data-theme="dark"] #toc {
    border-right-color: #3d444d;
    color: #9198a1;
}
#preview {
    flex: 1;
    overflow-y: auto;
    min-width: 0;  /* prevents flex blowout on wide pre blocks */
}
.toc-entry { display: block; padding: 2px 0; cursor: pointer; text-decoration: none; color: inherit; }
.toc-entry:hover { text-decoration: underline; }
.toc-active { font-weight: 600; color: #0969da; }
body[data-theme="dark"] .toc-active { color: #58a6ff; }
/* Indent per heading level */
.toc-h1 { padding-left: 0; }
.toc-h2 { padding-left: 12px; }
.toc-h3 { padding-left: 24px; }
.toc-h4 { padding-left: 36px; }
.toc-h5 { padding-left: 48px; }
.toc-h6 { padding-left: 60px; }
```

**`buildToc()` function:**
```javascript
// Source: CONTEXT.md D-07, D-08, D-09 — pure DOM, no library
function buildToc() {
    var toc = document.getElementById('toc');
    var headings = document.querySelectorAll('#preview h1,#preview h2,#preview h3,#preview h4,#preview h5,#preview h6');
    if (!headings.length) {
        toc.style.display = 'none';
        return;
    }
    toc.style.display = 'block';
    toc.innerHTML = '';
    headings.forEach(function(h) {
        var line = parseInt(h.getAttribute('data-line') || '-1', 10);
        var text = h.textContent || '';
        // Truncate long headings at 30 chars
        var label = text.length > 30 ? text.slice(0, 28) + '…' : text;
        var a = document.createElement('a');
        a.className = 'toc-entry toc-' + h.tagName.toLowerCase();
        a.textContent = label;
        a.title = text;  // full text on hover
        a.setAttribute('data-line', String(line));
        a.addEventListener('click', function(e) {
            e.preventDefault();
            // Navigate preview (smooth scroll)
            h.scrollIntoView({ behavior: 'smooth', block: 'start' });
            // Navigate editor (same lineClick mechanism — D-08)
            if (line >= 0) {
                window.chrome.webview.postMessage(
                    JSON.stringify({ type: 'lineClick', line: line })
                );
            }
        });
        toc.appendChild(a);
    });
}
```

**Active heading update (triggered by existing `scroll` message from C++):**
```javascript
// Source: CONTEXT.md D-09 — called in existing 'scroll' case of message dispatcher
function updateTocActive(line) {
    var entries = document.querySelectorAll('#toc .toc-entry');
    var best = null;
    entries.forEach(function(entry) {
        var entryLine = parseInt(entry.getAttribute('data-line') || '-1', 10);
        if (entryLine >= 0 && entryLine <= line) {
            best = entry;
        }
    });
    entries.forEach(function(e) { e.classList.remove('toc-active'); });
    if (best) { best.classList.add('toc-active'); }
}
```

**Integration into `renderMarkdown()`:** Call `buildToc()` after `injectCopyButtons()`. Integration into `scrollToLine()`: call `updateTocActive(line)` at the end. Integration into the `scroll` case in the message dispatcher: add `updateTocActive(msg.line || 0)`.

---

### Pattern 3: DLL Version Resource (.rc file)

**What:** A Windows VERSIONINFO resource embedded in the DLL at compile time. Required for Notepad++ Plugin Admin to compare installed vs available plugin versions.

**File:** `MarkdownPreview/MarkdownPreview.rc` (new file, added to vcxproj)

```rc
// Source: learn.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource
#include <winver.h>

VS_VERSION_INFO VERSIONINFO
FILEVERSION     1,0,0,0
PRODUCTVERSION  1,0,0,0
FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
FILEFLAGS       0
FILEOS          VOS_NT_WINDOWS32
FILETYPE        VFT_DLL
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904B0"
        BEGIN
            VALUE "CompanyName",      "MarkdownPreview Contributors\0"
            VALUE "FileDescription",  "Live Markdown Preview Panel for Notepad++\0"
            VALUE "FileVersion",      "1.0.0.0\0"
            VALUE "InternalName",     "MarkdownPreview\0"
            VALUE "OriginalFilename", "MarkdownPreview.dll\0"
            VALUE "ProductName",      "MarkdownPreview\0"
            VALUE "ProductVersion",   "1.0.0\0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0409, 1200
    END
END
```

**vcxproj change:** Add `<ResourceCompile Include="MarkdownPreview.rc" />` in the `<ItemGroup>` with `.cpp` files. MSVC automatically invokes `rc.exe` on `.rc` files listed as `ResourceCompile` items.

**Charset note:** `040904B0` = language 0x0409 (US English) + charset 0x04B0 (1200 = Unicode). This is the correct block ID for Unicode DLLs. [CITED: learn.microsoft.com/windows/win32/menurc/versioninfo-resource]

---

### Pattern 4: Notepad++ Plugin List ZIP Structure

**What:** The official Plugin Admin installs from a zip where the plugin DLL is at the zip root (not inside a subdirectory). Subfolders (like `assets/`) are preserved relative to root and land in the plugin's install directory.

**Required zip layout:**
```
MarkdownPreview_v1.0.0_x64.zip
├── MarkdownPreview.dll         ← must be at root
├── WebView2Loader.dll          ← also at root (required at runtime)
└── assets/
    ├── preview.html
    ├── welcome.html
    ├── markdown-it.min.js
    ├── ... (all bundled JS/CSS)
    └── fonts/
        └── KaTeX_*.woff2
```

**Notepad++ install result:**
```
%APPDATA%\Notepad++\plugins\MarkdownPreview\
├── MarkdownPreview.dll
├── WebView2Loader.dll
└── assets\
    └── ...
```

**Plugin list JSON entry (for `pl.x64.json` and `pl.x86.json` PR):**
```json
{
  "folder-name": "MarkdownPreview",
  "display-name": "MarkdownPreview",
  "version": "1.0.0",
  "id": "<SHA-256 of zip file — computed by package.ps1>",
  "repository": "https://github.com/.../releases/download/v1.0.0/MarkdownPreview_v1.0.0_x64.zip",
  "description": "Live Markdown preview panel with scroll sync, TOC, math, diagrams, and export",
  "author": "Henk",
  "homepage": "https://github.com/.../MarkdownPreview"
}
```

**SHA-256 id:** The `id` field is the SHA-256 fingerprint of the **zip file**, not the DLL. Computed with `Get-FileHash` in PowerShell. [CITED: npp-user-manual.org/docs/plugins/#plugins-admin]

---

### Pattern 5: `scripts/package.ps1` Automation

**What:** Repeatable PowerShell script that assembles the release zips from build outputs.

```powershell
# Source: PowerShell 5.1 Compress-Archive cmdlet (confirmed available on dev machine)
# CONTEXT.md D-12

param([string]$Version = "1.0.0")

$root    = Split-Path $PSScriptRoot -Parent
$binX64  = Join-Path $root "bin\x64\Release"
$binX86  = Join-Path $root "bin\x86\Release"
$distDir = Join-Path $root "dist"

# Create dist directory
New-Item -ItemType Directory -Force -Path $distDir | Out-Null

foreach ($arch in @("x64","x86")) {
    $binDir  = if ($arch -eq "x64") { $binX64 } else { $binX86 }
    $zipName = "MarkdownPreview_v${Version}_${arch}.zip"
    $zipPath = Join-Path $distDir $zipName

    # Stage files into a temp dir to control zip root structure
    $stage = Join-Path $env:TEMP "MdPreview_stage_$arch"
    if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $stage | Out-Null

    # Copy DLL(s) to stage root
    Copy-Item (Join-Path $binDir "MarkdownPreview.dll") $stage
    Copy-Item (Join-Path $binDir "WebView2Loader.dll")  $stage

    # Copy assets subtree preserving directory structure
    $assetsStage = Join-Path $stage "assets"
    Copy-Item (Join-Path $binDir "assets") $assetsStage -Recurse

    # Create zip from stage root contents (not the stage folder itself)
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zipPath

    # Compute SHA-256 for plugin list id field
    $hash = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToLower()
    Write-Host "${arch}: $zipPath"
    Write-Host "  SHA-256: $hash"

    # Cleanup stage
    Remove-Item $stage -Recurse -Force
}
```

**Critical:** `Compress-Archive -Path "$stage\*"` (with wildcard `*`) puts DLL at the zip root — NOT in a subdirectory named after the stage folder. If you pass the directory path without `*`, PowerShell wraps everything in a subdirectory, which breaks Plugin Admin installation. [VERIFIED: PowerShell Compress-Archive documentation]

---

### Anti-Patterns to Avoid

- **TOC listener on individual `<a>` elements vs delegation:** TOC is rebuilt on every render — attaching listeners per-entry is correct but only safe inside `buildToc()` which recreates them each time. Using `innerHTML` and re-creating is fine; do not attempt to diff/patch the TOC across renders.
- **Scroll container confusion:** With `#layout` flex + `#preview` scroll, `scrollIntoView()` works because `#preview` is the scroll container, not `<body>`. Ensure `body` and `html` have `height: 100%` and `overflow: hidden` so `#preview` gets `overflow-y: auto`. The existing CSS already sets `html, body { height: 100% }` but `body` currently has no `overflow: hidden`.
- **Blocking click events on TOC itself:** If the document-level click listener is used for `lineClick`, TOC clicks will also trigger it. The TOC entries carry `data-line`, so the TOC entry click will fire both the TOC handler and the document click handler. Prevent double-posting by calling `e.stopPropagation()` in the TOC entry click handler, OR by only attaching the document listener to `#preview` (not `document`).
- **Zip structure with nested directory:** `Compress-Archive -Path $stagePath` (without `*`) creates a zip with the stage folder name as the root — Plugin Admin will not find the DLL. Always use `$stagePath\*` or stage to a directory named `MarkdownPreview` and zip that.
- **Version mismatch between .rc and manifest.json:** Both must agree on `1.0.0` / `1,0,0,0`. Notepad++ Plugin Admin compares the DLL's binary `FILEVERSION` resource against the `version` string in the JSON entry. A mismatch will cause the plugin to appear as needing an update immediately after install.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| SHA-256 of zip | Custom hash function | `Get-FileHash -Algorithm SHA256` (PowerShell 5.1 built-in) | Native, exact format Plugin Admin expects |
| Zip file creation | Manual zip API | `Compress-Archive` (PowerShell 5.1 built-in) | Ships with Windows; no extra tools |
| DLL version info | Parse DLL headers | `VS_VERSION_INFO` resource in `.rc` file | Standard Windows mechanism; MSVC compiles it automatically via `rc.exe` |
| TOC smooth scroll | JS scroll polyfill | Native `scrollIntoView({ behavior: 'smooth' })` | WebView2 is Chromium — full ES2022 support; polyfills unnecessary |

---

## Common Pitfalls

### Pitfall 1: Click Handler on `#preview` Fires During TOC Clicks
**What goes wrong:** If the document-level click listener (for `lineClick`) is on `document` and TOC entries also have `data-line` attributes, a TOC click fires the TOC handler AND the document click handler, sending two `lineClick` messages.
**Why it happens:** Event bubbling — click on `<a>` inside `<nav id="toc">` bubbles through TOC, then layout, then document.
**How to avoid:** Either (a) attach the document click listener only to `#preview` (not `document` or `body`), or (b) call `evt.stopPropagation()` in each TOC entry's click handler.
**Warning signs:** Editor receives two `SCI_GOTOLINE` calls in rapid succession on TOC click.

### Pitfall 2: `overflow: hidden` Missing on `body` Breaks Scroll Container
**What goes wrong:** With flexbox layout, if `body` is not `overflow: hidden`, scrolling `#preview` causes the whole-page scrollbar to appear instead of just the `#preview` area, and `scrollToLine()` calls affect the page scroll position, not the preview scroll.
**Why it happens:** `scrollIntoView()` scrolls the nearest scrollable ancestor. If `body` scrolls before `#preview`, `body` is the scroll container.
**How to avoid:** Add `overflow: hidden` to `body` in `preview.html` CSS. `#preview` already has `overflow-y: auto`.

### Pitfall 3: `idle-state` Full-Height Breaks with Flex Layout
**What goes wrong:** The current `#idle-state` uses `height: 100vh` for centering. With `#layout` as a flex container, `#idle-state` is not inside `#layout`, so it still occupies the full viewport — but it must be visually excluded from the flex layout.
**Why it happens:** `#idle-state` and `#layout` are siblings. If both have `height: 100vh` or similar, they stack.
**How to avoid:** Keep `#idle-state` outside `#layout` and apply `position: absolute; top: 0; left: 0; width: 100%; height: 100%` so it overlays `#layout` when visible. Or simply keep existing `height: 100vh` — it works when `#idle-state` is the only visible element.

### Pitfall 4: vcxproj Doesn't Compile .rc Without Explicit `<ResourceCompile>` Item
**What goes wrong:** Adding `MarkdownPreview.rc` as a `<None>` or `<ClCompile>` item does not invoke `rc.exe`. The DLL is built without version info.
**Why it happens:** MSBuild only invokes the RC compiler for items in the `<ResourceCompile>` ItemGroup.
**How to avoid:** Add `<ResourceCompile Include="MarkdownPreview.rc" />` to the vcxproj ItemGroup.

### Pitfall 5: Plugin Admin Version Comparison Requires Integer FILEVERSION Match
**What goes wrong:** Plugin Admin reads the DLL's binary `FILEVERSION` (four WORD values: `1,0,0,0`) and compares to the JSON `"version": "1.0.0"`. Notepad++ uses a specific parsing rule — it splits the string on `.` and compares each segment. A mismatch causes spurious update prompts.
**Why it happens:** Binary FILEVERSION uses comma-separated four-part notation; JSON uses dotted three-part. They must match semantically.
**How to avoid:** Keep DLL FILEVERSION as `1,0,0,0` and JSON version as `"1.0.0"`. Notepad++ ignores the fourth segment. [CITED: community.notepad-plus-plus.org/topic/24433]

### Pitfall 6: `Compress-Archive` Directory Path Creates Nested Root
**What goes wrong:** `Compress-Archive -Path $stageDir -DestinationPath out.zip` puts all files inside a subdirectory named after `$stageDir` inside the zip. Plugin Admin looks for `MarkdownPreview.dll` at zip root — it won't find it.
**Why it happens:** PowerShell includes the source directory name when given a directory path.
**How to avoid:** Stage to a temp directory and use `-Path "$stageDir\*"` (wildcard) to include only contents, not the container directory. [VERIFIED: Microsoft PowerShell Compress-Archive documentation]

---

## Code Examples

### Existing Integration Points (Verified from Codebase)

#### `handleJsMessage()` current structure (PreviewPanel.cpp ~line 820):
```cpp
// Source: read from PreviewPanel.cpp — confirmed switch on type == "exportReady"
if (type == "exportReady") {
    std::string htmlContent = j.value("html", "");
    if (!htmlContent.empty()) { saveExportedHtml(htmlContent); }
}
// Add after this:
// if (type == "lineClick") { ... }
```

#### `renderMarkdown()` end hook (preview.html ~line 477):
```javascript
// Source: read from preview.html — confirmed call sequence
renderMermaidDiagrams();
injectCopyButtons();
// Add after injectCopyButtons():
// buildToc();
```

#### Message dispatcher `scroll` case (preview.html ~line 622):
```javascript
// Source: read from preview.html
case 'scroll':
    scrollToLine(msg.line || 0);
    // Add:
    // updateTocActive(msg.line || 0);
    break;
```

#### Scintilla handle retrieval pattern (reuse from getCurrentText()):
```cpp
// Source: read from PreviewPanel.cpp getCurrentText() ~line 475
// Same pattern used for navigateEditorToLine()
int sciId = 0;
::SendMessage(m_nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&sciId));
HWND hSci = (sciId == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
```

---

## Runtime State Inventory

Step 2.5: Not a rename/refactor phase. No runtime state migration needed.

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| PowerShell | `scripts/package.ps1` | Yes | 5.1.26100.7920 | — |
| Visual Studio 2022 / MSBuild | DLL build with .rc | Assumed (project builds today) | v145 toolset | — |
| `Get-FileHash` cmdlet | SHA-256 for zip | Yes (built into PS 5.1) | PS 5.1 built-in | — |
| `Compress-Archive` cmdlet | Zip creation | Yes (built into PS 5.1) | PS 5.1 built-in | — |

**Missing dependencies with no fallback:** None.

**Note on WebView2Loader.dll:** The Release build already copies `WebView2Loader.dll` to `bin\x64\Release\` and `bin\x86\Release\` via the vcxproj `<PostBuildEvent>`. The `package.ps1` script includes it in the zip alongside `MarkdownPreview.dll`. [VERIFIED: read from MarkdownPreview.vcxproj PostBuildEvent]

---

## Validation Architecture

`nyquist_validation` is explicitly `false` in `.planning/config.json` — this section is omitted.

---

## Security Domain

No new attack surfaces are introduced in Phase 4. The existing security posture is maintained:

- **lineClick message:** C++ reads the `line` integer field from JS. It uses `j.value("line", -1)` with `nlohmann::json` — type-safe, no string injection possible. The line value is passed directly to `SCI_GOTOLINE` as `WPARAM`. No path or shell execution involved.
- **TOC generation:** Uses `h.textContent` (not `innerHTML`) for heading text — no XSS vector. `data-line` attribute is set from integer-only `token.map[0]` values in the source-map plugin.
- **Package script:** Reads only from `bin\` output directories with hardcoded paths — no user input, no arbitrary file access.

No new ASVS categories apply beyond what Phases 1-3 already addressed (V5 input validation already covered by existing `html: false` and JSON parse pattern).

---

## Open Questions

1. **`WebView2Loader.dll` in the zip — required or optional?**
   - What we know: The vcxproj PostBuildEvent copies it to the Release output directory. It is the "fixed-version" loader that allows the plugin to use the Evergreen WebView2 runtime.
   - What's unclear: Does the Notepad++ Plugin Admin allow DLL files at the zip root that are not the plugin's own DLL? (The official docs say "additional files can be placed at the root level.")
   - Recommendation: Include `WebView2Loader.dll` in the zip. It is already present in both Release outputs and is required for WebView2 to initialize. The docs explicitly allow additional DLLs at the zip root.

2. **`manifest.json` exact schema**
   - What we know: Per CONTEXT.md D-11, a `manifest.json` is shipped alongside the zip for plugin list metadata. The Notepad++ plugin list uses `pl.x64.json` / `pl.x86.json` as the actual registry — `manifest.json` is a convenience artifact in the release, not a required file parsed by Plugin Admin.
   - What's unclear: Whether to include `manifest.json` inside the zip or only as a separate release asset.
   - Recommendation: Provide `manifest.json` as a repo-level file (not inside the zip) containing the plugin list JSON entry for the PR submission. The zip itself only needs DLL + assets.

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | TOC `buildToc()` calling `scrollIntoView()` on `h` (the heading element in `#preview`) will scroll `#preview` correctly, not `body` | Pattern 2 | TOC click scrolls the page instead of the preview area; fix: ensure `#preview` has `overflow-y: auto` and `body` has `overflow: hidden` |
| A2 | `data-line` attributes are present on h1-h6 heading elements from the markdown-it source_map rule | Standard Stack | TOC has no line numbers; fix: verify source_map rule applies to `heading_open` tokens |

**A2 verification note:** The source_map rule operates on `state.tokens` and sets `data-line` on any token with a `token.map` property. Heading tokens (`heading_open`) have `token.map` set by the markdown-it inline parser. This is confirmed by the existing `scrollToLine()` working correctly in Phase 2/3. [VERIFIED: read from preview.html source_map rule — `if (token.map && ...)`]

---

## Sources

### Primary (HIGH confidence)
- Read from `MarkdownPreview/assets/preview.html` — confirmed `data-line` source_map injection, `scrollToLine()`, `handleJsMessage()` pattern, existing message types
- Read from `MarkdownPreview/src/PreviewPanel.cpp` — confirmed `handleJsMessage()` switch, `scrollToLine()` C++ method, Scintilla handle retrieval pattern, PostBuildEvent for WebView2Loader.dll
- Read from `MarkdownPreview/src/PreviewPanel.h` — confirmed member variables, no existing version constants
- Read from `MarkdownPreview/MarkdownPreview.vcxproj` — confirmed build output paths (`bin\x64\Release\`, `bin\x86\Release\`), toolset v145, no existing .rc file
- [CITED: learn.microsoft.com/windows/win32/menurc/versioninfo-resource] — exact VERSIONINFO resource syntax, charset ID 04B0 (Unicode), block ID 040904B0
- [CITED: npp-user-manual.org/docs/plugins/#plugins-admin] — SHA-256 is of the zip file (not the DLL), zip structure (DLL at root), folder-name must match DLL name

### Secondary (MEDIUM confidence)
- WebSearch + WebFetch of nppPluginList README — folder-name uniqueness requirement, version comparison behavior, JSON fields required
- [CITED: community.notepad-plus-plus.org/topic/24433] — npp-compatible-versions and old-versions-compatibility fields in plugin list JSON
- PowerShell 5.1 Compress-Archive documentation (confirmed available via `powershell -NoProfile -Command '$PSVersionTable.PSVersion'`)
- Scintilla documentation (scintilla.org/ScintillaDoc.html) — SCI_GOTOLINE is 0-indexed, auto-scrolls; SCI_ENSUREVISIBLE expands folds

---

## Metadata

**Confidence breakdown:**
- SCRL-02 click navigation: HIGH — all integration points verified in existing code; Scintilla message sequence confirmed from documentation
- SCRL-03 TOC sidebar: HIGH — pure DOM/JS with no new dependencies; flex layout is standard CSS; integration hooks confirmed in preview.html
- INFR-04 packaging: HIGH — zip structure confirmed from official Notepad++ user manual; PowerShell 5.1 confirmed available; .rc file syntax confirmed from MSDN
- Plugin list submission: MEDIUM — process understood from official docs; actual PR submission requires GitHub repo hosting and a live download URL (out of scope for this phase)

**Research date:** 2026-04-10
**Valid until:** 2026-07-10 (stable domain — Scintilla API and Windows VERSIONINFO resource format do not change; Notepad++ plugin list schema is stable)
