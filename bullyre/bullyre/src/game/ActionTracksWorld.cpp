// The action tree's tracks acting on the world: attached effects (and the dialog
// kinds), breath, ceiling / decal / water ripple / ground / wall impact effects, the
// camera freeze path, doors, the rail driver, stealth props, notifications, gifts and
// items, overhead icons, stats, the balance meter, camera / head-track parameters, the
// mirror camera, the balance beam heading and motion intention; and two animation
// lookups. 0x6101F0..0x611DB0.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

// NiTransform-ish local transform: rotation 3x3, then the translation at +0x24
struct PathNode {
    float pos[3];      // +0x0
    float look[3];     // +0xC
    int32_t id;        // +0x18
    float wait;        // +0x1C
    re::Ptr32<void> actions;     // +0x20
};
static_assert(sizeof(PathNode) == 0x24);
// an AE effect proxy
struct EffectProxy {
    re::Ptr32<void> vtable;
    re::Ptr32<void> effect;
};
static_assert(sizeof(EffectProxy) == 8);

GAME_VAR(void*, effectLibrary, 0x00B6F1E8);
GAME_VAR(void*, effectManager, 0x00B6F1D4);
GAME_VAR(uint8_t*, dialog_1B8919C, 0x01B8919C);      // +0x59: done
inline void* const conversations = reinterpret_cast<void*>(0x00B9DAB8);
GAME_VAR(uint8_t, currentChapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);
GAME_VAR(int32_t, currentArea, 0x00BD1008);
inline void* const surfaceTable = reinterpret_cast<void*>(0x00C21BC0);
inline void* const effectHistory = reinterpret_cast<void*>(0x00CF1F60);
inline void* const waterLevels = reinterpret_cast<void*>(0x00BC5DA8);
GAME_VAR(void*, niCamera, 0x00C3CCF4);               // TheCamera's Gamebryo camera
GAME_VAR(void*, pathStore, 0x00D13CB4);
GAME_VAR(uint8_t, freezePathMissing_B0BC40, 0x00B0BC40);
inline const char* const freezePathName = reinterpret_cast<const char*>(0x00930D58);   // "FreezePath"
inline void* const camera = reinterpret_cast<void*>(0x00C3CC68);                      // TheCamera
GAME_VAR(float, pedRadius, 0x00CF311C);
inline void* const areaTransition = reinterpret_cast<void*>(0x00BCC580);
GAME_VAR(float, stealthPropX, 0x020C440C);
GAME_VAR(float, stealthPropY, 0x020C4410);
GAME_VAR(float, stealthPropZ, 0x020C4414);
GAME_VAR(float, stealthPropHeading, 0x020C4418);
GAME_VAR(int32_t, gameMode, 0x00C3CCF8);
GAME_VAR(uint8_t*, modeCamera_C3CEC8, 0x00C3CEC8);

GAME_FN(uint16_t, __thiscall, EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(uint8_t*, __thiscall, Conversations_GetSpeaker, 0x006A29F0, void* conversations, int32_t which);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int32_t area);
GAME_FN(void*, __thiscall, Nodes_FindByName, 0x004A6BB0, void* nodes, const char* name, int32_t* index);
GAME_FN(uint8_t, __cdecl, FindAnimNode, 0x0060AE00, re::Ptr32<uint8_t>* clump, int32_t* index, const char* name, uint8_t* blend);
GAME_FN(void, __thiscall, AE_Manager_Start, 0x00668290, void* manager, const int32_t* type, EffectProxy* proxy, int);
GAME_FN(void*, __thiscall, Effect_GetNode, 0x00668E00, EffectProxy* proxy);
GAME_FN(int, __thiscall, AnimManager_Attach, 0x006C2070, void* manager, uint8_t* clump, int32_t index, uintptr_t,
        int32_t* handle, uintptr_t, uintptr_t, void* node);
GAME_FN(void, __thiscall, Effect_Stop, 0x00668E40, EffectProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Kill, 0x00668BA0, EffectProxy* proxy);
GAME_FN(void, __thiscall, Effect_SetAttachment, 0x00668DE0, EffectProxy* proxy, uint8_t* clump, uintptr_t handle);
GAME_FN(void, __thiscall, AE_Proxy_Construct, 0x00668E90, EffectProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_SetPosition, 0x00669040, EffectProxy* proxy, const float* pos, int);
GAME_FN(void, __thiscall, AE_Proxy_Destruct, 0x00668EB0, EffectProxy* proxy);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const float* start, const float* end, ColPoint* colPoint,
        re::Ptr32<uint8_t>* entity, int flags, int, int);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __thiscall, Skeleton_FindBone, 0x00465580, void* skeleton, uint32_t hash, int32_t* bone);
GAME_FN(void, __thiscall, AnimManager_GetBoneDirection, 0x006C11E0, void* manager, uint8_t* anim, int32_t bone,
        float* out, uintptr_t);
GAME_FN(void, __thiscall, Bones_GetPosition, 0x006C1190, void* manager, uint8_t* anim, int32_t bone, float* out, uintptr_t);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(void, __thiscall, EffectHistory_Start, 0x005610C0, void* history, int id, const float* pos, const float* dir,
        uint32_t surface);
