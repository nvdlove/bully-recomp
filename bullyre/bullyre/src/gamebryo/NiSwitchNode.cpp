// The NiNode kinds of this part of the library, the slots the first passes left:
//   NiSwitchNode     (vtable 0x960F10, 0x114 bytes): its flags (+0xF4: 1 update only the active
//                    child, 2 the controllers' update flag), the active index (+0xF8, -1), the
//                    last update time (+0xFC), the revision (+0x100) and each child's
//                    (NiTPrimitiveArray<unsigned int> +0x104)
//   NiLODNode        (vtable 0x961460, 0x11C bytes): an NiSwitchNode whose index comes from its
//                    NiLODData (+0x114, a reference; slot 18 GetLODLevel, 19 UpdateWorldData,
//                    20 Duplicate, 21 GetLODIndex) when LOD is on (+0x118)
//   NiSortAdjustNode (vtable 0x9610C8, 0xF8 bytes): the sorting mode (+0xF4) its visible
//                    objects are flagged with
//   NiBSPNode        (vtable 0x961618, 0x114 bytes): two children on the two sides of a model
//                    plane (+0xF4) and its world plane (+0x104)
//   NiBillboardNode  (vtable 0x9616C8, 0xFC bytes): turned to the camera as it's drawn (the mode
//                    in bits 0..2 of +0xF4, bit 3 the controllers' update flag), the last update
//                    time (+0xF8)
// Main-thread maths: float, in the game's order.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiSwitchNode_RTTI, 0x020E3C00);
GAME_VAR(RTTI, NiLODNode_RTTI, 0x020E3E48);
GAME_VAR(RTTI, NiSortAdjustNode_RTTI, 0x020E3CD4);
GAME_VAR(RTTI, NiBSPNode_RTTI, 0x020E3EE8);
GAME_VAR(RTTI, NiBillboardNode_RTTI, 0x020E3F1C);

constexpr uintptr_t NiSwitchNode_vtable = 0x00960F10;
constexpr uintptr_t NiLODNode_vtable = 0x00961460;
constexpr uintptr_t NiSortAdjustNode_vtable = 0x009610C8;
constexpr uintptr_t NiBSPNode_vtable = 0x00961618;
constexpr uintptr_t NiBillboardNode_vtable = 0x009616C8;
constexpr uintptr_t UintArray_vtable = 0x00961988;   // NiTPrimitiveArray<unsigned int>

GAME_VAR(int32_t, NiLODNode_ms_iGlobalLOD, 0x00BC8B70);   // -1: the LOD data decides
GAME_ARRAY(float, Matrix3_IDENTITY, 0x00BC75FC);

struct NiPlane {
    NiPoint3 normal;   // +0x0
    float constant;    // +0xC
};
static_assert(sizeof(NiPlane) == 0x10);

struct NiSwitchNode : NiNode {
    uint16_t flags;                     // +0xF4
    uint16_t padF6;
    int32_t index;                      // +0xF8
    float savedTime;                    // +0xFC
    uint32_t revision;                  // +0x100
    NiTArray<uint32_t> childRevisions;  // +0x104
};
static_assert(offsetof(NiSwitchNode, index) == 0xF8);
static_assert(offsetof(NiSwitchNode, childRevisions) == 0x104);
static_assert(sizeof(NiSwitchNode) == 0x114);

struct NiLODNode : NiSwitchNode {
    re::Ptr32<NiObject> lodData;   // +0x114
    uint8_t lodActive;   // +0x118
    uint8_t pad119[3];
};
static_assert(sizeof(NiLODNode) == 0x11C);

struct NiSortAdjustNode : NiNode {
    int32_t sortingMode;   // +0xF4 (0 inherit, 1 off)
};
static_assert(sizeof(NiSortAdjustNode) == 0xF8);

struct NiBSPNode : NiNode {
    NiPlane modelPlane;   // +0xF4
    NiPlane worldPlane;   // +0x104
};
static_assert(sizeof(NiBSPNode) == 0x114);

struct NiBillboardNode : NiNode {
    uint16_t flags;     // +0xF4
    uint16_t padF6;
    float lastTime;     // +0xF8
};
static_assert(sizeof(NiBillboardNode) == 0xFC);

// NiNode's (another chunk's)
GAME_FN(void, __thiscall, Node_Construct, 0x00760780, void* self, uint32_t children);
GAME_FN(void, __thiscall, Node_Destruct, 0x007603B0, void* self);
GAME_FN(void, __thiscall, Node_CopyMembers, 0x0075F570, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Node_LoadBinary, 0x0075F600, void* self, void* stream);
GAME_FN(void, __thiscall, Node_SaveBinary, 0x007601D0, void* self, void* stream);
GAME_FN(void, __thiscall, Node_LinkObject, 0x00760590, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Node_RegisterStreamables, 0x00760150, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Node_IsEqual, 0x007602B0, void* self, void* other);
GAME_FN(void, __thiscall, Node_GetViewerStrings, 0x00760A80, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Node_OnVisible, 0x0075F3E0, void* self, void* culler);
GAME_FN(void, __thiscall, Node_UpdateDownwardPass, 0x0075F800, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, Node_UpdateSelectedDownwardPass, 0x0075F8C0, void* self, float time);
GAME_FN(void, __thiscall, Node_UpdateRigidDownwardPass, 0x0075F9B0, void* self, float time);
GAME_FN(void, __thiscall, AVObject_UpdateWorldData, 0x00779EB0, void* self);
GAME_FN(void, __thiscall, AVObject_Cull, 0x0075DC50, void* self, void* culler);
// NiSwitchNode's (another chunk's)
GAME_FN(void, __thiscall, SwitchNode_OnVisible, 0x007D9560, void* self, void* culler);
GAME_FN(uint8_t, __thiscall, SwitchNode_IsEqual, 0x007D96E0, void* self, void* other);
GAME_FN(void, __thiscall, SwitchNode_SaveBinary, 0x007D9A50, void* self, void* stream);
GAME_FN(void, __thiscall, SwitchNode_GetViewerStrings, 0x007D9CD0, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, UintArray_SetAt, 0x00833C70, void* array, uint32_t index, const uint32_t* value);
GAME_FN(void, __thiscall, ObjectArray_SetSize, 0x00856170, void* array, uint32_t size);
// the maths
GAME_FN(void, __thiscall, Transform_MakeIdentity, 0x00770920, NiTransform* self);
GAME_FN(NiTransform*, __thiscall, Transform_Multiply, 0x00406830, const NiTransform* self, NiTransform* result,
        const NiTransform* other);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Multiply, 0x00768920, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_TransposeTimes, 0x00768F60, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(uint8_t, __thiscall, Matrix3_Inverse, 0x00768B30, const NiMatrix3* self, NiMatrix3* result);
