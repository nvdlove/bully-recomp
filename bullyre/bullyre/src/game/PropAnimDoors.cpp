// CPropAnim as a door: taking hits (vf15), peds attaching to and leaving it (its lock at
// +0x398, the ped at +0x234), how far a ped pushes it open, and its trigger action.
// The hit fields: +0x1BC the hit animation state, +0x1C0 the attacker (registered),
// +0x1C4 where, +0x1DC from which way, +0x1F4 when, +0x1F8 how hard.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVec {
    float x, y, z;
};

template <typename R, typename... A>
R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}

GAME_VAR(uint8_t*, pd_playerPed, 0x00C1AEA8);
GAME_ARRAY(uint8_t*, pd_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, pd_meleeKind, 0x00BF4414);
GAME_VAR(int32_t, pd_meleeModel0, 0x00BF4184);
GAME_VAR(int32_t, pd_meleeModel1, 0x00BF3E60);
GAME_VAR(int32_t, pd_bigDoorModel, 0x00BF40E0);
GAME_VAR(int32_t, pd_hingeRightAnims, 0x00CF3148);
GAME_VAR(float, pd_doorReach, 0x00CF311C);
GAME_VAR(float, pd_halfPi, 0x00A453E4);
GAME_VAR(uint32_t, pd_pushLimitGuard, 0x00C3CC58);
GAME_VAR(float, pd_pushLimit, 0x00C3CC54);
GAME_VAR(int32_t, pd_enterNode, 0x01B654EC);
GAME_VAR(int32_t, pd_enterAnim, 0x00C8E90C);
GAME_VAR(uint32_t, pd_triggerAnimHash, 0x01B65504);
GAME_VAR(int32_t, pd_triggersOn, 0x020C8AB0);
GAME_VAR(uint32_t, pd_nodeIdGuard, 0x00C3C9D8);
GAME_VAR(uint16_t, pd_nodeId, 0x00C3C9D4);
GAME_VAR(uint16_t, pd_nextTypeId, 0x00BF3830);
GAME_VAR(void*, pd_stimuli, 0x00C2C1A0);
inline void* const pd_areaTransitions = reinterpret_cast<void*>(0x00BCC580);
inline void* const pd_enterInput = reinterpret_cast<void*>(0x01B654E0);
inline void* const pd_triggerInput = reinterpret_cast<void*>(0x01B65510);
inline void* const pd_actionTrees = reinterpret_cast<void*>(0x020C7E98);
inline void* const pd_triggers = reinterpret_cast<void*>(0x020C8A00);
inline const char* const pd_spraycan = reinterpret_cast<const char*>(0x0090503C);   // "spraycan"

GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsKind1, 0x00520440, uint8_t* modelInfo);
GAME_FN(uint8_t*, __thiscall, ModelInfo_GetAnimInfo, 0x005201D0, uint8_t* modelInfo);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(CVec*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVec* out);
GAME_FN(CVec*, __thiscall, Placement_GetRight, 0x00473600, void* placement, CVec* out);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVec* v);
GAME_FN(CVec*, __cdecl, VectorAdd, 0x004129A0, CVec* out, const CVec* a, const CVec* b);
GAME_FN(CVec*, __cdecl, VectorSub, 0x00414B70, CVec* out, const CVec* a, const CVec* b);
GAME_FN(CVec*, __cdecl, CrossProduct, 0x00414730, CVec* out, const CVec* a, const CVec* b);
GAME_FN(float, __cdecl, ACos, 0x00413D60, float x);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(uint8_t, __cdecl, IsEntityPointerValid, 0x004657F0, void* entity);
GAME_FN(void, __thiscall, HitAnim_SetState, 0x00472CC0, void* state, int value);
GAME_FN(void, __thiscall, Stimuli_Add, 0x004A6670, void* stimuli, int type, void* from, void* to, uintptr_t, float, float);
GAME_FN(void*, __cdecl, GetRwObjectFrame, 0x005EE1A0, void* rwObject);
GAME_FN(uint8_t*, __cdecl, GetHierarchy, 0x006C7710, void* frame);
GAME_FN(float, __cdecl, GetColWidth, 0x004EB7E0, void* entity);
GAME_FN(int, __thiscall, AreaTransition_GetTarget, 0x004152F0, void* self);
GAME_FN(void, __thiscall, AreaTransition_SetTarget, 0x004152B0, void* self, void* target);
GAME_FN(uint8_t, __thiscall, Controller_IsNodePlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(void, __thiscall, Controller_Stop, 0x005F40A0, void* controller);
GAME_FN(void*, __thiscall, Controller_GetNode, 0x005F49E0, void* controller, int index);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, void* entity, int node, float* transform, int flag);
GAME_FN(void, __thiscall, EntityRef_Set, 0x00471300, void* ref, void* entity, int value);
GAME_FN(void, __thiscall, PedUsePoint_Set, 0x00478850, void* usePoint, void* prop);
GAME_FN(void, __thiscall, Ped_PlayUseAnim, 0x00474100, void* ped, int anim);
GAME_FN(void, __thiscall, Ped_SetUsing, 0x0048BFB0, void* usePoint, int model);
GAME_FN(uint32_t*, __thiscall, Hash_Construct, 0x005FA870, uint32_t* hash, uint32_t value);
GAME_FN(void, __thiscall, Hash_Destruct, 0x0049A650, uint32_t* hash);
GAME_FN(void*, __thiscall, ActionTrees_Get, 0x006D7A60, void* trees, void* key);
GAME_FN(int32_t*, __thiscall, Node_GetId, 0x00655B80, void* node);
GAME_FN(void, __thiscall, Triggers_Fire, 0x0072C840, void* triggers, void* tree, void* action, int useId, int nodeId);
// (ours, through their addresses)
GAME_FN(uint8_t, __thiscall, DoorLock_Request, 0x004EADC0, void* lock, void* ped, int request, int);
GAME_FN(void, __thiscall, Door_SetCurrent, 0x004ED940, void* door);

