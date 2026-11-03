// NiRangeLODData (vtable 0x9612D0, 0x28 bytes): NiLODData (src/gamebryo/NiLODData.cpp) by
// distance: a model-space center (+0x8) and its world position (+0x14), and NiMalloc'd ranges
// (+0x24, a count at +0x20) of near and far distances in model space and in world space. The LOD
// level is the range the camera's distance along its view direction (times the camera's LOD
// adjustment) falls in. Main-thread maths: float, in the game's order.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
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
GAME_VAR(RTTI, NiRangeLODData_RTTI, 0x020E3DDC);
GAME_VAR(NiPoint3, NiPoint3_ZERO, 0x020E2D44);

constexpr uintptr_t NiRangeLODData_vtable = 0x009612D0;
constexpr float kMinFloat = -3.4028234663852886e+38f;   // 0x9009BC
constexpr float kMaxFloat = 3.4028234663852886e+38f;    // 0x9612C8

struct Range {
    float nearDist;        // +0x0
    float farDist;         // +0x4
    float worldNearDist;   // +0x8
    float worldFarDist;    // +0xC
};
static_assert(sizeof(Range) == 0x10);

struct NiRangeLODData : NiObject {
    NiPoint3 center;        // +0x8
    NiPoint3 worldCenter;   // +0x14
    uint32_t rangeCount;    // +0x20
    re::Ptr32<Range> ranges;          // +0x24
};
static_assert(offsetof(NiRangeLODData, rangeCount) == 0x20);
static_assert(sizeof(NiRangeLODData) == 0x28);

GAME_FN(void, __thiscall, LODData_Construct, 0x007F7920, void* self);
GAME_FN(void, __thiscall, LODData_Destruct, 0x007F7940, void* self);
GAME_FN(void, __thiscall, LODData_LoadBinary, 0x007F79A0, void* self, void* stream);
GAME_FN(void, __thiscall, LODData_SaveBinary, 0x00806A50, void* self, void* stream);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);
GAME_FN(NiPoint3*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, RangeLODData_SetNumRanges, 0x007DCB00, NiRangeLODData* self, uint32_t count);
GAME_FN(void, __thiscall, RangeLODData_CopyMembers, 0x007DCE00, NiRangeLODData* self, NiRangeLODData* dest,
        void* cloning);

// NiRangeLODData(): the center at the origin, no ranges
inline NiRangeLODData* NewRangeLODData() {
    NiRangeLODData* object = static_cast<NiRangeLODData*>(NiNew(0x28));
    if (object == nullptr) {
        return nullptr;
    }
    LODData_Construct(object);
    object->vtable = reinterpret_cast<void*>(NiRangeLODData_vtable);
    object->center = NiPoint3_ZERO;
    object->rangeCount = 0;
    object->ranges = nullptr;
    return object;
}

// vf8: the same ranges (model space; the class and the center aren't compared)
uint8_t __fastcall IsEqual(NiRangeLODData* self, re::Edx, NiRangeLODData* other) {
    uint32_t count = self->rangeCount;
    if (count != other->rangeCount) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (!(other->ranges[i].nearDist == self->ranges[i].nearDist) ||
            !(other->ranges[i].farDist == self->ranges[i].farDist)) {
            return 0;
        }
    }
    return 1;
}

const RTTI* __fastcall GetRTTI(void*, re::Edx) {
    return &NiRangeLODData_RTTI;
}

NiRangeLODData* __fastcall DeletingDestructor(NiRangeLODData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiRangeLODData_vtable);
    NiFree(self->ranges);
    LODData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x28);
    }
    return self;
}

// slot 19 UpdateWorldData(node): the center through the node's world transform, the ranges scaled
void __fastcall UpdateWorldData(NiRangeLODData* self, re::Edx, NiAVObject* node) {
    float scale = node->world.scale;
    NiPoint3 scaled;
    scaled.x = scale * self->center.x;
    scaled.y = self->center.y * scale;
    scaled.z = scale * self->center.z;
    NiPoint3 rotated;
    const NiPoint3* p = Matrix3_TimesPoint(&node->world.rotate, &rotated, &scaled);
    NiPoint3 world = {node->world.translate.x + p->x, node->world.translate.y + p->y, node->world.translate.z + p->z};
    self->worldCenter = world;
    for (uint32_t i = 0; i < self->rangeCount; i++) {
        self->ranges[i].worldNearDist = node->world.scale * self->ranges[i].nearDist;
        self->ranges[i].worldFarDist = self->ranges[i].farDist * node->world.scale;
    }
}

