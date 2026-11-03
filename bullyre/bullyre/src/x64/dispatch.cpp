// The 64-bit dispatch: see dispatch.h.

#include "x64/dispatch.h"

#include <windows.h>
#include <objbase.h>
#include <intrin.h>

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "reversed.h"
#include "runflags.h"
#include "x64/lowheap.h"

namespace x64 {
namespace {

constexpr uintptr_t kImageBase = 0x400000;
constexpr uintptr_t kReserveLo = 0x401000;    // src/x64/reserve64.asm: the game's image range
constexpr uintptr_t kReserveHi = 0x20F3000;

struct Section {
    std::string name;
    uint32_t va;        // absolute
    uint32_t vsize;
    uint32_t raw;       // file offset
    uint32_t rawSize;
};

std::vector<uint8_t> g_file;
std::vector<Section> g_sections;
uint32_t g_textLo = 0, g_textHi = 0;
uint32_t g_importRva = 0;
FILE* g_log = nullptr;
volatile uint32_t g_lastMissing = 0;

uint32_t Read32(size_t offset) {
    uint32_t v;
    std::memcpy(&v, &g_file[offset], 4);
    return v;
}
uint16_t Read16(size_t offset) {
    uint16_t v;
    std::memcpy(&v, &g_file[offset], 2);
    return v;
}

// the file bytes of an absolute address (n of them inside one section's raw data), or nullptr
const uint8_t* FileAt(uint32_t address, uint32_t n) {
    for (const Section& s : g_sections) {
        if (address >= s.va && address + n <= s.va + std::min(s.vsize, s.rawSize)) {
            return &g_file[s.raw + (address - s.va)];
        }
    }
    return nullptr;
}

// Data the linker put in the game's .text, which the game hands to the system: dinput8.lib's
// static data formats' object arrays (c_dfDIKeyboard's at 0x8D8A10, 256 of 0x10; c_dfDIMouse's
// at 0x8D9A10, 7; between them at 0x8D9A80 the import thunk DirectInput8Create; c_dfDIMouse2's at
// 0x8D9A90, 11; code again at 0x8D9B40). Ghidra's list has
// "functions" there; the dispatch's int3 fill and stubs went over them and SetDataFormat refused
// the keyboard and the mouse (no real input, 2026-10-31). Kept as the game's bytes, no start.
struct TextData {
    uint32_t lo, hi;
};
constexpr TextData kTextData[] = {{0x008D8A10, 0x008D9A80}, {0x008D9A90, 0x008D9B40}};

bool InTextData(uint32_t address) {
    for (const TextData& d : kTextData) {
        if (address >= d.lo && address < d.hi) {
            return true;
        }
    }
    return false;
}

// Islands: the longer pieces a 5-byte stub jumps to (traps; jumps too far for rel32). They're
// pages of the low heap (below 2 GB: src/x64/lowheap.cpp), so a rel32 jump from the stubs
// reaches them.
uint8_t* g_islands = nullptr;
size_t g_islandsUsed = 0;
constexpr size_t kIslandsSize = 4u << 20;

uint8_t* Island(size_t n) {
    if (!g_islands) {
        g_islands = static_cast<uint8_t*>(re::low::AllocPages(kIslandsSize, PAGE_EXECUTE_READWRITE));
    }
    if (!g_islands || g_islandsUsed + n > kIslandsSize) {
        return nullptr;
    }
    uint8_t* p = g_islands + g_islandsUsed;
    g_islandsUsed += (n + 15) & ~size_t(15);
    return p;
}

bool FitsRel32(uintptr_t from, uintptr_t to) {
    intptr_t rel = intptr_t(to) - intptr_t(from + 5);
    return rel >= INT32_MIN && rel <= INT32_MAX;
}

void Rel32Jump(uint8_t* at, uintptr_t to) {
    at[0] = 0xE9;
    int32_t rel = int32_t(intptr_t(to) - intptr_t(reinterpret_cast<uintptr_t>(at) + 5));
    std::memcpy(at + 1, &rel, 4);
}

// a 5-byte jump at `at` to `to` (through an island's absolute jump when rel32 doesn't reach)
bool WriteJump(uintptr_t at, uintptr_t to) {
    uint8_t* p = reinterpret_cast<uint8_t*>(at);
    if (FitsRel32(at, to)) {
        Rel32Jump(p, to);
        return true;
    }
    uint8_t* island = Island(14);   // jmp qword ptr [rip+0]; dq to
    if (!island) {
        return false;
    }
    island[0] = 0xFF;
    island[1] = 0x25;
    std::memset(island + 2, 0, 4);
    std::memcpy(island + 6, &to, 8);
    Rel32Jump(p, reinterpret_cast<uintptr_t>(island));
    return true;
}

// an island that jumps to `to` wherever it is (jmp qword ptr [rip+0]; dq to): below 2 GB itself,
// so a 4-byte slot of the game's can hold it
uint8_t* AbsoluteIsland(uintptr_t to) {
    uint8_t* island = Island(14);
    if (island) {
        island[0] = 0xFF;
        island[1] = 0x25;
        std::memset(island + 2, 0, 4);
        std::memcpy(island + 6, &to, 8);
    }
    return island;
}

volatile bool g_recoverable = false;
volatile bool g_lenient = false;
volatile long g_lenientCount = 0;

// The trap: reached by jumps only, so the return address on the stack is the caller's. In a
// guarded run (SetTrapsRecoverable) it raises kMissingFunction instead of exiting.
extern "C" __declspec(noinline) void MissingFunction(uint32_t address) {
    g_lastMissing = address;
    void* caller = _ReturnAddress();
    if (g_lenient) {
        // (a survey: logged once per address, then 0 back to the caller, which goes on)
        static std::unordered_set<uint32_t>* seen = new std::unordered_set<uint32_t>();
        if (seen->insert(address).second) {
            _InterlockedIncrement(&g_lenientCount);
            std::fprintf(stderr, "x64 dispatch (lenient): the game's function 0x%08X has no body (called from %p): 0 back\n",
                         unsigned(address), caller);
            if (g_log) {
                std::fprintf(g_log, "lenient: 0x%08X from %p\n", unsigned(address), caller);
                std::fflush(g_log);
            }
        }
        return;
    }
    if (g_recoverable) {
        ULONG_PTR info[2] = {address, reinterpret_cast<ULONG_PTR>(caller)};
        RaiseException(kMissingFunction, 0, 2, info);
    }
    std::fprintf(stderr, "x64 dispatch: the game's function 0x%08X has no body (called from %p)\n", unsigned(address), caller);
    if (g_log) {
        std::fprintf(g_log, "x64 dispatch: the game's function 0x%08X has no body (called from %p)\n", unsigned(address), caller);
        std::fflush(g_log);
    }
    std::fflush(stderr);
    ExitProcess(3);
}

bool WriteTrap(uintptr_t at) {
    uint8_t* island = Island(10);   // mov ecx, address; jmp MissingFunction
    if (!island) {
        return false;
    }
    island[0] = 0xB9;
    uint32_t a = uint32_t(at);
    std::memcpy(island + 1, &a, 4);
    uintptr_t target = reinterpret_cast<uintptr_t>(&MissingFunction);
    if (!FitsRel32(reinterpret_cast<uintptr_t>(island) + 5, target)) {
        return false;
    }
    Rel32Jump(island + 5, target);
    Rel32Jump(reinterpret_cast<uint8_t*>(at), reinterpret_cast<uintptr_t>(island));
    return true;
}

// The import thunks' targets: IAT slot address -> the 64-bit export of the same name.
struct Import {
    std::string dll;
    std::string name;   // empty: by ordinal
    uint16_t ordinal = 0;
};
std::unordered_map<uint32_t, Import> g_imports;

void ReadImports() {
    if (!g_importRva) {
        return;
    }
    const uint8_t* desc = FileAt(uint32_t(kImageBase + g_importRva), 20);
    for (; desc && Read32(desc - g_file.data() + 12) != 0; desc += 20) {
        size_t d = size_t(desc - g_file.data());
        uint32_t lookup = Read32(d + 0) ? Read32(d + 0) : Read32(d + 16);
        uint32_t iat = Read32(d + 16);
        const uint8_t* nameAt = FileAt(uint32_t(kImageBase + Read32(d + 12)), 1);
        std::string dll = nameAt ? reinterpret_cast<const char*>(nameAt) : "?";
        for (uint32_t i = 0;; i++) {
            const uint8_t* entry = FileAt(uint32_t(kImageBase + lookup + i * 4), 4);
            if (!entry) {
                break;
            }
            uint32_t v = Read32(size_t(entry - g_file.data()));
            if (v == 0) {
                break;
            }
            Import imp;
            imp.dll = dll;
            if (v & 0x80000000u) {
                imp.ordinal = uint16_t(v & 0xFFFF);
            } else {
                const uint8_t* byName = FileAt(uint32_t(kImageBase + v + 2), 1);
                imp.name = byName ? reinterpret_cast<const char*>(byName) : "?";
            }
            g_imports[uint32_t(kImageBase + iat + i * 4)] = imp;
        }
    }
}

// The 32-bit kernel32 exports that x64's doesn't (they're intrinsics there): ours, on the game's
// 32-bit longs.
LONG WINAPI Interlocked_Increment(volatile LONG* p) {
    return _InterlockedIncrement(p);
}
LONG WINAPI Interlocked_Decrement(volatile LONG* p) {
    return _InterlockedDecrement(p);
}
LONG WINAPI Interlocked_Exchange(volatile LONG* p, LONG v) {
    return _InterlockedExchange(p, v);
}
LONG WINAPI Interlocked_CompareExchange(volatile LONG* p, LONG v, LONG comparand) {
    return _InterlockedCompareExchange(p, v, comparand);
}
LONG WINAPI Interlocked_ExchangeAdd(volatile LONG* p, LONG v) {
    return _InterlockedExchangeAdd(p, v);
}

void* Replacement(const Import& imp) {
    static const struct {
        const char* name;
        void* fn;
    } table[] = {
        {"InterlockedIncrement", reinterpret_cast<void*>(&Interlocked_Increment)},
        {"InterlockedDecrement", reinterpret_cast<void*>(&Interlocked_Decrement)},
        {"InterlockedExchange", reinterpret_cast<void*>(&Interlocked_Exchange)},
        {"InterlockedCompareExchange", reinterpret_cast<void*>(&Interlocked_CompareExchange)},
        {"InterlockedExchangeAdd", reinterpret_cast<void*>(&Interlocked_ExchangeAdd)},
    };
    for (const auto& r : table) {
        if (imp.name == r.name) {
            return r.fn;
        }
    }
    return nullptr;
}

// Imports the bring-up watches: the process ending, and the message boxes (logged with their
// text and answered OK, so an unattended run doesn't wait on one)
void WINAPI Watched_ExitProcess(UINT code) {
    std::fprintf(stderr, "x64 dispatch: the game's code called ExitProcess(%u) from %p\n", code, _ReturnAddress());
    if (g_log) {
        std::fprintf(g_log, "x64 dispatch: the game's code called ExitProcess(%u) from %p\n", code, _ReturnAddress());
        std::fflush(g_log);
    }
    std::fflush(stderr);
    ExitProcess(code);
}
int WINAPI Watched_MessageBoxA(HWND, LPCSTR text, LPCSTR caption, UINT type) {
    std::fprintf(stderr, "x64 dispatch: message box \"%s\": \"%s\" (type 0x%X, from %p): answered OK\n", caption ? caption : "",
                 text ? text : "", type, _ReturnAddress());
    std::fflush(stderr);
    return IDOK;
}
int WINAPI Watched_MessageBoxW(HWND, LPCWSTR text, LPCWSTR caption, UINT type) {
    std::fprintf(stderr, "x64 dispatch: message box \"%ls\": \"%ls\" (type 0x%X, from %p): answered OK\n", caption ? caption : L"",
                 text ? text : L"", type, _ReturnAddress());
    std::fflush(stderr);
    return IDOK;
}

// The display mode is never changed by this program (its runs mustn't touch the user's monitor:
// the game's fullscreen sets the mode it was configured for): the change reported done.
void NoteDisplayChange(const void* from) {
    static bool noted = false;
    if (!noted) {
        noted = true;
        std::fprintf(stderr, "x64 dispatch: a display mode change (from %p) skipped: the mode stays the desktop's\n", from);
        std::fflush(stderr);
    }
}
LONG WINAPI Kept_ChangeDisplaySettingsA(void*, DWORD) {
    NoteDisplayChange(_ReturnAddress());
    return DISP_CHANGE_SUCCESSFUL;
}
LONG WINAPI Kept_ChangeDisplaySettingsExA(const char*, void*, HWND, DWORD, void*) {
    NoteDisplayChange(_ReturnAddress());
    return DISP_CHANGE_SUCCESSFUL;
}

// (re::ForceWindowed()) the game places and sizes its top-level window as a fullscreen one: at
// the screen's corner, its outer size the video mode's (SetAppActive's MoveWindow at every
// activation, ApplyVideoMode's SetWindowPos). A window with a frame is left where it is and its
// client area gets the mode's size: else the frame takes from the client area, bullygfx's
// resize feature asks the game for that smaller size, the game sets it the same way (a loop:
// the picture black at each round, the window back at the corner). A borderless one (the
// screen filled) is placed as asked.
bool Framed(HWND window) {
    LONG style = GetWindowLongA(window, GWL_STYLE);
    return (style & WS_CHILD) == 0 && (style & (WS_CAPTION | WS_THICKFRAME)) != 0;
}
void ClientSized(HWND window, int* width, int* height) {
    RECT r = {0, 0, *width, *height};
    AdjustWindowRectEx(&r, DWORD(GetWindowLongA(window, GWL_STYLE)), FALSE, DWORD(GetWindowLongA(window, GWL_EXSTYLE)));
    *width = r.right - r.left;
    *height = r.bottom - r.top;
}
BOOL WINAPI Windowed_MoveWindow(HWND window, int x, int y, int width, int height, BOOL repaint) {
    if (re::ForceWindowed() && Framed(window) && IsZoomed(window)) {
        return TRUE;   // (maximized: its size is the player's)
    }
    if (re::ForceWindowed() && Framed(window)) {
        RECT now;
        GetWindowRect(window, &now);
        ClientSized(window, &width, &height);
        return SetWindowPos(window, nullptr, now.left, now.top, width, height,
                            SWP_NOZORDER | SWP_NOACTIVATE | (repaint ? 0u : UINT(SWP_NOREDRAW)));
    }
    return MoveWindow(window, x, y, width, height, repaint);
}
BOOL WINAPI Windowed_SetWindowPos(HWND window, HWND after, int x, int y, int width, int height, UINT flags) {
    if (re::ForceWindowed() && Framed(window) && (flags & SWP_NOSIZE) == 0) {
        ClientSized(window, &width, &height);
        flags |= SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE;
        if (IsZoomed(window)) {
            flags |= SWP_NOSIZE;   // (maximized: its size is the player's)
        }
    }
    return SetWindowPos(window, after, x, y, width, height, flags);
}

// The game's top-level window: with a frame (re::ForceWindowed()) centred on the desktop's work
// area rather than at its corner; for a test run (re::Background()) shown without activation,
// the foreground never taken
HWND WINAPI Game_CreateWindowExA(DWORD exStyle, const char* cls, const char* name, DWORD style, int x, int y, int w, int h,
                                 HWND parent, HMENU menu, HINSTANCE instance, void* param) {
    if (re::ForceWindowed() && parent == nullptr && (style & WS_CHILD) == 0 && (style & (WS_CAPTION | WS_THICKFRAME)) != 0 &&
        x == 0 && y == 0) {
        RECT work;
        if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0)) {
            x = work.left + ((work.right - work.left) - w) / 2;
            y = work.top + ((work.bottom - work.top) - h) / 2;
            x = x < work.left ? work.left : x;
            y = y < work.top ? work.top : y;
        }
    }
    bool hide = re::Background() && (style & WS_VISIBLE) != 0 && parent == nullptr;
    HWND window = CreateWindowExA(exStyle, cls, name, hide ? style & ~WS_VISIBLE : style, x, y, w, h, parent, menu, instance, param);
    if (window && hide) {
        ShowWindow(window, SW_SHOWNOACTIVATE);
    }
    return window;
}
BOOL WINAPI Background_ShowWindow(HWND window, int command) {
    switch (command) {
    case SW_SHOWNORMAL:
    case SW_SHOWMAXIMIZED:
    case SW_RESTORE:
    case SW_SHOWDEFAULT:
        command = SW_SHOWNOACTIVATE;
        break;
    case SW_SHOW:
        command = SW_SHOWNA;
        break;
    default:
        break;
    }
    return ShowWindow(window, command);
}
BOOL WINAPI Background_SetForegroundWindow(HWND) {
    return TRUE;
}
HWND WINAPI Background_SetActiveWindow(HWND) {
    return GetActiveWindow();
}
BOOL WINAPI Background_BringWindowToTop(HWND) {
    return TRUE;
}

