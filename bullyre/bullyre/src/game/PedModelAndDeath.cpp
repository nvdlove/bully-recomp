// CPed (0x4785D0 .. 0x479B40): member structs' constructors and destructors, the model
// (vf8 SetModelIndex), the ped stats it's given, death (vf42), and its ground matrix
// (+0x1674, from a sphere cast below it).

#include <cmath>

#include "game/Ped.h"
#include "game/Pool.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;
const ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

constexpr int PED_TYPE_PLAYER = 0xD;

GAME_VAR(CPed*, PM_player, 0x00C1AEA8);               // FindPlayerPed
GAME_VAR(uint32_t, PM_timeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, PM_currArea, 0x00BD1008);
GAME_ARRAY(uint8_t*, PM_modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs
GAME_VAR(ped::CVector, PM_zeroVector, 0x00BCC564);
GAME_VAR(uint8_t*, PM_defaultSpheres, 0x020C88A0);    // the collision spheres without a model's own
GAME_VAR(CPool<CPed>*, PM_pedPool, 0x00C0F5F0);
GAME_VAR(CPool<uint8_t>*, PM_1D24Pool, 0x020C7C84);
GAME_VAR(void*, PM_radar, 0x00C674CC);
GAME_VAR(void*, PM_population, 0x00C2C108);
GAME_VAR(void*, PM_C2C1A0, 0x00C2C1A0);
GAME_VAR(float, PM_f_minusOne, 0x009009AC);
GAME_VAR(float, PM_deadHealth, 0x00906520);           // -100
GAME_VAR(float, PM_f_0_0001, 0x0090CF18);
GAME_VAR(double, PM_d_pi, 0x00900160);
GAME_VAR(double, PM_d_180, 0x00900158);
GAME_VAR(int32_t, PM_model4164, 0x00BF4164);
GAME_VAR(int32_t, PM_chapter, 0x00C0F6B8);
GAME_VAR(int32_t, PM_weapon436C, 0x00BF436C);
GAME_VAR(int32_t, PM_modelRat, 0x00BF42DC);
// The ground matrix's sphere cast
GAME_VAR(double, PM_d_0_45, 0x00900E48);
GAME_VAR(double, PM_d_0, 0x009009A0);
GAME_VAR(double, PM_d_2_5, 0x00900AB0);
GAME_VAR(double, PM_d_0_25, 0x00900498);
GAME_VAR(double, PM_d_0_5, 0x008FF1F8);
GAME_VAR(float, PM_castRadius, 0x0090CD88);           // 0.25
GAME_VAR(float, PM_fltMax, 0x009009B8);
GAME_VAR(uint8_t, PM_C1AE80, 0x00C1AE80);
GAME_VAR(uint8_t, PM_C1AE81, 0x00C1AE81);
inline void* const PM_actionContexts = reinterpret_cast<void*>(0x00D02850);
inline void* const PM_BD1078 = reinterpret_cast<void*>(0x00BD1078);

GAME_FN(void, __cdecl, PM_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, PM_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, PM_Refs474E90_Destruct, 0x00474E90, void* self);
GAME_FN(void, __thiscall, PM_Refs474E60_Destruct, 0x00474E60, void* self);
GAME_FN(void, __thiscall, PM_ActionHandleSet, 0x005D7F10, void* handle, uintptr_t node);
GAME_FN(void, __stdcall, PM_VectorDestructorIterator, 0x004017C0, void* array, uint32_t size, int count, void* dtor);
GAME_FN(void, __stdcall, PM_VectorConstructorIterator, 0x00401240, void* array, uint32_t size, int count, void* ctor);
GAME_FN(void, __thiscall, PM_Fn45BD20, 0x0045BD20, void* self);
GAME_FN(float, __thiscall, PM_VectorNormalise4294A0, 0x004294A0, ped::CVector* v);
GAME_FN(void, __thiscall, PM_RadarClearBlips, 0x0054A890, void* radar, int kind, int handle);
GAME_FN(uint8_t*, __cdecl, PM_GetStreamingInfo, 0x0052DD30, int model);
GAME_FN(void, __cdecl, PM_RequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __cdecl, PM_Fn43C4F0, 0x0043C4F0, const char* a, const char* b, uintptr_t c);
GAME_FN(void, __cdecl, PM_LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __thiscall, PM_EntitySetModelIndex, 0x00468660, CPed* ped, int model, int a);
GAME_FN(void, __thiscall, PM_PedModelInfoFn522940, 0x00522940, void* modelInfo, void* rwObject);
GAME_FN(void, __thiscall, PM_PedFindLimbNodes, 0x00472FB0, CPed* ped);
GAME_FN(void, __thiscall, PM_Fn72FEE0, 0x0072FEE0, void* self, void* animBlend, CPed* ped);
GAME_FN(void, __thiscall, PM_MatrixConstruct412770, 0x00412770, void* self);
GAME_FN(void, __thiscall, PM_Intel484E60, 0x00484E60, void* intelligence, CPed* ped);
GAME_FN(void*, __thiscall, PM_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(void, __thiscall, PM_MatrixCopy, 0x00412A10, void* self, const void* from);
GAME_FN(void, __thiscall, PM_MatrixAssign, 0x00412720, void* self, const void* from);
GAME_FN(void, __thiscall, PM_PathHandleSet, 0x005FA880, void* self, void* node);
GAME_FN(void, __thiscall, PM_PathHandleSetName, 0x005FA870, void* self, int name);
GAME_FN(void, __thiscall, PM_PathKeyMake, 0x005FA8C0, void* self, int count, void* handles);
GAME_FN(uint8_t*, __cdecl, PM_FindSphereSet, 0x0072AEA0, void* key, void* node);
GAME_FN(void, __thiscall, PM_PathHandleDestruct, 0x0049A650, void* self);
GAME_FN(void*, __cdecl, PM_FindActionTree, 0x005F5B10, const char* path, uintptr_t a);
GAME_FN(void*, __thiscall, PM_ActionTreeRoot, 0x005F5120, void* tree);
GAME_FN(void, __thiscall, PM_ActionsSetTree, 0x005F45F0, void* controller, void* node);
GAME_FN(void, __thiscall, PM_ActionsSetFloat, 0x005F4B00, void* controller, float value);
GAME_FN(void, __thiscall, PM_InventoryGive, 0x0045D030, void* inventory, int model, int ammo, int a, int b);
GAME_FN(uint8_t*, __cdecl, PM_Fn51B450, 0x0051B450, int id);
GAME_FN(void, __thiscall, PM_Fn7282A0, 0x007282A0, void* self);
GAME_FN(uint8_t, __thiscall, PM_PickAttackResponse, 0x004780D0, CPed* ped, int* outId, int* outA);
GAME_FN(uint8_t, __thiscall, PM_WantsGift, 0x00474A30, CPed* ped);
GAME_FN(uint8_t, __thiscall, PM_Has0GiftRequirement, 0x00474B80, CPed* ped);
inline int __cdecl PM_rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, PM_Fn52FE40, 0x0052FE40, int model);
GAME_FN(void, __thiscall, PM_PopulationFn49EE70, 0x0049EE70, void* population, CPed* ped);
GAME_FN(void, __thiscall, PM_Fn4A67D0, 0x004A67D0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PM_Fn4A6670, 0x004A6670, void* self, int kind, CPed* ped, void* killer, uintptr_t a, float b,
        float c);
GAME_FN(void, __cdecl, PM_IncrementStat, 0x00458480, int stat, int amount);
GAME_FN(void, __cdecl, PM_Fn5930A0, 0x005930A0, CPed* ped, int a);
GAME_FN(uint8_t, __thiscall, PM_Ped4473E0, 0x004473E0, CPed* ped);
GAME_FN(void, __thiscall, PM_PedRunAction18E8, 0x00474D20, CPed* ped, int force);
GAME_FN(void, __thiscall, PM_Fn485A40, 0x00485A40, void* self);
GAME_FN(void, __thiscall, PM_Fn431330, 0x00431330, void* self, int a, int b, int c);
GAME_FN(void, __thiscall, PM_WeaponFn4D9410, 0x004D9410, void* weapon);
GAME_FN(void, __cdecl, PM_Fn444340, 0x00444340, CPed* ped);
GAME_FN(void, __thiscall, PM_SetTargetEntity, 0x00498B10, CPed* ped, CEntity* target);
GAME_FN(uint8_t, __cdecl, PM_ProcessSphereCast, 0x004616A0, const ped::CVector* start, const ped::CVector* end,
        float radius, void* colPoint, void* entity, int a, int flags, int b);
GAME_FN(void, __thiscall, PM_PedGroundMatrix, 0x00479650, CPed* ped, int a, int b, int c, int d);
GAME_FN(uint8_t, __thiscall, PM_ActionIsValid, 0x005D80E0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PM_ActionGetNode, 0x005D80B0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PM_ActionGetNode2, 0x005D80D0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PM_ActionGetInstance, 0x005D9C20, ped::ActionHandle* handle);
GAME_FN(void, __thiscall, PM_ActionRelease, 0x005D9C50, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PM_ContextsFind, 0x005DB8F0, void* contexts, void* node);
GAME_FN(void*, __thiscall, PM_ContextsGetCurrent, 0x005DB960, void* contexts);
GAME_FN(void, __thiscall, PM_ContextsSetCurrent, 0x005DBDA0, void* contexts, void* context);
GAME_FN(void, __cdecl, PM_Instance5D8270, 0x005D8270, void* instance);
GAME_FN(void, __cdecl, PM_Instance5D8670, 0x005D8670, void* instance, void* node);
GAME_FN(void, __cdecl, PM_Instance5D85C0, 0x005D85C0, void* instance, int value);
GAME_FN(void, __cdecl, PM_Instance5D82A0, 0x005D82A0, void* instance, int a, int b);

void ReleaseRef(void* self, uint32_t offset) {
    re::Ptr32<void>& ref = At<void*>(self, offset);
    if (ref) {
        PM_CleanUpOldReference(ref, &ref);
    }
}

int PedHandle(CPed* ped) {
    int slot = PM_pedPool->GetIndex(ped);
    return PM_pedPool->flags[slot] + (slot << 8);
}

// 0x4785D0 (thiscall): a ped member's destructor (its references let go)
void __fastcall PedMember4785D0_Destruct(uint8_t* self, re::Edx) {
    ReleaseRef(self, 0x190);
    ReleaseRef(self, 0x18C);
    ReleaseRef(self, 0x188);
    ReleaseRef(self, 0x184);
    ReleaseRef(self, 0x180);
    ReleaseRef(self, 0x17C);
    ReleaseRef(self, 0x14C);
    PM_Refs474E90_Destruct(self + 0x94);
    ReleaseRef(self, 0x88);
    ReleaseRef(self, 0x7C);
    PM_Refs474E60_Destruct(self + 4);
}

// 0x4786D0 (thiscall): the destructor of 0x478890's
void __fastcall PedMember478890_Destruct(uint8_t* self, re::Edx) {
    PM_ActionHandleSet(self + 0x354, 0);
    ReleaseRef(self, 0x350);
    ReleaseRef(self, 0x34C);
    ReleaseRef(self, 0x9C);
    for (int i = 2; i >= 0; i--) {
        PM_VectorDestructorIterator(self + 0x18 + i * 0x2C, 4, 10, reinterpret_cast<void*>(0x004498A0));
    }
}

// 0x478760 (thiscall, TrackOrCondition::sub_478760): a registered entity pointer set from
// another
re::Ptr32<CEntity>* __fastcall EntityPtr_Assign(re::Ptr32<CEntity>* self, re::Edx, const re::Ptr32<CEntity>* from) {
    CEntity* entity = *from;
    if (entity != *self) {
        if (*self) {
            PM_CleanUpOldReference(*self, self);
        }
        *self = entity;
        if (entity) {
            PM_RegisterReference(entity, self);
        }
    }
    return self;
}

// 0x478820 (cdecl): `in` normalised into `out`
ped::CVector* __cdecl Vector_Normalised(ped::CVector* out, const ped::CVector* in) {
    out->x = in->x;
    out->y = in->y;
    out->z = in->z;
    PM_VectorNormalise4294A0(out);
    return out;
}

// 0x478850 (thiscall, TrackOrCondition::sub_478850): its registered +0x18C set
void __fastcall PedMember_Set18C(uint8_t* self, re::Edx, CEntity* entity) {
    re::Ptr32<CEntity>* ref = &At<CEntity*>(self, 0x18C);
    if (entity != *ref) {
        if (*ref) {
            PM_CleanUpOldReference(*ref, ref);
        }
        *ref = entity;
        if (entity) {
            PM_RegisterReference(entity, ref);
        }
    }
}

// 0x478890 (thiscall, CMissionObjectiveMgr::sub_478890): a ped member's constructor
void* __fastcall PedMember478890_Construct(uint8_t* self, re::Edx, int owner) {
    for (int i = 0; i < 3; i++) {
        uint8_t* group = self + 0x14 + i * 0x2C;
        PM_VectorConstructorIterator(group + 4, 4, 10, reinterpret_cast<void*>(0x006D0EB0));
        At<int32_t>(group, 0) = 0;
    }
    At<int32_t>(self, 0x9C) = 0;
    PM_Fn45BD20(self + 0xA0);
    for (int i = 0; i < 4; i++) {
        uint8_t* entry = self + 0xD4 + i * 0x94;
        PM_VectorConstructorIterator(entry, 0x18, 6, reinterpret_cast<void*>(0x0066BEA0));
        At<int32_t>(entry, 0x90) = 0;
    }
    static const uint16_t zeroed[] = {0x34C, 0x350, 0x354, 0x35C, 0x370, 0x384, 0x398, 0x3AC, 0x3C0, 0x3D4, 0x3E8};
    for (uint16_t offset : zeroed) {
        At<int32_t>(self, offset) = 0;
    }
    At<int32_t>(self, 0) = owner;
    return self;
}

// 0x478960 (thiscall, CCivilianPed::sub_478960): its blips of kinds 2, 13 and 6 removed
void __fastcall CPed_ClearBlips(CPed* self, re::Edx) {
    PM_RadarClearBlips(PM_radar, 2, PedHandle(self));
    PM_RadarClearBlips(PM_radar, 0xD, PedHandle(self));
    PM_RadarClearBlips(PM_radar, 6, PedHandle(self));
}

// vf8 0x478A20: SetModelIndex (the model loaded first; its anims, limb nodes, the
// collision spheres from its sphere set, and the weapons' action tree)
void __fastcall CPed_SetModelIndex(CPed* self, re::Edx, int model, int a) {
    self->unknown1C98 = 0;
    if (PM_GetStreamingInfo(model)[4] != 1) {
        PM_RequestModel(model, 0x14);
        PM_Fn43C4F0(reinterpret_cast<const char*>(0x0090CF50), reinterpret_cast<const char*>(0x0090250C), 0);
        PM_LoadAllRequestedModels(0);
    }
    uint8_t* modelInfo = PM_modelInfoPtrs[model];
    if (At<int32_t>(modelInfo, 0x20) == 0) {
        PM_Fn43C4F0(reinterpret_cast<const char*>(0x0090CF38), reinterpret_cast<const char*>(0x0090250C), 0);
        PM_LoadAllRequestedModels(0);
    }
    PM_EntitySetModelIndex(self, model, a);
    At<uint32_t>(self->animBlend, 4) = 0x0068EE40;
    At<CPed*>(self->animBlend, 8) = self;
    if (self->animBlend) {
        At<void*>(self->animBlend, 0xA4) = Bytes(self) + 0x1720;
    }
    if (self->animBlend) {
        At<void*>(self->animBlend, 0xA8) = Bytes(self) + 0x172C;
    }
    At<float>(self, 0x172C) = 0.0f;
    At<float>(self, 0x1730) = 0.0f;
    At<float>(self, 0x1734) = 0.0f;
    At<float>(self, 0x1738) = 1.0f;
    if (At<int32_t>(modelInfo, 0x58) == 0) {
        PM_PedModelInfoFn522940(modelInfo, self->rwObject);
    }
    PM_PedFindLimbNodes(self);
    PM_Fn72FEE0(self->unknown2EC, self->animBlend, self);
    PM_MatrixConstruct412770(Bytes(self) + 0x1630);
    PM_MatrixConstruct412770(Bytes(self) + 0x1674);
    PM_Intel484E60(self->intelligence, self);
    alignas(4) uint8_t matrix[0x44];
    PM_MatrixCopy(matrix, PM_GetMatrix(self));
    At<ped::CVector>(self, 0x1564) = At<ped::CVector>(matrix, 0x30);
    static const uint16_t zeroVectors[] = {0x173C, 0x174C, 0x175C, 0x1774, 0x1784, 0x179C, 0x17AC,
                                           0x17C4, 0x17D4, 0x17EC, 0x17FC, 0x1814, 0x1824, 0x183C,
                                           0x184C, 0x1864, 0x1874, 0x188C, 0x1898};
    for (uint16_t offset : zeroVectors) {
        At<ped::CVector>(self, offset) = PM_zeroVector;
    }
    At<uint8_t>(self, 0x18A4) = 0;
    At<uint8_t>(self, 0x18A5) = 0;
    // the model's sphere set, by name under the node at 0x1B65310 (the default's without)
    void* handles[2];
    PM_PathHandleSet(&handles[0], reinterpret_cast<void*>(0x01B65310));
    PM_PathHandleSetName(&handles[1], At<int32_t>(PM_modelInfoPtrs[self->modelIndex], 4));
    uint8_t key[8];
    PM_PathKeyMake(key, 2, handles);
    uint8_t* spheres = PM_FindSphereSet(key, reinterpret_cast<void*>(0x01B65314));
    if (!spheres) {
        spheres = PM_defaultSpheres;
    }
    for (int i = 0; i < 6; i++) {
        At<float>(self, 0x1D7C + i * 4) = At<float>(spheres, 0x1C + i * 8);
        At<int32_t>(self, 0x1D64 + i * 4) = At<int32_t>(spheres, 0x18 + i * 8);
    }
    void* tree = PM_FindActionTree(reinterpret_cast<const char*>(0x0090CF1C), 0);  // "/Global/Weapons/NeedInit"
    void* arg = reinterpret_cast<void*(__thiscall*)(CPed*)>(static_cast<void*>(self->vtable[0x40 / 4]))(self);
    void* node = reinterpret_cast<void*(__thiscall*)(void*, void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(tree))[0x4C / 4]))(tree, arg);
    if (!node) {
        node = PM_ActionTreeRoot(tree);
    }
    PM_ActionsSetTree(self->intelligence + 0x258, node);
    PM_ActionsSetFloat(self->intelligence + 0x258, PM_f_0_0001);
    for (int i = 1; i >= 0; i--) {
        PM_PathHandleDestruct(&handles[i]);
    }
}

