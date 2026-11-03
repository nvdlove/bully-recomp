// CPed (0x47C820 .. 0x47DB3E): peds bumping into each other, the stat meters (+0x18C0,
// +0x18D0), entering a bike, the nearby-peds list (+0x3C0), its reset, and applying its
// move speed and root motion each frame (vf37).

#include <cmath>

#include "game/Ped.h"
#include "game/Pool.h"
#include "reversed.h"

namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;
ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

constexpr int PED_TYPE_PLAYER = 0xD;

GAME_VAR(CPed*, PU_player, 0x00C1AEA8);
GAME_VAR(uint32_t, PU_timeInMs, 0x00C1A9B4);
GAME_VAR(float, PU_timeStep, 0x00C1A9A4);
GAME_VAR(float, PU_stepSeconds, 0x00A3396C);          // 0.02
GAME_ARRAY(uint8_t*, PU_modelInfoPtrs, 0x00C67738);
GAME_VAR(CPool<CPed>*, PU_pedPool, 0x00C0F5F0);
GAME_VAR(void*, PU_C2C1A0, 0x00C2C1A0);
GAME_VAR(float, PU_f_minusOne, 0x009009AC);
GAME_VAR(float, PU_f_2p32, 0x00900AC8);
// bumping
GAME_VAR(float, PU_bumpSpeed, 0x00900484);            // 0.2
GAME_VAR(float, PU_shoveSpeed, 0x0090CFD0);           // 0.28
GAME_VAR(float, PU_bumpSpeedSlide, 0x0090CFCC);       // 0.06
GAME_VAR(float, PU_shoveSpeedSlide, 0x0090CFC8);      // 0.09
GAME_VAR(float, PU_f_0_1, 0x00900170);
GAME_VAR(float, PU_f_0, 0x008FF1F0);
GAME_VAR(float, PU_f_0_4, 0x0090048C);
GAME_VAR(int32_t, PU_modelRat, 0x00BF42DC);
GAME_VAR(int32_t, PU_model42D8, 0x00BF42D8);
inline void* const PU_CF4378 = reinterpret_cast<void*>(0x00CF4378);
inline void* const PU_soundMgr = reinterpret_cast<void*>(0x00CF3DA0);
// the meters
GAME_VAR(double, PU_d_1000, 0x00900138);
GAME_VAR(double, PU_d_3000, 0x0090CFF8);
GAME_VAR(double, PU_d_200, 0x0090D000);
GAME_VAR(float, PU_f_199, 0x0090CFF0);
GAME_VAR(uint8_t*, PU_aiButes, 0x020C889C);           // +0x80 the meter's rate
GAME_VAR(uint8_t*, PU_tuning, 0x020C88B8);
GAME_VAR(uint8_t, PU_difficulty, 0x00C1AF4C);
GAME_VAR(float, PU_f_200, 0x00906948);
// nearby peds
GAME_VAR(double, PU_d_900, 0x00900B00);
GAME_VAR(uint16_t, PU_nearbyCount, 0x00C2B038);
GAME_ARRAY(CPed*, PU_nearby, 0x00C2B040);             // (51)
// motion
GAME_VAR(int32_t, PU_weapon4330, 0x00BF4330);
GAME_VAR(double, PU_d_100, 0x00900130);
GAME_VAR(double, PU_d_0_5, 0x008FF1F8);
GAME_VAR(float, PU_waterDeep, 0x0090D068);            // -1.45
GAME_VAR(double, PU_d_1_44, 0x0090D060);
GAME_VAR(float, PU_waterShallow, 0x0090D05C);         // -1.25

