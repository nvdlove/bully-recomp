// The clothing and tattoo store screens (HUDClothingImpl, HUDTattooImpl; 0x6CDB90..
// 0x6D0B40): the per-slot lists of what's on offer or owned, the preview on the
// player, buying, the "saved outfit" history, and their drawing. The player's clothing
// (0x20C5F70, the 6 worn slots and the 0x54-byte outfit definitions) is another file's.
//
// HUDClothingImpl (by offset):
//   +0x0     the script event sender (the Events_* helpers take the object itself)
//   +0x4     the 4 button textures (heap)      +0x8  the HudCloth txd slot
//   +0xC     the current category: 0..5 the slots, 6 the outfits
//   +0x10    int[6] the selected entry per slot
//   +0x28    6 lists of 0x70C: 150 entries {item hash, slot, price} and the count (+0x708)
//   +0x2A70  byte: pick the first category on the next draw
//   +0x2A71  byte: previewing (the preview goes back when the time at +0x2ACC is up)
//   +0x2A74  the preview clothing set (0x58), +0x2ACC the preview's end time
//   +0x2AD0  the clothing worn on entering (0x58), +0x2B28 the outfit browsed (-1 none)
//   +0x2B2C  110 bought items {hash, slot} (8 bytes each), their count at +0x2E9C
// HUDTattooImpl:
//   +0x4     2 groups of 0xCC: 10 entries of 0x14 {tattoo (12), price, text hash} and
//            the count (+0xC8)
//   +0x19C   40 owned tattoos (12 each), their count at +0x37C
//   +0x380   the 2 button textures (heap)      +0x384 the HUDTattoo txd slot
//   +0x388   int[2] the selected entry per group; +0x390 the group
//   +0x395   byte: has entries; +0x396 previewing; +0x398 the entry previewed;
//   +0x39C   the preview's end time; +0x3A0 a clothing set (0x58)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

// a clothing item: (name hash, slot)
struct ClothItem {
    uint32_t hash;
    int32_t slot;
};
static_assert(sizeof(ClothItem) == 8);

// an entry of a category's list (12)
struct ClothEntry {
    uint32_t hash;   // +0
    int32_t slot;    // +4
    uint32_t price;  // +8
};
static_assert(sizeof(ClothEntry) == 12);

struct Rgba {
    uint8_t r, g, b, a;
};

constexpr int kListStride = 0x70C;
constexpr int kOutfits = 6;   // the category past the 6 slots

// ---- game data ----
inline void* const g_clothes = reinterpret_cast<void*>(0x020C5F70);   // the player's clothing
inline uint8_t* const g_wornSlots = reinterpret_cast<uint8_t*>(0x020C5F70);   // 6 x 12: +0 hash, +8 slot
inline uint8_t* const g_wornSlotsEnd = reinterpret_cast<uint8_t*>(0x020C5FB8);
GAME_ARRAY(uint8_t, g_outfitDefs, 0x020C6B80);        // 0x54 each: +4 name hash, +8 the 6 slots, +0x50 flags
GAME_VAR(int32_t, g_currentOutfit, 0x020C76FC);       // -1: none
GAME_VAR(uint32_t, g_outfitFlags, 0x020C76F8);
GAME_VAR(int32_t, g_outfitSearch, 0x020C78E8);        // (the outfit iterators' index)
GAME_VAR(int32_t, g_ownedIterator, 0x020C78E4);       // (Clothes_NextOwned's index)
GAME_VAR(int32_t, g_tattooIterator, 0x020C78EC);      // (Clothes_NextTattoo's index)
GAME_VAR(int32_t, g_tattooRemoveIterator, 0x020C78E0);
GAME_VAR(uint8_t, g_clothesChanged, 0x020C78F8);
GAME_VAR(uint32_t, g_storeOutfitKey, 0x020C7BB4);
GAME_VAR(int32_t, g_noneHashA, 0x020C5F6C);         // (the "nothing worn" items)
GAME_VAR(int32_t, g_noneHashB, 0x020C7BB0);     // the outfit the store's "outfits" show
GAME_ARRAY(uint8_t, g_clothingLocks, 0x020C7B90);     // [7] per category (wardrobe)
GAME_ARRAY(uint8_t, g_storeLocks, 0x020C7B97);        // [7] per category (stores)
GAME_VAR(uint8_t, g_exitButton, 0x020C7B9E);
GAME_VAR(Rgba, g_lockedColour, 0x020C7BD4);
GAME_VAR(int32_t, g_currArea, 0x00BD1008);
GAME_VAR(uint32_t, g_menuTime, 0x00A14A50);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);          // its money at +0x1CA0
inline void* const g_playerInfo = reinterpret_cast<void*>(0x00C1AEA8);
inline void* const g_shop = reinterpret_cast<void*>(0x00C18748);
inline void* const g_camera = reinterpret_cast<void*>(0x00C3CC68);   // TheCamera
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_ARRAY(const char*, g_slotTitles, 0x00BC5BDC);    // CLT_HEAD..., by category
GAME_ARRAY(const char*, g_slotItemTitles, 0x00BC5BF8);   // CLT_OFITITEM_H...
GAME_VAR(const char*, g_buyOutfitTitle, 0x00BC5C10);  // CLT_BUYOFIT
GAME_VAR(const char*, g_noItemsText, 0x00BC5C14);     // CLT_NOITEMS
GAME_VAR(const char*, g_totalCostText, 0x00BC5C18);   // CLT_TOTCOST
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(float, g_rowGap, 0x00A14930);
GAME_VAR(float, g_rowHeight, 0x00A14940);
GAME_VAR(float, g_nameX, 0x00A14980);
GAME_VAR(float, g_moneyX, 0x00A14984);
GAME_VAR(uint32_t, g_fontScale, 0x00BA4E10);          // (a float, kept as bits)
GAME_VAR(Rgba, g_fontColour, 0x0208B764);
GAME_VAR(Rgba, g_fontDropColour, 0x0208B780);
GAME_VAR(float, g_fontWrapCentre, 0x0208B778);
GAME_VAR(Rgba, g_infoColour, 0x00C19228);
GAME_VAR(Rgba, g_titleColour, 0x00C1922C);
GAME_VAR(Rgba, g_textColour, 0x00C19230);
GAME_VAR(Rgba, g_listColour, 0x00C19234);
GAME_VAR(Rgba, g_flashColour, 0x00CF01A4);
inline void* const g_panelColour = reinterpret_cast<void*>(0x00C18744);
GAME_ARRAY(uint8_t, g_padLeft, 0x020C3280);
GAME_ARRAY(uint8_t, g_padRight, 0x020C327C);
GAME_ARRAY(uint8_t, g_padUp, 0x020C3278);
GAME_ARRAY(uint8_t, g_padDown, 0x020C3274);
inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);

// ---- the player's clothing (0x20C5F70) ----
GAME_FN(void, __thiscall, ClothItem_FromHash, 0x006CA4F0, ClothItem* item, uint32_t hash, int slot);
GAME_FN(uint8_t, __thiscall, Clothes_PlayerOwns, 0x006CA850, void* clothes, const void* item);
GAME_FN(uint8_t*, __thiscall, Clothes_NextOwned, 0x006CA890, void* clothes, int slot, uintptr_t);
GAME_FN(uint8_t*, __thiscall, Clothes_FindOutfit, 0x006CAA90, void* clothes, uint32_t key);
GAME_FN(uint8_t*, __thiscall, Clothes_NextOutfit, 0x006CAD80, void* clothes, int32_t* index);
GAME_FN(uint8_t*, __thiscall, Clothes_PrevOutfit, 0x006CADB0, void* clothes, int32_t* index);
GAME_FN(void, __thiscall, Clothes_Backup, 0x006CAE80, void* clothes, void* set);
GAME_FN(void, __thiscall, Clothes_Restore, 0x006CB9B0, void* clothes, void* set);
GAME_FN(void, __thiscall, Clothes_SetPlayer, 0x006CB670, void* clothes, const void* item);
GAME_FN(void, __thiscall, Clothes_GivePlayer, 0x006CB6C0, void* clothes, const void* item, int flag);
GAME_FN(void, __thiscall, Clothes_SetPlayerOutfit, 0x006CB720, void* clothes, uint32_t key, int flag);
GAME_FN(void, __thiscall, Clothes_Fn6CA940, 0x006CA940, void* clothes, int);
GAME_FN(uint8_t*, __thiscall, Clothes_NextTattoo, 0x006CAE20, void* clothes, uintptr_t);
GAME_FN(void, __thiscall, Clothes_RemoveTattoo, 0x006CADE0, void* clothes, void* tattoo);
GAME_FN(uint8_t*, __cdecl, Clothes_CurrentOutfit, 0x005BA890);
GAME_FN(void, __thiscall, ClothSet_Apply, 0x006CA6E0, void* set, int32_t outfit);
GAME_FN(void, __thiscall, ClothSlot_Construct, 0x006CB310, void* slot);
GAME_FN(void, __thiscall, ClothItem_Construct, 0x006CBB00, void* item);
GAME_FN(void, __thiscall, Tattoo_Construct, 0x006CB3D0, void* tattoo);
GAME_FN(void, __thiscall, Tattoo_Set, 0x006CA560, void* tattoo, uintptr_t name, int group, int index);
GAME_FN(const char*, __cdecl, Tattoo_GroupTitle, 0x006CA4B0, int group);
GAME_FN(void*, __cdecl, ClothingOverlay_Get, 0x006CA7C0);
GAME_FN(uint32_t*, __thiscall, ClothingOverlay_Build, 0x006CC320, void* overlay, void* out, void* item);
GAME_FN(void, __cdecl, Clothes_SetRefresh, 0x006CC2C0, int flag);   // (the byte at 0x20C7BD0)
GAME_FN(void*, __thiscall, ClothItem_Construct5BA870, 0x005BA870, ClothItem* item, uint32_t hash, int slot);

