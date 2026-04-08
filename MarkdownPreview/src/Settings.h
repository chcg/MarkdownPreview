// This file is part of MarkdownPreview plugin for Notepad++
// JSON settings persistence

#pragma once

#include <string>

struct Settings {
    bool panelVisible = false;

    void load(const std::wstring& configPath);
    void save(const std::wstring& configPath);
};
