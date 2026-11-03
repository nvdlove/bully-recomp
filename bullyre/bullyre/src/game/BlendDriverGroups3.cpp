// The ped constraint set, third part: the reach driver's two-bone solve (a ped's arm
// reaching for another ped's hand or a point on an entity), the joint constraint's
// skeleton (MODELS\MOT_CTRL.MXD) and the set's update. 0x732530..0x733320.
//
// The reach driver: +0x2C its ped, +0x30 its limb (+0 the chain: +0x20 the upper joint,
// +0x24 the lower), +0x34 the target, +0x40 the hand's rotation. The ped: +0x114 its
// model, +0x2F4 / +0x2F8 the peds it holds hands with, +0x153C reaching for a ped,
// +0x1540 which side, +0x148C / +0x16BC..+0x16C8 reaching for a point on an entity,
// +0x16E8 / +0x16EC its left / right limb, +0x16F4 / +0x16F8 their reach drivers.

#include <cstdint>

#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct Bd3Vec {
    float x, y, z;
};
struct Bd3Quat {
    float x, y, z, w;
};
struct Bd3Transform {
    float m[9];
    Bd3Vec t;
    float scale;
};
static_assert(sizeof(Bd3Transform) == 0x34);
struct Bd3Matrix {
    Bd3Vec right;
    uint32_t pad0C;
    Bd3Vec forward;
    uint32_t pad1C;
    Bd3Vec up;
    uint32_t pad2C;
    Bd3Vec pos;
    uint32_t pad3C;
    re::Ptr32<void> attached;
    uint32_t owns;
};
static_assert(sizeof(Bd3Matrix) == 0x48);

GAME_ARRAY(float, bd3Up, 0x009E33DC);
GAME_VAR(int32_t, bd3ConeOther, 0x00BC735C);
GAME_VAR(float, bd3ConeLimit, 0x00BC7358);       // -0.3
GAME_VAR(int32_t, bd3BlendOutOnCone, 0x00BC7354);
GAME_VAR(float, bd3ElbowOut, 0x00BC7350);        // 0.02
GAME_VAR(float, bd3HandReach, 0x00BC734C);       // 0.15
GAME_VAR(float, bd3TooFar, 0x00BC7348);          // -0.4
GAME_VAR(float, bd3BlendOutTime, 0x00BC7344);
GAME_VAR(float, bd3BlendInTime, 0x00BC7340);
GAME_VAR(float, bd3Slack, 0x020C9060);
inline void* const bd3AnimManager = reinterpret_cast<void*>(0x020C4910);
constexpr uint32_t kVtJointConstraint = 0x0094D28C;

GAME_FN(void, __thiscall, Bd3_DriverUpdate, 0x0072E8E0, uint8_t* driver, float dt);
GAME_FN(void, __thiscall, Bd3_GroupProcess, 0x007317E0, uint8_t* group);
GAME_FN(void, __thiscall, Bd3_GroupClear, 0x0072EBC0, uint8_t* group);
GAME_FN(void, __thiscall, Bd3_JointConstraintInit, 0x00733220, uint8_t* self);
GAME_FN(float, __cdecl, Bd3_Distance, 0x0041A950, const Bd3Vec* a, const Bd3Vec* b);
GAME_FN(Bd3Vec*, __thiscall, Bd3_GetRight, 0x004736F0, uint8_t* entity, Bd3Vec* out);
GAME_FN(Bd3Vec*, __thiscall, Bd3_GetForward, 0x004154A0, uint8_t* entity, Bd3Vec* out);
GAME_FN(uint8_t, __thiscall, Bd3_ClampToCone, 0x00414E70, Bd3Vec* self, const Bd3Vec* axis, float cosLimit);
GAME_FN(float, __thiscall, Bd3_Normalise, 0x00414E10, Bd3Vec* v);
GAME_FN(Bd3Vec*, __cdecl, Bd3_Cross, 0x00414730, Bd3Vec* out, const Bd3Vec* a, const Bd3Vec* b);
GAME_FN(Bd3Vec*, __cdecl, Bd3_TransformDirection, 0x0041C490, Bd3Vec* out, const void* matrix, const Bd3Vec* v);
GAME_FN(Bd3Vec*, __cdecl, Bd3_VectorAdd, 0x004129A0, Bd3Vec* out, const Bd3Vec* a, const Bd3Vec* b);
GAME_FN(float, __cdecl, Bd3_SafeAcos, 0x00413D60, float x);
GAME_FN(void, __thiscall, Bd3_QuatFromAxisAngle, 0x00414020, Bd3Quat* q, const Bd3Vec* axis, float angle);
GAME_FN(void, __thiscall, Bd3_QuatToNi, 0x00413E60, const Bd3Quat* q, void* transform);
GAME_FN(void, __thiscall, Bd3_QuatSet, 0x00465620, Bd3Quat* q, const void* transform);
GAME_FN(void, __thiscall, Bd3_MatrixFromNi, 0x00412A70, Bd3Matrix* m, const void* transform);
GAME_FN(void, __thiscall, Bd3_MatrixUpdateRW, 0x00413A70, void* m);
GAME_FN(void, __thiscall, Bd3_MatrixSetUnity, 0x00412770, void* m);
GAME_FN(uint8_t*, __thiscall, Bd3_EntityGetMatrix, 0x0046DD20, uint8_t* entity);
GAME_FN(int, __thiscall, Bd3_FindLoadedHierarchy, 0x006C1D70, void* manager, const char* name, re::Ptr32<void>* out, int flag);
GAME_FN(uint8_t*, __thiscall, Bd3_ModelConstruct, 0x006BA240, uint8_t* model);
GAME_FN(int, __thiscall, Bd3_AddModel, 0x006C30A0, void* manager, uint8_t* model, int a, void* hierarchy, int b, int c);
GAME_FN(void*, __cdecl, Bd3_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void*, __cdecl, Bd3_WorldTransform, 0x006C9A90, void* frame);

