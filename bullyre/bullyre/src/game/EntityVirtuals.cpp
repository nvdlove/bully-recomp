// The entity classes' small virtuals, their type ids, constructors and deleting
// destructors (CEntity, CPhysical, CObject, CProjectile, CVehicle, CAutomobile, CPropAnim
// and the rest of 0x449550..0x44CD60), the autopilot's constructor, the vehicles'
// reference arrays, and creating a prop (0x44CE70).
//
// Fields are reached by offset (At<T>): the classes aren't laid out here.

#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

// ---- the game's --------------------------------------------------------------------

GAME_VAR(int16_t, ev_nextTypeId, 0x00BF3830);    // the next class id to give out
GAME_ARRAY(uint8_t*, ev_modelInfoPtrs, 0x00C67738);
GAME_VAR(uint32_t, ev_timeInMs, 0x00C1A9B4);     // CTimer::m_snTimeInMilliseconds
GAME_VAR(void*, ev_propPoolLike, 0x020C7C48);    // a pool whose slot 0 is the default owner
GAME_VAR(void*, ev_persistentEntity, 0x00CF3120);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// `eh vector constructor iterator` (ptr, element size, count, constructor)
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* p, size_t size, int count, void* ctor);
inline void* const refSlotCtor = reinterpret_cast<void*>(0x00667DA0);
// a registered reference: lets go of the old entity, registers the new one
GAME_FN(void, __thiscall, Ref_Set, 0x005D7F10, re::Ptr32<void>* ref, void* entity);

GAME_FN(void*, __thiscall, Object_Construct, 0x004CF410, void* self);
GAME_FN(void*, __thiscall, Entity_Construct, 0x004656A0, void* self);
GAME_FN(void*, __thiscall, Physical_Construct, 0x00468E90, void* self);
GAME_FN(void, __thiscall, Seats_Construct, 0x004CE7A0, void* seats);
GAME_FN(void*, __thiscall, AutoPilot_Construct, 0x0044A070, void* autoPilot);
GAME_FN(void*, __thiscall, AnimData_Construct, 0x006BA240, void* p);
GAME_FN(void*, __thiscall, VehicleBase_Construct, 0x0044BF60, void* self);
GAME_FN(void, __thiscall, Fn4C7D50, 0x004C7D50, void* p);
GAME_FN(void, __thiscall, EffectSlot_Construct, 0x00668E90, void* p);
GAME_FN(void, __thiscall, Fn4E8D70, 0x004E8D70, void* p);
GAME_FN(void, __thiscall, Fn5F3E50, 0x005F3E50, void* p);
GAME_FN(void, __thiscall, Fn4F0210, 0x004F0210, void* p);
GAME_FN(void, __thiscall, Fn5F3C10, 0x005F3C10, void* p);
GAME_FN(void*, __thiscall, Entity_GetMatrix, 0x008B6120, void* self);
GAME_FN(void*, __thiscall, Matrix_Copy, 0x00412A10, void* self, const void* from);
GAME_FN(void, __thiscall, Vehicle_Destruct, 0x004CD130, void* self);
GAME_FN(void, __cdecl, Vehicle_Delete, 0x004CD270, void* p);
GAME_FN(int32_t, __thiscall, Seats_Fn4CE640, 0x004CE640, void* seats);
GAME_FN(void, __thiscall, PropAnim_Fn4ED940, 0x004ED940, void* self);
GAME_FN(uint8_t, __thiscall, ModelInfo_Fn520440, 0x00520440, void* modelInfo);
GAME_FN(void, __cdecl, Fn43C060, 0x0043C060, int, int);
GAME_FN(void, __cdecl, Pool_Fn44A4D0, 0x0044A4D0, void* pool, int);
GAME_FN(void, __thiscall, Fn5DB990, 0x005DB990, void* self);
// creating a prop
GAME_FN(void*, __cdecl, PropAnim_New, 0x004EB180, size_t size);
GAME_FN(void*, __thiscall, PropAnim_Construct2, 0x004EDB20, void* self, int modelIndex, void* owner);
GAME_FN(void, __thiscall, Matrix_Construct, 0x00412A70, void* self, void* attached);
GAME_FN(void, __thiscall, Physical_SetMatrix, 0x0046DE90, void* self, void* matrix, int);
GAME_FN(void, __thiscall, Entity_Fn443AA0, 0x00443AA0, void* self);
GAME_FN(void, __thiscall, PropAnim_Fn4EA450, 0x004EA450, void* self, int, int);
GAME_FN(void, __thiscall, ModelInfo_Fn50EB30, 0x0050EB30, void* modelInfo, void*, int);
GAME_FN(uint8_t*, __cdecl, Entity_Fn51AF50, 0x0051AF50, void* entity);
GAME_FN(void*, __cdecl, Fn56F0F0, 0x0056F0F0, int);
GAME_FN(void, __thiscall, Fn414410, 0x00414410, void* self, void*);
GAME_FN(void, __cdecl, Fn5B2990, 0x005B2990, void* entity);
GAME_FN(void, __thiscall, PropAnim_Fn4EF400, 0x004EF400, void* self, int modelIndex, void* owner);
GAME_FN(void, __cdecl, Fn5EB060, 0x005EB060, void* entity, int);

