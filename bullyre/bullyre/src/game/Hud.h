#pragma once

// The HUD: HUDComponent and its kinds, one object per element of the HUD layout
// (made by type by CreateHUDComponent, set up from the layout's record, drawn every
// frame through Render). Bully's own; nothing of GTA's CHud is left in it.
//
// The virtual functions (every kind has the same 13; vtable slots):
//   0 deleting destructor        7 Reset
//   1 Setup(record)              8 Draw (the shown state)
//   2 (empty, one argument)      9 DrawFadeIn (state 0; the base: straight to shown)
//   3 IsType(short id)          10 DrawFadeOut (state 2; the same)
//   4 IsVisible                 11 OnShow (becoming visible; empty in the base)
//   5 SetVisible(flag)          12 OnHide
//   6 SetDrawState(state)
// Registered functions are thiscall; the ones here are written as __fastcall
// (self, edx, ...) where one C++ function serves several classes.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

struct CRGBA {
    uint8_t r, g, b, a;
};
static_assert(sizeof(CRGBA) == 4);

// A HUD layout record: what Setup gets (its data at +4, a different layout for
// each kind: shorts for positions in the 640x480 screen, CRGBAs, alignment names)
struct HUDLayoutRecord {
    uint32_t unknown00;
    re::Ptr32<const uint8_t> data;  // +4
};

enum eHUDDrawState : int32_t {
    HUD_DRAW_FADING_IN = 0,
    HUD_DRAW_SHOWN = 1,
    HUD_DRAW_FADING_OUT = 2,
};

struct HUDComponent {
    re::Ptr32<re::Ptr32<void>> vtable;          // +0x00
    int32_t drawState;      // +0x04 eHUDDrawState: which of the Draw functions Render calls
    uint8_t visible;        // +0x08
    uint8_t unknown09[3];
    int32_t hAlign;         // +0x0C 0 left, 1 centre, 2 right (the layout's names, sub_5DAF00)
    int32_t vAlign;         // +0x10 0 top, 1 centre, 2 bottom
    uint8_t flashing;       // +0x14 (Render: drawn every other 250 ms, for a second)
    uint8_t suppressed;     // +0x15 not drawn while set
    uint8_t unknown16[2];
    uint32_t flashStart;    // +0x18 the timer when the flashing started
    float unknown1C;        // +0x1C (1.0 when made; nothing in the HUD reads it)
};
static_assert(sizeof(HUDComponent) == 0x20);
static_assert(offsetof(HUDComponent, hAlign) == 0xC);
static_assert(offsetof(HUDComponent, flashStart) == 0x18);

// The HUD's kinds, by the type the layout gives (CreateHUDComponent's switch, and
// the names GetHUDComponentTypeName returns)
enum eHUDComponentType {
    HUD_DETENTION = 0,
    HUD_BIG_TEXT_FOR_SCRIPT = 1,
    HUD_SMALL_TEXT_FOR_SCRIPT = 2,
    HUD_MISSION_TIMER = 3,
    HUD_HEALTH_METER = 4,
    HUD_ENTITY_HEALTH = 5,
    HUD_TIME_CYCLE = 6,
    HUD_NEWSPAPER = 7,
    HUD_GENERIC_COUNTER = 8,
    HUD_GENERIC_TEXT = 9,
    HUD_RACING = 10,
    HUD_RADAR = 11,
    HUD_SECONDARY_BAR = 12,
    HUD_MONEY = 13,
    HUD_CLOTHING = 14,
    HUD_CLOCK_WARNINGS = 15,
    HUD_PHOTOGRAPHY = 16,
    HUD_STRUGGLE_BUTTON = 17,
    HUD_BARBER = 18,
    HUD_TATTOO = 19,
    HUD_WEAPONS = 20,
    HUD_BUTTON_HISTORY = 21,
    HUD_DODGEBALL = 22,
    HUD_SOCIAL_APOLOGIZE = 23,   // .. 36: the social menu's actions
    HUD_SOCIAL_BURN = 36,
    HUD_SOCIAL = 37,
    HUD_TAGGING_METER = 38,
    HUD_MONITOR = 39,
    HUD_TUTORIAL = 40,
    HUD_2D_SIGNS = 41,
    HUD_TEXT = 42,
    HUD_SCREEN_FADE = 43,
    HUD_DIARY = 44,
    HUD_INVENTORY = 45,
    HUD_SOCIAL_ERRANDS = 46,
    HUD_TASKS = 47,
    HUD_NUM_COMPONENT_TYPES = 48,
};

