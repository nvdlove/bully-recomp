// The script event handlers (0x5B1DC0..0x5B5FB0): the Lua functions scripts register
// with RegisterGlobalEventHandler / RegisterHashEventHandler / RegisterPedEventHandler /
// RegisterPropEventHandler / RegisterTriggerEventHandler (NativesA.cpp), and the
// game's calls into them.
//
// A handler is a script reference (4 bytes: 0x5D80E0 set, 0x5D80B0 its script, 0x5D80D0
// its function, 0x5D9C20 its Lua state, 0x5D9C50 released). Global handlers are two
// arrays of 9 by event. The others are kept per object (keyed by a pointer: a hash
// object, a ped, a prop, a trigger) in small vectors: 10 entries inline, then a heap
// array (a count before it, as new[] makes) growing by doubling. An entry whose
// handlers are all unset is removed. Each vector has an owner script (whose ending
// clears it: SetNumberOfHandled...EventObjects sizes it for that script).
//
// Calling a handler: the script of the reference found (else it is unregistered), made
// current, its function called with the event's arguments, the previous script made
// current again.

#include <cstdint>

#include "game/Pool.h"
#include "game/ScriptApi.h"
#include "reversed.h"

namespace {

using re::At;

using ScriptRef = uint32_t;

GAME_FN(uint8_t, __thiscall, Ref_IsSet, 0x005D80E0, ScriptRef* ref);
GAME_FN(int, __thiscall, Ref_GetScript, 0x005D80B0, ScriptRef* ref);
GAME_FN(int, __thiscall, Ref_GetFunction, 0x005D80D0, ScriptRef* ref);
GAME_FN(lua_State*, __thiscall, Ref_GetState, 0x005D9C20, ScriptRef* ref);
GAME_FN(void, __thiscall, Ref_Release, 0x005D9C50, ScriptRef* ref);
// (0x5B1EB0: a jmp to Ref_Release, the destructor the arrays are destroyed with)
inline constexpr uintptr_t kRefDestructor = 0x005B1EB0;
inline constexpr uintptr_t kRefConstructor = 0x00667DA0;

inline void* const TheLurStore = reinterpret_cast<void*>(0x00D02850);
GAME_FN(void*, __thiscall, Scripts_GetCurrent, 0x005DB960, void* store);
GAME_FN(void*, __thiscall, Scripts_Find, 0x005DB8F0, void* store, int script);
GAME_FN(void, __thiscall, Scripts_SetCurrent, 0x005DBDA0, void* store, void* script);

GAME_FN(void, __cdecl, Lua_Begin, 0x005D8270, lua_State* L);
GAME_FN(void, __cdecl, Lua_Call, 0x005D82A0, lua_State* L, int nargs, int nresults);

// the eh vector constructor / destructor iterators
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, uint32_t size, int count, uintptr_t ctor);
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* array, uint32_t size, int count, uintptr_t dtor);
GAME_FN(void*, __cdecl, SE_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, SE_Free, 0x005EEAB0, void* p);

// the triggers (a pool; slot 0 is the world) and their handles
GAME_VAR(CPool<uint8_t>*, se_triggerPool, 0x020C7C48);
inline void* const se_triggerPoolWrapper = reinterpret_cast<void*>(0x020C7E98);
GAME_FN(int, __thiscall, Triggers_GetHandle, 0x006D7A60, void* wrapper, void* trigger);
GAME_VAR(CPool<uint8_t>*, se_objectPool, 0x00C0F614);
GAME_ARRAY(uint8_t*, se_modelInfoPtrs, 0x00C67738);
// the speech events' names, by id
GAME_ARRAY(const char*, se_speechNames, 0x00AE74E8);

// ---- the containers

template <int N>
struct EventEntry {
    int32_t key;             // +0 (0: unused)
    ScriptRef callbacks[N];  // +4
};
static_assert(sizeof(EventEntry<8>) == 0x24);
static_assert(sizeof(EventEntry<1>) == 8);
static_assert(sizeof(EventEntry<4>) == 0x14);
static_assert(sizeof(EventEntry<6>) == 0x1C);

template <int N>
struct EventVec {
    EventEntry<N> fixed[10];
    re::Ptr32<EventEntry<N>> heap;  // entries 10..
    uint32_t capacity;    // of the heap
    uint32_t count;

    EventEntry<N>* At(uint32_t i) { return i < 10 ? &fixed[i] : &heap[i - 10]; }
    // the entry's index, or count
    uint32_t Find(int32_t key) {
        uint32_t i = 0;
        while (i < count && At(i)->key != key) {
            i++;
        }
        return i;
    }
};
static_assert(sizeof(EventVec<8>) == 0x174);
static_assert(sizeof(EventVec<1>) == 0x5C);
static_assert(sizeof(EventVec<4>) == 0xD4);
static_assert(sizeof(EventVec<6>) == 0x124);

using HashVec = EventVec<8>;
using PedVec = EventVec<1>;
using PropVec = EventVec<4>;
using TriggerVec = EventVec<6>;

// global handlers: [flag][event]
GAME_ARRAY(ScriptRef, g_globalEvents0, 0x00CFAD58);
GAME_ARRAY(ScriptRef, g_globalEvents1, 0x00CFAD7C);
GAME_VAR(uint8_t, g_globalEventsLocked, 0x00CFADA0);   // (unregistering is ignored)

GAME_VAR(HashVec, g_hashEvents, 0x00CFADA8);
GAME_VAR(uint8_t, g_hashEventsLocked, 0x00CFAF1C);
GAME_VAR(int32_t, g_hashEventsOwner, 0x00CFAF20);
GAME_VAR(PedVec, g_pedEvents, 0x00CFAF28);
GAME_VAR(uint8_t, g_pedEventsLocked, 0x00CFAF84);
GAME_VAR(int32_t, g_pedEventsOwner, 0x00CFAF88);
GAME_VAR(PropVec, g_propEvents, 0x00CFAF90);
GAME_VAR(int32_t, g_propEventsOwner, 0x00CFB064);
GAME_VAR(TriggerVec, g_triggerEvents0, 0x00CFB068);
GAME_VAR(TriggerVec, g_triggerEvents1, 0x00CFB18C);
GAME_VAR(uint8_t, g_triggerEventsLocked, 0x00CFB2B0);
GAME_VAR(int32_t, g_triggerEventsOwner0, 0x00CFB2B4);
GAME_VAR(int32_t, g_triggerEventsOwner1, 0x00CFB2B8);

TriggerVec* TriggerVecOf(int which) {
    return which == 0 ? &g_triggerEvents0 : &g_triggerEvents1;
}

// The game's functions per container (so each call goes through its address)
struct HashFns {
    static constexpr int N = 8;
    static constexpr uintptr_t ClearCallback = 0x005B3890, ClearForScript = 0x005B38E0, Move = 0x005B3930,
                               RemoveAt = 0x005B3A90, Reserve = 0x005B3BA0, Clear = 0x005B3E90,
                               PushBack = 0x005B3FC0, EntryCtor = 0x005B3B80;
};
struct PedFns {
    static constexpr int N = 1;
    static constexpr uintptr_t ClearCallback = 0x005B4140, RemoveAt = 0x005B4290, Reserve = 0x005B4330,
                               Clear = 0x005B45B0, PushBack = 0x005B46F0, EntryCtor = 0x005B4320;
};
struct PropFns {
    static constexpr int N = 4;
    static constexpr uintptr_t ClearForScript = 0x005B4830, RemoveAt = 0x005B4980, Reserve = 0x005B4A60,
                               Clear = 0x005B4DD0, PushBack = 0x005B4F00, EntryCtor = 0x005B4A40;
};
struct TriggerFns {
    static constexpr int N = 6;
    static constexpr uintptr_t ClearCallback = 0x005B5080, ClearForScript = 0x005B50D0, VecAt = 0x005B5120,
                               RemoveAt = 0x005B5620, Reserve = 0x005B5710, Clear = 0x005B5BC0,
                               PushBack = 0x005B5DC0, EntryCtor = 0x005B56F0;
};