const Bd3Vec* EntityPos(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m != nullptr ? reinterpret_cast<Bd3Vec*>(m + 0x30) : reinterpret_cast<Bd3Vec*>(e + 4);
}
Bd3Transform* Joints(uint8_t* model) {
    return At<Bd3Transform*>(At<uint8_t*>(model, 0x14), 8);
}
// an arm's length (upper bone + lower bone) in a model
float ArmLength(uint8_t* model, uint8_t* limb) {
    uint8_t* chain = At<uint8_t*>(limb, 0);
    Bd3Transform* joints = Joints(model);
    Bd3Transform* lower = &joints[At<int32_t>(chain, 0x24)];
    Bd3Transform* upper = &joints[At<int32_t>(chain, 0x20)];
    Bd3Vec a = lower->t;
    Bd3Vec b = upper->t;
    float length = Bd3_Distance(&b, &a);
    Bd3Vec c = lower[1].t;
    Bd3Vec d = lower->t;
    return Bd3_Distance(&d, &c) + length;
}
Bd3Vec Negated(const Bd3Vec& v) {
    return Bd3Vec{-v.x, -v.y, -v.z};
}
Bd3Vec NegatedUp() {
    return Bd3Vec{-bd3Up[0], -bd3Up[1], -bd3Up[2]};
}

} // namespace

