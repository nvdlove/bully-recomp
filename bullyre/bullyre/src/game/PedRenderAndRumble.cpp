// CPed (0x476410 .. 0x476D64): the player's pad rumble from the ground, the flashlight
// beam, and its render (vf18: the ped, its weapon and held items, their motion blur).

#include <cmath>

#include "game/Ped.h"
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

GAME_VAR(uint32_t, PR_timeInMs, 0x00C1A9B4);          // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint32_t, PR_prevTimeInMs, 0x00C1A9AC);      // CTimer::m_snPreviousTimeInMilliseconds
GAME_VAR(float, PR_f_0_01, 0x0090054C);
GAME_VAR(double, PR_d_50, 0x00900AA0);
GAME_VAR(double, PR_d_1, 0x008FF1D8);
GAME_VAR(double, PR_d_0_025, 0x0090CED8);
GAME_VAR(float, PR_f_0_08, 0x00900DD8);
GAME_VAR(double, PR_d_15, 0x0090CED0);
GAME_VAR(double, PR_d_35, 0x00903918);

// The beam's colours and sizes
GAME_VAR(float, PR_beamC1, 0x0090CEFC);
GAME_VAR(float, PR_beamC2, 0x0090CEF8);
GAME_VAR(float, PR_beamC3, 0x0090CEF4);
GAME_VAR(float, PR_beamC4, 0x0090CEF0);
GAME_VAR(float, PR_beamC5, 0x0090CEEC);
GAME_VAR(float, PR_beamC6, 0x0090CEE8);
GAME_VAR(float, PR_f_3, 0x00900C14);
GAME_VAR(float, PR_beamLength, 0x0090CDD4);           // 6
GAME_VAR(double, PR_d_beamLength, 0x00900978);        // 6
GAME_VAR(float, PR_fltMax, 0x009009B8);
GAME_VAR(double, PR_d_1_1, 0x0090CEE0);
GAME_VAR(void*, PR_C2A8E8, 0x00C2A8E8);
inline void* const PR_animMgr = reinterpret_cast<void*>(0x020C4910);
inline void* const PR_beams = reinterpret_cast<void*>(0x00C66B78);

// Render state
GAME_VAR(uint8_t, PR_20C5BE9, 0x020C5BE9);
GAME_VAR(uint8_t, PR_cutsceneRunning, 0x020C5BE4);
GAME_VAR(uint8_t*, PR_C674F0, 0x00C674F0);            // +0x138: 1 or 2 scales the LOD distance
GAME_VAR(float, PR_lodScale, 0x00C3CD00);
GAME_VAR(float, PR_lodDistanceSq, 0x01B64878);
GAME_VAR(float, PR_lodDistanceSqInVehicle, 0x01B64874);
GAME_VAR(double, PR_d_60, 0x0090CE08);
GAME_VAR(double, PR_d_25, 0x00900DF0);
GAME_VAR(int32_t, PR_C3CCF8, 0x00C3CCF8);
GAME_VAR(int32_t, PR_flashlightModel, 0x00BF43A4);
inline void* const PR_camera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const PR_renderer = reinterpret_cast<void*>(0x00D140B8);

