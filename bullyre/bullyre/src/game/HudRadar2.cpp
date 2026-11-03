// The radar (HUD_RADAR), continued from HudRadar.cpp: the blips drawn (a character's
// triangle / box / circle, a sprite), the blips added and removed (by position, by entity,
// by collectible), a blip's position, the area and storey of a point, the tile slots, the
// pulses (the night's rings round the player), the map's "no map here" triangles, the
// constructor, the start-up and shut-down, the map's legend, the frame clipping and the
// map -> screen transform. 0x549C00..0x54BE20.
// All thiscall, written as __fastcall (self, edx, ...); the radar as raw offsets
// (vc::At): HudRadarDraw.cpp's list; the blips at +0x210 (300 of 0x30), the pulses at +0x20.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Hud.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct CVector {
    float x, y, z;
};
struct CVector2D {
    float x, y;
};
// a vector passed by value in 16 bytes (0x548FB0's and 0x54B0A0's: x, y, z and a spare)
struct CVectorArg {
    float x, y, z, w;
};
static_assert(sizeof(CVectorArg) == 0x10);
// the frame's rect (Collision.h's CRect's order: left, top, right, bottom)
struct Rect {
    float left, top, right, bottom;
};

// ---- the game's ------------------------------------------------------------------------
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawCircle, 0x0052D090, const float* centre, float radius, const CRGBA* colour, int segments);
GAME_FN(void, __cdecl, Draw2DPolygon, 0x0052C270, float x1, float y1, float x2, float y2, float x3, float y3,
        float x4, float y4, const CRGBA* colour);
GAME_FN(void, __thiscall, Sprite_Draw, 0x0052C6F0, void* sprite, const Rect* rect, const CRGBA* colour);
GAME_FN(void, __thiscall, Sprite_Construct, 0x00667DA0, void* sprite);
GAME_FN(void, __thiscall, Sprite_Delete, 0x0052C6D0, void* sprite);
GAME_FN(int, __thiscall, Schedule_GetType, 0x006A8CC0, void* schedule, int index);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t*, __cdecl, Pools_GetPed, 0x0044A550, int handle);
GAME_FN(uint8_t*, __cdecl, Pools_GetVehicle, 0x0044A580, int handle);
GAME_FN(uint8_t*, __cdecl, Pools_GetObject, 0x0044A5B0, int handle);
GAME_FN(uint8_t*, __cdecl, Pools_GetPropAnim, 0x0044A5E0, int handle);
GAME_FN(int, __cdecl, Pickups_Find, 0x004438B0, int handle);  // -1: none
GAME_FN(void, __cdecl, Markers_Forget, 0x005287B0, void* entity);
GAME_FN(void, __thiscall, LurStore_BlipRemoved, 0x005DBE60, void* store, int32_t blip);
GAME_FN(int16_t, __thiscall, Collectibles_FindByHash, 0x004263C0, void* mgr, int32_t hash);
GAME_FN(void, __thiscall, Collectibles_GetPosition, 0x00426310, void* mgr, CVector* out, int index);
GAME_FN(void, __thiscall, HUDComponent_Construct, 0x0053CDC0, void* self);
// text
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(void, __cdecl, FE_DrawPanel, 0x006A4420, float x1, float y1, float x2, float y2, float alpha,
        const CRGBA* colourA, const CRGBA* colourB);
// the radar's own (the ones before are HudRadar.cpp's, Hud.cpp's)
GAME_FN(uint8_t, __cdecl, Radar_IsInAreaBox, 0x00548CC0, float x, float y, float z, int area);
GAME_FN(void, __thiscall, Radar_SetBlipEntityFlag, 0x00548D30, void* radar, int index, int flag);
GAME_FN(void, __thiscall, Radar_FindTileTxds, 0x00548EA0, void* radar, int tile);
GAME_FN(uint8_t, __cdecl, Radar_PointInTriangle, 0x00548FB0, CVectorArg p, CVectorArg a, CVectorArg b,
        CVectorArg c);
GAME_FN(void, __thiscall, Radar_ClearBlips, 0x00549180, void* radar);
GAME_FN(void, __thiscall, Radar_LoadSprites, 0x00549260, void* radar);
GAME_FN(void, __thiscall, Radar_RemoveTiles, 0x005492E0, void* radar);
GAME_FN(void, __thiscall, Radar_DrawLegendButton, 0x00549370, void* radar, int index, float x);
GAME_FN(int, __thiscall, Radar_ClipEdge, 0x005497F0, void* radar, CVector2D* out, const CVector2D* a,
        const CVector2D* b);
GAME_FN(void, __thiscall, Radar_WorldToMap, 0x00549A90, void* radar, CVector2D* out, const CVector2D* in);
GAME_FN(void, __thiscall, Radar_DrawCharBlip, 0x00549C00, void* radar, float x, float y, float height, int r,
        int g, int b, int alpha, int arrow);
GAME_FN(void, __thiscall, Radar_DrawBlipSprite, 0x00549FD0, void* radar, int sprite, float x, float y, int alpha,
        int arrow, float scale);
GAME_FN(void, __thiscall, Radar_AllocTileSlots, 0x0054AA70, void* radar);
GAME_FN(int, __cdecl, Radar_GetArea, 0x0054AA20, CVector pos);
GAME_FN(void, __thiscall, Radar_LoadExitLocs, 0x005483A0, void* radar);
GAME_FN(void, __thiscall, Radar_LoadBlipLocs, 0x00548460, void* radar);
GAME_FN(void, __cdecl, Radar_LoadAreaBoxes, 0x00548560);

GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(int32_t, g_widescreen, 0x00C3CD0C);        // (nothing of the radar drawn while set)
GAME_VAR(uint8_t, g_blipDrawing, 0x020DCD20);       // set round a char blip's drawing
GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_pulseFade, 0x00AB7394);           // (0.02: a pulse's life per step)
GAME_VAR(float, g_pulseTurn, 0x00AB73A0);           // (pi / 16)
GAME_VAR(uint16_t, g_clockSeconds, 0x00BCEB48);
GAME_VAR(uint8_t, g_clockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t, g_clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, g_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, g_seasonByChapter, 0x00A147EC);  // 2: winter
GAME_ARRAY(uint8_t, g_blipSprites, 0x0091A3E8);      // [39] 0x14: name, +4 its legend line, +0xC colour
GAME_ARRAY(uint8_t, g_radarAreas, 0x00CF01E8);       // [64] 0x20: min / max x, y, z, +0x18 the kind
GAME_ARRAY(uint8_t, g_mapLegend, 0x00AB7338);        // [10] 8: the text key, +4 shown, +5 can toggle
GAME_ARRAY(uint8_t, g_pickups, 0x00C0A100);          // [] 0x48: +0x1C the entity
GAME_ARRAY(CRGBA, g_feColours, 0x020C3324);          // the frontend's colours (0 .. 6 used here)
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_VAR(int16_t, g_fontEdge, 0x0208B784);
inline void* const kSchedule = reinterpret_cast<void*>(0x020C35E8);
inline void* const kCollectibles = reinterpret_cast<void*>(0x00BCEB50);
inline void* const kTheLurStore = reinterpret_cast<void*>(0x00D02850);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
inline const CRGBA* const kPanelEdge = reinterpret_cast<const CRGBA*>(0x020C33D0);

int32_t& Mode(void* radar) {
    return At<int32_t>(radar, 0xE788);
}
uint8_t* Blip(void* radar, uint32_t index) {
    // (an index out of a blip id: the high nibble kept, so the address wraps like the game's)
    return reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(radar) + 0x210 + uintptr_t(index * 0x30u));
}
// a blip id: its generation << 12 | its index; a blip's generation 1 .. 0xFFFE
int32_t NextId(uint8_t* blip, uint32_t index) {
    int32_t& generation = At<int32_t>(blip, 0x14);
    if (generation < 0xFFFE) {
        generation = generation + 1;
    } else {
        generation = 1;
    }
    return int32_t(uint32_t(generation) << 12 | index);
}
// the first blip not in use (flag 2), 300: none
uint32_t FreeBlip(void* radar) {
    uint32_t index = 0;
    while (index < 300 && (At<uint8_t>(Blip(radar, index), 0x2F) & 2)) {
        index++;
    }
    return index;
}

// ---- the blips drawn ---------------------------------------------------------------------

// 0x549C00: a character's blip: 0 a triangle up, 1 a triangle down (half seen: on another
// storey), 2 a circle; each black and larger under the colour
void __fastcall Radar_DrawCharBlip_(uint8_t* self, re::Edx, float x, float y, float height, int r, int g, int b,
                                     int alpha, int arrow) {
    if (At<uint8_t>(self, 0xE7F4) || g_widescreen) {
        return;
    }
    g_blipDrawing = 1;
    if (Mode(self) == 2) {
        height = height * 2.0f;
    } else if (Mode(self) == 1) {
        height = height * 0.5f;
    }
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    CRGBA colour;
    if (arrow == 0) {
        float top = y - sy * (height + 3.0f);
        float bottom = y + sy * (height + 2.0f);
        float half = (height + 3.0f) * sx;
        RGBA_Construct(&colour, 0, 0, 0, alpha);
        Draw2DPolygon(x + half, bottom, x - half, bottom, x, top, x, top, &colour);
        float up = y - sy * (height + 1.0f);
        float down = y + sy * (height + 1.0f);
        float w = sx * (height + 1.0f);
        RGBA_Construct(&colour, r, g, b, alpha);
        Draw2DPolygon(x + w, down, x - w, down, x, up, x, up, &colour);
    } else if (arrow == 1) {
        float top = y - sy * (height + 2.0f);
        float bottom = y + sy * (height + 3.0f);
        float half = sx * (height + 3.0f);
        RGBA_Construct(&colour, 0, 0, 0, 0x7F);
        Draw2DPolygon(x, bottom, x, bottom, x + half, top, x - half, top, &colour);
        float up = y - sy * (height + 1.0f);
        float down = y + sy * (height + 1.0f);
        float w = sx * (height + 1.0f);
        RGBA_Construct(&colour, r, g, b, 0x7F);
        Draw2DPolygon(x, down, x, down, x + w, up, x - w, up, &colour);
    } else if (arrow == 2) {
        float centre[2] = {x, y};
        RGBA_Construct(&colour, 0, 0, 0, alpha);
        DrawCircle(centre, height, &colour, 8);
        float centre2[2] = {x, y};
        RGBA_Construct(&colour, r, g, b, alpha);
        DrawCircle(centre2, height - 1.0f, &colour, 8);
    }
    g_blipDrawing = 0;
}

// the sprite's rect, and whether any of it is on the 640x480 screen (NaNs: off)
bool SpriteRect(Rect& rect, float x, float y, float halfW, float halfH) {
    rect.left = x - halfW;
    rect.bottom = y - halfH;
    rect.right = x + halfW;
    rect.top = y + halfH;
    return rect.right >= 0.0f && rect.left <= 640.0f && rect.top >= 0.0f && rect.bottom <= 480.0f;
}

