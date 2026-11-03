// The radar's (HUD_RADAR) data and helpers: its .dat files (exit links, blip links, the
// areas' boxes), the map tiles' texture dictionaries (streamed, "radarNNN"), the
// rotation, the blips' reset, the world <-> map <-> screen transforms, the edge clipping,
// the legend's buttons and the direction arrow. 0x5483A0..0x54BE20 (the blips' small
// setters, the destructors are Hud.cpp's; the drawing after is HudRadarDraw.cpp's).
// All thiscall, written as __fastcall (self, edx, ...); the radar as raw offsets
// (vc::At), see HudRadarDraw.cpp's list.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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
// a vector passed by value in 16 bytes (x, y, z and a spare: 0x548FB0's arguments are 0x10 apart)
struct CVectorArg {
    float x, y, z, w;
};
static_assert(sizeof(CVectorArg) == 0x10);

// the file loader's
GAME_FN(void*, __cdecl, File_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(char*, __cdecl, FL_LoadLine, 0x00429C10, void* file);
GAME_FN(void, __cdecl, File_CloseFile, 0x0042D450, void* file);
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
template <typename... A>
inline int __cdecl Sprintf(char* out, const char* format, A... rest) { return crt::Sprintf(out, format, rest...); }
GAME_FN(int, __cdecl, GetAreaFromName, 0x004305D0, const char* name);
GAME_FN(int, __cdecl, AreaKindFromName, 0x005DAEC0, const char* name);
// textures, streaming
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void, __thiscall, CSprite2d_SetTexture, 0x0052C230, void* sprite, const char* name);
GAME_FN(void, __cdecl, CStreaming_RequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __cdecl, CStreaming_RemoveModel, 0x0052F640, int model);
// entities by handle
GAME_FN(uint8_t*, __cdecl, CPools_GetPed, 0x0044A550, int handle);
GAME_FN(uint8_t*, __cdecl, CPools_GetVehicle, 0x0044A580, int handle);
GAME_FN(uint8_t*, __cdecl, CPools_GetObject, 0x0044A5B0, int handle);
GAME_FN(uint8_t*, __cdecl, CPools_GetPropAnim, 0x0044A5E0, int handle);
// drawing, text
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, DrawSprite, 0x0052D610, const float* rect, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __thiscall, DrawQuadCorners, 0x0052C7F0, void* sprite, float x1, float y1, float x2, float y2, float x3, float y3,
        float x4, float y4, const CRGBA* colour);
GAME_FN(void*, __cdecl, GetButtonTexture, 0x00514EC0, int button);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __thiscall, HUDComponent_Reset, 0x0053CF00, void* self);
// its own, where modes apply
GAME_FN(void, __thiscall, Radar_SetBlipEntityFlag, 0x00548D30, void* radar, int index, int flag);

GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(float, g_cameraHeading, 0x00C3CD08);
GAME_VAR(float, g_arrowStep, 0x00AB7388);             // (pi/2: the arrow's corners)
GAME_VAR(float, g_arrowTurn, 0x00AB738C);             // (pi/4)
GAME_ARRAY(uint8_t, g_radarAreas, 0x00CF01E8);        // [64] 0x20 each: min x, max x, min y, max y, min z,
                                                      // max z, the kind, the map's turn
GAME_ARRAY(uint8_t, g_areaTurns, 0x00AB73A8);         // {area, turn} pairs, area -1 ends
GAME_ARRAY(const char*, g_radarSpriteNames, 0x00AB72FC);  // [2] ("radar_centre", ...)
GAME_ARRAY(uint8_t, g_blipSprites, 0x0091A3E8);       // [39] 0x14 each, the name first
GAME_ARRAY(uint8_t, g_legendButtons, 0x0091A6F8);     // [] 0xC each: the text key, the button
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);

// the tiles' txd slots: by tile (storeys), and the ground's and winter ground's by pair
int32_t* StoreySlots(void* radar) {
    return At<int32_t*>(radar, 0x3A54);
}
int32_t* GroundSlots(void* radar) {
    return At<int32_t*>(radar, 0x3A58);
}
int32_t* WinterSlots(void* radar) {
    return At<int32_t*>(radar, 0x3A5C);
}
int32_t& Mode(void* radar) {
    return At<int32_t>(radar, 0xE788);
}
int32_t& Cells(void* radar) {
    return At<int32_t>(radar, 0x130);
}

