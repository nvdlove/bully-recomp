// NiD3D10Renderer (src/gamebryo/NiD3D10Renderer.h): the render target groups' validation, the
// camera, the program caches, the frame (begin, display, end of a render target group), the batch
// and render slots, the geometry's precaching. The x87 maths are written in float in the game's
// order (the D3D10 renderer would run on the main thread, at 24-bit precision).

#include "gamebryo/NiD3D10Renderer.h"
#include "reversed.h"

namespace ni::c42 {
namespace {

using namespace ni::d3d10;

// ID3D10Device 30 RSSetViewports; ID3D10View 8 GetDesc
constexpr int kRSSetViewports = 30;
constexpr int kViewGetDesc = 8;

GAME_VAR(uint32_t, NiParticles_ClassBit, 0x020E3F8C);   // (the two geometry kinds PrecacheGeometry
GAME_VAR(uint32_t, NiLines_ClassBit, 0x020E3048);       //  packs its own way; names guesses)
GAME_VAR(void*, ms_spDefaultTexture, 0x020E28F0);

GAME_FN(const char*, __cdecl, Material_GetDefaultWorkingDirectory, 0x00762A00);
GAME_FN(uint32_t, __thiscall, Material_GetProgramVersion, 0x00786440, void* material, uint32_t type);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);
GAME_FN(void*, __thiscall, GPUProgramCache_Construct, 0x008B8D20, void* self, uint32_t version,
        const char* workingDir, uint32_t type, const re::Ptr32<char>* profile, const char* materialName, int autoWriteToDisk,
        int writeDebugFile, int locked, int load);
GAME_FN(void, __thiscall, PointerList_AddTail, 0x007843D0, void* list, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, Geometry_SetActiveMaterial, 0x00761F70, void* geometry, void* material, int data);
GAME_FN(void, __thiscall, NiPointer_AssignPointer, 0x00518100, re::Ptr32<void>* self, const re::Ptr32<void>* other);
GAME_FN(void*, __thiscall, MaterialInstance_GetData, 0x00856850, void* instance);
GAME_FN(void, __thiscall, TexturingProperty_SetBaseTexture, 0x0075DDF0, void* property, void* texture);
GAME_FN(void, __thiscall, ScreenTextureNode_Render, 0x007AB310, void* node, void* renderer);
GAME_FN(void, __cdecl, Geometry_UpdateSkinPartition, 0x0077D090, void* geometry);
GAME_FN(void*, __thiscall, BufferData_GetView, 0x008B6120, void* data);
GAME_FN(void*, __thiscall, GeometryData_Next, 0x00749E60, void* data);
GAME_FN(uint32_t, __thiscall, GeometryData_GetVertexCount, 0x008B7460, void* data);
GAME_FN(void, __thiscall, GeometryData_SetIndexBuffer, 0x008B7620, void* data, uint32_t buffer);
GAME_FN(uint32_t, __cdecl, GeometryData_CreateIndexBuffer, 0x008B7630, int flag, int a, int b, int c);
GAME_FN(uint8_t, __cdecl, GeometryData_Function8B7670, 0x008B7670, uint32_t value);
GAME_FN(void*, __thiscall, GeometryData_GetOwner, 0x008B7680, void* data);
// this chunk's
GAME_FN(uint8_t, __thiscall, Renderer_HandleDisplayFrameResult, 0x008AA680, void* self, long hr);
GAME_FN(void*, __thiscall, Renderer_CreateGeometryData, 0x008ABAD0, void* self, void* geometry);
GAME_FN(uint8_t, __thiscall, Renderer_PackGeometry, 0x008ABCB0, void* self, void* data, void* model, void* extra,
        void* skin, int force);
GAME_FN(uint8_t, __thiscall, Renderer_PackLines, 0x008A9DE0, void* self, void* data, void* model, void* skin,
        int force);
GAME_FN(uint8_t, __thiscall, Renderer_PackParticles, 0x008ADA50, void* self, void* data, void* model, void* skin,
        void* a, void* b, int force);
GAME_FN(void, __thiscall, Renderer_RenderGeometry, 0x008ACA80, void* self, void* geometry, uint32_t type);
GAME_FN(void, __thiscall, Renderer_PackScreenTexture, 0x008AE870, void* self, void* screenTexture);
GAME_FN(void, __thiscall, RenderStateManager_GetState, 0x008B04E0, void* self, void* state);
GAME_FN(void, __thiscall, RenderStateManager_SetState, 0x008B04C0, void* self, const void* state);

inline uint8_t* Device(uint8_t* self) {
    return At<uint8_t*>(self, kDevice);
}

// a D3D10 view's description: the format, the dimension, the union's three
struct ViewDesc {
    uint32_t format;
    uint32_t dimension;
    uint32_t u0, u1, u2;
};

// a buffer's size (its renderer data's buffer: +0x8 width, +0xC height)
inline uint32_t DataWidth(void* data) {
    return At<uint32_t>(At<void*>(data, 8), 8);
}
inline uint32_t DataHeight(void* data) {
    return At<uint32_t>(At<void*>(data, 8), 0xC);
}

// a group's buffer's renderer data and view
inline void* GroupData(void* group, uint32_t index) {
    return Virtual<void*>(group, 31, index);
}

// the viewport from the port and the target's size (truncated)
struct Viewport {
    int32_t x, y;
    uint32_t width, height;
    float minDepth, maxDepth;
};

// a program cache made for one kind and given to the material (the profile's name held while)
void SetProgramCache(uint8_t* material, uint32_t type, const char* profile, int autoWriteToDisk, int writeDebugFile,
                     int load, int locked, const char* workingDir) {
    void* memory = NiNew(0x58);
    void* cache = nullptr;
    if (memory != nullptr) {
        re::Ptr32<char> name = profile == nullptr ? static_cast<char*>(sys::kNullFixedString) : StringTable_AddString(profile);
        cache = GPUProgramCache_Construct(memory, Material_GetProgramVersion(material, type), workingDir, type, &name,
                                          At<const char*>(material, 8), autoWriteToDisk, writeDebugFile, locked, load);
        sys::FixedStringRelease(&name);
    }
    Virtual<void>(material, 10, cache, type);   // SetProgramCache
}

// the batch's render of one shape or strips: the material its first active one's (or the
// default), the shape's own put back after
void BatchRender(uint8_t* self, uint8_t* shape, int slot) {
    uint32_t active = At<uint32_t>(shape, 0xE0);
    void* material = nullptr;
    if (active < At<uint32_t>(shape, 0xDC)) {
        material = At<void*>(At<uint8_t*>(shape, 0xD4) + active * 16, 4);
        AddRef(material);
    }
    int data = -1;
    if (active < At<uint32_t>(shape, 0xDC)) {
        uint8_t* instance = At<uint8_t*>(shape, 0xD4) + active * 16;
        if (instance != nullptr) {
            data = int(reinterpret_cast<uintptr_t>(MaterialInstance_GetData(instance)));
        }
    }
    re::Ptr32<void>& batch = At<void*>(self, 0x470);
    if (batch == nullptr) {
        AssignRef(batch, material);
        if (batch == nullptr) {
            Geometry_SetActiveMaterial(shape, At<void*>(self, 0x18), -1);
            NiPointer_AssignPointer(&batch, &At<void*>(self, 0x18));
        }
    } else {
        Geometry_SetActiveMaterial(shape, batch, -1);
    }
    Virtual<void>(self, slot, shape);
    Geometry_SetActiveMaterial(shape, material, data);
    ReleaseRef(material);
}

} // namespace

