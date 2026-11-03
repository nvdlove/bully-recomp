// The frontend's photo album (FEPhotoAlbum), quit / restart confirmations and the save
// screen (FESave), and FESaveAlgie. 0x6984E0..0x69AB39.
//
// FEPhotoAlbum: +0xC / +0x24 the arrows' rects, +0x3C the thumbnails' rect, +0x54 / +0x6C
//   other rects, +0x84.. text styles (+0x84, +0xA0 highlight, +0xBC, +0x114, +0x13C),
//   positions (+0xD8, +0xE4, +0xF0, +0xFC, +0x108, +0x130, +0x158), +0x164 the mode
//   (0 browsing, 1 delete?, 2 deleted), +0x168 no chosen, +0x169 skip a page load,
//   +0x16C a pending job (1 refresh, 2 page, 3 delete), +0x170 the photo shown, +0x174 the
//   cursor, +0x178 the first thumbnail.
// FERestart: like FEExitGameToMenu (+0xC yes/no, styles +0x10 / +0x2C / +0x48, +0x64 a
//   position).
// FESave: the load screen's table (+0xC.. layout ints), +0x1C just entered, +0x24 the slot,
//   +0x28 the state (0 table, 1 save?, 2 overwrite?, 3 format?, 4 retry, 5 done, 6 recreate,
//   7 delete?), +0x2C no chosen, +0x30 the completion, styles +0x34 (highlight), +0x50,
//   +0x6C, +0x88 a position.

#include <cmath>

#include "game/FEScreensCommon.h"

using namespace fescr;

namespace {

constexpr uintptr_t VT_FEPhotoAlbum = 0x00940080;
constexpr uintptr_t VT_FEQuitGame = 0x009400F8;
constexpr uintptr_t VT_FERestart = 0x00940124;
constexpr uintptr_t VT_FESave = 0x00940170;
constexpr uintptr_t VT_FESaveAlgie = 0x009401EC;

void* const kWindowColour = reinterpret_cast<void*>(0x020C33E4);
void* const kSelectedColour = reinterpret_cast<void*>(0x020C33D4);
void* const kThumbColour = reinterpret_cast<void*>(0x020C33D8);

// the photo album (0xAB4688)
void* const kAlbum = reinterpret_cast<void*>(0x00AB4688);
GAME_VAR(int32_t, g_photoCount, 0x00AB468C);
GAME_VAR(int32_t, g_albumCurrent, 0x00AB4690);
GAME_VAR(int32_t, g_thumbCount, 0x00AB46A4);
uint8_t* const kThumbs = reinterpret_cast<uint8_t*>(0x00AB46A8);  // 0xC each
GAME_FN(uint8_t, __thiscall, Album_RefreshCount, 0x00543DE0, void* album, int);
GAME_FN(uint8_t, __thiscall, Album_Delete, 0x00543E10, void* album, int index);
GAME_FN(uint8_t, __thiscall, Album_Select, 0x005453F0, void* album, int index);
GAME_FN(void, __thiscall, Album_ClearThumbs, 0x00546200, void* album);
GAME_FN(void, __thiscall, Album_Clear, 0x00546260, void* album);
GAME_FN(uint8_t, __thiscall, Album_LoadThumbs, 0x00546490, void* album, int first);
GAME_FN(void*, __thiscall, Album_GetTexture, 0x0077E8D0, void* album);
GAME_FN(void*, __thiscall, Thumb_GetTexture, 0x00856850, void* thumb);
GAME_VAR(int32_t, g_albumLastState, 0x00BA6464);
GAME_ARRAY(const char*, g_albumNames, 0x00BA6400);  // [0] "FEPA_NPHOTO", then the layout's names

// the save system
GAME_FN(int32_t, __cdecl, SaveSys_GetState, 0x00735370);
GAME_FN(const char*, __cdecl, SaveSys_GetSlotInfo, 0x007354B0, int slot);
GAME_FN(void, __cdecl, SaveSys_Start, 0x007353E0, int slot);
GAME_FN(int32_t, __cdecl, SaveSys_FirstUsedSlot, 0x007373D0, int);
GAME_FN(void, __cdecl, SaveSys_DrawMessage, 0x00735660, const void* position, const void* style);
GAME_FN(uint8_t, __cdecl, SaveSys_7368C0, 0x007368C0, int);
GAME_FN(uint8_t, __cdecl, SaveSys_736E70, 0x00736E70, int);
GAME_FN(void, __cdecl, SaveSys_Init, 0x007351C0);
GAME_FN(void, __cdecl, SaveSys_735460, 0x00735460);
GAME_FN(uint8_t, __thiscall, Uber_HasSpace, 0x008A0200, void* uber, int size, uint32_t* needed, int);
GAME_FN(uint8_t, __thiscall, Uber_736480, 0x00736480, void* uber, int);
GAME_FN(void, __thiscall, Uber_7364A0, 0x007364A0, void* uber, int);
GAME_FN(float, __cdecl, Game_GetCompletion, 0x006A98C0);
GAME_VAR(uint8_t*, g_uberCompletion, 0x020CE75C);
GAME_VAR(int32_t, g_saveLastState, 0x00BA64BC);
GAME_VAR(uint32_t, g_hashCompletion, 0x020C3094);
GAME_ARRAY(const char*, g_restartStyleNames, 0x00BA6484);  // [4]
GAME_ARRAY(const char*, g_saveStyleNames, 0x00BA64AC);     // [4]
GAME_VAR(uint8_t, g_exitingToMenu, 0x020C32EC);

// drawing
GAME_FN(void, __cdecl, FE_DrawTextKey, 0x006A6380, int x, int y, const char* key, int font, int justify, int, int);
GAME_FN(void, __cdecl, FE_DrawKeyStyled, 0x006A6CC0, int x, int y, const char* key, const void* style);
GAME_FN(void, __cdecl, FE_DrawTextStyled, 0x006A6460, int x, int y, const uint16_t* text, const void* style);
GAME_FN(void, __cdecl, FE_DrawMessageStyled, 0x006A6D00, const void* position, const char* key, const void* style);
GAME_FN(void, __cdecl, FE_DrawMessageNumbers, 0x006A6630, const void* position, const char* key, const void* style);
GAME_FN(void, __cdecl, FE_DrawSaveInfo, 0x006A5540, const char* info, float left, float right, float y1, float y2,
        int colour, float scaleX, float scaleY, int highlighted);
GAME_FN(void, __cdecl, FE_DrawRectSprite, 0x006A42A0, const void* rect, void* texture, const void* colour,
        const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, FE_DrawFrameRect, 0x006A4420, float left, float top, float right, float bottom, float width,
        const void* inner, const void* border);
GAME_FN(void*, __cdecl, FE_GetTexture, 0x006A2600, uint32_t which);
GAME_FN(void, __cdecl, FE_PlaySoundId, 0x006A2340, int sound);
GAME_FN(void, __cdecl, FE_ExitToMainMenu, 0x006A4250);
GAME_FN(void, __thiscall, Position_Load, 0x006A4940, void* position, void* archive, const char* name);
GAME_FN(void, __thiscall, Rect_Load, 0x006A49C0, void* rect, void* archive, const char* name);
GAME_FN(float, __thiscall, Position_GetY, 0x006A2790, const void* position);
GAME_FN(float, __thiscall, Rect_GetX, 0x006A2800, const void* rect);
GAME_FN(float, __thiscall, Rect_GetY, 0x006A2890, const void* rect);
GAME_FN(float, __thiscall, Rect_GetWidth, 0x006A2920, const void* rect);
GAME_FN(float, __thiscall, Rect_GetHeight, 0x006A2940, const void* rect);
GAME_FN(void, __cdecl, Font_SetScale, 0x006880B0, float x, float y);
GAME_FN(void, __cdecl, Font_GetExtent, 0x00688EA0, float* rect, uintptr_t);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(uint8_t, __cdecl, Pad_ButtonJustDown, 0x00738900, int button, int pad);
GAME_VAR(uint8_t, g_menuFlagsByte, 0x0208B78A);
GAME_VAR(float, g_fontWrapX, 0x0208B778);
GAME_VAR(int16_t, g_fontSizeIndex, 0x0208B788);
GAME_ARRAY(float, g_fontLineHeights, 0x00BA4E8C);
GAME_VAR(float, g_fontScaleY, 0x0208B770);
GAME_ARRAY(uint8_t, g_menuColours, 0x020C331C);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);

