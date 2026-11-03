// 0x40E930 to 0x4107F0: the light manager (0xBCC124, 0x534 bytes): per area (64 of them,
// 0x14 each from +0x30: a name, a loaded flag, an array of flickering light states) the
// lights of its .gsa file, cached in a lights file (built from the .gsa files in
// development builds, 0xBCBB43); four point lights and a shadow spot light per shadow
// generator, handed out each frame to the nearest lights of the current area.
//
// The manager: +0 debug display, +4 the fade distance (5), +8/+0xC/+0x10 this frame's
// shadow candidates (16 bytes: state, brightness, factor, distance), +0x14/+0x18/+0x1C the
// shadows (0x6C: generator, spot light, 6 planes, ...), +0x20 the point lights (an
// NiTObjectArray<NiPointer<NiPointLight>>), +0x530 the current area.
// A light state (0x24): +0 its range, +4/+8 below / above limits, +0x10 active, +0x11
// cutscene only, +0x12/+0x13 flicker (and its phase), +0x14 the next flicker time, +0x18
// "co_", +0x19 not "_N_", +0x1C its light data (0x88), +0x20 its paired light.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"
#include "runflags.h"

#include "game/Crt.h"
namespace {

GAME_FN(void, __cdecl, DebugPrint, 0x0049A650, const char* format);   // (does nothing)
GAME_FN(void*, __cdecl, CMemoryMgr_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, CMemoryMgr_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
inline void* __cdecl memset_(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
inline char* __cdecl strncpy_(char* dst, const char* src, size_t count) { return crt::Strncpy(dst, src, count); }
inline char* __cdecl strstr_(const char* s, const char* sub) { return crt::Strstr(s, sub); }
template <typename... A>
inline int __cdecl sprintf_(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignCString, 0x00406730, re::Ptr32<char>* self, const char* s);
GAME_FN(uint8_t, __thiscall, FixedString_Contains, 0x00406510, const re::Ptr32<char>* self, const char* s);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* p, uint32_t size, int count, void* dtor);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void*, __cdecl, NiNewPointerArray, 0x00856120, uint32_t count);
GAME_FN(void, __cdecl, NiDeletePointerArray, 0x00836F80, void* p);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00518100, void* self, const re::Ptr32<void>* other);
GAME_FN(void, __thiscall, NiPointer_Release, 0x007549D0, void* self);
GAME_FN(void, __thiscall, NiTArray_SetSize, 0x00856170, void* self, uint32_t size);

// CFileMgr
GAME_VAR(const char*, ms_lightsFileName, 0x009E333C);
GAME_FN(void*, __cdecl, File_OpenForWriting, 0x0042D280, const char* name);
GAME_FN(void*, __cdecl, File_Open, 0x0042D260, const char* name, const char* mode, int);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(int, __cdecl, File_Write, 0x0042D2D0, void* file, const void* buffer, int size);
GAME_FN(void, __cdecl, File_Close, 0x0042D450, void* file);
// NiString (a path)
GAME_FN(void, __thiscall, NiString_Construct, 0x007741A0, void* self, const char* s);
GAME_FN(void, __thiscall, NiString_Fn774430, 0x00774430, void* self);
GAME_FN(void, __thiscall, NiString_Append, 0x00774890, void* self, const re::Ptr32<char>* s);
GAME_FN(const char*, __thiscall, NiString_CStr, 0x00773DB0, void* self);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, void* self);

