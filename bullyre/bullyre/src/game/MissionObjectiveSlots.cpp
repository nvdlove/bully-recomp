// CMissionObjectiveMgr (vtable 0x941DEC): ten CMissionObjective slots (0x54 bytes each,
// vtable 0x941DE4) kept in two linked lists (the shown ones and the free ones) through
// each slot's "next" byte, and a reminder message shown again after a while.
// 0x6AE450..0x6AE6F0 (the rest is chunk 38's, 0x6AE020..0x6AE410).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct CMissionObjective {
    re::Ptr32<void> vtable;        // +0 0x941DE4
    char text[0x18];     // +4 the objective's text key
    uint8_t unknown1C;   // +0x1C
    uint8_t unknown1D;   // +0x1D
    uint8_t unknown1E;   // +0x1E
    uint8_t pad1F;
    uint8_t params[0x30];  // +0x20 four 0xC-byte params (ctor 0x6ADF00)
    int8_t next;         // +0x50 the next slot of the list this one is on (-1: none)
    uint8_t pad51[3];
};
static_assert(offsetof(CMissionObjective, params) == 0x20);
static_assert(sizeof(CMissionObjective) == 0x54);

struct CMissionObjectiveMgr {
    re::Ptr32<void> vtable;                    // +0 0x941DEC
    CMissionObjective slots[10];     // +4
    int8_t firstShown;               // +0x34C
    int8_t firstFree;                // +0x34D
    uint8_t shownCount;              // +0x34E
    char reminderKey[0x19];          // +0x34F
    float reminderTime;              // +0x368 how long the reminder shows (< 0: never)
    uint32_t reminderAt;             // +0x36C when it shows next
};
static_assert(offsetof(CMissionObjectiveMgr, firstShown) == 0x34C);
static_assert(offsetof(CMissionObjectiveMgr, reminderKey) == 0x34F);
static_assert(offsetof(CMissionObjectiveMgr, reminderTime) == 0x368);
static_assert(offsetof(CMissionObjectiveMgr, reminderAt) == 0x36C);

// (the game reaches slot i's "next" byte as manager + (i + 1) * 0x54)
inline int8_t& NextOf(CMissionObjectiveMgr* self, int slot) {
    return self->slots[slot].next;
}

inline void* const kObjectiveVtable = reinterpret_cast<void*>(0x00941DE4);
inline void* const kMgrVtable = reinterpret_cast<void*>(0x00941DEC);
inline void* const kParamCtor = reinterpret_cast<void*>(0x006ADF00);
constexpr uint32_t kReminderDelay = 180000;  // 3 minutes

GAME_VAR(uint32_t, mos_timeInMs, 0x00C1A9B4);

inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void* ctor);
GAME_FN(void, __cdecl, FlushTextParams, 0x005D5510, int help);
GAME_FN(void, __cdecl, BigText_Show, 0x005142A0, const char* key, float time);
// chunk 38's
GAME_FN(void, __thiscall, Objective_Destruct, 0x006AE1E0, CMissionObjective* self);
GAME_FN(void, __thiscall, ObjectiveMgr_Clear, 0x006AE020, CMissionObjectiveMgr* self);
GAME_FN(void, __thiscall, ObjectiveMgr_Destruct, 0x006AE220, CMissionObjectiveMgr* self);

// 0x6AE450: sets slot's byte +0x1D (the game indexes byte +0x21 of the manager)
void __fastcall SetSlotFlag(CMissionObjectiveMgr* self, re::Edx, int slot, int value) {
    self->slots[uint8_t(slot)].unknown1D = uint8_t(value);
    self->reminderAt = mos_timeInMs + kReminderDelay;
}

