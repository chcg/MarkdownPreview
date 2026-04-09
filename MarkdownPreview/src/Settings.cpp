// This file is part of MarkdownPreview plugin for Notepad++
// JSON settings load/save using nlohmann/json

#include "Settings.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <windows.h>

void Settings::load(const std::wstring& configPath) {
    std::ifstream f(configPath);
    if (!f.is_open()) return;
    try {
        nlohmann::json j = nlohmann::json::parse(f);
        panelVisible = j.value("panelVisible", false);
        zoomLevel = j.value("zoomLevel", 1.0f);
    } catch (...) {
        // T-01-04 mitigation: use defaults on parse error
        // Plugin continues to function with default settings
    }
}

void Settings::save(const std::wstring& configPath) {
    if (configPath.empty()) return;

    // Ensure the parent directory exists — NPP config dir should exist,
    // but create it defensively in case it doesn't
    std::wstring dir = configPath;
    size_t pos = dir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        dir = dir.substr(0, pos);
        ::CreateDirectoryW(dir.c_str(), nullptr);  // no-op if exists
    }

    nlohmann::json j;
    j["panelVisible"] = panelVisible;
    j["zoomLevel"] = zoomLevel;
    std::ofstream f(configPath);
    if (f.is_open()) {
        f << j.dump(2);
    }
}
