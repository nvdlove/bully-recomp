// The character triggers (0xBCD098): up to 24 script-placed people the player can walk
// up to and talk to (shopkeepers, mission givers: the script's PedCharacterTrigger),
// each with its ped spawned near the camera, a radar blip, an optional cost, and the
// conversation's names. 0x424280..0x424DD0, with the colour helpers beside them.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
#define CHARTRIG_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

struct TVector {
    float x, y, z;
};

// A trigger (0x100)
struct CharTrigger {
    int32_t model;           // +0x00 the ped's model (-1: none)
    uint8_t kind;            // +0x04
    uint8_t unknown05[3];
    TVector pos;             // +0x08
    float heading;           // +0x14 (radians: 0x4333A0 of the script's degrees)
    float radius;            // +0x18 how near the player has to be
    char name1[64];          // +0x1C the conversation's (0x6879E0's last)
    char name2[64];          // +0x5C
    int32_t unknown9C;       // +0x9C (0x5FB8B0 with the player's 0x475380 value)
    int32_t unknownA0;       // +0xA0
    int32_t blip;            // +0xA4 (-1: none)
    uint8_t hasCost;         // +0xA8
    uint8_t unknownA9[3];
    int32_t cost;            // +0xAC (0x16..0x18: 100, 500, 1000 of money; else an item)
    char paidText[24];       // +0xB0 shown when it's paid
    char cantPayText[24];    // +0xC8 shown when it can't be
    uint8_t missionArg;      // +0xE0 (with +0xE4 to the mission manager's 0x6AA9A0)
    uint8_t unknownE1[3];
    int32_t mission;         // +0xE4 (-1: none; its flag 0x40 hides the blip)
    int32_t blipSprite;      // +0xE8 (3 unless given)
    uint8_t unknownEC;       // +0xEC (to 0xB9DAB8's 0x683EB0)
    uint8_t unknownED[3];
    int32_t weapon;          // +0xF0 the ped's weapon model (-1: none)
    int32_t unknownF4;       // +0xF4
    uint8_t blipArg;         // +0xF8
    uint8_t unknownF9[3];
    int32_t ped;             // +0xFC the ped's script handle (-1: none)
};
static_assert(offsetof(CharTrigger, pos) == 0x08);
static_assert(offsetof(CharTrigger, name1) == 0x1C);
static_assert(offsetof(CharTrigger, name2) == 0x5C);
static_assert(offsetof(CharTrigger, blip) == 0xA4);
static_assert(offsetof(CharTrigger, cost) == 0xAC);
static_assert(offsetof(CharTrigger, paidText) == 0xB0);
static_assert(offsetof(CharTrigger, cantPayText) == 0xC8);
static_assert(offsetof(CharTrigger, missionArg) == 0xE0);
static_assert(offsetof(CharTrigger, mission) == 0xE4);
static_assert(offsetof(CharTrigger, weapon) == 0xF0);
static_assert(offsetof(CharTrigger, blipArg) == 0xF8);
static_assert(offsetof(CharTrigger, ped) == 0xFC);
static_assert(sizeof(CharTrigger) == 0x100);

// The list (the manager's +4)
struct CharTriggerList {
    CharTrigger entries[24];  // +0
    uint32_t count;           // +0x1800
};
static_assert(sizeof(CharTriggerList) == 0x1804);

struct CCharTriggers {
    uint8_t talking;        // +0 a trigger is being talked to
    uint8_t costShown;      // +1
    uint8_t unknown2[2];
    CharTriggerList list;   // +4
};
static_assert(offsetof(CCharTriggers, list) == 4);
static_assert(sizeof(CCharTriggers) == 0x1808);

