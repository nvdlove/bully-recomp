// The ped's scanner (CPed +0x3BC): who's around it and what it's watching (0x48BE90 ..
// 0x48C500).
//
// +0 the ped, +4 / +8 the peds around (a count and the peds), +0x48 three points to go
// to (x 1e11: free), +0x129 / +0x12C a countdown, +0x130..0x132 button flags, +0x134 a
// ring of 5 (index +0x148), +0x17C the ped watched (registered), +0x180 / +0x184 two
// registered entities, +0x194 a time, +0x198 a list (count +0x210), +0x214 / +0x218 the
// current and previous value, +0x220 up to 2 (value, until) pairs (count +0x230).

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

constexpr float kFree = 99999997952.0f;   // 0x51BA43B7 (0x90FB98)
GAME_VAR(uint32_t, PSC_timeInMs, 0x00C1A9B4);
inline void* const PSC_nodeA = reinterpret_cast<void*>(0x01B65270);
inline void* const PSC_nodeB = reinterpret_cast<void*>(0x01B652AC);

GAME_FN(uint8_t, __thiscall, PSC_Sees48D470, 0x0048D470, void* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSC_ControllerIsPlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(void, __thiscall, PSC_ExpirePairs, 0x0048C200, uint8_t* self);
GAME_FN(void, __thiscall, PSC_CombatLeaveCurrentTarget, 0x00487B70, void* combat);
GAME_FN(void, __thiscall, PSC_CombatClearTargetMark, 0x00486D50, void* combat);
GAME_FN(void, __cdecl, PSC_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, PSC_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, PSC_List478590, 0x00478590, void* list);
GAME_FN(void, __thiscall, PSC_Sight48CEB0, 0x0048CEB0, void* sight);
GAME_FN(void, __thiscall, PSC_CombatResetSlots, 0x00486E20, void* combat);
GAME_FN(void, __thiscall, PSC_Prop4EE060, 0x004EE060, void* prop, void* ped);
GAME_FN(void*, __thiscall, PSC_Entity467B80, 0x00467B80, void* entity);
inline int __cdecl PSC_Rand() { return crt::Rand(); }
GAME_FN(float, __thiscall, PSC_Normalise, 0x00414E10, float* v);

// (inlined): the watched ped dropped (off its combat target first)
void StopWatching(uint8_t* self) {
    uint8_t* ped = At<uint8_t*>(self, 0);
    if (ped && At<void*>(self, 0x17C)) {
        PSC_CombatLeaveCurrentTarget(ped + 0x668);
        PSC_CombatClearTargetMark(At<uint8_t*>(self, 0) + 0x668);
        re::Ptr32<void>& watched = At<void*>(self, 0x17C);
        if (watched) {
            PSC_CleanUpOldReference(watched, &watched);
            watched = nullptr;
        }
        At<int32_t>(self, 0x230) = 0;
    }
}

void DropRef(uint8_t* self, uint32_t offset) {
    re::Ptr32<void>& ref = At<void*>(self, offset);
    if (ref) {
        PSC_CleanUpOldReference(ref, &ref);
        ref = nullptr;
    }
}

void SetRef(re::Ptr32<void>* ref, void* entity) {
    if (entity != *ref) {
        if (*ref) {
            PSC_CleanUpOldReference(*ref, ref);
        }
        *ref = entity;
        if (entity) {
            PSC_RegisterReference(entity, ref);
        }
    }
}

} // namespace