// 0x6AE480: clears a slot and moves it from the shown list to the free one
void __fastcall RemoveSlot(CMissionObjectiveMgr* self, re::Edx, int slotArg) {
    int slot = uint8_t(slotArg);
    CMissionObjective& o = self->slots[slot];
    Strncpy(o.text, "", 0x18);
    o.unknown1C = 0;
    o.unknown1D = 0;
    o.unknown1E = 0;
    if (self->shownCount != 0) {
        int8_t prev = self->firstShown;
        if (slot == prev) {
            self->firstShown = NextOf(self, prev);
            NextOf(self, prev) = self->firstFree;
            self->firstFree = prev;
            self->shownCount--;
        } else {
            if (prev >= 0) {
                for (;;) {
                    int8_t cur = NextOf(self, prev);
                    if (cur == slot) {
                        NextOf(self, prev) = NextOf(self, cur);
                        NextOf(self, cur) = self->firstFree;
                        self->firstFree = cur;
                        break;
                    }
                    prev = cur;
                    if (prev < 0) {
                        break;
                    }
                }
            }
            self->shownCount--;  // (even when the slot wasn't found)
        }
    }
    self->reminderAt = mos_timeInMs + kReminderDelay;
}

// 0x6AE580: when the reminder shows next (negative: never)
void __fastcall SetReminderTime(CMissionObjectiveMgr* self, re::Edx, float seconds) {
    if (seconds < 0.0f) {
        self->reminderTime = -1.0f;
        return;
    }
    // (x87 at 24 bits: the product is rounded to float, then truncated)
    float ms = float(double(seconds) * -1000.0);
    self->reminderAt = mos_timeInMs - uint32_t(int64_t(ms));
}

// 0x6AE5E0: shows the reminder again when it's due
void __fastcall UpdateReminder(CMissionObjectiveMgr* self, re::Edx) {
    if (0.0f < self->reminderTime && self->reminderAt < mos_timeInMs) {
        FlushTextParams(0);
        BigText_Show(self->reminderKey, self->reminderTime);
        self->reminderAt = mos_timeInMs + kReminderDelay;
    }
}

// 0x6AE640: CMissionObjective's deleting destructor
CMissionObjective* __fastcall Objective_DeletingDestruct(CMissionObjective* self, re::Edx, int flags) {
    Objective_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x6AE660
CMissionObjectiveMgr* __fastcall ObjectiveMgr_Construct(CMissionObjectiveMgr* self, re::Edx) {
    self->vtable = kMgrVtable;
    for (CMissionObjective& o : self->slots) {
        o.vtable = kObjectiveVtable;
        o.unknown1C = 0;
        o.unknown1D = 0;
        o.unknown1E = 1;
        VectorConstruct(o.params, 0xC, 4, kParamCtor);
        uint32_t* text = reinterpret_cast<uint32_t*>(o.text);
        for (int i = 0; i < 6; i++) {
            text[i] = 0;
        }
    }
    ObjectiveMgr_Clear(self);
    return self;
}

// 0x6AE6D0: the manager's deleting destructor
CMissionObjectiveMgr* __fastcall ObjectiveMgr_DeletingDestruct(CMissionObjectiveMgr* self, re::Edx, int flags) {
    ObjectiveMgr_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

} // namespace

#define MOSLOTS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

MOSLOTS_REVERSED("CMissionObjectiveMgr::SetSlotFlag", 0x006AE450, &SetSlotFlag);
MOSLOTS_REVERSED("CMissionObjectiveMgr::RemoveSlot", 0x006AE480, &RemoveSlot);
MOSLOTS_REVERSED("CMissionObjectiveMgr::SetReminderTime", 0x006AE580, &SetReminderTime);
MOSLOTS_REVERSED("CMissionObjectiveMgr::UpdateReminder", 0x006AE5E0, &UpdateReminder);
MOSLOTS_REVERSED("CMissionObjective::DeletingDestructor", 0x006AE640, &Objective_DeletingDestruct);
MOSLOTS_REVERSED("CMissionObjectiveMgr::CMissionObjectiveMgr", 0x006AE660, &ObjectiveMgr_Construct);
MOSLOTS_REVERSED("CMissionObjectiveMgr::DeletingDestructor", 0x006AE6D0, &ObjectiveMgr_DeletingDestruct);
