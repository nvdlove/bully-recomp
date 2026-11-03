// CPropAnim (vtable 0x90626C, from the prop pool 0xC0F608): its constructor and
// destructor, the child props sharing its animation (+0x280 a reference array, +0x284
// its capacity, +0x288 the count), its use-point model, its damage left, opening and
// closing (+0x184 open, +0x3B0 how far, +0x254 the closing speed), and the vehicles it
// crushes when it shuts.

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

struct PoolHeader {
    re::Ptr32<uint8_t> objects;   // +0
    re::Ptr32<uint8_t> flags;     // +4
    int32_t size;       // +8
    int32_t elemSize;   // +0xC
    int32_t unknown10;  // +0x10
    int32_t lastUsed;   // +0x14
};

// an array of registered references made with new[] (its count before it)
struct RefArray {
    re::Ptr32<re::Ptr32<void>> refs;        // +0
    uint32_t capacity;  // +4
    uint32_t count;     // +8
};
static_assert(sizeof(RefArray) == 0xC);

GAME_VAR(uint8_t*, pc_playerPed, 0x00C1AEA8);
GAME_ARRAY(uint8_t*, pc_modelInfoPtrs, 0x00C67738);
GAME_VAR(PoolHeader*, pc_propPool, 0x00C0F608);
GAME_VAR(PoolHeader*, pc_vehiclePool, 0x00C0F5F4);
GAME_VAR(PoolHeader*, pc_railPool, 0x020C7C98);
GAME_VAR(void*, pc_radar, 0x00C674CC);
GAME_VAR(void*, pc_effects, 0x00B6F1D4);
GAME_VAR(uint16_t, pc_crushEffect, 0x020C9010);
GAME_VAR(int32_t, pc_specialModel0, 0x00BF4488);
GAME_VAR(int32_t, pc_specialModel1, 0x00BF447C);
GAME_VAR(int32_t, pc_specialModel2, 0x00BF4484);
GAME_VAR(int32_t, pc_triggerModel, 0x00BF4470);
GAME_VAR(int32_t, pc_noCrushModel, 0x00BF42C8);
GAME_VAR(int32_t, pc_poiModel0, 0x00BF42DC);
GAME_VAR(int32_t, pc_poiModel1, 0x00BF42D4);
GAME_VAR(int32_t, pc_poiModel2, 0x00BF42D0);
GAME_VAR(int32_t, pc_poiModel3, 0x00BF42CC);
GAME_VAR(int32_t, pc_numTriggerProps, 0x020C8AC4);
GAME_VAR(float, pc_timeStep, 0x00C1A9A4);
GAME_VAR(float, pc_closeSpeed, 0x00A453E8);
GAME_VAR(int32_t, pc_closedAnim, 0x00C8E908);
GAME_VAR(const char*, pc_openAnimName, 0x00A76B58);
GAME_VAR(const char*, pc_openFullyAnimName, 0x00A76B5C);
GAME_VAR(const char*, pc_closeAnimName, 0x00A76B60);
GAME_ARRAY(uint8_t*, pc_specialProps, 0x00C3B7F0);
inline void* const pc_vtable = reinterpret_cast<void*>(0x0090626C);
inline void* const pc_soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const pc_sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const pc_renderer = reinterpret_cast<void*>(0x020C4910);
inline void* const pc_triggers = reinterpret_cast<void*>(0x020C8A00);
inline void* const pc_doors = reinterpret_cast<void*>(0x00C3B808);
inline void* const pc_closingInput = reinterpret_cast<void*>(0x01B654E4);
inline void* const pc_openInput = reinterpret_cast<void*>(0x01B652D0);
inline void* const pc_closeInput = reinterpret_cast<void*>(0x01B652D4);
inline const char* const pc_closeSound = reinterpret_cast<const char*>(0x00913E18);   // "SCDOOR_Close"

GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(uint8_t*, __thiscall, ModelInfo_GetAnimInfo, 0x005201D0, uint8_t* modelInfo);
GAME_FN(uint8_t*, __thiscall, ModelInfo_GetDamageData, 0x00776D50, uint8_t* modelInfo);
GAME_FN(void, __thiscall, DamageData_Prepare, 0x007283C0, uint8_t* data);
GAME_FN(int, __thiscall, PAnimModelInfo_GetStageAnim, 0x00520420, uint8_t* modelInfo, int stage);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsHidden, 0x00520430, uint8_t* modelInfo);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsKind1, 0x00520440, uint8_t* modelInfo);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsKind2, 0x00520460, uint8_t* modelInfo);
GAME_FN(void, __cdecl, RemoveModel, 0x0052F640, int index);
GAME_FN(void, __cdecl, CatStore_Remove, 0x0051B980, int index);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<void>* ref, void* entity);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __thiscall, RefArray_Resize, 0x00449F80, RefArray* array, uint32_t capacity, int);
GAME_FN(void, __thiscall, RefArray_Destruct, 0x0044A6E0, RefArray* array);
GAME_FN(void, __thiscall, Renderer_RemoveAnim, 0x006C2200, void* renderer, void* anim, void* handle);
GAME_FN(void, __thiscall, Entity_Construct, 0x004656A0, void* entity);
GAME_FN(void, __thiscall, Entity_Destruct, 0x00467E60, void* entity);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(void, __thiscall, Controller_Construct, 0x005F3E50, void* controller);
GAME_FN(void, __thiscall, Controller_Destruct, 0x005F40F0, void* controller);
GAME_FN(void, __thiscall, Controller_Stop, 0x005F40A0, void* controller);
GAME_FN(uint8_t, __thiscall, Controller_IsNodePlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(void, __thiscall, HitAnim_Construct, 0x005F3C10, void* state);
GAME_FN(void, __thiscall, AnimObject_Construct, 0x006BA240, void* animObject);
GAME_FN(void, __thiscall, AnimObject_Destruct, 0x006B5A80, void* animObject);
GAME_FN(void, __thiscall, Block160_Clear, 0x004F0210, void* block);
GAME_FN(void, __thiscall, Physical_SetInitialVelocity, 0x0046DDF0, void* entity);
GAME_FN(void, __thiscall, PropAnim_ApplyChapterSetting, 0x004EA660, void* prop);
GAME_FN(uint8_t*, __cdecl, GetObjectInfo, 0x004D0CA0, int model);
GAME_FN(void, __thiscall, SoundBanks_Unload, 0x0059A420, void* banks, int bank, int, int);
GAME_FN(void, __thiscall, Radar_RemoveBlipsFor, 0x0054A890, void* radar, int kind, int handle);
GAME_FN(void, __thiscall, Triggers_Remove, 0x0072C510, void* triggers);
GAME_FN(void, __cdecl, PropAnim_Unregister, 0x005B4C90, void* prop);
GAME_FN(void, __cdecl, AddWeaponModelRefs, 0x004DA680, int weapon, int remove);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, re::Ptr32<void>* sound, uintptr_t id);
GAME_FN(int, __thiscall, Sounds_Find, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVec* pos, int type, uintptr_t, int, int,
        int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void*, __cdecl, ActTree_FindNode, 0x005F5B10, const char* name, void* node);
