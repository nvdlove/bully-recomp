// The action tree's tracks of 0x6158A0..0x617B00: target face (its target pick and
// Update), sync (lining a ped up with another entity's node), IK attach / detach, ped
// reset, radar blips, breath, water ripple and weapon impact effects, the grind rail
// driver, set pos / rot, beam to a safe position, and the hit animation's Update.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"
#include "reversed.h"

using namespace actc;

namespace {

// a NiTransform: rotation, translation, scale
struct NiXform {
    float rot[9];     // +0x0
    float trans[3];   // +0x24
    float scale;      // +0x30
};
static_assert(sizeof(NiXform) == 0x34);
// a CMatrix with its rows' fourth words (right, forward, up, position; the attached
// matrix pointer)
struct Matrix44 {
    float right[4];    // +0x0
    float fwd[4];      // +0x10
    float up[4];       // +0x20
    float pos[4];      // +0x30
    uint32_t attach;   // +0x40
};
static_assert(sizeof(Matrix44) == 0x44);

GAME_VAR(int32_t, gameMode, 0x00C3CCF8);
GAME_VAR(uint8_t*, struggleButton, 0x00C674E4);
GAME_VAR(uint8_t*, hudRadar, 0x00C674CC);
GAME_VAR(void*, effectManager, 0x00B6F1D4);
GAME_VAR(void*, effectLibrary, 0x00B6F1E8);
GAME_VAR(float, railOffset, 0x01B6503C);
GAME_ARRAY(float, railLift, 0x00B0B1A0);
GAME_VAR(float, pedResetStep, 0x0090D0C0);   // (0.016)
inline void* const routes = reinterpret_cast<void*>(0x00BC5E7C);
inline void* const triggers = reinterpret_cast<void*>(0x020C7E98);
inline const float* const zAxis = reinterpret_cast<const float*>(0x009E33DC);
constexpr uint32_t kBlipColour = 0xFFBC7200;   // (r 0, g 0x72, b 0xBC, a 0xFF)

GAME_FN(uint8_t*, __thiscall, Targeting_GetUnknown9C, 0x004D62C0, uint8_t* targeting);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget, 0x004D5AD0, uint8_t* targeting);
GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget, 0x004D7780, uint8_t* targeting);
GAME_FN(void, __thiscall, Targeting_SetAim_x64, 0x004D5F70, uint8_t* targeting, float, float, float, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Targeting_SetAim(uint8_t* a0, V3 a1, int a2) {
    return Targeting_SetAim_x64(a0, a1.x, a1.y, a1.z, a2);
}
GAME_FN(void, __thiscall, Targeting_Update, 0x004D8A40, uint8_t* targeting, int);
GAME_FN(void, __thiscall, TargetFaceTrack_SetTarget, 0x0060F110, uint8_t* self, uint8_t* target);
GAME_FN(void, __thiscall, TargetFaceTrack_Turn, 0x0060F1F0, uint8_t* self, float dt);
GAME_FN(void, __thiscall, TargetFaceTrack_PickTarget, 0x006158A0, uint8_t* self);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(float, __cdecl, AngleOfVector2D, 0x00433190, const V3* v);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(uint8_t*, __thiscall, Ped_GetMatrix, 0x004734D0, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const V3* v);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(uint8_t*, __thiscall, FaceHeadingObjective_Construct, 0x00470B20, void* self, uint8_t* ped, float heading,
        int);
GAME_FN(void, __thiscall, Objectives_Prepend, 0x00471470, uint8_t* queue, uint8_t* objective);
GAME_FN(uint8_t*, __thiscall, Skeleton_FindBone, 0x00465580, void* skeleton, uint32_t hash, int32_t* bone);
GAME_FN(void, __thiscall, Vector4_Cross, 0x007D5660, const float* self, float* out, const float* other);
GAME_FN(void, __thiscall, Vector_Assign, 0x004129D0, float* dst, const float* src);
GAME_FN(V3*, __cdecl, TransformPoint, 0x005E17D0, V3* out, const void* matrix, const V3* v);
GAME_FN(void, __thiscall, Matrix_ToNiTransform, 0x00412AC0, const void* matrix, NiXform* out);
GAME_FN(void, __cdecl, Entity_GetNodeTransform, 0x00467D80, uint8_t* entity, uint32_t hash, NiXform* out, int);
GAME_FN(void, __thiscall, Ped_SetPosition, 0x0041EE20, uint8_t* ped, const V3* pos);
GAME_FN(void, __thiscall, Placeable_SetPosition, 0x0041C4E0, uint8_t* placeable, float x, float y, float z);
GAME_FN(float, __thiscall, Vector2D_Magnitude, 0x004246B0, const void* v);
GAME_FN(float, __cdecl, Min, 0x00414470, float a, float b);
GAME_FN(float, __cdecl, Max, 0x00414490, float a, float b);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, uint8_t* matrix, float angle);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x004734C0, uint8_t* ped, float heading);
GAME_FN(void, __thiscall, Ped_UpdateMoveDirection, 0x00474570, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, SyncTrack_Align, 0x00615960, uint8_t* self, uint8_t* other);
GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity2, 0x004EAC00, uint8_t* owner);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00443AA0, uint8_t* entity);
GAME_FN(void, __cdecl, Node_Update, 0x006C9AF0, void* node);
GAME_FN(float, __cdecl, AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(uint8_t*, __cdecl, FindPlayingAnimation, 0x00611A90, uint8_t* owner, uintptr_t name);
GAME_FN(void, __cdecl, Track_GetBone, 0x00611B40, uint8_t* owner, uint8_t* track, uint32_t* first, uint32_t* second);
GAME_FN(int, __thiscall, Request_Set, 0x00607580, uint8_t* request, uintptr_t who, uint32_t what, float time);
GAME_FN(void, __thiscall, MoveTarget_Set, 0x00472D70, uint8_t* target, const float* pos, float time);
GAME_FN(void, __thiscall, AnimManager_Detach, 0x006C2200, void* manager, uint8_t* clump, uintptr_t anim);
GAME_FN(void, __thiscall, Ped_ClearPartner, 0x00477470, uint8_t* ped);
GAME_FN(void, __thiscall, PropAnim_DetachPed, 0x004EE060, uint8_t* prop, uint8_t* ped);
GAME_FN(void, __thiscall, PedCombat_ReleaseSlots, 0x00486AB0, uint8_t* combat);
GAME_FN(int32_t, __thiscall, Radar_AddBlip, 0x0054A410, uint8_t* radar, int type, float x, float y, float z,
        uint32_t colour, int display, int sprite, int ownStorey);
GAME_FN(int32_t, __thiscall, Radar_AddEntityBlip, 0x0054A5B0, uint8_t* radar, int type, int handle, uint32_t colour,
        int display, int sprite, int ownStorey);
GAME_FN(uint8_t*, __thiscall, Routes_Find, 0x006D6410, void* routes, uintptr_t id);
GAME_FN(float*, __thiscall, Route_First, 0x006D59B0, uint8_t* route);
GAME_FN(float*, __thiscall, Route_Next, 0x006D59F0, uint8_t* route);
GAME_FN(int32_t, __thiscall, Triggers_FindByName, 0x006D7A10, void* triggers, const char* name);
GAME_FN(void*, __thiscall, Triggers_Get, 0x00415FD0, void* triggers, int32_t index);
GAME_FN(void*, __cdecl, Trigger_GetPropAnim, 0x004EA3C0, void* trigger);
GAME_FN(int32_t, __cdecl, PropAnim_GetRef, 0x0044CB20, void* prop);
GAME_FN(void, __thiscall, Colour_FromUint, 0x00424420, uint32_t* colour, uint32_t rgba);
GAME_FN(void, __thiscall, Breath_Emit, 0x006102A0, uint8_t* track);
GAME_FN(void, __thiscall, AE_Manager_Start, 0x00668290, void* manager, const int32_t* type, uint8_t* proxy, int);
GAME_FN(void, __thiscall, AE_Proxy_Kill, 0x00668BA0, uint8_t* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Fade_x64, 0x00668C40, uint8_t* proxy, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AE_Proxy_Fade(uint8_t* a0, float a1) {
    return AE_Proxy_Fade_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, AE_Proxy_SetPosition, 0x00669040, uint8_t* proxy, const float* pos, int);
GAME_FN(void, __thiscall, AE_Proxy_SetDirection, 0x00669360, uint8_t* proxy, const V3* dir);
GAME_FN(uint16_t, __thiscall, EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, AnimManager_GetNodePosition, 0x006C1190, void* manager, void* a, void* b, V3* out, uintptr_t);
GAME_FN(float, __thiscall, FXWaterRipplesTrack_WaterLevel, 0x00610920, uint8_t* self);
GAME_FN(uint8_t, __cdecl, Anim_FindNode, 0x0060AE00, void* outA, void* outB, const char* name, uint8_t* model);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const V3* start, const V3* end, ColPoint* colPoint,
        re::Ptr32<uint8_t>* entity, int flags, int, int);
GAME_FN(uint8_t*, __thiscall, PropAnim_GetRailList, 0x004EAE90, uint8_t* prop);
GAME_FN(uint8_t*, __thiscall, RailList_NextAfter, 0x004F0400, uint8_t* list, uint8_t* point);
GAME_FN(float*, __thiscall, RailPoint_GetPos, 0x004F02A0, uint8_t* point);
GAME_FN(V3*, __thiscall, Placement_GetRight, 0x00473600, uint8_t* placement, V3* out);
GAME_FN(void, __thiscall, RailDriverTrack_Raise, 0x00616D20, uint8_t* self, uint8_t* ped);
GAME_FN(void, __thiscall, RailDriverTrack_Lower, 0x00616DE0, uint8_t* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, RailDriverTrack_Advance, 0x00617760, uint8_t* self, uint8_t* point, float distance);
GAME_FN(void, __thiscall, Ped_SetHeadingNow, 0x00475730, uint8_t* ped, float heading);
GAME_FN(float, __cdecl, LinearMap, 0x00432B60, float x, float y0, float x0, float y1, float x1);
GAME_FN(float, __cdecl, Clamp, 0x0041A470, float value, float low, float high);
GAME_FN(uint8_t, __thiscall, AnimationTrack_Update, 0x0060AF60, uint8_t* self, float dt);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, uint8_t* blend, int32_t slot);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(void, __thiscall, AnimManager_SetSpeed, 0x006C0E60, void* manager, uint8_t* blend, uint8_t* anim, int,
        float speed);
