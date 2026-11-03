#include "game/Camera.h"

#include <cmath>
#include "reversed.h"

// FollowCameraController3 (the third-person follow camera), part 1: its value easing,
// the attribute sets ("butes") it picks and blends between (interior/exterior,
// vehicle, the target's states), the fov, the camera height over the target(s), the
// lock-on marker, the swing and two effect setters of FollowCameraBase.
// Offsets used here (FollowCameraBase and FollowCameraController3 share a layout):
//   +0x28 zoom (fov blend), +0x2C, +0x30 a flag, +0x48 height, +0x68 the swing
//   (0x24), +0xD0 a vector, +0x115/+0x116 flip flags, +0x13C the camera position,
//   +0x154 the point looked at, +0x160 a FollowCameraFight, +0x25C a blend,
//   +0x274 the height limit, +0x278 a FollowCameraVehicle (+0x280 its "in a
//   vehicle"), +0x2B0/+0x2C0 two effects, +0x518 the attribute set in use,
//   +0x51C, +0x520/+0x524 the sets blended to, +0x528.. the state that picked them,
//   +0x530 a time, +0x534 (blending off), +0x538 the lock-on marker's attribute set.

#define FOLLOWCAM3_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);           // CTimer's
GAME_VAR(float, timeStep, 0x00C1A9A4);              // CTimer::ms_fTimeStep
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);     // +0xFC the lock-on marker's name
GAME_VAR(void*, playerPed, 0x00C1AEA8);             // FindPlayerPed
GAME_VAR(int32_t, currArea, 0x00BD1008);            // CGame::currArea
GAME_VAR(CVector, cameraVector18C, 0x00C3CDF4);     // TheCamera+0x18C
GAME_VAR(int32_t, camera1B4, 0x00C3CE1C);           // TheCamera+0x1B4
GAME_VAR(uint8_t, fovLocked, 0x00C3CF82);           // TheCamera+0x31A (the follow camera leaves the fov alone)
// the follow camera's tunables
GAME_VAR(float, heightRange, 0x00A45750);
GAME_VAR(float, heightOffset, 0x00A45754);
GAME_VAR(float, offsetScale, 0x00A45758);
GAME_VAR(float, swingEase, 0x00A4575C);
GAME_VAR(float, swingAngleScale, 0x00A45760);
GAME_VAR(float, swingSpeed, 0x00A45764);
GAME_VAR(float, swingInputScale, 0x00C3CFD0);
GAME_VAR(int32_t, swingFrames, 0x00C3CFD4);
// the attribute set names
inline void* const kFollowRoot = reinterpret_cast<void*>(0x01B65388);
inline void* const kFollowType = reinterpret_cast<void*>(0x01B65384);
inline void* const kLockOnRoot = reinterpret_cast<void*>(0x01B6531C);
inline void* const kLockOnName = reinterpret_cast<void*>(0x01B6535C);
inline void* const kLockOnType = reinterpret_cast<void*>(0x01B65360);
inline void* const kNameVehicle = reinterpret_cast<void*>(0x01B653CC);
inline void* const kName53D0 = reinterpret_cast<void*>(0x01B653D0);  // the target's +0x16CC
inline void* const kName537C = reinterpret_cast<void*>(0x01B6537C);
inline void* const kName5380 = reinterpret_cast<void*>(0x01B65380);
inline void* const kName5400 = reinterpret_cast<void*>(0x01B65400);
inline void* const kName540C = reinterpret_cast<void*>(0x01B6540C);
inline void* const kName5414 = reinterpret_cast<void*>(0x01B65414);
inline void* const kName5418 = reinterpret_cast<void*>(0x01B65418);
inline void* const kName541C = reinterpret_cast<void*>(0x01B6541C);
inline void* const kName542C = reinterpret_cast<void*>(0x01B6542C);
inline void* const lockOnMarkerObject = reinterpret_cast<void*>(0x00A45500);
inline void* const clothing = reinterpret_cast<void*>(0x020C5F70);

GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(void, __thiscall, Name_Construct, 0x005FA910, void* self, uintptr_t value);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* self, int count, const void* names);
GAME_FN(void*, __cdecl, Config_Find, 0x0072AEA0, const void* path, const void* type);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);   // (the names' destructor)
GAME_FN(void, __stdcall, VectorDestruct, 0x004017C0, void* array, size_t size, int count, void* destructor);
GAME_FN(uint8_t, __cdecl, Area_430320, 0x00430320, int area);
// math
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, GetAngleOfVector2D, 0x00433190, const CVector* v);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
// (0x00414C80 stores 16 bytes through `out`: called through re::CallVec16)
static inline void TransformVector(CVector* out, const void* matrix, const CVector* v) { re::CallVec16<0x00414C80, void>(out, matrix, v); }
// entities
GAME_FN(void*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(void*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(void*, __cdecl, Fn_5EE1A0, 0x005EE1A0, void* object);
GAME_FN(uint8_t*, __cdecl, Fn_6C7710, 0x006C7710, void* object);
GAME_FN(uint8_t, __thiscall, Entity_473D60, 0x00473D60, void* entity);
GAME_FN(void, __thiscall, Ped_SetAlpha, 0x004732A0, void* ped, int alpha, int b);
GAME_FN(uint8_t, __thiscall, Weapon_GetInfoByte68, 0x004D96D0, void* weapon);
GAME_FN(void*, __thiscall, Targeting_4D7780, 0x004D7780, void* targeting);
GAME_FN(CVector*, __thiscall, Targeting_GetTargetPos, 0x004D6000, void* targeting, CVector* out, void* entity, int);
GAME_FN(float, __thiscall, Targeting_GetLockProgress, 0x004D5E20, void* targeting);
GAME_FN(float, __cdecl, Fn_738840, 0x00738840, int a, int b);   // (an input value by index)
GAME_FN(int, __thiscall, Fn_856850, 0x00856850, void* object);
GAME_FN(float, __thiscall, Clothing_6CAAE0, 0x006CAAE0, void* clothing);
// the lock-on marker
GAME_FN(void, __cdecl, LockOn_Clear, 0x004DB5F0);
GAME_FN(void, __cdecl, LockOn_4DB290, 0x004DB290, int value);
GAME_FN(void, __cdecl, LockOn_Draw_x64, 0x004DB6E0, float, float, float, float scale, int a, int b, int c, int d);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void LockOn_Draw(CVector a0, float a1, int a2, int a3, int a4, int a5) {
    return LockOn_Draw_x64(a0.x, a0.y, a0.z, a1, a2, a3, a4, a5);
}
// the camera
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(cam::NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __thiscall, Camera_Unknown4F2C60, 0x004F2C60, CCamera* camera, void* a, void* b);
GAME_FN(void, __thiscall, Camera_SetOffset, 0x004F0920, CCamera* camera, const CVector* offset);
GAME_FN(uint8_t*, __thiscall, FollowCameraVehicle_GetDrivenD4, 0x004FE070, void* self);
GAME_FN(CVector*, __thiscall, FollowCameraController3_Get90, 0x004FDEF0, void* self, CVector* out);
GAME_FN(void, __thiscall, FollowCameraController3_Call5044A0, 0x00504A00, void* self, CVector a, CVector b, int c);
// own functions, called through the game's address
GAME_FN(void, __cdecl, Game_BlendButes, 0x004FB710, void* out, const void* a, const void* b, float wa, float wb);
GAME_FN(void, __thiscall, Game_UpdateFov, 0x004FB5A0, void* self, float value);
GAME_FN(void, __thiscall, Game_BlendToButes, 0x004FBC50, void* self, float t);
GAME_FN(void, __thiscall, Game_GetHeightAdjust, 0x004FC770, void* self, void* ped, float* offset, float* t, float* height);

constexpr double kPi = 3.1415927410125732;       // (the float pi, as the game's double)
constexpr double kTenth = 0.10000000149011612;   // (0.1f as a double)
constexpr double kMilli = 0.0010000000474974513; // (0.001f as a double)

using re::At;

template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

const CVector* EntityPosition(void* entity) {
    void* matrix = At<void*>(entity, 0x14);
    return matrix ? &At<CVector>(matrix, 0x30) : &At<CVector>(entity, 4);
}

void* EmptyDestructor() {
    return reinterpret_cast<void*>(0x0049A650);
}

// {root, name}: a two-part name (destroyed with VectorDestruct)
void MakeParts(uint32_t* parts, const void* name) {
    Name_Copy(&parts[0], kFollowRoot);
    Name_Copy(&parts[1], name);
}

// ---- FollowInterp (the value easing, see Camera2.cpp)

struct FollowInterp {
    float from;          // +0
    float current;       // +4
    float target;        // +8
    uint32_t startTime;  // +0xC (CTimer ms; 0: not moving)
    int32_t duration;    // +0x10 ms
    uint8_t smooth;      // +0x14 eased in and out (a sine)
    uint8_t pad[3];
};
static_assert(sizeof(FollowInterp) == 0x18);

// 0x4FB3A0 (4 bytes: fld [ecx+4]; ret)
// 0x004FB3A0: [d9 41 04 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static float __fastcall Tiny_004FB3A0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const float*>(self + 0x4);
}
RE_UNHOOKED_FN("FollowInterp::GetCurrent", 0x004FB3A0, &Tiny_004FB3A0);
[[maybe_unused]] float __fastcall FollowInterp_GetCurrent(FollowInterp* self, re::Edx) {
    return self->current;
}

// 0x4FB400
void __fastcall FollowInterp_Update(FollowInterp* self, re::Edx) {
    if (self->startTime == 0) {
        return;
    }
    float t = float(double(uint32_t(timeInMs - self->startTime)) / double(self->duration));
    if (!(t < 1.0f)) {
        self->startTime = 0;
        self->current = self->target;
        return;
    }
    if (self->smooth) {
        float s = float(std::sin(double(float((double(t) - 0.5) * kPi))));
        t = float((double(s) + 1.0) * 0.5);
    }
    self->current = float((double(self->target) - self->from) * t + self->from);
}
FOLLOWCAM3_REVERSED("FollowInterp::Update", 0x004FB400, RE_BODY(&FollowInterp_Update), nullptr, nullptr, re::Mode::Original);

// ---- FollowCameraController3

// 0x4FB4D0: the lock-on marker's attribute set (+0x538), by the name in the camera
// settings (also the reload callback's last step: Camera2.cpp calls it
// FollowInterp_Unknown4FB4D0)
void __fastcall FollowCameraController3_LoadLockOnButes(void* self, re::Edx) {
    uint32_t name;
    Name_Construct(&name, At<int32_t>(cameraSettings, 0xFC));
    uint32_t parts[3];
    Name_Copy(&parts[0], kLockOnRoot);
    Name_Copy(&parts[1], kLockOnName);
    Name_Copy(&parts[2], &name);
    alignas(8) uint8_t path[8];
    Path_Build(path, 3, parts);
    At<void*>(self, 0x538) = Config_Find(path, kLockOnType);
    for (int i = 2; i >= 0; i--) {
        EmptyThiscall(&parts[i]);
    }
    EmptyThiscall(&name);
}
FOLLOWCAM3_REVERSED("FollowCameraController3::LoadLockOnButes4FB4D0", 0x004FB4D0, RE_BODY(&FollowCameraController3_LoadLockOnButes), nullptr, nullptr, re::Mode::Original);

// 0x4FB5A0: the fov between the camera's and the attribute set's (or the vehicle
// camera's) by the zoom (+0x28); `value` (non-zero) is the blend itself
void __fastcall FollowCameraController3_UpdateFov(void* self, re::Edx, float value) {
    uint8_t* butes = At<uint8_t*>(self, 0x518);
    float result;
    if (At<uint8_t>(self, 0x280)) {
        float k = float(double(At<float>(butes, 0x28)) * 30.0 * At<float>(self, 0x28));
        if (!(k < 1.0f)) {
            k = 1.0f;
        }
        if (value != 0.0f) {
            k = value;
        }
        float fov = TheCamera.fovOverridden ? TheCamera.fovOverride : TheCamera.fov;
        float vehicleFov = At<float>(FollowCameraVehicle_GetDrivenD4(static_cast<uint8_t*>(self) + 0x278), 0x18);
        result = float(double(k) * vehicleFov + (1.0 - k) * fov);
    } else {
        float k = float(double(At<float>(butes, 0x28)) * At<float>(self, 0x28));
        if (!(k < 1.0f)) {
            k = 1.0f;
        }
        if (value != 0.0f) {
            k = value;
        }
        float fov = TheCamera.fovOverridden ? TheCamera.fovOverride : TheCamera.fov;
        result = float((1.0 - k) * fov + double(At<float>(butes, 0x24)) * k);
    }
    if (!fovLocked) {
        TheCamera.fovOverridden = 0;
        TheCamera.fov = result;
    }
}
FOLLOWCAM3_REVERSED("FollowCameraController3::UpdateFov4FB5A0", 0x004FB5A0, RE_BODY(&FollowCameraController3_UpdateFov), nullptr, nullptr, re::Mode::Original);

// 0x4FB6C0 (4 bytes: mov al, [ecx+0x30]; ret): the flag CCamera::IsFollowCameraFlag30 reads
// 0x004FB6C0: [8a 41 30 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_004FB6C0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x30);
}
RE_UNHOOKED_FN("FollowCameraBase::GetFlag30", 0x004FB6C0, &Tiny_004FB6C0);
[[maybe_unused]] uint8_t __fastcall FollowCameraBase_GetFlag30(void* self, re::Edx) {
    return At<uint8_t>(self, 0x30);
}