// ---- the .dat files ----------------------------------------------------------------------

// 0x5483A0: Config\Dat\ExitLocs.dat: "area link x y z", where an area's links are
void __fastcall Radar_LoadExitLocs(uint8_t* self, re::Edx) {
    void* file = File_OpenFile("Config\\Dat\\ExitLocs.dat", "r", 1);
    char name[0x30];
    uint32_t link;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    for (char* line = FL_LoadLine(file); line; line = FL_LoadLine(file)) {
        if (*line == 0 || *line == '#') {
            continue;
        }
        Sscanf(line, "%s %d %f %f %f", name, &link, &x, &y, &z);
        int area = GetAreaFromName(name);
        if (area >= 0 && link <= 0x23) {
            CVector& p = At<CVector>(self, 0x7B34 + int(link + area * 0x24) * 0xC);
            p.x = x;
            p.y = y;
            p.z = z;
        }
    }
    File_CloseFile(file);
}

// 0x548460: Config\Dat\BlipLocs.dat: "area" then 64 numbers (an area's links by area)
void __fastcall Radar_LoadBlipLocs(uint8_t* self, re::Edx) {
    void* file = File_OpenFile("Config\\Dat\\BlipLocs.dat", "r", 1);
    std::memset(self + 0x3B34, 0, 0x1000 * 4);
    char name[0x30];
    int32_t unused;
    for (char* line = FL_LoadLine(file); line; line = FL_LoadLine(file)) {
        if (*line == 0 || *line == '#') {
            continue;
        }
        Sscanf(line, "%s", name, &unused);
        int area = GetAreaFromName(name);
        char* p = line;
        while (*p != ' ') {
            p++;
        }
        if (area < 0) {
            continue;
        }
        int32_t* out = &At<int32_t>(self, 0x3B34 + area * 0x100);
        char c = *p;
        for (int i = 0; i < 0x40; i++) {
            if (c == ' ') {
                do {
                    p++;
                } while (*p == ' ');
            }
            Sscanf(p, "%d", out);
            // (to the next space: past the line's end when there's none, as the game does)
            if (*p != ' ') {
                do {
                    c = p[1];
                    p++;
                } while (c != ' ');
            }
            c = ' ';
            out++;
        }
    }
    File_CloseFile(file);
}

// 0x548560: Config\Dat\areabbox.dat: the areas' boxes and kinds (all reset first), the
// map's turn from the table at 0xAB73A8
void __cdecl Radar_LoadAreaBoxes() {
    for (int i = 0; i < 0x40; i++) {
        uint8_t* e = g_radarAreas + i * 0x20;
        for (int k = 0; k < 6; k++) {
            reinterpret_cast<float*>(e)[k] = 0.0f;
        }
        *reinterpret_cast<int32_t*>(e + 0x18) = 6;
        *reinterpret_cast<float*>(e + 0x1C) = 0.0f;
    }
    void* file = File_OpenFile("Config\\Dat\\areabbox.dat", "r", 1);
    char name[0x30];
    char kind[0x30];
    float v[6];
    for (char* line = FL_LoadLine(file); line; line = FL_LoadLine(file)) {
        if (*line == 0 || *line == '#') {
            continue;
        }
        Sscanf(line, "%s %f %f %f %f %f %f %s", name, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], kind);
        int area = GetAreaFromName(name);
        if (area < 0) {
            continue;
        }
        uint8_t* e = g_radarAreas + area * 0x20;
        int32_t k = AreaKindFromName(kind);
        reinterpret_cast<float*>(e)[0] = v[0];
        *reinterpret_cast<int32_t*>(e + 0x18) = k;
        for (int j = 1; j < 6; j++) {
            reinterpret_cast<float*>(e)[j] = v[j];
        }
        *reinterpret_cast<float*>(e + 0x1C) = 0.0f;
        for (const uint8_t* t = g_areaTurns; *reinterpret_cast<const int32_t*>(t) != -1; t += 8) {
            if (area == *reinterpret_cast<const int32_t*>(t)) {
                *reinterpret_cast<float*>(e + 0x1C) = *reinterpret_cast<const float*>(t + 4);
                break;
            }
        }
    }
    File_CloseFile(file);
}

