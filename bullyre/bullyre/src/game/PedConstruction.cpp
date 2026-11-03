// CPed's constructor (0x480130) and destructor body (0x480CC0): its member objects, the
// parts it owns (inventory, intelligence, targeting, weapon, ...), and their teardown.

#include <initializer_list>

#include "game/Ped.h"
#include "game/Pool.h"
#include "reversed.h"

namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;

constexpr int PED_TYPE_PLAYER = 0xD;
constexpr uint32_t PED_VTABLE = 0x0090D0D4;

GAME_VAR(CPed*, PK2_player, 0x00C1AEA8);
inline void* const PK2_playerInfo = reinterpret_cast<void*>(0x00C1AEA8);
GAME_VAR(CPed*, PK2_constructing, 0x00C2A8E8);        // the ped being made or destroyed
GAME_VAR(uint32_t, PK2_timeInMs, 0x00C1A9B4);
GAME_VAR(CPool<CPed>*, PK2_pedPool, 0x00C0F5F0);
GAME_VAR(void*, PK2_radar, 0x00C674CC);
GAME_VAR(uint8_t*, PK2_population, 0x00C2C108);
GAME_VAR(void*, PK2_patrolPaths, 0x00C2B034);
GAME_ARRAY(uint8_t*, PK2_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, PK2_modelRat, 0x00BF42DC);
GAME_VAR(int32_t, PK2_modelBF3EC4, 0x00BF3EC4);
GAME_VAR(float, PK2_fltMax, 0x009009B8);
GAME_VAR(float, PK2_f_200, 0x00906948);
GAME_VAR(float, PK2_f_100, 0x00900490);
GAME_VAR(float, PK2_f_0_25, 0x0090CD88);
GAME_VAR(float, PK2_f_70, 0x0090D0C8);
GAME_VAR(float, PK2_f_0_0057, 0x0090D0C4);
GAME_VAR(float, PK2_f_0_05, 0x009068B8);
GAME_VAR(float, PK2_f_1000, 0x00900F40);
GAME_VAR(float, PK2_f_0_016, 0x0090D0C0);
GAME_ARRAY(uint32_t, PK2_A33970, 0x00A33970);          // (3 floats copied to +0x1DD8)
GAME_ARRAY(uint8_t, PK2_C0A100, 0x00C0A100);           // 0x48-byte records, +0x46 flags
inline void* const PK2_zeroVector = reinterpret_cast<void*>(0x00BCC564);
inline void* const PK2_C2C730 = reinterpret_cast<void*>(0x00C2C730);
inline void* const PK2_C3AEF8 = reinterpret_cast<void*>(0x00C3AEF8);
inline void* const PK2_20C7E98 = reinterpret_cast<void*>(0x020C7E98);

