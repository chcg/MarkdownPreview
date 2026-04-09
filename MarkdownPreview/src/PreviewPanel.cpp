// This file is part of MarkdownPreview plugin for Notepad++
// Dockable preview panel - WebView2 initialization, runtime detection, fallback UI

#include "PreviewPanel.h"
#include "Docking.h"
#include "Notepad_plus_msgs.h"
#include "Scintilla.h"

#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wrl.h>
#include <WebView2EnvironmentOptions.h>
#include <nlohmann/json.hpp>
#include <fstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace Microsoft::WRL;

// UTF-8 to wide string conversion helper.
// Uses MultiByteToWideChar(CP_UTF8) — the only correct approach for editor content.
// Never use std::wstring(str.begin(), str.end()) — that zero-extends bytes, corrupting
// any multi-byte UTF-8 sequence (accented chars, CJK, emoji).
static std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int wlen = ::MultiByteToWideChar(CP_UTF8, 0,
        utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0,
        utf8.c_str(), static_cast<int>(utf8.size()), &w[0], wlen);
    return w;
}

// Include PluginInterface.h for NppData struct definition.
// We declare nppData extern directly (defined in PluginDefinition.cpp).
// We do NOT include PluginDefinition.h here to avoid a circular dependency:
//   PreviewPanel.h is included by PluginDefinition.h, so PluginDefinition.h
//   cannot be included back into PreviewPanel.cpp.
#include "../include/PluginInterface.h"
#include "Settings.h"
extern NppData nppData;
extern Settings g_settings;

static const wchar_t PANEL_CLASS_NAME[] = L"MarkdownPreviewPanel";
static const wchar_t PANEL_TITLE[] = L"Markdown Preview";
static const wchar_t MODULE_NAME[] = L"MarkdownPreview.dll";

void PreviewPanel::init(HINSTANCE hInst, HWND nppHandle) {
    m_hInst = hInst;
    m_nppHandle = nppHandle;
}

void PreviewPanel::destroy() {
    // Unregister WebMessageReceived before Close() to avoid use-after-free
    // if the panel is destroyed and re-initialized rapidly (WR-01 mitigation).
    if (m_webview && m_webMessageReceivedToken.value != 0) {
        m_webview->remove_WebMessageReceived(m_webMessageReceivedToken);
        m_webMessageReceivedToken = {};
    }
    if (m_controller && m_accelKeyToken.value != 0) {
        m_controller->remove_AcceleratorKeyPressed(m_accelKeyToken);
        m_accelKeyToken = {};
    }
    if (m_webview && m_navigationCompletedToken.value != 0) {
        m_webview->remove_NavigationCompleted(m_navigationCompletedToken);
        m_navigationCompletedToken = {};
    }
    if (m_controller) {
        m_controller->Close();
        m_controller = nullptr;
        m_webview = nullptr;
    }
    if (m_hFallback) {
        ::DestroyWindow(m_hFallback);
        m_hFallback = nullptr;
    }
    // NOTE: Do NOT call DestroyWindow on m_hPanel here.
    // The panel HWND is managed by Notepad++'s docking manager after
    // NPPM_DMMREGASDCKDLG. NPP destroys it during its own shutdown.
    // Destroying it ourselves causes NPP to reference a stale HWND
    // which can lead to crashes or broken state on restart.
    m_hPanel = nullptr;

    // Reset state so re-registration works if needed
    m_isRegistered = false;
    m_isVisible = false;
    m_webview2Available = false;
    m_webview2Initialized = false;
}