template <typename V>
void CallRemoveAt(uintptr_t fn, V* vec, uint32_t i) {
    reinterpret_cast<void(__thiscall*)(V*, uint32_t)>(fn)(vec, i);
}
template <typename V>
void CallVoid(uintptr_t fn, V* self) {
    reinterpret_cast<void(__thiscall*)(V*)>(fn)(self);
}
template <typename E>
void CallEntryInt(uintptr_t fn, E* entry, int value) {
    reinterpret_cast<void(__thiscall*)(E*, int)>(fn)(entry, value);
}
template <typename E>
void CallEntryPtr(uintptr_t fn, E* entry, void* p) {
    reinterpret_cast<void(__thiscall*)(E*, void*)>(fn)(entry, p);
}

template <int N>
bool AnySet(EventEntry<N>* e) {
    for (int j = 0; j < N; j++) {
        if (Ref_IsSet(&e->callbacks[j]) != 0) {
            return true;
        }
    }
    return false;
}

// The entry stays when its key is set and a handler is
template <int N>
bool EntryInUse(EventEntry<N>* e) {
    return e->key != 0 && AnySet(e);
}

// Entry: releases handler `index` (if in range); the key cleared when none is left
template <int N>
void EntryClearCallback(EventEntry<N>* self, uint32_t index) {
    if (index < uint32_t(N)) {
        Ref_Release(&self->callbacks[index]);
    }
    if (!AnySet(self)) {
        self->key = 0;
    }
}

// Entry: releases the handlers belonging to the script; the key cleared when none of
// another script's is left
template <int N>
void EntryClearForScript(EventEntry<N>* self, const int* script) {
    bool allClear = true;
    for (int j = 0; j < N; j++) {
        ScriptRef* cb = &self->callbacks[j];
        if (Ref_IsSet(cb) != 0) {
            if (Ref_GetScript(cb) == *script) {
                Ref_Release(cb);
            } else {
                allClear = false;
            }
        }
    }
    if (allClear) {
        self->key = 0;
    }
}

// Moves src into dst (src's handlers zeroed, not released)
template <int N>
void MoveEntry(EventEntry<N>* dst, EventEntry<N>* src) {
    dst->key = src->key;
    for (int j = 0; j < N; j++) {
        dst->callbacks[j] = src->callbacks[j];
        src->callbacks[j] = 0;
    }
}

// Vector: removes entry i (the ones after moved down)
template <int N>
void VecRemoveAt(EventVec<N>* self, uint32_t i) {
    while (i < self->count - 1u) {
        MoveEntry(self->At(i), self->At(i + 1));
        i++;
    }
    self->count--;
}

// Vector: the heap array to hold at least n (moving the entries over)
template <int N>
void VecReserve(EventVec<N>* self, uint32_t n, uintptr_t entryCtor) {
    if (n <= self->capacity) {
        return;
    }
    uint64_t wide = uint64_t(n) * sizeof(EventEntry<N>);
    uint32_t bytes = wide > 0xFFFFFFFFull ? 0xFFFFFFFFu : uint32_t(wide);
    bytes = bytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : bytes + 4;
    uint32_t* block = static_cast<uint32_t*>(SE_Malloc(bytes));
    EventEntry<N>* array = nullptr;
    if (block != nullptr) {
        block[0] = n;
        array = reinterpret_cast<EventEntry<N>*>(block + 1);
        ConstructArray(array, sizeof(EventEntry<N>), int(n), entryCtor);
    }
    if (self->heap != nullptr) {
        // (the game's loop: at least once unless count is 10, while k < count - 10
        // unsigned; it only runs with count = capacity + 10)
        if (self->count != 10) {
            uint32_t k = 0;
            do {
                MoveEntry(&array[k], &self->heap[k]);
                k++;
            } while (k < self->count - 10);
        }
        EventEntry<N>* old = self->heap;
        int oldCount = reinterpret_cast<int32_t*>(old)[-1];
        for (int k = oldCount - 1; k >= 0; k--) {
            DestructArray(old[k].callbacks, 4, N, kRefDestructor);
        }
        SE_Free(reinterpret_cast<int32_t*>(old) - 1);
    }
    self->heap = array;
    self->capacity = n;
}

template <int N>
void DestroyHeap(EventVec<N>* self) {
    EventEntry<N>* heap = self->heap;
    int n = reinterpret_cast<int32_t*>(heap)[-1];
    for (int k = n - 1; k >= 0; k--) {
        DestructArray(heap[k].callbacks, 4, N, kRefDestructor);
    }
    SE_Free(reinterpret_cast<int32_t*>(heap) - 1);
}

template <int N>
EventVec<N>* VecConstruct(EventVec<N>* self) {
    for (int k = 0; k < 10; k++) {
        self->fixed[k].key = 0;
        ConstructArray(self->fixed[k].callbacks, 4, N, kRefConstructor);
    }
    self->heap = nullptr;
    self->capacity = 0;
    self->count = 0;
    return self;
}

template <int N>
void VecDestruct(EventVec<N>* self) {
    if (self->heap != nullptr) {
        DestroyHeap(self);
        self->heap = nullptr;
    }
    for (int k = 9; k >= 0; k--) {
        DestructArray(self->fixed[k].callbacks, 4, N, kRefDestructor);
    }
}

// (the inline entries' handlers are not released)
template <int N>
void VecClear(EventVec<N>* self) {
    if (self->heap == nullptr) {
        self->capacity = 0;
        self->count = 0;
        return;
    }
    DestroyHeap(self);
    self->heap = nullptr;
    self->capacity = 0;
    self->count = 0;
}

template <typename F, int N>
void VecPushBack(EventVec<N>* self, EventEntry<N>* entry) {
    if (self->count == self->capacity + 10) {
        CallEntryInt(F::Reserve, self, self->capacity == 0 ? 1 : int(self->capacity * 2));
    }
    MoveEntry(self->At(self->count), entry);
    self->count++;
}

// The handler slot for the key's event (entry 0's when the key has no entry)
template <int N>
ScriptRef* RegisterSlot(EventVec<N>* vec, int32_t key, const int* event) {
    uint32_t i = vec->Find(key);
    if (i < vec->count) {
        return &vec->At(i)->callbacks[*event];
    }
    return &vec->fixed[0].callbacks[*event];
}

template <int N>
uint8_t IsHandled(EventVec<N>* vec, int32_t key, const uint32_t* event) {
    uint32_t i = vec->Find(key);
    if (i < vec->count) {
        EventEntry<N>* e = vec->At(i);
        if (e->key != 0 && *event < uint32_t(N)) {
            return Ref_IsSet(&e->callbacks[*event]);
        }
    }
    return 0;
}

template <typename F, int N>
void Unregister(EventVec<N>* vec, int32_t key, const int* event) {
    uint32_t i = vec->Find(key);
    if (i >= vec->count) {
        return;
    }
    CallEntryInt(F::ClearCallback, vec->At(i), *event);
    if (EntryInUse(vec->At(i))) {
        return;
    }
    CallRemoveAt(F::RemoveAt, vec, i);
}

template <typename F, int N>
void ClearVecForScript(EventVec<N>* vec, const int* script) {
    uint32_t i = 0;
    while (i < vec->count) {
        CallEntryPtr(F::ClearForScript, vec->At(i), const_cast<int*>(script));
        if (EntryInUse(vec->At(i))) {
            i++;
        } else {
            CallRemoveAt(F::RemoveAt, vec, i);
        }
    }
}

