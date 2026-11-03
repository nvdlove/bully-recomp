// The action tree's conditions (continued): where the ped's target is (range and the
// angle off the ped's facing, its collision spheres, a box in the ped's space), how the
// two face each other, and the target's health / play time / node.
// 0x5FF030..0x5FF8B0.
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cmath>
#include <cstdint>

#include "game/ActionTree.h"

#include "game/Crt.h"
using namespace acttree;

namespace {

GAME_VAR(float, zeroF_8FF1F0, 0x008FF1F0);   // 0.0
GAME_VAR(float, twoPi_B0AABC, 0x00B0AABC);   // 2 pi

GAME_FN(uint32_t, __cdecl, CompareFloatsRaw, 0x005FB8B0, float a, float b, int op);
GAME_FN(uint8_t*, __thiscall, Ped_FindByType_477C00, 0x00477C00, uint8_t* ped, int32_t type, int32_t* extra);
GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget_4D7780d, 0x004D7780, uint8_t* targeting);
GAME_FN(uint8_t*, __thiscall, Ped_GetFightTarget_497100d, 0x00497100, uint8_t* ped);
GAME_FN(uint32_t, __cdecl, BoneTable_Get_520380, 0x00520380, int index);
GAME_FN(uint8_t, __cdecl, GetNodeTransform_467D80, 0x00467D80, uint8_t* entity, uint32_t node, float* transform, int flag);
GAME_FN(float*, __cdecl, VectorSubD, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(float*, __thiscall, Entity_GetForward_4154A0d, 0x004154A0, uint8_t* entity, float* out);
GAME_FN(float*, __thiscall, Placement_GetRight_473600, 0x00473600, uint8_t* placement, float* out);
GAME_FN(float, __thiscall, Vector_Normalise_414E10, 0x00414E10, float* v);
GAME_FN(float, __cdecl, SafeAcos_413D60, 0x00413D60, float x);
GAME_FN(float, __cdecl, AngleDifference_432980, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, LimitAngle0To2Pi_432FB0, 0x00432FB0, float angle);
GAME_FN(float, __cdecl, LimitAngle_432EE0, 0x00432EE0, float angle);
GAME_FN(uint8_t, __thiscall, Ped_GetBodySphere_474630, 0x00474630, uint8_t* ped, int index, float* centre, float* radius);
GAME_FN(void, __thiscall, Ped_SetGrapplePartnerD, 0x004744B0, uint8_t* ped, uint8_t* partner);
GAME_FN(void, __thiscall, Box_Set_564830, 0x00564830, float* box, const float* first, const float* second);
GAME_FN(float*, __thiscall, Placeable_GetMatrix_46DD20, 0x0046DD20, uint8_t* placeable);
GAME_FN(float*, __cdecl, InverseTransformPoint_414D20, 0x00414D20, float* out, const float* matrix, const float* point);
GAME_FN(uint32_t, __thiscall, ColBox_IsPointInside_565C20, 0x00565C20, const float* box, const float* point);
GAME_FN(uint32_t, __thiscall, Ctl_IsPlaying_5F4800, 0x005F4800, uint8_t* controller, const uint32_t* name, uint32_t ancestors,
        int nested);

uint8_t* Ped(uint8_t* owner) {
    return vc::At<uint8_t*>(owner, 8);
}

// sqrt (the C runtime's)
float CrtSqrtD(float value) {
    return crt::Sqrtf(value);
}

const float* EntityPos(uint8_t* entity) {
    uint8_t* matrix = vc::At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

bool IsPed(uint8_t* entity) {
    return (entity[0x108] & 7) == 3;
}

// |delta| through the x87 ((y^2 + x^2) + z^2) and the CRT's sqrt
float Length3D(const float* d) {
    float lengthSq = std::fabs(d[1] * d[1] + d[0] * d[0] + d[2] * d[2]);
    return CrtSqrtD(lengthSq);
}

// the ped's right vector (its matrix's first row, else from its heading)
void PedRight(uint8_t* ped, float* out) {
    uint8_t* matrix = vc::At<uint8_t*>(ped, 0x14);
    if (matrix) {
        out[0] = vc::At<float>(matrix, 0);
        out[1] = vc::At<float>(matrix, 4);
        out[2] = vc::At<float>(matrix, 8);
    } else {
        Placement_GetRight_473600(ped + 4, out);
    }
}

// the angle (0..2pi, clockwise past pi) of the unit direction off the ped's forward,
// against the wanted angle +8 within +0xC
bool FacingAngleWithin(uint8_t* self, const float* forward, const float* right, const float* dir) {
    float cosine = forward[1] * dir[1] + forward[0] * dir[0] + forward[2] * dir[2];
    float angle = SafeAcos_413D60(cosine);
    float side = right[1] * dir[1] + right[0] * dir[0] + right[2] * dir[2];
    if (side < zeroF_8FF1F0) {
        angle = float(double(twoPi_B0AABC) - angle);
    }
    float diff = AngleDifference_432980(angle, vc::At<float>(self, 8));
    diff = LimitAngle0To2Pi_432FB0(std::fabs(diff));
    return vc::At<float>(self, 0xC) >= diff;
}

// 0x5FF030 (thiscall): ConditionTargetRelativePosition: the target of kind +0x14 (else
// the looked-at one or +0x544; 0xE: the ped's +0x1D24 point; 4: a vehicle's first node)
// within range +4 and at the angle +8 (within +0xC) off the ped's facing; +0x10: the
// two grappled (when neither is already)
uint8_t __fastcall TargetRelativePosition_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    int32_t kind = vc::At<int32_t>(self, 0x14);
    uint8_t* target = Ped_FindByType_477C00(ped, kind, nullptr);
    bool isPoint = vc::At<int32_t>(self, 0x14) == 0xE;
    if (!target && !isPoint) {
        target = Targeting_GetLookTarget_4D7780d(vc::At<uint8_t*>(ped, 0x1C8));
        if (!target) {
            target = vc::At<uint8_t*>(ped, 0x544);
            if (!target) {
                return 0;
            }
        }
    }
    kind = vc::At<int32_t>(self, 0x14);
    if ((kind == 0 || kind == 1 || kind == 2) && !IsPed(target)) {
        return 0;
    }
    const float* p = EntityPos(ped);
    float pedPos[3] = {p[0], p[1], p[2]};
    float targetPos[3] = {0.0f, 0.0f, 0.0f};
    if (isPoint) {
        uint8_t* point = vc::At<uint8_t*>(ped, 0x1D24);
        if (!point) {
            return 0;
        }
        const float* t = EntityPos(point);
        targetPos[0] = t[0];
        targetPos[1] = t[1];
        targetPos[2] = t[2];
    } else {
        bool placed = false;
        if (kind == 4 && (target[0x108] & 7) == 6) {
            float transform[13];
            transform[12] = 1.0f;
            uint32_t node = BoneTable_Get_520380(0);
            if (GetNodeTransform_467D80(target, node, transform, 1)) {
                targetPos[0] = transform[9];
                targetPos[1] = transform[10];
                targetPos[2] = transform[11];
                placed = true;
            }
        }
        if (!placed) {
            const float* t = EntityPos(target);
            targetPos[0] = t[0];
            targetPos[1] = t[1];
            targetPos[2] = t[2];
        }
    }
    float dir[3];
    VectorSubD(dir, targetPos, pedPos);
    float distance = Length3D(dir);
    if (vc::At<float>(self, 4) < distance) {
        return 0;
    }
    float forward[3], right[3];
    Entity_GetForward_4154A0d(ped, forward);
    PedRight(ped, right);
    Vector_Normalise_414E10(dir);
    bool within = FacingAngleWithin(self, forward, right, dir);
    if (!self[0x10]) {
        return within ? 1 : 0;
    }
    if (!within || !IsPed(target)) {
        return 0;
    }
    if (vc::At<uint8_t*>(target, 0x155C) == nullptr && vc::At<uint8_t*>(ped, 0x155C) == nullptr) {
        Ped_SetGrapplePartnerD(target, ped);
        Ped_SetGrapplePartnerD(ped, target);
        return 1;
    }
    return vc::At<uint8_t*>(ped, 0x155C) == target ? 1 : 0;
}

// 0x5FF330 (thiscall): ConditionTargetRelativePedCollisionSpheres: one of the target's
// six body spheres within +4 of the ped, and the target at the angle +8 (within +0xC)
uint8_t __fastcall TargetRelativePedCollisionSpheres_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* target = Ped_FindByType_477C00(ped, 0, nullptr);
    if (!target) {
        target = Targeting_GetLookTarget_4D7780d(vc::At<uint8_t*>(ped, 0x1C8));
        if (!target) {
            target = vc::At<uint8_t*>(ped, 0x544);
            if (!target) {
                return 0;
            }
        }
    }
    if (!IsPed(target)) {
        return 0;
    }
    const float* p = EntityPos(ped);
    float pedPos[3] = {p[0], p[1], p[2]};
    float centre[3] = {0.0f, 0.0f, 0.0f};
    float radius;
    bool touching = false;
    for (int i = 0; i < 6; i++) {
        if (!Ped_GetBodySphere_474630(target, i, centre, &radius)) {
            continue;
        }
        float d[3];
        VectorSubD(d, centre, pedPos);
        float distance = Length3D(d);
        if (vc::At<float>(self, 4) + radius > distance) {
            touching = true;
            break;
        }
    }
    if (!touching) {
        return 0;
    }
    float forward[3], right[3], dir[3];
    Entity_GetForward_4154A0d(ped, forward);
    PedRight(ped, right);
    VectorSubD(dir, EntityPos(target), pedPos);
    Vector_Normalise_414E10(dir);
    return FacingAngleWithin(self, forward, right, dir) ? 1 : 0;
}