// algie
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* key);
GAME_FN(int32_t, __thiscall, MissionMgr_Find, 0x006A9FD0, void* mgr, uint32_t* key);

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

void UseMenuDrop() {
    g_fontDropColour[0] = g_menuColours[0];
    g_fontDropColour[3] = g_menuColours[3];
    g_fontDropColour[1] = g_menuColours[1];
    g_fontDropColour[2] = g_menuColours[2];
    g_fontDrop = 1;
}

// "yes" at yesY, "no" a line below, the highlight on the chosen one (its style's dx at
// -7 for language 7 while it's drawn)
void DrawYesNo(uint8_t* self, float yesY, bool noChosen, int hiliteStyle, int otherStyle) {
    float lineHeight = g_fontLineHeights[g_fontSizeIndex] * g_fontScaleY;
    float noY = lineHeight + float(2.0 + double(yesY));
    float& dx = At<float>(self, hiliteStyle + 8);
    float savedDx = dx;
    if (g_language == 7) {
        dx = -7.0f;
    }
    FE_DrawKeyStyled(0x140, int(noChosen ? noY : yesY), "FESA_HILITE", self + hiliteStyle);
    if (g_language == 7) {
        dx = savedDx;
    }
    FE_DrawKeyStyled(0x140, int(yesY), "YES", self + (!noChosen ? hiliteStyle : otherStyle));
    FE_DrawKeyStyled(0x140, int(noY), "NO", self + (noChosen ? hiliteStyle : otherStyle));
}

// an arrow (texture 3 at +0x24, 2 at +0xC), faint when dimmed
void DrawArrow(uint8_t* self, int rectOffset, uint32_t texture, bool dimmed) {
    uint8_t colour[4];
    RGBA_Construct(colour, 255, 255, 255, 255);
    if (dimmed) {
        colour[3] = 0x20;
    }
    float uv2[2] = {1.0f, 1.0f};
    float uv1[2] = {0.0f, 0.0f};
    FE_DrawRectSprite(self + rectOffset, FE_GetTexture(texture), colour, uv1, uv2);
}

} // namespace

namespace feps {

// ---- FEPhotoAlbum ------------------------------------------------------------------------

// 0x6984E0 (not in the function list): its rects, styles and positions zeroed
void* __fastcall PhotoAlbum_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEPhotoAlbum);
    // (rects: anchor 0 / 1, four floats)
    static const int kRects[4] = {0xC, 0x24, 0x3C, 0x54};
    for (int offset : kRects) {
        At<int32_t>(self, offset) = 0;
        At<float>(self, offset + 4) = 0.0f;
        At<float>(self, offset + 8) = 0.0f;
        At<float>(self, offset + 0xC) = 0.0f;
        At<float>(self, offset + 0x10) = 0.0f;
        At<int32_t>(self, offset + 0x14) = 1;
    }
    At<int32_t>(self, 0x6C) = 0;
    At<float>(self, 0x70) = 0.0f;
    At<float>(self, 0x74) = 0.0f;
    At<float>(self, 0x78) = 0.0f;
    At<float>(self, 0x7C) = 0.0f;
    At<int32_t>(self, 0x80) = 1;
    InitStyle(self + 0x84);
    InitStyle(self + 0xA0);
    InitStyle(self + 0xBC);
    static const int kPositions[5] = {0xD8, 0xE4, 0xF0, 0xFC, 0x108};
    for (int offset : kPositions) {
        At<int32_t>(self, offset) = 0;
        At<float>(self, offset + 4) = 0.0f;
        At<float>(self, offset + 8) = 0.0f;
    }
    InitStyle(self + 0x114);
    At<int32_t>(self, 0x130) = 0;
    At<float>(self, 0x134) = 0.0f;
    At<float>(self, 0x138) = 0.0f;
    InitStyle(self + 0x13C);
    At<int32_t>(self, 0x158) = 0;
    At<float>(self, 0x15C) = 0.0f;
    At<float>(self, 0x160) = 0.0f;
    return self;
}

// 0x6986F0 (vf1): Load (the names from the table at 0xBA6404)
void __fastcall PhotoAlbum_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    Rect_Load(self + 0xC, archive, g_albumNames[1]);
    Rect_Load(self + 0x24, archive, g_albumNames[2]);
    Rect_Load(self + 0x54, archive, g_albumNames[4]);
    Rect_Load(self + 0x6C, archive, g_albumNames[5]);
    TextStyle_Load(self + 0x84, archive, g_albumNames[6]);
    TextStyle_Load(self + 0xBC, archive, g_albumNames[7]);
    TextStyle_Load(self + 0xA0, archive, g_albumNames[8]);
    TextStyle_Load(self + 0x114, archive, g_albumNames[0xC]);
    At<uint8_t>(self, 0x12E) = 1;
    TextStyle_Load(self + 0x13C, archive, g_albumNames[0xD]);
    Position_Load(self + 0xD8, archive, g_albumNames[9]);
    Position_Load(self + 0xE4, archive, g_albumNames[0xA]);
    Position_Load(self + 0xF0, archive, g_albumNames[0xB]);
    Position_Load(self + 0x130, archive, g_albumNames[0xE]);
    Position_Load(self + 0x158, archive, g_albumNames[0xF]);
    Rect_Load(self + 0x3C, archive, g_albumNames[3]);
    Position_Load(self + 0xFC, archive, g_albumNames[0x10]);
    Position_Load(self + 0x108, archive, g_albumNames[0x11]);
}

