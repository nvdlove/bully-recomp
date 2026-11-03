// The patrol paths (0x471F40 .. 0x472C60): the scripts' patrol routes (AreaSet... natives),
// each with a ped created on it by the population when the player is near, followed by
// its FollowPath objective and shown on the radar.
//
// The manager (0xC2B034, 0x198 bytes) holds up to 100 entries from their own pool
// (0x20C7C5C, 0x28 bytes each).

#include <cmath>

#include "game/Ped.h"
#include "game/Pool.h"
#include "reversed.h"

namespace {

// A path of the path list (0x20C7C50): only what's read here
struct PatrolPath {
    uint8_t unknown0[5];
    int8_t pedType;      // +0x5 (-1: none; the population makes the ped from it)
    uint8_t unknown6[2];
    int32_t id;          // +0x8 (entries are matched by it)
};

struct PatrolEntry {
    re::Ptr32<PatrolPath> path;    // +0x00
    re::Ptr32<CPed> ped;           // +0x04 (registered)
    int32_t blip;        // +0x08 (-1)
    uint8_t released;    // +0x0C (0x471E70 done for the ped out of range)
    uint8_t unknownD;    // +0x0D (1; 0 once active hours are set)
    uint8_t unknownE;    // +0x0E (1; the ped's +0x1398, and its blip shows with it)
    uint8_t disabled;    // +0x0F (1)
    re::Ptr32<void> script;        // +0x10 the script that added it
    int32_t unknown14;   // +0x14 (2)
    int32_t unknown18;   // +0x18 (3)
    int32_t hoursFrom;   // +0x1C
    int32_t hoursTo;     // +0x20
    int32_t unknown24;   // +0x24 (1; copied to the FollowPath objective's +0x24)
};
static_assert(sizeof(PatrolEntry) == 0x28);

struct PatrolPaths {
    uint8_t enabled;           // +0x000
    uint8_t showVisionCones;   // +0x001
    uint8_t pad2[2];
    re::Ptr32<PatrolEntry> entries[100]; // +0x004
    uint32_t count;            // +0x194
};
static_assert(offsetof(PatrolPaths, count) == 0x194);
static_assert(sizeof(PatrolPaths) == 0x198);

GAME_VAR(PatrolPaths*, PP_paths, 0x00C2B034);
GAME_VAR(CPool<PatrolEntry>*, PP_entryPool, 0x020C7C5C);
GAME_VAR(void*, PP_pathList, 0x020C7C50);            // +8 the number of paths
GAME_VAR(uint8_t*, PP_population, 0x00C2C108);       // +0x7828, +0x782C, +0x7870: ranges
GAME_VAR(uint8_t, PP_noPedProduction, 0x00C2C10C);
GAME_VAR(CPool<CPed>*, PP_pedPool, 0x00C0F5F0);
GAME_VAR(void*, PP_radar, 0x00C674CC);
GAME_VAR(float, PP_two, 0x00900BFC);                 // 2.0f
GAME_VAR(double, PP_twoD, 0x00900980);               // 2.0
GAME_VAR(float, PP_minusOne, 0x009009AC);            // -1.0f
inline void* const PP_camera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(void, __thiscall, PP_EntryRelease, 0x00471E70, PatrolEntry* entry);
GAME_FN(uint8_t, __thiscall, PP_EntryIsActive, 0x00471D90, PatrolEntry* entry);
GAME_FN(uint8_t, __thiscall, PP_GetIsOnScreen, 0x00467460, CPed* ped);
GAME_FN(void, __cdecl, PP_RegisterReference, 0x004657D0, CPed* entity, re::Ptr32<CPed>* ref);
GAME_FN(void, __cdecl, PP_CleanUpOldReference, 0x004657E0, CPed* entity, re::Ptr32<CPed>* ref);
GAME_FN(void, __thiscall, PP_PedSet1570, 0x0047B7E0, CPed* ped, int state, int a, int b);
GAME_FN(PatrolEntry*, __thiscall, PP_PoolNew, 0x0045CAD0, CPool<PatrolEntry>* pool);
GAME_FN(ped::CVector*, __cdecl, PP_FindPlayerCoors, 0x004469E0, uintptr_t player);
GAME_FN(ped::CVector*, __cdecl, PP_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(int, __thiscall, PP_CameraGetFadeState, 0x004F12F0, void* camera);
GAME_FN(int, __thiscall, PP_CameraIsSphereVisible, 0x004F13D0, void* camera, const ped::CVector* centre, float radius);
GAME_FN(int, __thiscall, PP_PopulationChooseModel, 0x0049C1F0, void* population, int a);
GAME_FN(uint8_t*, __cdecl, PP_GetStreamingInfo, 0x0052DD30, int model);
GAME_FN(uint8_t, __cdecl, PP_Model4151E0, 0x004151E0, int model);
GAME_FN(void, __cdecl, PP_RequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __cdecl, PP_Fn446900, 0x00446900, ped::CVector* out);
GAME_FN(uint32_t, __thiscall, PP_PathNumNodes, 0x005EC670, PatrolPath* path);
GAME_FN(int, __cdecl, PP_Random, 0x0040DA80, int from, int to);
GAME_FN(ped::CVector*, __thiscall, PP_PathNodeAt, 0x005EC8C0, PatrolPath* path, int index);
GAME_FN(ped::CVector*, __thiscall, PP_PathFirstNode, 0x005EC830, PatrolPath* path);
GAME_FN(ped::CVector*, __thiscall, PP_PathNextNode, 0x005EC870, PatrolPath* path);
GAME_FN(CPed*, __thiscall, PP_PopulationAddPed, 0x004A0AD0, void* population, int pedType, ped::CVector pos);
GAME_FN(void, __thiscall, PP_Fn5F40A0, 0x005F40A0, void* self);
GAME_FN(void, __thiscall, PP_QueueClear, 0x00470350, ObjectiveQueue* queue);
GAME_FN(void, __thiscall, PP_QueueSetDefault, 0x00470300, ObjectiveQueue* queue, Objective* objective);
GAME_FN(void, __thiscall, PP_RadarClearBlips, 0x0054A890, void* radar, int kind, int handle);
GAME_FN(int, __thiscall, PP_RadarAddBlip, 0x0054A5B0, void* radar, int kind, int handle, uint32_t colour, int display,
        int a, int b);
GAME_FN(void, __thiscall, PP_RadarSetBlipSprite, 0x00548BF0, void* radar, int blip, int sprite);
GAME_FN(void*, __cdecl, PP_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, PP_Free, 0x005EEAB0, void* p);
GAME_FN(Objective*, __thiscall, PP_FollowPathConstruct, 0x0046FD70, void* self, CPed* ped);
GAME_FN(PatrolPath*, __thiscall, PP_PathListGet, 0x00415DE0, void* list, int index);
GAME_FN(Objective*, __thiscall, PP_QueueGetCurrent, 0x00470340, ObjectiveQueue* queue);
GAME_FN(uint8_t*, __cdecl, PP_AsFollowPath, 0x00471E10, Objective* objective);
GAME_FN(CEntity*, __thiscall, PP_PedGetTarget, 0x00497100, CPed* ped);
GAME_FN(CPed*, __thiscall, PP_PedGetTarget46ECE0, 0x0046ECE0, CPed* ped);

// Finds a path's entry (by the path's id)
int FindEntry(PatrolPaths* self, const PatrolPath* path) {
    for (uint32_t i = 0; i < self->count; i++) {
        if (self->entries[i]->path->id == path->id) {
            return int(i);
        }
    }
    return -1;
}

const ped::CVector* PedPos(CPed* ped) {
    return ped->matrix ? reinterpret_cast<ped::CVector*>(ped->matrix + 0x30) : &ped->position;
}

// (inlined): the ped let go
void ReleasePed(PatrolEntry* entry) {
    PP_EntryRelease(entry);
    if (entry->ped) {
        PP_PedSet1570(entry->ped, 1, 1, 1);
        if (entry->ped) {
            PP_CleanUpOldReference(entry->ped, &entry->ped);
            entry->ped = nullptr;
        }
    }
}

// (inlined): the entry's destructor (0x472420) and its slot back to the pool
void DeleteEntry(PatrolEntry* entry) {
    if (entry) {
        ReleasePed(entry);
        if (entry->ped) {
            PP_CleanUpOldReference(entry->ped, &entry->ped);
        }
        PP_entryPool->Delete(entry);
    }
}

// 0x471F40 (thiscall)
void __fastcall PatrolPaths_SetActiveHours(PatrolPaths* self, re::Edx, PatrolPath* path, int from, int to) {
    if (path) {
        int i = FindEntry(self, path);
        if (i >= 0) {
            PatrolEntry* entry = self->entries[i];
            entry->hoursFrom = from;
            entry->hoursTo = to;
            entry->unknownD = 0;
        }
    }
}

// 0x471FA0 (cdecl): a place is where the population may put a patrol ped: near enough
// the player, and (unless `ignoreCamera`) not on screen within the near range
uint8_t __cdecl PatrolPaths_IsPosInRange(const ped::CVector* pos, int ignoreCamera) {
    ped::CVector player = *PP_FindPlayerCoors(0);
    ped::CVector diff;
    PP_VectorSub(&diff, pos, &player);
    float zRange = *reinterpret_cast<float*>(PP_population + 0x7870);
    float distSq = std::fabs(diff.x * diff.x + diff.y * diff.y);
    float dist = static_cast<float>(std::sqrt(static_cast<double>(distSq)));
    if (re::Flag(ignoreCamera)) {
        zRange = static_cast<float>(zRange + PP_twoD);
    } else if (PP_CameraGetFadeState(PP_camera) != 2) {
        ped::CVector centre = *pos;
        if (PP_CameraIsSphereVisible(PP_camera, &centre, PP_two) != 0 &&
            *reinterpret_cast<float*>(PP_population + 0x782C) > dist) {
            return 0;
        }
    }
    if (*reinterpret_cast<float*>(PP_population + 0x7828) < dist) {
        return 0;
    }
    if (zRange < std::fabs(diff.z)) {
        return 0;
    }
    return 1;
}

// 0x4720D0 (thiscall)
void __fastcall PatrolPaths_Enable(PatrolPaths* self, re::Edx, PatrolPath* path) {
    int i = FindEntry(self, path);
    if (i >= 0) {
        self->entries[i]->disabled = 0;
    }
}

// 0x472120 (thiscall)
void __fastcall PatrolPaths_InitPath(PatrolPaths* self, re::Edx, PatrolPath* path, int a, int b, int c, int d) {
    if (path && c != -1) {
        int i = FindEntry(self, path);
        if (i >= 0) {
            self->entries[i]->unknown14 = a;
            self->entries[i]->unknown18 = b;
            self->entries[i]->unknown24 = c;
            self->entries[i]->unknownE = static_cast<uint8_t>(d);
            self->entries[i]->disabled = 0;
        }
    }
}

// 0x4721A0 (thiscall): the ped dropped once it's out of range (deleted unless on screen,
// or let go when its +0x1D24 kind is 3 .. 15)
void __fastcall PatrolEntry_UpdateOutOfRange(PatrolEntry* self, re::Edx) {
    if (!self->path || !self->ped) {
        return;
    }
    if (PatrolPaths_IsPosInRange(PedPos(self->ped), 1)) {
        return;
    }
    uint8_t* kind = self->ped->unknown1D24;
    if (kind && *reinterpret_cast<int32_t*>(kind + 0x70) > 2 && *reinterpret_cast<int32_t*>(kind + 0x70) < 0x10) {
        PP_EntryRelease(self);
    } else {
        if (PP_GetIsOnScreen(self->ped)) {
            return;
        }
        if (self->ped) {
            reinterpret_cast<void(__thiscall*)(CPed*, int)>(static_cast<void*>(self->ped->vtable[0]))(self->ped, 1);
        }
    }
    if (self->ped) {
        PP_CleanUpOldReference(self->ped, &self->ped);
        self->ped = nullptr;
    }
}

// 0x472230 (thiscall): a path added (once; not without a ped type)
void __fastcall PatrolPaths_Add(PatrolPaths* self, re::Edx, PatrolPath* path, void* script) {
    if (!path || path->pedType == -1) {
        return;
    }
    if (FindEntry(self, path) >= 0) {
        return;
    }
    PatrolEntry* entry = PP_PoolNew(PP_entryPool);
    if (entry) {
        entry->path = nullptr;
        entry->ped = nullptr;
        entry->blip = -1;
        entry->released = 0;
        entry->unknownD = 1;
        entry->unknownE = 1;
        entry->disabled = 1;
        entry->script = nullptr;
        entry->unknown14 = 2;
        entry->unknown18 = 3;
        entry->hoursFrom = 0;
        entry->hoursTo = 0;
        entry->unknown24 = 1;
    } else {
        entry = nullptr;  // (written through below, as the game does)
    }
    entry->path = path;
    entry->script = script;
    self->entries[self->count] = entry;
    self->count++;
}

// 0x4722F0 (thiscall): every one enabled (each looked up by its own path's id)
void __fastcall PatrolPaths_EnableAll(PatrolPaths* self, re::Edx) {
    self->enabled = 1;
    for (uint32_t n = 0; n < self->count; n++) {
        int i = FindEntry(self, self->entries[n]->path);
        if (i >= 0) {
            self->entries[i]->disabled = 0;
        }
    }
}

// 0x472350 (thiscall): the entry with this ped lets go of it
void __fastcall PatrolPaths_ForgetPed(PatrolPaths* self, re::Edx, CPed* ped) {
    for (uint32_t i = 0; i < self->count; i++) {
        PatrolEntry* entry = self->entries[i];
        if (entry->ped == ped) {
            if (entry->ped) {
                PP_CleanUpOldReference(entry->ped, &entry->ped);
                entry->ped = nullptr;
            }
            return;
        }
    }
}

// 0x4723A0 (thiscall)
void __fastcall PatrolPaths_Disable(PatrolPaths* self, re::Edx, PatrolPath* path) {
    int i = FindEntry(self, path);
    if (i >= 0) {
        ReleasePed(self->entries[i]);
        self->entries[i]->disabled = 1;
    }
}

// 0x472420 (thiscall): the entry's destructor
void __fastcall PatrolEntry_Destruct(PatrolEntry* self, re::Edx) {
    ReleasePed(self);
    if (self->ped) {
        PP_CleanUpOldReference(self->ped, &self->ped);
    }
}

// 0x472470 (thiscall): a ped made at a random node of the path in range (its model
// requested first), with the path as its default objective and a blip
void __fastcall PatrolEntry_CreatePed(PatrolEntry* self, re::Edx) {
    if (PP_noPedProduction) {
        return;
    }
    int pedType = self->path->pedType;
    if (pedType == -1) {
        return;
    }
    if (PP_EntryIsActive(self)) {
        int model = PP_PopulationChooseModel(PP_population, 0);
        if (model != -1 && PP_GetStreamingInfo(model)[4] != 2 && !PP_Model4151E0(model)) {
            PP_RequestModel(model, 0x10);
        }
    }
    ped::CVector unused;
    PP_Fn446900(&unused);
    if (PP_PathNumNodes(self->path) < 2) {
        return;
    }
    int index = PP_Random(0, static_cast<int>(PP_PathNumNodes(self->path)));
    ped::CVector* node = PP_PathNodeAt(self->path, index);
    if (!node) {
        return;
    }
    ped::CVector pos;
    for (;;) {
        pos = *node;
        if (PatrolPaths_IsPosInRange(&pos, 0)) {
            break;
        }
        node = PP_PathNextNode(self->path);
        index++;
        if (!node) {
            return;
        }
    }
    CPed* ped = PP_PopulationAddPed(PP_population, pedType, pos);
    if (ped != self->ped) {
        if (self->ped) {
            PP_CleanUpOldReference(self->ped, &self->ped);
        }
        self->ped = ped;
        if (ped) {
            PP_RegisterReference(ped, &self->ped);
        }
    }
    if (!self->ped) {
        return;
    }
    uint8_t* p = re::PtrCast<uint8_t*>(self->ped);
    p[0x1C99] = 0;
    *reinterpret_cast<int32_t*>(p + 0xE4) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x1398) = self->unknownE;
    PP_Fn5F40A0(self->ped->intelligence + 0x290);
    PP_QueueClear(&self->ped->objectives);
    int slot = PP_pedPool->GetIndex(self->ped);
    int handle = PP_pedPool->flags[slot] + (slot << 8);
    PP_RadarClearBlips(PP_radar, 6, handle);
    int display = (self->unknownE == 0 && PP_paths->showVisionCones == 0) ? 1 : 3;
    int blip = PP_RadarAddBlip(PP_radar, 6, handle, 0, display, 2, 0);
    self->blip = blip;
    PP_RadarSetBlipSprite(PP_radar, blip, 5);
    void* memory = PP_Malloc(0x40);
    Objective* followPath = memory ? PP_FollowPathConstruct(memory, self->ped) : nullptr;
    PP_QueueSetDefault(&self->ped->objectives, followPath);
    int numPaths = *reinterpret_cast<int32_t*>(re::PtrCast<uint8_t*>(PP_pathList) + 8);
    for (int i = 0; i < numPaths; i++) {
        if (self->path == PP_PathListGet(PP_pathList, i)) {
            uint8_t* o = reinterpret_cast<uint8_t*>(followPath);
            *reinterpret_cast<int32_t*>(o + 0x10) = i;
            *reinterpret_cast<int32_t*>(o + 0x14) = index;
            *reinterpret_cast<int32_t*>(o + 0x24) = self->unknown24;
        }
    }
}

// 0x472700 (thiscall): an enabled entry's ped made when it's active, dropped when not
void __fastcall PatrolEntry_Update(PatrolEntry* self, re::Edx) {
    if (self->disabled) {
        return;
    }
    uint8_t active = PP_EntryIsActive(self);
    if (!self->ped) {
        if (active) {
            PatrolEntry_CreatePed(self, nullptr);
        }
        return;
    }
    if (!active) {
        if (!self->released) {
            PP_EntryRelease(self);
            self->released = 1;
        }
        PatrolEntry_UpdateOutOfRange(self, nullptr);
        return;
    }
    Objective* current = PP_QueueGetCurrent(&self->ped->objectives);
    if (current && current->type == 0xC) {
        uintptr_t followPath = reinterpret_cast<uintptr_t>(PP_AsFollowPath(current)) + 0x10;
        if (followPath != 0) {
            int32_t& value = *reinterpret_cast<int32_t*>(followPath + 0x14);
            if (value != self->unknown24) {
                value = self->unknown24;
            }
        }
    } else {
        CEntity* target = PP_PedGetTarget(self->ped);
        if (target && (reinterpret_cast<uint8_t*>(target)[0x108] & 7) == 3) {
            CPed* other = PP_PedGetTarget46ECE0(self->ped);
            ped::CVector pos = *PedPos(other);
            ped::CVector* node = PP_PathFirstNode(self->path);
            if (node) {
                // (the nearest node's squared distance, found and never used)
                float best = PP_minusOne;
                do {
                    ped::CVector diff;
                    ped::CVector* d = PP_VectorSub(&diff, &pos, node);
                    float distSq = d->x * d->x + d->y * d->y;
                    if (best > distSq || 0.0f > best) {
                        best = distSq;
                    }
                    node = PP_PathNextNode(self->path);
                } while (node);
                (void)best;
            }
        }
    }
    PatrolEntry_UpdateOutOfRange(self, nullptr);
}

// 0x472860 (thiscall)
void __fastcall PatrolPaths_DisableAll(PatrolPaths* self, re::Edx) {
    self->enabled = 0;
    for (uint32_t i = 0; i < self->count; i++) {
        PatrolPaths_Disable(self, nullptr, self->entries[i]->path);
    }
}

// 0x4728A0 (thiscall): its entry deleted (the last one moved into its place)
void __fastcall PatrolPaths_Remove(PatrolPaths* self, re::Edx, PatrolPath* path) {
    int i = FindEntry(self, path);
    if (i < 0) {
        return;
    }
    ReleasePed(self->entries[i]);
    DeleteEntry(self->entries[i]);
    self->entries[i] = self->entries[self->count - 1];
    self->count--;
}

// 0x4729A0 (thiscall)
void __fastcall PatrolPaths_Update(PatrolPaths* self, re::Edx) {
    if (self->enabled) {
        for (uint32_t i = 0; i < self->count; i++) {
            PatrolEntry_Update(self->entries[i], nullptr);
        }
    }
}

// 0x4729D0 (thiscall): a script's entries deleted (the one moved into a freed place
// is skipped, as the game does)
void __fastcall PatrolPaths_ClearForScript(PatrolPaths* self, re::Edx, void* script) {
    for (uint32_t i = 0; i < self->count; i++) {
        if (self->entries[i]->script == script) {
            ReleasePed(self->entries[i]);
            DeleteEntry(self->entries[i]);
            self->entries[i] = self->entries[self->count - 1];
            self->count--;
        }
    }
}

// 0x472AF0 (thiscall)
void __fastcall PatrolPaths_ClearAll(PatrolPaths* self, re::Edx) {
    for (uint32_t i = 0; i < self->count; i++) {
        ReleasePed(self->entries[i]);
        DeleteEntry(self->entries[i]);
    }
    self->count = 0;
}

// 0x472BF0
void __cdecl PatrolPaths_Reset() {
    PatrolPaths_ClearAll(PP_paths, nullptr);
    PP_paths->enabled = 1;
    PP_paths->showVisionCones = 0;
}

// 0x472C10: made on first use
void __cdecl PatrolPaths_Init() {
    PatrolPaths* paths = PP_paths;
    if (!paths) {
        paths = static_cast<PatrolPaths*>(PP_Malloc(sizeof(PatrolPaths)));
        if (paths) {
            paths->count = 0;
        } else {
            paths = nullptr;
        }
        PP_paths = paths;
    }
    PatrolPaths_ClearAll(paths, nullptr);
    PP_paths->enabled = 1;
    PP_paths->showVisionCones = 0;
}

// 0x472C60
void __cdecl PatrolPaths_Shutdown() {
    PatrolPaths* paths = PP_paths;
    if (paths) {
        PatrolPaths_ClearAll(paths, nullptr);
        PP_paths->enabled = 1;
        PP_paths->showVisionCones = 0;
        PP_Free(paths);
        PP_paths = nullptr;
    }
}

#define PATROL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PATROL_REVERSED("PatrolPaths::SetActiveHours", 0x00471F40, &PatrolPaths_SetActiveHours);
PATROL_REVERSED("PatrolPaths::IsPosInRange", 0x00471FA0, &PatrolPaths_IsPosInRange);
PATROL_REVERSED("PatrolPaths::Enable", 0x004720D0, &PatrolPaths_Enable);
PATROL_REVERSED("PatrolPaths::InitPath", 0x00472120, &PatrolPaths_InitPath);
PATROL_REVERSED("PatrolEntry::UpdateOutOfRange", 0x004721A0, &PatrolEntry_UpdateOutOfRange);
PATROL_REVERSED("PatrolPaths::Add", 0x00472230, &PatrolPaths_Add);
PATROL_REVERSED("PatrolPaths::EnableAll", 0x004722F0, &PatrolPaths_EnableAll);
PATROL_REVERSED("PatrolPaths::ForgetPed", 0x00472350, &PatrolPaths_ForgetPed);
PATROL_REVERSED("PatrolPaths::Disable", 0x004723A0, &PatrolPaths_Disable);
PATROL_REVERSED("PatrolEntry::Destruct", 0x00472420, &PatrolEntry_Destruct);
PATROL_REVERSED("PatrolEntry::CreatePed", 0x00472470, &PatrolEntry_CreatePed);
PATROL_REVERSED("PatrolEntry::Update", 0x00472700, &PatrolEntry_Update);
PATROL_REVERSED("PatrolPaths::DisableAll", 0x00472860, &PatrolPaths_DisableAll);
PATROL_REVERSED("PatrolPaths::Remove", 0x004728A0, &PatrolPaths_Remove);
PATROL_REVERSED("PatrolPaths::Update", 0x004729A0, &PatrolPaths_Update);
PATROL_REVERSED("PatrolPaths::ClearForScript", 0x004729D0, &PatrolPaths_ClearForScript);
PATROL_REVERSED("PatrolPaths::ClearAll", 0x00472AF0, &PatrolPaths_ClearAll);
PATROL_REVERSED("PatrolPaths::Reset", 0x00472BF0, &PatrolPaths_Reset);
PATROL_REVERSED("PatrolPaths::Init", 0x00472C10, &PatrolPaths_Init);
PATROL_REVERSED("PatrolPaths::Shutdown", 0x00472C60, &PatrolPaths_Shutdown);

} // namespace