// SetNumRanges(count): reallocated (the ranges kept as far as they go, memcpy_s), freed for none
void __fastcall SetNumRanges(NiRangeLODData* self, re::Edx, uint32_t count) {
    if (count == self->rangeCount) {
        return;
    }
    if (count == 0) {
        NiFree(self->ranges);
        self->ranges = nullptr;
        self->rangeCount = 0;
        return;
    }
    Range* ranges = static_cast<Range*>(NiMalloc(count << 4));
    if (self->ranges != nullptr) {
        int32_t keep = int32_t(self->rangeCount);
        if (keep >= int32_t(count)) {
            keep = int32_t(count);
        }
        crt::MemcpyS(ranges, uint32_t(keep) << 4, self->ranges, uint32_t(keep) << 4);
    }
    NiFree(self->ranges);
    self->ranges = ranges;
    self->rangeCount = count;
}

// slot 18 GetLODLevel(camera, node): the range whose world near..far (near included) holds the
// camera's distance to the center along its view direction, times its LOD adjustment; -1 for none
int32_t __fastcall GetLODLevel(NiRangeLODData* self, re::Edx, NiAVObject* camera, void*) {
    NiPoint3 d = {self->worldCenter.x - camera->world.translate.x, self->worldCenter.y - camera->world.translate.y,
                  self->worldCenter.z - camera->world.translate.z};
    NiPoint3 direction = {camera->world.rotate.m[0][0], camera->world.rotate.m[1][0], camera->world.rotate.m[2][0]};
    float distance = direction.y * d.y + direction.x * d.x + direction.z * d.z;
    distance = distance * At<float>(camera, 0x138);
    distance = distance < 0.0f ? -distance : distance;
    for (uint32_t i = 0; i < self->rangeCount; i++) {
        if (self->ranges[i].worldNearDist <= distance && self->ranges[i].worldFarDist > distance) {
            return int32_t(i);
        }
    }
    return -1;
}

// slot 21 GetLODIndex(level): the level clamped to the ranges; then, once per level, the first
// range (in index order) whose near distance lies above the last one picked (as the game has it,
// the bound is moved as soon as a range is picked, so each pass takes the first range above the
// previous pick, not the nearest)
int32_t __fastcall GetLODIndex(NiRangeLODData* self, re::Edx, int32_t level) {
    if (level <= 0) {
        level = 0;
    }
    int32_t count = int32_t(self->rangeCount);
    if (level >= count - 1) {
        level = count - 1;
    }
    float previous = kMinFloat;
    int32_t result = -1;
    if (level < 0) {
        return -1;
    }
    for (int32_t pass = level + 1; pass != 0; pass--) {
        float best = kMaxFloat;
        for (int32_t i = 0; i < int32_t(self->rangeCount); i++) {
            float nearDist = self->ranges[i].nearDist;
            if (previous < nearDist && nearDist <= best) {
                previous = self->ranges[i].nearDist;
                best = self->ranges[i].nearDist;
                result = i;
            }
        }
    }
    return result;
}

// CopyMembers(dest, cloning): both centers and the ranges (not NiObject's)
void __fastcall CopyMembers(NiRangeLODData* self, re::Edx, NiRangeLODData* dest, void*) {
    dest->center = self->center;
    dest->worldCenter = self->worldCenter;
    RangeLODData_SetNumRanges(dest, self->rangeCount);
    uint32_t bytes = self->rangeCount << 4;
    crt::MemcpyS(dest->ranges, bytes, self->ranges, bytes);
}

NiRangeLODData* __cdecl CreateObject() {
    return NewRangeLODData();
}

