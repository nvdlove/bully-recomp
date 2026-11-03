#include "reversed.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
// The sky (GTA's CClouds, reworked): the sky domes (NIFs from \MODELS\SKYDOME\), the
// moon (Config\hdt\moon.hdt: its size, phase shadow, colour and position), the stars
// and the north star, and the background gradient split at the horizon line (tilted
// with the camera's roll).
//
// The x87 code keeps sums in extended precision between float stores; the float
// values here are computed in double and rounded where the game stores them.

#define CLOUDS_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

struct RGBA {
    uint8_t r, g, b, a;
};
static_assert(sizeof(RGBA) == 4);

struct Vec4 {
    float x, y, z, w;
};
static_assert(sizeof(Vec4) == 0x10);

GAME_FN(uint8_t, __cdecl, Area_Fn430320, 0x00430320, int area);   // (Game.cpp's CGame::IsAreaInterior)
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int32_t* value);
GAME_FN(int, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);   // NiTransform (rotate[9], translate)
GAME_FN(uint8_t, __cdecl, CalcScreenCoors, 0x0052AEE0, const float* pos, Vec4* out, float* w, float* h, int, int,
        int, int);
GAME_FN(void, __cdecl, RenderOneXLUSprite, 0x0052B690, float x, float y, float z, float w, float h, int r, int g,
        int b, int intensity, int recipZ);
GAME_FN(float, __cdecl, GetHorizonY, 0x0052C130);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const float* rect, const RGBA* colour);
GAME_FN(void, __cdecl, DrawGradient, 0x0052C5D0, float x0, float y0, float x1, float y1, float x2, float y2,
        float x3, float y3, const RGBA* c0, const RGBA* c1, const RGBA* c2, const RGBA* c3);
GAME_FN(RGBA*, __thiscall, RGBA_Construct, 0x00510DE0, RGBA* self, int r, int g, int b, int a);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(char*, __cdecl, GetFullPath, 0x00829900, const char* path);
GAME_FN(void, __thiscall, NiStream_Construct, 0x0075CE40, void* stream);
GAME_FN(uint8_t, __thiscall, NiStream_Load, 0x00759E10, void* stream, const char* path);
GAME_FN(void, __thiscall, NiStream_Destruct, 0x0075C020, void* stream);
GAME_FN(void, __cdecl, Fn_40AD10, 0x0040AD10, void*, uintptr_t, int, int);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E040, void* object);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E240, void* object);
GAME_FN(float, __cdecl, Clouds_GetMoonPhase, 0x0050EF20);
GAME_FN(void*, __cdecl, DataReader_Open, 0x005B07E0, const char* path, int32_t* out, int);
GAME_FN(void, __cdecl, DataReader_Close, 0x005B0860, void* handle);

struct NiTransform {
    float rotate[9];     // +0
    float translate[3];  // +0x24
    float scale;         // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

GAME_FN(void*, __thiscall, NiObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<char>* name);
GAME_FN(uint8_t, __thiscall, NiObjectNET_AddExtraData, 0x00764600, void* object, const re::Ptr32<char>* name, void* data);
GAME_FN(void, __thiscall, NiObjectNET_Fn762080, 0x00762080, void* object, void* name, int);
GAME_FN(void*, __thiscall, NiColorExtraData_Construct, 0x00769260, void* self, float r, float g, float b, float a);
GAME_FN(void, __thiscall, NiColorExtraData_SetValue, 0x007691F0, void* self, float r, float g, float b, float a);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rtti, void* object);
GAME_FN(void*, __thiscall, NiAVObject_GetProperty, 0x0075DC10, void* object, int type);
GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, NiTransform* self);
GAME_FN(NiTransform*, __thiscall, NiTransform_Multiply, 0x00406830, const NiTransform* self, NiTransform* out,
        const NiTransform* other);
GAME_FN(void, __cdecl, RotateAboutAxis, 0x00415EB0, NiTransform* t, const float* axis, float angle, int mode);
GAME_FN(void, __cdecl, Fn_407550, 0x00407550, void* camera);
GAME_FN(void, __cdecl, Clouds_SetGeometryColour, 0x00510220, void* geometry, int flag, uint32_t colour);
GAME_FN(void, __cdecl, Clouds_ColourSkyDome, 0x005103D0, void* object, uint32_t colour);
GAME_FN(void, __cdecl, Clouds_LoadMoonSettingsFn, 0x0050F160);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);