// 0x4FB710: out = a * wa + b * wb, field by field (the flags: the heavier one's).
// (Called with out == a: each field reads only its own offset)
void __cdecl FollowCam3Butes_Blend(void* out, void* a, void* b, float wa, float wb) {
    auto blend = [&](size_t offset) {
        At<float>(out, offset) = float(double(At<float>(a, offset)) * wa + double(At<float>(b, offset)) * wb);
    };
    auto pick = [&](size_t offset) {
        At<uint8_t>(out, offset) = wa > wb ? At<uint8_t>(a, offset) : At<uint8_t>(b, offset);
    };
    for (size_t offset = 0x18; offset <= 0xB8; offset += 4) {
        blend(offset);
    }
    for (size_t offset = 0xBC; offset <= 0xC4; offset++) {
        pick(offset);
    }
    for (size_t offset = 0xC8; offset <= 0xE8; offset += 4) {
        blend(offset);
    }
    pick(0xEC);
    blend(0xF0);
    blend(0xF4);
}
FOLLOWCAM3_REVERSED("FollowCam3Butes::Blend", 0x004FB710, &FollowCam3Butes_Blend, nullptr, nullptr, re::Mode::Original);

// 0x4FBC50: the attribute set in use moved `t` of the way to +0x524 (with +0x51C) or
// +0x520 (0: 0.1)
void __fastcall FollowCameraController3_BlendToButes(void* self, re::Edx, float t) {
    if (At<uint8_t>(self, 0x534) || At<uint8_t>(self, 0x280)) {
        return;
    }
    if (t == 0.0f) {
        t = 0.1f;
    }
    void* butes = At<void*>(self, 0x518);
    void* to = At<uint8_t>(self, 0x51C) ? At<void*>(self, 0x524) : At<void*>(self, 0x520);
    Game_BlendButes(butes, butes, to, float(1.0 - t), t);
}
FOLLOWCAM3_REVERSED("FollowCameraController3::BlendToButes4FBC50", 0x004FBC50, RE_BODY(&FollowCameraController3_BlendToButes), nullptr, nullptr, re::Mode::Original);

