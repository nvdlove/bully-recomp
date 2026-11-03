// The HUD's inventory pop-ups (an item's icon and "+n" sliding in at the top for a
// couple of seconds, a queue of 5) and the player's item counts they report.
// 0x540E50..0x541EC0 (the component's constructor and Reset are Hud.cpp's).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "reversed.h"

namespace {

struct CVector2D {
    float x, y;
};
struct CVector {
    float x, y, z;
};

// an entry's kinds
enum : int32_t {
    INV_CHANGE = 0,   // an item's count changed (from .. to)
    INV_TEXT = 1,     // a text (by its hash) with the money icon
    INV_FULL = 2,     // can't carry more
};

GAME_FN(uint8_t*, __cdecl, GetWeaponInfo, 0x0051B450, int weapon);   // (+0x34 the most, +0x4C the icon's name)
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void*, __thiscall, MissionMgr_GetItemIcon, 0x006AB3D0, void* mgr, int item);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int, const CVector* pos2,
        int, int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, DrawSprite, 0x0052D610, const col::CRect* rect, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(const uint16_t*, __thiscall, Text_GetByHash, 0x0068FF10, void* text, int hash);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_AddString, 0x006880F0, const uint16_t* string);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(col::CRect*, __cdecl, Font_GetStringRect, 0x0068B090, col::CRect* out, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(uint8_t, __cdecl, Save_CheckValue, 0x00733320, int, int, int value, int expected);
GAME_FN(uint8_t, __cdecl, Save_CheckBlock, 0x00733620, const char* name, const void* data, int size);
GAME_FN(void, __cdecl, CStats_IncrementStat, 0x00458480, int stat, int amount);
GAME_FN(void*, __thiscall, CPlayerInfo_Fn4473D0, 0x004473D0, void* player, int, int, int);
// its own, where modes apply
GAME_FN(void, __thiscall, Inventory_AddChange, 0x00540F00, HUDInventory* self, int item, int from, int to);
GAME_FN(void, __thiscall, Inventory_AddText, 0x00540F70, HUDInventory* self, int unused, int hash);
GAME_FN(void, __thiscall, Inventory_AddFull, 0x00540FD0, HUDInventory* self, int item);
GAME_FN(uint8_t, __thiscall, Inventory_HasFull, 0x00541040, HUDInventory* self, int item);
GAME_FN(void, __thiscall, Inventory_Hurry, 0x005410B0, HUDInventory* self, int item);
GAME_FN(void, __thiscall, Inventory_Show, 0x00541110, HUDInventory* self, int item, int from, int to, int always);
GAME_FN(void*, __cdecl, Inventory_GetIcon, 0x00541210, int item);
GAME_FN(void, __thiscall, Inventory_DrawEntry, 0x005413C0, HUDInventory* self, int index, HUDInventory::Entry* entry);

GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_timeStepScale, 0x00AB45E0);        // (0.02)
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
inline void* const kPlayerInfo = reinterpret_cast<void*>(0x00C1AEA8);   // (its first field: the ped)
inline void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const kSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_VAR(HUDInventory*, g_hudInventory, 0x00C67554);
GAME_VAR(uint8_t, g_playerFlag_C1AF4C, 0x00C1AF4C);
// the item ids: weapons (0xA136B0..B4) and items (0xA136B8..BC), first and last
GAME_VAR(int32_t, g_firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, g_lastWeapon, 0x00A136B4);
GAME_VAR(int32_t, g_firstItem, 0x00A136B8);
GAME_VAR(int32_t, g_lastItem, 0x00A136BC);
GAME_ARRAY(void*, g_weaponIcons, 0x00CEFCC8);
GAME_ARRAY(void*, g_itemIcons, 0x00CEFF20);
GAME_ARRAY(int32_t, g_itemCounts, 0x00CF0060);       // [0x50] the player's, by item - first
// items with their own icons (and stats)
GAME_VAR(int32_t, g_itemMoney1, 0x00BF44EC);
GAME_VAR(int32_t, g_itemMoney2, 0x00BF44E8);
GAME_VAR(int32_t, g_item_BF4294, 0x00BF4294);
GAME_VAR(int32_t, g_item_BF4268, 0x00BF4268);
GAME_VAR(int32_t, g_item_BF4264, 0x00BF4264);
GAME_VAR(int32_t, g_item_BF42A4, 0x00BF42A4);
GAME_VAR(int32_t, g_item_BF4278, 0x00BF4278);
GAME_VAR(int32_t, g_item_BF427C, 0x00BF427C);
GAME_VAR(int32_t, g_item_BF429C, 0x00BF429C);
GAME_VAR(void*, g_moneyIcon, 0x00C66ECC);
GAME_VAR(void*, g_icon_C66F5C, 0x00C66F5C);
GAME_VAR(void*, g_icon_C66F60, 0x00C66F60);
GAME_VAR(void*, g_icon_C66F64, 0x00C66F64);
GAME_VAR(void*, g_textIcon, 0x00C66F0C);
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_ARRAY(uint8_t, g_fontDropColour, 0x0208B780);

