// The mission objectives (CMissionObjectiveMgr at 0x20C4420): ten objectives in a linked
// list, their text parameters, adding one, walking them. 0x6ADF00..0x6AE440 (the rest of
// the manager is MissionObjectiveSlots.cpp).
// Manager: +0 vtable 0x941DEC, +4 objectives[10] (0x54 each), +0x34C head (int8, -1 none),
// +0x34D first free, +0x34E count, +0x34F the newest's text key [0x18], +0x368 its
// banner time (4.0, -1 none), +0x36C the reminder time (ms), +0x370 the walk's cursor.
// Objective: +0 vtable 0x941DE4, +4 text key [0x18], +0x1C parameter count, +0x1D done,
// +0x1E shown, +0x20 params[4] (0xC: vtable 0x941DDC, +4 value, +8 type: 0 int, 1 float,
// 2 text key (malloced)), +0x50 next (int8).

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace msnobj {

namespace {

using re::At;

void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_VAR(uint32_t, g_timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t, g_skipFlag20C870A, 0x020C870A);

GAME_FN(void*, __cdecl, MO_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, MO_Free, 0x005EEAB0, void* p);
inline char* __cdecl MO_Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(void, __stdcall, MO_VectorDestruct, 0x004017C0, void* array, size_t size, int count, void* dtor);
GAME_FN(uint8_t, __cdecl, MO_TimePaused, 0x00705500);
GAME_FN(const uint16_t*, __thiscall, MO_TextGet, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, MO_FontAddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, MO_FontAddString, 0x006880F0, const uint16_t* string);
GAME_FN(void, __cdecl, MO_FontFormat, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(void, __thiscall, MO_ObjectiveSetParamText, 0x006ADF30, void* objective, int param, const char* text);

constexpr uint32_t kParamVtable = 0x00941DDC;
constexpr uint32_t kObjectiveVtable = 0x00941DE4;
constexpr uint32_t kManagerVtable = 0x00941DEC;
constexpr uint32_t kReminderDelay = 180000;  // (0x2BF20 ms)

uint8_t* Objective(uint8_t* self, int32_t index) {
    return self + 4 + index * 0x54;
}

// an objective's next link (addressed the game's way, (index + 1) * 0x54)
int8_t& Next(uint8_t* self, int32_t index) {
    return At<int8_t>(self, (index + 1) * 0x54);
}

// a parameter's destruction (its text freed)
void ParamRelease(uint8_t* param) {
    At<uint32_t>(param, 0) = kParamVtable;
    if (At<int32_t>(param, 8) == 2) {
        void* text = At<void*>(param, 4);
        if (text != nullptr) {
            MO_Free(text);
        }
    }
}

// the objective's parameter `param` (counted from 1)
uint8_t* Param(uint8_t* self, int objective, int param) {
    return self + (uint8_t(objective) * 7 + uint8_t(param) + 2) * 0xC;
}

} // namespace

// 0x6ADF00 (not in the function list): a parameter's constructor (no type)
uint8_t* __fastcall Param_Construct(uint8_t* self) {
    At<uint32_t>(self, 0) = kParamVtable;
    At<int32_t>(self, 8) = -1;
    return self;
}

// 0x6ADF10 (not in the function list): a parameter's destructor (the objectives' vector
// destructor's element one)
void __fastcall Param_Destruct(uint8_t* self) {
    ParamRelease(self);
}

// 0x6ADF30: a text parameter (counted from 1) set: a malloced copy of the key; the copy
// is strlen bytes into strlen + 1, unterminated (and a previous one isn't freed)
void __fastcall Objective_SetParamText(uint8_t* self, re::Edx, int param, const char* text) {
    uint8_t* slot = self + 0x14 + uint8_t(param) * 0xC;
    At<int32_t>(slot, 8) = 2;
    char* copy = static_cast<char*>(MO_Malloc(uint32_t(std::strlen(text) + 1)));
    At<char*>(slot, 4) = copy;
    MO_Strncpy(copy, text, std::strlen(text));
}

// 0x6ADF90: an objective's text: its key's format filled with its parameters
void __fastcall Objective_GetText(uint8_t* self, re::Edx, uint16_t* out, int size) {
    for (int i = 0; i < self[0x1C]; ++i) {
        uint8_t* param = self + 0x20 + i * 0xC;
        int32_t type = At<int32_t>(param, 8);
        if (type == 1) {
            MO_FontAddNumber(At<float>(param, 4));
        } else if (type == 0) {
            MO_FontAddNumber(float(At<int32_t>(param, 4)));
        } else if (type == 2) {
            MO_FontAddString(MO_TextGet(kTheText, At<const char*>(param, 4)));
        }
    }
    MO_FontFormat(out, MO_TextGet(kTheText, reinterpret_cast<const char*>(self + 4)), size);
}

// 0x6AE020: Reset: no objectives, every one free and chained
void __fastcall Objectives_Reset(uint8_t* self) {
    At<uint8_t>(self, 0x34C) = 0xFF;
    At<uint8_t>(self, 0x34E) = 0;
    At<uint8_t>(self, 0x34D) = 0;
    uint8_t i = 0;
    do {
        At<uint8_t>(self, (i + 1) * 0x54) = uint8_t(i + 1);
        i = uint8_t(i + 1);
    } while (i < 9);
    At<uint8_t>(self, (i + 1) * 0x54) = 0xFF;
    At<uint32_t>(self, 0x36C) = 0;
    At<uint8_t>(self, 0x34F) = 0;
    At<float>(self, 0x368) = -1.0f;
    At<int32_t>(self, 0x370) = -1;
}

// 0x6AE080: an objective
uint8_t* __fastcall Objectives_Get(uint8_t* self, re::Edx, int32_t index) {
    return Objective(self, index);
}

// 0x6AE090: an objective is in the list
uint8_t __fastcall Objectives_Exists(uint8_t* self, re::Edx, uint32_t index) {
    if (At<uint8_t>(self, 0x34E) == 0 || index > 9) {
        return 0;
    }
    int32_t current = At<int8_t>(self, 0x34C);
    while (index != uint32_t(current)) {
        int8_t next = At<int8_t>(Objective(self, current), 0x50);
        if (next < 0) {
            return 0;
        }
        current = next;
    }
    return 1;
}

// 0x6AE0E0: the walk started at the head
void __fastcall Objectives_Rewind(uint8_t* self) {
    At<int32_t>(self, 0x370) = At<int8_t>(self, 0x34C);
}

// 0x6AE0F0: the walk moved to an objective
void __fastcall Objectives_SeekTo(uint8_t* self, re::Edx, int32_t index) {
    At<int32_t>(self, 0x370) = index;
}

// 0x6AE100 / 0x6AE150: the walk's next pending (done == 0) / done objective (its index
// through `out`), none: the walk ended (-1)
template <bool Done>
uint8_t* __fastcall Objectives_NextWhere(uint8_t* self, re::Edx, int32_t* out) {
    for (;;) {
        int32_t cursor = At<int32_t>(self, 0x370);
        if (cursor < 0 || At<uint8_t>(self, 0x34E) == 0) {
            At<int32_t>(self, 0x370) = -1;
            return nullptr;
        }
        if (out != nullptr) {
            *out = cursor;
        }
        cursor = At<int32_t>(self, 0x370);
        At<int32_t>(self, 0x370) = Next(self, cursor);
        uint8_t* objective = Objective(self, cursor);
        if ((At<uint8_t>(objective, 0x1D) != 0) == Done) {
            return objective;
        }
    }
}

// 0x6AE1A0 (vf0): a parameter's deleting destructor
uint8_t* __fastcall Param_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    ParamRelease(self);
    if (flags & 1) {
        MO_Free(self);
    }
    return self;
}

// 0x6AE1E0: an objective's destructor (its parameters, last first)
void __fastcall Objective_Destruct(uint8_t* self) {
    At<uint32_t>(self, 0) = kObjectiveVtable;
    for (int i = 3; i >= 0; --i) {
        ParamRelease(self + 0x20 + i * 0xC);
    }
}

// 0x6AE220: the manager's destructor (its objectives, last first)
void __fastcall Objectives_Destruct(uint8_t* self) {
    At<uint32_t>(self, 0) = kManagerVtable;
    for (int i = 9; i >= 0; --i) {
        uint8_t* objective = Objective(self, i);
        At<uint32_t>(objective, 0) = kObjectiveVtable;
        MO_VectorDestruct(objective + 0x20, 0xC, 4, reinterpret_cast<void*>(0x006ADF10));
    }
}

// 0x6AE260: Reset again (a lone jmp to 0x6AE020)
RE_UNHOOKED("CMissionObjectiveMgr::ResetJump", 0x006AE260);

// 0x6AE270: an objective added at the head from the free list (the banner shows its key
// for 4 s, the reminder waits 3 minutes); -1 when full or held by the paused clock
int32_t __fastcall Objectives_Add(uint8_t* self, re::Edx, const char* text, int paramCount) {
    if (MO_TimePaused() && g_skipFlag20C870A == 0) {
        return -1;
    }
    if (At<uint8_t>(self, 0x34E) >= 10) {
        return -1;
    }
    uint8_t* objective = Objective(self, At<int8_t>(self, 0x34D));
    MO_Strncpy(reinterpret_cast<char*>(objective + 4), text, 0x18);
    objective[0x1C] = uint8_t(paramCount);
    objective[0x1D] = 0;
    objective[0x1E] = 1;
    int8_t head = At<int8_t>(self, 0x34C);
    if (uint8_t(head) == 0xFF) {
        int8_t free = At<int8_t>(self, 0x34D);
        At<int8_t>(self, 0x34C) = free;
        At<int8_t>(self, 0x34D) = Next(self, free);
        Next(self, At<int8_t>(self, 0x34C)) = -1;
    } else {
        int8_t free = At<int8_t>(self, 0x34D);
        int8_t following = Next(self, free);
        Next(self, free) = head;
        At<int8_t>(self, 0x34C) = At<int8_t>(self, 0x34D);
        At<int8_t>(self, 0x34D) = following;
    }
    if (text != nullptr) {
        At<float>(self, 0x368) = 4.0f;
        MO_Strncpy(reinterpret_cast<char*>(self + 0x34F), text, 0x18);
    } else {
        At<float>(self, 0x368) = -1.0f;
    }
    At<uint8_t>(self, 0x34E) += 1;
    At<uint32_t>(self, 0x36C) = g_timeInMs + kReminderDelay;
    return At<int8_t>(self, 0x34C);
}

// 0x6AE390: an int parameter set (the reminder restarted)
void __fastcall Objectives_SetParamInt(uint8_t* self, re::Edx, int objective, int param, int32_t value) {
    uint8_t* slot = Param(self, objective, param);
    At<int32_t>(slot, 8) = 0;
    At<int32_t>(slot, 4) = value;
    At<uint32_t>(self, 0x36C) = g_timeInMs + kReminderDelay;
}

// 0x6AE3D0: a float parameter set
void __fastcall Objectives_SetParamFloat(uint8_t* self, re::Edx, int objective, int param, float value) {
    uint8_t* slot = Param(self, objective, param);
    At<int32_t>(slot, 8) = 1;
    At<float>(slot, 4) = value;
    At<uint32_t>(self, 0x36C) = g_timeInMs + kReminderDelay;
}

// 0x6AE410: a text parameter set
void __fastcall Objectives_SetParamText(uint8_t* self, re::Edx, int objective, int param, const char* text) {
    MO_ObjectiveSetParamText(Objective(self, uint8_t(objective)), param, text);
    At<uint32_t>(self, 0x36C) = g_timeInMs + kReminderDelay;
}

} // namespace msnobj

