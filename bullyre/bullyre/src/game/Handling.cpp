// The vehicles' handling (GTA's cHandlingDataMgr / tHandlingData / tTransmissionData):
// HANDLING.CFG read into 120 entries, converted to game units, and the transmission's
// gears and drive force.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Vehicle.h"
#include "reversed.h"

#include "game/Crt.h"
namespace c11handling {

struct tGear {
    float maxVelocity;          // +0x0
    float changeUpVelocity;     // +0x4
    float changeDownVelocity;   // +0x8
};
static_assert(sizeof(tGear) == 0xC);

struct tTransmissionData {
    tGear gears[6];             // +0x0 (0: reverse)
    char driveType;             // +0x48 '4', 'F', 'R'
    char engineType;            // +0x49
    uint8_t numberOfGears;      // +0x4A
    uint8_t flags;              // +0x4B (the handling flags' low byte: 1 and 2 shape the gear factor)
    float engineAcceleration;   // +0x4C
    float maxVelocity;          // +0x50
    float maxReverseVelocity;   // +0x54
    float currentVelocity;      // +0x58

    // 0x4CA740: the gears' speeds from the top speed
    void InitGearRatios();
    // 0x4CA800: the gear changed as the speed asks (recursively), then the drive force
    // for this frame
    float CalculateDriveAcceleration(float* gasPedal, uint8_t* gear, float* velocity, CVehicle* vehicle, float arg5,
                                     float speedFactor, float driverSpeedLimit);
    // 0x4CAC20: the same without gears
    float CalculateDriveAccelerationSimple(float* gasPedal, float* velocity, CVehicle* vehicle, float arg4,
                                           float speedFactor, float driverSpeedLimit);
};
static_assert(offsetof(tTransmissionData, driveType) == 0x48);
static_assert(offsetof(tTransmissionData, numberOfGears) == 0x4A);
static_assert(offsetof(tTransmissionData, engineAcceleration) == 0x4C);
static_assert(offsetof(tTransmissionData, maxVelocity) == 0x50);
static_assert(offsetof(tTransmissionData, currentVelocity) == 0x58);
static_assert(sizeof(tTransmissionData) == 0x5C);

// 0xDC bytes (the fields named where the loader or the conversion says what they are)
struct tHandlingData {
    int32_t index;                  // +0x0
    float mass;                     // +0x4
    float invMass;                  // +0x8
    float turnMass;                 // +0xC
    float dimensions[3];            // +0x10
    float centreOfMass[3];          // +0x1C
    uint8_t percentSubmerged;       // +0x28
    uint8_t unknown29[0x3];
    float buoyancyConstant;         // +0x2C
    float tractionMultiplier;       // +0x30
    tTransmissionData transmission; // +0x34
    float unknown90;                // +0x90 (scaled like the acceleration)
    float unknown94;                // +0x94
    uint8_t unknown98;              // +0x98
    uint8_t unknown99[0x3];
    float unknown9C;                // +0x9C
    float unknownA0;                // +0xA0
    float unknownA4;                // +0xA4
    uint8_t unknownA8[0x4];
    float unknownAC;                // +0xAC
    float unknownB0;                // +0xB0
    float unknownB4[4];             // +0xB4
    float unknownC4;                // +0xC4 (x 2000 / mass)
    float unknownC8;                // +0xC8
    uint32_t handlingFlags;         // +0xCC (hex)
    float unknownD0;                // +0xD0
    int32_t unknownD4;              // +0xD4
    uint8_t unknownD8;              // +0xD8
    uint8_t unknownD9;              // +0xD9
    uint8_t unknownDA[0x2];
};
static_assert(offsetof(tHandlingData, dimensions) == 0x10);
static_assert(offsetof(tHandlingData, percentSubmerged) == 0x28);
static_assert(offsetof(tHandlingData, transmission) == 0x34);
static_assert(offsetof(tHandlingData, unknown90) == 0x90);
static_assert(offsetof(tHandlingData, unknownC4) == 0xC4);
static_assert(offsetof(tHandlingData, handlingFlags) == 0xCC);
static_assert(offsetof(tHandlingData, unknownD8) == 0xD8);
static_assert(sizeof(tHandlingData) == 0xDC);

// 0x6F30 bytes
struct cHandlingDataMgr {
    float unknown0[5];                  // +0x0 (0.1, 0.9, 1, 0.8, 0.98)
    tHandlingData handlingData[120];    // +0x14
    uint8_t unknown6734[0x6F30 - 0x6734];   // the "!" (0x5C) and "%" (0x3C, ids 100..110 at +0x6C98) records

