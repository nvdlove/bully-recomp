// 0x5150F0 to 0x515570: Gamebryo's NiAccumulator (the part in the exe) and Bully's
// MdBullyAccumulator, the sorter the main view renders its alpha geometry through: the
// culler adds geometry to its array, FinishAccumulating sorts it (by the geometry's
// sort flags, then by distance) and draws it.

#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
// the camera position FinishAccumulating sorts by (points at its stack while it sorts)
GAME_VAR(const float*, ms_pSortCameraPos, 0x00C675B8);

GAME_FN(void, __thiscall, NiObject_Destruct, 0x007769D0, void* self);
GAME_FN(void, __thiscall, NiAccumulator_Construct, 0x00776980, void* self);
GAME_FN(void, __thiscall, NiAccumulator_StartAccumulating, 0x00856840, void* self, void* camera);
GAME_FN(void, __thiscall, NiAccumulator_FinishAccumulating, 0x007769A0, void* self);
GAME_FN(void, __thiscall, NiTArray_SetSize, 0x0084A110, void* self, uint32_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
inline void __cdecl Qsort(void* base, uint32_t count, uint32_t width, void* compare) { crt::Qsort(base, (size_t)count, (size_t)width, (int(__cdecl*)(const void*, const void*))compare); }
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040D9F0, const float* v);
GAME_FN(void, __thiscall, MdBullyAccumulator_Destruct, 0x00515160, void* self);

constexpr uint32_t kNiAccumulatorVtable = 0x009174B4;
constexpr uint32_t kNiTArrayGeometryVtable = 0x0091750C;          // NiTArray<NiGeometry*>
constexpr uint32_t kNiTPrimitiveArrayGeometryVtable = 0x0091756C; // NiTPrimitiveArray<NiGeometry*>
constexpr uint32_t kMdBullyAccumulatorVtable = 0x00917514;
constexpr uintptr_t kCompareGeometry = 0x00515200;

// NiTArray<NiGeometry*> (Gamebryo's)
struct GeometryArray {
    uint32_t vtable;        // +0
    re::Ptr32<re::Ptr32<uint8_t>> data;         // +4
    uint16_t maxSize;       // +8
    uint16_t size;          // +0xA
    uint16_t effectiveSize; // +0xC the non-null entries
    uint16_t growBy;        // +0xE
};
static_assert(sizeof(GeometryArray) == 0x10);

struct MdBullyAccumulator {
    uint32_t vtable;     // +0
    int32_t refCount;    // +4
    re::Ptr32<uint8_t> camera;     // +8 NiAccumulator's (set by StartAccumulating)
    GeometryArray items; // +0xC
};
static_assert(sizeof(MdBullyAccumulator) == 0x1C);
static_assert(offsetof(MdBullyAccumulator, items) == 0xC);

// NiVisibleArray as RegisterObjectArray reads it
struct VisibleArray {
    re::Ptr32<re::Ptr32<uint8_t>> data; // +0
    uint32_t count; // +4
};

// (inlined) NiTArray::RemoveAll: nulls the used entries
void RemoveAll(GeometryArray& a) {
    for (uint16_t i = 0; i < a.size; i++) {
        a.data[i] = nullptr;
    }
}

uint16_t Flags(const uint8_t* geometry) {
    return *reinterpret_cast<const uint16_t*>(geometry + 0xA8);
}
uint32_t Flags2(const uint8_t* geometry) {
    return *reinterpret_cast<const uint32_t*>(geometry + 0xB0);
}

// the distance from the sort camera to the geometry's world translate (+0x84)
float CameraDistance(const uint8_t* geometry) {
    const float* pos = reinterpret_cast<const float*>(geometry + 0x84);
    const float* cam = ms_pSortCameraPos;
    float d[3] = {pos[0] - cam[0], pos[1] - cam[1], pos[2] - cam[2]};
    return Vector_Magnitude(d);
}

} // namespace

namespace MdBullyAccumulatorCode {

// 0x5150F0 (thiscall): NiAccumulator::GetRTTI
static void* __fastcall NiAccumulator_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E2D50);
}

// 0x515100 (thiscall): NiAccumulator's deleting destructor
static void* __fastcall NiAccumulator_Destroy(uint32_t* self, re::Edx, int flags) {
    self[0] = kNiAccumulatorVtable;
    NiObject_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0xC);
    }
    return self;
}

