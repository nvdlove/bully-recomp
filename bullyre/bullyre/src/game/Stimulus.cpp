// Stimuli: things peds can notice (a noise, a fight, the player running...), each a
// CStimulus (0x2C bytes, pool 0xC0F610) of one of 0x57 types, held by the stimulus
// manager (0xC2C1A0, 0x2BC bytes: up to 0x57 stimuli, then their count at +0x15C, then
// the type table at +0x160). Also the broadcast stimuli (a table at 0xA3523C of types
// given to every ped for a while) and the player's noise and awareness levels.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

namespace {

using re::At;

struct Vec {
    float x, y, z;
};

// A type (from the data: +0x18 its id, +0x1C its priority, +0x20 how long it lasts
// (s), +0x24 / +0x2C the default radius / range, +0x28 how far it's heard, +0x34 the
// source's position first, +0x35 it follows its entity, +0x36 it's aimed)
struct CStimulus {
    uint32_t type;       // +0x0
    re::Ptr32<uint8_t> source;     // +0x4 (a registered reference)
    re::Ptr32<uint8_t> target;     // +0x8 (a registered reference)
    Vec pos;             // +0xC
    int32_t time;        // +0x18 when it last happened
    float radius;        // +0x1C sensed within this
    float range;         // +0x20 (by peds standing, walking or idle: states 0, 7, 8)
    int32_t repeats;     // +0x24
    uint8_t sensed;      // +0x28
    uint8_t unknown29;   // +0x29
    uint8_t expired;     // +0x2A
    uint8_t unknown2B;
};
static_assert(offsetof(CStimulus, pos) == 0xC);
static_assert(offsetof(CStimulus, time) == 0x18);
static_assert(offsetof(CStimulus, range) == 0x20);
static_assert(offsetof(CStimulus, sensed) == 0x28);
static_assert(sizeof(CStimulus) == 0x2C);

constexpr uint32_t kNumTypes = 0x57;

struct CStimulusManager {
    re::Ptr32<CStimulus> stimuli[kNumTypes];   // +0x0
    int32_t count;                   // +0x15C
    re::Ptr32<uint8_t> types[kNumTypes];       // +0x160
};
static_assert(offsetof(CStimulusManager, count) == 0x15C);
static_assert(offsetof(CStimulusManager, types) == 0x160);
static_assert(sizeof(CStimulusManager) == 0x2BC);

// a broadcast stimulus: its type, on (a byte, the rest of the dword unused), for how long
struct BroadcastEntry {
    int32_t type;
    uint8_t on;
    uint8_t unknown5[3];
    float timeLeft;
};
static_assert(sizeof(BroadcastEntry) == 0xC);

GAME_VAR(CStimulusManager*, g_stimuli, 0x00C2C1A0);
GAME_VAR(float, g_awareness, 0x00C2C1A4);       // the player's, 0..1
GAME_VAR(float, g_playerNoise, 0x00C2C1A8);
GAME_VAR(uint8_t, g_playerC2C1AC, 0x00C2C1AC);
GAME_VAR(uint8_t*, g_tuning, 0x020C8918);        // +0x18.. the awareness's decay, +0x28.. the noise
GAME_VAR(uint8_t*, g_surfaceNoise, 0x020C890C);  // +0x18 a float by surface (0x42 of them)
GAME_VAR(float, g_awarenessDecay, 0x00A35230);
GAME_ARRAY(BroadcastEntry, g_broadcast, 0x00A3523C);
GAME_VAR(uint32_t, g_numBroadcast, 0x00A35650);
GAME_VAR(CPool<uint8_t>*, stimulusPool, 0x00C0F610);
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(uint32_t, frameCounter, 0x00C1A99C);
GAME_VAR(void*, g_C0F78C, 0x00C0F78C);           // (told of new stimuli)
inline void* const g_pathBlockers = reinterpret_cast<void*>(0x00C2AB70);
inline void* const g_typeNames = reinterpret_cast<void*>(0x01B65420);
inline void* const g_typeRoot = reinterpret_cast<void*>(0x01B65424);
inline void* const loadTypesCallback = reinterpret_cast<void*>(0x004A5E50);
constexpr uintptr_t vtblBroadcastStimulusManager = 0x0091245C;

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(Vec*, __cdecl, VectorSub, 0x00414B70, Vec* out, const Vec* a, const Vec* b);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(uint8_t, __thiscall, Ped_IsPoiModel, 0x004473E0, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsGangType, 0x0041AA50, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_49B5F0, 0x0049B5F0, void* ped);
GAME_FN(uint8_t, __thiscall, Senses_CanHear_x64, 0x0048D0A0, void* senses, re::Float3);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t Senses_CanHear(void* a0, float a1, float a2, float a3) {
    return Senses_CanHear_x64(a0, re::Float3{a1, a2, a3});
}
GAME_FN(uint8_t, __thiscall, Senses_CanSee, 0x0048D470, void* senses, void* entity);
GAME_FN(uint8_t*, __thiscall, Fn44F2B0, 0x0044F2B0, void* object, CStimulus* stimulus);
GAME_FN(void, __thiscall, Fn450610, 0x00450610, void* object, CStimulus* stimulus);
GAME_FN(uint8_t, __thiscall, PathBlockers_Blocks, 0x0046EFB0, void* blockers, void* ped, const Vec* pos, int);
GAME_FN(float, __cdecl, Fn45B840, 0x0045B840);
GAME_FN(int, __cdecl, TypeNames_Count, 0x0072B7D0, void* path);
GAME_FN(uint8_t*, __cdecl, TypeNames_Get, 0x0072B810, void* path, int index, void* root);
GAME_FN(void, __cdecl, AddLoadCallback, 0x0072B870, void* fn, void* object);
GAME_FN(void, __cdecl, RemoveLoadCallback, 0x0072B8B0, void* fn, void* object);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(int32_t, __thiscall, Fn465580, 0x00465580, void* self, uint32_t hash, void* arg);
GAME_FN(CStimulus*, __thiscall, StimulusPool_New, 0x004A6350, CPool<uint8_t>* pool);
// (their own, called where modes apply)
GAME_FN(void, __thiscall, Stimuli_LoadTypes, 0x004A53F0, CStimulusManager* self);
GAME_FN(void, __cdecl, UpdatePlayerNoise, 0x004A5470);
GAME_FN(void, __cdecl, RaiseAwareness, 0x004A5A40, float level);
GAME_FN(void, __thiscall, Broadcast_RemoveAt, 0x004A5AA0, BroadcastEntry* table, uint32_t index);
GAME_FN(uint8_t, __thiscall, Stimulus_CanBeSensedBy, 0x004A5B10, CStimulus* self, uint8_t* ped);
GAME_FN(void, __thiscall, Stimulus_UpdatePosition, 0x004A5D10, CStimulus* self);
GAME_FN(void, __thiscall, Stimulus_OnSensed, 0x004A5D80, CStimulus* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Stimuli_IsBetter, 0x004A5E60, CStimulusManager* self, CStimulus* a, CStimulus* b, uint8_t* ped);
GAME_FN(CStimulus*, __thiscall, Stimulus_Construct, 0x004A63D0, CStimulus* self, uint32_t type, uint8_t* source,
        uint8_t* target, const Vec* pos, float radius, float range);
GAME_FN(void, __thiscall, Stimulus_Destruct, 0x004A64F0, CStimulus* self);
GAME_FN(void, __thiscall, Stimuli_Add, 0x004A6670, CStimulusManager* self, uint32_t type, uint8_t* source, uint8_t* target,
        const Vec* pos, float radius, float range);
GAME_FN(void, __thiscall, Stimuli_Clear, 0x004A6940, CStimulusManager* self);

uint8_t* TypeInfo(uint32_t type) {
    return type < kNumTypes ? g_stimuli->types[type] : nullptr;
}
const Vec* EntityPos(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(entity + 4);
}
uint8_t* IfPed(uint8_t* entity) {
    return entity && (At<uint8_t>(entity, 0x108) & 7) == 3 ? entity : nullptr;
}
float Length(const Vec& v) {
    return float(std::sqrt(std::fabs(float((double(v.x) * v.x + double(v.y) * v.y) + double(v.z) * v.z))));
}
// destroyed and its slot back to the pool
void DeleteStimulus(CStimulus* s) {
    Stimulus_Destruct(s);
    CPool<uint8_t>* pool = stimulusPool;
    int slot = int((reinterpret_cast<uint8_t*>(s) - pool->objects) / pool->elemSize);
    pool->flags[slot] |= 0x80;
    if (slot < pool->lastUsed) {
        pool->lastUsed = slot;
    }
}
// the stimulus's expiry time: when it happened plus its type's duration (as the game
// makes it: ftol(duration * -1000) taken off)
int32_t ExpiryOf(CStimulus* s, uint8_t* info) {
    return s->time - int32_t(double(At<float>(info, 0x20)) * -1000.0);
}

} // namespace

