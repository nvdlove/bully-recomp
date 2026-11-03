// The placement file reader's section loaders (IplLoaderImpl::SectionLoaderImpl and the
// loader objects in it: triggers, points, paths, POIs, rails, props, ...): the virtual
// functions the binary reader calls with each record's fields. Named by the game's RTTI
// and their slot in the vtable; the objects are laid out by offset (only these touch them).
// Also CRect's point tests from the same place in the exe.

#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_VAR(CPool<uint8_t>*, ms_pTriggerPool, 0x020C7C48);
inline void* const TheTriggers = reinterpret_cast<void*>(0x020C7E98);
GAME_VAR(void*, ms_pCurrentPerimeter, 0x00BD5C58);   // the last perimeter read (triggers get it)
inline char* const ms_nameBuffer = reinterpret_cast<char*>(0x00C221A8);

template <typename... A>
inline int __cdecl Snprintf(char* out, size_t size, const char* format, A... rest) { return crt::Snprintf(out, size, format, rest...); }
inline char* __cdecl StrUpr(char* s) { return crt::Strupr(s); }   // _strupr
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t, __cdecl, Triggers_Fn_6D67A0, 0x006D67A0, const char* name);
GAME_FN(void*, __cdecl, Trigger_New, 0x006D6AE0, size_t size);
GAME_FN(void*, __thiscall, Trigger_Construct, 0x006D6A70, void* trigger, int);
GAME_FN(void*, __cdecl, Point_New, 0x006D59A0, size_t size);
GAME_FN(void*, __thiscall, Point_Construct, 0x006D5920, void* point, int);
GAME_FN(void*, __cdecl, Path_New, 0x005EC820, size_t size);
GAME_FN(void*, __thiscall, Path_Construct, 0x005EC5E0, void* path, int);
GAME_FN(void, __thiscall, Path_Fn_5ECB20, 0x005ECB20, void* path, void*);
GAME_FN(void*, __cdecl, Poi_New, 0x00447B80, size_t size);
GAME_FN(void*, __thiscall, Poi_Construct, 0x00447A30, void* poi, int);
GAME_FN(void, __thiscall, Placeable_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float angle);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __thiscall, Rail_Fn_4F04A0, 0x004F04A0, void* rail, col::CVector* pos, float* heading);
GAME_FN(int, __thiscall, Triggers_FindByName, 0x006D7A10, void* triggers, const char* name);
GAME_FN(uint8_t*, __cdecl, CreatePropForTrigger, 0x004EFFB0, void* trigger, int, int, int);
GAME_FN(void, __cdecl, IplStore_IncludeEntity, 0x004384B0, int slot, uint8_t* entity);

using re::At;

// "name" or "_name" (names without the underscore get one), upper case, into the game's buffer
void MakeName(const char* name) {
    Snprintf(ms_nameBuffer, 0xFF, name[0] == '_' ? "%s" : "_%s", name);
    StrUpr(ms_nameBuffer);
}

// (x87: the degrees times pi (as a double), over 180)
float ToRadians(float degrees) {
    return float(double(degrees) * 3.1415927410125732 / 180.0);
}

} // namespace

// ---- CRect ------------------------------------------------------------------------------

namespace CRectTests {

// 0x435AA0 (thiscall). (NaN x passes, NaN y doesn't: the game's compares)
static uint8_t __fastcall IsPointInside(const col::CRect* rect, re::Edx, const float* xy) {
    float x = xy[0];
    float y = xy[1];
    return !(x < rect->left) && !(x > rect->right) && !(y < rect->bottom) && y <= rect->top;
}

// 0x435AF0 (thiscall): with a margin round it (each bound rounded to a float first)
static uint8_t __fastcall IsPointInsideMargin(const col::CRect* rect, re::Edx, const float* xy, float margin) {
    float x = xy[0];
    float y = xy[1];
    float left = float(double(rect->left) - double(margin));
    if (left > x) {
        return 0;
    }
    float right = float(double(rect->right) + double(margin));
    if (right < x) {
        return 0;
    }
    float bottom = float(double(rect->bottom) - double(margin));
    if (bottom > y) {
        return 0;
    }
    float top = float(double(rect->top) + double(margin));
    return top >= y;
}

// 0x435B70 (thiscall): from a rect laid out { top?, left, ... } (the other rect class's
// order: this gets its second, third, fourth and first)
static col::CRect* __fastcall AssignRotated(col::CRect* rect, re::Edx, const float* other) {
    float a = other[2];
    float b = other[3];
    float c = other[0];
    rect->left = other[1];
    rect->top = a;
    rect->right = b;
    rect->bottom = c;
    return rect;
}

} // namespace CRectTests