GAME_VAR(uint8_t*, ct_playerPed, 0x00C1AEA8);
GAME_VAR(void*, ct_radar, 0x00C674CC);
GAME_VAR(uint8_t, ct_noRandomPeds, 0x00C2C10C);
GAME_VAR(void*, ct_camera, 0x00C3CCF4);
GAME_VAR(uint8_t*, ct_population, 0x00C2C108);  // (+0x782C, +0x7830: the ped spawn ranges; +0x7870 the height)
GAME_VAR(uint8_t, ct_inCutscene, 0x00B9DB1C);
GAME_VAR(int32_t, ct_widescreen, 0x00C3CD0C);
GAME_VAR(uint8_t*, ct_hudColours, 0x020C88A4);
GAME_ARRAY(uint8_t, ct_textColour, 0x0208B764);   // [4]
inline void* const ct_missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const ct_theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const ct_conversation = reinterpret_cast<void*>(0x00B9DAB8);
inline void* const ct_pad = reinterpret_cast<void*>(0x00A45A30);
inline void* const ct_talkButton = reinterpret_cast<void*>(0x01B652A4);
inline const char* const ct_talk = reinterpret_cast<const char*>(0x00900F38);  // "TALK"
inline TVector* const ct_zeroVector = reinterpret_cast<TVector*>(0x00BCC564);

