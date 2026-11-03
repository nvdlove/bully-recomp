#include "game/Camera.h"
#include "game/Pool.h"

#include <cmath>
#include "reversed.h"
#include "runflags.h"

// FollowCameraController3, part 2 (0x4FD0D0..0x4FE1B0): its constructor, its
// virtuals (reset, look-at, process, the attribute set by name), the nearby-peds
// check, the stick input, and where it looks from (the target ped's head or its
// position raised). Thiscalls are __fastcall wrappers with the unused edx.
//
// Layout used here (FollowCameraController3, 0x538 bytes):
//   +0x08 the look-from position (+0x14 last frame's), +0x20/+0x24 the turn (yaw,
//   pitch) this frame, +0x28 the time step / 50, +0x2C the sub-step, +0x30 turned
//   around, +0x31 the ped's +0x1438, +0x88, +0xBC, +0x13C/+0x154 (vectors),
//   +0x160 the fight part, +0x278 the vehicle part (+0x280 its flag), +0x518 its
//   attribute set (FollowCam3Butes), +0x51C peds close by, +0x520/+0x524,
//   +0x528 in an interior, +0x529..+0x52C, +0x530 when to look for peds again
//   (CTimer ms as a float), +0x534 an attribute set by name is in use.

#define FOLLOWCAM3B_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);   // CTimer's
GAME_VAR(float, timeStep, 0x00C1A9A4);      // CTimer::ms_fTimeStep
GAME_VAR(int32_t, currArea, 0x00BD1008);    // CGame::currArea
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);
GAME_VAR(uint8_t, g_A14925, 0x00A14925);
GAME_VAR(uint8_t, lockButtonInputs, 0x00C1AF8C);
GAME_VAR(uint8_t, g_C1AF9B, 0x00C1AF9B);
GAME_VAR(uint8_t, widescreen, 0x00C1870E);
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);   // CWorld::ms_nCurrentScanCode
GAME_VAR(uint8_t, g_C1AE8A, 0x00C1AE8A);           // set around the sweep below
GAME_VAR(float, g_A457D8, 0x00A457D8);
GAME_VAR(float, g_A457DC, 0x00A457DC);
// the sweep's hit (a colpoint: +0 the position)
GAME_VAR(CVector, sweepHit, 0x00C216C0);
inline uint8_t* const sweepHitPoint = reinterpret_cast<uint8_t*>(0x00C216C0);
inline void* const userInputManager = reinterpret_cast<void*>(0x00A45A30);
// the butes names (hashes) and root the attribute set is looked up by
inline const uint32_t* const butesName_1B65388 = reinterpret_cast<const uint32_t*>(0x01B65388);
inline const uint32_t* const butesRoot_1B65384 = reinterpret_cast<const uint32_t*>(0x01B65384);

