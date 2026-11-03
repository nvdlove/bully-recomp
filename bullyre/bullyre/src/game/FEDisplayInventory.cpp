// The frontend's display settings (FEDisplaySettings), exit-to-menu and geography
// screens, and the inventory (FEInventory). 0x6928C0..0x695476.
//
// FEDisplaySettings: +0xC the row (0 resolution, 1 anti-aliasing, 2 vsync, 3 shadows),
//   +0x10 the state (0 browsing, 1 changing the row's value, 2 applied: waiting for the
//   device), +0x14 a direction held, +0x18 [128] resolutions (w, h) and +0x418 their count,
//   +0x41C [16] anti-aliasing levels and +0x45C their count, +0x460/+0x464 the resolution
//   (current / being chosen), +0x468/+0x46C the level, +0x470/+0x474 the shadows,
//   +0x478/+0x479 vsync, +0x47C.. the layout (TextStartX, ValueStartX, ValueDY, the rows'
//   Ys, MessageOffset), +0x49C the value's text.
// FEInventory: +0x14.. the grid (x, y, width, height, the icon's size, per row, rows shown),
//   +0x34 the selected item, +0x38 the group's count, +0x3C the first shown, +0x40 the
//   groups' tabs (FESelectionListWindowT<3>), +0xAD / +0xC0 choosing an item, +0xC4.. the
//   arrows, +0xDC "no items" position, +0xE8 its style, +0x104 the heading's position,
//   +0x110 its style.
// A text style (0x1C bytes): +0 font, +4 justification (3), +8 dx, +0xC dy, +0x10, +0x14
// colour (2), +0x18 flags. A position (0xC): +0 anchor (0..8), +4 x, +8 y.

#include <cmath>

#include "game/FEScreensCommon.h"
#include "reversed.h"

using namespace fescr;

namespace {

constexpr uintptr_t VT_FEDisplaySettings = 0x0093F5E8;
constexpr uintptr_t VT_FEExitGameToMenu = 0x0093F72C;
constexpr uintptr_t VT_FEGeography = 0x0093F788;
constexpr uintptr_t VT_FEInventory = 0x0093F9B4;
constexpr uintptr_t VT_SelListT3 = 0x0093F9AC;

void* const kWindowColour = reinterpret_cast<void*>(0x020C33E4);

// the graphics settings (bullygfx's area at 0x405580..: only called)
struct GraphicsSettings {
    int32_t width;            // +0x00
    int32_t height;           // +0x04
    int32_t antiAlias;        // +0x08
    uint8_t vsync;            // +0x0C
    uint8_t pad0D[3];
    int32_t shadows;          // +0x10
    uint8_t unknown14;        // +0x14
    uint8_t pad15[3];
};
static_assert(sizeof(GraphicsSettings) == 0x18);
GAME_FN(GraphicsSettings*, __cdecl, Graphics_Get, 0x00405BA0, GraphicsSettings* out);
GAME_FN(void, __cdecl, Graphics_Apply, 0x00405600, GraphicsSettings* settings);
GAME_FN(void, __cdecl, Graphics_SetShadows, 0x00405640, int32_t shadows);
GAME_FN(uint8_t, __cdecl, Graphics_Resetting, 0x00405670);
GAME_FN(int32_t, __cdecl, Graphics_VideoMemoryMb, 0x00405730);
GAME_FN(uint8_t*, __cdecl, Graphics_GetModes, 0x00405750);     // [128] (w, h); +0x400 the count
GAME_FN(uint8_t*, __cdecl, Graphics_GetAALevels, 0x00405760);  // [16]; +0x40 the count
GAME_VAR(int32_t, g_shadowsSetting, 0x00BCBC3C);
GAME_ARRAY(uint32_t, g_displayTextHashes, 0x020C2FCC);  // resolution, AA, vsync, shadows, on, off/low, med, high, ...
GAME_VAR(uint32_t, g_hashOn, 0x020C2FDC);
GAME_VAR(uint32_t, g_hashOff, 0x020C2FE0);
GAME_VAR(uint32_t, g_hashShadowNeedsRestart, 0x020C2FF0);
GAME_VAR(uint32_t, g_menuBackground, 0x020C2FC8);
GAME_FN(void, __cdecl, FE_DrawBackdrop, 0x006A2E00);
GAME_FN(void, __cdecl, FE_DrawBackground, 0x006A2FF0, uint32_t which);
GAME_FN(void, __cdecl, FE_DrawAligned, 0x006A64A0, const void* position, const uint16_t* text, const void* style);
// styles: the row being changed (label, value), not being changed, the others, a message
uint8_t* const kStyleChangingLabel = reinterpret_cast<uint8_t*>(0x00BA5FE0);
uint8_t* const kStyleChangingValue = reinterpret_cast<uint8_t*>(0x00BA5FFC);
uint8_t* const kStyleRowLabel = reinterpret_cast<uint8_t*>(0x00BA6018);
uint8_t* const kStyleRowValue = reinterpret_cast<uint8_t*>(0x00BA6034);
uint8_t* const kStyleOtherLabel = reinterpret_cast<uint8_t*>(0x00BA6050);
uint8_t* const kStyleOtherValue = reinterpret_cast<uint8_t*>(0x00BA606C);
uint8_t* const kStyleMessage = reinterpret_cast<uint8_t*>(0x00BA6088);

int WsprintfW(uint16_t* out, const uint16_t* format, int a = 0, int b = 0) {
    // (USER32's through the game's import slot)
    return re::PtrCast<int(__cdecl*)(uint16_t*, const uint16_t*, ...)>(*reinterpret_cast<re::Ptr32<void>*>(0x008FC27C))(
        out, format, a, b);
}

// exit to menu / text drawing
GAME_FN(void, __cdecl, FE_DrawTextKey, 0x006A6380, int x, int y, const char* key, int font, int justify, int, int);
GAME_FN(void, __cdecl, FE_DrawKeyStyled, 0x006A6CC0, int x, int y, const char* key, const void* style);
GAME_FN(void, __cdecl, FE_DrawMessageStyled, 0x006A6D00, const void* position, const char* key, const void* style);
GAME_FN(void, __thiscall, Position_Load, 0x006A4940, void* position, void* archive, const char* name);
GAME_FN(float, __thiscall, Position_GetX, 0x006A2730, const void* position);
GAME_FN(float, __thiscall, Position_GetY, 0x006A2790, const void* position);
GAME_FN(void, __cdecl, FE_ExitToMainMenu, 0x006A4250);
GAME_ARRAY(const char*, g_exitStyleNames, 0x00BA60C4);  // [4]
GAME_VAR(uint8_t, g_menuFlagsByte, 0x0208B78A);
GAME_VAR(float, g_fontWrapX, 0x0208B778);
GAME_VAR(int16_t, g_fontSizeIndex, 0x0208B788);
GAME_ARRAY(float, g_fontLineHeights, 0x00BA4E8C);
GAME_VAR(float, g_fontScaleY, 0x0208B770);
GAME_ARRAY(uint8_t, g_menuDropColour, 0x020C331C);
GAME_VAR(uint8_t, g_exitingToMenu, 0x020C32EC);
GAME_VAR(uint8_t, g_bd0fde, 0x00BD0FDE);
GAME_VAR(uint8_t, g_bd0fdd, 0x00BD0FDD);
GAME_FN(void, __thiscall, Ped_474C10, 0x00474C10, void* ped, int, int);

// the inventory
GAME_FN(void, __thiscall, SelList_Construct, 0x0069C3A0, void* list, void* entries, int count);
GAME_FN(uint8_t*, __cdecl, GetItemInfo, 0x0051B450, int32_t id);
GAME_FN(void, __thiscall, WeaponInv_BeginIterate, 0x007DB6B0, void* inventory);
GAME_FN(uint8_t*, __thiscall, WeaponInv_Next, 0x0045CDB0, void* inventory);
GAME_FN(int32_t, __thiscall, WeaponInv_GetAmmo, 0x0045CF40, void* inventory, int32_t weapon);
GAME_VAR(int16_t, g_unarmedWeapon, 0x00BF4360);
GAME_FN(uint8_t, __cdecl, Item_IsOwned, 0x00541E90, int index);
GAME_FN(uint8_t, __cdecl, Item_IsGift, 0x00541AC0, int index);
GAME_FN(uint8_t, __cdecl, Item_IsMissionObject, 0x00541B00, int index);
GAME_FN(const char*, __cdecl, Item_GetDescription, 0x00541A80, int index);
GAME_FN(int32_t, __cdecl, Item_GetCount, 0x00541E00, int32_t id);
GAME_FN(void*, __cdecl, Item_GetTexture, 0x00541210, int32_t id);
GAME_VAR(int32_t, g_firstItemId, 0x00A136B8);
void* const kCollectibles = reinterpret_cast<void*>(0x00BCEB50);
GAME_FN(int16_t, __thiscall, Collectibles_GetTotal, 0x00426430, void* collectibles, int kind);
GAME_FN(int16_t, __thiscall, Collectibles_GetCount, 0x00426440, void* collectibles, int kind);
GAME_FN(int32_t, __thiscall, Collectibles_GetIcon, 0x004265B0, void* unused, int kind);   // (ecx unused)
GAME_FN(const char*, __thiscall, HudWeapons_GetDescription, 0x0055BC40, void* hud, int32_t weapon);
GAME_VAR(void*, g_hudWeapons, 0x00C674F0);
GAME_FN(uint8_t, __cdecl, Weapon_HasUnlimitedAmmo, 0x0043F660, int32_t weapon);
GAME_FN(void, __thiscall, ItemInfo_7282A0, 0x007282A0, void* info);
GAME_FN(void, __cdecl, FE_DrawDescription, 0x006A4340, const char* key);
GAME_FN(void, __cdecl, Font_AddText, 0x006880F0, const uint16_t* text);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int withSpaces);
GAME_FN(void*, __cdecl, Hud_GetArrowTexture, 0x00514EC0, int which);
GAME_FN(void*, __cdecl, FE_GetTexture, 0x006A2600, uint32_t which);
GAME_FN(void, __cdecl, DrawQuad, 0x0052CCC0, float x1, float y1, float x2, float y2, float x3, float y3, float x4,
        float y4, void* texture, const void* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, DrawSprite, 0x0052D610, const float* rect, void* texture, const void* colour, const float* uv1,
        const float* uv2);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, FE_SetTextStyle, 0x006A3FC0, int, int, int, int);
