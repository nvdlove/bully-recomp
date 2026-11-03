#include "game/Camera.h"

#include <cmath>
#include "reversed.h"

// StoreCameraController (0x5093D0..0x509780): the shop camera, at a fixed position
// looking at a scripted entity, the look-at moved to a new entity along an eased
// curve; and ScriptCameraController's constructor and path set-up (0x5078C0,
// 0x507970). Thiscalls are __fastcall wrappers with the unused edx.
//
// Layout (StoreCameraController):
//   +4/+8 the entity's script handle and kind, +0xC snap (no easing), +0x10 the camera
//   position, +0x1C the look-at, +0x28 where the look-at goes, +0x34 where it came
//   from, +0x40 the way (+0x28 - +0x1C when set), +0x4C the speed, +0x50 how far along
//   (0..1), +0x54 that eased, +0x58 the turn about the camera (radians), +0x5C how far
//   the camera is pulled towards the look-at.
// The settings (0x20C891C): +0x18 accelerate (+0x1C by), +0x20 the start speed, +0x24
// the top speed, +0x28 keep the speed, +0x2C the easing (0 none, 1 out, 2 sine, 3 a
// power), +0x30 the power.
//
// ScriptCameraController: +8 the near clip, +0x10, +0x3C a spline, +0x90 a vector,
// +0xA4 the spline the camera moves along, +0xF4, +0x104 the fov.

#define STORECAM_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;
using cam::NiTransform;

// the game's CMatrix: right, forward, up, position (4 floats each), the attached
// NiTransform
struct Matrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(Matrix) == 0x44);

GAME_VAR(uint8_t*, storeCameraSettings, 0x020C891C);
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);
GAME_VAR(void*, theNiCamera, 0x00C3CCF4);   // TheCamera.camera
GAME_VAR(int32_t, g_BF4130, 0x00BF4130);    // (a model index: its position raised 0.5)
GAME_VAR(float, g_A45970, 0x00A45970);      // pi / 2

inline void* const scriptFiles = reinterpret_cast<void*>(0x00D02850);
constexpr uintptr_t kScriptCameraVtable = 0x009146D0;

GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Matrix_RotateZ, 0x00413220, Matrix* m, float angle);
GAME_FN(uint8_t*, __cdecl, Script_GetEntity, 0x005C28D0, int handle, int kind, int pedItself, int cutscene);
GAME_FN(CVector*, __thiscall, PedA64_GetPosition, 0x0048D020, void* self, CVector* out);   // (on the ped's +0xA64)
GAME_FN(void, __thiscall, CameraController_LookAt, 0x00507AC0, void* self, NiTransform* matrix, CVector position, CVector target);
GAME_FN(void, __thiscall, StoreCam_Advance, 0x005094A0, void* self);
// the script camera's
GAME_FN(void*, __thiscall, Spline_Construct, 0x005091C0, void* self);
GAME_FN(void, __thiscall, Spline_Clear, 0x00508420, void* self);
GAME_FN(void, __thiscall, Spline_SetPath, 0x00509240, void* self, void* path);
GAME_FN(void, __thiscall, ScriptCameraController_Clear, 0x00507670, void* self);
GAME_FN(void*, __thiscall, Scripts_GetCurrent, 0x005DB960, void* files);
GAME_FN(uint8_t*, __thiscall, LuaScript_GetCurrentThread, 0x005D8B90, void* script);
GAME_FN(void*, __thiscall, Path_Construct, 0x005EC5E0, void* path, int value);
GAME_FN(void, __thiscall, Path_Destruct, 0x005ECF80, void* path);
GAME_FN(void, __thiscall, PathPoint_Construct, 0x005EC570, void* point);
GAME_FN(void, __thiscall, PathPoint_Destruct, 0x005EC7B0, void* point);
GAME_FN(void, __thiscall, Path_AddPoint, 0x005ECB20, void* path, const void* point);
GAME_FN(uint32_t, __thiscall, Path_GetNumNodes, 0x005EC670, void* path);
GAME_FN(void*, __thiscall, Path_FirstNode, 0x005EC830, void* path);
GAME_FN(void*, __thiscall, Path_NextNode, 0x005EC870, void* path);

using re::At;

template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

