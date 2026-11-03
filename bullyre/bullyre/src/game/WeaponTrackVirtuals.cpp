// The action tree's weapon tracks, their virtuals and helpers (0x65EBB0..0x65FAF0,
// 0x661470..0x662E90, 0x664350..0x665FD0): firing (aim point, spread, leading a moving
// target, the launch velocity solve), the extinguisher / poison spray / weapon effect
// tracks, ammo, weapon flags and scale, collision (the weapon's swing tests and hits) and
// impulses. The fields are the tracks' own past +0x18 (ActionTracksCommon.h); the owner's
// +8 is the ped, the ped's +0x1D0 its weapon object.
//
// WeaponFireTrack: +0x28 the lead scale, +0x30, +0x32 the weapon dropped when out of
// ammo, +0x34 the power scale, +0x3C the owner, +0x40 the ped, +0x44 the power (0..1),
// +0x48 the projectile's info (+0x60/+0x64 its speed at no / full power, +0x68..+0x78
// its limits, +0x80 gravity, +0x84, +0x120 spread, +0x124 arcs, +0x128).

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

GAME_VAR(uint8_t*, weaponButes_20C88C8, 0x020C88C8);
GAME_VAR(float, velocityScale_B31CB8, 0x00B31CB8);      // 20
GAME_VAR(int32_t, forcedWeapon_B31C9C, 0x00B31C9C);    // -1: the ped's weapon's
GAME_VAR(uint32_t, sprayBoneHash_1B65300, 0x01B65300);
GAME_VAR(uint8_t*, effectLibrary_B6F1E8, 0x00B6F1E8);
GAME_VAR(int32_t, gameMode, 0x00C3CCF8);
inline void* const fireManager_BD0CF8 = reinterpret_cast<void*>(0x00BD0CF8);
inline int32_t ModelId(uintptr_t at) {
    return *reinterpret_cast<int32_t*>(at);
}
GAME_VAR(uint8_t*, ignoredEntity_C1AE70, 0x00C1AE70);   // CWorld's entity to ignore
GAME_ARRAY(float, zeroVector3, 0x00BCC564);
GAME_VAR(float, aimForward_1B65C34, 0x01B65C34);
GAME_VAR(float, aimSide_1B65C30, 0x01B65C30);
GAME_VAR(float, aimHeight_1B65C38, 0x01B65C38);
GAME_VAR(int32_t, specialWeapon_BF4370, 0x00BF4370);
GAME_VAR(void*, niCamera, 0x00C3CCF4);
GAME_ARRAY(uint32_t, fireVectorA_1B65C48, 0x01B65C48);
GAME_ARRAY(uint32_t, fireVectorB_1B65C24, 0x01B65C24);
GAME_VAR(uint16_t, scanCode_C1AE88, 0x00C1AE88);        // CWorld::ms_nCurrentScanCode
GAME_VAR(uint32_t*, sectorLinks_C0F788, 0x00C0F788);    // the sectors' list nodes
GAME_ARRAY(uint32_t*, sectorObjects_C1B17C, 0x00C1B17C);   // CWorld's 36 x 36 sectors (5 lists each)
GAME_ARRAY(uint32_t*, sectorVehicles_C1B180, 0x00C1B180);
GAME_VAR(uint8_t, chapter_A147E0, 0x00A147E0);
GAME_VAR(uint8_t*, sweepColModel_CF3140, 0x00CF3140);  // the swept sphere's collision model
GAME_VAR(double, sectorMargin_900970, 0x00900970);     // 4
GAME_VAR(double, sectorSize_900AA0, 0x00900AA0);       // 50
GAME_VAR(double, sectorOffset_900A98, 0x00900A98);     // 18
GAME_VAR(float, sectorLast_900A94, 0x00900A94);        // 35
GAME_VAR(int32_t, sprayCanModel_BF44E0, 0x00BF44E0);
GAME_VAR(uint8_t, frisbeeShot_1B65C14, 0x01B65C14);
GAME_ARRAY(float, frisbeeFlight_1B65C18, 0x01B65C18);   // side, along, up scales
GAME_VAR(uint8_t*, minigame_20C8704, 0x020C8704);
GAME_VAR(uint8_t*, butes_20C88B8, 0x020C88B8);
GAME_VAR(void*, eventManager_C2C1A0, 0x00C2C1A0);
inline void* const sounds_CF3DA0 = reinterpret_cast<void*>(0x00CF3DA0);
// CMatrix (0x412770 makes the identity; +0x40 the attached matrix)
struct SweepMatrix {
    float m[12];
    float pos[4];      // +0x30
    uint32_t attach;   // +0x40
    uint32_t owns;     // +0x44
};
static_assert(sizeof(SweepMatrix) == 0x48);
// a line as start and direction (what 0x574A00 measures against)
struct SweepLine {
    float start[3];
    float dir[3];
};
//@@TOP@@

GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const V3* a, const V3* b);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, uint8_t* entity);
GAME_FN(float, __thiscall, Fn4D5E20, 0x004D5E20, uint8_t* object);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, Stats_Increment, 0x00458480, int32_t stat, int32_t amount);
GAME_FN(uint8_t*, __thiscall, Skeleton_FindBone, 0x00465580, void* skeleton, uint32_t hash, int32_t* bone);
GAME_FN(void, __thiscall, AnimManager_GetBonePosition, 0x006C1190, void* manager, void* anim, int32_t bone,
        float* out, uintptr_t);
GAME_FN(void, __thiscall, AnimManager_GetBoneTransform, 0x006BF570, void* manager, void* anim, int32_t bone,
        float* out, uintptr_t);
GAME_FN(float*, __thiscall, Entity_GetMatrix_46DD20, 0x0046DD20, uint8_t* entity);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline float* Matrix_TransformPoint_41A9A0(float* out, const float* matrix, const float* v) { return re::CallVec16<0x0041A9A0, float*>(out, matrix, v); }
GAME_FN(void, __thiscall, FireManager_Extinguish_x64, 0x0042E2D0, void* manager, re::Float3, re::Float3, float radius, float strength);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void FireManager_Extinguish(void* a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8) {
    return FireManager_Extinguish_x64(a0, re::Float3{a1, a2, a3}, re::Float3{a4, a5, a6}, a7, a8);
}
GAME_FN(uint16_t, __thiscall, EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(void, __thiscall, Weapon_Fn4DA490, 0x004DA490, uint8_t* weapon);
GAME_FN(int32_t, __thiscall, Weapon_AddEffect, 0x004DA520, uint8_t* weapon, int effect, int32_t bone, int flag,
        int flag2);
GAME_FN(void, __thiscall, Weapon_Fn4D9650, 0x004D9650, uint8_t* weapon, int on);
GAME_FN(void, __thiscall, Weapon_StopEffect, 0x004D9540, uint8_t* weapon, int32_t id);
GAME_FN(void, __thiscall, Weapon_Fn4D9580, 0x004D9580, uint8_t* weapon, int32_t id);
inline void* const animManager_20C4910 = reinterpret_cast<void*>(0x020C4910);
GAME_FN(uint8_t, __thiscall, BaseCollisionTrack_Start, 0x005F95A0, uint8_t* track, uint8_t* owner);
GAME_FN(uint8_t, __cdecl, Ped_FindBonePosition, 0x0065E670, float* out, uintptr_t bone, uint8_t* ped);
GAME_FN(int*, __thiscall, Vehicle_GetState_4CAF30, 0x004CAF30, uint8_t* vehicle, int* out);
GAME_FN(uint8_t, __thiscall, Entity_IsTouchingMoving, 0x00465D10, uint8_t* entity, const float* centre, float radius);
GAME_FN(uint8_t, __thiscall, Ped_GetBoneSphere_474630, 0x00474630, uint8_t* ped, int bone, float* centre, float* radius);
GAME_FN(float, __cdecl, DistanceToLine_574A00, 0x00574A00, const float* point, const float* line, uintptr_t);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const float* start, const float* end, ColPoint* colPoint,
        re::Ptr32<uint8_t>* entity, int flags, int, int);
GAME_FN(uint8_t, __thiscall, CollisionTrack_Hit_5F8A50, 0x005F8A50, uint8_t* self, uint8_t* ped, const float* point,
        int32_t);
GAME_FN(int32_t, __thiscall, Inventory_FindItem, 0x0045CF30, uint8_t* inventory, int32_t weapon);
GAME_FN(uint8_t*, __thiscall, Inventory_GetItemInSlot, 0x0045CC90, uint8_t* inventory, int32_t slot);
GAME_FN(void, __thiscall, Weapon_SetScale_4D9970, 0x004D9970, uint8_t* weapon, const float* scale);
GAME_FN(uint8_t, __thiscall, Ped_Fn474BF0, 0x00474BF0, uint8_t* ped);
GAME_FN(void, __thiscall, Weapon_SpawnDamageEffect_4D9DC0, 0x004D9DC0, uint8_t* weapon);
GAME_FN(uint8_t, __thiscall, Weapon_HasNextDamageLevel_4D96F0, 0x004D96F0, uint8_t* weapon);
GAME_FN(void, __thiscall, Weapon_NextDamageLevel_4DA990, 0x004DA990, uint8_t* weapon);
GAME_FN(void, __thiscall, WeaponFireTrack_CountShotFn, 0x0065F630, uint8_t* self);
GAME_FN(void, __thiscall, Inventory_RemoveAmmo, 0x0045D0C0, uint8_t* inventory, int32_t weapon, int32_t ammo);
GAME_FN(int32_t, __thiscall, Inventory_GetAmmo, 0x0045CF40, uint8_t* inventory, int32_t weapon);
GAME_FN(void, __thiscall, Owner_SetNode, 0x005F3830, uint8_t* owner, uint8_t* node);
GAME_FN(void, __thiscall, Weapon_Reset_4D9410, 0x004D9410, uint8_t* weapon);
GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* self, int count, const void* names);
GAME_FN(uint32_t, __cdecl, Butes_Find, 0x0072AEA0, const void* path, const void* root);
GAME_FN(void, __thiscall, WeaponInfo_Load_7282A0, 0x007282A0, uint8_t* info);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(V3*, __thiscall, Targeting_GetTargetPos, 0x004D6000, void* targeting, V3* out, void* entity, int flag);
GAME_FN(V3*, __thiscall, WeaponFireTrack_SpreadFn, 0x0065ECA0, uint8_t* self, V3* out, const float* target);
GAME_FN(V3*, __thiscall, WeaponFireTrack_TargetPointFn, 0x00662460, uint8_t* self, V3* out, uint8_t* entity);
GAME_FN(V3*, __thiscall, WeaponFireTrack_TargetPoint2Fn, 0x006624E0, uint8_t* self, V3* out, uint8_t* entity);
GAME_FN(float*, __cdecl, TwoDEffect_GetMatrix_8B6120, 0x008B6120, void* effect);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, uint8_t* entity, int node, float* transform, int flag);
GAME_FN(float*, __thiscall, Ped_GetTargetPoint_496EC0, 0x00496EC0, uint8_t* ped, float* out);
GAME_FN(V3*, __cdecl, VectorCross, 0x00414730, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __thiscall, WeaponFireTrack_LeadTargetFn, 0x0065EE10, uint8_t* self, V3* out, float x, float y, float z,
        float* target, uint8_t* entity);
GAME_FN(uint8_t, __cdecl, Fn683A30, 0x00683A30);
GAME_FN(void, __cdecl, Fn683B50, 0x00683B50, const float* from, float* target, uint8_t* entity);
GAME_FN(float, __cdecl, Pad_GetValue_738840, 0x00738840, int control, int);
GAME_FN(int32_t, __thiscall, Targeting_GetMode_856850, 0x00856850, void* targeting);
GAME_FN(uint8_t*, __thiscall, TargetingSystem_GetTarget, 0x004D5AD0, void* targeting);
GAME_FN(uint8_t*, __thiscall, TargetingSystem_GetLookTarget, 0x004D7780, void* targeting);
GAME_FN(uint8_t, __thiscall, NearList_HasAimPoint_48BEE0, 0x0048BEE0, uint8_t* list);
GAME_FN(V3*, __thiscall, NearList_GetAimPoint_48BF00, 0x0048BF00, uint8_t* list, V3* out);
GAME_FN(V3*, __thiscall, WeaponFireTrack_AimPointFn, 0x0065EBB0, uint8_t* self, V3* out, float x, float y, float z,
        float k);
GAME_FN(float*, __thiscall, WeaponFireTrack_CameraTargetFn, 0x006622E0, uint8_t* self, float* out, float x, float y,
        float z, float distance);
GAME_FN(V3*, __thiscall, WeaponFireTrack_TargetPointForFn, 0x006626E0, uint8_t* self, V3* out, uint8_t* entity);
GAME_FN(V3*, __thiscall, WeaponFireTrack_MuzzleFn, 0x00662750, uint8_t* self, V3* out, uint8_t* weapon);
GAME_FN(V3*, __thiscall, WeaponFireTrack_TargetBonePointFn, 0x00662800, uint8_t* self, V3* out, uint8_t* entity,
        int bone);
GAME_FN(float*, __thiscall, WeaponFireTrack_AdjustTargetFn, 0x006628C0, uint8_t* self, float* out, float x, float y,
        float z, float* target, uint8_t* entity);
