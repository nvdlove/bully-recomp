// The DAT placement records (0x6D4850..0x6D6502): liquid volumes (triggers with a
// liquid type), perimeters (2D polygons of points, with a bounding box), point lists
// (named lists of 0x2C point records: position, model, sound, butes, a mission) and
// the point list manager (0xBC5E7C). Their points live in pools with uint16 "next"
// link arrays (DatManager.cpp's LinkArrays; 0xFFFE ends a list, 0xFFFF none).
//
// Perimeter (0x30): +0 vtable 0x94328C, +4 the first point, +8 / +0xC read ints, +0x10
// the name's hash, +0x14 the area, +0x18 the id, +0x1C a byte, +0x20 / +0x24 the min
// x / y, +0x28 / +0x2C the max. PointList (0x1C): +0 vtable 0x94326C, +4 the first
// point, +6 the iteration's next, +8 the name hash, +0xC a byte, +0xD the area, +0x10
// the script id, +0x14 a tag, +0x18 flags (1 "SV_", 2 "DT_").

#include <cmath>
#include <cstdint>

#include "game/Pool.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using AnyPool = CPool<uint8_t>;

using re::At;

struct CVec {
    float x, y, z;
};
static_assert(sizeof(CVec) == 12);

// the archive readers' small strings: a pointer to the buffer, its size, the buffer
template <int N>
struct SmallString {
    re::Ptr32<char> ptr;
    int32_t size;
    char buffer[N];

    void Init() {
        ptr = buffer;
        size = N;
        buffer[0] = 0;
    }
};

// the archive reader's fields (thiscall, by vtable offset)
void ReadVector(void* archive, void* out, const char* name) {
    vc::Call<void>(archive, 0x20, out, name);
}
template <int N>
void ReadString(void* archive, SmallString<N>* out, int max, const char* name) {
    vc::Call<void>(archive, 0x24, out, max, name);
}
void Read4(void* archive, void* out, const char* name) {
    vc::Call<void>(archive, 0x28, out, name);
}
uint8_t ReadInt(void* archive, int32_t* out, const char* name) {
    return vc::Call<uint8_t>(archive, 0x38, out, name);
}
void ReadByte(void* archive, uint8_t* out, const char* name) {
    vc::Call<void>(archive, 0x3C, out, name);
}

struct LinkArray {
    int32_t count;
    re::Ptr32<uint16_t> links;
    uint16_t last;
};
GAME_VAR(LinkArray, ppl_perimeterPointLinks, 0x00BC5D1C);
GAME_VAR(LinkArray, ppl_pointLinks, 0x00BC5D28);
GAME_VAR(AnyPool*, ppl_triggerPool, 0x020C7C48);
GAME_VAR(AnyPool*, ppl_perimeterPool, 0x020C7C4C);
GAME_VAR(AnyPool*, ppl_pointListPool, 0x020C7C54);
GAME_VAR(AnyPool*, ppl_pointPool, 0x020C7C88);
GAME_VAR(AnyPool*, ppl_perimeterPointPool, 0x020C7C90);
GAME_VAR(uint8_t, ppl_currentArea, 0x020C7E94);   // (the area a DAT is being loaded for)
GAME_VAR(int32_t, ppl_currArea, 0x00BD1008);
inline void* const ppl_datManager = reinterpret_cast<void*>(0x020C7C48);
inline void* const ppl_triggers = reinterpret_cast<void*>(0x020C7E98);
inline void* const ppl_scripts = reinterpret_cast<void*>(0x00D02850);
inline void* const ppl_soundBank = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const ppl_soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const ppl_missions = reinterpret_cast<void*>(0x020C3CA0);
inline const re::Ptr32<const char>* const ppl_liquidTypeNames = reinterpret_cast<const re::Ptr32<const char>*>(0x009432F8);
constexpr uintptr_t VT_LiquidLoader = 0x009433C4;
constexpr uintptr_t VT_Perimeter = 0x0094328C;
constexpr uintptr_t VT_PointList = 0x0094326C;
constexpr uintptr_t VT_IArchivable = 0x00900CDC;
constexpr uintptr_t VT_PointListManager = 0x009437F8;

// the liquid volumes (0xBC5DA8): {type, trigger slot} x 10, the count at +0x50
struct Liquid {
    int32_t type;
    int32_t trigger;
};
GAME_ARRAY(Liquid, ppl_liquids, 0x00BC5DA8);
GAME_VAR(int32_t, ppl_liquidCount, 0x00BC5DF8);

// the section / field names
GAME_VAR(const char*, ppl_nPerimeterPointX, 0x00BC5DFC);
GAME_VAR(const char*, ppl_nPerimeterPointY, 0x00BC5E00);
GAME_VAR(const char*, ppl_nPerimeterPoints, 0x00BC5E04);
GAME_VAR(const char*, ppl_nPerimeterName, 0x00BC5E08);
GAME_VAR(const char*, ppl_nPerimeterByte, 0x00BC5E0C);
GAME_VAR(const char*, ppl_nPerimeter8, 0x00BC5E10);
GAME_VAR(const char*, ppl_nPerimeterC, 0x00BC5E14);
GAME_VAR(const char*, ppl_nListName, 0x00BC5E1C);
GAME_VAR(const char*, ppl_nListByte, 0x00BC5E24);
GAME_VAR(const char*, ppl_nPointPosition, 0x00BC5E28);
GAME_VAR(const char*, ppl_nPointVector, 0x00BC5E2C);
GAME_VAR(const char*, ppl_nPointName, 0x00BC5E30);
GAME_VAR(const char*, ppl_nPointModel, 0x00BC5E34);
GAME_VAR(const char*, ppl_nPointSound, 0x00BC5E38);
GAME_VAR(const char*, ppl_nPointBank, 0x00BC5E3C);
GAME_VAR(const char*, ppl_nPointChance, 0x00BC5E40);
GAME_VAR(const char*, ppl_nPoint28, 0x00BC5E44);
GAME_VAR(const char*, ppl_nPoint29, 0x00BC5E48);
GAME_VAR(const char*, ppl_nPoint22, 0x00BC5E4C);
GAME_VAR(const char*, ppl_nPoint23, 0x00BC5E50);
GAME_VAR(const char*, ppl_nPointCondition, 0x00BC5E54);
GAME_VAR(const char*, ppl_nPointFlag, 0x00BC5E58);
GAME_VAR(const char*, ppl_nPointType, 0x00BC5E5C);
GAME_VAR(const char*, ppl_nListCount, 0x00BC5E60);
GAME_VAR(const char*, ppl_nPointByteA, 0x00BC5E64);
GAME_VAR(const char*, ppl_nPointMission, 0x00BC5E68);
GAME_VAR(const char*, ppl_nPointByteB, 0x00BC5E6C);
GAME_VAR(const char*, ppl_nPointIntC, 0x00BC5E70);
GAME_VAR(const char*, ppl_nPointButes, 0x00BC5E74);
GAME_VAR(const char*, ppl_nPointList, 0x00BC5E78);

