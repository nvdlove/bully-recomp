// The barber's screen (HUDBarberImpl, 0x284 bytes; made with the clothing catalogue):
// up to 10 groups of up to 4 haircuts {hair hash, price} the script adds, the group and
// haircut picked with the pad (each shown on the player for a while), buying (the money
// taken, a hat taken off unless the outfit keeps it, the stats), and the drawing: the
// title by area, the groups with arrows either side of the picked one, the price, the
// button hints. 0x6CCDB0..0x6CDB84.
//
// HUDBarberImpl (by offset):
//   +0x0     the script event sender       +0x4    10 group title hashes
//   +0x2C    10 groups of 0x24: 4 {hair hash, price} and the count (+0x20)
//   +0x194   the 2 button textures (heap)   +0x198  the HudBarber txd slot
//   +0x19C   the group count                +0x1A0  the group picked
//   +0x1A4   int[10] the haircut picked per group
//   +0x1CC   byte: previewing (the set at +0x1D0 goes back on at +0x228's time)
//   +0x1D0   a clothing set (0x58): the preview    +0x228 the preview's end time
//   +0x22C   the clothing set worn on entering (0x58)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

// a clothing item (12; see PlayerClothing.cpp)
struct BarberItem {
    uint32_t hash;
    uint32_t hash2;
    uint8_t bytes[4];
};
static_assert(sizeof(BarberItem) == 12);

// a clothing set (0x58)
struct BarberSet {
    BarberItem items[6];   // +0
    int32_t outfit;        // +0x48
    BarberItem hair;       // +0x4C
};
static_assert(sizeof(BarberSet) == 0x58);

struct BarberEntry {
    uint32_t hash;
    uint32_t price;
};

struct BarberGroup {
    BarberEntry entries[4];   // +0
    int32_t count;            // +0x20
};
static_assert(sizeof(BarberGroup) == 0x24);

struct HudBarber {
    uint32_t events;             // +0x0
    uint32_t groupTitles[10];    // +0x4
    BarberGroup groups[10];      // +0x2C
    re::Ptr32<re::Ptr32<void>> textures;             // +0x194
    int32_t txdSlot;             // +0x198
    int32_t groupCount;          // +0x19C
    int32_t group;               // +0x1A0
    int32_t picked[10];          // +0x1A4
    uint8_t previewing;          // +0x1CC
    uint8_t unknown1CD[3];
    BarberSet preview;           // +0x1D0
    uint32_t previewEnd;         // +0x228
    BarberSet entered;           // +0x22C
};
static_assert(offsetof(HudBarber, textures) == 0x194);
static_assert(offsetof(HudBarber, picked) == 0x1A4);
static_assert(offsetof(HudBarber, preview) == 0x1D0);
static_assert(offsetof(HudBarber, entered) == 0x22C);
static_assert(sizeof(HudBarber) == 0x284);

struct Rgba {
    uint8_t r, g, b, a;
};

// ---- game data ----
inline void* const hb_clothing = reinterpret_cast<void*>(0x020C5F70);
GAME_VAR(BarberItem, hb_hair, 0x020C6B74);              // CClothing +0xC04
GAME_ARRAY(uint8_t, hb_outfits, 0x020C6B80);            // CClothing +0xC10 (0x54 each, flags +0x50)
GAME_VAR(int32_t, hb_currentOutfit, 0x020C76FC);        // CClothing +0x178C
GAME_VAR(uint8_t, hb_clothesChanged, 0x020C78F8);       // CClothing +0x1988
GAME_VAR(uint32_t, hb_noneHead, 0x020C5F6C);
GAME_VAR(uint8_t*, hb_playerPed, 0x00C1AEA8);           // its money at +0x1CA0
GAME_VAR(uint32_t, hb_menuTime, 0x00A14A50);
GAME_VAR(int32_t, hb_currArea, 0x00BD1008);
GAME_VAR(int32_t, hb_language, 0x00C18724);
GAME_VAR(float, hb_boxOffset, 0x00BC5BD4);
GAME_VAR(float, hb_screenScaleX, 0x00A957A8);
GAME_VAR(float, hb_screenScaleY, 0x00A957AC);
GAME_VAR(float, hb_rowGap, 0x00A14930);
GAME_VAR(float, hb_nameX, 0x00A14980);
GAME_VAR(Rgba, hb_fontColour, 0x0208B764);
GAME_VAR(float, hb_fontWrapCentre, 0x0208B778);
GAME_VAR(Rgba, hb_infoColour, 0x00C19228);
GAME_VAR(Rgba, hb_titleColour, 0x00C1922C);
GAME_VAR(Rgba, hb_textColour, 0x00C19230);
GAME_VAR(Rgba, hb_listColour, 0x00C19234);
GAME_VAR(uint8_t, hb_padDown, 0x020C3274);
GAME_VAR(uint8_t, hb_padUp, 0x020C3278);
GAME_VAR(uint8_t, hb_padRight, 0x020C327C);
GAME_VAR(uint8_t, hb_padLeft, 0x020C3280);
inline void* const hb_shop = reinterpret_cast<void*>(0x00C18748);
inline void* const hb_camera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const hb_playerControl = reinterpret_cast<void*>(0x00A45A30);
inline void* const hb_soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const hb_text = reinterpret_cast<void*>(0x020C2FA4);

