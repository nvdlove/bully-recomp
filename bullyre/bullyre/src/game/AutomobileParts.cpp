// CAutomobile (vtable 0x905D04): its small parts: the wheel state reset, the wheels'
// surface effects, a component's damage shown on its frame, the particles of a hit,
// the destructor's body. Fields past CVehicle are reached by offset (Vehicle.h names
// only a few): the damage manager at +0x580, the frames at +0x674, four wheel records
// of 0x28 bytes at +0x6C4 (+0x8 a float, +0x22 the surface), the wheels' +0x764 /
// +0x784 / +0x79C floats, their +0x794 bytes, +0x7D4 a flag, the wheels' effect modes at
// +0x850, their effects at +0x870 (8 bytes each) with the effect ids at +0x890.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Vehicle.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

using re::At;

struct Vec {
    float x, y, z;
};

GAME_VAR(void*, g_20C8704, 0x020C8704);
GAME_VAR(uint16_t, g_defaultWheelEffect, 0x020C9014);
GAME_VAR(uint16_t, g_interiorWheelEffect, 0x020C8FD4);
GAME_VAR(float, g_speedScale, 0x00C2D170);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(void*, g_effects, 0x00B6F1D4);            // the effect manager
GAME_ARRAY(uint32_t, g_carColours, 0x00CE7128);    // (a colour a car colour index)
inline void* const g_surfaceEffects = reinterpret_cast<void*>(0x00C21BC0);
inline void* const vecUnset = reinterpret_cast<void*>(0x00BCC564);
inline void* const hideDamagedChild = reinterpret_cast<void*>(0x00534BD0);
constexpr uintptr_t vtblAutomobile = 0x00905D04;

GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, Node_SetFlags, 0x006C69D0, void* node, int flags);
GAME_FN(void, __cdecl, Node_ForEachChild, 0x006C9AA0, void* node, void* callback, uintptr_t arg);
GAME_FN(uint8_t*, __cdecl, Node_GetWorldTransform, 0x006CA0A0, void* node);
GAME_FN(void, __cdecl, DamagePartAll, 0x00411EA0, void* rwObject, int frame);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, void* entity);
GAME_FN(Vec*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, Vec* out);
GAME_FN(void, __cdecl, AddParticle, 0x00521080, int type, const Vec* pos, const void* velocity, float size, const void* colour,
        int a, int b);
