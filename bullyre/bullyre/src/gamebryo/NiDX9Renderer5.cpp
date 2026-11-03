// NiDX9Renderer's camera (NiDX9Renderer.h): Do_SetCameraData (the view matrix from the camera's
// frame, its inverse, the projection from the frustum (perspective or orthographic; the render
// state's +0xF4 flips x), the viewport from the port, the render state's near / far) and
// Do_SetScreenSpaceCameraData (the [0, 1] screen space camera: half a pixel off, depth range
// 1..10000, the frustum and port remembered).
//
// The float maths in the game's order, in float (the main thread's x87 runs at 24 bits); the
// viewport's values truncated through a 64-bit integer (fistp with chop).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::dx9 {
namespace {

inline float& F(uint8_t* self, int offset) {
    return At<float>(self, offset);
}

// the viewport from a port (fractions of the current group's width / height; y flipped)
inline void PortToViewport(uint32_t* viewport, const float* port, float width, float height) {
    viewport[0] = uint32_t(crt::FloatToInt(port[0] * width));
    viewport[1] = uint32_t(crt::FloatToInt((1.0f - port[2]) * height));
    viewport[2] = uint32_t(crt::FloatToInt(width * (port[1] - port[0])));
    viewport[3] = uint32_t(crt::FloatToInt((port[2] - port[3]) * height));
}

} // namespace