GAME_FN(void, __cdecl, PropAnim_OnOpen, 0x005B2F50, void* prop);
GAME_FN(void, __cdecl, PropAnim_OnClose, 0x005B3190, void* prop);
GAME_FN(uint8_t*, __thiscall, ObjectiveQueue_GetCurrent, 0x00470340, void* queue);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(CVec*, __thiscall, Placeable_GetPosition, 0x00415460, void* placeable);
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const void* matrix, void* out);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412720, void* matrix, const void* from);
GAME_FN(void, __thiscall, Matrix_Multiply, 0x00412800, void* matrix, const void* by);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVec* Matrix_TransformPoint(CVec* out, const void* m, const CVec* v) { return re::CallVec16<0x0041A9A0, CVec*>(out, m, v); }
GAME_FN(uint8_t*, __cdecl, GetColModel, 0x0051AF50, void* entity);
GAME_FN(uint8_t, __cdecl, SphereInBox, 0x00564B50, const float* sphere, const CVec* boxMin, const CVec* boxMax);
GAME_FN(void*, __thiscall, VehicleSeats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(CVec*, __cdecl, VectorSub, 0x00414B70, CVec* out, const CVec* a, const CVec* b);
GAME_FN(void, __thiscall, Proxy_Construct, 0x00668E90, void* proxy);
GAME_FN(void, __thiscall, Proxy_Destruct, 0x00668EB0, void* proxy);
GAME_FN(void, __thiscall, Proxy_SetPosition, 0x00669040, void* proxy, const CVec* pos, int);
GAME_FN(void, __thiscall, Effects_Start, 0x00668290, void* effects, const void* type, void* proxy, int);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
// (the door manager's and the prop's own, through their addresses)
GAME_FN(void, __thiscall, DM_SetState0, 0x004E99F0, void* doors, int id, int value);
GAME_FN(void, __thiscall, DM_SetState1, 0x004E9AC0, void* doors, int id, int value);
GAME_FN(void, __thiscall, DM_Hold, 0x004E9600, void* doors, void* door);
GAME_FN(void, __thiscall, DM_Shut, 0x004E96A0, void* doors, void* door);
GAME_FN(uint8_t, __thiscall, DM_AddPush, 0x004E9CB0, void* doors, void* ped, void* door);
GAME_FN(void, __thiscall, Prop_StopUsers, 0x004EAC30, void* prop, int own);
GAME_FN(void, __thiscall, Prop_FireTrigger, 0x004EDEB0, void* prop, int force);
GAME_FN(void, __thiscall, Prop_SetAnim, 0x004EAEC0, void* prop, void* anim, int animId, uint32_t bone);
GAME_FN(void, __thiscall, Prop_StartAnim, 0x004EA4C0, void* prop);
GAME_FN(uint32_t, __thiscall, Prop_FindChildByAnim, 0x004EAF20, void* prop, int animId);
GAME_FN(uint32_t, __thiscall, Prop_FreeChildSlot, 0x004EDA20, void* prop);
GAME_FN(void, __thiscall, RefArray_Free, 0x004EB0A0, RefArray* array);
GAME_FN(void, __thiscall, RefArray_RemoveAt, 0x004EDAC0, RefArray* array, uint32_t index);
GAME_FN(void, __thiscall, Prop_CrushVehicles, 0x004EE690, void* prop);
GAME_FN(void, __thiscall, Prop_UpdateLampGlow, 0x004EB840, void* prop);
GAME_FN(uint8_t, __thiscall, Door_CanPass, 0x004EAA10, void* door, void* ped);
GAME_FN(void, __thiscall, Door_SetCurrent, 0x004ED940, void* door);
GAME_FN(void, __thiscall, Door_SetAnim, 0x004EE010, void* door, void* anim);

const CVec& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVec*>(matrix + 0x30) : At<CVec>(entity, 4);
}

uint8_t* ModelInfoOf(uint8_t* entity) {
    return pc_modelInfoPtrs[At<int16_t>(entity, 0x10E)];
}

// 0x4ED8E0 (thiscall): the prop's use-point model let go once loaded (its users stopped)
void __fastcall ReleaseUseModel(uint8_t* self) {
    int use = At<int16_t>(self, 0x22C);
    if (use == -1) {
        return;
    }
    if (GetStreamingInfo(use + 0x5D41)[4] == 1) {
        ModelInfo_GetAnimInfo(ModelInfoOf(self));
        Prop_StopUsers(self, 0);
        RemoveModel(use + 0x5D41);
        CatStore_Remove(use);
    }
}

// 0x4ED9C0 (thiscall): deletes the child props and frees the array
void __fastcall DeleteChildren(uint8_t* self) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x288); i++) {
        void* child = At<re::Ptr32<void>*>(self, 0x280)[i];
        if (child != nullptr) {
            VCall<void>(child, 0, 1);
            SetRef(&At<re::Ptr32<void>*>(self, 0x280)[i], nullptr);
        }
    }
    RefArray_Free(&At<RefArray>(self, 0x280));
}

// 0x4EDA20 (thiscall): a free child slot (the array grown by one, doubling its capacity
// when full)
uint32_t __fastcall FreeChildSlot(uint8_t* self) {
    RefArray& children = At<RefArray>(self, 0x280);
    for (uint32_t i = 0; i < children.count; i++) {
        if (children.refs[i] == nullptr) {
            return i;
        }
    }
    re::Ptr32<void> none = nullptr;
    SetRef(&none, nullptr);
    if (children.count == children.capacity) {
        RefArray_Resize(&children, children.capacity ? children.capacity * 2 : 1, 0);
    }
    SetRef(&children.refs[children.count], none);
    children.count++;
    SetRef(&none, nullptr);
    return At<uint32_t>(self, 0x288) - 1;
}