namespace c07psc {

// 0x48BE90: a point queued in the first free place (none when all 3 are taken)
void __fastcall PushPoint(uint8_t* self, re::Edx, float x, float y, float z) {
    int i = 0;
    for (; i < 3; i++) {
        if (kFree == At<float>(self, 0x48 + i * 0xC)) {
            break;
        }
    }
    if (i == 3) {
        return;
    }
    At<float>(self, 0x48 + i * 0xC) = x;
    At<float>(self, 0x4C + i * 0xC) = y;
    At<float>(self, 0x50 + i * 0xC) = z;
}

// 0x48BEE0: a point is queued
uint32_t __fastcall HasPoint(uint8_t* self, re::Edx) {
    return kFree == At<float>(self, 0x48) ? 0 : 1;
}

// 0x48BF00: the first point taken off (the last place freed)
float* __fastcall PopPoint(uint8_t* self, re::Edx, float* out) {
    out[0] = At<float>(self, 0x48);
    out[1] = At<float>(self, 0x4C);
    out[2] = At<float>(self, 0x50);
    for (uint32_t offset = 0x48; offset <= 0x5C; offset += 4) {
        At<uint32_t>(self, offset) = At<uint32_t>(self, offset + 0xC);
    }
    At<float>(self, 0x60) = kFree;
    return out;
}

// 0x48BF50: a value into the ring of 5
void __fastcall RingPush(uint8_t* self, re::Edx, uint32_t value) {
    int32_t& index = At<int32_t>(self, 0x148);
    index++;
    if (index >= 5) {
        index = 0;
    }
    At<uint32_t>(self, 0x134 + index * 4) = value;
}

// 0x48BF80: the ring cleared
void __fastcall RingClear(uint8_t* self, re::Edx) {
    for (uint32_t offset = 0x134; offset <= 0x148; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
}

// 0x48BFB0: the current value (the old one kept as the previous)
void __fastcall SetCurrent(uint8_t* self, re::Edx, uint32_t value) {
    uint32_t current = At<uint32_t>(self, 0x214);
    if (current != value) {
        At<uint32_t>(self, 0x218) = current;
        At<uint32_t>(self, 0x214) = value;
    }
}

// 0x48BFD0: the value is the current or the previous
uint8_t __fastcall IsCurrentOrPrevious(uint8_t* self, re::Edx, uint32_t value) {
    return (value == At<uint32_t>(self, 0x214) || value == At<uint32_t>(self, 0x218)) ? 1 : 0;
}

// 0x48BFF0: the value isn't among the timed pairs
uint8_t __fastcall IsNotPaired(uint8_t* self, re::Edx, const uint32_t* value) {
    for (int i = 0; i < At<int32_t>(self, 0x230); i++) {
        if (At<uint32_t>(self, 0x220 + i * 8) == *value) {
            return 0;
        }
    }
    return 1;
}

// 0x48C030: the watched ped, if it's a ped and alive (+0x1C98 clear)
uint8_t* __fastcall GetWatchedPed(uint8_t* self, re::Edx) {
    uint8_t* ped = At<uint8_t*>(self, 0x17C);
    if (!ped || (At<uint8_t>(ped, 0x108) & 7) != 3 || At<uint8_t>(ped, 0x1C98)) {
        return nullptr;
    }
    return ped;
}

// 0x48C060
void* __fastcall Get184(uint8_t* self, re::Edx) {
    return At<void*>(self, 0x184);
}

// 0x48C070
void* __fastcall Get180(uint8_t* self, re::Edx) {
    return At<void*>(self, 0x180);
}

// 0x48C080: the value is in the list
uint32_t __fastcall ListHas(uint8_t* self, re::Edx, uint32_t value) {
    int found = -1;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x210); i++) {
        if (At<uint32_t>(self, 0x198 + i * 4) == value) {
            found = static_cast<int>(i);
            break;
        }
    }
    return found != -1 ? 1 : 0;
}

// 0x48C0D0: a value added to the list
void __fastcall ListAdd(uint8_t* self, re::Edx, uint32_t value) {
    At<uint32_t>(self, 0x198 + At<uint32_t>(self, 0x210) * 4) = value;
    At<uint32_t>(self, 0x210) += 1;
}

// 0x48C0F0: a value taken out of the list (the last moved into its place)
void __fastcall ListRemove(uint8_t* self, re::Edx, uint32_t value) {
    uint32_t count = At<uint32_t>(self, 0x210);
    for (uint32_t i = 0; i < count; i++) {
        if (At<uint32_t>(self, 0x198 + i * 4) == value) {
            At<uint32_t>(self, 0x198 + i * 4) = At<uint32_t>(self, 0x194 + count * 4);
            At<uint32_t>(self, 0x210) -= 1;
            return;
        }
    }
}

// 0x48C140: `ped` is around and the ped sees it (0x48D470 of CPed +0xA64)
uint8_t __fastcall IsAroundAndSeen(uint8_t* self, re::Edx, void* ped) {
    if (!ped) {
        return 0;
    }
    for (int i = 0; i < At<int32_t>(self, 4); i++) {
        if (At<void*>(self, 8 + i * 4) == ped) {
            return PSC_Sees48D470(At<uint8_t*>(self, 0) + 0xA64, ped);
        }
    }
    return 0;
}

