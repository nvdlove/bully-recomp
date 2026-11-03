// More of the points of interest (see POIs.cpp): POIList's loading, the POI manager's
// dynamic POIs, taking a POI over, picking one for a ped or the player, its per-frame
// update, and the range tests (the population's distances: +0x7828 on, +0x7830 on,
// +0x782C visible, +0x7870 height).

#include <cmath>
#include <cstdint>

#include "game/Collision.h"
#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_VAR(CPool<uint8_t>*, ms_poiPool, 0x020C7C84);
GAME_VAR(CPool<uint8_t>*, ms_poiListPool, 0x020C7C58);
GAME_VAR(CPool<uint8_t>*, ms_pool_C0F5F0, 0x00C0F5F0);
GAME_VAR(uint16_t*, ms_poiNext, 0x00BC5D08);
GAME_VAR(uint16_t, ms_poiTail, 0x00BC5D0C);
GAME_VAR(uint8_t*, ms_pedPopulation, 0x00C2C108);
GAME_VAR(uint8_t, ms_bPopulationOff, 0x00C2C10C);
GAME_VAR(uint8_t*, ms_poiManager, 0x00C0F5E4);
GAME_VAR(int32_t, ms_areaIndex, 0x020C3638);   // the POIs' per-area flag (+0x88 on)
GAME_VAR(int32_t, ms_checkHeight, 0x00BD1008);
GAME_VAR(uint8_t, ms_datLoad, 0x020C7E94);
GAME_VAR(uint8_t*, ms_pPlayerPed, 0x00C1AEA8);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const CStaticPOI_vftable = reinterpret_cast<void*>(0x00901858);
// the POIList file's keys
GAME_VAR(const char*, key_name, 0x00A1456C);
GAME_VAR(const char*, key_listValue, 0x00A14570);
GAME_VAR(const char*, key_count, 0x00A14574);
GAME_VAR(const char*, key_74, 0x00A14578);
GAME_VAR(const char*, key_70, 0x00A1457C);
GAME_VAR(const char*, key_78, 0x00A14580);
GAME_VAR(const char*, key_slots, 0x00A14584);
GAME_VAR(const char*, key_position, 0x00A14588);
GAME_VAR(const char*, key_rotation, 0x00A1458C);
GAME_ARRAY(const char*, key_flags, 0x00A14590);   // 10 bool keys
GAME_VAR(const char*, key_A2, 0x00A145B8);
GAME_VAR(const char*, key_9C, 0x00A145BC);
GAME_VAR(const char*, key_A3, 0x00A145C0);
GAME_VAR(const char*, key_A1, 0x00A145C4);

GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t*, __cdecl, POIPoint_New, 0x004583E0, size_t size);
GAME_FN(uint8_t*, __cdecl, DynamicPOI_New, 0x00429460, size_t size);
GAME_FN(uint8_t*, __thiscall, DynamicPOI_Construct, 0x00429330, void* self, float time);
GAME_FN(void, __thiscall, DynamicPOI_Update, 0x00429370, void* self, float step);
GAME_FN(void, __thiscall, DynamicPOI_GroupFn, 0x00429360, void* self);
GAME_FN(uint8_t*, __cdecl, POIGroup_New, 0x0048A100, size_t size);
GAME_FN(uint8_t*, __thiscall, POIGroup_Construct, 0x0048A080, void* self, const col::CVector* centre, float heading,
        int spots, int b, int c, float radius);
