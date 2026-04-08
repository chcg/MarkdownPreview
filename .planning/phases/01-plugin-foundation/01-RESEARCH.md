# Phase 1: Plugin Foundation - Research

**Researched:** 2026-04-08
**Domain:** Notepad++ C++ plugin development, Win32 docking panels, WebView2 initialization
**Confidence:** HIGH

## Summary

Phase 1 creates the foundational Notepad++ plugin: a C++ DLL that loads correctly in both 32-bit and 64-bit Notepad++, provides a dockable panel toggled via Ctrl+Shift+M, initializes WebView2 inside that panel, and gracefully handles missing WebView2 runtime. The NppCppMSVS VS2022 template provides scaffolding with docking dialog examples, settings JSON, and Scintilla interface -- this is the correct starting point per the locked decisions.

The core technical challenges are: (1) correctly implementing the Notepad++ plugin DLL exports and docking dialog registration, (2) initializing WebView2 asynchronously within a docked panel HWND, (3) detecting WebView2 runtime absence without crashing, and (4) building for both x86 and x64 architectures from a single solution.

**Primary recommendation:** Start from the NppCppMSVS template, strip its example dialogs, add a WebView2-hosting dockable panel, and use `GetAvailableCoreWebView2BrowserVersionString` for lazy runtime detection when the panel first opens.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Panel docks on the right side by default
- **D-02:** Standard Notepad++ redocking enabled -- user can drag the panel to any dock location
- **D-03:** Initial panel width is 50% of the Notepad++ window
- **D-04:** Panel dock position and size persist across sessions via plugin settings
- **D-05:** When WebView2 runtime is not installed, show a static message with a clickable download link to the Evergreen installer
- **D-06:** WebView2 availability is checked lazily -- only when the panel is first opened, not at plugin startup
- **D-07:** The missing-WebView2 message renders inside the docking panel area (Win32 static content), not as a modal dialog
- **D-08:** Default keyboard shortcut is Ctrl+Shift+M to toggle the preview panel
- **D-09:** Toggle command appears under Plugins > MarkdownPreview > Toggle Preview (standard plugin menu location only)
- **D-10:** In Phase 1, the panel requires manual toggle only -- no auto-open on .md file open (that's Phase 2 / REND-01)
- **D-11:** When WebView2 loads successfully but no markdown content is available, show a simple welcome page with the plugin name, version, and a hint ("Open a .md file to see the preview")
- **D-12:** Plugin display name is "MarkdownPreview" (no spaces, no Npp prefix)
- **D-13:** Use the NppCppMSVS VS2022 template as the project scaffolding
- **D-14:** Plugin settings stored as JSON in %AppData%/Notepad++/plugins/config/MarkdownPreview.json

### Claude's Discretion
- Exact welcome page HTML/styling
- WebView2 user data folder location
- Internal error handling patterns
- Build configuration details (debug/release, warning levels)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| INFR-01 | Toggle show/hide preview panel via menu item and keyboard shortcut | Docking dialog API (NPPM_DMMSHOW/HIDE), FuncItem with ShortcutKey for Ctrl+Shift+M, toggle state tracking |
| INFR-02 | Plugin loads correctly in both 32-bit and 64-bit Notepad++ | VS2022 solution with dual platform targets (x86/x64), architecture-specific WebView2Loader.dll |
| INFR-03 | Graceful handling when WebView2 runtime is not installed | GetAvailableCoreWebView2BrowserVersionString API for lazy detection, Win32 fallback UI in docking panel |
</phase_requirements>

## Project Constraints (from CLAUDE.md)

- **Platform:** Windows only (Notepad++ constraint)
- **Build system:** MSVC (Visual Studio 2022) with v143 toolset, MSBuild
- **Plugin API:** Must conform to Notepad++ plugin architecture (C++ DLL, specific exports)
- **WebView2 SDK:** Microsoft.Web.WebView2 NuGet package (1.0.3856.49 stable)
- **JSON library:** nlohmann/json 3.11.3 for settings persistence
- **Quality:** Publish-ready for Notepad++ plugin list
- **GSD Workflow:** All edits must go through GSD workflow entry points

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| MSVC v143 (VS2022) | 18.x (MSBuild 18.3) | C++ compiler and build system | Notepad++ itself uses MSVC; ABI compatibility required for docking dialogs [VERIFIED: local VS installation] |
| NppCppMSVS template | v1.0 | Plugin scaffolding with docking dialog examples | Provides VS2022 project template with Scintilla C++ interface, docking dialog examples, settings JSON mechanism [CITED: community.notepad-plus-plus.org/topic/26673] |
| Windows SDK | 10.0.22621+ | Win32 API for HWND management | Required for docking panel creation and WM_NOTIFY handling [ASSUMED] |
| Microsoft WebView2 SDK | 1.0.3856.49 (stable) | Chromium-based HTML rendering in docked panel | Ships with Windows 11, Evergreen runtime on Windows 10. WebView2 runtime 146.0.3856.109 confirmed on dev machine [VERIFIED: registry query] |
| nlohmann/json | 3.11.3 | JSON settings persistence | Header-only, widely used for C++ JSON. Used for panel position/size persistence [CITED: CLAUDE.md] |
| Windows Implementation Libraries (WIL) | Latest | COM helpers, RAII wrappers | Standard companion for WebView2 Win32 development. Provides wil::com_ptr, Callback<> helpers [CITED: learn.microsoft.com WebView2 getting started guide] |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| WebView2Loader.dll | (bundled with SDK) | WebView2 runtime loader | Must ship architecture-matched DLL with plugin [CITED: learn.microsoft.com/webview2/concepts/distribution] |

**NuGet packages to install:**
```
Microsoft.Web.WebView2 (1.0.3856.49)
Microsoft.Windows.ImplementationLibrary
```

**NppCppMSVS template download:**
```
https://github.com/Coises/NppCppMSVS/releases/download/v1.0/NppCppMSVS-1.0.zip
```
Place in `Documents\Visual Studio 2022\Templates\ProjectTemplates\` without unzipping. [CITED: community.notepad-plus-plus.org/topic/26673]

## Architecture Patterns

### Recommended Project Structure
```
MarkdownPreview/
├── MarkdownPreview.sln                    # VS2022 solution (x86 + x64 configs)
├── MarkdownPreview/
│   ├── src/
│   │   ├── PluginMain.cpp                 # DLL exports, plugin entry point
│   │   ├── PluginDefinition.cpp/.h        # Plugin logic, menu commands
│   │   ├── PreviewPanel.cpp/.h            # Docking panel + WebView2 host
│   │   └── Settings.cpp/.h               # JSON settings load/save
│   ├── include/
│   │   ├── Notepad_plus_msgs.h            # Npp message constants (from template)
│   │   ├── PluginInterface.h              # Required structures (from template)
│   │   ├── Scintilla.h                    # Scintilla interface (from template)
│   │   └── Docking.h                      # Docking constants (from template)
│   ├── assets/
│   │   └── welcome.html                   # Welcome/placeholder page
│   ├── packages.config                    # NuGet references
│   └── MarkdownPreview.vcxproj            # Project file (x86 + x64)
└── README.md
```

### Pattern 1: Plugin DLL Exports
**What:** Six required C-linkage exports that Notepad++ calls to communicate with the plugin.
**When to use:** Every Notepad++ plugin must implement these.
**Example:**
```cpp
// Source: https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/MISC/PluginsManager/PluginInterface.h
// [VERIFIED: Notepad++ source code on GitHub]

static NppData nppData;
static FuncItem funcItems[1]; // One menu item: Toggle Preview

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
    nppData = notepadPlusData;
}

