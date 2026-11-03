// The game's C runtime functions that keep state (its heap, rand, files, exit list, C++
// exception objects), ours in the 64-bit build: registered at the game's addresses for the
// dispatch (src/x64/dispatch.cpp). The 32-bit build keeps the game's own (src/game/Crt.h:
// "still the game's ... those switch with the rest at the 64-bit step"), so these are only in
// bullyre_x64. The game's heap is the low heap (src/x64/lowheap.cpp: below 2 GB), so the
// pointers fit the game's 4-byte fields.

#include <windows.h>

#include <intrin.h>
#include <malloc.h>
#include <fcntl.h>
#include <io.h>

#include <cerrno>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "reversed.h"
#include "x64/crt64.h"
#include "x64/lowheap.h"

namespace x64crt {

// ---- the exit list

std::vector<void(__cdecl*)()>& ExitList() {
    static std::vector<void(__cdecl*)()> list;
    return list;
}

// 0x85B8EA: atexit (the static initializers give it their globals' destructors)
int __cdecl Atexit(void(__cdecl* fn)()) {
    ExitList().push_back(fn);
    return 0;
}

void RunExitList() {
    auto& list = ExitList();
    while (!list.empty()) {
        auto fn = list.back();
        list.pop_back();
        fn();
    }
}

size_t ExitListSize() {
    return ExitList().size();
}

// 0x85BB8C: exit (the exit list, then the process)
void __cdecl Exit(int code) {
    std::fprintf(stderr, "x64 crt: exit(%d) from %p\n", code, _ReturnAddress());
    RunExitList();
    std::fflush(nullptr);
    ExitProcess(UINT(code));
}

// 0x869B9F: ___initmbctable (the multibyte code page's tables: ours are our runtime's)
int __cdecl InitMbcTable() {
    return 0;
}

// 0x860F3A: _strnicmp (some files call the game's copy by its address)
int __cdecl Strnicmp(const char* a, const char* b, size_t count) {
    return _strnicmp(a, b, count);
}

// 0x85F5DF: _stricmp (NiStricmp, 0x753820, is a lone jump to it)
int __cdecl Stricmp(const char* a, const char* b) {
    return _stricmp(a, b);
}

// 0x85DD0D, 0x860796: fprintf and fputs (TinyXml's printing, to our runtime's FILEs)
FILE* Unboxed(void* p);   // (the files, below)
int __cdecl Fprintf(FILE* file, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int n = std::vfprintf(Unboxed(file), format, args);
    va_end(args);
    return n;
}
int __cdecl Fputs(const char* text, FILE* file) {
    return std::fputs(text, Unboxed(file));
}

// ---- the heap: the low heap (src/x64/lowheap.cpp: below 2 GB, where the game's 4-byte
// pointers reach)

void* __cdecl Malloc(size_t size) {
    return re::low::Alloc(size);
}
void __cdecl Free(void* p) {
    re::low::Free(p);
}
void* __cdecl Realloc(void* p, size_t size) {
    return re::low::Realloc(p, size);
}
// _aligned_malloc's: the block's start kept just before the aligned pointer
void* __cdecl AlignedMalloc(size_t size, size_t alignment) {
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }
    auto* raw = static_cast<uint8_t*>(re::low::Alloc(size + alignment + sizeof(void*)));
    if (!raw) {
        return nullptr;
    }
    uintptr_t aligned = (reinterpret_cast<uintptr_t>(raw) + sizeof(void*) + alignment - 1) & ~uintptr_t(alignment - 1);
    reinterpret_cast<void**>(aligned)[-1] = raw;
    return reinterpret_cast<void*>(aligned);
}
void __cdecl AlignedFree(void* p) {
    if (p) {
        re::low::Free(static_cast<void**>(p)[-1]);
    }
}
void* __cdecl AlignedRealloc(void* p, size_t size, size_t alignment) {
    if (!p) {
        return AlignedMalloc(size, alignment);
    }
    if (size == 0) {
        AlignedFree(p);
        return nullptr;
    }
    void* raw = static_cast<void**>(p)[-1];
    size_t old = re::low::Capacity(raw) - (reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(raw));
    void* q = AlignedMalloc(size, alignment);
    if (q) {
        std::memcpy(q, p, old < size ? old : size);
        AlignedFree(p);
    }
    return q;
}

// ---- rand: VC8's, per thread (holdrand = holdrand * 214013 + 2531011; bits 16..30)

thread_local uint32_t t_holdrand = 1;

int __cdecl Rand() {
    t_holdrand = t_holdrand * 214013u + 2531011u;
    return int((t_holdrand >> 16) & 0x7FFF);
}
void __cdecl Srand(unsigned int seed) {
    t_holdrand = seed;
}

