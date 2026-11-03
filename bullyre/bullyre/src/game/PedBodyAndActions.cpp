// CPed and helpers (0x472CA0 .. 0x475050): its limb nodes, motion-blur trails, collision
// spheres, falling and ground snapping, which action-tree nodes are playing, and a few
// small constructors and vector helpers used by the ped code.

#include <cmath>
#include <cstring>

#include "game/Ped.h"
#include "reversed.h"

namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;

// A placeable's position: its matrix's if it has one
const ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

GAME_VAR(float, PB_timeStep, 0x00C1A9A4);            // CTimer::ms_fTimeStep
GAME_VAR(CPed*, PB_player, 0x00C1AEA8);              // FindPlayerPed
GAME_VAR(uint8_t*, PB_tuning, 0x020C88B8);           // +0x100, +0x104: fall heights
GAME_VAR(void*, PB_actionMgr, 0x01B649C4);           // vf19: a node for a ped's +0x300
GAME_VAR(uint8_t*, PB_pedColModel, 0x00CF312C);      // +0x38 -> the spheres (0x14 each)
GAME_VAR(float, PB_sphereRadius, 0x00CF311C);
GAME_VAR(double, PB_d_0_008, 0x00905A70);
GAME_VAR(double, PB_d_0_43, 0x0090CE50);
GAME_VAR(double, PB_d_0_45, 0x00900E48);
GAME_VAR(double, PB_d_1, 0x008FF1D8);
GAME_VAR(float, PB_f_3, 0x00900C14);
GAME_VAR(float, PB_f_5, 0x00900188);
GAME_VAR(float, PB_halfPi, 0x00A33960);
GAME_VAR(float, PB_quarterPi, 0x00A33964);
GAME_VAR(float, PB_twoPi, 0x00A33968);
inline void* const PB_animMgr = reinterpret_cast<void*>(0x020C4910);
inline void* const PB_actionContexts = reinterpret_cast<void*>(0x00D02850);

// The action-tree nodes tested here (handles in the game's data)
inline void* Node(uintptr_t address) {
    return reinterpret_cast<void*>(address);
}

// The models whose collision spheres are the ped's position (0xBF42CC..0xBF42DC)
GAME_VAR(int32_t, PB_model42DC, 0x00BF42DC);
GAME_VAR(int32_t, PB_model42D4, 0x00BF42D4);
GAME_VAR(int32_t, PB_model42D0, 0x00BF42D0);
GAME_VAR(int32_t, PB_model42CC, 0x00BF42CC);

// The action/anim ids 0x474120 counts stats for
GAME_VAR(int32_t, PB_id4348, 0x00BF4348);
GAME_VAR(int32_t, PB_id4384, 0x00BF4384);
GAME_VAR(int32_t, PB_id4364, 0x00BF4364);
GAME_VAR(int32_t, PB_id4448, 0x00BF4448);
GAME_VAR(int32_t, PB_id4358, 0x00BF4358);
GAME_VAR(int32_t, PB_model4444, 0x00BF4444);
GAME_VAR(int32_t, PB_id4434, 0x00BF4434);
GAME_VAR(int32_t, PB_id435C, 0x00BF435C);
GAME_VAR(int32_t, PB_id4108, 0x00BF4108);
GAME_VAR(int32_t, PB_id4100, 0x00BF4100);
GAME_VAR(int32_t, PB_id440C, 0x00BF440C);
GAME_VAR(int32_t, PB_id4428, 0x00BF4428);
GAME_VAR(int32_t, PB_id4344, 0x00BF4344);
GAME_VAR(int32_t, PB_id4414, 0x00BF4414);