GAME_FN(uint8_t, __thiscall, WaterLevels_Get, 0x006D4A20, void* levels, float* level, int32_t* flag, const float* pos);
GAME_FN(uint16_t, __thiscall, SurfaceEffects_Find, 0x004642B0, void* table, const uint32_t* surface, uint32_t hash);
GAME_FN(uint16_t, __thiscall, SurfaceTable_GetDecal, 0x00464330, void* table, const uint32_t* surface, uint32_t hash);
GAME_FN(PathNode*, __thiscall, PathNode_Construct, 0x005EC570, PathNode* node);
GAME_FN(void, __thiscall, PathNode_Destruct, 0x005EC7B0, PathNode* node);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);   // (+0x24: the translation)
GAME_FN(void*, __cdecl, PathPool_New, 0x005EC820, size_t size);
GAME_FN(void*, __thiscall, Path_Construct, 0x005EC5E0, void* path, int id);
GAME_FN(void, __thiscall, Path_SetName, 0x005EC610, void* path, const char* name);
GAME_FN(void*, __thiscall, PathStore_Get, 0x005ED360, void* store, const char* name);
GAME_FN(void, __thiscall, Path_Clear, 0x005ECBA0, void* path);
GAME_FN(void, __thiscall, Path_AddNode, 0x005ECB20, void* path, const PathNode* node);
GAME_FN(uint8_t*, __thiscall, Camera_GetCam, 0x004F3590, void* camera, int mode);
GAME_FN(void, __thiscall, ScriptCam_SetPath, 0x00507970, uint8_t* cam, void* path, int);
GAME_FN(void, __thiscall, ScriptCam_LookAtXYZ, 0x00507700, uint8_t* cam, const float* pos, int);
GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, uint8_t* owner);
GAME_FN(uint8_t, __thiscall, Door_IsLocked, 0x004EA3A0, uint8_t* door);
GAME_FN(uint8_t, __thiscall, Entity_IsTouchingMoving, 0x00465D10, uint8_t* entity, const float* centre, float radius);
GAME_FN(uint8_t, __thiscall, PropAnim_OpenDoor, 0x004EED10, uint8_t* prop, int, uint8_t* ped, int immediate);
GAME_FN(void, __thiscall, AreaTransition_SetTarget, 0x004152B0, void* transition, void* target);
GAME_FN(void, __thiscall, Player_LeaveRail, 0x0049AEB0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Events_IsOn, 0x005D80E0, uint8_t* events);
GAME_FN(void*, __thiscall, Events_New, 0x005D9C20, uint8_t* events);
GAME_FN(void, __cdecl, Event_Begin, 0x005D8270, void* event);
GAME_FN(int32_t, __thiscall, Events_GetId, 0x005D80D0, uint8_t* events);
GAME_FN(void, __cdecl, Event_SetId, 0x005D8670, void* event, int32_t id);
GAME_FN(void, __cdecl, Event_AddInt, 0x005D85C0, void* event, int32_t value);
GAME_FN(void, __cdecl, Event_Send, 0x005D82A0, void* event, int kind, int arg);
GAME_FN(int32_t*, __thiscall, Ped_GetGift, 0x00474AA0, uint8_t* ped, int32_t* out);
GAME_FN(uint8_t, __cdecl, IsWeaponItem, 0x0045B000, int32_t item);
GAME_FN(uint8_t, __cdecl, IsHeldItem, 0x0045B010, int32_t item);
GAME_FN(uint8_t, __cdecl, IsMoneyItem, 0x0045B020, int32_t item);
GAME_FN(int32_t, __cdecl, Inventory_FindItem, 0x00541EC0, int32_t item);
GAME_FN(void, __cdecl, Inventory_UseItem, 0x00541E40, int32_t slot);
GAME_FN(void, __thiscall, Ped_SetGiftAccepted, 0x0047D1B0, uint8_t* ped, int accepted);
GAME_FN(uint8_t, __thiscall, Ped_HasMoney, 0x004730E0, uint8_t* ped, int32_t amount);
GAME_FN(void, __thiscall, Ped_TakeMoney, 0x00473100, uint8_t* ped, int32_t amount);
GAME_FN(void, __thiscall, Ped_SetGiftWanted, 0x00474B30, uint8_t* ped, int32_t gift);
GAME_FN(void, __thiscall, Ped_SetCanAcceptGift, 0x00474A90, uint8_t* ped, int can);
GAME_FN(void, __thiscall, Ped_SetOverheadIcon, 0x0047D200, uint8_t* ped, int32_t icon);
GAME_FN(void, __cdecl, Stat_SetAsInt, 0x00458460, int32_t stat, int32_t value);
GAME_FN(void, __cdecl, Stat_SetAsFloat, 0x00458470, int32_t stat, float value);
GAME_FN(void, __cdecl, Stat_AddToInt, 0x00458480, int32_t stat, int32_t value);
GAME_FN(void, __cdecl, Stat_AddToFloat, 0x004585B0, int32_t stat, float value);
GAME_FN(void, __thiscall, BalanceMeter_Start, 0x004B9470, uint8_t* meter);
GAME_FN(void, __thiscall, BalanceMeter_SetInput, 0x004B94E0, uint8_t* meter, float input);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int);
GAME_FN(void, __thiscall, OffsetCamera_Mirror180, 0x00505F70, uint8_t* camera);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x00475730, uint8_t* ped, float heading);
GAME_FN(float, __cdecl, MapRange_432B60, 0x00432B60, float value, float a, float b, float c, float d);
GAME_FN(float, __cdecl, Clamp, 0x0041A470, float value, float low, float high);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const V3* v);
GAME_FN(V3*, __cdecl, Vector_FromAngle, 0x00432DF0, V3* out, float angle);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(void, __thiscall, AttachEffectTo, 0x006097B0, uint8_t* track, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, PlaceDecal, 0x006105A0, uint8_t* track, uint8_t* entity);
GAME_FN(void, __thiscall, Breath_Emit, 0x006102A0, uint8_t* track);

ColPoint EmptyColPoint() {
    ColPoint cp = {};
    cp.depth = 3.4028235e38f;
    return cp;
}
// the surface dword the effect history is given (the col point's +0x22..+0x25)
uint32_t SurfaceDword(const ColPoint& cp) {
    uint32_t value;
    std::memcpy(&value, reinterpret_cast<const uint8_t*>(&cp) + 0x22, 4);
    return value;
}

// 0x6101F0 (thiscall): AddAttachedEffectTrack::Start: attached to the ped
uint8_t __fastcall AddAttachedEffectTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    return reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, uint8_t*)>(0x006097B0)(self, OwnerPed(owner));
}

// the dialog tracks' target: 8 the speaker, 9 who it grapples (none: nobody), else the
// conversation's participant +0x28
template <uintptr_t Apply>
uint8_t __fastcall DialogTrack_Update(uint8_t* self, re::Edx, float) {
    auto apply = reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, uint8_t*)>(Apply);
    if (self[0x2C] != 0 || (dialog_1B8919C && dialog_1B8919C[0x59] == 0)) {
        return 1;
    }
    int32_t which = At<int32_t>(self, 0x28);
    uint8_t* target = nullptr;
    if (which == 8) {
        apply(self, OwnerPed(At<uint8_t*>(self, 0x24)));
        self[0x2C] = 1;
        return 1;
    }
    if (which == 9) {
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
        if (ped && At<uint8_t*>(ped, 0x155C)) {
            apply(self, At<uint8_t*>(ped, 0x155C));
            self[0x2C] = 1;
            return 1;
        }
    } else {
        target = Conversations_GetSpeaker(conversations, which);
    }
    apply(self, target);
    self[0x2C] = 1;
    return 1;
}

