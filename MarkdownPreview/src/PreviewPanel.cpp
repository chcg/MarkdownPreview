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

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace Microsoft::WRL;

// Include PluginInterface.h for NppData struct definition.
// We declare nppData extern directly (defined in PluginDefinition.cpp).
// We do NOT include PluginDefinition.h here to avoid a circular dependency:
//   PreviewPanel.h is included by PluginDefinition.h, so PluginDefinition.h
//   cannot be included back into PreviewPanel.cpp.
#include "../include/PluginInterface.h"
extern NppData nppData;

static const wchar_t PANEL_CLASS_NAME[] = L"MarkdownPreviewPanel";
static const wchar_t PANEL_TITLE[] = L"Markdown Preview";
static const wchar_t MODULE_NAME[] = L"MarkdownPreview.dll";

void PreviewPanel::init(HINSTANCE hInst, HWND nppHandle) {
    m_hInst = hInst;
    m_nppHandle = nppHandle;
}

void PreviewPanel::destroy() {
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

                env->CreateCoreWebView2Controller(m_hPanel,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) return result;

                            m_controller = controller;
                            m_controller->get_CoreWebView2(&m_webview);

                            // Resize to fill panel
                            resizeWebView2();

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

                            // Navigate to welcome page (per D-11)
                            m_webview->Navigate(L"https://appassets.mdpreview/welcome.html");
                            m_webview2Initialized = true;

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
    GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
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
    if (!m_webview || !m_webview2Initialized) return;
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
    std::string jsonStr = j.dump();
    std::wstring wjson(jsonStr.begin(), jsonStr.end());
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
    std::wstring wjson(jsonStr.begin(), jsonStr.end());
    m_webview->PostWebMessageAsJson(wjson.c_str());
}

// Phase 2: Navigate back to welcome.html when a non-.md file is activated
void PreviewPanel::showIdle() {
    if (m_webview && m_webview2Initialized) {
        m_webview->Navigate(L"https://appassets.mdpreview/welcome.html");
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
            // Handled in Plan 02-04 — stub here
            // Will call: saveExportedHtml(j["html"].get<std::string>())
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