// An entry for the key (only while there's room: never grows here)
template <typename F, int N>
void Prepare(EventVec<N>* vec, int32_t key) {
    if (vec->Find(key) < vec->count) {
        return;
    }
    if (vec->count != vec->capacity + 10) {
        EventEntry<N> entry;
        entry.key = key;
        for (int j = 0; j < N; j++) {
            entry.callbacks[j] = 0;
        }
        CallEntryPtr(F::PushBack, vec, &entry);
        for (int j = N - 1; j >= 0; j--) {
            Ref_Release(&entry.callbacks[j]);
        }
    }
}

// Sizes the vector for n objects of the script (only when empty: a vector still in use
// by the same script is cleared first)
template <typename F, int N>
void SetNumberOfObjects(EventVec<N>* vec, int32_t& owner, uint32_t n, int script) {
    if (vec->count != 0) {
        if (script == owner) {
            CallVoid(F::Clear, vec);
        }
        if (vec->count != 0) {
            return;
        }
    }
    if (n > 10) {
        CallEntryInt(F::Reserve, vec, int(n - 10));
    }
    owner = script;
}

// ---- calling handlers

// Makes the handler's script current and pushes its function; nullptr when the script
// has gone
lua_State* BeginCall(ScriptRef* cb) {
    void* script = Scripts_Find(TheLurStore, Ref_GetScript(cb));
    if (script == nullptr) {
        return nullptr;
    }
    Scripts_SetCurrent(TheLurStore, script);
    lua_State* L = Ref_GetState(cb);
    Lua_Begin(L);
    Script::PushRef(L, Ref_GetFunction(cb));
    return L;
}

void EndCall(lua_State* L, int nargs, void* previous) {
    Lua_Call(L, nargs, 0);
    Scripts_SetCurrent(TheLurStore, previous);
}

int PedHandle(void* ped) {
    using Fn = int(__thiscall*)(void*);
    return re::GameVirtual<Fn>(ped, 0xAC / 4)(ped);
}

int ObjectHandle(uint8_t* object) {
    CPool<uint8_t>* pool = se_objectPool;
    int slot = int(object - pool->objects) / pool->elemSize;
    return pool->flags[slot] + slot * 0x100;
}

uint8_t* WorldTrigger() {
    CPool<uint8_t>* pool = se_triggerPool;
    return (pool->flags[0] & 0x80) == 0 ? pool->objects : nullptr;
}

GAME_FN(uint8_t, __cdecl, GE_IsHandled, 0x005B1E30, int* event, int flag);
GAME_FN(ScriptRef*, __cdecl, GE_Slot, 0x005B1DC0, int* event, int flag);
GAME_FN(void, __cdecl, GE_Unregister, 0x005B1DF0, int* event, int flag);
GAME_FN(void, __cdecl, GE_CallTriggerPed, 0x005B1F10, ScriptRef* cb, void* trigger, void* ped);
GAME_FN(uint8_t, __cdecl, GE_TriggerPedEvent, 0x005B1F80, void* trigger, void* ped, int event);
GAME_FN(uint8_t, __cdecl, TE_TriggerPedEvent, 0x005B2020, void* trigger, void* ped, int which, int event);
GAME_FN(ScriptRef*, __cdecl, HE_Slot, 0x005B3990, int32_t key, int* event);
GAME_FN(uint8_t, __cdecl, HE_IsHandled, 0x005B3A10, int32_t key, uint32_t* event);
GAME_FN(void, __cdecl, HE_Unregister, 0x005B3CE0, int32_t key, int* event);
GAME_FN(void, __cdecl, HE_ClearForScript, 0x005B3F00, int* script);
GAME_FN(ScriptRef*, __cdecl, PE_Slot, 0x005B4190, void* ped, int* event);
GAME_FN(uint8_t, __cdecl, PE_IsHandled, 0x005B4210, void* ped, uint32_t* event);
GAME_FN(void, __cdecl, PE_Unregister, 0x005B4430, void* ped, int* event);
GAME_FN(void, __cdecl, PE_ClearForScript, 0x005B4610, int* script);
GAME_FN(ScriptRef*, __cdecl, PrE_Slot, 0x005B4880, void* prop, int* event);
GAME_FN(uint8_t, __cdecl, PrE_IsHandled, 0x005B4900, void* prop, uint32_t* event);
GAME_FN(void, __cdecl, PrE_Unregister, 0x005B4BA0, void* prop, uint32_t* event);
GAME_FN(void, __cdecl, PrE_ClearForScript, 0x005B4E40, int* script);
GAME_FN(ScriptRef*, __cdecl, TE_Slot, 0x005B5150, void* trigger, int* event, int which);
GAME_FN(uint8_t, __cdecl, TE_IsHandled, 0x005B5260, void* trigger, uint32_t* event, int which);
GAME_FN(void, __cdecl, TE_Unregister, 0x005B5850, void* trigger, int* event, int which);
GAME_FN(void, __cdecl, TE_ClearForScript, 0x005B5C40, int* script);
GAME_FN(void, __cdecl, GE_ClearForScript, 0x005B1E60, int* script);

// ---- global handlers

// 0x5B1DC0: the handler slot for the event
ScriptRef* __cdecl GlobalEvents_Slot(int* event, int flag) {
    if (flag == 0) {
        return &g_globalEvents0[*event];
    }
    return &g_globalEvents1[*event];
}

// 0x5B1DF0
void __cdecl GlobalEvents_Unregister(int* event, int flag) {
    if (g_globalEventsLocked != 0) {
        return;
    }
    if (flag == 0) {
        Ref_Release(&g_globalEvents0[*event]);
        return;
    }
    Ref_Release(&g_globalEvents1[*event]);
}

// 0x5B1E30
uint8_t __cdecl GlobalEvents_IsHandled(int* event, int flag) {
    if (flag == 0) {
        return Ref_IsSet(&g_globalEvents0[*event]);
    }
    return Ref_IsSet(&g_globalEvents1[*event]);
}

// 0x5B1E60: releases the script's global handlers
void __cdecl GlobalEvents_ClearForScript(int* script) {
    for (int i = 0; i < 9; i++) {
        if (Ref_GetScript(&g_globalEvents0[i]) == *script) {
            Ref_Release(&g_globalEvents0[i]);
        }
        if (Ref_GetScript(&g_globalEvents1[i]) == *script) {
            Ref_Release(&g_globalEvents1[i]);
        }
    }
}

// 0x5B1EC0 (thiscall on 0xCFAD58): the arrays' destructor
void __fastcall GlobalEvents_Destruct(ScriptRef* self, re::Edx) {
    for (int i = 17; i >= 9; i--) {
        Ref_Release(&self[i]);
    }
    for (int i = 8; i >= 0; i--) {
        Ref_Release(&self[i]);
    }
}

// 0x5B1F10: calls the handler with (trigger handle, ped handle). (The previous script
// is the caller's to restore)
void __cdecl CallTriggerPedHandler(ScriptRef* cb, void* trigger, void* ped) {
    lua_State* L = Ref_GetState(cb);
    Lua_Begin(L);
    Script::PushRef(L, Ref_GetFunction(cb));
    Script::PushInt(L, Triggers_GetHandle(se_triggerPoolWrapper, trigger));
    Script::PushInt(L, PedHandle(ped));
    Lua_Call(L, 2, 0);
}

