// 0x406CF0 to 0x407580: the extra data the game puts on its NIF objects (colours, floats,
// the "fade" floats, name tags), small renderer queries (alpha to coverage, the render
// target states), a generated 2x2 texture, and compiling an .fxl effect to its .fxb.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(uint8_t, __thiscall, FixedString_Contains, 0x00406510, const re::Ptr32<char>* self, const char* s);

GAME_FN(void*, __thiscall, NiObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<char>* name);
GAME_FN(uint8_t, __thiscall, NiObjectNET_AddExtraData, 0x00764600, void* object, const re::Ptr32<char>* name, void* data);
GAME_FN(void, __thiscall, NiObjectNET_Fn762080, 0x00762080, void* object, void* name, int);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rttiInfo, void* object);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __thiscall, NiColorExtraData_Construct, 0x00769260, void* self, float r, float g, float b, float a);
GAME_FN(void, __thiscall, NiColorExtraData_SetValue, 0x007691F0, void* self, float r, float g, float b, float a);
GAME_FN(void, __thiscall, NiExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, NiFloatsExtraData_Construct, 0x0076CA70, void* self, uint32_t count, const float* values);
GAME_FN(void, __thiscall, NiFloatsExtraData_SetValue, 0x0076C9B0, void* self, uint32_t index, float value);
GAME_FN(uint8_t, __cdecl, HasNameTag, 0x00407050, void* object);
GAME_FN(void, __thiscall, NiPixelData_Construct, 0x0076A610, void* self, uint32_t width, uint32_t height, const void* format,
        uint32_t mipmaps, uint32_t faces);
GAME_FN(void*, __cdecl, NiSourceTexture_CreateFromPixels, 0x007699A0, void* pixels, const void* prefs);
GAME_FN(void*, __cdecl, NiSourceTexture_CreateFromFile, 0x00769910, const re::Ptr32<char>* name, const void* prefs, int, int);
GAME_FN(uint8_t*, __cdecl, GetDisplaySettings, 0x00405BA0, void* out);   // (returns `out`)
GAME_FN(void, __thiscall, Renderer_Fn755CB0, 0x00755CB0, void* renderer, void* object);

GAME_FN(void*, __cdecl, NiFile_GetFile, 0x00753380, const char* path, int mode, uint32_t bufferSize);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(long, __stdcall, D3DXCreateEffectCompiler_, 0x0085AC60, const void* data, uint32_t length, const void* defines,
        void* include, uint32_t flags, void** compiler, void** errors);

inline re::Ptr32<char>* const ms_nameBCC500 = reinterpret_cast<re::Ptr32<char>*>(0x00BCC500);
inline re::Ptr32<char>* const ms_colourExtraName = reinterpret_cast<re::Ptr32<char>*>(0x00BCC504);
inline void* const ms_nameBCC508 = reinterpret_cast<void*>(0x00BCC508);
inline re::Ptr32<char>* const ms_floatsExtraName = reinterpret_cast<re::Ptr32<char>*>(0x00BCC514);
GAME_VAR(char*, ms_tagBCC51C, 0x00BCC51C);
GAME_VAR(char*, ms_tagBCC524, 0x00BCC524);
GAME_VAR(char*, ms_tagBCC528, 0x00BCC528);
GAME_VAR(re::Ptr32<char>*, ms_pFadeName_9D3BB0, 0x009D3BB0);   // a pointer to an NiFixedString
GAME_VAR(re::Ptr32<char>*, ms_pTagName_9D3BD0, 0x009D3BD0);
GAME_VAR(re::Ptr32<char>*, ms_pDepthBiasName_9D3BD4, 0x009D3BD4);
GAME_VAR(uint32_t, ms_NiColorExtraDataBit, 0x020E29E4);
GAME_VAR(uint32_t, ms_NiFloatExtraDataBit, 0x020E2924);
GAME_VAR(uint32_t, ms_NiStringExtraDataBit, 0x020E298C);
GAME_VAR(uint32_t, ms_NiFloatsExtraDataBit, 0x020E2AC4);
GAME_VAR(uint32_t, ms_NiNodeBit_20E25BC, 0x020E25BC);
GAME_VAR(uint32_t, ms_NiDX9RendererBit, 0x020E6D20);
inline void* const ms_rtti_20E2548 = reinterpret_cast<void*>(0x020E2548);
inline void* const ms_rtti_20E2A78 = reinterpret_cast<void*>(0x020E2A78);
GAME_VAR(void*, ms_pWhiteTexture_BCC118, 0x00BCC118);
GAME_VAR(void*, ms_pRandomDirections, 0x00BCC11C);
inline const void* const ms_pixelFormat_BC7750 = reinterpret_cast<const void*>(0x00BC7750);
inline const void* const ms_texturePrefs_BC7624 = reinterpret_cast<const void*>(0x00BC7624);
GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
constexpr uint32_t kNiFloatExtraDataVtable = 0x008FFA8C;

