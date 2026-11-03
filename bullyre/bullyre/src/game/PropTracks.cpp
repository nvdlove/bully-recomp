// The action tree's prop tracks of 0x655660..0x656450: animations played on the owner's
// prop (optionally synced to the sound clock), its effects, flags, tagging, geometry and
// collision, attaching / detaching prop animations; their destructors and field defaults
// (the New functions call these after construction).
//
// A track: +0 vtable, +4 link, +8 keeps alive, +0xC start, +0x10 end, +0x14 key
// (ActionTree.h); vf2 Start(owner), vf3 Update(dt), vf4 End(), vf5 Resolve().

#include <cstdint>

#include "game/ActionFactories.h"
#include "game/PropTracks.h"

using namespace actc;
using proptr::NodeTransform;
using proptr::GetNodeTransform;
using proptr::UsePoint_GetEntity;
namespace af = actfac;

namespace {

GAME_FN(void, __cdecl, Anim_SyncBegin_6C3B90, 0x006C3B90);
GAME_FN(void, __cdecl, Anim_SyncEnd_6C3BD0, 0x006C3BD0);
GAME_FN(int32_t, __thiscall, Clock_GetMilliseconds, 0x00595B30, void* clock);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, uint8_t* blend, uint32_t name);
GAME_FN(int32_t, __thiscall, Channel_IsIdle, 0x0058E060, void* channel);
GAME_FN(float, __thiscall, Anim_GetDuration_5F7110, 0x005F7110, uint8_t* anim, int i);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C0E90, void* manager, uint8_t* blend, uint8_t* anim, int a, float time,
        int b, int c);
GAME_FN(void, __thiscall, AnimManager_Update, 0x006C1910, void* manager, uint8_t* blend, float dt, int visible, uintptr_t a);
GAME_FN(uint8_t, __thiscall, AnimationTrack_Update, 0x0060AF60, uint8_t* self, float dt);
GAME_FN(uint8_t, __thiscall, AnimationTrack_End, 0x0060B0E0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, AnimationParametricTrack_Update, 0x00608140, uint8_t* self, float dt);
GAME_FN(void, __thiscall, PropAnim_SetUnknown254, 0x004EA2A0, uint8_t* prop, float value);
GAME_FN(void, __thiscall, PropAnim_SetFlag, 0x004EA450, uint8_t* prop, int index, int value);
GAME_FN(void, __thiscall, PropAnim_SetInList, 0x004EA430, uint8_t* prop, int add);
GAME_FN(void, __thiscall, PropAnim_AttachPed, 0x004ED830, uint8_t* prop, uint8_t* ped);
GAME_FN(void, __thiscall, PropAnim_RemoveChild, 0x004EE610, uint8_t* prop, int animId);
GAME_FN(void, __thiscall, AreaTransition_SetTarget, 0x004152B0, void* transition, void* target);
GAME_FN(uint16_t, __thiscall, EffectInfos_FindId, 0x006679A0, void* infos, uint32_t name);
GAME_FN(void, __thiscall, AE_Manager_Start, 0x00668290, void* manager, const int32_t* type, uint8_t* proxy, int);
GAME_FN(void, __thiscall, AE_Proxy_SetTransform, 0x00668EC0, uint8_t* proxy, const NodeTransform* transform);
GAME_FN(void, __thiscall, AE_Proxy_SetPosition, 0x00669040, uint8_t* proxy, const float* pos, int);
GAME_FN(void, __thiscall, AE_Proxy_Kill, 0x00668BA0, uint8_t* proxy);
GAME_FN(void, __thiscall, Ctl_PlayNode, 0x005F45F0, uint8_t* controller, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Node_Get0C, 0x005F4F90, uint8_t* reference, int32_t key);
GAME_FN(int, __cdecl, GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(uint8_t*, __cdecl, FindNearestObjectOfTypeInRange, 0x004607A0, int model, const float* pos, float range,
        uint32_t lists);
GAME_FN(void, __cdecl, SetNearestObjectOfTypeFlags, 0x0045E060, int model, const V3* pos, int hide, int enable);
GAME_FN(void, __thiscall, NavObstacleList_Remove, 0x005E6990, void* list, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, BaseCollisionTrack_Start, 0x005F95A0, uint8_t* track, uint8_t* owner);
GAME_FN(uint8_t, __thiscall, Track_TestPeds, 0x005F9B00, uint8_t* self, uint8_t* list, const V3* losStart,
        const V3* pos, float radius, float margin);
GAME_FN(uint8_t, __thiscall, Track_TestObjectSectors, 0x005F9920, uint8_t* self, const V3* losStart, const V3* pos,
        float radius, float margin);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(int, __cdecl, Prop_FindBone, 0x00655620, uint8_t* entity, int id);

GAME_VAR(uint8_t, animSyncEnabled, 0x020C5BE9);
GAME_VAR(void*, soundChannel_CF4290, 0x00CF4290);
GAME_VAR(void*, effectLibrary, 0x00B6F1E8);
GAME_VAR(void*, effectManager, 0x00B6F1D4);
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);   // CWorld::ms_nCurrentScanCode
inline void* const soundClock = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const areaTransition = reinterpret_cast<void*>(0x00BCC580);
inline void* const navObstacles = reinterpret_cast<void*>(0x00D12AF8);

