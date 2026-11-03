// HUD2DSigns (0x5390D0..0x53A4A5): the close-up "sign" the player reads when using a
// bulletin board, the fortune teller or the globe: a textured quad that flies from the
// object (+0x24) to in front of the camera, with a sign object's text over it.
//
// The component (vtable 0x918FC8 for the destructor 0x539210):
//   +8 visible byte, +0x20 the target entity (the player's +0x438), +0x24 its position,
//   +0x30 the txd slot, +0x34 the texture, +0x38/+0x3C the quad's half width/height,
//   +0x40 the fly-in amount (0..1), +0x44 the bulletin board sign, +0xF8 the fortune
//   teller's, +0x108 the globe's, +0x118 the sign in use.
// A sign: vtable, +4 its fade in (0..1), +8 its text hash; the bulletin board's has its
// entries at +0xC (40), the count at +0xAC and the one shown at +0xB0; the cards' has the
// number of cards at +0xC.
//
// Found here: 0x5390E0 (set +4 and visible) and 0x5390F0 (reset) are small members
// without function starts, 0x539210 the destructor.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace hud2dsigns17 {

using col::CVector;

struct Sign {
    re::Ptr32<re::Ptr32<void>> vtable;   // +0
    float fade;      // +4
    uint32_t text;   // +8
};

struct BulletinSign : Sign {
    int32_t entries[40];  // +0xC
    int32_t count;        // +0xAC
    int32_t index;        // +0xB0
};
static_assert(offsetof(BulletinSign, count) == 0xAC);
static_assert(sizeof(BulletinSign) == 0xB4);

struct CardSign : Sign {
    int32_t numCards;  // +0xC
};
static_assert(sizeof(CardSign) == 0x10);

struct HUD2DSigns {
    re::Ptr32<re::Ptr32<void>> vtable;         // +0
    uint8_t unk4[4];       // +4
    uint8_t visible;       // +8
    uint8_t unk9[0x17];    // +9
    re::Ptr32<void> target;          // +0x20
    CVector pos;           // +0x24
    int32_t txd;           // +0x30
    re::Ptr32<void> texture;         // +0x34
    float width;           // +0x38
    float height;          // +0x3C
    float amount;          // +0x40
    BulletinSign board;    // +0x44
    CardSign fortune;      // +0xF8
    CardSign globe;        // +0x108
    re::Ptr32<Sign> sign;            // +0x118
};
static_assert(offsetof(HUD2DSigns, target) == 0x20);
static_assert(offsetof(HUD2DSigns, board) == 0x44);
static_assert(offsetof(HUD2DSigns, fortune) == 0xF8);
static_assert(offsetof(HUD2DSigns, globe) == 0x108);
static_assert(offsetof(HUD2DSigns, sign) == 0x118);

// Im3D vertex (0x24)
struct Im3DVertex {
    CVector pos;      // +0
    CVector normal;   // +0xC
    uint32_t color;   // +0x18
    float u, v;       // +0x1C
};
static_assert(sizeof(Im3DVertex) == 0x24);

struct NiTransform {
    float rotate[9];    // +0
    CVector translate;  // +0x24
    float scale;        // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

inline void* const bulletinBoard = reinterpret_cast<void*>(0x00BCCE80);
inline void* const theText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const GetTextureCB = reinterpret_cast<void*>(0x005EE160);   // (ForAllTextures: the first)

GAME_VAR(int32_t, bulletinBoardFirst, 0x00BCCE80);   // (its first field: non-zero with entries)
GAME_VAR(uint8_t, fePadRight, 0x020C327C);
GAME_VAR(uint8_t, fePadLeft, 0x020C3280);
GAME_VAR(uint8_t, widescreen, 0x00C1870E);
GAME_VAR(int32_t, language, 0x00C18724);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, MI_BulletinBoard, 0x00BF3EA0);
GAME_VAR(int32_t, MI_FortuneTeller, 0x00BF3E9C);
GAME_VAR(int32_t, MI_Globe, 0x00BF3E98);
GAME_VAR(void*, cameraNode, 0x00C3CCF4);
GAME_VAR(CPool<uint8_t>*, ms_pTxdPool, 0x01B64654);
GAME_ARRAY(uint8_t, fontColour, 0x0208B764);
GAME_ARRAY(uint8_t, fontShadowColour, 0x0208B780);
GAME_VAR(uint16_t, fontWord784, 0x0208B784);
GAME_VAR(float, fontWrapX, 0x0208B778);
GAME_VAR(float, fortuneExitOffset, 0x00CEFC30);