// 0x8A97C0 (thiscall, slot 27): ValidateRenderTargetGroup (its render target views all of one
// kind and size, none twice; its depth/stencil buffer compatible with the first buffer)
uint8_t __fastcall NiD3D10Renderer_ValidateRenderTargetGroup(uint8_t* self, re::Edx, void* group) {
    if (group == nullptr) {
        return 0;
    }
    uint32_t dimension = 0;
    int32_t elementOffset = -1, elementWidth = -1;
    int32_t mipSlice = -1, arraySize = -1;
    int32_t width = -1, height = -1;
    void* firstBuffer = nullptr;
    for (uint32_t i = 0; i < 8; i++) {
        void* data = GroupData(group, i);
        if (data == nullptr) {
            continue;
        }
        if (firstBuffer == nullptr) {
            firstBuffer = Virtual<void*>(group, 27, i);
        }
        void* view = BufferData_GetView(data);
        if (view == nullptr) {
            continue;
        }
        ViewDesc desc;
        Com<long>(view, kViewGetDesc, &desc);
        if (dimension == 0) {
            dimension = desc.dimension;
        }
        uint32_t dim = desc.dimension;
        int32_t u0 = int32_t(desc.u0), u1 = int32_t(desc.u1), u2 = int32_t(desc.u2);
        bool checkWidth = false, checkSize = false;
        switch (dimension) {
        case 1:   // BUFFER
            if (elementOffset == -1) {
                elementOffset = u0;
                elementWidth = u1;
            } else if (dim != 1 || elementOffset != u0 || elementWidth != u1) {
                return 0;
            }
            break;
        case 2:   // TEXTURE1D
        case 4:   // TEXTURE2D
            if (dim == dimension) {
            } else if (dim == dimension + 1) {
                if (mipSlice != -1 && u2 != 1) {
                    return 0;
                }
            } else {
                return 0;
            }
            if (mipSlice == -1) {
                mipSlice = u0;
            } else if (mipSlice != u0) {
                return 0;
            }
            (dimension == 2 ? checkWidth : checkSize) = true;
            break;
        case 3:   // TEXTURE1DARRAY
        case 5:   // TEXTURE2DARRAY
            if (dim == dimension - 1) {
                if (arraySize != -1 && arraySize != 1) {
                    return 0;
                }
            } else if (dim == dimension) {
                if (arraySize != -1 && u2 != arraySize) {
                    return 0;
                }
            } else {
                return 0;
            }
            if (mipSlice != -1) {
                if (mipSlice != u0) {
                    return 0;
                }
            } else {
                mipSlice = u0;
                arraySize = u2;
            }
            (dimension == 3 ? checkWidth : checkSize) = true;
            break;
        case 6:   // TEXTURE2DMS
            if (dim == 6) {
            } else if (dim == 7) {
                if (u1 != 1) {
                    return 0;
                }
            } else {
                return 0;
            }
            checkSize = true;
            break;
        case 7:   // TEXTURE2DMSARRAY (quirk kept: the size kept is the union's third)
            if (dim == 6) {
                if (arraySize == -1) {
                    arraySize = u2;
                } else if (arraySize != 1) {
                    return 0;
                }
            } else if (dim == 7) {
                if (arraySize == -1) {
                    arraySize = u2;
                } else if (u1 != arraySize) {
                    return 0;
                }
            } else {
                return 0;
            }
            checkSize = true;
            break;
        case 8:   // TEXTURE3D
            if (mipSlice == -1) {
                mipSlice = u0;
                arraySize = u2;
            } else if (dim != 8 || mipSlice != u0 || arraySize != u2) {
                return 0;
            }
            checkSize = true;
            break;
        default:
            break;
        }
        if (checkWidth) {
            if (width == -1) {
                width = int32_t(DataWidth(data));
            } else if (int32_t(DataWidth(data)) != width) {
                return 0;
            }
        }
        if (checkSize) {
            if (width == -1) {
                width = int32_t(DataWidth(data));
                height = int32_t(DataHeight(data));
            } else if (int32_t(DataWidth(data)) != width || int32_t(DataHeight(data)) != height) {
                return 0;
            }
        }
        for (uint32_t j = 0; j < i; j++) {
            void* other = GroupData(group, j);
            if (other != nullptr && BufferData_GetView(other) == view) {
                return 0;
            }
        }
    }
    void* depthStencil = Virtual<void*>(group, 28);
    if (depthStencil != nullptr && Virtual<uint8_t>(self, 28, firstBuffer, depthStencil) == 0) {
        return 0;
    }
    return 1;
}