// 0x549FD0: a blip's sprite (arrow 2), or a small triangle in the sprite's colour (0 / 1:
// above / below the player); hidden sprites by the mask at +0xE82C; the school's (5)
// blinking to 6 out of class; the curfew clock's (0x12) reddening from 1 am to 6 am
void __fastcall Radar_DrawBlipSprite_(uint8_t* self, re::Edx, int sprite, float x, float y, int alpha, int arrow,
                                       float scale) {
    if (At<uint8_t>(self, 0xE7F4)) {
        return;
    }
    if (At<uint32_t>(self, 0xE82C) & (1u << (sprite & 31))) {
        return;
    }
    if (sprite == 5 && Schedule_GetType(kSchedule, -1) != 1) {
        uint8_t blink = uint8_t(uint8_t(g_clockSeconds) - uint8_t(g_clockMinutes * 4)) & 8;
        sprite = sprite + (blink != 0 ? 1 : 0);
    }
    float size = scale * 0.5f;
    const uint8_t* info = g_blipSprites + sprite * 0x14;
    CRGBA colour{info[0xC], info[0xD], info[0xE], uint8_t(arrow == 1 ? 0x7F : 0xFF)};
    if (Mode(self) == 2) {
        int32_t legend = *reinterpret_cast<const int32_t*>(info + 4);
        if (legend == -1 || !g_mapLegend[legend * 8 + 4]) {
            return;
        }
        float zoomMin = At<float>(self, 0x168) / At<float>(self, 0x158);
        float num = At<float>(self, 0xE7B0) - zoomMin;
        float zoomMax = At<float>(self, 0x168) / At<float>(self, 0x154);
        float t = 1.0f - num / (zoomMax - zoomMin);
        size = t * 1.3f + 0.7f;
    } else if (Mode(self) == 1) {
        size = 0.35f;
    }
    size = At<float>(self, 0x148) * size;
    float halfW = g_screenScaleX * size;
    float halfH = g_screenScaleY * size;
    if (arrow != 2) {
        // (the colour's bytes go as the low bytes of dwords read across it: the rest unused)
        Radar_DrawCharBlip(self, x, y, 5.0f, colour.r, colour.g, colour.b, colour.a, arrow);
        return;
    }
    Rect rect;
    if (sprite != 0x12) {
        if (!SpriteRect(rect, x, y, halfW, halfH)) {
            return;
        }
        CRGBA grey;
        RGBA_Construct(&grey, 0xEB, 0xEB, 0xEB, alpha);
        Sprite_Draw(self + 0x3A98 + sprite * 4, &rect, &grey);
        return;
    }
    CRGBA c = colour;
    uint8_t hours = g_clockHours;
    if (uint8_t(hours - 1) <= 5) {
        float t = 1.0f;
        if (hours < 2) {
            t = float(g_clockMinutes) / 59.0f;
        }
        CRGBA red;
        RGBA_Construct(&red, 0x73, 0x20, 0x20, 0xFF);
        c.r = uint8_t(int(float(int(red.r) - int(c.r)) * t + float(c.r)));
        c.g = uint8_t(int(float(int(red.g) - int(c.g)) * t + float(c.g)));
        c.b = uint8_t(int(t * float(int(red.b) - int(c.b)) + float(c.b)));
    }
    c.a = uint8_t(alpha);
    if (!SpriteRect(rect, x, y, halfW, halfH)) {
        return;
    }
    Sprite_Draw(self + 0x3AE0, &rect, &c);
}

// ---- the blips added, removed ------------------------------------------------------------

// 0x54A410: a blip at a position (-1: none free, or the position 0, 0, 0); flashing 2.5 s
int32_t __fastcall Radar_AddBlip(uint8_t* self, re::Edx, int type, float x, float y, float z, uint32_t,
                                 int display, int sprite, uint8_t ownStorey) {
    if (x == 0.0f && y == 0.0f && z == 0.0f) {
        return -1;
    }
    uint32_t index = FreeBlip(self);
    if (index >= 300) {
        return -1;
    }
    uint8_t* blip = Blip(self, index);
    At<int32_t>(blip, 0x20) = type;
    At<float>(blip, 0x1C) = 0.5f;
    uint8_t& flags = At<uint8_t>(blip, 0x2F);
    flags = uint8_t((flags & ~4) | ((ownStorey * 4) & 4) | 3);
    At<float>(blip, 4) = x;
    At<float>(blip, 8) = y;
    At<int32_t>(blip, 0) = 0;
    At<float>(blip, 0xC) = z;
    At<int32_t>(blip, 0x24) = display;
    At<int32_t>(blip, 0x28) = sprite;
    flags = uint8_t((flags & 0xF7) | 0x10);
    At<int8_t>(blip, 0x2E) = 1;
    At<uint32_t>(blip, 0x18) = g_timeInMilliseconds + 2500;
    At<int16_t>(blip, 0x2C) = -1;
    return NextId(blip, index);
}

// 0x54A5B0: a blip on an entity (the sprites 2, 0x11, 0x1B only on characters: 2, 6, 0xD)
int32_t __fastcall Radar_AddEntityBlip(uint8_t* self, re::Edx, int type, int handle, uint32_t, int display,
                                       int sprite, uint8_t ownStorey) {
    if ((sprite == 2 || sprite == 0x11 || sprite == 0x1B) && type != 2 && type != 0xD && type != 6) {
        return -1;
    }
    uint32_t index = FreeBlip(self);
    if (index >= 300) {
        return -1;
    }
    uint8_t* blip = Blip(self, index);
    At<int32_t>(blip, 0x20) = type;
    At<float>(blip, 0x1C) = 0.5f;
    At<int32_t>(blip, 0) = handle;
    At<int32_t>(blip, 0x24) = display;
    uint8_t& flags = At<uint8_t>(blip, 0x2F);
    flags = uint8_t(((ownStorey & 1) * 4) | (flags & 0xE3) | 3);
    At<int8_t>(blip, 0x2E) = 1;
    At<int32_t>(blip, 0x28) = sprite;
    At<uint32_t>(blip, 0x18) = g_timeInMilliseconds + 2500;
    At<int16_t>(blip, 0x2C) = -1;
    return NextId(blip, index);
}

// 0x54A700: a collectible's blip (type 0xB, at the collectible's position, by its hash)
int32_t __fastcall Radar_AddCollectibleBlip(uint8_t* self, re::Edx, int handle, int32_t hash, uint32_t,
                                            int display, int sprite, uint8_t ownStorey) {
    uint32_t index = FreeBlip(self);
    int16_t collectible = Collectibles_FindByHash(kCollectibles, hash);
    CVector pos;
    Collectibles_GetPosition(kCollectibles, &pos, collectible);
    if (index >= 300) {
        return -1;
    }
    uint8_t* blip = Blip(self, index);
    At<float>(blip, 0x1C) = 0.5f;
    uint8_t& flags = At<uint8_t>(blip, 0x2F);
    At<int32_t>(blip, 0x20) = 0xB;
    flags = uint8_t((flags & ~4) | ((ownStorey * 4) & 4) | 3);
    At<float>(blip, 4) = pos.x;
    At<float>(blip, 8) = pos.y;
    flags = uint8_t(flags & 0xE7);
    At<int32_t>(blip, 0) = handle;
    At<int32_t>(blip, 0x24) = display;
    At<int32_t>(blip, 0x10) = hash;
    At<int8_t>(blip, 0x2E) = 1;
    At<int32_t>(blip, 0x28) = sprite;
    At<uint32_t>(blip, 0x18) = g_timeInMilliseconds + 2500;
    At<int16_t>(blip, 0x2C) = -1;
    return NextId(blip, index);
}