// constructor callees
GAME_FN(void, __thiscall, PK2_PhysicalConstruct, 0x00468E90, CPed* self);
GAME_FN(void, __thiscall, PK2_Fn6BA240, 0x006BA240, void* self);
GAME_FN(void, __thiscall, PK2_Fn4853F0, 0x004853F0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn48C300, 0x0048C300, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_QueueConstruct, 0x00470160, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn4A48F0, 0x004A48F0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Member478890, 0x00478890, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Member474F00, 0x00474F00, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Member474F50, 0x00474F50, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn4A2BD0, 0x004A2BD0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn5E32A0, 0x005E32A0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn488CB0, 0x00488CB0, void* self);
GAME_FN(void, __thiscall, PK2_Fn669FD0, 0x00669FD0, void* self);
GAME_FN(void, __thiscall, PK2_HandleSet, 0x005D7F10, void* handle, uintptr_t value);
GAME_FN(intptr_t, __thiscall, PK2_RaisePendingEvent, 0x004779E0, CPed* ped);
GAME_FN(void*, __cdecl, PK2_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, PK2_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __thiscall, PK2_InventoryConstruct, 0x0045C990, void* self);
GAME_FN(void*, __cdecl, PK2_New468B60, 0x00468B60, uintptr_t size);
GAME_FN(void*, __thiscall, PK2_Construct468950, 0x00468950, void* self);
GAME_FN(void*, __cdecl, PK2_New485190, 0x00485190, uintptr_t size);
GAME_FN(void*, __thiscall, PK2_IntelligenceConstruct, 0x00484C00, void* self);
GAME_FN(void*, __cdecl, PK2_New4D64E0, 0x004D64E0, uintptr_t size);
GAME_FN(void*, __thiscall, PK2_TargetingConstruct, 0x004D8370, void* self, CPed* ped);
GAME_FN(void*, __thiscall, PK2_WeaponConstruct, 0x004DA760, void* self);
GAME_FN(void*, __cdecl, PK2_New4DAD50, 0x004DAD50, uintptr_t size);
GAME_FN(void*, __thiscall, PK2_Construct4DA9A0, 0x004DA9A0, void* self);
GAME_FN(void*, __cdecl, PK2_New4A7550, 0x004A7550, uintptr_t size);
GAME_FN(void*, __thiscall, PK2_Construct6CC2D0, 0x006CC2D0, void* self);
GAME_FN(void*, __cdecl, PK2_New46F9F0, 0x0046F9F0, uintptr_t size);
GAME_FN(void*, __thiscall, PK2_AttitudeSetConstruct, 0x0046F270, void* self);
GAME_FN(void*, __cdecl, PK2_New7315B0, 0x007315B0, uintptr_t size);
GAME_FN(void*, __thiscall, PK2_Construct731400, 0x00731400, void* self);
GAME_FN(void*, __cdecl, PK2_New730830, 0x00730830, uintptr_t size);
GAME_FN(void*, __thiscall, PK2_Construct72F830, 0x0072F830, void* self, CPed* ped);
GAME_FN(void*, __cdecl, PK2_New730F70, 0x00730F70, uintptr_t size);
GAME_FN(void*, __thiscall, PK2_Construct72F8C0, 0x0072F8C0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn496BB0, 0x00496BB0, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn499C70, 0x00499C70, void* self);
GAME_FN(void, __thiscall, PK2_Fn485920, 0x00485920, void* self);
GAME_FN(void, __thiscall, PK2_Fn486EA0, 0x00486EA0, void* self);
GAME_FN(void, __thiscall, PK2_Fn48CEB0, 0x0048CEB0, void* self);
GAME_FN(void, __thiscall, PK2_Fn49A650, 0x0049A650, void* self);
GAME_FN(void, __cdecl, PK2_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __thiscall, PK2_Fn4DA9C0, 0x004DA9C0, void* self, void* weapon);
GAME_FN(void, __thiscall, PK2_Fn5772F0, 0x005772F0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_SetModelIndex, 0x00478A20, CPed* ped, int model, int a);
GAME_FN(void, __thiscall, PK2_Fn48E8E0, 0x0048E8E0, void* self, CPed* ped);
GAME_FN(int, __thiscall, PK2_RadarAddBlip, 0x0054A5B0, void* radar, int kind, int handle, uint32_t colour, int display,
        int a, int b);
