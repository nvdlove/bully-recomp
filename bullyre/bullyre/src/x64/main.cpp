// bullyre_x64.exe: the first 64-bit program made of the reversed code (roadmap step 7). It
// links every object of src/game and src/gamebryo, installs the 64-bit dispatch
// (src/x64/dispatch.cpp: a jump at each of the game's function addresses) and calls game
// functions through their original addresses, as our code and the game's data do, checking
// each against a direct call of our body.
//
// bullyre_x64.exe (no arguments) or --play: the game, from the exe's folder (it goes in the game's
//   folder, next to Bully_unpacked.exe): no tests, no console; a crash leaves
//   bullyre_x64_crash.txt (where, the stack, named from bullyre_x64.map next to the exe) and
//   bullyre_x64_crash.dmp (a minidump) there and says so
// bullyre_x64.exe --play --game-dir <folder>: the same, the game in <folder> (its Bully_unpacked.exe
//   and data) wherever this exe is (bullymenu starts it so, from its own folder): that folder is
//   the current one (the logs, crash reports and doorways table go there) and bullygfx64's
//   (BULLYGFX_GAME_DIR: its settings, mods and caches)
// bullyre_x64.exe [--report] [--trap] [--init] [--image <Bully_unpacked.exe>] [--functions <more starts>]
//   --report  where Windows puts the heap, the stack, system DLLs and handles in this process
//   --trap    also call a game function with no body (the trap reports it and exits 3)
//   --winmain then the game's WinMain from the game's folder, guarded: where the 64-bit build stops
//   --seconds N  with --winmain: the game sampled every 10 seconds, the run ended after N
//   --keep-active  the game never paused for its window losing the focus (runflags.h)
//   --quit S  the front end's Quit chosen at S seconds (the shutdown path)
//   --keys K@S[:MS],...  keys pressed at S seconds from the start, held MS (150) ms: K a
//             DirectInput code or a name (ENTER, ESC, SPACE, the arrows, LALT, LSHIFT, TAB, letters,
//             1-4; LMB, RMB, MMB the mouse's buttons: AddKeys)
//   --init    then map the game's data and run its static initializers (the table at 0x8FC350)
//             through their game addresses, each guarded: what has no body, what faults

#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "reversed.h"
#include "runflags.h"
#include "x64/crt64.h"
#include "x64/dispatch.h"
#include "x64/lowheap.h"

extern "C" uint8_t g_game[];   // src/x64/reserve64.asm

#pragma comment(lib, "dbghelp.lib")

namespace {

int g_high = 0;
int g_failed = 0;
int g_passed = 0;

void Report(const char* what, const void* p) {
    uintptr_t a = reinterpret_cast<uintptr_t>(p);
    bool fits = a < 0x100000000ull;
    if (!fits) {
        g_high++;
    }
    std::printf("  %-34s %016llX %s\n", what, static_cast<unsigned long long>(a), fits ? "below 4 GB" : "ABOVE 4 GB");
}

void AddressReport() {
    std::printf("where this process's memory is:\n");
    int local = 0;
    Report("a stack variable", &local);
    Report("malloc(64)", std::malloc(64));
    Report("malloc(64 MB)", std::malloc(64u << 20));
    Report("VirtualAlloc(1 MB)", VirtualAlloc(nullptr, 1u << 20, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Report("HeapAlloc(process heap)", HeapAlloc(GetProcessHeap(), 0, 64));
    Report("this exe", GetModuleHandleA(nullptr));
    Report("the game range (reserve)", g_game);
    Report("ntdll.dll", GetModuleHandleA("ntdll.dll"));
    Report("kernel32.dll", GetModuleHandleA("kernel32.dll"));
    Report("d3d9.dll (system's)", LoadLibraryA("d3d9.dll"));
    Report("dinput8.dll", LoadLibraryA("dinput8.dll"));
    HKEY key = nullptr;
    RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &key);
    Report("a registry key handle", key);
    std::printf("%d of them above 4 GB\n", g_high);
}

void Check(const char* what, bool ok, const char* detail) {
    std::printf("  %-4s %-46s %s\n", ok ? "ok" : "FAIL", what, detail);
    (ok ? g_passed : g_failed)++;
}

bool Same(float a, float b) {
    return std::memcmp(&a, &b, 4) == 0;
}

struct V3 {
    float x, y, z;
};

// the 5-byte stub at a game address: where it jumps
uintptr_t StubTarget(uintptr_t at) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(at);
    if (p[0] != 0xE9) {
        return 0;
    }
    int32_t rel;
    std::memcpy(&rel, p + 1, 4);
    return at + 5 + rel;
}

void CallTests(bool trap) {
    char detail[160];
    std::printf("calls through the game's addresses:\n");

    // cdecl, floats in xmm0/xmm1: CGeneral::AngleDifference (0x432980)
    {
        auto viaGame = reinterpret_cast<float(__cdecl*)(float, float)>(0x00432980);
        auto direct = re::Ours<float(__cdecl*)(float, float)>(0x00432980);
        float a = viaGame(3.0f, -2.5f), b = direct(3.0f, -2.5f);
        std::snprintf(detail, sizeof detail, "%.6f (ours directly %.6f)", a, b);
        Check("CGeneral::AngleDifference(3, -2.5)", Same(a, b), detail);
    }
    // cdecl, five floats (the fifth on the stack): CGeneral::LinearMap (0x432B60)
    {
        auto viaGame = reinterpret_cast<float(__cdecl*)(float, float, float, float, float)>(0x00432B60);
        auto direct = re::Ours<float(__cdecl*)(float, float, float, float, float)>(0x00432B60);
        float a = viaGame(5.0f, 0.0f, 0.0f, 100.0f, 10.0f), b = direct(5.0f, 0.0f, 0.0f, 100.0f, 10.0f);
        std::snprintf(detail, sizeof detail, "%.6f (ours directly %.6f)", a, b);
        Check("CGeneral::LinearMap(5, 0, 0, 100, 10)", Same(a, b), detail);
    }
    // cdecl, pointers: CVector::Distance (0x41A950)
    {
        V3 p{1, 2, 3}, q{4, 6, 3};
        auto viaGame = reinterpret_cast<float(__cdecl*)(const V3*, const V3*)>(0x0041A950);
        float a = viaGame(&p, &q);
        std::snprintf(detail, sizeof detail, "%.6f (want 5)", a);
        Check("CVector::Distance((1,2,3), (4,6,3))", Same(a, 5.0f), detail);
    }
    // a __thiscall game function: the stub reaches the adapter, which calls our
    // __fastcall(self, re::Edx, divisor) body: CVector::operator/= (0x41A4A0)
    {
        V3 v{9, 6, 3};
        auto viaGame = reinterpret_cast<V3*(__thiscall*)(V3*, float)>(0x0041A4A0);
        V3* r = viaGame(&v, 3.0f);
        std::snprintf(detail, sizeof detail, "(%.3f, %.3f, %.3f), returned self: %s", v.x, v.y, v.z, r == &v ? "yes" : "no");
        Check("CVector::operator/= ((9,6,3), 3) [adapter]", r == &v && Same(v.x, 3) && Same(v.y, 2) && Same(v.z, 1), detail);
        uintptr_t to = StubTarget(0x0041A4A0);
        std::snprintf(detail, sizeof detail, "stub -> %p, body %p", reinterpret_cast<void*>(to), re::OursAt(0x0041A4A0));
        Check("  its stub goes to the adapter, not the body", to != 0 && to != reinterpret_cast<uintptr_t>(re::OursAt(0x0041A4A0)),
              detail);
    }
    // a game jump thunk: CIplStore::LoadIpb_thunk (0x439760, jmp 0x4390A0) aliases its target
    {
        uintptr_t to = StubTarget(0x00439760);
        std::snprintf(detail, sizeof detail, "stub -> %p", reinterpret_cast<void*>(to));
        Check("thunk 0x439760 jumps to 0x4390A0's stub", to == 0x004390A0, detail);
    }
    if (trap) {
        std::printf("calling 0x0043AD80 (no body): the trap should report it and exit 3\n");
        std::fflush(stdout);
        reinterpret_cast<void (*)()>(0x0043AD80)();
        Check("the trap exits", false, "returned");
    }
}

// ---- the game's static initializers (the table the C runtime runs before WinMain)

// our exe's functions by address, from the linker's map (bullyre_x64.map next to the exe): to
// name where a fault happened
struct MapSymbol {
    uintptr_t address;
    std::string name;
};
std::vector<MapSymbol> g_symbols;

void LoadMap() {
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (n < 4) {
        return;
    }
    std::strcpy(path + n - 4, ".map");
    FILE* f = std::fopen(path, "r");
    if (!f) {
        return;
    }
    char line[2048];
    while (std::fgets(line, sizeof line, f)) {
        unsigned seg, off;
        char name[1024];
        unsigned long long address;
        if (std::sscanf(line, " %x:%x %1023s %llx", &seg, &off, name, &address) == 4 && seg == 1 && address >= 0x400000) {
            g_symbols.push_back({uintptr_t(address), name});
        }
    }
    std::fclose(f);
    std::sort(g_symbols.begin(), g_symbols.end(), [](const MapSymbol& a, const MapSymbol& b) { return a.address < b.address; });
}

std::string SymbolAt(uintptr_t address) {
    // (outside our exe: the module and the offset in it)
    HMODULE module = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(address), &module) &&
        module != GetModuleHandleA(nullptr)) {
        char path[MAX_PATH] = "?";
        GetModuleFileNameA(module, path, MAX_PATH);
        const char* slash = std::strrchr(path, '\\');
        char buf[MAX_PATH + 32];
        std::snprintf(buf, sizeof buf, "%s+0x%llX", slash ? slash + 1 : path,
                      static_cast<unsigned long long>(address - reinterpret_cast<uintptr_t>(module)));
        return buf;
    }
    auto it = std::upper_bound(g_symbols.begin(), g_symbols.end(), address,
                               [](uintptr_t a, const MapSymbol& s) { return a < s.address; });
    if (it == g_symbols.begin()) {
        return "?";
    }
    --it;
    char buf[64];
    std::snprintf(buf, sizeof buf, "+0x%llX", static_cast<unsigned long long>(address - it->address));
    return it->name + buf;
}