// ---- the rest ----
GAME_FN(uint8_t, __thiscall, Events_IsOn, 0x005D80E0, void* events);
GAME_FN(void*, __thiscall, Events_New, 0x005D9C20, void* events);
GAME_FN(int32_t, __thiscall, Events_GetId, 0x005D80D0, void* events);
GAME_FN(void, __thiscall, Events_Reset, 0x005D9C50, void* events);
GAME_FN(void, __cdecl, Event_Begin, 0x005D8270, void* event);
GAME_FN(void, __cdecl, Event_SetId, 0x005D8670, void* event, int32_t id);
GAME_FN(void, __cdecl, Event_AddInt, 0x005D85C0, void* event, int32_t value);
GAME_FN(void, __cdecl, Event_Send, 0x005D82A0, void* event, int kind, int arg);
GAME_FN(void, __thiscall, Ped_TakeMoney, 0x00473100, void* playerInfo, int32_t amount);
GAME_FN(void, __cdecl, Stats_Add, 0x00458480, int32_t stat, int32_t value);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(int32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __cdecl, Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetStyle, 0x00688050, int style);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(float, __cdecl, Font_GetLineHeight, 0x00688220);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int withSpaces);
GAME_FN(void, __cdecl, Font_Measure, 0x0068B090, float* rect, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, Font_Print, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(Rgba*, __thiscall, Rgba_Set, 0x00510DE0, Rgba* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const Rgba* colour,
        const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, FE_DrawBox, 0x006A4420, float x1, float y1, float x2, float y2, float border, void* fill, void* line);
GAME_FN(void, __cdecl, FE_RequestSound, 0x006A2340, int sound);
GAME_FN(void, __cdecl, FE_RequestSoundDefault, 0x006A2360);
GAME_FN(uint8_t, __cdecl, FE_PadRepeatReady, 0x006A3170, int pad);
GAME_FN(void, __cdecl, FE_PadClear, 0x006A3120, int pad);
GAME_FN(void, __cdecl, FE_PlayMenuSound, 0x006A31B0);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int control, int pad);
GAME_FN(int, __thiscall, Camera_GetFadeState, 0x004F12F0, void* camera);
GAME_FN(void, __thiscall, Shop_StartFlash, 0x004551A0, void* shop, int state);
GAME_FN(void, __stdcall, Shop_DrawBox, 0x00455390, float x, float y, float h);
GAME_FN(void, __cdecl, Shop_DrawPanelBackground, 0x004556F0);
GAME_FN(void, __cdecl, Shop_DrawMoneyAlways, 0x00455EF0);
GAME_FN(void, __thiscall, Shop_DrawItemName, 0x00456790, void* shop, int32_t hash, int32_t price, int purchased);
GAME_FN(void, __thiscall, UserInput_SetEnabled, 0x0050DFA0, void* input, int enabled);
inline void* const g_userInput = reinterpret_cast<void*>(0x00A45A30);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(int, __cdecl, Txd_FindSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, RemoveModel, 0x0052F640, int id);
GAME_FN(void, __cdecl, LoadingScreen_Message, 0x0043C4F0, const char* title, const char* text, uintptr_t);
GAME_FN(void, __cdecl, LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __cdecl, Txd_Push, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_Pop, 0x005F0AA0);
GAME_FN(void, __cdecl, Txd_SetCurrent, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, Texture_Destroy, 0x005F0F90, void* texture);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, uint32_t size, int count, void* ctor);

constexpr int kTxdBase = 0x2FA8;   // (a txd slot's streaming id)

// the stores (not the wardrobe): clothing, barber, the school store
bool InStore() {
    return g_currArea == 0x21 || g_currArea == 0x22 || g_currArea == 2 || g_currArea == 0x38;
}

ClothEntry* Entry(uint8_t* self, int category, int index) {
    return reinterpret_cast<ClothEntry*>(self + category * kListStride + 0x28 + index * 12);
}
uint32_t& ListCount(uint8_t* self, int category) {
    return At<uint32_t>(self, category * kListStride + 0x730);
}
int32_t& Selected(uint8_t* self, int category) {
    return At<int32_t>(self, 0x10 + category * 4);
}
ClothItem* Bought(uint8_t* self) {
    return reinterpret_cast<ClothItem*>(self + 0x2B2C);
}
uint32_t& BoughtCount(uint8_t* self) {
    return At<uint32_t>(self, 0x2E9C);
}

// already bought here (the hashes)
bool IsBought(uint8_t* self, uint32_t hash) {
    for (uint32_t i = 0; i < BoughtCount(self); i++) {
        if (Bought(self)[i].hash == hash) {
            return true;
        }
    }
    return false;
}

void SetFontColour(const Rgba& colour) {
    g_fontColour = colour;
}

// the screen's title at the top, centred on its height
void DrawTitle(const uint16_t* title) {
    float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    Font_Reset();
    SetFontColour(g_titleColour);
    Font_SetJustify(2);
    g_fontWrapCentre = 240.0f;
    Font_Measure(rect, g_nameX, 55.0f, title);
    float height = std::fabs(rect[3] - rect[1]);
    Font_Print(g_nameX, 55.0f - height * 0.5f, title, 1);
    Font_Render();
}

// the arrows either side of a category's name (mirrored: the left one's texture drawn
// flipped, the tattoo screen's)
void DrawArrows(float x, float y, const uint16_t* text, void* left, void* right, bool mirrored) {
    float width = Font_GetStringWidth(text, 1) * 0.5f;
    float arrowW = g_screenScaleX * 15.0f;
    y = y - 8.75f;
    float x1 = (x - width) - 8.0f;
    float y2 = y + g_screenScaleY * 26.25f;
    float zero[2] = {0.0f, 0.0f};
    float one[2] = {1.0f, 1.0f};
    Rgba colour;
    Rgba_Set(&colour, 0xD7, 0xD7, 0xD7, 0xFF);
    DrawRectUV(x1 - arrowW, y, x1, y2, left, &colour, mirrored ? one : zero, mirrored ? zero : one);
    float x2 = width + x + 8.0f;
    Rgba_Set(&colour, 0xD7, 0xD7, 0xD7, 0xFF);
    DrawRectUV(x2, y, x2 + arrowW, y2, right, &colour, zero, one);
}

// this file's functions, called at the game's addresses (so modes apply)
GAME_FN(uint8_t*, __thiscall, G_HUDClothing_NextOutfit, 0x006CDC20, uint8_t* self);
GAME_FN(uint8_t*, __thiscall, G_HUDClothing_PrevOutfit, 0x006CDC70, uint8_t* self);
GAME_FN(void, __thiscall, G_HUDStore_SendEvent, 0x006CDD10, uint8_t* self, int32_t what, int32_t value);
GAME_FN(void, __thiscall, G_HUDClothing_InitStore, 0x006CDE20, uint8_t* self);
GAME_FN(void, __thiscall, G_HUDClothing_Buy, 0x006CDF80, uint8_t* self, ClothEntry* entry);
GAME_FN(void, __thiscall, G_HUDClothing_Preview, 0x006CE030, uint8_t* self, ClothEntry* entry);
GAME_FN(void, __thiscall, G_HUDClothing_PreviewOutfit, 0x006CE0E0, uint8_t* self, int32_t outfit);
GAME_FN(void, __thiscall, G_HUDClothing_SelectWorn, 0x006CE110, uint8_t* self);
GAME_FN(uint8_t, __thiscall, G_HUDClothing_InStore, 0x006CE170, uint8_t* self);
GAME_FN(uint8_t, __thiscall, G_HUDClothing_IsBought, 0x006CE190, uint8_t* self, ClothItem* item, int orOwned);
GAME_FN(void, __thiscall, G_HUDClothing_DrawArrows, 0x006CE1F0, uint8_t* self, float x, float y, const uint16_t* text);
GAME_FN(void, __thiscall, G_HUDClothing_InitWardrobe, 0x006CE380, uint8_t* self);
GAME_FN(void, __thiscall, G_HUDClothing_LeaveStore, 0x006CE420, uint8_t* self);
GAME_FN(void, __thiscall, G_HUDClothing_BuyOutfit, 0x006CE640, uint8_t* self);
GAME_FN(void, __thiscall, G_HUDClothing_DrawWardrobe, 0x006CE7B0, uint8_t* self);
GAME_FN(void, __thiscall, G_HUDClothing_DrawBill, 0x006CEA90, uint8_t* self, uint32_t* total);
GAME_FN(uint8_t, __thiscall, G_HUDClothing_CanPick, 0x006CEEA0, uint8_t* self, int category);
GAME_FN(void, __thiscall, G_HUDClothing_DrawStore, 0x006CF0A0, uint8_t* self);
GAME_FN(void, __thiscall, G_HUDClothing_StoreInput, 0x006CF3C0, uint8_t* self);
GAME_FN(void, __thiscall, G_HUDClothing_WardrobeInput, 0x006CF670, uint8_t* self);
GAME_FN(void, __thiscall, G_TattooList_Add, 0x006CFE90, uint8_t* self, const uint32_t* tattoo);
GAME_FN(uint8_t*, __thiscall, G_HUDTattoo_GetSelected, 0x006CFEC0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, G_HUDTattoo_IsOwned, 0x006D0150, uint8_t* self, const int32_t* tattoo);
GAME_FN(void, __thiscall, G_HUDTattoo_Preview, 0x006D0190, uint8_t* self, void*);
GAME_FN(void, __thiscall, G_HUDTattoo_DrawArrows, 0x006D01D0, uint8_t* self, float x, float y, const uint16_t* text);
GAME_FN(void, __thiscall, G_HUDTattoo_Input, 0x006D0440, uint8_t* self);
GAME_FN(void, __stdcall, G_ClothSet_FromStoreOutfit, 0x006CDB90, uint8_t* set);
GAME_FN(uint8_t, __stdcall, G_HUDClothing_OwnsChoice, 0x006CDCC0, int slot);