GAME_FN(uint8_t, __thiscall, PU_Fn485590, 0x00485590, void* self, CPed* other);
GAME_FN(uint8_t, __cdecl, PU_Fn683A30, 0x00683A30);
GAME_FN(ped::CVector*, __cdecl, PU_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PU_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __thiscall, PU_GetForward, 0x004154A0, void* entity, ped::CVector* out);
GAME_FN(uint8_t, __thiscall, PU_IsNodePlaying, 0x005F4800, void* controller, void* node, int a, int b);
GAME_FN(float, __thiscall, PU_VectorMagnitude2D, 0x004246B0, const ped::CVector* v);
GAME_FN(float, __cdecl, PU_Fn41A950, 0x0041A950, const ped::CVector* a, const ped::CVector* b, float c);
GAME_FN(ped::CVector*, __cdecl, PU_VectorScale, 0x00412350, ped::CVector* out, const ped::CVector* in, float s);
GAME_FN(void, __thiscall, PU_Fn495220, 0x00495220, void* self, CPed* other);
GAME_FN(uint8_t, __thiscall, PU_Fn48FB40, 0x0048FB40, void* self);
GAME_FN(uint8_t, __thiscall, PU_WantsGift, 0x00474A30, CPed* ped);
GAME_FN(uint8_t, __thiscall, PU_IsDog, 0x004473E0, CPed* ped);
GAME_FN(uint8_t, __thiscall, PU_Fn485530, 0x00485530, void* self);
GAME_FN(int, __thiscall, PU_Fn4A6150, 0x004A6150, void* self, CPed* ped, int kind, int a);
GAME_FN(void, __thiscall, PU_Fn4A6670, 0x004A6670, void* self, int kind, CPed* a, CPed* b, uintptr_t c, float d, float e);
GAME_FN(void, __cdecl, PU_PedBump, 0x0047C820, CPed* ped, CPed* other, uint8_t* shoved, uint8_t* shove,
        uint8_t* voice);
GAME_FN(void, __thiscall, PU_Fn5A4920, 0x005A4920, void* self, void* out, CPed* a, CPed* b, int c, int d, int e,
        int f, int g, int h, int i, uintptr_t j);
GAME_FN(int, __thiscall, PU_SoundGetId, 0x00595B80, void* mgr, const char* name);
GAME_FN(void, __thiscall, PU_SoundPlay, 0x0059B8A0, void* mgr, int id, const ped::CVector* pos, int a, uintptr_t b, int c,
        int d, int e, int f, uintptr_t g, uintptr_t h, int i, int j, float volume, int k);
