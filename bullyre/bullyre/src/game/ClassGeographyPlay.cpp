// Geography class (ClassGeography): the play: the pads, picking up and dropping flags,
// the cursors' snapping to targets, and the drawing of the map, slots, targets, labels,
// the timer and the score (0x6F3540..0x6F6E10).
//
// Sizes: a texture drawn at its pixel size times half the screen scale (flags 0.65 high);
// the constants the game multiplies by in double (0.65, 0.7, 0.749, 1.049, 1.2, 0.05) are
// kept double here, rounded to float after each step as the x87 does at 24-bit precision.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/ClassGeography.h"
#include "game/TrackMath.h"
#include "reversed.h"

namespace {

using namespace school;
using namespace geo;

GAME_VAR(float, geoTimerRate, 0x00BC6624);     // (0.02)
GAME_VAR(float, geoScoreTitleY, 0x00BC663C);
GAME_VAR(float, geoScoreY, 0x00BC6628);
GAME_VAR(int16_t, geoFontJustify, 0x0208B786);
// a rect with a colour per corner
GAME_FN(void, __cdecl, DrawRectUV4, 0x0052CB60, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* c1, const uint8_t* c2, const uint8_t* c3, const uint8_t* c4, const float* uv1,
        const float* uv2);

const float kUv0[2] = {0.0f, 0.0f};
const float kUv1[2] = {1.0f, 1.0f};

// a texture centred on a screen point, half-sizes given
inline void DrawCentred(const float* centre, float halfWidth, float halfHeight, void* texture, const uint8_t* colour) {
    DrawRectUV(centre[0] - halfWidth, centre[1] - halfHeight, centre[0] + halfWidth, centre[1] + halfHeight, texture,
               colour, kUv0, kUv1);
}
// a float times a double constant, rounded back
inline float MulD(float value, double factor) {
    return float(double(value) * factor);
}
inline float DivD(float value, double divisor) {
    return float(double(value) / divisor);
}
inline bool IsCursor(const GeoPoint& cursor, float x, float y) {
    return cursor.x == x && cursor.y == y;
}
inline void CopyColour(uint8_t* to, const uint8_t* from) {
    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2];
    to[3] = from[3];
}

// a stick direction (the d-pad button, else the two sticks' sum, negated)
inline float StickAxis(int button, int stickA, int stickB, int pad, float buttonValue) {
    if (Pad_GetValue(button, pad) == 0.0f) {
        float b = Pad_GetValue(stickB, pad);
        float a = Pad_GetValue(stickA, pad);
        return -(a + b);
    }
    return buttonValue;
}

// 0x6F3540: the pad's direction (x: left 1, right -1; y: up -1, down 1); `self` unused
GeoPoint* __stdcall GeoReadPad(GeoPoint* out, int pad) {
    out->x = 0.0f;
    out->y = 0.0f;
    if (Pad_GetValue(1, pad) != 0.0f || Pad_GetValue(0x10, pad) < -0.05 || Pad_GetValue(0x12, pad) < -0.05) {
        out->x = StickAxis(1, 0x10, 0x12, pad, 1.0f);
    }
    if (Pad_GetValue(0, pad) != 0.0f || Pad_GetValue(0x10, pad) > 0.05 || Pad_GetValue(0x12, pad) > 0.05) {
        out->x = StickAxis(0, 0x10, 0x12, pad, -1.0f);
    }
    if (Pad_GetValue(2, pad) != 0.0f || Pad_GetValue(0x11, pad) > 0.05 || Pad_GetValue(0x13, pad) > 0.05) {
        out->y = StickAxis(2, 0x11, 0x13, pad, -1.0f);
    }
    if (Pad_GetValue(3, pad) != 0.0f || Pad_GetValue(0x11, pad) < -0.05 || Pad_GetValue(0x13, pad) < -0.05) {
        out->y = StickAxis(3, 0x11, 0x13, pad, 1.0f);
    }
    return out;
}

// 0x6F3780: a cursor (the one at x, y) picks up the flag under it (40 units), with a rumble
void __fastcall GeoPickUp(ClassGeography* g, re::Edx, float x, float y) {
    if (g->fading != 0) {
        return;
    }
    for (uint32_t i = 0; i < g->countryCount; ++i) {
        if (g->placed[i] != 0) {
            continue;
        }
        int32_t id = g->ids[i];
        GeoPoint slot = g->slotPos[i];
        if (IsCursor(g->cursor[0], x, y)) {
            g->held1 = slot;
        } else {
            g->held2 = slot;
        }
        float dist = Distance2D(x - slot.x, y - slot.y);
        if (!(dist <= 40.0f)) {
            continue;
        }
        if (IsCursor(g->cursor[0], x, y) && (g->hover[0] == 0 || g->hoverId[0] != id)) {
            g->selected[0] = i;
            g->hover[0] = 1;
            g->hoverId[0] = id;
            PlayClassSound(g, GEO_SOUND_SELECT_FLAG);
            Pad_Rumble(0, 0x50, 0xE, 1);
        }
        if (!(g->cursor[1].x == x) || !(g->cursor[1].y == y)) {
            return;
        }
        if (g->hover[1] != 0 && g->hoverId[1] == id) {
            return;
        }
        g->selected[1] = i;
        g->hoverId[1] = id;
        g->hover[1] = 1;
        PlayClassSound(g, GEO_SOUND_SELECT_FLAG);
        Pad_Rumble(1, 0x50, 0xE, 1);
        return;
    }
}

