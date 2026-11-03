// NiDX9PersistentSrcTextureRendererData (vtable 0x99BF30, NiRTTI 0x20E6DD8 on
// NiPersistentSrcTextureRendererData's 0x20E3E14; 0x8C bytes, the base's layout: see
// NiPersistentSrcTextureRendererData.cpp): the DX9 renderer's persistent source texture data. It
// streams as its base (slot 14 gives the base's RTTI), loads its own way (LoadBinary), and is what
// NiDX9Renderer::Create registers as NiPersistentSrcTextureRendererData's creator.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kVtable = 0x0099BF30;
constexpr uint32_t kSize = 0x8C;

GAME_VAR(void*, NiRTTI_NiDX9PersistentSrcTextureRendererData, 0x020E6DD8);
GAME_VAR(uint32_t, NiDX9PersistentSrcTextureRendererData_ClassBit, 0x020E6DE0);
GAME_VAR(void*, NiRTTI_NiPersistentSrcTextureRendererData, 0x020E3E14);
GAME_VAR(uint8_t, ms_bFreePlatformData, 0x020E3E10);

GAME_FN(void*, __thiscall, PersistentData_Construct, 0x007DD480, void* self);
GAME_FN(void, __thiscall, PersistentData_Destruct, 0x007DD4E0, void* self);
GAME_FN(uint8_t, __thiscall, PersistentData_RegisterStreamables, 0x007DD420, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, PersistentData_IsEqual, 0x007DD820, void* self, void* other);
GAME_FN(void, __thiscall, PersistentData_GetViewerStrings, 0x007DDA10, void* self, void* strings);
GAME_FN(void, __thiscall, Object_LoadBinary, 0x007554E0, void* self, void* stream);
GAME_FN(void, __thiscall, PixelFormat_LoadBinary, 0x0076B520, void* self, void* stream);
GAME_FN(void, __thiscall, Stream_ReadLinkID, 0x0075A7B0, void* stream);
GAME_FN(char*, __cdecl, NiGetViewerString, 0x00785560, const char* text);
GAME_FN(void, __thiscall, StringArray_SetSize, 0x0084A110, void* self, uint32_t size);
GAME_FN(void, __thiscall, StringArray_SetAt, 0x00833C70, void* self, uint32_t index, const re::Ptr32<char>* value);

// NiStream's binary stream read (its read function at +0x8, the position at +0x4 moved on)
inline void StreamRead(uint8_t* stream, void* buffer, uint32_t bytes, uint32_t* componentSize, uint32_t size) {
    uint8_t* binary = At<uint8_t*>(stream, 0x188);
    using ReadFn = uint32_t(__cdecl*)(void*, void*, uint32_t, uint32_t*, uint32_t);
    *componentSize = size;
    At<uint32_t>(binary, 4) += At<ReadFn>(binary, 8)(binary, buffer, bytes, componentSize, 1);
}

} // namespace

// 0x886790: NiDX9PersistentSrcTextureRendererData::CreateObject
void* __cdecl PersistentData_CreateObject() {
    void* memory = NiNew(kSize);
    if (memory == nullptr) {
        return nullptr;
    }
    PersistentData_Construct(memory);
    At<uint32_t>(memory, 0) = kVtable;
    return memory;
}

// 0x8867C0, 0x8867F0: LinkObject and SaveBinary, lone jmps to the base's
RE_UNHOOKED("NiDX9PersistentSrcTextureRendererData::LinkObject", 0x008867C0);
RE_UNHOOKED("NiDX9PersistentSrcTextureRendererData::SaveBinary", 0x008867F0);

// 0x8867D0 (thiscall): NiDX9PersistentSrcTextureRendererData::RegisterStreamables (the base's)
uint8_t __fastcall PersistentData_RegisterStreamablesImpl(uint8_t* self, re::Edx, void* stream) {
    return PersistentData_RegisterStreamables(self, stream) != 0 ? 1 : 0;
}

// 0x886800 (thiscall): NiDX9PersistentSrcTextureRendererData::GetRTTI
void* __fastcall PersistentData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9PersistentSrcTextureRendererData;
}

// 0x886810 (thiscall): NiDX9PersistentSrcTextureRendererData's deleting destructor
uint8_t* __fastcall PersistentData_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtable;
    PersistentData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kSize);
    }
    return self;
}

// 0x886840 (thiscall, slot 14): the RTTI it streams as (NiPersistentSrcTextureRendererData's)
void* __fastcall PersistentData_GetStreamableRTTI(void*, re::Edx) {
    return &NiRTTI_NiPersistentSrcTextureRendererData;
}