// the light pieces
GAME_FN(void, __thiscall, Loader_Construct, 0x0040D920, void* self, const char* path);
GAME_FN(void, __thiscall, Loader_Destruct, 0x0040D5F0, void* self);
GAME_FN(uint8_t, __stdcall, InitFlicker, 0x0040DB40, void* state, void* loader, int32_t index);
GAME_FN(void, __thiscall, LightRecord_Construct, 0x0040DFF0, void* self, const void* state);
GAME_FN(void, __thiscall, LightRecord_ToState, 0x0040E930, void* self, void* state);
GAME_FN(void, __thiscall, GrowArray24, 0x0040DF00, void* self, uint32_t capacity, int setCount);
GAME_FN(void, __thiscall, NextFlickerTime, 0x0040DC90, void* state);
GAME_FN(int32_t, __cdecl, GetTimeMs, 0x0045BCA0);
GAME_FN(void, __cdecl, RemoveAllLights, 0x0040E9D0);
GAME_FN(void, __thiscall, PointerArray_RemoveAll, 0x0040E7A0, void* self);
GAME_FN(void, __thiscall, PointerArray_Compact, 0x0040EB20, void* self);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x0040E800, void* self, uint32_t index, const re::Ptr32<uint8_t>* item);
GAME_FN(void, __thiscall, ClearLevels, 0x0040F650, void* self);
GAME_FN(uint8_t*, __thiscall, LightManager_Construct, 0x0040F740, void* self);
GAME_FN(uint8_t*, __cdecl, GetLightManager, 0x0040F7A0);
GAME_FN(void, __thiscall, LoadLights, 0x0040EC60, void* self);
GAME_FN(void, __thiscall, CreateActiveLights, 0x0040F7F0, void* self);
GAME_FN(void, __thiscall, ActivateLight, 0x0040FB30, void* state, float brightness, float factor);
GAME_FN(void, __thiscall, DebugLight, 0x00410320, void* state, int x, int y);
GAME_FN(void, __thiscall, ActivateShadowLights, 0x00410540, void* self);
GAME_FN(void, __thiscall, UpdateShadowFrustums, 0x0040E120, void* self);
GAME_FN(void, __thiscall, UpdateLightBounds, 0x0040E070, void* self);
GAME_FN(void, __thiscall, AttachAffectedNodes, 0x0040E540, void* self, void* node);
GAME_FN(void, __thiscall, AddItem16_x64, 0x0040E8C0, void* self, uint32_t a, uint32_t, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AddItem16(void* a0, uint32_t a1, float a2, float a3, float a4) {
    return AddItem16_x64(a0, a1, re::BitsOf(a2), re::BitsOf(a3), re::BitsOf(a4));
}
GAME_FN(void, __stdcall, SetSpotLight, 0x0040E2D0, uint8_t* light, const void* data);
GAME_FN(void, __stdcall, SetPointLight, 0x0040E450, uint8_t* light, const void* data);
GAME_FN(float*, __stdcall, Blend, 0x0040DD20, float* out, float a0, float a1, float a2, float s, float b0, float b1, float b2,
        float t);
GAME_FN(void, __thiscall, Light_SetAmbient, 0x0040DAB0, void* light, const void* colour);
GAME_FN(void, __thiscall, Light_SetDiffuse, 0x0040DAE0, void* light, const void* colour);
GAME_FN(void, __thiscall, Light_SetSpecular, 0x0040DB10, void* light, const void* colour);
GAME_FN(void, __thiscall, UpdateMaterialValue, 0x0040DDB0, void* self);
GAME_FN(uint8_t, __cdecl, IsAreaLit, 0x00430320, int area);
GAME_FN(re::Ptr32<uint8_t>*, __cdecl, GetLightsNode, 0x0040E8A0, re::Ptr32<uint8_t>* out);
GAME_FN(void, __thiscall, NiDynamicEffect_SetSwitch, 0x00771960, void* self, int on, int);
GAME_FN(void, __thiscall, NiDynamicEffect_DetachAllAffectedNodes, 0x00771A80, void* self);
GAME_FN(void, __thiscall, ShadowGenerator_SetActive, 0x007736C0, void* self, int active, int, int);
GAME_FN(void*, __thiscall, NiPointLight_Construct, 0x007751A0, void* self);
GAME_FN(void*, __thiscall, NiSpotLight_Construct, 0x00774D60, void* self);
GAME_FN(void, __thiscall, NiPlane_Construct, 0x00770FE0, void* self);
GAME_FN(void*, __thiscall, NiShadowGenerator_Construct, 0x00773110, void* self, void* light);
GAME_FN(void*, __cdecl, ShadowTechnique_Find, 0x007571E0, const re::Ptr32<char>* name);
GAME_FN(void, __thiscall, ShadowGenerator_Fn7735F0, 0x007735F0, void* self);
GAME_FN(void, __cdecl, ShadowManager_AddGenerator, 0x007574F0, void* generator);
GAME_FN(uint8_t*, __cdecl, GetDisplaySettings, 0x00405BA0, void* out);   // (returns `out`)
GAME_FN(float*, __thiscall, GetPlayerPosition, 0x004468B0, void* self, float* out);
GAME_FN(float*, __cdecl, VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(uint8_t, __thiscall, Fn_50DF50, 0x0050DF50, void* self);

GAME_VAR(uint8_t*, ms_pLightManager, 0x00BCC124);
GAME_VAR(int32_t, ms_numShadows, 0x00BCC128);
GAME_VAR(int32_t, ms_numLights, 0x00BCC12C);
GAME_VAR(uint8_t, ms_bBuildLightsFile, 0x00BCBB43);
GAME_VAR(uint8_t, ms_bBrightLights_BCC120, 0x00BCC120);
GAME_VAR(uint8_t*, ms_pLights, 0x00BCBBB4);
GAME_VAR(uint8_t*, ms_pLitObjects, 0x00BCBBBC);
GAME_VAR(uint8_t*, ms_pShadowManager, 0x020E2410);
GAME_VAR(uint32_t, ms_NiDynamicEffectBit, 0x020E2C70);
GAME_VAR(int32_t, ms_currArea, 0x00BD1008);
GAME_VAR(void*, ms_pPlayerPed, 0x00C1AEA8);
GAME_VAR(uint8_t, ms_bCutscene, 0x020C5BE4);
GAME_VAR(uint8_t, ms_bResetBounds_20C5BE6, 0x020C5BE6);
GAME_VAR(uint8_t*, ms_pCamera, 0x00C3CCF4);
GAME_ARRAY(float, ms_nightColour, 0x00BB60BC);   // 3 floats
GAME_VAR(uint8_t, ms_clockHour, 0x00BCEB4B);
GAME_VAR(uint8_t, ms_clockMinute, 0x00BCEB4A);
GAME_VAR(uint8_t, ms_clockSeconds_BCEB48, 0x00BCEB48);

constexpr uint32_t kNiTArrayPointLightVtable = 0x0090017C;
constexpr uint32_t kNiTObjectArrayPointLightVtable = 0x00900184;

const char* const kLevelFiles[64] = {
    "tschool.gsa",  "None",         "ischool.gsa",  "None",         "isc_chem.gsa", "isc_offi.gsa", "isc_bio.gsa",
    "isc_nurs.gsa", "isc_boil.gsa", "isc_lib.gsa",  "None",         "None",         "None",         "isc_pool.gsa",
    "isc_dorm.gsa", "isc_clas.gsa", "itrailer.gsa", "isc_art.gsa",  "isc_auto.gsa", "isc_audt.gsa", "ichem.gsa",
    "None",         "None",         "isc_staf.gsa", "isc_atic.gsa", "ifunhous.gsa", "igrocery.gsa", "iboxing.gsa",
    "ifirewrk.gsa", "ibkshop.gsa",  "icomshp.gsa",  "None",         "isc_prep.gsa", "icloth_r.gsa", "icloth_p.gsa",
    "isc_Gdrm.gsa", "itenn.gsa",    "ifunhous.gsa", "iasylum.gsa",  "ibarber.gsa",  "iobserv.gsa",  "None",
    "tgokart.gsa",  "tjyard.gsa",   "None",         "ibaltos.gsa",  "isalon.gsa",   "None",         "None",
    "None",         "isouvnir.gsa", "iMGRaceA.gsa", "iMGRaceB.gsa", "iMGRaceC.gsa", "iware.gsa",    "ifreaks.gsa",
    "ipbarb.gsa",   "iDropS.gsa",   "None",         "iJockS.gsa",   "iPrepS.gsa",   "iGrsrS.gsa",   "tBMX.gsa",
    "isc_lib2.gsa"};

uint32_t& U(void* p, int offset) {
    return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + offset);
}
float& F(void* p, int offset) {
    return *reinterpret_cast<float*>(static_cast<uint8_t*>(p) + offset);
}
uint16_t& W(void* p, int offset) {
    return *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(p) + offset);
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

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

void AddRef(uint8_t* object) {
    if (object) {
        vc::Increment(object + 4);
    }
}

// a light data (0x88) freed with its name
void FreeLightData(uint8_t* data) {
    ReleaseString(reinterpret_cast<re::Ptr32<char>*>(data + 0x84));
    CMemoryMgr_Free(data);
}

uint8_t* Level(uint8_t* manager, int index) {
    return manager + 0x30 + index * 0x14;   // +0 name, +4 loaded, +8 items, +0xC capacity, +0x10 count
}

float SquaredLength(float x, float y, float z) {
    return float((double(x) * x + double(y) * y) + double(z) * z);
}

// the manager, made when there's none
uint8_t* Manager() {
    if (!ms_pLightManager) {
        void* p = CMemoryMgr_Malloc(0x534);
        ms_pLightManager = p ? LightManager_Construct(p) : nullptr;
    }
    return ms_pLightManager;
}

// the paired light's colours blended in at 19:00 (over 30 seconds of the clock), or
// taken outright at night; the dimmer likewise
void ApplyNightPartner(uint8_t* state, uint8_t* light, float* colour, float multiplier) {
    uint8_t* partner = *reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x20);
    if (!state[0x19] || !partner) {
        return;
    }
    uint8_t hour = ms_clockHour;
    if (!(hour >= 0x13 || hour < 7)) {
        return;
    }
    float value;
    if (hour == 0x13 && ms_clockMinute == 0 && ms_clockSeconds_BCEB48 < 0x1E) {
        float t = float(double(int32_t(ms_clockSeconds_BCEB48)) / 30.0);
        float s = float(1.0 - t);
        uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(partner + 0x1C);
        Light_SetAmbient(light, Blend(colour, F(light, 0xFC), F(light, 0x100), F(light, 0x104), s, F(data, 4), F(data, 8),
                                      F(data, 0xC), t));
        data = *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x20) + 0x1C);
        Light_SetDiffuse(light, Blend(colour, F(light, 0x108), F(light, 0x10C), F(light, 0x110), s, F(data, 0x10),
                                      F(data, 0x14), F(data, 0x18), t));
        data = *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x20) + 0x1C);
        Light_SetSpecular(light, Blend(colour, F(light, 0x114), F(light, 0x118), F(light, 0x11C), s, F(data, 0x1C),
                                       F(data, 0x20), F(data, 0x24), t));
        data = *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x20) + 0x1C);
        value = float(double(F(data, 0x68)) * multiplier * t + double(s) * F(light, 0xF8));
    } else {
        uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(partner + 0x1C);
        Light_SetAmbient(light, data + 4);
        Light_SetDiffuse(light, *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x20) + 0x1C) + 0x10);
        Light_SetSpecular(light, *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x20) + 0x1C) + 0x1C);
        data = *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x20) + 0x1C);
        value = float(double(F(data, 0x68)) * multiplier);
    }
    U(light, 0xD0)++;
    F(light, 0xF8) = value;
}

} // namespace