// 0x48C190: +0x194 the time since `time` (0 when that's still ahead)
void __fastcall SetTimeSince(uint8_t* self, re::Edx, uint32_t time) {
    uint32_t now = PSC_timeInMs;
    At<uint32_t>(self, 0x194) = now > time ? now - time : 0;
}

// 0x48C1C0: a value paired with a time `ms` ahead (up to 2)
void __fastcall AddPair(uint8_t* self, re::Edx, const uint32_t* value, uint32_t ms) {
    int count = At<int32_t>(self, 0x230);
    if (count >= 2) {
        return;
    }
    At<uint32_t>(self, 0x220 + count * 8) = *value;
    At<uint32_t>(self, 0x224 + count * 8) = PSC_timeInMs + ms;
    At<int32_t>(self, 0x230) = count + 1;
}

// 0x48C200: the pairs whose time has come dropped (the last one moved into the slot, which
// isn't looked at again)
void __fastcall ExpirePairs(uint8_t* self, re::Edx) {
    for (int i = 0; i < At<int32_t>(self, 0x230); i++) {
        if (At<uint32_t>(self, 0x224 + i * 8) > PSC_timeInMs) {
            continue;
        }
        int last = --At<int32_t>(self, 0x230);
        At<uint32_t>(self, 0x220 + i * 8) = At<uint32_t>(self, 0x220 + last * 8);
        At<uint32_t>(self, 0x224 + i * 8) = At<uint32_t>(self, 0x224 + last * 8);
    }
}

// 0x48C250: a step: the countdown, the button flags from the ped's main actions, and the
// expired pairs dropped (0x48C200)
void __fastcall Update(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x129)) {
        int32_t count = At<int32_t>(self, 0x12C);
        if (!(count > 0)) {
            At<uint8_t>(self, 0x129) = 0;
        }
        At<int32_t>(self, 0x12C) = count - 1;
    }
    if (At<uint8_t>(self, 0x130)) {
        At<uint8_t>(self, 0x130) = 0;
    }
    if (PSC_ControllerIsPlaying(At<uint8_t*>(At<uint8_t*>(self, 0), 0x2E0) + 0x258, PSC_nodeA, 1, 1)) {
        At<uint8_t>(self, 0x131) = 1;
    } else {
        if (At<uint8_t>(self, 0x131)) {
            At<uint8_t>(self, 0x130) = 1;
        }
        At<uint8_t>(self, 0x131) = 0;
    }
    if (PSC_ControllerIsPlaying(At<uint8_t*>(At<uint8_t*>(self, 0), 0x2E0) + 0x258, PSC_nodeB, 1, 1)) {
        At<uint8_t>(self, 0x132) = 1;
    }
    PSC_ExpirePairs(self);
}