// 0x879730 (thiscall): NiDX9Renderer::Do_SetCameraData(location, direction, up, right, frustum,
// port)
void __fastcall Renderer_Do_SetCameraData(uint8_t* self, re::Edx, const float* location, const float* direction,
                                          const float* up, const float* right, const float* frustum,
                                          const float* port) {
    if (self[kDeviceLost] != 0) {
        return;
    }
    // the view matrix (the camera's axes as columns, the location moved to the origin)
    F(self, 0x790) = right[0];
    F(self, 0x794) = up[0];
    F(self, 0x798) = direction[0];
    F(self, 0x79C) = 0.0f;
    F(self, 0x7A0) = right[1];
    F(self, 0x7A4) = up[1];
    F(self, 0x7A8) = direction[1];
    F(self, 0x7AC) = 0.0f;
    F(self, 0x7B0) = right[2];
    F(self, 0x7B4) = up[2];
    F(self, 0x7B8) = direction[2];
    F(self, 0x7BC) = 0.0f;
    float dot = location[0] * right[0] + right[1] * location[1] + right[2] * location[2];
    F(self, 0x7C0) = -dot;
    dot = up[1] * location[1] + location[0] * up[0] + location[2] * up[2];
    F(self, 0x7C4) = -dot;
    dot = location[0] * direction[0] + location[1] * direction[1] + direction[2] * location[2];
    F(self, 0x7C8) = -dot;
    F(self, 0x7CC) = 1.0f;
    // its inverse
    F(self, 0x810) = right[0];
    F(self, 0x814) = right[1];
    F(self, 0x818) = right[2];
    F(self, 0x81C) = 0.0f;
    F(self, 0x820) = up[0];
    F(self, 0x824) = up[1];
    F(self, 0x828) = up[2];
    F(self, 0x82C) = 0.0f;
    F(self, 0x830) = direction[0];
    F(self, 0x834) = direction[1];
    F(self, 0x838) = direction[2];
    F(self, 0x83C) = 0.0f;
    F(self, 0x840) = location[0];
    F(self, 0x844) = location[1];
    F(self, 0x848) = location[2];
    F(self, 0x84C) = 1.0f;
    Com<long>(At<void*>(self, kDevice), kDevSetTransform, 2u, self + kView);   // D3DTS_VIEW
    for (int i = 0; i < 3; i++) {
        At<uint32_t>(self, 0x468 + i * 4) = reinterpret_cast<const uint32_t*>(right)[i];
        At<uint32_t>(self, 0x450 + i * 4) = reinterpret_cast<const uint32_t*>(right)[i];
    }
    for (int i = 0; i < 3; i++) {
        At<uint32_t>(self, 0x474 + i * 4) = reinterpret_cast<const uint32_t*>(up)[i];
        At<uint32_t>(self, 0x45C + i * 4) = reinterpret_cast<const uint32_t*>(up)[i];
    }
    // the projection (frustum: left, right, top, bottom, near, far, ortho at +0x18)
    float nearZ = frustum[4];
    F(self, 0x480) = nearZ;
    float depth = frustum[5] - frustum[4];
    F(self, 0x484) = depth;
    float width = frustum[1] - frustum[0];
    float sumX = frustum[1] + frustum[0];
    float height = frustum[2] - frustum[3];
    float sumY = frustum[3] + frustum[2];
    float invDepth = 1.0f / depth;
    bool flip = At<uint32_t>(At<void*>(self, kRenderState), 0xF4) != 0;
    if (reinterpret_cast<const uint8_t*>(frustum)[0x18] != 0) {   // orthographic
        F(self, 0x7D0) = flip ? -2.0f / width : 2.0f / width;
        F(self, 0x7E0) = 0.0f;
        F(self, 0x7F0) = 0.0f;
        F(self, 0x800) = (flip ? sumX : -sumX) / width;
        F(self, 0x7D4) = 0.0f;
        F(self, 0x7E4) = 2.0f / height;
        F(self, 0x7F4) = 0.0f;
        F(self, 0x804) = -sumY / height;
        F(self, 0x7D8) = 0.0f;
        F(self, 0x7E8) = 0.0f;
        F(self, 0x7F8) = invDepth;
        float offset = invDepth * frustum[4];
        F(self, 0x808) = -offset;
        F(self, 0x7DC) = 0.0f;
        F(self, 0x7EC) = 0.0f;
        F(self, 0x7FC) = 0.0f;
        F(self, 0x80C) = 1.0f;
    } else {
        F(self, 0x7D0) = flip ? -2.0f / width : 2.0f / width;
        F(self, 0x7E0) = 0.0f;
        F(self, 0x7F0) = (flip ? sumX : -sumX) / width;
        F(self, 0x800) = 0.0f;
        F(self, 0x7D4) = 0.0f;
        F(self, 0x7E4) = 2.0f / height;
        F(self, 0x7F4) = -sumY / height;
        F(self, 0x804) = 0.0f;
        F(self, 0x7D8) = 0.0f;
        F(self, 0x7E8) = 0.0f;
        F(self, 0x7F8) = frustum[5] * invDepth;
        float offset = frustum[5] * frustum[4] * invDepth;
        F(self, 0x808) = -offset;
        F(self, 0x7DC) = 0.0f;
        F(self, 0x7EC) = 0.0f;
        F(self, 0x7FC) = 1.0f;
        F(self, 0x80C) = 0.0f;
    }
    Com<long>(At<void*>(self, kDevice), kDevSetTransform, 3u, self + kProjection);   // D3DTS_PROJECTION
    // the viewport (D3DVIEWPORT9)
    float targetWidth = float(Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 18, 0u));
    float targetHeight = float(Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 19, 0u));
    uint32_t viewport[6];
    PortToViewport(viewport, port, targetWidth, targetHeight);
    reinterpret_cast<float*>(viewport)[4] = 0.0f;
    reinterpret_cast<float*>(viewport)[5] = 1.0f;
    Com<long>(At<void*>(self, kDevice), kDevSetViewport, viewport);
    uint8_t* state = At<uint8_t*>(self, kRenderState);
    float nearPlane = frustum[4];
    float farPlane = frustum[5];
    At<float>(state, 0x78) = nearPlane;
    At<float>(state, 0x7C) = farPlane;
    At<float>(state, 0x80) = farPlane - nearPlane;
    crt::Memcpy(self + kFrustum, frustum, 0x1C);
    crt::Memcpy(self + kPort, port, 0x10);
}