#define MSNOBJ_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(msnobj_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(msnobj_original_, __LINE__), \
             re::Mode::Original)

MSNOBJ_REVERSED("CObjectiveParam::CObjectiveParam", 0x006ADF00, &msnobj::Param_Construct);
MSNOBJ_REVERSED("CObjectiveParam::~CObjectiveParam", 0x006ADF10, &msnobj::Param_Destruct);
MSNOBJ_REVERSED("CMissionObjective::SetParamText", 0x006ADF30, &msnobj::Objective_SetParamText);
MSNOBJ_REVERSED("CMissionObjective::GetText", 0x006ADF90, &msnobj::Objective_GetText);
MSNOBJ_REVERSED("CMissionObjectiveMgr::Reset", 0x006AE020, &msnobj::Objectives_Reset);
MSNOBJ_REVERSED("CMissionObjectiveMgr::Get", 0x006AE080, &msnobj::Objectives_Get);
MSNOBJ_REVERSED("CMissionObjectiveMgr::Exists", 0x006AE090, &msnobj::Objectives_Exists);
MSNOBJ_REVERSED("CMissionObjectiveMgr::Rewind", 0x006AE0E0, &msnobj::Objectives_Rewind);
MSNOBJ_REVERSED("CMissionObjectiveMgr::SeekTo", 0x006AE0F0, &msnobj::Objectives_SeekTo);
MSNOBJ_REVERSED("CMissionObjectiveMgr::NextPending", 0x006AE100, &msnobj::Objectives_NextWhere<false>);
MSNOBJ_REVERSED("CMissionObjectiveMgr::NextDone", 0x006AE150, &msnobj::Objectives_NextWhere<true>);
MSNOBJ_REVERSED("CObjectiveParam::DeletingDestructor", 0x006AE1A0, &msnobj::Param_DeletingDestruct);
MSNOBJ_REVERSED("CMissionObjective::~CMissionObjective", 0x006AE1E0, &msnobj::Objective_Destruct);
MSNOBJ_REVERSED("CMissionObjectiveMgr::~CMissionObjectiveMgr", 0x006AE220, &msnobj::Objectives_Destruct);
MSNOBJ_REVERSED("CMissionObjectiveMgr::Add", 0x006AE270, &msnobj::Objectives_Add);
MSNOBJ_REVERSED("CMissionObjectiveMgr::SetParamInt", 0x006AE390, &msnobj::Objectives_SetParamInt);
MSNOBJ_REVERSED("CMissionObjectiveMgr::SetParamFloat", 0x006AE3D0, &msnobj::Objectives_SetParamFloat);
MSNOBJ_REVERSED("CMissionObjectiveMgr::SetParamTextKey", 0x006AE410, &msnobj::Objectives_SetParamText);
