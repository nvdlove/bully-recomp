// The car generators (GTA's CCarGenerator / CTheCarGenerators): 50 places where a
// parked vehicle (in Bully mostly a bike) appears when the player comes near, while
// the camera isn't looking at the spot.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Pools.h"
#include "game/Vehicle.h"
#include "reversed.h"

#include "game/Crt.h"
namespace c11cargen {

using col::CVector;

// 0x30 bytes, CTheCarGenerators::CarGeneratorArray (0xC2D1C8)
struct CCarGenerator {
    int32_t modelIndex;       // +0x0 -1: none (below -1: a random one; its colours then taken from the vehicle)
    CVector pos;              // +0x4
    float angle;              // +0x10 degrees
    int16_t colour1;          // +0x14 -1: the vehicle's own
    int16_t colour2;          // +0x16
    uint8_t forceSpawn;       // +0x18
    uint8_t alarmChance;      // +0x19 percent: the vehicle's +0x354 becomes 2
    uint16_t minDelay;        // +0x1A
    uint16_t maxDelay;        // +0x1C
    uint8_t unknown1E[0x2];
    uint32_t timer;           // +0x20 when it next generates
    int32_t vehicleHandle;    // +0x24 -1: none
    uint16_t usesRemaining;   // +0x28 (0xFFFF: meant as endless, but see DoInternalProcessing)
    uint8_t isBlocking;       // +0x2A
    uint8_t upright;          // +0x2B a bike stands upright (else it's laid on its side)
    re::Ptr32<CEntity> vehicle;         // +0x2C a registered reference

    // 0x4C7210: when it next generates (a random delay between the two, from now)
    uint32_t CalcNextGen();
    // 0x4C7270: the position is within the population's range of the player, and
    // either the screen is fading or it's in view (0.3 behind the camera allowed)
    uint8_t CheckIfWithinRangeOfAnyPlayers(const CVector* position);
    // 0x4C7450
    void Setup(float x, float y, float z, float heading, int model, int16_t col1, int16_t col2, uint8_t force,
               uint8_t unused, uint8_t alarm, uint16_t minDel, uint16_t maxDel, uint8_t isUpright);
    // 0x4C7590: something whose height overlaps the model's (1 m slack) stands in the
    // way (then +0x2A is set)
    uint8_t CheckForBlockage(int model);
    // 0x4C7700: the vehicle made and placed
    void DoInternalProcessing();
    // 0x4C7B10
    void Process();
};
static_assert(offsetof(CCarGenerator, pos) == 0x4);
static_assert(offsetof(CCarGenerator, angle) == 0x10);
static_assert(offsetof(CCarGenerator, colour1) == 0x14);
static_assert(offsetof(CCarGenerator, forceSpawn) == 0x18);
static_assert(offsetof(CCarGenerator, alarmChance) == 0x19);
static_assert(offsetof(CCarGenerator, minDelay) == 0x1A);
static_assert(offsetof(CCarGenerator, maxDelay) == 0x1C);
static_assert(offsetof(CCarGenerator, timer) == 0x20);
static_assert(offsetof(CCarGenerator, vehicleHandle) == 0x24);
static_assert(offsetof(CCarGenerator, usesRemaining) == 0x28);
static_assert(offsetof(CCarGenerator, isBlocking) == 0x2A);
static_assert(offsetof(CCarGenerator, upright) == 0x2B);
static_assert(offsetof(CCarGenerator, vehicle) == 0x2C);
static_assert(sizeof(CCarGenerator) == 0x30);

// CTheCarGenerators
GAME_VAR(uint8_t, generatorsOn, 0x00C2D1A8);                     // (the script's bike generator toggle)
GAME_VAR(uint8_t, generateEvenIfPlayerIsCloseCounter, 0x00C2D1A9);
GAME_VAR(uint8_t, processCounter, 0x00C2D1AA);
GAME_VAR(int32_t, areaTransitionModel, 0x00C2D1AC);             // the model used while an area loads
GAME_VAR(int32_t, currentActiveCount, 0x00C2D1B0);
GAME_VAR(int32_t, numOfCarGenerators, 0x00C2D1B4);
GAME_ARRAY(CCarGenerator, carGeneratorArray, 0x00C2D1C8);

// 0x4C71E0
void __cdecl Init();
// 0x4C7200
void __cdecl SetGeneratorsOn(uint8_t on);
// 0x4C74E0: a new one (up to 50), endless; returns its index (the last one's when full)
int32_t __cdecl CreateCarGenerator(float x, float y, float z, float heading, int model, int16_t col1, int16_t col2,
                                   uint8_t force, uint8_t unused, uint8_t alarm, uint16_t minDel, uint16_t maxDel,
                                   uint8_t isUpright);
// 0x4C7C40: a quarter of them each frame
void __cdecl Process();

} // namespace c11cargen

