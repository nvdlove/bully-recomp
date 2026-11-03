// CExplosion (0xC39B60, 15 of them) and CProjectileInfo (0xC3A038, 32 of them, with
// their projectiles in ms_apProjectile), the target search the auto-aim of thrown
// projectiles uses, and the trail remnants a destroyed projectile leaves behind.
//
// (Weapon.cpp calls the explosions "trail effects" and ms_apProjectile "psets"; they are
// what the code here shows: explosions with a radius that grows over their lifetime,
// damage through CWorld::TriggerExplosion and a sound, and the projectile entities.)
//
// Arithmetic is float, in the game's order: its x87 code runs in single precision
// (D3D9's default FPU mode) and its double constants are floats widened.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Pool.h"
#include "reversed.h"

#define PROJECTILE_C12_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

namespace projectile_c12 {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// a vector the game passes by value with its (unset) fourth float
struct CVector4 {
    float x, y, z, w;
};
static_assert(sizeof(CVector4) == 0x10);

// Bully's CMatrix (0x44 bytes: no owns-attached flag)
struct CMatrix {
    CVector4 right;    // +0x00
    CVector4 forward;  // +0x10
    CVector4 up;       // +0x20
    CVector4 pos;      // +0x30
    re::Ptr32<void> attached;    // +0x40
};
static_assert(sizeof(CMatrix) == 0x44);

// the effects system's handle on an effect
struct AeEffectProxy {
    int32_t unknown0;  // +0x0
    int32_t handle;    // +0x4 (0: none)
};
static_assert(sizeof(AeEffectProxy) == 0x8);

// An explosion. `data` is its weapon's projectile data (the same block CProjectileInfo
// points to): +0x94 start radius, +0x98/+0x9C/+0xA0 what TriggerExplosion gets, +0xC8
// duration, +0xCC delay, +0xD0 radius growth per step, ...
struct CExplosion {
    re::Ptr32<uint8_t> data;          // +0x00
    AeEffectProxy effect;   // +0x04
    re::Ptr32<void> creator;          // +0x0C (registered) who threw it
    re::Ptr32<void> cause;            // +0x10 (registered) the projectile
    CVector position;       // +0x14
    uint32_t endTime;       // +0x20
    uint8_t active;         // +0x24
    uint8_t pad25[3];       // +0x25
    uint32_t delayUntil;    // +0x28 (0: gone off)
    float radius;           // +0x2C
    float unknown30;        // +0x30
    int32_t weaponType;     // +0x34
    uint32_t startTime;     // +0x38
};
static_assert(sizeof(CExplosion) == 0x3C);
static_assert(offsetof(CExplosion, position) == 0x14);
static_assert(offsetof(CExplosion, delayUntil) == 0x28);
static_assert(offsetof(CExplosion, startTime) == 0x38);

// A projectile in flight (its entity in ms_apProjectile at the same index)
struct CProjectileInfo {
    int32_t weaponType;     // +0x00
    re::Ptr32<void> source;           // +0x04 (registered) who threw it
    uint32_t startTime;     // +0x08
    uint32_t fuse;          // +0x0C ms after the start it goes off (0: on impact)
    int32_t unknown10;      // +0x10 (data +0xF0)
    uint8_t inUse;          // +0x14
    uint8_t unknown15;      // +0x15 (0x1B65C14 when added)
    uint8_t exploded;       // +0x16
    uint8_t animFlag;       // +0x17 which of its anims (PlayProjectileAnim)
    uint8_t landed;         // +0x18 its landing anim played
    uint8_t pad19[3];       // +0x19
    float unknown1C;        // +0x1C
    AeEffectProxy effect;   // +0x20
    int32_t trail;          // +0x28
    int32_t unknown2C;      // +0x2C
    re::Ptr32<uint8_t> data;          // +0x30 its weapon's projectile data
};
static_assert(sizeof(CProjectileInfo) == 0x34);
static_assert(offsetof(CProjectileInfo, inUse) == 0x14);
static_assert(offsetof(CProjectileInfo, effect) == 0x20);
static_assert(offsetof(CProjectileInfo, data) == 0x30);

// a sector list's node: the entity's pool and slot, and the next node's index
struct PtrNode {
    uint32_t bits;
};
static_assert(sizeof(PtrNode) == 4);

} // namespace projectile_c12

namespace {

using projectile_c12::AeEffectProxy;
using projectile_c12::CExplosion;
using projectile_c12::CMatrix;
using projectile_c12::CProjectileInfo;
using projectile_c12::CVector;
using projectile_c12::CVector4;
using projectile_c12::PtrNode;

using re::At;
inline uint8_t* Ptr(const void* object, uint32_t offset) {
    return At<uint8_t*>(object, offset);
}
// an entity's position (its matrix's, or its own when it has none)
inline const CVector* EntityPos(const void* entity) {
    uint8_t* matrix = Ptr(entity, 0x14);
    return reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : static_cast<const uint8_t*>(entity) + 4);
}
inline uint8_t EntityType(const void* entity) {
    return At<uint8_t>(entity, 0x108) & 7;
}
inline uint32_t FloatBits(float f) {
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    return bits;
}
inline float BitsFloat(uint32_t bits) {
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}
// the x87 control word (fnstcw), for a stack slot the game half overwrites with it
uint16_t X87ControlWord() {
#if defined(_M_IX86)
    uint16_t controlWord;
    __asm fnstcw controlWord;
    return controlWord;
#else
    return 0x007F;
#endif
}

// the explosions, projectiles and trail remnants
GAME_ARRAY(CExplosion, aExplosions, 0x00C39B60);
constexpr int NUM_EXPLOSIONS = 15;
GAME_VAR(uint8_t*, gDefaultExplosionData, 0x00C39B48);
GAME_ARRAY(CProjectileInfo, gaProjectileInfo, 0x00C3A038);
GAME_ARRAY(uint8_t*, ms_apProjectile, 0x00C3A6B8);
constexpr int NUM_PROJECTILES = 32;
GAME_VAR(int32_t, gNumTrailRemnants, 0x00C39F20);
GAME_ARRAY(int32_t, gTrailRemnants, 0x00C39F28);
GAME_ARRAY(int32_t, gTrailRemnantTimes, 0x00C39FA8);
GAME_ARRAY(CVector4, gTrailRemnantPos, 0x00C3A738);

// the target search's state
GAME_VAR(uint8_t, gSearchCheckHeight, 0x00C39EE4);
GAME_VAR(uint8_t, gSearchAnyAngle, 0x00C39EE5);
GAME_VAR(void*, gSearchFrom, 0x00C39EE8);
GAME_VAR(float, gSearchUnknownEC, 0x00C39EEC);
GAME_VAR(float, gSearchBestScore, 0x00C39EF0);
GAME_VAR(uint8_t*, gSearchBest, 0x00C39EF4);
GAME_VAR(float, gSearchRange, 0x00C39EF8);
GAME_VAR(float, gSearchHeading, 0x00C39EFC);
GAME_VAR(CVector, gSearchPos, 0x00C39F00);
GAME_VAR(float, gSearchAngleRange, 0x00A4533C);   // the angle off the heading a target scores 1 at
GAME_VAR(float, gSearchMaxAngle, 0x00A45340);     // (compared with pi - the angle off)

GAME_VAR(uint32_t, gTimer, 0x00C1A9B4);
GAME_VAR(float, gTimeStep, 0x00C1A9A4);
GAME_VAR(void*, gExplosionCause, 0x00C1AE70);     // the projectile of the explosion being processed
GAME_VAR(uint8_t, gUnknownC1AE83, 0x00C1AE83);
GAME_VAR(uint8_t*, gPlayerPed, 0x00C1AEA8);
GAME_VAR(int32_t, gCurrArea, 0x00BD1008);
GAME_VAR(void*, gStimuli, 0x00C2C1A0);
GAME_VAR(void*, gEffects, 0x00B6F1D4);
GAME_VAR(void*, gEffectLibrary, 0x00B6F1E8);
GAME_VAR(uint8_t, gUnknown1B65C14, 0x01B65C14);   // projectiles added aimed by angles
GAME_VAR(uint8_t*, gDefaultProjectileData, 0x020C88C8);
GAME_VAR(uint8_t*, gUnknown20C88B8, 0x020C88B8);  // +0x64: the speed above which a projectile is an event
GAME_ARRAY(uint8_t*, gModelInfoPtrs, 0x00C67738);
GAME_VAR(CPool<uint8_t>*, gPedPool, 0x00C0F5F0);
GAME_VAR(CPool<uint8_t>*, gVehiclePool, 0x00C0F5F4);
GAME_VAR(uint16_t, gScanCode, 0x00C1AE88);
GAME_VAR(PtrNode*, gPtrNodes, 0x00C0F788);
GAME_VAR(CVector, gZeroVector, 0x00BCC564);
inline uint8_t* const gSectorLists = reinterpret_cast<uint8_t*>(0x00C1B17C);   // 36 a row, 0x14 bytes each

// weapon/model indices the code singles out
GAME_VAR(int32_t, gModelFlatExplosion, 0x00BF43E8);   // its explosion uses the default data unless it hits a floor
GAME_VAR(int32_t, gModelUnequip, 0x00BF432C);
GAME_VAR(int32_t, gModelBall, 0x00BF43E4);
GAME_VAR(int32_t, gModelBF4414, 0x00BF4414);
GAME_VAR(int32_t, gModelBF4184, 0x00BF4184);
GAME_VAR(int32_t, gModelBF3E60, 0x00BF3E60);
GAME_VAR(int32_t, gModelLevel1, 0x00BF4368);   // (these four fly level: their forward has no z)
GAME_VAR(int32_t, gModelLevel2, 0x00BF4364);
GAME_VAR(int32_t, gModelLevel3, 0x00BF4344);
GAME_VAR(int32_t, gModelLevel4, 0x00BF4334);