GAME_FN(uint8_t, __cdecl, NearlyEqual, 0x00432A60, float a, float b, float epsilon);

constexpr float kEpsilon = 1e-5f;
constexpr float kPi = 3.14159274f;

void SetPedHeading(uint8_t* ped, float heading) {
    if (At<uint8_t*>(ped, 0x14)) {
        Matrix_SetRotateZOnly(At<uint8_t*>(ped, 0x14), heading);
    }
    At<float>(ped, 0x10) = heading;
    Ped_SetHeading(ped, heading);
}
void UpdateFrame(uint8_t* entity) {
    Entity_UpdateRwFrame(entity);
    Node_Update(At<void*>(At<uint8_t*>(entity, 0x114), 0xC));
}
// an IK request's hold cleared (+0x38 bit 0, +0x28 / +0x2C its times)
void ClearIk(uint8_t* ik) {
    At<uint32_t>(ik, 0x38) &= ~1u;
    At<float>(ik, 0x2C) = 0.0f;
    At<float>(ik, 0x28) = 0.0f;
}

// 0x6158A0 (thiscall): TargetFaceTrack's target: the targeting's pending one (+0x1434),
// the player's lock-on (button 10), a ped's attacker (+0xBB4), else its look target
void __fastcall TargetFaceTrack_PickTargetImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    if (At<int32_t>(ped, 0x1434) != 0 && Targeting_GetUnknown9C(At<uint8_t*>(ped, 0x1C8))) {
        TargetFaceTrack_SetTarget(self, Targeting_GetUnknown9C(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x3C)), 0x1C8)));
        return;
    }
    if (At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x3C)), 0x1310) == 0xD && !(Pad_GetValue(10, 0) == 0.0f)) {
        TargetFaceTrack_SetTarget(self, Targeting_GetTarget(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x3C)), 0x1C8)));
        return;
    }
    ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    if (At<int32_t>(ped, 0x1310) != 0xD && At<uint8_t*>(ped, 0xBB4)) {
        TargetFaceTrack_SetTarget(self, At<uint8_t*>(ped, 0xBB4));
        return;
    }
    TargetFaceTrack_SetTarget(self, Targeting_GetLookTarget(At<uint8_t*>(ped, 0x1C8)));
}