GAME_FN(void*, __cdecl, Hb_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Hb_Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, Hb_PlayerControlSetEnabled, 0x0050DFA0, void* control, int enabled);
GAME_FN(int, __thiscall, Hb_SoundBanksFind, 0x00597FC0, void* banks, const char* name);
GAME_FN(void, __thiscall, Hb_SoundBanksLoad, 0x0059A3E0, void* banks, int bank, int, int);
GAME_FN(void, __thiscall, Hb_SoundBanksUnload, 0x0059A420, void* banks, int bank, int, int);
GAME_FN(int, __cdecl, Hb_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Hb_RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, Hb_RemoveModel, 0x0052F640, int id);
GAME_FN(void, __cdecl, Hb_LoadingMessage, 0x0043C4F0, const char* where, const char* what, uintptr_t);
GAME_FN(void, __cdecl, Hb_LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __cdecl, Hb_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Hb_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, Hb_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Hb_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, Hb_TextureDestroy, 0x005F0F90, void* texture);
GAME_FN(void, __thiscall, Hb_EventsReset, 0x005D9C50, void* events);
GAME_FN(void, __thiscall, Hb_SendEvent, 0x006CDD10, HudBarber* self, int32_t what, int32_t value);
GAME_FN(int, __thiscall, Hb_CameraGetFadeState, 0x004F12F0, void* camera);
GAME_FN(uint8_t, __cdecl, Hb_PadPressed, 0x006A3170, int pad);
GAME_FN(uint8_t, __cdecl, Hb_PadJustPressed, 0x00738900, int control, int pad);
GAME_FN(void, __cdecl, Hb_PlaySound, 0x006A30D0, int sound, int);
GAME_FN(void, __cdecl, Hb_RequestSound, 0x006A2340, int sound);
GAME_FN(void, __cdecl, Hb_PlayMenuSound, 0x006A31B0);
GAME_FN(void, __thiscall, Hb_ShopSetFlash, 0x004551A0, void* shop, int state);
GAME_FN(void, __thiscall, Hb_TakeMoney, 0x00473100, uint8_t* ped, int32_t amount);
GAME_FN(void, __cdecl, Hb_StatsAdd, 0x00458480, int32_t stat, int32_t value);
GAME_FN(void, __thiscall, Hb_ShopDrawPanel, 0x004556F0, void* shop);
GAME_FN(void, __cdecl, Hb_ShopDrawMoney, 0x00455EF0);
GAME_FN(void, __stdcall, Hb_ShopDrawBox, 0x00455390, float x, float y, float h);
GAME_FN(void, __thiscall, Hb_ShopDrawItemName, 0x00456790, void* shop, uint32_t hash, uint32_t price, int purchased);
GAME_FN(const uint16_t*, __thiscall, Hb_TextGet, 0x0068FE80, void* text, const char* key);
GAME_FN(const uint16_t*, __thiscall, Hb_TextGetByHash, 0x0068FF10, void* text, uint32_t hash);
GAME_FN(void, __cdecl, Hb_FontReset, 0x00688E60);
GAME_FN(void, __cdecl, Hb_FontSetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Hb_FontSetStyle, 0x00688050, int style);
GAME_FN(float, __cdecl, Hb_FontGetLineHeight, 0x00688220);
GAME_FN(float, __cdecl, Hb_FontGetStringWidth, 0x0068A920, const uint16_t* text, int withSpaces);
GAME_FN(void, __cdecl, Hb_FontMeasure, 0x0068B090, float* rect, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, Hb_FontPrint, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Hb_FontRender, 0x0068CDF0);
GAME_FN(uint8_t*, __thiscall, Hb_RgbaConstruct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Hb_DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const void* colour, const float* uv1, const float* uv2);
// the clothing's (PlayerClothing.cpp, ClothingCatalog.cpp)
GAME_FN(BarberItem*, __thiscall, Hb_ItemConstruct, 0x006CB310, BarberItem* item);
GAME_FN(uint32_t, __thiscall, Hb_OwnedItemSet, 0x006CA4F0, void* item, uint32_t hash, int slot);
GAME_FN(void, __thiscall, Hb_SetPlayer, 0x006CB670, void* clothing, const void* item);
GAME_FN(void, __thiscall, Hb_SetPlayersHair, 0x006CA900, void* clothing, uint32_t hash);
GAME_FN(void, __thiscall, Hb_Backup, 0x006CAE80, void* clothing, BarberSet* set);
GAME_FN(void, __thiscall, Hb_Restore, 0x006CB9B0, void* clothing, const BarberSet* set);
GAME_FN(void*, __cdecl, Hb_GetCatalog, 0x006CA7C0);
GAME_FN(BarberItem*, __thiscall, Hb_CatalogFind, 0x006CC320, void* catalog, BarberItem* out, const void* item);
GAME_FN(BarberItem*, __thiscall, Hb_CatalogFindHair, 0x006CC3F0, void* catalog, BarberItem* out,
        const uint32_t* hash);