namespace LightManager {

// 0x40E930 (thiscall): a light state from a cached record (its light data rebuilt)
static void __fastcall RecordToState(uint8_t* self, re::Edx, uint8_t* state) {
    for (int i = 0; i < 7; i++) {
        U(state, i * 4) = U(self, i * 4);
    }
    PushMemoryTag(6);
    uint8_t* data = static_cast<uint8_t*>(CMemoryMgr_Malloc(0x88));
    if (data) {
        for (int i = 1; i <= 9; i++) {
            F(data, i * 4) = 0.0f;
        }
        *reinterpret_cast<re::Ptr32<char>*>(data + 0x84) = ms_nullString;
    }
    *reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x1C) = data;
    memset_(data, 0, 0x88);   // (the name handle becomes 0: the game's)
    PopMemoryTag();
    FixedString_AssignCString(reinterpret_cast<re::Ptr32<char>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x1C) + 0x84),
                              reinterpret_cast<const char*>(self + 0x1C));
    data = *reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x1C);
    for (int i = 0; i < 0x21; i++) {
        U(data, i * 4) = U(self, 0x5C + i * 4);
    }
    U(state, 0x20) = 0;
}

// 0x40E9D0: every light taken off the Lights node (switched off, its affected nodes
// dropped); the light count reset. (The index isn't held back after a removal: every
// other one of adjacent lights stays)
static void __cdecl RemoveAllLights_() {
    for (uint32_t i = 0;; i++) {
        uint8_t* lights = ms_pLights;
        AddRef(lights);
        bool more = i < W(lights, 0xCE);
        Release(lights);
        if (!more) {
            break;
        }
        lights = ms_pLights;
        AddRef(lights);
        uint8_t* child = i < W(lights, 0xCE) ? (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(lights + 0xC8))[i] : nullptr;
        Release(lights);
        if (!child) {
            continue;
        }
        uint8_t* rtti = vc::Call<uint8_t*>(child, 0x08);
        if (!(*reinterpret_cast<uint32_t*>(rtti + 0xC + (ms_NiDynamicEffectBit >> 5) * 4) &
              (1u << (ms_NiDynamicEffectBit & 0x1F)))) {
            continue;
        }
        NiDynamicEffect_SetSwitch(child, 0, 0);
        NiDynamicEffect_DetachAllAffectedNodes(child);
        lights = ms_pLights;
        AddRef(lights);
        uint8_t* detached = nullptr;
        vc::Call<void>(lights, 0x90, &detached, i);   // DetachChildAt
        Release(detached);
        Release(lights);
    }
    ms_numLights = 0;
}