// 0x515130 (thiscall): NiTArray<NiGeometry*>'s deleting destructor
static void* __fastcall GeometryArray_Destroy(GeometryArray* self, re::Edx, int flags) {
    self->vtable = kNiTArrayGeometryVtable;
    NiFree(self->data);
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x515160 (thiscall): the destructor's body
static void __fastcall Destruct(MdBullyAccumulator* self, re::Edx) {
    self->vtable = kMdBullyAccumulatorVtable;
    RemoveAll(self->items);
    self->items.size = 0;
    self->items.effectiveSize = 0;
    self->items.vtable = kNiTArrayGeometryVtable;
    NiFree(self->items.data);
    self->vtable = kNiAccumulatorVtable;
    NiObject_Destruct(self);
}

// 0x5151B0 (thiscall): GetRTTI
static void* __fastcall GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x00C675BC);
}

// 0x5151C0 (thiscall) vf18: StartAccumulating: the base's, then the array emptied
static void __fastcall StartAccumulating(MdBullyAccumulator* self, re::Edx, void* camera) {
    NiAccumulator_StartAccumulating(self, camera);
    RemoveAll(self->items);
    self->items.effectiveSize = 0;
    self->items.size = 0;
}

// 0x515200 (cdecl, qsort's comparer; not in functions.csv): flag 0x400 first, then
// 0x800, 0x1000, +0xB0 bit 0, 0x8000 and +0xB0 bit 1 last; then by the distance to the
// camera: far to near, or near to far with flag 0x1000
static int __cdecl CompareGeometry(const re::Ptr32<uint8_t>* pa, const re::Ptr32<uint8_t>* pb) {
    const uint8_t* a = *pa;
    const uint8_t* b = *pb;
    uint16_t fa = Flags(a);
    uint16_t fb = Flags(b);
    if (fa & 0x400) {
        if (!(fb & 0x400)) {
            return -1;
        }
    } else if (fb & 0x400) {
        return 1;
    }
    if (fa & 0x800) {
        if (!(fb & 0x800)) {
            return 1;
        }
    } else if (fb & 0x800) {
        return -1;
    }
    bool nearFirst = (fa & 0x1000) != 0;
    if (nearFirst) {
        if (!(fb & 0x1000)) {
            return 1;
        }
    } else if (fb & 0x1000) {
        return -1;
    }
    if (Flags2(a) & 1) {
        if (!(Flags2(b) & 1)) {
            return 1;
        }
    } else if (Flags2(b) & 1) {
        return -1;
    }
    if (fa & 0x8000) {
        if (!(fb & 0x8000)) {
            return 1;
        }
    } else if (fb & 0x8000) {
        return -1;
    }
    if (Flags2(a) & 2) {
        if (!(Flags2(b) & 2)) {
            return 1;
        }
    } else if (Flags2(b) & 2) {
        return -1;
    }
    float da = CameraDistance(a);
    float db = CameraDistance(b);
    // (x87: the difference unrounded, truncated by _ftol)
    return nearFirst ? int(double(db) - double(da)) : int(double(da) - double(db));
}

// 0x515420 (thiscall) vf19: FinishAccumulating: sorted (when there's a camera), each
// drawn (vtable[34], RenderImmediate), the array emptied, then the base's
static void __fastcall FinishAccumulating(MdBullyAccumulator* self, re::Edx) {
    uint8_t* renderer = ms_pRenderer;
    if (!renderer) {
        return;
    }
    if (self->camera) {
        const float* translate = reinterpret_cast<const float*>(self->camera + 0x84);
        float cameraPos[3] = {translate[0], translate[1], translate[2]};
        ms_pSortCameraPos = cameraPos;   // (left pointing at this stack)
        Qsort(self->items.data, self->items.size, 4, reinterpret_cast<void*>(kCompareGeometry));
    }
    for (uint32_t i = 0; i < self->items.size; i++) {
        uint8_t* geometry = self->items.data[i];
        if (geometry) {
            vc::Call<void>(geometry, 0x88, renderer);
        }
    }
    RemoveAll(self->items);
    self->items.size = 0;
    self->items.effectiveSize = 0;
    NiAccumulator_FinishAccumulating(self);
}

