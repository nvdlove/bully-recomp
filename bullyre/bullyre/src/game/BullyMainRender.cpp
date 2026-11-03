// BullyMain's renderer side (0x402E90 on): BullyNiDX9Renderer's DrawPrimitive (the
// shader passes and the D3D9 draw calls) and BullyMain::CreateRenderer. Both hold an
// address bullygfx patches in place (0x40309C, 0x403206): written, not hooked.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"
#include "runflags.h"

namespace {

GAME_FN(void, __cdecl, DebugPrint, 0x0049A650, const char* format, ...);   // (does nothing)
GAME_FN(uint8_t*, __thiscall, Geometry_GetShader, 0x00761630, void* geometry);
GAME_FN(void, __thiscall, Geometry_Fn761F70, 0x00761F70, void* geometry, void* renderer, int);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rttiInfo, void* object);
GAME_FN(uint8_t, __thiscall, SelectEntry, 0x00402880, void* self, uint32_t key);
GAME_VAR(uint32_t, ms_NiD3DShaderInterfaceBit, 0x020E6E14);
inline void* const ms_NiD3DShaderInterfaceRTTI = reinterpret_cast<void*>(0x020E6E0C);

// CreateRenderer's callees
GAME_FN(void, __cdecl, SetShaderDirectory, 0x00762A40, const char* path);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_FN(uint8_t*, __cdecl, MaterialLibrary_Find, 0x007627E0, re::Ptr32<char>* name);
GAME_FN(void, __cdecl, MaterialLibrary_Load, 0x00762820, void* library);
GAME_FN(void, __cdecl, Fn_518450, 0x00518450);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_VAR(void*, ms_pShaderConstantManagerFactory, 0x020E6D70);
GAME_VAR(uint8_t, ms_bRendererCreated_20E2180, 0x020E2180);
GAME_VAR(uint8_t, ms_bWidescreen, 0x00C1870E);
GAME_VAR(uint32_t, ms_adapter_BCBC24, 0x00BCBC24);
GAME_FN(int, __cdecl, Renderer_GetDeviceId, 0x00878D10);
GAME_FN(void, __cdecl, GetDisplaySettings, 0x00405BA0, uint32_t* settings);
GAME_FN(void, __cdecl, Fn_4055D0, 0x004055D0, int);
GAME_FN(uint8_t*, __cdecl, CreateNiRenderer, 0x00401D50, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
GAME_VAR(uint8_t, ms_bRenderStateB5_COPL, 0x00BCBB54);
GAME_VAR(uint8_t, ms_bRenderStateB5_COPM, 0x009D3451);
GAME_ARRAY(char, ms_rendererError, 0x020E2280);
GAME_FN(int, __cdecl, FormatString, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_VAR(uint32_t, ms_renderer_BCBB4C, 0x00BCBB4C);
inline void* const ms_renderer_20E25E8 = reinterpret_cast<void*>(0x020E25E8);
GAME_FN(void, __thiscall, Fn_50DE30, 0x0050DE30, void* self);
GAME_FN(void, __cdecl, Fn_4096F0, 0x004096F0);
GAME_FN(void, __cdecl, Fn_4094B0, 0x004094B0, int);
GAME_FN(void, __cdecl, Fn_4096C0, 0x004096C0);
GAME_FN(void, __cdecl, Fn_409750, 0x00409750);
GAME_FN(void, __cdecl, ShowSystemMessage, 0x0043B0F0, const char* key);
GAME_FN(void, __thiscall, Renderer_SetDefaultMaterial, 0x007559A0, void* renderer, void* material);
GAME_FN(uint8_t*, __thiscall, MdBullyAccumulator_Construct, 0x00515510, void* self);
GAME_FN(uint8_t*, __thiscall, MdBullyCullingProcess_Construct, 0x00515610, void* self, void* visible);
constexpr uint32_t kMdShaderConstantManagerFactoryVtable = 0x008FF320;

void Release(uint8_t* object) {
    if (vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

void Assign(re::Ptr32<uint8_t>& p, uint8_t* object) {
    if (p == object) {
        return;
    }
    if (p) {
        Release(p);
    }
    p = object;
    if (object) {
        vc::Increment(object + 4);
    }
}

void ReleaseString(re::Ptr32<char>* handle) {
    char* s = *handle;
    if (s == ms_nullString) {
        return;
    }
    uint16_t length = *reinterpret_cast<uint16_t*>(s - 2);
    if (vc::Decrement(s - 8) == 1) {
        StringTable_Release(ms_pStringTable, handle, length);
    }
}

} // namespace

namespace BullyNiDX9Renderer {

// 0x402E90 (thiscall): DrawPrimitive: the geometry's shader (the error shader, vtable[64],
// when there's none), then its passes: set up, each pass's geometry drawn (DrawPrimitive
// or DrawIndexedPrimitive per strip), torn down. The pass's own data (vtable[11]'s) is
// what later calls get instead of `rendererData` (the game's)
[[maybe_unused]] static void __fastcall DrawPrimitive(uint8_t* self, re::Edx, uint8_t* geometry, uint32_t /*unused*/,
                                                      uint32_t skin, uint32_t transform, uint32_t bound,
                                                      uint8_t* rendererData) {
    uint8_t* shader;
    if (!geometry) {
        shader = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x8A0);
    } else {
        shader = Geometry_GetShader(geometry);
        if (shader) {
            uint8_t* rtti = vc::Call<uint8_t*>(shader, 0x08);
            uint32_t bit = ms_NiD3DShaderInterfaceBit;
            if (!(*reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4) & (1u << (bit & 0x1F)))) {
                shader = nullptr;
            }
        }
        if (!shader) {
            uint32_t index = *reinterpret_cast<uint32_t*>(geometry + 0xE0);
            uint32_t key = 0;
            if (index < *reinterpret_cast<uint32_t*>(geometry + 0xDC)) {
                key = *reinterpret_cast<uint32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(geometry + 0xD4) + index * 0x10 + 4);
            }
            Geometry_Fn761F70(geometry, *reinterpret_cast<re::Ptr32<void>*>(self + 0x18), -1);
            shader = IsKindOf(ms_NiD3DShaderInterfaceRTTI, Geometry_GetShader(geometry));
            SelectEntry(geometry, key);
        }
    }
    if (!shader) {
        // (reads the geometry's name even when there's no geometry)
        DebugPrint("DrawPrimitive> No shader found for object %s!\nUsing Error Shader!\n",
                   *reinterpret_cast<re::Ptr32<const char>*>(geometry + 8));
        shader = vc::Call<uint8_t*>(self, 0x100);
    }
    uint32_t c = *reinterpret_cast<uint32_t*>(self + 0xC);
    uint32_t d = *reinterpret_cast<uint32_t*>(self + 0x10);
    if (vc::Call<int>(shader, 0x18, geometry, skin, rendererData, c, d, transform, bound) != 0) {
        return;
    }
    vc::Call<void>(shader, 0x1C, geometry, skin, rendererData, c, d, transform, bound);
    uint8_t* data = rendererData;
    void* device = *reinterpret_cast<re::Ptr32<void>*>(self + 0xA4);
    void* const* deviceVtable = re::ComVtable(device);
    for (int more = vc::Call<int>(shader, 0x3C); more; more = vc::Call<int>(shader, 0x40)) {
        vc::Call<void>(shader, 0x20, geometry, skin, data, *reinterpret_cast<uint32_t*>(self + 0xC),
                       *reinterpret_cast<uint32_t*>(self + 0x10), transform, bound);
        vc::Call<void>(shader, 0x24, geometry, skin, 0, data, *reinterpret_cast<uint32_t*>(self + 0xC),
                       *reinterpret_cast<uint32_t*>(self + 0x10), transform, bound);
        data = vc::Call<uint8_t*>(shader, 0x2C, geometry, 0, data, *reinterpret_cast<uint32_t*>(self + 0xC));
        vc::Call<void>(shader, 0x28, geometry, skin, 0, data, *reinterpret_cast<uint32_t*>(self + 0xC),
                       *reinterpret_cast<uint32_t*>(self + 0x10), transform, bound);
        uint8_t* settings = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x6B4);
        vc::Call<void>(*reinterpret_cast<re::Ptr32<void>*>(settings + 0x20F0), 0x08);
        device = *reinterpret_cast<re::Ptr32<void>*>(self + 0xA4);
        deviceVtable = re::ComVtable(device);
        uint32_t type = *reinterpret_cast<uint32_t*>(data + 0x3C);
        uint32_t baseVertex = *reinterpret_cast<uint32_t*>(data + 0x38);
        if (*reinterpret_cast<uint32_t*>(data + 0x34) == 0) {
            // IDirect3DDevice9::DrawPrimitive
            re::PtrCast<long(__stdcall*)(void*, uint32_t, uint32_t, uint32_t)>(deviceVtable[0x144 / 4])(
                device, type, baseVertex, *reinterpret_cast<uint32_t*>(data + 0x40));
        } else {
            uint32_t start = 0;
            for (uint32_t i = 0; i < *reinterpret_cast<uint32_t*>(data + 0x48); i++) {
                uint16_t* lengths = *reinterpret_cast<re::Ptr32<uint16_t>*>(data + 0x4C);
                uint32_t count = lengths ? lengths[i] - 2u : *reinterpret_cast<uint32_t*>(data + 0x40);
                // IDirect3DDevice9::DrawIndexedPrimitive (0x40309C: patched by bullygfx)
                re::PtrCast<long(__stdcall*)(void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(
                    deviceVtable[0x148 / 4])(device, type, baseVertex, 0, *reinterpret_cast<uint32_t*>(data + 0x18), start,
                                             count);
                start += count + 2;
            }
        }
        vc::Call<void>(shader, 0x30, geometry, skin, 0, data, *reinterpret_cast<uint32_t*>(self + 0xC),
                       *reinterpret_cast<uint32_t*>(self + 0x10), transform, bound);
    }
    vc::Call<void>(shader, 0x34, geometry, skin, data, *reinterpret_cast<uint32_t*>(self + 0xC),
                   *reinterpret_cast<uint32_t*>(self + 0x10), transform, bound);
    (*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x6B4))[0x20F5] = 0;
}
RE_UNHOOKED_FN("BullyNiDX9Renderer::DrawPrimitive", 0x00402E90, &DrawPrimitive);

} // namespace BullyNiDX9Renderer