// 0x40EB20 (thiscall): NiTArray<NiPointer<...>>::Compact (NiTNewInterface)
static void __fastcall PointerArray_Compact_(uint8_t* self, re::Edx) {
    uint16_t count = W(self, 0xC);
    if (count == W(self, 0xA)) {
        return;
    }
    if (count != 0) {
        uint16_t to = 0;
        for (uint16_t from = 0; from < W(self, 0xA); from++) {
            re::Ptr32<void>* items = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 4);
            void* item = items[from];
            if (item) {
                if (items[to] != item) {
                    NiPointer_Assign(&items[to], &items[from]);
                }
                to++;
            }
        }
    }
    re::Ptr32<void>* old = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 4);
    count = W(self, 0xC);
    W(self, 0xA) = count;
    W(self, 8) = count;
    if (count == 0) {
        *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 4) = nullptr;
        NiDeletePointerArray(old);
        return;
    }
    *reinterpret_cast<re::Ptr32<void>*>(self + 4) = NiNewPointerArray(count);
    for (uint16_t i = 0; i < W(self, 0xA); i++) {
        re::Ptr32<uint8_t>& to = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 4))[i];
        uint8_t* from = static_cast<uint8_t*>(old[i]);
        if (to != from) {
            Release(to);
            to = from;
            AddRef(from);
        }
    }
    NiDeletePointerArray(old);
}

// 0x40EC60 (thiscall): the lights file: built first from the areas' .gsa files when
// 0xBCBB43 is set (and no area is current), then read back into the areas; lights
// marked _N_ paired with their day counterparts; the fade distance 5
static void __fastcall LoadLights_(uint8_t* self, re::Edx) {
    re::Ptr32<char> names[64];
    for (int i = 0; i < 64; i++) {
        names[i] = StringTable_AddString(kLevelFiles[i]);
    }
    if (ms_bBuildLightsFile) {
        if (*reinterpret_cast<int32_t*>(self + 0x530) != -1) {
            DestructArray(names, 4, 64, reinterpret_cast<void*>(0x00517750));
            return;
        }
        void* file = File_OpenForWriting(ms_lightsFileName);
        if (file) {
            int32_t levels = 0;
            for (int i = 0; i < 64; i++) {
                if (!FixedString_Contains(&names[i], "None")) {
                    levels++;
                }
            }
            File_Write(file, &levels, 4);
            for (int32_t i = 0; i < 64; i++) {
                if (FixedString_Contains(&names[i], "None")) {
                    continue;
                }
                File_Write(file, &i, 4);
                uint8_t path[16];
                NiString_Construct(path, "Lights/");
                NiString_Fn774430(path);
                NiString_Append(path, &names[i]);
                char name[64];
                memset_(name, 0, 0x40);
                strncpy_(name, NiString_CStr(path), 0x3F);
                File_Write(file, name, 0x40);
                uint8_t loader[0xA8];
                Loader_Construct(loader, NiString_CStr(path));
                int32_t count = *reinterpret_cast<int32_t*>(loader + 0x70);
                File_Write(file, &count, 4);
                for (int32_t j = 0; j < *reinterpret_cast<int32_t*>(loader + 0x70); j++) {
                    uint8_t state[0x24];
                    if (!InitFlicker(state, loader, j)) {
                        continue;
                    }
                    uint8_t record[0xE0];
                    LightRecord_Construct(record, state);
                    File_Write(file, record, 0xE0);
                    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x1C);
                    if (data) {
                        FreeLightData(data);
                    }
                }
                Loader_Destruct(loader);
                NiString_Destruct(path);
            }
            File_Close(file);
        }
    }
    for (int i = 0; i < 64; i++) {
        Level(self, i)[4] = 0;
    }
    void* file = File_Open(ms_lightsFileName, "r", 1);
    if (file) {
        int32_t levels;
        if (File_Read(file, &levels, 4) == 4) {
            for (int32_t k = 0; k < levels; k++) {
                int32_t index;
                if (File_Read(file, &index, 4) != 4) {
                    break;
                }
                uint8_t* level = Level(self, index);
                level[4] = 1;
                char name[64];
                if (File_Read(file, name, 0x40) != 0x40) {
                    break;
                }
                FixedString_AssignCString(reinterpret_cast<re::Ptr32<char>*>(level), name);
                int32_t count;
                if (File_Read(file, &count, 4) != 4) {
                    break;
                }
                bool failed = false;
                for (int32_t j = 0; j < count; j++) {
                    uint8_t record[0xE0];
                    memset_(record, 0, 0xE0);
                    if (File_Read(file, record, 0xE0) != 0xE0) {
                        failed = true;
                        break;
                    }
                    uint32_t state[9] = {};
                    LightRecord_ToState(record, state);
                    uint8_t* items = level + 8;
                    if (U(items, 8) == U(items, 4)) {
                        GrowArray24(items, U(items, 4) ? U(items, 4) * 2 : 1, 0);
                    }
                    uint32_t* to = reinterpret_cast<uint32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(items) + U(items, 8) * 0x24);
                    for (int w = 0; w < 9; w++) {
                        to[w] = state[w];
                    }
                    U(items, 8)++;
                }
                if (failed) {
                    break;
                }
            }
        }
        File_Close(file);
    }
    for (int i = 0; i < 64; i++) {
        uint8_t* level = Level(self, i);
        if (!level[4]) {
            continue;
        }
        uint32_t count = U(level, 0x10);
        uint8_t* items = *reinterpret_cast<re::Ptr32<uint8_t>*>(level + 8);
        for (uint32_t a = 0; a < U(level, 0x10); a++) {
            uint8_t* night = *reinterpret_cast<re::Ptr32<uint8_t>*>(level + 8) + a * 0x24;
            if (night[0x19]) {
                continue;
            }
            const char* key = strstr_(*reinterpret_cast<re::Ptr32<char>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(night + 0x1C) + 0x84), "_N_") + 3;
            for (uint32_t b = 0; b < U(level, 0x10); b++) {
                uint8_t* day = *reinterpret_cast<re::Ptr32<uint8_t>*>(level + 8) + b * 0x24;
                if (day[0x19] &&
                    FixedString_Contains(re::PtrCast<const re::Ptr32<char>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(day + 0x1C) + 0x84), key)) {
                    *reinterpret_cast<re::Ptr32<uint8_t>*>(night + 0x20) = day;
                    *reinterpret_cast<re::Ptr32<uint8_t>*>(day + 0x20) = night;
                    break;
                }
            }
        }
        (void)count;
        (void)items;
    }
    F(self, 4) = 5.0f;
    *reinterpret_cast<int32_t*>(self + 0x530) = -1;
    self[0] = 0;
    for (int i = 63; i >= 0; i--) {
        ReleaseString(&names[i]);
    }
}

