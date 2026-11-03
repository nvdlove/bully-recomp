#pragma once

// Textures and texture dictionaries: the Gamebryo side of CTxdStore (TexDictionary, in
// TxdStore.h), done the way RenderWare's RwTexDictionary / RwTextureRead were in GTA.

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "game/TxdStore.h"
#include "reversed.h"

// Gamebryo objects, as far as they're used here
namespace Ni {

// NiRefObject: refcounted, deleted by its DeleteThis (virtual 1) at 0
struct RefObject {
    re::Ptr32<re::Ptr32<void>> vtable;           // +0
    volatile long refCount;  // +4
};

// NiTexture (an NiSourceTexture when read from a dictionary)
struct Texture : RefObject {
    re::Ptr32<const char> name;           // +8 (a fixed string)
    uint8_t unknownC[0x18];     // +0xC
    re::Ptr32<RefObject> rendererData;    // +0x24 (of a kind with the format at +0x18, mip levels at +0x60)
    uint8_t unknown28[8];       // +0x28
    re::Ptr32<const char> fileName;       // +0x30
};
static_assert(offsetof(Texture, name) == 8);
static_assert(offsetof(Texture, rendererData) == 0x24);
static_assert(offsetof(Texture, fileName) == 0x30);

// NiPointer's: the ref taken / let go (deleted at 0)
void AddRef(RefObject* object);
void Release(RefObject* object);

// the count of live NiRefObjects
GAME_VAR(long, ms_numRefObjects, 0x020E2168);

} // namespace Ni

// A dictionary's textures: NiTObjectArray<NiPointer<NiTexture>>
struct TextureArray {
    re::Ptr32<re::Ptr32<void>> vtable;           // +0
    re::Ptr32<re::Ptr32<Ni::Texture>> items;     // +4
    uint16_t maxSize;        // +8
    uint16_t size;           // +0xA the slots used (the last one + 1)
    uint16_t count;          // +0xC the non-empty ones
    uint16_t growBy;         // +0xE

    // 0x5F21A0 (thiscall): NiTArray's destructor (flags 1: and freed)
    TextureArray* Destruct(uint8_t flags);
};
static_assert(sizeof(TextureArray) == 0x10);

// The texture palette models are given: textures by name from the current dictionary
struct BullyTexturePalette : Ni::RefObject {
    // 0x5F1470 (vf2): the texture of a file name (its path and extension dropped) in the
    // current dictionary, else the error texture. (Not ref'd for the caller.)
    Ni::Texture* GetTexture(const char* name, int unused);
    // 0x5F14F0 (vf0): the destructor (flags 1: and freed)
    BullyTexturePalette* Destruct(uint8_t flags);
};

// A dictionary's key in the name tree (what the tree compares)
struct TxdKey {
    uint32_t nameHash;
    uint32_t season;
};

namespace CTexDictionary {

// 0x5F25B0: an empty one; nullptr: out of memory
TexDictionary* __cdecl Create();
// 0x5F17B0: its textures let go, and it freed. (One whose array is gone isn't freed.)
void __cdecl Destroy(TexDictionary* dict);
// 0x5F2750: the textures an open reader has into the dictionary, their memory counted;
// 0 if one of them had no data
uint8_t __cdecl Read(void* reader, TexDictionary* dict, const char*, const char*);
// 0x5F28F0: the texture added (ref'd)
Ni::Texture* __cdecl AddTexture(TexDictionary* dict, Ni::Texture* texture);
// 0x5F1240 (thiscall): one of its textures is held by something else
uint8_t __fastcall IsInUse(TexDictionary* dict, re::Edx edx);
// 0x5F12C0 (thiscall): the texture of a name (without path or extension, any case);
// not ref'd
Ni::Texture* __fastcall FindTexture(TexDictionary* dict, re::Edx edx, const char* name);
// 0x5F1590: the callback on each texture until it returns 0
TexDictionary* __cdecl ForAllTextures(TexDictionary* dict, void*(__cdecl* callback)(Ni::Texture*, void*), void* data);
// 0x5F1610: FindTexture, ref'd (nullptr dictionary: none)
Ni::Texture* __cdecl FindNamedTexture(TexDictionary* dict, const char* name);

} // namespace CTexDictionary

namespace CTxdStore {

GAME_VAR(Ni::Texture*, ms_pErrorTexture, 0x01B6486C);            // what missing textures become
GAME_VAR(BullyTexturePalette*, ms_pTexturePalette, 0x01B64860);
GAME_ARRAY(char, ms_texturePath, 0x01B64660);                   // [0x200]
GAME_VAR(int32_t, ms_textureMemoryBudget, 0x00BCBC28);
GAME_VAR(int32_t, ms_textureMemoryReserved, 0x00B0A3C0);

// 0x5F0A40: the budget left for textures
int32_t __cdecl GetTextureMemoryLeft();
// 0x5F0A60
void __cdecl DeleteNameTree();
// 0x5F0A80: a dictionary as the current one directly
void __cdecl SetCurrentTexDictionary(TexDictionary* dict);
// 0x5F0A90
void __cdecl PushCurrentTxd();
// 0x5F0AA0
void __cdecl PopCurrentTxd();
// 0x5F0B90: the path (nullptr: none), returned
const char* __cdecl SetTexturePath(const char* path);
// 0x5F0FC0: TxdDef's unknownB set
void __cdecl SetUnknownB(int slot);
// 0x5F0FF0: and cleared on all of them
void __cdecl ClearAllUnknownB();
// 0x5F16B0: a slot's key in the name tree (returned by value: `out`)
TxdKey* __cdecl GetKey(TxdKey* out, int16_t slot);
// 0x5F1710: the texture in the first loaded dictionary that has it; not ref'd
Ni::Texture* __cdecl FindTextureInAnyTxd(const char* name);
// 0x5F1770: from the current dictionary, else any; ref'd
Ni::Texture* __cdecl FindTexture(const char* name);
// 0x5F13F0: a 4x4 texture (made on first use), ref'd once for keeps
Ni::Texture* __cdecl GetErrorTexture();
// 0x5F1520: the palette (made on first use)
BullyTexturePalette* __cdecl GetTexturePalette();
// 0x5F2A80: a slot's dictionary from a file
uint8_t __cdecl LoadTxdFromFile(int slot, const char* fileName);
// 0x5F0F10 (thiscall): the pool's arrays for its size (TxdDefs: 16 bytes each)
void __fastcall PoolInit(CPool<TxdDef>* pool, re::Edx edx);
// 0x5F1640 (thiscall): a free slot's TxdDef, zeroed (nullptr: full)
TxdDef* __fastcall PoolNew(CPool<TxdDef>* pool, re::Edx edx);

} // namespace CTxdStore
