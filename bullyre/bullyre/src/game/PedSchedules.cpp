// The ambient peds' schedules (a manager at 0xC2AB68): each schedule (0x10C, in the pool
// at 0x20C7C68) has up to ten time periods of the day and a list of route starts (0xC, in
// the pool at 0x20C7C70); a ped is given a route start of a schedule active now, within
// the schedule's distances of it (and optionally in front of it).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Entity.h"
#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using col::CVector;

struct PedSchedule;

// a route's start (0xC)
struct RouteStart {
    int32_t route;         // +0 the route's index (0xBC5E7C's)
    int32_t slot;          // +4 an object in the pool at 0x20C7C48 (-1: none)
    re::Ptr32<PedSchedule> owner;    // +8

    uint8_t IsValid();
    CVector* GetPosition(CVector* out);
    void* GetObject();
    uint8_t CanUse(CEntity* ped, float minDist, float maxDist, int checkHeight, int checkFacing);
};
static_assert(sizeof(RouteStart) == 0xC);

struct TimePeriod {
    int32_t start;  // minutes of the day
    int32_t end;
};

struct PedSchedule {
    re::Ptr32<RouteStart> starts[40];  // +0
    uint32_t numStarts;      // +0xA0
    TimePeriod periods[10];  // +0xA4
    int32_t numPeriods;      // +0xF4
    int32_t tag;             // +0xF8
    uint8_t flags;           // +0xFC 1 active, 2 males, 4 females (a model's +0x50 is "female"), 8 (a +0x1570 4 ped too), 0x10 ped states 1-6, 11, 13 only,
                             //       0x20 ending soon, 0x40 height check, 0x80 facing check
    uint8_t forced;          // +0xFD 1: forced, 2: the forced value
    int8_t warnMinutes;      // +0xFE
    uint8_t unknownFF;       // +0xFF
    uint8_t unknown100;      // +0x100
    uint8_t pad101[3];
    float minDist;           // +0x104
    float maxDist;           // +0x108

    uint8_t IsActive();
    PedSchedule* Init(int a, int b, int tag_);
    void AddPeriod(int start, int length);
    void AddStart(int route, int slot);
    void ClearStarts();
    RouteStart* PickStart(CEntity* ped);
    RouteStart* FindNearest(CEntity* ped, float* outDist);
};
static_assert(offsetof(PedSchedule, numStarts) == 0xA0);
static_assert(offsetof(PedSchedule, numPeriods) == 0xF4);
static_assert(offsetof(PedSchedule, flags) == 0xFC);
static_assert(sizeof(PedSchedule) == 0x10C);

struct PedScheduleManager {
    re::Ptr32<PedSchedule> schedules[40];  // +0
    uint32_t count;              // +0xA0

    uint8_t IsStartActive(RouteStart* start);
    PedSchedule* AddSchedule(int a, int b, int tag);
    PedSchedule* Find(PedSchedule* schedule);
    RouteStart* Pick(CEntity* ped);
    RouteStart* FindNearest(CEntity* ped, float* outDist);
    void RemoveTagged(int tag);
    void Clear();
};
static_assert(sizeof(PedScheduleManager) == 0xA4);

GAME_VAR(PedScheduleManager*, s_manager, 0x00C2AB68);
GAME_VAR(CPool<uint8_t>*, s_schedulePool, 0x020C7C68);
GAME_VAR(CPool<uint8_t>*, s_startPool, 0x020C7C70);
GAME_VAR(CPool<uint8_t>*, s_objectPool, 0x020C7C48);
GAME_VAR(uint8_t, s_clockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t, s_clockHours, 0x00BCEB4B);
inline void* const s_routes = reinterpret_cast<void*>(0x00BC5E7C);

