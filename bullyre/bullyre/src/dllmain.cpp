// bullyre.dll: loaded by the game's first instruction (the launcher patches its entry
// point to LoadLibraryA it), before any of the game's own code has run.

#include <windows.h>

#include "reversed.h"

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        re::Startup();
    }
    return TRUE;
}
