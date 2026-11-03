// CPed (0x4750A0 .. 0x4763C0): its reset, the fade in and out of ambient peds, sorting
// entities by distance, water, the weapon sound banks, two peds holding hands, foot
// placement on a plane, and two checks on entities.

#include <cmath>

#include "game/Ped.h"

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

GAME_VAR(uint32_t, PS_timeInMs, 0x00C1A9B4);          // CTimer::m_snTimeInMilliseconds
GAME_VAR(int32_t, PS_currArea, 0x00BD1008);           // CGame::currArea
GAME_VAR(int32_t, PS_C3CCF8, 0x00C3CCF8);             // (13: the player keeps fading in)
GAME_VAR(uint8_t*, PS_population, 0x00C2C108);        // +0x7898 bit 0
GAME_VAR(double, PS_d_32767, 0x00900CE8);
GAME_VAR(ped::CVector, PS_zeroVector, 0x00BCC564);
GAME_VAR(void*, PS_actionMgr9C0, 0x01B649C0);         // vf19: a node for the intelligence's +0x358
GAME_VAR(void*, PS_actionMgr9B8, 0x01B649B8);         // vf19: a node for the intelligence's +0x300
GAME_VAR(uint8_t, PS_splashing, 0x00C2B1B4);
GAME_VAR(void*, PS_vehicleSomething, 0x00CF4274);
inline void* const PS_waterLevels = reinterpret_cast<void*>(0x00BC5DA8);
inline void* const PS_soundMgr = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const PS_bankMgr = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const PS_camera = reinterpret_cast<void*>(0x00C3CC68);

// The player's reset state (0xC2B109 .. 0xC2B14C, 0xA3393C)
GAME_VAR(uint8_t, PS_C2B109, 0x00C2B109);
GAME_VAR(uint8_t, PS_C2B10A, 0x00C2B10A);
GAME_VAR(int32_t, PS_C2B10C, 0x00C2B10C);
GAME_VAR(int32_t, PS_C2B110, 0x00C2B110);
GAME_VAR(int32_t, PS_C2B114, 0x00C2B114);
GAME_VAR(int32_t, PS_C2B11C, 0x00C2B11C);
GAME_VAR(ped::CVector, PS_C2B144, 0x00C2B144);
GAME_VAR(float, PS_A3393C, 0x00A3393C);
GAME_VAR(float, PS_A33940, 0x00A33940);

// The weapon sound banks
GAME_VAR(int32_t, PS_lastWeaponModel, 0x00A339A8);
GAME_VAR(uint32_t, PS_bankTime, 0x00A339AC);          // (-1: none)
GAME_VAR(int32_t, PS_bankModel, 0x00A339B0);          // (-1: none)
GAME_VAR(uint8_t, PS_bankPending, 0x00C2B1B5);
GAME_VAR(float, PS_f_10, 0x00900D38);
GAME_VAR(int32_t, PS_modelRat, 0x00BF42DC);
GAME_VAR(int32_t, PS_modelDog1, 0x00BF42D4);
GAME_VAR(int32_t, PS_modelDog2, 0x00BF42D0);
GAME_VAR(int32_t, PS_modelDog3, 0x00BF42CC);
GAME_VAR(int32_t, PS_weapon444C, 0x00BF444C);
GAME_VAR(int32_t, PS_weapon44D8, 0x00BF44D8);
GAME_VAR(int32_t, PS_weapon4448, 0x00BF4448);
GAME_VAR(int32_t, PS_weapon4338, 0x00BF4338);
GAME_VAR(int32_t, PS_weapon4444, 0x00BF4444);
GAME_VAR(int32_t, PS_weapon443C, 0x00BF443C);
GAME_VAR(int32_t, PS_weapon4440, 0x00BF4440);
GAME_VAR(int32_t, PS_weapon4438, 0x00BF4438);