// 0x6102A0 (thiscall): BreathTrack's breath effect +0x18 at node +0x1C of the owner's
// clump (+0x3C +4) in season +0x20 (0: any) and area +0x24 (1: outside, 2: inside)
void __fastcall BreathTrack_Emit(uint8_t* self, re::Edx) {
    int32_t season = At<int32_t>(self, 0x20);
    int32_t where = At<int32_t>(self, 0x24);
    bool allowed = true;
    if (season == 1 || season == 2 || season == 3) {
        if (chapterSeason[currentChapter] != season) {
            allowed = false;
        }
    }
    if (where == 1) {
        if (IsAreaInterior(currentArea)) {
            return;
        }
    } else if (where == 2) {
        if (!IsAreaInterior(currentArea)) {
            return;
        }
    }
    if (!allowed) {
        return;
    }
    const char* node = At<const char*>(self, 0x1C);
    int effect = EffectLibrary_Find(effectLibrary, At<const char*>(self, 0x18));
    int32_t index = 0;
    if (!node || !*node) {
        return;
    }
    uint8_t* clump = At<uint8_t*>(At<uint8_t*>(self, 0x3C), 4);
    if (!clump) {
        return;
    }
    if (!Nodes_FindByName(At<void*>(clump, 0x94), node, &index)) {
        re::Ptr32<uint8_t> found = clump;
        if (!FindAnimNode(&found, &index, node, At<uint8_t*>(At<uint8_t*>(self, 0x3C), 4))) {
            return;
        }
        clump = found;
    }
    EffectProxy* proxy = reinterpret_cast<EffectProxy*>(self + 0x2C);
    int32_t type = effect;
    AE_Manager_Start(effectManager, &type, proxy, 0);
    if (!At<void*>(self, 0x30)) {
        return;
    }
    void* effectNode = Effect_GetNode(proxy);
    int32_t handle = 0;
    if (AnimManager_Attach(animManager, clump, index, 0, &handle, 0, 0, effectNode)) {
        Effect_Stop(proxy);
        AE_Proxy_Kill(proxy);
    }
    Effect_SetAttachment(proxy, clump, handle);
}

// 0x610430 (thiscall): BreathTrack::Update: breathes (+0x28) while it has no effect
uint8_t __fastcall BreathTrack_Update(uint8_t* self, re::Edx, float) {
    if (!At<void*>(self, 0x30) && self[0x28] != 0) {
        Breath_Emit(self);
    }
    return 1;
}

// 0x610450 (thiscall): FXCeilingTrack::Start: effect +0x18 on the ceiling above the ped
// (a line from 2.5 up to 100 up)
uint8_t __fastcall FXCeilingTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    int effect = EffectLibrary_Find(effectLibrary, At<const char*>(self, 0x18));
    EffectProxy proxy;
    AE_Proxy_Construct(&proxy);
    float* p = Pos(OwnerPed(At<uint8_t*>(self, 0x1C)));
    float from[3] = {p[0], p[1], p[2]};
    float to[3] = {p[0], p[1], float(double(p[2]) + 100.0)};
    ColPoint cp = EmptyColPoint();
    re::Ptr32<uint8_t> hit = nullptr;
    from[2] = float(double(from[2]) + 2.5);
    if (ProcessLineOfSight(from, to, &cp, &hit, 0xF, 0, 0)) {
        int32_t type = effect;
        AE_Manager_Start(effectManager, &type, &proxy, 0);
        float at[3] = {cp.pos.x, cp.pos.y, cp.pos.z};
        AE_Proxy_SetPosition(&proxy, at, 0);
    }
    ReleaseColPoint(cp);
    AE_Proxy_Destruct(&proxy);
    return 0;
}

// 0x6105A0 (thiscall): a decal (effect +0x18) where a short line hits: from +0x20 above
// the entity along its forward, else from bone +0x1C along the bone, else straight down
uint8_t __fastcall DecalTrack_Place(uint8_t* self, re::Edx, uint8_t* entity) {
    if (!entity) {
        return 0;
    }
    int effect = EffectLibrary_Find(effectLibrary, At<const char*>(self, 0x18));
    const char* boneName = At<const char*>(self, 0x1C);
    V3 dir = {0.0f, 0.0f, 0.0f};
    float pos[3] = {0.0f, 0.0f, 0.0f};
    int32_t bone = 0;
    if (!(0.0f == At<float>(self, 0x20))) {
        float* p = Pos(entity);
        pos[0] = p[0];
        pos[1] = p[1];
        pos[2] = p[2];
        pos[2] = pos[2] + At<float>(self, 0x20);
        V3 forward;
        Placeable_GetForward(entity, &forward);
        forward.z = 0.0f;
        Vector_Normalise(&forward);
        dir = forward;
    } else if (boneName && *boneName) {
        void* skeleton = At<void*>(At<uint8_t*>(entity, 0x114), 0x94);
        if (!Skeleton_FindBone(skeleton, HashName(boneName), &bone)) {
            return 0;
        }
        AnimManager_GetBoneDirection(animManager, At<uint8_t*>(entity, 0x114), bone, &dir.x, 0);
        Bones_GetPosition(animManager, At<uint8_t*>(entity, 0x114), bone, pos, 0);
    } else {
        float* p = Pos(entity);
        pos[0] = p[0];
        pos[1] = p[1];
        pos[2] = p[2];
        dir = {0.0f, 0.0f, -1.0f};
    }
    Vector_Normalise(&dir);
    float back[3] = {dir.x * -0.5f, dir.y * -0.5f, -0.5f * dir.z};
    V3 from;
    VectorAdd(&from, pos, back);
    float ahead[3] = {dir.x * 3.0f, dir.y * 3.0f, 3.0f * dir.z};
    V3 to;
    VectorAdd(&to, pos, ahead);
    ColPoint cp = EmptyColPoint();
    re::Ptr32<uint8_t> hit = nullptr;
    if (ProcessLineOfSight(&from.x, &to.x, &cp, &hit, 9, 0, 0)) {
        EffectHistory_Start(effectHistory, effect, &cp.pos.x, &cp.normal.x, SurfaceDword(cp));
    }
    ReleaseColPoint(cp);
    return 1;
}

// 0x610870 (thiscall): FXDecalTrack::Start: the decal at the ped
uint8_t __fastcall FXDecalTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    return PlaceDecal(self, OwnerPed(owner));
}

