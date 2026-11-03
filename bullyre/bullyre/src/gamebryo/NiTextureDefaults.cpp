// The replacement pixel data the renderer shows for a texture it can't have (a 32x32 checker
// of two colours per kind, cached), NiPixelFormat's mask and shift of a component, and two of
// NiTexture's slots that are lone jumps to NiObjectNET's.

#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

// NiPixelFormat (0x44 bytes): flags (bit 0: the masks are little-endian), bits per pixel, ...,
// then four component descriptions: the component, its representation, its bits, signed
struct NiComponentSpec {
    int32_t component;        // +0x0
    int32_t representation;   // +0x4
    uint8_t bits;             // +0x8
    uint8_t isSigned;         // +0x9
    uint8_t pad[2];
};
struct NiPixelFormat {
    uint8_t flags;               // +0x0
    uint8_t bitsPerPixel;        // +0x1
    uint8_t unknown2[0x12];
    NiComponentSpec components[4];   // +0x14
};
static_assert(sizeof(NiPixelFormat) == 0x44);

GAME_FN(uint32_t, __thiscall, NiPixelFormat_GetMask, 0x007548E0, const NiPixelFormat* self, int component);
GAME_FN(uint8_t, __thiscall, NiPixelFormat_GetShift, 0x00754980, const NiPixelFormat* self, int component);
// NiPixelData(width, height, format, mipmap levels, faces) (0x70 bytes)
GAME_FN(void*, __thiscall, NiPixelData_Construct, 0x0076A610, void* self, uint32_t width, uint32_t height,
        const NiPixelFormat* format, uint32_t mipmapLevels, uint32_t faces);
// NiPointer<NiPixelData>::operator=
GAME_FN(void, __thiscall, PixelDataPointer_Assign, 0x00800420, re::Ptr32<void>* self, void* value);
GAME_FN(void*, __cdecl, NiTexture_CreateCheckerboard, 0x00754AB0, const float* color0, const float* color1,
        const NiPixelFormat* format, uint32_t faces);
GAME_FN(void*, __cdecl, NiTexture_DefaultReplacementData, 0x00754DF0, void* texture, uint32_t kind);

// the cache (one per kind) and the format they're made in
GAME_ARRAY(NiRefObject*, replacementData, 0x020E21D8);   // [10]
inline NiPixelFormat* const replacementFormat = reinterpret_cast<NiPixelFormat*>(0x020E2200);
using ReplacementFn = void*(__cdecl*)(void* texture, uint32_t kind);
GAME_VAR(ReplacementFn, replacementCallback, 0x00BC7554);

} // namespace

// 0x7548C0 and 0x7548D0: NiTexture's slots 4 and 5 (LoadBinary, LinkObject), jumps to
// NiObjectNET's
RE_UNHOOKED("NiTexture::LoadBinary", 0x007548C0);
RE_UNHOOKED("NiTexture::LinkObject", 0x007548D0);

// 0x7548E0: NiPixelFormat::GetMask, a component's bits in place (byte-swapped and moved down
// to the pixel's size when the format isn't little-endian); 0 for a component it hasn't
uint32_t __fastcall NiPixelFormat_GetMaskImpl(const NiPixelFormat* self, re::Edx, int component) {
    uint32_t shift = 0;
    uint32_t i = 0;
    for (; self->components[i].component != component; i++) {
        shift += self->components[i].bits;
        if (i + 1 > 3) {
            return 0;
        }
    }
    uint8_t bits = self->components[i].bits;
    if (bits == 0) {
        return 0;
    }
    uint32_t ones = 0;
    for (uint8_t k = uint8_t(bits - 1); k != 0; k--) {
        ones += 1u << (k & 31);
    }
    uint32_t mask = (ones + 1) << (shift & 31);
    if (self->flags & 1) {
        return mask;
    }
    uint32_t swapped = (mask >> 24) | (mask >> 8 & 0xFF00) | (mask << 8 & 0xFF0000) | (mask << 24);
    return swapped >> ((32 - self->bitsPerPixel) & 31);
}

// 0x754980: NiPixelFormat::GetShift (the bits of the components before it; from the top when
// not little-endian); 0 for a component it hasn't
uint8_t __fastcall NiPixelFormat_GetShiftImpl(const NiPixelFormat* self, re::Edx, int component) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < 4; i++) {
        if (self->components[i].component == component) {
            if (self->flags & 1) {
                return sum;
            }
            return uint8_t(self->bitsPerPixel - uint8_t(self->components[i].bits + sum));
        }
        sum = uint8_t(sum + self->components[i].bits);
    }
    return 0;
}

// 0x754A50: the replacements' format set (the cached ones released)
void __cdecl NiTexture_SetReplacementFormat(const NiPixelFormat* format) {
    crt::Memcpy(replacementFormat, format, sizeof(NiPixelFormat));
    for (int i = 0; i < 10; i++) {
        NiRefObject* data = replacementData[i];
        if (data != nullptr) {
            if (Decrement(&data->refCount) == 0) {
                Virtual<void>(data, 1);
            }
            replacementData[i] = nullptr;
        }
    }
}