// Holding hands
GAME_VAR(float, PS_f_2p32, 0x00900AC8);               // 4294967296
GAME_VAR(float, PS_holdDelay, 0x00A339B8);            // 2000 ms
GAME_VAR(float, PS_holdMinTime, 0x00A339B4);          // 1
GAME_VAR(float, PS_holdRange, 0x0090CEC8);            // 1.1
GAME_VAR(float, PS_holdRangeHolding, 0x0090CEC4);     // 1.6
GAME_VAR(float, PS_f_0, 0x008FF1F0);

// Foot placement
GAME_VAR(float, PS_footDrop, 0x00A33948);             // 0.145
GAME_VAR(float, PS_footRaise, 0x00C2B128);
GAME_VAR(int32_t, PS_footLimitRange, 0x00A339C0);
GAME_VAR(float, PS_footRange, 0x00A339BC);            // 2
GAME_VAR(int32_t, PS_footAlways, 0x00C2B1B8);

GAME_VAR(double, PS_d_0_1847944, 0x0090C720);
GAME_VAR(float, PS_f_0_5, 0x009055A0);
GAME_VAR(double, PS_d_0_1, 0x00900128);

GAME_FN(void, __thiscall, PS_ClearBlends, 0x00473AC0, CPed* ped);
GAME_FN(void, __thiscall, PS_Fn48C580, 0x0048C580, void* self);
GAME_FN(void, __thiscall, PS_Fn48CEB0, 0x0048CEB0, void* self);
GAME_FN(void, __thiscall, PS_Fn488CB0, 0x00488CB0, void* self);
GAME_FN(void, __thiscall, PS_Intel4840A0, 0x004840A0, void* intelligence);
inline int __cdecl PS_rand() { return crt::Rand(); }
GAME_FN(void, __thiscall, PS_ActionRelease, 0x005D9C50, ped::ActionHandle* handle);
GAME_FN(uint8_t, __thiscall, PS_GetIsOnScreen, 0x00467460, CPed* ped);
GAME_FN(int, __cdecl, PS_GetAlpha, 0x005F2B10, void* rwObject);
GAME_FN(void, __cdecl, PS_SetAlpha, 0x005F2B00, void* rwObject, int alpha);
GAME_FN(ped::CVector*, __cdecl, PS_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(void, __thiscall, PS_SortByDistance, 0x00475530, CPed* ped, re::Ptr32<CEntity>* entities, int from, int to);
GAME_FN(uint8_t, __thiscall, PS_Entity465A60, 0x00465A60, CPed* ped, int flag);
GAME_FN(uint8_t, __thiscall, PS_GetWaterLevel, 0x006D4A20, void* self, float* level, int32_t* cache,
        const ped::CVector* pos);
GAME_FN(void, __thiscall, PS_Fn499A40, 0x00499A40, void* self);
GAME_FN(void, __thiscall, PS_SetNode, 0x005F3D20, void* controller, void* node, int a, int b);
GAME_FN(void, __thiscall, PS_PlayNode, 0x005F4AC0, void* controller, void* node);
GAME_FN(uint8_t, __thiscall, PS_IsNodePlaying, 0x005F4800, void* controller, void* node, int a, int b);
GAME_FN(int, __thiscall, PS_SoundGetId, 0x00595B80, void* mgr, const char* name);
GAME_FN(void, __thiscall, PS_SoundPlay, 0x0059B8A0, void* mgr, int id, const ped::CVector* pos, int a, uintptr_t b, int c,
        int d, int e, int f, uintptr_t g, uintptr_t h, int i, int j, float volume, int k);
GAME_FN(int, __thiscall, PS_BankFind, 0x00597FC0, void* mgr, const char* name);
GAME_FN(int, __thiscall, PS_BankForModel, 0x00598180, void* mgr, int model);
GAME_FN(uint8_t, __thiscall, PS_BankIsLoaded, 0x005995F0, void* mgr, int bank);
GAME_FN(void, __thiscall, PS_BankRequest, 0x0059A860, void* mgr, int bank, int time, int a);
GAME_FN(ped::CVector*, __thiscall, PS_CameraGetPosition, 0x0041AAC0, void* camera);
GAME_FN(float, __thiscall, PS_VectorMagnitude, 0x0040DFB0, const ped::CVector* v);
GAME_FN(void, __thiscall, PS_Fn5924E0, 0x005924E0, void* self, CPed* ped, int a);
GAME_FN(uint8_t, __cdecl, PS_Fn4469A0, 0x004469A0);
GAME_FN(float, __cdecl, PS_GetTimeSeconds, 0x00425710);
GAME_FN(ped::CVector*, __thiscall, PS_GetRight, 0x004736F0, CPed* ped, ped::CVector* out);
GAME_FN(float, __thiscall, PS_VectorNormalise, 0x00414E10, ped::CVector* v);
GAME_FN(float, __cdecl, PS_DotProduct, 0x00414640, const ped::CVector* a, const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PS_Vector_From768A70, 0x00472E80, ped::CVector* out, int a, void* object);
GAME_FN(ped::CVector*, __cdecl, PS_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(float, __cdecl, PS_ProjectedZ, 0x00473670, float x, float y, float z, ped::CVector from, ped::CVector to);
GAME_FN(void, __thiscall, PS_MoveTargetSet, 0x00472D70, void* target, const ped::CVector* pos, float time);
GAME_FN(uint8_t, __cdecl, PS_Fn683A30, 0x00683A30);
GAME_FN(uint8_t, __cdecl, PS_Fn683B70, 0x00683B70, CPed* ped);
GAME_FN(uint8_t*, __cdecl, PS_Fn4D34B0, 0x004D34B0, void* a);

void* Actions258(CPed* ped) {
    return ped->intelligence + 0x258;
}

// 0x4750A0 (thiscall): its state back to the start (and the player's globals with it)
void __fastcall CPed_Reset(CPed* self, re::Edx) {
    PS_ClearBlends(self);
    At<int32_t>(self, 0x30) = 1;
    At<int32_t>(self, 0x3C) = 1;
    PS_Fn48C580(self->unknown3BC);
    PS_Fn48CEB0(self->unknownA64);
    At<float>(self, 0x1D5C) = 0.0f;
    self->vehicle = nullptr;
    self->desiredHeading = 0.0f;
    At<float>(self, 0x1708) = 0.0f;
    At<float>(self, 0x170C) = 0.0f;
    self->lastVehicle = nullptr;
    At<int32_t>(self, 0x1D60) = 0;
    At<int32_t>(self, 0x18B8) = 0;
    At<float>(self, 0x1710) = 0.0f;
    At<int32_t>(self, 0x18B4) = 0;
    PS_Fn488CB0(Bytes(self) + 0x1314);
    At<int32_t>(self, 0x13FC) = 0;
    At<int32_t>(self, 0x1D50) = 0;
    At<int32_t>(self, 0x1D4C) = -1;
    for (uint32_t offset = 0x1C20; offset <= 0x1C3C; offset += 4) {
        At<int32_t>(self, offset) = 0;
    }
    for (int i = 0; i < 0x40; i++) {
        At<int32_t>(self, 0x1900 + i * 4) = 0;
    }
    At<int32_t>(self, 0x1D54) = -1;
    At<uint32_t>(self, 0x18BC) = PS_timeInMs;
    At<int32_t>(self, 0x16DC) = 0;
    PS_Intel4840A0(self->intelligence);
    int r = PS_rand();
    At<int32_t>(self, 0x1D30) = 0;
    self->group = nullptr;
    self->unknown1D24 = nullptr;
    self->unknown1D28 = nullptr;
    self->punishment = 0;
    self->unknown1D44 = 0;
    self->punishmentTime = 0;
    At<int32_t>(self, 0x18DC) = 1;
    At<float>(self, 0x1748) = static_cast<float>((r / PS_d_32767) * 1.0 + 0.0);
    PS_ActionRelease(&At<ped::ActionHandle>(self, 0x18E0));
    PS_ActionRelease(&At<ped::ActionHandle>(self, 0x18E4));
    PS_ActionRelease(&At<ped::ActionHandle>(self, 0x18E8));
    At<float>(self, 0x18B0) = 0.0f;
    At<uint8_t>(self, 0x1C9A) = 0;
    if (self->pedType == PED_TYPE_PLAYER) {
        ped::CVector zero = PS_zeroVector;
        PS_C2B10C = 0;
        PS_C2B110 = 0;
        PS_C2B114 = 0;
        PS_C2B144 = zero;
        PS_C2B109 = 0;
        At<int32_t>(self, 0x151C) = 0;
        PS_A3393C = 1.0f;
        PS_A33940 = 1.0f;
        PS_C2B10A = 0;
        PS_C2B11C = 0;
    }
}

// 0x475420 (thiscall): fades in (16 a frame), or out once it's to go (+0x1338: after a
// minute, or 3 seconds unseen); an ambient ped only
void __fastcall CPed_UpdateFade(CPed* self, re::Edx) {
    if (self->unknown1384 != 0 || !self->rwObject) {
        return;
    }
    if (At<int32_t>(self, 0x1338) == 0 && self->unknown1C98 != 0 && self->pedType != PED_TYPE_PLAYER) {
        int age = static_cast<int>(PS_timeInMs - At<uint32_t>(self, 0x18B8));
        CPed* other = self->unknown4F0[self->unknown504];
        if (age > 60000) {
            At<int32_t>(self, 0x1338) = 1;
        } else if (age > 3000 && !PS_GetIsOnScreen(self)) {
            if ((PS_population[0x7898] & 1) || !other || other->pedType != PED_TYPE_PLAYER) {
                At<int32_t>(self, 0x1338) = 1;
            }
        }
    }
    int alpha = PS_GetAlpha(self->rwObject);
    if (At<int32_t>(self, 0x1338) != 0) {
        alpha += self->unknown1C98 ? -6 : -8;
        if (alpha < 0) {
            PS_SetAlpha(self->rwObject, 0);
            return;
        }
    } else if (self->pedType != PED_TYPE_PLAYER || PS_C3CCF8 != PED_TYPE_PLAYER) {
        alpha += 0x10;
        if (alpha > 0xFF) {
            alpha = 0xFF;
        }
    }
    PS_SetAlpha(self->rwObject, alpha);
}

float DistanceTo(const ped::CVector* from, void* entity) {
    ped::CVector d;
    ped::CVector* v = PS_VectorSub(&d, from, PosOf(entity));
    float sq = std::fabs(v->x * v->x + v->y * v->y + v->z * v->z);
    return static_cast<float>(std::sqrt(static_cast<double>(sq)));
}

// 0x475530 (thiscall): entities sorted nearest first (a quicksort)
void __fastcall CPed_SortByDistance(CPed* self, re::Edx, re::Ptr32<CEntity>* entities, int from, int to) {
    ped::CVector pos = *PosOf(self);
    if (from >= to) {
        return;
    }
    float pivot = DistanceTo(&pos, entities[(from + to) / 2]);
    int i = from;
    int j = to;
    for (;;) {
        while (i <= to && pivot > DistanceTo(&pos, entities[i])) {
            i++;
        }
        while (j >= 0 && DistanceTo(&pos, entities[j]) > pivot) {
            j--;
        }
        if (i > j) {
            break;
        }
        CEntity* swap = entities[i];
        entities[i] = entities[j];
        entities[j] = swap;
        i++;
        j--;
        if (i > j) {
            break;
        }
    }
    if (j < to && j >= 0) {
        PS_SortByDistance(self, entities, from, j);
    }
    if (i > from) {
        PS_SortByDistance(self, entities, i, to);
    }
}

// 0x4757A0 (thiscall): in water (outside only): its nodes for it, health at least 1,
// and a splash the first frame of a jump into it
void __fastcall CPed_UpdateInWater(CPed* self, re::Edx) {
    if (PS_currArea != 0) {
        At<uint8_t>(self, 0x16CC) = 0;
        return;
    }
    if (self->pedType == PED_TYPE_PLAYER || PS_Entity465A60(self, 0x10)) {
        int32_t cache = At<int32_t>(self, 0x1620);
        float* level = &At<float>(self, 0x16D0);
        uint8_t inWater = PS_GetWaterLevel(PS_waterLevels, level, &cache, PosOf(self));
        At<uint8_t>(self, 0x16CC) = inWater;
        At<int32_t>(self, 0x1620) = cache;
        if (inWater) {
            *level = 0.0f;
        }
    }
    if (!At<uint8_t>(self, 0x16CC)) {
        return;
    }
    void* node = reinterpret_cast<void*(__thiscall*)(void*, void*)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(PS_actionMgr9C0))[0x13]))(
        PS_actionMgr9C0, self->intelligence + 0x358);
    if (self->pedType != PED_TYPE_PLAYER && node) {
        if (At<int32_t>(self, 0x360) != 0) {
            PS_Fn499A40(Bytes(self) + 0x358);
        }
        PS_SetNode(self->intelligence + 0x290, node, 0, 0);
    }
    node = reinterpret_cast<void*(__thiscall*)(void*, void*)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(PS_actionMgr9B8))[0x13]))(
        PS_actionMgr9B8, self->intelligence + 0x300);
    if (node) {
        PS_PlayNode(Actions258(self), node);
    }
    if (!(0.0f < self->health)) {
        self->health = 1.0f;
    }
    bool splashing = PS_IsNodePlaying(Actions258(self), reinterpret_cast<void*>(0x01B652B8), 1, 1) ||
                     PS_IsNodePlaying(Actions258(self), reinterpret_cast<void*>(0x01B652BC), 1, 1);
    if (!PS_splashing && splashing) {
        int id = PS_SoundGetId(PS_soundMgr, reinterpret_cast<const char*>(0x0090CE90));  // "Jump_Splash"
        PS_SoundPlay(PS_soundMgr, id, PosOf(self), 0, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
        PS_splashing = 1;
    }
    if (!splashing) {
        PS_splashing = 0;
    }
    At<float>(self, 0x18D8) = 0.0f;
}