GAME_FN(void, __cdecl, WeaponFire_ResetGlobals_65D2C0, 0x0065D2C0);
GAME_FN(float*, __thiscall, WeaponFireTrack_FirePosition_65E9E0, 0x0065E9E0, uint8_t* self, float* out);
GAME_FN(float, __cdecl, Vector_Heading, 0x00433310, const V3* v);
GAME_FN(float, __cdecl, LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(V3*, __cdecl, VecFromHeading, 0x00432DF0, V3* out, float heading);
GAME_FN(void, __thiscall, WeaponFireTrack_ResolveInfoFn, 0x00662200, uint8_t* self);
GAME_FN(int32_t, __thiscall, Weapon_Fn4D95E0, 0x004D95E0, uint8_t* weapon);
GAME_FN(uint8_t*, __cdecl, AddProjectile_x64, 0x004D4D90, uint8_t* creator, int32_t weapon, re::Float3, re::Float3, uint8_t* info, float spread, float power, uintptr_t r20, uintptr_t sinceLast, int32_t hold, re::Float3, re::Float3, uint8_t* target, int flag);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* AddProjectile(uint8_t* a0, int32_t a1, float a2, float a3, float a4, float a5, float a6, float a7, uint8_t* a8, float a9, float a10, int32_t a11, uintptr_t a12, int32_t a13, uint32_t a14, uint32_t a15, uint32_t a16, uint32_t a17, uint32_t a18, uint32_t a19, uint8_t* a20, int a21) {
    return AddProjectile_x64(a0, a1, re::Float3{a2, a3, a4}, re::Float3{a5, a6, a7}, a8, a9, a10, a11, a12, a13, re::Float3{re::FloatOf(uint32_t(a14)), re::FloatOf(uint32_t(a15)), re::FloatOf(uint32_t(a16))}, re::Float3{re::FloatOf(uint32_t(a17)), re::FloatOf(uint32_t(a18)), re::FloatOf(uint32_t(a19))}, a20, a21);
}
GAME_FN(uint8_t*, __cdecl, GetEntityFromNode_44A3E0, 0x0044A3E0, int type, int index);
GAME_FN(void, __cdecl, ClearScanCodes_45D840, 0x0045D840);
GAME_FN(uint8_t*, __cdecl, GetColModel_51AF50, 0x0051AF50, uint8_t* entity);
GAME_FN(uint8_t, __cdecl, ProcessColModels_56C8C0, 0x0056C8C0, const SweepMatrix* matrixA, const SweepMatrix* matrixA2,
        uint8_t* colA, const float* matrixB, const float* matrixB2, uint8_t* colB, ColPoint* points, int, int, int, int,
        int, int lists, uintptr_t, int);
GAME_FN(void, __thiscall, Automobile_SetDamage_4A8410, 0x004A8410, uint8_t* automobile, int piece, float impulse,
        uint8_t* damager, const V3* normal);
GAME_FN(void, __thiscall, Entity_SetPosition_41C4E0, 0x0041C4E0, uint8_t* entity, float x, float y, float z);
GAME_FN(float, __thiscall, Entity_GetBoundRadius_465E10, 0x00465E10, uint8_t* entity);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce_46A950, 0x0046A950, uint8_t* physical, float fx, float fy, float fz,
        float px, float py, float pz);
GAME_FN(void, __thiscall, Physical_AddToMovingList_4697D0, 0x004697D0, uint8_t* entity);
GAME_FN(uint8_t*, __cdecl, GetProjectileInstance_4D34B0, 0x004D34B0, int32_t index);
GAME_FN(void*, __thiscall, EntityRef_Set_478550, 0x00478550, void* ref, uint8_t* entity);
GAME_FN(void, __thiscall, Events_Add_4A6670, 0x004A6670, void* events, int kind, uint8_t* ped, uint8_t* other,
        const float* pos, float a, float b);
GAME_FN(float, __thiscall, Vector_Magnitude_40DFB0, 0x0040DFB0, const V3* v);
GAME_FN(uint8_t*, __thiscall, Targeting_Fn4D5F60, 0x004D5F60, void* targeting);
GAME_FN(void, __thiscall, EffectProxy_Kill_668BA0, 0x00668BA0, uint8_t* proxy);
GAME_FN(void, __thiscall, Matrix_Construct_412770, 0x00412770, SweepMatrix* matrix);
GAME_FN(void, __thiscall, Box_Set_564830, 0x00564830, void* box, const V3* min, const V3* max);
GAME_FN(uint8_t*, __thiscall, Reference_Get_5F4F90, 0x005F4F90, uint8_t* ref, int32_t key);
GAME_FN(void, __thiscall, Vehicle_Knock_4CC4A0_x64, 0x004CC4A0, uint8_t* vehicle, uint8_t* ped, int, float, float, float, float, uintptr_t, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Vehicle_Knock_4CC4A0(uint8_t* a0, uint8_t* a1, int a2, float a3, V3 a4, int a5, int a6) {
    return Vehicle_Knock_4CC4A0_x64(a0, a1, a2, a3, a4.x, a4.y, a4.z, a5, a6);
}
GAME_FN(void, __thiscall, Ped_SetGrapplePartner_4744B0, 0x004744B0, uint8_t* ped, uint8_t* partner);
GAME_FN(void, __thiscall, Ctl_QueueTracks_5F3F80, 0x005F3F80, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_PlayNode_5F45F0, 0x005F45F0, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Stat_Add_4A74C0, 0x004A74C0, uint8_t* stat, float value);
GAME_FN(void, __thiscall, Stat_Add_4DAC30, 0x004DAC30, uint8_t* stat, float value);
GAME_FN(void, __thiscall, Ped_AddStun_47CD90, 0x0047CD90, uint8_t* ped, float time, int);
GAME_FN(void, __thiscall, Track_AgeHits_5F8AE0, 0x005F8AE0, uint8_t* track, float dt, float maxAge);
GAME_FN(int32_t, __thiscall, SoundStreams_GetId_595B80, 0x00595B80, void* sounds, const char* name);
GAME_FN(int32_t, __thiscall, SoundStreams_Play_59B8A0, 0x0059B8A0, void* sounds, int32_t sound, const float* pos,
        uint32_t, uintptr_t, int, int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(V3*, __cdecl, WeaponFire_LaunchVelocityFn, 0x0065F1B0, V3* out, float fx, float fy, float fz, float tx,
        float ty, float tz, float power, uint8_t* info, uint8_t* ped);
GAME_FN(V3*, __thiscall, WeaponFireTrack_ChooseTargetFn, 0x00662BA0, uint8_t* self, V3* out, float fx, float fy,
        float fz);
GAME_FN(float*, __thiscall, WeaponCollisionTrack_MiddleFn, 0x00661910, uint8_t* self, const float* p1,
        const float* p2, const float* p3, const float* p4, float* out);
GAME_FN(uint8_t, __thiscall, WeaponCollisionTrack_TestPedsFn, 0x00661B30, uint8_t* self, const float* centre,
        float radius, float* from, const float* towards1, const float* line1, int both, const float* towards2,
        const float* line2);
GAME_FN(void, __thiscall, WeaponCollisionTrack_CountHitFn, 0x00661E70, uint8_t* self);
GAME_FN(uint8_t, __thiscall, WeaponCollisionTrack_TestObjectListFn, 0x00664350, uint8_t* self, const float* centre,
        float radius, float* from, const SweepMatrix* to1, const SweepMatrix* from1, int both,
        const SweepMatrix* to2, const SweepMatrix* from2, uint32_t* list);
GAME_FN(uint8_t, __thiscall, WeaponCollisionTrack_TestVehicleListFn, 0x00664640, uint8_t* self, const float* centre,
        float radius, const SweepMatrix* to1, const SweepMatrix* from1, int both, const SweepMatrix* to2,
        const SweepMatrix* from2, uint32_t* list);
GAME_FN(uint8_t, __thiscall, WeaponCollisionTrack_TestSectorsFn, 0x00664D40, uint8_t* self, const float* centre,
        float radius, float* from, const SweepMatrix* to1, const SweepMatrix* from1, int both,
        const SweepMatrix* to2, const SweepMatrix* from2, uint32_t lists);
GAME_FN(void, __thiscall, WeaponImpulseTrack_ApplyFn, 0x00664C60, uint8_t* self, uint8_t* entity, float vx, float vy,
        float vz);
GAME_FN(uint8_t, __thiscall, WeaponImpulseTrack_StartFn, 0x00664FE0, uint8_t* self, uint8_t* owner);
GAME_FN(uint8_t, __thiscall, WeaponCollisionTrack_SweepFn, 0x006654F0, uint8_t* self, float* from, const float* tip1,
        int both, const float* tip2);
//@@DECL@@

inline float Sqrt(float x) {
    return crt::Sqrtf(x);
}
// a random number in -1..1 (from rand())
inline float RandomSigned() {
    float r = float(Rand()) / 32767.0f;
    r = r + r;
    return r - 1.0f;
}

// 0x65EBB0 (thiscall): a point ahead of a target along the ped's facing (k of its
// forward vector); level with the ped past 0.9; moved on by a lock-on target's velocity
V3* __fastcall WeaponFireTrack_AimPoint(uint8_t* self, re::Edx, V3* out, float tx, float ty, float tz, float k) {
    uint8_t* ped = At<uint8_t*>(self, 0x40);
    V3 forward;
    const V3* f = Placeable_GetForward(ped, &forward);
    V3 target = {tx, ty, tz};
    V3 v;
    v.y = f->y * k;
    v.z = f->z * k;
    v.x = k * f->x;
    VectorAdd(out, &target, &v);
    if (k > 0.9f) {
        out->z = Pos(At<uint8_t*>(self, 0x40))[2];
    }
    uint8_t* p = At<uint8_t*>(self, 0x40);
    uint8_t* locked = At<uint8_t*>(p, 0x1554);
    if (At<int32_t>(p, 0x1310) == 0xD && locked) {
        float scale = velocityScale_B31CB8;
        float a = At<float>(locked, 0x124) * scale;
        float b = At<float>(locked, 0x128) * scale;
        float c = scale * At<float>(locked, 0x12C);
        out->x = out->x + a;
        out->y = out->y + b;
        out->z = out->z + c;
    }
    return out;
}

// 0x65ECA0 (thiscall): a random aim error for a target: its flat distance / 20 by
// (100 - the ped's stat 10) / 50, the Butes' factor off screen, less while locked on;
// returns out
V3* __fastcall WeaponFireTrack_Spread(uint8_t* self, re::Edx, V3* out, const float* target) {
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8);
    V3 d;
    VectorSub(&d, target, Pos(ped));
    float flat = d.y * d.y + d.x * d.x;
    flat = std::fabs(flat);
    int32_t skill = Ped_GetStat(ped, 10);
    float distance = Sqrt(flat);
    float a = distance / 20.0f;
    float b = float(100 - skill) / 50.0f;
    float spread = a * b;
    if (!Entity_GetIsOnScreen(ped)) {
        spread = At<float>(weaponButes_20C88C8, 0x11C) * spread;
    }
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        float lock = Fn4D5E20(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1C8));
        spread = (1.0f - lock) * spread;
    }
    float first = RandomSigned();
    float second = RandomSigned();
    float third = RandomSigned();
    out->x = third * spread;
    out->y = second * spread;
    out->z = spread * first;
    return out;
}

// 0x65EE10 (thiscall): a moving target led: its velocity (a ped's, or its vehicle's, or a
// vehicle's) over the projectile's flight time from `from` (its speed at this power, an arc
// lengthening long shots) added to the target; the result also returned
V3* __fastcall WeaponFireTrack_LeadTarget(uint8_t* self, re::Edx, V3* out, float fx, float fy, float fz, float* target,
                                          uint8_t* entity) {
    V3 from = {fx, fy, fz};
    V3 d;
    VectorSub(&d, target, &from.x);
    d.z = 0.0f;
    float flat = d.y * d.y + d.x * d.x;
    flat = flat + 0.0f * 0.0f;
    flat = std::fabs(flat);
    float distance = Sqrt(flat);
    uint8_t* info = At<uint8_t*>(self, 0x48);
    float speed = 0.4f;
    if (info) {
        float slow = At<float>(info, 0x60);
        speed = slow + (At<float>(info, 0x64) - slow) * At<float>(self, 0x44);
    }
    V3 velocity = {0.0f, 0.0f, 0.0f};
    float time = distance / speed;
    time = time * At<float>(self, 0x28);
    if (info && info[0x124]) {
        float h = 0.5f * time;
        h = h + 12.5f;
        float excess = time - h;
        if (excess > 0.0f) {
            float e = excess * excess;
            e = e / At<float>(info, 0x128);
            e = std::fabs(e);
            e = Sqrt(e);
            time = e + h;
        }
    }
    uint8_t type = entity[0x108] & 7;
    if (type == 3) {
        uint8_t* vehicle = At<uint8_t*>(entity, 0x1554);
        if (vehicle) {
            float a = At<float>(vehicle, 0x124) * time;
            float b = At<float>(vehicle, 0x128) * time;
            float c = time * At<float>(vehicle, 0x12C);
            velocity.x = a + 0.0f;
            velocity.y = b + 0.0f;
            velocity.z = 0.0f + c;
        } else {
            float perSecond = 1.0f / timeStep;
            V3 move = {At<float>(entity, 0x1714), At<float>(entity, 0x1718), At<float>(entity, 0x171C)};
            V3 w = {move.x * perSecond, move.y * perSecond, perSecond * move.z};
            float a = w.x * time;
            float b = w.y * time;
            float c = time * w.z;
            velocity.x = a + 0.0f;
            velocity.y = b + 0.0f;
            velocity.z = 0.0f + c;
        }
    } else if (type == 2) {
        float a = At<float>(entity, 0x124) * time;
        float b = At<float>(entity, 0x128) * time;
        float c = time * At<float>(entity, 0x12C);
        velocity.x = a + 0.0f;
        velocity.y = b + 0.0f;
        velocity.z = 0.0f + c;
    }
    V3 n = d;
    Vector_Normalise(&n);
    float dot = n.x * velocity.x + n.y * velocity.y;
    dot = n.z * velocity.z + dot;
    float q = dot / time;
    float t2 = distance / (speed - q);
    float k = t2 / time;
    velocity.x = velocity.x * k;
    velocity.y = velocity.y * k;
    velocity.z = velocity.z * k;
    if (At<uint8_t*>(self, 0x48)) {
        float m = At<float>(At<uint8_t*>(self, 0x48), 0x84);
        velocity.x = m * velocity.x;
        velocity.y = velocity.y * m;
        velocity.z = m * velocity.z;
    }
    target[0] = target[0] + velocity.x;
    target[1] = target[1] + velocity.y;
    target[2] = target[2] + velocity.z;
    out->x = target[0];
    out->y = target[1];
    out->z = target[2];
    return out;
}

