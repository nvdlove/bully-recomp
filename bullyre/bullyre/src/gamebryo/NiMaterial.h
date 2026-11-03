#pragma once

// NiMaterial (vtable 0x951DC4, 0xC bytes) and NiMaterialLibrary (vtable 0x951DA8, 0xC bytes):
// NiRefObjects with a name. Every material is in the global map by name (ms_pkMaterials,
// 0x20E2704: an NiTFixedStringMap<NiMaterial*>), every library in the global array
// (0x20E25F4: data, allocated 0x20E25F8, count 0x20E25FC). From the constructors at 0x762CD0
// and 0x7628C0.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiMaterial : NiRefObject {
    re::Ptr32<const char> name;   // +0x8
};
static_assert(sizeof(NiMaterial) == 0xC);

// slot 2 CreateMaterial(kName) (the base's: none)
struct NiMaterialLibrary : NiRefObject {
    re::Ptr32<const char> name;   // +0x8
};
static_assert(sizeof(NiMaterialLibrary) == 0xC);

} // namespace ni
