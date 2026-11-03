// More of the pickups (see Pickups.cpp): what a dead ped drops, a pickup's removal and
// collection, CPickups' init and the removals by model or by butes.

#include <cmath>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_ARRAY(uint8_t, ms_pickups, 0x00C0A100);   // 300 of 0x48
GAME_ARRAY(uint8_t, ms_colours, 0x00C0F590);
GAME_ARRAY(int32_t, ms_collected, 0x00C09980);
GAME_VAR(uint32_t, ms_collectedNext, 0x00C0997C);
GAME_ARRAY(int32_t, ms_collectedKeys, 0x00C09CC0);
GAME_VAR(int32_t, ms_numCollectedKeys, 0x00C09978);
GAME_VAR(uint8_t, ms_bFlag_C0A0AC, 0x00C0A0AC);
GAME_VAR(uint32_t, ms_timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t*, ms_pDropChance, 0x020C892C);         // (+0x18: the weapon drop chance)
GAME_VAR(int32_t, ms_modelNoDrop, 0x00BF42DC);          // (the model index list's)
GAME_VAR(int32_t, ms_weaponRangeLow, 0x00A136B0);
GAME_VAR(int32_t, ms_weaponRangeHigh, 0x00A136B4);
GAME_VAR(void*, ms_pEntity_C2A8E8, 0x00C2A8E8);
GAME_VAR(col::CVector, ms_zeroVector, 0x00BCC564);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
// the model index list's weapons that aren't dropped
GAME_VAR(int32_t, ms_noDrop1, 0x00BF44D8);
GAME_VAR(int32_t, ms_noDrop2, 0x00BF444C);
GAME_VAR(int32_t, ms_noDrop3, 0x00BF435C);
GAME_VAR(int32_t, ms_noDrop4, 0x00BF4108);
GAME_VAR(int32_t, ms_noDrop5, 0x00BF4104);
GAME_VAR(int32_t, ms_noDrop6, 0x00BF4100);
inline uint8_t* const ms_defaultButes = reinterpret_cast<uint8_t*>(0x00C0F560);

inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(int, __thiscall, Ped_GetStat, 0x00477060, void* ped, int stat);
GAME_FN(uint8_t, __thiscall, Ped_Fn_478090, 0x00478090, void* ped, int model);
GAME_FN(uint8_t, __thiscall, Ped_ProcessLine, 0x0047AE30, void* ped, const col::CVector* from, const col::CVector* to,
        float radius, float* colPoint);
GAME_FN(void, __thiscall, Inventory_Begin, 0x007DB6B0, void* inventory);
GAME_FN(uint8_t*, __thiscall, Inventory_Next, 0x0045CDB0, void* inventory);
GAME_FN(uint8_t*, __cdecl, GetModelInfo, 0x0051B450, int model);
GAME_FN(int32_t, __cdecl, Pickups_GenerateNewOne, 0x00443C40, float x, float y, float z, int model, uint8_t* butes,
        int flag6, int value7, int flag8, int flag9);