// a blip let go: its entity flag off, its type / display / sprite / id cleared, the scripts told
void FreeBlipAt(uint8_t* self, uint8_t* blip, uint32_t index, int32_t id) {
    Radar_SetBlipEntityFlag(self, int(index), 0);
    At<uint8_t>(blip, 0x2F) &= 0xE5;
    At<int32_t>(blip, 0x20) = 0;
    At<int32_t>(blip, 0x24) = 0;
    At<int32_t>(blip, 0x28) = -1;
    At<int16_t>(blip, 0x2C) = -1;
    LurStore_BlipRemoved(kTheLurStore, id);
}

// 0x54A890: every blip of this type on this entity removed (a character's marker, if it
// was shown with one, forgotten: its vehicle's, if in one)
void __fastcall Radar_RemoveBlipsFor(uint8_t* self, re::Edx, int type, int handle) {
    for (uint32_t index = 0; index < 300; index++) {
        uint8_t* blip = Blip(self, index);
        int32_t blipType = At<int32_t>(blip, 0x20);
        if (blipType != type || At<int32_t>(blip, 0) != handle) {
            continue;
        }
        int32_t display = At<int32_t>(blip, 0x24);
        if ((display == 5 || display == 2 || display == 4) && (blipType == 2 || blipType == 0xD || blipType == 6)) {
            uint8_t* ped = Pools_GetPed(At<int32_t>(blip, 0));
            uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
            Markers_Forget(vehicle ? vehicle : ped);
        }
        Radar_SetBlipEntityFlag(self, int(index), 0);
        int32_t id = int32_t(uint32_t(At<int32_t>(blip, 0x14)) << 12 | index);
        At<uint8_t>(blip, 0x2F) &= 0xE5;
        At<int32_t>(blip, 0x20) = 0;
        At<int32_t>(blip, 0x24) = 0;
        At<int32_t>(blip, 0x28) = -1;
        At<int16_t>(blip, 0x2C) = -1;
        LurStore_BlipRemoved(kTheLurStore, id);
    }
}

// 0x54A940: a blip removed by its id (a stale id: nothing)
void __fastcall Radar_RemoveBlip(uint8_t* self, re::Edx, int32_t id) {
    if (id == -1) {
        return;
    }
    uint32_t index = uint32_t(id) & 0xF0000FFF;
    uint8_t* blip = Blip(self, index);
    if (int32_t(uint32_t(At<int32_t>(blip, 0x14)) << 12 | index) != id || index == 0xFFFFFFFF) {
        return;
    }
    FreeBlipAt(self, blip, index, id);
}

// 0x54AA20: the area a point's in (by the radar's boxes, 2 .. 63; 31 and none: 0)
int __cdecl Radar_GetArea_(CVector pos) {
    for (int area = 2; area < 0x40; area++) {
        if (Radar_IsInAreaBox(pos.x, pos.y, pos.z, area)) {
            return area != 0x1F ? area : 0;
        }
    }
    return 0;
}

// 0x54AA70: the tile slots (re)made for cells x cells tiles (the ground's and winter's by
// pair) and each tile's texture dictionaries found
void __fastcall Radar_AllocTileSlots_(uint8_t* self, re::Edx) {
    if (At<int32_t*>(self, 0x3A54)) {
        Free(At<int32_t*>(self, 0x3A54));
        Free(At<int32_t*>(self, 0x3A58));
        Free(At<int32_t*>(self, 0x3A5C));
    }
    auto bytes = [](int32_t count) {
        uint64_t size = uint64_t(uint32_t(count)) * 4;
        return (size >> 32) ? 0xFFFFFFFFu : uint32_t(size);
    };
    int32_t tiles = At<int32_t>(self, 0x130) * At<int32_t>(self, 0x130);
    At<void*>(self, 0x3A54) = Malloc(bytes(tiles));
    int32_t pairs = tiles >> 1;
    At<void*>(self, 0x3A58) = Malloc(bytes(pairs));
    At<void*>(self, 0x3A5C) = Malloc(bytes(pairs));
    for (int32_t i = 0; i < tiles; i++) {
        At<int32_t*>(self, 0x3A54)[i] = -1;
    }
    for (int32_t i = 0; i < pairs; i++) {
        At<int32_t*>(self, 0x3A58)[i] = -1;
        At<int32_t*>(self, 0x3A5C)[i] = -1;
    }
    for (int32_t y = 0; y < At<int32_t>(self, 0x130); y++) {
        for (int32_t x = 0; x < At<int32_t>(self, 0x130); x++) {
            Radar_FindTileTxds(self, At<int32_t>(self, 0x130) * y + x);
        }
    }
}