// 0x6F39C0: a flag dropped at x, y: placed when it is its country's (within the player's
// range): the star, the score (6.25), the next flag up into the freed slot. 1 when all are
// placed
uint8_t __fastcall GeoTryPlace(ClassGeography* g, re::Edx, float x, float y, int flag, int player) {
    int count = g->remaining;
    int placedCount = 0;
    for (int i = 0; i < count; ++i) {
        if (g->placed[i] == 1) {
            ++placedCount;
        }
    }
    uint32_t nextSlot = uint32_t(placedCount + 8);
    int k = placedCount + 7;   // the slot (and its flag) brought up next
    for (int i = 0; i < g->remaining; ++i) {
        if (g->placed[i] != 0 || flag != i) {
            continue;
        }
        float dist = Distance2D(x - g->countryPos[i].x, y - g->countryPos[i].y);
        if (g->range[player] < dist) {
            continue;
        }
        Pad_Rumble(player, 0xFA, 0x18, 1);
        PlayClassSound(g, GEO_SOUND_DROP_CORRECT);
        g->placed[i] = 1;
        g->invalidOperation = 0;
        g->validOperation = 1;
        g->dropPos.x = x;
        ++placedCount;
        g->dropPos.y = y;
        ++nextSlot;
        g->starTime = 0.0f;
        g->starSize = 100;
        g->starShown = 1;
        ++k;
        uint32_t countryCount;
        if (IsCursor(g->cursor[0], x, y)) {
            CopyColour(g->colours[i], GeoColour(0x020C8328));
            countryCount = g->countryCount;
            g->selected[0] = countryCount;
            g->hoverId[0] = 2;
            g->hover[0] = 0;
        } else {
            CopyColour(g->colours[i], GeoColour(0x020C834C));
            countryCount = g->countryCount;
            g->selected[1] = countryCount;
            g->hoverId[1] = 2;
            g->hover[1] = 0;
        }
        g->score = g->score + 6.25f;
        if (nextSlot > countryCount) {
            continue;
        }
        if (IsCursor(g->cursor[0], x, y)) {
            g->slotPos[k] = g->held1;
            g->anim1.x = g->held1.x;
            g->anim1Id = g->slotIds[k];
            g->anim1.y = g->held1.y + 250.0f;
            PlayClassSound(g, GEO_SOUND_ROLL_OVER_FLAG);
            g->anim1Active = 1;
        } else {
            g->slotPos[k] = g->held2;
            g->anim2.x = g->held2.x;
            g->anim2Id = g->slotIds[k];
            g->anim2.y = g->held2.y + 250.0f;
            PlayClassSound(g, GEO_SOUND_ROLL_OVER_FLAG);
            g->anim2Active = 1;
        }
    }
    return placedCount == g->remaining ? 1 : 0;
}

// 0x6F3D50: the next map fading in; on its first frame the old map's flags are kept to fall
// away and the next map is set up (Europe, the USA, Asia, Africa, Central America, Canada,
// southern Africa, Oceania, the north-east, Europe again)
void __fastcall GeoDrawNextMap(ClassGeography* g, re::Edx) {
    if (g->fade == 0xFF) {
        PlayClassSound(g, GEO_SOUND_NEW_MAP);
        g->prevMap = g->map;
        g->prevCount = int32_t(g->countryCount);
        g->dealing = 1;
        for (int i = 0; i < g->prevCount; ++i) {
            g->prevTexIds[i] = g->texIds[i];
            g->prevPos[i] = g->countryPos[i];
            g->falling[i] = 0;
            g->fall[i] = 0;
        }
        switch (g->map) {
        case 0: Geo_SetupUsa(g); break;
        case 1: Geo_SetupAfrica(g); break;
        case 2: Geo_SetupCentralAmerica(g); break;
        case 3: Geo_SetupAsia(g); break;
        case 4: Geo_SetupCanada(g); break;
        case 5: Geo_SetupSouthernAfrica(g); break;
        case 6: Geo_SetupOceania(g); break;
        case 7: Geo_SetupNorthEast(g); break;
        default: Geo_SetupEurope(g); break;
        }
        std::memset(g->dealt, 0, sizeof(g->dealt));
        Geo_ShuffleCountries(g);
        for (uint32_t i = 0; i < g->countryCount; ++i) {
            g->slotIds[i] = g->ids[i];
        }
        for (uint32_t k = 8; k < g->countryCount; ++k) {
            g->slotPos[k] = {160.0f, 330.0f};
        }
        for (uint32_t i = 0; i < g->countryCount; ++i) {
            g->placed[i] = 0;
        }
    }
    uint8_t rgba[4];
    uint8_t colour[4];
    CopyColour(colour, RGBA_Set(rgba, 0, 0, 0, uint8_t(g->fade)));
    float u = widescreen != 0 ? 0.0f : 0.125f;
    const float uv1[2] = {u, 0.0f};
    const float uv2[2] = {1.0f - u, 1.0f};
    void* map = Geo_GetTexture(g, g->map, 0);
    DrawRectUV(0.0f, 0.0f, 640.0f, 480.0f, map, colour, uv1, uv2);
    g->fade -= 2;
    if (g->fade <= 0) {
        g->fade = 0xFF;
        g->fading = 0;
    }
}

// 0x6F3FF0: the map (Oceania's cropped differently on 4:3)
void __fastcall GeoDrawMap(ClassGeography* g, re::Edx) {
    void* map = nullptr;   // (the game reads uninitialised stack for a map past 8)
    if (uint32_t(g->map) <= 8) {
        map = g->textureSets[g->map][0];
    }
    float uv1[2];
    float uv2[2];
    if (g->map == 7) {
        float a = 0.0f;
        float c = 0.0f;
        if (widescreen == 0) {
            a = 0.1f;
            c = 0.15f;
        }
        uv2[0] = 1.0f - a;
        uv2[1] = 1.0f;
        uv1[0] = c;
        uv1[1] = 0.0f;
    } else {
        float c = widescreen != 0 ? 0.0f : 0.125f;
        uv2[0] = 1.0f - c;
        uv2[1] = 1.0f;
        uv1[0] = c;
        uv1[1] = 0.0f;
    }
    DrawRectUV(0.0f, 0.0f, 640.0f, 480.0f, map, GeoColour(0x020C8300), uv1, uv2);
}