GAME_FN(col::CVector*, __cdecl, VectorAdd, 0x004129A0, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(col::CVector*, __cdecl, VectorSub, 0x00414B70, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(col::CVector*, __cdecl, HeadingToVector, 0x00432DF0, col::CVector* out, float heading);
GAME_FN(void, __cdecl, Entity_CleanUpReference, 0x004657E0, void* entity, re::Ptr32<void>* reference);
GAME_FN(void, __thiscall, ModelInfo_RemoveRef, 0x0050EBC0, uint8_t* mi);
GAME_FN(void, __cdecl, Fn_4DA680, 0x004DA680, int model, int);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(void, __thiscall, AnimObject_Destruct, 0x006B5A80, void* anim);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, EffectProxy_Stop, 0x00668BA0, void* proxy);
GAME_FN(void, __cdecl, Pickups_RemoveFromList, 0x00443A60, void* pickup);
GAME_FN(void, __cdecl, Pickups_StoreCollected, 0x00443940, uint8_t* pickup);
GAME_FN(void, __thiscall, Pickup_Remove, 0x004448F0, uint8_t* pickup, int deleteObject, int regenerate);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Pickups_LoadColours, 0x00443B30);
GAME_FN(void, __thiscall, PickupButes_Fn_729670, 0x00729670, void* butes);
GAME_FN(void, __thiscall, ModelInfo_Fn_7282A0, 0x007282A0, uint8_t* mi);
GAME_FN(int, __thiscall, Inventory_Count, 0x0045CF40, void* inventory, int model);
GAME_FN(void, __thiscall, Inventory_Give, 0x0045D030, void* inventory, int model, int count, int flag, int);
GAME_FN(void, __thiscall, Fn_5A4CB0, 0x005A4CB0, void* self, int model);
GAME_FN(int, __cdecl, Fn_541E00, 0x00541E00, int model);
GAME_FN(void, __thiscall, Fn_428A10, 0x00428A10, void* self, int id, int);
GAME_FN(void, __thiscall, Fn_428B00, 0x00428B00, void* self, int id);
GAME_FN(void, __cdecl, Fn_541C30, 0x00541C30, int model, int, int, int);
GAME_FN(void, __thiscall, Ped_AddHealth, 0x00474CC0, void* ped, float health);
GAME_FN(void, __thiscall, Player_Fn_446B40, 0x00446B40, void* player);
GAME_FN(void, __cdecl, Fn_458480, 0x00458480, int, int);
GAME_FN(void, __thiscall, Fn_541110, 0x00541110, void* self, int model, int, int, int);
GAME_FN(float, __thiscall, Ped_Fn_47CF10, 0x0047CF10, void* ped);
GAME_FN(void, __thiscall, Ped_Fn_47CD90, 0x0047CD90, void* ped, float value, int);
GAME_FN(void, __thiscall, Sounds_Play, 0x0059C150, void* sounds, int sound);
GAME_FN(void, __thiscall, Counter_Fn_5D9900, 0x005D9900, void* counter, int32_t handle, int);
GAME_FN(void, __cdecl, Pickups_AddToList, 0x004442A0, uint8_t* pickup);
GAME_FN(void, __cdecl, Pickups_AddCollectedKey, 0x004439C0, uint8_t* pickup);
GAME_VAR(void*, ms_pObj_C67554, 0x00C67554);
GAME_VAR(int32_t, ms_currArea, 0x00BD1008);
GAME_VAR(uint8_t*, ms_pPlayerPed, 0x00C1AEA8);
GAME_VAR(int32_t, ms_model_BF4328, 0x00BF4328);
GAME_VAR(int32_t, ms_model_BF4334, 0x00BF4334);
GAME_FN(void, __thiscall, Pickup_Collect, 0x00444A10, uint8_t* pickup, int deleteObject);
GAME_FN(uint8_t, __thiscall, Pickup_CreateObject, 0x00445540, uint8_t* pickup);
GAME_FN(uint8_t, __thiscall, Pickup_GiveToPed, 0x00444C10, uint8_t* pickup, uint8_t* ped, int playSound);
GAME_FN(void, __thiscall, Pickup_Update, 0x00445860, uint8_t* pickup);
GAME_FN(void, __cdecl, ModelInfo_GetByName, 0x0051C190, const char* name, int32_t* model);
GAME_FN(void*, __cdecl, Object_New, 0x004CFCD0, size_t size);
GAME_FN(uint8_t*, __thiscall, Object_Construct, 0x004D0580, void* object, int model, int);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void*, __thiscall, AnimObject_Construct, 0x006BA240, void* anim);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(uint8_t, __cdecl, Fn_4D9790, 0x004D9790, int model, int);
GAME_FN(uint8_t*, __cdecl, Streaming_GetInfo, 0x0052DD30, int model);
GAME_FN(void, __cdecl, Streaming_RequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __thiscall, ModelInfo_AddRef, 0x0050EB90, uint8_t* mi);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float angle);
GAME_FN(void, __thiscall, Entity_UpdateFromRW, 0x00443AA0, void* entity);
GAME_FN(void, __thiscall, Physical_Fn_465760, 0x00465760, void* entity);
GAME_FN(void, __cdecl, IplStore_IncludeEntity, 0x00438840, int slot, uint8_t* entity);
GAME_FN(void, __cdecl, Fn_52ABE0, 0x0052ABE0, col::CVector* pos, int model, uint8_t* pickup);
GAME_FN(float, __cdecl, IsWithinDistance, 0x0041A4D0, const col::CVector* a, const col::CVector* b, float distance);
GAME_FN(uint8_t, __thiscall, Entity_Fn_467460, 0x00467460, void* entity);
GAME_FN(void, __thiscall, Object_Fn_4CF620, 0x004CF620, void* object);
GAME_VAR(uint8_t, ms_bNoGlow_20C5BE6, 0x020C5BE6);
GAME_VAR(void*, ms_pEffects, 0x00B6F1D4);
GAME_VAR(int32_t, g_rwCamera, 0x00C3CCF4);
GAME_FN(uint8_t, __cdecl, Fn_4616A0, 0x004616A0, col::CVector* pos, const col::CVector* to, float radius, float* colPoint, re::Ptr32<void>* hit, int, int, int);
GAME_FN(void, __cdecl, Fn_4D3570, 0x004D3570, col::CVector* pos, float, int);
GAME_FN(void*, __thiscall, Sounds_Fn_595B80, 0x00595B80, void* sounds, uintptr_t id);
GAME_FN(void, __thiscall, Sounds_Fn_59B8A0, 0x0059B8A0, void* sounds, void* sound, col::CVector* pos, int, uintptr_t, int, int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, Placeable_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(uint8_t*, __cdecl, Pickups_FindPickupBy1C, 0x00443870, void* object);
GAME_FN(void, __thiscall, Pickup_ProcessDrop, 0x00444FC0, uint8_t* pickup);
GAME_FN(void, __thiscall, Effects_Start, 0x00668290, void* effects, void* def, void* proxy, int);
GAME_FN(void*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Matrix_ToTransform, 0x00412AC0, void* matrix, float* transform);
GAME_FN(void*, __cdecl, RwCameraGetFrame, 0x006C82F0, uintptr_t camera);
GAME_FN(float*, __cdecl, RwFrameGetLTM, 0x006C9A90, void* frame);
GAME_FN(float, __thiscall, Vector_Normalise, 0x004294A0, col::CVector* v);
GAME_FN(void, __thiscall, EffectProxy_SetTransform, 0x00668EC0, void* proxy, const float* transform);
GAME_FN(void, __thiscall, Matrix_SetRotate, 0x00412D00, void* matrix, const float* angles);

template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

using re::At;

uint8_t* Pickup(int i) {
    return ms_pickups + i * 0x48;
}

col::CVector& EntityPosition(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<col::CVector*>(matrix + 0x30) : *reinterpret_cast<col::CVector*>(entity + 4);
}

// the new pickup onto the ped's list (+0x1DA4, count +0x1DB0), bits 0 and 2 of its flags
// cleared (GetActualPickupIndex inlined)
void AddToPed(uint8_t* ped, int32_t handle) {
    int slot = handle & 0xFFFF;
    if (handle == -1 || !(Pickup(slot)[0x46] & 8) || int16_t(uint32_t(handle) >> 16) != At<int16_t>(Pickup(slot), 0x40)) {
        return;
    }
    if (slot == -1) {
        return;
    }
    At<int32_t>(ped, 0x1DA4 + At<int32_t>(ped, 0x1DB0) * 4) = handle;
    At<int32_t>(ped, 0x1DB0)++;
    Pickup(slot)[0x46] &= 0xFA;
}

// a pickup to be kept is stored before it goes (in use, kept, not taken, flag 0; not 0x10)
bool ShouldStore(const uint8_t* p) {
    uint8_t f = p[0x46];
    return !(f & 0x10) && (f & 8) && (f & 0x20) && p[0x44] == 0 && (f & 1);
}

// a line down through the point finds the ground (its height into the point when hit)
void DropToGround(uint8_t* ped, col::CVector& point, float* colPoint) {
    col::CVector up = {0.0f, 0.0f, 1.0f};
    col::CVector up2 = {0.0f, 0.0f, 1.0f};
    col::CVector below;
    col::CVector above;
    VectorSub(&below, &point, &up);
    VectorAdd(&above, &point, &up2);
    if (Ped_ProcessLine(ped, &above, &below, 0.05f, colPoint)) {
        point.z = colPoint[2];
    }
}

} // namespace