// 0x5B1F80: the global handler for a trigger/ped event
uint8_t __cdecl GlobalEvents_TriggerPed(void* trigger, void* ped, int event) {
    void* previous = Scripts_GetCurrent(TheLurStore);
    bool called = false;
    if (GE_IsHandled(&event, 0) != 0) {
        ScriptRef* cb = GE_Slot(&event, 0);
        void* script = Scripts_Find(TheLurStore, Ref_GetScript(cb));
        if (script != nullptr) {
            Scripts_SetCurrent(TheLurStore, script);
            GE_CallTriggerPed(cb, trigger, ped);
            Scripts_SetCurrent(TheLurStore, previous);
            called = true;
        } else {
            GE_Unregister(&event, 0);
        }
    }
    return called;
}

// 0x5B2020: the trigger's own handler (vector `which`) for the event
uint8_t __cdecl TriggerEvents_TriggerPed(void* trigger, void* ped, int which, int event) {
    void* previous = Scripts_GetCurrent(TheLurStore);
    if (TE_IsHandled(trigger, reinterpret_cast<uint32_t*>(&event), which) != 0) {
        ScriptRef* cb = TE_Slot(trigger, &event, which);
        void* script = Scripts_Find(TheLurStore, Ref_GetScript(cb));
        if (script != nullptr) {
            Scripts_SetCurrent(TheLurStore, script);
            GE_CallTriggerPed(cb, trigger, ped);
            Scripts_SetCurrent(TheLurStore, previous);
            return 1;
        }
        TE_Unregister(trigger, &event, which);
    }
    return 0;
}

// 0x5B20D0: trigger event 2 in both vectors, with (trigger handle, the short at
// ped +0x10E)
uint8_t __cdecl TriggerEvents_Event2(void* trigger, uint8_t* ped) {
    uint8_t called = 0;
    void* previous = Scripts_GetCurrent(TheLurStore);
    int event = 2;
    if (TE_IsHandled(trigger, reinterpret_cast<uint32_t*>(&event), 0) != 0) {
        event = 2;
        ScriptRef* cb = TE_Slot(trigger, &event, 0);
        lua_State* L = BeginCall(cb);
        if (L == nullptr) {
            event = 2;
            TE_Unregister(trigger, &event, 0);
        } else {
            Script::PushInt(L, Triggers_GetHandle(se_triggerPoolWrapper, trigger));
            Script::PushInt(L, At<int16_t>(ped, 0x10E));
            EndCall(L, 2, previous);
            called = 1;
        }
    }
    event = 2;
    if (TE_IsHandled(trigger, reinterpret_cast<uint32_t*>(&event), 1) != 0) {
        event = 2;
        ScriptRef* cb = TE_Slot(trigger, &event, 1);
        lua_State* L = BeginCall(cb);
        if (L != nullptr) {
            Script::PushInt(L, Triggers_GetHandle(se_triggerPoolWrapper, trigger));
            Script::PushInt(L, At<int16_t>(ped, 0x10E));
            EndCall(L, 2, previous);
            return 1;
        }
        event = 2;
        TE_Unregister(trigger, &event, 1);
    }
    return called;
}

// 0x5B22A0: trigger event 5, both vectors
uint8_t __cdecl TriggerEvents_Event5(void* trigger, void* ped) {
    uint8_t a = TE_TriggerPedEvent(trigger, ped, 0, 5);
    uint8_t b = TE_TriggerPedEvent(trigger, ped, 1, 5);
    return (a + b) != 0;
}

// 0x5B22F0: global event 7 (both) and ped event 0: (ped handle, other's handle; -1 for
// none). (A global handler whose script has gone unregisters ped event 0 instead)
uint8_t __cdecl PedEvents_Event0(void* ped, void* other) {
    uint8_t called = 0;
    void* previous = Scripts_GetCurrent(TheLurStore);
    for (int flag = 0; flag < 2; flag++) {
        int event = 7;
        if (GE_IsHandled(&event, flag) != 0) {
            event = 7;
            ScriptRef* cb = GE_Slot(&event, flag);
            lua_State* L = BeginCall(cb);
            if (L == nullptr) {
                event = 0;
                PE_Unregister(ped, &event);
            } else {
                int a = -1;
                int b = -1;
                if (ped != nullptr) {
                    a = PedHandle(ped);
                }
                if (other != nullptr) {
                    b = PedHandle(other);
                }
                Script::PushInt(L, a);
                Script::PushInt(L, b);
                EndCall(L, 2, previous);
                called = 1;
            }
        }
    }
    int event = 0;
    if (PE_IsHandled(ped, reinterpret_cast<uint32_t*>(&event)) != 0) {
        event = 0;
        ScriptRef* cb = PE_Slot(ped, &event);
        lua_State* L = BeginCall(cb);
        if (L != nullptr) {
            int a = -1;
            int b = -1;
            if (ped != nullptr) {
                a = PedHandle(ped);
            }
            if (other != nullptr) {
                b = PedHandle(other);
            }
            Script::PushInt(L, a);
            Script::PushInt(L, b);
            EndCall(L, 2, previous);
            return 1;
        }
        event = 0;
        PE_Unregister(ped, &event);
    }
    return called;
}

// 0x5B25E0: global event 8 (both): a ped spoke (ped handle, the speech's name, a, b).
// (Either failing unregisters flag 0's)
uint8_t __cdecl GlobalEvents_Speech(void* ped, int speech, int a, int b) {
    uint8_t called = 0;
    void* previous = Scripts_GetCurrent(TheLurStore);
    for (int flag = 0; flag < 2; flag++) {
        int event = 8;
        if (GE_IsHandled(&event, flag) != 0) {
            event = 8;
            ScriptRef* cb = GE_Slot(&event, flag);
            lua_State* L = BeginCall(cb);
            if (L == nullptr) {
                event = 8;
                GE_Unregister(&event, 0);
            } else {
                int handle = -1;
                if (ped != nullptr) {
                    handle = PedHandle(ped);
                }
                Script::PushInt(L, handle);
                Script::PushString(L, se_speechNames[speech]);
                Script::PushInt(L, a);
                Script::PushInt(L, b);
                EndCall(L, 4, previous);
                if (flag == 1) {
                    return 1;
                }
                called = 1;
            }
        }
    }
    return called;
}

// 0x5B27E0: everything the script registered
void __cdecl ScriptEvents_ClearForScript(int* script) {
    GE_ClearForScript(script);
    HE_ClearForScript(script);
    TE_ClearForScript(script);
    PrE_ClearForScript(script);
    PE_ClearForScript(script);
}

// 0x5B2810: a ped entered the trigger (the player, ped type 0xD: global 4 and trigger
// event 1; others global 2 and trigger event 0)
uint8_t __cdecl TriggerEvents_Enter(void* trigger, uint8_t* ped) {
    uint8_t a, b, c;
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        a = GE_TriggerPedEvent(trigger, ped, 4);
        b = TE_TriggerPedEvent(trigger, ped, 0, 1);
        c = TE_TriggerPedEvent(trigger, ped, 1, 1);
    } else {
        a = GE_TriggerPedEvent(trigger, ped, 2);
        b = TE_TriggerPedEvent(trigger, ped, 0, 0);
        c = TE_TriggerPedEvent(trigger, ped, 1, 0);
    }
    return (c + b + a) != 0;
}

// 0x5B28D0: a ped left it (global 5 / trigger 4 for the player; global 3 / trigger 3)
uint8_t __cdecl TriggerEvents_Exit(void* trigger, uint8_t* ped) {
    uint8_t a, b, c;
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        a = GE_TriggerPedEvent(trigger, ped, 5);
        b = TE_TriggerPedEvent(trigger, ped, 0, 4);
        c = TE_TriggerPedEvent(trigger, ped, 1, 4);
    } else {
        a = GE_TriggerPedEvent(trigger, ped, 3);
        b = TE_TriggerPedEvent(trigger, ped, 0, 3);
        c = TE_TriggerPedEvent(trigger, ped, 1, 3);
    }
    return (c + b + a) != 0;
}

