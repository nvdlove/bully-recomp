// 0x513010 to 0x513780: the tail of the coronas file: its two render passes, the
// lights-on factor from the time of day, and the light shafts (cones of light drawn
// for vehicle lights and the like: LightShafts at 0xC66B78, up to 5 a frame, each drawn
// as two copies of one model, with coronas at its start).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

struct LsVector {
    float x, y, z;
};
static_assert(sizeof(LsVector) == 0xC);

// Gamebryo's NiTransform (rotation, translation, scale)
struct LsTransform {
    float rot[9];      // +0x00
    LsVector trans;    // +0x24
    float scale;       // +0x30
};
static_assert(sizeof(LsTransform) == 0x34);

// a light shaft (what 0x513260 is given; AutomobileProcessControl.cpp fills them)
struct LightShaft {
    LsVector pos;        // +0x00
    LsVector dir;        // +0x0C
    float unknown18;     // +0x18 (the width's scale)
    float length;        // +0x1C
    float unknown20;     // +0x20
    float intensity;     // +0x24
    float colours[12];   // +0x28 three RGBA: the shaft's, the inner copy's, the coronas'
    float alpha;         // +0x58 scales the colours (and the coronas' alpha)
    uint8_t unknown5C;   // +0x5C
    uint8_t pad5D[3];
};
static_assert(sizeof(LightShaft) == 0x60);
static_assert(offsetof(LightShaft, colours) == 0x28);
static_assert(offsetof(LightShaft, alpha) == 0x58);

struct LightShafts {
    int32_t count;             // +0x000 this frame's
    LightShaft shafts[5];      // +0x004
    re::Ptr32<void> shaftClump;          // +0x1E4 an instance of model ms_shaftModel
    re::Ptr32<void> bone1E8;             // +0x1E8 (made once, not used here)
    re::Ptr32<void> clump1EC;            // +0x1EC an instance of model ms_shaftModel2 (kept up, never drawn)
};
static_assert(offsetof(LightShafts, shafts) == 0x4);
static_assert(offsetof(LightShafts, shaftClump) == 0x1E4);
static_assert(offsetof(LightShafts, clump1EC) == 0x1EC);

// the copies drawn for each of the 5 shafts (made the first time a slot is used)
struct ShaftSlot {
    re::Ptr32<uint8_t> inner;    // +0 drawn second, with the second colour
    re::Ptr32<uint8_t> outer;    // +4 drawn first, stretched by the shaft's length
    re::Ptr32<uint8_t> effect;   // +8 a 0x1C-byte object (0x51A460) on both copies' nodes
};
static_assert(sizeof(ShaftSlot) == 0xC);
GAME_ARRAY(ShaftSlot, ms_shaftSlots, 0x00C66B38);

GAME_VAR(int32_t, ms_shaftModel, 0x00BF417C);
GAME_VAR(int32_t, ms_shaftModel2, 0x00BF4178);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);

// the coronas' lights-on factor (1 at night, 0 by day)
GAME_VAR(float, ms_lightsOnFactor, 0x00C66B20);
GAME_VAR(uint8_t, ms_lightsOnOverride, 0x00C66B24);
GAME_VAR(float, ms_lightsOnOverrideValue, 0x00C66B28);
GAME_VAR(uint8_t, ms_clockSecondsLow, 0x00BCEB48);   // CClock's seconds (a uint16: its low byte is read)
GAME_VAR(uint8_t, ms_clockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t, ms_clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, ms_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, ms_chapterSeason, 0x00A147EC);
GAME_ARRAY(float, ms_seasonLightsOffHour, 0x00A6485C);   // by season (the morning)
GAME_ARRAY(float, ms_seasonLightsOnHour, 0x00A6486C);    // by season (the evening)

GAME_VAR(int32_t, ms_currentCamera, 0x00C3CCF4);
GAME_VAR(uint8_t, ms_bRenderFlagD14550, 0x00D14550);
GAME_VAR(LsVector, NiPoint3_ZERO, 0x020E2D44);
inline void* const TheRenderer = reinterpret_cast<void*>(0x00D140B8);

GAME_FN(void, __cdecl, Coronas_Render, 0x005115A0, int pass);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __cdecl, Coronas_Register, 0x005124D0, uintptr_t id, int r, int g, int b, int a, const LsVector* pos,
        float size, float range, int type, int a10, int a11, int a12, float angle, int a14, int a15, float a16);