GAME_FN(void, __thiscall, PB_TrackBaseConstruct, 0x005D7F10, void* self, uintptr_t a);
GAME_FN(uint8_t, __thiscall, PB_IsNodePlaying, 0x005F4800, void* controller, void* node, int a, int b);
GAME_FN(uint8_t, __thiscall, PB_IsPlayingPath, 0x005F3D50, void* controller, const char* path, int a);
GAME_FN(void, __thiscall, PB_PlayNode, 0x005F4AC0, void* controller, void* node);
GAME_FN(ped::CVector*, __thiscall, PB_Fn768A70, 0x00768A70, void* self, ped::CVector* out, uintptr_t a);
GAME_FN(void*, __cdecl, PB_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, PB_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __thiscall, PB_BlurConstruct, 0x00465080, void* self);
GAME_FN(void, __thiscall, PB_BlurAttach, 0x0044C070, void* blur, void* entity, int a, int b);
GAME_FN(void, __thiscall, PB_BlurSet, 0x004650B0, void* blur, float a, float b, int c, float d, float e, int f, int g);
GAME_FN(void, __thiscall, PB_BlurStop, 0x00465140, void* blur);
GAME_FN(int, __thiscall, PB_Held_Count, 0x004A73F0, void* held);
GAME_FN(void*, __thiscall, PB_Held_Get, 0x004A7410, void* held, int index);
GAME_FN(void, __thiscall, PB_Held_4A7490, 0x004A7490, void* held, int a);
GAME_FN(void, __thiscall, PB_AnimGetNode, 0x006C0F80, void* mgr, void* animBlend, uint32_t index, re::Ptr32<uint8_t>* out);
GAME_FN(void, __thiscall, PB_Fn72EDA0, 0x0072EDA0, void* self);
GAME_FN(void, __thiscall, PB_Entity466160, 0x00466160, void* entity, int a);
GAME_FN(void, __thiscall, PB_Weapon4DA3B0, 0x004DA3B0, void* weapon, int a);
GAME_FN(void, __thiscall, PB_Fn4DABF0, 0x004DABF0, void* self, int a);
GAME_FN(void, __thiscall, PB_Fn468740, 0x00468740, void* self);
GAME_FN(void, __thiscall, PB_PhysicalVf3, 0x0046A770, CPed* self);
GAME_FN(void, __thiscall, PB_PhysicalVf36, 0x0046ABE0, CPed* self);
GAME_FN(void, __thiscall, PB_EntityVf19, 0x00467EB0, CPed* self);
GAME_FN(void, __thiscall, PB_ApplyMoveForce_x64, 0x0046A7E0, CPed* self, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void PB_ApplyMoveForce(CPed* a0, ped::CVector a1) {
    return PB_ApplyMoveForce_x64(a0, a1.x, a1.y, a1.z);
}
GAME_FN(void, __thiscall, PB_ApplyTurnForce_x64, 0x0046A950, CPed* self, float, float, float, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void PB_ApplyTurnForce(CPed* a0, ped::CVector a1, ped::CVector a2) {
    return PB_ApplyTurnForce_x64(a0, a1.x, a1.y, a1.z, a2.x, a2.y, a2.z);
}
GAME_FN(void, __thiscall, PB_TrackBaseDestruct, 0x00608760, void* self);
GAME_FN(ped::CVector*, __cdecl, PB_CrossProduct, 0x00414730, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PB_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(uint8_t, __cdecl, PB_ProcessVerticalLine, 0x004629C0, const ped::CVector* pos, float zStop, void* colPoint,
        re::Ptr32<void>* entity, int a, int b);
GAME_FN(float, __cdecl, PB_FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int flags);
GAME_FN(float, __thiscall, PB_GetDistanceFromCentreOfMassToBase, 0x00466110, CPed* self);
GAME_FN(void, __cdecl, PB_IncrementStat, 0x00458480, int stat, int amount);
GAME_FN(uint8_t, __thiscall, PB_IsDog, 0x004473E0, CPed* ped);  // (the three dog models)
GAME_FN(void*, __cdecl, PB_GetAnimNode, 0x00731C30, void* animBlend, int index);
GAME_FN(void, __thiscall, PB_AnimNode72F4F0, 0x0072F4F0, void* node);
GAME_FN(ped::CVector*, __cdecl, PB_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(void, __thiscall, PB_Intel4842C0, 0x004842C0, void* intelligence, CPed* ped);
GAME_FN(void, __thiscall, PB_Intel484350, 0x00484350, void* intelligence, CPed* ped);
GAME_FN(void, __thiscall, PB_Intel4844C0, 0x004844C0, void* intelligence, uintptr_t a);
GAME_FN(uint8_t, __thiscall, PB_ActionIsValid, 0x005D80E0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PB_ActionGetNode, 0x005D80B0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PB_ActionGetNode2, 0x005D80D0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PB_ActionGetInstance, 0x005D9C20, ped::ActionHandle* handle);
GAME_FN(void, __thiscall, PB_ActionRelease, 0x005D9C50, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PB_ContextsFind, 0x005DB8F0, void* contexts, void* node);
GAME_FN(void*, __thiscall, PB_ContextsGetCurrent, 0x005DB960, void* contexts);
GAME_FN(void, __thiscall, PB_ContextsSetCurrent, 0x005DBDA0, void* contexts, void* context);
GAME_FN(void, __cdecl, PB_Instance5D8270, 0x005D8270, void* instance);
GAME_FN(void, __cdecl, PB_Instance5D8670, 0x005D8670, void* instance, void* node);
GAME_FN(void, __cdecl, PB_Instance5D85C0, 0x005D85C0, void* instance, int value);
GAME_FN(void, __cdecl, PB_Instance5D82A0, 0x005D82A0, void* instance, int a, int b);
GAME_FN(void, __cdecl, PB_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __thiscall, PB_Fn48CEB0, 0x0048CEB0, void* self);
GAME_FN(void, __thiscall, PB_MatrixConstruct, 0x00770920, void* self);
GAME_FN(void, __cdecl, PB_Fn415E30, 0x00415E30, void* out, void* matrix, int a);

// The ped's action controllers (the intelligence's +0x258 and +0x290)
void* Actions258(CPed* ped) {
    return ped->intelligence + 0x258;
}
void* Actions290(CPed* ped) {
    return ped->intelligence + 0x290;
}

// ---- small helpers -------------------------------------------------------------------

// 0x472CA0 (thiscall, TrackOrCondition::sub_472CA0): a handle constructed from a value
// (0x5D7F10 sets it; e.g. +0x16C8, the car a ped skitches on)
void* __fastcall Track_Construct(void* self, re::Edx, int a) {
    PB_TrackBaseConstruct(self, a);
    return self;
}

// 0x472CC0 (thiscall): a bit set in +0x68 (a pending +0x2C moved to +0x30 first,
// clearing the others)
void __fastcall MoveTarget_SetBit(uint8_t* self, re::Edx, int bit) {
    float& pending = At<float>(self, 0x2C);
    if (pending != 0.0f) {
        At<float>(self, 0x30) = pending;
        self[0x68] = 0;
        pending = 0.0f;
    }
    self[0x68] |= static_cast<uint8_t>(1 << (bit & 0x1F));
}

// 0x472D70 (thiscall): a target position and time (-1: keeps the time)
void __fastcall MoveTarget_Set(uint8_t* self, re::Edx, const ped::CVector* pos, float time) {
    At<ped::CVector>(self, 4) = *pos;
    if (time > 0.0f) {
        At<float>(self, 0x2C) = time;
        At<float>(self, 0x28) = time;
    } else if (time == -1.0f) {
        At<uint32_t>(self, 0x38) |= 2;
        At<int32_t>(self, 0x20) = 0;
        At<int32_t>(self, 0x24) = 0;
        return;
    } else {
        At<float>(self, 0x28) = 0.0f;
    }
    At<uint32_t>(self, 0x38) |= 2;
    At<int32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x24) = 0;
}

// 0x472DD0 (thiscall): +0x18C8 from a value clamped to 0 .. 5
void __fastcall CPed_Set18C8Clamped(CPed* self, re::Edx, int value) {
    float v = static_cast<float>(value);
    if (v < 0.0f) {
        v = 0.0f;
    } else if (PB_f_5 < v) {
        v = PB_f_5;
    }
    self->unknown18C8 = static_cast<int32_t>(v);
}

// 0x472E20 (thiscall): (0, 0, 0, 1)
void __fastcall Quaternion_SetIdentity(float* self, re::Edx) {
    self[0] = 0.0f;
    self[1] = 0.0f;
    self[2] = 0.0f;
    self[3] = 1.0f;
}

// 0x472E60 (cdecl)
uint8_t __cdecl CPed_IsPlaying1B65270(CPed* ped) {
    return PB_IsNodePlaying(Actions258(ped), Node(0x01B65270), 1, 1);
}

// 0x472E80 (cdecl): 0x768A70 of `object` into `out` (Ghidra lost the third argument)
ped::CVector* __cdecl Vector_From768A70(ped::CVector* out, int a, void* object) {
    ped::CVector v;
    ped::CVector* result = PB_Fn768A70(object, &v, a);
    *out = *result;
    return out;
}

// ---- motion-blur trails (+0x1D94 body, +0x1D98 weapon, +0x1D9C the two held items) ----

void* NewBlur() {
    void* memory = PB_Malloc(0x34);
    return memory ? PB_BlurConstruct(memory) : nullptr;
}

// 0x472EB0 (thiscall)
void __fastcall CPed_InitMotionBlur(CPed* self, re::Edx, int a) {
    re::Ptr32<void>& body = At<void*>(self, 0x1D94);
    if (!body) {
        body = NewBlur();
    }
    PB_BlurAttach(body, self, a, 4);
    re::Ptr32<void>& weapon = At<void*>(self, 0x1D98);
    if (!weapon) {
        weapon = NewBlur();
    }
    PB_BlurAttach(weapon, self->weapon, a, 0);
    int count = PB_Held_Count(At<void*>(self, 0x1D8));
    for (int i = 0; i < count; i++) {
        re::Ptr32<void>& held = At<void*>(self, 0x1D9C + i * 4);
        if (!held) {
            held = NewBlur();
        }
        PB_BlurAttach(held, PB_Held_Get(At<void*>(self, 0x1D8), i), a, 0);
    }
}

// 0x473890 (thiscall)
void __fastcall CPed_SetMotionBlur(CPed* self, re::Edx, float a, float b, int c, float d, float e, int f, int g) {
    PB_BlurSet(At<void*>(self, 0x1D94), a, b, c, d, e, f, g);
    PB_BlurSet(At<void*>(self, 0x1D98), a, b, c, d, e, 0, 0);
    for (int i = 0; i < 2; i++) {
        void* held = At<void*>(self, 0x1D9C + i * 4);
        if (held && PB_Held_Get(At<void*>(self, 0x1D8), i)) {
            PB_BlurSet(held, a, b, c, d, e, 0, 0);
        }
    }
}

// 0x473970 (thiscall): 1 only flags +0x130E
void __fastcall CPed_StopMotionBlur(CPed* self, re::Edx, int flag) {
    if (static_cast<int8_t>(flag) == 1) {
        At<uint8_t>(self, 0x130E) = 1;
        return;
    }
    PB_BlurStop(At<void*>(self, 0x1D94));
    PB_BlurStop(At<void*>(self, 0x1D98));
    for (int i = 0; i < 2; i++) {
        void* held = At<void*>(self, 0x1D9C + i * 4);
        if (held && PB_Held_Get(At<void*>(self, 0x1D8), i)) {
            PB_BlurStop(held);
        }
    }
}

// ---- body --------------------------------------------------------------------------

// 0x472FB0 (thiscall): the leg and arm nodes found by name (the arms flagged, and given
// to +0x16F4 / +0x16F8)
void __fastcall CPed_FindLimbNodes(CPed* self, re::Edx) {
    if (!self->animBlend) {
        return;
    }
    uint8_t* a = At<uint8_t*>(self->animBlend, 0x94);
    if (!a) {
        return;
    }
    uint8_t* clump = At<uint8_t*>(a, 0x10);
    if (!clump) {
        return;
    }
    uint32_t count = At<uint32_t>(clump, 0x78);
    for (uint32_t i = 0; i < count; i++) {
        re::Ptr32<uint8_t> node = nullptr;
        PB_AnimGetNode(PB_animMgr, self->animBlend, i, &node);
        if (!node) {
            continue;
        }
        const char* name = At<const char*>(node, 0);
        if (std::strcmp(name, "RightLeg") == 0) {
            At<uint8_t*>(self, 0x16E0) = node;
        } else if (std::strcmp(name, "LeftLeg") == 0) {
            At<uint8_t*>(self, 0x16E4) = node;
        } else if (std::strcmp(name, "Right") == 0) {
            At<uint8_t*>(self, 0x16E8) = node;
            At<uint32_t>(node, 0x38) |= 4;
            At<uint8_t*>(At<uint8_t*>(self, 0x16F4), 0x30) = node;
        } else if (std::strcmp(name, "LeftArm") == 0) {
            At<uint8_t*>(self, 0x16EC) = node;
            At<uint32_t>(node, 0x38) |= 4;
            At<uint8_t*>(At<uint8_t*>(self, 0x16F8), 0x30) = node;
        }
    }
    PB_Fn72EDA0(self->unknown2EC);
}

// 0x4732D0 (thiscall): passed on to the weapon (when it has +0x144) and the held items
void __fastcall CPed_Fn4732D0(CPed* self, re::Edx, int a) {
    void* weapon = self->weapon;
    if (weapon && At<int32_t>(weapon, 0x144) != 0) {
        PB_Entity466160(weapon, a);
        PB_Weapon4DA3B0(self->weapon, a);
    }
    if (At<void*>(self, 0x1D4)) {
        PB_Fn4DABF0(At<void*>(self, 0x1D4), a);
    }
    if (At<void*>(self, 0x1D8)) {
        PB_Held_4A7490(At<void*>(self, 0x1D8), a);
    }
}

// vf3 0x473330
void __fastcall CPed_Vf3(CPed* self, re::Edx) {
    PB_Fn468740(At<void*>(self, 0x2E4));
    PB_PhysicalVf3(self);
}

// vf36 0x473350: with both 0x1E24 flags, the move speed's z taken down by +0x1D58 a
// step (the old one kept in +0x1704), or CPhysical's with +0x1474
void __fastcall CPed_Vf36(CPed* self, re::Edx) {
    if ((At<uint8_t>(self, 0x1BC) & 2) && (self->flags1E24 & 1) && (self->flags1E24 & 2)) {
        if (At<int32_t>(self, 0x1474) != 0) {
            PB_PhysicalVf36(self);
            return;
        }
        float drop = At<float>(self, 0x1D58) * PB_timeStep;
        At<float>(self, 0x1704) = At<float>(self, 0x12C);
        At<float>(self, 0x12C) = At<float>(self, 0x12C) - drop;
    }
}

// 0x4733B0 (thiscall): +0x1708 the velocity: the move speed, or with +0x1334 the anim's
// root motion (+0x1720, turned by the desired heading) a step; 0 when time stands still
void __fastcall CPed_UpdateVelocity(CPed* self, re::Edx) {
    float& vx = At<float>(self, 0x1708);
    float& vy = At<float>(self, 0x170C);
    float& vz = At<float>(self, 0x1710);
    float z;
    if (At<int32_t>(self, 0x1334) == 0) {
        vx = At<float>(self, 0x124);
        vy = At<float>(self, 0x128);
        z = At<float>(self, 0x12C);
    } else {
        float c = static_cast<float>(std::cos(static_cast<double>(self->desiredHeading)));
        float s = static_cast<float>(std::sin(static_cast<double>(self->desiredHeading)));
        float rx = At<float>(self, 0x1720);
        float ry = At<float>(self, 0x1724);
        vx = c * rx - s * ry;
        vy = c * ry + s * rx;
        vz = At<float>(self, 0x1728);
        float inv = 1.0f / PB_timeStep;
        vx = vx * inv;
        vy = vy * inv;
        z = inv * vz;
    }
    vz = z;
    if (!(0.0f < PB_timeStep)) {
        vx = 0.0f;
        vy = 0.0f;
        vz = 0.0f;
    }
}

// 0x4734E0 (thiscall): a push at a point (by the mass, a step at most 3); with
// `onlyWhenBelow`, only when `fraction` is above 1
void __fastcall CPed_ApplyPush(CPed* self, re::Edx, float strength, const ped::CVector* dir,
                               const ped::CVector* point, float fraction, float scale, int onlyWhenBelow) {
    float rest = 1.0f - fraction;
    if (re::Flag(onlyWhenBelow) && rest >= 0.0f) {
        return;
    }
    float step = PB_timeStep < PB_f_3 ? PB_timeStep : PB_f_3;
    float k = static_cast<float>(-((static_cast<double>(At<float>(self, 0x158)) * PB_d_0_008 * 2.0) * strength * rest *
                                   step * scale));
    ped::CVector force = {k * dir->x, k * dir->y, k * dir->z};
    PB_ApplyMoveForce(self, force);
    PB_ApplyTurnForce(self, force, *point);
}

// 0x4735E0 (LocomotionSlideTrack vf1): the deleting destructor
void* __fastcall LocomotionSlideTrack_Destroy(void* self, re::Edx, uint8_t flags) {
    PB_TrackBaseDestruct(self);
    if (flags & 1) {
        PB_Free(self);
    }
    return self;
}

// 0x473600 (thiscall): a simple transform's right (from its heading at +0xC)
ped::CVector* __fastcall SimpleTransform_GetRight(uint8_t* self, re::Edx, ped::CVector* out) {
    ped::CVector up = {0.0f, 0.0f, 1.0f};
    float heading = At<float>(self, 0xC);
    ped::CVector forward;
    forward.x = -static_cast<float>(std::sin(static_cast<double>(heading)));
    forward.y = static_cast<float>(std::cos(static_cast<double>(heading)));
    forward.z = 0.0f;
    PB_CrossProduct(out, &forward, &up);
    return out;
}

// 0x473670 (cdecl): the z of `to - from` projected on (x, y, z) (the x and y parts
// multiplied by 0 and added)
float __cdecl ProjectedZ(float x, float y, float z, ped::CVector from, ped::CVector to) {
    ped::CVector d;
    PB_VectorSub(&d, &to, &from);
    float dot = x * d.x + y * d.y + z * d.z;
    float px = x * dot;
    float py = y * dot;
    float pz = z * dot;
    return 0.0f * py + 0.0f * px + pz;
}

// 0x473730 (thiscall): the matrix's up, (0, 0, 1) without one
ped::CVector* __fastcall Placeable_GetUp(uint8_t* self, re::Edx, ped::CVector* out) {
    uint8_t* matrix = At<uint8_t*>(self, 0x14);
    if (matrix) {
        *out = At<ped::CVector>(matrix, 0x20);
        return out;
    }
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 1.0f;
    return out;
}

// 0x473760 (cdecl): a vertical line through a bone (its +0x24 position), from `up`
// above it to `down` below; hit something
uint8_t __cdecl ProcessVerticalLineAtBone(uint8_t* object, int bone, void* colPoint, float up, float down) {
    re::Ptr32<void> entity = nullptr;
    uint8_t* bones = At<uint8_t*>(At<uint8_t*>(object, 0x14), 8);
    const ped::CVector& b = At<ped::CVector>(bones, bone * 0x34 + 0x24);
    ped::CVector pos = b;
    float zStop = b.z - down;
    pos.z = b.z + up;
    return PB_ProcessVerticalLine(&pos, zStop, colPoint, &entity, 1, 0);
}

// 0x4737D0 (cdecl)
float __cdecl Fn4737D0(float x) {
    return static_cast<float>((1.0f - x) * PB_d_0_43);
}

// 0x4737F0 (thiscall): which quarter a direction is in, relative to the desired heading
// (0 ahead, then counter-clockwise)
int __fastcall CPed_GetQuadrant(CPed* self, re::Edx, const ped::CVector* dir) {
    float angle = static_cast<float>(std::atan2(static_cast<double>(-dir->x), static_cast<double>(dir->y)));
    angle = angle - self->desiredHeading;
    angle = angle + PB_quarterPi;
    while (angle < 0.0f) {
        angle = PB_twoPi + angle;
    }
    int quadrant = static_cast<int>(angle / PB_halfPi);
    if (quadrant > 3) {
        quadrant = quadrant + (-4 - static_cast<int>(((static_cast<uint32_t>(quadrant) - 4) >> 2) * 4));
    }
    return quadrant;
}

// 0x473BC0 (thiscall): falling from high enough above the ground plays the node for it
void __fastcall CPed_CheckFall(CPed* self, re::Edx) {
    if (!(0.0f > At<float>(self, 0x12C)) || self->unknown1C98 != 0 || At<int32_t>(self, 0x1328) != 0) {
        return;
    }
    ped::CVector pos = *PosOf(self);
    float drop = At<float>(self, 0x18B0) - pos.z;
    if (!(At<float>(PB_tuning, 0x104) < drop)) {
        return;
    }
    uint8_t found;  // (set by the callee)
    float ground = PB_FindGroundZ(pos.x, pos.y, pos.z, &found, 0xB);
    if (!found) {
        return;
    }
    float height = pos.z - ground;
    if (!(At<float>(PB_tuning, 0x100) < height)) {
        return;
    }
    void* node = reinterpret_cast<void*(__thiscall*)(void*, void*)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(PB_actionMgr))[0x13]))(
        PB_actionMgr, self->intelligence + 0x300);
    if (node) {
        PB_PlayNode(Actions258(self), node);
    }
}

