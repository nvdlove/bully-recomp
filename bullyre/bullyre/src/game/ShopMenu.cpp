// CShop: the shops' browsing (the one shop at 0xC18748): the items on their shelves,
// the clerk, the item's text panel, buying through the clerk's speech. 0x455D60..0x457A20.
// (the item's own update/remove and the shop's camera and dialog helpers at 0x4551A0..
// 0x455B60 are another file's)

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

namespace {

template <typename T>
re::Field<T>& Field(void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(base) + offset);
}
re::Ptr32<void>* Vtable(void* object) {
    return *static_cast<re::Ptr32<re::Ptr32<void>>*>(object);
}

struct Rgba {
    uint8_t r, g, b, a;
};
static_assert(sizeof(Rgba) == 4);

// An item on a shelf (0x44)
struct CShopItem {
    uint8_t used;       // +0
    uint8_t pad1[3];
    int32_t model;      // +4
    re::Ptr32<void> object;       // +8 the prop shown (a registered reference)
    int32_t price;      // +0xC (money or tickets; 0: free)
    int32_t stock;      // +0x10 (0: out of stock)
    int32_t unknown14;  // +0x14
    float x, y, z;      // +0x18
    float heading;      // +0x24 degrees
    float unknown28;    // +0x28
    float unknown2C;    // +0x2C
    float unknown30;    // +0x30
    float unknown34;    // +0x34
    float unknown38;    // +0x38 (0.05 when made)
    float rotation;     // +0x3C radians (turning while it's the current one)
    uint32_t callback;  // +0x40 a script callback
};
static_assert(sizeof(CShopItem) == 0x44);

// A shelf (0x58, cShelf's vtable only)
struct CShopShelf {
    re::Ptr32<void> vtable;           // +0
    re::Ptr32<CShopItem> items[20];   // +4
    int8_t count;           // +0x54
    uint8_t pad55[3];
};
static_assert(sizeof(CShopShelf) == 0x58);

struct CShop {
    int32_t state;            // +0 0 none, 1 browsing, 2 leaving, 3 the bike question
    int32_t prevState;        // +4
    int32_t currency;         // +8 0 money, 1 tickets
    uint8_t active;           // +0xC
    uint8_t padD[3];
    re::Ptr32<uint8_t> clerk;           // +0x10 CPed (a registered reference)
    int32_t clerkModel;       // +0x14
    float clerkX, clerkY, clerkZ;  // +0x18
    float clerkHeading;       // +0x24 degrees
    float unknown28[8];       // +0x28
    float cameraTarget[3];    // +0x48 (the store camera's)
    float zoom;               // +0x54 (ShopSetCameraZoomPercentage)
    float angleOffset;        // +0x58 (ShopSetCameraAngleOffset)
    CShopShelf shelves[5];    // +0x5C
    uint8_t numShelves;       // +0x214
    char name[0x30];          // +0x215 (the clerk's speech: the shop)
    char speechPrefix[0x18];  // +0x245
    char speechExit[0x18];    // +0x25D
    char speechBuy[0x18];     // +0x275
    char speechNoMoney[0x18]; // +0x28D
    char speechFull[0x18];    // +0x2A5
    char speechNoStock[0x18]; // +0x2BD
    int8_t curShelf;          // +0x2D5
    int8_t curItem;           // +0x2D6
    uint8_t unknown2D7;       // +0x2D7
    uint32_t stopCallback;    // +0x2D8 (ShopSetOnStopShoppingCB)
    CShopItem items[30];      // +0x2DC

    CShopItem* Current() { return shelves[curShelf].items[curItem]; }
};
static_assert(offsetof(CShop, clerk) == 0x10);
static_assert(offsetof(CShop, cameraTarget) == 0x48);
static_assert(offsetof(CShop, shelves) == 0x5C);
static_assert(offsetof(CShop, numShelves) == 0x214);
static_assert(offsetof(CShop, speechNoStock) == 0x2BD);
static_assert(offsetof(CShop, curShelf) == 0x2D5);
static_assert(offsetof(CShop, stopCallback) == 0x2D8);
static_assert(sizeof(CShop) == 0xAD4);

// ---- game data ----
GAME_VAR(int32_t, language, 0x00C18724);          // 7: the one with bigger text
GAME_VAR(int32_t, flashState, 0x00C1873C);        // 1 the money, 2 the item's price flashes
GAME_VAR(uint8_t, browsing, 0x00C18738);          // (the pocket money shown)
GAME_VAR(float, flashTime, 0x00C18740);
GAME_VAR(Rgba, textColour, 0x00C19230);
GAME_VAR(Rgba, titleColour, 0x00C1922C);
GAME_VAR(Rgba, infoColour, 0x00C19228);
inline Rgba* const flashColour = reinterpret_cast<Rgba*>(0x00CF01A4);
inline void* const panelColourA = reinterpret_cast<void*>(0x00C18744);
inline void* const panelColourB = reinterpret_cast<void*>(0x00C19224);
inline void* const lineColour = reinterpret_cast<void*>(0x020C33E4);
GAME_VAR(float, panelAlpha, 0x00A14934);
GAME_VAR(float, moneyX, 0x00A14984);
GAME_VAR(float, nameX, 0x00A14980);
GAME_VAR(float, priceOffsetY, 0x00A149A4);        // (1 for language 7)
GAME_VAR(float, moneyOffsetY, 0x00A149A0);        // (3 for language 7)
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);        // (its money at +0x1CA0)
inline void* const playerInfo = reinterpret_cast<void*>(0x00C1AEA8);
GAME_VAR(int32_t, ticketItem, 0x00BF42A4);
GAME_VAR(int32_t, freeItemModel, 0x00BF42A0);     // free with mini objective 1
GAME_VAR(int32_t, remapModelA, 0x00BF4120);
GAME_VAR(int32_t, remapModelB, 0x00BF4130);
GAME_ARRAY(void*, modelInfoPtrs, 0x00C67738);
GAME_VAR(CPool<uint8_t>*, objectPool, 0x00C0F614);
constexpr uintptr_t VT_cShelf = 0x009068A8;