// Prop animation tracks (AnimationTrack's fields: +0x20 the mode (2 loops), +0x24 the
// anim's name, +0x30 its speed, +0x40 the owner, +0x48, +0x4C the prop's blend, +0x50 the
// prop's flag at the end); +Synced set: played at the sound clock's time instead of the
// track's own (Speed: times the speed)

// Update (vf3, thiscall): synced, the anim is placed at the clock's time and the track
// runs on while it hasn't reached the end (looping: while the channel plays)
template <int Synced, bool Speed>
uint8_t __fastcall SyncedAnimation_Update(uint8_t* self, re::Edx, float dt) {
    if (self[Synced] == 0) {
        if (At<int32_t>(self, 0x48) == -1 && At<int32_t>(self, 0x20) == 2) {
            return 1;
        }
        return AnimationTrack_Update(self, dt);
    }
    if (animSyncEnabled == 0) {
        return 1;
    }
    Anim_SyncBegin_6C3B90();
    vc::Call<void>(self, acttree::kTrackResolve);
    uint8_t* blend = At<uint8_t*>(self, 0x4C);
    if (!blend || !At<void*>(blend, 0x94)) {
        return 0;
    }
    float speed = At<float>(self, 0x30);
    uint32_t ms = uint32_t(Clock_GetMilliseconds(soundClock));
    float t = float(double(ms) / 1000.0);
    if constexpr (Speed) {
        t = t * speed;
    }
    uint8_t* anim = Blend_GetAnim(blend, At<uint32_t>(self, 0x24));
    uint8_t done;
    if (At<int32_t>(self, 0x20) == 2) {
        done = Channel_IsIdle(soundChannel_CF4290) != 0 ? 1 : 0;
        float duration = Anim_GetDuration_5F7110(anim, 0);
        while (t > duration) {
            t = t - Anim_GetDuration_5F7110(anim, 0);
        }
    } else {
        float duration = Anim_GetDuration_5F7110(anim, 0);
        done = t >= duration ? 1 : 0;
    }
    AnimManager_Play(animManager, At<uint8_t*>(self, 0x4C), anim, 0, t, 1, 1);
    AnimManager_Update(animManager, At<uint8_t*>(self, 0x4C), 0.0f, 1, 0);
    return done == 0 ? 1 : 0;
}

// End (vf4, thiscall): the prop's flag (+0xE4) set back, the sync ended
template <int Synced>
uint8_t __fastcall SyncedAnimation_End(uint8_t* self, re::Edx) {
    uint8_t* prop = UsePoint_GetEntity(At<uint8_t*>(self, 0x40));
    if (prop) {
        At<uint32_t>(prop, 0xE4) = self[0x50];
    }
    if (self[Synced] != 0) {
        Anim_SyncEnd_6C3BD0();
    }
    return AnimationTrack_End(self);
}

// 0x6556D0 (thiscall): PropAnimationTrack::Resolve: the blend is the prop's (+0x114)
uint8_t* __fastcall PropAnimation_Resolve(uint8_t* self, re::Edx) {
    uint8_t* prop = UsePoint_GetEntity(At<uint8_t*>(self, 0x40));
    if (!prop) {
        return nullptr;
    }
    uint8_t* owner = At<uint8_t*>(self, 0x40);
    if (!owner) {
        return nullptr;
    }
    prop = UsePoint_GetEntity(owner);
    At<uint8_t*>(self, 0x4C) = At<uint8_t*>(prop, 0x114);
    return prop;
}