void PreviewPanel::createHostWindow() {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = PreviewPanel::wndProc;
    wc.hInstance = m_hInst;
    wc.lpszClassName = PANEL_CLASS_NAME;
    wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    ::RegisterClassEx(&wc);

    // Per D-03: initial width is 50% of Notepad++ client area
    RECT nppRect = {};
    ::GetClientRect(m_nppHandle, &nppRect);
    int panelWidth = nppRect.right / 2;
    int panelHeight = nppRect.bottom;

    m_hPanel = ::CreateWindowEx(
        0,
        PANEL_CLASS_NAME,
        PANEL_TITLE,
        WS_CHILD | WS_CLIPCHILDREN,
        0, 0,
        panelWidth, panelHeight,
        m_nppHandle,
        nullptr,
        m_hInst,
        nullptr);

    if (m_hPanel) {
        // Store this pointer for WndProc dispatch
        ::SetWindowLongPtr(m_hPanel, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
}

void PreviewPanel::registerPanel() {
    createHostWindow();

    if (!m_hPanel) return;

    // Register with Notepad++ docking manager
    // Per D-01: dock on right side by default
    // Per Pitfall 4: register ONCE, then use DMMSHOW/DMMHIDE only
    tTbData dockData = {};
    dockData.hClient = m_hPanel;
    dockData.pszName = PANEL_TITLE;
    dockData.dlgID = 0;  // Index into funcItems
    dockData.uMask = DWS_DF_CONT_RIGHT;
    dockData.hIconTab = nullptr;
    dockData.pszAddInfo = nullptr;
    dockData.rcFloat = {};
    dockData.iPrevCont = -1;
    dockData.pszModuleName = MODULE_NAME;

    ::SendMessage(m_nppHandle, NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&dockData));

    // Per D-06: lazy check -- only when panel first opens
    if (checkWebView2Available()) {
        m_webview2Available = true;
        initWebView2();
    } else {
        showWebView2MissingFallback();
    }
}

void PreviewPanel::toggle(int cmdID) {
    m_cmdID = cmdID;

    if (!m_isRegistered) {
        registerPanel();
        // Only mark as registered if the panel was actually created
        if (!m_hPanel) return;
        m_isRegistered = true;
    }

    // Guard against toggle with no panel (should not happen, but defensive)
    if (!m_hPanel) return;

    if (m_isVisible) {
        ::SendMessage(m_nppHandle, NPPM_DMMHIDE, 0, reinterpret_cast<LPARAM>(m_hPanel));
        m_isVisible = false;
    } else {
        ::SendMessage(m_nppHandle, NPPM_DMMSHOW, 0, reinterpret_cast<LPARAM>(m_hPanel));
        m_isVisible = true;
    }

    // Update menu checkmark to reflect panel visibility state
    ::SendMessage(m_nppHandle, NPPM_SETMENUITEMCHECK, m_cmdID, m_isVisible);
}

bool PreviewPanel::isVisible() const {
    return m_isVisible;
}

HWND PreviewPanel::getHwnd() const {
    return m_hPanel;
}

// Per D-06: lazy runtime detection -- only called on first panel open
bool PreviewPanel::checkWebView2Available() {
    LPWSTR versionInfo = nullptr;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &versionInfo);
    bool available = SUCCEEDED(hr) && versionInfo != nullptr;
    if (versionInfo) CoTaskMemFree(versionInfo);
    return available;
}