// 0x615960 (thiscall): SyncTrack's alignment with other: the ped put at (+0x30 1), or
// moved towards over +0x30 a step (+0x38 its start, +0x40 its progress), the other's
// node (+0x2C, or the ped's own +0x43C when other is its +0x438) and turned along it;
// no node: just turned to face other. Model 0x2762: a face heading objective instead.
// 1 while still moving.
uint8_t __fastcall SyncTrack_AlignImpl(uint8_t* self, re::Edx, uint8_t* other) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x34));
    uint8_t moved = 0;
    if (!other) {
        return 0;
    }
    V3 face = {0.0f, 0.0f, 0.0f};
    if (At<int16_t>(other, 0x10E) == 0x2762) {
        uint8_t* m = Ped_GetMatrix(ped);
        V3 f = {At<float>(m, 0x10), At<float>(m, 0x14), At<float>(m, 0x18)};
        Vector_Normalise(&f);
        f.z = 0.0f;
        float heading = HeadingFromVector(&f);
        void* memory = Malloc(0x18);
        if (memory) {
            Objectives_Prepend(ped + 0x5F0, FaceHeadingObjective_Construct(memory, ped, heading, 1));
            return 0;
        }
        Objectives_Prepend(ped + 0x5F0, nullptr);
        return 0;
    }
    uint32_t bone = 0;
    if (!vc::Call<uint8_t>(other, 0x14) && At<uint32_t>(self, 0x2C) != 0) {
        bone = At<uint32_t>(self, 0x2C);
    } else if (other == At<uint8_t*>(ped, 0x438)) {
        bone = At<uint32_t>(ped, 0x43C);
    }
    if (bone == 0) {
        V3 d;
        VectorSub(&d, Pos(other), Pos(ped));
        face = {d.x, d.y, 0.0f};
        Vector_Normalise(&face);
    } else {
        // (a vehicle's node that isn't found leaves the transform as the stack had it)
        NiXform node;
        node.scale = 1.0f;
        if ((other[0x108] & 7) == 2 && At<int32_t>(other, 0x46C) == 1) {
            uint8_t* clump = At<uint8_t*>(other, 0x114);
            uint8_t* found = nullptr;
            if (clump && At<uint8_t*>(clump, 0x94)) {
                found = Skeleton_FindBone(At<uint8_t*>(clump, 0x94), bone, nullptr);
            }
            if (found) {
                // the node's position in the vehicle's frame flattened to the world's up,
                // turned a quarter towards the side it's on
                const float* at = vc::Call<const float*>(found, 0);
                V3 local = {at[0], at[1], at[2]};
                Matrix44 frame;   // (the rows' fourth words: whatever the stack had)
                frame.attach = 0;
                uint8_t* m = Placeable_GetMatrix(other);
                for (int i = 0; i < 4; i++) {
                    frame.fwd[i] = At<float>(m, 0x10 + i * 4);
                }
                frame.up[0] = zAxis[0];
                frame.up[1] = zAxis[1];
                frame.up[2] = zAxis[2];
                float cross[4];
                Vector4_Cross(frame.fwd, cross, frame.up);
                Vector_Assign(frame.right, cross);
                Vector4_Cross(frame.up, cross, frame.right);
                for (int i = 0; i < 4; i++) {
                    frame.fwd[i] = cross[i];
                }
                m = Placeable_GetMatrix(other);
                for (int i = 0; i < 4; i++) {
                    frame.pos[i] = At<float>(m, 0x30 + i * 4);
                }
                V3 world;
                TransformPoint(&world, &frame, &local);
                Matrix44 turned;
                turned.attach = 0;
                if (local.x < 0.0f) {
                    float negFwd[3] = {-frame.fwd[0], -frame.fwd[1], -frame.fwd[2]};
                    for (int i = 0; i < 4; i++) {
                        turned.fwd[i] = frame.right[i];
                    }
                    Vector_Assign(turned.right, negFwd);
                } else {
                    float negRight[3] = {-frame.right[0], -frame.right[1], -frame.right[2]};
                    turned.fwd[0] = negRight[0];
                    turned.fwd[1] = negRight[1];
                    turned.fwd[2] = negRight[2];
                    Vector_Assign(turned.right, frame.fwd);
                }
                for (int i = 0; i < 4; i++) {
                    turned.up[i] = frame.up[i];
                }
                turned.pos[0] = world.x;
                turned.pos[1] = world.y;
                turned.pos[2] = world.z;
                Matrix_ToNiTransform(&turned, &node);
            }
        } else {
            Entity_GetNodeTransform(other, bone, &node, 1);
        }
        face = {node.rot[1], node.rot[4], node.rot[7]};
        float rate = At<float>(self, 0x30);
        if (1.0f == rate) {
            V3 p = {node.trans[0], node.trans[1], node.trans[2]};
            Ped_SetPosition(ped, &p);
        } else if (0.0f < rate && rate < 1.0f) {
            const float* base = Pos(ped);
            float bx = base[0];
            float by = base[1];
            float fromStart[2] = {node.trans[0] - At<float>(self, 0x38), node.trans[1] - At<float>(self, 0x3C)};
            float total = Vector2D_Magnitude(fromStart);
            float toGo[2] = {node.trans[0] - bx, node.trans[1] - by};
            float left = Vector2D_Magnitude(toGo);
            float progress = At<float>(self, 0x40);
            if (total > 0.0f) {
                progress = (total - left) / total + progress;
            }
            progress = Min(progress, 1.0f);
            if (progress < 0.2f) {
                if (At<float>(self, 0x40) < At<float>(self, 0x30)) {
                    At<float>(self, 0x40) =
                        Min(At<float>(self, 0x30) / 7.0f + At<float>(self, 0x40), At<float>(self, 0x30));
                }
            } else if (progress > 0.8f && 0.0f < At<float>(self, 0x40)) {
                float least = At<float>(self, 0x30) / 5.0f;
                At<float>(self, 0x40) = Max(At<float>(self, 0x40) - At<float>(self, 0x30) / 7.0f, least);
            }
            if (progress < 0.98f && 0.04f < left) {
                V3 p;
                p.x = fromStart[0] * progress + At<float>(self, 0x38);
                p.y = progress * fromStart[1] + At<float>(self, 0x3C);
                p.z = Pos(ped)[2];
                Ped_SetPosition(ped, &p);
                moved = 1;
            } else {
                V3 p = {node.trans[0], node.trans[1], node.trans[2]};
                Ped_SetPosition(ped, &p);
                moved = 0;
            }
        }
    }
    SetPedHeading(ped, HeadingFromVector(&face));
    Ped_UpdateMoveDirection(ped);
    return moved;
}

// 0x616060 (thiscall): SyncTrack::Start: the start (+0x38) kept; by mode +0x28: aligned
// with the ped's +0x430 (2) or its use point's entity (3); turned to its own matrix
// and put at it (4); else turned by +0x24 from the hit's direction (6: the ped's own
// +0x1874 when it has one)
uint8_t __fastcall SyncTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x34) = owner;
    uint8_t* ped = OwnerPed(owner);
    const float* pos = Pos(ped);
    At<float>(self, 0x38) = pos[0];
    At<float>(self, 0x3C) = pos[1];
    At<float>(self, 0x40) = 0.0f;
    float angle;
    switch (At<int32_t>(self, 0x28)) {
    case 2:
        SyncTrack_Align(self, At<uint8_t*>(ped, 0x430));
        return 1;
    case 3:
        SyncTrack_Align(self, UsePoint_GetEntity2(At<uint8_t*>(self, 0x34)));
        UpdateFrame(ped);
        return 1;
    case 4: {
        uint8_t* m = Ped_GetMatrix(ped);
        float heading = -trk::CrtAtan2(At<float>(m, 0x14), At<float>(m, 0x10));
        SetPedHeading(ped, heading);
        Ped_UpdateMoveDirection(ped);
        Placeable_SetPosition(ped, At<float>(m, 0x30), At<float>(m, 0x34), At<float>(m, 0x38));
        UpdateFrame(ped);
        return 1;
    }
    case 6: {
        float d[3];
        if (At<int32_t>(ped, 0x13A8) != 0 && At<int32_t>(ped, 0x13AC) != 0) {
            d[0] = At<float>(ped, 0x1874);
            d[1] = At<float>(ped, 0x1878);
            d[2] = At<float>(ped, 0x187C);
        } else {
            const float* hit = &At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x34), 0x10), 0x20);
            d[0] = hit[0];
            d[1] = hit[1];
            d[2] = hit[2];
        }
        float a = AngleBetweenPoints(d[0], d[1], 0.0f, 0.0f);
        angle = At<float>(self, 0x24) + a;
        break;
    }
    default: {
        const float* hit = &At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x34), 0x10), 0x20);
        float a = AngleBetweenPoints(hit[0], hit[1], 0.0f, 0.0f);
        angle = At<float>(self, 0x24) + a;
        break;
    }
    }
    SetPedHeading(ped, LimitAngle(angle));
    Ped_UpdateMoveDirection(ped);
    return 1;
}

