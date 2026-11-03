// The ped constraint set's groups, second part: the floor motion driver (the ped's root
// tilted to its ground contacts), the drivers' matrices, the joint group's gather of
// its drivers' targets and its application to the skeleton root, the set's constructor.
// 0x730840..0x731BF0. (BlendDriverGroups.cpp has the layouts.)

#include <cstdint>

#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct Bd2Vec {
    float x, y, z;
};
struct Bd2Quat {
    float x, y, z, w;
};
// NiTransform
struct Bd2Transform {
    float m[9];       // +0x0
    Bd2Vec t;         // +0x24
    float scale;      // +0x30
};
static_assert(sizeof(Bd2Transform) == 0x34);
// CMatrix (right, forward, up, pos, the attached transform)
struct Bd2Matrix {
    Bd2Vec right;     // +0x0
    uint32_t pad0C;
    Bd2Vec forward;   // +0x10
    uint32_t pad1C;
    Bd2Vec up;        // +0x20
    uint32_t pad2C;
    Bd2Vec pos;       // +0x30
    uint32_t pad3C;
    re::Ptr32<void> attached;   // +0x40
    uint32_t owns;    // +0x44
};
static_assert(sizeof(Bd2Matrix) == 0x48);

GAME_VAR(float, bd2FrameRate, 0x00BC725C);   // 0.02
GAME_VAR(float, bd2TimeStep, 0x00C1A9A4);    // CTimer::ms_fTimeStep
GAME_VAR(uint32_t, bd2TimeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, bd2FirstDriverOnly, 0x020C9048);
GAME_VAR(int32_t, bd2KeepRootZ, 0x020C904C);
GAME_VAR(int32_t, bd2HipsFromOwner, 0x020C9050);
GAME_VAR(int32_t, bd2HipsDrop, 0x020C9054);
GAME_VAR(int32_t, bd2HipsBlend, 0x020C9058);
GAME_VAR(int32_t, bd2MaxApplies, 0x00BC7320);
GAME_VAR(int32_t, bd2ApplyRoot, 0x00BC7324);
GAME_VAR(float, bd2HipsDropScale, 0x00BC7328);   // 0.5
GAME_VAR(float, bd2HipsDropMin, 0x00BC732C);     // -0.1
GAME_VAR(float, bd2HipsLimit, 0x00BC7330);       // 1
GAME_VAR(float, bd2HipsBlendRate, 0x00BC7334);   // 1
GAME_ARRAY(float, bd2Up, 0x009E33DC);
GAME_ARRAY(uint32_t, bd2Zero, 0x00BCC564);
constexpr uint32_t kVtJointGroup = 0x0094D318;
constexpr uint32_t kVtIkGroup = 0x0094D324;

