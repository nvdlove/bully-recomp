// CShop's setup and buying (the one shop at 0xC18748), its items' setup, turning and
// removal, the store camera, the flashing of the money panel. 0x454ED0..0x455D30
// (the browsing and drawing are ShopMenu.cpp's).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

// An item on a shelf (0x44)
struct ShItem {
    uint8_t used;         // +0
    uint8_t pad1[3];
    int32_t model;        // +4
    re::Ptr32<uint8_t> object;      // +8 the prop shown (a registered reference)
    int32_t price;        // +0xC
    int32_t stock;        // +0x10 (bought: one less, when above 0)
    int32_t maxBuy;       // +0x14 (at most this many at once; below 0: none)
    float x, y, z;        // +0x18
    float heading;        // +0x24 degrees
    float unknown28;      // +0x28
    float unknown2C;      // +0x2C
    float scale;          // +0x30
    float unknown34;      // +0x34
    float unknown38;      // +0x38
    float rotation;       // +0x3C radians
    re::Ptr32<void> callback;       // +0x40 a script callback (a reference to a Lua function)
};
static_assert(offsetof(ShItem, x) == 0x18);
static_assert(offsetof(ShItem, rotation) == 0x3C);
static_assert(sizeof(ShItem) == 0x44);

struct ShVector {
    float x, y, z;
};

struct ShShelf {
    re::Ptr32<void> vtable;          // +0 (cShelf)
    re::Ptr32<ShItem> items[20];     // +4
    int8_t count;          // +0x54
    uint8_t pad55[3];
};
static_assert(sizeof(ShShelf) == 0x58);

struct ShShop {
    int32_t state;             // +0
    int32_t unknown4;          // +4
    int32_t currency;          // +8 0 money, 1 tickets
    uint8_t unknownC;          // +0xC
    uint8_t padD[3];
    re::Ptr32<uint8_t> clerk;            // +0x10 (a registered reference)
    uint32_t clerkSetup[7];    // +0x14 (SetClerk)
    uint32_t cameraSetup[6];   // +0x30 (SetCamera)
    uint32_t cameraTarget[3];  // +0x48
    float zoom;                // +0x54
    float angleOffset;         // +0x58 degrees
    ShShelf shelves[5];        // +0x5C
    int8_t numShelves;         // +0x214
    char name[0x30];           // +0x215
    char speech[6][0x18];      // +0x245
    int8_t curShelf;           // +0x2D5
    int8_t curItem;            // +0x2D6
    int8_t numItems;           // +0x2D7
    uint32_t stopCallback;     // +0x2D8
    ShItem items[30];          // +0x2DC
    uint8_t unknownAD4;        // +0xAD4
    uint8_t padAD5[3];
    float unknownAD8;          // +0xAD8

    ShItem* Current() { return shelves[curShelf].items[curItem]; }
};
static_assert(offsetof(ShShop, clerk) == 0x10);
static_assert(offsetof(ShShop, zoom) == 0x54);
static_assert(offsetof(ShShop, shelves) == 0x5C);
static_assert(offsetof(ShShop, numShelves) == 0x214);
static_assert(offsetof(ShShop, curShelf) == 0x2D5);
static_assert(offsetof(ShShop, items) == 0x2DC);
static_assert(offsetof(ShShop, unknownAD8) == 0xAD8);

