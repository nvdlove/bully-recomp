// CPlayerPed (vtable 0x911BA4, 0x1F44 bytes like every ped) and the ped functions
// next to it: getting peds into and out of vehicles, the item checks, the held object
// placed in front of the camera, the yearbook photos (0x49A3A0 .. 0x49BFCF).
//
// Ped fields are read by offset here (CPed's layout in Ped.h doesn't have them all).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// The game's CMatrix (rows right, forward, up, pos; +0x40 the attached Gamebryo matrix)
struct CMatrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(CMatrix) == 0x44);

// A Gamebryo transform (a 3x3 rotation, the translation, the scale)
struct NiTransform {
    float rot[9];        // +0x0
    CVector trans;       // +0x24
    float scale;         // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

using re::At;

template <typename R, typename... A>
R Virtual(void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *static_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(object, args...);
}

// ---- the game's, called where they are ----
GAME_VAR(void*, FindPlayerPed_ptr, 0x00C1AEA8);        // (CPlayerInfo's first field: its thiscalls get 0xC1AEA8)
inline void* const playerInfo = reinterpret_cast<void*>(0x00C1AEA8);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
GAME_VAR(void*, cameraNode, 0x00C3CCF4);               // the camera's Gamebryo object
GAME_VAR(int32_t, gameMode, 0x00C3CCF8);               // 2: the photo camera
GAME_VAR(int32_t, debugActive, 0x00C3CF00);
GAME_VAR(uint32_t, timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(float, unknownC1A9A8, 0x00C1A9A8);
GAME_VAR(void*, controlledPed, 0x00C2B120);
GAME_VAR(CVector, zeroVector, 0x00BCC564);
GAME_VAR(int32_t, unequipDelay, 0x00A35054);
GAME_VAR(int32_t, unequipModel, 0x00BF432C);
GAME_VAR(uint8_t*, hudWeapons, 0x00C674F0);
GAME_VAR(int32_t, firstWeaponModel, 0x00A136B0);
GAME_VAR(int32_t, lastWeaponModel, 0x00A136B4);
GAME_VAR(int32_t, keepModel1, 0x00BF4430);
GAME_VAR(int32_t, keepModel2, 0x00BF433C);
GAME_VAR(int32_t, keepModel3, 0x00BF444C);
GAME_VAR(int32_t, keepModel4, 0x00BF44D8);
GAME_VAR(int32_t, keepModel5, 0x00BF4330);
GAME_VAR(int32_t, itemModel11, 0x00BF43E0);
GAME_VAR(int32_t, itemModel13, 0x00BF434C);
GAME_VAR(int32_t, itemModel14, 0x00BF4340);
GAME_VAR(int32_t, itemModel15, 0x00BF4350);
GAME_VAR(uint8_t, lockButtonInputs, 0x00C1AF8C);
GAME_VAR(uint8_t, allowStickX, 0x00C1AF9F);
GAME_VAR(uint8_t, allowStickY, 0x00C1AFA0);
GAME_VAR(float, cameraHeading, 0x00C3CD08);
GAME_VAR(uint8_t, clockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);
GAME_VAR(int32_t, noStatModel1, 0x00BF42DC);
GAME_VAR(int32_t, noStatModel2, 0x00BF42D4);
GAME_VAR(int32_t, noStatModel3, 0x00BF42D0);
GAME_VAR(int32_t, noStatModel4, 0x00BF42CC);
GAME_ARRAY(int32_t, grades, 0x00C1B02C);                // 10
// the object in hand, held in front of the camera (photos): its bone (-2: off), scale
// and offsets along the camera's axes
GAME_VAR(float, heldScale, 0x00C2C0A8);
GAME_VAR(int32_t, heldBone, 0x00C2C0AC);
GAME_VAR(float, heldOffsetRight, 0x00C2C0C0);
GAME_VAR(float, heldOffsetForward, 0x00C2C0C4);
GAME_VAR(float, heldOffsetUp, 0x00C2C0C8);
GAME_VAR(float, heldUnknownCC, 0x00C2C0CC);
GAME_VAR(uint8_t*, pedPool, 0x00C0F5F0);               // CPool<CPed>*
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);
GAME_VAR(void*, yearbookPhotoPed, 0x00C1AE70);         // skipped by the line of sight
GAME_VAR(void*, unknownC1AA04, 0x00C1AA04);
inline void* const unknownD02850 = reinterpret_cast<void*>(0x00D02850);
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
GAME_ARRAY(uint32_t, girlfriendHashes, 0x00C2C0D0);     // 14
GAME_VAR(uint8_t, cheatPadsEnabled, 0x00A135DA);
inline void* const unknownC3B808 = reinterpret_cast<void*>(0x00C3B808);
GAME_VAR(void*, unknownC2C108, 0x00C2C108);
GAME_VAR(uint8_t*, missionConfig, 0x020C88B8);          // +0x10C.. mission names, +0x110.. floats
inline void* const actionInput = reinterpret_cast<void*>(0x01B652A8);

GAME_FN(void*, __cdecl, PP_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, PP_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PP_RegisterReference, 0x004657D0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __cdecl, PP_World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __cdecl, PP_World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __thiscall, PP_Ped_4778E0, 0x004778E0, void* ped, void* entity);
GAME_FN(void, __thiscall, PP_Ped_47D0D0, 0x0047D0D0, void* ped, void* bike);   // on a bike
GAME_FN(void, __thiscall, PP_Ped_SetVehicle, 0x00476180, void* ped, void* vehicle);
GAME_FN(void, __thiscall, PP_Ped_476DD0, 0x00476DD0, void* ped);
GAME_FN(void, __thiscall, PP_Ped_PlayAction, 0x004740F0, void* ped, const char* node, const char* file);
GAME_FN(int32_t*, __thiscall, PP_Vehicle_GetClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(void, __thiscall, PP_Seats_Remove, 0x004CE900, void* seats, void* ped);
GAME_FN(void, __thiscall, PP_Seats_SetDriver, 0x004CE600, void* seats, void* ped);
GAME_FN(void, __thiscall, PP_Seats_SetOccupant, 0x004CE590, void* seats, uint32_t seat, void* ped);
GAME_FN(uint8_t, __thiscall, PP_Seats_IsSeatTaken, 0x004CE510, void* seats, uint32_t seat, int orEntering);
GAME_FN(uint32_t, __thiscall, PP_Seats_HasFrontPassenger, 0x004CE5F0, void* seats);
GAME_FN(void, __thiscall, PP_Fn5F40A0, 0x005F40A0, void* self);
GAME_FN(uint8_t, __thiscall, PP_Fn5F4800, 0x005F4800, void* self, void* input, int, int);
GAME_FN(void, __thiscall, PP_ActionHandle_Release, 0x005D9C50, void* handle);
GAME_FN(void, __thiscall, PP_Ped_Destruct, 0x00480CC0, void* ped);
GAME_FN(void, __thiscall, PP_Ped_Construct, 0x00480130, void* ped, int, int);
GAME_FN(uint8_t, __thiscall, PP_Ped_4CE690, 0x004CE690, void* ped);
GAME_FN(void, __thiscall, PP_Ped_Vf18, 0x004769C0, void* ped);
GAME_FN(uint8_t, __thiscall, PP_Ped_Vf22, 0x00478520, void* ped);
GAME_FN(void, __thiscall, PP_Ped_Vf42, 0x00479260, void* ped, int, int);
GAME_FN(void, __thiscall, PP_Ped_Vf12, 0x00481420, void* ped);
GAME_FN(uint8_t, __thiscall, PP_IsFrameForRandomUpdate, 0x00465A60, void* entity, uint32_t frames);
GAME_FN(uint8_t, __thiscall, PP_Info_IsFlagSet, 0x00446860, void* info, int flag);
GAME_FN(void, __thiscall, PP_Info_SetFlag, 0x004467F0, void* info, int flag, int value);
GAME_FN(uint32_t, __thiscall, PP_Info_Get4466F0, 0x004466F0, void* info, int id);
GAME_FN(void, __thiscall, PP_Info_445E30, 0x00445E30, void* info, int);
GAME_FN(int, __cdecl, PP_GetStatInt, 0x00459A00, int stat);
GAME_FN(void, __cdecl, PP_IncrementStat, 0x00458480, int stat, int amount);
GAME_FN(void*, __thiscall, PP_Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(void*, __thiscall, PP_Placeable_MatrixPtr, 0x008B6120, void* placeable);   // +0x14
GAME_FN(void, __thiscall, PP_Placeable_SetMatrix, 0x0046DE90, void* placeable, const CMatrix* m, int force);
GAME_FN(void, __thiscall, PP_Matrix_CopyRotPos, 0x00412720, void* dst, const void* src);
GAME_FN(void, __thiscall, PP_Matrix_UpdateAttached, 0x00413A70, void* m);
GAME_FN(void, __thiscall, PP_Matrix_FromNiTransform, 0x00412A70, CMatrix* m, const NiTransform* t);
GAME_FN(void, __thiscall, PP_Matrix_Multiply, 0x00412800, void* m, const CMatrix* by);
GAME_FN(void, __thiscall, PP_Vector_Assign, 0x004129D0, void* dst, const CVector* src);
GAME_FN(void, __thiscall, PP_Quat_ToNiTransform, 0x00413C50, const void* quat, NiTransform* out);
GAME_FN(uint8_t, __cdecl, PP_IsWeaponItem, 0x0045B000, int item);
GAME_FN(uint8_t, __cdecl, PP_IsHeldItem, 0x0045B010, int item);
GAME_FN(uint8_t, __cdecl, PP_IsMoneyItem, 0x0045B020, int item);
GAME_FN(int, __cdecl, PP_ItemWeapon, 0x00541EC0, int item);
GAME_FN(int, __cdecl, PP_WeaponAmount, 0x00541E00, int weapon);
GAME_FN(uint8_t, __thiscall, PP_Ped_CanAfford, 0x004730E0, void* ped, int amount);
GAME_FN(int, __thiscall, PP_Inventory_GetAmmo, 0x0045CF40, void* inventory, int model);
GAME_FN(void, __thiscall, PP_Inventory_45D0C0, 0x0045D0C0, void* inventory, int model, int);
GAME_FN(void, __thiscall, PP_Inventory_45D030, 0x0045D030, void* inventory, int model, int, int, int);
GAME_FN(void, __thiscall, PP_Object_4D9410, 0x004D9410, void* object);
GAME_FN(void, __thiscall, PP_Fn5F3830, 0x005F3830, void* self, uintptr_t);
GAME_FN(void, __thiscall, PP_HudWeapons_55C840, 0x0055C840, void* hud, int);
GAME_FN(void, __thiscall, PP_HudWeapons_55C8D0, 0x0055C8D0, void* hud);
GAME_FN(float, __cdecl, PP_Pad_GetValue, 0x00738840, int button, int pad);
GAME_FN(float, __cdecl, PP_GetATanOfXY, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, PP_LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(void*, __cdecl, PP_Node_6C82F0, 0x006C82F0, void* node);          // +0x140
GAME_FN(NiTransform*, __cdecl, PP_Node_WorldTransform, 0x006C9A90, void* node);  // +0x14
GAME_FN(void*, __cdecl, PP_RwObject_Node, 0x006C7370, void* rwObject);    // +4
GAME_FN(void, __cdecl, PP_Node_SetDirty, 0x006C9AF0, void* node);
GAME_FN(void, __cdecl, PP_SetGameClock, 0x00426070, int hours, int minutes);
GAME_FN(uint8_t*, __thiscall, PP_Ped_474BE0, 0x00474BE0, void* ped);      // +4 the ped who did it
GAME_FN(void, __thiscall, PP_Camera_GetForward, 0x0041AA70, void* camera, CVector* out);
GAME_FN(void*, __thiscall, PP_AnimObject_GetBone, 0x00464EA0, void* node, int bone);
GAME_FN(void, __thiscall, PP_Entity_443AA0, 0x00443AA0, void* entity);
GAME_FN(void, __thiscall, PP_Entity_UpdateFromRwFrame, 0x00465AB0, void* entity);
GAME_FN(void*, __thiscall, PP_Construct4B9460, 0x004B9460, void* self);
GAME_FN(void, __thiscall, PP_Ped_478F80, 0x00478F80, void* ped, uintptr_t, int);
GAME_FN(void, __thiscall, PP_Ped_68EBB0, 0x0068EBB0, void* ped, int);
GAME_FN(int, __thiscall, PP_MissionMgr_Find, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, PP_MissionMgr_GetStats, 0x006AA7C0, void* mgr, int mission);   // +2 successes
GAME_FN(void, __thiscall, PP_MissionMgr_6ACF60, 0x006ACF60, void* mgr, void* ped);
GAME_FN(void*, __cdecl, PP_Ped_OperatorNew, 0x004789E0, size_t size);
GAME_FN(void, __thiscall, PP_Physical_SetOrientation, 0x0046A690, void* physical, float x, float y, float z);
GAME_FN(void*, __thiscall, PP_Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t, __thiscall, PP_Ped_473FB0, 0x00473FB0, void* ped);
GAME_FN(CVector*, __thiscall, PP_PedCam_48CEE0, 0x0048CEE0, void* cam, CVector* out);
GAME_FN(CVector*, __thiscall, PP_PedCam_GetPosition, 0x0048D020, void* cam, CVector* out);
GAME_FN(void, __thiscall, PP_Ped_GetForward, 0x004154A0, void* ped, CVector* out);
GAME_FN(CVector*, __cdecl, PP_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, PP_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, PP_DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, PP_Distance, 0x0041A950, const CVector* a, const CVector* b);
GAME_FN(uint8_t, __cdecl, PP_CalcScreenCoors, 0x0052AEE0, const CVector* pos, CVector* out, float* w, float* h, int, int,
        int, int);
GAME_FN(float, __thiscall, PP_Camera_GetZoom, 0x004F1250, void* camera);
GAME_FN(void, __thiscall, PP_ColPoint_Construct, 0x00415FA0, void* colPoint);
GAME_FN(void, __thiscall, PP_ColPoint_Destruct, 0x00415500, void* colPoint);
GAME_FN(uint8_t, __cdecl, PP_ProcessLineOfSight, 0x00461EA0, const CVector* start, const CVector* end, void* colPoint,
        re::Ptr32<void>* entity, int flags, int, int);
GAME_FN(void, __thiscall, PP_NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, PP_NameKey_Destruct, 0x0049A650, uint32_t* key);
GAME_FN(uint8_t, __cdecl, PP_Fn6A1350, 0x006A1350, int model);
GAME_FN(void, __cdecl, PP_Fn6A2250, 0x006A2250, int model, uint32_t* a, uint32_t* b, uint32_t* c);
GAME_FN(uint8_t, __thiscall, PP_Fn45BE70, 0x0045BE70, void* self, uint32_t a, uint32_t b, uint32_t c);
GAME_FN(int, __thiscall, PP_ActionHandle_5D80D0, 0x005D80D0, void* handle);
GAME_FN(int, __thiscall, PP_ActionHandle_5D80B0, 0x005D80B0, void* handle);
GAME_FN(void*, __thiscall, PP_ActionHandle_5D9C20, 0x005D9C20, void* handle);
GAME_FN(int, __thiscall, PP_Fn5DB8F0, 0x005DB8F0, void* self, int);
GAME_FN(void, __cdecl, PP_Fn5D8270, 0x005D8270, void*);
GAME_FN(void, __cdecl, PP_Fn5D8670, 0x005D8670, void*, int);
GAME_FN(void, __cdecl, PP_Fn5D85C0, 0x005D85C0, void*, int);
GAME_FN(void, __cdecl, PP_Fn5D82A0, 0x005D82A0, void*, int, int);
GAME_FN(uint32_t, __cdecl, PP_HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, PP_Fn5E2B20, 0x005E2B20, void* self);
GAME_FN(void, __cdecl, PP_Fn51AB30, 0x0051AB30);
GAME_FN(void, __thiscall, PP_Camera_4F2C20, 0x004F2C20, void* camera, int, int, int, int, int);
GAME_FN(void, __thiscall, PP_Ped_4732A0, 0x004732A0, void* ped, int, int);
GAME_FN(void, __thiscall, PP_Targeting_4D5EA0, 0x004D5EA0, void* targeting);
GAME_FN(void, __thiscall, PP_Targeting_4D5F70_x64, 0x004D5F70, void* targeting, float, float, float, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void PP_Targeting_4D5F70(void* a0, CVector a1, int a2) {
    return PP_Targeting_4D5F70_x64(a0, a1.x, a1.y, a1.z, a2);
}
GAME_FN(void, __thiscall, PP_Fn499330, 0x00499330, void* self, float, int, float, int);
GAME_FN(void, __thiscall, PP_Fn497570, 0x00497570, void* self);
GAME_FN(void, __thiscall, PP_Ped_450970, 0x00450970, void* ped);
GAME_FN(void, __cdecl, PP_Fn4A6560, 0x004A6560);
GAME_FN(uint8_t, __thiscall, PP_ModelInfo_520440, 0x00520440, void* modelInfo);
GAME_FN(void, __thiscall, PP_Fn4E9CB0, 0x004E9CB0, void* self, void* ped, void* vehicle);
GAME_FN(void, __thiscall, PP_Fn49DC80, 0x0049DC80, void* self, void* ped, void* vehicle);
// (ours below, called where modes apply)
GAME_FN(void, __thiscall, PP_Ped_49A470, 0x0049A470, void* ped, void* vehicle, uint32_t seat);
GAME_FN(void, __thiscall, PP_Ped_49A580, 0x0049A580, void* ped, void* vehicle, int front);
GAME_FN(CVector*, __thiscall, PP_Camera_GetRight, 0x0049AAC0, void* camera, CVector* out);
GAME_FN(CVector*, __thiscall, PP_Camera_GetUp, 0x0049AB10, void* camera, CVector* out);
GAME_FN(void, __cdecl, PP_QuatToTransform, 0x0049AAA0, const void* quat, NiTransform* out);
GAME_FN(void*, __thiscall, PP_PlayerPed_Construct, 0x0049B300, void* ped);
GAME_FN(uint8_t, __thiscall, PP_IsValidYearbookPhoto, 0x0049B680, void* player, void* ped, float* score);
GAME_FN(void, __thiscall, PP_PlayerPed_49A6C0, 0x0049A6C0, void* player);
GAME_FN(float, __cdecl, PP_GetStickDirection, 0x0049AB80, CVector* out);

// the vehicle "class" 0x4CAF30 gives (10: see the +0x48 / +0xD0 flags below)
int32_t VehicleClass(void* vehicle) {
    int32_t out;
    return *PP_Vehicle_GetClass(vehicle, &out);
}

CVector* EntityPosition(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return reinterpret_cast<CVector*>(matrix ? matrix + 0x30 : static_cast<uint8_t*>(entity) + 4);
}

// ---- getting in and out of vehicles ----

// 0x49A3A0 (cdecl): 0x4778E0 on the ped (a callback's)
void __cdecl Ped_Callback4778E0(void* ped, void* entity) {
    PP_Ped_4778E0(ped, entity);
}

// 0x49A3B0 (cdecl): the ped into the vehicle at once (a car: the driver's seat; a
// bike: 0x47D0D0), the vehicle told (its vf6: 2 a car, 3 a bike)
void __cdecl Ped_WarpIntoVehicle(void* ped, void* vehicle) {
    if (At<int32_t>(vehicle, 0x46C) == 0) {
        PP_Ped_49A580(ped, vehicle, 1);
        Virtual<void>(vehicle, 6, 2);
        return;
    }
    PP_Ped_47D0D0(ped, vehicle);
    Virtual<void>(vehicle, 6, 3);
}

// 0x49A3F0 (thiscall): out of its vehicle at once
void __fastcall Ped_WarpOutOfCar(uint8_t* ped, re::Edx) {
    if (At<void*>(ped, 0x1554)) {
        if (VehicleClass(At<void*>(ped, 0x1554)) == 10) {
            At<int32_t>(ped, 0x48) = 1;
            At<int32_t>(ped, 0xD0) = 0;
        }
        At<uint8_t>(At<void*>(ped, 0x1554), 0x33A) &= 0xFD;
        At<uint8_t>(At<void*>(ped, 0x1554), 0x2E4) = 0;
        PP_Seats_Remove(At<uint8_t*>(ped, 0x1554) + 0x1C4, ped);
    }
    PP_Ped_SetVehicle(ped, nullptr);
    At<int32_t>(ped, 0x20) = 1;
    PP_Fn5F40A0(At<uint8_t*>(ped, 0x2E0) + 0x258);
}

// 0x49A470 (thiscall): into the car's seat at once
void __fastcall Ped_WarpIntoSeat(uint8_t* ped, re::Edx, uint8_t* vehicle, uint32_t seat) {
    if (!vehicle) {
        return;
    }
    PP_Ped_SetVehicle(ped, vehicle);
    At<int32_t>(ped, 0x20) = 0;
    At<int32_t>(ped, 0x1324) = 0;
    if (At<int32_t>(vehicle, 0x46C) == 1) {
        PP_Seats_SetDriver(vehicle + 0x1C4, ped);
    } else {
        PP_Seats_SetOccupant(vehicle + 0x1C4, seat, ped);
    }
    if (At<int32_t>(ped, 0x1310) == 13) {   // the player
        Virtual<void>(vehicle, 6, 0);
    }
    PP_World_Remove(ped);
    PP_Ped_476DD0(ped);
    PP_World_Add(ped);
    if ((vehicle[0x33A] & 2) == 0) {
        vehicle[0x33A] |= 2;
    }
    At<int32_t>(ped, 0x132C) = 1;
    At<int32_t>(ped, 0x13A4) = 0;
    // (the old reference isn't cleaned up)
    if (At<void*>(ped, 0x430) != vehicle) {
        At<void*>(ped, 0x430) = vehicle;
        PP_RegisterReference(vehicle, &At<void*>(ped, 0x430));
    }
    if (!PP_Fn5F4800(At<uint8_t*>(ped, 0x2E0) + 0x258, actionInput, 1, 1)) {
        PP_Fn5F40A0(At<uint8_t*>(ped, 0x2E0) + 0x258);
        PP_Ped_PlayAction(ped, "/Global/Vehicles/Cars/Vehicles_CarRide", "Act/Vehicles.act");
    }
    if (VehicleClass(vehicle) == 10) {
        At<int32_t>(ped, 0x48) = 0;
        At<int32_t>(ped, 0xD0) = 1;
    }
}

// 0x49A580 (thiscall): into a free seat at once (`driver`: the driver's; else the
// front passenger's, when the car takes more than it has)
void __fastcall Ped_WarpIntoFreeSeat(uint8_t* ped, re::Edx, uint8_t* vehicle, int driver) {
    int32_t type = At<int32_t>(vehicle, 0x46C);
    if (type == 1) {
        PP_Ped_47D0D0(ped, vehicle);
        return;
    }
    if (type != 0) {
        return;
    }
    void* seats = vehicle + 0x1C4;
    if (re::Flag(driver)) {
        if (!PP_Seats_IsSeatTaken(seats, 0, 1)) {
            PP_Ped_49A470(ped, vehicle, 0);
        }
        return;
    }
    int32_t limit = At<int32_t>(vehicle, 0x28C);
    if (int32_t(PP_Seats_HasFrontPassenger(seats)) < limit) {
        for (uint32_t seat = 1; seat <= 1; seat++) {
            if (!PP_Seats_IsSeatTaken(seats, seat, 1)) {
                PP_Ped_49A470(ped, vehicle, seat);
                return;
            }
        }
    }
}

// ---- CPlayerPed ----

// 0x49A620 (thiscall): the destructor's body
void __fastcall PlayerPed_Destruct(uint8_t* ped, re::Edx) {
    At<void*>(ped, 0) = reinterpret_cast<void*>(0x00911BA4);
    PP_Free(At<void*>(ped, 0x1ED8));
    PP_ActionHandle_Release(ped + 0x1F00);
    PP_Ped_Destruct(ped);
}

// 0x49A650: empty (a stripped debug call; also used as destructors of 4-byte keys)
void __cdecl EmptyFunction() {}

// 0x49A660 / 0x49A670: the player out of / into the world
void __cdecl PlayerPed_RemoveFromWorld() {
    PP_World_Remove(FindPlayerPed_ptr);
}

void __cdecl PlayerPed_AddToWorld() {
    PP_World_Add(FindPlayerPed_ptr);
}

// 0x49A680 (thiscall) vf18: nothing on a class-10 vehicle (with 0x4CE690), or in
// game mode 12
void __fastcall PlayerPed_Vf18(uint8_t* ped, re::Edx) {
    if (PP_Ped_4CE690(ped) && At<void*>(ped, 0x1554) && VehicleClass(At<void*>(ped, 0x1554)) == 10) {
        return;
    }
    if (gameMode == 12) {
        return;
    }
    PP_Ped_Vf18(ped);
}

// 0x49A6C0 (thiscall): now and then: player flag 0xB when every grade is 5 or more,
// 0xC when ten stats add up to 5 or more
void __fastcall PlayerPed_UpdateAchievements(uint8_t* ped, re::Edx) {
    if (!PP_IsFrameForRandomUpdate(ped, 0x100)) {
        return;
    }
    if (!PP_Info_IsFlagSet(playerInfo, 0xB)) {
        uint8_t all = 1;
        for (int i = 0; i < 10; i++) {
            all &= grades[i] >= 5 ? 1 : 0;
        }
        if (all) {
            PP_Info_SetFlag(playerInfo, 0xB, 1);
        }
    }
    if (!PP_Info_IsFlagSet(playerInfo, 0xC)) {
        int s143 = PP_GetStatInt(0x143);
        int s13D = PP_GetStatInt(0x13D);
        int s13A = PP_GetStatInt(0x13A);
        int s140 = PP_GetStatInt(0x140);
        int s134 = PP_GetStatInt(0x134);
        int s137 = PP_GetStatInt(0x137);
        int s146 = PP_GetStatInt(0x146);
        int s14F = PP_GetStatInt(0x14F);
        int s149 = PP_GetStatInt(0x149);
        int s14C = PP_GetStatInt(0x14C);
        // (int adds wrap: unsigned to keep that defined)
        uint32_t sum = uint32_t(s14C) + uint32_t(s149) + uint32_t(s14F) + uint32_t(s146) + uint32_t(s137) +
                       uint32_t(s134) + uint32_t(s140) + uint32_t(s13A) + uint32_t(s13D) + uint32_t(s143);
        if (int32_t(sum) >= 5) {
            PP_Info_SetFlag(playerInfo, 0xC, 1);
        }
    }
}

// 0x49A810 (thiscall): its matrix kept at +0x1630
void __fastcall Ped_StoreMatrix(uint8_t* ped, re::Edx) {
    void* matrix = PP_Placeable_GetMatrix(ped);
    PP_Matrix_CopyRotPos(ped + 0x1630, matrix);
    PP_Matrix_UpdateAttached(ped + 0x1630);
}

// 0x49A850 (thiscall): the ped has the item (a weapon's: `amount` of it; a held
// object's: in hand; money: can afford it)
uint8_t __fastcall Ped_HasItem(uint8_t* ped, re::Edx, int item, int amount) {
    if (PP_IsWeaponItem(item)) {
        int weapon = PP_ItemWeapon(item);
        if (weapon == 0x50) {
            return 1;
        }
        if (weapon < 0 || PP_WeaponAmount(weapon) < amount) {
            return 0;
        }
        return 1;
    }
    if (PP_IsHeldItem(item)) {
        uint8_t* held = At<uint8_t*>(ped, 0x1D0);
        switch (item - 0x11) {
        case 0:
            return At<int16_t>(held, 0x10E) == itemModel11 ? 1 : 0;
        case 2:
            if (At<int16_t>(held, 0x10E) == itemModel13) {
                return 1;
            }
            [[fallthrough]];
        case 3:
            return At<int16_t>(held, 0x10E) == itemModel14 ? 1 : 0;
        case 4:
            return At<int16_t>(held, 0x10E) == itemModel15 ? 1 : 0;
        default:
            return 0;
        }
    }
    if (PP_IsMoneyItem(item)) {
        if (item == 0x16) {
            return PP_Ped_CanAfford(ped, 100) ? 1 : 0;
        }
        if (item == 0x17) {
            return PP_Ped_CanAfford(ped, 500) ? 1 : 0;
        }
        if (item == 0x18) {
            return PP_Ped_CanAfford(ped, 1000) ? 1 : 0;
        }
    }
    return 0;
}

// 0x49A980 (thiscall): every weapon but five taken away (0x45D0C0 with 1000); the one
// in hand put away, the HUD's cleared
void __fastcall Ped_RemoveWeapons(uint8_t* ped, re::Edx) {
    for (int model = firstWeaponModel; model <= lastWeaponModel; model++) {
        if (model == keepModel1 || model == keepModel2 || model == keepModel3 || model == keepModel4 ||
            model == keepModel5) {
            continue;
        }
        if (!PP_Inventory_GetAmmo(At<void*>(ped, 0x1C4), model)) {
            continue;
        }
        PP_Inventory_45D0C0(At<void*>(ped, 0x1C4), model, 1000);
        if (At<int16_t>(At<void*>(ped, 0x1D0), 0x10E) == model) {
            PP_Object_4D9410(At<void*>(ped, 0x1D0));
            uint8_t* intelligence = At<uint8_t*>(ped, 0x2E0);
            if (intelligence[0x33C]) {
                PP_Fn5F3830(intelligence + 0x300, At<int>(intelligence, 0x4B0));
                intelligence[0x33C] = 0;
            }
            At<int32_t>(ped, 0x1574) = -1;
            At<int32_t>(At<void*>(ped, 0x2E0), 0x18) = -1;
        }
        uint8_t* hud = hudWeapons;
        if (At<int32_t>(hud, 0xC0) == model) {
            PP_HudWeapons_55C840(hud, -1);
            PP_HudWeapons_55C8D0(hud);
        }
    }
}

// 0x49AA80 (thiscall) vf22: CPed's, or in the photo camera
uint8_t __fastcall PlayerPed_Vf22(uint8_t* ped, re::Edx) {
    uint8_t result = PP_Ped_Vf22(ped);
    if (!result && gameMode != 2) {
        return result;
    }
    return 1;
}

// 0x49AAA0 (cdecl): a quaternion into a transform (scale 1)
void __cdecl QuatToTransform(const void* quat, NiTransform* out) {
    PP_Quat_ToNiTransform(quat, out);
    out->scale = 1.0f;
}

// 0x49AAC0 / 0x49AB10 (thiscall on the camera): its rotation's first / second column
CVector* __fastcall Camera_GetRight(uint8_t* camera, re::Edx, CVector* out) {
    NiTransform* t = PP_Node_WorldTransform(PP_Node_6C82F0(At<void*>(camera, 0x8C)));
    CVector v = {t->rot[0], t->rot[3], t->rot[6]};
    *out = v;
    return out;
}

CVector* __fastcall Camera_GetUp(uint8_t* camera, re::Edx, CVector* out) {
    NiTransform* t = PP_Node_WorldTransform(PP_Node_6C82F0(At<void*>(camera, 0x8C)));
    CVector v = {t->rot[1], t->rot[4], t->rot[7]};
    *out = v;
    return out;
}

// 0x49AB60 (thiscall)
void __fastcall PlayerPed_Reset49AB60(uint8_t* ped, re::Edx) {
    unknownC1A9A8 = 1.0f;
    At<int32_t>(ped, 0x20) = 1;
    ped[0x1E90] = 1;
}

// 0x49AB80 (cdecl): the left stick as a direction in the world (by the camera's
// heading) and how far it's pushed (0 .. 1)
float __cdecl GetStickDirection(CVector* out) {
    float x;
    if (lockButtonInputs && !allowStickX) {
        x = 0.0f;
    } else {
        x = PP_Pad_GetValue(0x12, 0);
    }
    float y;
    if (lockButtonInputs && !allowStickY) {
        y = 0.0f;
    } else {
        y = PP_Pad_GetValue(0x13, 0);
    }
    if (x == 0.0f && y == 0.0f) {
        return 0.0f;
    }
    float angle = PP_GetATanOfXY(x, y, 0.0f, 0.0f) + cameraHeading;
    angle = PP_LimitRadianAngle(angle);
    out->x = static_cast<float>(std::sin(static_cast<double>(angle)));
    out->y = static_cast<float>(std::cos(static_cast<double>(angle)));
    out->z = 0.0f;
    float lengthSq = x * x + y * y;
    lengthSq = std::fabs(lengthSq);
    float length = static_cast<float>(std::sqrt(static_cast<double>(lengthSq)));
    if (length < 0.0f) {
        return 0.0f;
    }
    if (1.0f < length) {
        return 1.0f;
    }
    return length;
}

// 0x49AD00 (thiscall): out of the unequip weapon: it's taken away after a delay
void __fastcall PlayerPed_StartUnequipTimer(uint8_t* ped, re::Edx) {
    if (ped[0x1F3C] == 0 && PP_Inventory_GetAmmo(At<void*>(ped, 0x1C4), unequipModel) <= 0) {
        At<uint32_t>(ped, 0x1F40) = timeInMilliseconds + unequipDelay;
        ped[0x1F3C] = 1;
    }
}

// 0x49ADA0 (thiscall) vf42: knocked out (the clock to 2:00 when it's late; stats by
// who did it)
void __fastcall PlayerPed_Vf42(uint8_t* ped, re::Edx, int flag, int arg) {
    if (static_cast<uint8_t>(flag)) {
        At<int32_t>(ped, 0x1ED4) += 1;
    }
    int hours = clockHours;
    if (hours == 1 ? clockMinutes >= 0x19 : (hours > 1 && hours < 8)) {
        PP_SetGameClock(2, 0);
        PP_Info_445E30(playerInfo, 0);
    }
    uint8_t* attacker = At<uint8_t*>(PP_Ped_474BE0(ped), 4);
    if (attacker) {
        int stat = -1;
        switch (At<int32_t>(attacker, 0x1310)) {
        case 1: stat = 0x55; break;
        case 2: stat = 0x5D; break;
        case 3: stat = 0x5F; break;
        case 4: stat = 0x5B; break;
        case 5: stat = 0x59; break;
        case 11: stat = 0x57; break;
        default: break;
        }
        if (stat != -1) {
            PP_IncrementStat(stat, 1);
        }
        int model = At<int16_t>(attacker, 0x10E);
        if (model != noStatModel1 && model != noStatModel2 && model != noStatModel3 && model != noStatModel4) {
            PP_IncrementStat(0x35, 1);
        }
    }
    PP_Ped_Vf42(ped, flag, arg);
}

// 0x49AEB0 (thiscall): unless the vehicle it's on is model 10082, these cleared
void __fastcall Ped_Clear13C4(uint8_t* ped, re::Edx) {
    uint8_t* vehicle = At<uint8_t*>(ped, 0x548);
    if (!vehicle || At<uint16_t>(vehicle, 0x10E) != 0x2762) {
        At<int32_t>(ped, 0x13C4) = 0;
        At<int32_t>(ped, 0x1548) = 0;
        At<int32_t>(ped, 0x14F8) = 0;
        At<int32_t>(ped, 0x1E98) = 0;
        At<int32_t>(ped, 0x1E9C) = 0;
    }
}

// 0x49AEF0 (thiscall): the object in hand placed in front of the camera (the photo
// camera's view), by a bone of its own when it has one, mirrored in x
void __fastcall PlayerPed_PlaceHeldObject(uint8_t* ped, re::Edx) {
    if (heldBone == -2) {
        return;
    }
    uint8_t* object = At<uint8_t*>(ped, 0x1D0);
    void* rwObject = At<void*>(object, 0x18);
    if (!rwObject || !At<void*>(ped, 0x114)) {
        return;
    }
    CVector forward, right, up;
    PP_Camera_GetForward(theCamera, &forward);
    PP_Camera_GetRight(theCamera, &right);
    PP_Camera_GetUp(theCamera, &up);
    NiTransform* camera = PP_Node_WorldTransform(PP_Node_6C82F0(cameraNode));
    CVector cameraPos = camera->trans;
    float offsetRight = heldOffsetRight;
    float offsetForward = heldOffsetForward;
    float offsetUp = heldOffsetUp;
    CVector alongUp = {up.x * offsetUp, up.y * offsetUp, offsetUp * up.z};
    CVector alongForward = {offsetForward * forward.x, forward.y * offsetForward, offsetForward * forward.z};
    CVector alongRight = {right.x * offsetRight, right.y * offsetRight, right.z * offsetRight};
    CVector pos = {cameraPos.x - alongRight.x, cameraPos.y - alongRight.y, cameraPos.z - alongRight.z};
    pos = {pos.x + alongForward.x, pos.y + alongForward.y, pos.z + alongForward.z};
    pos = {pos.x + alongUp.x, pos.y + alongUp.y, pos.z + alongUp.z};
    float scale = heldScale;
    right = {right.x * scale, right.y * scale, right.z * scale};

    CMatrix m;
    m.attached = nullptr;
    PP_Vector_Assign(&m.m[0], &right);
    // (the game leaves the w column as uninitialised stack; unused)
    m.m[3] = 0.0f;
    m.m[4] = forward.x * scale;
    m.m[5] = forward.y * scale;
    m.m[6] = forward.z * scale;
    m.m[7] = 0.0f;
    m.m[8] = up.x * scale;
    m.m[9] = up.y * scale;
    m.m[10] = up.z * scale;
    m.m[11] = 0.0f;
    m.m[12] = pos.x;
    m.m[13] = pos.y;
    m.m[14] = pos.z;
    m.m[15] = 0.0f;
    PP_Placeable_SetMatrix(object, &m, 1);

    void* node = At<void*>(At<void*>(object, 0x114), 0x94);
    if (node) {
        void* bone = PP_AnimObject_GetBone(node, heldBone);
        if (bone) {
            NiTransform t;
            t.scale = 1.0f;
            PP_QuatToTransform(Virtual<void*>(bone, 1), &t);
            float* translation = Virtual<float*>(bone, 0);
            t.trans.x = -translation[0];
            t.trans.y = translation[1];
            t.trans.z = translation[2];
            CMatrix boneMatrix;
            boneMatrix.attached = &t;   // (a spilled pointer lands there; never read)
            PP_Matrix_FromNiTransform(&boneMatrix, &t);
            PP_Matrix_Multiply(PP_Placeable_MatrixPtr(object), &boneMatrix);
        }
    }
    float* matrix = static_cast<float*>(PP_Placeable_MatrixPtr(object));
    CVector mirrored = {-matrix[0], -matrix[1], -matrix[2]};
    PP_Vector_Assign(PP_Placeable_MatrixPtr(object), &mirrored);
    PP_Entity_443AA0(object);
    void* frame = PP_RwObject_Node(rwObject);
    if (frame) {
        PP_Node_SetDirty(frame);
        PP_Entity_UpdateFromRwFrame(object);
    }
    if (At<void*>(object, 0x114)) {
        At<uint32_t>(At<void*>(object, 0x114), 0x98) &= 0xFFFFDBFF;
    }
    At<int32_t>(object, 0xA4) = 0;
    Virtual<void>(object, 19);
    Virtual<void>(object, 20);
}

// 0x49B300 (thiscall): the constructor
void* __fastcall PlayerPed_Construct(uint8_t* ped, re::Edx) {
    PP_Ped_Construct(ped, 0, 1);
    At<void*>(ped, 0) = reinterpret_cast<void*>(0x00911BA4);
    At<int32_t>(ped, 0x1E98) = 0;
    At<float>(ped, 0x1EA0) = 0.0f;
    At<float>(ped, 0x1EA4) = 0.0f;
    At<float>(ped, 0x1EA8) = 0.0f;
    At<float>(ped, 0x1EB0) = 0.0f;
    At<float>(ped, 0x1EB4) = 0.0f;
    At<float>(ped, 0x1EB8) = 0.0f;
    At<int32_t>(ped, 0x1ED4) = 0;
    At<void*>(ped, 0x1F00) = nullptr;
    for (int offset = 0x1EDC; offset <= 0x1EFC; offset += 4) {
        At<float>(ped, offset) = 0.0f;
    }
    void* memory = PP_Malloc(0x2C);
    At<void*>(ped, 0x1ED8) = memory ? PP_Construct4B9460(memory) : nullptr;
    unknownC1A9A8 = 1.0f;
    At<int32_t>(ped, 0x20) = 1;
    ped[0x1E90] = 1;
    ped[0x1E91] = 0;
    PP_Ped_478F80(ped, 0, 0);
    PP_Ped_68EBB0(ped, 1);
    controlledPed = ped;
    At<int32_t>(ped, 0x1E94) = 0;
    At<CVector>(ped, 0x1EA0) = zeroVector;
    At<float>(ped, 0x1EAC) = 0.0f;
    At<CVector>(ped, 0x1EB0) = zeroVector;
    // (0x49AEB0 inlined)
    uint8_t* vehicle = At<uint8_t*>(ped, 0x548);
    if (!vehicle || At<uint16_t>(vehicle, 0x10E) != 0x2762) {
        At<int32_t>(ped, 0x13C4) = 0;
        At<int32_t>(ped, 0x1548) = 0;
        At<int32_t>(ped, 0x14F8) = 0;
        At<int32_t>(ped, 0x1E98) = 0;
        At<int32_t>(ped, 0x1E9C) = 0;
    }
    At<int32_t>(ped, 0x1E2C) = -1;
    At<int32_t>(ped, 0x1EBC) = 0;
    PP_ActionHandle_Release(ped + 0x1F00);
    heldBone = 0;
    heldOffsetRight = 0.0f;
    heldScale = 1.0f;
    heldOffsetForward = 0.0f;
    heldUnknownCC = 0.0f;
    heldOffsetUp = 0.0f;
    ped[0x1EC0] = 0;
    for (int i = 0; i < 14; i++) {
        At<int32_t>(ped, 0x1F04 + i * 4) = 1 << i;
    }
    At<int32_t>(ped, 0x1EC8) = -1;
    At<float>(ped, 0x1EC4) = 0.0f;
    At<int32_t>(ped, 0x1ECC) = -1;
    At<int32_t>(ped, 0x1ED0) = -1;
    // the three missions whose success adds to +0x1EC4 (vf12)
    if (At<uint32_t>(missionConfig, 0x10C) != 0) {
        At<int32_t>(ped, 0x1EC8) = PP_MissionMgr_Find(missionMgr, &At<uint32_t>(missionConfig, 0x10C));
    }
    if (At<uint32_t>(missionConfig, 0x114) != 0) {
        At<int32_t>(ped, 0x1ECC) = PP_MissionMgr_Find(missionMgr, &At<uint32_t>(missionConfig, 0x114));
    }
    if (At<uint32_t>(missionConfig, 0x11C) != 0) {
        At<int32_t>(ped, 0x1ED0) = PP_MissionMgr_Find(missionMgr, &At<uint32_t>(missionConfig, 0x11C));
    }
    At<uint32_t>(ped, 0x1F40) = 0;
    ped[0x1F3C] = 0;
    return ped;
}

// 0x49B570: the player made (heading 0), into the world, told to the mission manager
void __cdecl PlayerPed_SetupPlayerPed() {
    void* memory = PP_Ped_OperatorNew(0x1F44);
    FindPlayerPed_ptr = memory ? PP_PlayerPed_Construct(memory) : nullptr;
    // (0 degrees to radians)
    float angle = static_cast<float>(3.1415927410125732 * 0.0 / 180.0);
    PP_Physical_SetOrientation(FindPlayerPed_ptr, angle, angle, angle);
    PP_World_Add(FindPlayerPed_ptr);
    PP_MissionMgr_6ACF60(missionMgr, FindPlayerPed_ptr);
}

// 0x49B5F0: a ped other than the player targets the player (and 0x473FB0 of it)
uint8_t __cdecl AnyPedTargetsPlayer() {
    uint8_t* pool = pedPool;
    int size = At<int32_t>(pool, 8);
    for (int i = 0; i < size; i++) {
        if (At<uint8_t*>(pool, 4)[i] & 0x80) {
            continue;
        }
        uint8_t* ped = At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * i;
        if (!ped || At<int32_t>(ped, 0x1310) == 13) {
            continue;
        }
        uint8_t* target = static_cast<uint8_t*>(PP_Ped_GetTarget(ped));
        if (!target || (target[0x108] & 7) != 3) {
            continue;
        }
        target = static_cast<uint8_t*>(PP_Ped_GetTarget(ped));
        uint8_t* targetPed = nullptr;
        if (target) {
            targetPed = (target[0x108] & 7) == 3 ? target : nullptr;
        }
        // (targetPed is never nullptr here: the first call said it's a ped)
        if (At<int32_t>(targetPed, 0x1310) == 13 && PP_Ped_473FB0(ped)) {
            return 1;
        }
    }
    return 0;
}

// 0x49B680 (thiscall): the ped makes a yearbook photo: facing the camera, well on
// screen, near enough for the zoom, nothing in between; `score` the better the lower
uint8_t __fastcall PlayerPed_IsValidYearbookPhoto(uint8_t* player, re::Edx, uint8_t* ped, float* score) {
    CVector forward = {0.0f, 0.0f, 0.0f};
    CVector cameraPos = {0.0f, 0.0f, 0.0f};
    CVector tmp;
    const CVector* source;
    if (gameMode == 2) {
        NiTransform* t = PP_Node_WorldTransform(PP_Node_6C82F0(cameraNode));
        CVector column = {t->rot[2], t->rot[5], t->rot[8]};
        forward = column;
        forward.z = 0.0f;
        PP_Normalise(&forward);
        source = &PP_Node_WorldTransform(PP_Node_6C82F0(cameraNode))->trans;
    } else {
        uint8_t* camera = static_cast<uint8_t*>(FindPlayerPed_ptr) + 0xA64;
        forward = *PP_PedCam_48CEE0(camera, &tmp);
        forward.z = 0.0f;
        PP_Normalise(&forward);
        source = PP_PedCam_GetPosition(camera, &tmp);
    }
    cameraPos = *source;
    if (!ped) {
        return 0;
    }
    CVector pedForward;
    PP_Ped_GetForward(ped, &pedForward);
    CVector pedPos;
    PP_PedCam_GetPosition(ped + 0xA64, &pedPos);
    CVector screen = {0.0f, 0.0f, 0.0f};
    CVector toCamera;
    PP_VectorSub(&toCamera, &cameraPos, &pedPos);
    toCamera.z = 0.0f;
    PP_Normalise(&toCamera);
    float facing = toCamera.y * pedForward.y + pedForward.x * toCamera.x + pedForward.z * toCamera.z;
    if (static_cast<double>(facing) < static_cast<double>(0.9f)) {
        return 0;
    }
    float w, h;
    if (!PP_CalcScreenCoors(&pedPos, &screen, &w, &h, 1, 0, 0, 0)) {
        return 0;
    }
    if (!(screen.x > 128.0f) || !(screen.x < 512.0f)) {
        return 0;
    }
    if (!(screen.y > 96.0f) || !(screen.y < 384.0f)) {
        return 0;
    }
    double invZoom = 1.0 / static_cast<double>(PP_Camera_GetZoom(theCamera));
    float distance = static_cast<float>(static_cast<double>(PP_Distance(&cameraPos, &pedPos)) * invZoom);
    if (!(3.0f > distance)) {
        return 0;
    }
    float angle = PP_DotProduct(&pedForward, &forward);
    At<uint16_t>(player, 0x10A) = static_cast<uint16_t>(currentScanCode + 1);
    At<uint16_t>(ped, 0x10A) = static_cast<uint16_t>(currentScanCode + 1);
    alignas(4) uint8_t colPoint[0x28];
    PP_ColPoint_Construct(colPoint);
    re::Ptr32<void> hit = nullptr;
    yearbookPhotoPed = ped;
    PP_ProcessLineOfSight(&cameraPos, &pedPos, colPoint, &hit, 0xF, 0, 0);
    yearbookPhotoPed = nullptr;
    if (hit) {
        PP_ColPoint_Destruct(colPoint);
        return 0;
    }
    float absAngle = std::fabs(angle);
    float absDistance = std::fabs(distance);
    *score = static_cast<float>((1.0 - static_cast<double>(absAngle)) + static_cast<double>(absDistance) / 3.0);
    PP_ColPoint_Destruct(colPoint);
    return 1;
}

// 0x49B9E0 (thiscall): with C_Photography_1 passed: the best yearbook photo's ped
// (the last in the pool that makes one), its yearbook flags not set yet, played to
// the photo's action
void __fastcall PlayerPed_TakeYearbookPhoto(uint8_t* player, re::Edx) {
    uint32_t key;
    PP_NameKey_Construct(&key, "C_Photography_1");
    int mission = PP_MissionMgr_Find(missionMgr, &key);
    PP_NameKey_Destruct(&key);
    if (At<uint16_t>(PP_MissionMgr_GetStats(missionMgr, mission), 2) < 1) {
        return;
    }
    uint8_t* pool = pedPool;
    uint8_t* best = nullptr;
    for (int i = At<int32_t>(pool, 8); i != 0;) {
        i--;
        if (At<uint8_t*>(pool, 4)[i] & 0x80) {
            continue;
        }
        uint8_t* ped = At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * i;
        float score;
        if (ped && PP_Fn6A1350(At<int16_t>(ped, 0x10E)) && PP_IsValidYearbookPhoto(player, ped, &score)) {
            best = ped;
        }
    }
    if (!best) {
        return;
    }
    uint32_t a = 0, b = 0, c = 0;
    PP_Fn6A2250(At<int16_t>(best, 0x10E), &a, &b, &c);
    if (PP_Info_Get4466F0(playerInfo, 0x14) && (a & PP_Info_Get4466F0(playerInfo, 0x14))) {
        return;
    }
    if (PP_Info_Get4466F0(playerInfo, 0x15) && (b & PP_Info_Get4466F0(playerInfo, 0x15))) {
        return;
    }
    if (PP_Info_Get4466F0(playerInfo, 0x16) && (c & PP_Info_Get4466F0(playerInfo, 0x16))) {
        return;
    }
    if (!PP_Fn45BE70(unknownC1AA04, a, b, c)) {
        return;
    }
    void* handle = player + 0x1F00;
    int index = PP_ActionHandle_5D80D0(handle);
    if (index == -1) {
        return;
    }
    if (PP_Fn5DB8F0(unknownD02850, PP_ActionHandle_5D80B0(handle))) {
        void* s = PP_ActionHandle_5D9C20(handle);
        PP_Fn5D8270(s);
        PP_Fn5D8670(s, index);
        PP_Fn5D85C0(s, Virtual<int>(best, 43));
        PP_Fn5D82A0(s, 1, 0);
        return;
    }
    PP_ActionHandle_Release(handle);
}

// 0x49BBD0 (thiscall): the girlfriend slot's value for the model (0: not one; nothing
// for a model without info)
int32_t* __fastcall PlayerPed_GetGirlfriendIndex(uint8_t* player, re::Edx, int model, int32_t* out) {
    uint8_t* info = modelInfoPtrs[model];
    if (!info) {
        return out;
    }
    uint32_t hash = PP_HashName(reinterpret_cast<const char*>(info + 0x98));
    for (int i = 0; i < 14; i++) {
        if (girlfriendHashes[i] == hash) {
            *out = At<int32_t>(player, 0x1F04 + i * 4);
            return out;
        }
    }
    *out = 0;
    return out;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

// 0x49BC30 (thiscall) vf12: the player's process
void __fastcall PlayerPed_ProcessControl(uint8_t* player, re::Edx) {
    PP_Fn5E2B20(player + 0xDAC);
    if (cheatPadsEnabled) {
        if (PP_Pad_GetValue(10, 0) != 0.0f && PP_Pad_GetValue(11, 0) != 0.0f && PP_Pad_GetValue(12, 0) != 0.0f &&
            PP_Pad_GetValue(13, 0) != 0.0f) {
            PP_Fn51AB30();
        }
    }
    // the last positions
    CVector last = At<CVector>(player, 0x1EDC);
    At<CVector>(player, 0x1EE8) = last;
    At<CVector>(player, 0x1EF4) = last;
    At<CVector>(player, 0x1EDC) = *EntityPosition(player);
    if (At<int32_t>(player, 0x1E94) != 0) {
        if (gameMode != 2 && gameMode != 3 && gameMode != 4 && gameMode != 5) {
            PP_Camera_4F2C20(theCamera, 2, 0, 1, 0, 0);
        }
        At<CVector>(player, 0x124) = zeroVector;
        At<int32_t>(player, 0x1318) = 1;
        if (gameMode == 2 && debugActive == 0) {
            PP_Ped_4732A0(player, 0, 1);
        } else {
            PP_Ped_4732A0(player, 0xFF, 1);
        }
    } else {
        PP_Targeting_4D5EA0(At<void*>(player, 0x1C8));
        if (At<uint8_t>(At<void*>(player, 0x1C8), 0x7E)) {
            CVector direction = {0.0f, 0.0f, 0.0f};
            if (PP_GetStickDirection(&direction) > 0.3f) {
                PP_Targeting_4D5F70(At<void*>(player, 0x1C8), direction, 1);
            }
        }
    }
    PP_Ped_Vf12(player);
    PP_Fn499330(player + 0x34C, 0.0f, 0, 0.0f, 0);
    PP_Fn497570(player + 0x34C);
    if (At<int32_t>(player, 0x40) != 0) {
        return;
    }
    PP_Ped_450970(player);
    if (player[0x1C98] != 0) {
        return;
    }
    PP_Fn4A6560();
    PP_PlayerPed_49A6C0(player);
    uint8_t* vehicle = At<uint8_t*>(player, 0x548);
    if (vehicle && PP_ModelInfo_520440(modelInfoPtrs[At<int16_t>(vehicle, 0x10E)]) &&
        At<int32_t>(At<void*>(player, 0x548), 0x194) != 0) {
        PP_Fn4E9CB0(unknownC3B808, player, At<void*>(player, 0x548));
        PP_Fn49DC80(unknownC2C108, player, At<void*>(player, 0x548));
    }
    if (PP_IsFrameForRandomUpdate(player, 0x100)) {
        At<float>(player, 0x1EC4) = 0.0f;
        static const int missionFields[3][2] = {{0x1EC8, 0x110}, {0x1ECC, 0x118}, {0x1ED0, 0x120}};
        for (const auto& field : missionFields) {
            int mission = At<int32_t>(player, field[0]);
            if (mission != -1 && At<uint16_t>(PP_MissionMgr_GetStats(missionMgr, mission), 2) != 0) {
                At<float>(player, 0x1EC4) = At<float>(missionConfig, field[1]) + At<float>(player, 0x1EC4);
            }
        }
    }
    if (player[0x1F3C] != 0) {
        if (PP_Inventory_GetAmmo(At<void*>(player, 0x1C4), unequipModel) > 0) {
            player[0x1F3C] = 0;
            return;
        }
        if (At<uint32_t>(player, 0x1F40) <= timeInMilliseconds) {
            player[0x1F3C] = 0;
            PP_Inventory_45D030(At<void*>(player, 0x1C4), unequipModel, 1, 1, 0);
        }
    }
}

} // namespace

// (written above; too short to hook in the 32-bit game: the 64-bit dispatch's body, whatever
// it's called with)
RE_UNHOOKED_FN("EmptyFunction", 0x0049A650, &EmptyFunction);   // (1 byte)

#define C08P4_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P4_REVERSED("CPed::Callback4778E0", 0x0049A3A0, &Ped_Callback4778E0);
C08P4_REVERSED("CPed::WarpIntoVehicle", 0x0049A3B0, &Ped_WarpIntoVehicle);
C08P4_REVERSED("CPed::WarpOutOfCar", 0x0049A3F0, &Ped_WarpOutOfCar);
C08P4_REVERSED("CPed::WarpIntoSeat", 0x0049A470, &Ped_WarpIntoSeat);
C08P4_REVERSED("CPed::WarpIntoFreeSeat", 0x0049A580, &Ped_WarpIntoFreeSeat);
C08P4_REVERSED("CPlayerPed::Destruct", 0x0049A620, &PlayerPed_Destruct);
C08P4_REVERSED("CPlayerPed::RemoveFromWorld", 0x0049A660, &PlayerPed_RemoveFromWorld);
C08P4_REVERSED("CPlayerPed::AddToWorld", 0x0049A670, &PlayerPed_AddToWorld);
C08P4_REVERSED("CPlayerPed::Vf18", 0x0049A680, &PlayerPed_Vf18);
C08P4_REVERSED("CPlayerPed::UpdateAchievements", 0x0049A6C0, &PlayerPed_UpdateAchievements);
C08P4_REVERSED("CPed::StoreMatrix", 0x0049A810, &Ped_StoreMatrix);
C08P4_REVERSED("CPed::HasItem", 0x0049A850, &Ped_HasItem);
C08P4_REVERSED("CPed::RemoveWeapons", 0x0049A980, &Ped_RemoveWeapons);
C08P4_REVERSED("CPlayerPed::Vf22", 0x0049AA80, &PlayerPed_Vf22);
C08P4_REVERSED("QuatToTransform", 0x0049AAA0, &QuatToTransform);
C08P4_REVERSED("CCamera::GetRight", 0x0049AAC0, &Camera_GetRight);
C08P4_REVERSED("CCamera::GetUp_49AB10", 0x0049AB10, &Camera_GetUp);
C08P4_REVERSED("CPlayerPed::Reset49AB60", 0x0049AB60, &PlayerPed_Reset49AB60);
C08P4_REVERSED("GetStickDirection", 0x0049AB80, &GetStickDirection);
C08P4_REVERSED("CPlayerPed::StartUnequipTimer", 0x0049AD00, &PlayerPed_StartUnequipTimer);
C08P4_REVERSED("CPlayerPed::Vf42", 0x0049ADA0, &PlayerPed_Vf42);
C08P4_REVERSED("CPed::Clear13C4", 0x0049AEB0, &Ped_Clear13C4);
C08P4_REVERSED("CPlayerPed::PlaceHeldObject", 0x0049AEF0, &PlayerPed_PlaceHeldObject);
C08P4_REVERSED("CPlayerPed::Construct", 0x0049B300, &PlayerPed_Construct);
C08P4_REVERSED("CPlayerPed::SetupPlayerPed", 0x0049B570, &PlayerPed_SetupPlayerPed);
C08P4_REVERSED("CPed::AnyPedTargetsPlayer", 0x0049B5F0, &AnyPedTargetsPlayer);
C08P4_REVERSED("CPlayerPed::IsValidYearbookPhoto", 0x0049B680, &PlayerPed_IsValidYearbookPhoto);
C08P4_REVERSED("CPlayerPed::TakeYearbookPhoto", 0x0049B9E0, &PlayerPed_TakeYearbookPhoto);
C08P4_REVERSED("CPlayerPed::GetGirlfriendIndex", 0x0049BBD0, &PlayerPed_GetGirlfriendIndex);
C08P4_REVERSED("CPlayerPed::ProcessControl", 0x0049BC30, &PlayerPed_ProcessControl);