namespace bdg3 {

// 0x732530 (cdecl; the joint constraint model's callback): the constraint enabled, its
// group processed
void __cdecl JointConstraint_ModelCallback(void*, uint8_t* constraint) {
    vc::Call<void>(constraint, 0x54, 1);
    uint8_t* group = At<uint8_t*>(constraint, 8);
    if (group != nullptr) {
        Bd3_GroupProcess(group);
    }
}

// 0x732550 (ReachDriver vf3): the reach solved: towards the other ped's shoulder (the
// elbow from the law of cosines about the side axis), or a point on an entity; then
// blended in (or out when out of reach or past the cone) and the other ped's matching
// driver updated
void __fastcall ReachDriver_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* ped = At<uint8_t*>(self, 0x2C);
    uint8_t* selfModel = At<uint8_t*>(ped, 0x114);
    uint8_t* holderA = At<uint8_t*>(ped, 0x2F8);
    uint8_t* holderB = At<uint8_t*>(ped, 0x2F4);
    uint8_t blendOut = vc::Call<uint8_t>(self, 0x40) == 0;
    ped = At<uint8_t*>(self, 0x2C);
    bool haveA = holderA != nullptr;
    if (At<uint32_t>(ped, 0x153C) != 0) {
        uint8_t* other;
        uint8_t* otherModel;
        uint8_t* limb;
        bool right;
        uint8_t otherLeft;
        if (holderA != nullptr) {
            right = At<uint32_t>(ped, 0x1540) != 0;
            otherModel = At<uint8_t*>(holderA, 0x114);
            other = holderA;
            uint8_t* driver;
            if (right) {
                limb = At<uint8_t*>(holderA, 0x16EC);
                driver = At<uint8_t*>(holderA, 0x16F8);
                otherLeft = 0;
            } else {
                limb = At<uint8_t*>(holderA, 0x16E8);
                driver = At<uint8_t*>(holderA, 0x16F4);
                otherLeft = 1;
            }
            if (At<uint8_t>(driver, 0xC) == 0 && vc::Call<uint8_t>(driver, 0x40)) {
                return;
            }
        } else {
            if (holderB == nullptr) {
                return;
            }
            right = At<uint32_t>(ped, 0x1540) == 0;
            otherModel = At<uint8_t*>(holderB, 0x114);
            other = holderB;
            if (right) {
                limb = At<uint8_t*>(holderB, 0x16EC);
                otherLeft = 0;
            } else {
                limb = At<uint8_t*>(holderB, 0x16E8);
                otherLeft = 1;
            }
        }
        uint8_t side = haveA ? right : !right;
        // the arms' lengths, the other's shoulder from this one's
        float otherLength = ArmLength(otherModel, limb);
        float selfLength = ArmLength(selfModel, At<uint8_t*>(self, 0x30));
        uint8_t* otherChain = At<uint8_t*>(limb, 0);
        uint8_t* selfChain = At<uint8_t*>(At<uint8_t*>(self, 0x30), 0);
        const Bd3Transform* otherShoulder = &Joints(otherModel)[At<int32_t>(otherChain, 0x20)];
        const Bd3Transform* selfShoulder = &Joints(selfModel)[At<int32_t>(selfChain, 0x20)];
        Bd3Vec d{otherShoulder->t.x - selfShoulder->t.x, otherShoulder->t.y - selfShoulder->t.y,
                 otherShoulder->t.z - selfShoulder->t.z};
        Bd3Vec sideAxis{0.0f, 0.0f, 0.0f};
        Bd3Vec tmp;
        if (right) {
            sideAxis = *Bd3_GetRight(At<uint8_t*>(self, 0x2C), &tmp);
        } else {
            sideAxis = Negated(*Bd3_GetRight(At<uint8_t*>(self, 0x2C), &tmp));
        }
        uint8_t otherCone = 0;
        if (bd3ConeOther != 0 && other != nullptr) {
            Bd3Vec back = Negated(d);
            Bd3Vec otherAxis{0.0f, 0.0f, 0.0f};
            Bd3Vec t2;
            if (right) {
                otherAxis = Negated(*Bd3_GetRight(other, &t2));
            } else {
                otherAxis = *Bd3_GetRight(other, &t2);
            }
            otherCone = Bd3_ClampToCone(&back, &otherAxis, bd3ConeLimit);
        }
        uint8_t selfCone = Bd3_ClampToCone(&d, &sideAxis, bd3ConeLimit);
        float lengthSq = d.x * d.x + d.y * d.y;
        lengthSq = lengthSq + d.z * d.z;
        lengthSq = lengthSq < 0.0f ? -lengthSq : lengthSq;
        float distance = crt::Sqrtf(lengthSq);
        Bd3_Normalise(&d);
        bool both = otherLeft != 0 && right;
        if ((selfCone || otherCone || both) && bd3BlendOutOnCone != 0) {
            blendOut = 1;
        }
        Bd3Vec down = NegatedUp();
        Bd3Vec cross;
        Bd3_Cross(&cross, &d, &down);
        Bd3Vec axis = cross;
        // (the transform is the game's uninitialised stack but its scale: zero here)
        Bd3Transform t{};
        t.scale = 1.0f;
        Bd3Matrix m;
        m.attached = &t;   // (UpdateRW writes the result back into t)
        m.owns = 0;
        Bd3_MatrixFromNi(&m, &t);
        float elbowOut = bd3ElbowOut;
        Bd3Vec elbow = otherShoulder->t;
        if (side == 0) {
            elbowOut = -elbowOut;
        }
        float reachSelf = selfLength;
        float reachOther = otherLength;
        float sum = selfLength + bd3Slack;
        sum = sum + otherLength;
        sum = sum + bd3Slack;
        if (distance < sum) {
            reachSelf = selfLength - bd3Slack;
            reachOther = otherLength - bd3Slack;
        }
        float upper = bd3HandReach + reachSelf;
        float total = reachOther + upper;
        total = total + bd3HandReach;
        if (distance + bd3TooFar > total) {
            blendOut = 1;
        }
        if (total > distance) {
            float lower = reachOther + bd3HandReach;
            float c = upper * upper + distance * distance;
            c = c - lower * lower;
            c = c / ((upper + upper) * distance);
            float angle = Bd3_SafeAcos(c);
            Bd3Quat q;
            Bd3_QuatFromAxisAngle(&q, &axis, angle);
            Bd3_QuatToNi(&q, &t);
            Bd3_MatrixFromNi(&m, &t);
            Bd3Vec rotated;
            d = *Bd3_TransformDirection(&rotated, &m, &d);
            Bd3Vec arm{d.x * reachSelf, d.y * reachSelf, d.z * reachSelf};
            elbow = selfShoulder->t;
            elbow.x = arm.x + elbow.x;
            elbow.y = arm.y + elbow.y;
            elbow.z = arm.z + elbow.z;
            Bd3Vec out{axis.x * elbowOut, axis.y * elbowOut, axis.z * elbowOut};
            elbow.x = out.x + elbow.x;
            elbow.y = out.y + elbow.y;
            elbow.z = out.z + elbow.z;
        } else {
            blendOut = 1;
        }
        Bd3Vec up;
        if (side != 0) {
            up = Bd3Vec{axis.y * d.z - axis.z * d.y, axis.z * d.x - axis.x * d.z, axis.x * d.y - axis.y * d.x};
            axis = Negated(axis);
        } else {
            up = Bd3Vec{axis.z * d.y - axis.y * d.z, axis.x * d.z - axis.z * d.x, axis.y * d.x - axis.x * d.y};
        }
        // (the fourth dwords: the game's uninitialised stack, zero here)
        m.right = d;
        m.forward = axis;
        m.pad1C = 0;
        m.up = up;
        m.pad2C = 0;
        m.pos = elbow;
        m.pad3C = 0;
        Bd3_MatrixUpdateRW(&m);
        At<Bd3Vec>(self, 0x34) = elbow;
        Bd3_MatrixUpdateRW(&m);
        Bd3_QuatSet(&At<Bd3Quat>(self, 0x40), &t);
    } else if (At<uint32_t>(ped, 0x148C) != 0) {
        Bd3Vec offset{At<float>(ped, 0x16BC), At<float>(ped, 0x16C0), At<float>(ped, 0x16C4)};
        uint8_t* target = At<uint8_t*>(ped, 0x16C8);
        const Bd3Vec* pos = EntityPos(target);
        Bd3Vec dir;
        Bd3_TransformDirection(&dir, Bd3_EntityGetMatrix(target), &offset);
        Bd3Vec point;
        point = *Bd3_VectorAdd(&point, pos, &dir);
        Bd3Transform t{};
        t.scale = 1.0f;
        At<Bd3Vec>(self, 0x34) = point;
        Bd3Matrix m;
        m.attached = &t;   // (UpdateRW writes the result back into t)
        m.owns = 0;
        Bd3_MatrixFromNi(&m, &t);
        Bd3Vec forward;
        Bd3_GetForward(At<uint8_t*>(At<uint8_t*>(self, 0x2C), 0x16C8), &forward);
        forward.z = 0.0f;
        Bd3_Normalise(&forward);
        Bd3Vec down = NegatedUp();
        Bd3Vec c1;
        Bd3_Cross(&c1, &forward, &down);
        Bd3Vec c2;
        Bd3_Cross(&c2, &c1, &forward);
        m.right = Negated(forward);
        m.forward = c1;
        m.pad1C = 0;
        m.up = Negated(c2);
        m.pad2C = 0;
        m.pos = point;
        m.pad3C = 0;
        Bd3_MatrixUpdateRW(&m);
        Bd3_MatrixUpdateRW(&m);
        Bd3_QuatSet(&At<Bd3Quat>(self, 0x40), &t);
    }
    if (blendOut) {
        vc::Call<void>(self, 0x38, bd3BlendOutTime);
        vc::Call<void>(self, 0x48);
    } else {
        vc::Call<void>(self, 0x30, bd3BlendInTime);
        vc::Call<void>(self, 0x44);
    }
    Bd3_DriverUpdate(self, dt);
    vc::Call<void>(self, 0x54, 1);
    if (holderB == nullptr) {
        return;
    }
    if (At<uint8_t*>(self, 0x30) == At<uint8_t*>(At<uint8_t*>(self, 0x2C), 0x16E8)) {
        uint8_t* driver = At<uint8_t*>(holderB, 0x16F8);
        vc::Call<void>(driver, 0xC, dt);
        uint8_t* group = At<uint8_t*>(At<uint8_t*>(holderB, 0x16F8), 8);
        if (group != nullptr) {
            Bd3_GroupProcess(group);
        }
    }
    if (At<uint8_t*>(self, 0x30) == At<uint8_t*>(At<uint8_t*>(self, 0x2C), 0x16EC)) {
        uint8_t* driver = At<uint8_t*>(holderB, 0x16F4);
        vc::Call<void>(driver, 0xC, dt);
        uint8_t* group = At<uint8_t*>(At<uint8_t*>(holderB, 0x16F4), 8);
        if (group != nullptr) {
            Bd3_GroupProcess(group);
        }
    }
}

