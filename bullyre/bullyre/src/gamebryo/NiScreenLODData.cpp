// NiScreenLODData (vtable 0x9611F8, 0x30 bytes): LOD by the screen area the bound covers. Its
// model bound (+0x8), world bound (+0x18) and the screen proportions per level (+0x2C, their
// count +0x28; the first level whose proportion the bound's screen size reaches). Slots: 18
// GetLODLevel, 19 UpdateWorldData, 20 Duplicate, 21 GetLODIndex; 6 is NiObject's
// RegisterStreamables (a lone jump).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCamera.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiScreenLODData_RTTI, 0x020E3D70);
GAME_VAR(NiPoint3, NiPoint3_ZERO, 0x020E2D44);
GAME_VAR(float, kMinimumDistance, 0x00900A88);   // 1e-6

constexpr uintptr_t kVtable = 0x009611F8;

struct NiScreenLODData : NiObject {
    NiBound bound;            // +0x8 (model space)
    NiBound worldBound;       // +0x18
    uint32_t proportionCount; // +0x28
    re::Ptr32<float> proportions;       // +0x2C
};
static_assert(offsetof(NiScreenLODData, worldBound) == 0x18);
static_assert(sizeof(NiScreenLODData) == 0x30);

GAME_FN(void, __thiscall, LODData_Construct, 0x007F7920, void* self);
GAME_FN(void, __thiscall, LODData_Destruct, 0x007F7940, void* self);
GAME_FN(void, __thiscall, LODData_LoadBinary, 0x007F79A0, void* self, void* stream);
GAME_FN(void, __thiscall, LODData_SaveBinary, 0x00806A50, void* self, void* stream);
GAME_FN(void, __thiscall, Node_UpdateBound_7DDDB0, 0x007DDDB0, NiNode* self);
GAME_FN(NiTransform*, __thiscall, Transform_Invert, 0x00770950, const NiTransform* self, NiTransform* result);
GAME_FN(void, __thiscall, Bound_LoadBinary, 0x007D23D0, NiBound* self, void* stream);
GAME_FN(void, __thiscall, Bound_SaveBinary, 0x007D2410, const NiBound* self, void* stream);
GAME_FN(uint8_t, __thiscall, Bound_Equals, 0x007D2840, const NiBound* self, const NiBound* other);
GAME_FN(void, __thiscall, Bound_Merge, 0x007D2E60, NiBound* self, const NiBound* other);
GAME_FN(void, __thiscall, Bound_Update, 0x007D2FD0, NiBound* self, const NiBound* bound, const NiTransform* xform);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// (this file's, called where modes apply)
GAME_FN(NiScreenLODData*, __thiscall, ScreenLODData_Construct, 0x007DC040, void* self);
GAME_FN(void, __thiscall, ScreenLODData_SetNumProportions, 0x007DC0E0, NiScreenLODData* self, uint32_t count);
GAME_FN(void, __thiscall, ScreenLODData_UpdateWorldBoundFromChildren, 0x007DC390, NiScreenLODData* self,
        NiNode* node);

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

// a copy: the model bound and the proportions (not the world bound)
inline NiScreenLODData* Copy(const NiScreenLODData* self) {
    void* memory = NiNew(0x30);
    NiScreenLODData* copy = memory != nullptr ? ScreenLODData_Construct(memory) : nullptr;
    copy->bound = self->bound;
    ScreenLODData_SetNumProportions(copy, self->proportionCount);
    uint32_t bytes = self->proportionCount * 4;
    crt::MemcpyS(copy->proportions, bytes, self->proportions, bytes);
    return copy;
}

// 0x7DC010 vf21 GetLODIndex(level): clamped to 0 .. the proportion count
int __fastcall GetLODIndexImpl(NiScreenLODData* self, re::Edx, int level) {
    if (level < 0) {
        return 0;
    }
    int count = int(self->proportionCount);
    return level < count ? level : count;
}

// 0x7DC030 vf2
const RTTI* __fastcall GetRTTIImpl(NiScreenLODData*, re::Edx) {
    return &NiScreenLODData_RTTI;
}

// 0x7DC040
NiScreenLODData* __fastcall ConstructImpl(NiScreenLODData* self, re::Edx) {
    LODData_Construct(self);
    self->vtable = reinterpret_cast<void*>(kVtable);
    self->bound.center = NiPoint3_ZERO;
    self->bound.radius = 0.0f;
    self->worldBound.center = NiPoint3_ZERO;
    self->worldBound.radius = 0.0f;
    self->proportionCount = 0;
    self->proportions = nullptr;
    return self;
}

