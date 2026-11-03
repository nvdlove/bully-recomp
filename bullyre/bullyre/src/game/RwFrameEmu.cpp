// The RenderWare emulation over Gamebryo: RwFrame (0xA8): +0 type, +3 private flags (1|2
// on the dirty list, 4|8 its transforms stale), +4 the parent, +8 a name (NiFixedString),
// +0xC / +0x10 the dirty-list link (the list's sentinel at 0xBC5BC8), +0x14 the modelling
// transform (0x34), +0x48 the world transform (LTM), +0x7C the ring of objects on it (an
// object's link at its +8, its sync callback at its +0x10), +0x84 the first child, +0x88
// the next sibling, +0x8C the root (a clone while cloning), +0x90 the bone id, +0x94 its
// bone hierarchy, +0x98, +0x9C, +0xA0, +0xA4 a serial number. Created, destroyed,
// cloned, linked, transformed, and the dirty frames synced. 0x6C9A30..0x6CA4A2.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

inline uint8_t* const kDirtyList = reinterpret_cast<uint8_t*>(0x00BC5BC8);   // next, previous
GAME_VAR(char*, rwf_emptyString, 0x00951028);
GAME_VAR(void*, rwf_stringTable, 0x020E216C);
GAME_VAR(uint32_t, rwf_nextSerial, 0x020C5F64);

GAME_FN(void, __cdecl, Rwf_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, Rwf_PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, Rwf_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Rwf_Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, Rwf_FixedStringAssign, 0x00401CD0, re::Ptr32<char>* self, re::Ptr32<char>* other);
GAME_FN(void, __thiscall, Rwf_StringTableFree, 0x00754550, void* table, re::Ptr32<char>* string, uint32_t length);
GAME_FN(void, __thiscall, Rwf_MatrixIdentity, 0x00770920, void* self);
GAME_FN(void, __thiscall, Rwf_Orthonormalise, 0x00768CD0, void* self);
GAME_FN(float*, __thiscall, Rwf_TransformMultiply, 0x00406830, const void* self, float* out, const void* other);
GAME_FN(void*, __cdecl, Rwf_TransformCombine, 0x00415E30, void* transform, const float* other, int mode);
GAME_FN(void*, __cdecl, Rwf_TransformRotate, 0x00415EB0, void* transform, const float* axis, float degrees,
        int mode);
GAME_FN(uint8_t*, __cdecl, Rwf_ResetBone, 0x006C7720, uint8_t* frame);
GAME_FN(uint8_t*, __cdecl, Rwf_CopyBone, 0x006C7FA0, uint8_t* frame, uint8_t* source);
GAME_FN(uint8_t*, __cdecl, Rwf_ClearHierarchy, 0x006C8040, uint8_t* frame);
GAME_FN(void, __cdecl, Rwf_SyncObjects, 0x006C9A30, uint8_t* frame);
GAME_FN(uint8_t*, __cdecl, Rwf_UpdateObjects, 0x006C9AF0, uint8_t* frame);
GAME_FN(void, __cdecl, Rwf_SetRoot, 0x006C9C40, uint8_t* frame, uint8_t* root);
GAME_FN(uint8_t*, __cdecl, Rwf_RemoveChild, 0x006C9CB0, uint8_t* frame);
GAME_FN(void, __cdecl, Rwf_SyncHierarchy, 0x006C9EF0, uint8_t* frame, uint32_t flags);
GAME_FN(void, __cdecl, Rwf_UpdateLTMs, 0x006CA030, uint8_t* frame, uint32_t flags);
GAME_FN(void, __cdecl, Rwf_DestroyRecursive, 0x006CA2B0, uint8_t* frame);
GAME_FN(uint8_t*, __cdecl, Rwf_CloneRecursive, 0x006CA340, uint8_t* frame, uint8_t* root);

inline void ReleaseFixedString(re::Ptr32<char>* s) {
    if (*s == rwf_emptyString) {
        return;
    }
    uint32_t length = *reinterpret_cast<uint16_t*>(*s - 2);
    if (vc::Decrement(*s - 8) == 1) {
        Rwf_StringTableFree(rwf_stringTable, s, length);
    }
}