GAME_FN(uint8_t, __cdecl, FE_PadPressed, 0x006A3170, int pad);
GAME_FN(void, __cdecl, FE_PadClear, 0x006A3120, int pad);
GAME_FN(void, __cdecl, FE_PadUpdate, 0x006A31B0);
GAME_FN(void, __cdecl, HUD_RestoreVisibility, 0x00514210, int which);
GAME_FN(void, __cdecl, HUD_SaveVisibility, 0x005141D0, int which);
GAME_FN(uint8_t, __cdecl, Texture_Destroy, 0x005F0F90, void* texture);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, RemoveModel, 0x0052F640, int id);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, CTimer_EndUserPause, 0x0045B820);
GAME_FN(void, __cdecl, CTimer_StartUserPause, 0x0045B810);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, void* entity, int node, NiTransform* transform, int flag);
GAME_FN(int32_t, __cdecl, TxdStore_FindSeasonal, 0x005F2450, uint32_t name, int);
GAME_FN(void, __thiscall, Board_ListEntries, 0x00418710, void* board, int32_t* out, int32_t* count, int32_t* firstActive);
GAME_FN(uint32_t, __thiscall, Board_GetText, 0x00418810, void* board, int index);
GAME_FN(void, __thiscall, Board_SetRead, 0x004189E0, void* board, int index, int read);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetStyle, 0x00688050, int style);
GAME_FN(void, __cdecl, Text_CopyToWide, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(const uint16_t*, __thiscall, Text_GetByHash, 0x0068FF10, void* text, uint32_t hash);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
inline int __cdecl Rand() { return crt::Rand(); }
template <typename... A>
inline int __cdecl sprintf_(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, ForAllTextures, 0x005F1590, void* dict, void* callback, void* data);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int32_t* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(int, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void*, __cdecl, Im3D_Transform, 0x005EFAF0, void* vertices, int count, NiTransform* transform, uint32_t flags);
GAME_FN(void, __cdecl, Im3D_RenderIndexedPrimitive, 0x005F0170, int type, uint16_t* indices, int count, int);
GAME_FN(uint8_t, __cdecl, Im3D_End, 0x005EFB20);
GAME_FN(uint8_t, __cdecl, Pad_IsBeingReleased, 0x00738940, int button, int pad);
GAME_FN(void, __thiscall, Sounds_Play, 0x0059C150, void* sounds, int sound);
GAME_FN(void, __cdecl, Fn_6A23A0, 0x006A23A0);
GAME_FN(void, __thiscall, BulletinBoard_HandleInput_Game, 0x00539110, BulletinSign* self);

template <typename Fn>
Fn VFunc(const void* object, int index) {
    const re::Ptr32<void>* vtable = *static_cast<const re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<Fn>(static_cast<void*>(vtable[index]));
}

using SignSetupFn = uint8_t(__thiscall*)(Sign* self, float* width, float* height, CVector* pos, int32_t* txd, void* entity);
using SignVoidFn = void(__thiscall*)(Sign* self);
using SelfFn = void(__thiscall*)(HUD2DSigns* self);
using SelfIntFn = void(__thiscall*)(HUD2DSigns* self, int);

// The font's colour and (optionally) shadow from RGBAs
static void SetFontColours(const uint8_t* colour, const uint8_t* shadow) {
    fontColour[0] = colour[0];
    fontColour[1] = colour[1];
    fontColour[2] = colour[2];
    fontColour[3] = colour[3];
    if (shadow) {
        fontShadowColour[0] = shadow[0];
        fontShadowColour[1] = shadow[1];
        fontShadowColour[2] = shadow[2];
        fontShadowColour[3] = shadow[3];
    }
    fontWord784 = 0;
}

// The fade in, one step (0.05) at a time up to 1
static float StepFade(Sign* sign) {
    float f = float(double(sign->fade) + 0.05);
    if (f >= 1.0f) {
        f = 1.0f;
    }
    sign->fade = f;
    return f;
}

// 0x5390D0: HUDComponent::IsVisible (4 bytes: not hookable)
uint8_t __fastcall IsVisible(HUD2DSigns* self, re::Edx) {
    return self->visible;
}
RE_UNHOOKED_FN("HUDComponent::IsVisible_5390D0", 0x005390D0, &IsVisible);

// 0x539110 (thiscall): the bulletin board's left / right between its entries
void __fastcall BulletinBoard_HandleInput(BulletinSign* self, re::Edx) {
    if (fePadRight && FE_PadPressed(0)) {
        self->index++;
        int count = self->count;
        if (self->index >= count) {
            self->index = 0;
        }
        if (count > 1) {
            FE_PadClear(0);
        }
    } else if (fePadLeft && FE_PadPressed(0)) {
        if (--self->index < 0) {
            self->index = self->count - 1;
        }
        if (self->count > 1) {
            FE_PadClear(0);
        }
    }
    FE_PadUpdate();
}

// 0x5391A0 (thiscall, vf12): closed: the sign's, the texture and its txd let go, the
// game unpaused
void __fastcall Close(HUD2DSigns* self, re::Edx) {
    HUD_RestoreVisibility(0x29);
    if (self->sign) {
        VFunc<SignVoidFn>(self->sign, 2)(self->sign);
    }
    if (self->texture) {
        Texture_Destroy(self->texture);
        self->texture = nullptr;
    }
    if (self->txd != -1) {
        if (GetStreamingInfo(self->txd + 0x2FA8)[4] == 1) {
            RemoveModel(self->txd + 0x2FA8);
        }
        self->txd = -1;
    }
    CTimer_EndUserPause();
}

// 0x539240 (thiscall, the bulletin board sign's vf0): its size, the "Sign" node's
// position, its entries, and its txd requested (false without the node or entries)
uint8_t __fastcall BulletinBoard_Setup(BulletinSign* self, re::Edx, float* width, float* height, CVector* pos, int32_t* txd,
                                      void* entity) {
    self->fade = 0.0f;
    self->text = 0;
    if (widescreen) {
        if (language == 7) {
            *width = 0.72f;
            *height = 0.4f;
        } else {
            *width = 0.6f;
            *height = 0.5f;
        }
    } else if (language == 7) {
        *width = 0.72f;
        *height = 0.56f;
    } else {
        *width = 0.72f;
        *height = 0.6f;
    }
    self->fade = 0.0f;
    uint32_t hash = HashName("Sign");
    self->count = 0;
    self->index = 0;
    NiTransform transform;
    transform.scale = 1.0f;
    if (!GetNodeTransform(entity, int(hash), &transform, 1)) {
        return 0;
    }
    *pos = transform.translate;
    int32_t first = bulletinBoardFirst;
    Board_ListEntries(bulletinBoard, self->entries, &self->count, &self->index);
    if (!first) {
        return 0;
    }
    *txd = TxdStore_FindSeasonal(uint32_t(first), 0);
    RequestModel(*txd + 0x2FA8, 9);
    return 1;
}

// 0x5393A0 (thiscall, the bulletin board sign's vf1): the entry's text, "n of m" and the
// exit prompt; the entry shown marked read
void __fastcall BulletinBoard_Render(BulletinSign* self, re::Edx) {
    BulletinBoard_HandleInput_Game(self);
    StepFade(self);
    self->text = Board_GetText(bulletinBoard, self->entries[self->index]);
    int alpha = int(self->fade * self->fade * 215.0f);
    uint8_t shadow[4];
    RGBA_Construct(shadow, 0, 0, 0, alpha);
    if (self->text) {
        uint8_t base[4];
        RGBA_Construct(base, 0x60, 0x60, 0x60, 0);
        float y = language == 7 ? 115.0f : 100.0f;
        uint8_t colour[4];
        RGBA_Construct(colour, base[0], base[1], base[2], alpha);
        Font_Begin();
        SetFontColours(colour, shadow);
        Font_SetJustify(2);
        Font_SetStyle(3);
        fontWrapX = (widescreen && language != 7) ? 180.0f : 300.0f;
        uint16_t text[256];
        Text_CopyToWide(text, Text_GetByHash(theText, self->text), 0xFF);
        Font_PrintString(320.0f, y, text, 1);
    }
    uint8_t base[4];
    RGBA_Construct(base, 0x60, 0x60, 0x60, 0);
    uint8_t colour[4];
    RGBA_Construct(colour, base[0], base[1], base[2], alpha);
    Font_AddNumber(float(self->index + 1));
    Font_AddNumber(float(self->count));
    uint16_t messages[16];
    Text_CopyToWide(messages, Text_Get(theText, "BB_NMSGS"), 0xF);
    Font_Begin();
    SetFontColours(colour, nullptr);
    Font_SetJustify(1);
    Font_SetStyle(1);
    Font_PrintString(460.0f, 395.0f, messages, 1);
    Font_Begin();
    SetFontColours(colour, shadow);
    Font_SetJustify(2);
    Font_SetStyle(1);
    Font_PrintString(320.0f, 370.0f, Text_Get(theText, "BB_EXIT"), 1);
    if (self->index >= 0) {
        Board_SetRead(bulletinBoard, self->entries[self->index], 1);
    }
}

// The fortune teller's and the globe's vf0: their size, the entity's position, the card
// txd requested and a random card's text ("FT_001".."FT_<n-2>")
static uint8_t CardSetup(CardSign* self, CVector* pos, int32_t* txd, void* entity, const char* txdName, const char* format) {
    const uint8_t* e = static_cast<const uint8_t*>(entity);
    const uint8_t* matrix = *reinterpret_cast<const re::Ptr32<uint8_t>*>(e + 0x14);
    const CVector* p = matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : reinterpret_cast<const CVector*>(e + 4);
    *pos = *p;
    int32_t slot = TxdStore_FindSeasonal(HashName(txdName), 0);
    *txd = slot;
    RequestModel(slot + 0x2FA8, 9);
    int range = self->numCards - 2;
    int card = Rand() * range / 32767 + 1;
    char name[16];
    sprintf_(name, format, card);
    self->text = HashName(name);
    return *txd != -1;
}

// 0x5396D0 (thiscall, the fortune teller sign's vf0)
uint8_t __fastcall FortuneTeller_Setup(CardSign* self, re::Edx, float* width, float* height, CVector* pos, int32_t* txd, void* entity) {
    self->fade = 0.0f;
    self->text = 0;
    *width = language == 7 ? 0.75f : 0.55f;
    *height = 0.25f;
    return CardSetup(self, pos, txd, entity, "FortuneCard", "FT_%03d");
}

// 0x539A00 (thiscall, the globe sign's vf0)
uint8_t __fastcall Globe_Setup(CardSign* self, re::Edx, float* width, float* height, CVector* pos, int32_t* txd, void* entity) {
    self->fade = 0.0f;
    self->text = 0;
    *width = 0.45f;
    *height = 0.35f;
    return CardSetup(self, pos, txd, entity, "GlobeCard", "GLB_%03d");
}

// The cards' vf1: the card's text in brown and the exit prompt
static void CardRender(CardSign* self, float wrapX, float textY, const char* exitKey, bool fortuneTeller, float exitY) {
    StepFade(self);
    int alpha = int(self->fade * self->fade * 192.0f);
    uint8_t shadow[4];
    RGBA_Construct(shadow, 0, 0, 0, alpha);
    if (self->text) {
        uint8_t base[4];
        RGBA_Construct(base, 0x34, 0x22, 0xB, 0);
        uint8_t colour[4];
        RGBA_Construct(colour, base[0], base[1], base[2], alpha);
        Font_Begin();
        SetFontColours(colour, shadow);
        Font_SetJustify(2);
        Font_SetStyle(language == 7 ? 1 : 3);
        fontWrapX = wrapX;
        uint16_t text[256];
        Text_CopyToWide(text, Text_GetByHash(theText, self->text), 0xFF);
        Font_PrintString(320.0f, textY, text, 1);
    }
    uint8_t base[4];
    RGBA_Construct(base, 0x34, 0x22, 0xB, 0);
    uint8_t colour[4];
    RGBA_Construct(colour, base[0], base[1], base[2], alpha);
    Font_Begin();
    SetFontColours(colour, shadow);
    Font_SetJustify(2);
    Font_SetStyle(1);
    if (fortuneTeller) {
        // the fortune teller's: moved up for language 7 (for good: it's a global)
        if (language == 7) {
            fortuneExitOffset = -15.0f;
        }
        const uint16_t* exitText = Text_Get(theText, exitKey);
        Font_PrintString(320.0f, float(double(fortuneExitOffset) + 295.0), exitText, 1);
        return;
    }
    Font_PrintString(320.0f, exitY, Text_Get(theText, exitKey), 1);
}

// 0x5397C0 (thiscall, the fortune teller sign's vf1)
void __fastcall FortuneTeller_Render(CardSign* self, re::Edx) {
    CardRender(self, 300.0f, 170.0f, "FT_EXIT", true, 0.0f);
}

// 0x539AE0 (thiscall, the globe sign's vf1)
void __fastcall Globe_Render(CardSign* self, re::Edx) {
    CardRender(self, 315.0f, 110.0f, "GLB_EXIT", false, 350.0f);
}

// 0x539D00 (thiscall, vf11): opened on the player's target: its sign by model, set up;
// closed (vf12) without one, otherwise the game paused
void __fastcall Open(HUD2DSigns* self, re::Edx) {
    HUD_SaveVisibility(0x29);
    VFunc<SelfFn>(self, 7)(self);
    self->target = *reinterpret_cast<re::Ptr32<void>*>(playerPed + 0x438);
    if (!self->target) {
        VFunc<SelfFn>(self, 12)(self);
        return;
    }
    int model = *reinterpret_cast<int16_t*>(static_cast<uint8_t*>(self->target) + 0x10E);
    if (model == MI_BulletinBoard) {
        self->sign = &self->board;
    } else if (model == MI_FortuneTeller) {
        self->sign = &self->fortune;
    } else if (model == MI_Globe) {
        self->sign = &self->globe;
    } else {
        VFunc<SelfFn>(self, 12)(self);
        return;
    }
    if (!VFunc<SignSetupFn>(self->sign, 0)(self->sign, &self->width, &self->height, &self->pos, &self->txd, self->target)) {
        VFunc<SelfFn>(self, 12)(self);
        return;
    }
    CTimer_StartUserPause();
}

// 0x539E20 (thiscall, vf8): the card: its texture fetched once the txd is in, then the
// quad flown in from the object to in front of the camera and drawn, the sign's text
// over it; once there, button 8 released closes it (vf5)
void __fastcall Render3D(HUD2DSigns* self, re::Edx) {
    CTimer_StartUserPause();
    if (!self->texture) {
        if (GetStreamingInfo(self->txd + 0x2FA8)[4] != 1) {
            return;
        }
        CPool<uint8_t>* pool = ms_pTxdPool;
        uint8_t* def = (pool->flags[self->txd] & 0x80) ? nullptr : pool->objects + pool->elemSize * self->txd;
        void* dict = *reinterpret_cast<re::Ptr32<void>*>(def);   // (a free slot: null, as the game)
        if (!dict) {
            return;
        }
        Txd_PushCurrentTxd();
        Txd_SetCurrentTxd(self->txd);
        ForAllTextures(dict, GetTextureCB, &self->texture);
        Txd_PopCurrentTxd();
        return;
    }

    float s = self->amount;
    float d = std::fabs(s - 1.0f);
    if (!(d <= 1e-5f)) {
        s = (1.0f - s) * 0.5f + s;
    } else {
        s = 1.0f;
    }
    self->amount = s;
    int alpha = int(self->amount * 255.0f);

    // the camera's forward (column 2), right (0) and up (1)
    const NiTransform* t = GetLocalTransform(GetCameraNode(cameraNode));
    CVector fwd = {t->rotate[2], t->rotate[5], t->rotate[8]};
    t = GetLocalTransform(GetCameraNode(cameraNode));
    CVector ahead = {fwd.x + t->translate.x, t->translate.y + fwd.y, t->translate.z + fwd.z};
    t = GetLocalTransform(GetCameraNode(cameraNode));
    CVector left = {t->rotate[0] * -1.0f, t->rotate[3] * -1.0f, t->rotate[6] * -1.0f};
    float w = self->width;
    CVector across = {left.x * w, left.y * w, left.z * w};
    t = GetLocalTransform(GetCameraNode(cameraNode));
    float h = self->height;
    CVector up = {t->rotate[1] * h, t->rotate[4] * h, t->rotate[7] * h};
    float amount = self->amount;
    CVector a = {amount * ahead.x, amount * ahead.y, amount * ahead.z};
    float rest = 1.0f - self->amount;
    CVector b = {rest * self->pos.x, rest * self->pos.y, rest * self->pos.z};
    CVector mid = {b.x + a.x, b.y + a.y, b.z + a.z};

    CVector top = {mid.x + up.x, mid.y + up.y, mid.z + up.z};
    CVector bottom = {mid.x - up.x, mid.y - up.y, mid.z - up.z};
    Im3DVertex verts[4];
    verts[0].pos = {top.x - across.x, top.y - across.y, top.z - across.z};
    verts[1].pos = {top.x + across.x, top.y + across.y, top.z + across.z};
    verts[2].pos = {bottom.x + across.x, bottom.y + across.y, bottom.z + across.z};
    verts[3].pos = {bottom.x - across.x, bottom.y - across.y, bottom.z - across.z};
    for (Im3DVertex& v : verts) {
        v.normal = {0.0f, 0.0f, 1.0f};
    }
    uint8_t c[4];
    RGBA_Construct(c, 0xD7, 0xD7, 0xD7, 0xFF);
    verts[0].u = 0.0f;
    verts[0].v = 0.0f;
    verts[1].u = 1.0f;
    verts[1].v = 0.0f;
    verts[2].u = 1.0f;
    verts[2].v = 1.0f;
    verts[3].u = 0.0f;
    verts[3].v = 1.0f;
    uint32_t color = (uint32_t(alpha) << 24) | (uint32_t(c[0]) << 16) | (uint32_t(c[1]) << 8) | uint32_t(c[2]);
    for (Im3DVertex& v : verts) {
        v.color = color;
    }
    uint16_t indices[6] = {2, 1, 0, 0, 2, 3};

    int32_t zWrite, zTest, blend;
    GetRenderState(8, &zWrite);
    GetRenderState(6, &zTest);
    GetRenderState(0xC, &blend);
    SetRenderState(8, 0);
    SetRenderState(6, 0);
    SetRenderState(0xC, 1);
    SetRenderState(1, GetRaster(self->texture));
    Im3D_Transform(verts, 4, nullptr, 0x19);
    Im3D_RenderIndexedPrimitive(3, indices, 6, 1);
    Im3D_End();
    SetRenderState(8, zWrite);
    SetRenderState(6, zTest);
    SetRenderState(0xC, blend);

    if (self->sign) {
        VFunc<SignVoidFn>(self->sign, 1)(self->sign);
    }
    float left1 = std::fabs(self->amount - 1.0f);
    if (left1 <= 1e-5f && Pad_IsBeingReleased(8, 0)) {
        Sounds_Play(sounds, 1);
        Fn_6A23A0();
        VFunc<SelfIntFn>(self, 5)(self, 0);
    }
}

} // namespace hud2dsigns17