// ---- tiles ---------------------------------------------------------------------------------

// 0x5486D0: a tile's corners on the map (the tile's size apart, y counted from the top)
void __fastcall Radar_TileCorners(uint8_t* self, re::Edx, float x, float y, CVector2D* out) {
    float half = float(Cells(self)) * 0.5f;
    float cx = x - half;
    float cy = (float(Cells(self) - 1) - y) - half;
    float size = At<float>(self, 0xE76C);
    out[0].x = size * cx;
    out[0].y = size * cy;
    float cx1 = cx + 1.0f;
    out[1].x = size * cx1;
    out[1].y = size * cy;
    out[2].x = size * cx1;
    float cy1 = cy + 1.0f;
    out[2].y = size * cy1;
    out[3].x = size * cx;
    out[3].y = cy1 * size;
}

// 0x548790: a map position's texture coordinates in a tile (0..8), by the mode's scale
void __fastcall Radar_TileUV(uint8_t* self, re::Edx, CVector2D* out, const CVector2D* in, int tx, int ty) {
    float u = 0.0f;
    float v = 0.0f;
    int mode = Mode(self);
    if (mode == 2) {
        float half = float(Cells(self)) * 0.5f;
        float sx = At<float>(self, 0xE778);
        float sy = At<float>(self, 0xE77C);
        u = (in->x - (float(tx) - half) * sx) / sx;
        v = (in->y - sy * (float(Cells(self) - ty) - half)) / sy;
    } else if (mode == 0) {
        float cells = float(Cells(self));
        float half = cells * 0.5f;
        float size = At<float>(self, 0xE76C);
        u = (in->x - (float(tx) - half) * size) / size;
        v = -(in->y - ((cells - float(ty)) - half) * size) / size;
    } else if (mode == 1) {
        float mapCells = At<float>(self, 0x170);
        float half = mapCells * 0.5f;
        float sx = At<float>(self, 0xE780);
        float sy = At<float>(self, 0xE784);
        u = (in->x - (float(tx) - half) * sx) / sx;
        v = (in->y - sy * ((mapCells - float(ty)) - half)) / sy;
    }
    out->x = u < 0.0f ? 0.0f : (u > 8.0f ? 8.0f : u);
    out->y = v < 0.0f ? 0.0f : (v > 8.0f ? 8.0f : v);
}

// 0x548960: the radar's turn: the camera's (the map: a quarter turn; or a set one)
void __fastcall Radar_UpdateRotation(uint8_t* self, re::Edx) {
    float angle;
    if (Mode(self) == 2 || Mode(self) == 1) {
        At<float>(self, 0xE738) = float(std::sin(1.5707963705062866));
        At<float>(self, 0xE73C) = float(std::cos(1.5707963705062866));
        return;
    }
    if (!self[0xE811]) {
        // (the sine a half turn back, the cosine not)
        angle = float(double(g_cameraHeading) - double(3.14159274f));
        At<float>(self, 0xE738) = float(std::sin(double(angle)));
        At<float>(self, 0xE73C) = float(std::cos(double(g_cameraHeading)));
        return;
    }
    At<float>(self, 0xE738) = float(std::sin(double(At<float>(self, 0xE80C))));
    At<float>(self, 0xE73C) = float(std::cos(double(At<float>(self, 0xE80C))));
}

// 0x548CC0: a point in an area's box, 3 units round it (z unused)
uint8_t __cdecl Radar_IsInAreaBox(float x, float y, float, int area) {
    const float* box = reinterpret_cast<const float*>(g_radarAreas + area * 0x20);
    if (!(box[0] - 3.0f < x)) {
        return 0;
    }
    float maxX = box[1] + 3.0f;
    if (maxX < x || maxX == x) {
        return 0;
    }
    if (!(box[2] - 3.0f < y)) {
        return 0;
    }
    if (!(y < box[3] + 3.0f)) {
        return 0;
    }
    return 1;
}

