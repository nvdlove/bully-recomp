// CCivilianPed (vtable 0x90C804): its constructor (its gift, stats and first objective:
// wander, follow a path or idle), destructor and vf12; the peds' class ids (lazily
// numbered from 0xBF3830); and the matrix pool's set-up (gMatrixList).

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "reversed.h"

namespace {

inline re::Ptr32<void>* const s_civilianPedVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090C804);
inline void* const s_gifts = reinterpret_cast<void*>(0x020C3698);
GAME_ARRAY(int32_t, s_giftCounts, 0x020C3B9C);
GAME_VAR(uint32_t, s_pedCounter, 0x00C2AB5C);
GAME_VAR(uint16_t, s_nextClassId, 0x00BF3830);
// the lazily numbered class ids (and their guards)
GAME_VAR(uint32_t, s_idGuardA, 0x00C0F674);
GAME_VAR(uint16_t, s_idA, 0x00C0F670);
GAME_VAR(uint32_t, s_idGuardB, 0x00C0F66C);
GAME_VAR(uint16_t, s_idB, 0x00C0F668);
GAME_VAR(uint32_t, s_idGuardPed, 0x00C2AB64);
GAME_VAR(uint16_t, s_idPed, 0x00C2AB60);

GAME_FN(void, __thiscall, Ped_Construct, 0x00480130, void* ped, int a, int b);
GAME_FN(int, __thiscall, Gifts_For, 0x006A9A20, void* gifts, void* ped);
GAME_FN(int*, __thiscall, Ped_GetGift, 0x00474AA0, void* ped, int* out);
GAME_FN(void, __thiscall, Ped_SetPedStats, 0x00478F80, void* ped, uintptr_t a, int b);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void*, __thiscall, IdleObjective_Construct, 0x0046FC60, void* self, void* ped);
GAME_FN(void*, __thiscall, FollowPathObjective_Construct, 0x0046FD70, void* self, void* ped);
GAME_FN(void*, __thiscall, WanderObjective_Construct, 0x0046FC80, void* self, void* ped, float speed);
GAME_FN(void, __thiscall, ObjectiveQueue_SetDefault, 0x00470300, void* queue, void* objective);
GAME_FN(uint8_t, __thiscall, Ped_IsAmbient, 0x004765C0, void* ped);
GAME_FN(uint8_t, __cdecl, Fn5BB380, 0x005BB380);
GAME_FN(void, __thiscall, Ped_Fn68EBB0, 0x0068EBB0, void* ped, int a);
GAME_FN(void, __thiscall, Ped_Destruct, 0x00480CC0, void* ped);
GAME_FN(void, __cdecl, Ped_OperatorDelete, 0x004789F0, void* ped);
GAME_FN(void, __thiscall, Ped_Vf12, 0x00481420, void* ped);
GAME_FN(uint8_t, __cdecl, IsClassAOrB_, 0x0046DF20, uint16_t id);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, size_t size, int count, void* constructor);
inline void* const s_matrixLinkConstructor = reinterpret_cast<void*>(0x0046D770);
GAME_FN(void, __thiscall, MatrixLinks_Init, 0x0046D780, void* list, int count);

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

// the matrix pool: CMatrixLinkList (Entity.h) and its array
struct MatrixLinks {
    CMatrixLinkList lists;  // +0
    re::Ptr32<CMatrixLink> array;     // +0x1E0

    void Init(int count);
};
static_assert(offsetof(MatrixLinks, array) == 0x1E0);

// 0x46D780 (thiscall): `count` matrices (new[]) all in the free list, the lists empty
void MatrixLinks::Init(int count) {
    uint64_t bytes = uint64_t(uint32_t(count)) * 0x50;
    uint32_t size = bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
    size = size + 4 < size ? 0xFFFFFFFFu : size + 4;
    uint32_t* block = static_cast<uint32_t*>(Malloc(size));
    CMatrixLink* links = nullptr;
    if (block) {
        block[0] = uint32_t(count);
        links = reinterpret_cast<CMatrixLink*>(block + 1);
        ConstructArray(links, 0x50, count, s_matrixLinkConstructor);
    }
    lists.head.next = &lists.tail;
    lists.tail.prev = &lists.head;
    array = links;
    lists.allocatedHead.next = &lists.allocatedTail;
    lists.allocatedTail.prev = &lists.allocatedHead;
    lists.freeHead.next = &lists.freeTail;
    lists.freeTail.prev = &lists.freeHead;
    for (int i = count - 1; i >= 0; i--) {
        CMatrixLink* link = &array[i];
        link->next = lists.freeHead.next;
        lists.freeHead.next->prev = link;
        link->prev = &lists.freeHead;
        lists.freeHead.next = link;
    }
}

// 0x46DBF0: gMatrixList's 750 matrices (a static initialiser)
void __cdecl InitMatrixList() {
    MatrixLinks_Init(reinterpret_cast<void*>(0x00C2A930), 750);
}