struct Outcome {
    DWORD code = 0;
    uintptr_t at = 0;         // where
    uintptr_t detail = 0;     // the missing function's address, or the address an access violation touched
    void* frames[24] = {};    // the stack when it happened (from the filter, before unwinding)
    USHORT frameCount = 0;
};

// a frame's stack from its context: each return address, through the unwind data (a stub or
// island has none: its return address is on top of the stack)
USHORT WalkStack(CONTEXT c, void** frames, USHORT max) {
    USHORT count = 0;
    for (USHORT i = 0; i < max; i++) {
        frames[i] = reinterpret_cast<void*>(c.Rip);
        count = USHORT(i + 1);
        DWORD64 imageBase = 0;
        PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(c.Rip, &imageBase, nullptr);
        if (!fn) {
            if (c.Rsp == 0) {
                break;
            }
            c.Rip = *reinterpret_cast<DWORD64*>(c.Rsp);
            c.Rsp += 8;
        } else {
            void* handlerData = nullptr;
            DWORD64 establisher = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, c.Rip, fn, &c, &handlerData, &establisher, nullptr);
        }
        if (c.Rip == 0) {
            break;
        }
    }
    return count;
}

bool g_play = false;   // play mode (no arguments or --play)
void WriteMinidump(EXCEPTION_POINTERS* e);

int Filter(EXCEPTION_POINTERS* e, Outcome* o) {
    if (g_play) {
        WriteMinidump(e);
    }
    o->code = e->ExceptionRecord->ExceptionCode;
    o->at = reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress);
    if (o->code == x64::kMissingFunction) {
        o->detail = e->ExceptionRecord->ExceptionInformation[0];
        o->at = e->ExceptionRecord->ExceptionInformation[1];
    } else if (o->code == re::kPointerTooWide) {
        o->detail = e->ExceptionRecord->ExceptionInformation[0];
    } else if (o->code == EXCEPTION_ACCESS_VIOLATION && e->ExceptionRecord->NumberParameters >= 2) {
        o->detail = e->ExceptionRecord->ExceptionInformation[1];
    }
    // the faulting frame's stack, walked from its own context (the filter's would start in
    // the dispatcher)
    o->frameCount = WalkStack(*e->ContextRecord, o->frames, 24);
    return EXCEPTION_EXECUTE_HANDLER;
}

void PrintFrames(const Outcome& o, FILE* to = stdout) {
    for (USHORT i = 0; i < o.frameCount; i++) {
        uintptr_t a = reinterpret_cast<uintptr_t>(o.frames[i]);
        std::fprintf(to, "    %016llX %s\n", static_cast<unsigned long long>(a), SymbolAt(a).c_str());
    }
}

// ---- play mode: what a crash leaves (bullyre_x64_crash.txt and .dmp in the game's folder), and
// the accessibility settings put back (the game's WinMain turns their hotkeys off and restores
// them only when it returns)

STICKYKEYS g_stickyKeys{sizeof(STICKYKEYS)};
TOGGLEKEYS g_toggleKeys{sizeof(TOGGLEKEYS)};
FILTERKEYS g_filterKeys{sizeof(FILTERKEYS)};

