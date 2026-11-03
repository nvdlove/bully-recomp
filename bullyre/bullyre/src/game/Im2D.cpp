// The frontend's immediate 2D drawing straight on the D3D9 device: a growing vertex array
// (begin with a primitive type, set each vertex's position/uv/colour), a batch of up to 500
// quads (0x20D1148), a grid, line strips, their vertex/index buffers, the shaders and
// declarations (compiled from HLSL at first use) and the render states. Positions are in
// 640x480 units (scaled by 0xBC74B0/0xBC74B4), depths get 0.01 added. 0x738B10..0x73A590.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

// a vertex: position, uv, colour (0xAARRGGBB)
struct ImVertex {
    float x, y, z;
    float u, v;
    uint32_t colour;
};
static_assert(sizeof(ImVertex) == 0x18);

// a grid vertex (the second declaration): position, uv
struct ImGridVertex {
    float x, y, z;
    float u, v;
};
static_assert(sizeof(ImGridVertex) == 0x14);

// D3DVERTEXELEMENT9
struct ImElement {
    uint16_t stream;
    uint16_t offset;
    uint8_t type;
    uint8_t method;
    uint8_t usage;
    uint8_t usageIndex;
};
static_assert(sizeof(ImElement) == 8);

constexpr int kMaxQuads = 500;

inline ImVertex* const imQuads = reinterpret_cast<ImVertex*>(0x020D1148);   // kMaxQuads * 4
GAME_VAR(uint8_t*, imRaster, 0x020DCCC8);       // the texture's raster (null: untextured)
GAME_ARRAY(uint8_t, imLineColour, 0x020DCCCC);  // r, g, b, a
GAME_ARRAY(uint8_t, imVertexColour, 0x020DCCD0);
GAME_ARRAY(float, imCornerUV, 0x020DCCD4);      // 4 corners' u, v
GAME_ARRAY(uint8_t, imCornerColour, 0x020DCCF4);  // 4 corners' r, g, b, a
GAME_ARRAY(float, imRectUV, 0x020DCD04);        // u0, v0, u1, v1
GAME_ARRAY(uint8_t, imRectColour, 0x020DCD14);
GAME_VAR(uint8_t, imScreenSpace, 0x020DCD21);   // no view-projection
GAME_VAR(void*, imTexture, 0x020DCD24);         // the D3D texture
GAME_VAR(void*, imDeclaration, 0x020DCD28);
GAME_VAR(void*, imGridDeclaration, 0x020DCD2C);
GAME_VAR(void*, imVsUntextured, 0x020DCD30);
GAME_VAR(void*, imVsTextured, 0x020DCD34);
GAME_VAR(void*, imVsTexturedScreen, 0x020DCD38);
GAME_VAR(void*, imPsTextured, 0x020DCD3C);
GAME_VAR(void*, imPsUntextured, 0x020DCD40);
GAME_VAR(uint32_t, imQuadCount, 0x020DCD44);
GAME_VAR(int32_t, imPrimitive, 0x020DCD48);     // 0 none, 4 triangle list, 5 strip, 6 fan
GAME_VAR(uint32_t, imVertexCount, 0x020DCD4C);
GAME_VAR(int32_t, imVertexIndex, 0x020DCD50);
GAME_VAR(ImVertex*, imVertices, 0x020DCD54);
GAME_VAR(uint32_t, imVertexCapacity, 0x020DCD58);
GAME_VAR(void*, imQuadVB, 0x020DCD5C);
GAME_VAR(void*, imGridVB, 0x020DCD60);
GAME_VAR(void*, imVertexVB, 0x020DCD64);
GAME_VAR(void*, imIB, 0x020DCD68);
GAME_VAR(uint32_t, imBufferUsage, 0x00BC74B8);
GAME_VAR(float, imScaleX, 0x00BC74B0);   // 1/640
GAME_VAR(float, imScaleY, 0x00BC74B4);   // 1/480
GAME_VAR(uint8_t*, imRenderer, 0x020E2380);

