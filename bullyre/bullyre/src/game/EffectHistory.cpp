// The recent effects' history (0x561040..0x5610C0): a ring of the last 32 effects
// started by type (the object at 0xCF1F60; explosions and projectiles add to it), with
// a count of those of a type inside an area.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct CVector {
    float x, y, z;
};

struct EffectHistory {
    int32_t next;        // +0 the slot the next one goes in
    struct Entry {       // (0x10 bytes)
        CVector pos;     // +0
        int16_t type;    // +0xC (0xFF: none)
        uint8_t padE[2];
    } entries[32];       // +4
};
static_assert(offsetof(EffectHistory, entries) == 4);
static_assert(sizeof(EffectHistory) == 0x204);

// an effect proxy (8 bytes; AreaEffects.cpp has it too)
struct EffectProxy {
    uint32_t words[2];
};

GAME_FN(uint8_t, __thiscall, Area_Contains, 0x006D6FA0, void* area, const CVector* point);
GAME_FN(float, __thiscall, Vector_Normalise, 0x004294A0, CVector* v);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Proxy_Construct, 0x00668E90, EffectProxy* proxy);
GAME_FN(void, __thiscall, Proxy_Destruct, 0x00668EB0, EffectProxy* proxy);
GAME_FN(void, __thiscall, Proxy_SetPosition, 0x00669040, EffectProxy* proxy, const CVector* pos, int);
GAME_FN(void, __thiscall, Proxy_SetDirection, 0x00669360, EffectProxy* proxy, const CVector* dir);
GAME_FN(void, __thiscall, EffectManager_Start, 0x00668290, void* manager, const void* type, EffectProxy* proxy, int);
GAME_VAR(void*, g_effectManager, 0x00B6F1D4);

// ---- 0x561040 (thiscall): emptied
void __fastcall History_Construct(EffectHistory* self, re::Edx) {
    self->next = 0;
    for (int i = 0; i < 32; i++) {
        self->entries[i].type = 0xFF;
    }
}

// ---- 0x561060 (thiscall): how many of a type are inside the area
int32_t __fastcall History_CountInArea(EffectHistory* self, re::Edx, int type, void* area) {
    int32_t count = 0;
    for (int i = 0; i < 32; i++) {
        if (self->entries[i].type == int16_t(type)) {
            CVector pos = self->entries[i].pos;
            if (Area_Contains(area, &pos)) {
                count++;
            }
        }
    }
    return count;
}

// ---- 0x5610C0 (thiscall): an effect of a type started a hundredth along the direction
// from the position, and remembered (the fourth argument isn't read)
void __fastcall History_Start(EffectHistory* self, re::Edx, int type, const CVector* pos, const CVector* dir, int) {
    CVector normal = *dir;
    Vector_Normalise(&normal);
    CVector offset = {float(double(dir->x) * 0.009999999776482582), float(double(dir->y) * 0.009999999776482582), float(double(dir->z) * 0.009999999776482582)};
    CVector at;
    VectorAdd(&at, pos, &offset);
    self->entries[self->next].pos = at;
    self->entries[self->next].type = int16_t(type);
    self->next++;
    if (self->next >= 32) {
        self->next = 0;
    }
    EffectProxy proxy;
    Proxy_Construct(&proxy);
    EffectManager_Start(g_effectManager, &type, &proxy, 0);
    Proxy_SetPosition(&proxy, &at, 0);
    Proxy_SetDirection(&proxy, &normal);
    Proxy_Destruct(&proxy);
}

} // namespace

#define EFFHIST_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(effhist_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(effhist_original_, __LINE__), \
             re::Mode::Original)

EFFHIST_REVERSED("EffectHistory::EffectHistory", 0x00561040, &History_Construct);
EFFHIST_REVERSED("EffectHistory::CountInArea", 0x00561060, &History_CountInArea);
EFFHIST_REVERSED("EffectHistory::Start", 0x005610C0, &History_Start);