// 0x6F4150: the timer shown; at zero the game ends (a success from 75 points); the warning
// colour, the trigger function
void __fastcall GeoUpdateTimer(ClassGeography* g, re::Edx) {
    Minigame_SetTimerShown(g, g->timeLeft);
    if (!(0.0f < g->timeLeft)) {
        g->timeLeft = 0.0f;
        if (75.0f <= g->score) {
            Minigame_Success(g);
        }
        g->active = 0;
        return;
    }
    if (g->warnTime < g->timeLeft) {
        SetFontColour(GeoColour(0x020C8304));
    } else {
        SetFontColour(GeoColour(0x020C8308));
    }
    if (g->triggered == 0 && !(g->triggerTime < g->timeLeft)) {
        g->triggered = 1;
        RunScriptRef(&g->triggerFunction);
    }
    if (g->penaltyShown != 0) {
        SetFontColour(GeoColour(0x020C8308));
    }
}

// 0x6F4280: the timer (minutes, seconds)
void __fastcall GeoDrawTimer(ClassGeography* g, re::Edx) {
    if (fontStyleTexture[0] != nullptr) {
        fontStyle = 0;
    }
    Font_SetSizePreset(3);
    fontShadow = 2;
    if (g->warnTime < g->timeLeft) {
        SetFontColour(GeoColour(0x020C8304));
    } else {
        if (g->warned == 0) {
            PlayClassSound(g, GEO_SOUND_TIME_WARNING);
            g->warned = 1;
        }
        SetFontColour(GeoColour(0x020C8308));
    }
    if (g->triggered == 0 && !(g->triggerTime < g->timeLeft)) {
        g->triggered = 1;
        RunScriptRef(&g->triggerFunction);
    }
    if (g->penaltyShown != 0) {
        SetFontColour(GeoColour(0x020C8308));
    }
    Font_AddNumber(float(int(g->timeLeft / 60.0f)));
    Font_AddNumber(float(int(g->timeLeft) % 60));
    uint16_t text[0x40];
    Font_Format(text, GetText(GameString(0x00947134)), 0x40);   // MGGE_TIME
    float pos[2];
    ScreenPosition(pos, 0.0f, -215.0f, 1, 1);
    Font_PrintString(pos[0], pos[1], text, 1);
}

// 0x6F4480, 0x6F4640: a flag sliding to its slot (halving the distance a frame; done within 3
// units); player 1's is drawn 0.65 high, player 2's at full height
void GeoDrawSlide(ClassGeography* g, GeoPoint& held, GeoPoint& anim, int32_t& animId, uint8_t& active, bool flagHeight) {
    float sx = screenScaleX;
    float sy = screenScaleY;
    if (std::fabs(held.x - anim.x) < 3.0f && std::fabs(held.y - anim.y) < 3.0f) {
        animId = -1;
        active = 0;
        return;
    }
    anim.x = (anim.x + held.x) * 0.5f;
    anim.y = (held.y + anim.y) * 0.5f;
    void* flag = Geo_GetTexture(g, g->map, animId);
    int w = int(TexWidth(flag));
    int h = int(TexHeight(flag));
    float pos[2];
    ScreenPosition(pos, anim.x, anim.y, 1, 1);
    float halfHeight = float(h) * 0.5f * sy;
    if (flagHeight) {
        halfHeight = MulD(halfHeight, 0.65);
    }
    float halfWidth = 0.5f * float(w) * sx;
    DrawCentred(pos, halfWidth, halfHeight, flag, GeoColour(0x020C8338));
}
void __fastcall GeoDrawSlide1(ClassGeography* g, re::Edx) {
    GeoDrawSlide(g, g->held1, g->anim1, g->anim1Id, g->anim1Active, true);
}
void __fastcall GeoDrawSlide2(ClassGeography* g, re::Edx) {
    GeoDrawSlide(g, g->held2, g->anim2, g->anim2Id, g->anim2Active, false);
}

// a country's name over the label position (a shadow, then in the player's colour)
void GeoPrintName(const char* key, const float* pos, uint32_t colour) {
    uint16_t text[0x40];
    Font_Format(text, GetText(key), 0x40);
    SetFontColour(GeoColour(0x020C833C));
    Font_PrintString(pos[0] + 0.5f, pos[1] + 0.5f, text, 1);
    uint8_t bytes[4];
    std::memcpy(bytes, &colour, 4);
    SetFontColour(bytes);
    Font_PrintString(pos[0], pos[1], text, 1);
}

// 0x6F47F0: the cursor's label: over a free flag in reach, that flag enlarged and its
// country's name; else (holding a flag) the held flag's name. Above the cursor, or above
// the target it snapped to
void __fastcall GeoDrawLabel(ClassGeography* g, re::Edx, float x, float y, uint32_t colour, int hoverId, int player) {
    Font_SetSizePreset(language == 7 ? 3 : 1);
    float sx = screenScaleX;
    float sy = screenScaleY;
    float labelX = x;
    float labelY = y;
    for (int i = g->remaining - 1; i >= 0; --i) {
        if (g->placed[i] != 0) {
            continue;
        }
        float dist = Distance2D(x - g->countryPos[i].x, y - g->countryPos[i].y);
        if (!(g->range[player] < dist)) {
            labelX = g->anchor[player].x;
            labelY = g->anchor[player].y;
            break;
        }
    }
    Font_SetJustify(2);
    float textPos[2];
    ScreenPosition(textPos, labelX + 0.0f, labelY - 38.0f, 1, 1);
    bool drawn = false;
    bool free = false;
    if (player == 0) {
        free = g->hover[0] == 0;
    } else if (player == 1) {
        free = g->hover[1] == 0;
    }
    for (int i = g->remaining - 1; i >= 0; --i) {
        float dist = Distance2D(x - g->slotPos[i].x, y - g->slotPos[i].y);
        if (!(dist <= 40.0f) || g->placed[i] != 0 || !free || g->fading != 0) {
            continue;
        }
        int32_t id = g->ids[i];
        void* flag = Geo_GetTexture(g, g->map, id);
        if ((g->hover[0] != 0 && g->hoverId[0] == id) || (g->hover[1] != 0 && g->hoverId[1] == id) ||
            id == g->anim1Id || id == g->anim2Id) {
            drawn = true;
            break;
        }
        float pos[2];
        ScreenPosition(pos, g->slotPos[i].x, g->slotPos[i].y, 1, 1);
        int w = int(TexWidth(flag));
        int h = int(TexHeight(flag));
        float halfHeight = MulD(float(h) * 0.5f * sy, 0.65);
        float shadow[2] = {pos[0] + 2.0f, pos[1] + 2.0f};
        float halfWidth = 0.5f * float(w) * sx;
        DrawCentred(shadow, halfWidth, halfHeight, flag, GeoColour(0x020C833C));
        DrawCentred(pos, halfWidth, halfHeight, flag, GeoColour(0x020C8338));
        if (language == 7) {
            Font_SetScale(0.5f);
        }
        GeoPrintName(g->keys[i], textPos, colour);
        drawn = true;
        break;
    }
    for (int i = 0; i < g->remaining; ++i) {
        if (!free && g->ids[i] == hoverId && !drawn) {
            GeoPrintName(g->keys[i], textPos, colour);
        }
    }
    Font_SetSizePreset(3);
}