// The game's exe (LoadGameImage's, its full path): the game's GetModuleFileNameA for its own
// module (WinMain makes its folder the game's path: CFileMgr) is answered with it, so the game's
// folder is the game's wherever bullyre_x64.exe is (a launcher's folder, beside bullymenu)
char g_imagePath[MAX_PATH] = "";

DWORD WINAPI Game_GetModuleFileNameA(HMODULE module, char* out, DWORD size) {
    if ((module == nullptr || module == GetModuleHandleW(nullptr)) && g_imagePath[0] && out && size > 0) {
        size_t n = std::strlen(g_imagePath);
        if (n >= size) {
            std::memcpy(out, g_imagePath, size - 1);
            out[size - 1] = '\0';
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return size;
        }
        std::memcpy(out, g_imagePath, n + 1);
        return DWORD(n);
    }
    return GetModuleFileNameA(module, out, size);
}

// The game's LoadLibraryA (D3D9.DLL's renderer): a system DLL whose objects the game keeps gets
// its heap turned to the low heap before anything of it is made (src/x64/lowheap.cpp)
HMODULE WINAPI Game_LoadLibraryA(const char* name) {
    HMODULE m = nullptr;
    if (_stricmp(name, "D3D9.DLL") == 0) {
        // bullygfx64.dll beside the exe: the 64-bit build of the replacement renderer (the game
        // folder's d3d9.dll is the 32-bit game's). The system's d3d9 it runs on is loaded first,
        // so its heap is ours before bullygfx makes anything with it.
        wchar_t path[MAX_PATH];
        DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
        wchar_t* slash = n ? wcsrchr(path, L'\\') : nullptr;
        if (slash && size_t(slash - path) + 16 < MAX_PATH) {
            wcscpy(slash + 1, L"bullygfx64.dll");
            if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) {
                LoadLibraryExW(L"d3d9.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                re::low::KeepSystemObjectsLow();
                m = LoadLibraryW(path);
            }
        }
    }
    if (!m) {
        m = LoadLibraryA(name);
    }
    re::low::KeepSystemObjectsLow();
    return m;
}