void SaveAccessibility() {
    SystemParametersInfoA(SPI_GETSTICKYKEYS, sizeof g_stickyKeys, &g_stickyKeys, 0);
    SystemParametersInfoA(SPI_GETTOGGLEKEYS, sizeof g_toggleKeys, &g_toggleKeys, 0);
    SystemParametersInfoA(SPI_GETFILTERKEYS, sizeof g_filterKeys, &g_filterKeys, 0);
}

void RestoreAccessibility() {
    SystemParametersInfoA(SPI_SETSTICKYKEYS, sizeof g_stickyKeys, &g_stickyKeys, 0);
    SystemParametersInfoA(SPI_SETTOGGLEKEYS, sizeof g_toggleKeys, &g_toggleKeys, 0);
    SystemParametersInfoA(SPI_SETFILTERKEYS, sizeof g_filterKeys, &g_filterKeys, 0);
}

void WriteMinidump(EXCEPTION_POINTERS* e) {
    HANDLE f = CreateFileA(re::OutputName("crash.dmp"), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) {
        return;
    }
    MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(), e, FALSE};
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), f,
                      MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo), e ? &info : nullptr, nullptr,
                      nullptr);
    CloseHandle(f);
}

void WriteCrashReport(const Outcome& o, const char* thread) {
    FILE* f = std::fopen(re::OutputName("crash.txt"), "w");
    if (!f) {
        return;
    }
    if (o.code == x64::kMissingFunction) {
        std::fprintf(f, "bullyre_x64 stopped (%s thread): the game's 0x%08X has no 64-bit body (called from %s)\n", thread,
                     unsigned(o.detail), SymbolAt(o.at).c_str());
    } else if (o.code == re::kPointerTooWide) {
        std::fprintf(f, "bullyre_x64 stopped (%s thread): the pointer %llX (not from the low heap) stored into a game "
                     "field (4 bytes; the stack says where)\n", thread, static_cast<unsigned long long>(o.detail));
    } else {
        std::fprintf(f, "bullyre_x64 stopped (%s thread): exception %08lX at %016llX %s (address %llX)\n", thread, o.code,
                     static_cast<unsigned long long>(o.at), SymbolAt(o.at).c_str(), static_cast<unsigned long long>(o.detail));
    }
    if (*re::CurrentScriptedMission()) {
        std::fprintf(f, "the test run's mission: %s\n", re::CurrentScriptedMission());
    }
    std::fprintf(f, "the stack:\n");
    PrintFrames(o, f);
    std::fclose(f);
}