// this file's, kept as calls
GAME_FN(void, __thiscall, Hb_Preview, 0x006CD190, HudBarber* self, const BarberEntry* entry);
GAME_FN(void, __thiscall, Hb_DrawArrows, 0x006CD1F0, HudBarber* self, float x, float y, const uint16_t* text);
GAME_FN(void, __thiscall, Hb_Process, 0x006CD380, HudBarber* self);

// the player's head bared (the "nothing" head item put on)
void BareHead() {
    uint8_t item[8];
    Hb_OwnedItemSet(item, hb_noneHead, 0);
    Hb_SetPlayer(hb_clothing, item);
}

// 0x6CCDB0: the screen opened (the input off, its sounds and textures loaded, what's
// worn kept, the head bared)
void __fastcall Open(HudBarber* self, re::Edx) {
    Hb_PlayerControlSetEnabled(hb_playerControl, 0);
    Hb_SoundBanksLoad(hb_soundBanks, Hb_SoundBanksFind(hb_soundBanks, "BarbHud.bnk"), 1, 1);
    self->textures = static_cast<re::Ptr32<void>*>(Hb_Malloc(8));
    self->txdSlot = Hb_FindTxdSlot("HudBarber", 0);
    Hb_RequestModel(self->txdSlot + 0x2FA8, 9);
    Hb_LoadingMessage("HUDBarberImpl::Open()", "CStreaming::LoadAllRequestedModels()", 0);
    Hb_LoadAllRequestedModels(1);
    Hb_PushCurrentTxd();
    Hb_SetCurrentTxd(self->txdSlot);
    self->textures[0] = Hb_FindTexture("buttonbarbnormal", "buttonbarbnormal_x");
    self->textures[1] = Hb_FindTexture("buttonbarbdown", "buttonbarbdown_x");
    Hb_PopCurrentTxd();
    self->groupCount = 0;
    self->group = 0;
    for (int i = 0; i < 10; ++i) {
        self->picked[i] = 0;
        self->groupTitles[i] = 0;
    }
    self->previewing = 0;
    Hb_Backup(hb_clothing, &self->entered);
    BareHead();
}

// 0x6CCF10
void __fastcall SetGroupTitle(HudBarber* self, re::Edx, int group, uint32_t title) {
    self->groupTitles[group] = title;
}

// 0x6CCF20: HUDBarberImpl's constructor
HudBarber* __fastcall Construct(HudBarber* self, re::Edx) {
    self->events = 0;
    for (int i = 0; i < 10; ++i) {
        self->groups[i].count = 0;
    }
    for (int i = 0; i < 6; ++i) {
        Hb_ItemConstruct(&self->preview.items[i]);
    }
    Hb_ItemConstruct(&self->preview.hair);
    for (int i = 0; i < 6; ++i) {
        Hb_ItemConstruct(&self->entered.items[i]);
    }
    Hb_ItemConstruct(&self->entered.hair);
    self->textures = nullptr;
    self->txdSlot = -1;
    return self;
}