GAME_FN(void, __thiscall, PK2_RadarSetBlipSprite, 0x00548BF0, void* radar, int blip, int sprite);
GAME_FN(void, __thiscall, PK2_PedReset, 0x004750A0, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn4A7E50, 0x004A7E50, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn4DB1C0, 0x004DB1C0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn56EB80, 0x0056EB80, void* self, float a, const void* v);
GAME_FN(void, __thiscall, PK2_PopulationAddModel, 0x004A0A90, void* population, int model, int ambient);
GAME_FN(void, __thiscall, PK2_Ped49BBD0, 0x0049BBD0, CPed* ped, int model, uint32_t* out);
GAME_FN(uint32_t, __thiscall, PK2_PlayerInfo4466F0, 0x004466F0, void* info, int a);
GAME_FN(void, __cdecl, PK2_Fn5287B0, 0x005287B0, CPed* ped);
// destructor callees
GAME_FN(void, __thiscall, PK2_ClearPartner, 0x00477470, CPed* ped);
GAME_FN(void, __thiscall, PK2_Intel4840C0, 0x004840C0, void* intelligence);
GAME_FN(void, __thiscall, PK2_Fn485A50, 0x00485A50, void* self);
GAME_FN(void, __thiscall, PK2_Fn485BC0, 0x00485BC0, void* self);
GAME_FN(void, __thiscall, PK2_Fn4A7370, 0x004A7370, void* self);
GAME_FN(void, __thiscall, PK2_Fn4DA9E0, 0x004DA9E0, void* self);
GAME_FN(void, __thiscall, PK2_Fn68EBB0, 0x0068EBB0, CPed* ped, int a);
GAME_FN(void, __thiscall, PK2_PatrolForgetPed, 0x00472350, void* paths, CPed* ped);
GAME_FN(void, __thiscall, PK2_RadarClearBlips, 0x0054A890, void* radar, int kind, int handle);
GAME_FN(int16_t, __thiscall, PK2_ModelInfoFn5228D0, 0x005228D0, void* modelInfo);
GAME_FN(void, __thiscall, PK2_PopulationRemoveModel, 0x004A0AB0, void* population, int model, int ambient);
GAME_FN(void, __thiscall, PK2_Fn487B70, 0x00487B70, void* self);
GAME_FN(void, __thiscall, PK2_Fn486D50, 0x00486D50, void* self);
GAME_FN(void, __thiscall, PK2_Fn4878A0, 0x004878A0, void* self);
GAME_FN(void, __thiscall, PK2_SetTargetEntity, 0x00498B10, CPed* ped, CEntity* target);
GAME_FN(void, __cdecl, PK2_WorldRemove, 0x0045DD70, CPed* ped);
GAME_FN(void, __thiscall, PK2_SeatsFn4CE960, 0x004CE960, void* seats);
GAME_FN(void, __cdecl, PK2_Fn52FE40, 0x0052FE40, int model);
GAME_FN(void, __thiscall, PK2_Fn447560, 0x00447560, void* self, CPed* ped, int a);
GAME_FN(void, __thiscall, PK2_Fn4475D0, 0x004475D0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_GroupRemoveMember, 0x00489770, void* group, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn6D75A0, 0x006D75A0, void* self, CPed* ped);
GAME_FN(void, __thiscall, PK2_ClearBlends, 0x00473AC0, CPed* ped);
GAME_FN(void, __thiscall, PK2_WeaponFn4D9410, 0x004D9410, void* weapon);
GAME_FN(void, __thiscall, PK2_EntityFn4667A0, 0x004667A0, CPed* ped);
GAME_FN(int, __cdecl, PK2_Fn4D3D70, 0x004D3D70, CPed* ped);
GAME_FN(void, __cdecl, PK2_Fn4D3F10, 0x004D3F10, int a, int b);
GAME_FN(void, __thiscall, PK2_PropFn4EE060, 0x004EE060, void* prop, CPed* ped);
GAME_FN(void, __thiscall, PK2_Fn72FE60, 0x0072FE60, void* self);
GAME_FN(void, __cdecl, PK2_Delete731C00, 0x00731C00, void* p);
GAME_FN(void, __cdecl, PK2_Delete46FA60, 0x0046FA60, void* p);
GAME_FN(void, __cdecl, PK2_Delete4A7580, 0x004A7580, void* p);
GAME_FN(void, __cdecl, PK2_Delete4DAD60, 0x004DAD60, void* p);
GAME_FN(void, __thiscall, PK2_IntelligenceDestruct, 0x004844F0, void* self);
GAME_FN(void, __cdecl, PK2_Delete4852B0, 0x004852B0, void* p);
GAME_FN(void, __thiscall, PK2_Destruct468980, 0x00468980, void* self);
GAME_FN(void, __cdecl, PK2_Delete468B90, 0x00468B90, void* p);
GAME_FN(int, __cdecl, PK2_Fn4438B0, 0x004438B0, int id);
GAME_FN(void, __thiscall, PK2_ActionRelease, 0x005D9C50, void* handle);
GAME_FN(void, __thiscall, PK2_Refs474E00_Destruct, 0x00474E00, void* self);
GAME_FN(void, __thiscall, PK2_Fn48E7F0, 0x0048E7F0, void* self);
GAME_FN(void, __thiscall, PK2_Member478890_Destruct, 0x004786D0, void* self);
GAME_FN(void, __thiscall, PK2_Fn4A4740, 0x004A4740, void* self);
GAME_FN(void, __thiscall, PK2_QueueDestruct, 0x00471350, void* self);
GAME_FN(void, __thiscall, PK2_Member4785D0_Destruct, 0x004785D0, void* self);
GAME_FN(void, __thiscall, PK2_Fn6B5A80, 0x006B5A80, void* self);
GAME_FN(void, __thiscall, PK2_PhysicalDestruct, 0x004690B0, CPed* self);

void ZeroDwords(CPed* self, const uint16_t* offsets, size_t count) {
    for (size_t i = 0; i < count; i++) {
        At<uint32_t>(self, offsets[i]) = 0;
    }
}

// A registered reference let go (its field not cleared)
void DropRef(void* self, uint32_t offset) {
    re::Ptr32<void>& ref = At<void*>(self, offset);
    if (ref) {
        PK2_CleanUpOldReference(ref, &ref);
    }
}

// A deleting destructor through the vtable (vtable[0], flags 1)
void DeleteVirtual(void* object) {
    reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[0]))(object, 1);
}