uint8_t* IsKindOfBit(void* object, uint32_t bit) {
    if (!object) {
        return nullptr;
    }
    uint8_t* rtti = vc::Call<uint8_t*>(object, 0x08);
    uint32_t bits = *reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4);
    return (bits & (1u << (bit & 0x1F))) ? static_cast<uint8_t*>(object) : nullptr;
}

// a field of a pointer that may be null (the game writes through null there)
float& FloatAt(uint8_t* p, int offset) {
    return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(p) + offset);
}

uint8_t* NewFloatExtraData(float value) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x10));
    if (!data) {
        return nullptr;
    }
    NiExtraData_Construct(data);
    FloatAt(data, 0xC) = value;
    *reinterpret_cast<uint32_t*>(data) = kNiFloatExtraDataVtable;
    return data;
}

// IDirect3DDevice9::SetRenderState through the renderer's cache field
void SetCachedRenderState(uint8_t* settings, int cacheOffset, uint32_t state, uint32_t value) {
    if (*reinterpret_cast<uint32_t*>(settings + cacheOffset) == value) {
        return;
    }
    void* device = *reinterpret_cast<re::Ptr32<void>*>(settings + 0x2238);
    reinterpret_cast<long(__stdcall*)(void*, uint32_t, uint32_t)>(static_cast<void*>(re::ComVtable(device)[0xE4 / 4]))(device, state,
                                                                                                                  value);
    *reinterpret_cast<uint32_t*>(settings + cacheOffset) = value;
}

bool RendererTargetStateOk() {
    int32_t state = *reinterpret_cast<int32_t*>(ms_pRenderer + 0x94);
    return (state == 1 || state == 2 || state == 4) && ms_pRenderer[0x90];
}

} // namespace

namespace NiExtraDataHelpers {

// 0x406CF0: an object's colour extra data (0xBCC504) set (made when missing), then
// 0x762080(0xBCC508)
static void __cdecl SetColour(void* object, float r, float g, float b, float a) {
    uint8_t* extra = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_colourExtraName), ms_NiColorExtraDataBit);
    if (extra) {
        NiColorExtraData_SetValue(extra, r, g, b, a);
    } else {
        uint8_t* data = static_cast<uint8_t*>(NiNew(0x1C));
        if (data) {
            NiColorExtraData_Construct(data, r, g, b, a);
        }
        FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(reinterpret_cast<uintptr_t>(data) + 8), ms_colourExtraName);
        NiObjectNET_AddExtraData(object, ms_colourExtraName, data);
    }
    NiObjectNET_Fn762080(object, ms_nameBCC508, -1);
}