// 0x40F650 (thiscall): the lights off and every area's light states freed
static void __fastcall ClearLevels_(uint8_t* self, re::Edx) {
    RemoveAllLights();
    for (int i = 0; i < 64; i++) {
        uint8_t* level = Level(self, i);
        for (uint32_t j = 0; j < U(level, 0x10); j++) {
            uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(level + 8) + j * 0x24 + 0x1C);
            if (data) {
                FreeLightData(data);
            }
        }
        U(level, 0x10) = 0;
    }
    *reinterpret_cast<int32_t*>(self + 0x530) = -1;
}

// 0x40F710 (thiscall): NiTObjectArray<NiPointer<NiPointLight>>'s deleting destructor
static uint8_t* __fastcall PointLightArray_Destroy(uint8_t* self, re::Edx, int flags) {
    U(self, 0) = kNiTArrayPointLightVtable;
    NiDeletePointerArray(*reinterpret_cast<re::Ptr32<void>*>(self + 4));
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x40F740 (thiscall): the manager's constructor
static uint8_t* __fastcall Construct(uint8_t* self, re::Edx) {
    for (int offset = 8; offset <= 0x1C; offset += 4) {
        U(self, offset) = 0;
    }
    W(self, 0x28) = 0;
    W(self, 0x2E) = 1;
    W(self, 0x2A) = 0;
    W(self, 0x2C) = 0;
    U(self, 0x24) = 0;
    U(self, 0x20) = kNiTObjectArrayPointLightVtable;
    for (int i = 0; i < 64; i++) {
        uint8_t* level = Level(self, i);
        *reinterpret_cast<re::Ptr32<char>*>(level) = ms_nullString;
        U(level, 8) = 0;
        U(level, 0xC) = 0;
        U(level, 0x10) = 0;
    }
    return self;
}

// 0x40F7A0: the light manager (made the first time)
static uint8_t* __cdecl GetLightManager_() {
    if (!ms_pLightManager) {
        void* p = CMemoryMgr_Malloc(0x534);
        if (!p) {
            ms_pLightManager = nullptr;
            return nullptr;
        }
        ms_pLightManager = LightManager_Construct(p);
    }
    return ms_pLightManager;
}

// 0x40F7D0 (thiscall): shutdown: the areas cleared, the point lights released
static void __fastcall Shutdown(uint8_t* self, re::Edx) {
    ClearLevels(self);
    PointerArray_RemoveAll(self + 0x20);
    PointerArray_Compact(self + 0x20);
}

// 0x40F7F0 (thiscall): four point lights, and a shadow (a spot light, six frustum
// planes, and with shaders on a PCF shadow generator) per shadow the shadow manager has
static void __fastcall CreateActiveLights_(uint8_t* self, re::Edx) {
    ms_numLights = 0;
    uint8_t* array = self + 0x20;
    for (int n = 0; n < 4; n++) {
        void* p = NiNew(0x130);
        uint8_t* light = p ? static_cast<uint8_t*>(NiPointLight_Construct(p)) : nullptr;
        re::Ptr32<uint8_t> held = light;
        AddRef(light);
        NiDynamicEffect_SetSwitch(light, 0, 0);
        uint16_t size = W(array, 0xA);
        if (size >= W(array, 8)) {
            NiTArray_SetSize(array, uint32_t(W(array, 0xE)) + size);
        }
        PointerArray_SetAt(array, size, &held);
        Release(light);
    }
    ms_numShadows = 0;
    for (uint32_t i = 0; i < U(ms_pShadowManager, 0xB4); i++) {
        PushMemoryTag(6);
        uint8_t* shadow = static_cast<uint8_t*>(CMemoryMgr_Malloc(0x6C));
        if (shadow) {
            U(shadow, 4) = 0;
            for (int k = 0; k < 6; k++) {
                NiPlane_Construct(shadow + 8 + k * 0x10);
            }
            U(shadow, 0x68) = 0x3F;
        }
        PopMemoryTag();
        void* p = NiNew(0x150);
        uint8_t* spot = p ? static_cast<uint8_t*>(NiSpotLight_Construct(p)) : nullptr;
        re::Ptr32<uint8_t>& held = *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow + 4);
        if (held != spot) {
            Release(held);
            held = spot;
            AddRef(spot);
        }
        NiDynamicEffect_SetSwitch(held, 0, 0);
        uint8_t mode[0x18];
        if (*reinterpret_cast<int32_t*>(GetDisplaySettings(mode) + 0x10) == 0) {
            U(shadow, 0) = 0;
        } else {
            void* q = NiNew(0x68);
            uint8_t* generator = q ? static_cast<uint8_t*>(NiShadowGenerator_Construct(q, held)) : nullptr;
            *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow) = generator;
            W(generator, 8) &= 0xFFFB;
            W(*reinterpret_cast<re::Ptr32<uint8_t>*>(shadow), 0x64) = re::ShadowMapSize();   // (the game's 0x400)
            W(*reinterpret_cast<re::Ptr32<uint8_t>*>(shadow), 8) &= 0xFFDF;
            W(*reinterpret_cast<re::Ptr32<uint8_t>*>(shadow), 8) |= 0x10;
            re::Ptr32<char> name = StringTable_AddString("NiPCFShadowTechnique");
            generator = *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow);
            void* technique = ShadowTechnique_Find(&name);
            *reinterpret_cast<re::Ptr32<void>*>(generator + 0xC) = technique;
            UpdateMaterialValue(generator);   // (inlined in the game, the same)
            ShadowGenerator_Fn7735F0(generator);
            ReleaseString(&name);
            ShadowManager_AddGenerator(*reinterpret_cast<re::Ptr32<void>*>(shadow));
        }
        if (U(self, 0x1C) == U(self, 0x18)) {
            uint32_t capacity = U(self, 0x18) ? U(self, 0x18) * 2 : 1;
            U(self, 0x18) = capacity;
            re::Ptr32<uint8_t>* items = nullptr;
            if (capacity) {
                uint64_t bytes = uint64_t(capacity) * 4;
                items = static_cast<re::Ptr32<uint8_t>*>(CMemoryMgr_Malloc((bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes)));
            }
            re::Ptr32<uint8_t>* old = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 0x14);
            if (old) {
                for (uint32_t k = 0; k < U(self, 0x1C); k++) {
                    items[k] = old[k];
                }
                CMemoryMgr_Free(old);
            }
            *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 0x14) = items;
        }
        (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 0x14))[U(self, 0x1C)] = shadow;
        U(self, 0x1C)++;
    }
}

