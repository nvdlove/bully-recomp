// NiD3D10Renderer's error reporting (Gamebryo's NiD3D10Error): the last error's code (an 0x404-byte
// holder at 0x20E8F64: +0x0 the code, +0x4 a text buffer left unused) and its text into
// NiRenderer's last error string; the warnings are formatted and dropped (the debug output is
// compiled out of this build).

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "gamebryo/NiD3D10Renderer.h"

namespace ni::c42 {
namespace {

using d3d10::Text;

GAME_VAR(uint32_t*, ms_pkErrorHolder, 0x020E8F64);
GAME_ARRAY(char, ms_acErrorString, 0x020E2280);   // NiRenderer's last error (0x100)

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(const char*, __cdecl, D3D10_GetErrorText, 0x008AF890, uint32_t code);

constexpr uintptr_t kErrorTexts[0x12] = {
    0x009A0240, 0x009A021C, 0x009A01FC, 0x009A01E0, 0x009A01C0, 0x009A01A0,
    0x009A017C, 0x009A0158, 0x009A0138, 0x009A0114, 0x009A00F0, 0x009A00CC,
    0x009A00A0, 0x009A0078, 0x009A0058, 0x009A0034, 0x009A0014, 0x0099FFEC,
};

// the message's end: a newline added unless it has one (the length clamped below the buffer's
// end)
void EndLine(char* buffer, int length, int last) {
    if (length >= last) {
        length = last;
    }
    if (buffer[length - 1] == '\n') {
        buffer[length] = '\0';
    } else {
        buffer[length] = '\n';
        buffer[length + 1] = '\0';
    }
}

} // namespace

// 0x8AF890: NiD3D10Error's text for an error code ("No error.", "Error loading D3D10 library."...)
const char* __cdecl NiD3D10Error_GetErrorText(uint32_t code) {
    if (code < 0x12) {
        return Text(kErrorTexts[code]);
    }
    return Text(0x0099FFD8);   // "Missing error text."
}

// 0x8AFA90: NiD3D10Error's holder deleted (NiD3D10RendererSDM's shutdown; the 64-bit dispatch's
// body, not hooked in the 32-bit game)
void __cdecl NiD3D10Error_Destroy() {
    if (ms_pkErrorHolder != nullptr) {
        NiDelete(ms_pkErrorHolder, 0x404);
    }
    ms_pkErrorHolder = nullptr;
}
RE_UNHOOKED_FN("NiD3D10Error::Destroy", 0x008AFA90, &NiD3D10Error_Destroy);

// 0x8AF960: NiD3D10Error's holder made
void __cdecl NiD3D10Error_Create() {
    auto* holder = static_cast<uint32_t*>(NiNew(0x404));
    if (holder != nullptr) {
        holder[0] = 0;
        crt::Memset(holder + 1, 0, 0x400);
        ms_pkErrorHolder = holder;
        return;
    }
    ms_pkErrorHolder = nullptr;
}

// 0x8AF9B0: NiD3D10Renderer's warning ("NiD3D10Renderer WARNING: " and the message, formatted
// with vsprintf_s; nothing done with it)
void __cdecl NiD3D10Renderer_Warning(const char* format, ...) {
    if (format == nullptr) {
        format = Text(0x008FEFBB);   // ""
    }
    char buffer[0x400];
    crt::Memcpy(buffer, Text(0x009A024C), 0x1A);   // "NiD3D10Renderer WARNING: "
    crt::Memset(buffer + 0x1A, 0, 0x3E6);
    int length = int(crt::Strcspn(buffer, ""));
    va_list args;
    va_start(args, format);
    length += vsprintf_s(buffer + length, size_t(0x400 - length), format, args);   // (the game's
    va_end(args);                                                                    //  0x863EF8)
    EndLine(buffer, length, 0x3FE);
}

// 0x8AFAC0: NiD3D10Renderer's error (the code kept; "NiD3D10Renderer ERROR: <its text>" and the
// message into NiRenderer's last error string) when the holder exists
void __cdecl NiD3D10Renderer_Error(uint32_t code, const char* format, ...) {
    if (ms_pkErrorHolder == nullptr) {
        return;
    }
    *ms_pkErrorHolder = code;
    if (format == nullptr) {
        format = Text(0x008FEFBB);
    }
    char buffer[0x800];
    NiSprintf(buffer, 0x800, Text(0x009A0268), D3D10_GetErrorText(code));
    int length = int(crt::Strcspn(buffer, ""));
    va_list args;
    va_start(args, format);
    length += vsprintf_s(buffer + length, size_t(0x800 - length), format, args);
    va_end(args);
    EndLine(buffer, length, 0x7FE);
    crt::StrncpyS(ms_acErrorString, 0x100, buffer, 0xFF);
}

#define C42D_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42D_REVERSED("NiD3D10Error::GetErrorText", 0x008AF890, NiD3D10Error_GetErrorText);
C42D_REVERSED("NiD3D10Error::Create", 0x008AF960, NiD3D10Error_Create);
C42D_REVERSED("NiD3D10Renderer::Warning", 0x008AF9B0, NiD3D10Renderer_Warning);
C42D_REVERSED("NiD3D10Renderer::Error", 0x008AFAC0, NiD3D10Renderer_Error);

} // namespace ni::c42