// the next slot of the ring, the oldest kept when it's the first
HUDInventory::Entry& NextEntry(HUDInventory* self, int& index) {
    index = (self->last + 1) % 5;
    return self->entries[index];
}
void Commit(HUDInventory* self, int index) {
    int count = self->count;
    self->last = index;
    if (count <= 0) {
        self->first = index;
    }
    self->count = count + 1;
}
// the oldest dropped when the ring is full
void MakeRoom(HUDInventory* self) {
    if (self->count >= 5) {
        self->count--;
        self->first = (self->first + 1) % 5;
    }
}

// ---- the component ------------------------------------------------------------------------

// 0x540E50: the weapons' and items' icons found (in HUDElems)
void __cdecl Inventory_LoadIcons() {
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(Txd_FindTxdSlot("HUDElems", 0));
    for (int item = g_firstItem; item <= g_lastItem; item++) {
        const char* name = *reinterpret_cast<re::Ptr32<const char>*>(GetWeaponInfo(item) + 0x4C);
        if (name) {
            g_itemIcons[item - g_firstItem] = Txd_FindTexture(name, nullptr);
        }
    }
    for (int weapon = g_firstWeapon; weapon <= g_lastWeapon; weapon++) {
        const char* name = *reinterpret_cast<re::Ptr32<const char>*>(GetWeaponInfo(weapon) + 0x4C);
        if (name) {
            g_weaponIcons[weapon - g_firstWeapon] = Txd_FindTexture(name, nullptr);
        }
    }
    Txd_PopCurrentTxd();
}

// 0x540F00
void __fastcall Inventory_AddChange_(HUDInventory* self, re::Edx, int item, int from, int to) {
    int index;
    HUDInventory::Entry& e = NextEntry(self, index);
    e.time = 0.0f;
    e.state = INV_CHANGE;
    e.item = item;
    e.from = from;
    e.to = to;
    e.info = GetWeaponInfo(item);
    Commit(self, index);
}

// 0x540F70: (the first argument unused)
void __fastcall Inventory_AddText_(HUDInventory* self, re::Edx, int, int hash) {
    int index;
    HUDInventory::Entry& e = NextEntry(self, index);
    e.item = hash;
    e.time = 0.0f;
    e.state = INV_TEXT;
    e.from = 0;
    e.to = 1;
    Commit(self, index);
}

// 0x540FD0
void __fastcall Inventory_AddFull_(HUDInventory* self, re::Edx, int item) {
    int index;
    HUDInventory::Entry& e = NextEntry(self, index);
    e.time = 0.0f;
    e.state = INV_FULL;
    e.item = item;
    e.from = -1;
    e.to = -1;
    e.info = GetWeaponInfo(item);
    Commit(self, index);
}

// 0x541040: a "full" entry for the item queued (newest first; the index wrapped once)
uint8_t __fastcall Inventory_HasFull_(HUDInventory* self, re::Edx, int item) {
    int index = self->last;
    for (int i = 0; i < self->count; i++, index--) {
        const HUDInventory::Entry& e = self->entries[index < 0 ? index + 5 : index];
        if (e.state == INV_FULL && e.item == item) {
            return 1;
        }
    }
    return 0;
}

// 0x5410B0: the item 0xBF4278's changes queued sped along (to 3 seconds)
void __fastcall Inventory_Hurry_(HUDInventory* self, re::Edx, int item) {
    if (item != g_item_BF4278) {
        return;
    }
    for (int i = 0; i < self->count; i++) {
        int index = self->last - i;
        if (index < 0) {
            index += 5;
        }
        HUDInventory::Entry& e = self->entries[index];
        if (e.state == INV_CHANGE && e.item == item) {
            e.time = 3.0f;
        }
    }
}

