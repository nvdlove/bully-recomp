// NiSCMExtraData (vtable 0x9A1918, 0x30 bytes; NiRTTI 0x20E95F0): the shader constant map's
// extra data, 0x8CA390..0x8CA3F0: for each program kind (vertex, geometry, pixel: indexes 0, 1,
// 2) how many entries there's room for, how many are used, and the entries (8 bytes each,
// NiMalloc'd). Its constructor sizes the pixel entries by the vertex count: the game's bug.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiExtraData.h"
#include "reversed.h"

namespace ni {
namespace {

constexpr uintptr_t kNiSCMExtraDataVtable = 0x009A1918;

GAME_VAR(uint8_t, NiSCMExtraData_RTTI, 0x020E95F0);

struct NiSCMExtraData : NiExtraData {
    uint32_t entryCount[3];   // +0xC (vertex, geometry, pixel)
    uint32_t used[3];         // +0x18
    re::Ptr32<void> entries[3];         // +0x24
};
static_assert(offsetof(NiSCMExtraData, entries) == 0x24);
static_assert(sizeof(NiSCMExtraData) == 0x30);

GAME_FN(NiExtraData*, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, ExtraData_Destruct, 0x00765670, NiExtraData* self);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);

void* __fastcall GetRTTI(NiSCMExtraData*, re::Edx) {
    return &NiSCMExtraData_RTTI;
}

NiSCMExtraData* __fastcall DeletingDestructor(NiSCMExtraData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kNiSCMExtraDataVtable);
    NiFree(self->entries[0]);
    NiFree(self->entries[2]);
    NiFree(self->entries[1]);
    ExtraData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiSCMExtraData));
    }
    return self;
}

// the constructor: the name and the room for each kind's entries (none used yet)
NiSCMExtraData* __fastcall Construct(NiSCMExtraData* self, re::Edx, const re::Ptr32<const char>* name, uint32_t vertexCount,
                                     uint32_t pixelCount, uint32_t geometryCount) {
    ExtraData_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNiSCMExtraDataVtable);
    FixedString_Assign(&self->name, name);
    self->entryCount[0] = vertexCount;
    self->used[0] = 0;
    self->entries[0] = vertexCount != 0 ? NiMalloc(vertexCount * 8) : nullptr;
    self->entryCount[2] = pixelCount;
    self->used[2] = 0;
    self->entries[2] = vertexCount != 0 ? NiMalloc(vertexCount * 8) : nullptr;   // (sized by the vertex count)
    self->entryCount[1] = geometryCount;
    self->used[1] = 0;
    self->entries[1] = geometryCount != 0 ? NiMalloc(geometryCount * 8) : nullptr;
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISCMEXTRA_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISCMEXTRA_REVERSED("NiSCMExtraData::GetRTTI", 0x008CA390, &ni::GetRTTI);
NISCMEXTRA_REVERSED("NiSCMExtraData::DeletingDestructor", 0x008CA3A0, &ni::DeletingDestructor);
NISCMEXTRA_REVERSED("NiSCMExtraData::NiSCMExtraData", 0x008CA3F0, &ni::Construct);