namespace BullyMain {

// 0x403150 (thiscall): BullyMain::CreateRenderer: the shader libraries, the renderer
// (the window's width; the height 3/4 of it unless widescreen), its render states,
// accumulator and culling process. 0 when the renderer couldn't be made
[[maybe_unused]] static uint8_t __fastcall CreateRenderer(uint8_t* self, re::Edx) {
    DebugPrint("CreateRenderer");
    SetShaderDirectory("Shaders\\Generated");
    re::Ptr32<char> name = StringTable_AddString("NiSingleShaderMaterialLibrary");
    uint8_t* library = MaterialLibrary_Find(&name);
    ReleaseString(&name);
    if (library) {
        MaterialLibrary_Load(library);
    }
    Fn_518450();
    void* factory = NiNew(4);
    if (factory) {
        *reinterpret_cast<uint32_t*>(factory) = kMdShaderConstantManagerFactoryVtable;
    } else {
        factory = nullptr;
    }
    if (ms_pShaderConstantManagerFactory) {
        NiFreeSized(ms_pShaderConstantManagerFactory, 4);
    }
    ms_pShaderConstantManagerFactory = factory;
    uint8_t* window = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x2C);
    self[0x34] = re::ForceWindowed() ? 0 : 1;   // (0x403206: patched by bullygfx)
    uint32_t bpp = *reinterpret_cast<uint32_t*>(window + 4);
    ms_bRendererCreated_20E2180 = 1;
    uint32_t width = *reinterpret_cast<uint32_t*>(window + 0x14);
    uint32_t height = *reinterpret_cast<uint32_t*>(window + 0x1C);
    *reinterpret_cast<uint32_t*>(self + 0x38) = 0x20;
    if (!ms_bWidescreen) {
        height = (width * 3) >> 2;
    }
    uint8_t* renderer = nullptr;   // an NiPointer
    DebugPrint("NiXenonRendCreate");
    uint32_t adapter = ms_adapter_BCBC24;
    Renderer_GetDeviceId();
    uint32_t settings[6];   // (GetDisplaySettings writes 6)
    GetDisplaySettings(settings);
    uint32_t multisample = settings[2] < 2 ? 0 : (settings[2] | 0x80000000u);
    Fn_4055D0(1);
    uint8_t* created = CreateNiRenderer(width, height, 0, bpp, bpp, adapter, 0, 2, 0,
                                        reinterpret_cast<uint8_t*>(settings)[0xC] != 0, 0, multisample, 1, 0);
    if (created) {
        renderer = created;
        vc::Increment(created + 4);
    }
    DebugPrint(nullptr);   // (the stack's leftovers)
    void* device = *reinterpret_cast<re::Ptr32<void>*>(ms_pRenderer + 0xA4);
    // IDirect3DDevice9::SetRenderState(0xB5, 'COPL' / 'COPM')
    if (ms_bRenderStateB5_COPL) {
        reinterpret_cast<long(__stdcall*)(void*, uint32_t, uint32_t)>(static_cast<void*>(re::ComVtable(device)[0xE4 / 4]))(
            device, 0xB5, 0x4C504F43);
    }
    if (ms_bRenderStateB5_COPM) {
        reinterpret_cast<long(__stdcall*)(void*, uint32_t, uint32_t)>(static_cast<void*>(re::ComVtable(device)[0xE4 / 4]))(
            device, 0xB5, 0x4D504F43);
    }
    re::Ptr32<uint8_t>& spRenderer = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x24);
    Assign(spRenderer, renderer);
    if (!spRenderer) {
        char message[0x200];   // (formatted and never shown)
        FormatString(message, 0x200, "Xenon Render Creation Error\nError string from renderer was:\n\"%s\"\n",
                     ms_rendererError);
    }
    // (with no renderer these read through null, as the game does)
    vc::Call<void>(spRenderer, 0xB0, ms_renderer_BCBB4C);
    vc::Call<void>(spRenderer, 0x5C, ms_renderer_20E25E8);
    vc::Call<void>(self, 0xF4);
    Fn_50DE30(reinterpret_cast<void*>(0x00A45A30));
    Fn_4096F0();
    Fn_4094B0(7);
    Fn_4096C0();
    Fn_409750();
    uint8_t* r = ms_pRenderer;
    if (*reinterpret_cast<uint32_t*>(r + 0x94) == 3) {
        uint8_t ok = vc::Call<uint8_t>(r, 0x150);
        *reinterpret_cast<uint32_t*>(r + 0x94) = 0;
        if (ok) {
            (*reinterpret_cast<uint32_t*>(r + 0x9C))++;
        }
    }
    if (renderer) {
        Release(renderer);
        renderer = nullptr;
    }
    if (!spRenderer) {
        ShowSystemMessage("SYS_RENDER_FAIL");
        DebugPrint(nullptr);
        return 0;
    }
    name = StringTable_AddString("MdBullyMaterialLibrary");
    uint8_t* bullyLibrary = MaterialLibrary_Find(&name);
    ReleaseString(&name);
    name = StringTable_AddString("BullyDef");
    uint8_t* material = vc::Call<uint8_t*>(bullyLibrary, 0x08, &name);
    ReleaseString(&name);
    if (material) {
        Renderer_SetDefaultMaterial(spRenderer, material);
    }
    void* p = NiNew(0x1C);
    uint8_t* accumulator = p ? MdBullyAccumulator_Construct(p) : nullptr;
    Assign(*reinterpret_cast<re::Ptr32<uint8_t>*>(spRenderer + 8), accumulator);
    p = NiNew(0x94);
    uint8_t* culler = p ? MdBullyCullingProcess_Construct(p, self + 0xA4) : nullptr;
    Assign(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xB4), culler);
    DebugPrint(nullptr);
    return 1;
}
RE_UNHOOKED_FN("BullyMain::CreateRenderer_403150", 0x00403150, &CreateRenderer);

} // namespace BullyMain
