// 0x524290: the reflection manager's Render: the mirror whose box holds the camera (the
// last one first), its plane from its four corners, the box it is seen through (+0,
// +0x10) and the player's side of it (+0x20), then the scene drawn from the camera
// reflected in it (the player's weapon put away meanwhile outside first person) and
// the mirror given its plane. True when a mirror was drawn.
//
// The mirror entry (0x60 from +0x24): corners +0, +0x10, +0x20, +0x30 (the first's w
// is its area), the far clip +0x1C, +0x2C (to the camera as an int), a far distance
// +0x3C, the box +0x40..+0x5C.
//
// (x87 code: the sums are done in double here and stored as float where the game
// stores them)

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

namespace {

struct Vec {
    float x, y, z;
};

struct MirrorEntry {
    float corner0[3];   // +0
    int32_t area;       // +0xC
    float corner1[3];   // +0x10
    float farClip;      // +0x1C
    float corner2[3];   // +0x20
    float clip2C;       // +0x2C
    float corner3[3];   // +0x30
    float farDistance;  // +0x3C
    float boxMin[3];    // +0x40
    float unknown4C;
    float boxMax[3];    // +0x50
    float unknown5C;
};
static_assert(sizeof(MirrorEntry) == 0x60);

GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(void*, ms_pCamera_C3CCF4, 0x00C3CCF4);
GAME_VAR(void*, ms_pCameraMatrix_C3CCF0, 0x00C3CCF0);
GAME_VAR(int32_t, ms_cameraMode_C3CCF8, 0x00C3CCF8);
GAME_VAR(uint8_t, ms_bKeepFov_C3CF82, 0x00C3CF82);
GAME_VAR(int32_t, ms_bFovOverride, 0x00C3CDD4);
GAME_VAR(float, ms_fovOverride, 0x00C3CDD0);
GAME_VAR(float, ms_fov, 0x00C3CD04);
GAME_VAR(void*, ms_pMirrorSwitch_C674D8, 0x00C674D8);
GAME_VAR(uint8_t*, ms_pPlayerPed_C1AEA8, 0x00C1AEA8);
GAME_VAR(void*, ms_pMainRwCamera_C1AE7C, 0x00C1AE7C);
GAME_VAR(uint8_t*, ms_pEffects_B6F1D4, 0x00B6F1D4);
GAME_VAR(void*, ms_pEffects_B6F1EC, 0x00B6F1EC);
GAME_VAR(void*, ms_pMainFrame_BCBB48, 0x00BCBB48);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const TheCameraViewMatrix = reinterpret_cast<void*>(0x00C3CCB0);
inline void* const ms_lightAccumulator = reinterpret_cast<void*>(0x00D140B8);

GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(const float*, __cdecl, RwFrameGetLTM, 0x006CA0A0, void* frame);
GAME_FN(void, __cdecl, RwFrameUpdateObjects, 0x006C9AF0, void* frame);
GAME_FN(float, __thiscall, CVector_Normalise, 0x004294A0, void* vector);
GAME_FN(void, __thiscall, Vector_Max, 0x0051D0D0, float* self, const float* other);
GAME_FN(void, __thiscall, Vector_Min, 0x0051D150, float* self, const float* other);
GAME_FN(float*, __thiscall, Placeable_GetPosition_4154A0, 0x004154A0, void* entity, float* out);
GAME_FN(void, __thiscall, Ped_SetCurrentWeapon_4732A0, 0x004732A0, void* ped, int weapon, int flag);
GAME_FN(void, __cdecl, MatrixInvert, 0x00522440, float* out, const float* in);
GAME_FN(float*, __thiscall, NiTransform_Multiply, 0x00406830, const float* self, float* out, const float* other);
GAME_FN(void, __thiscall, Camera_SetRwCamera, 0x004F1800, void* camera, void* rwCamera);
GAME_FN(void, __thiscall, Camera_Update_4F1D10, 0x004F1D10, void* camera);
GAME_FN(void, __thiscall, Camera_Update_4F1F10, 0x004F1F10, void* camera);
GAME_FN(float, __thiscall, Camera_GetFarClip_4F1360, 0x004F1360, void* camera);
GAME_FN(void, __cdecl, Renderer_453A00, 0x00453A00);
GAME_FN(void, __cdecl, Renderer_451530, 0x00451530);
GAME_FN(void, __cdecl, Renderer_5EF890, 0x005EF890);
GAME_FN(void, __cdecl, Renderer_43AED0, 0x0043AED0);
GAME_FN(void, __cdecl, Renderer_43AF20, 0x0043AF20, int);
GAME_FN(void, __cdecl, RwCameraSetFarClipPlane, 0x006C8780, void* camera, float distance);
GAME_FN(void, __cdecl, RwCameraSetNearClipPlane, 0x006C8820, void* camera, float distance);
GAME_FN(void, __cdecl, RwCamera_6C8300, 0x006C8300, void* camera, int value);
GAME_FN(void, __cdecl, SetCameraProjection_5EF8A0, 0x005EF8A0, void* camera, uintptr_t, float tanHalfFov, float farClip);
GAME_FN(void, __cdecl, SetCurrentCamera_5F2EB0, 0x005F2EB0, void* camera);
GAME_FN(void, __cdecl, RwCameraClear, 0x006CA7D0, void* camera, const uint8_t* colour, int mode);
GAME_FN(void*, __cdecl, RwCameraBeginUpdate, 0x006C86A0, void* camera);
GAME_FN(void, __cdecl, RwCameraEndUpdate, 0x006C86C0, void* camera);
GAME_FN(void, __thiscall, Matrix_UpdateFromRw_412A70, 0x00412A70, void* matrix, void* rwMatrix);
GAME_FN(void, __cdecl, DefinedState_5EE1D0, 0x005EE1D0);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, void* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __thiscall, LightAccumulator_5EE560, 0x005EE560, void* self);
GAME_FN(void, __thiscall, Effects_578150, 0x00578150, void* self, void* camera);
GAME_FN(void, __thiscall, Effects_668780, 0x00668780, void* self, int);
GAME_FN(uint8_t*, __thiscall, MainFrame_GetMirror, 0x004025F0, void* self);
GAME_FN(void, __thiscall, MdBullyMirror_SetPlane, 0x00518AB0, void* mirror, const float* a, const float* b,
        const float* c, const float* d, void* camera, void* unused);