// 0x8AB770 (thiscall, slot 28; `this` unused): IsDepthBufferCompatible (the depth/stencil view's
// kind matching the target's, at least its size)
uint8_t __fastcall NiD3D10Renderer_IsDepthBufferCompatible(void*, re::Edx, uint8_t* buffer, uint8_t* depthStencil) {
    if (buffer == nullptr || depthStencil == nullptr) {
        return 1;
    }
    void* targetData = At<void*>(buffer, 0x10);
    if (targetData == nullptr) {
        return 0;
    }
    void* targetView = BufferData_GetView(targetData);
    if (targetView == nullptr) {
        return 0;
    }
    ViewDesc target;
    Com<long>(targetView, kViewGetDesc, &target);
    void* depthData = At<void*>(depthStencil, 0x10);
    if (depthData == nullptr) {
        return 0;
    }
    void* depthView = BufferData_GetView(depthData);
    if (depthView == nullptr) {
        return 0;
    }
    ViewDesc depth;
    Com<long>(depthView, kViewGetDesc, &depth);
    bool widthOnly = false;
    switch (target.dimension) {
    case 2:   // TEXTURE1D: DSV 1 TEXTURE1D, 2 TEXTURE1DARRAY of one
        if (depth.dimension == 1 || (depth.dimension == 2 && depth.u2 == 1)) {
            widthOnly = true;
            break;
        }
        return 0;
    case 3:   // TEXTURE1DARRAY
        if ((depth.dimension == 1 && target.u2 == 1) || (depth.dimension == 2 && depth.u2 == target.u2)) {
            widthOnly = true;
            break;
        }
        return 0;
    case 4:   // TEXTURE2D: DSV 3 TEXTURE2D, 4 TEXTURE2DARRAY
        if (depth.dimension == 3 || (depth.dimension == 4 && depth.u2 == 1)) {
            break;
        }
        return 0;
    case 5:
        if ((depth.dimension == 3 && target.u2 == 1) || (depth.dimension == 4 && depth.u2 == target.u2)) {
            break;
        }
        return 0;
    case 6:   // TEXTURE2DMS: DSV 5, 6
        if (depth.dimension == 5 || (depth.dimension == 6 && depth.u1 == 1)) {
            break;
        }
        return 0;
    case 7:
        if ((depth.dimension == 5 && target.u1 == 1) || (depth.dimension == 6 && depth.u1 == target.u1)) {
            break;
        }
        return 0;
    case 1:   // BUFFER, TEXTURE3D
    case 8:
        return 0;
    default:
        return 1;
    }
    if (widthOnly) {
        return DataWidth(targetData) <= DataWidth(depthData) ? 1 : 0;
    }
    if (DataWidth(targetData) > DataWidth(depthData)) {
        return 0;
    }
    return DataHeight(targetData) <= DataHeight(depthData) ? 1 : 0;
}