GAME_FN(int32_t, __cdecl, ParseKind_488BB0, 0x00488BB0, const char* s);
GAME_FN(int32_t, __cdecl, ParseKind_488B90, 0x00488B90, const char* s);
GAME_FN(int32_t, __cdecl, ParseKind_488B10, 0x00488B10, const char* s);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float angle);
GAME_FN(col::CVector*, __cdecl, VectorSub, 0x00414B70, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(int, __thiscall, Camera_GetFadeState, 0x004F12F0, void* camera);
GAME_FN(uint8_t, __thiscall, Camera_IsSphereVisible, 0x004F13D0, void* camera, const col::CVector* centre, float radius);
GAME_FN(void, __thiscall, Ped_Fn_479590, 0x00479590, void* self);
GAME_FN(uint8_t, __cdecl, Fn_4469A0, 0x004469A0);
GAME_FN(float, __cdecl, GetTimeStepSeconds, 0x0045B840);
// POIs.cpp's (through the game)
GAME_FN(uint8_t*, __thiscall, POI_Construct, 0x00447650, uint8_t* poi);
GAME_FN(void, __thiscall, POI_ReleaseAll, 0x00447620, uint8_t* poi);
GAME_FN(void, __thiscall, POI_RemoveGroup, 0x004474C0, uint8_t* poi);
GAME_FN(int, __thiscall, POI_CountPeds, 0x00447500, uint8_t* poi);
GAME_FN(void, __thiscall, POI_SetGroup, 0x00447870, uint8_t* poi, uint8_t* group);
GAME_FN(void, __thiscall, POI_SetDisabled, 0x004478F0, uint8_t* poi, int disabled);
GAME_FN(uint8_t, __thiscall, POI_AddPed, 0x00447410, uint8_t* poi, void* ped, int flag);
GAME_FN(uint8_t*, __thiscall, POI_Assign, 0x004484C0, uint8_t* poi, uint8_t* other);
GAME_FN(void, __thiscall, POIList_Clear, 0x00447C20, uint8_t* list);
GAME_FN(uint8_t*, __thiscall, POIList_First, 0x00447B90, uint8_t* list);
GAME_FN(uint8_t*, __thiscall, POIList_Next, 0x00447BD0, uint8_t* list);
GAME_FN(int, __thiscall, POIList_Count, 0x00447AC0, uint8_t* list);
GAME_FN(void, __thiscall, Manager_RemoveEntry, 0x004482C0, uint8_t* manager, uint32_t index);
// (ours, called where modes apply)
GAME_FN(uint8_t, __thiscall, Manager_InRange, 0x00448590, void* manager, uint8_t* poi, float x, float y, float z);
GAME_FN(uint8_t, __thiscall, Manager_OutOfRange, 0x004487E0, void* manager, uint8_t* poi, float x, float y, float z);
GAME_FN(uint8_t*, __thiscall, ManagedPOI_Assign, 0x00448B00, uint8_t* poi, uint8_t* other);
GAME_FN(uint8_t, __thiscall, Manager_TakeOver, 0x00449020, uint8_t* manager, uint8_t* poi);

using re::At;
template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

col::CVector* EntityPos(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<col::CVector*>(matrix + 0x30) : reinterpret_cast<col::CVector*>(entity + 4);
}

// the heading of a matrix (0x85CF1A: the CRT's x87 atan2)
float MatrixHeading(const uint8_t* matrix) {
    return float(std::atan2(-double(*reinterpret_cast<const float*>(matrix + 0x10)), double(*reinterpret_cast<const float*>(matrix + 0x14))));
}

float Distance2D(const col::CVector& d) {
    float sum = float(double(d.x) * d.x + double(d.y) * d.y);
    return float(std::sqrt(double(std::fabs(sum))));   // (0x85C500: the CRT's x87 sqrt)
}

uint8_t* PoolSlot(CPool<uint8_t>* pool, int index) {
    return (pool->flags[index] & 0x80) ? nullptr : pool->objects + pool->elemSize * index;
}

int PoiIndex(const uint8_t* poi) {
    return int((poi - ms_poiPool->objects) / ms_poiPool->elemSize);
}

// a string as the archive reads it (a pointer, a capacity, the characters)
template <int N>
struct FixedString {
    re::Ptr32<char> text;
    int32_t capacity;
    char data[N];

    FixedString() : text(data), capacity(N) { data[0] = 0; }
};

// the group of a POI (made from its centre and heading), onto it
uint8_t* MakeGroup(uint8_t* poi, uint8_t* source, int spots, int b, int c) {
    uint8_t* group = POIGroup_New(0x110);
    if (!group) {
        return nullptr;
    }
    uint8_t* matrix = At<uint8_t*>(source, 0x14);
    float heading = matrix ? MatrixHeading(matrix) : At<float>(source, 0x10);
    col::CVector* centre = matrix ? reinterpret_cast<col::CVector*>(matrix + 0x30) : reinterpret_cast<col::CVector*>(source + 4);
    (void)poi;
    return POIGroup_Construct(group, centre, heading, spots, b, c, 0.0f);
}

} // namespace