// yes/no answered by accept (the album's questions): up/down move it
static void AlbumYesNoKeys(uint8_t* self) {
    if (g_menuDown != 0 && FE_PadPressed(0) != 0) {
        FE_PlaySelect();
        At<uint8_t>(self, 0x168) = 1;
        return;
    }
    if (g_menuUp != 0 && FE_PadPressed(0) != 0) {
        FE_PlaySelect();
        At<uint8_t>(self, 0x168) = 0;
    }
}

// 0x6988B0 (vf5)
void __fastcall PhotoAlbum_Process(uint8_t* self, re::Edx) {
    int32_t state = SaveSys_GetState();
    int32_t count = g_photoCount;
    if (state == 0 || state == 5 || state == 0x19) {
        // retry the card?
        if (Input_BackPressed(kUserInput, 0) != 0 ||
            (Input_AcceptDown(kUserInput, 0) != 0 && At<uint8_t>(self, 0x168) == 1)) {
            FE_PlaySelect();
            g_menuActive = 0;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0 && At<uint8_t>(self, 0x168) == 0) {
            FE_PlaySelect();
            if (SaveSys_736E70(1) == 0) {
                VCall<void>(self, 0x10);
                VCall<void>(self, 0xC);
            }
            return;
        }
        AlbumYesNoKeys(self);
        return;
    }
    int32_t mode = At<int32_t>(self, 0x164);
    if (mode == 0) {
        if (Input_BackPressed(kUserInput, 0) != 0) {
            FE_PlaySelect();
            g_menuActive = 0;
            return;
        }
        if (count <= 0) {
            return;
        }
        if (Pad_ButtonJustDown(9, 0) != 0) {
            FE_PlaySelect();
            At<int32_t>(self, 0x164) = 1;
            return;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0) {
            FE_PlaySelect();
            if (At<int32_t>(self, 0x170) != At<int32_t>(self, 0x174)) {
                At<int32_t>(self, 0x170) = At<int32_t>(self, 0x174);
                At<int32_t>(self, 0x16C) = 2;
                At<uint8_t>(self, 0x169) = 1;
            }
            return;
        }
        if (g_menuLeft != 0 && FE_PadPressed(0) != 0) {
            At<int32_t>(self, 0x174) -= 1;
            if (At<int32_t>(self, 0x174) < 0) {
                At<int32_t>(self, 0x174) += 1;
            } else {
                FE_PadClear(0);
            }
            if (At<int32_t>(self, 0x174) < At<int32_t>(self, 0x178)) {
                At<int32_t>(self, 0x16C) = 2;
            }
            return;
        }
        if (g_menuRight != 0 && FE_PadPressed(0) != 0) {
            At<int32_t>(self, 0x174) += 1;
            if (At<int32_t>(self, 0x174) < count) {
                FE_PadClear(0);
            } else {
                At<int32_t>(self, 0x174) -= 1;
            }
            int32_t last = count < 5 ? count - 1 : At<int32_t>(self, 0x178) + 4;
            if (last < At<int32_t>(self, 0x174)) {
                At<int32_t>(self, 0x16C) = 2;
            }
        }
        return;
    }
    if (mode == 2) {
        // deleted: accept goes on
        if (Input_AcceptDown(kUserInput, 0) == 0) {
            return;
        }
    } else if (mode == 1) {
        // delete it?
        if (Input_BackPressed(kUserInput, 0) != 0 ||
            (Input_AcceptDown(kUserInput, 0) != 0 && At<uint8_t>(self, 0x168) == 1)) {
            FE_PlaySelect();
            At<uint8_t>(self, 0x168) = 1;
            At<int32_t>(self, 0x164) = 0;
        }
        if (Input_AcceptDown(kUserInput, 0) == 0 || At<uint8_t>(self, 0x168) != 0) {
            AlbumYesNoKeys(self);
            return;
        }
        FE_PlaySelect();
        if (At<uint8_t>(self, 0x168) == 0) {
            At<int32_t>(self, 0x164) = 0;
            At<int32_t>(self, 0x16C) = 3;
            At<uint8_t>(self, 0x169) = 1;
            At<uint8_t>(self, 0x168) = 1;
            return;
        }
    } else {
        return;
    }
    At<int32_t>(self, 0x164) = 0;
    At<uint8_t>(self, 0x168) = 1;
}

// 0x698BF0 / 0x698C60: the right / left arrow (faint when dimmed)
void __fastcall PhotoAlbum_DrawRightArrow(uint8_t* self, re::Edx, int dimmed) {
    DrawArrow(self, 0x24, 3, re::Flag(dimmed));
}
void __fastcall PhotoAlbum_DrawLeftArrow(uint8_t* self, re::Edx, int dimmed) {
    DrawArrow(self, 0xC, 2, re::Flag(dimmed));
}

