// This file is part of MarkdownPreview plugin for Notepad++
// Dockable preview panel - creation, registration, show/hide toggle

#include "PreviewPanel.h"
#include "Docking.h"
#include "Notepad_plus_msgs.h"

static const wchar_t PANEL_CLASS_NAME[] = L"MarkdownPreviewPanel";
static const wchar_t PANEL_TITLE[] = L"Markdown Preview";
static const wchar_t MODULE_NAME[] = L"MarkdownPreview.dll";

void PreviewPanel::init(HINSTANCE hInst, HWND nppHandle) {
    m_hInst = hInst;
    m_nppHandle = nppHandle;
}

void PreviewPanel::destroy() {
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

LRESULT CALLBACK PreviewPanel::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        // Will be used for WebView2 resize in Plan 03
        return 0;
    case WM_DESTROY:
        return 0;
    default:
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }
}