// A prop's hash handler (its hash, +0x3AC): (the prop's hash when its trigger, +0x23C,
// is the world, else the trigger's hash +0x2C, as userdata; the trigger's handle)
uint8_t PropHashEvent(uint8_t* prop, int hashEvent, void* previous) {
    uint32_t event = uint32_t(hashEvent);
    if (HE_IsHandled(At<int32_t>(prop, 0x3AC), &event) == 0) {
        return 0;
    }
    int e = hashEvent;
    ScriptRef* cb = HE_Slot(At<int32_t>(prop, 0x3AC), &e);
    lua_State* L = BeginCall(cb);
    if (L == nullptr) {
        e = hashEvent;
        HE_Unregister(At<int32_t>(prop, 0x3AC), &e);
        return 0;
    }
    uint8_t* trigger = At<uint8_t*>(prop, 0x23C);
    void* hash = trigger == WorldTrigger() ? At<void*>(prop, 0x3AC) : At<void*>(trigger, 0x2C);
    Script::PushUserdata(L, hash);
    Script::PushInt(L, Triggers_GetHandle(se_triggerPoolWrapper, trigger));
    EndCall(L, 2, previous);
    return 1;
}

// 0x5B2990: hash event 2 of a prop
uint8_t __cdecl PropEvents_Hash2(uint8_t* prop) {
    void* previous = Scripts_GetCurrent(TheLurStore);
    return PropHashEvent(prop, 2, previous);
}

// Hash event, then the prop event (not for a prop in the world trigger: unregistered)
uint8_t PropEvent(uint8_t* prop, int hashEvent, int propEvent) {
    void* previous = Scripts_GetCurrent(TheLurStore);
    uint8_t called = PropHashEvent(prop, hashEvent, previous);
    uint32_t event = uint32_t(propEvent);
    if (PrE_IsHandled(prop, &event) == 0) {
        return called;
    }
    uint8_t* trigger = At<uint8_t*>(prop, 0x23C);
    event = uint32_t(propEvent);
    if (trigger == WorldTrigger()) {
        PrE_Unregister(prop, &event);
        return called;
    }
    ScriptRef* cb = PrE_Slot(prop, reinterpret_cast<int*>(&event));
    lua_State* L = BeginCall(cb);
    if (L != nullptr) {
        Script::PushUserdata(L, At<void*>(trigger, 0x2C));
        Script::PushInt(L, Triggers_GetHandle(se_triggerPoolWrapper, trigger));
        EndCall(L, 2, previous);
        return 1;
    }
    event = uint32_t(propEvent);
    PrE_Unregister(prop, &event);
    return called;
}

// 0x5B2AD0: broken (hash 4, prop 0)
uint8_t __cdecl PropEvents_Broken(uint8_t* prop) {
    return PropEvent(prop, 4, 0);
}
// 0x5B2D10 (hash 5, prop 1)
uint8_t __cdecl PropEvents_Event1(uint8_t* prop) {
    return PropEvent(prop, 5, 1);
}
// 0x5B2F50: opened (hash 6, prop 2)
uint8_t __cdecl PropEvents_Open(uint8_t* prop) {
    return PropEvent(prop, 6, 2);
}
// 0x5B3190: closed (hash 7, prop 3)
uint8_t __cdecl PropEvents_Close(uint8_t* prop) {
    return PropEvent(prop, 7, 3);
}

// 0x5B33D0: global event 6 with the model's hash (model info +4) as userdata
uint8_t __cdecl GlobalEvents_Model(int model) {
    void* previous = Scripts_GetCurrent(TheLurStore);
    int event = 6;
    if (GE_IsHandled(&event, 0) == 0) {
        return 0;
    }
    event = 6;
    ScriptRef* cb = GE_Slot(&event, 0);
    lua_State* L = BeginCall(cb);
    if (L != nullptr) {
        Script::PushUserdata(L, At<void*>(se_modelInfoPtrs[model], 4));
        EndCall(L, 1, previous);
        return 1;
    }
    event = 6;
    GE_Unregister(&event, 0);
    return 0;
}

void PushObject(lua_State* L, uint8_t* object) {
    Script::PushUserdata(L, At<void*>(object, 0x228));
    Script::PushInt(L, ObjectHandle(object));
}

// 0x5B34B0: an object's hash event 0 (its hash +0x228), then global event 0; with
// (the hash as userdata, the object's handle)
uint8_t __cdecl ObjectEvents_Event0(uint8_t* object) {
    uint8_t called = 0;
    void* previous = Scripts_GetCurrent(TheLurStore);
    uint32_t event = 0;
    if (HE_IsHandled(At<int32_t>(object, 0x228), &event) != 0) {
        int e = 0;
        ScriptRef* cb = HE_Slot(At<int32_t>(object, 0x228), &e);
        lua_State* L = BeginCall(cb);
        if (L == nullptr) {
            e = 0;
            HE_Unregister(At<int32_t>(object, 0x228), &e);
        } else {
            PushObject(L, object);
            EndCall(L, 2, previous);
            called = 1;
        }
    }
    int e = 0;
    if (GE_IsHandled(&e, 0) != 0) {
        e = 0;
        ScriptRef* cb = GE_Slot(&e, 0);
        lua_State* L = BeginCall(cb);
        if (L != nullptr) {
            PushObject(L, object);
            EndCall(L, 2, previous);
            return 1;
        }
        e = 0;
        GE_Unregister(&e, 0);
    }
    return called;
}

// 0x5B3690: global event 1, then the object's hash event 3. (A global handler whose
// script has gone unregisters hash event 3 instead)
uint8_t __cdecl ObjectEvents_Event3(uint8_t* object) {
    uint8_t called = 0;
    void* previous = Scripts_GetCurrent(TheLurStore);
    int e = 1;
    if (GE_IsHandled(&e, 0) != 0) {
        e = 1;
        ScriptRef* cb = GE_Slot(&e, 0);
        lua_State* L = BeginCall(cb);
        if (L == nullptr) {
            e = 3;
            HE_Unregister(At<int32_t>(object, 0x228), &e);
        } else {
            PushObject(L, object);
            EndCall(L, 2, previous);
            called = 1;
        }
    }
    uint32_t event = 3;
    if (HE_IsHandled(At<int32_t>(object, 0x228), &event) != 0) {
        e = 3;
        ScriptRef* cb = HE_Slot(At<int32_t>(object, 0x228), &e);
        lua_State* L = BeginCall(cb);
        if (L != nullptr) {
            PushObject(L, object);
            EndCall(L, 2, previous);
            return 1;
        }
        e = 3;
        HE_Unregister(At<int32_t>(object, 0x228), &e);
    }
    return called;
}

// ---- hash events (8 per key)