// 0x698CD0: the album: the photo (not while asking), "n of m", the thumbnails (the
// cursor's framed), the dialogs; no photos: a message
void __fastcall PhotoAlbum_DrawAlbum(uint8_t* self, re::Edx) {
    int32_t count = g_photoCount;
    int32_t mode = At<int32_t>(self, 0x164);
    if (count <= 0 && mode != 2) {
        PhotoAlbum_DrawRightArrow(self, nullptr, 1);
        PhotoAlbum_DrawLeftArrow(self, nullptr, 1);
        FE_DrawMessageStyled(self + 0x158, "FEPA_NOPICS", self + 0x13C);
        FE_DrawTitle("FEPA_CONTROLS_N");
        return;
    }
    bool asking = mode == 1 || mode == 2;
    PhotoAlbum_DrawRightArrow(self, nullptr, asking ? 1 : 0);
    // the photo's rect: the left arrow's, moved and shrunk
    uint8_t rect[0x18];
    At<int32_t>(rect, 0) = At<int32_t>(self, 0xC);
    At<float>(rect, 4) = float(double(At<float>(self, 0x10)) + 5.0);
    At<float>(rect, 8) = float(double(At<float>(self, 0x14)) - 4.0);
    At<float>(rect, 0xC) = float(double(At<float>(self, 0x18)) * 0.9599999785423279);
    At<float>(rect, 0x10) = float(double(At<float>(self, 0x1C)) * 0.9599999785423279);
    At<int32_t>(rect, 0x14) = At<int32_t>(self, 0x20);
    if (!asking) {
        float uv2[2] = {1.0f, 1.0f};
        float uv1[2] = {0.0f, 0.0f};
        FE_DrawRectSprite(rect, Album_GetTexture(kAlbum), kSelectedColour, uv1, uv2);
        PhotoAlbum_DrawLeftArrow(self, nullptr, 0);
    }
    int32_t shown = count > 0 ? At<int32_t>(self, 0x174) + 1 : 0;
    Font_AddNumber(float(shown));
    Font_AddNumber(float(count));
    uint16_t text[0x18];
    Font_Format(text, Text_Get(kTheText, g_albumNames[0]), 0x18);
    UseMenuDrop();
    FE_DrawText(0x140, 0x130, text, 3, At<int32_t>(self, 0x84), At<int32_t>(self, 0x88), 1);
    int32_t thumbs = g_thumbCount;
    for (int i = 0; i < thumbs; i++) {
        float scaleX = g_screenScaleX;
        uint8_t thumb[0x18];
        for (int j = 0; j < 6; j++) {
            At<int32_t>(thumb, j * 4) = At<int32_t>(self, 0x3C + j * 4);
        }
        float width = Rect_GetWidth(self + 0x24);
        float offset = float(i) / 5.0f * width;
        float x = Rect_GetX(self + 0x24);
        bool cursor = At<int32_t>(self, 0x178) + i == At<int32_t>(self, 0x174);
        At<float>(thumb, 4) = scaleX * 8.0f + x + offset;
        float uv2[2] = {1.0f, 1.0f};
        float uv1[2] = {0.0f, 0.0f};
        FE_DrawRectSprite(thumb, Thumb_GetTexture(kThumbs + i * 0xC), cursor ? kSelectedColour : kThumbColour, uv1,
                          uv2);
        if (cursor) {
            uint8_t inner[4];
            uint8_t border[4];
            RGBA_Construct(inner, 0, 0, 0, 0);
            RGBA_Construct(border, 255, 255, 255, 0x80);
            float left = Rect_GetX(thumb);
            float right = Rect_GetWidth(thumb) + left;
            float top = Rect_GetY(thumb);
            float bottom = top + Rect_GetHeight(thumb);
            FE_DrawFrameRect(left, top, right, bottom, 2.0f, inner, border);
        }
    }
    if (mode == 1) {
        g_fontWrapX = 400.0f;
        FE_DrawMessageStyled(self + 0x130, "CNFRM_DEL", self + 0xBC);
        DrawYesNo(self, float(double(g_lastTextRect[3]) + 10.0), At<uint8_t>(self, 0x168) == 1, 0xA0, 0xBC);
        FE_DrawTitle("MC_YESNO");
        return;
    }
    if (mode == 2) {
        FE_DrawMessageStyled(self + 0x130, "SUCCES_DELETE", self + 0x114);
        FE_DrawTitle("FE_CONTINUE");
        return;
    }
    FE_DrawTitle("FEPA_CONTROLS_D");
}

// 0x699200: both arrows faint, "writing" / "deleting" while a job runs
void __fastcall PhotoAlbum_DrawBusy(uint8_t* self, re::Edx) {
    DrawArrow(self, 0x24, 3, true);
    DrawArrow(self, 0xC, 2, true);
    int32_t job = At<int32_t>(self, 0x16C);
    if (job > 0) {
        if (job < 3) {
            FE_DrawMessageNumbers(self + 0x130, "FELD_WR", self + 0x114);
        } else if (job == 3) {
            FE_DrawMessageNumbers(self + 0x130, "FEDL_WR", self + 0x114);
        }
    }
}

// 0x699320: the card's message and retry?
void __fastcall PhotoAlbum_DrawRetry(uint8_t* self, re::Edx) {
    DrawArrow(self, 0x24, 3, true);
    DrawArrow(self, 0xC, 2, true);
    SaveSys_DrawMessage(self + 0x130, self + 0x114);
    g_fontWrapX = 400.0f;
    FE_DrawKeyStyled(0x140, int(float(double(g_lastTextRect[3]) + 30.0)), "RETRY_Q", self + 0xBC);
    DrawYesNo(self, float(double(g_lastTextRect[3]) + 10.0), At<uint8_t>(self, 0x168) == 1, 0xA0, 0xBC);
    FE_DrawTitle("MC_YESNO");
}

// 0x699540: job 1: the album's count again (the first page loaded, the current photo shown)
uint8_t __fastcall PhotoAlbum_Refresh(uint8_t* self, re::Edx) {
    uint8_t done = 0;
    if (Album_RefreshCount(kAlbum, 1) == 0) {
        return done;
    }
    if (g_photoCount == 0) {
        return 1;
    }
    PhotoAlbum_DrawBusy(self, nullptr);
    if (Album_LoadThumbs(kAlbum, 0) != 0) {
        done = Album_Select(kAlbum, 0);
    }
    PhotoAlbum_DrawBusy(self, nullptr);
    return done;
}

// 0x6995A0: job 2: the page the cursor went off (the cursor stepped back one, the first
// thumbnail by 4, within the album), then the photo shown
uint8_t __fastcall PhotoAlbum_LoadPage(uint8_t* self, re::Edx) {
    PhotoAlbum_DrawBusy(self, nullptr);
    if (At<uint8_t>(self, 0x169) != 0) {
        At<uint8_t>(self, 0x169) = 0;
        return 0;
    }
    uint8_t ok = 1;
    int32_t count = g_photoCount;
    int32_t last = count < 5 ? count - 1 : At<int32_t>(self, 0x178) + 4;
    int32_t cursor = At<int32_t>(self, 0x174);
    bool page = false;
    float first = 0.0f;
    if (cursor > last) {
        At<int32_t>(self, 0x174) = cursor - 1;
        float next = float(At<int32_t>(self, 0x178) + 4);
        float limit = float(count - 5);
        first = limit <= next ? limit : next;
        page = true;
    } else if (cursor < At<int32_t>(self, 0x178)) {
        float previous = float(At<int32_t>(self, 0x178) - 4);
        At<int32_t>(self, 0x174) = cursor + 1;
        first = previous <= 0.0f ? 0.0f : previous;
        page = true;
    }
    if (page) {
        At<int32_t>(self, 0x178) = int32_t(first);
        ok = Album_LoadThumbs(kAlbum, int32_t(first));
        if (ok == 0) {
            return 0;
        }
    }
    Album_Select(kAlbum, At<int32_t>(self, 0x170));
    return ok;
}