GAME_FN(uint8_t, __thiscall, TimePeriod_Contains, 0x0045B870, const TimePeriod* period, int minute);
GAME_FN(void*, __thiscall, Routes_Get, 0x006D6450, void* routes, int index);
GAME_FN(float*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(uint8_t*, __cdecl, Object_GetProp, 0x004EA3C0, void* object);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, CEntity* entity, CVector* out);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, CEntity* ped);
inline int __cdecl Rand() { return crt::Rand(); }
inline void* __cdecl Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(RouteStart*, __thiscall, StartPool_New, 0x004F02E0, CPool<uint8_t>* pool);
// (their own, called where they are)
GAME_FN(uint8_t, __thiscall, Schedule_IsActive, 0x0046E170, PedSchedule* schedule);
GAME_FN(PedSchedule*, __thiscall, Schedule_Init, 0x0046E220, PedSchedule* schedule, int a, int b, int tag);
GAME_FN(uint8_t*, __thiscall, SchedulePool_New, 0x0046E310, CPool<uint8_t>* pool);
GAME_FN(uint8_t, __thiscall, Start_CanUse, 0x0046E460, RouteStart* start, CEntity* ped, float minDist, float maxDist, int checkHeight,
        int checkFacing);
GAME_FN(RouteStart*, __thiscall, Schedule_PickStart, 0x0046E640, PedSchedule* schedule, CEntity* ped);
GAME_FN(RouteStart*, __thiscall, Schedule_FindNearest, 0x0046E740, PedSchedule* schedule, CEntity* ped, float* outDist);
GAME_FN(void, __thiscall, Schedule_ClearStarts, 0x0046EB70, PedSchedule* schedule);
GAME_FN(void, __thiscall, Manager_Clear, 0x0046EC30, PedScheduleManager* manager);

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

// (inlined) CPool::Delete
void PoolDelete(CPool<uint8_t>* pool, void* object) {
    int slot = int((static_cast<uint8_t*>(object) - pool->objects) / pool->elemSize);
    pool->flags[slot] |= 0x80;
    if (slot < pool->lastUsed) {
        pool->lastUsed = slot;
    }
}

void* PoolObject(int slot) {
    if (slot < 0) {
        return nullptr;
    }
    CPool<uint8_t>* pool = s_objectPool;
    if (pool->IsFreeSlot(slot)) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * slot;
}

// (the gates PickStart and FindNearest share) the ped's state and gender allowed
bool AllowsPed(PedSchedule* s, CEntity* ped) {
    if (s->flags & 0x10) {
        int state = Field<int32_t>(ped, 0x1310);
        if (!((state > 0 && state < 7) || state == 11 || state == 13)) {
            return false;
        }
    }
    if (Field<uint8_t>(ped, 0x1570) == 4 && !(s->flags & 8)) {
        return false;
    }
    if (Ped_GetModelInfo(ped)[0x50]) {
        return (s->flags & 4) != 0;
    }
    return (s->flags & 2) != 0;
}

// 0x46E170 (thiscall): one of its periods covers the time now (or its forced value);
// sets +0xFC's 1, and 0x20 when the period ends within warnMinutes
uint8_t PedSchedule::IsActive() {
    if (forced & 1) {
        return (forced >> 1) & 1;
    }
    int minute = s_clockHours * 60 + s_clockMinutes;
    int i = numPeriods - 1;
    for (; i >= 0; i--) {
        if (TimePeriod_Contains(&periods[i], minute)) {
            break;
        }
    }
    if (i < 0) {
        if (flags & 1) {
            flags &= 0xDE;
        }
        return 0;
    }
    if (!(flags & 1)) {
        flags |= 1;
    }
    if (!(flags & 0x20) && periods[i].end - minute < warnMinutes) {
        flags |= 0x20;
    }
    return 1;
}

// 0x46E220 (thiscall; a and b unused)
PedSchedule* PedSchedule::Init(int, int, int tag_) {
    numStarts = 0;
    numPeriods = 0;
    minDist = 1.0f;
    tag = tag_;
    maxDist = 30.0f;
    flags = (flags & 0xC6) | 0x46;
    warnMinutes = 0;
    unknownFF = 0;
    unknown100 = 100;
    return this;
}

// 0x46E280 (thiscall): from start for length minutes (wrapping past midnight)
void PedSchedule::AddPeriod(int start, int length) {
    int end = start + length;
    if (end > 1440) {
        end -= 1440;
    }
    periods[numPeriods].start = start;
    periods[numPeriods].end = end;
    numPeriods++;
}