void PreviewPanel::initWebView2() {
    std::wstring userDataPath = getUserDataPath();
    // Create directory hierarchy if it doesn't exist
    // SHCreateDirectoryExW creates intermediate directories
    SHCreateDirectoryExW(nullptr, userDataPath.c_str(), nullptr);

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,  // browserExecutableFolder - use installed runtime
        userDataPath.c_str(),
        nullptr,  // environmentOptions
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) return result;

                // Store environment pointer — required for PDF export (Plan 04, PrintToPdf)
                m_environment = env;

                env->CreateCoreWebView2Controller(m_hPanel,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) return result;

                            m_controller = controller;
                            m_controller->get_CoreWebView2(&m_webview);

                            // Resize to fill panel
                            resizeWebView2();

                            // Phase 3: Zoom keyboard interception (THME-04, D-04)
                            // AcceleratorKeyPressed fires BEFORE key reaches web content.
                            // Registered on controller (not webview) — Pitfall 4 mitigation.
                            // put_Handled(TRUE) suppresses WebView2 built-in Ctrl+/-/0 zoom.
                            m_controller->add_AcceleratorKeyPressed(
                                Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                                    [this](ICoreWebView2Controller* /*sender*/,
                                           ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
                                        COREWEBVIEW2_KEY_EVENT_KIND kind;
                                        args->get_KeyEventKind(&kind);
                                        if (kind != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN &&
                                            kind != COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN) {
                                            return S_OK;
                                        }

                                        UINT vk = 0;
                                        args->get_VirtualKey(&vk);
                                        bool ctrlDown = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
                                        if (!ctrlDown) return S_OK;

                                        // VK_OEM_PLUS = 0xBB (= key on US keyboard, used for Ctrl+=)
                                        // VK_OEM_MINUS = 0xBD (- key)
                                        // 0x30 = virtual key code for '0' (Ctrl+0 = reset)
                                        bool isZoomKey = (vk == VK_OEM_PLUS || vk == VK_OEM_MINUS || vk == 0x30);
                                        if (!isZoomKey) return S_OK;

                                        // Suppress WebView2 built-in zoom behavior
                                        args->put_Handled(TRUE);

                                        // WR-02: Block zoom changes while PDF export is in flight.
                                        // Allowing zoom during the reset/restore cycle causes m_zoomLevel
                                        // and the JS zoom to diverge after the completion callback restores.
                                        if (m_printToPdfInProgress) {
                                            return S_OK;
                                        }

                                        // Compute new zoom level with 10% step (D-04)
                                        const float step = 0.1f;
                                        const float minZoom = 0.8f;   // 80% per THME-04
                                        const float maxZoom = 8.0f;   // 800% per THME-04

                                        if (vk == VK_OEM_PLUS) {
                                            m_zoomLevel = min(maxZoom, m_zoomLevel + step);
                                        } else if (vk == VK_OEM_MINUS) {
                                            m_zoomLevel = max(minZoom, m_zoomLevel - step);
                                        } else {  // 0x30 = '0'
                                            m_zoomLevel = 1.0f;  // Ctrl+0 resets to 100%
                                        }

                                        // D-05: persist immediately to settings.json
                                        g_settings.zoomLevel = m_zoomLevel;
                                        if (!m_configPath.empty()) {
                                            g_settings.save(m_configPath);
                                        }

                                        // Post zoom level to JS
                                        postZoomToJs(m_zoomLevel);

                                        return S_OK;
                                    }).Get(),
                                &m_accelKeyToken);

                            // Configure settings per UI-SPEC Phase-Specific Note 5
                            wil::com_ptr<ICoreWebView2Settings> settings;
                            m_webview->get_Settings(&settings);
                            settings->put_AreDefaultContextMenusEnabled(FALSE);
                            settings->put_AreDevToolsEnabled(FALSE);
                            settings->put_IsStatusBarEnabled(FALSE);

                            // Set up virtual host mapping per Pattern 6
                            wil::com_ptr<ICoreWebView2_3> webview3;
                            m_webview->QueryInterface(IID_PPV_ARGS(&webview3));
                            if (webview3) {
                                std::wstring assetsPath = getAssetsPath();
                                webview3->SetVirtualHostNameToFolderMapping(
                                    L"appassets.mdpreview",
                                    assetsPath.c_str(),
                                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
                            }

                            // Navigate to preview.html — the single rendering page that handles
                            // all message types including render, theme, scroll, export, and idle.
                            // welcome.html has no WebView2 message listener, so any PostWebMessageAsJson
                            // call while welcome.html is loaded would be silently discarded (BUG FIX).
                            m_webview->Navigate(L"https://appassets.mdpreview/preview.html");
                            m_webview2Initialized = true;

                            // Apply persisted zoom level (D-05)
                            m_zoomLevel = g_settings.zoomLevel;

                            // WR-03: Wire NavigationCompleted so that applyInitialZoom and any
                            // pending render are posted only after preview.html has fully loaded
                            // and its message listener is registered. Navigate() is async — posting
                            // zoom or render messages immediately after Navigate() risks them being
                            // silently dropped if the JS listener has not yet registered.
                            m_webview->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [this](ICoreWebView2* /*sender*/,
                                           ICoreWebView2NavigationCompletedEventArgs* /*args*/) -> HRESULT {
                                        applyInitialZoom(m_zoomLevel);
                                        if (!m_pendingFilePath.empty()) {
                                            std::wstring pending = m_pendingFilePath;
                                            m_pendingFilePath.clear();
                                            renderMarkdown(pending);
                                        }
                                        return S_OK;
                                    }).Get(),
                                &m_navigationCompletedToken);

                            // Wire JS->C++ message channel (used by export in Plan 02-04)
                            // T-02-04 mitigation: handler wraps parse in try/catch; no shell/exec
                            m_webview->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [this](ICoreWebView2* /*sender*/,
                                           ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        wil::unique_cotaskmem_string rawMsg;
                                        HRESULT hr = args->TryGetWebMessageAsString(&rawMsg);
                                        if (SUCCEEDED(hr) && rawMsg) {
                                            handleJsMessage(rawMsg.get());
                                        }
                                        return S_OK;
                                    }).Get(),
                                &m_webMessageReceivedToken);

                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}