const CVec& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVec*>(matrix + 0x30) : At<CVec>(entity, 4);
}

float Length(const CVec& v) {
    float sq = float(double(v.x) * v.x + double(v.y) * v.y + double(v.z) * v.z);
    sq = std::fabs(sq);
    return std::sqrt(sq);
}

// 0x4ED4E0 (thiscall, vf15, 18 arguments): a hit on the prop (0 when ignored: too soon
// after the last one, while it's in use, blocked, or by a spray can)
uint8_t __fastcall TakeHit(uint8_t* self, re::Edx, float time, float force, uint8_t* attacker, int kind, const CVec* pos,
                           const CVec* dir, int blocked, int, const char* weapon, int flag, float value11, float value12,
                           int, int, int, int, int, int value18) {
    if (!(At<float>(self, 0x1E8) != 0.0f || !(At<float>(self, 0x1F4) > time))) {
        return 0;
    }
    if (PAnimModelInfo_IsKind1(pd_modelInfoPtrs[At<int16_t>(self, 0x10E)]) && At<int32_t>(self, 0x184) != 0) {
        return 0;
    }
    if (blocked != 0) {
        return 0;
    }
    if (weapon != nullptr) {
        uint32_t hash = HashName(weapon);
        if (hash == HashName(pd_spraycan)) {
            return 0;
        }
    }
    CVec d = {0.0f, 0.0f, 0.0f};
    uint8_t fromFront = 0;
    CVec forward;
    Entity_GetForward(self, &forward);
    CVec back = {-forward.x, -forward.y, -forward.z};
    d = back;
    if (attacker != nullptr) {
        CVec temp;
        d = *VectorSub(&temp, &Position(self), &Position(attacker));
    } else if (dir != nullptr) {
        d = *dir;
    }
    Vector_Normalise(&d);
    float dot = float(double(forward.y) * d.y + double(forward.x) * d.x + double(forward.z) * d.z);
    if (0.0f < dot) {
        fromFront = 1;
    }
    self[0x1BC] = 1;
    self[0x1F0] = uint8_t(flag);
    if (attacker != nullptr || dir != nullptr) {
        At<CVec>(self, 0x1DC) = d;
    }
    At<CVec>(self, 0x1C4) = pos != nullptr ? *pos : Position(self);
    if (weapon != nullptr) {
        At<uint32_t>(self, 0x220) = HashName(weapon);
    }
    re::Ptr32<void>& ref = At<void*>(self, 0x1C0);
    if (attacker != ref) {
        if (ref != nullptr) {
            CleanUpOldReference(ref, &ref);
        }
        ref = attacker;
        if (attacker != nullptr) {
            RegisterReference(attacker, &ref);
        }
    }
    HitAnim_SetState(self + 0x1BC, fromFront ? 2 : 1);
    At<float>(self, 0x1F4) = time;
    At<float>(self, 0x1F8) = force;
    At<int16_t>(self, 0x21A) = int16_t(kind);
    At<float>(self, 0x208) = value11;
    At<int32_t>(self, 0x21C) = value18;
    At<int32_t>(self, 0x188) = 0;
    At<float>(self, 0x20C) = value12;
    if (attacker != nullptr) {
        if (kind != pd_meleeKind || (At<int16_t>(self, 0x10E) != pd_meleeModel0 && At<int16_t>(self, 0x10E) != pd_meleeModel1)) {
            Stimuli_Add(pd_stimuli, 2, attacker, self, 0, -1.0f, -1.0f);
        }
        At<int32_t>(self, 0x238) = VCall<int32_t>(attacker, 0xAC);
    }
    return 1;
}