GAME_ARRAY(char, sh_speechPath, 0x00C19240);
inline const char* const sh_globalFolder = reinterpret_cast<const char*>(0x00906898);   // "\\Global\\"
GAME_VAR(int32_t, sh_flashState, 0x00C1873C);
GAME_VAR(float, sh_flashTime, 0x00C18740);
GAME_VAR(uint8_t, sh_browsing, 0x00C18738);
GAME_VAR(float, sh_timeScale, 0x00A1494C);
GAME_VAR(float, sh_timeStep, 0x00C1A9A0);
GAME_VAR(float, sh_timeStep2, 0x00C1A9A4);
GAME_VAR(float, sh_twoPi, 0x00A14948);
GAME_VAR(float, sh_panelAlpha, 0x00A14934);
inline void* const sh_panelColourA = reinterpret_cast<void*>(0x00C18744);
inline void* const sh_panelColourB = reinterpret_cast<void*>(0x00C19224);
inline void* const sh_boxColour = reinterpret_cast<void*>(0x00C19238);
GAME_VAR(uint8_t*, sh_playerPed, 0x00C1AEA8);    // CWorld::Players[0]'s ped
GAME_VAR(int32_t, sh_ticketItem, 0x00BF42A4);
GAME_VAR(int32_t, sh_freeItemModel, 0x00BF42A0);
GAME_VAR(int32_t, sh_currArea, 0x00BD1008);       // CGame::currArea
GAME_VAR(int32_t, sh_firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, sh_lastWeapon, 0x00A136B4);
GAME_VAR(int32_t, sh_firstItem, 0x00A136B8);
GAME_VAR(int32_t, sh_lastItem, 0x00A136BC);
GAME_VAR(int32_t, sh_firstClothes, 0x00A136A8);
GAME_VAR(int32_t, sh_lastClothes, 0x00A136AC);
// the frontend's input and message state
GAME_VAR(int32_t, sh_messagePending, 0x020C33B4);
GAME_VAR(uint8_t, sh_menuUp, 0x020C3274);
GAME_VAR(uint8_t, sh_menuDown, 0x020C3278);
inline void* const sh_menu = reinterpret_cast<void*>(0x020C3370);
inline void* const sh_speechMgr = reinterpret_cast<void*>(0x00B9DAB8);
inline void* const sh_sounds = reinterpret_cast<void*>(0x00CF3DA0);
// the camera
inline void* const sh_camera = reinterpret_cast<void*>(0x00C3CC68);   // TheCamera
GAME_VAR(uint8_t, sh_allowScriptedChange, 0x00C3CE10);
GAME_VAR(int32_t, sh_allowChange, 0x00C3CE0C);
GAME_VAR(int32_t, sh_cameraCED8, 0x00C3CED8);
GAME_VAR(int32_t, sh_fovOverridden, 0x00C3CDD4);
GAME_VAR(float, sh_fovOverride, 0x00C3CDD0);
GAME_VAR(float, sh_fov, 0x00C3CD04);                // CCamera::ms_fov
GAME_VAR(uint8_t*, sh_storeCamera, 0x00C3D2A4);

GAME_FN(uint8_t, __thiscall, SH_Callback_IsSet, 0x005D80E0, void* callback);
GAME_FN(void*, __thiscall, SH_Callback_GetState, 0x005D9C20, void* callback);
GAME_FN(int, __thiscall, SH_Callback_GetRef, 0x005D80D0, void* callback);
GAME_FN(void, __cdecl, SH_Lua_5D8270, 0x005D8270, void* L);
GAME_FN(void, __cdecl, SH_Lua_PushRef, 0x005D8670, void* L, int ref);
GAME_FN(void, __cdecl, SH_Lua_PushNumber, 0x005D85C0, void* L, int value);
GAME_FN(void, __cdecl, SH_Lua_Call, 0x005D82A0, void* L, int args, int results);
GAME_FN(uint8_t*, __cdecl, SH_GetButes, 0x0051B450, int model);
GAME_FN(uint8_t, __cdecl, SH_IsVehicleClass1, 0x0051B3B0, int model);
inline char* __cdecl SH_strncpy(char* dest, const char* src, size_t count) { return crt::Strncpy(dest, src, count); }
GAME_FN(void, __cdecl, SH_Frontend_SetFlash, 0x006A2340, int);
GAME_FN(void, __cdecl, SH_Frontend_6A2360, 0x006A2360);
GAME_FN(void, __cdecl, SH_Frontend_6A31B0, 0x006A31B0);
GAME_FN(uint8_t, __cdecl, SH_Frontend_6A3170, 0x006A3170, int);
GAME_FN(void, __cdecl, SH_Frontend_6A3120, 0x006A3120, int);
GAME_FN(void, __thiscall, SH_Menu_Next, 0x006A2990, void* menu);
GAME_FN(void, __thiscall, SH_Menu_Previous, 0x006A29B0, void* menu);
GAME_FN(uint8_t, __cdecl, SH_Pad_Pressed, 0x00738900, int button, int);
GAME_FN(void, __thiscall, SH_Speech_Stop, 0x00684CF0, void* mgr);
GAME_FN(void, __thiscall, SH_Speech_Play, 0x00685C50, void* mgr, const char* name, const char* path);
GAME_FN(void, __thiscall, SH_Sounds_Play, 0x0059C150, void* sounds, int sound);
GAME_FN(uint8_t, __thiscall, SH_Ped_CanAfford, 0x004730E0, uint8_t* ped, uint32_t amount);
GAME_FN(void, __thiscall, SH_Ped_TakeMoney, 0x00473100, uint8_t* ped, uint32_t amount);
GAME_FN(void, __cdecl, SH_IncrementStat, 0x00458480, int stat, int amount);
GAME_FN(uint32_t, __cdecl, SH_Items_GetCount, 0x00541E00, int item);
GAME_FN(void, __cdecl, SH_Items_SetCount, 0x00541D60, int item, int count);
GAME_FN(int, __thiscall, SH_WeaponInventory_GetCount, 0x0045CF40, void* inventory, int weapon);
GAME_FN(void, __thiscall, SH_Camera_SetShot, 0x004F2C20, void* camera, int shot, int, int, int, int);
GAME_FN(void, __thiscall, SH_Camera_ResetFov, 0x004F1660, void* camera);
GAME_FN(void, __thiscall, SH_Camera_SetFov, 0x004F33A0, void* camera, float fov, int, int, float time, int);
GAME_FN(void, __cdecl, SH_DrawRect, 0x006A4730, float x, float y, float h, void* colour, float alpha);
GAME_FN(void, __cdecl, SH_DrawQuad, 0x0052C8F0, float x1, float y1, float x2, float y2, uintptr_t, void* colour, const float* uv0,
        const float* uv1);
