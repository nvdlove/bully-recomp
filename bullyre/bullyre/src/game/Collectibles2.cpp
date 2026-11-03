// CCollectiblesMgr, second part: its destructor, loading config\dat\colctbls.dat, a
// collectible taken (the stats, the save event), the radar blips by kind, and Init.
// 0x4288C0..0x428DA0. (The first part, and the manager's layout: Collectibles.cpp.)

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
#define COLLECT2_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

// A collectible (see Collectibles.cpp)
struct Collectible2 {
    int32_t type;       // +0 its kind (0..3 have blips)
    int32_t unknown4;   // +4 (its name's hash)
    int32_t unknown8;   // +8
    float pos[3];       // +0xC
};
static_assert(sizeof(Collectible2) == 0x18);

struct CollectiblesMgr2 {
    re::Ptr32<re::Ptr32<void>> vtable;                  // +0 (0x901088)
    uint8_t state[200][2];          // +4 byte 0: collected
    Collectible2 entries[200];      // +0x194
    int16_t countByType[4];         // +0x1454 how many there are of each kind
    uint32_t unknown145C;           // +0x145C
    int16_t unknown1460;            // +0x1460
    int16_t count;                  // +0x1462
    int32_t blips[200];             // +0x1464 their radar blips
    uint8_t unknown1784[0x708];     // +0x1784 the position tables
    uint8_t saveEvents[4];          // +0x1E8C (0x5D9C50 resets it)
};
static_assert(offsetof(CollectiblesMgr2, entries) == 0x194);
static_assert(offsetof(CollectiblesMgr2, countByType) == 0x1454);
static_assert(offsetof(CollectiblesMgr2, unknown145C) == 0x145C);
static_assert(offsetof(CollectiblesMgr2, count) == 0x1462);
static_assert(offsetof(CollectiblesMgr2, blips) == 0x1464);
static_assert(offsetof(CollectiblesMgr2, saveEvents) == 0x1E8C);

// The file's lines (0x426610 loads the file, 0x42B980 hands out the next line)
struct LineFile {
    re::Ptr32<char> data;     // +0 (freed after)
    int32_t left;   // +4 (lines remain while +4 != +8)
    int32_t end;    // +8
};
static_assert(sizeof(LineFile) == 0xC);

inline re::Ptr32<void>* const cm_vtable = reinterpret_cast<re::Ptr32<void>*>(0x00901088);
inline CollectiblesMgr2* const cm_theMgr = reinterpret_cast<CollectiblesMgr2*>(0x00BCEB50);
GAME_VAR(uint8_t*, cm_weaponHud, 0x00C674F0);        // (+0x15D)
GAME_VAR(void*, cm_radar, 0x00C674CC);               // HUDRadar
GAME_ARRAY(void*, cm_statNames, 0x009E51B0);         // by kind