// 0x6CCFD0: the screen closed (what was worn on entering put back, the groups emptied)
void __fastcall Close(HudBarber* self, re::Edx) {
    Hb_EventsReset(self);
    Hb_PlayerControlSetEnabled(hb_playerControl, 1);
    for (int i = 0; i < 2; ++i) {
        Hb_TextureDestroy(self->textures[i]);
    }
    Hb_Free(self->textures);
    int model = self->txdSlot + 0x2FA8;
    self->textures = nullptr;
    Hb_RemoveModel(model);
    self->txdSlot = -1;
    Hb_Restore(hb_clothing, &self->entered);
    hb_clothesChanged = 1;
    for (int i = 0; i < self->groupCount; ++i) {
        self->groups[i].count = 0;
    }
    Hb_SoundBanksUnload(hb_soundBanks, Hb_SoundBanksFind(hb_soundBanks, "BarbHud.bnk"), 1, 1);
    self->groupCount = 0;
}

// 0x6CD0B0: a haircut added to a group (the first one also sets the preview up)
void __fastcall AddHaircut(HudBarber* self, re::Edx, int group, uint32_t hash, uint32_t price) {
    BarberGroup& entries = self->groups[group];
    entries.entries[entries.count].hash = hash;
    entries.entries[entries.count].price = price;
    entries.count += 1;
    if (self->groupCount == 0) {
        Hb_SetPlayersHair(hb_clothing, hash);
        BareHead();
        hb_currentOutfit = -1;
        Hb_Backup(hb_clothing, &self->preview);
        self->previewing = 1;
        self->previewEnd = 0;
    }
    float count = static_cast<float>(self->groupCount);
    float needed = static_cast<float>(group + 1);
    float larger = needed < count ? count : needed;
    self->groupCount = static_cast<int32_t>(larger);
}

// 0x6CD190: a haircut shown on the player for 1.2 seconds
void __fastcall Preview(HudBarber* self, re::Edx, const BarberEntry* entry) {
    self->previewing = 1;
    BarberItem found;
    const BarberItem* hair = Hb_CatalogFindHair(Hb_GetCatalog(), &found, &entry->hash);
    self->preview.hair.hash = hair->hash;
    self->preview.hair.hash2 = hair->hash2;
    self->preview.hair.bytes[0] = hair->bytes[0];
    self->preview.hair.bytes[1] = hair->bytes[1];
    self->preview.hair.bytes[2] = hair->bytes[2];
    self->preview.hair.bytes[3] = hair->bytes[3];
    self->previewEnd = hb_menuTime + 0x4B0;
}

// 0x6CD1F0: the arrows either side of a group's title (the left one mirrored; each the
// pressed texture while its direction is held)
void __fastcall DrawArrows(HudBarber* self, re::Edx, float x, float y, const uint16_t* text) {
    float half = Hb_FontGetStringWidth(text, 1) * 0.5f;
    void* pressed = self->textures[1];
    void* normal = self->textures[0];
    float width = hb_screenScaleX * 15.0f;
    float height = hb_screenScaleY * 26.25f;
    float top = y - 8.75f;
    float left = (x - half) - 8.0f;
    void* texture = hb_padLeft != 0 ? pressed : normal;
    float uv0[2] = {0.0f, 0.0f};
    float uv1[2] = {1.0f, 1.0f};
    float bottom = top + height;
    uint8_t colour[4];
    Hb_DrawRectUV(left - width, top, left, bottom, texture, Hb_RgbaConstruct(colour, 0xD7, 0xD7, 0xD7, 0xFF), uv1,
                  uv0);
    float right = (half + x) + 8.0f;
    texture = hb_padRight != 0 ? pressed : normal;
    uint8_t colour2[4];
    Hb_DrawRectUV(right, top, right + width, bottom, texture, Hb_RgbaConstruct(colour2, 0xD7, 0xD7, 0xD7, 0xFF),
                  uv0, uv1);
}

