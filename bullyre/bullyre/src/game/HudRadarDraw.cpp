// The radar (HUD_RADAR) drawn: its map tiles (streamed texture dictionaries, "radarNN"
// by storey), the roads/area outlines, the blips of positions and of entities, the
// vision cones, the full-screen map's scrolling and zoom, Setup and Draw.
// 0x54C050..0x5512A0 (the radar's blips and helpers before it are Hud.cpp's and
// HudRadar's). All thiscall, written as __fastcall (self, edx, ...).
//
// What's used of the radar here, by offset (Hud.h's HUDRadar has most of it unknown):
//   +0x020 [8] blip effects (0x20 bytes: the pulses 0x54ADC0 adds)   +0x120 pulse timer
//   +0x130 cells (the map is cells x cells tiles)                    +0x150 zoom (map)
//   +0x154 / +0x158 zoom max / min                 +0x164.. frame rect (layout)
//   +0x174.. the map's frame, +0x184.. its UVs     +0x194.. the frame's clip rect
//   +0x210 blips[300]                               +0x3A50 zoom (radar)
//   +0x3A54 / 3A58 / 3A5C the tile slots (storeys, ground, winter ground)
//   +0x3A60 [14] sprites                            +0x3B34 [64 x 64] area links
//   +0x7B34 the links' positions (CVector)         +0xE734 the player's storey
//   +0xE738 / E73C sin / cos of the rotation       +0xE740.. the centre (world)
//   +0xE754.. the circle's centre (screen), +0xE75C its radius
//   +0xE764 world size, +0xE76C tile size          +0xE788 mode (0 radar, 1 / 2 map)
//   +0xE7A4 / E7A8 the map's scroll                +0xE7F8 the player's area
//   +0xE813.. blip colours                         +0xE828 winter tiles

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "game/TxdStore.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using vc::At;

struct CVector {
    float x, y, z;
};
struct CVector2D {
    float x, y;
};

// A blip, with what's used here (Hud.h's HUDRadarBlip)
struct RadarBlip {
    int32_t handle;           // +0x00 the entity's pool handle, or a collectible's hash
    float x, y, z;            // +0x04 a position blip's
    int32_t hash;             // +0x10 (a collectible's)
    int32_t generation;       // +0x14
    uint32_t flashUntil;      // +0x18 a time
    float size;               // +0x1C the marker's
    int32_t type;             // +0x20 1 car, 2 char, 3 object, 4 coord, 5, 6 char (vision cone), 7, 0xB pickup, 0xC, 0xD
    int32_t display;          // +0x24 1 blip, 2 marker, 3 both (5 / 4: the same, and more)
    int32_t sprite;           // +0x28 -1: none (the sprite table 0x91A3E8 by it)
    int16_t id;               // +0x2C
    int8_t heightOffset;      // +0x2E (a char's: the icon's)
    uint8_t flags;            // +0x2F 2 in use, 4 only on its storey, 8, 0x10 shown
};
static_assert(sizeof(RadarBlip) == 0x30);
static_assert(offsetof(RadarBlip, sprite) == 0x28);

// The blip sprites' table (0x14 bytes each, from 0x91A3E8)
struct BlipSpriteInfo {
    re::Ptr32<const char> name;         // +0x00 ("MI_mission", ...)
    uint8_t unknown04[8];
    CRGBA colour;             // +0x0C (r, g, b; +0xF: shown with height arrows indoors)
    uint8_t arrowsOutdoors;   // +0x10 (+0x11: shown off its area)
    uint8_t offArea;          // +0x11
    uint8_t unknown12[2];
};
static_assert(sizeof(BlipSpriteInfo) == 0x14);
GAME_ARRAY(uint8_t, g_blipSprites, 0x0091A3E8);
inline uint8_t* SpriteInfo(int sprite) {
    return g_blipSprites + sprite * 0x14;
}

// The areas' table (0x20 bytes each; +0x18 the kind: 0 none, 4 storeys, 6 ...)
GAME_ARRAY(uint8_t, g_radarAreas, 0x00CF01E0);
inline int32_t AreaKind(int area) {
    return *reinterpret_cast<int32_t*>(g_radarAreas + area * 0x20 + 0x20);
}

// the area's angle (the map turned by it)
inline float AreaAngle(int area) {
    return *reinterpret_cast<float*>(g_radarAreas + area * 0x20 + 0x24);
}

inline RadarBlip& Blip(void* radar, int index) {
    return At<RadarBlip>(radar, 0x210 + index * 0x30);
}
inline int32_t& Mode(void* radar) {
    return At<int32_t>(radar, 0xE788);
}
inline int32_t& Cells(void* radar) {
    return At<int32_t>(radar, 0x130);
}
inline bool IsMap(void* radar) {
    return Mode(radar) == 2 || Mode(radar) == 1;
}

// ---- the game's ------------------------------------------------------------------------
GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(uint8_t, g_chapter, 0x00A147E0);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(uint32_t, g_fanColour, 0x020DCCD0);
GAME_VAR(void*, g_renderer, 0x020E2380);
GAME_VAR(uint32_t, g_mapFillColour, 0x00CF09FC);
inline void* const kCollectibles = reinterpret_cast<void*>(0x00BCEB50);

GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, Im2D_SetTexture, 0x00738C00, void* texture);
GAME_FN(void, __cdecl, Im2D_BeginFan, 0x00738C30, int count);
GAME_FN(void, __cdecl, Im2D_BeginList, 0x00738C90, int count);
GAME_FN(void, __cdecl, Im2D_Vertex, 0x00738CC0, float x, float y, uint32_t z);
GAME_FN(void, __cdecl, Im2D_SetUV, 0x00738D40, float u, float v);
GAME_FN(void, __cdecl, Im2D_EndVertex, 0x00738D80);
GAME_FN(void, __cdecl, Im2D_Draw, 0x0073A300);
GAME_FN(void, __thiscall, NiCamera_Construct, 0x00763C20, void* camera);
GAME_FN(void, __thiscall, NiCamera_Destruct, 0x00762D50, void* camera);
GAME_FN(void, __thiscall, Renderer_GetCamera, 0x00755D50, void* renderer, void* camera);
GAME_FN(void*, __cdecl, Dictionary_FindTexture, 0x005F1610, void* dict, const char* name);
GAME_FN(void, __cdecl, ForAllTextures, 0x005F1590, void* dict, void* callback, void* data);
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, void* object);
GAME_FN(uint8_t, __cdecl, IsModelLoaded, 0x004151E0, int model);
GAME_FN(void, __cdecl, RemoveTxdModel, 0x00530860, int txd);
GAME_FN(int16_t, __thiscall, Collectibles_FindByHash, 0x004263C0, void* mgr, int32_t hash);
GAME_FN(uint8_t, __thiscall, Collectibles_IsCollected, 0x00426340, void* mgr, int index);
GAME_FN(void, __cdecl, PlaceMarker, 0x0052A600, int type, int blipSprite, int blipArg, CVector* pos, float size,
        const CVector* dir, int blipId, int red, int green, int blue, uint32_t id, int attach, int blipType);
GAME_FN(CVector*, __cdecl, FindPlayerPosition, 0x00446A40);
inline void* const kTextureInUseCallback = reinterpret_cast<void*>(0x006CA7D0);
GAME_FN(CRGBA*, __cdecl, LerpColour, 0x00551530, CRGBA* out, float t, CRGBA from, CRGBA to);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(void, __thiscall, Vector2D_Normalise, 0x00443730, CVector2D* v);
GAME_FN(void, __cdecl, Draw2DPolygonB, 0x0052C310, float x1, float y1, float x2, float y2, float x3, float y3, float x4,
        float y4, const CRGBA* colour);
GAME_FN(void, __cdecl, Im2D_Flush, 0x0052C5C0);
GAME_FN(void, __cdecl, DrawSprite, 0x0052D610, const col::CRect* rect, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, FE_DrawPanel, 0x006A4420, float x1, float y1, float x2, float y2, float alpha, CRGBA* colourA,
        CRGBA* colourB);
GAME_FN(void, __cdecl, PushMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopMemoryTag, 0x0052E710);
GAME_FN(void, __cdecl, Game_TidyUpMemory, 0x004302A0, int, int);
GAME_FN(void, __cdecl, Streaming_Update, 0x005346B0);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int);
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, Txd_LoadTxdFile, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_RemoveRef, 0x005F1160, int slot);
GAME_FN(void, __cdecl, Txd_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __thiscall, Sprite_SetTexture, 0x0052C230, void* sprite, const char* name);
GAME_FN(void, __thiscall, Sprite_Delete, 0x0052C6D0, void* sprite);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int control, int pad);
GAME_FN(void, __cdecl, FE_PlaySelect, 0x006A2360);
GAME_FN(void, __cdecl, FE_PlayScroll, 0x006A23A0);
GAME_FN(uint8_t, __cdecl, FE_PadPressed, 0x006A3170, int pad);
GAME_FN(void, __cdecl, FE_PadClear, 0x006A3120, int pad);
GAME_VAR(uint8_t, g_padDown, 0x020C3274);
GAME_VAR(uint8_t, g_padUp, 0x020C3278);
GAME_ARRAY(uint8_t, g_mapLegend, 0x00AB733C);  // [10] 8 bytes: shown, can be toggled
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(col::CRect*, __cdecl, ScreenRect, 0x0052D650, col::CRect* out, float x, float y, float width, float height,
        int hAlign, int vAlign);
GAME_FN(void, __thiscall, HUDComponent_SetAlignment, 0x0053CE00, HUDComponent* self, const char* h, const char* v);
GAME_FN(float, __cdecl, FindPlayerHeading, 0x00446A80);
GAME_FN(CVector*, __cdecl, FindPlayerSpeed, 0x00445D60);
GAME_FN(void*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, RemoveModel, 0x0052F640, int id);
GAME_FN(void*, __thiscall, Zones_GetByName, 0x00431740, void* zones, const char* name);
GAME_FN(uint8_t, __thiscall, Zone_IsPointInside, 0x006D6EB0, void* zone, const float* xy);
GAME_FN(uint8_t, __thiscall, MissionMgr_6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionMgr_InClass, 0x006AA840, void* mgr);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetStats, 0x006AA7C0, void* mgr, int id);  // (+4 flags)
inline void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const kZones = reinterpret_cast<void*>(0x020C7E98);
inline void* const kSchedule = reinterpret_cast<void*>(0x020C35E8);
GAME_VAR(int32_t, g_widescreen, 0x00C3CD0C);        // (nothing of the radar drawn while set)
GAME_VAR(float, g_cameraHeading, 0x00C3CD08);
GAME_VAR(void*, g_northTexture, 0x00C66F50);
GAME_VAR(void*, g_radarRing, 0x00C66F38);
GAME_VAR(float, g_halfPi, 0x00AB7388);
GAME_VAR(uint8_t*, g_gameMode, 0x020C8704);         // (+4 active, +0x3C the mode, +0x50C)
GAME_VAR(uint8_t*, g_classSkip, 0x00C0F78C);        // (+2: vision cones shown)
GAME_VAR(uint8_t*, g_showVisionCones, 0x00C2B034);  // (+1)
GAME_VAR(int32_t, g_flashExemptModel, 0x00BF42DC);
GAME_ARRAY(void*, g_modelInfos, 0x00C67738);
// entities
GAME_FN(uint8_t*, __cdecl, GetVehicleFromHandle, 0x0044A580, int handle);
GAME_FN(uint8_t*, __cdecl, GetPedFromHandle, 0x0044A550, int handle);
GAME_FN(uint8_t*, __cdecl, Pools_GetObject, 0x0044A5B0, int handle);
GAME_FN(uint8_t*, __cdecl, Pools_Get44A5E0, 0x0044A5E0, int handle);
GAME_FN(int, __cdecl, Pickups_Find, 0x004438B0, int handle);  // -1: none
GAME_FN(uint8_t*, __cdecl, GetColModel, 0x0051AF50, void* entity);
GAME_FN(uint8_t*, __thiscall, ModelInfo_5201D0, 0x005201D0, void* modelInfo);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, void* entity, int node, void* transform, int flag);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const CVector* v);
GAME_FN(uint8_t, __thiscall, PedScanner_IsAroundAndSeen, 0x0048C140, void* scanner, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_Unknown473FB0, 0x00473FB0, void* ped);
GAME_FN(void*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, ObjectiveQueue_GetCurrent, 0x00470340, void* queue);
GAME_FN(CVector*, __thiscall, PedLook_GetDir, 0x0048CEE0, void* self, CVector* out);
GAME_FN(CVector*, __thiscall, PedAi_48D020, 0x0048D020, void* self, CVector* out);
GAME_FN(uint8_t, __thiscall, PedFriends_485590, 0x00485590, void* self, void* ped);
GAME_FN(int32_t*, __thiscall, Ped_475400, 0x00475400, void* ped, int32_t* out, void* other);
GAME_FN(uint8_t, __thiscall, Ped_WantsGift, 0x00474A30, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_496F30, 0x00496F30, void* ped, void* other);
GAME_FN(uint8_t, __thiscall, Schedule_6A8D20, 0x006A8D20, void* schedule);