void ShowCrash() {
    MessageBoxA(nullptr,
                "Bully (64-bit) stopped.\n\nbullyre_x64_crash.txt and bullyre_x64_crash.dmp in the game's folder say where.",
                "bullyre_x64", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

// a crash on another thread than WinMain's (streaming, sound): reported the same way
LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* e) {
    Outcome o;
    Filter(e, &o);
    WriteCrashReport(o, "another");
    WriteMinidump(e);
    if (!g_play) {
        // (a test run: said on the console, no message box)
        std::printf("crashed on another thread: exception %08lX at %s (address %llX)%s%s\n", o.code, SymbolAt(o.at).c_str(),
                    static_cast<unsigned long long>(o.detail), *re::CurrentScriptedMission() ? "; mission " : "",
                    re::CurrentScriptedMission());
        PrintFrames(o);
        std::fflush(nullptr);
        TerminateProcess(GetCurrentProcess(), 3);
    }
    RestoreAccessibility();
    ShowCrash();
    return EXCEPTION_EXECUTE_HANDLER;
}

// ---- the watchdog (--seconds N): every 10 seconds where the game's main thread is, the
// game's frame counter and a picture of its window (build64/x64_shot.bmp); after N seconds
// the process ended (a test run's end)

int g_seconds = 0;
bool g_printBlips = false;
int g_shotEvery = 10;   // (--shots-every)   // (--blips: the radar's blips in each sample)
HANDLE g_mainThread = nullptr;

struct WindowSearch {
    DWORD pid;
    HWND found;
};

BOOL CALLBACK FindGameWindow(HWND w, LPARAM p) {
    auto* s = reinterpret_cast<WindowSearch*>(p);
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    char cls[64] = {};
    if (pid == s->pid && IsWindowVisible(w) && GetClassNameA(w, cls, sizeof cls) && !std::strcmp(cls, "Gamebryo Application")) {
        s->found = w;
        return FALSE;
    }
    return TRUE;
}

// the window's client area as the compositor has it (PrintWindow with full content: only this
// window, never what's over it on the screen; a Direct3D window's own DC reads black), as a
// 24-bit BMP
bool SaveWindowPicture(HWND w, const char* path) {
    RECT r;
    if (!GetClientRect(w, &r) || r.right <= 0 || r.bottom <= 0) {
        return false;
    }
    int width = r.right, height = r.bottom;
    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, width, height);
    HGDIOBJ old = SelectObject(mem, bmp);
    PrintWindow(w, mem, PW_CLIENTONLY | PW_RENDERFULLCONTENT);
    SelectObject(mem, old);
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof bi;
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    int stride = (width * 3 + 3) & ~3;
    std::vector<uint8_t> pixels(size_t(stride) * height);
    GetDIBits(mem, bmp, 0, UINT(height), pixels.data(), reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
    FILE* f = std::fopen(path, "wb");
    if (!f) {
        return false;
    }
    BITMAPFILEHEADER fh = {};
    fh.bfType = 0x4D42;
    fh.bfOffBits = sizeof fh + sizeof bi;
    fh.bfSize = DWORD(fh.bfOffBits + pixels.size());
    std::fwrite(&fh, sizeof fh, 1, f);
    std::fwrite(&bi, sizeof bi, 1, f);
    std::fwrite(pixels.data(), 1, pixels.size(), f);
    std::fclose(f);
    return true;
}

void SampleGame(int second) {
    void* frames[16] = {};
    USHORT count = 0;
    if (SuspendThread(g_mainThread) != DWORD(-1)) {
        CONTEXT c = {};
        c.ContextFlags = CONTEXT_FULL;
        if (GetThreadContext(g_mainThread, &c)) {
            count = WalkStack(c, frames, 16);
        }
        ResumeThread(g_mainThread);
    }
    // the game's CTimer::m_FrameCounter, m_snTimeInMilliseconds, its state, whether started
    uint32_t frame = *reinterpret_cast<volatile uint32_t*>(0x00C1A99C);
    uint32_t ms = *reinterpret_cast<volatile uint32_t*>(0x00C1A9B4);
    uint32_t state = *reinterpret_cast<volatile uint32_t*>(0x00BA6894);
    uint32_t started = *reinterpret_cast<volatile uint8_t*>(0x00BCBC3B);
    WindowSearch s = {GetCurrentProcessId(), nullptr};
    EnumWindows(&FindGameWindow, reinterpret_cast<LPARAM>(&s));
    // (the game's next Present saves its back buffer: only what the game drew)
    // (each sample's own picture: build64/shots/x64_shot_<second>.png)
    // (--shots-every N: only every N seconds' sample; 0: none)
    static char path[MAX_PATH];
    if (g_shotEvery > 0 && second % g_shotEvery == 0 && GetModuleHandleA("bullygfx64.dll")) {
        // (bullygfx64's renderer draws past the game's back buffer: its "shot final" command saves
        // what it drew, next to the exe: bullyre_x64[_tag]_shot_<second>.png)
        char cmd[64] = "bullygfx_cmd.txt";
        GetEnvironmentVariableA("BULLYGFX_CMD", cmd, sizeof cmd);
        if (FILE* f = std::fopen(cmd, "a")) {
            char what[40];
            const char* mission = re::CurrentScriptedMission();
            std::snprintf(what, sizeof what, "shot_%03d_m%d.png", second, *mission >= '0' && *mission <= '9' ? std::atoi(mission) : -1);
            std::fprintf(f, "shot final %s\n", re::OutputName(what));
            std::fclose(f);
        }
    } else if (g_shotEvery > 0 && second % g_shotEvery == 0) {
        CreateDirectoryA("C:/Users/user/Desktop/SPEEDRUNPROJECTS/bullyre/build64/shots", nullptr);
        std::snprintf(path, sizeof path, "C:/Users/user/Desktop/SPEEDRUNPROJECTS/bullyre/build64/shots/x64_shot_%03d.png", second);
        re::RequestFrameGrab(path);
    }
    bool shot = false;
    uint32_t draws = 0, failedDraws = 0;
    re::DrawCounts(&draws, &failedDraws);
    std::printf("[%3ds] frame %u, game time %u ms, state %u, started %u, window %s%s, lenient traps %ld, geometry draws %u (%u failed); the main thread:\n",
                second, frame, ms, state, started, s.found ? "up" : "none", shot ? " (picture saved)" : "", x64::LenientCount(), draws,
                failedDraws);
    if (*re::CurrentScriptedMission()) {
        std::printf("       mission: %s\n", re::CurrentScriptedMission());
    }
    {
        // the pools' slots in use (CPool: flags +4, size +8; a slot's flag 0x80: free)
        static const struct {
            const char* name;
            uintptr_t at;
        } kPools[] = {{"propanim", 0x00C0F608}, {"object", 0x00C0F614}, {"ped", 0x00C0F5F0}, {"vehicle", 0x00C0F5F4},
                      {"dummy", 0x00C0F600}, {"building", 0x00C0F5F8}};
        std::printf("       pools:");
        for (const auto& p : kPools) {
            uintptr_t pool = uintptr_t(*reinterpret_cast<volatile uint32_t*>(p.at));
            if (!pool) {
                continue;
            }
            const uint8_t* flags = reinterpret_cast<const uint8_t*>(uintptr_t(*reinterpret_cast<volatile uint32_t*>(pool + 4)));
            int size = *reinterpret_cast<volatile int32_t*>(pool + 8), used = 0;
            for (int i = 0; flags && i < size; i++) {
                used += (flags[i] & 0x80) == 0;
            }
            std::printf(" %s %d/%d", p.name, used, size);
        }
        std::printf("\n");
    }
    {
        // memory: the low heap (the game's data) and the whole process (the system's too)
        re::low::Stats h = re::low::GetStats();
        PROCESS_MEMORY_COUNTERS_EX pm{sizeof pm};
        GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pm), sizeof pm);
        std::printf("       memory: low heap %zu MB in %zu blocks, %zu MB committed of %zu MB held%s; the process %zu MB private\n",
                    h.inUse >> 20, h.blocks, h.committed >> 20, h.reserved >> 20, h.stress ? " (--stress-high)" : "",
                    size_t(pm.PrivateUsage) >> 20);
        re::low::HighStats hs = re::low::GetHighStats();
        std::printf("       the system DLLs' large blocks: %zu MB in %zu; %zu made, %zu reused, %zu released so far; %zu of our blocks "
                    "given to the system's heap from outside\n",
                    hs.bytes >> 20, hs.blocks, hs.allocated, hs.reused, hs.released, hs.foreign);
    }
    if (s.found && second == 10 * ((g_seconds + 9) / 10 / 2)) {   // (once, half way: where and how the window is)
        RECT r = {};
        GetWindowRect(s.found, &r);
        std::printf("       the window: (%ld, %ld)-(%ld, %ld), style %08lX; the screen %d x %d\n", r.left, r.top, r.right, r.bottom,
                    static_cast<unsigned long>(GetWindowLongA(s.found, GWL_STYLE)), GetSystemMetrics(SM_CXSCREEN),
                    GetSystemMetrics(SM_CYSCREEN));
    }
    // the camera (CVisibilityPlugins::ms_pCameraPosn) and the player (its matrix's position)
    auto at32 = [](uintptr_t a) { return uintptr_t(*reinterpret_cast<volatile uint32_t*>(a)); };
    if (uintptr_t cam = at32(0x01B64880)) {
        const float* c = reinterpret_cast<const float*>(cam);
        std::printf("       camera (%.2f, %.2f, %.2f)", c[0], c[1], c[2]);
    }
    if (uintptr_t ped = at32(0x00C1AEA8)) {
        uintptr_t matrix = at32(ped + 0x14);
        const float* p = matrix ? reinterpret_cast<const float*>(matrix + 0x30) : reinterpret_cast<const float*>(ped + 4);
        std::printf(" player (%.2f, %.2f, %.2f)", p[0], p[1], p[2]);
    }
    // the renderer's lists (CRenderer: visible entities 0xC13F00, LODs 0xC11D64) and the area
    std::printf(" visible %d lods %d area %d rendered %d", *reinterpret_cast<volatile int32_t*>(0x00C13F00),
                *reinterpret_cast<volatile int32_t*>(0x00C11D64), *reinterpret_cast<volatile int32_t*>(0x00BD1008),
                *reinterpret_cast<volatile int32_t*>(0x00C2A8F4));
    // the visible entities: how many have their rw object (+0x18) loaded, by type
    {
        int n = *reinterpret_cast<volatile int32_t*>(0x00C13F00);
        const uint32_t* list = reinterpret_cast<const uint32_t*>(0x00C11FC0);   // CRenderer::ms_aVisibleEntityPtrs
        int loaded = 0, types[8] = {};
        for (int i = 0; i < n && i < 1000; i++) {
            const uint8_t* e = reinterpret_cast<const uint8_t*>(uintptr_t(list[i]));
            if (!e) {
                continue;
            }
            types[e[0x108] & 7]++;
            loaded += *reinterpret_cast<const uint32_t*>(e + 0x18) != 0;   // (rwObject)
        }
        std::printf(" (loaded %d; buildings %d peds %d objects %d dummies %d props %d)", loaded, types[1], types[3], types[4], types[5],
                    types[6]);
    }
    std::printf(" draw sites %u/%u/%u/%u\n", re::DrawSiteCount(0), re::DrawSiteCount(1), re::DrawSiteCount(2), re::DrawSiteCount(3));
    // the radar's blips with a position (where a test run can teleport: the radar at 0xC674CC,
    // 300 blips of 0x30 at +0x210; +0x2F flag 2 in use; the sprites' names at 0x91A3E8, 0x14 each)
    if (g_printBlips) {
        if (const uint8_t* radar = reinterpret_cast<const uint8_t*>(uintptr_t(*reinterpret_cast<const uint32_t*>(0x00C674CC)))) {
            for (int i = 0; i < 300; i++) {
                const uint8_t* b = radar + 0x210 + i * 0x30;
                if (!(b[0x2F] & 2)) {
                    continue;
                }
                const float* p = reinterpret_cast<const float*>(b + 4);
                int sprite = *reinterpret_cast<const int32_t*>(b + 0x28);
                const char* name = "?";
                if (sprite >= 0 && sprite < 39) {
                    uint32_t text = *reinterpret_cast<const uint32_t*>(0x0091A3E8 + sprite * 0x14);
                    name = text ? reinterpret_cast<const char*>(uintptr_t(text)) : "-";
                }
                std::printf("       blip %d: type %d handle %d at (%.2f, %.2f, %.2f) sprite %d %s\n", i,
                            *reinterpret_cast<const int32_t*>(b + 0x20), *reinterpret_cast<const int32_t*>(b), p[0], p[1], p[2], sprite,
                            name);
            }
        }
    }
    for (USHORT i = 0; i < count; i++) {
        uintptr_t a = reinterpret_cast<uintptr_t>(frames[i]);
        std::printf("    %016llX %s\n", static_cast<unsigned long long>(a), SymbolAt(a).c_str());
    }
    std::fflush(stdout);
}