// 0x610920 (thiscall): the water level at the ped (-1000: none)
float __fastcall FXWaterRipplesTrack_WaterLevel(uint8_t* self, re::Edx) {
    float level;
    int32_t flag;
    if (!WaterLevels_Get(waterLevels, &level, &flag, Pos(OwnerPed(At<uint8_t*>(self, 0x20))))) {
        level = -1000.0f;
    }
    return level;
}

// 0x610970 (thiscall): GroundImpactTrack::Start: below bone +0x18, the ground's decal and
// effect for the surface hit (+0x1C names the set)
uint8_t __fastcall GroundImpactTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    V3 dir = {0.0f, 0.0f, 0.0f};
    float pos[3] = {0.0f, 0.0f, 0.0f};
    const char* boneName = At<const char*>(self, 0x18);
    At<uint8_t*>(self, 0x20) = owner;
    int32_t bone = 0;
    if (!boneName || !*boneName) {
        return 0;
    }
    uint8_t* anim = At<uint8_t*>(owner, 4);
    if (!Skeleton_FindBone(At<void*>(anim, 0x94), HashName(boneName), &bone)) {
        return 0;
    }
    Bones_GetPosition(animManager, At<uint8_t*>(At<uint8_t*>(self, 0x20), 4), bone, pos, 0);
    dir = {0.0f, 0.0f, -1.0f};
    Vector_Normalise(&dir);
    float back[3] = {dir.x * -0.5f, dir.y * -0.5f, -0.5f * dir.z};
    V3 from;
    VectorAdd(&from, pos, back);
    float ahead[3] = {dir.x * 3.0f, dir.y * 3.0f, 3.0f * dir.z};
    V3 to;
    VectorAdd(&to, pos, ahead);
    ColPoint cp = EmptyColPoint();
    re::Ptr32<uint8_t> hit = nullptr;
    if (ProcessLineOfSight(&from.x, &to.x, &cp, &hit, 9, 0, 0)) {
        uint32_t set = HashName(At<const char*>(self, 0x1C));
        uint32_t surface = reinterpret_cast<uint8_t*>(&cp)[0x20];
        int fx = SurfaceEffects_Find(surfaceTable, &surface, set);
        int decal = SurfaceTable_GetDecal(surfaceTable, &surface, set);
        float decalPos[3] = {cp.pos.x, cp.pos.y, float(double(cp.pos.z) + 0.05)};
        EffectHistory_Start(effectHistory, decal, decalPos, &cp.normal.x, SurfaceDword(cp));
        EffectProxy proxy;
        AE_Proxy_Construct(&proxy);
        int32_t type = fx;
        AE_Manager_Start(effectManager, &type, &proxy, 0);
        AE_Proxy_SetPosition(&proxy, &cp.pos.x, 0);
        AE_Proxy_Destruct(&proxy);
    }
    ReleaseColPoint(cp);
    return 0;
}

// 0x610C10 (thiscall): WallImpactTrack::Start: from bone +0x18 along the ped's forward
// (from +0x20 on, else the default reach), the wall's decal and effect
uint8_t __fastcall WallImpactTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    float pos[3] = {0.0f, 0.0f, 0.0f};
    const char* boneName = At<const char*>(self, 0x18);
    At<uint8_t*>(self, 0x24) = owner;
    int32_t bone = 0;
    if (!boneName || !*boneName) {
        return 0;
    }
    uint8_t* ped = OwnerPed(owner);
    if (!Skeleton_FindBone(At<void*>(At<uint8_t*>(ped, 0x114), 0x94), HashName(boneName), &bone)) {
        return 0;
    }
    Bones_GetPosition(animManager, At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x24)), 0x114), bone, pos, 0);
    V3 forward;
    Placeable_GetForward(OwnerPed(At<uint8_t*>(self, 0x24)), &forward);
    forward.z = 0.0f;
    Vector_Normalise(&forward);
    float nearReach = *reinterpret_cast<float*>(0x00B0B170);
    float farReach = *reinterpret_cast<float*>(0x00B0B174);
    V3 dir = forward;
    if (0.0f < At<float>(self, 0x20) || 0.0f == At<float>(self, 0x20)) {
        nearReach = At<float>(self, 0x20);
        farReach = -0.1f;
    }
    float a[3] = {nearReach * forward.x, forward.y * nearReach, nearReach * forward.z};
    V3 from;
    VectorAdd(&from, pos, a);
    float b[3] = {farReach * dir.x, dir.y * farReach, farReach * dir.z};
    V3 to;
    VectorAdd(&to, pos, b);
    ColPoint cp = EmptyColPoint();
    re::Ptr32<uint8_t> hit = nullptr;
    if (ProcessLineOfSight(&from.x, &to.x, &cp, &hit, 9, 0, 0)) {
        uint32_t set = HashName(At<const char*>(self, 0x1C));
        uint32_t surface = reinterpret_cast<uint8_t*>(&cp)[0x20];
        int fx = SurfaceEffects_Find(surfaceTable, &surface, set);
        int decal = SurfaceTable_GetDecal(surfaceTable, &surface, set);
        EffectHistory_Start(effectHistory, decal, &cp.pos.x, &cp.normal.x, SurfaceDword(cp));
        EffectProxy proxy;
        AE_Proxy_Construct(&proxy);
        int32_t type = fx;
        AE_Manager_Start(effectManager, &type, &proxy, 0);
        AE_Proxy_SetPosition(&proxy, &cp.pos.x, 0);
        AE_Proxy_Destruct(&proxy);
    }
    ReleaseColPoint(cp);
    return 0;
}

// 0x610ED0 (thiscall): CameraFreezeTrack::Start: the script camera held where it is (a
// one-node "FreezePath"), looking along its view
uint8_t __fastcall CameraFreezeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    PathNode node;
    PathNode_Construct(&node);
    float* t = GetLocalTransform(GetCameraNode(niCamera));
    node.pos[0] = t[9];
    node.pos[1] = t[10];
    node.pos[2] = t[11];
    if (freezePathMissing_B0BC40 != 0) {
        void* path = PathPool_New(0x14);
        path = path ? Path_Construct(path, -1) : nullptr;
        Path_SetName(path, freezePathName);
        freezePathMissing_B0BC40 = 0;
    }
    void* path = PathStore_Get(pathStore, freezePathName);
    Path_Clear(path);
    Path_AddNode(path, &node);
    uint8_t* cam = Camera_GetCam(camera, 4);
    ScriptCam_SetPath(cam, path, 1);
    t = GetLocalTransform(GetCameraNode(niCamera));
    float view[3] = {t[2], t[5], t[8]};
    float* t2 = GetLocalTransform(GetCameraNode(niCamera));
    float look[3] = {view[0] + t2[9], t2[10] + view[1], t2[11] + view[2]};
    ScriptCam_LookAtXYZ(cam, look, 1);
    PathNode_Destruct(&node);
    return 1;
}

