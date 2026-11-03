// The effect over the player's head (0x44DA40, 0x44DA90): an effect proxy of one of four
// kinds, placed 8 units above the player and faded in while nothing is overhead (a line
// test from 1 to 100 units above the player, or the camera when the player is far from
// it).

#include <cstdint>

#include "reversed.h"

namespace {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// The proxy's fields (the effect proxy's own come first)
struct COverheadEffect {
    re::Ptr32<void> vtable;    // +0x0
    re::Ptr32<void> effect;    // +0x4 (nullptr: not made yet)
    int32_t kind;    // +0x8 0 none, 1..4
    uint8_t clear;   // +0xC nothing overhead (the line test)
    uint8_t tinted;  // +0xD 1: drawn tinted (0x80FFFFFF)
    uint8_t unknownE[2];
    float fade;      // +0x10 0..1
};
static_assert(sizeof(COverheadEffect) == 0x14);

GAME_VAR(uint8_t*, oe_playerPed, 0x00C1AEA8);       // FindPlayerPed
GAME_VAR(void*, oe_ignoreEntity, 0x00C1AE70);       // the entity line tests skip
GAME_VAR(void*, oe_cameraSource, 0x00C3CCF4);
GAME_VAR(void*, oe_effectManager, 0x00B6F1D4);
GAME_VAR(int32_t, oe_currArea, 0x00BD1008);         // CGame::currArea
// the four kinds' effect names (hashes, 16 bits)
GAME_VAR(uint16_t, oe_kind1Name, 0x020C8FEC);
GAME_VAR(uint16_t, oe_kind2Name, 0x020C8FE8);
GAME_VAR(uint16_t, oe_kind3Name, 0x020C8FE4);
GAME_VAR(uint16_t, oe_kind4Name, 0x020C8FE0);

GAME_FN(float, __thiscall, Fn577300, 0x00577300, void* self);
GAME_FN(void*, __cdecl, Fn6C82F0, 0x006C82F0, void*);
GAME_FN(uint8_t*, __cdecl, Fn6C9A90, 0x006C9A90, void*);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const CVector* start, const CVector* end, void* colPoint,
        re::Ptr32<void>* entity, int flags, int, int);
