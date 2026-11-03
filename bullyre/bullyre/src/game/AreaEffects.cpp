// The area effects (0xC13FD8): up to 500 particle effects registered by the scripts
// with an area each (ids 0..499, running while their area is the current one, the
// night-only ones only at night), and up to 200 made at once (ids from 501 on,
// running until killed). And the bank of 12 effect proxies at 0xC13F70.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

// An effect proxy (8 bytes; 0x668E90 makes one, 0x668EB0 destroys it): its running
// effect at +4
struct AeEffectProxy {
    re::Ptr32<void> vtable;   // +0
    re::Ptr32<void> effect;   // +4 (nullptr: not running)
};
static_assert(sizeof(AeEffectProxy) == 8);

struct AeVector {
    float x, y, z;
};

// A registered effect (0x1C)
struct AeEffectDef {
    uint8_t flags;        // +0 bit 0 used, bit 1 night only, bits 2-7 the area
    uint8_t unknown1;     // +1
    uint16_t effectType;  // +2 (the library's index for its name)
    AeVector pos;         // +4
    AeVector dir;         // +0x10
};
static_assert(sizeof(AeEffectDef) == 0x1C);

// A running one (0xC): the id it was started for
struct AeEffectSlot {
    int32_t id;            // +0
    AeEffectProxy proxy;   // +4
};
static_assert(sizeof(AeEffectSlot) == 0xC);

struct CAreaEffects {
    uint8_t wasNight;              // +0 (what the clock said last update)
    uint8_t pad1[3];               // +1
    int32_t nextId;                // +4 (ids of made effects, from 501)
    int32_t area;                  // +8
    AeEffectDef defs[500];         // +0xC
    int32_t numActive;             // +0x36BC
    AeEffectSlot active[150];      // +0x36C0 the current area's
    AeEffectSlot made[200];        // +0x3DC8
};
static_assert(offsetof(CAreaEffects, numActive) == 0x36BC);
static_assert(offsetof(CAreaEffects, active) == 0x36C0);
static_assert(offsetof(CAreaEffects, made) == 0x3DC8);
static_assert(sizeof(CAreaEffects) == 0x4728);

// the bank at 0xC13F70 (0x64): proxies in groups of 5, 2, 1, 3, 1
struct CEffectProxyBank {
    int32_t unknown0;               // +0 (-1)
    AeEffectProxy group0[5];        // +4
    AeEffectProxy group1[2];        // +0x2C
    AeEffectProxy group2;           // +0x3C
    AeEffectProxy group3[3];        // +0x44
    AeEffectProxy group4;           // +0x5C
};
static_assert(sizeof(CEffectProxyBank) == 0x64);

GAME_VAR(void*, ae_effectLibrary, 0x00B6F1E8);
GAME_VAR(void*, ae_effectManager, 0x00B6F1D4);
GAME_VAR(uint8_t, ae_bNoAreaEffects, 0x00BF3808);
inline void* const ae_areaSounds = reinterpret_cast<void*>(0x00CE7580);
inline void* const ae_areaThing = reinterpret_cast<void*>(0x00BD5C90);

