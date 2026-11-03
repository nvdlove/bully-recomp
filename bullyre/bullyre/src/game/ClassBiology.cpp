// Biology class (ClassBiology): the dissection minigame (0x6E37F0..0x6E7FD0, with two of
// its script natives). The animal (+0xC8: 0 frog, 1 rat, 2 perch, 3 pigeon, 4 pig, 5 bat,
// 6 turtle, 7 alien) has its own set-up, start and reset functions elsewhere
// (0x716000..0x726000) that fill the incisions, organs, pins and marks used here.
//
// The cursor (+0x130) moves in screen units from the centre; the player picks a tool from
// the column on the left (1 magnifier, 2 pin, 3 forceps, 4 scalpel), places the pins, cuts
// along the incisions' points (each a line, or a closed loop) and lifts the organs out to
// the tray (220, 80). The x87 runs at 24-bit precision here (the main thread): float.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/FEScreensCommon.h"
#include "game/ScriptApi.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// a Minigame's texture table entry (Minigame::LoadTextures and the like)
struct BioTextureEntry {
    int32_t id;
    re::Ptr32<const char> name;
    re::Ptr32<const char> mask;
};
static_assert(sizeof(BioTextureEntry) == 0xC);
struct BioSoundEntry {
    int32_t id;
    re::Ptr32<const char> name;
};
static_assert(sizeof(BioSoundEntry) == 0x8);

// an incision's points are screen positions; a mark and a pin target too
struct BioPoint {
    float x, y;
};
static_assert(sizeof(BioPoint) == 8);

struct ClassBiology {
    re::Ptr32<re::Ptr32<void>> vtable;                     // +0x0
    uint8_t active;                    // +0x4
    uint8_t hud;                       // +0x5
    uint8_t unknown6[0xE];
    int32_t textureCount;              // +0x14 the animal's textures (LoadTextureSets)
    uint8_t unknown18[4];
    re::Ptr32<re::Ptr32<void>> textures;                   // +0x1C
    uint8_t unknown20[4];
    float warnTime;                    // +0x24 the timer turns the warning colour below this
    uint8_t unknown28;
    uint8_t triggered;                 // +0x29 the trigger function ran
    uint8_t unknown2A[2];
    float triggerTime;                 // +0x2C the script's function runs below this
    uint8_t unknown30[4];
    float timeLeft;                    // +0x34 seconds
    int32_t unknown38;                 // +0x38 (1)
    int32_t unknown3C;                 // +0x3C (4)
    uint8_t unknown40[0xC];
    re::Ptr32<uint32_t> sounds;                  // +0x4C
    int32_t popupKind;                 // +0x50 the popup (1 the exit question, 2 the info)
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;              // +0xA0
    uint8_t unknownA1[0x23];
    int32_t unknownC4;                 // +0xC4
    int32_t animal;                    // +0xC8
    int32_t bioTextureCount;           // +0xCC
    int32_t bioTxdSlot;                // +0xD0 "MG_Biology"
    re::Ptr32<re::Ptr32<void>> bioTextures;                // +0xD4 (the tools, icons, markers: see LoadBioTextures)
    int32_t setCount[3];               // +0xD8 the animal's three texture sets
    int32_t setTxdSlot[3];             // +0xE4
    uint8_t timerColour[4];            // +0xF0
    uint32_t triggerFunction;          // +0xF4 a script function's reference
    uint8_t unknownF8[4];
    float unknownFC;                   // +0xFC (SetTimer's second)
    float scorePercentage;             // +0x100
    uint8_t validOperation;            // +0x104
    uint8_t invalidOperation;          // +0x105
    uint8_t unknown106[6];
    int32_t unknown10C;                // +0x10C
    int32_t score;                     // +0x110
    int32_t maxScore;                  // +0x114
    uint8_t unknown118;
    uint8_t badMove;                   // +0x119
    uint8_t badMoveShown;              // +0x11A
    uint8_t grossOut;                  // +0x11B
    re::Ptr32<uint8_t> cutSound;                 // +0x11C a playing sound (+0x5C: stop)
    int32_t unknown120;                // +0x120
    float messageTime;                 // +0x124 the animal's message shows after 25
    int32_t background;                // +0x128
    int32_t padDelay;                  // +0x12C ticks before the stick repeats
    float cursorX, cursorY;            // +0x130
    int32_t cursorImage;               // +0x138 the tool's texture
    float unknown13C;                  // +0x13C (1)
    float unknown140;                  // +0x140 (4)
    int32_t tool;                      // +0x144 0 none, 1 magnifier, 2 pin, 3 forceps, 4 scalpel
    int32_t padDir;                    // +0x148 0 up, 1 down, 2 left, 3 right, 4 none
    int32_t pinCount;                  // +0x14C
    BioPoint pins[4];                  // +0x150
    uint8_t pinned[4];                 // +0x170
    int32_t markCount;                 // +0x174
    BioPoint marks[4];                 // +0x178
    uint8_t marked[4];                 // +0x198
    uint8_t unknown19C[0x2C];
    uint8_t cutting;                   // +0x1C8 a cut under way
    uint8_t unknown1C9[3];
    int32_t cutCount;                  // +0x1CC the incisions
    int32_t cutLoop[3];                // +0x1D0 0 a line, 1 a closed loop
    int32_t unknown1DC[3];
    uint8_t cutStarted[3];             // +0x1E8
    uint8_t cutDone[3];                // +0x1EB
    uint8_t unknown1EE[2];
    int32_t cutDir[3];                 // +0x1F0 the way being cut (-1 / 1, 0 either)
    int32_t pointCount[3];             // +0x1FC
    int32_t cutPoint[3];               // +0x208 the point reached
    uint8_t segmentShown[3][0x40];     // +0x214 drawn as cut
    uint8_t segmentCut[3][0x40];       // +0x2D4
    BioPoint points[3][0x40];          // +0x394
    float bonusBlink[3];               // +0x994
    uint8_t unknown9A0[0xC];
    BioPoint bonusPos[3];              // +0x9AC
    float bonusLeft[3];                // +0x9C4
    int32_t trailCount;                // +0x9D0 the cut's trail (-1 none)
    BioPoint trail[300];               // +0x9D4
    uint8_t organHeld;                 // +0x1334
    uint8_t unknown1335[3];
    int32_t organCount;                // +0x1338
    BioPoint organPos[4];              // +0x133C
    uint8_t organRemoved[4];           // +0x135C
    uint8_t organGrabbed[4];           // +0x1360
    int32_t organTool[4];              // +0x1364 the pad direction that turns it while held
    uint8_t unknown1374[0x20];
    int32_t organImage[4];             // +0x1394 shown on the tray once removed
    int32_t organFrames[4];            // +0x13A4 its markers' count
    int32_t organMarkers[4][4];        // +0x13B4 the markers showing it (as it lies, held, turned)
    int32_t organTurned[4];            // +0x13F4
    int32_t markerCount;               // +0x1404
    BioPoint markerPos[16];            // +0x1408
    float markerScale[16];             // +0x1488
    int32_t markerImage[16];           // +0x14C8
    uint8_t markerVisible[16];         // +0x1508
    uint8_t trayAtDrop;                // +0x1518 the tray where organs are dropped (220, 80)
    uint8_t unknown1519[3];
    float trayX, trayY;                // +0x151C
    uint8_t trayShowsItem;             // +0x1524
    uint8_t unknown1525[3];
    int32_t trayItem;                  // +0x1528
    float trayItemScale;               // +0x152C
    uint8_t trayReturning;             // +0x1530
    uint8_t unknown1531[3];
    int32_t pulseTick;                 // +0x1534
    int32_t pulse;                     // +0x1538 0..8
    int32_t pulseStep;                 // +0x153C
    int32_t cutEffect;                 // +0x1540 a particle effect (0 none)
};
static_assert(offsetof(ClassBiology, animal) == 0xC8);
static_assert(offsetof(ClassBiology, timerColour) == 0xF0);
static_assert(offsetof(ClassBiology, cursorX) == 0x130);
static_assert(offsetof(ClassBiology, pinned) == 0x170);
static_assert(offsetof(ClassBiology, cutting) == 0x1C8);
static_assert(offsetof(ClassBiology, cutDone) == 0x1EB);
static_assert(offsetof(ClassBiology, points) == 0x394);
static_assert(offsetof(ClassBiology, bonusPos) == 0x9AC);
static_assert(offsetof(ClassBiology, trail) == 0x9D4);
static_assert(offsetof(ClassBiology, organHeld) == 0x1334);
static_assert(offsetof(ClassBiology, organMarkers) == 0x13B4);
static_assert(offsetof(ClassBiology, markerVisible) == 0x1508);
static_assert(offsetof(ClassBiology, cutEffect) == 0x1540);
static_assert(sizeof(ClassBiology) == 0x1544);

// the bio textures (+0xD4)
enum BioTexture : int {
    BIO_TEX_HAND, BIO_TEX_MAGNIFIER, BIO_TEX_PIN_LONG, BIO_TEX_FORCEPS, BIO_TEX_SCALPEL, BIO_TEX_PIN,
    BIO_TEX_ICON_MAGNIFY, BIO_TEX_ICON_PIN, BIO_TEX_ICON_FORCEPS, BIO_TEX_ICON_SCALPEL, BIO_TEX_CUT_DOT,
    BIO_TEX_ARROW, BIO_TEX_BONUS, BIO_TEX_X_TARGET, BIO_TEX_PETRI_DISH,
};
// the sounds (+0x4C: "BioClass")
enum BioSound : int {
    BIO_SOUND_SELECT_TOOL, BIO_SOUND_PIN, BIO_SOUND_SCALPEL, BIO_SOUND_TWEEZE, BIO_SOUND_DROP, BIO_SOUND_LIFT,
    BIO_SOUND_ERROR, BIO_SOUND_TIME_WARNING, BIO_SOUND_IDENTIFY,
};