using re::At;

MirrorEntry* Entry(uint8_t* self, int index) {
    return reinterpret_cast<MirrorEntry*>(self + 0x24 + index * 0x60);
}

bool Contains(const MirrorEntry* entry, const float* pos) {
    return entry->area == currArea && entry->boxMin[0] < pos[0] && entry->boxMin[1] < pos[1] &&
           entry->boxMin[2] < pos[2] && entry->boxMax[0] > pos[0] && entry->boxMax[1] > pos[1] &&
           entry->boxMax[2] > pos[2];
}

} // namespace

namespace ReflectionRender {

// 0x524290 (thiscall)
static uint8_t __fastcall Render(uint8_t* self, re::Edx) {
    self[0x988] = 0xFF;
    self[0x98A] = 1;
    const float* cameraPos =
        reinterpret_cast<const float*>(GetLocalTransform(GetCameraNode(ms_pCamera_C3CCF4)) + 0x24);
    if (!vc::Call<uint8_t>(ms_pMirrorSwitch_C674D8, 0x10)) {
        int8_t last = int8_t(self[0x989]);
        if (uint8_t(last) != 0xFF && Contains(Entry(self, last), cameraPos)) {
            self[0x988] = uint8_t(last);
        } else {
            uint32_t count = At<uint32_t>(self, 0x984);
            for (uint32_t i = 0; i < count; i++) {
                if (Contains(Entry(self, int(i)), cameraPos)) {
                    self[0x988] = uint8_t(i);
                    break;
                }
            }
        }
    }
    if (self[0x988] == 0xFF) {
        self[0x98A] = 0;
        return 0;
    }
    self[0x989] = self[0x988];
    const MirrorEntry* entry = Entry(self, int8_t(self[0x988]));
    float a[3] = {entry->corner0[0], entry->corner0[1], entry->corner0[2]};
    float b[3] = {entry->corner1[0], entry->corner1[1], entry->corner1[2]};
    float c[3] = {entry->corner2[0], entry->corner2[1], entry->corner2[2]};
    float d[3] = {entry->corner3[0], entry->corner3[1], entry->corner3[2]};

    // the mirror's frame: the centre of the b-c diagonal, the edges b-d and b-a, the normal
    float centre[3];
    for (int i = 0; i < 3; i++) {
        float sum = c[i] + b[i];
        centre[i] = float(double(sum) * 0.5);
    }
    Vec e1 = {b[0] - d[0], b[1] - d[1], b[2] - d[2]};
    CVector_Normalise(&e1);
    Vec e2 = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    CVector_Normalise(&e2);
    Vec n = {
        float(double(e2.y) * e1.z - double(e2.z) * e1.y),
        float(double(e2.z) * e1.x - double(e1.z) * e2.x),
        float(double(e1.y) * e2.x - double(e2.y) * e1.x),
    };
    CVector_Normalise(&n);
    float mirror[13] = {e2.x, e1.x, n.x, e2.y, e1.y, n.y, e2.z, e1.z, n.z, centre[0], centre[1], centre[2], 1.0f};
    uint8_t* player = ms_pPlayerPed_C1AEA8;

    // the box the mirror is seen through
    const float e1v[3] = {e1.x, e1.y, e1.z};
    const float e2v[3] = {e2.x, e2.y, e2.z};
    float p1[3];
    float p2[3];
    for (int i = 0; i < 3; i++) {
        float u = float(double(e2v[i]) * -0.20000000298023224);
        float v = float(double(e1v[i]) * -0.10000000149011612);
        float start = float(double(v) + a[i]);
        p1[i] = start - u;
    }
    for (int i = 0; i < 3; i++) {
        float w = float(double(e2v[i]) * 0.20000000298023224);
        float x = float(double(e1v[i]) * 0.10000000149011612);
        float start = float(double(x) + d[i]);
        p2[i] = start - w;
    }
    const float nv[3] = {n.x, n.y, n.z};
    float q[3];
    float c1[3];
    float c2[3];
    for (int i = 0; i < 3; i++) {
        q[i] = float(double(nv[i]) * 1.25);
        c1[i] = q[i] + p1[i];
    }
    for (int i = 0; i < 3; i++) {
        c2[i] = q[i] + p2[i];
    }
    // (the box's fourth dwords are whatever the stack held; the minimum starts from the
    // maximum, so the first corner only counts for the maximum)
    float box[4] = {p1[0], p1[1], p1[2], 0.0f};
    Vector_Max(box, p2);
    Vector_Max(box, c1);
    Vector_Max(box, c2);
    std::memcpy(self, box, 16);
    Vector_Min(box, p2);
    Vector_Min(box, c1);
    Vector_Min(box, c2);
    std::memcpy(self + 0x10, box, 16);
    float playerPos[3];
    Placeable_GetPosition_4154A0(player, playerPos);
    At<float>(self, 0x20) = float((double(n.x) * playerPos[0] + double(n.y) * playerPos[1]) + double(n.z) * playerPos[2]);

    // the weapon put away (outside first person)
    uint8_t weaponFlag = 0;
    uint8_t hadWeapon = 0;
    uint8_t weapon = 0xFF;
    if (ms_cameraMode_C3CCF8 != 2) {
        weaponFlag = At<int32_t>(player, 0x1384) != 0 ? 1 : 0;
        weapon = player[0x1DE7];
        hadWeapon = weapon < 0xFF ? 1 : 0;
        Ped_SetCurrentWeapon_4732A0(player, 0xFF, 0);
    }
    float farClip = entry->farClip;
    float clip2C = entry->clip2C;
    float farDistance = entry->farDistance;
    float fov = ms_bFovOverride ? ms_fovOverride : ms_fov;

    // the reflected camera: the main camera in the mirror's space, z flipped, back out
    const float* ltm = RwFrameGetLTM(GetCameraNode(ms_pCamera_C3CCF4));
    void* mirrorCamera = At<void*>(self, 0x98C);
    Camera_SetRwCamera(TheCamera, mirrorCamera);
    uint8_t* node = static_cast<uint8_t*>(GetCameraNode(At<void*>(self, 0x98C)));
    float* frameMatrix = reinterpret_cast<float*>(GetLocalTransform(node));
    float inverse[13];
    inverse[12] = 1.0f;
    MatrixInvert(inverse, mirror);
    float local[13];
    float product[13];
    std::memcpy(local, NiTransform_Multiply(inverse, product, ltm), sizeof(local));
    local[6] = -local[6];
    local[7] = -local[7];
    local[8] = -local[8];
    local[11] = float(double(local[11]) * -1.0);
    float reflected[13];
    std::memcpy(reflected, NiTransform_Multiply(mirror, product, local), sizeof(reflected));
    RwCameraSetFarClipPlane(At<void*>(self, 0x98C), farClip);
    RwCameraSetNearClipPlane(At<void*>(self, 0x98C), 0.05f);
    RwCamera_6C8300(At<void*>(self, 0x98C), int32_t(clip2C));

    // first the scene set up from the mirror itself, 1 behind its plane, at 120 degrees
    std::memcpy(frameMatrix, mirror, sizeof(mirror));
    frameMatrix[9] = mirror[9] - mirror[2];
    frameMatrix[10] = mirror[10] - mirror[5];
    frameMatrix[11] = mirror[11] - mirror[8];
    std::memcpy(node + 0x48, frameMatrix, 0x34);
    node[3] = uint8_t((node[3] & 0xFB) | 8);
    RwFrameUpdateObjects(node);
    if (!ms_bKeepFov_C3CF82) {
        ms_bFovOverride = 0;
        ms_fov = 120.0f;
    }
    Camera_Update_4F1D10(TheCamera);
    Camera_Update_4F1F10(TheCamera);
    Renderer_453A00();
    Renderer_451530();
    Renderer_5EF890();

    // then the reflected camera
    std::memcpy(frameMatrix, reflected, sizeof(reflected));
    std::memcpy(node + 0x48, reflected, sizeof(reflected));
    node[3] = uint8_t((node[3] & 0xFB) | 8);
    RwFrameUpdateObjects(node);
    if (!ms_bKeepFov_C3CF82) {
        ms_bFovOverride = 0;
        ms_fov = fov;
    }
    Camera_Update_4F1D10(TheCamera);
    Camera_Update_4F1F10(TheCamera);
    float farPlane;
    if (farDistance == 0.0f) {
        farPlane = Camera_GetFarClip_4F1360(TheCamera);
    } else {
        float current = Camera_GetFarClip_4F1360(TheCamera);
        float cam[3] = {cameraPos[0], cameraPos[1], cameraPos[2]};
        float delta[3] = {centre[0] - cam[0], centre[1] - cam[1], centre[2] - cam[2]};
        float dist = float(std::sqrt(double(std::fabs(
            float((double(delta[0]) * delta[0] + double(delta[1]) * delta[1]) + double(delta[2]) * delta[2])))));
        float t = float(double(dist) / (double(farClip) * 0.5));
        float clamped;
        if (t < 0.0f) {
            clamped = 0.0f;
        } else if (1.0f < t) {
            clamped = 1.0f;
        } else {
            clamped = t;
        }
        farPlane = float((1.0 - clamped) * farDistance + double(current) * clamped);
    }
    float tanHalfFov = float(std::tan(double(float(double(fov) * 0.008726646502812704))));
    SetCameraProjection_5EF8A0(At<void*>(self, 0x98C), 0, tanHalfFov, farPlane);
    SetCurrentCamera_5F2EB0(At<void*>(self, 0x98C));
    const uint8_t grey[4] = {0x80, 0x80, 0x80, 0xFF};
    RwCameraClear(At<void*>(self, 0x98C), grey, 2);
    if (RwCameraBeginUpdate(At<void*>(self, 0x98C))) {
        Matrix_UpdateFromRw_412A70(TheCameraViewMatrix, ms_pCameraMatrix_C3CCF0);
        DefinedState_5EE1D0();
        uint32_t savedFogEnable;
        uint32_t savedCull;
        GetRenderState(0xE, &savedFogEnable);
        GetRenderState(0x14, &savedCull);
        SetRenderState(0x14, 3);
        SetRenderState(0xE, 0);
        Renderer_43AED0();
        if (ms_cameraMode_C3CCF8 == 2) {
            vc::Call<void>(player, 0x5C);
            At<int32_t>(player, 0xD0) = 0;
            vc::Call<void>(player, 0x48);
            At<int32_t>(player, 0xD0) = 1;
            LightAccumulator_5EE560(ms_lightAccumulator);
        }
        Renderer_43AF20(1);
        ms_pEffects_B6F1D4[5] = 0;
        Effects_578150(ms_pEffects_B6F1EC, ms_pCamera_C3CCF4);
        Effects_668780(ms_pEffects_B6F1D4, 0);
        ms_pEffects_B6F1D4[5] = 1;
        MdBullyMirror_SetPlane(MainFrame_GetMirror(ms_pMainFrame_BCBB48), a, b, c, d, At<void*>(self, 0x98C),
                               ms_pMainRwCamera_C1AE7C);
        SetRenderState(0x14, int(savedCull));
        SetRenderState(0xE, int(savedFogEnable));
        DefinedState_5EE1D0();
        RwCameraEndUpdate(At<void*>(self, 0x98C));
    }
    SetCurrentCamera_5F2EB0(ms_pMainRwCamera_C1AE7C);
    Camera_SetRwCamera(TheCamera, ms_pMainRwCamera_C1AE7C);
    Camera_Update_4F1D10(TheCamera);
    Camera_Update_4F1F10(TheCamera);
    if (hadWeapon) {
        // (the flag's upper bytes are left-over stack in the game)
        Ped_SetCurrentWeapon_4732A0(player, weapon, weaponFlag);
    }
    uint8_t drawn = self[0x988] != 0xFF ? 1 : 0;
    self[0x98A] = 0;
    return drawn;
}

} // namespace ReflectionRender

// ---- registration ----------------------------------------------------------------------

#define C16_REFLRENDER_REVERSED(name, fn, address)                                                   \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace ReflectionRender {

C16_REFLRENDER_REVERSED("CReflectionManager::Render", Render, 0x00524290);

} // namespace ReflectionRender