// the scene graph (Bully's RW-like layer over Gamebryo)
GAME_FN(void*, __cdecl, Node_GetRoot, 0x006C7370, void* object);
GAME_FN(void, __cdecl, Node_AttachBone, 0x006C6A10, void* object, void* bone);
GAME_FN(void, __cdecl, Bone_Destroy, 0x006CA260, void* bone);
GAME_FN(void, __cdecl, Node_Destroy, 0x006C72C0, void* object);
GAME_FN(void*, __cdecl, NewBone, 0x006CA210);
GAME_FN(void*, __cdecl, Node_Clone, 0x006C70D0, void* object);
GAME_FN(uint32_t, __cdecl, Node_GetSharedData, 0x00824EC0, void* object);
GAME_FN(void, __cdecl, Node_SetSharedData, 0x006CA7D0, void* object, uint32_t data, int);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(LsTransform*, __cdecl, Frame_GetTransform, 0x006C9A90, void* frame);
GAME_FN(void*, __cdecl, Camera_GetFrame, 0x006C82F0, uintptr_t camera);
GAME_FN(void, __cdecl, Frame_Update, 0x006C9AF0, void* frame);
GAME_FN(void, __cdecl, Fn_6C6B90, 0x006C6B90, void* object);
GAME_FN(void, __cdecl, SetObjectWorldTransform, 0x004068E0, void* object, LsTransform* transform);
GAME_FN(void, __cdecl, Object_SetColour, 0x004088C0, void* object, float r, float g, float b, float a);
GAME_FN(void, __cdecl, RenderObject, 0x00402700, void* object, int, int);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E040, void* object);
GAME_FN(LsTransform*, __thiscall, Transform_Multiply, 0x00406830, const LsTransform* self, LsTransform* out,
        const LsTransform* other);
GAME_FN(void, __thiscall, Matrix_MakeXRotation, 0x007687D0, float* matrix, float angle);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);

GAME_FN(void, __thiscall, ShaftEffect_Construct, 0x0051A460, void* self, uintptr_t);
GAME_FN(void, __thiscall, ShaftEffect_Add, 0x0051A740, void* self, void* node);
GAME_FN(void, __thiscall, ShaftEffect_Apply, 0x00519CB0, void* self);

GAME_FN(void, __thiscall, Renderer_Empty, 0x0049A650, void* renderer);   // (just a ret)
GAME_FN(void, __thiscall, Renderer_ResetState, 0x005EE560, void* renderer);
GAME_FN(void, __thiscall, Renderer_SetColour, 0x008A20A0, void* renderer, const float* colour);   // (does nothing)
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int32_t* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);

// vector helpers
GAME_FN(LsVector*, __cdecl, VectorSub, 0x00414B70, LsVector* out, const LsVector* a, const LsVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, LsVector* v);
GAME_FN(LsVector*, __cdecl, CrossProduct, 0x00414730, LsVector* out, const LsVector* a, const LsVector* b);
GAME_FN(float, __cdecl, ACos, 0x00413D60, float x);

// the camera's position
LsVector CameraPos() {
    LsTransform* camera = Frame_GetTransform(Camera_GetFrame(ms_currentCamera));
    return camera->trans;
}

uint8_t ColourByte(double value) {
    return uint8_t(int(value));
}

// a model's instance made (memory tag 0xD) or its shared data brought up to date:
// what 0x513780 does for both clumps
bool UpdateClump(re::Ptr32<void>& clump, uint8_t* modelInfo) {
    if (!clump) {
        PushMemoryTag(0xD);
        clump = vc::Call<void*>(modelInfo, 0x10);
        PopMemoryTag();
        if (!clump) {
            return false;
        }
        Node_AttachBone(clump, NewBone());
        Node_GetSharedData(clump);
    }
    uint32_t current = Node_GetSharedData(clump);
    uint32_t wanted = Node_GetSharedData(vc::Call<void*>(modelInfo, 0x14));
    if (current != wanted) {
        if (current) {
            Free(reinterpret_cast<void*>(uintptr_t(current)));
        }
        Node_SetSharedData(clump, wanted, 1);
        Node_GetSharedData(clump);
    }
    return true;
}

} // namespace

