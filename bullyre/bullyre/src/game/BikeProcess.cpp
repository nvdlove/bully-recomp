// CBike's per-frame work (vtable 0x912B24): in water (GTA's ProcessBuoyancy), its wheels'
// contacts and springs, Teleport (vf14), PreRender (vf17: skid marks, dust, exhaust smoke,
// lights), its constructor, and the rest of its virtuals of 0x4C12A0..0x4C71D0.
// Fields are reached by offset like the other Bike*.cpp files (CBike's layout in
// Vehicle.h only has a few named). The wheels: +0x5D8 two statuses, +0x5DC / +0x604 their
// CColPoints (0x28 bytes: +0x10 the normal, +0x22 the surface), +0x62C / +0x630 the
// suspension compression (1: fully out), +0x680 / +0x684 and +0x688 / +0x68C the spring
// lengths, +0x668 the spring directions, +0x6CC the offsets on +0x6E4's ground entities.

#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Vehicle.h"
#include "reversed.h"

#include "game/Crt.h"
namespace c11bikeproc {

using veh::CVector;

// a vector the game passes by value with its (unset) fourth float
struct CVector4 {
    float x, y, z, w;
};
static_assert(sizeof(CVector4) == 0x10);

// Gamebryo's NiTransform: a 3x3 rotation, the translation, the scale
struct NiTransform {
    float rotate[9];      // +0
    CVector translate;    // +0x24
    float scale;          // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

// a collision sphere (0x14 bytes)
struct Sphere {
    CVector centre;         // +0x0
    float radius;           // +0xC
    uint8_t surface[4];     // +0x10
};
static_assert(sizeof(Sphere) == 0x14);

// the game's CColPoint (0x28 bytes)
struct ColPoint {
    CVector point;          // +0x0
    re::Ptr32<CEntity> entity;        // +0xC (a registered reference)
    CVector normal;         // +0x10
    uint8_t unknown1C[0x4];
    uint8_t surfaceA;       // +0x20
    uint8_t pieceA;         // +0x21
    uint8_t surfaceB;       // +0x22
    uint8_t pieceB;         // +0x23
    float depth;            // +0x24
};
static_assert(offsetof(ColPoint, entity) == 0xC);
static_assert(offsetof(ColPoint, normal) == 0x10);
static_assert(offsetof(ColPoint, surfaceB) == 0x22);
static_assert(offsetof(ColPoint, depth) == 0x24);
static_assert(sizeof(ColPoint) == 0x28);

// 0x4C12A0: GTA's CBike::ProcessBuoyancy
void __fastcall ProcessBuoyancy(CBike* self, re::Edx edx);
// 0x4C1730 (thiscall, 1 argument): the wheels' contacts: the springs' compression
// rescaled, the contact points (their z scaled by 0.05) and spring directions,
// ProcessSuspension (0x4C09E0), the contact speeds, the springs' damping, the averaged
// ground normal (+0x5A0) and the wheels' "on the ground lately" timers (+0x63C / +0x640)
void __fastcall ProcessWheelContacts(CBike* self, re::Edx edx, CVector* contactPoints);
// vf14 0x4C21D0: GTA's Teleport
void __fastcall Teleport(CBike* self, re::Edx edx, float x, float y, float z);
// vf17 0x4C22D0: GTA's PreRender (sounds, skid marks, wheel dust, exhaust smoke, lights)
void __fastcall PreRender(CBike* self, re::Edx edx);
// vf41 0x4C3C30: GTA's ProcessEntityCollision with `entity` while moving to `matrix`:
// the wheels' lines first (a vehicle or object under a wheel kept with where it
// touches), then the swept test; on a hit the collision is applied and it's put back
// (`keepHeight`: only its height), then a crash sound by the damage (+0x19C). 1: hit
uint8_t __fastcall ProcessEntityCollision(CBike* self, re::Edx edx, const float* matrix, uint8_t* entity,
                                          ColPoint* colPoint, int keepHeight);
// vf49 0x4C43C0: the model's collision spheres rebuilt (eight allocated, five counted)
// from the wheels' frames (4, 5), and its bounding box and sphere copied out. 1
uint8_t __fastcall SetUpWheelColModel(CBike* self, re::Edx edx, uint8_t* colModelOut);
// 0x4C4830 (thiscall, 3 arguments): a hit on `colPoint`'s entity (+0xC) at `speed`: a hard
// one crashes the rider (with `crash` or the player riding); a ped hit is run over or
// shoved ("Bike_Hit" and an event once a second); the player's pad rumbles
void __fastcall ProcessImpact(CBike* self, re::Edx edx, CVector* speed, ColPoint* colPoint, int crash);
// 0x4C4E90 (thiscall, 2 arguments): the wheels' contacts, then the engine (the
// transmission: *thrust) and the brakes (*brake) through the front and rear wheels'
// forces (by the handling's 0x200000 the rear first); nearly stopped with no gas or
// brake, it's stopped
void __fastcall ProcessWheelForces(CBike* self, re::Edx edx, float* thrust, float* brake);
// 0x4C54A0: GTA's PushOutOfGround for its two wheels' spheres (0, 2): lines down from
// each; one in the ground is pushed out along the ground's normal, and a bike lying on
// its side with both wheels off the ground gets a turn force to right it
void __fastcall PushOutOfGround(CBike* self, re::Edx edx);
// vf8 0x4C6010: the model set, its frames, the animation's callback and its bones
void __fastcall SetModelIndex(CBike* self, re::Edx edx, int model, int arg);
// 0x4C6100 (thiscall, 2 arguments): the centre of mass, a riderless bike falling over
// slowed, CPhysical's ProcessControl, in water, the wheels' forces, the lean
void __fastcall ProcessPhysics(CBike* self, re::Edx edx, float* thrust, float* brake);
// vf12 0x4C6340: GTA's ProcessControl: the jump, the pedalling rate (+0x778), then while
// in the world its status, balance, crashes, physics, lean
void __fastcall ProcessControl(CBike* self, re::Edx edx);
// vf13 0x4C6660 (the argument unused): its move for the frame: vf37 / vf38 move it, it's
// kept where the wheel on the ground stood, pushed out of the ground, its collisions
// resolved (impacts, falling off), its speed from how far it went
void __fastcall ProcessCollisions(CBike* self, re::Edx edx, int unused);
// 0x4C6F70: the constructor (CVehicle's first; thiscall, returns this)
CBike* __fastcall Construct(CBike* self, re::Edx edx, int model, int createdBy);

} // namespace c11bikeproc

namespace {

using namespace c11bikeproc;

using re::At;

// a virtual by its byte offset in the vtable
template <typename R, typename... A>
R Virtual(void* object, int offset, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[offset / 4]))(object, args...);
}

GAME_VAR(float, timeStep, 0x00C1A9A4);        // CTimer's
GAME_VAR(CEntity*, playerPed, 0x00C1AEA8);    // (FindPlayerPed's)
GAME_VAR(CVector, vec9E33DC, 0x009E33DC);     // (a direction: gravity's? the ground normal's default)
GAME_VAR(CVector, vecUnset, 0x00BCC564);      // (a zero vector, also "unset")
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, currArea, 0x00BD1008);      // CGame's
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);   // (by chapter: 2 winter)
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, clockMinutes, 0x00BCEB4A);
GAME_VAR(float, lightsFogA, 0x00CE755C);      // (lights on in fog/rain above a vehicle's own level)
GAME_VAR(float, lightsFogB, 0x00CE7564);
GAME_VAR(float, speedScale, 0x00C2D194);      // (the move speed along it divided by this: compared with 10 and 130)
GAME_VAR(void*, texShadowC93728, 0x00C93728); // (the cars' taillight shadow; a bike uses it for both of its own)
GAME_VAR(void*, cameraObject, 0x00C3CCF4);    // (TheCamera's Gamebryo camera)
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const buoyancy = reinterpret_cast<void*>(0x00C2DB58);    // (mod_Buoyancy)
inline void* const animManager = reinterpret_cast<void*>(0x020C4910);
inline void* const trails = reinterpret_cast<void*>(0x01B850E0);
inline void* const actionNode1B652C4 = reinterpret_cast<void*>(0x01B652C4);

inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(CPed*, __thiscall, Seats_GetOccupant, 0x004CE560, CVehicleSeats* seats, uint32_t seat);
GAME_FN(void, __thiscall, Seats_RemoveDriver, 0x004CE960, CVehicleSeats* seats);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, CVehicle* vehicle, int32_t* out);
GAME_FN(void, __thiscall, Vehicle_SetCreatedBy, 0x004CB230, CVehicle* vehicle, int createdBy);
GAME_FN(uint8_t, __thiscall, ActionController_IsPlaying, 0x005F4800, void* controller, void* node, int a, int b);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(CVector*, __thiscall, Entity_GetPosition, 0x00415460, void* entity);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVector* out);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(uint8_t, __thiscall, Buoyancy_Process, 0x004C87C0, void* buoyancy, void* entity, float value, CVector* outCentre,
        CVector* outForce);
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0046A7E0, void* physical, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, void* physical, float fx, float fy, float fz, float px,
        float py, float pz);
GAME_FN(void, __thiscall, Physical_ApplySpringDampening, 0x0046C3C0, void* physical, float damping, CVector* springDir,
        CVector* point, CVector* speed);
GAME_FN(void, __thiscall, Placeable_SetOrientation, 0x0046A690, void* entity, float x, float y, float z);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(CVector*, __thiscall, Vector_ScaleAssign, 0x00413A20, CVector* self, CVector* out, float s);
GAME_FN(uint8_t, __cdecl, Vector_Equals, 0x00417870, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, Multiply3x3, 0x0041C490, CVector* out, const float* m, const CVector* v);
GAME_FN(CVector*, __cdecl, MultiplyMatrixVector, 0x0045D4A0, CVector* out, const void* m, const CVector* v);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline void Matrix_MultiplyPoint4(CVector4* out, const void* m, const CVector4* v) { re::CallVec16<0x0045D500, void>(out, m, v); }
GAME_FN(void, __thiscall, Matrix_FromTransform, 0x00412A70, veh::CMatrix* m, const NiTransform* t);
GAME_FN(void, __thiscall, Matrix_CopyOnlyMatrix, 0x00412720, veh::CMatrix* m, const void* from);
GAME_FN(void, __cdecl, Transform_SetRotation, 0x00415EB0, NiTransform* t, const CVector* axis, float angle, int degrees);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);
GAME_FN(float, __cdecl, RecipSqrt, 0x00414C20, float a, float b);   // (a / sqrt |b|)
GAME_FN(float, __cdecl, RandomFloatInRange, 0x0041A5B0, float lo, float hi);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(CVector*, __thiscall, Camera_GetPosition, 0x0041AAC0, void* camera);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
// the animation data's nodes (+0x114's +0x94)
GAME_FN(void*, __thiscall, Nodes_FindByName, 0x004A6BB0, void* nodes, const char* name, uintptr_t arg);
GAME_FN(void, __thiscall, AnimManager_GetNodePosition, 0x006C1190, void* manager, void* animData, int arg, CVector* out,
        void* node);
GAME_FN(void, __thiscall, AnimManager_GetNodeTransform, 0x006BF570, void* manager, void* animData, int arg,
        NiTransform* out, void* node);
// effects
GAME_FN(void, __thiscall, Trails_AddPoint_x64, 0x0066CD10, void* trails, int id, float, float, float, float, float width);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Trails_AddPoint(void* a0, int a1, CVector4 a2, float a3) {
    return Trails_AddPoint_x64(a0, a1, re::FloatAt(a2, 0), re::FloatAt(a2, 1), re::FloatAt(a2, 2), re::FloatAt(a2, 3), a3);
}
GAME_FN(void, __cdecl, Particles_Add, 0x00521080, int kind, const CVector* pos, const CVector* speed, float size,
        const uint8_t* colour, int a6, int a7);
GAME_FN(void, __cdecl, Fn5280C0, 0x005280C0, void* id, void* wheel, float x, float y, int surface, int);
GAME_FN(void, __cdecl, Fn58C940, 0x0058C940, CVehicle* vehicle, int surface, float arg);   // (wheel effects on a surface)
GAME_FN(void, __cdecl, Coronas_Register, 0x005124D0, uintptr_t id, int r, int g, int b, int a, const CVector* pos,
        float size, float range, int type, int a10, int a11, int a12, float a13, int a14, int a15, float a16);
GAME_FN(void, __cdecl, Coronas_Off, 0x00511480, uintptr_t id, const CVector* pos, float range, float angle);
GAME_FN(void, __cdecl, LightShadow_Add, 0x00528800, float px, float py, float pz, float ux, float uy, float uz, float rx,
        float ry, float rz, float fx, float fy, float fz, int kind, int a14, int a15, int a16);
GAME_FN(void, __cdecl, Shadows_StoreCarLight, 0x00527280, CVehicle* vehicle, uintptr_t id, void* texture,
        const CVector* pos, float fwdX, float fwdY, float sideX, float sideY, int r, int g, int b, float maxDist);
GAME_FN(void, __cdecl, PointLights_Add, 0x00522DB0, int type, float px, float py, float pz, float dx, float dy, float dz,
        float radius, float r, float g, float b, int fog, int shadows);

// CBike's own, called where they are (so bullyre.ini decides which runs)
GAME_FN(CPed*, __thiscall, Game_KnockOffRider, 0x004BA050, CBike* bike, int kind, const void* arg, CPed* ped, int unused);
GAME_FN(void, __thiscall, Game_ProcessSuspension, 0x004C09E0, CBike* bike, CVector* points);
GAME_FN(void, __thiscall, Game_UpdateBrakeSound, 0x004BCCE0, CBike* bike);
GAME_FN(void, __thiscall, Game_UpdateSurfaceSounds, 0x004BBB10, CBike* bike);
GAME_FN(void, __thiscall, Game_UpdateRiderMatrix, 0x004C0200, CBike* bike);
// (0x4B9CE0: a named node's position; BikeCore.cpp calls it PlayAnim)
GAME_FN(void, __thiscall, Game_GetNodePosition, 0x004B9CE0, CBike* bike, const char* name, CVector* out);