// (the game's state read from another thread: during the shutdown it's being freed)
void SampleGameGuarded(int second) {
    __try {
        SampleGame(second);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        std::printf("[%3ds] (the game's state unreadable: shutting down?)\n", second);
        std::fflush(stdout);
    }
}

DWORD WINAPI Watchdog(void*) {
    for (int second = 10;; second += 10) {
        Sleep(10000);
        SampleGameGuarded(second);
        if (second >= g_seconds) {
            std::printf("watchdog: %d seconds: the run ended here\n", g_seconds);
            std::fflush(nullptr);
            TerminateProcess(GetCurrentProcess(), 0);
        }
    }
}

// (--grabs FROM:EVERY:COUNT: COUNT more frame grabs, one every EVERY ms from FROM seconds on, into
// build64/shots/x64_grab_NNN.png: a closer look at something moving)
int g_grabFrom = 0, g_grabEvery = 0, g_grabCount = 0;

DWORD WINAPI Grabber(void*) {
    Sleep(DWORD(g_grabFrom) * 1000);
    for (int i = 0; i < g_grabCount; i++) {
        char path[MAX_PATH];
        std::snprintf(path, sizeof path, "C:/Users/user/Desktop/SPEEDRUNPROJECTS/bullyre/build64/shots/x64_grab_%03d.png", i);
        re::RequestFrameGrab(path);
        Sleep(DWORD(g_grabEvery));
    }
    return 0;
}

// (--profile S: from S seconds on, the main thread sampled about a thousand times a second for 5
// seconds; the functions it was in most printed, named by DbgHelp: ours, bullygfx64's (from its
// build folder), the system DLLs' exports)
int g_profileAt = 0;

DWORD WINAPI Profiler(void*) {
    Sleep(DWORD(g_profileAt) * 1000);
    std::map<DWORD64, int> hits;
    int samples = 0;
    ULONGLONG end = GetTickCount64() + 5000;
    while (GetTickCount64() < end) {
        if (SuspendThread(g_mainThread) != DWORD(-1)) {
            CONTEXT c{};
            c.ContextFlags = CONTEXT_CONTROL;
            if (GetThreadContext(g_mainThread, &c)) {
                hits[c.Rip]++;
                samples++;
            }
            ResumeThread(g_mainThread);
        }
        Sleep(1);
    }
    HANDLE process = GetCurrentProcess();
    char exeDir[MAX_PATH];
    GetModuleFileNameA(nullptr, exeDir, MAX_PATH);
    if (char* slash = std::strrchr(exeDir, '\\')) {
        *slash = 0;
    }
    std::string path = std::string(exeDir) + ";C:\\Users\\user\\Desktop\\SPEEDRUNPROJECTS\\bullygfx\\target\\x86_64-pc-windows-msvc\\release";
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymInitialize(process, path.c_str(), TRUE);
    std::map<std::string, int> byFunction;
    for (const auto& [at, n] : hits) {
        alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256];
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;
        DWORD64 displacement = 0;
        IMAGEHLP_MODULE64 module{};
        module.SizeOfStruct = sizeof module;
        std::string m = SymGetModuleInfo64(process, at, &module) ? module.ModuleName : "?";
        byFunction[SymFromAddr(process, at, &displacement, symbol) ? m + "!" + symbol->Name : m + "+?"] += n;
    }
    SymCleanup(process);
    std::vector<std::pair<int, std::string>> top;
    for (const auto& [name, n] : byFunction) {
        top.push_back({n, name});
    }
    std::sort(top.rbegin(), top.rend());
    std::printf("profile: %d samples of the main thread from %d s on; where it was most:\n", samples, g_profileAt);
    for (size_t i = 0; i < top.size() && i < 30; i++) {
        std::printf("  %5.1f%%  %s\n", 100.0 * top[i].first / (samples ? samples : 1), top[i].second.c_str());
    }
    std::fflush(nullptr);
    return 0;
}

void StartWatchdog(int seconds) {
    g_seconds = seconds;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &g_mainThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
    CloseHandle(CreateThread(nullptr, 0, &Watchdog, nullptr, 0, nullptr));
    if (g_grabCount > 0) {
        CloseHandle(CreateThread(nullptr, 0, &Grabber, nullptr, 0, nullptr));
    }
    if (g_profileAt > 0) {
        CloseHandle(CreateThread(nullptr, 0, &Profiler, nullptr, 0, nullptr));
    }
}