// 0x4ED830 (thiscall): a ped starts using the door (the lock asked, both sides linked)
void __fastcall AttachPed(uint8_t* self, re::Edx, uint8_t* ped) {
    DoorLock_Request(self + 0x398, ped, At<int32_t>(ped, 0x1310) == 0xD ? 3 : 2, 1);
    re::Ptr32<void>& user = At<void*>(self, 0x234);
    if (ped != user) {
        if (user != nullptr) {
            CleanUpOldReference(user, &user);
        }
        user = ped;
        RegisterReference(ped, &user);
    }
    re::Ptr32<void>& used = At<void*>(ped, 0x548);
    if (self != used) {
        if (used != nullptr) {
            CleanUpOldReference(used, &used);
        }
        used = self;
        if (self != nullptr) {
            RegisterReference(self, &used);
        }
    }
    Ped_SetUsing(ped + 0x3BC, At<int16_t>(self, 0x10E));
    At<int32_t>(ped, 0x1520) = 1;
}

// 0x4EE060 (thiscall): the ped leaves it (its action stopped while the ped's still valid)
void __fastcall DetachPed(uint8_t* self, re::Edx, uint8_t* ped) {
    DoorLock_Request(self + 0x398, ped, 0, 1);
    re::Ptr32<void>& user = At<void*>(self, 0x234);
    if (IsEntityPointerValid(user)) {
        if (user != nullptr) {
            CleanUpOldReference(user, &user);
            user = nullptr;
        }
        return;
    }
    Door_SetCurrent(self);
    Controller_Stop(self + 0x128);
}

// 0x4EDEB0 (thiscall): fires the prop's trigger (its trigger animation, a trigger set,
// and triggers on; when fully open or forced), with the id of its playing trigger node
void __fastcall FireTrigger(uint8_t* self, re::Edx, int force) {
    uint32_t hash = 0;
    Door_SetCurrent(self);
    Hash_Construct(&hash, At<uint32_t>(ModelInfo_GetAnimInfo(pd_modelInfoPtrs[At<int16_t>(self, 0x10E)]), 0x80));
    bool fire = pd_triggerAnimHash == hash && At<void*>(self, 0x138) != nullptr && pd_triggersOn != 0 &&
                (At<float>(self, 0x254) == 1.0f || uint8_t(force) == 1);
    Hash_Destruct(&hash);
    if (!fire) {
        return;
    }
    void* tree = ActionTrees_Get(pd_actionTrees, At<void*>(self, 0x23C));
    int nodeId = -1;
    if (Controller_IsNodePlaying(self + 0x128, pd_triggerInput, 1, 1)) {
        void* found = nullptr;
        for (int i = 0; i < At<int32_t>(self, 0x15C); i++) {
            void* node = Controller_GetNode(self + 0x128, i);
            if (node == nullptr) {
                continue;
            }
            if (!(pd_nodeIdGuard & 1)) {
                pd_nodeIdGuard |= 1;
                pd_nodeId = pd_nextTypeId;
                pd_nextTypeId = uint16_t(pd_nextTypeId + 1);
            }
            if (VCall<uint8_t>(node, 0, uint32_t(pd_nodeId))) {
                found = node;
                break;
            }
        }
        // (null when none is: the game reads through it all the same)
        nodeId = *Node_GetId(found);
    }
    Triggers_Fire(pd_triggers, tree, At<void*>(self, 0x138), At<int16_t>(self, 0x22C), nodeId);
}