// 0x65F1B0 (cdecl): a projectile's launch velocity from one point to another: tried with
// flight times from the distance over its speed up (x1.06 each time, 20 tries) until it
// meets its info's limits on vertical speed (+0x68/+0x6C), rise (+0x70/+0x74) and
// elevation angle (+0x78) (-1: none); then its spread added
inline V3* LaunchVelocity(V3* out, float fx, float fy, float fz, float tx, float ty, float tz, float power,
                          uint8_t* info, uint8_t* ped, bool lockOn) {
    V3 from = {fx, fy, fz};
    V3 target = {tx, ty, tz};
    float g = info ? At<float>(info, 0x80) : 1.0f;
    float halfGravity = g * -0.008f;
    float speed = 0.4f;
    if (info) {
        float slow = At<float>(info, 0x60);
        speed = slow + (At<float>(info, 0x64) - slow) * power;
    }
    if (speed < 0.01f) {
        speed = 0.01f;
    }
    V3 d;
    VectorSub(&d, &from.x, &target.x);
    float flat = d.y * d.y + d.x * d.x;
    flat = flat + 0.0f * 0.0f;
    flat = std::fabs(flat);
    float distance = Sqrt(flat);
    float time = distance / speed;
    if (!(time > 0.01f)) {
        time = 0.01f;
    }
    V3 a = {0.0f, 0.0f, 0.0f};
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    float gx = 0.0f * 0.5f;
    V3 gravity = {gx, gx, 0.5f * halfGravity};
    int tries = 0;
    for (;;) {
        V3 gt = {gravity.x * time, gravity.y * time, gravity.z * time};
        V3 gtt = {gt.x * time, gt.y * time, time * gt.z};
        V3 aimed;
        VectorSub(&aimed, &target.x, &gtt.x);
        a = aimed;
        float inverse = 1.0f / time;
        V3 rise;
        VectorSub(&rise, &a.x, &from.x);
        out->x = rise.x * inverse;
        out->y = rise.y * inverse;
        out->z = inverse * rise.z;
        float zero = 0.0f * time;
        V3 w = {zero, zero, time * halfGravity};
        V3 r;
        V3 end;
        const V3* sum = VectorAdd(&end, out, &w);
        r = *sum;
        float down = -r.z;
        float horizontal = r.y * r.y + r.x * r.x;
        horizontal = std::fabs(horizontal);
        horizontal = Sqrt(horizontal);
        float slope = down / horizontal;
        float angle = crt::Atanf(slope);   // atan
        angle = angle * 180.0f;
        angle = angle / 3.1415927f;
        bool ok = true;
        if (!info) {
            return out;
        }
        if (!(-1.0f == At<float>(info, 0x68)) && At<float>(info, 0x68) > out->z) {
            ok = false;
        }
        if (!(-1.0f == At<float>(info, 0x6C)) && At<float>(info, 0x6C) < out->z) {
            ok = false;
        }
        if (!(-1.0f == At<float>(info, 0x70)) && At<float>(info, 0x70) > r.z) {
            ok = false;
        }
        if (!(-1.0f == At<float>(info, 0x74)) && At<float>(info, 0x74) < r.z) {
            ok = false;
        }
        bool retry;
        if (!(-1.0f == At<float>(info, 0x78)) && At<float>(info, 0x78) > angle) {
            retry = true;
        } else {
            retry = !ok;
        }
        if (!retry || tries == 0x13) {
            break;
        }
        tries++;
        float longer = time + 1.0f;
        time = longer * 1.06f;
        if (tries >= 0x14) {
            break;
        }
    }
    if (!(0.0f == At<float>(info, 0x120))) {
        float spread = At<float>(info, 0x120);
        float first = RandomSigned();
        float second = RandomSigned();
        float third = RandomSigned();
        if (lockOn && ped && At<int32_t>(ped, 0x1310) == 0xD) {
            float lock = Fn4D5E20(At<uint8_t*>(ped, 0x1C8));
            spread = (1.0f - lock) * spread;
        }
        float cx = third * spread;
        float cy = second * spread;
        float cz = spread * first;
        out->x = out->x + cx;
        out->y = out->y + cy;
        out->z = cz + out->z;
    }
    return out;
}
V3* __cdecl WeaponFire_LaunchVelocity(V3* out, float fx, float fy, float fz, float tx, float ty, float tz,
                                      float power, uint8_t* info, uint8_t* ped) {
    return LaunchVelocity(out, fx, fy, fz, tx, ty, tz, power, info, ped, true);
}

// 0x65FFA0 (cdecl): the same without a ped (no lock-on narrowing of the spread)
V3* __cdecl WeaponFire_LaunchVelocityNoPed(V3* out, float fx, float fy, float fz, float tx, float ty, float tz,
                                           float power, uint8_t* info) {
    return LaunchVelocity(out, fx, fy, fz, tx, ty, tz, power, info, nullptr, false);
}

// 0x65F630 (thiscall): the player's shots counted: a stat by the weapon (0x2C, 0x46, ...,
// 0xA7; the slingshot's by its ammo), then shots fired (0x6B)
void __fastcall WeaponFireTrack_CountShot(uint8_t* self, re::Edx) {
    int32_t weapon = forcedWeapon_B31C9C;
    if (weapon == -1) {
        weapon = At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), 0x12C);
    }
    int32_t stat = -1;
    if (weapon == ModelId(0x00BF4398) || weapon == ModelId(0x00BF4394)) {
        stat = 0x2C;
    } else if (weapon == ModelId(0x00BF4348)) {
        stat = 0x46;
    } else if (weapon == ModelId(0x00BF4384)) {
        stat = 0x48;
    } else if (weapon == ModelId(0x00BF442C)) {
        stat = 0x4A;
    } else if (weapon == ModelId(0x00BF43F4)) {
        stat = 0x4C;
    } else if (weapon == ModelId(0x00BF43E8)) {
        stat = 0x4D;
    } else if (weapon == ModelId(0x00BF4364)) {
        stat = 0x52;
    } else if (weapon == ModelId(0x00BF4448)) {
        stat = 0x6E;
    } else if (weapon == ModelId(0x00BF4358)) {
        stat = 0x71;
    } else if (weapon == ModelId(0x00BF4434)) {
        stat = 0x74;
    } else if (weapon == ModelId(0x00BF435C)) {
        stat = 0x77;
    } else if (weapon == ModelId(0x00BF4108)) {
        int32_t model = At<int16_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), 0x10E);
        stat = model == ModelId(0x00BF4360) ? 0xED : 0x79;
    } else if (weapon == ModelId(0x00BF4100)) {
        stat = 0x7B;
    } else if (weapon == ModelId(0x00BF440C)) {
        stat = 0x7D;
    } else if (weapon == ModelId(0x00BF4428) || weapon == ModelId(0x00BF4344)) {
        stat = 0x7F;
    } else if (weapon == ModelId(0x00BF4414)) {
        stat = 0xA7;
    }
    if (stat != -1) {
        Stats_Increment(stat, 1);
    }
    Stats_Increment(0x6B, 1);
}

// the weapon's spray bone (its skeleton's bone by the hash at 0x1B65300; -1: none)
inline int32_t SprayBone(uint8_t* weapon) {
    uint8_t* anim = At<uint8_t*>(weapon, 0x114);
    int32_t bone = 0;
    if (!anim || !At<void*>(anim, 0x94)) {
        return -1;
    }
    if (!Skeleton_FindBone(At<void*>(anim, 0x94), sprayBoneHash_1B65300, &bone)) {
        return -1;
    }
    return bone;
}

// 0x65F7A0 (thiscall): ExtinguisherFireTrack::Update: fires put out along the spray (from
// the nozzle bone's position along its forward axis, else ahead of the ped)
uint8_t __fastcall ExtinguisherFireTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x24), 8);
    int32_t bone = SprayBone(At<uint8_t*>(ped, 0x1D0));
    float position[3];
    uint32_t direction[3];
    if (bone != -1) {
        AnimManager_GetBonePosition(animManager_20C4910, At<void*>(At<uint8_t*>(ped, 0x1D0), 0x114), bone, position,
                                    0);
        float transform[13];
        transform[12] = 1.0f;
        AnimManager_GetBoneTransform(animManager_20C4910,
                                     At<void*>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), 0x1D0), 0x114),
                                     bone, transform, 0);
        float forward[3] = {transform[1], transform[4], transform[7]};
        direction[0] = reinterpret_cast<uint32_t&>(forward[0]);
        direction[1] = reinterpret_cast<uint32_t&>(forward[1]);
        direction[2] = reinterpret_cast<uint32_t&>(forward[2]);
    } else {
        float up[3] = {0.0f, 0.0f, 1.0f};
        Matrix_TransformPoint_41A9A0(position, Entity_GetMatrix_46DD20(ped), up);
        V3 forward;
        const V3* f = Placeable_GetForward(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), &forward);
        direction[0] = reinterpret_cast<const uint32_t&>(f->x);
        direction[1] = reinterpret_cast<const uint32_t&>(f->y);
        direction[2] = reinterpret_cast<const uint32_t&>(f->z);
    }
    FireManager_Extinguish(fireManager_BD0CF8, position[0], position[1], position[2],
                           reinterpret_cast<float&>(direction[0]), reinterpret_cast<float&>(direction[1]),
                           reinterpret_cast<float&>(direction[2]), At<float>(self, 0x18), At<float>(self, 0x1C));
    return 1;
}

// 0x65F900 (thiscall): PoisonSprayFireTrack::Update: the nozzle's position and transform
// looked up (and nothing done with them)
uint8_t __fastcall PoisonSprayFireTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x24), 8);
    int32_t bone = SprayBone(At<uint8_t*>(ped, 0x1D0));
    if (bone == -1) {
        Entity_GetMatrix_46DD20(ped);
        return 1;
    }
    float position[3];
    AnimManager_GetBonePosition(animManager_20C4910, At<void*>(At<uint8_t*>(ped, 0x1D0), 0x114), bone, position, 0);
    float transform[13];
    transform[12] = 1.0f;
    AnimManager_GetBoneTransform(animManager_20C4910,
                                 At<void*>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), 0x1D0), 0x114),
                                 bone, transform, 0);
    return 1;
}

// WeaponFXTrack: +0x18 its effect stopped at the end, +0x19 ended another way, +0x1A the
// weapon reset first, +0x1B/+0x1C flags, +0x20 the effect's name, +0x24 the bone's hash,
// +0x28 the owner, +0x2C the weapon's effect (-1: none)

// 0x65F9D0 (thiscall): WeaponFXTrack::Start: the effect started on the weapon's bone
uint8_t __fastcall WeaponFXTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    const char* name = At<const char*>(self, 0x20);
    At<uint8_t*>(self, 0x28) = owner;
    At<int32_t>(self, 0x2C) = -1;
    if (!At<uint32_t>(self, 0x24) || !name) {
        return 0;
    }
    uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0);
    if (self[0x1A]) {
        Weapon_Fn4DA490(weapon);
    }
    void* skeleton = At<void*>(At<uint8_t*>(weapon, 0x114), 0x94);
    int32_t bone = 0;
    if (!skeleton || !Skeleton_FindBone(skeleton, At<uint32_t>(self, 0x24), &bone)) {
        return 0;
    }
    if (bone == -1) {
        return 0;
    }
    int effect = EffectLibrary_Find(effectLibrary_B6F1E8, name);
    uint8_t flag = At<int32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1310) == 0xD ? self[0x1B] : 0;
    At<int32_t>(self, 0x2C) = Weapon_AddEffect(weapon, effect, bone, flag, self[0x1C]);
    if (At<int32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1310) == 0xD) {
        Weapon_Fn4D9650(weapon, gameMode == 2 ? 1 : 0);
        return 0;
    }
    Weapon_Fn4D9650(weapon, 0);
    return 0;
}

// 0x65FAF0 (thiscall): WeaponFXTrack::End: its effect stopped (or ended)
uint8_t __fastcall WeaponFXTrack_End(uint8_t* self, re::Edx) {
    if (self[0x18] && At<int32_t>(self, 0x2C) != -1) {
        Weapon_StopEffect(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1D0), At<int32_t>(self, 0x2C));
        return 0;
    }
    if (self[0x19] && At<int32_t>(self, 0x2C) != -1) {
        Weapon_Fn4D9580(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1D0), At<int32_t>(self, 0x2C));
    }
    return 0;
}

// WeaponCollisionTrack: +0x5C its reach, +0x200/+0x220 two tracked points (bones +0x210,
// +0x214), +0x20C the owner, +0x22C

