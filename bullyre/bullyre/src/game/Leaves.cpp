// The falling leaves effects (Script::EffectAddLeavesInArea): up to 300 positions, listed
// per area; in the areas of chapters that have them (area props 1), 20 effect proxies
// play at the positions within 38 m of the camera. One object, at 0xBD5C90. Also the
// vector difference from the same place in the exe.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace {

// an effect proxy of the effects system (0xB6F1D4) and the slot's position
struct LeafSlot {
    re::Ptr32<void> vtable;          // +0 (EffectProxySetLocation's)
    re::Ptr32<void> handle;          // +4 the playing effect (nullptr: none)
    int32_t position;      // +8 the position it plays at
    uint8_t active;        // +0xC
    uint8_t unknownD[3];   // +0xD
};
static_assert(sizeof(LeafSlot) == 0x10);

struct LeafPosition {
    col::CVector pos;      // +0
    uint32_t slot;         // +0xC bits 0-6: the slot playing here
    uint8_t flags;         // +0x10 bit 0: a slot plays here
    uint8_t unknown11[3];  // +0x11
};
static_assert(sizeof(LeafPosition) == 0x14);

struct CLeafEffects {
    uint16_t effect;             // +0 the effect's id (0xFF when made)
    uint8_t unknown2[2];         // +2
    int32_t area;                // +4 the current area (-1: none)
    int32_t numPositions;        // +8
    int32_t ranges[64][2];       // +0xC each area's first and last position (-1: none)
    LeafSlot slots[20];          // +0x20C
    LeafPosition positions[300]; // +0x34C
    int32_t freeSlots[20];       // +0x1ABC (made by Update)
    float timer;                 // +0x1B0C (0x669FD0's)
};
static_assert(offsetof(CLeafEffects, ranges) == 0xC);
static_assert(offsetof(CLeafEffects, slots) == 0x20C);
static_assert(offsetof(CLeafEffects, positions) == 0x34C);
static_assert(offsetof(CLeafEffects, freeSlots) == 0x1ABC);
static_assert(offsetof(CLeafEffects, timer) == 0x1B0C);

GAME_VAR(uint8_t, ms_currentChapter, 0x00A147E0);
GAME_ARRAY(int32_t, ms_chapterAreaProps, 0x00A147EC);   // by chapter
GAME_VAR(uint16_t, ms_leavesEffect, 0x020C8FDC);
GAME_VAR(int32_t, g_rwCamera, 0x00C3CCF4);
GAME_VAR(void*, ms_pEffects, 0x00B6F1D4);

GAME_FN(void*, __cdecl, RwCameraGetFrame, 0x006C82F0, uintptr_t camera);
GAME_FN(float*, __cdecl, RwFrameGetLTM, 0x006C9A90, void* frame);   // (+0x24: the position)
GAME_FN(void, __thiscall, EffectProxy_Construct, 0x00668E90, LeafSlot* slot);
GAME_FN(void, __thiscall, EffectProxy_Stop, 0x00668BA0, LeafSlot* slot);
GAME_FN(void, __thiscall, EffectProxy_SetLocation, 0x00669040, LeafSlot* slot, const col::CVector* pos, int);
GAME_FN(void, __thiscall, Effects_Start, 0x00668290, void* effects, CLeafEffects* def, LeafSlot* slot, int);
GAME_FN(void, __thiscall, Timer_Construct, 0x00669FD0, float* timer);
GAME_FN(void, __thiscall, Timer_Reset, 0x00669FF0, float* timer);
GAME_FN(float, __thiscall, Timer_GetSeconds, 0x0066A010, float* timer);
// (CLeafEffects' own, called where modes apply)
GAME_FN(void, __thiscall, Leaves_StopAll, 0x00439D30, CLeafEffects* self);
GAME_FN(uint8_t, __thiscall, Leaves_HasPosition, 0x00439D80, CLeafEffects* self, const col::CVector* pos, int area);
GAME_FN(uint8_t, __thiscall, Leaves_IsNearCamera, 0x0043A050, CLeafEffects* self, int position);
GAME_FN(void, __thiscall, Leaves_Attach, 0x0043A110, CLeafEffects* self, int slot, int position);
GAME_FN(void, __thiscall, Leaves_Update, 0x0043A1E0, CLeafEffects* self);