// more than one owned for the slot (0x6CDCC0, inlined in the draw)
bool OwnsChoice(int slot) {
    int count = 0;
    g_ownedIterator = 0;
    uint8_t* item = Clothes_NextOwned(g_clothes, slot, 0);
    while (item != nullptr) {
        count++;
        if (count >= 2) {
            return true;
        }
        item = Clothes_NextOwned(g_clothes, slot, 0);
    }
    return false;
}

} // namespace

#define HUDCLOTH_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// ---------------------------------------------------------------------------------------
// HUDClothingImpl

// 0x6CDB90 (stdcall): a clothing set gets the store outfit's 6 slots, then is applied
void __stdcall ClothSet_FromStoreOutfit(uint8_t* set) {
    uint8_t* outfit = Clothes_FindOutfit(g_clothes, g_storeOutfitKey);
    if (outfit != nullptr) {
        for (int i = 0; i < 0x48; i += 4) {
            At<uint32_t>(set, i) = At<uint32_t>(outfit, 8 + i);
        }
    }
    ClothSet_Apply(set, -1);
}
HUDCLOTH_REVERSED("HUDClothingImpl::SetFromStoreOutfit", 0x006CDB90, &ClothSet_FromStoreOutfit);

// 0x6CDC20 (thiscall): the next outfit (wrapping); returns its definition
uint8_t* __fastcall HUDClothing_NextOutfit(uint8_t* self, re::Edx) {
    int32_t* index = &At<int32_t>(self, 0x2B28);
    if (*index < 0) {
        *index = -1;
    }
    g_outfitSearch = *index + 1;
    uint8_t* outfit = Clothes_NextOutfit(g_clothes, index);
    if (outfit == nullptr) {
        g_outfitSearch = 0;
        outfit = Clothes_NextOutfit(g_clothes, index);
    }
    return outfit;
}
HUDCLOTH_REVERSED("HUDClothingImpl::NextOutfit", 0x006CDC20, &HUDClothing_NextOutfit);

// 0x6CDC70 (thiscall): the previous outfit (wrapping from 0x22)
uint8_t* __fastcall HUDClothing_PrevOutfit(uint8_t* self, re::Edx) {
    int32_t* index = &At<int32_t>(self, 0x2B28);
    if (*index < 0) {
        *index = 0x23;
    }
    g_outfitSearch = *index - 1;
    uint8_t* outfit = Clothes_PrevOutfit(g_clothes, index);
    if (outfit == nullptr) {
        g_outfitSearch = 0x22;
        outfit = Clothes_PrevOutfit(g_clothes, index);
    }
    return outfit;
}
HUDCLOTH_REVERSED("HUDClothingImpl::PrevOutfit", 0x006CDC70, &HUDClothing_PrevOutfit);

// 0x6CDCC0 (stdcall): the player owns more than one item for the slot
uint8_t __stdcall HUDClothing_OwnsChoice(int slot) {
    int count = 0;
    g_ownedIterator = 0;
    uint8_t* item = Clothes_NextOwned(g_clothes, slot, 0);
    while (item != nullptr) {
        count++;
        if (count >= 2) {
            return 1;
        }
        item = Clothes_NextOwned(g_clothes, slot, 0);
    }
    return 0;
}
HUDCLOTH_REVERSED("HUDClothingImpl::OwnsChoice", 0x006CDCC0, &HUDClothing_OwnsChoice);

// 0x6CDD10 (thiscall): tells the script (event kind 2) what happened: (what, value)
void __fastcall HUDStore_SendEvent(uint8_t* self, re::Edx, int32_t what, int32_t value) {
    if (Events_IsOn(self) == 0) {
        return;
    }
    void* event = Events_New(self);
    Event_Begin(event);
    Event_SetId(event, Events_GetId(self));
    Event_AddInt(event, what);
    Event_AddInt(event, value);
    Event_Send(event, 2, 0);
}
HUDCLOTH_REVERSED("HUDClothingImpl::SendEvent", 0x006CDD10, &HUDStore_SendEvent);

// 0x6CDD70 (thiscall): the constructor
uint8_t* __fastcall HUDClothing_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0;
    for (int i = 0; i < 6; i++) {
        At<uint32_t>(self, 0x730 + i * kListStride) = 0;
    }
    for (int i = 0; i < 6; i++) {
        ClothSlot_Construct(self + 0x2A74 + i * 12);
    }
    ClothSlot_Construct(self + 0x2A74 + 0x4C);
    for (int i = 0; i < 6; i++) {
        ClothSlot_Construct(self + 0x2AD0 + i * 12);
    }
    ClothSlot_Construct(self + 0x2AD0 + 0x4C);
    for (int i = 0; i < 110; i++) {
        ClothItem_Construct(self + 0x2B2C + i * 8);
    }
    BoughtCount(self) = 0;
    At<uint32_t>(self, 4) = 0;
    At<int32_t>(self, 8) = -1;
    return self;
}
HUDCLOTH_REVERSED("HUDClothingImpl::HUDClothingImpl", 0x006CDD70, &HUDClothing_Construct);

// 0x6CDE20 (thiscall): the store's lists: in each slot the item worn (or the store
// outfit's when an outfit with flag 2 is on), which counts as bought
void __fastcall HUDClothing_InitStore(uint8_t* self, re::Edx) {
    BoughtCount(self) = 0;
    Clothes_Backup(g_clothes, self + 0x2AD0);
    Clothes_Backup(g_clothes, self + 0x2A74);
    uint8_t* current = g_currentOutfit == -1 ? nullptr : g_outfitDefs + g_currentOutfit * 0x54;
    uint8_t* storeOutfit = Clothes_FindOutfit(g_clothes, g_storeOutfitKey);
    uint32_t* fromOutfit = reinterpret_cast<uint32_t*>(storeOutfit + 8);
    uint8_t* worn = g_wornSlots;
    uint8_t* count = self + 0x730;
    int slot = 0;
    do {
        uint32_t slotCopy[3];
        At<uint32_t>(count, 0) = 0;
        ClothSlot_Construct(slotCopy);
        uint32_t a, b, c;
        if (current != nullptr && (current[0x50] & 2) != 0 && storeOutfit != nullptr) {
            a = fromOutfit[0];
            b = fromOutfit[1];
            c = fromOutfit[2];
        } else {
            a = At<uint32_t>(worn, 0);
            b = At<uint32_t>(worn, 4);
            c = At<uint32_t>(worn, 8);
        }
        slotCopy[0] = a;
        slotCopy[1] = b;
        slotCopy[2] = c;
        ClothEntry* entry = reinterpret_cast<ClothEntry*>(count + (At<int32_t>(count, 0) - 0x96) * 12);
        entry->hash = a;
        entry->slot = slot;
        entry->price = 0;
        At<int32_t>(count, 0) += 1;
        ClothItem item;
        ClothItem_FromHash(&item, a, int8_t(c));
        fromOutfit += 3;
        Bought(self)[BoughtCount(self)] = item;
        BoughtCount(self) += 1;
        worn += 12;
        slot++;
        count += kListStride;
    } while (worn < g_wornSlotsEnd);
    for (int i = 0; i < 6; i++) {
        Selected(self, i) = 0;
    }
}
HUDCLOTH_REVERSED("HUDClothingImpl::InitStore", 0x006CDE20, &HUDClothing_InitStore);

// 0x6CDF80 (thiscall): buys an entry: pays, remembers it, puts it on, stats and the
// script event 9
void __fastcall HUDClothing_Buy(uint8_t* self, re::Edx, ClothEntry* entry) {
    ClothItem item;
    ClothItem_FromHash(&item, entry->hash, entry->slot);
    int32_t outfit = At<int32_t>(self, 0x2B18);
    if (outfit >= 0 && BoughtCount(self) == 6 && (g_outfitDefs[outfit * 0x54 + 0x50] & 2) != 0) {
        G_ClothSet_FromStoreOutfit(self + 0x2AD0);
    }
    Ped_TakeMoney(g_playerPed, entry->price);
    Bought(self)[BoughtCount(self)] = item;
    BoughtCount(self) += 1;
    Stats_Add(0x42, 1);
    Stats_Add(0x3C, entry->price);
    G_HUDStore_SendEvent(self, 9, int32_t(item.hash));
}
HUDCLOTH_REVERSED("HUDClothingImpl::Buy", 0x006CDF80, &HUDClothing_Buy);