// ---- the kinds (only what's understood; sizes from CreateHUDComponent) -------------

// Script text params (Big/SmallText_ForScript's and Text's)
struct HUDTextParam {
    int32_t type;             // +0 1: an int, 2: a float, 3: a string, 4: 16-bit text
    uint8_t value[0x100];     // +4
};
static_assert(sizeof(HUDTextParam) == 0x104);

// Big/SmallText_ForScript: a line of script text for a while, with up to 3 numbers
// or strings in it
struct HUDScriptText : HUDComponent {
    float x, y;               // +0x20 (from the layout)
    CRGBA colour;             // +0x28
    int16_t font;             // +0x2C
    uint8_t unknown2E[2];
    float endTime;            // +0x30 seconds (timer / 1000) when it goes
    char text[0x80];          // +0x34
    int32_t numParams;        // +0xB4 (up to 3)
    HUDTextParam params[3];   // +0xB8
    uint8_t shown;            // +0x3C4 (set while one is up)
    uint8_t unknown3C5[3];
};
static_assert(offsetof(HUDScriptText, numParams) == 0xB4);
static_assert(offsetof(HUDScriptText, shown) == 0x3C4);
static_assert(sizeof(HUDScriptText) == 0x3C8);

struct HUDDetention : HUDComponent {
    float x, y;               // +0x20
    float width, height;      // +0x28
    float angleFrom, angleTo; // +0x30 radians (the layout has degrees)
    int32_t unknown38;        // +0x38
    CRGBA colours[2];         // +0x3C
    float textEndTime;        // +0x44 seconds
    char text[0x20];          // +0x48
    int32_t value;            // +0x68
    float unknown6C;          // +0x6C
    float unknown70;          // +0x70 (1.0)
};
static_assert(sizeof(HUDDetention) == 0x74);

// Health_Meter: the same layout as Detention's for its first part
struct HUDHealthMeter : HUDComponent {
    float x, y;               // +0x20
    float width, height;      // +0x28
    float angleFrom, angleTo; // +0x30
    int32_t unknown38;        // +0x38
    CRGBA colours[2];         // +0x3C
    float unknown44;          // +0x44
    float unknown48;          // +0x48
    float unknown4C;          // +0x4C
};
static_assert(sizeof(HUDHealthMeter) == 0x50);

struct HUDEntityHealth : HUDComponent {
    uint8_t unknown20;        // +0x20
    uint8_t unknown21[3];
    uint32_t unknown24;       // +0x24 (the layout's raw words)
    uint32_t unknown28;       // +0x28
    uint32_t unknown2C;       // +0x2C
    uint32_t unknown30;       // +0x30
    uint32_t unknown34;       // +0x34
    int32_t unknown38;        // +0x38
    CRGBA colours[2];         // +0x3C
};
static_assert(sizeof(HUDEntityHealth) == 0x44);

struct HUDMissionTimer : HUDComponent {
    col::CRect rect;          // +0x20
    float centreX, centreY;   // +0x30
    int32_t unknown38;        // +0x38
    int32_t unknown3C;        // +0x3C
    CRGBA colour;             // +0x40
    uint8_t unknown44;        // +0x44
    uint8_t unknown45[0x1F];
};
static_assert(sizeof(HUDMissionTimer) == 0x64);

struct HUDNewspaper : HUDComponent {
    col::CRect rect;          // +0x20
    col::CRect rect2;         // +0x30
    int32_t unknown40;        // +0x40
    float x44, y44;           // +0x44
    int32_t unknown4C;        // +0x4C (10)
    int32_t unknown50;        // +0x50
    int32_t unknown54;        // +0x54
    float x58, y58;           // +0x58
    int32_t unknown60;        // +0x60
    uint8_t unknown64;        // +0x64
    uint8_t unknown65;        // +0x65
    uint8_t unknown66[2];
};
static_assert(sizeof(HUDNewspaper) == 0x68);