// ---- OpenMP (the 32-bit vcomp.dll's three imports): the game's parallel regions run on the
// calling thread, which gets the whole of each loop (one thread's share)

void __cdecl VcompForStaticSimpleInit(int first, int last, int, int, int* lower, int* upper) {
    *lower = first;
    *upper = last;
}
void __cdecl VcompForStaticEnd() {}
void __cdecl VcompFork(int, int argCount, void* region, ...) {
    uintptr_t a[8] = {};
    va_list ap;
    va_start(ap, region);
    for (int i = 0; i < argCount && i < 8; i++) {
        a[i] = va_arg(ap, uintptr_t);
    }
    va_end(ap);
    // (the region's arguments are pointers and integers: extra ones are harmless)
    using Region = void(__cdecl*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
    reinterpret_cast<Region>(region)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
}

// ---- files (our runtime's FILEs, each handed to the game in a box of the low heap: the game
// keeps its FILE* in 4-byte fields (NiFile's), and our runtime's FILE may be anywhere)

struct GameFile {
    uint32_t magic;   // kGameFileMagic
    uint32_t pad;
    FILE* file;
};
constexpr uint32_t kGameFileMagic = 0x454C4946;   // "FILE"

FILE* Boxed(FILE* file) {
    if (!file) {
        return nullptr;
    }
    auto* box = static_cast<GameFile*>(re::low::Alloc(sizeof(GameFile)));
    if (!box) {
        std::fclose(file);
        return nullptr;
    }
    box->magic = kGameFileMagic;
    box->pad = 0;
    box->file = file;
    return reinterpret_cast<FILE*>(box);
}
// the FILE in a box (or one of our runtime's given as it is: our code's stdout, stderr)
FILE* Unboxed(void* p) {
    auto* box = static_cast<GameFile*>(p);
    if (box && re::low::Owns(box) && box->magic == kGameFileMagic) {
        return box->file;
    }
    return static_cast<FILE*>(p);
}

// A file opened as the game's runtime opens one: CreateFileA through the game's import slot
// (0x8FC064: bullygfx's mod loader hooks it there and sends a mod's textures, named by paths of
// the mod author's machine, to its cache), the handle then made a FILE of ours. errno set and
// nullptr when it can't be opened.
FILE* OpenThroughGame(const char* name, const char* mode) {
    if (!name || !mode || !*mode) {
        errno = EINVAL;
        return nullptr;
    }
    DWORD access = 0, disposition = 0;
    int flags = 0;
    switch (mode[0]) {
    case 'r':
        access = GENERIC_READ, disposition = OPEN_EXISTING, flags = _O_RDONLY;
        break;
    case 'w':
        access = GENERIC_WRITE, disposition = CREATE_ALWAYS, flags = _O_WRONLY | _O_CREAT | _O_TRUNC;
        break;
    case 'a':
        access = GENERIC_WRITE, disposition = OPEN_ALWAYS, flags = _O_WRONLY | _O_CREAT | _O_APPEND;
        break;
    default:
        errno = EINVAL;
        return nullptr;
    }
    flags |= _O_TEXT;
    for (const char* m = mode + 1; *m; m++) {
        if (*m == '+') {
            access = GENERIC_READ | GENERIC_WRITE;
            flags = (flags & ~(_O_RDONLY | _O_WRONLY)) | _O_RDWR;
        } else if (*m == 'b') {
            flags = (flags & ~_O_TEXT) | _O_BINARY;
        }
    }
    using CreateFileA_t = HANDLE(WINAPI*)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    auto create = re::GameFnAt<CreateFileA_t>(0x008FC064);
    ULONGLONG started = GetTickCount64();
    HANDLE h = create(name, access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    {
        // (opens that take long said, and every 2000th the opens' total: where a load's time goes)
        static ULONGLONG opens = 0, total = 0;
        ULONGLONG took = GetTickCount64() - started;
        total += took;
        if (took >= 500) {
            std::fprintf(stderr, "x64 crt: opening \"%s\" took %llu ms (%s)\n", name, took, h == INVALID_HANDLE_VALUE ? "not there" : "opened");
        }
        if (++opens % 2000 == 0) {
            std::fprintf(stderr, "x64 crt: %llu opens through the game's CreateFileA, %llu ms in all\n", opens, total);
        }
    }
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        errno = (e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND) ? ENOENT : e == ERROR_ACCESS_DENIED ? EACCES : EINVAL;
        return nullptr;
    }
    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(h), flags);
    if (fd < 0) {
        CloseHandle(h);
        return nullptr;
    }
    FILE* f = _fdopen(fd, mode);
    if (!f) {
        _close(fd);
    }
    return f;
}