// ---- class ids ---------------------------------------------------------------------
// Each class takes the next id (0xBF3830) the first time it's asked; IsA compares with
// the class's own and its bases'.

int16_t ClassId(uintptr_t guardAddress, uintptr_t idAddress) {
    uint32_t& guard = *reinterpret_cast<uint32_t*>(guardAddress);
    int16_t& id = *reinterpret_cast<int16_t*>(idAddress);
    if (!(guard & 1)) {
        guard |= 1;
        id = ev_nextTypeId;
        ev_nextTypeId = int16_t(ev_nextTypeId + 1);
    }
    return id;
}
int16_t EntityId() { return ClassId(0x00C0F66C, 0x00C0F668); }
int16_t PhysicalId() { return ClassId(0x00C0F674, 0x00C0F670); }
int16_t VehicleId() { return ClassId(0x00C0F6A4, 0x00C0F6A0); }
int16_t AutomobileId() { return ClassId(0x00C0F6AC, 0x00C0F6A8); }
int16_t RefCountedId() { return ClassId(0x00C0F67C, 0x00C0F678); }

// ---- CEntity / CPhysical / CProjectile ---------------------------------------------

// 0x449550 (thiscall): CProjectile's constructor
void* __fastcall Projectile_Construct(void* self) {
    Object_Construct(self);
    At<uint32_t>(self, 0) = 0x009059C4;
    At<int32_t>(self, 0x248) = 0;
    return self;
}

// 0x449570 (thiscall, vf6): the status (the type byte's high 5 bits)
void __fastcall Entity_SetStatus(void* self, re::Edx, int status) {
    At<uint8_t>(self, 0x108) = uint8_t(uint8_t(status) << 3) | (At<uint8_t>(self, 0x108) & 7);
}

// 0x4495A0 (thiscall, vf1)
uint8_t __fastcall Physical_IsA(void* self, re::Edx, int id) {
    (void)self;
    if (int16_t(id) == PhysicalId()) {
        return 1;
    }
    return int16_t(id) == EntityId();
}

// 0x449610 (thiscall, vf35)
float __fastcall Physical_Vf35(void*) {
    return *reinterpret_cast<const float*>(0x00905A6C);   // 0.008
}

// 0x449620 (thiscall, vf35): its +0x230 times 0.008 (as a double)
float __fastcall Projectile_Vf35(void* self) {
    return float(At<float>(self, 0x230) * *reinterpret_cast<const double*>(0x00905A70));
}

// 0x44A050 (thiscall, vf40): the matrix, returned by value (the caller's buffer)
void* __fastcall Physical_GetMatrixCopy(void* self, re::Edx, void* out) {
    Matrix_Copy(out, Entity_GetMatrix(self));
    return out;
}

// 0x44BFD0 (thiscall, vf7)
void __fastcall Entity_Vf7(void* self, re::Edx, int value) {
    At<int32_t>(self, 0x28) = uint8_t(value);
}