struct HUDGenericCounter : HUDComponent {
    col::CRect rect;          // +0x20
    float x30, y30;           // +0x30
    col::CRect rect2;         // +0x38
    re::Ptr32<void> icon;               // +0x48 a texture (Gamebryo, refcounted)
    int32_t max;              // +0x4C (10)
    int32_t value;            // +0x50
    int32_t previousValue;    // +0x54
    int32_t previousMax;      // +0x58
    CRGBA colour;             // +0x5C
    uint8_t unknown60;        // +0x60
    uint8_t unknown61;        // +0x61
    uint8_t unknown62;        // +0x62
    uint8_t unknown63;        // +0x63
    uint8_t unknown64[0x40];
    float unknownA4;          // +0xA4
    float unknownA8;          // +0xA8
    uint8_t unknownAC[0x250];
};
static_assert(offsetof(HUDGenericCounter, colour) == 0x5C);
static_assert(sizeof(HUDGenericCounter) == 0x2FC);

// Generic_Text: up to 5 lines, each for a while
struct HUDGenericText : HUDComponent {
    float x, y;               // +0x20
    CRGBA colour;             // +0x28
    int32_t unknown2C;        // +0x2C
    int32_t unknown30;        // +0x30
    struct Line {             // (0x208 bytes)
        uint16_t text[0x100]; // +0 (the game's 16-bit text; empty: no line)
        uint32_t startTime;   // +0x200
        int32_t id;           // +0x204 -1: none
    } lines[5];               // +0x34
};
static_assert(sizeof(HUDGenericText::Line) == 0x208);
static_assert(sizeof(HUDGenericText) == 0xA5C);

struct HUDRacing : HUDComponent {
    float x, y;               // +0x20
    float x28, y28;           // +0x28
    float x30, y30;           // +0x30
    CRGBA colours[2];         // +0x38
    col::CRect rect;          // +0x40
    int32_t unknown50[5];     // +0x50
    int32_t unknown64;        // +0x64
    uint32_t unknown68;       // +0x68 a time
    int32_t unknown6C;        // +0x6C
    int32_t unknown70;        // +0x70
    uint32_t unknown74;       // +0x74 a time
};
static_assert(offsetof(HUDRacing, unknown68) == 0x68);
static_assert(sizeof(HUDRacing) == 0x78);

struct HUDMoney : HUDComponent {
    int32_t x, y;             // +0x20 (whole pixels)
    CRGBA colour;             // +0x28
    float countSeconds;       // +0x2C how long the count up/down to a new amount takes
    uint8_t unknown30;        // +0x30
    uint8_t unknown31[3];
    uint32_t countEndTime;    // +0x34
    float shownMoney;         // +0x38 what's shown, counting towards the player's
    int32_t money;            // +0x3C the player's, when last looked
    float countFrom;          // +0x40 the difference being counted
    int32_t unknown44;        // +0x44
};
static_assert(sizeof(HUDMoney) == 0x48);

struct HUDClockWarnings : HUDComponent {
    col::CRect rect;          // +0x20
    col::CRect rect2;         // +0x30
    float centreX, centreY;   // +0x40
    int32_t colourDelta[3];   // +0x48 colours[1] - colours[0], per channel
    int32_t unknown54;        // +0x54
    int32_t unknown58;        // +0x58
    CRGBA colours[2];         // +0x5C
};
static_assert(sizeof(HUDClockWarnings) == 0x64);

struct HUDStruggleButton : HUDComponent {
    int32_t button;           // +0x20 (7)
    col::CRect rect;          // +0x24
    col::CRect rect2;         // +0x34
    char name[0x108];         // +0x44 ("NONE")
    col::CRect rect3;         // +0x14C
    int32_t unknown15C;       // +0x15C
    uint8_t unknown160;       // +0x160
    uint8_t unknown161;       // +0x161
    uint8_t unknown162;       // +0x162
    uint8_t unknown163;
    float unknown164;         // +0x164
};
static_assert(offsetof(HUDStruggleButton, rect3) == 0x14C);
static_assert(sizeof(HUDStruggleButton) == 0x168);