FILE* __cdecl Fopen(const char* name, const char* mode) {
    return Boxed(OpenThroughGame(name, mode));
}
char* __cdecl Fgets(char* buffer, int size, FILE* file) {
    return std::fgets(buffer, size, Unboxed(file));
}
int __cdecl Fclose(FILE* file) {
    FILE* f = Unboxed(file);
    if (f != file) {
        reinterpret_cast<GameFile*>(file)->magic = 0;
        re::low::Free(file);
    }
    return std::fclose(f);
}
int __cdecl FopenS(re::Ptr32<void>* out, const char* name, const char* mode) {
    if (!out) {
        return EINVAL;
    }
    errno = 0;
    FILE* f = OpenThroughGame(name, mode);
    *out = Boxed(f);
    return f ? 0 : (errno ? errno : EINVAL);
}
size_t __cdecl Fread(void* buffer, size_t size, size_t count, void* file) {
    return std::fread(buffer, size, count, Unboxed(file));
}
size_t __cdecl Fwrite(const void* buffer, size_t size, size_t count, void* file) {
    return std::fwrite(buffer, size, count, Unboxed(file));
}
int __cdecl Fseek(void* file, long offset, int origin) {
    return std::fseek(Unboxed(file), offset, origin);
}
long __cdecl Ftell(void* file) {
    return std::ftell(Unboxed(file));
}

// ---- std::exception in the game's layout (its vtable 0x98CC14, what, whether what is ours)

struct GameException {
    re::Ptr32<void> vtable;
    re::Ptr32<const char> what;
    int32_t doFree;
};
static_assert(sizeof(GameException) == 12);
constexpr uintptr_t kExceptionVtable = 0x0098CC14;

void __fastcall ExceptionConstruct(GameException* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kExceptionVtable);
    self->what = nullptr;
    self->doFree = 0;
}
void __fastcall ExceptionConstructMessage(GameException* self, re::Edx, const re::Ptr32<const char>* message) {
    self->vtable = reinterpret_cast<void*>(kExceptionVtable);
    self->what = nullptr;
    self->doFree = 0;
    if (*message) {
        size_t n = std::strlen(*message) + 1;
        char* copy = static_cast<char*>(Malloc(n));
        if (copy) {
            std::memcpy(copy, *message, n);
            self->what = copy;
            self->doFree = 1;
        }
    }
}
void __fastcall ExceptionCopy(GameException* self, re::Edx, const GameException* other) {
    re::Ptr32<const char> what = other->what;
    const re::Ptr32<const char>* p = &what;
    if (other->doFree) {
        ExceptionConstructMessage(self, nullptr, p);
    } else {
        self->vtable = reinterpret_cast<void*>(kExceptionVtable);
        self->what = other->what;
        self->doFree = 0;
    }
}
void __fastcall ExceptionDestruct(GameException* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kExceptionVtable);
    if (self->doFree) {
        Free(const_cast<char*>(static_cast<const char*>(self->what)));
    }
}

// A C++ exception thrown by the game's code (bad_alloc, XACT's errors, length_error): its
// ThrowInfo is the 32-bit runtime's, which x64's exception handling can't use, and nothing of
// ours catches these. Reported, then the process ends.
[[noreturn]] void Fatal(const char* what) {
    std::fprintf(stderr, "x64 crt: %s\n", what);
    std::fflush(nullptr);
    ExitProcess(4);
}
void __stdcall CxxThrowException(void* object, const void* throwInfo) {
    char text[256];
    const GameException* e = static_cast<const GameException*>(object);
    std::snprintf(text, sizeof text, "the game's code threw a C++ exception (ThrowInfo %p, what \"%s\")", throwInfo,
                  e && e->what ? static_cast<const char*>(e->what) : "");
    Fatal(text);
}
void __cdecl Xlen() {
    Fatal("std::_Xlen: length_error");
}
void __cdecl Xran() {
    Fatal("std::_Xran: out_of_range");
}
void __cdecl InvalidParameter() {
    Fatal("_invalid_parameter: a C runtime function was given a bad argument");
}

// The C runtime's own initializers at the end of the game's table (0x8FC350): its onexit
// table, its stdio, the FDIV-bug test, the exception filter, the SSE2 flag, the console
// handle. Our runtime has its own; these do nothing in the 64-bit build.
void __cdecl CrtInitializerNotNeeded() {}

} // namespace x64crt

