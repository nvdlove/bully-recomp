// CObject, CCutsceneObject, CDummyObject, CAccessory and CAccessoryContainer.

#include "game/Object.h"

#include <cmath>

#include "game/ModelInfo.h"
#include "game/Pools.h"
#include "reversed.h"

namespace {

using veh::CVector;

// ours, called where they are
GAME_FN(CObject*, __thiscall, Object_Construct, 0x004CF410, CObject* object);
GAME_FN(void, __thiscall, Object_Destruct, 0x004CFBA0, CObject* object);
GAME_FN(int, __thiscall, ObjectPool_GetIndexChained, 0x004CF3E0, CPool<uint8_t>* pool, void* object);
GAME_FN(void, __thiscall, Accessory_StopAnim, 0x004A6C40, CAccessory* accessory);
GAME_FN(void, __thiscall, Accessory_PlayAnim, 0x004A6CA0, CAccessory* accessory);
GAME_FN(void, __thiscall, Accessory_KnockOff, 0x004A71A0, CAccessory* accessory);
GAME_FN(uint8_t*, __thiscall, Accessory_GetInfo, 0x004A6C30, CAccessory* accessory);
GAME_FN(const char*, __thiscall, AnimGroupTable_GetName, 0x004A75B0, void* table, int index);

// CPhysical / CEntity / CDummy
GAME_FN(void, __thiscall, Physical_Construct, 0x00468E90, void* entity);
GAME_FN(void, __thiscall, Physical_Destruct, 0x004690B0, void* entity);
GAME_FN(void, __thiscall, Physical_4697D0, 0x004697D0, void* entity);
GAME_FN(void, __thiscall, Physical_469840, 0x00469840, void* entity);
GAME_FN(void, __thiscall, Entity_SetModelIndex, 0x00468660, void* entity, int model, int arg);
GAME_FN(void, __thiscall, Entity_SetModelIndexNoCreate, 0x00465E20, void* entity, int model);
GAME_FN(void, __thiscall, Entity_AttachToRwObject, 0x004666F0, void* entity, void* rwObject);
GAME_FN(void, __thiscall, Entity_DetachFromRwObject, 0x00466740, void* entity);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(void, __thiscall, Entity_Render, 0x00466D40, void* entity);
GAME_FN(void, __thiscall, Entity_PreRender, 0x00466CC0, void* entity);
GAME_FN(void, __thiscall, Entity_466160, 0x00466160, void* entity, int arg);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Entity_SetMatrix, 0x0046DE90, void* entity, void* matrix, int);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __thiscall, Dummy_Construct, 0x00465150, void* dummy);
GAME_FN(void, __thiscall, Dummy_Destruct, 0x00465180, void* dummy);
GAME_FN(void, __cdecl, Dummy_Delete, 0x00465420, void* dummy);

// the animation data (+0x114) and the animation manager
GAME_FN(void, __thiscall, AnimData_Construct, 0x006BA240, void* animData);
GAME_FN(void, __thiscall, AnimData_Destruct, 0x006B5A80, void* animData);
inline void* const animManager = reinterpret_cast<void*>(0x020C4910);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C2070, void* manager, void* animData, int anim, void* root, void* slot,
        void* group, void* groupAnimData, uintptr_t);
GAME_FN(void, __thiscall, AnimManager_Stop, 0x006C2200, void* manager, void* animData, void* anim);
GAME_FN(void*, __thiscall, AnimGroups_FindByHash, 0x00465580, void* groups, uint32_t hash, int* index);   // (the entry, nullptr: none)
GAME_FN(void*, __thiscall, AnimGroups_Get, 0x00464EA0, void* groups, int index);
inline void* const accessoryAnimGroups = reinterpret_cast<void*>(0x00C2C1B0);   // (0x1C bytes an entry, its name first)

// Gamebryo
GAME_FN(void*, __cdecl, Node_GetRoot, 0x006C7370, void* object);
GAME_FN(uint8_t*, __cdecl, Node_GetTransform, 0x006C9A90, void* node);   // (+0x24: the position)
GAME_FN(void*, __cdecl, Node_6CA0A0, 0x006CA0A0, void* node);
GAME_FN(void, __thiscall, Matrix_FromNode, 0x00412A70, void* matrix, void* node);
GAME_FN(void, __cdecl, Clump_Render, 0x006C7420, void* object);
GAME_FN(void, __cdecl, Fn_408380, 0x00408380, void* object);
GAME_FN(void, __cdecl, Fn_406910, 0x00406910, void* object);