extern "C" __declspec(dllexport) const wchar_t* getName() {
    return L"MarkdownPreview";
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF) {
    *nbF = 1;
    return funcItems;
}

extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notification) {
    // Handle NPPN_READY, NPPN_SHUTDOWN, NPPN_TBMODIFICATION
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT msg, WPARAM wp, LPARAM lp) {
    return TRUE;
}
```

### Pattern 2: Menu Item with Keyboard Shortcut
**What:** Register the toggle command with Ctrl+Shift+M shortcut.
**Example:**
```cpp
// [VERIFIED: PluginInterface.h structure definitions]
static ShortcutKey toggleShortcut = { true, false, true, 'M' }; // Ctrl+Shift+M

void initFuncItem() {
    wcscpy_s(funcItems[0]._itemName, menuItemSize, L"Toggle Preview");
    funcItems[0]._pFunc = togglePreview;
    funcItems[0]._init2Check = false;
    funcItems[0]._pShKey = &toggleShortcut;
}
```

### Pattern 3: Docking Panel Registration
**What:** Create a Win32 child window and register it as a Notepad++ dockable dialog.
**When to use:** On first toggle (lazy initialization per D-06).
**Example:**
```cpp
// Source: Notepad++ Docking.h constants
// [VERIFIED: Notepad++ source code on GitHub]

