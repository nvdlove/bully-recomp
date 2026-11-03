// NiScreenTexture (vtable 0x9611B0, 0x20 bytes): screen-space rectangles of one texture, drawn
// by the renderer (its slot 99). The rectangles (0x1C bytes: top, left, width, height, the
// texture's top / left pixel, an NiColorA) in an NiTPrimitiveSet (+0x8: data, allocated,
// count; doubling); its NiTexturingProperty (+0x14, a reference) holds the base texture. Also
// NiTexturingProperty::SetBaseClampMode, which its constructor calls.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiTexturingProperty.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiScreenTexture_RTTI, 0x020E3D3C);

constexpr uintptr_t kVtable = 0x009611B0;

struct ScreenRect {
    int16_t top;           // +0x0
    int16_t left;          // +0x2
    uint16_t width;        // +0x4
    uint16_t height;       // +0x6
    uint16_t pixTop;       // +0x8
    uint16_t pixLeft;      // +0xA
    float color[4];        // +0xC (NiColorA)
};
static_assert(sizeof(ScreenRect) == 0x1C);

// NiTPrimitiveSet<ScreenRect>
struct RectSet {
    re::Ptr32<ScreenRect> base;      // +0x0
    uint32_t allocated;    // +0x4
    uint32_t count;        // +0x8
};

struct NiScreenTexture : NiObject {
    RectSet rects;                   // +0x8
    re::Ptr32<NiObject> texturingProperty;     // +0x14 (a reference)
    uint16_t flags;                  // +0x18
    uint16_t pad1A;
    uint32_t unknown1C;              // +0x1C (0)
};
static_assert(offsetof(NiScreenTexture, texturingProperty) == 0x14);
static_assert(sizeof(NiScreenTexture) == 0x20);

GAME_FN(void, __cdecl, Object_PurgeRendererData, 0x007556C0, void* object);   // the renderer's slot 51
GAME_FN(NiObject*, __thiscall, TexturingProperty_Construct, 0x00765DD0, void* self);
GAME_FN(void, __thiscall, TexturingProperty_SetBaseTexture, 0x00407A60, NiObject* self, NiObject* texture);
GAME_FN(void*, __thiscall, TexturingMap_Construct, 0x00765BA0, void* self);
GAME_FN(void, __thiscall, MapArray_SetAt, 0x00833C70, NiTArray<NiTexturingMap*>* self, uint32_t index,
        const re::Ptr32<NiTexturingMap>* map);
GAME_FN(void, __thiscall, ColorA_LoadBinary, 0x007625D0, float* self, void* stream);
GAME_FN(void, __thiscall, ColorA_SaveBinary, 0x00762670, const float* self, void* stream);
GAME_FN(uint8_t, __thiscall, ColorA_NotEquals, 0x00769390, const float* self, const float* other);
GAME_FN(void, __thiscall, Object_LinkObject, 0x008A20A0, void* self, void* stream);   // (does nothing)
GAME_FN(void, __thiscall, Object_SaveBinary, 0x008A20A0, void* self, void* stream);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, RectSet_Realloc, 0x007DB710, RectSet* self, uint32_t size);
GAME_FN(void, __thiscall, TexturingProperty_SetBaseClampMode, 0x007DB780, NiObject* self, int mode);
GAME_FN(void, __thiscall, ScreenTexture_CopyMembers, 0x007DBC70, NiScreenTexture* self, NiScreenTexture* dest,
        void* cloning);

inline void Unref(NiObject* object) {
    if (object != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(&object->refCount)) == 0) {
        Virtual<void>(object, 1);
    }
}

// NiTPrimitiveSet::Add (doubling when full)
inline uint32_t AddRect(NiScreenTexture* self, const ScreenRect& rect) {
    RectSet& set = self->rects;
    if (set.count == set.allocated) {
        RectSet_Realloc(&set, set.allocated != 0 ? set.allocated * 2 : 1);
    }
    set.base[set.count] = rect;
    set.count++;
    return self->rects.count - 1;
}

inline void LoadRaw(void* stream, void* buffer, uint32_t bytes) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t componentSize = bytes;
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}

inline void SaveRaw(void* stream, const void* buffer, uint32_t bytes) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t componentSize = bytes;
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

inline void SetTexturingProperty(NiScreenTexture* self, NiObject* property) {
    NiObject* old = self->texturingProperty;
    if (old != property) {
        Unref(old);
        self->texturingProperty = property;
        AddRef(property);
    }
}

inline void ConstructFields(NiScreenTexture* self) {
    self->rects.base = nullptr;
    self->rects.allocated = 0;
    self->rects.count = 0;
    self->texturingProperty = nullptr;
    self->flags = 0;
}

// 0x7DB6B0 RemoveAllScreenRects (the game's code shares it: a count at +0x10 zeroed)
void __fastcall RemoveAllScreenRectsImpl(NiScreenTexture* self, re::Edx) {
    self->rects.count = 0;
}

// 0x7DB6C0 Draw(renderer): the renderer's slot 99 (a tail call)
void __fastcall DrawImpl(NiScreenTexture* self, re::Edx, void* renderer) {
    Virtual<void>(renderer, 99, static_cast<void*>(self));
}

