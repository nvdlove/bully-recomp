// NiDepthStencilBuffer's GetRTTI and its slot 20 (the renderer's data made through the
// renderer's slot 34 when there's none yet).

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiDepthStencilBuffer_RTTI, 0x020E3698);
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);

} // namespace

// 0x7ABE00: GetRTTI (slot 2)
const RTTI* __fastcall NiDepthStencilBuffer_GetRTTI(void*, re::Edx) {
    return &NiDepthStencilBuffer_RTTI;
}

// 0x7ABEB0: CreateRendererData (slot 20): the renderer's (its slot 34, this and both arguments)
// when the buffer has no renderer data; true when it has or the renderer made it
uint8_t __fastcall NiDepthStencilBuffer_CreateRendererData(void* self, re::Edx, uint32_t a, uint32_t b) {
    if (At<void*>(self, 0x10) == nullptr && ms_pkRenderer != nullptr) {
        if (!Virtual<uint8_t>(ms_pkRenderer, 34, self, a, b)) {
            return 0;
        }
    }
    return 1;
}

#define NIDEPTHSTENCIL2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIDEPTHSTENCIL2_REVERSED("NiDepthStencilBuffer::GetRTTI", 0x007ABE00, NiDepthStencilBuffer_GetRTTI);
NIDEPTHSTENCIL2_REVERSED("NiDepthStencilBuffer::CreateRendererData", 0x007ABEB0, NiDepthStencilBuffer_CreateRendererData);

} // namespace ni