// 0x548D30: a blip's entity's +0x54 (its "on the radar" flag), by the blip's type
void __fastcall Radar_SetBlipEntityFlag_(uint8_t* self, re::Edx, int index, int flag) {
    int handle = At<int32_t>(self, 0x210 + index * 0x30);
    uint8_t* entity;
    switch (At<int32_t>(self, 0x230 + index * 0x30)) {
    case 1:
    case 0xC:
        entity = CPools_GetVehicle(handle);
        break;
    case 2:
    case 6:
    case 0xD:
        entity = CPools_GetPed(handle);
        break;
    case 3:
    case 5:
    case 9:
    case 10:
        entity = CPools_GetObject(handle);
        break;
    case 7:
        entity = CPools_GetPropAnim(handle);
        break;
    default:
        return;
    }
    if (entity) {
        *reinterpret_cast<uint32_t*>(entity + 0x54) = uint8_t(flag);
    }
}

// 0x548EA0: a tile's dictionaries found ("radarNNN"; the pair's EXT and EXTwin ones once)
void __fastcall Radar_FindTileTxds(uint8_t* self, re::Edx, int tile) {
    char name[0x20];
    Sprintf(name, "radar%03d", tile);
    StoreySlots(self)[tile] = Txd_FindTxdSlot(name, 0);
    int pair = tile >> 1;
    if (GroundSlots(self)[pair] == -1) {
        Sprintf(name, "EXTradar%03d", tile | 1);
        GroundSlots(self)[pair] = Txd_FindTxdSlot(name, 0);
    }
    if (WinterSlots(self)[pair] == -1) {
        Sprintf(name, "EXTwinradar%03d", tile | 1);
        WinterSlots(self)[pair] = Txd_FindTxdSlot(name, 0);
    }
}

// 0x548FB0: p in the triangle abc (by the signs of the three cross products; z unused)
uint8_t __cdecl Radar_PointInTriangle(CVectorArg p, CVectorArg a, CVectorArg b, CVectorArg c) {
    float pax = p.x - a.x;
    float pay = p.y - a.y;
    float bax = b.x - a.x;
    float bay = b.y - a.y;
    float pcx = p.x - c.x;
    float pcy = p.y - c.y;
    float acx = a.x - c.x;
    float acy = a.y - c.y;
    float pbx = p.x - b.x;
    float pby = p.y - b.y;
    float cbx = c.x - b.x;
    float cby = c.y - b.y;
    float crossB = pby * cbx - cby * pbx;
    float crossA = pay * bax - bay * pax;
    if (!(0.0f < crossA * crossB)) {
        return 0;
    }
    float crossC = pcy * acx - acy * pcx;
    return crossB * crossC > 0.0f;
}

// 0x549180: every blip free
void __fastcall Radar_ClearBlips(uint8_t* self, re::Edx) {
    for (int i = 0; i < 300; i++) {
        uint8_t* blip = self + 0x210 + i * 0x30;
        *reinterpret_cast<int32_t*>(blip + 0x14) = 1;
        Radar_SetBlipEntityFlag(self, i, 0);
        blip[0x2F] &= 0xE1;
        *reinterpret_cast<int32_t*>(blip + 0x20) = 0;
        *reinterpret_cast<int32_t*>(blip + 0x24) = 0;
        *reinterpret_cast<int32_t*>(blip + 0x28) = -1;
        *reinterpret_cast<int32_t*>(blip + 0x18) = 0;
        *reinterpret_cast<int16_t*>(blip + 0x2C) = -1;
    }
}

// 0x5491D0 (vf7 Reset)
void __fastcall Radar_Reset(uint8_t* self, re::Edx) {
    HUDComponent_Reset(self);
    auto* c = reinterpret_cast<HUDComponent*>(self);
    reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(c->vtable[5]))(c, 1);
    At<int32_t>(self, 0xE830) = -1;
    At<float>(self, 0x3A50) = float(At<int32_t>(self, 0xE7DC));
    self[0xE810] = 0;
    self[0xE811] = 0;
    At<float>(self, 0x120) = 0.0f;
    self[0xE7F4] = 0;
    self[0xE812] = 0;
    At<uint32_t>(self, 0xE834) = 0;
    for (int i = 0; i < 8; i++) {
        At<float>(self, 0x38 + i * 0x20) = 0.0f;
    }
    self[0xE844] = 1;
}