GAME_FN(void, __thiscall, AE_Proxy_Construct, 0x00668E90, AeEffectProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Destruct, 0x00668EB0, AeEffectProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Kill, 0x00668BA0, AeEffectProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_668C40_x64, 0x00668C40, AeEffectProxy* proxy, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AE_Proxy_668C40(AeEffectProxy* a0, float a1) {
    return AE_Proxy_668C40_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, AE_Proxy_SlowKill, 0x00668D00, AeEffectProxy* proxy, float time);
GAME_FN(void, __thiscall, AE_Proxy_SetPosition, 0x00669040, AeEffectProxy* proxy, const AeVector* pos, int);
GAME_FN(void, __thiscall, AE_Proxy_SetDirection, 0x00669360, AeEffectProxy* proxy, const AeVector* dir);
GAME_FN(uint16_t, __thiscall, AE_Library_Find, 0x00667B60, void* library, const char* name);
GAME_FN(void, __thiscall, AE_Manager_Start, 0x00668290, void* manager, const void* effectType, AeEffectProxy* proxy, int);
GAME_FN(uint8_t, __cdecl, AE_IsNight, 0x00426000);
GAME_FN(void, __thiscall, AE_AreaSounds_SetArea, 0x00537BD0, void* self, int area);
GAME_FN(void, __thiscall, AE_Area_43A2C0, 0x0043A2C0, void* self, int area);
GAME_FN(void, __thiscall, AE_Area_439D30, 0x00439D30, void* self);
GAME_FN(void, __thiscall, AE_Area_43A030, 0x0043A030, void* self);
// ours, called where modes apply
GAME_FN(int, __thiscall, AE_FindSlot, 0x00453BF0, CAreaEffects* self, int id);
GAME_FN(void, __thiscall, AE_KillNightOnly, 0x00453B50, CAreaEffects* self);
GAME_FN(void, __thiscall, AE_KillActive, 0x00453BA0, CAreaEffects* self);
GAME_FN(void, __thiscall, AE_RestartNightOnly, 0x00454140, CAreaEffects* self);

// (inlined everywhere) the slot of an id FindSlot found
inline AeEffectSlot* SlotOf(CAreaEffects* self, int id, int index) {
    return id < 500 ? &self->active[index] : &self->made[index];
}

// the start of a registered effect (its position and direction set) in the next
// active slot
inline void StartActive(CAreaEffects* self, int index, AeEffectDef* def) {
    AeEffectSlot* slot = &self->active[self->numActive];
    slot->id = index;
    AE_Manager_Start(ae_effectManager, &def->effectType, &self->active[self->numActive].proxy, 0);
    AeVector pos = def->pos;
    AE_Proxy_SetPosition(&self->active[self->numActive].proxy, &pos, 0);
    AeVector dir = def->dir;
    AE_Proxy_SetDirection(&self->active[self->numActive].proxy, &dir);
    self->numActive++;
}

} // namespace

// ---- the proxy bank -------------------------------------------------------------------

namespace EffectProxyBank {

// 0x453A60 (thiscall)
CEffectProxyBank* __fastcall Construct(CEffectProxyBank* self, re::Edx) {
    self->unknown0 = -1;
    for (int i = 0; i < 5; i++) {
        AE_Proxy_Construct(&self->group0[i]);
    }
    for (int i = 0; i < 2; i++) {
        AE_Proxy_Construct(&self->group1[i]);
    }
    AE_Proxy_Construct(&self->group2);
    for (int i = 0; i < 3; i++) {
        AE_Proxy_Construct(&self->group3[i]);
    }
    AE_Proxy_Construct(&self->group4);
    return self;
}

// 0x453AD0 (thiscall): in reverse
void __fastcall Destruct(CEffectProxyBank* self, re::Edx) {
    AE_Proxy_Destruct(&self->group4);
    for (int i = 2; i >= 0; i--) {
        AE_Proxy_Destruct(&self->group3[i]);
    }
    AE_Proxy_Destruct(&self->group2);
    for (int i = 1; i >= 0; i--) {
        AE_Proxy_Destruct(&self->group1[i]);
    }
    for (int i = 4; i >= 0; i--) {
        AE_Proxy_Destruct(&self->group0[i]);
    }
}

} // namespace EffectProxyBank

// ---- the area effects -----------------------------------------------------------------