GAME_FN(uint8_t, __thiscall, Damage_GetPanelStatus, 0x004C7DC0, void* damage, int panel);
GAME_FN(uint8_t, __thiscall, Damage_GetDoorStatus, 0x004C7E00, void* damage, int door);
GAME_FN(void, __thiscall, Damage_SetDoorStatus, 0x004C7DF0, void* damage, int door, int status);
GAME_FN(int32_t, __thiscall, Damage_CountDamagedComponents, 0x004C7F70, void* damage);
GAME_FN(uint8_t, __thiscall, Vehicle_IsUnknown354Normal, 0x004CB210, void* vehicle);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(void, __thiscall, Automobile_ResetEffects, 0x004A84E0, void* automobile);
GAME_FN(void, __thiscall, Effect_Destruct, 0x00668EB0, void* effect);
GAME_FN(void, __thiscall, Vehicle_Destruct, 0x004CD130, void* vehicle);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(uint16_t, __thiscall, SurfaceEffects_Find, 0x004642B0, void* table, uint32_t* surface, uint32_t hash);
GAME_FN(void, __cdecl, Fn5280C0, 0x005280C0, void* id, void* wheel, float x, float y, int surface, int);
GAME_FN(void, __thiscall, Effect_SetStrength_x64, 0x00668C40, void* effect, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Effect_SetStrength(void* a0, float a1) {
    return Effect_SetStrength_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, Effect_SetRate_x64, 0x00668C10, void* effect, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Effect_SetRate(void* a0, float a1) {
    return Effect_SetRate_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, Effect_SlowKill, 0x00668D00, void* effect, float time);
GAME_FN(void, __thiscall, Effect_SetPosition, 0x00669040, void* effect, const void* pos, int);
GAME_FN(void, __thiscall, Effects_Release, 0x00668100, void* manager, void* effect);
GAME_FN(void, __thiscall, Effects_Start, 0x00668290, void* manager, uint32_t* type, void* effect, int);
// (its own, called where modes apply)
GAME_FN(Vec*, __cdecl, RandomisedNear, 0x004A8540, Vec* out, const Vec* pos);

// 0x58A680 takes a registered entity reference by value (a stack slot the caller
// registers and the callee lets go): built here the way the game does
__declspec(noinline) void Call58A680WithRef(void* entity, int arg) {
#if defined(_M_IX86)
    void* fn = reinterpret_cast<void*>(0x0058A680);
    void* setRef = reinterpret_cast<void*>(0x005D7F10);
    __asm {
        push arg
        push ecx
        mov ecx, esp
        mov dword ptr [ecx], 0
        push entity
        call setRef
        call fn
        add esp, 8
    }
#else
    // (the 64-bit build: the reference built in a local, passed by value as the game does)
    struct EntityRef64 {
        void* entity;
    } ref = {nullptr};
    re::Ours<void(__fastcall*)(void*, void*, void*)>(0x005D7F10)(&ref, nullptr, entity);
    re::Ours<void(__cdecl*)(EntityRef64, int)>(0x0058A680)(ref, arg);
#endif
}

float Rand01() {
    return float(double(Rand()) / 32767.0);
}

} // namespace

namespace c09autoparts {

// 0x4A8480: a registered reference made to the entity
re::Ptr32<void>* __fastcall RefConstruct(re::Ptr32<void>* self, re::Edx, void* entity) {
    *self = nullptr;
    SetRef(self, entity);
    return self;
}

// 0x4A84A0: 0x20C8704's +0x3C (0 without it)
int32_t __cdecl Get20C8704_3C() {
    void* object = g_20C8704;
    return object ? At<int32_t>(object, 0x3C) : 0;
}

// 0x4A84B0: a node's flags set (4 when its +0x6C's low bits are `bits`, else 0; a
// callback over a frame's children)
void* __cdecl SetNodeFlagsIfBits(uint8_t* node, uint32_t bits) {
    if ((At<uint32_t>(node, 0x6C) & 3) == bits) {
        Node_SetFlags(node, 4);
        return node;
    }
    Node_SetFlags(node, 0);
    return node;
}

// 0x4A8540: the point moved randomly by up to 1.2 across (x, y)
Vec* __cdecl RandomNear(Vec* out, const Vec* pos) {
    *out = *pos;
    float dx = float(double(Rand()) / 32767.0 * double(2.4f) - double(1.2f));
    out->x = out->x + dx;
    float dy = float(double(Rand()) / 32767.0 * double(2.4f) - double(1.2f));
    out->y = out->y + dy;
    return out;
}

// 0x4A85C0: the particles of a hit at `pos` (as many as the amount says): sparks,
// debris in its colour, dust. Only on screen
void __fastcall HitParticles(CAutomobile* self, re::Edx, const Vec* pos, float amount) {
    if (!Entity_GetIsOnScreen(self)) {
        return;
    }
    int n = int(amount);
    uint8_t colour[4] = {0, 0, 0, 0};
    int sparks = (n / 4) & 0x1F;
    Vec velocity;
    for (int i = sparks; i != 0; i--) {
        velocity = {0.0f, 0.0f, 0.0f};
        velocity.x = float(double(Rand()) / 32767.0 * double(0.2f) - double(0.1f));
        velocity.y = float(double(Rand()) / 32767.0 * double(0.2f) - double(0.1f));
        velocity.z = 0.006f;
        AddParticle(0, pos, &velocity, 0.0f, colour, 0, 0);
    }
    for (int i = n / 8; i > 0; i--) {
        Vec near;
        AddParticle(2, RandomisedNear(&near, pos), vecUnset, 0.5f, colour, 0, 0);
    }
    velocity = {0.0f, 0.0f, 0.0f};
    int debris = n / 20;
    uint32_t debrisColour = g_carColours[self->colour1];
    for (int i = debris; i > 0; i--) {
        velocity.x = float(double(Rand()) / 32767.0 * 0.5 - 0.25);
        velocity.y = float(double(Rand()) / 32767.0 * 0.5 - 0.25);
        velocity.z = float(double(Rand()) / 32767.0 * double(0.15f) + double(0.1f));
        int a = Rand() * 3 / 0x7FFF;
        int b = Rand() * 0x4F / 0x7FFF - 0x28;
        float size = float(double(Rand()) / 32767.0 * double(0.06f) + double(0.02f));
        AddParticle(3, pos, &velocity, size, &debrisColour, b, a);
    }
}

// 0x4A8810: a panel's damage shown: its frame's parts damaged and hidden when it's
// status 1. (The third argument is unused)
void __fastcall ShowPanelDamage(CAutomobile* self, re::Edx, int frame, int panel, int) {
    uint8_t status = Damage_GetPanelStatus(reinterpret_cast<uint8_t*>(self) + 0x580, panel);
    if (self->frames[frame] && status == 1) {
        DamagePartAll(self->rwObject, frame);
        Node_ForEachChild(self->frames[frame], reinterpret_cast<void*>(0x004A84B0), 2);
    }
}

// 0x4A8860: +0x368 counted down to 0
void __fastcall CountDown368(CAutomobile* self, re::Edx) {
    if (self->unknown368 != 0) {
        self->unknown368--;
    }
}

// 0x4A8880: the wheels' state reset (their +0x764 1, compressions 0, angles 0), the
// centre of mass the handling's
void __fastcall ResetWheels(CAutomobile* self, re::Edx) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    float angle = float(double(3.1415927f) * 0.0 / 180.0);
    for (int i = 0; i < 4; i++) {
        At<float>(p, 0x764 + i * 4) = 1.0f;
        At<int32_t>(p, 0x850 + i * 4) = 0;
        At<float>(p, 0x784 + i * 4) = 0.0f;
        At<float>(p, 0x79C + i * 4) = angle;
    }
    self->centreOfMass = *reinterpret_cast<veh::CVector*>(self->handling + 0x1C);
    self->unknown470 = 0.0f;
}

// 0x4A8990: its damage manager's damaged components counted
int32_t __fastcall CountDamagedComponents(CAutomobile* self, re::Edx) {
    return Damage_CountDamagedComponents(reinterpret_cast<uint8_t*>(self) + 0x580);
}

// 0x4A89A0: +0x10, +0x14, +0x18 set (on whatever this is)
void __fastcall Set10(uint8_t* self, re::Edx, float x, float y, float z) {
    At<float>(self, 0x10) = x;
    At<float>(self, 0x14) = y;
    At<float>(self, 0x18) = z;
}

// 0x4A89E0: the destructor's body: its effects reset and destroyed, the wheels'
// references let go, then CVehicle's
void __fastcall Destruct(CAutomobile* self, re::Edx) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(vtblAutomobile);
    Automobile_ResetEffects(self);
    for (int i = 3; i >= 0; i--) {
        Effect_Destruct(&self->effects[i * 8]);
    }
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    for (int i = 3; i >= 0; i--) {
        re::Ptr32<void>* ref = &At<void*>(p, 0x6D0 + i * 0x28);
        if (*ref) {
            CleanUpOldReference(*ref, ref);
        }
    }
    Vehicle_Destruct(self);
}