// 0x541110: a change shown (gains only, unless `always`)
void __fastcall Inventory_Show_(HUDInventory* self, re::Edx, int item, int from, int to, int always) {
    if (!re::Flag(always) && from >= to) {
        return;
    }
    MakeRoom(self);
    Inventory_Hurry(self, item);
    Inventory_AddChange(self, item, from, to);
}

// 0x541170 (then 0x540F70 with the same arguments)
void __fastcall Inventory_ShowText(HUDInventory* self, re::Edx, int unused, int hash) {
    MakeRoom(self);
    Inventory_AddText(self, unused, hash);
}

// 0x5411B0
uint8_t __cdecl Inventory_HasItemIcon(int item) {
    if (item >= g_firstItem && item <= g_lastItem) {
        return g_itemIcons[item - g_firstItem] != nullptr;
    }
    return 0;
}

// 0x5411E0
uint8_t __cdecl Inventory_HasWeaponIcon(int weapon) {
    if (weapon >= g_firstWeapon && weapon <= g_lastWeapon) {
        return g_weaponIcons[weapon - g_firstWeapon] != nullptr;
    }
    return 0;
}

// 0x541210: an item's icon (the mission manager's when it has none here)
void* __cdecl Inventory_GetIcon_(int item) {
    void* icon;
    if (item >= g_firstWeapon && item <= g_lastWeapon) {
        icon = g_weaponIcons[item - g_firstWeapon];
    } else if (item >= g_firstItem && item <= g_lastItem) {
        icon = g_itemIcons[item - g_firstItem];
    } else if (item == g_itemMoney1 || item == g_itemMoney2) {
        icon = g_moneyIcon;
    } else if (item == g_item_BF4294) {
        icon = g_icon_C66F5C;
    } else if (item == g_item_BF4268) {
        icon = g_icon_C66F60;
    } else if (item == g_item_BF4264) {
        icon = g_icon_C66F64;
    } else {
        return MissionMgr_GetItemIcon(kMissionMgr, item);
    }
    if (icon) {
        return icon;
    }
    return MissionMgr_GetItemIcon(kMissionMgr, item);
}

// 0x541320: "can't carry more" (once in the queue), with the pickup-full sound
void __fastcall Inventory_ShowFull(HUDInventory* self, re::Edx, int item) {
    MakeRoom(self);
    if (Inventory_HasFull(self, item)) {
        return;
    }
    Inventory_AddFull(self, item);
    uint8_t* player = g_playerPed;
    if (!player) {
        return;
    }
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(player + 0x14);
    const CVector* pos = reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : player + 4);
    int id = Sounds_GetId(kSounds, "PickUpFull");
    Sounds_PlayAt(kSounds, id, pos, 0, pos, 0, 0, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}