// ---- registration ----------------------------------------------------------------------

#define HUD2DSIGNS17_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace hud2dsigns17 {
HUD2DSIGNS17_REVERSED("HUD2DSigns::CBulletinBoard::HandleInput", 0x00539110, &BulletinBoard_HandleInput);
HUD2DSIGNS17_REVERSED("HUD2DSigns::Close", 0x005391A0, &Close);
HUD2DSIGNS17_REVERSED("HUD2DSigns::CBulletinBoard::Setup", 0x00539240, &BulletinBoard_Setup);
HUD2DSIGNS17_REVERSED("HUD2DSigns::CBulletinBoard::Render", 0x005393A0, &BulletinBoard_Render);
HUD2DSIGNS17_REVERSED("HUD2DSigns::CFortuneTeller::Setup", 0x005396D0, &FortuneTeller_Setup);
HUD2DSIGNS17_REVERSED("HUD2DSigns::CFortuneTeller::Render", 0x005397C0, &FortuneTeller_Render);
HUD2DSIGNS17_REVERSED("HUD2DSigns::CGlobe::Setup", 0x00539A00, &Globe_Setup);
HUD2DSIGNS17_REVERSED("HUD2DSigns::CGlobe::Render", 0x00539AE0, &Globe_Render);
HUD2DSIGNS17_REVERSED("HUD2DSigns::Open", 0x00539D00, &Open);
HUD2DSIGNS17_REVERSED("HUD2DSigns::Render3D", 0x00539E20, &Render3D);
} // namespace hud2dsigns17
