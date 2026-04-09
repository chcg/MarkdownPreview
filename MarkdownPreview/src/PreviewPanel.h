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

    // Phase 2: render/theme/idle public interface
    void renderMarkdown(const std::wstring& filePath);   // retrieve text from Scintilla, post render message
    void setTheme(bool isDark);                           // post {type:"theme", dark:bool} message
    void scheduleRender();                                // start/restart debounce timer (called from SCN_MODIFIED)
    void showIdle();                                      // navigate back to welcome.html (non-.md file activated)
    void setNppHandle(HWND nppHandle) { m_nppHandle = nppHandle; }  // already set in init(); no-op if already set
    void scrollToLine(int line);                          // post {type:"scroll", line:N} to JS
    void updateFileVirtualHost(const std::wstring& filePath);  // map file.mdpreview to file's parent dir
    void triggerExport();                                 // post {type:"export"} to JS; sets m_exportFilePath
    void setConfigPath(const std::wstring& path) { m_configPath = path; }  // called from onNppReady()
    void applyInitialZoom(float level);  // post zoom message after WebView2 nav completes
    void triggerPdfExport();  // Phase 3: PDF export via WebView2 PrintToPdf (EXPT-02, EXPT-03)

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

    // Phase 2: render pipeline private helpers
    void doRender();                                       // actual render after debounce fires
    std::wstring getCurrentText();                         // retrieves text from active Scintilla view
    void handleJsMessage(const std::wstring& message);    // dispatch JS->C++ messages
    void saveExportedHtml(const std::string& htmlUtf8);   // write HTML string to disk (UTF-8 BOM)

    static const UINT_PTR DEBOUNCE_TIMER_ID = 1;

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

    // Phase 2: state members
    bool m_renderPending = false;
    bool m_isDark = false;
    std::wstring m_currentFilePath;
    std::wstring m_pendingFilePath;  // stores filePath when renderMarkdown() is called before WebView2 is ready
    std::wstring m_exportFilePath;  // set before sending export trigger; used in exportReady handler
    EventRegistrationToken m_webMessageReceivedToken = {};

    // Phase 3: zoom controls (THME-04, D-04, D-05)
    float m_zoomLevel = 1.0f;
    EventRegistrationToken m_accelKeyToken = {};
    EventRegistrationToken m_navigationCompletedToken = {};  // WR-03: initial zoom posted after nav completes
    std::wstring m_configPath;  // stored for settings save in AcceleratorKeyPressed handler

    // Phase 3: environment pointer — required for PDF export (Plan 04)
    wil::com_ptr<ICoreWebView2Environment> m_environment;

    bool m_printToPdfInProgress = false;  // guard: only one PrintToPdf in flight at a time
    float m_savedZoomForPdf = 1.0f;       // zoom level saved before PDF export, restored after

    void postZoomToJs(float level);  // post {type:"zoom", level:N} to WebView2
};