// each object on the frame told through its sync callback
inline void SyncFrameObjects(uint8_t* frame) {
    uint8_t* ring = frame + 0x7C;
    for (uint8_t* link = At<uint8_t*>(frame, 0x7C); link != ring; link = At<uint8_t*>(link, 0)) {
        uint8_t* object = link - 8;
        using Sync = void*(__cdecl*)(uint8_t*);
        At<Sync>(object, 0x10)(object);
    }
}

// a frame taken off the dirty list
inline void UnlinkDirty(uint8_t* frame) {
    uint8_t* prev = At<uint8_t*>(frame, 0x10);
    At<uint8_t*>(prev, 0) = At<uint8_t*>(frame, 0xC);
    uint8_t* next = At<uint8_t*>(frame, 0xC);
    At<uint8_t*>(next, 4) = At<uint8_t*>(frame, 0x10);
}

// a root put on the dirty list (at its front) unless it's on it, and marked
inline void MarkDirty(uint8_t* frame) {
    uint8_t* root = At<uint8_t*>(frame, 0x8C);
    if (!(At<uint8_t>(root, 3) & 3)) {
        At<uint8_t*>(root, 0xC) = At<uint8_t*>(kDirtyList, 0);
        At<uint8_t*>(At<uint8_t*>(frame, 0x8C), 0x10) = kDirtyList;
        At<uint8_t*>(At<uint8_t*>(kDirtyList, 0), 4) = At<uint8_t*>(frame, 0x8C) + 0xC;
        At<uint8_t*>(kDirtyList, 0) = At<uint8_t*>(frame, 0x8C) + 0xC;
    }
    At<uint8_t>(At<uint8_t*>(frame, 0x8C), 3) |= 3;
    At<uint8_t>(frame, 3) |= 0xC;
}

// 0x6C9A30: the objects of a frame tree synced (the frames' 8 flag cleared)
void __cdecl SyncObjects(uint8_t* frame) {
    for (uint8_t* f = frame; f != nullptr; f = At<uint8_t*>(f, 0x88)) {
        SyncFrameObjects(f);
        At<uint8_t>(f, 3) &= 0xF7;
        Rwf_SyncObjects(At<uint8_t*>(f, 0x84));
    }
}

// 0x6C9A90: RwFrameGetMatrix (the modelling transform)
uint8_t* __cdecl GetMatrix(uint8_t* frame) {
    return frame + 0x14;
}

// 0x6C9AA0: RwFrameForAllObjects
uint8_t* __cdecl ForAllObjects(uint8_t* frame, void* callback, void* data) {
    uint8_t* ring = frame + 0x7C;
    uint8_t* link = At<uint8_t*>(frame, 0x7C);
    if (link == ring) {
        return frame;
    }
    using Callback = void*(__cdecl*)(uint8_t*, void*);
    do {
        uint8_t* next = At<uint8_t*>(link, 0);
        if (reinterpret_cast<Callback>(callback)(link - 8, data) == nullptr) {
            break;
        }
        link = next;
    } while (link != ring);
    return frame;
}

// 0x6C9AF0: RwFrameUpdateObjects (the root onto the dirty list)
uint8_t* __cdecl UpdateObjects(uint8_t* frame) {
    MarkDirty(frame);
    return frame;
}

// 0x6C9B50: RwFrameForAllChildren
uint8_t* __cdecl ForAllChildren(uint8_t* frame, void* callback, void* data) {
    using Callback = void*(__cdecl*)(uint8_t*, void*);
    uint8_t* child = At<uint8_t*>(frame, 0x84);
    while (child != nullptr) {
        uint8_t* next = At<uint8_t*>(child, 0x88);
        if (reinterpret_cast<Callback>(callback)(child, data) == nullptr) {
            break;
        }
        child = next;
    }
    return frame;
}

// 0x6C9BA0: whether the root is on the dirty list
int __cdecl IsDirty(uint8_t* frame) {
    return (At<uint8_t>(At<uint8_t*>(frame, 0x8C), 3) & 3) != 0 ? 1 : 0;
}