GAME_FN(int, __cdecl, PPL_FindNameInList, 0x0073AC90, const re::Ptr32<const char>* names, int count, const char* name, int fallback, int);
GAME_FN(void*, __cdecl, PPL_Archive_Open, 0x005B07E0, void* file, uint32_t* size, int);
GAME_FN(uint8_t, __thiscall, PPL_Archive_ReadSection, 0x005B0720, void* archive, void* loader, const char* name);
GAME_FN(void, __cdecl, PPL_Archive_Close, 0x005B0860, void* archive);
inline char* __cdecl PPL_Strncpy(char* dst, const char* src, uint32_t n) { return crt::Strncpy(dst, src, (size_t)n); }
inline void* __cdecl PPL_Memcpy(void* dst, const void* src, uint32_t n) { return crt::Memcpy(dst, src, (size_t)n); }
inline char* __cdecl PPL_StrNCat(char* dst, const char* src, uint32_t n) { return crt::Strncat(dst, src, (size_t)n); }
inline char* __cdecl PPL_Strchr(const char* s, int c) { return crt::Strchr(s, c); }
inline int __cdecl PPL_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(void, __thiscall, PPL_DATManager_LoadDat, 0x006D3C70, void* manager, const char* name, int area);
GAME_FN(int32_t, __thiscall, PPL_Triggers_FindByName, 0x006D7A10, void* triggers, const char* name);
GAME_FN(uint8_t, __cdecl, PPL_IsAreaExterior, 0x00430320, int32_t area);
GAME_FN(uint8_t, __thiscall, PPL_Trigger_Contains, 0x006D6FA0, void* trigger, const CVec* point);
GAME_FN(float, __thiscall, PPL_Trigger_GetTop, 0x006D6800, void* trigger);
GAME_FN(int32_t, __cdecl, PPL_HashName, 0x00576ED0, const char* name);
GAME_FN(int32_t, __cdecl, PPL_GetModelIndexHashed, 0x0051C390, const char* name);
GAME_FN(void*, __cdecl, PPL_GetModelInfo, 0x0051C190, const char* name, int32_t* id);
GAME_FN(CVec*, __cdecl, PPL_VectorSub, 0x00414B70, CVec* out, const CVec* a, const CVec* b);
GAME_FN(CVec*, __cdecl, PPL_VectorAdd, 0x004129A0, CVec* out, const CVec* a, const CVec* b);
GAME_FN(float, __thiscall, PPL_Normalise, 0x00414E10, CVec* v);
GAME_FN(float, __cdecl, PPL_DistToLine, 0x005691A0, const CVec* a, const CVec* b, const CVec* point);
GAME_FN(uint8_t*, __thiscall, PPL_PoolGetAt, 0x00415DE0, AnyPool* pool, int slot);
GAME_FN(void, __thiscall, PPL_Chain_Clear, 0x00447A70, LinkArray* links, int first);
GAME_FN(uint8_t*, __thiscall, PPL_PerimeterPointPool_New, 0x00450620, AnyPool* pool);
GAME_FN(void*, __thiscall, PPL_PerimeterPool_New, 0x0056EDF0, AnyPool* pool);
GAME_FN(void*, __thiscall, PPL_PointPool_New, 0x004A6350, AnyPool* pool);
GAME_FN(void*, __thiscall, PPL_PointListPool_New, 0x00447B00, AnyPool* pool);
GAME_FN(void, __thiscall, PPL_NameKey_Copy, 0x005FA880, uint32_t* self, const void* from);
GAME_FN(uint32_t*, __thiscall, PPL_NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, PPL_HashPath_Set, 0x005FA8C0, uint32_t* path, int count, const uint32_t* names);
GAME_FN(void*, __cdecl, PPL_Butes_Find, 0x0072AEA0, const uint32_t* path, const void* root);
GAME_FN(void, __thiscall, PPL_Nothing, 0x0049A650, void* self);   // (an empty function: destructors, debug prints)
GAME_FN(int32_t, __cdecl, PPL_Sfx_Find, 0x005A7D60, const char* name, int);
GAME_FN(int32_t, __thiscall, PPL_SoundBank_FindName, 0x00595B80, void* bank, const char* name);
GAME_FN(uint16_t, __thiscall, PPL_SoundBank_GetIndex, 0x00595160, void* bank, int32_t id);
GAME_FN(uint16_t, __thiscall, PPL_Banks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(void, __cdecl, PPL_AudioCondition_Parse, 0x00594070, const char* text, uint8_t* out);
GAME_FN(int32_t, __thiscall, PPL_Missions_Find, 0x006A9FD0, void* missions, const uint32_t* key);
GAME_FN(uint8_t*, __thiscall, PPL_Scripts_Current, 0x005DB960, void* scripts);
GAME_FN(void, __cdecl, PPL_Free, 0x005EEAB0, void* p);

// the n'th point after `first` (stopping at the list's end)
GAME_FN(int, __thiscall, PPL_Links_GetNth, 0x006D4AF0, LinkArray* links, int first, int n);
// (this file's, called at the game's addresses)
GAME_FN(int, __thiscall, PPL_Perimeter_CountPoints, 0x006D4CA0, void* perimeter);
GAME_FN(void, __thiscall, PPL_Perimeter_ClearPoints, 0x006D5520, void* perimeter);
GAME_FN(void, __thiscall, PPL_PointList_ClearPoints, 0x006D5A70, void* list);
GAME_FN(void*, __thiscall, PPL_PointList_Construct, 0x006D5920, void* list, int id);
GAME_FN(void*, __cdecl, PPL_PointList_New, 0x006D59A0, uint32_t size);
GAME_FN(void, __thiscall, PPL_Point_SetButes, 0x006D5880, void* point, const char* name);
GAME_FN(int32_t, __thiscall, PPL_PointLists_FindIndexByName, 0x006D63C0, void* self, const char* name);
GAME_FN(int32_t, __stdcall, PPL_PointLists_FindIndexByHash, 0x006D6480, int32_t hash);

// x87 float (truncated) to int, as the game's _ftol2
int FloatToInt(float value) {
    return crt::FloatToInt(value);
}

uint8_t* PoolSlot(AnyPool* pool, int slot) {
    if ((pool->flags[slot] & 0x80) != 0) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * slot;
}

int PoolIndexOf(AnyPool* pool, const void* object) {
    return int32_t(reinterpret_cast<uintptr_t>(object) - re::PtrCast<uintptr_t>(pool->objects)) / pool->elemSize;
}

// an element back to its pool
void PoolRelease(AnyPool* pool, const void* object) {
    int slot = PoolIndexOf(pool, object);
    pool->flags[slot] |= 0x80;
    if (slot < At<int32_t>(pool, 0x14)) {
        At<int32_t>(pool, 0x14) = slot;
    }
}

// a list's point count (0 for none)
int CountLinks(const LinkArray& links, uint16_t first) {
    if (first == 0xFFFF) {
        return 0;
    }
    int count = 1;
    for (uint16_t next = links.links[first]; next != 0xFFFE; next = links.links[next]) {
        count++;
    }
    return count;
}

// the crossing test of the point-in-polygon: the edge prev -> cur crosses the
// horizontal through p to its right
bool Crosses(const float* prev, const float* cur, const float* p) {
    if (prev[1] > p[1]) {
        if (!(p[1] >= cur[1])) {
            return false;
        }
        float a = (p[1] - cur[1]) * (prev[0] - cur[0]);
        float b = (p[0] - cur[0]) * (prev[1] - cur[1]);
        return b < a;
    }
    if (!(cur[1] > p[1])) {
        return false;
    }
    float a = (p[1] - cur[1]) * (prev[0] - cur[0]);
    float b = (p[0] - cur[0]) * (prev[1] - cur[1]);
    return b > a;
}

} // namespace