// 0x7331B0: the constraint set's frame (each group marked and processed; the second
// group cleared while the third is active)
void __fastcall Constraints_Update(uint8_t* self, re::Edx, float) {
    if (At<uint32_t>(self, 0x1F8) == 0) {
        return;
    }
    At<uint8_t>(self, 0xD0 + 0x29) = 1;
    Bd3_GroupProcess(self + 0xD0);
    if (At<uint8_t>(self, 0xFA) != 0) {
        Bd3_GroupClear(self + 0x68);
    } else {
        At<uint8_t>(self, 0x68 + 0x29) = 1;
        Bd3_GroupProcess(self + 0x68);
        At<uint8_t>(self, 0x29) = 1;
        Bd3_GroupProcess(self);
    }
    At<uint8_t>(self, 0x138 + 0x29) = 1;
    Bd3_GroupProcess(self + 0x138);
    At<uint8_t>(self, 0x198 + 0x29) = 1;
    Bd3_GroupProcess(self + 0x198);
}

// 0x733220 (JointConstraint vf2): its skeleton model made (once MOT_CTRL.MXD is loaded),
// its callback set, added to the anim manager; its matrix attached to the root
void __fastcall JointConstraint_Init(uint8_t* self, re::Edx) {
    re::Ptr32<void> hierarchy = nullptr;
    if (Bd3_FindLoadedHierarchy(bd3AnimManager, "MODELS\\MOT_CTRL.MXD", &hierarchy, 0) != 0) {
        return;
    }
    uint8_t* model = static_cast<uint8_t*>(Bd3_Malloc(0x104));
    model = model != nullptr ? Bd3_ModelConstruct(model) : nullptr;
    At<uint8_t*>(self, 0x2C) = model;
    At<uint32_t>(model, 4) = 0x00732530;   // (the game writes through null when out of memory)
    At<uint8_t*>(At<uint8_t*>(self, 0x2C), 8) = self;
    if (Bd3_AddModel(bd3AnimManager, At<uint8_t*>(self, 0x2C), 0, hierarchy, 0, 1) != 0) {
        return;
    }
    At<uint32_t>(self, 0x30) = At<uint32_t>(At<uint8_t*>(self, 0x2C), 0x10);
    At<void*>(self, 0x74) = Bd3_WorldTransform(At<void*>(At<uint8_t*>(self, 0x2C), 0xC));
    Bd3_MatrixUpdateRW(self + 0x34);
}

