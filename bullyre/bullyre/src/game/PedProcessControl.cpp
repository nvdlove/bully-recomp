// CPed::ProcessControl (vf12, 0x481420): the ped's per-frame update: its action
// controllers, sound bank, a seated ambient ped kept out of the world when far and
// unsupported, blends, collision models, water, fade, nearby peds, its members' updates,
// physics, meters, health, the player's falls and drowning, vehicle action trees, the
// feet's collision spheres, a fade value, holding hands.

#include <cmath>

#include "game/Ped.h"

#include "game/Crt.h"
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

GAME_VAR(uint32_t, PP_frameCounter, 0x00C1A99C);      // CTimer::m_FrameCounter
GAME_VAR(uint32_t, PP_timeInMs, 0x00C1A9B4);
GAME_VAR(float, PP_timeStep, 0x00C1A9A4);
GAME_VAR(float, PP_stepSeconds, 0x00A3396C);          // 0.02
GAME_VAR(CPed*, PP_player, 0x00C1AEA8);
GAME_VAR(int32_t, PP_currArea, 0x00BD1008);
GAME_VAR(int32_t, PP_weapon4330, 0x00BF4330);
GAME_ARRAY(uint8_t*, PP_modelInfoPtrs, 0x00C67738);
GAME_VAR(void*, PP_colModel3130, 0x00CF3130);
GAME_VAR(void*, PP_colModel3134, 0x00CF3134);
GAME_VAR(void*, PP_colModel312C, 0x00CF312C);
GAME_VAR(float, PP_pedRadius, 0x00CF311C);
GAME_VAR(float, PP_f_80, 0x009051D8);
GAME_VAR(float, PP_f_0_1, 0x00900170);
GAME_VAR(float, PP_f_1000, 0x00900F40);
GAME_VAR(float, PP_f_9999, 0x0090D1C8);
GAME_VAR(float, PP_f_1_2, 0x00900DF8);
GAME_VAR(float, PP_fallDepth, 0x0090D1C4);            // -9
GAME_VAR(double, PP_d_32767, 0x00900CE8);
GAME_VAR(double, PP_d_100, 0x00900130);
GAME_VAR(double, PP_d_0, 0x009009A0);
GAME_VAR(float, PP_f_epsilon, 0x0090D198);
GAME_VAR(double, PP_d_almostOne, 0x0090D190);
GAME_VAR(uint8_t, PP_C2B109, 0x00C2B109);
GAME_VAR(int32_t, PP_fallFrames, 0x00C2B11C);
GAME_VAR(uint8_t, PP_C1AF00, 0x00C1AF00);
GAME_VAR(void*, PP_C2C1A0, 0x00C2C1A0);
inline void* const PP_camera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const PP_20C7E98 = reinterpret_cast<void*>(0x020C7E98);