// 0x611030 (thiscall): ActivateDoorTrack::Start: the use point's door opened when the
// ped (or who it grapples) touches it
uint8_t __fastcall ActivateDoorTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* door = UsePoint_GetEntity(owner);
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (!ped || !door || Door_IsLocked(door) || At<int32_t>(door, 0x184) != 0) {
        return 0;
    }
    uint8_t* grappled = At<uint8_t*>(ped, 0x155C);
    float* p = Pos(ped);
    float at[3] = {p[0], p[1], p[2]};
    float radius = float(double(pedRadius) + 0.2);
    if (!Entity_IsTouchingMoving(door, at, radius)) {
        if (!grappled) {
            return 0;
        }
        float* q = Pos(grappled);
        float at2[3] = {q[0], q[1], q[2]};
        radius = float(double(pedRadius) + 0.2);
        if (!Entity_IsTouchingMoving(door, at2, radius)) {
            return 0;
        }
    }
    if (PropAnim_OpenDoor(door, 0, ped, 0) && At<int32_t>(door, 0x194) != 0) {
        AreaTransition_SetTarget(areaTransition, At<void*>(door, 0x23C));
    }
    return 0;
}

// 0x6111A0 (thiscall): RailDriverTrack::End: the ped off the rail
uint8_t __fastcall RailDriverTrack_End(uint8_t* self, re::Edx) {
    Player_LeaveRail(OwnerPed(At<uint8_t*>(self, 0x1C)));
    return 0;
}

// 0x6111B0 (thiscall): SetInStealthPropTrack::Start: the player's position and heading
// remembered, its stealth prop flag (+0x1E91) set to +0x18
uint8_t __fastcall SetInStealthPropTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* player = playerPed;
    if (OwnerPed(owner) != player) {
        return 0;
    }
    float heading;
    float* m = At<float*>(player, 0x14);
    if (m) {
        heading = float(std::atan2(double(-m[4]), double(m[5])));
    } else {
        heading = At<float>(player, 0x10);
    }
    float* p = Pos(player);
    stealthPropX = p[0];
    stealthPropY = p[1];
    stealthPropHeading = heading;
    stealthPropZ = p[2];
    player[0x1E91] = self[0x18];
    return 0;
}

// the ped's event block (at Events) told about the ped (its vf 0xAC id)
void SendEvent(uint8_t* ped, int events) {
    uint8_t* block = ped + events;
    void* event = Events_New(block);
    Event_Begin(event);
    Event_SetId(event, Events_GetId(block));
    Event_AddInt(event, vc::Call<int32_t>(ped, 0xAC));
    Event_Send(event, 1, 0);
}

// 0x611240 / 0x6112B0 (thiscall): NotifyOnSightPlayerTrack / NotifyOnHearTrack::Start
template <int Events>
uint8_t __fastcall NotifyTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (Events_IsOn(ped + Events)) {
        SendEvent(ped, Events);
    }
    return 0;
}

// 0x611320 (thiscall): NotifyOnMissionCriticalTrack::Start (mission critical peds: +0x141C;
// +0x18: only kind 1 (+0x1420))
uint8_t __fastcall NotifyOnMissionCriticalTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (At<int32_t>(ped, 0x141C) == 0 || !Events_IsOn(ped + 0x18E8)) {
        return 0;
    }
    if (self[0x18] != 0 && At<int32_t>(ped, 0x1420) != 1) {
        return 0;
    }
    SendEvent(ped, 0x18E8);
    return 0;
}

// the price of a money item (0x16: 100, 0x17: 500, else 1000); false when it isn't one
// the player can pay
bool MoneyItemPrice(int32_t item, int32_t* price) {
    if (!playerPed || item - 0x16 >= 3) {
        return false;
    }
    if (item == 0x16) {
        *price = 100;
    } else {
        *price = item != 0x17 ? 1000 : 500;
    }
    return true;
}

// 0x6113B0 (thiscall): AcceptGiftTrack::Start: the gift the ped wants taken from the
// player (a weapon / held item, or money) and marked accepted
uint8_t __fastcall AcceptGiftTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 1;
    }
    int32_t gift;
    int32_t item = *Ped_GetGift(ped, &gift);
    if (IsWeaponItem(item)) {
        int32_t slot = Inventory_FindItem(item);
        if (slot != 0x50) {
            if (slot < 0) {
                return 1;
            }
            Inventory_UseItem(slot);
        }
        Ped_SetGiftAccepted(ped, 1);
        return 1;
    }
    if (IsHeldItem(item)) {
        Ped_SetGiftAccepted(ped, 1);
        return 1;
    }
    if (!IsMoneyItem(item)) {
        return 1;
    }
    int32_t price;
    if (!MoneyItemPrice(item, &price)) {
        return 1;
    }
    uint8_t* player = playerPed;
    if (!Ped_HasMoney(player, price)) {
        return 1;
    }
    Ped_TakeMoney(player, price);
    Ped_SetGiftAccepted(ped, 1);
    return 1;
}

// 0x611480 (thiscall): RemoveItemTrack::Start: item +0x18 taken from the player
uint8_t __fastcall RemoveItemTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (!OwnerPed(owner)) {
        return 1;
    }
    int32_t item = At<int32_t>(self, 0x18);
    if (IsWeaponItem(item)) {
        int32_t slot = Inventory_FindItem(item);
        if (slot >= 0) {
            Inventory_UseItem(slot);
            return 1;
        }
        return 1;
    }
    if (IsHeldItem(item) || !IsMoneyItem(item)) {
        return 1;
    }
    int32_t price;
    if (!MoneyItemPrice(item, &price)) {
        return 1;
    }
    uint8_t* player = playerPed;
    if (Ped_HasMoney(player, price)) {
        Ped_TakeMoney(player, price);
    }
    return 1;
}

// 0x611530 (thiscall): SetCanAcceptGiftTrack::Start
uint8_t __fastcall SetCanAcceptGiftTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        if (At<int32_t>(self, 0x1C) != 0) {
            Ped_SetGiftWanted(ped, At<int32_t>(self, 0x1C));
        }
        Ped_SetCanAcceptGift(ped, self[0x18]);
    }
    return 1;
}

