// NiLODData (vtable 0x965328, 8 bytes: an NiObject with nothing of its own, the base of
// NiRangeLODData and NiScreenLODData): construction and destruction; and NiAVObjectPalette's
// viewer strings, next to it in the exe.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiLODData_RTTI, 0x020E4278);
GAME_VAR(RTTI, NiAVObjectPalette_RTTI, 0x020E42AC);
constexpr uintptr_t kNiLODDataVtable = 0x00965328;

// 0x7F7920
NiObject* __fastcall LODDataConstruct(NiObject* self, re::Edx) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNiLODDataVtable);
    return self;
}

// 0x7F7940 (a tail call into ~NiObject)
void __fastcall LODDataDestruct(NiObject* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiLODDataVtable);
    Object_Destruct(self);
}

// vf2
const RTTI* __fastcall LODDataGetRTTI(NiObject*, re::Edx) {
    return &NiLODData_RTTI;
}

// vf0 the deleting destructor
NiObject* __fastcall LODDataDeletingDestructor(NiObject* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kNiLODDataVtable);
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 8);
    }
    return self;
}

// NiAVObjectPalette vf9 GetViewerStrings
void __fastcall AVObjectPaletteGetViewerStrings(NiObject* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiAVObjectPalette_RTTI.name));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NILODDATA_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NILODDATA_REVERSED("NiLODData::NiLODData", 0x007F7920, ni::LODDataConstruct);
NILODDATA_REVERSED("NiLODData::~NiLODData", 0x007F7940, ni::LODDataDestruct);
NILODDATA_REVERSED("NiLODData::GetRTTI", 0x007F7950, ni::LODDataGetRTTI);
NILODDATA_REVERSED("NiLODData::DeletingDestructor", 0x007F7960, ni::LODDataDeletingDestructor);
NILODDATA_REVERSED("NiAVObjectPalette::GetViewerStrings", 0x007F79D0, ni::AVObjectPaletteGetViewerStrings);