// the renderer's current pass (thiscall on 0xD140B8)
inline void* const renderState = reinterpret_cast<void*>(0x00D140B8);
GAME_FN(void, __thiscall, RenderState_Set, 0x005EE570, void* state, int pass);

GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __cdecl, GetObjectInfo, 0x004D0CA0, int model);   // (the object data by model)
GAME_FN(uint8_t, __cdecl, Fn_4D0CD0, 0x004D0CD0, int model);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, uint32_t* sound, uintptr_t);
GAME_FN(uint8_t, __thiscall, Sounds_599F20, 0x00599F20, void* sounds, int sound, int model);
GAME_FN(void, __thiscall, Sounds_Stop, 0x0059A420, void* sounds, int sound, int, int);
inline void* const sounds = reinterpret_cast<void*>(0x00CF5DC4);
GAME_VAR(void*, fn5875F0Owner, 0x00CF4278);
GAME_FN(void, __thiscall, Fn_5875F0, 0x005875F0, void* owner, CObject* object, int);
inline void* const persistentEntities = reinterpret_cast<void*>(0x00BCEB50);
GAME_FN(int16_t, __thiscall, Persistent_Find, 0x004263C0, void* store, uint32_t hashId);
GAME_FN(uint8_t, __thiscall, Persistent_426360, 0x00426360, void* store, int id);
GAME_FN(uint8_t, __thiscall, Persistent_426380, 0x00426380, void* store, int id);
GAME_FN(uint8_t, __thiscall, Persistent_4263A0, 0x004263A0, void* store, int id);
GAME_FN(int, __thiscall, Persistent_4263F0, 0x004263F0, void* store, int id);
GAME_FN(void, __cdecl, Fn_4454F0, 0x004454F0, int, int);
GAME_VAR(void*, radar, 0x00C674CC);
GAME_FN(void, __thiscall, Radar_ClearBlipFor, 0x0054A890, void* radar, int kind, int handle);
GAME_VAR(uint8_t, byte20C5BE6, 0x020C5BE6);   // (mission objects aren't drawn while it's 1)
GAME_VAR(float, cutsceneDrawScale, 0x00C3CD00);
GAME_VAR(float, drawDistanceSq, 0x01B64878);

// the accessories
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(int, __thiscall, ModelInfo_AddRef, 0x0050EB90, CBaseModelInfo* mi);
GAME_FN(int, __thiscall, ModelInfo_RemoveRef, 0x0050EBC0, CBaseModelInfo* mi);
GAME_FN(void, __thiscall, Unknown2E0_Construct, 0x00669FD0, uint32_t* part);
GAME_FN(void, __thiscall, Unknown2E0_Destruct, 0x0049A650, uint32_t* part);
GAME_FN(void, __thiscall, Unknown2E0_669FF0, 0x00669FF0, uint32_t* part);
inline void* const randomGen = reinterpret_cast<void*>(0x00CF356C);
GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* gen);   // [0, 1)
inline void* const accessoryTable = reinterpret_cast<void*>(0x00C2C730);
GAME_FN(void, __thiscall, AccessoryTable_Remove, 0x004A7960, void* table, CAccessory* accessory);

inline re::Ptr32<void>* const objectVtable = reinterpret_cast<re::Ptr32<void>*>(0x009134F4);
inline re::Ptr32<void>* const cutsceneVtable = reinterpret_cast<re::Ptr32<void>*>(0x00913304);
inline re::Ptr32<void>* const dummyObjectVtable = reinterpret_cast<re::Ptr32<void>*>(0x009133B4);
inline re::Ptr32<void>* const accessoryVtable = reinterpret_cast<re::Ptr32<void>*>(0x00912484);

template <typename R, typename... A>
R CallVirtual(void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(object, args...);
}

using re::At;

