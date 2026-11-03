// The HUD's detention meter (the trouble arc, its pulsing icon and the crime text), the
// diary page, and the health rings over peds and props. 0x53D6E0..0x53EE60 (their
// constructors, Setup and Reset are Hud.cpp's).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Hud.h"
#include "game/Ped.h"
#include "reversed.h"

namespace {

struct CVector2D {
    float x, y;
};
struct CVector {
    float x, y, z;
};

// Entity_Health: the layout's words are floats (Hud.h has them raw)
struct EntityHealth : HUDComponent {
    uint8_t noTarget;         // +0x20 the player's target not ringed
    uint8_t unknown21[3];
    float unknown24;          // +0x24 the rings' radii
    float unknown28;          // +0x28
    float unknown2C;          // +0x2C
    float unknown30;          // +0x30
    float height;             // +0x34 over the entity
    int32_t segments;         // +0x38
    CRGBA colours[2];         // +0x3C empty .. full
};
static_assert(offsetof(EntityHealth, segments) == 0x38);
static_assert(sizeof(EntityHealth) == sizeof(HUDEntityHealth));

// A Gamebryo transform (rotation rows, translation, scale)
struct NiTransform {
    float rotate[9];
    CVector translate;
    float scale;
};
static_assert(sizeof(NiTransform) == 0x34);

// memory, textures
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, void* object);
GAME_FN(void, __cdecl, PushStreamingMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopMemoryTag, 0x0052E710);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, Txd_LoadTxdFile, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_RemoveRef, 0x005F1160, int slot);
GAME_FN(void, __cdecl, Txd_RemoveTxd, 0x005F1830, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, FontStyle_Load, 0x00687BC0, int style);
GAME_FN(void, __cdecl, FontStyle_Unload, 0x00687D10, int style);
GAME_FN(void, __cdecl, HUD_SaveVisibility, 0x005141D0, int which);
GAME_FN(void, __cdecl, HUD_RestoreVisibility, 0x00514210, int which);
GAME_FN(void, __cdecl, CTimer_StartUserPause, 0x0045B810);
GAME_FN(void, __cdecl, CTimer_EndUserPause, 0x0045B820);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int button, int pad);
// drawing
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, DrawRing, 0x0052D390, const CVector2D* centre, float outer, float inner, const CRGBA* colour,
        int segments, float start, float end);
GAME_FN(void, __cdecl, RenderRing, 0x0052BFF0, const NiTransform* transform, CRGBA colour, float r1, float r2,
        float degrees, int segments, int centred);