GAME_FN(void, __cdecl, SH_LerpColour, 0x006A45F0, void* out, void* to, void* from, float t);
GAME_FN(void, __cdecl, SH_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, SH_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, SH_SetModelDeletable, 0x0052FE40, int model);
GAME_FN(void, __thiscall, SH_AnimObject_Destruct, 0x006B5A80, void* anim);
GAME_FN(void, __cdecl, SH_World_Remove, 0x0045DD70, void* entity);
GAME_FN(float*, __thiscall, SH_Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(void, __thiscall, SH_Matrix_SetRotateZOnly, 0x00412C20, float* matrix, float angle);
GAME_FN(void, __thiscall, SH_Entity_UpdateRW, 0x00443AA0, uint8_t* entity);
GAME_FN(void, __thiscall, SH_Entity_UpdateRwFrame, 0x00465760, uint8_t* entity);
GAME_FN(void, __cdecl, SH_Node_Update, 0x006C9AF0, void* node);
// ours, called where modes apply
GAME_FN(char*, __cdecl, SH_MakeSpeechPath, 0x00454ED0, const char* folder, const char* name);
GAME_FN(void, __thiscall, SH_Item_Set, 0x00454F90, ShItem* item, int model, ShVector pos, ShVector angles, int stock,
        int price, int maxBuy);
GAME_FN(void, __thiscall, SH_Item_RunCallback, 0x00455010, ShItem* item);
GAME_FN(void, __thiscall, SH_BuyWithMoney, 0x00455490, ShShop* shop);
GAME_FN(void, __thiscall, SH_BuyWithTickets, 0x00455550, ShShop* shop);

} // namespace