// a matrix built from a node, set as the entity's
void FollowNode(void* entity, void* node) {
    alignas(4) uint8_t matrix[0x44];
    At<void*>(matrix, 0x40) = node;
    Matrix_FromNode(matrix, node);
    Entity_SetMatrix(entity, matrix, 0);
}

} // namespace

// ---- CObject --------------------------------------------------------------------------

CObject* CObject::Construct() {
    Physical_Construct(this);
    vtable = objectVtable;
    unknown204 = 0;
    sound = 0;
    unknown210[3] = 1.0f;
    unknown210[1] = 1.0f;
    objectFlags &= 0x90;
    unknown210[2] = 1.0f;
    unknown210[0] = 1.0f;
    typeAndStatus = uint8_t((typeAndStatus & 0xFC) | 4);
    objectKind = 0;
    objectType = 0;
    unknown220 = 0;
    unknown111 = 1;
    Sound_Set(&sound, 0);
    hashId = 0;
    unknown224 = 0;
    return this;
}

void CObject::Destruct() {
    vtable = objectVtable;
    if (objectFlags & 0x20) {
        uint8_t* info = GetObjectInfo(modelIndex);
        if (info && At<int16_t>(info, 0xE) != -1) {
            int16_t infoSound = At<int16_t>(info, 0xE);
            if (!(objectFlags & 1) || !Sounds_599F20(sounds, infoSound, modelIndex)) {
                Sounds_Stop(sounds, At<int16_t>(info, 0xE), 1, 1);
            }
            objectFlags &= 0xDF;
        }
    }
    // The radar's index for it: its slot, or the chain's. (The game compares this with
    // objects + slot in bytes, so only slot 0 counts as in this pool: the rest get the
    // pool's size plus the chain's index.)
    CPool<CObject>* pool = CPools::ms_pObjectPool;
    int slot = int((reinterpret_cast<uint8_t*>(this) - re::PtrCast<uint8_t*>(pool->objects)) / pool->elemSize);
    int index = slot;
    if (reinterpret_cast<uint8_t*>(this) != re::PtrCast<uint8_t*>(pool->objects) + slot) {
        index = pool->size;
        if (pool->Next()) {
            index += ObjectPool_GetIndexChained(pool->Next(), this);
        }
    }
    Radar_ClearBlipFor(radar, 3, index);
    if (hashId) {
        int16_t id = Persistent_Find(persistentEntities, hashId);
        if (id >= 0 && !Persistent_426360(persistentEntities, id) && !Persistent_426380(persistentEntities, id) &&
            !Persistent_4263A0(persistentEntities, id)) {
            Fn_4454F0(Persistent_4263F0(persistentEntities, id), 0);
        }
    }
    Sound_Set(&sound, 0);
    Physical_Destruct(this);
}

void CObject::Init() {
    typeAndStatus = uint8_t((typeAndStatus & 0xFC) | 4);
    uint8_t* info = GetObjectInfo(modelIndex);
    objectType = 1;
    CallVirtual<void>(this, 7, 1);   // SetIsStatic(true)
    objectFlags &= 0xC0;
    Sound_Set(&sound, 0);
    unknown210[3] = 1.0f;
    objectFlags &= 0xBF;
    unknown210[1] = 1.0f;
    unknown210[2] = 1.0f;
    unknown210[0] = 1.0f;
    unknown120 = 0;
    if (CModelInfo::ms_modelInfoPtrs[modelIndex]->type == MODEL_INFO_WEAPON) {
        objectFlags |= 0x10;
    }
    unknown111 = 1;
    if (CallVirtual<uint8_t>(this, 31)) {
        if (info && (info[0x22] & 1)) {
            Fn_5875F0(fn5875F0Owner, this, 0);
        }
        unknown48 = 1;
        unknown20 = 1;
    }
    hashId = 0;
    unknown224 = 0;
    if (info) {
        uint16_t id = At<uint16_t>(info, 0) & 0x7FFF;
        flags1BC &= 0xFD;
        unknown220 = int16_t(id);
        if (id == 30000) {
            objectFlags |= 0x40;
        }
    }
}