GAME_FN(int, __cdecl, CT_FindItem, 0x00541EC0, int32_t cost);
GAME_FN(void, __cdecl, CT_UseItem, 0x00541E40, int item);
GAME_FN(uint8_t, __thiscall, CT_Ped_HasMoney, 0x004730E0, uint8_t* ped, int32_t amount);
GAME_FN(void, __thiscall, CT_Ped_TakeMoney, 0x00473100, uint8_t* ped, int32_t amount);
GAME_FN(void, __thiscall, CT_Ped_SetPaid, 0x0047D1B0, uint8_t* ped, int32_t paid);
GAME_FN(uint8_t, __thiscall, CT_Pad_Fn50DF50, 0x0050DF50, void* pad);
GAME_FN(void, __cdecl, CT_ShowPrompt, 0x0068F580, const char* text, float time);
GAME_FN(void, __thiscall, CT_Radar_RemoveBlip, 0x0054A940, void* radar, int32_t blip);
GAME_FN(int32_t, __thiscall, CT_Radar_AddBlip_x64, 0x0054A410, void* radar, int type, float, float, float, uint32_t colour, int a, int32_t sprite, uint8_t arg);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline int32_t CT_Radar_AddBlip(void* a0, int a1, TVector a2, uint32_t a3, int a4, int32_t a5, uint8_t a6) {
    return CT_Radar_AddBlip_x64(a0, a1, re::FloatAt(a2, 0), re::FloatAt(a2, 1), re::FloatAt(a2, 2), a3, a4, a5, a6);
}
GAME_FN(void, __thiscall, CT_Radar_SetBlipId, 0x00548A40, void* radar, int32_t blip, int32_t id);
GAME_FN(void, __thiscall, CT_Radar_SetBlipSize, 0x00548BF0, void* radar, int32_t blip, int size);
GAME_FN(void, __thiscall, CT_Radar_SetBlipVisible, 0x00548B60, void* radar, int32_t blip, uint8_t on);
GAME_FN(void, __thiscall, CT_Radar_AddEntityFlag, 0x0054A890, void* radar, int kind, int32_t ref);
GAME_FN(uint8_t*, __cdecl, CT_GetPedFromHandle, 0x005C7380, int32_t handle, int kind);
GAME_FN(void, __cdecl, CT_DeletePedHandle, 0x005C73E0, int32_t handle, int kind);
GAME_FN(float, __cdecl, CT_DegToRad, 0x004333A0, float degrees);
inline void __cdecl CT_strncpy(char* dest, const char* src, size_t n) { crt::Strncpy(dest, src, n); }
GAME_FN(void*, __cdecl, CT_CameraObject, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, CT_WorldTransform, 0x006C9A90, void* object);
GAME_FN(TVector*, __cdecl, CT_Sub, 0x00414B70, TVector* out, const TVector* a, const TVector* b);
GAME_FN(int, __thiscall, CT_Camera_Fn4F12F0, 0x004F12F0, void* camera);
GAME_FN(uint8_t*, __cdecl, CT_GetModelInfo, 0x0052DD30, int32_t model);   // (the streaming info: +4 its state)
GAME_FN(uint8_t, __cdecl, CT_IsModelLoaded, 0x004151E0, int32_t model);
GAME_FN(void, __cdecl, CT_RequestModel, 0x00532020, int32_t model, int flags);
GAME_FN(int32_t, __cdecl, CT_CreatePed, 0x005CF810, uintptr_t a, int32_t model, float x, float y, float z, float heading, int b);
GAME_FN(int, __thiscall, CT_Inventory_Find, 0x0045CF40, void* inventory, int32_t weapon);
GAME_FN(void, __thiscall, CT_Inventory_Give, 0x0045D030, void* inventory, int32_t weapon, int a, int b, int c);
GAME_FN(void*, __thiscall, CT_Ped_Fn48D020, 0x0048D020, void* ai, void* out);
GAME_FN(void, __cdecl, CT_GivePedTask, 0x0052A600, int a, uint8_t b, int c, void* d, float e, const TVector* f, int32_t g, int h, int i, int j, uint8_t* ped, int k, int l);
GAME_FN(uint8_t, __thiscall, CT_Fn5F48D0, 0x005F48D0, void* obj, uintptr_t id, int a, int b);
GAME_FN(void, __thiscall, CT_Fn5F4660, 0x005F4660, void* obj, uintptr_t id);
GAME_FN(float, __cdecl, CT_FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int flag);
GAME_FN(void, __thiscall, CT_Ped_Teleport, 0x0041C4E0, uint8_t* ped, float x, float y, float z);
GAME_FN(uint8_t*, __thiscall, CT_MissionMgr_GetTaskFlags, 0x006AA7C0, void* mgr, int32_t mission);
GAME_FN(uint8_t, __thiscall, CT_MissionMgr_IsAvailable, 0x006AA9A0, void* mgr, int32_t mission, uint8_t arg);
GAME_FN(void, __thiscall, CT_Ped_Fn475380, 0x00475380, uint8_t* ped, int32_t* out, int32_t value);
GAME_FN(uint8_t, __cdecl, CT_Fn5FB8B0, 0x005FB8B0, float a, float b, int32_t c);
GAME_FN(uint8_t, __thiscall, CT_Button_Fn5F4800, 0x005F4800, void* obj, void* button, int a, int b);
GAME_FN(uint8_t, __thiscall, CT_Ped_Fn473FD0, 0x00473FD0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, CT_Ped_Fn4CE690, 0x004CE690, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, CT_Ai_CanTalkTo, 0x0048D470, void* ai, uint8_t* ped);
GAME_FN(void, __cdecl, CT_Fn688E60, 0x00688E60);
GAME_FN(void, __cdecl, CT_SetTextColour, 0x00424480, const uint8_t* rgba);
GAME_FN(void, __cdecl, CT_PrintHelp, 0x00514340, const char* text, float time);
GAME_FN(float, __cdecl, CT_Fn738840, 0x00738840, int a, int b);
GAME_FN(void, __thiscall, CT_Conv_Fn683EB0, 0x00683EB0, void* conv, uint8_t arg);
GAME_FN(void, __thiscall, CT_Conv_Start, 0x006879E0, void* conv, uint8_t* player, uint8_t* ped, const char* name2, const char* name1);
// ours, through the game's address
GAME_FN(uint8_t, __thiscall, CT_TryPay, 0x00424280, CCharTriggers* self, uint8_t* ped, int32_t cost, int take);
GAME_FN(void, __thiscall, CT_List_RemoveAt, 0x004243C0, CharTriggerList* list, uint32_t index);
GAME_FN(void, __thiscall, CT_AddBlip, 0x00424640, CharTrigger* trigger);
GAME_FN(uint8_t*, __thiscall, CT_UpdatePed, 0x00424940, CCharTriggers* self, uint32_t index);

TVector& EntityPos(uint8_t* e) {
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(e + 0x14);
    return matrix ? *reinterpret_cast<TVector*>(matrix + 0x30) : *reinterpret_cast<TVector*>(e + 4);
}

} // namespace

