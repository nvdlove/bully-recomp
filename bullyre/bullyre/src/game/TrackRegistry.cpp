// The action tree's text tokenizer (0x61A310..0x61A5C0: the Act files' paths and the
// object data's fields), the track / condition registries (name -> factory, 0x61A610..
// 0x61A870) and the tracks' and conditions' base constructors / destructor.

#include <cstdint>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

// A tokenizer over a text: +0 the cursor, +4 up to eight separators (a NUL among them
// when fewer), +0xC the start, +0x10 the saved cursors' count, +0x14 the saved cursors
struct Tokenizer {
    re::Ptr32<const char> cursor;   // +0x0
    char seps[8];         // +0x4
    re::Ptr32<const char> start;    // +0xC
    int32_t depth;        // +0x10
    re::Ptr32<const char> saved[1]; // +0x14 (as many as pushed)
};
static_assert(offsetof(Tokenizer, saved) == 0x14);

// A registry of factories by name key: +4 the entries, +8 capacity, +0xC count; an entry:
// +0 the key, +4 its track factory, +8 its condition factory
struct Registry {
    uint32_t vtable;    // +0x0
    re::Ptr32<re::Ptr32<uint8_t>> entries;  // +0x4
    uint32_t capacity;  // +0x8
    uint32_t count;     // +0xC
};
static_assert(sizeof(Registry) == 0x10);

constexpr uint32_t kVtTrackOrCondition = 0x00906418;
constexpr uint32_t kVtTrack = 0x00906484;
constexpr uint32_t kVtRegistry = 0x009332B4;
inline Registry* const trackRegistry = reinterpret_cast<Registry*>(0x01B6565C);

inline int __cdecl ToLower(int c) { return crt::Tolower(c); }
inline void* __cdecl Memcpy(void* dst, const void* src, uint32_t size) { return crt::Memcpy(dst, src, (size_t)size); }
inline int32_t __cdecl Atol(const char* text) { return (int32_t)crt::Atol(text); }
inline double __cdecl Atof(const char* text) { return crt::Atof(text); }
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __thiscall, PtrArray_Reserve, 0x006DB480, void* array, uint32_t size, int);
GAME_FN(int, __thiscall, PtrArray_RemoveAt, 0x006DC540, void* array, uint32_t index);
// the tokenizer's and registry's own (by address, so their modes apply)
GAME_FN(uint8_t, __thiscall, Tokenizer_Is, 0x0061A310, Tokenizer* tok, const char* token);
GAME_FN(void, __thiscall, Tokenizer_Next, 0x0061A4D0, Tokenizer* tok);
GAME_FN(char*, __thiscall, Tokenizer_Copy, 0x0061A3B0, Tokenizer* tok, const char* from, char* out, uint32_t size,
        int stop, int useSeps);
GAME_FN(void, __thiscall, Registry_Clear, 0x0061A780, Registry* registry);
GAME_FN(uint8_t*, __thiscall, TrackOrCondition_Construct, 0x0061A600, uint8_t* self);

bool IsSep(const Tokenizer* tok, char c) {
    for (uint32_t i = 0; i < 8; i++) {
        if (c == tok->seps[i]) {
            return true;
        }
    }
    return false;
}

// 0x61A310 (thiscall): the text at the cursor is token (a separator after it)
uint8_t __fastcall Tokenizer_IsImpl(Tokenizer* self, re::Edx, const char* token) {
    const char* p = self->cursor;
    char c = *token;
    if (c != 0) {
        const char* t = token;
        do {
            if (c != *p) {
                return 0;
            }
            c = *++t;
            p++;
        } while (c != 0);
    }
    return IsSep(self, *p) ? 1 : 0;
}