uint8_t CObject::CanBeDeleted() {
    switch (objectType) {
    case 2:
    case 4:
    case 5:
        return 0;
    default:
        return 1;
    }
}

int32_t CObject::GetInfo24() {
    uint8_t* info = GetObjectInfo(modelIndex);
    return info ? At<int32_t>(info, 0x24) : 0;
}

void CObject::SetIsStatic(int isStatic) {
    uint8_t flag = uint8_t(isStatic);
    if (objectKind == 7) {
        unknownB0 = flag;
    } else {
        unknown28 = flag;
    }
    unknown180 = flag ? 10 : 0;
}

void CObject::Render() {
    if (unknown94 != 0) {
        return;
    }
    if (byte20C5BE6 == 1 && (objectFlags & 1)) {
        return;
    }
    RenderState_Set(renderState, 3);
    Entity_Render(this);
}

uint32_t CObject::IsUnknownVf30() {
    return unknown48 == 0 && unknown20 == 0 ? 1 : 0;
}

uint32_t CObject::IsUnknownVf31() {
    return !(objectFlags & 1) && Fn_4D0CD0(modelIndex) ? 1 : 0;
}

float CObject::GetUnknown220() {
    return float(int(unknown220));
}

// 0x4CF3E0 (thiscall on the object pool): an object's index along the pool's chain
// (with the byte comparison Destruct has: only slot 0 of a pool matches)
static int __fastcall ObjectPool_GetIndexChainedImpl(CPool<uint8_t>* pool, re::Edx, void* object) {
    int slot = int((reinterpret_cast<uint8_t*>(object) - pool->objects) / pool->elemSize);
    if (reinterpret_cast<uint8_t*>(object) == pool->objects + slot) {
        return slot;
    }
    int index = pool->size;
    if (pool->Next()) {
        index += ObjectPool_GetIndexChained(pool->Next(), object);
    }
    return index;
}

// ---- CCutsceneObject ------------------------------------------------------------------

CCutsceneObject* CCutsceneObject::Construct() {
    Object_Construct(this);
    vtable = cutsceneVtable;
    AnimData_Construct(&animDataStorage);
    unknown158 = 1.0f;
    unknown15C = 1.0f;
    typeAndStatus = uint8_t((typeAndStatus & 7) | 0x10);
    unknown20 = 0;
    attachedTo = nullptr;
    attachedIsFrame = 0;
    animData = &animDataStorage;
    unknown74 = 1;
    unknownEC = 1;
    objectType = 4;
    boneIndex = -1;
    At<uint32_t>(&animDataStorage, 0x98) |= 0x10;
    At<uint32_t>(animData, 0x98) |= 0x20;
    return this;
}

void CCutsceneObject::Destruct() {
    vtable = cutsceneVtable;
    animData = nullptr;
    AnimData_Destruct(&animDataStorage);
    Object_Destruct(this);
}

void CCutsceneObject::SetModelIndex(int model, int arg) {
    Entity_SetModelIndex(this, model, arg);
    if (*rwObject == 2 && animData) {
        At<CVector*>(animData, 0xA4) = &moveSpeed;
        At<uint32_t>(animData, 0x98) |= 4;
    }
}

void CCutsceneObject::ProcessControl() {
    if (attachedTo) {
        FollowNode(this, attachedIsFrame ? static_cast<void*>(attachedTo) : Node_6CA0A0(attachedTo));
        return;
    }
    World_Remove(this);
    const float* pos = nullptr;
    uint32_t bone = uint32_t(boneIndex);
    if (bone != 0xFFFFFFFFu) {
        uint8_t* bones = At<uint8_t*>(At<uint8_t*>(animData, 0x94), 0x10);   // {entries, count}
        if (bone < At<uint32_t>(bones, 4)) {
            uint8_t* entry = At<uint8_t*>(bones, 0) + bone * 0x40;
            if (entry && (At<uint32_t>(entry, 0x18) & 0x200)) {
                pos = &At<float>(entry, 0x34);
            }
        }
    }
    if (!pos) {
        // (the root's position, through the animation data)
        uint8_t* root = At<uint8_t*>(At<uint8_t*>(this, 0x248), 8);
        pos = &At<float>(root, 0x58);
    }
    Entity_SetPosition(this, pos[0], pos[1], pos[2]);
    World_Add(this);
}