// 0x4EDAC0 (thiscall): a reference array's entry removed (the rest moved down; freed
// when empty, halved when under half full)
void __fastcall RemoveAt(RefArray* self, re::Edx, uint32_t index) {
    for (uint32_t i = index; i < self->count - 1; i++) {
        SetRef(&self->refs[i], self->refs[i + 1]);
    }
    if (--self->count == 0) {
        RefArray_Free(self);
    }
    uint32_t half = self->capacity >> 1;
    if (self->count < half) {
        RefArray_Resize(self, half, 0);
    }
}

// 0x4EDB20 (thiscall): the constructor, for a model and its placement record
uint8_t* __fastcall Construct(uint8_t* self, re::Edx, int model, uint8_t* placement) {
    Entity_Construct(self);
    At<void*>(self, 0) = pc_vtable;
    Controller_Construct(self + 0x128);
    Block160_Clear(self + 0x160);
    HitAnim_Construct(self + 0x1BC);
    At<int32_t>(self, 0x230) = 0;
    At<int32_t>(self, 0x234) = 0;
    At<int32_t>(self, 0x240) = 0;
    At<int32_t>(self, 0x280) = 0;
    At<int32_t>(self, 0x284) = 0;
    At<int32_t>(self, 0x288) = 0;
    AnimObject_Construct(self + 0x294);
    At<void*>(self, 0x39C) = nullptr;
    At<int32_t>(self, 0x398) = 0;
    SetRef(&At<void*>(self, 0x39C), nullptr);
    self[0x3A0] = 1;
    At<float>(self, 0x248) = -1.0f;
    At<uint8_t*>(self, 0x23C) = placement;
    At<float>(self, 0x24C) = 0.0f;
    self[0x3B6] = 0;
    At<float>(self, 0x254) = 0.0f;
    At<uint16_t>(self, 0x250) = 0xFF;
    At<int32_t>(self, 0x228) = 0;
    self[0x3A9] = 0;
    At<int32_t>(self, 0x238) = 0;
    self[0x3B7] = 0xFF;
    uint8_t* modelInfo = pc_modelInfoPtrs[model];
    int16_t use = VCall<int16_t>(modelInfo, 0x2C);
    self[0x108] = uint8_t((self[0x108] & 0xFE) | 6);
    At<int16_t>(self, 0x22C) = use;
    At<int32_t>(self, 0x20) = 1;
    At<int32_t>(self, 0x3C) = 1;
    At<int32_t>(self, 0x28) = 1;
    if (PAnimModelInfo_IsHidden(modelInfo)) {
        At<int32_t>(self, 0x20) = 0;
        At<int32_t>(self, 0x3C) = 0;
    }
    At<uint16_t>(self, 0x120) = 0;
    At<uint16_t>(self, 0x122) = 0;
    At<uint8_t*>(self, 0x114) = self + 0x294;
    if (model == pc_specialModel0) {
        At<uint32_t>(self, 0x294 + 0x98) |= 0x10;
        At<uint32_t>(At<uint8_t*>(self, 0x114), 0x98) |= 0x20;
    }
    At<uint8_t*>(self, 0x23C) = placement;
    self[0x244] = 1;
    At<uint16_t>(self, 0x246) = 0;
    At<int32_t>(self, 0x274) = 0;
    if (model == pc_specialModel0) {
        pc_specialProps[0] = self;
    }
    if (model == pc_specialModel2) {
        pc_specialProps[2] = self;
    }
    if (model == pc_specialModel1) {
        pc_specialProps[1] = self;
    }
    uint8_t* animInfo = ModelInfo_GetAnimInfo(modelInfo);
    if (animInfo != nullptr) {
        At<int32_t>(self, 0x168) = animInfo[0x8C];
        if (animInfo[0x86] != 0) {
            self[0x258] = 1;
            self[0x260] = 1;
            At<float>(self, 0x25C) = 0.25f;
        }
    } else {
        self[0x258] = 0;
        At<float>(self, 0x25C) = 1.0f;
    }
    At<int32_t>(self, 0x278) = -1;
    At<int32_t>(self, 0x27C) = -1;
    At<int32_t>(self, 0x28C) = 0;
    At<int32_t>(self, 0x290) = 0;
    if (model == pc_triggerModel) {
        pc_numTriggerProps++;
    }
    if (0.0f != At<float>(placement, 0x14) || 0.0f != At<float>(placement, 0x18)) {
        Physical_SetInitialVelocity(self);
    }
    At<float>(self, 0x3B0) = 0.0f;
    self[0x3A8] = 0;
    At<float>(self, 0x124) = 99999997952.0f;
    At<int32_t>(self, 0x3AC) = 0;
    PropAnim_ApplyChapterSetting(self);
    return self;
}