// 0x6558E0 (thiscall): PropAnimationParametricTrack::Resolve: the same, the prop not
// tested
uint8_t* __fastcall PropAnimationParametric_Resolve(uint8_t* self, re::Edx) {
    uint8_t* owner = At<uint8_t*>(self, 0x40);
    if (!owner) {
        return nullptr;
    }
    uint8_t* blend = At<uint8_t*>(UsePoint_GetEntity(owner), 0x114);
    At<uint8_t*>(self, 0x4C) = blend;
    return blend;
}

// 0x655900 (thiscall): PropAnimationParametricTrack::Update: the base's
uint8_t __fastcall PropAnimationParametric_Update(uint8_t* self, re::Edx, float dt) {
    return AnimationParametricTrack_Update(self, dt);
}

// 0x655950 (thiscall): PropEffectTrack::End: its effect (+0x28) killed
uint8_t __fastcall PropEffect_End(uint8_t* self, re::Edx) {
    AE_Proxy_Kill(self + 0x28);
    return 0;
}

// 0x655AF0 (thiscall): PropCollisionTrack's test at pos: the peds and the objects hit
// (a new scan code first)
uint8_t __fastcall PropCollision_Test(uint8_t* self, re::Edx, const V3* pos, float radius, float margin) {
    if (currentScanCode >= 0xFFFF) {
        ClearScanCodes();
        currentScanCode = 1;
    } else {
        currentScanCode = uint16_t(currentScanCode + 1);
    }
    uint8_t peds = Track_TestPeds(self, nullptr, nullptr, pos, radius, margin);
    uint8_t objects = Track_TestObjectSectors(self, nullptr, pos, radius, margin);
    return uint8_t(objects | peds);
}

// 0x655B80 (thiscall, under 5 bytes): the field at +0x30
uint8_t* __fastcall Field30(uint8_t* self, re::Edx) {
    return self + 0x30;
}
RE_UNHOOKED_FN("PropAnim::Field30_655B80", 0x00655B80, &Field30);

// 0x655BB0 (thiscall): PropTagEffectTrack::Start: the prop's tagged fraction (+0x254) set
// by the mode (+0x18: 0 / 3 all, 1 / 2 none, 4 / 5 blending to all / none over the track:
// +0x20 the fraction, +0x1C its rate)
uint8_t __fastcall PropTagEffect_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (!prop) {
        return 1;
    }
    switch (At<uint32_t>(self, 0x18)) {
    case 0:
    case 3:
        PropAnim_SetUnknown254(prop, 1.0f);
        At<float>(self, 0x20) = 1.0f;
        At<float>(self, 0x1C) = 0.0f;
        break;
    case 1:
    case 2:
        PropAnim_SetUnknown254(prop, 0.0f);
        At<float>(self, 0x20) = 0.0f;
        At<float>(self, 0x1C) = 0.0f;
        break;
    case 4: {
        float v = At<float>(prop, 0x254);
        At<float>(self, 0x20) = v;
        float duration = acttree::TrackEnd(self) - acttree::TrackStart(self);
        At<float>(self, 0x1C) = (1.0f - v) / duration;
        break;
    }
    case 5: {
        float v = 1.0f - At<float>(prop, 0x254);
        At<float>(self, 0x20) = v;
        float duration = acttree::TrackEnd(self) - acttree::TrackStart(self);
        At<float>(self, 0x1C) = (1.0f - v) / duration;
        break;
    }
    default:
        break;
    }
    return 1;
}

// 0x655CB0 (thiscall): PropFlagsTrack::Start: the prop's flag +0x1C set to +0x18
uint8_t __fastcall PropFlags_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (prop) {
        PropAnim_SetFlag(prop, At<int32_t>(self, 0x1C), self[0x18]);
    }
    return 1;
}

// 0x655D20 (thiscall): PropAlterObstacleTrack::Start: the prop in or out of the obstacle
// list (+0x18)
uint8_t __fastcall PropAlterObstacle_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (prop) {
        PropAnim_SetInList(prop, self[0x18]);
    }
    return 1;
}