// the picked haircut of the group picked
BarberEntry* Picked(HudBarber* self) {
    return &self->groups[self->group].entries[self->picked[self->group]];
}

// 0x6CD380: the pad (left/right the haircut, up/down the group, the buy button)
void __fastcall Process(HudBarber* self, re::Edx) {
    if (Hb_CameraGetFadeState(hb_camera) != 0) {
        return;
    }
    if (hb_padRight != 0 && Hb_PadPressed(0)) {
        int32_t& picked = self->picked[self->group];
        picked += 1;
        if (picked == self->groups[self->group].count) {
            picked = 0;
        }
        Hb_Preview(self, &self->groups[self->group].entries[picked]);
        Hb_SendEvent(self, 4, picked);
        Hb_PlaySound(0xD, 0);
        Hb_PlayMenuSound();
        return;
    }
    if (hb_padLeft != 0 && Hb_PadPressed(0)) {
        int32_t& picked = self->picked[self->group];
        picked += -1;
        if (picked < 0) {
            picked = self->groups[self->group].count - 1;
        }
        Hb_Preview(self, &self->groups[self->group].entries[picked]);
        Hb_SendEvent(self, 4, picked);
        Hb_PlaySound(0xD, 0);
        Hb_PlayMenuSound();
        return;
    }
    bool moved = false;
    if (hb_padDown != 0 && Hb_PadPressed(0)) {
        self->group += 1;
        if (self->group == self->groupCount) {
            self->group = 0;
        }
        moved = true;
    } else if (hb_padUp != 0 && Hb_PadPressed(0)) {
        self->group += -1;
        if (self->group < 0) {
            self->group = self->groupCount - 1;
        }
        moved = true;
    }
    if (moved) {
        Hb_Preview(self, Picked(self));
        Hb_SendEvent(self, 3, self->group);
        Hb_PlaySound(0xE, 0);
        Hb_PlayMenuSound();
        return;
    }
    if (Hb_PadJustPressed(7, 0) && self->previewing == 0) {
        uint32_t price = Picked(self)->price;
        if (hb_hair.hash == self->entered.hair.hash) {
            // (already the player's)
            Hb_ShopSetFlash(hb_shop, 2);
            Hb_SendEvent(self, 7, 1);
        } else if (price > *reinterpret_cast<uint32_t*>(hb_playerPed + 0x1CA0)) {
            Hb_ShopSetFlash(hb_shop, 1);
            Hb_SendEvent(self, 8, static_cast<int32_t>(price));
        } else {
            Hb_RequestSound(0xB);
            Hb_TakeMoney(hb_playerPed, static_cast<int32_t>(price));
            int32_t outfit = self->entered.outfit;
            if (outfit < 0 || (hb_outfits[outfit * 0x54 + 0x50] & 2) == 0) {
                // (a hat worn on entering comes off)
                uint8_t item[8];
                Hb_OwnedItemSet(item, hb_noneHead, 0);
                BarberItem found;
                const BarberItem* head = Hb_CatalogFind(Hb_GetCatalog(), &found, item);
                self->entered.items[0] = *head;
            }
            self->entered.hair = hb_hair;
            Hb_StatsAdd(0x43, 1);
            Hb_StatsAdd(0x3E, static_cast<int32_t>(price));
            Hb_SendEvent(self, 9, static_cast<int32_t>(hb_hair.hash));
        }
    }
    Hb_PlayMenuSound();
}