namespace POIs2 {

// POIList vf1 (0x447D20): loaded: its name's hash, a value, then its points (CStaticPOIs
// from the POI pool) chained on to the list's end
static void __fastcall POIList_Load(uint8_t* self, re::Edx, void* archive) {
    POIList_Clear(self);
    self[0xC] = ms_datLoad;
    FixedString<0x21> name;
    VCall<void>(archive, 0x24, &name, 0x20, key_name);
    At<uint32_t>(self, 4) = HashName(name.text);
    VCall<uint8_t>(archive, 0x38, self + 8, key_listValue);
    int32_t count;
    count = VCall<uint8_t>(archive, 0x38, &count, key_count) ? count : 0;
    // (read into the same stack words on every point, never reset before the reads)
    int32_t slots = 0;
    uint8_t flag = 0;
    for (; count > 0; count--) {
        uint8_t* poi = POIPoint_New(0xA4);
        if (poi) {
            POI_Construct(poi);
            At<void*>(poi, 0) = CStaticPOI_vftable;
            poi[0xA0] = 0xFF;
        } else {
            poi = nullptr;
        }
        FixedString<0x19> text;
        VCall<void>(archive, 0x24, &text, 0x18, key_74);
        if (text.text[0] != '\0') {
            At<int32_t>(poi, 0x74) = ParseKind_488BB0(text.text);
        }
        VCall<void>(archive, 0x24, &name, 0x20, key_name);
        At<uint32_t>(poi, 0x84) = HashName(name.text);
        VCall<void>(archive, 0x24, &text, 0x18, key_70);
        if (text.text[0] != '\0') {
            At<int32_t>(poi, 0x70) = ParseKind_488B90(text.text);
        }
        VCall<void>(archive, 0x24, &text, 0x18, key_78);
        if (text.text[0] != '\0') {
            At<int32_t>(poi, 0x78) = ParseKind_488B10(text.text);
        }
        VCall<uint8_t>(archive, 0x38, &slots, key_slots);   // (used even when missing)
        poi[0x81] = slots <= 0 ? 0 : uint8_t(slots);
        col::CVector v = {0.0f, 0.0f, 0.0f};
        VCall<uint8_t>(archive, 0x20, &v, key_position);
        if (uint8_t* matrix = At<uint8_t*>(poi, 0x14)) {
            At<float>(matrix, 0x30) = v.x;
            At<float>(matrix, 0x34) = v.y;
            At<float>(matrix, 0x38) = v.z;
            // (and +0x3C from an uninitialised stack word)
        }
        At<float>(poi, 4) = v.x;
        At<float>(poi, 8) = v.y;
        At<float>(poi, 0xC) = v.z;
        VCall<uint8_t>(archive, 0x20, &v, key_rotation);
        float heading = float(double(v.x) * 3.1415927410125732 / 180.0);
        if (uint8_t* matrix = At<uint8_t*>(poi, 0x14)) {
            Matrix_SetRotateZOnly(matrix, heading);
        }
        At<float>(poi, 0x10) = heading;
        At<float>(poi, 0x68) = float(double(v.x) * 3.1415927410125732 / 180.0);
        static const uint32_t flagOffsets[8] = {0x8E, 0x92, 0x90, 0x91, 0x93, 0x8A, 0x8F, 0x8D};
        for (int i = 0; i < 8; i++) {
            VCall<uint8_t>(archive, 0x3C, &flag, key_flags[i]);
            poi[flagOffsets[i]] = flag;
        }
        VCall<uint8_t>(archive, 0x3C, &flag, key_flags[8]);
        poi[0x8B] = flag;
        poi[0x8C] = flag;
        VCall<uint8_t>(archive, 0x3C, &flag, key_flags[9]);
        poi[0x88] = flag;
        poi[0x89] = flag;
        float f = 0.0f;
        uint8_t b = 0;
        VCall<uint8_t>(archive, 0x3C, &b, key_A2);
        poi[0xA2] = b;
        VCall<uint8_t>(archive, 0x28, &f, key_9C);
        At<float>(poi, 0x9C) = f;
        uint8_t b3 = 0;
        VCall<uint8_t>(archive, 0x3C, &b3, key_A3);
        poi[0xA3] = b3;
        int32_t value = 0;
        VCall<uint8_t>(archive, 0x38, &value, key_A1);
        poi[0xA1] = uint8_t(value);
        uint16_t index = uint16_t(PoiIndex(poi));
        if (At<uint16_t>(self, 0x16) == 0xFFFF) {
            ms_poiNext[index] = 0xFFFE;
            ms_poiTail = index;
            At<uint16_t>(self, 0x16) = index;
        } else {
            ms_poiNext[ms_poiTail] = index;
            ms_poiNext[index] = 0xFFFE;
            ms_poiTail = index;
        }
    }
}

// 0x448590 (thiscall on the POI manager): the POI in range of the point for peds: on in
// this area, within +0x7870 in height (when checked, unless it has its own radius), within
// +0x7830 (2D) and, closer than +0x782C, off screen (unless fading); or within its own
// radius (+0x9C, flag +0xA2)
static uint8_t __fastcall InRange(void*, re::Edx, uint8_t* poi, float x, float y, float z) {
    if (ms_bPopulationOff) {
        return 0;
    }
    col::CVector p = *EntityPos(poi);
    float radius = At<float>(ms_pedPopulation, 0x7830);
    uint8_t own = poi[0xA2];
    if (own) {
        radius = At<float>(poi, 0x9C);
    }
    if (!poi[0x88 + ms_areaIndex]) {
        return 0;
    }
    if (ms_checkHeight && !own) {
        float dz = std::fabs(float(double(p.z) - z));
        if (At<float>(ms_pedPopulation, 0x7870) < dz) {
            return 0;
        }
    }
    col::CVector point = {x, y, z};
    col::CVector d;
    float dist = Distance2D(*VectorSub(&d, &point, &p));
    if (own) {
        return radius > dist;
    }
    if (!(At<float>(ms_pedPopulation, 0x7830) > dist)) {
        return 0;
    }
    if (Camera_GetFadeState(TheCamera) != 0) {
        return 1;
    }
    col::CVector centre = p;
    if (At<float>(ms_pedPopulation, 0x782C) > dist && Camera_IsSphereVisible(TheCamera, &centre, 0.25f)) {
        return 0;
    }
    return 1;
}

// 0x4487E0 (thiscall on the POI manager): a disabled POI now out of range (+0x7828, or
// its own radius plus 10); 1 while the manager is off. (Kind 3 POIs with peds stay)
static uint8_t __fastcall OutOfRange(void*, re::Edx, uint8_t* poi, float x, float y, float z) {
    if (!ms_poiManager[0xA4] || !ms_poiManager[0xA5]) {
        return 1;
    }
    col::CVector p = *EntityPos(poi);
    uint8_t own = poi[0xA2];
    float radius = At<float>(ms_pedPopulation, 0x7830);
    if (own) {
        radius = float(double(At<float>(poi, 0x9C)) + 10.0);
    }
    if (!poi[0x82]) {
        return 0;
    }
    if (At<int32_t>(poi, 0x70) == 3 && POI_CountPeds(poi) > 0) {
        return 0;
    }
    if (!poi[0x88 + ms_areaIndex]) {
        return 1;
    }
    if (ms_checkHeight && !own) {
        float dz = std::fabs(float(double(p.z) - z));
        if (At<float>(ms_pedPopulation, 0x7870) < dz) {
            return 1;
        }
    }
    col::CVector point = {x, y, z};
    col::CVector d;
    float dist = Distance2D(*VectorSub(&d, &point, &p));
    if (!own) {
        return At<float>(ms_pedPopulation, 0x7828) < dist;
    }
    return radius < dist;
}

// the peds of the pool at 0xC0F5F0 through 0x479590, last first
static void ProcessPool_C0F5F0() {
    CPool<uint8_t>* pool = ms_pool_C0F5F0;
    int i = pool->size;
    while (i != 0) {
        i--;
        if (uint8_t* object = PoolSlot(pool, i)) {
            Ped_Fn_479590(object);
        }
    }
}

// 0x448990 (thiscall; unused): the POILists of the area code (+0xC) cleared and deleted
// (those of 0x5DB960's +0x44, or all when forced), then 0x479590 on 0xC0F5F0's pool
static void __fastcall RemoveListsOfArea(void*, re::Edx, int area, int force) {
    uint8_t* owner = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(0x005DB960)(reinterpret_cast<void*>(0x00D02850));
    int32_t value = owner ? At<int32_t>(owner, 0x44) : -2;
    int size = ms_poiListPool->size;
    for (int i = 0; i < size; i++) {
        uint8_t* list = PoolSlot(ms_poiListPool, i);
        if (!list || int8_t(list[0xC]) != area) {
            continue;
        }
        if (At<int32_t>(list, 0x10) != value && !uint8_t(force)) {
            continue;
        }
        POIList_Clear(list);
        VCall<void>(list, 0, 1);
    }
    ProcessPool_C0F5F0();
}

// 0x448A40 (thiscall; unused): the POILists with that +0x14 cleared and deleted, then
// 0x479590 on 0xC0F5F0's pool
static void __fastcall RemoveListsOf14(void*, re::Edx, int value) {
    int size = ms_poiListPool->size;
    uint8_t b = uint8_t(value);
    for (int i = 0; i < size; i++) {
        uint8_t* list = PoolSlot(ms_poiListPool, i);
        if (!list || list[0x14] != b) {
            continue;
        }
        POIList_Clear(list);
        VCall<void>(list, 0, 1);
    }
    ProcessPool_C0F5F0();
}

// 0x448AD0 (thiscall; unused): a POIList by slot
static uint8_t* __fastcall GetList(void*, re::Edx, int index) {
    return PoolSlot(ms_poiListPool, index);
}

// 0x448B00 (thiscall): the manager's POI's assignment (CPOI's, then +0x84 .. +0xA3)
static uint8_t* __fastcall ManagedPOI_AssignImpl(uint8_t* self, re::Edx, uint8_t* other) {
    POI_Assign(self, other);
    At<int32_t>(self, 0x84) = At<int32_t>(other, 0x84);
    for (int i = 0; i < 0x14; i++) {
        self[0x88 + i] = other[0x88 + i];
    }
    At<float>(self, 0x9C) = At<float>(other, 0x9C);
    self[0xA0] = other[0xA0];
    self[0xA1] = other[0xA1];
    self[0xA2] = other[0xA2];
    self[0xA3] = other[0xA3];
    return self;
}

// 0x448B90 (thiscall; unused): a random POI (of a random list, from a random point, round
// each once) within the radius (2D) and +0x7870 in height of the ped that takes it: the
// ped added (flagged)
static uint8_t __fastcall FindPOIForPed(void*, re::Edx, uint8_t* ped, float radius) {
    if (!ped) {
        return 0;
    }
    col::CVector p = *EntityPos(ped);
    float radiusSq = radius * radius;
    int lists = ms_poiListPool->size;
    int index = Rand() * (lists - 1) / 0x7FFF;
    if (lists == 0) {
        return 0;
    }
    for (int left = lists; left != 0;) {
        left--;
        uint8_t* list = PoolSlot(ms_poiListPool, index);
        index++;
        if (index >= lists) {
            index = 0;
        }
        if (!list) {
            continue;
        }
        uint8_t* poi = POIList_First(list);
        int count = POIList_Count(list);
        int skip = Rand() * (count - 1) / 0x7FFF;
        int skipped = 0;
        if (skip > 0) {
            skipped = skip;
            for (int i = skip; i != 0; i--) {
                poi = POIList_Next(list);
            }
        }
        int remaining = count - skipped;
        while (poi) {
            if (remaining-- == 0) {
                break;
            }
            uint8_t* next = POIList_Next(list);
            col::CVector d;
            VectorSub(&d, EntityPos(poi), &p);
            float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
            if (radiusSq > distSq) {
                float dz = std::fabs(float(double(EntityPos(poi)->z) - p.z));
                if (At<float>(ms_pedPopulation, 0x7870) >= dz && VCall<uint8_t>(poi, 4, ped)) {   // (NaN: no)
                    POI_AddPed(poi, ped, 1);
                    return 1;
                }
            }
            poi = next;
            if (!poi) {
                poi = POIList_First(list);
            }
        }
    }
    return 0;
}

// 0x448DC0 (thiscall): a POI by its pool slot (-2: the manager's own)
static uint8_t* __fastcall Manager_GetPOI(uint8_t* self, re::Edx, int index) {
    if (index == -2) {
        return self + 0xA8;
    }
    return PoolSlot(ms_poiPool, index);
}

// 0x448E00 (thiscall): a dynamic POI (a free entry's reused, else a new one) at the
// point, turned, with its group (of 2 spots), disabled. (With no group the game writes
// its POI to the null group's +0x40 and crashes)
static uint8_t* __fastcall Manager_AddDynamicPOI(uint8_t* self, re::Edx, float time, float x, float y, float z,
                                                 float heading) {
    uint8_t* poi = nullptr;
    uint32_t count = At<uint32_t>(self, 0xA0);
    uint32_t i = 0;
    for (; i < count; i++) {
        if (At<void*>(self, i * 8 + 4) && self[i * 8] == 1) {
            break;
        }
    }
    if (i < count) {
        poi = At<uint8_t*>(self, i * 8 + 4);
        self[i * 8] = 0;
        At<float>(poi, 0xA4) = time;
    } else {
        uint8_t* memory = DynamicPOI_New(0xA8);
        poi = memory ? DynamicPOI_Construct(memory, time) : nullptr;
        uint32_t n = At<uint32_t>(self, 0xA0);
        self[n * 8] = 1;   // (the entry's other three bytes from an uninitialised stack word)
        At<uint8_t*>(self, n * 8 + 4) = poi;
        At<uint32_t>(self, 0xA0) = n + 1;
        self[(n + 1) * 8 - 8] = 0;
    }
    if (uint8_t* matrix = At<uint8_t*>(poi, 0x14)) {
        At<float>(matrix, 0x30) = x;
        At<float>(matrix, 0x34) = y;
        At<float>(matrix, 0x38) = z;
        // (and +0x3C from an uninitialised stack word)
    }
    At<float>(poi, 4) = x;
    At<float>(poi, 8) = y;
    At<float>(poi, 0xC) = z;
    if (uint8_t* matrix = At<uint8_t*>(poi, 0x14)) {
        Matrix_SetRotateZOnly(matrix, heading);
    }
    At<float>(poi, 0x10) = heading;
    uint8_t* group = MakeGroup(poi, poi, 2, At<int32_t>(poi, 0x78), At<int32_t>(poi, 0x74));
    re::StoreVolatile(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(group) + 0x40), poi);
    POI_SetGroup(poi, group);
    POI_SetDisabled(poi, 1);
    return poi;
}