GAME_FN(NiPoint3*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(NiPoint3*, __cdecl, Point3_TimesMatrix, 0x00768AD0, NiPoint3* result, const NiPoint3* point,
        const NiMatrix3* matrix);
GAME_FN(void, __thiscall, Matrix3_MakeRotation, 0x00768820, NiMatrix3* self, float angle, float x, float y, float z);
GAME_FN(float, __cdecl, NiACos, 0x00768740, float value);
GAME_FN(float, __thiscall, Point3_Unitize, 0x00415130, NiPoint3* self);
GAME_FN(NiPoint3*, __thiscall, Point3_UnitCross, 0x00762F30, const NiPoint3* self, NiPoint3* result,
        const NiPoint3* other);
GAME_FN(void, __thiscall, Plane_Construct, 0x00770FE0, NiPlane* self);
GAME_FN(char*, __thiscall, Plane_GetViewerString, 0x00771070, const NiPlane* self, const char* prefix);
GAME_FN(void, __thiscall, Plane_LoadBinary, 0x007D23D0, NiPlane* self, void* stream);   // (NiBound's: 16 bytes)
GAME_FN(void, __thiscall, Plane_SaveBinary, 0x007D2410, const NiPlane* self, void* stream);
GAME_FN(uint8_t, __thiscall, Plane_NotEqual, 0x00769390, const NiPlane* self, const NiPlane* other);   // (NiColorA's)
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);

// (this file's, called where modes apply)
GAME_FN(NiSwitchNode*, __thiscall, SwitchNode_Construct, 0x007D9AC0, void* self);
GAME_FN(void, __thiscall, SwitchNode_CopyMembers, 0x007D93A0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, SwitchNode_LoadBinary, 0x007D99E0, void* self, void* stream);
GAME_FN(void, __thiscall, LODNode_SetLODData, 0x007DDEF0, NiLODNode* self, NiObject* data);
GAME_FN(void, __thiscall, LODNode_Destruct, 0x007DDF40, NiLODNode* self);
GAME_FN(void, __thiscall, LODNode_CopyMembers, 0x007DDFA0, NiLODNode* self, NiLODNode* dest, void* cloning);
GAME_FN(uint32_t, __thiscall, Plane_WhichSide, 0x007DEA20, const NiPlane* self, const NiPoint3* point);
GAME_FN(void, __thiscall, BSPNode_UpdateWorldPlane, 0x007DEA70, NiBSPNode* self);
GAME_FN(uint8_t, __thiscall, Billboard_FaceCenter, 0x007DF0B0, void* self, void* camera, const NiPoint3* translate,
        NiPoint3* dir, NiPoint3* up, NiPoint3* right);
GAME_FN(void, __thiscall, Billboard_RotateToCamera, 0x007DF350, NiBillboardNode* self, void* camera);
GAME_FN(char*, __cdecl, Billboard_GetViewerString, 0x007DFBE0, const char* prefix, uint32_t mode);

// a reference slot assigned through the imports: the old one let go, then the new one held
template <typename T>
inline void SetReference(re::Ptr32<T>& slot, std::type_identity_t<T>* value) {
    T* old = slot;
    if (old == value) {
        return;
    }
    if (old != nullptr && ni::sys::Decrement(&old->refCount) == 0) {
        Virtual<void>(old, 1);
    }
    slot = value;
    if (value != nullptr) {
        ni::sys::Increment(&value->refCount);
    }
}

// the camera's world rotation's columns: the view direction, up, right (a camera looks down its
// first column)
inline NiPoint3 CameraColumn(void* camera, int column) {
    const NiMatrix3& m = static_cast<NiAVObject*>(camera)->world.rotate;
    return {m.m[0][column], m.m[1][column], m.m[2][column]};
}

// ---- NiSwitchNode

// CopyMembers: NiNode's, the flags and the index
void __fastcall SwitchCopyMembers(NiSwitchNode* self, re::Edx, NiSwitchNode* dest, void* cloning) {
    Node_CopyMembers(self, dest, cloning);
    dest->flags = self->flags;
    dest->index = self->index;
}

// the flags and the index (the controllers' update flag set)
void __fastcall SwitchLoadBinary(NiSwitchNode* self, re::Edx, void* stream) {
    Node_LoadBinary(self, stream);
    LoadBytes(stream, &self->flags, 2);
    LoadBytes(stream, &self->index, 4);
    self->flags |= 2;
}

// NiSwitchNode(): no children, none active, revision 1, the child revisions (one slot), both
// flags set
NiSwitchNode* __fastcall SwitchConstruct(NiSwitchNode* self, re::Edx) {
    Node_Construct(self, 0);
    self->savedTime = 0.0f;
    self->vtable = reinterpret_cast<void*>(NiSwitchNode_vtable);
    self->flags = 0;
    self->index = -1;
    self->revision = 1;
    self->childRevisions.vtable = reinterpret_cast<void*>(UintArray_vtable);
    self->childRevisions.maxSize = 1;
    self->childRevisions.growBy = 1;
    self->childRevisions.size = 0;
    self->childRevisions.esize = 0;
    self->childRevisions.base = static_cast<uint32_t*>(NiMalloc(4));
    self->childRevisions.vtable = reinterpret_cast<void*>(UintArray_vtable);
    self->flags |= 3;
    return self;
}

const RTTI* __fastcall SwitchGetRTTI(void*, re::Edx) {
    return &NiSwitchNode_RTTI;
}

// the deleting destructor: the child revisions freed, NiNode's destructor
NiSwitchNode* __fastcall SwitchDeletingDestructor(NiSwitchNode* self, re::Edx, uint32_t flags) {
    self->childRevisions.vtable = reinterpret_cast<void*>(UintArray_vtable);
    NiFree(self->childRevisions.base);
    Node_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x114);
    }
    return self;
}

