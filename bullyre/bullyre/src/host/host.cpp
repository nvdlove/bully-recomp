// bullyre_host.exe: Bully run from our own exe instead of Bully.exe. Step 4 of the road to
// 64-bit: our code as its own program, still using the original's libraries (Gamebryo,
// XACT, Lua, its C runtime), which live in the game's image.
//
// How:
// - The exe is linked at the game's base, 0x400000, with the game's whole image reserved as
//   the start of our .text (reserve.asm), so the game's sections sit at their own addresses;
//   our code follows at 0x20F3000.
// - HostEntry, the exe's entry point, runs before our C runtime. It reads the unpacked game
//   exe, copies its sections into the reserve, fills its import table, gives each section
//   the game's protection, and edits our in-memory header: the section table becomes the
//   game's four sections followed by ours, and the resources the game's. (The game's C
//   runtime asks the header which section a pointer is in before calling through it, and
//   its window icon is resource 101.) Then our C runtime starts: the static initializers,
//   which are the registrations.
// - WinMain does what bullyre.dll's DllMain does (re::Startup: our functions installed as
//   bullyre.ini says), then calls the game's entry point: its C runtime's startup, which
//   runs the game's WinMain and ends the process.
// - A launcher that patches the game (bullymenu) gets a window between mapping and running:
//   HandOverToLauncher.
//
// bullyre_host.exe [--image PATH] [--check]
//   --image PATH  the unpacked game exe (default: Bully_unpacked.exe next to the host, else
//                 Bully.unpacked.exe there)
//   --check       map, install and verify; write bullyre_host_check.txt next to the host and
//                 exit (0 when everything checks) without running the game

#include <windows.h>
#include <shellapi.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../log.h"
#include "../reversed.h"

extern "C" unsigned char g_game[];             // reserve.asm
extern "C" DWORD WinMainCRTStartup(LPVOID);    // our C runtime's own entry point

namespace {

constexpr uintptr_t kBase = 0x00400000;
constexpr uint32_t kSizeOfImage = 0x01CF3000;
constexpr uint32_t kReserve = kSizeOfImage - 0x1000;   // reserve.asm's size
constexpr uint32_t kTimeDateStamp = 0x493FEA7E;         // the build bullyre is written against
constexpr uintptr_t kEntry = 0x00860E31;                // the game's C runtime startup
constexpr uintptr_t kOursByDefault = 0x0050E8C0;        // CBaseModelInfo::Construct, Mode::Ours

// Filled by HostEntry before our C runtime starts, so constant-initialized only: nothing
// for the runtime's static initializers to reset.
struct State {
    bool check;
    wchar_t image[MAX_PATH];
    uint8_t* file;
    DWORD fileSize;
    const IMAGE_NT_HEADERS32* nt;   // the game's headers, in `file`
    char error[1024];
};
constinit State g{};

uint32_t AlignUp(uint32_t v, uint32_t a) {
    return (v + a - 1) & ~(a - 1);
}

void ExeDir(wchar_t* out) {
    DWORD n = GetModuleFileNameW(nullptr, out, MAX_PATH);
    while (n > 0 && out[n - 1] != L'\\') {
        n--;
    }
    out[n] = 0;
}

// (before our C runtime: kernel32 / user32 only)
void WriteCheckFile(const char* text) {
    wchar_t path[MAX_PATH];
    ExeDir(path);
    lstrcatW(path, L"bullyre_host_check.txt");
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(h, text, DWORD(lstrlenA(text)), &written, nullptr);
        CloseHandle(h);
    }
}

[[noreturn]] void Fail(const char* format, ...) {
    va_list args;
    va_start(args, format);
    wvsprintfA(g.error, format, args);
    va_end(args);
    if (g.check) {
        WriteCheckFile(g.error);
    } else {
        MessageBoxA(nullptr, g.error, "bullyre host", MB_OK | MB_ICONERROR);
    }
    ExitProcess(2);
}