struct HUDTutorial : HUDComponent {
    uint8_t unknown20;
    uint8_t unknown21;        // +0x21
    uint8_t unknown22[0x12];
    int32_t unknown34;        // +0x34
    int32_t unknown38;        // +0x38 (-1)
    uint8_t unknown3C;        // +0x3C
    uint8_t unknown3D[3];
    col::CRect rect;          // +0x40
};
static_assert(sizeof(HUDTutorial) == 0x50);

struct HUDDiary : HUDComponent {
    float x, y;               // +0x20
    float x28, y28;           // +0x28
    float x30, y30;           // +0x30
    float width, height;      // +0x38
    re::Ptr32<void> texture;            // +0x40 Bea_diary (Gamebryo, refcounted)
    int32_t txd;              // +0x44 its dictionary's slot, -1: none
};
static_assert(sizeof(HUDDiary) == 0x48);

struct HUDInventory : HUDComponent {
    struct Entry {            // (0x18 bytes, a ring of 5 from +0x20)
        int32_t state;        // +0
        int32_t item;         // +4
        re::Ptr32<void> info;           // +8
        int32_t from, to;     // +0xC
        float time;           // +0x14
    } entries[5];             // +0x20
    int32_t count;            // +0x98
    int32_t last;             // +0x9C
    int32_t first;            // +0xA0
};
static_assert(offsetof(HUDInventory, count) == 0x98);
static_assert(sizeof(HUDInventory) == 0xA4);

struct HUDTasks : HUDComponent {
    int32_t blipType;         // +0x20 1: +0x28 is a blip's, 2: +0x30 is
    float unknown24;          // +0x24 (-1)
    int32_t unknown28[5];     // +0x28 (-1)
};
static_assert(sizeof(HUDTasks) == 0x3C);

struct HUDScreenFade : HUDComponent {
    int32_t unknown20;
    int32_t unknown24;
    uint8_t unknown28[4];
};
static_assert(sizeof(HUDScreenFade) == 0x2C);

// Text: 8 lines of script text, each with up to 3 params
struct HUDText : HUDComponent {
    struct Slot {             // (0x390 bytes)
        int32_t numParams;    // +0
        HUDTextParam params[3];  // +4
        char text[0x80];      // +0x310 empty: free
    } slots[8];               // +0x20
};
static_assert(sizeof(HUDText::Slot) == 0x390);
static_assert(sizeof(HUDText) == 0x1CA0);

struct HUDMonitor : HUDComponent {
    re::Ptr32<void> texture;            // +0x20 "monitor"
    uint8_t loaded;           // +0x24 its texture and the noise are made
    uint8_t unknown25[3];
    int32_t unknown28;        // +0x28 (from a name, 0x6A7110)
    struct Line {             // (0x1C bytes, 5 of them; the layout places the first 4)
        float x, y;           // +0
        char text[0x11];      // +8
        uint8_t unknown19;    // +0x19
        uint8_t unknown1A[2];
    } lines[5];               // +0x2C
};
static_assert(sizeof(HUDMonitor::Line) == 0x1C);
static_assert(sizeof(HUDMonitor) == 0xB8);

// What the social menu's components share (Setup 0x555610, the tagging meter's too)
struct HUDSocialBase : HUDComponent {
    float x, y;               // +0x20 (scaled, not aligned)
    float iconX, iconY;       // +0x28
    float width, height;      // +0x30 (scaled)
    float unknown38;          // +0x38 (y - height) / 2
    float unknown3C;          // +0x3C (x - width) / 2
};
static_assert(sizeof(HUDSocialBase) == 0x40);

struct HUDSocial : HUDSocialBase {
    uint8_t unknown40;        // +0x40
    uint8_t unknown41[0xB];
    uint32_t unknown4C[14];   // +0x4C (a copy of 0xC674FC)
};
static_assert(sizeof(HUDSocial) == 0x84);

struct HUDSocialBurn : HUDComponent {
    float x, y;               // +0x20
    float iconX, iconY;       // +0x28
    float width, height;      // +0x30
    int16_t unknown38;        // +0x38 (y - height) / 2
    int16_t unknown3A;        // +0x3A (x - width) / 2
};
static_assert(sizeof(HUDSocialBurn) == 0x3C);