    // 0x4C9B70: the index of the word among `count` words `stride` apart (count: none)
    int32_t FindExactWord(const char* word, const char* words, int stride, int count);
    // 0x4C9BE0: a vehicle's handling index by its name (120: none)
    int32_t GetHandlingId(const char* name);
    // 0x4C9C20: the loaded values made game units (and the top speed found by
    // stepping it down until the drag no longer beats the acceleration)
    void ConvertDataToGameUnits(tHandlingData* data);
    // 0x4C9DF0: a "!" record's angles (degrees) into sines / radians
    void ConvertBikeDataToGameUnits(uint8_t* data);
    // 0x4C9E80: Config\HANDLING.CFG
    void LoadHandlingData();
    // 0x4CA680: the constructor (everything zeroed; returns this)
    cHandlingDataMgr* Construct();
    // 0x4CA700
    void Initialise();
};
static_assert(offsetof(cHandlingDataMgr, handlingData) == 0x14);
static_assert(sizeof(cHandlingDataMgr) == 0x6F30);

} // namespace c11handling

namespace {

using namespace c11handling;
using veh::CVector;

GAME_FN(int32_t, __thiscall, Mgr_FindExactWord, 0x004C9B70, cHandlingDataMgr* mgr, const char* word, const char* words,
        int stride, int count);
GAME_FN(void, __thiscall, Mgr_ConvertDataToGameUnits, 0x004C9C20, cHandlingDataMgr* mgr, tHandlingData* data);
GAME_FN(void, __thiscall, Mgr_ConvertBikeDataToGameUnits, 0x004C9DF0, cHandlingDataMgr* mgr, uint8_t* data);
GAME_FN(void, __thiscall, Mgr_LoadHandlingData, 0x004C9E80, cHandlingDataMgr* mgr);
GAME_FN(void, __thiscall, Transmission_InitGearRatios, 0x004CA740, tTransmissionData* t);
GAME_FN(float, __thiscall, Transmission_CalculateDriveAcceleration, 0x004CA800, tTransmissionData* t, float* gasPedal,
        uint8_t* gear, float* velocity, CVehicle* vehicle, float arg5, float speedFactor, float driverSpeedLimit);
GAME_FN(tTransmissionData*, __thiscall, Transmission_Construct, 0x0066BEA0, tTransmissionData* t);

// the game's C runtime (strtok's state is the game's)
template <typename... A>
inline int __cdecl Sprintf(char* out, const char* format, A... rest) { return crt::Sprintf(out, format, rest...); }
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
inline int __cdecl Strncmp(const char* a, const char* b, size_t n) { return crt::Strncmp(a, b, n); }
inline char* __cdecl Strtok(char* s, const char* delimiters) { return crt::Strtok(s, delimiters); }
inline double __cdecl Atof(const char* s) { return crt::Atof(s); }
inline int __cdecl Atoi(const char* s) { return (int)crt::Atol(s); }
inline void* __cdecl Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
// reads a whole file into the buffer
GAME_FN(int, __cdecl, LoadFile, 0x0042D180, const char* path, char* buffer, int size, const char* mode);

GAME_FN(CPed*, __thiscall, Seats_GetOccupant, 0x004CE560, CVehicleSeats* seats, uint32_t seat);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, CVehicle* entity);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, CVehicle* entity, CVector* out);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);