GAME_VAR(uint32_t, niGeometryClassIndex, 0x020E25BC);     // (the RTTI class-set bits)
GAME_VAR(uint32_t, niColorExtraDataClassIndex, 0x020E29E4);
inline void* const rttiPropertyC = reinterpret_cast<void*>(0x020E28BC);   // (property 8's class)
inline void* const rttiNiNode = reinterpret_cast<void*>(0x020E2548);
inline re::Ptr32<char>* const domeColourName = reinterpret_cast<re::Ptr32<char>*>(0x00BCC4AC);  // the colour extra data's name
inline void* const name_BCC4A0 = reinterpret_cast<void*>(0x00BCC4A0);
inline void* const name_BCC4A4 = reinterpret_cast<void*>(0x00BCC4A4);
GAME_VAR(float, skyDomeRotation, 0x00C6603C);
GAME_VAR(float, g_20C4828, 0x020C4828);           // (the rotation speed)
GAME_VAR(float, cloudDomeDrift, 0x00A55178);      // -180..180
GAME_ARRAY(uint8_t, skyDomeColours, 0x020C482C);  // two RGB triplets (the domes 0 and 1)

// an NiObject's RTTI class set has `index` (NiDynamicCast)
bool IsKindOfIndex(uint8_t* object, uint32_t index) {
    using RttiFn = uint8_t*(__thiscall*)(void*);
    uint8_t* rtti = re::PtrCast<RttiFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[2])(object);
    uint32_t bits = reinterpret_cast<uint32_t*>(rtti + 0xC)[index >> 5];
    return (bits & (1u << (index & 0x1F))) != 0;
}

using InterlockedFn = long(__stdcall*)(long*);
GAME_VAR(InterlockedFn, InterlockedDecrement_, 0x008FC170);
GAME_VAR(InterlockedFn, InterlockedIncrement_, 0x008FC174);

GAME_VAR(int32_t, currArea, 0x00BD1008);          // CGame::currArea
GAME_VAR(void*, theNiCamera, 0x00C3CCF4);         // TheCamera.camera
GAME_VAR(uint16_t, clockSeconds, 0x00BCEB48);
GAME_VAR(uint8_t, clockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);
GAME_VAR(int32_t, clockDays, 0x00BCEB4C);
GAME_VAR(float, weatherCE755C, 0x00CE755C);       // fog and cloud cover (the larger dims the moon and stars)
GAME_VAR(float, weatherCE7560, 0x00CE7560);
GAME_VAR(uint32_t, niNodeClassIndex, 0x020E251C); // NiNode's bit in the RTTI class sets
GAME_VAR(uint8_t*, theRenderer, 0x020E2380);      // +0x6B4: its D3D state

// the moon (moon.hdt)
GAME_VAR(float, moonBlue, 0x00C66010);
GAME_VAR(float, moonGreen, 0x00C66014);
GAME_VAR(float, moonRed, 0x00C66018);
GAME_VAR(int32_t, moonCycleDays, 0x00C6601C);
GAME_VAR(float, moonShadowRight, 0x00C66020);
GAME_VAR(float, moonShadowLeft, 0x00C66024);
GAME_VAR(float, moonShadowSize, 0x00C66028);
GAME_VAR(float, moonSize, 0x00C6602C);
GAME_VAR(void*, moonMaskTexture, 0x00C66030);     // "MoonMask" from the particle txd (the phase shadow)
GAME_VAR(float, horizonTilt, 0x00C66034);         // half the horizon's rise across the screen (px)
GAME_VAR(float, horizonY, 0x00C66038);
GAME_VAR(int32_t, numSkyDomes, 0x00C66040);
GAME_VAR(RGBA, skyColourC66044, 0x00C66044);
GAME_VAR(RGBA, skyColourC66048, 0x00C66048);
GAME_VAR(Vec4, moonPos, 0x00C6604C);
GAME_VAR(Vec4, northStarPos, 0x00C6605C);
GAME_ARRAY(uint8_t*, skyDomes, 0x00C6606C);       // NiNode* (NiPointers)
GAME_VAR(void*, starTexture, 0x00C66080);
GAME_VAR(void*, moonTexture, 0x00C66088);
// the stars (offsets from the camera)
constexpr int kNumStars = 200;
GAME_ARRAY(float, starSizes, 0x00C65390);
GAME_ARRAY(float, starZ, 0x00C656B0);
GAME_ARRAY(float, starY, 0x00C659D0);
GAME_ARRAY(float, starX, 0x00C65CF0);

float* CameraTransform() {
    return GetLocalTransform(GetCameraNode(theNiCamera));
}

// the brighter of fog and cloud cover (the first on a tie)
float Coverage() {
    float a = weatherCE755C;
    float b = weatherCE7560;
    return a < b ? b : a;
}