// 0x549260: its sprites' textures from HudElems (the two of its own, the blips')
void __fastcall Radar_LoadSprites(uint8_t* self, re::Edx) {
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(Txd_FindTxdSlot("HudElems", 0));
    for (int i = 0; i < 2; i++) {
        const char* name = g_radarSpriteNames[i];
        if (*name) {
            CSprite2d_SetTexture(self + 0x3A60 + i * 4, name);
        }
    }
    for (int i = 0; i < 39; i++) {
        const char* name = *reinterpret_cast<const re::Ptr32<const char>*>(g_blipSprites + i * 0x14);
        if (*name) {
            CSprite2d_SetTexture(self + 0x3A98 + i * 4, name);
        }
    }
    Txd_PopCurrentTxd();
}

// 0x5492E0: every tile's dictionaries let go
void __fastcall Radar_RemoveTiles(uint8_t* self, re::Edx) {
    for (int y = 0; y < Cells(self); y++) {
        for (int x = 0; x < Cells(self); x++) {
            int tile = Cells(self) * x + y;
            CStreaming_RemoveModel(StoreySlots(self)[tile] + 0x2FA8);
            int pair = tile >> 1;
            int slot = At<int32_t>(self, 0xE828) == 0 ? GroundSlots(self)[pair] : WinterSlots(self)[pair];
            CStreaming_RemoveModel(slot + 0x2FA8);
        }
    }
}

// 0x549370: a legend button (its button's texture at its aspect, at most 100 wide) and
// its text under it
void __fastcall Radar_DrawLegendButton(uint8_t* self, re::Edx, int index, float x) {
    float halfW = g_screenScaleX;
    float halfH = g_screenScaleY;
    const uint8_t* entry = g_legendButtons + index * 0xC;
    void* texture = GetButtonTexture(*reinterpret_cast<const int32_t*>(entry + 4));
    if (texture) {
        re::Ptr32<void>* vt = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture);
        uint32_t w = re::PtrCast<uint32_t(__thiscall*)(void*)>(vt[0x48 / 4])(texture);
        float wide = float(w) * At<float>(self, 0x1FC);
        uint32_t h = re::PtrCast<uint32_t(__thiscall*)(void*)>(vt[0x4C / 4])(texture);
        float width = float(double(wide) / double(float(h)));
        if (!(width < 100.0f)) {
            width = 100.0f;
        }
        halfW = width * 0.5f * halfW;
        halfH = 0.5f * At<float>(self, 0x1FC) * halfH;
    }
    float rect[4];
    float y = At<float>(self, 0x1E8);
    rect[0] = x - halfW;
    rect[3] = y - halfH;
    rect[2] = x + halfW;
    rect[1] = halfH + y;
    SetRenderState(0xC, 1);
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    CRGBA white;
    RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
    DrawSprite(rect, texture, &white, &uv1, &uv2);
    Font_Begin();
    Font_SetJustify(2);
    if (g_language == 7) {
        Font_SetScale(0.4f);
    } else {
        Font_SetSizePreset(1);
    }
    uint16_t text[0x80];
    Font_FormatText(text, Text_Get(kTheText, *reinterpret_cast<const re::Ptr32<const char>*>(entry)), 0x80);
    Font_PrintString(x, At<float>(self, 0x20C), text, 1);
}