// 0x611570 (thiscall): SetOverheadIconTrack::Start: icon +0x18 over the ped (+0x1C: over
// its +0x538 ped)
uint8_t __fastcall SetOverheadIconTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (self[0x1C] != 0) {
        uint8_t* other = At<uint8_t*>(ped, 0x538);
        if (other && IsPedEntity(other)) {
            Ped_SetOverheadIcon(other, At<int32_t>(self, 0x18));
            return 1;
        }
        return 1;
    }
    if (ped) {
        Ped_SetOverheadIcon(ped, At<int32_t>(self, 0x18));
    }
    return 1;
}

// the stat tracks: (+0x18) for the player only, (+0x19) when the player is the target
bool StatTrackApplies(uint8_t* self, uint8_t* owner) {
    uint8_t* player = playerPed;
    if (self[0x18] != 0 && OwnerPed(owner) != player) {
        return false;
    }
    if (self[0x19] != 0 && OwnerPed(owner)) {
        if (Ped_GetTarget(OwnerPed(owner)) != player) {
            return false;
        }
    }
    return true;
}

// 0x6115C0 / 0x611620 / 0x611680 / 0x6116E0 (thiscall): stat +0x20 set to / added +0x24
uint8_t __fastcall StatSetAsIntTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (StatTrackApplies(self, owner)) {
        Stat_SetAsInt(At<int32_t>(self, 0x20), At<int32_t>(self, 0x24));
    }
    return 0;
}
uint8_t __fastcall StatSetAsFloatTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (StatTrackApplies(self, owner)) {
        Stat_SetAsFloat(At<int32_t>(self, 0x20), At<float>(self, 0x24));
    }
    return 0;
}
uint8_t __fastcall StatAddToIntTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (StatTrackApplies(self, owner)) {
        Stat_AddToInt(At<int32_t>(self, 0x20), At<int32_t>(self, 0x24));
    }
    return 0;
}
uint8_t __fastcall StatAddToFloatTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (StatTrackApplies(self, owner)) {
        Stat_AddToFloat(At<int32_t>(self, 0x20), At<float>(self, 0x24));
    }
    return 0;
}

// 0x611740 (thiscall): BalanceMeterTrack::Start: the player's meter set up (+0x18 degrees,
// +0x1C.. its tuning) and on
uint8_t __fastcall BalanceMeterTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x38) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (At<int32_t>(ped, 0x1310) == 13) {
        uint8_t* meter = At<uint8_t*>(ped, 0x1ED8);
        At<float>(meter, 8) = float(double(At<float>(self, 0x18)) * double(3.14159274f) / 180.0);
        At<float>(meter, 4) = At<float>(self, 0x1C);
        At<float>(meter, 0xC) = At<float>(self, 0x20);
        At<float>(meter, 0x10) = At<float>(self, 0x24);
        At<float>(meter, 0x14) = At<float>(self, 0x2C);
        At<float>(meter, 0x18) = At<float>(self, 0x28);
        BalanceMeter_Start(meter);
        meter[0] = 1;
    }
    return 1;
}

// 0x6117A0 (thiscall): BalanceMeterTrack::Update: the player steers the meter (+0x30: 0
// the left stick, 1 the other reversed; a bike's crash pushes it back); runs while it's
// within its range; +0x34: the parameter from its position (+0x35 not reversed). Others
// sway on a sine
uint8_t __fastcall BalanceMeterTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x38);
    uint8_t* ped = OwnerPed(owner);
    if (At<int32_t>(ped, 0x1310) != 13) {
        float sway = float(std::sin(double(float(double(timeInMs) * 0.001))));
        *At<float*>(At<uint8_t*>(self, 0x38), 0x38) = float(double(sway) * 0.2 + 0.5);
        return 1;
    }
    uint8_t* meter = At<uint8_t*>(ped, 0x1ED8);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    // (for other +0x30 kinds the game passes stack garbage as the input)
    float input = 0.0f;
    if (vehicle && At<uint32_t>(vehicle, 0x724) + 400 > timeInMs &&
        At<uint32_t>(vehicle, 0x728) + 400 > At<uint32_t>(vehicle, 0x724)) {
        input = (0.0f < At<float>(meter, 0x20) || 0.0f == At<float>(meter, 0x20) ||
                 std::isnan(At<float>(meter, 0x20)))
                    ? -1.0f
                    : 1.0f;
    } else if (At<int32_t>(self, 0x30) == 0) {
        input = Pad_GetValue(0x10, 0);
    } else if (At<int32_t>(self, 0x30) == 1) {
        input = -Pad_GetValue(0x11, 0);
    }
    BalanceMeter_SetInput(meter, input);
    float position = At<float>(meter, 0x20) / (At<float>(meter, 8) * 0.5f);
    uint8_t inside = -1.0f < position && position < 1.0f ? 1 : 0;
    if (self[0x34] == 0) {
        return inside;
    }
    float fraction = position * 0.5f + 0.5f;
    if (fraction < 0.0f) {
        fraction = 0.0f;
    }
    if (1.0f < fraction) {
        fraction = 1.0f;
    }
    if (self[0x35] == 0) {
        *At<float*>(At<uint8_t*>(self, 0x38), 0x38) = 1.0f - fraction;
        return inside;
    }
    *At<float*>(At<uint8_t*>(self, 0x38), 0x38) = fraction;
    return inside;
}

// 0x611950 (thiscall): BalanceMeterTrack::End: the player's meter off
uint8_t __fastcall BalanceMeterTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x38));
    if (At<int32_t>(ped, 0x1310) == 13) {
        At<uint8_t*>(ped, 0x1ED8)[0] = 0;
    }
    return 1;
}

// 0x611970 (thiscall): CameraGetParametricTrack::Update: the player's mode 13 camera's
// +0x12C (+0x18) or +0x130 as the parameter
uint8_t __fastcall CameraGetParametricTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x1C);
    if (At<int32_t>(OwnerPed(owner), 0x1310) == 13 && gameMode == 13) {
        uint8_t* cam = modeCamera_C3CEC8;
        *At<float*>(owner, 0x38) = self[0x18] != 0 ? At<float>(cam, 0x12C) : At<float>(cam, 0x130);
    }
    return 1;
}

