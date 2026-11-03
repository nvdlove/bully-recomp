// NiShaderAttributeDesc's last two (the rest is NiShaderDesc.cpp's): SetRange_Array and the
// copy (operator=, slot 6), which sets every part through the setters by the source's type.
// The layout (0xE4 bytes) as NiShaderDesc.cpp has it: the type +0x10, the flags +0x14, the
// value +0x18 (an array's: data, element type +0x1C, element size +0x20), the count +0x58, the
// range +0x5C / +0x9C (an array's: two NiMalloc'd copies), the texture map's name +0xDC.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiShaderAttributeDesc {
    re::Ptr32<void> vtable;                // +0x0
    uint8_t base[0xC];           // +0x4 (NiShaderDescBase's: the name +0x8, the description +0xC)
    uint32_t type;               // +0x10 (1 bool, 2 string, 3 unsigned int, 4 float, 5 point2,
                                 //  6 point3, 7 point4, 8 matrix3, 9 matrix4, 10 color,
                                 //  11 texture, 14 array)
    uint32_t flags;              // +0x14
    uint8_t value[0x40];         // +0x18
    uint32_t count;              // +0x58
    uint8_t low[0x40];           // +0x5C
    uint8_t high[0x40];          // +0x9C
    re::Ptr32<char> textureMap;            // +0xDC
    uint32_t textureMapSize;     // +0xE0
};
static_assert(offsetof(NiShaderAttributeDesc, count) == 0x58);
static_assert(offsetof(NiShaderAttributeDesc, textureMap) == 0xDC);
static_assert(sizeof(NiShaderAttributeDesc) == 0xE4);

using Desc = NiShaderAttributeDesc;

// copies with memcpy_s (chunk 00's wrapper: destination, source, size)
GAME_FN(void, __cdecl, CopyBytes, 0x0074D2A0, void* to, const void* from, uint32_t size);
// the setters (NiShaderDesc.cpp's)
GAME_FN(void, __thiscall, SetValue_Bool, 0x00784690, Desc* self, int value);
GAME_FN(void, __thiscall, SetValue_UnsignedInt, 0x00784700, Desc* self, uint32_t value);
GAME_FN(void, __thiscall, SetValue_Float_x64, 0x00784770, Desc* self, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void SetValue_Float(Desc* a0, float a1) {
    return SetValue_Float_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, SetValue_Point2, 0x007847D0, Desc* self, const float* value);
GAME_FN(void, __thiscall, SetValue_Point3, 0x00784840, Desc* self, const float* value);
GAME_FN(void, __thiscall, SetValue_ColorA, 0x00784920, Desc* self, const float* value);
GAME_FN(void, __thiscall, SetRange_UnsignedInt, 0x007849E0, Desc* self, uint32_t low, uint32_t high);
GAME_FN(void, __thiscall, SetRange_Float_x64, 0x00784A10, Desc* self, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void SetRange_Float(Desc* a0, float a1, float a2) {
    return SetRange_Float_x64(a0, re::BitsOf(a1), re::BitsOf(a2));
}
GAME_FN(void, __thiscall, SetRange_Point2, 0x00784A40, Desc* self, const float* low, const float* high);
GAME_FN(void, __thiscall, SetRange_Point3, 0x00784A80, Desc* self, const float* low, const float* high);
GAME_FN(void, __thiscall, SetRange_Point4, 0x00784AD0, Desc* self, const void* low, const void* high);
GAME_FN(void, __thiscall, SetRange_ColorA, 0x00784B80, Desc* self, const float* low, const float* high);
GAME_FN(void, __thiscall, SetValue_String, 0x00784D00, Desc* self, const char* text);
GAME_FN(void, __thiscall, SetValue_Point4, 0x00784DE0, Desc* self, const void* value);
GAME_FN(void, __thiscall, SetValue_Matrix3, 0x00784E50, Desc* self, const float* value);
GAME_FN(void, __thiscall, SetValue_Matrix4, 0x00784EE0, Desc* self, const void* value);
GAME_FN(void, __thiscall, SetValue_Texture, 0x00784F50, Desc* self, uint32_t slot, const char* map);
GAME_FN(void, __thiscall, SetValue_Array, 0x00785070, Desc* self, const void* data, uint32_t elementType,
        uint32_t elementSize, uint32_t count);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, SetRange_Array, 0x00785110, Desc* self, uint32_t elementSize, uint32_t count,
        const void* low, const void* high);

template <typename T>
inline re::Field<T>& Field(Desc* self, int offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(self) + offset);
}