// 0x46E2C0 (thiscall): the schedule that has this start is active
uint8_t PedScheduleManager::IsStartActive(RouteStart* start) {
    for (int i = int(count) - 1; i >= 0; i--) {
        PedSchedule* s = schedules[i];
        for (uint32_t j = 0; j < s->numStarts; j++) {
            if (s->starts[j] == start) {
                return Schedule_IsActive(s);
            }
        }
    }
    return 0;
}

// 0x46E310 (thiscall): GTA's CPool::New for the schedules (cleared)
uint8_t* __fastcall SchedulePoolNew(CPool<uint8_t>* pool, re::Edx) {
    bool wrapped = false;
    if (!pool->IsFreeSlot(pool->lastUsed)) {
        do {
            if (++pool->lastUsed >= pool->size) {
                if (wrapped) {
                    return nullptr;
                }
                pool->lastUsed = 0;
                wrapped = true;
            }
        } while (!pool->IsFreeSlot(pool->lastUsed));
    }
    uint8_t& flag = pool->flags[pool->lastUsed];
    flag &= 0x7F;
    flag = uint8_t((flag & 0x80) | ((flag + 1) & 0x7F));
    uint8_t* object = pool->objects + pool->elemSize * pool->lastUsed;
    Memset(object, 0, 0x10C);
    return object;
}

// 0x46E390 (thiscall): its route exists, or its object does
uint8_t RouteStart::IsValid() {
    if (Routes_Get(s_routes, route)) {
        return 1;
    }
    return PoolObject(slot) != nullptr;
}

// 0x46E3D0 (thiscall): its route's first point (zero without one)
CVector* RouteStart::GetPosition(CVector* out) {
    void* r = Routes_Get(s_routes, route);
    if (r) {
        float* first = Route_First(r);
        out->x = first[0];
        out->y = first[1];
        out->z = first[2];
        return out;
    }
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    return out;
}

// 0x46E420 (thiscall): its object's prop (0x4EA3C0)
void* RouteStart::GetObject() {
    return Object_GetProp(PoolObject(slot));
}

// 0x46E460 (thiscall): the ped can take it: within 4 in height (checkHeight), its object
// not busy (+0x180), minDist..maxDist away, in front of it (checkFacing)
uint8_t RouteStart::CanUse(CEntity* ped, float minDist, float maxDist, int checkHeight, int checkFacing) {
    void* r = Routes_Get(s_routes, route);
    CVector start;
    if (r) {
        float* first = Route_First(r);
        start = {first[0], first[1], first[2]};
    } else {
        start = {0.0f, 0.0f, 0.0f};
    }
    CVector pos = ped->GetPosition();
    if (re::Flag(checkHeight) && std::fabs(start.z - pos.z) > 4.0f) {
        return 0;
    }
    uint8_t* prop = Object_GetProp(PoolObject(slot));
    if (prop && Field<int32_t>(prop, 0x180)) {
        return 0;
    }
    CVector d;
    VectorSub(&d, &start, &pos);
    float sq = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
    float dist = float(std::sqrt(double(std::fabs(sq))));
    if (maxDist < dist) {
        return 0;
    }
    if (minDist > dist) {
        return 0;
    }
    if (re::Flag(checkFacing)) {
        CVector dir;
        VectorSub(&dir, &start, &pos);
        dir.z = 0.0f;
        Vector_Normalise(&dir);
        CVector forward;
        Entity_GetForward(ped, &forward);
        float dot = float(double(dir.y) * forward.y + double(forward.x) * dir.x + double(forward.z) * dir.z);
        if (dot < 0.0f) {
            return 0;
        }
    }
    return 1;
}

// 0x46E640 (thiscall): a start the ped can take, from a random one on
RouteStart* PedSchedule::PickStart(CEntity* ped) {
    if (!AllowsPed(this, ped) || numStarts == 0) {
        return nullptr;
    }
    uint32_t first = uint32_t(Rand() * int(numStarts - 1) / 0x7FFF);
    uint32_t i = first;
    do {
        if (Start_CanUse(starts[i], ped, minDist, maxDist, (flags >> 6) & 1, flags >> 7)) {
            return starts[i];
        }
        if (++i >= numStarts) {
            i = 0;
        }
    } while (i != first);
    return nullptr;
}