// 0x475980 (thiscall): the sound bank for its weapon (or its kind: rats, dogs)
// requested; the player's changes are held back a while (0xA339AC)
void __fastcall CPed_RequestSoundBank(CPed* self, re::Edx) {
    int model = self->modelIndex;
    int bank = -1;
    int time = 2000;
    int weapon;
    if (model == PS_modelRat) {
        bank = PS_BankFind(PS_bankMgr, reinterpret_cast<const char*>(0x0090CEB0));  // "Objects\PedRats.bnk"
        goto request;
    }
    if (model == PS_modelDog1 || model == PS_modelDog2 || model == PS_modelDog3) {
        bank = PS_BankFind(PS_bankMgr, reinterpret_cast<const char*>(0x0090CE9C));  // "Objects_2\Dog.bnk"
        goto request;
    }
    // (the weapon slot isn't checked for null)
    if (At<int16_t>(self->weapon, 0x10E) == -1) {
        uint32_t now = PS_timeInMs;
        uint32_t until;
        if (self->pedType == PED_TYPE_PLAYER && PS_bankPending) {
            until = now + 6000;
            PS_bankPending = 0;
            PS_bankTime = until;
        } else {
            until = PS_bankTime;
        }
        if (self->pedType == PED_TYPE_PLAYER && static_cast<int32_t>(until) > 0 && now < until) {
            bank = PS_BankForModel(PS_bankMgr, PS_bankModel);
            time = 500;
        }
        goto request;
    }
    weapon = At<int16_t>(self->weapon, 0x10E);
    if (self->pedType != PED_TYPE_PLAYER && weapon != PS_weapon444C && weapon != PS_weapon44D8 &&
        weapon != PS_weapon4448 && weapon != PS_weapon4338 && weapon != PS_weapon4444 && weapon != PS_weapon443C &&
        weapon != PS_weapon4440 && weapon != PS_weapon4438) {
        ped::CVector camera = *PS_CameraGetPosition(PS_camera);
        ped::CVector d;
        ped::CVector* v = PS_VectorSub(&d, &camera, PosOf(self));
        if (PS_VectorMagnitude(v) > PS_f_10) {
            goto request;
        }
        PS_bankPending = 1;
    }
    bank = PS_BankForModel(PS_bankMgr, At<int16_t>(self->weapon, 0x10E));
    if (self->pedType == PED_TYPE_PLAYER) {
        weapon = At<int16_t>(self->weapon, 0x10E);
        if (PS_bankModel != -1 && PS_bankModel != weapon) {
            PS_bankTime = (weapon == PS_weapon44D8 || weapon == PS_weapon444C) ? PS_timeInMs : PS_timeInMs + 500;
            PS_bankModel = weapon;
            bank = -1;
        } else if (PS_bankTime != 0xFFFFFFFF && PS_timeInMs <= PS_bankTime) {
            bank = -1;
        } else {
            PS_bankTime = 0xFFFFFFFF;
            PS_bankModel = At<int16_t>(self->weapon, 0x10E);
            time = 500;
        }
    } else {
        time = 5000;
    }
request:
    if (self->pedType == PED_TYPE_PLAYER) {
        if (PS_lastWeaponModel != At<int16_t>(self->weapon, 0x10E) && self->unknown143C != 0) {
            PS_Fn5924E0(PS_vehicleSomething, self, 0);
        }
        PS_lastWeaponModel = At<int16_t>(self->weapon, 0x10E);
    }
    if (bank != -1 &&
        (PS_BankIsLoaded(PS_bankMgr, bank) || self->pedType == PED_TYPE_PLAYER || !PS_Fn4469A0())) {
        PS_BankRequest(PS_bankMgr, bank, time, 0);
    }
}