// 0x6162D0 (thiscall): SyncTrack::Update: still aligning while it moves (modes 2 and 3,
// only with a rate)
uint8_t __fastcall SyncTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x34));
    if (!(0.0f == At<float>(self, 0x30))) {
        int32_t mode = At<int32_t>(self, 0x28);
        if (mode == 2) {
            return SyncTrack_Align(self, At<uint8_t*>(ped, 0x430));
        }
        if (mode == 3) {
            uint8_t moved = SyncTrack_Align(self, UsePoint_GetEntity2(At<uint8_t*>(self, 0x34)));
            UpdateFrame(ped);
            return moved;
        }
    }
    return 0;
}

// 0x616350 (thiscall): AttachIKTrack::Start: mode 4 (+0x18): the ped's IK on (+0x13A0);
// else its playing animation's (+0x24) IK request set to the track's bone
uint8_t __fastcall AttachIKTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    if (At<int32_t>(self, 0x18) == 4) {
        At<int32_t>(OwnerPed(owner), 0x13A0) = 1;
        return 1;
    }
    uint8_t* ik = FindPlayingAnimation(owner, At<uint32_t>(self, 0x24));
    if (ik) {
        uint32_t first = 0;
        uint32_t second = 0;
        Track_GetBone(At<uint8_t*>(self, 0x20), self, &first, &second);
        if (second) {
            At<uint32_t>(ik, 0x38) |= 1;
            At<float>(ik, 0x2C) = 0.0f;
            At<float>(ik, 0x28) = 0.0f;
            Request_Set(ik, second, first, 0.0f);
        }
    }
    return 1;
}

// 0x6163E0 (thiscall): DetachIKTrack::Start: the playing animation's (+0x18) IK request
// released to nowhere
uint8_t __fastcall DetachIKTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ik = FindPlayingAnimation(owner, At<uint32_t>(self, 0x18));
    if (ik) {
        ClearIk(ik);
        float zero[3] = {0.0f, 0.0f, 0.0f};
        MoveTarget_Set(ik, zero, 0.0f);
    }
    return 1;
}

// 0x616430 (thiscall): PedResetTrack::Start: the ped's IK, attached animation, struggle
// prompt, partner, prop, target and combat slots let go, and its state flags back to
// their defaults
uint8_t __fastcall PedResetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (At<uint8_t*>(ped, 0x16EC)) {
        ClearIk(At<uint8_t*>(ped, 0x16EC));
        MoveTarget_Set(At<uint8_t*>(ped, 0x16EC), zeroVector, 0.0f);
    }
    if (At<uint8_t*>(ped, 0x16E8)) {
        ClearIk(At<uint8_t*>(ped, 0x16E8));
        MoveTarget_Set(At<uint8_t*>(ped, 0x16E8), zeroVector, 0.0f);
    }
    if (At<uint32_t>(ped, 0x161C) != 0) {
        AnimManager_Detach(animManager, At<uint8_t*>(ped, 0x114), At<uint32_t>(ped, 0x161C));
        At<uint32_t>(ped, 0x161C) = 0;
    }
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        vc::Call<void>(struggleButton, 0x14, 0);
    }
    uint8_t* partner = At<uint8_t*>(ped, 0x155C);
    if (partner) {
        if (At<uint8_t*>(partner, 0x155C) == ped) {
            Ped_ClearPartner(partner);
        }
        Ped_ClearPartner(ped);
    }
    re::Ptr32<uint8_t>* prop = &At<uint8_t*>(ped, 0x548);
    if (*prop) {
        PropAnim_DetachPed(*prop, ped);
    }
    if (At<int32_t>(ped, 0x1418) == 0 && *prop) {
        Entity_CleanUpOldReference(*prop, prop);
        *prop = nullptr;
    }
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        re::Ptr32<uint8_t>* target = &At<uint8_t*>(ped, 0x438);
        if (*target) {
            Entity_CleanUpOldReference(*target, target);
            *target = nullptr;
        }
        At<uint32_t>(ped, 0x43C) = 0;
    }
    PedCombat_ReleaseSlots(ped + 0x668);
    if (ped == playerPed) {
        playerPed[0x1E91] = 0;
    }
    At<float>(ped, 0x1D58) = pedResetStep;
    ped[0x1BC] |= 2;
    At<int32_t>(ped, 0x1344) = 0;
    At<int32_t>(ped, 0x1348) = 0;
    At<int32_t>(ped, 0x133C) = 1;
    At<int32_t>(ped, 0x13A0) = 0;
    At<int32_t>(ped, 0x13A4) = 0;
    At<int32_t>(ped, 0x13C0) = 1;
    At<int32_t>(ped, 0x1438) = 0;
    At<int32_t>(ped, 0x1434) = 0;
    At<int32_t>(ped, 0x1450) = 1;
    At<int32_t>(ped, 0x131C) = 0;
    At<int32_t>(ped, 0x1320) = 0;
    At<int32_t>(ped, 0x1458) = 1;
    At<int32_t>(ped, 0x13CC) = 0;
    At<int32_t>(ped, 0x13E0) = 0;
    At<int32_t>(ped, 0x1468) = 0;
    At<int32_t>(ped, 0x1328) = 0;
    At<int32_t>(ped, 0x1414) = 0;
    At<int32_t>(ped, 0x13DC) = 0;
    At<int32_t>(ped, 0x1474) = 0;
    At<int32_t>(ped, 0x1484) = 0;
    At<int32_t>(ped, 0x1334) = 1;
    At<int32_t>(ped, 0x13E8) = 0;
    At<int32_t>(ped, 0x1520) = 0;
    At<int32_t>(ped, 0x1514) = 0;
    At<int32_t>(ped, 0x1530) = 0;
    if (At<int32_t>(ped, 0x1310) != 0xD || gameMode != 2) {
        At<int32_t>(ped, 0xD0) = 0;
    }
    At<uint8_t*>(ped, 0x1C8)[0x84] = 0;
    return 1;   // (eax is left 1)
}