// 0x54AC30: a blip's position (its entity's, a position blip's own); false: a stale id or
// a type without one (an entity gone: true, nothing written)
uint8_t __fastcall Radar_GetBlipPosition(uint8_t* self, re::Edx, int32_t id, CVector* out) {
    if (id == -1) {
        return 0;
    }
    uint32_t index = uint32_t(id) & 0xF0000FFF;
    uint8_t* blip = Blip(self, index);
    if (int32_t(uint32_t(At<int32_t>(blip, 0x14)) << 12 | index) != id || index == 0xFFFFFFFF) {
        return 0;
    }
    int32_t handle = At<int32_t>(blip, 0);
    uint8_t* entity;
    switch (At<int32_t>(blip, 0x20)) {
    case 1:
    case 0xC:
        entity = Pools_GetVehicle(handle);
        break;
    case 2:
    case 6:
    case 0xD:
        entity = Pools_GetPed(handle);
        break;
    case 3:
        entity = Pools_GetObject(handle);
        break;
    case 4:
    case 5:
    case 8:
    case 9:
        out->x = At<float>(blip, 4);
        out->y = At<float>(blip, 8);
        out->z = At<float>(blip, 0xC);
        return 1;
    case 7:
        entity = Pools_GetPropAnim(handle);
        break;
    case 0xB: {
        int pickup = Pickups_Find(handle);
        if (pickup < 0) {
            return 0;
        }
        entity = *reinterpret_cast<re::Ptr32<uint8_t>*>(g_pickups + pickup * 0x48 + 0x1C);
        break;
    }
    default:
        return 0;
    }
    if (entity) {
        uint8_t* matrix = At<uint8_t*>(entity, 0x14);
        const uint32_t* pos = reinterpret_cast<const uint32_t*>(matrix ? matrix + 0x30 : entity + 4);
        uint32_t* dst = reinterpret_cast<uint32_t*>(out);
        dst[0] = pos[0];
        dst[1] = pos[1];
        dst[2] = pos[2];
    }
    return 1;
}

// 0x54AD80: the tiles dropped, winter's used in a winter chapter
void __fastcall Radar_RefreshSeason(uint8_t* self, re::Edx) {
    Radar_RemoveTiles(self);
    At<int32_t>(self, 0xE828) = g_seasonByChapter[g_chapter] == 2 ? 1 : 0;
}

// ---- the pulses (the night's rings on the radar) -------------------------------------------

// A pulse (8 at +0x20, 0x20 each)
struct Pulse {
    float x, y;            // +0x00 (where added; unused after)
    float offsetX, offsetY;  // +0x08 from the latest point (+0x124)
    float velX, velY;      // +0x10
    float life;            // +0x18 1.5 .. 0 (0: free)
    float phase;           // +0x1C its wobble
};
static_assert(sizeof(Pulse) == 0x20);

// 0x54ADC0: at night (7 pm .. 7 am) on the radar, a pulse at the point, 0.8 s apart
void __fastcall Radar_AddPulse(uint8_t* self, re::Edx, float x, float y) {
    uint8_t hours = g_clockHours;
    if (hours < 19 && hours >= 7) {
        return;
    }
    if (Mode(self) != 0) {
        return;
    }
    At<float>(self, 0x124) = x;
    At<float>(self, 0x128) = y;
    if (!(At<float>(self, 0x120) == 0.0f)) {
        return;
    }
    At<float>(self, 0x120) = 0.8f;
    for (int i = 0; i < 8; i++) {
        Pulse& pulse = At<Pulse>(self, 0x20 + i * 0x20);
        if (pulse.life == 0.0f) {
            pulse.x = x;
            pulse.y = y;
            pulse.offsetX = 0.0f;
            pulse.offsetY = 0.0f;
            pulse.velX = 0.0f;
            pulse.velY = -0.4f;
            pulse.life = 1.5f;
            pulse.phase = float(Rand()) / 32767.0f * 6.2831855f - 3.1415927f;
            return;
        }
    }
}

// 0x54AE90: the pulses drawn (sprite 0x12, growing, fading in and out, wobbling) and moved
void __fastcall Radar_DrawPulses(uint8_t* self, re::Edx) {
    if (Mode(self) != 0) {
        return;
    }
    float timer = At<float>(self, 0x120) - g_pulseFade * g_timeStep;
    At<float>(self, 0x120) = timer;
    if (timer <= 0.0f) {
        At<float>(self, 0x120) = 0.0f;
    }
    for (int i = 0; i < 8; i++) {
        Pulse& pulse = At<Pulse>(self, 0x20 + i * 0x20);
        int alpha = 0xFF;
        float age = pulse.life / 1.5f;
        float size = (1.0f - age) * 0.79999995f + 0.5f;
        if (age < 0.4f) {
            alpha = int(age / 0.4f * 255.0f);
        } else if (double(age) > 0.7999999970197678) {
            alpha = int((1.0f - age) / 0.2f * 255.0f);
        }
        float y = At<float>(self, 0x128) + pulse.offsetY;
        float wobble = float(std::sin(double(pulse.phase)));
        float x = pulse.offsetX + At<float>(self, 0x124) + wobble * size * 3.0f;
        Radar_DrawBlipSprite(self, 0x12, x, y, alpha, 2, size);
        float step = g_timeStep;
        float dx = pulse.velX * step;
        float dy = step * pulse.velY;
        pulse.offsetX = pulse.offsetX + dx;
        pulse.offsetY = pulse.offsetY + dy;
        float turn = float(Rand()) / 32767.0f * 0.19999999f + 0.8f;
        float phase = g_pulseTurn * turn + pulse.phase;
        pulse.phase = phase;
        if (phase > 3.1415927f) {
            pulse.phase = -3.1415927f;
        }
        float life = pulse.life - g_pulseFade * g_timeStep;
        pulse.life = life;
        if (life <= 0.0f) {
            pulse.life = 0.0f;
        }
    }
}

// ---- where the map is --------------------------------------------------------------------

// 0x54B0A0 (registers: ecx the triangles, edx the points, edi their count): the point in
// any of the triangles
uint8_t InTriangles(const int32_t* triangles, const CVector2D* points, int count, CVectorArg pos) {
    for (int i = 0; i < count; i++) {
        const int32_t* t = triangles + i * 3;
        CVectorArg a{points[t[0]].x, points[t[0]].y, 0.0f, 0.0f};
        CVectorArg b{points[t[1]].x, points[t[1]].y, 0.0f, 0.0f};
        CVectorArg c{points[t[2]].x, points[t[2]].y, 0.0f, 0.0f};
        if (Radar_PointInTriangle(pos, a, b, c)) {
            return 1;
        }
    }
    return 0;
}

// the chapters' "off the map" triangles (by chapter, from each chapter to the last's)
struct OffMap {
    uintptr_t count, triangles, points;
};
constexpr OffMap kOffMap[4] = {
    {0x00AB595C, 0x00AB5CE0, 0x00AB5960},
    {0x00AB5124, 0x00AB5478, 0x00AB5128},
    {0x00AB4A38, 0x00AB4A40, 0x00AB4978},
    {0x00AB4DB0, 0x00AB4DB8, 0x00AB4B48},
};