namespace {

using namespace c11cargen;

GAME_FN(uint32_t, __thiscall, Gen_CalcNextGen, 0x004C7210, CCarGenerator* gen);
GAME_FN(uint8_t, __thiscall, Gen_CheckIfWithinRange, 0x004C7270, CCarGenerator* gen, const CVector* pos);
GAME_FN(void, __thiscall, Gen_Setup, 0x004C7450, CCarGenerator* gen, float x, float y, float z, float heading, int model,
        int16_t col1, int16_t col2, uint8_t force, uint8_t unused, uint8_t alarm, uint16_t minDel, uint16_t maxDel,
        uint8_t isUpright);
GAME_FN(uint8_t, __thiscall, Gen_CheckForBlockage, 0x004C7590, CCarGenerator* gen, int model);
GAME_FN(void, __thiscall, Gen_DoInternalProcessing, 0x004C7700, CCarGenerator* gen);
GAME_FN(void, __thiscall, Gen_Process, 0x004C7B10, CCarGenerator* gen);

inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<CEntity>* ref, CEntity* entity);
GAME_FN(CVector*, __cdecl, FindPlayerCoors, 0x004469E0, uintptr_t player);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(int, __thiscall, Camera_GetFadeState, 0x004F12F0, void* camera);
GAME_FN(uint8_t, __thiscall, Population_IsSphereVisible, 0x0049C930, void* population, const CVector* pos);
GAME_FN(uint8_t, __thiscall, Population_CanSpawnNear, 0x0041FF20, void* population, const CVector* pos);
GAME_FN(int, __thiscall, Pool_GetNoOfFreeSpaces, 0x0041EDF0, void* pool);
GAME_FN(void, __cdecl, FindObjectsInRange, 0x0045E3C0, const CVector* pos, float range, int twoD, int16_t* count,
        int16_t maxCount, re::Ptr32<CEntity>* out, uint32_t lists);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(uint8_t, __thiscall, AreaTransition_IsLoading, 0x00415380, void* transition);
GAME_FN(float, __cdecl, FindGroundZ, 0x004635A0, float x, float y, float z, uint8_t* found, int arg);
GAME_FN(void*, __cdecl, Vehicle_New, 0x004CC490, uintptr_t size);
GAME_FN(CVehicle*, __thiscall, Bike_Construct, 0x004C6F70, void* self, int model, int createdBy);
GAME_FN(CVehicle*, __thiscall, Automobile_Construct, 0x004B7B80, void* self, int model, int createdBy);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(float, __cdecl, LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float heading);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(CVector*, __thiscall, Entity_GetRight, 0x004736F0, void* entity, CVector* out);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVector* out);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __cdecl, SetObjectAlpha, 0x005F2B00, void* rwObject, int alpha);

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, inFrontEnd, 0x020C5BE6);
GAME_VAR(CEntity*, playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, population, 0x00C2C108);     // (its +0x7830: the cull distance)
GAME_VAR(void*, population2, 0x00BCD080);
GAME_VAR(void*, rwCamera, 0x00C3CCF4);
GAME_VAR(int32_t, areaTransitionModelSource, 0x00BF411C);
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const areaTransition = reinterpret_cast<void*>(0x00BCC580);

template <typename R, typename... A>
R CallVirtual(void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(object, args...);
}

// an entity's position (its matrix's, else its placement's)
const CVector& Position(const void* entity) {
    auto e = static_cast<const uint8_t*>(entity);
    uint8_t* matrix = *reinterpret_cast<const re::Ptr32<uint8_t>*>(e + 0x14);
    return *reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : e + 4);
}