// 0x616640 (thiscall): AddRadarBlipTrack::Start: a blip (+0x38) of type +0x24 (display
// +0x20, sprite +0x1C): on the ped (2, 6, 0xD), a route's point +0x30 (route +0x2C;
// 4, 5, 8..10) or a trigger's prop (+0x28; 7)
uint8_t __fastcall AddRadarBlipTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x34) = owner;
    At<int32_t>(self, 0x38) = -1;
    uint8_t* radar = hudRadar;
    switch (At<int32_t>(self, 0x24)) {
    case 2:
    case 6:
    case 0xD: {
        uint8_t* ped = OwnerPed(owner);
        if (!ped) {
            break;
        }
        uint8_t* pool = re::PtrCast<uint8_t*>(pedPool);
        int32_t index = int32_t(ped - At<uint8_t*>(pool, 0)) / At<int32_t>(pool, 0xC);
        int32_t handle = (index << 8) + At<uint8_t*>(pool, 4)[index];
        At<int32_t>(self, 0x38) = Radar_AddEntityBlip(radar, At<int32_t>(self, 0x24), handle, kBlipColour,
                                                      At<int32_t>(self, 0x20), At<int32_t>(self, 0x1C), 0);
        return 1;
    }
    case 4:
    case 5:
    case 8:
    case 9:
    case 10: {
        uint8_t* route = Routes_Find(routes, At<int32_t>(self, 0x2C));
        float* point = Route_First(route);
        for (int32_t i = 1; i < At<int32_t>(self, 0x30); i++) {
            float* next = Route_Next(route);
            if (!next) {
                break;
            }
            point = next;
        }
        At<int32_t>(self, 0x38) =
            Radar_AddBlip(radar, At<int32_t>(self, 0x24), point[0], point[1], point[2], kBlipColour,
                          At<int32_t>(self, 0x20), At<int32_t>(self, 0x1C), self[0x18]);
        return 1;
    }
    case 7: {
        int32_t index = Triggers_FindByName(triggers, At<const char*>(self, 0x28));
        if (index != -1) {
            int32_t ref = PropAnim_GetRef(Trigger_GetPropAnim(Triggers_Get(triggers, index)));
            if (ref >= 0) {
                uint32_t colour;
                Colour_FromUint(&colour, 0x0072BCFF);
                At<int32_t>(self, 0x38) = Radar_AddEntityBlip(radar, At<int32_t>(self, 0x24), ref, colour,
                                                              At<int32_t>(self, 0x20), At<int32_t>(self, 0x1C), 0);
            }
        }
        break;
    }
    default:
        break;
    }
    return 1;
}

// 0x6167F0 (thiscall): BreathTrack::Start: one breath (not kept running)
uint8_t __fastcall BreathTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x3C) = owner;
    Breath_Emit(self);
    return 0;
}

// 0x616810 (thiscall): FXWaterRipplesTrack::Update: its effect (+0x24, proxy +0x28)
// started once, then put on the water under the ped's bone (+0x1C) while that's under
// the water (else faded out)
uint8_t __fastcall FXWaterRipplesTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t started = 0;
    uint8_t* proxy = self + 0x28;
    if (At<uint32_t>(self, 0x2C) == 0) {
        started = 1;
        int32_t type = At<uint16_t>(self, 0x24);
        AE_Manager_Start(effectManager, &type, proxy, 0);
    }
    uint8_t shown = 0;
    if (At<uint32_t>(self, 0x2C) != 0) {
        const char* bone = At<const char*>(self, 0x1C);
        V3 pos = {0.0f, 0.0f, 0.0f};
        int32_t index = 0;
        if (bone && *bone != 0) {
            uint8_t* clump = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x114);
            if (!Skeleton_FindBone(At<uint8_t*>(clump, 0x94), HashName(bone), &index)) {
                return 0;
            }
            AnimManager_GetNodePosition(animManager, At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x114),
                                        reinterpret_cast<void*>(intptr_t(index)), &pos, 0);
            float z = pos.z;
            if (FXWaterRipplesTrack_WaterLevel(self) > z) {
                float z2 = pos.z;
                if (z2 - FXWaterRipplesTrack_WaterLevel(self) < 2.0f) {
                    pos.z = FXWaterRipplesTrack_WaterLevel(self);
                    shown = 1;
                    AE_Proxy_SetPosition(proxy, &pos.x, 0);
                    V3 up = {0.0f, 0.0f, 1.0f};
                    AE_Proxy_SetDirection(proxy, &up);
                }
            }
        }
    }
    if (started && !shown) {
        AE_Proxy_Kill(proxy);
        return 1;
    }
    AE_Proxy_Fade(proxy, shown ? 1.0f : 0.0f);
    return 1;
}

// 0x616990 (thiscall): FXWeaponImpactTrack::Update: its effect (+0x1C named, proxy
// +0x28) shown at the model's (+0x24, its +4) dummy +0x18 while something solid is
// within +0x20 above or below it
uint8_t __fastcall FXWeaponImpactTrack_Update(uint8_t* self, re::Edx, float) {
    V3 pos = {0.0f, 0.0f, 0.0f};
    void* a;
    void* b;
    if (!Anim_FindNode(&a, &b, At<const char*>(self, 0x18), At<uint8_t*>(At<uint8_t*>(self, 0x24), 4))) {
        return 1;
    }
    AnimManager_GetNodePosition(animManager, a, b, &pos, 0);
    V3 start = {pos.x, pos.y, pos.z - At<float>(self, 0x20)};
    V3 end = {pos.x, pos.y, pos.z + At<float>(self, 0x20)};
    ColPoint hit;
    hit.pos = {0.0f, 0.0f, 0.0f};
    hit.entity = nullptr;
    hit.normal = {0.0f, 0.0f, 0.0f};
    hit.depth = 3.40282347e+38f;
    re::Ptr32<uint8_t> entity = nullptr;
    uint8_t* proxy = self + 0x28;
    if (ProcessLineOfSight(&start, &end, &hit, &entity, 9, 0, 0)) {
        if (At<uint32_t>(self, 0x2C) == 0) {
            int32_t type = EffectLibrary_Find(effectLibrary, At<const char*>(self, 0x1C));
            AE_Manager_Start(effectManager, &type, proxy, 0);
        }
        AE_Proxy_Fade(proxy, 1.0f);
        AE_Proxy_SetPosition(proxy, &pos.x, 0);
    } else {
        AE_Proxy_Fade(proxy, 0.0f);
    }
    ReleaseColPoint(hit);
    return 1;
}