// the font
GAME_VAR(Rgba, fontColour, 0x0208B764);
GAME_VAR(float, fontWrapLeft, 0x0208B774);
GAME_VAR(float, fontWrapCentre, 0x0208B778);
GAME_VAR(float, fontScale, 0x00BA4E10);
inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(const uint16_t*, __thiscall, Text_GetByHash, 0x0068FF10, void* text, uint32_t hash);
GAME_FN(void, __cdecl, Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688050, int style);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);           // (for the next Format)
GAME_FN(void, __cdecl, Font_AddString, 0x006880F0, const uint16_t* string);
GAME_FN(void, __cdecl, Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* format, int max);
GAME_FN(float, __cdecl, Font_GetHeight, 0x00688220);
GAME_FN(int16_t, __cdecl, Font_GetNumLines, 0x0068B030, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, Font_Print, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);

// the front end
GAME_FN(void, __cdecl, FE_DrawPanel, 0x006A4420, float, float, float, float, float alpha, void* colourA, void* colourB);
GAME_FN(void, __cdecl, FE_DrawLine, 0x006A4730, float x, float width, float y, void* colour, float thickness);
GAME_FN(uint8_t, __cdecl, FE_PadPressed, 0x006A3170, int pad);
GAME_FN(void, __cdecl, FE_PadClear, 0x006A3120, int pad);
GAME_FN(void, __cdecl, FE_PadUpdate, 0x006A31B0);
GAME_FN(void, __cdecl, FE_PlaySound, 0x006A2340, int sound);
GAME_FN(void, __cdecl, FE_6A2360, 0x006A2360);
GAME_VAR(uint8_t, fePadUp, 0x020C3274);
GAME_VAR(uint8_t, fePadDown, 0x020C3278);
GAME_VAR(uint8_t, fePadRight, 0x020C327C);
GAME_VAR(uint8_t, fePadLeft, 0x020C3280);
inline void* const feDialog = reinterpret_cast<void*>(0x020C3370);
GAME_VAR(int32_t, feDialogShown, 0x020C33B4);     // (feDialog +0x44)
GAME_FN(void, __thiscall, FEDialog_Draw, 0x006A5A10, void* dialog);
GAME_FN(void, __thiscall, FEDialog_SetText, 0x006A4BD0, void* dialog, const char* key);
GAME_FN(void, __thiscall, FEDialog_AddOption, 0x006A2960, void* dialog, const char* key);
GAME_FN(uint8_t, __cdecl, Pad_IsBeingPressed, 0x00738900, int button, int pad);

// the clerk's speech
inline void* const conversations = reinterpret_cast<void*>(0x00B9DAB8);
GAME_FN(void, __thiscall, Conv_684CF0, 0x00684CF0, void* self);
GAME_FN(void, __thiscall, Conv_683EC0, 0x00683EC0, void* self);
GAME_FN(void, __thiscall, Conv_AddPed, 0x00683ED0, void* self, void* ped);
GAME_FN(void, __thiscall, Conversations_SetMovePeds, 0x00683EB0, void* self, uint8_t on);
GAME_FN(void, __thiscall, Conv_Say, 0x00685C50, void* self, const char* name, const char* speech);
GAME_FN(void, __thiscall, Conv_SayGoodbye, 0x006879E0, void* self, void* player, void* clerk, const char* name,
        const char* speech);
GAME_FN(const char*, __cdecl, Shop_SpeechName, 0x00454ED0, const char* prefix, const char* name);   // (a static buffer)

// the camera and the input
inline uint8_t* const TheCamera = reinterpret_cast<uint8_t*>(0x00C3CC68);
GAME_VAR(void*, storeCamera, 0x00C3D2A4);          // (TheCamera +0x63C)
inline void* const userInput = reinterpret_cast<void*>(0x00A45A30);
GAME_FN(void, __thiscall, UserInputManager_SetEnabled2, 0x0050DFC0, void* self, int value);
GAME_FN(void, __thiscall, Camera_Init, 0x004F3720, void* camera);
GAME_FN(void, __thiscall, Camera_SetShot, 0x004F2C20, void* camera, int shot, int a, uint8_t instant, int b, int c);
GAME_FN(void, __thiscall, StoreCamera_Set10, 0x00509480, void* self, const float* v);
GAME_FN(void, __thiscall, StoreCamera_Focus, 0x005095F0, void* self, int handle, int, int instant);
inline re::Ptr32<void>* const hudVisibility = reinterpret_cast<re::Ptr32<void>*>(0x00C674D4);

// the script callback
GAME_FN(uint8_t, __thiscall, Callback_IsSet, 0x005D80E0, uint32_t* callback);
GAME_FN(void*, __thiscall, ScriptRef_GetState, 0x005D9C20, uint32_t* ref);
GAME_FN(int, __thiscall, ScriptRef_GetFunction, 0x005D80D0, uint32_t* ref);
GAME_FN(void, __thiscall, ScriptRef_Clear, 0x005D9C50, uint32_t* ref);
GAME_FN(void, __cdecl, Lua_Fn_5D8270, 0x005D8270, void* state);
GAME_FN(void, __cdecl, Lua_Fn_5D8670, 0x005D8670, void* state, int function);
GAME_FN(void, __cdecl, Lua_Call, 0x005D82A0, void* state, int args, int results);