// 0x6617C0 (thiscall): WeaponCollisionTrack::Start: the ped set swinging (until +0x18
// seconds from now), its two points taken from its bones; the player's swings counted
uint8_t __fastcall WeaponCollisionTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20C) = owner;
    self[0x22C] = 0;
    BaseCollisionTrack_Start(self, owner);
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8);
    At<int32_t>(ped, 0x71C) = 1;
    ped[0x4E5] = 1;
    At<int32_t>(ped, 0x4E8) = 1;
    float ms = At<float>(self, 0x18) * 1000.0f;
    int32_t now = int32_t(timeInMs);
    double clock = double(now);
    if (now < 0) {
        clock = double(float(clock + 4294967296.0));
    }
    At<int32_t>(ped, 0xE4) = 1;
    float until = float(double(ms) + clock);
    At<uint32_t>(ped, 0x720) = uint32_t(int64_t(until));   // (truncated to 64 bits, its low half)
    const float* pos = Pos(At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8));
    At<float>(self, 0x200) = pos[0];
    At<float>(self, 0x204) = pos[1];
    At<float>(self, 0x208) = pos[2];
    At<float>(self, 0x220) = pos[0];
    At<float>(self, 0x224) = pos[1];
    At<float>(self, 0x228) = pos[2];
    Ped_FindBonePosition(&At<float>(self, 0x200), At<uint32_t>(self, 0x210), At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8));
    Ped_FindBonePosition(&At<float>(self, 0x220), At<uint32_t>(self, 0x214), At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8));
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        Stats_Increment(0x69, 1);
    }
    return 1;
}

// 0x6618F0 (thiscall): WeaponCollisionTrack::End: the ped no longer swinging
uint8_t __fastcall WeaponCollisionTrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8), 0x71C) = 0;
    return 1;
}

// 0x661910 (thiscall, the track unused): the middle of four points (between the middles
// of 1-4 and 2-3; its fourth word is left as the stack has it in the game: 0 here)
float* __fastcall WeaponCollisionTrack_Middle(uint8_t*, re::Edx, const float* p1, const float* p2, const float* p3,
                                             const float* p4, float* out) {
    V3 d;
    VectorSub(&d, p1, p4);
    V3 half = {d.x * 0.5f, d.y * 0.5f, 0.5f * d.z};
    V3 middle14;
    VectorAdd(&middle14, reinterpret_cast<const V3*>(p4), &half);
    VectorSub(&d, p2, p3);
    half = {d.x * 0.5f, d.y * 0.5f, 0.5f * d.z};
    V3 middle23;
    VectorAdd(&middle23, reinterpret_cast<const V3*>(p3), &half);
    VectorSub(&d, &middle23.x, &middle14.x);
    half = {0.5f * d.x, d.y * 0.5f, d.z * 0.5f};
    V3 centre;
    VectorAdd(&centre, &middle14, &half);
    out[0] = centre.x;
    out[1] = centre.y;
    out[2] = centre.z;
    out[3] = 0.0f;
    return out;
}

// 0x661B30 (thiscall): the peds near the owner (its +0x3BC list) tested against the swing:
// a ped touching the sphere (a1, a2) and not in a vehicle (unless the vehicle's state is 6
// or 7) has its 6 bone spheres tested against the line a5 (hit point towards a4) and, with
// a6, the line a8 (towards a7); a hit with a clear line from a3 (at the hit's height) to the
// ped is passed to the track's hit (0x5F8A50). True when any hit was taken.
uint8_t __fastcall WeaponCollisionTrack_TestPeds(uint8_t* self, re::Edx, const float* centre, float radius, float* from,
                                                 const float* towards1, const float* line1, int both,
                                                 const float* towards2, const float* line2) {
    float reach = At<float>(self, 0x5C);
    uint8_t* list = At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8) + 0x3BC;
    uint8_t hit = 0;
    for (int i = 0; i < At<int32_t>(list, 4); i++) {
        uint8_t* ped = At<uint8_t*>(list, 8 + i * 4);
        if (!ped) {
            continue;
        }
        uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
        if (vehicle && At<uint32_t>(vehicle, 0x46C) == 0) {
            int state;
            if (*Vehicle_GetState_4CAF30(vehicle, &state) != 7) {
                int state2;
                if (*Vehicle_GetState_4CAF30(vehicle, &state2) != 6) {
                    continue;
                }
            }
        }
        if (!Entity_IsTouchingMoving(ped, centre, radius)) {
            continue;
        }
        V3 dir = {0.0f, 0.0f, 0.0f};
        float sphere[3] = {0.0f, 0.0f, 0.0f};
        float sphereRadius = 0.0f;
        int kind = 0;
        for (int bone = 0; bone < 6 && !kind; bone++) {
            if (!Ped_GetBoneSphere_474630(ped, bone, sphere, &sphereRadius)) {
                continue;
            }
            float distance = DistanceToLine_574A00(sphere, line1, 0);
            if (sphereRadius + reach > distance) {
                VectorSub(&dir, towards1, sphere);
                Vector_Normalise(&dir);
                kind = 1;
            } else if (re::Flag(both)) {
                distance = DistanceToLine_574A00(sphere, line2, 0);
                if (sphereRadius + reach > distance) {
                    VectorSub(&dir, towards2, sphere);
                    Vector_Normalise(&dir);
                    kind = 2;
                }
            }
        }
        if (!kind) {
            continue;
        }
        dir.x = dir.x * sphereRadius;
        dir.y = dir.y * sphereRadius;
        dir.z = sphereRadius * dir.z;
        dir.x = sphere[0] + dir.x;
        dir.y = sphere[1] + dir.y;
        dir.z = sphere[2] + dir.z;
        const float* at = Pos(ped);
        float target[3] = {at[0], at[1], at[2]};
        target[2] = kind == 1 ? towards1[2] : towards2[2];
        from[2] = target[2];
        ColPoint colPoint;
        colPoint.pos = {0.0f, 0.0f, 0.0f};
        colPoint.normal = {0.0f, 0.0f, 0.0f};
        colPoint.depth = 3.4028235e38f;
        colPoint.entity = nullptr;
        re::Ptr32<uint8_t> entity = nullptr;
        ignoredEntity_C1AE70 = ped;
        uint8_t blocked = ProcessLineOfSight(from, target, &colPoint, &entity, 0x49, 0, 0);
        ignoredEntity_C1AE70 = nullptr;
        if (!blocked && CollisionTrack_Hit_5F8A50(self, ped, &dir.x, -1)) {
            hit = 1;
        }
        ReleaseColPoint(colPoint);
    }
    return hit;
}

// 0x661E70 (thiscall): the swing's first hit counted on the weapon (+0x268)
void __fastcall WeaponCollisionTrack_CountHit(uint8_t* self, re::Edx) {
    if (self[0x22C]) {
        return;
    }
    self[0x22C] = 1;
    At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8), 0x1D0), 0x268) += 1;
}

// 0x661EA0 (thiscall): SetWeaponFlagsTrack::Start: flag +0x1C set to +0x18 on the inventory
// item and the weapon object
uint8_t __fastcall SetWeaponFlagsTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    int32_t weapon = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E);
    if (weapon == -1) {
        return 1;
    }
    int32_t slot = Inventory_FindItem(At<uint8_t*>(ped, 0x1C4), weapon);
    if (slot != -1) {
        uint8_t* item = Inventory_GetItemInSlot(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x20), 8), 0x1C4), slot);
        if (item) {
            At<uint32_t>(item, 4 + At<int32_t>(self, 0x1C) * 4) = self[0x18];
        }
    }
    At<uint32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x20), 8), 0x1D0), 0x26C + At<int32_t>(self, 0x1C) * 4) =
        self[0x18];
    return 1;
}

// 0x661F20 (thiscall): WeaponScaleTrack::Start: the weapon scaled from +0x18 towards +0x24
// over the track's time (the rate kept at +0x30)
uint8_t __fastcall WeaponScaleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint32_t>(self, 0x3C) = 0;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    if (!ped || !At<uint8_t*>(ped, 0x1D0)) {
        return 0;
    }
    uint8_t* weapon = At<uint8_t*>(ped, 0x1D0);
    float time = acttree::TrackEnd(self) - acttree::TrackStart(self);
    At<uint8_t*>(self, 0x3C) = weapon;
    float dx = At<float>(self, 0x24) - At<float>(self, 0x18);
    At<float>(self, 0x30) = dx / time;
    float dy = At<float>(self, 0x28) - At<float>(self, 0x1C);
    At<float>(self, 0x34) = dy / time;
    float dz = At<float>(self, 0x2C) - At<float>(self, 0x20);
    At<float>(self, 0x38) = dz / time;
    float scale[3] = {At<float>(self, 0x18), At<float>(self, 0x1C), At<float>(self, 0x20)};
    Weapon_SetScale_4D9970(weapon, scale);
    return 0;
}

// 0x661FA0 (thiscall, the track unused): WeaponSpawnDamageEffectTrack::Start
uint8_t __fastcall WeaponSpawnDamageEffectTrack_Start(uint8_t*, re::Edx, uint8_t* owner) {
    if (Ped_Fn474BF0(At<uint8_t*>(owner, 8))) {
        Weapon_SpawnDamageEffect_4D9DC0(At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0));
    }
    return 1;
}

// 0x661FD0 (thiscall, the track unused): WeaponSetNextDamageLevelTrack::Start: the weapon
// on to its next damage model (kept as the ped's picked-up model), else its hits reset
uint8_t __fastcall WeaponSetNextDamageLevelTrack_Start(uint8_t*, re::Edx, uint8_t* owner) {
    if (!Ped_Fn474BF0(At<uint8_t*>(owner, 8))) {
        return 1;
    }
    uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0);
    if (!Weapon_HasNextDamageLevel_4D96F0(weapon)) {
        At<int32_t>(weapon, 0x268) = 0;
        return 1;
    }
    Weapon_NextDamageLevel_4DA990(weapon);
    At<int32_t>(At<uint8_t*>(owner, 8), 0x1574) = At<int16_t>(weapon, 0x10E);
    At<int32_t>(At<uint8_t*>(owner, 8), 0x1578) = At<int32_t>(weapon, 0x268);
    return 1;
}

// 0x662040 (thiscall): WeaponPlayerFireAutoTargetTrack::Start: the player's auto-target flag
uint8_t __fastcall WeaponPlayerFireAutoTargetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        ped[0x1EC0] = self[0x18];
    }
    return 1;
}

// the weapon being fired (a forced one, else the ped's weapon's ammo weapon)
inline int32_t FiredWeapon(uint8_t* self) {
    int32_t weapon = forcedWeapon_B31C9C;
    if (weapon == -1) {
        weapon = At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), 0x12C);
    }
    return weapon;
}

// 0x662070 (thiscall): WeaponFireTrack::Start: the shot fired (its power from the weapon's
// charge by +0x34, 0..1; vf5 does the firing), counted for the player, a round used (unless
// infinite); out of ammo the weapon dropped (+0x32) and its action node stopped (+0x30)
uint8_t __fastcall WeaponFireTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x3C) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    int32_t model = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E);
    if (model == -1) {
        WeaponFire_ResetGlobals_65D2C0();   // (called with the ped in ecx, unused)
        return 0;
    }
    At<uint8_t*>(self, 0x40) = ped;
    float power = At<float>(At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0), 0x128);
    if (0.0f < At<float>(self, 0x34)) {
        power = power * At<float>(self, 0x34);
    }
    float clamped = power;
    if (power < 0.0f) {
        clamped = 0.0f;
    } else if (1.0f < power) {
        clamped = 1.0f;
    }
    At<float>(self, 0x44) = clamped;
    vc::Call<void>(self, 0x14);
    if (At<uint8_t*>(self, 0x40) == playerPed) {
        WeaponFireTrack_CountShotFn(self);
    }
    At<uint32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x1D0), 0x140) = 0;
    uint8_t* shooter = At<uint8_t*>(self, 0x40);
    if (At<uint32_t>(shooter, 0x1374) == 0) {
        Inventory_RemoveAmmo(At<uint8_t*>(shooter, 0x1C4), FiredWeapon(self), 1);
    }
    if (self[0x32] && model == FiredWeapon(self)) {
        if (Inventory_GetAmmo(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x1C4), FiredWeapon(self)) == 0) {
            uint8_t* owners = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x2E0);
            uint8_t* actions = owners + 0x300;
            if (self[0x30] && actions[0x3C]) {
                Owner_SetNode(actions, At<uint8_t*>(owners, 0x4B0));
                actions[0x3C] = 0;
            }
            Weapon_Reset_4D9410(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0));
        }
    }
    return 0;
}

// the Butes by the path (0x1B6531C, 0x1B65394, the name key at `key`) under 0x1B65398
inline uint32_t FireButes(const void* key) {
    uint32_t names[3];
    Name_Copy(&names[0], reinterpret_cast<const void*>(0x01B6531C));
    Name_Copy(&names[1], reinterpret_cast<const void*>(0x01B65394));
    Name_Copy(&names[2], key);
    uint8_t path[8];
    Path_Build(path, 3, names);
    uint32_t butes = Butes_Find(path, reinterpret_cast<const void*>(0x01B65398));
    NameKey_Destruct(&names[2]);
    NameKey_Destruct(&names[1]);
    NameKey_Destruct(&names[0]);
    return butes;
}

// 0x662200 (thiscall): the projectile's info (+0x48): the Butes named by +0x38, else the
// weapon info's own (+0x80)
void __fastcall WeaponFireTrack_ResolveInfo(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x48) = 0;
    if (At<uint32_t>(self, 0x38)) {
        At<uint32_t>(self, 0x48) = FireButes(self + 0x38);
        return;
    }
    int32_t weapon = forcedWeapon_B31C9C;
    if (weapon == -1) {
        weapon = At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), 0x12C);
    }
    uint8_t* info = At<uint8_t*>(modelInfoPtrs[weapon], 0x18);
    if (!info) {
        return;
    }
    WeaponInfo_Load_7282A0(info);
    if (At<uint32_t>(info, 0x80)) {
        WeaponInfo_Load_7282A0(info);
        At<uint32_t>(self, 0x48) = At<uint32_t>(info, 0x80);
    }
}