// the rail's side offset (the ped's right times 0x1B6503C, plus 0xB0B1A0) from pos
V3 RailOffset(uint8_t* ped) {
    V3 right;
    uint8_t* m = At<uint8_t*>(ped, 0x14);
    if (m) {
        right = {At<float>(m, 0), At<float>(m, 4), At<float>(m, 8)};
    } else {
        Placement_GetRight(ped + 4, &right);
    }
    float k = railOffset;
    return {k * right.x, k * right.y, k * right.z};
}

// 0x616B20 (thiscall): RailDriverTrack::Start: the ped faced along its matrix, stopped,
// its grind's rail (+0x20 the ped's +0x1E9C, +0x24 its prop +0x1E98) and start (+0x34)
// kept, and put up onto the rail (+0x40, blended in by +0x30)
uint8_t __fastcall RailDriverTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    self[0x28] = 0;
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        uint8_t* m = Ped_GetMatrix(ped);
        V3 f = {At<float>(m, 0x10), At<float>(m, 0x14), At<float>(m, 0x18)};
        Vector_Normalise(&f);
        f.z = 0.0f;
        float heading = HeadingFromVector(&f);
        Ped_SetHeading(ped, heading);
        if (At<uint8_t*>(ped, 0x14)) {
            Matrix_SetRotateZOnly(At<uint8_t*>(ped, 0x14), heading);
        }
        At<float>(ped, 0x10) = heading;
        At<float>(ped, 0x124) = 0.0f;
        At<float>(ped, 0x128) = 0.0f;
        At<float>(ped, 0x130) = 0.0f;
        At<float>(ped, 0x12C) = 0.0f;
        At<float>(ped, 0x134) = 0.0f;
        At<float>(ped, 0x138) = 0.0f;
        At<uint32_t>(self, 0x20) = At<uint32_t>(ped, 0x1E9C);
        At<uint32_t>(self, 0x24) = At<uint32_t>(ped, 0x1E98);
        const float* pos = Pos(ped);
        At<float>(self, 0x34) = pos[0];
        At<float>(self, 0x38) = pos[1];
        At<float>(self, 0x3C) = pos[2];
        uint8_t* m2 = Ped_GetMatrix(ped);
        V3 at = {At<float>(m2, 0x30), At<float>(m2, 0x34), At<float>(m2, 0x38)};
        V3 side = RailOffset(ped);
        V3 t = {at.x + side.x, at.y + side.y, at.z + side.z};
        V3 up = {t.x + railLift[0], t.y + railLift[1], t.z + railLift[2]};
        At<float>(self, 0x40) = up.x;
        At<float>(self, 0x44) = up.y;
        At<float>(self, 0x48) = up.z;
        At<float>(self, 0x30) = 0.0f;
        Placeable_SetPosition(ped, At<float>(self, 0x40), At<float>(self, 0x44), At<float>(self, 0x48));
        At<int32_t>(self, 0x2C) = 1;
        UpdateFrame(ped);
    }
    return 0;
}

// 0x616D20 / 0x616DE0 (thiscall): the ped raised onto / lowered off the rail (+0x28)
void __fastcall RailDriverTrack_RaiseImpl(uint8_t* self, re::Edx, uint8_t* ped) {
    if (self[0x28] != 0) {
        return;
    }
    V3 side = RailOffset(ped);
    const float* pos = Pos(ped);
    V3 t = {pos[0] + side.x, pos[1] + side.y, pos[2] + side.z};
    V3 p = {t.x + railLift[0], t.y + railLift[1], t.z + railLift[2]};
    Placeable_SetPosition(ped, p.x, p.y, p.z);
    self[0x28] = 1;
}
void __fastcall RailDriverTrack_LowerImpl(uint8_t* self, re::Edx, uint8_t* ped) {
    if (self[0x28] == 0) {
        return;
    }
    V3 side = RailOffset(ped);
    const float* pos = Pos(ped);
    V3 t = {pos[0] - side.x, pos[1] - side.y, pos[2] - side.z};
    V3 p = {t.x - railLift[0], t.y - railLift[1], t.z - railLift[2]};
    Placeable_SetPosition(ped, p.x, p.y, p.z);
    self[0x28] = 0;
}

float Length(const V3& v) {
    return std::sqrt(std::fabs(v.x * v.x + v.y * v.y + v.z * v.z));
}

// 0x617760 (thiscall): RailDriverTrack's move along the rail from point by distance:
// faced along the segment, onto the next ones as far as it reaches (recursively); 0 at
// the rail's end
uint8_t __fastcall RailDriverTrack_AdvanceImpl(uint8_t* self, re::Edx, uint8_t* point, float distance) {
    At<uint8_t*>(self, 0x20) = point;
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    uint8_t* list = PropAnim_GetRailList(At<uint8_t*>(self, 0x24));
    uint8_t* next = RailList_NextAfter(list, point);
    if (!next) {
        return 0;
    }
    V3 dir;
    VectorSub(&dir, RailPoint_GetPos(next), RailPoint_GetPos(point));
    Vector_Normalise(&dir);
    SetPedHeading(ped, HeadingFromVector(&dir));
    uint8_t result = 1;
    RailDriverTrack_Lower(self, ped);
    V3 d;
    const float* at = Pos(ped);
    VectorSub(&d, at, RailPoint_GetPos(next));
    float toNext = Length(d);
    at = Pos(ped);
    VectorSub(&d, at, RailPoint_GetPos(point));
    float fromPointSq = std::fabs(d.x * d.x + d.y * d.y + d.z * d.z);
    float* nextPos = RailPoint_GetPos(next);
    VectorSub(&d, RailPoint_GetPos(point), nextPos);
    float fromPoint = std::sqrt(fromPointSq);
    float segment = Length(d);
    if (fromPoint > segment) {
        if (!RailList_NextAfter(list, next)) {
            return 0;
        }
    }
    if (toNext < distance) {
        float* p = RailPoint_GetPos(next);
        Placeable_SetPosition(ped, p[0], p[1], p[2]);
        result = RailDriverTrack_Advance(self, next, distance - toNext);
    } else {
        at = Pos(ped);
        VectorSub(&d, at, RailPoint_GetPos(point));
        float along = Length(d) + distance;
        V3 step = {dir.x * along, dir.y * along, dir.z * along};
        float* p = RailPoint_GetPos(point);
        V3 np = {p[0] + step.x, p[1] + step.y, p[2] + step.z};
        Placeable_SetPosition(ped, np.x, np.y, np.z);
    }
    RailDriverTrack_Raise(self, ped);
    At<float>(ped, 0x124) = 0.0f;
    At<float>(ped, 0x128) = 0.0f;
    At<float>(ped, 0x130) = 0.0f;
    At<float>(ped, 0x12C) = 0.0f;
    At<float>(ped, 0x134) = 0.0f;
    At<float>(ped, 0x138) = 0.0f;
    UpdateFrame(ped);
    return result;
}