// 0x44BFE0 (thiscall, vf15): 0x48 bytes of arguments, ignored
struct Args48 {
    uint32_t words[18];
};
static_assert(sizeof(Args48) == 0x48);
uint8_t __fastcall Entity_Vf15(void*, re::Edx, Args48) {
    return 0;
}

// 0x44BFF0 (thiscall, vf30)
uint8_t __fastcall Entity_Vf30(void* self) {
    return At<int32_t>(self, 0xCC) != 0;
}

// 0x44C010 (thiscall, vf32)
float __fastcall Entity_Vf32(void*) {
    return *reinterpret_cast<const float*>(0x008FF34C);   // 10000
}

// 0x44CCD0 (thiscall, vf1)
uint8_t __fastcall Entity_IsA(void*, re::Edx, int id) {
    return int16_t(id) == EntityId();
}

// ---- "SimpleRefCounted" (vtable 0x905B3C) -------------------------------------------

// 0x449830 (thiscall, vf0 of its derived classes): is it one (the base's id only)
uint8_t __fastcall RefCounted_IsA(void*, re::Edx, int id) {
    return int16_t(id) == RefCountedId();
}

// 0x449870 (thiscall, vf1): the scalar deleting destructor
void* __fastcall RefCounted_DeletingDtor(void* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = 0x00905B3C;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x449890 (thiscall): a constructor (its +4 made by 0x4E8D70)
void* __fastcall Fn449890(void* self) {
    Fn4E8D70(static_cast<uint8_t*>(self) + 4);
    return self;
}

// 0x44A390 (thiscall): a constructor (+4 as above, +2 -1, +0 and +0x24 0)
void* __fastcall Fn44A390(void* self) {
    Fn4E8D70(static_cast<uint8_t*>(self) + 4);
    At<uint16_t>(self, 2) = 0xFFFF;
    At<uint16_t>(self, 0) = 0;
    At<int32_t>(self, 0x24) = 0;
    return self;
}

// ---- the vector deleting destructors (vf0) -------------------------------------------
// flags & 2: an array made with new[] (its count before it), each destroyed from the
// last, then the block freed; else the one object, and flags & 1 its operator delete.

template <size_t Size, uintptr_t Dtor, uintptr_t Delete>
void* __fastcall VectorDeletingDtor(void* self, re::Edx, int flags) {
    auto dtor = reinterpret_cast<void(__thiscall*)(void*)>(Dtor);
    if (flags & 2) {
        int32_t* block = static_cast<int32_t*>(self) - 1;
        for (int i = block[0] - 1; i >= 0; i--) {
            dtor(static_cast<uint8_t*>(self) + i * Size);
        }
        if (flags & 1) {
            Free(block);
        }
        return block;
    }
    dtor(self);
    if (flags & 1) {
        reinterpret_cast<void(__cdecl*)(void*)>(Delete)(self);
    }
    return self;
}

// ---- the vehicles' reference arrays ---------------------------------------------------
// {references, capacity, count}: the block made with new[] (its count before it)

struct RefArray {
    re::Ptr32<re::Ptr32<void>> refs;        // +0
    uint32_t capacity;  // +4
    uint32_t count;     // +8
};
static_assert(sizeof(RefArray) == 0xC);

// 0x449F80 (thiscall): a new capacity, the references moved over (setCount: count =
// capacity)
void __fastcall RefArray_Resize(RefArray* self, re::Edx, uint32_t capacity, int setCount) {
    re::Ptr32<void>* refs = nullptr;
    self->capacity = capacity;
    if (capacity > 0) {
        // (new[]'s size check: all ones when it overflows)
        uint64_t bytes = uint64_t(capacity) * 4;
        uint32_t size = bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
        size = size + 4 < size ? 0xFFFFFFFFu : size + 4;
        int32_t* block = static_cast<int32_t*>(Malloc(size));
        if (block) {
            block[0] = int32_t(capacity);
            refs = reinterpret_cast<re::Ptr32<void>*>(block + 1);
            VectorConstruct(refs, 4, int(capacity), refSlotCtor);
        } else {
            refs = nullptr;
        }
    }
    if (self->refs) {
        for (uint32_t i = 0; i < self->count; i++) {
            Ref_Set(&refs[i], self->refs[i]);
        }
        if (self->refs) {
            int32_t* block = re::PtrCast<int32_t*>(self->refs) - 1;
            for (int i = block[0] - 1; i >= 0; i--) {
                Ref_Set(&self->refs[i], nullptr);
            }
            Free(block);
        }
    }
    self->refs = refs;
    if (uint8_t(setCount)) {
        self->count = self->capacity;
    }
}

// 0x44A6E0 (thiscall): every reference let go, the block freed
void __fastcall RefArray_Free(RefArray* self) {
    if (!self->refs) {
        return;
    }
    int32_t* block = re::PtrCast<int32_t*>(self->refs) - 1;
    for (int i = block[0] - 1; i >= 0; i--) {
        Ref_Set(&self->refs[i], nullptr);
    }
    Free(block);
    self->refs = nullptr;
}

// ---- CAutoPilot ----------------------------------------------------------------------

// 0x44A070 (thiscall): the constructor
void* __fastcall AutoPilot_ConstructImpl(void* self) {
    At<float>(self, 0x48) = 0.0f;
    At<float>(self, 0x4C) = 0.0f;
    At<int32_t>(self, 0x80) = 0;
    At<float>(self, 0x50) = 0.0f;
    At<float>(self, 0x38) = 10.0f;
    At<uint8_t>(self, 0x44) &= 0xFC;
    At<uint8_t>(self, 0x2E) = 1;
    At<uint8_t>(self, 0x2D) = 1;
    uint8_t flags44 = At<uint8_t>(self, 0x44);
    At<int32_t>(self, 0xC) = -1;
    At<int32_t>(self, 0x10) = -1;
    At<int32_t>(self, 8) = 0;
    At<int32_t>(self, 4) = 0;
    At<int32_t>(self, 0) = 0;
    At<int32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x1C) = 0;
    At<int32_t>(self, 0x18) = 0;
    At<uint8_t>(self, 0x30) = 0;
    At<uint8_t>(self, 0x2F) = 0;
    At<uint8_t>(self, 0x31) = 0;
    At<uint8_t>(self, 0x32) = 0;
    At<uint8_t>(self, 0x33) = 0;
    At<uint8_t>(self, 0x3C) = 10;
    At<uint16_t>(self, 0x74) = 0;
    At<int32_t>(self, 0x7C) = 0;
    uint32_t now = ev_timeInMs;
    At<float>(self, 0x40) = 1.0f;
    At<uint8_t>(self, 0x46) &= 0xFE;
    At<uint32_t>(self, 0x24) = now;
    At<uint32_t>(self, 0x28) = now;
    At<int32_t>(self, 0x84) = -1;
    At<int32_t>(self, 0x88) = -1;
    At<uint8_t>(self, 0x44) = flags44 & 0xF7;
    At<uint8_t>(self, 0x3D) = 0;
    return self;
}

