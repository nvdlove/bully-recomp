// The Im3D batch's pooled shapes (Im3DBatch.cpp draws with them): the NiLines and
// NiTriShape stand-ins' constructors (their data made, the model data shared, a property
// attached, a vertex colour property added), the lines' destructors, the vertex colour
// property, and a property state's destructor. 0x5EFBD0..0x5EFF20.

#include <cstdint>

#include "reversed.h"
#include "game/VCall.h"

namespace {

constexpr uintptr_t kVertexColorPropertyVtable = 0x0092D0D4;
constexpr uintptr_t kLineShapeVtable = 0x0092D12C;
constexpr uintptr_t kTriShapeVtable = 0x0092D1CC;
inline void* const im_propertySlot = reinterpret_cast<void*>(0x00BCC4B4);

GAME_FN(void*, __cdecl, IS_NiNew, 0x00752460, uint32_t size);
GAME_FN(void, __cdecl, IS_NiDelete, 0x00752540, void* p, uint32_t size);
GAME_FN(void, __thiscall, IS_Property_Construct, 0x00764550, void* self);
GAME_FN(void, __thiscall, IS_Property_Destruct, 0x00764590, void* self);
GAME_FN(void*, __thiscall, IS_LinesData_Construct, 0x00779C50, void* self);
GAME_FN(void, __thiscall, IS_Lines_Construct, 0x00779980, void* self, void* data);
GAME_FN(void*, __thiscall, IS_TriShapeData_Construct, 0x0077BA40, void* self);
GAME_FN(void, __thiscall, IS_TriShape_Construct, 0x0077B860, void* self, void* data);
GAME_FN(void, __thiscall, IS_PropertyState_Construct, 0x0051A460, void* self, uintptr_t);
GAME_FN(void, __thiscall, IS_PropertyState_Set, 0x0051A2D0, void* self, void* object);
GAME_FN(void, __thiscall, IS_AttachProperty, 0x00762080, void* self, void* property, int);
GAME_FN(void, __thiscall, IS_AddProperty, 0x00407BD0, void* list, re::Ptr32<void>* property);
GAME_FN(void, __thiscall, IS_Update, 0x0075DB00, void* self, float time, int);
GAME_FN(void, __thiscall, IS_UpdateProperties, 0x0075E040, void* self);
GAME_FN(void, __thiscall, IS_UpdateEffects, 0x0075E240, void* self);
GAME_FN(void, __thiscall, IS_LinesData_SetFlags, 0x00779C70, void* data, uint8_t* flags);
GAME_FN(void, __thiscall, IS_GeometryData_SetData, 0x0077A030, void* data, int count, float* positions,
        float* normals, float* colors, float* uvs, int uvSets, int);
GAME_FN(void, __thiscall, IS_Geometry_Destruct, 0x00761D10, void* self);
GAME_FN(void, __thiscall, IS_PropertyState_Destruct, 0x005EFC30, void* self);
GAME_FN(void, __thiscall, IS_Lines_Destruct, 0x005EFE60, void* self);

// a reference dropped (its vf1 when none are left)
void Release(uint8_t* object) {
    if (vc::Decrement(object + 4) == 0) {
        using Fn = void(__thiscall*)(void*);
        re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[1])(object);
    }
}
void ReleaseIfAny(uint8_t* object) {
    if (object) {
        Release(object);
    }
}

// 0x5EFBD0 (thiscall): NiVertexColorProperty's constructor (flags 8)
uint8_t* __fastcall VertexColorProperty_Construct(uint8_t* self) {
    IS_Property_Construct(self);
    *reinterpret_cast<uint32_t*>(self) = kVertexColorPropertyVtable;
    *reinterpret_cast<uint16_t*>(self + 0x18) = 8;
    return self;
}

// 0x5EFBF0 (vf2): the RTTI
void* __cdecl VertexColorProperty_GetRTTI() {
    return reinterpret_cast<void*>(0x020E3008);
}

// 0x5EFC00 (vf18): the property type
int __cdecl VertexColorProperty_Type() {
    return 9;
}

// 0x5EFC10 (thiscall, vf0): the deleting destructor
uint8_t* __fastcall VertexColorProperty_Delete(uint8_t* self, re::Edx, uint32_t flags) {
    IS_Property_Destruct(self);
    if (flags & 1) {
        IS_NiDelete(self, 0x1C);
    }
    return self;
}

// 0x5EFC30 (thiscall): a property state's destructor: its pointers released, the last
// first (+0x14 isn't one)
void __fastcall PropertyState_Destruct(re::Ptr32<uint8_t>* self) {
    ReleaseIfAny(self[6]);
    ReleaseIfAny(self[4]);
    ReleaseIfAny(self[3]);
    ReleaseIfAny(self[2]);
    ReleaseIfAny(self[1]);
    ReleaseIfAny(self[0]);
}