RE_UNHOOKED_FN("_atexit (x64)", 0x0085B8EA, &x64crt::Atexit);
RE_UNHOOKED_FN("_exit (x64)", 0x0085BB8C, &x64crt::Exit);
RE_UNHOOKED_FN("___initmbctable (x64)", 0x00869B9F, &x64crt::InitMbcTable);
RE_UNHOOKED_FN("_malloc (x64)", 0x0085E17F, &x64crt::Malloc);
RE_UNHOOKED_FN("_free (x64)", 0x0085F217, &x64crt::Free);
RE_UNHOOKED_FN("_realloc (x64)", 0x00861487, &x64crt::Realloc);
RE_UNHOOKED_FN("_aligned_malloc (x64)", 0x0085FA22, &x64crt::AlignedMalloc);
RE_UNHOOKED_FN("_aligned_free (x64)", 0x0085FA0D, &x64crt::AlignedFree);
RE_UNHOOKED_FN("_aligned_realloc (x64)", 0x0085FBA3, &x64crt::AlignedRealloc);
RE_UNHOOKED_FN("_rand (x64)", 0x0085C6FA, &x64crt::Rand);
RE_UNHOOKED_FN("_vcomp_for_static_end (x64)", 0x00877BD8, &x64crt::VcompForStaticEnd);
RE_UNHOOKED_FN("_vcomp_for_static_simple_init (x64)", 0x00877BDE, &x64crt::VcompForStaticSimpleInit);
RE_UNHOOKED_FN("_vcomp_fork (x64)", 0x00877BE4, &x64crt::VcompFork);
RE_UNHOOKED_FN("_srand (x64)", 0x0085C6ED, &x64crt::Srand);
RE_UNHOOKED_FN("_fopen (x64)", 0x0085B763, &x64crt::Fopen);
RE_UNHOOKED_FN("_fgets (x64)", 0x0085B518, &x64crt::Fgets);
RE_UNHOOKED_FN("_fclose (x64)", 0x0085B49C, &x64crt::Fclose);
RE_UNHOOKED_FN("_fopen_s (x64)", 0x0085B776, &x64crt::FopenS);
RE_UNHOOKED_FN("_fread (x64)", 0x0085EF57, &x64crt::Fread);
RE_UNHOOKED_FN("_fwrite (x64)", 0x0085F455, &x64crt::Fwrite);
RE_UNHOOKED_FN("_fseek (x64)", 0x0085FEB3, &x64crt::Fseek);
RE_UNHOOKED_FN("_ftell (x64)", 0x0085FDBE, &x64crt::Ftell);
RE_UNHOOKED_FN("std::exception::exception (x64)", 0x0085BDD1, &x64crt::ExceptionConstruct);
RE_UNHOOKED_FN("std::exception::exception(what) (x64)", 0x0085BDE2, &x64crt::ExceptionConstructMessage);
RE_UNHOOKED_FN("std::exception::exception(copy) (x64)", 0x0085BE48, &x64crt::ExceptionCopy);
RE_UNHOOKED_FN("std::exception::~exception (x64)", 0x0085BEA0, &x64crt::ExceptionDestruct);
RE_UNHOOKED_FN("__CxxThrowException (x64)", 0x0085C0BF, &x64crt::CxxThrowException);
RE_UNHOOKED_FN("std::_Xlen (x64)", 0x0085ABC9, &x64crt::Xlen);
RE_UNHOOKED_FN("std::_Xran (x64)", 0x0085AC08, &x64crt::Xran);
RE_UNHOOKED_FN("_invalid_parameter (x64)", 0x0085C0AF, &x64crt::InvalidParameter);
RE_UNHOOKED_FN("C runtime init 0x85B87F (onexit table, x64: ours)", 0x0085B87F, &x64crt::CrtInitializerNotNeeded);
RE_UNHOOKED_FN("C runtime init 0x85EF78 (stdio, x64: ours)", 0x0085EF78, &x64crt::CrtInitializerNotNeeded);
RE_UNHOOKED_FN("C runtime init 0x85F029 (stdio, x64: ours)", 0x0085F029, &x64crt::CrtInitializerNotNeeded);
RE_UNHOOKED_FN("C runtime init 0x86263F (floating point, x64: ours)", 0x0086263F, &x64crt::CrtInitializerNotNeeded);
RE_UNHOOKED_FN("C runtime init 0x871826 (exception filter, x64: ours)", 0x00871826, &x64crt::CrtInitializerNotNeeded);
RE_UNHOOKED_FN("C runtime init 0x872592 (SSE2 flag, x64: ours)", 0x00872592, &x64crt::CrtInitializerNotNeeded);
RE_UNHOOKED_FN("C runtime init 0x877A79 (console, x64: ours)", 0x00877A79, &x64crt::CrtInitializerNotNeeded);
RE_UNHOOKED_FN("_strnicmp (x64)", 0x00860F3A, &x64crt::Strnicmp);
RE_UNHOOKED_FN("_stricmp (x64)", 0x0085F5DF, &x64crt::Stricmp);
RE_UNHOOKED_FN("_fprintf (x64)", 0x0085DD0D, &x64crt::Fprintf);
RE_UNHOOKED_FN("_fputs (x64)", 0x00860796, &x64crt::Fputs);
