// The shader constant map extra data pieces near NiD3DShader: NiSCMExtraData::AddEntry, the five
// texture coordinate generation callbacks (camera-space position / reflection / normal, the
// texture stage's matrix from a rotation and the renderer's inverse view) kept at 0x20E6E40, and
// their static set-up / shutdown (the "__NDL_SCMData" name at 0x20E6E54).
//
// The callbacks take (rotation (3x3 floats), translation, the texture stage, a save flag, the
// renderer): they set the stage's TEXCOORDINDEX and TEXTURETRANSFORMFLAGS (through its group,
// +0xC), mark it as using a transform (+0x5A), and fill its D3DMATRIX (+0x18): the rotation's
// rows against the inverse view's (+0x810), float maths in the game's order (the main thread's
// 24-bit precision).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c39 {
namespace {

using ni::sys::At;

GAME_VAR(uint32_t, ms_uiEntryShift, 0x009A1914);           // (16)
GAME_VAR(uint32_t, ms_uiTexCoordFlags, 0x020E6D0C);
GAME_VAR(NiFixedString, ms_kSCMDataName, 0x020E6E54);      // "__NDL_SCMData"
GAME_ARRAY(uint32_t, ms_apfnTexCoordCallbacks, 0x020E6E40);   // five
GAME_FN(void, __thiscall, StageGroup_SetStageState, 0x0089F9E0, void* group, uint32_t state, uint32_t value,
        int save);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);

// the stage's matrix from the rotation rows against the inverse view (the game's groupings)
inline void FillRotation(const float* m, uint8_t* stage, const uint8_t* r) {
    auto V = [r](int offset) { return At<float>(r, offset); };
    float* out = &At<float>(stage, 0);
    out[0x18 / 4] = ((m[1] * V(0x814) + V(0x810) * m[0]) + V(0x818) * m[2]);
    out[0x28 / 4] = ((m[0] * V(0x820) + m[1] * V(0x824)) + m[2] * V(0x828));
    out[0x38 / 4] = ((m[0] * V(0x830) + m[1] * V(0x834)) + m[2] * V(0x838));
    out[0x1C / 4] = ((m[4] * V(0x814) + V(0x810) * m[3]) + V(0x818) * m[5]);
    out[0x2C / 4] = ((m[4] * V(0x824) + m[3] * V(0x820)) + V(0x828) * m[5]);
    out[0x3C / 4] = ((m[4] * V(0x834) + m[3] * V(0x830)) + V(0x838) * m[5]);
    out[0x20 / 4] = ((V(0x810) * m[6] + m[7] * V(0x814)) + m[8] * V(0x818));
    out[0x30 / 4] = ((V(0x824) * m[7] + m[6] * V(0x820)) + m[8] * V(0x828));
    out[0x40 / 4] = ((V(0x834) * m[7] + m[6] * V(0x830)) + m[8] * V(0x838));
}

inline void ClearColumn(uint8_t* stage) {
    At<float>(stage, 0x54) = 0.0f;
    At<float>(stage, 0x44) = 0.0f;
    At<float>(stage, 0x34) = 0.0f;
    At<float>(stage, 0x24) = 0.0f;
}

} // namespace

// 0x888790 (thiscall): NiSCMExtraData::AddEntry (a vertex (0) or pixel (1) entry: the map index,
// the pass (0xFF for global maps) shifted by 0x9A1914, the extra data)
void __fastcall SCMExtraData_AddEntry(uint8_t* self, re::Edx, uint32_t index, uint32_t pass, uint32_t pixel,
                                      void* extraData, int global) {
    uint32_t key = (uint8_t(global) != 0 ? 0xFFu : pass) << (ms_uiEntryShift & 0x1F);
    uint8_t* entries = At<uint8_t*>(self, 0x24 + int(pixel) * 4);
    At<uint32_t>(entries, At<int32_t>(self, 0x18 + int(pixel) * 4) * 8) = key | index;
    At<void*>(At<uint8_t*>(self, 0x24 + int(pixel) * 4), At<int32_t>(self, 0x18 + int(pixel) * 4) * 8 + 4) =
        extraData;
    At<int32_t>(self, 0x18 + int(pixel) * 4)++;
}

// 0x888870: camera-space position, two coordinates (translated: the rotation rows against the
// inverse view's translation, then the translation)
void __cdecl TexCoord_CameraSpacePosition(const float* m, const float* t, uint8_t* stage, int save,
                                          const uint8_t* r) {
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0xB, 0x20000, save);
    At<uint8_t>(stage, 0x5A) = 1;
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0x18, 2, save);
    FillRotation(m, stage, r);
    auto V = [r](int offset) { return At<float>(r, offset); };
    At<float>(stage, 0x48) = (((m[0] * V(0x840) + m[1] * V(0x844)) + m[2] * V(0x848)) + t[0]);
    At<float>(stage, 0x4C) = (((m[4] * V(0x844) + V(0x840) * m[3]) + m[5] * V(0x848)) + t[1]);
    At<float>(stage, 0x50) = (((V(0x840) * m[6] + m[7] * V(0x844)) + m[8] * V(0x848)) + t[2]);
    ClearColumn(stage);
}