// 0x617B00 (thiscall): RailDriverTrack::Update: up onto the rail first (+0x30 at 5.4 a
// second from +0x34 to +0x40), then along it at +0x18
uint8_t __fastcall RailDriverTrack_Update(uint8_t* self, re::Edx, float dt) {
    if (At<uint32_t>(self, 0x24) == 0) {
        return 0;
    }
    if (At<int32_t>(self, 0x2C) != 0) {
        return RailDriverTrack_Advance(self, At<uint8_t*>(self, 0x20), At<float>(self, 0x18) * dt);
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    float t = dt * 5.4f + At<float>(self, 0x30);
    At<float>(self, 0x30) = t;
    if (!(t < 1.0f)) {
        t = 1.0f;
    }
    At<float>(self, 0x30) = t;
    V3 d;
    VectorSub(&d, &At<float>(self, 0x40), &At<float>(self, 0x34));
    float f = At<float>(self, 0x30);
    V3 s = {f * d.x, d.y * f, f * d.z};
    const float* from = &At<float>(self, 0x34);
    V3 p = {from[0] + s.x, from[1] + s.y, from[2] + s.z};
    Placeable_SetPosition(ped, p.x, p.y, p.z);
    UpdateFrame(ped);
    if (1.0f == At<float>(self, 0x30)) {
        At<int32_t>(self, 0x2C) = 1;
    }
    return 1;
}

// 0x616EA0 (thiscall): SetPosRotTrack::Start: the ped put at +0x18 facing +0x24 degrees
uint8_t __fastcall SetPosRotTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    uint8_t* ped = OwnerPed(owner);
    Placeable_SetPosition(ped, At<float>(self, 0x18), At<float>(self, 0x1C), At<float>(self, 0x20));
    Ped_SetHeadingNow(OwnerPed(At<uint8_t*>(self, 0x28)), At<float>(self, 0x24) * kPi / 180.0f);
    UpdateFrame(OwnerPed(At<uint8_t*>(self, 0x28)));
    return 0;
}

bool IsBeamingPlayer(uint8_t* ped) {
    uint8_t* player = playerPed;
    return ped && player && ped == player && At<int32_t>(player, 0x1548) != 0;
}

// 0x616F40 (thiscall): BeamMoveToSafePosTrack::Start: the player (while beamable,
// +0x1548) moved from where it is (+0x28) to its safe position (+0x1EA0, kept at +0x34):
// at once with +0x18, else over +0x20 seconds from +0x1C
uint8_t __fastcall BeamMoveToSafePosTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* player = playerPed;
    if (!IsBeamingPlayer(ped)) {
        return 0;
    }
    const float* pos = Pos(ped);
    At<float>(self, 0x28) = pos[0];
    At<float>(self, 0x2C) = pos[1];
    At<float>(self, 0x30) = pos[2];
    At<uint32_t>(self, 0x34) = At<uint32_t>(player, 0x1EA0);
    At<uint32_t>(self, 0x38) = At<uint32_t>(player, 0x1EA4);
    At<float>(self, 0x40) = 0.0f;
    At<uint32_t>(self, 0x3C) = At<uint32_t>(player, 0x1EA8);
    if (0.0f > At<float>(self, 0x20)) {
        At<float>(self, 0x20) = 0.0f;
    }
    if (self[0x18] != 0) {
        Ped_SetPosition(ped, reinterpret_cast<V3*>(&At<float>(self, 0x34)));
    }
    return 1;
}

// 0x616FE0 (thiscall): BeamMoveToSafePosTrack::Update: moved along (+0x40 the time in)
uint8_t __fastcall BeamMoveToSafePosTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    if (!IsBeamingPlayer(ped)) {
        return 0;
    }
    if (self[0x18] != 0) {
        Placeable_SetPosition(ped, At<float>(self, 0x34), At<float>(self, 0x38), At<float>(self, 0x3C));
        return 1;
    }
    float time = At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 0), 8);
    float from = At<float>(self, 0x1C);
    if (!(0.0f < from || 0.0f == from)) {
        return 1;
    }
    if (!(from < time)) {
        return 1;
    }
    if (!(At<float>(self, 0x20) + At<float>(self, 0x1C) >= time)) {
        return 1;
    }
    float in = time - At<float>(self, 0x1C);
    At<float>(self, 0x40) = in;
    float until = At<float>(self, 0x20) + At<float>(self, 0x1C);
    float t = Clamp(LinearMap(in, 0.0f, At<float>(self, 0x1C), 1.0f, until), 0.0f, 1.0f);
    V3 d;
    VectorSub(&d, &At<float>(self, 0x34), &At<float>(self, 0x28));
    V3 s = {d.x * t, d.y * t, d.z * t};
    const float* start = &At<float>(self, 0x28);
    At<float>(self, 0x44) = s.x + start[0];
    At<float>(self, 0x48) = s.y + start[1];
    At<float>(self, 0x4C) = s.z + start[2];
    Ped_SetPosition(ped, reinterpret_cast<V3*>(&At<float>(self, 0x44)));
    return 1;
}

// 0x617170 (thiscall): BeamMoveToSafePosTrack::End: put at the safe position
uint8_t __fastcall BeamMoveToSafePosTrack_End(uint8_t* self, re::Edx) {
    Placeable_SetPosition(OwnerPed(At<uint8_t*>(self, 0x24)), At<float>(self, 0x34), At<float>(self, 0x38),
                          At<float>(self, 0x3C));
    return 1;
}

// 0x6171A0 (thiscall): HitAnimationTrack::Update: the animation, its speed (blend out
// rate times the ped's animation speed stat), and the entity slid the rest of the hit's
// distance (+0x7C at +0x74 a second; facing the hit)
uint8_t __fastcall HitAnimationTrack_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t running = AnimationTrack_Update(self, dt);
    if (!running) {
        return running;
    }
    uint8_t* anim = Blend_GetAnim(At<uint8_t*>(self, 0x4C), At<int32_t>(self, 0x24));
    if (!anim) {
        return 0;
    }
    float rate = vc::Call<float>(self, 0x18);
    float speed = 1.0f;
    if (OwnerPed(At<uint8_t*>(self, 0x40))) {
        speed = float(Ped_GetStat(OwnerPed(At<uint8_t*>(self, 0x40)), 0x14)) / 100.0f;
    }
    AnimManager_SetSpeed(animManager, At<uint8_t*>(self, 0x4C), anim, 0, speed * rate);
    if (!(0.0f < At<float>(self, 0x7C))) {
        return running;
    }
    if (std::fabs(At<float>(self, 0x7C)) <= kEpsilon) {
        return running;
    }
    uint8_t* owner = At<uint8_t*>(self, 0x40);
    uint8_t* entity = nullptr;
    if (At<uint8_t*>(owner, 8)) {
        entity = At<uint8_t*>(owner, 8);
    } else if (At<uint8_t*>(owner, 0xC)) {
        entity = At<uint8_t*>(owner, 0xC);
    }
    V3 fwd;
    Placeable_GetForward(entity, &fwd);
    const float* dir = &At<float>(At<uint8_t*>(owner, 0x10), 0x20);
    float dot = dir[1] * fwd.y + fwd.x * dir[0] + dir[2] * fwd.z;
    float step = dt * At<float>(self, 0x74);
    if (NearlyEqual(At<float>(self, 0x78), 0.0f, kEpsilon)) {
        step = At<float>(self, 0x7C) * At<float>(self, 0x74);
    }
    float size = std::fabs(step);
    if (At<float>(self, 0x7C) < size || NearlyEqual(At<float>(self, 0x7C), size, kEpsilon)) {
        // the last of it
        step = 0.0f > step ? -At<float>(self, 0x7C) : At<float>(self, 0x7C);
        At<float>(self, 0x7C) = 0.0f;
    } else {
        At<float>(self, 0x7C) = At<float>(self, 0x7C) - size;
    }
    if (0.0f > dot) {
        step = -step;
    }
    V3 move = {fwd.x * step, fwd.y * step, step * fwd.z};
    const float* at = Pos(entity);
    V3 p = {at[0] + move.x, at[1] + move.y, at[2] + move.z};
    Placeable_SetPosition(entity, p.x, p.y, p.z);
    UpdateFrame(entity);
    return running;
}