void registerDockingPanel(HWND hPanel) {
    tTbData dockData = {};
    dockData.hClient = hPanel;
    dockData.pszName = L"Markdown Preview";
    dockData.dlgID = 0; // Index into funcItems
    dockData.uMask = DWS_DF_CONT_RIGHT; // (CONT_RIGHT << 28) = 0x10000000
    dockData.pszModuleName = L"MarkdownPreview.dll";

    ::SendMessage(nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&dockData);
}

void showPanel(HWND hPanel) {
    ::SendMessage(nppData._nppHandle, NPPM_DMMSHOW, 0, (LPARAM)hPanel);
}

void hidePanel(HWND hPanel) {
    ::SendMessage(nppData._nppHandle, NPPM_DMMHIDE, 0, (LPARAM)hPanel);
}
```

### Pattern 4: WebView2 Initialization in Docked Panel
**What:** Asynchronously create WebView2 environment and controller inside the docking panel HWND.
**When to use:** After the docking panel is registered and shown, and only if WebView2 runtime is detected.
**Example:**
```cpp
// Source: https://learn.microsoft.com/en-us/microsoft-edge/webview2/get-started/win32
// [CITED: Microsoft official WebView2 Win32 getting started guide]

#include <WebView2.h>
#include <wrl.h>
#include <wil/com.h>

using namespace Microsoft::WRL;

// Must call CoInitializeEx before WebView2 creation
void initWebView2(HWND hHost) {
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, // browserExecutableFolder - use installed runtime
        L"<user_data_folder>", // userDataFolder
        nullptr, // environmentOptions
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hHost](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) return result;

                env->CreateCoreWebView2Controller(hHost,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hHost](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) return result;

                            // Store controller and webview
                            // Resize to fill panel
                            RECT bounds;
                            GetClientRect(hHost, &bounds);
                            controller->put_Bounds(bounds);

                            wil::com_ptr<ICoreWebView2> webview;
                            controller->get_CoreWebView2(&webview);

                            // Configure settings
                            wil::com_ptr<ICoreWebView2Settings> settings;
                            webview->get_Settings(&settings);
                            settings->put_AreDefaultContextMenusEnabled(FALSE);
                            settings->put_AreDevToolsEnabled(FALSE);
                            settings->put_IsStatusBarEnabled(FALSE);

                            // Navigate to welcome page
                            webview->Navigate(L"https://appassets.mdpreview/welcome.html");
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}
```

### Pattern 5: WebView2 Runtime Detection (Lazy)
**What:** Check WebView2 availability only when panel first opens, show fallback UI if missing.
**Example:**
```cpp
// Source: https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution
// [CITED: Microsoft official WebView2 distribution guide]