GAME_FN(void, __cdecl, PR_Rumble, 0x00738680, int pad, int duration, int strength, int a);
GAME_FN(float, __cdecl, PR_RandomFloat, 0x0041A5B0, float from, float to);
GAME_FN(float, __thiscall, PR_VectorMagnitude, 0x0040D9F0, const ped::CVector* v);
GAME_FN(void, __cdecl, PR_FindNode, 0x0060AE00, void* outA, void* outB, const char* name, void* animBlend);
GAME_FN(void, __thiscall, PR_NodeGetWorldPos, 0x006C1190, void* mgr, uintptr_t a, int b, ped::CVector* out, uintptr_t c);
GAME_FN(void, __thiscall, PR_NodeGetWorldTransform, 0x006BF570, void* mgr, uintptr_t a, int b, void* out, uintptr_t c);
GAME_FN(ped::CVector*, __cdecl, PR_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PR_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(uint8_t, __cdecl, PR_ProcessLineOfSight, 0x00461EA0, const ped::CVector* start, const ped::CVector* end,
        void* colPoint, re::Ptr32<void>* entity, int a, int b, int c);
GAME_FN(void, __thiscall, PR_AddBeam, 0x00513260, void* beams, void* beam);
GAME_FN(void, __cdecl, PR_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(float, __thiscall, PR_CameraFn4F1250, 0x004F1250, void* camera);
GAME_FN(ped::CVector*, __thiscall, PR_CameraGetPosition, 0x0041AAC0, void* camera);
GAME_FN(void, __thiscall, PR_PedFn4732D0, 0x004732D0, CPed* ped, int alpha);
GAME_FN(void, __thiscall, PR_RendererSetFloat, 0x008A20A0, void* renderer, float value);
GAME_FN(void, __thiscall, PR_RendererFn5EE570, 0x005EE570, void* renderer, int a);
GAME_FN(void, __thiscall, PR_HeldRender, 0x004A7460, void* held);
GAME_FN(void, __thiscall, PR_Fn4DABC0, 0x004DABC0, void* self);
GAME_FN(void, __thiscall, PR_EntityRender, 0x00466D40, CPed* ped);
GAME_FN(int, __thiscall, PR_BlurRender, 0x007566D0, void* blur, void* entity, uintptr_t a);
GAME_FN(void, __thiscall, PR_BlurDestruct, 0x0049A650, void* blur);
GAME_FN(void*, __cdecl, PR_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, PR_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __thiscall, PR_BlurConstruct, 0x00465080, void* self);
GAME_FN(void, __thiscall, PR_BlurAttach, 0x0044C070, void* blur, void* entity, int a, int b);
GAME_FN(void, __thiscall, PR_Blur435A00, 0x00435A00, void* blur, void* entity, int a);
GAME_FN(void, __thiscall, PR_BlurSet, 0x004650B0, void* blur, float a, float b, int c, float d, float e, int f, int g);
GAME_FN(void*, __thiscall, PR_HeldGet, 0x004A7410, void* held, int index);
GAME_FN(void, __thiscall, PR_PedRenderFlashlightBeam, 0x004766A0, CPed* ped);

// 0x476410 (thiscall): the player's pad rumbles on bumpy ground, and now and then on
// surface 0x3B
void __fastcall CPed_UpdateGroundRumble(CPed* self, re::Edx) {
    if (self->pedType != PED_TYPE_PLAYER) {
        return;
    }
    uint8_t surface = At<uint8_t>(self, 0x1BE);
    bool bumpy = surface == 8 || surface == 0x23 || surface == 0x2A || surface == 0x38 || surface == 0x3A;
    const ped::CVector& speed = At<ped::CVector>(self, 0x124);
    float sq2d = speed.x * speed.x + speed.y * speed.y;
    if (sq2d > PR_f_0_01 && bumpy) {
        float sq = speed.x * speed.x + speed.y * speed.y + speed.z * speed.z;
        int strength = static_cast<int>(sq * PR_d_50 + PR_d_1);
        PR_Rumble(0, 0x4B, strength & 0xFF, 1);
        return;
    }
    sq2d = speed.x * speed.x + speed.y * speed.y;
    if (sq2d > PR_d_0_025 && surface == 0x3B && ((PR_prevTimeInMs ^ PR_timeInMs) & 0xFFFFFF00) != 0) {
        float chance = PR_RandomFloat(0.0f, PR_f_0_08);
        if (chance < PR_VectorMagnitude(&speed)) {
            int strength = static_cast<int>(PR_VectorMagnitude(&speed) * PR_d_15) & 0xFF;
            int duration = static_cast<int>(PR_VectorMagnitude(&speed) * PR_d_35);
            // (only the low 16 bits are multiplied: imul ax)
            int pushed = static_cast<int>((static_cast<uint32_t>(duration) & 0xFFFF0000u) |
                                          (static_cast<uint32_t>(duration * 0x1E) & 0xFFFFu));
            PR_Rumble(0, pushed, strength, 1);
        }
    }
}

// 0x4766A0 (thiscall): the flashlight's beam from its "FxNode", 6 long (shortened to
// what it hits)
void __fastcall CPed_RenderFlashlightBeam(CPed* self, re::Edx) {
    // (the game leaves some of these uninitialised; ours are zeroed)
    alignas(4) uint8_t colPoint[0x28] = {};
    alignas(4) uint8_t beam[0x60] = {};
    alignas(4) uint8_t transform[0x34] = {};
    int32_t nodeA = 0;
    int32_t nodeB = 0;
    re::Ptr32<void> hitEntity = nullptr;
    ped::CVector pos = {0.0f, 0.0f, 0.0f};
    PR_FindNode(&nodeA, &nodeB, reinterpret_cast<const char*>(0x0090CF00), self->animBlend);  // "FxNode"
    PR_NodeGetWorldPos(PR_animMgr, nodeA, nodeB, &pos, 0);
    At<float>(transform, 0x30) = 1.0f;
    PR_NodeGetWorldTransform(PR_animMgr, nodeA, nodeB, transform, 0);
    float dirX = At<float>(transform, 0x04);
    float dirY = At<float>(transform, 0x10);
    float dirZ = At<float>(transform, 0x1C);
    At<ped::CVector>(beam, 0x00) = pos;
    At<float>(beam, 0x0C) = dirX;
    At<float>(beam, 0x10) = dirY;
    At<float>(beam, 0x14) = dirZ;
    At<float>(beam, 0x18) = 1.0f;
    At<float>(beam, 0x1C) = PR_f_3;
    At<float>(beam, 0x20) = PR_beamLength;
    At<float>(beam, 0x24) = 1.0f;
    At<float>(beam, 0x28) = PR_beamC3;
    At<float>(beam, 0x2C) = PR_beamC4;
    At<float>(beam, 0x30) = PR_beamC5;
    At<float>(beam, 0x34) = 1.0f;
    At<float>(beam, 0x38) = PR_beamC1;
    At<float>(beam, 0x3C) = PR_beamC2;
    At<float>(beam, 0x40) = PR_beamC3;
    At<float>(beam, 0x44) = 1.0f;
    At<float>(beam, 0x48) = PR_beamC1;
    At<float>(beam, 0x4C) = PR_beamC2;
    At<float>(beam, 0x50) = PR_beamC3;
    At<float>(beam, 0x54) = PR_beamC6;
    At<float>(beam, 0x58) = 1.0f;
    beam[0x5C] = 0;
    ped::CVector start = pos;
    ped::CVector reach = {static_cast<float>(dirX * PR_d_beamLength), static_cast<float>(dirY * PR_d_beamLength),
                          static_cast<float>(dirZ * PR_d_beamLength)};
    ped::CVector end;
    PR_VectorAdd(&end, &start, &reach);
    At<float>(colPoint, 0x24) = PR_fltMax;
    if (PR_ProcessLineOfSight(&start, &end, colPoint, &hitEntity, 1, 0, 0)) {
        ped::CVector d;
        ped::CVector* v = PR_VectorSub(&d, &At<ped::CVector>(colPoint, 0), &start);
        float dist = static_cast<float>(std::sqrt(static_cast<double>(std::fabs(v->x * v->x + v->y * v->y + v->z * v->z))));
        At<float>(beam, 0x24) = static_cast<float>(dist / At<float>(beam, 0x20) * PR_d_1_1);
    }
    PR_AddBeam(PR_beams, beam);
    // the col point's entity (+0xC, a registered reference)
    re::Ptr32<void>& entity = At<void*>(colPoint, 0xC);
    if (entity) {
        PR_CleanUpOldReference(entity, &entity);
    }
}

// vf18 0x4769C0: rendered (not in a cutscene, not when +0xD0, not a far ped in a vehicle)
// with its weapon and held items and their trails
void __fastcall CPed_Render(CPed* self, re::Edx) {
    if (PR_20C5BE9 == 0 && PR_cutsceneRunning != 0) {
        return;
    }
    if (At<int32_t>(self, 0xD0) != 0) {
        return;
    }
    bool scaled = false;
    int32_t mode = At<int32_t>(PR_C674F0, 0x138);
    if (mode == 2 || mode == 1) {
        float scale = PR_lodScale;
        scaled = true;
        float r = PR_CameraFn4F1250(PR_camera);
        PR_lodDistanceSq = static_cast<float>((scale * PR_d_60) * (PR_d_60 * scale) * r);
    }
    uint8_t* vehicle = re::PtrCast<uint8_t*>(self->vehicle);
    if (vehicle && At<int32_t>(vehicle, 0x46C) != 1 && self->pedType != PED_TYPE_PLAYER) {
        ped::CVector camera = *PR_CameraGetPosition(PR_camera);
        ped::CVector d;
        float dist = PR_VectorMagnitude(PR_VectorSub(&d, &camera, PosOf(self)));
        double range = PR_lodScale * PR_d_25;
        if (dist > range * range) {
            return;  // (without putting the LOD distance back)
        }
    }
    if (At<int16_t>(self->weapon, 0x10E) == PR_flashlightModel && At<int32_t>(self, 0x1410) != 0) {
        PR_PedRenderFlashlightBeam(self);
    }
    if (At<uint8_t>(self, 0x130E) == 0) {
        if (self->pedType == PED_TYPE_PLAYER && At<int32_t>(self, 0x1E94) != 0 && PR_C3CCF8 == 2) {
            goto done;
        }
        PR_PedFn4732D0(self, self->alpha);
        if (At<int32_t>(self->weapon, 0x144) != 0) {
            PR_RendererSetFloat(PR_renderer, 1.0f);
            reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(self->weapon))[0x48 / 4]))(self->weapon);
            PR_RendererSetFloat(PR_renderer, 1.0f);
        }
        PR_HeldRender(At<void*>(self, 0x1D8));
        PR_Fn4DABC0(At<void*>(self, 0x1D4));
        PR_PedFn4732D0(self, 0xFF);
        if (self->vehicle) {
            PR_lodDistanceSq = PR_lodDistanceSqInVehicle;
        }
        PR_RendererFn5EE570(PR_renderer, 7);
        PR_EntityRender(self);
        if (self->vehicle) {
            double d = PR_lodScale * PR_d_60;
            PR_lodDistanceSq = static_cast<float>(d * d);
        }
        goto done;
    }
    if (PR_BlurRender(At<void*>(self, 0x1D94), self, 1) == 0) {
        At<uint8_t>(self, 0x130E) = 0;
    }
    PR_RendererFn5EE570(PR_renderer, 7);
    if (self->pedType == PED_TYPE_PLAYER) {
        if (At<int32_t>(self, 0x1E94) != 0 && PR_C3CCF8 == 2) {
            goto done;
        }
        if (PR_C3CCF8 == 2) {
            goto heldItems;
        }
    }
    PR_BlurRender(At<void*>(self, 0x1D98), self->weapon, 0);