// 0x6C9BC0: RwFrameGetRoot
uint8_t* __cdecl GetRoot(uint8_t* frame) {
    return At<uint8_t*>(frame, 0x8C);
}

// 0x6C9BD0: an object's frame set (off its old frame's ring, onto the new one's front)
void __cdecl SetObjectFrame(uint8_t* object, uint8_t* frame) {
    if (At<void*>(object, 4) != nullptr) {
        uint8_t* prev = At<uint8_t*>(object, 0xC);
        At<uint8_t*>(prev, 0) = At<uint8_t*>(object, 8);
        uint8_t* next = At<uint8_t*>(object, 8);
        At<uint8_t*>(next, 4) = At<uint8_t*>(object, 0xC);
    }
    At<uint8_t*>(object, 4) = frame;
    if (frame == nullptr) {
        return;
    }
    uint8_t* ring = frame + 0x7C;
    uint8_t* link = object + 8;
    At<uint8_t*>(link, 0) = At<uint8_t*>(ring, 0);
    At<uint8_t*>(object, 0xC) = ring;
    At<uint8_t*>(At<uint8_t*>(ring, 0), 4) = link;
    At<uint8_t*>(ring, 0) = link;
    Rwf_UpdateObjects(frame);
}

// 0x6C9C20: an object taken off its frame's ring (its frame field kept)
uint8_t* __cdecl DetachObject(uint8_t* object) {
    if (At<void*>(object, 4) != nullptr) {
        uint8_t* prev = At<uint8_t*>(object, 0xC);
        At<uint8_t*>(prev, 0) = At<uint8_t*>(object, 8);
        uint8_t* next = At<uint8_t*>(object, 8);
        At<uint8_t*>(next, 4) = At<uint8_t*>(object, 0xC);
    }
    return object;
}

// 0x6C9C40: the root set through a tree
void __cdecl SetRoot(uint8_t* frame, uint8_t* root) {
    At<uint8_t*>(frame, 0x8C) = root;
    for (uint8_t* child = At<uint8_t*>(frame, 0x84); child != nullptr; child = At<uint8_t*>(child, 0x88)) {
        Rwf_SetRoot(child, root);
    }
}

// 0x6C9C80: the roots of a tree set from its parent's (itself without one)
uint8_t* __cdecl ResetRoots(uint8_t* frame) {
    uint8_t* parent = At<uint8_t*>(frame, 4);
    Rwf_SetRoot(frame, parent != nullptr ? static_cast<uint8_t*>(At<uint8_t*>(parent, 0x8C)) : frame);
    return frame;
}

// 0x6C9CB0: RwFrameRemoveChild (the frame made its own root and dirtied)
uint8_t* __cdecl RemoveChild(uint8_t* frame) {
    uint8_t* parent = At<uint8_t*>(frame, 4);
    uint8_t* first = At<uint8_t*>(parent, 0x84);
    if (first == frame) {
        At<uint8_t*>(parent, 0x84) = At<uint8_t*>(frame, 0x88);
    } else {
        uint8_t* prev = first;
        uint8_t* sibling = At<uint8_t*>(first, 0x88);
        while (sibling != frame) {
            prev = sibling;
            sibling = At<uint8_t*>(sibling, 0x88);
        }
        At<uint8_t*>(prev, 0x88) = At<uint8_t*>(frame, 0x88);
    }
    uint8_t* child = At<uint8_t*>(frame, 0x84);
    At<uint8_t*>(frame, 4) = nullptr;
    At<uint8_t*>(frame, 0x88) = nullptr;
    At<uint8_t*>(frame, 0x8C) = frame;
    for (; child != nullptr; child = At<uint8_t*>(child, 0x88)) {
        Rwf_SetRoot(child, frame);
    }
    MarkDirty(frame);
    return frame;
}