// 0x473CE0 (thiscall)
uint8_t __fastcall CPed_IsInActionA(CPed* self, re::Edx) {
    if (At<int32_t>(self, 0x1340) != 0 || At<int32_t>(self, 0x146C) != 0) {
        return 1;
    }
    void* actions = Actions258(self);
    if (PB_IsNodePlaying(actions, Node(0x01B652DC), 1, 1) || PB_IsNodePlaying(actions, Node(0x01B652E0), 1, 1) ||
        PB_IsNodePlaying(actions, Node(0x01B652E4), 1, 1)) {
        return 1;
    }
    return PB_IsNodePlaying(actions, Node(0x01B6528C), 1, 1) != 0;
}

// 0x473D60 (thiscall)
uint8_t __fastcall CPed_IsInActionB(CPed* self, re::Edx) {
    void* actions = Actions258(self);
    if (PB_IsNodePlaying(actions, Node(0x01B65330), 1, 1) || PB_IsNodePlaying(actions, Node(0x01B65334), 1, 1) ||
        PB_IsNodePlaying(actions, Node(0x01B65338), 1, 1)) {
        return 1;
    }
    return PB_IsNodePlaying(actions, Node(0x01B65340), 1, 1) != 0;
}

// 0x473DD0 (thiscall): moving fast enough, in neither
uint8_t __fastcall CPed_IsFreeToRun(CPed* self, re::Edx) {
    if (At<float>(self->intelligence, 0x1C) < PB_d_0_45) {
        return 0;
    }
    if (CPed_IsInActionB(self, nullptr)) {
        return 0;
    }
    return CPed_IsInActionA(self, nullptr) == 0;
}