namespace CharTriggers {

// 0x424280 (thiscall): the trigger's cost paid (or only checked, without `take`): an
// item (0x50: free), else 0x16..0x18 as 100, 500 or 1000 money. With `take`, the ped
// told (0x47D1B0) whether it was
uint8_t __fastcall TryPay(CCharTriggers* self, re::Edx, uint8_t* ped, int32_t cost, int take) {
    (void)self;
    uint8_t result = 0;
    // (the ped's flag is pushed as a whole stack slot of which only the low byte is set;
    // the rest is the manager's pointer)
    uint32_t paid = uint32_t(reinterpret_cast<uintptr_t>(self)) & 0xFFFFFF00;
    uint8_t* player = ct_playerPed;
    if (player && ped) {
        int item = CT_FindItem(cost);
        if (item == 0x50) {
            result = 1;
            paid = (paid & 0xFFFFFF00) | 1;
        } else if (item >= 0) {
            result = 1;
            paid = (paid & 0xFFFFFF00) | 1;
            if (!re::Flag(take)) {
                return 1;
            }
            CT_UseItem(item);
        } else {
            int32_t amount;
            switch (cost) {
            case 0x16:
                amount = 100;
                break;
            case 0x17:
                amount = 500;
                break;
            case 0x18:
                amount = 1000;
                break;
            default:
                amount = -1;
                break;
            }
            if (amount != -1 && CT_Ped_HasMoney(player, amount)) {
                result = 1;
                paid = (paid & 0xFFFFFF00) | 1;
                if (!re::Flag(take)) {
                    return 1;
                }
                CT_Ped_TakeMoney(player, amount);
            }
        }
    }
    if (re::Flag(take)) {
        CT_Ped_SetPaid(ped, int32_t(paid));
    }
    return result;
}

// 0x424340 (thiscall): the "TALK" prompt while a trigger is talked to (not while its
// cost is shown)
void __fastcall ShowTalkPrompt(CCharTriggers* self, re::Edx) {
    if (!self->talking || self->costShown) {
        return;
    }
    uint8_t* player = ct_playerPed;
    uint8_t* state = reinterpret_cast<uint8_t*(__thiscall*)(uint8_t*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(player))[0x40 / 4]))(player);
    if (state[0x1D] == 0 && CT_Pad_Fn50DF50(ct_pad)) {
        CT_ShowPrompt(ct_talk, 1.0f);
    }
}

// 0x424380 (thiscall, on the list): every position zeroed, none left
void __fastcall ClearList(CharTriggerList* list, re::Edx) {
    for (int i = 0; i < 24; i++) {
        list->entries[i].pos.x = 0.0f;
        list->entries[i].pos.y = 0.0f;
        list->entries[i].pos.z = 0.0f;
    }
    list->count = 0;
}

// 0x4243C0 (thiscall, on the list): the ones after moved down over it
void __fastcall RemoveAt(CharTriggerList* list, re::Edx, uint32_t index) {
    for (uint32_t i = index; i < list->count - 1; i++) {
        list->entries[i] = list->entries[i + 1];
    }
    list->count--;
}

// 0x424420 (thiscall): a colour from 0xRRGGBBAA
void __fastcall ColourFromUint(uint8_t* rgba, re::Edx, uint32_t value) {
    rgba[0] = uint8_t(value >> 24);
    rgba[1] = uint8_t(value >> 16);
    rgba[2] = uint8_t(value >> 8);
    rgba[3] = uint8_t(value);
}

// 0x424450 (thiscall): a colour copied; returns this
uint8_t* __fastcall ColourCopy(uint8_t* rgba, re::Edx, const uint8_t* from) {
    rgba[0] = from[0];
    rgba[1] = from[1];
    rgba[2] = from[2];
    rgba[3] = from[3];
    return rgba;
}

// 0x424480: the help text's colour (0x208B764)
void __cdecl SetTextColour(const uint8_t* rgba) {
    ct_textColour[0] = rgba[0];
    ct_textColour[1] = rgba[1];
    ct_textColour[2] = rgba[2];
    ct_textColour[3] = rgba[3];
}