// The game's CoCreateInstance (DirectShow's graph and renderer for the movies, WMI for the
// pads): the class's DLL (its InprocServer32) loaded first and its heap turned to the low heap,
// so the object it makes comes back below 2 GB (COM would load the DLL inside the call and make
// the object at once)
HRESULT WINAPI Game_CoCreateInstance(REFCLSID clsid, IUnknown* outer, DWORD context, REFIID iid, void** out) {
    wchar_t guid[64];
    if (StringFromGUID2(clsid, guid, 64)) {
        wchar_t key[128];
        swprintf(key, 128, L"CLSID\\%s\\InprocServer32", guid);
        wchar_t path[MAX_PATH];
        DWORD size = sizeof path;
        if (RegGetValueW(HKEY_CLASSES_ROOT, key, nullptr, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, path, &size) ==
            ERROR_SUCCESS) {
            re::low::KeepModuleObjectsLow(LoadLibraryW(path));   // (kept loaded: COM holds it anyway)
        }
    }
    return CoCreateInstance(clsid, outer, context, iid, out);
}

// The game's CoTaskMemFree: what our XACT shim hands it for COM memory (a cue's properties) is
// the low heap's in the 64-bit build
void WINAPI Game_CoTaskMemFree(void* p) {
    if (re::low::Owns(p)) {
        re::low::Free(p);
    } else {
        CoTaskMemFree(p);
    }
}