// 0x54B170: a point on the map drawn this chapter (not in its or a later chapter's
// "off the map" triangles)
uint8_t __fastcall Radar_IsOnMap(uint8_t*, re::Edx, float x, float y) {
    uint32_t chapter = g_chapter;
    if (chapter > 3) {
        return 1;
    }
    for (uint32_t c = chapter; c < 4; c++) {
        const OffMap& t = kOffMap[c];
        CVectorArg pos{x, y, 0.0f, 0.0f};
        if (InTriangles(reinterpret_cast<const int32_t*>(t.triangles), reinterpret_cast<const CVector2D*>(t.points),
                        *reinterpret_cast<const int32_t*>(t.count), pos)) {
            return 0;
        }
    }
    return 1;
}

// ---- made, started, shut ----------------------------------------------------------------

// 0x54B280: HUDRadar::HUDRadar
void* __fastcall Radar_Construct(uint8_t* self, re::Edx) {
    HUDComponent_Construct(self);
    At<uint32_t>(self, 0) = 0x0091A810;
    const float big = 1000000.0f;
    At<float>(self, 0x138) = big;
    At<float>(self, 0x144) = big;
    At<float>(self, 0x140) = -big;
    At<float>(self, 0x13C) = -big;
    At<float>(self, 0x19C) = -big;
    At<float>(self, 0x198) = -big;
    At<float>(self, 0x194) = big;
    At<float>(self, 0x1A0) = big;
    At<float>(self, 0x1BC) = big;
    At<float>(self, 0x1C8) = big;
    At<float>(self, 0x1C4) = -big;
    At<float>(self, 0x1C0) = -big;
    At<void*>(self, 0x3A54) = nullptr;
    At<void*>(self, 0x3A58) = nullptr;
    At<void*>(self, 0x3A5C) = nullptr;
    for (int i = 0; i < 14; i++) {
        Sprite_Construct(self + 0x3A60 + i * 4);
    }
    for (int i = 0; i < 39; i++) {
        Sprite_Construct(self + 0x3A98 + i * 4);
    }
    for (int i = 0; i < 0x900; i++) {
        float* p = &At<float>(self, 0x7B34 + i * 0xC);
        p[0] = 0.0f;
        p[1] = 0.0f;
        p[2] = 0.0f;
    }
    At<float>(self, 0xE748) = 0.0f;
    At<float>(self, 0xE74C) = 0.0f;
    At<float>(self, 0xE750) = 0.0f;
    At<int32_t>(self, 0xE788) = 0;
    At<float>(self, 0xE764) = 1600.0f;
    At<float>(self, 0xE7A4) = 0.0f;
    At<float>(self, 0xE7A8) = 0.0f;
    At<float>(self, 0xE7AC) = 0.0f;
    At<uint8_t>(self, 0xE7E8) = 0;
    At<int32_t>(self, 0xE7EC) = 0;
    At<uint8_t>(self, 0xE7F5) = 0;
    At<float>(self, 0xE800) = 0.0f;
    At<float>(self, 0xE804) = 0.0f;
    At<float>(self, 0xE808) = 0.0f;
    At<int32_t>(self, 0xE828) = 0;
    At<int32_t>(self, 0xE82C) = 0;
    At<float>(self, 0xE838) = 0.0f;
    At<float>(self, 0xE83C) = 0.0f;
    At<float>(self, 0xE840) = 0.0f;
    return self;
}

// 0x54B3F0: started: vf7 (reset), the blips cleared, the tile slots, the sprites, the .dat files
void __fastcall Radar_Initialise(uint8_t* self, re::Edx) {
    re::Ptr32<void>* vtable = At<re::Ptr32<void>*>(self, 0);
    re::PtrCast<void(__thiscall*)(void*)>(vtable[7])(self);
    Radar_ClearBlips(self);
    Radar_AllocTileSlots(self);
    Radar_LoadSprites(self);
    Radar_LoadExitLocs(self);
    Radar_LoadBlipLocs(self);
    Radar_LoadAreaBoxes();
}

// 0x54B430: shut: the sprites deleted, the tiles dropped
void __fastcall Radar_Shutdown(uint8_t* self, re::Edx) {
    for (int i = 0; i < 14; i++) {
        Sprite_Delete(self + 0x3A60 + i * 4);
    }
    for (int i = 0; i < 39; i++) {
        Sprite_Delete(self + 0x3A98 + i * 4);
    }
    Radar_RemoveTiles(self);
}

// ---- the map's legend --------------------------------------------------------------------