// 0x40FB30 (thiscall, on a light state): the light switched on: a free shadow spot (for
// type 1) or point light (type 2) set from its data, its dimmer (brightness x factor; a
// quarter while flickering off), the area's night colour, its night partner, a limit
// (without shaders: 0.4 or 1.27 / 0.2 in some areas; 2.5 with 0xBCC120), attached to the
// Lights node
static void __fastcall ActivateLight_(uint8_t* self, re::Edx, float brightness, float factor) {
    float dimmer = brightness * factor;
    float colour[3] = {ms_nightColour[0], ms_nightColour[1], ms_nightColour[2]};
    self[0x10] = 1;
    if (self[0x12] && !self[0x13]) {
        dimmer = float(dimmer * 0.25);
    }
    float limit = 100.0f;
    uint8_t mode[0x18];
    bool clampSpot = *reinterpret_cast<int32_t*>(GetDisplaySettings(mode) + 0x10) == 0;
    bool clampPoint = false;
    if (clampSpot && ms_pPlayerPed) {
        if (ms_currArea == 0xE) {
            limit = 0.4f;
        } else if (ms_currArea == 0x23) {
            limit = 1.27f;
            if (ms_bCutscene) {
                limit = 0.2f;
            }
        } else {
            clampSpot = false;
        }
    }
    if (ms_bBrightLights_BCC120) {
        clampPoint = true;
        limit = 2.5f;
    }
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C);
    uint32_t type = U(data, 0);
    if (type == 0) {
        return;
    }
    if (type == 1) {
        uint8_t* manager = Manager();
        int32_t index = ms_numShadows;
        if (index == int32_t(U(manager, 0x1C))) {
            return;
        }
        uint8_t* shadow = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(GetLightManager() + 0x14))[index];
        ms_numShadows++;
        NiDynamicEffect_SetSwitch(*reinterpret_cast<re::Ptr32<void>*>(shadow + 4), 1, 0);
        uint8_t* spot = *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow + 4);
        AddRef(spot);
        GetLightManager();
        SetSpotLight(spot, *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C));
        uint8_t* generator = *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow);
        if (generator) {
            data = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C);
            W(generator, 8) = data[0x79] ? uint16_t(W(generator, 8) | 4) : uint16_t(W(generator, 8) & 0xFFFB);
            // (the light's own size, 0x400 from the loader: scaled when the shadow maps are
            // bigger than the game's, runflags.h)
            uint16_t size = W(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C), 0x7A);
            if (re::ShadowMapSize() != 0x400) {
                size = uint16_t(std::min<uint32_t>(uint32_t(size) * re::ShadowMapSize() / 0x400, 0x800));
            }
            W(*reinterpret_cast<re::Ptr32<uint8_t>*>(shadow), 0x64) = size;
            generator = *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow);
            data = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C);
            W(generator, 8) = data[0x7C] ? uint16_t(W(generator, 8) | 0x20) : uint16_t(W(generator, 8) & 0xFFDF);
            W(generator, 8) = data[0x7D] ? uint16_t(W(generator, 8) | 0x10) : uint16_t(W(generator, 8) & 0xFFEF);
            if (data[0x7E]) {
                UpdateMaterialValue(generator);
            } else {
                F(generator, 0x54) = F(data, 0x80);
            }
            ShadowGenerator_SetActive(generator, (*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C))[0x78], 0, 0);
            W(*reinterpret_cast<re::Ptr32<uint8_t>*>(shadow), 8) |= 0x40;
            W(*reinterpret_cast<re::Ptr32<uint8_t>*>(shadow), 8) |= 0x80;
        }
        uint8_t* light = *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow + 4);
        U(light, 0xD0)++;
        F(light, 0xF8) = dimmer;
        if (IsAreaLit(ms_currArea) && !self[0x18]) {
            Light_SetDiffuse(light, colour);
        }
        ApplyNightPartner(self, *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow + 4), colour, factor);
        light = *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow + 4);
        float value = F(light, 0xF8);
        if (clampSpot && limit < value) {
            value = limit;
        }
        U(light, 0xD0)++;
        F(light, 0xF8) = value;
        uint8_t* lights = ms_pLights;
        AddRef(lights);
        vc::Call<void>(lights, 0x88, light, 1);   // AttachChild
        Release(lights);
        return;
    }
    if (type != 2 || ms_numLights == 4) {
        return;
    }
    uint8_t* manager = GetLightManager();
    uint8_t* light = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(manager + 0x24))[ms_numLights];
    uint8_t* held = light;
    AddRef(light);
    ms_numLights++;
    NiDynamicEffect_SetSwitch(light, 1, 0);
    AddRef(light);
    GetLightManager();
    SetPointLight(light, *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C));
    U(light, 0xD0)++;
    F(light, 0xF8) = dimmer;
    if (IsAreaLit(ms_currArea) && !self[0x18]) {
        Light_SetDiffuse(light, colour);
    }
    ApplyNightPartner(self, light, colour, factor);
    float value = F(light, 0xF8);
    if (clampPoint && limit < value) {   // (the point lights check 0xBCC120's flag: the game's)
        value = limit;
    }
    U(light, 0xD0)++;
    F(light, 0xF8) = value;
    re::Ptr32<uint8_t> node = nullptr;
    GetLightsNode(&node);
    vc::Call<void>(node, 0x88, light, 1);
    NiPointer_Release(&node);
    NiPointer_Release(&held);
}