heldItems:
    for (int i = 0; i < 2; i++) {
        void* held = PR_HeldGet(At<void*>(self, 0x1D8), i);
        re::Ptr32<void>& blur = At<void*>(self, 0x1D9C + i * 4);
        if (blur) {
            if (held) {
                PR_BlurRender(blur, held, 0);
            } else {
                PR_BlurDestruct(blur);
                PR_Free(blur);
                blur = nullptr;
            }
        } else if (held) {
            uint8_t* body = At<uint8_t*>(self, 0x1D94);
            float a = At<float>(body, 0x24);
            int c = At<int32_t>(body, 0x10);
            int attach = At<int32_t>(body, 8);
            float b = At<float>(body, 0x28);
            float e = At<float>(body, 0x30);
            float d = At<float>(body, 0x2C);
            void* memory = PR_Malloc(0x34);
            blur = memory ? PR_BlurConstruct(memory) : nullptr;
            PR_BlurAttach(blur, held, attach, 0);
            PR_Blur435A00(blur, held, attach);
            PR_BlurSet(blur, a, b, c, d, e, 0, 0);
        }
    }
done:
    if (scaled) {
        double d = PR_lodScale * PR_d_60;
        PR_lodDistanceSq = static_cast<float>(d * d);
    }
}

#define PEDRENDER_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDRENDER_REVERSED("CPed::UpdateGroundRumble", 0x00476410, &CPed_UpdateGroundRumble);
PEDRENDER_REVERSED("CPed::RenderFlashlightBeam", 0x004766A0, &CPed_RenderFlashlightBeam);
PEDRENDER_REVERSED("CPed::Render", 0x004769C0, &CPed_Render);

} // namespace

// Ghidra's FUN_004769ca: not a function, the middle of CPed::Render (0x4769C0) after its
// first two instructions (nothing calls or jumps to it)
RE_UNHOOKED("CPed::Render (mid-function)", 0x004769CA);
