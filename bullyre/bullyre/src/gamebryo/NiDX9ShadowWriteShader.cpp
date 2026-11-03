// NiDX9ShadowWriteShader (vtable 0x99BE00, NiRTTI 0x20E6DA4 on NiDX9FragmentShader's 0x20E7054;
// 0x70 bytes): the fragment shader the shadow write materials draw shadow maps with. It only
// changes the constructor and PreProcessPipeline (the cull mode from its own table, 0xBCB808, when
// the flag at 0xBCB804 asks).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kShadowWriteShaderVtable = 0x0099BE00;
GAME_VAR(uint8_t, ms_bUseShadowCullModes, 0x00BCB804);
GAME_ARRAY(uint32_t, ms_auiShadowCullModes, 0x00BCB808);   // two per stencil draw mode
GAME_VAR(void*, NiRTTI_NiDX9ShadowWriteShader, 0x020E6DA4);

GAME_FN(void*, __thiscall, FragmentShader_Construct, 0x008941D0, void* self, void* materialDescriptor);
GAME_FN(void, __thiscall, FragmentShader_Destruct, 0x00893F30, void* self);
// NiD3DShader::PreProcessPipeline (through NiDX9FragmentShader's thunk at 0x893160)
GAME_FN(uint32_t, __thiscall, FragmentShader_PreProcessPipeline, 0x00893160, void* self, void* geometry, void* skin,
        void* bufferData, void* properties, void* effects, const void* world, const void* bound);

} // namespace

// 0x884B30 (thiscall): NiDX9ShadowWriteShader::NiDX9ShadowWriteShader
uint8_t* __fastcall ShadowWriteShader_Construct(uint8_t* self, re::Edx, void* materialDescriptor) {
    FragmentShader_Construct(self, materialDescriptor);
    At<uint32_t>(self, 0) = kShadowWriteShaderVtable;
    return self;
}

// 0x884B50 (thiscall): NiDX9ShadowWriteShader::GetRTTI
void* __fastcall ShadowWriteShader_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9ShadowWriteShader;
}

// 0x884B60 (thiscall): NiDX9ShadowWriteShader's deleting destructor
uint8_t* __fastcall ShadowWriteShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kShadowWriteShaderVtable;
    FragmentShader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x70);
    }
    return self;
}

// 0x884B90 (thiscall, slot 6): NiDX9ShadowWriteShader::PreProcessPipeline (the base's, then the
// cull mode by the stencil property's draw mode from the shadow table, the render state's flip
// picking the column)
uint32_t __fastcall ShadowWriteShader_PreProcessPipeline(uint8_t* self, re::Edx, void* geometry, void* skin,
                                                         void* bufferData, uint8_t* properties, void* effects,
                                                         const void* world, const void* bound) {
    uint32_t result =
        FragmentShader_PreProcessPipeline(self, geometry, skin, bufferData, properties, effects, world, bound);
    if (ms_bUseShadowCullModes != 0) {
        uint32_t mode = (At<uint16_t>(At<uint8_t*>(properties, 0x24), 0x18) >> 10) & 3;
        uint8_t* state = At<uint8_t*>(ms_pkRenderer, kRenderState);
        uint32_t cull = ms_auiShadowCullModes[(At<uint32_t>(state, 0xF4) != 0 ? 1 : 0) + mode * 2];
        uint32_t& cached = At<uint32_t>(state, 0x1D0);
        if (cached != cull) {
            Com<long>(At<void*>(state, 0x2238), 57, 22u, cull);   // SetRenderState(CULLMODE)
            cached = cull;
        }
    }
    return result;
}

#define C39E_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39E_REVERSED("NiDX9ShadowWriteShader::NiDX9ShadowWriteShader", 0x00884B30, ShadowWriteShader_Construct);
C39E_REVERSED("NiDX9ShadowWriteShader::GetRTTI", 0x00884B50, ShadowWriteShader_GetRTTI);
C39E_REVERSED("NiDX9ShadowWriteShader::DeletingDestructor", 0x00884B60, ShadowWriteShader_DeletingDestructor);
C39E_REVERSED("NiDX9ShadowWriteShader::PreProcessPipeline", 0x00884B90, ShadowWriteShader_PreProcessPipeline);

} // namespace ni::c39