// 0x879CD0 (thiscall): NiDX9Renderer::Do_SetScreenSpaceCameraData(port) (no port: the whole
// target)
void __fastcall Renderer_Do_SetScreenSpaceCameraData(uint8_t* self, re::Edx, const float* port) {
    if (self[kDeviceLost] != 0) {
        return;
    }
    uint32_t targetWidth = Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 18, 0u);
    uint32_t targetHeight = Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 19, 0u);
    float width = float(targetWidth);
    float height = float(targetHeight);
    // the view: x kept, y flipped, (0.5, 0.5) to the origin
    static const float kScreenView[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f,  1.0f, 1.0f};
    static const float kScreenInvView[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f,  -1.0f, 1.0f};
    for (int i = 0; i < 16; i++) {
        F(self, kView + i * 4) = kScreenView[i];
        F(self, kInvView + i * 4) = kScreenInvView[i];
    }
    Com<long>(At<void*>(self, kDevice), kDevSetTransform, 2u, self + kView);
    F(self, 0x468) = 1.0f;
    F(self, 0x450) = 1.0f;
    F(self, 0x46C) = 0.0f;
    F(self, 0x454) = 0.0f;
    F(self, 0x474) = 0.0f;
    F(self, 0x45C) = 0.0f;
    F(self, 0x480) = 1.0f;
    F(self, 0x470) = 0.0f;
    F(self, 0x484) = 9999.0f;
    F(self, 0x458) = 0.0f;
    F(self, 0x478) = -1.0f;
    F(self, 0x460) = -1.0f;
    F(self, 0x47C) = 0.0f;
    F(self, 0x464) = 0.0f;
    // the projection (half a pixel's offset in the last row)
    F(self, 0x7D0) = 2.0f;
    F(self, 0x7E0) = 0.0f;
    F(self, 0x7F0) = 0.0f;
    F(self, 0x800) = -1.0f / width;
    F(self, 0x7D4) = 0.0f;
    F(self, 0x7F4) = 0.0f;
    F(self, 0x7E4) = 2.0f;
    F(self, 0x804) = 1.0f / height;
    F(self, 0x7D8) = 0.0f;
    F(self, 0x7E8) = 0.0f;
    At<uint32_t>(self, 0x7F8) = 0x38D1C3D5;   // 1.0001e-4 (1 / 9999)
    At<uint32_t>(self, 0x808) = 0xB8D1C3D5;
    F(self, 0x7DC) = 0.0f;
    F(self, 0x7EC) = 0.0f;
    F(self, 0x7FC) = 0.0f;
    F(self, 0x80C) = 1.0f;
    Com<long>(At<void*>(self, kDevice), kDevSetTransform, 3u, self + kProjection);
    uint32_t viewport[6];
    if (port != nullptr) {
        PortToViewport(viewport, port, width, height);
        crt::Memcpy(self + kPort, port, 0x10);
    } else {
        viewport[0] = 0;
        viewport[1] = 0;
        F(self, kPort) = 0.0f;
        viewport[2] = targetWidth;
        F(self, kPort + 4) = 1.0f;
        F(self, kPort + 8) = 1.0f;
        viewport[3] = targetHeight;
        F(self, kPort + 0xC) = 0.0f;
    }
    reinterpret_cast<float*>(viewport)[4] = 0.0f;
    reinterpret_cast<float*>(viewport)[5] = 1.0f;
    Com<long>(At<void*>(self, kDevice), kDevSetViewport, viewport);
    uint8_t* state = At<uint8_t*>(self, kRenderState);
    At<float>(state, 0x78) = 1.0f;
    At<float>(state, 0x7C) = 10000.0f;
    At<float>(state, 0x80) = 9999.0f;
    // the frustum (orthographic, half a pixel off)
    self[kFrustum + 0x18] = 1;
    float doubleWidth = width * 2.0f;
    float doubleHeight = 2.0f * height;
    float halfPixelX = 1.0f / doubleWidth;
    F(self, kFrustum) = halfPixelX - 0.5f;
    F(self, kFrustum + 4) = halfPixelX + 0.5f;
    float halfPixelY = 1.0f / doubleHeight;
    F(self, kFrustum + 8) = 0.5f - halfPixelY;
    F(self, kFrustum + 0xC) = -0.5f - halfPixelY;
    F(self, kFrustum + 0x10) = 1.0f;
    F(self, kFrustum + 0x14) = 10000.0f;
}

#define DX9REND38E_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

DX9REND38E_REVERSED("NiDX9Renderer::Do_SetCameraData", 0x00879730, Renderer_Do_SetCameraData);
DX9REND38E_REVERSED("NiDX9Renderer::Do_SetScreenSpaceCameraData", 0x00879CD0, Renderer_Do_SetScreenSpaceCameraData);

} // namespace ni::dx9
