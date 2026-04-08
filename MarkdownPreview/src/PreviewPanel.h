// This file is part of MarkdownPreview plugin for Notepad++
// Dockable preview panel management

#pragma once

#include <windows.h>

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

    HINSTANCE m_hInst = nullptr;
    HWND m_nppHandle = nullptr;
    HWND m_hPanel = nullptr;
    int m_cmdID = 0;
    bool m_isRegistered = false;
    bool m_isVisible = false;
};
