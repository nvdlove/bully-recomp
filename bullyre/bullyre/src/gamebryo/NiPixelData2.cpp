// NiPixelFormat's full constructor (every field given: the format, bits, tiling, endianness,
// renderer hint, sRGB and the four components) and NiPixelData::GetViewerStrings.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiPixelData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiPixelData_RTTI, 0x020E2A44);

GAME_FN(NiPixelFormat*, __thiscall, PixelFormat_Init, 0x0076AF20, NiPixelFormat* self, int32_t format,
        uint32_t bitsPerPixel, uint32_t tiling, int littleEndian, uint32_t rendererHint, uint32_t srgb,
        uint32_t c0, uint32_t r0, uint32_t b0, uint32_t s0, uint32_t c1, uint32_t r1, uint32_t b1, uint32_t s1,
        uint32_t c2, uint32_t r2, uint32_t b2, uint32_t s2, uint32_t c3, uint32_t r3, uint32_t b3, uint32_t s3);

inline void SetComponent(NiPixelComponent& component, uint32_t channel, uint32_t representation, uint32_t bits,
                         uint32_t isSigned) {
    component.component = channel;
    component.representation = representation;
    component.bitsPerComponent = uint8_t(bits);
    component.isSigned = uint8_t(isSigned);
}

} // namespace

// 0x76AF20: the fields set from all 22 arguments (bytes from the low byte of theirs; the extra
// data none)
void __fastcall NiPixelFormat_Init(NiPixelFormat* self, re::Edx, int32_t format, uint32_t bitsPerPixel,
                                   uint32_t tiling, int littleEndian, uint32_t rendererHint, uint32_t srgb,
                                   uint32_t c0, uint32_t r0, uint32_t b0, uint32_t s0, uint32_t c1, uint32_t r1,
                                   uint32_t b1, uint32_t s1, uint32_t c2, uint32_t r2, uint32_t b2, uint32_t s2,
                                   uint32_t c3, uint32_t r3, uint32_t b3, uint32_t s3) {
    self->bitsPerPixel = uint8_t(bitsPerPixel);
    self->srgb = uint8_t(srgb);
    self->format = format;
    self->rendererHint = rendererHint;
    self->tiling = tiling;
    self->flags = re::Flag(littleEndian) ? 1 : 0;
    SetComponent(self->components[0], c0, r0, b0, s0);
    SetComponent(self->components[1], c1, r1, b1, s1);
    SetComponent(self->components[2], c2, r2, b2, s2);
    self->extraData = 0;
    SetComponent(self->components[3], c3, r3, b3, s3);
}

// 0x76B2F0: NiPixelFormat::NiPixelFormat(all the fields): the above, this returned
NiPixelFormat* __fastcall NiPixelFormat_ConstructFull(NiPixelFormat* self, re::Edx, int32_t format,
                                                       uint32_t bitsPerPixel, uint32_t tiling, int littleEndian,
                                                       uint32_t rendererHint, uint32_t srgb, uint32_t c0,
                                                       uint32_t r0, uint32_t b0, uint32_t s0, uint32_t c1,
                                                       uint32_t r1, uint32_t b1, uint32_t s1, uint32_t c2,
                                                       uint32_t r2, uint32_t b2, uint32_t s2, uint32_t c3,
                                                       uint32_t r3, uint32_t b3, uint32_t s3) {
    PixelFormat_Init(self, format, bitsPerPixel, tiling, littleEndian, rendererHint, srgb, c0, r0, b0, s0, c1, r1,
                     b1, s1, c2, r2, b2, s2, c3, r3, b3, s3);
    return self;
}

// 0x76ACB0: NiPixelData::GetViewerStrings (slot 9)
void __fastcall NiPixelData_GetViewerStrings(NiPixelData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiPixelData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("Width", self->widths[0]));
    AddViewerString(strings, GetViewerStringUint("Height", self->heights[0]));
    AddViewerString(strings, GetViewerStringUint("m_uiMipmapLevels", self->mipmapLevels));
    AddViewerString(strings, GetViewerStringUint("m_uiFaces", self->faces));
    AddViewerString(strings,
                    GetViewerStringUint("TotalSizeInBytes", self->offsets[self->mipmapLevels] * self->faces));
    AddViewerString(strings, GetViewerStringUint("FaceSizeInBytes", self->offsets[self->mipmapLevels]));
}

#define NIPIXEL2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIPIXEL2_REVERSED("NiPixelData::GetViewerStrings", 0x0076ACB0, NiPixelData_GetViewerStrings);
// (0x76B2F0, the original, relies on ecx surviving the call (it returns ecx): turn on together with
// NiPixelFormat::NiPixelFormatFull)
NIPIXEL2_REVERSED("NiPixelFormat::Init", 0x0076AF20, NiPixelFormat_Init);
NIPIXEL2_REVERSED("NiPixelFormat::NiPixelFormatFull", 0x0076B2F0, NiPixelFormat_ConstructFull);

} // namespace ni