GAME_VAR(float, bioScreenScaleX, 0x00A957A8);
GAME_VAR(float, bioScreenScaleY, 0x00A957AC);
GAME_VAR(float, bioTimeStep, 0x00C1A9A4);
GAME_VAR(int32_t, bioLanguage, 0x00C18724);
GAME_VAR(uint8_t*, bioCurrentMinigame, 0x020C8704);
GAME_VAR(int32_t, bioUserStartResponse, 0x020C87AC);
GAME_VAR(uint8_t, bioRadarHidden, 0x00A459FC);
GAME_VAR(float, bioTimerRate, 0x00BC63A8);       // (0.02)
GAME_VAR(float, bioScoreY, 0x00BC63AC);          // (-190)
GAME_VAR(float, bioTextScale, 0x00BC63D0);
GAME_VAR(float, bioFontScale, 0x00BA4E10);
GAME_VAR(int32_t, bioFontArcade, 0x0208B218);
GAME_VAR(int32_t, bioFontOutlineOn, 0x0208B230);
GAME_VAR(int16_t, bioFontOutline, 0x0208B788);
GAME_ARRAY(uint8_t, bioBackColour, 0x020C8244);
GAME_ARRAY(uint8_t, bioColourWhite, 0x020C8248);   // (the text's outline)
GAME_ARRAY(uint8_t, bioTimerNormal, 0x020C8254);
GAME_ARRAY(uint8_t, bioTimerWarning, 0x020C8258);
GAME_ARRAY(uint8_t, bioScoreColour, 0x020C825C);
GAME_ARRAY(uint8_t, bioMessageColour, 0x020C8260);
GAME_ARRAY(uint8_t, bioCutColour, 0x020C826C);
GAME_ARRAY(uint8_t, bioTextColour, 0x020C8270);
GAME_ARRAY(uint8_t, bioColourShown, 0x020C827C);   // (the opaque one)
GAME_ARRAY(uint8_t, bioColourDim, 0x020C8280);
inline const float* const bioCutWidth = reinterpret_cast<const float*>(0x00944ACC);
inline void* const bioSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const bioCamera = reinterpret_cast<void*>(0x00A14810);

inline const char* GameString(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}
// the CRT's x87 sqrt (0x85C500)
inline float Sqrt(float v) {
    return float(std::sqrt(double(v)));
}
inline float Distance(float dx, float dy) {
    return Sqrt(std::fabs(dx * dx + dy * dy));
}

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Streaming_RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, Streaming_LoadAllRequestedModels, 0x00533EE0, int);
GAME_FN(void, __cdecl, Streaming_RemoveModel, 0x0052F640, int id);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, NiRelease, 0x005F0F90, void* object);
GAME_FN(uint8_t*, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, uint32_t id, const float* pos, int, uintptr_t, int,
        int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, int duration, int strength, int);
GAME_FN(float*, __cdecl, ScreenPosition, 0x0052D4F0, float* out, float x, float y, int hAlign, int vAlign);
GAME_FN(float*, __cdecl, ScreenPositionScaled, 0x0052D590, float* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, DrawLine, 0x0052CF40, float x1, float y1, float x2, float y2, const float* width,
        const uint8_t* colour);
GAME_FN(uint8_t*, __thiscall, RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }

// the minigame's own (Minigame) and its popup
GAME_FN(void, __thiscall, Minigame_Construct, 0x007059D0, void* self);
GAME_FN(void, __thiscall, Minigame_Destruct, 0x00705A50, void* self);
GAME_FN(void, __thiscall, Minigame_EndTextures, 0x00704FD0, void* self);
GAME_FN(void, __thiscall, Minigame_LoadSounds, 0x00704E90, void* self, int count, const BioSoundEntry* entries,
        const char* bank);
GAME_FN(void, __thiscall, Minigame_SetTimerShown, 0x007058A0, void* minigame, float time);
GAME_FN(void, __cdecl, Minigame_EndSequence, 0x00704D00, int);
GAME_FN(int, __thiscall, Popup_Result, 0x00705080, void* popup);
GAME_FN(void, __thiscall, Popup_Show, 0x00705220, void* popup, int, const char* title, const char* buttons, int,
        int);
GAME_FN(void, __thiscall, Popup_Draw, 0x00706000, void* popup);
GAME_FN(int, __cdecl, Effect_Start, 0x007053A0, const char* name, const float* pos, int, int);
GAME_FN(void, __cdecl, Effect_Move, 0x00705470, uintptr_t effect, const float* pos);
GAME_FN(void, __cdecl, FontStyle_Load, 0x00687BC0, int style);
GAME_FN(void, __cdecl, FontStyle_Unload, 0x00687D10, int style);
GAME_FN(void, __cdecl, PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopMemoryTag, 0x0052E710);
GAME_FN(void, __thiscall, Camera_SetFov, 0x00454DB0, void* camera, float fov);
GAME_FN(void, __thiscall, Camera_RestoreFov, 0x00454DE0, void* camera);
GAME_FN(int32_t, __thiscall, Channel_IsPlaying, 0x005A6D60, void* channel);
GAME_FN(void, __thiscall, Channel_Pause, 0x005A6E40, void* channel);
GAME_FN(int32_t, __thiscall, Channel_IsPaused, 0x005A6D80, void* channel);
GAME_FN(void, __thiscall, Channel_Resume, 0x005A6E70, void* channel);

// the script callback
GAME_FN(uint8_t, __thiscall, ScriptRef_IsSet, 0x005D80E0, uint32_t* ref);
GAME_FN(void*, __thiscall, ScriptRef_GetState, 0x005D9C20, uint32_t* ref);
GAME_FN(int32_t, __thiscall, ScriptRef_GetId, 0x005D80D0, uint32_t* ref);
GAME_FN(void, __thiscall, ScriptRef_Clear, 0x005D9C50, uint32_t* ref);
GAME_FN(void, __cdecl, Event_Begin, 0x005D8270, void* state);
GAME_FN(void, __cdecl, Lua_GetRef, 0x005D8670, void* state, int ref);
GAME_FN(void, __cdecl, Lua_Call, 0x005D82A0, void* state, int args, int results);

// the biology class's own (through the game's addresses)
GAME_FN(void, __thiscall, Bio_LoadBioTextures, 0x006E3AF0, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_UnloadBioTextures, 0x006E3D60, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_UnloadTextureSets, 0x006E3A70, ClassBiology* self);
GAME_FN(int, __thiscall, Bio_FirstCuttable, 0x006E3E40, ClassBiology* self, int cut);
GAME_FN(int, __thiscall, Bio_LastCuttable, 0x006E3EE0, ClassBiology* self, int cut);
GAME_FN(void, __thiscall, Bio_TurnOrgan, 0x006E3FA0, ClassBiology* self, int organ);
GAME_FN(void, __thiscall, Bio_Setup, 0x006E4200, ClassBiology* self);
GAME_FN(float*, __thiscall, Bio_ReadPad, 0x006E4540, void* self, float* out);
GAME_FN(void, __thiscall, Bio_PickTool, 0x006E4790, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_ClearCut, 0x006E4AD0, ClassBiology* self, int cut);
GAME_FN(void, __thiscall, Bio_DrawBackground, 0x006E4280, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_UpdateTimer, 0x006E4380, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_UpdateBonuses, 0x006E44C0, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawTimer, 0x006E4E10, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawScore, 0x006E4F30, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawMessage, 0x006E5020, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawToolIcons, 0x006E51E0, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawMarkers, 0x006E53F0, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawMarks, 0x006E5570, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawCuts, 0x006E56C0, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawOrgans, 0x006E6060, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawTray, 0x006E6320, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_DrawCursor, 0x006E6580, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_Destruct, 0x006E66F0, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_HandleInput, 0x006E6A30, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_UpdateTray, 0x006E6E10, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_MoveCursor, 0x006E6F50, ClassBiology* self);
GAME_FN(uint8_t, __thiscall, Bio_AddTrail, 0x006E7080, ClassBiology* self, int cut);
GAME_FN(void, __thiscall, Bio_DrawWorld, 0x006E71B0, ClassBiology* self);
GAME_FN(void, __thiscall, Bio_StartAnimal, 0x006E37F0, ClassBiology* self);

// the animals' own (another pass's)
using BioAnimalFn = void(__thiscall*)(ClassBiology*);
inline BioAnimalFn AnimalFn(uintptr_t address) {
    return reinterpret_cast<BioAnimalFn>(address);
}
// by animal: the start (the pinning stage), the reset, the set-up
constexpr uintptr_t kBioStart[8] = {0x0071A680, 0x00723510, 0x0071C5A0, 0x007215A0,
                                    0x0071E3C0, 0x0071A680, 0x00725A50, 0x00716240};
constexpr uintptr_t kBioReset[8] = {0x0071A9E0, 0x00723930, 0x0071C940, 0x00721980,
                                    0x0071E940, 0x00718980, 0x00725E80, 0x00716560};
constexpr uintptr_t kBioSetup[8] = {0x0071A4D0, 0x00723340, 0x0071C4C0, 0x00721370,
                                    0x0071E1B0, 0x007187D0, 0x00725920, 0x00716040};

inline uint8_t* PlayBioSound(const ClassBiology* bio, int sound) {
    return Sounds_Play(bioSounds, bio->sounds[sound], nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}
inline void StopBioSound(re::Ptr32<uint8_t>& sound) {
    if (sound != nullptr) {
        sound[0x5C] = 1;
        sound = nullptr;
    }
}

// ---- the animal's stages --------------------------------------------------------------------

// 0x6E37F0: the animal's start (the bat uses the frog's)
void __fastcall BioStartAnimal(ClassBiology* bio, re::Edx) {
    if (uint32_t(bio->animal) < 8) {
        AnimalFn(kBioStart[bio->animal])(bio);
    }
}

// 0x6E3850: the message timer restarts with the animal's reset
void __fastcall BioResetAnimal(ClassBiology* bio, re::Edx) {
    bio->messageTime = 0.0f;
    if (uint32_t(bio->animal) < 8) {
        AnimalFn(kBioReset[bio->animal])(bio);
    }
}

// 0x6E38B0: the animal's textures, from three txds: entries [0, n1) from the first,
// [n1, n1 + n2) from the second, the rest of `count` from the third (whose count is kept
// as n2: the game's); the array allocated on the first call. Always 1
uint8_t __fastcall BioLoadTextureSets(ClassBiology* bio, re::Edx, int count, const BioTextureEntry* entries,
                                     const char* txd1, const char* txd2, const char* txd3, int n1, int n2, int) {
    if (bio->textures == nullptr) {
        uint64_t bytes = uint64_t(uint32_t(count)) * 4;
        bio->textures = static_cast<re::Ptr32<void>*>(Malloc(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes)));
    }
    bio->textureCount = count;
    const char* txds[3] = {txd1, txd2, txd3};
    int first[3] = {0, n1, n1 + n2};
    int last[3] = {n1, n1 + n2, count};
    int kept[3] = {n1, n2, n2};
    for (int set = 0; set < 3; ++set) {
        int slot = Txd_FindTxdSlot(txds[set], 0);
        bio->setTxdSlot[set] = slot;
        Streaming_RequestModel(slot + 0x2FA8, 9);
        Streaming_LoadAllRequestedModels(1);
        bio->setCount[set] = kept[set];
        Txd_PushCurrentTxd();
        Txd_SetCurrentTxd(bio->setTxdSlot[set]);
        for (int i = first[set]; i < last[set]; ++i) {
            bio->textures[entries[i].id] = Txd_FindTexture(entries[i].name, entries[i].mask);
        }
        Txd_PopCurrentTxd();
    }
    return 1;
}