// ---- the section loaders ---------------------------------------------------------------

namespace IplLoaders {

// OcclusionVolumeLoaderImpl vf1 (0x435D70): the centre (+4)
static void __fastcall Occlusion_SetCentre(void* self, re::Edx, float x, float y, float z) {
    At<float>(self, 4) = x;
    At<float>(self, 8) = y;
    At<float>(self, 0xC) = z;
}

// PerimeterLoaderImpl vf2 (0x435E70): a coordinate (+0xC on)
static void __fastcall Perimeter_SetCoord(void* self, re::Edx, int index, float value) {
    At<float>(self, 0xC + index * 4) = value;
}

// TriggerLoaderImpl vf0 (0x435E80): a new trigger (+0x1C), named (upper case, "_" first)
static void __fastcall Trigger_Begin(void* self, re::Edx, const char* name, int, int flag) {
    MakeName(name);
    void* memory = Trigger_New(0x40);
    uint8_t* trigger = memory ? static_cast<uint8_t*>(Trigger_Construct(memory, -2)) : nullptr;
    At<uint8_t*>(self, 0x1C) = trigger;
    trigger[0x3E] = At<uint8_t>(self, 4);
    At<uint8_t>(self, 0x20) = Triggers_Fn_6D67A0(ms_nameBuffer);
    *reinterpret_cast<uint32_t*>(At<uint8_t*>(self, 0x1C) + 0x2C) = HashName(ms_nameBuffer);
    At<uint8_t*>(self, 0x1C)[0x3C] = uint8_t(flag);
    *reinterpret_cast<re::Ptr32<void>*>(At<uint8_t*>(self, 0x1C) + 0x20) = ms_pCurrentPerimeter;
    At<uint8_t*>(self, 0x1C)[0x3D] = 5;
}

// vf1 (0x435F20): the position (kept at +8, and the trigger's +4)
static void __fastcall Trigger_SetPosition(void* self, re::Edx, float x, float y, float z) {
    uint8_t* trigger = At<uint8_t*>(self, 0x1C);
    At<float>(self, 8) = x;
    At<float>(self, 0xC) = y;
    At<float>(self, 0x10) = z;
    std::memcpy(trigger + 4, static_cast<uint8_t*>(self) + 8, 12);
}

// vf2 (0x435F50): the size (the trigger's +0x10)
static void __fastcall Trigger_SetSize(void* self, re::Edx, float x, float y, float z) {
    float* size = reinterpret_cast<float*>(At<uint8_t*>(self, 0x1C) + 0x10);
    size[0] = x;
    size[1] = y;
    size[2] = z;
}

// vf3 (0x435F90): the heading in degrees (+0x38 radians)
static void __fastcall Trigger_SetHeading(void* self, re::Edx, float degrees) {
    *reinterpret_cast<float*>(At<uint8_t*>(self, 0x1C) + 0x38) = ToRadians(degrees);
}

// vf4 (0x435FC0)
static void __fastcall Trigger_SetField34(void* self, re::Edx, float value) {
    *reinterpret_cast<float*>(At<uint8_t*>(self, 0x1C) + 0x34) = value;
}

// vf5 / vf6 (0x435FD0 / 0x436010): flags 1 / 2 of +0x3F (set when the value isn't 0)
static void __fastcall Trigger_SetFlag1(void* self, re::Edx, float value) {
    uint8_t& flags = At<uint8_t*>(self, 0x1C)[0x3F];
    flags = value != 0.0f ? uint8_t(flags | 1) : uint8_t(flags & ~1);
}

static void __fastcall Trigger_SetFlag2(void* self, re::Edx, float value) {
    uint8_t& flags = At<uint8_t*>(self, 0x1C)[0x3F];
    flags = value != 0.0f ? uint8_t(flags | 2) : uint8_t(flags & ~2);
}

// PointLoaderImpl vf0 (0x436050): a new point (+4); "SV_" names flag 1, "DT_" flag 2
static void __fastcall Point_Begin(void* self, re::Edx, const char* name, int, int flag) {
    void* memory = Point_New(0x1C);
    uint8_t* point = memory ? static_cast<uint8_t*>(Point_Construct(memory, -2)) : nullptr;
    At<uint8_t*>(self, 4) = point;
    point[0x14] = At<uint8_t>(self, 0x20);
    uint32_t& flags = *reinterpret_cast<uint32_t*>(At<uint8_t*>(self, 4) + 0x18);
    flags &= ~1u;
    flags &= ~2u;
    if (name[0] == 'S' && name[1] == 'V' && name[2] == '_') {
        flags |= 1;
    } else if (name[0] == 'D' && name[1] == 'T' && name[2] == '_') {
        flags |= 2;
    } else if (name[0] == '_') {
        if (name[1] == 'D' && name[2] == 'T' && name[3] == '_') {
            flags |= 2;
        } else if (name[1] == 'S' && name[2] == 'V' && name[3] == '_') {
            flags |= 1;
        }
    }
    MakeName(name);
    *reinterpret_cast<uint32_t*>(At<uint8_t*>(self, 4) + 8) = HashName(ms_nameBuffer);
    At<uint8_t*>(self, 4)[0xC] = uint8_t(flag);
    At<uint8_t*>(self, 4)[0xD] = 5;
}

// vf1 / vf2 (0x436150 / 0x436170): two vectors (+8, +0x14)
static void __fastcall Point_SetPosition(void* self, re::Edx, float x, float y, float z) {
    At<float>(self, 8) = x;
    At<float>(self, 0xC) = y;
    At<float>(self, 0x10) = z;
}

static void __fastcall Point_SetVector2(void* self, re::Edx, float x, float y, float z) {
    At<float>(self, 0x14) = x;
    At<float>(self, 0x18) = y;
    At<float>(self, 0x1C) = z;
}

// PathPointsLoaderImpl vf1 / vf2 / vf3 (0x436190 / 0x4361B0 / 0x4361D0)
static void __fastcall PathPoints_SetPosition(void* self, re::Edx, float x, float y, float z) {
    At<float>(self, 0xC) = x;
    At<float>(self, 0x10) = y;
    At<float>(self, 0x14) = z;
}

static void __fastcall PathPoints_SetVector2(void* self, re::Edx, float x, float y, float z) {
    At<float>(self, 0x18) = x;
    At<float>(self, 0x1C) = y;
    At<float>(self, 0x20) = z;
}

static void __fastcall PathPoints_SetFields(void* self, re::Edx, float value, int field24, int field8) {
    At<float>(self, 0x28) = value;
    At<int32_t>(self, 0x24) = field24;
    At<int32_t>(self, 8) = field8;
}

// PathLoaderImpl vf0 (0x437000): a new path (+0x34), named
static void __fastcall Path_Begin(void* self, re::Edx, const char* name, int, int flag4, int flag5, int) {
    void* memory = Path_New(0x14);
    uint8_t* path = memory ? static_cast<uint8_t*>(Path_Construct(memory, -2)) : nullptr;
    At<uint8_t*>(self, 0x34) = path;
    path[7] = At<uint8_t>(self, 0x38);
    MakeName(name);
    *reinterpret_cast<uint32_t*>(At<uint8_t*>(self, 0x34) + 8) = HashName(ms_nameBuffer);
    At<uint8_t*>(self, 0x34)[4] = uint8_t(flag4);
    At<uint8_t*>(self, 0x34)[5] = uint8_t(flag5);
    At<uint8_t*>(self, 0x34)[6] = 5;
}

// vf3 (0x437090): a point (its record +0xC) onto the path
static void __fastcall Path_AddPoint(void* self, re::Edx, int, uint8_t* record) {
    Path_Fn_5ECB20(At<void*>(self, 0x34), record + 0xC);
}

// PoiLoaderImpl vf0 (0x4370B0): a new POI list (+0x18)
static void __fastcall Poi_Begin(void* self, re::Edx, const char* name, int, int field8, int) {
    void* memory = Poi_New(0x1C);
    uint8_t* poi = memory ? static_cast<uint8_t*>(Poi_Construct(memory, -2)) : nullptr;
    At<uint8_t*>(self, 0x18) = poi;
    poi[0x14] = At<uint8_t>(self, 0x1C);
    *reinterpret_cast<uint32_t*>(At<uint8_t*>(self, 0x18) + 4) = HashName(name);
    *reinterpret_cast<int32_t*>(At<uint8_t*>(self, 0x18) + 8) = field8;
    At<uint8_t*>(self, 0x18)[0xC] = 5;
    At<int32_t>(self, 0xC) = At<int32_t>(self, 0x1C);
    At<uint8_t*>(self, 0x10) = At<uint8_t*>(self, 0x18);
    At<int32_t>(self, 0x14) = 0xFFFF;
}

// PoiPointsLoaderImpl vf1 (0x437F60): the POI's position
static void __fastcall PoiPoints_SetPosition(void* self, re::Edx, float x, float y, float z) {
    Placeable_SetPosition(At<void*>(self, 4), x, y, z);
}

// RailLoaderImpl vf4 (0x4381B0): the prop (+4) placed at the rail's (+8) start, into the world
static void __fastcall Rail_End(void* self, re::Edx) {
    col::CVector pos = {0.0f, 0.0f, 0.0f};
    float heading;   // (whatever 0x4F04A0 leaves)
    Rail_Fn_4F04A0(At<void*>(self, 8), &pos, &heading);
    Placeable_SetPosition(At<void*>(self, 4), pos.x, pos.y, pos.z);
    uint8_t* entity = At<uint8_t*>(self, 4);
    if (void* matrix = *reinterpret_cast<re::Ptr32<void>*>(entity + 0x14)) {
        Matrix_SetRotateZOnly(matrix, heading);
    }
    *reinterpret_cast<float*>(entity + 0x10) = heading;
    World_Add(At<void*>(self, 4));
}

// PropLoaderImpl vf0 (0x438EE0): the prop of a trigger (flag 0x10), made now
static void __fastcall Prop_Load(void* self, re::Edx, const char* triggerName, int) {
    int index = Triggers_FindByName(TheTriggers, triggerName);
    if (index == -1) {
        return;
    }
    CPool<uint8_t>* pool = ms_pTriggerPool;
    if (pool->flags[index] & 0x80) {
        return;
    }
    uint8_t* trigger = pool->objects + pool->elemSize * index;
    if (!trigger || !(trigger[0x3F] & 0x10)) {
        return;
    }
    uint8_t* entity = CreatePropForTrigger(trigger, Triggers_Fn_6D67A0(triggerName), -1, 1);
    if (entity) {
        entity[0x110] = At<uint8_t>(self, 4);
        IplStore_IncludeEntity(At<int32_t>(self, 4), entity);
    }
}

// SectionLoaderImpl's getters (vf2, vf7, vf13, ...): the loader of a section kind, told the
// file's def (+8), slot (+0x10) and flags; the section index argument unused
static void* __fastcall Section_Objects(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x2C) = At<int32_t>(self, 0x10);
    At<uint8_t>(self, 0x28) = At<uint8_t>(self, 0x14);
    At<int32_t>(self, 0x1C) = At<int32_t>(self, 8);
    return self + 0x18;
}