// collisions
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);     // CTimer's
GAME_VAR(uint8_t, flagA35739, 0x00A35739);    // (the wheels' lines tested against entities)
GAME_VAR(int32_t, ignoredPedModel, 0x00BF42DC);
GAME_VAR(uint32_t, lastCrashSound, 0x00C2D1A0);
inline void* const audioFocusPlayer = reinterpret_cast<void*>(0x00CF3DA0);
GAME_FN(void*, __cdecl, GetColModel, 0x0051AF50, void* entity);
GAME_FN(float*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(void, __cdecl, Col_ProcessWheels, 0x00569DC0, const float* matrix, void* colModel, const float* otherMatrix,
        void* otherColModel, void* wheelColPoints, float* wheelCompressions);
GAME_FN(int32_t, __cdecl, Col_ProcessSweep, 0x0056C8C0, const float* to, const float* from, void* colModel,
        const float* matrixA, const float* matrixB, void* colModel2, ColPoint* colPoint, int, int, int, int, int,
        int lists, uintptr_t, int);
GAME_FN(uint8_t, __thiscall, Ped_IsInVehicle, 0x004762B0, void* ped, void* entity);
GAME_FN(uint8_t, __thiscall, Ped_IsOwnEntity, 0x004763C0, void* ped, void* entity);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, Physical_ApplyCollision, 0x0046B870, void* physical, void* entity, ColPoint* colPoint, float arg);
GAME_FN(void, __thiscall, Physical_46C9D0, 0x0046C9D0, void* physical, const float* matrix, ColPoint* colPoint, int arg);
GAME_FN(void, __thiscall, Physical_SetPositionV, 0x0041EE20, void* entity, const CVector* pos);
GAME_FN(void, __thiscall, Matrix_Reorthogonalise, 0x00413A80, veh::CMatrix* m);
GAME_FN(float, __thiscall, Vector_DotProduct, 0x00468CD0, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Vector_Scale, 0x0088A480, CVector* v, float s);
GAME_FN(CVector*, __cdecl, VectorScale, 0x00412350, CVector* out, const CVector* v, float s);
GAME_FN(float, __cdecl, MaxFloat, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, MinFloat, 0x00414470, float a, float b);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int, const CVector* pos2,
        int, int, int, int, uintptr_t, uintptr_t, int, int, float, int);

// the collision spheres
GAME_VAR(float, bikeSphereRadius, 0x00A35770);
GAME_FN(void, __cdecl, FreeArray, 0x005EEA60, void* p);
GAME_FN(void*, __cdecl, MemoryMgr_Malloc2, 0x005EEA50, uint32_t size, int arg);
GAME_FN(float, __thiscall, VehicleModel_GetUnknown58, 0x005350F0, uint8_t* modelInfo);   // (its +0x58 plus 0xA95814)
GAME_FN(void, __thiscall, Game_GetFrameMatrix, 0x004C0920, CBike* bike, int frame, float* out);

// the impacts
GAME_VAR(uint8_t*, bikeTuning, 0x020C8900);   // (the bike tuning values: +0x18 a hard hit's speed)
GAME_VAR(uint32_t, lastHitSound, 0x00C2D1A4);
inline void* const eventsCF4378 = reinterpret_cast<void*>(0x00CF4378);
inline void* const actionNode1B65270 = reinterpret_cast<void*>(0x01B65270);
GAME_FN(void, __thiscall, Events_Add, 0x005A4920, void* events, void* out, void* ped, uintptr_t a, int b, int kind, int c, int d,
        int e, int f, int g, uintptr_t h);
GAME_FN(uint8_t, __thiscall, Seats_IsDriver, 0x004CEA70, CVehicleSeats* seats, CPed* ped);
GAME_FN(void, __thiscall, Game_CrashRider, 0x004BD0D0, CBike* bike, ColPoint* colPoint);
GAME_FN(void, __thiscall, Game_OnCollision, 0x004C08B0, CBike* bike, ColPoint* colPoint);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, int duration, int strength, int arg);

// the wheels' forces
GAME_VAR(float, floatC2D190, 0x00C2D190);    // (scales the handling's +0x30 into the wheels' spring force)
GAME_FN(float, __thiscall, Game_GetBrakeGrip, 0x004BF780, CBike* bike, float scale);
GAME_FN(void, __thiscall, Game_4BA620_x64, 0x004BA620, CBike* bike, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Game_4BA620(CBike* a0, float a1) {
    return Game_4BA620_x64(a0, re::BitsOf(a1));
}
GAME_FN(float, __thiscall, Game_GetTopSpeed, 0x004B9C30, CBike* bike);
GAME_FN(float, __thiscall, Vehicle_GetUnknown470, 0x004CB510, CVehicle* vehicle);
GAME_FN(float, __thiscall, Transmission_CalculateDriveAccelerationSimple, 0x004CAC20, void* transmission, float* gas,
        float* speed, CVehicle* vehicle, float pedal, float traction, float topSpeed);
GAME_FN(float, __thiscall, Transmission_CalculateDriveAcceleration, 0x004CA800, void* transmission, float* gas,
        uint8_t* gear, float* speed, CVehicle* vehicle, float pedal, float traction, float topSpeed);
GAME_FN(void, __thiscall, Game_ShiftGear, 0x004BD110, CBike* bike);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Multiply3x3b(CVector* out, const float* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
GAME_FN(void, __thiscall, Game_ProcessFrontWheel, 0x004BEB60, CBike* bike, int wheel, float force, CVector point,
        float brake, float thrust);
GAME_FN(void, __thiscall, Game_ProcessRearWheel, 0x004BEF50, CBike* bike, int wheel, float force, CVector point,
        float brake, float thrust);
GAME_FN(void, __thiscall, Game_ProcessWheelContacts, 0x004C1730, CBike* bike, CVector* contactPoints);

// pushed out of the ground
GAME_VAR(CEntity*, ignoreEntity, 0x00C1AE70);   // CWorld::pIgnoreEntity
GAME_VAR(CEntity*, entityC2A8E8, 0x00C2A8E8);   // (compared with before a reference is let go: both ways the same)
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine, 0x004629C0, const CVector* point, float zStop, ColPoint* colPoint,
        re::Ptr32<CEntity>* entity, int lists, int arg);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(CVector*, __thiscall, Physical_GetUp, 0x00473730, void* physical, CVector* out);
GAME_FN(CVector*, __cdecl, ScaleVector, 0x00412320, CVector* out, float s, const CVector* v);
// (0x46A950 with the two by pointer: the force, then the point)
GAME_FN(void, __thiscall, Physical_ApplyTurnForceV, 0x0045D410, void* physical, const CVector* force,
        const CVector* point);
GAME_FN(void, __thiscall, Game_CheckFallOff, 0x004BC9B0, CBike* bike, ColPoint* colPoint);

// the control and the collisions
GAME_VAR(float, turnFrictionA3570C, 0x00A3570C);   // (0.8: a riderless bike's speeds against a building)
GAME_VAR(float, turnFrictionA35710, 0x00A35710);
inline void* const doorManager = reinterpret_cast<void*>(0x00C3B808);
GAME_FN(void, __thiscall, DoorManager_SuspendShut, 0x004E9EF0, void* manager, int on);
GAME_FN(void, __thiscall, DoorManager_SuspendHeld, 0x004E9DF0, void* manager, int on);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412A10, veh::CMatrix* m, const float* from);
GAME_FN(void, __thiscall, Matrix_SetTranslate, 0x004127C0, veh::CMatrix* m, const CVector4* pos);   // (reads 4 floats)
GAME_FN(veh::CMatrix*, __cdecl, Matrix_Multiply, 0x004138D0, veh::CMatrix* out, const float* a, const veh::CMatrix* b);
GAME_FN(void, __thiscall, Physical_RotateAbout, 0x00468450, void* physical, void* rotation, void* translation);
GAME_FN(void, __thiscall, Physical_RemoveAndAdd, 0x00469470, void* physical);
GAME_FN(void, __thiscall, Entity_443AA0, 0x00443AA0, void* entity);
GAME_FN(void, __thiscall, Vector_SubAssign, 0x00415230, CVector* v, const CVector* other);
GAME_FN(int32_t, __thiscall, Vehicle_ResolveCollisionsAtHeight, 0x004CDC30, CVehicle* vehicle, veh::CMatrix* saved,
        float dz, float* outDz, ColPoint* colPoint, int keepFlags, int arg6, CVector* offset);
GAME_FN(int32_t, __thiscall, Vehicle_ResolveCollisions, 0x004CCD30, CVehicle* vehicle, veh::CMatrix* saved, float* dz,
        int hasDriver, ColPoint* colPoint, int keepFlags, int arg6);
GAME_FN(void, __thiscall, Game_LimitSpeed, 0x004BD1A0, CBike* bike);
GAME_FN(void, __thiscall, Game_PushOutOfGround, 0x004C54A0, CBike* bike);
GAME_FN(void, __thiscall, Game_ProcessImpact, 0x004C4830, CBike* bike, CVector* speed, ColPoint* colPoint, int crash);
GAME_FN(void, __thiscall, Game_UpdateColSurface, 0x004C0EC0, CBike* bike);
GAME_FN(void, __thiscall, Game_DoJump, 0x004BD280, CBike* bike);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(void, __thiscall, Vehicle_4CC350, 0x004CC350, CVehicle* vehicle);
GAME_FN(void, __thiscall, Vehicle_UpdateFade, 0x004CC270, CVehicle* vehicle);
GAME_FN(uint8_t, __thiscall, Game_ProcessStatus, 0x004BE0E0, CBike* bike);
GAME_FN(void, __thiscall, Game_ProcessPedalling, 0x004BAC00, CBike* bike);
GAME_FN(void, __thiscall, Game_ProcessBalance, 0x004BA7A0, CBike* bike);
GAME_FN(void, __thiscall, Game_ProcessStabiliser, 0x004BE410, CBike* bike);
GAME_FN(void, __thiscall, Game_CheckStatic, 0x004BA220, CBike* bike);
GAME_FN(uint8_t, __thiscall, Game_4B96C0, 0x004B96C0, CBike* bike);
GAME_FN(void, __thiscall, Game_ProcessCrash, 0x004BDCC0, CBike* bike);
GAME_FN(void, __thiscall, Game_4BA4E0, 0x004BA4E0, CBike* bike);
GAME_FN(void, __thiscall, Game_ProcessPhysics, 0x004C6100, CBike* bike, float* thrust, float* brake);
GAME_FN(void, __thiscall, Game_UpdateWheelsLast, 0x004BB330, CBike* bike);
GAME_FN(void, __thiscall, Game_HoldLean, 0x004BF820, CBike* bike);
GAME_FN(void, __thiscall, Game_4B9740_x64, 0x004B9740, CBike* bike, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Game_4B9740(CBike* a0, float a1, float a2) {
    return Game_4B9740_x64(a0, re::BitsOf(a1), re::BitsOf(a2));
}
GAME_FN(void, __thiscall, Game_4BA700, 0x004BA700, CBike* bike);
GAME_FN(void, __thiscall, Physical_ProcessControl, 0x00469B70, void* physical);
GAME_FN(void, __thiscall, Game_ProcessBuoyancy, 0x004C12A0, CBike* bike);
GAME_FN(void, __thiscall, Game_ProcessWheelForces, 0x004C4E90, CBike* bike, float* thrust, float* brake);
GAME_FN(void, __thiscall, Game_UpdateRumble, 0x004BCE70, CBike* bike);
GAME_FN(void, __thiscall, Game_UpdateLean, 0x004BF300, CBike* bike, CVector target);
GAME_FN(void, __thiscall, Game_UpdateWheelHeights, 0x004BF6B0, CBike* bike, int wheel1, int wheel2);
GAME_FN(CVehicle*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);

// the constructor and the model
inline void* const bikeVtable = reinterpret_cast<void*>(0x00912B24);
inline const uintptr_t bikeAnimCallback = 0x004C4820;   // (set on its animation data: +4, with the bike at +8)
GAME_FN(void, __thiscall, Game_UpdateWheelTransforms, 0x004BFD90, CBike* bike);
GAME_FN(CVehicle*, __thiscall, Vehicle_Construct, 0x004CB730, CVehicle* vehicle, int createdBy);
GAME_FN(void, __thiscall, Effect_Construct, 0x00668E90, void* effect);
GAME_FN(void, __thiscall, Vehicle_SetModelIndex, 0x004CB190, CVehicle* vehicle, int model, int arg);
GAME_FN(void, __thiscall, Game_SetModelIndex, 0x004C6010, CBike* bike, int model, int arg);
GAME_FN(void, __thiscall, Game_SetUp, 0x004C0EF0, CBike* bike, int arg);
GAME_FN(void, __thiscall, Game_LoadSounds, 0x004B9B00, CBike* bike);
GAME_FN(void, __cdecl, Clump_FindFrames, 0x00510B40, void* rwObject, re::Ptr32<void>* frames);
GAME_FN(int32_t, __cdecl, Clump_FindBone, 0x0068E4E0, void* animData, const char* name);

// 0x589B00 takes a registered entity reference by value (a CEntityRef the caller builds
// with 0x4A8480 and the callee lets go), the damage and the other entity
__declspec(noinline) void Call589B00WithRef(void* self, float damage, void* other) {
#if defined(_M_IX86)
    void* fn = reinterpret_cast<void*>(0x00589B00);
    void* construct = reinterpret_cast<void*>(0x004A8480);
    __asm {
        push 0
        push other
        sub esp, 8
        mov eax, damage
        mov dword ptr [esp + 4], eax
        mov ecx, esp
        push self
        call construct
        call fn
        add esp, 0x10
    }
#else
    // (the 64-bit build: the reference built in a local, passed by value as the game does)
    struct EntityRef64 {
        void* entity;
    } ref = {nullptr};
    re::Ours<void*(__fastcall*)(void*, void*, void*)>(0x004A8480)(&ref, nullptr, self);
    re::Ours<void(__cdecl*)(EntityRef64, float, void*, int)>(0x00589B00)(ref, damage, other, 0);
#endif
}

CVector* EntityPos(void* entity) {
    veh::CMatrix* m = At<veh::CMatrix*>(entity, 0x14);
    return m ? reinterpret_cast<CVector*>(&m->m[12]) : &At<CVector>(entity, 4);
}

uint8_t* ColModelOf(CVehicle* v) {
    return At<uint8_t*>(modelInfoPtrs[v->modelIndex], 0xC);
}

uintptr_t Id(void* self, int n) {
    return reinterpret_cast<uintptr_t>(self) + n;
}

// fistp with truncation (0x80000000 when out of range)
int32_t Trunc(double x) {
    if (x > -2147483649.0 && x < 2147483648.0) {
        return int32_t(x);
    }
    return INT32_MIN;
}

// GTA's CPhysical::GetSpeed(point), inlined: the move speed plus the turn speed across
// the point
CVector SpeedAt(void* physical, const CVector& point) {
    CVector p = point;
    CVector cross, sum;
    CVector* c = CrossProduct(&cross, &At<CVector>(physical, 0x130), &p);
    return *VectorAdd(&sum, c, &At<CVector>(physical, 0x124));
}

// the animation data's node by name (twice, as the game looks it up), nullptr when
// there are no nodes or no such one
void* FindNode(CBike* self, const char* name, bool& haveNodes) {
    haveNodes = false;
    if (!self->animData || !At<void*>(self->animData, 0x94)) {
        return nullptr;
    }
    if (!Nodes_FindByName(At<void*>(self->animData, 0x94), name, 0)) {
        return nullptr;
    }
    haveNodes = true;
    return Nodes_FindByName(At<void*>(self->animData, 0x94), name, 0);
}

// a node's vf0 (a vector: its model position)
CVector* NodeVector(void* node) {
    return reinterpret_cast<CVector*(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(node))[0]))(node);
}

} // namespace