GAME_FN(uint8_t, __cdecl, Pad_ButtonJustDown, 0x00738900, int button, int pad);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
void* const kIconColour = reinterpret_cast<void*>(0x020C33D8);
void* const kSelectedIconColour = reinterpret_cast<void*>(0x020C33D4);
void* const kFrameColour = reinterpret_cast<void*>(0x020C33DC);

// a text style / position initialised as the constructors do
void InitStyle(uint8_t* style) {
    At<int32_t>(style, 0) = 0;
    At<int32_t>(style, 4) = 3;
    At<float>(style, 8) = 0.0f;
    At<float>(style, 0xC) = 0.0f;
    At<float>(style, 0x10) = 0.0f;
    At<int32_t>(style, 0x14) = 2;
    At<uint8_t>(style, 0x18) = 1;
    At<uint8_t>(style, 0x19) = 0;
    At<uint8_t>(style, 0x1A) = 0;
}

struct AlignedPosition {
    int32_t anchor;
    float x;
    float y;
};

} // namespace

namespace fedi {

// ---- FEDisplaySettings --------------------------------------------------------------------

// 0x6928C0 (vf1): Load (the layout, ints)
void __fastcall Display_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    VCall<void>(archive, 0x38, self + 0x47C, "TextStartX");
    VCall<void>(archive, 0x38, self + 0x480, "ValueStartX");
    VCall<void>(archive, 0x38, self + 0x484, "ValueDY");
    VCall<void>(archive, 0x38, self + 0x488, "ResolutionY");
    VCall<void>(archive, 0x38, self + 0x48C, "AntiAliasY");
    VCall<void>(archive, 0x38, self + 0x490, "VSyncY");
    VCall<void>(archive, 0x38, self + 0x494, "ShadowsY");
    VCall<void>(archive, 0x38, self + 0x498, "MessageOffset");
}

static bool Changing(uint8_t* self, int row) {
    return At<int32_t>(self, 0x10) == 1 && At<int32_t>(self, 0xC) == row;
}

// 0x692980: vsync's text (on / off; the text is the format)
void __fastcall Display_FormatVSync(uint8_t* self, re::Edx) {
    uint8_t on = Changing(self, 2) ? At<uint8_t>(self, 0x479) : At<uint8_t>(self, 0x478);
    uint32_t hash = on != 0 ? g_hashOn : g_hashOff;
    WsprintfW(reinterpret_cast<uint16_t*>(self + 0x49C), Text_GetByHash(kTheText, hash));
}

// 0x6929D0: the shadows' text (off, low, medium, high; anything else: hash 0)
void __fastcall Display_FormatShadows(uint8_t* self, re::Edx) {
    int32_t shadows = Changing(self, 3) ? At<int32_t>(self, 0x474) : At<int32_t>(self, 0x470);
    uint32_t hash = 0;
    switch (shadows) {
    case 0:
        hash = g_displayTextHashes[5];
        break;
    case 1:
        hash = g_displayTextHashes[6];
        break;
    case 2:
        hash = g_displayTextHashes[7];
        break;
    case 3:
        hash = g_displayTextHashes[8];
        break;
    }
    WsprintfW(reinterpret_cast<uint16_t*>(self + 0x49C), Text_GetByHash(kTheText, hash));
}

// 0x692A50 (not in the function list): FEDisplaySettings' constructor
void* __fastcall Display_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEDisplaySettings);
    At<int32_t>(self, 0x418) = 0;
    At<int32_t>(self, 0x45C) = 0;
    return self;
}

// 0x692A70: the resolution's text ("%d x %d")
void __fastcall Display_FormatResolution(uint8_t* self, re::Edx) {
    int32_t index = Changing(self, 0) ? At<int32_t>(self, 0x464) : At<int32_t>(self, 0x460);
    WsprintfW(reinterpret_cast<uint16_t*>(self + 0x49C), reinterpret_cast<const uint16_t*>(0x0093F604),
              At<int32_t>(self, 0x18 + index * 8), At<int32_t>(self, 0x1C + index * 8));
}