void CCutsceneObject::PreRender() {
    if (attachedTo) {
        FollowNode(this, attachedIsFrame ? static_cast<void*>(attachedTo) : Node_6CA0A0(attachedTo));
    }
}

void CCutsceneObject::Render() {
    if (unknown94 != 0) {
        return;
    }
    RenderState_Set(renderState, 7);
    double distance = double(cutsceneDrawScale) * 60.0;
    drawDistanceSq = float(distance * distance + distance * distance);
    Entity_Render(this);
    distance = double(cutsceneDrawScale) * 60.0;
    drawDistanceSq = float(distance * distance);
}

// ---- CDummyObject ---------------------------------------------------------------------

CDummyObject* CDummyObject::Construct() {
    Dummy_Construct(this);
    vtable = dummyObjectVtable;
    return this;
}

CDummyObject* CDummyObject::Construct(CObject* object) {
    Dummy_Construct(this);
    vtable = dummyObjectVtable;
    Entity_SetModelIndexNoCreate(this, object->modelIndex);
    if (object->rwObject) {
        Entity_AttachToRwObject(this, object->rwObject);
    }
    Entity_DetachFromRwObject(object);
    At<uint8_t>(this, 0x111) = object->unknown111;
    hashId = object->hashId;
    return this;
}

CDummyObject* CDummyObject::Destroy(uint8_t flags) {
    vtable = dummyObjectVtable;
    Dummy_Destruct(this);
    if (flags & 1) {
        Dummy_Delete(this);
    }
    return this;
}

// ---- CAccessory -----------------------------------------------------------------------

CAccessory* CAccessory::Construct() {
    Physical_Construct(this);
    vtable = accessoryVtable;
    AnimData_Construct(&animDataStorage);
    Unknown2E0_Construct(&unknown2E0);
    typeAndStatus |= 7;
    unknown160 = 0.97f;
    model = -1;
    unknown158 = 1.0f;
    animId = -1;
    unknown15C = 1.0f;
    animGroup = -1;
    unknown164 = 0.1f;
    unknown20 = 1;
    unknown3C = 1;
    animData = &animDataStorage;
    anim = nullptr;
    wearerAnimData = nullptr;
    unknown2DC = 0;
    info = nullptr;
    flags1BC = uint8_t((flags1BC & 0xF3) | 2);
    return this;
}

void CAccessory::Destruct() {
    vtable = accessoryVtable;
    Entity_DeleteRwObject(this);
    animData = nullptr;
    Unknown2E0_Destruct(&unknown2E0);
    AnimData_Destruct(&animDataStorage);
    Physical_Destruct(this);
}

void CAccessory::SetInfo(uint8_t* newInfo) {
    info = newInfo;
}

uint8_t* CAccessory::GetInfo() {
    return info;
}

void CAccessory::StopAnim() {
    if (!rwObject) {
        return;
    }
    if (wearerAnimData && anim) {
        AnimManager_Stop(animManager, wearerAnimData, anim);
        anim = nullptr;
        wearerAnimData = nullptr;
    }
    if (model != -1) {
        CallVirtual<void>(this, 8, -1, 1);   // SetModelIndex(-1, true)
    }
}

void CAccessory::PlayAnim() {
    if (!rwObject) {
        return;
    }
    void* group = nullptr;
    if (animGroup != -1) {
        void* groups = At<void*>(animData, 0x94);
        if (groups) {
            // (the index starts as this pointer's value: what's left when the name isn't found)
            int index = int(reinterpret_cast<uintptr_t>(this));
            const char* name = AnimGroupTable_GetName(accessoryAnimGroups, animGroup);
            AnimGroups_FindByHash(groups, HashName(name), &index);
            groups = At<void*>(animData, 0x94);
            group = AnimGroups_Get(groups, index);
            if (!group) {
                group = AnimGroups_Get(groups, 0);
            }
        }
    }
    if (At<void*>(animData, 0x94)) {
        AnimManager_Play(animManager, wearerAnimData, animId, nullptr, &anim, group, animData, 0);
    } else {
        AnimManager_Play(animManager, wearerAnimData, animId, Node_GetRoot(rwObject), &anim, nullptr, nullptr, 0);
    }
}