namespace c11bikeproc {

// ---- in water -------------------------------------------------------------------------

// 0x4C12A0: a rider doing the 0x1B652C4 action is put off first (moved by 0x9E33DC).
// In water (+0x84 1) its speeds are slowed by how deep it is (+0x1BF 1 past 0.22);
// sinking (flags +0x1BC 0x10: past 0.85, or past 0.7 with a wheel's spring fully out)
// throws the rider off (-2), or lets a riderless bike go as a random one
void __fastcall ProcessBuoyancy(CBike* self, re::Edx) {
    CVector point = {0.0f, 0.0f, 0.0f};
    CVector impulse = {0.0f, 0.0f, 0.0f};
    CPed* driver = Seats_GetOccupant(&self->seats, 0);
    if (driver && At<uint8_t>(driver, 0x16CC) &&
        ActionController_IsPlaying(At<uint8_t*>(driver, 0x2E0) + 0x258, actionNode1B652C4, 1, 1)) {
        CVector sum;
        CVector* p = VectorAdd(&sum, EntityPos(driver), &vec9E33DC);
        Entity_SetPosition(driver, p->x, p->y, p->z);
        Seats_RemoveDriver(&self->seats);
    }
    float value = float(double(Virtual<float>(self, 0x8C)) * At<float>(self, 0x158) * 100.0 /
                        double(At<uint8_t>(self->handling, 0x28)));
    At<float>(self, 0x168) = value;
    if (!Buoyancy_Process(buoyancy, self, value, &point, &impulse)) {
        At<uint8_t>(self, 0x1BC) &= 0xEF;
        At<uint8_t>(self, 0x1BF) = 0;
        At<int32_t>(self, 0x84) = 0;
        return;
    }
    At<int32_t>(self, 0x84) = 1;
    float step = timeStep;
    if (timeStep < 0.5) {
        step = 0.5f;
    }
    float ratio = float(double(impulse.z) / (double(Virtual<float>(self, 0x8C)) * At<float>(self, 0x158) * step));
    float resistance = float(1.0 - double(ratio) * 0.05000000074505806);
    float k = float(std::pow(double(resistance), double(timeStep)));
    self->moveSpeed.x = k * self->moveSpeed.x;
    self->moveSpeed.y = self->moveSpeed.y * k;
    self->moveSpeed.z = self->moveSpeed.z * k;
    self->turnSpeed.x = k * self->turnSpeed.x;
    self->turnSpeed.y = self->turnSpeed.y * k;
    self->turnSpeed.z = k * self->turnSpeed.z;
    At<int32_t>(self, 0x84) = 1;
    Physical_ApplyMoveForce(self, impulse.x, impulse.y, impulse.z);
    Physical_ApplyTurnForce(self, impulse.x, impulse.y, impulse.z, point.x, point.y, point.z);
    At<uint8_t>(self, 0x1BF) = ratio > 0.2199999988079071 ? 1 : 0;
    const double full = 0.949999988079071;
    if (!(ratio > 0.8500000238418579) &&
        !(ratio > 0.7f && (At<float>(self, 0x62C) >= full || At<float>(self, 0x630) >= full))) {
        At<uint8_t>(self, 0x1BC) &= 0xEF;
        if (!(ratio > 0.10000000149011612)) {
            return;
        }
        float base = float(1.0 - double(ratio) * ratio);
        float k1 = float(std::pow(double(base), double(float(timeStep * 0.15000000596046448))));
        self->moveSpeed.x = k1 * self->moveSpeed.x;
        self->moveSpeed.y = self->moveSpeed.y * k1;
        self->moveSpeed.z = k1 * self->moveSpeed.z;
        float k2 = float(std::pow(double(base), double(float(timeStep * 0.15000000596046448))));
        self->turnSpeed.x = k2 * self->turnSpeed.x;
        self->turnSpeed.y = self->turnSpeed.y * k2;
        self->turnSpeed.z = k2 * self->turnSpeed.z;
        return;
    }
    // sinking
    At<uint8_t>(self, 0x1BC) |= 0x10;
    if (self->moveSpeed.z < -0.10000000149011612) {
        self->moveSpeed.z = -0.1f;
    }
    if (!driver) {
        if (veh::PedLastVehicle(re::PtrCast<CPed*>(playerPed)) == self) {
            veh::PedLastVehicle(re::PtrCast<CPed*>(playerPed)) = nullptr;
        }
        Vehicle_SetCreatedBy(self, 1);
        SetRef(&self->owner, nullptr);
        return;
    }
    // (the driver found at the start, even when the action put him off already)
    At<int32_t>(driver, 0x84) = 1;
    CVector offset = {float(vec9E33DC.x * 0.6499999761581421), float(vec9E33DC.y * 0.6499999761581421),
                      float(vec9E33DC.z * 0.6499999761581421)};
    CVector sum;
    CVector* p = VectorAdd(&sum, EntityPos(driver), &offset);
    Entity_SetPosition(driver, p->x, p->y, p->z);
    Game_KnockOffRider(self, -2, nullptr, driver, 0);
}

// ---- the wheels' contacts -------------------------------------------------------------

void __fastcall ProcessWheelContacts(CBike* self, re::Edx, CVector* contactPoints) {
    float t0 = float(1.0 - double(At<float>(self, 0x680)) / At<float>(self, 0x688));
    At<float>(self, 0x62C) = float((double(At<float>(self, 0x62C)) - t0) / (1.0 - t0));
    float t1 = float(1.0 - double(At<float>(self, 0x684)) / At<float>(self, 0x68C));
    At<float>(self, 0x630) = float((double(At<float>(self, 0x630)) - t1) / (1.0 - t1));
    uint8_t* colModel = ColModelOf(self);
    CVector springDir[2] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    CVector contactSpeed[2] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    CVector fwd;
    if (self->matrix) {
        fwd = *reinterpret_cast<CVector*>(&self->matrix->m[4]);
    } else {
        double heading = At<float>(self, 0x10);
        fwd = {-float(std::sin(heading)), float(std::cos(heading)), 0.0f};
    }
    float fwdSpeed = std::fabs(float((double(self->moveSpeed.y) * fwd.y + double(self->moveSpeed.x) * fwd.x) +
                                     double(self->moveSpeed.z) * fwd.z));
    for (int i = 0; i < 2; i++) {
        float& compression = At<float>(self, 0x62C + i * 4);
        uint8_t status = At<uint8_t>(self, 0x5D8 + i);
        if (status == 2) {
            compression = 1.0f;
        } else if (status == 1) {
            // a burst tyre bumps now and then, more often the faster it goes
            int32_t r = Rand();
            int32_t chance = int32_t(uint16_t(Trunc(double(fwdSpeed) * 40.0))) + 0x61;
            if (int32_t(uint32_t(r) * uint32_t(chance)) / 0x7FFF < 100) {
                float lengthA = At<float>(self, 0x680 + i * 4);
                float v = float((double(At<float>(self, 0x688 + i * 4)) - lengthA) * 0.30000001192092896 / lengthA +
                                compression);
                compression = v;
                if (1.0f < v) {
                    compression = 1.0f;
                }
            }
        }
        if (!(compression < 1.0f)) {
            continue;
        }
        CVector axis = {1.0f, 0.0f, 0.0f};
        CVector d;
        CVector* rel = VectorSub(&d, &At<CVector>(self, 0x5DC + i * 0x28), Entity_GetPosition(self));
        contactPoints[i] = *rel;
        contactPoints[i].z = float(contactPoints[i].z * 0.05000000074505806);
        // the wheel's spring turned by the wheel's rotation (+0x648) about x
        NiTransform* transforms = At<NiTransform*>(At<uint8_t*>(self->animData, 0x14), 8);
        NiTransform t = transforms[i == 0 ? At<int32_t>(self, 0x788) : At<int32_t>(self, 0x78C)];
        float angle = float(-double(At<float>(self, 0x648 + i * 4)) * 180.0 / 3.1415927410125732);
        Transform_SetRotation(&t, &axis, angle, 1);
        veh::CMatrix m;
        m.attached = &t;
        Matrix_FromTransform(&m, &t);
        uint8_t* line = At<uint8_t*>(At<uint8_t*>(colModel, 0x38), 4) + i * 0x20;
        CVector dd;
        CVector dir = *VectorSub(&dd, reinterpret_cast<CVector*>(line + 0x10), reinterpret_cast<CVector*>(line));
        springDir[i].x = float((double(m.m[0]) * dir.x + double(m.m[4]) * dir.y) + double(m.m[8]) * dir.z);
        springDir[i].y = float((double(m.m[5]) * dir.y + double(m.m[1]) * dir.x) + double(m.m[9]) * dir.z);
        springDir[i].z = float((double(m.m[6]) * dir.y + double(m.m[2]) * dir.x) + double(m.m[10]) * dir.z);
        Vector_Normalise(&springDir[i]);
        At<CVector>(self, 0x668 + i * 0xC) = springDir[i];
    }
    At<uint8_t>(self, 0x645) = 0;
    At<uint8_t>(self, 0x644) = 0;
    if (chapterSeason[chapter] == 2 && IsAreaInterior(currArea)) {
        At<uint8_t>(self, 0x645) = 3;
        At<uint8_t>(self, 0x644) = 3;
    }
    At<uint8_t>(self, 0x647) = 0;
    At<uint8_t>(self, 0x646) = 0;
    Game_ProcessSuspension(self, contactPoints);

    // the speeds at the contact points, less the ground's (the ground entities let go
    // of without their references: as GTA does)
    auto contactSpeeds = [&] {
        for (int i = 0; i < 2; i++) {
            contactSpeed[i] = SpeedAt(self, contactPoints[i]);
            re::Ptr32<CEntity>& ground = At<CEntity*>(self, 0x6E4 + i * 4);
            if (ground) {
                CVector groundSpeed = SpeedAt(ground, At<CVector>(self, 0x6CC + i * 0xC));
                ground = nullptr;
                contactSpeed[i].x = contactSpeed[i].x - groundSpeed.x;
                contactSpeed[i].y = contactSpeed[i].y - groundSpeed.y;
                contactSpeed[i].z = contactSpeed[i].z - groundSpeed.z;
            }
        }
    };
    contactSpeeds();
    // on steep ground the spring pushes along the ground's normal
    for (int i = 0; i < 2; i++) {
        if (At<float>(self, 0x62C + i * 4) < 1.0f) {
            CVector normal = At<CVector>(self, 0x5EC + i * 0x28);
            if (normal.z > 0.35f) {
                springDir[i] = {-normal.x, -normal.y, -normal.z};
            }
        }
    }
    for (int i = 0; i < 2; i++) {
        if (At<float>(self, 0x62C + i * 4) < 1.0f) {
            Physical_ApplySpringDampening(self, At<float>(self->handling, 0xB0), &springDir[i], &contactPoints[i],
                                          &contactSpeed[i]);
        }
    }
    contactSpeeds();

    // the ground normal (+0x5A0) averaged over the wheels on the ground in the last 2
    // time units (+0x6EC how many, +0x6ED the rear one's)
    uint8_t& count = At<uint8_t>(self, 0x6EC);
    CVector& groundNormal = At<CVector>(self, 0x5A0);
    count = 0;
    At<uint8_t>(self, 0x6ED) = 0;
    for (int i = 0; i < 2; i++) {
        float& timer = At<float>(self, 0x63C + i * 4);
        if (At<float>(self, 0x62C + i * 4) < 1.0f) {
            timer = 2.0f;
        } else {
            float t = float(timer - timeStep);
            timer = t > 0.0f ? t : 0.0f;
        }
        if (0.0f < timer) {
            count++;
            if (i == 1) {
                At<uint8_t>(self, 0x6ED) = 1;
            }
            const CVector& normal = At<CVector>(self, 0x5EC + i * 0x28);
            if (count == 1) {
                groundNormal = normal;
            } else {
                groundNormal.x = groundNormal.x + normal.x;
                groundNormal.y = normal.y + groundNormal.y;
                groundNormal.z = normal.z + groundNormal.z;
            }
        }
    }
    if (count == 0) {
        groundNormal = vec9E33DC;
        return;
    }
    float inv = float(1.0 / int32_t(count));
    groundNormal.x = inv * groundNormal.x;
    groundNormal.y = groundNormal.y * inv;
    groundNormal.z = inv * groundNormal.z;
    float* m = Entity_GetMatrix(self);
    float up = float((double(groundNormal.y) * m[9] + double(groundNormal.x) * m[8]) + double(groundNormal.z) * m[10]);
    if (up < -0.5f) {
        groundNormal.x = float(groundNormal.x * -1.0);
        groundNormal.y = float(groundNormal.y * -1.0);
        groundNormal.z = float(groundNormal.z * -1.0);
    }
}

// ---- Teleport -------------------------------------------------------------------------

void __fastcall Teleport(CBike* self, re::Edx, float x, float y, float z) {
    World_Remove(self);
    Entity_SetPosition(self, x, y, z);
    float angle = float(3.1415927410125732 * 0.0 / 180.0);
    Placeable_SetOrientation(self, angle, angle, angle);
    self->moveSpeed = {0.0f, 0.0f, 0.0f};
    self->turnSpeed = {0.0f, 0.0f, 0.0f};
    At<float>(self, 0x648) = angle;
    At<float>(self, 0x62C) = 1.0f;
    At<int32_t>(self, 0x70C) = 0;
    At<int32_t>(self, 0x710) = 0;
    At<float>(self, 0x63C) = 0.0f;
    At<float>(self, 0x64C) = angle;
    At<float>(self, 0x630) = 1.0f;
    At<float>(self, 0x640) = 0.0f;
    World_Add(self);
}

// ---- PreRender ------------------------------------------------------------------------

namespace {

// a light's shadow on the ground (the handling's byte plus `add` its kind)
void LightShadow(CBike* self, const CVector& pos, const float* up, const float* right, const float* fwd,
                 int handlingByte, int add) {
    int kind = uint8_t(At<uint8_t>(self->handling, handlingByte) + add);
    LightShadow_Add(pos.x, pos.y, pos.z, up[0], up[1], up[2], right[0], right[1], right[2], fwd[0], fwd[1], fwd[2], kind,
                    0, 0, 0);
}

// the lights wanted: at night (hours 21..6, 20 and 7 by the vehicle's own minute), or in
// fog above the vehicle's own level
uint8_t LightsWanted(CBike* self) {
    uint8_t hour = clockHours;
    uint8_t own = At<uint8_t>(self, 0x10C) & 0x3F;
    if (hour > 20) {
        return 1;
    }
    if (hour > 19 && clockMinutes > own) {
        return 1;
    }
    if (hour < 7) {
        return 1;
    }
    if (hour < 8 && clockMinutes < own) {
        return 1;
    }
    double level = double(At<uint16_t>(self, 0x10C)) / 50000.0;
    if (lightsFogA > level || lightsFogB > level) {
        return 1;
    }
    return 0;
}

} // namespace

void __fastcall PreRender(CBike* self, re::Edx) {
    if (At<int32_t>(self, 0xD0) != 0) {
        return;
    }
    Game_UpdateBrakeSound(self);
    Game_UpdateSurfaceSounds(self);

    // the skid marks follow the wheels
    CVector wheelPos;
    if (At<uint8_t>(self, 0x708)) {
        uint8_t* obj = At<uint8_t*>(self, 0x5D4);
        if (At<float>(obj, 0x4C) != 0.0f || At<float>(obj, 0x48) != 0.0f) {
            // (the vector's fourth float is whatever is on the stack there)
            Game_GetNodePosition(self, "frontwheel", &wheelPos);
            Trails_AddPoint(trails, At<int32_t>(self, 0x700), CVector4{wheelPos.x, wheelPos.y, wheelPos.z, 0.0f}, 0.05f);
            Game_GetNodePosition(self, "backwheel", &wheelPos);
            Trails_AddPoint(trails, At<int32_t>(self, 0x704), CVector4{wheelPos.x, wheelPos.y, wheelPos.z, 0.0f}, 0.05f);
        }
    }

    // the rear wheel's dust (+0x626: its surface; 0x1E has none)
    bool pedalling = !(At<int32_t>(self, 0x710) == 0 &&
                       !(At<float>(self, 0x470) > 0.10000000149011612 && At<float>(self, 0x694) > 0.005f));
    if (!pedalling || At<uint8_t>(self, 0x626) == 0x1E) {
        if (At<uint8_t>(self, 0x647)) {
            CVector fwd;
            Entity_GetForward(self, &fwd);
            Fn5280C0(self, &At<uint8_t>(self, 0x604), fwd.x, fwd.y, At<uint8_t>(self, 0x645), 0);
        }
    } else if (0.0f < At<float>(self, 0x640)) {
        float lean = At<float>(self, 0x6A8);
        uint8_t* colModel = ColModelOf(self);
        float s = float(std::sin(double(lean)));
        CVector p = At<CVector>(self, 0x604);
        float k = float(double(s) * (double(At<float>(colModel, 0x18)) * 0.800000011920929));
        if (At<float>(self, 0x638) < 1.0f) {
            // (the wheel's point plus itself, halved: the same point)
            CVector sum;
            CVector* r = VectorAdd(&sum, &p, &At<CVector>(self, 0x604));
            p = {float(r->x * 0.5), float(r->y * 0.5), float(r->z * 0.5)};
        }
        float* m = Entity_GetMatrix(self);
        CVector off = {m[0] * k, m[1] * k, k * m[2]};
        p.x = off.x + p.x;
        p.y = p.y + off.y;
        p.z = p.z + off.z;
        Fn58C940(self, At<uint8_t>(self, 0x626), -1.0f);
        CVector fwd;
        Entity_GetForward(self, &fwd);
        // (p, the point leaned out, is never used: the wheel's own point goes)
        Fn5280C0(self, &At<uint8_t>(self, 0x604), fwd.x, fwd.y, At<uint8_t>(self, 0x645), 0);
    }

    // exhaust smoke, below 130 (speedScale units) along its forward
    if ((self->flags33A & 2) && !(At<uint32_t>(self->handling, 0xCC) & 0x100000)) {
        CVector tmp;
        CVector* f = Entity_GetForward(self, &tmp);
        float along = float((double(self->moveSpeed.y) * f->y + double(f->x) * self->moveSpeed.x) +
                            double(self->moveSpeed.z) * f->z);
        float speed = float(double(along) / speedScale);
        if (speed < 130.0f) {
            CVector left = *EntityPos(self);
            CVector right = {0.0f, 0.0f, 0.0f};
            uint8_t two = 0;
            bool haveNodes;
            void* node = FindNode(self, "Exhaust_left", haveNodes);
            if (node) {
                AnimManager_GetNodePosition(animManager, self->animData, -1, &left, node);
            }
            node = FindNode(self, "Exhaust_right", haveNodes);
            if (node) {
                AnimManager_GetNodePosition(animManager, self->animData, -1, &right, node);
                two = 1;
            }
            CVector vel = {0.0f, 0.0f, 0.0f};
            if (speed < 10.0f) {
                // slow: out sideways by the steering
                float s = float(std::sin(double(self->steerAngle)));
                float c = float(std::cos(double(self->steerAngle)));
                CVector dir = {-s, c, 0.0f};
                CVector rot;
                Multiply3x3(&rot, Entity_GetMatrix(self), &dir);
                float r = RandomFloatInRange(-0.06f, -0.03f);
                vel.x = rot.x * r;
                vel.y = r * rot.y;
            } else {
                vel.x = self->moveSpeed.x;
                vel.y = self->moveSpeed.y;
            }
            float gas = At<float>(self, 0x294);
            float rnd = RandomFloatInRange(1.0f, 3.0f);
            if ((double(gas) + 1.100000023841858) * rnd > 2.0) {
                uint8_t colour[4] = {0, 0, 0, 0};
                Particles_Add(4, &left, &vel, 0.0f, colour, 0, 0);
                if (two) {
                    Particles_Add(4, &right, &vel, 0.0f, colour, 0, 0);
                }
            }
        }
    }

    // the lights: bikes of class 3 and 8 only
    int32_t cls;
    if (*Vehicle_GetVehicleClass(self, &cls) != 3 && *Vehicle_GetVehicleClass(self, &cls) != 8) {
        return;
    }
    uint8_t on = LightsWanted(self);
    uint8_t flags = self->flags33A;
    uint8_t current = (flags >> 3) & 1;
    if (on != current) {
        uint8_t status = self->typeAndStatus >> 3;
        if (status != 5) {
            if (status == 4) {
                // a parked bike's lights only go off once the camera is 100 away
                if (current) {
                    float dx = std::fabs(float(Camera_GetPosition(theCamera)->x - EntityPos(self)->x));
                    float dy = std::fabs(float(Camera_GetPosition(theCamera)->y - EntityPos(self)->y));
                    if (double(dy) + dx > 100.0) {
                        self->flags33A &= 0xF7;
                    }
                }
            } else {
                self->flags33A = uint8_t(((uint8_t(on * 8) ^ flags) & 8) ^ flags);
            }
        }
    }

    const float* riderMatrix = &At<float>(self, 0x734);
    if ((self->flags33A & 2) && (self->flags33A & 8)) {
        Game_UpdateRiderMatrix(self);
        CVector camPos = *reinterpret_cast<CVector*>(GetLocalTransform(GetCameraNode(cameraObject)) + 0x24);
        CVector d;
        VectorSub(&d, EntityPos(self), &camPos);
        CVector dir = d;
        float dist = Vector_Magnitude(&dir);
        if (dist == 0.0f) {
            dir = {1.0f, 0.0f, 0.0f};
        } else {
            CVector tmp;
            Vector_ScaleAssign(&dir, &tmp, float(1.0 / dist));
        }
        CVector tmp;
        CVector fwd = *Entity_GetForward(self, &tmp);
        // how much the camera looks at its front (below 0) or back (above 0)
        float cosA = float((double(dir.y) * fwd.y + double(dir.x) * fwd.x) + double(dir.z) * fwd.z);
        float angle = std::fabs(SafeAcos(std::fabs(cosA)));

        // the headlight: the "Headlight" node's, else the model's (the rider's matrix)
        veh::CMatrix mat = {};
        mat.attached = nullptr;
        CVector headPos = {0.0f, 0.0f, 0.0f};
        CVector headModel = {0.0f, 0.0f, 0.0f};
        bool haveNodes;
        void* node = FindNode(self, "Headlight", haveNodes);
        if (haveNodes) {
            // (a second lookup that failed leaves these unset)
            if (node) {
                NiTransform t;
                t.scale = 1.0f;
                AnimManager_GetNodeTransform(animManager, self->animData, -1, &t, node);
                Matrix_FromTransform(&mat, &t);
                headPos = {mat.m[12], mat.m[13], mat.m[14]};
                headModel = *NodeVector(node);
            }
        } else {
            CVector p = At<CVector>(modelInfoPtrs[self->modelIndex], 0x64);
            headModel = Vector_Equals(&p, &vecUnset) ? CVector{0.0f, 0.5f, 0.0f} : p;
            Matrix_CopyOnlyMatrix(&mat, riderMatrix);
            CVector r;
            headPos = *MultiplyMatrixVector(&r, &mat, &headModel);
        }
        uint32_t handlingFlags = At<uint32_t>(self->handling, 0xCC);
        if (cosA < 0.0f) {
            float front = -cosA;
            float k1 = float(front * 0.5 + 0.30000001192092896);
            float k2 = float((double(front) + 1.0) * 0.699999988079071);
            bool police = (handlingFlags & 0x8000) != 0;
            if (cosA < -0.9700000286102295 && dist < 30.0) {
                // looking straight into it: a flare
                Coronas_Register(Id(self, 6), police ? 0x96 : 0xA0, police ? 0x96 : 0xA0, police ? 0xC3 : 0x8C, 0xFF,
                                 &headPos, 0.65f, 45.0f, 3, 0, 0, 0, angle, 0, 1, 1.5f);
            }
            police = (At<uint32_t>(self->handling, 0xCC) & 0x8000) != 0;
            int rg = uint8_t(Trunc(k1 * (police ? 190.0 : 210.0)));
            int b = uint8_t(Trunc(k1 * (police ? 255.0 : 195.0)));
            Coronas_Register(Id(self, 0), rg, rg, b, 0xFF, &headPos, k2, 50.0f, 7, 0, 0, 1, angle, 0, 1, 1.5f);
        } else {
            Coronas_Off(Id(self, 0), &headPos, 50.0f, angle);
        }
        LightShadow(self, headPos, &mat.m[8], &mat.m[0], &mat.m[4], 0xD8, 4);

        // the taillight: the "Taillight" node's, else the model's
        CVector tailPos = {0.0f, 0.0f, 0.0f};
        CVector tailModel = {0.0f, 0.0f, 0.0f};
        node = FindNode(self, "Taillight", haveNodes);
        if (haveNodes) {
            if (node) {
                AnimManager_GetNodePosition(animManager, self->animData, -1, &tailPos, node);
                tailModel = *NodeVector(node);
            }
        } else {
            CVector p = At<CVector>(modelInfoPtrs[self->modelIndex], 0x70);
            tailModel = Vector_Equals(&p, &vecUnset) ? CVector{0.0f, -0.7f, 0.0f} : p;
            CVector r;
            tailPos = *MultiplyMatrixVector(&r, riderMatrix, &tailModel);
        }
        if (cosA > 0.0f) {
            float t1 = float(double(cosA) * 0.4000000059604645 + 0.4000000059604645);
            float t2 = float((double(cosA) * 0.25 + 1.0) * 0.20000000298023224);
            float gas = At<float>(self, 0x294);
            // (reversing: the corona left as it was)
            if (!(gas < 0.0f)) {
                if (0.0f < At<float>(self, 0x298)) {
                    t1 = float(0.4000000059604645 + t1);
                    t2 = float(t2 + 0.05000000074505806);
                }
                Coronas_Register(Id(self, 0xE), uint8_t(Trunc(t1 * 128.0)), 0, 0, 0xFF, &tailPos, t2, 50.0f, 7, 0, 0, 1,
                                 angle, 0, 1, 1.5f);
            }
        } else {
            Coronas_Off(Id(self, 0xE), &tailPos, 50.0f, angle);
        }
        LightShadow(self, tailPos, riderMatrix + 8, riderMatrix, riderMatrix + 4, 0xD9, 8);

        // the lights on the ground ahead and behind (along the headlight matrix's forward)
        CVector pos = *EntityPos(self);
        CVector shadowPos = pos;
        float fx = mat.m[4];
        float fy = mat.m[5];
        float lengthSqr = float(double(fx) * fx + double(fy) * fy);
        float k = lengthSqr;
        if (lengthSqr != 0.0f) {
            k = RecipSqrt(1.0f, lengthSqr);
        }
        float kx = fx * k;
        float ky = k * fy;
        double ahead = double(headModel.y) + 6.0;
        CVector off = {float(kx * ahead), float(ahead * ky), 0.0f};
        shadowPos.x = off.x + pos.x;
        shadowPos.y = shadowPos.y + off.y;
        shadowPos.z = float(shadowPos.z + 2.0);
        Shadows_StoreCarLight(self, Id(self, 0x16), texShadowC93728, &shadowPos, float(kx * 7.0), float(ky * 7.0),
                              float(ky * 3.5), float(kx * -3.5), 0x2D, 0x2D, 0x2D, 7.0f);
        // (from the headlight's model y and the taillight's, not a length)
        double back = (double(headModel.y) + 8.5) - tailModel.y;
        off = {float(kx * back), float(back * ky), 0.0f};
        shadowPos.x = shadowPos.x - off.x;
        shadowPos.y = shadowPos.y - off.y;
        shadowPos.z = float(shadowPos.z - 0.0);
        Shadows_StoreCarLight(self, Id(self, 0x19), texShadowC93728, &shadowPos, 3.0f, 0.0f, 0.0f, -3.0f, 8, 0, 0, 4.0f);

        // the point lights
        const CVector& v = self->moveSpeed;
        int still = double(v.x) * v.x + double(v.y) * v.y < 0.20249998569488525 ? 1 : 0;
        const CVector* at = EntityPos(self);
        const CVector* f = Entity_GetForward(self, &tmp);
        PointLights_Add(1, at->x, at->y, at->z, f->x, f->y, f->z, 10.0f, 1.0f, 1.0f, 1.0f, still, 0);
        float brake = At<float>(self, 0x298);
        f = Entity_GetForward(self, &tmp);
        CVector backDir = {-f->x, -f->y, -f->z};
        if (0.0f < brake) {
            PointLights_Add(0, tailPos.x, tailPos.y, tailPos.z, backDir.x, backDir.y, backDir.z, 5.0f, 0.4f, 0.0f, 0.0f,
                            0, 0);
        } else {
            PointLights_Add(0, tailPos.x, tailPos.y, tailPos.z, backDir.x, backDir.y, backDir.z, 3.5f, 0.2f, 0.0f, 0.0f,
                            0, 0);
        }
        return;
    }

    // lights off: the taillight still shows braking and while not reversing, seen from
    // behind (not parked or abandoned)
    uint8_t status = self->typeAndStatus >> 3;
    if (status == 4 || status == 5) {
        return;
    }
    Game_UpdateRiderMatrix(self);
    CVector4 tail = {0.0f, 0.0f, 0.0f, 0.0f};
    bool haveNodes;
    void* node = FindNode(self, "Taillight", haveNodes);
    if (haveNodes) {
        if (node) {
            AnimManager_GetNodePosition(animManager, self->animData, -1, reinterpret_cast<CVector*>(&tail), node);
        }
    } else {
        CVector p = At<CVector>(modelInfoPtrs[self->modelIndex], 0x70);
        if (Vector_Equals(&p, &vecUnset)) {
            tail.x = 0.0f;
            tail.y = -0.7f;
            tail.z = 0.0f;
        } else {
            tail.x = p.x;
            tail.y = p.y;
            tail.z = p.z;
        }
        Matrix_MultiplyPoint4(&tail, riderMatrix, &tail);
    }
    CVector* tailPos = reinterpret_cast<CVector*>(&tail);
    if (!(0.0f < At<float>(self, 0x298)) && !(At<float>(self, 0x294) < 0.0f)) {
        Coronas_Off(Id(self, 0xE), tailPos, 50.0f, 0.0f);
        return;
    }
    CVector camPos = *reinterpret_cast<CVector*>(GetLocalTransform(GetCameraNode(cameraObject)) + 0x24);
    CVector d;
    VectorSub(&d, EntityPos(self), &camPos);
    Vector_Normalise(&d);
    CVector tmp;
    CVector* f = Entity_GetForward(self, &tmp);
    float dot = float((double(f->y) * d.y + double(d.x) * f->x) + double(f->z) * d.z);
    if (!(0.0f < dot)) {
        Coronas_Off(Id(self, 0xE), tailPos, 50.0f, 0.0f);
        return;
    }
    if (At<float>(self, 0x294) < 0.0f) {
        return;
    }
    Coronas_Register(Id(self, 0xE), 0x78, 0, 0, 0xFF, tailPos, 0.4f, 50.0f, 0, 0, 0, 1, 0.0f, 0, 1, 1.0f);
    LightShadow(self, *tailPos, riderMatrix + 8, riderMatrix, riderMatrix + 4, 0xD9, 8);
}

// ---- ProcessEntityCollision -----------------------------------------------------------

namespace {

void PlayCrashSound(CBike* self, const char* name, float volume) {
    CVector* pos = EntityPos(self);
    int id = Sounds_GetId(audioFocusPlayer, name);
    Sounds_PlayAt(audioFocusPlayer, id, pos, 2, nullptr, 0, 0, 0, 0, 0, 0, 6, 1, volume, 0);
}

} // namespace

uint8_t __fastcall ProcessEntityCollision(CBike* self, re::Edx, const float* matrix, uint8_t* entity, ColPoint* colPoint,
                                          int keepHeight) {
    if ((self->typeAndStatus & 0xF8) != 0x10) {
        self->flags33B |= 2;
    }
    uint8_t type = At<uint8_t>(entity, 0x108) & 7;
    if (type == 7) {
        return 0;
    }
    if (type == 3 && (At<int16_t>(entity, 0x10E) == ignoredPedModel || At<uint8_t>(entity, 0x1C98) ||
                      Ped_IsInVehicle(entity, self) || Ped_IsOwnEntity(entity, self))) {
        return 0;
    }
    uint8_t* colModel = ColModelOf(self);
    if (flagA35739 && (At<uint8_t>(entity, 0x108) & 7) != 3) {
        float before[2] = {At<float>(self, 0x62C), At<float>(self, 0x630)};
        void* entityCol = GetColModel(entity);
        float* entityMatrix = Placeable_GetMatrix(entity);
        Col_ProcessWheels(Entity_GetMatrix(self), colModel, entityMatrix, entityCol, &At<uint8_t>(self, 0x5DC),
                          &At<float>(self, 0x62C));
        if (!(At<uint8_t>(self, 0x1BD) & 4) && !(At<uint8_t>(self, 0x1BC) & 0x80)) {
            for (int i = 0; i < 2; i++) {
                float now = At<float>(self, 0x62C + i * 4);
                if (!(1.0f > now) || !(before[i] > now)) {
                    continue;
                }
                uint8_t kind = At<uint8_t>(entity, 0x108) & 7;
                if (kind == 2 || kind == 4) {
                    re::Ptr32<CEntity>& ground = At<CEntity*>(self, 0x6E4 + i * 4);
                    ground = reinterpret_cast<CEntity*>(entity);
                    RegisterReference(entity, &ground);
                    CVector tmp;
                    CVector* d = VectorSub(&tmp, &At<CVector>(self, 0x5DC + i * 0x28), Entity_GetPosition(entity));
                    At<CVector>(self, 0x6CC + i * 0xC) = *d;
                }
                At<uint8_t>(self, 0x1BE) = At<uint8_t>(self, 0x5FE + i * 0x28);   // (the surface under it)
            }
        }
    }
    uint8_t result = 0;
    void* entityCol = GetColModel(entity);
    float* entityMatrixB = Placeable_GetMatrix(entity);
    float* entityMatrixA = Placeable_GetMatrix(entity);
    if (!uint8_t(Col_ProcessSweep(matrix, Entity_GetMatrix(self), colModel, entityMatrixA, entityMatrixB, entityCol,
                                  colPoint, 0, 1, 0, 0, 0, 0xF, 0, 1))) {
        return result;
    }
    uint8_t hitBefore = At<int32_t>(self, 0x1C0) > 0 ? 1 : 0;
    result = 1;
    Physical_ApplyCollision(self, entity, colPoint, 0.0f);
    bool moved = true;
    if (!re::Flag(keepHeight)) {
        if (0.0f < colPoint->depth) {
            At<int32_t>(self, 0x1C0)++;
            CVector target = {matrix[12], matrix[13], matrix[14]};
            float nz = colPoint->normal.z;
            CVector tmp, tmp2;
            if (!hitBefore && nz < 0.9999989867210388 && nz > -0.9999989867210388) {
                // against something level: slid back along the ground and out along the
                // normal's flat part
                CVector back;
                VectorSub(&back, EntityPos(self), &target);
                float left = MaxFloat(0.0f, float(colPoint->depth - 0.004999999888241291));
                CVector part;
                VectorScale(&part, &back, left);
                back = {back.x - part.x, back.y - part.y, back.z - part.z};
                CVector flat = {colPoint->normal.x, colPoint->normal.y, 0.0f};
                Vector_Normalise(&flat);
                CVector away = {-back.x, -back.y, -back.z};
                Vector_Scale(&flat, float(Vector_DotProduct(&away, &flat) + 0.05000000074505806));
                CVector* c = VectorAdd(&tmp, &back, &flat);
                CVector* e = VectorAdd(&tmp2, &target, &part);
                CVector pos;
                VectorAdd(&pos, e, c);
                Physical_SetPositionV(self, &pos);
                colPoint->depth = MaxFloat(0.0f, float(colPoint->depth - 0.004999999888241291));
            } else {
                CVector back;
                VectorSub(&back, EntityPos(self), &target);
                float k = float(colPoint->depth - double(At<int32_t>(self, 0x1C0)) * 0.10000000149011612);
                if (k < 0.0) {
                    k = 0.0f;
                }
                CVector part = {back.x * k, back.y * k, k * back.z};
                CVector pos;
                VectorAdd(&pos, &target, &part);
                Physical_SetPositionV(self, &pos);
                k = float(colPoint->depth - double(At<int32_t>(self, 0x1C0)) * 0.10000000149011612);
                if (k < 0.0) {
                    k = 0.0f;
                }
                colPoint->depth = k;
            }
        } else {
            moved = false;
        }
    } else {
        float k = float(colPoint->depth - 0.004999999888241291);
        if (k < 0.0) {
            k = 0.0f;
        }
        colPoint->depth = k;
        float z = matrix[14];
        float up = float(EntityPos(self)->z - z);
        float scaled = float(double(up) * k);
        EntityPos(self)->z = float(double(z) + scaled);
    }
    if (moved) {
        Physical_46C9D0(self, matrix, colPoint, 0);
        if (self->matrix) {
            Matrix_Reorthogonalise(self->matrix);
        }
    }
    uint8_t kind = At<uint8_t>(entity, 0x108) & 7;
    if (kind == 1 || (kind == 4 && (At<uint8_t>(entity, 0x1BC) & 4))) {
        At<int32_t>(self, 0x7C) = 1;
    }

    // the crash's sound (one each 250ms at most)
    if (!(At<float>(self, 0x19C) > 0.029999999329447746)) {
        return result;
    }
    float damage = At<float>(self, 0x19C);
    uint32_t now = timeInMs;
    int32_t cls;
    if (*Vehicle_GetVehicleClass(self, &cls) == 8) {
        Call589B00WithRef(self, damage, entity);
        return 1;
    }
    const char* sound = "Bike_Crash";
    if (At<int32_t>(self, 0x6BC) == 0) {
        if (now <= lastCrashSound + 0xFA) {
            return result;
        }
        if (!(damage <= 0.4000000059604645)) {
            damage = 1.0f;
        } else {
            damage = float(damage / 0.4000000059604645);
        }
    } else {
        if ((At<uint8_t>(entity, 0x108) & 7) == 3) {
            return result;
        }
        if (now <= lastCrashSound + 0xFA) {
            return result;
        }
        uint8_t surface = At<uint8_t>(self, 0x1BE);
        if (surface == 0x23 || surface == 8 || surface == 0x38) {
            lastCrashSound = now;
            return result;
        }
        if (damage <= 0.5f) {
            damage = float(damage + damage);
            sound = "Bike_Hit";
        } else if (1.0f < damage) {
            damage = 1.0f;
        } else {
            damage = float((damage - 0.5) + 0.5);
        }
    }
    damage = MinFloat(damage, 1.0f);
    PlayCrashSound(self, sound, damage);
    lastCrashSound = now;
    return result;
}

// ---- the wheels' collision spheres ----------------------------------------------------

uint8_t __fastcall SetUpWheelColModel(CBike* self, re::Edx, uint8_t* colModelOut) {
    uint8_t* modelInfo = modelInfoPtrs[self->modelIndex];
    uint8_t* colModel = At<uint8_t*>(modelInfo, 0xC);
    uint8_t* colData = At<uint8_t*>(colModel, 0x38);
    At<int8_t>(colData, 0x1E) = 8;
    FreeArray(At<void*>(colData, 0));
    At<Sphere*>(colData, 0) = static_cast<Sphere*>(MemoryMgr_Malloc2(At<int8_t>(colData, 0x1E) * 0x14, 0));
    for (int i = 0; i < 12; i++) {   // (the bounding box and sphere)
        At<uint32_t>(colModelOut, i * 4) = At<uint32_t>(colModel, i * 4);
    }
    auto spheres = [&] { return At<Sphere*>(colData, 0); };
    veh::CMatrix m;
    m.attached = nullptr;

    // the front wheel (frame 4) and the rear (frame 5)
    Game_GetFrameMatrix(self, 4, m.m);
    spheres()[0].centre = {m.m[12], m.m[13], m.m[14]};
    Sphere* s = &spheres()[0];
    s->centre.x = float(s->centre.x + 0.0);
    s->centre.y = float(0.0 + s->centre.y);
    s->centre.z = float(s->centre.z + 0.10000000149011612);
    spheres()[0].radius = float(VehicleModel_GetUnknown58(modelInfo) * 0.2199999988079071);
    Game_GetFrameMatrix(self, 5, m.m);
    s = &spheres()[2];
    s->centre = {m.m[12], m.m[13], m.m[14]};
    s->centre.x = float(s->centre.x + 0.0);
    s->centre.y = float(s->centre.y + 0.10000000149011612);
    s->centre.z = float(0.10000000149011612 + s->centre.z);
    spheres()[2].radius = float(VehicleModel_GetUnknown58(modelInfo) * 0.2199999988079071);

    // the body between them: a quarter, three quarters and half the way, 0.21 up
    const double along[3] = {0.25, 0.75, 0.5};
    const int index[3] = {1, 3, 4};
    for (int j = 0; j < 3; j++) {
        CVector tmp, tmp2;
        CVector* d = VectorSub(&tmp, &spheres()[2].centre, &spheres()[0].centre);
        CVector part = {float(d->x * along[j]), float(d->y * along[j]), float(d->z * along[j])};
        CVector* c = VectorAdd(&tmp2, &spheres()[0].centre, &part);
        spheres()[index[j]].centre = *c;
        s = &spheres()[index[j]];
        s->centre.z = float(s->centre.z + 0.20999999344348907);
        spheres()[index[j]].radius = float(VehicleModel_GetUnknown58(modelInfo) * 0.3799999952316284);
    }

    // above the wheels, and between the body's first and second
    spheres()[5].centre = spheres()[0].centre;
    s = &spheres()[5];
    s->centre.x = float(s->centre.x + 0.0);
    s->centre.y = float(s->centre.y - 0.15000000596046448);
    s->centre.z = float(s->centre.z + 0.75);
    spheres()[5].radius = bikeSphereRadius;
    spheres()[6].centre = spheres()[2].centre;
    s = &spheres()[6];
    s->centre.x = float(0.0 + s->centre.x);
    s->centre.y = float(0.15000000596046448 + s->centre.y);
    s->centre.z = float(0.75 + s->centre.z);
    spheres()[6].radius = bikeSphereRadius;
    CVector tmp;
    CVector* c = VectorAdd(&tmp, &spheres()[1].centre, &spheres()[3].centre);
    CVector mid = {float(c->x * 0.5), float(c->y * 0.5), float(c->z * 0.5)};
    spheres()[7].centre = mid;
    s = &spheres()[7];
    s->centre.x = float(s->centre.x + 0.0);
    s->centre.y = float(0.0 + s->centre.y);
    s->centre.z = float(s->centre.z + 0.949999988079071);
    spheres()[7].radius = bikeSphereRadius;
    // (eight made, five counted)
    At<int8_t>(colData, 0x1E) = 5;
    return 1;
}

// ---- an impact --------------------------------------------------------------------------

namespace {

// the ped's damage virtual (vf15)
void PedHit(void* ped, float amount, CEntity* attacker, int weaponType, void* colPoint, CVector* pos,
            const char* name) {
    // (CPed's vf15 CPed_ApplyHit: the node source a pointer, full width)
    using Fn = void(__thiscall*)(void*, float, float, CEntity*, int, void*, CVector*, uintptr_t, int, const char*, int, float,
                                 float, int, int, int, int, float, int);
    Fn fn = re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0x3C / 4]);
    fn(ped, amount, 0.0f, attacker, weaponType, colPoint, pos, 0, 0, name, 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
}

// a ped run over: from behind the bike ("bikerunover") or in front of it ("carrunover")
void RunOver(CBike* self, uint8_t* ped, ColPoint* colPoint, float amount) {
    CVector* pedPos = EntityPos(ped);
    CVector fwdTmp, tmp;
    CVector* fwd = Entity_GetForward(ped, &fwdTmp);
    CVector* d = VectorSub(&tmp, &colPoint->point, pedPos);
    const char* name = DotProduct(d, fwd) > 0.0f ? "carrunover" : "bikerunover";
    CPed* driver = Seats_GetOccupant(&self->seats, 0);
    PedHit(ped, amount, reinterpret_cast<CEntity*>(driver), -5, colPoint, nullptr, name);
}

// "Bike_Hit" and an event (0x1E) about the ped, once a second at most
void HitSoundAndEvent(CBike* self, uint8_t* ped, uint32_t now) {
    if (now <= lastHitSound + 1000) {
        return;
    }
    CVector* pos = EntityPos(self);
    int id = Sounds_GetId(audioFocusPlayer, "Bike_Hit");
    Sounds_PlayAt(audioFocusPlayer, id, pos, 0, pos, 0, 0, 0, 0, 0, 0, 6, 1, 1.0f, 0);
    uint8_t out[0x30];
    Events_Add(eventsCF4378, out, ped, 0, 0, 0x1E, 0, 0, 0, 0, -1, 0);
    lastHitSound = now;
}

} // namespace