// 0x8AA8D0 (thiscall, slot 86): Do_SetCameraData(location, direction, up, right, frustum, port)
// (the view, its inverse, the projection (left-right swapped as asked), the viewport; the
// frustum kept)
void __fastcall NiD3D10Renderer_SetCameraData(uint8_t* self, re::Edx, const float* location, const float* direction,
                                              const float* up, const float* right, const uint32_t* frustum,
                                              const float* port) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099F5F8));
        return;
    }
    float* view = &At<float>(self, kView);
    view[0] = right[0];
    view[1] = up[0];
    view[2] = direction[0];
    view[3] = 0.0f;
    view[4] = right[1];
    view[5] = up[1];
    view[6] = direction[1];
    view[7] = 0.0f;
    view[8] = right[2];
    view[9] = up[2];
    view[10] = direction[2];
    view[11] = 0.0f;
    float d = location[0] * right[0] + right[1] * location[1] + right[2] * location[2];
    view[12] = -d;
    d = up[1] * location[1] + location[0] * up[0] + location[2] * up[2];
    view[13] = -d;
    d = location[0] * direction[0] + location[1] * direction[1] + direction[2] * location[2];
    view[14] = -d;
    view[15] = 1.0f;
    float* inverse = &At<float>(self, kInvView);
    for (int i = 0; i < 3; i++) {
        inverse[i] = right[i];
        inverse[4 + i] = up[i];
        inverse[8 + i] = direction[i];
        inverse[12 + i] = location[i];
    }
    inverse[3] = 0.0f;
    inverse[7] = 0.0f;
    inverse[11] = 0.0f;
    inverse[15] = 1.0f;
    const auto* f = reinterpret_cast<const float*>(frustum);
    float width = f[1] - f[0];
    float sumX = f[0] + f[1];
    float heightSpan = f[2] - f[3];
    float sumY = f[3] + f[2];
    float depth = f[5] - f[4];
    float invDepth = 1.0f / depth;
    float* proj = &At<float>(self, kProjection);
    bool swap = At<uint8_t>(self, kLeftRightSwap) != 0;
    float scaleX = (swap ? -2.0f : 2.0f) / width;
    float offsetX = (swap ? sumX : -sumX) / width;
    if (reinterpret_cast<const uint8_t*>(frustum)[0x18] != 0) {   // orthographic
        proj[0] = scaleX;
        proj[4] = 0.0f;
        proj[8] = 0.0f;
        proj[12] = offsetX;
        proj[1] = 0.0f;
        proj[5] = 2.0f / heightSpan;
        proj[9] = 0.0f;
        proj[13] = -sumY / heightSpan;
        proj[2] = 0.0f;
        proj[6] = 0.0f;
        proj[10] = invDepth;
        proj[14] = -(invDepth * f[4]);
        proj[3] = 0.0f;
        proj[7] = 0.0f;
        proj[11] = 0.0f;
        proj[15] = 1.0f;
    } else {
        proj[0] = scaleX;
        proj[4] = 0.0f;
        proj[8] = offsetX;
        proj[12] = 0.0f;
        proj[1] = 0.0f;
        proj[5] = 2.0f / heightSpan;
        proj[9] = -sumY / heightSpan;
        proj[13] = 0.0f;
        proj[2] = 0.0f;
        proj[6] = 0.0f;
        proj[10] = f[5] * invDepth;
        proj[14] = -(invDepth * (f[5] * f[4]));
        proj[3] = 0.0f;
        proj[7] = 0.0f;
        proj[15] = 0.0f;
        proj[11] = 1.0f;
    }
    void* group = At<void*>(self, kCurrentRenderTargetGroup);
    float targetWidth = float(Virtual<uint32_t>(group, 18, 0u));
    float targetHeight = float(Virtual<uint32_t>(group, 19, 0u));
    Viewport viewport;
    viewport.x = crt::FloatToInt(port[0] * targetWidth);
    viewport.y = crt::FloatToInt((1.0f - port[2]) * targetHeight);
    viewport.width = uint32_t(crt::FloatToInt((port[1] - port[0]) * targetWidth));
    viewport.height = uint32_t(crt::FloatToInt((port[2] - port[3]) * targetHeight));
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    Com<long>(Device(self), kRSSetViewports, 1u, &viewport);
    for (int i = 0; i < 7; i++) {
        At<uint32_t>(self, kFrustum + i * 4) = frustum[i];
    }
}