#define PPL_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// ---------------------------------------------------------------------------------------
// liquids

// 0x6D4850 (cdecl): a liquid type's index by name (0: none)
int __cdecl LiquidTypes_Find(const char* name) {
    return PPL_FindNameInList(ppl_liquidTypeNames, 5, name, -1, 0);
}
PPL_REVERSED("LiquidTypes::Find", 0x006D4850, &LiquidTypes_Find);

// 0x6D4870 (thiscall): loads the liquid volumes of a file
void __fastcall LiquidManager_Load(uint8_t* self, re::Edx, void* file) {
    uint32_t size = 0;
    void* archive = PPL_Archive_Open(file, &size, 0);
    for (;;) {
        uintptr_t loader = VT_LiquidLoader;
        if (PPL_Archive_ReadSection(archive, &loader, "LiquidVolume") == 0) {
            break;
        }
        At<int32_t>(self, 0x50) += 1;
    }
    PPL_Archive_Close(archive);
}
PPL_REVERSED("LiquidManager::Load", 0x006D4870, &LiquidManager_Load);

// 0x6D48D0 (thiscall, LiquidLoader's slot 1): a volume: its trigger (loading the
// trigger's DAT, "<trigger>.dat", for area 1) and its liquid type (1 when unknown)
void __fastcall LiquidLoader_Load(void*, re::Edx, void* archive) {
    SmallString<0x24> trigger;
    trigger.Init();
    trigger.size = 0x21;
    ReadString(archive, &trigger, 0x21, "Trigger");
    SmallString<0x24> dat;
    dat.Init();
    dat.size = 0x21;
    PPL_Strncpy(dat.buffer, trigger.ptr, 0x21);
    PPL_Memcpy(dat.buffer, trigger.buffer, 0x21);
    uint32_t length = 0;
    while (dat.ptr[length] != 0) {
        length++;
    }
    PPL_StrNCat(dat.ptr, ".dat", uint32_t(dat.size) - length);
    PPL_DATManager_LoadDat(ppl_datManager, dat.ptr, 1);
    ppl_liquids[ppl_liquidCount].trigger = PPL_Triggers_FindByName(ppl_triggers, trigger.ptr);
    SmallString<0x24> type;
    type.Init();
    type.size = 0x21;
    ReadString(archive, &type, 0x21, "LiquidType");
    int found = LiquidTypes_Find(type.ptr);
    ppl_liquids[ppl_liquidCount].type = found != 0 ? found : 1;
}
PPL_REVERSED("LiquidLoader::Load", 0x006D48D0, &LiquidLoader_Load);

// 0x6D4A20 (thiscall): the liquid at a point: its surface height and type (an outdoor
// point at or below 0: the sea, type 1 at 0). False when none (-1e6, -1).
uint8_t __fastcall LiquidManager_GetLiquid(uint8_t* self, re::Edx, float* height, int32_t* type, const CVec* point) {
    *type = -1;
    *height = -1000000.0f;
    uint8_t found = 0;
    if (point->z <= 0.0f && PPL_IsAreaExterior(ppl_currArea) != 0) {
        *type = 1;
        *height = 0.0f;
        return 1;
    }
    for (int i = 0; i < At<int32_t>(self, 0x50); i++) {
        int32_t slot = At<int32_t>(self, i * 8 + 4);
        uint8_t* trigger = slot < 0 ? nullptr : PoolSlot(ppl_triggerPool, slot);
        if (found == 0 && PPL_Trigger_Contains(trigger, point) != 0) {
            found = 1;
            *height = PPL_Trigger_GetTop(trigger);
            *type = At<int32_t>(self, i * 8);
        }
    }
    return found;
}
PPL_REVERSED("LiquidManager::GetLiquid", 0x006D4A20, &LiquidManager_GetLiquid);

// ---------------------------------------------------------------------------------------
// perimeters

// 0x6D4AF0 (thiscall): the n'th point after `first` (or the list's last)
int __fastcall Links_GetNth(LinkArray* self, re::Edx, int first, int n) {
    uint32_t current = uint16_t(first);
    uint16_t next = self->links[current];
    for (int i = 0; next != 0xFFFE && i != n; i++) {
        current = next;
        next = self->links[current];
    }
    return int(current);
}
PPL_REVERSED("LinkArray::GetNth", 0x006D4AF0, &Links_GetNth);

// 0x6D4B30 (thiscall): the constructor
uint8_t* __fastcall Perimeter_Construct(uint8_t* self, re::Edx, int32_t id) {
    At<uint32_t>(self, 0) = VT_Perimeter;
    At<int32_t>(self, 0x10) = -1;
    At<int32_t>(self, 0x18) = id;
    At<uint8_t>(self, 0x1C) = 0xFF;
    At<uint16_t>(self, 4) = 0xFFFF;
    return self;
}
PPL_REVERSED("Perimeter::Perimeter", 0x006D4B30, &Perimeter_Construct);

// 0x6D4B60 (thiscall): the name (its hash)
void __fastcall Perimeter_SetName(uint8_t* self, re::Edx, const char* name) {
    if (name != nullptr) {
        At<int32_t>(self, 0x10) = PPL_GetModelIndexHashed(name);
    }
}
PPL_REVERSED("Perimeter::SetName", 0x006D4B60, &Perimeter_SetName);

