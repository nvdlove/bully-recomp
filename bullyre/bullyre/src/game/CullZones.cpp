// CCullZones (0x538C50..0x539084): the attribute zones (boxes of shorts at 0xCED010,
// count 0xCEFC1C) the camera and the player are in, checked on alternate frames: the
// camera's (0xCEFC18: 8 no rain, 0x80 switches the buildings' +0x48 flag), the
// player's (0xCEFC14, with the zones' largest value in 0xCEFC10), and whether the
// camera is inside one fixed quad (0xCEFC21).

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Pool.h"
#include "reversed.h"

namespace cullzones17 {

using col::CVector;

struct CAttributeZone {
    int16_t minX, maxX;   // +0
    int16_t minY, maxY;   // +4
    int16_t minZ, maxZ;   // +8
    int16_t attributes;   // +0xC
    int16_t value;        // +0xE (the largest is kept, e.g. a wanted level drop)
};
static_assert(sizeof(CAttributeZone) == 0x10);

GAME_ARRAY(CAttributeZone, aAttributeZones, 0x00CED010);
GAME_VAR(int32_t, NumAttributeZones, 0x00CEFC1C);
GAME_VAR(int32_t, CurrentValue_Player, 0x00CEFC10);
GAME_VAR(uint32_t, CurrentFlags_Player, 0x00CEFC14);
GAME_VAR(uint32_t, CurrentFlags_Camera, 0x00CEFC18);
GAME_VAR(uint8_t, bNoMirrorsLast, 0x00CEFC20);
GAME_VAR(uint8_t, bCamInQuad, 0x00CEFC21);
GAME_VAR(uint32_t, frameCounter, 0x00C1A99C);
GAME_VAR(void*, cameraNode, 0x00C3CCF4);
GAME_VAR(CPool<uint8_t>*, ms_pBuildingPool, 0x00C0F5F8);
GAME_VAR(CPool<uint8_t>*, ms_pBuildingPool2, 0x00C0F5FC);
GAME_ARRAY(float, quadCorners, 0x00918EF8);   // x1, y1 .. x4, y4

GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(float, __cdecl, MaxFloat, 0x00414490, float a, float b);
GAME_FN(CVector*, __cdecl, FindPlayerCoors, 0x00446900, CVector* out);
GAME_FN(uint8_t, __cdecl, IsPointInQuad_Game, 0x00538C90, float px, float py, float x1, float y1, float x2, float y2, float x3,
        float y3, float x4, float y4);
GAME_FN(uint32_t, __cdecl, FindAttributesForCoors_Game, 0x00538D80, float x, float y, float z, int32_t* value);
GAME_FN(void, __cdecl, UpdateCamInQuad_Game, 0x00538EA0);
GAME_FN(void, __cdecl, SetBuildingsFlag48_Game, 0x00538F40, int value);

// 0x538C50: CCullZones::Init
void __cdecl Init() {
    NumAttributeZones = 0;
    CurrentValue_Player = 0;
    CurrentFlags_Player = 0;
    CurrentFlags_Camera = 0;
    bNoMirrorsLast = 0;
}

// 0x538C70: CCullZones::CamNoRain
uint8_t __cdecl CamNoRain() {
    return uint8_t((uint8_t(CurrentFlags_Camera) >> 3) & 1);
}

// 0x538C80: CCullZones::PlayerNoRain
uint8_t __cdecl PlayerNoRain() {
    return uint8_t((uint8_t(CurrentFlags_Player) >> 3) & 1);
}

// 0x538C90: (px, py) on the inner side of the quad's four edges
uint8_t __cdecl IsPointInQuad(float px, float py, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) {
    float c1 = (px - x1) * (y2 - y1) - (x2 - x1) * (py - y1);
    if (c1 < 0.0f) {
        return 0;
    }
    float c2 = (px - x2) * (y3 - y2) - (x3 - x2) * (py - y2);
    if (c2 < 0.0f) {
        return 0;
    }
    float c3 = (px - x3) * (y4 - y3) - (x4 - x3) * (py - y3);
    if (c3 < 0.0f) {
        return 0;
    }
    float c4 = (px - x4) * (y1 - y4) - (py - y4) * (x1 - x4);
    if (c4 < 0.0f) {
        return 0;
    }
    return 1;
}

// 0x538D80: CCullZones::FindAttributesForCoors: the attributes of every zone the
// point is in, and the largest of their values
uint32_t __cdecl FindAttributesForCoors(float x, float y, float z, int32_t* value) {
    uint32_t attributes = 0;
    if (value) {
        *value = 0;
    }
    for (int i = 0; i < NumAttributeZones; i++) {
        const CAttributeZone& zone = aAttributeZones[i];
        if (float(zone.minX) < x && x < float(zone.maxX) && float(zone.minY) < y && y < float(zone.maxY) && float(zone.minZ) < z &&
            z < float(zone.maxZ)) {
            attributes |= uint32_t(int32_t(zone.attributes));
            if (value) {
                *value = int(MaxFloat(float(*value), float(zone.value)));
            }
        }
    }
    return attributes;
}

// 0x538EA0: whether the camera is inside the fixed quad
void __cdecl UpdateCamInQuad() {
    float y = *reinterpret_cast<float*>(GetLocalTransform(GetCameraNode(cameraNode)) + 0x28);
    float x = *reinterpret_cast<float*>(GetLocalTransform(GetCameraNode(cameraNode)) + 0x24);
    bCamInQuad = IsPointInQuad_Game(x, y, quadCorners[0], quadCorners[1], quadCorners[2], quadCorners[3], quadCorners[4],
                                    quadCorners[5], quadCorners[6], quadCorners[7]);
}

// 0x538F40: the buildings with an object (+0x88) get +0x48 = value
void __cdecl SetBuildingsFlag48(uint8_t value) {
    CPool<uint8_t>* pools[2] = {ms_pBuildingPool, ms_pBuildingPool2};
    for (CPool<uint8_t>* pool : pools) {
        for (int i = pool->size; i != 0;) {
            i--;
            if (pool->flags[i] & 0x80) {
                continue;
            }
            uint8_t* b = pool->objects + pool->elemSize * i;
            if (b && *reinterpret_cast<uint32_t*>(b + 0x88)) {
                *reinterpret_cast<uint32_t*>(b + 0x48) = value;
            }
        }
    }
}

// 0x538FC0: CCullZones::Update (frame 2 of 8: the camera; 4: the quad; 6: the player)
void __cdecl Update() {
    switch (frameCounter & 7) {
    case 2: {
        const float* cam = reinterpret_cast<float*>(GetLocalTransform(GetCameraNode(cameraNode)) + 0x24);
        uint32_t attributes = FindAttributesForCoors_Game(cam[0], cam[1], cam[2], nullptr);
        uint8_t noMirrors = (attributes & 0x80) == 0;
        CurrentFlags_Camera = attributes;
        if (noMirrors != bNoMirrorsLast) {
            SetBuildingsFlag48_Game(noMirrors == 0);
            bNoMirrorsLast = noMirrors;
        }
        break;
    }
    case 4:
        UpdateCamInQuad_Game();
        break;
    case 6: {
        CVector tmp;
        CVector* pos = FindPlayerCoors(&tmp);
        CurrentFlags_Player = FindAttributesForCoors_Game(pos->x, pos->y, pos->z, &CurrentValue_Player);
        break;
    }
    default:
        break;
    }
}

} // namespace cullzones17

// ---- registration ----------------------------------------------------------------------

#define CULLZONES17_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace cullzones17 {
CULLZONES17_REVERSED("CCullZones::Init", 0x00538C50, &Init);
CULLZONES17_REVERSED("CCullZones::CamNoRain", 0x00538C70, &CamNoRain);
CULLZONES17_REVERSED("CCullZones::PlayerNoRain", 0x00538C80, &PlayerNoRain);
CULLZONES17_REVERSED("CCullZones::IsPointInQuad", 0x00538C90, &IsPointInQuad);
CULLZONES17_REVERSED("CCullZones::FindAttributesForCoors", 0x00538D80, &FindAttributesForCoors);
CULLZONES17_REVERSED("CCullZones::UpdateCamInQuad", 0x00538EA0, &UpdateCamInQuad);
CULLZONES17_REVERSED("CCullZones::SetBuildingsFlag48", 0x00538F40, &SetBuildingsFlag48);
CULLZONES17_REVERSED("CCullZones::Update", 0x00538FC0, &Update);
} // namespace cullzones17
