// NiSourceTexture's slots 12 PostLinkObject (with preloading on, the renderer data made under the
// texture lock) and 21 DestroyAppPixelData (shared with NiSourceCubeMap), and the LoadBinary the
// one-flag properties share (NiShadeProperty, NiWireframeProperty, NiSpecularProperty,
// NiDitherProperty: NiProperty's, then the flags).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSourceTexture.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

GAME_VAR(uint8_t, NiSourceTexture_ms_bPreload, 0x00BC7620);
inline NiCriticalSection& NiSourceTexture_ms_kCritSec = *reinterpret_cast<NiCriticalSection*>(0x020E248C);

GAME_FN(void, __thiscall, Property_LoadBinary, 0x007548C0, void* self, void* stream);

// 0x7DA320 NiShadeProperty::LoadBinary(stream)
void __fastcall ShadeProperty_LoadBinaryImpl(uint8_t* self, re::Edx, void* stream) {
    Property_LoadBinary(self, stream);
    scene::BinaryStream* in = At<scene::BinaryStream*>(stream, 0x188);
    uint32_t componentSize = 2;
    in->position += in->read(in, self + 0x18, 2, &componentSize, 1);
}

// 0x7DA550 vf21 DestroyAppPixelData: the source pixel data let go
void __fastcall SourceTexture_DestroyAppPixelDataImpl(NiSourceTexture* self, re::Edx) {
    NiObject* data = self->srcPixelData;
    if (data != nullptr) {
        if (Decrement(&data->refCount) == 0) {
            Virtual<void>(data, 1);
        }
        self->srcPixelData = nullptr;
    }
}

// 0x7DA930 vf12 PostLinkObject(stream): with preloading, CreateRendererData (slot 22) under
// the lock
void __fastcall SourceTexture_PostLinkObjectImpl(NiSourceTexture* self, re::Edx, void*) {
    if (NiSourceTexture_ms_bPreload == 0) {
        return;
    }
    Lock(NiSourceTexture_ms_kCritSec);
    Virtual<void>(self, 22);
    Unlock(NiSourceTexture_ms_kCritSec);
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISRCTEX2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISRCTEX2_REVERSED("NiShadeProperty::LoadBinary", 0x007DA320, ni::ShadeProperty_LoadBinaryImpl);
NISRCTEX2_REVERSED("NiSourceTexture::DestroyAppPixelData", 0x007DA550, ni::SourceTexture_DestroyAppPixelDataImpl);
NISRCTEX2_REVERSED("NiSourceTexture::PostLinkObject", 0x007DA930, ni::SourceTexture_PostLinkObjectImpl);