// 0x6622E0 (thiscall): the point `distance` along the camera's view (its column 2), or
// what's in the way before it (the ped ignored)
float* __fastcall WeaponFireTrack_CameraTarget(uint8_t* self, re::Edx, float* out, float, float, float,
                                               float distance) {
    const uint8_t* camera = GetLocalTransform(GetCameraNode(niCamera));
    V3 from = {At<float>(const_cast<uint8_t*>(camera), 0x24), At<float>(const_cast<uint8_t*>(camera), 0x28),
               At<float>(const_cast<uint8_t*>(camera), 0x2C)};
    uint8_t* view = GetLocalTransform(GetCameraNode(niCamera));
    float dir[3] = {At<float>(view, 8), At<float>(view, 0x14), At<float>(view, 0x20)};
    V3 step = {dir[0] * distance, dir[1] * distance, distance * dir[2]};
    VectorAdd(reinterpret_cast<V3*>(out), &from, &step);
    ColPoint colPoint;
    colPoint.pos = {0.0f, 0.0f, 0.0f};
    colPoint.normal = {0.0f, 0.0f, 0.0f};
    colPoint.depth = 3.4028235e38f;
    colPoint.entity = nullptr;
    re::Ptr32<uint8_t> entity = nullptr;
    ignoredEntity_C1AE70 = At<uint8_t*>(self, 0x40);
    if (ProcessLineOfSight(&from.x, out, &colPoint, &entity, 0x10F, 1, 1)) {
        out[0] = colPoint.pos.x;
        out[1] = colPoint.pos.y;
        out[2] = colPoint.pos.z;
    }
    ignoredEntity_C1AE70 = nullptr;
    ReleaseColPoint(colPoint);
    return out;
}

// the aim's "fully charged" flag: an info with +0x144 and power over 0.99
inline uint8_t FullyCharged(uint8_t* self) {
    uint8_t* info = At<uint8_t*>(self, 0x48);
    return info && info[0x144] && At<float>(self, 0x44) > 0.99f ? 1 : 0;
}

// 0x662460 (thiscall): the targeting's aim point for an entity, with the aim error
V3* __fastcall WeaponFireTrack_TargetPoint(uint8_t* self, re::Edx, V3* out, uint8_t* entity) {
    uint8_t charged = FullyCharged(self);
    Targeting_GetTargetPos(At<void*>(At<uint8_t*>(self, 0x40), 0x1C8), out, entity, charged);
    V3 error;
    const V3* e = WeaponFireTrack_SpreadFn(self, &error, &out->x);
    out->x = out->x + e->x;
    out->y = e->y + out->y;
    out->z = e->z + out->z;
    return out;
}

// 0x6624E0 (thiscall): a target's aim point: for the special weapon (0xBF4370) its position
// raised 0.8 (and 0.5 more fully charged); else the targeting's, raised by its height times
// 0x1B65C38, moved along the ped's facing (0x1B65C34) and sideways (0x1B65C30), with the aim
// error
V3* __fastcall WeaponFireTrack_TargetPoint2(uint8_t* self, re::Edx, V3* out, uint8_t* entity) {
    uint8_t charged = FullyCharged(self);
    out->x = zeroVector3[0];
    out->y = zeroVector3[1];
    out->z = zeroVector3[2];
    int32_t weapon = forcedWeapon_B31C9C;
    if (weapon == -1) {
        weapon = At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), 0x12C);
    }
    if (weapon == specialWeapon_BF4370) {
        const float* at = Pos(entity);
        out->x = at[0];
        out->y = at[1];
        out->z = at[2];
        float z = out->z + 0.8f;
        out->z = z;
        if (charged) {
            out->z = z + 0.5f;
        }
        return out;
    }
    V3 aim;
    const V3* p = Targeting_GetTargetPos(At<void*>(At<uint8_t*>(self, 0x40), 0x1C8), &aim, entity, charged);
    *out = *p;
    float height = vc::Call<float>(entity, 0x6C);
    out->z = height * aimHeight_1B65C38 + out->z;
    V3 forward;
    Placeable_GetForward(At<uint8_t*>(self, 0x40), &forward);
    float a = aimForward_1B65C34;
    V3 along = {a * forward.x, a * forward.y, a * forward.z};
    out->x = out->x + along.x;
    out->y = out->y + along.y;
    out->z = out->z + along.z;
    float b = aimSide_1B65C30;
    V3 side = {forward.y * b, -forward.x * b, b * 0.0f};
    out->x = out->x + side.x;
    out->y = out->y + side.y;
    out->z = out->z + side.z;
    V3 error;
    const V3* e = WeaponFireTrack_SpreadFn(self, &error, &out->x);
    out->x = e->x + out->x;
    out->y = e->y + out->y;
    out->z = e->z + out->z;
    return out;
}

// 0x6626E0 (thiscall): an entity's aim point (the first way for one in a vehicle whose
// +0x46C is 1)
V3* __fastcall WeaponFireTrack_TargetPointFor(uint8_t* self, re::Edx, V3* out, uint8_t* entity) {
    uint8_t* vehicle = At<uint8_t*>(entity, 0x1554);
    bool riding = vehicle && At<int32_t>(vehicle, 0x46C) == 1;
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    V3 point;
    const V3* p = riding ? WeaponFireTrack_TargetPointFn(self, &point, entity)
                         : WeaponFireTrack_TargetPoint2Fn(self, &point, entity);
    *out = *p;
    return out;
}

// 0x662750 (thiscall, the track unused): a weapon's muzzle: its model's first 2D effect of
// kind 0x11 placed by its frame, else the weapon's position
V3* __fastcall WeaponFireTrack_Muzzle(uint8_t*, re::Edx, V3* out, uint8_t* weapon) {
    uint8_t* info = modelInfoPtrs[At<int16_t>(weapon, 0x10E)];
    uint8_t* effects = At<uint8_t*>(At<uint8_t*>(info, 0xC), 0x38);
    uint32_t count = uint32_t(int32_t(At<int8_t>(effects, 0x1E)));
    uint8_t* list = At<uint8_t*>(effects, 0);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* effect = list + i * 0x14;
        if (effect[0x11] == 0x11) {
            float muzzle[3];
            Matrix_TransformPoint_41A9A0(muzzle, TwoDEffect_GetMatrix_8B6120(effect), reinterpret_cast<float*>(effect));
            out->x = muzzle[0];
            out->y = muzzle[1];
            out->z = muzzle[2];
            return out;
        }
    }
    const float* at = Pos(weapon);
    out->x = at[0];
    out->y = at[1];
    out->z = at[2];
    return out;
}

// 0x662800 (thiscall): a point to shoot at on an entity: its node `bone`'s position, else
// the ped's target point when it's the ped's target, else its position; with the aim error
V3* __fastcall WeaponFireTrack_TargetBonePoint(uint8_t* self, re::Edx, V3* out, uint8_t* entity, int bone) {
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    float transform[13];
    transform[12] = 1.0f;
    if (bone != -1 && GetNodeTransform(entity, bone, transform, 1)) {
        out->x = transform[9];
        out->y = transform[10];
        out->z = transform[11];
    } else {
        const float* p;
        float point[3];
        if (Ped_GetTarget(At<uint8_t*>(self, 0x40)) == entity) {
            p = Ped_GetTargetPoint_496EC0(At<uint8_t*>(self, 0x40), point);
        } else {
            p = Pos(entity);
        }
        out->x = p[0];
        out->y = p[1];
        out->z = p[2];
    }
    V3 error;
    const V3* e = WeaponFireTrack_SpreadFn(self, &error, &out->x);
    out->x = out->x + e->x;
    out->y = e->y + out->y;
    out->z = e->z + out->z;
    return out;
}

// 0x6628C0 (thiscall): a target point adjusted: offset up by +0x20, along the shot by
// +0x1C and sideways by +0x18 times the distance; along the entity's facing by +0x24; led
// for the entity's movement; the result also in out
float* __fastcall WeaponFireTrack_AdjustTarget(uint8_t* self, re::Edx, float* out, float fx, float fy, float fz,
                                               float* target, uint8_t* entity) {
    float from[3] = {fx, fy, fz};
    V3 offset = {0.0f, 0.0f, 0.0f};
    if (!(0.0f == At<float>(self, 0x18) && 0.0f == At<float>(self, 0x1C) && 0.0f == At<float>(self, 0x20))) {
        V3 d;
        VectorSub(&d, target, from);
        d.z = 0.0f;
        float flat = d.y * d.y + d.x * d.x;
        flat = flat + 0.0f * 0.0f;
        flat = std::fabs(flat);
        float distance = Sqrt(flat);
        V3 up = {0.0f, 0.0f, 1.0f};
        V3 side;
        VectorCross(&side, &d, &up);
        Vector_Normalise(&side);
        float rise = At<float>(self, 0x20);
        float flatRise = rise * 0.0f;
        V3 v1 = {flatRise, flatRise, rise};
        float along = At<float>(self, 0x1C);
        V3 v2 = {along * d.x, d.y * along, along * d.z};
        float across = At<float>(self, 0x18);
        V3 scaled = {side.x * distance, side.y * distance, distance * side.z};
        V3 v3 = {scaled.x * across, scaled.y * across, across * scaled.z};
        V3 sum;
        const V3* s = VectorAdd(&sum, &v3, &v2);
        V3 total;
        const V3* t = VectorAdd(&total, s, &v1);
        offset.x = t->x + 0.0f;
        offset.y = t->y + 0.0f;
        offset.z = 0.0f + t->z;
    }
    if (!(0.0f == At<float>(self, 0x24)) && entity) {
        float k = At<float>(self, 0x24);
        V3 forward;
        const V3* f = Placeable_GetForward(entity, &forward);
        V3 w = {f->x * k, f->y * k, k * f->z};
        offset.x = w.x + offset.x;
        offset.y = w.y + offset.y;
        offset.z = w.z + offset.z;
    }
    target[0] = target[0] + offset.x;
    target[1] = target[1] + offset.y;
    target[2] = offset.z + target[2];
    if (entity) {
        V3 led;
        const V3* l = WeaponFireTrack_LeadTargetFn(self, &led, from[0], from[1], from[2], target, entity);
        target[0] = l->x;
        target[1] = l->y;
        target[2] = l->z;
    }
    if (Fn683A30()) {
        Fn683B50(from, target, entity);
    }
    out[0] = target[0];
    out[1] = target[1];
    out[2] = target[2];
    return out;
}

// 0x662BA0 (thiscall): where the shot goes from `from`: a scripted aim point (the ped's
// +0x3BC), else its target (the node's +0x438 with its bone +0x43C when +0x31; else the ped's
// kept target +0x54C (released), its targeting's target or look target; none when the
// player isn't aiming with a stick at 0): a ped's aim point, an object's muzzle point (its
// vf 0x14), else a bone's; with no target the point ahead at the weapon's range (the
// camera's in first person or mode 13); adjusted except for objects and bones
V3* __fastcall WeaponFireTrack_ChooseTarget(uint8_t* self, re::Edx, V3* out, float fx, float fy, float fz) {
    int32_t mode = gameMode;
    uint8_t* ped = At<uint8_t*>(self, 0x40);
    int32_t type = At<int32_t>(ped, 0x1310);
    bool firstPerson = type == 0xD && mode == 2;
    bool mode13 = type == 0xD && mode == 0xD;
    int32_t bone = -1;
    uint8_t* target = nullptr;
    if (self[0x31]) {
        target = At<uint8_t*>(ped, 0x438);
        bone = At<int32_t>(ped, 0x43C);
    } else {
        bool none = false;
        if (type == 0xD && !ped[0x1EC0] && 0.0f == Pad_GetValue_738840(10, 0) &&
            Targeting_GetMode_856850(At<void*>(At<uint8_t*>(self, 0x40), 0x1C8)) != 2 && !firstPerson && !mode13) {
            none = true;
        }
        if (!none) {
            uint8_t* p = At<uint8_t*>(self, 0x40);
            re::Ptr32<uint8_t>* kept = &At<uint8_t*>(p, 0x54C);
            target = *kept;
            if (!target) {
                target = TargetingSystem_GetTarget(At<void*>(p, 0x1C8));
            } else {
                Entity_CleanUpOldReference(target, kept);
                *kept = nullptr;
            }
            if (!target) {
                target = TargetingSystem_GetLookTarget(At<void*>(At<uint8_t*>(self, 0x40), 0x1C8));
            }
        }
    }
    bool isPed = target && (target[0x108] & 7) == 3;
    bool special = target && vc::Call<uint8_t>(target, 0x14);
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    uint8_t* near = At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8) + 0x3BC;
    if (NearList_HasAimPoint_48BEE0(near)) {
        V3 point;
        *out = *NearList_GetAimPoint_48BF00(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8) + 0x3BC, &point);
        return out;
    }
    V3 point;
    if (!target) {
        uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0);
        float lo = At<float>(weapon, 0x124);
        float range = lo + (At<float>(weapon, 0x120) - lo) * At<float>(self, 0x44);
        if (range < 0.01f) {
            range = 0.01f;
        }
        if (!firstPerson && !mode13) {
            *out = *WeaponFireTrack_AimPointFn(self, &point, fx, fy, fz, range);
        } else {
            *out = *reinterpret_cast<V3*>(WeaponFireTrack_CameraTargetFn(self, &point.x, fx, fy, fz, range));
        }
    } else if (isPed) {
        *out = *WeaponFireTrack_TargetPointForFn(self, &point, target);
    } else {
        if (special) {
            *out = *WeaponFireTrack_MuzzleFn(self, &point, target);
        } else {
            *out = *WeaponFireTrack_TargetBonePointFn(self, &point, target, bone);
        }
        return out;
    }
    float adjusted[3];
    WeaponFireTrack_AdjustTargetFn(self, adjusted, fx, fy, fz, &out->x, target);
    return out;
}