// 0x549560: the direction arrow (a square turned by the camera or the map's turn),
// bigger on the map as it zooms in, drawn with `sprite`
void __fastcall Radar_DrawArrow(uint8_t* self, re::Edx, void* sprite, float x, float y, float angle, int alpha) {
    float size = 11.0f;
    int mode = Mode(self);
    if (mode == 2 || mode == 1) {
        angle = (g_arrowStep + g_arrowTurn) + angle;
        if (mode == 2) {
            float lo = At<float>(self, 0x168) / At<float>(self, 0x158);
            float hi = At<float>(self, 0x168) / At<float>(self, 0x154);
            float t = (At<float>(self, 0xE7B0) - lo) / (hi - lo);
            float grow = (1.0f - t) * 1.5f + 1.0f;
            size = At<float>(self, 0x148) * grow;
        }
    } else if (mode == 0) {
        angle = (g_cameraHeading - g_arrowTurn) + angle;
    }
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    CVector2D p[5];
    for (int i = 0; i < 4;) {
        float a = float(i) * g_arrowStep + angle;
        float s = float(std::sin(double(a)));
        float c = float(std::cos(double(a)));
        i++;
        p[i].x = ((c * 0.0f + s) * size) * sx + x;
        p[i].y = ((c - s * 0.0f) * size) * sy + y;
    }
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    CRGBA colour;
    RGBA_Construct(&colour, 0xFF, 0xFF, 0xFF, alpha);
    DrawQuadCorners(sprite, p[3].x, p[3].y, p[4].x, p[4].y, p[2].x, p[2].y, p[1].x, p[1].y, &colour);
}

// 0x549740: a tile's dictionary asked for (the radar's storeys, else the (winter)
// ground's; the map outside the middle only on the radar)
void __fastcall Radar_RequestTile(uint8_t* self, re::Edx, int x, int y) {
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
    int mode = Mode(self);
    int tile = Cells(self) * y + x;
    bool storeys = (mode == 2 || mode == 1) ? false : At<int32_t>(self, 0xE734) != 0;
    if ((x < 7 || y <= 1 || y >= 0x13) && mode != 0) {
        return;
    }
    int slot;
    if (storeys) {
        slot = StoreySlots(self)[tile];
    } else if (At<int32_t>(self, 0xE828) == 0) {
        slot = GroundSlots(self)[tile >> 1];
    } else {
        slot = WinterSlots(self)[tile >> 1];
    }
    CStreaming_RequestModel(slot + 0x2FA8, 0xD);
}

// 0x5497F0: where the segment a..b leaves the square -1..1 (the nearest crossing):
// 3 left, 1 right, 0 bottom, 2 top, -1 none. (thiscall in its callers, ecx unused)
int __fastcall Radar_ClipEdge(void*, re::Edx, CVector2D* out, const CVector2D* a, const CVector2D* b) {
    float best = 1.0f;
    int edge = -1;
    float da = -1.0f - a->x;
    float db = -1.0f - b->x;
    if (db * da < 0.0f) {
        float t = da / (da - db);
        float y = (b->y - a->y) * t + a->y;
        if (y >= -1.0f && y <= 1.0f && t <= 1.0f) {
            edge = 3;
            out->x = -1.0f;
            out->y = y;
            best = t;
        }
    }
    da = a->x - 1.0f;
    db = b->x - 1.0f;
    if (db * da < 0.0f) {
        float t = da / (da - db);
        float y = (b->y - a->y) * t + a->y;
        if (y >= -1.0f && y <= 1.0f && t <= best) {
            edge = 1;
            out->x = 1.0f;
            out->y = y;
            best = t;
        }
    }
    da = -1.0f - a->y;
    db = -1.0f - b->y;
    if (db * da < 0.0f) {
        float t = da / (da - db);
        float x = (b->x - a->x) * t + a->x;
        if (x >= -1.0f && x <= 1.0f && t <= best) {
            out->x = x;
            edge = 0;
            out->y = -1.0f;
            best = t;
        }
    }
    da = a->y - 1.0f;
    db = b->y - 1.0f;
    if (db * da < 0.0f) {
        float t = da / (da - db);
        float x = (b->x - a->x) * t + a->x;
        if (x >= -1.0f && x <= 1.0f && t <= best) {
            out->x = x;
            out->y = 1.0f;
            return 2;
        }
    }
    return edge;
}