// the radar's own (Hud.cpp's, HudRadar's and these), where modes apply
GAME_FN(void, __thiscall, Radar_WorldToMap, 0x00549A90, void* radar, CVector2D* out, const CVector2D* in);
GAME_FN(void, __thiscall, Radar_MapToScreen, 0x0054BE20, void* radar, CVector2D* out, const CVector2D* in);
GAME_FN(void, __thiscall, Radar_MapToWorld, 0x00549B80, void* radar, CVector2D* out, const CVector2D* in);
GAME_FN(void, __thiscall, Radar_TileUV, 0x00548790, void* radar, CVector2D* out, const CVector2D* in, int x, int y);
GAME_FN(void, __thiscall, Radar_TileCorners, 0x005486D0, void* radar, float x, float y, CVector2D* out);
GAME_FN(int, __thiscall, Radar_ClipToFrame, 0x0054B960, void* radar, CVector2D* out, const CVector2D* in);
GAME_FN(float, __thiscall, Radar_LimitToEdge, 0x0054BD70, void* radar, CVector2D* v);
GAME_FN(void, __thiscall, Radar_DrawBlipSprite, 0x00549FD0, void* radar, int sprite, float x, float y, int alpha,
        int arrow, float scale);
GAME_FN(void, __thiscall, Radar_DrawCharBlip, 0x00549C00, void* radar, float x, float y, float height, int red,
        int green, int blue, int alpha, int arrow);
GAME_FN(int, __cdecl, Radar_GetArea, 0x0054AA20, CVector pos);
GAME_FN(int, __thiscall, Radar_GetStorey, 0x0054B8D0, void* radar, CVector pos);
GAME_FN(uint8_t, __thiscall, Radar_IsOnMap, 0x0054B170, void* radar, float x, float y);
GAME_FN(void, __thiscall, Radar_AddPulse, 0x0054ADC0, void* radar, float x, float y);
GAME_FN(void, __thiscall, Radar_LoadTile, 0x00549740, void* radar, int x, int y);
GAME_FN(int, __thiscall, Radar_GetTxdSlot, 0x00548E60, void* radar, int tile, int storeys);
GAME_FN(uint8_t, __thiscall, Radar_IsIndoors, 0x00548F60, void* radar);
GAME_FN(void, __thiscall, Radar_RemoveBlipsFor, 0x0054A890, void* radar, int type, int handle);
GAME_FN(void*, __thiscall, Radar_GetTileDict_, 0x0054C500, void* radar, int tile, int storeys);
GAME_FN(CVector*, __thiscall, Radar_ToPlayersArea_, 0x0054C0F0, void* radar, CVector* out, CVector pos);
GAME_FN(void, __thiscall, Radar_DrawPolys_, 0x0054C570, void* radar, int unused, const CVector2D* points, int count,
        const int32_t* triangles, const uint32_t* colour);
GAME_FN(void, __thiscall, Radar_DrawTile_, 0x0054CA10, void* radar, int x, int y);
GAME_FN(void, __thiscall, Radar_UnloadTile_, 0x0054D1F0, void* radar, int x, int y);
GAME_FN(void, __thiscall, Radar_ClampToCircle_, 0x0054D620, void* radar, float* x, float* y);
GAME_FN(void, __thiscall, Radar_StreamTiles_, 0x0054D700, void* radar, int x, int y);
GAME_FN(void, __thiscall, Radar_DrawCone_, 0x0054D860, void* radar, float x, float y, float angle, float range,
        float width, float dirX, float dirY, float dirZ, int alert);
GAME_FN(void, __thiscall, Radar_UpdateScale_, 0x0054E200, void* radar);
GAME_FN(void, __thiscall, Radar_DrawMask_, 0x0054CF10, void* radar);
GAME_FN(void, __thiscall, Radar_DrawOutlines_, 0x0054C8F0, void* radar);
GAME_FN(void, __thiscall, Radar_DrawCollectibleBlip_, 0x0054C050, void* radar, int index);
GAME_FN(void, __thiscall, Radar_DrawCoordBlip_, 0x0054D290, void* radar, int index);
GAME_FN(void, __thiscall, Radar_DrawEntityBlip_, 0x0054EE80, void* radar, int index);
GAME_FN(void, __thiscall, Radar_DrawTileWhole_, 0x0054CD10, void* radar, int x, int y);
GAME_FN(void, __thiscall, Radar_MapControls_, 0x0054EB10, void* radar);
GAME_FN(void, __thiscall, Radar_DrawTiles_, 0x0054E560, void* radar);
GAME_FN(void, __thiscall, Radar_DrawMap_, 0x0054E6E0, void* radar);
GAME_FN(void, __thiscall, Radar_DrawBlips_, 0x005500D0, void* radar);
GAME_FN(void, __thiscall, Radar_DrawMapTiles_, 0x00550A70, void* radar);
GAME_FN(void, __thiscall, Radar_Update_, 0x00550EC0, void* radar);
GAME_FN(void, __thiscall, Radar_UpdateRotation, 0x00548960, void* radar);
GAME_FN(void, __thiscall, Radar_DrawArrow, 0x00549560, void* radar, void* sprite, float x, float y, float angle, int alpha);
GAME_FN(void, __thiscall, Radar_DrawPulses, 0x0054AE90, void* radar);
GAME_FN(void, __thiscall, Radar_DrawLegend, 0x0054B480, void* radar);

// (an NiCamera is 0x13C bytes: its constructor writes up to +0x138, the game news 0x13C. The
// 0x12C this once had let the viewport and LOD fields run over the clipped tile beside it:
// the radar's torn, missing tiles)
struct NiCameraOnStack {
    alignas(8) uint8_t bytes[0x13C];  // (+0x118: the far plane)
    float Far() const { return *reinterpret_cast<const float*>(bytes + 0x118); }
};

} // namespace