// 0x6996A0: job 3: the cursor's photo deleted (failing: the album cleared, entered again),
// the page and photo again, "deleted"
uint8_t __fastcall PhotoAlbum_DeletePhoto(uint8_t* self, re::Edx) {
    PhotoAlbum_DrawBusy(self, nullptr);
    if (At<uint8_t>(self, 0x169) != 0) {
        At<uint8_t>(self, 0x169) = 0;
        return 0;
    }
    uint8_t ok = Album_Delete(kAlbum, At<int32_t>(self, 0x174));
    if (ok == 0) {
        Album_Clear(kAlbum);
        VCall<void>(self, 0xC);
        return 0;
    }
    int32_t cursor = At<int32_t>(self, 0x174);
    if (cursor >= g_photoCount && cursor != 0) {
        At<int32_t>(self, 0x174) = cursor - 1;
    }
    At<int32_t>(self, 0x170) = g_albumCurrent;
    int32_t count = g_photoCount;
    if (count < 5) {
        Album_ClearThumbs(kAlbum);
        if (count != 0) {
            ok = Album_LoadThumbs(kAlbum, 0);
        }
    } else {
        if (At<int32_t>(self, 0x178) + 4 >= count) {
            At<int32_t>(self, 0x178) -= 1;
        }
        Album_ClearThumbs(kAlbum);
        ok = Album_LoadThumbs(kAlbum, At<int32_t>(self, 0x178));
    }
    if (ok == 1) {
        At<int32_t>(self, 0x164) = 2;
        if (count != 0) {
            Album_Select(kAlbum, At<int32_t>(self, 0x170));
        }
    }
    At<int32_t>(g_uberCompletion, 0x274) = 3;
    return ok;
}

// 0x6997A0: the pending job run; done: none pending
void __fastcall PhotoAlbum_RunJob(uint8_t* self, re::Edx) {
    uint8_t done;
    switch (At<int32_t>(self, 0x16C)) {
    case 1:
        done = PhotoAlbum_Refresh(self, nullptr);
        break;
    case 2:
        done = PhotoAlbum_LoadPage(self, nullptr);
        break;
    case 3:
        done = PhotoAlbum_DeletePhoto(self, nullptr);
        break;
    default:
        return;
    }
    if (done != 0) {
        At<int32_t>(self, 0x16C) = 0;
    }
}

// 0x6997E0 (vf6): by the save system's state: the album, a job running, or the card's
// retry (the album reset)
void __fastcall PhotoAlbum_Draw(uint8_t* self, re::Edx) {
    int32_t state = SaveSys_GetState();
    if (g_albumLastState != state) {
        g_albumLastState = state;
        At<uint8_t>(self, 0x168) = 1;
    }
    if (state != 0) {
        if (state == 5) {
            if (SaveSys_7368C0(1) != 0) {
                VCall<void>(self, 0x10);
                VCall<void>(self, 0xC);
                return;
            }
        } else if (state != 0x19) {
            if (At<int32_t>(self, 0x16C) == 0) {
                PhotoAlbum_DrawAlbum(self, nullptr);
                return;
            }
            FE_DrawTitle(nullptr);
            PhotoAlbum_RunJob(self, nullptr);
            return;
        }
    }
    Album_Clear(kAlbum);
    At<int32_t>(self, 0x170) = 0;
    At<int32_t>(self, 0x174) = 0;
    At<int32_t>(self, 0x178) = 0;
    At<int32_t>(self, 0x164) = 0;
    At<int32_t>(self, 0x16C) = 1;
    At<uint8_t>(g_uberCompletion, 0x269) = 1;
    PhotoAlbum_DrawRetry(self, nullptr);
}

// ---- FEQuitGame / FERestart ------------------------------------------------------------------

// 0x6998A0 (not in the function list)
void* __fastcall QuitGame_Construct(void* self, re::Edx) {
    SetVtable(self, VT_FEQuitGame);
    return self;
}

// 0x6998C0 (not in the function list)
void* __fastcall Restart_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FERestart);
    InitStyle(self + 0x10);
    InitStyle(self + 0x2C);
    InitStyle(self + 0x48);
    At<int32_t>(self, 0x64) = 0;
    At<float>(self, 0x68) = 0.0f;
    At<float>(self, 0x6C) = 0.0f;
    return self;
}

// 0x699950 (vf1)
void __fastcall Restart_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    TextStyle_Load(self + 0x2C, archive, g_restartStyleNames[0]);
    TextStyle_Load(self + 0x10, archive, g_restartStyleNames[1]);
    At<uint8_t>(self, 0x2A) = 1;
    TextStyle_Load(self + 0x48, archive, g_restartStyleNames[2]);
    Position_Load(self + 0x64, archive, g_restartStyleNames[3]);
}

// 0x6999B0 (vf5): yes / no; accept on yes returns to the main menu
void __fastcall Restart_Process(uint8_t* self, re::Edx) {
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
        if (At<int32_t>(self, 0xC) > 1) {
            At<int32_t>(self, 0xC) = 1;
        } else {
            moved = true;
        }
    }
    if (moved) {
        FE_PadClear(0);
    }
    if (Input_AcceptDown(kUserInput, 0) != 0) {
        FE_PlaySelect();
        g_menuActive = 0;
        if (At<int32_t>(self, 0xC) == 0) {
            g_exitingToMenu = 1;
            FE_ExitToMainMenu();
        }
    }
}

// 0x699A60 (vf6): the header, the message, yes / no
void __fastcall Restart_Draw(uint8_t* self, re::Edx) {
    UseMenuDrop();
    FE_DrawTextKey(0x140, 87, "FEM_RESTART_HEADER", 8, 2, 1, 1);
    g_menuFlagsByte &= 0xEF;
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    g_fontWrapX = 400.0f;
    FE_DrawMessageStyled(self + 0x64, "MEM_RSTRT", self + 0x10);
    DrawYesNo(self, float(double(g_lastTextRect[3]) + 10.0), At<int32_t>(self, 0xC) == 1, 0x10, 0x2C);
    FE_DrawTitle("FER_CONTROLS");
}

// ---- FESave ------------------------------------------------------------------------------------

// 0x699C20 (not in the function list)
void* __fastcall Save_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FESave);
    InitStyle(self + 0x34);
    InitStyle(self + 0x50);
    InitStyle(self + 0x6C);
    At<int32_t>(self, 0x88) = 0;
    At<float>(self, 0x8C) = 0.0f;
    At<float>(self, 0x90) = 0.0f;
    return self;
}