// The game's WinMain (0x43E7D0, ours) through its address, from the game's folder, guarded:
// how far the 64-bit build gets
int RunWinMain(FILE* log, const char* folder) {
    SetCurrentDirectoryA(folder);
    std::printf("WinMain (0x43E7D0) from %s:\n", folder);
    std::fflush(stdout);
    Outcome o;
    x64::SetTrapsRecoverable(true);
    static char cmdline[] = "";
    struct Args {
        HINSTANCE instance;
        char* cmdline;
        int result;
    } args{GetModuleHandleA(nullptr), cmdline, 0};
    bool ok = [&] {
        __try {
            args.result = reinterpret_cast<int(__stdcall*)(HINSTANCE, HINSTANCE, char*, int)>(0x0043E7D0)(args.instance, nullptr, args.cmdline,
                                                                                                       SW_SHOWDEFAULT);
            return true;
        } __except (Filter(GetExceptionInformation(), &o)) {
            return false;
        }
    }();
    x64::SetTrapsRecoverable(false);
    if (ok) {
        std::printf("WinMain returned %d\n", args.result);
        return 0;
    }
    if (o.code == x64::kMissingFunction) {
        std::printf("WinMain stopped: the game's 0x%08X has no body (called from %s)\n", unsigned(o.detail), SymbolAt(o.at).c_str());
    } else if (o.code == re::kPointerTooWide) {
        std::printf("WinMain stopped: the pointer %llX (not from the low heap) stored into a game field\n",
                    static_cast<unsigned long long>(o.detail));
    } else {
        std::printf("WinMain stopped: exception %08lX at %s (address %llX)\n", o.code, SymbolAt(o.at).c_str(),
                    static_cast<unsigned long long>(o.detail));
    }
    PrintFrames(o);
    if (log) {
        std::fprintf(log, "WinMain stopped: exception %08lX, detail %llX\n", o.code, static_cast<unsigned long long>(o.detail));
    }
    if (g_play) {
        WriteCrashReport(o, "the game's main");
        RestoreAccessibility();
        ShowCrash();
    }
    return 1;
}

bool Guarded(uintptr_t fn, Outcome* o) {
    __try {
        reinterpret_cast<void (*)()>(fn)();
        return true;
    } __except (Filter(GetExceptionInformation(), o)) {
        return false;
    }
}

int RunInitializers(FILE* log) {
    if (!x64::MapGameData(log)) {
        std::printf("can't map the game's data\n");
        return 1;
    }
    LoadMap();
    x64::SetTrapsRecoverable(true);
    const uint32_t* table = reinterpret_cast<const uint32_t*>(0x008FC350);
    int ran = 0, trapped = 0, faulted = 0;
    std::map<uint32_t, int> missing;               // function -> initializers it stopped
    std::map<std::string, int> faults;             // where -> count
    std::map<std::string, uintptr_t> faultSample;
    for (int i = 0; i < 4096; i++) {
        uint32_t fn = table[i];
        if (fn == 0) {
            continue;
        }
        if (fn < 0x401000 || fn >= 0x8FC000) {
            break;
        }
        Outcome o;
        if (Guarded(fn, &o)) {
            ran++;
        } else if (o.code == x64::kMissingFunction) {
            trapped++;
            missing[uint32_t(o.detail)]++;
            if (log) {
                std::fprintf(log, "initializer 0x%08X: the game's 0x%08X has no body (called from %s)\n", unsigned(fn),
                             unsigned(o.detail), SymbolAt(o.at).c_str());
            }
        } else {
            faulted++;
            char key[512];
            std::snprintf(key, sizeof key, "%08lX at %s", o.code, SymbolAt(o.at).c_str());
            faults[key]++;
            faultSample[key] = o.detail;
            if (log) {
                std::fprintf(log, "initializer 0x%08X: exception %08lX at %s (address %llX)\n", unsigned(fn), o.code, SymbolAt(o.at).c_str(),
                             static_cast<unsigned long long>(o.detail));
            }
        }
    }
    x64::SetTrapsRecoverable(false);
    std::printf("static initializers: %d ran, %d stopped at a function with no body, %d faulted; %zu exit functions registered\n",
                ran, trapped, faulted, x64crt::ExitListSize());
    if (!missing.empty()) {
        std::printf("the functions with no body they reached (initializers stopped):\n");
        for (auto& [address, count] : missing) {
            std::printf("  0x%08X  %d\n", unsigned(address), count);
        }
    }
    if (!faults.empty()) {
        std::printf("the faults:\n");
        for (auto& [where, count] : faults) {
            std::printf("  %4d  %s (address %llX)\n", count, where.c_str(), static_cast<unsigned long long>(faultSample[where]));
        }
    }
    return (trapped || faulted) ? 1 : 0;
}

// The display drivers start threads while DirectDraw has the adapter open and are unloaded when
// it's released (the game's video-memory query, at start-up): a thread that starts after its DLL
// went runs into whatever was mapped there next (d3d9.dll, 2026-10-31). Every DLL loaded from
// here on stays loaded (pinned from the loader's notification).
struct DllLoadedData {
    ULONG flags;
    const void* fullName;
    const void* baseName;
    void* base;
    ULONG size;
};
VOID CALLBACK OnDllNotification(ULONG reason, const DllLoadedData* data, PVOID) {
    if (reason == 1 && data != nullptr) {   // LDR_DLL_NOTIFICATION_REASON_LOADED
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           static_cast<LPCWSTR>(data->base), &module);
    }
}

void PinDllsFromNowOn() {
    using Register = LONG(NTAPI*)(ULONG, VOID(CALLBACK*)(ULONG, const DllLoadedData*, PVOID), PVOID, PVOID*);
    auto reg = reinterpret_cast<Register>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrRegisterDllNotification"));
    static PVOID cookie = nullptr;
    if (reg != nullptr) {
        reg(0, &OnDllNotification, nullptr, &cookie);
    }
}

