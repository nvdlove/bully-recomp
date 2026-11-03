// The bulletin board: up to 40 entries, each a mission (by index; -1 for none) and a
// text's hash, shown while active; whether any is active kept in a global flag; saved
// as a byte per entry ("BulletinBoardSaveData").

#include <cstdint>

#include "reversed.h"

namespace c01board {

struct BoardEntry {
    int32_t mission;    // +0 (-1: none)
    uint32_t text;      // +4 the text's hash
    uint8_t active;     // +8
    uint8_t unknown9[3];
};
static_assert(sizeof(BoardEntry) == 0xC);

struct BulletinBoard {
    uint32_t unknown0;
    int32_t count;              // +4
    BoardEntry entries[40];     // +8
};

// What 0x418870 saves (and 0x4188C0 checks and loads)
struct BoardSaveData {
    int32_t version;     // +0 1
    int32_t size0;       // +4 0x78
    int32_t size1;       // +8 0x28
    uint8_t active[40];  // +0xC
    int32_t count;       // +0x34
    uint8_t block[0x10]; // +0x38 ("BulletinBoardSaveData")
};
static_assert(sizeof(BoardSaveData) == 0x48);

GAME_VAR(uint8_t, ms_bBoardHasActive, 0x020C8B9D);
GAME_VAR(uint32_t, ms_frameCounter, 0x00C1A99C);
inline void* const theMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline const char* const emptyString = reinterpret_cast<const char*>(0x00900434);

GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetStats, 0x006AA7C0, void* self, int mission);
GAME_FN(int32_t, __thiscall, MissionMgr_FindMission, 0x006A9FD0, void* self, void* key);
GAME_FN(void*, __thiscall, NameKey_Construct, 0x005FA910, void* self, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* self);
GAME_FN(uint8_t, __cdecl, Save_CheckValue, 0x00733320, int, int, int value, int expected);
GAME_FN(uint8_t, __cdecl, Save_CheckBlock, 0x00733620, const char* name, void* data, int size);
GAME_FN(uint8_t, __thiscall, Board_HasActive, 0x00418820, BulletinBoard* self);

// a mission that's there to show: none, or one with stats (+2)
bool IsAvailable(const BoardEntry& entry) {
    return entry.mission < 0 || *reinterpret_cast<uint16_t*>(MissionMgr_GetStats(theMissionMgr, entry.mission) + 2) != 0;
}

// 0x4186E0 (thiscall): a name's hash (0 for none) into the first field
void __fastcall SetNameHash(uint32_t* self, re::Edx, const char* name) {
    if (name) {
        *self = HashName(name);
        return;
    }
    *self = 0;
}

// 0x418710 (thiscall): the entries to show, the inactive ones first; `firstActive`
// the first active's place (the last's without any)
void __fastcall ListEntries(BulletinBoard* self, re::Edx, int32_t* out, int32_t* count, int32_t* firstActive) {
    int32_t inactive[40];
    int32_t active[40];
    int numInactive = 0;
    int numActive = 0;
    *count = 0;
    for (int i = 0; i < self->count; i++) {
        if (!IsAvailable(self->entries[i])) {
            continue;
        }
        if (self->entries[i].active) {
            active[numActive++] = i;
        } else {
            inactive[numInactive++] = i;
        }
    }
    for (int i = 0; i < numInactive; i++) {
        out[*count] = inactive[i];
        (*count)++;
    }
    for (int i = 0; i < numActive; i++) {
        out[*count] = active[i];
        (*count)++;
    }
    if (numActive > 0) {
        *firstActive = numInactive;
        return;
    }
    *firstActive = *count - 1;
}

// 0x418810 (thiscall): an entry's text hash
uint32_t __fastcall GetText(BulletinBoard* self, re::Edx, int index) {
    return self->entries[index].text;
}

// 0x418820 (thiscall): any entry available and active
uint8_t __fastcall HasActive(BulletinBoard* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        if (IsAvailable(self->entries[i]) && self->entries[i].active) {
            return 1;
        }
    }
    return 0;
}

// 0x418870 (thiscall): the save data (the actives, the count)
void __fastcall Save(BulletinBoard* self, re::Edx, BoardSaveData* data) {
    data->version = 1;
    data->size0 = 0x78;
    data->size1 = 0x28;
    for (int i = 0; i < self->count; i++) {
        data->active[i] = self->entries[i].active;
    }
    data->count = self->count;
}

// 0x4188C0 (thiscall): the save data checked and the actives loaded
void __fastcall Load(BulletinBoard* self, re::Edx, BoardSaveData* data) {
    if (!Save_CheckValue(0, 0, data->version, 1) || !Save_CheckValue(0, 0, data->size0, 0x78) ||
        !Save_CheckValue(0, 0, data->size1, 0x28) || !Save_CheckBlock("BulletinBoardSaveData", data->block, 0x10)) {
        return;
    }
    for (int i = 0; i < data->count; i++) {
        self->entries[i].active = data->active[i];
    }
}

// 0x418950 (thiscall): an entry added (the mission by name; "" for none), active;
// its index
int __fastcall Add(BulletinBoard* self, re::Edx, const char* mission, const char* text) {
    int32_t index = -1;
    if (HashName(mission) != HashName(emptyString)) {
        uint8_t key[8];
        index = MissionMgr_FindMission(theMissionMgr, NameKey_Construct(key, mission));
        NameKey_Destruct(key);
    }
    uint32_t hash = text ? HashName(text) : 0;
    BoardEntry& entry = self->entries[self->count++];
    entry.mission = index;
    entry.text = hash;
    entry.active = 1;
    ms_bBoardHasActive = Board_HasActive(self);
    return self->count - 1;
}

// 0x4189E0 (thiscall): an entry marked read (flag set: inactive)
void __fastcall SetRead(BulletinBoard* self, re::Edx, int index, int read) {
    self->entries[index].active = !re::Flag(read);
    ms_bBoardHasActive = Board_HasActive(self);
}

// 0x418A00 (thiscall): the flag refreshed every 64 frames
void __fastcall Update(BulletinBoard* self, re::Edx) {
    if ((ms_frameCounter & 0x3F) == 0) {
        ms_bBoardHasActive = Board_HasActive(self);
    }
}

// 0x418A20: a byte of the entity (+0x2ED) plus 5
float __cdecl GetByte2EDPlus5(const uint8_t* entity) {
    return static_cast<float>(entity[0x2ED] + 5.0);
}

} // namespace c01board

// ---- registration ----------------------------------------------------------------------

#define BOARD_REVERSED_C01(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01board {
BOARD_REVERSED_C01("CHashedName::Set", 0x004186E0, &SetNameHash);
BOARD_REVERSED_C01("BulletinBoard::ListEntries", 0x00418710, &ListEntries);
BOARD_REVERSED_C01("BulletinBoard::GetText", 0x00418810, &GetText);
BOARD_REVERSED_C01("BulletinBoard::HasActive", 0x00418820, &HasActive);
BOARD_REVERSED_C01("BulletinBoard::Save", 0x00418870, &Save);
BOARD_REVERSED_C01("BulletinBoard::Load", 0x004188C0, &Load);
BOARD_REVERSED_C01("BulletinBoard::Add", 0x00418950, &Add);
BOARD_REVERSED_C01("BulletinBoard::SetRead", 0x004189E0, &SetRead);
BOARD_REVERSED_C01("BulletinBoard::Update", 0x00418A00, &Update);
BOARD_REVERSED_C01("CEntity::GetByte2EDPlus5", 0x00418A20, &GetByte2EDPlus5);
} // namespace c01board