// 0x406DF0: a float extra data (named by 0x9D3BB0) of 1 added when missing
static void __cdecl AddFadeExtraData(void* object) {
    if (IsKindOfBit(NiObjectNET_GetExtraData(object, ms_pFadeName_9D3BB0), ms_NiFloatExtraDataBit)) {
        return;
    }
    uint8_t* data = NewFloatExtraData(1.0f);
    FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(reinterpret_cast<uintptr_t>(data) + 8), ms_pFadeName_9D3BB0);
    NiObjectNET_AddExtraData(object, ms_pFadeName_9D3BB0, data);
}

// 0x406E80: a 2x2 texture (red, green, blue, white; RGBA bytes), made once (0xBCC118);
// returns it (the game leaves it in eax, and callers use it)
static void* __cdecl CreateTestTexture() {
    if (ms_pWhiteTexture_BCC118) {
        return ms_pWhiteTexture_BCC118;
    }
    uint8_t* pixels = static_cast<uint8_t*>(NiNew(0x70));
    if (pixels) {
        NiPixelData_Construct(pixels, 2, 2, ms_pixelFormat_BC7750, 1, 1);
    }
    uint8_t* p = **reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(pixels + 0x5C) + *reinterpret_cast<uint32_t*>(pixels + 0x50);
    static const uint8_t kPixels[16] = {0xFF, 0, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0xFF};
    for (int i = 0; i < 16; i++) {
        p[i] = kPixels[i];
    }
    (*reinterpret_cast<uint32_t*>(pixels + 0x68))++;   // the revision
    ms_pWhiteTexture_BCC118 = NiSourceTexture_CreateFromPixels(pixels, ms_texturePrefs_BC7624);
    return ms_pWhiteTexture_BCC118;
}

// 0x406F40: an object to draw as geometry: not hidden (+0xA8 & 1), and a node or one of
// two classes
static uint8_t __cdecl IsDrawable(uint8_t* object) {
    if (object[0xA8] & 1) {
        return 0;
    }
    if (IsKindOfBit(object, ms_NiNodeBit_20E25BC)) {
        return 1;
    }
    if (IsKindOf(ms_rtti_20E2548, object)) {
        return 1;
    }
    return IsKindOf(ms_rtti_20E2A78, object) != nullptr;
}

// 0x406FB0: the depth bias float extra data (named by 0x9D3BD4) set to 0 (made when
// missing)
static void __cdecl ClearDepthBias(void* object) {
    uint8_t* extra = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_pDepthBiasName_9D3BD4), ms_NiFloatExtraDataBit);
    if (extra) {
        FloatAt(extra, 0xC) = 0.0f;
        return;
    }
    uint8_t* data = NewFloatExtraData(0.0f);
    FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(reinterpret_cast<uintptr_t>(data) + 8), ms_pDepthBiasName_9D3BD4);
    NiObjectNET_AddExtraData(object, ms_pDepthBiasName_9D3BD4, data);
    FloatAt(data, 0xC) = 0.0f;
}

// 0x407050: the object's string extra data (0xBCC500) contains the tag at 0x9D3BD0
static uint8_t __cdecl HasNameTag_(void* object) {
    if (!object) {
        return 0;
    }
    uint8_t* extra = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_nameBCC500), ms_NiStringExtraDataBit);
    if (extra && FixedString_Contains(re::PtrCast<const re::Ptr32<char>*>(extra + 0xC), *ms_pTagName_9D3BD0)) {
        return 1;
    }
    return 0;
}

// 0x4070C0: the depth bias set to 0.001 (0.002 when both forced and tagged). (When the
// object has none, the game writes through null)
static void __cdecl SetDepthBias(void* object, int force, int doubled) {
    bool tagged;
    if (re::Flag(force) || HasNameTag(object)) {
        tagged = true;
    } else {
        if (!re::Flag(doubled)) {
            return;
        }
        tagged = false;
    }
    float bias = 0.001f;
    if (re::Flag(doubled) && tagged) {
        bias = 0.002f;
    }
    void* extra = NiObjectNET_GetExtraData(object, ms_pDepthBiasName_9D3BD4);
    FloatAt(IsKindOfBit(extra, ms_NiFloatExtraDataBit), 0xC) = bias;
}