// 0x4244B0 (thiscall): every trigger gone: blips and peds removed
void __fastcall Clear(CCharTriggers* self, re::Edx) {
    void* radar = ct_radar;
    for (uint32_t i = 0; i < self->list.count; i++) {
        CharTrigger& t = self->list.entries[i];
        if (radar && t.blip != -1) {
            CT_Radar_RemoveBlip(radar, t.blip);
        }
        if (t.ped != -1) {
            CT_DeletePedHandle(t.ped, 2);
        }
    }
    self->list.count = 0;
}

// 0x424530 (thiscall): the trigger of the kind at the position (x, y) removed
void __fastcall Remove(CCharTriggers* self, re::Edx, uint8_t kind, const float* pos) {
    for (uint32_t i = 0; i < self->list.count; i++) {
        CharTrigger& t = self->list.entries[i];
        if (t.kind != kind || t.pos.x != pos[0] || t.pos.y != pos[1]) {
            continue;
        }
        if (t.ped != -1) {
            CT_DeletePedHandle(t.ped, 2);
        }
        if (ct_radar && t.blip != -1) {
            CT_Radar_RemoveBlip(ct_radar, t.blip);
        }
        CT_List_RemoveAt(&self->list, i);
        return;
    }
}

// 0x4245E0 (thiscall): the ped is one of the triggers'
uint8_t __fastcall IsTriggerPed(CCharTriggers* self, re::Edx, uint8_t* ped) {
    for (uint32_t i = 0; i < self->list.count; i++) {
        int32_t handle = self->list.entries[i].ped;
        if (handle != -1 && CT_GetPedFromHandle(handle, 2) == ped) {
            return 1;
        }
    }
    return 0;
}

// 0x424640 (thiscall, on a trigger): its radar blip (type 4, colour 0x0000FFFF bytes)
void __fastcall AddBlip(CharTrigger* t, re::Edx) {
    void* radar = ct_radar;
    uint8_t colourBytes[4] = {0, 0, 0xFF, 0xFF};
    uint32_t colour;
    std::memcpy(&colour, colourBytes, 4);
    int32_t blip = CT_Radar_AddBlip(radar, 4, t->pos, colour, 1, t->blipSprite, t->blipArg);
    t->blip = blip;
    if (t->mission >= 0) {
        CT_Radar_SetBlipId(radar, blip, t->mission);
    }
}

// 0x4246B0 (thiscall): a 2D vector's length
float __fastcall Vector2DMagnitude(const float* v, re::Edx) {
    float sq = v[1] * v[1] + v[0] * v[0];
    return std::sqrt(std::fabs(sq));
}

// 0x4246E0 (thiscall, ret 0x50): a trigger added, unless one of that model is at the
// position (x, y) already, or the random peds are off
void __fastcall Add(CCharTriggers* self, re::Edx, int32_t model, uint8_t kind, const TVector* pos, float headingDegrees, float radius,
                    const char* name1, const char* name2, int32_t a8, int32_t a9, int32_t blipSprite, uint8_t blipArg, uint8_t hasCost,
                    int32_t cost, const char* paidText, const char* cantPayText, uint8_t missionArg, int32_t mission, uint8_t aEC,
                    int32_t weapon, int32_t aF4) {
    for (uint32_t i = 0; i < self->list.count; i++) {
        CharTrigger& t = self->list.entries[i];
        if (model == t.model && t.pos.x == pos->x && t.pos.y == pos->y) {
            return;
        }
    }
    if (ct_noRandomPeds) {
        return;
    }
    // (built on the stack: the bytes not set are the stack's)
    CharTrigger t;
    t.kind = kind;
    t.model = model;
    t.pos = *pos;
    t.heading = CT_DegToRad(headingDegrees);
    t.radius = radius;
    t.unknown9C = a8;
    t.unknownA0 = a9;
    t.cost = cost;
    t.missionArg = missionArg;
    t.mission = mission;
    t.unknownEC = aEC;
    t.weapon = weapon;
    t.ped = -1;
    t.hasCost = hasCost;
    t.paidText[0] = 0;
    t.cantPayText[0] = 0;
    t.blipSprite = 3;
    t.blip = -1;
    t.unknownF4 = aF4;
    t.blipArg = blipArg;
    if (hasCost) {
        if (paidText) {
            CT_strncpy(t.paidText, paidText, 0x17);
            t.paidText[0x17] = 0;
        }
        if (cantPayText) {
            CT_strncpy(t.cantPayText, cantPayText, 0x17);
            t.cantPayText[0x17] = 0;
        }
    }
    std::strcpy(t.name1, name1);
    std::strcpy(t.name2, name2);
    void* radar = ct_radar;
    if (blipSprite != -1) {
        t.blipSprite = blipSprite;
        CT_AddBlip(&t);
    }
    CT_Radar_SetBlipSize(radar, t.blip, 5);
    CT_Radar_SetBlipVisible(radar, t.blip, 0);
    self->list.entries[self->list.count] = t;
    self->list.count++;
}