constexpr double kPi = 3.1415927410125732;   // (float pi, as a double)

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, re::Ptr32<void>* ref);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVector* out);
GAME_FN(CVector*, __thiscall, Placeable_GetPosition, 0x00415460, void* placeable);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const CVector* v);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, Min, 0x00414470, float a, float b);
GAME_FN(float, __cdecl, MaxFloat, 0x00414490, float a, float b);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(cam::NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void*, __cdecl, GetRwObjectFrame, 0x005EE1A0, void* rwObject);
GAME_FN(uint8_t*, __cdecl, GetHierarchy, 0x006C7710, void* frame);
// peds
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_Unknown496C70, 0x00496C70, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_Unknown473FB0, 0x00473FB0, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_Unknown473E10, 0x00473E10, void* ped);
// input
GAME_FN(void*, __cdecl, Input_Unknown738070, 0x00738070);
GAME_FN(uint8_t, __cdecl, Input_Unknown738500, 0x00738500, void* arg);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int pad, int);
GAME_FN(uint8_t, __thiscall, UserInputManager_IsEnabled, 0x0050DF50, void* self);
// the butes (names are 4-byte hashes; a path is {count, names})
GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(void, __thiscall, Name_Construct, 0x005FA910, void* self, uintptr_t value);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* self, int count, const void* names);
GAME_FN(void*, __cdecl, Butes_Find, 0x0072AEA0, const void* path, const void* root);
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* array, size_t size, int count, void* dtor);
GAME_FN(void, __thiscall, Track_Construct, 0x0061A880, void* self);   // (the butes' base)
GAME_FN(void, __cdecl, AddLoadCallback, 0x0072B870, void* fn, void* object);
// the sweep (a swept sphere, 0x48 bytes on the stack)
GAME_FN(void, __thiscall, Sweep_Construct, 0x00563060, void* self, const CVector* from, const CVector* to, float radius, int flags, void* hit, void* filter, int a, int b, uintptr_t c);
GAME_FN(int32_t, __thiscall, Sweep_Test, 0x00564080, void* self);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
// the camera
GAME_FN(float, __thiscall, Camera_Unknown4F1580, 0x004F1580, CCamera* self);
GAME_FN(float, __thiscall, Camera_Unknown4F2A90, 0x004F2A90, CCamera* self);
GAME_FN(void, __thiscall, Camera_SetEntity1B4, 0x004FD0D0, CCamera* self, void* entity);
GAME_FN(void, __thiscall, CameraLimits_Get, 0x004F41A0, void* self, float* a, float* b, float* low, float* high, CVector* centre);
// the follow camera's other parts
GAME_FN(void*, __thiscall, FollowCameraBase_Construct, 0x00504B50, void* self);
GAME_FN(void, __thiscall, FollowCameraBase_Process, 0x00504D90, void* self);   // FollowCameraBase vtable[7]
GAME_FN(void, __thiscall, FollowCameraBase_LookAt, 0x005048E0, void* self, CVector from, CVector at);
GAME_FN(void, __thiscall, FollowCameraBase_LookAt2, 0x00504A00, void* self, CVector from, CVector at, int value);
GAME_FN(void, __thiscall, FollowCameraVehicle_Update, 0x004FE020, void* self);
GAME_FN(void*, __thiscall, FollowCameraVehicle_GetDrivenD4, 0x004FE070, void* self);
GAME_FN(void, __thiscall, FollowCameraVehicle_Reset, 0x004FDFC0, void* self);
GAME_FN(void, __thiscall, FollowCameraFight_Clear, 0x00500630, void* self);
GAME_FN(void, __thiscall, FC3_Unknown4FB4D0, 0x004FB4D0, void* self);
GAME_FN(void, __thiscall, FC3_Unknown4FB5A0, 0x004FB5A0, void* self, float value);
GAME_FN(void, __cdecl, FC3_Unknown4FB710, 0x004FB710, void* a, void* b, void* c, float d, float e);
GAME_FN(void, __thiscall, FC3_Unknown4FBC50, 0x004FBC50, void* self, float value);
GAME_FN(void, __thiscall, FC3_Unknown4FBCF0, 0x004FBCF0, void* self, CVector a, CVector b);
GAME_FN(void, __thiscall, FC3_Unknown4FBD70, 0x004FBD70, void* self);
GAME_FN(void, __thiscall, FC3_Unknown4FBDD0, 0x004FBDD0, void* self);
GAME_FN(void, __thiscall, FC3_Unknown4FC0C0, 0x004FC0C0, void* self);
GAME_FN(void, __thiscall, FC3_Unknown4FC770, 0x004FC770, void* self, void* ped, float* a, float* b, float* c);
GAME_FN(void, __thiscall, FC3_Unknown4FCA30, 0x004FCA30, void* self);
// own functions, through the game's address
GAME_FN(void, __thiscall, FC3_FindPedsClose, 0x004FD590, void* self);
GAME_FN(void, __thiscall, FC3_ReadInput, 0x004FD8C0, void* self);
GAME_FN(CVector*, __thiscall, FC3_GetLookFrom, 0x004FE1B0, void* self, CVector* out);

using re::At;

uint8_t* TargetPed() {
    return static_cast<uint8_t*>(TheCamera.targetPed);
}
// TheCamera+0x1B0: the second target (an entity)
uint8_t* SecondTarget() {
    return At<uint8_t*>(&TheCamera, 0x1B0);
}
// TheCamera+0x1B4: a ped close by (a registered reference)
re::Ptr32<void>& CloseEntity() {
    return At<void*>(&TheCamera, 0x1B4);
}

int EntityType(void* entity) {
    return At<uint8_t>(entity, 0x108) & 7;
}