// 0x655DA0 (thiscall): PropAttemptDoorTransitionTrack::Start: a door with a transition
// (+0x194) sends the area transition to its target (+0x23C)
uint8_t __fastcall PropAttemptDoorTransition_Start(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (prop && At<void*>(prop, 0x194)) {
        AreaTransition_SetTarget(areaTransition, At<void*>(prop, 0x23C));
    }
    return 0;
}

// 0x655DD0 (thiscall): PropAttachPropAnimationTrack's field defaults
void __fastcall PropAttachPropAnimation_InitFields(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00607660)(self);   // AnimationTrack's
    self[8] = 1;
    proptr::SetNoneRaw(self, 0x58);
    ClearName(self, 0x5C);
    self[0x60] = 0;
    self[0x3C] = 0;
    At<int32_t>(self, 0x68) = -1;
}

// 0x656000 (thiscall): PropAnimAttachToPropAnimTrack's field defaults
void __fastcall PropAnimAttachToPropAnim_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    proptr::SetNoneRaw(self, 0x18);
    proptr::SetNoneRaw(self, 0x1C);
    proptr::SetNoneRaw(self, 0x20);
    At<int32_t>(self, 0x28) = 0;
    At<int32_t>(self, 0x2C) = -1;
    At<int32_t>(self, 0x30) = -1;
}

// 0x6560A0 (thiscall): PropAnimRemoveAttachmentTrack's field defaults
void __fastcall PropAnimRemoveAttachment_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    proptr::SetNoneRaw(self, 0x18);
    At<int32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x24) = -1;
}

// 0x656100 (thiscall): PropAnimRemoveAttachmentTrack::Start: the prop's child animation
// on the bone +0x18 (its index +0x24, 0 when none) removed
uint8_t __fastcall PropAnimRemoveAttachment_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (!prop) {
        return 0;
    }
    uint8_t* anim = At<uint8_t*>(prop, 0x114);
    if (!anim || !At<void*>(anim, 0x94)) {
        return 0;
    }
    int index = Prop_FindBone(prop, At<int32_t>(self, 0x18));
    At<int32_t>(self, 0x24) = index;
    if (index == -1) {
        At<int32_t>(self, 0x24) = 0;
    }
    At<uint8_t*>(self, 0x20) = At<uint8_t*>(prop, 0x114);
    PropAnim_RemoveChild(prop, At<int32_t>(self, 0x24));
    return 1;
}

// 0x656180 (thiscall): PropEffectTrack::Start: its effect (+0x1C) started on the prop's
// bone (+0x18), or at the prop when the bone has no transform (the prop not tested)
uint8_t __fastcall PropEffect_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* prop = UsePoint_GetEntity(owner);
    int32_t type = EffectInfos_FindId(effectLibrary, At<uint32_t>(self, 0x1C));
    uint8_t* proxy = self + 0x28;
    AE_Manager_Start(effectManager, &type, proxy, 0);
    NodeTransform transform;
    transform.scale = 1.0f;
    if (GetNodeTransform(prop, At<int32_t>(self, 0x18), &transform, 1)) {
        AE_Proxy_SetTransform(proxy, &transform);
        return 0;
    }
    AE_Proxy_SetPosition(proxy, Pos(prop), 0);
    return 0;
}

// 0x656220 (thiscall): PropEffectTrack::Update: following the bone (+0x20 set)
uint8_t __fastcall PropEffect_Update(uint8_t* self, re::Edx, float) {
    if (self[0x20] != 0) {
        uint8_t* prop = UsePoint_GetEntity(At<uint8_t*>(self, 0x24));
        if (!prop) {
            return 0;
        }
        NodeTransform transform;
        transform.scale = 1.0f;
        if (GetNodeTransform(prop, At<int32_t>(self, 0x18), &transform, 1)) {
            AE_Proxy_SetTransform(self + 0x28, &transform);
        }
    }
    return 1;
}

// 0x656280 (thiscall): PropInteractTrack::Start: the ped's prop in use (+0x438, an
// object) with an action node (+0x230): its flag 10 cleared, the node's playable played
// on the ped's controller and the ped attached to it
uint8_t __fastcall PropInteract_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* prop = At<uint8_t*>(ped, 0x438);
    if (prop && (prop[0x108] & 7) == 6) {
        uint8_t* node = At<uint8_t*>(prop, 0x230);
        if (node) {
            PropAnim_SetFlag(prop, 0xA, 0);
            uint8_t* playable = vc::Call<uint8_t*>(node, acttree::kNodeFindPlayable, At<uint8_t*>(self, 0x18));
            Ctl_PlayNode(At<uint8_t*>(ped, 0x2E0) + 0x258, playable);
            PropAnim_AttachPed(prop, ped);
        }
    }
    return 0;
}