// 0x6F4E30: the score (the good colour from 75)
void __fastcall GeoDrawScore(ClassGeography* g, re::Edx) {
    Font_Reset();
    if (!(75.0f > g->score)) {
        SetFontColour(GeoColour(0x020C830C));
    } else {
        SetFontColour(GeoColour(0x020C8310));
    }
    float score = g->score;
    float unused[2];   // (computed, never used)
    ScreenPosition(unused, 60.0f, geoScoreTitleY, 1, 1);
    Font_AddNumber(score);
    uint16_t text[0x40];
    Font_Format(text, GetText(GameString(0x00947140)), 0x40);   // MGGE_SCORE
    float pos[2];
    ScreenPosition(pos, 200.0f, geoScoreY, 1, 1);
    Font_SetSizePreset(3);
    geoFontJustify = 3;
    Font_PrintString(pos[0], pos[1], text, 1);
}

// 0x6F4F60: the flags waiting in their slots (not the ones held or sliding)
void __fastcall GeoDrawSlots(ClassGeography* g, re::Edx) {
    float sx = screenScaleX;
    float sy = screenScaleY;
    for (uint32_t i = 0; i < g->countryCount; ++i) {
        GeoPoint slot = g->slotPos[i];
        int32_t id = g->ids[i];
        bool placed = g->placed[i] == 1;
        uint8_t colour[4];
        void* texture;
        if (placed) {
            CopyColour(colour, GeoColour(0x020C833C));
            texture = Minigame_GetTexture(g, GEO_TEX_X_TARGET);
        } else {
            CopyColour(colour, GeoColour(0x020C8338));
            texture = Geo_GetTexture(g, g->map, id);
        }
        int w = 1;
        int h = 1;
        if (texture != nullptr) {
            w = int(TexWidth(texture));
            h = int(TexHeight(texture));
        }
        if (placed || i == g->selected[0] || i == g->selected[1] || id == g->anim2Id || id == g->anim1Id) {
            continue;
        }
        float pos[2];
        ScreenPosition(pos, slot.x, slot.y, 1, 1);
        float halfHeight = DivD(MulD(float(h) * 0.5f * sy, 0.65), 1.2);
        float halfWidth = DivD(0.5f * float(w) * sx, 1.2);
        DrawCentred(pos, halfWidth, halfHeight, texture, colour);
    }
}

// 0x6F5190: the new map's top-row flags dealt one at a time (sliding in from below); the
// dealt ones drawn grey
void __fastcall GeoDealFlags(ClassGeography* g, re::Edx) {
    // (the game's eight-entry stack lists; read uninitialised past the ones collected)
    GeoPoint listPos[16] = {};
    int32_t listIds[16] = {};
    int n = 0;
    for (uint32_t i = 0; i < g->countryCount; ++i) {
        if (330.0f > g->slotPos[i].y) {
            listPos[n] = g->slotPos[i];
            listIds[n] = g->ids[i];
            ++n;
        }
    }
    for (int k = 0; k < 8; ++k) {
        if (g->dealt[k] == 0 || g->anim1Id == listIds[k]) {
            continue;
        }
        float sx = screenScaleX;
        float sy = screenScaleY;
        uint8_t colour[4];
        RGBA_Set(colour, 0xD7, 0xD7, 0xD7, 0xFF);
        float pos[2];
        ScreenPosition(pos, listPos[k].x, listPos[k].y, 1, 1);
        void* flag = Geo_GetTexture(g, g->map, listIds[k]);
        int w = int(TexWidth(flag));
        int h = int(TexHeight(flag));
        float halfHeight = DivD(MulD(float(h) * 0.5f * sy, 0.65), 1.2);
        float halfWidth = DivD(0.5f * float(w) * sx, 1.2);
        DrawCentred(pos, halfWidth, halfHeight, flag, colour);
    }
    int dealtCount = 0;
    for (int k = 0; k < 8; ++k) {
        uint8_t dealt = g->dealt[k];
        if (dealt == 0) {
            if (g->anim1Active == 0) {
                g->held1 = listPos[k];
                g->anim1Id = listIds[k];
                g->anim1Active = 1;
                g->anim1.x = g->held1.x;
                g->anim1.y = g->held1.y + 200.0f;
                g->dealt[k] = 1;
                return;
            }
        } else {
            ++dealtCount;
        }
        if (dealtCount >= 8 || uint32_t(dealtCount) >= g->countryCount) {
            g->dealing = 0;
        }
    }
}