#include <WebView2EnvironmentOptions.h>

bool isWebView2Available() {
    LPWSTR versionInfo = nullptr;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &versionInfo);
    bool available = SUCCEEDED(hr) && versionInfo != nullptr;
    if (versionInfo) CoTaskMemFree(versionInfo);
    return available;
}

// If not available, create a Win32 static control in the panel with:
// "WebView2 Runtime is required for Markdown Preview.\n\n"
// "Download it from:\nhttps://developer.microsoft.com/microsoft-edge/webview2"
// Make the URL a clickable link (SysLink control or WM_CTLCOLORSTATIC + WM_SETCURSOR)
```

### Pattern 6: Local Content via Virtual Host Mapping
**What:** Map a local assets folder to a virtual hostname so WebView2 can load local HTML/CSS/JS.
**When to use:** For loading welcome.html and later preview.html from the plugin's assets directory.
**Example:**
```cpp
// Source: https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/working-with-local-content
// [CITED: Microsoft official WebView2 local content guide]

// After getting ICoreWebView2 (version 3+):
wil::com_ptr<ICoreWebView2_3> webview3;
webview->QueryInterface(IID_PPV_ARGS(&webview3));

webview3->SetVirtualHostNameToFolderMapping(
    L"appassets.mdpreview",
    assetsPath.c_str(),  // Absolute path to plugin's assets/ directory
    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);