namespace Pickups3 {

// 0x444340: what the ped drops: money (stat 0 as the model, by the chance of stat 1) and
// one weapon it carries (by the chance at 0x20C892C +0x18; not the model list's six), just
// above it (not for 0xBF42DC's model); two of them either side of it, dropped onto the
// ground. (Its collision point's +0x8C/+0x90 are left unset)
static void __cdecl DropForDeadPed(uint8_t* ped) {
    col::CVector offset;
    if (At<int16_t>(ped, 0x10E) == ms_modelNoDrop) {
        offset = ms_zeroVector;
    } else {
        offset = {0.0f, 0.0f, 0.44999998807907104f};
    }
    int32_t models[2];
    int count = 0;
    if (Ped_GetStat(ped, 0) != -1) {
        int roll = Rand() * 99 / 0x7FFF;
        if (roll < Ped_GetStat(ped, 1)) {
            models[0] = Ped_GetStat(ped, 0);
            count = 1;
        }
    }
    float chance = float(double(Rand()) / 32767.0 * 1.0 + 0.0);
    if (At<float>(ms_pDropChance, 0x18) > chance) {
        void* inventory = At<void*>(ped, 0x1C4);
        Inventory_Begin(inventory);
        for (uint8_t* weapon = Inventory_Next(inventory); weapon; weapon = Inventory_Next(At<void*>(ped, 0x1C4))) {
            int model = At<int16_t>(weapon, 2);
            uint8_t* mi = GetModelInfo(model);
            int type = At<int32_t>(mi, 0x18);
            if ((type != 0 && type != 1) || !mi[0x68]) {
                continue;
            }
            if (model == ms_noDrop1 || model == ms_noDrop2 || model == ms_noDrop3 || model == ms_noDrop4 ||
                model == ms_noDrop5 || model == ms_noDrop6) {
                continue;
            }
            if (At<int32_t>(ped, 0x152C) != 0 || type != 0 || Ped_Fn_478090(ped, model)) {
                models[count++] = model;
                break;
            }
        }
    }
    if (count == 1) {
        col::CVector pos;
        VectorAdd(&pos, &EntityPosition(ped), &offset);
        AddToPed(ped, Pickups_GenerateNewOne(pos.x, pos.y, pos.z, models[0], nullptr, 0, 0, 0, 0));
        return;
    }
    if (count != 2) {
        return;
    }
    uint8_t* matrix = At<uint8_t*>(ped, 0x14);
    col::CVector position = EntityPosition(ped);
    float heading;
    if (matrix) {
        // (0x85CF1A: the CRT's x87 atan2)
        heading = float(std::atan2(-double(At<float>(matrix, 0x10)), double(At<float>(matrix, 0x14))));
    } else {
        heading = At<float>(ped, 0x10);
    }
    col::CVector dir;
    HeadingToVector(&dir, heading);
    col::CVector half = {float(double(dir.x) * 0.5), float(double(dir.y) * 0.5), float(double(dir.z) * 0.5)};
    col::CVector left;
    VectorSub(&left, &position, &half);
    half = {float(double(dir.x) * 0.5), float(double(dir.y) * 0.5), float(double(dir.z) * 0.5)};
    col::CVector right;
    VectorAdd(&right, &position, &half);
    float colPoint[10];
    colPoint[0] = 0.0f;
    colPoint[1] = 0.0f;
    colPoint[2] = 0.0f;
    *reinterpret_cast<int32_t*>(&colPoint[3]) = 0;   // (the entity hit)
    colPoint[4] = 0.0f;
    colPoint[5] = 0.0f;
    colPoint[6] = 0.0f;
    // (colPoint[7], [8] left unset, as the game's stack)
    colPoint[9] = 3.4028234663852886e+38f;
    DropToGround(ped, left, colPoint);
    DropToGround(ped, right, colPoint);
    col::CVector pos;
    VectorAdd(&pos, &left, &offset);
    AddToPed(ped, Pickups_GenerateNewOne(pos.x, pos.y, pos.z, models[0], nullptr, 0, 0, 0, 0));
    VectorAdd(&pos, &right, &offset);
    AddToPed(ped, Pickups_GenerateNewOne(pos.x, pos.y, pos.z, models[1], nullptr, 0, 0, 0, 0));
    void* hit = *reinterpret_cast<re::Ptr32<void>*>(&colPoint[3]);
    if (hit) {
        (void)ms_pEntity_C2A8E8;   // (both branches of its test the same)
        Entity_CleanUpReference(hit, reinterpret_cast<re::Ptr32<void>*>(&colPoint[3]));
    }
}

// 0x4448F0 (thiscall on a pickup): its object's model reference dropped (flag 1) and the
// object deleted (when asked), its effect stopped; regenerating ones get their time
static void __fastcall Remove(uint8_t* self, re::Edx, int deleteObject, int regenerate) {
    uint8_t* object = At<uint8_t*>(self, 0x1C);
    if (object && (self[0x46] & 2)) {
        ModelInfo_RemoveRef(ms_modelInfoPtrs[At<int16_t>(object, 0x10E)]);
        int model = At<int32_t>(self, 0x30);
        if (model >= ms_weaponRangeLow && model <= ms_weaponRangeHigh) {
            Fn_4DA680(model, 1);
        }
        self[0x46] &= 0xFD;
    }
    object = At<uint8_t*>(self, 0x1C);
    if (object && re::Flag(deleteObject)) {
        World_Remove(object);
        Entity_DeleteRwObject(At<void*>(self, 0x1C));
        void* anim = At<void*>(At<uint8_t*>(self, 0x1C), 0x114);
        if (anim) {
            AnimObject_Destruct(anim);
            Free(anim);
        }
        if (void* o = At<void*>(self, 0x1C)) {
            VCall<void>(o, 0, 1);   // deleted
        }
    }
    At<void*>(self, 0x1C) = nullptr;
    EffectProxy_Stop(self);
    Pickups_RemoveFromList(self);
    if (!re::Flag(regenerate)) {
        return;
    }
    uint8_t flags = self[0x46];
    if ((flags & 1) && !(flags & 0x10)) {
        double delay = double(At<float>(At<uint8_t*>(self, 0x20), 0x1C)) * 1000.0;
        self[0x44] = 0;
        At<uint32_t>(self, 0x24) = uint32_t(int64_t(delay + double(ms_timeInMs)));
        return;
    }
    self[0x46] = flags & 0xF7;
}

// 0x444A10 (thiscall on a pickup): gone for good: stored first when kept
static void __fastcall Collect(uint8_t* self, re::Edx, int deleteObject) {
    if (ShouldStore(self)) {
        Pickups_StoreCollected(self);
    }
    Pickup_Remove(self, deleteObject, 0);
    self[0x46] &= 0xF7;
}

// 0x444A50: CPickups::Init: every pickup removed (kept ones stored) and cleared, the
// colours to black, the collected tables emptied, colours.dat, the default butes
static void __cdecl Init() {
    ms_bFlag_C0A0AC = 0;
    for (int i = 0; i < 300; i++) {
        uint8_t* p = Pickup(i);
        p[0x46] &= 0xF7;
        At<int16_t>(p, 0x40) = 1;
        if (At<int32_t>(p, 0x1C) != 0) {
            if (ShouldStore(p)) {
                Pickups_StoreCollected(p);
            }
            Pickup_Remove(p, 1, 0);
            p[0x46] &= 0xF7;
        }
        At<int32_t>(p, 0x1C) = 0;
        At<int32_t>(p, 0x3C) = 0;
        p[0x45] = 0;
    }
    for (int i = 0; i < 8; i++) {
        uint8_t rgba[4];
        uint8_t* c = RGBA_Construct(rgba, 0, 0, 0, 0xFF);
        ms_colours[i * 4] = c[0];
        ms_colours[i * 4 + 1] = c[1];
        ms_colours[i * 4 + 2] = c[2];
        ms_colours[i * 4 + 3] = c[3];
    }
    ms_collectedNext = 0;
    for (int i = 0; i < 100; i++) {
        ms_collected[i * 2] = -1;
    }
    ms_numCollectedKeys = 0;
    for (int i = 0; i < 250; i++) {
        ms_collectedKeys[i] = -1;
    }
    Pickups_LoadColours();
    PickupButes_Fn_729670(ms_defaultButes);
}

// 0x444B50: the pickups of a model removed (kept ones stored)
static void __cdecl RemoveByModel(int32_t model) {
    for (int i = 0; i < 300; i++) {
        uint8_t* p = Pickup(i);
        if ((p[0x46] & 8) && At<int32_t>(p, 0x30) == model) {
            if (ShouldStore(p)) {
                Pickups_StoreCollected(p);
            }
            Pickup_Remove(p, 1, 0);
            p[0x46] &= 0xF7;
        }
    }
}

// 0x444BB0: the pickups whose butes have +0x2D set removed (kept ones stored)
static void __cdecl RemoveFlagged() {
    for (int i = 0; i < 300; i++) {
        uint8_t* p = Pickup(i);
        if ((p[0x46] & 8) && At<uint8_t*>(p, 0x20)[0x2D]) {
            if (ShouldStore(p)) {
                Pickups_StoreCollected(p);
            }
            Pickup_Remove(p, 1, 0);
            p[0x46] &= 0xF7;
        }
    }
}

// 0x444C10 (thiscall on a pickup): given to the ped by the model's type: 0 a weapon (or
// 0x5A4CB0's without +0x64) and its ammo (butes +0x28 - 1), 1 ammo, 2-4 (with +0x1570
// 2) the player's items, 5 health, 6, 7 (a stat); the sound (the player's, when asked),
// its key collected, the counter (+0x18) told, then removed (regenerating). 0 when the
// ped can't take it (the player's full ones go onto the list at 0xC09960)
static uint8_t __fastcall GiveToPed(uint8_t* self, re::Edx, uint8_t* ped, int playSound) {
    uint8_t* mi = GetModelInfo(At<int32_t>(self, 0x30));
    int sound = -1;
    bool isPlayer = At<int32_t>(ped, 0x1310) == 0xD;
    void* inventory = At<void*>(ped, 0x1C4);
    switch (At<int32_t>(mi, 0x18)) {
    case 0: {
        if (At<int32_t>(mi, 0x64) == 0) {
            Fn_5A4CB0(reinterpret_cast<void*>(0x00CF4378), At<int32_t>(self, 0x30));
            sound = 6;
            break;
        }
        bool gaveWeapon = false;
        bool gaveAmmo = false;
        if (Inventory_Count(inventory, At<int32_t>(self, 0x30)) == 0) {
            Inventory_Give(At<void*>(ped, 0x1C4), At<int32_t>(self, 0x30), 1, isPlayer, 0);
            gaveWeapon = true;
        } else if (isPlayer && (At<int32_t>(self, 0x30) == ms_noDrop2 || At<int32_t>(self, 0x30) == ms_noDrop1)) {
            Pickups_AddToList(self);
            return 0;
        }
        if (At<int32_t>(At<uint8_t*>(self, 0x20), 0x28) > 1) {
            ModelInfo_Fn_7282A0(mi);
            if (At<int32_t>(mi, 0x84) != -1) {
                ModelInfo_Fn_7282A0(mi);
                uint8_t* ammo = GetModelInfo(At<int32_t>(mi, 0x84));
                ModelInfo_Fn_7282A0(mi);
                if (Inventory_Count(At<void*>(ped, 0x1C4), At<int32_t>(mi, 0x84)) < At<int32_t>(ammo, 0x34)) {
                    // (the flag's upper bytes are the stack's, unset)
                    int flag = (isPlayer && !gaveWeapon) ? 1 : 0;
                    int count = At<int32_t>(At<uint8_t*>(self, 0x20), 0x28);
                    ModelInfo_Fn_7282A0(mi);
                    Inventory_Give(At<void*>(ped, 0x1C4), At<int32_t>(mi, 0x84), count - 1, flag, 0);
                    gaveAmmo = true;
                } else if (isPlayer) {
                    Pickups_AddToList(self);
                }
            }
        }
        if (!gaveWeapon && !gaveAmmo) {
            return 0;
        }
        sound = 6;
        break;
    }
    case 1:
        if (Inventory_Count(inventory, At<int32_t>(self, 0x30)) >= At<int32_t>(mi, 0x34)) {
            if (isPlayer) {
                Pickups_AddToList(self);
            }
            return 0;
        }
        Inventory_Give(At<void*>(ped, 0x1C4), At<int32_t>(self, 0x30), At<int32_t>(At<uint8_t*>(self, 0x20), 0x28),
                       isPlayer, 0);
        sound = 6;
        break;
    case 2:
    case 3:
    case 4:
        if (ped[0x1570] != 2) {
            return 0;
        }
        if (isPlayer) {
            if (Fn_541E00(At<int32_t>(self, 0x30)) >= At<int32_t>(mi, 0x34)) {
                Pickups_AddToList(self);
                return 0;
            }
            if (self[0x46] & 0x10) {
                Fn_428A10(reinterpret_cast<void*>(0x00BCEB50), At<uint16_t>(self, 0x42), 1);
                Fn_428B00(reinterpret_cast<void*>(0x00BCEB50), At<uint16_t>(self, 0x42));
            }
            Fn_541C30(At<int32_t>(self, 0x30), 1, 1, 0);
        }
        switch (At<int32_t>(mi, 0x18)) {
        case 2:
            sound = 8;
            break;
        case 3:
            sound = 10;
            break;
        case 4:
            sound = 9;
            break;
        }
        break;
    case 5: {
        float a = At<float>(ped, 0x1CA4);
        float b = At<float>(ped, 0x1CB8);
        if (a < b || a == b) {
            return 0;
        }
        Ped_AddHealth(ped, float(At<int32_t>(At<uint8_t*>(self, 0x20), 0x28)));
        if (isPlayer) {
            Player_Fn_446B40(reinterpret_cast<void*>(0x00C1AEA8));
            Fn_458480(0x28, 1);
            Fn_541110(ms_pObj_C67554, At<int32_t>(self, 0x30), 0, 1, 0);
        }
        sound = 0xB;
        break;
    }
    case 7: {
        double limit = double(At<float>(ped, 0x18C0));
        float value = Ped_Fn_47CF10(ped);
        if (value < limit || value == limit) {
            return 0;
        }
        Ped_Fn_47CD90(ped, float(At<int32_t>(At<uint8_t*>(self, 0x20), 0x28)), 0);
        sound = 5;
        break;
    }
    case 6:
        sound = 5;
        break;
    default:
        break;
    }
    if (uint8_t(playSound) && isPlayer && sound != -1) {
        Sounds_Play(reinterpret_cast<void*>(0x00CF3DA0), sound);
    }
    uint8_t flags = self[0x46];
    if (!(flags & 0x10) && (flags & 0x20) && !(flags & 1) && !At<uint8_t*>(self, 0x20)[0x2C]) {
        Pickups_AddCollectedKey(self);
    }
    if (At<void*>(self, 0x18) && !(self[0x46] & 1)) {
        Counter_Fn_5D9900(At<void*>(self, 0x18), At<int32_t>(self, 0x34), 1);
    }
    Pickup_Remove(self, 1, 1);
    return 1;
}

// 0x444FC0 (thiscall on a pickup): a falling one (flag 2 of its flags) moves down 0.12:
// on hitting something (a 0.03 sphere) it lands (the model's height above the hit, its
// landing sound); outside, below ground level on models with +0x25, it sits at 0 (and
// 0x4D3570 is told); its object follows
static void __fastcall ProcessDrop(uint8_t* self, re::Edx) {
    uint8_t* mi = GetModelInfo(At<int32_t>(self, 0x30));
    if (!(self[0x46] & 4)) {
        return;
    }
    col::CVector* pos = reinterpret_cast<col::CVector*>(self + 8);
    float newZ = float(double(pos->z) - 0.12000000476837158);
    col::CVector from = {pos->x, pos->y, newZ};
    float colPoint[10];
    colPoint[0] = 0.0f;
    colPoint[1] = 0.0f;
    colPoint[2] = 0.0f;
    *reinterpret_cast<int32_t*>(&colPoint[3]) = 0;   // (the entity hit)
    colPoint[4] = 0.0f;
    colPoint[5] = 0.0f;
    colPoint[6] = 0.0f;
    // (colPoint[7], [8] left unset, as the game's stack)
    colPoint[9] = 3.4028234663852886e+38f;
    col::CVector height = {0.0f, 0.0f, At<float>(mi, 0x44)};
    col::CVector to;
    re::Ptr32<void> hitEntity;   // (0x4616A0's out)
    VectorSub(&to, &from, &height);
    if (!Fn_4616A0(pos, &to, 0.029999999329447746f, colPoint, &hitEntity, 9, 0xF, 1)) {
        if (ms_currArea == 0 && !(0.0f < pos->z) && pos->z == pos->z && mi[0x25]) {
            self[0x46] &= 0xFB;
            pos->z = 0.0f;
            Fn_4D3570(pos, 0.0f, 1);
        } else {
            pos->z = newZ;
        }
    } else {
        self[0x46] &= 0xFB;
        pos->z = At<float>(mi, 0x44) + colPoint[2];
        if (At<int32_t>(mi, 0x74)) {
            void* sound = Sounds_Fn_595B80(reinterpret_cast<void*>(0x00CF3DA0), At<int32_t>(mi, 0x74));
            Sounds_Fn_59B8A0(reinterpret_cast<void*>(0x00CF3DA0), sound, pos, At<int32_t>(mi, 0x7C), 0, 0, 0x78, 0, 0, 0, 0,
                             6, 1, 1.0f, 0);
        }
    }
    Placeable_SetPosition(At<void*>(self, 0x1C), pos->x, pos->y, pos->z);
    void* hit = *reinterpret_cast<re::Ptr32<void>*>(&colPoint[3]);
    if (hit) {
        Entity_CleanUpReference(hit, reinterpret_cast<re::Ptr32<void>*>(&colPoint[3]));
    }
}

// 0x445170: the pickup object's look each frame (the pickup found by its object): its
// drop, the glow (models with +0x20 1 or 3: the pickup's effect at the object, pulled
// towards the camera by +0x30) and pulse (2 or 3: the object's colour) between 0 and 1,
// its rotation (the model's +0x38, +0x3C degrees, the heading, spinning with +0x27) and
// scale (+0x2C), then the object's vf 0x4C when it's animated
static void __cdecl UpdateObject(uint8_t* object) {
    uint8_t* self = Pickups_FindPickupBy1C(object);
    if (!self) {
        return;
    }
    Pickup_ProcessDrop(self);
    uint8_t* mi = GetModelInfo(At<int32_t>(self, 0x30));
    int kind = At<int32_t>(mi, 0x20);
    bool glow = kind == 1 || kind == 3;
    bool pulse = kind == 2 || kind == 3;
    uint32_t time = ms_timeInMs;
    if (glow || pulse) {
        float level = At<float>(self, 0x2C) + At<float>(self, 0x28);
        At<float>(self, 0x28) = level;
        if (level >= 1.0f) {
            At<float>(self, 0x28) = 1.0f;
            At<float>(self, 0x2C) = -0.05000000074505806f;
        } else if (level <= 0.0f) {
            At<float>(self, 0x28) = 0.0f;
            At<float>(self, 0x2C) = 0.05000000074505806f;
        }
    }
    if (glow && !ms_bNoGlow_20C5BE6) {
        if (!At<void*>(self, 4)) {
            Effects_Start(ms_pEffects, reinterpret_cast<void*>(0x020C8FD0), self, 0);
        }
        float transform[13];   // (an NiTransform: rotation, translation, scale)
        transform[12] = 1.0f;
        Matrix_ToTransform(Entity_GetMatrix(object), transform);
        float* camera = RwFrameGetLTM(RwCameraGetFrame(g_rwCamera)) + 9;
        col::CVector d = {camera[0] - transform[9], camera[1] - transform[10], camera[2] - transform[11]};
        Vector_Normalise(&d);
        float pull = At<float>(mi, 0x30);
        col::CVector delta = {d.x * pull, d.y * pull, pull * d.z};
        transform[9] = transform[9] + delta.x;
        transform[10] = transform[10] + delta.y;
        transform[11] = transform[11] + delta.z;
        EffectProxy_SetTransform(self, transform);
    }
    if (pulse) {
        float v = float((double(At<float>(self, 0x28)) + 0.75) * 0.5);
        uint8_t* entity = At<uint8_t*>(self, 0x1C);
        At<float>(entity, 0x210) = v;
        At<float>(entity, 0x214) = v;
        At<float>(entity, 0x218) = v;
        At<float>(entity, 0x21C) = 1.0f;
        At<uint8_t*>(self, 0x1C)[0x209] |= 0x10;
    }
    uint8_t* matrix = static_cast<uint8_t*>(Entity_GetMatrix(object));
    col::CVector saved = *reinterpret_cast<col::CVector*>(matrix + 0x30);
    float angles[3];
    angles[0] = float(double(At<int32_t>(mi, 0x38)) * 3.1415927410125732 / 180.0);
    angles[1] = float(3.1415927410125732 * At<int32_t>(mi, 0x3C) / 180.0);
    float heading = At<float>(self, 0x14);
    angles[2] = heading;
    if (mi[0x27]) {
        angles[2] = float(double(time & 0x7FF) * 0.003056640736758709 + heading);
    }
    Matrix_SetRotate(matrix, angles);
    float scale = At<float>(mi, 0x2C);
    float* m = reinterpret_cast<float*>(matrix);
    m[0] = m[0] * scale;
    m[1] = m[1] * scale;
    m[2] = m[2] * scale;
    m[4] = scale * m[4];
    m[5] = m[5] * scale;
    m[6] = m[6] * scale;
    m[8] = scale * m[8];
    m[9] = scale * m[9];
    m[10] = scale * m[10];
    *reinterpret_cast<col::CVector*>(matrix + 0x30) = saved;
    if (pulse) {
        m[15] = 1.0f;   // (without the pulse the game writes an unset stack word here)
    }
    if (At<void*>(object, 0x114)) {
        VCall<void>(object, 0x4C);
    }
}

// 0x4454F0: the handle's pickup collected (GetActualPickupIndex inlined)
static void __cdecl RemovePickup(int32_t handle, int deleteObject) {
    int slot = handle & 0xFFFF;
    if (handle == -1 || !(Pickup(slot)[0x46] & 8) || int16_t(uint32_t(handle) >> 16) != At<int16_t>(Pickup(slot), 0x40)) {
        return;
    }
    if (slot < 0) {
        return;
    }
    Pickup_Collect(Pickup(slot), deleteObject);
}

// 0x445540 (thiscall on a pickup): its object made (a CObject of its model, or the model
// the info's +0x58 names; with an animation when the model has one) and, once the model
// is streamed in (weapons: once 0x4D9790 allows), its RW object, heading and scale set up.
// 0 until then
static uint8_t __fastcall CreateObject(uint8_t* self, re::Edx) {
    uint8_t* mi = GetModelInfo(At<int32_t>(self, 0x30));
    int32_t model = At<int32_t>(self, 0x30);
    if (At<const char*>(mi, 0x58)) {
        ModelInfo_GetByName(At<const char*>(mi, 0x58), &model);
    }
    if (!At<void*>(self, 0x1C)) {
        void* memory = Object_New(0x22C);
        uint8_t* object = memory ? Object_Construct(memory, model, 0) : nullptr;
        At<uint8_t*>(self, 0x1C) = object;
        object[0x208] = 2;
        At<int32_t>(At<uint8_t*>(self, 0x1C), 0x44) = 1;
        At<uint8_t*>(self, 0x1C)[0x1BC] &= 0xFD;
        At<int32_t>(At<uint8_t*>(self, 0x1C), 0x20) = 0;
        At<uint8_t*>(self, 0x1C)[0x209] |= 1;
        At<int32_t>(At<uint8_t*>(self, 0x1C), 0xBC) = 1;
        At<uint8_t*>(self, 0x1C)[0x209] &= 0xEF;
        Placeable_SetPosition(At<void*>(self, 0x1C), At<float>(self, 8), At<float>(self, 0xC), At<float>(self, 0x10));
        if (VCall<int32_t>(ms_modelInfoPtrs[model], 0x1C) != -1) {
            void* anim = Malloc(0x104);
            At<void*>(At<uint8_t*>(self, 0x1C), 0x114) = anim ? AnimObject_Construct(anim) : nullptr;
        }
        World_Add(At<void*>(self, 0x1C));
    }
    if (At<void*>(At<uint8_t*>(self, 0x1C), 0x18)) {
        return 1;
    }
    int32_t weapon = At<int32_t>(self, 0x30);
    bool isWeapon = weapon >= ms_weaponRangeLow && weapon <= ms_weaponRangeHigh;
    uint8_t allowed = 1;
    if (isWeapon) {
        allowed = Fn_4D9790(weapon, 0) & 1;
    }
    if (Streaming_GetInfo(model)[4] != 1) {
        Streaming_RequestModel(model, 4);
        return 0;
    }
    if (!allowed) {
        return 0;
    }
    ModelInfo_AddRef(ms_modelInfoPtrs[model]);
    if (isWeapon) {
        Fn_4DA680(At<int32_t>(self, 0x30), 0);
    }
    self[0x46] |= 2;
    VCall<void>(At<void*>(self, 0x1C), 0x28, 1, 1);   // CreateRwObject
    float heading = At<float>(self, 0x14);
    uint8_t* object = At<uint8_t*>(self, 0x1C);
    if (At<void*>(object, 0x14)) {
        Matrix_SetRotateZOnly(At<void*>(object, 0x14), heading);
    }
    At<float>(object, 0x10) = heading;
    Entity_UpdateFromRW(At<void*>(self, 0x1C));
    Physical_Fn_465760(At<void*>(self, 0x1C));
    object = At<uint8_t*>(self, 0x1C);
    if (VCall<int32_t>(ms_modelInfoPtrs[At<int16_t>(object, 0x10E)], 0x1C) != -1) {
        bool was = At<int32_t>(At<uint8_t*>(self, 0x1C), 0xE4) != 0;
        At<int32_t>(At<uint8_t*>(self, 0x1C), 0xE4) = 1;
        VCall<void>(At<void*>(self, 0x1C), 0x4C);
        At<int32_t>(At<uint8_t*>(self, 0x1C), 0xE4) = was;
    }
    return 1;
}

// 0x445790: a pickup where the entity is (its model, value +0x124, area +0x110; flag 6),
// its object made at once, the id (+0x42) and value set, into the IPL store's slot
static int32_t __cdecl CreateFromEntity(int id, uint8_t* entity) {
    uint8_t area = entity[0x110];
    int32_t value = At<int32_t>(entity, 0x124);
    col::CVector& pos = EntityPosition(entity);
    int32_t handle = Pickups_GenerateNewOne(pos.x, pos.y, pos.z, At<int16_t>(entity, 0x10E), nullptr, 1, value, area, 0);
    int slot = handle & 0xFFFF;
    if (handle == -1 || !(Pickup(slot)[0x46] & 8) || int16_t(uint32_t(handle) >> 16) != At<int16_t>(Pickup(slot), 0x40) ||
        slot < 0) {
        return handle;
    }
    uint8_t* p = Pickup(slot);
    At<int16_t>(p, 0x42) = int16_t(id);
    Pickup_CreateObject(p);
    At<int32_t>(At<uint8_t*>(p, 0x1C), 0x228) = At<int32_t>(p, 0x3C);
    At<uint8_t*>(p, 0x1C)[0x110] = p[0x45];
    IplStore_IncludeEntity(p[0x45], At<uint8_t*>(p, 0x1C));
    return handle;
}

// 0x445860 (thiscall on a pickup): each frame: some models tell 0x52ABE0; beyond 40 of
// the player (0x41A4D0) an object without its RW object goes; near it, a pickup in play
// (+0x44 1) times out (removed, regenerating) or gets its object and is given to the
// player on touching (within 2 in height, on foot (+0x149C), alive, models with +0x25);
// one waiting to come back (flag 1) does once its time is up and the player is 10 away
// (or 0xC0A0AC says so)
static void __fastcall Update(uint8_t* self, re::Edx) {
    int32_t model = At<int32_t>(self, 0x30);
    if ((model == ms_model_BF4328 || model == ms_model_BF4334) && !(self[0x46] & 4)) {
        Fn_52ABE0(reinterpret_cast<col::CVector*>(self + 8), model, self);
    }
    uint8_t* player = ms_pPlayerPed;
    col::CVector* position = reinterpret_cast<col::CVector*>(self + 8);
    float far = IsWithinDistance(&EntityPosition(player), position, 40.0f);
    if (!(far < 0.0f || far == 0.0f)) {
        uint8_t* object = At<uint8_t*>(self, 0x1C);
        if (object && !At<void*>(object, 0x18) && !(self[0x46] & 0x10)) {
            Pickup_Remove(self, 1, 0);
        }
        return;
    }
    if (self[0x44] == 1) {
        uint8_t* mi = GetModelInfo(At<int32_t>(self, 0x30));
        if (!(self[0x46] & 0x10) && At<uint8_t*>(self, 0x20)[0x20] && ms_timeInMs > At<uint32_t>(self, 0x24)) {
            if (mi[0x28] || !At<void*>(self, 0x1C) || !Entity_Fn_467460(At<void*>(self, 0x1C))) {
                Pickup_Remove(self, 1, 1);
                return;
            }
        }
        if (!Pickup_CreateObject(self)) {
            return;
        }
        Object_Fn_4CF620(At<void*>(self, 0x1C));
        player = ms_pPlayerPed;
        if (!(self[0x46] & 4) && At<int32_t>(player, 0x149C) && mi[0x25] && 0.0f < At<float>(player, 0x1CB8)) {
            col::CVector& pp = EntityPosition(player);
            float dz = std::fabs(float(double(pp.z) - At<float>(self, 0x10)));
            if (!(dz > 2.0f)) {
                float touch = IsWithinDistance(&pp, position, 1.0f);
                if (touch < 0.0f || touch == 0.0f) {
                    Pickup_GiveToPed(self, ms_pPlayerPed, 1);
                    return;
                }
            }
        }
        Pickups_RemoveFromList(self);
        return;
    }
    if (!(self[0x46] & 1) || !(ms_timeInMs > At<uint32_t>(self, 0x24))) {
        return;
    }
    float away = IsWithinDistance(&EntityPosition(ms_pPlayerPed), position, 10.0f);
    if ((away < 0.0f || away == 0.0f) && !ms_bFlag_C0A0AC) {
        return;
    }
    uint8_t* butes = At<uint8_t*>(self, 0x20);
    if (butes[0x20]) {
        At<uint32_t>(self, 0x24) = uint32_t(int64_t(double(At<float>(butes, 0x24)) * 1000.0 + double(ms_timeInMs)));
    }
    self[0x44] = 1;
}

// 0x445AC0: every pickup in use updated
static void __cdecl UpdateAll() {
    for (int i = 0; i < 300; i++) {
        if (Pickup(i)[0x46] & 8) {
            Pickup_Update(Pickup(i));
        }
    }
}

} // namespace Pickups3