// 0x6F53F0: the old map's flags falling away (shaking until they drop, a random one
// starting each frame); the new flags dealt once all have fallen
void __fastcall GeoDrawFalling(ClassGeography* g, re::Edx) {
    float sx = screenScaleX;
    float sy = screenScaleY;
    g->pulse = float(double(g->pulse) + 0.05);
    int fallen = 0;
    for (int i = 0; i < g->prevCount; ++i) {
        if (g->fall[i] < 300) {
            g->fall[i] *= 2;
            continue;
        }
        if (g->falling[i] == 1) {
            PlayClassSound(g, GEO_SOUND_FLAG_FALLS);
        }
        ++fallen;
        g->falling[i] = 0;
        if (fallen == g->prevCount) {
            Geo_DealFlags(g);
        }
    }
    int j = Rand() * 0x17 / 0x7FFF;
    if (j < g->prevCount && g->falling[j] == 0 && g->fall[j] < 300) {
        g->falling[j] = 1;
        g->fall[j] = 1;
        g->dropPos = g->prevPos[j];
        g->starSize = 150;
        g->starShown = 1;
        g->starTime = 0.0f;
    }
    for (int i = 0; i < g->prevCount; ++i) {
        uint8_t colour[4];
        CopyColour(colour, g->colours[i]);
        void* flag = Geo_GetTexture(g, g->prevMap, g->prevTexIds[i]);
        int w = 1;
        int h = 1;
        if (flag != nullptr) {
            w = int(TexWidth(flag));
            h = int(TexHeight(flag));
        }
        void* pin = Minigame_GetTexture(g, GEO_TEX_PIN);
        int pinW = 1;
        int pinH = 1;
        if (pin != nullptr) {
            pinW = int(TexWidth(pin));
            pinH = int(TexHeight(pin));
        }
        int shake = 0;
        if (g->falling[i] == 0) {
            int r = Rand();
            shake = r * (fallen / 5 - 1) / 0x7FFF;
        }
        float s = float(shake);
        float pos[2];
        ScreenPosition(pos, s + g->prevPos[i].x, s + g->prevPos[i].y + float(g->fall[i]), 1, 1);
        DrawCentred(pos, 0.5f * float(w) * sx, float(h) * 0.5f * sy, flag, colour);
        if (g->falling[i] == 0) {
            float pinPos[2];
            ScreenPosition(pinPos, s + g->prevPos[i].x, s + g->prevPos[i].y + float(g->fall[i]), 1, 1);
            DrawCentred(pinPos, 0.5f * float(pinW) * sx, float(pinH) * 0.5f * sy, pin, colour);
        }
    }
}

// 0x6F5830: the targets: a pulsing "x" with a shadow on each free country, the flag (0.7
// size) and a pin on a placed one
void __fastcall GeoDrawTargets(ClassGeography* g, re::Edx) {
    g->unknown828 = 4;
    g->unknown82C = 0xB;
    float sx = screenScaleX;
    float sy = screenScaleY;
    g->pulse = float(double(g->pulse) + 0.05);
    for (int i = 0; i < g->remaining; ++i) {
        uint8_t colour[4];
        CopyColour(colour, GeoColour(0x020C8340));
        void* target = Minigame_GetTexture(g, GEO_TEX_X_TARGET);
        int w = 1;
        int h = 1;
        if (target != nullptr) {
            w = int(TexWidth(target));
            h = int(TexHeight(target));
        }
        float s;
        if (g->placed[i] == 0) {
            float c = trk::CrtCos(g->pulse);
            s = (c + 3.0f) / 3.0f;
            if (5.0f < g->pulse) {
                g->pulse = 1.0f;
            }
        } else {
            s = 2.0f;
        }
        void* texture = target;
        const GeoPoint& at = g->countryPos[i];
        if (g->placed[i] == 1) {
            CopyColour(colour, g->colours[i]);
            void* flag = Geo_GetTexture(g, g->map, g->texIds[i]);
            int flagW = int(TexWidth(flag));
            int flagH = int(TexHeight(flag));
            texture = Minigame_GetTexture(g, GEO_TEX_PIN);
            float pos[2];
            ScreenPosition(pos, at.x, at.y, 1, 1);
            float halfHeight = MulD(float(flagH) * 0.5f * sy, 0.7);
            float halfWidth = MulD(0.5f * float(flagW) * sx, 0.7);
            DrawCentred(pos, halfWidth, halfHeight, flag, GeoColour(0x020C8338));
        } else {
            // the shadow: black, its alpha 55 / the pulse
            float offset = 3.0f * s;
            float pos[2];
            ScreenPosition(pos, offset + at.x, at.y + offset, 1, 1);
            int64_t alpha = int64_t(55.0f / s);
            uint8_t none[3][4] = {};
            uint8_t shadow[4] = {uint8_t(alpha >> 24), uint8_t(alpha >> 16), uint8_t(alpha >> 8), uint8_t(alpha)};
            float twice = s + s;
            float halfHeight = float(h) * 0.5f * sy / twice;
            float halfWidth = 0.5f * float(w) * sx / twice;
            DrawRectUV4(pos[0] - halfWidth, pos[1] - halfHeight, pos[0] + halfWidth, pos[1] + halfHeight, target,
                        none[2], none[1], none[0], shadow, kUv0, kUv1);
        }
        float scale = g->placed[i] != 0 ? 0.7f : 1.0f;
        float pos[2];
        ScreenPosition(pos, at.x, at.y, 1, 1);
        float halfHeight = float(h) * 0.5f * sy * s * scale;
        float halfWidth = 0.5f * float(w) * sx * s * scale;
        DrawCentred(pos, halfWidth, halfHeight, texture, colour);
    }
}