// 0x6CE030 (thiscall): previews an entry on the player
void __fastcall HUDClothing_Preview(uint8_t* self, re::Edx, ClothEntry* entry) {
    At<uint8_t>(self, 0x2A71) = 1;
    int32_t outfit = At<int32_t>(self, 0x2B28);
    if (outfit >= 0 && (g_outfitDefs[outfit * 0x54 + 0x50] & 2) != 0) {
        G_ClothSet_FromStoreOutfit(self + 0x2A74);
    }
    ClothSet_Apply(self + 0x2A74, -1);
    At<int32_t>(self, 0x2B28) = -1;
    uint32_t* slot = reinterpret_cast<uint32_t*>(self + 0x2A74 + entry->slot * 12);
    ClothItem item;
    ClothItem_FromHash(&item, entry->hash, entry->slot);
    uint32_t built[3];
    uint32_t* result = ClothingOverlay_Build(ClothingOverlay_Get(), built, &item);
    slot[0] = result[0];
    slot[1] = result[1];
    slot[2] = result[2];
    At<uint32_t>(self, 0x2ACC) = g_menuTime + 0x4B0;
}
HUDCLOTH_REVERSED("HUDClothingImpl::Preview", 0x006CE030, &HUDClothing_Preview);

// 0x6CE0E0 (thiscall): previews an outfit
void __fastcall HUDClothing_PreviewOutfit(uint8_t* self, re::Edx, int32_t outfit) {
    At<uint8_t>(self, 0x2A71) = 1;
    ClothSet_Apply(self + 0x2A74, outfit);
    At<uint32_t>(self, 0x2ACC) = g_menuTime + 0x4B0;
}
HUDCLOTH_REVERSED("HUDClothingImpl::PreviewOutfit", 0x006CE0E0, &HUDClothing_PreviewOutfit);

// 0x6CE110 (thiscall): selects in each list the item worn
void __fastcall HUDClothing_SelectWorn(uint8_t* self, re::Edx) {
    for (int i = 0; i < 6; i++) {
        Selected(self, i) = 0;
    }
    uint8_t* worn = g_wornSlots;
    int category = 0;
    do {
        uint32_t count = ListCount(self, category);
        for (int j = 0; j < int32_t(count); j++) {
            if (Entry(self, category, j)->hash == At<uint32_t>(worn, 0)) {
                Selected(self, category) = j;
            }
        }
        worn += 12;
        category++;
    } while (worn < g_wornSlotsEnd);
}
HUDCLOTH_REVERSED("HUDClothingImpl::SelectWorn", 0x006CE110, &HUDClothing_SelectWorn);

// 0x6CE170: in a store (not the wardrobe)
uint8_t __fastcall HUDClothing_InStore(uint8_t*, re::Edx) {
    return InStore() ? 1 : 0;
}
HUDCLOTH_REVERSED("HUDClothingImpl::InStore", 0x006CE170, &HUDClothing_InStore);

// 0x6CE190 (thiscall): bought here, or (orOwned) owned
uint8_t __fastcall HUDClothing_IsBought(uint8_t* self, re::Edx, ClothItem* item, int orOwned) {
    if (IsBought(self, item->hash)) {
        return 1;
    }
    if (re::Flag(orOwned)) {
        return Clothes_PlayerOwns(g_clothes, item);
    }
    return 0;
}
HUDCLOTH_REVERSED("HUDClothingImpl::IsBought", 0x006CE190, &HUDClothing_IsBought);

// 0x6CE1F0 (thiscall): the arrows either side of the current category's name (lit by
// the pad's left / right)
void __fastcall HUDClothing_DrawArrows(uint8_t* self, re::Edx, float x, float y, const uint16_t* text) {
    re::Ptr32<void>* textures = At<re::Ptr32<void>*>(self, 4);
    void* right = textures[0];
    void* left = textures[2];
    if (g_padLeft[0] != 0) {
        left = textures[3];
    }
    if (g_padRight[0] != 0) {
        right = textures[1];
    }
    DrawArrows(x, y, text, left, right, false);
}
HUDCLOTH_REVERSED("HUDClothingImpl::DrawArrows", 0x006CE1F0, &HUDClothing_DrawArrows);

// 0x6CE380 (thiscall): the wardrobe's lists: what the player owns
void __fastcall HUDClothing_InitWardrobe(uint8_t* self, re::Edx) {
    Clothes_Backup(g_clothes, self + 0x2AD0);
    Clothes_Backup(g_clothes, self + 0x2A74);
    for (int slot = 0; slot < 6; slot++) {
        uint32_t& count = ListCount(self, slot);
        count = 0;
        g_ownedIterator = 0;
        uint8_t* item = Clothes_NextOwned(g_clothes, slot, 0);
        while (item != nullptr) {
            ClothEntry* entry = Entry(self, slot, int32_t(count));
            int32_t itemSlot = int8_t(item[4]);
            entry->hash = At<uint32_t>(item, 0);
            entry->slot = itemSlot;
            entry->price = 0;
            count++;
            item = Clothes_NextOwned(g_clothes, slot, 0);
        }
    }
    G_HUDClothing_SelectWorn(self);
}
HUDCLOTH_REVERSED("HUDClothingImpl::InitWardrobe", 0x006CE380, &HUDClothing_InitWardrobe);

// 0x6CE420 (thiscall): leaving a store: drops the 6 items worn on entry from the bought
// list; gives the player what was bought and puts on, per slot, the last bought (or
// what was worn on entry)
void __fastcall HUDClothing_LeaveStore(uint8_t* self, re::Edx) {
    ClothItem* bought = Bought(self);
    for (int n = 6; n != 0; n--) {
        uint32_t j = 0;
        // (a do-while: runs once even with an empty list)
        if (BoughtCount(self) - 1 != 0) {
            do {
                bought[j] = bought[j + 1];
                j++;
            } while (j < BoughtCount(self) - 1);
        }
        BoughtCount(self) -= 1;
    }
    if (BoughtCount(self) == 0) {
        Clothes_Restore(g_clothes, self + 0x2AD0);
        g_clothesChanged = 1;
        return;
    }
    for (uint32_t i = 0; i < BoughtCount(self); i++) {
        if (Clothes_PlayerOwns(g_clothes, &bought[i]) == 0) {
            Clothes_GivePlayer(g_clothes, &bought[i], 1);
        }
    }
    uint8_t* worn = g_wornSlots;
    uint8_t* entry = self + 0x2AD0;
    int slot = 0;
    do {
        ClothItem current;
        ClothItem_FromHash(&current, At<uint32_t>(worn, 0), int8_t(worn[8]));
        ClothItem found;
        ClothItem_Construct(&found);
        ClothItem entered;
        const ClothItem* put = nullptr;
        if (Clothes_PlayerOwns(g_clothes, &current) != 0) {
            put = &current;
        } else {
            for (int k = int32_t(BoughtCount(self)) - 1; k >= 0; k--) {
                if (int8_t(bought[k].slot) == slot) {
                    found = bought[k];
                    put = &found;
                    break;
                }
            }
            if (put == nullptr) {
                ClothItem_FromHash(&entered, At<uint32_t>(entry, 0), int8_t(entry[8]));
                put = &entered;
            }
        }
        Clothes_SetPlayer(g_clothes, put);
        worn += 12;
        slot++;
        entry += 12;
    } while (worn < g_wornSlotsEnd);
    g_clothesChanged = 1;
}
HUDCLOTH_REVERSED("HUDClothingImpl::LeaveStore", 0x006CE420, &HUDClothing_LeaveStore);

// 0x6CE5A0 (thiscall): adds an item to its slot's list (the script's ClothingStoreAdd);
// in a store, not one the player owns already
void __fastcall HUDClothing_Add(uint8_t* self, re::Edx, ClothItem* item, uint32_t price) {
    if (InStore()) {
        g_ownedIterator = 0;
        uint8_t* owned = Clothes_NextOwned(g_clothes, int8_t(item->slot), 0);
        while (owned != nullptr) {
            if (At<uint32_t>(owned, 0) == item->hash && int8_t(owned[4]) == int8_t(item->slot)) {
                return;
            }
            owned = Clothes_NextOwned(g_clothes, int8_t(item->slot), 0);
        }
    }
    int slot = int8_t(item->slot);
    uint32_t& count = ListCount(self, slot);
    ClothEntry* entry = Entry(self, slot, int32_t(count));
    entry->hash = item->hash;
    entry->slot = slot;
    entry->price = price;
    count++;
}
HUDCLOTH_REVERSED("HUDClothingImpl::Add", 0x006CE5A0, &HUDClothing_Add);

// the selected entries not bought yet (for the outfit's price)
int CollectUnbought(uint8_t* self, ClothEntry* out, uint32_t* total, bool slotFromList) {
    int count = 0;
    for (int category = 0; category < 6; category++) {
        ClothEntry* entry = Entry(self, category, Selected(self, category));
        ClothItem item;
        ClothItem_FromHash(&item, entry->hash, slotFromList ? entry->slot : category);
        if (IsBought(self, item.hash)) {
            continue;
        }
        if (Clothes_PlayerOwns(g_clothes, &item) == 0) {
            out[count] = *entry;
            *total += entry->price;
            count++;
        }
    }
    return count;
}

// 0x6CE640 (thiscall): buys the whole outfit selected (what isn't owned yet)
void __fastcall HUDClothing_BuyOutfit(uint8_t* self, re::Edx) {
    ClothEntry toBuy[6];
    uint32_t total = 0;
    int count = CollectUnbought(self, toBuy, &total, true);
    if (count <= 0) {
        return;
    }
    if (At<uint32_t>(g_playerPed, 0x1CA0) < total) {
        Shop_StartFlash(g_shop, 1);
        if (Events_IsOn(self) != 0) {
            void* event = Events_New(self);
            Event_Begin(event);
            Event_SetId(event, Events_GetId(self));
            Event_AddInt(event, 8);
            Event_AddInt(event, int32_t(total));
            Event_Send(event, 2, 0);
        }
        return;
    }
    FE_RequestSound(0xB);
    for (int i = 0; i < count; i++) {
        G_HUDClothing_Buy(self, &toBuy[i]);
    }
}
HUDCLOTH_REVERSED("HUDClothingImpl::BuyOutfit", 0x006CE640, &HUDClothing_BuyOutfit);