// 0x886850 (thiscall, slot 4): NiDX9PersistentSrcTextureRendererData::LoadBinary (the object, the
// pixel format, the palette's link; the levels, the stride, each level's width, height and
// offset (to the game's stack: 16 at most), the total, the pad offset (read from 20.2.0.6 on,
// else the total), the faces, the platform; the pixels allocated (slot 18) and read; a copy kept
// for the platform while ms_bFreePlatformData)
void __fastcall PersistentData_LoadBinary(uint8_t* self, re::Edx, uint8_t* stream) {
    uint32_t componentSize;
    uint32_t offsets[16];
    uint32_t widths[16];
    uint32_t heights[16];
    Object_LoadBinary(self, stream);
    PixelFormat_LoadBinary(self + 0x14, stream);
    Stream_ReadLinkID(stream);
    StreamRead(stream, self + 0x6C, 4, &componentSize, 4);
    StreamRead(stream, self + 0x70, 4, &componentSize, 4);
    uint32_t& levels = At<uint32_t>(self, 0x6C);
    for (uint32_t i = 0; i < levels; i++) {
        StreamRead(stream, &widths[i], 4, &componentSize, 4);
        StreamRead(stream, &heights[i], 4, &componentSize, 4);
        StreamRead(stream, &offsets[i], 4, &componentSize, 4);
    }
    StreamRead(stream, &offsets[levels], 4, &componentSize, 4);
    if (At<uint32_t>(stream, 0x14) < 0x14020006) {
        crt::MemcpyS(self + 0x68, 4, &offsets[levels], 4);
    } else {
        At<uint32_t>(self, 0x68) = 0;
        StreamRead(stream, self + 0x68, 4, &componentSize, 4);
    }
    StreamRead(stream, self + 0x74, 4, &componentSize, 4);
    uint32_t platform;
    StreamRead(stream, &platform, 4, &componentSize, 4);
    At<uint32_t>(self, 0x78) = platform;
    Virtual<void>(self, 18, levels, At<uint32_t>(self, 0x74), offsets[levels]);   // AllocateData
    uint32_t bytes = levels * 4;
    crt::MemcpyS(At<void*>(self, 0x5C), bytes, widths, bytes);
    crt::MemcpyS(At<void*>(self, 0x60), bytes, heights, bytes);
    crt::MemcpyS(At<void*>(self, 0x64), levels * 4 + 4, offsets, levels * 4 + 4);
    uint32_t pixels = At<uint32_t*>(self, 0x64)[levels] * At<uint32_t>(self, 0x74);
    StreamRead(stream, At<void*>(self, 0x58), pixels, &componentSize, 1);
    if (ms_bFreePlatformData != 0) {
        uint32_t faceSize = At<uint32_t*>(self, 0x64)[levels];
        uint32_t total = faceSize * At<uint32_t>(self, 0x74);
        At<uint32_t>(self, 0x80) = faceSize;
        At<uint32_t>(self, 0x84) = At<uint32_t>(self, 0x68);
        void* copy = NiMalloc(total);
        At<void*>(self, 0x7C) = copy;
        crt::MemcpyS(copy, total, At<void*>(self, 0x58), total);
    }
}

// 0x886B10 (thiscall): NiDX9PersistentSrcTextureRendererData::IsEqual (one of its kind, then the
// base's comparison)
uint8_t __fastcall PersistentData_IsEqualImpl(uint8_t* self, re::Edx, void* other) {
    if (other == nullptr || !HasClassBit(other, NiDX9PersistentSrcTextureRendererData_ClassBit)) {
        return 0;
    }
    return PersistentData_IsEqual(self, other) != 0 ? 1 : 0;
}

// 0x886B60 (thiscall): NiDX9PersistentSrcTextureRendererData::GetViewerStrings (the base's, then
// the class name)
void __fastcall PersistentData_GetViewerStringsImpl(uint8_t* self, re::Edx, uint8_t* strings) {
    PersistentData_GetViewerStrings(self, strings);
    re::Ptr32<char> name = NiGetViewerString(*static_cast<const re::Ptr32<const char>*>(static_cast<void*>(
        &NiRTTI_NiDX9PersistentSrcTextureRendererData)));
    uint32_t size = At<uint16_t>(strings, 0xA);
    if (size >= At<uint16_t>(strings, 8)) {
        StringArray_SetSize(strings, At<uint16_t>(strings, 0xE) + size);
    }
    StringArray_SetAt(strings, size, &name);
}

#define C39H_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39H_REVERSED("NiDX9PersistentSrcTextureRendererData::CreateObject", 0x00886790, PersistentData_CreateObject);
C39H_REVERSED("NiDX9PersistentSrcTextureRendererData::RegisterStreamables", 0x008867D0,
              PersistentData_RegisterStreamablesImpl);
C39H_REVERSED("NiDX9PersistentSrcTextureRendererData::GetRTTI", 0x00886800, PersistentData_GetRTTI);
C39H_REVERSED("NiDX9PersistentSrcTextureRendererData::DeletingDestructor", 0x00886810,
              PersistentData_DeletingDestructor);
C39H_REVERSED("NiDX9PersistentSrcTextureRendererData::GetStreamableRTTI", 0x00886840,
              PersistentData_GetStreamableRTTI);
C39H_REVERSED("NiDX9PersistentSrcTextureRendererData::LoadBinary", 0x00886850, PersistentData_LoadBinary);
C39H_REVERSED("NiDX9PersistentSrcTextureRendererData::IsEqual", 0x00886B10, PersistentData_IsEqualImpl);
C39H_REVERSED("NiDX9PersistentSrcTextureRendererData::GetViewerStrings", 0x00886B60,
              PersistentData_GetViewerStringsImpl);

} // namespace ni::c39