// 0x6F5D20: the "-5" sticker flying from the wrong drop to the timer; on arrival 5
// seconds come off
void __fastcall GeoDrawPenalty(ClassGeography* g, re::Edx) {
    float sx = screenScaleX;
    float sy = screenScaleY;
    float x = g->penaltyPos.x;
    float t = (x + 0.0f) * 0.5f;
    float u = (t + x) * 0.5f;
    g->penaltyPos.x = (x + u) * 0.5f;
    float y = g->penaltyPos.y;
    t = (y - 215.0f) * 0.5f;
    u = (t + y) * 0.5f;
    g->penaltyPos.y = (y + u) * 0.5f;
    if (g->textures != nullptr && g->textures[GEO_TEX_MINUS_5] != nullptr) {
        void* sticker = g->textures[GEO_TEX_MINUS_5];
        int w = int(TexWidth(sticker));
        int h = int(TexHeight(sticker));
        uint8_t front[4];
        uint8_t back[4];
        RGBA_Set(front, 0xD7, 0xD7, 0xD7, 0x9B);
        RGBA_Set(back, 0xD7, 0xD7, 0xD7, 0x9B);
        float pos[2];
        ScreenPosition(pos, 2.0f + g->penaltyPos.x, g->penaltyPos.y + 2.0f, 1, 1);
        float halfHeight = MulD(float(h), 0.05) * sy * 10.0f;
        float halfWidth = MulD(float(w), 0.05) * sx * 10.0f;
        DrawCentred(pos, halfWidth, halfHeight, sticker, back);
        ScreenPosition(pos, g->penaltyPos.x, g->penaltyPos.y, 1, 1);
        DrawCentred(pos, halfWidth, halfHeight, sticker, front);
    }
    if (std::fabs(g->penaltyPos.x - 0.0f) < 3.0f && std::fabs(g->penaltyPos.y - -215.0f) < 3.0f) {
        g->penaltyShown = 0;
        g->timeLeft = g->timeLeft - 5.0f;
    }
}

// 0x6F6030: a player's cursor: it snaps to a target within its range (the range then
// doubled, half again on leaving), easing back after; the label; then the held flag with
// the closed hand (and a halo on a target), or the open hand
void __fastcall GeoDrawCursor(ClassGeography* g, re::Edx, float x, float y, uint32_t colour, int hoverId, int player) {
    float sx = screenScaleX;
    float sy = screenScaleY;
    float cursorPos[2];
    ScreenPosition(cursorPos, x, y + 0.0f, 1, 1);
    float anchorX = x;
    float anchorY = y;
    bool found = false;
    for (int i = g->remaining - 1; i >= 0; --i) {
        if (g->placed[i] != 0) {
            continue;
        }
        float dist = Distance2D(x - g->countryPos[i].x, y - g->countryPos[i].y);
        if (!(g->range[player] < dist)) {
            if (g->inRange[player] == 0) {
                g->inRange[player] = 1;
                g->range[player] = g->range[player] + g->range[player];
                g->anchor[player] = g->countryPos[i];
                g->snap[player] = 0;
            }
            anchorX = g->countryPos[i].x;
            found = true;
            anchorY = g->countryPos[i].y;
            break;
        }
        if (g->inRange[player] != 0) {
            g->inRange[player] = 0;
            g->range[player] = g->range[player] * 0.5f;
        }
    }
    int snap = g->snap[player];
    if (snap > 1) {
        anchorX = g->anchor[player].x;
        anchorY = g->anchor[player].y;
        g->snap[player] = snap - 1;
    } else if (snap == 1) {
        GeoPoint cursors[2] = {g->cursor[0], g->cursor[1]};
        g->snap[player] = 0;
        anchorX = (g->anchor[player].x + cursors[player].x) * 0.5f;
        anchorY = (cursors[player].y + g->anchor[player].y) * 0.5f;
    }
    float handPos[2];
    ScreenPosition(handPos, anchorX + 0.0f, anchorY + 0.0f, 1, 1);
    float flagPos[2];
    ScreenPosition(flagPos, anchorX - 0.0f, anchorY - 0.0f, 1, 1);
    uint8_t sliding[2] = {g->anim1Active, g->anim2Active};
    if (g->fading == 0 && sliding[player] == 0) {
        Geo_DrawLabel(g, x, y, colour, hoverId, player);
    }
    uint8_t colourBytes[4];
    std::memcpy(colourBytes, &colour, 4);
    bool free = false;
    if (player == 0) {
        free = g->hover[0] == 0;
    } else if (player == 1) {
        free = g->hover[1] == 0;
    }
    if (free && hoverId == 2) {
        // the open hand
        if (g->textures == nullptr || g->textures[GEO_TEX_OPEN_HAND] == nullptr) {
            return;
        }
        void* hand = g->textures[GEO_TEX_OPEN_HAND];
        int w = int(TexWidth(hand));
        int h = int(TexHeight(hand));
        DrawCentred(cursorPos, MulD(0.5f * float(w) * sx, 0.749), MulD(float(h) * 0.5f * sy, 0.749), hand,
                    colourBytes);
        return;
    }
    void* flag = Geo_GetTexture(g, g->map, hoverId);
    int w = int(TexWidth(flag));
    int h = int(TexHeight(flag));
    float halfHeight = MulD(MulD(float(h) * 0.5f * sy, 0.65), 0.749);
    float halfWidth = MulD(0.5f * float(w) * sx, 0.749);
    DrawCentred(flagPos, halfWidth, halfHeight, flag, GeoColour(0x020C8330));
    if (found && g->textures != nullptr && g->textures[GEO_TEX_HAND_HALO] != nullptr) {
        void* halo = g->textures[GEO_TEX_HAND_HALO];
        w = int(TexWidth(halo));
        h = int(TexHeight(halo));
        DrawCentred(handPos, MulD(0.5f * float(w) * sx, 1.049), MulD(float(h) * 0.5f * sy, 1.049), halo,
                    GeoColour(0x020C8334));
    }
    if (g->textures == nullptr || g->textures[GEO_TEX_CLOSED_HAND] == nullptr) {
        return;
    }
    void* hand = g->textures[GEO_TEX_CLOSED_HAND];
    w = int(TexWidth(hand));
    h = int(TexHeight(hand));
    DrawCentred(handPos, MulD(0.5f * float(w) * sx, 0.749), MulD(float(h) * 0.5f * sy, 0.749), hand, colourBytes);
}