// 0x61A350 (thiscall): the same, case blind
uint8_t __fastcall Tokenizer_IsNoCase(Tokenizer* self, re::Edx, const char* token) {
    const char* p = self->cursor;
    while (*token != 0) {
        int a = ToLower(*p);
        if (ToLower(*token) != a) {
            return 0;
        }
        token++;
        p++;
    }
    return IsSep(self, *p) ? 1 : 0;
}

// 0x61A3B0 (thiscall): the text from `from` up to stop (or a separator with useSeps)
// copied out; null when it didn't fit (cut to size - 1)
char* __fastcall Tokenizer_CopyImpl(Tokenizer* self, re::Edx, const char* from, char* out, uint32_t size, int stop,
                                    int useSeps) {
    const char* p = from;
    char c = *p;
    bool atStop = c == char(stop);
    bool atSep = uint8_t(useSeps) != 0 && IsSep(self, c);
    while (!atStop && !atSep) {
        c = *++p;
        atStop = c == char(stop);
        atSep = uint8_t(useSeps) != 0 && IsSep(self, c);
    }
    uint32_t length = uint32_t(p - from);
    char* result = out;
    if (length >= size) {
        result = nullptr;
        length = size - 1;
    }
    Memcpy(out, from, length);
    out[length] = 0;
    return result;
}

// 0x61A450 / 0x61A460 (thiscall): the cursor saved / restored
void __fastcall Tokenizer_Push(Tokenizer* self, re::Edx) {
    self->saved[self->depth] = self->cursor;
    self->depth++;
}
void __fastcall Tokenizer_Pop(Tokenizer* self, re::Edx) {
    self->depth--;
    self->cursor = self->saved[self->depth];
}

// 0x61A470 (thiscall): over text with the separators seps (at most 8 with their NUL),
// leading separators skipped
Tokenizer* __fastcall Tokenizer_Construct(Tokenizer* self, re::Edx, const char* text, const char* seps) {
    self->cursor = text;
    self->start = text;
    self->depth = 0;
    for (int i = 0; i < 8; i++) {
        self->seps[i] = 0;
    }
    char* to = self->seps;
    char c;
    do {
        c = *seps++;
        *to++ = c;
    } while (c != 0);
    for (;;) {
        char at = *self->cursor;
        if (!IsSep(self, at) || at == 0) {
            return self;
        }
        self->cursor++;
    }
}

// the cursor past separators (stops at the text's end)
void SkipSeps(Tokenizer* self) {
    for (;;) {
        char at = *self->cursor;
        if (!IsSep(self, at) || at == 0) {
            return;
        }
        self->cursor++;
    }
}

// 0x61A4D0 (thiscall): the cursor on to the next token
void __fastcall Tokenizer_NextImpl(Tokenizer* self, re::Edx) {
    if (IsSep(self, *self->cursor)) {
        SkipSeps(self);
        return;
    }
    for (;;) {
        char at = *self->cursor;
        if (IsSep(self, at) || at == 0) {
            break;
        }
        self->cursor++;
    }
    SkipSeps(self);
}

// 0x61A560 (thiscall): on to token (or the end). (The game keeps ecx across 0x61A310 and
// 0x61A4D0: turn this on with them.)
void __fastcall Tokenizer_SkipTo(Tokenizer* self, re::Edx, const char* token) {
    while (*self->cursor != 0 && !Tokenizer_Is(self, token)) {
        Tokenizer_Next(self);
    }
}

// 0x61A590 / 0x61A5C0 (thiscall): the token at the cursor as a number (0 when too long)
int32_t __fastcall Tokenizer_GetInt(Tokenizer* self, re::Edx) {
    char text[16];
    if (Tokenizer_Copy(self, self->cursor, text, 0x10, 0, 1)) {
        return Atol(text);
    }
    return 0;
}
float __fastcall Tokenizer_GetFloat(Tokenizer* self, re::Edx) {
    char text[16];
    if (Tokenizer_Copy(self, self->cursor, text, 0x10, 0, 1)) {
        return float(Atof(text));
    }
    return 0.0f;
}