// 0x754AB0: 32x32 pixels per face in 16x16 squares of the two colours (RGBA floats, packed
// into the format: each channel * 255 truncated, shifted and masked)
void* __cdecl NiTexture_CreateCheckerboardImpl(const float* color0, const float* color1,
                                               const NiPixelFormat* format, uint32_t faces) {
    void* memory = NiNew(0x70);
    uint8_t* data = memory != nullptr
                        ? static_cast<uint8_t*>(NiPixelData_Construct(memory, 0x20, 0x20, format, 1, faces))
                        : nullptr;
    uint32_t packed[2] = {0, 0};
    const float* colors[2] = {color0, color1};
    for (int c = 0; c < 2; c++) {
        for (int component = 0; component < 4; component++) {
            uint8_t shift = NiPixelFormat_GetShift(format, component);
            uint32_t value = uint32_t(crt::FloatToInt(float(colors[c][component] * 255.0f))) << (shift & 31);
            packed[c] |= NiPixelFormat_GetMask(format, component) & value;
        }
    }
    for (uint32_t face = 0; face < faces; face++) {
        uint32_t* offsets = At<uint32_t*>(data, 0x5C);
        uint32_t levels = At<uint32_t>(data, 0x60);
        uint8_t* to = offsets[levels] * face + offsets[0] + At<uint8_t*>(data, 0x50);
        uint32_t bytes = format->bitsPerPixel >> 3;
        for (uint32_t y = 0; y < 0x20; y++) {
            for (uint32_t x = 0; x < 0x20; x++) {
                crt::MemcpyS(to, bytes, &packed[(y >> 4) != (x >> 4) ? 1 : 0], bytes);
                to += bytes;
            }
        }
    }
    return data;
}

// 0x754DF0: the default replacement of a kind (0x80000001..0x80000009; anything else the
// magenta one), made on first use. (`kind` indexes the cache as the game's does: the top bit
// falls off the address arithmetic.)
void* __cdecl NiTexture_DefaultReplacementDataImpl(void* texture, uint32_t kind) {
    (void)texture;
    uint32_t index = kind & 0x7FFFFFFF;
    if (index >= 10) {
        return nullptr;
    }
    re::Ptr32<void>* slot = reinterpret_cast<re::Ptr32<void>*>(&replacementData[index]);
    if (*slot == nullptr) {
        static const float kColors[9][2][3] = {
            {{0, 0, 1}, {1, 1, 0}},   // 0x80000001
            {{0, 1, 0}, {1, 0, 0}},   // 0x80000002 (6 faces)
            {{0, 1, 1}, {1, 1, 0}},   // 0x80000003
            {{1, 0, 0}, {1, 1, 0}},   // 0x80000004
            {{1, 0, 1}, {1, 0, 1}},   // 0x80000005 (and anything else)
            {{0, 1, 1}, {0, 1, 1}},   // 0x80000006
            {{0, 1, 0}, {1, 1, 0}},   // 0x80000007 (6 faces)
            {{0, 1, 0}, {1, 0, 1}},   // 0x80000008 (6 faces)
            {{0, 1, 0}, {0, 1, 1}},   // 0x80000009 (6 faces)
        };
        uint32_t row = kind - 0x80000001u;
        if (row > 8) {
            row = 4;
        }
        uint32_t faces = row == 1 || row >= 6 ? 6 : 1;
        float color0[4] = {kColors[row][0][0], kColors[row][0][1], kColors[row][0][2], 1.0f};
        float color1[4] = {kColors[row][1][0], kColors[row][1][1], kColors[row][1][2], 1.0f};
        PixelDataPointer_Assign(slot, NiTexture_CreateCheckerboard(color0, color1, replacementFormat, faces));
    }
    return *slot;
}

// 0x755170: the replacement for a kind with the top bit set (through the hook, the default
// one when none is set); none for others
void* __cdecl NiTexture_GetReplacementData(void* texture, int32_t kind) {
    if (kind >= 0) {
        return nullptr;
    }
    if (replacementCallback == nullptr) {
        replacementCallback = reinterpret_cast<ReplacementFn>(0x00754DF0);
    }
    return replacementCallback(texture, uint32_t(kind));
}

// 0x754A00: NiTexture::_SDMShutdown (NiMainSDM::Shutdown's): the cached replacements let go, each
// slot emptied
void __cdecl NiTexture_SDMShutdown() {
    for (int i = 0; i < 10; i++) {
        NiRefObject* data = replacementData[i];
        if (data != nullptr) {
            if (Decrement(&data->refCount) == 0) {
                Virtual<void>(data, 1);
            }
            replacementData[i] = nullptr;
        }
    }
}

#define NITEXDEF_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NITEXDEF_REVERSED("NiPixelFormat::GetMask", 0x007548E0, NiPixelFormat_GetMaskImpl);
NITEXDEF_REVERSED("NiPixelFormat::GetShift", 0x00754980, NiPixelFormat_GetShiftImpl);
NITEXDEF_REVERSED("NiTexture::SetReplacementFormat", 0x00754A50, NiTexture_SetReplacementFormat);
NITEXDEF_REVERSED("NiTexture::CreateCheckerboard", 0x00754AB0, NiTexture_CreateCheckerboardImpl);
NITEXDEF_REVERSED("NiTexture::DefaultReplacementData", 0x00754DF0, NiTexture_DefaultReplacementDataImpl);
NITEXDEF_REVERSED("NiTexture::GetReplacementData", 0x00755170, NiTexture_GetReplacementData);
// (never hooked in the 32-bit game: a body at its address for the 64-bit build)
RE_UNHOOKED_FN("NiTexture::_SDMShutdown", 0x00754A00, &NiTexture_SDMShutdown);

} // namespace ni