GAME_FN(uint8_t, __thiscall, PP_Entity465A60, 0x00465A60, CPed* ped, int flag);
GAME_FN(void, __thiscall, PP_Fn5DCD80, 0x005DCD80, void* self);
GAME_FN(void, __thiscall, PP_Fn5DF1E0, 0x005DF1E0, void* self);
GAME_FN(void, __thiscall, PP_RequestSoundBank, 0x00475980, CPed* ped);
GAME_FN(void, __cdecl, PP_Fn597D50, 0x00597D50, CPed* ped);
GAME_FN(void, __thiscall, PP_UpdateGroundRumble, 0x00476410, CPed* ped);
GAME_FN(void, __thiscall, PP_Fn490BE0, 0x00490BE0, void* self);
GAME_FN(void, __thiscall, PP_Fn72EDE0, 0x0072EDE0, void* self);
GAME_FN(float, __thiscall, PP_GetDistanceFromCentreOfMassToBase, 0x00466110, CPed* ped);
GAME_FN(uint8_t, __cdecl, PP_Fn570330, 0x00570330, void* xy, int32_t* area);
GAME_FN(uint8_t, __cdecl, PP_Fn430320, 0x00430320, int area);
GAME_FN(float, __thiscall, PP_DistanceToPlayer, 0x004777B0, CPed* ped);
GAME_FN(float, __cdecl, PP_RandomFloat, 0x0041A5B0, float from, float to);
GAME_FN(float, __cdecl, PP_FindGround4635A0, 0x004635A0, float x, float y, float z, uint8_t* found, int flags);
GAME_FN(void, __thiscall, PP_SetPositionV, 0x0041EE20, CPed* ped, const ped::CVector* pos);
GAME_FN(void, __thiscall, PP_UpdateBlends, 0x00473B10, CPed* ped);
GAME_FN(void, __thiscall, PP_Fn450970, 0x00450970, CPed* ped);
GAME_FN(void, __thiscall, PP_ModelInfoFn50EB30, 0x0050EB30, void* modelInfo, void* colModel, int a);
GAME_FN(void, __thiscall, PP_UpdateInWater, 0x004757A0, CPed* ped);
GAME_FN(uint8_t, __thiscall, PP_IsNodePlaying, 0x005F4800, void* controller, void* node, int a, int b);
GAME_FN(void, __thiscall, PP_Fn48D7F0, 0x0048D7F0, void* self);
GAME_FN(void, __thiscall, PP_UpdateFade, 0x00475420, CPed* ped);
GAME_FN(void, __thiscall, PP_UpdateNearbyPeds, 0x0047D320, CPed* ped, int rebuild);
GAME_FN(void, __thiscall, PP_Fn485BE0, 0x00485BE0, void* self);
GAME_FN(void, __thiscall, PP_Fn499A20, 0x00499A20, void* self);
GAME_FN(void, __thiscall, PP_Fn4878D0, 0x004878D0, void* self);
GAME_FN(void, __thiscall, PP_Fn48CE40, 0x0048CE40, void* self);
GAME_FN(void, __thiscall, PP_Fn49A650, 0x0049A650, void* self);
GAME_FN(uint8_t, __thiscall, PP_Fn485320, 0x00485320, void* self);
GAME_FN(uint8_t, __thiscall, PP_Fn485330, 0x00485330, void* self);
GAME_FN(void, __thiscall, PP_PhysicalFn468DF0, 0x00468DF0, CPed* ped);
GAME_FN(void, __thiscall, PP_PhysicalProcessControl, 0x00469B70, CPed* ped);
GAME_FN(void, __thiscall, PP_AddMeter18C0, 0x0047CD90, CPed* ped, float amount, int delayed);
GAME_FN(void, __thiscall, PP_PhysicalFn469470, 0x00469470, CPed* ped);
GAME_FN(void, __thiscall, PP_CheckFall, 0x00473BC0, CPed* ped);
GAME_FN(void, __thiscall, PP_RegenerateHealth, 0x00473140, CPed* ped);
GAME_FN(void, __thiscall, PP_Fn4A6080, 0x004A6080, void* self, CPed* ped, void* a);
GAME_FN(void, __thiscall, PP_Fn48C250, 0x0048C250, void* self);
GAME_FN(void, __thiscall, PP_QueueFn4704E0, 0x004704E0, void* queue);
GAME_FN(void, __thiscall, PP_Fn499490, 0x00499490, CPed* ped);
GAME_FN(void, __thiscall, PP_RefillMeter18D0, 0x0047CF30, CPed* ped);
inline int __cdecl PP_rand() { return crt::Rand(); }
GAME_FN(int, __thiscall, PP_GetStat, 0x00477060, CPed* ped, int stat);
GAME_FN(void, __thiscall, PP_IntelFn485200, 0x00485200, void* intelligence, float a);
GAME_FN(void, __thiscall, PP_Fn4921D0, 0x004921D0, void* self);
GAME_FN(int, __thiscall, PP_CameraGetFadeState, 0x004F12F0, void* camera);
GAME_FN(uint8_t, __cdecl, PP_Fn45EAE0_x64, 0x0045EAE0, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t PP_Fn45EAE0(ped::CVector a0) {
    return PP_Fn45EAE0_x64(a0.x, a0.y, a0.z);
}
GAME_FN(void, __thiscall, PP_RaisePendingEvent, 0x004779E0, CPed* ped);
GAME_FN(int*, __thiscall, PP_VehicleFn4CAF30, 0x004CAF30, void* vehicle, int* out);
GAME_FN(void, __thiscall, PP_IntelLoadActions, 0x00484AF0, void* intelligence, const char* tree, const char* file);
GAME_FN(void, __thiscall, PP_TargetingFn4D8A40, 0x004D8A40, void* targeting, int a);
GAME_FN(void, __thiscall, PP_Fn48C190, 0x0048C190, void* self, int a);
GAME_FN(void, __thiscall, PP_RecordPosition, 0x00477AC0, CPed* ped);
GAME_FN(void, __thiscall, PP_Fn6D70B0, 0x006D70B0, void* self, CPed* ped);
GAME_FN(void*, __thiscall, PP_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(void, __thiscall, PP_MatrixCopy, 0x00412A10, void* self, const void* from);
GAME_FN(void*, __cdecl, PP_MatrixInvert, 0x00413770, const void* matrix, void* out);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline ped::CVector* PP_TransformPoint(ped::CVector* out, const void* matrix, const ped::CVector* v) { return re::CallVec16<0x0045D500, ped::CVector*>(out, matrix, v); }
GAME_FN(void, __thiscall, PP_SphereSet, 0x0056EB80, void* sphere, float radius, const ped::CVector* centre);
GAME_FN(void, __cdecl, PP_UpdateHoldingHands, 0x00475C60, CPed* a, CPed* b);
GAME_FN(void, __thiscall, PP_HandleSet, 0x005D7F10, void* handle, uintptr_t value);

void* ColModelFor(CPed* self) {
    if (At<int32_t>(self, 0x131C) != 0) {
        return PP_colModel3130;
    }
    return At<int32_t>(self, 0x1320) != 0 ? PP_colModel3134 : PP_colModel312C;
}

// A bone's position in the ped's own space (the inverse of its matrix at +0x1564)
ped::CVector BoneLocal(CPed* self, const void* inverse, int bone) {
    uint8_t* b = At<uint8_t*>(At<uint8_t*>(self->animBlend, 0x14), 8) + bone * 0x34;
    ped::CVector p = At<ped::CVector>(b, 0x24);
    ped::CVector out;
    PP_TransformPoint(&out, inverse, &p);
    return out;
}

// vf12 0x481420
void __fastcall CPed_ProcessControl(CPed* self, re::Edx) {
    void* actions = Bytes(self) + 0x11E4;
    if (At<int32_t>(self, 0x1390) != 0) {
        PP_Fn5DCD80(actions);
        PP_Fn5DF1E0(actions);
    } else if (PP_Entity465A60(self, 0x10)) {
        PP_Fn5DCD80(actions);
    } else if (PP_Entity465A60(self, 8)) {
        PP_Fn5DF1E0(actions);
    }
    if ((PP_frameCounter & 0x1FF) == 0) {
        At<int32_t>(self, 0x150C) = 0;
        At<int32_t>(self, 0x1510) = 0;
    }
    PP_RequestSoundBank(self);
    if (self->unknown1470 != 0 || At<int16_t>(self->weapon, 0x10E) == PP_weapon4330) {
        PP_Fn597D50(self);
        PP_UpdateGroundRumble(self);
    }
    PP_Fn490BE0(Bytes(self) + 0xB44);
    if (self->unknown1559 != 0 && At<int32_t>(self, 0x40) == 0) {
        At<int32_t>(self, 0x40) = 1;
        return;
    }
    PP_Fn72EDE0(self->unknown2EC);
    // a seated ambient ped: kept out of the world (+0x1E24 bit 1) while it has nothing
    // under it, or when far enough to be moved a little and still nothing
    if (self->pedType != PED_TYPE_PLAYER && self->seatState == 2 && At<int32_t>(self, 0x1758) == 0) {
        ped::CVector pos = *PosOf(self);
        pos.z = PP_GetDistanceFromCentreOfMassToBase(self) + pos.z;
        float xy[3] = {pos.x, pos.y, 0.0f};  // (the third float is uninitialised stack in the game)
        int32_t area = PP_currArea;
        uint8_t found = PP_Fn570330(xy, &area);
        if (area == PP_currArea && PP_Fn430320(area) && PP_DistanceToPlayer(self) > PP_f_80) {
            float r = PP_RandomFloat(PP_f_0_1, PP_pedRadius);
            PP_FindGround4635A0(pos.x + r, pos.y + r, pos.z, &found, 9);
            if (!found) {
                PP_FindGround4635A0(pos.x - r, pos.y - r, pos.z, &found, 9);
            }
        }
        if (!found) {
            At<int32_t>(self, 0x40) = 0;
            if (!(self->flags1E24 & 2)) {
                return;
            }
            if (self->vehicle) {
                reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(self->vehicle))[0x1C / 4]))(
                    self->vehicle, 1);
            } else {
                PP_SetPositionV(self, &pos);
            }
            self->flags1E24 &= ~2;
            At<ped::CVector>(self, 0x15E8) = {PP_f_1000, PP_f_1000, PP_f_1000};
            At<int32_t>(self, 0x1358) = 1;
            return;
        }
        if (self->vehicle) {
            reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(self->vehicle))[0x1C / 4]))(
                self->vehicle, 0);
        }
        self->flags1E24 |= 2;
        At<int32_t>(self, 0x1358) = 0;
    }
    PP_UpdateBlends(self);
    self->unknown1E14 = 0;
    self->unknown1E18 = *PosOf(self);
    At<float>(self->intelligence, 0x1C) = 0.0f;
    if (((At<uint8_t>(self, 0x10C) + PP_frameCounter) & 0x1F) == 0) {
        PP_Fn450970(self);
    }
    PP_ModelInfoFn50EB30(PP_modelInfoPtrs[self->modelIndex], ColModelFor(self), 0);
    At<uint8_t>(self, 0x16B8) = 0;
    PP_UpdateInWater(self);
    if (self->partner == PP_player &&
        (PP_IsNodePlaying(self->intelligence + 0x258, reinterpret_cast<void*>(0x01B654FC), 1, 1) ||
         PP_IsNodePlaying(self->intelligence + 0x258, reinterpret_cast<void*>(0x01B65500), 1, 1))) {
        PP_Fn48D7F0(Bytes(self) + 0xB44);
    }
    PP_UpdateFade(self);
    if (PP_timeInMs > At<uint32_t>(self, 0x18BC)) {
        At<uint32_t>(self, 0x18BC) = PP_timeInMs + 30000;
        At<int32_t>(self, 0x134C) = 0;
        At<int32_t>(self, 0x1354) = 0;
    }
    PP_UpdateNearbyPeds(self, 0);
    PP_Fn485BE0(Bytes(self) + 0x2F0);
    PP_Fn499A20(Bytes(self) + 0x358);
    PP_Fn4878D0(Bytes(self) + 0x668);
    PP_Fn48CE40(self->unknownA64);
    PP_Fn49A650(Bytes(self) + 0xD6C);
    if (PP_Fn485320(Bytes(self) + 0x2F0)) {
        self->unknown1624 = 1;
    }
    // physics (skipped for a still ambient ped, +0x1318)
    const ped::CVector& speed = At<ped::CVector>(self, 0x124);
    if (self->pedType != PED_TYPE_PLAYER && At<int32_t>(self, 0x1318) != 0 && speed.x == 0.0f && speed.y == 0.0f &&
        speed.z == 0.0f && At<float>(self, 0x1720) == 0.0f && At<float>(self, 0x1724) == 0.0f &&
        At<int32_t>(self, 0x1324) == 0) {
        At<int32_t>(self, 0x2C) = 0;
        At<int32_t>(self, 0x38) = 0;
        At<int32_t>(self, 0x40) = 0;
        At<int32_t>(self, 0x7C) = 0;
        At<uint8_t>(self, 0x181) = 0;
        At<int32_t>(self, 0x4C) = 0;
        PP_PhysicalFn468DF0(self);
    } else {
        PP_PhysicalProcessControl(self);
    }
    int8_t& counter = At<int8_t>(self, 0x16D4);
    if (counter > 0) {
        counter--;
    } else if (counter < 0) {
        counter++;
    }
    if (At<uint8_t>(self, 0x1E25) == 0 && PP_timeInMs > At<uint32_t>(self, 0x18F8)) {
        PP_AddMeter18C0(self, self->unknown18C4 * PP_timeStep, 1);
    }
    if (self->vehicle == nullptr && self->unknown1C98 == 0) {
        At<int32_t>(self, 0x20) = 1;
    } else {
        PP_PhysicalFn469470(self);
    }
    if (At<int32_t>(self, 0x1324) != 0) {
        PP_CheckFall(self);
    } else {
        At<float>(self, 0x18B0) = PosOf(self)->z;
    }
    float last = At<float>(self, 0x1DD8);
    if (PP_f_9999 <= last || At<int32_t>(self, 0x1324) == 0) {
        At<ped::CVector>(self, 0x1DD8) = *PosOf(self);
    }
    PP_RegenerateHealth(self);
    PP_Fn4A6080(PP_C2C1A0, self, Bytes(self) + 0x1D30);
    if (At<uint8_t>(self, 0x1E25) == 0) {
        reinterpret_cast<void(__thiscall*)(CPed*)>(static_cast<void*>(self->vtable[0xB0 / 4]))(self);
        PP_Fn48C250(self->unknown3BC);
        PP_QueueFn4704E0(&self->objectives);
        PP_Fn499490(self);
        if (At<uint8_t>(self, 0x1E25) == 0) {
            PP_RefillMeter18D0(self);
        }
    }
    int r = PP_rand();
    At<float>(self, 0x1748) = static_cast<float>((r / PP_d_32767) * 1.0 + 0.0);
    int stat = PP_GetStat(self, 0x14);
    float step = PP_stepSeconds * PP_timeStep;
    float scale = static_cast<float>(stat / PP_d_100);
    PP_IntelFn485200(self->intelligence, step * scale);
    At<float>(self, 0x1E78) = At<float>(self->intelligence, 0x1C);
    PP_Fn4921D0(Bytes(self) + 0xB44);
    // the player: falling out of the world, or under water too long
    if (self->pedType == PED_TYPE_PLAYER) {
        ped::CVector pos = *PosOf(self);
        bool dead = self->unknown1C98 != 0 || PP_CameraGetFadeState(PP_camera) != 0;
        bool count = false;
        if (At<uint8_t>(self, 0x16CC) && PP_Fn430320(PP_currArea)) {
            float depth = At<float>(self, 0x16D0) - pos.z;
            if (!(depth > PP_f_1_2) &&
                !PP_IsNodePlaying(self->intelligence + 0x258, reinterpret_cast<void*>(0x01B652C4), 1, 1)) {
                PP_C2B109 = 0;
            } else {
                PP_C2B109 = 1;
                if (!dead && !PP_Fn45EAE0(pos)) {
                    count = true;
                }
            }
        } else {
            PP_C2B109 = 0;
            if (!(PP_fallDepth < pos.z) && !std::isnan(pos.z) && !dead) {
                count = true;
            }
        }
        bool reset = true;
        if (count) {
            PP_fallFrames = PP_fallFrames + 1;
            if (PP_fallFrames <= 10) {
                reset = false;
            } else {
                reinterpret_cast<void(__thiscall*)(CPed*, int, int)>(static_cast<void*>(self->vtable[0xA8 / 4]))(self, 0, 0);
                PP_C1AF00 = 1;
            }
        }
        if (reset) {
            PP_fallFrames = 0;
        }
    }
    PP_ModelInfoFn50EB30(PP_modelInfoPtrs[self->modelIndex], ColModelFor(self), 0);
    PP_RaisePendingEvent(self);
    if (At<int32_t>(self, 0x1318) != 0 && At<int32_t>(self, 0x14D4) != 0) {
        At<int32_t>(self, 0x14D4) = 0;
    }
    // riding: the vehicle's action tree loaded
    uint8_t* vehicle = re::PtrCast<uint8_t*>(self->vehicle);
    if (vehicle) {
        int32_t kind = At<int32_t>(vehicle, 0x46C);
        const char* file = reinterpret_cast<const char*>(0x00901018);  // "Act/Vehicles.act"
        if (kind == 1) {
            if (self->intelligence[0x31D] == 0 &&
                !PP_IsNodePlaying(self->intelligence + 0x258, reinterpret_cast<void*>(0x01B652A8), 1, 1)) {
                int tmp;
                const char* tree;
                if (*PP_VehicleFn4CAF30(self->vehicle, &tmp) == 3) {
                    tree = reinterpret_cast<const char*>(0x0090D030);  // Motorcycle
                } else if (*PP_VehicleFn4CAF30(self->vehicle, &tmp) == 8) {
                    tree = reinterpret_cast<const char*>(0x0090D008);  // Scooter
                } else {
                    tree = reinterpret_cast<const char*>(0x00900FF0);  // Bikes
                }
                PP_IntelLoadActions(self->intelligence, tree, file);
            }
        } else if (kind == 0 && !(vehicle[0x1BC] & 0x10) && At<uint8_t>(self, 0x16CC) == 0 &&
                   self->intelligence[0x31D] == 0 &&
                   !PP_IsNodePlaying(self->intelligence + 0x258, reinterpret_cast<void*>(0x01B652A8), 1, 1)) {
            PP_IntelLoadActions(self->intelligence, reinterpret_cast<const char*>(0x0090D19C), file);  // CarRide
        }
    }
    if (At<uint8_t>(self, 0x1E25) == 0) {
        PP_TargetingFn4D8A40(self->targeting, 0);
    }
    if (self->pedType == PED_TYPE_PLAYER &&
        PP_IsNodePlaying(self->intelligence + 0x258, reinterpret_cast<void*>(0x01B65260), 1, 1)) {
        PP_Fn48C190(self->unknown3BC, 0);
    }
    if ((PP_frameCounter & 3) == 3) {
        PP_RecordPosition(self);
    }
    PP_Fn6D70B0(PP_20C7E98, self);
    // the feet's collision spheres (+0x15FC, +0x160C) at their bones, in its own space
    if (self->unknownA4 == 0 && At<int32_t>(self, 0xA8) == 0 &&
        (At<int32_t>(self, 0x13E8) != 0 || (self->pedType == PED_TYPE_PLAYER && At<int32_t>(self, 0x131C) != 0))) {
        alignas(4) uint8_t matrix[0x44];
        PP_MatrixCopy(matrix, PP_GetMatrix(self));
        At<ped::CVector>(matrix, 0x30) = At<ped::CVector>(self, 0x1564);
        At<float>(matrix, 0x3C) = 0.0f;  // (uninitialised stack in the game)
        alignas(4) uint8_t inverse[0x44] = {};
        PP_MatrixInvert(matrix, inverse);
        ped::CVector a = BoneLocal(self, inverse, At<int32_t>(self, 0x1D68));
        ped::CVector b = BoneLocal(self, inverse, At<int32_t>(self, 0x1D6C));
        PP_SphereSet(Bytes(self) + 0x15FC, At<float>(self, 0x1D80), &a);
        PP_SphereSet(Bytes(self) + 0x160C, At<float>(self, 0x1D84), &b);
    }
    // a value going to 0 or 1 at +0x1E88 a second
    At<uint8_t>(self, 0x1DE6) = 0;
    float rate = At<float>(self, 0x1E88);
    if (rate != PP_d_0) {
        float s = PP_stepSeconds * PP_timeStep;
        float v = s * rate + At<float>(self, 0x1E84);
        At<float>(self, 0x1E84) = v;
        if (0.0f > At<float>(self, 0x1E88)) {
            if (PP_f_epsilon > v) {
                At<float>(self, 0x1E84) = 0.0f;
                At<float>(self, 0x1E88) = 0.0f;
            }
        } else if (v > PP_d_almostOne) {
            At<float>(self, 0x1E84) = 1.0f;
            At<float>(self, 0x1E88) = 0.0f;
        }
    }
    bool keepHolding = false;
    if (At<int32_t>(self, 0x14EC) != 0) {
        if (PP_Fn485330(Bytes(self) + 0x2F0)) {
            PP_UpdateHoldingHands(self, re::PtrCast<CPed*>(self->unknown2F8));
            keepHolding = true;
        } else if (PP_Fn485320(Bytes(self) + 0x2F0)) {
            PP_UpdateHoldingHands(self->unknown2F4, self);
            keepHolding = true;
        } else {
            At<int32_t>(self, 0x14EC) = 0;
        }
    }
    if (!keepHolding) {
        At<int32_t>(self, 0x153C) = 0;
    }
    uint8_t* e = At<uint8_t*>(self, 0x1DD4);
    At<int32_t>(self, 0x1390) = 0;
    if (e && (e[0x108] & 7) == 6 && At<int32_t>(e, 0x184) == 0) {
        PP_HandleSet(Bytes(self) + 0x1DD4, 0);
        At<int32_t>(self, 0x1538) = 1;
    }
}

REVERSED("CPed::ProcessControl", 0x00481420, RE_BODY(&CPed_ProcessControl), nullptr, nullptr,
         re::Mode::Original);

} // namespace