// objects the code calls into
inline void* const gSoundStreams = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const gTrails = reinterpret_cast<void*>(0x01B850E0);
inline void* const gSurfaceTable = reinterpret_cast<void*>(0x00C21BC0);
inline void* const gSurfaceSounds = reinterpret_cast<void*>(0x00CF1F60);
inline void* const gWater = reinterpret_cast<void*>(0x00BC5DA8);
inline void* const gAnimManager = reinterpret_cast<void*>(0x020C4910);

// the config names and type of the default explosion data
inline const int32_t* const gExplosionPathNames[3] = {reinterpret_cast<const int32_t*>(0x01B6531C),
                                                      reinterpret_cast<const int32_t*>(0x01B65394),
                                                      reinterpret_cast<const int32_t*>(0x01B65544)};
inline void* const gExplosionDataType = reinterpret_cast<void*>(0x01B65398);

GAME_FN(void, __cdecl, ClearAllExplosions, 0x004D2680);
GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, int32_t* dst, const int32_t* src);
GAME_FN(void, __thiscall, Name_Destruct, 0x0049A650, int32_t* name);   // (does nothing)
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* path, int count, const int32_t* names);
GAME_FN(uint8_t*, __cdecl, Config_Get, 0x0072AEA0, void* path, void* type);
GAME_FN(uint8_t, __cdecl, TriggerExplosion, 0x00463CC0, int type, const CVector* pos, float radius, float innerRadius,
        float force, float damage, void* creator, int includeCreator);
GAME_FN(void, __thiscall, AE_Proxy_SlowKill, 0x00668D00, AeEffectProxy* proxy, float time);
GAME_FN(void, __thiscall, AE_Proxy_Kill, 0x00668BA0, AeEffectProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_668C40_x64, 0x00668C40, AeEffectProxy* proxy, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AE_Proxy_668C40(AeEffectProxy* a0, float a1) {
    return AE_Proxy_668C40_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, AE_Proxy_Construct, 0x00668E90, AeEffectProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Destruct, 0x00668EB0, AeEffectProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_SetPosition, 0x00669040, AeEffectProxy* proxy, const CVector* pos, int);
GAME_FN(void, __thiscall, AE_Proxy_SetDirection, 0x00669360, AeEffectProxy* proxy, const CVector* dir);
GAME_FN(void, __thiscall, AE_Effects_Release, 0x00668100, void* effects, AeEffectProxy* proxy);
GAME_FN(void, __thiscall, AE_Effects_Start, 0x00668290, void* effects, const uint32_t* type, AeEffectProxy* proxy, int);
GAME_FN(void, __thiscall, AE_Effects_StartEffect, 0x006680C0, void* effects, void* effect, AeEffectProxy* proxy);
GAME_FN(uint16_t, __thiscall, AE_Library_Find, 0x00667B60, void* library, const char* name);
GAME_FN(void, __thiscall, ProjectileData_Resolve, 0x00728430, uint8_t* data);   // (+0x14C from the name at +0xB4)
GAME_FN(void, __thiscall, Butes_Resolve, 0x007282A0, uint8_t* butes);
GAME_FN(uint8_t*, __cdecl, WeaponInfo_Get, 0x0051B450, int model);
GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(void, __cdecl, Explosion_GetPlayerVehicleSound, 0x0070CB60, const CVector* pos, int, int32_t* a, uint32_t* b);
GAME_FN(uint8_t, __cdecl, IsAreaWithWater, 0x00430320, int area);
GAME_FN(int, __thiscall, SoundStreams_GetId, 0x00595B80, void* streams, const char* name);
GAME_FN(uint32_t, __thiscall, SoundStreams_Play, 0x0059B8A0, void* streams, int id, const CVector* pos, int a3,
        const CVector* pos2, int a5, int a6, int a7, int a8, uintptr_t a9, uintptr_t a10, int a11, int a12, float volume, int a14);
GAME_FN(void, __thiscall, SoundStreams_59BAB0, 0x0059BAB0, void* streams, int id, int32_t a, uint32_t b, int c);
GAME_FN(void, __thiscall, SoundStreams_59BBD0, 0x0059BBD0, void* streams, int id, const void* colPoint, int surface,
        int a, int count);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, void* sound, uintptr_t handle);
GAME_FN(void, __thiscall, Stimuli_Add, 0x004A6670, void* stimuli, int kind, void* from, void* to, const CVector* pos,
        float a, float b);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(float, __cdecl, FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int flag);
GAME_FN(uint8_t, __thiscall, SurfaceTable_HasEffects, 0x00464280, void* table, int32_t set);
GAME_FN(uint16_t, __thiscall, SurfaceTable_GetEffect, 0x004642B0, void* table, const uint8_t* surface, int32_t set);
GAME_FN(uint16_t, __thiscall, SurfaceTable_GetSound, 0x00464330, void* table, const uint8_t* surface, int32_t set);
GAME_FN(void, __thiscall, SurfaceSounds_Play, 0x005610C0, void* sounds, int id, const CVector* pos, const CVector* dir,
        int surface);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* entity, CVector* out);
GAME_FN(float, __cdecl, GetATanOfXY, 0x00432FE0, float x, float y);
GAME_FN(float, __cdecl, AngleDifference, 0x004D3070, float a, float b);
GAME_FN(uint8_t, __thiscall, PedTargets_CanTarget, 0x0048D470, void* targets, void* ped);
GAME_FN(uint8_t, __thiscall, Seats_IsAnyoneIn, 0x004CE640, void* seats);
GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, uint32_t seat);
GAME_FN(int, __cdecl, GetGameMode, 0x004A84A0);
GAME_FN(uint8_t, __thiscall, AnimDict_Find, 0x006B1D60, void* dict, const char* name, uintptr_t, int32_t* index);
GAME_FN(void*, __thiscall, Clump_Fn6B9380, 0x006B9380, void* clump, int);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C0D40, void* manager, void* clump, uintptr_t index, void* anim, int,
        float, float);
GAME_FN(void, __thiscall, Trails_Remove, 0x0066BF40, void* trails, int id);
GAME_FN(void, __thiscall, Trails_AddPoint_x64, 0x0066CD10, void* trails, int id, float, float, float, float, float width);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Trails_AddPoint(void* a0, int a1, CVector4 a2, float a3) {
    return Trails_AddPoint_x64(a0, a1, re::FloatAt(a2, 0), re::FloatAt(a2, 1), re::FloatAt(a2, 2), re::FloatAt(a2, 3), a3);
}
GAME_FN(void, __thiscall, Trails_SetAlpha, 0x0066BDC0, void* trails, int id, float alpha);
GAME_FN(int32_t, __thiscall, Trails_Add, 0x0066BB00, void* trails, int kind, float width, uint32_t colour, int);
GAME_FN(CVector*, __thiscall, Entity_GetBoundCentre, 0x00466C70, void* entity, CVector* out);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __thiscall, Entity_443AA0, 0x00443AA0, void* entity);
GAME_FN(void, __thiscall, Entity_465760, 0x00465760, void* entity);
GAME_FN(uint8_t, __thiscall, Water_GetLevel, 0x006D4A20, void* water, float* level, int32_t* type, const CVector* pos);
GAME_FN(uint8_t, __thiscall, Ped_IsAlive, 0x0047B7A0, void* ped);
GAME_FN(void*, __cdecl, Frame_GetTransform, 0x006C9A90, void* frame);
GAME_FN(float, __thiscall, Vector_NormaliseSafe, 0x00415130, CVector* v);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, Vector_Normalise2, 0x004294A0, CVector* v);
GAME_FN(void, __cdecl, ScaleVector, 0x004D3660, CVector* out, const CVector* v, float scale);
GAME_FN(void, __cdecl, AddVectors, 0x004D36B0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, RwObject_SetAlpha, 0x005F2B00, void* object, int alpha);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(uint8_t*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, uint32_t slot);
GAME_FN(void*, __cdecl, Projectile_New, 0x004D2640, uintptr_t size);   // CProjectile::operator new
GAME_FN(uint8_t*, __thiscall, Projectile_Construct, 0x004D22B0, void* memory, int model, int index);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);   // (+0x14)
GAME_FN(void, __thiscall, Matrix_SetRotate, 0x00412D00, CMatrix* m, const CVector* angles);
GAME_FN(void, __thiscall, Matrix_Multiply, 0x00412800, CMatrix* m, const CMatrix* by);   // (*=)
GAME_FN(void, __thiscall, Placeable_SetMatrix, 0x0046DE90, void* entity, const CMatrix* m, int);
// (our own, called where modes apply)
GAME_FN(void, __thiscall, Explosion_Process_, 0x004D27E0, CExplosion* self, int withSound);
GAME_FN(uint8_t, __cdecl, Explosion_Add_, 0x004D2C90, int weaponType, int useSurface, const uint8_t* surface, void* cause,
        void* creator, const CVector* pos, const CVector* normal, uint8_t* data);