// 0x8AAD80 (thiscall, slot 87): Do_SetScreenSpaceCameraData(port) (the fixed screen space view
// and projection, the viewport from the port or the whole target, an orthographic frustum half a
// pixel off)
void __fastcall NiD3D10Renderer_SetScreenSpaceCameraData(uint8_t* self, re::Edx, const float* port) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099F62C));
        return;
    }
    void* group = At<void*>(self, kCurrentRenderTargetGroup);
    uint32_t width = Virtual<uint32_t>(group, 18, 0u);
    uint32_t height = Virtual<uint32_t>(group, 19, 0u);
    float widthF = float(width);
    float heightF = float(height);
    static constexpr float kScreenView[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f, 1.0f, 1.0f};
    static constexpr float kScreenInverse[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                           0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, -1.0f, 1.0f};
    for (int i = 0; i < 16; i++) {
        At<float>(self, kView + i * 4) = kScreenView[i];
        At<float>(self, kInvView + i * 4) = kScreenInverse[i];
    }
    float* proj = &At<float>(self, kProjection);
    proj[0] = 2.0f;
    proj[4] = 0.0f;
    proj[8] = 0.0f;
    proj[12] = -1.0f / widthF;
    proj[1] = 0.0f;
    proj[9] = 0.0f;
    proj[5] = 2.0f;
    proj[13] = 1.0f / heightF;
    proj[2] = 0.0f;
    proj[6] = 0.0f;
    At<uint32_t>(self, kProjection + 0x28) = 0x38D1BC76;   // 1.0001e-4
    At<uint32_t>(self, kProjection + 0x38) = 0xB8D1BC76;   // -1.0001e-4
    proj[3] = 0.0f;
    proj[7] = 0.0f;
    proj[11] = 0.0f;
    proj[15] = 1.0f;
    Viewport viewport;
    if (port != nullptr) {
        viewport.x = crt::FloatToInt(port[0] * widthF);
        viewport.y = crt::FloatToInt((1.0f - port[2]) * heightF);
        viewport.width = uint32_t(crt::FloatToInt((port[1] - port[0]) * widthF));
        viewport.height = uint32_t(crt::FloatToInt((port[2] - port[3]) * heightF));
        for (int i = 0; i < 4; i++) {
            At<uint32_t>(self, kPort + i * 4) = reinterpret_cast<const uint32_t*>(port)[i];
        }
    } else {
        viewport.height = height;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = width;
        At<float>(self, kPort) = 0.0f;
        At<float>(self, kPort + 0xC) = 0.0f;
        At<float>(self, kPort + 4) = 1.0f;
        At<float>(self, kPort + 8) = 1.0f;
    }
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    Com<long>(Device(self), kRSSetViewports, 1u, &viewport);
    float halfPixelX = 1.0f / (widthF * 2.0f);
    At<uint8_t>(self, kFrustum + 0x18) = 1;
    float halfPixelY = 1.0f / (2.0f * heightF);
    At<float>(self, kFrustum) = halfPixelX - 0.5f;
    At<float>(self, kFrustum + 4) = halfPixelX + 0.5f;
    At<float>(self, kFrustum + 8) = 0.5f - halfPixelY;
    At<float>(self, kFrustum + 0xC) = -0.5f - halfPixelY;
    At<float>(self, kFrustum + 0x10) = 1.0f;
    At<float>(self, kFrustum + 0x14) = 10000.0f;
}

