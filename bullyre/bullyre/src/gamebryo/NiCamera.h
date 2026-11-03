#pragma once

// NiCamera (vtable 0x951DF0, 0x13C bytes): an NiAVObject looking down its world rotation's first
// column (its direction; the second is up, the third right), with the world-to-camera matrix,
// the view frustum and the viewport. From the constructor at 0x763C20.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"

namespace ni {

struct NiFrustum {
    float left;      // +0x0
    float right;     // +0x4
    float top;       // +0x8
    float bottom;    // +0xC
    float nearDist;  // +0x10 (near, far: windows.h macros)
    float farDist;   // +0x14
    uint8_t ortho;   // +0x18
    uint8_t pad19[3];
};
static_assert(sizeof(NiFrustum) == 0x1C);

struct NiRectFloat {
    float left, right, top, bottom;
};
static_assert(sizeof(NiRectFloat) == 0x10);

struct NiCamera : NiAVObject {
    float worldToCam[4][4];      // +0xC4
    NiFrustum frustum;           // +0x104
    float minNearPlaneDist;      // +0x120 (0.1)
    float maxFarNearRatio;       // +0x124 (10000)
    NiRectFloat port;            // +0x128 (0, 1, 1, 0)
    float lodAdjust;             // +0x138 (1)
};
static_assert(offsetof(NiCamera, worldToCam) == 0xC4);
static_assert(offsetof(NiCamera, frustum) == 0x104);
static_assert(offsetof(NiCamera, minNearPlaneDist) == 0x120);
static_assert(offsetof(NiCamera, port) == 0x128);
static_assert(offsetof(NiCamera, lodAdjust) == 0x138);
static_assert(sizeof(NiCamera) == 0x13C);

} // namespace ni