// 0x410320 (thiscall, on a light state): the debug line of a light (formatted, not shown)
static void __fastcall DebugLight_(uint8_t* self, re::Edx, int, int) {
    float position[3];
    GetPlayerPosition(reinterpret_cast<void*>(0x00C1AEA8), position);
    float d[3];
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C);
    VectorSub(d, position, reinterpret_cast<float*>(data + 0x4C));
    float distance = float(std::sqrt(double(std::fabs(SquaredLength(d[0], d[1], d[2])))));
    char label[200];
    char* name = *reinterpret_cast<re::Ptr32<char>*>(data + 0x84);
    if (name == ms_nullString || *reinterpret_cast<uint16_t*>(name - 4) <= 0x11) {
        strcpy(label, name);
    } else if (U(data, 0) == 1) {
        sprintf_(label, "Shadow");
    } else {
        sprintf_(label, "Diffuse");
    }
    char text[564];
    uint8_t* manager = Manager();
    sprintf_(text, self[0x10] ? "%s ON at distance %.2f/%.2f - %.2f" : "%s OFF at distance %.2f/%.2f - %.2f", label,
             double(distance), double(F(self, 0)), double(F(manager, 4) + F(self, 0)));
}

// 0x4104C0 (thiscall): the manager started: the lights loaded, the active lights made.
// (The name comes by value and is released)
static void __fastcall Initialise(uint8_t* self, re::Edx, void*, re::Ptr32<char> name) {
    *reinterpret_cast<int32_t*>(self + 0x530) = -1;
    DebugPrint("InitGSA");
    LoadLights(self);
    DebugPrint(nullptr);
    DebugPrint("CreateActiveLights");
    CreateActiveLights(self);
    DebugPrint(nullptr);
    ReleaseString(&name);
}

// 0x410540 (thiscall): this frame's shadow candidates (at most 8) sorted by distance and
// switched on nearest first
static void __fastcall ActivateShadowLights_(uint8_t* self, re::Edx) {
    uint32_t count = U(self, 0x10);
    uint8_t* items[8];
    for (uint32_t i = 0; i < count; i++) {
        items[i] = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 8) + i * 0x10;
    }
    // insertion sort on +0xC (the game unrolls it four at a time)
    for (uint32_t i = 1; i < count; i++) {
        float key = F(items[i], 0xC);
        for (int32_t j = int32_t(i) - 1; j >= 0 && key < F(items[j], 0xC); j--) {
            uint8_t* t = items[j];
            items[j] = items[j + 1];
            items[j + 1] = t;
        }
    }
    for (uint32_t i = 0; i < U(self, 0x10); i++) {
        ActivateLight(*reinterpret_cast<re::Ptr32<void>*>(items[i]), F(items[i], 4), F(items[i], 8));
    }
}

// 0x410670 (thiscall): the debug display of the current area's lights (formatted, not
// shown). Its body continues at 0x410680
static void __fastcall DebugDisplay(uint8_t* self, re::Edx) {
    if (!Manager()[0]) {
        return;
    }
    uint8_t* lights = ms_pLights;
    AddRef(lights);
    char text[0x1F4];
    int32_t y = 0;
    int32_t on = 0;
    int32_t total = 0;
    for (uint32_t i = 0; i < U(Level(self, *reinterpret_cast<int32_t*>(self + 0x530)), 0x10); i++) {
        uint8_t* state = *reinterpret_cast<re::Ptr32<uint8_t>*>(Level(self, *reinterpret_cast<int32_t*>(self + 0x530)) + 8) + i * 0x24;
        if (U(*reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x1C), 0) == 1) {
            continue;
        }
        if (state[0x10]) {
            on++;
            DebugLight(state, 0, y + 10);
            y += 0x14;
        }
        total++;
    }
    sprintf_(text, "DIFFUSE LIGHT COUNT: %d/%d", on, total);
    y += 0x28;
    on = 0;
    total = 0;
    for (uint32_t i = 0; i < U(Level(self, *reinterpret_cast<int32_t*>(self + 0x530)), 0x10); i++) {
        uint8_t* state = *reinterpret_cast<re::Ptr32<uint8_t>*>(Level(self, *reinterpret_cast<int32_t*>(self + 0x530)) + 8) + i * 0x24;
        if (U(*reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x1C), 0) != 1) {
            continue;
        }
        if (state[0x10]) {
            DebugLight(state, 0, y);
            y += 0x14;
            on++;
        }
        total++;
    }
    sprintf_(text, "SHADOW LIGHT COUNT: %d/%d", on, total);
    Release(lights);
}
// 0x410680: the middle of 0x410670 (Ghidra starts a function there): not an entry point,
// and a hook there would break 0x410670's fall-through
RE_UNHOOKED_FN("LightManager::DebugDisplay_body", 0x00410680, &DebugDisplay);