// Then navigate to: https://appassets.mdpreview/welcome.html
```

### Anti-Patterns to Avoid
- **Blocking the UI thread during WebView2 init:** WebView2 creation is async. Never use synchronous waits. The Callback<> pattern ensures non-blocking init. [CITED: Microsoft WebView2 docs]
- **Checking WebView2 at DLL load time:** Per D-06, check lazily on first panel open. Checking at startup adds load time for users who never open the preview.
- **Using file:// URLs in WebView2:** Some WebView2 versions have restrictions on file:// access. Use `SetVirtualHostNameToFolderMapping` instead. [CITED: learn.microsoft.com WebView2 local content]
- **Single-platform build:** Always maintain both x86 and x64 configurations. Notepad++ 32-bit is still widely used. [ASSUMED]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| COM pointer management | Raw AddRef/Release | WIL `wil::com_ptr<>` | Prevents leaks, exception-safe RAII [CITED: Microsoft WIL docs] |
| WebView2 async callbacks | Custom callback classes | WRL `Callback<>` template | Standard pattern from Microsoft samples, type-safe [CITED: WebView2 getting started guide] |
| JSON settings | Custom parser | nlohmann/json | Header-only, handles edge cases, already in CLAUDE.md stack [CITED: CLAUDE.md] |
| Plugin scaffolding | From-scratch DLL | NppCppMSVS template | Provides correct export signatures, Scintilla headers, dialog examples [CITED: community.notepad-plus-plus.org] |
| Docking dialog infrastructure | Custom window management | Notepad++ NPPM_DMMREGASDCKDLG API | Notepad++ handles dock position persistence, drag-to-redock, tab management [VERIFIED: Notepad++ source] |

## Common Pitfalls

### Pitfall 1: WebView2Loader.dll Architecture Mismatch
**What goes wrong:** Plugin loads in 64-bit Notepad++ but WebView2 fails because 32-bit WebView2Loader.dll is bundled (or vice versa).
**Why it happens:** WebView2Loader.dll must match the plugin DLL architecture, not the WebView2 Runtime architecture.
**How to avoid:** Build produces separate x86/x64 output directories. Each includes the matching WebView2Loader.dll from the NuGet package (`build/native/{x86|x64}/WebView2Loader.dll`). Verify in post-build step.
**Warning signs:** `HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT)` or DLL load failure.
[VERIFIED: Microsoft WebView2 distribution docs state "WebView2Loader.dll is a native and architecture-specific binary"]

### Pitfall 2: CoInitializeEx Not Called Before WebView2
**What goes wrong:** `CreateCoreWebView2EnvironmentWithOptions` returns `CO_E_NOTINITIALIZED` or undefined behavior.
**Why it happens:** WebView2 requires COM STA (Single-Threaded Apartment) initialization.
**How to avoid:** Call `CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)` early in the plugin initialization (in `setInfo` or before first WebView2 call). Notepad++ may already initialize COM, but it's safest to call it explicitly (duplicate calls return `S_FALSE` which is fine).
**Warning signs:** Random crashes or HRESULT errors during environment creation.
[CITED: Microsoft WebView2 docs - "The application is expected to call CoInitializeEx before calling CreateCoreWebView2EnvironmentWithOptions"]

### Pitfall 3: Panel HWND Not Ready When WebView2 Initializes
**What goes wrong:** WebView2 controller creation fails because the host HWND is not yet visible or has zero size.
**Why it happens:** Docking panel registration and display are asynchronous in Notepad++.
**How to avoid:** Initialize WebView2 after `NPPM_DMMSHOW` returns and the panel has a valid client rect. Handle `WM_SIZE` to resize the WebView2 controller bounds.
**Warning signs:** WebView2 appears as zero-size or doesn't render.
[ASSUMED]

### Pitfall 4: Docking Panel Not Persisting Position
**What goes wrong:** Panel always opens on the right even after user moves it.
**Why it happens:** Notepad++ handles dock position persistence internally through its config, but the plugin must NOT re-register the docking dialog on every toggle. Register once, then use DMMSHOW/DMMHIDE.
**How to avoid:** Register the docking dialog once (on first use), then only toggle visibility with NPPM_DMMSHOW/NPPM_DMMHIDE. Notepad++ remembers the last dock position.
**Warning signs:** Panel jumps back to default position after re-opening Notepad++.
[ASSUMED]

### Pitfall 5: WebView2 User Data Folder Conflicts
**What goes wrong:** Multiple instances of Notepad++ or other WebView2 apps lock the same user data folder.
**Why it happens:** WebView2 creates browser profile data in the user data folder. If two processes use the same folder path, one will fail.
**How to avoid:** Use a unique user data folder path specific to the plugin, e.g., `%LOCALAPPDATA%\MarkdownPreview\WebView2Data`. Do NOT use the plugin config dir under `%APPDATA%\Notepad++` -- that's for settings only.
**Warning signs:** Second Notepad++ instance fails to initialize WebView2 with `HRESULT_FROM_WIN32(ERROR_INVALID_STATE)`.
[CITED: Microsoft WebView2 docs - "the version associated with a WebView2 Environment may have been removed"]

### Pitfall 6: NppCppMSVS Template GitHub Source Code Zip Won't Work
**What goes wrong:** Creating a VS project from the template fails with errors.
**Why it happens:** GitHub's "Source code (zip)" download adds an extra folder level that Visual Studio rejects as a template.
**How to avoid:** Download only `NppCppMSVS-1.0.zip` from the releases page. Place it in the VS templates directory without unzipping.
**Warning signs:** Visual Studio doesn't show the template in "Create New Project" dialog.
[CITED: community.notepad-plus-plus.org/topic/26673]

## Code Examples

### Settings Persistence with nlohmann/json
```cpp
// Source: CLAUDE.md stack decision + nlohmann/json standard patterns
// [ASSUMED: standard nlohmann/json usage pattern]

#include <nlohmann/json.hpp>
#include <fstream>

struct PluginSettings {
    bool panelVisible = false;
    // Dock position is managed by Notepad++ internally

    void load(const std::wstring& configPath) {
        std::ifstream f(configPath);
        if (!f.is_open()) return;
        try {
            nlohmann::json j = nlohmann::json::parse(f);
            panelVisible = j.value("panelVisible", false);
        } catch (...) {
            // Use defaults on parse error
        }
    }