void ReleaseRef(uint8_t* object) {
    if (InterlockedDecrement_(reinterpret_cast<long*>(object + 4)) == 0) {
        using DeleteFn = void(__thiscall*)(void*);
        re::PtrCast<DeleteFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[1])(object);
    }
}

} // namespace

// 0x50EF20: the moon shadow's offset for today (from left to right over the cycle)
float __cdecl Clouds_GetMoonPhaseOurs() {
    float offset = 0.0f;
    if (moonCycleDays > 0) {
        float t = float(double(clockDays % moonCycleDays) / double(moonCycleDays));
        offset = float((double(moonShadowRight) - moonShadowLeft) * t + moonShadowLeft);
    }
    return offset;
}
CLOUDS_REVERSED("CClouds::GetMoonPhase_50EF20", 0x0050EF20, &Clouds_GetMoonPhaseOurs, nullptr, nullptr, re::Mode::Original);

// 0x50EF70: loads a sky dome (\MODELS\SKYDOME\<name>.nif) into the next slot
uint8_t __cdecl Clouds_LoadSkyDome(const char* name) {
    char path[0x108];
    strcpy(path, GetFullPath("\\MODELS\\SKYDOME\\"));
    strcat(path, name);
    strcat(path, ".nif");
    alignas(4) uint8_t stream[0x40C];
    NiStream_Construct(stream);
    if (NiStream_Load(stream, path)) {
        // the first top object, when it's an NiNode
        uint8_t* object = **reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(stream + 0x15C);   // (the top objects)
        uint8_t* node = nullptr;
        if (object != nullptr) {
            node = IsKindOfIndex(object, niNodeClassIndex) ? object : nullptr;
        }
        re::Ptr32<uint8_t>* slot = &skyDomes[numSkyDomes];
        uint8_t* old = *slot;
        if (old != node) {
            if (old != nullptr) {
                ReleaseRef(old);
            }
            *slot = node;
            if (node != nullptr) {
                InterlockedIncrement_(reinterpret_cast<long*>(node + 4));
            }
        }
        if (skyDomes[numSkyDomes] != nullptr) {
            Fn_40AD10(skyDomes[numSkyDomes], 0, 1, 0);
            AVObject_Update(skyDomes[numSkyDomes], 0.0f, 1);
            AVObject_UpdateEffects(skyDomes[numSkyDomes]);
            AVObject_UpdateProperties(skyDomes[numSkyDomes]);
            numSkyDomes = numSkyDomes + 1;
        }
    }
    NiStream_Destruct(stream);
    return 0;
}
CLOUDS_REVERSED("CClouds::LoadSkyDome_50EF70", 0x0050EF70, &Clouds_LoadSkyDome, nullptr, nullptr, re::Mode::Original);

// 0x50F110: releases the sky domes (the count stays)
void __cdecl Clouds_ReleaseSkyDomes() {
    for (int i = 0; i < numSkyDomes; i++) {
        if (uint8_t* dome = skyDomes[i]) {
            ReleaseRef(dome);
            skyDomes[i] = nullptr;
        }
    }
}
CLOUDS_REVERSED("CClouds::ReleaseSkyDomes_50F110", 0x0050F110, &Clouds_ReleaseSkyDomes, nullptr, nullptr, re::Mode::Original);

// 0x50F160: reads Config\hdt\moon.hdt (the positions' w is whatever was on the
// stack: the game copies 16 bytes of a 12-byte vector)
void __cdecl Clouds_LoadMoonSettings() {
    int32_t unused = 0;
    uint8_t* reader = static_cast<uint8_t*>(DataReader_Open("Config\\hdt\\moon.hdt", &unused, 0));
    if (reader == nullptr) {
        return;
    }
    using GetFloatFn = void(__thiscall*)(void*, float*, const char*);
    using GetIntFn = void(__thiscall*)(void*, int32_t*, const char*);
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reader);
    auto getFloat = [&](float* out, const char* key) {
        re::PtrCast<GetFloatFn>(vtable[0x28 / 4])(reader, out, key);
        vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reader);
    };
    getFloat(&moonSize, "Moon Size");
    getFloat(&moonShadowSize, "Shadow Size");
    getFloat(&moonShadowLeft, "Shadow Left");
    getFloat(&moonShadowRight, "Shadow Right");
    re::PtrCast<GetIntFn>(vtable[0x38 / 4])(reader, &moonCycleDays, "Moon Cycle Days");
    vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reader);
    getFloat(&moonRed, "Red Value");
    getFloat(&moonGreen, "Green Value");
    getFloat(&moonBlue, "Blue Value");
    Vec4 pos;   // (w: uninitialised, as in the game)
    getFloat(&pos.x, "Moon Pos X");
    getFloat(&pos.y, "Moon Pos Y");
    getFloat(&pos.z, "Moon Pos Z");
    moonPos = pos;
    getFloat(&pos.x, "North Star Pos X");
    getFloat(&pos.y, "North Star Pos Y");
    getFloat(&pos.z, "North Star Pos Z");
    northStarPos = pos;
    DataReader_Close(reader);
}
CLOUDS_REVERSED("CClouds::LoadMoonSettings_50F160", 0x0050F160, &Clouds_LoadMoonSettings, nullptr, nullptr, re::Mode::Original);