// 0x692AB0: anti-aliasing's text ("%d", or off for 1 and below)
void __fastcall Display_FormatAntiAlias(uint8_t* self, re::Edx) {
    int32_t level = At<int32_t>(self, 0x41C + At<int32_t>(self, 0x468) * 4);
    if (Changing(self, 1)) {
        level = At<int32_t>(self, 0x41C + At<int32_t>(self, 0x46C) * 4);
    }
    if (level > 1) {
        WsprintfW(reinterpret_cast<uint16_t*>(self + 0x49C), reinterpret_cast<const uint16_t*>(0x0093F614), level);
        return;
    }
    WsprintfW(reinterpret_cast<uint16_t*>(self + 0x49C), Text_GetByHash(kTheText, g_hashOff));
}

// 0x692B20: the chosen settings applied (shadows alone from their row)
void __fastcall Display_Apply(uint8_t* self, re::Edx) {
    GraphicsSettings settings;
    int32_t mode = At<int32_t>(self, 0x464);
    settings.width = At<int32_t>(self, 0x18 + mode * 8);
    settings.height = At<int32_t>(self, 0x1C + mode * 8);
    settings.antiAlias = At<int32_t>(self, 0x41C + At<int32_t>(self, 0x46C) * 4);
    settings.vsync = At<uint8_t>(self, 0x479);
    settings.shadows = At<int32_t>(self, 0x474);
    GraphicsSettings current;
    settings.unknown14 = Graphics_Get(&current)->unknown14;
    if (At<int32_t>(self, 0xC) == 3) {
        Graphics_SetShadows(At<int32_t>(self, 0x474));
    } else {
        Graphics_Apply(&settings);
    }
    At<int32_t>(self, 0x10) = 2;
}

// 0x692BC0: the modes and levels the device offers, the current ones found
void __fastcall Display_Refresh(uint8_t* self, re::Edx) {
    GraphicsSettings buffer;
    GraphicsSettings current = *Graphics_Get(&buffer);
    uint8_t* modes = Graphics_GetModes();
    At<int32_t>(self, 0x418) = At<int32_t>(modes, 0x400);
    for (int i = 0; i < At<int32_t>(modes, 0x400); i++) {
        At<int32_t>(self, 0x18 + i * 8) = At<int32_t>(modes, i * 8);
        At<int32_t>(self, 0x1C + i * 8) = At<int32_t>(modes, i * 8 + 4);
        if (At<int32_t>(modes, i * 8) == current.width && At<int32_t>(modes, i * 8 + 4) == current.height) {
            At<int32_t>(self, 0x460) = i;
            At<int32_t>(self, 0x464) = i;
        }
    }
    uint8_t* levels = Graphics_GetAALevels();
    At<int32_t>(self, 0x45C) = At<int32_t>(levels, 0x40);
    for (int i = 0; i < At<int32_t>(levels, 0x40); i++) {
        At<int32_t>(self, 0x41C + i * 4) = At<int32_t>(levels, i * 4);
        if (At<int32_t>(levels, i * 4) == current.antiAlias) {
            At<int32_t>(self, 0x468) = i;
            At<int32_t>(self, 0x46C) = i;
        }
    }
    At<uint8_t>(self, 0x479) = current.vsync;
    At<uint8_t>(self, 0x478) = current.vsync;
    At<int32_t>(self, 0x470) = current.shadows;
    At<int32_t>(self, 0x474) = current.shadows;
}

// 0x692CC0 (vf5): browsing: rows by up/down, accept starts changing; changing: the value
// cycles, back restores, accept applies (a direction acts once per press)
void __fastcall Display_Process(uint8_t* self, re::Edx) {
    bool forward = false;
    bool back = false;
    if ((g_menuRight != 0 || g_menuUp != 0) && FE_PadPressed(0) != 0 && At<uint8_t>(self, 0x14) == 0) {
        forward = true;
    } else if ((g_menuLeft != 0 || g_menuDown != 0) && FE_PadPressed(0) != 0 && At<uint8_t>(self, 0x14) == 0) {
        back = true;
    }
    At<uint8_t>(self, 0x14) = (g_menuLeft != 0 || g_menuRight != 0 || g_menuUp != 0 || g_menuDown != 0) ? 1 : 0;
    int32_t state = At<int32_t>(self, 0x10);
    if (state == 1) {
        switch (At<int32_t>(self, 0xC)) {
        case 0: {
            int32_t count = At<int32_t>(self, 0x418);
            if (forward) {
                FE_PlaySelect();
                At<int32_t>(self, 0x464) = (At<int32_t>(self, 0x464) + 1) % count;
                return;
            }
            if (back) {
                FE_PlaySelect();
                At<int32_t>(self, 0x464) = (At<int32_t>(self, 0x464) - 1 + count) % count;
                return;
            }
            break;
        }
        case 1: {
            int32_t count = At<int32_t>(self, 0x45C);
            if (forward) {
                FE_PlaySelect();
                At<int32_t>(self, 0x46C) = (At<int32_t>(self, 0x46C) + 1) % count;
                return;
            }
            if (back) {
                FE_PlaySelect();
                At<int32_t>(self, 0x46C) = (count - 1 + At<int32_t>(self, 0x46C)) % count;
                return;
            }
            break;
        }
        case 2:
            if (forward || back) {
                FE_PlaySelect();
                At<uint8_t>(self, 0x479) = At<uint8_t>(self, 0x479) == 0;
                return;
            }
            break;
        case 3: {
            // (high shadows only with more than 1000 MB of video memory)
            int32_t count = Graphics_VideoMemoryMb() > 1000 ? 4 : 3;
            if (forward) {
                FE_PlaySelect();
                At<int32_t>(self, 0x474) = (At<int32_t>(self, 0x474) + 1) % count;
                return;
            }
            if (back) {
                FE_PlaySelect();
                At<int32_t>(self, 0x474) = (At<int32_t>(self, 0x474) + count - 1) % count;
                return;
            }
            break;
        }
        }
        if (Input_BackPressed(kUserInput, 0) != 0) {
            At<int32_t>(self, 0x46C) = At<int32_t>(self, 0x468);
            At<int32_t>(self, 0x464) = At<int32_t>(self, 0x460);
            At<uint8_t>(self, 0x479) = At<uint8_t>(self, 0x478);
            At<int32_t>(self, 0x10) = 0;
            At<int32_t>(self, 0x474) = At<int32_t>(self, 0x470);
            return;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0) {
            reinterpret_cast<void(__thiscall*)(void*)>(0x00692B20)(self);
        }
        return;
    }
    if (state != 0) {
        return;
    }
    if (Input_BackPressed(kUserInput, 0) != 0) {
        FE_PlaySelect();
        g_menuActive = 0;
        return;
    }
    if (Input_AcceptDown(kUserInput, 0) == 0) {
        if (forward) {
            ThisCall<void>(0x00692880, self);
            FE_PlaySelect();
            return;
        }
        if (back) {
            ThisCall<void>(0x00692840, self);
            FE_PlaySelect();
        }
        return;
    }
    FE_PlaySelect();
    At<int32_t>(self, 0x10) = 1;
    switch (At<int32_t>(self, 0xC)) {
    case 0:
        At<int32_t>(self, 0x464) = At<int32_t>(self, 0x460);
        break;
    case 1:
        At<int32_t>(self, 0x46C) = At<int32_t>(self, 0x468);
        break;
    case 2:
        At<uint8_t>(self, 0x479) = At<uint8_t>(self, 0x478);
        break;
    case 3:
        At<int32_t>(self, 0x474) = At<int32_t>(self, 0x470);
        break;
    }
}