// 0x6119D0 (thiscall): HeadtrackGetParametricTrack::Update: the ped's head-track +0x1E7C
// (+0x18) or +0x1E80 as the parameter (0.5 with no ped)
uint8_t __fastcall HeadtrackGetParametricTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x1C);
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        *At<float*>(owner, 0x38) = 0.5f;
        return 1;
    }
    *At<float*>(owner, 0x38) = self[0x18] != 0 ? At<float>(ped, 0x1E7C) : At<float>(ped, 0x1E80);
    return 1;
}

// 0x611A20 / 0x611A50 (thiscall): OffsetCameraMirror180Track: the player's mode 13 camera
// turned round, running while it turns
uint8_t __fastcall OffsetCameraMirror180Track_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    if (At<int32_t>(OwnerPed(owner), 0x1310) == 13 && gameMode == 13) {
        OffsetCamera_Mirror180(modeCamera_C3CEC8);
    }
    return 1;
}
uint8_t __fastcall OffsetCameraMirror180Track_Update(uint8_t* self, re::Edx, float) {
    if (At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x18)), 0x1310) == 13 && gameMode == 13 &&
        modeCamera_C3CEC8[0x134] != 0) {
        return 1;
    }
    return 0;
}

// 0x611A90 (cdecl): the ped's playing animation (in its blend's list, +0xDC / +0xF0) of
// the group entry named name (0x30 bytes each)
uint32_t* __cdecl FindPlayingAnimation(uint8_t* owner, const char* name) {
    uint8_t* blend = At<uint8_t*>(OwnerPed(owner), 0x114);
    uint8_t* table = At<uint8_t*>(At<uint8_t*>(blend, 0x94), 0x10);
    uint8_t* entry = nullptr;
    for (uint32_t i = 0; i < At<uint32_t>(table, 0x78); i++) {
        if (Stricmp(reinterpret_cast<const char*>(At<uint8_t*>(table, 0x74) + i * 0x30), name) == 0) {
            entry = At<uint8_t*>(table, 0x74) + i * 0x30;
            break;
        }
    }
    if (!entry) {
        return nullptr;
    }
    uint32_t* anim = At<int32_t>(blend, 0xF0) != 0 ? *At<re::Ptr32<uint32_t>*>(blend, 0xDC) : nullptr;
    while (anim) {
        if (anim[0] == reinterpret_cast<uintptr_t>(entry)) {
            return anim;
        }
        uint8_t* link = reinterpret_cast<uint8_t*>(uintptr_t(anim[0xD]));
        if (!link || !At<re::Ptr32<uint32_t>*>(link, 8)) {
            return nullptr;
        }
        anim = *At<re::Ptr32<uint32_t>*>(link, 8);
    }
    return anim;
}

// 0x611B40 (cdecl): a track's (+0x18: 0 the ped, 1 who it grapples, 3 its prop) anim
// object and bone (+0x1C)
void __cdecl Track_GetBone(uint8_t* owner, uint8_t* track, re::Ptr32<uint8_t>* anim, int32_t* bone) {
    uint8_t* entity;
    switch (At<int32_t>(track, 0x18)) {
    case 0:
        entity = OwnerPed(owner);
        break;
    case 1:
        entity = At<uint8_t*>(OwnerPed(owner), 0x155C);
        if (!entity) {
            return;
        }
        break;
    case 3:
        entity = At<uint8_t*>(OwnerPed(owner), 0x438);
        if (!entity) {
            return;
        }
        break;
    default:
        return;
    }
    *anim = At<uint8_t*>(entity, 0x114);
    void* skeleton = At<void*>(*anim, 0x94);
    *bone = reinterpret_cast<int32_t(__thiscall*)(void*, uint32_t, int32_t*)>(0x00465580)(
        skeleton, HashName(At<const char*>(track, 0x1C)), nullptr);
}

// the beam's player (the balance beam on)
bool OnBeam(uint8_t* ped) {
    uint8_t* player = playerPed;
    return ped && player && ped == player && At<int32_t>(player, 0x1548) != 0;
}

// 0x611BC0 (thiscall): BeamOrientToSafeHeadingTrack::Start: from the heading (+0x28) to
// the beam's (+0x2C); +0x18 at once
uint8_t __fastcall BeamOrientTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!OnBeam(ped)) {
        return 0;
    }
    At<float>(self, 0x28) = At<float>(ped, 0x1700);
    At<float>(self, 0x2C) = At<float>(playerPed, 0x1EAC);
    At<float>(self, 0x30) = 0.0f;
    if (At<float>(self, 0x20) < 0.0f) {
        At<float>(self, 0x20) = 0.0f;
    }
    if (self[0x18] != 0) {
        Ped_SetHeading(ped, At<float>(self, 0x2C));
    }
    return 1;
}

// 0x611C30 (thiscall): BeamOrientToSafeHeadingTrack::Update: turned between +0x1C and
// +0x1C + +0x20 into the node
uint8_t __fastcall BeamOrientTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x24);
    uint8_t* ped = OwnerPed(owner);
    if (!OnBeam(ped)) {
        return 0;
    }
    if (self[0x18] != 0) {
        Ped_SetHeading(ped, At<float>(self, 0x2C));
        return 1;
    }
    float time = At<float>(At<uint8_t*>(owner, 0), 8);
    float start = At<float>(self, 0x1C);
    if (!(0.0f < start || 0.0f == start)) {
        return 1;
    }
    if (!(start < time)) {
        return 1;
    }
    if (start + At<float>(self, 0x20) < time || std::isnan(start + At<float>(self, 0x20))) {
        return 1;
    }
    float into = time - At<float>(self, 0x1C);
    At<float>(self, 0x30) = into;
    float fraction = MapRange_432B60(into, 0.0f, At<float>(self, 0x1C), 1.0f, At<float>(self, 0x20) + At<float>(self, 0x1C));
    fraction = Clamp(fraction, 0.0f, 1.0f);
    float turn = LimitAngle(At<float>(self, 0x2C) - At<float>(self, 0x28));
    float heading = turn * fraction + At<float>(self, 0x28);
    At<float>(self, 0x34) = heading;
    Ped_SetHeading(ped, heading);
    return 1;
}

// 0x611D90 (thiscall): BeamOrientToSafeHeadingTrack::End: the beam's heading
uint8_t __fastcall BeamOrientTrack_End(uint8_t* self, re::Edx) {
    Ped_SetHeading(OwnerPed(At<uint8_t*>(self, 0x24)), At<float>(self, 0x2C));
    return 1;
}