// 0x656300 (thiscall): PropSetPedActionTrack::Start: the owner's prop (+0xC) given the
// node (+0x18) as its ped action (+0x230)
uint8_t __fastcall PropSetPedAction_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    if (At<uint8_t*>(owner, 0xC)) {
        uint8_t* node = Node_Get0C(self + 0x18, -1);
        At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 0xC), 0x230) = node;
    }
    return 0;
}

// 0x656330 (thiscall): PropGeometryTrack::Start: the objects of the model (+0x2C) nearest
// the prop (or its bone +0x30) hidden / enabled (+0x28, +0x34). +0x29: the prop (or the
// model's nearest to the ped within 1) taken out of the nav obstacles first
uint8_t __fastcall PropGeometry_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x38) = owner;
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (self[0x29] != 0) {
        if (!prop) {
            int model = GetModelIndexByHash(At<uint32_t>(self, 0x2C), 0, 0x2FA7);
            prop = FindNearestObjectOfTypeInRange(model, Pos(OwnerPed(At<uint8_t*>(self, 0x38))), 1.0f, 0x1F);
            if (!prop) {
                return 0;
            }
        }
        At<uint32_t>(prop, 0x48) = 0;
        At<uint32_t>(prop, 0x20) = 0;
        NavObstacleList_Remove(navObstacles, prop);
    }
    if (prop) {
        V3 pos = *reinterpret_cast<V3*>(Pos(prop));
        int32_t bone = At<int32_t>(self, 0x30);
        if (bone) {
            NodeTransform transform;
            transform.scale = 1.0f;
            if (GetNodeTransform(prop, bone, &transform, 1)) {
                pos = transform.pos;
            }
        }
        int model = GetModelIndexByHash(At<uint32_t>(self, 0x2C), 0, 0x2FA7);
        SetNearestObjectOfTypeFlags(model, &pos, self[0x28], self[0x34]);
    }
    return 0;
}

// 0x656450 (thiscall): PropCollisionTrack::Start: the base's start, the owner's prop
// (+0xC) flagged (+0xE4), the test centre (+0x200) its bone (+0x210) or itself
uint8_t __fastcall PropCollision_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20C) = owner;
    BaseCollisionTrack_Start(self, owner);
    uint8_t* prop = At<uint8_t*>(At<uint8_t*>(self, 0x20C), 0xC);
    At<uint32_t>(prop, 0xE4) = 1;
    At<V3>(self, 0x200) = *reinterpret_cast<V3*>(Pos(prop));
    NodeTransform transform;
    transform.scale = 1.0f;
    if (GetNodeTransform(At<uint8_t*>(At<uint8_t*>(self, 0x20C), 0xC), At<int32_t>(self, 0x210), &transform, 1)) {
        At<V3>(self, 0x200) = transform.pos;
    }
    return 1;
}

} // namespace

// (variadic: the template arguments' commas)
#define PROPTRK_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

using af::Construct;
using af::Fields;
using af::Vt;
using af::KeyDtor;
using af::Call;
using af::ClearName;
using af::F32;
using af::I32;
using af::U8;
using af::TrackBaseDtor;