GAME_FN(void, __cdecl, Im_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, Im_PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, Im_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Im_Free, 0x005EEAB0, void* p);
inline void* __cdecl Im_Memcpy(void* to, const void* from, uint32_t size) { return crt::Memcpy(to, from, (size_t)size); }
GAME_FN(int, __cdecl, Im_GetRaster, 0x0051A120, void* texture);
GAME_FN(int, __cdecl, Im_GetRenderState, 0x00519290, int state, int32_t* value);
GAME_FN(uint8_t, __cdecl, Im_CanDraw, 0x00407430);
GAME_FN(void, __cdecl, Im_FlushRenderer, 0x00409960);
GAME_FN(void*, __stdcall, Im_MatrixMultiply, 0x0085AC84, float* out, const float* a, const float* b);
GAME_FN(void*, __stdcall, Im_MatrixTranspose, 0x0085AC90, float* out, const float* m);
GAME_FN(long, __stdcall, Im_CompileShader, 0x0085AC96, const char* source, uint32_t length, const void* defines,
        void* include, const char* function, const char* profile, uint32_t flags, void** shader, void** errors,
        void** constants);
// (the game's own)
GAME_FN(void, __cdecl, Im_Reserve, 0x00738B80, uint32_t count);
GAME_FN(void, __cdecl, Im_SetupStates, 0x00738DD0);
GAME_FN(void*, __cdecl, Im_GetQuadVB, 0x00739070);
GAME_FN(void*, __cdecl, Im_GetVertexVB, 0x007390B0);
GAME_FN(void*, __cdecl, Im_GetIB, 0x007390F0);
GAME_FN(void, __cdecl, Im_CreateShaders, 0x00739130);
GAME_FN(void, __cdecl, Im_FlushQuads, 0x00739430);
GAME_FN(void, __cdecl, Im_DrawTexturedRect, 0x00739B90, int, int, const float* uv, void* texture);
GAME_FN(void, __cdecl, Im_DrawGrid, 0x00739C90, int columns, int rows, const float* uv);

// IDirect3DDevice9 and friends (stdcall, this first) by vtable index
template <typename R, typename... A>
R Com(void* obj, int index, A... args) {
    return re::ComCall<R>(obj, index, args...);
}

enum : int {
    kRelease = 2,
    kGetBufferPointer = 3,   // ID3DXBuffer
    kLock = 11,              // IDirect3DVertexBuffer9/IndexBuffer9
    kUnlock = 12,
    kCreateVertexBuffer = 26,
    kCreateIndexBuffer = 27,
    kSetTexture = 65,
    kSetSamplerState = 69,
    kDrawPrimitive = 81,
    kDrawIndexedPrimitive = 82,
    kCreateVertexDeclaration = 86,
    kSetVertexDeclaration = 87,
    kCreateVertexShader = 91,
    kSetVertexShader = 92,
    kSetVertexShaderConstantF = 94,
    kSetStreamSource = 100,
    kSetIndices = 104,
    kCreatePixelShader = 106,
    kSetPixelShader = 107,
};

void* Device() {
    return *reinterpret_cast<re::Ptr32<void>*>(imRenderer + 0xA4);
}

// 0xAARRGGBB from r, g, b, a bytes
uint32_t Argb(const uint8_t* c) {
    return (static_cast<uint32_t>(c[3]) << 24) | (static_cast<uint32_t>(c[0]) << 16) |
           (static_cast<uint32_t>(c[1]) << 8) | c[2];
}

// the raster's D3D texture
void* RasterTexture(uint8_t* raster) {
    uint8_t* texture = *reinterpret_cast<re::Ptr32<uint8_t>*>(raster + 0x34);
    uint8_t* native = *reinterpret_cast<re::Ptr32<uint8_t>*>(texture + 0x24);
    return *reinterpret_cast<re::Ptr32<void>*>(native + 0x5C);
}