// ---- CVehicle / CAutomobile --------------------------------------------------------------

// 0x44BF60 (thiscall): CVehicle's constructor (as CAutomobile's and CBike's first step)
void* __fastcall Vehicle_ConstructBase(void* self) {
    Physical_Construct(self);
    Seats_Construct(static_cast<uint8_t*>(self) + 0x1C4);
    At<uint32_t>(self, 0) = 0x00905C0C;
    AutoPilot_Construct(static_cast<uint8_t*>(self) + 0x2A8);
    At<int32_t>(self, 0x358) = 0;
    At<int32_t>(self, 0x3D8) = -1;
    At<int32_t>(self, 0x444) = -1;
    At<uint8_t>(self, 0x3DC) = 0;
    At<uint8_t>(self, 0x448) = 0;
    At<float>(self, 0x458) = 0.0f;
    At<float>(self, 0x45C) = 0.0f;
    At<float>(self, 0x460) = 0.0f;
    At<int32_t>(self, 0x450) = 0;
    AnimData_Construct(static_cast<uint8_t*>(self) + 0x47C);
    return self;
}

// 0x44C020 (thiscall, vf1)
uint8_t __fastcall Vehicle_IsA(void*, re::Edx, int id) {
    return int16_t(id) == VehicleId();
}

// 0x44C080 (thiscall, vf52): the seats' 0x4CE640
int32_t __fastcall Vehicle_Vf52(void* self) {
    return Seats_Fn4CE640(static_cast<uint8_t*>(self) + 0x1C4);
}