// field defaults (thiscall, after construction)
PROPTRK_REVERSED("PropAnimationTrack::InitFields", 0x00655660, &Fields<Call(0x00607660), U8(0x8, 1), ClearName(0x58), U8(0x5C, 0), U8(0x3C, 0)>);
PROPTRK_REVERSED("PropAnimationParametricTrack::InitFields", 0x006558B0, &Fields<Call(0x00608120), U8(0x8, 1)>);
PROPTRK_REVERSED("PropEffectTrack::InitFields", 0x00655910, &Fields<U8(0x8, 0), U8(0x20, 0)>);
PROPTRK_REVERSED("PropWeaponFireTrack::InitFields", 0x00655960, &Fields<U8(0x8, 0), ClearName(0x20), ClearName(0x24), ClearName(0x28), F32(0x1C, -1.0f), F32(0x18, -1.0f)>);
PROPTRK_REVERSED("PropGeometryTrack::InitFields", 0x00655A40, &Fields<U8(0x8, 0), U8(0x28, 0), U8(0x29, 0), U8(0x34, 0)>);
PROPTRK_REVERSED("PedFlagsTrack::InitFields_655A80", 0x00655A80, &Fields<U8(0x18, 1), I32(0x1C, 0), U8(0x8, 0)>);  // also EntityFlagsTrack
PROPTRK_REVERSED("PropCollisionTrack::InitFields", 0x00655AA0, &Fields<F32(0x5C, 0.5f), U8(0x8, 0), U8(0x214, 0), U8(0x215, 0), U8(0x216, 0), Call(0x005F76D0)>);
PROPTRK_REVERSED("PropTaggedTrack::InitFields", 0x00655B60, &Fields<U8(0x8, 1), F32(0x10, 300.0f), I32(0x1C, 0)>);
PROPTRK_REVERSED("PropTagEffectTrack::InitFields", 0x00655B90, &Fields<U8(0x8, 0), I32(0x18, 1)>);
PROPTRK_REVERSED("PropFlagsTrack::InitFields", 0x00655C90, &Fields<U8(0x18, 1), I32(0x1C, 4), U8(0x8, 0)>);
PROPTRK_REVERSED("PropRotateProxyTrack::InitFields", 0x00655CE0, &Fields<F32(0x18, 3.14159274f), U8(0x8, 0), U8(0x1C, 0)>);
PROPTRK_REVERSED("PropAttachPropAnimationTrack::InitFields", 0x00655DD0, &PropAttachPropAnimation_InitFields);
PROPTRK_REVERSED("PropAnimAttachToPropAnimTrack::InitFields", 0x00656000, &PropAnimAttachToPropAnim_InitFields);
PROPTRK_REVERSED("PropAnimRemoveAttachmentTrack::InitFields", 0x006560A0, &PropAnimRemoveAttachment_InitFields);

// destructors (thiscall)
PROPTRK_REVERSED("PropAnimationTrack::Destruct", 0x006556A0, &Fields<Vt(0x00935390), Call(0x00668EB0, 0x60), KeyDtor(0x58), Call(0x00608240)>);
PROPTRK_REVERSED("PropAnimationParametricTrack::Destruct", 0x006558C0, &Fields<Vt(0x009353AC), KeyDtor(0x60), Call(0x00608240)>);
PROPTRK_REVERSED("PropEffectTrack::Destruct", 0x00655920, &Fields<Vt(0x00935410), Call(0x00668EB0, 0x28), KeyDtor(0x1C), KeyDtor(0x18), TrackBaseDtor>);
PROPTRK_REVERSED("PropWeaponFireTrack::Destruct", 0x006559E0, &Fields<Vt(0x00935428), KeyDtor(0x28), KeyDtor(0x24), KeyDtor(0x20), TrackBaseDtor>);
PROPTRK_REVERSED("PropInteractTrack::Destruct", 0x00655A10, &Fields<Vt(0x00935440), TrackBaseDtor>);
PROPTRK_REVERSED("PlayOnPropTrack::Destruct", 0x00655A20, &Fields<Vt(0x00938220), Call(0x005F7BC0)>);
PROPTRK_REVERSED("PropSetPedActionTrack::Destruct", 0x00655A30, &Fields<Vt(0x00938248), Call(0x005F7BC0)>);
PROPTRK_REVERSED("PropGeometryTrack::Destruct", 0x00655A50, &Fields<Vt(0x00938270), KeyDtor(0x30), KeyDtor(0x2C), Call(0x005F7BC0)>);
PROPTRK_REVERSED("PropEntityFlagsTrack::Destruct", 0x00655A90, &Fields<Vt(0x00935470), TrackBaseDtor>);
PROPTRK_REVERSED("PropCollisionTrack::Destruct", 0x00655AD0, &Fields<Vt(0x00938298), KeyDtor(0x210), Call(0x005F8200)>);
PROPTRK_REVERSED("PropTagEffectTrack::Destruct", 0x00655BA0, &Fields<Vt(0x009353C8), TrackBaseDtor>);
PROPTRK_REVERSED("PropFlagsTrack::Destruct", 0x00655CA0, &Fields<Vt(0x00935458), TrackBaseDtor>);
PROPTRK_REVERSED("PropRotateProxyTrack::Destruct", 0x00655D00, &Fields<Vt(0x00935488), TrackBaseDtor>);
PROPTRK_REVERSED("PropAlterObstacleTrack::Destruct", 0x00655D10, &Fields<Vt(0x00935938), TrackBaseDtor>);
PROPTRK_REVERSED("PropUseTextTrack::Destruct", 0x00655D50, &Fields<Vt(0x00911978), KeyDtor(0x18), TrackBaseDtor>);
PROPTRK_REVERSED("PropSetTargetSocketTrack::Destruct", 0x00655D70, &Fields<Vt(0x009360F4), KeyDtor(0x18), TrackBaseDtor>);
PROPTRK_REVERSED("PropAttemptDoorTransitionTrack::Destruct", 0x00655D90, &Fields<Vt(0x009362E0), TrackBaseDtor>);
PROPTRK_REVERSED("PropAttachPropAnimationTrack::Destruct", 0x00655E30, &Fields<Vt(0x00936370), Call(0x00668EB0, 0x6C), KeyDtor(0x5C), KeyDtor(0x58), Call(0x00608240)>);
PROPTRK_REVERSED("PropAnimAttachToPropAnimTrack::Destruct", 0x00656070, &Fields<Vt(0x0093638C), KeyDtor(0x20), KeyDtor(0x1C), KeyDtor(0x18), TrackBaseDtor>);
PROPTRK_REVERSED("PropAnimRemoveAttachmentTrack::Destruct", 0x006560E0, &Fields<Vt(0x009363A4), KeyDtor(0x18), TrackBaseDtor>);
PROPTRK_REVERSED("PropTransferOwnershipTrack::Destruct", 0x00656170, &Fields<Vt(0x009363BC), TrackBaseDtor>);