namespace shop {

// 0x454ED0: "\Global\<folder>\<name>" (the clerk's speech files) in a static buffer
char* __cdecl MakeSpeechPath(const char* folder, const char* name) {
    std::memcpy(sh_speechPath, sh_globalFolder, 9);
    std::strcat(sh_speechPath, folder);
    std::strcat(sh_speechPath, "\\");
    std::strcat(sh_speechPath, name);
    return sh_speechPath;
}

// 0x454F90: an item's model, place and numbers; the model's butes looked up (twice;
// nothing kept)
void __fastcall ItemSet(ShItem* item, re::Edx, int model, ShVector pos, ShVector angles, int stock, int price, int maxBuy) {
    item->x = pos.x;
    item->y = pos.y;
    item->z = pos.z;
    item->heading = angles.x;
    item->unknown28 = angles.y;
    item->unknown2C = angles.z;
    item->stock = stock;
    item->used = 1;
    item->model = model;
    item->price = price;
    item->maxBuy = maxBuy;
    item->rotation = float((item->heading - 90.0) * double(3.14159274f) / 180.0);
    if (SH_GetButes(model)) {
        SH_GetButes(model);
    }
}

// 0x455010: its script callback called with its model
void __fastcall ItemRunCallback(ShItem* item, re::Edx) {
    if (!SH_Callback_IsSet(&item->callback)) {
        return;
    }
    void* L = SH_Callback_GetState(&item->callback);
    SH_Lua_5D8270(L);
    SH_Lua_PushRef(L, SH_Callback_GetRef(&item->callback));
    SH_Lua_PushNumber(L, item->model);
    SH_Lua_Call(L, 1, 0);
}

// 0x455060 (ShopSetClerk...): 7 words at +0x14
void __fastcall SetClerk(ShShop* shop, re::Edx, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g) {
    shop->clerkSetup[0] = a;
    shop->clerkSetup[1] = b;
    shop->clerkSetup[2] = c;
    shop->clerkSetup[4] = e;
    shop->clerkSetup[3] = d;
    shop->clerkSetup[5] = f;
    shop->clerkSetup[6] = g;
}

// 0x4550A0: 6 words at +0x30
void __fastcall SetCamera(ShShop* shop, re::Edx, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f) {
    shop->cameraSetup[0] = a;
    shop->cameraSetup[1] = b;
    shop->cameraSetup[3] = d;
    shop->cameraSetup[2] = c;
    shop->cameraSetup[4] = e;
    shop->cameraSetup[5] = f;
}

// 0x4550D0: the store camera's target
void __fastcall SetCameraTarget(ShShop* shop, re::Edx, uint32_t x, uint32_t y, uint32_t z) {
    shop->cameraTarget[0] = x;
    shop->cameraTarget[1] = y;
    shop->cameraTarget[2] = z;
}

// 0x4550F0: the shop's name and the clerk's speech names (the 7th argument unused)
void __fastcall SetSpeech(ShShop* shop, re::Edx, const char* name, const char* s0, const char* s1, const char* s2, const char* s3,
                          const char* s4, int, const char* s5) {
    SH_strncpy(shop->name, name, 0x30);
    SH_strncpy(shop->speech[0], s0, 0x17);
    SH_strncpy(shop->speech[1], s1, 0x17);
    SH_strncpy(shop->speech[2], s2, 0x17);
    SH_strncpy(shop->speech[3], s3, 0x17);
    SH_strncpy(shop->speech[4], s4, 0x17);
    SH_strncpy(shop->speech[5], s5, 0x17);
}

// 0x455190
void __cdecl ClearFlash() {
    sh_flashState = 0;
}

// 0x4551A0 (thiscall; the shop unused): the money (1) or the price (2) flashing
void __fastcall StartFlash(ShShop*, re::Edx, int state) {
    sh_flashTime = 0.0f;
    sh_flashState = state;
    SH_Frontend_SetFlash(0xC);
}

// 0x4551C0: the "leave" question's input: back (9) or yes (7, the clerk's
// goodbye when no message is up), or the menu moved
void __fastcall HandleExitInput(ShShop* shop, re::Edx) {
    if (SH_Pad_Pressed(9, 0)) {
        shop->state = 1;
        SH_Frontend_6A2360();
        SH_Frontend_6A31B0();
        return;
    }
    if (SH_Pad_Pressed(7, 0)) {
        int noMessage = sh_messagePending == 0;
        shop->state = 1;
        if (noMessage) {
            SH_Speech_Stop(sh_speechMgr);
            char* path = SH_MakeSpeechPath(shop->speech[0], shop->speech[2]);
            SH_Speech_Play(sh_speechMgr, shop->name, path);
            SH_Sounds_Play(sh_sounds, 0x18);
        }
        SH_Frontend_6A2360();
        SH_Frontend_6A31B0();
        return;
    }
    if (SH_Frontend_6A3170(0)) {
        if (sh_menuUp) {
            SH_Menu_Next(sh_menu);
            SH_Frontend_6A3120(0);
            SH_Frontend_6A31B0();
            return;
        }
        if (sh_menuDown) {
            SH_Menu_Previous(sh_menu);
            SH_Frontend_6A3120(0);
        }
    }
    SH_Frontend_6A31B0();
}

// 0x4552A0: enough money (the player's) or tickets. (0 in al for another currency)
uint8_t __fastcall CanAfford(ShShop* shop, re::Edx, uint32_t price) {
    if (shop->currency == 0) {
        return SH_Ped_CanAfford(sh_playerPed, price);
    }
    if (shop->currency == 1) {
        return SH_Items_GetCount(sh_ticketItem) >= price;
    }
    return 0;
}

// 0x4552E0: the store camera: shot 7, the fov reset (to the override when there is
// one), its zoom and angle offset
void __fastcall SetupCamera(ShShop* shop, re::Edx) {
    SH_Camera_SetShot(sh_camera, 7, 0, 1, 0, 0);
    sh_allowScriptedChange = 1;
    sh_allowChange = 0;
    sh_cameraCED8 = 0;
    SH_Camera_ResetFov(sh_camera);
    float fov = sh_fovOverridden ? sh_fovOverride : sh_fov;
    SH_Camera_SetFov(sh_camera, fov, 0, 1, 0.1f, 0x80);
    *reinterpret_cast<float*>(sh_storeCamera + 0x5C) = shop->zoom;
    *reinterpret_cast<float*>(sh_storeCamera + 0x58) = float(shop->angleOffset * double(3.14159274f) / 180.0);
}

// 0x455390 (ecx unused): a box in the panel colour
void __stdcall DrawBox(float x, float y, float h) {
    SH_DrawRect(x, y, h, sh_boxColour, 1.0f);
}

// 0x4553C0: cShelf's vf0 (the deleting destructor)
void* __fastcall ShelfDestroy(ShShelf* shelf, re::Edx, uint8_t flags) {
    shelf->vtable = reinterpret_cast<void*>(0x009068A8);
    if (flags & 1) {
        SH_Free(shelf);
    }
    return shelf;
}

// 0x4553E0: the next free item onto a shelf (the shelves counted up to it), set
ShItem* __fastcall AddItem(ShShop* shop, re::Edx, int shelfIndex, int model, ShVector pos, ShVector angles, int stock, int price,
                           int maxBuy) {
    int shelf = int8_t(shelfIndex);
    if (shelf + 1 > shop->numShelves) {
        shop->numShelves++;
    }
    ShShelf* s = &shop->shelves[shelf];
    ShItem* item = &shop->items[shop->numItems++];
    s->items[s->count] = item;
    s->count++;
    SH_Item_Set(item, model, pos, angles, stock, price, maxBuy);
    return item;
}

// 0x455490: the current item paid with money (the stats by shop: 0x3B the barber,
// 0x3D the clothes; bikes 0x41 and 0x3A, others 0x40; the free item's 0x28, 0xC2)
void __fastcall BuyWithMoney(ShShop* shop, re::Edx) {
    ShItem* item = shop->Current();
    SH_Ped_TakeMoney(sh_playerPed, item->price);
    if (sh_currArea == 0x1A) {
        SH_IncrementStat(0x3B, item->price);
    } else if (sh_currArea == 0x1E) {
        SH_IncrementStat(0x3D, item->price);
    }
    if (SH_IsVehicleClass1(item->model)) {
        SH_IncrementStat(0x41, 1);
        SH_IncrementStat(0x3A, item->price);
    } else {
        SH_IncrementStat(0x40, 1);
    }
    if (item->model == sh_freeItemModel) {
        SH_IncrementStat(0x28, 1);
        SH_IncrementStat(0xC2, 1);
    }
    if (item->stock > 0) {
        item->stock--;
    }
    SH_Item_RunCallback(item);
    sh_browsing = 1;
}

// 0x455550: the current item paid with tickets
void __fastcall BuyWithTickets(ShShop* shop, re::Edx) {
    ShItem* item = shop->Current();
    uint32_t tickets = SH_Items_GetCount(sh_ticketItem);
    int price = item->price;
    SH_Items_SetCount(sh_ticketItem, int(tickets - price));
    SH_IncrementStat(0xE7, price);
    SH_IncrementStat(0xE8, 1);
    if (item->stock > 0) {
        item->stock--;
    }
    SH_Item_RunCallback(item);
}

// 0x4555C0: how many of the current item can be bought: room left (weapons, items;
// clothes: 1) up to the item's limit (none when that's below 0)
int __fastcall GetMaxBuyable(ShShop* shop, re::Edx) {
    ShItem* item = shop->Current();
    int model = item->model;
    int owned = 0;
    if (model >= sh_firstWeapon && model <= sh_lastWeapon) {
        owned = SH_WeaponInventory_GetCount(*reinterpret_cast<re::Ptr32<void>*>(sh_playerPed + 0x1C4), model);
    } else if (model >= sh_firstItem && model <= sh_lastItem) {
        owned = int(SH_Items_GetCount(model));
    } else if (model >= sh_firstClothes && model <= sh_lastClothes) {
        return 1;
    }
    int room = *reinterpret_cast<int32_t*>(SH_GetButes(shop->Current()->model) + 0x34) - owned;
    // (through floats: the room at least 0, then no more than the limit)
    float fRoom = float(room);
    if (!(fRoom > 0.0f)) {
        fRoom = 0.0f;
    }
    float limit = float(item->maxBuy);
    float n = float(int(fRoom));
    float result;
    if (limit < 0.0f) {
        result = 0.0f;
    } else if (n < limit) {
        result = n;
    } else {
        result = limit;
    }
    return int(result);
}

// 0x4556F0: the panel's two halves (320 across and the panel's width past that)
void __cdecl DrawPanelBackground() {
    float uv1[2] = {1.0f, 1.0f};
    float uv0[2] = {0.0f, 0.0f};
    SH_DrawQuad(0.0f, 0.0f, 320.0f, 480.0f, 0, sh_panelColourA, uv0, uv1);
    float uv3[2] = {1.0f, 1.0f};
    float uv2[2] = {0.0f, 0.0f};
    float right = float(sh_panelAlpha + 320.0);
    SH_DrawQuad(320.0f, 0.0f, right, 480.0f, 0, sh_panelColourB, uv2, uv3);
}

// 0x4557A0: a flashing colour (from `a` to `b` over 0.3 s, eased); the flash ends
// after 0.3 s
void* __cdecl FlashColour(void* out, void* a, void* b) {
    float t = float(sh_flashTime / double(0.3f));
    if (t < 0.0f) {
        t = 0.0f;
    } else if (1.0f < t) {
        t = 1.0f;
    }
    SH_LerpColour(out, b, a, t * t * t);
    float step = sh_timeScale * sh_timeStep;
    sh_flashTime = step + sh_flashTime;
    if (0.3f < sh_flashTime) {
        sh_flashState = 0;
    }
    return out;
}

// 0x455850: +0xAD4 set, or in one of the shops' interiors
int __fastcall IsShopInterior(ShShop* shop, re::Edx) {
    if (!shop->unknownAD4) {
        switch (sh_currArea) {
        case 0x21:
        case 0x22:
        case 0x38:
        case 0x2E:
        case 0x27:
        case 0x10:
        case 0x1A:
        case 0x1D:
        case 0x1E:
            break;
        default:
            return 0;
        }
    }
    return 1;
}

// 0x4558A0
int __cdecl IsInArea50() {
    return sh_currArea == 0x32;
}

// 0x4558B0 (thiscall): an item's constructor (its object reference let go when set,
// which it can't be here)
ShItem* __fastcall ItemConstruct(ShItem* item, re::Edx) {
    item->x = 0.0f;
    item->y = 0.0f;
    item->z = 0.0f;
    item->heading = 0.0f;
    item->object = nullptr;
    item->unknown28 = 0.0f;
    item->callback = nullptr;
    item->unknown2C = 0.0f;
    item->used = 0;
    item->model = 0;
    if (item->object) {
        SH_CleanUpOldReference(item->object, &item->object);
        item->object = nullptr;
    }
    item->x = 0.0f;
    item->y = 0.0f;
    item->heading = 0.0f;
    item->z = 0.0f;
    item->scale = 0.0f;
    item->unknown34 = 0.0f;
    item->unknown28 = 0.0f;
    item->unknown38 = 0.05f;
    item->rotation = 0.0f;
    item->price = 0;
    item->stock = 0;
    item->unknown2C = 0.0f;
    return item;
}

// 0x455960: its prop out of the world and deleted (its model deletable, its
// animation object freed). (The reference is read again after each step: the
// delete clears it)
void __fastcall ItemRemoveObject(ShItem* item, re::Edx) {
    item->used = 0;
    if (!item->object) {
        return;
    }
    SH_SetModelDeletable(*reinterpret_cast<int16_t*>(item->object + 0x10E));
    void* anim = *reinterpret_cast<re::Ptr32<void>*>(item->object + 0x114);
    if (anim) {
        SH_AnimObject_Destruct(anim);
        SH_Free(anim);
        *reinterpret_cast<re::Ptr32<void>*>(item->object + 0x114) = nullptr;
        *reinterpret_cast<re::Ptr32<void>*>(item->object + 0x18) = nullptr;
    }
    SH_World_Remove(item->object);
    if (item->object) {
        re::Ptr32<void>* vtable = *re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(item->object);
        re::PtrCast<void(__thiscall*)(uint8_t*, int)>(vtable[0])(item->object, 1);
    }
    if (item->object) {
        SH_CleanUpOldReference(item->object, &item->object);
        item->object = nullptr;
    }
}

// 0x4559F0: its prop turned (while `turning`: flag 0x10 of +0x209 on) and scaled 1.
// (The second argument unused.) The game writes its uninitialised stack to the
// matrix's +0x3C; ours leaves it
void __fastcall ItemUpdate(ShItem* item, re::Edx, int turning, int) {
    item->unknown34 = 1.5f;
    if (item->scale == 0.0f) {
        item->scale = 1.5f;
    }
    item->scale = 1.0f;
    uint8_t* object = item->object;
    if (uint8_t(turning)) {
        object[0x209] |= 0x10;
        float step = sh_timeScale * sh_timeStep2;
        float rotation = float(step * double(3.14159274f) + item->rotation);
        item->rotation = rotation;
        if (sh_twoPi <= rotation) {
            item->rotation = rotation - sh_twoPi;
        }
    } else {
        object[0x209] &= ~0x10;
    }
    float* m = SH_Entity_GetMatrix(item->object);
    float x = m[12];
    float y = m[13];
    float z = m[14];
    SH_Matrix_SetRotateZOnly(m, item->rotation);
    float s = item->scale;
    m[0] = m[0] * s;
    m[1] = m[1] * s;
    m[2] = m[2] * s;
    m[4] = m[4] * s;
    m[5] = m[5] * s;
    m[6] = m[6] * s;
    m[8] = m[8] * s;
    m[9] = s * m[9];
    m[10] = s * m[10];
    m[12] = x;
    m[13] = y;
    m[14] = z;
    SH_Entity_UpdateRW(item->object);
    uint8_t* entity = item->object;
    if (*reinterpret_cast<re::Ptr32<void>*>(entity + 0x114) == nullptr) {
        SH_Entity_UpdateRwFrame(entity);
        return;
    }
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity);
    re::PtrCast<void(__thiscall*)(uint8_t*)>(vtable[0x4C / 4])(entity);
    void* node = *reinterpret_cast<re::Ptr32<void>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(item->object + 0x114) + 0xC);
    if (node) {
        SH_Node_Update(node);
    }
}

