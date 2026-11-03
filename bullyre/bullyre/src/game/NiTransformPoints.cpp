// Points through a Gamebryo NiTransform (translate + scale * rotate * p), one or an
// array; a point moved along a segment to a height; a float ceil; the animation
// group's entry by index. (0x450B10 .. 0x451410)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Streaming.h"
#include "reversed.h"

namespace {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

struct NiTransform {
    float rotate[9];      // +0 (NiMatrix3)
    CVector translate;    // +0x24
    float scale;          // +0x30
};
static_assert(offsetof(NiTransform, translate) == 0x24);
static_assert(offsetof(NiTransform, scale) == 0x30);

// NiMatrix3 * NiPoint3 (the result by hidden pointer)
GAME_FN(CVector*, __thiscall, NiMatrix3_MultiplyPoint, 0x00768A70, const NiTransform* rotate, CVector* out, const CVector* p);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);

// 0x450B10 (thiscall): NiTransform * NiPoint3 (the result by hidden pointer)
CVector* __fastcall NiTransform_MultiplyPoint(const NiTransform* self, re::Edx, CVector* out, const CVector* p) {
    float scale = self->scale;
    CVector r;
    const CVector* rotated = NiMatrix3_MultiplyPoint(self, &r, p);
    float x = rotated->x * scale;
    float y = rotated->y * scale;
    float z = scale * rotated->z;
    out->x = self->translate.x + x;
    out->y = self->translate.y + y;
    out->z = self->translate.z + z;
    return out;
}

// 0x451410: `count` points through the transform
CVector* __cdecl NiTransform_MultiplyPoints(CVector* out, const CVector* in, int count, const NiTransform* t) {
    for (int i = 0; i < count; i++) {
        float scale = t->scale;
        CVector r;
        const CVector* rotated = NiMatrix3_MultiplyPoint(t, &r, &in[i]);
        float x = rotated->x * scale;
        float y = rotated->y * scale;
        float z = scale * rotated->z;
        CVector result;
        result.x = t->translate.x + x;
        result.y = t->translate.y + y;
        result.z = t->translate.z + z;
        out[i] = result;
    }
    return out;
}

// 0x451380: `p` moved along the line from `a` to where its z is `z`
void __cdecl MoveToHeight(CVector* p, const CVector* a, float z) {
    float t = (z - a->z) / (p->z - a->z);
    CVector d;
    const CVector* delta = VectorSub(&d, p, a);
    CVector scaled;
    scaled.y = t * delta->y;
    scaled.z = delta->z * t;
    scaled.x = t * delta->x;
    CVector r;
    *p = *VectorAdd(&r, a, &scaled);
}

// 0x450BA0: (the CRT's ceil, through a double)
float __cdecl CeilF(float value) {
    return float(std::ceil(double(value)));
}

// 0x450B80 (thiscall): an animation group's entry (nullptr past the end)
AgrEntry* __fastcall AgrGroup_GetEntry(AgrGroup* self, re::Edx, uint32_t index) {
    if (index < self->numEntries) {
        return &self->entries[index];
    }
    return nullptr;
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define C04B_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04B_REVERSED("NiTransform::MultiplyPoint", 0x00450B10, &NiTransform_MultiplyPoint);
C04B_REVERSED("AgrGroup::GetEntry", 0x00450B80, &AgrGroup_GetEntry);
C04B_REVERSED("CeilF", 0x00450BA0, &CeilF);
C04B_REVERSED("CVector::MoveToHeight", 0x00451380, &MoveToHeight);
C04B_REVERSED("NiTransform::MultiplyPoints", 0x00451410, &NiTransform_MultiplyPoints);