// a row: its label and its value's text (formatted by `format`)
static void DrawRow(uint8_t* self, int rowY, uint32_t labelHash, uintptr_t format, const void* labelStyle,
                    const void* valueStyle) {
    AlignedPosition label = {3, float(At<int32_t>(self, 0x47C)), float(rowY)};
    FE_DrawAligned(&label, Text_GetByHash(kTheText, labelHash), labelStyle);
    ThisCall<void>(format, self);
    AlignedPosition value = {5, float(At<int32_t>(self, 0x480)), float(At<int32_t>(self, 0x484) + rowY)};
    FE_DrawAligned(&value, reinterpret_cast<uint16_t*>(self + 0x49C), valueStyle);
}

// the shadows' "needs a restart" note when the choice differs from what's running
static void DrawShadowsMessage(uint8_t* self, int32_t shadows) {
    if ((g_shadowsSetting != 0) == (shadows != 0)) {
        return;
    }
    AlignedPosition message = {5, float(At<int32_t>(self, 0x480)),
                               float(At<int32_t>(self, 0x498) + At<int32_t>(self, 0x484) + At<int32_t>(self, 0x494))};
    FE_DrawAligned(&message, Text_GetByHash(kTheText, g_hashShadowNeedsRestart), kStyleMessage);
}

// 0x692FF0 (vf6): after applying, the modes again once the device is back; the rows (the
// current one in its own styles), the title
void __fastcall Display_Draw(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x10) == 2 && Graphics_Resetting() == 0) {
        ThisCall<void>(0x00692BC0, self);
        At<int32_t>(self, 0x10) = 0;
    }
    FE_DrawBackdrop();
    FE_DrawBackground(g_menuBackground);
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    bool changing = At<int32_t>(self, 0x10) == 1;
    const void* labelStyle = changing ? kStyleChangingLabel : kStyleRowLabel;
    const void* valueStyle = changing ? kStyleChangingValue : kStyleRowValue;
    int32_t row = At<int32_t>(self, 0xC);
    static const uintptr_t kFormats[4] = {0x00692A70, 0x00692AB0, 0x00692980, 0x006929D0};
    static const int kRowY[4] = {0x488, 0x48C, 0x490, 0x494};
    if (row >= 0 && row <= 3) {
        DrawRow(self, At<int32_t>(self, kRowY[row]), g_displayTextHashes[row], kFormats[row], labelStyle, valueStyle);
        if (row == 3) {
            DrawShadowsMessage(self, At<int32_t>(self, 0x474));
        }
    }
    for (int other = 0; other < 4; other++) {
        if (row == other) {
            continue;
        }
        DrawRow(self, At<int32_t>(self, kRowY[other]), g_displayTextHashes[other], kFormats[other], kStyleOtherLabel,
                kStyleOtherValue);
        if (other == 3) {
            DrawShadowsMessage(self, At<int32_t>(self, 0x470));
        }
    }
    FE_DrawTitle(At<int32_t>(self, 0x10) == 1 ? "FEO_CHGCTRL" : "FEO_CONTROLS");
}

// ---- FEExitGameToMenu -----------------------------------------------------------------------

// 0x693620 (not in the function list): its three text styles and the message's position
void* __fastcall ExitGame_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEExitGameToMenu);
    InitStyle(self + 0x10);
    InitStyle(self + 0x2C);
    InitStyle(self + 0x48);
    At<int32_t>(self, 0x64) = 0;
    At<float>(self, 0x68) = 0.0f;
    At<float>(self, 0x6C) = 0.0f;
    return self;
}

// 0x6936B0 (vf1): Load
void __fastcall ExitGame_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    TextStyle_Load(self + 0x2C, archive, g_exitStyleNames[0]);
    TextStyle_Load(self + 0x10, archive, g_exitStyleNames[1]);
    At<uint8_t>(self, 0x2A) = 1;
    TextStyle_Load(self + 0x48, archive, g_exitStyleNames[2]);
    At<uint8_t>(self, 0x62) = 1;
    At<int32_t>(self, 0x5C) = 3;
    Position_Load(self + 0x64, archive, g_exitStyleNames[3]);
}

// 0x693720 (vf5): yes / no; accept on yes quits to the main menu
void __fastcall ExitGame_Process(uint8_t* self, re::Edx) {
    if (Input_BackPressed(kUserInput, 0) != 0) {
        FE_PlaySelect();
        g_menuActive = 0;
    }
    bool moved = false;
    if (g_menuUp != 0 && FE_PadPressed(0) != 0) {
        At<int32_t>(self, 0xC) -= 1;
        if (At<int32_t>(self, 0xC) < 0) {
            At<int32_t>(self, 0xC) = 0;
        } else {
            moved = true;
        }
    } else if (g_menuDown != 0 && FE_PadPressed(0) != 0) {
        At<int32_t>(self, 0xC) += 1;
        if (At<int32_t>(self, 0xC) >= 2) {
            At<int32_t>(self, 0xC) = 1;
        } else {
            moved = true;
        }
    }
    if (moved) {
        FE_PadClear(0);
    }
    if (Input_AcceptDown(kUserInput, 0) == 0) {
        return;
    }
    FE_PlaySelect();
    g_menuActive = 0;
    if (At<int32_t>(self, 0xC) == 0) {
        g_exitingToMenu = 1;
        FE_ExitToMainMenu();
        g_bd0fde = 1;
        Ped_474C10(g_player, 1, 0);
        g_bd0fdd = 0;
    }
}

// 0x6937E0 (vf6): the header, the message, the highlight on the choice, yes and no
void __fastcall ExitGame_Draw(uint8_t* self, re::Edx) {
    g_fontDropColour[0] = g_menuDropColour[0];
    g_fontDropColour[1] = g_menuDropColour[1];
    g_fontDropColour[2] = g_menuDropColour[2];
    g_fontDropColour[3] = g_menuDropColour[3];
    g_fontDrop = 1;
    FE_DrawTextKey(0x140, 87, "FEM_EXIT_HEADER", 8, 2, 1, 1);
    g_menuFlagsByte &= 0xEF;
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    g_fontWrapX = 400.0f;
    FE_DrawMessageStyled(self + 0x64, "MEM_EXIT", self + 0x10);
    float yesY = float(double(g_lastTextRect[3]) + 10.0);
    float lineHeight = g_fontLineHeights[g_fontSizeIndex] * g_fontScaleY;
    float noY = lineHeight + float(2.0 + double(yesY));
    float savedDx = At<float>(self, 0x18);
    if (g_language == 7) {
        At<float>(self, 0x18) = -7.0f;
    }
    float highlightY = At<int32_t>(self, 0xC) == 1 ? noY : yesY;
    FE_DrawKeyStyled(0x140, int(highlightY), "FESA_HILITE", self + 0x10);
    if (g_language == 7) {
        At<float>(self, 0x18) = savedDx;
    }
    FE_DrawKeyStyled(0x140, int(yesY), "YES", At<int32_t>(self, 0xC) == 0 ? self + 0x10 : self + 0x2C);
    FE_DrawKeyStyled(0x140, int(noY), "NO", At<int32_t>(self, 0xC) == 1 ? self + 0x10 : self + 0x2C);
    FE_DrawTitle("FER_CONTROLS");
}