void ParseArgs() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; argv && i < argc; i++) {
        if (lstrcmpiW(argv[i], L"--check") == 0) {
            g.check = true;
        } else if (lstrcmpiW(argv[i], L"--image") == 0 && i + 1 < argc) {
            lstrcpynW(g.image, argv[++i], MAX_PATH);
        }
    }
    LocalFree(argv);
    if (g.image[0] == 0) {
        ExeDir(g.image);
        lstrcatW(g.image, L"Bully_unpacked.exe");
        if (GetFileAttributesW(g.image) == INVALID_FILE_ATTRIBUTES) {
            ExeDir(g.image);
            lstrcatW(g.image, L"Bully.unpacked.exe");
        }
    }
}

void ReadImage() {
    HANDLE h = CreateFileW(g.image, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        Fail("can't open the game exe %ls (error %lu)", g.image, GetLastError());
    }
    g.fileSize = GetFileSize(h, nullptr);
    g.file = static_cast<uint8_t*>(VirtualAlloc(nullptr, g.fileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    DWORD got = 0;
    if (!g.file || !ReadFile(h, g.file, g.fileSize, &got, nullptr) || got != g.fileSize) {
        Fail("can't read %ls (error %lu)", g.image, GetLastError());
    }
    CloseHandle(h);

    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(g.file);
    if (g.fileSize < 0x1000 || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 0x800) {
        Fail("%ls isn't an exe", g.image);
    }
    g.nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(g.file + dos->e_lfanew);
    const IMAGE_NT_HEADERS32& nt = *g.nt;
    if (nt.Signature != IMAGE_NT_SIGNATURE || nt.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        Fail("%ls isn't a 32-bit exe", g.image);
    }
    if (nt.FileHeader.TimeDateStamp != kTimeDateStamp || nt.OptionalHeader.ImageBase != kBase ||
        nt.OptionalHeader.SizeOfImage != kSizeOfImage || nt.OptionalHeader.AddressOfEntryPoint != kEntry - kBase ||
        nt.FileHeader.NumberOfSections != 4) {
        Fail("%ls isn't the unpacked Bully build bullyre is written against (timestamp %08lX, size %08lX, entry "
             "%08lX, %u sections; expected %08lX, %08lX, %08lX, 4). Run bullydumper on the Steam Bully.exe.",
             g.image, nt.FileHeader.TimeDateStamp, nt.OptionalHeader.SizeOfImage,
             nt.OptionalHeader.AddressOfEntryPoint + kBase, unsigned(nt.FileHeader.NumberOfSections),
             kTimeDateStamp, kSizeOfImage, kEntry);
    }
    const IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(g.nt);
    for (int i = 0; i < 4; i++) {
        if (s[i].VirtualAddress < 0x1000 || s[i].VirtualAddress + s[i].Misc.VirtualSize > kSizeOfImage ||
            s[i].PointerToRawData + s[i].SizeOfRawData > g.fileSize) {
            Fail("%ls: section %d lies outside the image or the file", g.image, i);
        }
    }
}

void Map() {
    if (reinterpret_cast<uintptr_t>(g_game) != kBase + 0x1000) {
        Fail("the host's reserve is at %p, not at 0x401000 (reserve.asm / the link order)", g_game);
    }
    DWORD old = 0;
    if (!VirtualProtect(g_game, kReserve, PAGE_EXECUTE_READWRITE, &old)) {
        Fail("can't make the reserve writable (error %lu)", GetLastError());
    }
    const IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(g.nt);
    for (int i = 0; i < 4; i++) {
        uint32_t span = AlignUp(s[i].Misc.VirtualSize, 0x1000);
        uint32_t copy = s[i].SizeOfRawData < span ? s[i].SizeOfRawData : span;
        memcpy(reinterpret_cast<void*>(kBase + s[i].VirtualAddress), g.file + s[i].PointerToRawData, copy);
    }
}

void Imports() {
    const IMAGE_DATA_DIRECTORY& dir = g.nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    for (auto* d = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(kBase + dir.VirtualAddress); d->Name; d++) {
        const char* dll = reinterpret_cast<const char*>(kBase + d->Name);
        HMODULE h = LoadLibraryA(dll);
        if (!h) {
            DWORD e = GetLastError();
            Fail("the game's %s didn't load (error %lu)%s", dll, e,
                 e == ERROR_SXS_CANT_GEN_ACTCTX || e == ERROR_MOD_NOT_FOUND
                     ? ": the Visual C++ 2005 runtime (VCRedist\\vcredist_x86.exe in the game folder) or DirectX "
                       "(DirectX\\DXSETUP.exe) may be missing"
                     : "");
        }
        auto* names = reinterpret_cast<const uint32_t*>(kBase + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        auto* iat = reinterpret_cast<uint32_t*>(kBase + d->FirstThunk);
        for (size_t i = 0; names[i]; i++) {
            FARPROC p;
            if (IMAGE_SNAP_BY_ORDINAL32(names[i])) {
                p = GetProcAddress(h, MAKEINTRESOURCEA(IMAGE_ORDINAL32(names[i])));
                if (!p) {
                    Fail("%s has no export #%lu, which the game imports", dll, IMAGE_ORDINAL32(names[i]));
                }
            } else {
                const char* name = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(kBase + names[i])->Name;
                p = GetProcAddress(h, name);
                if (!p) {
                    Fail("%s has no %s, which the game imports", dll, name);
                }
            }
            iat[i] = uint32_t(reinterpret_cast<uintptr_t>(p));
        }
    }
}

DWORD Protection(DWORD characteristics) {
    bool x = characteristics & IMAGE_SCN_MEM_EXECUTE, w = characteristics & IMAGE_SCN_MEM_WRITE;
    return x ? (w ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ) : (w ? PAGE_READWRITE : PAGE_READONLY);
}

void Protect() {
    const IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(g.nt);
    for (int i = 0; i < 4; i++) {
        DWORD old = 0;
        if (!VirtualProtect(reinterpret_cast<void*>(kBase + s[i].VirtualAddress), AlignUp(s[i].Misc.VirtualSize, 0x1000),
                            Protection(s[i].Characteristics), &old)) {
            Fail("can't set section %d's protection (error %lu)", i, GetLastError());
        }
    }
}

// Our header, as code reading it should see the process: the game's four sections, then ours
// (our .text without the reserve at its start), and the game's resources.
void PatchHeader() {
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(kBase);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(kBase + dos->e_lfanew);
    IMAGE_SECTION_HEADER* ours = IMAGE_FIRST_SECTION(nt);
    WORD n = nt->FileHeader.NumberOfSections;
    if (n == 0 || ours[0].VirtualAddress != 0x1000 || ours[0].Misc.VirtualSize <= kReserve) {
        Fail("the host's .text doesn't start with the reserve");
    }
    IMAGE_SECTION_HEADER merged[32];
    WORD m = 0;
    if (n + 4 > 32 || reinterpret_cast<uint8_t*>(ours + n + 4) > reinterpret_cast<uint8_t*>(kBase + 0x1000)) {
        Fail("no room in the host's header for the game's sections");
    }
    const IMAGE_SECTION_HEADER* game = IMAGE_FIRST_SECTION(g.nt);
    for (int i = 0; i < 4; i++) {
        merged[m++] = game[i];
    }
    IMAGE_SECTION_HEADER code = ours[0];
    code.VirtualAddress += kReserve;
    code.Misc.VirtualSize -= kReserve;
    if (code.SizeOfRawData > kReserve) {
        code.PointerToRawData += kReserve;
        code.SizeOfRawData -= kReserve;
    }
    merged[m++] = code;
    for (WORD i = 1; i < n; i++) {
        merged[m++] = ours[i];
    }
    DWORD old = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(kBase), 0x1000, PAGE_READWRITE, &old)) {
        Fail("can't write the host's header (error %lu)", GetLastError());
    }
    memcpy(ours, merged, m * sizeof(IMAGE_SECTION_HEADER));
    nt->FileHeader.NumberOfSections = m;
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] =
        g.nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
    VirtualProtect(reinterpret_cast<void*>(kBase), 0x1000, old, &old);
}