// 0x5413C0: an entry: its box slides in from above (0.3 s), stays, and goes (from
// 1.5 s); the icon in the box, the text beside it. (`index` unused)
void __fastcall Inventory_DrawEntry_(HUDInventory*, re::Edx, int, HUDInventory::Entry* e) {
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    float boxW = sx * 40.0f;
    float boxH = sy * 40.0f;
    float textDrop = sy * 10.0f;
    float gap = sx * 5.0f;
    CRGBA textColour, dropColour, iconColour;
    RGBA_Construct(&textColour, 0x22, 0x5D, 0x42, 0xFF);
    RGBA_Construct(&dropColour, 0, 0, 0, 0x94);
    RGBA_Construct(&iconColour, 0xFF, 0xFF, 0xFF, 0xFF);
    uint8_t textAlpha;
    float boxY;
    auto clamp01 = [](float v) {
        if (v < 0.0f) {
            return 0.0f;
        }
        if (1.0f < v) {
            return 1.0f;
        }
        return v;
    };
    if (e->time < 0.3f) {
        float f = clamp01(e->time / 0.3f);
        float c = f * f * f;
        boxY = (1.0f - c) * 150.0f + 150.0f * c;   // (150 either way)
        textAlpha = uint8_t(int(float(textColour.a) * c));
        dropColour.a = uint8_t(int(float(dropColour.a) * c));
        iconColour.a = uint8_t(int(c * float(iconColour.a)));
    } else if (e->time < 1.5f) {
        textAlpha = textColour.a;
        boxY = 150.0f;
    } else {
        float f = clamp01((e->time - 1.5f) + (e->time - 1.5f));
        float c = f * f * f;
        float u = 1.0f - c;
        boxY = 150.0f * u + 150.0f * c;
        textAlpha = uint8_t(int(float(textColour.a) * u));
        dropColour.a = uint8_t(int(float(dropColour.a) * u));
        iconColour.a = uint8_t(int(u * float(iconColour.a)));
    }
    Font_Begin();
    void* icon;
    if (e->state == INV_CHANGE || e->state == INV_FULL) {
        Font_AddString(Text_Get(kTheText, *reinterpret_cast<re::Ptr32<const char>*>(static_cast<uint8_t*>(e->info) + 0x8C)));
        icon = Inventory_GetIcon(e->item);
    } else if (e->state == INV_TEXT) {
        Font_AddString(Text_GetByHash(kTheText, e->item));
        icon = g_textIcon;
    } else {
        icon = nullptr;
    }
    Font_AddNumber(float(e->to - e->from));
    const char* key = "INV_CHANGE";
    if (e->state == INV_CHANGE) {
        if (e->item == g_itemMoney1 || e->item == g_itemMoney2) {
            key = "MON_CHANGE";
        } else if (e->item == g_item_BF42A4) {
            key = "INV_CHANGEX";
        }
    } else if (e->state == INV_FULL) {
        key = "INV_MAX_CAP";
    }
    uint16_t text[0x80];
    Font_FormatText(text, Text_Get(kTheText, key), 0x80);
    g_fontColour[0] = textColour.r;
    g_fontColour[1] = textColour.g;
    g_fontColour[2] = textColour.b;
    g_fontColour[3] = textAlpha;
    std::memcpy(g_fontDropColour, &dropColour, 4);
    Font_SetSizePreset(1);
    col::CRect textRect = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    Font_GetStringRect(&textRect, 0.0f, 0.0f, text);
    float textWidth = std::fabs(textRect.right - textRect.left);
    float total = gap + boxW + textWidth;
    float left = 320.0f - total * 0.5f;
    col::CRect box;
    box.left = left;
    box.bottom = boxY;
    box.right = left + boxW;
    box.top = boxY + boxH;
    SetRenderState(0xC, 1);
    SetRenderState(10, 5);
    SetRenderState(11, 6);
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    DrawSprite(&box, icon, &iconColour, &uv1, &uv2);
    Font_PrintString(gap + box.right, boxY + textDrop, text, 1);
    Font_Render();
}

// 0x541960 (vf8 Draw): the oldest entry, its time run on; those done taken off
void __fastcall Inventory_Draw(HUDInventory* self, re::Edx) {
    int done = 0;
    int state12, state10, state11;
    GetRenderState(0xC, &state12);
    GetRenderState(10, &state10);
    GetRenderState(11, &state11);
    for (int i = 0; i < self->count; i++) {
        int index = self->last - i;
        if (index < 0) {
            index += 5;
        }
        if (index != self->first) {
            continue;
        }
        HUDInventory::Entry* e = &self->entries[index];
        e->time = g_timeStepScale * g_timeStep + e->time;
        if (e->time >= 2.0f) {
            done++;
        }
        Inventory_DrawEntry(self, i, e);
    }
    if (done > 0) {
        int count = self->count - done;
        int first = self->first;
        for (int n = done; n != 0; n--) {
            first = (first + 1) % 5;
        }
        self->count = count;
        self->first = first;
    }
    SetRenderState(0xC, state12);
    SetRenderState(10, state10);
    SetRenderState(11, state11);
}

// ---- the player's items ---------------------------------------------------------------------

// an item id (or an index below 0x50) to its slot in the counts (a far one: slot 0)
bool ItemSlot(uint32_t item, uint32_t& slot) {
    slot = item;
    if (item > 0x4F) {
        slot = item - uint32_t(g_firstItem);
        if (slot > 0x4F) {
            slot = 0;
            return true;
        }
    }
    return int32_t(slot) >= 0 && int32_t(slot) < 0x50;
}

