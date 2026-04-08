// This file is part of MarkdownPreview plugin for Notepad++
// DLL entry point and six required Notepad++ plugin exports

#include "PluginDefinition.h"
#include "Scintilla.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/) {
    switch (reasonForCall) {
    case DLL_PROCESS_ATTACH:
        pluginInit(hModule);
        break;
    case DLL_PROCESS_DETACH:
        pluginCleanUp();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData data) {
    nppData = data;
    commandMenuInit();
}

extern "C" __declspec(dllexport) const wchar_t* getName() {
    return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF) {
    *nbF = NB_FUNC;
    return funcItems;
}

extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notification) {
    switch (notification->nmhdr.code) {
    case NPPN_READY:
        // Notepad++ is fully initialized
        break;
    case NPPN_SHUTDOWN:
        // Notepad++ is about to shut down
        commandMenuCleanUp();
        break;
    case NPPN_TBMODIFICATION:
        // Toolbar modification opportunity (for toolbar button, if needed)
        break;
    default:
        break;
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/) {
    return TRUE;
}