GAME_FN(void, __cdecl, TargetSearch_Begin_, 0x004D30B0, uint8_t* entity, float range, int checkHeight);
GAME_FN(void, __cdecl, TargetSearch_Consider_, 0x004D3160, uint8_t* entity);
GAME_FN(void, __cdecl, Projectile_Splash_, 0x004D3570, const CVector* pos, float waterZ, int type);
GAME_FN(void, __cdecl, AddTrailRemnant_, 0x004D3730, int trail, int duration, const CVector* pos);
GAME_FN(void, __cdecl, UpdateTrailRemnants_, 0x004D3860);
GAME_FN(void, __cdecl, Projectile_Explode_, 0x004D3920, int index, int hasSurface, const uint8_t* surface, CVector pos,
        CVector normal);
GAME_FN(void, __thiscall, ProjectileInfo_UpdateTrail_, 0x004D3DE0, CProjectileInfo* self, uint8_t* projectile, float alpha);
GAME_FN(void, __cdecl, Projectile_Destroy_, 0x004D3F10, int index, int explode);
GAME_FN(int, __cdecl, Projectile_DestroyOldest_, 0x004D40B0);
GAME_FN(int, __cdecl, Projectile_FindNearest_, 0x004D4140, float x, float y, float z, float radius, float maxSpeed, int model,
        void* source, int armedOnly);
GAME_FN(int, __cdecl, Projectile_FindFreeSlot_, 0x004D46E0);

} // namespace

namespace projectile_c12 {

// CExplosion

// 0x4D2760: every explosion cleared, and the default explosion data found (the one a
// flat-exploding weapon's explosion uses when it doesn't hit a floor)
void __cdecl Explosion_Initialise() {
    ClearAllExplosions();
    int32_t names[3];
    Name_Copy(&names[0], gExplosionPathNames[0]);
    Name_Copy(&names[1], gExplosionPathNames[1]);
    Name_Copy(&names[2], gExplosionPathNames[2]);
    uint8_t path[8];
    Path_Build(path, 3, names);
    gDefaultExplosionData = Config_Get(path, gExplosionDataType);
    for (int i = 2; i >= 0; i--) {
        Name_Destruct(&names[i]);
    }
}

// 0x4D27E0: the explosion's damage (CWorld::TriggerExplosion), the end of its effect,
// and with `withSound` its sound, louder and more to the front the nearer the camera
void __fastcall Explosion_Process(CExplosion* self, re::Edx, int withSound) {
    // (the global goes back to what it was only when there's a cause: else it's cleared)
    void* saved = nullptr;
    if (self->cause) {
        saved = gExplosionCause;
        gExplosionCause = self->cause;
    }
    uint8_t* data = self->data;
    // (the flag goes as a dword whose upper bytes are uninitialised stack in the game)
    uint8_t hit = TriggerExplosion(self->weaponType, &self->position, self->radius, At<float>(data, 0x98),
                                   At<float>(data, 0x9C), At<float>(data, 0xA0), self->creator, data[0x90]);
    gExplosionCause = saved;
    data = self->data;
    if (data[0xDD] && hit) {
        uint32_t lifetime = At<uint32_t>(data, 0xE0);
        if (lifetime == 0) {
            AE_Proxy_SlowKill(&self->effect, 0.5f);
            if (self->effect.handle) {
                AE_Effects_Release(gEffects, &self->effect);
            }
            self->active = 0;
        } else {
            // (the end brought forward to now + the lifetime, through floats)
            float end = float(self->endTime);
            float soon = float(uint32_t(gTimer + lifetime));
            float result = !(soon > end) ? soon : end;
            self->endTime = uint32_t(int64_t(result));
        }
    }
    data = self->data;
    ProjectileData_Resolve(data);
    if (At<int32_t>(data, 0x154) != 0 && re::Flag(withSound)) {
        // `slot` is the argument's stack slot, which the game reuses as a float and a
        // vehicle class and passes on as the sound id when the data names none
        uint32_t slot;
        CVector camera;
        CVector toCamera;
        VectorSub(&toCamera, &self->position, GetCameraPos(&camera));
        float lengthSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z;
        float distance = std::sqrt(std::fabs(lengthSq));
        if (70.0f < distance) {
            distance = 70.0f;
        }
        float closeness = distance / 70.0f;
        slot = FloatBits(closeness);
        int left;
        int right;
        if (!(0.2f < closeness)) {
            left = 127;
            right = int(int64_t(closeness / 0.8f * 127.0f));
            slot = (slot & 0xFFFF0000u) | X87ControlWord();
        } else if (!(0.8f > closeness)) {
            left = 0;
            right = 127;
        } else {
            slot = (slot & 0xFFFF0000u) | X87ControlWord();
            right = int(int64_t(closeness / 0.8f * 127.0f));
            float fade = (closeness - 0.2f) / 0.6f;
            slot = FloatBits(fade);
            slot = (slot & 0xFFFF0000u) | X87ControlWord();
            left = 127 - int(int64_t(127.0f * fade));
        }
        bool rocket = false;
        if (gPlayerPed && Ptr(gPlayerPed, 0x1554)) {
            int32_t* vehicleClass = Vehicle_GetVehicleClass(Ptr(gPlayerPed, 0x1554), reinterpret_cast<int32_t*>(&slot));
            if (*vehicleClass == 10 && self->weaponType == GetWeaponIndex("brocket")) {
                rocket = true;
            }
        }
        if (rocket) {
            // (the player's bike rocket: its own sound)
            int32_t volume = 0;
            Explosion_GetPlayerVehicleSound(&self->position, 5, &volume, &slot);
            SoundStreams_59BAB0(gSoundStreams, SoundStreams_GetId(gSoundStreams, "MissleExplode"), volume, slot, 0);
        } else {
            int id;
            if (self->position.z < 0.0f && IsAreaWithWater(gCurrArea)) {
                id = SoundStreams_GetId(gSoundStreams, "WaterExplosion");
            } else if (const char* name = reinterpret_cast<const char*>(Ptr(self->data, 0xB8))) {
                id = SoundStreams_GetId(gSoundStreams, name);
            } else {
                id = int(slot);
            }
            SoundStreams_Play(gSoundStreams, id, &self->position, At<int32_t>(self->data, 0xBC), nullptr, 0, 0x78, left,
                              right, 0, 0, 6, 0, 1.0f, 0);
        }
    }
    data = self->data;
    if (data[0x8F]) {
        Stimuli_Add(gStimuli, 3, self->creator, nullptr, &self->position, At<float>(data, 0xD8), -1.0f);
    }
}

// 0x4D2BF0: each explosion's delay counted down and then its lifetime (its radius
// growing), processed each step
void __cdecl Explosion_Update() {
    for (CExplosion* e = aExplosions; e < aExplosions + NUM_EXPLOSIONS; e++) {
        if (!e->active) {
            continue;
        }
        if (e->delayUntil) {
            if (gTimer > e->delayUntil) {
                e->delayUntil = 0;
                Explosion_Process_(e, 1);
            }
            continue;
        }
        bool alive = gTimer < e->endTime;
        e->radius = At<float>(e->data, 0xD0) * gTimeStep + e->radius;
        if (alive) {
            Explosion_Process_(e, 0);
        } else {
            e->active = 0;
            AE_Proxy_SlowKill(&e->effect, 0.5f);
            if (e->effect.handle) {
                AE_Effects_Release(gEffects, &e->effect);
            }
        }
    }
}

// 0x4D2C90: a new explosion (with all in use, the oldest whose data allows it is
// replaced), its effect (the surface's when `useSurface`) and, without a delay, its
// first processing. 0: no room
uint8_t __cdecl Explosion_Add(int weaponType, int useSurface, const uint8_t* surface, void* cause, void* creator,
                              const CVector* pos, const CVector* normal, uint8_t* data) {
    uint32_t oldestTime = 0xFFFFFFFFu;
    int oldest = -1;
    int slot = 0;
    for (; slot < NUM_EXPLOSIONS; slot++) {
        CExplosion* e = &aExplosions[slot];
        if (!e->active) {
            break;
        }
        if (e->data[0xDC] && e->startTime < oldestTime) {
            oldestTime = e->startTime;
            oldest = slot;
        }
    }
    if (slot == NUM_EXPLOSIONS) {
        if (oldest == -1) {
            return 0;
        }
        CExplosion* e = &aExplosions[oldest];
        AE_Proxy_SlowKill(&e->effect, 5.0f);
        if (e->effect.handle) {
            AE_Effects_Release(gEffects, &e->effect);
        }
        e->active = 0;
        slot = oldest;
    }
    CExplosion* e = &aExplosions[slot];
    e->data = data;
    if (weaponType == gModelFlatExplosion && std::fabs(normal->z) < 0.96f) {
        e->data = gDefaultExplosionData;
    }
    e->startTime = gTimer;
    e->position = *pos;
    e->unknown30 = 0.0f;
    e->radius = At<float>(data, 0x94);
    e->weaponType = weaponType;
    e->creator = creator;
    if (creator) {
        RegisterReference(creator, &e->creator);
    }
    e->cause = cause;
    if (cause) {
        RegisterReference(cause, &e->cause);
    }
    e->active = 1;
    uint32_t delay = At<uint32_t>(data, 0xCC);
    e->delayUntil = delay ? delay + gTimer : 0;
    e->endTime = At<uint32_t>(data, 0xC8) + At<uint32_t>(data, 0xCC) + gTimer;
    // (the result unused)
    FindGroundZFor3DCoord(pos->x, pos->y, pos->z + 3.0f, nullptr, 1);
    uint32_t effectType = 0xFFFF;
    ProjectileData_Resolve(e->data);
    if (re::Flag(useSurface)) {
        if (SurfaceTable_HasEffects(gSurfaceTable, At<int32_t>(e->data, 0x14C))) {
            ProjectileData_Resolve(e->data);
            effectType = SurfaceTable_GetEffect(gSurfaceTable, surface, At<int32_t>(e->data, 0x14C));
            ProjectileData_Resolve(e->data);
            uint16_t sound = SurfaceTable_GetSound(gSurfaceTable, surface, At<int32_t>(e->data, 0x14C));
            SurfaceSounds_Play(gSurfaceSounds, sound, &e->position, normal, *surface);
        } else {
            ProjectileData_Resolve(e->data);
            effectType = At<uint16_t>(e->data, 0x14A);
        }
    } else {
        effectType = At<uint16_t>(e->data, 0x14A);
    }
    AE_Effects_Start(gEffects, &effectType, &e->effect, 0);
    AE_Proxy_SetPosition(&e->effect, &e->position, 0);
    if (data[0xD4]) {
        CVector direction = *normal;
        AE_Proxy_SetDirection(&e->effect, &direction);
    }
    if (e->delayUntil == 0) {
        Explosion_Process_(e, 1);
    }
    return 1;
}

// The target search (the auto-aim of thrown projectiles)

// 0x4D30B0: a search from the entity (its position and heading) within `range`
void __cdecl TargetSearch_Begin(uint8_t* entity, float range, int checkHeight) {
    gSearchPos = *EntityPos(entity);
    CVector forwardA;
    CVector forwardB;
    CVector* a = Placeable_GetForward(entity, &forwardA);
    CVector* b = Placeable_GetForward(entity, &forwardB);
    gSearchHeading = GetATanOfXY(a->x, b->y);
    gSearchRange = range;
    gSearchFrom = entity;
    gSearchBestScore = -10000.0f;
    gSearchBest = nullptr;
    gSearchAnyAngle = 0;
    gSearchUnknownEC = 0.0f;
    gSearchCheckHeight = uint8_t(checkHeight);
}

// 0x4D3160: the entity scored (nearer and nearer the heading is better) and kept if best
void __cdecl TargetSearch_Consider(uint8_t* entity) {
    if (At<int32_t>(entity, 0xC0) != 0) {
        return;
    }
    if (EntityType(entity) == 3 && entity[0x1C98]) {
        return;
    }
    const CVector* pos = EntityPos(entity);
    float dx = pos->x - gSearchPos.x;
    float dy = pos->y - gSearchPos.y;
    float distance = std::sqrt(std::fabs(dx * dx + dy * dy));
    if (!(gSearchRange >= distance)) {
        return;
    }
    if (gSearchCheckHeight) {
        float dz = std::fabs(pos->z - gSearchPos.z);
        if (!(dz <= 2.0f)) {
            return;
        }
    }
    float nearScore = -distance * 0.1f;
    CVector offset;
    VectorSub(&offset, pos, &gSearchPos);
    offset.z = offset.z + 1.1f;
    float angle = GetATanOfXY(offset.x, offset.y);
    GetATanOfXY(distance, offset.z);   // (the pitch, unused)
    float angleOff = std::fabs(AngleDifference(angle, gSearchHeading));
    float ratio = angleOff / gSearchAngleRange;
    if (!gSearchAnyAngle && gSearchMaxAngle > 3.1415927f - angleOff) {
        return;
    }
    if (!(ratio <= 1.0f) && !(3.0f > distance)) {
        return;
    }
    float score = nearScore - ratio * 0.9f * 5.0f;
    if (gSearchBestScore < score) {
        gSearchBestScore = score;
        gSearchBest = entity;
    }
}

// 0x4D3370: the best ped for the ped to throw at within `range` (nullptr: none)
uint8_t* __cdecl TargetSearch_FindPed(uint8_t* ped, float range) {
    TargetSearch_Begin_(ped, range, 0);
    CPool<uint8_t>* pool = gPedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* other = pool->objects + pool->elemSize * slot;
        if (!other || other == ped || other[0x1C98]) {
            continue;
        }
        if (PedTargets_CanTarget(ped + 0xA64, other)) {
            TargetSearch_Consider_(other);
        }
    }
    return gSearchBest;
}