GAME_VAR(const char*, handlingFileName, 0x00A357C0);   // "HANDLING.CFG"
inline char* const fileBuffer = reinterpret_cast<char*>(0x00BD7800);   // 0x1C000 bytes
inline const char* const vehicleNames = reinterpret_cast<const char*>(0x00A357C8);   // 120 x 14 chars
GAME_VAR(float, accelerationMultiplier, 0x00C2DEC0);
GAME_VAR(float, velocityMultiplier, 0x00C2DEC8);
GAME_VAR(uint8_t*, gameMode, 0x020C8704);      // (+4 set and +0x3C 14: the handling cheat)
GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(tGear*, lastGear, 0x00C34E04);        // (InitGearRatios leaves its last two gears here)
GAME_VAR(tGear*, lastPrevGear, 0x00C34E00);

template <typename R, typename... A>
R CallVirtual(void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return re::PtrCast<R(__thiscall*)(void*, re::Arg<A>...)>(vtable[slot])(object, args...);
}

using re::At;

bool HandlingCheat() {
    return gameMode && gameMode[4] && *reinterpret_cast<int32_t*>(gameMode + 0x3C) == 0xE;
}

// the driver is a ped of kind 13 (+0x1310)
bool DriverIsKind13(CVehicle* vehicle) {
    return Seats_GetOccupant(&vehicle->seats, 0) &&
           At<int32_t>(Seats_GetOccupant(&vehicle->seats, 0), 0x1310) == 0xD;
}

// the drive force's common end (both variants): `limit` is the top speed times the
// factor, `factor` the gear factor
float DriveForce(tTransmissionData* t, float* gasPedal, float velocity, CVehicle* vehicle, float arg, float speedFactor,
                 float factor, bool cheat) {
    float maxVelocity = t->maxVelocity;
    if (vehicle->vehicleType == 0 && Seats_GetOccupant(&vehicle->seats, 0) &&
        At<int32_t>(Seats_GetOccupant(&vehicle->seats, 0), 0x1310) != 0xD && cheat) {
        maxVelocity = float(maxVelocity * 10.0);
    }
    double target = double(maxVelocity) * speedFactor;
    float force = float((target - velocity) * t->engineAcceleration * factor);
    float absTarget = std::fabs(float(target));
    if (absTarget > 0.0f) {
        force = force / absTarget;
    }
    float result = 0.0f;
    if (std::fabs(velocity) < absTarget || cheat) {
        result = float(double(force) * *gasPedal * timeStep);
    }
    result = vehicle->accelerationMult * result;

    float forwardZ = Entity_GetMatrix(vehicle)[6];
    if (!(0.0f < arg) || !(0.01f < forwardZ)) {
        return result;
    }
    if (*gasPedal > 0.10000000149011612 || (vehicle->vehicleType == 1 && 0.01f < static_cast<CBike*>(vehicle)->jump)) {
        // uphill: gravity's share given back
        float ts = timeStep;
        float g = CallVirtual<float>(vehicle, 35);
        return float(double(forwardZ) * arg * g * ts + result);
    }
    if (!(-0.01f < *gasPedal)) {
        return result;
    }
    CVector forward;
    if (!(DotProduct(&vehicle->moveSpeed, Entity_GetForward(vehicle, &forward)) > 0.005f)) {
        return result;
    }
    float ts = timeStep;
    float g = CallVirtual<float>(vehicle, 35);
    return float(double(arg) * 0.10000000149011612 * forwardZ * g * ts + result);
}

} // namespace