// 0x7DB6E0 vf6 RegisterStreamables
uint8_t __fastcall RegisterStreamablesImpl(NiScreenTexture* self, re::Edx, void* stream) {
    if (Object_RegisterStreamables(self, stream) == 0) {
        return 0;
    }
    Virtual<uint8_t>(self->texturingProperty, 6, stream);
    return 1;
}

// 0x7DB710 NiTPrimitiveSet<ScreenRect>::Realloc(size): the rectangles moved to a new block
void __fastcall RectSet_ReallocImpl(RectSet* self, re::Edx, uint32_t size) {
    if (size == self->allocated) {
        return;
    }
    ScreenRect* rects = nullptr;
    if (size != 0) {
        rects = static_cast<ScreenRect*>(NiMalloc(size * 0x1C));
        for (uint32_t i = 0; i < self->count; i++) {
            rects[i] = self->base[i];
        }
    }
    NiFree(self->base);
    self->base = rects;
    self->allocated = size;
}

// 0x7DB780 NiTexturingProperty::SetBaseClampMode(mode): the base map made when there's none,
// its clamp mode (flags bits 12..13) set
void __fastcall TexturingProperty_SetBaseClampModeImpl(NiTexturingProperty* self, re::Edx, int mode) {
    re::Ptr32<NiTexturingMap> map = self->maps.base[0];
    if (map == nullptr) {
        void* memory = NiNew(0x10);
        map = memory != nullptr ? static_cast<NiTexturingMap*>(TexturingMap_Construct(memory)) : nullptr;
        MapArray_SetAt(&self->maps, 0, &map);
    }
    map->flags = uint16_t((map->flags & 0xCFFF) | uint16_t(mode << 12));
}

// 0x7DB7E0 vf2
const RTTI* __fastcall GetRTTIImpl(NiScreenTexture*, re::Edx) {
    return &NiScreenTexture_RTTI;
}

// 0x7DB7F0 vf5 LinkObject(stream): the texturing property
void __fastcall LinkObjectImpl(NiScreenTexture* self, re::Edx, void* stream) {
    Object_LinkObject(self, stream);
    SetTexturingProperty(self, static_cast<NiObject*>(Stream_ResolveLinkID(stream)));
}

// 0x7DB850 vf7 SaveBinary(stream)
void __fastcall SaveBinaryImpl(NiScreenTexture* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    uint32_t count = self->rects.count;
    SaveRaw(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        const ScreenRect& rect = self->rects.base[i];
        SaveRaw(stream, &rect.top, 2);
        SaveRaw(stream, &rect.left, 2);
        SaveRaw(stream, &rect.width, 2);
        SaveRaw(stream, &rect.height, 2);
        SaveRaw(stream, &rect.pixTop, 2);
        SaveRaw(stream, &rect.pixLeft, 2);
        float color[4] = {rect.color[0], rect.color[1], rect.color[2], rect.color[3]};
        ColorA_SaveBinary(color, stream);
    }
    Virtual<void>(stream, 12, static_cast<void*>(self->texturingProperty));   // SaveLinkID
}

// 0x7DB9D0 vf8 IsEqual(other): the counts, the rectangles (the game's quirk: one only differs
// when every field does), the texturing properties
uint8_t __fastcall IsEqualImpl(NiScreenTexture* self, re::Edx, NiScreenTexture* other) {
    if (Object_IsEqual(self, other) == 0) {
        return 0;
    }
    uint32_t count = self->rects.count;
    if (count != other->rects.count) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        const ScreenRect& a = self->rects.base[i];
        const ScreenRect& b = other->rects.base[i];
        if (a.top != b.top && a.left != b.left && a.width != b.width && a.height != b.height &&
            a.pixTop != b.pixTop && a.pixLeft != b.pixLeft && ColorA_NotEquals(a.color, b.color) != 0) {
            return 0;
        }
    }
    return Virtual<uint8_t>(self->texturingProperty, 8, static_cast<void*>(other->texturingProperty)) != 0 ? 1 : 0;
}

// 0x7DBAB0 NiScreenTexture(texture): a texturing property with the texture as its base
// (replace mode, clamp mode 0)
NiScreenTexture* __fastcall ConstructImpl(NiScreenTexture* self, re::Edx, NiObject* texture) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(kVtable);
    ConstructFields(self);
    void* memory = NiNew(0x34);
    NiObject* property = memory != nullptr ? TexturingProperty_Construct(memory) : nullptr;
    SetTexturingProperty(self, property);
    At<uint16_t>(self->texturingProperty, 0x18) &= 0xFFF1;
    TexturingProperty_SetBaseTexture(self->texturingProperty, texture);
    TexturingProperty_SetBaseClampMode(self->texturingProperty, 0);
    self->unknown1C = 0;
    return self;
}

// 0x7DBB50 vf0
NiScreenTexture* __fastcall DeletingDestructorImpl(NiScreenTexture* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kVtable);
    Object_PurgeRendererData(self);
    Unref(self->texturingProperty);
    NiFree(self->rects.base);
    Object_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x20);
    }
    return self;
}