// 0x5FF580 (thiscall): ConditionTargetRelativeOrientation: the angle between the ped's
// and the other's facings (+4: the grapple partner, else +0x434 or the looked-at one)
// near +8: twice its wrapped difference within +0xC
uint32_t __fastcall TargetRelativeOrientation_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* other;
    if (self[4]) {
        other = vc::At<uint8_t*>(ped, 0x155C);
    } else {
        other = vc::At<uint8_t*>(ped, 0x434);
        if (!other) {
            other = Targeting_GetLookTarget_4D7780d(vc::At<uint8_t*>(ped, 0x1C8));
        }
    }
    if (!other) {
        return 0;
    }
    float mine[3], theirs[3];
    Entity_GetForward_4154A0d(ped, mine);
    Entity_GetForward_4154A0d(other, theirs);
    float cosine = theirs[1] * mine[1] + mine[0] * theirs[0] + theirs[2] * mine[2];
    float diff = float(double(SafeAcos_413D60(cosine)) - vc::At<float>(self, 8));
    diff = LimitAngle_432EE0(std::fabs(diff));
    return vc::At<float>(self, 0xC) >= diff + diff ? 1u : 0u;
}

// 0x5FF670 (thiscall): ConditionTargetRelativePositionInsideBox: the target of kind +0x1C
// inside the box (+4/+0xC/+0x14 .. +8/+0x10/+0x18) in the ped's space
uint32_t __fastcall TargetRelativePositionInsideBox_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* target = Ped_FindByType_477C00(ped, vc::At<int32_t>(self, 0x1C), nullptr);
    if (!target) {
        return 0;
    }
    float first[3] = {vc::At<float>(self, 4), vc::At<float>(self, 0xC), vc::At<float>(self, 0x14)};
    float second[3] = {vc::At<float>(self, 8), vc::At<float>(self, 0x10), vc::At<float>(self, 0x18)};
    float box[8];
    Box_Set_564830(box, first, second);
    float* matrix = Placeable_GetMatrix_46DD20(ped);
    const float* t = EntityPos(target);
    float targetPos[3] = {t[0], t[1], t[2]};
    float local[3];
    InverseTransformPoint_414D20(local, matrix, targetPos);
    return ColBox_IsPointInside_565C20(box, local);
}