void __fastcall ProcessImpact(CBike* self, re::Edx, CVector* speed, ColPoint* colPoint, int crash) {
    uint8_t* entity = re::PtrCast<uint8_t*>(At<CEntity*>(colPoint, 0xC));
    if (!entity || At<int32_t>(entity, 0xC4) != 0) {
        return;
    }
    uint8_t* ped = (At<uint8_t>(entity, 0x108) & 7) == 3 ? entity : nullptr;
    if (Virtual<uint8_t>(entity, 0x7C)) {
        double limit = Virtual<float>(entity, 0x80);
        if (Virtual<float>(self, 0xE4) > limit) {
            return;
        }
    }
    CVector d;
    if (At<int32_t>(entity, 0xF0) != 0) {
        CVector tmp;
        d = *VectorSub(&tmp, speed, &At<CVector>(entity, 0x124));
    } else {
        d = *speed;
    }
    uint32_t now = timeInMs;
    float along = float((double(colPoint->normal.y) * d.y + double(colPoint->normal.x) * d.x) +
                        double(colPoint->normal.z) * d.z);
    float hard = -At<float>(bikeTuning, 0x18);
    if (hard > along) {
        // a hard hit
        if (re::Flag(crash) || Seats_IsDriver(&self->seats, re::PtrCast<CPed*>(playerPed))) {
            if (!ped) {
                Game_CrashRider(self, colPoint);
            }
            Game_OnCollision(self, colPoint);
        }
        if (ped && !At<CVehicle*>(ped, 0x1554) && At<CBike*>(ped, 0x430) != self) {
            if (!ActionController_IsPlaying(At<uint8_t*>(ped, 0x2E0) + 0x258, actionNode1B65270, 1, 1)) {
                RunOver(self, ped, colPoint, 20.0f);
            }
            HitSoundAndEvent(self, ped, now);
        }
    } else if (!(double(hard) * 0.8999999761581421 <= along) ||
               (Seats_GetOccupant(&self->seats, 0) == re::PtrCast<CPed*>(playerPed) && along < 0.0f)) {
        // a softer one: a ped is run over or shoved aside
        if (ped && !At<CVehicle*>(ped, 0x1554)) {
            float m2 = Vector_MagnitudeSqr(speed);
            if (0.001f < m2 || 0.001f < std::fabs(At<float>(self, 0x294))) {
                if (!ActionController_IsPlaying(At<uint8_t*>(ped, 0x2E0) + 0x258, actionNode1B65270, 1, 1)) {
                    if (-double(At<float>(bikeTuning, 0x18)) * 0.699999988079071 > along) {
                        RunOver(self, ped, colPoint, 12.0f);
                    } else {
                        CVector* ownPos = EntityPos(self);
                        CVector* pedPos = EntityPos(ped);
                        float dist = Distance(pedPos, ownPos);
                        CVector fwdTmp, a, b, c, shove;
                        CVector* fwd = Entity_GetForward(self, &fwdTmp);
                        CVector* ahead = VectorScale(&b, VectorScale(&a, fwd, dist), 0.2f);
                        VectorSub(&shove, pedPos, VectorAdd(&c, ownPos, ahead));
                        PedHit(ped, 0.0f, nullptr, -1, nullptr, &shove, "BUMPSHOVE");
                    }
                }
                HitSoundAndEvent(self, ped, now);
            }
        }
    }
    // the rider's pad rumbles by the speed
    if (Seats_GetOccupant(&self->seats, 0) == re::PtrCast<CPed*>(playerPed)) {
        const CVector& v = self->moveSpeed;
        float sq = float((double(v.y) * v.y + double(v.x) * v.x) + double(v.z) * v.z);
        int8_t b = int8_t(Trunc(sq * 200.0));
        int16_t strength = int16_t(b * 0x19);
        Pad_Rumble(0, 0x4B, uint16_t(strength), 1);
    }
}