// 0x7DBBB0 AddScreenRect(top, left, width, height, pixTop, pixLeft, color): its index
uint32_t __fastcall AddScreenRectImpl(NiScreenTexture* self, re::Edx, int top, int left, int width, int height,
                                      int pixTop, int pixLeft, float r, float g, float b, float a) {
    ScreenRect rect;
    rect.top = int16_t(top);
    rect.left = int16_t(left);
    rect.width = uint16_t(width);
    rect.height = uint16_t(height);
    rect.pixTop = uint16_t(pixTop);
    rect.pixLeft = uint16_t(pixLeft);
    rect.color[0] = r;
    rect.color[1] = g;
    rect.color[2] = b;
    rect.color[3] = a;
    return AddRect(self, rect);
}

// 0x7DBC70 CopyMembers(dest, cloning)
void __fastcall CopyMembersImpl(NiScreenTexture* self, re::Edx, NiScreenTexture* dest, void* cloning) {
    Object_CopyMembers(self, dest, cloning);
    for (uint32_t i = 0; i < self->rects.count; i++) {
        ScreenRect rect = self->rects.base[i];
        AddRect(dest, rect);
    }
    SetTexturingProperty(dest, self->texturingProperty);
    dest->flags = self->flags;
}

// 0x7DBE00 vf4 LoadBinary(stream)
void __fastcall LoadBinaryImpl(NiScreenTexture* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    uint32_t count;
    LoadRaw(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        ScreenRect rect;
        rect.color[0] = 0.0f;
        rect.color[1] = 0.0f;
        rect.color[2] = 0.0f;
        rect.color[3] = 0.0f;
        LoadRaw(stream, &rect.top, 2);
        LoadRaw(stream, &rect.left, 2);
        LoadRaw(stream, &rect.width, 2);
        LoadRaw(stream, &rect.height, 2);
        LoadRaw(stream, &rect.pixTop, 2);
        LoadRaw(stream, &rect.pixLeft, 2);
        ColorA_LoadBinary(rect.color, stream);
        AddRect(self, rect);
    }
    Stream_ReadLinkID(stream);   // the texturing property
}

// 0x7DBFA0 vf3 CreateClone(cloning)
NiScreenTexture* __fastcall CreateCloneImpl(NiScreenTexture* self, re::Edx, void* cloning) {
    NiScreenTexture* clone = static_cast<NiScreenTexture*>(NiNew(0x20));
    if (clone == nullptr) {
        ScreenTexture_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    RefObject_Construct(clone);
    clone->vtable = reinterpret_cast<void*>(kVtable);
    ConstructFields(clone);
    clone->unknown1C = 0;
    ScreenTexture_CopyMembers(self, clone, cloning);
    return clone;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISCREENTEX_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

RE_UNHOOKED("NiScreenTexture::GetViewerStrings (jump)", 0x007DB6A0);   // -> NiObject's, 0x7555A0
NISCREENTEX_REVERSED("NiScreenTexture::RemoveAllScreenRects", 0x007DB6B0, ni::RemoveAllScreenRectsImpl);
NISCREENTEX_REVERSED("NiScreenTexture::Draw", 0x007DB6C0, ni::DrawImpl);
NISCREENTEX_REVERSED("NiScreenTexture::RegisterStreamables", 0x007DB6E0, ni::RegisterStreamablesImpl);
NISCREENTEX_REVERSED("NiTPrimitiveSet<NiScreenTexture::ScreenRect>::Realloc", 0x007DB710, ni::RectSet_ReallocImpl);
NISCREENTEX_REVERSED("NiTexturingProperty::SetBaseClampMode", 0x007DB780,
                     ni::TexturingProperty_SetBaseClampModeImpl);
NISCREENTEX_REVERSED("NiScreenTexture::GetRTTI", 0x007DB7E0, ni::GetRTTIImpl);
NISCREENTEX_REVERSED("NiScreenTexture::LinkObject", 0x007DB7F0, ni::LinkObjectImpl);
NISCREENTEX_REVERSED("NiScreenTexture::SaveBinary", 0x007DB850, ni::SaveBinaryImpl);
NISCREENTEX_REVERSED("NiScreenTexture::IsEqual", 0x007DB9D0, ni::IsEqualImpl);
NISCREENTEX_REVERSED("NiScreenTexture::NiScreenTexture", 0x007DBAB0, ni::ConstructImpl);
NISCREENTEX_REVERSED("NiScreenTexture::DeletingDestructor", 0x007DBB50, ni::DeletingDestructorImpl);
NISCREENTEX_REVERSED("NiScreenTexture::AddScreenRect", 0x007DBBB0, ni::AddScreenRectImpl);
NISCREENTEX_REVERSED("NiScreenTexture::CopyMembers", 0x007DBC70, ni::CopyMembersImpl);
NISCREENTEX_REVERSED("NiScreenTexture::LoadBinary", 0x007DBE00, ni::LoadBinaryImpl);
NISCREENTEX_REVERSED("NiScreenTexture::CreateClone", 0x007DBFA0, ni::CreateCloneImpl);