namespace hudradar {

// ---- blips and tiles ---------------------------------------------------------------------

// 0x54C050: a collectible's blip, while it's still to be collected
void __fastcall DrawCollectibleBlip(uint8_t* self, re::Edx, int index) {
    RadarBlip& blip = Blip(self, index);
    int16_t slot = Collectibles_FindByHash(kCollectibles, blip.hash);
    if (!Collectibles_IsCollected(kCollectibles, slot)) {
        CVector2D pos = {blip.x, blip.y};
        CVector2D map;
        Radar_WorldToMap(self, &map, &pos);
        Radar_MapToScreen(self, &pos, &map);
        Radar_DrawBlipSprite(self, blip.sprite, pos.x, pos.y, 0xFF, 2, 1.0f);
    }
}

// 0x54C0F0: a position as it shows from the player's area: through the link between
// the two areas (the map: the area's own link) when there is one
CVector* __fastcall ToPlayersArea(uint8_t* self, re::Edx, CVector* out, CVector pos) {
    int area = Radar_GetArea(pos);
    if (IsMap(self)) {
        if (AreaKind(area) != 0 && AreaKind(area) != 6) {
            int link = At<int32_t>(self, 0x3B34 + area * 4);
            *out = At<CVector>(self, 0x7B34 + link * 0xC);
            return out;
        }
    } else {
        CVector player = At<CVector>(self, 0xE748);
        int playerArea = Radar_GetArea(player);
        if (playerArea != area) {
            int link = At<int32_t>(self, 0x3B34 + (playerArea * 0x40 + area) * 4) + playerArea * 0x24;
            *out = At<CVector>(self, 0x7B34 + link * 0xC);
            return out;
        }
    }
    *out = pos;
    return out;
}

// 0x54C290: the blips of doors and the like (sprites 0, 1, 0x11, 0x1A, 0x1D, 0x1E,
// 0x22) flash for 2.5 seconds
void __fastcall FlashDoorBlips(uint8_t* self, re::Edx) {
    for (int i = 0; i < 300; i++) {
        RadarBlip& blip = Blip(self, i);
        if (!(blip.flags & 2)) {
            continue;
        }
        int sprite = blip.sprite;
        if (sprite != 0 && sprite != 1 && sprite != 0x11 && sprite != 0x1A && sprite != 0x1D && sprite != 0x1E &&
            sprite != 0x22) {
            continue;
        }
        uint32_t handle = uint32_t(blip.generation) << 12 | uint32_t(i);
        if (handle == 0xFFFFFFFFu) {
            continue;
        }
        uint32_t index = handle & 0xF0000FFFu;
        RadarBlip& named = *reinterpret_cast<RadarBlip*>(self + 0x210 + uint32_t(index * 0x30u));
        if ((uint32_t(named.generation) << 12 | index) == handle && index != 0xFFFFFFFFu) {
            named.flashUntil = g_timeInMilliseconds + 2500;
        }
    }
}

// 0x54C500: a tile's texture dictionary (storeys: the storey tiles', else the ground's,
// two tiles to a slot); nullptr when it has none
void* __fastcall GetTileDict(uint8_t* self, re::Edx, int tile, int storeys) {
    int32_t slot;
    if (re::Flag(storeys)) {
        slot = At<int32_t*>(self, 0x3A54)[tile];
    } else {
        int32_t* slots = At<int32_t>(self, 0xE828) == 0 ? At<int32_t*>(self, 0x3A58) : At<int32_t*>(self, 0x3A5C);
        slot = slots[tile >> 1];
    }
    if (slot == -1) {
        return nullptr;
    }
    // (a free slot reads through nullptr, as the game's does)
    return CTxdStore::ms_pTxdPool->GetAt(slot)->dict;
}

// 0x54C570: triangles of the map's outlines in one colour (clamped to the frame on the
// map). `unused`: the table's count, not read
void __fastcall DrawPolys(uint8_t* self, re::Edx, int, const CVector2D* points, int count, const int32_t* triangles,
                          const uint32_t* colour) {
    g_fanColour = *colour;
    Im2D_SetTexture(nullptr);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    Im2D_BeginList(count * 3);
    bool clamp = Mode(self) == 1 || Mode(self) == 2;
    for (int i = 0; i < count; i++) {
        for (int corner = 0; corner < 3; corner++) {
            const CVector2D& point = points[triangles[i * 3 + corner]];
            CVector2D world = {point.x + 0.0f, point.y + 0.0f};
            CVector2D map;
            Radar_WorldToMap(self, &map, &world);
            CVector2D screen;
            Radar_MapToScreen(self, &screen, &map);
            if (clamp) {
                float x = At<float>(self, 0x194);
                if (!(x > screen.x)) {
                    x = screen.x;
                    if (At<float>(self, 0x19C) < screen.x) {
                        x = At<float>(self, 0x19C);
                    }
                }
                screen.x = x;
                float y = At<float>(self, 0x198);
                if (!(y > screen.y)) {
                    y = screen.y;
                    if (At<float>(self, 0x1A0) < screen.y) {
                        y = At<float>(self, 0x1A0);
                    }
                }
                screen.y = y;
            }
            Im2D_Vertex(screen.x, screen.y, 0);
            Im2D_EndVertex();
        }
    }
    Im2D_Draw();
}

// the outline tables (count, points, triangle count, triangles)
struct PolyTable {
    uintptr_t count, points, triangleCount, triangles;
};
void DrawTable(uint8_t* self, const PolyTable& t, const uint32_t* colour) {
    Radar_DrawPolys_(self, *reinterpret_cast<int32_t*>(t.count), reinterpret_cast<const CVector2D*>(t.points),
                     *reinterpret_cast<int32_t*>(t.triangleCount), reinterpret_cast<const int32_t*>(t.triangles), colour);
}

// 0x54C8F0: the map's outlines: the two always there, then the chapter's and the
// later ones (the switch falls through)
void __fastcall DrawOutlines(uint8_t* self, re::Edx) {
    uint8_t chapter = g_chapter;
    if (Mode(self) == 0) {
        return;
    }
    uint32_t colour = g_mapFillColour;
    DrawTable(self, {0xAB6214, 0xAB6218, 0xAB6558, 0xAB6560}, &colour);
    DrawTable(self, {0xAB655C, 0xAB6A28, 0xAB6D60, 0xAB6D68}, &colour);
    switch (chapter) {
    case 0:
        DrawTable(self, {0xAB5958, 0xAB5960, 0xAB595C, 0xAB5CE0}, &colour);
        [[fallthrough]];
    case 1:
        DrawTable(self, {0xAB4DB4, 0xAB5128, 0xAB5124, 0xAB5478}, &colour);
        [[fallthrough]];
    case 2:
        DrawTable(self, {0xAB4974, 0xAB4978, 0xAB4A38, 0xAB4A40}, &colour);
        [[fallthrough]];
    case 3:
        DrawTable(self, {0xAB4A3C, 0xAB4B48, 0xAB4DB0, 0xAB4DB8}, &colour);
        break;
    default:
        break;
    }
}

// 0x54CA10: a tile of the radar, clipped to its frame (the storey's tiles indoors)
void __fastcall DrawTile(uint8_t* self, re::Edx, int x, int y) {
    CVector2D corners[8];
    Radar_TileCorners(self, float(x), float(y), corners);
    if (x < 0) {
        x = 0;
    }
    int last = Cells(self) - 1;
    if (x > last) {
        x = last;
    }
    if (y < 0) {
        y = 0;
    }
    if (y > last) {
        y = last;
    }
    int tile = Cells(self) * y + x;
    int storey = At<int32_t>(self, 0xE734);
    if (At<uint8_t>(self, 0xE810) && storey != 4) {
        storey = At<int32_t>(self, 0xE7FC);
    }
    char name[32];  // (left as it was, uninitialised, for a storey past 3)
    switch (storey) {
    case 0:
        std::sprintf(name, At<int32_t>(self, 0xE828) == 1 ? "winradar%02d" : "radar%02d", tile);
        break;
    case 1:
        std::sprintf(name, "1stradar%02d", tile);
        break;
    case 2:
        std::sprintf(name, "2ndradar%02d", tile);
        break;
    case 3:
        std::sprintf(name, "3rdradar%02d", tile);
        break;
    default:
        break;
    }
    void* dict = Radar_GetTileDict_(self, tile, storey != 0);
    if (!dict) {
        return;
    }
    void* texture = Dictionary_FindTexture(dict, name);
    if (!texture) {
        return;
    }
    CVector2D uv[8];
    for (int i = 0; i < 4; i++) {
        Radar_WorldToMap(self, &uv[i], &corners[i]);
    }
    CVector2D clipped[8];
    int count = Radar_ClipToFrame(self, clipped, uv);
    if (count == 0) {
        NiRelease(texture);
        return;
    }
    Im2D_SetTexture(texture);
    g_fanColour = 0xFFFFFFFFu;
    Im2D_BeginFan(count);
    NiCameraOnStack camera;
    NiCamera_Construct(camera.bytes);
    Renderer_GetCamera(g_renderer, camera.bytes);
    float half = float(double(camera.Far()) * 0.5);
    CVector2D screen[8];
    for (int i = 0; i < count; i++) {
        uint32_t z = uint32_t(int32_t(half));
        Radar_MapToWorld(self, &corners[i], &clipped[i]);
        Radar_TileUV(self, &uv[i], &corners[i], x, y);
        Radar_MapToScreen(self, &screen[i], &clipped[i]);
        Im2D_SetUV(uv[i].x, uv[i].y);
        Im2D_Vertex(screen[i].x, screen[i].y, z);
        Im2D_EndVertex();
    }
    Im2D_Draw();
    NiRelease(texture);
    NiCamera_Destruct(camera.bytes);
}

// 0x54CD10: a ground tile whole (the map's: no clipping)
void __fastcall DrawTileWhole(uint8_t* self, re::Edx, int x, int y) {
    if (x < 0) {
        x = 0;
    }
    int last = Cells(self) - 1;
    if (x > last) {
        x = last;
    }
    if (y < 0) {
        y = 0;
    }
    if (y > last) {
        y = last;
    }
    CVector2D corners[4];
    Radar_TileCorners(self, float(x), float(y), corners);
    int tile = Cells(self) * y + x;
    int32_t* slots = At<int32_t>(self, 0xE828) == 0 ? At<int32_t*>(self, 0x3A58) : At<int32_t*>(self, 0x3A5C);
    int32_t slot = slots[tile >> 1];
    if (slot == -1) {
        return;
    }
    void* dict = CTxdStore::ms_pTxdPool->GetAt(slot)->dict;
    if (!dict) {
        return;
    }
    char name[32];
    std::sprintf(name, At<int32_t>(self, 0xE828) == 1 ? "winradar%02d" : "radar%02d", tile);
    void* texture = Dictionary_FindTexture(dict, name);
    if (!texture) {
        return;
    }
    CVector2D map[4];
    for (int i = 0; i < 4; i++) {
        Radar_WorldToMap(self, &map[i], &corners[i]);
    }
    const CVector2D uv[4] = {{0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}};
    Im2D_SetTexture(texture);
    g_fanColour = 0xFFFFFFFFu;
    Im2D_BeginFan(4);
    for (int i = 0; i < 4; i++) {
        CVector2D screen;
        Radar_MapToScreen(self, &screen, &map[i]);
        Im2D_SetUV(uv[i].x, uv[i].y);
        Im2D_Vertex(screen.x, screen.y, 0xFFF);
        Im2D_EndVertex();
    }
    Im2D_Draw();
    NiRelease(texture);
}

// 0x54CF10: the radar's round mask: a disc at the far plane (transparent), and the
// four corners outside it filled nearer (so the tiles show only in the circle)
void __fastcall DrawMask(uint8_t* self, re::Edx) {
    const CVector2D corners[4] = {{1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}, {-1.0f, -1.0f}};
    SetRenderState(1, 0);
    SetRenderState(10, 5);
    SetRenderState(0xB, 6);
    SetRenderState(0xE, 0);
    SetRenderState(9, 2);
    SetRenderState(7, 1);
    SetRenderState(6, 1);
    SetRenderState(8, 1);
    SetRenderState(0xC, 1);
    CVector2D centre = {0.0f, 0.0f};
    CVector2D screen;
    Radar_MapToScreen(self, &screen, &centre);
    Im2D_SetTexture(nullptr);
    g_fanColour = 0;
    Im2D_BeginFan(0x20);
    NiCameraOnStack camera;
    NiCamera_Construct(camera.bytes);
    Renderer_GetCamera(g_renderer, camera.bytes);
    uint32_t z = uint32_t(int32_t(camera.Far()));
    Im2D_Vertex(screen.x, screen.y, z);
    Im2D_EndVertex();
    CVector2D rim[0x1F];
    for (int i = 0; i < 0x1F; i++) {
        float angle = float(double(i) * double(6.2831855f) / 30.0);
        CVector2D point = {float(std::cos(angle)), float(std::sin(angle))};
        Radar_MapToScreen(self, &rim[i], &point);
        Im2D_Vertex(rim[i].x, rim[i].y, z);
        Im2D_EndVertex();
    }
    Im2D_Draw();
    for (const CVector2D& corner : corners) {
        CVector2D point = corner;
        Radar_MapToScreen(self, &screen, &point);
        Im2D_BeginFan(8);
        Im2D_Vertex(screen.x, screen.y, 0);
        Im2D_EndVertex();
        for (int i = 0; i < 7; i++) {
            float angle = float(double(i) * double(1.5707964f) / 6.0);
            point.x = float(std::cos(angle)) * corner.x;
            point.y = float(std::sin(angle)) * corner.y;
            Radar_MapToScreen(self, &rim[i], &point);
            Im2D_Vertex(rim[i].x, rim[i].y, 0);
            Im2D_EndVertex();
        }
        Im2D_Draw();
    }
    NiCamera_Destruct(camera.bytes);
}

// 0x54D1F0: a tile's dictionary let go, when none of its textures is in use
void __fastcall UnloadTile(uint8_t* self, re::Edx, int x, int y) {
    if (x < 0 || x > Cells(self) - 1 || y < 0 || y > Cells(self) - 1) {
        return;
    }
    int tile = Cells(self) * y + x;
    uint8_t storeys = Radar_IsIndoors(self);
    if (void* dict = Radar_GetTileDict_(self, tile, storeys)) {
        uint8_t unused = 1;  // (the callback clears it for a texture in use)
        ForAllTextures(dict, kTextureInUseCallback, &unused);
        if (!unused) {
            return;
        }
    }
    int slot = Radar_GetTxdSlot(self, tile, storeys);
    if (IsModelLoaded(slot + 0x2FA8)) {
        RemoveTxdModel(slot);
    }
}

// 0x54D290: a position blip (a marker in the world for display 2/4/5, the sprite for
// 1/3/4/5), with the arrow for above / below
void __fastcall DrawCoordBlip(uint8_t* self, re::Edx, int index) {
    RadarBlip& blip = Blip(self, index);
    if (!(blip.flags & 0x10)) {
        return;
    }
    if (Mode(self) != 0 && blip.sprite != 0 && blip.sprite != 0x1D && !Radar_IsOnMap(self, blip.x, blip.y)) {
        return;
    }
    CVector pos = {blip.x, blip.y, blip.z};
    int area = Radar_GetArea(pos);
    CVector shown;
    shown = *Radar_ToPlayersArea_(self, &shown, pos);
    CVector2D screen = {shown.x, shown.y};
    // (the colour's alpha from 0xE81E; the r, g, b bytes go as the low bytes of three
    // dwords, whose upper bytes are the next ones on the game's stack)
    uint8_t colour[8] = {};
    std::memcpy(colour, self + 0xE81B, 4);
    if (blip.sprite != -1) {
        colour[0] = SpriteInfo(blip.sprite)[0xC];
        colour[1] = SpriteInfo(blip.sprite)[0xD];
        colour[2] = SpriteInfo(blip.sprite)[0xE];
    }
    if ((blip.display == 5 || blip.display == 2 || blip.display == 4) && area == At<int32_t>(self, 0xE7F8)) {
        CVector dir = {0.0f, 0.0f, 0.0f};
        CVector at = {screen.x, screen.y, shown.z};
        int32_t r, g, b;
        std::memcpy(&r, colour + 0, 4);
        std::memcpy(&g, colour + 1, 4);
        std::memcpy(&b, colour + 2, 4);
        PlaceMarker(2, -1, 0, &at, blip.size, &dir, -1, r, g, b, 0, 1, 5);
    }
    if (blip.display != 5 && blip.display != 1 && blip.display != 4 && blip.display != 3) {
        return;
    }
    CVector2D map;
    Radar_WorldToMap(self, &map, &screen);
    float distance = Radar_LimitToEdge(self, &map);
    Radar_MapToScreen(self, &screen, &map);
    bool offEdge = 1.0f < distance && Mode(self) == 0;
    bool shownNow = true;
    if (blip.type != 6 && blip.flashUntil > g_timeInMilliseconds && Mode(self) == 0 &&
        ((blip.flashUntil - g_timeInMilliseconds) / 250 & 1) == 0) {
        shownNow = false;
    }
    float blipZ = shown.z;
    CVector player = *FindPlayerPosition();
    int storey = Radar_GetStorey(self, shown);
    if ((blip.flags & 4) && (offEdge || At<int32_t>(self, 0xE7F8) != area || storey != At<int32_t>(self, 0xE734)) &&
        Mode(self) == 0) {
        return;
    }
    int arrow = 2;
    int playerStorey = At<int32_t>(self, 0xE734);
    int sprite = blip.sprite;
    if (playerStorey != 0 && Mode(self) == 0) {
        if (SpriteInfo(sprite)[0xF]) {
            if (storey < playerStorey) {
                arrow = 1;
            } else if (storey > playerStorey) {
                arrow = 0;
            }
        }
    } else if (SpriteInfo(sprite)[0x10]) {
        if (player.z < blipZ - 4.5) {
            arrow = 0;
        } else if (blipZ + 4.5 < player.z) {
            arrow = 1;
        }
    }
    if (!shownNow) {
        return;
    }
    if (sprite == 0x12) {
        Radar_AddPulse(self, screen.x, screen.y);
    } else {
        Radar_DrawBlipSprite(self, sprite, screen.x, screen.y, 0xFF, arrow, 1.0f);
    }
    if (blip.sprite == 8 && area != 2) {
        Radar_AddPulse(self, screen.x, screen.y);
    }
}

// 0x54D620: a point outside the circle (by more than its radius less 5) pulled onto it
void __fastcall ClampToCircle(uint8_t* self, re::Edx, float* x, float* y) {
    float px = *x;
    float py = *y;
    float cx = At<float>(self, 0xE754);
    float cy = At<float>(self, 0xE758);
    CVector2D d = {px - cx, py - cy};
    float lengthSq = d.y * d.y + d.x * d.x;
    lengthSq = std::fabs(lengthSq);
    float length = float(std::sqrt(double(lengthSq)));
    if (double(At<float>(self, 0xE75C)) - 5.0 < double(length)) {
        Vector2D_Normalise(&d);
        float scaleY = g_screenScaleY;
        *x = d.x * At<float>(self, 0xE75C) * g_screenScaleX + At<float>(self, 0xE754);
        *y = d.y * At<float>(self, 0xE75C) * scaleY + At<float>(self, 0xE758);
    }
}

// 0x54D700: the tiles round (x, y) loaded, the rest let go (the radar: the 3x3 and
// the next column each side; the map and indoors: a square of the zoom's size)
void __fastcall StreamTiles(uint8_t* self, re::Edx, int x, int y) {
    int reach = 1;
    if (Mode(self) == 0) {
        if (At<int32_t>(self, 0xE734) == 0) {
            for (int i = 0; i < Cells(self); i++) {
                for (int j = 0; j < Cells(self); j++) {
                    if (i < x - 1 || x + 1 < i || j < y - 1 || y + 1 < j) {
                        if ((i != x - 2 && i != x + 2) || j < y - 1 || y + 1 < j) {
                            Radar_UnloadTile_(self, i, j);
                        }
                    } else {
                        Radar_LoadTile(self, i, j);
                    }
                }
            }
            return;
        }
    } else {
        // (the tiles across half the screen at the map's scale, and 3 more)
        reach = int32_t(640.0 / double(At<float>(self, 0xE7B0)) * 0.5 + 3.0);
    }
    for (int i = 0; i < Cells(self); i++) {
        for (int j = 0; j < Cells(self); j++) {
            if (i < x - reach || x + reach < i || j < y - reach || y + reach < j) {
                Radar_UnloadTile_(self, i, j);
            } else {
                Radar_LoadTile(self, i, j);
            }
        }
    }
}

// 0x54D860: a char's vision cone: a fan of 12 triangles, `width` wide (radians) and
// `range` long, pointing along (dirX, -dirY) turned with the radar, each point pulled
// into the circle; green, yellow to red by the player's trouble (alert 1), or red.
// (the third argument and dirZ are not read)
void __fastcall DrawCone(uint8_t* self, re::Edx, float x, float y, float, float range, float width, float dirX, float dirY,
                         float, int alert) {
    if (At<uint8_t>(self, 0xE7F4)) {
        return;
    }
    CRGBA colours[3] = {{0x22, 0x5D, 0x42, 0xC0}, {200, 200, 0x32, 0xC0}, {0x73, 0x20, 0x20, 0xC0}};
    const CRGBA* colour = &colours[alert];
    CRGBA lerped;
    if (alert == 1) {
        float t = float(double(*reinterpret_cast<int32_t*>(g_playerPed + 0x1D40)) / 300.0);
        if (t < 0.0f) {
            t = 0.0f;
        } else if (1.0f < t) {
            t = 1.0f;
        }
        CRGBA out;
        lerped = *LerpColour(&out, t, colours[2], colours[1]);
        colour = &lerped;
    }
    float step = float(double(width) / 12.0);
    float scale = 0.0f;
    bool scaled = true;
    switch (Mode(self)) {
    case 2:
        scale = At<float>(self, 0xE770) / At<float>(self, 0xE764);
        break;
    case 0:
        scale = At<int32_t>(self, 0xE734) == 0 ? At<float>(self, 0xE764) / At<float>(self, 0xE770)
                                                : At<float>(self, 0xE75C) / At<float>(self, 0x3A50);
        break;
    case 1:
        scale = At<float>(self, 0xE7C0) / At<float>(self, 0xE764);
        break;
    default:
        scaled = false;
        break;
    }
    if (scaled) {
        range = scale * range;
    }
    CVector dir = {dirX, float(double(dirY) * -1.0), 0.0f};
    Vector_Normalise(&dir);
    float s = At<float>(self, 0xE738);
    float c = At<float>(self, 0xE73C);
    float fx = c * dir.x - dir.y * s;
    float fy = c * dir.y + dir.x * s;
    fx = fx * range;
    fy = fy * range;
    // the angles -6..-1, +1..+6 steps (their cos and sin)
    float cosA[12], sinA[12];
    for (int k = 1; k <= 6; k++) {
        float a = float(double(k) * double(step));
        cosA[5 + k] = float(std::cos(double(a)));
        sinA[5 + k] = float(std::sin(double(a)));
        a = -a;
        cosA[6 - k] = float(std::cos(double(a)));
        sinA[6 - k] = float(std::sin(double(a)));
    }
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    // 13 points round the rim (the middle one straight ahead), then the centre
    float px[14], py[14];
    auto point = [&](int slot, float cosT, float sinT) {
        px[slot] = float((double(cosT) * fx - double(sinT) * fy) * sx + x);
        py[slot] = float((double(cosT) * fy + double(sinT) * fx) * sy + y);
    };
    px[6] = float(double(fx) * sx + x);
    py[6] = float(double(fy) * sy + y);
    for (int i = 0; i < 6; i++) {
        point(i, cosA[i], sinA[i]);
        point(7 + i, cosA[6 + i], sinA[6 + i]);
    }
    px[13] = x;
    py[13] = y;
    for (int i = 0; i < 14; i++) {
        Radar_ClampToCircle_(self, &px[i], &py[i]);
    }
    for (int i = 0; i < 12; i++) {
        Draw2DPolygonB(px[i], py[i], px[i + 1], py[i + 1], px[13], py[13], px[13], py[13], colour);
    }
    Im2D_Flush();
}

// 0x54DEF0: the radar's mode (0 the radar, 1 / 2 the pause map: the map's sprites and
// the "Radar" dictionary loaded; back to 0: let go)
void __fastcall SetMode(uint8_t* self, re::Edx, int mode) {
    Mode(self) = mode;
    if (mode == 2 || mode == 1) {
        At<float>(self, 0xE7A4) = 198.0f;
        At<float>(self, 0xE7A8) = -73.0f;
        At<uint8_t>(self, 0xE7E8) = 0;
        At<float>(self, 0x150) = float(double(At<float>(self, 0x158) + At<float>(self, 0x154)) * 0.5);
        float half = float(double(At<float>(self, 0xE764)) * 0.5);
        float across = float(double(half) + 198.0) / At<float>(self, 0xE778);
        float down = float(double(half) - 73.0) / At<float>(self, 0xE77C);
        float rowF = float(double(Cells(self) - 1) - double(down));
        int row = int32_t(float(std::ceil(double(rowF))));
        Radar_StreamTiles_(self, int32_t(float(std::floor(double(across)))), row);
        PushMemoryTag();
        Game_TidyUpMemory(0, 1);
        Txd_PushCurrentTxd();
        if (Txd_FindTxdSlot("Radar", 0) == -1) {
            int slot = Txd_AddTxdSlot("Radar");
            Txd_LoadTxdFile(slot, "TXD/RdrItems.NFT");
            Txd_AddRef(slot);
        }
        Txd_SetCurrentTxd(Txd_FindTxdSlot("Radar", 0));
        const re::Ptr32<const char>* names = reinterpret_cast<const re::Ptr32<const char>*>(0x00AB7304);  // HUD_winleft .. minimap_win04
        for (int i = 0; i < 12; i++) {
            Sprite_SetTexture(self + 0x3A68 + i * 4, names[i]);
        }
        Txd_SetCurrentTxd(Txd_FindTxdSlot("HudElems", 0));
        if (mode == 2) {
            At<float>(self, 0x194) = -704.0f;
            At<float>(self, 0x198) = -704.0f;
            At<float>(self, 0x19C) = 1344.0f;
            At<float>(self, 0x1A0) = 1184.0f;
        }
        Txd_PopCurrentTxd();
        PopMemoryTag();
    } else {
        for (int i = 0; i < 12; i++) {
            if (At<void*>(self, 0x3A68 + i * 4)) {
                Sprite_Delete(self + 0x3A68 + i * 4);
            }
        }
        if (Txd_FindTxdSlot("Radar", 0) != -1) {
            Txd_RemoveRef(Txd_FindTxdSlot("Radar", 0));
            Txd_RemoveTxdSlot(Txd_FindTxdSlot("Radar", 0));
        }
    }
}

// 0x54E120: the player's storey from a position, and the tiles round it streamed
void __fastcall SetCentre(uint8_t* self, re::Edx, const CVector* pos) {
    At<int32_t>(self, 0xE734) = Radar_GetStorey(self, *pos);
    float x = pos->x;
    float y = pos->y;
    float half = float(double(At<float>(self, 0xE764)) * 0.5);
    float tile = At<float>(self, 0xE76C);
    float across = float(x + half) / tile;
    float down = float(y + half) / tile;
    float rowF = float(double(Cells(self) - 1) - double(down));
    int row = int32_t(float(std::ceil(double(rowF))));
    int col = int32_t(float(std::floor(double(across))));
    Radar_StreamTiles_(self, col, row);
}

// 0x54E200: the map's scale from its zoom, its scroll kept in bounds (the map's tiles
// streamed round its centre)
void __fastcall UpdateScale(uint8_t* self, re::Edx) {
    float sy = g_screenScaleY;
    float scaleX = float(double(At<float>(self, 0x164) / At<float>(self, 0x150)) * g_screenScaleX);
    At<float>(self, 0xE7B0) = scaleX;
    float scaleY = float(double(At<float>(self, 0x168) / At<float>(self, 0x150)) * sy);
    At<float>(self, 0xE7B4) = scaleY;
    float h = float(240.0 / double(scaleY) * 80.0);
    float w = float(320.0 / double(scaleX) * 80.0);
    float top = float(double(h) - 320.0);
    At<float>(self, 0xE7C8) = top;
    float left = float(double(w) - 720.0);
    At<float>(self, 0xE7CC) = left;
    float bottom = float(800.0 - double(h));
    At<float>(self, 0xE7D0) = bottom;
    float right = float(640.0 - double(w));
    At<float>(self, 0xE7D4) = right;
    float v = bottom <= At<float>(self, 0xE7A4) ? bottom : At<float>(self, 0xE7A4);
    At<float>(self, 0xE7A4) = v;
    At<float>(self, 0xE7A4) = v <= top ? top : v;
    v = right <= At<float>(self, 0xE7A8) ? right : At<float>(self, 0xE7A8);
    At<float>(self, 0xE7A8) = v;
    At<float>(self, 0xE7A8) = v <= left ? left : v;
    float frameX = At<float>(self, 0x17C) / At<float>(self, 0x170);
    At<float>(self, 0xE7B8) = frameX;
    float frameY = At<float>(self, 0x180) / At<float>(self, 0x170);
    At<float>(self, 0xE7BC) = frameY;
    float cells = float(Cells(self));
    float sizeX = cells * scaleX;
    At<float>(self, 0xE770) = sizeX;
    float sizeY = scaleY * cells;
    At<float>(self, 0xE774) = sizeY;
    float tileX = At<float>(self, 0xE764) * scaleX / sizeX;
    At<float>(self, 0xE778) = tileX;
    float tileY = At<float>(self, 0xE764) * scaleY / sizeY;
    At<float>(self, 0xE77C) = tileY;
    float frameW = frameX * At<float>(self, 0x170);
    At<float>(self, 0xE7C0) = frameW;
    float frameH = frameY * At<float>(self, 0x170);
    At<float>(self, 0xE7C4) = frameH;
    At<float>(self, 0xE780) = At<float>(self, 0xE764) * frameX / frameW;
    At<float>(self, 0xE784) = At<float>(self, 0xE764) * frameY / frameH;
    float factor = float(37.64705882352941 / double(g_screenScaleX));
    At<float>(self, 0x154) = At<float>(self, 0x164) / factor;
    if (Mode(self) != 2 && Mode(self) != 1) {
        return;
    }
    float half = float(double(At<float>(self, 0xE764)) * 0.5);
    float across = float(At<float>(self, 0xE740) + half) / tileX;
    float down = float(half + At<float>(self, 0xE744)) / tileY;
    float rowF = float(double(Cells(self) - 1) - double(down));
    int row = int32_t(float(std::ceil(double(rowF))));
    int col = int32_t(float(std::floor(double(across))));
    Radar_StreamTiles_(self, col, row);
}

// 0x54E560: the radar's tiles: the mask, then the 3x3 round the centre
void __fastcall DrawTiles(uint8_t* self, re::Edx) {
    Radar_DrawMask_(self);
    float half = float(double(At<float>(self, 0xE764)) * 0.5);
    float y = At<float>(self, 0xE744);
    int col = int32_t(float(std::floor(double(float(At<float>(self, 0xE740) + half) / At<float>(self, 0xE76C)))));
    float rowF = float(double(Cells(self) - 1) - double(float(half + y) / At<float>(self, 0xE76C)));
    int row = int32_t(float(std::ceil(double(rowF))));
    Radar_StreamTiles_(self, col, row);
    SetRenderState(0xE, 0);
    SetRenderState(10, 5);
    SetRenderState(0xB, 6);
    SetRenderState(9, 2);
    SetRenderState(7, 1);
    SetRenderState(6, 1);
    SetRenderState(8, 0);
    SetRenderState(0xC, 0);
    SetRenderState(2, 3);
    SetRenderState(5, 0);
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            Radar_DrawTile_(self, col + dx, row + dy);
        }
    }
}