// 0x4D33F0: the best vehicle (of the type, -1 any; with `emptyOnly` nobody in it) within
// `range`, heights checked; `anyAngle`: behind counts too
uint8_t* __cdecl TargetSearch_FindVehicle(uint8_t* entity, float range, int vehicleType, int emptyOnly, int anyAngle) {
    TargetSearch_Begin_(entity, range, 1);
    gSearchAnyAngle = uint8_t(anyAngle);
    CPool<uint8_t>* pool = gVehiclePool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* vehicle = pool->objects + pool->elemSize * slot;
        if (!vehicle || (vehicle[0x1BC] & 0x10)) {
            continue;
        }
        if (vehicleType != -1 && At<int32_t>(vehicle, 0x46C) != vehicleType) {
            continue;
        }
        if (re::Flag(emptyOnly) && Seats_IsAnyoneIn(vehicle + 0x1C4)) {
            continue;
        }
        TargetSearch_Consider_(vehicle);
    }
    return gSearchBest;
}

// CPhysical

struct CPhysicalC12 {
    // 0x4D3480
    void SetTurnSpeed(const CVector* speed) {
        At<CVector>(this, 0x130) = *speed;
    }
};

// CProjectileInfo

// 0x4D34C0 (the projectile in eax, its data in ecx): one of its four anims played on its
// clump, the launch pair (+0x134/+0x138) or the landing pair (+0x13C/+0x140)
void PlayProjectileAnim(uint8_t* projectile, uint8_t* data, int second, int launch) {
    uint8_t* clump = Ptr(projectile, 0x114);
    if (!clump || !Ptr(clump, 0x94) || !Ptr(Ptr(clump, 0x94), 0x10)) {
        return;
    }
    void* dict = Ptr(Ptr(clump, 0x94), 0x10);
    uint32_t first = re::Flag(launch) ? 0x134 : 0x13C;
    const char* anim = re::Flag(second) ? reinterpret_cast<const char*>(Ptr(data, first + 4))
                                        : reinterpret_cast<const char*>(Ptr(data, first));
    int32_t index = -1;
    if (!anim) {
        return;
    }
    AnimDict_Find(dict, anim, 0, &index);
    if (index == -1) {
        return;
    }
    AnimManager_Play(gAnimManager, clump, index, Clump_Fn6B9380(clump, 0), 0, 1.0f, -1.0f);
}

// 0x4D3570: the splash sound and effect (by the water's type) where it hit the water
void __cdecl Projectile_Splash(const CVector* pos, float waterZ, int type) {
    CVector at = {pos->x, pos->y, waterZ};
    uint32_t effect = 0xFFFFFFFFu;
    const char* name = nullptr;
    if (type == 1) {
        name = "SplashWater";
    } else if (type == 3) {
        name = "SplashAcid";
    } else if (type == 2) {
        name = "SplashMud";
    }
    if (name) {
        effect = AE_Library_Find(gEffectLibrary, name);
    }
    SoundStreams_Play(gSoundStreams, SoundStreams_GetId(gSoundStreams, "PebbleSplash"), &at, 3, nullptr, 0, 0x78, 0, 0, 0,
                      0, 6, 0, 1.0f, 0);
    AeEffectProxy proxy;
    AE_Proxy_Construct(&proxy);
    uint32_t effectType = uint16_t(effect);
    AE_Effects_Start(gEffects, &effectType, &proxy, 0);
    AE_Proxy_SetPosition(&proxy, &at, 0);
    AE_Proxy_Destruct(&proxy);
}

// 0x4D3730: a destroyed projectile's trail kept fading for `duration` ms at its last
// position (in a free slot, or the one ending first)
void __cdecl AddTrailRemnant(int trail, int duration, const CVector* pos) {
    if (!trail) {
        return;
    }
    int bestTime = 0;
    int slot = 0;
    for (int i = 0; i < NUM_PROJECTILES; i++) {
        if (gTrailRemnants[i] == 0) {
            bestTime = 0;
            slot = i;
        } else if (gTrailRemnantTimes[i] < bestTime) {
            bestTime = gTrailRemnantTimes[i];
            slot = i;
        }
    }
    if (gTrailRemnants[slot]) {
        Trails_Remove(gTrails, gTrailRemnants[slot]);
        gNumTrailRemnants -= 1;
        gTrailRemnants[slot] = 0;
    }
    gTrailRemnantTimes[slot] = int32_t(gTimer + uint32_t(duration));
    gTrailRemnants[slot] = trail;
    gNumTrailRemnants += 1;
    // (the fourth float is uninitialised stack in the game; never read)
    gTrailRemnantPos[slot] = CVector4{pos->x, pos->y, pos->z, 0.0f};
}

// 0x4D3860 (missing from functions.csv): the remnants ended removed, the others given
// their point again
void __cdecl UpdateTrailRemnants() {
    if (gNumTrailRemnants == 0) {
        return;
    }
    int32_t now = int32_t(gTimer);
    for (int i = 0; i < NUM_PROJECTILES; i++) {
        if (gTrailRemnants[i] == 0) {
            continue;
        }
        if (now > gTrailRemnantTimes[i]) {
            Trails_Remove(gTrails, gTrailRemnants[i]);
            gNumTrailRemnants -= 1;
            gTrailRemnants[i] = 0;
        }
        if (gTrailRemnants[i]) {
            // (the fourth float is uninitialised stack in the game)
            const CVector4& pos = gTrailRemnantPos[i];
            Trails_AddPoint(gTrails, gTrailRemnants[i], CVector4{pos.x, pos.y, pos.z, pos.w}, 0.01f);
        }
    }
}