// A thread another process starts in ours with 32-bit code (bullymenu attaches to any window of
// the game's class, "Gamebryo Application", and runs its x86 patches with CreateRemoteThread):
// its start address is in memory no module of ours holds. Its code is made a 64-bit `return 0`
// before it runs (a TLS callback sees every new thread first), so the thread ends on its own:
// ending it here would leave the loader lock (held around TLS callbacks) owned by a dead
// thread, and the next DLL load (the graphics driver's) waits forever.
void NTAPI OnThreadStart(PVOID, DWORD reason, PVOID) {
    if (reason != DLL_THREAD_ATTACH) {
        return;
    }
    using Query = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    static auto query = reinterpret_cast<Query>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread"));
    void* start = nullptr;
    if (query == nullptr || query(GetCurrentThread(), 9 /* ThreadQuerySetWin32StartAddress */, &start, sizeof start, nullptr) != 0) {
        return;
    }
    HMODULE module = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast<LPCWSTR>(start), &module)) {
        return;   // (a module's thread: the drivers', ours)
    }
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(start, &info, sizeof info) == sizeof info && info.Type == MEM_PRIVATE) {
        static const uint8_t kReturnZero[] = {0x33, 0xC0, 0xC3};   // xor eax, eax; ret
        DWORD old = 0;
        if (VirtualProtect(start, sizeof kReturnZero, PAGE_EXECUTE_READWRITE, &old)) {
            std::memcpy(start, kReturnZero, sizeof kReturnZero);
            FlushInstructionCache(GetCurrentProcess(), start, sizeof kReturnZero);
        }
        std::fprintf(stderr, "bullyre_x64: a thread another process started at %p (not in a module: an x86 payload, bullymenu's?): made to return\n",
                     start);
    }
}

// --keys: "K@S[:MS],..." (K a DirectInput code or a name, S seconds from the start, MS the hold)
void AddKeys(const char* list) {
    static const struct {
        const char* name;
        uint16_t code;
    } kNames[] = {{"ENTER", 0x1C}, {"ESC", 0x01},  {"SPACE", 0x39}, {"UP", 0xC8},   {"DOWN", 0xD0}, {"LEFT", 0xCB},
                  {"RIGHT", 0xCD}, {"LALT", 0x38}, {"W", 0x11},     {"A", 0x1E},    {"S", 0x1F},    {"D", 0x20},
                  {"LSHIFT", 0x2A}, {"TAB", 0x0F}, {"R", 0x13},     {"X", 0x2D},    {"C", 0x2E},    {"F", 0x21},
                  {"E", 0x12},     {"Q", 0x10},    {"M", 0x32},     {"1", 0x02},    {"2", 0x03},    {"3", 0x04},
                  {"4", 0x05},     {"LMB", 0x100}, {"RMB", 0x101},  {"MMB", 0x102}};
    std::string all(list);
    size_t at = 0;
    while (at < all.size()) {
        size_t end = all.find(',', at);
        std::string item = all.substr(at, end == std::string::npos ? std::string::npos : end - at);
        at = end == std::string::npos ? all.size() : end + 1;
        size_t sign = item.find('@');
        if (sign == std::string::npos) {
            continue;
        }
        std::string key = item.substr(0, sign);
        uint16_t code = uint16_t(std::strtoul(key.c_str(), nullptr, 0));
        for (const auto& n : kNames) {
            if (key == n.name) {
                code = n.code;
            }
        }
        double seconds = std::atof(item.c_str() + sign + 1);
        size_t colon = item.find(':', sign);
        uint32_t hold = colon == std::string::npos ? 150 : uint32_t(std::atoi(item.c_str() + colon + 1));
        re::AddScriptedKey(code, uint32_t(seconds * 1000.0), hold);
    }
}

} // namespace

#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:bullyre_x64_tls_callback")
#pragma const_seg(".CRT$XLB")
extern "C" const PIMAGE_TLS_CALLBACK bullyre_x64_tls_callback = &OnThreadStart;
#pragma const_seg()