// 0x54E6E0: the pause map: its four quarters (the season's), the outlines, the dark
// panel, and the tiles round its centre streamed
void __fastcall DrawMap(uint8_t* self, re::Edx) {
    CRGBA white = {0xD7, 0xD7, 0xD7, 0xFF};
    float u = float((0.5 - double(At<float>(self, 0x184))) / double(float(At<float>(self, 0x18C) - At<float>(self, 0x184))));
    float v = float((0.5 - double(At<float>(self, 0x188))) / double(float(At<float>(self, 0x190) - At<float>(self, 0x188))));
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    float w = At<float>(self, 0x17C);
    float h = At<float>(self, 0x180);
    float a = float(double(u) * w * sx);
    float b = float((1.0 - double(u)) * w * sx);
    float c = float(double(v) * h * sy);
    float d = float((1.0 - double(v)) * h * sy);
    float x0 = At<float>(self, 0x174);
    float y0 = At<float>(self, 0x178);
    double xmE = double(a) + double(x0);
    double ymE = double(y0) + double(c);
    float xm = float(xmE);
    float ym = float(ymE);
    float x2 = float(double(xm) + b);
    float y2 = float(double(ym) + d);
    col::CRect topLeft = {x0, ym, xm, y0};
    col::CRect bottomRight = {xm, y2, x2, ym};
    col::CRect topRight = {xm, ym, x2, y0};
    col::CRect bottomLeft = {x0, y2, xm, ym};
    At<float>(self, 0x194) = x0;
    At<float>(self, 0x198) = y0;
    At<float>(self, 0x19C) = float(xmE + b);
    At<float>(self, 0x1A0) = float(ymE + d);
    float u0 = float(double(At<float>(self, 0x184)) * 2.0);
    float v0 = float(double(At<float>(self, 0x188)) * 2.0);
    float u1 = float(double(At<float>(self, 0x18C)) * 2.0);
    float v1 = float(double(At<float>(self, 0x190)) * 2.0);
    int sprite = At<int32_t>(self, 0xE828) == 1 ? 10 : 6;
    CVector2D uvA = {u0, v0}, uvB = {1.0f, 1.0f};
    DrawSprite(&topLeft, At<void*>(self, 0x3A60 + sprite * 4), &white, &uvA, &uvB);
    float u1m = float(double(u1) - 1.0);
    uvA = {0.0f, v0};
    uvB = {u1m, 1.0f};
    DrawSprite(&topRight, At<void*>(self, 0x3A60 + (sprite + 1) * 4), &white, &uvA, &uvB);
    float v1m = float(double(v1) - 1.0);
    uvA = {u0, 0.0f};
    uvB = {1.0f, v1m};
    DrawSprite(&bottomLeft, At<void*>(self, 0x3A60 + (sprite + 2) * 4), &white, &uvA, &uvB);
    uvA = {0.0f, 0.0f};
    uvB = {u1m, v1m};
    DrawSprite(&bottomRight, At<void*>(self, 0x3A60 + (sprite + 3) * 4), &white, &uvA, &uvB);
    Radar_DrawOutlines_(self);
    CRGBA dark = {0, 0, 0, 0xC0};
    CRGBA clear = {0, 0, 0, 0};
    FE_DrawPanel(x0, y0, x2, y2, 1.0f, &clear, &dark);
    float half = float(double(At<float>(self, 0xE764)) * 0.5);
    float across = float(At<float>(self, 0xE740) + half) / At<float>(self, 0xE778);
    float down = float(half + At<float>(self, 0xE744)) / At<float>(self, 0xE77C);
    float rowF = float(double(Cells(self) - 1) - double(down));
    int row = int32_t(float(std::ceil(double(rowF))));
    int col = int32_t(float(std::floor(double(across))));
    Radar_StreamTiles_(self, col, row);
    Streaming_Update();
}