// 0x4107F0 (thiscall): the frame's update: shadows off, then for each light of the
// current area (when it's loaded) its flicker advanced and, unless cutscene-only, its
// brightness from the distance to the player (or camera): full inside its range, fading
// over the fade distance, with height limits above and below; shadow lights become
// candidates, the others are switched on. Then the nearest shadows, the frustums, the
// bounds and the lit objects' affecting lights
static void __fastcall Update(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < U(self, 0x1C); i++) {
        void* generator = *(*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x14) + i * 4));
        if (generator) {
            ShadowGenerator_SetActive(generator, 0, 1, 0);
        }
    }
    int32_t area = ms_currArea;
    ms_numShadows = 0;
    *reinterpret_cast<int32_t*>(self + 0x530) = area;
    if (!Level(self, area)[4]) {
        return;
    }
    float position[3] = {0.0f, 0.0f, 0.0f};
    if (!ms_bResetBounds_20C5BE6 && Fn_50DF50(reinterpret_cast<void*>(0x00A45A30))) {
        float temp[3];
        const float* p = GetPlayerPosition(reinterpret_cast<void*>(0x00C1AEA8), temp);
        if (const float* from = re::LightsFrom()) {
            p = from;   // (the remaster's seamless doors: runflags.h)
        }
        position[0] = p[0];
        position[1] = p[1];
        position[2] = p[2];
    } else {
        position[0] = F(ms_pCamera, 0x50);
        position[1] = F(ms_pCamera, 0x54);
        position[2] = F(ms_pCamera, 0x58);
    }
    re::Ptr32<void>& candidates = *reinterpret_cast<re::Ptr32<void>*>(self + 8);
    if (candidates) {
        CMemoryMgr_Free(candidates);
        candidates = nullptr;
    }
    U(self, 0xC) = 0;
    U(self, 0x10) = 0;
    for (uint32_t i = 0; i < U(Level(self, *reinterpret_cast<int32_t*>(self + 0x530)), 0x10); i++) {
        uint8_t* state = *reinterpret_cast<re::Ptr32<uint8_t>*>(Level(self, *reinterpret_cast<int32_t*>(self + 0x530)) + 8) + i * 0x24;
        if (!state[0x19]) {
            continue;
        }
        if (state[0x12] && uint32_t(GetTimeMs()) > U(state, 0x14)) {
            state[0x13] = !state[0x13];
            NextFlickerTime(state);
            if (!state[0x13]) {
                state[0x10] = 0;
            }
        }
        state[0x10] = 0;
        if (state[0x11] && !ms_bCutscene) {
            continue;
        }
        uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(state + 0x1C);
        float d[3];
        VectorSub(d, position, reinterpret_cast<float*>(data + 0x4C));
        float distance = float(std::sqrt(double(std::fabs(SquaredLength(d[0], d[1], d[2])))));
        float range = F(state, 0);
        float fade = F(self, 4);
        float brightness = 0.0f;   // (set on every path that leaves `on`)
        float factor = 0.0f;
        bool on = true;
        if (range == 1.0f && !(1.0f < F(state, 4))) {
            brightness = F(data, 0x68);
            factor = 1.0f;
        } else {
            float height = d[2];
            float limit = 0.0f;
            if (0.0f < height || 0.0f == height || height != height) {
                if (0.01f < F(state, 8)) {
                    limit = F(state, 8);
                }
            } else {
                height = float(height * -1.0);
                limit = F(state, 4);
            }
            bool fading = range < distance;
            if (!fading) {
                if (!(1.0f < F(state, 4)) || !(limit < height)) {
                    brightness = F(data, 0x68);
                    factor = 1.0f;
                } else {
                    fading = true;
                }
            }
            if (fading) {
                if (1.0f < F(state, 4)) {
                    float span = fade;
                    if (limit < span) {
                        span = limit;
                    }
                    if (span + limit < height) {
                        on = false;
                    } else {
                        float byHeight = float(1.0 - (height - limit) / span);
                        float byDistance = float(1.0 - (distance - range) / fade);
                        float result = byDistance;
                        if (byHeight < byDistance) {
                            result = byHeight;
                        }
                        if (!(0.0f < result)) {
                            on = false;
                        } else {
                            brightness = F(data, 0x68);
                            factor = result;
                        }
                    }
                } else if (range + fade < distance) {
                    on = false;
                } else {
                    brightness = F(data, 0x68);
                    factor = float(1.0 - (distance - range) / fade);
                }
            }
        }
        if (!on) {
            continue;
        }
        if (U(data, 0) == 1) {
            AddItem16(self, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(state)), brightness, factor, distance);
        } else {
            ActivateLight(state, brightness, factor);
        }
    }
    ActivateShadowLights(self);
    UpdateShadowFrustums(self);
    UpdateLightBounds(self);
    uint8_t* lit = ms_pLitObjects;
    AddRef(lit);
    AttachAffectedNodes(self, lit);
    Release(lit);
}

} // namespace LightManager

// ---- registration ----------------------------------------------------------------------

#define C00_LM_REVERSED(name, fn, address)                                                          \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace LightManager {
C00_LM_REVERSED("LightRecord::ToState", RecordToState, 0x0040E930);
C00_LM_REVERSED("LightManager::RemoveAllLights", RemoveAllLights_, 0x0040E9D0);
C00_LM_REVERSED("NiTArray<NiPointer>::CompactNew", PointerArray_Compact_, 0x0040EB20);
C00_LM_REVERSED("LightManager::LoadLights", LoadLights_, 0x0040EC60);
C00_LM_REVERSED("LightManager::ClearLevels", ClearLevels_, 0x0040F650);
C00_LM_REVERSED("NiTObjectArray<NiPointLight>::Destroy", PointLightArray_Destroy, 0x0040F710);
C00_LM_REVERSED("LightManager::Construct", Construct, 0x0040F740);
C00_LM_REVERSED("LightManager::Get", GetLightManager_, 0x0040F7A0);
C00_LM_REVERSED("LightManager::Shutdown", Shutdown, 0x0040F7D0);
C00_LM_REVERSED("LightManager::CreateActiveLights", CreateActiveLights_, 0x0040F7F0);
C00_LM_REVERSED("LightManager::ActivateLight", ActivateLight_, 0x0040FB30);
C00_LM_REVERSED("LightManager::DebugLight", DebugLight_, 0x00410320);
C00_LM_REVERSED("LightManager::Initialise", Initialise, 0x004104C0);
C00_LM_REVERSED("LightManager::ActivateShadowLights", ActivateShadowLights_, 0x00410540);
C00_LM_REVERSED("LightManager::DebugDisplay", DebugDisplay, 0x00410670);
C00_LM_REVERSED("LightManager::Update", Update, 0x004107F0);
} // namespace LightManager