void* __cdecl SwitchCreateObject() {
    void* memory = NiNew(0x114);
    if (memory == nullptr) {
        return nullptr;
    }
    return SwitchNode_Construct(memory);
}

// the children not updated since the last switch (their revision behind the node's): updated now
// (slot 25 UpdateDownwardPass) at the saved time, their revision brought up
void __fastcall SwitchUpdateChildren(NiSwitchNode* self, re::Edx) {
    for (int32_t i = 0; i < int32_t(self->children.size); i++) {
        NiAVObject* child = self->children.base[i];
        if (child == nullptr || self->childRevisions.base[i] == self->revision) {
            continue;
        }
        UintArray_SetAt(&self->childRevisions, uint32_t(i), &self->revision);
        Virtual<void>(child, 25, self->savedTime, int((self->flags >> 1) & 1));
    }
}

// ---- NiLODNode

// LoadBinary: NiSwitchNode's, then the LOD data's link
void __fastcall LODLoadBinary(NiLODNode* self, re::Edx, void* stream) {
    SwitchNode_LoadBinary(self, stream);
    Stream_ReadLinkID(stream);
}

// slot 30 UpdateWorldData: NiAVObject's, then the LOD data's (its slot 19)
void __fastcall LODUpdateWorldData(NiLODNode* self, re::Edx) {
    AVObject_UpdateWorldData(self);
    if (self->lodData != nullptr) {
        Virtual<void>(self->lodData, 19, static_cast<void*>(self));
    }
}

// slot 32 OnVisible: with LOD on, the index from the LOD data (its level for the culler's camera,
// or the global level when there is one), moved down to a child that exists; then NiSwitchNode's
void __fastcall LODOnVisible(NiLODNode* self, re::Edx, void* culler) {
    if (self->lodActive != 0 && self->lodData != nullptr) {
        self->index = Virtual<int32_t>(self->lodData, 18, At<void*>(culler, 0x10), static_cast<void*>(self));
        if (NiLODNode_ms_iGlobalLOD >= 0) {
            self->index = Virtual<int32_t>(self->lodData, 21, NiLODNode_ms_iGlobalLOD);
        }
        while (self->index >= 0) {
            int32_t index = self->index;
            if (index < int32_t(self->children.size) && self->children.base[index] != nullptr) {
                break;
            }
            self->index = index - 1;
        }
    }
    SwitchNode_OnVisible(self, culler);
}

// LinkObject: NiNode's, then the LOD data
void __fastcall LODLinkObject(NiLODNode* self, re::Edx, void* stream) {
    Node_LinkObject(self, stream);
    NiObject* data = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
    SetReference(self->lodData, data);
}

uint8_t __fastcall LODRegisterStreamables(NiLODNode* self, re::Edx, void* stream) {
    if (!Node_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->lodData != nullptr) {
        Virtual<uint8_t>(self->lodData, 6, stream);
    }
    return 1;
}

// SaveBinary: NiSwitchNode's, then the LOD data's link (the stream's slot 12)
void __fastcall LODSaveBinary(NiLODNode* self, re::Edx, void* stream) {
    SwitchNode_SaveBinary(self, stream);
    Virtual<void>(stream, 12, self->lodData);
}

void __fastcall LODSetLODData(NiLODNode* self, re::Edx, NiObject* data) {
    SetReference(self->lodData, data);
}

// the LOD data let go, the child revisions freed, NiNode's destructor
void __fastcall LODDestruct(NiLODNode* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiLODNode_vtable);
    NiObject* data = self->lodData;
    if (data != nullptr && ni::sys::Decrement(&data->refCount) == 0) {
        Virtual<void>(data, 1);
    }
    self->childRevisions.vtable = reinterpret_cast<void*>(UintArray_vtable);
    NiFree(self->childRevisions.base);
    Node_Destruct(self);
}

const RTTI* __fastcall LODGetRTTI(void*, re::Edx) {
    return &NiLODNode_RTTI;
}

// CopyMembers: NiSwitchNode's, then a duplicate of the LOD data (its slot 20), or none
void __fastcall LODCopyMembers(NiLODNode* self, re::Edx, NiLODNode* dest, void* cloning) {
    SwitchNode_CopyMembers(self, dest, cloning);
    if (self->lodData != nullptr) {
        LODNode_SetLODData(dest, Virtual<NiObject*>(self->lodData, 20));
        return;
    }
    NiObject* old = dest->lodData;
    if (old != nullptr) {
        if (ni::sys::Decrement(&old->refCount) == 0) {
            Virtual<void>(old, 1);
        }
        dest->lodData = nullptr;
    }
}

// NiSwitchNode's, then both LOD data or neither; of the same class, compared (slot 8). (Data of
// different classes compare equal, as the game has it)
uint8_t __fastcall LODIsEqual(NiLODNode* self, re::Edx, NiLODNode* other) {
    if (!SwitchNode_IsEqual(self, other)) {
        return 0;
    }
    NiObject* a = self->lodData;
    NiObject* b = other->lodData;
    if (a == nullptr) {
        return b == nullptr ? 1 : 0;
    }
    if (b == nullptr) {
        return 0;
    }
    if (Virtual<const RTTI*>(a, 2) == Virtual<const RTTI*>(b, 2) && !Virtual<uint8_t>(self->lodData, 8, b)) {
        return 0;
    }
    return 1;
}

NiLODNode* __fastcall LODDeletingDestructor(NiLODNode* self, re::Edx, uint32_t flags) {
    LODNode_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x11C);
    }
    return self;
}

inline NiLODNode* NewLODNode() {
    NiLODNode* node = static_cast<NiLODNode*>(NiNew(0x11C));
    if (node == nullptr) {
        return nullptr;
    }
    SwitchNode_Construct(node);
    node->vtable = reinterpret_cast<void*>(NiLODNode_vtable);
    node->lodData = nullptr;
    node->index = 0;
    node->lodActive = 1;
    return node;
}

// vf3 CreateClone (copied into a null clone when the allocation fails, as the game has it)
NiLODNode* __fastcall LODCreateClone(NiLODNode* self, re::Edx, void* cloning) {
    NiLODNode* clone = NewLODNode();
    LODNode_CopyMembers(self, clone, cloning);
    return clone;
}