// 0x6F6710: a cursor moved by its pad (speed 4 near a target or with the trigger, else 9),
// kept on the screen; the cursor size from the held flag's (or the hand's) texture
GeoPoint* __fastcall GeoMoveCursor(ClassGeography* g, re::Edx, GeoPoint* out, float x, float y, int player) {
    void* texture = nullptr;
    bool haveTexture = true;
    int which = player == 0 ? 0 : 1;
    if (g->hover[which] != 0) {
        texture = Geo_GetTexture(g, g->map, g->hoverId[which]);
    } else if (g->textures == nullptr) {
        haveTexture = false;
    } else {
        texture = g->textures[g->hoverId[which]];
    }
    if (haveTexture && texture != nullptr) {
        float sx = screenScaleX;
        g->cursorWidth = float(TexWidth(texture)) * sx;
        float sy = screenScaleY;
        g->cursorHeight = float(TexHeight(texture)) * sy;
    }
    GeoPoint move;
    Geo_ReadPad(&move, player);
    bool nearTarget = false;
    for (int i = 0; i < g->remaining; ++i) {
        if (g->placed[i] != 0) {
            continue;
        }
        float dist = Distance2D(x - g->countryPos[i].x, y - g->countryPos[i].y);
        bool free = false;
        if (player == 0) {
            free = g->hover[0] == 0;
        } else if (player == 1) {
            free = g->hover[1] == 0;
        }
        if (!(g->range[player] < dist) && !free) {
            nearTarget = true;
        }
    }
    float speed;
    if (Pad_GetValue(0xD, player) != 0.0f || nearTarget) {
        speed = 4.0f;
    } else {
        speed = 9.0f;
    }
    float moveX = move.x * speed;
    float moveY = speed * move.y;
    x = x + moveX;
    y = y + moveY;
    float limit = 320.0f / screenScaleX - 30.0f;
    float low = -limit;
    if (!(low < x)) {
        x = low;
    } else if (!(x < limit)) {
        x = limit;
    }
    if (y <= -210.0f) {
        y = -210.0f;
    } else if (!(y < 210.0f)) {
        y = 210.0f;
    }
    out->x = x;
    out->y = y;
    return out;
}

// 0x6F69A0: a cursor lets go: its flag placed (all placed: the map done), or sliding back
// to its slot (dropped on the map: a mistake, the "-5" sticker)
void __fastcall GeoDrop(ClassGeography* g, re::Edx, float x, float y, int hoverId, uint32_t slot, int player) {
    uint8_t hovering = player == 0 ? g->hover[0] : g->hover[1];
    if ((hovering != 0 || hoverId != 2) && 150.0f > y) {
        g->invalidOperation = 1;
    }
    for (uint32_t i = 0; i < g->countryCount; ++i) {
        if (i == slot && g->mapDone == 0 && Geo_TryPlace(g, x, y, int(slot), player) == 1) {
            g->remaining = 0;
            g->mapDone = 1;
            Geo_ResetCountries(g);
            PlayClassSound(g, GEO_SOUND_MAP_FINISH);
        }
    }
    if (IsCursor(g->cursor[0], x, y) && (g->hover[0] != 0 || g->hoverId[0] != 2)) {
        PlayClassSound(g, GEO_SOUND_FLAG_SLIDE);
        g->anim1Active = 1;
        g->anim1 = {x, y};
        g->anim1Id = hoverId;
        if (150.0f > y) {
            PlayClassSound(g, GEO_SOUND_DROP_INCORRECT);
            Pad_Rumble(player, 200, 0x14, 1);
        }
        if (g->penaltyShown == 0 && 150.0f > y) {
            g->penaltyShown = 1;
            g->penaltyPos = {x, y};
        }
    } else if (IsCursor(g->cursor[1], x, y) && (g->hover[1] != 0 || g->hoverId[1] != 2)) {
        PlayClassSound(g, GEO_SOUND_FLAG_SLIDE);
        g->anim2Active = 1;
        g->anim2 = {x, y};
        g->anim2Id = hoverId;
        if (150.0f > y) {
            PlayClassSound(g, GEO_SOUND_DROP_INCORRECT);
            Pad_Rumble(player, 200, 0x14, 1);
        }
    }
    if (IsCursor(g->cursor[0], x, y)) {
        g->selected[0] = g->countryCount;
        g->hoverId[0] = 2;
        g->hover[0] = 0;
        return;
    }
    g->selected[1] = g->countryCount;
    g->hoverId[1] = 2;
    g->hover[1] = 0;
}

// 0x6F6D10 (virtual +8)
void __fastcall GeoDraw(ClassGeography* g, re::Edx) {
    if (g->hud == 0) {
        return;
    }
    Geo_DrawMap(g);
    if (g->fading != 0) {
        Geo_DrawNextMap(g);
    }
    if (g->anim1Active != 0) {
        Geo_DrawSlide1(g);
    }
    if (g->anim2Active != 0) {
        Geo_DrawSlide2(g);
    }
    if (g->dealing != 0) {
        Geo_DrawFalling(g);
    } else {
        if (g->fading != 0) {
            Geo_DrawNextMap(g);
        }
        Geo_DrawTargets(g);
        Geo_DrawSlots(g);
    }
    Geo_DrawScore(g);
    Geo_DrawTimer(g);
    if (g->popupShowing == 0) {
        uint32_t colour;
        std::memcpy(&colour, GeoColour(0x020C8328), 4);
        Geo_DrawCursor(g, g->cursor[0].x, g->cursor[0].y, colour, g->hoverId[0], 0);
    }
    if (g->starShown != 0 && g->starTime > 25.0f) {
        g->starShown = 0;
        g->starTime = 0.0f;
    }
    if (g->penaltyShown != 0) {
        Geo_DrawPenalty(g);
    }
    if (g->popupShowing != 0) {
        Popup_Draw(&g->popupKind);
    }
}

// a stick pushed past 0.05 (only when the repeat delay allows)
inline bool GeoStickBelow(int control, bool idle) {
    return Pad_GetValue(control, 0) < -0.05 && idle;
}
inline bool GeoStickAbove(int control, bool idle) {
    return Pad_GetValue(control, 0) > 0.05 && idle;
}