// 0x4FBCF0: hands the view to TheCamera (0x4F2C60 on the position and the point
// looked at, the offset, +0x18C, +0x198) and updates the fov
void __fastcall FollowCameraController3_SetCameraView(void* self, re::Edx, CVector a, CVector offset) {
    uint8_t* p = static_cast<uint8_t*>(self);
    Camera_Unknown4F2C60(&TheCamera, p + 0x13C, p + 0x154);
    Camera_SetOffset(&TheCamera, &offset);
    cameraVector18C = a;
    TheCamera.unknown198 = At<CVector>(self, 0x154);
    Game_UpdateFov(self, 0.0f);
}
FOLLOWCAM3_REVERSED("FollowCameraController3::SetCameraView4FBCF0", 0x004FBCF0, RE_BODY(&FollowCameraController3_SetCameraView), nullptr, nullptr, re::Mode::Original);

// 0x4FBD70: +0x115 follows the flag at +0x30, with a reset (vtable[3]) on each change
// (+0x116 set during it)
void __fastcall FollowCameraController3_UpdateFlip(void* self, re::Edx) {
    if (At<uint8_t>(self, 0x30)) {
        if (!At<uint8_t>(self, 0x115)) {
            At<uint8_t>(self, 0x115) = 1;
            At<uint8_t>(self, 0x116) = 1;
            VCall<void>(self, 3);
            At<uint8_t>(self, 0x116) = 0;
        }
        return;
    }
    if (At<uint8_t>(self, 0x115)) {
        At<uint8_t>(self, 0x115) = 0;
        At<uint8_t>(self, 0x116) = 1;
        VCall<void>(self, 3);
        At<uint8_t>(self, 0x116) = 0;
    }
}
FOLLOWCAM3_REVERSED("FollowCameraController3::UpdateFlip4FBD70", 0x004FBD70, RE_BODY(&FollowCameraController3_UpdateFlip), nullptr, nullptr, re::Mode::Original);

// 0x4FBDD0: the lock-on marker over the player's target, while aiming (input 10) or
// with +0x1EC0, with a weapon that has one
void __fastcall FollowCameraController3_DrawLockOn(void* self, re::Edx) {
    LockOn_Clear();
    uint8_t* ped = static_cast<uint8_t*>(playerPed);
    bool aiming = Fn_738840(10, 0) > 0.0f;
    uint8_t flag = At<uint8_t>(ped, 0x1EC0);
    if (!aiming && !flag) {
        return;
    }
    void* weapon = At<void*>(ped, 0x1D0);
    if (!weapon || At<int16_t>(weapon, 0x10E) == -1 || !Weapon_GetInfoByte68(weapon)) {
        return;
    }
    void* targeting = At<void*>(ped, 0x1C8);
    void* target = Ped_GetTarget(ped);
    if (!target) {
        if (!flag) {
            return;
        }
        target = Targeting_4D7780(At<void*>(ped, 0x1C8));
        if (!target) {
            return;
        }
    }
    CVector position;
    Targeting_GetTargetPos(targeting, &position, target, 0);
    float lock = Targeting_GetLockProgress(targeting);
    float scale = float(1.0 + (1.0 - lock));
    if (Fn_856850(lockOnMarkerObject) != 0) {
        return;
    }
    void* butes = At<void*>(self, 0x538);
    EmptyThiscall(butes);
    LockOn_4DB290(At<int32_t>(butes, 0x40));
    uint8_t* b = At<uint8_t*>(self, 0x538);
    LockOn_Draw(position, float(At<float>(b, 0x1C) * scale), At<uint8_t>(b, 0x30), At<uint8_t>(b, 0x34), At<uint8_t>(b, 0x38),
                At<uint8_t>(b, 0x3C));
}
FOLLOWCAM3_REVERSED("FollowCameraController3::DrawLockOn4FBDD0", 0x004FBDD0, RE_BODY(&FollowCameraController3_DrawLockOn), nullptr, nullptr, re::Mode::Original);

