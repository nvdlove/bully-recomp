#include "log.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace re {

static std::mutex g_logMutex;

void Log(const char* format, ...) {
    char text[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);

    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    while (n > 0 && path[n - 1] != '\\') {
        n--;
    }
    strcpy_s(path + n, MAX_PATH - n, "bullyre_log.txt");

    std::lock_guard lock(g_logMutex);
    if (FILE* f = fopen(path, "a")) {
        fprintf(f, "%s\n", text);
        fclose(f);
    }
}

} // namespace re