// 0x4EDD80 (thiscall): the fraction of the prop's damage stages left (0 without any)
float __fastcall GetHealthFraction(uint8_t* self) {
    uint8_t* modelInfo = ModelInfoOf(self);
    ModelInfo_GetAnimInfo(modelInfo);
    ModelInfo_GetDamageData(modelInfo);
    float total = 0.0f;
    float done = 0.0f;
    for (int stage = 1; stage < 11; stage++) {
        if (PAnimModelInfo_GetStageAnim(modelInfo, stage) == -1) {
            continue;
        }
        uint8_t* info = ModelInfoOf(self);
        uint8_t* animInfo = ModelInfo_GetAnimInfo(info);
        uint8_t* data = ModelInfo_GetDamageData(info);
        int health;
        uint16_t fixed = At<uint16_t>(self, 0x120);
        if (fixed != 0) {
            health = fixed;
        } else if (data != nullptr) {
            DamageData_Prepare(data);
            health = At<int32_t*>(data, 0x40)[stage - 1];
        } else {
            health = At<int32_t>(animInfo, 0x1C);
        }
        float value = float(health);
        total = value + total;
        if (stage < int8_t(self[0x244])) {
            done = value + done;
        }
    }
    if (total == 0.0f) {
        return 0.0f;
    }
    float taken = float(double(At<int16_t>(self, 0x246)) + done);
    return float((total - taken) / total);
}

// 0x4EE5B0 (thiscall): a child prop playing this prop's animation object
void __fastcall AddChild(uint8_t* self, re::Edx, int animId, void* child, uint32_t bone) {
    uint32_t slot = Prop_FreeChildSlot(self);
    SetRef(&At<re::Ptr32<void>*>(self, 0x280)[slot], child);
    Prop_SetAnim(At<re::Ptr32<void>*>(self, 0x280)[slot], At<void*>(self, 0x114), animId, bone);
    Prop_StartAnim(At<re::Ptr32<void>*>(self, 0x280)[slot]);
}

// 0x4EE610 (thiscall): the child playing an animation id stopped, deleted and removed
void __fastcall RemoveChild(uint8_t* self, re::Edx, int animId) {
    uint32_t index = Prop_FindChildByAnim(self, animId);
    if (index == 0xFFFFFFFF) {
        return;
    }
    RefArray* children = &At<RefArray>(self, 0x280);
    uint8_t* child = static_cast<uint8_t*>(children->refs[index]);
    if (At<void*>(child, 0x290) != nullptr && At<void*>(child, 0x28C) != nullptr) {
        Renderer_RemoveAnim(pc_renderer, At<void*>(child, 0x290), At<void*>(child, 0x28C));
        At<void*>(child, 0x290) = nullptr;
        At<void*>(child, 0x28C) = nullptr;
    }
    VCall<void>(child, 0, 1);
    SetRef(&children->refs[index], nullptr);
    RefArray_RemoveAt(children, index);
}