// 0x7332C0 (thiscall): JointConstraint's constructor
uint8_t* __fastcall JointConstraint_Construct(uint8_t* self, re::Edx) {
    At<float>(self, 0x14) = 0.1f;
    At<float>(self, 0x18) = 0.1f;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<float>(self, 0x1C) = 0.0f;
    At<int32_t>(self, 0x28) = 2;
    At<uint32_t>(self, 0) = kVtJointConstraint;
    At<float>(self, 0x20) = 1.0f;
    At<uint32_t>(self, 0x2C) = 0;
    At<uint32_t>(self, 0x30) = 0;
    At<float>(self, 0x24) = 0.0f;
    At<uint32_t>(self, 0x74) = 0;
    At<uint8_t>(self, 0x7C) = 0;
    At<float>(self, 0x78) = 1.0f;
    Bd3_MatrixSetUnity(self + 0x34);
    Bd3_JointConstraintInit(self);
    return self;
}

} // namespace bdg3

RE_UNHOOKED("TriggerLoaderImpl::vf9", 0x0073255F);   // (not a function: inside 0x732550)

#define BDG3_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

BDG3_REVERSED("JointConstraint::ModelCallback", 0x00732530, &bdg3::JointConstraint_ModelCallback);
BDG3_REVERSED("ReachDriver::Update", 0x00732550, &bdg3::ReachDriver_Update);
BDG3_REVERSED("PedConstraints::Update", 0x007331B0, &bdg3::Constraints_Update);
BDG3_REVERSED("JointConstraint::Init", 0x00733220, &bdg3::JointConstraint_Init);
BDG3_REVERSED("JointConstraint::Construct", 0x007332C0, &bdg3::JointConstraint_Construct);