GAME_FN(void, __cdecl, Entity_CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(uint8_t, __cdecl, Area_Fn430320, 0x00430320, int area);
GAME_FN(void, __thiscall, EffectManager_Create, 0x00668290, void* manager, void* name, void* proxy, int);
GAME_FN(void, __thiscall, Proxy_SetRange, 0x00668BD0, void* self, float, float, int);
GAME_FN(void, __thiscall, Proxy_FadeOut, 0x00668D00, void* self, float time);
GAME_FN(void, __thiscall, Proxy_Fn668BA0, 0x00668BA0, void* self);
GAME_FN(void, __cdecl, Vector_Add, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Proxy_SetPosition, 0x00669040, void* self, const CVector* pos, int);
GAME_FN(void, __thiscall, Proxy_SetAlpha_x64, 0x00668C40, void* self, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Proxy_SetAlpha(void* a0, float a1) {
    return Proxy_SetAlpha_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, Proxy_Fn668C70_x64, 0x00668C70, void* self, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Proxy_Fn668C70(void* a0, float a1) {
    return Proxy_Fn668C70_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, Proxy_Fn668C10_x64, 0x00668C10, void* self, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Proxy_Fn668C10(void* a0, float a1) {
    return Proxy_Fn668C10_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, Proxy_SetColour, 0x00668CA0, void* self, const uint32_t* colour);

const CVector& EntityPosition(uint8_t* entity) {
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x14);
    return *reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : entity + 4);
}

// 0x44DA40 (thiscall): the kind (a new kind fades the old one out first)
void __fastcall OverheadEffect_SetKind(COverheadEffect* self, re::Edx, int32_t kind, int tinted) {
    if (self->kind) {
        if (kind != self->kind) {
            self->fade = 0.0f;
            Proxy_FadeOut(self, 0.5f);
        }
        self->kind = kind;
        self->tinted = uint8_t(tinted);
        return;
    }
    self->tinted = uint8_t(tinted);
    self->kind = kind;
}

// 0x44DA90 (thiscall): once a frame
void __fastcall OverheadEffect_Update(COverheadEffect* self, re::Edx, float step) {
    bool hasKind = self->kind != 0;
    // (a dword local of which the game sets only the low 16 bits for the effect's name:
    // the rest is what the stack held, the distance limit below when the test ran)
    uint32_t name = 0;
    if (hasKind) {
        float fade = float(self->fade + 0.01);
        self->fade = fade;
        if (fade > 1.0f) {
            self->fade = 1.0f;
        }
        if (Fn577300(reinterpret_cast<void*>(0x00CF356C)) <= 0.25f) {
            uint8_t* player = oe_playerPed;
            CVector camera = *reinterpret_cast<const CVector*>(Fn6C9A90(Fn6C82F0(oe_cameraSource)) + 0x24);
            CVector playerPos = EntityPosition(player);
            float dx = playerPos.x - camera.x;
            float dy = playerPos.y - camera.y;
            float dz = playerPos.z - camera.z;
            float distSq = dy * dy + dx * dx + dz * dz;
            float limit = 31.0f;
            if (*reinterpret_cast<re::Ptr32<void>*>(player + 0x1554)) {   // in a vehicle
                limit = 53.0f;
            }
            name = *reinterpret_cast<uint32_t*>(&limit);
            CVector target = distSq <= limit ? playerPos : camera;
            float x = target.x + 0.0f;
            float y = target.y + 0.0f;
            void* saved = oe_ignoreEntity;
            CVector start = {x, y, float(target.z + 1.0)};
            CVector end = {x, y, float(target.z + 100.0)};
            // the colpoint (0x28 bytes: position, entity reference, normal, ..., depth)
            alignas(4) uint8_t colPoint[0x28];
            re::Ptr32<void>& hitEntity = *reinterpret_cast<re::Ptr32<void>*>(colPoint + 0xC);
            oe_ignoreEntity = player;
            hitEntity = nullptr;
            float* f = reinterpret_cast<float*>(colPoint);
            f[0] = f[1] = f[2] = 0.0f;
            f[4] = f[5] = f[6] = 0.0f;
            f[9] = *reinterpret_cast<const float*>(0x009009B8);   // FLT_MAX
            re::Ptr32<void> entity;
            self->clear = ProcessLineOfSight(&start, &end, colPoint, &entity, 0xF, 0, 0);
            oe_ignoreEntity = saved;
            if (hitEntity) {
                Entity_CleanUpOldReference(hitEntity, &hitEntity);
            }
        }
    }
    if (!self->clear && hasKind) {
        int32_t kind = self->kind;
        switch (kind) {
        case 1: name = (name & 0xFFFF0000) | oe_kind1Name; break;
        case 2: name = (name & 0xFFFF0000) | oe_kind2Name; break;
        case 3: name = (name & 0xFFFF0000) | oe_kind3Name; break;
        case 4: name = (name & 0xFFFF0000) | oe_kind4Name; break;
        }
        if (!self->effect) {
            float nearRange, farRange;
            if (kind == 4) {
                nearRange = 2.5f;
                farRange = 6.0f;
            } else {
                nearRange = 5.0f;
                farRange = 8.0f;
            }
            EffectManager_Create(oe_effectManager, &name, self, 0);
            Proxy_SetRange(self, nearRange, farRange, 0);
            if (!self->effect) {
                return;
            }
        }
    } else {
        if (!self->effect) {
            return;
        }
        if (!self->clear && Area_Fn430320(oe_currArea)) {
            float fade = float(self->fade - 0.01);
            self->fade = fade;
            if (fade < 0.0f) {
                self->fade = 0.0f;
                Proxy_FadeOut(self, 1.0f);
            }
        } else {
            Proxy_Fn668BA0(self);
        }
        if (!self->effect) {
            return;
        }
    }
    CVector pos = EntityPosition(oe_playerPed);
    CVector offset = {0.0f, 0.0f, 8.0f};
    CVector at;
    Vector_Add(&at, &pos, &offset);
    Proxy_SetPosition(self, &at, 0);
    float alpha = self->fade * step;
    if (self->tinted != 1) {
        Proxy_SetAlpha(self, alpha);
        return;
    }
    uint32_t colour = 0x80FFFFFF;
    Proxy_SetAlpha(self, alpha);
    Proxy_Fn668C70(self, 0.5f);
    Proxy_Fn668C10(self, 0.0f);
    Proxy_SetColour(self, &colour);
}

#define C04A_FX(name, address, fn)                                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04A_FX("COverheadEffect::SetKind", 0x0044DA40, &OverheadEffect_SetKind);
C04A_FX("COverheadEffect::Update", 0x0044DA90, &OverheadEffect_Update);

} // namespace
