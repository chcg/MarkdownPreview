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
    // Guard the entire dispatch against both C++ exceptions and hardware SEH faults.
    // beNotified() is called from Notepad++'s own WndProc — any unhandled exception
    // crosses the extern "C" frame boundary and calls std::terminate(), killing NPP.
    //
    // We use try/catch(...) here (not __try/__except) because:
    //   - The project is compiled with /EHa (ExceptionHandling=Async in the vcxproj).
    //   - Under /EHa, catch(...) catches BOTH C++ exceptions AND hardware SEH faults
    //     (access violations, stack overflow, etc.).
    //   - __try/__except under /EHsc does NOT catch C++ exceptions, and LTCG (Release
    //     build WholeProgramOptimization) can inline functions with C++ objects into a
    //     __try body, silently generating broken exception tables (C2712 bypass).
    //   - try/catch(...) with /EHa is correct in all configurations, does not have the
    //     C++ object restriction of __try/__except, and works cleanly with LTCG.
    try {
        switch (notification->nmhdr.code) {
        case NPPN_READY:
            // Notepad++ is fully initialized — load settings and init panel
            onNppReady();
            break;
        case NPPN_SHUTDOWN:
            // Notepad++ is about to shut down — persist settings
            onNppShutdown();
            commandMenuCleanUp();
            break;
        case NPPN_TBMODIFICATION:
            // Toolbar modification opportunity (for toolbar button, if needed)
            break;
        case NPPN_BUFFERACTIVATED:
            // File switched or opened — auto-open preview for .md files (REND-01)
            onBufferActivated(notification->nmhdr.idFrom);
            break;
        case NPPN_DARKMODECHANGED:
            // Notepad++ dark/light mode toggled (THME-03)
            onDarkModeChanged();
            break;
        case SCN_MODIFIED:
            // Text inserted or deleted — schedule debounced re-render (REND-02)
            onScnModified(notification);
            break;
        case SCN_UPDATEUI:
            // Caret/scroll position changed — scroll sync (Plan 02-03)
            onScnUpdateUi(notification);
            break;
        default:
            break;
        }
    } catch (...) {
        // Swallow all exceptions (C++ and SEH under /EHa) at the plugin boundary.
        // Losing one render/scroll update is far better than crashing NPP.
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/) {
    return TRUE;
}