// 0x4A8A50: the wheels' surface effects (dust, gravel...) by the surface under each
// and the speed; skid effects and 0x58A680's for spinning / locked wheels; a special
// one indoors for wheels off the ground; nothing for class 10, all off when it isn't
// being driven or parked (status 0..3)
void __fastcall UpdateWheelEffects(CAutomobile* self, re::Edx) {
    int32_t vehicleClass;
    Vehicle_GetVehicleClass(self, &vehicleClass);
    if (vehicleClass == 10) {
        return;
    }
    uint8_t status = self->typeAndStatus >> 3;
    if (status != 3 && status != 0 && status != 1 && status != 2) {
        Automobile_ResetEffects(self);
        return;
    }
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    Vec forward;
    Vec* f = Entity_GetForward(self, &forward);
    uint32_t defaultEffect = g_defaultWheelEffect;
    float along = float(double(self->moveSpeed.y) * f->y + double(self->moveSpeed.x) * f->x + double(self->moveSpeed.z) * f->z);
    along = float(double(along) / g_speedScale);
    float speed = std::fabs(along);
    uint32_t hash = HashName("AutoWheel");
    float strength = float((double(speed) - 5.0) / 15.0);
    if (strength < 0.0f) {
        strength = 0.0f;
    } else if (1.0f < strength) {
        strength = 1.0f;
    }
    uint8_t interior = IsAreaInterior(currArea);
    for (int i = 0; i < 4; i++) {
        uint8_t* wheel = p + 0x6C4 + i * 0x28;
        uint8_t* effect = p + 0x870 + i * 8;
        uint32_t* effectId = &At<uint32_t>(p, 0x890 + i * 4);
        uint8_t offGround = 1;
        if (!(0.0f > At<float>(wheel, 8))) {
            offGround = 0;
        }
        uint32_t id = 0xFFFF;
        if (1.0f > At<float>(p, 0x774 + i * 4) && 5.0f < speed) {
            uint32_t surface = At<uint8_t>(wheel, 0x22);
            id = SurfaceEffects_Find(g_surfaceEffects, &surface, hash);
        }
        int32_t mode = At<int32_t>(p, 0x850 + i * 4);
        bool emit = false;
        if (mode == 1) {
            strength = 1.0f;
            if (id == 0xFFFF) {
                id = defaultEffect;
            }
            Call58A680WithRef(self, 1);
            emit = true;
        } else if (mode == 2) {
            if (i == 1 || i == 3) {
                strength = 1.0f;
                if (id == 0xFFFF && vehicleClass != 6) {
                    id = defaultEffect;
                }
                if (!At<uint8_t>(p, 0x7D4) && 0.0f < At<float>(p, 0x784 + i * 4)) {
                    Vec a, b;
                    Vec* fa = Entity_GetForward(self, &a);
                    int surface = At<uint8_t>(p, 0x794 + i);
                    Vec* fb = Entity_GetForward(self, &b);
                    Fn5280C0(p + i, wheel, fa->x, fb->y, surface, 0);
                }
                Call58A680WithRef(self, 2);
            }
        } else if (mode == 3) {
            strength = 1.0f;
            if (id == 0xFFFF && vehicleClass != 6) {
                id = defaultEffect;
            }
            Call58A680WithRef(self, 2);
            emit = true;
        }
        if (emit && !At<uint8_t>(p, 0x7D4) && 0.0f < At<float>(p, 0x784 + i * 4)) {
            Vec a, b;
            Vec* fa = Entity_GetForward(self, &a);
            int surface = At<uint8_t>(p, 0x794 + i);
            Vec* fb = Entity_GetForward(self, &b);
            strength = 1.0f;
            Fn5280C0(p + i, wheel, fa->x, fb->y, surface, 0);
        }
        if (interior == 1 && offGround == 1) {
            id = g_interiorWheelEffect;
        }
        bool restart = false;
        if (id == 0xFFFF || *effectId != id) {
            if (id != 0xFFFF || *effectId != id) {
                restart = true;
            }
            *effectId = 0xFFFF;
            if (At<void*>(effect, 4)) {
                Effect_SetStrength(effect, 0.0f);
                Effect_SlowKill(effect, 1.0f);
                if (At<void*>(effect, 4)) {
                    Effects_Release(g_effects, effect);
                }
            }
        }
        if (id == 0xFFFF) {
            continue;
        }
        if (restart || !At<void*>(effect, 4)) {
            uint32_t type = id & 0xFFFF;
            Effects_Start(g_effects, &type, effect, 0);
            *effectId = id;
            if (!At<void*>(effect, 4)) {
                continue;
            }
        }
        if (*effectId == id) {
            Effect_SetStrength(effect, strength);
            Effect_SetRate(effect, strength);
            Effect_SetPosition(effect, wheel, 0);
        }
    }
}

