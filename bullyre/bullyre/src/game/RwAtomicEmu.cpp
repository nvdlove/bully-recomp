// The RenderWare emulation over Gamebryo (see RwStateEmulation.cpp): RpAtomic and RpClump.
// An atomic (0x80) wraps an NiGeometry (+0x14, reference counted); a clump (0x3C) keeps
// its atomics in a ring through the atomics' +0x4C links. Also the NiTMap / NiTPointerMap
// deleting destructors of the clone maps and NiCloningProcess's constructor.
// 0x6C69D0..0x6C773F.
//
// An atomic: +0 type 1, +1, +2 flags (4 render), +3 private flags (1 bound dirty), +4 its
// frame, +0x10 sync callback, +0x14 the geometry, +0x18 a name (NiFixedString), +0x1C
// alpha (0xFF), +0x20 the renderer frame it last updated, +0x24, +0x28 bounding sphere
// (+0x34 radius), +0x38 world bounding sphere, +0x48 its clump, +0x4C / +0x50 the clump
// ring's next / previous, +0x54 render callback, +0x58 (3), +0x5C / +0x5E, +0x60..+0x68
// (1.0), +0x6C, +0x70 serial number, +0x74, +0x78 an optional helper (0xF4), +0x7C.
// A clump: +0 type 2, +4 its frame, +8 / +0xC the atomics ring, +0x10 alpha, +0x14, +0x18
// a name, +0x1C and +0x24 two more rings, +0x2C, +0x30, +0x34 skinned, +0x38.

#include <cmath>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct CVector {
    float x, y, z;
};

GAME_VAR(char*, rwa_emptyString, 0x00951028);
GAME_VAR(void*, rwa_stringTable, 0x020E216C);
GAME_VAR(uint32_t, rwa_nextSerial, 0x020C5C54);
GAME_VAR(uint8_t, rwa_immediateRender, 0x00C936FA);
GAME_VAR(uint32_t, rwa_geometryClassBit, 0x020E2550);
GAME_VAR(uint32_t, rwa_copyType, 0x020E2770);
GAME_VAR(uint8_t, rwa_appendChar, 0x00BC75F4);
inline void* const kDefaultRenderCallback = reinterpret_cast<void*>(0x006C6AF0);
inline void* const kSyncCallback = reinterpret_cast<void*>(0x006CA7D0);
inline void* const kCountCallback = reinterpret_cast<void*>(0x006C73D0);
inline void* const kDestroyCallback = reinterpret_cast<void*>(0x006C7480);
inline void* const kCloneCallback = reinterpret_cast<void*>(0x006C74E0);