// 0x6C9D80: RwFrameAddChild (at the front; off the dirty list and dirtied)
uint8_t* __cdecl AddChild(uint8_t* parent, uint8_t* child) {
    if (At<void*>(child, 4) != nullptr) {
        Rwf_RemoveChild(child);
    }
    At<uint8_t*>(child, 0x88) = At<uint8_t*>(parent, 0x84);
    At<uint8_t*>(parent, 0x84) = child;
    At<uint8_t*>(child, 4) = parent;
    Rwf_SetRoot(child, At<uint8_t*>(parent, 0x8C));
    if (At<uint8_t>(child, 3) & 3) {
        UnlinkDirty(child);
        At<uint8_t>(child, 3) &= 0xFC;
    }
    Rwf_UpdateObjects(child);
    return parent;
}

// 0x6C9DF0 (the frame in esi: not hookable): a frame's teardown: its hierarchy let go of,
// off its parent and the dirty list, its children orphaned
void Destruct(uint8_t* frame) {
    Rwf_ClearHierarchy(frame);
    if (At<void*>(frame, 4) != nullptr) {
        Rwf_RemoveChild(frame);
    }
    if (At<uint8_t>(frame, 3) & 3) {
        UnlinkDirty(frame);
    }
    for (uint8_t* child = At<uint8_t*>(frame, 0x84); child != nullptr; child = At<uint8_t*>(child, 0x88)) {
        At<uint8_t*>(child, 4) = nullptr;
    }
}

// 0x6C9E50: RwFrameOrthoNormalize (the transform copied onto itself first)
uint8_t* __cdecl OrthoNormalize(uint8_t* frame) {
    uint32_t* m = reinterpret_cast<uint32_t*>(frame + 0x14);
    for (int i = 0; i < 13; i++) {
        m[i] = m[i];
    }
    Rwf_Orthonormalise(m);
    Rwf_UpdateObjects(frame);
    return frame;
}

// 0x6C9E80 (the frame in esi: not hookable): a frame's fields set up
void Init(uint8_t* frame) {
    At<uint32_t>(frame, 0xA4) = rwf_nextSerial;
    rwf_nextSerial += 1;
    At<uint8_t>(frame, 0) = 0;
    At<uint8_t>(frame, 1) = 0;
    At<uint8_t>(frame, 2) = 0;
    At<uint8_t>(frame, 3) = 0;
    At<void*>(frame, 4) = nullptr;
    At<uint8_t*>(frame, 0x7C) = frame + 0x7C;
    At<uint8_t*>(frame, 0x80) = frame + 0x7C;
    Rwf_MatrixIdentity(frame + 0x14);
    Rwf_MatrixIdentity(frame + 0x48);
    At<void*>(frame, 0x84) = nullptr;
    At<void*>(frame, 0x88) = nullptr;
    At<uint8_t*>(frame, 0x8C) = frame;
    At<uint32_t>(frame, 0x98) = 0;
    At<uint32_t>(frame, 0x9C) = 0;
    At<uint8_t>(frame, 0xA0) = 0;
    Rwf_ResetBone(frame);
}

// the world transform from the parent's and the modelling one
inline void UpdateLTM(uint8_t* frame) {
    float product[13];
    const uint32_t* from = reinterpret_cast<const uint32_t*>(
        Rwf_TransformMultiply(At<uint8_t*>(frame, 4) + 0x48, product, frame + 0x14));
    for (int i = 0; i < 13; i++) {
        At<uint32_t>(frame, 0x48 + i * 4) = from[i];
    }
}

// 0x6C9EF0: a tree's stale world transforms made and its objects synced
void __cdecl SyncHierarchy(uint8_t* frame, uint32_t flags) {
    for (uint8_t* f = frame; f != nullptr; f = At<uint8_t*>(f, 0x88)) {
        uint32_t own = uint32_t(At<uint8_t>(f, 3)) | flags;
        if (own & 4) {
            UpdateLTM(f);
        }
        SyncFrameObjects(f);
        At<uint8_t>(f, 3) &= 0xF3;
        Rwf_SyncHierarchy(At<uint8_t*>(f, 0x84), own);
    }
}