// 0x44C090 (thiscall, vf56): the steering angle
void __fastcall Vehicle_SetSteerAngle(void* self, re::Edx, float angle) {
    At<float>(self, 0x2A0) = angle;
}

// 0x44C0A0 (thiscall, vf0): the scalar deleting destructor
void* __fastcall Vehicle_DeletingDtor(void* self, re::Edx, int flags) {
    Vehicle_Destruct(self);
    if (flags & 1) {
        Vehicle_Delete(self);
    }
    return self;
}

// 0x44C0C0 (thiscall): CAutomobile's constructor
void* __fastcall Automobile_Construct(void* self) {
    uint8_t* p = static_cast<uint8_t*>(self);
    VehicleBase_Construct(self);
    At<uint32_t>(self, 0) = 0x00905D04;
    Fn4C7D50(p + 0x580);
    At<float>(self, 0x580) = 0.5f;
    At<uint8_t>(self, 0x598) = 1;
    // six 0x24-byte entries from +0x59C (their last three floats 0, then all of it 0)
    for (int i = 0; i < 6; i++) {
        uint8_t* e = p + 0x59C + i * 0x24;
        At<float>(e, 0x18) = 0.0f;
        At<float>(e, 0x1C) = 0.0f;
        At<float>(e, 0x20) = 0.0f;
        for (int k = 0; k < 9; k++) {
            At<int32_t>(e, k * 4) = 0;
        }
    }
    const float maxFloat = *reinterpret_cast<const float*>(0x009009B8);   // FLT_MAX
    for (int off : {0x6C4, 0x6C8, 0x6CC, 0x6D4, 0x6D8, 0x6DC}) {
        At<float>(self, off) = 0.0f;
    }
    for (int off : {0x6D0, 0x6F8, 0x720, 0x748}) {
        At<int32_t>(self, off) = 0;
    }
    At<float>(self, 0x6E8) = maxFloat;
    for (int off : {0x6EC, 0x6F0, 0x6F4, 0x6FC, 0x700, 0x704, 0x714, 0x718, 0x71C}) {
        At<float>(self, off) = 0.0f;
    }
    At<float>(self, 0x710) = maxFloat;
    for (int off : {0x724, 0x728, 0x72C, 0x73C, 0x740, 0x744}) {
        At<float>(self, off) = 0.0f;
    }
    At<float>(self, 0x738) = maxFloat;
    At<float>(self, 0x74C) = 0.0f;
    At<float>(self, 0x750) = 0.0f;
    At<float>(self, 0x754) = 0.0f;
    At<float>(self, 0x760) = maxFloat;
    for (int off = 0x80C; off <= 0x838; off += 4) {
        At<float>(self, off) = 0.0f;
    }
    At<float>(self, 0x864) = 0.0f;
    At<float>(self, 0x868) = 0.0f;
    At<float>(self, 0x86C) = 0.0f;
    for (int i = 0; i < 4; i++) {
        EffectSlot_Construct(p + 0x870 + i * 8);
    }
    for (int off = 0x8A0; off <= 0x8BC; off += 4) {
        At<int32_t>(self, off) = 0;
    }
    return self;
}

// 0x44C2B0 (thiscall, vf1)
uint8_t __fastcall Automobile_IsA(void*, re::Edx, int id) {
    if (int16_t(id) == AutomobileId()) {
        return 1;
    }
    return int16_t(id) == VehicleId();
}

// 0x44C320 (thiscall, vf54)
float __fastcall Automobile_Vf54(void* self) {
    return At<float>(self, 0x7F8);
}

// ---- CPropAnim ----------------------------------------------------------------------------