// 0x6939A0 (not in the function list): FEGeography's constructor (its text style at +0xC)
void* __fastcall Geography_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEGeography);
    InitStyle(self + 0xC);
    return self;
}

// ---- FEInventory ------------------------------------------------------------------------------

// 0x693A30 (vf1): Load
void __fastcall Inventory_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    void* tabs = self + 0x40;
    SelList_Load(tabs, archive);
    SelList_Clear(tabs);
    SelList_AddByName(tabs, "FEI_SEL_WEAPONS");
    SelList_AddByName(tabs, "FEI_SEL_GIFTS");
    SelList_AddByName(tabs, "FEI_SEL_MISOBJ");
    SelList_AddByName(tabs, "FEI_SEL_CLCTBLS");
    VCall<void>(archive, 0x28, self + 0x14, "Display StartX");
    VCall<void>(archive, 0x28, self + 0x18, "Display StartY");
    VCall<void>(archive, 0x28, self + 0x1C, "Display Width");
    VCall<void>(archive, 0x28, self + 0x20, "Display Height");
    VCall<void>(archive, 0x28, self + 0x24, "Icon Width");
    VCall<void>(archive, 0x28, self + 0x28, "Icon Height");
    VCall<void>(archive, 0x38, self + 0x2C, "Display Per Row");
    VCall<void>(archive, 0x38, self + 0x30, "Number Of Rows Displayed");
    VCall<void>(archive, 0x28, self + 0xC4, "LeftArrowXOffset");
    VCall<void>(archive, 0x28, self + 0xC8, "LeftArrowYOffset");
    VCall<void>(archive, 0x28, self + 0xCC, "RightArrowXOffset");
    VCall<void>(archive, 0x28, self + 0xD0, "RightArrowYOffset");
    VCall<void>(archive, 0x28, self + 0xD4, "ArrowWidth");
    VCall<void>(archive, 0x28, self + 0xD8, "ArrowHeight");
    Position_Load(self + 0xDC, archive, "GroupNoItemsPosInfo");
    TextStyle_Load(self + 0xE8, archive, "GroupNoItemsStyle");
    Position_Load(self + 0x104, archive, "GroupHeadingPosInfo");
    TextStyle_Load(self + 0x110, archive, "GroupHeadingStyle");
}

// the heading style's text at (x, y) offset by the style
static void DrawHeading(uint8_t* self, float x, float y, const uint16_t* text) {
    FE_DrawText(int(x + At<float>(self, 0x118)), int(y + At<float>(self, 0x11C)), text, At<int32_t>(self, 0x124),
                At<int32_t>(self, 0x110), At<int32_t>(self, 0x114), 1);
}
static void DrawHeadingKey(uint8_t* self, float x, float y, const char* key) {
    FE_DrawTextKey(int(x + At<float>(self, 0x118)), int(y + At<float>(self, 0x11C)), key, 0xB,
                   At<int32_t>(self, 0x110), At<int32_t>(self, 0x114), 0);
}
static void UseMenuDrop() {
    g_fontDropColour[0] = g_menuDropColour[0];
    g_fontDropColour[3] = g_menuDropColour[3];
    g_fontDropColour[1] = g_menuDropColour[1];
    g_fontDropColour[2] = g_menuDropColour[2];
    g_fontDrop = 1;
}

// 0x693C10: a collectible kind's name, count and total (none: "no collectibles")
void __fastcall Inventory_DrawCollectible(uint8_t* self, re::Edx, float x, float y, int kind, int32_t item) {
    uint8_t* info = GetItemInfo(item);
    if (info == nullptr || kind == -1) {
        g_menuFlagsByte |= 0x10;
        DrawHeadingKey(self, x, y, "FEI_NO_CLCTBLS");
        g_menuFlagsByte &= 0xEF;
        return;
    }
    const char* name;
    const char* description;
    if (kind == 0) {
        name = "Item_Gnome";
        description = "Desc_Gnome";
    } else if (kind == 4) {
        name = "Item_Pumpkin";
        description = "Desc_Pumpkin";
    } else if (kind == 5) {
        name = "Item_Tombstone";
        description = "Desc_Tombstone";
    } else {
        name = At<const char*>(info, 0x8C);
        description = At<const char*>(info, 0x48);
    }
    if (description != nullptr) {
        FE_DrawDescription(description);
    }
    const uint16_t* nameText = Text_Get(kTheText, name);
    int32_t total = Collectibles_GetTotal(kCollectibles, kind);
    int32_t count = Collectibles_GetCount(kCollectibles, kind);
    g_menuFlagsByte |= 0x10;
    Font_AddText(nameText);
    Font_AddNumber(float(count));
    Font_AddNumber(float(total));
    uint16_t text[0x100];
    Font_Format(text, Text_Get(kTheText, "FEI_ITEMINFO"), 0x100);
    DrawHeading(self, x, y, text);
    g_menuFlagsByte &= 0xEF;
}

// 0x693DC0: how many items the selected group has
int __fastcall Inventory_CountGroup(uint8_t* self, re::Edx) {
    uint8_t* player = g_player;
    int count = 0;
    switch (SelList_Selected(self + 0x40)) {
    case 0: {
        void* inventory = At<void*>(player, 0x1C4);
        WeaponInv_BeginIterate(inventory);
        for (uint8_t* slot = WeaponInv_Next(At<void*>(player, 0x1C4)); slot != nullptr;
             slot = WeaponInv_Next(At<void*>(player, 0x1C4))) {
            int16_t weapon = At<int16_t>(slot, 2);
            if (At<int32_t>(GetItemInfo(weapon), 0x64) == 1 && weapon != g_unarmedWeapon) {
                count++;
            }
        }
        return count;
    }
    case 1:
        for (int i = 0; i < 0x50; i++) {
            if (Item_IsOwned(i) != 0 && Item_IsGift(i) != 0) {
                count++;
            }
        }
        return count;
    case 2:
        for (int i = 0; i < 0x50; i++) {
            if (Item_IsOwned(i) != 0 && Item_IsMissionObject(i) != 0) {
                count++;
            }
        }
        return count;
    case 3:
        for (int i = 0; i < 7; i++) {
            if (Collectibles_GetCount(kCollectibles, i) > 0) {
                count++;
            }
        }
        return count;
    }
    return 0;
}

// 0x693F10 (not in the function list): FEInventory's constructor
void* __fastcall Inventory_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEInventory);
    SelList_Construct(self + 0x40, self + 0x40 + 0x70, 4);
    SetVtable(self + 0x40, VT_SelListT3);
    At<int32_t>(self, 0xDC) = 0;
    At<float>(self, 0xE0) = 0.0f;
    At<float>(self, 0xE4) = 0.0f;
    InitStyle(self + 0xE8);
    At<int32_t>(self, 0x104) = 0;
    At<float>(self, 0x108) = 0.0f;
    At<float>(self, 0x10C) = 0.0f;
    InitStyle(self + 0x110);
    return self;
}