// a model's collision model (CBaseModelInfo +0xC)
CColModel* ModelColModel(int model) {
    return *reinterpret_cast<re::Ptr32<CColModel>*>(modelInfoPtrs[model] + 0xC);
}

} // namespace

namespace c11cargen {

void __cdecl Init() {
    numOfCarGenerators = 0;
    currentActiveCount = 0;
    processCounter = 0;
    generateEvenIfPlayerIsCloseCounter = 0;
}

void __cdecl SetGeneratorsOn(uint8_t on) {
    generatorsOn = on;
}

uint32_t CCarGenerator::CalcNextGen() {
    if (minDelay != maxDelay) {
        int range = (int(maxDelay) - 1) - int(minDelay);
        return uint32_t(Rand() * range / 0x7FFF + int(minDelay)) + timeInMs;
    }
    return timeInMs + 4000;
}

uint8_t CCarGenerator::CheckIfWithinRangeOfAnyPlayers(const CVector* position) {
    CVector player = *FindPlayerCoors(0);
    CVector d;
    VectorSub(&d, position, &player);
    float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
    float dist = float(std::sqrt(double(std::fabs(distSq))));
    if (*reinterpret_cast<float*>(population + 0x7830) < dist) {
        return 0;
    }

    CVector camPos = {0.0f, 0.0f, 0.0f};
    float* transform = GetLocalTransform(GetCameraNode(rwCamera));
    camPos = *reinterpret_cast<CVector*>(transform + 9);
    transform = GetLocalTransform(GetCameraNode(rwCamera));
    CVector camDir = {transform[2], transform[5], transform[8]};
    Vector_Normalise(&camDir);
    CVector toPos;
    VectorSub(&toPos, position, &camPos);
    Vector_Normalise(&toPos);

    bool fading = Camera_GetFadeState(theCamera) != 0;
    CVector copy = *position;
    bool inView = false;
    if (Population_IsSphereVisible(population, &copy)) {
        float dot = float(double(toPos.y) * camDir.y + double(camDir.x) * toPos.x + double(toPos.z) * camDir.z);
        inView = dot > -0.3f;
    }
    return (fading || inView) ? 1 : 0;
}

void CCarGenerator::Setup(float x, float y, float z, float heading, int model, int16_t col1, int16_t col2, uint8_t force,
                          uint8_t, uint8_t alarm, uint16_t minDel, uint16_t maxDel, uint8_t isUpright) {
    pos.x = x;
    modelIndex = model;
    pos.y = y;
    colour1 = col1;
    pos.z = z;
    colour2 = col2;
    angle = heading;
    forceSpawn = force;
    alarmChance = alarm;
    minDelay = minDel;
    maxDelay = maxDel;
    vehicleHandle = -1;
    uint32_t now = timeInMs;
    // (the next time already counts from now: now is added twice)
    timer = Gen_CalcNextGen(this) + now;
    upright = isUpright;
    usesRemaining = 0;
    isBlocking = 0;
    SetRef(&vehicle, nullptr);
}

uint8_t CCarGenerator::CheckForBlockage(int model) {
    CColModel* genCol = ModelColModel(model);
    CVector at = pos;
    int16_t count;
    re::Ptr32<CEntity> found[8];
    FindObjectsInRange(&at, genCol->sphereRadius, 1, &count, 8, found, 6);
    for (int i = 0; i < count; i++) {
        CEntity* entity = found[i];
        CColModel* col = ModelColModel(reinterpret_cast<int16_t*>(entity)[0x10E / 2]);
        double entityZ = Position(entity).z;
        if (double(pos.z) + genCol->boxMin.z < entityZ + col->boxMax.z + 1.0 &&
            entityZ + col->boxMin.z - 1.0 < double(pos.z) + genCol->boxMax.z) {
            isBlocking = 1;
            return 1;
        }
    }
    return 0;
}

void CCarGenerator::DoInternalProcessing() {
    int model = -1;
    if (modelIndex >= 0) {
        RequestModel(modelIndex, 0xC);
        model = modelIndex;
    }
    if (Gen_CheckForBlockage(this, model)) {
        timer += 4000;
        return;
    }
    if (GetStreamingInfo(model)[4] != 1) {
        if (!AreaTransition_IsLoading(areaTransition) || GetStreamingInfo(areaTransitionModel)[4] != 1) {
            return;
        }
        model = areaTransitionModel;
    }

    float x = pos.x;
    float y = pos.y;
    uint8_t foundGround;
    float groundZ = FindGroundZ(x, y, pos.z, &foundGround, 1);
    if (!foundGround) {
        groundZ = pos.z;
    }

    bool isBike = *reinterpret_cast<int32_t*>(modelInfoPtrs[model] + 0x50) == 1;
    CVehicle* v;
    if (isBike) {
        if (generatorsOn != 1) {
            return;
        }
        void* memory = Vehicle_New(0x7A4);
        if (!memory) {
            return;
        }
        v = Bike_Construct(memory, model, 3);
    } else {
        void* memory = Vehicle_New(0x8C0);
        if (!memory) {
            return;
        }
        v = Automobile_Construct(memory, model, 3);
    }
    if (!v) {
        return;
    }

    v->flags33A &= ~8;
    CallVirtual<void>(v, 7, 0);   // SetIsStatic
    v->flags33A &= ~2;
    Entity_SetPosition(v, x, y, groundZ);
    float heading = float(double(angle) * 3.1415927410125732 / 180.0);
    heading = LimitRadianAngle(heading);
    if (v->matrix) {
        Matrix_SetRotateZOnly(v->matrix, heading);
    }
    *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(v) + 0x10) = heading;
    CallVirtual<void>(v, 6, 4);   // SetStatus
    v->unknown354 = 1;
    veh::SetRef(&v->owner, playerPed);