// entities, models, streaming
GAME_FN(void, __cdecl, CleanUpReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float heading);
GAME_FN(void, __thiscall, Entity_443AA0, 0x00443AA0, void* entity);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00465760, void* entity);
GAME_FN(void, __cdecl, Rw_UpdateFrame, 0x006C9AF0, void* frame);
GAME_FN(void*, __cdecl, Object_New, 0x004CFCD0, uintptr_t size);
GAME_FN(void*, __thiscall, Object_Construct, 0x004D0580, void* object, int model, int);
GAME_FN(void*, __cdecl, Ped_New, 0x004789E0, uintptr_t size);
GAME_FN(void*, __thiscall, CivilianPed_Construct, 0x0046DFF0, void* ped, int model, int, int);
GAME_FN(void, __thiscall, Ped_478960, 0x00478960, void* ped);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x00475730, void* ped, float heading);
GAME_FN(uint8_t, __thiscall, Ped_CanAfford, 0x004730E0, void* ped, int price);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __thiscall, AnimData_Construct, 0x006BA240, void* animData);
GAME_FN(uint8_t*, __cdecl, GetButes, 0x0051B450, int model);
GAME_FN(uint8_t, __cdecl, IsVehicleClass1, 0x0051B3B0, int model);   // (a bike)
GAME_FN(void*, __cdecl, GetModelInfoByName, 0x0051C190, const char* name, int* id);
GAME_FN(void, __thiscall, ModelInfo_SetRemap, 0x00535680, void* modelInfo, int a, int b, void* clump);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);   // (+4: the load state, 1 loaded)
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, uint32_t flags);
GAME_FN(void, __cdecl, LoadAllRequestedModels, 0x00533EE0, int priorityOnly);
GAME_FN(void, __cdecl, SetModelNotNeededAmbient, 0x0052FE40, int model);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(int, __cdecl, Items_GetCount, 0x00541E00, int item);
GAME_FN(int, __cdecl, Player_GetBike, 0x004C89B0, int);            // (the model, <0: none)
GAME_FN(uint8_t, __thiscall, PlayerInfo_GetMiniObjectiveComplete, 0x00446860, void* self, int id);
GAME_FN(uint8_t, __cdecl, Fn_514120, 0x00514120);

// the shop's other helpers (0x4551A0..0x455B60)
GAME_FN(Rgba*, __cdecl, Shop_FlashColour, 0x004557A0, Rgba* out, const Rgba* colour, const Rgba* flash);
GAME_FN(void, __thiscall, Shop_SetFlash, 0x004551A0, CShop* shop, int state);
GAME_FN(void, __thiscall, Shop_UpdateQuestion, 0x004551C0, CShop* shop);
GAME_FN(uint8_t, __thiscall, Shop_CanAfford, 0x004552A0, CShop* shop, int price);
GAME_FN(void, __thiscall, Shop_UpdateCamera, 0x004552E0, CShop* shop);
GAME_FN(int, __thiscall, Shop_GetAvailable, 0x004555C0, CShop* shop);   // (room left in the inventory)
GAME_FN(void, __thiscall, Shop_455B60, 0x00455B60, CShop* shop);
GAME_FN(CShopItem*, __thiscall, ShopItem_Construct, 0x004558B0, CShopItem* item);
GAME_FN(void, __thiscall, ShopItem_RemoveObject, 0x00455960, CShopItem* item);
GAME_FN(void, __thiscall, ShopItem_Update, 0x004559F0, CShopItem* item, int current, int currency);

// ours, through the game
GAME_FN(void, __cdecl, Shop_DrawMoneyCall, 0x00455D60, float x, float y, int force, int tickets);
GAME_FN(void, __thiscall, Shop_DrawItemInfoCall, 0x00455FF0, CShop* shop, uint32_t title, int price, uint32_t desc,
        int stock, int available, int tickets);
GAME_FN(void, __thiscall, Shop_DrawCurrentItemCall, 0x00456D80, CShop* shop);
GAME_FN(void, __thiscall, ShopItem_CreateObjectCall, 0x00456A70, CShopItem* item);
GAME_FN(void, __thiscall, Shop_CreateClerkCall, 0x00456F20, CShop* shop);
GAME_FN(void, __thiscall, Shop_StopBrowsingCall, 0x00457000, CShop* shop);
GAME_FN(void, __thiscall, Shop_FocusCurrentCall, 0x004571D0, CShop* shop, int instant);
GAME_FN(void, __thiscall, Shop_PrevItemCall, 0x00457230, CShop* shop);
GAME_FN(void, __thiscall, Shop_NextItemCall, 0x004572C0, CShop* shop);
GAME_FN(void, __thiscall, Shop_NextShelfCall, 0x00457350, CShop* shop);
GAME_FN(void, __thiscall, Shop_PrevShelfCall, 0x004573F0, CShop* shop);
GAME_FN(void, __thiscall, Shop_EndCall, 0x00457560, CShop* shop);
GAME_FN(void, __thiscall, Shop_UpdateBrowsingCall, 0x00457660, CShop* shop);

// the item's heading in degrees to radians (all in double, as the game's x87 code)
float ToRadians(float degrees) {
    return float((double(degrees) - 90.0) * 3.1415927410125732 / 180.0);
}

// what the camera and the script know an object by: its pool slot and the slot's flags
int ObjectHandle(void* object) {
    int index = objectPool->GetIndex(static_cast<uint8_t*>(object));
    return (index << 8) + objectPool->flags[index];
}

// the colour, or its flash when `state` is the one flashing (the flash always runs on)
Rgba FlashIf(int state, const Rgba& colour) {
    if (flashState == state) {
        Rgba out;
        return *Shop_FlashColour(&out, &colour, flashColour);
    }
    return colour;
}