// 0x54B480: the legend's buttons along the bottom (the toggle button only while the legend's
// open, +0xE7E8), and, open, its panel: a line each (a box, ticked if shown, the name; the
// selected line, +0xE7EC, edged)
void __fastcall Radar_DrawLegend(uint8_t* self, re::Edx) {
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    int buttons = 0;
    float boxW = sx * 10.0f;
    float boxH = sy * 10.0f;
    float tickW = sx * 16.0f;
    float tickH = sy * 16.0f;
    for (int i = 0; i <= 4; i++) {
        if (At<uint8_t>(self, 0xE7E8) && i == 1) {
            continue;
        }
        Radar_DrawLegendButton(self, i, float(buttons) * 80.0f + 160.0f);
        buttons++;
    }
    if (At<uint8_t>(self, 0xE7E8)) {
        Radar_DrawLegendButton(self, 5, float(buttons) * 80.0f + 160.0f);
    }
    Font_Begin();
    Font_SetJustify(3);
    Font_SetSizePreset(1);
    float widest = 0.0f;
    for (int i = 0; i < 10; i++) {
        const char* key = *reinterpret_cast<const re::Ptr32<const char>*>(g_mapLegend + i * 8);
        float w = Font_GetStringWidth(Text_Get(kTheText, key), 1);
        if (!(w < widest)) {
            widest = w;
        }
    }
    float textX = At<float>(self, 0x1C4) - 8.0f - widest - 15.0f;
    float boxX = textX - 8.0f;
    float panelX = boxX - boxW;
    if (!At<uint8_t>(self, 0xE7E8)) {
        return;
    }
    CRGBA panel;
    RGBA_Construct(&panel, 0x80, 0x80, 0x80, 0xD8);
    CRGBA line;
    RGBA_Construct(&line, 0x60, 0x60, 0x60, 0xE1);
    FE_DrawPanel(panelX - 8.0f, At<float>(self, 0x1C0), At<float>(self, 0x1C4), At<float>(self, 0x1C8) + 72.0f, 2.0f,
                 &panel, kPanelEdge);
    Font_Begin();
    float rowX = panelX + 0.0f;
    for (int i = 0; i < 10; i++) {
        const uint8_t* entry = g_mapLegend + i * 8;
        bool selected = i == At<int32_t>(self, 0xE7EC);
        CRGBA colour;
        if (!entry[5]) {
            colour = selected ? g_feColours[3] : g_feColours[0];
        } else {
            colour = selected ? g_feColours[6] : g_feColours[1];
        }
        g_fontEdge = selected ? 2 : 0;
        float rowY = float(i) * At<float>(self, 0x1E0) + At<float>(self, 0x1C0);
        float boxY = rowY + 6.0f;
        FE_DrawPanel(rowX, boxY, rowX + boxW, boxY + boxH, 2.0f, &line, kPanelEdge);
        if (entry[4]) {
            float tickY = rowY + 0.0f;
            Rect rect{rowX, tickY + tickH, rowX + tickW, tickY};
            CRGBA white;
            RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
            Sprite_Draw(self + 0x3A74, &rect, &white);
        }
        Font_SetJustify(3);
        Font_SetSizePreset(1);
        g_fontColour[1] = colour.g;
        g_fontColour[3] = colour.a;
        g_fontColour[0] = colour.r;
        g_fontColour[2] = colour.b;
        const uint16_t* text = Text_Get(kTheText, *reinterpret_cast<const re::Ptr32<const char>*>(entry));
        Font_PrintString(textX, rowY + 1.0f, text, 1);
        Font_Render();
    }
}

// 0x54B8D0: a point's storey (an area's kind, but a storeyed area's (4) by thirds of its
// height: 1, 2, 3)
int __fastcall Radar_GetStorey(uint8_t*, re::Edx, CVector pos) {
    int area = Radar_GetArea(pos);
    const uint8_t* box = g_radarAreas + area * 0x20;
    int32_t kind = *reinterpret_cast<const int32_t*>(box + 0x18);
    if (kind != 4) {
        return kind;
    }
    float minZ = *reinterpret_cast<const float*>(box + 0x10);
    float third = (*reinterpret_cast<const float*>(box + 0x14) - minZ) / 3.0f;
    float up = pos.z - minZ;
    if (third > up) {
        return 1;
    }
    return third + third > up ? 2 : 3;
}

// ---- the frame ---------------------------------------------------------------------------

// 0x54B960: a quad (in -1..1 frame space) clipped to the frame, out up to 8 corners (the
// frame's own corners between edge crossings); the map modes: nothing clipped. A quad
// round the whole frame: the frame's 4 corners.
int __fastcall Radar_ClipToFrame(uint8_t* self, re::Edx, CVector2D* out, const CVector2D* in) {
    // [0]: a spare point (read as corner -1 when no crossing's found: the game's), then the
    // frame's corners
    CVector2D pts[5] = {{0.0f, 0.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}, {-1.0f, -1.0f}};
    CVector2D& spare = pts[0];
    CVector2D* corners = pts + 1;
    int mode = Mode(self);
    uint8_t inside[4];
    for (int k = 0; k < 4; k++) {
        if (mode == 2 || mode == 1) {
            inside[k] = 1;
        } else {
            inside[k] = !(in[k].x < -1.0f) && !(in[k].x > 1.0f) && !(in[k].y < -1.0f) && !(in[k].y > 1.0f);
        }
    }
    int count = 0;
    int last = -1;
    for (int i = 0; i < 4; i++) {
        const CVector2D* cur = in + i;
        if (inside[i]) {
            out[count] = *cur;
            count++;
            continue;
        }
        int into = Radar_ClipEdge(self, out + count, cur, in + ((i - 1) & 3));
        if (into != -1) {
            count++;
            last = into;
        }
        CVector2D* at = out + count;
        int outOf = Radar_ClipEdge(self, at, cur, in + ((i + 1) & 3));
        if (outOf == -1) {
            continue;
        }
        if (into == -1) {
            if (last == -1) {
                for (int k = 3; k >= i; k--) {
                    int e = k == 0 ? Radar_ClipEdge(self, &spare, in, in + 3)
                                   : Radar_ClipEdge(self, &spare, in + k, in + k - 1);
                    if (e != -1) {
                        last = e;
                        break;
                    }
                }
            }
            spare = *at;
            CVector2D* p = at;
            for (int k = last; k != outOf; k = (k + 1) & 3) {
                *p = corners[k];
                count++;
                p++;
            }
            out[count] = spare;
        }
        count++;
    }
    if (count != 0) {
        return count;
    }
    float d1 = in[0].x - in[1].x;
    float d3 = in[0].x - in[3].x;
    float s1 = 0.0f;
    float s3 = 0.0f;
    if (d1 > 0.0f || d1 < 0.0f) {
        s1 = (in[0].y - in[1].y) / d1;
    }
    if (d3 > 0.0f || d3 < 0.0f) {
        s3 = (in[0].y - in[3].y) / d3;
    }
    if (!((s1 * in[3].x - in[3].y) * (s1 * in[0].x - in[0].y) < 0.0f)) {
        return count;
    }
    if (!((s3 * in[1].x - in[1].y) * (s3 * in[0].x - in[0].y) < 0.0f)) {
        return count;
    }
    out[0] = {1.0f, -1.0f};
    out[1] = {1.0f, 1.0f};
    out[2] = {-1.0f, 1.0f};
    out[3] = {-1.0f, -1.0f};
    return 4;
}