// 0x693FE0 (vf3): the first group from its start
void __fastcall Inventory_OnEnter(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x34) = 0;
    At<uint8_t>(self, 0x3D) = 0;
    At<uint8_t>(self, 0x3C) = 0;
    At<int32_t>(self, 0xC0) = 0;
    SelList_Reset(self + 0x40);
    At<int32_t>(self, 0x38) = Inventory_CountGroup(self, nullptr);
}

// 0x694010 (vf5): the groups by left/right (the selection back to the first), accept
// browses the group's items (left/right), button 8 stops browsing
void __fastcall Inventory_Process(uint8_t* self, re::Edx) {
    int32_t mode = At<int32_t>(self, 0xC0);
    if (mode == 0) {
        if (Input_BackPressed(kUserInput, 0) != 0) {
            FE_PlaySelect();
            g_menuActive = 0;
            return;
        }
        void* tabs = self + 0x40;
        bool changed = false;
        if (g_menuRight != 0) {
            if (FE_PadPressed(0) != 0) {
                int8_t before = SelList_Selected(tabs);
                changed = before != SelList_Next(tabs);
            }
        } else if (g_menuLeft != 0 && FE_PadPressed(0) != 0) {
            int8_t before = SelList_Selected(tabs);
            changed = before != SelList_Previous(tabs);
        }
        if (changed) {
            At<int32_t>(self, 0x34) = 0;
            At<uint8_t>(self, 0x3D) = 0;
            At<uint8_t>(self, 0x3C) = 0;
            At<int32_t>(self, 0x38) = Inventory_CountGroup(self, nullptr);
            FE_PadClear(0);
        }
        if (Input_AcceptDown(kUserInput, 0) != 0 &&
            At<int32_t>(self, 0x38) > 0) {
            FE_PlaySelect();
            At<int32_t>(self, 0xC0) = 1;
            At<uint8_t>(self, 0xAD) = 1;
        }
        return;
    }
    if (mode != 1) {
        return;
    }
    bool moved = false;
    if (g_menuLeft != 0) {
        if (FE_PadPressed(0) != 0) {
            At<int32_t>(self, 0x34) -= 1;
            if (At<int32_t>(self, 0x34) < 0) {
                At<int32_t>(self, 0x34) += 1;
            } else {
                moved = true;
            }
        }
    } else if (g_menuRight != 0 && FE_PadPressed(0) != 0) {
        At<int32_t>(self, 0x34) += 1;
        if (At<int32_t>(self, 0x34) >= At<int32_t>(self, 0x38)) {
            At<int32_t>(self, 0x34) -= 1;
        } else {
            moved = true;
        }
    }
    if (moved) {
        ThisCall<void>(0x00693BE0, self);
        FE_PadClear(0);
    }
    if (Pad_ButtonJustDown(8, 0) == 0) {
        return;
    }
    FE_PlaySelect();
    At<int32_t>(self, 0xC0) = 0;
    At<uint8_t>(self, 0xAD) = 0;
}

// 0x694190: a weapon's name, ammo and clip (or unlimited); none: "no weapons"
void __fastcall Inventory_DrawWeapon(uint8_t* self, re::Edx, float x, float y, const int32_t* weapon) {
    if (*weapon < 0) {
        g_menuFlagsByte |= 0x10;
        DrawHeadingKey(self, x, y, "FEI_NO_WEAPONS");
        g_menuFlagsByte &= 0xEF;
        return;
    }
    const char* description = HudWeapons_GetDescription(g_hudWeapons, *weapon);
    if (description != nullptr) {
        FE_DrawDescription(description);
    }
    const uint16_t* name = Text_Get(kTheText, At<const char*>(GetItemInfo(*weapon), 0x8C));
    g_menuFlagsByte |= 0x10;
    uint16_t text[0x100];
    if (Weapon_HasUnlimitedAmmo(*weapon) != 0) {
        Font_AddText(name);
        Font_Format(text, Text_Get(kTheText, "FEI_UNLIMITEDAMMO"), 0x100);
    } else {
        // (the ammo's weapon: the info's +0x84 when set)
        int32_t ammoWeapon;
        if (GetItemInfo(*weapon) != nullptr) {
            uint8_t* info = GetItemInfo(*weapon);
            ItemInfo_7282A0(info);
            ammoWeapon = At<int32_t>(info, 0x84);
        } else {
            ammoWeapon = *weapon;
        }
        if (ammoWeapon <= 0) {
            ammoWeapon = *weapon;
        }
        int32_t clip = GetItemInfo(ammoWeapon) != nullptr ? At<int32_t>(GetItemInfo(ammoWeapon), 0x34) : 1;
        int32_t ammo = WeaponInv_GetAmmo(At<void*>(g_player, 0x1C4), ammoWeapon);
        Font_AddText(name);
        Font_AddNumber(float(ammo));
        Font_AddNumber(float(clip));
        Font_Format(text, Text_Get(kTheText, "FEI_ITEMINFO"), 0x100);
    }
    UseMenuDrop();
    DrawHeading(self, x, y, text);
    g_menuFlagsByte &= 0xEF;
}

// 0x6943E0: a gift's / mission object's name and count; none: "no gifts" / "no mission
// objects" (another kind: an uninitialised key, as the game)
void __fastcall Inventory_DrawItem(uint8_t* self, re::Edx, float x, float y, int kind, int32_t index) {
    if (index < 0) {
        g_menuFlagsByte |= 0x10;
        char key[0x40];
        if (kind == 0) {
            Strncpy(key, "FEI_NO_GIFTS", 0x3F);
        } else if (kind == 1) {
            Strncpy(key, "FEI_NO_MISOBJ", 0x3F);
        } else {
            key[0] = 0;  // (the game's buffer is left as it was)
        }
        DrawHeadingKey(self, x, y, key);
        g_menuFlagsByte &= 0xEF;
        return;
    }
    const char* description = Item_GetDescription(index);
    if (description != nullptr) {
        FE_DrawDescription(description);
    }
    int32_t id = index + g_firstItemId;
    const char* nameKey = GetItemInfo(id) != nullptr ? At<const char*>(GetItemInfo(id), 0x8C) : nullptr;
    const uint16_t* name = Text_Get(kTheText, nameKey);
    int32_t clip = GetItemInfo(id) != nullptr ? At<int32_t>(GetItemInfo(id), 0x34) : 1;
    int32_t count = Item_GetCount(id);
    g_menuFlagsByte |= 0x10;
    Font_AddText(name);
    Font_AddNumber(float(count));
    Font_AddNumber(float(clip));
    uint16_t text[0x100];
    Font_Format(text, Text_Get(kTheText, "FEI_ITEMINFO"), 0x100);
    UseMenuDrop();
    DrawHeading(self, x, y, text);
    g_menuFlagsByte &= 0xEF;
}