namespace c09stimulus {

// 0x4A53F0: the type table filled from the data (by each type's id; 0x57: none)
void __fastcall LoadTypes(CStimulusManager* self, re::Edx) {
    struct {
        int32_t count;
        re::Ptr32<void> names;
    } path = {1, g_typeNames};
    int count = TypeNames_Count(&path);
    for (uint32_t i = 0; i < kNumTypes; i++) {
        self->types[i] = nullptr;
    }
    for (int i = 0; i < count; i++) {
        uint8_t* info = TypeNames_Get(&path, i, g_typeRoot);
        int32_t id = At<int32_t>(info, 0x18);
        if (id != int32_t(kNumTypes)) {
            self->types[id] = info;
        }
    }
}

// 0x4A5470: how much noise the player makes: by the surface under him and how fast
// he goes (running, or sneaking / crouching: its own values); none while +0x1E91
void __cdecl UpdatePlayerNoise() {
    uint8_t* player = playerPed;
    uint32_t surface = At<uint8_t>(player, 0x1BE);
    float surfaceFactor = surface <= 0x41 ? At<float>(g_surfaceNoise, 0x18 + surface * 4) : At<float>(g_surfaceNoise, 0x18);
    float speed = At<float>(At<uint8_t*>(player, 0x2E0), 0x1C);
    float noise = 0.0f;
    if (At<int32_t>(player, 0x131C) == 0 && At<int32_t>(player, 0x1504) == 0) {
        if (!(double(speed) < double(0.1f))) {
            float lo = At<float>(g_tuning, 0x30);
            float hi = At<float>(g_tuning, 0x34);
            float t = float(double(speed) / double(1.49f));
            noise = float((double(hi) - lo) * t + lo);
        }
    } else if (!(double(speed) < double(0.1f))) {
        noise = speed < 0.5f ? At<float>(g_tuning, 0x28) : At<float>(g_tuning, 0x2C);
    }
    g_playerNoise = 0.0f;
    if (At<uint8_t>(player, 0x1E91) == 0) {
        g_playerNoise = float(double(noise) * surfaceFactor);
    }
}

// 0x4A5A40: the player's awareness raised to the level (scaled, within 0..1)
void __cdecl RaiseAwarenessTo(float level) {
    float v = float(double(level) / At<float>(g_tuning, 0x20));
    if (!(g_awareness < v)) {
        return;
    }
    if (0.0f > v) {
        g_awareness = 0.0f;
    } else if (1.0f < v) {
        g_awareness = 1.0f;
    } else {
        g_awareness = v;
    }
}

// 0x4A5A90
float __cdecl GetPlayerNoise() {
    return g_playerNoise;
}

// 0x4A5AA0: a broadcast stimulus taken out (the ones after it moved down)
void __fastcall BroadcastRemoveAt(BroadcastEntry* table, re::Edx, uint32_t index) {
    uint32_t& count = At<uint32_t>(table, 0x414);
    for (; index < count - 1; index++) {
        table[index].type = table[index + 1].type;
        At<uint32_t>(&table[index], 4) = At<uint32_t>(&table[index + 1], 4);
        table[index].timeLeft = table[index + 1].timeLeft;
    }
    count--;
}

// 0x4A5AF0: CBroadcastStimulusManager's deleting destructor
void* __fastcall BroadcastManagerDestroy(uint8_t* self, re::Edx, uint8_t flags) {
    At<uint32_t>(self, 0) = vtblBroadcastStimulusManager;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x4A5B10: the ped senses it: its target always, its source never; within its radius
// (range for a ped standing, walking or idle), or heard within its type's reach
uint8_t __fastcall CanBeSensedBy(CStimulus* self, re::Edx, uint8_t* ped) {
    if (ped == self->target) {
        return 1;
    }
    if (ped == self->source) {
        return 0;
    }
    uint32_t type = self->type;
    if (type == 0x30 && !Ped_IsPoiModel(ped)) {
        return 0;
    }
    uint8_t* info = type <= 0x56 ? g_stimuli->types[type] : nullptr;
    int32_t state = At<int32_t>(ped, 0x1310);
    float radius = (state == 0 || state == 8 || state == 7) ? self->range : self->radius;
    Vec d;
    VectorSub(&d, &self->pos, EntityPos(ped));
    float dist = Length(d);
    if (radius > dist) {
        return 1;
    }
    if (info && At<float>(info, 0x28) > dist && Senses_CanHear(ped + 0xA64, self->pos.x, self->pos.y, self->pos.z)) {
        return 1;
    }
    return 0;
}

// 0x4A5C50: over by the time (or already expired, or with nobody or no type)
uint8_t __fastcall HasExpiredBy(CStimulus* self, re::Edx, int32_t time) {
    if (self->expired || (!self->source && !self->target) || self->type > 0x56) {
        return 1;
    }
    uint8_t* info = g_stimuli->types[self->type];
    if (!info) {
        return 1;
    }
    return ExpiryOf(self, info) < time;
}

// 0x4A5CB0 / 0x4A5CD0: its source / target when that's a ped
uint8_t* __fastcall GetSourcePed(CStimulus* self, re::Edx) {
    return IfPed(self->source);
}
uint8_t* __fastcall GetTargetPed(CStimulus* self, re::Edx) {
    return IfPed(self->target);
}

// 0x4A5CF0: its type's priority (0: no type)
int32_t __fastcall GetPriority(CStimulus* self, re::Edx) {
    uint8_t* info = TypeInfo(self->type);
    return info ? At<int32_t>(info, 0x1C) : 0;
}

// 0x4A5D10: its position from its source (when the type says so) or target or source
void __fastcall UpdatePosition(CStimulus* self, re::Edx) {
    uint8_t* entity = nullptr;
    uint8_t* info = TypeInfo(self->type);
    if (info && At<uint8_t>(info, 0x34) && self->source) {
        entity = self->source;
    } else if (self->target) {
        entity = self->target;
    } else if (self->source) {
        entity = self->source;
    } else {
        return;
    }
    self->pos = *EntityPos(entity);
}

// 0x4A5D80: a gang ped sensed it (from a ped): seen when within its range or the ped
// sees the source; passed on (0xC0F78C) when the source's +0x1D40 is below the
// listener's
void __fastcall OnSensed(CStimulus* self, re::Edx, uint8_t* ped) {
    uint8_t* source = self->source;
    if (!source || (At<uint8_t>(source, 0x108) & 7) != 3) {
        return;
    }
    Vec d;
    VectorSub(&d, EntityPos(ped), &self->pos);
    float distSq = float((double(d.x) * d.x + double(d.y) * d.y) + double(d.z) * d.z);
    if (distSq < float(double(self->range) * self->range) || Senses_CanSee(ped + 0xA64, source)) {
        self->sensed = 1;
    }
    uint8_t* listener = Fn44F2B0(g_C0F78C, self);
    if (listener && At<int32_t>(source, 0x1D40) < At<int32_t>(listener, 0x1C) && !self->unknown29) {
        Fn450610(g_C0F78C, self);
    }
}

// 0x4A5E60: a matters more to the ped than b: aimed at it (types that say so), then
// by priority, then the nearer
uint8_t __fastcall IsBetter(CStimulusManager*, re::Edx, CStimulus* a, CStimulus* b, uint8_t* ped) {
    uint8_t* infoA;
    uint8_t* infoB;
    if (a->target == ped && (infoA = TypeInfo(a->type)) != nullptr && At<uint8_t>(infoA, 0x36)) {
        if (b->target != ped) {
            return 1;
        }
    } else if (b->target == ped && (infoB = TypeInfo(b->type)) != nullptr && At<uint8_t>(infoB, 0x36)) {
        return 0;
    }
    infoA = TypeInfo(a->type);
    int32_t priorityA = infoA ? At<int32_t>(infoA, 0x1C) : 0;
    infoB = TypeInfo(b->type);
    int32_t priorityB = infoB ? At<int32_t>(infoB, 0x1C) : 0;
    if (priorityA != priorityB) {
        return priorityA > priorityB;
    }
    Vec posA = a->pos;
    Vec d;
    float distA = Length(*VectorSub(&d, EntityPos(ped), &posA));
    Vec posB = b->pos;
    float distB = Length(*VectorSub(&d, EntityPos(ped), &posB));
    return distA < distB;
}

// 0x4A6080: the one the ped should react to (kept in *best when none is better): the
// ones it senses and can reach; gang peds note them as sensed. None for a ped busy
// (+0x1370 / +0x1DD4)
void __fastcall FindBestFor(CStimulusManager* self, re::Edx, uint8_t* ped, re::Ptr32<CStimulus>* best) {
    if (At<int32_t>(ped, 0x1370) != 0 || At<int32_t>(ped, 0x1DD4) != 0) {
        *best = nullptr;
        return;
    }
    for (int i = self->count; i != 0;) {
        CStimulus* s = self->stimuli[i - 1];
        Vec pos = s->pos;
        i--;
        if (PathBlockers_Blocks(g_pathBlockers, ped, &pos, 0)) {
            continue;
        }
        if (!Stimulus_CanBeSensedBy(self->stimuli[i], ped)) {
            continue;
        }
        if (Ped_IsGangType(ped)) {
            Stimulus_OnSensed(self->stimuli[i], ped);
        }
        if (*best == nullptr || Stimuli_IsBetter(self, self->stimuli[i], *best, ped)) {
            *best = self->stimuli[i];
        }
    }
}

// 0x4A6150: the one of a source and type (and time, unless -1)
CStimulus* __fastcall Find(CStimulusManager* self, re::Edx, uint8_t* source, uint32_t type, int32_t time) {
    for (int i = self->count; i != 0;) {
        CStimulus* s = self->stimuli[i - 1];
        i--;
        if (s->source != source || s->type != type) {
            continue;
        }
        if (time == -1 || s->time == time) {
            return s;
        }
    }
    return nullptr;
}

// 0x4A61A0: the widest-ranging recent one (within maxAge ms) the ped senses
CStimulus* __fastcall FindStrongestRecent(CStimulusManager* self, re::Edx, uint8_t* ped, uint32_t maxAge) {
    if (At<int32_t>(ped, 0x1370) != 0) {
        return nullptr;
    }
    CStimulus* best = nullptr;
    for (int i = self->count; i != 0;) {
        CStimulus* s = self->stimuli[i - 1];
        i--;
        if (0.0f < s->range && uint32_t(timeInMs - s->time) <= maxAge && Stimulus_CanBeSensedBy(s, ped) &&
            (!best || best->range < s->range)) {
            best = s;
        }
    }
    return best;
}

// 0x4A6220: a broadcast stimulus on for a while (off: time <= 0), added when new.
// (A new entry's `on` dword gets stack garbage above its byte in the game; 0 here)
void __cdecl BroadcastSet(int32_t type, float time) {
    uint32_t i = 0;
    for (; i < g_numBroadcast; i++) {
        if (g_broadcast[i].type == type) {
            if (0.0f < time) {
                g_broadcast[i].timeLeft = time;
                g_broadcast[i].on = 1;
            } else {
                g_broadcast[i].timeLeft = 0.0f;
                g_broadcast[i].on = 0;
            }
            break;
        }
    }
    if (i != g_numBroadcast) {
        return;
    }
    uint32_t n = g_numBroadcast;
    g_broadcast[n].type = type;
    At<uint32_t>(&g_broadcast[n], 4) = 0.0f < time ? 1u : 0u;
    g_broadcast[n].timeLeft = time;
    g_numBroadcast = n + 1;
}

// 0x4A62E0: a broadcast stimulus's entries taken out. (The one moved into a removed
// entry's place is skipped, as in the game)
void __cdecl BroadcastRemove(int32_t type) {
    uint32_t count = g_numBroadcast;
    for (uint32_t i = 0; i < count; i++) {
        if (g_broadcast[i].type != type) {
            continue;
        }
        for (uint32_t j = i; j < g_numBroadcast - 1; j++) {
            g_broadcast[j].type = g_broadcast[j + 1].type;
            At<uint32_t>(&g_broadcast[j], 4) = At<uint32_t>(&g_broadcast[j + 1], 4);
            g_broadcast[j].timeLeft = g_broadcast[j + 1].timeLeft;
            count = g_numBroadcast;
        }
        count--;
        g_numBroadcast = count;
    }
}

// 0x4A63D0: the constructor: its position given or from its entities; radius and range
// -1: the type's; the player's awareness raised by one from him
CStimulus* __fastcall Construct(CStimulus* self, re::Edx, uint32_t type, uint8_t* source, uint8_t* target, const Vec* pos,
                                float radius, float range) {
    self->type = type;
    self->source = source;
    if (source) {
        RegisterReference(source, &self->source);
    }
    self->target = target;
    if (target) {
        RegisterReference(target, &self->target);
    }
    self->pos = {0.0f, 0.0f, 0.0f};
    self->repeats = 0;
    self->radius = radius;
    self->sensed = 0;
    self->unknown29 = 0;
    self->range = range;
    self->expired = 0;
    if (!pos) {
        Stimulus_UpdatePosition(self);
    } else {
        self->pos = *pos;
    }
    uint8_t* info;
    if (self->radius == -1.0f && self->type < kNumTypes && (info = g_stimuli->types[self->type]) != nullptr) {
        self->radius = At<float>(info, 0x24);
    }
    if (self->range == -1.0f && self->type < kNumTypes && (info = g_stimuli->types[self->type]) != nullptr) {
        self->range = At<float>(info, 0x2C);
    }
    if (self->source == playerPed) {
        RaiseAwareness(self->range);
    }
    self->time = timeInMs;
    return self;
}

// 0x4A64F0: the destructor's body: peds reacting to it (+0x1D30) let go, its
// references too
void __fastcall Destruct(CStimulus* self, re::Edx) {
    CPool<uint8_t>* pool = pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (ped && At<CStimulus*>(ped, 0x1D30) == self) {
            At<CStimulus*>(ped, 0x1D30) = nullptr;
        }
    }
    if (self->target) {
        CleanUpOldReference(self->target, &self->target);
    }
    if (self->source) {
        CleanUpOldReference(self->source, &self->source);
    }
}

// 0x4A6560: a frame of the player's awareness (decaying, raised by his noise) and,
// every so many frames, 0xC2C1AC
void __cdecl UpdatePlayer() {
    uint8_t* tuning = g_tuning;
    float step = float(double(g_awarenessDecay) * timeStep);
    g_awareness = float(double(g_awareness) - double(step) / At<float>(tuning, 0x1C) * At<float>(tuning, 0x18));
    if (0.0f > g_awareness) {
        g_awareness = 0.0f;
    }
    UpdatePlayerNoise();
    RaiseAwareness(g_playerNoise);
    if (frameCounter % At<uint32_t>(tuning, 0x24) == 0) {
        g_playerC2C1AC = Ped_49B5F0(playerPed);
    }
}

// 0x4A65F0: the manager made (once), its types loaded (and reloaded with the data)
void __cdecl Init() {
    if (g_stimuli) {
        return;
    }
    CStimulusManager* manager = static_cast<CStimulusManager*>(Malloc(0x2BC));
    if (manager) {
        manager->count = 0;
        AddLoadCallback(loadTypesCallback, manager);
        Stimuli_LoadTypes(manager);
        g_stimuli = manager;
        return;
    }
    g_stimuli = nullptr;
}

// 0x4A6640: the manager gone
void __cdecl Shutdown() {
    CStimulusManager* manager = g_stimuli;
    if (manager) {
        RemoveLoadCallback(loadTypesCallback, manager);
        Free(manager);
        g_stimuli = nullptr;
    }
}

// 0x4A6670: a stimulus happened: the same one again (type, source, target) renewed,
// else a new one (the oldest dropped when full); nothing for a type without data
void __fastcall Add(CStimulusManager* self, re::Edx, uint32_t type, uint8_t* source, uint8_t* target, const Vec* pos,
                    float radius, float range) {
    if (!self->types[type]) {
        return;
    }
    for (int i = self->count; i != 0;) {
        CStimulus* s = self->stimuli[i - 1];
        i--;
        if (s->type == type && s->source == source && s->target == target) {
            s->expired = 0;
            s->sensed = 0;
            s->unknown29 = 0;
            if (timeInMs != s->time) {
                s->repeats++;
                s->time = timeInMs;
            }
            if (s->source == playerPed) {
                RaiseAwareness(s->range);
            }
            return;
        }
    }
    if (self->count == int32_t(kNumTypes)) {
        // (none older than now: the one before the array, as the game reads it)
        int oldest = -1;
        int32_t oldestTime = timeInMs;
        for (int i = kNumTypes; i != 0;) {
            int32_t time = self->stimuli[i - 1]->time;
            i--;
            if (time <= oldestTime) {
                oldest = i;
                oldestTime = time;
            }
        }
        re::Ptr32<CStimulus>* slot = &self->stimuli[0] + oldest;
        if (*slot) {
            DeleteStimulus(*slot);
        }
        *slot = self->stimuli[self->count - 1];
        self->count--;
    }
    CStimulus* s = StimulusPool_New(stimulusPool);
    s = s ? Stimulus_Construct(s, type, source, target, pos, radius, range) : nullptr;
    self->stimuli[self->count] = s;
    self->count++;
    Fn450610(g_C0F78C, s);
}

// 0x4A67D0: the ones from or at the ped (as a ped) dropped
uint8_t __fastcall RemoveForPed(CStimulusManager* self, re::Edx, uint8_t* ped) {
    uint8_t removed = 0;
    for (int i = self->count; i != 0;) {
        CStimulus* s = self->stimuli[i - 1];
        i--;
        if (IfPed(s->source) != ped && IfPed(s->target) != ped) {
            continue;
        }
        if (s) {
            DeleteStimulus(s);
        }
        self->stimuli[i] = self->stimuli[self->count - 1];
        self->count--;
        removed = 1;
    }
    return removed;
}

// 0x4A6880: the ones of a type from or at the ped dropped
uint8_t __fastcall RemoveForPedOfType(CStimulusManager* self, re::Edx, uint8_t* ped, uint32_t type) {
    uint8_t removed = 0;
    for (int i = self->count; i != 0;) {
        CStimulus* s = self->stimuli[i - 1];
        i--;
        if (s->type != type) {
            continue;
        }
        if (IfPed(s->source) != ped && IfPed(s->target) != ped) {
            continue;
        }
        if (s) {
            DeleteStimulus(s);
        }
        self->stimuli[i] = self->stimuli[self->count - 1];
        self->count--;
        removed = 1;
    }
    return removed;
}

// 0x4A6940: all dropped
void __fastcall Clear(CStimulusManager* self, re::Edx) {
    for (int i = self->count; i != 0;) {
        CStimulus* s = self->stimuli[i - 1];
        i--;
        if (s) {
            DeleteStimulus(s);
        }
    }
    self->count = 0;
}

// 0x4A69A0: the broadcast stimuli counted down (the timed-out ones taken out), the rest
// given to every ped not in state 0xD
uint8_t __cdecl BroadcastUpdate() {
    float step = Fn45B840();
    uint32_t i = 0;
    if (g_numBroadcast == 0) {
        return 1;
    }
    while (i < g_numBroadcast) {
        if (g_broadcast[i].on) {
            g_broadcast[i].timeLeft = g_broadcast[i].timeLeft - step;
            if (!(0.0f < g_broadcast[i].timeLeft)) {
                Broadcast_RemoveAt(g_broadcast, i);
                continue;
            }
        }
        i++;
    }
    if (g_numBroadcast == 0) {
        return 1;
    }
    CPool<uint8_t>* pool = pedPool;
    CStimulusManager* manager = g_stimuli;
    for (int slot = 0; slot < pool->size; slot++) {
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (!ped || At<int32_t>(ped, 0x1310) == 0xD) {
            continue;
        }
        for (uint32_t j = 0; j < g_numBroadcast; j++) {
            Stimuli_Add(manager, g_broadcast[j].type, ped, nullptr, nullptr, -1.0f, -1.0f);
        }
    }
    return 1;
}

// 0x4A6AC0: all dropped (when there's a manager)
void __cdecl ClearAll() {
    if (g_stimuli) {
        Stimuli_Clear(g_stimuli);
    }
}

// 0x4A6AD0: the expired ones dropped; the rest following their entity (types that
// do) moved
void __fastcall RemoveExpired(CStimulusManager* self, re::Edx) {
    for (int i = self->count; i != 0;) {
        CStimulus* s = self->stimuli[i - 1];
        i--;
        uint8_t* info;
        if (!s->expired && (s->source || s->target) && s->type <= 0x56 && (info = g_stimuli->types[s->type]) != nullptr &&
            ExpiryOf(s, info) >= timeInMs) {
            if (s->type <= 0x56 && (info = g_stimuli->types[s->type]) != nullptr && At<uint8_t>(info, 0x35)) {
                Stimulus_UpdatePosition(s);
            }
            continue;
        }
        if (s) {
            DeleteStimulus(s);
        }
        self->stimuli[i] = self->stimuli[self->count - 1];
        self->count--;
    }
}

// 0x4A6BB0: 0x465580 by the name's hash (a lookup on this object, not the stimuli's)
int32_t __fastcall FindByName(void* self, re::Edx, const char* name, void* arg) {
    return Fn465580(self, HashName(name), arg);
}

} // namespace c09stimulus