// 0x44CC50 (thiscall): the constructor
void* __fastcall PropAnim_Construct(void* self) {
    uint8_t* p = static_cast<uint8_t*>(self);
    Entity_Construct(self);
    At<uint32_t>(self, 0) = 0x0090626C;
    Fn5F3E50(p + 0x128);
    Fn4F0210(p + 0x160);
    Fn5F3C10(p + 0x1BC);
    At<int32_t>(self, 0x234) = 0;
    At<int32_t>(self, 0x240) = 0;
    At<int32_t>(self, 0x280) = 0;
    At<int32_t>(self, 0x284) = 0;
    At<int32_t>(self, 0x288) = 0;
    AnimData_Construct(p + 0x294);
    At<void*>(self, 0x39C) = nullptr;
    At<int32_t>(self, 0x398) = 0;
    Ref_Set(&At<void*>(self, 0x39C), nullptr);
    At<uint8_t>(self, 0x3A0) = 1;
    return self;
}

// 0x44CD10 (thiscall, vf30)
uint8_t __fastcall PropAnim_Vf30(void* self) {
    return At<int32_t>(self, 0x174) != 0;
}

// 0x44CD20 (thiscall, vf16): 0x4ED940, then the props' area (0xA453F8)
void* __fastcall PropAnim_Vf16(void* self) {
    PropAnim_Fn4ED940(self);
    return reinterpret_cast<void*>(0x00A453F8);
}

// 0x44CD30 (thiscall, vf33)
uint8_t __fastcall PropAnim_Vf33(void* self) {
    if (ModelInfo_Fn520440(ev_modelInfoPtrs[At<int16_t>(self, 0x10E)])) {
        return 0;
    }
    return At<uint8_t>(self, 0x3A8);
}

// 0x44CD60 (cdecl): 0x44A4D0 over every entity pool (0x43C060 between each)
void __cdecl Pools_Fn44CD60() {
    static const uintptr_t pools[] = {0x00C0F5F0, 0x00C0F5F4, 0x00C0F614, 0x00C0F618, 0x00C0F61C,
                                      0x00C0F600, 0x00C0F608, 0x00C0F5F8, 0x00C0F5FC, 0x00C0F60C};
    for (uintptr_t pool : pools) {
        Fn43C060(1, 0);
        Pool_Fn44A4D0(*reinterpret_cast<re::Ptr32<void>*>(pool), 0);
    }
    Fn43C060(1, 0);
    Fn5DB990(reinterpret_cast<void*>(0x00D02850));
    Fn43C060(1, 0);
}

// 0x44CE70 (cdecl): a prop made and placed (owner: nullptr for the default one, slot 0
// of the pool at 0x20C7C48)
void* __cdecl PropAnim_Create(int modelIndex, int flag111, void* matrix, float value11C, uint8_t* owner,
                              int flag3A8, int32_t value3AC) {
    auto defaultOwner = []() -> uint8_t* {
        uint8_t* pool = static_cast<uint8_t*>(ev_propPoolLike);
        uint8_t* flags = *reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4);
        return (flags[0] & 0x80) ? nullptr : *reinterpret_cast<re::Ptr32<uint8_t>*>(pool);
    };
    if (!owner) {
        uint8_t* pool = static_cast<uint8_t*>(ev_propPoolLike);
        if (!(**reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4) & 0x80)) {
            owner = *reinterpret_cast<re::Ptr32<uint8_t>*>(pool);
        }
    }
    void* mem = PropAnim_New(0x3B8);
    // (no check after a failed allocation: the game writes through nullptr)
    uint8_t* prop = mem ? static_cast<uint8_t*>(PropAnim_Construct2(mem, modelIndex, owner)) : nullptr;
    At<float>(prop, 0x11C) = value11C;
    At<uint8_t>(prop, 0x111) = uint8_t(flag111);
    At<uint8_t>(prop, 0x3A8) = uint8_t(flag3A8);
    vc::Call<void>(prop, 0x24, modelIndex);   // SetModelIndex
    alignas(16) uint8_t local[0x44];
    *reinterpret_cast<re::Ptr32<void>*>(local + 0x40) = matrix;
    Matrix_Construct(local, matrix);
    Physical_SetMatrix(prop, local, 0);
    Entity_Fn443AA0(prop);
    At<int32_t>(prop, 0x3AC) = value3AC;
    if (owner != defaultOwner()) {
        At<void*>(owner, 0x1C) = prop;
        PropAnim_Fn4EA450(prop, 0xD, (owner[0x3F] >> 3) & 1);
    }
    uint8_t* info = ev_modelInfoPtrs[modelIndex];
    if (At<int32_t>(info, 0xC) == 0) {
        ModelInfo_Fn50EB30(info, ev_persistentEntity, 0);
    }
    uint16_t id = At<uint16_t>(Entity_Fn51AF50(prop), 0x30);
    uint8_t buffer[0x10];
    void* v = vc::Call<void*>(prop, 0x2C, static_cast<void*>(buffer));
    Fn414410(Fn56F0F0(id), v);
    Fn5B2990(prop);
    PropAnim_Fn4EF400(prop, modelIndex, owner);
    if (ModelInfo_Fn520440(ev_modelInfoPtrs[At<int16_t>(prop, 0x10E)])) {
        Fn5EB060(prop, At<int32_t>(prop, 0x180) == 0);
    }
    return prop;
}

