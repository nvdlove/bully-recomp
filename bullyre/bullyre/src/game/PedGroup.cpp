// CPedGroup (0x489520 .. 0x489F30)

#include "game/Ped.h"

#include <cstring>

#include "game/Crt.h"
#include "reversed.h"
namespace {
inline re::Ptr32<void>* const PG_vtable = reinterpret_cast<re::Ptr32<void>*>(0x0090FAA8);
GAME_FN(void, __cdecl, PG_Free, 0x005EEAB0, void* p);
inline void* __cdecl PG_Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
// a registered entity pointer set (the old reference cleaned up, the new registered)
GAME_FN(void, __thiscall, PG_RefSet, 0x005D7F10, void* slot, void* entity);
GAME_FN(void, __thiscall, PG_Destruct, 0x00489670, CPedGroup* group);
} // namespace

CPedGroup* CPedGroup::Construct(int max, int value34, int value38, int value3C) {
    vtable = PG_vtable;
    for (re::Ptr32<CPed>& member : members) {
        member = nullptr;
    }
    count = 0;
    maxMembers = max;
    unknown34 = value34;
    unknown38 = value38;
    unknown3C = value3C;
    unknown40 = 0;
    unknown44 = 0;
    unknown68[0] = 0.0f;
    unknown68[1] = 0.0f;
    unknown68[2] = 0.0f;
    PG_Memset(&bits48, 0, 0x30);
    bits54 &= 0xFFFFFFFC;
    flags64 &= 0xFE;
    unknown58 = 1000;
    flags50 = static_cast<uint8_t>((flags50 & 5) | 0xAA);
    unknown51 = 0xF8;
    flags52 = static_cast<uint8_t>((flags52 & 0xFA) | 2);
    unknown74 = -1;
    leader = nullptr;
    float limit = static_cast<float>(max);
    if (!(limit < 10.0f)) {
        limit = 10.0f;
    }
    maxMembers = static_cast<int>(limit);
    for (int i = 0; i < maxMembers; i++) {
        // (through an empty temporary, as the game's copy does)
        CPed* none = nullptr;
        PG_RefSet(&none, nullptr);
        PG_RefSet(&members[count], none);
        count++;
        PG_RefSet(&none, nullptr);
    }
    bits4C &= 0xFFFFF800;
    bits48 = (bits48 & 0xFE0001FF) | 0x1FF;
    unknown58 = 0;
    unknown78 = 0;
    PG_RefSet(&leader, nullptr);
    return this;
}

void CPedGroup::Destruct() {
    vtable = PG_vtable;
    unknown44 = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (members[i]) {
            members[i]->group = nullptr;
            PG_RefSet(&members[i], nullptr);
        }
    }
    bits4C &= 0xFFFFF800;
    bits48 = (bits48 & 0xFE0001FF) | 0x1FF;
    unknown58 = 0;
    // (the leader and the members' destructors)
    PG_RefSet(&leader, nullptr);
    PG_RefSet(&leader, nullptr);
    for (int i = 9; i >= 0; i--) {
        PG_RefSet(&members[i], nullptr);
    }
}

CPedGroup* CPedGroup::Destroy(uint8_t flags) {
    PG_Destruct(this);
    if (flags & 1) {
        PG_Free(this);
    }
    return this;
}

uint8_t CPedGroup::IsFull() {
    int used = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (members[i]) {
            used++;
        }
    }
    return used >= maxMembers;
}

int32_t* CPedGroup::Get38(int32_t* out) {
    *out = unknown38;
    return out;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

int CPedGroup::CountMembers() {
    int used = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (members[i]) {
            used++;
        }
    }
    return used;
}

void CPedGroup::RemoveMember(CPed* ped) {
    // (looked for through a temporary registered pointer to it)
    CPed* held = nullptr;
    PG_RefSet(&held, ped);
    int found = -1;
    for (uint32_t i = 0; i < count; i++) {
        if (members[i] == held) {
            found = static_cast<int>(i);
            break;
        }
    }
    PG_RefSet(&held, nullptr);
    if (found != -1) {
        PG_RefSet(&members[found], nullptr);
        ped->group = nullptr;
    }
}

#define PEDGROUP_REVERSED(name, address, fn) \
    REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)

PEDGROUP_REVERSED("CPedGroup::Construct", 0x00489520, &CPedGroup::Construct);
PEDGROUP_REVERSED("CPedGroup::Destruct", 0x00489670, &CPedGroup::Destruct);
PEDGROUP_REVERSED("CPedGroup::Destroy", 0x00489F30, &CPedGroup::Destroy);
PEDGROUP_REVERSED("CPedGroup::IsFull", 0x00489F00, &CPedGroup::IsFull);
PEDGROUP_REVERSED("CPedGroup::Get38", 0x00489660, &CPedGroup::Get38);
PEDGROUP_REVERSED("CPedGroup::CountMembers", 0x00489700, &CPedGroup::CountMembers);
PEDGROUP_REVERSED("CPedGroup::RemoveMember", 0x00489770, &CPedGroup::RemoveMember);