    float* m = Entity_GetMatrix(v);
    if (isBike && !upright) {
        // laid on its side: its up is its right. (The game also stores uninitialised
        // stack into the matrix's +0x2C; ours leaves it)
        CVector right1, right2, right3;
        float rx = Entity_GetRight(v, &right1)->x;
        float ry = Entity_GetRight(v, &right2)->y;
        float rz = Entity_GetRight(v, &right3)->z;
        m[8] = rx;
        m[9] = ry;
        m[10] = rz;
    } else {
        // up: (its right + 0.1 up) x its forward. (+0x2C as above)
        CVector lift = {0.0f, 0.0f, 0.1f};
        CVector forward, right, sum, up;
        Entity_GetForward(v, &forward);
        CrossProduct(&up, VectorAdd(&sum, Entity_GetRight(v, &right), &lift), &forward);
        m[8] = up.x;
        m[9] = up.y;
        m[10] = up.z;
        CallVirtual<void>(v, 6, 2);   // SetStatus
    }
    World_Add(v);

    float h;
    if (v->matrix) {
        h = float(std::atan2(-double(v->matrix->m[4]), double(v->matrix->m[5])));
    } else {
        h = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(v) + 0x10);
    }
    CallVirtual<void>(v, 53, x, y, groundZ, h);
    SetRef(&vehicle, reinterpret_cast<CEntity*>(v));

    if (Rand() * 99 / 0x7FFF < int(alarmChance)) {
        v->unknown354 = 2;
    }
    if (colour1 != -1 && colour2 != -1) {
        v->colour1 = uint8_t(colour1);
        v->colour2 = uint8_t(colour2);
    } else if (modelIndex < -1) {
        colour1 = v->colour1;
        colour2 = v->colour2;
    }
    SetObjectAlpha(v->rwObject, 1);

    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    int slot = int((reinterpret_cast<uint8_t*>(v) - re::PtrCast<uint8_t*>(pool->objects)) / pool->elemSize);
    vehicleHandle = (slot << 8) + pool->flags[slot];
    // (compared with a 32-bit -1, which a 16-bit count never is: always counted down)
    if (uint32_t(usesRemaining) != 0xFFFFFFFFu) {
        usesRemaining--;
    }
    timer = Gen_CalcNextGen(this);
    if (usesRemaining == 0) {
        currentActiveCount--;
    }
}