void __fastcall HashEntry_ClearCallback(EventEntry<8>* self, re::Edx, uint32_t index) {
    EntryClearCallback(self, index);
}
void __fastcall HashEntry_ClearForScript(EventEntry<8>* self, re::Edx, const int* script) {
    EntryClearForScript(self, script);
}
void __fastcall HashEntry_Move(EventEntry<8>* self, re::Edx, EventEntry<8>* src) {
    MoveEntry(self, src);
}
ScriptRef* __cdecl HashEvents_Slot(int32_t key, int* event) {
    return RegisterSlot(&g_hashEvents, key, event);
}
uint8_t __cdecl HashEvents_IsHandled(int32_t key, uint32_t* event) {
    return IsHandled(&g_hashEvents, key, event);
}
void __fastcall HashVec_RemoveAt(HashVec* self, re::Edx, uint32_t i) {
    VecRemoveAt(self, i);
}
void __fastcall HashVec_Reserve(HashVec* self, re::Edx, uint32_t n) {
    VecReserve(self, n, HashFns::EntryCtor);
}
void __cdecl HashEvents_Unregister(int32_t key, int* event) {
    if (g_hashEventsLocked == 0) {
        Unregister<HashFns>(&g_hashEvents, key, event);
    }
}
HashVec* __fastcall HashVec_Construct(HashVec* self, re::Edx) {
    return VecConstruct(self);
}
void __fastcall HashVec_Destruct(HashVec* self, re::Edx) {
    VecDestruct(self);
}
void __fastcall HashVec_Clear(HashVec* self, re::Edx) {
    VecClear(self);
}
void __cdecl HashEvents_ClearForScript(int* script) {
    if (*script == g_hashEventsOwner) {
        CallVoid(HashFns::Clear, &g_hashEvents);
        g_hashEventsOwner = -1;
        return;
    }
    ClearVecForScript<HashFns>(&g_hashEvents, script);
}
void __cdecl HashEvents_Clear() {
    CallVoid(HashFns::Clear, &g_hashEvents);
}
// (the entry moved through the game's Move)
void __fastcall HashVec_PushBack(HashVec* self, re::Edx, EventEntry<8>* entry) {
    if (self->count == self->capacity + 10) {
        CallEntryInt(HashFns::Reserve, self, self->capacity == 0 ? 1 : int(self->capacity * 2));
    }
    CallEntryPtr(HashFns::Move, self->At(self->count), entry);
    self->count++;
}
void __cdecl HashEvents_Prepare(int32_t key) {
    Prepare<HashFns>(&g_hashEvents, key);
}
void __cdecl HashEvents_SetNumberOfObjects(uint32_t n, int script) {
    SetNumberOfObjects<HashFns>(&g_hashEvents, g_hashEventsOwner, n, script);
}

// ---- ped events (1 per ped)

void __fastcall PedEntry_ClearCallback(EventEntry<1>* self, re::Edx, uint32_t index) {
    EntryClearCallback(self, index);
}
ScriptRef* __cdecl PedEvents_Slot(int32_t ped, int* event) {
    return RegisterSlot(&g_pedEvents, ped, event);
}
uint8_t __cdecl PedEvents_IsHandled(int32_t ped, uint32_t* event) {
    return IsHandled(&g_pedEvents, ped, event);
}
void __fastcall PedVec_RemoveAt(PedVec* self, re::Edx, uint32_t i) {
    VecRemoveAt(self, i);
}
void __fastcall PedVec_Reserve(PedVec* self, re::Edx, uint32_t n) {
    VecReserve(self, n, PedFns::EntryCtor);
}
void __cdecl PedEvents_Unregister(int32_t ped, int* event) {
    if (g_pedEventsLocked == 0) {
        Unregister<PedFns>(&g_pedEvents, ped, event);
    }
}
PedVec* __fastcall PedVec_Construct(PedVec* self, re::Edx) {
    return VecConstruct(self);
}
void __fastcall PedVec_Destruct(PedVec* self, re::Edx) {
    VecDestruct(self);
}
void __fastcall PedVec_Clear(PedVec* self, re::Edx) {
    VecClear(self);
}
// (the entry's clearing inlined)
void __cdecl PedEvents_ClearForScript(int* script) {
    if (*script == g_pedEventsOwner) {
        CallVoid(PedFns::Clear, &g_pedEvents);
        g_pedEventsOwner = -1;
        return;
    }
    uint32_t i = 0;
    while (i < g_pedEvents.count) {
        EventEntry<1>* e = g_pedEvents.At(i);
        ScriptRef* cb = &e->callbacks[0];
        if (Ref_IsSet(cb) == 0) {
            e->key = 0;
        } else if (Ref_GetScript(cb) == *script) {
            Ref_Release(cb);
            e->key = 0;
        }
        if (EntryInUse(g_pedEvents.At(i))) {
            i++;
        } else {
            CallRemoveAt(PedFns::RemoveAt, &g_pedEvents, i);
        }
    }
}
void __cdecl PedEvents_Clear() {
    CallVoid(PedFns::Clear, &g_pedEvents);
}
void __fastcall PedVec_PushBack(PedVec* self, re::Edx, EventEntry<1>* entry) {
    VecPushBack<PedFns>(self, entry);
}
void __cdecl PedEvents_Prepare(int32_t ped) {
    Prepare<PedFns>(&g_pedEvents, ped);
}
void __cdecl PedEvents_SetNumberOfObjects(uint32_t n, int script) {
    SetNumberOfObjects<PedFns>(&g_pedEvents, g_pedEventsOwner, n, script);
}

// ---- prop events (4 per prop)

void __fastcall PropEntry_ClearForScript(EventEntry<4>* self, re::Edx, const int* script) {
    EntryClearForScript(self, script);
}
ScriptRef* __cdecl PropEvents_Slot(int32_t prop, int* event) {
    return RegisterSlot(&g_propEvents, prop, event);
}
uint8_t __cdecl PropEvents_IsHandled(int32_t prop, uint32_t* event) {
    return IsHandled(&g_propEvents, prop, event);
}
void __fastcall PropVec_RemoveAt(PropVec* self, re::Edx, uint32_t i) {
    VecRemoveAt(self, i);
}
void __fastcall PropVec_Reserve(PropVec* self, re::Edx, uint32_t n) {
    VecReserve(self, n, PropFns::EntryCtor);
}
// (no lock checked; the entry's clearing inlined)
void __cdecl PropEvents_Unregister(int32_t prop, uint32_t* event) {
    uint32_t i = g_propEvents.Find(prop);
    if (i >= g_propEvents.count) {
        return;
    }
    EntryClearCallback(g_propEvents.At(i), *event);
    if (EntryInUse(g_propEvents.At(i))) {
        return;
    }
    CallRemoveAt(PropFns::RemoveAt, &g_propEvents, i);
}
// 0x5B4C90: the prop's entry removed (its handlers not released)
void __cdecl PropEvents_Remove(int32_t prop) {
    uint32_t i = g_propEvents.Find(prop);
    if (i < g_propEvents.count) {
        CallRemoveAt(PropFns::RemoveAt, &g_propEvents, i);
    }
}
PropVec* __fastcall PropVec_Construct(PropVec* self, re::Edx) {
    return VecConstruct(self);
}
void __fastcall PropVec_Destruct(PropVec* self, re::Edx) {
    VecDestruct(self);
}
void __fastcall PropVec_Clear(PropVec* self, re::Edx) {
    VecClear(self);
}
void __cdecl PropEvents_ClearForScript(int* script) {
    if (*script == g_propEventsOwner) {
        CallVoid(PropFns::Clear, &g_propEvents);
        g_propEventsOwner = -1;
        return;
    }
    ClearVecForScript<PropFns>(&g_propEvents, script);
}
void __cdecl PropEvents_Clear() {
    CallVoid(PropFns::Clear, &g_propEvents);
}
void __fastcall PropVec_PushBack(PropVec* self, re::Edx, EventEntry<4>* entry) {
    VecPushBack<PropFns>(self, entry);
}
void __cdecl PropEvents_Prepare(int32_t prop) {
    Prepare<PropFns>(&g_propEvents, prop);
}
void __cdecl PropEvents_SetNumberOfObjects(uint32_t n, int script) {
    SetNumberOfObjects<PropFns>(&g_propEvents, g_propEventsOwner, n, script);
}

// ---- trigger events (6 per trigger, in one of two vectors)