// ---- the wheels' forces ---------------------------------------------------------------

void __fastcall ProcessWheelForces(CBike* self, re::Edx, float* thrust, float* brake) {
    CVector contactPoints[2] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    Game_ProcessWheelContacts(self, contactPoints);
    uint8_t* handling = self->handling;
    float scale = float(double(At<float>(handling, 0x30)) * floatC2D190 * 0.25);
    CVector f;
    Entity_GetForward(self, &f);
    const CVector& v = self->moveSpeed;
    float fwdSpeed = float((double(v.y) * f.y + double(v.x) * f.x) + double(v.z) * f.z);
    At<float>(self, 0x694) = fwdSpeed;
    float grip = Game_GetBrakeGrip(self, scale);
    Game_4BA620(self, grip);
    *brake = float(double(At<float>(self, 0x298)) * At<float>(self->handling, 0x90) * timeStep);
    if (At<uint8_t>(self, 0x6B8) & 8) {
        *brake = float(timeStep * 1000.0);
    }
    float traction;
    if (Seats_IsDriver(&self->seats, re::PtrCast<CPed*>(playerPed))) {
        float a = std::fabs(At<float>(self, 0x69C));
        traction = float(a / At<float>(self->handling, 0x84));
        At<float>(self, 0x29C) = traction;
    } else {
        traction = 2.0f;
    }
    float pedal = 1.0f;
    int32_t cls;
    if (*Vehicle_GetVehicleClass(self, &cls) == 8) {
        pedal = At<float>(At<uint8_t*>(self, 0x5D4), 0x58);
        if (!Seats_IsDriver(&self->seats, re::PtrCast<CPed*>(playerPed))) {
            if (pedal < 0.5) {
                pedal = 0.5f;
            }
        } else {
            pedal = float(double(At<float>(self, 0x294)) * 0.30000001192092896 + double(pedal) * 0.699999988079071);
        }
    }
    float gas = At<float>(self, 0x294);
    float topSpeed = Game_GetTopSpeed(self);
    if (0.0f < At<float>(self, 0x470)) {
        At<float>(self, 0x29C) = 1.0f;
        traction = 1.0f;
        gas = Vehicle_GetUnknown470(self);
        if (!(fwdSpeed > topSpeed)) {
            topSpeed = fwdSpeed;
        }
    }
    void* transmission = self->handling + 0x34;
    if (At<uint8_t>(self->handling, 0x7E) == 1) {
        *thrust = Transmission_CalculateDriveAccelerationSimple(transmission, &gas, &fwdSpeed, self, pedal, traction,
                                                                topSpeed);
    } else {
        uint8_t& gear = At<uint8_t>(self, 0x344);
        uint8_t old = gear;
        *thrust = Transmission_CalculateDriveAcceleration(transmission, &gas, &gear, &fwdSpeed, self, pedal, traction,
                                                          topSpeed);
        if (old != gear) {
            Game_ShiftGear(self);
        }
    }
    At<float>(self, 0x698) = *thrust;
    float savedBrake = *brake;

    // the wheels' points: their lines' starts less the springs' compression, half the
    // wheel's size up, turned by its matrix
    uint8_t* modelInfo = modelInfoPtrs[self->modelIndex];
    double half = double(VehicleModel_GetUnknown58(modelInfo)) * 0.5;
    uint8_t* lines = At<uint8_t*>(At<uint8_t*>(ColModelOf(self), 0x38), 4);
    CVector local = {0.0f, At<float>(lines, 4),
                     float((double(At<float>(lines, 8)) - double(At<float>(self, 0x680)) * At<float>(self, 0x62C)) -
                           half)};
    struct { CVector v; float pad; } frontPosPad16;   // (the game's transform writes a 4th float)
    CVector& frontPos = frontPosPad16.v;
    Multiply3x3b(&frontPos, Entity_GetMatrix(self), &local);
    local = {0.0f, At<float>(lines, 0x24),
             float((double(At<float>(lines, 0x28)) - double(At<float>(self, 0x684)) * At<float>(self, 0x630)) - half)};
    struct { CVector v; float pad; } rearPosPad16;   // (the game's transform writes a 4th float)
    CVector& rearPos = rearPosPad16.v;
    Multiply3x3b(&rearPos, Entity_GetMatrix(self), &local);
    bool rearFirst = ((At<uint32_t>(self->handling, 0xCC) >> 21) & 1) != 0;
    lines = At<uint8_t*>(At<uint8_t*>(ColModelOf(self), 0x38), 4);
    // (the lines' directions, normalised and never used)
    CVector dir;
    VectorSub(&dir, reinterpret_cast<CVector*>(lines), reinterpret_cast<CVector*>(lines + 0x10));
    Vector_Normalise(&dir);
    // the springs' push along the ground's normals (+0x668 / +0x674 the springs,
    // +0x5EC / +0x614 the normals)
    float dot = float((double(At<float>(self, 0x66C)) * At<float>(self, 0x5F0) +
                       double(At<float>(self, 0x5EC)) * At<float>(self, 0x668)) +
                      double(At<float>(self, 0x670)) * At<float>(self, 0x5F4));
    float frontForce = float(-dot * scale);
    if (!rearFirst) {
        Game_ProcessFrontWheel(self, 0, frontForce, frontPos, *brake, *thrust);
    }
    VectorSub(&dir, reinterpret_cast<CVector*>(lines + 0x20), reinterpret_cast<CVector*>(lines + 0x30));
    Vector_Normalise(&dir);
    dot = float((double(At<float>(self, 0x678)) * At<float>(self, 0x618) +
                 double(At<float>(self, 0x674)) * At<float>(self, 0x614)) +
                double(At<float>(self, 0x67C)) * At<float>(self, 0x61C));
    float rearForce = float(-dot * scale);
    Game_ProcessRearWheel(self, 1, rearForce, rearPos, savedBrake, *thrust);
    if (rearFirst) {
        Game_ProcessFrontWheel(self, 0, frontForce, frontPos, *brake, *thrust);
    }

    // nearly still with no gas (or no traction) and no brake, on the ground: stopped
    if (!(At<float>(self, 0x294) == 0.0f || At<float>(self, 0x29C) == 0.0f)) {
        return;
    }
    if (0.01f < *brake) {
        return;
    }
    if (!(std::fabs(self->moveSpeed.x) < 0.005f) || !(std::fabs(self->moveSpeed.y) < 0.005f) ||
        !(std::fabs(self->moveSpeed.z) < 0.02f)) {
        return;
    }
    if (!(0.9f > At<float>(self, 0x634)) || !(At<float>(self, 0x638) < 0.9f) || At<uint8_t>(self, 0x780)) {
        return;
    }
    // (the move speed's x and y and the turn speed's z)
    self->moveSpeed.x = 0.0f;
    self->moveSpeed.y = 0.0f;
    self->turnSpeed.z = 0.0f;
}

