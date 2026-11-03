// The C runtime as the game's code uses it (Crt.h): ours, where it has no state of its own.

#include "game/Crt.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <direct.h>

namespace crt {

char* __cdecl Strchr(const char* s, int c) {
    return const_cast<char*>(std::strchr(s, c));
}
char* __cdecl Strrchr(const char* s, int c) {
    return const_cast<char*>(std::strrchr(s, c));
}
char* __cdecl Strstr(const char* text, const char* part) {
    return const_cast<char*>(std::strstr(text, part));
}
size_t __cdecl Strcspn(const char* s, const char* set) {
    return std::strcspn(s, set);
}
int __cdecl Strcmp(const char* a, const char* b) {
    return std::strcmp(a, b);
}
int __cdecl Strncmp(const char* a, const char* b, size_t n) {
    return std::strncmp(a, b, n);
}
int __cdecl Stricmp(const char* a, const char* b) {
    return _stricmp(a, b);
}
int __cdecl Strnicmp(const char* a, const char* b, size_t n) {
    return _strnicmp(a, b, n);
}
char* __cdecl Strncpy(char* to, const char* from, size_t n) {
    return std::strncpy(to, from, n);
}
char* __cdecl Strncat(char* to, const char* from, size_t n) {
    return std::strncat(to, from, n);
}
char* __cdecl Strtok(char* s, const char* delimiters) {
    return std::strtok(s, delimiters);
}
int __cdecl StrcpyS(char* to, size_t size, const char* from) {
    return strcpy_s(to, size, from);
}
int __cdecl StrcatS(char* to, size_t size, const char* from) {
    return strcat_s(to, size, from);
}
int __cdecl StrncpyS(char* to, size_t size, const char* from, size_t count) {
    return strncpy_s(to, size, from, count);
}
wchar_t* __cdecl Wcsstr(const wchar_t* s, const wchar_t* find) {
    return const_cast<wchar_t*>(std::wcsstr(s, find));
}
int __cdecl WcscpyS(wchar_t* to, size_t size, const wchar_t* from) {
    return wcscpy_s(to, size, from);
}
size_t __cdecl Mbstowcs(wchar_t* to, const char* from, size_t count) {
    return std::mbstowcs(to, from, count);
}
int __cdecl Islower(int c) {
    return std::islower(c);
}
int __cdecl Isspace(int c) {
    return std::isspace(c);
}
int __cdecl Tolower(int c) {
    return std::tolower(c);
}
// _strupr / _strlwr in the "C" locale (the game never sets another): ASCII letters only. (A null
// string is the game's invalid-parameter case: null back.)
char* __cdecl Strupr(char* s) {
    if (s == nullptr) {
        return nullptr;
    }
    for (char* p = s; *p != '\0'; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = char(*p - 0x20);
        }
    }
    return s;
}
char* __cdecl Strlwr(char* s) {
    if (s == nullptr) {
        return nullptr;
    }
    for (char* p = s; *p != '\0'; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = char(*p + 0x20);
        }
    }
    return s;
}

void* __cdecl Memcpy(void* to, const void* from, size_t size) {
    return std::memcpy(to, from, size);
}
void* __cdecl Memset(void* p, int value, size_t size) {
    return std::memset(p, value, size);
}
int __cdecl MemcpyS(void* to, size_t size, const void* from, size_t count) {
    return memcpy_s(to, size, from, count);
}
int __cdecl MemmoveS(void* to, size_t size, const void* from, size_t count) {
    return memmove_s(to, size, from, count);
}

int __cdecl Sprintf(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsprintf(buffer, format, args);
    va_end(args);
    return n;
}
int __cdecl SprintfS(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsprintf_s(buffer, size, format, args);
    va_end(args);
    return n;
}
int __cdecl Snprintf(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int n = _vsnprintf(buffer, size, format, args);
    va_end(args);
    return n;
}
int __cdecl Vsnprintf(char* buffer, size_t size, const char* format, va_list args) {
    return _vsnprintf(buffer, size, format, args);
}
int __cdecl VsnprintfS(char* buffer, size_t size, size_t count, const char* format, va_list args) {
    return _vsnprintf_s(buffer, size, count, format, args);
}
int __cdecl Sscanf(const char* s, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsscanf(s, format, args);
    va_end(args);
    return n;
}
int __cdecl Swscanf(const wchar_t* s, const wchar_t* format, ...) {
    va_list args;
    va_start(args, format);
    int n = vswscanf(s, format, args);
    va_end(args);
    return n;
}
long __cdecl Atol(const char* s) {
    return std::atol(s);
}
double __cdecl Atof(const char* s) {
    return std::atof(s);
}

void __cdecl Qsort(void* base, size_t count, size_t width, int(__cdecl* compare)(const void*, const void*)) {
    std::qsort(base, count, width, compare);
}
void* __cdecl Bsearch(const void* key, const void* base, size_t count, size_t width,
                      int(__cdecl* compare)(const void*, const void*)) {
    return std::bsearch(key, base, count, width, compare);
}

int __cdecl Mkdir(const char* path) {
    return _mkdir(path);
}

double __cdecl Cos(double x) {
    return std::cos(x);
}
double __cdecl Sin(double x) {
    return std::sin(x);
}
double __cdecl Tan(double x) {
    return std::tan(x);
}
double __cdecl Asin(double x) {
    return std::asin(x);
}
double __cdecl Acos(double x) {
    return std::acos(x);
}
double __cdecl Atan(double x) {
    return std::atan(x);
}
double __cdecl Atan2(double y, double x) {
    return std::atan2(y, x);
}
double __cdecl Pow(double x, double y) {
    return std::pow(x, y);
}
double __cdecl Sqrt(double x) {
    return std::sqrt(x);
}
double __cdecl Floor(double x) {
    return std::floor(x);
}
double __cdecl Ceil(double x) {
    return std::ceil(x);
}
int __cdecl FloatToInt(double x) {
    // (x87's fistp gives the "integer indefinite" 0x8000000000000000 out of range, NaN too:
    // its low half is 0. The double to int64 conversion of ours gives the same)
    return int(int64_t(x));
}
int64_t __stdcall Alldiv(int64_t a, int64_t b) {
    return a / b;
}

} // namespace crt