// 0x662E90 (thiscall): WeaponFireSpecificOrientationTrack's vf5: a projectile fired from
// the weapon at a set angle: the ped's heading turned by +0x4C degrees, pitched by +0x50
// (-90..90) degrees, at speed +0x54; its spread from the info by power and the ped's stat
// 0x1F; then the fire globals reset
void __fastcall WeaponFireSpecificOrientationTrack_Fire(uint8_t* self, re::Edx) {
    float position[3];
    WeaponFireTrack_FirePosition_65E9E0(self, position);
    V3 forward;
    Placeable_GetForward(At<uint8_t*>(self, 0x40), &forward);
    float heading = Vector_Heading(&forward);
    float yaw = At<float>(self, 0x4C);
    yaw = yaw * 3.1415927f;
    yaw = yaw / 180.0f;
    yaw = yaw + heading;
    yaw = LimitRadianAngle(yaw);
    V3 dir;
    VecFromHeading(&dir, yaw);
    float pitch = At<float>(self, 0x50);
    if (pitch < -90.0f) {
        pitch = -90.0f;
    } else if (90.0f < pitch) {
        pitch = 90.0f;
    }
    pitch = pitch * 3.1415927f;
    pitch = pitch / 180.0f;
    float rise = crt::Tanf(pitch);   // tan
    dir.z = rise + dir.z;
    Vector_Normalise(&dir);
    float speed = At<float>(self, 0x54);
    dir.x = dir.x * speed;
    dir.y = dir.y * speed;
    dir.z = speed * dir.z;
    WeaponFireTrack_ResolveInfoFn(self);
    float spread = 0.0f;
    uint8_t* info = At<uint8_t*>(self, 0x48);
    if (info) {
        float s = At<float>(info, 0x50) - At<float>(info, 0x4C);
        s = s * At<float>(self, 0x44);
        spread = s + At<float>(info, 0x4C);
    }
    int32_t stat = Ped_GetStat(At<uint8_t*>(self, 0x40), 0x1F);
    float skill = float(stat) / 100.0f;
    spread = skill * spread;
    int32_t r20 = Weapon_Fn4D95E0(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0));
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8);
    uint8_t* weapon = At<uint8_t*>(ped, 0x1D0);
    int32_t sinceLast = At<int32_t>(weapon, 0x140);
    if (sinceLast) {
        sinceLast = int32_t(timeInMs) - sinceLast;
    }
    float holdMs = At<float>(ped, 0x1CC0) * 1000.0f;
    int32_t hold = int32_t(int64_t(holdMs));
    int32_t fired = forcedWeapon_B31C9C;
    if (fired == -1) {
        fired = At<int32_t>(weapon, 0x12C);
    }
    int flag = At<uint32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x1D0), 0x26C) != 0 ? 1 : 0;
    AddProjectile(At<uint8_t*>(self, 0x40), fired, position[0], position[1], position[2], dir.x, dir.y, dir.z,
                  At<uint8_t*>(self, 0x48), spread, At<float>(self, 0x44), r20, sinceLast, hold,
                  fireVectorB_1B65C24[0], fireVectorB_1B65C24[1], fireVectorB_1B65C24[2], fireVectorA_1B65C48[0],
                  fireVectorA_1B65C48[1], fireVectorA_1B65C48[2], 0, flag);
    WeaponFire_ResetGlobals_65D2C0();
}

// the next scan code (all cleared on wrapping)
inline void NextScanCode() {
    if (scanCode_C1AE88 < 0xFFFF) {
        scanCode_C1AE88 = uint16_t(scanCode_C1AE88 + 1);
    } else {
        ClearScanCodes_45D840();
        scanCode_C1AE88 = 1;
    }
}
// a sector list node's entity and the next node (0x3FFF: the end)
inline uint8_t* NodeEntity(const uint32_t* node) {
    return GetEntityFromNode_44A3E0(int(*node & 0xF), int((*node >> 4) & 0x3FFF));
}
inline uint32_t* NextNode(const uint32_t* node) {
    uint32_t next = *node >> 18;
    return next == 0x3FFF ? nullptr : sectorLinks_C0F788 + next;
}
// an entity worth sweeping: with an rw object and collision, in this chapter, not hidden
inline bool SweepListed(uint8_t* entity) {
    return At<void*>(entity, 0x20) && (At<void*>(entity, 0x1C) || At<void*>(entity, 0x60)) &&
           (modelInfoPtrs[At<int16_t>(entity, 0x10E)][0xB] & uint8_t(1 << chapter_A147E0)) &&
           At<int32_t>(entity, 0xF4) == 0;
}
inline void ClearColPoint(ColPoint& colPoint) {
    colPoint.pos = {0.0f, 0.0f, 0.0f};
    colPoint.normal = {0.0f, 0.0f, 0.0f};
    colPoint.entity = nullptr;
    colPoint.depth = 3.4028235e38f;
}
// a matrix made the identity at a point (its fourth position word is a stale stack word in
// the game: 0 here)
inline void SweepMatrixAt(SweepMatrix* m, const float* at) {
    m->attach = 0;
    Matrix_Construct_412770(m);
    m->pos[0] = at[0];
    m->pos[1] = at[1];
    m->pos[2] = at[2];
    m->pos[3] = 0.0f;
}
inline float SweepLength(const V3& d) {
    float lengthSq = d.x * d.x + d.y * d.y;
    lengthSq = lengthSq + d.z * d.z;
    return Sqrt(std::fabs(lengthSq));
}
// the sector of a coordinate (x / 50 + 18, truncated)
inline int SweepSector(float v) {
    float scaled = float(double(v) / sectorSize_900AA0);
    return int(float(double(scaled) + sectorOffset_900A98));
}
// the entity an impulse track pushes: the ped's +0x544, else its +0x438 (physical only)
inline uint8_t* ImpulseTarget(uint8_t* ped) {
    uint8_t* target = At<uint8_t*>(ped, 0x544);
    if (!target) {
        target = At<uint8_t*>(ped, 0x438);
    }
    return target && At<int32_t>(target, 0xC4) ? target : nullptr;
}
// the projectile an impulsed entity is (+0x238) credited to the ped, now
inline void ImpulseCredit(uint8_t* target, uint8_t* ped) {
    int32_t index = At<int32_t>(target, 0x238);
    if (index < 0) {
        return;
    }
    uint8_t* projectile = GetProjectileInstance_4D34B0(index);
    if (projectile) {
        EntityRef_Set_478550(projectile + 4, ped);
        At<uint32_t>(projectile, 8) = timeInMs;
    }
}

// 0x664350 (thiscall): a sector's object list swept: each entity (once per scan code, the
// code taken at the start) touching the sphere, solid (vf 0x7C) and not vf 0x78, its
// collision model against the swept sphere from `from1` to `to1` (or, with `both`, `from2`
// to `to2`); a hit with a clear line from `from` (raised to the sweep's end height; the
// entity ignored) to the entity passed to the track's hit
uint8_t __fastcall WeaponCollisionTrack_TestObjectList(uint8_t* self, re::Edx, const float* centre, float radius,
                                                       float* from, const SweepMatrix* to1, const SweepMatrix* from1,
                                                       int both, const SweepMatrix* to2, const SweepMatrix* from2,
                                                       uint32_t* list) {
    uint16_t code = scanCode_C1AE88;
    ColPoint colPoint;
    ClearColPoint(colPoint);
    uint8_t hit = 0;
    if (!list) {
        return 0;
    }
    do {
        uint8_t* entity = NodeEntity(list);
        list = NextNode(list);
        if (At<uint16_t>(entity, 0x10A) == code) {
            continue;
        }
        At<uint16_t>(entity, 0x10A) = code;
        if (!SweepListed(entity) || !Entity_IsTouchingMoving(entity, centre, radius)) {
            continue;
        }
        if (!vc::Call<uint8_t>(entity, 0x7C) || vc::Call<uint8_t>(entity, 0x78)) {
            continue;
        }
        float* matrix = Entity_GetMatrix_46DD20(entity);
        uint8_t* colModel = GetColModel_51AF50(entity);
        int kind;
        if (ProcessColModels_56C8C0(from1, to1, sweepColModel_CF3140, matrix, matrix, colModel, &colPoint, 0, 0, 0, 0,
                                    0, 0xF, 0, 1)) {
            kind = 1;
        } else if (uint8_t(both) && ProcessColModels_56C8C0(from2, to2, sweepColModel_CF3140, matrix, matrix,
                                                            colModel, &colPoint, 0, 0, 0, 0, 0, 0xF, 0, 1)) {
            kind = 2;
        } else {
            continue;
        }
        float end[3] = {matrix[12], matrix[13], 0.0f};
        float z = kind == 1 ? to1->pos[2] : to2->pos[2];
        end[2] = z;
        from[2] = z;
        re::Ptr32<uint8_t> blocker = nullptr;
        ignoredEntity_C1AE70 = entity;
        uint8_t blocked = ProcessLineOfSight(from, end, &colPoint, &blocker, 0x49, 0, 0);
        ignoredEntity_C1AE70 = nullptr;
        NextScanCode();
        if (!blocked && CollisionTrack_Hit_5F8A50(self, entity, &colPoint.pos.x, -1)) {
            hit = 1;
        }
    } while (list);
    ReleaseColPoint(colPoint);
    return hit;
}

// 0x664640 (thiscall): a sector's vehicle list swept (no scan codes): each listed vehicle
// touching the sphere and not a train-like +0x46C, its collision model against the swept
// sphere(s); a hit passed to the track's hit, the vehicle damaged at the hit piece by the
// track's +0x34 (the owner's ped the damager)
uint8_t __fastcall WeaponCollisionTrack_TestVehicleList(uint8_t* self, re::Edx, const float* centre, float radius,
                                                        const SweepMatrix* to1, const SweepMatrix* from1, int both,
                                                        const SweepMatrix* to2, const SweepMatrix* from2,
                                                        uint32_t* list) {
    ColPoint colPoint;
    ClearColPoint(colPoint);
    uint8_t hit = 0;
    if (!list) {
        return 0;
    }
    do {
        uint8_t* entity = NodeEntity(list);
        list = NextNode(list);
        if (!SweepListed(entity) || (entity[0x108] & 7) != 2) {
            continue;
        }
        if (!Entity_IsTouchingMoving(entity, centre, radius) || At<int32_t>(entity, 0x46C) != 0) {
            continue;
        }
        float* matrix = Entity_GetMatrix_46DD20(entity);
        uint8_t* colModel = GetColModel_51AF50(entity);
        if (!ProcessColModels_56C8C0(from1, to1, sweepColModel_CF3140, matrix, matrix, colModel, &colPoint, 0, 0, 0, 0,
                                     0, 0xF, 0, 1) &&
            !(uint8_t(both) && ProcessColModels_56C8C0(from2, to2, sweepColModel_CF3140, matrix, matrix, colModel,
                                                       &colPoint, 0, 0, 0, 0, 0, 0xF, 0, 1))) {
            continue;
        }
        if (CollisionTrack_Hit_5F8A50(self, entity, &colPoint.pos.x, -1)) {
            hit = 1;
        }
        // (the piece goes as a word zero-extended from the point's byte +0x23)
        Automobile_SetDamage_4A8410(entity, colPoint.pad1C[7], At<float>(self, 0x34),
                                    At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8), &colPoint.normal);
    } while (list);
    ReleaseColPoint(colPoint);
    return hit;
}

// 0x664D40 (thiscall): the sectors within 4 of the owner's ped (clamped to the 36 x 36
// grid), row by row: their object lists (lists & 8) and vehicle lists (lists & 2) swept.
// True when any hit was taken.
uint8_t __fastcall WeaponCollisionTrack_TestSectors(uint8_t* self, re::Edx, const float* centre, float radius,
                                                    float* from, const SweepMatrix* to1, const SweepMatrix* from1,
                                                    int both, const SweepMatrix* to2, const SweepMatrix* from2,
                                                    uint32_t lists) {
    const float* pos = Pos(At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8));
    float x = pos[0];
    float y = pos[1];
    float maxX = float(double(x) + sectorMargin_900970);
    float minY = float(double(y) - sectorMargin_900970);
    float maxY = float(double(y) + sectorMargin_900970);
    float minX = float(double(x) - sectorMargin_900970);
    float v = float(SweepSector(minX));
    int x0 = int(v > 0.0f ? v : 0.0f);
    v = float(SweepSector(minY));
    int y0 = int(v > 0.0f ? v : 0.0f);
    v = float(SweepSector(maxX));
    int x1 = int(v < sectorLast_900A94 ? v : sectorLast_900A94);
    v = float(SweepSector(maxY));
    int y1 = int(v < sectorLast_900A94 ? v : sectorLast_900A94);
    uint8_t hit = 0;
    for (int sy = y0; sy <= y1; sy++) {
        for (int sx = x0; sx <= x1; sx++) {
            int cell = (sx + sy * 36) * 5;
            if ((lists & 8) && WeaponCollisionTrack_TestObjectListFn(self, centre, radius, from, to1, from1, both, to2,
                                                                     from2, sectorObjects_C1B17C[cell])) {
                hit = 1;
            }
            if ((lists & 2) && WeaponCollisionTrack_TestVehicleListFn(self, centre, radius, to1, from1, both, to2,
                                                                      from2, sectorVehicles_C1B180[cell])) {
                hit = 1;
            }
        }
    }
    return hit;
}