GAME_FN(void, __thiscall, CM_SaveEvents_Reset, 0x005D9C50, void* events);
GAME_FN(void, __cdecl, CM_Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, CM_LineFile_Load, 0x00426610, LineFile* file, const char* name);
GAME_FN(char*, __cdecl, CM_LineFile_NextLine, 0x0042B980, LineFile* file);
template <typename... A>
inline int __cdecl CM_sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline int __cdecl CM_stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(uint32_t, __cdecl, CM_HashName, 0x005DAEE0, const char* name);
GAME_FN(void, __thiscall, CM_Add, 0x004261D0, CollectiblesMgr2* self, uint32_t group, const char* name);
GAME_FN(int16_t, __thiscall, CM_GetNumCollected, 0x00426440, CollectiblesMgr2* self, int type);
GAME_FN(void, __thiscall, CM_SetCollected, 0x00428A10, CollectiblesMgr2* self, int index, int collected);
GAME_FN(void, __thiscall, CM_Load, 0x004288F0, CollectiblesMgr2* self);
// the save events
GAME_FN(uint8_t, __thiscall, CM_Events_IsOn, 0x005D80E0, void* events);
GAME_FN(void*, __thiscall, CM_Events_New, 0x005D9C20, void* events);
GAME_FN(int32_t, __thiscall, CM_Events_Fn5D80D0, 0x005D80D0, void* events);
GAME_FN(void, __cdecl, CM_Event_Begin, 0x005D8270, void* event);
GAME_FN(void, __cdecl, CM_Event_SetId, 0x005D8670, void* event, int32_t id);
GAME_FN(void, __cdecl, CM_Event_AddInt, 0x005D85C0, void* event, int32_t value);
GAME_FN(void, __cdecl, CM_Event_Send, 0x005D82A0, void* event, int kind, int arg);
// the HUD
GAME_FN(void, __cdecl, CM_Hud_PushNumber, 0x005142E0, float value);
GAME_FN(void, __cdecl, CM_Hud_ShowMessage, 0x00514340, void* text, float seconds);
GAME_FN(int32_t, __thiscall, CM_Radar_AddBlip, 0x0054A700, void* radar, int32_t a, int32_t b, uint32_t colour, int arg, int32_t sprite, int arg2);
GAME_FN(void, __thiscall, CM_Radar_SetBlipScale, 0x00548BF0, void* radar, int32_t blip, int scale);
GAME_FN(void, __thiscall, CM_Radar_RemoveBlip, 0x0054A940, void* radar, int32_t blip);
GAME_FN(void, __thiscall, CM_Radar_ShowKind, 0x00548E30, void* radar, int kind, int show);
inline void* __cdecl CM_Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }

} // namespace