namespace CCoronas {

// 0x513010: the first render pass
static void __cdecl RenderPass0() {
    Coronas_Render(0);
}

// 0x513020: the second
static void __cdecl RenderPass1() {
    Coronas_Render(1);
}

// 0x513030: how far the lights are on, from the time of day and the season's hours
// (going off over the hour before the morning's hour, coming on over the hour before
// the evening's; the override's value when it's set)
static void __cdecl UpdateLightsOnFactor() {
    const double day = 0.0416666679084301;   // 1/24
    int season = ms_chapterSeason[ms_chapter];
    float minutes = float(ms_clockMinutes * 0.01666666753590107);
    float seconds = float(ms_clockSecondsLow * 0.00027777778450399637);
    float t = float((double(minutes) + ms_clockHours + seconds) * day);
    if (ms_lightsOnOverride) {
        ms_lightsOnFactor = ms_lightsOnOverrideValue;
        return;
    }
    double off = ms_seasonLightsOffHour[season];
    double on = ms_seasonLightsOnHour[season];
    if (t < (off - 1.0) * day) {
        ms_lightsOnFactor = 1.0f;
        return;
    }
    if (t >= (off - 1.0) * day && off * day > t) {
        ms_lightsOnFactor = float((off * day - t) / day);
        return;
    }
    if (off * day <= t && (on - 1.0) * day > t) {
        ms_lightsOnFactor = 0.0f;
        return;
    }
    if ((on - 1.0) * day <= t && on * day > t) {
        ms_lightsOnFactor = float(1.0 - (on * day - t) / day);
        return;
    }
    if (t >= on * day && 1.0f >= t) {
        ms_lightsOnFactor = 1.0f;
    }
}

} // namespace CCoronas

