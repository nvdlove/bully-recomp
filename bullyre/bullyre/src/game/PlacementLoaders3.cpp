// More of the placement file reader's loader objects (see PlacementLoaders.cpp): the
// instance loaders (buildings, dummies, props, projectiles), the perimeters', points',
// POIs' and rails' records into their pools and linked lists, the section loader's
// constructor; and helpers of theirs (an entity's rotation from a record, its bounding
// rectangle, a trigger by name). Named by the game's RTTI and vtable slot; the objects
// are laid out by offset.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Placement.h"
#include "game/Pool.h"
#include "reversed.h"

namespace {

GAME_ARRAY(uint8_t*, pl3_modelInfoPtrs, 0x00C67738);
GAME_VAR(CPool<uint8_t>*, pl3_triggerPool, 0x020C7C48);
GAME_VAR(void*, pl3_sharedColModel, 0x00CF3120);
GAME_VAR(void*, pl3_currentPerimeter, 0x00BD5C58);
GAME_VAR(int32_t, pl3_currArea, 0x00BD1008);
GAME_VAR(uint8_t, pl3_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, pl3_chapterSeason, 0x00A147EC);   // by chapter: 0..3
GAME_VAR(uint8_t, pl3_occlusionOverride, 0x00BF3811);
GAME_VAR(uint8_t, pl3_occlusionOverrideValue, 0x00BF3817);
inline void* const pl3_propFlags = reinterpret_cast<void*>(0x00BCEB50);   // (the props' removed/kept flags)
inline void* const pl3_objectLimits = reinterpret_cast<void*>(0x00BA6C4C);
inline void* const pl3_soundBank = reinterpret_cast<void*>(0x00CF3DA0);
// the pools and linked lists (uint16 "next" per slot, 0xFFFE the end) of the records
GAME_VAR(CPool<uint8_t>*, pl3_perimeterPointPool, 0x020C7C90);
GAME_VAR(uint16_t*, pl3_perimeterPointLinks, 0x00BC5D20);
GAME_VAR(uint16_t, pl3_perimeterPointLast, 0x00BC5D24);
GAME_VAR(CPool<uint8_t>*, pl3_pointPool, 0x020C7C88);
GAME_VAR(uint16_t*, pl3_pointLinks, 0x00BC5D2C);
GAME_VAR(uint16_t, pl3_pointLast, 0x00BC5D30);
GAME_VAR(CPool<uint8_t>*, pl3_poiPointPool, 0x020C7C84);
GAME_VAR(uint16_t*, pl3_poiPointLinks, 0x00BC5D08);
GAME_VAR(uint16_t, pl3_poiPointLast, 0x00BC5D0C);
GAME_VAR(CPool<uint8_t>*, pl3_railNodePool, 0x020C7C94);
GAME_VAR(uint16_t*, pl3_railNodeLinks, 0x00BC5D38);
GAME_VAR(uint16_t, pl3_railNodeLast, 0x00BC5D3C);
GAME_VAR(CPool<uint8_t>*, pl3_railPool, 0x020C7C98);

GAME_FN(uint8_t, __thiscall, PL3_ObjectLimits_Allows, 0x006AE790, void* limits, uint32_t a, uint32_t b);
GAME_FN(float, __cdecl, PL3_ACos, 0x00413D60, float value);   // (clamped to -1..1)
GAME_FN(void, __thiscall, PL3_Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float angle);
GAME_FN(void, __thiscall, PL3_AllocateStaticMatrix, 0x0046DD40, uint8_t* entity);
GAME_FN(float*, __thiscall, PL3_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(void, __thiscall, PL3_Matrix_SetRotate, 0x00412E40, float* matrix, const float* quaternion);
GAME_FN(void, __cdecl, PL3_AddOcclusionVolume, 0x0051CE50, float x, float y, float z, float a, float b, float c, float d, uint32_t e, int);
GAME_FN(void*, __cdecl, PL3_Perimeter_New, 0x006D4CF0, uint32_t size);
GAME_FN(void*, __thiscall, PL3_Perimeter_Construct, 0x006D4B30, void* perimeter, int);
GAME_FN(void, __thiscall, PL3_Perimeter_SetName, 0x006D4B60, void* perimeter, const char* name);
GAME_FN(void*, __cdecl, PL3_Poi_New, 0x004583E0, uint32_t size);
GAME_FN(void*, __thiscall, PL3_Poi_Construct, 0x00447650, void* poi);
GAME_FN(uint32_t, __cdecl, PL3_Fn_488BB0, 0x00488BB0, const char*);
GAME_FN(uint32_t, __cdecl, PL3_HashName, 0x00576ED0, const char*);
GAME_FN(uint32_t, __cdecl, PL3_Fn_488B90, 0x00488B90, const char*);
GAME_FN(uint32_t, __cdecl, PL3_Fn_488B10, 0x00488B10, const char*);
GAME_FN(int32_t, __thiscall, PL3_Triggers_Find, 0x006D79C0, void* triggers, const char* name);
GAME_FN(float, __thiscall, PL3_AtomicModel_GetRadius, 0x00527420, uint8_t* mi);
GAME_FN(float, __thiscall, PL3_PAnimModel_GetRadius, 0x00520320, uint8_t* mi);
GAME_FN(void, __cdecl, PL3_TransformPoint, 0x0046D850, col::CVector* out, const float* transform, const col::CVector* point);
GAME_FN(void, __thiscall, PL3_Rect_ExtendPoint, 0x004144B0, col::CRect* rect, float x, float y);
GAME_FN(void, __thiscall, PL3_Rect_Extend, 0x00414410, col::CRect* rect, const col::CRect* other);
GAME_FN(uint8_t*, __thiscall, PL3_PAnimModel_GetSeasons, 0x005201D0, uint8_t* mi);
GAME_FN(uint8_t*, __cdecl, PL3_CreatePropForTrigger, 0x004EFFB0, void* trigger, int, int, int);
GAME_FN(uint32_t, __thiscall, PL3_PAnimModel_Fn520440, 0x00520440, uint8_t* mi);
GAME_FN(uint8_t*, __thiscall, PL3_PerimeterPointPool_New, 0x00450620, CPool<uint8_t>* pool);
GAME_FN(void*, __cdecl, PL3_Point_New, 0x006D5990, uint32_t size);
GAME_FN(uint8_t*, __thiscall, PL3_Point_Construct, 0x006D57E0, void* point);
GAME_FN(void, __thiscall, PL3_Point_SetName, 0x006D5840, uint8_t* point, const char* name);
GAME_FN(void, __thiscall, PL3_Point_Fn6D5880, 0x006D5880, uint8_t* point, const char* name);
GAME_FN(int32_t, __thiscall, PL3_Sound_FindName, 0x00595B80, void* bank, const char* name);
GAME_FN(uint16_t, __thiscall, PL3_Sound_GetIndex, 0x00595160, void* bank, int32_t id);
GAME_FN(void*, __cdecl, PL3_RailNode_New, 0x004F0350, uint32_t size);
GAME_FN(uint8_t*, __thiscall, PL3_RailNode_Construct, 0x004F0260, void* node, float x, float y, float z);
GAME_FN(void*, __cdecl, PL3_Rail_New, 0x004F0360, uint32_t size);
GAME_FN(uint8_t*, __thiscall, PL3_Rail_Construct, 0x004F02B0, void* rail, int32_t firstNode);
GAME_FN(void, __thiscall, PL3_SectionPaths_Construct, 0x005EC570, void* object);
// the instance loaders'
GAME_FN(void, __thiscall, PL3_SetColModel, 0x0050EB30, uint8_t* mi, void* col, int owns);
GAME_FN(uint16_t, __thiscall, PL3_PropFlags_Find, 0x004263C0, void* flags, uint32_t value);
GAME_FN(uint8_t, __thiscall, PL3_PropFlags_IsRemoved, 0x00426340, void* flags, int32_t index);
GAME_FN(uint8_t, __thiscall, PL3_PropFlags_IsKept, 0x00426350, void* flags, int32_t index);
GAME_FN(void*, __cdecl, PL3_PropAnim_New, 0x004EB180, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, PL3_PropAnim_Construct, 0x004EDB20, void* prop, int32_t model, void* trigger);
GAME_FN(void, __cdecl, PL3_SetRotationFromRecord, 0x00435C70, uint8_t* record, uint8_t* entity);
GAME_FN(void, __thiscall, PL3_SetPosition, 0x0041C4E0, uint8_t* entity, float x, float y, float z);
GAME_FN(col::CRect*, __cdecl, PL3_ColStore_GetBoundingBox, 0x0056F0F0, int slot);
GAME_FN(void, __cdecl, PL3_Fn_5B2990, 0x005B2990, uint8_t* prop);
GAME_FN(void, __thiscall, PL3_PropAnim_Fn4EF400, 0x004EF400, uint8_t* prop, int32_t model, void* trigger);
GAME_FN(void, __cdecl, PL3_Fn_5EB060, 0x005EB060, uint8_t* prop, int);
GAME_FN(void*, __cdecl, PL3_Building_New, 0x004CED70, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, PL3_Building_Construct, 0x004CECB0, void* building);
GAME_FN(uint8_t, __cdecl, PL3_Fn_43F3A0, 0x0043F3A0, int32_t model);
GAME_FN(void, __thiscall, PL3_SetupBigBuilding, 0x00467BC0, uint8_t* entity);
GAME_FN(void, __thiscall, PL3_Fn_466260, 0x00466260, uint8_t* entity);
GAME_FN(void*, __cdecl, PL3_Dummy_New, 0x004653A0, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, PL3_DummyObject_Construct, 0x004CF200, void* dummy);
GAME_FN(uint8_t*, __cdecl, PL3_GetColModel, 0x0051AF50, uint8_t* entity);
GAME_FN(void, __cdecl, PL3_World_Add, 0x0045D6B0, uint8_t* entity);
GAME_FN(void, __cdecl, PL3_IncludeEntity, 0x004384B0, int32_t slot, uint8_t* entity);
GAME_FN(col::CRect*, __cdecl, PL3_GetEntityBoundRect, 0x00436C40, col::CRect* rect, uint8_t* entity);
struct Vector16 {
    float x, y, z;
    uint32_t pad;   // (the game passes uninitialised stack here)
};
GAME_FN(uint8_t*, __cdecl, PL3_CreateProjectile, 0x004D4AF0, int32_t model, Vector16 pos, Vector16 rotation);

using re::At;

template <typename R = void, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    re::Ptr32<void>* vtable = *static_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[offset / 4]))(object, args...);
}