NiLODNode* __cdecl LODCreateObject() {
    return NewLODNode();
}

// NiSwitchNode's, the class, the LOD data's strings (slot 9) or "NULL LOD Data" (the literal itself
// put in the array, as the game has it)
void __fastcall LODGetViewerStrings(NiLODNode* self, re::Edx, NiTArray<char*>* strings) {
    SwitchNode_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiLODNode_RTTI.name));
    if (self->lodData != nullptr) {
        Virtual<void>(self->lodData, 9, strings);
        return;
    }
    AddViewerString(strings, const_cast<char*>("NULL LOD Data"));
}

// ---- NiSortAdjustNode

void* __cdecl SortAdjustCreateObject() {
    NiSortAdjustNode* node = static_cast<NiSortAdjustNode*>(NiNew(0xF8));
    if (node == nullptr) {
        return nullptr;
    }
    Node_Construct(node, 0);
    node->vtable = reinterpret_cast<void*>(NiSortAdjustNode_vtable);
    node->sortingMode = 0;
    return node;
}

uint8_t __fastcall SortAdjustIsEqual(NiSortAdjustNode* self, re::Edx, NiSortAdjustNode* other) {
    if (!Node_IsEqual(self, other)) {
        return 0;
    }
    return self->sortingMode == other->sortingMode ? 1 : 0;
}

const RTTI* __fastcall SortAdjustGetRTTI(void*, re::Edx) {
    return &NiSortAdjustNode_RTTI;
}

NiSortAdjustNode* __fastcall SortAdjustDeletingDestructor(NiSortAdjustNode* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiSortAdjustNode_vtable);
    Node_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xF8);
    }
    return self;
}

// slot 32 OnVisible: NiNode's, then the objects it added to the culler's visible set (+0xC: its
// array, count) flagged (0x40 of their flags) with sorting off (mode 1) or cleared
void __fastcall SortAdjustOnVisible(NiSortAdjustNode* self, re::Edx, void* culler) {
    int32_t mode = self->sortingMode;
    uint32_t first = At<uint32_t>(At<void*>(culler, 0xC), 4);
    Node_OnVisible(self, culler);
    void* visible = At<void*>(culler, 0xC);
    uint32_t count = At<uint32_t>(visible, 4);
    for (uint32_t i = first; i < count; i++) {
        NiAVObject* object = At<re::Ptr32<NiAVObject>*>(visible, 0)[i];
        if (mode == 1) {
            object->flags |= 0x40;
        } else {
            object->flags &= 0xFFBF;
        }
    }
}

// vf3 CreateClone (the clone isn't checked: a failed allocation crashes, as in the game)
NiSortAdjustNode* __fastcall SortAdjustCreateClone(NiSortAdjustNode* self, re::Edx, void* cloning) {
    NiSortAdjustNode* clone = static_cast<NiSortAdjustNode*>(NiNew(0xF8));
    if (clone != nullptr) {
        Node_Construct(clone, 0);
        clone->vtable = reinterpret_cast<void*>(NiSortAdjustNode_vtable);
        clone->sortingMode = 0;
    }
    Node_CopyMembers(self, clone, cloning);
    clone->sortingMode = self->sortingMode;
    return clone;
}

// LinkObject: NiNode's, and (before version 20.0.0.4) the old accumulator link resolved and dropped
void __fastcall SortAdjustLinkObject(NiSortAdjustNode* self, re::Edx, void* stream) {
    Node_LinkObject(self, stream);
    if (StreamVersion(stream) < 0x14000004) {
        Stream_ResolveLinkID(stream);
    }
}

// the mode (past 1: 0), and before version 20.0.0.4 the old accumulator's link
void __fastcall SortAdjustLoadBinary(NiSortAdjustNode* self, re::Edx, void* stream) {
    Node_LoadBinary(self, stream);
    int32_t mode = 0;
    LoadBytes(stream, &mode, 4);
    self->sortingMode = mode;
    if (mode > 1) {
        self->sortingMode = 0;
    }
    if (StreamVersion(stream) < 0x14000004) {
        Stream_ReadLinkID(stream);
    }
}

void __fastcall SortAdjustSaveBinary(NiSortAdjustNode* self, re::Edx, void* stream) {
    Node_SaveBinary(self, stream);
    SaveBytes(stream, &self->sortingMode, 4);
}

// ---- NiBSPNode

void __fastcall BSPLoadBinary(NiBSPNode* self, re::Edx, void* stream) {
    Node_LoadBinary(self, stream);
    Plane_LoadBinary(&self->modelPlane, stream);
}

void __fastcall BSPSaveBinary(NiBSPNode* self, re::Edx, void* stream) {
    Node_SaveBinary(self, stream);
    Plane_SaveBinary(&self->modelPlane, stream);
}

// NiPlane::WhichSide(point): 2 behind, 1 in front, 0 on it (or not a number)
uint32_t __fastcall PlaneWhichSide(const NiPlane* self, re::Edx, const NiPoint3* point) {
    float distance = point->y * self->normal.y + point->x * self->normal.x + point->z * self->normal.z - self->constant;
    if (distance < 0.0f) {
        return 2;
    }
    if (distance > 0.0f) {
        return 1;
    }
    return 0;
}

// the world plane from the model plane: its normal by the world rotation, its constant moved by the
// world translation and scaled
void __fastcall BSPUpdateWorldPlane(NiBSPNode* self, re::Edx) {
    NiPoint3 normal;
    Matrix3_TimesPoint(&self->world.rotate, &normal, &self->modelPlane.normal);
    const NiPoint3& t = self->world.translate;
    float dot = t.y * normal.y + t.x * normal.x;
    self->worldPlane.normal = normal;
    dot = dot + t.z * normal.z;
    self->worldPlane.constant = dot + self->modelPlane.constant * self->world.scale;
}

// slot 30 UpdateWorldData: NiAVObject's, then the world plane
void __fastcall BSPUpdateWorldData(NiBSPNode* self, re::Edx) {
    AVObject_UpdateWorldData(self);
    BSPNode_UpdateWorldPlane(self);
}