// 0x4A8ED0 (vf43): a frame's position in the world (left as it is without the frame)
void __fastcall GetFramePosition(CAutomobile* self, re::Edx, int frame, Vec* out) {
    void* node = self->frames[frame];
    if (!node) {
        return;
    }
    uint8_t* transform = Node_GetWorldTransform(node);
    out->x = At<float>(transform, 0x24);
    out->y = At<float>(transform, 0x28);
    out->z = At<float>(transform, 0x2C);
}

// 0x4A8F00: a door's damage shown on its frame: a damaged one (not on a car whose +0x354
// is special) knocked to status 1 (a boot, door 1, with handling flag 0x400: 3), doors
// past the first swinging (3) set to 2; status 1 hides the frame's parts, 2 their
// damaged look. (The third argument is unused)
void __fastcall ShowDoorDamage(CAutomobile* self, re::Edx, int frame, int door, int) {
    if (!self->frames[frame]) {
        return;
    }
    void* damage = reinterpret_cast<uint8_t*>(self) + 0x580;
    uint8_t status = Damage_GetDoorStatus(damage, door);
    if (!Vehicle_IsUnknown354Normal(self) && status > 1 && door != 0 && door != 1) {
        status = 1;
        Damage_SetDoorStatus(damage, door, 1);
    } else if (door == 1) {
        if (status == 2 && (At<uint32_t>(self->handling, 0xCC) & 0x400)) {
            Damage_SetDoorStatus(damage, 1, 3);
            status = 3;
        }
    }
    if (door > 1 && status == 3) {
        Damage_SetDoorStatus(damage, door, 2);
        return;
    }
    if (status == 1) {
        DamagePartAll(self->rwObject, frame);
        Node_ForEachChild(self->frames[frame], reinterpret_cast<void*>(0x004A84B0), 2);
        return;
    }
    if (status != 3 && status == 2) {
        Node_ForEachChild(self->frames[frame], hideDamagedChild, 0x800);
    }
}

} // namespace c09autoparts

