// The entities' vf23 (GTA's SetupLighting): the renderer's ambient scale (a leftover: the
// setter is an empty stub now) and the point lights for the entity's position.

#include <cstdint>

#include "game/Entity.h"
#include "reversed.h"

namespace {

inline void* const renderState = reinterpret_cast<void*>(0x00D140B8);
// (empty: `ret 4`; the calls pass a float or a pointer to the colour)
GAME_FN(void, __thiscall, RenderState_SetAmbient, 0x008A20A0, void* state, float scale);
GAME_FN(void, __thiscall, RenderState_SetAmbientColour, 0x008A20A0, void* state, const void* colour);
GAME_FN(void, __thiscall, RenderState_Empty, 0x0049A650, void* state);   // (just a ret)
GAME_FN(void, __cdecl, SetLightsForPosition, 0x00522E50, const col::CVector* pos);

using re::At;

} // namespace

namespace EntityLighting {

// CDummy vf23 (0x43A370)
static void __fastcall Dummy_SetupLighting(CEntity*, re::Edx) {
    RenderState_Empty(renderState);
}

// CPropAnim vf23 (0x43A380): its colour (+0x264) when +0x258 is 1
static void __fastcall PropAnim_SetupLighting(CEntity* self, re::Edx) {
    if (At<uint8_t>(self, 0x258) == 1) {
        RenderState_SetAmbientColour(renderState, &At<uint8_t>(self, 0x264));
    } else {
        RenderState_SetAmbient(renderState, 1.0f);
    }
    RenderState_Empty(renderState);
}

// CObject vf23 (0x43A3C0): its colour (+0x210) with flag 0x10 of +0x209
static void __fastcall Object_SetupLighting(CEntity* self, re::Edx) {
    if (At<uint8_t>(self, 0x209) & 0x10) {
        RenderState_SetAmbientColour(renderState, &At<uint8_t>(self, 0x210));
    } else {
        RenderState_SetAmbient(renderState, 1.0f);
    }
    RenderState_Empty(renderState);
}

// CProjectile vf23 (0x43A400)
static void __fastcall Projectile_SetupLighting(CEntity*, re::Edx) {
    RenderState_SetAmbient(renderState, 1.0f);
    RenderState_Empty(renderState);
}

// CWeapon vf23 (0x43A420): and the lights at its position
static void __fastcall Weapon_SetupLighting(CEntity* self, re::Edx) {
    RenderState_SetAmbient(renderState, 1.0f);
    RenderState_Empty(renderState);
    SetLightsForPosition(&self->GetPosition());
}

// CAccessory vf23 (0x43A460): as the weapon's, unless +0x2DC
static void __fastcall Accessory_SetupLighting(CEntity* self, re::Edx) {
    if (At<int32_t>(self, 0x2DC) != 0) {
        return;
    }
    RenderState_SetAmbient(renderState, 1.0f);
    RenderState_Empty(renderState);
    SetLightsForPosition(&self->GetPosition());
}

// CCutsceneObject vf23 (0x43A4B0): the lights at its animated object's node (its world
// translation, +0x8C) when it has one, else at its position
static void __fastcall CutsceneObject_SetupLighting(CEntity* self, re::Edx) {
    RenderState_SetAmbient(renderState, 1.0f);
    RenderState_Empty(renderState);
    col::CVector pos = {0.0f, 0.0f, 0.0f};
    uint8_t* object = static_cast<uint8_t*>(self->animObject);
    if (uint8_t* part = *reinterpret_cast<re::Ptr32<uint8_t>*>(object + 0x14)) {
        uint8_t* node = *reinterpret_cast<re::Ptr32<uint8_t>*>(part + 8);
        pos.x = *reinterpret_cast<float*>(node + 0x8C);
        pos.y = *reinterpret_cast<float*>(node + 0x90);
        pos.z = *reinterpret_cast<float*>(node + 0x94);
    } else {
        pos = self->GetPosition();
    }
    SetLightsForPosition(&pos);
}

} // namespace EntityLighting

// ---- registration ----------------------------------------------------------------------

#define LIGHTING_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace EntityLighting {
LIGHTING_REVERSED("CDummy::SetupLighting", 0x0043A370, &Dummy_SetupLighting);
LIGHTING_REVERSED("CPropAnim::SetupLighting", 0x0043A380, &PropAnim_SetupLighting);
LIGHTING_REVERSED("CObject::SetupLighting", 0x0043A3C0, &Object_SetupLighting);
LIGHTING_REVERSED("CProjectile::SetupLighting", 0x0043A400, &Projectile_SetupLighting);
LIGHTING_REVERSED("CWeapon::SetupLighting", 0x0043A420, &Weapon_SetupLighting);
LIGHTING_REVERSED("CAccessory::SetupLighting", 0x0043A460, &Accessory_SetupLighting);
LIGHTING_REVERSED("CCutsceneObject::SetupLighting", 0x0043A4B0, &CutsceneObject_SetupLighting);
} // namespace EntityLighting
