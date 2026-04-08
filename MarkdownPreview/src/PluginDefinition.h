// This file is part of MarkdownPreview plugin for Notepad++

#pragma once

#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"

const int NB_FUNC = 1;  // One menu item: Toggle Preview
const wchar_t PLUGIN_NAME[] = L"MarkdownPreview";

// External data set by PluginMain.cpp
extern NppData nppData;
extern FuncItem funcItems[NB_FUNC];

// Plugin lifecycle
void pluginInit(HANDLE hModule);
void pluginCleanUp();
void commandMenuInit();
void commandMenuCleanUp();

// Menu commands
void togglePreview();