// 0x6945F0: the scroll arrows either side (bright when there's more that way)
void __fastcall Inventory_DrawArrows(uint8_t* self, re::Edx) {
    void* leftTexture = Hud_GetArrowTexture(0);
    void* rightTexture = Hud_GetArrowTexture(1);
    float width = At<float>(self, 0xD4) * g_screenScaleX;
    int32_t perRow = At<int32_t>(self, 0x2C);
    uint8_t first = At<uint8_t>(self, 0x3C);
    float height = At<float>(self, 0xD8) * g_screenScaleY;
    float halfHeight = float(double(height) * 0.5);
    float middle = float(double(At<float>(self, 0x20)) * 0.5) + At<float>(self, 0x18);
    float alpha = first != 0 ? 255.0f : 102.0f;
    float x = At<float>(self, 0xC4) + At<float>(self, 0x14) - width;
    float y = (At<float>(self, 0xC8) + middle) - halfHeight;
    float uv2[2] = {1.0f, 1.0f};
    float uv1[2] = {0.0f, 0.0f};
    uint8_t colour[4];
    RGBA_Construct(colour, 255, 255, 255, uint8_t(int(alpha)));
    DrawQuad(x, y, width + x, y, x, height + y, width + x, height + y, leftTexture, colour, uv1, uv2);
    int32_t last = perRow + first - 1;
    alpha = last < At<int32_t>(self, 0x38) - 1 ? 255.0f : 102.0f;
    x = At<float>(self, 0x1C) + At<float>(self, 0x14) + At<float>(self, 0xCC);
    y = (At<float>(self, 0xD0) + middle) - halfHeight;
    float uvA[2] = {1.0f, 1.0f};
    float uvB[2] = {0.0f, 0.0f};
    RGBA_Construct(colour, 255, 255, 255, uint8_t(int(alpha)));
    DrawQuad(x, y, x + width, y, x, y + height, x + width, y + height, rightTexture, colour, uvB, uvA);
}

// 0x694850: the two windows and the frame's ends
void __fastcall Inventory_DrawFrame(void*, re::Edx) {
    FE_DrawWindow(320.0f, 440.0f, 150.0f, kWindowColour, 2.0f);
    FE_DrawWindow(320.0f, 440.0f, 270.0f, kWindowColour, 2.0f);
    float scaleX = g_screenScaleX;
    float scaleY = g_screenScaleY;
    float uvOne[2] = {1.0f, 1.0f};
    float uvZero[2] = {0.0f, 0.0f};
    // (x87 at 24 bits: the products rounded to float)
    double width = double(scaleX * 20.0f);
    float top = 400.0f - scaleY * 80.0f;
    DrawRectUV(100.0f, top, float(width + 100.0), 400.0f, FE_GetTexture(7), kFrameColour, uvZero, uvOne);
    float uvA[2] = {1.0f, 0.0f};
    float uvB[2] = {0.0f, 1.0f};
    DrawRectUV(float(540.0 - width), top, 540.0f, 400.0f, FE_GetTexture(7), kFrameColour, uvA, uvB);
}

// the grid's cell of the n-th shown item, and the icon drawn there
struct Grid {
    float cellWidth, cellHeight, halfIconWidth, halfIconHeight;
};
static Grid GridOf(uint8_t* self) {
    Grid grid;
    grid.cellWidth = At<float>(self, 0x1C) / float(At<int32_t>(self, 0x2C));
    grid.cellHeight = At<float>(self, 0x20) / float(At<int32_t>(self, 0x30));
    grid.halfIconWidth = float(double(At<float>(self, 0x24) * g_screenScaleX) * 0.5);
    grid.halfIconHeight = float(double(At<float>(self, 0x28) * g_screenScaleY) * 0.5);
    return grid;
}
static void DrawIcon(uint8_t* self, const Grid& grid, int shown, void* texture, bool selected) {
    int32_t perRow = At<int32_t>(self, 0x2C);
    float x = At<float>(self, 0x14) + grid.cellWidth * 0.5f + float(shown % perRow) * grid.cellWidth;
    float y = At<float>(self, 0x18) + grid.cellHeight * 0.5f + float(shown / perRow) * grid.cellHeight;
    float rect[4] = {x - grid.halfIconWidth, y + grid.halfIconHeight, x + grid.halfIconWidth, y - grid.halfIconHeight};
    float uv2[2] = {1.0f, 1.0f};
    float uv1[2] = {0.0f, 0.0f};
    DrawSprite(rect, texture, selected ? kSelectedIconColour : kIconColour, uv1, uv2);
}

// 0x694A10: the weapons (the selected one's details when browsing)
void __fastcall Inventory_DrawWeapons(uint8_t* self, re::Edx, int browsing) {
    Grid grid = GridOf(self);
    uint8_t first = At<uint8_t>(self, 0x3C);
    int32_t perRow = At<int32_t>(self, 0x2C);
    uint8_t* player = g_player;
    Text_Get(kTheText, "FEI_WEAPONS");
    Inventory_DrawFrame(self, nullptr);
    WeaponInv_BeginIterate(At<void*>(player, 0x1C4));
    int count = 0;
    for (uint8_t* slot = WeaponInv_Next(At<void*>(player, 0x1C4)); slot != nullptr;
         slot = WeaponInv_Next(At<void*>(player, 0x1C4))) {
        int32_t weapon = At<int16_t>(slot, 2);
        if (At<int32_t>(GetItemInfo(weapon), 0x64) != 1 || weapon == g_unarmedWeapon) {
            continue;
        }
        if (first <= count && count <= first - 1 + perRow) {
            bool selected = re::Flag(browsing) && count == At<int32_t>(self, 0x34);
            DrawIcon(self, grid, count - first, Item_GetTexture(weapon), selected);
            if (selected) {
                float y = Position_GetY(self + 0x104);
                float x = Position_GetX(self + 0x104);
                Inventory_DrawWeapon(self, nullptr, x, y, &weapon);
            }
        }
        count++;
    }
    if (count <= 0) {
        int32_t none = -1;
        float y = Position_GetY(self + 0xDC);
        float x = Position_GetX(self + 0xDC);
        Inventory_DrawWeapon(self, nullptr, x, y, &none);
    }
    if (count > At<int32_t>(self, 0x2C)) {
        Inventory_DrawArrows(self, nullptr);
    }
}

// 0x694CF0: the collectibles
void __fastcall Inventory_DrawCollectibles(uint8_t* self, re::Edx, int browsing) {
    Grid grid = GridOf(self);
    uint8_t first = At<uint8_t>(self, 0x3C);
    int32_t perRow = At<int32_t>(self, 0x2C);
    Font_GetStringWidth(Text_Get(kTheText, "FEI_CLCTBLS"), 1);
    Inventory_DrawFrame(self, nullptr);
    int count = 0;
    for (int kind = 0; kind < 7; kind++) {
        if (Collectibles_GetCount(kCollectibles, kind) <= 0) {
            continue;
        }
        if (first <= count && count <= first - 1 + perRow) {
            int32_t icon = Collectibles_GetIcon(nullptr, kind);
            bool selected = re::Flag(browsing) && count == At<int32_t>(self, 0x34);
            DrawIcon(self, grid, count - first, Item_GetTexture(icon), selected);
            if (selected) {
                float y = Position_GetY(self + 0x104);
                float x = Position_GetX(self + 0x104);
                Inventory_DrawCollectible(self, nullptr, x, y, kind, icon);
            }
        }
        count++;
    }
    if (count < 1) {
        // (DrawCollectible inlined with no item: its info looked up, then "none")
        float x = Position_GetX(self + 0xDC);
        float y = Position_GetY(self + 0xDC);
        GetItemInfo(-1);
        g_menuFlagsByte |= 0x10;
        DrawHeadingKey(self, x, y, "FEI_NO_CLCTBLS");
        g_menuFlagsByte &= 0xEF;
    }
    if (count > At<int32_t>(self, 0x2C)) {
        Inventory_DrawArrows(self, nullptr);
    }
}