// 0x448F80 (thiscall; unused): the pool slot of the named POI (a list of that name: its
// first point), -1 when none
static int __fastcall FindPOIByName(void*, re::Edx, const char* name) {
    uint32_t hash = HashName(name);
    for (int i = ms_poiListPool->size - 1; i >= 0; i--) {
        uint8_t* list = PoolSlot(ms_poiListPool, i);
        if (!list) {
            continue;
        }
        if (hash == At<uint32_t>(list, 4)) {
            return PoiIndex(POIList_First(list));
        }
        uint8_t* poi = POIList_First(list);
        while (poi) {
            uint8_t* next = POIList_Next(list);
            if (At<uint32_t>(poi, 0x84) == hash) {
                return PoiIndex(poi);
            }
            poi = next;
        }
    }
    return -1;
}

// 0x449020 (thiscall): the manager's own POI as a copy of the given one (its group made
// anew), unless locked (+0x14C); with none only the own POI's peds and group dropped
static uint8_t __fastcall TakeOver(uint8_t* self, re::Edx, uint8_t* source) {
    uint8_t* own = self + 0xA8;
    if (!source && At<void*>(self, 0x114)) {
        POI_ReleaseAll(own);
        POI_RemoveGroup(own);
    }
    if (At<int32_t>(self, 0x14C) != 0) {
        return 0;
    }
    POI_ReleaseAll(own);
    POI_RemoveGroup(own);
    ManagedPOI_Assign(own, source);
    uint8_t* group = MakeGroup(own, source, int8_t(source[0x81]), At<int32_t>(source, 0x78), At<int32_t>(source, 0x74));
    re::StoreVolatile(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(group) + 0x40), own);
    POI_SetGroup(own, group);
    return 1;
}

