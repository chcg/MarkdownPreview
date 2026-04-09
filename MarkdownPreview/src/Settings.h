// This file is part of MarkdownPreview plugin for Notepad++
// JSON settings persistence

#pragma once

#include <string>

struct Settings {
    bool panelVisible = false;
    float zoomLevel = 1.0f;  // D-05: global zoom level, 0.8-8.0, default 1.0 (100%)

    void load(const std::wstring& configPath);
    void save(const std::wstring& configPath);
};