struct HUDSocialErrands : HUDComponent {
    float x, y;               // +0x20
    float x28, y28;           // +0x28
    float width, height;      // +0x30 (scaled)
    CRGBA colours[2];         // +0x38
    uint8_t unknown40[8];
    uint8_t unknown48, unknown49, unknown4A, unknown4B;
    float unknown4C[4];       // +0x4C
    uint8_t unknown5C;        // +0x5C
    uint8_t unknown5D[0x1F];
};
static_assert(sizeof(HUDSocialErrands) == 0x7C);

struct HUDSecondaryHealthBar : HUDComponent {
    uint8_t unknown20[0x20];
    col::CRect rect;          // +0x40
    col::CRect rect2;         // +0x50
    uint8_t unknown60[0x4C];
    re::Ptr32<void> unknownAC;          // +0xAC
    re::Ptr32<void> unknownB0;          // +0xB0
    re::Ptr32<void> unknownB4;          // +0xB4
    uint8_t unknownB8[0xC8];
};
static_assert(sizeof(HUDSecondaryHealthBar) == 0x180);

// 2DSigns: the full-screen signs (bulletin board, fortune teller, globe), each a
// small object with its own vtable
struct HUD2DSigns : HUDComponent {
    int32_t unknown20;        // +0x20
    float unknown24[3];       // +0x24
    int32_t txd;              // +0x30 -1: none
    re::Ptr32<void> texture;            // +0x34 (Gamebryo, refcounted)
    uint8_t unknown38[8];
    float unknown40;          // +0x40
    re::Ptr32<void> bulletinBoard;      // +0x44 vtable 0x919000 (an object of 0xB4 bytes)
    uint8_t unknown48[0xB0];
    re::Ptr32<void> fortuneTeller;      // +0xF8 vtable 0x919010
    uint8_t unknownFC[8];
    int32_t unknown104;       // +0x104
    re::Ptr32<void> globe;              // +0x108 vtable 0x919020
    uint8_t unknown10C[8];
    int32_t unknown114;       // +0x114
    re::Ptr32<void> current;            // +0x118 the sign being shown
};
static_assert(offsetof(HUD2DSigns, fortuneTeller) == 0xF8);
static_assert(sizeof(HUD2DSigns) == 0x11C);

struct HUDWeapons : HUDComponent {
    uint8_t unknown20[0x4C];
    int32_t unknown6C;        // +0x6C
    uint8_t unknown70[0x4C];
    int32_t unknownBC;        // +0xBC
    uint8_t unknownC0[0x6C];
    uint8_t unknown12C;       // +0x12C
    uint8_t unknown12D;       // +0x12D
    uint8_t unknown12E[0x6E];
};
static_assert(sizeof(HUDWeapons) == 0x19C);

struct HUDPhoto : HUDComponent {
    uint8_t unknown20;        // +0x20
    uint8_t unknown21;        // +0x21
    uint8_t unknown22[2];
    int32_t unknown24;        // +0x24
    uint8_t unknown28[4];
    int32_t unknown2C;        // +0x2C
    uint8_t unknown30[6];
    uint8_t unknown36, unknown37, unknown38, unknown39;  // +0x36
    uint8_t unknown3A[2];
    float unknown3C;          // +0x3C (1.0)
    uint8_t unknown40;        // +0x40
    uint8_t unknown41[3];
    uint32_t unknown44;       // +0x44 a time
    int32_t unknown48;        // +0x48
    int32_t unknown4C;        // +0x4C
    uint8_t unknown50;        // +0x50
    uint8_t unknown51[3];
    int32_t unknown54;        // +0x54
    uint32_t nameHash58;      // +0x58
    uint32_t nameHash5C;      // +0x5C
    uint8_t unknown60[0x1C];
    int32_t unknown7C;        // +0x7C
    int32_t unknown80;        // +0x80
    uint8_t unknown84[4];
};
static_assert(sizeof(HUDPhoto) == 0x88);