static void* __fastcall Section_Projectiles(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x44) = At<int32_t>(self, 0x10);
    At<uint8_t>(self, 0x40) = At<uint8_t>(self, 0x14);
    At<int32_t>(self, 0x34) = At<int32_t>(self, 8);
    return self + 0x30;
}

static void* __fastcall Section_Occlusion(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x58) = At<int32_t>(self, 0x24);
    At<uint8_t>(self, 0x5C) = At<uint8_t>(self, 0x14);
    return self + 0x48;
}

static void* __fastcall Section_Perimeters(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x64) = At<int32_t>(self, 0x10);
    return self + 0x60;
}

static void* __fastcall Section_Triggers(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0xC8) = At<int32_t>(self, 0x10);
    return self + 0xC4;
}

static void* __fastcall Section_Points(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x108) = At<int32_t>(self, 0x10);
    return self + 0xE8;
}

static void* __fastcall Section_Paths(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x144) = At<int32_t>(self, 0x10);
    return self + 0x10C;
}

static void* __fastcall Section_Props(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x14C) = At<int32_t>(self, 0x10);
    return self + 0x148;
}

static void* __fastcall Section_Pois(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x16C) = At<int32_t>(self, 0x10);
    return self + 0x150;
}

static void* __fastcall Section_Rails(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x17C) = At<int32_t>(self, 0x10);
    return self + 0x170;
}