// 0x4EE690 (thiscall): empty vehicles within 5 whose collision spheres are inside the
// prop's box are removed with an effect (props of kinds 1 and 2)
void __fastcall CrushVehicles(uint8_t* self) {
    if (!PAnimModelInfo_IsKind1(ModelInfoOf(self)) && !PAnimModelInfo_IsKind2(ModelInfoOf(self))) {
        return;
    }
    if (At<int16_t>(self, 0x10E) == pc_noCrushModel) {
        return;
    }
    PoolHeader* pool = pc_vehiclePool;
    alignas(16) uint8_t inverse[0x48] = {};
    alignas(16) uint8_t local[0x48] = {};
    Matrix_Invert(Placeable_GetMatrix(self), inverse);
    uint8_t* propCol = GetColModel(self);
    CVec* boxMin = reinterpret_cast<CVec*>(propCol + 0x10);
    CVec* boxMax = reinterpret_cast<CVec*>(propCol + 0x20);
    for (int i = 0; i < pool->size; i++) {
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* vehicle = pool->objects + pool->elemSize * i;
        if (vehicle == nullptr || VehicleSeats_GetOccupant(vehicle + 0x1C4, 0) != nullptr) {
            continue;
        }
        const CVec& vehiclePos = Position(vehicle);
        CVec d;
        VectorSub(&d, Placeable_GetPosition(self), &vehiclePos);
        float distSq = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
        if (!(distSq < 25.0)) {
            continue;
        }
        Matrix_Copy(local, inverse);
        Matrix_Multiply(local, Placeable_GetMatrix(vehicle));
        uint8_t* col = GetColModel(vehicle);
        struct { CVec v; float pad; } centrePad16;   // (the game's transform writes a 4th float)
        CVec& centre = centrePad16.v;
        Matrix_TransformPoint(&centre, local, reinterpret_cast<CVec*>(col));
        float sphere[4] = {centre.x, centre.y, centre.z, At<float>(col, 0xC)};
        if (!SphereInBox(sphere, boxMin, boxMax)) {
            continue;
        }
        uint8_t* data = At<uint8_t*>(col, 0x38);
        for (int j = 0; j < int8_t(data[0x1E]); j++) {
            uint8_t* part = At<uint8_t*>(data, 0) + j * 0x14;
            Matrix_TransformPoint(&centre, local, reinterpret_cast<CVec*>(part));
            sphere[0] = centre.x;
            sphere[1] = centre.y;
            sphere[2] = centre.z;
            sphere[3] = float(At<float>(part, 0xC) * 0.949999988079071);
            if (SphereInBox(sphere, boxMin, boxMax)) {
                // (the game builds the effect id in the low half of a used stack slot)
                uint32_t effect = pc_crushEffect;
                uint8_t proxy[8];
                Proxy_Construct(proxy);
                Effects_Start(pc_effects, &effect, proxy, 0);
                Proxy_SetPosition(proxy, &Position(vehicle), 0);
                World_Remove(vehicle);
                VCall<void>(vehicle, 0, 1);
                Proxy_Destruct(proxy);
                break;
            }
        }
    }
}

// 0x4EE950 (thiscall): the destructor
void __fastcall Destruct(uint8_t* self) {
    At<void*>(self, 0) = pc_vtable;
    if (At<int32_t>(self, 0x178) != 0) {
        uint8_t* info = GetObjectInfo(At<int16_t>(self, 0x10E));
        if (info != nullptr && At<int16_t>(info, 0xE) != -1) {
            SoundBanks_Unload(pc_soundBanks, At<int16_t>(info, 0xE), 1, 1);
            At<int32_t>(self, 0x178) = 0;
        }
    }
    uint16_t use = At<uint16_t>(self, 0x22C);
    int changed = 1;
    if (use != 0xFFFF && use == uint16_t(VCall<int16_t>(ModelInfoOf(self), 0x2C))) {
        changed = 0;
    }
    Prop_FireTrigger(self, changed);
    Prop_StopUsers(self, 1);
    Door_SetCurrent(self);
    Controller_Stop(self + 0x128);
    PoolHeader* pool = pc_propPool;
    int slot = int((self - pool->objects) / pool->elemSize);
    Radar_RemoveBlipsFor(pc_radar, 7, (slot << 8) + pool->flags[slot]);
    if (PAnimModelInfo_IsKind1(ModelInfoOf(self))) {
        DM_SetState0(pc_doors, At<int32_t>(At<uint8_t*>(self, 0x23C), 0x2C), At<int32_t>(self, 0x17C) != 0);
        DM_SetState1(pc_doors, At<int32_t>(At<uint8_t*>(self, 0x23C), 0x2C), At<int32_t>(self, 0x180) != 0);
    }
    Entity_DeleteRwObject(self);
    At<int32_t>(At<uint8_t*>(self, 0x23C), 0x1C) = 0;
    At<void*>(self, 0x114) = nullptr;
    uint8_t index = self[0x245];
    if (index != 0) {
        PoolHeader* rails = pc_railPool;
        int railSlot = index - 1;
        void* rail = (rails->flags[railSlot] & 0x80) ? nullptr : rails->objects + rails->elemSize * railSlot;
        if (rail != nullptr) {
            VCall<void>(rail, 0, 1);
        }
        self[0x245] = 0;
    }
    if (At<int16_t>(self, 0x10E) == pc_triggerModel) {
        Triggers_Remove(pc_triggers);
    }
    PropAnim_Unregister(self);
    int weapon = At<int32_t>(ModelInfo_GetAnimInfo(ModelInfoOf(self)), 0x88);
    if (weapon != -1 && At<int32_t>(self, 0x1B0) != 0) {
        AddWeaponModelRefs(weapon, 1);
        At<int32_t>(self, 0x1B0) = 0;
    }
    SetRef(&At<void*>(self, 0x39C), nullptr);
    AnimObject_Destruct(self + 0x294);
    RefArray_Destruct(&At<RefArray>(self, 0x280));
    Sound_Set(&At<void*>(self, 0x240), 0);
    if (At<void*>(self, 0x234) != nullptr) {
        CleanUpOldReference(At<void*>(self, 0x234), &At<void*>(self, 0x234));
    }
    if (At<void*>(self, 0x1C0) != nullptr) {
        CleanUpOldReference(At<void*>(self, 0x1C0), &At<void*>(self, 0x1C0));
    }
    Controller_Destruct(self + 0x128);
    Entity_Destruct(self);
}

