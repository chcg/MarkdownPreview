// This file is part of MarkdownPreview plugin for Notepad++
// Dockable preview panel - WebView2 initialization, runtime detection, fallback UI

#include "PreviewPanel.h"
#include "Docking.h"
#include "Notepad_plus_msgs.h"

#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wrl.h>
#include <WebView2EnvironmentOptions.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace Microsoft::WRL;

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
    if (m_hPanel) {
        ::DestroyWindow(m_hPanel);
        m_hPanel = nullptr;
    }
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
        m_isRegistered = true;
    }

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

LRESULT CALLBACK PreviewPanel::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
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