// Per D-05, D-07: show fallback message in panel when WebView2 is missing
void PreviewPanel::showWebView2MissingFallback() {
    // Per UI-SPEC: SysLink at (20, 20), 400x100
    m_hFallback = CreateWindowExW(0, WC_LINK,
        L"WebView2 Runtime is required for Markdown Preview.\r\n\r\n"
        L"<a href=\"https://developer.microsoft.com/microsoft-edge/webview2\">Download WebView2 Runtime</a>",
        WS_VISIBLE | WS_CHILD,
        20, 20, 400, 100,
        m_hPanel, nullptr, m_hInst, nullptr);
}

void PreviewPanel::resizeWebView2() {
    if (m_controller && m_hPanel) {
        RECT bounds;
        ::GetClientRect(m_hPanel, &bounds);
        m_controller->put_Bounds(bounds);
    }
}

std::wstring PreviewPanel::getUserDataPath() {
    wchar_t localAppData[MAX_PATH] = {};
    DWORD ret = ::GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    if (ret == 0 || ret >= MAX_PATH) {
        // Fallback: place WebView2 data next to the DLL (always writable by plugin)
        return getAssetsPath() + L"\\WebView2Data";
    }
    return std::wstring(localAppData) + L"\\MarkdownPreview\\WebView2Data";
}

std::wstring PreviewPanel::getAssetsPath() {
    wchar_t dllPath[MAX_PATH] = {};
    GetModuleFileNameW(m_hInst, dllPath, MAX_PATH);
    std::wstring path(dllPath);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) path = path.substr(0, pos);
    return path + L"\\assets";
}

// Phase 2: Debounce timer — start/restart 300ms countdown
void PreviewPanel::scheduleRender() {
    if (!m_webview2Initialized || !m_hPanel) return;
    ::KillTimer(m_hPanel, DEBOUNCE_TIMER_ID);
    m_renderPending = true;
    ::SetTimer(m_hPanel, DEBOUNCE_TIMER_ID, 300, nullptr);
}

// Phase 2: Called when debounce timer fires
void PreviewPanel::doRender() {
    m_renderPending = false;
    if (!m_webview || m_currentFilePath.empty()) return;
    renderMarkdown(m_currentFilePath);
}

// Phase 2: Retrieve UTF-8 text from active Scintilla view and convert to wstring
std::wstring PreviewPanel::getCurrentText() {
    int sciId = 0;
    ::SendMessage(m_nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&sciId));
    // nppData is defined in PluginDefinition.cpp; accessed via extern declaration above
    HWND hSci = (sciId == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    int len = static_cast<int>(::SendMessage(hSci, SCI_GETLENGTH, 0, 0));
    std::string utf8Text(static_cast<size_t>(len) + 1, '\0');
    ::SendMessage(hSci, SCI_GETTEXT, static_cast<WPARAM>(len + 1),
        reinterpret_cast<LPARAM>(utf8Text.data()));
    utf8Text.resize(static_cast<size_t>(len));

    // Convert UTF-8 to wstring (CP_UTF8 — never use CP_ACP for editor content)
    if (utf8Text.empty()) return L"";
    int wlen = ::MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(),
        static_cast<int>(utf8Text.size()), nullptr, 0);
    std::wstring wtext(static_cast<size_t>(wlen), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(),
        static_cast<int>(utf8Text.size()), &wtext[0], wlen);
    return wtext;
}