// 0x475C60 (cdecl): two peds holding hands (+0x153C) while both play the node, close
// enough and not too soon (+0x550 + 2 s); the side they hold by (+0x1540) from where
// the other is
void __cdecl CPed_UpdateHoldingHands(CPed* a, CPed* b) {
    if (!b) {
        goto clearBoth;
    }
    if (!a) {
        goto clearB;
    }
    if (At<int32_t>(b, 0x14EC) == 0 || At<int32_t>(a, 0x14EC) == 0) {
        goto clearBoth;
    }
    {
        // (unsigned to float the way the game does it: signed, plus 2^32 when negative)
        double nowD = static_cast<double>(static_cast<int32_t>(PS_timeInMs));
        if (static_cast<int32_t>(PS_timeInMs) < 0) {
            nowD = nowD + PS_f_2p32;
        }
        float now = static_cast<float>(nowD);
        auto startOf = [](CPed* p) {
            int32_t t = At<int32_t>(p, 0x550);
            double v = static_cast<double>(t);
            if (t < 0) {
                v = v + PS_f_2p32;
            }
            return static_cast<float>(v + PS_holdDelay);
        };
        bool tooSoonA = now < startOf(a);
        bool tooSoonB = now < startOf(b);
        void* armA = At<void*>(a, 0x16F4);
        void* armB = At<void*>(b, 0x16F8);
        if (At<int32_t>(a, 0x1540) == 0) {
            armA = At<void*>(a, 0x16F8);
            armB = At<void*>(b, 0x16F4);
        }
        uint8_t playingA = PS_IsNodePlaying(Actions258(a), reinterpret_cast<void*>(0x01B652F0), 1, 1);
        uint8_t playingB = PS_IsNodePlaying(Actions258(b), reinterpret_cast<void*>(0x01B652F0), 1, 1);
        auto armVf16 = [](void* arm) {
            return reinterpret_cast<uint8_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(arm))[0x40 / 4]))(arm);
        };
        bool holding = At<int32_t>(a, 0x153C) != 0 && At<int32_t>(b, 0x153C) != 0 && armVf16(armA) && armVf16(armB);
        float range = holding ? PS_holdRangeHolding : PS_holdRange;
        ped::CVector posA = *PosOf(a);
        ped::CVector posB = *PosOf(b);
        ped::CVector d;
        PS_VectorSub(&d, &posB, &posA);
        bool close = !tooSoonA && !tooSoonB && playingA && playingB && At<int32_t>(a, 0x131C) == 0 &&
                     At<int32_t>(b, 0x131C) == 0 && range > PS_VectorMagnitude(&d);
        if (!holding) {
            ped::CVector right;
            PS_GetRight(a, &right);
            PS_VectorNormalise(&d);
            int side = PS_DotProduct(&right, &d) > PS_f_0 ? 1 : 0;
            At<int32_t>(a, 0x1540) = side;
            At<int32_t>(b, 0x1540) = side;
            if (close) {
                float t = PS_GetTimeSeconds();
                if (t < At<float>(a, 0x16FC) + PS_holdMinTime) {
                    close = false;
                }
                if (PS_holdMinTime + At<float>(b, 0x16FC) > t) {
                    At<int32_t>(a, 0x153C) = 0;
                    At<int32_t>(b, 0x153C) = 0;
                    return;
                }
                At<int32_t>(a, 0x153C) = close ? 1 : 0;
                At<int32_t>(b, 0x153C) = close ? 1 : 0;
                return;
            }
        }
        At<float>(a, 0x16FC) = PS_GetTimeSeconds();
        At<float>(b, 0x16FC) = PS_GetTimeSeconds();
        At<int32_t>(a, 0x153C) = close ? 1 : 0;
        At<int32_t>(b, 0x153C) = close ? 1 : 0;
        return;
    }