// 0x478F80 (thiscall): its ped stats (+0x1C9C, the model's by default) copied in: the
// stats, weapons (the attack response picked by chance, or sometimes a slingshot for a
// wanderer in chapters 2 and 3), health and the rest
void __fastcall CPed_SetPedStats(CPed* self, re::Edx, uint8_t* stats, int keepWeapons) {
    if (!stats) {
        if (At<uint8_t*>(self, 0x1C9C) == nullptr) {
            At<uint8_t*>(self, 0x1C9C) = At<uint8_t*>(PM_modelInfoPtrs[self->modelIndex], 0x54);
        }
        stats = At<uint8_t*>(self, 0x1C9C);
    } else {
        At<uint8_t*>(self, 0x1C9C) = stats;
    }
    for (int i = 0; i < 0x40; i++) {
        At<int32_t>(self, 0x1900 + i * 4) = At<int32_t>(stats, 0x1C + i * 4);
    }
    void* inventory = At<void*>(self, 0x1C4);
    if ((PM_rand() * 99) / 0x7FFF < At<int32_t>(stats, 0xCC)) {
        // (the game uses its two argument slots for these)
        int weapon = keepWeapons;
        int count = 0;
        if (PM_PickAttackResponse(self, &weapon, &count)) {
            PM_InventoryGive(inventory, weapon, 1, 1, 0);
            uint8_t* info = PM_Fn51B450(weapon);
            bool setCurrent;
            if (self->vehicle != nullptr || self->pedType == PED_TYPE_PLAYER) {
                setCurrent = true;
            } else {
                setCurrent = self->seatState == 2 || self->seatState == 3;
            }
            if (setCurrent && info[0x26] != 0) {
                At<int32_t>(self->intelligence, 0x18) = weapon;
            }
            PM_Fn7282A0(info);
            int ammoWeapon = At<int32_t>(info, 0x84);
            if (count == -1) {
                count = 1;
                At<int32_t>(self, 0x1374) = 1;
            }
            if (ammoWeapon == -1) {
                if (count >= 2) {
                    PM_InventoryGive(At<void*>(self, 0x1C4), weapon, count - 1, 1, 0);
                }
            } else {
                PM_InventoryGive(At<void*>(self, 0x1C4), ammoWeapon, count, 1, 0);
            }
        }
    } else if (static_cast<uint8_t>(keepWeapons) == 0 && PM_currArea == 0) {
        int type = self->pedType;
        if (type != PED_TYPE_PLAYER && type != 0 && type != 8 && type != 7 && self->modelIndex != PM_model4164 &&
            type != 3 && type != 4 && self->seatState == 1 && !PM_WantsGift(self) && !PM_Has0GiftRequirement(self) &&
            (PM_chapter == 2 || PM_chapter == 3) && At<int32_t>(stats, 0x54) <= 0 && (PM_rand() & 1) != 0) {
            PM_InventoryGive(At<void*>(self, 0x1C4), PM_weapon436C, 1, 1, 0);
            At<int32_t>(self->intelligence, 0x18) = PM_weapon436C;
        }
    }
    int extra = At<int32_t>(stats, 0xBC);
    if (extra >= 0) {
        PM_InventoryGive(At<void*>(self, 0x1C4), extra, 1, 1, 0);
    }
    float v28 = static_cast<float>(At<int32_t>(stats, 0x28));
    At<float>(self, 0xA6C) = v28;
    At<float>(self, 0xA70) = v28;
    self->unknownA68 = static_cast<float>(At<int32_t>(stats, 0x24) * PM_d_pi / PM_d_180);
    self->maxHealth = static_cast<float>(At<int32_t>(stats, 0x2C));
    At<float>(self, 0x1CA8) = 0.0f;
    self->unknown1C98 = 0;
    self->healthRegen = static_cast<float>(At<int32_t>(stats, 0x30));
    self->health = self->maxHealth;
    self->unknown1CBC = self->maxHealth;
    self->unknown18C0 = static_cast<float>(At<int32_t>(stats, 0x74));
    self->unknown18C8 = At<int32_t>(stats, 0x78);
    self->unknown18D0 = static_cast<float>(At<int32_t>(stats, 0x5C));
    At<float>(self, 0x1CB4) = static_cast<float>(At<int32_t>(stats, 0x3C));
    At<int32_t>(self, 0x714) = At<int32_t>(stats, 0x60);
}