// 0x4EEBA0 (thiscall): a closing door swings shut (its speed growing); once shut it plays
// its sound and closed animation; the glow is updated either way
void __fastcall UpdateClosing(uint8_t* self) {
    if (At<int32_t>(ModelInfoOf(self), 0x80) == 0) {
        int32_t visible = At<int32_t>(self, 0x184) != 0 ? 0 : 1;
        At<int32_t>(self, 0x20) = visible;
        At<int32_t>(self, 0x3C) = visible;
    }
    Door_SetCurrent(self);
    if (Controller_IsNodePlaying(self + 0x128, pc_closingInput, 1, 1)) {
        float step = pc_closeSpeed * pc_timeStep;
        At<float>(self, 0x3B0) = float(At<float>(self, 0x3B0) - double(step) * At<float>(self, 0x254));
        At<float>(self, 0x254) = float(At<float>(self, 0x254) + 0.07500000298023224);
        if (At<int32_t>(self, 0x17C) != 0) {
            DM_Hold(pc_doors, self);
        }
        if (At<float>(self, 0x3B0) <= 0.0f) {
            At<float>(self, 0x3B0) = 0.0f;
            if (At<int32_t>(self, 0x184) != 0) {
                const CVec* pos = &Position(self);
                int id = Sounds_Find(pc_sounds, pc_closeSound);
                Sounds_PlayAt(pc_sounds, id, pos, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
            }
            Door_SetAnim(self, reinterpret_cast<void*>(intptr_t(pc_closedAnim)));
            Prop_UpdateLampGlow(self);
            return;
        }
        if (At<int32_t>(self, 0x194) != 0) {
            DM_AddPush(pc_doors, pc_playerPed, self);
        }
        DM_Shut(pc_doors, self);
    }
    Prop_UpdateLampGlow(self);
}

// the checks shared by opening for a ped (0: it may not)
uint8_t PedMayOpen(uint8_t* self, uint8_t* ped) {
    int type = At<int32_t>(ped, 0x1310);
    if (type == 0xD) {
        if (At<int32_t>(self, 0x17C) != 0) {
            return 0;
        }
    } else {
        if (At<int32_t>(self, 0x180) != 0) {
            return 0;
        }
        if (type != 0xD && VCall<uint8_t*>(ped, 0x40)[0x1D] != 0) {
            return 0;
        }
    }
    int model = At<int16_t>(ped, 0x10E);
    if (model == pc_poiModel0 || model == pc_poiModel1 || model == pc_poiModel2 || model == pc_poiModel3) {
        return 0;
    }
    if (ObjectiveQueue_GetCurrent(ped + 0x5F0) != nullptr && !Door_CanPass(self, ped)) {
        return 0;
    }
    type = At<int32_t>(ped, 0x1310);
    if (type != 0xD) {
        if (At<void*>(ped, 0x155C) != nullptr) {
            return 0;
        }
        if (type != 0xD) {
            return 1;
        }
    }
    uint8_t* other = At<uint8_t*>(ped, 0x155C);
    if (other != nullptr && At<int32_t>(other, 0x1DD4) == 0) {
        return 0;
    }
    return 1;
}

// 0x4EED10 (thiscall): opens the door (one way or the other, fully at once or not) for a
// ped, or for no one; 0 when it's open already or the ped may not
uint8_t __fastcall Open(uint8_t* self, re::Edx, int direction, uint8_t* ped, int fully) {
    if (At<int32_t>(self, 0x184) != 0) {
        return 0;
    }
    if (ped != nullptr && !PedMayOpen(self, ped)) {
        return 0;
    }
    Door_SetCurrent(self);
    if (!Controller_IsNodePlaying(self + 0x128, pc_openInput, 1, 1)) {
        bool full = uint8_t(fully) != 0;
        void* anim = ActTree_FindNode(full ? pc_openFullyAnimName : pc_openAnimName, nullptr);
        if (anim != nullptr) {
            if (full) {
                At<float>(self, 0x3B0) = 1.0f;
            }
            if (At<int32_t>(ModelInfoOf(self), 0x80) == 0) {
                At<int32_t>(self, 0x20) = 0;
                At<int32_t>(self, 0x3C) = 0;
            }
            At<int32_t>(self, 0x160) = uint8_t(direction);
            At<int32_t>(self, 0x19C) = uint8_t(direction);
            At<int32_t>(self, 0x184) = 1;
            PropAnim_OnOpen(self);
            Door_SetAnim(self, anim);
            if (full) {
                VCall<void>(self, 0x4C);
            }
        }
    }
    return 1;
}

// 0x4EEEC0 (thiscall): closes it (crushing what's in the way); 0 when it's shut already
// or the ped may not
uint8_t __fastcall Close(uint8_t* self, re::Edx, uint8_t* ped) {
    if (At<int32_t>(self, 0x184) == 0) {
        return 0;
    }
    if (ped != nullptr) {
        if (At<int32_t>(ped, 0x1310) == 0xD) {
            if (At<int32_t>(self, 0x17C) != 0) {
                return 0;
            }
        } else if (At<int32_t>(self, 0x180) != 0) {
            return 0;
        }
    }
    Door_SetCurrent(self);
    if (!Controller_IsNodePlaying(self + 0x128, pc_closeInput, 1, 1)) {
        void* anim = ActTree_FindNode(pc_closeAnimName, nullptr);
        if (anim != nullptr) {
            if (At<int32_t>(ModelInfoOf(self), 0x80) == 0) {
                At<int32_t>(self, 0x20) = 1;
                At<int32_t>(self, 0x3C) = 1;
                Prop_CrushVehicles(self);
            }
            At<int32_t>(self, 0x160) = 0;
            At<int32_t>(self, 0x19C) = 0;
            At<int32_t>(self, 0x184) = 0;
            PropAnim_OnClose(self);
            Door_SetAnim(self, anim);
        }
    }
    return 1;
}

// 0x4F0210 (thiscall): the 0x5C block at +0x160 cleared
void __fastcall ClearBlock160(uint32_t* self) {
    for (int i = 0; i < 23; i++) {
        self[i] = 0;
    }
}

#define C13D_CORE_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C13D_CORE_REVERSED("CPropAnim::ReleaseUseModel", 0x004ED8E0, &ReleaseUseModel);
C13D_CORE_REVERSED("CPropAnim::DeleteChildren", 0x004ED9C0, &DeleteChildren);
C13D_CORE_REVERSED("CPropAnim::FreeChildSlot", 0x004EDA20, &FreeChildSlot);
C13D_CORE_REVERSED("RefArray::RemoveAt", 0x004EDAC0, &RemoveAt);
C13D_CORE_REVERSED("CPropAnim::CPropAnim", 0x004EDB20, &Construct);
C13D_CORE_REVERSED("CPropAnim::GetHealthFraction", 0x004EDD80, &GetHealthFraction);
C13D_CORE_REVERSED("CPropAnim::AddChild", 0x004EE5B0, &AddChild);
C13D_CORE_REVERSED("CPropAnim::RemoveChild", 0x004EE610, &RemoveChild);
C13D_CORE_REVERSED("CPropAnim::CrushVehicles", 0x004EE690, &CrushVehicles);
C13D_CORE_REVERSED("CPropAnim::~CPropAnim", 0x004EE950, &Destruct);
C13D_CORE_REVERSED("CPropAnim::UpdateClosing", 0x004EEBA0, &UpdateClosing);
C13D_CORE_REVERSED("CPropAnim::Open", 0x004EED10, &Open);
C13D_CORE_REVERSED("CPropAnim::Close", 0x004EEEC0, &Close);
C13D_CORE_REVERSED("CPropAnim::ClearBlock160", 0x004F0210, &ClearBlock160);

} // namespace