void CAccessory::Fn_4A6DD0() {
    if (unknown2DC != 0) {
        return;
    }
    Physical_469840(this);
    CallVirtual<void>(this, 3);
}

void CAccessory::SetTurnSpeed(float x, float y, float z) {
    turnSpeed = {x, y, z};
}

void CAccessory::GetNodePosition(CVector* out) {
    uint8_t* transform = Node_GetTransform(At<void*>(animData, 0xC));
    *out = At<CVector>(transform, 0x24);
}

void CAccessory::Set(int newModel, int group, uint8_t* wearer, int animation, int arg5) {
    animGroup = group;
    model = newModel;
    wearerAnimData = wearer;
    animId = animation;
    unknown2DC = arg5;
    if (newModel != -1) {
        if (GetStreamingInfo(newModel)[4] != 1) {
            RequestModel(newModel, 5);
        }
        ModelInfo_AddRef(CModelInfo::ms_modelInfoPtrs[newModel]);
    }
    if (rwObject) {
        Accessory_StopAnim(this);
    }
    if (model != -1) {
        CallVirtual<void>(this, 8, model, 1);   // SetModelIndex(model, true)
        Accessory_PlayAnim(this);
    }
}

void CAccessory::Clear() {
    if (model != -1) {
        ModelInfo_RemoveRef(CModelInfo::ms_modelInfoPtrs[model]);
    }
    Accessory_StopAnim(this);
    model = -1;
}

void CAccessory::KnockOff() {
    uint8_t* transform = Node_GetTransform(At<void*>(animData, 0xC));
    CVector pos = At<CVector>(transform, 0x24);
    if (wearerAnimData && anim) {
        AnimManager_Stop(animManager, wearerAnimData, anim);
        anim = nullptr;
        wearerAnimData = nullptr;
    }
    Entity_SetPosition(this, pos.x, pos.y, pos.z);
    unknown2DC = 0;
    Unknown2E0_669FF0(&unknown2E0);
    // (the constants are floats widened to doubles)
    const double spread = double(0.14f);
    const double half = double(0.07f);
    moveSpeed.x = float(double(Random_GetFloat(randomGen)) * spread - half);
    moveSpeed.y = float(double(Random_GetFloat(randomGen)) * spread - half);
    moveSpeed.z = float(double(Random_GetFloat(randomGen)) * (half - 0.0) + 0.0);
    // (z doesn't count: the game squares a stale 0 in its place)
    float lengthSq = float(double(moveSpeed.x) * moveSpeed.x + double(moveSpeed.y) * moveSpeed.y + 0.0);
    float length = float(std::sqrt(double(lengthSq)));
    if (0.04f > length) {
        moveSpeed.x = 0.04f;
        moveSpeed.y = 0.04f;
    }
    turnSpeed = {0.1f, 0.13f, 0.12f};
    Physical_4697D0(this);
}

void CAccessory::PreRender() {
    Entity_PreRender(this);
}

void CAccessory::Render() {
    RenderState_Set(renderState, 7);
    if (rwObject[0x34]) {
        Fn_408380(rwObject);
    } else {
        Fn_406910(rwObject);
    }
    Clump_Render(rwObject);
}

// 0x4A6DF0 (thiscall): its +0x2E0's 0x66A010
GAME_FN(float, __thiscall, Unknown2E0_66A010, 0x0066A010, uint32_t* part);
static float __fastcall Accessory_Fn_4A6DF0(CAccessory* accessory, re::Edx) {
    return Unknown2E0_66A010(&accessory->unknown2E0);
}

// 0x4A7340 (cdecl): CAccessory's operator delete (the slot back to the pool)
static void __cdecl Accessory_Delete(void* accessory) {
    CPools::ms_pAccessoryPool->Delete(static_cast<CAccessory*>(accessory));
}

// 0x4A75B0 (thiscall on a table of 0x1C-byte entries): an entry's first field
static const char* __fastcall AnimGroupTable_GetNameImpl(uint8_t* table, re::Edx, int index) {
    return At<const char*>(table, index * 0x1C);
}

