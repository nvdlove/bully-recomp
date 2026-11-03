// The action tree's conditions on props (the prop the owner uses: 0x4EABB0), 0x654F90..
// 0x655620: their Evaluate(owner) (vf1, thiscall), destructors and field defaults (the
// New functions call these after construction), and a bone lookup on a prop's skeleton.

#include <cmath>
#include <cstdint>

#include "game/ActionFactories.h"
#include "game/ActionTracksCommon.h"

using namespace actc;
namespace af = actfac;

namespace {

GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, uint8_t* owner);   // the owner's prop
GAME_FN(float, __thiscall, PropAnim_GetHealthFraction, 0x004EDD80, uint8_t* prop);
GAME_FN(void, __thiscall, PropAnim_4ED940, 0x004ED940, uint8_t* prop);
GAME_FN(uint8_t, __thiscall, Ctl_IsPlaying, 0x005F4800, uint8_t* controller, const void* name, int ancestors, int nested);
GAME_FN(int, __cdecl, GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);   // +4 the load state (1 loaded, 2 requested)
GAME_FN(uint8_t, __cdecl, Streaming_IsLoading, 0x00450C00, int id);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __thiscall, PropFinder_Setup, 0x00499330, uint8_t* finder, float range, int flag, float height, int);
GAME_FN(uint8_t, __thiscall, PropFinder_Find, 0x00497B00, uint8_t* finder, int, float range, int, int, int, int);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, uint8_t* entity, int node, float* transform, int flag);
GAME_FN(int, __cdecl, BoneTable_Get, 0x00520380, int index);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const V3* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(void*, __thiscall, ElementTable_FindById, 0x00465580, void* table, int id, int* outIndex);

GAME_VAR(int32_t, weaponModelFirst, 0x00A136A0);   // the model range GetModelIndexByHash searches
GAME_VAR(int32_t, weaponModelLast, 0x00A136A4);
GAME_VAR(uint8_t*, missionPool, 0x020C7C48);       // a pool: +0 its objects, +4 their flags (0x80: free)

// a node's transform (NiTransform: the scale is set to 1 before asking)
struct NodeTransform {
    float m[9];   // +0x0
    V3 pos;       // +0x24
    float scale;  // +0x30
};
static_assert(sizeof(NodeTransform) == 0x34);

// 0x655030 (thiscall): the prop's health fraction compared (+4 the value, +8 the operator)
uint8_t __fastcall PropHealth_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (!prop) {
        return 0;
    }
    return CompareOp(PropAnim_GetHealthFraction(prop), At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x655090 (thiscall): the prop's controller (+0x128) plays the node (+4, +8 its ancestors
// too)
uint8_t __fastcall PropIsPlaying_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (prop) {
        PropAnim_4ED940(prop);
        if (Ctl_IsPlaying(prop + 0x128, self + 4, self[8], 1)) {
            return 1;
        }
    }
    return 0;
}

// 0x6550F0 (thiscall): the prop's flag (+4 its index, from the prop's +0x160) is set
uint8_t __fastcall PropFlags_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (prop && At<int32_t>(prop, 0x160 + At<int32_t>(self, 4) * 4) != 0) {
        return 1;
    }
    return 0;
}

// 0x655170 (thiscall): the prop's tagged fraction (+0x254) compared (the prop not tested
// for null)
uint8_t __fastcall PropTagPercentage_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    return CompareOp(At<float>(prop, 0x254), At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x6551C0 (thiscall): +8 set: the prop has its skeleton; else the model named by +4 is
// loaded (requested when not loaded or on its way)
uint8_t __fastcall PropModelLoaded_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (self[8] != 0) {
        uint8_t* prop = UsePoint_GetEntity(owner);
        if (prop) {
            uint8_t* anim = At<uint8_t*>(prop, 0x114);
            if (anim && At<void*>(anim, 0x94)) {
                return 1;
            }
        }
        return 0;
    }
    int model = GetModelIndexByHash(At<uint32_t>(self, 4), weaponModelFirst, weaponModelLast + 1);
    if (GetStreamingInfo(model)[4] == 1) {
        return 1;
    }
    if (GetStreamingInfo(model)[4] != 2 && !Streaming_IsLoading(model)) {
        RequestModel(model, 0x10);
    }
    return 0;
}