// The game's critical sections: 0x18 bytes in its layouts (x86's), 0x28 on x64. A game section
// holds, in its first 4 bytes, a pointer to a real one of ours (below 2 GB: this process isn't
// large-address-aware), made on first use. The calls the game's code makes through its import
// slots go through these; our own code's direct calls use re::GameSection (reversed.h).
void WINAPI Game_InitializeCriticalSection(CRITICAL_SECTION* section) {
    *reinterpret_cast<volatile uint32_t*>(section) = 0;
    re::GameSection(section);
}
BOOL WINAPI Game_InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION* section, DWORD) {
    Game_InitializeCriticalSection(section);
    return TRUE;
}
void WINAPI Game_EnterCriticalSection(CRITICAL_SECTION* section) {
    EnterCriticalSection(re::GameSection(section));
}
void WINAPI Game_LeaveCriticalSection(CRITICAL_SECTION* section) {
    LeaveCriticalSection(re::GameSection(section));
}
BOOL WINAPI Game_TryEnterCriticalSection(CRITICAL_SECTION* section) {
    return TryEnterCriticalSection(re::GameSection(section));
}
void WINAPI Game_DeleteCriticalSection(CRITICAL_SECTION* section) {
    re::DeleteGameSection(section);
}

void* Override(const Import& imp) {
    static const struct {
        const char* name;
        void* fn;
    } sections[] = {
        {"InitializeCriticalSection", reinterpret_cast<void*>(&Game_InitializeCriticalSection)},
        {"InitializeCriticalSectionAndSpinCount", reinterpret_cast<void*>(&Game_InitializeCriticalSectionAndSpinCount)},
        {"EnterCriticalSection", reinterpret_cast<void*>(&Game_EnterCriticalSection)},
        {"LeaveCriticalSection", reinterpret_cast<void*>(&Game_LeaveCriticalSection)},
        {"TryEnterCriticalSection", reinterpret_cast<void*>(&Game_TryEnterCriticalSection)},
        {"DeleteCriticalSection", reinterpret_cast<void*>(&Game_DeleteCriticalSection)},
    };
    for (const auto& s : sections) {
        if (imp.name == s.name) {
            return s.fn;
        }
    }
    if (imp.name == "ExitProcess") {
        return reinterpret_cast<void*>(&Watched_ExitProcess);
    }
    if (imp.name == "LoadLibraryA") {
        return reinterpret_cast<void*>(&Game_LoadLibraryA);
    }
    if (imp.name == "GetModuleFileNameA") {
        return reinterpret_cast<void*>(&Game_GetModuleFileNameA);
    }
    if (imp.name == "CoCreateInstance") {
        return reinterpret_cast<void*>(&Game_CoCreateInstance);
    }
    if (imp.name == "CoTaskMemFree") {
        return reinterpret_cast<void*>(&Game_CoTaskMemFree);
    }
    if (imp.name == "MessageBoxA") {
        return reinterpret_cast<void*>(&Watched_MessageBoxA);
    }
    if (imp.name == "MessageBoxW") {
        return reinterpret_cast<void*>(&Watched_MessageBoxW);
    }
    if (imp.name == "ChangeDisplaySettingsA" || imp.name == "ChangeDisplaySettingsW") {
        return reinterpret_cast<void*>(&Kept_ChangeDisplaySettingsA);
    }
    if (imp.name == "ChangeDisplaySettingsExA" || imp.name == "ChangeDisplaySettingsExW") {
        return reinterpret_cast<void*>(&Kept_ChangeDisplaySettingsExA);
    }
    if (imp.name == "CreateWindowExA" && (re::ForceWindowed() || re::Background())) {
        return reinterpret_cast<void*>(&Game_CreateWindowExA);
    }
    if (re::ForceWindowed()) {
        if (imp.name == "MoveWindow") {
            return reinterpret_cast<void*>(&Windowed_MoveWindow);
        }
        if (imp.name == "SetWindowPos") {
            return reinterpret_cast<void*>(&Windowed_SetWindowPos);
        }
    }
    if (re::Background()) {
        static const struct {
            const char* name;
            void* fn;
        } kBackground[] = {
            {"ShowWindow", reinterpret_cast<void*>(&Background_ShowWindow)},
            {"SetForegroundWindow", reinterpret_cast<void*>(&Background_SetForegroundWindow)},
            {"SetActiveWindow", reinterpret_cast<void*>(&Background_SetActiveWindow)},
            {"BringWindowToTop", reinterpret_cast<void*>(&Background_BringWindowToTop)},
        };
        for (const auto& b : kBackground) {
            if (imp.name == b.name) {
                return b.fn;
            }
        }
    }
    return nullptr;
}