// 0x6D4B80 (cdecl; out in ebx, start in esi, end in edi): the point of a segment closest
// to `point` (in 3D, along the normalised direction)
CVec* Segment_ClosestPoint(CVec* out, const CVec* start, const CVec* end, const CVec* point) {
    CVec temp;
    CVec direction = *PPL_VectorSub(&temp, end, start);
    PPL_Normalise(&direction);
    const CVec* v = PPL_VectorSub(&temp, end, start);
    float length = (v->y * direction.y + v->x * direction.x) + v->z * direction.z;
    v = PPL_VectorSub(&temp, point, start);
    float along = (v->y * direction.y + v->x * direction.x) + v->z * direction.z;
    if (along < 0.0f || along == 0.0f) {
        *out = *start;
        return out;
    }
    if (length < along || length == along) {
        *out = *end;
        return out;
    }
    CVec scaled = {along * direction.x, along * direction.y, along * direction.z};
    PPL_VectorAdd(out, start, &scaled);
    return out;
}
RE_UNHOOKED_FN("Segment::ClosestPoint", 0x006D4B80, &Segment_ClosestPoint);

// 0x6D4CA0 (thiscall): how many points
int __fastcall Perimeter_CountPoints(uint8_t* self, re::Edx) {
    return CountLinks(ppl_perimeterPointLinks, At<uint16_t>(self, 4));
}
PPL_REVERSED("Perimeter::CountPoints", 0x006D4CA0, &Perimeter_CountPoints);

// 0x6D4CE0 (stdcall): a 2D point zeroed; returns it
float* __stdcall Vector2_Zero(float* v) {
    v[0] = 0.0f;
    v[1] = 0.0f;
    return v;
}
PPL_REVERSED("CVector2D::Zero_6D4CE0", 0x006D4CE0, &Vector2_Zero);

// 0x6D4CF0 (cdecl): operator new (from the perimeter pool)
void* __cdecl Perimeter_New(uint32_t) {
    return PPL_PerimeterPool_New(ppl_perimeterPool);
}
PPL_REVERSED("Perimeter::operator new", 0x006D4CF0, &Perimeter_New);

namespace {
// the polygon's edges: from the last point round (the last got through CPool::GetAt,
// or inline)
template <bool viaGetAt, typename Visit>
void WalkEdges(uint8_t* self, Visit visit) {
    AnyPool* pool = ppl_perimeterPointPool;
    int last = PPL_Links_GetNth(&ppl_perimeterPointLinks, At<uint16_t>(self, 4), PPL_Perimeter_CountPoints(self) - 1);
    float* prev;
    if constexpr (viaGetAt) {
        prev = reinterpret_cast<float*>(PPL_PoolGetAt(pool, last));
    } else {
        prev = reinterpret_cast<float*>(PoolSlot(pool, last));
    }
    uint32_t index = At<uint16_t>(self, 4);
    for (;;) {
        uint16_t next = ppl_perimeterPointLinks.links[index];
        float* cur = reinterpret_cast<float*>(PoolSlot(pool, int(index)));
        visit(prev, cur);
        if (next == 0xFFFE) {
            break;
        }
        prev = cur;
        index = next;
    }
}
} // namespace

// 0x6D4D00 (thiscall): the 2D point is inside (in the bounding box, then by parity)
uint8_t __fastcall Perimeter_IsPointInside(uint8_t* self, re::Edx, const float* point) {
    if (CountLinks(ppl_perimeterPointLinks, At<uint16_t>(self, 4)) < 3) {
        return 0;
    }
    bool inside = false;
    if (!(At<float>(self, 0x20) < point[0]) || !(At<float>(self, 0x24) < point[1]) ||
        !(At<float>(self, 0x28) > point[0]) || !(At<float>(self, 0x2C) > point[1])) {
        return 0;
    }
    WalkEdges<true>(self, [&](const float* prev, const float* cur) {
        if (Crosses(prev, cur, point)) {
            inside = !inside;
        }
    });
    return inside ? 1 : 0;
}
PPL_REVERSED("Perimeter::IsPointInside", 0x006D4D00, &Perimeter_IsPointInside);

// 0x6D4EA0 (thiscall): the point is within `distance` of the perimeter (inside counts
// as negative distance)
uint8_t __fastcall Perimeter_IsWithinDistance(uint8_t* self, re::Edx, const float* point, float distance) {
    if (CountLinks(ppl_perimeterPointLinks, At<uint16_t>(self, 4)) < 3) {
        return 0;
    }
    float nearest = 1000000.0f;
    CVec flat = {point[0], point[1], 0.0f};
    bool inside = false;
    WalkEdges<false>(self, [&](const float* prev, const float* cur) {
        CVec a = {prev[0], prev[1], 0.0f};
        CVec b = {cur[0], cur[1], 0.0f};
        if (Crosses(prev, cur, point)) {
            inside = !inside;
        }
        float d = PPL_DistToLine(&a, &b, &flat);
        if (d < nearest) {
            nearest = d;
        }
    });
    if (inside) {
        nearest = -nearest;
    }
    return distance >= nearest ? 1 : 0;
}
PPL_REVERSED("Perimeter::IsWithinDistance", 0x006D4EA0, &Perimeter_IsWithinDistance);

// 0x6D50C0 (thiscall): keeps the 2D position within `margin` of the perimeter (inside
// negative): past it, it's put `margin` from the nearest edge point, towards itself
// (outside) or away (inside)
uint8_t __fastcall Perimeter_KeepWithin(uint8_t* self, re::Edx, float* position, float margin) {
    if (CountLinks(ppl_perimeterPointLinks, At<uint16_t>(self, 4)) < 3) {
        return 0;
    }
    float nearest = 1000000.0f;
    CVec closest = {0.0f, 0.0f, 0.0f};
    CVec flat = {position[0], position[1], 0.0f};
    bool inside = false;
    WalkEdges<false>(self, [&](const float* prev, const float* cur) {
        CVec a = {prev[0], prev[1], 0.0f};
        CVec b = {cur[0], cur[1], 0.0f};
        if (Crosses(prev, cur, position)) {
            inside = !inside;
        }
        CVec onEdge;
        Segment_ClosestPoint(&onEdge, &a, &b, &flat);
        CVec temp;
        const CVec* d = PPL_VectorSub(&temp, &flat, &onEdge);
        float length = crt::Sqrtf(std::fabs((d->y * d->y + d->x * d->x) + d->z * d->z));
        if (length < nearest) {
            nearest = length;
            closest = onEdge;
        }
    });
    CVec temp;
    const CVec* d;
    if (inside) {
        nearest = -nearest;
        d = PPL_VectorSub(&temp, &closest, &flat);
    } else {
        d = PPL_VectorSub(&temp, &flat, &closest);
    }
    CVec direction = *d;
    PPL_Normalise(&direction);
    if (margin < nearest) {
        position[0] = direction.x * margin + closest.x;
        position[1] = margin * direction.y + closest.y;
    }
    return 1;
}
PPL_REVERSED("Perimeter::KeepWithin", 0x006D50C0, &Perimeter_KeepWithin);