// ---- CAccessoryContainer --------------------------------------------------------------

void CAccessoryContainer::Clear() {
    for (int i = 0; i < 2; i++) {
        if (accessories[i]) {
            AccessoryTable_Remove(accessoryTable, accessories[i]);
            if (accessories[i]) {
                CallVirtual<void>(accessories[i], 0, 1);   // deleted
            }
            accessories[i] = nullptr;
        }
    }
}

void CAccessoryContainer::Add(CAccessory* accessory) {
    for (int i = 0; i < 2; i++) {
        if (!accessories[i]) {
            accessories[i] = accessory;
            return;
        }
    }
}

uint8_t CAccessoryContainer::HasRoom() {
    for (int i = 0; i < 2; i++) {
        if (!accessories[i]) {
            return 1;
        }
    }
    return 0;
}

uint8_t CAccessoryContainer::Count() {
    uint8_t count = accessories[0] != nullptr;
    if (accessories[1]) {
        count++;
    }
    return count;
}

CAccessory* CAccessoryContainer::Get(uint32_t index) {
    return index < 2 ? accessories[index] : nullptr;
}

void CAccessoryContainer::PreRender() {
    for (int i = 0; i < 2; i++) {
        if (accessories[i]) {
            CallVirtual<void>(accessories[i], 17);
        }
    }
}

void CAccessoryContainer::Render() {
    for (int i = 0; i < 2; i++) {
        if (accessories[i]) {
            CallVirtual<void>(accessories[i], 18);
        }
    }
}

void CAccessoryContainer::Fn_4A7490(int arg) {
    for (int i = 0; i < 2; i++) {
        if (accessories[i]) {
            Entity_466160(accessories[i], arg);
        }
    }
}

void CAccessoryContainer::KnockOff(float chance) {
    if (chance < 0.0f) {
        return;
    }
    if (1.0f < chance) {
        chance = 1.0f;
    } else if (chance <= 0.0f) {
        return;
    }
    for (int i = 0; i < 2; i++) {
        if (!accessories[i]) {
            continue;
        }
        if (chance < Random_GetFloat(randomGen)) {
            continue;
        }
        if (Accessory_GetInfo(accessories[i])[0x15] & 1) {
            continue;
        }
        Accessory_KnockOff(accessories[i]);
        accessories[i] = nullptr;
    }
}

// ---- registration ---------------------------------------------------------------------

#define OBJECT_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)
#define OBJECT_MEMBER(name, address, member) OBJECT_REVERSED(name, address, member)

OBJECT_MEMBER("CObject::Construct", 0x004CF410, &CObject::Construct);
OBJECT_MEMBER("CObject::Destruct", 0x004CFBA0, &CObject::Destruct);
OBJECT_MEMBER("CObject::Init", 0x004CFCE0, &CObject::Init);
OBJECT_MEMBER("CObject::CanBeDeleted", 0x004CF2B0, &CObject::CanBeDeleted);
OBJECT_MEMBER("CObject::GetInfo24", 0x004CF3C0, &CObject::GetInfo24);
OBJECT_MEMBER("CObject::SetIsStatic", 0x004CF2E0, &CObject::SetIsStatic);
OBJECT_MEMBER("CObject::Render", 0x004CF320, &CObject::Render);
OBJECT_MEMBER("CObject::IsUnknownVf30", 0x004CF360, &CObject::IsUnknownVf30);
OBJECT_MEMBER("CObject::IsUnknownVf31", 0x004CF380, &CObject::IsUnknownVf31);
OBJECT_MEMBER("CObject::GetUnknown220", 0x004CF3B0, &CObject::GetUnknown220);
OBJECT_REVERSED("CPool<CObject>::GetIndexChained", 0x004CF3E0, &ObjectPool_GetIndexChainedImpl);