// 0x61A600 (thiscall): TrackOrCondition's constructor
uint8_t* __fastcall TrackOrCondition_ConstructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtTrackOrCondition;
    return self;
}

// 0x61A610 (thiscall): a track made by the factory registered for the name (null: none)
uint8_t* __fastcall Registry_CreateTrack(Registry* self, re::Edx, const uint32_t* name) {
    for (int32_t i = 0; i < int32_t(self->count); i++) {
        uint8_t* entry = self->entries[i];
        if (*name == At<uint32_t>(entry, 0)) {
            // (called with ecx at the entry's slot, as the game does)
            return re::PtrCast<uint8_t*(__thiscall*)(re::Ptr32<uint8_t>*)>(At<void*>(entry, 4))(&self->entries[i]);
        }
    }
    return nullptr;
}
// 0x61A650 (thiscall): a condition made from data by the factory registered for the name
uint8_t* __fastcall Registry_CreateCondition(Registry* self, re::Edx, const uint32_t* name, uint8_t* data) {
    for (int32_t i = 0; i < int32_t(self->count); i++) {
        uint8_t* entry = self->entries[i];
        if (*name == At<uint32_t>(entry, 0)) {
            return re::PtrCast<uint8_t*(__cdecl*)(uint8_t*)>(At<void*>(entry, 8))(data);
        }
    }
    return nullptr;
}
// 0x61A690 (thiscall): room for 800 entries
void __fastcall Registry_Reserve(Registry* self, re::Edx) {
    if (self->capacity >= 800) {
        return;
    }
    self->capacity = 800;
    re::Ptr32<uint8_t>* entries = static_cast<re::Ptr32<uint8_t>*>(Malloc(800 * 4));
    if (self->entries) {
        for (uint32_t i = 0; i < self->count; i++) {
            entries[i] = self->entries[i];
        }
        Free(self->entries);
    }
    self->entries = entries;
}
// 0x61A700 (thiscall): an entry added (the list doubled when full)
void __fastcall Registry_Add(Registry* self, re::Edx, uint8_t* entry) {
    if (self->count == self->capacity) {
        PtrArray_Reserve(&self->entries, self->capacity ? self->capacity * 2 : 1, 0);
    }
    self->entries[self->count] = entry;
    self->count++;
}
// 0x61A740 (thiscall): an entry removed (eax: the list's result, or the count when not
// there)
int __fastcall Registry_Remove(Registry* self, re::Edx, uint8_t* entry) {
    uint32_t i = 0;
    for (; i < self->count; i++) {
        if (self->entries[i] == entry) {
            if (i == uint32_t(-1)) {
                return int(i);
            }
            return PtrArray_RemoveAt(&self->entries, i);
        }
    }
    return int(i);
}
// 0x61A780 (thiscall): every entry freed, and the list
void __fastcall Registry_ClearImpl(Registry* self, re::Edx) {
    int32_t count = int32_t(self->count);
    for (int32_t i = 0; i < count; i++) {
        uint8_t* entry = self->entries[i];
        if (entry) {
            NameKey_Destruct(entry);
            Free(entry);
        }
    }
    if (self->entries) {
        Free(self->entries);
        self->entries = nullptr;
    }
    self->count = 0;
    self->capacity = 0;
}
// 0x61A7E0 (thiscall): the registry's constructor
Registry* __fastcall Registry_Construct(Registry* self, re::Edx) {
    self->vtable = kVtRegistry;
    self->entries = nullptr;
    self->capacity = 0;
    self->count = 0;
    return self;
}
// 0x61A800 (thiscall): its destructor
void __fastcall Registry_Destruct(Registry* self, re::Edx) {
    self->vtable = kVtRegistry;
    Registry_Clear(self);
    if (self->entries) {
        Free(self->entries);
        self->entries = nullptr;
    }
}
// 0x61A830 (thiscall): its deleting destructor
Registry* __fastcall Registry_DeletingDestruct(Registry* self, re::Edx, uint32_t flags) {
    self->vtable = kVtRegistry;
    Registry_Clear(self);
    if (self->entries) {
        Free(self->entries);
        self->entries = nullptr;
    }
    if (flags & 1) {
        Free(self);
    }
    return self;
}
// 0x61A870 (cdecl): the track registry cleared (at shutdown)
void __cdecl TrackRegistry_Shutdown() {
    Registry_Clear(trackRegistry);
}