// 0x699CC0 (vf1)
void __fastcall Save_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    VCall<void>(archive, 0x38, self + 0xC, "Filetable XPos");
    VCall<void>(archive, 0x38, self + 0x10, "Filetable Right XPos ");
    VCall<void>(archive, 0x38, self + 0x14, "Filetable YPos");
    VCall<void>(archive, 0x38, self + 0x18, "Filetable Spacing");
    TextStyle_Load(self + 0x50, archive, g_saveStyleNames[0]);
    TextStyle_Load(self + 0x34, archive, g_saveStyleNames[1]);
    TextStyle_Load(self + 0x6C, archive, g_saveStyleNames[2]);
    Position_Load(self + 0x88, archive, g_saveStyleNames[3]);
}

// 0x699D60 (vf3)
void __fastcall Save_OnEnter(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x24) = 0;
    At<int32_t>(self, 0x28) = 0;
    At<int32_t>(self, 0x2C) = 1;
    At<uint8_t>(self, 0x1C) = 1;
    At<float>(self, 0x30) = Game_GetCompletion();
}

// yes/no by down/up (the save's questions); true: moved
static void SaveYesNoKeys(uint8_t* self) {
    if (g_menuDown != 0 && FE_PadPressed(0) != 0) {
        At<int32_t>(self, 0x2C) += 1;
        if (At<int32_t>(self, 0x2C) > 1) {
            At<int32_t>(self, 0x2C) -= 1;
            return;
        }
    } else {
        if (g_menuUp == 0 || FE_PadPressed(0) == 0) {
            return;
        }
        At<int32_t>(self, 0x2C) -= 1;
        if (At<int32_t>(self, 0x2C) < 0) {
            At<int32_t>(self, 0x2C) += 1;
            return;
        }
    }
    FE_PadClear(0);
}

// 0x699D80 (vf5)
void __fastcall Save_Process(uint8_t* self, re::Edx) {
    if (g_uberCompletion == nullptr) {
        SaveSys_Init();
    }
    int32_t state = At<int32_t>(self, 0x28);
    if (state == 0) {
        if (Input_BackPressed(kUserInput, 0) != 0) {
            FE_PlaySelect();
            FE_ExitToMainMenu();
            return;
        }
        if (Pad_ButtonJustDown(6, 0) != 0) {
            FE_PlaySelect();
            return;
        }
        int32_t slot;
        if (Pad_ButtonJustDown(9, 0) != 0) {
            // (delete: not on the frame the screen was entered)
            if (At<uint8_t>(self, 0x1C) != 0) {
                FE_PlaySoundId(4);
                return;
            }
            slot = At<int32_t>(self, 0x24);
        } else {
            if (Input_AcceptDown(kUserInput, 0) != 0) {
                const char* info = SaveSys_GetSlotInfo(At<int32_t>(self, 0x24));
                FE_PlaySelect();
                At<int32_t>(self, 0x28) = *info == 0 ? 1 : 2;
                At<int32_t>(self, 0x2C) = 1;
                return;
            }
            if (Pad_ButtonJustDown(9, 0) == 0) {
                if (g_menuUp != 0 && FE_PadPressed(0) != 0) {
                    At<int32_t>(self, 0x24) -= 1;
                    if (At<int32_t>(self, 0x24) < 0) {
                        At<int32_t>(self, 0x24) = 5;
                    }
                } else {
                    if (g_menuDown == 0 || FE_PadPressed(0) == 0) {
                        return;
                    }
                    At<int32_t>(self, 0x24) += 1;
                    if (At<int32_t>(self, 0x24) > 5) {
                        At<int32_t>(self, 0x24) = 0;
                    }
                }
                FE_PadClear(0);
                return;
            }
            slot = At<int32_t>(self, 0x24);
        }
        const char* info = SaveSys_GetSlotInfo(slot);
        FE_PlaySelect();
        if (*info != 0) {
            At<int32_t>(self, 0x28) = 7;
            At<int32_t>(self, 0x2C) = 1;
        }
        return;
    }
    if (state == 4) {
        // retry?
        if (Input_BackPressed(kUserInput, 0) != 0) {
            FE_PlaySelect();
            FE_ExitToMainMenu();
            return;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0) {
            FE_PlaySelect();
            if (At<int32_t>(self, 0x2C) == 1) {
                FE_ExitToMainMenu();
                return;
            }
            if (SaveSys_736E70(0) != 0) {
                return;
            }
            At<int32_t>(self, 0x28) = 0;
            At<int32_t>(self, 0x2C) = 1;
            At<int32_t>(g_uberCompletion, 0x264) = 6;
            return;
        }
        if (g_menuUp != 0 && FE_PadPressed(0) != 0) {
            At<int32_t>(self, 0x2C) = 0;
            FE_PlaySelect();
            return;
        }
        if (g_menuDown != 0 && FE_PadPressed(0) != 0) {
            At<int32_t>(self, 0x2C) = 1;
            FE_PlaySelect();
        }
        return;
    }
    if (state == 6) {
        // recreate?
        if (Input_BackPressed(kUserInput, 0) != 0) {
            FE_PlaySelect();
            FE_ExitToMainMenu();
            return;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0) {
            FE_PlaySelect();
            if (At<int32_t>(self, 0x2C) != 1) {
                SaveSys_Start(At<int32_t>(self, 0x24));
                At<int32_t>(self, 0x2C) = 1;
                return;
            }
            FE_ExitToMainMenu();
            return;
        }
        if (g_menuUp != 0 && FE_PadPressed(0) != 0) {
            FE_PlaySelect();
            At<int32_t>(self, 0x2C) = 0;
            return;
        }
        if (g_menuDown != 0 && FE_PadPressed(0) != 0) {
            FE_PlaySelect();
            At<int32_t>(self, 0x2C) = 1;
        }
        return;
    }
    if (state != 1 && state != 2 && state != 3 && state != 7) {
        if (state != 5) {
            return;
        }
        // done: accept goes on
        if (SaveSys_GetState() == 0x16) {
            if (Input_AcceptDown(kUserInput, 0) == 0) {
                return;
            }
            FE_PlaySelect();
            At<int32_t>(g_uberCompletion, 0x264) = 6;
            FE_ExitToMainMenu();
            return;
        }
        if (SaveSys_GetState() != 0x17) {
            return;
        }
        if (Input_AcceptDown(kUserInput, 0) == 0) {
            return;
        }
        FE_PlaySelect();
        At<int32_t>(self, 0x28) = 0;
        At<int32_t>(g_uberCompletion, 0x264) = 6;
        return;
    }
    // save / overwrite / format / delete?
    if (Input_BackPressed(kUserInput, 0) != 0) {
        FE_PlaySelect();
        if (At<int32_t>(self, 0x28) == 3) {
            FE_ExitToMainMenu();
        }
        At<int32_t>(self, 0x2C) = 1;
        At<int32_t>(self, 0x28) = 0;
        return;
    }
    if (Input_AcceptDown(kUserInput, 0) != 0) {
        FE_PlaySelect();
        int32_t question = At<int32_t>(self, 0x28);
        if (question == 1 || question == 2) {
            if (At<int32_t>(self, 0x2C) == 0) {
                SaveSys_Start(At<int32_t>(self, 0x24));
                if (Uber_736480(g_uberCompletion, 0) == 0) {
                    Uber_7364A0(g_uberCompletion, 1);
                }
            }
        } else if (question == 7 && At<int32_t>(self, 0x2C) == 0) {
            At<int32_t>(g_uberCompletion, 0x264) = 9;
            SaveSys_Start(At<int32_t>(self, 0x24));
            At<int32_t>(self, 0x2C) = 1;
            At<int32_t>(self, 0x28) = 0;
            return;
        }
        At<int32_t>(self, 0x2C) = 1;
        At<int32_t>(self, 0x28) = 0;
        return;
    }
    SaveYesNoKeys(self);
}

