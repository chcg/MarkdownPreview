// This file is part of MarkdownPreview plugin for Notepad++

#pragma once

#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"
#include "PreviewPanel.h"
#include "Settings.h"

const int NB_FUNC = 1;  // One menu item: Toggle Preview
const wchar_t PLUGIN_NAME[] = L"MarkdownPreview";

// External data set by PluginMain.cpp
extern NppData nppData;
extern FuncItem funcItems[NB_FUNC];
extern PreviewPanel g_previewPanel;
extern Settings g_settings;

// Plugin lifecycle
void pluginInit(HANDLE hModule);
void pluginCleanUp();
void commandMenuInit();
void commandMenuCleanUp();

// Notification handler called from PluginMain.cpp
void onNppReady();
void onNppShutdown();

// Menu commands
void togglePreview();