// 0x54BD70: a vector's length; on the radar (mode 0) one past the rim brought to it
float __fastcall Radar_LimitToEdge(uint8_t* self, re::Edx, CVector2D* v) {
    float sum = v->x * v->x + v->y * v->y;
    float length = float(std::sqrt(double(std::fabs(sum))));
    if (Mode(self) == 2 || Mode(self) == 1) {
        return length;
    }
    if (length > 1.0f) {
        float inv = 1.0f / length;
        v->x = v->x * inv;
        v->y = inv * v->y;
    }
    return length;
}

// 0x54BE20: a map point (-1..1 round the centre) to the screen: the radar's circle, the
// full map's frame, or the zoomed map (by the world)
void __fastcall Radar_MapToScreen(uint8_t* self, re::Edx, CVector2D* out, const CVector2D* in) {
    int mode = Mode(self);
    if (mode == 2) {
        CVector2D centre;
        Radar_WorldToMap(self, &centre, &At<CVector2D>(self, 0xE740));
        float kx = At<float>(self, 0xE7B0) / At<float>(self, 0xE778);
        float ky = At<float>(self, 0xE7B4) / At<float>(self, 0xE77C);
        out->x = in->x;
        float y = in->y;
        out->y = y;
        out->x = (out->x - centre.x) * kx + 320.0f;
        out->y = 240.0f - (y - centre.y) * ky;
    } else if (mode == 0) {
        float sy = g_screenScaleY;
        out->x = in->x * At<float>(self, 0x134) * g_screenScaleX + At<float>(self, 0xE754);
        out->y = At<float>(self, 0xE758) - in->y * At<float>(self, 0x134) * sy;
    } else if (mode == 1) {
        float sx = g_screenScaleX;
        float sy = g_screenScaleY;
        out->x = in->x;
        out->y = in->y;
        out->x = out->x + At<float>(self, 0xE780);
        out->y = out->y * -1.0f + At<float>(self, 0xE784);
        out->x = out->x - At<float>(self, 0x170) * At<float>(self, 0xE780) * At<float>(self, 0x184);
        out->y = out->y - At<float>(self, 0x170) * At<float>(self, 0xE784) * At<float>(self, 0x188);
        float ky = At<float>(self, 0x180) / (At<float>(self, 0x190) - At<float>(self, 0x188));
        ky = ky / (At<float>(self, 0x170) * At<float>(self, 0xE784));
        float kx = At<float>(self, 0x17C) / (At<float>(self, 0x18C) - At<float>(self, 0x184));
        kx = kx / (At<float>(self, 0x170) * At<float>(self, 0xE780));
        out->x = kx * out->x;
        float x = out->x * sx;
        out->x = x;
        float y = out->y * ky * sy;
        out->y = y;
        out->x = x + At<float>(self, 0x174);
        out->y = y + At<float>(self, 0x178);
    }
}

} // namespace

#define HUDRADAR2_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(hudradar2_original_, __LINE__);                                       \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudradar2_original_, __LINE__), \
             re::Mode::Original)

HUDRADAR2_REVERSED("HUDRadar::DrawCharBlip", 0x00549C00, &Radar_DrawCharBlip_);
HUDRADAR2_REVERSED("HUDRadar::DrawBlipSprite", 0x00549FD0, &Radar_DrawBlipSprite_);
HUDRADAR2_REVERSED("HUDRadar::AddBlip", 0x0054A410, &Radar_AddBlip);
HUDRADAR2_REVERSED("HUDRadar::AddEntityBlip", 0x0054A5B0, &Radar_AddEntityBlip);
HUDRADAR2_REVERSED("HUDRadar::AddCollectibleBlip", 0x0054A700, &Radar_AddCollectibleBlip);
HUDRADAR2_REVERSED("HUDRadar::RemoveBlipsFor", 0x0054A890, &Radar_RemoveBlipsFor);
HUDRADAR2_REVERSED("HUDRadar::RemoveBlip", 0x0054A940, &Radar_RemoveBlip);
HUDRADAR2_REVERSED("HUDRadar::GetArea", 0x0054AA20, &Radar_GetArea_);
HUDRADAR2_REVERSED("HUDRadar::AllocTileSlots", 0x0054AA70, &Radar_AllocTileSlots_);
HUDRADAR2_REVERSED("HUDRadar::GetBlipPosition", 0x0054AC30, &Radar_GetBlipPosition);
HUDRADAR2_REVERSED("HUDRadar::RefreshSeason", 0x0054AD80, &Radar_RefreshSeason);
HUDRADAR2_REVERSED("HUDRadar::AddPulse", 0x0054ADC0, &Radar_AddPulse);
HUDRADAR2_REVERSED("HUDRadar::DrawPulses", 0x0054AE90, &Radar_DrawPulses);
// (register arguments: ecx the triangles, edx the points, edi the count)
RE_UNHOOKED("HUDRadar::InTriangles", 0x0054B0A0);
HUDRADAR2_REVERSED("HUDRadar::IsOnMap", 0x0054B170, &Radar_IsOnMap);
HUDRADAR2_REVERSED("HUDRadar::HUDRadar", 0x0054B280, &Radar_Construct);
HUDRADAR2_REVERSED("HUDRadar::Initialise", 0x0054B3F0, &Radar_Initialise);
HUDRADAR2_REVERSED("HUDRadar::Shutdown", 0x0054B430, &Radar_Shutdown);
HUDRADAR2_REVERSED("HUDRadar::DrawLegend", 0x0054B480, &Radar_DrawLegend);
HUDRADAR2_REVERSED("HUDRadar::GetStorey", 0x0054B8D0, &Radar_GetStorey);
HUDRADAR2_REVERSED("HUDRadar::ClipToFrame", 0x0054B960, &Radar_ClipToFrame);
HUDRADAR2_REVERSED("HUDRadar::LimitToEdge", 0x0054BD70, &Radar_LimitToEdge);
HUDRADAR2_REVERSED("HUDRadar::MapToScreen", 0x0054BE20, &Radar_MapToScreen);