// slot 20 Duplicate: the center and the ranges (the copy isn't checked, as in the game)
NiRangeLODData* __fastcall Duplicate(NiRangeLODData* self, re::Edx) {
    NiRangeLODData* copy = NewRangeLODData();
    copy->center = self->center;
    RangeLODData_SetNumRanges(copy, self->rangeCount);
    uint32_t bytes = self->rangeCount << 4;
    crt::MemcpyS(copy->ranges, bytes, self->ranges, bytes);
    return copy;
}

// vf3 CreateClone (copied into a null clone when the allocation fails, as the game has it)
NiRangeLODData* __fastcall CreateClone(NiRangeLODData* self, re::Edx, void* cloning) {
    NiRangeLODData* clone = NewRangeLODData();
    RangeLODData_CopyMembers(self, clone, cloning);
    return clone;
}

// the center, the count, each range's near and far
void __fastcall LoadBinary(NiRangeLODData* self, re::Edx, void* stream) {
    LODData_LoadBinary(self, stream);
    Point3_LoadBinary(&self->center, stream);
    uint32_t count;
    LoadBytes(stream, &count, 4);
    RangeLODData_SetNumRanges(self, count);
    for (uint32_t i = 0; i < count; i++) {
        LoadBytes(stream, &self->ranges[i].nearDist, 4);
        LoadBytes(stream, &self->ranges[i].farDist, 4);
    }
}

void __fastcall SaveBinary(NiRangeLODData* self, re::Edx, void* stream) {
    LODData_SaveBinary(self, stream);
    Point3_SaveBinary(&self->center, stream);
    SaveBytes(stream, &self->rangeCount, 4);
    for (uint32_t i = 0; i < self->rangeCount; i++) {
        SaveBytes(stream, &self->ranges[i].nearDist, 4);
        SaveBytes(stream, &self->ranges[i].farDist, 4);
    }
}

// the class, both centers, each range (not NiObject's strings)
void __fastcall GetViewerStrings(NiRangeLODData* self, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, GetViewerString(NiRangeLODData_RTTI.name));
    AddViewerString(strings, Point3_GetViewerString(&self->center, "m_kCenter"));
    AddViewerString(strings, Point3_GetViewerString(&self->worldCenter, "m_kWorldCenter"));
    for (uint32_t i = 0; i < self->rangeCount; i++) {
        char* text = static_cast<char*>(NiMalloc(0x80));
        NiSprintf(text, 0x80, "range[%d] = %g   %g", i, double(self->ranges[i].nearDist),
                  double(self->ranges[i].farDist));
        AddViewerStringInline(strings, text);
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIRANGELOD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIRANGELOD_REVERSED("NiRangeLODData::IsEqual", 0x007DC980, ni::IsEqual);
NIRANGELOD_REVERSED("NiRangeLODData::GetRTTI", 0x007DC9F0, ni::GetRTTI);
NIRANGELOD_REVERSED("NiRangeLODData::DeletingDestructor", 0x007DCA00, ni::DeletingDestructor);
NIRANGELOD_REVERSED("NiRangeLODData::UpdateWorldData", 0x007DCA40, ni::UpdateWorldData);
NIRANGELOD_REVERSED("NiRangeLODData::SetNumRanges", 0x007DCB00, ni::SetNumRanges);
NIRANGELOD_REVERSED("NiRangeLODData::GetLODLevel", 0x007DCB70, ni::GetLODLevel);
NIRANGELOD_REVERSED("NiRangeLODData::GetLODIndex", 0x007DCC40, ni::GetLODIndex);
NIRANGELOD_REVERSED("NiRangeLODData::CopyMembers", 0x007DCE00, ni::CopyMembers);
NIRANGELOD_REVERSED("NiRangeLODData::CreateObject", 0x007DCE60, ni::CreateObject);
NIRANGELOD_REVERSED("NiRangeLODData::Duplicate", 0x007DCEB0, ni::Duplicate);
NIRANGELOD_REVERSED("NiRangeLODData::CreateClone", 0x007DCF40, ni::CreateClone);
NIRANGELOD_REVERSED("NiRangeLODData::LoadBinary", 0x007DCFC0, ni::LoadBinary);
NIRANGELOD_REVERSED("NiRangeLODData::SaveBinary", 0x007DD090, ni::SaveBinary);
NIRANGELOD_REVERSED("NiRangeLODData::GetViewerStrings", 0x007DD150, ni::GetViewerStrings);