// ---- registration ---------------------------------------------------------------------

#define STIMULUS_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

STIMULUS_REVERSED("CStimulusManager::LoadTypes", 0x004A53F0, &c09stimulus::LoadTypes);
STIMULUS_REVERSED("CStimulusManager::UpdatePlayerNoise", 0x004A5470, &c09stimulus::UpdatePlayerNoise);
STIMULUS_REVERSED("CStimulusManager::RaiseAwareness", 0x004A5A40, &c09stimulus::RaiseAwarenessTo);
STIMULUS_REVERSED("CStimulusManager::GetPlayerNoise", 0x004A5A90, &c09stimulus::GetPlayerNoise);
STIMULUS_REVERSED("CBroadcastStimulusManager::RemoveAt", 0x004A5AA0, &c09stimulus::BroadcastRemoveAt);
STIMULUS_REVERSED("CBroadcastStimulusManager::Destroy", 0x004A5AF0, &c09stimulus::BroadcastManagerDestroy);
STIMULUS_REVERSED("CStimulus::CanBeSensedBy", 0x004A5B10, &c09stimulus::CanBeSensedBy);
STIMULUS_REVERSED("CStimulus::HasExpiredBy", 0x004A5C50, &c09stimulus::HasExpiredBy);
STIMULUS_REVERSED("CStimulus::GetSourcePed", 0x004A5CB0, &c09stimulus::GetSourcePed);
STIMULUS_REVERSED("CStimulus::GetTargetPed", 0x004A5CD0, &c09stimulus::GetTargetPed);
STIMULUS_REVERSED("CStimulus::GetPriority", 0x004A5CF0, &c09stimulus::GetPriority);
STIMULUS_REVERSED("CStimulus::UpdatePosition", 0x004A5D10, &c09stimulus::UpdatePosition);
STIMULUS_REVERSED("CStimulus::OnSensed", 0x004A5D80, &c09stimulus::OnSensed);
STIMULUS_REVERSED("CStimulusManager::IsBetter", 0x004A5E60, &c09stimulus::IsBetter);
STIMULUS_REVERSED("CStimulusManager::FindBestFor", 0x004A6080, &c09stimulus::FindBestFor);
STIMULUS_REVERSED("CStimulusManager::Find", 0x004A6150, &c09stimulus::Find);
STIMULUS_REVERSED("CStimulusManager::FindStrongestRecent", 0x004A61A0, &c09stimulus::FindStrongestRecent);
STIMULUS_REVERSED("CBroadcastStimulusManager::Set", 0x004A6220, &c09stimulus::BroadcastSet);
STIMULUS_REVERSED("CBroadcastStimulusManager::Remove", 0x004A62E0, &c09stimulus::BroadcastRemove);
STIMULUS_REVERSED("CStimulus::Construct", 0x004A63D0, &c09stimulus::Construct);
STIMULUS_REVERSED("CStimulus::Destruct", 0x004A64F0, &c09stimulus::Destruct);
STIMULUS_REVERSED("CStimulusManager::UpdatePlayer", 0x004A6560, &c09stimulus::UpdatePlayer);
STIMULUS_REVERSED("CStimulusManager::Init", 0x004A65F0, &c09stimulus::Init);
STIMULUS_REVERSED("CStimulusManager::Shutdown", 0x004A6640, &c09stimulus::Shutdown);
STIMULUS_REVERSED("CStimulusManager::Add", 0x004A6670, &c09stimulus::Add);
STIMULUS_REVERSED("CStimulusManager::RemoveForPed", 0x004A67D0, &c09stimulus::RemoveForPed);
STIMULUS_REVERSED("CStimulusManager::RemoveForPedOfType", 0x004A6880, &c09stimulus::RemoveForPedOfType);
STIMULUS_REVERSED("CStimulusManager::Clear", 0x004A6940, &c09stimulus::Clear);
STIMULUS_REVERSED("CBroadcastStimulusManager::Update", 0x004A69A0, &c09stimulus::BroadcastUpdate);
STIMULUS_REVERSED("CStimulusManager::ClearAll", 0x004A6AC0, &c09stimulus::ClearAll);
STIMULUS_REVERSED("CStimulusManager::RemoveExpired", 0x004A6AD0, &c09stimulus::RemoveExpired);
STIMULUS_REVERSED("CEntity::FindByNameHash", 0x004A6BB0, &c09stimulus::FindByName);
