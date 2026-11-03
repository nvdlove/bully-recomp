// NiPath (the library's path helpers: backslashes, "..", absolute paths) and NiFilename (a
// path split into drive, directory, name and extension, put back together with a platform
// subdirectory).

#include <cstring>
#include <cstdlib>
#include <direct.h>

#include "gamebryo/NiFile.h"
#include "gamebryo/NiSystem.h"

namespace ni {
namespace {

using namespace ni::sys;

GAME_FN(uint8_t, __cdecl, NiPath_Standardize, 0x00752C20, char* path);
GAME_FN(void, __cdecl, NiPath_RemoveSlashDotSlash, 0x00752D30, char* path);
GAME_FN(void, __cdecl, NiPath_RemoveDotDots, 0x00752D90, char* path);
GAME_FN(uint32_t, __cdecl, NiPath_ConvertToAbsolute4, 0x007530C0, char* absolutePath, uint32_t bytes,
        const char* relativePath, const char* relativeToHere);
GAME_FN(uint32_t, __cdecl, NiPath_ConvertToAbsolute3, 0x007531F0, char* path, uint32_t bytes,
        const char* relativeToHere);
GAME_FN(const char*, __cdecl, NiPath_SkipDrive, 0x00753AE0, const char* path);
GAME_FN(uint8_t, __cdecl, NiPath_IsRelative, 0x00753B50, const char* path);
GAME_FN(uint8_t, __cdecl, NiPath_GetCurrentWorkingDirectory, 0x00753BA0, char* path, uint32_t bytes);
GAME_FN(int, __cdecl, NiStrnicmp, 0x00753830, const char* a, const char* b, size_t n);
GAME_FN(char*, __cdecl, NiStrcat, 0x00749150, char* to, uint32_t bytes, const char* from);

// toupper as the game's C runtime does it in the "C" locale (it never changes locale)
inline int ToUpper(int c) {
    return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
}

inline bool IsDriveLetter(const char* path) {
    return path[1] == ':' && uint8_t(ToUpper(path[0]) - 'A') <= 25;
}

} // namespace

// 0x752C20: NiPath::Standardize, '/' made '\', runs of '\' after the second character made
// one; whether it changed anything
uint8_t __cdecl NiPath_StandardizeImpl(char* path) {
    uint8_t changed = 0;
    if (path == nullptr) {
        return changed;
    }
    uint32_t length = uint32_t(std::strlen(path));
    uint32_t out = 0;
    bool previousSlash = false;
    for (uint32_t i = 0; i < length; i++) {
        char c = path[i];
        if (c == '/') {
            changed = 1;
            c = '\\';
        }
        bool slash = c == '\\';
        if (slash && previousSlash && i > 1) {
            changed = 1;
            continue;
        }
        path[out++] = c;
        previousSlash = slash;
    }
    path[out] = 0;
    return changed;
}

// 0x752C90: NiPath::IsUniqueAbsolute, absolute, without "..", and already standard
uint8_t __cdecl NiPath_IsUniqueAbsolute(const char* path) {
    if (NiPath_IsRelative(path)) {
        return 0;
    }
    const char* rest = NiPath_SkipDrive(path);
    if (crt::Strstr(rest, "..") != nullptr) {
        return 0;
    }
    char standard[0x104];
    crt::StrcpyS(standard, 0x104, rest);
    return NiPath_Standardize(standard) ? 0 : 1;
}

// 0x752D30: NiPath::RemoveSlashDotSlash, each "\.\" made "\" (the game strcpy_s's the tail
// over itself, forwards, sized to the tail: always fits)
void __cdecl NiPath_RemoveSlashDotSlashImpl(char* path) {
    char* found = crt::Strstr(path, "\\.\\");
    while (found != nullptr) {
        std::memmove(found, found + 2, std::strlen(found + 2) + 1);
        found = crt::Strstr(path, "\\.\\");
    }
}

// 0x752D90: NiPath::RemoveDotDots, each "\.." taken out with the directory before it (a
// relative path's leading dots and slashes skipped, an absolute one's drive)
void __cdecl NiPath_RemoveDotDotsImpl(char* path) {
    NiPath_Standardize(path);
    NiPath_RemoveSlashDotSlash(path);
    if (NiPath_IsRelative(path)) {
        if (path == nullptr) {
            return;
        }
        while (*path == '.' || *path == '\\') {
            path++;
        }
    } else {
        path = const_cast<char*>(NiPath_SkipDrive(path));
    }
    if (path == nullptr) {
        return;
    }
    char* end = path + std::strlen(path) + 1;
    char* found = crt::Strstr(path, "\\..");
    while (found != nullptr) {
        *found = 0;
        char* to = crt::Strrchr(path, '\\');
        char* from;
        if (to != nullptr) {
            from = found + 3;
        } else {
            // (the game tests found + 3 for null)
            from = reinterpret_cast<uintptr_t>(found) + 3 == 0 ? nullptr : found + 4;
            to = path;
        }
        ptrdiff_t offset = from - to;
        for (int count = int(end - from); count > 0; count--) {
            *to = to[offset];
            to++;
        }
        found = crt::Strstr(path, "\\..");
    }
}

// 0x7530C0: NiPath::ConvertToAbsolute(absolutePath, bytes, relativePath, relativeToHere):
// relativeToHere (the working directory when empty) + '\' + relativePath, the dots taken out;
// the length, 0 (and an empty string) when it doesn't fit
uint32_t __cdecl NiPath_ConvertToAbsoluteImpl(char* absolutePath, uint32_t bytes, const char* relativePath,
                                              const char* relativeToHere) {
    if (relativeToHere == nullptr) {
        relativeToHere = "";
    }
    uint32_t hereLength = uint32_t(std::strlen(relativeToHere));
    if (hereLength == 0) {
        char directory[0x104];
        if (!NiPath_GetCurrentWorkingDirectory(directory, 0x104)) {
            if (bytes > 0) {
                absolutePath[0] = 0;
            }
            return 0;
        }
        return NiPath_ConvertToAbsolute4(absolutePath, bytes, relativePath, directory);
    }
    uint32_t relativeLength = uint32_t(std::strlen(relativePath));
    bool addSlash = relativeToHere[hereLength - 1] != '\\';
    if (bytes < (addSlash ? 1u : 0u) + relativeLength + hereLength + 1) {
        if (bytes > 0) {
            absolutePath[0] = 0;
        }
        return 0;
    }
    crt::StrcpyS(absolutePath, bytes, relativeToHere);
    if (addSlash) {
        crt::StrcatS(absolutePath, bytes, "\\");
    }
    crt::StrcatS(absolutePath, bytes, relativePath);
    NiPath_RemoveDotDots(absolutePath);
    return uint32_t(std::strlen(absolutePath));
}

// 0x7531F0: NiPath::ConvertToAbsolute(path, bytes, relativeToHere), in place
uint32_t __cdecl NiPath_ConvertToAbsoluteInPlace(char* path, uint32_t bytes, const char* relativeToHere) {
    char absolute[0x20C];
    uint32_t length = NiPath_ConvertToAbsolute4(absolute, 0x20A, path, relativeToHere);
    if (length < bytes) {
        crt::StrcpyS(path, bytes, absolute);
        return length;
    }
    path[0] = 0;
    return 0;
}

// 0x753280: NiPath::ConvertToAbsolute(path, bytes), against the working directory
uint32_t __cdecl NiPath_ConvertToAbsoluteFromCwd(char* path, uint32_t bytes) {
    char directory[0x104];
    if (!NiPath_GetCurrentWorkingDirectory(directory, 0x104)) {
        return 0;
    }
    return NiPath_ConvertToAbsolute3(path, bytes, directory);
}

// 0x7538C0: NiFilename(path), split by _splitpath_s (ours: it keeps no state)
NiFilename* __fastcall NiFilename_Construct(NiFilename* self, re::Edx, const char* path) {
    self->platformSubDir[0] = 0;
    _splitpath_s(path, self->drive, 3, self->dir, 0x100, self->fname, 0x100, self->ext, 0x100);
    return self;
}

// 0x753910: NiFilename::GetFullPath (drive, directory, platform subdirectory, name, extension);
// false when it doesn't fit
uint8_t __fastcall NiFilename_GetFullPath(NiFilename* self, re::Edx, char* out, uint32_t bytes) {
    if (out == nullptr) {
        return 0;
    }
    uint32_t needed = self->drive[0] != 0 ? 3 : 1;
    if (self->dir[0] != 0) {
        uint32_t n = uint32_t(std::strlen(self->dir));
        needed += n;
        char last = self->dir[n - 1];
        if (last != '\\' && last != '/') {
            needed++;
        }
    }
    if (self->platformSubDir[0] != 0) {
        uint32_t n = uint32_t(std::strlen(self->platformSubDir));
        needed += n;
        char last = self->platformSubDir[n - 1];
        if (last != '\\' && last != '/') {
            needed++;
        }
    }
    if (self->fname[0] != 0) {
        needed += uint32_t(std::strlen(self->fname));
    }
    if (self->ext[0] != 0) {
        if (self->ext[0] != '.') {
            needed++;
        }
        needed += uint32_t(std::strlen(self->ext));
    }
    if (bytes < needed) {
        return 0;
    }
    out[0] = 0;
    if (self->drive[0] != 0) {
        out[0] = self->drive[0];
        out[1] = ':';
        out[2] = 0;
    }
    if (self->dir[0] != 0) {
        crt::StrcatS(out, bytes, self->dir);
        char last = out[std::strlen(out) - 1];
        if (last != '\\' && last != '/') {
            NiStrcat(out, bytes, "\\");
        }
    }
    if (self->platformSubDir[0] != 0) {
        crt::StrcatS(out, bytes, self->platformSubDir);
        char last = out[std::strlen(out) - 1];
        if (last != '\\' && last != '/') {
            NiStrcat(out, bytes, "\\");
        }
    }
    if (self->fname[0] != 0) {
        crt::StrcatS(out, bytes, self->fname);
    }
    if (self->ext[0] != 0) {
        if (self->ext[0] != '.') {
            crt::StrcatS(out, bytes, ".");
        }
        crt::StrcatS(out, bytes, self->ext);
    }
    return 1;
}

// 0x753AD0: a lone jump to GetFullPath (another of NiFilename's names for it)
RE_UNHOOKED_FN("NiFilename::GetFullPath (jump)", 0x00753AD0, &NiFilename_GetFullPath);

// 0x753AE0: past a path's drive: "\\server\" (a UNC name: past the next '\', or to address 1
// when there's none), a leading '\', or "X:" and the '\' after it
const char* __cdecl NiPath_SkipDriveImpl(const char* path) {
    if (NiStrnicmp(path, "\\\\", 2) == 0) {
        return crt::Strchr(path + 2, '\\') + 1;
    }
    if (NiStrnicmp(path, "\\", 1) != 0) {
        if (!IsDriveLetter(path)) {
            return path;
        }
        path += 2;
        if (*path != '\\') {
            return path;
        }
    }
    return path + 1;
}

// 0x753B50: NiPath::IsRelative (not starting with a slash or a drive letter)
uint8_t __cdecl NiPath_IsRelativeImpl(const char* path) {
    if (std::strlen(path) < 2) {
        return 1;
    }
    if (path[0] == '\\' || path[0] == '/') {
        return 0;
    }
    return IsDriveLetter(path) ? 0 : 1;
}

// 0x753BA0: NiPath::GetCurrentWorkingDirectory (_getcwd: ours, it asks the system)
uint8_t __cdecl NiPath_GetCurrentWorkingDirectoryImpl(char* path, uint32_t bytes) {
    return _getcwd(path, int(bytes)) != nullptr ? 1 : 0;
}

#define NIPATH_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIPATH_REVERSED("NiPath::Standardize", 0x00752C20, NiPath_StandardizeImpl);
NIPATH_REVERSED("NiPath::IsUniqueAbsolute", 0x00752C90, NiPath_IsUniqueAbsolute);
NIPATH_REVERSED("NiPath::RemoveSlashDotSlash", 0x00752D30, NiPath_RemoveSlashDotSlashImpl);
NIPATH_REVERSED("NiPath::RemoveDotDots", 0x00752D90, NiPath_RemoveDotDotsImpl);
NIPATH_REVERSED("NiPath::ConvertToAbsolute", 0x007530C0, NiPath_ConvertToAbsoluteImpl);
NIPATH_REVERSED("NiPath::ConvertToAbsolute(relativeToHere)", 0x007531F0, NiPath_ConvertToAbsoluteInPlace);
NIPATH_REVERSED("NiPath::ConvertToAbsolute(cwd)", 0x00753280, NiPath_ConvertToAbsoluteFromCwd);
NIPATH_REVERSED("NiFilename::NiFilename", 0x007538C0, NiFilename_Construct);
NIPATH_REVERSED("NiFilename::GetFullPath", 0x00753910, NiFilename_GetFullPath);
NIPATH_REVERSED("NiPath::SkipDrive", 0x00753AE0, NiPath_SkipDriveImpl);
NIPATH_REVERSED("NiPath::IsRelative", 0x00753B50, NiPath_IsRelativeImpl);
NIPATH_REVERSED("NiPath::GetCurrentWorkingDirectory", 0x00753BA0, NiPath_GetCurrentWorkingDirectoryImpl);

} // namespace ni