// slot 20 ApplyTransform(matrix, translate, on the left): the model plane taken through the
// transform (on the left: as seen from the parent), the children told with "on the left" set
void __fastcall BSPApplyTransform(NiBSPNode* self, re::Edx, const NiMatrix3* matrix, const NiPoint3* translate,
                                  int onLeft) {
    NiPlane& plane = self->modelPlane;
    if (uint8_t(onLeft) == 0) {
        NiMatrix3 inverse;
        Matrix3_Inverse(matrix, &inverse);
        NiPoint3 normal;
        plane.normal = *Point3_TimesMatrix(&normal, &plane.normal, &inverse);
        float dot = plane.normal.y * translate->y + translate->x * plane.normal.x + plane.normal.z * translate->z;
        plane.constant = dot + plane.constant;
        for (uint32_t i = 0; i < self->children.size; i++) {
            NiAVObject* child = self->children.base[i];
            if (child != nullptr) {
                Virtual<void>(child, 20, matrix, translate, 1);
            }
        }
        return;
    }
    NiMatrix3 product;
    Matrix3_Multiply(matrix, &product, &self->local.rotate);
    NiMatrix3 rotate;
    Matrix3_TransposeTimes(&self->local.rotate, &rotate, &product);
    NiPoint3 moved;
    Matrix3_TimesPoint(matrix, &moved, &self->local.translate);
    NiPoint3 total = {translate->x + moved.x, moved.y + translate->y, moved.z + translate->z};
    NiPoint3 delta = {total.x - self->local.translate.x, total.y - self->local.translate.y,
                      total.z - self->local.translate.z};
    float scale = self->local.scale;
    NiPoint3 local;
    const NiPoint3* r = Point3_TimesMatrix(&local, &delta, &self->local.rotate);
    float inverseScale = 1.0f / scale;
    NiPoint3 offset;
    offset.y = r->y * inverseScale;
    offset.z = r->z * inverseScale;
    offset.x = inverseScale * r->x;
    NiMatrix3 inverse;
    Matrix3_Inverse(&rotate, &inverse);
    NiPoint3 normal;
    plane.normal = *Point3_TimesMatrix(&normal, &plane.normal, &inverse);
    float dot = plane.normal.y * offset.y + plane.normal.x * offset.x + plane.normal.z * offset.z;
    plane.constant = dot + plane.constant;
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 20, static_cast<const NiMatrix3*>(&rotate), static_cast<const NiPoint3*>(&offset), 1);
        }
    }
}

// slot 32 OnVisible: the side of the world plane the camera is on (on it: the side its view
// direction points to); the far child culled first
void __fastcall BSPOnVisible(NiBSPNode* self, re::Edx, void* culler) {
    NiAVObject* camera = At<NiAVObject*>(culler, 0x10);
    uint32_t side = Plane_WhichSide(&self->worldPlane, &camera->world.translate);
    if (side == 0) {
        NiPoint3 direction = {camera->world.rotate.m[0][0], camera->world.rotate.m[1][0],
                              camera->world.rotate.m[2][0]};
        side = Plane_WhichSide(&self->worldPlane, &direction);
    }
    NiAVObject* front = self->children.base[0];
    NiAVObject* back = self->children.base[1];
    if (side == 2) {
        if (back != nullptr) {
            AVObject_Cull(back, culler);
        }
        if (front != nullptr) {
            AVObject_Cull(front, culler);
        }
    } else {
        if (front != nullptr) {
            AVObject_Cull(front, culler);
        }
        if (back != nullptr) {
            AVObject_Cull(back, culler);
        }
    }
}

uint8_t __fastcall BSPIsEqual(NiBSPNode* self, re::Edx, NiBSPNode* other) {
    if (!Node_IsEqual(self, other)) {
        return 0;
    }
    return Plane_NotEqual(&self->modelPlane, &other->modelPlane) == 0 ? 1 : 0;
}

const RTTI* __fastcall BSPGetRTTI(void*, re::Edx) {
    return &NiBSPNode_RTTI;
}

NiBSPNode* __fastcall BSPDeletingDestructor(NiBSPNode* self, re::Edx, uint32_t flags) {
    Node_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x114);
    }
    return self;
}

// NiBSPNode(): two child slots (not growing), the planes empty
inline NiBSPNode* NewBSPNode() {
    NiBSPNode* node = static_cast<NiBSPNode*>(NiNew(0x114));
    if (node == nullptr) {
        return nullptr;
    }
    Node_Construct(node, 0);
    node->vtable = reinterpret_cast<void*>(NiBSPNode_vtable);
    Plane_Construct(&node->modelPlane);
    Plane_Construct(&node->worldPlane);
    ObjectArray_SetSize(&node->children, 2);
    node->children.growBy = 0;
    return node;
}

// vf3 CreateClone: the model plane copied (the clone isn't checked, as in the game)
NiBSPNode* __fastcall BSPCreateClone(NiBSPNode* self, re::Edx, void* cloning) {
    NiBSPNode* clone = NewBSPNode();
    Node_CopyMembers(self, clone, cloning);
    clone->modelPlane = self->modelPlane;
    return clone;
}

NiBSPNode* __cdecl BSPCreateObject() {
    return NewBSPNode();
}

void __fastcall BSPGetViewerStrings(NiBSPNode* self, re::Edx, NiTArray<char*>* strings) {
    Node_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSPNode_RTTI.name));
    AddViewerString(strings, Plane_GetViewerString(&self->modelPlane, "m_kModelPlane"));
    AddViewerString(strings, Plane_GetViewerString(&self->worldPlane, "m_kWorldPlane"));
}

// ---- NiBillboardNode

// slot 26 / 27: the time kept, NiNode's, then the world bound (slot 31)
void __fastcall BillboardUpdateSelectedDownwardPass(NiBillboardNode* self, re::Edx, float time) {
    self->lastTime = time;
    Node_UpdateSelectedDownwardPass(self, time);
    Virtual<void>(self, 31);
}

void __fastcall BillboardUpdateRigidDownwardPass(NiBillboardNode* self, re::Edx, float time) {
    self->lastTime = time;
    Node_UpdateRigidDownwardPass(self, time);
    Virtual<void>(self, 31);
}