// 0x407160: a floats extra data (0xBCC514) of four zeros added when missing
static void __cdecl AddFloatsExtraData(void* object) {
    if (IsKindOfBit(NiObjectNET_GetExtraData(object, ms_floatsExtraName), ms_NiFloatsExtraDataBit)) {
        return;
    }
    float values[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x14));
    if (data) {
        NiFloatsExtraData_Construct(data, 4, values);
    }
    FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(reinterpret_cast<uintptr_t>(data) + 8), ms_floatsExtraName);
    NiObjectNET_AddExtraData(object, ms_floatsExtraName, data);
}

// 0x407200: the string extra data (0xBCC500) contains either of two tags
static uint8_t __cdecl HasTagEither(void* object) {
    uint8_t* extra = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_nameBCC500), ms_NiStringExtraDataBit);
    if (!extra) {
        return 0;
    }
    const re::Ptr32<char>* text = reinterpret_cast<const re::Ptr32<char>*>(extra + 0xC);
    if (FixedString_Contains(text, ms_tagBCC51C) || FixedString_Contains(text, ms_tagBCC524)) {
        return 1;
    }
    return 0;
}

// 0x407270: ... contains the tag at 0xBCC528
static uint8_t __cdecl HasTag528(void* object) {
    uint8_t* extra = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_nameBCC500), ms_NiStringExtraDataBit);
    if (extra && FixedString_Contains(re::PtrCast<const re::Ptr32<char>*>(extra + 0xC), ms_tagBCC528)) {
        return 1;
    }
    return 0;
}

// 0x4072D0: Graphics\NormalizedRandomDirections.tga, loaded once
static void* __cdecl GetRandomDirectionsTexture() {
    if (!ms_pRandomDirections) {
        re::Ptr32<char> name = StringTable_AddString("Graphics\\NormalizedRandomDirections.tga");
        ms_pRandomDirections = NiSourceTexture_CreateFromFile(&name, ms_texturePrefs_BC7624, 0, 0);
        if (name != ms_nullString) {
            uint16_t length = *reinterpret_cast<uint16_t*>(name - 2);
            if (vc::Decrement(name - 8) == 1) {
                StringTable_Release(ms_pStringTable, &name, length);
            }
        }
    }
    return ms_pRandomDirections;
}

// 0x407340: the floats extra data's first two values set (not checked for null)
static void __cdecl SetFloatsExtraData(void* object, const float* values) {
    uint8_t* extra = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_floatsExtraName), ms_NiFloatsExtraDataBit);
    NiFloatsExtraData_SetValue(extra, 0, values[0]);
    NiFloatsExtraData_SetValue(extra, 1, values[1]);
}

// 0x4073B0: +0xB0's flags: not "8 without 4"
static uint8_t __cdecl Flags4Or8Not8Only(uint8_t* object) {
    uint32_t flags = *reinterpret_cast<uint32_t*>(object + 0xB0);
    if (!(flags & 4) && (flags & 8)) {
        return 0;
    }
    return 1;
}

// 0x4073D0: ... not "4 without 8"
static uint8_t __cdecl Flags4Or8Not4Only(uint8_t* object) {
    uint32_t flags = *reinterpret_cast<uint32_t*>(object + 0xB0);
    if (!(flags & 8) && (flags & 4)) {
        return 0;
    }
    return 1;
}

// 0x4073F0: the renderer's vtable[58] (buffer, depth, 0, 0, 0): the group's buffer 0
// (vtable[27]) and the other target's depth buffer (vtable[20])
static void __cdecl SetRenderTarget(void* group, void* depthSource) {
    void* buffer = vc::Call<void*>(group, 0x6C, 0);
    void* depth = vc::Call<void*>(depthSource, 0x50);
    vc::Call<void>(ms_pRenderer, 0xE8, buffer, depth, 0, 0, 0);
}