// uvs in texels to 0..1 (by the raster's size when it has one)
void ScaleUV(float& u, float& v) {
    uint8_t* raster = imRaster;
    if (raster == nullptr) {
        return;
    }
    int32_t width = *reinterpret_cast<int32_t*>(raster + 0xC);
    if (width != 0) {
        u = u / static_cast<float>(width);
    }
    int32_t height = *reinterpret_cast<int32_t*>(raster + 0x10);
    if (height != 0) {
        v = v / static_cast<float>(height);
    }
}

float Depth(float z) {
    return static_cast<float>(z + 0.01);
}

float Depth(uint32_t z) {
    return static_cast<float>(static_cast<double>(z) + 0.01);
}

void EnsureShaders(void* declaration) {
    if (declaration == nullptr) {
        Im_PushMemoryTag(0x39);
        Im_CreateShaders();
        Im_PopMemoryTag();
    }
}

// the D3D address mode for a RenderWare one
int AddressMode(int32_t rw) {
    switch (rw) {
    case 2: return 2;
    case 3: return 3;
    case 4: return 4;
    default: return 1;
    }
}

void CompileShader(const char* source, const char* profile, re::Ptr32<void>* out, bool pixel) {
    re::Ptr32<void> code;
    re::Ptr32<void> errors;
    Im_CompileShader(source, static_cast<uint32_t>(std::strlen(source)), nullptr, nullptr, "main", profile, 0, re::OutPtr(&code),
                     re::OutPtr(&errors), nullptr);
    Im_PushMemoryTag(0x3C);
    void* device = Device();
    const void* function = Com<const void*>(code, kGetBufferPointer);
    Com<long>(device, pixel ? kCreatePixelShader : kCreateVertexShader, function, out);
    Im_PopMemoryTag();
}

// one corner of a quad: position, its uv and colour slots
void SetCorner(ImVertex& v, float x, float y, float z, int corner) {
    float u = imCornerUV[corner * 2];
    float w = imCornerUV[corner * 2 + 1];
    ScaleUV(u, w);
    v.x = x * imScaleX;
    v.y = y * imScaleY;
    v.z = z;
    v.u = u;
    v.v = w;
    v.colour = Argb(imCornerColour + corner * 4);
}

} // namespace