// 0x6CE7B0 (thiscall): the wardrobe's text: the title, the item's name, the buttons
void __fastcall HUDClothing_DrawWardrobe(uint8_t* self, re::Edx) {
    DrawTitle(Text_Get(TheText, "CLT_MANAGER_TITLE"));
    Font_Reset();
    int32_t category = At<int32_t>(self, 0xC);
    int32_t outfit = At<int32_t>(self, 0x2B28);
    int32_t name;
    if (category == kOutfits) {
        if (outfit > -1) {
            name = At<int32_t>(g_outfitDefs, outfit * 0x54 + 4);
        } else {
            name = HashName("STO_NONE");
        }
    } else {
        name = int32_t(Entry(self, category, Selected(self, category))->hash);
        if (outfit > -1 && outfit * 0x54 != -0x20C6B80 && (g_outfitDefs[outfit * 0x54 + 0x50] & 2) != 0) {
            name = At<int32_t>(g_outfitDefs, outfit * 0x54 + 4);
        } else if (name == g_noneHashA || name == g_noneHashB) {
            name = HashName("STO_NONE");
        }
    }
    Shop_DrawItemName(g_shop, name, -1, 0);
    Font_Reset();
    Font_SetStyle(1);
    Font_SetJustify(2);
    SetFontColour(g_textColour);
    if (At<uint8_t>(self, 0x2A71) == 0) {
        Font_Print(g_nameX, 400.0f, Text_Get(TheText, "STO_ACCEPTEXIT"), 1);
        Font_Render();
    }
    if (g_exitButton == 0) {
        SetFontColour(g_lockedColour);
    }
    Font_Print(g_nameX, 420.0f, Text_Get(TheText, "STO_EXIT"), 1);
    Font_Render();
    if (At<uint8_t>(self, 0x2A71) == 0) {
        Font_Reset();
        Font_SetStyle(1);
        SetFontColour(g_textColour);
        Font_SetJustify(2);
        Font_Print(g_nameX, 340.0f, Text_Get(TheText, "STO_SAVE_OFIT"), 1);
        Font_Render();
    }
}
HUDCLOTH_REVERSED("HUDClothingImpl::DrawWardrobe", 0x006CE7B0, &HUDClothing_DrawWardrobe);

// 0x6CEA90 (thiscall): the outfit's bill: a box listing what isn't owned yet with the
// prices and the total (flashing when the player can't pay); *total gets the sum
void __fastcall HUDClothing_DrawBill(uint8_t* self, re::Edx, uint32_t* total) {
    *total = 0;
    Font_Reset();
    SetFontColour(g_infoColour);
    Font_SetStyle(1);
    ClothEntry rows[6];
    uint32_t unused = 0;
    int count = CollectUnbought(self, rows, &unused, false);
    float height = float(count + 1) * g_rowHeight;
    float x1 = g_moneyX - 110.0f;
    float x2 = x1 + 220.0f;
    float y = 240.0f - height * 0.5f;
    FE_DrawBox(x1 - 10.0f, y - 10.0f, x2 + 10.0f, (y + height) + 10.0f, 0.0f, g_panelColour, g_panelColour);
    uint16_t buffer[64];
    for (int i = 0; i < count; i++) {
        uint32_t price = rows[i].price;
        *total += price;
        float rowY = float(i) * g_rowHeight + y;
        Font_SetJustify(0);
        Font_Print(x1, rowY, Text_Get(TheText, g_slotItemTitles[rows[i].slot]), 1);
        Font_SetJustify(1);
        Font_AddNumber(float(int32_t(price)));
        Font_Format(buffer, Text_Get(TheText, "STO_MONEY"), 0x40);
        Font_Print(x2, rowY, buffer, 1);
    }
    y = float(count) * g_rowHeight + y;
    if (count <= 0) {
        Font_Print(x1, y, Text_Get(TheText, g_noItemsText), 1);
        return;
    }
    if (At<uint32_t>(g_playerPed, 0x1CA0) < *total) {
        SetFontColour(g_flashColour);
    }
    Font_SetJustify(0);
    Font_Print(x1, y, Text_Get(TheText, g_totalCostText), 1);
    Font_SetJustify(1);
    Font_AddNumber(float(int32_t(*total)));
    Font_Format(buffer, Text_Get(TheText, "STO_MONEY"), 0x40);
    Font_Print(x2, y, buffer, 1);
    SetFontColour(g_textColour);
}
HUDCLOTH_REVERSED("HUDClothingImpl::DrawBill", 0x006CEA90, &HUDClothing_DrawBill);

// 0x6CEEA0 (thiscall): a category can be picked: not locked, and with a choice (the
// wardrobe: more than one owned; a store: more than one on offer)
uint8_t __fastcall HUDClothing_CanPick(uint8_t* self, re::Edx, int category) {
    if (!InStore()) {
        if (category != kOutfits && G_HUDClothing_OwnsChoice(category) == 0) {
            return 0;
        }
        return g_clothingLocks[category] == 0 ? 1 : 0;
    }
    if (category != kOutfits && ListCount(self, category) < 2) {
        return 0;
    }
    return g_storeLocks[category] == 0 ? 1 : 0;
}
HUDCLOTH_REVERSED("HUDClothingImpl::CanPick", 0x006CEEA0, &HUDClothing_CanPick);

// 0x6CEF10 (thiscall): opening: its txd and buttons, the lists
void __fastcall HUDClothing_Open(uint8_t* self, re::Edx) {
    UserInput_SetEnabled(g_userInput, 0);
    At<re::Ptr32<void>*>(self, 4) = static_cast<re::Ptr32<void>*>(Malloc(0x10));
    int slot = Txd_FindSlot("HudCloth", 0);
    At<int32_t>(self, 8) = slot;
    RequestModel(slot + kTxdBase, 9);
    LoadingScreen_Message("HUDClothingImpl::Open()", "CStreaming::LoadAllRequestedModels()", 0);
    LoadAllRequestedModels(1);
    Txd_Push();
    Txd_SetCurrent(At<int32_t>(self, 8));
    At<re::Ptr32<void>*>(self, 4)[0] = Txd_FindTexture("buttonclothdown", "buttonclothdown_x");
    At<re::Ptr32<void>*>(self, 4)[1] = Txd_FindTexture("buttonclothnormal", "buttonclothnormal_x");
    At<re::Ptr32<void>*>(self, 4)[2] = Txd_FindTexture("lbuttonclothdown", "lbuttonclothdown_x");
    At<re::Ptr32<void>*>(self, 4)[3] = Txd_FindTexture("lbuttonclothnormal", "lbuttonclothnormal_x");
    Txd_Pop();
    if (InStore()) {
        G_HUDClothing_InitStore(self);
    } else {
        G_HUDClothing_InitWardrobe(self);
    }
    At<uint8_t>(self, 0x2A71) = 0;
    At<uint8_t>(self, 0x2A70) = 1;
    At<int32_t>(self, 0x2B28) = g_currentOutfit;
    G_HUDStore_SendEvent(self, 0, At<int32_t>(self, 0xC));
}
HUDCLOTH_REVERSED("HUDClothingImpl::Open", 0x006CEF10, &HUDClothing_Open);

// 0x6CF020 (thiscall): closing
void __fastcall HUDClothing_Close(uint8_t* self, re::Edx) {
    Events_Reset(self);
    UserInput_SetEnabled(g_userInput, 1);
    for (int i = 0; i < 0x10; i += 4) {
        Texture_Destroy(At<void*>(At<uint8_t*>(self, 4), i));
    }
    Free(At<void*>(self, 4));
    At<void*>(self, 4) = nullptr;
    RemoveModel(At<int32_t>(self, 8) + kTxdBase);
    At<int32_t>(self, 8) = -1;
    if (InStore()) {
        G_HUDClothing_LeaveStore(self);
    }
}
HUDCLOTH_REVERSED("HUDClothingImpl::Close", 0x006CF020, &HUDClothing_Close);

// 0x6CF0A0 (thiscall): a store's text: the title, the item (or the outfit's bill),
// the buttons
void __fastcall HUDClothing_DrawStore(uint8_t* self, re::Edx) {
    const uint16_t* title = nullptr;
    uint32_t total = 0;
    if (g_currArea == 0x21) {
        title = Text_Get(TheText, "CLT_RICHCLOTH_TITLE");
    } else if (g_currArea == 0x22) {
        title = Text_Get(TheText, "CLT_POORCLOTH_TITLE");
    } else if (g_currArea == 0x38) {
        title = Text_Get(TheText, "CLT_POORBARB_TITLE");
    } else if (g_currArea == 2) {
        title = Text_Get(TheText, "CLT_SCHOOLS_TITLE");
    }
    DrawTitle(title);
    Shop_DrawMoneyAlways();
    bool canBuy = false;
    int32_t category = At<int32_t>(self, 0xC);
    if (category == kOutfits) {
        G_HUDClothing_DrawBill(self, &total);
        if (int32_t(total) > 0 && At<uint8_t>(self, 0x2A71) == 0) {
            canBuy = true;
        }
    } else {
        ClothEntry* entry = Entry(self, category, Selected(self, category));
        int32_t price = int32_t(entry->price);
        int32_t name = int32_t(entry->hash);
        int32_t outfit = g_currentOutfit;
        if (outfit == -1 || outfit * 0x54 == -0x20C6B80 || (g_outfitDefs[outfit * 0x54 + 0x50] & 2) == 0) {
            if (name == g_noneHashA || name == g_noneHashB) {
                name = HashName("STO_NONE");
                price = -1;
            }
        } else {
            name = At<int32_t>(g_outfitDefs, outfit * 0x54 + 4);
            price = -1;
        }
        ClothItem item;
        ClothItem_FromHash(&item, entry->hash, entry->slot);
        uint8_t bought = IsBought(self, item.hash) ? 1 : Clothes_PlayerOwns(g_clothes, &item);
        Shop_DrawItemName(g_shop, name, price, bought != 0 ? 1 : 0);
        if (bought == 0 && At<uint8_t>(self, 0x2A71) == 0) {
            canBuy = true;
        }
    }
    Font_Reset();
    Font_SetStyle(1);
    Font_SetJustify(2);
    SetFontColour(g_textColour);
    if (canBuy && total <= At<uint32_t>(g_playerPed, 0x1CA0)) {
        Font_Print(g_nameX, 400.0f, Text_Get(TheText, "STO_BUYEXIT"), 1);
    }
    if (g_exitButton == 0) {
        SetFontColour(g_lockedColour);
    }
    Font_Print(g_nameX, 420.0f, Text_Get(TheText, "STO_EXIT"), 1);
    Font_Render();
}
HUDCLOTH_REVERSED("HUDClothingImpl::DrawStore", 0x006CF0A0, &HUDClothing_DrawStore);

