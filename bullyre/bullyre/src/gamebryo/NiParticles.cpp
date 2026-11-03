// NiParticles (vtable 0x961898, 0xEC bytes: NiGeometry's, nothing of its own): cloning, drawing
// (the renderer's RenderPoints, slot 97), its data marked volatile, viewer strings.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometry.h"
#include "gamebryo/NiGeometryData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiParticles_RTTI, 0x020E3F84);
constexpr uintptr_t kNiParticlesVtable = 0x00961898;

// NiGeometry's (another chunk's)
GAME_FN(void, __thiscall, Geometry_CopyMembers, 0x00761390, NiGeometry* self, NiGeometry* dest, void* cloning);
GAME_FN(void, __thiscall, Geometry_RenderImmediate, 0x00761550, NiGeometry* self, void* renderer);
GAME_FN(void, __thiscall, Geometry_GetViewerStrings, 0x00761C50, NiGeometry* self, NiTArray<char*>* strings);
GAME_FN(NiGeometry*, __thiscall, Geometry_Construct, 0x00761CA0, NiGeometry* self);

// vf2
const RTTI* __fastcall ParticlesGetRTTI(NiGeometry*, re::Edx) {
    return &NiParticles_RTTI;
}

// vf3 CreateClone(cloning): NiGeometry's CopyMembers (called on a failed allocation too)
NiGeometry* __fastcall ParticlesCreateClone(NiGeometry* self, re::Edx, void* cloning) {
    NiGeometry* clone = static_cast<NiGeometry*>(NiNew(0xEC));
    if (clone != nullptr) {
        Geometry_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(kNiParticlesVtable);
    } else {
        clone = nullptr;
    }
    Geometry_CopyMembers(self, clone, cloning);
    return clone;
}

// vf9
void __fastcall ParticlesGetViewerStrings(NiGeometry* self, re::Edx, NiTArray<char*>* strings) {
    Geometry_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiParticles_RTTI.name));
}

// vf34 RenderImmediate: the renderer's RenderPoints (slot 97)
void __fastcall ParticlesRenderImmediate(NiGeometry* self, re::Edx, void* renderer) {
    Geometry_RenderImmediate(self, renderer);
    Virtual<void>(renderer, 97, self);
}

// vf37 CalculateConsistency(bool): the data's consistency always volatile (0x8000 in bits 12..15)
void __fastcall ParticlesCalculateConsistency(NiGeometry* self, re::Edx, int) {
    NiGeometryData* data = re::PtrCast<NiGeometryData*>(self->modelData);
    data->dirtyFlags = uint16_t((data->dirtyFlags & 0x0FFF) | 0x8000);
}

} // namespace

// a jmp to NiGeometry::RegisterStreamables (0x7619E0): slot 6 of NiParticles, NiLines and
// NiTriBasedGeom
RE_UNHOOKED("NiParticles::RegisterStreamables", 0x007E0860);

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPARTICLES_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIPARTICLES_REVERSED("NiParticles::GetRTTI", 0x007E0870, ni::ParticlesGetRTTI);
NIPARTICLES_REVERSED("NiParticles::RenderImmediate", 0x007E0880, ni::ParticlesRenderImmediate);
NIPARTICLES_REVERSED("NiParticles::CalculateConsistency", 0x007E08A0, ni::ParticlesCalculateConsistency);
NIPARTICLES_REVERSED("NiParticles::CreateClone", 0x007E08C0, ni::ParticlesCreateClone);
NIPARTICLES_REVERSED("NiParticles::GetViewerStrings", 0x007E0910, ni::ParticlesGetViewerStrings);