// Phase 2: Retrieve Scintilla text, encode as JSON, post to WebView2
void PreviewPanel::renderMarkdown(const std::wstring& filePath) {
    if (!m_webview || !m_webview2Initialized) {
        // WebView2 is still initializing (async). Store the path so the controller
        // completion callback can fire the render once initialization is done.
        m_pendingFilePath = filePath;
        return;
    }
    m_pendingFilePath.clear();  // cancel any stored pending — this call supersedes it
    m_currentFilePath = filePath;

    std::wstring wtext = getCurrentText();
    // Convert wstring to UTF-8 for nlohmann (nlohmann handles UTF-8 natively)
    int utf8len = ::WideCharToMultiByte(CP_UTF8, 0, wtext.c_str(),
        static_cast<int>(wtext.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8Markdown(static_cast<size_t>(utf8len), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wtext.c_str(),
        static_cast<int>(wtext.size()), &utf8Markdown[0], utf8len, nullptr, nullptr);

    // Build file path as UTF-8
    int pathLen = ::WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(),
        static_cast<int>(filePath.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8FilePath(static_cast<size_t>(pathLen), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(),
        static_cast<int>(filePath.size()), &utf8FilePath[0], pathLen, nullptr, nullptr);

    // T-02-01 mitigation: use nlohmann for all JSON construction — never concatenate markdown content
    nlohmann::json j;
    j["type"] = "render";
    j["markdown"] = utf8Markdown;
    j["filePath"] = utf8FilePath;

    // THME-02 (D-07): Read custom CSS from %APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css
    // Absence of the file is silently ignored per D-07 — send JSON null.
    {
        wchar_t appData[MAX_PATH] = {};
        DWORD ret = ::GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);
        if (ret > 0 && ret < MAX_PATH) {
            std::wstring cssPath = std::wstring(appData)
                + L"\\Notepad++\\plugins\\config\\MarkdownPreview\\custom.css";
            std::ifstream cssFile(cssPath, std::ios::in | std::ios::binary);
            if (cssFile.is_open()) {
                std::string cssContent((std::istreambuf_iterator<char>(cssFile)),
                                        std::istreambuf_iterator<char>());
                cssFile.close();
                if (!cssContent.empty()) {
                    j["customCss"] = cssContent;
                } else {
                    j["customCss"] = nullptr;
                }
            } else {
                j["customCss"] = nullptr;
            }
        } else {
            j["customCss"] = nullptr;
        }
    }

    std::string jsonStr = j.dump();
    std::wstring wjson = Utf8ToWide(jsonStr);
    m_webview->PostWebMessageAsJson(wjson.c_str());
}

// Phase 2: Post theme change message to WebView2
void PreviewPanel::setTheme(bool isDark) {
    m_isDark = isDark;
    if (!m_webview || !m_webview2Initialized) return;
    nlohmann::json j;
    j["type"] = "theme";
    j["dark"] = isDark;
    std::string jsonStr = j.dump();
    std::wstring wjson = Utf8ToWide(jsonStr);
    m_webview->PostWebMessageAsJson(wjson.c_str());
}

// Phase 2: Show idle/welcome state when a non-.md file is activated.
// Posts {type:"idle"} to preview.html instead of re-navigating to welcome.html.
// Re-navigating to welcome.html would unload the message listener in preview.html,
// causing any subsequent renderMarkdown() call to be silently dropped (BUG FIX).
void PreviewPanel::showIdle() {
    if (!m_webview || !m_webview2Initialized) return;
    nlohmann::json j;
    j["type"] = "idle";
    std::string jsonStr = j.dump();
    std::wstring wjson = Utf8ToWide(jsonStr);
    m_webview->PostWebMessageAsJson(wjson.c_str());
}

// Phase 3: Apply initial zoom level from persisted settings (D-05, THME-04)
// Called after WebView2 navigation completes and JS is ready to receive messages.
void PreviewPanel::applyInitialZoom(float level) {
    m_zoomLevel = level;
    postZoomToJs(level);
}

// Phase 3: Post {type:"zoom", level:N} to WebView2 JS dispatcher
void PreviewPanel::postZoomToJs(float level) {
    if (!m_webview || !m_webview2Initialized) return;
    nlohmann::json j;
    j["type"] = "zoom";
    j["level"] = level;
    std::string jsonStr = j.dump();
    std::wstring wjson = Utf8ToWide(jsonStr);
    m_webview->PostWebMessageAsJson(wjson.c_str());
}

// Phase 2 Plan 03: Post scroll message to JS — {type:"scroll", line:N}
void PreviewPanel::scrollToLine(int line) {
    if (!m_webview || !m_webview2Initialized) return;
    nlohmann::json j;
    j["type"] = "scroll";
    j["line"] = line;
    std::string jsonStr = j.dump();
    std::wstring wjson = Utf8ToWide(jsonStr);
    m_webview->PostWebMessageAsJson(wjson.c_str());
}

// Phase 2 Plan 03: Map file.mdpreview virtual host to active file's parent directory (REND-05, D-08)
// Uses ICoreWebView2_3 ClearVirtualHostNameToFolderMapping + SetVirtualHostNameToFolderMapping
// DENY_CORS prevents cross-origin requests (T-02-10 mitigation)
void PreviewPanel::updateFileVirtualHost(const std::wstring& filePath) {
    if (!m_webview) return;

    // Extract parent directory from file path
    std::wstring dir = filePath;
    size_t pos = dir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        dir = dir.substr(0, pos);
    } else {
        return;  // no directory component — skip
    }

    wil::com_ptr<ICoreWebView2_3> webview3;
    m_webview->QueryInterface(IID_PPV_ARGS(&webview3));
    if (!webview3) return;

    // Clear existing mapping before setting new one (Pitfall 3 mitigation — timing)
    // ClearVirtualHostNameToFolderMapping is safe even if no prior mapping exists
    webview3->ClearVirtualHostNameToFolderMapping(L"file.mdpreview");
    webview3->SetVirtualHostNameToFolderMapping(
        L"file.mdpreview",
        dir.c_str(),
        COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
}

// Phase 2 Plan 04: Trigger HTML export — sets m_exportFilePath, posts {type:"export"} to JS
void PreviewPanel::triggerExport() {
    if (!m_webview || !m_webview2Initialized || m_currentFilePath.empty()) return;

    // Derive target path: replace .md extension with .html (D-10)
    std::wstring exportPath = m_currentFilePath;
    size_t dotPos = exportPath.rfind(L'.');
    if (dotPos != std::wstring::npos) {
        exportPath = exportPath.substr(0, dotPos) + L".html";
    } else {
        exportPath += L".html";
    }
    m_exportFilePath = exportPath;

    // Send export trigger to JS
    nlohmann::json j;
    j["type"] = "export";
    std::string jsonStr = j.dump();
    std::wstring wjson = Utf8ToWide(jsonStr);
    m_webview->PostWebMessageAsJson(wjson.c_str());
}

// Phase 2 Plan 04: Write UTF-8 HTML file to disk with BOM (D-10, T-02-15 mitigation)
// Path is derived from m_currentFilePath (NPPM_GETFULLCURRENTPATH) — no arbitrary path from JS.
// Silent overwrite per D-10 — no prompt.
void PreviewPanel::saveExportedHtml(const std::string& htmlUtf8) {
    if (m_exportFilePath.empty()) return;

    // Write UTF-8 file (with BOM for maximum browser compatibility)
    std::ofstream f(m_exportFilePath, std::ios::out | std::ios::binary);
    if (!f.is_open()) return;

    // UTF-8 BOM: 0xEF, 0xBB, 0xBF
    const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    f.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    f.write(htmlUtf8.c_str(), static_cast<std::streamsize>(htmlUtf8.size()));
    f.close();
    m_exportFilePath.clear();  // reset after write
}

// Phase 3: Trigger PDF export via WebView2 ICoreWebView2_7::PrintToPdf (EXPT-02, EXPT-03)
// D-06: Output path = source .md path with .pdf extension. Auto-save, no dialog, overwrite if exists.
// D-07: US Letter portrait, fixed — no user config.
// D-08: HeaderTitle = basename of .md file; ShouldPrintHeaderAndFooter=TRUE enables "Page N of M" footer.
//       Footer shows default URI + page number (ICoreWebView2PrintSettings API limitation — no custom
//       footer string; accepted compromise per CONTEXT.md D-08 and RESEARCH.md Open Questions RESOLVED).
// ZOOM RESET: CSS zoom scales PDF output. Save current zoom, reset to 1.0 before PrintToPdf,
//             restore after completion callback fires. Uses existing postZoomToJs() from Plan 03.
// CRITICAL: m_environment must be set (done in initWebView2() environment callback by Plan 03).
void PreviewPanel::triggerPdfExport() {
    if (!m_webview || !m_webview2Initialized || m_currentFilePath.empty()) return;
    if (m_printToPdfInProgress) return;  // guard: ICoreWebView2_7::PrintToPdf — only one at a time

    // D-06: Derive PDF output path — replace .md extension with .pdf
    std::wstring pdfPath = m_currentFilePath;
    size_t dotPos = pdfPath.rfind(L'.');
    if (dotPos != std::wstring::npos) {
        pdfPath = pdfPath.substr(0, dotPos) + L".pdf";
    } else {
        pdfPath += L".pdf";
    }

    // CR-01: Validate that the PDF output path stays within the same directory as the source file.
    // Prevents path traversal if m_currentFilePath contains directory traversal components
    // (e.g., from a crafted NPPM_GETFULLCURRENTPATH response on a network share).
    {
        std::wstring pdfDir = pdfPath.substr(0, pdfPath.find_last_of(L"\\/"));
        std::wstring srcDir = m_currentFilePath.substr(0, m_currentFilePath.find_last_of(L"\\/"));
        if (_wcsicmp(pdfDir.c_str(), srcDir.c_str()) != 0) return;  // reject escaped path
    }

    // D-08: Extract basename of .md file for HeaderTitle
    std::wstring basename = m_currentFilePath;
    size_t slashPos = basename.find_last_of(L"\\/");
    if (slashPos != std::wstring::npos) {
        basename = basename.substr(slashPos + 1);
    }

    // Get ICoreWebView2Environment6 from stored m_environment (set in initWebView2 env callback)
    if (!m_environment) return;
    wil::com_ptr<ICoreWebView2Environment6> env6;
    m_environment->QueryInterface(IID_PPV_ARGS(&env6));
    if (!env6) return;  // Pitfall 5: env6 null if m_environment was not stored

    // Create print settings
    wil::com_ptr<ICoreWebView2PrintSettings> printSettings;
    HRESULT hr = env6->CreatePrintSettings(&printSettings);
    if (FAILED(hr) || !printSettings) return;

    // D-07: US Letter portrait (8.5 x 11 inches). Portrait is the default orientation.
    printSettings->put_Orientation(COREWEBVIEW2_PRINT_ORIENTATION_PORTRAIT);

    // EXPT-03: Enable page numbers + header/footer
    printSettings->put_ShouldPrintHeaderAndFooter(TRUE);

    // D-08: Header = basename of .md file (e.g., "README.md")
    //       Footer = default URI + "Page N of M" (ICoreWebView2PrintSettings limitation)
    printSettings->put_HeaderTitle(basename.c_str());

    // Include background colors (code blocks, Mermaid diagrams render with backgrounds)
    printSettings->put_ShouldPrintBackgrounds(TRUE);

    // Margins: Claude's discretion per D-08 context
    printSettings->put_MarginTop(0.5);
    printSettings->put_MarginBottom(0.5);
    printSettings->put_MarginLeft(0.75);
    printSettings->put_MarginRight(0.75);

    // Get ICoreWebView2_7 from m_webview for PrintToPdf method
    wil::com_ptr<ICoreWebView2_7> webview7;
    m_webview->QueryInterface(IID_PPV_ARGS(&webview7));
    if (!webview7) return;

    // ZOOM RESET: Save current zoom level and reset to 1.0 before PrintToPdf.
    // CSS zoom (document.body.style.zoom) scales the PDF output — a user at 400% zoom would
    // produce an incorrectly scaled PDF. PostWebMessageAsJson is synchronous on the C++ side;
    // the renderer processes the zoom message before the PrintToPdf capture begins because
    // PrintToPdf is issued as a new task to the renderer after the current message queue drains.
    // After the PDF completion callback fires, restore the saved zoom level.
    m_savedZoomForPdf = m_zoomLevel;
    postZoomToJs(1.0f);  // reset to 100% zoom for accurate PDF capture

    m_printToPdfInProgress = true;
    hr = webview7->PrintToPdf(
        pdfPath.c_str(),
        printSettings.get(),
        Callback<ICoreWebView2PrintToPdfCompletedHandler>(
            [this](HRESULT errorCode, BOOL isSuccessful) -> HRESULT {
                m_printToPdfInProgress = false;

                // ZOOM RESTORE: Restore the zoom level that was active before PDF export.
                // This runs on the UI thread (WebView2 completion callbacks are marshalled
                // back to the thread that called PrintToPdf).
                postZoomToJs(m_savedZoomForPdf);

                // D-06: Silent completion — no dialog, no notification on success or failure
                UNREFERENCED_PARAMETER(errorCode);
                UNREFERENCED_PARAMETER(isSuccessful);
                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        m_printToPdfInProgress = false;  // reset on immediate failure
        postZoomToJs(m_savedZoomForPdf); // restore zoom even on immediate failure
    }
}

// Phase 2: Dispatch JS->C++ messages (T-02-04 mitigation: parse with try/catch, no shell/exec)
void PreviewPanel::handleJsMessage(const std::wstring& message) {
    // Convert wstring to UTF-8 for nlohmann parsing
    int utf8len = ::WideCharToMultiByte(CP_UTF8, 0, message.c_str(),
        static_cast<int>(message.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8Msg(static_cast<size_t>(utf8len), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, message.c_str(),
        static_cast<int>(message.size()), &utf8Msg[0], utf8len, nullptr, nullptr);

    try {
        nlohmann::json j = nlohmann::json::parse(utf8Msg);
        std::string type = j.value("type", "");
        if (type == "exportReady") {
            // j["html"] is the full standalone HTML document as a UTF-8 string (D-09)
            std::string htmlContent = j.value("html", "");
            if (!htmlContent.empty()) {
                saveExportedHtml(htmlContent);
            }
        }
        // Future message types added here (scroll feedback, etc.)
    } catch (...) {
        // Malformed message — ignore silently
    }
}

LRESULT CALLBACK PreviewPanel::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        if (wParam == PreviewPanel::DEBOUNCE_TIMER_ID) {
            ::KillTimer(hWnd, PreviewPanel::DEBOUNCE_TIMER_ID);
            PreviewPanel* self = reinterpret_cast<PreviewPanel*>(
                ::GetWindowLongPtr(hWnd, GWLP_USERDATA));
            if (self && self->m_renderPending) self->doRender();
            return 0;
        }
        break;
    case WM_SIZE: {
        PreviewPanel* self = reinterpret_cast<PreviewPanel*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (self) self->resizeWebView2();
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
        if (nmhdr->code == NM_CLICK || nmhdr->code == NM_RETURN) {
            PNMLINK link = reinterpret_cast<PNMLINK>(lParam);
            ShellExecuteW(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
        }
        break;
    }
    case WM_DESTROY:
        return 0;
    default:
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
