// The effect textures (0x5781C0..0x578412): the list of textures the particle effects
// use, from the "EffectTextures" texture dictionary (the object at 0x1B83278,
// *0xB6F1F0), found by name hash; and the particle definitions' texture reference (a
// byte: bit 0 set, the index above).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

// An entry (0xC)
struct EffectTexture {
    uint8_t flags;      // +0 bit 0: used
    uint8_t pad[3];
    uint32_t hash;      // +4 the name's
    re::Ptr32<void> texture;      // +8
};
static_assert(sizeof(EffectTexture) == 0xC);

// The list (0x654)
struct EffectTextures {
    re::Ptr32<char> name;                 // +0 a fixed string: its buffer
    int32_t capacity;           // +4 (0x41)
    char buffer[0x41];          // +8
    uint8_t pad49[3];
    int32_t count;              // +0x4C
    uint8_t loaded;             // +0x50
    uint8_t pad51[3];
    EffectTexture entries[128]; // +0x54
};
static_assert(offsetof(EffectTextures, count) == 0x4C);
static_assert(offsetof(EffectTextures, entries) == 0x54);
static_assert(sizeof(EffectTextures) == 0x654);

// A TxdDef's texture dictionary is its first field
struct TxdDefHead {
    re::Ptr32<void> dictionary;
};