// 0x6F6E10 (virtual +4): the popup, the timer, the pad: picking up (button 7 or 12),
// letting go, moving player 1's cursor (player 2's on pad 1 picks up and drops only)
void __fastcall GeoUpdate(ClassGeography* g, re::Edx) {
    if (g->popupShowing != 0) {
        if (g->popupKind == 1) {
            if (Channel_IsPlaying(musicChannel) != 0) {
                Channel_Pause(musicChannel);
            }
            if (Popup_Result(&g->popupKind) == 2) {
                g->active = 0;
            }
            return;
        }
        if (g->popupShowing != 0 && g->popupKind == 2) {
            Popup_Result(&g->popupKind);
            return;
        }
    }
    if (userStartResponse != 0) {
        return;
    }
    if (g->fading == 0) {
        float step = geoTimerRate * timeStep;
        g->timeLeft = g->timeLeft - step;
    }
    Geo_UpdateTimer(g);
    if (g->active == 0) {
        return;
    }
    if (Channel_IsPaused(musicChannel) != 0) {
        Channel_Resume(musicChannel);
    }
    if ((Pad_GetValue(1, 0) != 0.0f && g->padRepeat == 0) || Pad_JustDown(1, 0) != 0 ||
        GeoStickBelow(0x10, g->padRepeat == 0)) {
        g->padDir = 3;
        g->padRepeat = 300;
    } else if ((Pad_GetValue(0, 0) != 0.0f && g->padRepeat == 0) || Pad_JustDown(0, 0) != 0 ||
               GeoStickAbove(0x10, g->padRepeat == 0)) {
        g->padDir = 2;
        g->padRepeat = 300;
    } else if (Pad_JustDown(2, 0) != 0 || GeoStickAbove(0x11, g->padRepeat == 0)) {
        g->padDir = 0;
        g->padRepeat = 300;
    } else if (Pad_JustDown(3, 0) != 0 || GeoStickBelow(0x11, g->padRepeat == 0)) {
        g->padDir = 1;
        g->padRepeat = 300;
    } else {
        g->padDir = 4;
    }
    if ((Pad_JustDown(7, 0) != 0 || Pad_JustDown(0xC, 0) != 0) && g->anim1Active == 0) {
        Geo_PickUp(g, g->cursor[0].x, g->cursor[0].y);
    }
    if (Pad_JustDown(7, 1) != 0 && g->anim2Active == 0) {
        Geo_PickUp(g, g->cursor[1].x, g->cursor[1].y);
    }
    if (Pad_JustUp(7, 0) != 0 || Pad_JustUp(0xC, 0) != 0) {
        Geo_Drop(g, g->cursor[0].x, g->cursor[0].y, g->hoverId[0], g->selected[0], 0);
    }
    if (Pad_JustUp(7, 1) != 0) {
        Geo_Drop(g, g->cursor[1].x, g->cursor[1].y, g->hoverId[1], g->selected[1], 1);
    }
    GeoPoint moved;
    GeoPoint* to = Geo_MoveCursor(g, &moved, g->cursor[0].x, g->cursor[0].y, 0);
    g->cursor[0].x = to->x;
    g->cursor[0].y = to->y;
    if (Pad_JustDown(8, 0) != 0) {
        Popup_Show(&g->popupKind, 1, GameString(0x00944690), GameString(0x00941D14), 7, 8);   // EXIT_QUESTION
        return;
    }
    if (Pad_JustDown(6, 0) != 0) {
        Popup_Show(&g->popupKind, 2, GameString(0x00947014), GameString(0x00947020), 7, -1);   // MGGE_INFO
    }
}

} // namespace

#define CGEOP43_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CGEOP43_REVERSED("ClassGeography::ReadPad_6F3540", 0x006F3540, &GeoReadPad);
CGEOP43_REVERSED("ClassGeography::PickUp_6F3780", 0x006F3780, &GeoPickUp);
CGEOP43_REVERSED("ClassGeography::TryPlace_6F39C0", 0x006F39C0, &GeoTryPlace);
CGEOP43_REVERSED("ClassGeography::DrawNextMap_6F3D50", 0x006F3D50, &GeoDrawNextMap);
CGEOP43_REVERSED("ClassGeography::DrawMap_6F3FF0", 0x006F3FF0, &GeoDrawMap);
CGEOP43_REVERSED("ClassGeography::UpdateTimer_6F4150", 0x006F4150, &GeoUpdateTimer);
CGEOP43_REVERSED("ClassGeography::DrawTimer_6F4280", 0x006F4280, &GeoDrawTimer);
CGEOP43_REVERSED("ClassGeography::DrawSlide1_6F4480", 0x006F4480, &GeoDrawSlide1);
CGEOP43_REVERSED("ClassGeography::DrawSlide2_6F4640", 0x006F4640, &GeoDrawSlide2);
CGEOP43_REVERSED("ClassGeography::DrawLabel_6F47F0", 0x006F47F0, &GeoDrawLabel);
CGEOP43_REVERSED("ClassGeography::DrawScore_6F4E30", 0x006F4E30, &GeoDrawScore);
CGEOP43_REVERSED("ClassGeography::DrawSlots_6F4F60", 0x006F4F60, &GeoDrawSlots);
CGEOP43_REVERSED("ClassGeography::DealFlags_6F5190", 0x006F5190, &GeoDealFlags);
CGEOP43_REVERSED("ClassGeography::DrawFalling_6F53F0", 0x006F53F0, &GeoDrawFalling);
CGEOP43_REVERSED("ClassGeography::DrawTargets_6F5830", 0x006F5830, &GeoDrawTargets);
CGEOP43_REVERSED("ClassGeography::DrawPenalty_6F5D20", 0x006F5D20, &GeoDrawPenalty);
CGEOP43_REVERSED("ClassGeography::DrawCursor_6F6030", 0x006F6030, &GeoDrawCursor);
CGEOP43_REVERSED("ClassGeography::MoveCursor_6F6710", 0x006F6710, &GeoMoveCursor);
CGEOP43_REVERSED("ClassGeography::Drop_6F69A0", 0x006F69A0, &GeoDrop);
CGEOP43_REVERSED("ClassGeography::Draw_6F6D10", 0x006F6D10, &GeoDraw);
CGEOP43_REVERSED("ClassGeography::Update_6F6E10", 0x006F6E10, &GeoUpdate);