// 0x50F320: the moon (a black disc over the stars, the phase shadow, the lit moon)
// and at night the stars and the north star; only outside (or in area 0x20)
void __cdecl Clouds_RenderMoonAndStars() {
    if (!Area_Fn430320(currArea) && currArea != 0x20) {
        return;
    }
    SetRenderState(8, 0);
    SetRenderState(6, 0);
    SetRenderState(0xC, 1);
    SetRenderState(10, 2);
    SetRenderState(0xB, 2);
    const float* transform = CameraTransform();
    float camX = transform[9];
    float camY = transform[10];
    uint8_t hour = clockHours;
    float camZ = transform[11];

    // the moon: up from 21:00 to 03:00, brightest at 03:00 (minute 180)
    float minute = float(double(hour * 60 + clockMinutes) + double(clockSeconds) / 60.0);
    float fromPeak = std::fabs(float(double(minute) - 180.0));
    if (fromPeak < 180.0f) {
        float base = float(180.0 - fromPeak);
        float brightness = float((1.0 - Coverage()) * base);
        if (0.0f < brightness) {
            float world[3];
            world[0] = camX + moonPos.x;
            world[1] = camY + moonPos.y;
            world[2] = camZ + moonPos.z;
            Vec4 screen;
            float w, h;
            if (CalcScreenCoors(world, &screen, &w, &h, 0, 0, 0, 1)) {
                float phase = Clouds_GetMoonPhase();
                SetRenderState(1, 0);
                SetRenderState(10, 5);
                SetRenderState(0xB, 2);
                RenderOneXLUSprite(screen.x, screen.y, screen.z, float(double(moonSize) * w),
                                   float(double(moonSize) * h), 0, 0, 0, 0xFF, 0xFF);
                SetRenderState(1, GetRaster(moonMaskTexture));
                SetRenderState(10, 5);
                SetRenderState(0xB, 2);
                double shadowW = double(moonShadowSize) * w;
                float shadowX = float(double(float(shadowW * phase)) + screen.x);
                RenderOneXLUSprite(shadowX, screen.y, screen.z, float(shadowW), float(double(moonShadowSize) * h), 0, 0,
                                   0, 0, 0xFF);
                SetRenderState(10, 7);
                SetRenderState(0xB, 2);
                SetRenderState(1, GetRaster(moonTexture));
                int recipZ = int(1.0 / screen.z) & 0xFF;
                int b = int(double(moonBlue) * brightness) & 0xFF;
                int g = int(double(moonGreen) * brightness) & 0xFF;
                int r = int(double(brightness) * moonRed) & 0xFF;
                RenderOneXLUSprite(screen.x, screen.y, screen.z, float(double(moonSize) * w), float(double(moonSize) * h),
                                   r, g, b, 0xFF, recipZ);
                SetRenderState(10, 2);
                SetRenderState(0xB, 2);
            }
        }
    }

    // the stars: from 20:00 to 06:00 (fading in over 20:00 and out over 05:00)
    if (uint8_t(hour - 6) <= 0xD) {
        goto done;
    }
    {
        int starBrightness;
        if (uint8_t(hour - 5) <= 0xF) {
            int minutes = hour == 20 ? clockMinutes : 60 - clockMinutes;
            starBrightness = minutes * 255 / 60;
            if (starBrightness == 0) {
                goto done;
            }
        } else {
            starBrightness = 0xFF;
        }
        starBrightness = int((1.0 - Coverage()) * starBrightness);
        SetRenderState(1, GetRaster(starTexture));
        for (int i = 0; i < kNumStars; i++) {
            float pos[3] = {0.0f, 0.0f, 0.0f};
            const float* t = CameraTransform();
            pos[0] = pos[0] + t[9];
            pos[1] = t[10] + pos[1];
            pos[2] = t[11] + pos[2];
            pos[0] = starX[i] + pos[0];
            pos[1] = starY[i] + pos[1];
            pos[2] = starZ[i] + pos[2];
            Vec4 screen;
            float w, h;
            if (CalcScreenCoors(pos, &screen, &w, &h, 0, 0, 0, 1)) {
                int random = Rand() & 0x1F;
                int c = int((1.0 - random * 0.014999999664723873) * starBrightness);
                double size = starSizes[i] * 0.800000011920929;
                int recipZ = int(1.0 / screen.z) & 0xFF;
                RenderOneXLUSprite(screen.x, screen.y, screen.z, float(w * size), float(h * size), c, c, c, 0xFF,
                                   recipZ);
            }
        }
        SetRenderState(1, GetRaster(starTexture));
        float pos[3];
        pos[0] = camX + northStarPos.x;
        pos[1] = camY + northStarPos.y;
        pos[2] = camZ + northStarPos.z;
        pos[0] = pos[0] + 0.0f;
        pos[1] = float(double(pos[1]) - 90.0);
        pos[2] = 0.0f + pos[2];
        Vec4 screen;
        float w, h;
        if (CalcScreenCoors(pos, &screen, &w, &h, 0, 0, 0, 1)) {
            int random = Rand() & 0x7F;
            int c = int((random * 0.0015625000232830644 + 0.5) * starBrightness);
            int recipZ = int(1.0 / screen.z) & 0xFF;
            RenderOneXLUSprite(screen.x, screen.y, screen.z, float(w * 5.0), float(h * 5.0), c, c, c, 0xFF, recipZ);
        }
    }
done:
    SetRenderState(0xC, 0);
    SetRenderState(8, 1);
    SetRenderState(6, 1);
    SetRenderState(10, 5);
    SetRenderState(0xB, 6);
}
CLOUDS_REVERSED("CClouds::RenderMoonAndStars_50F320", 0x0050F320, &Clouds_RenderMoonAndStars, nullptr, nullptr, re::Mode::Original);