// the current item wrapped round within its shelf
void WrapItem(CShop* self) {
    CShopShelf& shelf = self->shelves[self->curShelf];
    if (self->curItem < 0) {
        self->curItem = int8_t(shelf.count + self->curItem);
    }
    self->curItem = int8_t(int(self->curItem) % int(shelf.count));
}

void FocusNoWait(CShop* self) {
    int handle = ObjectHandle(self->Current()->object);
    if (Field<int32_t>(TheCamera, 0x90) != 7) {
        Shop_UpdateCamera(self);
    }
    StoreCamera_Focus(storeCamera, handle, 0, 0);
}

} // namespace

namespace shop {

// 0x455D60: the pocket money (or the tickets) under the panel; only while browsing
// unless forced
void __cdecl DrawMoney(float x, float y, int force, int tickets) {
    Rgba colour = FlashIf(1, textColour);
    float offset = 0.0f;
    if (!browsing && !re::Flag(force)) {
        return;
    }
    uint16_t text[32];
    Font_Reset();
    Font_SetScale(1);
    fontColour = colour;
    if (re::Flag(tickets)) {
        Font_AddNumber(float(Items_GetCount(ticketItem)));
        Font_Format(text, Text_Get(TheText, "STO_TICKETS"), 0x20);
        if (language == 7) {
            offset = -2.0f;
        }
    } else {
        Font_AddNumber(float(uint32_t(Field<int32_t>(playerPed, 0x1CA0))));
        Font_Format(text, Text_Get(TheText, "POCKET_MONEY_STR"), 0x20);
    }
    Font_SetJustify(2);
    offset += y;
    Font_Print(x, offset, text, 1);
    Font_Render();
}

// 0x455EF0: the money, always
void __cdecl DrawMoneyAlways() {
    Rgba colour = FlashIf(1, infoColour);
    uint16_t text[32];
    Font_Reset();
    fontColour = colour;
    Font_AddNumber(float(uint32_t(Field<int32_t>(playerPed, 0x1CA0))));
    Font_Format(text, Text_Get(TheText, "STO_MONEY"), 0x20);
    Font_SetJustify(2);
    Font_Print(moneyX, 380.0f, text, 1);
    Font_Render();
}

// 0x455FF0 (thiscall): the item panel: its name, price (or why it can't be had), its
// description, the money and the buttons
void __fastcall DrawItemInfo(CShop* self, re::Edx, uint32_t titleHash, int price, uint32_t descHash, int stock,
                             int available, int tickets) {
    if (language == 7) {
        priceOffsetY = 1.0f;
        moneyOffsetY = 3.0f;
    }
    FE_DrawPanel(60.0f, 82.5f, 340.0f, 397.5f, panelAlpha, panelColourA, panelColourB);
    FE_DrawLine(200.0f, 220.0f, 130.0f, lineColour, 2.0f);
    FE_DrawLine(200.0f, 220.0f, 155.0f, lineColour, 2.0f);
    FE_DrawLine(200.0f, 220.0f, 285.0f, lineColour, 2.0f);
    FE_DrawLine(200.0f, 220.0f, 310.0f, lineColour, 2.0f);
    const uint16_t* title = Text_GetByHash(TheText, titleHash);
    const uint16_t* desc = descHash != 0 ? Text_GetByHash(TheText, descHash) : nullptr;
    Font_Reset();
    Font_SetScale(3);
    Font_SetJustify(2);
    fontWrapCentre = 260.0f;
    if (title) {
        fontColour = titleColour;
        int lines = Font_GetNumLines(200.0f, 0.0f, title);
        float y = float(106.25 - double(Font_GetHeight()) * lines * 0.5 - 1.0);
        Font_Print(200.0f, y, title, 1);
    }

    if (price >= 0) {
        Rgba colour = FlashIf(2, textColour);
        uint16_t line[0x100];
        const uint16_t* format;
        if (stock == 0) {
            if (price == 0) {
                format = Text_Get(TheText, "STO_FREEOUTOFSTOCK");
            } else {
                Font_AddNumber(float(price));
                format = Text_Get(TheText, re::Flag(tickets) ? "STO_OUTOFSTOCKT" : "STO_OUTOFSTOCK");
            }
        } else if (available <= 0) {
            if (price == 0) {
                format = Text_Get(TheText, "STO_FREEINVENTORYFULL");
            } else {
                Font_AddNumber(float(price));
                format = Text_Get(TheText, re::Flag(tickets) ? "STO_INVENTORYFULLT" : "STO_INVENTORYFULL");
            }
        } else {
            const char* key;
            if (price == 0) {
                key = "STO_FREE";
            } else {
                uint16_t units[32];
                uint16_t cost[32];
                Font_AddNumber(float(available));
                Font_Format(units, Text_Get(TheText, available == 1 ? "STO_UNIT" : "STO_UNITS"), 0x1F);
                Font_AddNumber(float(price));
                if (re::Flag(tickets)) {
                    Font_Format(cost, Text_Get(TheText, price == 1 ? "STO_TICKET" : "STO_TICKETS"), 0x1F);
                } else {
                    Font_Format(cost, Text_Get(TheText, "STO_MONEY"), 0x1F);
                }
                Font_AddString(units);
                Font_AddString(cost);
                key = "STO_UNITPRICE";
            }
            format = Text_Get(TheText, key);
        }
        Font_Format(line, format, 0xFF);
        Font_SetScale(1);
        fontColour = colour;
        float y = float(142.5 - double(Font_GetHeight()) * 0.5 + double(priceOffsetY));
        Font_Print(200.0f, y, line, 1);
    }

    if (desc) {
        float x;
        if (IsVehicleClass1(self->Current()->model)) {
            Font_SetJustify(0);
            x = 80.0f;
            fontWrapLeft = 300.0f;
        } else {
            x = 200.0f;
        }
        Font_SetScale(2);
        fontColour = infoColour;
        uint16_t text[0x100];
        Font_Format(text, desc, 0xFF);
        int lines = Font_GetNumLines(x, 0.0f, text);
        float y = float(220.0 - double(Font_GetHeight()) * lines * 0.5 - 2.0);
        Font_Print(x, y, text, 1);
    }

    float moneyY = float(297.5 - double(Font_GetHeight()) * 0.5 + double(moneyOffsetY));
    Shop_DrawMoneyCall(200.0f, moneyY, 0, tickets);
    Font_Reset();
    Font_SetScale(1);
    Font_SetJustify(2);
    fontColour = textColour;
    if (stock != 0 && available > 0) {
        uint8_t canBuy;
        bool known = true;
        if (self->currency == 0) {
            canBuy = Ped_CanAfford(playerPed, price);
        } else if (self->currency == 1) {
            canBuy = uint32_t(Items_GetCount(ticketItem)) >= uint32_t(price);
        } else {
            canBuy = 0;
            known = false;
        }
        if (known && canBuy) {
            const uint16_t* text = Text_Get(TheText, "STO_BUYEXIT");
            Font_Print(200.0f, float(353.75 - double(Font_GetHeight()) - 3.0), text, 1);
        }
    }
    Font_Print(200.0f, 353.75f, Text_Get(TheText, "STO_NAVIGATE"), 1);
    const uint16_t* exitText = Text_Get(TheText, "STO_EXIT");
    Font_Print(200.0f, float(double(Font_GetHeight()) + 353.75 - -3.0), exitText, 1);
    Font_Render();
}

// 0x456790 (thiscall): an item's name and price (the script's shops draw it; `this`
// unused)
void __fastcall DrawItemName(CShop*, re::Edx, uint32_t hash, int price, int purchased) {
    Font_Reset();
    Font_SetScale(1);
    Font_SetJustify(2);
    fontWrapCentre = 240.0f;
    int lines = 0;
    const uint16_t* title = Text_GetByHash(TheText, hash);
    bool bought = re::Flag(purchased) && price >= 0;
    int titleLines = 0;
    if (title) {
        titleLines = Font_GetNumLines(nameX, 0.0f, title);
        lines = titleLines;
    }
    if (bought || price >= 0) {
        lines++;
    }
    if (lines <= 0) {
        return;
    }
    int priceLine = 0;
    float y = float(285.0 - double(Font_GetHeight()) * lines * 0.5);
    if (title) {
        fontColour = bought ? infoColour : FlashIf(2, infoColour);
        Font_Print(nameX, y, title, 1);
        fontColour = textColour;
        priceLine = titleLines;
    }
    if (price < 0) {
        return;
    }
    Rgba colour = FlashIf(2, infoColour);
    Font_AddNumber(float(price));
    const char* key = bought ? "STO_PURCHASED" : price == 0 ? "STO_FREE" : "STO_MONEY";
    uint16_t text[64];
    Font_Format(text, Text_Get(TheText, key), 0x40);
    fontColour = colour;
    Font_Print(nameX, float(double(Font_GetHeight()) * priceLine + double(y)), text, 1);
    fontColour = textColour;
}

// 0x456A70 (thiscall): the item's prop made and put on its shelf
void __fastcall ItemCreateObject(CShopItem* self, re::Edx) {
    uint8_t* butes = GetButes(self->model);
    int model = self->model;
    if (GetStreamingInfo(model)[4] != 1) {
        RequestModel(model, 5);
        LoadAllRequestedModels(0);
    }
    if (model == remapModelA) {
        void* mi = modelInfoPtrs[model];
        ModelInfo_SetRemap(mi, 3, 3, Field<void*>(mi, 0x20));
    } else if (model == remapModelB) {
        void* mi = modelInfoPtrs[model];
        ModelInfo_SetRemap(mi, 99, 99, Field<void*>(mi, 0x20));
    }
    // (the butes may name another model to show)
    if (const char* shown = Field<const char*>(butes, 0x58)) {
        GetModelInfoByName(shown, &model);
        if (model < 0) {
            model = self->model;
        }
    }
    void* memory = Object_New(0x22C);
    uint8_t* object = memory ? static_cast<uint8_t*>(Object_Construct(memory, model, 0)) : nullptr;
    auto animFile = [&]() {
        void* mi = modelInfoPtrs[model];
        return re::PtrCast<int(__thiscall*)(void*)>(Vtable(mi)[7])(mi);
    };
    if (animFile() != -1) {
        void* anim = Malloc(0x104);
        if (anim) {
            AnimData_Construct(anim);
        }
        Field<void*>(object, 0x114) = anim;
    }
    re::PtrCast<void(__thiscall*)(void*, int, int)>(Vtable(object)[10])(object, 1, 1);
    object[0x208] = 2;
    Entity_SetPosition(object, self->x, self->y, self->z);
    float heading = ToRadians(self->heading);
    if (void* matrix = Field<void*>(object, 0x14)) {
        Matrix_SetRotateZOnly(matrix, heading);
    }
    Field<float>(object, 0x10) = heading;
    auto updateAnim = [&]() {
        if (animFile() != -1) {
            bool was = Field<int32_t>(object, 0xE4) != 0;
            Field<int32_t>(object, 0xE4) = 1;
            re::PtrCast<void(__thiscall*)(void*)>(Vtable(object)[19])(object);
            Field<int32_t>(object, 0xE4) = was;
        }
    };
    updateAnim();
    object[0x1BC] &= ~2;
    object[0x209] &= ~1;
    Field<int32_t>(object, 0x44) = 1;
    Field<int32_t>(object, 0x20) = 0;
    Field<int32_t>(object, 0xBC) = 1;
    Entity_443AA0(object);
    if (void* anim = Field<void*>(object, 0x114)) {
        if (void* frame = Field<void*>(anim, 0xC)) {
            Rw_UpdateFrame(frame);
        }
    } else {
        Entity_UpdateRwFrame(object);
    }
    World_Add(object);
    updateAnim();
    if (object != self->object) {
        if (self->object) {
            CleanUpReference(self->object, &self->object);
        }
        self->object = object;
        RegisterReference(object, &self->object);
    }
}

// 0x456CD0 (thiscall): the constructor
CShop* __fastcall Construct(CShop* self, re::Edx) {
    self->clerkX = self->clerkY = self->clerkZ = 0.0f;
    self->clerk = nullptr;
    self->clerkHeading = 0.0f;
    for (float& f : self->unknown28) {
        f = 0.0f;
    }
    for (float& f : self->cameraTarget) {
        f = 0.0f;
    }
    for (CShopShelf& shelf : self->shelves) {
        shelf.vtable = reinterpret_cast<void*>(VT_cShelf);
        shelf.count = 0;
    }
    self->stopCallback = 0;
    for (CShopItem& item : self->items) {
        ShopItem_Construct(&item);
    }
    Shop_455B60(self);
    return self;
}

// 0x456D80 (thiscall): the current item's panel
void __fastcall DrawCurrentItem(CShop* self, re::Edx) {
    uint8_t* butes = GetButes(self->Current()->model);
    uint32_t titleHash = 0;
    uint32_t descHash = 0;
    const char* descName = nullptr;
    if (butes) {
        descName = Field<const char*>(butes, 0x48);
        if (const char* titleName = Field<const char*>(butes, 0x8C)) {
            titleHash = HashName(titleName);
        }
    }
    if (descName) {
        descHash = HashName(descName);
    }
    CShopItem* item = self->Current();
    // (the game passes the flag as a dword with only its low byte set: the rest is stack)
    int tickets = self->currency == 1;
    int available = Shop_GetAvailable(self);
    Shop_DrawItemInfoCall(self, titleHash, item->price, descHash, item->stock, available, tickets);
}

// 0x456E30 (thiscall): the shop drawn (the panel, the bike question, the items)
void __fastcall Draw(CShop* self, re::Edx) {
    if (!self->active) {
        return;
    }
    float savedScale = fontScale;
    if (language == 7) {
        fontScale = 0.9f;
    }
    if (self->active && (self->state == 1 || self->state == 3)) {
        Shop_DrawCurrentItemCall(self);
        if (self->state == 3) {
            FEDialog_Draw(feDialog);
        }
    }
    for (int s = 0; s < self->numShelves; s++) {
        CShopShelf& shelf = self->shelves[int8_t(s)];
        for (int i = 0; i < shelf.count; i++) {
            CShopItem* item = shelf.items[int8_t(i)];
            if (item->used) {
                int current = self->state == 1 && item == self->Current();
                ShopItem_Update(item, current, self->currency);
            }
        }
    }
    if (language == 7) {
        fontScale = savedScale;
    }
}

// 0x456F20 (thiscall): the clerk made at his place
void __fastcall CreateClerk(CShop* self, re::Edx) {
    if (GetStreamingInfo(self->clerkModel)[4] != 1) {
        RequestModel(self->clerkModel, 5);
    }
    void* memory = Ped_New(0x1E90);
    uint8_t* ped = memory ? static_cast<uint8_t*>(CivilianPed_Construct(memory, self->clerkModel, 2, 0)) : nullptr;
    Field<int32_t>(ped, 0x1310) = 10;
    Ped_478960(ped);
    Entity_SetPosition(ped, self->clerkX, self->clerkY, self->clerkZ);
    Ped_SetHeading(ped, ToRadians(self->clerkHeading));
    World_Add(ped);
    if (ped != self->clerk) {
        if (self->clerk) {
            CleanUpReference(self->clerk, &self->clerk);
        }
        self->clerk = ped;
        RegisterReference(ped, &self->clerk);
    }
}

// 0x457000 (thiscall): browsing over: the items turned back, the clerk put back, the
// camera and the controls back, the script's callback
void __fastcall StopBrowsing(CShop* self, re::Edx) {
    self->state = 0;
    UserInputManager_SetEnabled2(userInput, 1);
    for (int s = 0; s < self->numShelves; s++) {
        CShopShelf& shelf = self->shelves[int8_t(s)];
        for (int i = 0; i < shelf.count; i++) {
            CShopItem* item = shelf.items[int8_t(i)];
            if (item->used) {
                item->rotation = ToRadians(item->heading);
                ShopItem_Update(item, 0, self->currency);
            }
        }
    }
    uint8_t* clerk = self->clerk;
    float x = self->clerkX, y = self->clerkY, z = self->clerkZ;
    if (uint8_t* matrix = Field<uint8_t*>(clerk, 0x14)) {
        Field<float>(matrix, 0x30) = x;
        Field<float>(matrix, 0x34) = y;
        Field<float>(matrix, 0x38) = z;
        // (the game also copies an uninitialised stack dword to +0x3C, the matrix's pad)
    }
    Field<float>(clerk, 4) = x;
    Field<float>(clerk, 8) = y;
    Field<float>(clerk, 0xC) = z;
    Ped_SetHeading(self->clerk, ToRadians(self->clerkHeading));
    browsing = 0;
    Field<int32_t>(TheCamera, 0x1A4) = 1;
    TheCamera[0x1A8] = 0;
    Field<int32_t>(TheCamera, 0x270) = 0;
    Camera_Init(TheCamera);
    Camera_SetShot(TheCamera, 1, 0, 1, 0, 0);
    self->curShelf = 0;
    self->curItem = 0;
    uint32_t* callback = &self->stopCallback;
    if (Callback_IsSet(callback) && Callback_IsSet(callback)) {
        void* state = ScriptRef_GetState(callback);
        Lua_Fn_5D8270(state);
        Lua_Fn_5D8670(state, ScriptRef_GetFunction(callback));
        Lua_Call(state, 0, 0);
    }
}

// 0x4571D0 (thiscall): the store camera onto the current item
void __fastcall FocusCurrent(CShop* self, re::Edx, int instant) {
    int handle = ObjectHandle(self->Current()->object);
    if (Field<int32_t>(TheCamera, 0x90) != 7) {
        Shop_UpdateCamera(self);
    }
    // (the game's flag dword has the handle's upper bits above the low byte)
    StoreCamera_Focus(storeCamera, handle, 0, re::Flag(instant) ? 1 : 0);
}

// 0x457230 (thiscall): the previous item on the shelf
void __fastcall PrevItem(CShop* self, re::Edx) {
    self->curItem = int8_t(self->curItem - 1);
    WrapItem(self);
    FocusNoWait(self);
}

// 0x4572C0 (thiscall): the next item on the shelf
void __fastcall NextItem(CShop* self, re::Edx) {
    self->curItem = int8_t(self->curItem + 1);
    WrapItem(self);
    FocusNoWait(self);
}

// 0x457350 (thiscall): the next shelf
void __fastcall NextShelf(CShop* self, re::Edx) {
    self->curShelf = int8_t(int(int8_t(uint8_t(self->curShelf) + 1)) % int(self->numShelves));
    WrapItem(self);
    FocusNoWait(self);
}

// 0x4573F0 (thiscall): the previous shelf
void __fastcall PrevShelf(CShop* self, re::Edx) {
    self->curShelf = int8_t(self->curShelf - 1);
    if (self->curShelf < 0) {
        self->curShelf = int8_t(self->numShelves + self->curShelf);
    }
    WrapItem(self);
    FocusNoWait(self);
}

// 0x4574A0 (thiscall): ShopStart: the items' props and the clerk made, the HUD hidden
void __fastcall Start(CShop* self, re::Edx, int currency) {
    self->currency = currency;
    self->state = 0;
    self->prevState = 0;
    self->curShelf = 0;
    self->curItem = 0;
    void* hud = *hudVisibility;
    browsing = 0;
    re::PtrCast<void(__thiscall*)(void*, int)>(Vtable(hud)[5])(hud, 0);
    static_cast<uint8_t*>(hud)[0x30] = 1;
    for (int s = 0; s < self->numShelves; s++) {
        CShopShelf& shelf = self->shelves[int8_t(s)];
        for (int i = 0; i < shelf.count; i++) {
            CShopItem* item = shelf.items[int8_t(i)];
            if (item->used) {
                ShopItem_CreateObjectCall(item);
            }
        }
    }
    Shop_CreateClerkCall(self);
    LoadAllRequestedModels(0);
    flashState = 0;
    self->active = 1;
}

// 0x457560 (thiscall): ShopEnd: the clerk and the props gone, the shelves emptied
void __fastcall End(CShop* self, re::Edx) {
    if (self->state == 1) {
        Shop_StopBrowsingCall(self);
    }
    self->active = 0;
    if (self->clerk) {
        SetModelNotNeededAmbient(Field<int16_t>(self->clerk, 0x10E));
        World_Remove(self->clerk);
        // (the reference is read again after each call: deleting the ped clears it)
        if (void* clerk = self->clerk) {
            re::PtrCast<void(__thiscall*)(void*, int)>(Vtable(clerk)[0])(clerk, 1);
        }
        if (self->clerk) {
            CleanUpReference(self->clerk, &self->clerk);
            self->clerk = nullptr;
        }
    }
    for (int s = 0; s < self->numShelves; s++) {
        CShopShelf& shelf = self->shelves[int8_t(s)];
        for (int i = 0; i < shelf.count; i++) {
            CShopItem* item = shelf.items[int8_t(i)];
            if (item->used) {
                ShopItem_RemoveObject(item);
            }
        }
        shelf.count = 0;
    }
    self->curShelf = 0;
    self->curItem = 0;
    self->numShelves = 0;
    self->unknown2D7 = 0;
    if (Fn_514120()) {
        static_cast<uint8_t*>(*hudVisibility)[0x30] = 0;
    }
    ScriptRef_Clear(&self->stopCallback);
}

// 0x457660 (thiscall): browsing: moving between the items, buying (through the clerk's
// line), leaving
void __fastcall UpdateBrowsing(CShop* self, re::Edx) {
    if (FE_PadPressed(0)) {
        bool moved = true;
        if (fePadLeft) {
            Shop_PrevItemCall(self);
        } else if (fePadRight) {
            Shop_NextItemCall(self);
        } else if (fePadDown) {
            Shop_NextShelfCall(self);
        } else if (fePadUp) {
            Shop_PrevShelfCall(self);
        } else {
            moved = false;
        }
        if (moved) {
            FE_PadClear(0);
        }
    }
    if (self->Current()->model == freeItemModel && PlayerInfo_GetMiniObjectiveComplete(playerInfo, 1)) {
        self->Current()->price = 0;
    }
    if (Pad_IsBeingPressed(7, 0)) {
        // (the game tests 0 < fade: a NaN passes)
        if (!(0.0f < Field<float>(TheCamera, 0xC0))) {
            Conv_684CF0(conversations);
            Conv_683EC0(conversations);
            Conv_AddPed(conversations, playerPed);
            Conv_AddPed(conversations, self->clerk);
            Conversations_SetMovePeds(conversations, 0);
            const char* speech;
            if (self->Current()->stock == 0) {
                speech = self->speechNoStock;
            } else if (Shop_GetAvailable(self) <= 0) {
                speech = self->speechFull;
            } else {
                if (!Shop_CanAfford(self, self->Current()->price)) {
                    Conv_Say(conversations, self->name, Shop_SpeechName(self->speechPrefix, self->speechNoMoney));
                    Shop_SetFlash(self, 1);
                    FE_PadUpdate();
                    return;
                }
                if (IsVehicleClass1(self->Current()->model) && Player_GetBike(1) >= 0) {
                    CShopItem* item = self->Current();
                    FEDialog_SetText(feDialog, Player_GetBike(1) == item->model ? "STO_BSAME" : "STO_BOTHER");
                    FEDialog_AddOption(feDialog, "YES");
                    FEDialog_AddOption(feDialog, "NO");
                    feDialogShown = 1;
                    self->state = 3;
                    FE_PadUpdate();
                    return;
                }
                Conv_Say(conversations, self->name, Shop_SpeechName(self->speechPrefix, self->speechBuy));
                FE_PlaySound(0xB);
                FE_PadUpdate();
                return;
            }
            Conv_Say(conversations, self->name, Shop_SpeechName(self->speechPrefix, speech));
            flashTime = 0.0f;
            flashState = 2;
            FE_PlaySound(0xC);
            FE_PadUpdate();
            return;
        }
    } else if (Pad_IsBeingPressed(8, 0)) {
        Conv_684CF0(conversations);
        Conversations_SetMovePeds(conversations, 0);
        void* clerk = self->clerk;
        void* player = playerPed;
        Conv_SayGoodbye(conversations, player, clerk, self->name, Shop_SpeechName(self->speechPrefix, self->speechExit));
        FE_6A2360();
    }
    FE_PadUpdate();
}

// 0x4579A0 (thiscall): every frame, by the state
void __fastcall Update(CShop* self, re::Edx) {
    if (!self->active) {
        return;
    }
    switch (self->state) {
    case 1:
        if (self->prevState != 1) {
            UserInputManager_SetEnabled2(userInput, 0);
            browsing = 1;
            Shop_UpdateCamera(self);
            StoreCamera_Set10(storeCamera, self->cameraTarget);
            Shop_FocusCurrentCall(self, 1);
        } else {
            Shop_UpdateBrowsingCall(self);
        }
        break;
    case 2:
        Shop_StopBrowsingCall(self);
        break;
    case 3:
        Shop_UpdateQuestion(self);
        break;
    }
    self->prevState = self->state;
}

// 0x457A20 (thiscall): the destructor
void __fastcall Destruct(CShop* self, re::Edx) {
    Shop_EndCall(self);
    for (int i = 29; i >= 0; i--) {
        CShopItem* item = &self->items[i];
        if (item->used) {
            ShopItem_RemoveObject(item);
        }
        ScriptRef_Clear(&item->callback);
        if (item->object) {
            CleanUpReference(item->object, &item->object);
        }
    }
    ScriptRef_Clear(&self->stopCallback);
    for (int i = 4; i >= 0; i--) {
        self->shelves[i].vtable = reinterpret_cast<void*>(VT_cShelf);
    }
    if (self->clerk) {
        CleanUpReference(self->clerk, &self->clerk);
    }
}

} // namespace shop