// 0x4FBF10: the direction from the point looked at to the camera, normalised
// (reversed unless +0x115)
CVector* __fastcall FollowCameraController3_GetLookDirection(void* self, re::Edx, CVector* out) {
    uint8_t* p = static_cast<uint8_t*>(self);
    CVector d;
    VectorSub(&d, reinterpret_cast<CVector*>(p + 0x13C), reinterpret_cast<CVector*>(p + 0x154));
    Vector_Normalise(&d);
    if (!At<uint8_t>(self, 0x115)) {
        *out = {-d.x, -d.y, -d.z};
    } else {
        *out = d;
    }
    return out;
}
FOLLOWCAM3_REVERSED("FollowCameraController3::GetLookDirection4FBF10", 0x004FBF10, RE_BODY(&FollowCameraController3_GetLookDirection), nullptr, nullptr, re::Mode::Original);

// 0x4FBFC0: how far the camera's target is above +0x90 (0 when below, or none)
float __fastcall FollowCameraController3_GetTargetHeightAbove(void* self, re::Edx) {
    void* ped = TheCamera.targetPed;
    if (!ped) {
        return 0.0f;
    }
    double z = EntityPosition(ped)->z;
    CVector v;
    float d = float(z - FollowCameraController3_Get90(self, &v)->z);
    return d > 0.0f ? d : 0.0f;
}
FOLLOWCAM3_REVERSED("FollowCameraController3::GetTargetHeightAbove4FBFC0", 0x004FBFC0, RE_BODY(&FollowCameraController3_GetTargetHeightAbove), nullptr, nullptr, re::Mode::Original);

// 0x4FC030 (vtable[4]): starts from where the camera is
void __fastcall FollowCameraController3_Vf4(void* self, re::Edx) {
    cam::NiTransform* t = GetLocalTransform(GetCameraNode(TheCamera.camera));
    At<int32_t>(self, 0x2C) = 0;
    FollowCameraController3_Call5044A0(self, TheCamera.unknown198, t->translate, 1);
    At<float>(self, 0x530) = float(timeInMs);
    At<float>(self, 0x88) = 0.0f;
}
FOLLOWCAM3_REVERSED("FollowCameraController3::vf4", 0x004FC030, RE_BODY(&FollowCameraController3_Vf4), nullptr, nullptr, re::Mode::Original);