// 0x8AA3F0 (thiscall, slot 76): SetDefaultProgramCache(material, auto write, debug file, load,
// locked, working directory) (a vertex, geometry and pixel NiD3D10GPUProgramCache for the
// material, with the device's profiles)
void __fastcall NiD3D10Renderer_SetDefaultProgramCache(uint8_t* self, re::Edx, uint8_t* material, int autoWriteToDisk,
                                                       int writeDebugFile, int load, int locked,
                                                       const char* workingDir) {
    using ProfileFn = const char*(__stdcall*)(void*);
    const char* profile = ms_hD3D10 != nullptr
                              ? re::PtrCast<ProfileFn>(ms_apfnD3D10[kD3D10GetVertexShaderProfile])(Device(self))
                              : nullptr;
    if (workingDir == nullptr) {
        workingDir = Material_GetDefaultWorkingDirectory();
    }
    SetProgramCache(material, 0, profile, autoWriteToDisk, writeDebugFile, load, locked, workingDir);
    profile = ms_hD3D10 != nullptr
                  ? re::PtrCast<ProfileFn>(ms_apfnD3D10[kD3D10GetGeometryShaderProfile])(Device(self))
                  : nullptr;
    SetProgramCache(material, 2, profile, autoWriteToDisk, writeDebugFile, load, locked, workingDir);
    profile = ms_hD3D10 != nullptr
                  ? re::PtrCast<ProfileFn>(ms_apfnD3D10[kD3D10GetPixelShaderProfile])(Device(self))
                  : nullptr;
    SetProgramCache(material, 1, profile, autoWriteToDisk, writeDebugFile, load, locked, workingDir);
}

// 0x8AC590 (thiscall, slot 82): Do_BeginFrame (none once the device is lost; while occluded the
// default swap chain tested; the buffers in use list emptied)
uint8_t __fastcall NiD3D10Renderer_BeginFrame(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, kDeviceRemoved) != 0) {
        return 0;
    }
    if (At<uint8_t>(self, kOccluded) != 0) {
        void* group = At<void*>(self, kDefaultRenderTargetGroup);
        if (group == nullptr) {
            return 0;
        }
        void* data = GroupData(group, 0);
        long hr = Virtual<long>(data, 4, 0u, 1u);   // Present(0, DXGI_PRESENT_TEST)
        Renderer_HandleDisplayFrameResult(self, hr);
        if (At<uint8_t>(self, kOccluded) != 0) {
            return 0;
        }
    }
    uint8_t* list = self + kBuffersInUse;
    while (At<uint32_t>(list, 0xC) != 0) {
        uint8_t* node = At<uint8_t*>(list, 4);
        uint8_t* next = At<uint8_t*>(node, 0);
        At<uint8_t*>(list, 4) = next;
        if (next != nullptr) {
            At<void*>(next, 4) = nullptr;
        } else {
            At<void*>(list, 8) = nullptr;
        }
        void* object = At<void*>(node, 8);
        AddRef(object);
        Virtual<void>(list, 2, node);
        At<uint32_t>(list, 0xC)--;
        ReleaseRef(object);
    }
    return 1;
}

// 0x8AC660 (thiscall, slot 84): Do_DisplayFrame (each buffer in use presented with the sync
// interval at +0x440; false at the first the result handling refuses)
uint8_t __fastcall NiD3D10Renderer_DisplayFrame(uint8_t* self, re::Edx) {
    uint8_t* list = self + kBuffersInUse;
    while (At<uint32_t>(list, 0xC) != 0) {
        uint8_t* node = At<uint8_t*>(list, 4);
        uint8_t* next = At<uint8_t*>(node, 0);
        At<uint8_t*>(list, 4) = next;
        if (next != nullptr) {
            At<void*>(next, 4) = nullptr;
        } else {
            At<void*>(list, 8) = nullptr;
        }
        void* object = At<void*>(node, 8);
        AddRef(object);
        Virtual<void>(list, 2, node);
        At<uint32_t>(list, 0xC)--;
        long hr = Virtual<long>(object, 4, At<uint32_t>(self, 0x440), 0u);
        if (Renderer_HandleDisplayFrameResult(self, hr) == 0) {
            ReleaseRef(object);
            return 0;
        }
        if (sys::Decrement(&At<uint32_t>(object, 4)) == 0) {   // (not checked for null here)
            Virtual<void>(object, 1);
        }
    }
    return 1;
}

