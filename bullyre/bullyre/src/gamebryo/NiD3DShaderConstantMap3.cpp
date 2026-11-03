// NiD3DShaderConstantMap's constant and global constant setters (slots 17 and 19) and an entry's
// data as registers by its element count (the static buffers at 0x20E6EE0 (floats), 0x20E6F30
// (a bool), 0x20E6F34 (integers), 0x20E6F48 (a matrix)).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

GAME_ARRAY(uint32_t, ms_auiRegisters, 0x020E6EE0);
GAME_ARRAY(uint32_t, ms_auiBoolRegister, 0x020E6F30);
GAME_ARRAY(uint32_t, ms_auiIntRegisters, 0x020E6F34);
GAME_ARRAY(uint32_t, ms_auiMatrixRegisters, 0x020E6F48);   // 16
GAME_ARRAY(const uint32_t, ms_auiGlobalTypes, 0x020E408C);

GAME_FN(const void*, __thiscall, ConstantMap_PadRegisters, 0x0088C610, void* self, void* entry, const void* data);
GAME_FN(uint32_t, __thiscall, ConstantMap_SetEntryData, 0x0088BF50, void* self, void* program, void* entry, const void* data);
GAME_FN(const void*, __thiscall, ConstantMap_GetConstantData, 0x0088C200, void* self, void* entry);

inline uint32_t TypeOf(const uint8_t* entry) {
    return ms_auiGlobalTypes[At<uint32_t>(entry, 0x10) & 0xFF];
}

inline uint32_t Send(void* program, void* entry, const void* data) {
    return Virtual<uint8_t>(program, 14, entry, data, 0u) ? 0 : 0x50;
}

} // namespace

// 0x88C200 (stdcall): an entry's data as registers by its element count (1: a bool, an integer or
// a float broadcast; 2: twice; 3: with w 1; 4; 8 / 9 / 12 / 16: a matrix padded with zeros); null for
// other counts
const void* __fastcall ConstantMap_GetConstantDataImpl(void*, re::Edx, uint8_t* entry) {
    const uint32_t* p = At<const uint32_t*>(entry, 0x2C);
    uint32_t* reg = ms_auiRegisters;
    uint32_t* ints = ms_auiIntRegisters;
    uint32_t* m = ms_auiMatrixRegisters;
    switch (At<uint32_t>(entry, 0x24) / At<uint32_t>(entry, 0x28)) {
    case 1:
        if (TypeOf(entry) == 1) {
            ms_auiBoolRegister[0] = *reinterpret_cast<const uint8_t*>(p);
            return ms_auiBoolRegister;
        }
        if (TypeOf(entry) == 3) {
            ints[0] = p[0]; ints[1] = p[0]; ints[2] = p[0]; ints[3] = p[0];
            return ints;
        }
        reg[0] = p[0]; reg[1] = p[0]; reg[2] = p[0]; reg[3] = p[0];
        return reg;
    case 2:
        if (TypeOf(entry) == 3) {
            ints[0] = p[0]; ints[2] = p[0]; ints[1] = p[1]; ints[3] = p[1];
            return ints;
        }
        reg[0] = p[0]; reg[1] = p[1]; reg[2] = p[0]; reg[3] = p[1];
        return reg;
    case 3:
        if (TypeOf(entry) == 3) {
            ints[0] = p[0]; ints[1] = p[1]; ints[2] = p[2]; ints[3] = 1;
            return ints;
        }
        reg[0] = p[0]; reg[1] = p[1]; reg[2] = p[2]; reg[3] = 0x3F800000;
        return reg;
    case 4:
        if (TypeOf(entry) == 3) {
            ints[0] = p[0]; ints[1] = p[1]; ints[2] = p[2]; ints[3] = p[3];
            return ints;
        }
        reg[0] = p[0]; reg[1] = p[1]; reg[2] = p[2]; reg[3] = p[3];
        return reg;
    case 8:
        for (int i = 0; i < 8; i++) {
            m[i] = p[i];
        }
        m[8] = 0; m[9] = 0; m[10] = 0; m[11] = 0;
        m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 0;
        return m;
    case 9:
        m[0] = p[0]; m[1] = p[1]; m[2] = p[2]; m[3] = 0;
        m[4] = p[3]; m[5] = p[4]; m[6] = p[5]; m[7] = 0;
        m[8] = p[6]; m[9] = p[7]; m[10] = p[8];
        m[11] = 0;
        m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 0;
        return m;
    case 0xC:
        for (int i = 0; i < 12; i++) {
            m[i] = p[i];
        }
        m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 0;
        return m;
    case 0x10:
        for (int i = 0; i < 16; i++) {
            m[i] = p[i];
        }
        return m;
    default:
        return nullptr;
    }
}