// the next category that can be picked, from `from` by `step` (1 or -1, wrapping over
// 7); -1 when none
int NextCategory(uint8_t* self, int from, int step) {
    for (int i = 1; i <= 6; i++) {
        int category;
        if (step > 0) {
            category = (from + i) % 7;
        } else {
            category = from - i;
            if (category < 0) {
                category += 7;
            }
        }
        if (G_HUDClothing_CanPick(self, category) != 0) {
            return category;
        }
    }
    return -1;
}

// the next / previous entry of the current (slot) category
ClothEntry* StepEntry(uint8_t* self, int step) {
    int32_t category = At<int32_t>(self, 0xC);
    int32_t& selected = Selected(self, category);
    if (step > 0) {
        selected++;
        if (ListCount(self, category) <= uint32_t(selected)) {
            selected = 0;
        }
    } else {
        selected--;
        if (selected < 0) {
            selected = int32_t(ListCount(self, category)) - 1;
        }
    }
    return Entry(self, category, selected);
}

// 0x6CF3C0 (thiscall): a store's input
void __fastcall HUDClothing_StoreInput(uint8_t* self, re::Edx) {
    if (Camera_GetFadeState(g_camera) != 0) {
        return;
    }
    int step = 0;
    if (g_padRight[0] != 0 && FE_PadRepeatReady(0) != 0) {
        step = 1;
    } else if (g_padLeft[0] != 0 && FE_PadRepeatReady(0) != 0) {
        step = -1;
    }
    if (step != 0) {
        if (At<int32_t>(self, 0xC) != kOutfits) {
            ClothEntry* entry = StepEntry(self, step);
            G_HUDClothing_Preview(self, entry);
            G_HUDStore_SendEvent(self, 1, int32_t(entry->hash));
            FE_PadClear(0);
            Clothes_SetRefresh(1);
        }
        FE_PlayMenuSound();
        return;
    }
    int direction = 0;
    if (g_padDown[0] != 0 && FE_PadRepeatReady(0) != 0) {
        direction = 1;
    } else if (g_padUp[0] != 0 && FE_PadRepeatReady(0) != 0) {
        direction = -1;
    }
    if (direction != 0) {
        int category = NextCategory(self, At<int32_t>(self, 0xC), direction);
        if (category >= 0) {
            At<int32_t>(self, 0xC) = category;
            G_HUDStore_SendEvent(self, 0, category);
            FE_PadClear(0);
            Clothes_SetRefresh(1);
        }
        FE_PlayMenuSound();
        return;
    }
    if (Pad_JustPressed(7, 0) != 0 && At<uint8_t>(self, 0x2A71) == 0) {
        int32_t category = At<int32_t>(self, 0xC);
        if (category == kOutfits) {
            G_HUDClothing_BuyOutfit(self);
            FE_PlayMenuSound();
            return;
        }
        ClothEntry* entry = Entry(self, category, Selected(self, category));
        uint32_t price = entry->price;
        ClothItem item;
        ClothItem_Construct5BA870(&item, entry->hash, category);
        if (G_HUDClothing_IsBought(self, &item, 1) != 0) {
            Shop_StartFlash(g_shop, 2);
            G_HUDStore_SendEvent(self, 7, 1);
        } else if (price <= At<uint32_t>(g_playerPed, 0x1CA0)) {
            FE_RequestSound(0xB);
            G_HUDClothing_Buy(self, entry);
        } else {
            Shop_StartFlash(g_shop, 1);
            G_HUDStore_SendEvent(self, 8, int32_t(price));
        }
        FE_PlayMenuSound();
        return;
    }
    if (Pad_JustPressed(0xC, 0) != 0 && At<int32_t>(self, 0xC) != kOutfits) {
        FE_RequestSoundDefault();
    }
    FE_PlayMenuSound();
}
HUDCLOTH_REVERSED("HUDClothingImpl::StoreInput", 0x006CF3C0, &HUDClothing_StoreInput);

// 0x6CF670 (thiscall): the wardrobe's input
void __fastcall HUDClothing_WardrobeInput(uint8_t* self, re::Edx) {
    if (Camera_GetFadeState(g_camera) != 0) {
        return;
    }
    int step = 0;
    if (g_padRight[0] != 0 && FE_PadRepeatReady(0) != 0) {
        step = 1;
    } else if (g_padLeft[0] != 0 && FE_PadRepeatReady(0) != 0) {
        step = -1;
    }
    if (step != 0) {
        uint32_t value;
        if (At<int32_t>(self, 0xC) == kOutfits) {
            uint8_t* outfit = step > 0 ? G_HUDClothing_NextOutfit(self) : G_HUDClothing_PrevOutfit(self);
            G_HUDClothing_PreviewOutfit(self, At<int32_t>(self, 0x2B28));
            value = At<uint32_t>(outfit, 0);
        } else {
            ClothEntry* entry = StepEntry(self, step);
            G_HUDClothing_Preview(self, entry);
            value = entry->hash;
        }
        G_HUDStore_SendEvent(self, 1, int32_t(value));
        FE_PadClear(0);
        FE_PlayMenuSound();
        return;
    }
    int direction = 0;
    if (g_padDown[0] != 0 && FE_PadRepeatReady(0) != 0) {
        direction = 1;
    } else if (g_padUp[0] != 0 && FE_PadRepeatReady(0) != 0) {
        direction = -1;
    }
    if (direction != 0) {
        if (At<int32_t>(self, 0xC) == kOutfits) {
            // leaving the outfits: the store outfit's (flag 2) goes on
            uint8_t* outfit = Clothes_CurrentOutfit();
            if (outfit != nullptr && (outfit[0x50] & 2) != 0) {
                Clothes_SetPlayerOutfit(g_clothes, g_storeOutfitKey, 0);
                Clothes_Backup(g_clothes, self + 0x2A74);
            }
        }
        int category = NextCategory(self, At<int32_t>(self, 0xC), direction);
        if (category < 0) {
            FE_PlayMenuSound();
            return;
        }
        At<int32_t>(self, 0xC) = category;
        if (category == kOutfits && At<int32_t>(self, 0x2B28) != -1) {
            G_HUDClothing_PreviewOutfit(self, At<int32_t>(self, 0x2B28));
        }
        G_HUDStore_SendEvent(self, 0, At<int32_t>(self, 0xC));
        FE_PadClear(0);
        FE_PlayMenuSound();
        return;
    }
    if (Pad_JustPressed(7, 0) != 0 && At<uint8_t>(self, 0x2A71) == 0) {
        G_HUDStore_SendEvent(self, 10, -1);
        FE_PlayMenuSound();
        return;
    }
    if (Pad_JustPressed(9, 0) != 0 && At<uint8_t>(self, 0x2A71) == 0) {
        g_outfitFlags |= 0x10;
        Clothes_Fn6CA940(g_clothes, 0x22);
        G_HUDStore_SendEvent(self, 2, -1);
        FE_RequestSoundDefault();
        FE_PlayMenuSound();
        return;
    }
    if (Pad_JustPressed(0xC, 0) != 0 && At<int32_t>(self, 0xC) != kOutfits) {
        FE_RequestSoundDefault();
    }
    FE_PlayMenuSound();
}
HUDCLOTH_REVERSED("HUDClothingImpl::WardrobeInput", 0x006CF670, &HUDClothing_WardrobeInput);