namespace im2d {

// 0x738B10
void __cdecl ReleaseBuffers() {
    re::Ptr32<void>* buffers[4] = {&imQuadVB, &imGridVB, &imVertexVB, &imIB};
    for (re::Ptr32<void>* b : buffers) {
        if (*b != nullptr) {
            Com<unsigned long>(*b, kRelease);
            *b = nullptr;
        }
    }
}

// 0x738B80: room for count vertices (in steps of 512)
void __cdecl Reserve(uint32_t count) {
    imVertexCount = count;
    if (count <= imVertexCapacity) {
        return;
    }
    if (imVertices != nullptr) {
        Im_Free(imVertices);
    }
    uint32_t capacity = ((count >> 9) + 1) << 9;
    imVertexCapacity = capacity;
    unsigned long long bytes = static_cast<unsigned long long>(capacity) * sizeof(ImVertex);
    imVertices = static_cast<ImVertex*>(Im_Malloc(bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<uint32_t>(bytes)));
}

// 0x738BE0
void __cdecl FreeVertices() {
    Im_Free(imVertices);
    imVertices = nullptr;
}

// 0x738C00
void __cdecl SetTexture(void* texture) {
    if (texture != nullptr) {
        imRaster = reinterpret_cast<uint8_t*>(Im_GetRaster(texture));
    } else {
        imRaster = nullptr;
    }
}

// 0x738C30 (fan), 0x738C60 (strip), 0x738C90 (list)
template <int Primitive>
void __cdecl Begin(uint32_t count) {
    imPrimitive = Primitive;
    Im_Reserve(count);
    imVertexIndex = 0;
}

// 0x738CC0
void __cdecl SetPosition(float x, float y, uint32_t z) {
    if (imPrimitive == 0) {
        return;
    }
    ImVertex& v = imVertices[imVertexIndex];
    v.x = x * imScaleX;
    v.y = y * imScaleY;
    v.z = Depth(z);
}

// 0x738D40
void __cdecl SetUV(float u, float v) {
    if (imPrimitive == 0) {
        return;
    }
    imVertices[imVertexIndex].u = u;
    imVertices[imVertexIndex].v = v;
}

// 0x738D80: the vertex's colour, then the next vertex
void __cdecl EndVertex() {
    if (imPrimitive != 0) {
        imVertices[imVertexIndex].colour = Argb(imVertexColour);
    }
    imVertexIndex++;
}

// 0x738DD0: the device's states for the drawing (shaders, texture, sampler, the
// view-projection as the vertex shader's c0..c3)
void __cdecl SetupStates() {
    alignas(16) float transposed[16];
    alignas(16) float product[16];
    alignas(16) float view[16];
    alignas(16) float projection[16];
    int32_t state;
    Im_GetRenderState(6, &state);   // (read, unused)
    Im_GetRenderState(8, &state);
    Im_GetRenderState(0xC, &state);
    Im_GetRenderState(0xA, &state);
    Im_GetRenderState(0xB, &state);
    Im_GetRenderState(0x1D, &state);
    Im_GetRenderState(0x1E, &state);
    void* device = Device();
    Com<long>(device, kSetIndices, static_cast<void*>(nullptr));
    if (imTexture != nullptr) {
        Com<long>(device, kSetVertexShader, imScreenSpace ? imVsTexturedScreen : imVsTextured);
        Com<long>(device, kSetPixelShader, imPsTextured);
        Com<long>(device, kSetTexture, 0u, imTexture);
        Com<long>(device, kSetSamplerState, 0u, 6u, 2u);   // min: linear
        Com<long>(device, kSetSamplerState, 0u, 5u, 2u);   // mag
        Com<long>(device, kSetSamplerState, 0u, 7u, 2u);   // mip
        Im_GetRenderState(3, &state);
        Com<long>(device, kSetSamplerState, 0u, 1u, AddressMode(state));   // address u
        Im_GetRenderState(4, &state);
        Com<long>(device, kSetSamplerState, 0u, 2u, AddressMode(state));   // address v
    } else {
        Com<long>(device, kSetVertexShader, imVsUntextured);
        Com<long>(device, kSetPixelShader, imPsUntextured);
        Com<long>(device, kSetTexture, 0u, static_cast<void*>(nullptr));
    }
    Com<long>(device, kSetVertexDeclaration, imDeclaration);
    if (!imScreenSpace) {
        std::memcpy(projection, imRenderer + 0x7D0, sizeof(projection));
        std::memcpy(view, imRenderer + 0x790, sizeof(view));
        Im_MatrixMultiply(product, view, projection);
        Im_MatrixTranspose(transposed, product);
        Com<long>(device, kSetVertexShaderConstantF, 0u, static_cast<const float*>(transposed), 4u);
    } else {
        std::memcpy(projection, imRenderer + 0x7D0, sizeof(projection));
        Im_MatrixTranspose(product, projection);
        Com<long>(device, kSetVertexShaderConstantF, 0u, static_cast<const float*>(product), 4u);
    }
}

// 0x739070 (the quads'), 0x7390B0 (the vertex array's): a dynamic 0x30000-byte vertex buffer
template <uintptr_t Slot>
void* __cdecl GetVertexBuffer() {
    re::Ptr32<void>* vb = reinterpret_cast<re::Ptr32<void>*>(Slot);
    if (*vb == nullptr) {
        Com<long>(Device(), kCreateVertexBuffer, 0x30000u, imBufferUsage, 0u, 0u, vb, static_cast<void*>(nullptr));
    }
    return *vb;
}

// 0x7390F0: the 32-bit index buffer
void* __cdecl GetIndexBuffer() {
    if (imIB == nullptr) {
        Com<long>(Device(), kCreateIndexBuffer, 0x18000u, imBufferUsage, 0x66u, 0u, &imIB, static_cast<void*>(nullptr));
    }
    return imIB;
}

// 0x739130: the declarations and shaders (their compiled code is never released)
void __cdecl CreateShaders() {
    void* device = Device();
    ImElement elements[4] = {
        {0, 0, 2, 0, 0, 0},       // position: float3
        {0, 0xC, 1, 0, 5, 0},     // texcoord: float2
        {0, 0x14, 5, 0, 0xA, 0},  // colour: ubyte4
        {0xFF, 0, 0x11, 0, 0, 0}, // end
    };
    Com<long>(device, kCreateVertexDeclaration, static_cast<const ImElement*>(elements), &imDeclaration);
    ImElement gridElements[3] = {
        {0, 0, 2, 0, 0, 0},
        {0, 0xC, 1, 0, 5, 0},
        {0xFF, 0, 0x11, 0, 0, 0},
    };
    Com<long>(device, kCreateVertexDeclaration, static_cast<const ImElement*>(gridElements), &imGridDeclaration);
    CompileShader(reinterpret_cast<const char*>(0x0094EA80), "vs_2_0", &imVsTextured, false);
    CompileShader(reinterpret_cast<const char*>(0x0094E380), "vs_2_0", &imVsTexturedScreen, false);
    CompileShader(reinterpret_cast<const char*>(0x0094DE10), "vs_2_0", &imVsUntextured, false);
    CompileShader(reinterpret_cast<const char*>(0x0094DA60), "ps_2_0", &imPsTextured, true);
    CompileShader(reinterpret_cast<const char*>(0x0094D798), "ps_2_0", &imPsUntextured, true);
}

// 0x739430: draw the batched quads
void __cdecl FlushQuads() {
    if (imQuadCount == 0 || !Im_CanDraw()) {
        return;
    }
    if (!imScreenSpace) {
        Im_FlushRenderer();
    }
    Im_SetupStates();
    uint32_t count = imQuadCount;
    void* device = Device();
    uint32_t vertices = count * 4;
    uint32_t bytes = vertices * sizeof(ImVertex);
    uint32_t indices = count * 6;
    void* vb = Im_GetQuadVB();
    void* ib = Im_GetIB();
    void* data;
    if (Com<long>(vb, kLock, 0u, bytes, &data, 0x2000u) >= 0) {   // discard
        Im_Memcpy(data, imQuads, bytes);
        Com<long>(vb, kUnlock);
    }
    uint32_t* index;
    if (Com<long>(ib, kLock, 0u, indices * 4, &index, 0x2000u) >= 0) {
        uint32_t base = 0;
        for (int32_t i = 0; i < static_cast<int32_t>(imQuadCount); i++) {
            index[0] = base;
            index[1] = base + 1;
            index[2] = base + 2;
            index[3] = base;
            index[4] = base + 2;
            index[5] = base + 3;
            index += 6;
            base += 4;
        }
        Com<long>(ib, kUnlock);
    }
    Com<long>(device, kSetIndices, ib);
    Com<long>(device, kSetStreamSource, 0u, vb, 0u, static_cast<uint32_t>(sizeof(ImVertex)));
    Com<long>(device, kDrawIndexedPrimitive, 4, 0, 0u, vertices, 0u, indices / 3);   // triangle list
    imQuadCount = 0;
}

// 0x7395B0: a rectangle (the rect uv and colour), drawn at once
void __cdecl DrawRect(float x1, float y1, float z1, float x2, float y2, float z2) {
    EnsureShaders(imDeclaration);
    Im_FlushQuads();
    float u0 = imRectUV[0];
    float v0 = imRectUV[1];
    float u1 = imRectUV[2];
    float v1 = imRectUV[3];
    uint8_t* raster = imRaster;
    if (raster != nullptr) {
        int32_t width = *reinterpret_cast<int32_t*>(raster + 0xC);
        if (width != 0) {
            float w = static_cast<float>(width);
            u0 = u0 / w;
            u1 = u1 / w;
        }
        int32_t height = *reinterpret_cast<int32_t*>(raster + 0x10);
        if (height != 0) {
            float h = static_cast<float>(height);
            v0 = v0 / h;
            v1 = v1 / h;
        }
    }
    float sx1 = x1 * imScaleX;
    float sy1 = y1 * imScaleY;
    float sz1 = Depth(z1);
    float sy2 = y2 * imScaleY;
    float sx2 = x2 * imScaleX;
    float sz2 = Depth(z2);
    uint32_t colour = Argb(imRectColour);
    ImVertex* q = imQuads + imQuadCount * 4;
    q[0] = {sx1, sy1, sz1, u0, v0, colour};
    q[1] = {sx1, sy2, sz1, u0, v1, colour};
    q[2] = {sx2, sy2, sz2, u1, v1, colour};
    q[3] = {sx2, sy1, sz2, u1, v0, colour};
    imQuadCount++;
    imTexture = raster != nullptr ? RasterTexture(raster) : nullptr;
    Im_FlushQuads();
}

// 0x739820: a quad by its corners (0, 1, 2, 3 around; the corner uvs and colours), drawn at once
void __cdecl DrawQuad(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3,
                      float y3, float z3) {
    EnsureShaders(imDeclaration);
    Im_FlushQuads();
    ImVertex* q = imQuads + imQuadCount * 4;
    SetCorner(q[0], x0, y0, Depth(z0), 0);
    SetCorner(q[1], x2, y2, Depth(z2), 2);
    SetCorner(q[2], x3, y3, Depth(z3), 3);
    SetCorner(q[3], x1, y1, Depth(z1), 1);
    uint8_t* raster = imRaster;
    imQuadCount++;
    imTexture = raster != nullptr ? RasterTexture(raster) : nullptr;
    Im_FlushQuads();
}

// 0x739B90: a white rectangle with a whole D3D texture (in the batch's first slot)
void __cdecl DrawTexturedRect(int, int, const float* rect, void* texture) {
    EnsureShaders(imGridDeclaration);
    Im_FlushQuads();
    const uint32_t white = 0xFFFFFFFF;
    ImVertex* q = imQuads;
    q[0] = {rect[0], rect[1], 0.01f, 0.0f, 0.0f, white};
    q[1] = {rect[2], rect[1], 0.01f, 1.0f, 0.0f, white};
    q[2] = {rect[2], rect[3], 0.01f, 1.0f, 1.0f, white};
    q[3] = {rect[0], rect[3], 0.01f, 0.0f, 1.0f, white};
    imQuadCount++;
    imTexture = texture;
    Im_FlushQuads();
}

// 0x739C90: a (columns+2) x (rows+2) grid over 0..1 with its uvs spread over the rect, as
// triangle strips joined by degenerate triangles
void __cdecl DrawGrid(int columns, int rows, const float* rect) {
    void* device = Device();
    EnsureShaders(imGridDeclaration);
    int across = columns + 2;
    int down = rows + 2;
    int vertices = down * across;
    int strips = rows + 1;
    int indices = (2 * across + 2) * strips;
    if (imGridVB == nullptr) {
        Com<long>(Device(), kCreateVertexBuffer, 0x14000u, imBufferUsage, 0u, 0u, &imGridVB,
                  static_cast<void*>(nullptr));
    }
    void* vb = imGridVB;
    if (imIB == nullptr) {
        Com<long>(Device(), kCreateIndexBuffer, 0x18000u, imBufferUsage, 0x66u, 0u, &imIB,
                  static_cast<void*>(nullptr));
    }
    void* ib = imIB;
    Com<long>(device, kSetVertexDeclaration, imGridDeclaration);
    ImGridVertex* v;
    if (Com<long>(vb, kLock, 0u, static_cast<uint32_t>(vertices * sizeof(ImGridVertex)), &v, 0x2000u) >= 0) {
        for (int j = 0; j < down; j++) {
            if (across <= 0) {
                continue;
            }
            float du = 1.0f / static_cast<float>(columns + 1);
            float dv = 1.0f / static_cast<float>(rows + 1);
            float y = dv * static_cast<float>(j);
            for (int i = 0; i < across; i++) {
                float x = static_cast<float>(i) * du;
                v->x = x;
                v->y = y;
                v->z = 0.01f;
                float width = std::fabs(rect[2] - rect[0]);
                v->u = x * width + rect[0];
                float height = std::fabs(rect[3] - rect[1]);
                v->v = height * y + rect[1];
                v++;
            }
        }
        Com<long>(vb, kUnlock);
    }
    uint32_t* index;
    if (Com<long>(ib, kLock, 0u, static_cast<uint32_t>(indices * 4), &index, 0x2000u) >= 0) {
        int base = 0;
        for (int s = strips; s > 0; s--) {
            *index++ = base;
            for (int i = 0; i < across; i++) {
                *index++ = base + i;
                *index++ = base + across + i;
            }
            *index++ = base + across * 2 - 1;
            base += across;
        }
        Com<long>(ib, kUnlock);
    }
    Com<long>(device, kSetIndices, ib);
    Com<long>(device, kSetStreamSource, 0u, vb, 0u, static_cast<uint32_t>(sizeof(ImGridVertex)));
    Com<long>(device, kDrawIndexedPrimitive, 5, 0, 0u, static_cast<uint32_t>(vertices), 0u,
              static_cast<uint32_t>(indices - 2));   // triangle strip
}

// 0x739F50: a quad by its corners into the batch (drawn when it is full)
void __cdecl DrawQuadBatched(float x0, float y0, uint32_t z0, float x1, float y1, uint32_t z1, float x2, float y2,
                             uint32_t z2, float x3, float y3, uint32_t z3) {
    EnsureShaders(imDeclaration);
    uint32_t count = imQuadCount;
    ImVertex* q = imQuads + count * 4;
    SetCorner(q[0], x0, y0, Depth(z0), 0);
    SetCorner(q[1], x2, y2, Depth(z2), 2);
    SetCorner(q[2], x3, y3, Depth(z3), 3);
    SetCorner(q[3], x1, y1, Depth(z1), 1);
    uint8_t* raster = imRaster;
    imTexture = raster != nullptr ? RasterTexture(raster) : nullptr;
    imQuadCount = count + 1;
    if (static_cast<int32_t>(imQuadCount) >= kMaxQuads) {
        Im_FlushQuads();
    }
}

// 0x73A300: draw the vertex array (Begin..EndVertex)
void __cdecl Draw() {
    Im_FlushQuads();
    if (imPrimitive == 0 || !Im_CanDraw()) {
        return;
    }
    Im_FlushRenderer();
    uint8_t* raster = imRaster;
    if (raster != nullptr && *reinterpret_cast<re::Ptr32<void>*>(raster + 0x34) != nullptr) {
        imTexture = RasterTexture(raster);
    } else {
        imTexture = nullptr;
    }
    Im_SetupStates();
    uint32_t count = imVertexCount;
    void* device = Device();
    uint32_t bytes = count * sizeof(ImVertex);
    void* vb = Im_GetVertexVB();
    void* data;
    if (Com<long>(vb, kLock, 0u, bytes, &data, 0x2000u) >= 0) {
        Im_Memcpy(data, imVertices, bytes);
        Com<long>(vb, kUnlock);
    }
    Com<long>(device, kSetStreamSource, 0u, vb, 0u, static_cast<uint32_t>(sizeof(ImVertex)));
    int primitive = imPrimitive;
    uint32_t primitives = 0;
    if (primitive == 4) {
        primitives = count / 3;
    } else if (static_cast<uint32_t>(primitive - 5) <= 1) {
        primitives = count - 2;
    }
    Com<long>(device, kDrawPrimitive, primitive, 0u, primitives);
    imPrimitive = 0;
    imVertexIndex = 0;
}

// 0x73A410: a line strip through count points (x, y pairs; the line colour)
void __cdecl DrawLineStrip(uint32_t count, const float* points) {
    if (!Im_CanDraw()) {
        return;
    }
    Im_FlushRenderer();
    imTexture = nullptr;
    Im_SetupStates();
    void* device = Device();
    uint32_t bytes = count * sizeof(ImVertex);
    if (imQuadVB == nullptr) {
        Com<long>(device, kCreateVertexBuffer, 0x30000u, imBufferUsage, 0u, 0u, &imQuadVB, static_cast<void*>(nullptr));
    }
    void* vb = imQuadVB;
    ImVertex* v;
    if (Com<long>(vb, kLock, 0u, bytes, &v, 0x2000u) >= 0) {
        for (uint32_t i = 0; i < count; i++) {
            v[i].x = points[i * 2] * imScaleX;
            v[i].y = points[i * 2 + 1] * imScaleY;
            v[i].z = 0.01f;
            v[i].colour = Argb(imLineColour);
        }
        Com<long>(vb, kUnlock);
    }
    Com<long>(device, kSetStreamSource, 0u, vb, 0u, static_cast<uint32_t>(sizeof(ImVertex)));
    Com<long>(device, kDrawPrimitive, 3, 0u, count - 1);   // line strip
}

// 0x73A550
void __cdecl DrawTexturedFull(int a, int b, void* texture) {
    float rect[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    Im_DrawTexturedRect(a, b, rect, texture);
}

// 0x73A590
void __cdecl DrawGridFull(int columns, int rows) {
    float rect[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    Im_DrawGrid(columns, rows, rect);
}

} // namespace im2d

#define IM2D_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

IM2D_REVERSED("Im2D::ReleaseBuffers", 0x00738B10, &im2d::ReleaseBuffers);
IM2D_REVERSED("Im2D::Reserve", 0x00738B80, &im2d::Reserve);
IM2D_REVERSED("Im2D::FreeVertices", 0x00738BE0, &im2d::FreeVertices);
IM2D_REVERSED("Im2D::SetTexture", 0x00738C00, &im2d::SetTexture);
IM2D_REVERSED("Im2D::BeginFan", 0x00738C30, &im2d::Begin<6>);
IM2D_REVERSED("Im2D::BeginStrip", 0x00738C60, &im2d::Begin<5>);
IM2D_REVERSED("Im2D::BeginList", 0x00738C90, &im2d::Begin<4>);
IM2D_REVERSED("Im2D::SetPosition", 0x00738CC0, &im2d::SetPosition);
IM2D_REVERSED("Im2D::SetUV", 0x00738D40, &im2d::SetUV);
IM2D_REVERSED("Im2D::EndVertex", 0x00738D80, &im2d::EndVertex);
IM2D_REVERSED("Im2D::SetupStates", 0x00738DD0, &im2d::SetupStates);
IM2D_REVERSED("Im2D::GetQuadVB", 0x00739070, &im2d::GetVertexBuffer<0x020DCD5C>);
IM2D_REVERSED("Im2D::GetVertexVB", 0x007390B0, &im2d::GetVertexBuffer<0x020DCD64>);
IM2D_REVERSED("Im2D::GetIndexBuffer", 0x007390F0, &im2d::GetIndexBuffer);
IM2D_REVERSED("Im2D::CreateShaders", 0x00739130, &im2d::CreateShaders);
IM2D_REVERSED("Im2D::FlushQuads", 0x00739430, &im2d::FlushQuads);
IM2D_REVERSED("Im2D::DrawRect", 0x007395B0, &im2d::DrawRect);
IM2D_REVERSED("Im2D::DrawQuad", 0x00739820, &im2d::DrawQuad);
IM2D_REVERSED("Im2D::DrawTexturedRect", 0x00739B90, &im2d::DrawTexturedRect);
IM2D_REVERSED("Im2D::DrawGrid", 0x00739C90, &im2d::DrawGrid);
IM2D_REVERSED("Im2D::DrawQuadBatched", 0x00739F50, &im2d::DrawQuadBatched);
IM2D_REVERSED("Im2D::Draw", 0x0073A300, &im2d::Draw);
IM2D_REVERSED("Im2D::DrawLineStrip", 0x0073A410, &im2d::DrawLineStrip);
IM2D_REVERSED("Im2D::DrawTexturedFull", 0x0073A550, &im2d::DrawTexturedFull);
IM2D_REVERSED("Im2D::DrawGridFull", 0x0073A590, &im2d::DrawGridFull);