// (inlined): the death action at +0x18E8 run in its context
void RunAction18E8(CPed* self) {
    ped::ActionHandle* handle = &At<ped::ActionHandle>(self, 0x18E8);
    void* context = PM_ContextsFind(PM_actionContexts, PM_ActionGetNode(handle));
    if (context) {
        void* saved = PM_ContextsGetCurrent(PM_actionContexts);
        PM_ContextsSetCurrent(PM_actionContexts, context);
        void* instance = PM_ActionGetInstance(handle);
        PM_Instance5D8270(instance);
        PM_Instance5D8670(instance, PM_ActionGetNode2(handle));
        int value = reinterpret_cast<int(__thiscall*)(CPed*)>(static_cast<void*>(self->vtable[0xAC / 4]))(self);
        PM_Instance5D85C0(instance, value);
        PM_Instance5D82A0(instance, 1, 0);
        PM_ContextsSetCurrent(PM_actionContexts, saved);
    }
    PM_ActionRelease(handle);
}

// vf42 0x479260: killed (once; +0x13FC only runs its death action): its killer (the
// intelligence's +0x40C) told and credited, health -100, blips gone, the player's death
// or the ambient ped's cleanup
void __fastcall CPed_Kill(CPed* self, re::Edx, int a, int b) {
    if (self->unknown1C98 != 0) {
        return;
    }
    if (At<int32_t>(self, 0x13FC) != 0) {
        if (At<int32_t>(self, 0x141C) == 0) {
            return;
        }
        if (!PM_ActionIsValid(&At<ped::ActionHandle>(self, 0x18E8))) {
            return;
        }
        RunAction18E8(self);
        return;
    }
    CPed* killer = At<CPed*>(self->intelligence, 0x40C);
    if (killer) {
        PM_PopulationFn49EE70(PM_population, self);
        At<uint32_t>(At<uint8_t*>(self->intelligence, 0x40C), 0x13F0) = static_cast<uint8_t>(a);
        PM_Fn4A67D0(PM_C2C1A0, self);
        PM_Fn4A6670(PM_C2C1A0, 7, self, At<void*>(self->intelligence, 0x40C), 0, PM_f_minusOne, PM_f_minusOne);
        if (At<CPed*>(self->intelligence, 0x40C) == PM_player) {
            int stat = -1;
            switch (self->pedType) {
            case 1: stat = 0x54; break;
            case 2: stat = 0x5C; break;
            case 3: stat = 0x5E; break;
            case 4: stat = 0x5A; break;
            case 5: stat = 0x58; break;
            case 11: stat = 0x56; break;
            default: break;
            }
            if (stat != -1) {
                PM_IncrementStat(stat, 1);
            }
            if (self->modelIndex == PM_modelRat) {
                PM_IncrementStat(0x60, 1);
                PM_Fn5930A0(self, 1);
            }
            if (self->modelIndex != PM_modelRat && !PM_Ped4473E0(self)) {
                PM_IncrementStat(0xC6, 1);
            }
        }
    }
    self->health = PM_deadHealth;
    self->unknown1C98 = 1;
    PM_PedRunAction18E8(self, 1);
    PM_RadarClearBlips(PM_radar, 2, PedHandle(self));
    PM_RadarClearBlips(PM_radar, 0xD, PedHandle(self));
    PM_RadarClearBlips(PM_radar, 6, PedHandle(self));
    At<int32_t>(self, 0x135C) = 0;
    At<float>(self, 0x1708) = 0.0f;
    At<float>(self, 0x170C) = 0.0f;
    if (self->pedType == PED_TYPE_PLAYER) {
        PM_Fn485A40(Bytes(self) + 0x2F0);
        PM_Fn431330(PM_BD1078, a, static_cast<uint8_t>(a) == 0 ? 1 : 0, b);
        PM_WeaponFn4D9410(self->weapon);
    } else {
        At<uint32_t>(self, 0x18B8) = PM_timeInMs;
        PM_Fn444340(self);
        PM_SetTargetEntity(self, nullptr);
    }
    re::Ptr32<void>& listener = At<void*>(self, 0x1B8);
    if (listener) {
        reinterpret_cast<void(__thiscall*)(void*, CPed*)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(listener))[1]))(listener, self);
        listener = nullptr;
    }
}