// 0x664870 (thiscall): WeaponFireTrack's vf5: the shot: none without a weapon to fire;
// its spread (the info's by power, or +0x2C by power) by the ped's stat 0x1F; aimed from
// the fire position at the chosen target and launched (a frisbee shot's velocity instead
// built from the side / along / up scales); a spray can's effect killed instead of a
// projectile; the weapon's +0x128 set to -100; the fire globals reset
void __fastcall WeaponFireTrack_Fire(uint8_t* self, re::Edx) {
    if (forcedWeapon_B31C9C == -1 &&
        At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), 0x12C) == -1) {
        WeaponFire_ResetGlobals_65D2C0();
        return;
    }
    WeaponFireTrack_ResolveInfoFn(self);
    float spread = 0.0f;
    uint8_t* info = At<uint8_t*>(self, 0x48);
    if (info) {
        float s = At<float>(info, 0x50) - At<float>(info, 0x4C);
        s = s * At<float>(self, 0x44);
        spread = s + At<float>(info, 0x4C);
    }
    if (!(-1.0f == At<float>(self, 0x2C))) {
        spread = At<float>(self, 0x44) * At<float>(self, 0x2C);
    }
    int32_t stat = Ped_GetStat(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1F);
    float skill = float(stat) / 100.0f;
    spread = skill * spread;
    float position[3];
    WeaponFireTrack_FirePosition_65E9E0(self, position);
    V3 target;
    WeaponFireTrack_ChooseTargetFn(self, &target, position[0], position[1], position[2]);
    V3 velocity;
    WeaponFire_LaunchVelocityFn(&velocity, position[0], position[1], position[2], target.x, target.y, target.z,
                                At<float>(self, 0x44), At<uint8_t*>(self, 0x48), At<uint8_t*>(self, 0x40));
    if (frisbeeShot_1B65C14) {
        V3 up = {0.0f, 0.0f, 1.0f};
        V3 d;
        VectorSub(&d, &target.x, position);
        Vector_Normalise(&d);
        V3 side;
        VectorCross(&side, &d, &up);
        Vector_Normalise(&side);
        float k = frisbeeFlight_1B65C18[2];
        V3 v1 = {k * up.x, k * up.y, k * up.z};
        k = frisbeeFlight_1B65C18[1];
        V3 v2 = {d.x * k, d.y * k, k * d.z};
        k = frisbeeFlight_1B65C18[0];
        V3 v3 = {side.x * k, side.y * k, k * side.z};
        V3 sum;
        const V3* s = VectorAdd(&sum, &v3, &v2);
        V3 total;
        velocity = *VectorAdd(&total, s, &v1);
    }
    int32_t r20 = Weapon_Fn4D95E0(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0));
    uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0);
    int32_t sinceLast = At<int32_t>(weapon, 0x140);
    if (sinceLast) {
        sinceLast = int32_t(timeInMs) - sinceLast;
    }
    if (At<int16_t>(weapon, 0x10E) == sprayCanModel_BF44E0) {
        if (r20) {
            EffectProxy_Kill_668BA0(reinterpret_cast<uint8_t*>(r20));   // (a spray can's: its effect)
        }
    } else {
        uint8_t* locked = TargetingSystem_GetTarget(At<void*>(At<uint8_t*>(self, 0x40), 0x1C8));
        if (!locked) {
            locked = Targeting_Fn4D5F60(At<void*>(At<uint8_t*>(self, 0x40), 0x1C8));
        }
        uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8);
        float holdMs = At<float>(ped, 0x1CC0) * 1000.0f;
        int32_t hold = int32_t(int64_t(holdMs));
        int32_t fired = forcedWeapon_B31C9C;
        if (fired == -1) {
            fired = At<int32_t>(At<uint8_t*>(ped, 0x1D0), 0x12C);
        }
        // (the flag goes as a dword whose upper bytes are the weapon pointer's)
        int flag = At<uint32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x1D0), 0x26C) != 0 ? 1 : 0;
        AddProjectile(ped, fired, position[0], position[1], position[2], velocity.x, velocity.y, velocity.z,
                      At<uint8_t*>(self, 0x48), spread, At<float>(self, 0x44), r20, sinceLast, hold,
                      fireVectorB_1B65C24[0], fireVectorB_1B65C24[1], fireVectorB_1B65C24[2], fireVectorA_1B65C48[0],
                      fireVectorA_1B65C48[1], fireVectorA_1B65C48[2], locked, flag);
    }
    At<float>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), 0x128) = -100.0f;
    WeaponFire_ResetGlobals_65D2C0();
}

// 0x664C60 (thiscall, the track unused): an entity given a velocity: re-placed where it is,
// its move speed set and turn speed cleared, pushed at its top (bound radius up) unless
// +0x30, its vf 0x1C(0) called and it put on the moving list
void __fastcall WeaponImpulseTrack_Apply(uint8_t*, re::Edx, uint8_t* entity, float vx, float vy, float vz) {
    const float* at = Pos(entity);
    Entity_SetPosition_41C4E0(entity, at[0], at[1], at[2]);
    At<float>(entity, 0x124) = vx;
    At<float>(entity, 0x128) = vy;
    At<float>(entity, 0x12C) = vz;
    At<float>(entity, 0x130) = 0.0f;
    At<float>(entity, 0x134) = 0.0f;
    At<float>(entity, 0x138) = 0.0f;
    if (!At<int32_t>(entity, 0x30)) {
        float top = Entity_GetBoundRadius_465E10(entity);
        Physical_ApplyTurnForce_46A950(entity, vx, vy, vz, 0.0f, 0.0f, top);
    }
    vc::Call<void>(entity, 0x1C, 0);
    Physical_AddToMovingList_4697D0(entity);
}

// 0x664FE0 (thiscall): WeaponImpulseTrack::Start: the ped's held / grabbed entity pushed
// along its flat facing pitched up +0x1C degrees at speed +0x18; a projectile credited to
// the ped; an event (0xE) past the Butes' speed
uint8_t __fastcall WeaponImpulseTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    uint8_t* target = ImpulseTarget(ped);
    if (!target) {
        return 1;
    }
    V3 forward;
    Placeable_GetForward(ped, &forward);
    forward.z = 0.0f;
    Vector_Normalise(&forward);
    float angle = At<float>(self, 0x1C) * 3.14159274f;
    angle = angle / 180.0f;
    forward.z = trk::CrtSin(angle);
    float c = trk::CrtCos(angle);
    forward.x = c * forward.x;
    forward.y = c * forward.y;
    float k = At<float>(self, 0x18);
    V3 v = {k * forward.x, k * forward.y, k * forward.z};
    WeaponImpulseTrack_ApplyFn(self, target, v.x, v.y, v.z);
    target[0x22C] = 0;
    ImpulseCredit(target, At<uint8_t*>(owner, 8));
    float speed = SweepLength(v);
    if (At<float>(butes_20C88B8, 0x64) < speed) {
        const float* p = Pos(target);
        float at[3] = {p[0], p[1], p[2]};
        Events_Add_4A6670(eventManager_C2C1A0, 0xE, target, nullptr, at, -1.0f, -1.0f);
    }
    return 1;
}

// 0x6651E0 (thiscall): WeaponTargetImpulseTrack::Start: with a look target (not the pushed
// entity itself) the entity launched at it (half its vf 0x6C up) as its model's weapon at
// full power; else the plain impulse
uint8_t __fastcall WeaponTargetImpulseTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* look = TargetingSystem_GetLookTarget(At<void*>(At<uint8_t*>(owner, 8), 0x1C8));
    if (!look) {
        return WeaponImpulseTrack_StartFn(self, owner);
    }
    uint8_t* target = ImpulseTarget(At<uint8_t*>(At<uint8_t*>(self, 0x20), 8));
    if (!target) {
        return 1;
    }
    if (look == target) {
        return WeaponImpulseTrack_StartFn(self, owner);
    }
    const float* p = Pos(target);
    float from[3] = {p[0], p[1], p[2]};
    float h = vc::Call<float>(look, 0x6C) * 0.5f;
    V3 offset = {0.0f, 0.0f, h};
    V3 aim;
    VectorAdd(&aim, reinterpret_cast<const V3*>(Pos(look)), &offset);
    uint8_t* weaponInfo = At<uint8_t*>(modelInfoPtrs[At<int16_t>(target, 0x10E)], 0x18);
    if (!weaponInfo) {
        return 1;
    }
    WeaponInfo_Load_7282A0(weaponInfo);
    V3 velocity;
    WeaponFire_LaunchVelocityFn(&velocity, from[0], from[1], from[2], aim.x, aim.y, aim.z, 1.0f,
                                At<uint8_t*>(weaponInfo, 0x80), nullptr);
    target[0x22C] = 0;
    ImpulseCredit(target, At<uint8_t*>(owner, 8));
    WeaponImpulseTrack_ApplyFn(self, target, velocity.x, velocity.y, velocity.z);
    if (At<float>(butes_20C88B8, 0x64) < Vector_Magnitude_40DFB0(&velocity)) {
        Events_Add_4A6670(eventManager_C2C1A0, 0xE, target, look, from, -1.0f, -1.0f);
    }
    return 1;
}

// 0x6653D0 (thiscall): WeaponMinigameImpulseTrack::Start: in a minigame the entity launched
// (power +0x38 = 1) at the minigame's point (its vf 0x18) as its model's weapon; else the
// plain impulse
uint8_t __fastcall WeaponMinigameImpulseTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    if (!minigame_20C8704) {
        return WeaponImpulseTrack_StartFn(self, owner);
    }
    uint8_t* target = ImpulseTarget(At<uint8_t*>(owner, 8));
    if (!target) {
        return 1;
    }
    At<float>(self, 0x38) = 1.0f;
    const float* p = Pos(target);
    float from[3] = {p[0], p[1], p[2]};
    V3 point;
    vc::Call<void*>(minigame_20C8704, 0x18, &point, 0);
    uint8_t* weaponInfo = At<uint8_t*>(modelInfoPtrs[At<int16_t>(target, 0x10E)], 0x18);
    if (!weaponInfo) {
        return 1;
    }
    WeaponInfo_Load_7282A0(weaponInfo);
    V3 velocity;
    WeaponFire_LaunchVelocityFn(&velocity, from[0], from[1], from[2], point.x, point.y, point.z,
                                At<float>(self, 0x38), At<uint8_t*>(weaponInfo, 0x80), nullptr);
    WeaponImpulseTrack_ApplyFn(self, target, velocity.x, velocity.y, velocity.z);
    return 1;
}

// 0x6654F0 (thiscall): one swing sweep: the swept spheres from the last tip points (+0x200,
// +0x220) to the new ones, around their middle with room for the swing's length and twice
// the reach (the larger of the two with `both`): peds (the owner's near list), then the
// sectors' objects and vehicles. True when either took a hit.
uint8_t __fastcall WeaponCollisionTrack_Sweep(uint8_t* self, re::Edx, float* from, const float* tip1, int both,
                                              const float* tip2) {
    const float* last1 = &At<float>(self, 0x200);
    V3 d;
    VectorSub(&d, tip1, last1);
    SweepLine line1 = {{last1[0], last1[1], last1[2]}, {d.x, d.y, d.z}};
    SweepMatrix to1, from1, to2, from2;
    SweepMatrixAt(&to1, tip1);
    SweepMatrixAt(&from1, last1);
    V3 half = {d.x * 0.5f, d.y * 0.5f, d.z * 0.5f};
    V3 t;
    float centre[4];
    centre[0] = VectorAdd(&t, reinterpret_cast<const V3*>(last1), &half)->x;
    centre[1] = VectorAdd(&t, reinterpret_cast<const V3*>(last1), &half)->y;
    centre[2] = VectorAdd(&t, reinterpret_cast<const V3*>(last1), &half)->z;
    centre[3] = 0.0f;
    float length = SweepLength(d);
    float reach = At<float>(self, 0x5C);
    float radius = length + (reach + reach);
    to2.attach = 0;
    from2.attach = 0;
    SweepLine line2 = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    if (uint8_t(both)) {
        const float* last2 = &At<float>(self, 0x220);
        V3 d2;
        VectorSub(&d2, tip2, last2);
        line2 = {{last2[0], last2[1], last2[2]}, {d2.x, d2.y, d2.z}};
        SweepMatrixAt(&to2, tip2);
        SweepMatrixAt(&from2, last2);
        WeaponCollisionTrack_MiddleFn(self, tip1, last1, tip2, last2, centre);
        float length2 = SweepLength(d2);
        float reach2 = At<float>(self, 0x5C);
        float radius2 = length2 + (reach2 + reach2);
        if (radius < radius2) {
            radius = radius2;
        }
    }
    uint8_t peds = WeaponCollisionTrack_TestPedsFn(self, centre, radius, from, tip1, line1.start, both, tip2,
                                                   line2.start);
    if (WeaponCollisionTrack_TestSectorsFn(self, centre, radius, from, &to1, &from1, both, &to2, &from2, 10)) {
        return 1;
    }
    return peds;
}