GAME_FN(void, __thiscall, Bd2_DriverUpdate, 0x0072E8E0, uint8_t* driver, float dt);
GAME_FN(void, __thiscall, Bd2_GroupUpdate, 0x0072F990, uint8_t* group, float dt);
GAME_FN(float, __thiscall, Bd2_GroupBlend, 0x0072FA80, uint8_t* group, int kind, Bd2Vec* pos, Bd2Quat* rot);
GAME_FN(void, __thiscall, Bd2_Gather, 0x00730F80, uint8_t* group);
GAME_FN(Bd2Vec*, __thiscall, Bd2_TargetVelocity, 0x0072EC50, uint8_t* group, Bd2Vec* out, float dt);
GAME_FN(void, __thiscall, Bd2_ApplyRoot, 0x00731200, uint8_t* group, Bd2Transform* out);
GAME_FN(uint8_t*, __cdecl, Bd2_MatrixFromQuatPos, 0x00730730, uint8_t* out, Bd2Quat q, const Bd2Vec* pos);
GAME_FN(int16_t, __cdecl, Bd2_JointConstraintTypeId, 0x0072E7A0);
GAME_FN(uint8_t*, __thiscall, Bd2_EntityGetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(void, __thiscall, Bd2_MatrixFromNi, 0x00412A70, Bd2Matrix* m, const void* transform);
GAME_FN(void, __thiscall, Bd2_MatrixToNi, 0x00412AC0, const Bd2Matrix* m, void* transform);
GAME_FN(void, __thiscall, Bd2_MatrixUpdateRW, 0x00413A70, Bd2Matrix* m);
GAME_FN(void, __thiscall, Bd2_MatrixSetUp_x64, 0x00413600, Bd2Matrix* m, float x, float y, float z, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Bd2_MatrixSetUp(Bd2Matrix* a0, float a1, float a2, float a3, uint32_t a4) {
    return Bd2_MatrixSetUp_x64(a0, a1, a2, a3, re::FloatOf(uint32_t(a4)));
}
GAME_FN(void, __thiscall, Bd2_MatrixSetUpKeepRight_x64, 0x004136C0, Bd2Matrix* m, float x, float y, float z, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Bd2_MatrixSetUpKeepRight(Bd2Matrix* a0, float a1, float a2, float a3, uint32_t a4) {
    return Bd2_MatrixSetUpKeepRight_x64(a0, a1, a2, a3, re::FloatOf(uint32_t(a4)));
}
GAME_FN(void, __thiscall, Bd2_QuatSet, 0x00465620, Bd2Quat* q, const void* transform);
GAME_FN(void, __thiscall, Bd2_QuatToNi, 0x00413E60, const Bd2Quat* q, void* transform);
GAME_FN(void, __thiscall, Bd2_QuatSlerp, 0x004143C0, Bd2Quat* self, const Bd2Quat* a, const Bd2Quat* b, float t);
GAME_FN(Bd2Vec*, __thiscall, Bd2_VectorScale, 0x00413A20, Bd2Vec* self, Bd2Vec* out, float s);
GAME_FN(float, __thiscall, Bd2_Normalise, 0x004294A0, Bd2Vec* v);
GAME_FN(Bd2Vec*, __cdecl, Bd2_VectorAdd, 0x004129A0, Bd2Vec* out, const Bd2Vec* a, const Bd2Vec* b);
GAME_FN(Bd2Vec*, __thiscall, Bd2_GetForward, 0x004154A0, uint8_t* placeable, Bd2Vec* out);
GAME_FN(Bd2Transform*, __cdecl, Bd2_WorldTransform, 0x006C9A90, void* frame);
GAME_FN(void, __thiscall, Bd2_EntityUpdateRW, 0x00465AB0, uint8_t* entity);
GAME_FN(void, __thiscall, Bd2_EntityUpdateFromRW, 0x00443AA0, uint8_t* entity);
GAME_FN(float, __cdecl, Bd2_AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, Bd2_LimitAngle, 0x00432EE0, float angle);
GAME_FN(void, __thiscall, Bd2_PedSetHeading, 0x004734C0, uint8_t* ped, float heading);
GAME_FN(void, __thiscall, Bd2_PedUpdateMoveDirection, 0x00474570, uint8_t* ped);
GAME_FN(void, __thiscall, Bd2_FeetUpdate, 0x0047A4C0, void* feet);
GAME_FN(uint8_t*, __thiscall, Bd2_SlotsGetIfNotAnim, 0x006B1F50, uint8_t* model, int slot);
GAME_FN(uint8_t*, __thiscall, Bd2_AgrGetEntry, 0x00450B80, void* group, uint32_t index);
GAME_FN(void, __thiscall, Bd2_ModelTranslate_x64, 0x006BA090, uint8_t* model, float, float, float, int a, float b);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Bd2_ModelTranslate(uint8_t* a0, Bd2Vec a1, int a2, float a3) {
    return Bd2_ModelTranslate_x64(a0, re::FloatAt(a1, 0), re::FloatAt(a1, 1), re::FloatAt(a1, 2), a2, a3);
}
GAME_FN(void, __thiscall, Bd2_ModelReRoot, 0x006B9DA0, uint8_t* model, int joint, Bd2Transform* t, int a, float b);
GAME_FN(void, __thiscall, Bd2_ModelUpdateIfChanged, 0x006B95E0, uint8_t* model, float dt);

const Bd2Vec* EntityPos(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m != nullptr ? reinterpret_cast<Bd2Vec*>(m + 0x30) : reinterpret_cast<Bd2Vec*>(e + 4);
}
// a model's joint transforms
Bd2Transform* Joints(uint8_t* model) {
    return At<Bd2Transform*>(At<uint8_t*>(model, 0x14), 8);
}
float Abs(float v) {
    return v < 0.0f ? -v : v;
}
// a matrix's rotation round-tripped through a CMatrix (its attached transform updated),
// then as a quaternion
void QuatFromEntityMatrix(Bd2Quat* q, uint8_t* entity) {
    void* transform = At<void*>(Bd2_EntityGetMatrix(entity), 0x40);
    Bd2Matrix m;
    m.attached = transform;
    m.owns = 0;
    Bd2_MatrixFromNi(&m, transform);
    Bd2_MatrixUpdateRW(&m);
    Bd2_QuatSet(q, transform);
}

} // namespace

namespace bdg2 {

// 0x730840 (FloorMotionDriver vf3): the ped's root rotation tilted to the ground's
// normal (the contacts' normals weighted by their z) by the driver's weight
void __fastcall FloorMotionDriver_Update(uint8_t* self, re::Edx, float dt) {
    Bd2_DriverUpdate(self, dt);
    uint8_t* ped = At<uint8_t*>(self, 0x2C);
    if (0.0f == At<float>(self, 0x24)) {
        QuatFromEntityMatrix(&At<Bd2Quat>(self, 0x30), ped);
        return;
    }
    uint8_t* clump = At<uint8_t*>(ped, 0x18);
    const Bd2Transform* root = nullptr;
    if (clump != nullptr && At<void*>(clump, 4) != nullptr) {
        root = Bd2_WorldTransform(At<void*>(clump, 4));
    }
    Bd2Transform t = *root;   // (the game copies from null without one)
    Bd2Matrix m;
    Bd2_MatrixFromNi(&m, &t);
    Bd2Vec normal{At<float>(ped, 0x175C), At<float>(ped, 0x1760), At<float>(ped, 0x1764)};
    if (At<uint32_t>(ped, 0x148C) != 0 && At<uint32_t>(ped, 0x1758) == At<uint32_t>(ped, 0x16C8)) {
        normal = Bd2Vec{bd2Up[0], bd2Up[1], bd2Up[2]};
    }
    bool contacts = At<uint32_t>(ped, 0x1474) != 0 || At<uint32_t>(ped, 0x1470) != 0;
    if (contacts && At<uint32_t>(ped, 0x1758) != 0) {
        float total = Abs(At<float>(ped, 0x1764));
        Bd2Vec scaled;
        Bd2_VectorScale(&normal, &scaled, total);
        static const int kContacts[4] = {0x1780, 0x17A8, 0x17D0, 0x17F8};
        for (int at : kContacts) {
            if (At<uint32_t>(ped, at) == 0) {
                continue;
            }
            float wx = Abs(At<float>(ped, at + 0xC)) * At<float>(ped, at + 4);
            float wy = Abs(At<float>(ped, at + 0xC)) * At<float>(ped, at + 8);
            float w = Abs(At<float>(ped, at + 0xC));
            float wz = w * At<float>(ped, at + 0xC);
            normal.x = wx + normal.x;
            normal.y = wy + normal.y;
            normal.z = wz + normal.z;
            total = w + total;
        }
        if (total >= 0.0001 || total != total) {
            float inv = 1.0f / total;
            normal.x = inv * normal.x;
            normal.y = normal.y * inv;
            normal.z = inv * normal.z;
            Bd2_Normalise(&normal);
        } else {
            normal = Bd2Vec{bd2Up[0], bd2Up[1], bd2Up[2]};
        }
    }
    if (contacts && At<uint32_t>(ped, 0x1478) == 0) {
        Bd2Matrix driverMatrix;
        vc::Call<void>(self, 0x10, &driverMatrix);
        normal.x = driverMatrix.up.x + normal.x;
        normal.y = driverMatrix.up.y + normal.y;
        normal.z = driverMatrix.up.z + normal.z;
        Bd2Vec scaled;
        Bd2_VectorScale(&normal, &scaled, 0.5f);
        Bd2_Normalise(&normal);
    }
    // (the fourth dword is the game's uninitialised stack: unused)
    Bd2_MatrixSetUpKeepRight(&m, normal.x, normal.y, normal.z, 0);
    Bd2_MatrixToNi(&m, &t);
    ped = At<uint8_t*>(self, 0x2C);
    if ((At<uint32_t>(ped, 0x1474) != 0 || At<uint32_t>(ped, 0x1470) != 0) && At<uint32_t>(ped, 0x1478) == 0) {
        Bd2Quat tilted;
        Bd2_QuatSet(&tilted, &t);
        Bd2Quat current;
        QuatFromEntityMatrix(&current, At<uint8_t*>(self, 0x2C));
        if (!(normal.z < 0.8)) {
            Bd2_QuatSlerp(&At<Bd2Quat>(self, 0x30), &current, &tilted, float(At<float>(self, 0x24) * 0.9));
            return;
        }
        uint8_t* p = At<uint8_t*>(self, 0x2C);
        float speedSq = At<float>(p, 0x124) * At<float>(p, 0x124) + At<float>(p, 0x128) * At<float>(p, 0x128);
        Bd2Vec forwardBuf;
        const Bd2Vec* forward = Bd2_GetForward(p, &forwardBuf);
        float dot = forward->y * normal.y + forward->x * normal.x;
        dot = dot + normal.z * forward->z;
        float lean = float(normal.z * 1.25) + float(speedSq * 5.0);
        lean = Abs(dot) + lean;
        float k;
        if (lean < 0.0f) {
            k = 0.0f;
        } else if (1.0f < lean) {
            k = 1.0f;
        } else {
            k = lean;
        }
        Bd2_QuatSlerp(&At<Bd2Quat>(self, 0x30), &current, &tilted, float(k * 0.9) * At<float>(self, 0x24));
        return;
    }
    Bd2_QuatSet(&At<Bd2Quat>(self, 0x30), &t);
}

// 0x730EE0 (FloorMotionDriver vf4): its matrix (its rotation at the ped's position)
uint8_t* __fastcall FloorMotionDriver_GetMatrix(uint8_t* self, re::Edx, uint8_t* out) {
    const Bd2Vec* pos = EntityPos(At<uint8_t*>(self, 0x2C));
    Bd2_MatrixFromQuatPos(out, At<Bd2Quat>(self, 0x30), pos);
    return out;
}

// 0x730F30 (ReachDriver vf4): its matrix (its rotation at its target)
uint8_t* __fastcall ReachDriver_GetMatrix(uint8_t* self, re::Edx, uint8_t* out) {
    Bd2_MatrixFromQuatPos(out, At<Bd2Quat>(self, 0x40), &At<Bd2Vec>(self, 0x34));
    return out;
}

// 0x730F80: the joint group's target gathered: the first driver's (0x20C9048), else
// from its joint's transform, each kind from the highest down blended in by weight
void __fastcall JointGroup_Gather(uint8_t* self, re::Edx) {
    bool any = false;
    int minKind = 0;
    int maxKind = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t* driver = At<uint8_t*>(self, 8 + 4 * i);
        if (driver == nullptr) {
            continue;
        }
        int kind = vc::Call<int>(driver, 0x2C);
        if (kind < minKind) {
            minKind = kind;
        }
        if (kind > maxKind) {
            maxKind = kind;
        }
        any = true;
    }
    if (!any) {
        return;
    }
    if (bd2FirstDriverOnly != 0) {
        for (int i = 0; i < 8; i++) {
            uint8_t* driver = At<uint8_t*>(self, 8 + 4 * i);
            if (driver != nullptr) {
                Bd2Quat q;
                At<Bd2Quat>(self, 0x2C) = *vc::Call<Bd2Quat*>(driver, 0x14, &q);
                Bd2Vec v;
                At<Bd2Vec>(self, 0x3C) = *vc::Call<Bd2Vec*>(driver, 0x18, &v);
                return;
            }
        }
    }
    Bd2Transform t;
    vc::Call<void>(self, 0x4, &t);
    Bd2Quat& rot = At<Bd2Quat>(self, 0x2C);
    Bd2_QuatSet(&rot, &t);
    At<Bd2Vec>(self, 0x3C) = t.t;
    for (int kind = maxKind; kind >= minKind; kind--) {
        Bd2Vec pos{0.0f, 0.0f, 0.0f};
        Bd2Quat blended;
        float w = Bd2_GroupBlend(self, kind, &pos, &blended);
        if (!(w > 0.0f)) {
            continue;
        }
        Bd2Quat from = rot;
        Bd2_QuatSlerp(&rot, &from, &blended, w);
        float lengthSq = rot.x * rot.x + rot.y * rot.y;
        lengthSq = lengthSq + rot.z * rot.z;
        lengthSq = lengthSq + rot.w * rot.w;
        float inv = 1.0f / crt::Sqrtf(lengthSq);
        rot.x = rot.x * inv;
        rot.y = rot.y * inv;
        rot.z = inv * rot.z;
        rot.w = inv * rot.w;
        Bd2Vec part{pos.x * w, pos.y * w, pos.z * w};
        float keep = 1.0f - w;
        Bd2Vec& target = At<Bd2Vec>(self, 0x3C);
        Bd2Vec kept{target.x * keep, target.y * keep, keep * target.z};
        Bd2Vec sum;
        target = *Bd2_VectorAdd(&sum, &kept, &part);
    }
}

// 0x731200: the joint group's root applied to the skeleton and the ped: the root joint
// upright at the target (keeping z with 0x20C904C), the ped moved there and turned to it,
// its velocity from the target's
void __fastcall JointGroup_ApplyRoot(uint8_t* self, re::Edx, Bd2Transform* out) {
    if (out == nullptr) {
        return;
    }
    uint8_t* model = At<uint8_t*>(self, 4);
    int root = At<int32_t>(At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10), 0x5C) - 1;
    *out = Joints(model)[root];
    uint8_t* ped = At<uint8_t*>(self, 0x5C);
    Bd2Vec pos = *EntityPos(ped);
    if (bd2KeepRootZ != 0) {
        out->t.z = pos.z;
    }
    Bd2Matrix m;
    Bd2_MatrixFromNi(&m, out);
    Bd2_MatrixSetUp(&m, 0.0f, 0.0f, 1.0f, 0);   // (the fourth dword: the game's stack)
    Bd2_MatrixToNi(&m, out);
    At<float>(At<uint8_t*>(self, 0x5C), 0x10) = trk::CrtAtan2(-m.forward.x, m.forward.y);
    At<Bd2Vec>(At<uint8_t*>(self, 0x5C), 4) = m.pos;
    Bd2_EntityUpdateRW(At<uint8_t*>(self, 0x5C));
    Bd2Vec forward = *Bd2_GetForward(At<uint8_t*>(self, 0x5C), &pos);
    float heading = Bd2_LimitAngle(Bd2_AngleBetweenPoints(forward.x, forward.y, 0.0f, 0.0f));
    Bd2_PedSetHeading(At<uint8_t*>(self, 0x5C), heading);
    Bd2_PedUpdateMoveDirection(At<uint8_t*>(self, 0x5C));
    uint8_t* p = At<uint8_t*>(self, 0x5C);
    At<uint32_t>(p, 0x1720) = bd2Zero[0];
    At<uint32_t>(p, 0x1724) = bd2Zero[1];
    At<uint32_t>(p, 0x1728) = bd2Zero[2];
    float elapsed = float(double(bd2TimeInMs - At<uint32_t>(self, 0x54)));
    Bd2Vec velocity;
    const Bd2Vec* v = Bd2_TargetVelocity(self, &velocity, elapsed);
    p = At<uint8_t*>(self, 0x5C);
    At<Bd2Vec>(p, 0x124) = *v;
    At<Bd2Quat>(At<uint8_t*>(self, 0x5C), 0x172C) = Bd2Quat{0.0f, 0.0f, 0.0f, 1.0f};
}

// 0x731400 (thiscall): the constraint set's constructor (three joint groups, two IK
// groups: no drivers, no targets)
uint8_t* __fastcall Constraints_Construct(uint8_t* self, re::Edx) {
    static const int kGroups[5] = {0, 0x68, 0xD0, 0x138, 0x198};
    for (int g = 0; g < 5; g++) {
        uint8_t* group = self + kGroups[g];
        for (int at = 0x8; at <= 0x24; at += 4) {
            At<uint32_t>(group, at) = 0;
        }
        for (int at = 0x3C; at <= 0x50; at += 4) {
            At<float>(group, at) = 0.0f;
        }
        At<uint32_t>(group, 0) = g < 3 ? kVtJointGroup : kVtIkGroup;
    }
    At<uint32_t>(self, 0x1F8) = 0;
    At<uint32_t>(self, 0x1FC) = 0;
    return self;
}

// 0x7317E0: a group's frame: its drivers updated; with none left it's switched off
// (vf0(0)); once every driver is enabled its target is gathered and applied (vf0(1)) up
// to 0xBC7320 times
void __fastcall Group_Process(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x29) == 0) {
        return;
    }
    Bd2_GroupUpdate(self, bd2FrameRate * bd2TimeStep);
    if (At<uint8_t>(self, 0x2A) == 0) {
        vc::Call<void>(self, 0, 0);
        return;
    }
    bool any = false;
    for (int i = 0; i < 8; i++) {
        uint8_t* driver = At<uint8_t*>(self, 8 + 4 * i);
        if (driver != nullptr) {
            if (At<uint8_t>(driver, 0xC) == 0) {
                return;
            }
            any = true;
        }
    }
    if (any && At<int32_t>(self, 0x58) < bd2MaxApplies) {
        At<int32_t>(self, 0x58) += 1;
        Bd2_Gather(self);
        vc::Call<void>(self, 0, 1);
    }
}