// 0x61A880 (thiscall): Track's constructor: unlinked, from 0, no end
uint8_t* __fastcall Track_ConstructImpl(uint8_t* self, re::Edx) {
    TrackOrCondition_Construct(self);
    At<uint32_t>(self, 4) = 0;
    At<float>(self, 0xC) = 0.0f;
    At<uint32_t>(self, 0) = kVtTrack;
    At<float>(self, 0x10) = -1.0f;
    return self;
}
// 0x61A8B0 (thiscall): Track's destructor
void __fastcall Track_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtTrack;
}

} // namespace

#define TRREG_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TRREG_REVERSED("Tokenizer::Is", 0x0061A310, &Tokenizer_IsImpl);
TRREG_REVERSED("Tokenizer::IsNoCase", 0x0061A350, &Tokenizer_IsNoCase);
TRREG_REVERSED("Tokenizer::Copy", 0x0061A3B0, &Tokenizer_CopyImpl);
TRREG_REVERSED("Tokenizer::Push", 0x0061A450, &Tokenizer_Push);
TRREG_REVERSED("Tokenizer::Pop", 0x0061A460, &Tokenizer_Pop);
TRREG_REVERSED("Tokenizer::Construct", 0x0061A470, &Tokenizer_Construct);
TRREG_REVERSED("Tokenizer::Next", 0x0061A4D0, &Tokenizer_NextImpl);   // register hazard (regcheck2): 0x61A560 keeps ecx across its call at 0x61A57A: switch with SkipTo
TRREG_REVERSED("Tokenizer::SkipTo", 0x0061A560, &Tokenizer_SkipTo);   // callers: see its comment
TRREG_REVERSED("Tokenizer::GetInt", 0x0061A590, &Tokenizer_GetInt);
TRREG_REVERSED("Tokenizer::GetFloat", 0x0061A5C0, &Tokenizer_GetFloat);
TRREG_REVERSED("TrackOrCondition::Construct", 0x0061A600, &TrackOrCondition_ConstructImpl);
TRREG_REVERSED("TrackRegistry::CreateTrack", 0x0061A610, &Registry_CreateTrack);
TRREG_REVERSED("TrackRegistry::CreateCondition", 0x0061A650, &Registry_CreateCondition);
TRREG_REVERSED("TrackRegistry::Reserve", 0x0061A690, &Registry_Reserve);
TRREG_REVERSED("TrackRegistry::Add", 0x0061A700, &Registry_Add);
TRREG_REVERSED("TrackRegistry::Remove", 0x0061A740, &Registry_Remove);
TRREG_REVERSED("TrackRegistry::Clear", 0x0061A780, &Registry_ClearImpl);
TRREG_REVERSED("TrackRegistry::Construct", 0x0061A7E0, &Registry_Construct);
TRREG_REVERSED("TrackRegistry::Destruct", 0x0061A800, &Registry_Destruct);
TRREG_REVERSED("TrackRegistry::DeletingDestruct", 0x0061A830, &Registry_DeletingDestruct);
TRREG_REVERSED("TrackRegistry::Shutdown_61A870", 0x0061A870, &TrackRegistry_Shutdown);
TRREG_REVERSED("Track::Construct", 0x0061A880, &Track_ConstructImpl);
TRREG_REVERSED("Track::Destruct", 0x0061A8B0, &Track_DestructImpl);