// 0x473E40 (thiscall)
uint8_t __fastcall CPed_IsPlaying1B654FC(CPed* self, re::Edx) {
    return PB_IsNodePlaying(Actions258(self), Node(0x01B654FC), 1, 1);
}

// 0x473E60 (thiscall): put on the ground below it (from a metre above)
void __fastcall CPed_PlaceOnGround(CPed* self, re::Edx) {
    ped::CVector pos = *PosOf(self);
    float z = static_cast<float>(pos.z + PB_d_1);
    uint8_t found = 0;
    float ground = PB_FindGroundZ(pos.x, pos.y, z, &found, 1);
    if (found) {
        float height = PB_GetDistanceFromCentreOfMassToBase(self) + ground;
        ped::CVector at = {pos.x, pos.y, height};
        reinterpret_cast<void(__thiscall*)(CPed*, float, float, float)>(static_cast<void*>(self->vtable[0x38 / 4]))(
            self, at.x, at.y, at.z);   // (Teleport: the vector as its floats)
    }
}

// 0x473FB0 (thiscall)
uint8_t __fastcall CPed_IsPlaying1B6528C(CPed* self, re::Edx) {
    return PB_IsNodePlaying(Actions290(self), Node(0x01B6528C), 1, 1);
}

// 0x473FD0 (thiscall): the node, but not while interacting with a prop or picking up
uint8_t __fastcall CPed_IsPlaying1B654DC(CPed* self, re::Edx) {
    if (PB_IsNodePlaying(Actions258(self), Node(0x01B654DC), 1, 1) &&
        !PB_IsPlayingPath(Actions290(self), reinterpret_cast<const char*>(0x0090CE74), 1) &&  // /GrapplesMoves/PropInteract
        !PB_IsPlayingPath(Actions290(self), reinterpret_cast<const char*>(0x0090CE5C), 1)) {  // /GrapplesMoves/Pickup
        return 1;
    }
    return 0;
}