// 0x424940 (thiscall): the trigger's ped: made when the camera is in the population's
// spawn ring (or near, with the camera's mode 2) and its models are loaded (requested
// otherwise), given its weapon and radar flags; while there, kept at its place (a task,
// put back when more than a unit away), deleted past the ring
uint8_t* __fastcall UpdatePed(CCharTriggers* self, re::Edx, uint32_t index) {
    CharTrigger* t = &self->list.entries[index];
    uint8_t* ped = nullptr;
    uint8_t* transform = CT_WorldTransform(CT_CameraObject(ct_camera));
    TVector cam = *reinterpret_cast<TVector*>(transform + 0x24);
    TVector d;
    CT_Sub(&d, &cam, &t->pos);
    float dist = std::sqrt(std::fabs(d.x * d.x + d.y * d.y));
    if (t->ped != -1) {
        ped = CT_GetPedFromHandle(t->ped, 2);
        if (ped) {
            if (ct_inCutscene) {
                return ped;
            }
            TVector at = t->pos;
            alignas(4) uint8_t task[0xC];   // (a CVector: 0x48D020 writes the ped's position, 12 bytes)
            CT_Ped_Fn48D020(ped + 0xA64, task);
            CT_GivePedTask(5, t->blipArg, 1, task, 0.5f, ct_zeroVector, t->mission, 0, 0x72, 0xBC, ped, 1, 5);
            *reinterpret_cast<int32_t*>(ped + 0x138C) = 1;
            *reinterpret_cast<int32_t*>(ped + 0x1360) = 1;
            uint8_t* obj = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x2E0);
            // (the member's address tested: only a null object at -0x258 fails)
            if (reinterpret_cast<uintptr_t>(obj) + 0x258 != 0 && t->unknownF4 != 0 && !CT_Fn5F48D0(obj + 0x258, t->unknownF4, 1, 0)) {
                CT_Fn5F4660(obj + 0x258, t->unknownF4);
            }
            TVector off;
            TVector* o = CT_Sub(&off, &EntityPos(ped), &at);
            float away = std::sqrt(std::fabs(o->y * o->y + o->x * o->x + o->z * o->z));
            if (away > 1.0f) {
                TVector put = at;
                uint8_t found = 0;
                float ground = CT_FindGroundZ(at.x, at.y, float(at.z + 2.0), &found, 1);
                if (found) {
                    put.z = ground;
                }
                CT_Ped_Teleport(ped, put.x, put.y, put.z);
            }
            if (*reinterpret_cast<float*>(ct_population + 0x7830) < dist) {
                if (t->ped != -1) {
                    CT_DeletePedHandle(t->ped, 2);
                }
                t->ped = -1;
                return nullptr;
            }
            return ped;
        }
    }
    uint8_t* pop = ct_population;
    float dz = std::fabs(d.z);
    if (!(*reinterpret_cast<float*>(pop + 0x7870) > dz && *reinterpret_cast<float*>(pop + 0x782C) < dist &&
          *reinterpret_cast<float*>(pop + 0x7830) > dist)) {
        if (*reinterpret_cast<float*>(pop + 0x782C) <= dist) {
            return ped;
        }
        if (CT_Camera_Fn4F12F0(ct_theCamera) != 2) {
            return ped;
        }
    }
    bool waiting = false;
    if (CT_GetModelInfo(t->model)[4] == 2 || t->model == -1) {
        waiting = true;
    } else if (!CT_IsModelLoaded(t->model)) {
        CT_RequestModel(t->model, 0x10);
        waiting = true;
    }
    if (t->weapon != -1) {
        if (CT_GetModelInfo(t->weapon)[4] == 2) {
            return nullptr;
        }
        if (!CT_IsModelLoaded(t->weapon)) {
            CT_RequestModel(t->weapon, 0x10);
            return nullptr;
        }
    }
    if (waiting) {
        return nullptr;
    }
    t->ped = CT_CreatePed(0, t->model, t->pos.x, t->pos.y, t->pos.z, t->heading, 1);
    ped = CT_GetPedFromHandle(t->ped, 2);
    *reinterpret_cast<int32_t*>(ped + 0x138C) = 1;
    *reinterpret_cast<int32_t*>(ped + 0x1360) = 1;
    if (t->weapon != -1) {
        void* inventory = *reinterpret_cast<re::Ptr32<void>*>(ped + 0x1C4);
        if (CT_Inventory_Find(inventory, t->weapon) == 0) {
            CT_Inventory_Give(*reinterpret_cast<re::Ptr32<void>*>(ped + 0x1C4), t->weapon, 1, 1, 0);
            *reinterpret_cast<int32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x2E0) + 0x18) = t->weapon;
        }
    }
    // the ped's pool reference (slot << 8 | its flag byte)
    re::Ptr32<uint8_t>* pedPool = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(0x00C0F5F0);
    int32_t slot = int32_t((ped - pedPool[0]) / *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pedPool) + 0xC));
    int32_t ref = pedPool[1][slot] + (slot << 8);
    void* radar = ct_radar;
    CT_Radar_AddEntityFlag(radar, 6, ref);
    CT_Radar_AddEntityFlag(radar, 2, ref);
    CT_Radar_AddEntityFlag(radar, 0xD, ref);
    return ped;
}