// the count clamped to 0 .. the item's most (in floats, as the game does)
int ClampCount(int count, const uint8_t* info) {
    float v = float(count);
    float most = float(*reinterpret_cast<const int32_t*>(info + 0x34));
    if (v < 0.0f) {
        v = 0.0f;
    } else if (!(most >= v)) {
        v = most;
    }
    return int(v);
}

// 0x541A60
void __cdecl Items_Clear() {
    int n = g_lastItem - g_firstItem;
    if (n >= 0) {
        std::memset(g_itemCounts, 0, size_t(n + 1) * 4);
    }
}

// 0x541A80: the item's +0x48
int32_t __cdecl Items_Get48(int index) {
    if (!GetWeaponInfo(g_firstItem + index)) {
        return 0;
    }
    return *reinterpret_cast<int32_t*>(GetWeaponInfo(g_firstItem + index) + 0x48);
}

// 0x541AC0: the item's type is 3
int32_t __cdecl Items_IsType3(int index) {
    int item = g_firstItem + index;
    if (GetWeaponInfo(item) && *reinterpret_cast<int32_t*>(GetWeaponInfo(item) + 0x18) == 3) {
        return 1;
    }
    return 0;
}

// 0x541B00: ... 2
int32_t __cdecl Items_IsType2(int index) {
    int item = g_firstItem + index;
    if (GetWeaponInfo(item) && *reinterpret_cast<int32_t*>(GetWeaponInfo(item) + 0x18) == 2) {
        return 1;
    }
    return 0;
}

// 0x541B40: the counts to a save (version 1, sizes 0x18C and 0x50)
void __cdecl Items_Save(int32_t* out) {
    out[0] = 1;
    out[1] = 0x18C;
    out[2] = 0x50;
    for (int i = 0; i < 0x50; i++) {
        out[3 + i] = g_itemCounts[i];
    }
}

// 0x541B80: ... from one (cleared, then read when the save checks out)
void __cdecl Items_Load(const int32_t* in) {
    Items_Clear();
    if (!Save_CheckValue(0, 0, in[0], 1) || !Save_CheckValue(0, 0, in[1], 0x18C) ||
        !Save_CheckValue(0, 0, in[2], 0x50) || !Save_CheckBlock("ItemsSaveData", in + 0x53, 0x10)) {
        return;
    }
    int n = g_lastItem - g_firstItem;
    for (int i = 0; i <= n; i++) {
        g_itemCounts[i] = in[3 + i];
    }
}

// 0x541C30: `amount` more of an item (some count as stats; 0xBF429C isn't counted),
// shown when asked
void __cdecl Items_Add(uint32_t item, int amount, int show, int always) {
    uint32_t slot;
    if (!ItemSlot(item, slot)) {
        return;
    }
    HUDInventory* hud = g_hudInventory;
    int id = g_firstItem + int(slot);
    if (id == g_item_BF427C) {
        CStats_IncrementStat(0x25, amount);
    } else if (id == g_item_BF4278) {
        CStats_IncrementStat(0x27, amount);
    } else if (id == g_item_BF429C) {
        if (g_playerFlag_C1AF4C) {
            CPlayerInfo_Fn4473D0(kPlayerInfo, 1, 0, 1);
        }
        Inventory_Show(hud, id, 0, 1, 0);
        return;
    } else if (id == g_item_BF42A4) {
        CStats_IncrementStat(0xE6, amount);
    }
    const uint8_t* info = GetWeaponInfo(int(item));
    int old = g_itemCounts[slot];
    int now = ClampCount(old + amount, info);
    g_itemCounts[slot] = now;
    if (re::Flag(show)) {
        Inventory_Show(hud, int(item), old, now, always);
    }
}

// 0x541D60: an item's count set (shown when it grew)
void __cdecl Items_Set(uint32_t item, int count) {
    uint32_t slot;
    if (!ItemSlot(item, slot)) {
        return;
    }
    const uint8_t* info = GetWeaponInfo(int(item));
    int old = g_itemCounts[slot];
    int now = ClampCount(count, info);
    g_itemCounts[slot] = now;
    Inventory_Show(g_hudInventory, int(item), old, now, 0);
}

// 0x541E00
int32_t __cdecl Items_GetCount(uint32_t item) {
    uint32_t slot;
    if (!ItemSlot(item, slot)) {
        return 0;
    }
    return g_itemCounts[slot];
}