// 0x6C9F70: the dirty roots synced (their trees' transforms and objects), the list emptied
int __cdecl SyncDirty() {
    for (uint8_t* link = At<uint8_t*>(kDirtyList, 0); link != kDirtyList; link = At<uint8_t*>(link, 0)) {
        uint8_t* root = link - 0xC;
        uint8_t flags = At<uint8_t>(root, 3);
        if (flags & 1) {
            uint32_t stale = flags & 4;
            if (stale) {
                for (int i = 0; i < 13; i++) {
                    At<uint32_t>(root, 0x48 + i * 4) = At<uint32_t>(root, 0x14 + i * 4);
                }
            }
            SyncFrameObjects(root);
            Rwf_SyncHierarchy(At<uint8_t*>(root, 0x84), stale);
        } else {
            SyncFrameObjects(root);
            Rwf_SyncObjects(At<uint8_t*>(root, 0x84));
        }
        At<uint8_t>(root, 3) = flags & 0xF0;
    }
    At<uint8_t*>(kDirtyList, 0) = kDirtyList;
    At<uint8_t*>(kDirtyList, 4) = kDirtyList;
    return 1;
}

// 0x6CA030: a tree's stale world transforms made
void __cdecl UpdateLTMs(uint8_t* frame, uint32_t flags) {
    for (uint8_t* f = frame; f != nullptr; f = At<uint8_t*>(f, 0x88)) {
        uint32_t own = uint32_t(At<uint8_t>(f, 3)) | flags;
        if (own & 4) {
            UpdateLTM(f);
            At<uint8_t>(f, 3) &= 0xFB;
        }
        Rwf_UpdateLTMs(At<uint8_t*>(f, 0x84), own);
    }
}

// 0x6CA0A0: RwFrameGetLTM (the root's tree brought up to date first)
uint8_t* __cdecl GetLTM(uint8_t* frame) {
    uint8_t* root = At<uint8_t*>(frame, 0x8C);
    if (At<uint8_t>(root, 3) & 1) {
        uint8_t flags = At<uint8_t>(root, 3);
        if (flags & 4) {
            for (int i = 0; i < 13; i++) {
                At<uint32_t>(root, 0x48 + i * 4) = At<uint32_t>(root, 0x14 + i * 4);
            }
        }
        Rwf_UpdateLTMs(At<uint8_t*>(root, 0x84), flags);
        At<uint8_t>(root, 3) &= 0xFA;
    }
    return frame + 0x48;
}

// 0x6CA0F0: RwFrameTransform
uint8_t* __cdecl Transform(uint8_t* frame, const float* matrix, int mode) {
    Rwf_TransformCombine(frame + 0x14, matrix, mode);
    Rwf_UpdateObjects(frame);
    return frame;
}

// 0x6CA120: RwFrameScale (the scale matrix's translation left unset, as the game has it)
uint8_t* __cdecl Scale(uint8_t* frame, const float* scale, int mode) {
    float m[13];
    m[12] = 1.0f;
    Rwf_MatrixIdentity(m);
    m[0] = scale[0];
    m[4] = scale[1];
    m[8] = scale[2];
    Rwf_TransformCombine(frame + 0x14, m, mode);
    Rwf_UpdateObjects(frame);
    return frame;
}

// 0x6CA180: RwFrameTranslate
uint8_t* __cdecl Translate(uint8_t* frame, const float* by, int mode) {
    float m[13];
    m[12] = 1.0f;
    Rwf_MatrixIdentity(m);
    m[9] = by[0];
    m[10] = by[1];
    m[11] = by[2];
    Rwf_TransformCombine(frame + 0x14, m, mode);
    Rwf_UpdateObjects(frame);
    return frame;
}

// 0x6CA1E0: RwFrameRotate (degrees)
uint8_t* __cdecl Rotate(uint8_t* frame, const float* axis, float degrees, int mode) {
    Rwf_TransformRotate(frame + 0x14, axis, degrees, mode);
    Rwf_UpdateObjects(frame);
    return frame;
}