namespace CCollectiblesMgr {

// 0x4288C0 vf0 (thiscall): the destructor (flags 1: and freed)
CollectiblesMgr2* __fastcall Destroy(CollectiblesMgr2* self, re::Edx, uint8_t flags) {
    self->vtable = cm_vtable;
    CM_SaveEvents_Reset(self->saveEvents);
    if (flags & 1) {
        CM_Free(self);
    }
    return self;
}

// 0x4288F0 (thiscall): config\dat\colctbls.dat: a group's name, then its collectibles'
// names up to "end"; lines empty or starting with '#' skipped
void __fastcall Load(CollectiblesMgr2* self, re::Edx) {
    LineFile file = {nullptr, 0, 0};
    char group[64];
    char name[64];
    CM_LineFile_Load(&file, "config\\dat\\colctbls.dat");
    while (file.left != file.end) {
        char* line = CM_LineFile_NextLine(&file);
        if (!line) {
            break;
        }
        if (*line == '\0' || *line == '#') {
            continue;
        }
        CM_sscanf(line, "%s", group);
        uint32_t hash = CM_HashName(group);
        while (file.left != file.end) {
            line = CM_LineFile_NextLine(&file);
            if (!line || CM_stricmp(line, "end") == 0) {
                break;
            }
            if (CM_sscanf(line, "%s", name) > 0) {
                CM_Add(self, hash, name);
            }
        }
    }
    if (file.data) {
        CM_Free(file.data);
    }
}

// 0x428A10 (thiscall): a collectible taken (or not): all of kind 2 taken sets the
// weapon HUD's +0x15D; a save event with its kind, count, total and ids when events are on
void __fastcall SetCollected(CollectiblesMgr2* self, re::Edx, int index, int collected) {
    int i = int16_t(index);
    self->state[i][0] = uint8_t(collected);
    if (!re::Flag(collected)) {
        return;
    }
    Collectible2* entry = &self->entries[i];
    if (entry->type == 2 && CM_GetNumCollected(self, 2) >= self->countByType[2]) {
        cm_weaponHud[0x15D] = 1;
    }
    if (!CM_Events_IsOn(self->saveEvents)) {
        return;
    }
    void* event = CM_Events_New(self->saveEvents);
    CM_Event_Begin(event);
    CM_Event_SetId(event, CM_Events_Fn5D80D0(self->saveEvents));
    CM_Event_AddInt(event, entry->type);
    CM_Event_AddInt(event, CM_GetNumCollected(self, entry->type));
    CM_Event_AddInt(event, self->countByType[entry->type]);
    CM_Event_AddInt(event, entry->unknown4);
    CM_Event_AddInt(event, entry->unknown8);
    CM_Event_AddInt(event, i);
    CM_Event_Send(event, 6, 0);
}

// 0x428B00 (thiscall): the "n of m" message for a collectible's kind (2 seconds)
void __fastcall ShowCollectedMessage(CollectiblesMgr2* self, re::Edx, int index) {
    Collectible2* entry = &self->entries[int16_t(index)];
    int16_t total = self->countByType[entry->type];
    CM_Hud_PushNumber(float(int(CM_GetNumCollected(self, entry->type))));
    CM_Hud_PushNumber(float(int(total)));
    CM_Hud_ShowMessage(cm_statNames[entry->type], 2.0f);
}

// 0x428B70 (thiscall): the radar blips of a kind's collectibles not yet taken, on or
// off, and the kind's radar legend
void __fastcall SetBlips(CollectiblesMgr2* self, re::Edx, int type, int enable) {
    void* radar = cm_radar;
    // (a kind other than 0..3 would use whatever was on the stack for the sprite)
    int32_t sprite = 0;
    for (int i = 0; i < self->count; i++) {
        Collectible2* entry = &self->entries[i];
        if (entry->type != type || self->state[int16_t(i)][0] != 0) {
            continue;
        }
        if (uint8_t(enable) == 1) {
            if (type == 2) {
                sprite = 0x24;
            } else if (type == 3) {
                sprite = 0x25;
            } else if (type == 1) {
                sprite = 0x23;
            } else if (type == 0) {
                sprite = 0x26;
            }
            int32_t blip = CM_Radar_AddBlip(radar, entry->unknown8, entry->unknown4, 0, 1, sprite, 0);
            self->blips[i] = blip;
            CM_Radar_SetBlipScale(radar, blip, 5);
        } else if (uint8_t(enable) == 0 && self->blips[i] != 0) {
            CM_Radar_RemoveBlip(radar, self->blips[i]);
        }
    }
    if (type == 2) {
        CM_Radar_ShowKind(radar, 6, enable);
    } else if (type == 3) {
        CM_Radar_ShowKind(radar, 7, enable);
    } else if (type == 1) {
        CM_Radar_ShowKind(radar, 5, enable);
    } else if (type == 0) {
        CM_Radar_ShowKind(radar, 8, enable);
    }
}

// 0x428CF0 (thiscall): every collectible of a kind taken (or not). (The game's
// SetCollected goes to the manager at 0xBCEB50, whichever this is)
void __fastcall SetAllAsCollected(CollectiblesMgr2* self, re::Edx, int type, int collected) {
    for (int i = 0; i < self->count; i++) {
        if (self->entries[i].type == type) {
            CM_SetCollected(cm_theMgr, i, collected);
        }
    }
}

// 0x428D40 (thiscall): everything cleared, the file loaded, the save events reset
void __fastcall Init(CollectiblesMgr2* self, re::Edx) {
    CM_Memset(self->state, 0, 400);
    CM_Memset(self->entries, 0, 0x12C0);
    *reinterpret_cast<uint32_t*>(&self->countByType[0]) = 0;
    *reinterpret_cast<uint32_t*>(&self->countByType[2]) = 0;
    self->unknown145C = 0;
    self->unknown1460 = 0;
    self->count = 0;
    CM_Load(self);
    CM_SaveEvents_Reset(self->saveEvents);
}

COLLECT2_REVERSED("CCollectiblesMgr::Destroy", 0x004288C0, &Destroy);
COLLECT2_REVERSED("CCollectiblesMgr::LoadDat", 0x004288F0, &Load);
COLLECT2_REVERSED("CCollectiblesMgr::SetCollected", 0x00428A10, &SetCollected);
COLLECT2_REVERSED("CCollectiblesMgr::ShowCollectedMessage", 0x00428B00, &ShowCollectedMessage);
COLLECT2_REVERSED("CCollectiblesMgr::SetBlips", 0x00428B70, &SetBlips);
COLLECT2_REVERSED("CCollectiblesMgr::SetAllAsCollected", 0x00428CF0, &SetAllAsCollected);
COLLECT2_REVERSED("CCollectiblesMgr::Init", 0x00428D40, &Init);

} // namespace CCollectiblesMgr
