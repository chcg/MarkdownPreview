// This file is part of MarkdownPreview plugin for Notepad++
// Dockable preview panel management with WebView2 rendering

#pragma once

#include <windows.h>
#include <string>
#include <WebView2.h>
#include <wrl.h>
#include <wil/com.h>

class PreviewPanel {
public:
    void init(HINSTANCE hInst, HWND nppHandle);
    void destroy();
    void toggle(int cmdID);
    bool isVisible() const;
    HWND getHwnd() const;

private:
    void createHostWindow();
    void registerPanel();
    static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // WebView2 methods
    bool checkWebView2Available();
    void initWebView2();
    void showWebView2MissingFallback();
    void resizeWebView2();
    std::wstring getAssetsPath();
    std::wstring getUserDataPath();

    HINSTANCE m_hInst = nullptr;
    HWND m_nppHandle = nullptr;
    HWND m_hPanel = nullptr;
    int m_cmdID = 0;
    bool m_isRegistered = false;
    bool m_isVisible = false;

    // WebView2 members
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2> m_webview;
    HWND m_hFallback = nullptr;  // SysLink control when WebView2 missing
    bool m_webview2Available = false;
    bool m_webview2Initialized = false;
};