GAME_FN(int, __cdecl, ET_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, ET_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(int, __cdecl, ET_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(uint8_t, __cdecl, ET_LoadTxd, 0x005F2A80, int slot, const char* filename);
GAME_FN(void, __cdecl, ET_AddRef, 0x005F1130, int slot);
GAME_FN(void*, __cdecl, ET_FindTexture, 0x005F1610, void* dictionary, const char* name);
GAME_FN(void, __cdecl, ET_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, ET_PopMemoryTag, 0x005EF100);
inline char* __cdecl ET_Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(uint32_t, __cdecl, ET_HashName, 0x00576ED0, const char* name);
// ours
GAME_FN(int, __thiscall, ET_FindIndex, 0x00578350, EffectTextures* self, uint32_t hash);

// the texture dictionary pool (CTxdStore's): objects, flags, size, stride
struct TxdPool {
    re::Ptr32<uint8_t> objects;  // +0
    re::Ptr32<uint8_t> flags;    // +4
    int32_t size;      // +8
    int32_t stride;    // +0xC
};
GAME_VAR(TxdPool*, g_txdPool, 0x01B64654);
GAME_VAR(EffectTextures*, g_effectTextures, 0x00B6F1F0);
inline const char* const kEffectTxd = reinterpret_cast<const char*>(0x00918568);   // "EffectTextures"
inline const char* const kNotFound = reinterpret_cast<const char*>(0x0091C13C);   // "Effect Texture Not Found in Effects.nft: "

} // namespace

namespace EffectTexturesImpl {

// 0x5781C0 (thiscall): a texture reference cleared
void __fastcall ClearRef(uint8_t* self, re::Edx) {
    *self = 0;
}

// 0x5781D0: the dictionary removed
void __cdecl RemoveTxd() {
    ET_RemoveTxdSlot(ET_FindTxdSlot(kEffectTxd, 0));
}

// 0x5781F0 (thiscall): a texture with the name's hash is in the list
uint8_t __fastcall HasTexture(EffectTextures* self, re::Edx, uint32_t hash) {
    for (int i = 0; i < self->count; i++) {
        if ((self->entries[i].flags & 1) && self->entries[i].hash == hash) {
            return 1;
        }
    }
    return 0;
}

// 0x578230 (thiscall): the constructor
EffectTextures* __fastcall Construct(EffectTextures* self, re::Edx) {
    self->name = self->buffer;
    self->capacity = 0x41;
    self->buffer[0] = 0;
    self->count = 0;
    self->loaded = 0;
    for (int i = 0; i < 128; i++) {
        self->entries[i].flags &= 0xFE;
        self->entries[i].texture = nullptr;
    }
    return self;
}

// 0x578280 (thiscall): the dictionary loaded from the file (a new slot, kept)
void __fastcall Load(EffectTextures* self, re::Edx, const char* filename) {
    ET_PushMemoryTag(0x17);
    self->loaded = 1;
    int slot = ET_AddTxdSlot(kEffectTxd);
    ET_LoadTxd(slot, filename);
    ET_AddRef(slot);
    ET_PopMemoryTag();
}

// 0x5782C0 (thiscall): a texture from the dictionary into the list by name (the first
// argument isn't read); false when the dictionary hasn't it
uint8_t __fastcall AddTexture(EffectTextures* self, re::Edx, int, const char* name) {
    int slot = ET_FindTxdSlot(kEffectTxd, 0);
    TxdPool* pool = g_txdPool;
    TxdDefHead* def = (pool->flags[slot] & 0x80) ? nullptr : reinterpret_cast<TxdDefHead*>(pool->objects + pool->stride * slot);
    self->entries[self->count].texture = ET_FindTexture(def->dictionary, name);
    if (self->entries[self->count].texture == nullptr) {
        return 0;
    }
    self->entries[self->count].flags |= 1;
    self->entries[self->count].hash = ET_HashName(name);
    self->count += 1;
    return 1;
}

// 0x578350 (thiscall): a texture's index by name hash (0 when it's missing: the game
// builds its "not found" message and doesn't show it)
int __fastcall FindIndex(EffectTextures* self, re::Edx, uint32_t hash) {
    int i = 0;
    for (; i < self->count; i++) {
        if ((self->entries[i].flags & 1) && self->entries[i].hash == hash) {
            break;
        }
    }
    if (i >= self->count) {
        struct {
            char* text;
            int32_t capacity;
            char buffer[0x81];
        } message;
        message.text = message.buffer;
        message.capacity = 0x81;
        message.buffer[0] = 0;
        ET_Strncpy(message.buffer, kNotFound, 0x81);
    }
    for (int j = 0; j < self->count; j++) {
        if ((self->entries[j].flags & 1) && self->entries[j].hash == hash) {
            return j;
        }
    }
    return 0;
}

// 0x5783F0 (thiscall on a texture reference byte): set to the texture's index
void __fastcall SetRef(uint8_t* self, re::Edx, uint32_t hash) {
    *self |= 1;
    int index = ET_FindIndex(g_effectTextures, hash);
    *self = uint8_t((*self & 1) | uint8_t(index * 2));
}

} // namespace EffectTexturesImpl

// ---- registration: all original until tested ---------------------------------------

#define EFFECTTEX_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(effecttex_original_, __LINE__);                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(effecttex_original_, __LINE__), re::Mode::Original)

EFFECTTEX_REVERSED("EffectTextureRef::Clear", 0x005781C0, &EffectTexturesImpl::ClearRef);
EFFECTTEX_REVERSED("EffectTextures::RemoveTxd", 0x005781D0, &EffectTexturesImpl::RemoveTxd);
EFFECTTEX_REVERSED("EffectTextures::HasTexture", 0x005781F0, &EffectTexturesImpl::HasTexture);
EFFECTTEX_REVERSED("EffectTextures::EffectTextures", 0x00578230, &EffectTexturesImpl::Construct);
EFFECTTEX_REVERSED("EffectTextures::Load", 0x00578280, &EffectTexturesImpl::Load);
EFFECTTEX_REVERSED("EffectTextures::AddTexture", 0x005782C0, &EffectTexturesImpl::AddTexture);
EFFECTTEX_REVERSED("EffectTextures::FindIndex", 0x00578350, &EffectTexturesImpl::FindIndex);
EFFECTTEX_REVERSED("EffectTextureRef::Set", 0x005783F0, &EffectTexturesImpl::SetRef);
