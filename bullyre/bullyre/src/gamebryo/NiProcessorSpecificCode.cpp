// NiProcessorSpecificCode: the processor's kind (from cpuid, under an exception handler in case
// the instruction faults) and the vertex transforms picked for it, called through two pointers
// (0xBC8B4C TransformVectors: the rotation only; 0xBC8B50 TransformPoints: rotation, scale,
// translation). Both take (count, in, out, NiTransform or NiMatrix3), out may be in. The plain
// versions (compiled code) and the hand-written x87 ones (for every processor kind 1..6: in
// practice always, cpuid's extended leaf having its top bit set on any processor since).
//
// Main-thread maths: float, in the game's order of operations and of reads and writes (an
// in-place transform sees what it already wrote where the game's code does).

#include <cstddef>
#include <cstdint>
#include <intrin.h>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {
namespace {

using TransformFn = void(__cdecl*)(uint32_t count, const float* in, float* out, const float* transform);

GAME_VAR(TransformFn, ms_pfnTransformVectors, 0x00BC8B4C);
GAME_VAR(TransformFn, ms_pfnTransformPoints, 0x00BC8B50);
GAME_VAR(int32_t, ms_eProcessor, 0x020E39E0);     // 0 unknown, 1..6 (6: has extended cpuid leaves)
GAME_VAR(uint8_t, ms_bDetected, 0x020E39E4);
GAME_VAR(uint32_t, ms_uiFeatures, 0x020E39E8);    // the last cpuid result kept

constexpr uintptr_t kTransformVectorsX87 = 0x007D59E0;
constexpr uintptr_t kTransformPointsX87 = 0x007D5A60;
constexpr uintptr_t kTransformPoints = 0x007D5D20;
constexpr uintptr_t kTransformVectors = 0x007D5E10;

// (this file's, called where modes apply)
GAME_FN(void, __cdecl, DetectProcessor_, 0x007D5B70);
GAME_FN(void, __cdecl, SetTransformVectors_, 0x007D5EE0, uintptr_t fn);
GAME_FN(void, __cdecl, SetTransformPoints_, 0x007D5EF0, uintptr_t fn);

// the x87 TransformVectors: each row's products summed left to right; the third row reads the
// input's x after the output's x is written
void __cdecl TransformVectorsX87(uint32_t count, const float* in, float* out, const float* m) {
    for (uint32_t n = uint16_t(count); n != 0; n--) {
        float a = in[0] * m[0];
        float b = in[1] * m[1];
        float c = in[2] * m[2];
        float ab = a + b;
        float d = in[0] * m[3];
        float x = ab + c;
        float e = in[1] * m[4];
        float f = in[2] * m[5];
        out[0] = x;
        float de = d + e;
        float g = in[0] * m[6];
        float y = de + f;
        float h = in[1] * m[7];
        float i = in[2] * m[8];
        float gh = g + h;
        out[1] = y;
        out[2] = gh + i;
        in += 3;
        out += 3;
    }
}

// the x87 TransformPoints: the rotation scaled once (each entry rounded to a float), then per
// point ((x*s0 + y*s1) + z*s2) + t, all the input read before the output is written
void __cdecl TransformPointsX87(uint32_t count, const float* in, float* out, const float* transform) {
    float tx = transform[9];
    float ty = transform[10];
    float tz = transform[11];
    float scale = transform[12];
    float s[9];
    for (int i = 0; i < 9; i++) {
        s[i] = transform[i] * scale;
    }
    for (uint32_t n = uint16_t(count); n != 0; n--) {
        float a = in[0] * s[0];
        float b = in[1] * s[1];
        float c = in[2] * s[2];
        float ab = a + b;
        float d = in[0] * s[3];
        float x = ab + c;
        float e = in[1] * s[4];
        float f = in[2] * s[5];
        float de = d + e;
        x = x + tx;
        float g = s[6] * in[0];
        float y = f + de;
        float h = s[7] * in[1];
        y = y + ty;
        float i = s[8] * in[2];
        float gh = g + h;
        out[0] = x;
        float z = i + gh;
        out[1] = y;
        out[2] = z + tz;
        in += 3;
        out += 3;
    }
}

// the processor's kind: 6 when cpuid has extended leaves (the second query meant for leaf
// 0x80000001 puts it in edx, so it asks 0x80000000 again: the game's bug, harmless), else from
// leaf 1's features: MMX (bit 23) and CMOV (bit 15) give 1..4, SSE (bit 25) with both 5. A fault
// in the first or third query leaves it unknown; in the second, the features cleared
void __cdecl DetectProcessor() {
    int regs[4];
    ms_bDetected = 1;
    __try {
        __cpuid(regs, int(0x80000000));
        ms_uiFeatures = uint32_t(regs[0]);
    } __except (1) {
        ms_eProcessor = 0;
        return;
    }
    if ((ms_uiFeatures & 0x80000000) != 0) {
        __try {
            __cpuid(regs, int(0x80000000));
            ms_uiFeatures = uint32_t(regs[0]);
        } __except (1) {
            ms_uiFeatures = 0;
        }
        if ((ms_uiFeatures & 0x80000000) != 0) {
            ms_eProcessor = 6;
            return;
        }
    }
    __try {
        __cpuid(regs, 1);
        ms_uiFeatures = uint32_t(regs[3]);
    } __except (1) {
        ms_eProcessor = 0;
        return;
    }
    if ((ms_uiFeatures & 0x800000) == 0) {
        ms_eProcessor = (ms_uiFeatures & 0x8000) != 0 ? 3 : 1;
        return;
    }
    if ((ms_uiFeatures & 0x8000) == 0) {
        ms_eProcessor = 2;
        return;
    }
    ms_eProcessor = 4;
    if ((ms_uiFeatures & 0x2000000) != 0) {
        ms_eProcessor = 5;
    }
}

// the plain TransformPoints: (((m0*x + m1*y) + m2*z) * scale) + t per row, each row reading the
// input after the rows before it are written
void __cdecl TransformPoints(uint32_t count, const float* in, float* out, const float* transform) {
    for (uint32_t i = 0; i < uint16_t(count); i++) {
        out[0] = (transform[0] * in[0] + in[1] * transform[1] + in[2] * transform[2]) * transform[12] + transform[9];
        out[1] = (transform[3] * in[0] + in[1] * transform[4] + in[2] * transform[5]) * transform[12] + transform[10];
        out[2] = (transform[6] * in[0] + in[1] * transform[7] + in[2] * transform[8]) * transform[12] + transform[11];
        in += 3;
        out += 3;
    }
}

// the plain TransformVectors: (m0*x + m1*y) + m2*z per row, as TransformPoints
void __cdecl TransformVectors(uint32_t count, const float* in, float* out, const float* m) {
    for (uint32_t i = 0; i < uint16_t(count); i++) {
        out[0] = m[0] * in[0] + in[1] * m[1] + in[2] * m[2];
        out[1] = m[3] * in[0] + in[1] * m[4] + in[2] * m[5];
        out[2] = m[6] * in[0] + in[1] * m[7] + in[2] * m[8];
        in += 3;
        out += 3;
    }
}

void __cdecl SetTransformVectors(uintptr_t fn) {
    ms_pfnTransformVectors = reinterpret_cast<TransformFn>(fn);
}

void __cdecl SetTransformPoints(uintptr_t fn) {
    ms_pfnTransformPoints = reinterpret_cast<TransformFn>(fn);
}

// the end of NiMain's _SDMInit: the processor detected once, the plain transforms, the x87 ones
// for a known kind
void __cdecl Initialize() {
    if (ms_bDetected == 0) {
        DetectProcessor_();
    }
    SetTransformPoints_(kTransformPoints);
    SetTransformVectors_(kTransformVectors);
    if (ms_eProcessor > 0 && ms_eProcessor <= 6) {
        SetTransformPoints_(kTransformPointsX87);
        SetTransformVectors_(kTransformVectorsX87);
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPROCSPEC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIPROCSPEC_REVERSED("NiProcessorSpecificCode::TransformVectorsX87", 0x007D59E0, ni::TransformVectorsX87);
NIPROCSPEC_REVERSED("NiProcessorSpecificCode::TransformPointsX87", 0x007D5A60, ni::TransformPointsX87);
NIPROCSPEC_REVERSED("NiProcessorSpecificCode::DetectProcessor", 0x007D5B70, ni::DetectProcessor);
NIPROCSPEC_REVERSED("NiProcessorSpecificCode::TransformPoints", 0x007D5D20, ni::TransformPoints);
NIPROCSPEC_REVERSED("NiProcessorSpecificCode::TransformVectors", 0x007D5E10, ni::TransformVectors);
NIPROCSPEC_REVERSED("NiProcessorSpecificCode::SetTransformVectors", 0x007D5EE0, ni::SetTransformVectors);
NIPROCSPEC_REVERSED("NiProcessorSpecificCode::SetTransformPoints", 0x007D5EF0, ni::SetTransformPoints);
NIPROCSPEC_REVERSED("NiProcessorSpecificCode::Initialize", 0x007D5F00, ni::Initialize);