// 0x54EB10: the pause map's pad controls: zoom (axis 0x13), scroll (the sticks and
// the d-pad) or, in the legend (button 0xC), up / down through it and toggling one
void __fastcall MapControls(uint8_t* self, re::Edx) {
    float scrollY = float(double(Pad_GetValue(0x10, 0)) * 10.0);
    float scrollX = float(double(Pad_GetValue(0x11, 0)) * 10.0);
    float zoom = Pad_GetValue(0x13, 0);
    if (Pad_GetValue(2, 0) != 0.0f) {
        scrollX = 10.0f;
    }
    if (Pad_GetValue(3, 0) != 0.0f) {
        scrollX = -10.0f;
    }
    if (Pad_GetValue(0, 0) != 0.0f) {
        scrollY = 10.0f;
    }
    if (Pad_GetValue(1, 0) != 0.0f) {
        scrollY = -10.0f;
    }
    if (zoom < -0.3f) {
        float z = float(double(At<float>(self, 0x150)) + double(0.1f));
        float max = At<float>(self, 0x154);
        At<float>(self, 0x150) = max <= z ? max : z;
        Radar_UpdateScale_(self);
    }
    if (0.3f < zoom) {
        float z = float(double(At<float>(self, 0x150)) - double(0.1f));
        float min = At<float>(self, 0x158);
        At<float>(self, 0x150) = z <= min ? min : z;
        Radar_UpdateScale_(self);
    }
    if (Pad_GetValue(0xE, 0) != 0.0f) {
        FE_PlaySelect();
        At<float>(self, 0xE7A4) = 198.0f;
        At<float>(self, 0xE7A8) = -73.0f;
    }
    if (Pad_JustPressed(0xC, 0)) {
        FE_PlaySelect();
        At<uint8_t>(self, 0xE7E8) = At<uint8_t>(self, 0xE7E8) == 0;
    }
    if (At<uint8_t>(self, 0xE7E8) == 0) {
        if (!(double(std::fabs(scrollX)) < double(0.0001f))) {
            FE_PlayScroll();
            float q = At<float>(self, 0x150) / At<float>(self, 0x154);
            At<float>(self, 0xE7A4) = float(int32_t(double(q) * (double(scrollX) * 1.75) + At<float>(self, 0xE7A4)));
        }
        if (!(double(std::fabs(scrollY)) < double(0.0001f))) {
            FE_PlayScroll();
            float q = At<float>(self, 0x150) / At<float>(self, 0x154);
            At<float>(self, 0xE7A8) = float(int32_t(double(q) * (double(scrollY) * 1.75) + At<float>(self, 0xE7A8)));
        }
        At<int32_t>(self, 0xE7EC) = 0;
        Radar_UpdateScale_(self);
        return;
    }
    int32_t& legend = At<int32_t>(self, 0xE7EC);
    if (g_padDown && FE_PadPressed(0)) {
        legend++;
        if (legend < 10) {
            FE_PadClear(0);
        } else {
            legend--;
        }
    } else if (g_padUp && FE_PadPressed(0)) {
        legend--;
        if (legend >= 0) {
            FE_PadClear(0);
        } else {
            legend++;
        }
    }
    if (Pad_JustPressed(7, 0) && g_mapLegend[legend * 8 + 1]) {
        FE_PlaySelect();
        g_mapLegend[legend * 8] = g_mapLegend[legend * 8] == 0;
    }
    Radar_UpdateScale_(self);
}