// 0x4FC0C0: picks the attribute sets for the situation (in a vehicle; the target ped
// with +0x1470/+0x1474, +0x16CC or +0x131C; the area kind from 0x430320) and blends
// to them (at once, or with 0x4FBC50 for the area switch)
void __fastcall FollowCameraController3_SelectButes(void* self, re::Edx) {
    uint8_t* p = static_cast<uint8_t*>(self);
    uint8_t& areaKind = p[0x528];     // 0x430320's answer the sets were picked for
    uint8_t& state131C = p[0x529];
    uint8_t& inVehicle = p[0x52A];
    uint8_t& state16CC = p[0x52B];
    uint8_t& state1470 = p[0x52C];
    if (p[0x534]) {
        return;
    }
    uint32_t parts[2];
    alignas(8) uint8_t path[8];
    if (p[0x280]) {
        if (inVehicle) {
            return;
        }
        MakeParts(parts, kNameVehicle);
        Path_Build(path, 2, parts);
        void* butes = Config_Find(path, kFollowType);
        At<void*>(self, 0x520) = butes;
        At<void*>(self, 0x524) = butes;
        areaKind = 0;
        state1470 = 0;
        inVehicle = 1;
        Game_BlendButes(At<void*>(self, 0x518), At<void*>(self, 0x518), butes, 0.0f, 1.0f);
        VectorDestruct(parts, 4, 2, EmptyDestructor());
        return;
    }
    if (inVehicle) {
        areaKind = Area_430320(currArea);
        inVehicle = 0;
    }
    uint8_t* ped = static_cast<uint8_t*>(TheCamera.targetPed);
    if (!ped || (ped[0x108] & 7) != 3) {
        ped = nullptr;
    } else if (At<int32_t>(ped, 0x1474) != 0 || At<int32_t>(ped, 0x1470) != 0) {
        if (Area_430320(currArea)) {
            if (areaKind == 0 && state1470 == 1) {
                return;
            }
            MakeParts(parts, kName541C);
            Path_Build(path, 2, parts);
            areaKind = 0;
        } else {
            if (areaKind == 1 && state1470 == 1) {
                return;
            }
            MakeParts(parts, kName5418);
            Path_Build(path, 2, parts);
            areaKind = 1;
        }
        void* butes = Config_Find(path, kFollowType);
        At<void*>(self, 0x524) = butes;
        At<void*>(self, 0x520) = butes;
        inVehicle = 0;
        state131C = 0;
        state1470 = 1;
        Game_BlendButes(At<void*>(self, 0x518), At<void*>(self, 0x518), butes, 0.0f, 1.0f);
        VectorDestruct(parts, 4, 2, EmptyDestructor());
        return;
    }
    if (state1470) {
        areaKind = Area_430320(currArea);
    }
    state1470 = 0;
    if (ped && ped[0x16CC]) {
        if (state16CC) {
            return;
        }
        MakeParts(parts, kName53D0);
        Path_Build(path, 2, parts);
        void* butes = Config_Find(path, kFollowType);
        At<void*>(self, 0x520) = butes;
        At<void*>(self, 0x524) = butes;
        state16CC = 1;
        Game_BlendButes(At<void*>(self, 0x518), At<void*>(self, 0x518), butes, 0.0f, 1.0f);
        VectorDestruct(parts, 4, 2, EmptyDestructor());
        return;
    }
    if (state16CC) {
        areaKind = Area_430320(currArea);
        state16CC = 0;
    } else if (!state131C) {
        if (ped && At<int32_t>(ped, 0x131C) != 0) {
            MakeParts(parts, areaKind ? kName542C : kName5414);
            Path_Build(path, 2, parts);
            At<void*>(self, 0x520) = Config_Find(path, kFollowType);
            VectorDestruct(parts, 4, 2, EmptyDestructor());
            state131C = 1;
            Game_BlendButes(At<void*>(self, 0x518), At<void*>(self, 0x518), At<void*>(self, 0x520), 0.0f, 1.0f);
        }
    } else if (ped && At<int32_t>(ped, 0x131C) == 0) {
        MakeParts(parts, areaKind ? kName5400 : kName540C);
        Path_Build(path, 2, parts);
        At<void*>(self, 0x520) = Config_Find(path, kFollowType);
        VectorDestruct(parts, 4, 2, EmptyDestructor());
        state131C = 0;
        Game_BlendButes(At<void*>(self, 0x518), At<void*>(self, 0x518), At<void*>(self, 0x520), 0.0f, 1.0f);
    }
    // the area switch: +0x520 the set to blend to (the +0x131C one with state131C),
    // +0x524 the other
    uint32_t a[2], b[2], c[2];
    alignas(8) uint8_t pathA[8], pathB[8], pathC[8];
    if (Area_430320(currArea)) {
        if (areaKind == 0) {
            return;
        }
        MakeParts(a, kName540C);
        MakeParts(b, kName537C);
        MakeParts(c, kName5414);
        Path_Build(pathA, 2, a);
        Path_Build(pathB, 2, b);
        Path_Build(pathC, 2, c);
        At<void*>(self, 0x520) = Config_Find(state131C ? pathC : pathA, kFollowType);
        At<void*>(self, 0x524) = Config_Find(pathB, kFollowType);
        areaKind = 0;
        Game_BlendToButes(self, 1.0f);
        VectorDestruct(c, 4, 2, EmptyDestructor());
        VectorDestruct(b, 4, 2, EmptyDestructor());
        VectorDestruct(a, 4, 2, EmptyDestructor());
        return;
    }
    if (areaKind != 0) {
        return;
    }
    MakeParts(a, kName5400);
    MakeParts(b, kName5380);
    MakeParts(c, kName542C);
    Path_Build(pathA, 2, a);
    Path_Build(pathB, 2, b);
    Path_Build(pathC, 2, c);
    At<void*>(self, 0x520) = Config_Find(state131C ? pathC : pathA, kFollowType);
    At<void*>(self, 0x524) = Config_Find(pathB, kFollowType);
    areaKind = 1;
    Game_BlendToButes(self, 1.0f);
    VectorDestruct(c, 4, 2, EmptyDestructor());
    VectorDestruct(b, 4, 2, EmptyDestructor());
    VectorDestruct(a, 4, 2, EmptyDestructor());
}
FOLLOWCAM3_REVERSED("FollowCameraController3::SelectButes4FC0C0", 0x004FC0C0, RE_BODY(&FollowCameraController3_SelectButes), nullptr, nullptr, re::Mode::Original);