// 0x50FAE0: the background: `top` above the horizon line, `bottom` below it (to the
// screen's bottom when the line is near it); plain grey when looking nearly straight
// down. The horizon tilts with the camera's roll (tan(roll) * 320 px each side)
void __cdecl Clouds_RenderBackground(int topR, int topG, int topB, int bottomR, int bottomG, int bottomB,
                                     int alpha) {
    const float* t = CameraTransform();
    float rightX = t[0];
    float rightY = t[3];
    float rightZ = t[6];
    float horizontal = float(double(rightX) * rightX + double(rightY) * rightY);
    horizontal = std::fabs(horizontal);
    horizontal = float(std::sqrt(double(horizontal)));
    float c;
    if (horizontal < 0.0f) {
        c = 0.0f;
    } else if (1.0f < horizontal) {
        c = 1.0f;
    } else {
        c = horizontal;
    }
    float roll = float(std::acos(double(c)));
    if (rightZ < 0.0f) {
        roll = -roll;
    }
    roll = float(std::tan(double(roll)));
    horizonTilt = float(double(roll) * 320.0);
    float lookZ = CameraTransform()[8];
    if (lookZ < -0.8999999761581421) {
        float rect[4] = {0.0f, 480.0f, 640.0f, 0.0f};   // left, bottom, right, top
        skyColourC66044.r = 0x32;
        skyColourC66044.g = 0x32;
        skyColourC66044.b = 0x32;
        skyColourC66044.a = 0xFF;
        DrawRect(rect, &skyColourC66044);
        return;
    }
    SetRenderState(7, 2);
    uint8_t* d3dState = *reinterpret_cast<re::Ptr32<uint8_t>*>(theRenderer + 0x6B4);
    if (*reinterpret_cast<int32_t*>(d3dState + 0x168) != 2) {
        void* device = *reinterpret_cast<re::Ptr32<void>*>(d3dState + 0x2238);
        using SetRenderStateFn = long(__stdcall*)(void*, int, int);
        re::PtrCast<SetRenderStateFn>(re::ComVtable(device)[0xE4 / 4])(device, 9, 2);   // D3DRS_SHADEMODE, D3DSHADE_GOURAUD
        *reinterpret_cast<int32_t*>(d3dState + 0x168) = 2;
    }
    horizonY = GetHorizonY();
    uint8_t a = uint8_t(alpha);
    double y = horizonY;
    double tilt = horizonTilt;
    // the sky, from 240 px above the line down to it
    skyColourC66044 = {uint8_t(topR), uint8_t(topG), uint8_t(topB), a};
    skyColourC66048 = {uint8_t(bottomR), uint8_t(bottomG), uint8_t(bottomB), a};
    DrawGradient(0.0f, float(y + tilt - 240.0), 640.0f, float(y - tilt - 240.0), 0.0f, float(y + tilt), 640.0f,
                 float(y - tilt), &skyColourC66044, &skyColourC66044, &skyColourC66048, &skyColourC66048);
    // a 4 px strip of the bottom colour under the line
    y = horizonY;
    tilt = horizonTilt;
    skyColourC66044 = {uint8_t(bottomR), uint8_t(bottomG), uint8_t(bottomB), a};
    DrawGradient(0.0f, float(y + tilt), 640.0f, float(y - tilt), 0.0f, float(y + tilt + 4.0), 640.0f,
                 float(y - tilt + 4.0), &skyColourC66048, &skyColourC66048, &skyColourC66048, &skyColourC66048);
    // the top colour above the gradient, when it doesn't reach the screen's top
    y = horizonY;
    tilt = horizonTilt;
    double aboveLeft = y + tilt - 240.0;
    if (0.0 < aboveLeft || 0.0 < y - tilt - 240.0) {
        skyColourC66044 = {uint8_t(topR), uint8_t(topG), uint8_t(topB), a};
        if (y - std::fabs(float(tilt)) - 240.0 > 480.0) {
            DrawGradient(0.0f, 0.0f, 640.0f, 0.0f, 0.0f, 480.0f, 640.0f, 480.0f, &skyColourC66044, &skyColourC66044,
                         &skyColourC66044, &skyColourC66044);
        } else {
            float right = float(y - tilt - 960.0);
            if (right > 0.0f) {
                right = 0.0f;
            }
            float left = float(y + tilt - 960.0);
            if (left > 0.0f) {
                left = 0.0f;
            }
            DrawGradient(0.0f, left, 640.0f, right, 0.0f, float(aboveLeft), 640.0f, float(y - tilt - 240.0),
                         &skyColourC66044, &skyColourC66044, &skyColourC66044, &skyColourC66044);
        }
    }
    skyColourC66044.r = uint8_t(bottomR);
    skyColourC66044.g = uint8_t(bottomG);
    skyColourC66044.b = uint8_t(bottomB);
    skyColourC66048.r = uint8_t(bottomR);
    skyColourC66048.g = uint8_t(bottomG);
    skyColourC66048.b = uint8_t(bottomB);
}
CLOUDS_REVERSED("CClouds::RenderBackground_50FAE0", 0x0050FAE0, &Clouds_RenderBackground, nullptr, nullptr, re::Mode::Original);