// 0x6E3A70: the animal's textures released and their txds removed
void __fastcall BioUnloadTextureSets(ClassBiology* bio, re::Edx) {
    if (bio->textures == nullptr) {
        return;
    }
    for (int i = 0; i < bio->textureCount; ++i) {
        if (bio->textures[i] != nullptr) {
            NiRelease(bio->textures[i]);
        }
    }
    Free(bio->textures);
    for (int set = 0; set < 3; ++set) {
        Streaming_RemoveModel(bio->setTxdSlot[set] + 0x2FA8);
        bio->setTxdSlot[set] = -1;
        bio->setCount[set] = 0;
    }
    bio->textures = nullptr;
    bio->textureCount = 0;
}

// 0x6E3AF0: the tools' and markers' textures from "MG_Biology"; 0 when one is missing
uint8_t __fastcall BioLoadBioTextures(ClassBiology* bio, re::Edx) {
    const BioTextureEntry entries[15] = {
        {0, GameString(0x0093ED8C), GameString(0x00944C80)},    // hand2
        {1, GameString(0x00944C74), GameString(0x00944C64)},    // magnifying
        {2, GameString(0x00944C58), GameString(0x00944C4C)},    // pin_long
        {3, GameString(0x00944C44), GameString(0x00944C38)},    // forceps
        {4, GameString(0x00944C30), GameString(0x00944C24)},    // scalpel
        {5, GameString(0x00944C20), GameString(0x00944C18)},    // pin
        {6, GameString(0x00944C08), GameString(0x00944BF8)},    // icon_magnify
        {7, GameString(0x00944BEC), GameString(0x00944BE0)},    // icon_pin
        {8, GameString(0x00944BD0), GameString(0x00944BC0)},    // icon_forceps
        {9, GameString(0x00944BB0), GameString(0x00944BA0)},    // icon_scalpel
        {10, GameString(0x00944B98), GameString(0x00944B8C)},   // cut_dot
        {11, GameString(0x00944B80), GameString(0x00944B74)},   // bio_arrow
        {12, GameString(0x00944B68), GameString(0x00944B5C)},   // bio_bonus
        {13, GameString(0x00944B50), GameString(0x00944B44)},   // x_target
        {14, GameString(0x00944B38), GameString(0x00944B28)},   // petri_dish
    };
    uint8_t ok = 1;
    int slot = Txd_FindTxdSlot(GameString(0x00944C88), 0);   // "MG_Biology"
    bio->bioTxdSlot = slot;
    Streaming_RequestModel(slot + 0x2FA8, 9);
    Streaming_LoadAllRequestedModels(1);
    bio->bioTextures = static_cast<re::Ptr32<void>*>(Malloc(15 * 4));
    bio->bioTextureCount = 15;
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(bio->bioTxdSlot);
    for (const BioTextureEntry& e : entries) {
        bio->bioTextures[e.id] = Txd_FindTexture(e.name, e.mask);
        if (bio->bioTextures[e.id] == nullptr) {
            ok = 0;
        }
    }
    Txd_PopCurrentTxd();
    return ok;
}

// 0x6E3D60
void __fastcall BioUnloadBioTextures(ClassBiology* bio, re::Edx) {
    if (bio->bioTextures == nullptr) {
        return;
    }
    for (int i = 0; i < bio->bioTextureCount; ++i) {
        NiRelease(bio->bioTextures[i]);
    }
    Free(bio->bioTextures);
    Streaming_RemoveModel(bio->bioTxdSlot + 0x2FA8);
    bio->bioTextures = nullptr;
    bio->bioTextureCount = 0;
    bio->bioTxdSlot = -1;
}

// 0x6E3DE0: every incision uncut (its argument unused)
void __fastcall BioResetCuts(ClassBiology* bio, re::Edx, int) {
    for (int c = 0; c < bio->cutCount; ++c) {
        bio->cutDone[c] = 0;
        bio->cutStarted[c] = 0;
        for (int i = 0; i < bio->pointCount[c]; ++i) {
            bio->segmentShown[c][i] = 0;
            bio->segmentCut[c][i] = 0;
        }
    }
}