// 0x48C300: the constructor for a ped
uint8_t* __fastcall Construct(uint8_t* self, re::Edx, void* ped) {
    At<void*>(self, 0) = ped;
    for (uint32_t offset = 8; offset <= 0x44; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    At<uint32_t>(self, 4) = 0;
    for (uint32_t offset = 0x48; offset <= 0x68; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    static const uint16_t zeros[] = {0x7C, 0x80, 0x88, 0x8C, 0x94, 0x98, 0xA0, 0xA4, 0xAC, 0xB0, 0xB8, 0xBC, 0xC4,
                                     0xC8, 0xD0, 0xD4, 0xDC, 0xE0, 0xE8, 0xEC, 0xF4, 0xF8, 0x100, 0x104, 0x10C};
    for (uint16_t offset : zeros) {
        At<uint32_t>(self, offset) = 0;
    }
    At<float>(self, 0x154) = 0.0f;
    At<float>(self, 0x158) = 0.0f;
    At<uint32_t>(self, 0x14C) = 0;
    At<float>(self, 0x15C) = 0.0f;
    At<uint8_t>(self, 0x164) = 0;
    At<float>(self, 0x168) = 0.5f;
    At<uint32_t>(self, 0x17C) = 0;
    At<uint32_t>(self, 0x180) = 0;
    At<uint32_t>(self, 0x184) = 0;
    At<float>(self, 0x16C) = 0.0f;
    At<uint32_t>(self, 0x188) = 0;
    At<float>(self, 0x170) = 0.0f;
    At<uint32_t>(self, 0x18C) = 0;
    At<float>(self, 0x174) = 0.0f;
    At<uint32_t>(self, 0x190) = 0;
    At<uint32_t>(self, 0x210) = 0;
    At<uint32_t>(self, 0x214) = 0;
    At<uint32_t>(self, 0x218) = 0;
    At<uint8_t>(self, 0x21C) = 0;
    At<float>(self, 0x48) = kFree;
    At<float>(self, 0x54) = kFree;
    At<float>(self, 0x60) = kFree;
    return self;
}

// 0x48C480: the entity at +0x184
void __fastcall Set184(uint8_t* self, re::Edx, void* entity) {
    SetRef(&At<void*>(self, 0x184), entity);
}

// 0x48C4C0: the entity at +0x180
void __fastcall Set180(uint8_t* self, re::Edx, void* entity) {
    SetRef(&At<void*>(self, 0x180), entity);
}

// 0x48C500: a ped to watch (not the ped itself): off its combat target first, the pairs
// cleared
void __fastcall Watch(uint8_t* self, re::Edx, void* ped) {
    if (At<void*>(self, 0) == ped || At<void*>(self, 0x17C) == ped) {
        return;
    }
    PSC_CombatLeaveCurrentTarget(At<uint8_t*>(self, 0) + 0x668);
    PSC_CombatClearTargetMark(At<uint8_t*>(self, 0) + 0x668);
    SetRef(&At<void*>(self, 0x17C), ped);
    At<int32_t>(self, 0x230) = 0;
}

// 0x48C580: reset (the lists, the sight and combat slots, what's watched, a random
// facing)
void __fastcall Reset(uint8_t* self, re::Edx) {
    At<float>(self, 0x48) = kFree;
    PSC_List478590(self + 4);
    PSC_Sight48CEB0(At<uint8_t*>(self, 0) + 0xA64);
    PSC_CombatResetSlots(At<uint8_t*>(self, 0) + 0x668);
    At<int32_t>(self, 0x74) = 0;
    At<uint8_t>(self, 0x120) = 0xFF;
    At<int32_t>(self, 0x124) = 0;
    At<int32_t>(self, 0x110) = -1;
    At<int32_t>(self, 0x78) = 0;
    DropRef(self, 0x7C);
    At<int32_t>(self, 0x80) = 0;
    DropRef(self, 0x88);
    At<int32_t>(self, 0x8C) = 0;
    StopWatching(self);
    DropRef(self, 0x180);
    At<float>(self, 0x70) = 0.0f;
    At<int32_t>(self, 0x6C) = 0;
    At<int32_t>(self, 0x194) = 0;
    At<int32_t>(self, 0x114) = 0;
    At<int32_t>(self, 0x118) = 0;
    At<int32_t>(self, 0x11C) = 0;
    At<uint8_t>(self, 0x128) = 0;
    StopWatching(self);
    DropRef(self, 0x184);
    void* prop = At<void*>(self, 0x18C);
    if (prop) {
        PSC_Prop4EE060(prop, At<void*>(self, 0));
        DropRef(self, 0x18C);
    }
    At<int32_t>(self, 0x210) = 0;
    At<float>(self, 0x160) = 1.0f;
    At<uint8_t>(self, 0x129) = 0;
    At<int32_t>(self, 0x12C) = 0;
    At<float>(self, 0x168) = 0.5f;
    At<uint8_t>(self, 0x130) = 0;
    At<uint8_t>(self, 0x131) = 0;
    At<uint8_t>(self, 0x132) = 0;
    At<uint8_t>(self, 0x150) = 0;
    At<uint8_t>(self, 0x164) = 0;
    float first = static_cast<float>(PSC_Rand() / 32767.0 * 2.0 - 1.0);
    float second = static_cast<float>(PSC_Rand() / 32767.0 * 2.0 - 1.0);
    float facing[3] = {second, first, 0.0f};
    PSC_Normalise(facing);
    At<float>(self, 0x16C) = facing[0];
    At<float>(self, 0x170) = facing[1];
    At<uint8_t>(self, 0x21C) = 0;
    At<float>(self, 0x174) = facing[2];
}

// 0x48C800: an entity remembered with a value and a score (10 at most; one of the same
// kind (0x467B80) or model is replaced when the new score is lower)
void __fastcall Remember(uint8_t* self, re::Edx, uint8_t* entity, uint32_t value, float score) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x10C); i++) {
        uint8_t* entry = self + 0x94 + i * 0xC;
        uint8_t* other = At<uint8_t*>(entry, 0);
        if (!other) {
            continue;
        }
        bool same = false;
        if (PSC_Entity467B80(other)) {
            void* kind = PSC_Entity467B80(At<void*>(entry, 0));
            if (PSC_Entity467B80(entity) == kind) {
                same = true;
            }
        }
        if (!same && At<int16_t>(At<uint8_t*>(entry, 0), 0x10E) == At<int16_t>(entity, 0x10E)) {
            same = true;
        }
        if (same) {
            if (!(At<float>(entry, 8) > score)) {
                return;
            }
            SetRef(&At<void*>(entry, 0), entity);
            At<float>(entry, 8) = score;
            At<uint32_t>(entry, 4) = value;
            return;
        }
    }
    if (At<uint32_t>(self, 0x10C) == 10) {
        return;
    }
    // (the game makes a registered copy of the entity for the entry and drops it after)
    void* copy = nullptr;
    if (entity) {
        copy = entity;
        PSC_RegisterReference(entity, &copy);
    }
    uint8_t* entry = self + 0x94 + At<uint32_t>(self, 0x10C) * 0xC;
    SetRef(&At<void*>(entry, 0), copy);
    At<uint32_t>(entry, 4) = value;
    At<float>(entry, 8) = score;
    At<uint32_t>(self, 0x10C) += 1;
    if (copy) {
        PSC_CleanUpOldReference(copy, &copy);
    }
}

} // namespace c07psc