// ---- --check

struct Report {
    FILE* f;
    int bad;
    void Line(bool ok, const char* format, ...) {
        char text[512];
        va_list args;
        va_start(args, format);
        vsnprintf(text, sizeof text, format, args);
        va_end(args);
        bad += !ok;
        if (f) {
            fprintf(f, "%s  %s\n", ok ? "ok  " : "FAIL", text);
        }
        re::Log("host check: %s  %s", ok ? "ok" : "FAIL", text);
    }
};

// As the game's C runtime asks it (_IsNonwritableInCurrentImage): the section of our module
// holding `p`, whether it's writable; -1 when no section holds it.
int SectionWritable(uintptr_t p) {
    auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(kBase + reinterpret_cast<const IMAGE_DOS_HEADER*>(kBase)->e_lfanew);
    const IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(nt);
    uintptr_t rva = p - kBase;
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (rva >= s[i].VirtualAddress && rva < s[i].VirtualAddress + s[i].Misc.VirtualSize) {
            return (s[i].Characteristics & IMAGE_SCN_MEM_WRITE) ? 1 : 0;
        }
    }
    return -1;
}

// Before re::Startup (which changes the code where it installs our functions).
void CheckMapped(Report& r) {
    const IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(g.nt);
    const IMAGE_DATA_DIRECTORY& iatDir = g.nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT];
    for (int i = 0; i < 4; i++) {
        uint32_t span = AlignUp(s[i].Misc.VirtualSize, 0x1000);
        uint32_t size = s[i].SizeOfRawData < span ? s[i].SizeOfRawData : span;
        const uint8_t* mem = reinterpret_cast<const uint8_t*>(kBase + s[i].VirtualAddress);
        const uint8_t* file = g.file + s[i].PointerToRawData;
        uint32_t differ = 0, first = 0;
        for (uint32_t k = 0; k < size; k++) {
            uint32_t rva = s[i].VirtualAddress + k;
            bool iat = rva >= iatDir.VirtualAddress && rva < iatDir.VirtualAddress + iatDir.Size;
            if (!iat && mem[k] != file[k] && differ++ == 0) {
                first = uint32_t(kBase) + rva;
            }
        }
        r.Line(differ == 0, "%.8s at 0x%08X: %u bytes as in the file (%u differ, first at 0x%08X)", s[i].Name,
               unsigned(kBase + s[i].VirtualAddress), size, differ, first);
        MEMORY_BASIC_INFORMATION mbi{};
        VirtualQuery(mem, &mbi, sizeof mbi);
        r.Line(mbi.Protect == Protection(s[i].Characteristics), "%.8s protection 0x%lX (expected 0x%lX)", s[i].Name,
               mbi.Protect, Protection(s[i].Characteristics));
    }
    // every import slot holds what the dll exports under that name
    const IMAGE_DATA_DIRECTORY& dir = g.nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    int slots = 0, wrong = 0;
    for (auto* d = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(kBase + dir.VirtualAddress); d->Name; d++) {
        const char* dll = reinterpret_cast<const char*>(kBase + d->Name);
        HMODULE h = GetModuleHandleA(dll);
        // (the names come from the file: the mapped thunks of a table without a separate
        // name list have been overwritten with the addresses)
        uint32_t namesRva = d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk;
        const IMAGE_SECTION_HEADER* rdata = &s[1];
        auto* names = reinterpret_cast<const uint32_t*>(g.file + rdata->PointerToRawData + (namesRva - rdata->VirtualAddress));
        auto* iat = reinterpret_cast<const uint32_t*>(kBase + d->FirstThunk);
        for (size_t i = 0; names[i]; i++, slots++) {
            FARPROC p = IMAGE_SNAP_BY_ORDINAL32(names[i])
                            ? GetProcAddress(h, MAKEINTRESOURCEA(IMAGE_ORDINAL32(names[i])))
                            : GetProcAddress(h, reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(kBase + names[i])->Name);
            wrong += !h || !p || iat[i] != uint32_t(reinterpret_cast<uintptr_t>(p));
        }
    }
    r.Line(slots > 0 && wrong == 0, "imports: %d slots filled, %d wrong", slots, wrong);
    // what the game's C runtime sees in the header
    r.Line(SectionWritable(0x00401000) == 0, "the game's .text reads as not writable");
    r.Line(SectionWritable(0x008FC000) == 0, "the game's .rdata reads as not writable (its _cinit calls _FPinit)");
    r.Line(SectionWritable(0x009C4000) == 1, "the game's .data reads as writable");
    r.Line(SectionWritable(reinterpret_cast<uintptr_t>(&SectionWritable)) == 0, "our code reads as a section of its own");
    HMODULE self = GetModuleHandleA(nullptr);
    r.Line(FindResourceA(self, MAKEINTRESOURCEA(101), MAKEINTRESOURCEA(14)) != nullptr, "the game's icon group (101) is found");
    r.Line(LoadIconA(self, MAKEINTRESOURCEA(101)) != nullptr, "the game's icon loads");
    r.Line(*reinterpret_cast<const uint8_t*>(kEntry) == 0xE8, "the game's entry point starts with its call to __security_init_cookie");
}