// 0x474040 (thiscall)
int __fastcall CPed_IsPlaying1B65534Or8(CPed* self, re::Edx) {
    if (PB_IsNodePlaying(Actions258(self), Node(0x01B65534), 1, 1) ||
        PB_IsNodePlaying(Actions258(self), Node(0x01B65538), 1, 1)) {
        return 1;
    }
    return 0;
}

// 0x4740A0 / 0x4740B0 / 0x474100: the intelligence's own, forwarded
void __fastcall CPed_Intelligence4842C0(CPed* self, re::Edx) {
    PB_Intel4842C0(self->intelligence, self);
}
void __fastcall CPed_Intelligence484350(CPed* self, re::Edx) {
    PB_Intel484350(self->intelligence, self);
}
void __fastcall CPed_Intelligence4844C0(CPed* self, re::Edx, int a) {
    PB_Intel4844C0(self->intelligence, a);
}

// 0x474120 (thiscall; the ped only passed on to IsDog): the stats counted for an action
// or anim ending (`started` 0) or starting
void __fastcall CPed_CountActionStat(CPed* self, re::Edx, int id, int started) {
    bool flag = re::Flag(started);
    if (id == PB_id4348) {
        if (!flag) PB_IncrementStat(0x47, 1);
    } else if (id == PB_id4384) {
        if (!flag) PB_IncrementStat(0x49, 1);
    } else if (id == PB_id4364) {
        if (flag) PB_IncrementStat(0x53, 1);
    } else if (id == PB_id4448) {
        if (flag) PB_IncrementStat(0x6F, 1);
    } else if (id == PB_id4358) {
        // (the player's weapon slot isn't checked for null)
        if (PB_player && At<int16_t>(PB_player->weapon, 0x10E) == PB_model4444 && !flag) {
            PB_IncrementStat(0x72, 1);
        }
    } else if (id == PB_id4434) {
        if (!flag) PB_IncrementStat(0x75, 1);
    } else if (id == PB_id435C) {
        if (flag) PB_IncrementStat(0x78, 1);
    } else if (id == PB_id4108) {
        if (!flag) PB_IncrementStat(0x7A, 1);
    } else if (id == PB_id4100) {
        if (!flag) PB_IncrementStat(0x7C, 1);
    } else if (id == PB_id440C) {
        if (!flag) PB_IncrementStat(0x7E, 1);
    } else if (id == PB_id4428 || id == PB_id4344) {
        if (!flag) PB_IncrementStat(0x80, 1);
    } else if (id == PB_id4414 && !flag) {
        PB_IncrementStat(PB_IsDog(self) ? 0xAA : 0xA9, 1);
    }
}