// 0x655270 (thiscall): the ped's prop finder (+0x34C) finds the prop wanted (+8 its kind,
// +4 the range; +0xC..+0x17 the search's flags). A ped in state 0xD with range 0 skips the
// search's set-up; one already using something (+0x155C) finds none unless +0x17
uint8_t __fastcall TargetInteractiveProp_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    if (!(At<int32_t>(ped, 0x1310) == 0xD && self[0xC] == 0 && At<float>(self, 4) == 0.0f)) {
        PropFinder_Setup(ped + 0x34C, At<float>(self, 4), self[0xC], At<float>(self, 0x10), self[0x14]);
    }
    if (At<int32_t>(ped, 0x155C) != 0 && self[0x17] == 0) {
        return 0;
    }
    return PropFinder_Find(ped + 0x34C, At<int32_t>(self, 8), At<float>(self, 4), -1, 0, self[0x15], self[0x16]);
}

// 0x655300 (thiscall): the player's prop finder told to show the use text; true
uint8_t __fastcall PropDisplayUseText_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    playerPed[0x34C] = 1;
    return 1;
}

// 0x655320 (thiscall): someone uses the prop (+0x234)
uint8_t __fastcall PropPedIsUsingMe_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (!prop || At<void*>(prop, 0x234) == nullptr) {
        return 0;
    }
    return 1;
}

// 0x655350 (thiscall): the prop's unique id is +4: its owner's (+0x23C, +0x2C) when it
// has one other than the pool's first object, else its own (+0x3AC)
uint8_t __fastcall PropUniqueID_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (!prop) {
        return 0;
    }
    uint8_t* holder = At<uint8_t*>(prop, 0x23C);
    if (holder) {
        uint8_t* first = (*At<uint8_t*>(missionPool, 4) & 0x80) ? nullptr : At<uint8_t*>(missionPool, 0);
        if (holder != first) {
            return At<int32_t>(holder, 0x2C) == At<int32_t>(self, 4);
        }
    }
    return At<int32_t>(prop, 0x3AC) == At<int32_t>(self, 4);
}

// 0x6553B0 (thiscall): the ped is near the prop's socket. +0x14 set: of the bones at
// +0x18/+0x1C/+0x20 (the ones set, at least two) the first is the nearest; else within +4
// of the bone (+0x10, or the table's +0xC), only in height when +8
uint8_t __fastcall PedNearPropSocket_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (!ped || !prop) {
        return 0;
    }
    NodeTransform transform;
    transform.scale = 1.0f;
    V3 d;
    if (self[0x14] != 0) {
        int bones[3];
        int count = 0;
        for (int at = 0x18; at <= 0x20; at += 4) {
            if (At<int32_t>(self, at) != 0) {
                bones[count++] = At<int32_t>(self, at);
            }
        }
        if (count <= 1) {
            return 0;
        }
        V3 pedPos = *reinterpret_cast<V3*>(Pos(ped));
        if (!GetNodeTransform(prop, bones[0], transform.m, 1)) {
            return 0;
        }
        float first = Vector_MagnitudeSqr(VectorSub(&d, &transform.pos.x, &pedPos.x));
        float nearest = first;
        for (int i = 1; i < count; i++) {
            if (!GetNodeTransform(prop, bones[i], transform.m, 1)) {
                return 0;
            }
            VectorSub(&d, &transform.pos.x, &pedPos.x);
            float dist = d.y * d.y + d.x * d.x + d.z * d.z;
            if (!(nearest < dist)) {
                nearest = dist;
            }
        }
        return nearest >= first ? 1 : 0;
    }
    int bone = At<int32_t>(self, 0x10);
    if (bone == 0) {
        bone = BoneTable_Get(At<int32_t>(self, 0xC));
    }
    if (!GetNodeTransform(prop, bone, transform.m, 1)) {
        return 0;
    }
    float dist;
    if (self[8] != 0) {
        dist = std::fabs(transform.pos.z - Pos(ped)[2]);
    } else {
        VectorSub(&d, &transform.pos.x, Pos(ped));
        dist = Vector_Magnitude(&d);
    }
    return At<float>(self, 4) >= dist ? 1 : 0;
}

// 0x6555E0 (thiscall): the prop's model's hash (its model info's +4) is +4
uint8_t __fastcall PropModelID_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (prop && At<int32_t>(modelInfoPtrs[At<int16_t>(prop, 0x10E)], 4) == At<int32_t>(self, 4)) {
        return 1;
    }
    return 0;
}

// 0x655620 (cdecl): the index of the bone with the id in the entity's skeleton (+0x114,
// its table +0x94); -1 when none
int __cdecl Prop_FindBone(uint8_t* entity, int id) {
    int index = 0;
    uint8_t* anim = At<uint8_t*>(entity, 0x114);
    if (anim && At<void*>(anim, 0x94)) {
        if (ElementTable_FindById(At<void*>(anim, 0x94), id, &index) && index != -1) {
            return index;
        }
    }
    return -1;
}

} // namespace

// (variadic: the template arguments' commas)
#define PROPCOND_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