namespace AreaEffects {

// 0x453B50: the running night-only ones stopped (daybreak)
void __fastcall KillNightOnly(CAreaEffects* self, re::Edx) {
    for (int i = 0; i < self->numActive; i++) {
        if (self->defs[self->active[i].id].flags & 2) {
            AE_Proxy_Kill(&self->active[i].proxy);
        }
    }
}

// 0x453BA0: every running one of the area stopped
void __fastcall KillActive(CAreaEffects* self, re::Edx) {
    for (int i = 0; i < self->numActive; i++) {
        AE_Proxy_Kill(&self->active[i].proxy);
    }
    self->numActive = 0;
}

// 0x453BF0: the slot running an id (-1: none); the last one of the id when several
int __fastcall FindSlot(CAreaEffects* self, re::Edx, int id) {
    int found = -1;
    if (id < 500) {
        for (int i = 0; i < self->numActive; i++) {
            if (self->active[i].id == id) {
                found = i;
            }
        }
        return found;
    }
    for (int i = 0; i < 200; i++) {
        if (self->made[i].id == id) {
            found = i;
        }
    }
    return found;
}

// 0x453C70 (thiscall): the constructor
CAreaEffects* __fastcall Construct(CAreaEffects* self, re::Edx) {
    self->wasNight = 1;
    self->nextId = 501;
    self->area = -1;
    for (int i = 0; i < 500; i++) {
        self->defs[i].flags &= ~1;
    }
    self->numActive = 0;
    for (int i = 0; i < 150; i++) {
        AE_Proxy_Construct(&self->active[i].proxy);
    }
    for (int i = 0; i < 200; i++) {
        AE_Proxy_Construct(&self->made[i].proxy);
    }
    return self;
}

// 0x453CF0 (thiscall): the destructor
void __fastcall Destruct(CAreaEffects* self, re::Edx) {
    for (int i = 199; i >= 0; i--) {
        AE_Proxy_Destruct(&self->made[i].proxy);
    }
    for (int i = 149; i >= 0; i--) {
        AE_Proxy_Destruct(&self->active[i].proxy);
    }
}

// 0x453D40: everything forgotten and stopped (a new game)
void __fastcall Reset(CAreaEffects* self, re::Edx) {
    AE_Area_439D30(ae_areaThing);
    AE_Area_43A030(ae_areaThing);
    for (int i = 0; i < 500; i++) {
        self->defs[i].flags &= ~1;
    }
    for (int i = 0; i < self->numActive; i++) {
        AE_Proxy_Kill(&self->active[i].proxy);
    }
    self->numActive = 0;
    for (int i = 0; i < 200; i++) {
        AE_Proxy_Kill(&self->made[i].proxy);
    }
    self->wasNight = 1;
}

// 0x453DD0: an effect started at pos in the first free made slot, with the next
// id; the id (-1: no free slot)
int __fastcall Create(CAreaEffects* self, re::Edx, const char* name, const AeVector* pos) {
    int id = -1;
    for (int i = 0; i < 200; i++) {
        AeEffectSlot* slot = &self->made[i];
        if (id != -1 || slot->proxy.effect != nullptr) {
            continue;
        }
        uint32_t type = AE_Library_Find(ae_effectLibrary, name);
        AE_Manager_Start(ae_effectManager, &type, &slot->proxy, 0);
        AeVector p = *pos;
        AE_Proxy_SetPosition(&slot->proxy, &p, 0);
        slot->id = self->nextId;
        id = self->nextId;
        self->nextId++;
    }
    return id;
}

// 0x453E70
uint8_t __fastcall IsRunning(CAreaEffects* self, re::Edx, int id) {
    int index = AE_FindSlot(self, id);
    if (index < 0) {
        return 0;
    }
    AeEffectSlot* slot = SlotOf(self, id, index);
    return slot && slot->proxy.effect != nullptr;
}

// 0x453EC0: a running one moved (1), and a registered one's position kept
uint8_t __fastcall SetPosition(CAreaEffects* self, re::Edx, int id, const AeVector* pos) {
    uint8_t result = 0;
    int index = AE_FindSlot(self, id);
    if (index >= 0) {
        AeEffectSlot* slot = SlotOf(self, id, index);
        if (slot && slot->proxy.effect) {
            AeVector p = *pos;
            AE_Proxy_SetPosition(&slot->proxy, &p, 0);
            result = 1;
        }
    }
    if (id < 500) {
        self->defs[id].pos = *pos;
    }
    return result;
}

// 0x453F60: the same for the direction
uint8_t __fastcall SetDirection(CAreaEffects* self, re::Edx, int id, const AeVector* dir) {
    uint8_t result = 0;
    int index = AE_FindSlot(self, id);
    if (index >= 0) {
        AeEffectSlot* slot = SlotOf(self, id, index);
        if (slot && slot->proxy.effect) {
            AE_Proxy_SetDirection(&slot->proxy, dir);
            result = 1;
        }
    }
    if (id < 500) {
        self->defs[id].dir = *dir;
    }
    return result;
}

// 0x453FE0: a registered one night only or not; a running one stopped when it
// becomes night only by day. 1 when it's running
uint8_t __fastcall SetNightOnly(CAreaEffects* self, re::Edx, int id, int nightOnly) {
    AeEffectSlot* slot = nullptr;
    int index = AE_FindSlot(self, id);
    if (index >= 0) {
        slot = SlotOf(self, id, index);
    }
    uint8_t flag = uint8_t(nightOnly);
    if (id < 500) {
        uint8_t& flags = self->defs[id].flags;
        flags ^= (uint8_t(flag * 2) ^ flags) & 2;
    }
    if (!slot || !slot->proxy.effect) {
        return 0;
    }
    if (!AE_IsNight() && flag) {
        AE_Proxy_Kill(&slot->proxy);
    }
    return 1;
}

// 0x454070: a made one stopped
void __fastcall Kill(CAreaEffects* self, re::Edx, int id) {
    int index = AE_FindSlot(self, id);
    if (index >= 0 && id >= 500) {
        AE_Proxy_Kill(&self->made[index].proxy);
    }
}

// 0x4540A0: a made one faded out
void __fastcall SlowKill(CAreaEffects* self, re::Edx, int id, float time, int flag) {
    int index = AE_FindSlot(self, id);
    if (index >= 0 && id >= 500) {
        if (uint8_t(flag) == 1) {
            AE_Proxy_668C40(&self->made[index].proxy, 0.0f);
        }
        AE_Proxy_SlowKill(&self->made[index].proxy, time);
    }
}

// 0x454100: a registered one stopped and forgotten (its entry back). (The game
// clears the used bit of defs[id] whatever the id, past the array for made ones)
AeEffectDef* __fastcall UnregisterInArea(CAreaEffects* self, re::Edx, int id) {
    int index = AE_FindSlot(self, id);
    if (index >= 0 && id < 500) {
        AE_Proxy_Kill(&self->active[index].proxy);
    }
    AeEffectDef* def = reinterpret_cast<AeEffectDef*>(reinterpret_cast<uint8_t*>(self) + 0xC + id * 0x1C);
    def->flags &= ~1;
    return def;
}

// 0x454140: the night-only ones not running started again (nightfall)
void __fastcall RestartNightOnly(CAreaEffects* self, re::Edx) {
    for (int i = 0; i < self->numActive; i++) {
        AeEffectSlot* slot = &self->active[i];
        AeEffectDef* def = &self->defs[slot->id];
        if ((def->flags & 2) && slot->proxy.effect == nullptr) {
            AE_Manager_Start(ae_effectManager, &def->effectType, &slot->proxy, 0);
            AeVector pos = self->defs[slot->id].pos;
            AE_Proxy_SetPosition(&slot->proxy, &pos, 0);
            AeVector dir = self->defs[slot->id].dir;
            AE_Proxy_SetDirection(&slot->proxy, &dir);
        }
    }
}

// 0x454240: the area changed: the area's sounds and 0xBD5C90 told, and (for
// another area, or none running) the old ones stopped and the new area's started
void __fastcall SetArea(CAreaEffects* self, re::Edx, int area) {
    self->wasNight = 1;
    AE_AreaSounds_SetArea(ae_areaSounds, area);
    AE_Area_43A2C0(ae_areaThing, area);
    if (self->area == area && self->numActive != 0) {
        return;
    }
    AE_KillActive(self);
    self->area = area;
    if (ae_bNoAreaEffects) {
        return;
    }
    for (int i = 0; i < 500; i++) {
        AeEffectDef* def = &self->defs[i];
        if ((def->flags & 1) && int(def->flags >> 2) == area) {
            StartActive(self, i, def);
        }
    }
}

// 0x454380: an effect registered in the first free entry (at pos, direction +y),
// started when its area is the current one; its id (-1: none free)
int __fastcall RegisterInArea(CAreaEffects* self, re::Edx, const char* name, const AeVector* pos, int area) {
    int id = -1;
    for (int i = 0; i < 500; i++) {
        AeEffectDef* def = &self->defs[i];
        if (id != -1 || (def->flags & 1)) {
            continue;
        }
        def->flags = uint8_t(uint8_t(area) * 4) | 1;
        id = i;
        def->effectType = AE_Library_Find(ae_effectLibrary, name);
        def->pos = *pos;
        def->dir.x = 0.0f;
        def->dir.y = 1.0f;
        def->dir.z = 0.0f;
        if (area == self->area) {
            StartActive(self, i, def);
        }
    }
    return id;
}

// 0x454500: night-only ones stopped or started when the clock goes over
void __fastcall Update(CAreaEffects* self, re::Edx) {
    if (self->wasNight != AE_IsNight()) {
        if (AE_IsNight()) {
            AE_RestartNightOnly(self);
            self->wasNight = AE_IsNight();
            return;
        }
        AE_KillNightOnly(self);
    }
    self->wasNight = AE_IsNight();
}

} // namespace AreaEffects