// 0x8AC720 (thiscall, slot 90): Do_EndUsingRenderTargetGroup (its swap chain buffers put in the
// buffers in use list once each; no current group)
uint8_t __fastcall NiD3D10Renderer_EndUsingRenderTargetGroup(uint8_t* self, re::Edx) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099FAE8));
        return 0;
    }
    for (uint32_t i = 0; i < Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 24); i++) {
        void* data = GroupData(At<void*>(self, kCurrentRenderTargetGroup), i);
        if (data == nullptr || Virtual<uint8_t>(data, 3) == 0) {
            continue;
        }
        AddRef(data);
        bool listed = false;
        for (uint8_t* node = At<uint8_t*>(self, kBuffersInUse + 4); node != nullptr; node = At<uint8_t*>(node, 0)) {
            if (data == At<void*>(node, 8)) {
                listed = true;
                break;
            }
        }
        ReleaseRef(data);
        if (!listed) {
            AddRef(data);
            re::Ptr32<void> element = data;
            PointerList_AddTail(self + kBuffersInUse, &element);
            ReleaseRef(data);
        }
    }
    At<void*>(self, kCurrentRenderTargetGroup) = nullptr;
    return 1;
}

// 0x8AC840 (thiscall, slot 93): Do_BatchRenderShape
void __fastcall NiD3D10Renderer_BatchRenderShape(uint8_t* self, re::Edx, uint8_t* shape) {
    BatchRender(self, shape, 95);
}

// 0x8AC960 (thiscall, slot 94): Do_BatchRenderStrips
void __fastcall NiD3D10Renderer_BatchRenderStrips(uint8_t* self, re::Edx, uint8_t* strips) {
    BatchRender(self, strips, 96);
}

// 0x8AE770 / 0x8AE7B0 / 0x8AE7F0 / 0x8AE830 (thiscall, slots 95..98): Do_RenderShape,
// Do_RenderTristrips, Do_RenderPoints, Do_RenderLines (the geometry rendered as its kind 0..3
// when it has vertices)
namespace {
void RenderKind(uint8_t* self, uint8_t* geometry, uint32_t kind) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099FE80));
        return;
    }
    if (At<uint16_t>(At<uint8_t*>(geometry, 0xCC), 0x30) != 0) {
        Renderer_RenderGeometry(self, geometry, kind);
    }
}
} // namespace
void __fastcall NiD3D10Renderer_RenderShape(uint8_t* self, re::Edx, uint8_t* geometry) {
    RenderKind(self, geometry, 0);
}
void __fastcall NiD3D10Renderer_RenderTristrips(uint8_t* self, re::Edx, uint8_t* geometry) {
    RenderKind(self, geometry, 1);
}
void __fastcall NiD3D10Renderer_RenderPoints(uint8_t* self, re::Edx, uint8_t* geometry) {
    RenderKind(self, geometry, 2);
}
void __fastcall NiD3D10Renderer_RenderLines(uint8_t* self, re::Edx, uint8_t* geometry) {
    RenderKind(self, geometry, 3);
}

// 0x8AF540 (thiscall, slot 46): PrecacheGeometry(geometry, bones per partition, bones per vertex)
// (its renderer data found for its extra data, else made; its skin partition made; packed by its
// kind)
uint8_t __fastcall NiD3D10Renderer_PrecacheGeometry(uint8_t* self, re::Edx, uint8_t* geometry, uint32_t, uint32_t) {
    void* model = At<void*>(geometry, 0xCC);
    void* extra = At<void*>(geometry, 0xD0);
    void* data = At<void*>(model, 0x3C);
    while (data != nullptr && GeometryData_GetOwner(data) != extra) {
        data = GeometryData_Next(data);
    }
    if (data == nullptr) {
        data = Renderer_CreateGeometryData(self, geometry);
    }
    void* skin = At<void*>(geometry, 0xE4);
    if (skin == nullptr) {
        if (HasClassBit(geometry, NiParticles_ClassBit)) {
            uint32_t count = GeometryData_GetVertexCount(data);
            uint8_t flag = GeometryData_Function8B7670(count);
            GeometryData_SetIndexBuffer(data, GeometryData_CreateIndexBuffer(flag, 1, 1, 1));
            Geometry_UpdateSkinPartition(geometry);
            skin = At<void*>(geometry, 0xE4);
            GeometryData_SetIndexBuffer(data, count);
        } else {
            Geometry_UpdateSkinPartition(geometry);
            skin = At<void*>(geometry, 0xE4);
        }
    }
    if (HasClassBit(geometry, NiParticles_ClassBit)) {
        return Renderer_PackParticles(self, data, model, skin, reinterpret_cast<void*>(0x00BC8A9C),
                                  reinterpret_cast<void*>(0x00BC8AA8), 0) != 0
                   ? 1
                   : 0;
    }
    if (HasClassBit(geometry, NiLines_ClassBit)) {
        return Renderer_PackLines(self, data, model, skin, 0) != 0 ? 1 : 0;
    }
    return Renderer_PackGeometry(self, data, model, extra, skin, 0) != 0 ? 1 : 0;
}