// 0x479590 (thiscall): +0x1D24 / +0x1D28 dropped (the model released) when no longer in
// their pool, unless of kind 3
void __fastcall CPed_Validate1D24(CPed* self, re::Edx) {
    CPool<uint8_t>* pool = PM_1D24Pool;
    static const uint32_t offsets[] = {0x1D24, 0x1D28};
    for (uint32_t offset : offsets) {
        re::Ptr32<uint8_t>& p = At<uint8_t*>(self, offset);
        if (!p) {
            continue;
        }
        int index = static_cast<int>((p - pool->objects) / pool->elemSize);
        bool valid = index >= 0 && index < pool->size && p == pool->objects + pool->elemSize * index;
        if (!valid && At<int32_t>(p, 0x70) != 3) {
            PM_Fn52FE40(self->modelIndex);
            p = nullptr;
        }
    }
}

// 0x479650 (thiscall): the ground matrix (+0x1674: right, forward, up, position; each
// with a fourth float) from a sphere cast down in front of its feet, and a second one
// toward where it hit for the forward
void __fastcall CPed_ComputeGroundMatrix(CPed* self, re::Edx, int a, int b, int c, int d) {
    // (the w floats written to +0x1690, +0x16A0 and +0x16B0 are uninitialised stack in the
    // game; ours are 0)
    alignas(4) uint8_t colPoint[0x28] = {};
    void* hitEntity = nullptr;  // (written by the casts, never released)
    ped::CVector pos = *PosOf(self);
    float s = static_cast<float>(std::sin(static_cast<double>(self->desiredHeading)));
    ped::CVector dir;
    dir.x = static_cast<float>(-s * PM_d_0_45);
    float co = static_cast<float>(std::cos(static_cast<double>(self->desiredHeading)));
    dir.y = static_cast<float>(co * PM_d_0_45);
    dir.z = 0.0f;
    ped::CVector top;
    top.x = dir.x + pos.x;
    top.y = dir.y + pos.y;
    top.z = static_cast<float>(pos.z + PM_d_0);
    ped::CVector bottom = {top.x, top.y, top.z};
    top.z = static_cast<float>(bottom.z + PM_d_2_5 + PM_d_0_25);
    bottom.z = static_cast<float>(bottom.z - PM_d_0_5);
    At<float>(colPoint, 0x24) = PM_fltMax;
    int flags = re::Flag(a) ? 0xC : 0xE;
    if (!re::Flag(b)) {
        flags |= 1;
    }
    PM_C1AE80 = static_cast<uint8_t>(c);
    PM_C1AE81 = static_cast<uint8_t>(d);
    ped::CVector& up = At<ped::CVector>(self, 0x1694);
    ped::CVector& at = At<ped::CVector>(self, 0x16A4);
    if (PM_ProcessSphereCast(&top, &bottom, PM_castRadius, colPoint, &hitEntity, 9, flags, 0)) {
        ped::CVector hit = At<ped::CVector>(colPoint, 0);
        at = hit;
        At<float>(self, 0x16B0) = 0.0f;
        up = At<ped::CVector>(colPoint, 0x10);
        At<float>(self, 0x16A0) = 0.0f;
        ped::CVector from = {pos.x, pos.y, hit.z};
        if (PM_ProcessSphereCast(&from, &hit, PM_castRadius, colPoint, &hitEntity, 9, flags, 1)) {
            const ped::CVector& n = At<ped::CVector>(colPoint, 0x10);
            dir.x = -n.x;
            dir.y = -n.y;
            dir.z = -n.z;
        }
    } else {
        at = bottom;
        At<float>(self, 0x16B0) = 0.0f;
        up.x = 0.0f;
        up.y = 0.0f;
        up.z = 1.0f;
        At<float>(self, 0x16A0) = 0.0f;
    }
    PM_C1AE80 = 0;
    PM_C1AE81 = 0;
    PM_VectorNormalise4294A0(&dir);
    ped::CVector right;
    right.x = dir.y * up.z - dir.z * up.y;
    right.y = dir.z * up.x - dir.x * up.z;
    right.z = dir.x * up.y - dir.y * up.x;
    PM_VectorNormalise4294A0(&right);
    ped::CVector& r = At<ped::CVector>(self, 0x1674);
    r = right;
    ped::CVector forward;
    forward.x = r.z * up.y - r.y * up.z;
    forward.y = r.x * up.z - r.z * up.x;
    forward.z = up.x * r.y - up.y * r.x;
    PM_VectorNormalise4294A0(&forward);
    At<ped::CVector>(self, 0x1684) = forward;
    At<float>(self, 0x1690) = 0.0f;
    re::Ptr32<void>& colEntity = At<void*>(colPoint, 0xC);
    if (colEntity) {
        PM_CleanUpOldReference(colEntity, &colEntity);
    }
}