// 0x4D3920: the projectile goes off (once): its explosion when its data has a radius,
// and its stimulus
void __cdecl Projectile_Explode(int index, int hasSurface, const uint8_t* surface, CVector pos, CVector normal) {
    CProjectileInfo* info = &gaProjectileInfo[index];
    if (info->exploded) {
        return;
    }
    uint8_t* data = info->data;
    info->exploded = 1;
    if (0.0f < At<float>(data, 0x94)) {
        Explosion_Add_(info->weaponType, hasSurface, surface, ms_apProjectile[index], info->source, &pos, &normal, data);
    }
    if (info->data[0x40]) {
        Stimuli_Add(gStimuli, At<int32_t>(info->data, 0x44), info->source, nullptr, nullptr, -1.0f, -1.0f);
    }
}

// the bounce counter in the projectile's +0x23C (its low three bits, up to 5)
static void CountBounce(uint8_t* projectile) {
    uint32_t bits = At<uint32_t>(projectile, 0x23C);
    if ((bits & 7) < 5) {
        At<uint32_t>(projectile, 0x23C) = (((bits + 1) ^ bits) & 7) ^ bits;
    }
}

// 0x4D39B0: the projectile hit something (`other`, at the colpoint): it goes off when
// its data says so for what it hit, bounce sounds, and its landing anim
void __cdecl Projectile_OnCollision(uint8_t* projectile, uint8_t* other, uint8_t* colPoint, int playSound) {
    int index = At<int32_t>(projectile, 0x238);
    CProjectileInfo* info = &gaProjectileInfo[index];
    if (info->data[0x40]) {
        Stimuli_Add(gStimuli, At<int32_t>(info->data, 0x44), projectile, nullptr, nullptr, -1.0f, -1.0f);
    }
    uint8_t* data = info->data;
    bool explode = false;
    if (data[0x8C] || data[0x8D] || data[0x8E]) {
        if (data[0x8C]) {
            explode = true;
        } else {
            // +0x8D: on hitting a floor, not a ped
            bool onImpact = true;
            if (data[0x8D] && (0.5f > At<float>(colPoint, 0x18) || EntityType(other) == 3)) {
                onImpact = false;
            }
            if (data[0x8E]) {
                // +0x8E: on hitting a ped or an occupied bike (`other` read without a
                // null check, as the game does)
                uint8_t* bike = nullptr;
                if (other && EntityType(other) == 2) {
                    bike = At<int32_t>(other, 0x46C) == 1 ? other : nullptr;
                }
                if (EntityType(other) == 3) {
                    explode = true;
                } else if (bike && Seats_GetOccupant(bike + 0x1C4, 0)) {
                    explode = true;
                }
            } else {
                explode = onImpact;
            }
        }
    }
    if (explode) {
        uint8_t hasSurface = 0;
        // (uninitialised stack in the game when there's no colpoint)
        int32_t surface = 0;
        if (colPoint) {
            hasSurface = 1;
            surface = colPoint[0x22];
        }
        CVector offset = {0.0f, 0.0f, 0.05f};
        CVector pos;
        // (the colpoint read without a null check, as the game does)
        CVector* at = VectorAdd(&pos, reinterpret_cast<const CVector*>(colPoint), &offset);
        CVector normal = At<CVector>(colPoint, 0x10);
        Projectile_Explode_(index, hasSurface, reinterpret_cast<const uint8_t*>(&surface), *at, normal);
        if (other && 0.0f < info->unknown1C) {
            uint8_t type = EntityType(other);
            if (type == 4 || type == 6 || surface == 15) {
                bool exempt = At<int16_t>(projectile, 0x10E) == gModelBF4414 &&
                              (At<int16_t>(other, 0x10E) == gModelBF4184 || At<int16_t>(other, 0x10E) == gModelBF3E60);
                if (!exempt) {
                    Stimuli_Add(gStimuli, 2, info->source, nullptr, reinterpret_cast<const CVector*>(colPoint),
                                At<float>(info->data, 0x94), -1.0f);
                }
            }
        }
    }
    if (At<int16_t>(projectile, 0x10E) == gModelUnequip) {
        At<uint32_t>(projectile, 0x23C) &= ~7u;
    }
    if (Ptr(info->data, 0xC0)) {
        if (re::Flag(playSound)) {
            CountBounce(projectile);
            int id = SoundStreams_GetId(gSoundStreams, reinterpret_cast<const char*>(Ptr(info->data, 0xC0)));
            int surface = colPoint ? colPoint[0x22] : 0;
            SoundStreams_59BBD0(gSoundStreams, id, colPoint, surface, At<int32_t>(info->data, 0xC4),
                                At<uint32_t>(projectile, 0x23C) & 7);
        }
    } else if (At<int16_t>(projectile, 0x10E) == gModelBall && other && EntityType(other) != 6) {
        CountBounce(projectile);
        int surface = colPoint ? colPoint[0x22] : 0;
        const char* name = nullptr;
        if (GetGameMode() == 8) {
            name = "BsBallMiss";
        } else if (GetGameMode() == 13) {
            name = "DunkBallMiss";
        }
        if (name) {
            int id = SoundStreams_GetId(gSoundStreams, name);
            if (id) {
                SoundStreams_59BBD0(gSoundStreams, id, colPoint, surface, At<int32_t>(info->data, 0xC4),
                                    At<uint32_t>(projectile, 0x23C) & 7);
            }
        }
    }
    // (the colpoint read without a null check, as the game does)
    if (!info->landed && 0.5f < At<float>(colPoint, 0x18)) {
        PlayProjectileAnim(projectile, info->data, info->animFlag, 0);
        info->landed = 1;
    }
}

// 0x4D3DE0: the projectile's trail given its point, at `alpha`
void __fastcall ProjectileInfo_UpdateTrail(CProjectileInfo* self, re::Edx, uint8_t* projectile, float alpha) {
    uint8_t* data = self->data;
    if (!(0.0f < At<float>(data, 0x18))) {
        return;
    }
    const CVector* pos = EntityPos(projectile);
    // (the fourth float is uninitialised stack in the game)
    Trails_AddPoint(gTrails, self->trail, CVector4{pos->x, pos->y, pos->z, 0.0f}, At<float>(data, 0x18));
    Trails_SetAlpha(gTrails, self->trail, alpha);
}

// 0x4D3E90: every projectile slot cleared (the trail remnants forgotten, not removed)
void __cdecl ProjectileInfo_Initialise() {
    for (int i = 0; i < NUM_PROJECTILES; i++) {
        re::Ptr32<uint8_t>* ref = &ms_apProjectile[i];
        if (*ref) {
            CleanUpOldReference(*ref, ref);
            *ref = nullptr;
        }
        CProjectileInfo* info = &gaProjectileInfo[i];
        info->weaponType = -1;
        if (info->source) {
            CleanUpOldReference(info->source, &info->source);
            info->source = nullptr;
        }
        info->startTime = 0;
        info->fuse = 0;
        info->unknown10 = 0;
        info->inUse = 0;
        AE_Proxy_Kill(&info->effect);
        info->unknown2C = 0;
        gTrailRemnants[i] = 0;
    }
    gNumTrailRemnants = 0;
}

// 0x4D3F10: the projectile gone (with `explode`, going off where it is): its effect,
// its trail left as a remnant, its entity deleted (or left to be)
void __cdecl Projectile_Destroy(int index, int explode) {
    re::Ptr32<uint8_t>* ref = &ms_apProjectile[index];
    CProjectileInfo* info = &gaProjectileInfo[index];
    uint8_t* projectile = *ref;
    if (!info->inUse) {
        return;
    }
    info->inUse = 0;
    if (info->effect.handle) {
        AE_Proxy_668C40(&info->effect, 0.0f);
        if (info->data[0x145]) {
            AE_Proxy_Kill(&info->effect);
        } else {
            AE_Proxy_SlowKill(&info->effect, 1.0f);
        }
        if (info->effect.handle) {
            AE_Effects_Release(gEffects, &info->effect);
        }
    }
    if (!projectile) {
        return;
    }
    AddTrailRemnant_(info->trail, 1000, EntityPos(projectile));
    info->trail = 0;
    if (re::Flag(explode)) {
        CVector normal = {0.0f, 0.0f, 1.0f};
        CVector offset = {0.0f, 0.0f, 0.05f};
        CVector centre;
        CVector pos;
        CVector* at = VectorAdd(&pos, Entity_GetBoundCentre(projectile, &centre), &offset);
        // (the surface pointed to is uninitialised stack in the game; unread without one)
        int32_t surface = 0;
        Projectile_Explode_(index, 0, reinterpret_cast<const uint8_t*>(&surface), *at, normal);
    }
    bool deleteLater = (At<int32_t>(projectile, 0x28) == 0 && At<int32_t>(projectile, 0xAC) == 0) ||
                       At<int32_t>(projectile, 0xB0) != 0;
    if (gUnknownC1AE83 && deleteLater) {
        At<int32_t>(projectile, 0xD0) = 1;
        At<int32_t>(projectile, 0x78) = 1;
    } else {
        World_Remove(projectile);
        // the deleting destructor (vtable slot 0)
        using DeletingDtor = void(__thiscall*)(void*, int);
        re::GameVirtual<DeletingDtor>(projectile, 0)(projectile, 1);
    }
    if (*ref) {
        CleanUpOldReference(*ref, ref);
        *ref = nullptr;
    }
}