// 0x455B60: the shop emptied (the clerk let go). (+0x24..+0x2C, the cursor and the
// shelves' own counts are left)
void __fastcall Reset(ShShop* shop, re::Edx) {
    if (shop->clerk) {
        SH_CleanUpOldReference(shop->clerk, &shop->clerk);
        shop->clerk = nullptr;
    }
    shop->clerkSetup[1] = 0;
    shop->clerkSetup[2] = 0;
    shop->cameraSetup[0] = 0;
    shop->clerkSetup[3] = 0;
    shop->cameraSetup[3] = 0;
    shop->cameraSetup[1] = 0;
    shop->cameraSetup[2] = 0;
    shop->cameraTarget[0] = 0;
    shop->zoom = 0.0f;
    shop->cameraSetup[4] = 0;
    shop->angleOffset = 0.0f;
    shop->cameraSetup[5] = 0;
    shop->cameraTarget[1] = 0;
    shop->clerkSetup[0] = 0;
    shop->cameraTarget[2] = 0;
    shop->state = 0;
    shop->unknown4 = 0;
    shop->currency = 0;
    shop->unknownC = 0;
    std::memset(shop->name, 0, 0x30);
    std::memset(shop->speech, 0, sizeof(shop->speech));
    shop->unknownAD8 = 0.0f;
    shop->numShelves = 0;
    shop->numItems = 0;
    shop->unknownAD4 = 0;
}