// vf19 0x474300: its 7 anim nodes, then CEntity's
void __fastcall CPed_Vf19(CPed* self, re::Edx) {
    for (int i = 0; i < 7; i++) {
        void* node = PB_GetAnimNode(self->animBlend, i);
        if (node) {
            PB_AnimNode72F4F0(node);
        }
    }
    PB_EntityVf19(self);
}

// ---- collision spheres -----------------------------------------------------------

bool IsSphereless(CPed* self) {
    int model = self->modelIndex;
    return model == PB_model42DC || model == PB_model42D4 || model == PB_model42D0 || model == PB_model42CC;
}

const ped::CVector& BonePos(CPed* self, int bone) {
    uint8_t* bones = At<uint8_t*>(At<uint8_t*>(self->animBlend, 0x14), 8);
    return At<ped::CVector>(bones, bone * 0x34 + 0x24);
}

const ped::CVector* SphereOffset(int index) {
    uint8_t* spheres = *At<re::Ptr32<uint8_t>*>(PB_pedColModel, 0x38);
    return reinterpret_cast<ped::CVector*>(spheres + index * 0x14);
}

// 0x474630 (thiscall): a collision sphere (0 .. 5): at its bone (+0x1D64) with its
// radius (+0x1D7C), or (+0xA4) the col model's offsets from its position; the position
// for some models
uint8_t __fastcall CPed_GetColSphere(CPed* self, re::Edx, int index, ped::CVector* pos, float* radius) {
    if (IsSphereless(self) || index >= 6) {
        *pos = *PosOf(self);
        *radius = PB_sphereRadius;
        return 1;
    }
    if (self->unknownA4 == 0) {
        *pos = BonePos(self, At<int32_t>(self, 0x1D64 + index * 4));
        *radius = At<float>(self, 0x1D7C + index * 4);
        return 1;
    }
    int sphere;
    if (index == 1) {
        sphere = 1;
    } else if (index == 2) {
        sphere = 2;
    } else if (index == 4) {
        sphere = 0;
    } else {
        return 0;
    }
    ped::CVector v;
    ped::CVector* p = PB_VectorAdd(&v, PosOf(self), SphereOffset(sphere));
    *pos = *p;
    *radius = PB_sphereRadius;
    return 1;
}

// 0x474770 (thiscall): a sphere's centre (0 .. 2); the position raised by +0x1D7C for
// some models and past 2
ped::CVector* __fastcall CPed_GetSphereCentre(CPed* self, re::Edx, ped::CVector* out, int index) {
    if (IsSphereless(self) || index >= 3) {
        const ped::CVector* p = PosOf(self);
        float y = p->y;
        float x = p->x;
        float z = At<float>(self, 0x1D7C) + p->z;
        out->x = x;
        out->y = y;
        out->z = z;
        return out;
    }
    if (self->unknownA4 != 0) {
        PB_VectorAdd(out, PosOf(self), SphereOffset(index));
        return out;
    }
    *out = BonePos(self, At<int32_t>(self, 0x1D64 + index * 4));
    return out;
}