// 0x6E3E40: where a cut can start: a line's first uncut segment; a loop's first segment
// on the other side of the cut from its last one (-1: none)
int __fastcall BioFirstCuttable(ClassBiology* bio, re::Edx, int cut) {
    const uint8_t* segmentCut = bio->segmentCut[cut];
    int count = bio->pointCount[cut];
    if (bio->cutLoop[cut] == 0) {
        for (int i = 0; i < count - 1; ++i) {
            if (segmentCut[i] == 0) {
                return i;
            }
        }
        return -1;
    }
    if (bio->cutLoop[cut] == 1) {
        uint8_t last = segmentCut[count - 1];
        for (int i = 0; i < count; ++i) {
            if (last == 0) {
                if (segmentCut[i] == 1) {
                    return i;
                }
            } else if (last == 1) {
                if (segmentCut[i] == 0) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// 0x6E3EE0: the other end: a line's last point after an uncut segment, counting down; a
// loop's second change of cut along it
int __fastcall BioLastCuttable(ClassBiology* bio, re::Edx, int cut) {
    const uint8_t* segmentCut = bio->segmentCut[cut];
    int count = bio->pointCount[cut];
    if (bio->cutLoop[cut] == 0) {
        int last = count - 1;
        for (int i = last; i >= 0; --i) {
            int before = i != 0 ? i - 1 : last;
            if (segmentCut[before] == 0) {
                return i;
            }
        }
        return -1;
    }
    if (bio->cutLoop[cut] == 1) {
        uint8_t state = segmentCut[count - 1];
        int changes = 0;
        for (int i = 0; i < count; ++i) {
            if (state == 0) {
                if (segmentCut[i] == 1) {
                    state = 1;
                    ++changes;
                }
            } else if (state == 1) {
                if (segmentCut[i] == 0) {
                    state = 0;
                    ++changes;
                }
            }
            if (changes == 2) {
                return i;
            }
        }
    }
    return -1;
}

// 0x6E3FA0: the held organ turned: with three markers (or a non-zero organ index's byte,
// with other counts: the game tests the index) it is dropped turned; with four it
// toggles between two held markers
void __fastcall BioTurnOrgan(ClassBiology* bio, re::Edx, int organ) {
    int frames = bio->organFrames[organ];
    const int32_t* markers = bio->organMarkers[organ];
    if (frames != 3 && (frames == 4 || uint8_t(organ) == 0)) {
        bio->markerVisible[markers[1]] = 0;
        if (bio->organTurned[organ] == 0) {
            bio->markerVisible[markers[2]] = 1;
            bio->organTurned[organ] = 1;
            return;
        }
        bio->markerVisible[markers[2]] = 0;
        bio->markerVisible[markers[3]] = 1;
        bio->organTurned[organ] = 0;
        bio->organHeld = 0;
        bio->organRemoved[organ] = 1;
        return;
    }
    bio->markerVisible[markers[1]] = 0;
    bio->markerVisible[markers[2]] = 1;
    bio->organHeld = 0;
    bio->organRemoved[organ] = 1;
}

// 0x6E4080: ClassBiologySetAnimal's (clamped to 0..7)
void __fastcall BioSetAnimal(ClassBiology* bio, re::Edx, int animal) {
    if (animal < 0) {
        bio->animal = 0;
        return;
    }
    if (animal >= 8) {
        animal = 7;
    }
    bio->animal = animal;
}

// 0x6E40B0, 0x6E40D0: a valid / invalid operation since the last ask (cleared)
uint8_t __fastcall BioTakeValidOperation(ClassBiology* bio, re::Edx) {
    uint8_t was = bio->validOperation;
    bio->validOperation = 0;
    return was == 1 ? 1 : 0;
}
uint8_t __fastcall BioTakeInvalidOperation(ClassBiology* bio, re::Edx) {
    uint8_t was = bio->invalidOperation;
    bio->invalidOperation = 0;
    return was == 1 ? 1 : 0;
}

// 0x6E40F0: ClassBiologySetTimer's
void __fastcall BioSetTimer(ClassBiology* bio, re::Edx, float time, float second) {
    bio->timeLeft = time;
    bio->unknownFC = second;
}

// 0x6E4110: ClassBiologySubtractTime's (not below zero)
void __fastcall BioSubtractTime(ClassBiology* bio, re::Edx, float time) {
    float left = bio->timeLeft - time;
    bio->timeLeft = left;
    if (0.0f > left) {
        bio->timeLeft = 0.0f;
    }
}

// 0x6E4140
ClassBiology* __fastcall BioConstruct(ClassBiology* bio, re::Edx) {
    Minigame_Construct(bio);
    bio->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00944C98);
    bio->triggerFunction = 0;
    bio->unknownC4 = 0;
    bio->bioTextureCount = 0;
    bio->bioTextures = nullptr;
    bio->animal = 0;
    bio->badMoveShown = 0;
    bio->badMove = 0;
    bio->unknown38 = 1;
    bio->unknown3C = 4;
    bio->bioTxdSlot = -1;
    bio->setCount[0] = 0;
    bio->setCount[1] = 0;
    bio->setCount[2] = 0;
    bio->setTxdSlot[0] = -1;
    bio->setTxdSlot[1] = -1;
    bio->setTxdSlot[2] = -1;
    return bio;
}

// 0x6E41C0 (virtual +0x14): the game ends: the textures released, the font style, the
// radar and the camera's field of view back
void __fastcall BioEnd(ClassBiology* bio, re::Edx) {
    bio->active = 0;
    bio->hud = 0;
    Minigame_EndTextures(bio);
    Bio_UnloadBioTextures(bio);
    Bio_UnloadTextureSets(bio);
    FontStyle_Unload(6);
    bioRadarHidden = 1;
    Camera_RestoreFov(bioCamera);
}

// 0x6E4200: the bio textures, then the animal's set-up
void __fastcall BioSetup(ClassBiology* bio, re::Edx) {
    Bio_LoadBioTextures(bio);
    if (uint32_t(bio->animal) < 8) {
        AnimalFn(kBioSetup[bio->animal])(bio);
    }
}

// ---- drawing helpers ------------------------------------------------------------------------

// a texture's size (its virtuals +0x48, +0x4C)
inline uint32_t TexWidth(void* texture) {
    return vc::Call<uint32_t>(texture, 0x48);
}
inline uint32_t TexHeight(void* texture) {
    return vc::Call<uint32_t>(texture, 0x4C);
}

// a texture centred on a screen point, half `halfW` across and `halfH` down, its whole
// image (texture coordinates (0, 0)..(1, 1))
void DrawCentred(void* texture, const uint8_t* colour, float x, float y, float halfW, float halfH) {
    const float uv1[2] = {0.0f, 0.0f};
    const float uv2[2] = {1.0f, 1.0f};
    DrawRectUV(x - halfW, y - halfH, x + halfW, y + halfH, texture, colour, uv1, uv2);
}
// the usual size: the texture's own (as signed ints), scaled across by the screen
inline void DrawCentredTexture(void* texture, const uint8_t* colour, const float* pos) {
    int w = int(TexWidth(texture));
    int h = int(TexHeight(texture));
    DrawCentred(texture, colour, pos[0], pos[1], 0.5f * float(w) * bioScreenScaleX, float(h) * 0.5f);
}

// text outlined in white (drawn at the four diagonal offsets) over the text colour
void PrintOutlined(float x, float y, const uint16_t* text) {
    std::memcpy(fescr::g_fontColour, bioColourWhite, 4);
    fescr::Font_PrintString(x + 1.0f, y + 1.0f, text, 1);
    fescr::Font_PrintString(x + 1.0f, y - 1.0f, text, 1);
    fescr::Font_PrintString(x - 1.0f, y - 1.0f, text, 1);
    fescr::Font_PrintString(x - 1.0f, y + 1.0f, text, 1);
    std::memcpy(fescr::g_fontColour, bioTextColour, 4);
    fescr::Font_PrintString(x, y, text, 1);
}

// ---- drawing --------------------------------------------------------------------------------

// 0x6E4280: the backdrop (one of three by +0x128) over the 4:3 area. (For other values
// the game indexes the textures with the half width's float bits.)
void __fastcall BioDrawBackground(ClassBiology* bio, re::Edx) {
    float halfW = bioScreenScaleX * 426.5f;
    int index;
    switch (bio->background) {
    case 0:
        index = 0;
        break;
    case 1:
        index = 1;
        break;
    case 2:
        index = 2;
        break;
    default:
        std::memcpy(&index, &halfW, 4);
        break;
    }
    float centre[2];
    ScreenPositionScaled(centre, 0.0f, 0.0f, 1, 1);
    const float uv1[2] = {0.0f, 0.0f};
    const float uv2[2] = {1.0f, 1.0f};
    void* texture = bio->textures != nullptr ? bio->textures[index] : nullptr;
    DrawRectUV(centre[0] - halfW, centre[1] - 240.0f, halfW + centre[0], centre[1] + 240.0f, texture, bioBackColour,
               uv1, uv2);
}

// 0x6E4380: the timer runs down (at 0: the game ends); its colour turns below the warning
// time, and the script's trigger function runs once below the trigger time
void __fastcall BioUpdateTimer(ClassBiology* bio, re::Edx) {
    float left = bio->timeLeft - bioTimerRate * bioTimeStep;
    bio->timeLeft = left;
    Minigame_SetTimerShown(bio, left);
    if (!(0.0f < bio->timeLeft)) {
        bio->timeLeft = 0.0f;
        bio->active = 0;
        Minigame_EndSequence(0);
        StopBioSound(bio->cutSound);
        return;
    }
    if (bio->warnTime < bio->timeLeft) {
        std::memcpy(bio->timerColour, bioTimerNormal, 4);
    } else {
        std::memcpy(bio->timerColour, bioTimerWarning, 4);
    }
    if (bio->triggered == 0 && !(bio->triggerTime < bio->timeLeft)) {
        bio->triggered = 1;
        if (ScriptRef_IsSet(&bio->triggerFunction) != 0) {
            void* state = ScriptRef_GetState(&bio->triggerFunction);
            Event_Begin(state);
            Lua_GetRef(state, ScriptRef_GetId(&bio->triggerFunction));
            Lua_Call(state, 0, 0);
        }
    }
}

// 0x6E44C0: the bonuses' counters run down, a point every half "second" of the timer's
// rate
void __fastcall BioUpdateBonuses(ClassBiology* bio, re::Edx) {
    float rate = bioTimerRate;
    for (int c = 0; c < bio->cutCount; ++c) {
        if (0.0f < bio->bonusLeft[c]) {
            float blink = rate * bioTimeStep + bio->bonusBlink[c];
            bio->bonusBlink[c] = blink;
            if (blink > 0.5f) {
                bio->bonusBlink[c] = 0.0f;
                bio->bonusLeft[c] = bio->bonusLeft[c] - 1.0f;
            }
        }
    }
}

// 0x6E4540: the cursor's direction: the d-pad (1 each way), else both sticks summed
// (inverted) past a 0.05 dead zone
float* __fastcall BioReadPad(void*, re::Edx, float* out) {
    out[0] = 0.0f;
    out[1] = 0.0f;
    if (Pad_GetValue(1, 0) != 0.0f || Pad_GetValue(0x10, 0) < -0.05f || Pad_GetValue(0x12, 0) < -0.05f) {
        if (Pad_GetValue(1, 0) != 0.0f) {
            out[0] = 1.0f;
        } else {
            float b = Pad_GetValue(0x12, 0);
            out[0] = -(Pad_GetValue(0x10, 0) + b);
        }
    }
    if (Pad_GetValue(0, 0) != 0.0f || Pad_GetValue(0x10, 0) > 0.05f || Pad_GetValue(0x12, 0) > 0.05f) {
        if (Pad_GetValue(0, 0) != 0.0f) {
            out[0] = -1.0f;
        } else {
            float b = Pad_GetValue(0x12, 0);
            out[0] = -(Pad_GetValue(0x10, 0) + b);
        }
    }
    if (Pad_GetValue(2, 0) != 0.0f || Pad_GetValue(0x11, 0) > 0.05f || Pad_GetValue(0x13, 0) > 0.05f) {
        if (Pad_GetValue(2, 0) != 0.0f) {
            out[1] = -1.0f;
        } else {
            float b = Pad_GetValue(0x13, 0);
            out[1] = -(Pad_GetValue(0x11, 0) + b);
        }
    }
    if (Pad_GetValue(3, 0) != 0.0f || Pad_GetValue(0x11, 0) < -0.05f || Pad_GetValue(0x13, 0) < -0.05f) {
        if (Pad_GetValue(3, 0) != 0.0f) {
            out[1] = 1.0f;
        } else {
            float b = Pad_GetValue(0x13, 0);
            out[1] = -(Pad_GetValue(0x11, 0) + b);
        }
    }
    return out;
}

// the tool column: its icons' points, in the tool order
constexpr float kBioToolY[4] = {-100.0f, -33.0f, 33.0f, 100.0f};

// 0x6E4790: the tool under the cursor picked (within 25)
void __fastcall BioPickTool(ClassBiology* bio, re::Edx) {
    static constexpr int kTool[4] = {1, 2, 3, 4};
    static constexpr int kImage[4] = {1, 2, 4, 3};
    for (int i = 0; i < 4; ++i) {
        float dx = bio->cursorX - -220.0f;
        float dy = bio->cursorY - kBioToolY[i];
        if (Distance(dx, dy) <= 25.0f) {
            PlayBioSound(bio, BIO_SOUND_SELECT_TOOL);
            Pad_Rumble(0, 0x50, 0xE, 1);
            bio->cursorImage = kImage[i];
            bio->tool = kTool[i];
            return;
        }
    }
}

// 0x6E48F0: the pins placed (each within 40 of its target); 1 once all are
uint8_t __fastcall BioCheckPins(ClassBiology* bio, re::Edx) {
    int placed = 0;
    for (int i = 0; i < bio->pinCount; ++i) {
        uint8_t was = bio->pinned[i];
        if (was == 0) {
            if (Distance(bio->cursorX - bio->pins[i].x, bio->cursorY - bio->pins[i].y) <= 40.0f) {
                bio->pinned[i] = 1;
                ++placed;
            }
        } else if (was == 1) {
            ++placed;
        }
    }
    return placed == bio->pinCount ? 1 : 0;
}

// 0x6E49C0: the marks hit (within 20: the pin sound, the tool put down); 1 once all are
uint8_t __fastcall BioCheckMarks(ClassBiology* bio, re::Edx) {
    int marked = 0;
    for (int i = 0; i < bio->markCount; ++i) {
        if (bio->marked[i] == 0 &&
            Distance(bio->cursorX - bio->marks[i].x, bio->cursorY - bio->marks[i].y) <= 20.0f) {
            bio->marked[i] = 1;
            PlayBioSound(bio, BIO_SOUND_PIN);
            bio->tool = 0;
            bio->cursorImage = 0;
        }
        if (bio->marked[i] == 1) {
            ++marked;
        }
    }
    return marked == bio->markCount ? 1 : 0;
}

// 0x6E4AD0: an incision's unfinished cut wiped (the segments shown as cut but not cut,
// and their ends); the trail cleared
void __fastcall BioClearCut(ClassBiology* bio, re::Edx, int cut) {
    bio->cutting = 0;
    int count = bio->pointCount[cut];
    for (int i = 0; i < count; ++i) {
        int last = count - 1;
        int next = i == last ? 0 : i + 1;
        int prev = i != 0 ? i - 1 : last;
        uint8_t* shown = bio->segmentShown[cut];
        const uint8_t* segmentCut = bio->segmentCut[cut];
        if (bio->cutLoop[cut] == 0) {
            if (segmentCut[i] == 0) {
                if (i == count - 1) {
                    if (segmentCut[prev] == 0) {
                        shown[i] = 0;
                    }
                } else if (i == count - 2) {
                    shown[i] = 0;
                } else if (i < count - 2) {
                    shown[i] = 0;
                    shown[next] = 0;
                }
            }
        } else if (bio->cutLoop[cut] == 1 && segmentCut[i] == 0) {
            shown[i] = 0;
            shown[next] = 0;
        }
        count = bio->pointCount[cut];
    }
    bio->trailCount = -1;
}

// 0x6E4BB0: the organs: held, one is turned by its pad direction and dropped on the tray
// (within 40 of (220, 80)); else one is lifted (within 25). 1 once all are out
uint8_t __fastcall BioUpdateOrgans(ClassBiology* bio, re::Edx) {
    if (bio->organHeld == 1) {
        for (int i = 0; i < bio->organCount; ++i) {
            if (bio->organRemoved[i] != 0 || bio->organGrabbed[i] != 1) {
                continue;
            }
            if (bio->organTool[i] == bio->padDir) {
                Bio_TurnOrgan(bio, i);
            }
            if (Distance(bio->cursorX - 220.0f, bio->cursorY - 80.0f) <= 40.0f) {
                PlayBioSound(bio, BIO_SOUND_DROP);
                bio->organHeld = 0;
                bio->organGrabbed[i] = 0;
                bio->organRemoved[i] = 1;
                bio->trayShowsItem = 1;
                bio->trayItem = bio->organImage[i];
            }
        }
    } else {
        for (int i = 0; i < bio->organCount; ++i) {
            if (bio->organRemoved[i] != 0) {
                continue;
            }
            if (Distance(bio->cursorX - bio->organPos[i].x, bio->cursorY - bio->organPos[i].y) <= 25.0f) {
                PlayBioSound(bio, BIO_SOUND_LIFT);
                bio->markerVisible[bio->organMarkers[i][0]] = 0;
                if (bio->organFrames[i] > 1) {
                    bio->markerVisible[bio->organMarkers[i][1]] = 1;
                }
                bio->organGrabbed[i] = 1;
                bio->organHeld = 1;
            }
        }
    }
    int removed = 0;
    for (int i = 0; i < bio->organCount; ++i) {
        if (bio->organRemoved[i] == 1) {
            ++removed;
        }
    }
    return removed == bio->organCount ? 1 : 0;
}

// 0x6E4E10: the time left, "MGBI_TIME" (minutes, seconds)
void __fastcall BioDrawTimer(ClassBiology* bio, re::Edx) {
    float pos[2];
    ScreenPosition(pos, -180.0f, -190.0f, 1, 1);
    std::memcpy(fescr::g_fontColour, bio->timerColour, 4);
    if (bioFontArcade != 0) {
        bioFontOutline = 0;
    }
    fescr::Font_SetSizePreset(3);
    fescr::Font_SetJustify(2);
    fescr::g_fontDrop = 2;
    fescr::Font_AddNumber(float(int(bio->timeLeft / 60.0f)));
    fescr::Font_AddNumber(float(int(bio->timeLeft) % 60));
    uint16_t text[0x40];
    fescr::Font_Format(text, fescr::Text_Get(fescr::kTheText, GameString(0x00944CC0)), 0x40);   // "MGBI_TIME"
    fescr::Font_PrintString(pos[0], pos[1], text, 1);
    fescr::Font_Render();
}

// 0x6E4F30: the score, percent of the most, "MGBI_SCORE"
void __fastcall BioDrawScore(ClassBiology* bio, re::Edx) {
    int percent = 0;
    if (bio->score > 0) {
        percent = int(float(bio->score) / float(bio->maxScore) * 100.0f);
    }
    std::memcpy(fescr::g_fontColour, bioScoreColour, 4);
    fescr::g_fontDrop = 2;
    fescr::Font_AddNumber(float(percent));
    uint16_t text[0x40];
    fescr::Font_Format(text, fescr::Text_Get(fescr::kTheText, GameString(0x00944CCC)), 0x40);   // "MGBI_SCORE"
    float pos[2];
    ScreenPosition(pos, 240.0f, bioScoreY, 1, 1);
    fescr::Font_PrintString(pos[0], pos[1], text, 1);
    fescr::Font_Render();
}

// 0x6E5020: after 25 of the timer's "seconds", the animal's message for the score
// ("MGBI_FROG_<score>" ...). (Past the eighth animal the key is the stack's leftovers.)
void __fastcall BioDrawMessage(ClassBiology* bio, re::Edx) {
    static constexpr uintptr_t kFormats[8] = {0x00944D48, 0x00944D3C, 0x00944D2C, 0x00944D1C,
                                              0x00944D10, 0x00944D04, 0x00944CF4, 0x00944CE4};
    float t = bioTimeStep + bio->messageTime;
    bio->messageTime = t;
    if (t > 25.0f) {
        char key[32];
        if (uint32_t(bio->animal) < 8) {
            Sprintf(key, GameString(kFormats[bio->animal]), bio->score);
        }
        std::memcpy(fescr::g_fontColour, bioMessageColour, 4);
        if (bioFontArcade != 0) {
            bioFontOutline = 0;
        }
        fescr::Font_SetSizePreset(1);
        fescr::Font_SetJustify(2);
        fescr::g_fontDrop = 1;
        uint16_t text[0x80];
        fescr::Font_Format(text, fescr::Text_Get(fescr::kTheText, key), 0x80);
        float pos[2];
        ScreenPosition(pos, 0.0f, 182.0f, 1, 1);
        fescr::Font_PrintString(pos[0], pos[1], text, 1);
    }
    fescr::Font_Render();
}

// 0x6E51E0: the tool column (the chosen tool's icon dimmed); the icons at a quarter of
// their size
void __fastcall BioDrawToolIcons(ClassBiology* bio, re::Edx) {
    static constexpr int kIcon[4] = {BIO_TEX_ICON_MAGNIFY, BIO_TEX_ICON_PIN, BIO_TEX_ICON_SCALPEL,
                                     BIO_TEX_ICON_FORCEPS};
    float scaleX = bioScreenScaleX;
    float scaleY = bioScreenScaleY;
    for (int tool = 1; tool < 5; ++tool) {
        void* texture = bio->bioTextures[kIcon[tool - 1]];
        int halfW = int(float(TexWidth(texture)) * 0.5f);
        int halfH = int(float(TexHeight(texture)) * 0.5f);
        uint8_t colour[4];
        std::memcpy(colour, tool == bio->tool ? bioColourDim : bioColourShown, 4);
        float pos[2];
        ScreenPosition(pos, -220.0f, kBioToolY[tool - 1], 1, 1);
        DrawCentred(texture, colour, pos[0], pos[1], 0.5f * float(halfW) * scaleX, float(halfH) * 0.5f * scaleY);
    }
}

// 0x6E53F0: the animal's markers shown (each at its own scale)
void __fastcall BioDrawMarkers(ClassBiology* bio, re::Edx) {
    float scaleX = bioScreenScaleX;
    for (int i = 0; i < bio->markerCount; ++i) {
        if (bio->markerVisible[i] != 1 || bio->textures == nullptr) {
            continue;
        }
        void* texture = bio->textures[bio->markerImage[i]];
        if (texture == nullptr) {
            continue;
        }
        int w = int(float(TexWidth(texture)) * bio->markerScale[i]);
        int h = int(float(TexHeight(texture)) * bio->markerScale[i]);
        float pos[2];
        ScreenPosition(pos, bio->markerPos[i].x, bio->markerPos[i].y, 1, 1);
        DrawCentred(texture, bioColourShown, pos[0], pos[1], 0.5f * float(w) * scaleX, float(h) * 0.5f);
    }
}

// 0x6E5570: the marks (an x, a pin once marked)
void __fastcall BioDrawMarks(ClassBiology* bio, re::Edx) {
    for (int i = 0; i < bio->markCount; ++i) {
        void* texture = bio->bioTextures[BIO_TEX_X_TARGET];
        uint8_t colour[4];
        std::memcpy(colour, bioColourDim, 4);
        if (bio->marked[i] == 1) {
            texture = bio->bioTextures[BIO_TEX_PIN];
            std::memcpy(colour, bioColourShown, 4);
        }
        float pos[2];
        int w = int(TexWidth(texture));
        int h = int(TexHeight(texture));
        ScreenPosition(pos, bio->marks[i].x, bio->marks[i].y, 1, 1);
        DrawCentred(texture, colour, pos[0], pos[1], 0.5f * float(w) * bioScreenScaleX, float(h) * 0.5f);
    }
}

// 0x6E56C0: the incisions (the cut segments as lines, the uncut points as dots); then
// each bonus still counting: its icon and "MGB2_TB1", "MGB2_TB2" and the points left
void __fastcall BioDrawCuts(ClassBiology* bio, re::Edx) {
    for (int c = 0; c < bio->cutCount; ++c) {
        if (bio->cutDone[c] != 0) {
            continue;
        }
        for (int i = 0; i < bio->pointCount[c]; ++i) {
            const BioPoint point = bio->points[c][i];
            int last = bio->pointCount[c] - 1;
            int next = i == last ? 0 : i + 1;
            int prev = i != 0 ? i - 1 : last;
            float here[2];
            float before[2];   // (worked out, unused)
            float after[2];
            ScreenPosition(here, point.x, point.y, 1, 1);
            ScreenPosition(before, bio->points[c][prev].x, bio->points[c][prev].y, 1, 1);
            ScreenPosition(after, bio->points[c][next].x, bio->points[c][next].y, 1, 1);
            if (bio->segmentCut[c][i] == 1) {
                DrawLine(here[0], here[1], after[0], after[1], bioCutWidth, bioCutColour);
            }
            if (bio->segmentShown[c][i] == 0) {
                void* dot = bio->bioTextures[BIO_TEX_CUT_DOT];
                uint8_t colour[4];
                std::memcpy(colour, bioColourDim, 4);
                DrawCentredTexture(dot, colour, here);
            }
        }
    }
    for (int c = 0; c < bio->cutCount; ++c) {
        if (!(0.0f < bio->bonusLeft[c])) {
            continue;
        }
        const BioPoint at = bio->bonusPos[c];
        float pos[2];
        ScreenPosition(pos, at.x, at.y, 1, 1);
        void* icon = bio->bioTextures[BIO_TEX_BONUS];
        uint8_t colour[4];
        std::memcpy(colour, bioColourShown, 4);
        DrawCentredTexture(icon, colour, pos);

        ScreenPosition(pos, at.x, (at.y - 10.0f) - 55.0f, 1, 1);
        if (bioLanguage != 7 && bioLanguage != 6 && bioFontOutlineOn != 0) {
            bioFontOutline = 6;
        }
        fescr::g_fontDrop = 0;
        fescr::Font_SetJustify(2);
        char key[32];
        uint16_t text[0x80];
        Sprintf(key, GameString(0x00944D78));   // "MGB2_TB1"
        fescr::Font_Format(text, fescr::Text_Get(fescr::kTheText, key), 0x40);
        if (bioLanguage == 7) {
            bioTextScale = 0.9f;
        }
        Font_SetScale(bioTextScale);
        PrintOutlined(pos[0], pos[1], text);

        ScreenPosition(pos, at.x, (at.y - 10.0f) - 38.0f, 1, 1);
        Sprintf(key, GameString(0x00944D60));   // "MGB2_TB2"
        fescr::Font_Format(text, fescr::Text_Get(fescr::kTheText, key), 0x40);
        Font_SetScale(bioTextScale);
        PrintOutlined(pos[0], pos[1], text);

        ScreenPosition(pos, at.x, at.y - 18.5f, 1, 1);
        Sprintf(key, GameString(0x00944CCC));   // "MGBI_SCORE"
        fescr::Font_AddNumber(float(int(bio->bonusLeft[c])));
        fescr::Font_Format(text, fescr::Text_Get(fescr::kTheText, key), 0x40);
        Font_SetScale(0.9f);
        PrintOutlined(pos[0], pos[1], text);
    }
    fescr::Font_Render();
}

// 0x6E6060: the organs: a target on each not yet touched; the held one (when its tool is
// 5) at the cursor, at the first organ's first marker's scale (the game's) and 1.2 more
void __fastcall BioDrawOrgans(ClassBiology* bio, re::Edx) {
    float scaleX = bioScreenScaleX;
    for (int i = 0; i < bio->organCount; ++i) {
        if (bio->organRemoved[i] != 0) {
            continue;
        }
        if (bio->organHeld == 0 && bio->organGrabbed[i] == 0) {
            void* target = bio->bioTextures[BIO_TEX_X_TARGET];
            int w = int(TexWidth(target));
            int h = int(TexHeight(target));
            float pos[2];
            ScreenPosition(pos, bio->organPos[i].x, bio->organPos[i].y, 1, 1);
            DrawCentred(target, bioColourDim, pos[0], pos[1], 0.5f * float(w) * scaleX, float(h) * 0.5f);
        }
        if (bio->organHeld == 1 && bio->organGrabbed[i] == 1 && bio->organTool[i] == 5 && bio->textures != nullptr) {
            void* image = bio->textures[bio->organImage[i]];
            if (image == nullptr) {
                continue;
            }
            float scale = bio->markerScale[bio->organMarkers[0][0]];
            int w = int(float(double(float(TexWidth(image)) * scale) * 1.2));
            int h = int(float(double(float(TexHeight(image)) * scale) * 1.2));
            float pos[2];
            ScreenPosition(pos, bio->cursorX, bio->cursorY, 1, 1);
            DrawCentred(image, bioColourShown, pos[0], pos[1], 0.5f * float(w) * scaleX, float(h) * 0.5f);
        }
    }
}

// 0x6E6320: the tray (the petri dish) and the organ last dropped on it
void __fastcall BioDrawTray(ClassBiology* bio, re::Edx) {
    float scaleX = bioScreenScaleX;
    float pos[2];
    ScreenPosition(pos, bio->trayX, bio->trayY, 1, 1);
    void* dish = bio->bioTextures[BIO_TEX_PETRI_DISH];
    int w = int(double(TexWidth(dish)));
    int h = int(double(TexHeight(dish)));
    DrawCentred(dish, bioColourShown, pos[0], pos[1], 0.5f * float(w) * scaleX, float(h) * 0.5f);
    if (bio->trayShowsItem != 1 || bio->textures == nullptr) {
        return;
    }
    void* item = bio->textures[bio->trayItem];
    if (item == nullptr) {
        return;
    }
    w = int(float(TexWidth(item)) * bio->trayItemScale);
    h = int(float(TexHeight(item)) * bio->trayItemScale);
    float at[2];
    float* p = ScreenPosition(at, bio->trayX, bio->trayY, 1, 1);
    pos[0] = p[0];
    pos[1] = p[1];
    DrawCentred(item, bioColourShown, pos[0], pos[1], 0.5f * float(w) * scaleX, float(h) * 0.5f);
}

// 0x6E6580: the cursor: the tool's texture at three quarters, in amber
void __fastcall BioDrawCursor(ClassBiology* bio, re::Edx) {
    float scaleX = bioScreenScaleX;
    uint8_t colour[4];
    uint8_t set[4];
    std::memcpy(colour, bioColourShown, 4);
    std::memcpy(colour, RGBA_Set(set, 0xFF, 0xC0, 0, 0xFF), 4);
    float pos[2];
    ScreenPosition(pos, bio->cursorX, bio->cursorY, 1, 1);
    void* texture = bio->bioTextures[bio->cursorImage];
    int w = int(float(TexWidth(texture)) * 0.75f);
    int h = int(float(TexHeight(texture)) * 0.75f);
    DrawCentred(texture, colour, pos[0], pos[1], 0.5f * float(w) * scaleX, float(h) * 0.5f);
}

// ---- the game's virtuals and its update -----------------------------------------------------

// 0x6E66F0: the destructor (as End, then the trigger function's reference cleared)
void __fastcall BioDestruct(ClassBiology* bio, re::Edx) {
    bio->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00944C98);
    bio->active = 0;
    bio->hud = 0;
    Minigame_EndTextures(bio);
    Bio_UnloadBioTextures(bio);
    Bio_UnloadTextureSets(bio);
    FontStyle_Unload(6);
    bioRadarHidden = 1;
    Camera_RestoreFov(bioCamera);
    ScriptRef_Clear(&bio->triggerFunction);
    Minigame_Destruct(bio);
}

// 0x6E6750 (virtual +0x10): the game starts: the timer's colours at 30 / 15, the cursor
// in the middle, the tray to the right; the animal's textures and set-up, the font style,
// the sounds ("BioClass"), the info popup; the animal's most points and its reset; the
// radar hidden and the camera's field of view at 1.6
void __fastcall BioStart(ClassBiology* bio, re::Edx) {
    bio->warnTime = 30.0f;
    bio->hud = 0;
    bio->triggerTime = 15.0f;
    bio->pulseTick = 0;
    bio->pulseStep = 1;
    bio->scorePercentage = 100.0f;
    bio->maxScore = 0;
    bio->cutSound = nullptr;
    bio->unknown13C = 1.0f;
    bio->score = 0;
    bio->unknown120 = 0;
    bio->unknown140 = 4.0f;
    bio->padDir = 4;
    bio->cursorImage = 0;
    bio->unknown10C = 0;
    bio->cutCount = 0;
    bio->messageTime = 0.0f;
    bio->cutting = 0;
    bio->cursorX = 0.0f;
    bio->organCount = 0;
    bio->cursorY = 0.0f;
    bio->organHeld = 0;
    bio->trayAtDrop = 0;
    bio->padDelay = 0;
    bio->organTurned[0] = 0;
    bio->organTurned[1] = 0;
    bio->organTurned[2] = 0;
    bio->organTurned[3] = 0;
    bio->trayX = 430.0f;
    bio->trayY = 200.0f;
    Bio_Setup(bio);
    PushTxdMemoryTag();
    FontStyle_Load(6);
    PopMemoryTag();
    const BioSoundEntry sounds[9] = {
        {0, GameString(0x00944DD4)},   // SELECT_TOOL
        {1, GameString(0x00944DD0)},   // PIN
        {2, GameString(0x00944DC8)},   // SCALPEL
        {3, GameString(0x00944DC0)},   // TWEEZE
        {4, GameString(0x00944DB8)},   // DROP
        {5, GameString(0x00944DB0)},   // LIFT
        {6, GameString(0x00944DA8)},   // ERROR
        {7, GameString(0x0094448C)},   // TimeWarning
        {8, GameString(0x00944D9C)},   // IDENTIFY
    };
    Minigame_LoadSounds(bio, 9, sounds, GameString(0x00944DE0));   // "BioClass"
    Popup_Show(&bio->popupKind, 2, GameString(0x00944CD8), GameString(0x00944D90), 7, -1);   // "MGBI_INFO"
    int animal = bio->animal;
    static constexpr int kMaxScore[8] = {0xF, 0x12, 0xF, 0x11, 0x18, 0xF, 0x12, 0x11};
    if (uint32_t(animal) < 8) {
        bio->maxScore = kMaxScore[animal];
        bio->score = 0;
    }
    bio->messageTime = 0.0f;
    if (uint32_t(animal) < 8) {
        AnimalFn(kBioReset[animal])(bio);
    }
    bio->active = 1;
    bioRadarHidden = 0;
    Camera_SetFov(bioCamera, 1.6f);
}

// a stick past its 0.05 dead zone, while the pad's repeat delay has run out
inline bool StickPast(int control, float limit, bool negative, int delay) {
    float v = Pad_GetValue(control, 0);
    return (negative ? v < limit : v > limit) && delay == 0;
}

// 0x6E6A30: the pad: a direction (repeating after 300 of the delay) for turning organs;
// the action button (held while cutting or holding an organ) picks a tool and works it
// (the animal's start function), released it wipes the unfinished cuts and puts the
// organs back; the exit and info popups
void __fastcall BioHandleInput(ClassBiology* bio, re::Edx) {
    bool set = true;
    if ((Pad_GetValue(1, 0) != 0.0f && bio->padDelay == 0) || Pad_JustDown(1, 0) != 0 ||
        StickPast(0x10, -0.05f, true, bio->padDelay) || StickPast(0x12, -0.05f, true, bio->padDelay)) {
        bio->padDir = 3;
    } else if ((Pad_GetValue(0, 0) != 0.0f && bio->padDelay == 0) || Pad_JustDown(0, 0) != 0 ||
               StickPast(0x10, 0.05f, false, bio->padDelay) || StickPast(0x12, 0.05f, false, bio->padDelay)) {
        bio->padDir = 2;
    } else if (Pad_JustDown(2, 0) != 0 || StickPast(0x11, 0.05f, false, bio->padDelay) ||
               StickPast(0x13, 0.05f, false, bio->padDelay)) {
        bio->padDir = 0;
    } else if (Pad_JustDown(3, 0) != 0 || StickPast(0x11, -0.05f, true, bio->padDelay) ||
               StickPast(0x13, -0.05f, true, bio->padDelay)) {
        bio->padDir = 1;
    } else {
        bio->padDir = 4;
        set = false;
    }
    if (set) {
        bio->padDelay = 300;
    }
    bool act = false;
    if ((bio->cutting == 1 || bio->organHeld == 1) &&
        (Pad_GetValue(7, 0) != 0.0f || Pad_GetValue(0xC, 0) != 0.0f)) {
        act = true;
    } else if (Pad_JustDown(7, 0) != 0 || Pad_JustDown(0xC, 0) != 0) {
        Bio_PickTool(bio);
        act = true;
    } else {
        Minigame_EndSequence(0);
        bio->cutEffect = 0;
        StopBioSound(bio->cutSound);
        for (int c = 0; c < bio->cutCount; ++c) {
            Bio_ClearCut(bio, c);
        }
        bio->organHeld = 0;
        for (int i = 0; i < bio->organCount; ++i) {
            if (bio->organRemoved[i] == 0) {
                bio->markerVisible[bio->organMarkers[i][0]] = 1;
                if (bio->organFrames[i] > 1) {
                    for (int k = 1; k < bio->organFrames[i]; ++k) {
                        bio->markerVisible[bio->organMarkers[i][k]] = 0;
                    }
                    bio->organTurned[i] = 0;
                }
            }
            bio->organGrabbed[i] = 0;
        }
    }
    if (act) {
        Bio_StartAnimal(bio);
    }
    if (Pad_JustDown(8, 0) != 0) {
        Popup_Show(&bio->popupKind, 1, GameString(0x00944690), GameString(0x00941D14), 7, 8);   // "EXIT_QUESTION"
        return;
    }
    if (Pad_JustDown(6, 0) != 0) {
        Popup_Show(&bio->popupKind, 2, GameString(0x00944CD8), GameString(0x00944D90), 7, -1);   // "MGBI_INFO"
        return;
    }
    if (bio->padDelay != 0) {
        float step = bioTimerRate * bioTimeStep;
        int left = int(float(bio->padDelay) - step * 1000.0f);
        bio->padDelay = left;
        if (left < 0) {
            bio->padDelay = 0;
        }
    }
}

// 0x6E6E10: the tray eases (0.3 to the power of the time step each frame) towards its
// place: aside at (430, 200), or at (220, 80) to drop on; returning, it switches once
// within 50 of the side
void __fastcall BioUpdateTray(ClassBiology* bio, re::Edx) {
    uint8_t returning = bio->trayReturning;
    if (returning != 0) {
        bio->trayAtDrop = 0;
    }
    float targetX;
    float targetY;
    if (bio->trayAtDrop == 1) {
        targetX = 220.0f;
        targetY = 80.0f;
    } else {
        targetX = 430.0f;
        targetY = 200.0f;
        if (returning != 0 && Distance(430.0f - bio->trayX, 200.0f - bio->trayY) < 50.0f) {
            bio->trayReturning = 0;
            bio->trayAtDrop = 1;
        }
    }
    float f = trk::CrtPow(0.3f, bioTimeStep);
    float dx = targetX - bio->trayX;
    float dy = targetY - bio->trayY;
    dx = dx * f;
    dy = f * dy;
    bio->trayX = bio->trayX + dx;
    bio->trayY = bio->trayY + dy;
}

// 0x6E6F50: the cursor moves (9 a frame, 4 with the slow button), kept within the screen
// less 40 across and 190 up and down
void __fastcall BioMoveCursor(ClassBiology* bio, re::Edx) {
    float pad[2];
    Bio_ReadPad(bio, pad);
    if (0.0f == pad[0] && 0.0f == pad[1]) {
        return;
    }
    float speed = Pad_GetValue(0xD, 0) != 0.0f ? 4.0f : 9.0f;
    float mx = pad[0] * speed;
    float my = speed * pad[1];
    bio->cursorX = bio->cursorX + mx;
    bio->cursorY = bio->cursorY + my;
    float limit = 320.0f / bioScreenScaleX - 40.0f;
    float lower = -limit;
    if (lower >= bio->cursorX) {
        bio->cursorX = lower;
    } else if (bio->cursorX >= limit) {
        bio->cursorX = limit;
    }
    if (-190.0f >= bio->cursorY) {
        bio->cursorY = -190.0f;
    } else if (190.0f <= bio->cursorY) {
        bio->cursorY = 190.0f;
    }
}

// 0x6E7080: the cut's trail: started at the cut point; a point added each time the
// cursor is 2 away from the last (0 once 300 are used: the trail is reset)
uint8_t __fastcall BioAddTrail(ClassBiology* bio, re::Edx, int cut) {
    int count = bio->trailCount;
    if (count <= -1) {
        bio->trailCount = 0;
        bio->trail[0] = bio->points[cut][bio->cutPoint[cut]];
        return 1;
    }
    BioPoint last = bio->trail[count];
    if (Distance(last.x - bio->cursorX, last.y - bio->cursorY) > 2.0f) {
        int next = count + 1;
        bio->trailCount = next;
        if (next >= 300) {
            bio->trailCount = -1;
            return 0;
        }
        bio->trail[next].x = bio->cursorX;
        bio->trail[next].y = bio->cursorY;
        return 1;
    }
    return 1;
}

// 0x6E71B0: the pulse (0..8 and back, a step every third frame), then the marks, the
// incisions, the tray and the organs
void __fastcall BioDrawWorld(ClassBiology* bio, re::Edx) {
    bio->pulseTick += 1;
    if (bio->pulseTick > 2) {
        bio->pulse += bio->pulseStep;
        if (bio->pulse == 8) {
            bio->pulseStep = -1;
        }
        if (bio->pulse == 0) {
            bio->pulseStep = 1;
        }
        bio->pulseTick = 0;
    }
    Bio_DrawMarks(bio);
    Bio_DrawCuts(bio);
    Bio_DrawTray(bio);
    Bio_DrawOrgans(bio);
}

// 0x6E7220 (virtual +0): the deleting destructor
ClassBiology* __fastcall BioDelete(ClassBiology* bio, re::Edx, int flags) {
    Bio_Destruct(bio);
    if ((flags & 1) != 0) {
        Free(bio);
    }
    return bio;
}

GAME_VAR(void*, bioMusicChannel, 0x00CF4288);

// 0x6E7240 (virtual +4): the popups (the music paused while the exit question shows);
// else (no start response pending) the timer, and while running the input, the tray,
// the cursor and the bonuses
void __fastcall BioUpdate(ClassBiology* bio, re::Edx) {
    if (bio->popupShowing != 0) {
        if (bio->popupKind == 1) {
            if (Channel_IsPlaying(bioMusicChannel) != 0) {
                Channel_Pause(bioMusicChannel);
            }
            if (Popup_Result(&bio->popupKind) == 2) {
                bio->active = 0;
            }
            return;
        }
        if (bio->popupShowing != 0 && bio->popupKind == 2) {
            Popup_Result(&bio->popupKind);
            return;
        }
    }
    if (bioUserStartResponse != 0) {
        return;
    }
    Bio_UpdateTimer(bio);
    if (bio->active == 0) {
        return;
    }
    if (Channel_IsPaused(bioMusicChannel) != 0) {
        Channel_Resume(bioMusicChannel);
    }
    Bio_HandleInput(bio);
    Bio_UpdateTray(bio);
    Bio_MoveCursor(bio);
    Bio_UpdateBonuses(bio);
}

// a cut started at a point
inline void StartCutAt(ClassBiology* bio, int c, int point) {
    bio->cutting = 1;
    bio->segmentShown[c][point] = 1;
    bio->cutPoint[c] = point;
    bio->cutDir[c] = 0;
}

// 0x6E72F0: the scalpel: not cutting, a cut starts at a point within 30 (where a started
// incision can go on; a line's ends; a loop's nearest point not yet shown); cutting, the
// cursor within 10 of the next or previous point cuts that segment (one way per cut:
// the first segment sets it), straying beyond 70 is a bad move, and the trail grows. An
// incision is done once all its segments are cut. 1 once all are (the cut sound and
// effect stopped)
uint8_t __fastcall BioUpdateCut(ClassBiology* bio, re::Edx) {
    if (bio->cutting == 1) {
        for (int c = 0; c < bio->cutCount; ++c) {
            int count = bio->pointCount[c];
            int cut = 0;
            for (int i = 0; i < count; ++i) {
                if (bio->segmentCut[c][i] == 1) {
                    ++cut;
                }
            }
            BioPoint here = bio->points[c][bio->cutPoint[c]];
            if (Distance(bio->cursorX - here.x, bio->cursorY - here.y) < 10.0f && bio->cutEffect != 0) {
                float pos[2] = {bioScreenScaleX * bio->cursorX, bio->cursorY};
                Effect_Move(bio->cutEffect, pos);
            }
            if (bio->cutDone[c] == 0) {
                bool full = bio->cutLoop[c] == 0 ? bio->pointCount[c] - cut == 2 : bio->pointCount[c] - cut == 1;
                if (!full) {
                    int candidates[2] = {Bio_FirstCuttable(bio, c), Bio_LastCuttable(bio, c)};
                    for (int k = 0; k < 2; ++k) {
                        int p = candidates[k];
                        if (p != -1 &&
                            Distance(bio->cursorX - bio->points[c][p].x, bio->cursorY - bio->points[c][p].y) <= 30.0f) {
                            StartCutAt(bio, c, p);
                        }
                    }
                }
                int point = bio->cutPoint[c];
                int last = bio->pointCount[c] - 1;
                int next = point == last ? 0 : point + 1;
                int prev = point == 0 ? last : point - 1;
                BioPoint nextP = bio->points[c][next];
                BioPoint prevP = bio->points[c][prev];
                float nextX = bio->cursorX - nextP.x;
                float nextY = bio->cursorY - nextP.y;
                float distancePrev = Distance(bio->cursorX - prevP.x, bio->cursorY - prevP.y);
                float distanceNext = Distance(nextX, nextY);
                if (10.0f > distanceNext) {
                    if (bio->cutSound == nullptr) {
                        bio->cutSound = PlayBioSound(bio, BIO_SOUND_SCALPEL);
                    }
                    float pos[2] = {bioScreenScaleX * nextP.x, nextP.y};
                    if (bio->cutEffect == 0) {
                        bio->cutEffect = Effect_Start(GameString(0x00944DEC), pos, 0, 0);   // "BioCut"
                    }
                    if (bio->segmentCut[c][point] == 0 && (bio->cutDir[c] == 0 || bio->cutDir[c] == 1)) {
                        bio->cutStarted[c] = 1;
                        bio->segmentShown[c][next] = 1;
                        bio->segmentCut[c][point] = 1;
                        bio->cutDir[c] = 1;
                        bio->cutPoint[c] = next;
                        bio->trailCount = -1;
                        bio->badMoveShown = 0;
                        goto finished;
                    }
                } else if (10.0f > distancePrev) {
                    float pos[2] = {bioScreenScaleX * prevP.x, prevP.y};
                    if (bio->cutEffect == 0) {
                        bio->cutEffect = Effect_Start(GameString(0x00944DEC), pos, 0, 0);
                    }
                    if (bio->cutSound == nullptr) {
                        bio->cutSound = PlayBioSound(bio, BIO_SOUND_SCALPEL);
                    }
                    if (bio->segmentCut[c][prev] == 0 && (bio->cutDir[c] == 0 || bio->cutDir[c] == -1)) {
                        bio->cutStarted[c] = 1;
                        bio->segmentShown[c][prev] = 1;
                        bio->segmentCut[c][prev] = 1;
                        bio->cutDir[c] = -1;
                        bio->cutPoint[c] = prev;
                        bio->trailCount = -1;
                        bio->badMoveShown = 0;
                        goto finished;
                    }
                }
                BioPoint at = bio->points[c][bio->cutPoint[c]];
                if (Distance(bio->cursorX - at.x, bio->cursorY - at.y) > 70.0f) {
                    StopBioSound(bio->cutSound);
                    if (bio->badMoveShown == 0) {
                        bio->badMove = 1;
                        bio->badMoveShown = 1;
                    }
                } else if (Bio_AddTrail(bio, c) == 0) {
                    bio->cutDir[c] = 0;
                    for (int i = 0; i < bio->pointCount[c]; ++i) {
                        bio->segmentShown[c][i] = 0;
                    }
                    bio->trailCount = -1;
                }
            }
            if (bio->cutLoop[c] == 0) {
                if (cut == bio->pointCount[c] - 1) {
                    bio->cutDone[c] = 1;
                    bio->bonusLeft[c] = 0.0f;
                    bio->cutting = 0;
                }
            } else if (bio->cutLoop[c] == 1 && cut == bio->pointCount[c]) {
                bio->cutDone[c] = 1;
                bio->bonusLeft[c] = 0.0f;
                bio->cutting = 0;
            }
        }
    } else {
        for (int c = 0; c < bio->cutCount; ++c) {
            if (bio->cutDone[c] != 0) {
                continue;
            }
            if (bio->cutStarted[c] == 1) {
                int candidates[2] = {Bio_FirstCuttable(bio, c), Bio_LastCuttable(bio, c)};
                for (int k = 0; k < 2; ++k) {
                    int p = candidates[k];
                    if (p != -1 &&
                        Distance(bio->cursorX - bio->points[c][p].x, bio->cursorY - bio->points[c][p].y) <= 30.0f) {
                        StartCutAt(bio, c, p);
                    }
                }
            } else if (bio->cutLoop[c] == 0) {
                int ends[2] = {0, bio->pointCount[c] - 1};
                for (int k = 0; k < 2; ++k) {
                    const BioPoint& p = bio->points[c][ends[k]];
                    if (Distance(bio->cursorX - p.x, bio->cursorY - p.y) <= 30.0f) {
                        StartCutAt(bio, c, ends[k]);
                        break;
                    }
                }
            } else if (bio->cutLoop[c] == 1) {
                float best = 100000.0f;
                int bestPoint = -1;
                for (int i = 0; i < bio->pointCount[c]; ++i) {
                    if (bio->segmentShown[c][i] != 0) {
                        continue;
                    }
                    float d = Distance(bio->cursorX - bio->points[c][i].x, bio->cursorY - bio->points[c][i].y);
                    if (d <= 30.0f && best > d) {
                        best = d;
                        bestPoint = i;
                    }
                }
                if (bestPoint != -1) {
                    StartCutAt(bio, c, bestPoint);
                }
            }
        }
    }
finished:
    int done = 0;
    for (int c = 0; c < bio->cutCount; ++c) {
        if (bio->cutDone[c] == 1) {
            ++done;
        }
    }
    if (done != bio->cutCount) {
        return 0;
    }
    Pad_Rumble(0, 0xFA, 0x18, 1);
    StopBioSound(bio->cutSound);
    Minigame_EndSequence(0);
    bio->cutEffect = 0;
    return 1;
}

// 0x6E7E00 (virtual +8): the backdrop, the markers, the score, the timer, the tools, the
// world, the message, the cursor and a popup (the font's scale 1 for language 7 meanwhile)
void __fastcall BioRender(ClassBiology* bio, re::Edx) {
    if (bio->hud == 0) {
        return;
    }
    float scale = bioFontScale;
    if (bioLanguage == 7) {
        bioFontScale = 1.0f;
    }
    Bio_DrawBackground(bio);
    Bio_DrawMarkers(bio);
    Bio_DrawScore(bio);
    Bio_DrawTimer(bio);
    Bio_DrawToolIcons(bio);
    Bio_DrawWorld(bio);
    Bio_DrawMessage(bio);
    Bio_DrawCursor(bio);
    if (bio->popupShowing != 0) {
        Popup_Draw(&bio->popupKind);
    }
    if (bioLanguage == 7) {
        bioFontScale = scale;
    }
}

// ---- script natives ------------------------------------------------------------------------

GAME_FN(void, __thiscall, Bio_SetTimer, 0x006E40F0, void* self, float time, float second);
GAME_FN(int, __thiscall, Minigame_GetScore, 0x00824EC0, void* self);      // (the game's shared "return 0")
GAME_FN(int, __thiscall, Minigame_GetMaxScore, 0x006F2E70, void* self);   // (a shared "return 1")

// 0x6E7EF0: ClassBiologySetTimer(time, [second])
int __cdecl Native_ClassBiologySetTimer(lua_State* L) {
    Script::GetArgCount(L);
    float time = Script::GetFloat(L, 0);
    float second = 0.0f;
    if (Script::GetArgCount(L) >= 2) {
        second = Script::GetFloat(L, 1);
    }
    Bio_SetTimer(bioCurrentMinigame, time, second);
    return 0;
}

// 0x6E7FD0: ClassBiologyGetScorePercentage (the getters it calls are the shared
// constants 0 and 1: always 0)
int __cdecl Native_ClassBiologyGetScorePercentage(lua_State* L) {
    float score = float(Minigame_GetScore(bioCurrentMinigame));
    int most = Minigame_GetMaxScore(bioCurrentMinigame);
    Script::PushFloat(L, score / float(most) * 100.0f);
    return 1;
}

} // namespace

// Untested: the game's code runs until each is checked.
#define CLASSBIO42_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CLASSBIO42_REVERSED("ClassBiology::StartAnimal", 0x006E37F0, &BioStartAnimal);
CLASSBIO42_REVERSED("ClassBiology::ResetAnimal", 0x006E3850, &BioResetAnimal);
CLASSBIO42_REVERSED("ClassBiology::LoadTextureSets", 0x006E38B0, &BioLoadTextureSets);
CLASSBIO42_REVERSED("ClassBiology::UnloadTextureSets", 0x006E3A70, &BioUnloadTextureSets);
CLASSBIO42_REVERSED("ClassBiology::LoadBioTextures", 0x006E3AF0, &BioLoadBioTextures);
CLASSBIO42_REVERSED("ClassBiology::UnloadBioTextures", 0x006E3D60, &BioUnloadBioTextures);
CLASSBIO42_REVERSED("ClassBiology::ResetCuts", 0x006E3DE0, &BioResetCuts);
CLASSBIO42_REVERSED("ClassBiology::FirstCuttable", 0x006E3E40, &BioFirstCuttable);
CLASSBIO42_REVERSED("ClassBiology::LastCuttable", 0x006E3EE0, &BioLastCuttable);
CLASSBIO42_REVERSED("ClassBiology::TurnOrgan", 0x006E3FA0, &BioTurnOrgan);
CLASSBIO42_REVERSED("ClassBiology::SetAnimal", 0x006E4080, &BioSetAnimal);
CLASSBIO42_REVERSED("ClassBiology::TakeValidOperation", 0x006E40B0, &BioTakeValidOperation);
CLASSBIO42_REVERSED("ClassBiology::TakeInvalidOperation", 0x006E40D0, &BioTakeInvalidOperation);
CLASSBIO42_REVERSED("ClassBiology::SetTimer", 0x006E40F0, &BioSetTimer);
CLASSBIO42_REVERSED("ClassBiology::SubtractTime", 0x006E4110, &BioSubtractTime);
CLASSBIO42_REVERSED("ClassBiology::ClassBiology", 0x006E4140, &BioConstruct);
CLASSBIO42_REVERSED("ClassBiology::End", 0x006E41C0, &BioEnd);
CLASSBIO42_REVERSED("ClassBiology::Setup", 0x006E4200, &BioSetup);
CLASSBIO42_REVERSED("ClassBiology::DrawBackground", 0x006E4280, &BioDrawBackground);
CLASSBIO42_REVERSED("ClassBiology::UpdateTimer", 0x006E4380, &BioUpdateTimer);
CLASSBIO42_REVERSED("ClassBiology::UpdateBonuses", 0x006E44C0, &BioUpdateBonuses);
CLASSBIO42_REVERSED("ClassBiology::ReadPad", 0x006E4540, &BioReadPad);
CLASSBIO42_REVERSED("ClassBiology::PickTool", 0x006E4790, &BioPickTool);
CLASSBIO42_REVERSED("ClassBiology::CheckPins", 0x006E48F0, &BioCheckPins);
CLASSBIO42_REVERSED("ClassBiology::CheckMarks", 0x006E49C0, &BioCheckMarks);
CLASSBIO42_REVERSED("ClassBiology::ClearCut", 0x006E4AD0, &BioClearCut);
CLASSBIO42_REVERSED("ClassBiology::UpdateOrgans", 0x006E4BB0, &BioUpdateOrgans);
CLASSBIO42_REVERSED("ClassBiology::DrawTimer", 0x006E4E10, &BioDrawTimer);
CLASSBIO42_REVERSED("ClassBiology::DrawScore", 0x006E4F30, &BioDrawScore);
CLASSBIO42_REVERSED("ClassBiology::DrawMessage", 0x006E5020, &BioDrawMessage);
CLASSBIO42_REVERSED("ClassBiology::DrawToolIcons", 0x006E51E0, &BioDrawToolIcons);
CLASSBIO42_REVERSED("ClassBiology::DrawMarkers", 0x006E53F0, &BioDrawMarkers);
CLASSBIO42_REVERSED("ClassBiology::DrawMarks", 0x006E5570, &BioDrawMarks);
CLASSBIO42_REVERSED("ClassBiology::DrawCuts", 0x006E56C0, &BioDrawCuts);
CLASSBIO42_REVERSED("ClassBiology::DrawOrgans", 0x006E6060, &BioDrawOrgans);
CLASSBIO42_REVERSED("ClassBiology::DrawTray", 0x006E6320, &BioDrawTray);
CLASSBIO42_REVERSED("ClassBiology::DrawCursor", 0x006E6580, &BioDrawCursor);
CLASSBIO42_REVERSED("ClassBiology::~ClassBiology", 0x006E66F0, &BioDestruct);
CLASSBIO42_REVERSED("ClassBiology::Start", 0x006E6750, &BioStart);
CLASSBIO42_REVERSED("ClassBiology::HandleInput", 0x006E6A30, &BioHandleInput);
CLASSBIO42_REVERSED("ClassBiology::UpdateTray", 0x006E6E10, &BioUpdateTray);
CLASSBIO42_REVERSED("ClassBiology::MoveCursor", 0x006E6F50, &BioMoveCursor);
CLASSBIO42_REVERSED("ClassBiology::AddTrail", 0x006E7080, &BioAddTrail);
CLASSBIO42_REVERSED("ClassBiology::DrawWorld", 0x006E71B0, &BioDrawWorld);
CLASSBIO42_REVERSED("ClassBiology::Delete", 0x006E7220, &BioDelete);
CLASSBIO42_REVERSED("ClassBiology::Update", 0x006E7240, &BioUpdate);
CLASSBIO42_REVERSED("ClassBiology::UpdateCut", 0x006E72F0, &BioUpdateCut);
CLASSBIO42_REVERSED("ClassBiology::Render", 0x006E7E00, &BioRender);
CLASSBIO42_REVERSED("Native_ClassBiologySetTimer", 0x006E7EF0, &Native_ClassBiologySetTimer);
CLASSBIO42_REVERSED("Native_ClassBiologyGetScorePercentage", 0x006E7FD0, &Native_ClassBiologyGetScorePercentage);