// 0x407430: the renderer's target state is 1, 2 or 4 and +0x90 is set
static uint8_t __cdecl IsRenderTargetStateOk() {
    return RendererTargetStateOk() ? 1 : 0;
}

// 0x407460: alpha to coverage ('ATOC' on render state 0xB5, 'A2M1' on 0x9A) on, when
// multisampling, or off ('A2M0'): the NVIDIA and ATI driver hacks
static void __cdecl SetAlphaToCoverage(int on) {
    uint8_t* renderer = IsKindOfBit(ms_pRenderer, ms_NiDX9RendererBit);
    uint8_t* settings = *reinterpret_cast<re::Ptr32<uint8_t>*>(reinterpret_cast<uintptr_t>(renderer) + 0x6B4);
    uint32_t atiState;
    if (re::Flag(on)) {
        uint8_t mode[0x18];
        if (*reinterpret_cast<int32_t*>(GetDisplaySettings(mode) + 8) <= 1) {
            return;
        }
        SetCachedRenderState(settings, 0x6C8, 0xB5, 0x434F5441);
        atiState = 0x314D3241;
    } else {
        if (*reinterpret_cast<uint32_t*>(settings + 0x6C8) != 0) {
            void* device = *reinterpret_cast<re::Ptr32<void>*>(settings + 0x2238);
            reinterpret_cast<long(__stdcall*)(void*, uint32_t, uint32_t)>(static_cast<void*>(re::ComVtable(device)[0xE4 / 4]))(
                device, 0xB5, 0);
            *reinterpret_cast<uint32_t*>(settings + 0x6C8) = 0;
        }
        atiState = 0x304D3241;
    }
    SetCachedRenderState(settings, 0x5F0, 0x9A, atiState);
}

// 0x407550: the renderer's 0x755CB0 on the object, when its target state allows
static void __cdecl RenderIfTargetOk(void* object) {
    if (RendererTargetStateOk()) {
        Renderer_Fn755CB0(ms_pRenderer, object);
    }
}

// 0x407580: an .fxl effect compiled (D3DX) and the result written to the .fxb path
static void __cdecl CompileEffect(const char* fxl, const char* fxb, void* include, const void* defines) {
    uint8_t source[0x13400];
    uint32_t length = 0;
    uint8_t* file = static_cast<uint8_t*>(NiFile_GetFile(fxl, 0, 0x8000));
    if (file) {
        uint32_t one = 1;
        auto read = re::PtrCast<uint32_t(__cdecl*)(void*, void*, uint32_t, uint32_t*, int)>(
            *reinterpret_cast<re::Ptr32<void>*>(file + 8));
        length = read(file, source, 0x13400, &one, 1);
        *reinterpret_cast<uint32_t*>(file + 4) += length;
        vc::Call<void>(file, 0x00, 1);   // deleting destructor
    }
    void* compiled = nullptr;
    void* errors = nullptr;
    void* compiler;
    PushMemoryTag(0x3D);
    long hr = D3DXCreateEffectCompiler_(source, length, defines, include, 0, &compiler, &errors);
    if (hr >= 0) {
        // ID3DXEffectCompiler::CompileEffect
        hr = re::PtrCast<long(__stdcall*)(void*, uint32_t, void**, void**)>(re::ComVtable(compiler)[0xEC / 4])(
            compiler, 0, &compiled, &errors);
    }
    PopMemoryTag();
    if (hr >= 0) {
        uint8_t* out = static_cast<uint8_t*>(NiFile_GetFile(fxb, 1, 0x8000));
        if (out && vc::Call<uint8_t>(out, 0x04)) {
            void* const* vtable = re::ComVtable(compiled);
            uint32_t size = re::PtrCast<uint32_t(__stdcall*)(void*)>(vtable[4])(compiled);   // GetBufferSize
            void* data = re::PtrCast<void*(__stdcall*)(void*)>(vtable[3])(compiled);         // GetBufferPointer
            uint32_t one = 1;
            auto write = re::PtrCast<uint32_t(__cdecl*)(void*, const void*, uint32_t, uint32_t*, int)>(
                *reinterpret_cast<re::Ptr32<void>*>(out + 0xC));
            *reinterpret_cast<uint32_t*>(out + 4) += write(out, data, size, &one, 1);
            vc::Call<void>(out, 0x00, 1);
        }
    }
    // (the compiler itself is never released: the game's)
    if (errors) {
        re::PtrCast<unsigned long(__stdcall*)(void*)>(re::ComVtable(errors)[2])(errors);
        errors = nullptr;
    }
    if (compiled) {
        reinterpret_cast<unsigned long(__stdcall*)(void*)>(static_cast<void*>(re::ComVtable(compiled)[2]))(compiled);
    }
}

} // namespace NiExtraDataHelpers