namespace c11handling {

int32_t cHandlingDataMgr::FindExactWord(const char* word, const char* words, int stride, int count) {
    for (int i = 0;;) {
        size_t length = std::strlen(words);
        char copy[20];
        Strncpy(copy, words, length);
        if (Strncmp(word, copy, length) == 0) {
            return i;
        }
        words += stride;
        i++;
        if (i >= count) {
            return count;
        }
    }
}

int32_t cHandlingDataMgr::GetHandlingId(const char* name) {
    int32_t id = 0;
    for (const char* entry = vehicleNames; entry < vehicleNames + 120 * 14; entry += 14) {
        if (Strncmp(name, entry, 14) == 0) {
            break;
        }
        id++;
    }
    return id;
}

void cHandlingDataMgr::ConvertDataToGameUnits(tHandlingData* h) {
    double accelMult = accelerationMultiplier;
    float acceleration = float(accelMult * h->transmission.engineAcceleration);
    h->transmission.engineAcceleration = acceleration;
    float velocity = float(double(h->transmission.maxVelocity) * velocityMultiplier);
    h->transmission.maxVelocity = velocity;
    h->unknown90 = float(accelMult * h->unknown90);

    float turnMass =
        float((double(h->dimensions[1]) * h->dimensions[1] + double(h->dimensions[0]) * h->dimensions[0]) * h->mass / 12.0);
    h->turnMass = turnMass;
    if (turnMass < 10.0f) {
        h->turnMass = float(turnMass * 5.0);
    }
    h->invMass = float(1.0 / h->mass);
    h->buoyancyConstant = float(double(h->mass) * 0.00800000037997961 * 100.0 / int(h->percentSubmerged));
    h->unknownC4 = float(double(h->unknownC4) * 2000.0 / h->mass);
    float drag = float(double(h->dimensions[0]) * 0.5 * h->dimensions[2] / h->mass);

    // the top speed: stepped down by 0.01 until the drag no longer beats the acceleration
    float top = velocity;
    while (0.0f < top) {
        top = float(top - 0.01);
        float limit = float(double(acceleration) * 0.1666666865348816);
        double x = top;
        float dragForce = float(-x * (1.0 / (x * drag * x + 1.0) - 1.0));
        if (!(dragForce > limit)) {
            break;
        }
    }

    float reverse;
    if (h->index < 0x55 || h->index > 0x63) {
        h->transmission.maxVelocity = top;
        reverse = float(top * -0.3499999940395355);
        if (reverse < -0.20000000298023224) {
            reverse = -0.2f;
        }
    } else {
        h->transmission.maxVelocity = top;
        reverse = -0.1f;
    }
    h->transmission.maxReverseVelocity = reverse;
    if (h->transmission.driveType == '4') {
        h->transmission.engineAcceleration = float(h->transmission.engineAcceleration * 0.25);
    } else {
        h->transmission.engineAcceleration = float(h->transmission.engineAcceleration * 0.5);
    }
    Transmission_InitGearRatios(&h->transmission);
}

void cHandlingDataMgr::ConvertBikeDataToGameUnits(uint8_t* data) {
    constexpr double kPi = 3.1415927410125732;
    At<float>(data, 0x14) = float(std::sin(double(float(At<float>(data, 0x14) * kPi / 180.0))));
    At<float>(data, 0x18) = float(At<float>(data, 0x18) * kPi / 180.0);
    At<float>(data, 0x2C) = float(std::sin(double(float(At<float>(data, 0x2C) * kPi / 180.0))));
    At<float>(data, 0x30) = float(std::sin(double(float(At<float>(data, 0x30) * kPi / 180.0))));
}

void cHandlingDataMgr::LoadHandlingData() {
    char path[256];
    Sprintf(path, "Config\\%s", handlingFileName);
    LoadFile(path, fileBuffer, 0x1C000, "r");

    const char delimiters[3] = {' ', '\t', 0};
    tHandlingData* car = nullptr;
    uint8_t* record = nullptr;       // the last "!" line's
    uint8_t* bikeRecord = nullptr;   // the last "%" line's
    char* start = fileBuffer;
    char* search = fileBuffer + 1;
    char line[200];
    uint8_t* base = reinterpret_cast<uint8_t*>(this);
    for (;;) {
        char* end = search;
        while (*end != '\n') {
            end++;
        }
        int length = int(end - start);
        Strncpy(line, start, size_t(length - 1));
        start = end + 1;
        search = end + 2;
        line[length - 1] = 0;
        if (std::memcmp(line, ";the end", 9) == 0) {
            return;
        }
        if (line[0] == ';') {
            continue;
        }

        if (line[0] == '!') {
            uint8_t field = 0;
            char* token = Strtok(line, delimiters);
            do {
                if (field == 1) {
                    int32_t id = Mgr_FindExactWord(this, token, vehicleNames, 14, 120);
                    record = base + 0x48A8 + id * 0x5C;
                    At<int32_t>(record, 0) = id;
                } else if (field >= 2 && field <= 23) {
                    At<float>(record, (field - 1) * 4) = float(Atof(token));
                }
                token = Strtok(nullptr, delimiters);
                field++;
            } while (token);
            Mgr_ConvertBikeDataToGameUnits(this, record);
        } else if (line[0] == '$') {
            Strtok(line, delimiters);
            while (Strtok(nullptr, delimiters)) {
            }
        } else if (line[0] == '%') {
            // (the offsets by field: 2..8 +4..+0x1C, 9..14 +0x24..+0x38, 15 +0x20)
            static const uint8_t kOffsets[16] = {0, 0, 0x4, 0x8, 0xC, 0x10, 0x14, 0x18, 0x1C,
                                                 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x20};
            uint8_t field = 0;
            char* token = Strtok(line, delimiters);
            do {
                if (field == 1) {
                    int32_t id = Mgr_FindExactWord(this, token, vehicleNames, 14, 120);
                    if (uint8_t(uint8_t(id) - 100) <= 10) {
                        bikeRecord = base + 0x5528 + uint8_t(id) * 0x3C;
                    } else {
                        bikeRecord = base + 0x6C98;
                    }
                    At<int32_t>(bikeRecord, 0) = id;
                } else if (field >= 2 && field <= 15) {
                    At<float>(bikeRecord, kOffsets[field]) = float(Atof(token));
                }
                token = Strtok(nullptr, delimiters);
                field++;
            } while (token);
        } else {
            uint8_t field = 0;
            char* token = Strtok(line, delimiters);
            do {
                switch (field) {
                case 0: {
                    int32_t id = Mgr_FindExactWord(this, token, vehicleNames, 14, 120);
                    car = reinterpret_cast<tHandlingData*>(base + 0x14 + id * 0xDC);
                    car->index = id;
                    break;
                }
                case 1: car->mass = float(Atof(token)); break;
                case 2: car->dimensions[0] = float(Atof(token)); break;
                case 3: car->dimensions[1] = float(Atof(token)); break;
                case 4: car->dimensions[2] = float(Atof(token)); break;
                case 5: car->centreOfMass[0] = float(Atof(token)); break;
                case 6: car->centreOfMass[1] = float(Atof(token)); break;
                case 7: car->centreOfMass[2] = float(Atof(token)); break;
                case 8: car->percentSubmerged = uint8_t(Atoi(token)); break;
                case 9: car->tractionMultiplier = float(Atof(token)); break;
                case 10: car->unknownA0 = float(Atof(token)); break;
                case 11: car->unknownA4 = float(Atof(token)); break;
                case 12: car->transmission.numberOfGears = uint8_t(Atoi(token)); break;
                case 13: car->transmission.maxVelocity = float(Atof(token)); break;
                case 14: {
                    float value = float(Atof(token));
                    car->transmission.engineAcceleration = float(value * 0.4000000059604645);
                    break;
                }
                case 15: car->transmission.driveType = token[0]; break;
                case 16: car->transmission.engineType = token[0]; break;
                case 17: car->unknown90 = float(Atof(token)); break;
                case 18: car->unknown94 = float(Atof(token)); break;
                case 19: car->unknown98 = Atoi(token) != 0 ? 1 : 0; break;
                case 20: car->unknown9C = float(Atof(token)); break;
                case 21: car->unknownAC = float(Atof(token)); break;
                case 22: car->unknownB0 = float(Atof(token)); break;
                case 23: car->unknownD0 = float(Atof(token)); break;
                case 24: car->unknownC4 = float(Atof(token)); break;
                case 25: car->unknownD4 = Atoi(token); break;
                case 26: car->unknownB4[0] = float(Atof(token)); break;
                case 27: car->unknownB4[1] = float(Atof(token)); break;
                case 28: car->unknownB4[2] = float(Atof(token)); break;
                case 29: car->unknownB4[3] = float(Atof(token)); break;
                case 30:
                    Sscanf(token, "%x", &car->handlingFlags);
                    car->transmission.flags = uint8_t(car->handlingFlags);
                    break;
                case 31: car->unknownD8 = uint8_t(Atoi(token)); break;
                case 32: car->unknownD9 = uint8_t(Atoi(token)); break;
                case 33: car->unknownC8 = float(Atof(token)); break;
                default: break;
                }
                token = Strtok(nullptr, delimiters);
                field++;
            } while (token);
            Mgr_ConvertDataToGameUnits(this, car);
        }
    }
}

cHandlingDataMgr* cHandlingDataMgr::Construct() {
    for (int i = 0; i < 120; i++) {
        tHandlingData& h = handlingData[i];
        for (int j = 0; j < 3; j++) {
            h.dimensions[j] = 0.0f;
            h.centreOfMass[j] = 0.0f;
        }
        Transmission_Construct(&h.transmission);
    }
    uint8_t* base = reinterpret_cast<uint8_t*>(this);
    for (int i = 0; i < 11; i++) {
        float* vectors = reinterpret_cast<float*>(base + 0x6CBC + i * 0x3C);
        for (int j = 0; j < 6; j++) {
            vectors[j] = 0.0f;
        }
    }
    Memset(this, 0, 0x6F30);
    return this;
}

void cHandlingDataMgr::Initialise() {
    Mgr_LoadHandlingData(this);
    unknown0[0] = 0.1f;
    unknown0[1] = 0.9f;
    unknown0[2] = 1.0f;
    unknown0[3] = 0.8f;
    unknown0[4] = 0.98f;
}

void tTransmissionData::InitGearRatios() {
    Memset(this, 0, 0x48);
    float step = float(1.0 / int(numberOfGears));
    if (numberOfGears >= 1) {
        for (uint8_t g = 1; g <= numberOfGears; g++) {
            tGear* gear = &gears[g];
            tGear* prev = gear - 1;
            lastGear = gear;
            lastPrevGear = prev;
            float speed = float(double(g) * step * maxVelocity);
            gear->maxVelocity = speed;
            float range = float(speed - prev->maxVelocity);
            float up;
            if (g < numberOfGears) {
                gears[g + 1].changeDownVelocity = float(range * 0.41999998688697815 + prev->maxVelocity);
                up = float(range * 0.949999988079071 + prev->maxVelocity);
            } else {
                up = maxVelocity;
            }
            gear->changeUpVelocity = up;
        }
    }
    gears[0].maxVelocity = maxReverseVelocity;
    gears[0].changeUpVelocity = -0.01f;
    gears[0].changeDownVelocity = maxReverseVelocity;
    gears[1].changeDownVelocity = -0.01f;
}

float tTransmissionData::CalculateDriveAcceleration(float* gasPedal, uint8_t* gear, float* velocity, CVehicle* vehicle,
                                                    float arg5, float speedFactor, float driverSpeedLimit) {
    if (*velocity < maxReverseVelocity && *gasPedal < 0.0f) {
        return 0.0f;
    }
    bool cheat = HandlingCheat();
    if (0.0f < *gasPedal) {
        float limit = maxVelocity * speedFactor;
        if ((limit < *velocity || (driverSpeedLimit < *velocity && DriverIsKind13(vehicle))) && !cheat) {
            return 0.0f;
        }
    }

    float speed = *velocity;
    currentVelocity = speed;
    // the gear: up past its change-up speed (not out of neutral without gas), else down
    // below its change-down speed (not out of first unless reversing); then again
    uint8_t g = *gear;
    int change = 0;
    bool goingUp = false;
    if (g < numberOfGears) {
        float up = int8_t(g) < 0 ? -gears[-int8_t(g)].changeDownVelocity : gears[g].changeUpVelocity;
        if (up < speed) {
            goingUp = true;
            if (g != 0 || 0.0f < *gasPedal) {
                change = 1;
            }
        }
    }
    if (!goingUp) {
        float down = int8_t(g) < 0 ? -gears[-int8_t(g)].changeUpVelocity : gears[g].changeDownVelocity;
        if (speed < down && g != 0 && (g != 1 || *gasPedal < 0.0f)) {
            change = -1;
        }
    }
    if (change) {
        *gear = uint8_t(g + change);
        return Transmission_CalculateDriveAcceleration(this, gasPedal, gear, &speed, vehicle, arg5, speedFactor,
                                                       driverSpeedLimit);
    }

    float factor;
    if (numberOfGears == 1) {
        factor = 1.0f;
    } else if (g < 1) {
        factor = 2.0f;
    } else {
        int q = (int(g) - 1) / (int(numberOfGears) - 1);
        float r = float(1.0 - q);
        double x;
        if (flags & 1) {
            x = 3.0 * r;
        } else if (flags & 2) {
            x = 2.5 * r;
        } else {
            x = double(r) + r;
        }
        factor = float(x * r + 1.0);
    }
    return DriveForce(this, gasPedal, speed, vehicle, arg5, speedFactor, factor, cheat);
}

float tTransmissionData::CalculateDriveAccelerationSimple(float* gasPedal, float* velocity, CVehicle* vehicle, float arg4,
                                                          float speedFactor, float driverSpeedLimit) {
    if (*velocity < maxReverseVelocity && *gasPedal < 0.0f) {
        return 0.0f;
    }
    bool cheat = HandlingCheat();
    if (0.0f < *gasPedal) {
        float limit = maxVelocity * speedFactor;
        if ((limit < *velocity || (driverSpeedLimit < *velocity && DriverIsKind13(vehicle))) && !cheat) {
            return 0.0f;
        }
    }
    currentVelocity = *velocity;
    return DriveForce(this, gasPedal, *velocity, vehicle, arg4, speedFactor, 1.0f, cheat);
}

} // namespace c11handling