// 0x4D40B0: the projectile in flight longest destroyed (for room); its index, -1: none
int __cdecl Projectile_DestroyOldest() {
    uint32_t oldestTime = 0xFFFFFFFFu;
    int oldest = -1;
    for (int i = 0; i < NUM_PROJECTILES; i++) {
        CProjectileInfo* info = &gaProjectileInfo[i];
        if (info->inUse && info->startTime < oldestTime) {
            oldest = i;
            oldestTime = info->startTime;
        }
    }
    if (oldest != -1) {
        Projectile_Destroy_(oldest, 0);
    }
    return oldest;
}

// the sector range a coordinate falls in, as the game clamps it (through floats)
static int SectorLow(float v) {
    float f = float(int(v / 50.0f + 18.0f));
    return int(f > 0.0f ? f : 0.0f);
}
static int SectorHigh(float v) {
    float f = float(int(v / 50.0f + 18.0f));
    return int(f < 35.0f ? f : 35.0f);
}

// 0x4D4140: the index of the nearest projectile in flight within `radius` of the point
// (of the model, -1 any; slower than `maxSpeed`; from `source` unless nullptr; with
// `armedOnly` only those armed long enough), -1: none. Through the sectors' first lists
inline int Projectile_FindNearestImpl(float x, float y, float z, float radius, float maxSpeed, int model, void* source,
                                      int armedOnly) {
    uint8_t* best = nullptr;
    float bestDistanceSq = 1e21f;
    float xMax = x + radius;
    float yMin = y - radius;
    float yMax = y + radius;
    float xMin = x - radius;
    int sx0 = SectorLow(xMin);
    int sy0 = SectorLow(yMin);
    int sx1 = SectorHigh(xMax);
    int sy1 = SectorHigh(yMax);
    if (gScanCode < 0xFFFF) {
        gScanCode += 1;
    } else {
        ClearScanCodes();
        gScanCode = 1;
    }
    if (sy0 > sy1) {
        return -1;
    }
    CVector point = {x, y, z};
    for (int sy = sy0; sy <= sy1; sy++) {
        if (sx0 > sx1) {
            continue;
        }
        for (int sx = sx0; sx <= sx1; sx++) {
            PtrNode* node = *reinterpret_cast<re::Ptr32<PtrNode>*>(gSectorLists + (sy * 36 + sx) * 0x14);
            while (node) {
                uint8_t* entity = GetEntityInPool(node->bits & 0xF, (node->bits >> 4) & 0x3FFF);
                uint32_t next = node->bits >> 18;
                node = next == 0x3FFF ? nullptr : &gPtrNodes[next];
                if (model != -1 && model != At<int16_t>(entity, 0x10E)) {
                    continue;
                }
                if (At<uint16_t>(entity, 0x10A) == gScanCode) {
                    continue;
                }
                At<uint16_t>(entity, 0x10A) = gScanCode;
                if (At<int32_t>(entity, 0x78) != 0 || At<int32_t>(entity, 0xC4) == 0) {
                    continue;
                }
                CProjectileInfo* info = &gaProjectileInfo[At<int32_t>(entity, 0x238)];
                if (!info->inUse) {
                    continue;
                }
                const CVector& speed = At<CVector>(entity, 0x124);
                float speedSq = speed.y * speed.y + speed.x * speed.x + speed.z * speed.z;
                if (!(speedSq < maxSpeed * maxSpeed)) {
                    continue;
                }
                CVector d;
                CVector* delta = VectorSub(&d, EntityPos(entity), &point);
                float distanceSq = delta->y * delta->y + delta->x * delta->x + delta->z * delta->z;
                if (!(bestDistanceSq > distanceSq)) {
                    continue;
                }
                if (re::Flag(armedOnly)) {
                    int32_t armTime = At<int32_t>(info->data, 0x88);
                    if (info->fuse != 0 && armTime != 0 && int32_t(info->startTime - gTimer + info->fuse) < armTime) {
                        continue;
                    }
                    if (!info->data[0x34]) {
                        continue;
                    }
                }
                if (source == nullptr || source == info->source) {
                    bestDistanceSq = distanceSq;
                    best = entity;
                }
            }
        }
    }
    if (!best) {
        return -1;
    }
    if (bestDistanceSq > radius * radius) {
        return -1;
    }
    return At<int32_t>(best, 0x238);
}
int __cdecl Projectile_FindNearest(float x, float y, float z, float radius, float maxSpeed, int model, void* source,
                                   int armedOnly) {
    return Projectile_FindNearestImpl(x, y, z, radius, maxSpeed, model, source, armedOnly);
}

// 0x4D4500: the projectiles within `radius` of the point destroyed, going off
void __cdecl Projectile_DestroyInSphere(float x, float y, float z, float radius, int model, void* source) {
    int index = Projectile_FindNearest_(x, y, z, radius, 1e11f, model, source, 0);
    while (index != -1) {
        Projectile_Destroy_(index, 1);
        index = Projectile_FindNearest_(x, y, z, radius, 1e11f, model, source, 0);
    }
}

// 0x4D4590: each projectile moved (vtable slot 13) unless its next step leaves the world
// (it's then destroyed)
void __cdecl Projectile_ProcessAll() {
    for (int i = 0; i < NUM_PROJECTILES; i++) {
        uint8_t* projectile = ms_apProjectile[i];
        if (!gaProjectileInfo[i].inUse || !projectile) {
            continue;
        }
        float step = gTimeStep;
        float sx = At<float>(projectile, 0x124) * step;
        float sy = At<float>(projectile, 0x128) * step;
        float sz = step * At<float>(projectile, 0x12C);
        float scale = At<float>(projectile, 0x154);
        CVector move = {sx * scale, sy * scale, scale * sz};
        CVector next;
        CVector* at = VectorAdd(&next, EntityPos(projectile), &move);
        float nx = at->x;
        float ny = at->y;
        float nz = at->z;
        if (!(-900.0f < nx) || !(900.0f > nx) || !(ny > -900.0f) || !(ny < 900.0f) || nz < -30.0f) {
            Projectile_Destroy_(i, 0);
            continue;
        }
        using ProcessFn = void(__thiscall*)(void*, int);
        re::GameVirtual<ProcessFn>(projectile, 13)(projectile, 1);
        Entity_443AA0(projectile);
        Entity_465760(projectile);
    }
}

// 0x4D46E0: a free projectile slot (the oldest projectile destroyed when there's none)
int __cdecl Projectile_FindFreeSlot() {
    int index = 0;
    for (; index < NUM_PROJECTILES; index++) {
        if (!gaProjectileInfo[index].inUse) {
            break;
        }
    }
    if (index == NUM_PROJECTILES) {
        index = Projectile_DestroyOldest_();
    }
    return index;
}

// 0x4D4730: each projectile's step: its thrower forgotten when dead, into water (a
// splash, maybe going off, gone), its fuse burnt (gone off), else its effect moved to
// its tail, its fade and its trail; then the trail remnants
void __cdecl Projectile_UpdateAll() {
    for (int index = 0; index < NUM_PROJECTILES; index++) {
        CProjectileInfo* info = &gaProjectileInfo[index];
        uint8_t* projectile = ms_apProjectile[index];
        if (!info->inUse) {
            continue;
        }
        if (info->source && EntityType(info->source) == 3 && !Ped_IsAlive(info->source) && info->source) {
            CleanUpOldReference(info->source, &info->source);
            info->source = nullptr;
        }
        if (!projectile) {
            info->inUse = 0;
            continue;
        }
        CVector centre;
        Entity_GetBoundCentre(projectile, &centre);
        float waterLevel;
        int32_t waterType;
        if (Water_GetLevel(gWater, &waterLevel, &waterType, &centre)) {
            Projectile_Splash_(&centre, waterLevel, waterType);
            if (info->data[0x8C] || info->data[0x8D]) {
                CVector normal = {0.0f, 0.0f, 1.0f};
                CVector offset = {0.0f, 0.0f, 0.05f};
                CVector pos;
                CVector* at = VectorAdd(&pos, &centre, &offset);
                // (the surface pointed to is uninitialised stack in the game; unread)
                int32_t surface = 0;
                Projectile_Explode_(index, 0, reinterpret_cast<const uint8_t*>(&surface), *at, normal);
            }
            Projectile_Destroy_(index, 0);
            continue;
        }
        if (info->exploded || (gTimer > info->startTime + info->fuse && info->fuse != 0)) {
            Projectile_Destroy_(index, 1);
            continue;
        }
        if (info->effect.handle) {
            CVector pos = *EntityPos(projectile);
            uint8_t* rwObject = Ptr(projectile, 0x18);
            if (rwObject && Ptr(rwObject, 4)) {
                if (uint8_t* transform = static_cast<uint8_t*>(Frame_GetTransform(Ptr(rwObject, 4)))) {
                    // (a tenth of its frame's up axis further on)
                    CVector axis = {At<float>(transform, 0x8), At<float>(transform, 0x14), At<float>(transform, 0x20)};
                    Vector_NormaliseSafe(&axis);
                    ScaleVector(&axis, &axis, 0.1f);
                    AddVectors(&pos, &pos, &axis);
                }
            }
            CVector effectPos = pos;
            AE_Proxy_SetPosition(&info->effect, &effectPos, 0);
        }
        int alpha = 0xFF;
        uint8_t* rwObject = Ptr(projectile, 0x18);
        if (rwObject && info->fuse) {
            int32_t fadeTime = At<int32_t>(info->data, 0x88);
            if (fadeTime) {
                int32_t left = int32_t(info->startTime - gTimer + info->fuse);
                if (left < fadeTime) {
                    alpha = left * 0xFF / fadeTime;
                    if (alpha < 0) {
                        alpha = 0;
                    }
                    RwObject_SetAlpha(rwObject, alpha);
                }
            }
        }
        ProjectileInfo_UpdateTrail_(info, projectile, float(alpha) * 0.003921569f);
    }
    UpdateTrailRemnants_();
}