// (x87: each float compare as the game's; equal floats only)
bool Same(const col::CVector& a, const col::CVector& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

} // namespace

namespace Leaves {

// 0x439D30 (thiscall): every slot stopped and the positions freed, no area
static void __fastcall StopAll(CLeafEffects* self, re::Edx) {
    for (int i = 0; i < 300; i++) {
        self->positions[i].flags &= ~1;
    }
    for (int i = 0; i < 20; i++) {
        self->slots[i].active = 0;
        EffectProxy_Stop(&self->slots[i]);
    }
    self->area = -1;
}

// 0x439D80 (thiscall): is the position one of the area's already
static uint8_t __fastcall HasPosition(CLeafEffects* self, re::Edx, const col::CVector* pos, int area) {
    int first = self->ranges[area][0];
    if (first == -1) {
        return 0;
    }
    uint8_t found = 0;
    for (int i = first; i <= self->ranges[area][1]; i++) {
        if (Same(self->positions[i].pos, *pos)) {
            found = 1;
        }
    }
    return found;
}

// 0x439F00 (thiscall): a position for the area (at the end of the list: an area's
// positions must be added together, its range runs from its first to its last)
static void __fastcall AddPosition(CLeafEffects* self, re::Edx, const col::CVector* pos, int area) {
    if (Leaves_HasPosition(self, pos, area)) {
        return;
    }
    if (self->ranges[area][0] == -1) {
        self->ranges[area][0] = self->numPositions;
    }
    self->ranges[area][1] = self->numPositions;
    self->positions[self->numPositions].pos = *pos;
    self->numPositions++;
}

// 0x439F60 (cdecl): CVector a - b, into `out` (which may be a or b)
static col::CVector* __cdecl VectorSub(col::CVector* out, const col::CVector* a, const col::CVector* b) {
    float x = a->x - b->x;
    float y = a->y - b->y;
    float z = a->z - b->z;
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}

// 0x439FB0 (thiscall): the constructor
static CLeafEffects* __fastcall Construct(CLeafEffects* self, re::Edx) {
    self->effect = 0xFF;
    self->area = -1;
    self->numPositions = 0;
    for (int i = 0; i < 64; i++) {
        self->ranges[i][0] = -1;
        self->ranges[i][1] = -1;
    }
    for (int i = 0; i < 20; i++) {
        EffectProxy_Construct(&self->slots[i]);
        self->slots[i].active = 0;
    }
    for (int i = 0; i < 300; i++) {
        self->positions[i].flags &= ~1;
    }
    Timer_Construct(&self->timer);
    return self;
}

// 0x43A030 (thiscall): leaving the area
static void __fastcall Clear(CLeafEffects* self, re::Edx) {
    Leaves_StopAll(self);
    self->area = -1;
}

// 0x43A050 (thiscall): the position within 38 m of the camera (2D; NaN: no)
static uint8_t __fastcall IsNearCamera(CLeafEffects* self, re::Edx, int position) {
    float* ltm = RwFrameGetLTM(RwCameraGetFrame(g_rwCamera));
    const col::CVector& pos = self->positions[position].pos;
    float dx = pos.x - ltm[9];
    float dy = pos.y - ltm[10];
    float dist2 = dy * dy + dx * dx;   // (the game adds them in this order)
    return dist2 <= 1444.0f;
}

// 0x43A110 (thiscall): the slot to the position (its effect started if it has none)
static void __fastcall Attach(CLeafEffects* self, re::Edx, int slot, int position) {
    LeafSlot& s = self->slots[slot];
    if (!s.handle) {
        Effects_Start(ms_pEffects, self, &s, 0);
    }
    if (s.active) {
        self->positions[s.position].flags &= ~1;
    }
    s.position = position;
    s.active = 1;
    LeafPosition& p = self->positions[position];
    p.flags |= 1;
    p.slot ^= (p.slot ^ uint32_t(slot)) & 0x7F;
    col::CVector pos = p.pos;
    EffectProxy_SetLocation(&s, &pos, 0);
}

// 0x43A1E0 (thiscall): the slots whose effects have stopped playing near the camera go
// to the positions near it that have none. (A slot still playing goes on the free list
// too; and the area's last position is never used: its range ends before it)
static void __fastcall Update(CLeafEffects* self, re::Edx) {
    int numFree = 0;
    for (int i = 0; i < 20; i++) {
        LeafSlot& s = self->slots[i];
        if (s.active && Leaves_IsNearCamera(self, s.position) && !s.handle) {
            Leaves_Attach(self, i, s.position);
            continue;
        }
        self->freeSlots[numFree++] = i;
    }
    // (the area's range read through the words from +0xC, as the game: area -1 reads +4)
    int32_t* ranges = &self->ranges[0][0];
    int used = 0;
    for (int i = ranges[self->area * 2]; i < ranges[self->area * 2 + 1]; i++) {
        if (used >= numFree) {
            continue;
        }
        LeafPosition& p = self->positions[i];
        if ((p.flags & 1) && self->slots[p.slot & 0x7F].handle) {
            continue;
        }
        if (Leaves_IsNearCamera(self, i)) {
            Leaves_Attach(self, self->freeSlots[used], i);
            used++;
        }
    }
}

// 0x43A2C0 (thiscall): the area entered (only chapters with area props 1 have leaves)
static void __fastcall SetArea(CLeafEffects* self, re::Edx, int area) {
    if (ms_chapterAreaProps[ms_currentChapter] != 1) {
        Leaves_StopAll(self);
        return;
    }
    if (self->area == area) {
        return;
    }
    Timer_Reset(&self->timer);
    self->effect = ms_leavesEffect;
    Leaves_StopAll(self);
    self->area = area;
    Leaves_Update(self);
}

// 0x43A320 (thiscall): every half second, an update
static void __fastcall Process(CLeafEffects* self, re::Edx) {
    if (ms_chapterAreaProps[ms_currentChapter] != 1 || self->area < 0) {
        return;
    }
    if (Timer_GetSeconds(&self->timer) > 0.5f) {
        Timer_Reset(&self->timer);
        Leaves_Update(self);
    }
}

} // namespace Leaves

// ---- registration ----------------------------------------------------------------------

#define LEAVES_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace Leaves {
LEAVES_REVERSED("CLeafEffects::StopAll", 0x00439D30, &StopAll);
LEAVES_REVERSED("CLeafEffects::HasPosition", 0x00439D80, &HasPosition);
LEAVES_REVERSED("CLeafEffects::AddPosition", 0x00439F00, &AddPosition);
LEAVES_REVERSED("CVector::operator-", 0x00439F60, &VectorSub);
LEAVES_REVERSED("CLeafEffects::Construct", 0x00439FB0, &Construct);
LEAVES_REVERSED("CLeafEffects::Clear", 0x0043A030, &Clear);
LEAVES_REVERSED("CLeafEffects::IsNearCamera", 0x0043A050, &IsNearCamera);
LEAVES_REVERSED("CLeafEffects::Attach", 0x0043A110, &Attach);
LEAVES_REVERSED("CLeafEffects::Update", 0x0043A1E0, &Update);
LEAVES_REVERSED("CLeafEffects::SetArea", 0x0043A2C0, &SetArea);
LEAVES_REVERSED("CLeafEffects::Process", 0x0043A320, &Process);
} // namespace Leaves