// the tracks' virtuals
PROPTRK_REVERSED("PropAnimationTrack::Resolve", 0x006556D0, &PropAnimation_Resolve);
PROPTRK_REVERSED("PropAnimationTrack::Update", 0x00655700, &SyncedAnimation_Update<0x5C, true>);
PROPTRK_REVERSED("PropAnimationTrack::End", 0x00655880, &SyncedAnimation_End<0x5C>);
PROPTRK_REVERSED("PropAnimationParametricTrack::Resolve", 0x006558E0, &PropAnimationParametric_Resolve);
PROPTRK_REVERSED("PropAnimationParametricTrack::Update", 0x00655900, &PropAnimationParametric_Update);
PROPTRK_REVERSED("PropEffectTrack::End", 0x00655950, &PropEffect_End);
PROPTRK_REVERSED("PropCollisionTrack::Test", 0x00655AF0, &PropCollision_Test);
PROPTRK_REVERSED("PropTagEffectTrack::Start", 0x00655BB0, &PropTagEffect_Start);
PROPTRK_REVERSED("PropFlagsTrack::Start", 0x00655CB0, &PropFlags_Start);
PROPTRK_REVERSED("PropAlterObstacleTrack::Start", 0x00655D20, &PropAlterObstacle_Start);
PROPTRK_REVERSED("PropAttemptDoorTransitionTrack::Start", 0x00655DA0, &PropAttemptDoorTransition_Start);
PROPTRK_REVERSED("PropAttachPropAnimationTrack::Update", 0x00655E60, &SyncedAnimation_Update<0x60, false>);
PROPTRK_REVERSED("PropAttachPropAnimationTrack::End", 0x00655FD0, &SyncedAnimation_End<0x60>);
PROPTRK_REVERSED("PropAnimRemoveAttachmentTrack::Start", 0x00656100, &PropAnimRemoveAttachment_Start);
PROPTRK_REVERSED("PropEffectTrack::Start", 0x00656180, &PropEffect_Start);
PROPTRK_REVERSED("PropEffectTrack::Update", 0x00656220, &PropEffect_Update);
PROPTRK_REVERSED("PropInteractTrack::Start", 0x00656280, &PropInteract_Start);
PROPTRK_REVERSED("PropSetPedActionTrack::Start", 0x00656300, &PropSetPedAction_Start);
PROPTRK_REVERSED("PropGeometryTrack::Start", 0x00656330, &PropGeometry_Start);
PROPTRK_REVERSED("PropCollisionTrack::Start", 0x00656450, &PropCollision_Start);