// 0x549A90: a world position to the radar's (centred on the player, zoomed, turned) or
// the map's (turned)
void __fastcall Radar_WorldToMap(uint8_t* self, re::Edx, CVector2D* out, const CVector2D* in) {
    float x = in->x;
    float y = in->y;
    int mode = Mode(self);
    float s = At<float>(self, 0xE738);
    float c = At<float>(self, 0xE73C);
    if (mode == 2 || mode == 1) {
        out->x = c * x - s * y;
        out->y = c * y + s * x;
    } else if (mode == 0) {
        x = x - At<float>(self, 0xE740);
        y = y - At<float>(self, 0xE744);
        float inv = 1.0f / At<float>(self, 0x3A50);
        x = inv * x;
        y = inv * y;
        out->x = c * x + s * y;
        out->y = c * y - s * x;
    }
}

// 0x549B80: ... and back to the world
void __fastcall Radar_MapToWorld(uint8_t* self, re::Edx, CVector2D* out, const CVector2D* in) {
    float s = At<float>(self, 0xE738);
    float c = At<float>(self, 0xE73C);
    out->x = in->x * c - s * in->y;
    out->y = in->x * s + c * in->y;
    float zoom = At<float>(self, 0x3A50);
    out->x = out->x * zoom;
    out->y = out->y * zoom;
    out->x = out->x + At<float>(self, 0xE740);
    out->y = out->y + At<float>(self, 0xE744);
}

} // namespace

// Four are written but left unhooked: their callers keep values in ecx / edx across the
// call (the originals leave them alone; compiled code needn't). Registered with their bodies
// all the same, for the 64-bit build's calls by address.
#define HUDRADAR_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(hudradar_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudradar_original_, __LINE__), \
             re::Mode::Original)

HUDRADAR_REVERSED("HUDRadar::LoadExitLocs", 0x005483A0, &Radar_LoadExitLocs);
HUDRADAR_REVERSED("HUDRadar::LoadBlipLocs", 0x00548460, &Radar_LoadBlipLocs);
HUDRADAR_REVERSED("HUDRadar::LoadAreaBoxes", 0x00548560, &Radar_LoadAreaBoxes);
RE_UNHOOKED_FN("HUDRadar::TileCorners", 0x005486D0, &Radar_TileCorners);  // 0x54CD10 keeps the cells in edx across the call
HUDRADAR_REVERSED("HUDRadar::TileUV", 0x00548790, &Radar_TileUV);
HUDRADAR_REVERSED("HUDRadar::UpdateRotation", 0x00548960, &Radar_UpdateRotation);
RE_UNHOOKED_FN("HUDRadar::IsInAreaBox", 0x00548CC0, &Radar_IsInAreaBox);  // 0x54AA20 keeps the area in edx across the call
HUDRADAR_REVERSED("HUDRadar::SetBlipEntityFlag", 0x00548D30, &Radar_SetBlipEntityFlag_);
HUDRADAR_REVERSED("HUDRadar::FindTileTxds", 0x00548EA0, &Radar_FindTileTxds);
RE_UNHOOKED_FN("HUDRadar::PointInTriangle", 0x00548FB0, &Radar_PointInTriangle);  // 0x54B0A0 keeps its tables in ecx / edx across the call
HUDRADAR_REVERSED("HUDRadar::ClearBlips", 0x00549180, &Radar_ClearBlips);
HUDRADAR_REVERSED("HUDRadar::Reset", 0x005491D0, &Radar_Reset);
HUDRADAR_REVERSED("HUDRadar::LoadSprites", 0x00549260, &Radar_LoadSprites);
HUDRADAR_REVERSED("HUDRadar::RemoveTiles", 0x005492E0, &Radar_RemoveTiles);
HUDRADAR_REVERSED("HUDRadar::DrawLegendButton", 0x00549370, &Radar_DrawLegendButton);
HUDRADAR_REVERSED("HUDRadar::DrawArrow", 0x00549560, &Radar_DrawArrow);
HUDRADAR_REVERSED("HUDRadar::RequestTile", 0x00549740, &Radar_RequestTile);
HUDRADAR_REVERSED("HUDRadar::ClipEdge", 0x005497F0, &Radar_ClipEdge);
RE_UNHOOKED_FN("HUDRadar::WorldToMap", 0x00549A90, &Radar_WorldToMap);  // 0x54BE20 keeps the radar in ecx across the call
HUDRADAR_REVERSED("HUDRadar::MapToWorld", 0x00549B80, &Radar_MapToWorld);