GAME_FN(CRGBA*, __cdecl, LerpColour, 0x00551530, CRGBA* out, float t, CRGBA from, CRGBA to);
// text
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_AddString, 0x006880F0, const uint16_t* string);
GAME_FN(void, __cdecl, Font_CopyText, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
// the rest
GAME_FN(void, __cdecl, EmptyFunction, 0x0049A650, int);
GAME_FN(float, __cdecl, LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(int, __cdecl, CrimeFromName, 0x00488C10, const char* name);
GAME_FN(void, __thiscall, Punishment_SetCrime, 0x0044E3C0, void* system, int crime);
GAME_FN(void, __thiscall, HUDDetention_DrawText, 0x0053D6E0, HUDDetention* self, CRGBA colour, int tier);
GAME_FN(void, __thiscall, HUDDiary_DrawTitle, 0x0053E4A0, HUDDiary* self, const char* key);
GAME_FN(void, __thiscall, HUDDiary_DrawPages, 0x0053E5F0, HUDDiary* self, const char* left, const char* right);
GAME_FN(void, __thiscall, EntityHealth_DrawRings, 0x0053E8B0, EntityHealth* self, uint8_t* entity, int alpha);
GAME_FN(void, __thiscall, CBike_GetWheelPoints, 0x004BBAD0, void* bike, CVector* front, CVector* rear);
GAME_FN(CVector*, __cdecl, CVector_Add, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(float, __thiscall, CVector_NormaliseAndMag, 0x004294A0, CVector* v);
GAME_FN(uint8_t, __thiscall, CPed_IsSwimming, 0x00476D70, void* ped);
GAME_FN(float, __thiscall, CPropAnim_GetHealthFraction, 0x004EDD80, void* prop);
GAME_FN(uint8_t, __cdecl, Dodgeball_IsActive, 0x00683A30);
GAME_FN(uint8_t*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, int slot);
GAME_FN(uint8_t, __thiscall, CPed_Fn473FB0, 0x00473FB0, void* ped);
GAME_FN(uint8_t*, __thiscall, CPed_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, CTargetingSystem_GetTarget, 0x004D5AD0, void* system);

GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(CPed*, g_playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, g_punishment, 0x00C0F78C);     // +0x30, +0x34: the tiers' points, +0x38 the most
GAME_VAR(void*, g_detentionIcon, 0x00C66F4C);
GAME_VAR(float, g_twoPi, 0x00AB4530);
GAME_VAR(float, g_detentionTextRaise, 0x00CEFCA0);   // (0; 15 once drawn in language 7)
GAME_VAR(void*, g_camera, 0x00C3CCF4);
GAME_VAR(uint8_t, g_healthRingsOn, 0x00C18701);
GAME_VAR(int32_t, g_int_C3CD0C, 0x00C3CD0C);
GAME_VAR(uint8_t*, g_weaponHud, 0x00C674F0);
GAME_VAR(uint32_t*, g_ptrNodes, 0x00C0F788);        // the sector lists' nodes
GAME_ARRAY(uint8_t, g_sectors, 0x00C1B178);        // [36 * 36] of 5 lists (0x14)
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_ARRAY(uint8_t, g_fontDropColour, 0x0208B780);
GAME_VAR(float, g_fontWrapLeft, 0x0208B774);
GAME_VAR(float, g_fontScaleY, 0x0208B770);
GAME_VAR(int16_t, g_fontStyle, 0x0208B788);
GAME_VAR(int16_t, g_fontEdge, 0x0208B784);
GAME_ARRAY(int32_t, g_fontStyleLoaded, 0x0208B218);
GAME_ARRAY(float, g_fontStyleHeights, 0x00BA4E8C);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);

template <typename R, typename... A>
R VCall(HUDComponent* self, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(HUDComponent*, re::Arg<A>...)>(static_cast<void*>(self->vtable[slot]))(self, args...);
}

// ---- detention ------------------------------------------------------------------------

// 0x53D6E0: the crime's text under the meter, fading out over its last 3 seconds.
// (`tier` is passed but unused)
void __fastcall HUDDetention_DrawText_(HUDDetention* self, re::Edx, CRGBA colour, int) {
    float now = float(double(g_timeInMilliseconds) * double(0.001f));
    if (!(now < self->textEndTime)) {
        return;
    }
    CRGBA shadow;
    RGBA_Construct(&shadow, 0x10, 0x10, 0x10, 0x80);
    CRGBA drop = shadow;
    float left = self->textEndTime - float(double(g_timeInMilliseconds) * double(0.001f));
    if (!(0.0f < left)) {
        left = 0.0f;
    }
    float t = 1.0f - left / 3.0f;
    float a;
    if (t < 0.2f) {
        float u = t / 0.2f;
        a = u * u;
    } else {
        float u = float(double(t - 0.2f) / 0.7999999970197678);   // (1 - 0.2f, in double)
        a = 1.0f - u * u;
    }
    uint8_t alpha = uint8_t(int(float(colour.a) * a));
    drop.a = uint8_t(int(a * float(shadow.a)));
    Font_Begin();
    Font_SetJustify(1);
    g_fontColour[0] = colour.r;
    g_fontColour[1] = colour.g;
    g_fontColour[2] = colour.b;
    g_fontColour[3] = alpha;
    std::memcpy(g_fontDropColour, &drop, 4);
    Font_SetSizePreset(2);
    CVector2D pos;
    ScreenPosition(&pos, -40.0f, 120.0f, self->hAlign, self->vAlign);
    Font_AddString(Text_Get(kTheText, self->text));
    uint16_t text[0x80];
    Font_CopyText(text, Text_Get(kTheText, "HUD_PNSH_ADD"), 0x80);
    if (g_language == 7) {
        g_detentionTextRaise = 15.0f;
    }
    Font_PrintString(pos.x, g_detentionTextRaise + pos.y, text, 1);
    Font_Render();
    Punishment_SetCrime(g_punishment, CrimeFromName(self->text));
}

// 0x53D970 (vf8 Draw): the arc filled to the player's trouble points (pulsing while
// they're coming down, red at the top tier), the icon beside it
void __fastcall HUDDetention_Draw(HUDDetention* self, re::Edx) {
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    CRGBA from, red, unused, to, shadow, back;
    RGBA_Construct(&from, 0xEB, 0xEB, 0xEB, 0x80);
    RGBA_Construct(&red, 0x73, 0x20, 0x20, 0xFF);
    RGBA_Construct(&unused, 0xEB, 0xEB, 0xEB, 0xFF);
    RGBA_Construct(&to, 0x40, 0x40, 0x40, 0xFF);
    RGBA_Construct(&shadow, 0x10, 0x10, 0x10, 0x80);
    RGBA_Construct(&back, 0x32, 0x32, 0x32, 0x80);
    float iconX = sx * 40.0f;
    float iconY = sy * -40.0f;
    float iconW = sx * 20.0f;
    float iconH = sy * 20.0f;
    CPed* player = g_playerPed;
    int points = player->punishment;
    float fraction = float(double(points) / double(*reinterpret_cast<int32_t*>(g_punishment + 0x38)));
    if (fraction < 0.0f) {
        fraction = 0.0f;
    } else if (fraction > 1.0f) {
        fraction = 1.0f;
    }
    self->unknown70 = fraction;
    float angleNow = self->angleFrom;
    CRGBA colour;
    float oldPulse = self->unknown6C;
    float speed = fraction;
    if (player->unknown14A8 != 0) {
        speed = 1.0f;
    }
    self->unknown6C = speed * 30.0f + self->unknown6C;
    if (fraction != 0.0f) {
        EmptyFunction(0);
    }
    int tier;
    if (points > *reinterpret_cast<int32_t*>(g_punishment + 0x34)) {
        EmptyFunction(0);
        to = red;
        colour = self->colours[1];
        tier = 3;
    } else if (points > *reinterpret_cast<int32_t*>(g_punishment + 0x30)) {
        tier = 1;
        colour.r = uint8_t(int(float(self->colours[0].r + self->colours[1].r) * 0.5f));
        colour.g = uint8_t(int(float(self->colours[0].g + self->colours[1].g) * 0.5f));
        colour.b = uint8_t(int(float(self->colours[0].b + self->colours[1].b) * 0.5f));
        colour.a = uint8_t(int(0.5f * float(self->colours[0].a + self->colours[1].a)));
    } else {
        colour = self->colours[0];
        tier = 0;
        // (the pulse stops at the end of a cycle)
        if (player->unknown14A8 == 0 &&
            (oldPulse < 0.01f || float(double(g_twoPi) - double(0.01f)) < oldPulse)) {
            self->unknown6C = 0.0f;
        }
    }
    float s = float(std::sin(double(LimitRadianAngle(self->unknown6C * 3.14159274f / 180.0f))));
    angleNow = (self->angleTo - self->angleFrom) * self->unknown70 + angleNow;
    if (0.0f < self->unknown6C) {
        colour.a = uint8_t(int(float(colour.a) * s * 0.5f + float(colour.a)));
    }
    SetRenderState(1, 0);
    const auto* centre = reinterpret_cast<const CVector2D*>(&self->x);
    constexpr float kThreeDegrees = float(0.05235987901687622);
    int segments = self->unknown38;
    DrawRing(centre, self->height - 2.0f, self->width + 2.0f, &shadow, segments, self->angleFrom + kThreeDegrees,
             self->angleTo - kThreeDegrees);
    DrawRing(centre, self->height, self->width, &back, self->unknown38, angleNow, self->angleTo);
    DrawRing(centre, self->height, self->width, &colour, self->unknown38, self->angleFrom, angleNow);
    DrawRing(centre, self->height, self->width, &shadow, self->unknown38, 1.65f, 1.61f);
    DrawRing(centre, self->height, self->width, &shadow, self->unknown38, 2.25f, 2.21f);
    CRGBA icon;
    icon.r = uint8_t(int(float(to.r - from.r) * s + float(from.r)));
    icon.g = uint8_t(int(float(to.g - from.g) * s + float(from.g)));
    icon.b = uint8_t(int(float(to.b - from.b) * s + float(from.b)));
    icon.a = uint8_t(int(float(to.a - from.a) * s + float(from.a)));
    float scale = s * 0.6f + 1.0f;
    if (scale < 0.5f) {
        scale = 0.5f;
    }
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    float halfH = iconH * 0.5f * scale;
    float cy = self->y + iconY;
    float halfW = iconW * 0.5f * scale;
    float cx = iconX + self->x;
    DrawRectUV(cx - halfW, cy - halfH, cx + halfW, cy + halfH, g_detentionIcon, &icon, &uv1, &uv2);
    HUDDetention_DrawText(self, colour, tier);
}

// ---- the diary ----------------------------------------------------------------------------

// 0x53E230 (vf11 OnShow): its texture dictionary loaded, the game paused
void __fastcall HUDDiary_OnShow(HUDDiary* self, re::Edx) {
    PushStreamingMemoryTag();
    self->txd = Txd_FindTxdSlot("beadiary", 0);
    if (self->txd == -1) {
        self->txd = Txd_AddTxdSlot("beadiary");
    }
    Txd_LoadTxdFile(self->txd, "TXD/beadiary.NFT");
    Txd_AddRef(self->txd);
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(self->txd);
    self->texture = Txd_FindTexture("Bea_diary", nullptr);
    Txd_PopCurrentTxd();
    if (g_language != 7 && g_language != 6) {
        FontStyle_Load(3);
    }
    PopMemoryTag();
    HUD_SaveVisibility(0x2C);
    CTimer_StartUserPause();
}

// 0x53E2D0 (vf12 OnHide)
void __fastcall HUDDiary_OnHide(HUDDiary* self, re::Edx) {
    if (self->texture) {
        NiRelease(self->texture);
        self->texture = nullptr;
    }
    if (self->txd != -1) {
        Txd_RemoveRef(self->txd);
        Txd_RemoveTxd(self->txd);
        self->txd = -1;
    }
    if (g_language != 7 && g_language != 6) {
        FontStyle_Unload(3);
    }
    HUD_RestoreVisibility(0x2C);
    CTimer_EndUserPause();
}

// 0x53E4A0: the title at the bottom
void __fastcall HUDDiary_DrawTitle_(HUDDiary*, re::Edx, const char* key) {
    float y = (480.0f - 30.0f) - g_screenScaleY * 8.0f;
    Font_SetJustify(0);
    CRGBA white;
    RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
    std::memcpy(g_fontColour, &white, 4);
    if (g_fontStyleLoaded[0] != 0) {
        g_fontStyle = 0;
    }
    Font_SetSizePreset(3);
    g_fontEdge = 2;
    CRGBA black;
    RGBA_Construct(&black, 0, 0, 0, 0xFF);
    std::memcpy(g_fontDropColour, &black, 4);
    const uint16_t* text = Text_Get(kTheText, key);
    Font_GetStringWidth(text, 1);
    float lineHeight = g_fontStyleHeights[g_fontStyle] * g_fontScaleY;
    Font_PrintString(float(0.0 + 50.0), (y - lineHeight) + 0.0f, text, 1);
    Font_Render();
}

// 0x53E5F0: the two pages' text (ecx unused)
void __fastcall HUDDiary_DrawPages_(HUDDiary*, re::Edx, const char* left, const char* right) {
    Font_SetJustify(0);
    CRGBA grey;
    RGBA_Construct(&grey, 100, 100, 100, 0xFF);
    std::memcpy(g_fontColour, &grey, 4);
    if (g_language == 7 || g_language == 6) {
        if (g_fontStyleLoaded[0] != 0) {
            g_fontStyle = 0;
        }
    } else if (g_fontStyleLoaded[3] != 0) {
        g_fontStyle = 3;
        g_fontEdge = 0;
    }
    Font_SetSizePreset(1);
    g_fontWrapLeft = 320.0f;
    g_fontEdge = 0;
    const uint16_t* leftText = Text_Get(kTheText, left);
    const uint16_t* rightText = Text_Get(kTheText, right);
    Font_PrintString(50.0f, 100.0f, leftText, 1);
    g_fontWrapLeft = 640.0f;
    Font_PrintString(370.0f, 100.0f, rightText, 1);
    Font_Render();
}

// 0x53E710 (vf8 Draw): the page full screen; the back button closes it
void __fastcall HUDDiary_Draw(HUDDiary* self, re::Edx) {
    CTimer_StartUserPause();
    if (!self->texture) {
        return;
    }
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    CRGBA white;
    RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
    DrawRectUV(self->x, self->y, 640.0f, 480.0f, self->texture, &white, &uv1, &uv2);
    HUDDiary_DrawTitle(self, "1_G1_DIARY_BACK");
    HUDDiary_DrawPages(self, "1_G1_DIARYP1", "1_G1_DIARYP2");
    if (Pad_JustPressed(9, 0)) {
        VCall<void>(self, 5, 0);
    }
}

// ---- entity health ------------------------------------------------------------------------

// 0x53E8B0: the rings over an entity (a ped or a prop), facing the camera, filled to
// its health
void __fastcall EntityHealth_DrawRings_(EntityHealth* self, re::Edx, uint8_t* entity, int alpha) {
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x14);
    CVector pos = *reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : entity + 4);
    if ((entity[0x108] & 7) == 3) {
        uint8_t* vehicle = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x1554);
        if (vehicle && *reinterpret_cast<int32_t*>(vehicle + 0x46C) == 1) {
            // on a bike: over it, unless it's (nearly) upright on both
            float f634 = *reinterpret_cast<float*>(vehicle + 0x634);
            float f638 = *reinterpret_cast<float*>(vehicle + 0x638);
            bool flag = (vehicle[0x1BC] >> 4) & 1;
            bool below = 0.8f > f634 || (flag && 1.0f > f634) || f638 < 0.8f;
            if (!below) {
                if (!flag) {
                    return;
                }
                if (!(1.0f > f638)) {
                    return;
                }
            }
            CVector front, rear;
            CBike_GetWheelPoints(vehicle, &front, &rear);
            pos = rear;
        } else {
            // between the ped and its (animated) root, by +0x1E84 (the z isn't: the
            // game scales the ped's z twice)
            float t = *reinterpret_cast<float*>(entity + 0x1E84);
            uint8_t* node = *reinterpret_cast<re::Ptr32<uint8_t>*>(
                *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x114) + 0x14) + 8);
            CVector nodePos = *reinterpret_cast<const CVector*>(node + 0x8C);
            float u = 1.0f - t;
            CVector a = {u * pos.x, u * pos.y, pos.z * u};
            CVector b = {nodePos.x * t, nodePos.y * t, pos.z * t};
            pos = *CVector_Add(&nodePos, &b, &a);
        }
    }
    pos.z = self->height + pos.z;
    const uint8_t* cam = GetLocalTransform(GetCameraNode(g_camera));
    CVector camPos = *reinterpret_cast<const CVector*>(cam + 0x24);
    CVector d = {pos.x - camPos.x, pos.y - camPos.y, pos.z - camPos.z};
    // right = d x up, then up' = up x right (written out with the zero terms)
    CVector right;
    right.x = d.y - d.z * 0.0f;
    right.y = d.z * 0.0f - d.x;
    right.z = d.x * 0.0f - d.y * 0.0f;
    CVector_NormaliseAndMag(&right);
    CVector up;
    up.x = right.z * 0.0f - right.y;
    up.y = right.x - right.z * 0.0f;
    up.z = right.y * 0.0f - right.x * 0.0f;
    CVector_NormaliseAndMag(&up);
    NiTransform transform;
    transform.rotate[0] = up.x;
    transform.rotate[1] = right.x;
    transform.rotate[2] = 0.0f;
    transform.rotate[3] = up.y;
    transform.rotate[4] = right.y;
    transform.rotate[5] = 0.0f;
    transform.rotate[6] = up.z;
    transform.rotate[7] = right.z;
    transform.rotate[8] = 1.0f;
    transform.translate = {pos.x, pos.y, float(double(pos.z) + double(0.03f))};
    transform.scale = 1.0f;
    float health;
    uint8_t type = entity[0x108] & 7;
    if (type == 3) {
        health = *reinterpret_cast<float*>(entity + 0x1CB8) / *reinterpret_cast<float*>(entity + 0x1CA4);
        if (CPed_IsSwimming(entity)) {
            return;
        }
    } else if (type == 6) {
        health = CPropAnim_GetHealthFraction(entity);
    } else {
        return;
    }
    if (health < 0.0f || health == 0.0f) {
        return;
    }
    int degrees = int(health * 360.0f);
    CRGBA dark, black;
    RGBA_Construct(&dark, 0x32, 0x32, 0x32, alpha);
    RGBA_Construct(&black, 0, 0, 0, alpha);
    SetRenderState(8, 0);
    SetRenderState(6, 1);
    SetRenderState(10, 5);
    SetRenderState(11, 6);
    SetRenderState(12, 1);
    CRGBA lerped;
    const CRGBA* c = LerpColour(&lerped, health, self->colours[0], self->colours[1]);
    // doubled, at most 255 (through 64 bits, compared unsigned)
    auto twice = [](uint8_t v) {
        uint32_t x = uint32_t(int64_t(float(v) * 2.0f));
        return uint8_t(x > 0xFF ? 0xFF : x);
    };
    CRGBA bright;
    uint8_t r = c->r, g = c->g, b = c->b;
    bright.r = twice(r);
    bright.g = twice(g);
    bright.b = twice(b);
    bright.a = uint8_t(alpha);
    if (alpha != 0xFF) {
        bright.a = uint8_t(uint8_t(alpha) >> 1);
    }
    RenderRing(&transform, black, self->unknown24, self->unknown2C + self->unknown24, 360.0f, self->segments, 1);
    RenderRing(&transform, dark, self->unknown28, self->unknown24, 360.0f, self->segments, 1);
    RenderRing(&transform, bright, self->unknown28, self->unknown24, float(degrees), self->segments, 1);
    RenderRing(&transform, black, self->unknown28 - self->unknown2C, self->unknown28, 360.0f, self->segments, 1);
    SetRenderState(6, 0);
}