// ---- 0x5078C0: ScriptCameraController's constructor
void* __fastcall ScriptCameraController_ConstructOurs(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kScriptCameraVtable;
    Spline_Construct(self + 0x3C);
    At<float>(self, 0x90) = 0.0f;
    At<float>(self, 0x94) = 0.0f;
    At<float>(self, 0x98) = 0.0f;
    Spline_Construct(self + 0xA4);
    At<float>(self, 8) = 0.05f;
    At<float>(self, 0x104) = float(At<int32_t>(cameraSettings, 0x6C));
    ScriptCameraController_Clear(self);
    At<int32_t>(self, 0x10) = 1;
    Spline_Clear(self + 0xA4);
    return self;
}
STORECAM_REVERSED("ScriptCameraController::ScriptCameraController_5078C0", 0x005078C0, RE_BODY(&ScriptCameraController_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x507970: the camera to move along `path` (`mode` 0: from where the camera is now;
// otherwise a one-point path's point twice, so the spline has a segment), the path
// copied under the running script thread's id
void __fastcall ScriptCameraController_SetPathOurs(uint8_t* self, re::Edx, void* path, int mode) {
    At<int32_t>(self, 0x10) = 1;
    Spline_Clear(self + 0xA4);
    At<int32_t>(self, 0xF4) = 0;
    At<int32_t>(self, 0x10) = mode;
    void* script = Scripts_GetCurrent(scriptFiles);
    int32_t id = -1;
    if (script) {
        id = At<int32_t>(LuaScript_GetCurrentThread(script), 0x20);
    }
    alignas(4) uint8_t copy[0x14];
    Path_Construct(copy, id);
    if (mode == 0) {
        NiTransform* t = GetLocalTransform(GetCameraNode(theNiCamera));
        alignas(4) uint8_t point[0x24];   // (a PathNode: 0x24 bytes, the position at +0)
        PathPoint_Construct(point);
        At<CVector>(point, 0) = t->translate;
        Path_AddPoint(copy, point);
        PathPoint_Destruct(point);
    } else if (Path_GetNumNodes(path) == 1) {
        Path_AddPoint(copy, Path_FirstNode(path));
    }
    for (void* node = Path_FirstNode(path); node;) {
        void* next = Path_NextNode(path);
        Path_AddPoint(copy, node);
        node = next;
    }
    Spline_SetPath(self + 0xA4, copy);
    Path_Destruct(copy);
}
STORECAM_REVERSED("ScriptCameraController::SetPath_507970", 0x00507970, RE_BODY(&ScriptCameraController_SetPathOurs), nullptr, nullptr, re::Mode::Original);

// ---- 0x5093D0 (vtable[3]): reset (no entity, everything at 0, snap)
void __fastcall StoreCam_ResetOurs(uint8_t* self, re::Edx) {
    At<int32_t>(self, 4) = -1;
    At<int32_t>(self, 8) = -1;
    for (size_t offset = 0x1C; offset <= 0x5C; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    At<uint8_t>(self, 0xC) = 1;
}
STORECAM_REVERSED("StoreCameraController::Reset_5093D0", 0x005093D0, RE_BODY(&StoreCam_ResetOurs), nullptr, nullptr, re::Mode::Original);

// 0x5094A0: the speed (accelerated up to the top speed), how far along, and that eased
void __fastcall StoreCam_AdvanceOurs(uint8_t* self, re::Edx) {
    uint8_t* settings = storeCameraSettings;
    if (At<uint8_t>(settings, 0x18)) {
        At<float>(self, 0x4C) = At<float>(settings, 0x1C) + At<float>(self, 0x4C);
        settings = storeCameraSettings;
    }
    if (At<float>(settings, 0x24) < At<float>(self, 0x4C)) {
        At<float>(self, 0x4C) = At<float>(settings, 0x24);
    }
    float along = At<float>(self, 0x50) + At<float>(self, 0x4C);
    At<float>(self, 0x50) = along;
    switch (At<int32_t>(storeCameraSettings, 0x2C)) {
    case 0:
        At<float>(self, 0x54) = along;
        break;
    case 1: {
        float back = along - 1.0f;
        float squared = back * back;
        At<float>(self, 0x54) = 1.0f - squared;
        break;
    }
    case 2: {
        float angle = along * g_A45970;
        At<float>(self, 0x54) = float(std::sin(double(angle)));
        break;
    }
    case 3:
        At<float>(self, 0x54) = float(std::pow(double(along), double(At<float>(storeCameraSettings, 0x30))));
        break;
    default:
        break;
    }
}
STORECAM_REVERSED("StoreCameraController::Advance_5094A0", 0x005094A0, RE_BODY(&StoreCam_AdvanceOurs), nullptr, nullptr, re::Mode::Original);

// 0x5095F0: the look-at to a scripted entity (a ped: its +0xA64's position; model
// 0xBF4130: raised 0.5), turned about the camera by +0x58; the move starts from where
// the look-at is (processed at once with `snap`)
void __fastcall StoreCam_SetTargetOurs(uint8_t* self, re::Edx, int handle, int kind, int snap) {
    uint8_t* settings = storeCameraSettings;
    if (!At<uint8_t>(settings, 0x28) || 0.0f == At<float>(self, 0x4C)) {
        At<float>(self, 0x4C) = At<float>(settings, 0x20);
    }
    At<float>(self, 0x54) = 0.0f;
    At<float>(self, 0x50) = 0.0f;
    At<int32_t>(self, 4) = handle;
    At<int32_t>(self, 8) = kind;
    At<uint8_t>(self, 0xC) = uint8_t(snap);
    uint8_t* entity = Script_GetEntity(handle, kind, 0, 0);
    CVector* to = &At<CVector>(self, 0x28);
    if ((At<uint8_t>(entity, 0x108) & 7) == 3) {
        CVector at;
        *to = *PedA64_GetPosition(entity + 0xA64, &at);
    } else {
        uint8_t* matrix = At<uint8_t*>(entity, 0x14);
        *to = matrix ? At<CVector>(matrix, 0x30) : At<CVector>(entity, 4);
        if (int32_t(At<int16_t>(entity, 0x10E)) == g_BF4130) {
            to->z = float(double(to->z) + 0.5);
        }
    }
    // (turned as a matrix's position; the rest of the matrix is uninitialised stack)
    Matrix m;
    m.attached = nullptr;
    CVector away;
    VectorSub(&away, to, &At<CVector>(self, 0x10));
    m.m[12] = away.x;
    m.m[13] = away.y;
    m.m[14] = away.z;
    Matrix_RotateZ(&m, At<float>(self, 0x58));
    CVector sum;
    *to = *VectorAdd(&sum, &At<CVector>(self, 0x10), reinterpret_cast<CVector*>(&m.m[12]));
    At<CVector>(self, 0x34) = At<CVector>(self, 0x1C);
    CVector way;
    At<CVector>(self, 0x40) = *VectorSub(&way, to, &At<CVector>(self, 0x1C));
    if (At<uint8_t>(self, 0xC)) {
        VCall<void>(self, 7);
    }
}
STORECAM_REVERSED("StoreCameraController::SetTarget_5095F0", 0x005095F0, RE_BODY(&StoreCam_SetTargetOurs), nullptr, nullptr, re::Mode::Original);

// 0x509780 (vtable[7]): the look-at moved along (or snapped; all stopped once it's
// there), then the camera's node turned to look at it from the camera position pulled
// towards it by +0x5C
void __fastcall StoreCam_ProcessOurs(uint8_t* self, re::Edx) {
    NiTransform* t = GetLocalTransform(GetCameraNode(theNiCamera));
    CVector* lookAt = &At<CVector>(self, 0x1C);
    if (lookAt->x == At<float>(self, 0x28) && lookAt->y == At<float>(self, 0x2C) && lookAt->z == At<float>(self, 0x30)) {
        At<float>(self, 0x4C) = 0.0f;
        At<float>(self, 0x54) = 0.0f;
        At<float>(self, 0x50) = 0.0f;
    } else {
        bool snap = At<uint8_t>(self, 0xC) != 0;
        if (!snap) {
            StoreCam_Advance(self);
            float along = At<float>(self, 0x50);
            snap = along >= 1.0f;   // (NaN: moves on)
        }
        if (snap) {
            *lookAt = At<CVector>(self, 0x28);
        } else {
            float eased = At<float>(self, 0x54);
            CVector part{eased * At<float>(self, 0x40), At<float>(self, 0x44) * eased, eased * At<float>(self, 0x48)};
            CVector sum;
            *lookAt = *VectorAdd(&sum, &At<CVector>(self, 0x34), &part);
        }
    }
    float pull = At<float>(self, 0x5C);
    CVector toLookAt;
    const CVector* d = VectorSub(&toLookAt, lookAt, &At<CVector>(self, 0x10));
    CVector part{pull * d->x, pull * d->y, d->z * pull};
    CVector from;
    VectorAdd(&from, &At<CVector>(self, 0x10), &part);
    CameraController_LookAt(self, t, from, *lookAt);
}
STORECAM_REVERSED("StoreCameraController::Process_509780", 0x00509780, RE_BODY(&StoreCam_ProcessOurs), nullptr, nullptr, re::Mode::Original);

} // namespace