// 0x6D53D0 (thiscall): the farthest point's 2D distance
float __fastcall Perimeter_GetMaxDistance(uint8_t* self, re::Edx, const float* position) {
    float farthest = 0.0f;
    uint16_t first = At<uint16_t>(self, 4);
    if (first == 0xFFFF) {
        return farthest;
    }
    if (CountLinks(ppl_perimeterPointLinks, first) == 0) {
        return farthest;
    }
    AnyPool* pool = ppl_perimeterPointPool;
    uint32_t index = first;
    for (;;) {
        uint16_t next = ppl_perimeterPointLinks.links[index];
        const float* point = reinterpret_cast<const float*>(PoolSlot(pool, int(index)));
        float dx = point[0] - position[0];
        float dy = point[1] - position[1];
        float d = crt::Sqrtf(std::fabs(dy * dy + dx * dx));
        if (farthest < d) {
            farthest = d;
        }
        if (next == 0xFFFE) {
            break;
        }
        index = next;
    }
    return farthest;
}
PPL_REVERSED("Perimeter::GetMaxDistance", 0x006D53D0, &Perimeter_GetMaxDistance);

// 0x6D54F0 (cdecl): operator delete (into the perimeter pool)
void __cdecl Perimeter_Delete(void* perimeter) {
    PoolRelease(ppl_perimeterPool, perimeter);
}
PPL_REVERSED("Perimeter::operator delete", 0x006D54F0, &Perimeter_Delete);

// 0x6D5520 (thiscall): the points back to their pool, the list cleared
void __fastcall Perimeter_ClearPoints(uint8_t* self, re::Edx) {
    if (At<uint16_t>(self, 4) == 0xFFFF) {
        return;
    }
    AnyPool* pool = ppl_perimeterPointPool;
    uint32_t index = At<uint16_t>(self, 4);
    for (;;) {
        uint16_t next = ppl_perimeterPointLinks.links[index];
        uint8_t* point = PoolSlot(pool, int(index));
        if (point != nullptr) {
            PoolRelease(pool, point);
        }
        if (next == 0xFFFE) {
            break;
        }
        index = next;
    }
    PPL_Chain_Clear(&ppl_perimeterPointLinks, At<uint16_t>(self, 4));
    At<uint16_t>(self, 4) = 0xFFFF;
}
PPL_REVERSED("Perimeter::ClearPoints", 0x006D5520, &Perimeter_ClearPoints);

// 0x6D55A0 (thiscall, slot 1): loads the perimeter: its name, fields and points (the
// bounding box grown round them)
void __fastcall Perimeter_Load(uint8_t* self, re::Edx, void* archive) {
    At<uint8_t>(self, 0x14) = ppl_currentArea;
    PPL_Perimeter_ClearPoints(self);
    SmallString<0x1C> name;
    name.Init();
    name.size = 0x19;
    ReadString(archive, &name, 0x18, ppl_nPerimeterName);
    if (name.ptr[0] == 0) {
        At<int32_t>(self, 0x10) = -1;
    } else if (name.ptr != nullptr) {
        At<int32_t>(self, 0x10) = PPL_GetModelIndexHashed(name.ptr);
    }
    uint8_t unused;
    ReadByte(archive, &unused, ppl_nPerimeterByte);
    Read4(archive, self + 8, ppl_nPerimeter8);
    Read4(archive, self + 0xC, ppl_nPerimeterC);
    int32_t count;
    if (ReadInt(archive, &count, ppl_nPerimeterPoints) == 0 || count == 0) {
        return;
    }
    AnyPool* pool = ppl_perimeterPointPool;
    float* point = reinterpret_cast<float*>(PPL_PerimeterPointPool_New(pool));
    int slot = PoolIndexOf(pool, point);
    ppl_perimeterPointLinks.links[slot] = 0xFFFE;
    ppl_perimeterPointLinks.last = uint16_t(slot);
    At<uint16_t>(self, 4) = uint16_t(slot);
    Read4(archive, point, ppl_nPerimeterPointX);
    Read4(archive, point + 1, ppl_nPerimeterPointY);
    At<float>(self, 0x20) = point[0];
    At<float>(self, 0x24) = point[1];
    At<float>(self, 0x28) = point[0];
    At<float>(self, 0x2C) = point[1];
    for (count--; count > 0; count--) {
        point = reinterpret_cast<float*>(PPL_PerimeterPointPool_New(pool));
        Read4(archive, point, ppl_nPerimeterPointX);
        Read4(archive, point + 1, ppl_nPerimeterPointY);
        slot = PoolIndexOf(pool, point);
        ppl_perimeterPointLinks.links[ppl_perimeterPointLinks.last] = uint16_t(slot);
        ppl_perimeterPointLinks.links[slot] = 0xFFFE;
        ppl_perimeterPointLinks.last = uint16_t(slot);
        if (At<float>(self, 0x20) > point[0]) {
            At<float>(self, 0x20) = point[0];
        }
        if (At<float>(self, 0x24) > point[1]) {
            At<float>(self, 0x24) = point[1];
        }
        if (At<float>(self, 0x28) < point[0]) {
            At<float>(self, 0x28) = point[0];
        }
        if (At<float>(self, 0x2C) < point[1]) {
            At<float>(self, 0x2C) = point[1];
        }
    }
}
PPL_REVERSED("Perimeter::Load", 0x006D55A0, &Perimeter_Load);

// 0x6D57C0 (thiscall): the destructor
void __fastcall Perimeter_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = VT_Perimeter;
    PPL_Perimeter_ClearPoints(self);
    At<uint32_t>(self, 0) = VT_IArchivable;
}
PPL_REVERSED("Perimeter::~Perimeter", 0x006D57C0, &Perimeter_Destruct);

// ---------------------------------------------------------------------------------------
// point lists

namespace {
// a point record's construction (0x2C)
void PointRecord_Init(uint8_t* point) {
    At<float>(point, 0) = 0.0f;
    At<int32_t>(point, 0xC) = -1;
    At<float>(point, 4) = 0.0f;
    At<uint8_t>(point, 0x24) = 2;
    At<float>(point, 8) = 0.0f;
    At<float>(point, 0x1C) = 0.0f;
    At<uint8_t>(point, 0x2A) &= 0xE2;
    At<uint16_t>(point, 0x20) = 0;
    At<uint8_t>(point, 0x22) = 0;
    At<uint8_t>(point, 0x23) = 0;
    At<uint32_t>(point, 0x18) = 0;
    At<uint8_t>(point, 0x27) = 0;
}
} // namespace

// 0x6D57E0 (thiscall): a point record's constructor
uint8_t* __fastcall PointRecord_Construct(uint8_t* self, re::Edx) {
    PointRecord_Init(self);
    return self;
}
PPL_REVERSED("PointRecord::PointRecord", 0x006D57E0, &PointRecord_Construct);