// 0x6CA210: RwFrameCreate (memory tag 4)
uint8_t* __cdecl Create() {
    Rwf_PushMemoryTag(4);
    uint8_t* frame = static_cast<uint8_t*>(Rwf_Malloc(0xA8));
    if (frame != nullptr) {
        At<float>(frame, 0x44) = 1.0f;
        At<char*>(frame, 8) = rwf_emptyString;
        At<float>(frame, 0x78) = 1.0f;
        Init(frame);
    }
    Rwf_PopMemoryTag();
    return frame;
}

// 0x6CA260: RwFrameDestroy (returns 1)
int __cdecl Destroy(uint8_t* frame) {
    Destruct(frame);
    if (frame != nullptr) {
        ReleaseFixedString(reinterpret_cast<re::Ptr32<char>*>(frame + 8));
        Rwf_Free(frame);
    }
    return 1;
}

// 0x6CA2B0: a tree destroyed, children first
void __cdecl DestroyRecursive(uint8_t* frame) {
    if (frame == nullptr) {
        return;
    }
    uint8_t* child = At<uint8_t*>(frame, 0x84);
    while (child != nullptr) {
        uint8_t* next = At<uint8_t*>(child, 0x88);
        Rwf_DestroyRecursive(child);
        child = next;
    }
    Rwf_ClearHierarchy(frame);
    if (At<uint8_t>(frame, 3) & 3) {
        UnlinkDirty(frame);
    }
    ReleaseFixedString(reinterpret_cast<re::Ptr32<char>*>(frame + 8));
    Rwf_Free(frame);
}

// 0x6CA340: a tree cloned (each source's +0x8C pointed at its clone meanwhile); the partly
// made clone destroyed when a child fails
uint8_t* __cdecl CloneRecursive(uint8_t* source, uint8_t* root) {
    uint8_t* frame = static_cast<uint8_t*>(Rwf_Malloc(0xA8));
    if (frame == nullptr) {
        return nullptr;
    }
    At<float>(frame, 0x44) = 1.0f;
    At<float>(frame, 0x78) = 1.0f;
    At<char*>(frame, 8) = rwf_emptyString;
    Init(frame);
    if (root == nullptr) {
        root = frame;
    }
    At<uint8_t>(frame, 0) = At<uint8_t>(source, 0);
    At<uint8_t>(frame, 1) = At<uint8_t>(source, 1);
    At<uint8_t>(frame, 2) = At<uint8_t>(source, 2);
    At<uint8_t>(frame, 3) = At<uint8_t>(source, 3);
    At<void*>(frame, 4) = nullptr;
    At<uint8_t*>(frame, 0x7C) = frame + 0x7C;
    At<uint8_t*>(frame, 0x80) = frame + 0x7C;
    Rwf_FixedStringAssign(reinterpret_cast<re::Ptr32<char>*>(frame + 8), reinterpret_cast<re::Ptr32<char>*>(source + 8));
    for (int i = 0; i < 13; i++) {
        At<uint32_t>(frame, 0x14 + i * 4) = At<uint32_t>(source, 0x14 + i * 4);
    }
    At<void*>(frame, 0x84) = nullptr;
    At<void*>(frame, 0x88) = nullptr;
    At<uint8_t*>(frame, 0x8C) = root;
    At<uint8_t*>(source, 0x8C) = frame;
    for (uint8_t* child = At<uint8_t*>(source, 0x84); child != nullptr; child = At<uint8_t*>(child, 0x88)) {
        uint8_t* clone = Rwf_CloneRecursive(child, root);
        if (clone == nullptr) {
            Rwf_DestroyRecursive(frame);
            return nullptr;
        }
        At<uint8_t*>(clone, 0x88) = At<uint8_t*>(frame, 0x84);
        At<uint8_t*>(frame, 0x84) = clone;
        At<uint8_t*>(clone, 4) = frame;
    }
    At<uint32_t>(frame, 0x98) = At<uint32_t>(source, 0x98);
    Rwf_ResetBone(frame);
    Rwf_CopyBone(frame, source);
    return frame;
}