// 0x449140 (thiscall): a POI for the player (a random one as 0x448B90 picks, of the kind
// unless 0x10, kinds 3 to 15, in range): taken over (-2) for kind 3, else disabled; its
// pool slot, -1 when none
static int __fastcall FindPOIForPlayer(uint8_t* self, re::Edx, int kind) {
    col::CVector p = *EntityPos(ms_pPlayerPed);
    bool filter = kind != 0x10;
    bool match = true;
    int lists = ms_poiListPool->size;
    int index = Rand() * (lists - 1) / 0x7FFF;
    if (lists == 0) {
        return -1;
    }
    for (int left = lists; left != 0;) {
        left--;
        uint8_t* list = PoolSlot(ms_poiListPool, index);
        index++;
        if (index >= lists) {
            index = 0;
        }
        if (!list) {
            continue;
        }
        uint8_t* poi = POIList_First(list);
        int count = POIList_Count(list);
        int skip = Rand() * (count - 1) / 0x7FFF;
        int skipped = 0;
        if (skip > 0) {
            skipped = skip;
            for (int i = skip; i != 0; i--) {
                poi = POIList_Next(list);
            }
        }
        int remaining = count - skipped;
        while (poi) {
            if (remaining-- == 0) {
                break;
            }
            uint8_t* next = POIList_Next(list);
            if (filter && kind < 0x10) {
                match = At<int32_t>(poi, 0x70) == kind;
            }
            int poiKind = At<int32_t>(poi, 0x70);
            uint8_t* manager = ms_poiManager;
            if (match && poiKind > 2 && poiKind < 0x10 && manager[0xA4] && manager[0xA5] &&
                (!poi[0x82] || (poiKind == 3 && At<int32_t>(manager, 0x14C) == 0)) &&
                Manager_InRange(manager, poi, p.x, p.y, p.z)) {
                if (At<int32_t>(poi, 0x70) == 3 && At<int32_t>(self, 0x14C) == 0 && Manager_TakeOver(self, poi)) {
                    return -2;
                }
                POI_SetDisabled(poi, 1);
                return PoiIndex(poi);
            }
            poi = next;
            if (!poi) {
                poi = POIList_First(list);
            }
        }
    }
    return -1;
}