// 0x4FC770: for a ped the camera follows: the height to rise by (`offset`, eased in
// by closeness `t`, within the attribute set's +0x88..+0x8C distances) and the
// height the camera may reach (`height`, from the ped's head or its vf 0x6C)
void __fastcall FollowCameraController3_GetHeightAdjust(void* self, re::Edx, void* ped, float* offset, float* t, float* height) {
    uint8_t* p = static_cast<uint8_t*>(self);
    float h;
    if (At<int32_t>(ped, 0xA4) != 0 && At<int32_t>(ped, 0x1310) != 0xD) {
        const CVector* position = EntityPosition(ped);
        h = float(double(VCall<float>(ped, 27)) * 0.8999999761581421 + position->z);
    } else {
        void* object = Fn_5EE1A0(At<void*>(ped, 0x18));
        float z1 = At<float>(Fn_6C7710(object), 0x304);
        float z2 = At<float>(Fn_6C7710(object), 0x94);
        const CVector* position = EntityPosition(ped);
        float lowest = z2 < z1 ? z1 : z2;
        h = float(double(VCall<float>(ped, 27)) * 0.1599999964237213 + position->z);
        if (h < lowest) {
            h = lowest;
        }
    }
    double top = double(heightOffset) + h;
    float lo = float(top - At<float>(self, 0x48));
    float hi = float((top + heightRange) - At<float>(self, 0x48));
    float limit = At<float>(self, 0x274);
    float result = lo;
    if (!(lo > limit)) {
        result = limit;
        if (hi < limit) {
            result = hi;
        }
    }
    *height = result;
    CVector d;
    VectorSub(&d, reinterpret_cast<CVector*>(p + 0x13C), EntityPosition(ped));
    uint8_t* butes = At<uint8_t*>(self, 0x518);
    double farDist = At<float>(butes, 0x8C);
    float flat = std::fabs(float(double(d.x) * d.x + double(d.y) * d.y));
    float dist = float(std::sqrt(double(flat)));
    float closeness = float((farDist - dist) / (double(At<float>(butes, 0x8C)) - At<float>(butes, 0x88)));
    *t = closeness;
    if (closeness < 0.0f) {
        closeness = 0.0f;
    } else if (closeness > 1.0f) {
        closeness = 1.0f;
    }
    *t = closeness;
    *offset = 0.0f;
    if (*t != 0.0f) {
        float scale = Clothing_6CAAE0(clothing);
        if (camera1B4 != 0) {
            scale = float(scale + kTenth);
        }
        float s = float(std::sin(double(float((double(*t) - 0.5) * kPi))));
        float w = float((double(s) + 1.0) * 0.5);
        *t = w;
        float wanted = float(double(*height) * scale + At<float>(self, 0x48));
        float z = float(double(wanted) * w + (1.0 - w) * double(At<float>(self, 0x144)));
        *offset = float(double(z) - At<float>(self, 0x144));
    }
}
FOLLOWCAM3_REVERSED("FollowCameraController3::GetHeightAdjust4FC770", 0x004FC770, RE_BODY(&FollowCameraController3_GetHeightAdjust), nullptr, nullptr, re::Mode::Original);

// 0x4FCA30: the camera and the point looked at raised for the target (and what it's
// in: +0x155C), the camera eased towards them; the target faded when too close
void __fastcall FollowCameraController3_UpdateHeight(void* self, re::Edx) {
    uint8_t* p = static_cast<uint8_t*>(self);
    void* ped = TheCamera.targetPed;
    float offset, t, height;
    Game_GetHeightAdjust(self, ped, &offset, &t, &height);
    if (void* other = At<void*>(ped, 0x155C)) {
        float offset2, t2, height2;
        Game_GetHeightAdjust(self, other, &offset2, &t2, &height2);
        offset = offset2 < offset ? offset : offset2;
        t = t2 < t ? t : t2;
        height = height2 < height ? height : height2;
    }
    At<float>(self, 0x144) = At<float>(self, 0x144) + offset;
    if (camera1B4 != 0 && Entity_473D60(ped)) {
        At<float>(self, 0x15C) = float(double(offset) * 0.5 + At<float>(self, 0x15C));
    } else {
        At<float>(self, 0x15C) = At<float>(self, 0x15C) + offset;
    }
    float g = offsetScale;
    CVector scaled = {g * At<float>(self, 0xD0), At<float>(self, 0xD4) * g, g * At<float>(self, 0xD8)};
    CVector base;
    VectorSub(&base, reinterpret_cast<CVector*>(p + 0x13C), &scaled);
    float k = At<float>(self, 0x25C);
    CVector look = At<CVector>(self, 0x154);
    CVector lookK = {look.x * k, look.y * k, look.z * k};
    float w = float(1.0 - At<float>(self, 0x25C));
    CVector baseW = {base.x * w, base.y * w, base.z * w};
    CVector c;
    VectorAdd(&c, &baseW, &lookK);
    CVector cT = {c.x * t, c.y * t, c.z * t};
    float w2 = float(1.0 - t);
    CVector lookW = {At<float>(self, 0x154) * w2, At<float>(self, 0x158) * w2, At<float>(self, 0x15C) * w2};
    CVector sum;
    const CVector* r = VectorAdd(&sum, &lookW, &cT);
    At<CVector>(self, 0x154) = *r;
    uint8_t* butes = At<uint8_t*>(self, 0x518);
    if (butes[0xC4]) {
        float fadeRange = At<float>(butes, 0xC8);
        float fadeStart = At<float>(butes, 0xCC);
        CVector position = *EntityPosition(ped);
        CVector d;
        const CVector* dp = VectorSub(&d, &position, reinterpret_cast<CVector*>(p + 0x13C));
        float distSq = float(double(dp->x) * dp->x + double(dp->y) * dp->y);
        float e = float(double(fadeRange) - (double(distSq) - fadeStart));
        if (e < 0.0f) {
            e = 0.0f;
        } else if (e > fadeRange) {
            e = fadeRange;
        }
        float alpha = float(1.0 - double(e) / fadeRange);
        Ped_SetAlpha(ped, int(double(alpha) * 255.0), 0);
    }
    At<float>(self, 0x274) = height;
}
FOLLOWCAM3_REVERSED("FollowCameraController3::UpdateHeight4FCA30", 0x004FCA30, RE_BODY(&FollowCameraController3_UpdateHeight), nullptr, nullptr, re::Mode::Original);

