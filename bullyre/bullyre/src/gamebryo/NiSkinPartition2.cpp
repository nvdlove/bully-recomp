// NiSkinPartition's qsort comparisons (the vertex interactions by weight, the largest first; the
// bone indices ascending) and its Partition's constructor (vtable 0x952BFC, 0x2C bytes: the
// arrays none; the counts left as the memory had them, but the strips).

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSkinPartition.h"
#include "reversed.h"

namespace ni {
namespace {

constexpr uintptr_t Partition_vtable = 0x00952BFC;

// a vertex interaction: the bone, its weight
struct Interaction {
    uint32_t bone;   // +0x0
    float weight;    // +0x4
};

} // namespace

// 0x76DD90: the interactions by weight, the largest first
int __cdecl NiSkinPartition_CompareWeights(const void* a, const void* b) {
    const Interaction* x = static_cast<const Interaction*>(a);
    const Interaction* y = static_cast<const Interaction*>(b);
    if (y->weight < x->weight) {
        return -1;
    }
    if (x->weight < y->weight) {
        return 1;
    }
    return 0;
}

// 0x76DDE0: 16-bit values ascending
int __cdecl NiSkinPartition_CompareShorts(const void* a, const void* b) {
    uint16_t x = *static_cast<const uint16_t*>(a);
    uint16_t y = *static_cast<const uint16_t*>(b);
    if (x < y) {
        return -1;
    }
    return y < x ? 1 : 0;
}

// 0x76DE00: NiSkinPartition::Partition::Partition
NiSkinPartitionPart* __fastcall NiSkinPartition_PartitionConstruct(NiSkinPartitionPart* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(Partition_vtable);
    self->bones = nullptr;
    self->weights = nullptr;
    self->vertexMap = nullptr;
    self->bonePalette = nullptr;
    self->triList = nullptr;
    self->stripLengths = nullptr;
    self->strips = 0;
    self->buffData = nullptr;
    return self;
}

#define NISKINPART2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NISKINPART2_REVERSED("NiSkinPartition::CompareWeights", 0x0076DD90, NiSkinPartition_CompareWeights);
NISKINPART2_REVERSED("NiSkinPartition::CompareShorts", 0x0076DDE0, NiSkinPartition_CompareShorts);
NISKINPART2_REVERSED("NiSkinPartition::Partition::Partition", 0x0076DE00, NiSkinPartition_PartitionConstruct);

} // namespace ni