void __fastcall TriggerEntry_ClearCallback(EventEntry<6>* self, re::Edx, uint32_t index) {
    EntryClearCallback(self, index);
}
void __fastcall TriggerEntry_ClearForScript(EventEntry<6>* self, re::Edx, const int* script) {
    EntryClearForScript(self, script);
}
EventEntry<6>* __fastcall TriggerVec_At(TriggerVec* self, re::Edx, uint32_t i) {
    return self->At(i);
}
ScriptRef* __cdecl TriggerEvents_Slot(int32_t trigger, int* event, int which) {
    return RegisterSlot(TriggerVecOf(which), trigger, event);
}
uint8_t __cdecl TriggerEvents_IsHandled(int32_t trigger, uint32_t* event, int which) {
    return IsHandled(TriggerVecOf(which), trigger, event);
}
// 0x5B5360: entry i's key (0 past the end)
int32_t __cdecl TriggerEvents_KeyAt(uint32_t i, int which) {
    TriggerVec* vec = TriggerVecOf(which);
    if (i < vec->count) {
        return vec->At(i)->key;
    }
    return 0;
}
EventEntry<6>* CallTriggerVecAt(TriggerVec* vec, uint32_t i) {
    return reinterpret_cast<EventEntry<6>*(__thiscall*)(TriggerVec*, uint32_t)>(TriggerFns::VecAt)(vec, i);
}
// 0x5B53E0: entry i has a handler other than event 2's
uint8_t __cdecl TriggerEvents_HasHandler(uint32_t i, int which) {
    TriggerVec* vec = TriggerVecOf(which);
    EventEntry<6>* e = vec->At(i);
    if (e->key != 0 && Ref_IsSet(&e->callbacks[0]) != 0) {
        return 1;
    }
    e = vec->At(i);
    if (e->key != 0 && Ref_IsSet(&e->callbacks[1]) != 0) {
        return 1;
    }
    for (int j = 3; j < 6; j++) {
        e = CallTriggerVecAt(vec, i);
        if (e->key != 0 && Ref_IsSet(&e->callbacks[j]) != 0) {
            return 1;
        }
    }
    return 0;
}
// 0x5B55A0: entry i has event 2's handler
uint8_t __cdecl TriggerEvents_HasHandler2(uint32_t i, int which) {
    EventEntry<6>* e = TriggerVecOf(which)->At(i);
    if (e->key != 0 && Ref_IsSet(&e->callbacks[2]) != 0) {
        return 1;
    }
    return 0;
}
void __fastcall TriggerVec_RemoveAt(TriggerVec* self, re::Edx, uint32_t i) {
    VecRemoveAt(self, i);
}
void __fastcall TriggerVec_Reserve(TriggerVec* self, re::Edx, uint32_t n) {
    VecReserve(self, n, TriggerFns::EntryCtor);
}
void __cdecl TriggerEvents_Unregister(int32_t trigger, int* event, int which) {
    if (g_triggerEventsLocked == 0) {
        Unregister<TriggerFns>(TriggerVecOf(which), trigger, event);
    }
}
// 0x5B5A30: the trigger's entries removed from both (their handlers not released)
void __cdecl TriggerEvents_Remove(int32_t trigger) {
    uint32_t i = g_triggerEvents0.Find(trigger);
    if (i < g_triggerEvents0.count) {
        CallRemoveAt(TriggerFns::RemoveAt, &g_triggerEvents0, i);
    }
    i = g_triggerEvents1.Find(trigger);
    if (i < g_triggerEvents1.count) {
        CallRemoveAt(TriggerFns::RemoveAt, &g_triggerEvents1, i);
    }
}
TriggerVec* __fastcall TriggerVec_Construct(TriggerVec* self, re::Edx) {
    return VecConstruct(self);
}
void __fastcall TriggerVec_Destruct(TriggerVec* self, re::Edx) {
    VecDestruct(self);
}
void __fastcall TriggerVec_Clear(TriggerVec* self, re::Edx) {
    VecClear(self);
}
void __cdecl TriggerEvents_ClearForScript(int* script) {
    if (*script == g_triggerEventsOwner0) {
        CallVoid(TriggerFns::Clear, &g_triggerEvents0);
        g_triggerEventsOwner0 = -1;
    } else {
        ClearVecForScript<TriggerFns>(&g_triggerEvents0, script);
    }
    if (*script == g_triggerEventsOwner1) {
        CallVoid(TriggerFns::Clear, &g_triggerEvents1);
        g_triggerEventsOwner1 = -1;
        return;
    }
    ClearVecForScript<TriggerFns>(&g_triggerEvents1, script);
}
void __cdecl TriggerEvents_Clear() {
    CallVoid(TriggerFns::Clear, &g_triggerEvents0);
    CallVoid(TriggerFns::Clear, &g_triggerEvents1);
}
void __fastcall TriggerVec_PushBack(TriggerVec* self, re::Edx, EventEntry<6>* entry) {
    VecPushBack<TriggerFns>(self, entry);
}
void __cdecl TriggerEvents_Prepare(int32_t trigger, int which) {
    Prepare<TriggerFns>(TriggerVecOf(which), trigger);
}
void __cdecl TriggerEvents_SetNumberOfObjects(uint32_t n, int script, int which) {
    if (which == 0) {
        SetNumberOfObjects<TriggerFns>(&g_triggerEvents0, g_triggerEventsOwner0, n, script);
        return;
    }
    SetNumberOfObjects<TriggerFns>(&g_triggerEvents1, g_triggerEventsOwner1, n, script);
}

// 0x5B3B80, 0x5B4320, 0x5B4A40, 0x5B56F0 (thiscall, the vector constructor's): an entry cleared,
// its key and callbacks (eax 0; 0x5B4320 leaves it as it was)
template <int N>
uint32_t __fastcall EventEntry_Construct(EventEntry<N>* self, re::Edx) {
    uint32_t* words = reinterpret_cast<uint32_t*>(self);
    for (int i = 0; i < 1 + N; i++) {
        words[i] = 0;
    }
    return 0;
}

} // namespace

#define SCRIPTEVENTS_REVERSED(name, address, fn)                                    \
    static void* RE_CONCAT(Original_SE_, address);                                  \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &RE_CONCAT(Original_SE_, address), \
             re::Mode::Original)