// (inlined: a slot's index in its pool; the game divides whatever pointer it has)
int32_t PoolIndex(CPool<uint8_t>* pool, const void* object) {
    return int32_t((reinterpret_cast<intptr_t>(object) - re::PtrCast<intptr_t>(pool->objects)) / pool->elemSize);
}

// (inlined: a new slot at the end of a record list)
void LinkLast(uint16_t* links, uint16_t& last, int32_t index) {
    links[last] = uint16_t(index);
    links[index] = 0xFFFE;
    last = uint16_t(index);
}

// (inlined: an entity's position and heading from its matrix, as CSimpleTransform)
void TransformFromMatrix(const float* m, float* out) {
    out[0] = m[0xC];
    out[1] = m[0xD];
    out[2] = m[0xE];
    out[3] = float(std::atan2(double(-m[4]), double(m[5])));
}

// (inlined: the trigger pool's first slot, nullptr when free)
void* FirstTrigger() {
    CPool<uint8_t>* pool = pl3_triggerPool;
    return (pool->flags[0] & 0x80) ? nullptr : pool->objects;
}

} // namespace

namespace IplLoaders3 {

// 0x4359D0 ObjectInstanceLoaderImpl vf6: the record through vf5 if the object limits
// (0xBA6C4C) allow its +0x7C/+0x78
static void __fastcall Instance_Filter(void* self, re::Edx, uint8_t* record) {
    if (PL3_ObjectLimits_Allows(pl3_objectLimits, At<uint32_t>(record, 0x7C), At<uint32_t>(record, 0x78))) {
        VCall(self, 0x14, record);
    }
}

// 0x435A00 PoiLoaderImpl vf3: nothing (a lone "ret 8")
// 0x435A00 (vf3, thiscall, two arguments): empty (ret 8)
void __fastcall PoiLoader_vf3(void*, re::Edx, uint32_t, uint32_t) {}
RE_UNHOOKED_FN("PoiLoaderImpl::vf3", 0x00435A00, &PoiLoader_vf3);

// 0x435C20 (thiscall on the result): a record's position (+0x30) and its heading from
// the record's +0x10/+0x14 (a CSimpleTransform)
static float* __fastcall Record_GetTransform(float* out, re::Edx, uint8_t* record) {
    float y = At<float>(record, 0x34);
    float z = At<float>(record, 0x38);
    out[0] = At<float>(record, 0x30);
    out[1] = y;
    out[2] = z;
    out[3] = float(std::atan2(double(-At<float>(record, 0x10)), double(At<float>(record, 0x14))));
    return out;
}

// 0x435C70: an entity's rotation from its record's quaternion (+0x60): about z only (no
// matrix needed) when x and y are within 0.005, else a static matrix from it
static void __cdecl SetRotationFromRecord(uint8_t* record, uint8_t* entity) {
    float qx = std::fabs(At<float>(record, 0x60));
    if (!(0.005f < qx) && !(0.005f < std::fabs(At<float>(record, 0x64)))) {
        float angle;
        if (!(At<float>(record, 0x68) < 0.0f)) {  // (NaN too)
            angle = float(PL3_ACos(At<float>(record, 0x6C)) * -2.0);
        } else {
            float a = PL3_ACos(At<float>(record, 0x6C));
            angle = a + a;
        }
        if (void* matrix = At<void*>(entity, 0x14)) {
            PL3_Matrix_SetRotateZOnly(matrix, angle);
        }
        At<float>(entity, 0x10) = angle;
        return;
    }
    float quaternion[4] = {-At<float>(record, 0x60), -At<float>(record, 0x64), -At<float>(record, 0x68), At<float>(record, 0x6C)};
    PL3_AllocateStaticMatrix(entity);
    PL3_Matrix_SetRotate(PL3_GetMatrix(entity), quaternion);
}

// 0x435D90 OcclusionVolumeLoaderImpl vf2: the volume added (its centre +4, raised by half
// its height), when on (+0x14, or 0xBF3817 while 0xBF3811 overrides)
static void __fastcall Occlusion_Add(uint8_t* self, re::Edx, float a, float b, float c, float d) {
    uint8_t on = pl3_occlusionOverride ? pl3_occlusionOverrideValue : self[0x14];
    if (!on) {
        return;
    }
    float z = float(c * 0.5 + At<float>(self, 0xC));
    At<float>(self, 0xC) = z;
    PL3_AddOcclusionVolume(At<float>(self, 4), At<float>(self, 8), z, a, b, c, d, At<uint32_t>(self, 0x10), 0);
}

// 0x435E10 PerimeterLoaderImpl vf0: a new perimeter (+0x60), named, two values (+8,
// +0xC), the file's area (+0x1C); the triggers after it get it (0xBD5C58)
static void __fastcall Perimeter_Begin(uint8_t* self, re::Edx, const char* name, int, int, float a, float b, int) {
    void* memory = PL3_Perimeter_New(0x30);
    uint8_t* perimeter = memory ? static_cast<uint8_t*>(PL3_Perimeter_Construct(memory, -2)) : nullptr;
    At<uint8_t*>(self, 0x60) = perimeter;
    PL3_Perimeter_SetName(perimeter, name);
    At<float>(At<uint8_t*>(self, 0x60), 8) = a;
    At<float>(At<uint8_t*>(self, 0x60), 0xC) = b;
    At<uint8_t*>(self, 0x60)[0x1C] = self[4];
    pl3_currentPerimeter = At<uint8_t*>(self, 0x60);
}

// 0x4361F0 PoiPointsLoaderImpl vf0: a new POI point (+4, 0xA4 bytes), the file's area
// (+0xA0), its four names hashed or looked up; +0x81 the flag for kinds 0..3 (else 0)
static void __fastcall PoiPoints_Begin(uint8_t* self, re::Edx, const char* s1, int, const char* s2, int, const char* s3, int, const char* s4, int,
                                       int flag) {
    uint8_t* point = static_cast<uint8_t*>(PL3_Poi_New(0xA4));
    if (point) {
        PL3_Poi_Construct(point);
        At<uint32_t>(point, 0) = 0x00901858;
        point[0xA0] = 0xFF;
    }
    At<uint8_t*>(self, 4) = point;
    point[0xA0] = self[8];
    At<uint32_t>(At<uint8_t*>(self, 4), 0x74) = PL3_Fn_488BB0(s1);
    At<uint32_t>(At<uint8_t*>(self, 4), 0x84) = PL3_HashName(s2);
    At<uint32_t>(At<uint8_t*>(self, 4), 0x70) = PL3_Fn_488B90(s3);
    At<uint32_t>(At<uint8_t*>(self, 4), 0x78) = PL3_Fn_488B10(s4);
    point = At<uint8_t*>(self, 4);
    int32_t kind = At<int32_t>(point, 0x70);
    if (kind == 0 || kind == 2 || kind == 1 || kind == 3) {
        point[0x81] = uint8_t(flag);
    } else {
        point[0x81] = 0;
    }
}

// 0x4362C0 PoiPointsLoaderImpl vf2: the heading in degrees (the point's +0x10 and +0x68;
// its matrix too if it has one)
static void __fastcall PoiPoints_SetHeading(uint8_t* self, re::Edx, float degrees, float, float) {
    // (the x87 at 24-bit precision: each step rounded to float; the double pi is float pi)
    float radians = degrees * 3.14159274f / 180.0f;
    uint8_t* point = At<uint8_t*>(self, 4);
    if (void* matrix = At<void*>(point, 0x14)) {
        PL3_Matrix_SetRotateZOnly(matrix, radians);
    }
    At<float>(point, 0x10) = radians;
    At<float>(At<uint8_t*>(self, 4), 0x68) = radians;
}

// 0x436C10 (thiscall on TheTriggers, unused): a trigger by name (nullptr: none)
static void* __fastcall FindTriggerByName(void* triggers, re::Edx, const char* name) {
    int32_t index = PL3_Triggers_Find(triggers, name);
    if (index == -1) {
        return nullptr;
    }
    CPool<uint8_t>* pool = pl3_triggerPool;
    if (pool->flags[index] & 0x80) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * index;
}

// 0x436C40: an entity's bounding rectangle, from a cube of its model's radius (50 unless
// an atomic, time or prop anim model) about its position and heading (four corners)
static col::CRect* __cdecl GetEntityBoundRect(col::CRect* rect, uint8_t* entity) {
    rect->left = 1000000.0f;
    rect->bottom = 1000000.0f;
    rect->right = -1000000.0f;
    rect->top = -1000000.0f;
    uint8_t* mi = pl3_modelInfoPtrs[At<int16_t>(entity, 0x10E)];
    float radius = 50.0f;
    if (mi[8] == 1 || mi[8] == 3) {
        radius = PL3_AtomicModel_GetRadius(mi);
    } else if (mi[8] == 0xA) {
        radius = PL3_PAnimModel_GetRadius(mi);
    }
    col::CVector cornerA = {-radius, -radius, -radius};
    col::CVector cornerB = {radius, radius, radius};
    col::CVector point;
    float transform[4];
    auto extend = [&](const col::CVector* corner) {
        const float* xf = reinterpret_cast<const float*>(entity + 4);
        if (At<void*>(entity, 0x14)) {
            TransformFromMatrix(PL3_GetMatrix(entity), transform);
            xf = transform;
        }
        PL3_TransformPoint(&point, xf, corner);
        PL3_Rect_ExtendPoint(rect, point.x, point.y);
    };
    extend(&cornerA);
    extend(&cornerB);
    float x = cornerA.x;
    cornerA.x = cornerB.x;
    cornerB.x = x;
    extend(&cornerA);
    extend(&cornerB);
    return rect;
}

// 0x436F50 TriggerLoaderImpl vf8: the trigger's prop made now, if it's a prop trigger of
// this area whose model is for the chapter's season
static void __fastcall Trigger_End(uint8_t* self, re::Edx, int, int, int, int, int) {
    uint8_t* trigger = At<uint8_t*>(self, 0x1C);
    if (!trigger || (trigger[0x3F] & 1)) {
        return;
    }
    int32_t model = At<int32_t>(At<uint8_t*>(trigger, 0x20), 0x10);
    if (model == -1 || int8_t(trigger[0x3C]) != pl3_currArea) {
        return;
    }
    uint8_t* seasons = PL3_PAnimModel_GetSeasons(pl3_modelInfoPtrs[model]);
    int32_t season = pl3_chapterSeason[pl3_chapter];
    if ((seasons[0x74] && season == 0) || (seasons[0x75] && season == 1) || (seasons[0x77] && season == 3) || (seasons[0x76] && season == 2)) {
        PL3_CreatePropForTrigger(At<uint8_t*>(self, 0x1C), self[0x20], -1, 1);
    }
}

// 0x437120 PoiLoaderImpl vf2: the POI loader (+4)
static void* __fastcall Poi_GetPointLoader(uint8_t* self, re::Edx, int) {
    return self + 4;
}

// 0x437C90 (thiscall on an entity): its prop anim model's 0x520440 (a flag in al; the
// rest of eax the model's +0x70)
static uint32_t __fastcall Entity_ModelFn520440(uint8_t* entity, re::Edx) {
    return PL3_PAnimModel_Fn520440(pl3_modelInfoPtrs[At<int16_t>(entity, 0x10E)]);
}

// 0x437D20: a lone jmp to CIplStore::RemoveIpb (0x4377F0)
RE_UNHOOKED("CIplStore::RemoveIpb_thunk", 0x00437D20);

// 0x437D40 PerimeterLoaderImpl vf4: the perimeter's points (the coordinates at +0xC,
// count +8) into the point pool's list (the perimeter's +4 the first), its bounds
// (+0x20 min x, min y, max x, max y)
static void __fastcall Perimeter_End(uint8_t* self, re::Edx) {
    int32_t count = At<int32_t>(self, 8);
    if (count == 0) {
        return;
    }
    CPool<uint8_t>* pool = pl3_perimeterPointPool;
    float* node = reinterpret_cast<float*>(PL3_PerimeterPointPool_New(pool));
    int32_t index = PoolIndex(pool, node);
    pl3_perimeterPointLinks[index] = 0xFFFE;
    pl3_perimeterPointLast = uint16_t(index);
    At<uint16_t>(At<uint8_t*>(self, 0x60), 4) = uint16_t(index);
    node[0] = At<float>(self, 0xC);
    float y = At<float>(self, 0x10);
    node[1] = y;
    float minX = node[0], minY = y, maxX = node[0], maxY = y;
    const float* coords = reinterpret_cast<const float*>(self + 0x14);
    for (int32_t i = 2; i < At<int32_t>(self, 8); i += 2, coords += 2) {
        node = reinterpret_cast<float*>(PL3_PerimeterPointPool_New(pool));
        node[0] = coords[0];
        node[1] = coords[1];
        LinkLast(pl3_perimeterPointLinks, pl3_perimeterPointLast, PoolIndex(pool, node));
        if (minX > node[0]) {
            minX = node[0];
        }
        if (minY > node[1]) {
            minY = node[1];
        }
        if (maxX < node[0]) {
            maxX = node[0];
        }
        if (maxY < node[1]) {
            maxY = node[1];
        }
    }
    uint8_t* perimeter = At<uint8_t*>(self, 0x60);
    At<float>(perimeter, 0x20) = minX;
    At<float>(perimeter, 0x24) = minY;
    At<float>(perimeter, 0x28) = maxX;
    At<float>(perimeter, 0x2C) = maxY;
}

// 0x437EA0 PointLoaderImpl vf3: a point (0x2C) from the record: the kept position (+8)
// and the area (+0x20); a name, a sound, three bytes, another name; into the point
// pool's list (the loader's +4 told its index)
static void __fastcall Point_Add(uint8_t* self, re::Edx, float, const char*, int, const char* name, int, const char* sound, int, uint32_t v0,
                                 uint32_t v1, uint32_t v2, uint32_t, uint32_t, uint32_t, const char*, int, const char* name2, int) {
    void* memory = PL3_Point_New(0x2C);
    uint8_t* point = memory ? PL3_Point_Construct(memory) : nullptr;
    point[0x26] = self[0x20];
    At<uint32_t>(point, 0) = At<uint32_t>(self, 8);
    At<uint32_t>(point, 4) = At<uint32_t>(self, 0xC);
    At<uint32_t>(point, 8) = At<uint32_t>(self, 0x10);
    At<float>(point, 0x1C) = At<float>(self, 0x14);
    if (name[0]) {
        PL3_Point_SetName(point, name);
    } else {
        At<uint16_t>(point, 0x20) = 0;
    }
    uint16_t soundIndex = PL3_Sound_GetIndex(pl3_soundBank, PL3_Sound_FindName(pl3_soundBank, sound));
    point[0x24] = uint8_t(v0);
    At<uint16_t>(point, 0x14) = soundIndex;
    point[0x23] = uint8_t(v2);
    point[0x22] = uint8_t(v1);
    PL3_Point_Fn6D5880(point, name2);
    int32_t index = PoolIndex(pl3_pointPool, point);
    pl3_pointLinks[index] = 0xFFFE;
    pl3_pointLast = uint16_t(index);
    At<uint16_t>(At<uint8_t*>(self, 4), 4) = uint16_t(index);
}

// 0x437F90 PoiPointsLoaderImpl vf3: the POI point's flags (+0x88..+0x93, +0xA2, +0xA3:
// values not 0), a float (+0x9C) and a byte (+0xA1); into the POI point list (the POI's
// +0x16 the first, kept at +0x10)
static void __fastcall PoiPoints_SetFields(uint8_t* self, re::Edx, int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
                                           int a9, int a10, float a11, int a12, int a13) {
    uint8_t* p = At<uint8_t*>(self, 4);
    p[0x8E] = a0 != 0;
    p[0x92] = a1 != 0;
    p[0x90] = a2 != 0;
    p[0x91] = a3 != 0;
    p[0x93] = a4 != 0;
    p[0x8A] = a5 != 0;
    p[0x8F] = a6 != 0;
    p[0x8D] = a7 != 0;
    p[0x8B] = a8 != 0;
    p[0x8C] = a8 != 0;
    p[0x88] = a9 != 0;
    p[0x89] = a9 != 0;
    p[0xA2] = a10 != 0;
    At<float>(p, 0x9C) = a11;
    p[0xA3] = a12 != 0;
    p[0xA1] = uint8_t(a13);
    int32_t index = PoolIndex(pl3_poiPointPool, At<uint8_t*>(self, 4));
    if (At<int32_t>(self, 0x10) == 0xFFFF) {
        pl3_poiPointLinks[index] = 0xFFFE;
        pl3_poiPointLast = uint16_t(index);
        At<int32_t>(self, 0x10) = index;
        At<uint16_t>(At<uint8_t*>(self, 0xC), 0x16) = uint16_t(At<int32_t>(self, 0x10));
        return;
    }
    LinkLast(pl3_poiPointLinks, pl3_poiPointLast, index);
}

// 0x4380F0 RailLoaderImpl vf2: a rail node at the point, into the node list; the first
// makes the rail (+8), whose index + 1 the prop (+4) keeps (+0x245)
static void __fastcall Rail_AddPoint(uint8_t* self, re::Edx, int i, float x, float y, float z) {
    CPool<uint8_t>* pool = pl3_railNodePool;
    void* memory = PL3_RailNode_New(0x10);
    uint8_t* node = memory ? PL3_RailNode_Construct(memory, x, y, z) : nullptr;
    int32_t index = PoolIndex(pool, node);
    if (i != 0) {
        LinkLast(pl3_railNodeLinks, pl3_railNodeLast, index);
        return;
    }
    pl3_railNodeLinks[index] = 0xFFFE;
    pl3_railNodeLast = uint16_t(index);
    void* railMemory = PL3_Rail_New(0xC);
    uint8_t* rail = railMemory ? PL3_Rail_Construct(railMemory, index) : nullptr;
    At<uint8_t*>(self, 8) = rail;
    int32_t railIndex = PoolIndex(pl3_railPool, rail);
    At<uint8_t*>(self, 4)[0x245] = uint8_t(railIndex + 1);
}

// 0x438240 IplLoaderImpl::SectionLoaderImpl's constructor: the loader objects' vtables,
// their vectors zeroed, the path loader's (+0x11C) constructed
static uint8_t* __fastcall SectionLoader_Construct(uint8_t* self, re::Edx) {
    At<float>(self, 0x4C) = 0.0f;
    At<float>(self, 0x50) = 0.0f;
    At<uint32_t>(self, 0) = 0x00903684;
    At<float>(self, 0x54) = 0.0f;
    At<uint32_t>(self, 0x18) = 0x009034B0;
    At<float>(self, 0xCC) = 0.0f;
    At<uint32_t>(self, 0x30) = 0x009034D0;
    At<float>(self, 0xD0) = 0.0f;
    At<uint32_t>(self, 0x48) = 0x009034D8;
    At<float>(self, 0xD4) = 0.0f;
    At<uint32_t>(self, 0x60) = 0x009034E8;
    At<float>(self, 0xF0) = 0.0f;
    At<uint32_t>(self, 0xC4) = 0x00903500;
    At<float>(self, 0xF4) = 0.0f;
    At<uint32_t>(self, 0xE8) = 0x0090352C;
    At<float>(self, 0xF8) = 0.0f;
    At<uint32_t>(self, 0x10C) = 0x0090364C;
    At<float>(self, 0xFC) = 0.0f;
    At<float>(self, 0x100) = 0.0f;
    At<uint32_t>(self, 0x110) = 0x00903564;
    At<float>(self, 0x104) = 0.0f;
    PL3_SectionPaths_Construct(self + 0x11C);
    At<uint32_t>(self, 0x148) = 0x00903540;
    At<uint32_t>(self, 0x150) = 0x00903668;
    At<uint32_t>(self, 0x154) = 0x00903578;
    At<uint32_t>(self, 0x170) = 0x0090354C;
    return self;
}

// 0x4388E0 ObjectInstanceLoaderImpl vf5: an instance record's entity: a prop anim (for
// the trigger pool's first trigger, when it has flag 0x10), a building (models without
// +0x12, not in the prop flags) or a dummy (the others the prop flags keep). Normally
// into the world and the file's slot ranges; when only reading the bounds (+0x10), its
// rectangle grows the file's bounds (within -900..900, as far as checked) and it's
// deleted again
static void __fastcall Instance_Add(uint8_t* self, re::Edx, uint8_t* record) {
    int32_t model = At<int32_t>(record, 0);
    uint8_t* mi = pl3_modelInfoPtrs[model];
    At<int32_t>(self, 0xC) = int32_t(int64_t(At<float>(record, 0x44)));
    if (At<void*>(mi, 0xC) == nullptr && mi[8] != 0xA) {
        PL3_SetColModel(mi, pl3_sharedColModel, 0);
    }
    At<uint8_t*>(self, 8) = nullptr;
    int16_t flagIndex = int16_t(PL3_PropFlags_Find(pl3_propFlags, At<uint32_t>(record, 0x74)));
    if (mi[8] == 0xA) {
        void* trigger = FirstTrigger();
        if (!(static_cast<uint8_t*>(trigger)[0x3F] & 0x10)) {
            return;
        }
        void* memory = PL3_PropAnim_New(0x3B8);
        uint8_t* prop = memory ? PL3_PropAnim_Construct(memory, model, trigger) : nullptr;
        At<uint8_t*>(self, 8) = prop;
        prop[0x3A8] = 0;
        prop[0x111] = uint8_t(int32_t(At<float>(record, 0x44)));
        At<float>(prop, 0x11C) = At<float>(record, 0x70);
        VCall(prop, 0x24, At<int32_t>(record, 0));
        PL3_SetRotationFromRecord(record, At<uint8_t*>(self, 8));
        PL3_SetPosition(At<uint8_t*>(self, 8), At<float>(record, 0x48), At<float>(record, 0x4C), At<float>(record, 0x50));
        At<uint32_t>(prop, 0x3AC) = At<uint32_t>(record, 0x74);
        if (trigger != FirstTrigger()) {
            At<uint8_t*>(trigger, 0x1C) = prop;
        }
        uint8_t* recordMi = pl3_modelInfoPtrs[At<int32_t>(record, 0)];
        if (At<void*>(recordMi, 0xC) == nullptr) {
            PL3_SetColModel(recordMi, pl3_sharedColModel, 0);
        }
        int32_t propModel = At<int16_t>(prop, 0x10E);
        uint16_t colSlot = At<uint16_t>(At<uint8_t*>(pl3_modelInfoPtrs[propModel], 0xC), 0x30);
        col::CRect rect;
        col::CRect* bounds = VCall<col::CRect*>(prop, 0x2C, &rect);
        PL3_Rect_Extend(PL3_ColStore_GetBoundingBox(colSlot), bounds);
        PL3_Fn_5B2990(prop);
        PL3_PropAnim_Fn4EF400(prop, propModel, trigger);
        if (uint8_t(PL3_PAnimModel_Fn520440(pl3_modelInfoPtrs[At<int16_t>(prop, 0x10E)]))) {
            PL3_Fn_5EB060(prop, At<int32_t>(prop, 0x180) == 0);
        }
    } else if (At<int16_t>(mi, 0x12) == -1 && flagIndex < 0) {
        void* memory = PL3_Building_New(0x120);
        uint8_t* building = memory ? PL3_Building_Construct(memory) : nullptr;
        At<uint8_t*>(self, 8) = building;
        VCall(building, 0x24, At<int32_t>(record, 0));
        if (PL3_Fn_43F3A0(At<int16_t>(At<uint8_t*>(self, 8), 0x10E))) {
            At<int32_t>(At<uint8_t*>(self, 8), 0xC8) = 1;
        }
        At<uint8_t*>(self, 8)[0x111] = uint8_t(int32_t(At<float>(record, 0x44)));
        At<float>(At<uint8_t*>(self, 8), 0x11C) = At<float>(record, 0x70);
        if (mi[8] == 1 || mi[8] == 3) {
            if (mi[0x28] & 0x10) {
                PL3_SetupBigBuilding(At<uint8_t*>(self, 8));
            }
            if (At<uint32_t>(mi, 0x28) & 0x800) {
                At<int32_t>(At<uint8_t*>(self, 8), 0xA0) = 1;
            }
        }
        if (PL3_AtomicModel_GetRadius(mi) < 2.0f) {
            At<int32_t>(At<uint8_t*>(self, 8), 0x48) = 0;
        }
        if (At<void*>(mi, 0xC) == nullptr) {
            PL3_SetColModel(mi, pl3_sharedColModel, 0);
        }
        PL3_SetRotationFromRecord(record, At<uint8_t*>(self, 8));
        PL3_SetPosition(At<uint8_t*>(self, 8), At<float>(record, 0x48), At<float>(record, 0x4C), At<float>(record, 0x50));
        PL3_Fn_466260(At<uint8_t*>(self, 8));
    } else {
        if (flagIndex >= 0) {
            if (PL3_PropFlags_IsRemoved(pl3_propFlags, uint16_t(flagIndex))) {
                return;
            }
            if (!PL3_PropFlags_IsKept(pl3_propFlags, uint16_t(flagIndex))) {
                return;
            }
        }
        void* memory = PL3_Dummy_New(0x128);
        uint8_t* dummy = memory ? PL3_DummyObject_Construct(memory) : nullptr;
        At<uint8_t*>(self, 8) = dummy;
        VCall(dummy, 0x24, At<int32_t>(record, 0));
        PL3_SetRotationFromRecord(record, At<uint8_t*>(self, 8));
        PL3_SetPosition(At<uint8_t*>(self, 8), At<float>(record, 0x48), At<float>(record, 0x4C), At<float>(record, 0x50));
        if (At<void*>(mi, 0xC) == nullptr) {
            PL3_SetColModel(mi, pl3_sharedColModel, 0);
        }
        uint16_t colSlot = At<uint16_t>(PL3_GetColModel(At<uint8_t*>(self, 8)), 0x30);
        col::CRect rect;
        col::CRect* bounds = VCall<col::CRect*>(At<uint8_t*>(self, 8), 0x2C, &rect);
        PL3_Rect_Extend(PL3_ColStore_GetBoundingBox(colSlot), bounds);
        At<uint8_t*>(self, 8)[0x111] = uint8_t(int32_t(At<float>(record, 0x44)));
        At<float>(At<uint8_t*>(self, 8), 0x11C) = At<float>(record, 0x70);
        At<uint32_t>(dummy, 0x124) = At<uint32_t>(record, 0x74);
    }
    if (!self[0x10]) {
        At<uint8_t*>(self, 8)[0x110] = self[0x14];
        PL3_World_Add(At<uint8_t*>(self, 8));
        PL3_IncludeEntity(At<int32_t>(self, 0x14), At<uint8_t*>(self, 8));
        return;
    }
    col::CRect rect;
    PL3_GetEntityBoundRect(&rect, At<uint8_t*>(self, 8));
    // (right checked twice, left not at all)
    if (!(-900.0 > rect.right) && !(900.0 < rect.right) && !(rect.bottom < -900.0) && !(rect.top > 900.0)) {
        PL3_Rect_Extend(At<col::CRect*>(self, 4), &rect);
    }
    if (uint8_t* entity = At<uint8_t*>(self, 8)) {
        VCall(entity, 0, 1);
    }
    At<int32_t>(At<uint8_t*>(self, 4), 0x34) = int32_t(At<float>(record, 0x44));
}

// 0x438DF0 ProjectileInstanceLoaderImpl vf0: a projectile at the record's position with
// its rotation (degrees: +0x64, +0x68, +0x60 less 90), into the file's slot ranges
// (not when only reading the bounds)
static void __fastcall Projectile_Add(uint8_t* self, re::Edx, uint8_t* record) {
    if (self[0x10]) {
        return;
    }
    const double pi = 3.1415927410125732;
    Vector16 rotation;
    rotation.x = float(pi * At<float>(record, 0x64) / 180.0);
    rotation.y = float(At<float>(record, 0x68) * pi / 180.0);
    rotation.z = float((At<float>(record, 0x60) - 90.0) * pi / 180.0);
    rotation.pad = 0;
    Vector16 pos = {At<float>(record, 0x48), At<float>(record, 0x4C), At<float>(record, 0x50), 0};
    uint8_t* projectile = PL3_CreateProjectile(At<int32_t>(record, 0), pos, rotation);
    if (!projectile) {
        return;
    }
    projectile[0x111] = uint8_t(int32_t(At<float>(record, 0x44)));
    projectile[0x110] = self[0x14];
    PL3_IncludeEntity(At<int32_t>(self, 0x14), projectile);
}

#define PLACE3_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

PLACE3_REVERSED("ObjectInstanceLoaderImpl::vf6", 0x004359D0, &Instance_Filter);
PLACE3_REVERSED("IplLoaderImpl::RecordGetTransform", 0x00435C20, &Record_GetTransform);
PLACE3_REVERSED("IplLoaderImpl::SetRotationFromRecord", 0x00435C70, &SetRotationFromRecord);
PLACE3_REVERSED("OcclusionVolumeLoaderImpl::vf2", 0x00435D90, &Occlusion_Add);
PLACE3_REVERSED("PerimeterLoaderImpl::vf0", 0x00435E10, &Perimeter_Begin);
PLACE3_REVERSED("PoiPointsLoaderImpl::vf0", 0x004361F0, &PoiPoints_Begin);
PLACE3_REVERSED("PoiPointsLoaderImpl::vf2", 0x004362C0, &PoiPoints_SetHeading);
PLACE3_REVERSED("CTriggers::FindByName", 0x00436C10, &FindTriggerByName);
PLACE3_REVERSED("IplLoaderImpl::GetEntityBoundRect", 0x00436C40, &GetEntityBoundRect);
PLACE3_REVERSED("TriggerLoaderImpl::vf8", 0x00436F50, &Trigger_End);
PLACE3_REVERSED("PoiLoaderImpl::vf2", 0x00437120, &Poi_GetPointLoader);
PLACE3_REVERSED("CEntity::ModelFn520440", 0x00437C90, &Entity_ModelFn520440);
PLACE3_REVERSED("PerimeterLoaderImpl::vf4", 0x00437D40, &Perimeter_End);
PLACE3_REVERSED("PointLoaderImpl::vf3", 0x00437EA0, &Point_Add);
PLACE3_REVERSED("PoiPointsLoaderImpl::vf3", 0x00437F90, &PoiPoints_SetFields);
PLACE3_REVERSED("RailLoaderImpl::vf2", 0x004380F0, &Rail_AddPoint);
PLACE3_REVERSED("IplLoaderImpl::SectionLoaderImpl::SectionLoaderImpl", 0x00438240, &SectionLoader_Construct);
PLACE3_REVERSED("ObjectInstanceLoaderImpl::vf5", 0x004388E0, &Instance_Add);
PLACE3_REVERSED("ProjectileInstanceLoaderImpl::vf0", 0x00438DF0, &Projectile_Add);

} // namespace IplLoaders3