// 0x8AF6E0 (thiscall, slot 99): Do_RenderScreenTexture (packed, its texture on the screen
// texture node's property, rendered with the render state's +0x18 off; the default texture put
// back)
void __fastcall NiD3D10Renderer_RenderScreenTexture(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099FE80));
        return;
    }
    if (screenTexture == nullptr || At<void*>(screenTexture, 0x10) == nullptr) {
        return;
    }
    Renderer_PackScreenTexture(self, screenTexture);
    void* property = At<void*>(At<uint8_t*>(self, 0x468), 0xC4);
    AddRef(property);
    TexturingProperty_SetBaseTexture(property, At<void*>(screenTexture, 0x14));
    uint32_t state[10];
    RenderStateManager_GetState(At<void*>(self, kRenderStateManager), state);
    uint8_t saved = state[6] != 0 ? 1 : 0;
    state[6] = 0;
    RenderStateManager_SetState(At<void*>(self, kRenderStateManager), state);
    ScreenTextureNode_Render(At<void*>(self, 0x468), self);
    state[6] = saved;
    RenderStateManager_SetState(At<void*>(self, kRenderStateManager), state);
    TexturingProperty_SetBaseTexture(property, ms_spDefaultTexture);
    ReleaseRef(property);
}

#define C42F_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42F_REVERSED("NiD3D10Renderer::ValidateRenderTargetGroup", 0x008A97C0, NiD3D10Renderer_ValidateRenderTargetGroup);
C42F_REVERSED("NiD3D10Renderer::Do_SetDefaultProgramCache", 0x008AA3F0, NiD3D10Renderer_SetDefaultProgramCache);
C42F_REVERSED("NiD3D10Renderer::Do_SetCameraData", 0x008AA8D0, NiD3D10Renderer_SetCameraData);
C42F_REVERSED("NiD3D10Renderer::Do_SetScreenSpaceCameraData", 0x008AAD80, NiD3D10Renderer_SetScreenSpaceCameraData);
C42F_REVERSED("NiD3D10Renderer::IsDepthBufferCompatible", 0x008AB770, NiD3D10Renderer_IsDepthBufferCompatible);
C42F_REVERSED("NiD3D10Renderer::Do_BeginFrame", 0x008AC590, NiD3D10Renderer_BeginFrame);
C42F_REVERSED("NiD3D10Renderer::Do_DisplayFrame", 0x008AC660, NiD3D10Renderer_DisplayFrame);
C42F_REVERSED("NiD3D10Renderer::Do_EndUsingRenderTargetGroup", 0x008AC720,
              NiD3D10Renderer_EndUsingRenderTargetGroup);
C42F_REVERSED("NiD3D10Renderer::Do_BatchRenderShape", 0x008AC840, NiD3D10Renderer_BatchRenderShape);
C42F_REVERSED("NiD3D10Renderer::Do_BatchRenderStrips", 0x008AC960, NiD3D10Renderer_BatchRenderStrips);
C42F_REVERSED("NiD3D10Renderer::Do_RenderShape", 0x008AE770, NiD3D10Renderer_RenderShape);
C42F_REVERSED("NiD3D10Renderer::Do_RenderTristrips", 0x008AE7B0, NiD3D10Renderer_RenderTristrips);
C42F_REVERSED("NiD3D10Renderer::Do_RenderPoints", 0x008AE7F0, NiD3D10Renderer_RenderPoints);
C42F_REVERSED("NiD3D10Renderer::Do_RenderLines", 0x008AE830, NiD3D10Renderer_RenderLines);
C42F_REVERSED("NiD3D10Renderer::PrecacheGeometry", 0x008AF540, NiD3D10Renderer_PrecacheGeometry);
C42F_REVERSED("NiD3D10Renderer::Do_RenderScreenTexture", 0x008AF6E0, NiD3D10Renderer_RenderScreenTexture);

} // namespace ni::c42