// 0x4EE0D0 (thiscall): how far a ped pushes the door open (0..1, by the angle between
// the door and the way the ped comes at it past the hinge; 0 when out of reach or below
// it). The player pushing a door to another area also starts the area change and walks in
void __fastcall GetPush(uint8_t* self, re::Edx, uint8_t* ped, float* amount) {
    *amount = 0.0f;
    if (At<void*>(self, 0x18) == nullptr) {
        return;
    }
    void* frame = GetRwObjectFrame(At<void*>(self, 0x18));
    uint8_t* hinge;
    if (At<int16_t>(self, 0x10E) == pd_bigDoorModel) {
        hinge = GetHierarchy(frame) + 0x208;
    } else {
        hinge = GetHierarchy(frame) + 0x34;
    }
    float width = GetColWidth(self);
    bool hingeRight = At<int32_t>(pd_modelInfoPtrs[At<int16_t>(self, 0x10E)], 0x80) == pd_hingeRightAnims;
    CVec right;
    float* matrix = At<float*>(self, 0x14);
    if (matrix != nullptr) {
        right = {matrix[0], matrix[1], matrix[2]};
    } else {
        Placement_GetRight(self + 4, &right);
    }
    CVec hingePos = At<CVec>(hinge, 0x24);
    CVec pedPos = Position(ped);
    if (!(pedPos.z < double(hingePos.z) + width)) {
        return;
    }
    hingePos.z = pedPos.z;
    if (hingeRight) {
        right.x = float(right.x * -1.0);
        right.y = float(right.y * -1.0);
        right.z = float(right.z * -1.0);
    }
    Vector_Normalise(&right);
    CVec span = {right.x * width, right.y * width, width * right.z};
    CVec half = {float(span.x * 0.5), float(span.y * 0.5), float(span.z * 0.5)};
    CVec centre;
    VectorAdd(&centre, &hingePos, &half);
    CVec toPed;
    VectorSub(&toPed, &pedPos, &centre);
    float dist = Length(toPed);
    float reach = pd_doorReach;
    float limit = reach + width;
    if (!(dist < limit)) {
        return;
    }
    CVec out;
    VectorSub(&out, &pedPos, &hingePos);
    Vector_Normalise(&out);
    CVec forward;
    Entity_GetForward(self, &forward);
    Vector_Normalise(&forward);
    CVec zAxis = {0.0f, 0.0f, 1.0f};
    CVec side;
    CrossProduct(&side, &out, &zAxis);
    Vector_Normalise(&side);
    float k = hingeRight ? float(reach + 0.30000001192092896) : float(-(reach + 0.30000001192092896));
    side.x = k * side.x;
    side.y = side.y * k;
    side.z = k * side.z;
    CVec along = {out.x * dist, out.y * dist, dist * out.z};
    CVec way;
    VectorAdd(&way, &along, &side);
    Vector_Normalise(&way);
    float dot1 = float(double(way.y) * right.y + double(way.x) * right.x + double(way.z) * right.z);
    float angle = std::fabs(ACos(dot1));
    float dot2 = float(double(forward.y) * way.y + double(forward.x) * way.x + double(forward.z) * way.z);
    float offAxis = std::fabs(ACos(dot2));
    if (!(pd_pushLimitGuard & 1)) {
        pd_pushLimitGuard |= 1;
        pd_pushLimit = float(pd_halfPi + 0.17000000178813934);
    }
    if (!(offAxis <= pd_pushLimit)) {
        return;
    }
    float push = angle / pd_pushLimit;
    *amount = push;
    push = push < 1.0f ? push : 1.0f;
    *amount = push;
    uint8_t* player = pd_playerPed;
    if (At<int32_t>(ped, 0x1310) != 0xD || At<int32_t>(self, 0x194) == 0 || AreaTransition_GetTarget(pd_areaTransitions) != 0) {
        return;
    }
    AreaTransition_SetTarget(pd_areaTransitions, At<void*>(self, 0x23C));
    if (Controller_IsNodePlaying(At<uint8_t*>(player, 0x2E0) + 0x258, pd_enterInput, 1, 1)) {
        return;
    }
    float transform[13];
    transform[12] = 1.0f;
    if (GetNodeTransform(self, pd_enterNode, transform, 0)) {
        EntityRef_Set(player + 0x438, self, pd_enterNode);
        PedUsePoint_Set(player + 0x3BC, self);
        Ped_PlayUseAnim(player, pd_enterAnim);
    }
}

#define C13D_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C13D_REVERSED("CPropAnim::TakeHit", 0x004ED4E0, &TakeHit);
C13D_REVERSED("CPropAnim::AttachPed", 0x004ED830, &AttachPed);
C13D_REVERSED("CPropAnim::FireTrigger", 0x004EDEB0, &FireTrigger);
C13D_REVERSED("CPropAnim::DetachPed", 0x004EE060, &DetachPed);
C13D_REVERSED("CPropAnim::GetPush", 0x004EE0D0, &GetPush);

} // namespace