// 0x455D30: the current item bought with the shop's currency
void __fastcall Buy(ShShop* shop, re::Edx) {
    if (shop->currency == 0) {
        SH_BuyWithMoney(shop);
    } else if (shop->currency == 1) {
        SH_BuyWithTickets(shop);
    }
    SH_Frontend_6A31B0();
}

} // namespace shop

// ---- registration ---------------------------------------------------------------------

#define C04C_SH_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04C_SH_REVERSED("CShop::MakeSpeechPath", 0x00454ED0, &shop::MakeSpeechPath);
C04C_SH_REVERSED("CShopItem::Set", 0x00454F90, &shop::ItemSet);
C04C_SH_REVERSED("CShopItem::RunCallback", 0x00455010, &shop::ItemRunCallback);
C04C_SH_REVERSED("CShop::SetClerk", 0x00455060, &shop::SetClerk);
C04C_SH_REVERSED("CShop::SetCamera", 0x004550A0, &shop::SetCamera);
C04C_SH_REVERSED("CShop::SetCameraTarget", 0x004550D0, &shop::SetCameraTarget);
C04C_SH_REVERSED("CShop::SetSpeech", 0x004550F0, &shop::SetSpeech);
C04C_SH_REVERSED("CShop::ClearFlash", 0x00455190, &shop::ClearFlash);
C04C_SH_REVERSED("CShop::StartFlash", 0x004551A0, &shop::StartFlash);
C04C_SH_REVERSED("CShop::HandleExitInput", 0x004551C0, &shop::HandleExitInput);
C04C_SH_REVERSED("CShop::CanAfford", 0x004552A0, &shop::CanAfford);
C04C_SH_REVERSED("CShop::SetupCamera", 0x004552E0, &shop::SetupCamera);
C04C_SH_REVERSED("CShop::DrawBox", 0x00455390, &shop::DrawBox);
C04C_SH_REVERSED("cShelf::vf0", 0x004553C0, &shop::ShelfDestroy);
C04C_SH_REVERSED("CShop::AddItem", 0x004553E0, &shop::AddItem);
C04C_SH_REVERSED("CShop::BuyWithMoney", 0x00455490, &shop::BuyWithMoney);
C04C_SH_REVERSED("CShop::BuyWithTickets", 0x00455550, &shop::BuyWithTickets);
C04C_SH_REVERSED("CShop::GetMaxBuyable", 0x004555C0, &shop::GetMaxBuyable);
C04C_SH_REVERSED("CShop::DrawPanelBackground", 0x004556F0, &shop::DrawPanelBackground);
C04C_SH_REVERSED("CShop::FlashColour", 0x004557A0, &shop::FlashColour);
C04C_SH_REVERSED("CShop::IsShopInterior", 0x00455850, &shop::IsShopInterior);
C04C_SH_REVERSED("CShop::IsInArea50", 0x004558A0, &shop::IsInArea50);
C04C_SH_REVERSED("CShopItem::CShopItem", 0x004558B0, &shop::ItemConstruct);
C04C_SH_REVERSED("CShopItem::RemoveObject", 0x00455960, &shop::ItemRemoveObject);
C04C_SH_REVERSED("CShopItem::Update", 0x004559F0, &shop::ItemUpdate);
C04C_SH_REVERSED("CShop::Reset", 0x00455B60, &shop::Reset);
C04C_SH_REVERSED("CShop::Buy", 0x00455D30, &shop::Buy);