// ---- registration ---------------------------------------------------------------------

#define AUTOPARTS_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTOPARTS_REVERSED("CEntityRef::Construct", 0x004A8480, &c09autoparts::RefConstruct);
AUTOPARTS_REVERSED("Get20C8704_3C", 0x004A84A0, &c09autoparts::Get20C8704_3C);
AUTOPARTS_REVERSED("CAutomobile::SetNodeFlagsIfBits", 0x004A84B0, &c09autoparts::SetNodeFlagsIfBits);
AUTOPARTS_REVERSED("CAutomobile::RandomNear", 0x004A8540, &c09autoparts::RandomNear);
AUTOPARTS_REVERSED("CAutomobile::HitParticles", 0x004A85C0, &c09autoparts::HitParticles);
AUTOPARTS_REVERSED("CAutomobile::ShowPanelDamage", 0x004A8810, &c09autoparts::ShowPanelDamage);
AUTOPARTS_REVERSED("CAutomobile::CountDown368", 0x004A8860, &c09autoparts::CountDown368);
AUTOPARTS_REVERSED("CAutomobile::ResetWheels", 0x004A8880, &c09autoparts::ResetWheels);
AUTOPARTS_REVERSED("CAutomobile::CountDamagedComponents", 0x004A8990, &c09autoparts::CountDamagedComponents);
AUTOPARTS_REVERSED("Set10", 0x004A89A0, &c09autoparts::Set10);
AUTOPARTS_REVERSED("CAutomobile::Destruct", 0x004A89E0, &c09autoparts::Destruct);
AUTOPARTS_REVERSED("CAutomobile::UpdateWheelEffects", 0x004A8A50, &c09autoparts::UpdateWheelEffects);
AUTOPARTS_REVERSED("CAutomobile::GetFramePosition", 0x004A8ED0, &c09autoparts::GetFramePosition);
AUTOPARTS_REVERSED("CAutomobile::ShowDoorDamage", 0x004A8F00, &c09autoparts::ShowDoorDamage);