// 0x50FFD0: the horizon band over the background: the 4 px strip again (opaque),
// then a dark red band 34 px deep fading from the bottom colour, down to the screen's
// bottom
void __cdecl Clouds_RenderHorizon() {
    float lookZ = CameraTransform()[8];
    if (lookZ < -0.8999999761581421) {
        return;
    }
    skyColourC66048.a = 0xFF;
    skyColourC66044.a = 0xFF;
    SetRenderState(7, 2);
    double y = horizonY;
    double tilt = horizonTilt;
    DrawGradient(0.0f, float(y + tilt), 640.0f, float(y - tilt), 0.0f, float(y + tilt + 4.0), 640.0f,
                 float(y - tilt + 4.0), &skyColourC66048, &skyColourC66048, &skyColourC66048, &skyColourC66048);
    RGBA band;
    RGBA_Construct(&band, 0x32, 0x1E, 0x1E, 0x80);
    tilt = horizonTilt;
    y = horizonY;
    float lineLeft = float(tilt + y);
    float lineRight = float(y - tilt);
    float bandRight = float(double(lineRight) + 34.28571319580078 + 4.0);
    float bandLeft = float(double(lineLeft) + 34.28571319580078 + 4.0);
    DrawGradient(0.0f, float(double(lineLeft) + 4.0), 640.0f, float(double(lineRight) + 4.0), 0.0f, bandLeft, 640.0f,
                 bandRight, &skyColourC66048, &skyColourC66048, &skyColourC66044, &skyColourC66044);
    float bottomRight = bandRight < 480.0 ? 480.0f : bandRight;
    float bottomLeft = bandLeft < 480.0 ? 480.0f : bandLeft;
    DrawGradient(0.0f, bandLeft, 640.0f, bandRight, 0.0f, bottomLeft, 640.0f, bottomRight, &skyColourC66044,
                 &skyColourC66044, &band, &band);
}
CLOUDS_REVERSED("CClouds::RenderHorizon_50FFD0", 0x0050FFD0, &Clouds_RenderHorizon, nullptr, nullptr, re::Mode::Original);