// 0x46DF10: the low byte
uint32_t __cdecl LowByte(uint32_t value) {
    return value & 0xFF;
}

uint16_t ClassId(uint32_t& guard, uint16_t& id) {
    if (!(guard & 1)) {
        guard |= 1;
        id = s_nextClassId;
        s_nextClassId = uint16_t(s_nextClassId + 1);
    }
    return id;
}

// 0x46DF20: the id is one of two base classes' (numbered on first use)
uint8_t __cdecl IsClassAOrB(uint16_t id) {
    if (id == ClassId(s_idGuardA, s_idA)) {
        return 1;
    }
    return id == ClassId(s_idGuardB, s_idB);
}

// 0x46DF90 CPed vf1 (thiscall): IsA
uint8_t __fastcall PedIsA(void*, re::Edx, uint16_t id) {
    if (id == ClassId(s_idGuardPed, s_idPed)) {
        return 1;
    }
    return IsClassAOrB_(id);
}

// 0x46DFE0 CPed vf43 (thiscall): +0x1D2C
int32_t __fastcall PedGet1D2C(uint8_t* self, re::Edx) {
    return Field<int32_t>(self, 0x1D2C);
}

// 0x46DFF0 (thiscall): CCivilianPed's constructor
void* __fastcall CivilianPedConstruct(uint8_t* self, re::Edx, int a, int behaviour, int stats) {
    Ped_Construct(self, a, behaviour);
    Field<re::Ptr32<void>*>(self, 0) = s_civilianPedVtable;
    int gift = Gifts_For(s_gifts, self);
    Field<int32_t>(self, 0x1E2C) = gift;
    if (gift != -1) {
        int tmp;
        if (*Ped_GetGift(self, &tmp) == 0) {
            s_giftCounts[Field<int32_t>(self, 0x1E2C)]++;
        }
    }
    Ped_SetPedStats(self, 0, stats);
    void* objective;
    switch (uint8_t(behaviour)) {
    case 1: {
        void* mem = Malloc(0x14);
        objective = mem ? WanderObjective_Construct(mem, self, 0.3f) : nullptr;
        break;
    }
    case 4: {
        void* mem = Malloc(0x40);
        objective = mem ? FollowPathObjective_Construct(mem, self) : nullptr;
        break;
    }
    default: {
        void* mem = Malloc(0x10);
        objective = mem ? IdleObjective_Construct(mem, self) : nullptr;
        break;
    }
    }
    ObjectiveQueue_SetDefault(self + 0x5F0, objective);
    if (Ped_IsAmbient(self) && Fn5BB380()) {
        Field<int32_t>(self, 0x1370) = 1;
    }
    uint32_t counter = s_pedCounter + 1;
    s_pedCounter = counter;
    if (counter > 0x10000) {
        counter = 0;
        s_pedCounter = 0;
    }
    Field<int32_t>(self, 0x1D2C) += int32_t(counter << 8);
    Ped_Fn68EBB0(self, 0);
    return self;
}

// 0x46E110 CCivilianPed vf0 (thiscall): the destructor (flags 1: and freed)
void* __fastcall CivilianPedDestroy(uint8_t* self, re::Edx, uint8_t flags) {
    Field<re::Ptr32<void>*>(self, 0) = s_civilianPedVtable;
    Ped_Destruct(self);
    if (flags & 1) {
        Ped_OperatorDelete(self);
    }
    return self;
}

// 0x46E140 CCivilianPed vf12 (thiscall): CPed's, unless +0x1570 is 3 (state 10: +0x1318 set)
void __fastcall CivilianPedVf12(uint8_t* self, re::Edx) {
    if (Field<uint8_t>(self, 0x1570) == 3) {
        return;
    }
    if (Field<int32_t>(self, 0x1310) == 10) {
        Field<int32_t>(self, 0x1318) = 1;
    }
    Ped_Vf12(self);
}

} // namespace

#define PEDTYPE_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

REVERSED("CMatrixLinkList::Init", 0x0046D780, re::MemberAddress(&MatrixLinks::Init), nullptr, nullptr, re::Mode::Original);
PEDTYPE_C05_REVERSED("InitMatrixList", 0x0046DBF0, &InitMatrixList);
PEDTYPE_C05_REVERSED("LowByte", 0x0046DF10, &LowByte);
PEDTYPE_C05_REVERSED("IsClassAOrB", 0x0046DF20, &IsClassAOrB);
PEDTYPE_C05_REVERSED("CPed::IsA", 0x0046DF90, &PedIsA);
PEDTYPE_C05_REVERSED("CPed::Get1D2C", 0x0046DFE0, &PedGet1D2C);
PEDTYPE_C05_REVERSED("CCivilianPed::CCivilianPed", 0x0046DFF0, &CivilianPedConstruct);
PEDTYPE_C05_REVERSED("CCivilianPed::~CCivilianPed", 0x0046E110, &CivilianPedDestroy);
PEDTYPE_C05_REVERSED("CCivilianPed::vf12", 0x0046E140, &CivilianPedVf12);