clearBoth:
    if (a) {
        At<int32_t>(a, 0x153C) = 0;
        At<int32_t>(a, 0x14EC) = 0;
    }
clearB:
    if (b) {
        At<int32_t>(b, 0x153C) = 0;
        At<int32_t>(b, 0x14EC) = 0;
    }
}

// A foot's target (see 0x472CC0 / 0x472D70)
struct FootTarget {
    re::Ptr32<uint8_t> bone;            // +0x00 (+0x24 its bone index; the foot is the next bone)
    ped::CVector target;      // +0x04
    uint8_t unknown10[0x10];
    int32_t unknown20;        // +0x20
    int32_t unknown24;        // +0x24
    float unknown28;          // +0x28
    float unknown2C;          // +0x2C
    re::Ptr32<void> offsetSource;       // +0x30 (vf0: a value for 0x472E80)
    uint8_t unknown34[4];
    uint32_t flags;           // +0x38 bit 0: placed
};
static_assert(offsetof(FootTarget, offsetSource) == 0x30);
static_assert(offsetof(FootTarget, flags) == 0x38);

// A plane: a point and its normal
struct FootPlane {
    ped::CVector point;       // +0x00
    float unknownC;
    ped::CVector normal;      // +0x10
};

// 0x475F80 (cdecl): a foot below the plane (or always, with 0xC2B1B8) gets a target on it
void __cdecl CPed_PlaceFoot(CPed* ped, FootPlane* plane, FootTarget* foot) {
    if (!plane || !foot) {
        return;
    }
    int index = At<int32_t>(foot->bone, 0x24) + 1;
    uint8_t* bone = At<uint8_t*>(At<uint8_t*>(ped->animBlend, 0x14), 8) + index * 0x34;
    float boneX = At<float>(bone, 0x24);
    float boneY = At<float>(bone, 0x28);
    float boneZ = At<float>(bone, 0x2C);
    ped::CVector p = {boneX, boneY, boneZ};
    if (foot->offsetSource) {
        ped::CVector offset = {0.0f, 0.0f, 0.0f};
        // (vf0 is called with the bone left on the stack for 0x472E80, as its third argument)
        int value = reinterpret_cast<int(__thiscall*)(void*)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(foot->offsetSource))[0]))(
            foot->offsetSource);
        PS_Vector_From768A70(&offset, value, bone);
        p.x = p.x + offset.x;
        p.y = offset.y + p.y;
        p.z = offset.z + p.z;
    } else {
        p.z = boneZ - PS_footDrop;
    }
    ped::CVector raise = {0.0f, 0.0f, PS_footRaise};
    ped::CVector unused;
    PS_VectorAdd(&unused, &p, &raise);
    float height = PS_ProjectedZ(plane->normal.x, plane->normal.y, plane->normal.z, plane->point, p);
    if (PS_footLimitRange != 0) {
        ped::CVector d;
        ped::CVector* v = PS_VectorSub(&d, &plane->point, &p);
        float dist = static_cast<float>(std::sqrt(static_cast<double>(std::fabs(v->x * v->x + v->y * v->y + v->z * v->z))));
        if (PS_footRange < dist) {
            height = 0.0f;
        }
    }
    if (PS_footAlways == 0 && !(height < 0.0f)) {
        foot->flags &= ~1u;
        foot->unknown2C = 0.0f;
        foot->unknown28 = 0.0f;
        return;
    }
    ped::CVector target = {boneX, boneY, boneZ - height};
    foot->flags |= 1;
    foot->unknown2C = 0.0f;
    foot->unknown28 = 0.0f;
    PS_MoveTargetSet(foot, &target, 0.0f);
}