// 0x46E740 (thiscall): the nearest free start within 4 in height (its distance out)
RouteStart* PedSchedule::FindNearest(CEntity* ped, float* outDist) {
    if (!AllowsPed(this, ped) || numStarts == 0) {
        return nullptr;
    }
    float nearest = 3.40282347e+38f;
    RouteStart* found = nullptr;
    CVector pos = ped->GetPosition();
    // (the count compared signed here: none looked at when it's negative)
    for (int32_t i = 0; i < int32_t(numStarts); i++) {
        RouteStart* s = starts[i];
        uint8_t* prop = Object_GetProp(PoolObject(s->slot));
        if (prop && Field<int32_t>(prop, 0x180)) {
            continue;
        }
        void* r = Routes_Get(s_routes, s->route);
        CVector start;
        if (r) {
            float* p = Route_First(r);
            start = {p[0], p[1], p[2]};
        } else {
            start = {0.0f, 0.0f, 0.0f};
        }
        CVector d;
        VectorSub(&d, &start, &pos);
        if (std::fabs(d.z) > 4.0f) {
            continue;
        }
        float sq = d.x * d.x + d.y * d.y + d.z * d.z;  // (in float: the main thread's x87 at 24 bits)
        if (nearest > sq) {
            nearest = sq;
            found = s;
        }
    }
    if (found) {
        *outDist = float(std::sqrt(double(std::fabs(nearest))));
    }
    return found;
}

// 0x46E940: the manager, if there's none
void __cdecl InitManager() {
    if (s_manager) {
        return;
    }
    PedScheduleManager* m = static_cast<PedScheduleManager*>(Malloc(0xA4));
    if (m) {
        m->count = 0;
        s_manager = m;
    } else {
        s_manager = nullptr;
    }
}

// 0x46E980 (thiscall): a new schedule (a null entry when the pool is full); returns it
PedSchedule* PedScheduleManager::AddSchedule(int a, int b, int tag) {
    uint8_t* object = SchedulePool_New(s_schedulePool);
    PedSchedule* schedule = object ? Schedule_Init(reinterpret_cast<PedSchedule*>(object), a, b, tag) : nullptr;
    schedules[count] = schedule;
    count++;
    return schedule;
}

// 0x46E9E0 (thiscall): the schedule, if it's one of the manager's
PedSchedule* PedScheduleManager::Find(PedSchedule* schedule) {
    for (uint32_t i = 0; i < count; i++) {
        if (schedules[i] == schedule) {
            return schedules[i];
        }
    }
    return nullptr;
}

// 0x46EA10 (thiscall): a start for the ped from an active schedule, from a random one on
RouteStart* PedScheduleManager::Pick(CEntity* ped) {
    if (count == 0) {
        return nullptr;
    }
    uint32_t first = uint32_t(Rand() * int(count - 1) / 0x7FFF);
    uint32_t i = first;
    do {
        if (Schedule_IsActive(schedules[i])) {
            RouteStart* start = Schedule_PickStart(schedules[i], ped);
            if (start) {
                return start;
            }
        }
        if (++i >= count) {
            i = 0;
        }
    } while (i != first);
    return nullptr;
}

// 0x46EA90 (thiscall): the nearest start of any schedule
RouteStart* PedScheduleManager::FindNearest(CEntity* ped, float* outDist) {
    if (count == 0) {
        return nullptr;
    }
    float nearest = 3.40282347e+38f;
    RouteStart* found = nullptr;
    for (uint32_t i = 0; i < count; i++) {
        float dist = 3.40282347e+38f;
        RouteStart* start = Schedule_FindNearest(schedules[i], ped, &dist);
        if (nearest > dist) {
            nearest = dist;
            found = start;
        }
    }
    if (found) {
        *outDist = nearest;
    }
    return found;
}

// 0x46EB20 (thiscall): a start (a null entry when the pool is full)
void PedSchedule::AddStart(int route, int slot) {
    RouteStart* start = StartPool_New(s_startPool);
    if (start) {
        start->owner = this;
        start->route = route;
        start->slot = slot;
    }
    starts[numStarts] = start;
    numStarts++;
}

// 0x46EB70 (thiscall): its starts back to their pool
void PedSchedule::ClearStarts() {
    for (uint32_t i = 0; i < numStarts; i++) {
        PoolDelete(s_startPool, starts[i]);
    }
    numStarts = 0;
}