// 0x69A220: the state's question at y, yes / no below
void __fastcall Save_DrawQuestion(uint8_t* self, re::Edx, float y) {
    g_fontWrapX = 400.0f;
    const char* question;
    switch (At<int32_t>(self, 0x28)) {
    case 1:
        question = "FESZ_QS";
        break;
    case 2:
        question = "FESZ_QO";
        break;
    case 3:
        question = "FORMAT_Q";
        break;
    case 6:
        question = "RECRE_Q";
        break;
    case 7:
        question = "FESZ_QD";
        break;
    default:
        question = "RETRY_Q";
        break;
    }
    FE_DrawKeyStyled(0x140, int(y), question, self + 0x50);
    float yesY = float(double(g_lastTextRect[3]) + 10.0);
    g_fontWrapX = 400.0f;
    DrawYesNo(self, yesY, At<int32_t>(self, 0x2C) == 1, 0x34, 0x50);
}

// 0x69A3C0: the six slots (as FELoad's, the row spacing 15, 17.5 for language 7)
void __fastcall Save_DrawFiles(uint8_t* self, re::Edx) {
    static const char* const kSlots[6] = {"FEM_SL1", "FEM_SL2", "FEM_SL3", "FEM_SL4", "FEM_SL5", "FEM_SL6"};
    float spacing = 15.0f;
    if (g_language == 7) {
        spacing = 17.5f;
    }
    float scaleY = float(double(g_screenScaleY) * 0.5);
    for (int slot = 0; slot < 6; slot++) {
        const char* info = SaveSys_GetSlotInfo(slot);
        int32_t colour = At<int32_t>(self, 0x64);
        float y = float(At<int32_t>(self, 0x18) * slot + At<int32_t>(self, 0x14));
        if (slot == At<int32_t>(self, 0x24)) {
            colour = At<int32_t>(self, 0x48);
        }
        if (*info != 0) {
            FE_DrawSaveInfo(info, float(At<int32_t>(self, 0xC)), float(At<int32_t>(self, 0x10)), y, y + spacing,
                            colour, 0.39f, scaleY, colour == At<int32_t>(self, 0x48));
            continue;
        }
        float rect[4] = {1e6f, -1e6f, -1e6f, 1e6f};
        Font_Reset();
        g_fontColour[0] = g_menuColours[colour * 4];
        g_fontColour[1] = g_menuColours[colour * 4 + 1];
        g_fontColour[2] = g_menuColours[colour * 4 + 2];
        g_fontColour[3] = g_menuColours[colour * 4 + 3];
        if (slot == At<int32_t>(self, 0x24)) {
            g_fontDropColour[0] = g_menuColours[0];
            g_fontDropColour[1] = g_menuColours[1];
            g_fontDropColour[2] = g_menuColours[2];
            g_fontDropColour[3] = g_menuColours[3];
            g_fontDrop = 1;
        } else {
            g_fontDrop = 0;
        }
        Font_SetJustify(0);
        Font_SetScale(0.39f, scaleY);
        float x = float(At<int32_t>(self, 0xC));
        Font_PrintString(x, y, Text_Get(kTheText, kSlots[slot]), 1);
        Font_GetExtent(rect, 0);
        FE_DrawHighlight(std::fabs(rect[2] - rect[0]) + x, float(At<int32_t>(self, 0x10)), y);
        Font_PrintString(x, y + spacing, Text_Get(kTheText, "FEM_NOT_PRESENT"), 1);
        Font_GetExtent(rect, 0);
        FE_DrawHighlight(std::fabs(rect[2] - rect[0]) + x, float(At<int32_t>(self, 0x10)), y + spacing);
    }
}