// 0x6D5840 (thiscall): its model by name (0 when unknown... then the -1's low 16 bits)
void __fastcall PointRecord_SetModel(uint8_t* self, re::Edx, const char* name) {
    int32_t id = 0;
    PPL_GetModelInfo(name, &id);
    if (id == -1) {
        At<uint16_t>(self, 0x20) = 0;
    }
    At<uint16_t>(self, 0x20) = uint16_t(id);
}
PPL_REVERSED("PointRecord::SetModel", 0x006D5840, &PointRecord_SetModel);

// 0x6D5880 (thiscall): its butes: PickupButes/PickupData/<name or "Default">
void __fastcall PointRecord_SetButes(uint8_t* self, re::Edx, const char* name) {
    uint32_t names[2];
    uint32_t path[2];
    names[0] = 0;
    PPL_NameKey_Copy(&names[0], reinterpret_cast<const void*>(0x01B652EC));
    bool made = false;
    uint32_t key;
    const void* second;
    if (name != nullptr && name[0] != 0) {
        made = true;
        second = PPL_NameKey_Construct(&key, name);
    } else {
        second = reinterpret_cast<const void*>(0x01B652F4);
    }
    PPL_NameKey_Copy(&names[1], second);
    if (made) {
        PPL_Nothing(&key);
    }
    PPL_HashPath_Set(path, 2, names);
    At<void*>(self, 0x18) = PPL_Butes_Find(path, reinterpret_cast<const void*>(0x01B652E8));
    for (int i = 1; i >= 0; i--) {
        PPL_Nothing(&names[i]);
    }
}
PPL_REVERSED("PointRecord::SetButes", 0x006D5880, &PointRecord_SetButes);

// 0x6D5920 (thiscall): PointList's constructor
uint8_t* __fastcall PointList_Construct(uint8_t* self, re::Edx, int32_t id) {
    At<int32_t>(self, 0x10) = id;
    At<uint32_t>(self, 0) = VT_PointList;
    At<uint8_t>(self, 0x14) = 0xFF;
    At<uint16_t>(self, 4) = 0xFFFF;
    At<uint16_t>(self, 6) = 0xFFFF;
    return self;
}
PPL_REVERSED("PointList::PointList", 0x006D5920, &PointList_Construct);

// 0x6D5950 (thiscall): how many points
int __fastcall PointList_Count(uint8_t* self, re::Edx) {
    return CountLinks(ppl_pointLinks, At<uint16_t>(self, 4));
}
PPL_REVERSED("PointList::Count", 0x006D5950, &PointList_Count);

// 0x6D5990 (cdecl): a point record's operator new
void* __cdecl PointRecord_New(uint32_t) {
    return PPL_PointPool_New(ppl_pointPool);
}
PPL_REVERSED("PointRecord::operator new", 0x006D5990, &PointRecord_New);

// 0x6D59A0 (cdecl): PointList's operator new
void* __cdecl PointList_New(uint32_t) {
    return PPL_PointListPool_New(ppl_pointListPool);
}
PPL_REVERSED("PointList::operator new", 0x006D59A0, &PointList_New);

// 0x6D59B0 (thiscall): the first point (the iteration starts)
uint8_t* __fastcall PointList_First(uint8_t* self, re::Edx) {
    uint16_t first = At<uint16_t>(self, 4);
    if (first == 0xFFFF) {
        return nullptr;
    }
    At<uint16_t>(self, 6) = ppl_pointLinks.links[first];
    return PoolSlot(ppl_pointPool, first);
}
PPL_REVERSED("PointList::First", 0x006D59B0, &PointList_First);

// 0x6D59F0 (thiscall): the next point
uint8_t* __fastcall PointList_Next(uint8_t* self, re::Edx) {
    uint16_t next = At<uint16_t>(self, 6);
    if (next == 0xFFFF || next == 0xFFFE) {
        return nullptr;
    }
    At<uint16_t>(self, 6) = ppl_pointLinks.links[next];
    return PoolSlot(ppl_pointPool, next);
}
PPL_REVERSED("PointList::Next", 0x006D59F0, &PointList_Next);

// 0x6D5A40 (cdecl): PointList's operator delete
void __cdecl PointList_Delete(void* list) {
    PoolRelease(ppl_pointListPool, list);
}
PPL_REVERSED("PointList::operator delete", 0x006D5A40, &PointList_Delete);

// 0x6D5A70 (thiscall): the points back to their pool, the list cleared
void __fastcall PointList_ClearPoints(uint8_t* self, re::Edx) {
    if (At<uint16_t>(self, 4) == 0xFFFF) {
        return;
    }
    uint32_t index = At<uint16_t>(self, 4);
    for (;;) {
        AnyPool* pool = ppl_pointPool;
        uint16_t next = ppl_pointLinks.links[index];
        uint8_t* point = PoolSlot(pool, int(index));
        if (point != nullptr) {
            PoolRelease(pool, point);
        }
        if (next == 0xFFFE) {
            break;
        }
        index = next;
    }
    PPL_Chain_Clear(&ppl_pointLinks, At<uint16_t>(self, 4));
    At<uint16_t>(self, 4) = 0xFFFF;
}
PPL_REVERSED("PointList::ClearPoints", 0x006D5A70, &PointList_ClearPoints);