// 0x4D49F0: the projectiles of the model destroyed
void __cdecl Projectile_DestroyOfModel(int model) {
    for (int i = 0; i < NUM_PROJECTILES; i++) {
        if (ms_apProjectile[i] && At<int16_t>(ms_apProjectile[i], 0x10E) == model) {
            Projectile_Destroy_(i, 0);
        }
    }
}

// 0x4D4A30: the projectiles whose weapon's butes don't keep them (+0x88, its +0x2D)
// destroyed
void __cdecl Projectile_DestroyNotKept() {
    for (int i = 0; i < NUM_PROJECTILES; i++) {
        if (!ms_apProjectile[i]) {
            continue;
        }
        uint8_t* weaponInfo = WeaponInfo_Get(At<int16_t>(ms_apProjectile[i], 0x10E));
        Butes_Resolve(weaponInfo);
        if (Ptr(weaponInfo, 0x88)) {
            Butes_Resolve(weaponInfo);
            if (!Ptr(weaponInfo, 0x88)[0x2D]) {
                continue;
            }
        }
        Projectile_Destroy_(i, 0);
    }
}

// 0x4D4A90
void __cdecl Projectile_DestroyAll() {
    for (int i = 0; i < NUM_PROJECTILES; i++) {
        Projectile_Destroy_(i, 0);
    }
}

// 0x4D4AB0: the entity's projectile slots destroyed; always 0
uint8_t __cdecl Projectile_DestroyEntity(uint8_t* entity) {
    for (int i = 0; i < NUM_PROJECTILES; i++) {
        if (ms_apProjectile[i] == entity) {
            Projectile_Destroy_(i, 0);
        }
    }
    return 0;
}

// the projectile's data: the given one, its model's butes' (+0x18, its +0x80), else the
// default
static uint8_t* ModelProjectileData(int model) {
    uint8_t* butes = Ptr(gModelInfoPtrs[model], 0x18);
    if (butes) {
        Butes_Resolve(butes);
        if (uint8_t* data = Ptr(butes, 0x80)) {
            return data;
        }
    }
    return gDefaultProjectileData;
}

// the projectile given its slot: registered in it and its info reset
static void SetSlotProjectile(int index, uint8_t* projectile) {
    re::Ptr32<uint8_t>* ref = &ms_apProjectile[index];
    uint8_t* old = *ref;
    if (projectile != old) {
        if (old) {
            CleanUpOldReference(old, ref);
        }
        *ref = projectile;
        RegisterReference(projectile, ref);
    }
}

// 0x4D4AF0: a projectile of the model placed (not thrown) at the point, rotated by the
// angles: no thrower, no speed; its launch anim. nullptr: no room
uint8_t* __cdecl Projectile_Place(int model, CVector4 pos, CVector4 angles) {
    int index = Projectile_FindFreeSlot_();
    if (index == -1) {
        return nullptr;
    }
    void* memory = Projectile_New(0x24C);
    uint8_t* projectile = memory ? Projectile_Construct(memory, model, index) : nullptr;
    if (!projectile) {
        return nullptr;
    }
    uint8_t* weaponInfo = WeaponInfo_Get(model);
    // (here the weapon info itself when the model has no butes, unlike Projectile_Add)
    uint8_t* data = weaponInfo;
    uint8_t* butes = Ptr(gModelInfoPtrs[model], 0x18);
    if (butes) {
        Butes_Resolve(butes);
        data = Ptr(butes, 0x80);
    }
    if (!data) {
        data = gDefaultProjectileData;
    }
    uint8_t flags = data[0x7C] ? 7 : 6;
    Matrix_SetRotate(reinterpret_cast<CMatrix*>(Entity_GetMatrix(projectile)), reinterpret_cast<const CVector*>(&angles));
    float raise = At<float>(weaponInfo, 0x44);
    float x = pos.x + 0.0f;
    float y = pos.y + 0.0f;
    float z = pos.z + raise;
    float* matrix = Entity_GetMatrix(projectile);
    matrix[12] = x;
    matrix[13] = y;
    matrix[14] = z;
    // (the game also writes matrix[15] from uninitialised stack; left as it is)
    At<CVector>(projectile, 0x124) = gZeroVector;
    At<CVector>(projectile, 0x130) = gZeroVector;
    projectile[0x1BC] |= 2;
    At<float>(projectile, 0x164) = At<float>(data, 0x5C);
    projectile[0x20A] = flags;
    At<float>(projectile, 0x244) = 0.0f;
    At<float>(projectile, 0x230) = At<float>(data, 0x80);
    At<int32_t>(projectile, 0xB0) = 1;
    World_Add(projectile);
    SetSlotProjectile(index, projectile);
    CProjectileInfo* info = &gaProjectileInfo[index];
    info->weaponType = model;
    info->unknown15 = 0;
    if (info->source) {
        CleanUpOldReference(info->source, &info->source);
        info->source = nullptr;
    }
    info->startTime = gTimer;
    info->unknown1C = 0.0f;
    info->fuse = 0;
    info->unknown10 = At<int32_t>(data, 0xF0);
    info->inUse = 1;
    info->exploded = 0;
    info->animFlag = 0;
    info->landed = 1;
    AE_Proxy_Kill(&info->effect);
    AE_Proxy_SetPosition(&info->effect, reinterpret_cast<const CVector*>(&pos), 0);
    info->data = data;
    info->trail = -1;
    uint8_t* clump = Ptr(projectile, 0x114);
    if (clump && Ptr(clump, 0x94) && Ptr(Ptr(clump, 0x94), 0x10)) {
        void* dict = Ptr(Ptr(clump, 0x94), 0x10);
        const char* anim = reinterpret_cast<const char*>(Ptr(data, 0x134));
        int32_t animIndex = -1;
        if (anim) {
            AnimDict_Find(dict, anim, 0, &animIndex);
            if (animIndex != -1) {
                AnimManager_Play(gAnimManager, clump, animIndex, Clump_Fn6B9380(clump, 0), 0, 1.0f, -1.0f);
            }
        }
    }
    return projectile;
}