// 0x424DD0 (thiscall): every frame: the blips (hidden for missions flagged 0x40, or
// unavailable), the peds, the nearest trigger the player is inside of (and 0x5FB8B0
// allows); then talking to it when the player can: its cost asked (and the paid or
// can't-pay text shown), the conversation started with its names
void __fastcall Update(CCharTriggers* self, re::Edx) {
    float nearest = 1000.0f;
    self->talking = 0;
    self->costShown = 0;
    void* radar = ct_radar;
    int32_t best = -1;
    for (uint32_t i = 0; i < self->list.count; i++) {
        CharTrigger& t = self->list.entries[i];
        if (t.blip != -1 && t.mission >= 0 && (CT_MissionMgr_GetTaskFlags(ct_missionMgr, t.mission)[4] & 0x40)) {
            CT_Radar_RemoveBlip(radar, t.blip);
            t.blip = -1;
        }
        if (t.blip == -1 && (t.mission < 0 || !(CT_MissionMgr_GetTaskFlags(ct_missionMgr, t.mission)[4] & 0x40))) {
            CT_AddBlip(&t);
        }
        if (!CT_MissionMgr_IsAvailable(ct_missionMgr, t.mission, t.missionArg)) {
            if (t.ped != -1) {
                CT_DeletePedHandle(t.ped, 2);
                t.ped = -1;
            }
            CT_Radar_SetBlipVisible(radar, t.blip, 0);
            continue;
        }
        TVector& p = EntityPos(ct_playerPed);
        float dx = p.x - t.pos.x;
        float dy = EntityPos(ct_playerPed).y - t.pos.y;
        float dist = std::sqrt(std::fabs(dx * dx + dy * dy));
        uint8_t* ped = CT_UpdatePed(self, i);
        if (ped && dist < t.radius && dist < nearest) {
            CT_Radar_SetBlipVisible(radar, t.blip, 0);
            int32_t value;
            CT_Ped_Fn475380(ped, &value, *reinterpret_cast<int32_t*>(ct_playerPed + 0x1310));
            if (CT_Fn5FB8B0(float(value), float(t.unknown9C), t.unknownA0)) {
                nearest = dist;
                best = int32_t(i);
            }
            continue;
        }
        if (CT_MissionMgr_IsAvailable(ct_missionMgr, t.mission, t.missionArg)) {
            CT_Radar_SetBlipVisible(radar, t.blip, 1);
        }
    }
    uint8_t* playerObj = *reinterpret_cast<re::Ptr32<uint8_t>*>(ct_playerPed + 0x2E0);
    if (CT_Button_Fn5F4800(playerObj + 0x258, ct_talkButton, 1, 1)) {
        return;
    }
    if (best == -1) {
        return;
    }
    CharTrigger& t = self->list.entries[best];
    uint8_t* ped = CT_GetPedFromHandle(t.ped, 2);
    if (ct_inCutscene || ct_widescreen != 0 || !ped) {
        return;
    }
    if (CT_Ped_Fn473FD0(ped) || CT_Ped_Fn4CE690(ct_playerPed)) {
        return;
    }
    uint8_t canTalk = CT_Ai_CanTalkTo(ct_playerPed + 0xA64, ped);
    if (canTalk == 1) {
        if (t.hasCost) {
            self->costShown = canTalk;
            uint8_t canPay = CT_TryPay(self, ped, t.cost, 0);
            uint8_t* colours = ct_hudColours;
            uint8_t rgba[4] = {colours[0x18], colours[0x1C], colours[0x20], colours[0x24]};
            CT_Fn688E60();
            CT_SetTextColour(rgba);
            if (!canPay) {
                CT_PrintHelp(t.cantPayText, 0.1f);
                if (!self->talking) {
                    return;
                }
            } else {
                CT_PrintHelp(t.paidText, 0.1f);
                if (!self->talking) {
                    self->talking = 1;
                }
            }
        } else {
            self->talking = 1;
        }
    } else if (!self->talking) {
        return;
    }
    if (!CT_Pad_Fn50DF50(ct_pad) || t.ped == -1) {
        return;
    }
    if (!(CT_Fn738840(9, 0) > 0.0f)) {
        return;
    }
    if (t.hasCost) {
        if (ped[0x1E27] != 0 || !CT_TryPay(self, ped, t.cost, 1)) {
            return;
        }
    }
    CT_Conv_Fn683EB0(ct_conversation, t.unknownEC);
    CT_Conv_Start(ct_conversation, ct_playerPed, ped, t.name2, t.name1);
}