void CheckInstalled(Report& r) {
    const uint8_t* at = reinterpret_cast<const uint8_t*>(kOursByDefault);
    r.Line(at[0] == 0xE9, "CBaseModelInfo::Construct (ours by default) jumps away: %02X", at[0]);
}

// A launcher patching the game (bullymenu: windowed mode, free mouse...) can't do it before
// the game is mapped. One that started the host suspended creates two events named after
// its process id; the host, once mapped, sets the first and waits for the second while the
// launcher writes its patches. Started any other way, the events don't exist: no wait.
void HandOverToLauncher() {
    wchar_t mapped[64], go[64];
    wsprintfW(mapped, L"Local\\bullyre_host_mapped_%lu", GetCurrentProcessId());
    wsprintfW(go, L"Local\\bullyre_host_go_%lu", GetCurrentProcessId());
    HANDLE m = OpenEventW(EVENT_MODIFY_STATE, FALSE, mapped);
    HANDLE g2 = OpenEventW(SYNCHRONIZE, FALSE, go);
    if (m && g2) {
        SetEvent(m);
        if (WaitForSingleObject(g2, 60000) != WAIT_OBJECT_0) {
            Fail("the launcher didn't finish patching the game within a minute");
        }
    }
    if (m) {
        CloseHandle(m);
    }
    if (g2) {
        CloseHandle(g2);
    }
}

} // namespace

extern "C" DWORD HostEntry(LPVOID peb) {
    ParseArgs();
    ReadImage();
    Map();
    Imports();
    Protect();
    PatchHeader();
    HandOverToLauncher();
    return WinMainCRTStartup(peb);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    re::Log("---- bullyre host: the game mapped from %ls", g.image);
    if (g.check) {
        char path[MAX_PATH];
        DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
        while (n > 0 && path[n - 1] != '\\') {
            n--;
        }
        strcpy_s(path + n, MAX_PATH - n, "bullyre_host_check.txt");
        Report r{fopen(path, "w"), 0};
        CheckMapped(r);
        re::Startup();
        CheckInstalled(r);
        r.Line(r.bad == 0, "%d failed", r.bad);
        if (r.f) {
            fclose(r.f);
        }
        return r.bad == 0 ? 0 : 1;
    }
    re::Startup();
    VirtualFree(g.file, 0, MEM_RELEASE);
    re::Log("bullyre host: running the game");
    reinterpret_cast<void(__cdecl*)()>(kEntry)();
    return 0;   // (not reached: the game's runtime ends the process)
}
