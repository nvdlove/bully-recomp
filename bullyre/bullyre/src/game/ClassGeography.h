#pragma once

// Geography class (ClassGeography, 0x830 bytes, vtable 0x946FF8): the flag game. A map
// (9 of them: Europe, Asia, Africa, the USA, Central America, Canada, southern Africa,
// Oceania, the north-east USA) shows its countries' targets; the flags wait in slots (8 on
// the top row, the rest below) and the player drags each onto its country. Two cursors
// (the second player's on pad 1). ClassGeographyMaps.cpp (set-up, maps, the class's
// virtuals) and ClassGeographyPlay.cpp (input and drawing) share this.

#include <cstddef>
#include <cstdint>

#include "game/SchoolClasses.h"
#include "reversed.h"

namespace geo {

struct GeoScoreMsg {
    float score;
    char key[0x40];
};
static_assert(sizeof(GeoScoreMsg) == 0x44);

struct GeoPoint {
    float x, y;
};
static_assert(sizeof(GeoPoint) == 8);

struct ClassGeography {
    re::Ptr32<re::Ptr32<void>> vtable;                     // +0x0
    uint8_t active;                    // +0x4
    uint8_t hud;                       // +0x5
    uint8_t success;                   // +0x6
    uint8_t unknown7[0x15];
    re::Ptr32<re::Ptr32<void>> textures;                   // +0x1C (the "MG_Geography_Common" set: see GeoTexture)
    uint8_t unknown20[4];
    float warnTime;                    // +0x24
    uint8_t unknown28;
    uint8_t triggered;                 // +0x29 the trigger function ran
    uint8_t unknown2A[2];
    float triggerTime;                 // +0x2C
    uint8_t unknown30[4];
    float timeLeft;                    // +0x34 seconds
    int32_t unknown38;                 // +0x38 (1)
    int32_t unknown3C;                 // +0x3C (3)
    uint8_t unknown40[0xC];
    re::Ptr32<uint32_t> sounds;                  // +0x4C
    int32_t popupKind;                 // +0x50 the popup (1 the exit question, 2 the info)
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;              // +0xA0
    uint8_t unknownA1[0x23];
    int32_t level;                     // +0xC4 the map played (0..8)
    int32_t unknownC8;                 // +0xC8
    int32_t scoreMsgCount;             // +0xCC
    GeoScoreMsg scoreMsgs[10];         // +0xD0
    uint8_t warned;                    // +0x378 the time warning played
    uint8_t unknown379[3];
    uint32_t triggerFunction;          // +0x37C a script function's reference
    uint8_t unknown380[4];
    float unknown384;                  // +0x384 (SetTimer's second)
    float scorePercentage;             // +0x388
    float score;                       // +0x38C 6.25 a flag
    int32_t padRepeat;                 // +0x390 (set to 300 by the d-pad; never counted down here)
    int32_t mapDone;                   // +0x394 all flags placed (1)
    int32_t map;                       // +0x398 the map shown
    int32_t prevMap;                   // +0x39C the map whose flags fall away
    int32_t prevCount;                 // +0x3A0
    int32_t ids[16];                   // +0x3A4 the countries' ids (1..)
    int32_t texIds[16];                // +0x3E4 their "_p" flags' texture index
    re::Ptr32<const char> keys[16];              // +0x424 their names' text keys
    uint8_t validOperation;            // +0x464
    uint8_t invalidOperation;          // +0x465
    uint8_t penaltyShown;              // +0x466 the "-5" sticker flying to the timer
    uint8_t unknown467;
    GeoPoint held1;                    // +0x468 player 1's dragged flag
    GeoPoint anim1;                    // +0x470 a flag sliding (player 1's, or the next one up)
    GeoPoint dropPos;                  // +0x478 where the star shows
    GeoPoint held2;                    // +0x480
    GeoPoint anim2;                    // +0x488
    uint8_t unknown490[8];
    uint8_t anim1Active;               // +0x498
    uint8_t anim2Active;               // +0x499
    uint8_t starShown;                 // +0x49A
    uint8_t unknown49B;
    float starTime;                    // +0x49C
    int32_t starSize;                  // +0x4A0 (100, 150)
    uint8_t unknown4A4[8];
    int32_t unknown4AC;                // +0x4AC
    int32_t unknown4B0;                // +0x4B0
    uint8_t unknown4B4[4];
    int32_t unknown4B8;                // +0x4B8 (-1)
    int32_t anim1Id;                   // +0x4BC the sliding flags' countries (-1 none)
    int32_t anim2Id;                   // +0x4C0
    float cursorWidth;                 // +0x4C4
    float cursorHeight;                // +0x4C8
    GeoPoint cursor[2];                // +0x4CC
    GeoPoint penaltyPos;               // +0x4DC
    uint8_t inRange[2];                // +0x4E4 the cursor near a target (its range doubled)
    uint8_t unknown4E6[2];
    float range[2];                    // +0x4E8 (10)
    GeoPoint anchor[2];                // +0x4F0 the target the cursor snapped to
    int32_t snap[2];                   // +0x500 frames of the snap left
    float unknown508;                  // +0x508 (4)
    int32_t hoverId[2];                // +0x50C the flag picked up (2: none, a game quirk: 2 is a real id)
    uint32_t selected[2];              // +0x514 its slot (the count: none)
    int32_t padDir;                    // +0x51C 0 up, 1 down, 2 right, 3 left, 4 none
    int32_t remaining;                 // +0x520 the countries being played
    float pulse;                       // +0x524 the targets' pulse phase
    GeoPoint countryPos[16];           // +0x528 the targets
    uint8_t colours[16][4];            // +0x5A8 a placed flag's tint (the placing player's)
    GeoPoint slotPos[16];              // +0x5E8 the flag slots
    uint8_t placed[16];                // +0x668
    int32_t fade;                      // +0x678 the map fading in (255 down to 0)
    uint8_t fading;                    // +0x67C
    uint8_t dealing;                   // +0x67D the old flags falling, the new ones dealt
    uint8_t unknown67E[2];
    int32_t prevTexIds[16];            // +0x680
    GeoPoint prevPos[16];              // +0x6C0
    uint8_t falling[16];               // +0x740
    uint8_t dealt[8];                  // +0x750
    int32_t fall[16];                  // +0x758 a falling flag's drop (doubling)
    re::Ptr32<re::Ptr32<void>> textureSets[9];             // +0x798 by map: [0] the map, then the flags, again, the "_p" flags
    int32_t txdSlots[9];               // +0x7BC
    uint8_t hover[2];                  // +0x7E0 the cursor over a flag
    uint8_t unknown7E2[2];
    uint32_t countryCount;             // +0x7E4
    int32_t slotIds[16];               // +0x7E8 the flag in each slot
    int32_t unknown828;                // +0x828 (4)
    int32_t unknown82C;                // +0x82C (11)
};
static_assert(offsetof(ClassGeography, scoreMsgs) == 0xD0);
static_assert(offsetof(ClassGeography, warned) == 0x378);
static_assert(offsetof(ClassGeography, triggerFunction) == 0x37C);
static_assert(offsetof(ClassGeography, ids) == 0x3A4);
static_assert(offsetof(ClassGeography, keys) == 0x424);
static_assert(offsetof(ClassGeography, held1) == 0x468);
static_assert(offsetof(ClassGeography, anim1Active) == 0x498);
static_assert(offsetof(ClassGeography, anim1Id) == 0x4BC);
static_assert(offsetof(ClassGeography, cursor) == 0x4CC);
static_assert(offsetof(ClassGeography, range) == 0x4E8);
static_assert(offsetof(ClassGeography, hoverId) == 0x50C);
static_assert(offsetof(ClassGeography, countryPos) == 0x528);
static_assert(offsetof(ClassGeography, slotPos) == 0x5E8);
static_assert(offsetof(ClassGeography, fade) == 0x678);
static_assert(offsetof(ClassGeography, prevTexIds) == 0x680);
static_assert(offsetof(ClassGeography, fall) == 0x758);
static_assert(offsetof(ClassGeography, textureSets) == 0x798);
static_assert(offsetof(ClassGeography, hover) == 0x7E0);
static_assert(offsetof(ClassGeography, slotIds) == 0x7E8);
static_assert(sizeof(ClassGeography) == 0x830);

// the "MG_Geography_Common" textures (+0x1C)
enum GeoTexture : int {
    GEO_TEX_STAR, GEO_TEX_CLOSED_HAND, GEO_TEX_OPEN_HAND, GEO_TEX_HAND_HALO, GEO_TEX_MINUS_5, GEO_TEX_PIN,
    GEO_TEX_X_TARGET,
};
// the sounds (+0x4C: "GeoClass")
enum GeoSound : int {
    GEO_SOUND_SELECT_FLAG, GEO_SOUND_DROP_CORRECT, GEO_SOUND_DROP_INCORRECT, GEO_SOUND_MAP_FINISH,
    GEO_SOUND_NEW_MAP, GEO_SOUND_ROLL_OVER_FLAG, GEO_SOUND_FLAG_SLIDE, GEO_SOUND_FLAG_FALLS,
    GEO_SOUND_TIME_WARNING,
};

// the class's own functions, called through the game's addresses
GAME_FN(void, __thiscall, Geo_ResetCountries, 0x006EE550, ClassGeography* self);
GAME_FN(void*, __thiscall, Geo_GetTexture, 0x006EE5D0, ClassGeography* self, int map, int index);
GAME_FN(void, __thiscall, Geo_SetupEurope, 0x006EE6A0, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_SetupUsa, 0x006EEFD0, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_SetupAsia, 0x006EF870, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_SetupAfrica, 0x006F0110, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_SetupCentralAmerica, 0x006F0A40, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_SetupCanada, 0x006F1360, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_SetupSouthernAfrica, 0x006F1B00, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_SetupOceania, 0x006F23B0, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_SetupNorthEast, 0x006F28C0, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_ShuffleCountries, 0x006F2F30, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_End, 0x006EE420, ClassGeography* self);
GAME_FN(GeoPoint*, __stdcall, Geo_ReadPad, 0x006F3540, GeoPoint* out, int pad);
GAME_FN(void, __thiscall, Geo_PickUp, 0x006F3780, ClassGeography* self, float x, float y);
GAME_FN(uint8_t, __thiscall, Geo_TryPlace, 0x006F39C0, ClassGeography* self, float x, float y, int flag, int player);
GAME_FN(void, __thiscall, Geo_DrawNextMap, 0x006F3D50, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawMap, 0x006F3FF0, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_UpdateTimer, 0x006F4150, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawTimer, 0x006F4280, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawSlide1, 0x006F4480, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawSlide2, 0x006F4640, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawLabel, 0x006F47F0, ClassGeography* self, float x, float y, uint32_t colour, int hoverId,
        int player);
GAME_FN(void, __thiscall, Geo_DrawScore, 0x006F4E30, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawSlots, 0x006F4F60, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DealFlags, 0x006F5190, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawFalling, 0x006F53F0, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawTargets, 0x006F5830, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawPenalty, 0x006F5D20, ClassGeography* self);
GAME_FN(void, __thiscall, Geo_DrawCursor, 0x006F6030, ClassGeography* self, float x, float y, uint32_t colour, int hoverId,
        int player);
GAME_FN(GeoPoint*, __thiscall, Geo_MoveCursor, 0x006F6710, ClassGeography* self, GeoPoint* out, float x, float y,
        int player);
GAME_FN(void, __thiscall, Geo_Drop, 0x006F69A0, ClassGeography* self, float x, float y, int hoverId, uint32_t slot,
        int player);
GAME_FN(float, __thiscall, Geo_GetScore, 0x006F2E60, ClassGeography* self);
GAME_FN(int, __thiscall, Geo_GetMaxScore, 0x006F2E70, ClassGeography* self);   // (the shared "return 1")
GAME_FN(void, __thiscall, Geo_SetTimer, 0x006F2E80, ClassGeography* self, float time, float second);

// the classes' colours (0x20C8300..)
inline uint8_t* GeoColour(uintptr_t address) {
    return school::GameColour(address);
}

} // namespace geo
