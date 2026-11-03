// NiD3DUtility's file name helpers, NiD3DRenderer's error report (formatted into a local buffer
// only when 0xBCB8C4 says so, and shown nowhere) and the binary stream's raw byte read / write
// (NiBinaryStream: +0x4 the position, +0x8 the read function, +0xC the write function).
//
// _splitpath_s isn't in game/Crt.h: the C runtime's own is called (it keeps no state).

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c39 {
namespace {

using ni::sys::At;

GAME_VAR(uint8_t, ms_bReportErrors, 0x00BCB8C4);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);

using StreamFn = uint32_t(__cdecl*)(void* stream, void* buffer, uint32_t bytes, uint32_t* componentSizes,
                                    uint32_t components);

inline size_t Length(const char* s) {
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

} // namespace

// 0x888450: NiD3DUtility::GetSplitPath (drive 3, directory 0x104, name 0x104, extension 0x100)
uint8_t __cdecl D3DUtility_GetSplitPath(const char* path, char* drive, char* directory, char* name, char* extension) {
    _splitpath_s(path, drive, 3, directory, 0x104, name, 0x104, extension, 0x100);
    return 1;
}

// 0x888490: NiD3DUtility::GetFileExtension
uint8_t __cdecl D3DUtility_GetFileExtension(const char* path, char* extension, size_t size) {
    char drive[4];
    char directory[0x104];
    char name[0x100];
    char ext[0x100];
    _splitpath_s(path, drive, 3, directory, 0x104, name, 0x100, ext, 0x100);
    crt::StrncpyS(extension, size, ext, Length(ext));
    return 1;
}

// 0x888530: NiD3DUtility::SetString (a NiMalloc'd copy kept in *to: emptied for an empty string,
// the buffer reused while it's bigger than the string; the buffer's size)
size_t __cdecl D3DUtility_SetString(re::Ptr32<char>* to, size_t size, const char* from) {
    if (from == nullptr || *from == '\0') {
        NiFree(*to);
        *to = nullptr;
        return 0;
    }
    size_t needed = Length(from) + 1;
    if (*to == nullptr || size <= needed) {
        if (*to != nullptr) {
            NiFree(*to);
            *to = nullptr;
        }
        *to = static_cast<char*>(NiMalloc(uint32_t(needed)));
        size = needed;
    }
    crt::StrcpyS(*to, size, from);
    return size;
}

// 0x8885C0: NiBinaryStream's raw write (bytes as one-byte components)
void __cdecl Stream_SaveBytes(uint8_t* stream, const void* buffer, uint32_t count) {
    uint32_t componentSize = 1;
    At<uint32_t>(stream, 4) += At<StreamFn>(stream, 0xC)(stream, const_cast<void*>(buffer), count, &componentSize, 1);
}

// 0x8885F0: NiBinaryStream's raw read
void __cdecl Stream_LoadBytes(uint8_t* stream, void* buffer, uint32_t count) {
    uint32_t componentSize = 1;
    At<uint32_t>(stream, 4) += At<StreamFn>(stream, 8)(stream, buffer, count, &componentSize, 1);
}

// 0x888620: NiD3DRenderer::Error (formatted when errors are reported; the text goes nowhere)
void __cdecl D3DRenderer_Error(const char* format, ...) {
    char buffer[0x400];
    if (ms_bReportErrors != 0) {
        va_list args;
        va_start(args, format);
        buffer[0] = '\0';
        crt::VsnprintfS(buffer, 0x400, size_t(-1), format, args);
        va_end(args);
    }
}

// 0x888680: NiD3DUtility::GetFileName (the name without its directory or extension; false when it
// doesn't fit)
uint8_t __cdecl D3DUtility_GetFileName(const char* path, char* name, uint32_t size) {
    char drive[4];
    char directory[0x104];
    char file[0x100];
    char ext[0x100];
    _splitpath_s(path, drive, 3, directory, 0x104, file, 0x100, ext, 0x100);
    if (Length(file) + 1 > size) {
        return 0;
    }
    NiSprintf(name, size, "%s", file);
    return 1;
}

#define C39K_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39K_REVERSED("NiD3DUtility::GetSplitPath", 0x00888450, D3DUtility_GetSplitPath);
C39K_REVERSED("NiD3DUtility::GetFileExtension", 0x00888490, D3DUtility_GetFileExtension);
C39K_REVERSED("NiD3DUtility::SetString", 0x00888530, D3DUtility_SetString);
C39K_REVERSED("NiBinaryStream::SaveBytes_8885C0", 0x008885C0, Stream_SaveBytes);
C39K_REVERSED("NiBinaryStream::LoadBytes_8885F0", 0x008885F0, Stream_LoadBytes);
C39K_REVERSED("NiD3DRenderer::Error", 0x00888620, D3DRenderer_Error);
C39K_REVERSED("NiD3DUtility::GetFileName", 0x00888680, D3DUtility_GetFileName);

} // namespace ni::c39