// the "face center" frame: the unit direction to the camera, the camera's up and right turned by
// the rotation that takes its (reversed) view direction there; false when the camera is too close
// (squared distance under 0.001). The node isn't used
uint8_t __fastcall BillboardFaceCenter(void*, re::Edx, NiAVObject* camera, const NiPoint3* translate, NiPoint3* dir,
                                       NiPoint3* up, NiPoint3* right) {
    NiPoint3 toCamera = {camera->world.translate.x - translate->x, camera->world.translate.y - translate->y,
                         camera->world.translate.z - translate->z};
    float squared = toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z;
    if (squared < 0.001f) {
        return 0;
    }
    Point3_Unitize(&toCamera);
    NiPoint3 column0 = CameraColumn(camera, 0);
    dir->x = -column0.x;
    dir->y = -column0.y;
    dir->z = -column0.z;
    *up = CameraColumn(camera, 1);
    *right = CameraColumn(camera, 2);
    float dot = dir->y * toCamera.y + dir->x * toCamera.x + dir->z * toCamera.z;
    if (double(dot) < 0.9999989867210388) {
        NiPoint3 axis;
        Point3_UnitCross(&toCamera, &axis, dir);
        NiMatrix3 rotation;
        Matrix3_MakeRotation(&rotation, NiACos(dot), axis.x, axis.y, axis.z);
        *dir = toCamera;
        NiPoint3 turned;
        *up = *Matrix3_TimesPoint(&rotation, &turned, up);
        *right = *Matrix3_TimesPoint(&rotation, &turned, right);
    }
    return 1;
}

// slot 25 UpdateDownwardPass(time, update controllers): the controllers' flag set, the time kept,
// NiNode's, then the world bound (slot 31)
void __fastcall BillboardUpdateDownwardPass(NiBillboardNode* self, re::Edx, float time, int updateControllers) {
    self->flags |= 8;
    self->lastTime = time;
    Node_UpdateDownwardPass(self, time, updateControllers);
    Virtual<void>(self, 31);
}

// NiNode's and the same mode
uint8_t __fastcall BillboardIsEqual(NiBillboardNode* self, re::Edx, NiBillboardNode* other) {
    if (!Node_IsEqual(self, other)) {
        return 0;
    }
    return ((uint8_t(other->flags) ^ uint8_t(self->flags)) & 7) == 0 ? 1 : 0;
}

const RTTI* __fastcall BillboardGetRTTI(void*, re::Edx) {
    return &NiBillboardNode_RTTI;
}

NiBillboardNode* __fastcall BillboardDeletingDestructor(NiBillboardNode* self, re::Edx, uint32_t flags) {
    Node_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xFC);
    }
    return self;
}

// the frame turned into the node's world rotation, as columns of the rotation applied to it
inline void ToWorld(const NiMatrix3* world, NiPoint3* v) {
    NiPoint3 result;
    *v = *Point3_TimesMatrix(&result, v, world);
}

// RotateToCamera(camera): the world transform made from the parent's and the local one, then its
// rotation turned toward the camera as the mode says (0 always face the camera, 1 rotate about up,
// 2 rigid face the camera, 3 / 4 the same facing the camera's position), the children updated
void __fastcall BillboardRotateToCamera(NiBillboardNode* self, re::Edx, NiAVObject* camera) {
    NiTransform parentWorld;
    if (self->parent != nullptr) {
        parentWorld = self->parent->world;
    } else {
        Transform_MakeIdentity(&parentWorld);
    }
    NiTransform world;
    self->world = *Transform_Multiply(&parentWorld, &world, &self->local);
    uint32_t mode = self->flags & 7;
    NiMatrix3* w = &self->world.rotate;
    NiMatrix3 rotate;
    std::memcpy(&rotate, Matrix3_IDENTITY, sizeof(rotate));
    float(&m)[3][3] = rotate.m;
    NiPoint3 dir;
    NiPoint3 up;
    NiPoint3 right;
    switch (mode) {
    case 0:
    case 3: {
        if (mode == 3) {
            if (!Billboard_FaceCenter(self, camera, &self->world.translate, &dir, &up, &right)) {
                break;
            }
        } else {
            NiPoint3 column0 = CameraColumn(camera, 0);
            dir = {-column0.x, -column0.y, -column0.z};
            up = CameraColumn(camera, 1);
            right = CameraColumn(camera, 2);
        }
        ToWorld(w, &dir);
        ToWorld(w, &up);
        ToWorld(w, &right);
        float length = crt::Sqrtf(up.y * up.y + right.y * right.y);
        if (length > 1e-06f) {
            float inverse = 1.0f / length;
            float c = up.y * inverse;
            float s = -right.y * inverse;
            m[0][0] = right.x * c + up.x * s;
            m[1][0] = s * up.y + c * right.y;
            m[2][0] = up.z * s + right.z * c;
            m[0][1] = up.x * c - right.x * s;
            m[1][1] = up.y * c - right.y * s;
            m[2][1] = c * up.z - s * right.z;
        } else {
            m[0][0] = -right.x;
            m[1][0] = -right.y;
            m[2][0] = -right.z;
            m[0][1] = -up.x;
            m[1][1] = -up.y;
            m[2][1] = -up.z;
        }
        m[0][2] = dir.x;
        m[1][2] = dir.y;
        m[2][2] = dir.z;
        break;
    }
    case 1: {
        NiPoint3 delta = {camera->world.translate.x - self->world.translate.x,
                          camera->world.translate.y - self->world.translate.y,
                          camera->world.translate.z - self->world.translate.z};
        float scale = self->world.scale;
        NiPoint3 local;
        const NiPoint3* r = Point3_TimesMatrix(&local, &delta, w);
        float inverse = 1.0f / scale;
        float x = r->x * inverse;
        float z = inverse * r->z;
        float length = crt::Sqrtf(z * z + x * x);
        if (!(length < 1e-12f)) {
            float inverseLength = 1.0f / length;
            x = x * inverseLength;
            z = inverseLength * z;
            m[0][0] = z;
            m[1][0] = 0.0f;
            m[2][0] = -x;
            m[0][1] = 0.0f;
            m[1][1] = 1.0f;
            m[2][1] = 0.0f;
            m[1][2] = 0.0f;
            m[0][2] = x;
            m[2][2] = z;
        }
        break;
    }
    case 2:
    case 4: {
        if (mode == 4) {
            if (!Billboard_FaceCenter(self, camera, &self->world.translate, &dir, &up, &right)) {
                break;
            }
        } else {
            NiPoint3 column0 = CameraColumn(camera, 0);
            dir = {-column0.x, -column0.y, -column0.z};
            up = CameraColumn(camera, 1);
            right = CameraColumn(camera, 2);
        }
        ToWorld(w, &dir);
        ToWorld(w, &up);
        ToWorld(w, &right);
        m[0][0] = right.x;
        m[1][0] = right.y;
        m[2][0] = right.z;
        m[0][1] = up.x;
        m[1][1] = up.y;
        m[2][1] = up.z;
        m[0][2] = dir.x;
        m[1][2] = dir.y;
        m[2][2] = dir.z;
        break;
    }
    default:
        break;
    }
    NiMatrix3 product;
    *w = *Matrix3_Multiply(w, &product, &rotate);
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 25, self->lastTime, int((self->flags >> 3) & 1));
        }
    }
}