// ---- the swing (FollowCameraBase+0x68, 0x24 bytes): the camera leaning into turns

struct FollowSwing {
    float input;      // +0 (+0x68)
    CVector from;     // +4
    CVector to;       // +0x10
    float angle;      // +0x1C the turn between them
    float value;      // +0x20 (+0x88)
};
static_assert(sizeof(FollowSwing) == 0x24);

// 0x4FCD70: eased towards the input and the turn, less the faster the player moves
void __fastcall FollowSwing_Update(FollowSwing* self, re::Edx) {
    CVector a = self->from;
    CVector b = self->to;
    Vector_Normalise(&a);
    Vector_Normalise(&b);
    float headingA = GetAngleOfVector2D(&a);
    float headingB = GetAngleOfVector2D(&b);
    float turn = AngleDiff(headingB, headingA);
    self->angle = turn;
    const CVector& v = At<CVector>(playerPed, 0x124);   // its move speed
    float speedSq = float((double(v.y) * v.y + double(v.x) * v.x) + double(v.z) * v.z);
    float speed = float(std::sqrt(double(std::fabs(speedSq))));
    float perStep = float(double(speed) / timeStep);
    float k = float(1.0 - double(perStep) / swingSpeed);
    if (k < 0.0f) {
        k = 0.0f;
    } else if (k > 1.0f) {
        k = 1.0f;
    }
    if (std::fabs(self->input) < 0.01f) {
        swingFrames = 0;
        self->value = 0.0f;
    } else {
        swingFrames++;
        if (swingFrames > 1) {
            float wanted = float(double(self->input) * swingInputScale + double(turn) * swingAngleScale);
            float r = swingEase;
            self->value = float(double(r) * wanted + (1.0 - r) * double(self->value));
        }
    }
    float value = float(double(self->value) * k);
    self->value = value;
    // (swinging against the input stops it)
    if (self->input < kTenth) {
        if (value > 0.0f) {
            self->value = 0.0f;
            swingFrames = 0;
        }
    } else if (self->input > kTenth) {
        if (value < 0.0f) {
            self->value = 0.0f;
            swingFrames = 0;
        }
    }
}
FOLLOWCAM3_REVERSED("FollowSwing::Update", 0x004FCD70, RE_BODY(&FollowSwing_Update), nullptr, nullptr, re::Mode::Original);

// ---- FollowCameraBase's two timed effects

// 0x4FCF70: the one at +0x2B0 (started now, in seconds)
void __fastcall FollowCameraBase_StartEffect2B0(void* self, re::Edx, float a, float b, int flag) {
    At<uint8_t>(self, 0x2B0) = 1;
    At<uint8_t>(self, 0x2B1) = uint8_t(flag);
    At<uint8_t>(self, 0x2B2) = 1;
    At<float>(self, 0x2B4) = float(double(timeInMs) * kMilli);
    At<float>(self, 0x2B8) = a;
    At<float>(self, 0x2BC) = b;
}
FOLLOWCAM3_REVERSED("FollowCameraBase::StartEffect2B0", 0x004FCF70, RE_BODY(&FollowCameraBase_StartEffect2B0), nullptr, nullptr, re::Mode::Original);

// 0x4FCFC0: the one at +0x2C0: a point (into the camera target's frame) and a box
// (-w, -h, 0)..(w, h, 0)
void __fastcall FollowCameraBase_StartEffect2C0(void* self, re::Edx, float a, float b, int flag, CVector point, float w, float h) {
    At<uint8_t>(self, 0x2C0) = 1;
    At<uint8_t>(self, 0x2D0) = uint8_t(flag);
    At<float>(self, 0x2C4) = float(double(timeInMs) * kMilli);
    At<float>(self, 0x2C8) = a;
    CVector& p = At<CVector>(self, 0x2D4);
    p.x = point.x;
    At<float>(self, 0x2CC) = b;
    p.y = point.y;
    p.z = point.z;
    At<float>(self, 0x2E0) = -w;
    At<float>(self, 0x2E4) = -h;
    At<float>(self, 0x2E8) = 0.0f;
    At<float>(self, 0x2EC) = w;
    At<float>(self, 0x2F0) = h;
    At<float>(self, 0x2F4) = 0.0f;
    // (0x414C80 writes 16 bytes, the fourth from its own uninitialised stack: a CVector
    // here had it overwrite the stack cookie)
    float out[4];
    TransformVector(reinterpret_cast<CVector*>(out), Placeable_GetMatrix(TheCamera.targetPed), &p);
    p.x = out[0];
    p.y = out[1];
    p.z = out[2];
}
FOLLOWCAM3_REVERSED("FollowCameraBase::StartEffect2C0", 0x004FCFC0, RE_BODY(&FollowCameraBase_StartEffect2C0), nullptr, nullptr, re::Mode::Original);

} // namespace