OBJECT_MEMBER("CCutsceneObject::Construct", 0x004CEE60, &CCutsceneObject::Construct);
OBJECT_MEMBER("CCutsceneObject::Destruct", 0x004CEEF0, &CCutsceneObject::Destruct);
OBJECT_MEMBER("CCutsceneObject::SetModelIndex", 0x004CEF20, &CCutsceneObject::SetModelIndex);
OBJECT_MEMBER("CCutsceneObject::ProcessControl", 0x004CF0C0, &CCutsceneObject::ProcessControl);
OBJECT_MEMBER("CCutsceneObject::PreRender", 0x004CF040, &CCutsceneObject::PreRender);
OBJECT_MEMBER("CCutsceneObject::Render", 0x004CEF70, &CCutsceneObject::Render);

OBJECT_MEMBER("CDummyObject::Construct", 0x004CF200, static_cast<CDummyObject* (CDummyObject::*)()>(&CDummyObject::Construct));
OBJECT_MEMBER("CDummyObject::ConstructFromObject", 0x004CF220,
              static_cast<CDummyObject* (CDummyObject::*)(CObject*)>(&CDummyObject::Construct));
OBJECT_MEMBER("CDummyObject::Destroy", 0x004CF280, &CDummyObject::Destroy);

OBJECT_MEMBER("CAccessory::Construct", 0x004A6E40, &CAccessory::Construct);
OBJECT_MEMBER("CAccessory::Destruct", 0x004A6BE0, &CAccessory::Destruct);
OBJECT_MEMBER("CAccessory::SetInfo", 0x004A6C20, &CAccessory::SetInfo);
OBJECT_MEMBER("CAccessory::GetInfo", 0x004A6C30, &CAccessory::GetInfo);
OBJECT_MEMBER("CAccessory::StopAnim", 0x004A6C40, &CAccessory::StopAnim);
OBJECT_MEMBER("CAccessory::PlayAnim", 0x004A6CA0, &CAccessory::PlayAnim);
OBJECT_MEMBER("CAccessory::Fn_4A6DD0", 0x004A6DD0, &CAccessory::Fn_4A6DD0);
OBJECT_MEMBER("CAccessory::SetTurnSpeed", 0x004A6E00, &CAccessory::SetTurnSpeed);
OBJECT_MEMBER("CAccessory::GetNodePosition", 0x004A6EF0, &CAccessory::GetNodePosition);
OBJECT_MEMBER("CAccessory::Set", 0x004A6FA0, &CAccessory::Set);
OBJECT_MEMBER("CAccessory::Clear", 0x004A7040, &CAccessory::Clear);
OBJECT_MEMBER("CAccessory::KnockOff", 0x004A71A0, &CAccessory::KnockOff);
OBJECT_MEMBER("CAccessory::PreRender", 0x004A6DC0, &CAccessory::PreRender);
OBJECT_MEMBER("CAccessory::Render", 0x004A6D70, &CAccessory::Render);
OBJECT_REVERSED("CAccessory::Fn_4A6DF0", 0x004A6DF0, &Accessory_Fn_4A6DF0);
OBJECT_REVERSED("CAccessory::operator delete", 0x004A7340, &Accessory_Delete);
OBJECT_REVERSED("AccessoryAnimGroups::GetName", 0x004A75B0, &AnimGroupTable_GetNameImpl);

OBJECT_MEMBER("CAccessoryContainer::Clear", 0x004A7370, &CAccessoryContainer::Clear);
OBJECT_MEMBER("CAccessoryContainer::Add", 0x004A73B0, &CAccessoryContainer::Add);
OBJECT_MEMBER("CAccessoryContainer::HasRoom", 0x004A73D0, &CAccessoryContainer::HasRoom);
OBJECT_MEMBER("CAccessoryContainer::Count", 0x004A73F0, &CAccessoryContainer::Count);
OBJECT_MEMBER("CAccessoryContainer::Get", 0x004A7410, &CAccessoryContainer::Get);
OBJECT_MEMBER("CAccessoryContainer::PreRender", 0x004A7430, &CAccessoryContainer::PreRender);
OBJECT_MEMBER("CAccessoryContainer::Render", 0x004A7460, &CAccessoryContainer::Render);
OBJECT_MEMBER("CAccessoryContainer::Fn_4A7490", 0x004A7490, &CAccessoryContainer::Fn_4A7490);
OBJECT_MEMBER("CAccessoryContainer::KnockOff", 0x004A74C0, &CAccessoryContainer::KnockOff);