// 0x5154F0 (thiscall): the deleting destructor
static void* __fastcall Destroy(MdBullyAccumulator* self, re::Edx, int flags) {
    MdBullyAccumulator_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x1C);
    }
    return self;
}

// 0x515510 (thiscall): the constructor
static MdBullyAccumulator* __fastcall Construct(MdBullyAccumulator* self, re::Edx) {
    NiAccumulator_Construct(self);
    self->vtable = kMdBullyAccumulatorVtable;
    self->items.maxSize = 0;
    self->items.growBy = 1;
    self->items.size = 0;
    self->items.effectiveSize = 0;
    self->items.data = nullptr;
    self->items.vtable = kNiTPrimitiveArrayGeometryVtable;
    // (the inlined RemoveAll that follows has nothing to clear)
    self->items.size = 0;
    self->items.effectiveSize = 0;
    return self;
}

// 0x515570 (thiscall) vf20: RegisterObjectArray: every entry appended (SetAtGrow at
// the end, null ones included)
static void __fastcall RegisterObjectArray(MdBullyAccumulator* self, re::Edx, VisibleArray* visible) {
    uint32_t count = visible->count;
    GeometryArray& a = self->items;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t index = a.size;
        uint8_t* geometry = visible->data[i];
        if (index >= a.maxSize) {
            NiTArray_SetSize(&a, a.growBy + index);
        }
        if (index >= a.size) {
            a.size = uint16_t(index + 1);
            if (geometry) {
                a.effectiveSize++;
            }
        } else if (geometry) {
            if (!a.data[index]) {
                a.effectiveSize++;
            }
        } else if (a.data[index]) {
            a.effectiveSize--;
        }
        a.data[index] = geometry;
    }
}

} // namespace MdBullyAccumulatorCode

// ---- registration ----------------------------------------------------------------------

#define C16B_ACC_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace MdBullyAccumulatorCode {

static int(__cdecl* Original_CompareGeometry)(const re::Ptr32<uint8_t>*, const re::Ptr32<uint8_t>*);

static int __cdecl Shadow_CompareGeometry(const re::Ptr32<uint8_t>* a, const re::Ptr32<uint8_t>* b) {
    int theirs = Original_CompareGeometry(a, b);
    int ours = CompareGeometry(a, b);
    static uint32_t compared = 0;
    re::Compared("MdBullyAccumulator::CompareGeometry", compared);
    if (theirs != ours) {
        re::Mismatch("MdBullyAccumulator::CompareGeometry", "%p %p: game %d, ours %d", *a, *b, theirs, ours);
    }
    return theirs;
}

C16B_ACC_REVERSED("NiAccumulator::GetRTTI_5150F0", NiAccumulator_GetRTTI, 0x005150F0);
C16B_ACC_REVERSED("NiAccumulator::Destroy_515100", NiAccumulator_Destroy, 0x00515100);
C16B_ACC_REVERSED("NiTArray<NiGeometry*>::Destroy_515130", GeometryArray_Destroy, 0x00515130);
C16B_ACC_REVERSED("MdBullyAccumulator::Destruct", Destruct, 0x00515160);
C16B_ACC_REVERSED("MdBullyAccumulator::GetRTTI", GetRTTI, 0x005151B0);
C16B_ACC_REVERSED("MdBullyAccumulator::StartAccumulating", StartAccumulating, 0x005151C0);
REVERSED("MdBullyAccumulator::CompareGeometry", 0x00515200, RE_BODY(&CompareGeometry),
         reinterpret_cast<void*>(&Shadow_CompareGeometry), reinterpret_cast<void**>(&Original_CompareGeometry),
         re::Mode::Original);
C16B_ACC_REVERSED("MdBullyAccumulator::FinishAccumulating", FinishAccumulating, 0x00515420);
C16B_ACC_REVERSED("MdBullyAccumulator::Destroy", Destroy, 0x005154F0);
C16B_ACC_REVERSED("MdBullyAccumulator::Construct", Construct, 0x00515510);
C16B_ACC_REVERSED("MdBullyAccumulator::RegisterObjectArray", RegisterObjectArray, 0x00515570);

} // namespace MdBullyAccumulatorCode