// 0x510220: a geometry's colour (RGBA bytes, red lowest) into its colour extra data
// (made when missing; a failed allocation writes the name to address 8, as the game
// does), and its 0x762080 by the flag
void __cdecl Clouds_SetGeometryColourOurs(uint8_t* geometry, int flag, uint32_t colour) {
    float r = float(double(colour & 0xFF) / 255.0);
    float g = float(double((colour >> 8) & 0xFF) / 255.0);
    float b = float(double((colour >> 16) & 0xFF) / 255.0);
    float a = float(double(colour >> 24) / 255.0);
    uint8_t* extra = static_cast<uint8_t*>(NiObjectNET_GetExtraData(geometry, domeColourName));
    if (extra != nullptr && IsKindOfIndex(extra, niColorExtraDataClassIndex)) {
        NiColorExtraData_SetValue(extra, r, g, b, a);
    } else {
        void* p = NiNew(0x1C);
        uint8_t* data = p ? static_cast<uint8_t*>(NiColorExtraData_Construct(p, r, g, b, a)) : nullptr;
        FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(data + 8), domeColourName);
        NiObjectNET_AddExtraData(geometry, domeColourName, data);
    }
    // (read and dropped)
    int32_t state;
    GetRenderState(6, &state);
    GetRenderState(8, &state);
    GetRenderState(0xC, &state);
    GetRenderState(10, &state);
    GetRenderState(0xB, &state);
    GetRenderState(0x1D, &state);
    GetRenderState(0x1E, &state);
    NiObjectNET_Fn762080(geometry, re::Flag(flag) ? name_BCC4A0 : name_BCC4A4, -1);
}
CLOUDS_REVERSED("CClouds::SetGeometryColour_510220", 0x00510220, &Clouds_SetGeometryColourOurs, nullptr, nullptr, re::Mode::Original);

// 0x5103D0: colours a sky dome's geometries (walking its nodes) and renders them
void __cdecl Clouds_ColourSkyDomeOurs(uint8_t* object, uint32_t colour) {
    if (object == nullptr) {
        return;
    }
    if (IsKindOfIndex(object, niGeometryClassIndex)) {
        uint8_t* geometry = object;
        void* property = NiAVObject_GetProperty(geometry, 8);
        uint8_t* p = IsKindOf(rttiPropertyC, property);
        int flag = 0;
        if (p != nullptr) {
            uint8_t* q = **reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(p + 0x20);
            if (q != nullptr && *reinterpret_cast<int32_t*>(q + 8) != 0) {
                flag = 1;
            }
        }
        Clouds_SetGeometryColour(geometry, flag, colour);
        using RenderFn = void(__thiscall*)(void*, void*);
        re::PtrCast<RenderFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(geometry))[0x88 / 4])(geometry, theRenderer);
        return;
    }
    uint8_t* node = IsKindOf(rttiNiNode, object);
    if (node == nullptr) {
        return;
    }
    uint32_t count = *reinterpret_cast<uint16_t*>(node + 0xCE);
    if (count == 0) {
        return;
    }
    uint32_t i = 0;
    do {
        void* child = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(node + 0xC8))[i];
        Clouds_ColourSkyDome(child, colour);
        i++;
    } while (*reinterpret_cast<uint16_t*>(node + 0xCE) > i);
}
CLOUDS_REVERSED("CClouds::ColourSkyDome_5103D0", 0x005103D0, &Clouds_ColourSkyDomeOurs, nullptr, nullptr, re::Mode::Original);