    void save(const std::wstring& configPath) {
        nlohmann::json j;
        j["panelVisible"] = panelVisible;
        std::ofstream f(configPath);
        f << j.dump(2);
    }
};
```

### Getting Plugin Config Directory
```cpp
// Source: https://npp-user-manual.org/docs/plugin-communication/
// [VERIFIED: Notepad++ plugin message reference]

std::wstring getConfigDir() {
    wchar_t configDir[MAX_PATH] = {};
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR,
                  MAX_PATH, (LPARAM)configDir);
    return std::wstring(configDir);
}
// Settings file: getConfigDir() + L"\\MarkdownPreview.json"
```

### Handling WM_SIZE for WebView2 Resize
```cpp
// Source: Microsoft WebView2 Win32 getting started guide
// [CITED: learn.microsoft.com WebView2 getting started guide]

// In the panel's WndProc:
case WM_SIZE: {
    if (webviewController) {
        RECT bounds;
        GetClientRect(hWnd, &bounds);
        webviewController->put_Bounds(bounds);
    }
    return 0;
}
```

### Win32 Fallback UI for Missing WebView2
```cpp
// [ASSUMED: standard Win32 SysLink control pattern]

void showWebView2MissingMessage(HWND hPanel) {
    // Use a SysLink control for clickable URL
    HWND hLink = CreateWindowExW(0, WC_LINK, 
        L"WebView2 Runtime is required for Markdown Preview.\r\n\r\n"
        L"<a href=\"https://developer.microsoft.com/microsoft-edge/webview2\">Download WebView2 Runtime</a>",
        WS_VISIBLE | WS_CHILD,
        20, 20, 400, 100,
        hPanel, nullptr, hInstance, nullptr);
    // Handle NM_CLICK/NM_RETURN in WM_NOTIFY to open URL via ShellExecute
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| CEF (Chromium Embedded Framework) | WebView2 | 2020+ | WebView2 is 200MB+ lighter, auto-updates, ships with Windows 11 |
| C# plugin with .NET | C++ native plugin | Always preferred | No .NET runtime dependency, direct ABI compatibility with Notepad++ |
| Custom HTML renderer | WebView2 | 2020+ | Full HTML5/CSS3/JS support, maintained by Microsoft |
| Manual COM reference counting | WIL (wil::com_ptr) | 2019+ | RAII-based, exception-safe, less boilerplate |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Windows SDK 10.0.22621+ is available in VS installation | Standard Stack | Build failure -- would need SDK install |
| A2 | Notepad++ 32-bit is still widely used enough to require x86 build | Anti-Patterns | Could skip x86 but plugin list requires both |
| A3 | Panel HWND must be visible before WebView2 controller creation | Pitfall 3 | WebView2 might handle zero-size HWND gracefully |
| A4 | Notepad++ persists dock position without plugin intervention | Pitfall 4 | Plugin may need to save/restore dock position manually |
| A5 | SysLink control (WC_LINK) is the right approach for clickable URL in fallback UI | Code Examples | Could use static text + manual hit testing instead |

## Open Questions

1. **NppCppMSVS template internal structure**
   - What we know: It provides docking dialog examples, settings JSON, Scintilla headers
   - What's unclear: Exact file layout, whether it includes both x86/x64 configurations out of the box
   - Recommendation: Download and inspect the template during implementation; adapt structure as needed

2. **WebView2 user data folder location**
   - What we know: Must be unique per plugin to avoid conflicts; D-14 puts settings in %AppData%/Notepad++/plugins/config/
   - What's unclear: Best location for WebView2 browser profile data (can be hundreds of MB)
   - Recommendation: Use `%LOCALAPPDATA%\MarkdownPreview\WebView2Data` -- keeps it separate from roaming settings and is the standard pattern for WebView2 apps (Claude's discretion area)

3. **Initial panel width at 50%**
   - What we know: D-03 specifies 50% of Notepad++ window width
   - What's unclear: Whether Notepad++ docking API allows specifying initial width, or if it's controlled by the panel's initial window size
   - Recommendation: Set the panel's initial window width to 50% of Notepad++ client area before registering; Notepad++ may or may not honor this

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Visual Studio 2022 | Build system | Yes | Professional (MSBuild 18.3) | -- |
| Notepad++ (64-bit) | Testing | Yes | (installed at C:/Program Files/Notepad++) | -- |
| WebView2 Runtime | Rendering | Yes | 146.0.3856.109 | Fallback UI per D-05/D-07 |
| NuGet | WebView2 SDK + WIL install | Via VS only | (not in PATH standalone) | Use VS NuGet Package Manager UI or PackageReference in vcxproj |
| vcpkg | Optional (nlohmann/json) | No | -- | Use header-only include or NuGet for nlohmann/json |

**Missing dependencies with no fallback:**
- None -- all critical dependencies are available.

**Missing dependencies with fallback:**
- vcpkg not installed -- use nlohmann/json as a single header include (`json.hpp`) downloaded directly, or install via NuGet.
- NuGet CLI not in PATH -- use Visual Studio's built-in NuGet Package Manager for package installation.

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | No | N/A -- local desktop plugin |
| V3 Session Management | No | N/A |
| V4 Access Control | No | N/A |
| V5 Input Validation | Yes (Phase 2+) | Not applicable in Phase 1 -- no user input processing yet |
| V6 Cryptography | No | N/A |

### Known Threat Patterns for WebView2 in Plugin Context

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| WebView2 navigating to external URLs | Information Disclosure | Disable default context menus, restrict navigation to local virtual host only [CITED: WebView2 settings API] |
| Plugin DLL injection | Tampering | Notepad++ handles DLL loading; sign plugin DLL for distribution [ASSUMED] |
| Malicious content in rendered markdown | Elevation of Privilege | Phase 2 concern -- sanitize HTML output from markdown-it |

Phase 1 has minimal security surface: WebView2 only loads local content (welcome page), no user input processing, no network access needed.

## Sources

### Primary (HIGH confidence)
- [Notepad++ PluginInterface.h](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/MISC/PluginsManager/PluginInterface.h) - DLL exports, FuncItem, NppData structures
- [Notepad++ Docking.h](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/WinControls/DockingWnd/Docking.h) - Docking constants, tTbData structure
- [Notepad++ Notepad_plus_msgs.h](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/MISC/PluginsManager/Notepad_plus_msgs.h) - NPPM_DMMSHOW/HIDE/REGASDCKDLG constants
- [WebView2 Win32 Getting Started](https://learn.microsoft.com/en-us/microsoft-edge/webview2/get-started/win32) - Initialization pattern, WIL/WRL usage
- [WebView2 Distribution Guide](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution) - Runtime detection, GetAvailableCoreWebView2BrowserVersionString
- [WebView2 Local Content](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/working-with-local-content) - SetVirtualHostNameToFolderMapping

### Secondary (MEDIUM confidence)
- [NppCppMSVS Community Forum](https://community.notepad-plus-plus.org/topic/26673/nppcppmsvs-a-visual-studio-project-template-for-a-notepad-c-plugin) - Template features, download instructions
- [NppCppMSVS GitHub Releases](https://github.com/Coises/NppCppMSVS/releases) - v1.0 template download

### Tertiary (LOW confidence)
- None -- all claims verified against official sources or flagged as ASSUMED.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All libraries verified against official sources and CLAUDE.md decisions
- Architecture: HIGH - Patterns derived from official Notepad++ source code and Microsoft WebView2 documentation
- Pitfalls: MEDIUM - Some pitfalls based on experience patterns rather than documented issues (A3, A4)

**Research date:** 2026-04-08
**Valid until:** 2026-05-08 (stable APIs, no rapid changes expected)