// the constructors' shared part, after the base's: the model data (+0xCC) shared into
// +0xEC and flagged, the property at 0xBCC4B4 attached, the property state (+0xF0) set,
// a vertex colour property (flags | 0x10, ~0x28) added, and the object updated
void FinishShape(uint8_t* self) {
    re::Ptr32<uint8_t>& data = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xEC);
    uint8_t* model = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xCC);
    if (data != model) {
        if (data) {
            Release(data);
        }
        data = model;
        if (model) {
            vc::Increment(model + 4);
        }
    }
    uint16_t& dataFlags = *reinterpret_cast<uint16_t*>(data + 0x34);
    dataFlags = uint16_t((dataFlags & 0xFFF) | 0x8000);
    IS_AttachProperty(self, im_propertySlot, -1);
    IS_PropertyState_Set(self + 0xF0, self);
    uint8_t* property = static_cast<uint8_t*>(IS_NiNew(0x1C));
    if (property) {
        IS_Property_Construct(property);
        *reinterpret_cast<uint32_t*>(property) = kVertexColorPropertyVtable;
        *reinterpret_cast<uint16_t*>(property + 0x18) = 8;
        vc::Increment(property + 4);
    }
    // (with no memory the game carries on with a null property and faults here)
    uint16_t& flags = *reinterpret_cast<uint16_t*>(property + 0x18);
    flags = uint16_t((flags & 0xFFD7) | 0x10);
    re::Ptr32<void> argument = property;
    vc::Increment(property + 4);
    IS_AddProperty(self + 0x94, &argument);
    Release(property);
    IS_Update(self, 0.0f, 1);
    IS_UpdateProperties(self);
    IS_UpdateEffects(self);
    Release(property);
}

// 0x5EFCE0 (thiscall): the line shape's constructor
uint8_t* __fastcall LineShape_Construct(uint8_t* self) {
    void* memory = IS_NiNew(0x44);
    void* data = memory ? IS_LinesData_Construct(memory) : nullptr;
    IS_Lines_Construct(self, data);
    *reinterpret_cast<uint32_t*>(self) = kLineShapeVtable;
    *reinterpret_cast<re::Ptr32<void>*>(self + 0xEC) = nullptr;
    IS_PropertyState_Construct(self + 0xF0, 0);
    FinishShape(self);
    return self;
}

// 0x5EFE50 (vf2): the line shape's RTTI
void* __cdecl LineShape_GetRTTI() {
    return reinterpret_cast<void*>(0x020E3040);
}

// 0x5EFE60 (thiscall): the line shape's destructor: its data emptied, the shared data
// dropped, the property state destroyed, then the base's
void __fastcall LineShape_Destruct(uint8_t* self) {
    *reinterpret_cast<uint32_t*>(self) = kLineShapeVtable;
    IS_LinesData_SetFlags(*reinterpret_cast<re::Ptr32<void>*>(self + 0xEC), nullptr);
    IS_GeometryData_SetData(*reinterpret_cast<re::Ptr32<void>*>(self + 0xEC), 0, nullptr, nullptr, nullptr, nullptr, 0, 0);
    re::Ptr32<uint8_t>& data = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xEC);
    if (data) {
        Release(data);
        data = nullptr;
    }
    IS_PropertyState_Destruct(self + 0xF0);
    ReleaseIfAny(data);
    IS_Geometry_Destruct(self);
}

// 0x5EFEF0 (thiscall, vf0): the line shape's deleting destructor
uint8_t* __fastcall LineShape_Delete(uint8_t* self, re::Edx, uint32_t flags) {
    IS_Lines_Destruct(self);
    if (flags & 1) {
        IS_NiDelete(self, 0x10C);
    }
    return self;
}

// 0x5EFF20 (thiscall): the triangle shape's constructor
uint8_t* __fastcall TriShape_Construct(uint8_t* self) {
    void* memory = IS_NiNew(0x58);
    void* data = memory ? IS_TriShapeData_Construct(memory) : nullptr;
    IS_TriShape_Construct(self, data);
    *reinterpret_cast<uint32_t*>(self) = kTriShapeVtable;
    *reinterpret_cast<re::Ptr32<void>*>(self + 0xEC) = nullptr;
    IS_PropertyState_Construct(self + 0xF0, 0);
    FinishShape(self);
    return self;
}

#define IM3DSHAPES_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Im3DShapesOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Im3DShapesOriginal_, __LINE__), \
             re::Mode::Original)

IM3DSHAPES_REVERSED("NiVertexColorProperty::Construct", 0x005EFBD0, &VertexColorProperty_Construct);
IM3DSHAPES_REVERSED("NiVertexColorProperty::GetRTTI", 0x005EFBF0, &VertexColorProperty_GetRTTI);
IM3DSHAPES_REVERSED("NiVertexColorProperty::Type", 0x005EFC00, &VertexColorProperty_Type);
IM3DSHAPES_REVERSED("NiVertexColorProperty::DeletingDestruct", 0x005EFC10, &VertexColorProperty_Delete);
IM3DSHAPES_REVERSED("NiPropertyState::Destruct", 0x005EFC30, &PropertyState_Destruct);
IM3DSHAPES_REVERSED("LineShape::Construct", 0x005EFCE0, &LineShape_Construct);
IM3DSHAPES_REVERSED("LineShape::GetRTTI", 0x005EFE50, &LineShape_GetRTTI);
IM3DSHAPES_REVERSED("LineShape::Destruct", 0x005EFE60, &LineShape_Destruct);
IM3DSHAPES_REVERSED("LineShape::DeletingDestruct", 0x005EFEF0, &LineShape_Delete);
IM3DSHAPES_REVERSED("TriShape::Construct", 0x005EFF20, &TriShape_Construct);

} // namespace
