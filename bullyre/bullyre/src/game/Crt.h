#pragma once

// The C runtime as the game's code uses it: one place for all of it (Crt.cpp).
//
// What has no state of its own (strings, memory, formatting and parsing, character classes,
// maths) is ours: the C runtime we're built with, in place of the game's copy of VC8's
// (0x85A000..0x877000). Where the two can differ in a last bit (the maths' last place, a
// float printed at an exact tie), ours is what the 64-bit build will have.
//
// What keeps state the game's own code shares (the heap its blocks come from, the rand
// sequence, the FILE table, the exit list, the C++ exception objects its handlers catch) is
// still the game's, while any of the game's code runs: those switch with the rest at the
// 64-bit step.

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "reversed.h"

namespace crt {

// strings
char* __cdecl Strchr(const char* s, int c);
char* __cdecl Strrchr(const char* s, int c);
char* __cdecl Strstr(const char* text, const char* part);
size_t __cdecl Strcspn(const char* s, const char* set);
int __cdecl Strcmp(const char* a, const char* b);
int __cdecl Strncmp(const char* a, const char* b, size_t n);
int __cdecl Stricmp(const char* a, const char* b);   // _stricmp
int __cdecl Strnicmp(const char* a, const char* b, size_t n);   // _strnicmp
char* __cdecl Strncpy(char* to, const char* from, size_t n);
char* __cdecl Strncat(char* to, const char* from, size_t n);
char* __cdecl Strtok(char* s, const char* delimiters);
int __cdecl StrcpyS(char* to, size_t size, const char* from);
int __cdecl StrcatS(char* to, size_t size, const char* from);
int __cdecl StrncpyS(char* to, size_t size, const char* from, size_t count);
wchar_t* __cdecl Wcsstr(const wchar_t* s, const wchar_t* find);
int __cdecl WcscpyS(wchar_t* to, size_t size, const wchar_t* from);
size_t __cdecl Mbstowcs(wchar_t* to, const char* from, size_t count);
int __cdecl Islower(int c);
int __cdecl Isspace(int c);
int __cdecl Tolower(int c);
char* __cdecl Strupr(char* s);   // _strupr (0x8D878D: in place)
char* __cdecl Strlwr(char* s);   // _strlwr (0x8D89A2: in place)

// memory
void* __cdecl Memcpy(void* to, const void* from, size_t size);
void* __cdecl Memset(void* p, int value, size_t size);
int __cdecl MemcpyS(void* to, size_t size, const void* from, size_t count);
int __cdecl MemmoveS(void* to, size_t size, const void* from, size_t count);

// formatting and parsing (the _snprintf family: no terminator and -1 when it doesn't fit)
int __cdecl Sprintf(char* buffer, const char* format, ...);
int __cdecl SprintfS(char* buffer, size_t size, const char* format, ...);
int __cdecl Snprintf(char* buffer, size_t size, const char* format, ...);   // _snprintf
int __cdecl Vsnprintf(char* buffer, size_t size, const char* format, va_list args);   // _vsnprintf
int __cdecl VsnprintfS(char* buffer, size_t size, size_t count, const char* format, va_list args);   // _vsnprintf_s
int __cdecl Sscanf(const char* s, const char* format, ...);
int __cdecl Swscanf(const wchar_t* s, const wchar_t* format, ...);
long __cdecl Atol(const char* s);
double __cdecl Atof(const char* s);

// sorting and searching
void __cdecl Qsort(void* base, size_t count, size_t width, int(__cdecl* compare)(const void*, const void*));
void* __cdecl Bsearch(const void* key, const void* base, size_t count, size_t width,
                      int(__cdecl* compare)(const void*, const void*));

// the file system
int __cdecl Mkdir(const char* path);   // _mkdir

// maths (the game's code reaches the VC8 ones on the x87 stack: the _CI* entries at 0x85AD80..)
double __cdecl Cos(double x);
double __cdecl Sin(double x);
double __cdecl Tan(double x);
double __cdecl Asin(double x);
double __cdecl Acos(double x);
double __cdecl Atan(double x);
double __cdecl Atan2(double y, double x);
double __cdecl Pow(double x, double y);
double __cdecl Sqrt(double x);
double __cdecl Floor(double x);
double __cdecl Ceil(double x);
// float to int as the game's compiler does it (__ftol2, 0x85C720): through a 64-bit integer,
// the low half kept (so out of range isn't 0x80000000 as a plain cast's would be)
int __cdecl FloatToInt(double x);
// the long division helper (__alldiv, 0x85BC20)
int64_t __stdcall Alldiv(int64_t a, int64_t b);

// float versions for the game's float maths: the double result rounded, as its code stores it
inline float Cosf(float x) {
    return float(Cos(x));
}
inline float Sinf(float x) {
    return float(Sin(x));
}
inline float Tanf(float x) {
    return float(Tan(x));
}
inline float Asinf(float x) {
    return float(Asin(x));
}
inline float Acosf(float x) {
    return float(Acos(x));
}
inline float Atanf(float x) {
    return float(Atan(x));
}
inline float Atan2f(float y, float x) {
    return float(Atan2(y, x));
}
inline float Powf(float x, float y) {
    return float(Pow(x, y));
}
inline float Sqrtf(float x) {
    return float(Sqrt(x));
}

// ---- the game's, while its code runs (state it shares with us)

// its heap (malloc 0x85E17F, free 0x85F217, realloc, the aligned blocks Gamebryo's allocator
// takes from it)
GAME_FN(void*, __cdecl, Malloc, 0x0085E17F, size_t size);
GAME_FN(void, __cdecl, Free, 0x0085F217, void* p);
GAME_FN(void*, __cdecl, Realloc, 0x00861487, void* p, size_t size);
GAME_FN(void*, __cdecl, AlignedMalloc, 0x0085FA22, size_t size, size_t alignment);   // _aligned_malloc
GAME_FN(void, __cdecl, AlignedFree, 0x0085FA0D, void* p);                           // _aligned_free
GAME_FN(void*, __cdecl, AlignedRealloc, 0x0085FBA3, void* p, size_t size, size_t alignment);
// its rand sequence (rand 0x85C6FA, srand 0x85C6ED; Lua's math.random draws from it too)
GAME_FN(int, __cdecl, Rand, 0x0085C6FA);
GAME_FN(void, __cdecl, Srand, 0x0085C6ED, unsigned int seed);
// its FILE table
GAME_FN(FILE*, __cdecl, Fopen, 0x0085B763, const char* name, const char* mode);
GAME_FN(char*, __cdecl, Fgets, 0x0085B518, char* buffer, int size, FILE* file);
GAME_FN(int, __cdecl, Fclose, 0x0085B49C, FILE* file);
// (NiFile's: the file as void*)
GAME_FN(int, __cdecl, FopenS, 0x0085B776, re::Ptr32<void>* file, const char* name, const char* mode);   // fopen_s
GAME_FN(size_t, __cdecl, Fread, 0x0085EF57, void* buffer, size_t size, size_t count, void* file);
GAME_FN(size_t, __cdecl, Fwrite, 0x0085F455, const void* buffer, size_t size, size_t count, void* file);
GAME_FN(int, __cdecl, Fseek, 0x0085FEB3, void* file, long offset, int origin);
GAME_FN(long, __cdecl, Ftell, 0x0085FDBE, void* file);
// its exit list
GAME_FN(int, __cdecl, Atexit, 0x0085B8EA, void(__cdecl* fn)());
GAME_FN(void, __cdecl, Exit, 0x0085BB8C, int code);
// its C++ runtime: exceptions the game's handlers catch (std::exception 0x85BDD1..0x85BEA0,
// _CxxThrowException 0x85C0BF, std::_Xlen / _Xran 0x85ABC9 / 0x85AC08), and the invalid
// parameter report its checked functions end in (0x85C0AF)
GAME_FN(void, __thiscall, ExceptionConstruct, 0x0085BDD1, void* self);
GAME_FN(void, __thiscall, ExceptionConstructMessage, 0x0085BDE2, void* self, const re::Ptr32<const char>* message);
GAME_FN(void, __thiscall, ExceptionCopy, 0x0085BE48, void* self, const void* other);
GAME_FN(void, __thiscall, ExceptionDestruct, 0x0085BEA0, void* self);
GAME_FN(void, __stdcall, CxxThrowException, 0x0085C0BF, void* object, const void* throwInfo);
GAME_FN(void, __cdecl, Xlen, 0x0085ABC9);
GAME_FN(void, __cdecl, Xran, 0x0085AC08);
GAME_FN(void, __cdecl, InvalidParameter, 0x0085C0AF);

} // namespace crt