// vf27 0x4748A0: how far the top of its spheres is above it
float __fastcall CPed_Vf27(CPed* self, re::Edx) {
    float z = PosOf(self)->z;
    float best = 0.0f;
    for (int i = 0; i < 3; i++) {
        ped::CVector centre;
        CPed_GetSphereCentre(self, nullptr, &centre, i);
        float top = (centre.z - z) + At<float>(self, 0x1D7C + i * 4);
        if (best < top) {
            best = top;
        }
    }
    return best;
}

// 0x474D20 (thiscall): the action at +0x18E8 run once in its context (not while
// +0x1C98, unless `force`)
void __fastcall CPed_RunAction18E8(CPed* self, re::Edx, int force) {
    if (self->unknown1C98 != 0 && !re::Flag(force)) {
        return;
    }
    if (At<int32_t>(self, 0x141C) == 0) {
        return;
    }
    ped::ActionHandle* handle = &At<ped::ActionHandle>(self, 0x18E8);
    if (!PB_ActionIsValid(handle)) {
        return;
    }
    void* context = PB_ContextsFind(PB_actionContexts, PB_ActionGetNode(handle));
    if (!context) {
        PB_ActionRelease(handle);
        return;
    }
    void* saved = PB_ContextsGetCurrent(PB_actionContexts);
    PB_ContextsSetCurrent(PB_actionContexts, context);
    void* instance = PB_ActionGetInstance(handle);
    PB_Instance5D8270(instance);
    PB_Instance5D8670(instance, PB_ActionGetNode2(handle));
    int value = reinterpret_cast<int(__thiscall*)(CPed*)>(static_cast<void*>(self->vtable[0xAC / 4]))(self);
    PB_Instance5D85C0(instance, value);
    PB_Instance5D82A0(instance, 1, 0);
    PB_ContextsSetCurrent(PB_actionContexts, saved);
    PB_ActionRelease(handle);
}

// ---- the ped's member structs ------------------------------------------------------

// 0x474E00 (thiscall): the references at +0x68 .. +0x5C, +8 and +4 dropped
void __fastcall PedRefs474E00_Destruct(uint8_t* self, re::Edx) {
    for (int i = 3; i >= 0; i--) {
        re::Ptr32<void>* ref = &At<void*>(self, 0x5C + i * 4);
        if (*ref) {
            PB_CleanUpOldReference(*ref, ref);
        }
    }
    if (At<void*>(self, 8)) {
        PB_CleanUpOldReference(At<void*>(self, 8), &At<void*>(self, 8));
    }
    if (At<void*>(self, 4)) {
        PB_CleanUpOldReference(At<void*>(self, 4), &At<void*>(self, 4));
    }
}

// 0x474E60 (thiscall): the 16 references at +0x40 .. +4 dropped
void __fastcall PedRefs474E60_Destruct(uint8_t* self, re::Edx) {
    for (int i = 15; i >= 0; i--) {
        re::Ptr32<void>* ref = &At<void*>(self, 4 + i * 4);
        if (*ref) {
            PB_CleanUpOldReference(*ref, ref);
        }
    }
}

// 0x474E90 (thiscall, TrackOrCondition::sub_474E90): the 10 references (12 bytes apart)
// dropped
void __fastcall PedRefs474E90_Destruct(uint8_t* self, re::Edx) {
    for (int i = 9; i >= 0; i--) {
        re::Ptr32<void>* ref = &At<void*>(self, i * 0xC);
        if (*ref) {
            PB_CleanUpOldReference(*ref, ref);
        }
    }
}

// 0x474EC0 (cdecl): an objective if it's of type 0x17
Objective* __cdecl AsObjective17(Objective* objective) {
    if (!objective) {
        return nullptr;
    }
    return objective->type == 0x17 ? objective : nullptr;
}

// 0x474F00 (thiscall)
void* __fastcall PedMember474F00_Construct(uint8_t* self, re::Edx, int owner) {
    for (int i = 0; i < 16; i++) {
        uint8_t* e = self + 0x10 + i * 0xC;
        At<int32_t>(e, 0) = 0;
        e[4] = 0;
        At<int32_t>(e, 8) = 0;
    }
    At<float>(self, 0xD4) = 0.0f;
    At<float>(self, 0xD8) = 0.0f;
    At<int32_t>(self, 0) = owner;
    At<float>(self, 0xDC) = 0.0f;
    PB_Fn48CEB0(self);
    return self;
}

// 0x474F50 (thiscall)
void* __fastcall PedMember474F50_Construct(uint8_t* self, re::Edx, int owner) {
    static const uint16_t zeroed[] = {0x24,  0x28,  0x2C,  0x30,  0x34,  0x38,  0x3C,  0x40,  0x44,  0x48,  0x4C,
                                      0x50,  0x54,  0x58,  0x70,  0x74,  0x11C, 0x120, 0x124, 0x128, 0x138, 0x170,
                                      0x174, 0x178, 0x17C, 0x180, 0x184, 0x188, 0x18C, 0x190, 0x194, 0x198, 0x19C,
                                      0x1A0, 0x1A4, 0x1A8, 0x1AC, 0x1B0, 0x1B4, 0x1B8, 0x1BC, 0x1C0, 0x1C4, 0x1C8,
                                      0x1CC, 0x1D0, 0x1D4, 0x210};
    for (uint16_t offset : zeroed) {
        At<int32_t>(self, offset) = 0;
    }
    At<int32_t>(self, 0) = owner;
    return self;
}

// 0x475050 (cdecl): 0x415E30 with a matrix made from a vector (its x, y, z down the
// first column)
void* __cdecl Fn475050(void* out, const ped::CVector* v, int a) {
    alignas(4) uint8_t matrix[0x34];
    At<float>(matrix, 0x30) = 1.0f;
    PB_MatrixConstruct(matrix);
    At<float>(matrix, 0x00) = v->x;
    At<float>(matrix, 0x10) = v->y;
    At<float>(matrix, 0x20) = v->z;
    PB_Fn415E30(out, matrix, a);
    return out;
}