// ---- pushed out of the ground ---------------------------------------------------------

void __fastcall PushOutOfGround(CBike* self, re::Edx) {
    At<uint8_t>(self, 0x780) = 0;
    CVector up = {0.0f, 0.0f, 1.0f};
    if (self->matrix) {
        up = {self->matrix->m[8], self->matrix->m[9], self->matrix->m[10]};
        if (0.0f > up.z) {
            return;
        }
    }
    uint8_t* modelInfo = modelInfoPtrs[self->modelIndex];
    Sphere* spheres = At<Sphere*>(At<uint8_t*>(At<uint8_t*>(modelInfo, 0xC), 0x38), 0);
    // the wheels' spheres (the fourth floats are uninitialised stack in the game)
    CVector4 front = {spheres[0].centre.x, spheres[0].centre.y, spheres[0].centre.z, 0.0f};
    CVector4 rear = {spheres[2].centre.x, spheres[2].centre.y, spheres[2].centre.z, 0.0f};
    Matrix_MultiplyPoint4(&front, Entity_GetMatrix(self), &front);
    Matrix_MultiplyPoint4(&rear, Entity_GetMatrix(self), &rear);
    float depthA = 0.0f;
    float depthB = 0.0f;
    ColPoint hitA = {};
    hitA.depth = 3.4028234663852886e+38f;
    ColPoint hitB = {};
    hitB.depth = 3.4028234663852886e+38f;
    re::Ptr32<CEntity> hitEntity = nullptr;
    ignoreEntity = reinterpret_cast<CEntity*>(self);
    float k = 0.495f;
    if (VehicleModel_GetUnknown58(modelInfo) > 0.7f) {
        k = 0.525f;
    }
    // (both a tenth back along its up)
    front.x = front.x - float(up.x * 0.10000000149011612);
    front.y = front.y - float(up.y * 0.10000000149011612);
    front.z = front.z - float(up.z * 0.10000000149011612);
    rear.x = rear.x - float(up.x * 0.10000000149011612);
    rear.y = rear.y - float(up.y * 0.10000000149011612);
    rear.z = rear.z - float(up.z * 0.10000000149011612);
    // the lines down to 3 below; a hit's depth past where the wheel should rest
    float restA = 0.0f;   // (uninitialised in the game while its line misses; never read then)
    float restB = 0.0f;
    auto cast = [&](CVector4& p, ColPoint& hit, float& rest, float& depth) {
        float s = float(double(std::fabs(self->moveSpeed.z)) * timeStep + 0.10000000149011612);
        CVector along = {s * vec9E33DC.x, s * vec9E33DC.y, s * vec9E33DC.z};
        float zStop = float(p.z - 3.0);
        CVector start;
        VectorAdd(&start, reinterpret_cast<CVector*>(&p), &along);
        if (ProcessVerticalLine(&start, zStop, &hit, &hitEntity, 0x19, 0)) {
            float upZ = self->matrix ? self->matrix->m[10] : 1.0f;
            float r = float(VehicleModel_GetUnknown58(modelInfo) * k);
            rest = p.z - upZ * r;
            float d = hit.point.z - rest;
            depth = d < 0.0 ? 0.0f : d;
        }
    };
    cast(front, hitA, restA, depthA);
    cast(rear, hitB, restB, depthB);
    ignoreEntity = nullptr;
    if (0.001f < depthA || 0.001f < depthB) {
        CVector tmp;
        if (At<float>(self, 0x634) == 1.0f && At<float>(self, 0x638) == 1.0f && !(At<uint8_t>(self, 0x1BC) & 0x10) &&
            double(Physical_GetUp(self, &tmp)->z) < 0.10000000149011612) {
            // lying on its side with both wheels up: slowed, and turned upright
            CVector* speed = &self->moveSpeed;
            Vector_Scale(speed, float(std::pow(0.9599999785423279, double(timeStep))));
            At<float>(self, 0x650) = float(std::pow(0.5, double(timeStep))) * At<float>(self, 0x650);
            At<float>(self, 0x654) = float(std::pow(0.5, double(timeStep))) * At<float>(self, 0x654);
            float sign;
            if (DotProduct(Physical_GetUp(self, &tmp), speed) > 0.0f || Vector_MagnitudeSqr(speed) < 0.08f) {
                sign = 1.0f;
            } else {
                sign = -1.0f;
            }
            // up by (0.7 - twice its speed squared) * 0.8, its mass, 0.008 and the step
            auto right = [&](const CVector& point, float step) {
                float mass = At<float>(self, 0x158);
                CVector upNow;
                Physical_GetUp(self, &upNow);
                float lift = float(0.699999988079071 - 2.0 * Vector_MagnitudeSqr(speed));
                float s = float(double(MaxFloat(0.0f, lift)) * 0.800000011920929 * sign);
                CVector a, b, c, force;
                ScaleVector(&a, s, &upNow);
                VectorScale(&b, &a, mass);
                VectorScale(&c, &b, 0.008f);
                VectorScale(&force, &c, step);
                Physical_ApplyTurnForceV(self, &force, &point);
            };
            CVector com, point, fwd;
            if (0.0f < depthA) {
                // at its back (the front wheel's in the ground)
                Entity_GetForward(self, &fwd);
                CVector back = {-fwd.x, -fwd.y, -fwd.z};
                float step = timeStep;
                Multiply3x3(&com, Entity_GetMatrix(self), &self->centreOfMass);
                VectorAdd(&point, &back, &com);
                right(point, step);
                Game_CheckFallOff(self, &hitA);
            } else {
                float step = timeStep;
                Multiply3x3(&com, Entity_GetMatrix(self), &self->centreOfMass);
                VectorAdd(&point, Entity_GetForward(self, &fwd), &com);
                right(point, step);
                Game_CheckFallOff(self, &hitB);
            }
            At<uint8_t>(self, 0x780) = 1;
        }
        // along the ground's normal unless the ground's steeper than 45 degrees or
        // flatter than about 22 (then straight up), the depth a bit more on a slope
        auto adjust = [](ColPoint& hit, float rest, float& depth) {
            float nz = hit.normal.z;
            if (0.9300000071525574 < nz || 0.707f > nz) {
                hit.normal = vec9E33DC;
            } else {
                float d = float(((1.0 - nz) * 0.3799999952316284 + hit.point.z) - rest);
                depth = d < 0.0 ? 0.0f : d;
            }
        };
        adjust(hitA, restA, depthA);
        adjust(hitB, restB, depthB);
        // each pushed out by its share
        float alongA = depthA * hitA.normal.z;
        float weightA = std::fabs(alongA);
        float alongB = depthB * hitB.normal.z;
        float weightB = std::fabs(alongB);
        float sum = weightB + weightA;
        CVector vB = {hitB.normal.x * depthB, depthB * hitB.normal.y, alongB};
        float kB = weightB / sum;
        CVector pushB = {vB.x * kB, vB.y * kB, kB * alongB};
        CVector vA = {hitA.normal.x * depthA, depthA * hitA.normal.y, alongA};
        float kA = weightA / sum;
        CVector pushA = {vA.x * kA, vA.y * kA, kA * alongA};
        CVector t1, t2;
        VectorAdd(&t1, EntityPos(self), &pushA);
        CVector* p = VectorAdd(&t2, &t1, &pushB);
        Entity_SetPosition(self, p->x, p->y, p->z);
    }
    if (hitB.entity) {
        CleanUpOldReference(hitB.entity, &hitB.entity);
    }
    if (hitA.entity) {
        CleanUpOldReference(hitA.entity, &hitA.entity);
    }
}

