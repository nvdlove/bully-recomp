// The registry of static NiFixedStrings: the game's global fixed strings are registered
// with their text (up to 100), created together once Gamebryo's string table exists
// and released together at shutdown.

#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace c01fixedstr {

using InterlockedFn = long(__stdcall*)(volatile long*);
GAME_VAR(InterlockedFn, Import_InterlockedDecrement, 0x008FC170);

// Gamebryo's NiFixedString: refcounted (-8) and sized (-2), in a pool
GAME_FN(char*, __cdecl, FixedString_Create, 0x00754760, const char* s);
GAME_FN(void, __thiscall, FixedStringPool_Free, 0x00754550, void* pool, re::Ptr32<char>* string, uint32_t length);
GAME_VAR(void*, fixedStringPool, 0x020E216C);
GAME_VAR(char*, emptyFixedString, 0x00951028);

// (inlined in the game) one reference less on `*string`
void Release(re::Ptr32<char>* string) {
    char* s = *string;
    if (s == emptyFixedString) {
        return;
    }
    uint16_t length;
    std::memcpy(&length, s - 2, 2);
    if (Import_InterlockedDecrement(reinterpret_cast<volatile long*>(s - 8)) == 1) {
        FixedStringPool_Free(fixedStringPool, string, length);
    }
}

struct Entry {
    re::Ptr32<re::Ptr32<char>> string;      // +0 the static
    re::Ptr32<const char> text;   // +4
};
struct Registry {
    Entry entries[100];   // +0
    int32_t count;        // +0x320
};
static_assert(sizeof(Registry) == 0x324);

GAME_VAR(Registry, theRegistry, 0x00BCC168);
GAME_VAR(uint32_t, registryGuard, 0x00BCC48C);   // (the function-local static's guard)
GAME_FN(Registry*, __thiscall, Registry_Construct, 0x004113D0, Registry* self);

// 0x4113D0 (thiscall): the constructor
Registry* __fastcall Construct(Registry* self, re::Edx) {
    for (int i = 0; i < 100; i++) {
        self->entries[i].string = nullptr;
        self->entries[i].text = nullptr;
    }
    self->count = 0;
    return self;
}

// (inlined everywhere: the static constructed on first use)
Registry& Get() {
    if (!(registryGuard & 1)) {
        registryGuard |= 1;
        for (int i = 0; i < 100; i++) {
            theRegistry.entries[i].string = nullptr;
            theRegistry.entries[i].text = nullptr;
        }
        theRegistry.count = 0;
    }
    return theRegistry;
}

// 0x411410: each static set to its text (unless it's that string already)
void __cdecl CreateAll() {
    for (int i = 0; i < Get().count; i++) {
        re::Ptr32<char>* string = Get().entries[i].string;
        re::Ptr32<char> old = *string;
        if (old != Get().entries[i].text) {
            *string = FixedString_Create(Get().entries[i].text);
            Release(&old);
        }
    }
}

// 0x411520: each static released (set to the null string) and the registry emptied
void __cdecl ReleaseAll() {
    for (int i = 0; i < Get().count; i++) {
        re::Ptr32<char>* string = Get().entries[i].string;
        re::Ptr32<char> old = *string;
        if (old) {
            *string = FixedString_Create(nullptr);
            Release(&old);
        }
    }
    Get().count = 0;
}

// 0x411620: registers a static with its text
void __cdecl Register(re::Ptr32<char>* string, const char* text) {
    if (!(registryGuard & 1)) {
        registryGuard |= 1;
        Registry_Construct(&theRegistry);
    }
    theRegistry.entries[theRegistry.count].string = string;
    theRegistry.entries[theRegistry.count].text = text;
    theRegistry.count++;
}

} // namespace c01fixedstr

// ---- registration ----------------------------------------------------------------------

#define FIXEDSTR_REVERSED_C01(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01fixedstr {
FIXEDSTR_REVERSED_C01("FixedStringRegistry::FixedStringRegistry", 0x004113D0, &Construct);
FIXEDSTR_REVERSED_C01("FixedStringRegistry::CreateAll", 0x00411410, &CreateAll);
FIXEDSTR_REVERSED_C01("FixedStringRegistry::ReleaseAll", 0x00411520, &ReleaseAll);
FIXEDSTR_REVERSED_C01("FixedStringRegistry::Register", 0x00411620, &Register);
} // namespace c01fixedstr