struct HUDButtonHistory : HUDComponent {
    uint8_t unknown20[0x14];
    int32_t capacity;         // +0x34 (9: the three arrays' entries, 0x18 bytes each)
    uint8_t unknown38[0xC];
    struct Buffer {           // (0x14 bytes, 2 of them)
        int32_t count;        // +0
        float time;           // +4
        re::Ptr32<void> data;           // +8 new'd
        uint32_t unknown0C;
        int32_t index;        // +0x10
    } buffers[2];             // +0x44
    uint8_t unknown6C[4];
    re::Ptr32<void> lists[3];           // +0x70 (objects with a destructor, 0x5D9C50)
    uint8_t unknown7C[8];
    re::Ptr32<void> unknown84;          // +0x84 new'd
    uint8_t unknown88[4];
    re::Ptr32<void> unknown8C;          // +0x8C new'd
    uint8_t unknown90[4];
    re::Ptr32<void> unknown94;          // +0x94 new'd
    uint8_t unknown98[0x18];
    uint8_t unknownB0;        // +0xB0
    uint8_t unknownB1[3];
    int32_t unknownB4;        // +0xB4 (-1)
    int32_t unknownB8;        // +0xB8
    int32_t unknownBC;        // +0xBC
    uint8_t unknownC0[0xC];
    uint8_t unknownCC;        // +0xCC
    uint8_t unknownCD[0x1F];
};
static_assert(offsetof(HUDButtonHistory, lists) == 0x70);
static_assert(offsetof(HUDButtonHistory, unknownCC) == 0xCC);
static_assert(sizeof(HUDButtonHistory) == 0xEC);

// The radar's blips: 300, a handle is (generation << 12) | index
struct HUDRadarBlip {         // (0x30 bytes)
    int32_t entity;           // +0x00 (a pool handle, by the type)
    float x, y;               // +0x04
    uint8_t unknown0C[8];
    int32_t generation;       // +0x14
    uint32_t flashUntil;      // +0x18 a time
    float unknown1C;          // +0x1C the marker size (SetBlipMarkerSize)
    int32_t type;             // +0x20 (2, 6, 13: ...)
    int32_t unknown24;        // +0x24
    int32_t unknown28;        // +0x28
    int16_t id;               // +0x2C
    uint8_t unknown2E;        // +0x2E
    uint8_t flags;            // +0x2F 2: in use (by id), 4, 8, 0x10
};
static_assert(sizeof(HUDRadarBlip) == 0x30);

struct HUDRadar : HUDComponent {
    uint8_t unknown20[0x1F0];
    HUDRadarBlip blips[300];  // +0x210
    float zoom;               // +0x3A50
    re::Ptr32<int32_t> unknown3A54;     // +0x3A54
    re::Ptr32<int32_t> unknown3A58;     // +0x3A58
    re::Ptr32<int32_t> unknown3A5C;     // +0x3A5C
    re::Ptr32<void> unknown3A60[14];    // +0x3A60 (objects of 4 bytes, 0x52C6D0 destroys)
    re::Ptr32<void> unknown3A98[39];    // +0x3A98 (the same)
    uint8_t unknown3B34[0xAC00];
    int32_t unknownE734;      // +0xE734
    uint8_t unknownE738[0x50];
    int32_t mode;             // +0xE788 (1, 2: ...)
    uint8_t unknownE78C[0x9C];
    int32_t unknownE828;      // +0xE828 (1 in a chapter whose area props say 2)
    uint32_t unknownE82C;     // +0xE82C flags, by 0 .. 38
    int32_t selectedBlip;     // +0xE830 an index, -1: none
    uint32_t selectedUntil;   // +0xE834 a time
    uint8_t unknownE838[0x10];
};
static_assert(offsetof(HUDRadar, zoom) == 0x3A50);
static_assert(offsetof(HUDRadar, unknownE734) == 0xE734);
static_assert(offsetof(HUDRadar, mode) == 0xE788);
static_assert(offsetof(HUDRadar, unknownE82C) == 0xE82C);
static_assert(sizeof(HUDRadar) == 0xE848);

namespace hud {
// 0x53D030: a new component of a type (nullptr for an unknown one)
HUDComponent* __cdecl CreateHUDComponent(uint32_t type);
// 0x53D010
const char* __cdecl GetHUDComponentTypeName(uint32_t type);
} // namespace hud