// CPlaceable::GetPosition, inlined: the matrix's position, or the placeable's own
const CVector* Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : &At<CVector>(entity, 4);
}

cam::NiTransform* CameraTransform() {
    return GetLocalTransform(GetCameraNode(TheCamera.camera));
}

void* Butes(void* self) {
    return At<void*>(self, 0x518);
}

float TimeAsFloat(uint32_t ms) {
    return float(double(ms));
}

} // namespace

namespace {

// 0x4FD0D0 (thiscall on TheCamera): the ped close by (+0x1B4), a registered reference
void __fastcall Camera_SetEntity1B4Ours(CCamera* self, re::Edx, void* entity) {
    re::Ptr32<void>* ref = &At<void*>(self, 0x1B4);
    void* old = *ref;
    if (entity == old) {
        return;
    }
    if (old) {
        CleanUpOldReference(old, ref);
    }
    *ref = entity;
    if (entity) {
        RegisterReference(entity, ref);
    }
}
FOLLOWCAM3B_REVERSED("CCamera::SetCloseEntity", 0x004FD0D0, RE_BODY(&Camera_SetEntity1B4Ours), nullptr, nullptr, re::Mode::Original);

// 0x4FD110: the constructor (the base's, then its attribute set)
void* __fastcall FollowCameraController3_Construct(void* self, re::Edx) {
    FollowCameraBase_Construct(self);
    At<uint32_t>(self, 0) = 0x00914598;
    At<int32_t>(self, 0x520) = 0;
    At<int32_t>(self, 0x524) = 0;
    void* butes = Malloc(0xF8);
    if (butes) {
        Track_Construct(butes);
        At<uint32_t>(butes, 0) = 0x00914580;   // FollowCam3Butes
    } else {
        butes = nullptr;
    }
    At<void*>(self, 0x518) = butes;
    AddLoadCallback(reinterpret_cast<void*>(0x004FB6F0), self);
    return self;
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::FollowCameraController3", 0x004FD110, RE_BODY(&FollowCameraController3_Construct), nullptr, nullptr, re::Mode::Original);

// 0x4FD1C0 (vtable[1])
void __fastcall FollowCameraController3_Vf1(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 0x116) = 0;
    At<uint8_t>(self, 0x534) = 0;
    At<uint8_t>(self, 0x52A) = 0;
    At<uint8_t>(self, 0x52B) = 0;
    At<uint8_t>(self, 0x529) = 0;
    At<uint8_t>(self, 0x52C) = 0;
    At<uint8_t>(self, 0x528) = IsAreaInterior(currArea);
    FollowCameraVehicle_Update(self + 0x278);
    FC3_Unknown4FC0C0(self);
    FollowCameraFight_Clear(self + 0x160);
    FollowCameraVehicle_Reset(self + 0x278);
    FC3_Unknown4FB4D0(self);
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::vf1", 0x004FD1C0, RE_BODY(&FollowCameraController3_Vf1), nullptr, nullptr, re::Mode::Original);

// the look-at virtuals' shared end
void FinishLookAt(uint8_t* self) {
    At<float>(self, 0x530) = TimeAsFloat(timeInMs);
    // (the remaster's seamless doors, runflags.h: just through one, the fov eases to the new area's
    // as it does when the attribute set changes, not snapped to it with the view)
    if (!re::SeamlessCameraFree()) {
        FC3_Unknown4FB5A0(self, 1.0f);
    }
    At<float>(self, 0x88) = 0.0f;
}

// 0x4FD230 (vtable[3], reset): behind the target ped (in front of it when it's
// +0x1474/+0x1470 with +0x1488)
void __fastcall FollowCameraController3_Reset(uint8_t* self, re::Edx) {
    if (!TheCamera.targetPed) {
        return;
    }
    FollowCameraVehicle_Update(self + 0x278);
    FC3_Unknown4FC0C0(self);
    At<int32_t>(self, 0x2C) = 0;
    uint8_t* ped = TargetPed();
    CVector forward;
    Entity_GetForward(ped, &forward);
    if (ped && (At<int32_t>(ped, 0x1474) != 0 || At<int32_t>(ped, 0x1470) != 0) && At<int32_t>(ped, 0x1488) != 0) {
        forward.x = float(double(forward.x) * -1.0);
        forward.y = float(double(forward.y) * -1.0);
        forward.z = float(double(forward.z) * -1.0);
    }
    CVector from;
    FC3_GetLookFrom(self, &from);
    FollowCameraBase_LookAt(self, from, forward);
    FinishLookAt(self);
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::vf3", 0x004FD230, RE_BODY(&FollowCameraController3_Reset), nullptr, nullptr, re::Mode::Original);

// 0x4FD330 (vtable[5])
void __fastcall FollowCameraController3_Vf5(uint8_t* self, re::Edx, CVector at) {
    if (!TheCamera.targetPed) {
        return;
    }
    FollowCameraVehicle_Update(self + 0x278);
    FC3_Unknown4FC0C0(self);
    At<int32_t>(self, 0x2C) = 0;
    CVector from;
    FC3_GetLookFrom(self, &from);
    FollowCameraBase_LookAt(self, from, at);
    FinishLookAt(self);
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::vf5", 0x004FD330, RE_BODY(&FollowCameraController3_Vf5), nullptr, nullptr, re::Mode::Original);

// 0x4FD3E0 (vtable[6])
void __fastcall FollowCameraController3_Vf6(uint8_t* self, re::Edx, CVector at, int value) {
    if (!TheCamera.targetPed) {
        return;
    }
    FollowCameraVehicle_Update(self + 0x278);
    FC3_Unknown4FC0C0(self);
    At<int32_t>(self, 0x2C) = 0;
    CVector from;
    FC3_GetLookFrom(self, &from);
    FollowCameraBase_LookAt2(self, from, at, value);
    FinishLookAt(self);
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::vf6", 0x004FD3E0, RE_BODY(&FollowCameraController3_Vf6), nullptr, nullptr, re::Mode::Original);

// 0x4FD490 (vtable[8]): an attribute set by name (0: the default); 0 when not found
uint8_t __fastcall FollowCameraController3_Vf8(uint8_t* self, re::Edx, int name) {
    void* found = nullptr;
    if (name != 0) {
        uint32_t names[2];
        uint32_t path[2];
        Name_Copy(&names[0], butesName_1B65388);
        Name_Construct(&names[1], name);
        Path_Build(path, 2, names);
        found = Butes_Find(path, butesRoot_1B65384);
        if (!found) {
            At<uint8_t>(self, 0x534) = 0;
            FollowCameraVehicle_Update(self + 0x278);
            FC3_Unknown4FC0C0(self);
            DestructArray(names, 4, 2, reinterpret_cast<void*>(0x0049A650));
            return 0;
        }
        FC3_Unknown4FB710(Butes(self), Butes(self), found, 0.0f, 1.0f);
        DestructArray(names, 4, 2, reinterpret_cast<void*>(0x0049A650));
    }
    At<uint8_t>(self, 0x534) = found != nullptr;
    FollowCameraVehicle_Update(self + 0x278);
    FC3_Unknown4FC0C0(self);
    FC3_Unknown4FBC50(self, 1.0f);
    return 1;
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::vf8", 0x004FD490, RE_BODY(&FollowCameraController3_Vf8), nullptr, nullptr, re::Mode::Original);

// 0x4FD590: every 300 ms, whether peds going for the target ped are close by (+0x51C;
// within 20), and, with butes +0xC3 and the camera within 1.5 of the ped, the
// closest of them within 2.5 as TheCamera's close entity. (The loop stops at the
// first such ped when the camera isn't that close.)
void __fastcall FollowCameraController3_FindPedsClose(uint8_t* self, re::Edx) {
    if (double(At<float>(self, 0x530)) > double(timeInMs)) {
        return;
    }
    uint8_t* ped = TargetPed();
    if (!ped || EntityType(ped) != 3) {
        return;
    }
    At<float>(self, 0x530) = TimeAsFloat(timeInMs + 300);
    At<uint8_t>(self, 0x51C) = 0;
    if (CloseEntity()) {
        CleanUpOldReference(CloseEntity(), &CloseEntity());
        CloseEntity() = nullptr;
    }
    float best = 100000.0f;
    uint8_t cameraClose = 0;
    if (At<uint8_t>(Butes(self), 0xC3)) {
        cam::NiTransform* transform = CameraTransform();
        CVector d;
        VectorSub(&d, Position(ped), &transform->translate);
        float lengthSq = float(double(d.x) * d.x + double(d.y) * d.y);
        if (lengthSq < 2.25f) {
            cameraClose = 1;
        }
    }
    CPool<uint8_t>* pool = pedPool;
    int size = pool->size;
    for (int slot = 0; slot < size; slot++) {
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* other = pool->objects + pool->elemSize * slot;
        if (!other || other == ped) {
            continue;
        }
        uint8_t* target = Ped_GetTarget(other);
        if (target && EntityType(target) == 3) {
            uint8_t* targetPed = Ped_GetTarget(other);
            if (targetPed && EntityType(targetPed) != 3) {
                targetPed = nullptr;
            }
            if (targetPed == ped && Ped_Unknown473FB0(other)) {
                const CVector* from = Position(other);
                CVector d;
                VectorSub(&d, from, Placeable_GetPosition(ped));
                float distanceSq = Vector_MagnitudeSqr(&d);
                if (distanceSq < 400.0) {
                    At<uint8_t>(self, 0x51C) = 1;
                }
                if (!cameraClose) {
                    break;
                }
                if (distanceSq < 6.25 && distanceSq < best) {
                    Camera_SetEntity1B4(&TheCamera, other);
                    best = distanceSq;
                }
            }
        }
        uint8_t* other2 = Ped_Unknown496C70(other);
        if (other2 && other2 == ped) {
            const CVector* from = Position(other);
            CVector d;
            VectorSub(&d, from, Placeable_GetPosition(ped));
            float distanceSq = float((double(d.x) * d.x + double(d.y) * d.y) + double(d.z) * d.z);
            if (distanceSq < 400.0) {
                At<uint8_t>(self, 0x51C) = 1;
            }
            if (!cameraClose) {
                break;
            }
            if (distanceSq < 6.25 && distanceSq < best) {
                Camera_SetEntity1B4(&TheCamera, other);
                best = distanceSq;
            }
        }
    }
    if (At<int32_t>(ped, 0x2F4) != 0) {
        At<uint8_t>(self, 0x51C) = 1;
    }
    if (At<int32_t>(ped, 0x2F8) != 0) {
        At<uint8_t>(self, 0x51C) = 1;
    }
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::FindPedsClose", 0x004FD590, RE_BODY(&FollowCameraController3_FindPedsClose), nullptr, nullptr, re::Mode::Original);

// 0x4FD8C0: this frame's turn from the stick (+0x20 yaw, +0x24 pitch), turned
// around (+0x30) on the look-behind button, and held inside the camera's limits
// while the ped is +0x148C/+0x16C8 (the limits' push-back is multiplied by 0 and -0:
// a tuning value zeroed). Then the look-from position.
void __fastcall FollowCameraController3_ReadInput(uint8_t* self, re::Edx) {
    At<float>(self, 0x20) = 0.0f;
    At<uint8_t>(self, 0x30) = 0;
    At<float>(self, 0x24) = 0.0f;
    At<uint8_t>(self, 0x31) = 0;
    uint8_t* target = TargetPed();
    uint8_t* ped = nullptr;
    if (EntityType(target) == 3) {
        ped = target;
        if (At<int32_t>(ped, 0x1438) != 0) {
            At<uint8_t>(self, 0x31) = 1;
        }
    }
    // (ped is read without a check here: the target is a ped whenever this runs)
    if (g_A14925 && !Input_Unknown738500(Input_Unknown738070()) && At<int32_t>(ped, 0x1310) == 0xD &&
        Ped_Unknown473E10(ped)) {
        // (no turning)
    } else if (At<int32_t>(&TheCamera, 0xA4) == 0 && UserInputManager_IsEnabled(userInputManager)) {
        float step = timeStep;
        float scaled = float(Camera_Unknown4F1580(&TheCamera) * step);
        At<float>(self, 0x20) = float(double(At<float>(Butes(self), 0x1C)) * scaled);
        uint8_t* ped2 = TargetPed();
        float unusedA;
        float unusedC;
        FC3_Unknown4FC770(self, ped2, &unusedA, &scaled, &unusedC);
        At<float>(self, 0x20) = float((double(scaled) * double(0.6f) + 1.0) * At<float>(self, 0x20));
        step = timeStep;
        float pitch = float(-Camera_Unknown4F2A90(&TheCamera) * step);
        At<float>(self, 0x24) = float(double(At<float>(Butes(self), 0x20)) * pitch);

        bool turnAround = false;
        bool check = true;
        if (lockButtonInputs != 0 && g_C1AF9B == 0) {
            check = false;   // (to the limits)
        }
        if (check) {
            float button = Pad_GetValue(0xE, 0);
            bool viaVehicle = true;
            if (!(button == 0.0f)) {
                if ((At<int32_t>(ped2, 0x148C) == 0 || At<int32_t>(ped2, 0x16C8) == 0) && SecondTarget() == nullptr) {
                    turnAround = true;
                    viaVehicle = false;
                }
            }
            if (viaVehicle && At<uint8_t>(self, 0x280) != 0 &&
                At<uint8_t>(FollowCameraVehicle_GetDrivenD4(self + 0x278), 0x24) != 0) {
                turnAround = true;
            }
        }
        if (turnAround) {
            At<float>(self, 0x24) = 0.0f;
            At<uint8_t>(self, 0x30) = 1;
            At<float>(self, 0x20) = float(double(At<float>(self, 0x20)) * -1.0);
        } else if (ped2 && At<int32_t>(ped2, 0x148C) != 0 && At<int32_t>(ped2, 0x16C8) != 0 &&
                   At<uint8_t>(&TheCamera, 0x224) != 0) {
            float absTurn = std::fabs(At<float>(self, 0x20));
            if (absTurn > 1e-5f) {
                CVector centre{0.0f, 0.0f, 0.0f};
                float a;
                float b;
                float low;
                float high;
                CameraLimits_Get(&At<uint8_t>(&TheCamera, 0x208), &a, &b, &low, &high, &centre);
                cam::NiTransform* transform = CameraTransform();
                CVector look{transform->rotate[2], transform->rotate[5], transform->rotate[8]};
                float heading = VecHeading(&look);
                float wanted = float(double(At<float>(self, 0x20)) * kPi / 180.0 + heading);
                float centreHeading = VecHeading(&centre);
                float diff = AngleDiff(centreHeading, wanted);
                if (low > diff && 0.0f > At<float>(self, 0x20)) {
                    float stepNow = timeStep;
                    float speedSq = float(double(At<float>(ped2, 0x124)) * At<float>(ped2, 0x124) +
                                          double(At<float>(ped2, 0x128)) * At<float>(ped2, 0x128));
                    double lowD = low;
                    float limit = float((lowD - AngleDiff(centreHeading, heading)) * 180.0 / kPi);
                    float pushBack = float(double(stepNow) * -0.0 * speedSq);
                    At<float>(self, 0x20) = Min(pushBack, limit);
                } else if (high < diff && 0.0f < At<float>(self, 0x20)) {
                    float stepNow = timeStep;
                    float speedSq = float(double(At<float>(ped2, 0x124)) * At<float>(ped2, 0x124) +
                                          double(At<float>(ped2, 0x128)) * At<float>(ped2, 0x128));
                    double highD = high;
                    float limit = float((highD - AngleDiff(centreHeading, heading)) * 180.0 / kPi);
                    float pushBack = float(double(stepNow) * 0.0 * speedSq);
                    At<float>(self, 0x20) = MaxFloat(pushBack, limit);
                }
            }
        }
    }
    At<float>(self, 0x28) = float(double(timeStep) / 50.0);
    CVector from;
    FC3_GetLookFrom(self, &from);
    At<CVector>(self, 8) = from;
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::ReadInput", 0x004FD8C0, RE_BODY(&FollowCameraController3_ReadInput), nullptr, nullptr, re::Mode::Original);

// 0x4FDCF0 (vtable[7], process): the base's process run three times over a third of
// the frame's movement and turn each, then the collision and the moves' deltas
void __fastcall FollowCameraController3_Process(uint8_t* self, re::Edx) {
    FollowCameraVehicle_Update(self + 0x278);
    FC3_Unknown4FC0C0(self);
    FC3_FindPedsClose(self);
    FC3_Unknown4FBC50(self, 0.0f);
    FC3_ReadInput(self);
    FC3_Unknown4FBD70(self);
    FC3_Unknown4FBDD0(self);
    CVector old13C = At<CVector>(self, 0x13C);
    CVector old154 = At<CVector>(self, 0x154);
    CVector* from = &At<CVector>(self, 8);
    At<CVector>(self, 0x14) = *from;
    CVector start = At<CVector>(self, 0xBC);
    // (+0x20, +0x24, +0x28 as they were: the game copies +8..+0x33)
    float savedYaw = At<float>(self, 0x20);
    float savedPitch = At<float>(self, 0x24);
    float savedStep = At<float>(self, 0x28);
    CVector move;
    VectorSub(&move, from, &start);
    float yawThird = float(double(savedYaw) / 3.0);
    float pitchThird = float(double(savedPitch) / 3.0);
    float stepThird = float(double(savedStep) / 3.0);
    for (int i = 0; i < 3; i++) {
        At<float>(self, 0x20) = yawThird;
        At<float>(self, 0x24) = pitchThird;
        At<int32_t>(self, 0x2C) = i;
        CVector part{float(double(move.x) * double(0.33333334f)), float(double(move.y) * double(0.33333334f)),
                     float(double(move.z) * double(0.33333334f))};
        CVector sum;
        CVector* result = VectorAdd(&sum, &At<CVector>(self, 0xBC), &part);
        *from = *result;
        At<float>(self, 0x28) = stepThird;
        FollowCameraBase_Process(self);
    }
    At<float>(self, 0x28) = savedStep;
    At<int32_t>(self, 0x2C) = 0;
    FC3_Unknown4FCA30(self);
    CVector d13C;
    CVector* delta13C = VectorSub(&d13C, &At<CVector>(self, 0x13C), &old13C);
    CVector d154;
    CVector* delta154 = VectorSub(&d154, &At<CVector>(self, 0x154), &old154);
    FC3_Unknown4FBCF0(self, *delta154, *delta13C);
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::vf7", 0x004FDCF0, RE_BODY(&FollowCameraController3_Process), nullptr, nullptr, re::Mode::Original);

// 0x4FDF20 (stdcall): `value` moved towards `limit` by `t` of its way past `start`,
// then held at `limit`
float __stdcall FollowCamera_EaseToLimit(float value, float limit, float start, float t) {
    if (limit < start) {
        if (value < start) {
            float step = float((double(start) - value) * t / (double(start) - limit));
            value = float(double(step) + value);
        }
        if (!(limit > value)) {
            return value;
        }
        return limit;
    }
    if (start < value) {
        float step = float((double(value) - start) * t / (double(limit) - start));
        value = float(double(value) - step);
    }
    if (limit < value) {
        return limit;
    }
    return value;
}
FOLLOWCAM3B_REVERSED("FollowCamera::EaseToLimit", 0x004FDF20, &FollowCamera_EaseToLimit, nullptr, nullptr, re::Mode::Original);

// 0x4FDFC0 (on the vehicle part, +0x278): its angles' defaults, then the driven
// controller's vtable[1]
void __fastcall FollowCameraVehicle_ResetOurs(uint8_t* self, re::Edx) {
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 180.0f;
    At<float>(self, 0x28) = 360.0f;
    At<float>(self, 0x2C) = 0.1f;
    At<float>(self, 0x30) = 0.2f;
    At<float>(self, 0x1C) = 0.0f;
    void* driven = At<void*>(self, 4);
    reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(driven))[1]))(driven);
}
FOLLOWCAM3B_REVERSED("FollowCameraVehicle::Reset", 0x004FDFC0, RE_BODY(&FollowCameraVehicle_ResetOurs), nullptr, nullptr, re::Mode::Original);

// 0x4FE110: the attribute set's distance for what the target ped is doing
float __fastcall FollowCameraController3_GetDistance(uint8_t* self, re::Edx) {
    uint8_t* ped = TargetPed();
    if (EntityType(ped) != 3) {
        return At<float>(Butes(self), 0x60);
    }
    bool busy = At<int32_t>(ped, 0x2F4) != 0 || At<int32_t>(ped, 0x2F8) != 0;
    if (At<int32_t>(ped, 0x1320) != 0) {
        return At<float>(Butes(self), 0x68);
    }
    if (At<int32_t>(ped, 0x131C) != 0) {
        return At<float>(Butes(self), 0x64);
    }
    if (busy) {
        return At<float>(Butes(self), 0x6C);
    }
    return At<float>(Butes(self), 0x60);
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::GetDistance4FE110", 0x004FE110, RE_BODY(&FollowCameraController3_GetDistance), nullptr, nullptr, re::Mode::Original);

// the head's position (the hierarchy's +0x8C), or the ped's raised by 0.4 when +0xA4
CVector HeadPosition(uint8_t* ped) {
    uint8_t* hierarchy = GetHierarchy(GetRwObjectFrame(At<void*>(ped, 0x18)));
    CVector head = At<CVector>(hierarchy, 0x8C);
    if (At<int32_t>(ped, 0xA4) != 0) {
        head = *Position(ped);
        head.z = float(double(head.z) + double(0.4f));
    }
    return head;
}

// 0x4FE1B0: where the camera looks from: the target ped's position, raised by the
// attribute set (to the head when aiming or +0x13CC/+0x16CC, unless faded out; with
// a sweep up and down for the ground otherwise), lowered when crouching (+0x1320),
// +0x131C, or in 16:9
CVector* __fastcall FollowCameraController3_GetLookFrom(uint8_t* self, re::Edx, CVector* out) {
    *out = *Position(TargetPed());
    bool fadedOut = double(TheCamera.fadeAlpha) > 250.0;
    uint8_t* ped = TargetPed();
    if (EntityType(ped) != 3) {
        return out;
    }
    if (At<int32_t>(ped, 0x13E0) != 0 && !fadedOut) {
        CVector head = HeadPosition(ped);
        float raise = At<float>(Butes(self), 0x30);
        float zero = float(0.0 * raise);
        CVector offset{zero, zero, raise};
        CVector sum;
        *out = *VectorAdd(&sum, &head, &offset);
    } else if ((At<int32_t>(ped, 0x13CC) != 0 || At<uint8_t>(ped, 0x16CC) != 0) && !fadedOut) {
        CVector head = HeadPosition(ped);
        out->z = At<float>(Butes(self), 0x30) + head.z;
    } else if (At<int32_t>(ped, 0x13B0) != 0) {
        out->z = At<float>(Butes(self), 0x2C) + out->z;
    } else {
        g_C1AE8A = 1;
        float radius = At<float>(Butes(self), 0x18);
        CVector up{float(0.0 * 0.5), float(0.0 * 0.5), 0.5f};
        CVector top;
        VectorAdd(&top, out, &up);
        CVector down{float(0.0 * 1.5), float(0.0 * 1.5), 1.5f};
        CVector bottom;
        VectorSub(&bottom, out, &down);
        alignas(4) uint8_t sweep[0x48];
        Sweep_Construct(sweep, &top, &bottom, radius, 0x8B, sweepHitPoint, reinterpret_cast<void*>(0x0045EE90), 0, 0, 0);
        At<uint16_t>(TargetPed(), 0x10A) = uint16_t(currentScanCode + 1);
        uint8_t* again = TargetPed();
        if (EntityType(again) == 3 && At<uint8_t*>(again, 0x1554)) {
            At<uint16_t>(At<uint8_t*>(again, 0x1554), 0x10A) = uint16_t(currentScanCode + 1);
        }
        if (Sweep_Test(sweep) != 0) {
            float hitZ = sweepHit.z;
            if (hitZ < out->z) {
                out->z = hitZ;
            }
        } else {
            out->z = float(double(out->z) - 1.5);
        }
        g_C1AE8A = 0;
        out->z = At<float>(Butes(self), 0x2C) + out->z;
        EmptyThiscall(sweep);
    }
    if (At<int32_t>(ped, 0x1320) != 0) {
        out->z = out->z - g_A457DC;
    } else if (At<int32_t>(ped, 0x131C) != 0) {
        out->z = out->z - g_A457D8;
    } else if (widescreen == 1 && At<uint8_t>(self, 0x280) == 0 && At<uint8_t>(ped, 0x16CC) == 0 &&
               At<uint8_t>(self, 0x51C) == 0) {
        out->z = float(double(out->z) - double(0.3f));
    }
    return out;
}
FOLLOWCAM3B_REVERSED("FollowCameraController3::GetLookFrom", 0x004FE1B0, RE_BODY(&FollowCameraController3_GetLookFrom), nullptr, nullptr, re::Mode::Original);

} // namespace
