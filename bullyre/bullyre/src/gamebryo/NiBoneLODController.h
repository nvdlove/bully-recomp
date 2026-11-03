#pragma once

// NiBoneLODController (vtable 0x9678B8, 0x68 bytes, constructor 0x81E2B0): a skinned actor's bone
// levels of detail. Per level a set of bones (selectively updated at that level and every coarser
// one: the sets from the level's index up) and a set of skinned shapes with the skin instances
// they get at that level; the shapes of every level are kept apart too, to be culled. The global
// level (ms_iGlobalLOD) drives them all from Update. Everything in ni::anim.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "gamebryo/NiTimeController.h"
#include "reversed.h"

namespace ni::anim {

// the global level every controller follows (-1: none)
GAME_VAR(int32_t, ms_iGlobalLOD, 0x00BC8C34);

// a level's skinned shape and the skin instance it gets
struct BoneLODSkinInfo {
    re::Ptr32<NiAVObject> shape;          // +0x0 (a NiGeometry: its skin instance at +0xD0)
    re::Ptr32<NiObject> skinInstance;     // +0x4 (a reference)
};
static_assert(sizeof(BoneLODSkinInfo) == 8);

struct NiBoneLODController : NiTimeController {
    int32_t lod;                                     // +0x34 (-1: none set)
    uint32_t numLODs;                                // +0x38
    NiTArray<NiTSet<NiNode*>*> bones;                // +0x3C (NiTPrimitiveArray, vtable 0x961988;
                                                     //  each set NiNew'd, 0xC bytes)
    NiTArray<NiTSet<BoneLODSkinInfo*>*> skins;       // +0x4C (the same; each info NiNew'd)
    NiTSet<NiAVObject*> shapes;                      // +0x5C (every level's skinned shapes)
};
static_assert(offsetof(NiBoneLODController, bones) == 0x3C);
static_assert(offsetof(NiBoneLODController, skins) == 0x4C);
static_assert(offsetof(NiBoneLODController, shapes) == 0x5C);
static_assert(sizeof(NiBoneLODController) == 0x68);

}   // namespace ni::anim