void* ResolveImport(const Import& imp) {
    if (void* watched = Override(imp)) {
        return watched;
    }
    HMODULE m = LoadLibraryA(imp.dll.c_str());
    void* proc = nullptr;
    if (m) {
        proc = reinterpret_cast<void*>(imp.name.empty() ? GetProcAddress(m, MAKEINTRESOURCEA(imp.ordinal)) : GetProcAddress(m, imp.name.c_str()));
    }
    return proc ? proc : Replacement(imp);
}

} // namespace

bool LoadGameImage(const char* path) {
    FILE* f = std::fopen(path, "rb");
    if (!f) {
        return false;
    }
    if (GetFullPathNameA(path, MAX_PATH, g_imagePath, nullptr) == 0) {
        std::snprintf(g_imagePath, sizeof g_imagePath, "%s", path);
    }
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    g_file.resize(size_t(size));
    bool ok = std::fread(g_file.data(), 1, g_file.size(), f) == g_file.size();
    std::fclose(f);
    if (!ok || g_file.size() < 0x400) {
        return false;
    }
    uint32_t pe = Read32(0x3C);
    uint16_t count = Read16(pe + 6);
    uint16_t optional = Read16(pe + 20);
    uint32_t base = Read32(pe + 24 + 28);
    if (base != kImageBase) {
        return false;
    }
    g_importRva = Read32(pe + 24 + 96 + 1 * 8);
    for (uint16_t i = 0; i < count; i++) {
        size_t o = pe + 24 + optional + i * 40;
        Section s;
        s.name.assign(reinterpret_cast<const char*>(&g_file[o]), strnlen(reinterpret_cast<const char*>(&g_file[o]), 8));
        s.vsize = Read32(o + 8);
        s.va = uint32_t(kImageBase + Read32(o + 12));
        s.rawSize = Read32(o + 16);
        s.raw = Read32(o + 20);
        if (s.name == ".text") {
            g_textLo = s.va;
            g_textHi = s.va + s.vsize;
        }
        g_sections.push_back(s);
    }
    ReadImports();
    return g_textLo != 0;
}