// 0x6D5AF0 (thiscall, slot 1): loads the list and its points
void __fastcall PointList_Load(uint8_t* self, re::Edx, void* archive) {
    PPL_PointList_ClearPoints(self);
    SmallString<0x80 + 8> name;   // (the point's name, 0x81)
    SmallString<0x1C> model;
    SmallString<0x24> listName;
    SmallString<0x14> condition;
    SmallString<0x104> sound;
    CVec vector = {0.0f, 0.0f, 0.0f};
    name.Init();
    name.size = 0x81;
    model.Init();
    model.size = 0x19;
    listName.Init();
    listName.size = 0x21;
    condition.Init();
    condition.size = 0x11;
    sound.Init();
    sound.size = 0x101;
    At<uint8_t>(self, 0xD) = ppl_currentArea;
    ReadString(archive, &listName, 0x20, ppl_nListName);
    At<uint32_t>(self, 0x18) &= 0xFFFFFFFC;
    const char* text = listName.ptr;
    if (text[0] == 'S' && text[1] == 'V' && text[2] == '_') {
        At<uint32_t>(self, 0x18) |= 1;
    } else if (text[0] == 'D' && text[1] == 'T' && text[2] == '_') {
        At<uint32_t>(self, 0x18) |= 2;
    }
    At<int32_t>(self, 8) = PPL_HashName(listName.ptr);
    int32_t value = 0;
    ReadInt(archive, &value, ppl_nListByte);
    At<uint8_t>(self, 0xC) = uint8_t(value);
    int32_t count;
    uint8_t ok = ReadInt(archive, &count, ppl_nListCount);
    AnyPool* pool = ppl_pointPool;
    bool first = true;
    count = ok != 0 ? count : 0;
    for (; count > 0; count--) {
        uint8_t* point = static_cast<uint8_t*>(PPL_PointPool_New(ppl_pointPool));
        if (point != nullptr) {
            PointRecord_Init(point);
        }
        ReadVector(archive, point, ppl_nPointPosition);
        ReadVector(archive, &vector, ppl_nPointVector);
        At<float>(point, 0x1C) = vector.x;
        ReadString(archive, &name, 0x80, ppl_nPointName);
        At<int32_t>(point, 0x10) = PPL_HashName(name.ptr);
        ReadString(archive, &model, 0x18, ppl_nPointModel);
        if (model.ptr[0] == 0) {
            At<uint16_t>(point, 0x20) = 0;
        } else {
            int32_t id = 0;
            PPL_GetModelInfo(model.ptr, &id);
            if (id == -1) {
                At<uint16_t>(point, 0x20) = 0;
            }
            At<uint16_t>(point, 0x20) = uint16_t(id);
        }
        At<uint8_t>(point, 0x2A) &= 0xFD;
        ReadString(archive, &sound, 0x100, ppl_nPointSound);
        if (sound.ptr[0] != 0) {
            char* dot = PPL_Strchr(sound.ptr, '.');
            if (dot == nullptr) {
                sound.ptr[0xF] = 0;
                int32_t id = PPL_SoundBank_FindName(ppl_soundBank, sound.ptr);
                At<uint16_t>(point, 0x14) = PPL_SoundBank_GetIndex(ppl_soundBank, id);
            } else if (PPL_Stricmp(dot + 1, "rsm") == 0) {
                At<uint8_t>(point, 0x2A) |= 2;
                At<int32_t>(point, 0x14) = PPL_Sfx_Find(sound.ptr, 1);
            } else {
                // (a debug print: "Warning! A sound emitter "%s" has an invalid SFXID: %s")
                reinterpret_cast<void(__cdecl*)(int, const char*, const char*, const char*)>(0x0049A650)(
                    0, reinterpret_cast<const char*>(0x00943704), listName.ptr, sound.ptr);
            }
        }
        ReadString(archive, &sound, 0x100, ppl_nPointBank);
        if ((At<uint8_t>(point, 0x2A) & 2) == 0) {
            At<uint16_t>(point, 0x16) = PPL_Banks_Find(ppl_soundBanks, sound.ptr);
        }
        float chance;
        Read4(archive, &chance, ppl_nPointChance);
        if (!(chance > 0.0f) && !(chance != chance)) {
            At<uint8_t>(point, 0x25) = 0;
        } else if (!(1.0f > chance) && !(chance != chance)) {
            At<uint8_t>(point, 0x25) = 100;
        } else {
            At<uint8_t>(point, 0x25) = uint8_t(FloatToInt(chance * 100.0f));
        }
        int32_t bits;   // (uninitialised stack when the fields are missing, as the game's)
        ReadInt(archive, &bits, ppl_nPoint28);
        At<uint8_t>(point, 0x28) = uint8_t(bits >> 7);
        ReadInt(archive, &bits, ppl_nPoint29);
        At<uint8_t>(point, 0x29) = uint8_t(bits >> 7);
        int32_t type;
        ReadInt(archive, &type, ppl_nPointType);
        switch (type) {
        case -1:
            At<uint8_t>(point, 0x24) = 1;
            break;
        case 1:
            At<uint8_t>(point, 0x24) = 3;
            break;
        case 2:
            At<uint8_t>(point, 0x24) = 5;
            break;
        case 3:
            At<uint8_t>(point, 0x24) = 0;
            break;
        case 4:
            At<uint8_t>(point, 0x24) = 7;
            break;
        default:
            At<uint8_t>(point, 0x24) = 2;
            break;
        }
        int32_t byte22;
        ReadInt(archive, &byte22, ppl_nPoint22);
        At<uint8_t>(point, 0x22) = uint8_t(byte22);
        int32_t byte23;
        ReadInt(archive, &byte23, ppl_nPoint23);
        At<uint8_t>(point, 0x23) = uint8_t(byte23);
        ReadString(archive, &condition, 0x10, ppl_nPointCondition);
        PPL_AudioCondition_Parse(condition.ptr, point + 0x27);
        uint8_t flag;
        ReadByte(archive, &flag, ppl_nPointFlag);
        At<uint8_t>(point, 0x2A) ^= (At<uint8_t>(point, 0x2A) ^ flag) & 1;
        uint8_t unusedA;
        ReadByte(archive, &unusedA, ppl_nPointByteA);
        SmallString<0x44> mission;
        mission.Init();
        mission.size = 0x41;
        ReadString(archive, &mission, 0x40, ppl_nPointMission);
        if (mission.ptr[0] == 0) {
            At<int32_t>(point, 0xC) = -1;
        } else {
            int32_t index;
            uint32_t key;
            if (mission.ptr[0] == '!') {
                At<uint8_t>(point, 0x2A) |= 4;
                PPL_NameKey_Construct(&key, mission.ptr + 1);
                index = PPL_Missions_Find(ppl_missions, &key);
            } else if (mission.ptr[0] == '-') {
                At<uint8_t>(point, 0x2A) |= 8;
                PPL_NameKey_Construct(&key, mission.ptr + 1);
                index = PPL_Missions_Find(ppl_missions, &key);
            } else if (PPL_Stricmp(mission.ptr, "NONE") == 0) {
                At<int32_t>(point, 0xC) = -2;
                goto missionDone;
            } else {
                PPL_NameKey_Construct(&key, mission.ptr);
                index = PPL_Missions_Find(ppl_missions, &key);
            }
            PPL_Nothing(&key);
            if (index != -1) {
                At<int32_t>(point, 0xC) = index;
            } else {
                // (a debug print: "Could not find a mission index for mission name [%s]")
                reinterpret_cast<void(__cdecl*)(int, const char*, const char*)>(0x0049A650)(
                    0, reinterpret_cast<const char*>(0x009436CC), mission.ptr);
                At<int32_t>(point, 0xC) = int32_t(0xFFF0BDC0);
            }
        }
    missionDone:
        uint8_t unusedB;
        ReadByte(archive, &unusedB, ppl_nPointByteB);
        int32_t unusedC;
        ReadInt(archive, &unusedC, ppl_nPointIntC);
        SmallString<0x1C> butes;
        butes.Init();
        butes.size = 0x19;
        ReadString(archive, &butes, 0x18, ppl_nPointButes);
        PPL_Point_SetButes(point, butes.ptr);
        int slot = PoolIndexOf(pool, point);
        if (first) {
            ppl_pointLinks.links[slot] = 0xFFFE;
            ppl_pointLinks.last = uint16_t(slot);
            At<uint16_t>(self, 4) = uint16_t(slot);
            first = false;
        } else {
            ppl_pointLinks.links[ppl_pointLinks.last] = uint16_t(slot);
            ppl_pointLinks.links[slot] = 0xFFFE;
            ppl_pointLinks.last = uint16_t(slot);
        }
    }
}
PPL_REVERSED("PointList::Load", 0x006D5AF0, &PointList_Load);