// 0x665940 (thiscall): WeaponCollisionTrack::Update: before the start time the tip points
// just follow the bones; within the window (or with a negative end) the ped swings (state
// 2), past it the swing is over (3). Swinging: the swept sphere set to the reach, the tips
// swept from the ped (at the first tip's height); each fresh hit (age 0): a vehicle knocked
// (a spray can's skipped) with the hit sound; anything else (unless the node won't play
// for the owner or either is grappling) damaged by +0x34 times the ped's stat 0x1F / 100
// (an integer: +0x50 for objects), with the node's effects, a ped also grappled (+0x21C),
// stunned and made to play the node (queued with +0x21D). The hits aged, the tips kept.
uint8_t __fastcall WeaponCollisionTrack_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x20C), 8);
    float time = At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x20C), 0), 8);
    if (At<float>(self, 0x18) > time) {
        Ped_FindBonePosition(&At<float>(self, 0x200), At<uint32_t>(self, 0x210), ped);
        Ped_FindBonePosition(&At<float>(self, 0x220), At<uint32_t>(self, 0x214), ped);
        return 1;
    }
    bool active = false;
    if (At<float>(self, 0x18) < time) {
        active = At<float>(self, 0x1C) > time || 0.0f > At<float>(self, 0x1C);
    }
    if (active) {
        At<int32_t>(ped, 0x71C) = 2;
    } else if (At<float>(self, 0x1C) < time) {
        At<int32_t>(ped, 0x71C) = 3;
        return 1;
    }
    const float* p = Pos(ped);
    float tip1[3] = {p[0], p[1], p[2]};
    float tip2[3] = {p[0], p[1], p[2]};
    float pedX = p[0];
    float pedY = p[1];
    Ped_FindBonePosition(tip1, At<uint32_t>(self, 0x210), ped);
    uint8_t both = Ped_FindBonePosition(tip2, At<uint32_t>(self, 0x214), ped);
    float from[3] = {pedX, pedY, tip1[2]};
    float radius = At<float>(self, 0x5C);
    At<float>(sweepColModel_CF3140, 0xC) = radius;
    At<float>(At<uint8_t*>(At<uint8_t*>(sweepColModel_CF3140, 0x38), 0), 0xC) = radius;
    V3 boxMax = {radius, radius, radius};
    float negative = -radius;
    V3 boxMin = {negative, negative, negative};
    Box_Set_564830(sweepColModel_CF3140 + 0x10, &boxMin, &boxMax);
    NextScanCode();
    // (the flag goes as a dword whose upper bytes are stale)
    if (WeaponCollisionTrack_SweepFn(self, from, tip1, both, tip2)) {
        for (int i = 0; i < 16; i++) {
            uint8_t* hit = self + 0x80 + i * 0x18;   // {entity, age, pos, sphere}
            uint8_t* entity = At<uint8_t*>(hit, 0);
            if (!entity || !(0.0f == At<float>(hit, 4))) {
                continue;
            }
            uint8_t type = entity[0x108] & 7;
            if (type == 2) {
                uint8_t* node = Reference_Get_5F4F90(self + 0x60, -1);
                if (node && !vc::Call<uint8_t*>(node, acttree::kNodeFindPlayable, At<uint8_t*>(self, 0x20C))) {
                    break;
                }
                At<float>(hit, 4) = 0.0f;
                if (At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E) == sprayCanModel_BF44E0) {
                    continue;
                }
                WeaponCollisionTrack_CountHitFn(self);
                V3 at = At<V3>(hit, 8);
                Vehicle_Knock_4CC4A0(entity, ped, -1, 0.0f, at, 0, 1);
                if (At<const char*>(self, 0x20)) {
                    int32_t sound = SoundStreams_GetId_595B80(sounds_CF3DA0, At<const char*>(self, 0x20));
                    SoundStreams_Play_59B8A0(sounds_CF3DA0, sound, &at.x, 2, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
                }
                continue;
            }
            if (type == 4 && At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E) == sprayCanModel_BF44E0) {
                continue;
            }
            uint8_t* node = Reference_Get_5F4F90(self + 0x60, -1);
            if (node) {
                if (!vc::Call<uint8_t*>(node, acttree::kNodeFindPlayable, At<uint8_t*>(self, 0x20C))) {
                    break;
                }
                if ((entity[0x108] & 7) == 3 && At<uint8_t*>(entity, 0x155C)) {
                    break;
                }
                if (At<uint8_t*>(ped, 0x155C)) {
                    break;
                }
            }
            At<float>(hit, 4) = 0.0f;
            float base = At<float>(self, 0x34);
            int32_t stat = Ped_GetStat(ped, 0x1F);
            float damage = float(stat / 100) * base;
            type = entity[0x108] & 7;
            if (type == 6 || type == 4) {
                damage = At<float>(self, 0x50);
            }
            if (At<int32_t>(ped, 0x1310) == 0xD) {
                Stats_Increment(0x6A, 1);
            }
            WeaponCollisionTrack_CountHitFn(self);
            V3 dir = {0.0f, 0.0f, 0.0f};
            V3 hitPos = At<V3>(hit, 8);
            float f3c = At<float>(self, 0x3C);
            float f38 = At<float>(self, 0x38);
            // (the byte flags go as dwords whose upper bytes are stale)
            int b21e = self[0x21E];
            int b58 = self[0x58];
            int32_t v2c = At<int32_t>(self, 0x2C);
            uint8_t* node70 = Reference_Get_5F4F90(self + 0x70, -1);
            int32_t model = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E);
            if (!vc::Call<uint8_t>(entity, 0x3C, damage, 0.0f, ped, model, &hitPos, &dir, node70, b21e, v2c, 0, f38,
                                   f3c, b58, 1, 0, -1, 0.0f, 1)) {
                continue;
            }
            if (Reference_Get_5F4F90(self + 0x60, -1) && (entity[0x108] & 7) == 3 && self[0x21C]) {
                Ped_SetGrapplePartner_4744B0(entity, ped);
                Ped_SetGrapplePartner_4744B0(ped, entity);
            }
            if (vc::Call<uint8_t*>(entity, 0x40)) {
                if (At<const char*>(self, 0x28)) {
                    uint32_t id = EffectLibrary_Find(effectLibrary_B6F1E8, At<const char*>(self, 0x28));
                    At<uint32_t>(At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x48) = id;
                }
                if (At<const char*>(self, 0x20)) {
                    int32_t sound = SoundStreams_GetId_595B80(sounds_CF3DA0, At<const char*>(self, 0x20));
                    At<int32_t>(At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x40) = sound;
                    uint32_t volume = At<uint32_t>(self, 0x24);
                    At<uint8_t>(At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x44) = uint8_t(volume);
                }
                float f44 = At<float>(self, 0x44);
                At<float>(At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x54) = f44;
                float f40 = At<float>(self, 0x40);
                At<float>(At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x58) = f40;
            }
            if ((entity[0x108] & 7) == 3) {
                float f4c = At<float>(self, 0x4C);
                Stat_Add_4A74C0(At<uint8_t*>(entity, 0x1D8), f4c);
                // (the game reloads it from the damage's stack slot it was stored over)
                Stat_Add_4DAC30(At<uint8_t*>(entity, 0x1D4), f4c);
                Ped_AddStun_47CD90(ped, At<float>(self, 0x54), 0);
            }
            uint8_t* node2 = Reference_Get_5F4F90(self + 0x60, -1);
            if (node2) {
                uint8_t* controller = At<uint8_t*>(ped, 0x2E0) + 0x258;
                uint8_t* playable = vc::Call<uint8_t*>(node2, acttree::kNodeFindPlayable, At<uint8_t*>(self, 0x20C));
                if (self[0x21D]) {
                    Ctl_QueueTracks_5F3F80(controller, playable);
                } else {
                    Ctl_PlayNode_5F45F0(controller, playable);
                }
            }
        }
    }
    Track_AgeHits_5F8AE0(self, dt, At<float>(self, 0x30));
    At<float>(self, 0x200) = tip1[0];
    At<float>(self, 0x204) = tip1[1];
    At<float>(self, 0x208) = tip1[2];
    At<float>(self, 0x220) = tip2[0];
    At<float>(self, 0x224) = tip2[1];
    At<float>(self, 0x228) = tip2[2];
    return 1;
}

//@@NEXT@@
} // namespace

#define WEAPONTRACKV_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

WEAPONTRACKV_REVERSED("WeaponFireTrack::AimPoint_65EBB0", 0x0065EBB0, &WeaponFireTrack_AimPoint);
WEAPONTRACKV_REVERSED("WeaponFireTrack::Spread_65ECA0", 0x0065ECA0, &WeaponFireTrack_Spread);
WEAPONTRACKV_REVERSED("WeaponFireTrack::LeadTarget_65EE10", 0x0065EE10, &WeaponFireTrack_LeadTarget);
WEAPONTRACKV_REVERSED("WeaponFire::LaunchVelocity_65F1B0", 0x0065F1B0, &WeaponFire_LaunchVelocity);
WEAPONTRACKV_REVERSED("WeaponFire::LaunchVelocityNoPed_65FFA0", 0x0065FFA0, &WeaponFire_LaunchVelocityNoPed);
WEAPONTRACKV_REVERSED("WeaponFireTrack::CountShot_65F630", 0x0065F630, &WeaponFireTrack_CountShot);
WEAPONTRACKV_REVERSED("ExtinguisherFireTrack::Update_65F7A0", 0x0065F7A0, &ExtinguisherFireTrack_Update);
WEAPONTRACKV_REVERSED("PoisonSprayFireTrack::Update_65F900", 0x0065F900, &PoisonSprayFireTrack_Update);
WEAPONTRACKV_REVERSED("WeaponFXTrack::Start_65F9D0", 0x0065F9D0, &WeaponFXTrack_Start);
WEAPONTRACKV_REVERSED("WeaponFXTrack::End_65FAF0", 0x0065FAF0, &WeaponFXTrack_End);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::Start_6617C0", 0x006617C0, &WeaponCollisionTrack_Start);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::End_6618F0", 0x006618F0, &WeaponCollisionTrack_End);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::Middle_661910", 0x00661910, &WeaponCollisionTrack_Middle);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::TestPeds_661B30", 0x00661B30, &WeaponCollisionTrack_TestPeds);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::CountHit_661E70", 0x00661E70, &WeaponCollisionTrack_CountHit);
WEAPONTRACKV_REVERSED("SetWeaponFlagsTrack::Start_661EA0", 0x00661EA0, &SetWeaponFlagsTrack_Start);
WEAPONTRACKV_REVERSED("WeaponScaleTrack::Start_661F20", 0x00661F20, &WeaponScaleTrack_Start);
WEAPONTRACKV_REVERSED("WeaponSpawnDamageEffectTrack::Start_661FA0", 0x00661FA0, &WeaponSpawnDamageEffectTrack_Start);
WEAPONTRACKV_REVERSED("WeaponSetNextDamageLevelTrack::Start_661FD0", 0x00661FD0, &WeaponSetNextDamageLevelTrack_Start);
WEAPONTRACKV_REVERSED("WeaponPlayerFireAutoTargetTrack::Start_662040", 0x00662040, &WeaponPlayerFireAutoTargetTrack_Start);
WEAPONTRACKV_REVERSED("WeaponFireTrack::Start_662070", 0x00662070, &WeaponFireTrack_Start);
WEAPONTRACKV_REVERSED("WeaponFireTrack::ResolveInfo_662200", 0x00662200, &WeaponFireTrack_ResolveInfo);
WEAPONTRACKV_REVERSED("WeaponFireTrack::CameraTarget_6622E0", 0x006622E0, &WeaponFireTrack_CameraTarget);
WEAPONTRACKV_REVERSED("WeaponFireTrack::TargetPoint_662460", 0x00662460, &WeaponFireTrack_TargetPoint);
WEAPONTRACKV_REVERSED("WeaponFireTrack::TargetPoint2_6624E0", 0x006624E0, &WeaponFireTrack_TargetPoint2);
WEAPONTRACKV_REVERSED("WeaponFireTrack::TargetPointFor_6626E0", 0x006626E0, &WeaponFireTrack_TargetPointFor);
WEAPONTRACKV_REVERSED("WeaponFireTrack::Muzzle_662750", 0x00662750, &WeaponFireTrack_Muzzle);
WEAPONTRACKV_REVERSED("WeaponFireTrack::TargetBonePoint_662800", 0x00662800, &WeaponFireTrack_TargetBonePoint);
WEAPONTRACKV_REVERSED("WeaponFireTrack::AdjustTarget_6628C0", 0x006628C0, &WeaponFireTrack_AdjustTarget);
WEAPONTRACKV_REVERSED("WeaponFireTrack::ChooseTarget_662BA0", 0x00662BA0, &WeaponFireTrack_ChooseTarget);
WEAPONTRACKV_REVERSED("WeaponFireSpecificOrientationTrack::Fire_662E90", 0x00662E90, &WeaponFireSpecificOrientationTrack_Fire);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::TestObjectList_664350", 0x00664350, &WeaponCollisionTrack_TestObjectList);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::TestVehicleList_664640", 0x00664640, &WeaponCollisionTrack_TestVehicleList);
WEAPONTRACKV_REVERSED("WeaponFireTrack::Fire_664870", 0x00664870, &WeaponFireTrack_Fire);
WEAPONTRACKV_REVERSED("WeaponImpulseTrack::Apply_664C60", 0x00664C60, &WeaponImpulseTrack_Apply);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::TestSectors_664D40", 0x00664D40, &WeaponCollisionTrack_TestSectors);
WEAPONTRACKV_REVERSED("WeaponImpulseTrack::Start_664FE0", 0x00664FE0, &WeaponImpulseTrack_Start);
WEAPONTRACKV_REVERSED("WeaponTargetImpulseTrack::Start_6651E0", 0x006651E0, &WeaponTargetImpulseTrack_Start);
WEAPONTRACKV_REVERSED("WeaponMinigameImpulseTrack::Start_6653D0", 0x006653D0, &WeaponMinigameImpulseTrack_Start);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::Sweep_6654F0", 0x006654F0, &WeaponCollisionTrack_Sweep);
WEAPONTRACKV_REVERSED("WeaponCollisionTrack::Update_665940", 0x00665940, &WeaponCollisionTrack_Update);
//@@REG@@