// 0x611DB0 (thiscall): SetMotionIntentionTrack::Start: the ped's intended direction (its
// +0x2E0 block) +0x18 off its flat forward
uint8_t __fastcall SetMotionIntentionTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 1;
    }
    V3 forward;
    Placeable_GetForward(ped, &forward);
    forward.z = 0.0f;
    Vector_Normalise(&forward);
    float heading = HeadingFromVector(&forward);
    float offset = LimitAngle(At<float>(self, 0x18));
    V3 dir;
    V3* d = Vector_FromAngle(&dir, offset + heading);
    float* intention = At<float*>(ped, 0x2E0);
    intention[0] = d->x;
    intention[1] = d->y;
    intention[2] = d->z;
    return 1;
}

} // namespace

#define ACTTRACKSWORLD_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

ACTTRACKSWORLD_REVERSED("AddAttachedEffectTrack::Start", 0x006101F0, &AddAttachedEffectTrack_Start);
ACTTRACKSWORLD_REVERSED("DialogAddAttachedEffectTrack::Update", 0x00610210, &DialogTrack_Update<0x006097B0>);
ACTTRACKSWORLD_REVERSED("BreathTrack::Emit", 0x006102A0, &BreathTrack_Emit);
ACTTRACKSWORLD_REVERSED("BreathTrack::Update", 0x00610430, &BreathTrack_Update);
ACTTRACKSWORLD_REVERSED("FXCeilingTrack::Start", 0x00610450, &FXCeilingTrack_Start);
ACTTRACKSWORLD_REVERSED("FXDecalTrack::Place", 0x006105A0, &DecalTrack_Place);
ACTTRACKSWORLD_REVERSED("FXDecalTrack::Start", 0x00610870, &FXDecalTrack_Start);
ACTTRACKSWORLD_REVERSED("DialogFXDecalTrack::Update", 0x00610890, &DialogTrack_Update<0x006105A0>);
ACTTRACKSWORLD_REVERSED("FXWaterRipplesTrack::WaterLevel", 0x00610920, &FXWaterRipplesTrack_WaterLevel);
ACTTRACKSWORLD_REVERSED("GroundImpactTrack::Start", 0x00610970, &GroundImpactTrack_Start);
ACTTRACKSWORLD_REVERSED("WallImpactTrack::Start", 0x00610C10, &WallImpactTrack_Start);
ACTTRACKSWORLD_REVERSED("CameraFreezeTrack::Start", 0x00610ED0, &CameraFreezeTrack_Start);
ACTTRACKSWORLD_REVERSED("ActivateDoorTrack::Start", 0x00611030, &ActivateDoorTrack_Start);
ACTTRACKSWORLD_REVERSED("RailDriverTrack::End", 0x006111A0, &RailDriverTrack_End);
ACTTRACKSWORLD_REVERSED("SetInStealthPropTrack::Start", 0x006111B0, &SetInStealthPropTrack_Start);
ACTTRACKSWORLD_REVERSED("NotifyOnSightPlayerTrack::Start", 0x00611240, &NotifyTrack_Start<0x18E0>);
ACTTRACKSWORLD_REVERSED("NotifyOnHearTrack::Start", 0x006112B0, &NotifyTrack_Start<0x18E4>);
ACTTRACKSWORLD_REVERSED("NotifyOnMissionCriticalTrack::Start", 0x00611320, &NotifyOnMissionCriticalTrack_Start);
ACTTRACKSWORLD_REVERSED("AcceptGiftTrack::Start", 0x006113B0, &AcceptGiftTrack_Start);
ACTTRACKSWORLD_REVERSED("RemoveItemTrack::Start", 0x00611480, &RemoveItemTrack_Start);
ACTTRACKSWORLD_REVERSED("SetCanAcceptGiftTrack::Start", 0x00611530, &SetCanAcceptGiftTrack_Start);
ACTTRACKSWORLD_REVERSED("SetOverheadIconTrack::Start", 0x00611570, &SetOverheadIconTrack_Start);
ACTTRACKSWORLD_REVERSED("StatSetAsIntTrack::Start", 0x006115C0, &StatSetAsIntTrack_Start);
ACTTRACKSWORLD_REVERSED("StatSetAsFloatTrack::Start", 0x00611620, &StatSetAsFloatTrack_Start);
ACTTRACKSWORLD_REVERSED("StatAddToIntTrack::Start", 0x00611680, &StatAddToIntTrack_Start);
ACTTRACKSWORLD_REVERSED("StatAddToFloatTrack::Start", 0x006116E0, &StatAddToFloatTrack_Start);
ACTTRACKSWORLD_REVERSED("BalanceMeterTrack::Start", 0x00611740, &BalanceMeterTrack_Start);
ACTTRACKSWORLD_REVERSED("BalanceMeterTrack::Update", 0x006117A0, &BalanceMeterTrack_Update);
ACTTRACKSWORLD_REVERSED("BalanceMeterTrack::End", 0x00611950, &BalanceMeterTrack_End);
ACTTRACKSWORLD_REVERSED("CameraGetParametricTrack::Update", 0x00611970, &CameraGetParametricTrack_Update);
ACTTRACKSWORLD_REVERSED("HeadtrackGetParametricTrack::Update", 0x006119D0, &HeadtrackGetParametricTrack_Update);
ACTTRACKSWORLD_REVERSED("OffsetCameraMirror180Track::Start", 0x00611A20, &OffsetCameraMirror180Track_Start);
ACTTRACKSWORLD_REVERSED("OffsetCameraMirror180Track::Update", 0x00611A50, &OffsetCameraMirror180Track_Update);
ACTTRACKSWORLD_REVERSED("ActionTracks::FindPlayingAnimation_611A90", 0x00611A90, &FindPlayingAnimation);
ACTTRACKSWORLD_REVERSED("ActionTracks::GetBone_611B40", 0x00611B40, &Track_GetBone);
ACTTRACKSWORLD_REVERSED("BeamOrientToSafeHeadingTrack::Start", 0x00611BC0, &BeamOrientTrack_Start);
ACTTRACKSWORLD_REVERSED("BeamOrientToSafeHeadingTrack::Update", 0x00611C30, &BeamOrientTrack_Update);
ACTTRACKSWORLD_REVERSED("BeamOrientToSafeHeadingTrack::End", 0x00611D90, &BeamOrientTrack_End);
ACTTRACKSWORLD_REVERSED("SetMotionIntentionTrack::Start", 0x00611DB0, &SetMotionIntentionTrack_Start);