// 0x6CF970 (thiscall): the screen: input, the text, the category list (greyed when it
// can't be picked), the help and the preview's end
void __fastcall HUDClothing_Draw(uint8_t* self, re::Edx) {
    uint32_t savedScale = g_fontScale;
    if (g_language == 7 || g_language == 6) {
        g_fontScale = 0x3F666666;   // 0.9
    }
    if (At<uint8_t>(self, 0x2A70) != 0) {
        At<int32_t>(self, 0xC) = 0;
        for (int i = 0; i < 7; i++) {
            if (G_HUDClothing_CanPick(self, i) != 0) {
                At<int32_t>(self, 0xC) = i;
                break;
            }
        }
        At<uint8_t>(self, 0x2A70) = 0;
    }
    Shop_DrawPanelBackground();
    if (InStore()) {
        G_HUDClothing_StoreInput(self);
        G_HUDClothing_DrawStore(self);
    } else {
        G_HUDClothing_WardrobeInput(self);
        G_HUDClothing_DrawWardrobe(self);
    }
    Font_Reset();
    Font_SetJustify(2);
    if (g_language == 7 || g_language == 6) {
        Font_SetScale(0.5f);
    } else {
        Font_SetStyle(2);
    }
    float centre = (240.0f + 80.0f) * 0.5f;
    float top = (centre - (Font_GetLineHeight() + g_rowGap) * 7.0f * 0.5f) + 5.0f;
    for (int i = 0; i <= 6; i++) {
        float y = (Font_GetLineHeight() + g_rowGap) * float(i) + top;
        SetFontColour(g_listColour);
        const char* key;
        if (i == kOutfits && G_HUDClothing_InStore(self) != 0) {
            key = g_buyOutfitTitle;
        } else {
            key = g_slotTitles[i];
        }
        const uint16_t* text = Text_Get(TheText, key);
        if (i == At<int32_t>(self, 0xC)) {
            SetFontColour(g_infoColour);
            if (i != kOutfits || G_HUDClothing_InStore(self) == 0) {
                G_HUDClothing_DrawArrows(self, g_nameX, y, text);
            }
        }
        bool locked;
        if (InStore()) {
            if (i != kOutfits && ListCount(self, i) <= 1) {
                locked = true;
            } else {
                locked = g_storeLocks[i] != 0;
            }
        } else if (i != kOutfits && !OwnsChoice(i)) {
            locked = true;
        } else {
            locked = g_clothingLocks[i] != 0;
        }
        Rgba drop;
        if (locked) {
            Rgba_Set(&drop, 0, 0, 0, 0);
            SetFontColour(g_lockedColour);
            g_fontDropColour = drop;
        } else {
            g_fontDropColour = *Rgba_Set(&drop, 0, 0, 0, 0xFF);
        }
        Font_Print(g_nameX, y - 8.75f, text, 1);
    }
    Font_Reset();
    if (g_language == 7 || g_language == 6) {
        Font_SetScale(0.5f);
    } else {
        Font_SetStyle(1);
    }
    SetFontColour(g_textColour);
    Font_SetJustify(2);
    if (At<int32_t>(self, 0xC) != kOutfits) {
        Font_Print(g_nameX, 380.0f, Text_Get(TheText, "STO_ROTATE"), 1);
    }
    Font_Print(g_nameX, 360.0f, Text_Get(TheText, "STO_ROTATE2"), 1);
    Font_Render();
    Shop_DrawBox(g_nameX, 240.0f, 80.0f);
    Shop_DrawBox(g_nameX, 240.0f, 240.0f);
    Shop_DrawBox(g_nameX, 240.0f, 330.0f);
    if (At<uint8_t>(self, 0x2A71) != 0 && At<uint32_t>(self, 0x2ACC) <= g_menuTime && g_padLeft[0] == 0 &&
        g_padRight[0] == 0 && g_padUp[0] == 0 && g_padDown[0] == 0) {
        Clothes_Restore(g_clothes, self + 0x2A74);
        G_HUDClothing_SelectWorn(self);
        g_clothesChanged = 1;
        At<uint8_t>(self, 0x2A71) = 0;
    }
    if (g_language == 7 || g_language == 6) {
        g_fontScale = savedScale;
    }
}
HUDCLOTH_REVERSED("HUDClothingImpl::Draw", 0x006CF970, &HUDClothing_Draw);

// ---------------------------------------------------------------------------------------
// HUDTattooImpl

// 0x6CFE80 (thiscall): an entry's constructor (for the array iterator)
uint8_t* __fastcall TattooEntry_Construct(uint8_t* self, re::Edx) {
    Tattoo_Construct(self);
    return self;
}
HUDCLOTH_REVERSED("HUDTattooImpl::Entry::Entry", 0x006CFE80, &TattooEntry_Construct);

// 0x6CFE90 (thiscall on the owned list at +0x19C): adds a tattoo
void __fastcall TattooList_Add(uint8_t* self, re::Edx, const uint32_t* tattoo) {
    int32_t& count = At<int32_t>(self, 0x1E0);
    uint32_t* slot = &At<uint32_t>(self, count * 12);
    slot[0] = tattoo[0];
    slot[1] = tattoo[1];
    slot[2] = tattoo[2];
    count++;
}
HUDCLOTH_REVERSED("HUDTattooImpl::OwnedList::Add", 0x006CFE90, &TattooList_Add);

uint8_t* TattooSelected(uint8_t* self) {
    int32_t group = At<int32_t>(self, 0x390);
    return self + group * 0xCC + 4 + At<int32_t>(self, 0x388 + group * 4) * 0x14;
}

// 0x6CFEC0 (thiscall): the entry selected
uint8_t* __fastcall HUDTattoo_GetSelected(uint8_t* self, re::Edx) {
    return TattooSelected(self);
}
HUDCLOTH_REVERSED("HUDTattooImpl::GetSelected", 0x006CFEC0, &HUDTattoo_GetSelected);

// 0x6CFEE0 (thiscall): the constructor
uint8_t* __fastcall HUDTattoo_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0;
    uint8_t* group = self + 4;
    for (int i = 0; i < 2; i++) {
        ConstructArray(group, 0x14, 10, reinterpret_cast<void*>(0x006CFE80));
        At<uint32_t>(group, 0xC8) = 0;
        group += 0xCC;
    }
    for (int i = 0; i < 40; i++) {
        Tattoo_Construct(self + 0x19C + i * 12);
    }
    At<uint32_t>(self, 0x19C + 0x1E0) = 0;
    for (int i = 0; i < 6; i++) {
        ClothSlot_Construct(self + 0x3A0 + i * 12);
    }
    ClothSlot_Construct(self + 0x3A0 + 0x4C);
    At<uint32_t>(self, 0x380) = 0;
    At<int32_t>(self, 0x384) = -1;
    At<uint8_t>(self, 0x395) = 0;
    return self;
}
HUDCLOTH_REVERSED("HUDTattooImpl::HUDTattooImpl", 0x006CFEE0, &HUDTattoo_Construct);

// 0x6CFF80 (thiscall): opening: its txd and buttons, the tattoos owned
void __fastcall HUDTattoo_Open(uint8_t* self, re::Edx) {
    UserInput_SetEnabled(g_userInput, 0);
    At<re::Ptr32<void>*>(self, 0x380) = static_cast<re::Ptr32<void>*>(Malloc(8));
    int slot = Txd_FindSlot("HUDTattoo", 0);
    At<int32_t>(self, 0x384) = slot;
    RequestModel(slot + kTxdBase, 9);
    LoadingScreen_Message("HUDTattooImpl::Open()", "CStreaming::LoadAllRequestedModels()", 0);
    LoadAllRequestedModels(1);
    Txd_Push();
    Txd_SetCurrent(At<int32_t>(self, 0x384));
    At<re::Ptr32<void>*>(self, 0x380)[1] = Txd_FindTexture("buttonctattoodown", "buttontattoodown_x");
    At<re::Ptr32<void>*>(self, 0x380)[0] = Txd_FindTexture("buttontattoonormal", "buttontattoonormal_x");
    Txd_Pop();
    At<uint32_t>(self, 0x388) = 0;
    At<uint32_t>(self, 0xCC) = 0;
    At<uint32_t>(self, 0x38C) = 0;
    At<uint32_t>(self, 0x198) = 0;
    At<uint32_t>(self, 0x37C) = 0;
    g_tattooIterator = 0;
    uint8_t* tattoo = Clothes_NextTattoo(g_clothes, 0);
    while (tattoo != nullptr) {
        uint32_t* owned = &At<uint32_t>(self, 0x19C + At<int32_t>(self, 0x37C) * 12);
        owned[0] = At<uint32_t>(tattoo, 0);
        owned[1] = At<uint32_t>(tattoo, 4);
        owned[2] = At<uint32_t>(tattoo, 8);
        At<int32_t>(self, 0x37C) += 1;
        tattoo = Clothes_NextTattoo(g_clothes, 0);
    }
    At<uint32_t>(self, 0x390) = 0;
    At<uint8_t>(self, 0x396) = 0;
}
HUDCLOTH_REVERSED("HUDTattooImpl::Open", 0x006CFF80, &HUDTattoo_Open);

// 0x6D00A0 (thiscall): closing: the preview comes off (every owned tattoo put back)
void __fastcall HUDTattoo_Close(uint8_t* self, re::Edx) {
    Events_Reset(self);
    UserInput_SetEnabled(g_userInput, 1);
    for (int i = 0; i < 8; i += 4) {
        Texture_Destroy(At<void*>(At<uint8_t*>(self, 0x380), i));
    }
    Free(At<void*>(self, 0x380));
    At<void*>(self, 0x380) = nullptr;
    RemoveModel(At<int32_t>(self, 0x384) + kTxdBase);
    At<int32_t>(self, 0x384) = -1;
    g_tattooRemoveIterator = 0;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x37C); i++) {
        Clothes_RemoveTattoo(g_clothes, self + 0x19C + i * 12);
    }
    At<uint8_t>(self, 0x395) = 0;
}
HUDCLOTH_REVERSED("HUDTattooImpl::Close", 0x006D00A0, &HUDTattoo_Close);

// 0x6D0150 (thiscall): the player has the tattoo (the first dword compared)
uint8_t __fastcall HUDTattoo_IsOwned(uint8_t* self, re::Edx, const int32_t* tattoo) {
    for (int i = 0; i < At<int32_t>(self, 0x37C); i++) {
        if (At<int32_t>(self, 0x19C + i * 12) == *tattoo) {
            return 1;
        }
    }
    return 0;
}
HUDCLOTH_REVERSED("HUDTattooImpl::IsOwned", 0x006D0150, &HUDTattoo_IsOwned);