// 0x54EE80: an entity's blip (a car, char, object, pickup...): gone when its entity
// is; a char's coloured by how it stands with the player, with its vision cone
void __fastcall DrawEntityBlip(uint8_t* self, re::Edx, int index) {
    RadarBlip& blip = Blip(self, index);
    uint8_t* ped = nullptr;
    uint8_t* entity = nullptr;
    bool hostile = false;    // (it's after the player: red cone)
    bool targeting = false;  // (its target is the player)
    bool chasing = false;    // (its current objective is 0x19)
    float coneHeading = 0.0f;
    float coneWidth = 0.0f;
    CVector look = {0.0f, 0.0f, 0.0f};
    switch (blip.type) {
    case 1:
    case 0xC:
        entity = GetVehicleFromHandle(blip.handle);
        break;
    case 3:
    case 5:
        entity = Pools_GetObject(blip.handle);
        break;
    case 2:
    case 6:
    case 0xD: {
        ped = GetPedFromHandle(blip.handle);
        if (!ped) {
            break;
        }
        entity = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x1554);  // (in a vehicle: that)
        if (entity) {
            break;
        }
        entity = ped;
        if (blip.flags & 0x10) {
            hostile = true;
        } else {
            hostile = PedScanner_IsAroundAndSeen(ped + 0x3BC, g_playerPed) != 0;
        }
        targeting = Ped_Unknown473FB0(ped) && Ped_GetTarget(ped) == g_playerPed;
        uint8_t* objective = ObjectiveQueue_GetCurrent(ped + 0x5F0);
        chasing = objective && *reinterpret_cast<int32_t*>(objective + 8) == 0x19;
        CVector forward;
        CVector flat = *Placeable_GetForward(ped, &forward);
        flat.z = 0.0f;
        Vector_Normalise(&flat);  // (not used after)
        CVector out;
        look = *PedLook_GetDir(ped + 0xA64, &out);
        coneHeading = HeadingFromVector(&look);
        coneWidth = *reinterpret_cast<float*>(ped + 0xA68);
        break;
    }
    case 7: {
        entity = Pools_Get44A5E0(blip.handle);
        if (!entity) {
            break;
        }
        uint8_t* info = ModelInfo_5201D0(g_modelInfos[*reinterpret_cast<int16_t*>(entity + 0x10E)]);
        uint8_t transform[0x34] = {};
        *reinterpret_cast<float*>(transform + 0x30) = 1.0f;
        GetNodeTransform(entity, *reinterpret_cast<int32_t*>(info + 0x60), transform, 1);  // (not used after)
        break;
    }
    case 0xB: {
        int pickup = Pickups_Find(blip.handle);
        if (pickup < 0) {
            break;
        }
        entity = *reinterpret_cast<re::Ptr32<uint8_t>*>(0x00C0A11C + pickup * 0x48);
        break;
    }
    default:
        break;
    }
    if (!entity) {
        Radar_RemoveBlipsFor(self, blip.type, blip.handle);
        return;
    }
    // (r, g, b and a; the three are passed as dwords from here on: the bytes after the
    // alpha are the game's stack, zero here)
    uint8_t colour[8] = {SpriteInfo(blip.sprite)[0xC], SpriteInfo(blip.sprite)[0xD], SpriteInfo(blip.sprite)[0xE], 0xFF};
    bool neutralIcon = false;  // (drawn with sprite 0)
    auto matrixPos = [](uint8_t* e) {
        uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(e + 0x14);
        return matrix ? *reinterpret_cast<CVector*>(matrix + 0x30) : *reinterpret_cast<CVector*>(e + 4);
    };
    CVector pos = matrixPos(entity);
    if (Mode(self) != 0) {
        int sprite = blip.sprite;
        if (sprite != 0 && sprite != 0x1D && sprite != 0x11 && sprite != 0x1A && sprite != 0x22 &&
            !Radar_IsOnMap(self, pos.x, pos.y)) {
            return;
        }
    }
    bool shownNow = true;
    CVector offset = {};
    if (!ped) {
        if (uint8_t* col = GetColModel(entity)) {
            offset = *reinterpret_cast<CVector*>(col + 0x20);
            pos.z = *reinterpret_cast<float*>(col + 0x28) + pos.z;
        }
    } else {
        const char* spriteName = *reinterpret_cast<re::Ptr32<const char>*>(SpriteInfo(blip.sprite));
        if (spriteName[0] == '\0') {
            std::memcpy(colour, self + 0xE813, 4);
            if (blip.type == 6) {
                std::memcpy(colour, self + 0xE817, 4);
            } else if (PedFriends_485590(g_playerPed + 0x2F0, ped) || blip.sprite == 0x1B) {
                std::memcpy(colour, self + 0xE823, 4);
            } else if (*reinterpret_cast<int32_t*>(ped + 0x141C) != 0 || blip.type == 0xD || blip.sprite == 0x11) {
                std::memcpy(colour, self + 0xE81B, 4);
                neutralIcon = true;
            } else if (*reinterpret_cast<int32_t*>(ped + 0x1398) == 0) {
                int32_t out;
                if (*Ped_475400(ped, &out, g_playerPed) > 1) {
                    if (Ped_WantsGift(ped) == 1) {
                        std::memcpy(colour, (ped[0xD68] & 0x40) ? self + 0xE817 : self + 0xE813, 4);
                    } else if (*reinterpret_cast<int32_t*>(ped + 0x14C0) != 0) {
                        std::memcpy(colour, self + 0xE817, 4);
                    } else if (Ped_496F30(g_playerPed, ped)) {
                        std::memcpy(colour, self + 0xE823, 4);
                    }
                } else {
                    std::memcpy(colour, self + 0xE817, 4);
                }
            } else {
                std::memcpy(colour, self + 0xE817, 4);
            }
        }
        pos = *PedAi_48D020(ped + 0xA64, &offset);
    }
    uint32_t rgba;
    std::memcpy(&rgba, colour, 4);
    if (blip.display == 5 || blip.display == 2 || blip.display == 4) {
        CVector dir = {0.0f, 0.0f, 0.0f};
        PlaceMarker(2, -1, 0, &pos, blip.size, &dir, -1, int32_t(rgba), int32_t(rgba >> 8), int32_t(rgba >> 16),
                    uint32_t(reinterpret_cast<uintptr_t>(entity)), 1, 5);
    }
    if (blip.display != 5 && blip.display != 1 && blip.display != 4 && blip.display != 3) {
        return;
    }
    int area = Radar_GetArea(matrixPos(entity));
    if (area != At<int32_t>(self, 0xE7F8) && !SpriteInfo(blip.sprite)[0x11]) {
        return;
    }
    CVector shown;
    shown = *Radar_ToPlayersArea_(self, &shown, matrixPos(entity));
    CVector2D world = {shown.x, shown.y};
    CVector2D map;
    Radar_WorldToMap(self, &map, &world);
    CVector2D coneAt;
    Radar_MapToScreen(self, &coneAt, &map);
    float distance = Radar_LimitToEdge(self, &map);
    CVector2D screen;
    Radar_MapToScreen(self, &screen, &map);
    bool offEdge = 1.0f < distance && Mode(self) == 0;
    float height = float(int32_t(blip.heightOffset));
    if (blip.type != 6 && blip.flashUntil > g_timeInMilliseconds && Mode(self) == 0 &&
        (!ped || int32_t(*reinterpret_cast<int16_t*>(ped + 0x10E)) != g_flashExemptModel) &&
        ((blip.flashUntil - g_timeInMilliseconds) / 250 & 1) == 0) {
        shownNow = false;
    }
    float playerZ = FindPlayerPosition()->z;
    int storey = Radar_GetStorey(self, shown);
    uint8_t flags = blip.flags;
    if ((flags & 4) && (offEdge || At<int32_t>(self, 0xE7F8) != area || storey != At<int32_t>(self, 0xE734))) {
        if (!ped || *reinterpret_cast<int32_t*>(ped + 0x1398) == 0) {
            if (Mode(self) == 0) {
                return;
            }
        }
    }
    int arrow = 2;
    if (Mode(self) == 0) {
        int playerStorey = At<int32_t>(self, 0xE734);
        if (playerStorey == 0) {
            if (SpriteInfo(blip.sprite)[0x10]) {
                if (playerZ < shown.z - 4.5) {
                    arrow = 0;
                } else if (shown.z + 4.5 < playerZ) {
                    arrow = 1;
                }
            }
        } else if (SpriteInfo(blip.sprite)[0xF]) {
            if (storey < playerStorey) {
                arrow = 1;
            } else if (storey > playerStorey) {
                arrow = 0;
            }
        }
    }
    bool onEdgeOk = !offEdge;
    bool draw;
    if (ped && *reinterpret_cast<int32_t*>(ped + 0x1398) != 0) {
        draw = true;
        if (arrow != 2) {
            onEdgeOk = false;
            if (offEdge) {
                draw = false;
            }
        }
    } else {
        draw = shownNow;
    }
    int type = blip.type;
    if (type == 6 && arrow != 2) {
        draw = false;
    }
    if (type != 6 && type != 2 && type != 0xD) {
        if (draw) {
            Radar_DrawBlipSprite(self, blip.sprite, screen.x, screen.y, 0xFF, arrow, 1.0f);
        }
        return;
    }
    if (!draw) {
        return;
    }
    if (ped && *reinterpret_cast<int32_t*>(ped + 0x1528) != 0) {
        bool level = arrow == 2 || (flags & 8);
        if (onEdgeOk && level) {
            bool inTrouble = *reinterpret_cast<int32_t*>(g_playerPed + 0x1D40) > 0;
            int alert = -1;
            if (type == 6) {
                if (g_classSkip[2] || targeting || Schedule_6A8D20(kSchedule) || g_showVisionCones[1] || inTrouble) {
                    alert = hostile ? 2 : (targeting || chasing || inTrouble) ? 1 : 0;
                }
            } else if (blip.display == 3 || blip.display == 5) {
                alert = hostile ? 2 : (targeting || chasing) ? 1 : 0;
            }
            if (alert >= 0) {
                float range = float(int32_t(*reinterpret_cast<float*>(ped + 0xA6C)));
                Radar_DrawCone_(self, coneAt.x, coneAt.y, coneHeading, range, coneWidth, look.x, look.y, look.z,
                                alert);
            }
        }
    }
    int sprite = blip.sprite;
    if (neutralIcon || sprite == 0x11) {
        Radar_DrawBlipSprite(self, 0, screen.x, screen.y, 0xFF, arrow, 1.0f);
    } else if (sprite == 2 || sprite == 0x1B) {
        int32_t r, g, b;
        std::memcpy(&r, colour + 0, 4);
        std::memcpy(&g, colour + 1, 4);
        std::memcpy(&b, colour + 2, 4);
        Radar_DrawCharBlip(self, screen.x, screen.y, height, r, g, b, 0xFF, arrow);
    } else {
        Radar_DrawBlipSprite(self, sprite, screen.x, screen.y, 0xFF, arrow, 1.0f);
    }
}

// 0x54F9D0 (vf1 Setup): the radar's layout: its size, the circle, the zoom limits,
// the map's frame and its UVs, the blip colours
void __fastcall Setup(uint8_t* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    auto s16 = [d](int offset) { return int32_t(*reinterpret_cast<const int16_t*>(d + offset)); };
    auto hAlign = [self]() { return At<int32_t>(self, 0xC); };
    auto vAlign = [self]() { return At<int32_t>(self, 0x10); };
    HUDComponent* component = reinterpret_cast<HUDComponent*>(self);
    HUDComponent_SetAlignment(component, reinterpret_cast<const char*>(d + 0x84), reinterpret_cast<const char*>(d + 0x8C));
    At<float>(self, 0x12C) = float(s16(0x28));
    At<int32_t>(self, 0x130) = s16(0x2A);
    col::CRect rect;
    At<col::CRect>(self, 0x138) = *ScreenRect(&rect, float(s16(0x2C)), float(s16(0x2E)), float(s16(0x30)),
                                              float(s16(0x32)), hAlign(), vAlign());
    At<float>(self, 0x134) = float(s16(0x34));
    At<float>(self, 0x148) = float(s16(0x36));
    HUDComponent_SetAlignment(component, reinterpret_cast<const char*>(d + 0x94), reinterpret_cast<const char*>(d + 0x9C));
    At<float>(self, 0x14C) = float(s16(0x38));
    At<float>(self, 0x150) = float(s16(0x3A));
    At<float>(self, 0x154) = float(s16(0x3C));
    At<float>(self, 0x158) = float(s16(0x3E));
    At<float>(self, 0x164) = float(s16(0x44));
    At<float>(self, 0x168) = float(s16(0x46));
    CVector2D point;
    At<CVector2D>(self, 0x15C) = *ScreenPosition(&point, float(s16(0x40)), float(s16(0x42)), hAlign(), vAlign());
    HUDComponent_SetAlignment(component, reinterpret_cast<const char*>(d + 0xA4), reinterpret_cast<const char*>(d + 0xAC));
    At<float>(self, 0x16C) = float(s16(0x48));
    At<float>(self, 0x170) = float(s16(0x4A));
    At<float>(self, 0x17C) = float(s16(0x50));
    At<float>(self, 0x180) = float(s16(0x52));
    std::memcpy(self + 0x184, d, 0x10);  // the map's UVs (floats)
    At<CVector2D>(self, 0x174) = *ScreenPosition(&point, float(s16(0x4C)), float(s16(0x4E)), hAlign(), vAlign());
    At<float>(self, 0x174) = float(320.0 - double(At<float>(self, 0x17C)) * 0.5 * g_screenScaleX);
    At<int32_t>(self, 0x1A4) = s16(0x54);
    At<int32_t>(self, 0x1A8) = s16(0x56);
    At<int32_t>(self, 0x1AC) = s16(0x58);
    At<int32_t>(self, 0x1B0) = s16(0x5A);
    At<uint32_t>(self, 0x1B4) = *reinterpret_cast<const uint32_t*>(d + 0x10);
    HUDComponent_SetAlignment(component, reinterpret_cast<const char*>(d + 0xB4), reinterpret_cast<const char*>(d + 0xBC));
    At<col::CRect>(self, 0x1BC) = *ScreenRect(&rect, float(s16(0x5C)), float(s16(0x5E)), float(s16(0x60)),
                                              float(s16(0x62)), hAlign(), vAlign());
    At<float>(self, 0x1DC) = float(s16(0x68));
    At<CVector2D>(self, 0x1CC) = *ScreenPosition(&point, float(s16(0x64)), float(s16(0x66)), hAlign(), vAlign());
    At<CVector2D>(self, 0x1D4) = *ScreenPosition(&point, float(s16(0x6A)), float(s16(0x6C)), hAlign(), vAlign());
    At<float>(self, 0x1E0) = float(s16(0x6E));
    HUDComponent_SetAlignment(component, reinterpret_cast<const char*>(d + 0xC4), reinterpret_cast<const char*>(d + 0xCC));
    At<CVector2D>(self, 0x1E4) = *ScreenPosition(&point, float(s16(0x70)), float(s16(0x72)), hAlign(), vAlign());
    At<float>(self, 0x1E8) = float(double(At<float>(self, 0x1E8)) - 15.0);
    At<float>(self, 0x1EC) = float(s16(0x74));
    At<float>(self, 0x1F0) = float(s16(0x76));
    At<float>(self, 0x1F4) = float(s16(0x78));
    At<float>(self, 0x1F8) = float(s16(0x7A));
    At<float>(self, 0x1FC) = float(s16(0x7C));
    At<float>(self, 0x200) = float(s16(0x7E));
    At<float>(self, 0x204) = float(s16(0x80));
    At<CVector2D>(self, 0x208) = *ScreenPosition(&point, float(s16(0x70)), float(s16(0x82)), hAlign(), vAlign());
    At<float>(self, 0x20C) = float(double(At<float>(self, 0x20C)) - 15.0);
    std::memcpy(self + 0xE813, d + 0x14, 0x14);  // the five blip colours
    At<int32_t>(self, 0xE7D8) = At<int32_t>(self, 0x1A4);
    At<int32_t>(self, 0xE7DC) = At<int32_t>(self, 0x1A8);
    At<int32_t>(self, 0xE7E0) = At<int32_t>(self, 0x1AC);
    At<int32_t>(self, 0xE7E4) = At<int32_t>(self, 0x1B0);
    float width = std::fabs(At<float>(self, 0x140) - At<float>(self, 0x138));
    At<float>(self, 0xE754) = float(double(width) * 0.5 + At<float>(self, 0x138));
    float height = std::fabs(At<float>(self, 0x144) - At<float>(self, 0x13C));
    At<float>(self, 0xE758) = float(double(height) * 0.5 + At<float>(self, 0x13C));
    float radius = At<float>(self, 0x134);
    At<float>(self, 0xE75C) = radius;
    At<float>(self, 0xE760) = radius * radius;
    float size = float(At<int32_t>(self, 0x130)) * At<float>(self, 0x12C);
    At<float>(self, 0xE768) = size;
    At<float>(self, 0xE76C) = At<float>(self, 0xE764) * At<float>(self, 0x12C) / size;
    Radar_UpdateScale_(self);
}