CHARTRIG_REVERSED("CharTriggers::TryPay", 0x00424280, &TryPay);
CHARTRIG_REVERSED("CharTriggers::ShowTalkPrompt", 0x00424340, &ShowTalkPrompt);
CHARTRIG_REVERSED("CharTriggers::ClearList", 0x00424380, &ClearList);
CHARTRIG_REVERSED("CharTriggers::RemoveAt", 0x004243C0, &RemoveAt);
CHARTRIG_REVERSED("CRGBA::FromUint", 0x00424420, &ColourFromUint);
CHARTRIG_REVERSED("CRGBA::Copy", 0x00424450, &ColourCopy);
CHARTRIG_REVERSED("CharTriggers::SetTextColour", 0x00424480, &SetTextColour);
CHARTRIG_REVERSED("CharTriggers::Clear", 0x004244B0, &Clear);
CHARTRIG_REVERSED("CharTriggers::Remove", 0x00424530, &Remove);
CHARTRIG_REVERSED("CharTriggers::IsTriggerPed", 0x004245E0, &IsTriggerPed);
CHARTRIG_REVERSED("CharTrigger::AddBlip", 0x00424640, &AddBlip);
CHARTRIG_REVERSED("CVector2D::Magnitude", 0x004246B0, &Vector2DMagnitude);
CHARTRIG_REVERSED("CharTriggers::Add", 0x004246E0, &Add);
CHARTRIG_REVERSED("CharTriggers::UpdatePed", 0x00424940, &UpdatePed);
CHARTRIG_REVERSED("CharTriggers::Update", 0x00424DD0, &Update);

} // namespace CharTriggers