// 0x6CA460: RwFrameCloneHierarchy (its flags cleared, dirtied)
uint8_t* __cdecl CloneHierarchy(uint8_t* source) {
    uint8_t* frame = Rwf_CloneRecursive(source, nullptr);
    if (frame == nullptr) {
        return nullptr;
    }
    At<uint8_t>(frame, 3) = 0;
    Rwf_UpdateObjects(frame);
    return frame;
}

// 0x6CA490: RwFrameDestroyHierarchy (returns 1)
int __cdecl DestroyHierarchy(uint8_t* frame) {
    Rwf_DestroyRecursive(frame);
    return 1;
}

} // namespace

RE_UNHOOKED("RwFrame::Destruct", 0x006C9DF0);
RE_UNHOOKED("RwFrame::Init", 0x006C9E80);

// (register hazards, regcheck2: 0x6C9C80 keeps the frame in ecx across 0x6C9C40 (calls at
// 0x6C9C93, 0x6C9CA0); 0x6C9CB0 keeps ecx and edx (the child walked) across it (0x6C9D16))
#define RWFRAME_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

RWFRAME_REVERSED("RwFrame::SyncObjects", 0x006C9A30, &SyncObjects);
RWFRAME_REVERSED("RwFrame::GetMatrix", 0x006C9A90, &GetMatrix);
RWFRAME_REVERSED("RwFrame::ForAllObjects", 0x006C9AA0, &ForAllObjects);
RWFRAME_REVERSED("RwFrame::UpdateObjects", 0x006C9AF0, &UpdateObjects);
RWFRAME_REVERSED("RwFrame::ForAllChildren", 0x006C9B50, &ForAllChildren);
RWFRAME_REVERSED("RwFrame::IsDirty", 0x006C9BA0, &IsDirty);
RWFRAME_REVERSED("RwFrame::GetRoot", 0x006C9BC0, &GetRoot);
RWFRAME_REVERSED("RwObject::SetFrame", 0x006C9BD0, &SetObjectFrame);
RWFRAME_REVERSED("RwObject::DetachFrame", 0x006C9C20, &DetachObject);
RWFRAME_REVERSED("RwFrame::SetRoot", 0x006C9C40, &SetRoot);   // (keep ecx, edx: callers 0x6C9C80, 0x6C9CB0)
RWFRAME_REVERSED("RwFrame::ResetRoots", 0x006C9C80, &ResetRoots);
RWFRAME_REVERSED("RwFrame::RemoveChild", 0x006C9CB0, &RemoveChild);
RWFRAME_REVERSED("RwFrame::AddChild", 0x006C9D80, &AddChild);
RWFRAME_REVERSED("RwFrame::OrthoNormalize", 0x006C9E50, &OrthoNormalize);
RWFRAME_REVERSED("RwFrame::SyncHierarchy", 0x006C9EF0, &SyncHierarchy);
RWFRAME_REVERSED("RwFrame::SyncDirty", 0x006C9F70, &SyncDirty);
RWFRAME_REVERSED("RwFrame::UpdateLTMs", 0x006CA030, &UpdateLTMs);
RWFRAME_REVERSED("RwFrame::GetLTM", 0x006CA0A0, &GetLTM);
RWFRAME_REVERSED("RwFrame::Transform", 0x006CA0F0, &Transform);
RWFRAME_REVERSED("RwFrame::Scale", 0x006CA120, &Scale);
RWFRAME_REVERSED("RwFrame::Translate", 0x006CA180, &Translate);
RWFRAME_REVERSED("RwFrame::Rotate", 0x006CA1E0, &Rotate);
RWFRAME_REVERSED("RwFrame::Create", 0x006CA210, &Create);
RWFRAME_REVERSED("RwFrame::Destroy", 0x006CA260, &Destroy);
RWFRAME_REVERSED("RwFrame::DestroyRecursive", 0x006CA2B0, &DestroyRecursive);
RWFRAME_REVERSED("RwFrame::CloneRecursive", 0x006CA340, &CloneRecursive);
RWFRAME_REVERSED("RwFrame::CloneHierarchy", 0x006CA460, &CloneHierarchy);
RWFRAME_REVERSED("RwFrame::DestroyHierarchy", 0x006CA490, &DestroyHierarchy);
