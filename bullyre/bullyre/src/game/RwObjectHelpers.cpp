// Small RenderWare-style search helpers built on the for-all iterators: a clump's first
// atomic, a frame's first object, a dictionary's n-th texture, a frame's descendants.
// 0x5EDFF0..0x5EE1A0 (the callbacks at 0x5EE020, 0x5EE060, 0x5EE180 are the game's
// labels, passed as they are).

#include <cstdint>

#include "reversed.h"
#include "game/VCall.h"

namespace {

GAME_FN(void, __cdecl, RW_ForAllAtomics, 0x006C7380, void* object, void* callback, void* data);
GAME_FN(void, __cdecl, RW_ForAllFrameObjects, 0x006C9AA0, void* frame, void* callback, void* data);
GAME_FN(void, __cdecl, RW_ForAllTextures, 0x005F1590, void* dict, void* callback, void* data);
GAME_FN(void*, __cdecl, RW_ForAllChildren, 0x006C9B50, void* frame, void* callback, void* data);
GAME_FN(void*, __cdecl, RW_FrameFirstObject, 0x006C7700, void* frame);
GAME_FN(void*, __cdecl, RW_GetFrame, 0x006C7370, void* object);

void* const kStoreFirst = reinterpret_cast<void*>(0x005EE020);       // *data = it; stop
void* const kCountDown = reinterpret_cast<void*>(0x005EE060);        // {n, result}: the n-th
void* const kFrameOfFirst = reinterpret_cast<void*>(0x005EE180);     // *data = 0x6C7700(its +4); stop
void* const kFindObject = reinterpret_cast<void*>(0x005EE0C0);       // this file's
void* const kCountChildren = reinterpret_cast<void*>(0x005EE130);    // this file's

// 0x5EDFF0: the first atomic
void* __cdecl FirstAtomic(void* clump) {
    void* result = nullptr;
    RW_ForAllAtomics(clump, kStoreFirst, &result);
    return result;
}

// 0x5EE030: a frame's first object
void* __cdecl FirstFrameObject(void* frame) {
    void* result = nullptr;
    RW_ForAllFrameObjects(frame, kStoreFirst, &result);
    return result;
}

// 0x5EE080: a dictionary's n-th texture
void* __cdecl NthTexture(void* dict, int index) {
    struct {
        int count;
        re::Ptr32<void> result;
    } data = {index, nullptr};
    RW_ForAllTextures(dict, kCountDown, &data);
    return data.result;
}

// 0x5EE0B0: the byte at +0x34 (the rest of eax is the pointer's upper bytes)
uint8_t __cdecl Byte34(const uint8_t* object) {
    return object[0x34];
}

// 0x5EE0C0 (a for-all-children callback): the first object under a frame, depth first
void* __cdecl FindObjectCB(void* frame, re::Ptr32<void>* result) {
    void* object = RW_FrameFirstObject(frame);
    if (!object) {
        RW_ForAllChildren(frame, kFindObject, result);
        return frame;
    }
    *result = object;
    return nullptr;
}

// 0x5EE100: the first object below an object's frame
void* __cdecl FindObjectBelow(void* object) {
    void* result = nullptr;
    void* frame = RW_GetFrame(object);
    RW_ForAllChildren(frame, kFindObject, &result);
    return result;
}

// 0x5EE130 (a for-all-children callback): the descendants counted
void* __cdecl CountChildrenCB(void* frame, int* count) {
    RW_ForAllChildren(frame, kCountChildren, count);
    *count += 1;
    return frame;
}

// 0x5EE160 (a for-all-textures callback): the first texture, referenced
void* __cdecl TakeTextureCB(uint8_t* texture, re::Ptr32<void>* result) {
    vc::Increment(texture + 4);
    *result = texture;
    return nullptr;
}

// 0x5EE1A0: what 0x6C7700 gives for the first atomic's +4
void* __cdecl FirstAtomicFrameObject(void* clump) {
    void* result = nullptr;
    RW_ForAllAtomics(clump, kFrameOfFirst, &result);
    return result;
}

#define RWOBJHELP_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(RwObjHelpOriginal_, __LINE__);                                         \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(RwObjHelpOriginal_, __LINE__), \
             re::Mode::Original)

RWOBJHELP_REVERSED("RwHelpers::FirstAtomic", 0x005EDFF0, &FirstAtomic);
RWOBJHELP_REVERSED("RwHelpers::FirstFrameObject", 0x005EE030, &FirstFrameObject);
RWOBJHELP_REVERSED("RwHelpers::NthTexture", 0x005EE080, &NthTexture);
RWOBJHELP_REVERSED("RwHelpers::Byte34", 0x005EE0B0, &Byte34);
RWOBJHELP_REVERSED("RwHelpers::FindObjectCB", 0x005EE0C0, &FindObjectCB);
RWOBJHELP_REVERSED("RwHelpers::FindObjectBelow", 0x005EE100, &FindObjectBelow);
RWOBJHELP_REVERSED("RwHelpers::CountChildrenCB", 0x005EE130, &CountChildrenCB);
RWOBJHELP_REVERSED("RwHelpers::TakeTextureCB", 0x005EE160, &TakeTextureCB);
RWOBJHELP_REVERSED("RwHelpers::FirstAtomicFrameObject", 0x005EE1A0, &FirstAtomicFrameObject);

} // namespace