// ---- the model --------------------------------------------------------------------------

// 0x4C4820 (cdecl, the animation data's callback: the bike, set at +8, its second argument):
// the wheels' transforms (a jump to 0x4BFD90 with the bike in ecx)
void __cdecl AnimCallback(void*, CBike* bike) {
    Game_UpdateWheelTransforms(bike);
}

void __fastcall SetModelIndex(CBike* self, re::Edx, int model, int arg) {
    Vehicle_SetModelIndex(self, model, arg);
    for (int i = 0; i < 8; i++) {
        self->frames[i] = nullptr;
    }
    Clump_FindFrames(self->rwObject, self->frames);
    At<uint32_t>(self->animData, 4) = bikeAnimCallback;
    At<CBike*>(self->animData, 8) = self;
    At<void*>(self->animData, 0xA4) = &At<uint8_t>(self, 0x5B8);
    At<void*>(self->animData, 0xA8) = &At<uint8_t>(self, 0x5C4);
    At<int32_t>(self, 0x784) = Clump_FindBone(self->animData, "Handlebars");
    At<int32_t>(self, 0x788) = Clump_FindBone(self->animData, "Front_Wheel");
    At<int32_t>(self, 0x78C) = Clump_FindBone(self->animData, "Rear_Wheel");
    At<int32_t>(self, 0x790) = Clump_FindBone(self->animData, "Front_Mud");
}

// ---- the physics --------------------------------------------------------------------------

void __fastcall ProcessPhysics(CBike* self, re::Edx, float* thrust, float* brake) {
    self->centreOfMass = At<CVector>(self->handling, 0x1C);
    if (!At<uint8_t>(self, 0x708)) {
        CVector f;
        self->centreOfMass.y = float(Entity_GetForward(self, &f)->z * 0.25 + self->centreOfMass.y);
    }
    if (At<uint8_t>(self, 0x6B8) & 3) {
        At<float>(self, 0x298) = 1.0f;
    }
    Game_4BA700(self);
    if (!Seats_GetOccupant(&self->seats, 0)) {
        // riderless and the handlebars tipped past 45 degrees: slowed
        NiTransform* transforms = At<NiTransform*>(At<uint8_t*>(self->animData, 0x14), 8);
        veh::CMatrix m;
        m.attached = nullptr;
        Matrix_FromTransform(&m, &transforms[At<int32_t>(self, 0x784)]);
        const CVector& v = self->moveSpeed;
        float sq = float((double(v.y) * v.y + double(v.x) * v.x) + double(v.z) * v.z);
        if (sq > 0.0f && 0.7071f > m.m[10]) {
            float mass = At<float>(self, 0x158);
            float k = At<float>(bikeTuning, 0x58);
            CVector f = {k * -v.x, -v.y * k, k * -v.z};
            Physical_ApplyMoveForce(self, f.x * mass, f.y * mass, mass * f.z);
        }
    }
    Physical_ProcessControl(self);
    Game_ProcessBuoyancy(self);
    Game_ProcessWheelForces(self, thrust, brake);
    CVector speed = self->moveSpeed;
    if (Seats_GetOccupant(&self->seats, 0) && At<int32_t>(Seats_GetOccupant(&self->seats, 0), 0x1310) == 0xD) {
        Game_UpdateRumble(self);
    }
    Game_UpdateLean(self, speed);
    Game_UpdateWheelHeights(self, 0, 1);
    if (At<uint32_t>(self, 0x368) != 0) {
        At<uint32_t>(self, 0x368)--;
    }
    FindPlayerVehicle(0);   // (the result unused)
}

// ---- ProcessControl -----------------------------------------------------------------------

void __fastcall ProcessControl(CBike* self, re::Edx) {
    // (a braking non-pedestrian rider's gas: 0 * +0x29C)
    if ((!Seats_GetOccupant(&self->seats, 0) || At<int32_t>(Seats_GetOccupant(&self->seats, 0), 0x1310) != 0xD) &&
        0.0f < At<float>(self, 0x298)) {
        At<float>(self, 0x294) = float(double(At<float>(self, 0x29C)) * 0.0);
    }
    Game_DoJump(self);
    if (At<int32_t>(self, 0xB0) != 0) {
        return;
    }
    float thrust = 0.0f;
    float brake = 0.0f;
    // the pedalling rate (+0x778): falls by the tuning's +0x24 a step, rises by its +0x2C
    float& rate = At<float>(self, 0x778);
    float value;
    int32_t cls;
    if (*Vehicle_GetVehicleClass(self, &cls) == 8 || *Vehicle_GetVehicleClass(self, &cls) == 3) {
        if (0.0f < Pad_GetValue(7, 0)) {
            value = 1.0f;
        } else {
            rate = float(rate - double(At<float>(bikeTuning, 0x24)) * timeStep);
            value = rate;
            if (value < 0.0f) {
                value = 0.0f;
            }
            if (1.0f < value) {
                value = 1.0f;
            }
        }
    } else if (Seats_GetOccupant(&self->seats, 0) &&
               At<int32_t>(Seats_GetOccupant(&self->seats, 0), 0x1310) != 0xD) {
        // a ped riding: up seven times in eight
        if (Rand() & 7) {
            rate = float(double(At<float>(bikeTuning, 0x2C)) * timeStep + rate);
        } else {
            rate = float(rate - double(At<float>(bikeTuning, 0x24)) * timeStep);
        }
        value = MinFloat(1.0f, MaxFloat(0.0f, rate));
    } else if (timeInMs < At<uint32_t>(self, 0x724) + 400 &&
               At<uint32_t>(self, 0x724) < At<uint32_t>(self, 0x728) + 400) {
        // pedalled twice within 400ms: up to 1.1 (falling a tenth as fast)
        float v = float(rate - double(At<float>(bikeTuning, 0x24)) * 0.10000000149011612 * timeStep);
        value = MinFloat(1.1f, MaxFloat(0.0f, v));
    } else {
        rate = float(rate - double(At<float>(bikeTuning, 0x24)) * timeStep);
        value = rate;
        if (value < 0.0f) {
            value = 0.0f;
        }
        if (1.0f < value) {
            value = 1.0f;
        }
    }
    self->autoPilotFlags &= 0xFC;
    rate = value;
    self->flags33A &= 0xEF;
    At<uint8_t>(self, 0x6B8) &= 0xEB;
    At<uint16_t>(self, 0x6C0) = 0;
    Vehicle_4CC350(self);
    if (self->flags33C & 4) {
        return;
    }
    Vehicle_UpdateFade(self);
    if (!Game_ProcessStatus(self)) {
        return;
    }
    Game_ProcessPedalling(self);
    Game_ProcessBalance(self);
    Game_ProcessStabiliser(self);
    Game_CheckStatic(self);
    Game_4B96C0(self);
    // (the game calls 0x4BDCC0 with whatever ecx 0x4B96C0 left: the bike)
    Game_ProcessCrash(self);
    Game_UpdateColSurface(self);
    if (At<uint8_t>(self, 0x6B8) & 0x20) {
        Game_4BA4E0(self);
    } else {
        Game_ProcessPhysics(self, &thrust, &brake);
    }
    Game_UpdateWheelsLast(self);
    Game_HoldLean(self);
    Game_4B9740(self, thrust, brake);
}