// 0x5500D0: the blips, in 8 passes by their sprite's layer (the nearest of the
// shops' and of sprite 0x1C only), with the north marker and the player's arrow
void __fastcall DrawBlips(uint8_t* self, re::Edx) {
    if (At<uint32_t>(self, 0xE834) <= g_timeInMilliseconds) {
        At<int32_t>(self, 0xE830) = -1;
    }
    if (g_widescreen != 0) {
        return;
    }
    int saved[5];
    GetRenderState(8, &saved[0]);
    GetRenderState(6, &saved[1]);
    GetRenderState(0xC, &saved[2]);
    GetRenderState(10, &saved[3]);
    GetRenderState(0xB, &saved[4]);
    SetRenderState(8, 0);
    SetRenderState(6, 0);
    SetRenderState(0xC, 1);
    SetRenderState(10, 5);
    SetRenderState(0xB, 6);
    float heading = FindPlayerHeading();
    if (Mode(self) == 0 && !At<uint8_t>(self, 0xE7F4)) {
        // the north marker round the rim
        float halfW = float(double(float(std::fabs(At<float>(self, 0x140) - At<float>(self, 0x138)))) * 0.5);
        float halfH = float(double(float(std::fabs(At<float>(self, 0x144) - At<float>(self, 0x13C)))) * 0.5);
        float sx = g_screenScaleX;
        float sy = g_screenScaleY;
        float areaAngle = AreaAngle(At<int32_t>(self, 0xE7F8));
        float s, c;
        if (!At<uint8_t>(self, 0xE811)) {
            float a = float(double(g_cameraHeading) - double(3.1415927f) + areaAngle);
            s = float(std::sin(double(a)));
            float b = g_cameraHeading + areaAngle;
            c = float(std::cos(double(b)));
        } else {
            float a = At<float>(self, 0xE80C) + areaAngle;
            s = float(std::sin(double(a)));
            c = float(std::cos(double(a)));
        }
        void* north = g_northTexture;
        float cx = float(double(At<float>(self, 0x138)) + double(halfW) + double(sx) * (double(c) * 40.0 - double(s) * 0.0));
        float cy = float(double(At<float>(self, 0x13C)) + double(halfH) + double(sy) * (double(s) * 40.0 + double(c) * 0.0));
        CVector2D uv2 = {1.0f, 1.0f};
        CVector2D uv1 = {0.0f, 0.0f};
        double half = double(At<float>(self, 0x148)) * 0.5;
        double hy = double(sy) * half;
        double hx = half * double(sx);
        CRGBA grey = {0xEB, 0xEB, 0xEB, 0xFF};
        DrawRectUV(float(cx - hx), float(cy - hy), float(cx + hx), float(cy + hy), north, &grey, &uv1, &uv2);
    }
    CVector player;
    player = *Radar_ToPlayersArea_(self, &player, At<CVector>(self, 0xE748));
    int area = Radar_GetArea(At<CVector>(self, 0xE748));
    bool inSchool;
    bool indoors;
    if (area == 2 && Mode(self) == 0) {
        inSchool = true;
        indoors = Mode(self) == 0;
    } else {
        inSchool = false;
        if (area != 0) {
            indoors = Mode(self) == 0;
        } else {
            if (Mode(self) == 0) {
                void* school = Zones_GetByName(kZones, "AMB_SCHOOL_AREA");
                void* forest = Zones_GetByName(kZones, "Forest");
                float xy[2] = {At<float>(self, 0xE748), At<float>(self, 0xE74C)};
                if (school && Zone_IsPointInside(school, xy)) {
                    inSchool = true;
                } else if (forest && Zone_IsPointInside(forest, xy)) {
                    inSchool = true;
                } else {
                    inSchool = false;
                }
            }
            indoors = false;
        }
    }
    // the nearest shop (type 10) blip: the one in the player's area, else the closest
    int nearestShop = -1;
    if (!MissionMgr_6AA7E0(kMissionMgr)) {
        float best = 3.4028235e+38f;
        for (int i = 0; i < 300; i++) {
            RadarBlip& blip = Blip(self, i);
            if (blip.type != 10) {
                continue;
            }
            CVector pos = {blip.x, blip.y, blip.z};
            if (Radar_GetArea(pos) == area) {
                nearestShop = i;
                break;
            }
            CVector shown;
            shown = *Radar_ToPlayersArea_(self, &shown, pos);
            CVector d;
            VectorSub(&d, &shown, &player);
            float distSq = d.x * d.x + d.y * d.y;
            if (distSq < best) {
                best = distSq;
                nearestShop = i;
            }
        }
    }
    int nearest1C = -1;
    if (At<uint8_t>(self, 0xE844) == 1 && !inSchool && !indoors) {
        float best = 3.4028235e+38f;
        for (int i = 0; i < 300; i++) {
            RadarBlip& blip = Blip(self, i);
            if (blip.sprite != 0x1C) {
                continue;
            }
            CVector pos = {blip.x, blip.y, blip.z};
            CVector shown;
            shown = *Radar_ToPlayersArea_(self, &shown, pos);
            CVector d;
            VectorSub(&d, &shown, &player);
            float distSq = d.x * d.x + d.y * d.y;
            if (distSq < best) {
                best = distSq;
                nearest1C = i;
            }
        }
    }
    for (int layer = 0; layer < 8; layer++) {
        for (int i = 0; i < 300; i++) {
            RadarBlip& blip = Blip(self, i);
            int32_t selected = At<int32_t>(self, 0xE830);
            if (selected >= 0 && i != selected) {
                continue;
            }
            if (!(blip.flags & 2)) {
                continue;
            }
            if (blip.type == 5 && (MissionMgr_GetStats(kMissionMgr, blip.id)[4] & 0x40)) {
                continue;
            }
            int sprite = blip.sprite;
            if (sprite == 7 || *reinterpret_cast<int32_t*>(SpriteInfo(sprite) + 8) != layer) {
                continue;
            }
            if (sprite == 4 && indoors) {
                continue;
            }
            bool coord = false;
            bool mission = false;
            switch (blip.type) {
            case 1:
            case 2:
            case 3:
            case 6:
            case 7:
            case 0xB:
            case 0xD:
                if (sprite == 0x23 || sprite == 0x24 || sprite == 0x25 || sprite == 0x26) {
                    if (Mode(self) == 2) {
                        Radar_DrawCollectibleBlip_(self, i);
                    }
                } else if (Mode(self) == 0 || sprite == 0 || sprite == 0x11 || sprite == 1 || sprite == 3 ||
                           sprite == 0x1D || sprite == 0x1E || sprite == 0x1F || sprite == 0x1A || sprite == 0x22) {
                    Radar_DrawEntityBlip_(self, i);
                }
                break;
            case 4:
                coord = true;
                break;
            case 5:
                mission = true;
                break;
            case 8:
            case 9:
                Radar_DrawCoordBlip_(self, i);
                break;
            case 10:
                mission = i == nearestShop || Mode(self) != 0;
                break;
            case 0xC:
                if (Mode(self) != 0) {
                    Radar_DrawEntityBlip_(self, i);
                }
                break;
            default:
                break;
            }
            if (mission) {
                if ((!MissionMgr_6AA7E0(kMissionMgr) || blip.sprite == 5 || blip.sprite == 6) &&
                    !MissionMgr_InClass(kMissionMgr)) {
                    coord = true;
                }
            }
            if (coord && (blip.sprite != 0x1C || i == nearest1C || Mode(self) != 0)) {
                Radar_DrawCoordBlip_(self, i);
            }
        }
    }
    CVector2D world = {player.x, player.y};
    CVector2D map;
    Radar_WorldToMap(self, &map, &world);
    CVector2D screen;
    Radar_MapToScreen(self, &screen, &map);
    if (!At<uint8_t>(self, 0xE7F4) && !At<uint8_t>(self, 0xE812)) {
        if (Mode(self) == 2 || Mode(self) == 1) {
            uint8_t* game = g_gameMode;
            if (game && game[4] && *reinterpret_cast<int32_t*>(game + 0x3C) == 0xE && game[0x50C]) {
                CVector at;
                at = *Radar_ToPlayersArea_(self, &at, At<CVector>(self, 0xE838));
                CVector2D atXY = {at.x, at.y};
                Radar_WorldToMap(self, &map, &atXY);
                Radar_MapToScreen(self, &screen, &map);
            }
            float angle = float(double(heading) - double(g_halfPi) - double(AreaAngle(At<int32_t>(self, 0xE7F8))));
            Radar_DrawArrow(self, self + 0x3A60, screen.x, screen.y, angle, 0xFF);
        } else if (Mode(self) == 0) {
            Radar_DrawArrow(self, self + 0x3A60, screen.x, screen.y, heading, 0xFF);
        }
    }
    SetRenderState(8, saved[0]);
    SetRenderState(6, saved[1]);
    SetRenderState(0xC, saved[2]);
    SetRenderState(10, saved[3]);
    SetRenderState(0xB, saved[4]);
}