// 0x617460 (thiscall): TargetFaceTrack::Update: the player's free aim while it isn't
// locked on (within +0x18..+0x24 or with +0x30 / +0x31: aimed along the stick, within
// +0x28 degrees of facing), a new target when outside its window, turning to it
// within the window (+0x33 into ped +0x1DE5 meanwhile)
uint8_t __fastcall TargetFaceTrack_Update(uint8_t* self, re::Edx, float dt) {
    self[0x48] = 0;
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    if (At<int32_t>(ped, 0x1310) == 0xD && (gameMode == 2 || gameMode == 0xD)) {
        return 1;
    }
    ped[0x1DE5] = self[0x33];
    float time = At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 0), 8);
    uint8_t aimed = 0;
    uint8_t lockOn = 0;
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        float button = Pad_GetValue(10, 0);
        if (!(0.0f == button)) {
            lockOn = 1;
        } else {
            bool aim;
            if (self[0x31] != 0) {
                aim = At<float>(self, 0x18) < 0.0f || time < At<float>(self, 0x18);
            } else {
                aim = self[0x30] != 0 && !(At<float>(self, 0x24) < 0.0f) && At<float>(self, 0x24) < time;
            }
            if (aim) {
                V3 fwd;
                Placeable_GetForward(ped, &fwd);
                float* in = At<float*>(ped, 0x2E0);
                V3 stick = {in[0], in[1], in[2]};
                float facing = AngleOfVector2D(&fwd);
                float wanted = AngleOfVector2D(&stick);
                float limit = At<float>(self, 0x28) * 0.5f;
                limit = limit * kPi / 180.0f;
                float d = AngleDiff(facing, wanted);
                if (-limit > d) {
                    d = -limit;
                } else if (d > limit) {
                    d = limit;
                }
                float a = d + facing - 1.57079637f;
                float c = trk::CrtCos(a);
                float s = trk::CrtSin(a);
                Targeting_SetAim(At<uint8_t*>(ped, 0x1C8), V3{c, s, 0.0f}, 0);
                self[0x48] = 1;
                aimed = 1;
            }
        }
    }
    float from = At<float>(self, 0x18);
    float to = At<float>(self, 0x24);
    bool retarget = from < 0.0f || from >= time || (to >= 0.0f && to < time) || lockOn;
    if (retarget) {
        if (aimed) {
            Targeting_Update(At<uint8_t*>(ped, 0x1C8), 1);
        }
        TargetFaceTrack_PickTarget(self);
    }
    from = At<float>(self, 0x18);
    to = At<float>(self, 0x24);
    if ((0.0f < from || 0.0f == from) && (from < time || from == time)) {
        if (to >= time || 0.0f > to) {
            TargetFaceTrack_Turn(self, dt);
            ped[0x1DE5] = 0;
            return 1;
        }
    }
    ped[0x1DE5] = 0;
    return 1;
}

} // namespace

#define TRSYNC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TRSYNC_REVERSED("TargetFaceTrack::PickTarget", 0x006158A0, &TargetFaceTrack_PickTargetImpl);
TRSYNC_REVERSED("SyncTrack::Align", 0x00615960, &SyncTrack_AlignImpl);
TRSYNC_REVERSED("SyncTrack::Start", 0x00616060, &SyncTrack_Start);
TRSYNC_REVERSED("SyncTrack::Update", 0x006162D0, &SyncTrack_Update);
TRSYNC_REVERSED("AttachIKTrack::Start", 0x00616350, &AttachIKTrack_Start);
TRSYNC_REVERSED("DetachIKTrack::Start", 0x006163E0, &DetachIKTrack_Start);
TRSYNC_REVERSED("PedResetTrack::Start", 0x00616430, &PedResetTrack_Start);
TRSYNC_REVERSED("AddRadarBlipTrack::Start", 0x00616640, &AddRadarBlipTrack_Start);
TRSYNC_REVERSED("BreathTrack::Start", 0x006167F0, &BreathTrack_Start);
TRSYNC_REVERSED("FXWaterRipplesTrack::Update", 0x00616810, &FXWaterRipplesTrack_Update);
TRSYNC_REVERSED("FXWeaponImpactTrack::Update", 0x00616990, &FXWeaponImpactTrack_Update);
TRSYNC_REVERSED("RailDriverTrack::Start", 0x00616B20, &RailDriverTrack_Start);
TRSYNC_REVERSED("RailDriverTrack::Raise", 0x00616D20, &RailDriverTrack_RaiseImpl);
TRSYNC_REVERSED("RailDriverTrack::Lower", 0x00616DE0, &RailDriverTrack_LowerImpl);
TRSYNC_REVERSED("SetPosRotTrack::Start", 0x00616EA0, &SetPosRotTrack_Start);
TRSYNC_REVERSED("BeamMoveToSafePosTrack::Start", 0x00616F40, &BeamMoveToSafePosTrack_Start);
TRSYNC_REVERSED("BeamMoveToSafePosTrack::Update", 0x00616FE0, &BeamMoveToSafePosTrack_Update);
TRSYNC_REVERSED("BeamMoveToSafePosTrack::End", 0x00617170, &BeamMoveToSafePosTrack_End);
TRSYNC_REVERSED("HitAnimationTrack::Update", 0x006171A0, &HitAnimationTrack_Update);
TRSYNC_REVERSED("TargetFaceTrack::Update", 0x00617460, &TargetFaceTrack_Update);
TRSYNC_REVERSED("RailDriverTrack::Advance", 0x00617760, &RailDriverTrack_AdvanceImpl);
TRSYNC_REVERSED("RailDriverTrack::Update", 0x00617B00, &RailDriverTrack_Update);