// 0x46EBC0 (thiscall): the schedules with the tag deleted (the last moved into their place)
void PedScheduleManager::RemoveTagged(int tag) {
    for (uint32_t i = 0; i < count;) {
        PedSchedule* s = schedules[i];
        if (s->tag != tag) {
            i++;
            continue;
        }
        if (s) {
            Schedule_ClearStarts(s);
            PoolDelete(s_schedulePool, s);
        }
        schedules[i] = schedules[count - 1];
        count--;
    }
}

// 0x46EC30 (thiscall): every schedule deleted
void PedScheduleManager::Clear() {
    for (int i = int(count) - 1; i >= 0; i--) {
        PedSchedule* s = schedules[i];
        if (s) {
            Schedule_ClearStarts(s);
            PoolDelete(s_schedulePool, s);
        }
    }
    count = 0;
}

// 0x46EC80: the manager deleted and made again
void __cdecl ResetManager() {
    if (PedScheduleManager* m = s_manager) {
        Manager_Clear(m);
        Free(m);
        s_manager = nullptr;
    }
    PedScheduleManager* m = static_cast<PedScheduleManager*>(Malloc(0xA4));
    if (m) {
        m->count = 0;
        s_manager = m;
    } else {
        s_manager = nullptr;
    }
}

} // namespace

#define SCHED_C05_REVERSED(name, address, fn) REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)
#define SCHED_C05_REVERSED_FN(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

SCHED_C05_REVERSED("PedSchedule::IsActive", 0x0046E170, &PedSchedule::IsActive);
SCHED_C05_REVERSED("PedSchedule::Init", 0x0046E220, &PedSchedule::Init);
SCHED_C05_REVERSED("PedSchedule::AddPeriod", 0x0046E280, &PedSchedule::AddPeriod);
SCHED_C05_REVERSED("PedScheduleManager::IsStartActive", 0x0046E2C0, &PedScheduleManager::IsStartActive);
SCHED_C05_REVERSED_FN("CPool<PedSchedule>::New", 0x0046E310, &SchedulePoolNew);
SCHED_C05_REVERSED("RouteStart::IsValid", 0x0046E390, &RouteStart::IsValid);
SCHED_C05_REVERSED("RouteStart::GetPosition", 0x0046E3D0, &RouteStart::GetPosition);
SCHED_C05_REVERSED("RouteStart::GetObject", 0x0046E420, &RouteStart::GetObject);
SCHED_C05_REVERSED("RouteStart::CanUse", 0x0046E460, &RouteStart::CanUse);
SCHED_C05_REVERSED("PedSchedule::PickStart", 0x0046E640, &PedSchedule::PickStart);
SCHED_C05_REVERSED("PedSchedule::FindNearest", 0x0046E740, &PedSchedule::FindNearest);
SCHED_C05_REVERSED_FN("PedScheduleManager::Init", 0x0046E940, &InitManager);
SCHED_C05_REVERSED("PedScheduleManager::AddSchedule", 0x0046E980, &PedScheduleManager::AddSchedule);
SCHED_C05_REVERSED("PedScheduleManager::Find", 0x0046E9E0, &PedScheduleManager::Find);
SCHED_C05_REVERSED("PedScheduleManager::Pick", 0x0046EA10, &PedScheduleManager::Pick);
SCHED_C05_REVERSED("PedScheduleManager::FindNearest", 0x0046EA90, &PedScheduleManager::FindNearest);
SCHED_C05_REVERSED("PedSchedule::AddStart", 0x0046EB20, &PedSchedule::AddStart);
SCHED_C05_REVERSED("PedSchedule::ClearStarts", 0x0046EB70, &PedSchedule::ClearStarts);
SCHED_C05_REVERSED("PedScheduleManager::RemoveTagged", 0x0046EBC0, &PedScheduleManager::RemoveTagged);
SCHED_C05_REVERSED("PedScheduleManager::Clear", 0x0046EC30, &PedScheduleManager::Clear);
SCHED_C05_REVERSED_FN("PedScheduleManager::Reset", 0x0046EC80, &ResetManager);