// 0x6D0190 (thiscall, the argument unused): previews the entry selected
void __fastcall HUDTattoo_Preview(uint8_t* self, re::Edx, void*) {
    At<uint8_t>(self, 0x396) = 1;
    At<uint8_t*>(self, 0x398) = TattooSelected(self);
    At<uint32_t>(self, 0x39C) = g_menuTime + 0x4B0;
}
HUDCLOTH_REVERSED("HUDTattooImpl::Preview", 0x006D0190, &HUDTattoo_Preview);

// 0x6D01D0 (thiscall): the arrows either side of the group's name
void __fastcall HUDTattoo_DrawArrows(uint8_t* self, re::Edx, float x, float y, const uint16_t* text) {
    re::Ptr32<void>* textures = At<re::Ptr32<void>*>(self, 0x380);
    void* normal = textures[0];
    void* down = textures[1];
    DrawArrows(x, y, text, g_padLeft[0] == 0 ? normal : down, g_padRight[0] == 0 ? normal : down, true);
}
HUDCLOTH_REVERSED("HUDTattooImpl::DrawArrows", 0x006D01D0, &HUDTattoo_DrawArrows);

// 0x6D0360 (thiscall): adds a tattoo to a group (the script's TattooStoreAdd); the first
// one added is previewed
void __fastcall HUDTattoo_Add(uint8_t* self, re::Edx, int group, int index, uint32_t name, int32_t price,
                              int32_t text) {
    uint32_t entry[5];
    Tattoo_Construct(entry);
    Tattoo_Set(entry, name, group, index);
    entry[3] = uint32_t(price);
    entry[4] = uint32_t(text);
    uint8_t* groupBase = self + group * 0xCC + 4;
    int32_t& count = At<int32_t>(groupBase, 0xC8);
    uint32_t* slot = &At<uint32_t>(groupBase, count * 0x14);
    for (int i = 0; i < 5; i++) {
        slot[i] = entry[i];
    }
    count++;
    if (At<uint8_t>(self, 0x395) == 0) {
        At<int32_t>(self, 0x390) = group;
        At<uint8_t>(self, 0x396) = 1;
        At<uint8_t*>(self, 0x398) = self + group * 0xCC + At<int32_t>(self, 0x388 + group * 4) * 0x14 + 4;
        At<uint32_t>(self, 0x39C) = 0;
        G_HUDStore_SendEvent(self, 5, text);
    }
    At<uint8_t>(self, 0x395) = 1;
}
HUDCLOTH_REVERSED("HUDTattooImpl::Add", 0x006D0360, &HUDTattoo_Add);

// 0x6D0440 (thiscall): the input
void __fastcall HUDTattoo_Input(uint8_t* self, re::Edx) {
    if (Camera_GetFadeState(g_camera) != 0) {
        return;
    }
    int step = 0;
    if (g_padRight[0] != 0 && FE_PadRepeatReady(0) != 0) {
        step = 1;
    } else if (g_padLeft[0] != 0 && FE_PadRepeatReady(0) != 0) {
        step = -1;
    }
    uint8_t* entry;
    if (step != 0) {
        int32_t group = At<int32_t>(self, 0x390);
        int32_t& selected = At<int32_t>(self, 0x388 + group * 4);
        int32_t count = At<int32_t>(self, (At<int32_t>(self, 0x390) + 1) * 0xCC);
        if (step > 0) {
            selected++;
            if (selected == count) {
                selected = 0;
            }
        } else {
            selected--;
            if (selected < 0) {
                selected = count - 1;
            }
        }
        entry = TattooSelected(self);
        G_HUDTattoo_Preview(self, entry);
    } else {
        int direction = 0;
        if (g_padDown[0] != 0 && FE_PadRepeatReady(0) != 0) {
            direction = 1;
        } else if (g_padUp[0] != 0 && FE_PadRepeatReady(0) != 0) {
            direction = -1;
        }
        if (direction == 0) {
            if (Pad_JustPressed(7, 0) != 0 && At<uint8_t>(self, 0x396) == 0) {
                uint8_t* selected = TattooSelected(self);
                uint32_t price = At<uint32_t>(selected, 0xC);
                if (G_HUDTattoo_IsOwned(self, reinterpret_cast<int32_t*>(selected)) != 0) {
                    Shop_StartFlash(g_shop, 2);
                    G_HUDStore_SendEvent(self, 7, 1);
                } else if (At<uint32_t>(g_playerPed, 0x1CA0) < price) {
                    Shop_StartFlash(g_shop, 1);
                    G_HUDStore_SendEvent(self, 8, int32_t(price));
                } else {
                    FE_RequestSound(0xB);
                    Ped_TakeMoney(g_playerPed, int32_t(price));
                    uint8_t* bought = G_HUDTattoo_GetSelected(self);
                    G_TattooList_Add(self + 0x19C, reinterpret_cast<uint32_t*>(bought));
                    Stats_Add(0x44, 1);
                    Stats_Add(0x3F, int32_t(price));
                    G_HUDStore_SendEvent(self, 9, At<int32_t>(bought, 0x10));
                }
            }
            FE_PlayMenuSound();
            return;
        }
        int32_t& group = At<int32_t>(self, 0x390);
        if (direction > 0) {
            group++;
            if (group == 2) {
                group = 0;
            }
        } else {
            group--;
            if (group < 0) {
                group = 1;   // (2 groups)
            }
        }
        entry = TattooSelected(self);
        G_HUDTattoo_Preview(self, entry);
        At<uint32_t>(self, 0x39C) = 0;
    }
    G_HUDStore_SendEvent(self, 5, At<int32_t>(entry, 0x10));
    FE_PadClear(0);
    FE_PlayMenuSound();
}
HUDCLOTH_REVERSED("HUDTattooImpl::Input", 0x006D0440, &HUDTattoo_Input);

// 0x6D06A0 (thiscall): the screen (once there are entries)
void __fastcall HUDTattoo_Draw(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x395) == 0) {
        return;
    }
    G_HUDTattoo_Input(self);
    Shop_DrawPanelBackground();
    Shop_DrawMoneyAlways();
    Font_Reset();
    SetFontColour(g_titleColour);
    Font_SetJustify(2);
    Font_Print(g_nameX, 55.0f - 15.0f, Text_Get(TheText, "CLT_TATTOO_TITLE"), 1);
    Font_Render();
    Font_Reset();
    Font_SetJustify(2);
    Font_SetStyle(2);
    float centre = (240.0f + 80.0f) * 0.5f;
    float row = Font_GetLineHeight() + g_rowGap;
    float top = (centre - (row + row) * 0.5f) + 5.0f;
    for (int i = 0; i < 2; i++) {
        float y = (Font_GetLineHeight() + g_rowGap) * float(i) + top;
        SetFontColour(g_listColour);
        const uint16_t* text = Text_Get(TheText, Tattoo_GroupTitle(i));
        if (i == At<int32_t>(self, 0x390)) {
            SetFontColour(g_infoColour);
            if (At<uint32_t>(self, (i + 1) * 0xCC) > 1) {
                G_HUDTattoo_DrawArrows(self, g_nameX, y, text);
            }
        }
        Font_Print(g_nameX, y - 8.75f, text, 1);
        Font_Render();
    }
    Font_Reset();
    Font_SetStyle(1);
    SetFontColour(g_textColour);
    Font_SetJustify(2);
    Font_Print(g_nameX, 340.0f, Text_Get(TheText, "STO_ROTATE2"), 1);
    Font_Render();
    uint8_t* selected = TattooSelected(self);
    uint8_t owned = 0;
    for (int i = 0; i < At<int32_t>(self, 0x37C); i++) {
        if (At<uint32_t>(self, 0x19C + i * 12) == At<uint32_t>(selected, 0)) {
            owned = 1;
            break;
        }
    }
    Shop_DrawItemName(g_shop, At<int32_t>(selected, 0), At<int32_t>(selected, 0xC), owned);
    Font_Reset();
    Font_SetStyle(1);
    Font_SetJustify(2);
    SetFontColour(g_textColour);
    if (At<uint8_t>(self, 0x396) == 0) {
        Font_Print(g_nameX, 400.0f, Text_Get(TheText, "STO_BUYEXIT"), 1);
    }
    Font_Print(g_nameX, 420.0f, Text_Get(TheText, "STO_EXIT"), 1);
    Font_Render();
    Shop_DrawBox(g_nameX, 240.0f, 80.0f);
    Shop_DrawBox(g_nameX, 240.0f, 240.0f);
    Shop_DrawBox(g_nameX, 240.0f, 330.0f);
    if (At<uint8_t>(self, 0x396) != 0 && At<uint32_t>(self, 0x39C) <= g_menuTime &&
        ((g_padLeft[0] == 0 && g_padRight[0] == 0 && g_padUp[0] == 0 && g_padDown[0] == 0) ||
         At<uint32_t>(self, 0x39C) == 0)) {
        g_tattooRemoveIterator = 0;
        for (uint32_t i = 0; i < At<uint32_t>(self, 0x37C); i++) {
            Clothes_RemoveTattoo(g_clothes, self + 0x19C + i * 12);
        }
        if (At<void*>(self, 0x398) != nullptr) {
            Clothes_RemoveTattoo(g_clothes, At<void*>(self, 0x398));
        }
        g_clothesChanged = 1;
        At<uint8_t>(self, 0x396) = 0;
    }
}
HUDCLOTH_REVERSED("HUDTattooImpl::Draw", 0x006D06A0, &HUDTattoo_Draw);