// 0x694FF0: gifts (kind 0) or mission objects (1)
void __fastcall Inventory_DrawItems(uint8_t* self, re::Edx, int browsing, int kind) {
    Grid grid = GridOf(self);
    uint8_t first = At<uint8_t>(self, 0x3C);
    int32_t perRow = At<int32_t>(self, 0x2C);
    const uint16_t* title = nullptr;
    if (kind == 0) {
        title = Text_Get(kTheText, "FEI_GIFTS");
    } else if (kind == 1) {
        title = Text_Get(kTheText, "FEI_MISOBJ");
    }
    char key[0x40];
    key[0] = 0;
    if (kind == 0) {
        Strncpy(key, "FEI_GIFTS", 0x3F);
    } else if (kind == 1) {
        Strncpy(key, "FEI_MISOBJ", 0x3F);
    }
    Font_GetStringWidth(title, 1);
    Inventory_DrawFrame(self, nullptr);
    int count = 0;
    for (int i = 0; i < 0x50; i++) {
        int32_t id = g_firstItemId + i;
        if (Item_IsOwned(i) == 0) {
            continue;
        }
        if (kind == 0 ? Item_IsGift(i) == 0 : (kind == 1 && Item_IsMissionObject(i) == 0)) {
            continue;
        }
        if (first <= count && count <= first - 1 + perRow) {
            bool selected = re::Flag(browsing) && count == At<int32_t>(self, 0x34);
            DrawIcon(self, grid, count - first, Item_GetTexture(id), selected);
            if (selected) {
                float y = Position_GetY(self + 0x104);
                float x = Position_GetX(self + 0x104);
                Inventory_DrawItem(self, nullptr, x, y, kind, i);
            }
        }
        count++;
    }
    if (count < 1) {
        float y = Position_GetY(self + 0xDC);
        float x = Position_GetX(self + 0xDC);
        Inventory_DrawItem(self, nullptr, x, y, kind, -1);
    }
    if (count > At<int32_t>(self, 0x2C)) {
        Inventory_DrawArrows(self, nullptr);
    }
}

// 0x695330 (vf6): the window, the tabs, the group, the pocket money, the title
void __fastcall Inventory_Draw(uint8_t* self, re::Edx) {
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    SelList_Draw(self + 0x40);
    int browsing = At<int32_t>(self, 0xC0) == 1 ? 1 : 0;
    switch (SelList_Selected(self + 0x40)) {
    case 0:
        Inventory_DrawWeapons(self, nullptr, browsing);
        break;
    case 1:
        Inventory_DrawItems(self, nullptr, browsing, 0);
        break;
    case 2:
        Inventory_DrawItems(self, nullptr, browsing, 1);
        break;
    case 3:
        Inventory_DrawCollectibles(self, nullptr, browsing);
        break;
    }
    Font_Reset();
    FE_SetTextStyle(3, 2, 1, 1);
    Font_AddNumber(float(At<uint32_t>(g_player, 0x1CA0)));
    uint16_t text[0x20];
    Font_Format(text, Text_Get(kTheText, "POCKET_MONEY_STR"), 0x20);
    Font_PrintString(320.0f, 122.0f, text, 1);
    Font_Render();
    FE_DrawTitle(browsing != 0 ? "FEI_CTL_NCHS" : "FEI_CONTROLS");
}

} // namespace fedi

#define FEDI_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(fedi_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(fedi_original_, __LINE__), \
             re::Mode::Original)

FEDI_REVERSED("FEDisplaySettings::Load", 0x006928C0, &fedi::Display_Load);
FEDI_REVERSED("FEDisplaySettings::FormatVSync", 0x00692980, &fedi::Display_FormatVSync);
FEDI_REVERSED("FEDisplaySettings::FormatShadows", 0x006929D0, &fedi::Display_FormatShadows);
FEDI_REVERSED("FEDisplaySettings::FEDisplaySettings", 0x00692A50, &fedi::Display_Construct);
FEDI_REVERSED("FEDisplaySettings::FormatResolution", 0x00692A70, &fedi::Display_FormatResolution);
FEDI_REVERSED("FEDisplaySettings::FormatAntiAlias", 0x00692AB0, &fedi::Display_FormatAntiAlias);
FEDI_REVERSED("FEDisplaySettings::Apply", 0x00692B20, &fedi::Display_Apply);
FEDI_REVERSED("FEDisplaySettings::Refresh", 0x00692BC0, &fedi::Display_Refresh);
FEDI_REVERSED("FEDisplaySettings::Process", 0x00692CC0, &fedi::Display_Process);
FEDI_REVERSED("FEDisplaySettings::Draw", 0x00692FF0, &fedi::Display_Draw);
FEDI_REVERSED("FEExitGameToMenu::FEExitGameToMenu", 0x00693620, &fedi::ExitGame_Construct);
FEDI_REVERSED("FEExitGameToMenu::Load", 0x006936B0, &fedi::ExitGame_Load);
FEDI_REVERSED("FEExitGameToMenu::Process", 0x00693720, &fedi::ExitGame_Process);
FEDI_REVERSED("FEExitGameToMenu::Draw", 0x006937E0, &fedi::ExitGame_Draw);
FEDI_REVERSED("FEGeography::FEGeography", 0x006939A0, &fedi::Geography_Construct);
FEDI_REVERSED("FEInventory::Load", 0x00693A30, &fedi::Inventory_Load);
FEDI_REVERSED("FEInventory::DrawCollectible", 0x00693C10, &fedi::Inventory_DrawCollectible);
FEDI_REVERSED("FEInventory::CountGroup", 0x00693DC0, &fedi::Inventory_CountGroup);
FEDI_REVERSED("FEInventory::FEInventory", 0x00693F10, &fedi::Inventory_Construct);
FEDI_REVERSED("FEInventory::OnEnter", 0x00693FE0, &fedi::Inventory_OnEnter);
FEDI_REVERSED("FEInventory::Process", 0x00694010, &fedi::Inventory_Process);
FEDI_REVERSED("FEInventory::DrawWeapon", 0x00694190, &fedi::Inventory_DrawWeapon);
FEDI_REVERSED("FEInventory::DrawItem", 0x006943E0, &fedi::Inventory_DrawItem);
FEDI_REVERSED("FEInventory::DrawArrows", 0x006945F0, &fedi::Inventory_DrawArrows);
FEDI_REVERSED("FEInventory::DrawFrame", 0x00694850, &fedi::Inventory_DrawFrame);
FEDI_REVERSED("FEInventory::DrawWeapons", 0x00694A10, &fedi::Inventory_DrawWeapons);
FEDI_REVERSED("FEInventory::DrawCollectibles", 0x00694CF0, &fedi::Inventory_DrawCollectibles);
FEDI_REVERSED("FEInventory::DrawItems", 0x00694FF0, &fedi::Inventory_DrawItems);
FEDI_REVERSED("FEInventory::Draw", 0x00695330, &fedi::Inventory_Draw);