// IplLoaderImpl vf2 (0x439080): the section loader, told the def, flag, slot and first-read
static void* __fastcall Ipl_SectionLoader(uint8_t* self, re::Edx, int) {
    At<int32_t>(self, 0x24) = At<int32_t>(self, 0xC);
    At<int32_t>(self, 0x1C) = At<int32_t>(self, 4);
    At<uint8_t>(self, 0x28) = At<uint8_t>(self, 0x10);
    At<uint8_t>(self, 0x20) = At<uint8_t>(self, 8);
    return self + 0x14;
}

} // namespace IplLoaders

// ---- registration ----------------------------------------------------------------------

#define LOADER_REVERSED(name, fn, address)                                                            \
    static void* Original_##fn;                                                                       \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace CRectTests {
LOADER_REVERSED("CRect::IsPointInside", IsPointInside, 0x00435AA0);
LOADER_REVERSED("CRect::IsPointInsideMargin", IsPointInsideMargin, 0x00435AF0);
LOADER_REVERSED("CRect::AssignRotated", AssignRotated, 0x00435B70);
} // namespace CRectTests

namespace IplLoaders {
LOADER_REVERSED("OcclusionVolumeLoaderImpl::vf1", Occlusion_SetCentre, 0x00435D70);
LOADER_REVERSED("PerimeterLoaderImpl::vf2", Perimeter_SetCoord, 0x00435E70);
LOADER_REVERSED("TriggerLoaderImpl::vf0", Trigger_Begin, 0x00435E80);
LOADER_REVERSED("TriggerLoaderImpl::vf1", Trigger_SetPosition, 0x00435F20);
LOADER_REVERSED("TriggerLoaderImpl::vf2", Trigger_SetSize, 0x00435F50);
LOADER_REVERSED("TriggerLoaderImpl::vf3", Trigger_SetHeading, 0x00435F90);
LOADER_REVERSED("TriggerLoaderImpl::vf4", Trigger_SetField34, 0x00435FC0);
LOADER_REVERSED("TriggerLoaderImpl::vf5", Trigger_SetFlag1, 0x00435FD0);
LOADER_REVERSED("TriggerLoaderImpl::vf6", Trigger_SetFlag2, 0x00436010);
LOADER_REVERSED("PointLoaderImpl::vf0", Point_Begin, 0x00436050);
LOADER_REVERSED("PointLoaderImpl::vf1", Point_SetPosition, 0x00436150);
LOADER_REVERSED("PointLoaderImpl::vf2", Point_SetVector2, 0x00436170);
LOADER_REVERSED("PathPointsLoaderImpl::vf1", PathPoints_SetPosition, 0x00436190);
LOADER_REVERSED("PathPointsLoaderImpl::vf2", PathPoints_SetVector2, 0x004361B0);
LOADER_REVERSED("PathPointsLoaderImpl::vf3", PathPoints_SetFields, 0x004361D0);
LOADER_REVERSED("PathLoaderImpl::vf0", Path_Begin, 0x00437000);
LOADER_REVERSED("PathLoaderImpl::vf3", Path_AddPoint, 0x00437090);
LOADER_REVERSED("PoiLoaderImpl::vf0", Poi_Begin, 0x004370B0);
LOADER_REVERSED("PoiPointsLoaderImpl::vf1", PoiPoints_SetPosition, 0x00437F60);
LOADER_REVERSED("RailLoaderImpl::vf4", Rail_End, 0x004381B0);
LOADER_REVERSED("PropLoaderImpl::vf0", Prop_Load, 0x00438EE0);
LOADER_REVERSED("SectionLoaderImpl::vf2", Section_Objects, 0x00438310);
LOADER_REVERSED("SectionLoaderImpl::vf7", Section_Projectiles, 0x00438330);
LOADER_REVERSED("SectionLoaderImpl::vf13", Section_Occlusion, 0x00438350);
LOADER_REVERSED("SectionLoaderImpl::vf17", Section_Perimeters, 0x00438370);
LOADER_REVERSED("SectionLoaderImpl::vf21", Section_Triggers, 0x00438380);
LOADER_REVERSED("SectionLoaderImpl::vf25", Section_Points, 0x004383A0);
LOADER_REVERSED("SectionLoaderImpl::vf29", Section_Paths, 0x004383C0);
LOADER_REVERSED("SectionLoaderImpl::vf33", Section_Props, 0x004383E0);
LOADER_REVERSED("SectionLoaderImpl::vf37", Section_Pois, 0x00438400);
LOADER_REVERSED("SectionLoaderImpl::vf41", Section_Rails, 0x00438420);
LOADER_REVERSED("IplLoaderImpl::vf2", Ipl_SectionLoader, 0x00439080);
} // namespace IplLoaders
