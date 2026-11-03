#pragma once

// The shapes the action tree's hundreds of small class members share (0x630E80..0x64B1F0:
// ConditionFactories.cpp, TrackFactories.cpp, ActionDestructors.cpp): each one is a
// straight run of steps on the object, written once here as templates and instanced
// with its steps per class:
//   CreateAt<steps...>      (cdecl) a registry's factory: the object built in place in
//                           the given memory (null: none); the condition registry's +8,
//                           the track registry's
//   Construct<steps...>     (thiscall) a constructor: the steps, then itself
//   DeletingDtor<steps...>  (thiscall) a deleting destructor: the steps, freed on flag 1
//   New<size, steps...>     (cdecl) the condition registry's +4: allocated, then built
//                           (the steps after Then run on a failed allocation too)
//   NewVia / CreateVia      the same through the class's own constructor
//
// A step: a vtable set, a thiscall on the object or one of its members (a base or
// member constructor / destructor), or a field set to a constant.

#include <bit>
#include <cstdint>

#include "reversed.h"

namespace actfac {

// (by integer: the condition factories' steps after the construction also run on a
// failed allocation, at null + offset, as the game's do)
using re::At;

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __thiscall, EntityRef_Set, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(void, __cdecl, Entity_CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, NameKey_FromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(void, __thiscall, StringRef_Assign, 0x005772F0, uint32_t* field, uint32_t value);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);

enum StepKind : uint8_t {
    kVt,          // +0 = a
    kCall,        // thiscall a(object + b)
    kF32,         // float at a = b's bits
    kI32,         // dword at a = b
    kU16,         // word at a = b
    kU8,          // byte at a = b
    kRefSet0,     // the entity reference at a set to none (0x5D7F10)
    kRefRelease,  // the entity reference at a released when set (0x4657E0), not cleared
    kClearName,   // the name key at a set to none through a temporary key
    kVCall,       // its own virtual at a (a byte offset in its vtable), thiscall
    kThen,        // New: the steps after this one run whether the allocation worked or not
};
struct Step {
    StepKind kind;
    uint32_t a;
    uint32_t b;
};

constexpr Step Vt(uint32_t vtable) { return {kVt, vtable, 0}; }
// thiscall fn(object + at), its result unused
constexpr Step Call(uint32_t fn, uint32_t at = 0) { return {kCall, fn, at}; }
constexpr Step Zero(uint32_t at) { return {kF32, at, 0}; }
constexpr Step One(uint32_t at) { return {kF32, at, std::bit_cast<uint32_t>(1.0f)}; }
constexpr Step F32(uint32_t at, float value) { return {kF32, at, std::bit_cast<uint32_t>(value)}; }
constexpr Step I32(uint32_t at, uint32_t value) { return {kI32, at, value}; }
constexpr Step U16(uint32_t at, uint32_t value) { return {kU16, at, value}; }
constexpr Step U8(uint32_t at, uint32_t value) { return {kU8, at, value}; }
constexpr Step RefSet0(uint32_t at) { return {kRefSet0, at, 0}; }
constexpr Step RefRelease(uint32_t at) { return {kRefRelease, at, 0}; }
constexpr Step ClearName(uint32_t at) { return {kClearName, at, 0}; }
constexpr Step VCall(uint32_t at) { return {kVCall, at, 0}; }
inline constexpr Step Then = {kThen, 0, 0};

// the common calls
inline constexpr Step TrackBase = Call(0x0061A880);      // Track's constructor
inline constexpr Step CondBase = Call(0x0061A600);       // TrackOrCondition's constructor
inline constexpr Step TrackBaseDtor = Call(0x0061A8B0);  // Track's destructor
constexpr Step Key(uint32_t at) { return Call(0x0066BEA0, at); }      // a name key, empty
constexpr Step KeyDtor(uint32_t at) { return Call(0x0049A650, at); }  // a name key's destructor (empty)
constexpr Step NodeRef(uint32_t at) { return Call(0x005F53E0, at); }  // an ActionNodeReference

template <Step S>
inline void Do(uint8_t* self) {
    if constexpr (S.kind == kVt) {
        At<uint32_t>(self, 0) = S.a;
    } else if constexpr (S.kind == kCall) {
        reinterpret_cast<void(__thiscall*)(uint8_t*)>(S.a)(&At<uint8_t>(self, S.b));
    } else if constexpr (S.kind == kF32) {
        At<float>(self, S.a) = std::bit_cast<float>(S.b);
    } else if constexpr (S.kind == kI32) {
        At<uint32_t>(self, S.a) = S.b;
    } else if constexpr (S.kind == kU16) {
        At<uint16_t>(self, S.a) = uint16_t(S.b);
    } else if constexpr (S.kind == kU8) {
        At<uint8_t>(self, S.a) = uint8_t(S.b);
    } else if constexpr (S.kind == kRefSet0) {
        EntityRef_Set(&At<uint8_t*>(self, S.a), nullptr);
    } else if constexpr (S.kind == kRefRelease) {
        re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, S.a);
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
        }
    } else if constexpr (S.kind == kVCall) {
        uintptr_t fn = At<uint32_t>(reinterpret_cast<uint8_t*>(At<uint32_t>(self, 0)), S.a);
        reinterpret_cast<void(__thiscall*)(uint8_t*)>(fn)(self);
    } else if constexpr (S.kind == kClearName) {
        uint32_t name;
        NameKey_FromValue(&name, 0);
        StringRef_Assign(&At<uint32_t>(self, S.a), name);
        NameKey_Destruct(&name);
    }
}

template <Step... S>
uint8_t* __cdecl CreateAt(uint8_t* memory) {
    if (!memory) {
        return nullptr;
    }
    (Do<S>(memory), ...);
    return memory;
}

template <Step... S>
uint8_t* __fastcall Construct(uint8_t* self, re::Edx) {
    (Do<S>(self), ...);
    return self;
}

// The same steps for the game's functions that return nothing: field initializers and
// non-deleting destructors (the side-by-side sweep showed their eax is the caller's or a
// leftover zero, never `this`)
template <Step... S>
void __fastcall Fields(uint8_t* self, re::Edx) {
    (Do<S>(self), ...);
}

template <Step... S>
uint8_t* __fastcall DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    (Do<S>(self), ...);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

template <Step S>
inline void DoNew(uint8_t* self, bool& after) {
    if constexpr (S.kind == kThen) {
        after = true;
    } else if (self || after) {
        Do<S>(self);
    }
}
template <uint32_t Size, Step... S>
uint8_t* __cdecl New() {
    uint8_t* self = static_cast<uint8_t*>(Malloc(Size));
    bool after = false;
    (DoNew<S>(self, after), ...);
    return self;
}

// the same through the class's own constructor (thiscall, returning the object), then a
// thiscall on the result (null too)
template <uint32_t Size, uintptr_t Ctor, uintptr_t After>
uint8_t* __cdecl NewVia() {
    void* memory = Malloc(Size);
    uint8_t* self = memory ? reinterpret_cast<uint8_t*(__thiscall*)(void*)>(Ctor)(memory) : nullptr;
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(After)(self);
    return self;
}
template <uintptr_t Ctor>
uint8_t* __cdecl CreateVia(uint8_t* memory) {
    if (!memory) {
        return nullptr;
    }
    return reinterpret_cast<uint8_t*(__thiscall*)(uint8_t*)>(Ctor)(memory);
}

} // namespace actfac
