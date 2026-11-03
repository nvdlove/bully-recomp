// CPlaceable's small accessors (inlined in most places, out of line here), and a few
// neighbours: CVector's += / -= and a copy normalised, the streaming load-state
// checks, CWorld's scan code.

#include <cmath>
#include <cstdint>

#include "game/Entity.h"
#include "reversed.h"

namespace c01placeable {

using col::CVector;

GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);   // (+4: the load state)
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(float, __thiscall, Vector_NormaliseSafe, 0x00415130, CVector* v);
GAME_VAR(uint16_t, ms_nCurrentScanCode, 0x00C1AE88);

float Sin(float a) {
    return static_cast<float>(std::sin(static_cast<double>(a)));
}
float Cos(float a) {
    return static_cast<float>(std::cos(static_cast<double>(a)));
}

// 0x415460 (thiscall): the position
CVector* __fastcall GetPosition(CPlaceable* self, re::Edx) {
    return &self->GetPosition();
}

// 0x415470 (thiscall): the heading (from the forward row when it has a matrix)
float __fastcall GetHeading(CPlaceable* self, re::Edx) {
    if (self->matrix) {
        return static_cast<float>(std::atan2(-static_cast<double>(self->matrix->m.m[4]), static_cast<double>(self->matrix->m.m[5])));
    }
    return self->placement.heading;
}

// 0x4154A0 (thiscall): the forward direction
CVector* __fastcall GetForward(CPlaceable* self, re::Edx, CVector* out) {
    if (self->matrix) {
        out->x = self->matrix->m.m[4];
        out->y = self->matrix->m.m[5];
        out->z = self->matrix->m.m[6];
        return out;
    }
    out->x = -Sin(self->placement.heading);
    out->y = Cos(self->placement.heading);
    out->z = 0.0f;
    return out;
}

// 0x415500 (thiscall): the entity reference at +0xC let go of
void __fastcall ClearReferenceC(uint8_t* self, re::Edx) {
    re::Ptr32<void>* ref = reinterpret_cast<re::Ptr32<void>*>(self + 0xC);
    if (*ref) {
        CleanUpOldReference(*ref, ref);
    }
}

// 0x415200 (thiscall): +=
void __fastcall Vector_Add(CVector* self, re::Edx, const CVector* v) {
    self->x = v->x + self->x;
    self->y = v->y + self->y;
    self->z = v->z + self->z;
}

// 0x415230 (thiscall): -=
void __fastcall Vector_Sub(CVector* self, re::Edx, const CVector* v) {
    self->x = self->x - v->x;
    self->y = self->y - v->y;
    self->z = self->z - v->z;
}

// 0x415440: out = v normalised (0x415130: zero when too short); its length
float __cdecl Normalised(CVector* out, const CVector* v) {
    *out = *v;
    return Vector_NormaliseSafe(out);
}

// 0x4151C0: GTA's LOADSTATE_REQUESTED
uint8_t __cdecl IsModelRequested(int id) {
    return GetStreamingInfo(id)[4] == 2;
}

// 0x4151E0: GTA's LOADSTATE_LOADED
uint8_t __cdecl HasModelLoaded(int id) {
    return GetStreamingInfo(id)[4] == 1;
}

// 0x415260: CWorld::AdvanceCurrentScanCode (every code cleared when it wraps)
void __cdecl AdvanceCurrentScanCode() {
    if (ms_nCurrentScanCode != 0xFFFF) {
        ms_nCurrentScanCode++;
        return;
    }
    ClearScanCodes();
    ms_nCurrentScanCode = 1;
}

} // namespace c01placeable

// ---- registration ----------------------------------------------------------------------

#define PLACEABLE_REVERSED_C01(name, address, fn)                                            \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01placeable {
PLACEABLE_REVERSED_C01("CPlaceable::GetPosition", 0x00415460, &GetPosition);
PLACEABLE_REVERSED_C01("CPlaceable::GetHeading", 0x00415470, &GetHeading);
PLACEABLE_REVERSED_C01("CPlaceable::GetForward", 0x004154A0, &GetForward);
PLACEABLE_REVERSED_C01("CEntityRef::Clear (+0xC)", 0x00415500, &ClearReferenceC);
PLACEABLE_REVERSED_C01("CVector::operator+=", 0x00415200, &Vector_Add);
PLACEABLE_REVERSED_C01("CVector::operator-=", 0x00415230, &Vector_Sub);
PLACEABLE_REVERSED_C01("CVector::Normalised", 0x00415440, &Normalised);
PLACEABLE_REVERSED_C01("CStreaming::IsModelRequested", 0x004151C0, &IsModelRequested);
PLACEABLE_REVERSED_C01("CStreaming::HasModelLoaded", 0x004151E0, &HasModelLoaded);
PLACEABLE_REVERSED_C01("CWorld::AdvanceCurrentScanCode", 0x00415260, &AdvanceCurrentScanCode);
} // namespace c01placeable