using af::Construct;
using af::Fields;
using af::Vt;
using af::KeyDtor;
using af::Call;
using af::F32;
using af::I32;
using af::U8;

// field defaults (thiscall, after construction)
PROPCOND_REVERSED("ConditionTargetInteractiveProp::InitFields", 0x00654F90, &Fields<F32(0x4, 2.0f), U8(0xC, 0), F32(0x10, 2.0f), U8(0x14, 0), U8(0x15, 0), U8(0x16, 0), U8(0x17, 0)>);
PROPCOND_REVERSED("ConditionPedNearPropSocket::InitFields", 0x00654FC0, &Fields<F32(0x4, 2.0f), I32(0xC, 0), U8(0x8, 0), U8(0x14, 0)>);
PROPCOND_REVERSED("ConditionPropHealth::InitFields", 0x00655010, &Fields<I32(0x8, 5), F32(0x4, 1.0f)>);
PROPCOND_REVERSED("ConditionPropTagPercentage::InitFields", 0x00655150, &Fields<F32(0x4, 0.0f)>);  // also ConditionFactionNear

// destructors (thiscall)
PROPCOND_REVERSED("ConditionTargetInteractiveProp::Destruct", 0x00654FB0, &Fields<Vt(0x00936838), KeyDtor(0x8)>);
PROPCOND_REVERSED("ConditionPedNearPropSocket::Destruct", 0x00654FE0, &Fields<Vt(0x0093684C), KeyDtor(0x20), KeyDtor(0x1C), KeyDtor(0x18), KeyDtor(0x10)>);
PROPCOND_REVERSED("ConditionPropHealth::Destruct", 0x00655020, &Fields<Vt(0x00936860)>);
PROPCOND_REVERSED("ConditionPropIsPlaying::Destruct", 0x00655080, &Fields<Vt(0x00938358), Call(0x005FC2E0)>);
PROPCOND_REVERSED("ConditionPropFlags::Destruct", 0x006550E0, &Fields<Vt(0x00936874)>);
PROPCOND_REVERSED("ConditionPropModelID::Destruct", 0x00655120, &Fields<Vt(0x00937CAC), KeyDtor(0x4)>);
PROPCOND_REVERSED("ConditionPropDisplayUseText::Destruct", 0x00655130, &Fields<Vt(0x00937D24)>);
PROPCOND_REVERSED("ConditionPropPedIsUsingMe::Destruct", 0x00655140, &Fields<Vt(0x00937D88)>);
PROPCOND_REVERSED("ConditionPropTagPercentage::Destruct", 0x00655160, &Fields<Vt(0x00936888)>);
PROPCOND_REVERSED("ConditionPropModelLoaded::Destruct", 0x006551B0, &Fields<Vt(0x0093689C), KeyDtor(0x4)>);
PROPCOND_REVERSED("ConditionPropUniqueID::Destruct", 0x00655260, &Fields<Vt(0x00937CC0), KeyDtor(0x4)>);

// Evaluate (vf1, thiscall, the owner)
PROPCOND_REVERSED("ConditionPropHealth::Evaluate", 0x00655030, &PropHealth_Evaluate);
PROPCOND_REVERSED("ConditionPropIsPlaying::Evaluate", 0x00655090, &PropIsPlaying_Evaluate);
PROPCOND_REVERSED("ConditionPropFlags::Evaluate", 0x006550F0, &PropFlags_Evaluate);
PROPCOND_REVERSED("ConditionPropTagPercentage::Evaluate", 0x00655170, &PropTagPercentage_Evaluate);
PROPCOND_REVERSED("ConditionPropModelLoaded::Evaluate", 0x006551C0, &PropModelLoaded_Evaluate);
PROPCOND_REVERSED("ConditionTargetInteractiveProp::Evaluate", 0x00655270, &TargetInteractiveProp_Evaluate);
PROPCOND_REVERSED("ConditionPropDisplayUseText::Evaluate", 0x00655300, &PropDisplayUseText_Evaluate);
PROPCOND_REVERSED("ConditionPropPedIsUsingMe::Evaluate", 0x00655320, &PropPedIsUsingMe_Evaluate);
PROPCOND_REVERSED("ConditionPropUniqueID::Evaluate", 0x00655350, &PropUniqueID_Evaluate);
PROPCOND_REVERSED("ConditionPedNearPropSocket::Evaluate", 0x006553B0, &PedNearPropSocket_Evaluate);
PROPCOND_REVERSED("ConditionPropModelID::Evaluate", 0x006555E0, &PropModelID_Evaluate);

PROPCOND_REVERSED("Prop::FindBone_655620", 0x00655620, &Prop_FindBone);