// 0x550A70: the full-screen map (mode 2): the controls, the sea, the ground tiles on
// screen (only 6..19 x 2..18 exist) and the outlines
void __fastcall DrawMapTiles(uint8_t* self, re::Edx) {
    Radar_MapControls_(self);
    col::CRect screenRect = {0.0f, 480.0f, 640.0f, 0.0f};
    CRGBA sea;
    if (At<int32_t>(self, 0xE828) == 1) {
        sea = {0xDA, 0xE9, 0xEA, 0xFF};
    } else {
        sea = {0x1D, 0x73, 0x1D, 0xFF};
    }
    float half = float(double(At<float>(self, 0xE764)) * 0.5);
    float x = At<float>(self, 0xE740) + half;
    float y = half + At<float>(self, 0xE744);
    int col = int32_t(float(std::floor(double(float(x / At<float>(self, 0xE778))))));
    float rowF = float(double(Cells(self) - 1) - double(float(y / At<float>(self, 0xE77C))));
    int row = int32_t(float(std::ceil(double(rowF))));
    Radar_StreamTiles_(self, col, row);
    Streaming_Update();
    SetRenderState(0xE, 0);
    SetRenderState(10, 5);
    SetRenderState(0xB, 6);
    SetRenderState(9, 2);
    SetRenderState(7, 1);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xC, 0);
    SetRenderState(2, 3);
    SetRenderState(5, 0);
    CVector2D uv1 = {0.0f, 0.0f};
    CVector2D uv2 = {1.0f, 1.0f};
    DrawSprite(&screenRect, nullptr, &sea, &uv1, &uv2);
    At<float>(self, 0xE78C) = float(col);
    At<float>(self, 0xE790) = float(row);
    At<float>(self, 0xE794) = x;
    At<float>(self, 0xE798) = y;
    int tileW = int32_t(double(At<float>(self, 0xE778)) + 0.5);
    At<float>(self, 0xE79C) = float(int32_t(double(x) + 0.5) % tileW);
    int yi = int32_t(double(y) + 0.5);
    At<float>(self, 0xE7A0) = float(yi % int32_t(double(At<float>(self, 0xE77C)) + 0.5));
    int reach = int32_t(640.0 / double(At<float>(self, 0xE7B0)) * 0.5 + 1.0);
    float v = float(col - reach);
    int minX = int32_t(0.0f <= v ? v : 0.0f);
    float last = float(Cells(self) - 1);
    v = float(reach + col);
    int maxX = int32_t(last <= v ? last : v);
    v = float(row - reach);
    int minY = int32_t(v < 0.0f ? 0.0f : v);
    v = float(reach + row);
    int maxY = int32_t(last <= v ? last : v);
    v = float(minX);
    minX = int32_t(v <= 6.0f ? 6.0f : v);
    v = float(maxX);
    maxX = int32_t(v < 19.0f ? v : 19.0f);
    v = float(minY);
    minY = int32_t(v <= 2.0f ? 2.0f : v);
    v = float(maxY);
    maxY = int32_t(v < 18.0f ? v : 18.0f);
    for (int i = minX; i <= maxX; i++) {
        for (int j = minY; j <= maxY; j++) {
            Radar_DrawTileWhole_(self, i, j);
        }
    }
    Radar_DrawOutlines_(self);
}

// 0x550EA0: the blips alone (the radar's own drawing off while they're drawn)
void __fastcall DrawBlipsOnly(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 0xE7F4) = 1;
    Radar_DrawBlips_(self);
    At<uint8_t>(self, 0xE7F4) = 0;
}

// 0x550EC0: the radar's centre (the player, or a set point), its zoom eased towards
// the speed's (the storey's indoors), the storey tiles let go outdoors, then the
// tiles (or the map) drawn
void __fastcall Update(uint8_t* self, re::Edx) {
    const CVector* source;
    if (At<uint8_t>(self, 0xE811) && Mode(self) == 0) {
        source = &At<CVector>(self, 0xE800);
    } else {
        uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(g_playerPed + 0x14);
        source = matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : reinterpret_cast<const CVector*>(g_playerPed + 4);
    }
    CVector pos = *source;
    if (Mode(self) == 0 && g_widescreen != 0) {
        return;
    }
    Radar_UpdateRotation(self);
    FindPlayerSpeed();
    int storey = Radar_GetStorey(self, pos);
    At<int32_t>(self, 0xE734) = storey;
    float& zoom = At<float>(self, 0x3A50);
    if (storey != 0) {
        zoom = float(At<int32_t>(self, 0xE7E4));
    } else {
        float target = FindPlayerVehicle(0) ? float(At<int32_t>(self, 0xE7DC)) : 40.0f;
        float step = At<float>(self, 0x1B4);
        if (target > zoom) {
            float next = float(step + zoom);
            zoom = next < target ? next : target;
        } else if (zoom > target) {
            float next = float(zoom - step);
            zoom = next <= target ? target : next;
        }
        float z = zoom;
        zoom = z <= 40.0f ? 40.0f : z;
        if (FindPlayerVehicle(0)) {
            float limit = float(At<int32_t>(self, 0xE7DC));
            zoom = limit <= zoom ? limit : zoom;
        }
    }
    switch (Mode(self)) {
    case 0: {
        At<CVector>(self, 0xE748) = pos;
        At<float>(self, 0xE740) = pos.x;
        At<float>(self, 0xE744) = pos.y;
        if (At<uint8_t>(self, 0xE7F5) && At<int32_t>(self, 0xE734) != 0) {
            At<uint8_t>(self, 0xE7F5) = 0;
            for (int x = 0; x < Cells(self); x++) {
                for (int y = 0; y < Cells(self); y++) {
                    int entry = (Cells(self) * y + x) >> 1;
                    int32_t* slots =
                        At<int32_t>(self, 0xE828) == 0 ? At<int32_t*>(self, 0x3A58) : At<int32_t*>(self, 0x3A5C);
                    int32_t slot = slots[entry];
                    if (slot != -1) {
                        void* dict = CTxdStore::ms_pTxdPool->GetAt(slot)->dict;
                        if (dict) {
                            uint8_t unused = 1;
                            ForAllTextures(dict, kTextureInUseCallback, &unused);
                            if (!unused) {
                                continue;
                            }
                        }
                    }
                    // (for a tile without a slot too: model 0x2FA7)
                    slots = At<int32_t>(self, 0xE828) == 0 ? At<int32_t*>(self, 0x3A58) : At<int32_t*>(self, 0x3A5C);
                    int model = slots[entry] + 0x2FA8;
                    if (GetStreamingInfo(model)[4] == 1) {
                        RemoveModel(model);
                        At<uint8_t>(self, 0xE7F5) = 1;
                    }
                }
            }
        }
        if (At<uint8_t>(self, 0x1B8)) {
            Radar_DrawTiles_(self);
        }
        break;
    }
    case 1:
        At<uint8_t>(self, 0xE7F5) = 1;
        At<float>(self, 0xE740) = At<float>(self, 0xE7A4);
        At<float>(self, 0xE744) = At<float>(self, 0xE7A8);
        Radar_DrawMap_(self);
        break;
    case 2:
        At<uint8_t>(self, 0xE7F5) = 1;
        At<float>(self, 0xE740) = At<float>(self, 0xE7A4);
        At<float>(self, 0xE744) = At<float>(self, 0xE7A8);
        Radar_DrawMapTiles_(self);
        break;
    default:
        break;
    }
}

// 0x551290 (vf8 Draw): not in areas of kind 5 (the tiles off), the player's area
// kept for kind 6; the ring, the blips, the pulses, the map's legend
void __fastcall Draw(uint8_t* self, re::Edx) {
    int mode = Mode(self);
    At<uint8_t>(self, 0x1B8) = 1;
    if (mode == 0) {
        int kind = AreaKind(At<int32_t>(self, 0xE7F8));
        if (kind == 5) {
            At<uint8_t>(self, 0x1B8) = 0;
        } else if (kind == 6) {
            uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(g_playerPed + 0x14);
            const CVector* pos =
                matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : reinterpret_cast<const CVector*>(g_playerPed + 4);
            At<int32_t>(self, 0xE7F8) = Radar_GetArea(*pos);
            return;
        }
    }
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xC, 0);
    Radar_Update_(self);
    At<int32_t>(self, 0xE7F8) = Radar_GetArea(At<CVector>(self, 0xE748));
    if (mode == 0 && At<uint8_t>(self, 0x1B8) && g_widescreen == 0) {
        SetRenderState(6, 0);
        SetRenderState(8, 0);
        SetRenderState(0xC, 1);
        void* ring = g_radarRing;
        CRGBA dark = {0x10, 0x10, 0x10, 0xFF};
        CVector2D uv2 = {1.0f, 1.0f};
        CVector2D uv1 = {0.0f, 0.0f};
        DrawRectUV(At<float>(self, 0x138), At<float>(self, 0x13C), At<float>(self, 0x140), At<float>(self, 0x144), ring,
                   &dark, &uv1, &uv2);
    }
    Radar_DrawBlips_(self);
    Radar_DrawPulses(self);
    if (mode == 2) {
        Radar_DrawLegend(self);
    }
}

} // namespace hudradar

#define HUDRADARDRAW_REVERSED(name, address, fn)                                                       static void* RE_CONCAT(hudradardraw_original_, __LINE__);                                           REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudradardraw_original_, __LINE__),              re::Mode::Original)

HUDRADARDRAW_REVERSED("HUDRadar::DrawCollectibleBlip", 0x0054C050, &hudradar::DrawCollectibleBlip);
HUDRADARDRAW_REVERSED("HUDRadar::ToPlayersArea", 0x0054C0F0, &hudradar::ToPlayersArea);
HUDRADARDRAW_REVERSED("HUDRadar::FlashDoorBlips", 0x0054C290, &hudradar::FlashDoorBlips);
HUDRADARDRAW_REVERSED("HUDRadar::GetTileDict", 0x0054C500, &hudradar::GetTileDict);
HUDRADARDRAW_REVERSED("HUDRadar::DrawPolys", 0x0054C570, &hudradar::DrawPolys);
HUDRADARDRAW_REVERSED("HUDRadar::DrawOutlines", 0x0054C8F0, &hudradar::DrawOutlines);
HUDRADARDRAW_REVERSED("HUDRadar::DrawTile", 0x0054CA10, &hudradar::DrawTile);
HUDRADARDRAW_REVERSED("HUDRadar::DrawTileWhole", 0x0054CD10, &hudradar::DrawTileWhole);
HUDRADARDRAW_REVERSED("HUDRadar::DrawMask", 0x0054CF10, &hudradar::DrawMask);
HUDRADARDRAW_REVERSED("HUDRadar::UnloadTile", 0x0054D1F0, &hudradar::UnloadTile);
HUDRADARDRAW_REVERSED("HUDRadar::DrawCoordBlip", 0x0054D290, &hudradar::DrawCoordBlip);
HUDRADARDRAW_REVERSED("HUDRadar::ClampToCircle", 0x0054D620, &hudradar::ClampToCircle);
HUDRADARDRAW_REVERSED("HUDRadar::StreamTiles", 0x0054D700, &hudradar::StreamTiles);
HUDRADARDRAW_REVERSED("HUDRadar::DrawCone", 0x0054D860, &hudradar::DrawCone);
HUDRADARDRAW_REVERSED("HUDRadar::SetMode", 0x0054DEF0, &hudradar::SetMode);
HUDRADARDRAW_REVERSED("HUDRadar::SetCentre", 0x0054E120, &hudradar::SetCentre);
HUDRADARDRAW_REVERSED("HUDRadar::UpdateScale", 0x0054E200, &hudradar::UpdateScale);
HUDRADARDRAW_REVERSED("HUDRadar::DrawTiles", 0x0054E560, &hudradar::DrawTiles);
HUDRADARDRAW_REVERSED("HUDRadar::DrawMap", 0x0054E6E0, &hudradar::DrawMap);
HUDRADARDRAW_REVERSED("HUDRadar::MapControls", 0x0054EB10, &hudradar::MapControls);
HUDRADARDRAW_REVERSED("HUDRadar::DrawEntityBlip", 0x0054EE80, &hudradar::DrawEntityBlip);
HUDRADARDRAW_REVERSED("HUDRadar::Setup", 0x0054F9D0, &hudradar::Setup);
HUDRADARDRAW_REVERSED("HUDRadar::DrawBlips", 0x005500D0, &hudradar::DrawBlips);
HUDRADARDRAW_REVERSED("HUDRadar::DrawMapTiles", 0x00550A70, &hudradar::DrawMapTiles);
HUDRADARDRAW_REVERSED("HUDRadar::DrawBlipsOnly", 0x00550EA0, &hudradar::DrawBlipsOnly);
HUDRADARDRAW_REVERSED("HUDRadar::Update", 0x00550EC0, &hudradar::Update);
HUDRADARDRAW_REVERSED("HUDRadar::Draw", 0x00551290, &hudradar::Draw);
