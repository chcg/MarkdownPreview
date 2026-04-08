// This file is part of MarkdownPreview plugin for Notepad++
// JSON settings load/save using nlohmann/json

#include "Settings.h"
#include <nlohmann/json.hpp>
#include <fstream>

void Settings::load(const std::wstring& configPath) {
    std::ifstream f(configPath);
    if (!f.is_open()) return;
    try {
        nlohmann::json j = nlohmann::json::parse(f);
        panelVisible = j.value("panelVisible", false);
    } catch (...) {
        // T-01-04 mitigation: use defaults on parse error
        // Plugin continues to function with default settings
    }
}

void Settings::save(const std::wstring& configPath) {
    nlohmann::json j;
    j["panelVisible"] = panelVisible;
    std::ofstream f(configPath);
    if (f.is_open()) {
        f << j.dump(2);
    }
}