DispatchCounts InstallDispatch(const char* functionList, FILE* log, const char* namedStarts) {
    g_log = log;
    DispatchCounts counts;
    // the function starts: the exe's (Ghidra's list and the ones named in our source: built in,
    // game_starts.inc; the lists given as files added), and every registered address
    static const uint32_t kGameStarts[] = {
#include "x64/game_starts.inc"
    };
    std::vector<uint32_t> starts(std::begin(kGameStarts), std::end(kGameStarts));
    for (const char* list : {functionList, namedStarts}) {
        FILE* f = list ? std::fopen(list, "r") : nullptr;
        if (!f) {
            continue;
        }
        char line[512];
        while (std::fgets(line, sizeof line, f)) {
            if (line[0] == '0' && line[1] == 'x') {
                starts.push_back(uint32_t(std::strtoul(line, nullptr, 16)));
            }
        }
        std::fclose(f);
    }
    for (size_t i = 0; i < re::RegisteredCount(); i++) {
        starts.push_back(uint32_t(re::Registered(i).address));
    }
    std::sort(starts.begin(), starts.end());
    starts.erase(std::unique(starts.begin(), starts.end()), starts.end());
    starts.erase(std::remove_if(starts.begin(), starts.end(), [](uint32_t a) { return a < g_textLo || a >= g_textHi || InTextData(a); }),
                 starts.end());

    DWORD old;
    VirtualProtect(reinterpret_cast<void*>(kReserveLo), g_textHi - kReserveLo, PAGE_EXECUTE_READWRITE, &old);
    std::memset(reinterpret_cast<void*>(kReserveLo), 0xCC, g_textHi - kReserveLo);

    std::unordered_map<uint32_t, std::pair<void*, void*>> bodies;   // address -> (ours, entry)
    for (size_t i = 0; i < re::RegisteredCount(); i++) {
        const re::Function& f = re::Registered(i);
        if (f.ours.entry) {
            bodies[uint32_t(f.address)] = {f.ours.ours, f.ours.entry};
        }
    }

    // the starts that get a stub (room for 5 bytes): a jump thunk aliases its target only when
    // the target is one of them (some of the exe's "functions" jump into the middle of another)
    std::unordered_set<uint32_t> stubbed;
    for (size_t i = 0; i < starts.size(); i++) {
        uint32_t next = i + 1 < starts.size() ? starts[i + 1] : g_textHi;
        if (next - starts[i] >= 5) {
            stubbed.insert(starts[i]);
        }
    }
    auto alias = [&](uint32_t at, uint32_t target) {
        if (stubbed.count(target)) {
            return WriteJump(at, target);
        }
        if (log) {
            std::fprintf(log, "x64 dispatch: 0x%08X jumps into 0x%08X, no function start: a trap\n", unsigned(at), unsigned(target));
        }
        return false;
    };
    std::vector<std::pair<uint32_t, uint32_t>> trapped;
    std::vector<std::pair<uint32_t, char>> kinds;   // b body, j jump thunk, i import, t trap, c too close
    for (size_t i = 0; i < starts.size(); i++) {
        uint32_t at = starts[i];
        uint32_t next = i + 1 < starts.size() ? starts[i + 1] : g_textHi;
        if (next - at < 5) {
            counts.tooClose++;
            kinds.push_back({at, 'c'});
            if (log) {
                std::fprintf(log, "x64 dispatch: 0x%08X is %u bytes from the next start: no stub\n", unsigned(at), unsigned(next - at));
            }
            continue;
        }
        counts.functions++;
        auto b = bodies.find(at);
        if (b != bodies.end()) {
            if (WriteJump(at, reinterpret_cast<uintptr_t>(b->second.second))) {
                counts.ours++;
                kinds.push_back({at, 'b'});
                if (b->second.second != b->second.first) {
                    counts.adapters++;
                }
                continue;
            }
        }
        // the game's x86 bytes there: a jump thunk, an import thunk, or neither
        const uint8_t* code = FileAt(at, 6);
        if (code && code[0] == 0xE9) {
            int32_t rel;
            std::memcpy(&rel, code + 1, 4);
            uint32_t target = uint32_t(at + 5 + rel);
            if (target >= g_textLo && target < g_textHi && alias(at, target)) {
                counts.thunks++;
                kinds.push_back({at, 'j'});
                continue;
            }
        } else if (code && code[0] == 0xEB) {
            uint32_t target = uint32_t(at + 2 + int8_t(code[1]));
            if (target >= g_textLo && target < g_textHi && alias(at, target)) {
                counts.thunks++;
                kinds.push_back({at, 'j'});
                continue;
            }
        } else if (code && code[0] == 0xFF && code[1] == 0x25) {
            uint32_t slot;
            std::memcpy(&slot, code + 2, 4);
            auto imp = g_imports.find(slot);
            if (imp != g_imports.end()) {
                void* proc = ResolveImport(imp->second);
                if (proc && WriteJump(at, reinterpret_cast<uintptr_t>(proc))) {
                    counts.imports++;
                    kinds.push_back({at, 'i'});
                    continue;
                }
                if (log) {
                    std::fprintf(log, "x64 dispatch: import %s!%s (thunk 0x%08X): no 64-bit export\n", imp->second.dll.c_str(),
                                 imp->second.name.empty() ? "#ordinal" : imp->second.name.c_str(), unsigned(at));
                }
            } else if (slot >= g_textHi && slot < kReserveHi) {
                // a jump through one of the game's variables (a function pointer it sets at run time,
                // NiFile's): an island reading the 4-byte pointer when called
                // (mov eax, dword ptr [slot]; jmp rax)
                if (uint8_t* island = Island(9)) {
                    island[0] = 0x8B;
                    island[1] = 0x04;
                    island[2] = 0x25;
                    std::memcpy(island + 3, &slot, 4);
                    island[7] = 0xFF;
                    island[8] = 0xE0;
                    Rel32Jump(reinterpret_cast<uint8_t*>(uintptr_t(at)), reinterpret_cast<uintptr_t>(island));
                    counts.thunks++;
                    kinds.push_back({at, 'j'});
                    continue;
                }
            }
        }
        if (WriteTrap(at)) {
            counts.traps++;
            trapped.push_back({at, next - at});
            kinds.push_back({at, 't'});
        }
    }
    // the list of what has no body yet (each start and the bytes to the next start), for the
    // bring-up: bullyre_x64_traps.txt in the working folder; every start and what it got in
    // bullyre_x64_starts.txt
    if (FILE* list = std::fopen(re::OutputName("starts.txt"), "w")) {
        for (const auto& [address, kind] : kinds) {
            auto body = kind == 'b' ? bodies.find(address) : bodies.end();
            if (body != bodies.end()) {   // (and the body's address, for tools/x64_tiny_check.py)
                std::fprintf(list, "0x%08X %c 0x%llX\n", unsigned(address), kind,
                             static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(body->second.first)));
            } else {
                std::fprintf(list, "0x%08X %c\n", unsigned(address), kind);
            }
        }
        std::fclose(list);
    }
    if (FILE* list = std::fopen(re::OutputName("traps.txt"), "w")) {
        for (const auto& [address, size] : trapped) {
            std::fprintf(list, "0x%08X %u\n", unsigned(address), unsigned(size));
        }
        std::fclose(list);
    }
    // the data in .text: the game's bytes back over the fill (and over the stub a start before
    // it may have written into its first bytes)
    for (const TextData& d : kTextData) {
        if (const uint8_t* bytes = FileAt(d.lo, d.hi - d.lo)) {
            std::memcpy(reinterpret_cast<void*>(uintptr_t(d.lo)), bytes, d.hi - d.lo);
        }
    }
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(kReserveLo), g_textHi - kReserveLo);
    return counts;
}

