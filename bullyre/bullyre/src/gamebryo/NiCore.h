#pragma once

// Gamebryo as Bully's exe has it (a 2.x build with Bully's own additions, no C++ RTTI: its
// classes name themselves through NiRTTI, tools/gamebryo_vtables.txt), the core every other
// class builds on. Layouts read from the constructors (NiRefObject 0x755260, NiObjectNET
// 0x764550, NiAVObject 0x75E750, NiNode 0x760780) and checked with static_asserts.
//
// The reachable part of the library (tools/gamebryo_reached.txt, what the game's code gets to)
// is written over src/gamebryo/ like the game's code: one function per address, registered
// (default original), the rest called through the game's addresses.

#include <cstddef>
#include <cstdint>
#include <intrin.h>

#include "reversed.h"

namespace ni {

struct NiPoint3 {
    float x, y, z;
};
static_assert(sizeof(NiPoint3) == 0xC);

struct NiMatrix3 {
    float m[3][3];
};
static_assert(sizeof(NiMatrix3) == 0x24);

// rotate, translate, scale (0x770920 makes the identity)
struct NiTransform {
    NiMatrix3 rotate;     // +0x0
    NiPoint3 translate;   // +0x24
    float scale;          // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

struct NiBound {
    NiPoint3 center;   // +0x0
    float radius;      // +0xC
};
static_assert(sizeof(NiBound) == 0x10);

// NiFixedString: a pointer to the characters of a string in the global string table, whose
// header (reference count, length) is just before them (0x754760 makes one, 0x754550 lets
// one go)
#if defined(_WIN64)
using NiFixedString = re::Ptr32<const char>;   // (the game's 4-byte handle, in fields and locals alike)
#else
using NiFixedString = const char*;
#endif

// NiTArray<T> and its kinds (NiTPrimitiveArray, NiTObjectArray): the vtable, the elements,
// the allocated and used counts, the "first empty" count and the growth step
template <typename T>
struct NiTArray {
    re::Ptr32<void> vtable;        // +0x0
    re::Ptr32<re::Field<T>> base;  // +0x4 (re::Field: a T* element is the game's 4-byte pointer)
    uint16_t maxSize;    // +0x8
    uint16_t size;       // +0xA (the highest used slot + 1)
    uint16_t esize;      // +0xC (slots in use)
    uint16_t growBy;     // +0xE
};
static_assert(sizeof(NiTArray<void*>) == 0x10);

// NiTPointerList<T>: the vtable (its allocator's: slot 2 frees a node), the ends, the count.
// A node: next, previous, the element
template <typename T>
struct NiTListNode {
    re::Ptr32<NiTListNode> next;   // +0x0
    re::Ptr32<NiTListNode> prev;   // +0x4
    re::Field<T> element;          // +0x8
};
template <typename T>
struct NiTPointerList {
    re::Ptr32<void> vtable;              // +0x0
    re::Ptr32<NiTListNode<T>> head;      // +0x4
    re::Ptr32<NiTListNode<T>> tail;      // +0x8
    uint32_t count;            // +0xC
};
static_assert(sizeof(NiTPointerList<void*>) == 0x10);

// ---- the object hierarchy

// NiRefObject (vtable 0x8FEFD4): slot 0 the destructor (scalar deleting), slot 1 DeleteThis
// (what the last DecRefCount calls). Every one constructed counts in ms_uiObjects (0x20E2168)
struct NiRefObject {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t refCount;   // +0x4 (changed with Interlocked*)
};
static_assert(sizeof(NiRefObject) == 8);

// NiObject (vtable 0x951038): the streamable base; GetRTTI is slot 2
struct NiObject : NiRefObject {};
static_assert(sizeof(NiObject) == 8);

struct NiExtraData;
struct NiTimeController;
struct NiProperty;
struct NiNode;
struct NiGeometry;

// NiObjectNET (vtable 0x951F10): a named object with controllers and extra data
struct NiObjectNET : NiObject {
    re::Ptr32<const char> name;            // +0x8
    re::Ptr32<NiTimeController> controller;  // +0xC (the first; they chain)
    re::Ptr32<re::Ptr32<NiExtraData>> extraData;       // +0x10
    uint16_t extraDataSize;        // +0x14
    uint16_t extraDataMax;         // +0x16
};
static_assert(sizeof(NiObjectNET) == 0x18);

// NiAVObject (vtable 0x951A70): a node of the scene graph
struct NiAVObject : NiObjectNET {
    re::Ptr32<NiNode> parent;                          // +0x18
    NiBound worldBound;                      // +0x1C
    NiTransform local;                       // +0x2C
    NiTransform world;                       // +0x60
    NiTPointerList<NiProperty*> properties;  // +0x94 (vtable 0x95127C; references held)
    re::Ptr32<NiObject> collisionObject;               // +0xA4 (a reference)
    uint16_t flags;                          // +0xA8 (0x1 app culled, 0x2 selective update,
                                             //  0x4 its transforms, 0x8 its property
                                             //  controllers, 0x10 rigid, 0x80 never rigid,
                                             //  0x100 a node; 0x10 set by the constructor)
    uint16_t unknownAA;
    float unknownAC;                         // +0xAC (1.0: Bully's)
    uint32_t unknownB0;                      // +0xB0 (Bully's)
    NiTArray<NiGeometry*> geometries;        // +0xB4 (Bully's: NiTPrimitiveArray<NiGeometry*>)
};
static_assert(offsetof(NiAVObject, parent) == 0x18);
static_assert(offsetof(NiAVObject, local) == 0x2C);
static_assert(offsetof(NiAVObject, world) == 0x60);
static_assert(offsetof(NiAVObject, properties) == 0x94);
static_assert(offsetof(NiAVObject, flags) == 0xA8);
static_assert(offsetof(NiAVObject, geometries) == 0xB4);
static_assert(sizeof(NiAVObject) == 0xC4);

// NiNode (vtable 0x951BF8). 0xF4 bytes: CreateClone (0x760A10) and the game's code allocate
// that much; the constructor leaves the bound at +0xE4 alone
struct NiNode : NiAVObject {
    NiTArray<NiAVObject*> children;               // +0xC4 (NiTObjectArray: references held)
    NiTPointerList<NiObject*> effects;            // +0xD4 (vtable 0x952E10: the dynamic effects)
    NiBound bound;                                // +0xE4 (model space: UpdateRigidDownwardPass
                                                  //  makes the world bound from it)
};
static_assert(offsetof(NiNode, children) == 0xC4);
static_assert(offsetof(NiNode, effects) == 0xD4);
static_assert(offsetof(NiNode, bound) == 0xE4);
static_assert(sizeof(NiNode) == 0xF4);

// ---- references (NiPointer's behaviour)

inline re::Ptr32<void>* VtableOf(const void* object) {
    return *static_cast<const re::Ptr32<re::Ptr32<void>>*>(object);   // (the game's 4-byte vtable pointer)
}

// IncRefCount
inline void AddRef(NiRefObject* object) {
    if (object != nullptr) {
        _InterlockedIncrement(reinterpret_cast<volatile long*>(&object->refCount));
    }
}

// DecRefCount: DeleteThis (slot 1, thiscall) when it was the last
inline void Release(NiRefObject* object) {
    if (object != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(&object->refCount)) == 0) {
        re::PtrCast<void(__thiscall*)(NiRefObject*)>(VtableOf(object)[1])(object);
    }
}

// a reference slot assigned (NiPointer::operator=): the new one held first, the old released
template <typename T>
inline void Assign(re::Ptr32<T>& slot, T* value) {
    if (slot != value) {
        AddRef(value);
        T* old = slot;
        slot = value;
        Release(old);
    }
}
#if defined(_WIN64)
// (the value read from a Ptr32 field: a template can't deduce T* through it)
template <typename T, typename U>
inline void Assign(re::Ptr32<T>& slot, re::Ptr32<U> value) {
    Assign(slot, static_cast<T*>(value.get()));
}
#endif

// ---- memory (NiMemObject's operators, NiMalloc/NiFree: the game's addresses)

GAME_FN(void*, __cdecl, NiNew, 0x00752460, uint32_t size);              // NiMemObject::operator new
GAME_FN(void, __cdecl, NiDelete, 0x00752540, void* p, uint32_t size);   // NiMemObject::operator delete
GAME_FN(void*, __cdecl, NiMalloc, 0x00752300, uint32_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);

} // namespace ni