// 0x888A60: camera-space position with the renderer's texture coordinate flags (their own
// groupings)
void __cdecl TexCoord_CameraSpacePositionFlags(const float* m, const float* t, uint8_t* stage, int save,
                                               const uint8_t* r) {
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0xB, 0x20000, save);
    uint32_t flags = ms_uiTexCoordFlags;
    At<uint8_t>(stage, 0x5A) = (flags != 0 && flags != 0x100) ? 1 : 0;
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0x18, flags, save);
    auto V = [r](int offset) { return At<float>(r, offset); };
    float* out = &At<float>(stage, 0);
    out[0x18 / 4] = ((V(0x810) * m[0] + V(0x814) * m[1]) + V(0x818) * m[2]);
    out[0x28 / 4] = ((m[1] * V(0x824) + m[0] * V(0x820)) + m[2] * V(0x828));
    out[0x38 / 4] = ((m[1] * V(0x834) + m[0] * V(0x830)) + m[2] * V(0x838));
    out[0x1C / 4] = ((V(0x810) * m[3] + V(0x814) * m[4]) + m[5] * V(0x818));
    out[0x2C / 4] = ((m[4] * V(0x824) + m[3] * V(0x820)) + m[5] * V(0x828));
    out[0x3C / 4] = ((m[4] * V(0x834) + m[3] * V(0x830)) + m[5] * V(0x838));
    out[0x20 / 4] = ((V(0x810) * m[6] + V(0x814) * m[7]) + m[8] * V(0x818));
    out[0x30 / 4] = ((m[6] * V(0x820) + V(0x824) * m[7]) + m[8] * V(0x828));
    out[0x40 / 4] = ((m[6] * V(0x830) + V(0x834) * m[7]) + m[8] * V(0x838));
    out[0x48 / 4] = (((m[0] * V(0x840) + V(0x844) * m[1]) + V(0x848) * m[2]) + t[0]);
    out[0x4C / 4] = (((m[3] * V(0x840) + V(0x844) * m[4]) + m[5] * V(0x848)) + t[1]);
    out[0x50 / 4] = (((V(0x844) * m[7] + m[6] * V(0x840)) + V(0x848) * m[8]) + t[2]);
    ClearColumn(stage);
}

// 0x888C60, 0x888DF0, 0x888F80: reflection vector (two / three coordinates) and camera-space
// normal (three): the rotation part only; the fourth row from the rotation's first (as the game
// has it)
void __cdecl TexCoord_ReflectionVector2(const float* m, const float*, uint8_t* stage, int save, const uint8_t* r) {
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0xB, 0x30000, save);
    At<uint8_t>(stage, 0x5A) = 1;
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0x18, 2, save);
    FillRotation(m, stage, r);
    At<float>(stage, 0x48) = m[0];
    At<float>(stage, 0x4C) = m[1];
    At<float>(stage, 0x50) = m[2];
    ClearColumn(stage);
}
void __cdecl TexCoord_ReflectionVector3(const float* m, const float*, uint8_t* stage, int save, const uint8_t* r) {
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0xB, 0x30000, save);
    At<uint8_t>(stage, 0x5A) = 1;
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0x18, 3, save);
    FillRotation(m, stage, r);
    At<float>(stage, 0x48) = m[0];
    At<float>(stage, 0x4C) = m[1];
    At<float>(stage, 0x50) = m[2];
    ClearColumn(stage);
}
void __cdecl TexCoord_CameraSpaceNormal(const float* m, const float*, uint8_t* stage, int save, const uint8_t* r) {
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0xB, 0x10000, save);
    At<uint8_t>(stage, 0x5A) = 1;
    StageGroup_SetStageState(At<void*>(stage, 0xC), 0x18, 3, save);
    FillRotation(m, stage, r);
    At<float>(stage, 0x48) = m[0];
    At<float>(stage, 0x4C) = m[1];
    At<float>(stage, 0x50) = m[2];
    ClearColumn(stage);
}

// 0x889110: the SCM data name and the callbacks set up
void __cdecl SCMExtraData_InitStatics() {
    FixedString_AssignText(&ms_kSCMDataName, "__NDL_SCMData");
    ms_apfnTexCoordCallbacks[0] = 0x00888870;
    ms_apfnTexCoordCallbacks[1] = 0x00888A60;
    ms_apfnTexCoordCallbacks[2] = 0x00888C60;
    ms_apfnTexCoordCallbacks[3] = 0x00888DF0;
    ms_apfnTexCoordCallbacks[4] = 0x00888F80;
}

// 0x889160: the name let go
void __cdecl SCMExtraData_ShutdownStatics() {
    FixedString_AssignText(&ms_kSCMDataName, nullptr);
}

#define C39L_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39L_REVERSED("NiSCMExtraData::AddEntry", 0x00888790, SCMExtraData_AddEntry);
C39L_REVERSED("NiD3DTextureStage::CameraSpacePositionCallback", 0x00888870, TexCoord_CameraSpacePosition);
C39L_REVERSED("NiD3DTextureStage::CameraSpacePositionFlagsCallback", 0x00888A60, TexCoord_CameraSpacePositionFlags);
C39L_REVERSED("NiD3DTextureStage::ReflectionVector2Callback", 0x00888C60, TexCoord_ReflectionVector2);
C39L_REVERSED("NiD3DTextureStage::ReflectionVector3Callback", 0x00888DF0, TexCoord_ReflectionVector3);
C39L_REVERSED("NiD3DTextureStage::CameraSpaceNormalCallback", 0x00888F80, TexCoord_CameraSpaceNormal);
C39L_REVERSED("NiSCMExtraData::InitStatics", 0x00889110, SCMExtraData_InitStatics);
C39L_REVERSED("NiSCMExtraData::ShutdownStatics", 0x00889160, SCMExtraData_ShutdownStatics);

} // namespace ni::c39