// 0x7DC0A0 vf0
NiScreenLODData* __fastcall DeletingDestructorImpl(NiScreenLODData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kVtable);
    NiFree(self->proportions);
    LODData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x30);
    }
    return self;
}

// 0x7DC0E0 SetNumProportions(count): the array reallocated, what fits kept
void __fastcall SetNumProportionsImpl(NiScreenLODData* self, re::Edx, uint32_t count) {
    if (count == self->proportionCount) {
        return;
    }
    if (count == 0) {
        NiFree(self->proportions);
        self->proportions = nullptr;
        self->proportionCount = 0;
        return;
    }
    float* proportions = static_cast<float*>(NiMalloc(count * 4));
    if (self->proportions != nullptr) {
        int kept = int(self->proportionCount);
        if (!(kept < int(count))) {
            kept = int(count);
        }
        crt::MemcpyS(proportions, count * 4, self->proportions, uint32_t(kept) * 4);
    }
    NiFree(self->proportions);
    self->proportions = proportions;
    self->proportionCount = count;
}

// 0x7DC160 vf18 GetLODLevel(camera, unused): 0 inside the bound (or at the camera); else the
// first level whose proportion is at most the bound's projected size (its radius over its
// distance, or the radius for an orthographic camera, times the larger frustum scale and the
// camera's LOD adjustment)
uint32_t __fastcall GetLODLevelImpl(NiScreenLODData* self, re::Edx, NiCamera* camera, void*) {
    float dx = self->worldBound.center.x - camera->world.translate.x;
    float dy = self->worldBound.center.y - camera->world.translate.y;
    float dz = self->worldBound.center.z - camera->world.translate.z;
    float lengthSquared = dx * dx + dy * dy + dz * dz;
    float distance = crt::Sqrtf(lengthSquared);
    if (self->worldBound.radius > distance || !(kMinimumDistance < distance)) {
        return 0;
    }
    float ratio = camera->frustum.ortho != 0 ? self->worldBound.radius : self->worldBound.radius / distance;
    float scaleX = 2.0f / (camera->frustum.right - camera->frustum.left);
    float scaleY = 2.0f / (camera->frustum.top - camera->frustum.bottom);
    float scale = scaleY < scaleX ? scaleX : scaleY;
    float size = scale * ratio * camera->lodAdjust;
    uint32_t level = 0;
    for (; level < self->proportionCount; level++) {
        if (self->proportions[level] <= size) {
            break;
        }
    }
    return level;
}

// 0x7DC2C0 vf8 IsEqual(other): the bounds and the proportions (not NiObject's)
uint8_t __fastcall IsEqualImpl(NiScreenLODData* self, re::Edx, NiScreenLODData* other) {
    if (Bound_Equals(&self->bound, &other->bound) == 0 || Bound_Equals(&self->worldBound, &other->worldBound) == 0) {
        return 0;
    }
    uint32_t count = self->proportionCount;
    if (count != other->proportionCount) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (!(self->proportions[i] == other->proportions[i])) {
            return 0;
        }
    }
    return 1;
}

// 0x7DC330 vf20 Duplicate
NiScreenLODData* __fastcall DuplicateImpl(NiScreenLODData* self, re::Edx) {
    return Copy(self);
}

// 0x7DC390 the world bound merged from the node's children, the model bound from it (through
// the node's inverse world transform)
void __fastcall UpdateWorldBoundFromChildrenImpl(NiScreenLODData* self, re::Edx, NiNode* node) {
    Node_UpdateBound_7DDDB0(node);
    self->worldBound.radius = 0.0f;
    for (int i = 0; int(node->children.size) > i; i++) {
        if (uint32_t(i) < node->children.size) {
            NiAVObject* child = node->children.base[i];
            if (child != nullptr) {
                Bound_Merge(&self->worldBound, &child->worldBound);
            }
        }
    }
    NiTransform inverse;
    Transform_Invert(&node->world, &inverse);
    Bound_Update(&self->bound, &self->worldBound, &inverse);
}

// 0x7DC410 vf3 CreateClone(cloning)
NiScreenLODData* __fastcall CreateCloneImpl(NiScreenLODData* self, re::Edx, void*) {
    return Copy(self);
}