// ---- registration ---------------------------------------------------------------------

#define C04C_AE_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04C_AE_REVERSED("EffectProxyBank::Construct", 0x00453A60, &EffectProxyBank::Construct);
C04C_AE_REVERSED("EffectProxyBank::Destruct", 0x00453AD0, &EffectProxyBank::Destruct);
C04C_AE_REVERSED("CAreaEffects::KillNightOnly", 0x00453B50, &AreaEffects::KillNightOnly);
C04C_AE_REVERSED("CAreaEffects::KillActive", 0x00453BA0, &AreaEffects::KillActive);
C04C_AE_REVERSED("CAreaEffects::FindSlot", 0x00453BF0, &AreaEffects::FindSlot);
C04C_AE_REVERSED("CAreaEffects::Construct", 0x00453C70, &AreaEffects::Construct);
C04C_AE_REVERSED("CAreaEffects::Destruct", 0x00453CF0, &AreaEffects::Destruct);
C04C_AE_REVERSED("CAreaEffects::Reset", 0x00453D40, &AreaEffects::Reset);
C04C_AE_REVERSED("CAreaEffects::Create", 0x00453DD0, &AreaEffects::Create);
C04C_AE_REVERSED("CAreaEffects::IsRunning", 0x00453E70, &AreaEffects::IsRunning);
C04C_AE_REVERSED("CAreaEffects::SetPosition", 0x00453EC0, &AreaEffects::SetPosition);
C04C_AE_REVERSED("CAreaEffects::SetDirection", 0x00453F60, &AreaEffects::SetDirection);
C04C_AE_REVERSED("CAreaEffects::SetNightOnly", 0x00453FE0, &AreaEffects::SetNightOnly);
C04C_AE_REVERSED("CAreaEffects::Kill", 0x00454070, &AreaEffects::Kill);
C04C_AE_REVERSED("CAreaEffects::SlowKill", 0x004540A0, &AreaEffects::SlowKill);
C04C_AE_REVERSED("CAreaEffects::UnregisterInArea", 0x00454100, &AreaEffects::UnregisterInArea);
C04C_AE_REVERSED("CAreaEffects::RestartNightOnly", 0x00454140, &AreaEffects::RestartNightOnly);
C04C_AE_REVERSED("CAreaEffects::SetArea", 0x00454240, &AreaEffects::SetArea);
C04C_AE_REVERSED("CAreaEffects::RegisterInArea", 0x00454380, &AreaEffects::RegisterInArea);
C04C_AE_REVERSED("CAreaEffects::Update", 0x00454500, &AreaEffects::Update);
