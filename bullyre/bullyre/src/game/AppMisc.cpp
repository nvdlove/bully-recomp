// Small pieces of the application layer: the RenderWare-style event handler's wrapper,
// the thread checks, the version from Config/version.cfg.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace c01app {

using GetCurrentThreadId_t = uint32_t(__stdcall*)();
GAME_VAR(GetCurrentThreadId_t, Import_GetCurrentThreadId, 0x008FC180);

GAME_FN(void, __thiscall, App_Fn_4107F0, 0x004107F0, void* app);
GAME_FN(void, __thiscall, App_Fn_410670, 0x00410670, void* app);
// (returns a byte in al; the caller compares all of eax)
GAME_FN(int, __cdecl, AppEventHandler, 0x0043B8D0, int event, void* param);
GAME_FN(int, __cdecl, File_OpenFile, 0x0042D260, const char* name, const char* mode, int);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, int file, void* buffer, int size);
GAME_FN(int, __cdecl, File_CloseFile, 0x0042D450, int file);
inline int __cdecl AtoI(const char* s) { return (int)crt::Atol(s); }
inline const char* const readMode = reinterpret_cast<const char*>(0x008FF548);   // "r"

// 0x410BF0 (thiscall): the two halves of the application's scene release
void __fastcall App_ReleaseScene(void* app, re::Edx) {
    App_Fn_4107F0(app);
    App_Fn_410670(app);
}

// 0x410C00: GTA's RsEventHandler: the application's handler, and the events it leaves
// unprocessed (2) that count as processed anyway
int __cdecl RsEventHandler(int event, void* param) {
    int result = AppEventHandler(event, param);
    if (result == 2) {
        switch (event) {
        case 1:
        case 3:
        case 10:
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x25:
            return 0;
        default:
            return 2;
        }
    }
    return result;
}

GAME_VAR(uint32_t, ms_secondThreadId, 0x00BCC550);
GAME_VAR(uint32_t, ms_mainThreadId, 0x00BCC554);
GAME_VAR(uint8_t, ms_bMainThreadSet, 0x00BCC558);

// 0x411670: the calling thread is the main one
void __cdecl SetMainThread() {
    ms_bMainThreadSet = 1;
    ms_mainThreadId = Import_GetCurrentThreadId();
}

// 0x411690
uint8_t __cdecl IsMainThread() {
    return ms_mainThreadId == Import_GetCurrentThreadId();
}

// 0x4116B0: the calling thread is the other one (set elsewhere; 0 before)
uint32_t __cdecl IsSecondThread() {
    if (ms_secondThreadId != 0 && ms_secondThreadId == Import_GetCurrentThreadId()) {
        return 1;
    }
    return 0;
}

GAME_VAR(int32_t, ms_versionMajor, 0x00BCC560);
GAME_VAR(int32_t, ms_versionMinor, 0x00BCC55C);

// 0x411F60: Config/version.cfg: the major version on the first line, the minor on the
// second (the major is then forced to 200)
void __cdecl ReadVersion() {
    int file = File_OpenFile("Config/version.cfg", readMode, 1);
    if (file < 0) {
        return;
    }
    // the game's stack: major[8], minor[8], the file's first 16 bytes (a long line
    // runs on into the next array, as in the game)
    char stack[32];
    char* major = stack;
    char* minor = stack + 8;
    char* text = stack + 16;
    File_Read(file, text, 16);
    int majorLength = 0;
    int minorLength = 0;
    bool firstLine = true;
    for (int i = 0; i < 16; i++) {
        char c = text[i];
        if (static_cast<uint8_t>(c) == 0xFF) {
            break;
        }
        if (c == '\n') {
            if (!firstLine) {
                break;
            }
            firstLine = false;
        } else if (c != '\r') {
            if (firstLine) {
                major[majorLength++] = c;
            } else {
                minor[minorLength++] = c;
            }
        }
    }
    major[majorLength] = 0;
    minor[minorLength] = 0;
    ms_versionMajor = AtoI(major);
    ms_versionMinor = AtoI(minor);
    ms_versionMajor = 200;
    File_CloseFile(file);
}

// 0x4120C0: major / 1000 + minor
float __cdecl GetVersion() {
    return static_cast<float>(ms_versionMajor / 1000.0 + ms_versionMinor);
}

} // namespace c01app

// ---- registration ----------------------------------------------------------------------

#define APPMISC_REVERSED_C01(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01app {
APPMISC_REVERSED_C01("BullyApp::ReleaseScene", 0x00410BF0, &App_ReleaseScene);
APPMISC_REVERSED_C01("RsEventHandler", 0x00410C00, &RsEventHandler);
APPMISC_REVERSED_C01("Threads::SetMainThread", 0x00411670, &SetMainThread);
APPMISC_REVERSED_C01("Threads::IsMainThread", 0x00411690, &IsMainThread);
APPMISC_REVERSED_C01("Threads::IsSecondThread", 0x004116B0, &IsSecondThread);
APPMISC_REVERSED_C01("Version::Read", 0x00411F60, &ReadVersion);
APPMISC_REVERSED_C01("Version::Get", 0x004120C0, &GetVersion);
} // namespace c01app