// ---- registration ----------------------------------------------------------------------

#define PICKUPS3_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace Pickups3 {
PICKUPS3_REVERSED("CPickups::DropForDeadPed", 0x00444340, &DropForDeadPed);
PICKUPS3_REVERSED("CPickup::Remove", 0x004448F0, &Remove);
PICKUPS3_REVERSED("CPickup::Collect", 0x00444A10, &Collect);
PICKUPS3_REVERSED("CPickup::GiveToPed", 0x00444C10, &GiveToPed);
PICKUPS3_REVERSED("CPickups::Init", 0x00444A50, &Init);
PICKUPS3_REVERSED("CPickups::RemoveByModel", 0x00444B50, &RemoveByModel);
PICKUPS3_REVERSED("CPickups::RemoveFlagged", 0x00444BB0, &RemoveFlagged);
PICKUPS3_REVERSED("CPickup::ProcessDrop", 0x00444FC0, &ProcessDrop);
PICKUPS3_REVERSED("CPickups::UpdateObject", 0x00445170, &UpdateObject);
PICKUPS3_REVERSED("CPickups::RemovePickup", 0x004454F0, &RemovePickup);
PICKUPS3_REVERSED("CPickup::CreateObject", 0x00445540, &CreateObject);
PICKUPS3_REVERSED("CPickups::CreateFromEntity", 0x00445790, &CreateFromEntity);
PICKUPS3_REVERSED("CPickup::Update", 0x00445860, &Update);
PICKUPS3_REVERSED("CPickups::Update", 0x00445AC0, &UpdateAll);
} // namespace Pickups3