// ---- registration ----------------------------------------------------------------------

#define C00_EXTRA_REVERSED(name, fn, address)                                                       \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace NiExtraDataHelpers {
C00_EXTRA_REVERSED("NiExtraDataHelpers::SetColour", SetColour, 0x00406CF0);
C00_EXTRA_REVERSED("NiExtraDataHelpers::AddFadeExtraData", AddFadeExtraData, 0x00406DF0);
C00_EXTRA_REVERSED("NiExtraDataHelpers::CreateTestTexture", CreateTestTexture, 0x00406E80);
C00_EXTRA_REVERSED("NiExtraDataHelpers::IsDrawable", IsDrawable, 0x00406F40);
C00_EXTRA_REVERSED("NiExtraDataHelpers::ClearDepthBias", ClearDepthBias, 0x00406FB0);
C00_EXTRA_REVERSED("NiExtraDataHelpers::HasNameTag", HasNameTag_, 0x00407050);
C00_EXTRA_REVERSED("NiExtraDataHelpers::SetDepthBias", SetDepthBias, 0x004070C0);
C00_EXTRA_REVERSED("NiExtraDataHelpers::AddFloatsExtraData", AddFloatsExtraData, 0x00407160);
C00_EXTRA_REVERSED("NiExtraDataHelpers::HasTagEither", HasTagEither, 0x00407200);
C00_EXTRA_REVERSED("NiExtraDataHelpers::HasTag528", HasTag528, 0x00407270);
C00_EXTRA_REVERSED("NiExtraDataHelpers::GetRandomDirectionsTexture", GetRandomDirectionsTexture, 0x004072D0);
C00_EXTRA_REVERSED("NiExtraDataHelpers::SetFloatsExtraData", SetFloatsExtraData, 0x00407340);
C00_EXTRA_REVERSED("NiExtraDataHelpers::Flags4Or8Not8Only", Flags4Or8Not8Only, 0x004073B0);
C00_EXTRA_REVERSED("NiExtraDataHelpers::Flags4Or8Not4Only", Flags4Or8Not4Only, 0x004073D0);
C00_EXTRA_REVERSED("NiExtraDataHelpers::SetRenderTarget", SetRenderTarget, 0x004073F0);
C00_EXTRA_REVERSED("NiExtraDataHelpers::IsRenderTargetStateOk", IsRenderTargetStateOk, 0x00407430);
C00_EXTRA_REVERSED("NiExtraDataHelpers::SetAlphaToCoverage", SetAlphaToCoverage, 0x00407460);
C00_EXTRA_REVERSED("NiExtraDataHelpers::RenderIfTargetOk", RenderIfTargetOk, 0x00407550);
C00_EXTRA_REVERSED("CScreenFX::CompileEffect", CompileEffect, 0x00407580);
} // namespace NiExtraDataHelpers