SCRIPTEVENTS_REVERSED("ScriptEvents::GlobalSlot_5B1DC0", 0x005B1DC0, GlobalEvents_Slot);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashEntryCtor_5B3B80", 0x005B3B80, EventEntry_Construct<8>);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedEntryCtor_5B4320", 0x005B4320, EventEntry_Construct<1>);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropEntryCtor_5B4A40", 0x005B4A40, EventEntry_Construct<4>);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerEntryCtor_5B56F0", 0x005B56F0, EventEntry_Construct<6>);
SCRIPTEVENTS_REVERSED("ScriptEvents::GlobalUnregister_5B1DF0", 0x005B1DF0, GlobalEvents_Unregister);
SCRIPTEVENTS_REVERSED("ScriptEvents::GlobalIsHandled_5B1E30", 0x005B1E30, GlobalEvents_IsHandled);
SCRIPTEVENTS_REVERSED("ScriptEvents::GlobalClearForScript_5B1E60", 0x005B1E60, GlobalEvents_ClearForScript);
SCRIPTEVENTS_REVERSED("ScriptEvents::GlobalDestruct_5B1EC0", 0x005B1EC0, GlobalEvents_Destruct);
SCRIPTEVENTS_REVERSED("ScriptEvents::CallTriggerPed_5B1F10", 0x005B1F10, CallTriggerPedHandler);
SCRIPTEVENTS_REVERSED("ScriptEvents::GlobalTriggerPed_5B1F80", 0x005B1F80, GlobalEvents_TriggerPed);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerTriggerPed_5B2020", 0x005B2020, TriggerEvents_TriggerPed);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerEvent2_5B20D0", 0x005B20D0, TriggerEvents_Event2);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerEvent5_5B22A0", 0x005B22A0, TriggerEvents_Event5);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedEvent0_5B22F0", 0x005B22F0, PedEvents_Event0);
SCRIPTEVENTS_REVERSED("ScriptEvents::GlobalSpeech_5B25E0", 0x005B25E0, GlobalEvents_Speech);
SCRIPTEVENTS_REVERSED("ScriptEvents::ClearForScript_5B27E0", 0x005B27E0, ScriptEvents_ClearForScript);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerEnter_5B2810", 0x005B2810, TriggerEvents_Enter);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerExit_5B28D0", 0x005B28D0, TriggerEvents_Exit);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropHash2_5B2990", 0x005B2990, PropEvents_Hash2);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropBroken_5B2AD0", 0x005B2AD0, PropEvents_Broken);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropEvent1_5B2D10", 0x005B2D10, PropEvents_Event1);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropOpen_5B2F50", 0x005B2F50, PropEvents_Open);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropClose_5B3190", 0x005B3190, PropEvents_Close);
SCRIPTEVENTS_REVERSED("ScriptEvents::GlobalModel_5B33D0", 0x005B33D0, GlobalEvents_Model);
SCRIPTEVENTS_REVERSED("ScriptEvents::ObjectEvent0_5B34B0", 0x005B34B0, ObjectEvents_Event0);
SCRIPTEVENTS_REVERSED("ScriptEvents::ObjectEvent3_5B3690", 0x005B3690, ObjectEvents_Event3);

SCRIPTEVENTS_REVERSED("ScriptEvents::HashEntryClearCallback_5B3890", 0x005B3890, HashEntry_ClearCallback);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashEntryClearForScript_5B38E0", 0x005B38E0, HashEntry_ClearForScript);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashEntryMove_5B3930", 0x005B3930, HashEntry_Move);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashSlot_5B3990", 0x005B3990, HashEvents_Slot);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashIsHandled_5B3A10", 0x005B3A10, HashEvents_IsHandled);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashRemoveAt_5B3A90", 0x005B3A90, HashVec_RemoveAt);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashReserve_5B3BA0", 0x005B3BA0, HashVec_Reserve);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashUnregister_5B3CE0", 0x005B3CE0, HashEvents_Unregister);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashConstruct_5B3DB0", 0x005B3DB0, HashVec_Construct);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashDestruct_5B3E10", 0x005B3E10, HashVec_Destruct);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashVecClear_5B3E90", 0x005B3E90, HashVec_Clear);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashClearForScript_5B3F00", 0x005B3F00, HashEvents_ClearForScript);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashClear_5B3FB0", 0x005B3FB0, HashEvents_Clear);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashPushBack_5B3FC0", 0x005B3FC0, HashVec_PushBack);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashPrepare_5B4040", 0x005B4040, HashEvents_Prepare);
SCRIPTEVENTS_REVERSED("ScriptEvents::HashSetNumber_5B40F0", 0x005B40F0, HashEvents_SetNumberOfObjects);

SCRIPTEVENTS_REVERSED("ScriptEvents::PedEntryClearCallback_5B4140", 0x005B4140, PedEntry_ClearCallback);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedSlot_5B4190", 0x005B4190, PedEvents_Slot);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedIsHandled_5B4210", 0x005B4210, PedEvents_IsHandled);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedRemoveAt_5B4290", 0x005B4290, PedVec_RemoveAt);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedReserve_5B4330", 0x005B4330, PedVec_Reserve);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedUnregister_5B4430", 0x005B4430, PedEvents_Unregister);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedConstruct_5B44F0", 0x005B44F0, PedVec_Construct);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedDestruct_5B4540", 0x005B4540, PedVec_Destruct);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedVecClear_5B45B0", 0x005B45B0, PedVec_Clear);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedClearForScript_5B4610", 0x005B4610, PedEvents_ClearForScript);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedClear_5B46E0", 0x005B46E0, PedEvents_Clear);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedPushBack_5B46F0", 0x005B46F0, PedVec_PushBack);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedPrepare_5B4760", 0x005B4760, PedEvents_Prepare);
SCRIPTEVENTS_REVERSED("ScriptEvents::PedSetNumber_5B47E0", 0x005B47E0, PedEvents_SetNumberOfObjects);

SCRIPTEVENTS_REVERSED("ScriptEvents::PropEntryClearForScript_5B4830", 0x005B4830, PropEntry_ClearForScript);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropSlot_5B4880", 0x005B4880, PropEvents_Slot);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropIsHandled_5B4900", 0x005B4900, PropEvents_IsHandled);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropRemoveAt_5B4980", 0x005B4980, PropVec_RemoveAt);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropReserve_5B4A60", 0x005B4A60, PropVec_Reserve);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropUnregister_5B4BA0", 0x005B4BA0, PropEvents_Unregister);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropRemove_5B4C90", 0x005B4C90, PropEvents_Remove);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropConstruct_5B4CF0", 0x005B4CF0, PropVec_Construct);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropDestruct_5B4D50", 0x005B4D50, PropVec_Destruct);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropVecClear_5B4DD0", 0x005B4DD0, PropVec_Clear);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropClearForScript_5B4E40", 0x005B4E40, PropEvents_ClearForScript);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropClear_5B4EF0", 0x005B4EF0, PropEvents_Clear);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropPushBack_5B4F00", 0x005B4F00, PropVec_PushBack);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropPrepare_5B4F90", 0x005B4F90, PropEvents_Prepare);
SCRIPTEVENTS_REVERSED("ScriptEvents::PropSetNumber_5B5030", 0x005B5030, PropEvents_SetNumberOfObjects);

SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerEntryClearCallback_5B5080", 0x005B5080, TriggerEntry_ClearCallback);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerEntryClearForScript_5B50D0", 0x005B50D0, TriggerEntry_ClearForScript);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerVecAt_5B5120", 0x005B5120, TriggerVec_At);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerSlot_5B5150", 0x005B5150, TriggerEvents_Slot);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerIsHandled_5B5260", 0x005B5260, TriggerEvents_IsHandled);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerKeyAt_5B5360", 0x005B5360, TriggerEvents_KeyAt);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerHasHandler_5B53E0", 0x005B53E0, TriggerEvents_HasHandler);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerHasHandler2_5B55A0", 0x005B55A0, TriggerEvents_HasHandler2);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerRemoveAt_5B5620", 0x005B5620, TriggerVec_RemoveAt);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerReserve_5B5710", 0x005B5710, TriggerVec_Reserve);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerUnregister_5B5850", 0x005B5850, TriggerEvents_Unregister);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerRemove_5B5A30", 0x005B5A30, TriggerEvents_Remove);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerConstruct_5B5AD0", 0x005B5AD0, TriggerVec_Construct);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerDestruct_5B5B30", 0x005B5B30, TriggerVec_Destruct);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerVecClear_5B5BC0", 0x005B5BC0, TriggerVec_Clear);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerClearForScript_5B5C40", 0x005B5C40, TriggerEvents_ClearForScript);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerClear_5B5DA0", 0x005B5DA0, TriggerEvents_Clear);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerPushBack_5B5DC0", 0x005B5DC0, TriggerVec_PushBack);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerPrepare_5B5E70", 0x005B5E70, TriggerEvents_Prepare);
SCRIPTEVENTS_REVERSED("ScriptEvents::TriggerSetNumber_5B5FB0", 0x005B5FB0, TriggerEvents_SetNumberOfObjects);