// 0x88D6D0 (thiscall, slot 17): NiD3DShaderConstantMap::SetConstantConstant (arrays padded and sent
// as they are; else the data as registers)
uint32_t __fastcall ConstantMap_SetConstantConstant(uint8_t* self, re::Edx, void* program, uint8_t* entry, uint32_t) {
    if (TypeOf(entry) == 0xE) {
        const void* padded = ConstantMap_PadRegisters(self, entry, At<const void*>(entry, 0x2C));
        return ConstantMap_SetEntryData(nullptr, program, entry, padded);
    }
    const void* data = ConstantMap_GetConstantData(nullptr, entry);
    if (data == nullptr) {
        return 1;
    }
    return Send(program, entry, data);
}

// 0x88D740 (thiscall, slot 19): NiD3DShaderConstantMap::SetGlobalConstant (by the attribute type: a
// bool, an integer or float broadcast, a point2 twice, a point3 with w 1, four floats, a matrix3
// padded, a matrix4 as it is, an array padded; 0 for the others)
uint32_t __fastcall ConstantMap_SetGlobalConstant(uint8_t* self, re::Edx, void* program, uint8_t* entry, uint32_t) {
    const uint32_t* p = At<const uint32_t*>(entry, 0x2C);
    float* reg = reinterpret_cast<float*>(ms_auiRegisters);
    const float* f = At<const float*>(entry, 0x2C);
    switch (TypeOf(entry)) {
    case 1:
        ms_auiBoolRegister[0] = *reinterpret_cast<const uint8_t*>(p);
        return Send(program, entry, ms_auiBoolRegister);
    case 3: {
        uint32_t v = p[0];
        ms_auiIntRegisters[0] = v; ms_auiIntRegisters[1] = v; ms_auiIntRegisters[2] = v; ms_auiIntRegisters[3] = v;
        return Send(program, entry, ms_auiIntRegisters);
    }
    case 4:
        reg[0] = f[0]; reg[1] = reg[0]; reg[2] = reg[0]; reg[3] = reg[0];
        return Send(program, entry, reg);
    case 5:
        reg[0] = f[0]; reg[1] = f[1]; reg[2] = reg[0]; reg[3] = reg[1];
        return Send(program, entry, reg);
    case 6:
        reg[0] = f[0]; reg[1] = f[1]; reg[2] = f[2]; reg[3] = 1.0f;
        return Send(program, entry, reg);
    case 7:
    case 10:
        reg[0] = f[0]; reg[1] = f[1]; reg[2] = f[2]; reg[3] = f[3];
        return Send(program, entry, reg);
    case 8: {
        float* m = reinterpret_cast<float*>(ms_auiMatrixRegisters);
        m[0] = f[0]; m[1] = f[1]; m[2] = f[2]; m[3] = 0.0f;
        m[4] = f[3]; m[5] = f[4]; m[6] = f[5]; m[7] = 0.0f;
        m[8] = f[6]; m[9] = f[7]; m[10] = f[8]; m[11] = 0.0f;
        return Send(program, entry, m);
    }
    case 9:
        return Send(program, entry, p);
    case 0xE: {
        const void* padded = ConstantMap_PadRegisters(self, entry, p);
        return ConstantMap_SetEntryData(nullptr, program, entry, padded);
    }
    default:
        return 0;
    }
}

#define C39Q_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39Q_REVERSED("NiD3DShaderConstantMap::GetConstantData", 0x0088C200, ConstantMap_GetConstantDataImpl);
C39Q_REVERSED("NiD3DShaderConstantMap::SetConstantConstant", 0x0088D6D0, ConstantMap_SetConstantConstant);
C39Q_REVERSED("NiD3DShaderConstantMap::SetGlobalConstant", 0x0088D740, ConstantMap_SetGlobalConstant);

} // namespace ni::c39