GAME_FN(void, __cdecl, Rwa_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, Rwa_PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, Rwa_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Rwa_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Rwa_NiNew, 0x00752460, uint32_t size);
GAME_FN(void*, __cdecl, Rwa_NiMalloc, 0x00752300, uint32_t size);
GAME_FN(void, __cdecl, Rwa_NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, Rwa_NiFreeSized, 0x00752540, void* p, uint32_t size);
inline void* __cdecl Rwa_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(void, __thiscall, Rwa_MapRemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, Rwa_AllocatorFree, 0x00401B30, void* allocator, void* item);
GAME_FN(void, __thiscall, Rwa_FixedStringAssign, 0x00401CD0, re::Ptr32<char>* self, re::Ptr32<char>* other);
GAME_FN(void, __thiscall, Rwa_StringTableFree, 0x00754550, void* table, re::Ptr32<char>* string, uint32_t length);
GAME_FN(void, __cdecl, Rwa_PrepareRender, 0x005F0FC0, void* p);
GAME_FN(void, __cdecl, Rwa_GeometryPrepare, 0x0040A040, void* geometry);
GAME_FN(uint32_t, __cdecl, Rwa_RendererFrame, 0x00402600);
GAME_FN(void, __cdecl, Rwa_GeometryNewFrame, 0x00408BC0, void* geometry);
GAME_FN(void, __cdecl, Rwa_GeometryUpdate, 0x004026E0, void* geometry);
GAME_FN(void, __cdecl, Rwa_RenderImmediate, 0x00408C90, void* geometry, int);
GAME_FN(void, __cdecl, Rwa_RenderQueued, 0x00408D90, void* geometry, int);
GAME_FN(void, __cdecl, Rwa_SetParentFrame, 0x006C9BD0, void* object, void* frame);
GAME_FN(void, __cdecl, Rwa_DetachFromFrame, 0x006C9C20, void* object);
GAME_FN(int, __cdecl, Rwa_FrameDirty, 0x006C9BA0, void* frame);
GAME_FN(uint8_t*, __cdecl, Rwa_FrameLTM, 0x006CA0A0, void* frame);
GAME_FN(CVector*, __thiscall, Rwa_TransformPoint, 0x00450B10, const void* m, CVector* out, const CVector* v);
GAME_FN(void, __thiscall, Rwa_AVObjectUpdate, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(void, __thiscall, Rwa_HelperSetGeometry, 0x00411F00, void* helper, void* geometry);
GAME_FN(void*, __thiscall, Rwa_HelperConstruct, 0x00411F30, void* self, void* geometry);
GAME_FN(void, __thiscall, Rwa_HelperInit, 0x00411E60, void* self);
GAME_FN(void, __thiscall, Rwa_HelperDestruct, 0x004116D0, void* self);
GAME_FN(void*, __thiscall, Rwa_NiObjectClone, 0x007551F0, void* object, void* process);
GAME_FN(void, __cdecl, Rwa_GeometrySetup, 0x0040AA70, void* geometry);
GAME_FN(uint8_t, __cdecl, Rwa_GeometrySkinned, 0x00407D60, void* geometry);
GAME_FN(uint8_t*, __thiscall, Rwa_AtomicConstruct, 0x006C6EB0, void* self);
GAME_FN(void, __thiscall, Rwa_AtomicDestruct, 0x006C6FA0, void* self);
GAME_FN(uint8_t*, __cdecl, Rwa_AtomicClone, 0x006C70D0, uint8_t* atomic);
GAME_FN(int, __cdecl, Rwa_AtomicDestroy, 0x006C72C0, uint8_t* atomic);
GAME_FN(void*, __thiscall, Rwa_CloningConstruct, 0x006C7000, void* self, uint32_t buckets);
GAME_FN(uint8_t*, __cdecl, Rwa_ForAllAtomics, 0x006C7380, uint8_t* clump, void* callback, void* data);
GAME_FN(uint8_t*, __cdecl, Rwa_AddAtomic, 0x006C74A0, uint8_t* clump, uint8_t* atomic);
GAME_FN(uint8_t*, __cdecl, Rwa_ClumpCreate, 0x006C7540);
GAME_FN(int, __cdecl, Rwa_ClumpDestroy, 0x006C75C0, uint8_t* clump);
GAME_FN(void*, __cdecl, Rwa_FrameCloneHierarchy, 0x006CA460, void* frame);
GAME_FN(void, __cdecl, Rwa_FrameClearClones, 0x006C9C80, void* frame);
GAME_FN(void, __cdecl, Rwa_FrameDestroyHierarchy, 0x006CA490, void* frame);

// a reference counted Gamebryo object let go of (its vf 1 at none)
inline void Release(void* object) {
    if (object != nullptr && vc::Decrement(static_cast<uint8_t*>(object) + 4) == 0) {
        vc::Call<void>(object, 4);
    }
}
inline void AddRef(void* object) {
    vc::Increment(static_cast<uint8_t*>(object) + 4);
}
// a NiPointer member assigned
inline void Assign(re::Ptr32<void>& member, void* object) {
    void* old = member;
    if (old == object) {
        return;
    }
    Release(old);
    member = object;
    if (object != nullptr) {
        AddRef(object);
    }
}

inline void ReleaseFixedString(re::Ptr32<char>* s) {
    if (*s == rwa_emptyString) {
        return;
    }
    uint32_t length = *reinterpret_cast<uint16_t*>(*s - 2);
    if (vc::Decrement(*s - 8) == 1) {
        Rwa_StringTableFree(rwa_stringTable, s, length);
    }
}

// 0x6C69D0: the atomic's flags (+2)
void __cdecl Atomic_SetFlags(uint8_t* atomic, int flags) {
    At<uint8_t>(atomic, 2) = uint8_t(flags);
}

// 0x6C69E0
uint8_t* __cdecl Atomic_GetClump(uint8_t* atomic) {
    return At<uint8_t*>(atomic, 0x48);
}

// 0x6C69F0: rendered through its callback
int __cdecl Atomic_Render(uint8_t* atomic) {
    Rwa_PrepareRender(At<void*>(atomic, 0x7C));
    using Callback = int(__cdecl*)(uint8_t*);
    return At<Callback>(atomic, 0x54)(atomic);
}

// 0x6C6A10: its frame set (the bound marked dirty)
uint8_t* __cdecl Atomic_SetFrame(uint8_t* atomic, void* frame) {
    Rwa_SetParentFrame(atomic, frame);
    At<uint8_t>(atomic, 3) |= 1;
    return atomic;
}

// 0x6C6A30
void* __cdecl Atomic_GetRenderCallback(uint8_t* atomic) {
    return At<void*>(atomic, 0x54);
}

// 0x6C6A40 (thiscall): the cloning process's two maps deleted (NiCloningProcess's destructor)
void __fastcall Cloning_Destruct(re::Ptr32<void>* self, re::Edx) {
    if (self[0] != nullptr) {
        vc::Call<void>(self[0], 0, 1);
    }
    if (self[1] != nullptr) {
        vc::Call<void>(self[1], 0, 1);
    }
}

// the maps' deleting destructors: each level's vtable set and emptied, the buckets freed
template <uintptr_t... Vtables>
void* DeleteMap(void* self, int flags) {
    ((At<uint32_t>(self, 0) = Vtables, Rwa_MapRemoveAll(self)), ...);
    Rwa_NiFree(At<void*>(self, 8));
    if (flags & 1) {
        Rwa_NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x6C6A70 (thiscall, vf 0): NiTMapBase<NiObject*, NiObject*>
void* __fastcall MapBaseObject_Delete(void* self, re::Edx, int flags) {
    return DeleteMap<0x00942880>(self, flags);
}

// 0x6C6AB0 (thiscall, vf 0): NiTMapBase<NiObject*, bool>
void* __fastcall MapBaseBool_Delete(void* self, re::Edx, int flags) {
    return DeleteMap<0x00942898>(self, flags);
}

// 0x6C6BE0 (thiscall, vf 0): NiTPointerMap<NiObject*, NiObject*>
void* __fastcall PointerMapObject_Delete(void* self, re::Edx, int flags) {
    return DeleteMap<0x009428B0, 0x00942880>(self, flags);
}

// 0x6C6C20 (thiscall, vf 0): NiTPointerMap<NiObject*, bool>
void* __fastcall PointerMapBool_Delete(void* self, re::Edx, int flags) {
    return DeleteMap<0x009428C8, 0x00942898>(self, flags);
}

// 0x6C6BC0 (thiscall, vf 4): NiTPointerMap<NiObject*, bool>'s item cleared and freed
void __fastcall PointerMapBool_ClearValue(uint8_t* self, re::Edx, uint8_t* item) {
    At<uint8_t>(item, 8) = 0;
    Rwa_AllocatorFree(self + 0xC, item);
}

// 0x6C6AF0: the default render callback: the geometry drawn at the atomic's alpha
// (queued, or at once when 0xC936FA is set), updated once a renderer frame (memory tag 0x37)
uint8_t* __cdecl Atomic_DefaultRender(uint8_t* atomic) {
    if (atomic == nullptr) {
        return nullptr;
    }
    Rwa_PushMemoryTag(0x37);
    Rwa_GeometryPrepare(At<void*>(atomic, 0x14));
    uint8_t* geometry = At<uint8_t*>(atomic, 0x14);
    At<uint16_t>(geometry, 0xA8) |= 0x2000;
    float alpha = float(double(At<int32_t>(atomic, 0x1C)) / 255.0);
    At<float>(geometry, 0xAC) = alpha;
    uint32_t frame = Rwa_RendererFrame();
    if (At<uint32_t>(atomic, 0x20) != frame) {
        Rwa_GeometryNewFrame(At<void*>(atomic, 0x14));
        At<uint32_t>(atomic, 0x20) = frame;
        Rwa_GeometryUpdate(At<void*>(atomic, 0x14));
    }
    if (rwa_immediateRender != 0) {
        Rwa_RenderImmediate(At<void*>(atomic, 0x14), 1);
    } else {
        Rwa_RenderQueued(At<void*>(atomic, 0x14), 1);
    }
    Rwa_PopMemoryTag();
    return atomic;
}

// 0x6C6B90
uint8_t* __cdecl Atomic_Get28(uint8_t* atomic) {
    return atomic + 0x28;
}

// 0x6C6BA0: the render callback (null: the default)
void __cdecl Atomic_SetRenderCallback(uint8_t* atomic, void* callback) {
    At<void*>(atomic, 0x54) = callback != nullptr ? callback : kDefaultRenderCallback;
}

// 0x6C6C60: the geometry set (given as a NiPointer by value: let go of at the end),
// updated, its helper told, its bounds copied
uint8_t* __cdecl Atomic_SetGeometry(uint8_t* atomic, void* geometry, int) {
    if (geometry != At<void*>(atomic, 0x14)) {
        Assign(At<void*>(atomic, 0x14), geometry);
        if (At<void*>(atomic, 0x78) != nullptr) {
            Rwa_AVObjectUpdate(At<void*>(atomic, 0x14), 0.0f, 1);
            Rwa_HelperSetGeometry(At<void*>(atomic, 0x78), At<void*>(atomic, 0x14));
        }
        Rwa_AVObjectUpdate(At<void*>(atomic, 0x14), 0.0f, 1);
        uint8_t* g = At<uint8_t*>(atomic, 0x14);
        At<uint32_t>(atomic, 0x28) = At<uint32_t>(g, 0x1C);
        At<uint32_t>(atomic, 0x2C) = At<uint32_t>(g, 0x20);
        At<uint32_t>(atomic, 0x30) = At<uint32_t>(g, 0x24);
        At<float>(atomic, 0x34) = At<float>(g, 0x28);
        At<uint32_t>(atomic, 0x38) = At<uint32_t>(g, 0x1C);
        At<uint32_t>(atomic, 0x3C) = At<uint32_t>(g, 0x20);
        At<uint32_t>(atomic, 0x40) = At<uint32_t>(g, 0x24);
        At<float>(atomic, 0x44) = At<float>(g, 0x28);
    }
    Release(geometry);
    return atomic;
}

// 0x6C6D30: the world bounding sphere: when the frame moved and the bound is dirty, the
// sphere put through the frame (radius by the largest axis' length); then the geometry's
// world bound copied over it (its radius only when above 0)
CVector* __cdecl Atomic_GetWorldBoundingSphere(uint8_t* atomic) {
    if (Rwa_FrameDirty(At<void*>(atomic, 4)) != 0 && (At<uint8_t>(atomic, 3) & 1)) {
        const float* m = reinterpret_cast<const float*>(Rwa_FrameLTM(At<void*>(atomic, 4)));
        CVector center;
        CVector* c = Rwa_TransformPoint(m, &center, reinterpret_cast<CVector*>(atomic + 0x28));
        At<CVector>(atomic, 0x38) = *c;
        float a = (m[0] * m[0] + m[3] * m[3]) + m[6] * m[6];
        float b = (m[1] * m[1] + m[4] * m[4]) + m[7] * m[7];
        float cc = (m[2] * m[2] + m[5] * m[5]) + m[8] * m[8];
        float largest = (b < a) ? a : b;
        if (largest < cc) {
            largest = cc;
        }
        largest = std::fabs(largest);
        largest = std::sqrt(largest);
        At<uint8_t>(atomic, 3) &= 0xFE;
        At<float>(atomic, 0x44) = largest * At<float>(atomic, 0x34);
    }
    uint8_t* g = At<uint8_t*>(atomic, 0x14);
    At<uint32_t>(atomic, 0x38) = At<uint32_t>(g, 0x1C);
    At<uint32_t>(atomic, 0x3C) = At<uint32_t>(g, 0x20);
    At<uint32_t>(atomic, 0x40) = At<uint32_t>(g, 0x24);
    if (0.0f < At<float>(g, 0x28)) {
        At<float>(atomic, 0x44) = At<float>(g, 0x28);
    }
    return reinterpret_cast<CVector*>(atomic + 0x38);
}

// 0x6C6EB0 (thiscall): the atomic's constructor
uint8_t* __fastcall Atomic_Construct(uint8_t* self, re::Edx) {
    At<void*>(self, 0x14) = nullptr;
    At<char*>(self, 0x18) = rwa_emptyString;
    At<uint32_t>(self, 0x70) = rwa_nextSerial;
    rwa_nextSerial += 1;
    At<uint8_t>(self, 0) = 1;
    At<uint8_t>(self, 1) = 0;
    At<uint8_t>(self, 2) = 5;
    At<uint8_t>(self, 3) = 1;
    At<void*>(self, 4) = nullptr;
    At<void*>(self, 0x10) = kSyncCallback;
    Rwa_SetParentFrame(self, nullptr);
    At<float>(self, 0x34) = 0.0f;
    At<float>(self, 0x30) = 0.0f;
    At<uint32_t>(self, 0x50) = 0;
    At<float>(self, 0x2C) = 0.0f;
    At<uint32_t>(self, 0x4C) = 0;
    At<float>(self, 0x28) = 0.0f;
    At<uint16_t>(self, 0x5E) = 0;
    At<float>(self, 0x44) = 0.0f;
    At<uint16_t>(self, 0x5C) = 0;
    At<float>(self, 0x40) = 0.0f;
    At<uint32_t>(self, 0x20) = 0;
    At<float>(self, 0x3C) = 0.0f;
    At<uint32_t>(self, 0x6C) = 0;
    At<float>(self, 0x38) = 0.0f;
    At<uint32_t>(self, 0x74) = 0;
    At<uint32_t>(self, 0x78) = 0;
    At<float>(self, 0x64) = 1.0f;
    At<void*>(self, 0x54) = kDefaultRenderCallback;
    At<float>(self, 0x60) = 1.0f;
    At<uint32_t>(self, 0x58) = 3;
    At<uint32_t>(self, 0x1C) = 0xFF;
    At<float>(self, 0x68) = 0.0f;   // (the fldz left under the fld1)
    return self;
}

// 0x6C6F50: RpAtomicCreate (memory tag 0xB)
uint8_t* __cdecl Atomic_Create() {
    Rwa_PushMemoryTag(0xB);
    void* p = Rwa_Malloc(0x80);
    uint8_t* atomic = p != nullptr ? Rwa_AtomicConstruct(p) : nullptr;
    Rwa_PopMemoryTag();
    At<uint32_t>(atomic, 0x1C) = 0xFF;
    return atomic;
}

// 0x6C6FA0 (thiscall): the atomic's destructor (its name and geometry let go of)
void __fastcall Atomic_Destruct(uint8_t* self, re::Edx) {
    ReleaseFixedString(reinterpret_cast<re::Ptr32<char>*>(self + 0x18));
    Release(At<void*>(self, 0x14));
}

// 0x6C7000 (thiscall): NiCloningProcess(buckets): its object and bool maps made
void* __fastcall Cloning_Construct(uint8_t* self, re::Edx, uint32_t buckets) {
    At<uint32_t>(self, 0xC) = 1;
    At<uint32_t>(self, 0x10) = 1;
    static const uintptr_t kBase[2] = {0x00942880, 0x00942898};
    static const uintptr_t kPointer[2] = {0x009428B0, 0x009428C8};
    for (int i = 0; i < 2; i++) {
        uint8_t* map = static_cast<uint8_t*>(Rwa_NiNew(0x10));
        if (map != nullptr) {
            At<uint32_t>(map, 0) = kBase[i];
            At<uint32_t>(map, 4) = buckets;
            At<uint32_t>(map, 0xC) = 0;
            void* table = Rwa_NiMalloc(buckets * 4);
            At<void*>(map, 8) = table;
            Rwa_Memset(table, 0, At<uint32_t>(map, 4) * 4);
            At<uint32_t>(map, 0) = kPointer[i];
        }
        At<uint8_t*>(self, i * 4) = map;
    }
    At<uint32_t>(self, 8) = rwa_copyType;
    At<uint8_t>(self, 0x14) = rwa_appendChar;
    return self;
}

// 0x6C70D0: RpAtomicClone: a new atomic with the source's settings, its geometry cloned
// (kept only when the clone is a geometry), its bounds and helper
uint8_t* __cdecl Atomic_Clone(uint8_t* source) {
    Rwa_PushMemoryTag(0xB);
    Rwa_PushMemoryTag(0xB);
    void* p = Rwa_Malloc(0x80);
    uint8_t* atomic = p != nullptr ? Rwa_AtomicConstruct(p) : nullptr;
    Rwa_PopMemoryTag();
    At<uint32_t>(atomic, 0x1C) = 0xFF;
    At<uint32_t>(atomic, 0x1C) = At<uint32_t>(source, 0x1C);
    At<uint32_t>(atomic, 0x24) = At<uint32_t>(source, 0x24);
    Rwa_FixedStringAssign(reinterpret_cast<re::Ptr32<char>*>(atomic + 0x18), reinterpret_cast<re::Ptr32<char>*>(source + 0x18));
    At<uint32_t>(atomic, 0x7C) = At<uint32_t>(source, 0x7C);
    At<uint8_t>(atomic, 2) = At<uint8_t>(source, 2);
    At<uint8_t>(atomic, 3) = At<uint8_t>(source, 3);
    At<uint8_t>(atomic, 1) = At<uint8_t>(source, 1);
    At<uint8_t>(atomic, 0) = At<uint8_t>(source, 0);
    At<void*>(atomic, 4) = nullptr;
    Rwa_SetParentFrame(atomic, nullptr);
    At<uint8_t>(atomic, 3) |= 1;
    static const int kCopied[] = {0x28, 0x2C, 0x30, 0x34, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C, 0x74};
    for (int off : kCopied) {
        At<uint32_t>(atomic, off) = At<uint32_t>(source, off);
    }
    uint8_t process[0x18];
    Rwa_CloningConstruct(process, 0x25);
    At<uint32_t>(process, 8) = 1;
    At<uint8_t>(process, 0x14) = 0x25;
    void* clone = Rwa_NiObjectClone(At<void*>(source, 0x14), process);
    if (clone != nullptr) {
        const uint8_t* rtti = vc::Call<const uint8_t*>(clone, 8);
        uint32_t bit = rwa_geometryClassBit;
        if (!(reinterpret_cast<const uint32_t*>(rtti + 0xC)[bit >> 5] & (1u << (bit & 0x1F)))) {
            clone = nullptr;
        }
    }
    Assign(At<void*>(atomic, 0x14), clone);
    Rwa_GeometrySetup(At<void*>(atomic, 0x14));
    uint8_t* from = At<uint8_t*>(source, 0x14);
    uint8_t* to = At<uint8_t*>(atomic, 0x14);
    for (int i = 0; i < 4; i++) {
        At<uint32_t>(to, 0x1C + i * 4) = At<uint32_t>(from, 0x1C + i * 4);
    }
    At<uint32_t>(atomic, 0x20) = 0;
    if (At<void*>(source, 0x78) != nullptr) {
        void* made = Rwa_Malloc(0xF4);
        void* helper = made != nullptr ? Rwa_HelperConstruct(made, At<void*>(atomic, 0x14)) : nullptr;
        At<void*>(atomic, 0x78) = helper;
        Rwa_HelperInit(helper);
    }
    Rwa_PopMemoryTag();
    Cloning_Destruct(reinterpret_cast<re::Ptr32<void>*>(process), nullptr);
    return atomic;
}

// 0x6C72C0: RpAtomicDestroy (returns 1)
int __cdecl Atomic_Destroy(uint8_t* atomic) {
    At<uint32_t>(atomic, 0x74) = 0;
    Rwa_DetachFromFrame(atomic);
    void* geometry = At<void*>(atomic, 0x14);
    if (geometry != nullptr) {
        Release(geometry);
        At<void*>(atomic, 0x14) = nullptr;
    }
    void* helper = At<void*>(atomic, 0x78);
    if (helper != nullptr) {
        Rwa_HelperDestruct(helper);
        Rwa_Free(helper);
        At<void*>(atomic, 0x78) = nullptr;
    }
    Rwa_AtomicDestruct(atomic);
    Rwa_Free(atomic);
    return 1;
}

// 0x6C7340: RpClumpRemoveAtomic
uint8_t* __cdecl Clump_RemoveAtomic(uint8_t* clump, uint8_t* atomic) {
    uint8_t* prev = At<uint8_t*>(atomic, 0x50);
    At<uint8_t*>(prev, 0) = At<uint8_t*>(atomic, 0x4C);
    uint8_t* next = At<uint8_t*>(atomic, 0x4C);
    At<uint8_t*>(next, 4) = At<uint8_t*>(atomic, 0x50);
    At<void*>(atomic, 0x48) = nullptr;
    return clump;
}

// 0x6C7370: an atomic's or clump's frame
void* __cdecl Object_GetFrame(uint8_t* object) {
    return At<void*>(object, 4);
}

// 0x6C7380: RpClumpForAllAtomics (until the callback returns null)
uint8_t* __cdecl Clump_ForAllAtomics(uint8_t* clump, void* callback, void* data) {
    uint8_t* head = clump + 8;
    uint8_t* link = At<uint8_t*>(clump, 8);
    if (link == head) {
        return clump;
    }
    using Callback = void*(__cdecl*)(uint8_t*, void*);
    do {
        uint8_t* next = At<uint8_t*>(link, 0);
        if (reinterpret_cast<Callback>(callback)(link - 0x4C, data) == nullptr) {
            break;
        }
        link = next;
    } while (link != head);
    return clump;
}

// 0x6C73D0: the counting callback
uint8_t* __cdecl CountCallback(uint8_t* atomic, int32_t* count) {
    *count += 1;
    return atomic;
}

// 0x6C73E0: RpClumpGetNumAtomics
int32_t __cdecl Clump_GetNumAtomics(uint8_t* clump) {
    int32_t count = 0;
    Rwa_ForAllAtomics(clump, kCountCallback, &count);
    return count;
}

// 0x6C7410: RpClumpSetFrame
uint8_t* __cdecl Clump_SetFrame(uint8_t* clump, void* frame) {
    At<void*>(clump, 4) = frame;
    return clump;
}

// 0x6C7420: RpClumpRender: the atomics with the render flag drawn at the clump's alpha
// (null when one's callback fails)
uint8_t* __cdecl Clump_Render(uint8_t* clump) {
    uint8_t* result = clump;
    uint8_t* head = clump + 8;
    for (uint8_t* link = At<uint8_t*>(clump, 8); link != head; link = At<uint8_t*>(link, 0)) {
        uint8_t* atomic = link - 0x4C;
        Rwa_PrepareRender(At<void*>(link, 0x30));
        At<uint32_t>(atomic, 0x1C) = At<uint32_t>(clump, 0x10);
        if (At<uint8_t>(atomic, 2) & 4) {
            Rwa_FrameLTM(At<void*>(atomic, 4));
            using Callback = int(__cdecl*)(uint8_t*);
            if (At<Callback>(atomic, 0x54)(atomic) == 0) {
                result = nullptr;
            }
        }
    }
    return result;
}

// 0x6C7480: the destroying callback
uint8_t* __cdecl DestroyCallback(uint8_t* atomic, void*) {
    Rwa_AtomicDestroy(atomic);
    return atomic;
}

// 0x6C74A0: RpClumpAddAtomic (at the ring's front; the clump marked skinned by it)
uint8_t* __cdecl Clump_AddAtomic(uint8_t* clump, uint8_t* atomic) {
    uint8_t skinned = Rwa_GeometrySkinned(At<void*>(atomic, 0x14));
    uint8_t* first = At<uint8_t*>(clump, 8);
    At<uint8_t>(clump, 0x34) = skinned != 0 ? 1 : 0;
    uint8_t* link = atomic + 0x4C;
    At<uint8_t*>(link, 0) = first;
    At<uint8_t*>(atomic, 0x50) = clump + 8;
    At<uint8_t*>(At<uint8_t*>(clump, 8), 4) = link;
    At<uint8_t*>(clump, 8) = link;
    At<uint8_t*>(atomic, 0x48) = clump;
    return clump;
}

// 0x6C74E0: the cloning callback ({clump, ok}): the atomic cloned onto its frame's clone
uint8_t* __cdecl CloneCallback(uint8_t* atomic, uint8_t* data) {
    uint8_t* clone = Rwa_AtomicClone(atomic);
    if (clone == nullptr) {
        At<uint8_t>(data, 4) = 0;
        return nullptr;
    }
    uint8_t* frame = At<uint8_t*>(atomic, 4);
    if (frame == nullptr) {
        Rwa_AtomicDestroy(clone);
        return nullptr;
    }
    Rwa_SetParentFrame(clone, At<void*>(frame, 0x8C));
    At<uint8_t>(clone, 3) |= 1;
    Rwa_AddAtomic(At<uint8_t*>(data, 0), clone);
    return atomic;
}

// 0x6C7540: RpClumpCreate (memory tag 0xC)
uint8_t* __cdecl Clump_Create() {
    Rwa_PushMemoryTag(0xC);
    uint8_t* clump = static_cast<uint8_t*>(Rwa_Malloc(0x3C));
    if (clump != nullptr) {
        At<char*>(clump, 0x18) = rwa_emptyString;
        At<uint8_t*>(clump, 8) = clump + 8;
        At<uint8_t*>(clump, 0xC) = clump + 8;
        At<uint8_t*>(clump, 0x1C) = clump + 0x1C;
        At<uint8_t*>(clump, 0x20) = clump + 0x1C;
        At<uint8_t>(clump, 0) = 2;
        At<uint8_t>(clump, 1) = 0;
        At<uint8_t>(clump, 2) = 0;
        At<uint8_t>(clump, 3) = 0;
        At<void*>(clump, 4) = nullptr;
        At<uint8_t*>(clump, 0x24) = clump + 0x24;
        At<uint8_t*>(clump, 0x28) = clump + 0x24;
        At<uint32_t>(clump, 0x30) = 0;
        At<uint32_t>(clump, 0x2C) = 0;
        At<uint32_t>(clump, 0x38) = 0;
    }
    At<uint32_t>(clump, 0x10) = 0xFF;
    Rwa_PopMemoryTag();
    return clump;
}

// 0x6C75C0: RpClumpDestroy: its atomics, its frame hierarchy, its name (returns 1)
int __cdecl Clump_Destroy(uint8_t* clump) {
    Rwa_ForAllAtomics(clump, kDestroyCallback, nullptr);
    if (At<void*>(clump, 4) != nullptr) {
        Rwa_FrameDestroyHierarchy(At<void*>(clump, 4));
    }
    ReleaseFixedString(reinterpret_cast<re::Ptr32<char>*>(clump + 0x18));
    Rwa_Free(clump);
    return 1;
}

// 0x6C7630: RpClumpClone: the frame hierarchy and atomics cloned (null when either fails:
// the memory tag's left pushed then)
uint8_t* __cdecl Clump_Clone(uint8_t* source) {
    Rwa_PushMemoryTag(0xC);
    uint8_t* clump = Rwa_ClumpCreate();
    At<uint32_t>(clump, 0x10) = At<uint32_t>(source, 0x10);
    Rwa_FixedStringAssign(reinterpret_cast<re::Ptr32<char>*>(clump + 0x18), reinterpret_cast<re::Ptr32<char>*>(source + 0x18));
    At<uint32_t>(clump, 0x14) = At<uint32_t>(source, 0x14);
    At<uint8_t>(clump, 0x34) = At<uint8_t>(source, 0x34);
    void* frame = Rwa_FrameCloneHierarchy(At<void*>(source, 4));
    At<void*>(clump, 4) = frame;
    if (frame == nullptr) {
        Rwa_ClumpDestroy(clump);
        return nullptr;
    }
    struct {
        re::Ptr32<uint8_t> clump;
        uint8_t ok;
    } data = {clump, 1};
    Rwa_ForAllAtomics(source, kCloneCallback, &data);
    Rwa_FrameClearClones(At<void*>(clump, 4));
    if (data.ok == 0) {
        Rwa_ClumpDestroy(clump);
        return nullptr;
    }
    At<uint32_t>(clump, 0x38) = At<uint32_t>(source, 0x38);
    Rwa_PopMemoryTag();
    return clump;
}

// 0x6C76D0: a frame's bone hierarchy set (the old one's frame cleared; returns 1)
int __cdecl Frame_SetHierarchy(uint8_t* frame, uint8_t* hierarchy) {
    uint8_t* old = At<uint8_t*>(frame, 0x94);
    if (old != nullptr) {
        At<void*>(old, 0x10) = nullptr;
    }
    At<uint8_t*>(frame, 0x94) = hierarchy;
    if (hierarchy != nullptr) {
        At<uint8_t*>(hierarchy, 0x10) = frame;
    }
    return 1;
}

// 0x6C7700
uint8_t* __cdecl Frame_GetHierarchy(uint8_t* frame) {
    return At<uint8_t*>(frame, 0x94);
}

// 0x6C7710: a hierarchy's matrices
uint8_t* __cdecl Hierarchy_GetMatrices(uint8_t* hierarchy) {
    return At<uint8_t*>(hierarchy, 8);
}

// 0x6C7720: a frame's bone fields reset
uint8_t* __cdecl Frame_ResetBone(uint8_t* frame) {
    At<int32_t>(frame, 0x90) = -1;
    At<void*>(frame, 0x94) = nullptr;
    return frame;
}

} // namespace

#define RWATOMIC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

RWATOMIC_REVERSED("RpAtomic::SetFlags", 0x006C69D0, &Atomic_SetFlags);
RWATOMIC_REVERSED("RpAtomic::GetClump", 0x006C69E0, &Atomic_GetClump);
RWATOMIC_REVERSED("RpAtomic::Render", 0x006C69F0, &Atomic_Render);
RWATOMIC_REVERSED("RpAtomic::SetFrame", 0x006C6A10, &Atomic_SetFrame);
RWATOMIC_REVERSED("RpAtomic::GetRenderCallback", 0x006C6A30, &Atomic_GetRenderCallback);
RWATOMIC_REVERSED("NiCloningProcess::Destruct", 0x006C6A40, &Cloning_Destruct);
RWATOMIC_REVERSED("NiTMapBase<NiObject*,NiObject*>::Delete", 0x006C6A70, &MapBaseObject_Delete);
RWATOMIC_REVERSED("NiTMapBase<NiObject*,bool>::Delete", 0x006C6AB0, &MapBaseBool_Delete);
RWATOMIC_REVERSED("RpAtomic::DefaultRender", 0x006C6AF0, &Atomic_DefaultRender);
RWATOMIC_REVERSED("RpAtomic::Get28", 0x006C6B90, &Atomic_Get28);
RWATOMIC_REVERSED("RpAtomic::SetRenderCallback", 0x006C6BA0, &Atomic_SetRenderCallback);
RWATOMIC_REVERSED("NiTPointerMap<NiObject*,bool>::ClearValue", 0x006C6BC0, &PointerMapBool_ClearValue);
RWATOMIC_REVERSED("NiTPointerMap<NiObject*,NiObject*>::Delete", 0x006C6BE0, &PointerMapObject_Delete);
RWATOMIC_REVERSED("NiTPointerMap<NiObject*,bool>::Delete", 0x006C6C20, &PointerMapBool_Delete);
RWATOMIC_REVERSED("RpAtomic::SetGeometry", 0x006C6C60, &Atomic_SetGeometry);
RWATOMIC_REVERSED("RpAtomic::GetWorldBoundingSphere", 0x006C6D30, &Atomic_GetWorldBoundingSphere);
RWATOMIC_REVERSED("RpAtomic::Construct", 0x006C6EB0, &Atomic_Construct);
RWATOMIC_REVERSED("RpAtomic::Create", 0x006C6F50, &Atomic_Create);
RWATOMIC_REVERSED("RpAtomic::Destruct", 0x006C6FA0, &Atomic_Destruct);
RWATOMIC_REVERSED("NiCloningProcess::Construct", 0x006C7000, &Cloning_Construct);
RWATOMIC_REVERSED("RpAtomic::Clone", 0x006C70D0, &Atomic_Clone);
RWATOMIC_REVERSED("RpAtomic::Destroy", 0x006C72C0, &Atomic_Destroy);
RWATOMIC_REVERSED("RpClump::RemoveAtomic", 0x006C7340, &Clump_RemoveAtomic);
RWATOMIC_REVERSED("RwObject::GetFrame", 0x006C7370, &Object_GetFrame);
RWATOMIC_REVERSED("RpClump::ForAllAtomics", 0x006C7380, &Clump_ForAllAtomics);
RWATOMIC_REVERSED("RpClump::CountCallback", 0x006C73D0, &CountCallback);
RWATOMIC_REVERSED("RpClump::GetNumAtomics", 0x006C73E0, &Clump_GetNumAtomics);
RWATOMIC_REVERSED("RpClump::SetFrame", 0x006C7410, &Clump_SetFrame);
RWATOMIC_REVERSED("RpClump::Render", 0x006C7420, &Clump_Render);
RWATOMIC_REVERSED("RpClump::DestroyCallback", 0x006C7480, &DestroyCallback);
RWATOMIC_REVERSED("RpClump::AddAtomic", 0x006C74A0, &Clump_AddAtomic);
RWATOMIC_REVERSED("RpClump::CloneCallback", 0x006C74E0, &CloneCallback);
RWATOMIC_REVERSED("RpClump::Create", 0x006C7540, &Clump_Create);
RWATOMIC_REVERSED("RpClump::Destroy", 0x006C75C0, &Clump_Destroy);
RWATOMIC_REVERSED("RpClump::Clone", 0x006C7630, &Clump_Clone);
RWATOMIC_REVERSED("RwFrame::SetHierarchy", 0x006C76D0, &Frame_SetHierarchy);
RWATOMIC_REVERSED("RwFrame::GetHierarchy", 0x006C7700, &Frame_GetHierarchy);
RWATOMIC_REVERSED("RpHAnimHierarchy::GetMatrices", 0x006C7710, &Hierarchy_GetMatrices);
RWATOMIC_REVERSED("RwFrame::ResetBone", 0x006C7720, &Frame_ResetBone);