bool MapGameData(FILE* log) {
    DWORD old;
    uintptr_t lo = (uintptr_t(g_textHi) + 0xFFF) & ~uintptr_t(0xFFF);
    if (!VirtualProtect(reinterpret_cast<void*>(lo), kReserveHi - lo, PAGE_READWRITE, &old)) {
        return false;
    }
    for (const Section& s : g_sections) {
        if (s.name == ".text") {
            continue;
        }
        if (s.va + s.vsize > kReserveHi) {
            if (log) {
                std::fprintf(log, "x64 dispatch: section %s past the reserve\n", s.name.c_str());
            }
            return false;
        }
        std::memcpy(reinterpret_cast<void*>(uintptr_t(s.va)), &g_file[s.raw], std::min(s.rawSize, s.vsize));
        if (log) {
            std::fprintf(log, "x64 dispatch: %s mapped at 0x%08X (0x%X bytes from the file, 0x%X in all)\n", s.name.c_str(),
                         unsigned(s.va), unsigned(std::min(s.rawSize, s.vsize)), unsigned(s.vsize));
        }
    }
    // The import table bound: our code calls some imports through the game's slots (the static
    // initializers' `call [__imp_InitializeCriticalSection]`). A slot is 4 bytes and an export
    // may be above 4 GB (kernel32's forward to ntdll), so each slot holds an island below 2 GB
    // that jumps to the 64-bit export; one with no 64-bit export holds a trap (its slot's address).
    int bound = 0, missing = 0;
    for (const auto& [slot, imp] : g_imports) {
        uint8_t* island = nullptr;
        if (void* proc = ResolveImport(imp)) {
            island = AbsoluteIsland(reinterpret_cast<uintptr_t>(proc));
            bound++;
        } else {
            island = Island(10);   // mov ecx, slot; jmp MissingFunction
            if (island) {
                island[0] = 0xB9;
                std::memcpy(island + 1, &slot, 4);
                Rel32Jump(island + 5, reinterpret_cast<uintptr_t>(&MissingFunction));
            }
            missing++;
            if (log) {
                std::fprintf(log, "x64 dispatch: import %s!%s (slot 0x%08X): no 64-bit export, a trap\n", imp.dll.c_str(),
                             imp.name.empty() ? "#ordinal" : imp.name.c_str(), unsigned(slot));
            }
        }
        if (island) {
            uint32_t v = uint32_t(reinterpret_cast<uintptr_t>(island));
            std::memcpy(reinterpret_cast<void*>(uintptr_t(slot)), &v, 4);
        }
    }
    if (log) {
        std::fprintf(log, "x64 dispatch: %d import slots bound, %d without a 64-bit export\n", bound, missing);
    }
    re::low::KeepSystemObjectsLow();   // (DirectInput and the rest the imports loaded)
    return true;
}

uint32_t LastMissing() {
    return g_lastMissing;
}

void SetTrapsRecoverable(bool on) {
    g_recoverable = on;
}

void SetTrapsLenient(bool on) {
    g_lenient = on;
}

long LenientCount() {
    return g_lenientCount;
}

} // namespace x64