// 0x4D4D90: a projectile of the model thrown by `thrower` from `pos` along `dir` (its
// speed from the data when it has one, +0xF4), with its effect, trail, stimuli, sound and
// launch anim. nullptr: no room
uint8_t* __cdecl Projectile_Add(uint8_t* thrower, int model, CVector pos, CVector dir, uint8_t* data, float unknown1C,
                                float unknown244, void* effect, uint32_t fuseStart, uint32_t fuse, CVector angles,
                                CVector turnSpeed, void* target, int animFlag) {
    int index = Projectile_FindFreeSlot_();
    if (index == -1) {
        return nullptr;
    }
    void* memory = Projectile_New(0x24C);
    if (!memory) {
        return nullptr;
    }
    uint8_t* projectile = Projectile_Construct(memory, model, index);
    if (!projectile) {
        return nullptr;
    }
    if (!data) {
        data = ModelProjectileData(model);
    }
    Stimuli_Add(gStimuli, 0, thrower, nullptr, &pos, At<float>(data, 0x38), At<float>(data, 0x3C));
    if (fuse != 0 || (fuse = At<uint32_t>(data, 0x48)) != 0) {
        fuse -= fuseStart;
        if (fuse < 1) {
            fuse = 1;
        }
    }
    uint8_t flags = data[0x7C] ? 7 : 6;
    bool level = model == gModelLevel1 || model == gModelLevel2 || model == gModelLevel3 || model == gModelLevel4;

    // its matrix: forward along the direction (level for the four), right = forward x z,
    // up = right x forward (straight up: fixed axes). The fourth floats of the game's
    // matrix are uninitialised stack
    CMatrix m = {};
    m.attached = nullptr;
    if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 1.0f) {
        m.forward = CVector4{dir.x, dir.y, dir.z, 0.0f};
        m.right.x = 1.0f;
        m.right.y = 0.0f;
        m.right.z = 0.0f;
        m.up.x = 0.0f;
        m.up.y = 1.0f;
        m.up.z = 0.0f;
    } else if (level) {
        m.forward = CVector4{dir.x, dir.y, 0.0f, 0.0f};
        float zero = 0.0f * 0.0f;
        m.right.x = dir.y - zero;
        m.right.y = zero - dir.x;
        m.right.z = dir.x * 0.0f - dir.y * 0.0f;
        m.up.x = 0.0f;
        m.up.y = 0.0f;
        m.up.z = 1.0f;
    } else {
        m.forward = CVector4{dir.x, dir.y, dir.z, 0.0f};
        float zz = dir.z * 0.0f;
        m.right.x = dir.y - zz;
        m.right.y = zz - dir.x;
        m.right.z = dir.x * 0.0f - dir.y * 0.0f;
        const CVector4& r = m.right;
        const CVector4& f = m.forward;
        m.up.x = r.y * f.z - r.z * f.y;
        m.up.y = r.z * f.x - r.x * f.z;
        m.up.z = r.x * f.y - r.y * f.x;
    }
    m.pos = CVector4{pos.x, pos.y, pos.z, 0.0f};
    CVector velocity = dir;
    if (At<int32_t>(data, 0xF0) > 0) {
        // the direction normalised (in place in the game's argument), times the speed
        float length = std::sqrt(std::fabs(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z));
        Vector_Normalise(&dir);
        float speed = At<float>(data, 0xF4);
        float vx = dir.x * length;
        float vy = dir.y * length;
        float vz = length * dir.z;
        velocity.x = vx * speed;
        velocity.y = vy * speed;
        velocity.z = speed * vz;
        // (a little less up)
        velocity.z = velocity.z - length * At<float>(data, 0xF4) * 0.05f;
    }
    if (gUnknown1B65C14) {
        // aimed by angles: the matrix from the direction flattened, rotated by them
        float zz = dir.z * 0.0f;
        CVector side = {dir.y - zz, zz - dir.x, dir.x * 0.0f - dir.y * 0.0f};
        float sz = side.z * 0.0f;
        CVector along = {sz - side.y, side.x - sz, side.y * 0.0f - side.x * 0.0f};
        Vector_Normalise2(&side);
        Vector_Normalise2(&along);
        m.right.x = side.x;
        m.right.y = side.y;
        m.right.z = side.z;
        m.forward = CVector4{along.x, along.y, along.z, 0.0f};
        m.up = CVector4{0.0f, 0.0f, 1.0f, 0.0f};
        m.pos = CVector4{0.0f, 0.0f, 0.0f, 0.0f};
        CMatrix rotation;
        rotation.attached = nullptr;
        Matrix_SetRotate(&rotation, &angles);
        Matrix_Multiply(&m, &rotation);
        m.pos.x = pos.x;
        m.pos.y = pos.y;
        m.pos.z = pos.z;
    }
    // then by the weapon's rotation (degrees)
    uint8_t* weaponInfo = WeaponInfo_Get(model);
    CMatrix rotation;
    rotation.attached = nullptr;
    CVector rotationAngles = {float(At<int32_t>(weaponInfo, 0x38)) * 3.1415927f / 180.0f,
                              float(At<int32_t>(weaponInfo, 0x3C)) * 3.1415927f / 180.0f,
                              3.1415927f * float(At<int32_t>(weaponInfo, 0x40)) / 180.0f};
    Matrix_SetRotate(&rotation, &rotationAngles);
    Matrix_Multiply(&m, &rotation);
    Placeable_SetMatrix(projectile, &m, 0);
    At<CVector>(projectile, 0x130) = turnSpeed;
    At<CVector>(projectile, 0x124) = velocity;
    projectile[0x1BC] = uint8_t((projectile[0x1BC] & ~2) | (gUnknown1B65C14 == 0 ? 2 : 0));
    At<float>(projectile, 0x164) = At<float>(data, 0x5C);
    projectile[0x20A] = flags;
    At<float>(projectile, 0x244) = unknown244;
    At<float>(projectile, 0x230) = At<float>(data, 0x80);
    World_Add(projectile);
    SetSlotProjectile(index, projectile);
    CProjectileInfo* info = &gaProjectileInfo[index];
    info->unknown15 = gUnknown1B65C14;
    info->weaponType = model;
    if (thrower != info->source) {
        if (info->source) {
            CleanUpOldReference(info->source, &info->source);
        }
        info->source = thrower;
        if (thrower) {
            RegisterReference(thrower, &info->source);
        }
    }
    info->unknown1C = unknown1C;
    info->startTime = gTimer;
    info->fuse = fuse;
    info->animFlag = uint8_t(animFlag);
    info->unknown10 = At<int32_t>(data, 0xF0);
    info->inUse = 1;
    info->exploded = 0;
    info->landed = 0;
    AE_Proxy_Kill(&info->effect);
    if (effect) {
        AE_Effects_StartEffect(gEffects, effect, &info->effect);
    }
    if (info->effect.handle) {
        AE_Proxy_SetPosition(&info->effect, &pos, 0);
    }
    info->data = data;
    info->trail = -1;
    if (0.0f < At<float>(data, 0x18)) {
        ProjectileData_Resolve(data);
        info->trail = Trails_Add(gTrails, At<int32_t>(data, 0x30), At<float>(data, 0x2C), At<uint32_t>(data, 0x146), 2);
        ProjectileInfo_UpdateTrail_(info, projectile, 1.0f);
    }
    if (thrower && EntityType(thrower) == 3 && Ptr(thrower, 0x18B4) == nullptr) {
        At<uint8_t*>(thrower, 0x18B4) = projectile;
    }
    float speed = std::sqrt(std::fabs(velocity.y * velocity.y + velocity.x * velocity.x + velocity.z * velocity.z));
    if (At<float>(gUnknown20C88B8, 0x64) < speed) {
        Stimuli_Add(gStimuli, 0xE, projectile, target, &pos, -1.0f, -1.0f);
    }
    if (data[0x130]) {
        Stimuli_Add(gStimuli, 0x30, thrower, projectile, &pos, -1.0f, -1.0f);
    }
    if (const char* sound = reinterpret_cast<const char*>(Ptr(data, 0xAC))) {
        int id = SoundStreams_GetId(gSoundStreams, sound);
        const CVector* at = EntityPos(projectile);
        uint32_t handle = SoundStreams_Play(gSoundStreams, id, at, At<int32_t>(data, 0xB0), at, 0, 0x78, 0, 0, 0, 0, 6, 0,
                                            1.0f, 0);
        Sound_Set(projectile + 0x248, handle);
        if (uint8_t* playing = Ptr(projectile, 0x248)) {
            playing[0x3D] = 0x80;
            At<uint16_t>(Ptr(projectile, 0x248), 0x3E) = 0xFA;
        }
    }
    PlayProjectileAnim(projectile, data, animFlag, 1);
    return projectile;
}

} // namespace projectile_c12

namespace {
using namespace projectile_c12;

PROJECTILE_C12_REVERSED("CExplosion::Initialise", 0x004D2760, &Explosion_Initialise);
PROJECTILE_C12_REVERSED("CExplosion::Process", 0x004D27E0, &Explosion_Process);
PROJECTILE_C12_REVERSED("CExplosion::Update", 0x004D2BF0, &Explosion_Update);
PROJECTILE_C12_REVERSED("CExplosion::AddExplosion", 0x004D2C90, &Explosion_Add);
PROJECTILE_C12_REVERSED("CProjectileInfo::TargetSearch_Begin", 0x004D30B0, &TargetSearch_Begin);
PROJECTILE_C12_REVERSED("CProjectileInfo::TargetSearch_Consider", 0x004D3160, &TargetSearch_Consider);
PROJECTILE_C12_REVERSED("CProjectileInfo::FindPedTarget", 0x004D3370, &TargetSearch_FindPed);
PROJECTILE_C12_REVERSED("CProjectileInfo::FindVehicleTarget", 0x004D33F0, &TargetSearch_FindVehicle);
REVERSED("CPhysical::SetTurnSpeed", 0x004D3480, re::MemberAddress(&CPhysicalC12::SetTurnSpeed), nullptr, nullptr,
         re::Mode::Original);
// (the projectile in eax)
RE_UNHOOKED("CProjectileInfo::PlayProjectileAnim", 0x004D34C0);
PROJECTILE_C12_REVERSED("CProjectileInfo::Splash", 0x004D3570, &Projectile_Splash);
PROJECTILE_C12_REVERSED("CProjectileInfo::AddTrailRemnant", 0x004D3730, &AddTrailRemnant);
PROJECTILE_C12_REVERSED("CProjectileInfo::UpdateTrailRemnants", 0x004D3860, &UpdateTrailRemnants);
PROJECTILE_C12_REVERSED("CProjectileInfo::Explode", 0x004D3920, &Projectile_Explode);
PROJECTILE_C12_REVERSED("CProjectileInfo::OnCollision", 0x004D39B0, &Projectile_OnCollision);
PROJECTILE_C12_REVERSED("CProjectileInfo::UpdateTrail", 0x004D3DE0, &ProjectileInfo_UpdateTrail);
PROJECTILE_C12_REVERSED("CProjectileInfo::Initialise", 0x004D3E90, &ProjectileInfo_Initialise);
PROJECTILE_C12_REVERSED("CProjectileInfo::DestroyProjectile", 0x004D3F10, &Projectile_Destroy);
PROJECTILE_C12_REVERSED("CProjectileInfo::DestroyOldest", 0x004D40B0, &Projectile_DestroyOldest);
PROJECTILE_C12_REVERSED("CProjectileInfo::FindNearest", 0x004D4140, &Projectile_FindNearest);
PROJECTILE_C12_REVERSED("CProjectileInfo::DestroyInSphere", 0x004D4500, &Projectile_DestroyInSphere);
PROJECTILE_C12_REVERSED("CProjectileInfo::ProcessAll", 0x004D4590, &Projectile_ProcessAll);
PROJECTILE_C12_REVERSED("CProjectileInfo::FindFreeSlot", 0x004D46E0, &Projectile_FindFreeSlot);
PROJECTILE_C12_REVERSED("CProjectileInfo::Update", 0x004D4730, &Projectile_UpdateAll);
PROJECTILE_C12_REVERSED("CProjectileInfo::DestroyOfModel", 0x004D49F0, &Projectile_DestroyOfModel);
PROJECTILE_C12_REVERSED("CProjectileInfo::DestroyNotKept", 0x004D4A30, &Projectile_DestroyNotKept);
PROJECTILE_C12_REVERSED("CProjectileInfo::DestroyAll", 0x004D4A90, &Projectile_DestroyAll);
PROJECTILE_C12_REVERSED("CProjectileInfo::DestroyEntity", 0x004D4AB0, &Projectile_DestroyEntity);
PROJECTILE_C12_REVERSED("CProjectileInfo::PlaceProjectile", 0x004D4AF0, &Projectile_Place);
PROJECTILE_C12_REVERSED("CProjectileInfo::AddProjectile", 0x004D4D90, &Projectile_Add);

} // namespace