// ---- registration ---------------------------------------------------------------------

#define C04D_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04D_REVERSED("CShop::DrawMoney", 0x00455D60, &shop::DrawMoney);
C04D_REVERSED("CShop::DrawMoneyAlways", 0x00455EF0, &shop::DrawMoneyAlways);
C04D_REVERSED("CShop::DrawItemInfo", 0x00455FF0, &shop::DrawItemInfo);
C04D_REVERSED("CShop::DrawItemName", 0x00456790, &shop::DrawItemName);
C04D_REVERSED("CShopItem::CreateObject", 0x00456A70, &shop::ItemCreateObject);
C04D_REVERSED("CShop::CShop", 0x00456CD0, &shop::Construct);
C04D_REVERSED("CShop::DrawCurrentItem", 0x00456D80, &shop::DrawCurrentItem);
C04D_REVERSED("CShop::Draw", 0x00456E30, &shop::Draw);
C04D_REVERSED("CShop::CreateClerk", 0x00456F20, &shop::CreateClerk);
C04D_REVERSED("CShop::StopBrowsing", 0x00457000, &shop::StopBrowsing);
C04D_REVERSED("CShop::FocusCurrent", 0x004571D0, &shop::FocusCurrent);
C04D_REVERSED("CShop::PrevItem", 0x00457230, &shop::PrevItem);
C04D_REVERSED("CShop::NextItem", 0x004572C0, &shop::NextItem);
C04D_REVERSED("CShop::NextShelf", 0x00457350, &shop::NextShelf);
C04D_REVERSED("CShop::PrevShelf", 0x004573F0, &shop::PrevShelf);
C04D_REVERSED("CShop::Start", 0x004574A0, &shop::Start);
C04D_REVERSED("CShop::End", 0x00457560, &shop::End);
C04D_REVERSED("CShop::UpdateBrowsing", 0x00457660, &shop::UpdateBrowsing);
C04D_REVERSED("CShop::Update", 0x004579A0, &shop::Update);
C04D_REVERSED("CShop::~CShop", 0x00457A20, &shop::Destruct);