// slot 32 OnVisible: turned to the culler's camera, then NiNode's
void __fastcall BillboardOnVisible(NiBillboardNode* self, re::Edx, void* culler) {
    Billboard_RotateToCamera(self, At<void*>(culler, 0x10));
    Node_OnVisible(self, culler);
}

// slot 31 UpdateWorldBound: with children, the bound's center moved to the world translation and
// its radius grown by the distance it moved
void __fastcall BillboardUpdateWorldBound(NiBillboardNode* self, re::Edx) {
    if (self->children.esize == 0) {
        return;
    }
    NiPoint3 d = {self->worldBound.center.x - self->world.translate.x,
                  self->worldBound.center.y - self->world.translate.y,
                  self->worldBound.center.z - self->world.translate.z};
    float distance = crt::Sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
    self->worldBound.center = self->world.translate;
    self->worldBound.radius = distance + self->worldBound.radius;
}

inline NiBillboardNode* NewBillboardNode() {
    NiBillboardNode* node = static_cast<NiBillboardNode*>(NiNew(0xFC));
    if (node == nullptr) {
        return nullptr;
    }
    Node_Construct(node, 0);
    node->lastTime = 0.0f;
    node->vtable = reinterpret_cast<void*>(NiBillboardNode_vtable);
    node->flags = 9;
    return node;
}

// vf3 CreateClone (the clone isn't checked, as in the game)
NiBillboardNode* __fastcall BillboardCreateClone(NiBillboardNode* self, re::Edx, void* cloning) {
    NiBillboardNode* clone = NewBillboardNode();
    Node_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    return clone;
}

NiBillboardNode* __cdecl BillboardCreateObject() {
    return NewBillboardNode();
}

// the flags (the controllers' update flag set)
void __fastcall BillboardLoadBinary(NiBillboardNode* self, re::Edx, void* stream) {
    Node_LoadBinary(self, stream);
    LoadBytes(stream, &self->flags, 2);
    self->flags |= 8;
}

void __fastcall BillboardSaveBinary(NiBillboardNode* self, re::Edx, void* stream) {
    Node_SaveBinary(self, stream);
    SaveBytes(stream, &self->flags, 2);
}

// "<prefix> = <mode>" (a mode past 4 leaves the new buffer as it is)
char* __cdecl BillboardGetViewerString(const char* prefix, uint32_t mode) {
    size_t size = std::strlen(prefix) + 0x16;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    switch (mode) {
    case 0:
        NiSprintf(text, size, "%s = ALWAYS_FACE_CAMERA", prefix);
        break;
    case 1:
        NiSprintf(text, size, "%s = ROTATE_ABOUT_UP", prefix);
        break;
    case 2:
        NiSprintf(text, size, "%s = RIGID_FACE_CAMERA", prefix);
        break;
    case 3:
        NiSprintf(text, size, "%s = ALWAYS_FACE_CENTER", prefix);
        break;
    case 4:
        NiSprintf(text, size, "%s = RIGID_FACE_CENTER", prefix);
        break;
    default:
        break;
    }
    return text;
}

void __fastcall BillboardGetViewerStrings(NiBillboardNode* self, re::Edx, NiTArray<char*>* strings) {
    Node_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBillboardNode_RTTI.name));
    AddViewerString(strings, Billboard_GetViewerString("m_eMode", uint8_t(self->flags) & 7));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISWITCH_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

RE_UNHOOKED("NiSortAdjustNode::ProcessClone (jump)", 0x007DAE70);   // -> 0x760510 NiNode::ProcessClone