#define PSC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PSC_REVERSED("PedScanner::PushPoint", 0x0048BE90, &c07psc::PushPoint);
PSC_REVERSED("PedScanner::HasPoint", 0x0048BEE0, &c07psc::HasPoint);
PSC_REVERSED("PedScanner::PopPoint", 0x0048BF00, &c07psc::PopPoint);
PSC_REVERSED("PedScanner::RingPush", 0x0048BF50, &c07psc::RingPush);
PSC_REVERSED("PedScanner::RingClear", 0x0048BF80, &c07psc::RingClear);
PSC_REVERSED("PedScanner::SetCurrent", 0x0048BFB0, &c07psc::SetCurrent);
PSC_REVERSED("PedScanner::IsCurrentOrPrevious", 0x0048BFD0, &c07psc::IsCurrentOrPrevious);
PSC_REVERSED("PedScanner::IsNotPaired", 0x0048BFF0, &c07psc::IsNotPaired);
PSC_REVERSED("PedScanner::GetWatchedPed", 0x0048C030, &c07psc::GetWatchedPed);
PSC_REVERSED("PedScanner::Get184", 0x0048C060, &c07psc::Get184);
PSC_REVERSED("PedScanner::Get180", 0x0048C070, &c07psc::Get180);
PSC_REVERSED("PedScanner::ListHas", 0x0048C080, &c07psc::ListHas);
PSC_REVERSED("PedScanner::ListAdd", 0x0048C0D0, &c07psc::ListAdd);
PSC_REVERSED("PedScanner::ListRemove", 0x0048C0F0, &c07psc::ListRemove);
PSC_REVERSED("PedScanner::IsAroundAndSeen", 0x0048C140, &c07psc::IsAroundAndSeen);
PSC_REVERSED("PedScanner::SetTimeSince", 0x0048C190, &c07psc::SetTimeSince);
PSC_REVERSED("PedScanner::AddPair", 0x0048C1C0, &c07psc::AddPair);
PSC_REVERSED("PedScanner::ExpirePairs", 0x0048C200, &c07psc::ExpirePairs);
PSC_REVERSED("PedScanner::Update", 0x0048C250, &c07psc::Update);
PSC_REVERSED("PedScanner::Construct", 0x0048C300, &c07psc::Construct);
PSC_REVERSED("PedScanner::Set184", 0x0048C480, &c07psc::Set184);
PSC_REVERSED("PedScanner::Set180", 0x0048C4C0, &c07psc::Set180);
PSC_REVERSED("PedScanner::Watch", 0x0048C500, &c07psc::Watch);
PSC_REVERSED("PedScanner::Reset", 0x0048C580, &c07psc::Reset);
PSC_REVERSED("PedScanner::Remember", 0x0048C800, &c07psc::Remember);