int main(int argc, char** argv) {
    re::SetForceWindowed(true);   // (never the user's display mode: runflags.h)
    re::SetShadowMapSize(2048);   // (sharper shadows than the game's 1024)
    re::SetUltraWide(true);       // (screens wider than 16:9: runflags.h)
    re::SetSeamlessDoors(true);   // (no fade to black at doors: runflags.h)
    PinDllsFromNowOn();
    // the game's exe: next to ours (a player's copy, in the game's folder), else Steam's folder
    static char image[MAX_PATH] = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Bully Scholarship Edition\\Bully_unpacked.exe";
    {
        char beside[MAX_PATH];
        DWORD n = GetModuleFileNameA(nullptr, beside, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            std::strcpy(std::strrchr(beside, '\\') + 1, "Bully_unpacked.exe");
            if (GetFileAttributesA(beside) != INVALID_FILE_ATTRIBUTES) {
                std::strcpy(image, beside);
            }
        }
    }
    const char* functions = nullptr;   // (the starts are built in: game_starts.inc; --functions adds a list)
    bool report = false, trap = false, init = false, winmain = false;
    int seconds = 0;
    g_play = argc == 1;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--play")) {
            g_play = true;
        } else if (!std::strcmp(argv[i], "--init")) {
            init = true;
        } else if (!std::strcmp(argv[i], "--lenient")) {
            x64::SetTrapsLenient(true);
        } else if (!std::strcmp(argv[i], "--winmain")) {
            init = true;
            winmain = true;
        } else if (!std::strcmp(argv[i], "--seconds") && i + 1 < argc) {
            seconds = std::atoi(argv[++i]);
        } else if (!std::strcmp(argv[i], "--profile") && i + 1 < argc) {
            g_profileAt = std::atoi(argv[++i]);
        } else if (!std::strcmp(argv[i], "--teleport") && i + 1 < argc) {
            // "S:x,y,z,area[,heading];..." (S seconds on the keys' clock)
            for (const char* p = argv[++i]; p && *p;) {
                double s = 0;
                float x = 0, y = 0, z = 0, h = 0;
                int area = 0;
                int n = std::sscanf(p, "%lf:%f,%f,%f,%d,%f", &s, &x, &y, &z, &area, &h);
                if (n >= 5) {
                    re::AddScriptedTeleport(uint32_t(s * 1000.0), x, y, z, area, h);
                }
                p = std::strchr(p, ';');
                p = p ? p + 1 : nullptr;
            }
        } else if (!std::strcmp(argv[i], "--missions") && i + 1 < argc) {
            // "S:E:LIST": from S seconds on the keys' clock the missions in LIST (indexes and
            // ranges: 4,8,10-20) force-started one after another, E seconds each
            double s = 0, e = 60;
            int used = 0;
            const char* p = argv[++i];
            if (std::sscanf(p, "%lf:%lf:%n", &s, &e, &used) >= 2 && used > 0) {
                std::vector<int> missions;
                for (p += used; *p;) {
                    int first = 0, last = 0, n = 0;
                    if (std::sscanf(p, "%d-%d%n", &first, &last, &n) == 2) {
                    } else if (std::sscanf(p, "%d%n", &first, &n) == 1) {
                        last = first;
                    } else {
                        break;
                    }
                    for (int m = first; m <= last; m++) {
                        missions.push_back(m);
                    }
                    p += n;
                    if (*p == ',') {
                        p++;
                    }
                }
                re::AddScriptedMissions(uint32_t(s * 1000.0), uint32_t(e * 1000.0), missions.data(), int(missions.size()));
            }
        } else if (!std::strcmp(argv[i], "--start")) {
            re::SetEnterUntilStarted(true);
        } else if (!std::strcmp(argv[i], "--mission-pass") && i + 1 < argc) {
            re::SetScriptedMissionPass(uint32_t(std::atof(argv[++i]) * 1000.0));
        } else if (!std::strcmp(argv[i], "--tag") && i + 1 < argc) {
            re::SetOutputTag(argv[++i]);
            char cmd[64];
            std::snprintf(cmd, sizeof cmd, "bullygfx_cmd_%s.txt", argv[i]);
            SetEnvironmentVariableA("BULLYGFX_CMD", cmd);   // (bullygfx64's commands: this run's own file)
        } else if (!std::strcmp(argv[i], "--no-portal")) {
            re::SetPortals(false);
        } else if (!std::strcmp(argv[i], "--portal-full")) {
            re::SetPortalFull(true);
        } else if (!std::strcmp(argv[i], "--portal-test")) {
            re::SetPortalTest(true);
        } else if (!std::strcmp(argv[i], "--play-window")) {
            re::SetPlayWindow(true);   // (a test run with the player's window: sizing, shared input)
        } else if (!std::strcmp(argv[i], "--no-two-areas")) {
            re::SetTwoAreas(false);   // (seamless doors as before: only the current area peopled)
        } else if (!std::strcmp(argv[i], "--no-seamless-doors")) {
            re::SetSeamlessDoors(false);
        } else if (!std::strcmp(argv[i], "--doors")) {
            re::SetDoorReport(true);
        } else if (!std::strcmp(argv[i], "--use-door") && i + 1 < argc) {
            double s = 0;
            int door = 0;
            if (std::sscanf(argv[++i], "%lf:%d", &s, &door) == 2) {
                re::AddScriptedDoorUse(uint32_t(s * 1000.0), door);
            }
        } else if (!std::strcmp(argv[i], "--shadow-map") && i + 1 < argc) {
            re::SetShadowMapSize(uint16_t(std::clamp(std::atoi(argv[++i]), 256, 2048)));
        } else if (!std::strcmp(argv[i], "--shots-every") && i + 1 < argc) {
            g_shotEvery = std::atoi(argv[++i]);
        } else if (!std::strcmp(argv[i], "--quit") && i + 1 < argc) {
            re::AddScriptedQuit(uint32_t(std::atof(argv[++i]) * 1000.0));
        } else if (!std::strcmp(argv[i], "--keys") && i + 1 < argc) {
            AddKeys(argv[++i]);
        } else if (!std::strcmp(argv[i], "--blips")) {
            g_printBlips = true;
        } else if (!std::strcmp(argv[i], "--grabs") && i + 1 < argc) {
            std::sscanf(argv[++i], "%d:%d:%d", &g_grabFrom, &g_grabEvery, &g_grabCount);
        } else if (!std::strcmp(argv[i], "--keep-active")) {
            re::SetKeepActive(true);
        } else if (!std::strcmp(argv[i], "--report")) {
            report = true;
        } else if (!std::strcmp(argv[i], "--trap")) {
            trap = true;
        } else if (!std::strcmp(argv[i], "--image") && i + 1 < argc) {
            std::snprintf(image, sizeof image, "%s", argv[++i]);
        } else if (!std::strcmp(argv[i], "--game-dir") && i + 1 < argc) {
            const char* dir = argv[++i];
            size_t n = std::strlen(dir);
            bool slash = n > 0 && (dir[n - 1] == '\\' || dir[n - 1] == '/');
            std::snprintf(image, sizeof image, "%s%sBully_unpacked.exe", dir, slash ? "" : "\\");
        } else if (!std::strcmp(argv[i], "--functions") && i + 1 < argc) {
            functions = argv[++i];
        }
    }
    if (g_play) {
        init = true;
        winmain = true;
        re::SetPlayWindow(true);   // (an ordinary window: sized at its edges, input shared: runflags.h)
        re::SetKeepActive(true);   // (not paused or minimized when another window is in front)
        DWORD processes[2];
        if (GetConsoleProcessList(processes, 2) == 1) {
            FreeConsole();   // (started from Explorer: the console was ours alone)
        }
        SaveAccessibility();
    } else {
        re::SetBackground(true);   // (a test run's window never takes the focus: runflags.h)
    }
    SetUnhandledExceptionFilter(&OnUnhandledException);
    std::printf("bullyre_x64: %zu functions registered\n", re::RegisteredCount());
    if (report) {
        AddressReport();
    }
    // the game's folder (its exe's): current from here on (our files go there; the game finds its
    // data there), and bullygfx64's
    {
        char folder[MAX_PATH];
        std::snprintf(folder, sizeof folder, "%s", image);
        if (char* slash = std::strrchr(folder, '\\')) {
            *slash = '\0';
            SetCurrentDirectoryA(folder);
            SetEnvironmentVariableA("BULLYGFX_GAME_DIR", folder);
        }
    }
    if (!x64::LoadGameImage(image)) {
        std::printf("can't read the game's exe: %s\n", image);
        if (g_play) {
            char text[MAX_PATH + 128];
            std::snprintf(text, sizeof text, "Bully (64-bit) can't start: there's no game exe at\n%s\n\n(Bully_unpacked.exe, made by bullydumper, in the game's folder)", image);
            MessageBoxA(nullptr, text, "Bully (64-bit)", MB_OK | MB_ICONERROR);
        }
        return 2;
    }
    FILE* log = std::fopen(re::OutputName("log.txt"), "w");
    x64::DispatchCounts c = x64::InstallDispatch(functions, log);
    std::printf("dispatch: %d function starts given a stub: %d to our bodies (%d through an adapter), %d jump thunks, %d imports,"
                " %d traps; %d starts too close to the next for a stub\n",
                c.functions, c.ours, c.adapters, c.thunks, c.imports, c.traps, c.tooClose);
    int result = 0;
    if (!g_play) {
        CallTests(trap);
        std::printf("%d passed, %d failed\n", g_passed, g_failed);
        result = g_failed ? 1 : 0;
    }
    if (init) {
        result = RunInitializers(log);
    }
    if (winmain) {
        if (seconds > 0) {
            StartWatchdog(seconds);
        }
        char folder[MAX_PATH];
        std::snprintf(folder, sizeof folder, "%s", image);
        *std::strrchr(folder, '\\') = '\0';
        result = RunWinMain(log, folder);
    }
    if (log) {
        std::fclose(log);
    }
    return result;
}