// 0x479AC0 (thiscall): the ground matrix, worked out once
void* __fastcall CPed_GetGroundMatrix(CPed* self, re::Edx, int a, int b, int c, int d) {
    if (!At<uint8_t>(self, 0x16B8)) {
        PM_PedGroundMatrix(self, a, b, c, d);
        At<uint8_t>(self, 0x16B8) = 1;
    }
    return Bytes(self) + 0x1674;
}

// 0x479B00 (thiscall): the ground matrix copied into +0x1630
void __fastcall CPed_UseGroundMatrix(CPed* self, re::Edx, int a, int b, int c, int d) {
    if (!At<uint8_t>(self, 0x16B8)) {
        PM_PedGroundMatrix(self, a, b, c, d);
        At<uint8_t>(self, 0x16B8) = 1;
    }
    PM_MatrixAssign(Bytes(self) + 0x1630, Bytes(self) + 0x1674);
}

#define PEDMODEL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDMODEL_REVERSED("PedMember4785D0::Destruct", 0x004785D0, &PedMember4785D0_Destruct);
PEDMODEL_REVERSED("PedMember478890::Destruct", 0x004786D0, &PedMember478890_Destruct);
PEDMODEL_REVERSED("EntityPtr::Assign", 0x00478760, &EntityPtr_Assign);
PEDMODEL_REVERSED("CVector::Normalised_478820", 0x00478820, &Vector_Normalised);
PEDMODEL_REVERSED("PedMember::Set18C", 0x00478850, &PedMember_Set18C);
PEDMODEL_REVERSED("PedMember478890::Construct", 0x00478890, &PedMember478890_Construct);
PEDMODEL_REVERSED("CPed::ClearBlips", 0x00478960, &CPed_ClearBlips);
PEDMODEL_REVERSED("CPed::SetModelIndex", 0x00478A20, &CPed_SetModelIndex);
PEDMODEL_REVERSED("CPed::SetPedStats", 0x00478F80, &CPed_SetPedStats);
PEDMODEL_REVERSED("CPed::Kill", 0x00479260, &CPed_Kill);
PEDMODEL_REVERSED("CPed::Validate1D24", 0x00479590, &CPed_Validate1D24);
PEDMODEL_REVERSED("CPed::ComputeGroundMatrix", 0x00479650, &CPed_ComputeGroundMatrix);
PEDMODEL_REVERSED("CPed::GetGroundMatrix", 0x00479AC0, &CPed_GetGroundMatrix);
PEDMODEL_REVERSED("CPed::UseGroundMatrix", 0x00479B00, &CPed_UseGroundMatrix);

} // namespace