// 0x5104C0: the sky domes around the camera (its world position, at z 0), each
// turned about z (dome 2, the drifting one, not turned but moved) and flattened to
// 0.94, coloured (domes 0 and 1 from the time of day, dome 2 white at 0x7D) and
// rendered. Domes past the third take their "colour" from the words after the three
// colours (the axis, then the position), as the game reads its stack
void __cdecl Clouds_RenderSkyDomes(uint8_t* camera) {
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xE, 0);
    SetRenderState(0xC, 1);
    SetRenderState(10, 2);
    SetRenderState(0xB, 2);
    struct {
        RGBA colours[3];  // +0
        float axis[3];    // +0xC
        float pos[3];     // +0x18
    } locals;
    static_assert(sizeof(locals) == 0x24);
    locals.colours[0] = {skyDomeColours[0], skyDomeColours[1], skyDomeColours[2], 0xFF};
    locals.colours[1] = {skyDomeColours[3], skyDomeColours[4], skyDomeColours[5], 0xFF};
    locals.colours[2] = {0xFF, 0xFF, 0xFF, 0x7D};
    locals.axis[0] = *reinterpret_cast<float*>(camera + 0x84);
    locals.axis[1] = *reinterpret_cast<float*>(camera + 0x88);
    locals.axis[2] = *reinterpret_cast<float*>(camera + 0x8C);
    locals.pos[0] = locals.axis[0];
    locals.pos[1] = locals.axis[1];
    locals.pos[2] = 0.0f;
    skyDomeRotation = float(double(skyDomeRotation) - double(g_20C4828) * 0.3141592741012573);
    if (numSkyDomes > 0) {
        locals.axis[0] = 0.0f;
        locals.axis[1] = 0.0f;
        locals.axis[2] = 1.0f;
        int i = 0;
        do {
            float angle;
            if (i == 2) {
                SetRenderState(10, 5);
                SetRenderState(0xB, 6);
                angle = 0.0f;
                cloudDomeDrift = float(double(cloudDomeDrift) + 0.03999999910593033);
                if (180.0f < cloudDomeDrift) {
                    cloudDomeDrift = -180.0f;
                }
                float rad = float(double(cloudDomeDrift) * 0.5 * 3.1415927410125732 / 180.0);
                float c = float(std::cos(double(rad)));
                float dx = float(-double(c) * 110.0);
                float dy = -cloudDomeDrift;
                locals.pos[0] = dx + locals.pos[0];
                locals.pos[1] = dy + locals.pos[1];
                locals.pos[2] = 20.0f;
            } else {
                angle = float((1.0 - double(i) * 0.30000001192092896) * skyDomeRotation);
            }
            if (uint8_t* dome = skyDomes[i]) {
                NiTransform local = *reinterpret_cast<NiTransform*>(dome + 0x2C);
                RotateAboutAxis(&local, locals.axis, angle, 0);
                NiTransform s;
                NiTransform out;
                s.scale = 1.0f;
                NiTransform_MakeIdentity(&s);
                s.rotate[0] = 0.9399999976158142f;
                s.rotate[4] = 0.9399999976158142f;
                s.rotate[8] = 1.0f;
                local = *NiTransform_Multiply(&s, &out, &local);
                s.scale = 1.0f;
                NiTransform_MakeIdentity(&s);
                s.translate[0] = locals.pos[0];
                s.translate[1] = locals.pos[1];
                s.translate[2] = locals.pos[2];
                NiTransform* result = NiTransform_Multiply(&s, &out, &local);
                dome = skyDomes[i];
                local = *result;
                *reinterpret_cast<NiTransform*>(dome + 0x2C) = *result;
                float& scale = *reinterpret_cast<float*>(dome + 0x5C);
                scale = std::fabs(scale);
                AVObject_Update(skyDomes[i], 0.0f, 1);
                Fn_407550(camera);
                Clouds_ColourSkyDome(skyDomes[i], reinterpret_cast<const uint32_t*>(&locals)[i]);
            }
            i++;
        } while (i < numSkyDomes);
    }
    SetRenderState(0xE, 1);
    SetRenderState(10, 5);
    SetRenderState(0xB, 6);
}
CLOUDS_REVERSED("CClouds::RenderSkyDomes_5104C0", 0x005104C0, &Clouds_RenderSkyDomes, nullptr, nullptr, re::Mode::Original);

// 0x510820: CClouds::Init: the moon mask, moon.hdt, and 200 random stars (pushed out
// by 50 on each axis when within 20 of the camera)
void __cdecl Clouds_Init() {
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(Txd_FindTxdSlot("particle", 0));
    moonMaskTexture = Txd_FindTexture("MoonMask", nullptr);
    Txd_PopCurrentTxd();
    Clouds_LoadMoonSettingsFn();
    double zRange = 100.0 - 0.0;
    for (int i = 0; i < kNumStars; i++) {
        starX[i] = float(Rand() / 32767.0 * 200.0 - 100.0);
        starY[i] = float(Rand() / 32767.0 * 200.0 - 100.0);
        starZ[i] = float(Rand() / 32767.0 * zRange + 0.0);
        starSizes[i] = float(Rand() / 32767.0 * 0.29999999701976776 + 0.20000000298023224);
        float x = starX[i];
        float y = starY[i];
        float z = starZ[i];
        float lengthSq = float(double(x) * x + double(y) * y + double(z) * z);
        float length = float(std::sqrt(double(lengthSq)));
        if (length < 20.0f) {
            starX[i] = float(starX[i] + 50.0);
            starY[i] = float(starY[i] + 50.0);
            starZ[i] = float(50.0 + starZ[i]);
        }
    }
}
CLOUDS_REVERSED("CClouds::Init_510820", 0x00510820, &Clouds_Init, nullptr, nullptr, re::Mode::Original);