// 0x69A680 (vf6): by the save system's state
void __fastcall Save_Draw(uint8_t* self, re::Edx) {
    UseMenuDrop();
    FE_DrawTextKey(0x140, 87, "FEM_SAVE_HEADER", 8, 2, 1, 1);
    g_menuFlagsByte &= 0xEF;
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    if (At<uint8_t>(self, 0x1C) != 0) {
        At<uint8_t>(self, 0x1C) = 0;
    }
    if (g_uberCompletion == nullptr) {
        SaveSys_Init();
    }
    if (g_saveLastState != SaveSys_GetState()) {
        At<int32_t>(self, 0x2C) = 1;
        g_saveLastState = SaveSys_GetState();
    }
    g_fontWrapX = 400.0f;
    switch (SaveSys_GetState()) {
    case 1:
    case 0x19: {
        uint32_t needed = 0;
        uint8_t hasSpace = Uber_HasSpace(g_uberCompletion, 0x56F4, &needed, 1);
        uint8_t other = Uber_736480(g_uberCompletion, 0);
        if (SaveSys_GetState() != 5) {
            if (hasSpace != 0 || (SaveSys_GetState() != 0x19 && other != 0)) {
                SaveSys_735460();
                At<int32_t>(self, 0x24) = SaveSys_FirstUsedSlot(1);
                FE_DrawTitle(nullptr);
                return;
            }
            Font_AddNumber(float(needed));
            FE_DrawMessageNumbers(self + 0x88, "SAVE_FULL", self + 0x6C);
            Save_DrawQuestion(self, nullptr, float(double(g_lastTextRect[3]) + 10.0));
        }
        FE_DrawTitle("MC_YESNO");
        At<int32_t>(self, 0x28) = 4;
        return;
    }
    case 3:
    case 4:
    case 0x10:
    case 0x11:
    case 0x14:
        SaveSys_DrawMessage(self + 0x88, self + 0x6C);
        FE_DrawTitle(nullptr);
        return;
    case 6:
        FE_DrawTitle(nullptr);
        return;
    case 8:
        if (At<int32_t>(self, 0x28) == 4) {
            At<int32_t>(self, 0x28) = 0;
        }
        switch (At<int32_t>(self, 0x28)) {
        case 0:
            Save_DrawFiles(self, nullptr);
            FE_DrawTitle("FESA_CONTROLS");
            return;
        case 1:
        case 2:
        case 3:
        case 7:
            Save_DrawQuestion(self, nullptr, Position_GetY(self + 0x88));
            FE_DrawTitle("MC_YESNO");
            return;
        }
        return;
    case 0x16: {
        At<int32_t>(self, 0x28) = 5;
        const char* key = At<uint8_t>(g_uberCompletion, 0x278) == 1 ? "SUCCES_OVER" : "SUCCES_SAVE";
        FE_DrawKeyStyled(0x140, int(Position_GetY(self + 0x88)), key, self + 0x6C);
        uint8_t style[0x1C];
        for (int i = 0; i < 7; i++) {
            At<int32_t>(style, i * 4) = At<int32_t>(self, 0x6C + i * 4);
        }
        At<int32_t>(style, 4) = 1;
        Font_AddNumber(At<float>(self, 0x30));
        uint16_t text[0x80];
        Font_Format(text, Text_GetByHash(kTheText, g_hashCompletion), 0x7F);
        FE_DrawTextStyled(0x140, int(float(double(g_lastTextRect[3]) + 10.0)), text, style);
        FE_DrawTitle("FE_CONTINUE");
        return;
    }
    case 0x17:
        At<int32_t>(self, 0x28) = 5;
        FE_DrawMessageStyled(self + 0x88, "RECRE_S", self + 0x6C);
        FE_DrawTitle("FE_CONTINUE");
        return;
    default:
        At<int32_t>(self, 0x28) = 4;
        SaveSys_DrawMessage(self + 0x88, self + 0x6C);
        Save_DrawQuestion(self, nullptr, float(double(g_lastTextRect[3]) + 10.0));
        FE_DrawTitle("MC_YESNO");
        if (SaveSys_7368C0(0) != 0) {
            At<int32_t>(self, 0x2C) = 1;
            At<int32_t>(self, 0x28) = 0;
            At<int32_t>(g_uberCompletion, 0x264) = 6;
        }
        return;
    }
}

// 0x69AAC0 (not in the function list): FESaveAlgie's constructor (its text style at +0xC)
void* __fastcall SaveAlgie_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FESaveAlgie);
    InitStyle(self + 0xC);
    return self;
}

// 0x69AB00 (vf3): the menu closed, mission 1_05B looked up (the result unused)
void __fastcall SaveAlgie_OnEnter(void*, re::Edx) {
    g_specialEntitiesCreated = 0;
    g_menu20C32F6 = 1;
    g_menuActive = 0;
    uint32_t key;
    NameKey_Construct(&key, "1_05B");
    MissionMgr_Find(kMissionMgr, &key);
    NameKey_Destruct(&key);
}

} // namespace feps

#define FEPS_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(feps_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(feps_original_, __LINE__), \
             re::Mode::Original)

FEPS_REVERSED("FEPhotoAlbum::FEPhotoAlbum", 0x006984E0, &feps::PhotoAlbum_Construct);
FEPS_REVERSED("FEPhotoAlbum::Load", 0x006986F0, &feps::PhotoAlbum_Load);
FEPS_REVERSED("FEPhotoAlbum::Process", 0x006988B0, &feps::PhotoAlbum_Process);
FEPS_REVERSED("FEPhotoAlbum::DrawRightArrow", 0x00698BF0, &feps::PhotoAlbum_DrawRightArrow);
FEPS_REVERSED("FEPhotoAlbum::DrawLeftArrow", 0x00698C60, &feps::PhotoAlbum_DrawLeftArrow);
FEPS_REVERSED("FEPhotoAlbum::DrawAlbum", 0x00698CD0, &feps::PhotoAlbum_DrawAlbum);
FEPS_REVERSED("FEPhotoAlbum::DrawBusy", 0x00699200, &feps::PhotoAlbum_DrawBusy);
FEPS_REVERSED("FEPhotoAlbum::DrawRetry", 0x00699320, &feps::PhotoAlbum_DrawRetry);
FEPS_REVERSED("FEPhotoAlbum::Refresh", 0x00699540, &feps::PhotoAlbum_Refresh);
FEPS_REVERSED("FEPhotoAlbum::LoadPage", 0x006995A0, &feps::PhotoAlbum_LoadPage);
FEPS_REVERSED("FEPhotoAlbum::DeletePhoto", 0x006996A0, &feps::PhotoAlbum_DeletePhoto);
FEPS_REVERSED("FEPhotoAlbum::RunJob", 0x006997A0, &feps::PhotoAlbum_RunJob);
FEPS_REVERSED("FEPhotoAlbum::Draw", 0x006997E0, &feps::PhotoAlbum_Draw);
FEPS_REVERSED("FEQuitGame::FEQuitGame", 0x006998A0, &feps::QuitGame_Construct);
FEPS_REVERSED("FERestart::FERestart", 0x006998C0, &feps::Restart_Construct);
FEPS_REVERSED("FERestart::Load", 0x00699950, &feps::Restart_Load);
FEPS_REVERSED("FERestart::Process", 0x006999B0, &feps::Restart_Process);
FEPS_REVERSED("FERestart::Draw", 0x00699A60, &feps::Restart_Draw);
FEPS_REVERSED("FESave::FESave", 0x00699C20, &feps::Save_Construct);
FEPS_REVERSED("FESave::Load", 0x00699CC0, &feps::Save_Load);
FEPS_REVERSED("FESave::OnEnter", 0x00699D60, &feps::Save_OnEnter);
FEPS_REVERSED("FESave::Process", 0x00699D80, &feps::Save_Process);
FEPS_REVERSED("FESave::DrawQuestion", 0x0069A220, &feps::Save_DrawQuestion);
FEPS_REVERSED("FESave::DrawFiles", 0x0069A3C0, &feps::Save_DrawFiles);
FEPS_REVERSED("FESave::Draw", 0x0069A680, &feps::Save_Draw);
FEPS_REVERSED("FESaveAlgie::FESaveAlgie", 0x0069AAC0, &feps::SaveAlgie_Construct);
FEPS_REVERSED("FESaveAlgie::OnEnter", 0x0069AB00, &feps::SaveAlgie_OnEnter);