// 0x7DC480 vf4 LoadBinary(stream)
void __fastcall LoadBinaryImpl(NiScreenLODData* self, re::Edx, void* stream) {
    LODData_LoadBinary(self, stream);
    Bound_LoadBinary(&self->bound, stream);
    Bound_LoadBinary(&self->worldBound, stream);
    uint32_t count;
    LoadRaw(stream, &count, 4);
    ScreenLODData_SetNumProportions(self, count);
    for (uint32_t i = 0; i < count; i++) {
        LoadRaw(stream, &self->proportions[i], 4);
    }
}

// 0x7DC520 vf7 SaveBinary(stream)
void __fastcall SaveBinaryImpl(NiScreenLODData* self, re::Edx, void* stream) {
    LODData_SaveBinary(self, stream);
    Bound_SaveBinary(&self->bound, stream);
    Bound_SaveBinary(&self->worldBound, stream);
    SaveRaw(stream, &self->proportionCount, 4);
    for (uint32_t i = 0; i < self->proportionCount; i++) {
        SaveRaw(stream, &self->proportions[i], 4);
    }
}

// 0x7DC5B0 vf19 UpdateWorldData(node): the world bound from the model bound, or (none yet)
// from the children
void __fastcall UpdateWorldDataImpl(NiScreenLODData* self, re::Edx, NiNode* node) {
    if (0.0f == self->bound.radius) {
        ScreenLODData_UpdateWorldBoundFromChildren(self, node);
        return;
    }
    Bound_Update(&self->worldBound, &self->bound, &node->world);
}

// 0x7DC5E0 vf9 GetViewerStrings (not NiObject's): the class, the bounds, the proportions
void __fastcall GetViewerStringsImpl(NiScreenLODData* self, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, GetViewerString(NiScreenLODData_RTTI.name));
    AddViewerString(strings, Bound_GetViewerString(&self->bound, "Bound"));
    AddViewerString(strings, Bound_GetViewerString(&self->worldBound, "World Bound"));
    for (uint32_t i = 0; i < self->proportionCount; i++) {
        char* text = static_cast<char*>(NiMalloc(0x80));
        NiSprintf(text, 0x80, "Proportion[%d] = %g", i, double(self->proportions[i]));
        AddViewerStringInline(strings, text);
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISCREENLOD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISCREENLOD_REVERSED("NiScreenLODData::GetLODIndex", 0x007DC010, ni::GetLODIndexImpl);
NISCREENLOD_REVERSED("NiScreenLODData::GetRTTI", 0x007DC030, ni::GetRTTIImpl);
NISCREENLOD_REVERSED("NiScreenLODData::NiScreenLODData", 0x007DC040, ni::ConstructImpl);
NISCREENLOD_REVERSED("NiScreenLODData::DeletingDestructor", 0x007DC0A0, ni::DeletingDestructorImpl);
NISCREENLOD_REVERSED("NiScreenLODData::SetNumProportions", 0x007DC0E0, ni::SetNumProportionsImpl);
NISCREENLOD_REVERSED("NiScreenLODData::GetLODLevel", 0x007DC160, ni::GetLODLevelImpl);
NISCREENLOD_REVERSED("NiScreenLODData::IsEqual", 0x007DC2C0, ni::IsEqualImpl);
NISCREENLOD_REVERSED("NiScreenLODData::Duplicate", 0x007DC330, ni::DuplicateImpl);
NISCREENLOD_REVERSED("NiScreenLODData::UpdateWorldBoundFromChildren", 0x007DC390,
                     ni::UpdateWorldBoundFromChildrenImpl);
NISCREENLOD_REVERSED("NiScreenLODData::CreateClone", 0x007DC410, ni::CreateCloneImpl);
NISCREENLOD_REVERSED("NiScreenLODData::LoadBinary", 0x007DC480, ni::LoadBinaryImpl);
NISCREENLOD_REVERSED("NiScreenLODData::SaveBinary", 0x007DC520, ni::SaveBinaryImpl);
NISCREENLOD_REVERSED("NiScreenLODData::UpdateWorldData", 0x007DC5B0, ni::UpdateWorldDataImpl);
NISCREENLOD_REVERSED("NiScreenLODData::GetViewerStrings", 0x007DC5E0, ni::GetViewerStringsImpl);
RE_UNHOOKED("NiScreenLODData::RegisterStreamables (jump)", 0x007DC970);   // -> 0x807310 -> NiObject's