namespace LightShaftsFns {

// 0x513190 (thiscall): none this frame, the models requested (their instances are
// made when first drawn)
static void __fastcall Init(LightShafts* self, re::Edx) {
    self->count = 0;
    self->shaftClump = nullptr;
    RequestModel(ms_shaftModel, 5);
    RequestModel(ms_shaftModel2, 5);
    self->clump1EC = nullptr;
}

// 0x5131D0 (thiscall): the two instances destroyed (with their bones)
static void __fastcall Shutdown(LightShafts* self, re::Edx) {
    if (self->clump1EC) {
        void* bone = Node_GetRoot(self->clump1EC);
        Node_AttachBone(self->clump1EC, nullptr);
        Bone_Destroy(bone);
        Node_Destroy(self->clump1EC);
        self->clump1EC = nullptr;
    }
    if (self->shaftClump) {
        void* bone = Node_GetRoot(self->shaftClump);
        Node_AttachBone(self->shaftClump, nullptr);
        Bone_Destroy(bone);
        Node_Destroy(self->shaftClump);
        self->shaftClump = nullptr;
    }
}

// 0x513260 (thiscall): one more shaft this frame (up to 5), its first two colours
// doubled (to at most 1), with two coronas at its start when it points at the camera
static void __fastcall Add(LightShafts* self, re::Edx, const LightShaft* shaft) {
    if (self->count >= 5) {
        return;
    }
    LightShaft& s = self->shafts[self->count];
    s = *shaft;
    for (int i = 0; i < 8; i++) {
        s.colours[i] = float(s.colours[i] * 2.0);
        if (1.0f < s.colours[i]) {
            s.colours[i] = 1.0f;
        }
    }
    self->count++;

    LsVector camera = CameraPos();
    LsVector toCamera;
    VectorSub(&toCamera, &camera, &shaft->pos);
    Vector_Normalise(&toCamera);
    LsVector away = {-toCamera.x, -toCamera.y, -toCamera.z};
    float dot = float(double(shaft->dir.y) * away.y + double(shaft->dir.x) * away.x + double(shaft->dir.z) * away.z);
    if (!(dot < -0.800000011920929)) {
        return;
    }
    if (-1.0f > dot) {
        dot = -1.0f;
    }
    float angle = float(std::fabs(ACos(dot)));
    uint8_t r = ColourByte(double(shaft->colours[8]) * 255.0);
    uint8_t g = ColourByte(double(shaft->colours[9]) * 255.0);
    uint8_t b = ColourByte(double(shaft->colours[10]) * 255.0);
    uint8_t a = ColourByte(double(shaft->alpha) * shaft->colours[11] * 255.0);
    Coronas_Register(reinterpret_cast<uintptr_t>(self) + self->count, r, g, b, a, &shaft->pos, 0.65f, 45.0f, 3, 0, 0, 0,
                     angle, 0, 1, 1.5f);
    float size = float((1.0 - dot) * 0.699999988079071);
    Coronas_Register(reinterpret_cast<uintptr_t>(self), r, g, b, a, &shaft->pos, size, 50.0f, 7, 0, 0, 1, angle, 0, 1,
                     1.5f);
}

// 0x5136E0 (thiscall): the constructor (the shafts' positions and directions zeroed)
static LightShafts* __fastcall Construct(LightShafts* self, re::Edx) {
    for (LightShaft& s : self->shafts) {
        s.pos = {0.0f, 0.0f, 0.0f};
        s.dir = {0.0f, 0.0f, 0.0f};
    }
    return self;
}

// 0x513780 (thiscall): this frame's shafts drawn (two copies of the shaft model for
// each, turned along the shaft and towards the camera) and cleared
static void __fastcall Render(LightShafts* self, re::Edx) {
    int32_t savedSrcBlend;
    int32_t savedDestBlend;
    GetRenderState(10, &savedSrcBlend);
    GetRenderState(11, &savedDestBlend);
    if (!UpdateClump(self->shaftClump, ms_modelInfoPtrs[ms_shaftModel])) {
        return;   // (the render states were only read so far)
    }
    if (!self->bone1E8) {
        self->bone1E8 = NewBone();
    }
    if (!UpdateClump(self->clump1EC, ms_modelInfoPtrs[ms_shaftModel2])) {
        return;
    }

    for (int i = 0; i < self->count; i++) {
        LightShaft& s = self->shafts[i];
        ShaftSlot& slot = ms_shaftSlots[i];
        Renderer_Empty(TheRenderer);
        Renderer_ResetState(TheRenderer);
        float colour[4] = {s.colours[0] * s.alpha, s.colours[1] * s.alpha, s.colours[2] * s.alpha,
                           s.colours[3] * s.alpha};
        Renderer_SetColour(TheRenderer, colour);
        if (!slot.outer) {
            // (both copies are of the first model's instance)
            slot.outer = static_cast<uint8_t*>(Node_Clone(self->shaftClump));
            slot.inner = static_cast<uint8_t*>(Node_Clone(self->shaftClump));
            Node_AttachBone(slot.outer, NewBone());
            Node_AttachBone(slot.inner, NewBone());
            void* memory = Malloc(0x1C);
            uint8_t* effect = nullptr;
            if (memory) {
                ShaftEffect_Construct(memory, 0);
                effect = static_cast<uint8_t*>(memory);
            }
            slot.effect = effect;
            // (a failed allocation is read through, as in the game)
            uint8_t* old = vc::At<uint8_t*>(effect, 0x10);
            if (old) {
                if (vc::Decrement(old + 4) == 0) {
                    vc::Call<void>(old, 0x04);
                }
                vc::At<uint8_t*>(effect, 0x10) = nullptr;
            }
            ShaftEffect_Add(slot.effect, vc::At<void*>(slot.outer, 0x14));
            ShaftEffect_Add(slot.effect, vc::At<void*>(slot.inner, 0x14));
            AVObject_UpdateEffects(vc::At<void*>(slot.outer, 0x14));
            AVObject_UpdateEffects(vc::At<void*>(slot.inner, 0x14));
        }

        void* outerFrame = Node_GetRoot(slot.outer);
        LsTransform* xf = Frame_GetTransform(outerFrame);
        LsVector dir = s.dir;
        if (0.0f == dir.x && 0.0f == dir.y) {
            dir.x = dir.y = 0.0001f;
        }
        LsVector camera = CameraPos();
        LsVector toCamera;
        VectorSub(&toCamera, &camera, &s.pos);
        Vector_Normalise(&toCamera);
        LsVector side;
        CrossProduct(&side, &dir, &toCamera);
        Vector_Normalise(&side);
        LsVector up;
        CrossProduct(&up, &side, &dir);
        Vector_Normalise(&up);
        float along = float(s.intensity * s.unknown20);
        float width = float(double(along) / s.unknown20 * s.unknown18);
        width = width + width;
        LsVector pos = s.pos;
        LsTransform turn;
        turn.scale = 1.0f;
        Matrix_MakeXRotation(turn.rot, 1.57079637f);
        turn.trans = NiPoint3_ZERO;

        float length = s.length;
        LsVector upW = {up.x * width, up.y * width, up.z * width};
        xf->rot[0] = upW.x * length;
        xf->rot[3] = upW.y * length;
        xf->rot[6] = upW.z * length;
        xf->rot[2] = along * dir.x;
        xf->rot[5] = along * dir.y;
        xf->rot[8] = along * dir.z;
        LsVector sideW = {side.x * width, side.y * width, side.z * width};
        xf->trans = pos;
        xf->rot[1] = sideW.x * length;
        xf->rot[4] = sideW.y * length;
        xf->rot[7] = sideW.z * length;
        LsTransform product;
        *xf = *Transform_Multiply(xf, &product, &turn);
        Frame_Update(outerFrame);
        Fn_6C6B90(slot.outer);
        SetRenderState(6, 1);
        SetRenderState(8, 0);
        SetRenderState(0x14, 1);
        SetRenderState(0x1D, 8);
        SetRenderState(10, 5);
        SetRenderState(11, 2);
        ms_bRenderFlagD14550 = 1;
        ShaftEffect_Apply(slot.effect);
        SetObjectWorldTransform(slot.outer, Frame_GetTransform(Node_GetRoot(slot.outer)));
        Object_SetColour(vc::At<void*>(slot.outer, 0x14), colour[0], colour[1], colour[2], colour[3]);
        RenderObject(vc::At<void*>(slot.outer, 0x14), 1, 0);

        colour[0] = s.colours[4] * s.alpha;
        colour[1] = s.colours[5] * s.alpha;
        colour[2] = s.colours[6] * s.alpha;
        colour[3] = s.colours[7] * s.alpha;
        Renderer_SetColour(TheRenderer, colour);
        // the inner copy: the same without the length (and without the render states,
        // the frame update and the effect)
        xf = Frame_GetTransform(Node_GetRoot(slot.inner));
        xf->rot[0] = up.x * width;
        xf->rot[3] = up.y * width;
        xf->rot[6] = up.z * width;
        xf->rot[2] = along * dir.x;
        xf->rot[5] = along * dir.y;
        xf->rot[8] = along * dir.z;
        xf->trans = pos;
        xf->rot[1] = side.x * width;
        xf->rot[4] = side.y * width;
        xf->rot[7] = side.z * width;
        *xf = *Transform_Multiply(xf, &product, &turn);
        ms_bRenderFlagD14550 = 1;
        SetObjectWorldTransform(slot.inner, Frame_GetTransform(Node_GetRoot(slot.inner)));
        Object_SetColour(vc::At<void*>(slot.inner, 0x14), colour[0], colour[1], colour[2], colour[3]);
        RenderObject(vc::At<void*>(slot.inner, 0x14), 1, 0);
    }
    self->count = 0;
    SetRenderState(10, savedSrcBlend);
    SetRenderState(11, savedDestBlend);
}

} // namespace LightShaftsFns

#define C16A_LIGHTS_REVERSED(name, fn, address)                                                    \
    static void* Original_##fn = nullptr;                                                        \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

using namespace CCoronas;
using namespace LightShaftsFns;
C16A_LIGHTS_REVERSED("CCoronas::RenderPass0", RenderPass0, 0x00513010);
C16A_LIGHTS_REVERSED("CCoronas::RenderPass1", RenderPass1, 0x00513020);
C16A_LIGHTS_REVERSED("CCoronas::UpdateLightsOnFactor", UpdateLightsOnFactor, 0x00513030);
C16A_LIGHTS_REVERSED("LightShafts::Init", Init, 0x00513190);
C16A_LIGHTS_REVERSED("LightShafts::Shutdown", Shutdown, 0x005131D0);
C16A_LIGHTS_REVERSED("LightShafts::Add", Add, 0x00513260);
C16A_LIGHTS_REVERSED("LightShafts::LightShafts", Construct, 0x005136E0);
C16A_LIGHTS_REVERSED("LightShafts::Render", Render, 0x00513780);