// 0x6CD6A0: the screen drawn (and run: the pad handled first; the preview put on once
// it's been idle long enough)
void __fastcall Draw(HudBarber* self, re::Edx) {
    Hb_Process(self);
    Hb_ShopDrawPanel(hb_shop);
    Hb_ShopDrawMoney();
    const uint16_t* title = nullptr;
    int32_t area = hb_currArea;
    if (area == 0x38) {
        title = Hb_TextGet(hb_text, "CLT_POORBARB_TITLE");
    } else if (area == 0x2E) {
        title = Hb_TextGet(hb_text, "CLT_HAIRSALON_TITLE");
    } else if (area == 0x27) {
        title = Hb_TextGet(hb_text, "CLT_BARBER_TITLE");
    }
    float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    Hb_FontReset();
    hb_fontColour = hb_titleColour;
    Hb_FontSetJustify(2);
    hb_fontWrapCentre = 240.0f;
    Hb_FontMeasure(rect, hb_nameX, 55.0f, title);
    float height = std::fabs(rect[3] - rect[1]);
    Hb_FontPrint(hb_nameX, 55.0f - height * 0.5f, title, 1);
    Hb_FontRender();
    Hb_FontReset();
    Hb_FontSetJustify(2);
    Hb_FontSetStyle(2);
    int32_t count = self->groupCount;
    float centre = (240.0f + 80.0f) * 0.5f;
    float lines = (Hb_FontGetLineHeight() + hb_rowGap) * static_cast<float>(count);
    float firstY = (centre - lines * 0.5f) + 5.0f;
    for (int32_t i = 0; i < self->groupCount; ++i) {
        float rowY = (Hb_FontGetLineHeight() + hb_rowGap) * static_cast<float>(i) + firstY;
        hb_fontColour = hb_listColour;
        const uint16_t* text = Hb_TextGetByHash(hb_text, self->groupTitles[i]);
        if (i == self->group) {
            hb_fontColour = hb_infoColour;
            if (static_cast<uint32_t>(self->groups[self->group].count) > 1) {
                Hb_DrawArrows(self, hb_nameX, rowY, text);
            }
        }
        Hb_FontPrint(hb_nameX, rowY - 8.75f, text, 1);
        Hb_FontRender();
    }
    BarberEntry* entry = Picked(self);
    // (the flag goes as the game's edx: the group times 9 above its low byte)
    int32_t purchased = ((self->group * 9) & ~0xFF) | (self->entered.hair.hash == entry->hash ? 1 : 0);
    Hb_ShopDrawItemName(hb_shop, entry->hash, entry->price, purchased);
    Hb_FontReset();
    Hb_FontSetStyle(1);
    Hb_FontSetJustify(2);
    hb_fontColour = hb_textColour;
    if (self->previewing == 0) {
        Hb_FontPrint(hb_nameX, 400.0f, Hb_TextGet(hb_text, "STO_BUYEXIT"), 1);
    }
    Hb_FontPrint(hb_nameX, 420.0f, Hb_TextGet(hb_text, "STO_EXIT"), 1);
    Hb_FontRender();
    Hb_FontReset();
    Hb_FontSetStyle(1);
    hb_fontColour = hb_textColour;
    Hb_FontSetJustify(2);
    Hb_FontPrint(hb_nameX, 360.0f, Hb_TextGet(hb_text, "STO_ROTATE2"), 1);
    Hb_FontRender();
    if (hb_currArea == 0x38 && (hb_language == 7 || hb_language == 6)) {
        hb_boxOffset = -12.0f;
    } else {
        hb_boxOffset = 0.0f;
    }
    Hb_ShopDrawBox(hb_nameX, 240.0f, hb_boxOffset + 80.0f);
    Hb_ShopDrawBox(hb_nameX, 240.0f, 240.0f);
    Hb_ShopDrawBox(hb_nameX, 240.0f, 330.0f);
    if (self->previewing != 0 && self->previewEnd <= hb_menuTime && hb_padLeft == 0 && hb_padRight == 0 &&
        hb_padUp == 0 && hb_padDown == 0) {
        Hb_Restore(hb_clothing, &self->preview);
        hb_clothesChanged = 1;
        self->previewing = 0;
    }
}

} // namespace

#define HUDBARBER_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

HUDBARBER_REVERSED("HUDBarberImpl::Open", 0x006CCDB0, &Open);
HUDBARBER_REVERSED("HUDBarberImpl::SetGroupTitle", 0x006CCF10, &SetGroupTitle);
HUDBARBER_REVERSED("HUDBarberImpl::Construct", 0x006CCF20, &Construct);
HUDBARBER_REVERSED("HUDBarberImpl::Close", 0x006CCFD0, &Close);
HUDBARBER_REVERSED("HUDBarberImpl::AddHaircut", 0x006CD0B0, &AddHaircut);
HUDBARBER_REVERSED("HUDBarberImpl::Preview", 0x006CD190, &Preview);
HUDBARBER_REVERSED("HUDBarberImpl::DrawArrows", 0x006CD1F0, &DrawArrows);
HUDBARBER_REVERSED("HUDBarberImpl::Process", 0x006CD380, &Process);
HUDBARBER_REVERSED("HUDBarberImpl::Draw", 0x006CD6A0, &Draw);