GAME_FN(void, __thiscall, PU_Add18C8, 0x004743D0, CPed* ped, int value);
GAME_FN(uint8_t, __cdecl, PU_Fn425330, 0x00425330);
GAME_FN(void, __thiscall, PU_VehicleFn4BDF80, 0x004BDF80, void* vehicle, CPed* ped);
GAME_FN(void, __thiscall, PU_EnterVehicle, 0x00476640, CPed* ped, void* vehicle);
GAME_FN(void, __thiscall, PU_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(int*, __thiscall, PU_VehicleFn4CAF30, 0x004CAF30, void* vehicle, int* out);
GAME_FN(void, __thiscall, PU_IntelLoadActions, 0x00484AF0, void* intelligence, const char* tree, const char* file);
GAME_FN(intptr_t, __thiscall, PU_RaisePendingEvent, 0x004779E0, CPed* ped);
GAME_FN(void, __cdecl, PU_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, PU_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, PU_PedReset, 0x004750A0, CPed* ped);
GAME_FN(void, __thiscall, PU_SetPedStats, 0x00478F80, CPed* ped, void* stats, int keepWeapons);
GAME_FN(void, __thiscall, PU_Intel484E60, 0x00484E60, void* intelligence, CPed* ped);
GAME_FN(void, __thiscall, PU_Intel4840F0, 0x004840F0, void* intelligence);
GAME_FN(uint8_t, __thiscall, PU_Entity465A60, 0x00465A60, CPed* ped, int flag);
GAME_FN(void, __thiscall, PU_ListRemoveAt, 0x0047D220, void* list, int index);
GAME_FN(void, __thiscall, PU_ListSwap, 0x0047D280, void* list, int a, int b);
GAME_FN(void, __thiscall, PU_SortByDistance, 0x00475530, CPed* ped, re::Ptr32<CPed>* entities, int from, int to);
GAME_FN(void, __thiscall, PU_ListClear, 0x00478590, void* list);
GAME_FN(void, __thiscall, PU_UpdateVelocity, 0x004733B0, CPed* ped);
GAME_FN(void, __thiscall, PU_MatrixSetRotateZ, 0x00412C20, void* matrix, float heading);
GAME_FN(void, __thiscall, PU_PhysicalFn4698B0, 0x004698B0, CPed* ped);
GAME_FN(void, __thiscall, PU_ApplyRootMotion, 0x00468450, CPed* ped, void* rotation, void* translation);
GAME_FN(void, __thiscall, PU_QuaternionSetIdentity, 0x00472E20, void* q);
GAME_FN(float, __cdecl, PU_AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(int, __thiscall, PU_GetStat, 0x00477060, CPed* ped, int stat);
GAME_FN(void, __thiscall, PU_UpdateSlide, 0x0047A280, CPed* ped, float dt);

using ApplyHitFn = uint8_t(__thiscall*)(CPed*, float, float, CPed*, int, ped::CVector*, ped::CVector*, void*,
                                        const char*, int, int, float, float, int, int, int, int, float, int);

// 0x47C820 (cdecl): the player bumping into `other` from behind, fast enough: a slight
// bump, or a shove (then both flagged); `voice` the other's model's +0x50
void __cdecl CPed_Bump(CPed* ped, CPed* other, uint8_t* shoved, uint8_t* shove, uint8_t* voice) {
    float bump = PU_bumpSpeed;
    float hard = PU_shoveSpeed;
    if (PU_Fn485590(Bytes(ped) + 0x2F0, other) || PU_Fn683A30()) {
        bump = PU_f_0_1;
        hard = PU_f_0_1;
    } else if (At<int32_t>(ped, 0x1474) != 0) {
        bump = PU_bumpSpeedSlide;
        hard = PU_shoveSpeedSlide;
    }
    ped::CVector a = *PosOf(ped);
    ped::CVector b = *PosOf(other);
    if (ped->pedType != PED_TYPE_PLAYER) {
        return;
    }
    ped::CVector d;
    ped::CVector* v = PU_VectorSub(&d, &b, &a);
    ped::CVector tmp;
    ped::CVector* f = PU_GetForward(ped, &tmp);
    float ahead = f->y * v->y + f->x * v->x + f->z * v->z;
    if (!(ahead > PU_f_0)) {
        return;
    }
    if (PU_IsNodePlaying(other->intelligence + 0x258, reinterpret_cast<void*>(0x01B65270), 1, 1) ||
        PU_IsNodePlaying(other->intelligence + 0x258, reinterpret_cast<void*>(0x01B652AC), 1, 1)) {
        return;
    }
    ped::CVector* moving = At<int32_t>(ped, 0x1474) != 0 ? &At<ped::CVector>(ped, 0x124) : &At<ped::CVector>(ped, 0x1720);
    float speed = PU_VectorMagnitude2D(moving);
    if (!(bump <= speed)) {
        return;
    }
    *voice = PU_modelInfoPtrs[other->modelIndex][0x50];
    float r = PU_Fn41A950(&b, &a, PU_f_0_4);
    ped::CVector s1;
    ped::CVector s2;
    ped::CVector s3;
    ped::CVector impulse;
    PU_VectorScale(&s2, PU_VectorScale(&s1, PU_GetForward(ped, &tmp), r), PU_f_0_4);
    PU_VectorSub(&impulse, &b, PU_VectorAdd(&s3, &a, &s2));
    ApplyHitFn hit = re::PtrCast<ApplyHitFn>(other->vtable[0x3C / 4]);
    if (hard <= speed) {
        hit(other, 0.0f, 0.0f, nullptr, -1, nullptr, &impulse, nullptr, reinterpret_cast<const char*>(0x0090CFBC), 0,
            0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);  // "BUMPSHOVE"
        PU_Fn495220(Bytes(ped) + 0xB44, other);
        *shoved = 1;
        *shove = 1;
        return;
    }
    hit(other, 0.0f, 0.0f, nullptr, -1, nullptr, &impulse, nullptr, reinterpret_cast<const char*>(0x0090CFB0), 0, 0,
        0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);  // "SLIGHTBUMP"
}

// 0x47CAD0 (thiscall): two peds touching: unless both mean to (+0x14D0, holding hands,
// ...), an event, each bumping the other, and a hit voice
void __fastcall CPed_Touch(CPed* self, re::Edx, CPed* other) {
    if (!other) {
        return;
    }
    uint8_t both14D0 = At<int32_t>(other, 0x14D0) != 0 && At<int32_t>(self, 0x14D0) != 0;
    uint8_t b16 = PU_Fn48FB40(Bytes(other) + 0xB44);
    uint8_t gift = PU_WantsGift(other);
    uint8_t b13 = At<int32_t>(other, 0x1348) != 0 || At<int32_t>(self, 0x1348) != 0;
    uint8_t animal = other->modelIndex == PU_modelRat || self->modelIndex == PU_modelRat || PU_IsDog(other) ||
                     PU_IsDog(self);
    auto kind3 = [](CPed* p) {
        return (p->unknown1D24 && At<int32_t>(p->unknown1D24, 0x70) == 3) || At<int32_t>(p, 0x14CC) != 0;
    };
    uint8_t scripted = kind3(other) || kind3(self);
    uint8_t model42D8 = other->modelIndex == PU_model42D8 || self->modelIndex == PU_model42D8;
    bool otherSeated = other->pedType != PED_TYPE_PLAYER && other->seatState == 2;
    bool selfSeated = self->pedType != PED_TYPE_PLAYER && self->seatState == 2;
    bool holding = At<int32_t>(other, 0x153C) != 0 && At<int32_t>(self, 0x153C) != 0;
    bool react;
    if (holding) {
        react = PU_Fn683A30() != 0;
    } else if (both14D0 && !b16 && !gift && !b13 && !animal && !scripted && !model42D8 && !otherSeated &&
               !selfSeated) {
        react = true;
    } else {
        react = PU_Fn485530(Bytes(other) + 0x2F0) || PU_Fn683A30();
    }
    if (!react) {
        return;
    }
    uint8_t shove = 0;
    if (PU_C2C1A0 && PU_Fn4A6150(PU_C2C1A0, self, 0x12, -1) == 0) {
        PU_Fn4A6670(PU_C2C1A0, 0x12, self, other, 0, PU_f_minusOne, PU_f_minusOne);
    }
    uint8_t shoved = 0;
    uint8_t voice = 0;
    PU_PedBump(self, other, &shoved, &shove, &voice);
    PU_PedBump(other, self, &shoved, &shove, &voice);
    if (shove) {
        // (the out argument, a whole SpeechEvent (0x30: Speech::Add fills it all), is
        // uninitialised stack in the game; 0x10 here once ran over the stack cookie)
        alignas(4) uint8_t out[0x30] = {};
        if (other->pedType == PED_TYPE_PLAYER) {
            PU_Fn5A4920(PU_CF4378, out, self, other, 0, 0x12, 0, 0, 0, 0, -1, 0);
        } else {
            PU_Fn5A4920(PU_CF4378, out, other, self, 0, 0x12, 0, 0, 0, 0, -1, 0);
        }
    }
    if (shoved) {
        const char* name = voice ? reinterpret_cast<const char*>(0x0090CFE0)   // "FemaleHitVox"
                                 : reinterpret_cast<const char*>(0x0090CFD4);  // "MaleHitVox"
        int id = PU_SoundGetId(PU_soundMgr, name);
        const ped::CVector* pos = other->pedType == PED_TYPE_PLAYER ? PosOf(self) : PosOf(other);
        PU_SoundPlay(PU_soundMgr, id, pos, 2, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
    }
}

double UnsignedToDouble(uint32_t v) {
    double d = static_cast<double>(static_cast<int32_t>(v));
    if (static_cast<int32_t>(v) < 0) {
        d = d + PU_f_2p32;
    }
    return d;
}

// 0x47CD90 (thiscall): the +0x18C0 meter filled; each 200 moves one into +0x18C8 (at
// once, or `delayed` 3 seconds after it first could); capped at 199 once +0x18C8 is 5
void __fastcall CPed_AddMeter18C0(CPed* self, re::Edx, float amount, int delayed) {
    uint32_t now = PU_timeInMs;
    if (!re::Flag(delayed)) {
        At<uint32_t>(self, 0x18FC) = 0;
        At<int32_t>(self, 0x18F8) = static_cast<int32_t>(static_cast<int64_t>(UnsignedToDouble(now) + PU_d_1000));
    }
    int count = self->unknown18C8;
    float v = amount + self->unknown18C0;
    self->unknown18C0 = v;
    int whole = static_cast<int>(v / PU_d_200);
    if (0.0f > v) {
        whole--;
    }
    if (whole != 0) {
        uint32_t due = At<uint32_t>(self, 0x18FC);
        if (!re::Flag(delayed) || (now > due && due != 0)) {
            PU_Add18C8(self, whole);
            count += whole;
            At<uint32_t>(self, 0x18FC) = 0;
            self->unknown18C0 = self->unknown18C0 - static_cast<float>(whole * 200);
        } else if (due == 0) {
            At<uint32_t>(self, 0x18FC) =
                static_cast<uint32_t>(static_cast<int64_t>(UnsignedToDouble(now) + PU_d_3000));
        }
    }
    if (static_cast<float>(count) < 0.0f || 0.0f > self->unknown18C0) {
        self->unknown18C0 = 0.0f;
        return;
    }
    if (count < 5) {
        return;
    }
    self->unknown18C0 = PU_f_199;
}

// 0x47CF10 (thiscall): its ped stats' +0x74
float __fastcall CPed_GetStats74(CPed* self, re::Edx) {
    return static_cast<float>(At<int32_t>(At<uint8_t*>(PU_modelInfoPtrs[self->modelIndex], 0x54), 0x74));
}

// 0x47CF30 (thiscall): the +0x18D0 meter refills (by difficulty for the player), up to
// its ped stats' +0x5C; full with 0x425330
void __fastcall CPed_RefillMeter18D0(CPed* self, re::Edx) {
    float rate = At<float>(PU_aiButes, 0x80);
    float max = static_cast<float>(At<int32_t>(At<uint8_t*>(PU_modelInfoPtrs[self->modelIndex], 0x54), 0x5C));
    if (self->pedType == PED_TYPE_PLAYER) {
        if (PU_difficulty == 1) {
            rate = At<float>(PU_tuning, 0xB8);
            max = At<float>(PU_tuning, 0xC0);
        } else if (PU_difficulty == 2) {
            rate = At<float>(PU_tuning, 0xBC);
            max = At<float>(PU_tuning, 0xC4);
        }
    }
    float step = PU_stepSeconds * PU_timeStep;
    float v = step * rate + self->unknown18D0;
    self->unknown18D0 = v;
    float result;
    if (v < 0.0f) {
        result = 0.0f;
    } else if (max < v) {
        result = max;
    } else {
        result = v;
    }
    self->unknown18D0 = result;
    if (PU_Fn425330() && self == PU_player) {
        self->unknown18D0 = PU_f_200;
    }
}

// 0x47D030 (thiscall): +0x18D0 changed, within 0 .. its ped stats' +0x5C (not taken
// down with +0x13C8 or when the player is +0x2F4)
void __fastcall CPed_AddMeter18D0(CPed* self, re::Edx, float delta) {
    if (delta <= 0.0f &&
        (At<int32_t>(self, 0x13C8) != 0 || (self->unknown2F4 && self->unknown2F4->pedType == PED_TYPE_PLAYER))) {
        return;
    }
    float max = static_cast<float>(At<int32_t>(At<uint8_t*>(PU_modelInfoPtrs[self->modelIndex], 0x54), 0x5C));
    float v = delta + self->unknown18D0;
    float result;
    if (v < 0.0f) {
        result = 0.0f;
    } else if (max < v) {
        result = max;
    } else {
        result = v;
    }
    self->unknown18D0 = result;
}

// 0x47D0D0 (thiscall): onto a bike (its action tree by kind: motorcycle, scooter, bike)
void __fastcall CPed_MountBike(CPed* self, re::Edx, uint8_t* bike) {
    if (!bike) {
        return;
    }
    uint8_t* target = self->intelligence + 0x408;
    target[0x68] = 0;
    At<int32_t>(target, 0x64) = 0;
    target[0x34] = 0;
    PU_VehicleFn4BDF80(bike, self);
    PU_EnterVehicle(self, bike);
    const ped::CVector* pos = PosOf(bike);
    PU_SetPosition(self, pos->x, pos->y, pos->z);
    int kind;
    const char* file = reinterpret_cast<const char*>(0x00901018);  // "Act/Vehicles.act"
    if (*PU_VehicleFn4CAF30(bike, &kind) == 3) {
        PU_IntelLoadActions(self->intelligence, reinterpret_cast<const char*>(0x0090D030), file);  // Motorcycle
        return;
    }
    if (*PU_VehicleFn4CAF30(bike, &kind) == 8) {
        PU_IntelLoadActions(self->intelligence, reinterpret_cast<const char*>(0x0090D008), file);  // Scooter
        return;
    }
    PU_IntelLoadActions(self->intelligence, reinterpret_cast<const char*>(0x00900FF0), file);  // Bikes
}

// 0x47D200 (thiscall): an event to raise (see 0x4779E0)
intptr_t __fastcall CPed_SetPendingEvent(CPed* self, re::Edx, int event) {
    At<int32_t>(self, 0x1E30) = event;
    return PU_RaisePendingEvent(self);
}

struct EntityList {
    int32_t count;
    re::Ptr32<CEntity> entries[1];
};

void SetRef(re::Ptr32<CEntity>* ref, CEntity* entity) {
    if (entity != *ref) {
        if (*ref) {
            PU_CleanUpOldReference(*ref, ref);
        }
        *ref = entity;
        if (entity) {
            PU_RegisterReference(entity, ref);
        }
    }
}

// 0x47D220 (thiscall): one removed, the rest moved down (the last slot keeps its
// reference, as the game does)
void __fastcall EntityList_RemoveAt(EntityList* self, re::Edx, int index) {
    self->count--;
    for (int i = index; i < self->count; i++) {
        SetRef(&self->entries[i], self->entries[i + 1]);
    }
}

// 0x47D280 (thiscall, TrackOrCondition::sub_47D280): two swapped
void __fastcall EntityList_Swap(EntityList* self, re::Edx, int a, int b) {
    CEntity* first = self->entries[a];
    CEntity* second = self->entries[b];
    SetRef(&self->entries[a], second);
    SetRef(&self->entries[b], first);
}

// 0x47D2F0 (thiscall): reset with its model's stats, its intelligence too
void __fastcall CPed_ResetAll(CPed* self, re::Edx) {
    PU_PedReset(self);
    PU_SetPedStats(self, nullptr, 0);
    PU_Intel484E60(self->intelligence, self);
    PU_Intel4840F0(self->intelligence);
}

float FlatDistanceSqMinus900(const ped::CVector* from, CPed* other) {
    ped::CVector d;
    PU_VectorSub(&d, from, PosOf(other));
    d.z = 0.0f;
    float sq = d.y * d.y + d.x * d.x + 0.0f * 0.0f;
    return static_cast<float>(sq - PU_d_900);
}

float DistanceSq(const ped::CVector* from, CPed* other) {
    ped::CVector d;
    ped::CVector* v = PU_VectorSub(&d, from, PosOf(other));
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

// 0x47D320 (thiscall): the peds near it (+0x3C0, up to 16 within 30 m, nearest first):
// found again from the pool with `rebuild` (or 0x465A60(0x10)), else the far ones
// dropped and one bubble pass made
void __fastcall CPed_UpdateNearbyPeds(CPed* self, re::Edx, int rebuild) {
    ped::CVector pos = *PosOf(self);
    EntityList* list = reinterpret_cast<EntityList*>(Bytes(self) + 0x3C0);
    if (!re::Flag(rebuild) && !PU_Entity465A60(self, 0x10)) {
        for (int i = 0; i < list->count; i++) {
            CPed* p = re::PtrCast<CPed*>(list->entries[i]);
            if (!p || 0.0f < FlatDistanceSqMinus900(&pos, p)) {
                PU_ListRemoveAt(list, i);
                i--;
            }
        }
        for (int i = 0; i < list->count - 1; i++) {
            CPed* a = re::PtrCast<CPed*>(list->entries[i]);
            CPed* b = re::PtrCast<CPed*>(list->entries[i + 1]);
            if (a && b) {
                float da = DistanceSq(&pos, a);
                float db = DistanceSq(&pos, b);
                if (db < da) {
                    PU_ListSwap(list, i, i + 1);
                }
            }
        }
        return;
    }
    CPool<CPed>* pool = PU_pedPool;
    PU_nearbyCount = 0;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        CPed* p = reinterpret_cast<CPed*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
        if (!p || p == self) {
            continue;
        }
        if (!(0.0f < FlatDistanceSqMinus900(&pos, p)) && p->unknown1C98 == 0 && PU_nearbyCount < 0x32) {
            PU_nearby[PU_nearbyCount] = p;
            PU_nearbyCount = static_cast<uint16_t>(PU_nearbyCount + 1);
        }
    }
    int count = PU_nearbyCount;
    PU_nearby[count] = nullptr;
    PU_SortByDistance(self, PU_nearby, 0, count - 1);
    if (PU_nearbyCount > 0x10) {
        PU_nearbyCount = 0x10;
    }
    PU_ListClear(list);
    for (int i = 0; i < PU_nearbyCount; i++) {
        int n = list->count;
        list->count = n + 1;
        SetRef(&list->entries[n], re::PtrCast<CEntity*>(PU_nearby[i]));
    }
}

// vf37 0x47D660: its movement applied: the heading, the anims' root motion (+0x1720,
// +0x172C) or the move speed a step, the slide, and bobbing in water (+0x16CC)
void __fastcall CPed_ApplyMovement(CPed* self, re::Edx) {
    PU_UpdateVelocity(self);
    if (At<int32_t>(self, 0x1324) == 0 && self->unknown1C98 == 0) {
        float heading = self->desiredHeading;
        if (self->matrix) {
            PU_MatrixSetRotateZ(self->matrix, heading);
        }
        self->heading = heading;
    }
    ped::CVector& rootMove = At<ped::CVector>(self, 0x1720);
    float* rootTurn = &At<float>(self, 0x172C);
    if (At<int32_t>(self, 0x1474) != 0) {
        PU_PhysicalFn4698B0(self);
        if (At<int32_t>(self, 0x1484) == 0) {
            return;
        }
        if (rootTurn[0] == 0.0f && rootTurn[1] == 0.0f && rootTurn[2] == 0.0f) {
            return;
        }
        rootMove = {0.0f, 0.0f, 0.0f};
        PU_ApplyRootMotion(self, rootTurn, &rootMove);
        PU_QuaternionSetIdentity(rootTurn);
        ped::CVector forward;
        PU_GetForward(self, &forward);
        self->desiredHeading = PU_AngleBetweenPoints(forward.x, forward.y, 0.0f, 0.0f);
        return;
    }
    reinterpret_cast<void(__thiscall*)(CPed*)>(static_cast<void*>(self->vtable[0x90 / 4]))(self);
    if (self->vehicle == nullptr && self->unknown1470 == 0 && At<int16_t>(self->weapon, 0x10E) != PU_weapon4330) {
        int stat = PU_GetStat(self, 0x14);
        float step = PU_stepSeconds * PU_timeStep;
        float scale = static_cast<float>(stat / PU_d_100);
        PU_UpdateSlide(self, step * scale);
    }
    if (At<uint8_t>(self, 0x1BD) & 1) {
        At<ped::CVector>(self, 0x124) = {0.0f, 0.0f, 0.0f};
        return;
    }
    float ts = PU_timeStep;
    if (rootTurn[0] == 0.0f && rootTurn[1] == 0.0f && rootTurn[2] == 0.0f) {
        if (self->inVehicleSeat == 0) {
            float heading = self->desiredHeading;
            if (self->matrix) {
                PU_MatrixSetRotateZ(self->matrix, heading);
            }
            self->heading = heading;
            if (At<int32_t>(self, 0x1334) != 0) {
                At<float>(self, 0x124) = At<float>(self, 0x1708);
                At<float>(self, 0x128) = At<float>(self, 0x170C);
            }
            PosOf(self)->x = At<float>(self, 0x124) * ts + PosOf(self)->x;
            PosOf(self)->y = At<float>(self, 0x128) * ts + PosOf(self)->y;
        }
        if (!(At<uint8_t>(self, 0x1BC) & 2) && At<int32_t>(self, 0x1334) != 0) {
            float vz = At<float>(self, 0x1710);
            At<float>(self, 0x12C) = vz;
            PosOf(self)->z = vz * ts + PosOf(self)->z;
        }
    } else {
        ped::CVector old = At<ped::CVector>(self, 0x1564);
        PU_ApplyRootMotion(self, rootTurn, &rootMove);
        rootMove = {0.0f, 0.0f, 0.0f};
        PU_QuaternionSetIdentity(rootTurn);
        ped::CVector pos = *PosOf(self);
        ped::CVector d;
        ped::CVector* moved = PU_VectorSub(&d, &pos, &old);
        At<ped::CVector>(self, 0x1708) = *moved;
        At<float>(self, 0x124) = At<float>(self, 0x1708);
        At<float>(self, 0x128) = At<float>(self, 0x170C);
        ped::CVector forward;
        PU_GetForward(self, &forward);
        self->desiredHeading = PU_AngleBetweenPoints(forward.x, forward.y, 0.0f, 0.0f);
        ts = PU_timeStep;
    }
    if (At<uint8_t>(self, 0x1BC) & 2) {
        PosOf(self)->z = static_cast<float>((At<float>(self, 0x1704) + At<float>(self, 0x12C)) * PU_d_0_5 * ts +
                                            PosOf(self)->z);
        return;
    }
    if (At<uint8_t>(self, 0x16CC) == 0) {
        return;
    }
    float water = At<float>(self, 0x16D0);
    float depth = PosOf(self)->z - water;
    if (depth < PU_waterDeep) {
        PosOf(self)->z = static_cast<float>(water - PU_d_1_44);
        return;
    }
    if (!(depth < PU_waterShallow)) {
        return;
    }
    float sink = At<float>(self, 0x1D58) * PU_timeStep;
    At<float>(self, 0x1704) = At<float>(self, 0x12C);
    float vz = At<float>(self, 0x12C) + sink;
    At<float>(self, 0x12C) = vz;
    PosOf(self)->z = static_cast<float>((vz + At<float>(self, 0x1704)) * PU_d_0_5 * ts + PosOf(self)->z);
}

#define PEDBUMP_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDBUMP_REVERSED("CPed::Bump", 0x0047C820, &CPed_Bump);
PEDBUMP_REVERSED("CPed::Touch", 0x0047CAD0, &CPed_Touch);
PEDBUMP_REVERSED("CPed::AddMeter18C0", 0x0047CD90, &CPed_AddMeter18C0);
PEDBUMP_REVERSED("CPed::GetStats74", 0x0047CF10, &CPed_GetStats74);
PEDBUMP_REVERSED("CPed::RefillMeter18D0", 0x0047CF30, &CPed_RefillMeter18D0);
PEDBUMP_REVERSED("CPed::AddMeter18D0", 0x0047D030, &CPed_AddMeter18D0);
PEDBUMP_REVERSED("CPed::MountBike", 0x0047D0D0, &CPed_MountBike);
PEDBUMP_REVERSED("CPed::SetPendingEvent", 0x0047D200, &CPed_SetPendingEvent);
PEDBUMP_REVERSED("EntityList::RemoveAt", 0x0047D220, &EntityList_RemoveAt);
PEDBUMP_REVERSED("EntityList::Swap", 0x0047D280, &EntityList_Swap);
PEDBUMP_REVERSED("CPed::ResetAll", 0x0047D2F0, &CPed_ResetAll);
PEDBUMP_REVERSED("CPed::UpdateNearbyPeds", 0x0047D320, &CPed_UpdateNearbyPeds);
PEDBUMP_REVERSED("CPed::ApplyMovement", 0x0047D660, &CPed_ApplyMovement);

} // namespace