// ---- registration -------------------------------------------------------------------------

#define C04A_ENT(name, address, fn)                                                            \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04A_ENT("CProjectile::Construct", 0x00449550, &Projectile_Construct);
C04A_ENT("CEntity::SetStatus", 0x00449570, &Entity_SetStatus);
RE_UNHOOKED("CEntity::vf29", 0x00449590);   // (jmp 0x465E10)
C04A_ENT("CPhysical::IsA", 0x004495A0, &Physical_IsA);
C04A_ENT("CPhysical::vf35", 0x00449610, &Physical_Vf35);
C04A_ENT("CProjectile::vf35", 0x00449620, &Projectile_Vf35);
C04A_ENT("SimpleRefCounted::IsA", 0x00449830, &RefCounted_IsA);
C04A_ENT("SimpleRefCounted::DeletingDtor", 0x00449870, &RefCounted_DeletingDtor);
C04A_ENT("Fn449890::Construct", 0x00449890, &Fn449890);
C04A_ENT("CAutomobile::VectorDeletingDtor", 0x004499D0, (&VectorDeletingDtor<0x8C0, 0x004A89E0, 0x004CD270>));
C04A_ENT("CPlayerPed::VectorDeletingDtor", 0x00449A40, (&VectorDeletingDtor<0x1F44, 0x0049A620, 0x004789F0>));
C04A_ENT("CObject::VectorDeletingDtor", 0x00449AB0, (&VectorDeletingDtor<0x22C, 0x004CFBA0, 0x004D0690>));
C04A_ENT("CProjectile::VectorDeletingDtor", 0x00449B20, (&VectorDeletingDtor<0x24C, 0x004D15E0, 0x004D2650>));
C04A_ENT("CCutsceneObject::VectorDeletingDtor", 0x00449B90, (&VectorDeletingDtor<0x33C, 0x004CEEF0, 0x004CF1D0>));
C04A_ENT("CDummy::VectorDeletingDtor", 0x00449C00, (&VectorDeletingDtor<0x128, 0x00465180, 0x00465420>));
C04A_ENT("CPropAnim::VectorDeletingDtor", 0x00449C70, (&VectorDeletingDtor<0x3B8, 0x004EE950, 0x004EB190>));
C04A_ENT("CBuilding::VectorDeletingDtor", 0x00449CE0, (&VectorDeletingDtor<0x120, 0x004CECA0, 0x004CED80>));
C04A_ENT("CTreadable::VectorDeletingDtor", 0x00449D50, (&VectorDeletingDtor<0x120, 0x004CEE20, 0x004CEE30>));
C04A_ENT("CAccessory::VectorDeletingDtor", 0x00449DC0, (&VectorDeletingDtor<0x2E8, 0x004A6BE0, 0x004A7340>));
C04A_ENT("TargetingSystem::VectorDeletingDtor", 0x00449E30, (&VectorDeletingDtor<0xE0, 0x004D83F0, 0x004D79B0>));
C04A_ENT("JointConstraint::VectorDeletingDtor", 0x00449EA0, (&VectorDeletingDtor<0x80, 0x0072F6B0, 0x00731750>));
C04A_ENT("SFXItem::VectorDeletingDtor", 0x00449F10, (&VectorDeletingDtor<0x74, 0x0059CEA0, 0x0059D910>));
C04A_ENT("RefArray::Resize", 0x00449F80, &RefArray_Resize);
C04A_ENT("CPhysical::GetMatrixCopy", 0x0044A050, &Physical_GetMatrixCopy);
C04A_ENT("CAutoPilot::Construct", 0x0044A070, &AutoPilot_ConstructImpl);
C04A_ENT("Fn44A390::Construct", 0x0044A390, &Fn44A390);
C04A_ENT("RefArray::Free", 0x0044A6E0, &RefArray_Free);
C04A_ENT("CVehicle::ConstructBase", 0x0044BF60, &Vehicle_ConstructBase);
C04A_ENT("CEntity::vf7", 0x0044BFD0, &Entity_Vf7);
C04A_ENT("CEntity::vf15", 0x0044BFE0, &Entity_Vf15);
C04A_ENT("CEntity::vf30", 0x0044BFF0, &Entity_Vf30);
RE_UNHOOKED("CEntity::vf31", 0x0044C000);   // (jmp 0x512760)
C04A_ENT("CEntity::vf32", 0x0044C010, &Entity_Vf32);
C04A_ENT("CVehicle::IsA", 0x0044C020, &Vehicle_IsA);
C04A_ENT("CVehicle::vf52", 0x0044C080, &Vehicle_Vf52);
C04A_ENT("CVehicle::SetSteerAngle", 0x0044C090, &Vehicle_SetSteerAngle);
C04A_ENT("CVehicle::DeletingDtor", 0x0044C0A0, &Vehicle_DeletingDtor);
C04A_ENT("CAutomobile::Construct", 0x0044C0C0, &Automobile_Construct);
C04A_ENT("CAutomobile::IsA", 0x0044C2B0, &Automobile_IsA);
C04A_ENT("CAutomobile::vf54", 0x0044C320, &Automobile_Vf54);
C04A_ENT("CPropAnim::Construct", 0x0044CC50, &PropAnim_Construct);
C04A_ENT("CEntity::IsA", 0x0044CCD0, &Entity_IsA);
C04A_ENT("CPropAnim::vf30", 0x0044CD10, &PropAnim_Vf30);
C04A_ENT("CPropAnim::vf16", 0x0044CD20, &PropAnim_Vf16);
C04A_ENT("CPropAnim::vf33", 0x0044CD30, &PropAnim_Vf33);
C04A_ENT("Pools::Fn44CD60", 0x0044CD60, &Pools_Fn44CD60);
C04A_ENT("CPropAnim::Create", 0x0044CE70, &PropAnim_Create);

// (under 5 bytes: written, not hooked)
// 0x44C060 (thiscall, ObjectInstanceLoaderImpl vf0): nothing (four arguments)
[[maybe_unused]] void __fastcall ObjectInstanceLoader_Vf0(void*, re::Edx, int, int, int, int) {}
// 0x0044C060: [c2 10 00 cc cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void __cdecl Tiny_0044C060() {}
RE_UNHOOKED_FN("ObjectInstanceLoaderImpl::vf0", 0x0044C060, &Tiny_0044C060);
// 0x44C070 (thiscall, TriggerLoaderImpl vf7): nothing (three arguments)
[[maybe_unused]] void __fastcall TriggerLoader_Vf7(void*, re::Edx, int, int, int) {}
// 0x0044C070: [c2 0c 00 cc cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void __cdecl Tiny_0044C070() {}
RE_UNHOOKED_FN("TriggerLoaderImpl::vf7", 0x0044C070, &Tiny_0044C070);

} // namespace
