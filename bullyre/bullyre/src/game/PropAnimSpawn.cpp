// A prop (CPropAnim) made for a placement trigger: the trigger's model (or one given),
// placed at the trigger's position turned by its three angles, added to the world
// with its collision in the col store's bounds.
//
// The trigger, as used here: +4 the position, +0x10 the heading (degrees, less +0x38 in
// radians), +0x14 and +0x18 the other two angles, +0x1C the prop made, +0x20 its
// placement (+0x10 the model), +0x3C a byte for the prop's +0x111.

#include <cstdint>
#include <cstdio>

#include "game/Camera.h"   // (cam::CVector, cam::NiTransform)
#include "reversed.h"

#define C13G_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

namespace {

using cam::CVector;
using cam::NiTransform;

constexpr double kPi = 3.1415927410125732;  // (float pi, as a double)

using re::At;

// vtable[index] of an object, as a thiscall
template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

// GTA's CMatrix: a RwMatrix and the transform it's attached to
struct PropMatrix {
    float right[3];    // +0
    uint32_t flags;    // +0xC
    float up[3];       // +0x10
    uint32_t pad1;     // +0x1C
    float at[3];       // +0x20
    uint32_t pad2;     // +0x2C
    float pos[3];      // +0x30
    uint32_t pad3;     // +0x3C
    re::Ptr32<void> attached;    // +0x40
};
static_assert(sizeof(PropMatrix) == 0x44);

GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(void*, ms_pPropAnimPool, 0x00C0F608);
GAME_VAR(void*, tempColModel, 0x00CF3120);   // the col model for models without one

GAME_FN(void*, __thiscall, PropAnimPool_New, 0x004EB100, void* pool);
GAME_FN(uint8_t*, __thiscall, PropAnim_Construct, 0x004EDB20, void* self, int model, void* trigger);
GAME_FN(void, __thiscall, PropAnim_Fn4EA450, 0x004EA450, void* self, int a, int b);
GAME_FN(void, __thiscall, PropAnim_Fn4EF400, 0x004EF400, void* self, int model, void* trigger);
GAME_FN(void, __thiscall, ModelInfo_SetColModel, 0x0050EB30, void* modelInfo, void* colModel, int owns);
GAME_FN(uint8_t, __thiscall, ModelInfo_Fn520440, 0x00520440, void* modelInfo);
GAME_FN(void, __cdecl, CombineRotation, 0x00415EB0, NiTransform* t, const CVector* axis, float degrees, int mode);
GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, NiTransform* self);
GAME_FN(NiTransform*, __thiscall, NiTransform_Multiply, 0x00406830, const NiTransform* self, NiTransform* out, const NiTransform* other);
GAME_FN(void, __thiscall, Matrix_FromTransform, 0x00412A70, PropMatrix* self, const NiTransform* t);
GAME_FN(void, __thiscall, Entity_SetMatrix, 0x0046DE90, void* entity, const PropMatrix* m, int force);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(uint8_t*, __cdecl, GetColModel, 0x0051AF50, void* entity);
GAME_FN(void*, __cdecl, ColStore_GetBoundingBox, 0x0056F0F0, int slot);
GAME_FN(void, __thiscall, Rect_Extend, 0x00414410, void* rect, const void* other);
GAME_FN(void, __cdecl, Door_SetPathableToPeds, 0x005EB060, void* entity, int pathable);

// 0x4EFFB0: the prop for a trigger (nullptr when the trigger has no model); `animArg`
// goes to 0x4EA450 with 0xD, `modelOverride` (-1: none) replaces the model (the model
// info checked for collision stays the trigger's), `flag` is the prop's +0x3A8
uint8_t* __cdecl CreatePropForTrigger(uint8_t* trigger, int animArg, int modelOverride, int flag) {
    NiTransform m;
    m.scale = 1.0f;
    int32_t model = At<int32_t>(At<uint8_t*>(trigger, 0x20), 0x10);
    if (model == -1) {
        return nullptr;
    }
    uint8_t* info = ms_modelInfoPtrs[model];
    if (modelOverride != -1) {
        model = modelOverride;
    }
    void* memory = PropAnimPool_New(ms_pPropAnimPool);
    uint8_t* prop = memory != nullptr ? PropAnim_Construct(memory, model, trigger) : nullptr;
    if (prop == nullptr) {
        // (the pool full (220): the game wrote through the null and stopped; a prop is left
        // out instead, as for a trigger without a model. Seen starting a race in the business
        // district with 137 props in already)
        static bool said = false;
        if (!said) {
            said = true;
            std::fprintf(stderr, "x64: the PropAnim pool is full: a trigger's prop left out\n");
        }
        return nullptr;
    }
    prop[0x3A8] = uint8_t(flag);
    PropAnim_Fn4EA450(prop, 0xD, animArg);
    At<uint8_t*>(trigger, 0x1C) = prop;
    if (At<void*>(info, 0xC) == nullptr) {
        ModelInfo_SetColModel(info, tempColModel, 0);
    }
    VCall<void>(prop, 9, model);
    CVector xAxis = {1.0f, 0.0f, 0.0f};
    CVector yAxis = {0.0f, 1.0f, 0.0f};
    CVector zAxis = {0.0f, 0.0f, 1.0f};
    float heading = At<float>(trigger, 0x10);
    float angleY = At<float>(trigger, 0x14);
    float angleX = At<float>(trigger, 0x18);
    float degrees = float(double(heading) - double(At<float>(trigger, 0x38)) * 180.0 / kPi);
    CombineRotation(&m, &zAxis, degrees, 0);
    CombineRotation(&m, &yAxis, angleY, 2);
    CombineRotation(&m, &xAxis, angleX, 2);
    NiTransform place;
    place.scale = 1.0f;
    NiTransform_MakeIdentity(&place);
    place.translate = At<CVector>(trigger, 4);
    NiTransform product;
    m = *NiTransform_Multiply(&place, &product, &m);
    // (a CMatrix attached to the transform; its flags and pads are the game's
    // uninitialised stack)
    PropMatrix matrix = {};
    matrix.attached = &m;
    Matrix_FromTransform(&matrix, &m);
    Entity_SetMatrix(prop, &matrix, 0);
    prop[0x111] = trigger[0x3C];
    World_Add(prop);
    uint16_t slot = At<uint16_t>(GetColModel(prop), 0x30);
    float bounds[4];
    void* rect = VCall<void*>(prop, 0x2C / 4, bounds);
    Rect_Extend(ColStore_GetBoundingBox(slot), rect);
    PropAnim_Fn4EF400(prop, model, trigger);
    if (ModelInfo_Fn520440(ms_modelInfoPtrs[At<int16_t>(prop, 0x10E)])) {
        Door_SetPathableToPeds(prop, At<int32_t>(prop, 0x180) == 0 ? 1 : 0);
    }
    return prop;
}
C13G_REVERSED("CPropAnim::CreateForTrigger", 0x004EFFB0, &CreatePropForTrigger);

} // namespace
