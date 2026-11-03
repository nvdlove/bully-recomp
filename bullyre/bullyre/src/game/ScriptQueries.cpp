// Two small queries: whether the script callback (0x20C8704) is active, and a trigger
// by name from the trigger pool. 0x42E430, 0x431740.

#include <cstdint>

#include "reversed.h"

#define SCRIPTQ_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

GAME_VAR(uint8_t*, sq_scriptCallback, 0x020C8704);   // an object with a byte "active" at +4
GAME_VAR(uint8_t*, sq_triggerPool, 0x020C7C48);      // a CPool (objects +0, flags +4, element size +0xC)
GAME_FN(int, __thiscall, SQ_TriggerNames_Find, 0x006D7A10, void* self, const char* name);   // -1: none

} // namespace

namespace ScriptQueries {

// 0x42E430: the script callback exists and is active (only al is set)
uint8_t __cdecl IsScriptCallbackActive() {
    uint8_t* callback = sq_scriptCallback;
    return callback && callback[4] ? 1 : 0;
}

// 0x431740 (thiscall; `self` is passed on to the name search): the trigger of that name
// in the trigger pool (nullptr: none, or a free slot)
uint8_t* __fastcall GetTriggerByName(void* self, re::Edx, const char* name) {
    int slot = SQ_TriggerNames_Find(self, name);
    if (slot == -1) {
        return nullptr;
    }
    uint8_t* pool = sq_triggerPool;
    uint8_t* flags = *reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4);
    if (flags[slot] & 0x80) {
        return nullptr;
    }
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(pool) + *reinterpret_cast<int32_t*>(pool + 0xC) * slot;
}

SCRIPTQ_REVERSED("ScriptQueries::IsScriptCallbackActive", 0x0042E430, &IsScriptCallbackActive);
SCRIPTQ_REVERSED("CTriggers::GetByName", 0x00431740, &GetTriggerByName);

} // namespace ScriptQueries