// 0x6D6240 (thiscall): the destructor
void __fastcall PointList_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = VT_PointList;
    PPL_PointList_ClearPoints(self);
    At<uint32_t>(self, 0) = VT_IArchivable;
}
PPL_REVERSED("PointList::~PointList", 0x006D6240, &PointList_Destruct);

// ---------------------------------------------------------------------------------------
// the point list manager (0xBC5E7C)

namespace {
int32_t CurrentScriptId() {
    uint8_t* script = PPL_Scripts_Current(ppl_scripts);
    return script != nullptr ? At<int32_t>(script, 0x44) : -2;
}
} // namespace

// 0x6D6260 (thiscall; ecx unused): loads `count` lists (the current script's), the
// empty ones deleted
void __fastcall PointListManager_Load(void*, re::Edx, void* archive, int32_t count, int) {
    int32_t id = CurrentScriptId();
    for (int32_t i = count; i > 0; i--) {
        void* memory = PPL_PointList_New(0x1C);
        uint8_t* list = memory != nullptr ? static_cast<uint8_t*>(PPL_PointList_Construct(memory, id)) : nullptr;
        PPL_Archive_ReadSection(archive, list, ppl_nPointList);
        if (At<uint16_t>(list, 4) == 0xFFFF) {
            vc::Call<void>(list, 0, 1);
        }
    }
}
PPL_REVERSED("PointListManager::Load", 0x006D6260, &PointListManager_Load);

// 0x6D62D0 (thiscall): the deleting destructor
void* __fastcall PointListManager_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = VT_PointListManager;
    if ((flags & 1) != 0) {
        PPL_Free(self);
    }
    return self;
}
PPL_REVERSED("PointListManager::DeletingDestructor", 0x006D62D0, &PointListManager_DeletingDestructor);

// 0x6D62F0 (cdecl): the list pool's size
int32_t __cdecl PointLists_GetPoolSize() {
    return ppl_pointListPool->size;
}
PPL_REVERSED("PointLists::GetPoolSize", 0x006D62F0, &PointLists_GetPoolSize);

// 0x6D6300 (thiscall; ecx unused): deletes an area's lists (the current script's, or all)
void __fastcall PointListManager_RemoveArea(void*, re::Edx, int32_t area, int all) {
    int32_t id = CurrentScriptId();
    AnyPool* pool = ppl_pointListPool;
    int32_t size = pool->size;
    for (int32_t slot = 1; slot < size; slot++) {
        uint8_t* list = PoolSlot(pool, slot);
        if (list == nullptr || At<int8_t>(list, 0xD) != area) {
            continue;
        }
        if (At<int32_t>(list, 0x10) == id || re::Flag(all)) {
            vc::Call<void>(list, 0, 1);
        }
    }
}
PPL_REVERSED("PointListManager::RemoveArea", 0x006D6300, &PointListManager_RemoveArea);

// 0x6D6370 (thiscall; ecx unused): deletes the lists with a tag
void __fastcall PointListManager_RemoveTagged(void*, re::Edx, int tag) {
    AnyPool* pool = ppl_pointListPool;
    int32_t size = pool->size;
    for (int32_t slot = 1; slot < size; slot++) {
        uint8_t* list = PoolSlot(pool, slot);
        if (list != nullptr && At<uint8_t>(list, 0x14) == uint8_t(tag)) {
            vc::Call<void>(list, 0, 1);
        }
    }
}
PPL_REVERSED("PointListManager::RemoveTagged", 0x006D6370, &PointListManager_RemoveTagged);

namespace {
int32_t FindByHash(int32_t hash) {
    AnyPool* pool = ppl_pointListPool;
    int32_t size = pool->size;
    for (int32_t slot = 1; slot < size; slot++) {
        uint8_t* list = PoolSlot(pool, slot);
        if (list != nullptr && At<int32_t>(list, 8) == hash) {
            return slot;
        }
    }
    return -1;
}
} // namespace

// 0x6D63C0 (stdcall): a list's slot by name (-1: none)
int32_t __fastcall PointLists_FindIndexByName(void*, re::Edx, const char* name) {
    return FindByHash(PPL_HashName(name));
}
PPL_REVERSED("PointLists::FindIndexByName", 0x006D63C0, &PointLists_FindIndexByName);

// 0x6D6410 (stdcall): a list by name. (Only slot 0 is refused: -1 reads the flag
// before the array.)
uint8_t* __fastcall PointLists_GetByName(void*, re::Edx, const char* name) {
    int32_t slot = PPL_PointLists_FindIndexByName(nullptr, name);
    AnyPool* pool = ppl_pointListPool;
    if (slot != 0 && slot < pool->size && (pool->flags[slot] & 0x80) == 0) {
        return pool->objects + pool->elemSize * slot;
    }
    return nullptr;
}
PPL_REVERSED("PointLists::GetByName", 0x006D6410, &PointLists_GetByName);

// 0x6D6450 (stdcall): a list by slot
uint8_t* __fastcall PointLists_GetAt(void*, re::Edx, int32_t slot) {
    AnyPool* pool = ppl_pointListPool;
    if (slot < pool->size && (pool->flags[slot] & 0x80) == 0) {
        return pool->objects + pool->elemSize * slot;
    }
    return nullptr;
}
PPL_REVERSED("PointLists::GetAt", 0x006D6450, &PointLists_GetAt);

// 0x6D6480 (stdcall): a list's slot by hash (-1: none)
int32_t __stdcall PointLists_FindIndexByHash(int32_t hash) {
    return FindByHash(hash);
}
PPL_REVERSED("PointLists::FindIndexByHash", 0x006D6480, &PointLists_FindIndexByHash);

// 0x6D64D0 (stdcall): a list by hash
uint8_t* __fastcall PointLists_GetByHash(void*, re::Edx, int32_t hash) {
    int32_t slot = PPL_PointLists_FindIndexByHash(hash);
    AnyPool* pool = ppl_pointListPool;
    if (slot != -1 && slot < pool->size && (pool->flags[slot] & 0x80) == 0) {
        return pool->objects + pool->elemSize * slot;
    }
    return nullptr;
}
PPL_REVERSED("PointLists::GetByHash", 0x006D64D0, &PointLists_GetByHash);