#define PEDBODY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDBODY_REVERSED("Handle::Construct472CA0", 0x00472CA0, &Track_Construct);
PEDBODY_REVERSED("MoveTarget::SetBit", 0x00472CC0, &MoveTarget_SetBit);
PEDBODY_REVERSED("MoveTarget::Set", 0x00472D70, &MoveTarget_Set);
PEDBODY_REVERSED("CPed::Set18C8Clamped", 0x00472DD0, &CPed_Set18C8Clamped);
PEDBODY_REVERSED("CQuaternion::SetIdentity", 0x00472E20, &Quaternion_SetIdentity);
PEDBODY_REVERSED("CPed::IsPlaying1B65270", 0x00472E60, &CPed_IsPlaying1B65270);
PEDBODY_REVERSED("Vector_From768A70", 0x00472E80, &Vector_From768A70);
PEDBODY_REVERSED("CPed::InitMotionBlur", 0x00472EB0, &CPed_InitMotionBlur);
PEDBODY_REVERSED("CPed::FindLimbNodes", 0x00472FB0, &CPed_FindLimbNodes);
PEDBODY_REVERSED("CPed::Fn4732D0", 0x004732D0, &CPed_Fn4732D0);
PEDBODY_REVERSED("CPed::vf3", 0x00473330, &CPed_Vf3);
PEDBODY_REVERSED("CPed::vf36", 0x00473350, &CPed_Vf36);
PEDBODY_REVERSED("CPed::UpdateVelocity", 0x004733B0, &CPed_UpdateVelocity);
PEDBODY_REVERSED("CPed::ApplyPush", 0x004734E0, &CPed_ApplyPush);
PEDBODY_REVERSED("LocomotionSlideTrack::Destroy", 0x004735E0, &LocomotionSlideTrack_Destroy);
PEDBODY_REVERSED("CSimpleTransform::GetRight", 0x00473600, &SimpleTransform_GetRight);
PEDBODY_REVERSED("ProjectedZ", 0x00473670, &ProjectedZ);
PEDBODY_REVERSED("CPlaceable::GetUp", 0x00473730, &Placeable_GetUp);
PEDBODY_REVERSED("ProcessVerticalLineAtBone", 0x00473760, &ProcessVerticalLineAtBone);
PEDBODY_REVERSED("Fn4737D0", 0x004737D0, &Fn4737D0);
PEDBODY_REVERSED("CPed::GetQuadrant", 0x004737F0, &CPed_GetQuadrant);
PEDBODY_REVERSED("CPed::SetMotionBlur", 0x00473890, &CPed_SetMotionBlur);
PEDBODY_REVERSED("CPed::StopMotionBlur", 0x00473970, &CPed_StopMotionBlur);
PEDBODY_REVERSED("CPed::CheckFall", 0x00473BC0, &CPed_CheckFall);
PEDBODY_REVERSED("CPed::IsInActionA", 0x00473CE0, &CPed_IsInActionA);
PEDBODY_REVERSED("CPed::IsInActionB", 0x00473D60, &CPed_IsInActionB);
PEDBODY_REVERSED("CPed::IsFreeToRun", 0x00473DD0, &CPed_IsFreeToRun);
PEDBODY_REVERSED("CPed::IsPlaying1B654FC", 0x00473E40, &CPed_IsPlaying1B654FC);
PEDBODY_REVERSED("CPed::PlaceOnGround", 0x00473E60, &CPed_PlaceOnGround);
PEDBODY_REVERSED("CPed::IsPlaying1B6528C", 0x00473FB0, &CPed_IsPlaying1B6528C);
PEDBODY_REVERSED("CPed::IsPlaying1B654DC", 0x00473FD0, &CPed_IsPlaying1B654DC);
PEDBODY_REVERSED("CPed::IsPlaying1B65534Or8", 0x00474040, &CPed_IsPlaying1B65534Or8);
PEDBODY_REVERSED("CPed::Intelligence4842C0", 0x004740A0, &CPed_Intelligence4842C0);
PEDBODY_REVERSED("CPed::Intelligence484350", 0x004740B0, &CPed_Intelligence484350);
PEDBODY_REVERSED("CPed::Intelligence4844C0", 0x00474100, &CPed_Intelligence4844C0);
PEDBODY_REVERSED("CPed::CountActionStat", 0x00474120, &CPed_CountActionStat);
PEDBODY_REVERSED("CPed::vf19", 0x00474300, &CPed_Vf19);
PEDBODY_REVERSED("CPed::GetColSphere", 0x00474630, &CPed_GetColSphere);
PEDBODY_REVERSED("CPed::GetSphereCentre", 0x00474770, &CPed_GetSphereCentre);
PEDBODY_REVERSED("CPed::vf27", 0x004748A0, &CPed_Vf27);
PEDBODY_REVERSED("CPed::RunAction18E8", 0x00474D20, &CPed_RunAction18E8);
PEDBODY_REVERSED("PedRefs474E00::Destruct", 0x00474E00, &PedRefs474E00_Destruct);
PEDBODY_REVERSED("PedRefs474E60::Destruct", 0x00474E60, &PedRefs474E60_Destruct);
PEDBODY_REVERSED("PedRefs474E90::Destruct", 0x00474E90, &PedRefs474E90_Destruct);
PEDBODY_REVERSED("AsObjective17", 0x00474EC0, &AsObjective17);
PEDBODY_REVERSED("PedMember474F00::Construct", 0x00474F00, &PedMember474F00_Construct);
PEDBODY_REVERSED("PedMember474F50::Construct", 0x00474F50, &PedMember474F50_Construct);
PEDBODY_REVERSED("Fn475050", 0x00475050, &Fn475050);

} // namespace
