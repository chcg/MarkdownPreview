// This file is part of MarkdownPreview plugin for Notepad++

#pragma once

#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"
#include "Scintilla.h"
#include "PreviewPanel.h"
#include "Settings.h"

const int NB_FUNC = 6;  // Toggle Preview + Export as HTML + Export as PDF + Zoom In + Zoom Out + Zoom Reset
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

// Phase 2 notification handlers (called from PluginMain.cpp beNotified)
void onBufferActivated(UINT_PTR bufferId);
void onDarkModeChanged();
void onScnModified(SCNotification* notification);
void onScnUpdateUi(SCNotification* notification);

// Menu commands
void togglePreview();
void exportMarkdown();   // menu command: triggers HTML export via WebView2 JS
void exportMarkdownAsPdf();  // menu command: triggers PDF export via WebView2 PrintToPdf
void zoomInPreview();    // menu command: increase preview zoom by 10% (Ctrl+=)
void zoomOutPreview();   // menu command: decrease preview zoom by 10% (Ctrl+-)
void zoomResetPreview(); // menu command: reset preview zoom to 100% (Ctrl+0, D-04)