// ---- ProcessCollisions ----------------------------------------------------------------------

namespace {

// a point (the wheel's line's start, times 0.6) by the matrix: translated, then times
// its own matrix (the translation's fourth float is uninitialised stack in the game)
CVector WheelPoint(CBike* self, const uint8_t* start, veh::CMatrix* translate, veh::CMatrix* moved) {
    const float* s = reinterpret_cast<const float*>(start);
    CVector4 p = {float(s[0] * 0.6000000238418579), float(s[1] * 0.6000000238418579),
                  float(s[2] * 0.6000000238418579), 0.0f};
    Matrix_SetTranslate(translate, &p);
    veh::CMatrix product;
    Matrix_Multiply(&product, Entity_GetMatrix(self), translate);
    Matrix_CopyOnlyMatrix(moved, &product);
    return {moved->m[12], moved->m[13], moved->m[14]};
}

void SuspendDoors(CBike* self, int on) {
    CPed* driver = Seats_GetOccupant(&self->seats, 0);
    if (driver && At<int32_t>(Seats_GetOccupant(&self->seats, 0), 0x1310) != 0xD) {
        DoorManager_SuspendShut(doorManager, on);
    } else {
        DoorManager_SuspendHeld(doorManager, on);
    }
}

} // namespace

void __fastcall ProcessCollisions(CBike* self, re::Edx, int) {
    re::Ptr32<CEntity>* ref450 = &self->unknown450;
    if (*ref450 && int8_t(self->unknown454) > 0) {
        self->unknown454--;
    }
    if (At<int32_t>(self, 0xB0) != 0 || self->entityFlags == 0) {
        At<float>(self, 0x19C) = 0.0f;
        return;
    }
    if ((self->typeAndStatus & 0xF8) == 0x10) {
        At<int32_t>(self, 0x34) = 0;
        At<int32_t>(self, 0x38) = 1;
        Physical_RemoveAndAdd(self);
        At<float>(self, 0x19C) = 0.0f;
        return;
    }
    if (At<uint8_t>(self, 0x6B8) & 8) {
        At<float>(self, 0x19C) = 0.0f;
        return;
    }
    SuspendDoors(self, 1);
    uint8_t* colData = At<uint8_t*>(ColModelOf(self), 0x38);
    veh::CMatrix before;
    Matrix_Copy(&before, Entity_GetMatrix(self));
    veh::CMatrix saved;
    Matrix_Copy(&saved, Entity_GetMatrix(self));
    Virtual<void>(self, 0x94);
    CVector offset = {0.0f, 0.0f, 0.0f};
    veh::CMatrix translate, moved;
    translate.attached = nullptr;
    moved.attached = nullptr;
    CVector frontBefore = WheelPoint(self, At<uint8_t*>(colData, 4), &translate, &moved);
    CVector rearBefore = WheelPoint(self, At<uint8_t*>(colData, 4) + 0x20, &translate, &moved);
    Physical_RotateAbout(self, &At<uint8_t>(self, 0x5C4), &At<uint8_t>(self, 0x5B8));
    Virtual<void>(self, 0x98);
    if (self->matrix) {
        Matrix_Reorthogonalise(self->matrix);
    }
    CVector frontAfter = WheelPoint(self, At<uint8_t*>(colData, 4), &translate, &moved);
    CVector rearAfter = WheelPoint(self, At<uint8_t*>(colData, 4) + 0x20, &translate, &moved);

    // kept where the rear wheel stood, or the front one when it's on the ground and
    // the bike is going (+0x634 / +0x638 the compressions, +0x70C / +0x710 the wheels'
    // states: 0 or 3 is gripping)
    bool frontDown = false;
    if (1.0f > At<float>(self, 0x634)) {
        int32_t state = At<int32_t>(self, 0x70C);
        if (state == 0 || state == 3) {
            frontDown = true;
        }
    }
    bool useRear = false;
    if (1.0f > At<float>(self, 0x638)) {
        int32_t state = At<int32_t>(self, 0x710);
        if (state == 0 || state == 3) {
            useRear = !(frontDown &&
                        (0.0f < At<float>(self, 0x470) || At<float>(self, 0x298) > 0.10000000149011612) &&
                        Vector_MagnitudeSqr(&self->moveSpeed) > 0.01f);
        }
    }
    CVector tmp, tmp2;
    if (useRear) {
        CVector* pos = EntityPos(self);
        Physical_SetPositionV(self, VectorAdd(&tmp2, pos, VectorSub(&tmp, &rearBefore, &rearAfter)));
        offset = *VectorSub(&tmp, &rearAfter, EntityPos(self));
    } else if (frontDown) {
        CVector* pos = EntityPos(self);
        Physical_SetPositionV(self, VectorAdd(&tmp2, pos, VectorSub(&tmp, &frontBefore, &frontAfter)));
        offset = *VectorSub(&tmp, &frontAfter, EntityPos(self));
    } else {
        offset = vecUnset;
    }
    Game_LimitSpeed(self);
    Game_PushOutOfGround(self);
    float margin = Seats_GetOccupant(&self->seats, 0) ? 0.0f : 0.1f;
    CVector fwd;
    Entity_GetForward(self, &fwd);
    CVector speed = self->moveSpeed;
    float along = float((double(fwd.y) * speed.y + double(fwd.x) * speed.x) + double(fwd.z) * speed.z);
    Game_UpdateColSurface(self);

    // its collisions: at a height by its speed, then the rest
    float dz = 0.0f;
    ColPoint hit = {};
    hit.depth = 3.4028234663852886e+38f;
    flagA35739 = 0;
    float radius = float(double(std::fabs(along)) * 0.019999999552965164 + margin);
    int32_t result = Vehicle_ResolveCollisionsAtHeight(self, &saved, radius, &dz, &hit, 0, 1, &offset);
    if (result == 1 || result == 2) {
        Game_CheckFallOff(self, &hit);
        Game_ProcessImpact(self, &speed, &hit, 1);
    }
    flagA35739 = 1;
    int32_t result2 =
        Vehicle_ResolveCollisions(self, &saved, &dz, Seats_GetOccupant(&self->seats, 0) != nullptr, &hit, 0, 1);
    if (result2 == 1) {
        Game_CheckFallOff(self, &hit);
    }
    Entity_443AA0(self);
    CVector oldPos = {before.m[12], before.m[13], before.m[14]};
    At<int32_t>(self, 0x34) = 0;
    At<int32_t>(self, 0x38) = 1;
    At<float>(self, 0x19C) = Vector_Magnitude(VectorSub(&tmp, EntityPos(self), &oldPos));
    Physical_RemoveAndAdd(self);
    CVector went = {EntityPos(self)->x, EntityPos(self)->y, 0.0f};
    // ridden into something: its speed across the ground is how far it went (unless
    // the thing it hit is one it can drive over)
    if (Seats_GetOccupant(&self->seats, 0) && (result == 1 || result == 2)) {
        bool keep = true;
        if (hit.entity && Virtual<uint8_t>(hit.entity, 0x7C)) {
            double limit = Virtual<float>(hit.entity, 0x80);
            if (Virtual<float>(self, 0xE4) > limit) {
                keep = false;
            }
        }
        if (keep) {
            Vector_SubAssign(&went, &oldPos);
            Vector_Scale(&went, float(1.0 / timeStep));
            self->moveSpeed.x = went.x;
            self->moveSpeed.y = went.y;
        }
    }
    // riderless against a building: slowed
    if (!Seats_GetOccupant(&self->seats, 0) && result2 == 1 && hit.entity &&
        (At<uint8_t>(hit.entity, 0x108) & 7) == 1) {
        Vector_Scale(&self->moveSpeed, float(std::pow(double(turnFrictionA3570C), double(timeStep))));
        Vector_Scale(&self->turnSpeed, float(std::pow(double(turnFrictionA35710), double(timeStep))));
    }
    if (int8_t(self->unknown454) <= 0) {
        SetRef(ref450, nullptr);
    }
    SuspendDoors(self, 0);
    if (hit.entity) {
        CleanUpOldReference(hit.entity, &hit.entity);
    }
}

// ---- the constructor ------------------------------------------------------------------------

CBike* __fastcall Construct(CBike* self, re::Edx, int model, int createdBy) {
    Vehicle_Construct(self, createdBy);
    self->vtable = static_cast<re::Ptr32<void>*>(bikeVtable);
    for (int i = 0; i < 9; i++) {
        At<float>(self, 0x5A0 + i * 4) = 0.0f;
    }
    At<int32_t>(self, 0x5E8) = 0;
    At<int32_t>(self, 0x610) = 0;
    At<int32_t>(self, 0x6BC) = 0;
    // the wheels' collision points (+0x5DC, +0x604): no point, no entity, no normal,
    // the most depth
    for (int i = 0; i < 2; i++) {
        int p = 0x5DC + i * 0x28;
        At<float>(self, p) = 0.0f;
        At<float>(self, p + 4) = 0.0f;
        At<float>(self, p + 8) = 0.0f;
        At<float>(self, p + 0x10) = 0.0f;
        At<float>(self, p + 0x14) = 0.0f;
        At<float>(self, p + 0x18) = 0.0f;
        At<float>(self, p + 0x24) = 3.4028234663852886e+38f;
    }
    for (int i = 0; i < 6; i++) {
        At<float>(self, 0x668 + i * 4) = 0.0f;
        At<float>(self, 0x6CC + i * 4) = 0.0f;
    }
    Effect_Construct(&At<uint8_t>(self, 0x6F0));
    Effect_Construct(&At<uint8_t>(self, 0x6F8));
    Effect_Construct(&At<uint8_t>(self, 0x718));
    At<int32_t>(self, 0x774) = 0;
    for (int i = 0; i < 4; i++) {
        At<int32_t>(self, 0x794 + i * 4) = 0;
    }
    At<uint8_t>(self, 0x344) = 0;
    At<int32_t>(self, 0x700) = -1;
    At<int32_t>(self, 0x704) = -1;
    // the springs' directions unset, their lengths 0
    At<CVector>(self, 0x668) = vecUnset;
    At<float>(self, 0x680) = 0.0f;
    At<float>(self, 0x688) = 0.0f;
    At<int32_t>(self, 0x6E4) = 0;
    At<float>(self, 0x658) = 0.0f;
    At<float>(self, 0x660) = 0.0f;
    At<CVector>(self, 0x674) = vecUnset;
    At<float>(self, 0x684) = 0.0f;
    At<float>(self, 0x68C) = 0.0f;
    At<float>(self, 0x65C) = 0.0f;
    At<float>(self, 0x664) = 0.0f;
    At<int32_t>(self, 0x6E8) = 0;
    Game_SetModelIndex(self, model, 1);
    Game_SetUp(self, 1);
    Game_LoadSounds(self);
    At<uint8_t>(self, 0x6B8) &= 0xF7;
    self->unknown778 = 0.0f;
    self->jump = 0.0f;
    At<int32_t>(self, 0x714) = 1;
    At<uint8_t>(self, 0x72C) = 0;
    At<uint8_t>(self, 0x72D) = 0;
    At<uint8_t>(self, 0x72E) = 0;
    At<uint8_t>(self, 0x72F) = 0;
    self->unknown724 = 0;
    self->unknown728 = 0;
    At<uint8_t>(self, 0x780) = 0;
    At<uint8_t>(self, 0x730) = 0;
    Matrix_CopyOnlyMatrix(reinterpret_cast<veh::CMatrix*>(&At<uint8_t>(self, 0x734)), Entity_GetMatrix(self));
    return self;
}

} // namespace c11bikeproc

// ---- registration ---------------------------------------------------------------------

#define BIKEPROC_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

BIKEPROC_REVERSED("CBike::ProcessBuoyancy", 0x004C12A0, &c11bikeproc::ProcessBuoyancy);
BIKEPROC_REVERSED("CBike::ProcessWheelContacts", 0x004C1730, &c11bikeproc::ProcessWheelContacts);
BIKEPROC_REVERSED("CBike::Teleport", 0x004C21D0, &c11bikeproc::Teleport);
BIKEPROC_REVERSED("CBike::PreRender", 0x004C22D0, &c11bikeproc::PreRender);
BIKEPROC_REVERSED("CBike::ProcessEntityCollision", 0x004C3C30, &c11bikeproc::ProcessEntityCollision);
BIKEPROC_REVERSED("CBike::SetUpWheelColModel", 0x004C43C0, &c11bikeproc::SetUpWheelColModel);
BIKEPROC_REVERSED("CBike::ProcessImpact", 0x004C4830, &c11bikeproc::ProcessImpact);
BIKEPROC_REVERSED("CBike::ProcessWheelForces", 0x004C4E90, &c11bikeproc::ProcessWheelForces);
BIKEPROC_REVERSED("CBike::PushOutOfGround", 0x004C54A0, &c11bikeproc::PushOutOfGround);
BIKEPROC_REVERSED("CBike::AnimCallback", 0x004C4820, &c11bikeproc::AnimCallback);
BIKEPROC_REVERSED("CBike::SetModelIndex", 0x004C6010, &c11bikeproc::SetModelIndex);
BIKEPROC_REVERSED("CBike::ProcessPhysics", 0x004C6100, &c11bikeproc::ProcessPhysics);
BIKEPROC_REVERSED("CBike::ProcessControl", 0x004C6340, &c11bikeproc::ProcessControl);
BIKEPROC_REVERSED("CBike::ProcessCollisions", 0x004C6660, &c11bikeproc::ProcessCollisions);
BIKEPROC_REVERSED("CBike::Construct", 0x004C6F70, &c11bikeproc::Construct);