// 0x541E40: one used
void __cdecl Items_UseOne(uint32_t item) {
    uint32_t slot;
    if (!ItemSlot(item, slot)) {
        return;
    }
    int count = g_itemCounts[slot];
    if (count > 0) {
        g_itemCounts[slot] = count - 1;
        Inventory_Show(g_hudInventory, int(item), count, count - 1, 0);
    }
}

// 0x541E90: (no range check below 0x50)
uint8_t __cdecl Items_Has(uint32_t item) {
    uint32_t slot = item;
    if (item > 0x4F) {
        slot = item - uint32_t(g_firstItem);
        if (slot > 0x4F) {
            slot = 0;
        }
    }
    return g_itemCounts[slot] > 0;
}

// 0x541EC0: the first item held whose +0x60 is `value` (its index), -1: none
int32_t __cdecl Items_FindHeldBy60(int value) {
    for (uint32_t i = 0; int(i) <= g_lastItem - g_firstItem; i++) {
        uint32_t slot = i;
        if (i > 0x4F) {
            slot = i - uint32_t(g_firstItem);
            if (slot > 0x4F) {
                slot = 0;
            }
        }
        if (g_itemCounts[slot] > 0 &&
            *reinterpret_cast<int32_t*>(GetWeaponInfo(g_firstItem + int(i)) + 0x60) == value) {
            return int32_t(i);
        }
    }
    return -1;
}

} // namespace

#define HUDINV_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(hudinv_original_, __LINE__);                                          \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudinv_original_, __LINE__), \
             re::Mode::Original)

HUDINV_REVERSED("HUDInventory::LoadIcons", 0x00540E50, &Inventory_LoadIcons);
HUDINV_REVERSED("HUDInventory::AddChange", 0x00540F00, &Inventory_AddChange_);
HUDINV_REVERSED("HUDInventory::AddText", 0x00540F70, &Inventory_AddText_);
HUDINV_REVERSED("HUDInventory::AddFull", 0x00540FD0, &Inventory_AddFull_);
HUDINV_REVERSED("HUDInventory::HasFull", 0x00541040, &Inventory_HasFull_);
HUDINV_REVERSED("HUDInventory::Hurry", 0x005410B0, &Inventory_Hurry_);
HUDINV_REVERSED("HUDInventory::Show", 0x00541110, &Inventory_Show_);
HUDINV_REVERSED("HUDInventory::ShowText", 0x00541170, &Inventory_ShowText);
HUDINV_REVERSED("HUDInventory::HasItemIcon", 0x005411B0, &Inventory_HasItemIcon);
HUDINV_REVERSED("HUDInventory::HasWeaponIcon", 0x005411E0, &Inventory_HasWeaponIcon);
HUDINV_REVERSED("HUDInventory::GetIcon", 0x00541210, &Inventory_GetIcon_);
HUDINV_REVERSED("HUDInventory::ShowFull", 0x00541320, &Inventory_ShowFull);
HUDINV_REVERSED("HUDInventory::DrawEntry", 0x005413C0, &Inventory_DrawEntry_);
HUDINV_REVERSED("HUDInventory::Draw", 0x00541960, &Inventory_Draw);
HUDINV_REVERSED("Items::Clear", 0x00541A60, &Items_Clear);
HUDINV_REVERSED("Items::Get48", 0x00541A80, &Items_Get48);
HUDINV_REVERSED("Items::IsType3", 0x00541AC0, &Items_IsType3);
HUDINV_REVERSED("Items::IsType2", 0x00541B00, &Items_IsType2);
HUDINV_REVERSED("Items::Save", 0x00541B40, &Items_Save);
HUDINV_REVERSED("Items::Load", 0x00541B80, &Items_Load);
HUDINV_REVERSED("Items::Add", 0x00541C30, &Items_Add);
HUDINV_REVERSED("Items::Set", 0x00541D60, &Items_Set);
HUDINV_REVERSED("Items::GetCount", 0x00541E00, &Items_GetCount);
HUDINV_REVERSED("Items::UseOne", 0x00541E40, &Items_UseOne);
HUDINV_REVERSED("Items::Has", 0x00541E90, &Items_Has);
HUDINV_REVERSED("Items::FindHeldBy60", 0x00541EC0, &Items_FindHeldBy60);