// 0x449390 (thiscall): the POI manager's frame: on unless 0x4469A0; the dynamic POIs'
// timers (out of time: their groups and themselves deleted, the entry removed); the
// disabled POIs out of the player's range enabled again; the own POI disabled while it
// has peds (or given up when it has none)
static void __fastcall Update(uint8_t* self, re::Edx) {
    self[0xA5] = Fn_4469A0() == 0;
    col::CVector p = *EntityPos(ms_pPlayerPed);
    float step = GetTimeStepSeconds();
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xA0); i++) {
        if (self[i * 8] != 0) {
            continue;
        }
        uint8_t* poi = At<uint8_t*>(self, i * 8 + 4);
        DynamicPOI_Update(poi, step);
        poi = At<uint8_t*>(self, i * 8 + 4);
        if (!(0.0f > At<float>(poi, 0xA4))) {
            continue;
        }
        At<float>(poi, 0xA4) = 0.0f;
        self[i * 8] = 1;
        DynamicPOI_GroupFn(At<uint8_t*>(self, i * 8 + 4));
        POI_RemoveGroup(At<uint8_t*>(self, i * 8 + 4));
        if (uint8_t* object = At<uint8_t*>(self, i * 8 + 4)) {
            VCall<void>(object, 0, 1);
        }
        At<void*>(self, i * 8 + 4) = nullptr;
        Manager_RemoveEntry(self, i);
    }
    for (int i = 0; i < ms_poiListPool->size; i++) {
        uint8_t* list = PoolSlot(ms_poiListPool, i);
        if (!list) {
            continue;
        }
        uint8_t* poi = POIList_First(list);
        while (poi) {
            uint8_t* next = POIList_Next(list);
            int kind = At<int32_t>(poi, 0x70);
            if (kind > 2 && kind < 0x10 && Manager_OutOfRange(ms_poiManager, poi, p.x, p.y, p.z)) {
                POI_SetDisabled(poi, 0);
            }
            poi = next;
        }
    }
    if (At<uint32_t>(self, 0x14C) == 0) {
        return;
    }
    uint8_t* own = self + 0xA8;
    if (self[0x12A]) {
        if (POI_CountPeds(own) == 0) {
            Manager_TakeOver(self, nullptr);
            return;
        }
        if (self[0x12A]) {
            return;
        }
    }
    if (POI_CountPeds(own) > 0) {
        POI_SetDisabled(own, 1);
    }
}

} // namespace POIs2