// the target a Target* condition reads: the fight target (a ped), else the looked-at
// one if a ped; null if none
uint8_t* TargetPed(uint8_t* ped) {
    uint8_t* target = Ped_GetFightTarget_497100d(ped);
    if (target && !IsPed(target)) {
        target = nullptr;
    }
    if (!target) {
        target = Targeting_GetLookTarget_4D7780d(vc::At<uint8_t*>(ped, 0x1C8));
        if (!target || !IsPed(target)) {
            return nullptr;
        }
    }
    return target;
}

// 0x5FF730 (thiscall): ConditionTargetHealth: the grapple partner's (else the looked-at
// ped's) health compared (0 without one)
uint32_t __fastcall TargetHealth_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    float health = 0.0f;
    uint8_t* partner = vc::At<uint8_t*>(ped, 0x155C);
    if (partner) {
        health = vc::At<float>(partner, 0x1CB8);
    } else {
        uint8_t* looked = Targeting_GetLookTarget_4D7780d(vc::At<uint8_t*>(ped, 0x1C8));
        if (looked && IsPed(looked)) {
            health = vc::At<float>(looked, 0x1CB8);
        }
    }
    return CompareFloatsRaw(health, vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FF7A0 (thiscall): ConditionTargetPlayTime: the target's action play time compared
uint32_t __fastcall TargetPlayTime_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float time = 0.0f;
    uint8_t* target = TargetPed(Ped(owner));
    if (target) {
        time = vc::At<float>(vc::At<uint8_t*>(target, 0x2E0), 0x260);
    }
    return CompareFloatsRaw(time, vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FF830 (thiscall): ConditionTargetPlaying: the target's controller plays the node
// named at +4 (+8: or an ancestor), nested ones included
uint32_t __fastcall TargetPlaying_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* target = TargetPed(Ped(owner));
    if (!target) {
        return 0;
    }
    uint8_t* controller = vc::At<uint8_t*>(target, 0x2E0) + 0x258;
    return Ctl_IsPlaying_5F4800(controller, reinterpret_cast<uint32_t*>(self + 4), self[8], 1);
}

} // namespace

#define CONDD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CONDD_REVERSED("ConditionTargetRelativePosition::Evaluate", 0x005FF030, &TargetRelativePosition_Evaluate);
CONDD_REVERSED("ConditionTargetRelativePedCollisionSpheres::Evaluate", 0x005FF330,
               &TargetRelativePedCollisionSpheres_Evaluate);
CONDD_REVERSED("ConditionTargetRelativeOrientation::Evaluate", 0x005FF580, &TargetRelativeOrientation_Evaluate);
CONDD_REVERSED("ConditionTargetRelativePositionInsideBox::Evaluate", 0x005FF670,
               &TargetRelativePositionInsideBox_Evaluate);
CONDD_REVERSED("ConditionTargetHealth::Evaluate", 0x005FF730, &TargetHealth_Evaluate);
CONDD_REVERSED("ConditionTargetPlayTime::Evaluate", 0x005FF7A0, &TargetPlayTime_Evaluate);
CONDD_REVERSED("ConditionTargetPlaying::Evaluate", 0x005FF830, &TargetPlaying_Evaluate);