// ---- registration ---------------------------------------------------------------------

#define HANDLING_REVERSED(name, address, member)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, re::MemberAddress(member), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

HANDLING_REVERSED("cHandlingDataMgr::FindExactWord", 0x004C9B70, &c11handling::cHandlingDataMgr::FindExactWord);
HANDLING_REVERSED("cHandlingDataMgr::GetHandlingId", 0x004C9BE0, &c11handling::cHandlingDataMgr::GetHandlingId);
HANDLING_REVERSED("cHandlingDataMgr::ConvertDataToGameUnits", 0x004C9C20,
                  &c11handling::cHandlingDataMgr::ConvertDataToGameUnits);
HANDLING_REVERSED("cHandlingDataMgr::ConvertBikeDataToGameUnits", 0x004C9DF0,
                  &c11handling::cHandlingDataMgr::ConvertBikeDataToGameUnits);
HANDLING_REVERSED("cHandlingDataMgr::LoadHandlingData", 0x004C9E80, &c11handling::cHandlingDataMgr::LoadHandlingData);
HANDLING_REVERSED("cHandlingDataMgr::Construct", 0x004CA680, &c11handling::cHandlingDataMgr::Construct);
HANDLING_REVERSED("cHandlingDataMgr::Initialise", 0x004CA700, &c11handling::cHandlingDataMgr::Initialise);
HANDLING_REVERSED("tTransmissionData::InitGearRatios", 0x004CA740, &c11handling::tTransmissionData::InitGearRatios);
HANDLING_REVERSED("tTransmissionData::CalculateDriveAcceleration", 0x004CA800,
                  &c11handling::tTransmissionData::CalculateDriveAcceleration);
HANDLING_REVERSED("tTransmissionData::CalculateDriveAccelerationSimple", 0x004CAC20,
                  &c11handling::tTransmissionData::CalculateDriveAccelerationSimple);
