// NiGetViewerString and its overloads (NiViewerStrings.cpp in Gamebryo): a new NiMalloc'd string
// for the scene viewer, "--- name ---" or "prefix = value", sized from the prefix's length plus
// a fixed allowance per type (the game's: a value too long for it is cut by sprintf_s).

#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {
namespace {

// sprintf_s with truncation (GamebryoInlines.cpp's)
GAME_FN(int, __cdecl, FormatString, 0x00402E00, char* buffer, size_t size, const char* format, ...);

// NiGetViewerString(name): "--- name ---"
char* __cdecl GetViewerString(const char* name) {
    uint32_t size = uint32_t(std::strlen(name)) + 9;
    char* text = static_cast<char*>(NiMalloc(size));
    FormatString(text, size, "--- %s ---", name);
    return text;
}

char* __cdecl GetViewerStringBool(const char* prefix, int value) {
    uint32_t size = uint32_t(std::strlen(prefix)) + 9;
    char* text = static_cast<char*>(NiMalloc(size));
    if (uint8_t(value) != 0) {
        FormatString(text, size, "%s = true", prefix);
    } else {
        FormatString(text, size, "%s = false", prefix);
    }
    return text;
}

// (an unsigned char)
char* __cdecl GetViewerStringByte(const char* prefix, uint32_t value) {
    uint32_t size = uint32_t(std::strlen(prefix)) + 7;
    char* text = static_cast<char*>(NiMalloc(size));
    FormatString(text, size, "%s = %u", prefix, uint32_t(uint8_t(value)));
    return text;
}

// (an unsigned short)
char* __cdecl GetViewerStringShort(const char* prefix, uint32_t value) {
    uint32_t size = uint32_t(std::strlen(prefix)) + 9;
    char* text = static_cast<char*>(NiMalloc(size));
    FormatString(text, size, "%s = %hu", prefix, uint32_t(uint16_t(value)));
    return text;
}

char* __cdecl GetViewerStringInt(const char* prefix, int32_t value) {
    uint32_t size = uint32_t(std::strlen(prefix)) + 0xF;
    char* text = static_cast<char*>(NiMalloc(size));
    FormatString(text, size, "%s = %d", prefix, value);
    return text;
}

char* __cdecl GetViewerStringUint(const char* prefix, uint32_t value) {
    uint32_t size = uint32_t(std::strlen(prefix)) + 0xE;
    char* text = static_cast<char*>(NiMalloc(size));
    FormatString(text, size, "%s = %u", prefix, value);
    return text;
}

char* __cdecl GetViewerStringFloat(const char* prefix, float value) {
    uint32_t size = uint32_t(std::strlen(prefix)) + 0x14;
    char* text = static_cast<char*>(NiMalloc(size));
    FormatString(text, size, "%s = %g", prefix, double(value));
    return text;
}

char* __cdecl GetViewerStringPointer(const char* prefix, const void* value) {
    uint32_t size = uint32_t(std::strlen(prefix)) + 0x10;
    char* text = static_cast<char*>(NiMalloc(size));
    FormatString(text, size, "%s = %p", prefix, value);
    return text;
}

// (a string: "NULL" for none)
char* __cdecl GetViewerStringString(const char* prefix, const char* value) {
    if (value != nullptr) {
        uint32_t size = uint32_t(std::strlen(prefix)) + uint32_t(std::strlen(value)) + 4;
        char* text = static_cast<char*>(NiMalloc(size));
        FormatString(text, size, "%s = %s", prefix, value);
        return text;
    }
    uint32_t size = uint32_t(std::strlen(prefix)) + 8;
    char* text = static_cast<char*>(NiMalloc(size));
    FormatString(text, size, "%s = NULL", prefix);
    return text;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIVIEWERSTRING_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIVIEWERSTRING_REVERSED("NiGetViewerString", 0x00785560, &ni::GetViewerString);
NIVIEWERSTRING_REVERSED("NiGetViewerString(bool)", 0x007855A0, &ni::GetViewerStringBool);
NIVIEWERSTRING_REVERSED("NiGetViewerString(unsigned char)", 0x00785600, &ni::GetViewerStringByte);
NIVIEWERSTRING_REVERSED("NiGetViewerString(unsigned short)", 0x00785650, &ni::GetViewerStringShort);
NIVIEWERSTRING_REVERSED("NiGetViewerString(int)", 0x007856A0, &ni::GetViewerStringInt);
NIVIEWERSTRING_REVERSED("NiGetViewerString(unsigned int)", 0x007856F0, &ni::GetViewerStringUint);
NIVIEWERSTRING_REVERSED("NiGetViewerString(float)", 0x00785740, &ni::GetViewerStringFloat);
NIVIEWERSTRING_REVERSED("NiGetViewerString(void*)", 0x00785790, &ni::GetViewerStringPointer);
NIVIEWERSTRING_REVERSED("NiGetViewerString(const char*)", 0x007857E0, &ni::GetViewerStringString);