// 0x480130 (thiscall): CPed::CPed(model, seat state)
CPed* __fastcall CPed_Construct(CPed* self, re::Edx, int model, int seatState) {
    PK2_PhysicalConstruct(self);
    At<uint32_t>(self, 0) = PED_VTABLE;
    PK2_Fn6BA240(Bytes(self) + 0x1DC);
    PK2_Fn4853F0(Bytes(self) + 0x2F0, self);
    At<CPed*>(self, 0x350) = self;
    At<uint8_t>(self, 0x354) = 1;
    At<uint8_t>(self, 0x34C) = 0;
    static const uint16_t zeroA[] = {0x368, 0x36C, 0x370, 0x374, 0x378, 0x37C, 0x380, 0x394,
                                     0x398, 0x39C, 0x3A0, 0x3A4, 0x3A8, 0x3AC};
    ZeroDwords(self, zeroA, sizeof(zeroA) / sizeof(zeroA[0]));
    At<CPed*>(self, 0x358) = self;
    PK2_Fn48C300(self->unknown3BC, self);
    PK2_QueueConstruct(&self->objectives, self);
    PK2_Fn4A48F0(Bytes(self) + 0x654, self);
    PK2_Member478890(self->inventory, self);
    PK2_Member474F00(self->unknownA64, self);
    PK2_Member474F50(Bytes(self) + 0xB44, self);
    At<CPed*>(self, 0xD6C) = self;
    PK2_Fn4A2BD0(Bytes(self) + 0xD70, self);
    PK2_Fn5E32A0(Bytes(self) + 0xDAC, self);
    PK2_Fn488CB0(Bytes(self) + 0x1314);
    static const uint16_t zeroB[] = {0x1560, 0x1564, 0x1568, 0x156C, 0x15E8, 0x15EC, 0x15F0, 0x15FC,
                                     0x1600, 0x1604, 0x160C, 0x1610, 0x1614, 0x161C};
    ZeroDwords(self, zeroB, sizeof(zeroB) / sizeof(zeroB[0]));
    self->seatState = static_cast<uint8_t>(seatState);
    for (uint32_t offset = 0x1588; offset < 0x15E8; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    PK2_Fn669FD0(Bytes(self) + 0x1628);
    At<int32_t>(self, 0x162C) = -1;
    static const uint16_t zeroC[] = {0x1670, 0x16B4, 0x16BC, 0x16C0, 0x16C4, 0x16C8, 0x16D8, 0x16E0, 0x16E4,
                                     0x16E8, 0x16EC, 0x1700, 0x1708, 0x170C, 0x1710, 0x1714, 0x1718, 0x171C,
                                     0x1720, 0x1724, 0x1728, 0x173C, 0x1740, 0x1744, 0x188C, 0x1890, 0x1894,
                                     0x1898, 0x189C, 0x18A0, 0x18A8, 0x18C0, 0x18C4, 0x18CC, 0x18D8, 0x18E0,
                                     0x18E4, 0x18E8, 0x18EC, 0x18F8, 0x18FC, 0x1D1C, 0x1D20, 0x1D24, 0x1D34,
                                     0x1D38, 0x1D3C, 0x1DBC, 0x1DC8, 0x1DCC, 0x1DD0, 0x1DD4};
    ZeroDwords(self, zeroC, sizeof(zeroC) / sizeof(zeroC[0]));
    At<uint8_t>(self, 0x16D4) = 0;
    // eight col points (0x174C, 0x28 apart): position, entity, normal 0; depth FLT_MAX
    for (int i = 0; i < 8; i++) {
        uint8_t* cp = Bytes(self) + 0x174C + i * 0x28;
        for (uint32_t offset : {0x0u, 0x4u, 0x8u, 0xCu, 0x10u, 0x14u, 0x18u}) {
            At<uint32_t>(cp, offset) = 0;
        }
        At<float>(cp, 0x24) = PK2_fltMax;
    }
    At<int32_t>(self, 0x18AC) = -1;
    self->unknown18C8 = 4;
    self->unknown18D0 = PK2_f_200;
    self->unknown18D4 = 1.0f;
    self->health = PK2_f_100;
    self->unknown1CBC = PK2_f_100;
    self->unknown1C98 = 0;
    At<uint8_t>(self, 0x1C99) = 1;
    At<uint8_t>(self, 0x1C9B) = 0;
    PK2_HandleSet(Bytes(self) + 0x1DD4, 0);
    static const uint16_t zeroD[] = {0x1DD8, 0x1DDC, 0x1DE0, 0x1E08, 0x1E18, 0x1E1C, 0x1E20, 0x1E28, 0x1E34,
                                     0x1E3C, 0x1E40, 0x1E44, 0x1E48, 0x1E4C, 0x1E50, 0x1E54, 0x1E58, 0x1E5C,
                                     0x1E60, 0x1E64, 0x1E68, 0x1E6C, 0x1E78, 0x1E84, 0x1E88, 0x1E30};
    ZeroDwords(self, zeroD, sizeof(zeroD) / sizeof(zeroD[0]));
    self->alpha = 0xFF;
    self->flags1E24 = 3;
    self->unknown1E26 = 1;
    self->unknown1E27 = 0;
    self->historyIndex = -1;
    At<uint8_t>(self, 0x1E8C) = 0;
    PK2_constructing = self;
    At<int32_t>(self, 0x1390) = 1;
    At<int32_t>(self, 0x18DC) = 1;
    PK2_RaisePendingEvent(self);
    // the parts it owns
    void* p = PK2_Malloc(0x14);
    At<void*>(self, 0x1C4) = p ? PK2_InventoryConstruct(p) : nullptr;
    p = PK2_New468B60(0x74);
    At<void*>(self, 0x2E4) = p ? PK2_Construct468950(p) : nullptr;
    p = PK2_New485190(0x5C0);
    self->intelligence = static_cast<uint8_t*>(p ? PK2_IntelligenceConstruct(p) : nullptr);
    p = PK2_New4D64E0(0xE0);
    self->targeting = p ? PK2_TargetingConstruct(p, self) : nullptr;
    p = PK2_Malloc(0x298);
    self->weapon = static_cast<CEntity*>(p ? PK2_WeaponConstruct(p) : nullptr);
    p = PK2_New4DAD50(0x20);
    At<void*>(self, 0x1D4) = p ? PK2_Construct4DA9A0(p) : nullptr;
    p = PK2_New4A7550(8);
    At<void*>(self, 0x1D8) = p ? PK2_Construct6CC2D0(p) : nullptr;
    p = PK2_New46F9F0(0x38);
    self->attitudeSet = p ? PK2_AttitudeSetConstruct(p) : nullptr;
    p = PK2_New7315B0(0x200);
    self->unknown2EC = p ? PK2_Construct731400(p) : nullptr;
    // three refcounted limb objects (+4 the count; read through when the allocation fails)
    p = PK2_New730830(0x40);
    void* limb = p ? PK2_Construct72F830(p, self) : nullptr;
    At<void*>(self, 0x16F0) = limb;
    At<int32_t>(limb, 4) += 1;
    p = PK2_New730F70(0x50);
    limb = p ? PK2_Construct72F8C0(p, self) : nullptr;
    At<void*>(self, 0x16F4) = limb;
    At<int32_t>(limb, 4) += 1;
    p = PK2_New730F70(0x50);
    limb = p ? PK2_Construct72F8C0(p, self) : nullptr;
    At<void*>(self, 0x16F8) = limb;
    At<int32_t>(limb, 4) += 1;
    void* first = At<void*>(self, 0x16F0);
    reinterpret_cast<void(__thiscall*)(void*, float)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(first))[0x38 / 4]))(first, PK2_f_0_25);
    At<CPed*>(At<void*>(self, 0x1C4), 4) = self;
    At<uint32_t>(self, 0x1DD8) = PK2_A33970[0];
    At<uint32_t>(self, 0x1DDC) = PK2_A33970[1];
    At<uint32_t>(self, 0x1DE0) = PK2_A33970[2];
    At<int32_t>(self, 0x1DB0) = 0;
    At<uint8_t>(self, 0x1E25) = 0;
    self->unknown1580 = 0;
    At<int32_t>(self, 0x1584) = 0;
    for (uint32_t offset = 0x1DE8; offset <= 0x1E00; offset += 8) {
        At<int32_t>(self, offset) = -1;
        At<float>(self, offset + 4) = 0.0f;
    }
    PK2_Fn496BB0(self);
    PK2_Fn499C70(Bytes(self) + 0x358);
    self->unknown1E0C = 0.0f;
    PK2_Fn485920(Bytes(self) + 0x2F0);
    PK2_Fn486EA0(self->inventory);
    PK2_Fn48CEB0(self->unknownA64);
    PK2_Fn49A650(Bytes(self) + 0xD6C);
    At<uint8_t>(self, 0x1DE6) = 0;
    re::Ptr32<void>& ref1E08 = At<void*>(self, 0x1E08);
    if (ref1E08) {
        PK2_CleanUpOldReference(ref1E08, &ref1E08);
        ref1E08 = nullptr;
    }
    At<int32_t>(self, 0x1574) = -1;
    At<int32_t>(self, 0x1578) = 0;
    self->pedType = At<int32_t>(PK2_modelInfoPtrs[model], 0x4C);
    self->typeFlags = static_cast<uint8_t>((self->typeFlags & 0xFB) | 3);
    self->animBlend = Bytes(self) + 0x1DC;
    At<uint32_t>(self->animBlend, 0x98) |= 0x10;
    At<float>(self, 0x158) = PK2_f_70;
    At<float>(self, 0x15C) = PK2_f_100;
    At<float>(self, 0x160) = PK2_f_0_0057;
    At<int32_t>(self, 0x1C9C) = 0;
    At<float>(self, 0x164) = PK2_f_0_05;
    At<float>(self, 0x1704) = 0.0f;
    PK2_Fn4DA9C0(At<void*>(self, 0x1D4), self->weapon);
    PK2_Fn5772F0(At<void*>(self, 0x1D4), self);
    At<float>(self, 0x16D0) = 0.0f;
    At<int32_t>(self, 0x1CC) = 0;
    self->vehicle = nullptr;
    self->inVehicleSeat = 0;
    self->partner = nullptr;
    self->unknown1559 = 0;
    At<int32_t>(self, 0x1620) = 0;
    At<uint8_t>(self, 0x16CC) = 0;
    PK2_SetModelIndex(self, model, 1);
    PK2_Fn48E8E0(Bytes(self) + 0xB44, self);
    if (self->modelIndex == PK2_modelRat) {
        At<int32_t>(self, 0xA0) = 1;
        At<uint8_t>(self, 0x1E25) = 1;
    }
    int32_t type = self->pedType;
    if (type == 0 || type == 7 || type == 8) {
        int slot = PK2_pedPool->GetIndex(self);
        int handle = PK2_pedPool->flags[slot] + (slot << 8);
        int blip = PK2_RadarAddBlip(PK2_radar, 6, handle, 0, 3, 2, 1);
        PK2_RadarSetBlipSprite(PK2_radar, blip, 5);
    }
    At<int32_t>(self, 0x1D2C) = PK2_pedPool->GetIndex(self);
    PK2_PedReset(self);
    At<CPed*>(At<void*>(self, 0x2E4), 0) = self;
    PK2_Fn4A7E50(PK2_C2C730, self);
    PK2_Fn4DB1C0(PK2_C3AEF8, self);
    self->unknown1624 = 0;
    for (uint32_t offset = 0x1588; offset < 0x15E8; offset += 4) {
        re::Ptr32<void>& ref = At<void*>(self, offset);
        if (ref) {
            PK2_CleanUpOldReference(ref, &ref);
            ref = nullptr;
        }
    }
    At<int32_t>(self, 0x15F4) = 0;
    At<float>(self, 0x15E8) = PK2_f_1000;
    At<float>(self, 0x15EC) = PK2_f_1000;
    At<float>(self, 0x15F0) = PK2_f_1000;
    At<uint8_t>(self, 0x15F8) = 1;
    for (int i = 0; i < 2; i++) {
        PK2_Fn56EB80(Bytes(self) + 0x15FC + i * 0x10, 0.0f, PK2_zeroVector);
    }
    At<uint8_t>(self, 0x130C) = 0;
    At<float>(self, 0x1D58) = PK2_f_0_016;
    At<uint8_t>(self, 0x130D) = 0;
    At<int32_t>(self, 0x1D94) = 0;
    At<int32_t>(self, 0x1D98) = 0;
    At<int32_t>(self, 0x1D9C) = 0;
    At<int32_t>(self, 0x1DA0) = 0;
    At<float>(self, 0x1CC0) = 0.0f;
    At<uint8_t>(self, 0x130E) = 0;
    // (the game pushes the flag with garbage above its low byte)
    int ambient = (self->seatState == 1 || self->seatState == 4) ? 1 : 0;
    PK2_PopulationAddModel(PK2_population, model, ambient);
    uint32_t bits = 0;
    PK2_Ped49BBD0(PK2_player, self->modelIndex, &bits);
    if (PK2_PlayerInfo4466F0(PK2_playerInfo, 0x1B) != 0 && (bits & PK2_PlayerInfo4466F0(PK2_playerInfo, 0x1B)) != 0) {
        void* set = self->attitudeSet;
        if (set) {
            reinterpret_cast<void(__thiscall*)(void*, int, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(set))[1]))(
                set, PED_TYPE_PLAYER, 4);
        }
    }
    At<float>(self, 0x1CC4) = 0.0f;
    At<int32_t>(self, 0x1CC8) = 0;
    for (uint32_t offset : {0x1CCCu, 0x1CF4u, 0x1CD0u, 0x1CF8u, 0x1CD4u, 0x1CFCu, 0x1CD8u, 0x1D00u}) {
        At<float>(self, offset) = 1.0f;
    }
    PK2_Fn5287B0(self);
    PK2_constructing = nullptr;
    return self;
}