// 0x731870 (JointBlendDriverGroup vf0): the target applied: the root joint from the
// ped's frame, re-rooted at the target (its height eased or dropped towards the
// constraint owner's hips), the model updated, the root applied back to the ped
void __fastcall JointGroup_Apply(uint8_t* self, re::Edx, int on) {
    if (!re::Flag(on)) {
        return;
    }
    Bd2_EntityUpdateFromRW(At<uint8_t*>(self, 0x5C));
    uint8_t* model = At<uint8_t*>(self, 4);
    int root = At<int32_t>(At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10), 0x5C) - 1;
    uint8_t* clump = At<uint8_t*>(At<uint8_t*>(self, 0x5C), 0x18);
    const Bd2Transform* frame = nullptr;
    if (clump != nullptr && At<void*>(clump, 4) != nullptr) {
        frame = Bd2_WorldTransform(At<void*>(clump, 4));
    }
    Joints(At<uint8_t*>(self, 4))[root] = *frame;   // (the game copies from null without one)
    Bd2Transform t;
    t.scale = 1.0f;
    Bd2_QuatToNi(&At<Bd2Quat>(self, 0x2C), &t);
    t.t = At<Bd2Vec>(self, 0x3C);
    if (At<int32_t>(self, 0x60) == 0) {
        if (bd2HipsBlend != 0) {
            float rootZ = Joints(At<uint8_t*>(self, 4))[0].t.z;
            float d = rootZ - t.t.z;
            t.t.z = t.t.z + d * bd2HipsBlendRate;
        } else if (bd2HipsDrop != 0) {
            uint8_t* owner = nullptr;
            for (int i = 0; i < 8; i++) {
                uint8_t* driver = At<uint8_t*>(self, 8 + 4 * i);
                if (driver == nullptr) {
                    continue;
                }
                uint8_t* vtable = At<uint8_t*>(driver, 0);
                int16_t id = Bd2_JointConstraintTypeId();
                if (!re::PtrCast<uint8_t(__thiscall*)(uint8_t*, int)>(At<void*>(vtable, 0))(driver, id)) {
                    continue;
                }
                uint8_t* m2 = At<uint8_t*>(driver, 0x2C);
                if (m2 != nullptr && At<uint8_t*>(m2, 0x9C) != nullptr) {
                    owner = At<uint8_t*>(m2, 0x9C);
                    break;
                }
            }
            if (owner != nullptr) {
                if (bd2HipsFromOwner != 0 && At<uint32_t>(At<uint8_t*>(self, 0x5C), 0x155C) != 0 && At<int32_t>(self, 0x58) <= 1) {
                    Bd2Transform* a = Joints(owner);
                    Bd2Transform* b = Joints(At<uint8_t*>(self, 4));
                    float a60 = At<float>(a, 0x60);
                    float b60 = At<float>(b, 0x60);
                    float ownerDrop = a60 - At<float>(a, 0x2C);
                    float drop = b60 - At<float>(b, 0x2C);
                    float ratio = 1.0f;
                    uint8_t* slot = Bd2_SlotsGetIfNotAnim(At<uint8_t*>(self, 4), 0);
                    if (slot != nullptr && At<uint8_t*>(slot, 0x18) != nullptr) {
                        uint32_t id = At<uint16_t>(At<uint8_t*>(slot, 0x18), 0x32);
                        uint8_t* data = At<uint8_t*>(At<uint8_t*>(self, 4), 0x94);
                        uint8_t* entry = Bd2_AgrGetEntry(At<void*>(data, 0x10), id);
                        if (entry != nullptr) {
                            float length = At<float>(entry, 0x28);
                            if (length != 0.0f) {
                                ratio = At<float>(At<uint8_t*>(data, 0xA8), 0x14) / length;
                            }
                        }
                    }
                    float x = drop / ratio;
                    x = x - ownerDrop;
                    float between = b60 - a60;
                    t.t.z = x - between + t.t.z;
                }
                float rootZ = Joints(At<uint8_t*>(self, 4))[0].t.z;
                float d = rootZ - t.t.z;
                float limit = bd2HipsLimit;
                float minus = -limit;
                float c;
                if (minus > d) {
                    c = minus;
                } else if (d > limit) {
                    c = limit;
                } else {
                    c = d;
                }
                if (bd2HipsDropMin > c && At<int32_t>(self, 0x58) <= 1) {
                    Bd2_ModelTranslate(owner, Bd2Vec{0.0f, 0.0f, c * bd2HipsDropScale}, 1, -1.0f);
                    void* feet = At<void*>(At<uint8_t*>(self, 0x5C), 0x155C);
                    if (feet != nullptr) {
                        Bd2_FeetUpdate(feet);
                    }
                    return;
                }
                if (bd2HipsDropMin < c) {
                    t.t.z = t.t.z + c * (1.0f - bd2HipsDropScale);
                } else {
                    t.t.z = c + t.t.z;
                }
            }
        }
    }
    Bd2_ModelReRoot(At<uint8_t*>(self, 4), At<int32_t>(self, 0x60), &t, 1, -1.0f);
    Bd2_ModelUpdateIfChanged(At<uint8_t*>(self, 4), bd2FrameRate * bd2TimeStep);
    Bd2Transform* world = Bd2_WorldTransform(At<void*>(At<uint8_t*>(self, 4), 0xC));
    if (bd2ApplyRoot != 0 && At<uint8_t>(self, 0x64) != 0) {
        Bd2_ApplyRoot(self, world);
    }
}

} // namespace bdg2

#define BDG2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

BDG2_REVERSED("FloorMotionDriver::Update", 0x00730840, &bdg2::FloorMotionDriver_Update);
BDG2_REVERSED("FloorMotionDriver::GetMatrix", 0x00730EE0, &bdg2::FloorMotionDriver_GetMatrix);
BDG2_REVERSED("ReachDriver::GetMatrix", 0x00730F30, &bdg2::ReachDriver_GetMatrix);
BDG2_REVERSED("JointBlendDriverGroup::Gather", 0x00730F80, &bdg2::JointGroup_Gather);
BDG2_REVERSED("JointBlendDriverGroup::ApplyRoot", 0x00731200, &bdg2::JointGroup_ApplyRoot);
BDG2_REVERSED("PedConstraints::Construct", 0x00731400, &bdg2::Constraints_Construct);
BDG2_REVERSED("BlendDriverGroup::Process", 0x007317E0, &bdg2::Group_Process);
BDG2_REVERSED("JointBlendDriverGroup::Apply", 0x00731870, &bdg2::JointGroup_Apply);