// SetRange_Array(elementSize, count, low, high): for an array of that shape only; both ends
// copied into new NiMalloc'd buffers (4 bytes an element, whatever its size: the game's
// arithmetic). The old buffers are freed only when the new range is bigger, which it can't be
// (the shape must match): they leak, as the game has it
uint8_t __fastcall SetRangeArray(Desc* self, re::Edx, uint32_t elementSize, uint32_t count, const void* low,
                                 const void* high) {
    if (self->type != 0xE) {
        return 0;
    }
    if (elementSize != Field<uint32_t>(self, 0x20) || count != self->count) {
        return 0;
    }
    uint32_t size = elementSize * count;
    if ((Field<void*>(self, 0x5C) != nullptr || Field<void*>(self, 0x9C) != nullptr) &&
        size > self->count * Field<uint32_t>(self, 0x20)) {
        NiFree(Field<void*>(self, 0x5C));
        NiFree(Field<void*>(self, 0x9C));
        Field<void*>(self, 0x5C) = nullptr;
        Field<void*>(self, 0x9C) = nullptr;
    }
    if (low == nullptr || high == nullptr) {
        Field<void*>(self, 0x5C) = nullptr;
        Field<void*>(self, 0x9C) = nullptr;
        return 1;
    }
    size *= 4;
    Field<void*>(self, 0x5C) = NiMalloc(size);
    Field<void*>(self, 0x9C) = NiMalloc(size);
    CopyBytes(Field<void*>(self, 0x5C), low, size);
    CopyBytes(Field<void*>(self, 0x9C), high, size);
    return 1;
}

// operator= (slot 6): the name and description (slots 4 and 5 from the other's 2 and 3), the
// type and flags, the texture map's name dropped, then the value and range through the setters
// for the type (the type already set, so they free nothing: a string's or array's old buffers
// leak, as the game has it). A string's range goes through SetRange_UnsignedInt, as the game
// has it (it fails for the type)
Desc* __fastcall AssignDesc(Desc* self, re::Edx, Desc* other) {
    Virtual<void>(self, 4, Virtual<const void*>(other, 2));
    Virtual<void>(self, 5, Virtual<const void*>(other, 3));
    self->type = other->type;
    self->flags = other->flags;
    NiFree(self->textureMap);
    self->textureMap = nullptr;
    self->textureMapSize = 0;
    switch (self->type) {
    case 1:
        SetValue_Bool(self, Field<uint8_t>(other, 0x18));
        break;
    case 2:
        SetValue_String(self, Field<const char*>(other, 0x18));
        SetRange_UnsignedInt(self, Field<uint32_t>(other, 0x5C), Field<uint32_t>(other, 0x9C));
        break;
    case 3:
        SetValue_UnsignedInt(self, Field<uint32_t>(other, 0x18));
        SetRange_UnsignedInt(self, Field<uint32_t>(other, 0x5C), Field<uint32_t>(other, 0x9C));
        break;
    case 4:
        SetValue_Float(self, Field<float>(other, 0x18));
        SetRange_Float(self, Field<float>(other, 0x5C), Field<float>(other, 0x9C));
        break;
    case 5: {
        float value[2] = {Field<float>(other, 0x18), Field<float>(other, 0x1C)};
        SetValue_Point2(self, value);
        float low[2] = {Field<float>(other, 0x5C), Field<float>(other, 0x60)};
        float high[2] = {Field<float>(other, 0x9C), Field<float>(other, 0xA0)};
        SetRange_Point2(self, low, high);
        break;
    }
    case 6: {
        float value[3] = {Field<float>(other, 0x18), Field<float>(other, 0x1C), Field<float>(other, 0x20)};
        SetValue_Point3(self, value);
        float low[3] = {Field<float>(other, 0x5C), Field<float>(other, 0x60), Field<float>(other, 0x64)};
        float high[3] = {Field<float>(other, 0x9C), Field<float>(other, 0xA0), Field<float>(other, 0xA4)};
        SetRange_Point3(self, low, high);
        break;
    }
    case 7:
        SetValue_Point4(self, other->value);
        SetRange_Point4(self, other->low, other->high);
        break;
    case 8: {
        // the rows were stored padded to four
        float value[9];
        for (int row = 0; row < 3; row++) {
            for (int column = 0; column < 3; column++) {
                value[row * 3 + column] = Field<float>(other, 0x18 + row * 0x10 + column * 4);
            }
        }
        SetValue_Matrix3(self, value);
        break;
    }
    case 9:
        SetValue_Matrix4(self, other->value);
        break;
    case 10: {
        float value[4], low[4], high[4];
        for (int i = 0; i < 4; i++) {
            value[i] = Field<float>(other, 0x18 + i * 4);
            low[i] = Field<float>(other, 0x5C + i * 4);
            high[i] = Field<float>(other, 0x9C + i * 4);
        }
        SetValue_ColorA(self, value);
        SetRange_ColorA(self, low, high);
        break;
    }
    case 11:
        SetValue_Texture(self, Field<uint32_t>(other, 0x18), other->textureMap);
        break;
    case 14:
        SetValue_Array(self, Field<const void*>(other, 0x18), Field<uint32_t>(other, 0x1C),
                       Field<uint32_t>(other, 0x20), other->count);
        SetRange_Array(self, Field<uint32_t>(other, 0x20), other->count, Field<const void*>(other, 0x5C),
                       Field<const void*>(other, 0x9C));
        break;
    default:
        break;
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISHADERATTR2_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISHADERATTR2_REVERSED("NiShaderAttributeDesc::SetRange_Array", 0x00785110, &ni::SetRangeArray);
NISHADERATTR2_REVERSED("NiShaderAttributeDesc::operator=", 0x007851F0, &ni::AssignDesc);