// 0x480CC0 (thiscall): the destructor's body (then CPhysical's)
void __fastcall CPed_Destruct(CPed* self, re::Edx) {
    At<uint32_t>(self, 0) = PED_VTABLE;
    CPed* partner = self->partner;
    if (partner && partner->partner == self) {
        PK2_ClearPartner(partner);
    }
    PK2_ClearPartner(self);
    PK2_constructing = self;
    PK2_Intel4840C0(self->intelligence);
    PK2_Fn49A650(Bytes(self) + 0x654);
    PK2_Fn485A50(Bytes(self) + 0x2F0);
    PK2_Fn485BC0(Bytes(self) + 0x2F0);
    PK2_Fn4A7370(At<void*>(self, 0x1D8));
    PK2_Fn4DA9E0(At<void*>(self, 0x1D4));
    PK2_Fn68EBB0(self, 0);
    PK2_PatrolForgetPed(PK2_patrolPaths, self);
    for (int kind : {2, 0xD, 6}) {
        int slot = PK2_pedPool->GetIndex(self);
        PK2_RadarClearBlips(PK2_radar, kind, PK2_pedPool->flags[slot] + (slot << 8));
    }
    // an ambient ped: when its model may come back (the model info's +0x60)
    CPed* other = self->unknown4F0[self->unknown504];
    if (self->seatState == 1 && !(PK2_population[0x7898] & 1)) {
        uint8_t* mi = PK2_modelInfoPtrs[self->modelIndex];
        bool keep = mi[0x50] != 0 && PK2_ModelInfoFn5228D0(mi) == 1 &&
                    (self->modelIndex == PK2_modelBF3EC4 || (At<int32_t>(mi, 0x4C) > 0 && At<int32_t>(mi, 0x4C) < 6));
        if (keep || (other && other->pedType == PED_TYPE_PLAYER)) {
            At<uint32_t>(mi, 0x60) = PK2_timeInMs;
        } else {
            At<uint32_t>(mi, 0x60) = 0;
        }
    }
    int ambient = (self->seatState == 1 || self->seatState == 4) ? 1 : 0;
    PK2_PopulationRemoveModel(PK2_population, self->modelIndex, ambient);
    PK2_Fn487B70(self->inventory);
    PK2_Fn486D50(self->inventory);
    PK2_Fn4878A0(self->inventory);
    PK2_SetTargetEntity(self, nullptr);
    PK2_WorldRemove(self);
    uint8_t* vehicle = re::PtrCast<uint8_t*>(self->vehicle);
    if (vehicle) {
        PK2_SeatsFn4CE960(vehicle + 0x1C4);
        DeleteVirtual(vehicle);
        self->vehicle = nullptr;
    }
    if (self->unknown1D24) {
        PK2_Fn52FE40(self->modelIndex);
        PK2_Fn447560(self->unknown1D24, self, 0);
    }
    if (self->unknown1D24) {
        uint8_t* old = self->unknown1D24;
        self->unknown1D24 = nullptr;
        PK2_Fn4475D0(old, self);
    }
    if (self->unknown1D28) {
        uint8_t* old = self->unknown1D28;
        self->unknown1D28 = nullptr;
        PK2_Fn4475D0(old, self);
    }
    if (self->group) {
        PK2_GroupRemoveMember(self->group, self);
    }
    PK2_Fn6D75A0(PK2_20C7E98, self);
    for (uint32_t offset : {0x1D94u, 0x1D98u, 0x1D9Cu, 0x1DA0u}) {
        re::Ptr32<void>& blur = At<void*>(self, offset);
        if (blur) {
            PK2_Fn49A650(blur);
            PK2_Free(blur);
            blur = nullptr;
        }
    }
    PK2_ClearBlends(self);
    PK2_WeaponFn4D9410(self->weapon);
    PK2_EntityFn4667A0(self);
    if (At<int32_t>(self, 0x1394) != 0) {
        for (int r = PK2_Fn4D3D70(self); r != -1; r = PK2_Fn4D3D70(self)) {
            PK2_Fn4D3F10(r, 0);
        }
    }
    uint8_t* prop = At<uint8_t*>(self, 0x548);
    if (prop) {
        PK2_PropFn4EE060(prop, self);
        At<int32_t>(prop, 0x398) = 0;
        PK2_HandleSet(prop + 0x39C, 0);
        prop[0x3A0] = 1;
        re::Ptr32<void>& ref = At<void*>(self, 0x548);
        if (ref) {
            PK2_CleanUpOldReference(ref, &ref);
            ref = nullptr;
        }
    }
    self->animBlend = nullptr;
    for (uint32_t offset : {0x16F0u, 0x16F4u, 0x16F8u}) {
        void* limb = At<void*>(self, offset);
        At<int32_t>(limb, 4) += -1;
        if (At<int32_t>(limb, 4) <= 0) {
            reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(limb))[1]))(limb, 1);
        }
    }
    if (void* p = self->unknown2EC) {
        PK2_Fn72FE60(p);
        PK2_Delete731C00(p);
    }
    PK2_Delete46FA60(self->attitudeSet);
    if (void* p = At<void*>(self, 0x1D8)) {
        PK2_Fn49A650(p);
        PK2_Delete4A7580(p);
    }
    if (void* p = At<void*>(self, 0x1D4)) {
        PK2_Fn49A650(p);
        PK2_Delete4DAD60(p);
    }
    if (self->weapon) {
        DeleteVirtual(self->weapon);
    }
    if (self->targeting) {
        DeleteVirtual(self->targeting);
    }
    if (void* p = self->intelligence) {
        PK2_IntelligenceDestruct(p);
        PK2_Delete4852B0(p);
    }
    if (void* p = At<void*>(self, 0x2E4)) {
        PK2_Destruct468980(p);
        PK2_Delete468B90(p);
    }
    if (At<void*>(self, 0x1C4)) {
        DeleteVirtual(At<void*>(self, 0x1C4));
    }
    At<void*>(self, 0x1C4) = nullptr;
    // (+0x1DBC let go here only when it's the ped itself)
    re::Ptr32<void>& ref1DBC = At<void*>(self, 0x1DBC);
    if (ref1DBC == self && ref1DBC) {
        PK2_CleanUpOldReference(ref1DBC, &ref1DBC);
        ref1DBC = nullptr;
    }
    PK2_constructing = nullptr;
    for (int i = 0; i < At<int32_t>(self, 0x1DB0); i++) {
        int id = At<int32_t>(self, 0x1DA4 + i * 4);
        if (id != -1) {
            int r = PK2_Fn4438B0(id);
            if (r != -1) {
                PK2_C0A100[r * 0x48 + 0x46] |= 4;
            }
        }
    }
    // the members' destructors
    DropRef(self, 0x1E08);
    PK2_HandleSet(Bytes(self) + 0x1DD4, 0);
    DropRef(self, 0x1DBC);
    PK2_ActionRelease(Bytes(self) + 0x18E8);
    PK2_ActionRelease(Bytes(self) + 0x18E4);
    PK2_ActionRelease(Bytes(self) + 0x18E0);
    for (int i = 7; i >= 0; i--) {
        DropRef(self, 0x1758 + i * 0x28);  // the col points' entities
    }
    DropRef(self, 0x16D8);
    PK2_HandleSet(Bytes(self) + 0x16C8, 0);
    PK2_Fn49A650(Bytes(self) + 0x1628);
    for (int i = 23; i >= 0; i--) {
        DropRef(self, 0x1588 + i * 4);
    }
    DropRef(self, 0x1308);
    DropRef(self, 0x1304);
    PK2_Refs474E00_Destruct(Bytes(self) + 0x11E4);
    DropRef(self, 0xD70);
    PK2_Fn48E7F0(Bytes(self) + 0xB44);
    PK2_Member478890_Destruct(self->inventory);
    PK2_Fn4A4740(Bytes(self) + 0x654);
    PK2_QueueDestruct(&self->objectives);
    PK2_Member4785D0_Destruct(self->unknown3BC);
    for (uint32_t offset : {0x3A0u, 0x374u, 0x324u, 0x2F8u, 0x2F4u}) {
        PK2_HandleSet(Bytes(self) + offset, 0);
    }
    PK2_Fn6B5A80(Bytes(self) + 0x1DC);
    PK2_PhysicalDestruct(self);
}

#define PEDCTOR_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDCTOR_REVERSED("CPed::CPed", 0x00480130, &CPed_Construct);
PEDCTOR_REVERSED("CPed::~CPed (body)", 0x00480CC0, &CPed_Destruct);

} // namespace