// a sector list's entities (Bully's packed nodes: pool, slot, next)
template <typename F>
void ForEachInList(uint32_t* node, F&& f) {
    while (node) {
        uint32_t v = *node;
        f(GetEntityInPool(v & 0xF, (v >> 4) & 0x3FFF));
        uint32_t next = *node >> 0x12;
        if (next == 0x3FFF) {
            break;
        }
        node = g_ptrNodes + next;
    }
}

// 0x53EE60 (vf8 Draw): rings over the peds fighting the player (or its target's), the
// damaged props around, and the player's target
void __fastcall EntityHealth_Draw(EntityHealth* self, re::Edx) {
    if (!g_healthRingsOn || g_int_C3CD0C != 0 || *reinterpret_cast<int32_t*>(g_weaponHud + 0x138) == 2 ||
        Dodgeball_IsActive()) {
        return;
    }
    CPed* player = g_playerPed;
    col::CRect bounds;
    reinterpret_cast<void(__thiscall*)(CPed*, col::CRect*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(player))[0x2C / 4]))(player,
                                                                                                          &bounds);
    int x0 = int((bounds.left - 5.0f) / 50.0f + 18.0f);
    int x1 = int((bounds.right + 5.0f) / 50.0f + 18.0f);
    int y1 = int((bounds.top + 5.0f) / 50.0f + 18.0f);
    int y0 = int((bounds.bottom - 5.0f) / 50.0f + 18.0f);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            uint8_t* sector = g_sectors + (y * 36 + x) * 0x14;
            ForEachInList(*reinterpret_cast<re::Ptr32<uint32_t>*>(sector + 0xC), [&](uint8_t* ped) {
                if (!ped || *reinterpret_cast<int32_t*>(ped + 0x1310) == 0xD ||
                    *reinterpret_cast<int32_t*>(ped + 0x1324) != 0 || *reinterpret_cast<int32_t*>(ped + 0x14A4) != 0) {
                    return;
                }
                if (*reinterpret_cast<int32_t*>(ped + 0x135C) != 0) {
                    EntityHealth_DrawRings(self, ped, 0x80);
                    return;
                }
                bool onPlayer;
                if (CPed_Fn473FB0(ped)) {
                    uint8_t* target = CPed_GetTarget(ped);
                    if (!target) {
                        onPlayer = player == nullptr;
                    } else {
                        onPlayer = ((target[0x108] & 7) == 3 ? target : nullptr) == reinterpret_cast<uint8_t*>(player);
                    }
                } else {
                    onPlayer = *reinterpret_cast<re::Ptr32<CPed>*>(ped + 0x155C) == player;
                }
                if (onPlayer) {
                    EntityHealth_DrawRings(self, ped, 0x40);
                }
            });
            // (no null check on these)
            ForEachInList(*reinterpret_cast<re::Ptr32<uint32_t>*>(sector + 4), [&](uint8_t* prop) {
                if ((prop[0x108] & 7) == 6 && *reinterpret_cast<int32_t*>(prop + 0x16C) != 0) {
                    EntityHealth_DrawRings(self, prop, 0x80);
                }
            });
        }
    }
    void* targeting = *reinterpret_cast<re::Ptr32<void>*>(reinterpret_cast<uint8_t*>(player) + 0x1C8);
    if (CTargetingSystem_GetTarget(targeting) && !self->noTarget) {
        uint8_t* target = CTargetingSystem_GetTarget(*reinterpret_cast<re::Ptr32<void>*>(reinterpret_cast<uint8_t*>(player) + 0x1C8));
        if (target && (target[0x108] & 7) == 3 && *reinterpret_cast<int32_t*>(target + 0x14A4) == 0) {
            EntityHealth_DrawRings(self, target, 0x80);
        }
    }
}

} // namespace

#define HUDDD_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(hudddd_original_, __LINE__);                                          \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudddd_original_, __LINE__), \
             re::Mode::Original)

HUDDD_REVERSED("HUDDetention::DrawText", 0x0053D6E0, &HUDDetention_DrawText_);
HUDDD_REVERSED("HUDDetention::Draw", 0x0053D970, &HUDDetention_Draw);
HUDDD_REVERSED("HUDDiary::OnShow", 0x0053E230, &HUDDiary_OnShow);
HUDDD_REVERSED("HUDDiary::OnHide", 0x0053E2D0, &HUDDiary_OnHide);
HUDDD_REVERSED("HUDDiary::DrawTitle", 0x0053E4A0, &HUDDiary_DrawTitle_);
HUDDD_REVERSED("HUDDiary::DrawPages", 0x0053E5F0, &HUDDiary_DrawPages_);
HUDDD_REVERSED("HUDDiary::Draw", 0x0053E710, &HUDDiary_Draw);
HUDDD_REVERSED("HUDEntityHealth::DrawRings", 0x0053E8B0, &EntityHealth_DrawRings_);
HUDDD_REVERSED("HUDEntityHealth::Draw", 0x0053EE60, &EntityHealth_Draw);