// 0x4762F0 (thiscall): the ped can use an entity (with +0xC4): +0xFC, standing on it
// still, 0x683B70, or its owner
uint8_t __fastcall CPed_CanUseEntity(CPed* self, re::Edx, CEntity* entity) {
    if (At<int32_t>(entity, 0xC4) == 0) {
        return 0;
    }
    if (At<int32_t>(self, 0xFC) != 0) {
        return 1;
    }
    if (PS_Fn683A30()) {
        const ped::CVector* e = PosOf(entity);
        const ped::CVector* p = PosOf(self);
        float dz = e->z - p->z;
        dz = static_cast<float>(dz - PS_d_0_1847944);
        if (dz < PS_f_0_5 && PS_VectorMagnitude(&At<ped::CVector>(entity, 0x124)) < PS_d_0_1) {
            return 1;
        }
        if (PS_Fn683B70(self)) {
            return 1;
        }
    }
    uint8_t* owner = PS_Fn4D34B0(At<void*>(entity, 0x238));
    return At<CPed*>(owner, 4) == self ? 1 : 0;
}

// 0x4763C0 (thiscall): the entity is its own (+0x430 unless +0x13B8, the player's
// +0x1E94, or its +0x430 of kind 7 with +0x46C 1)
uint8_t __fastcall CPed_IsOwnEntity(CPed* self, re::Edx, CEntity* entity) {
    CEntity* own = self->unknown430;
    if (At<int32_t>(self, 0x13B8) == 0 && own == entity) {
        return 1;
    }
    if (self->pedType == PED_TYPE_PLAYER && entity == At<CEntity*>(self, 0x1E94)) {
        return 1;
    }
    if (entity == own && At<int32_t>(own, 0x46C) == 1 && (At<uint8_t>(own, 0x108) & 0xF8) == 0x38) {
        return 1;
    }
    return 0;
}

#define PEDSTATE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDSTATE_REVERSED("CPed::Reset", 0x004750A0, &CPed_Reset);
PEDSTATE_REVERSED("CPed::UpdateFade", 0x00475420, &CPed_UpdateFade);
PEDSTATE_REVERSED("CPed::SortByDistance", 0x00475530, &CPed_SortByDistance);
PEDSTATE_REVERSED("CPed::UpdateInWater", 0x004757A0, &CPed_UpdateInWater);
PEDSTATE_REVERSED("CPed::RequestSoundBank", 0x00475980, &CPed_RequestSoundBank);
PEDSTATE_REVERSED("CPed::UpdateHoldingHands", 0x00475C60, &CPed_UpdateHoldingHands);
PEDSTATE_REVERSED("CPed::PlaceFoot", 0x00475F80, &CPed_PlaceFoot);
PEDSTATE_REVERSED("CPed::CanUseEntity", 0x004762F0, &CPed_CanUseEntity);
PEDSTATE_REVERSED("CPed::IsOwnEntity", 0x004763C0, &CPed_IsOwnEntity);

} // namespace