void CCarGenerator::Process() {
    if (currArea != 0) {
        return;
    }
    if (vehicleHandle == -1 &&
        (generateEvenIfPlayerIsCloseCounter != 0 || Camera_GetFadeState(theCamera) != 0 || timeInMs >= timer) &&
        usesRemaining != 0) {
        if (Pool_GetNoOfFreeSpaces(CPools::ms_pVehiclePool) > 0) {
            CVector at = pos;
            if (!(vehicle && Gen_CheckIfWithinRange(this, &Position(vehicle))) && Gen_CheckIfWithinRange(this, &at) &&
                Population_CanSpawnNear(population2, FindPlayerCoors(0))) {
                Gen_DoInternalProcessing(this);
            }
        }
    }

    if (vehicleHandle == -1) {
        return;
    }
    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    int slot = vehicleHandle >> 8;
    CVehicle* v = nullptr;
    if (pool->flags[slot] == uint8_t(vehicleHandle)) {
        v = reinterpret_cast<CVehicle*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
    }
    if (!v) {
        vehicleHandle = -1;
        return;
    }
    if ((v->typeAndStatus & 0xF8) == 0) {   // the player took it
        timer += 60000;
        vehicleHandle = -1;
        isBlocking = 1;
        if (modelIndex < 0) {
            modelIndex = -1;
        }
    }
}

int32_t __cdecl CreateCarGenerator(float x, float y, float z, float heading, int model, int16_t col1, int16_t col2,
                                   uint8_t force, uint8_t unused, uint8_t alarm, uint16_t minDel, uint16_t maxDel,
                                   uint8_t isUpright) {
    if (uint32_t(numOfCarGenerators) < 50) {
        CCarGenerator* gen = &carGeneratorArray[numOfCarGenerators];
        Gen_Setup(gen, x, y, z, heading, model, col1, col2, force, unused, alarm, minDel, maxDel, isUpright);
        gen->usesRemaining = 0xFFFF;
        gen->timer = Gen_CalcNextGen(gen);
        currentActiveCount++;
        numOfCarGenerators++;
    }
    areaTransitionModel = areaTransitionModelSource;
    generatorsOn = 1;
    return numOfCarGenerators - 1;
}

void __cdecl Process() {
    if (inFrontEnd) {
        return;
    }
    processCounter++;
    if (processCounter == 4) {
        processCounter = 0;
    }
    for (int i = processCounter; i < numOfCarGenerators; i += 4) {
        Gen_Process(&carGeneratorArray[i]);
    }
    if (generateEvenIfPlayerIsCloseCounter) {
        generateEvenIfPlayerIsCloseCounter--;
    }
}

} // namespace c11cargen

// ---- registration ---------------------------------------------------------------------

#define CARGEN_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)
#define CARGEN_MEMBER(name, address, member) CARGEN_REVERSED(name, address, member)

CARGEN_REVERSED("CTheCarGenerators::Init", 0x004C71E0, &c11cargen::Init);
CARGEN_REVERSED("CTheCarGenerators::SetGeneratorsOn", 0x004C7200, &c11cargen::SetGeneratorsOn);
CARGEN_MEMBER("CCarGenerator::CalcNextGen", 0x004C7210, &c11cargen::CCarGenerator::CalcNextGen);
CARGEN_MEMBER("CCarGenerator::CheckIfWithinRangeOfAnyPlayers", 0x004C7270,
              &c11cargen::CCarGenerator::CheckIfWithinRangeOfAnyPlayers);
CARGEN_MEMBER("CCarGenerator::Setup", 0x004C7450, &c11cargen::CCarGenerator::Setup);
CARGEN_REVERSED("CTheCarGenerators::CreateCarGenerator", 0x004C74E0, &c11cargen::CreateCarGenerator);
CARGEN_MEMBER("CCarGenerator::CheckForBlockage", 0x004C7590, &c11cargen::CCarGenerator::CheckForBlockage);
CARGEN_MEMBER("CCarGenerator::DoInternalProcessing", 0x004C7700, &c11cargen::CCarGenerator::DoInternalProcessing);
CARGEN_MEMBER("CCarGenerator::Process", 0x004C7B10, &c11cargen::CCarGenerator::Process);
CARGEN_REVERSED("CTheCarGenerators::Process", 0x004C7C40, &c11cargen::Process);