NISWITCH_REVERSED("NiSwitchNode::CopyMembers", 0x007D93A0, ni::SwitchCopyMembers);
NISWITCH_REVERSED("NiSwitchNode::LoadBinary", 0x007D99E0, ni::SwitchLoadBinary);
NISWITCH_REVERSED("NiSwitchNode::NiSwitchNode", 0x007D9AC0, ni::SwitchConstruct);
NISWITCH_REVERSED("NiSwitchNode::GetRTTI", 0x007D9B50, ni::SwitchGetRTTI);
NISWITCH_REVERSED("NiSwitchNode::DeletingDestructor", 0x007D9B60, ni::SwitchDeletingDestructor);
NISWITCH_REVERSED("NiSwitchNode::CreateObject", 0x007D9CB0, ni::SwitchCreateObject);
NISWITCH_REVERSED("NiSortAdjustNode::CreateObject", 0x007DAE80, ni::SortAdjustCreateObject);
NISWITCH_REVERSED("NiSortAdjustNode::IsEqual", 0x007DAEC0, ni::SortAdjustIsEqual);
NISWITCH_REVERSED("NiSortAdjustNode::GetRTTI", 0x007DAEF0, ni::SortAdjustGetRTTI);
NISWITCH_REVERSED("NiSortAdjustNode::DeletingDestructor", 0x007DAF00, ni::SortAdjustDeletingDestructor);
NISWITCH_REVERSED("NiSortAdjustNode::OnVisible", 0x007DAF30, ni::SortAdjustOnVisible);
NISWITCH_REVERSED("NiSortAdjustNode::CreateClone", 0x007DAF90, ni::SortAdjustCreateClone);
NISWITCH_REVERSED("NiSortAdjustNode::LinkObject", 0x007DAFF0, ni::SortAdjustLinkObject);
NISWITCH_REVERSED("NiSortAdjustNode::LoadBinary", 0x007DB010, ni::SortAdjustLoadBinary);
NISWITCH_REVERSED("NiSortAdjustNode::SaveBinary", 0x007DB080, ni::SortAdjustSaveBinary);
NISWITCH_REVERSED("NiLODNode::LoadBinary", 0x007DDCE0, ni::LODLoadBinary);
NISWITCH_REVERSED("NiLODNode::UpdateWorldData", 0x007DDD00, ni::LODUpdateWorldData);
NISWITCH_REVERSED("NiLODNode::OnVisible", 0x007DDD20, ni::LODOnVisible);
NISWITCH_REVERSED("NiSwitchNode::UpdateChildren", 0x007DDDB0, ni::SwitchUpdateChildren);
NISWITCH_REVERSED("NiLODNode::LinkObject", 0x007DDE30, ni::LODLinkObject);
NISWITCH_REVERSED("NiLODNode::RegisterStreamables", 0x007DDE90, ni::LODRegisterStreamables);
NISWITCH_REVERSED("NiLODNode::SaveBinary", 0x007DDEC0, ni::LODSaveBinary);
NISWITCH_REVERSED("NiLODNode::SetLODData", 0x007DDEF0, ni::LODSetLODData);
NISWITCH_REVERSED("NiLODNode::~NiLODNode", 0x007DDF40, ni::LODDestruct);
NISWITCH_REVERSED("NiLODNode::GetRTTI", 0x007DDF90, ni::LODGetRTTI);
NISWITCH_REVERSED("NiLODNode::CopyMembers", 0x007DDFA0, ni::LODCopyMembers);
NISWITCH_REVERSED("NiLODNode::IsEqual", 0x007DE010, ni::LODIsEqual);
NISWITCH_REVERSED("NiLODNode::DeletingDestructor", 0x007DE080, ni::LODDeletingDestructor);
NISWITCH_REVERSED("NiLODNode::CreateClone", 0x007DE0B0, ni::LODCreateClone);
NISWITCH_REVERSED("NiLODNode::CreateObject", 0x007DE120, ni::LODCreateObject);
NISWITCH_REVERSED("NiLODNode::GetViewerStrings", 0x007DE170, ni::LODGetViewerStrings);
NISWITCH_REVERSED("NiBSPNode::LoadBinary", 0x007DE9E0, ni::BSPLoadBinary);
NISWITCH_REVERSED("NiBSPNode::SaveBinary", 0x007DEA00, ni::BSPSaveBinary);
NISWITCH_REVERSED("NiPlane::WhichSide", 0x007DEA20, ni::PlaneWhichSide);
NISWITCH_REVERSED("NiBSPNode::UpdateWorldPlane", 0x007DEA70, ni::BSPUpdateWorldPlane);
NISWITCH_REVERSED("NiBSPNode::UpdateWorldData", 0x007DEB00, ni::BSPUpdateWorldData);
NISWITCH_REVERSED("NiBSPNode::ApplyTransform", 0x007DEB10, ni::BSPApplyTransform);
NISWITCH_REVERSED("NiBSPNode::OnVisible", 0x007DED60, ni::BSPOnVisible);
NISWITCH_REVERSED("NiBSPNode::IsEqual", 0x007DEE00, ni::BSPIsEqual);
NISWITCH_REVERSED("NiBSPNode::GetRTTI", 0x007DEE40, ni::BSPGetRTTI);
NISWITCH_REVERSED("NiBSPNode::DeletingDestructor", 0x007DEE50, ni::BSPDeletingDestructor);
NISWITCH_REVERSED("NiBSPNode::CreateClone", 0x007DEE80, ni::BSPCreateClone);
NISWITCH_REVERSED("NiBSPNode::CreateObject", 0x007DEF20, ni::BSPCreateObject);
NISWITCH_REVERSED("NiBSPNode::GetViewerStrings", 0x007DEF80, ni::BSPGetViewerStrings);
NISWITCH_REVERSED("NiBillboardNode::UpdateSelectedDownwardPass", 0x007DF050,
                  ni::BillboardUpdateSelectedDownwardPass);
NISWITCH_REVERSED("NiBillboardNode::UpdateRigidDownwardPass", 0x007DF080, ni::BillboardUpdateRigidDownwardPass);
NISWITCH_REVERSED("NiBillboardNode::FaceCenter", 0x007DF0B0, ni::BillboardFaceCenter);
NISWITCH_REVERSED("NiBillboardNode::UpdateDownwardPass", 0x007DF2B0, ni::BillboardUpdateDownwardPass);
NISWITCH_REVERSED("NiBillboardNode::IsEqual", 0x007DF2E0, ni::BillboardIsEqual);
NISWITCH_REVERSED("NiBillboardNode::GetRTTI", 0x007DF310, ni::BillboardGetRTTI);
NISWITCH_REVERSED("NiBillboardNode::DeletingDestructor", 0x007DF320, ni::BillboardDeletingDestructor);
NISWITCH_REVERSED("NiBillboardNode::RotateToCamera", 0x007DF350, ni::BillboardRotateToCamera);
NISWITCH_REVERSED("NiBillboardNode::OnVisible", 0x007DF9F0, ni::BillboardOnVisible);
NISWITCH_REVERSED("NiBillboardNode::UpdateWorldBound", 0x007DFA10, ni::BillboardUpdateWorldBound);
NISWITCH_REVERSED("NiBillboardNode::CreateClone", 0x007DFAB0, ni::BillboardCreateClone);
NISWITCH_REVERSED("NiBillboardNode::CreateObject", 0x007DFB10, ni::BillboardCreateObject);
NISWITCH_REVERSED("NiBillboardNode::LoadBinary", 0x007DFB50, ni::BillboardLoadBinary);
NISWITCH_REVERSED("NiBillboardNode::SaveBinary", 0x007DFBA0, ni::BillboardSaveBinary);
NISWITCH_REVERSED("NiBillboardNode::GetViewerString", 0x007DFBE0, ni::BillboardGetViewerString);
NISWITCH_REVERSED("NiBillboardNode::GetViewerStrings", 0x007DFCA0, ni::BillboardGetViewerStrings);