// ---- registration ----------------------------------------------------------------------

#define C03F_POIS2_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace POIs2 {
C03F_POIS2_REVERSED("POIList::Load", 0x00447D20, &POIList_Load);
C03F_POIS2_REVERSED("CPOIManager::InRange", 0x00448590, &InRange);
C03F_POIS2_REVERSED("CPOIManager::OutOfRange", 0x004487E0, &OutOfRange);
C03F_POIS2_REVERSED("CPOIManager::RemoveListsOfArea", 0x00448990, &RemoveListsOfArea);
C03F_POIS2_REVERSED("CPOIManager::RemoveListsOf14", 0x00448A40, &RemoveListsOf14);
C03F_POIS2_REVERSED("CPOIManager::GetList", 0x00448AD0, &GetList);
C03F_POIS2_REVERSED("CManagedPOI::Assign", 0x00448B00, &ManagedPOI_AssignImpl);
C03F_POIS2_REVERSED("CPOIManager::FindPOIForPed", 0x00448B90, &FindPOIForPed);
C03F_POIS2_REVERSED("CPOIManager::GetPOI", 0x00448DC0, &Manager_GetPOI);
C03F_POIS2_REVERSED("CPOIManager::AddDynamicPOI", 0x00448E00, &Manager_AddDynamicPOI);
C03F_POIS2_REVERSED("CPOIManager::FindPOIByName", 0x00448F80, &FindPOIByName);
C03F_POIS2_REVERSED("CPOIManager::TakeOver", 0x00449020, &TakeOver);
C03F_POIS2_REVERSED("CPOIManager::FindPOIForPlayer", 0x00449140, &FindPOIForPlayer);
C03F_POIS2_REVERSED("CPOIManager::Update", 0x00449390, &Update);
} // namespace POIs2
