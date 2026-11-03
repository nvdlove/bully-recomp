// Two helpers of the D3DX effect parsing code (0x74C690, 0x74F390) placed at the start of the
// system library's code.

#include "gamebryo/NiSystem.h"

namespace ni {

// 0x7520C0: float2 or float3 rows widened to float4 (zeros after); only for one register
// class (1); whether it did. (The game copies through the x87: a signalling NaN would come out
// quiet.)
uint8_t __cdecl NiD3DXEffect_ExpandToVector4(const float* from, float* to, int registerClass, uint32_t columns,
                                             uint32_t count) {
    if (registerClass != 1) {
        return 0;
    }
    if (columns == 2) {
        for (uint32_t i = 0; i < count; i++) {
            to[i * 4 + 0] = from[i * 2 + 0];
            to[i * 4 + 1] = from[i * 2 + 1];
            to[i * 4 + 2] = 0.0f;
            to[i * 4 + 3] = 0.0f;
        }
        return 1;
    }
    if (columns == 3) {
        for (uint32_t i = 0; i < count; i++) {
            to[i * 4 + 0] = from[i * 3 + 0];
            to[i * 4 + 1] = from[i * 3 + 1];
            to[i * 4 + 2] = from[i * 3 + 2];
            to[i * 4 + 3] = 0.0f;
        }
        return 1;
    }
    return 0;
}

// 0x7522E0: the parser's scratch technique description (0x2C bytes), cleared
void* __cdecl NiD3DXEffect_ClearTechniqueDesc() {
    void* desc = reinterpret_cast<void*>(0x020E207C);
    crt::Memset(desc, 0, 0x2C);
    return desc;
}

#define NID3DXUTIL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NID3DXUTIL_REVERSED("NiD3DXEffect::ExpandToVector4", 0x007520C0, NiD3DXEffect_ExpandToVector4);
NID3DXUTIL_REVERSED("NiD3DXEffect::ClearTechniqueDesc", 0x007522E0, NiD3DXEffect_ClearTechniqueDesc);

} // namespace ni
