#include "game/Texture.h"

#include <intrin.h>

#include <cstring>

#include "game/Crt.h"
#include "reversed.h"
namespace {
// Gamebryo, not reversed yet
GAME_FN(void*, __cdecl, NiMalloc, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiDelete, 0x00752540, void* p, size_t size);
GAME_FN(void, __cdecl, NiFree, 0x00836F80, void* p);
GAME_FN(void, __thiscall, TextureArray_RemoveAll, 0x0040E7A0, TextureArray* array);
GAME_FN(void, __thiscall, TextureArray_SetSize, 0x00856170, TextureArray* array, uint32_t size);
GAME_FN(void, __thiscall, TextureArray_SetAt, 0x0040E800, TextureArray* array, uint32_t index, re::Ptr32<Ni::Texture>* texture);
GAME_FN(void, __thiscall, Texture_SetName, 0x0077C3A0, Ni::Texture* texture, const re::Ptr32<const char>* name);
GAME_FN(void*, __cdecl, Texture_GetPixels, 0x004076F0, Ni::Texture* texture);   // nullptr: none
GAME_FN(Ni::RefObject*, __thiscall, PixelData_Construct, 0x0076A610, void* mem, int width, int height, const void* format,
        int mipmaps, int faces);
GAME_FN(Ni::Texture*, __cdecl, SourceTexture_Create, 0x007699A0, Ni::RefObject* pixels, const void* prefs);
GAME_VAR(uint32_t, textureDataKind, 0x020E2250);   // the kind rendererData is checked for
inline re::Ptr32<void>* const refObjectVtable = reinterpret_cast<re::Ptr32<void>*>(0x008FEFD4);
inline re::Ptr32<void>* const paletteVtable = reinterpret_cast<re::Ptr32<void>*>(0x0092D278);
inline re::Ptr32<void>* const tArrayVtable = reinterpret_cast<re::Ptr32<void>*>(0x0092D28C);
inline re::Ptr32<void>* const textureArrayVtable = reinterpret_cast<re::Ptr32<void>*>(0x0092D294);
inline const void* const errorTextureFormat = reinterpret_cast<const void*>(0x00BC7750);
inline const void* const errorTexturePrefs = reinterpret_cast<const void*>(0x00BC7624);
// the CRT's (locale dependent)
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline int __cdecl Strnicmp(const char* a, const char* b, size_t n) { return crt::Strnicmp(a, b, n); }
// the reader LoadTxd builds on its stack (0x40C bytes)
GAME_FN(const char*, __cdecl, FixPath, 0x00829900, const char* path);
GAME_FN(void, __thiscall, Reader_Construct, 0x0075CE40, void* reader);
GAME_FN(uint8_t, __thiscall, Reader_Load, 0x00759E10, void* reader, const char* path);
GAME_FN(void, __thiscall, Reader_Destruct, 0x0075C020, void* reader);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

// (our own, called where modes apply)
GAME_FN(Ni::Texture*, __thiscall, Dict_FindTexture, 0x005F12C0, TexDictionary* dict, const char* name);
GAME_FN(Ni::Texture*, __cdecl, Txd_FindTextureInAnyTxd, 0x005F1710, const char* name);
GAME_FN(Ni::Texture*, __cdecl, Txd_GetErrorTexture, 0x005F13F0);
GAME_FN(uint8_t, __cdecl, Txd_LoadTxdFromReader, 0x005F2960, int slot, void* reader, uint32_t size);

// NiTObjectArray::Add (inlined in the game)
void AddToArray(TextureArray* array, re::Ptr32<Ni::Texture>* texture) {
    uint32_t n = array->size;
    if (n >= array->maxSize) {
        TextureArray_SetSize(array, array->growBy + n);
    }
    TextureArray_SetAt(array, n, texture);
}

// an object if it's of the kind (Gamebryo's type check: a bit in its class's set)
Ni::RefObject* AsDataKind(Ni::RefObject* object) {
    if (!object) {
        return nullptr;
    }
    auto* info = reinterpret_cast<uint8_t*(__thiscall*)(Ni::RefObject*)>(static_cast<void*>(object->vtable[2]))(object);
    uint32_t kind = textureDataKind;
    uint32_t bits = reinterpret_cast<uint32_t*>(info + 0xC)[kind >> 5];
    return bits & (1u << (kind & 31)) ? object : nullptr;
}

template <typename T>
re::Field<T> Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(object) + offset);
}
} // namespace

namespace Ni {

void AddRef(RefObject* object) {
    _InterlockedIncrement(&object->refCount);
}

void Release(RefObject* object) {
    if (_InterlockedDecrement(&object->refCount) == 0) {
        reinterpret_cast<void(__thiscall*)(RefObject*)>(static_cast<void*>(object->vtable[1]))(object);
    }
}

// 0x5F0F90: (a zero refcount isn't taken below 0)
static uint8_t __cdecl ReleaseChecked(RefObject* object) {
    if (uint32_t(object->refCount) > 0) {
        Release(object);
    }
    return 1;
}

} // namespace Ni

TextureArray* TextureArray::Destruct(uint8_t flags) {
    vtable = tArrayVtable;
    NiFree(items);
    if (flags & 1) {
        NiDelete(this, 0x10);
    }
    return this;
}

// ---- dictionaries --------------------------------------------------------------------------

namespace CTexDictionary {

TexDictionary* __cdecl Create() {
    auto* dict = static_cast<TexDictionary*>(Malloc(0xC));  // (not checked)
    auto* array = static_cast<TextureArray*>(NiMalloc(0x10));
    if (array) {
        array->maxSize = 0;
        array->growBy = 1;
        array->size = 0;
        array->count = 0;
        array->items = nullptr;
        array->vtable = textureArrayVtable;
    }
    dict->textures = array;
    dict->bytes = 0;
    dict->removeWhenUnused = 0;
    return dict;
}

void __cdecl Destroy(TexDictionary* dict) {
    if (!dict || !dict->textures) {
        return;
    }
    TextureArray_RemoveAll(dict->textures);
    if (TextureArray* array = dict->textures) {
        reinterpret_cast<void(__thiscall*)(TextureArray*, int)>(static_cast<void*>(array->vtable[0]))(array, 1);
    }
    dict->textures = nullptr;
    Free(dict);
}

uint8_t __cdecl Read(void* reader, TexDictionary* dict, const char*, const char*) {
    dict->bytes = 0;
    uint8_t ok = 1;
    for (uint32_t i = 0; i < Field<uint32_t>(reader, 0x164); i++) {
        auto texture = Field<re::Ptr32<Ni::Texture>*>(reader, 0x15C)[i];
        if (!texture) {
            continue;
        }
        re::Ptr32<Ni::Texture> held = texture;
        Ni::AddRef(texture);
        if (!Texture_GetPixels(texture)) {
            ok = 0;
        }
        Texture_SetName(texture, &texture->fileName);
        AddToArray(dict->textures, &held);
        // its memory: the mip levels (each a quarter of the one before) in its format
        Ni::RefObject* data = AsDataKind(texture->rendererData);
        uint16_t mipLevels = Field<uint16_t>(data, 0x60);
        using Dimension = int32_t(__thiscall*)(Ni::Texture*);
        int32_t width = re::PtrCast<Dimension>(texture->vtable[0x4C / 4])(texture);
        int32_t height = re::PtrCast<Dimension>(texture->vtable[0x48 / 4])(texture);
        int32_t level = width * height;
        int32_t pixels = level;
        for (int m = 1; m < mipLevels; m++) {
            level /= 4;
            pixels += level;
        }
        int32_t format = Field<int32_t>(data, 0x18);
        int32_t bytes;
        if (format == 6) {
            bytes = (pixels >> 4) * 15;
        } else if (format == 4) {
            bytes = pixels >> 1;
        } else {
            bytes = pixels * 4;
        }
        dict->bytes += bytes;
        Ni::Release(held);
    }
    return ok;
}

Ni::Texture* __cdecl AddTexture(TexDictionary* dict, Ni::Texture* texture) {
    re::Ptr32<Ni::Texture> held = texture;
    if (texture) {
        Ni::AddRef(texture);
    }
    AddToArray(dict->textures, &held);
    if (texture) {
        Ni::Release(texture);
    }
    return texture;
}

uint8_t __fastcall IsInUse(TexDictionary* dict, re::Edx) {
    for (uint32_t i = 0; i < dict->textures->size; i++) {
        Ni::Texture* texture = dict->textures->items[i];
        if (texture) {
            Ni::AddRef(texture);
        }
        // (the dictionary's ref and ours; the game doesn't expect an empty slot)
        if (uint32_t(texture->refCount) > 2) {
            Ni::Release(texture);
            return 1;
        }
        Ni::Release(texture);
    }
    return 0;
}

Ni::Texture* __fastcall FindTexture(TexDictionary* dict, re::Edx, const char* name) {
    if (!dict->textures) {
        return nullptr;
    }
    for (uint32_t i = 0; i < dict->textures->size; i++) {
        Ni::Texture* texture = dict->textures->items[i];
        if (!texture) {
            continue;
        }
        Ni::AddRef(texture);
        const char* textureName = texture->name;
        if (!textureName) {
            Ni::Release(texture);
            continue;
        }
        const char* slash = std::strrchr(textureName, '\\');
        const char* file = slash ? slash + 1 : textureName;
        const char* dot = std::strrchr(file, '.');
        bool match;
        if (dot) {
            int length = int(dot - file);
            match = int(std::strlen(name)) == length && Strnicmp(name, file, size_t(length)) == 0;
        } else {
            match = Stricmp(name, file) == 0;
        }
        Ni::Release(texture);
        if (match) {
            return texture;
        }
    }
    return nullptr;
}

TexDictionary* __cdecl ForAllTextures(TexDictionary* dict, void*(__cdecl* callback)(Ni::Texture*, void*), void* data) {
    for (uint32_t i = 0; i < dict->textures->size; i++) {
        Ni::Texture* texture = dict->textures->items[i];
        if (!texture) {
            continue;
        }
        Ni::AddRef(texture);
        if (!callback(texture, data)) {
            Ni::Release(texture);
            return dict;
        }
        Ni::Release(texture);
    }
    return dict;
}

Ni::Texture* __cdecl FindNamedTexture(TexDictionary* dict, const char* name) {
    if (!dict) {
        return nullptr;
    }
    Ni::Texture* texture = Dict_FindTexture(dict, name);
    if (texture) {
        Ni::AddRef(texture);
    }
    return texture;
}

} // namespace CTexDictionary

// ---- the palette ---------------------------------------------------------------------------

Ni::Texture* BullyTexturePalette::GetTexture(const char* name, int) {
    if (TexDictionary* dict = CTxdStore::ms_pCurrentTxd) {
        const char* slash = std::strrchr(name, '\\');
        char key[0x200];
        std::strncpy(key, slash ? slash + 1 : name, sizeof(key));  // (unterminated if too long, as in the game)
        if (char* dot = std::strrchr(key, '.')) {
            *dot = '\0';
        }
        if (Ni::Texture* texture = Dict_FindTexture(dict, key)) {
            return texture;
        }
    }
    return Txd_GetErrorTexture();
}

BullyTexturePalette* BullyTexturePalette::Destruct(uint8_t flags) {
    vtable = refObjectVtable;
    _InterlockedDecrement(&Ni::ms_numRefObjects);
    if (flags & 1) {
        NiDelete(this, 8);
    }
    return this;
}

// ---- CTxdStore's --------------------------------------------------------------------------

namespace CTxdStore {

int32_t __cdecl GetTextureMemoryLeft() {
    return ms_textureMemoryBudget - ms_totalBytes - ms_textureMemoryReserved;
}

void __cdecl DeleteNameTree() {
    Free(ms_pNameTree);
    ms_pNameTree = nullptr;
}

void __cdecl SetCurrentTexDictionary(TexDictionary* dict) {
    ms_pCurrentTxd = dict;
}

void __cdecl PushCurrentTxd() {
    ms_pStoredTxd = ms_pCurrentTxd;
}

void __cdecl PopCurrentTxd() {
    ms_pCurrentTxd = ms_pStoredTxd;
    ms_pStoredTxd = nullptr;
}

const char* __cdecl SetTexturePath(const char* path) {
    if (!path) {
        ms_texturePath[0] = '\0';
        return path;
    }
    std::strncpy(ms_texturePath, path, 0x1FF);
    return path;
}

void __cdecl SetUnknownB(int slot) {
    // (a free slot: the game writes to address 0xB)
    reinterpret_cast<uint8_t*>(ms_pTxdPool->GetAt(slot))[offsetof(TxdDef, unknownB)] = 1;
}

void __cdecl ClearAllUnknownB() {
    for (int slot = 0; slot < ms_pTxdPool->size; slot++) {
        if (TxdDef* def = ms_pTxdPool->GetAt(slot)) {
            def->unknownB = 0;
        }
    }
}

TxdKey* __cdecl GetKey(TxdKey* out, int16_t slot) {
    // (a free slot reads from address 0)
    auto* def = reinterpret_cast<uint8_t*>(ms_pTxdPool->GetAt(slot));
    uint8_t season = def[offsetof(TxdDef, season)];
    out->nameHash = *reinterpret_cast<uint32_t*>(def + offsetof(TxdDef, nameHash));
    out->season = season;
    return out;
}

Ni::Texture* __cdecl FindTextureInAnyTxd(const char* name) {
    // (the pool's size as it was at the start)
    uint32_t size = uint32_t(ms_pTxdPool->size);
    Ni::Texture* texture = nullptr;
    for (uint32_t slot = 0; slot < size; slot++) {
        TxdDef* def = ms_pTxdPool->GetAt(int(slot));
        if (def && def->dict) {
            texture = Dict_FindTexture(def->dict, name);
            if (texture) {
                return texture;
            }
        }
    }
    return texture;
}

Ni::Texture* __cdecl FindTexture(const char* name) {
    Ni::Texture* texture = nullptr;
    if (ms_pCurrentTxd) {
        texture = Dict_FindTexture(ms_pCurrentTxd, name);
    }
    if (!texture) {
        texture = Txd_FindTextureInAnyTxd(name);
    }
    if (texture) {
        Ni::AddRef(texture);
    }
    return texture;
}

Ni::Texture* __cdecl GetErrorTexture() {
    if (ms_pErrorTexture) {
        return ms_pErrorTexture;
    }
    void* mem = NiMalloc(0x70);
    if (!mem) {
        return ms_pErrorTexture;
    }
    Ni::RefObject* pixels = PixelData_Construct(mem, 4, 4, errorTextureFormat, 1, 1);
    if (pixels) {
        // the first mip level filled with 0xA5
        uint32_t* offsets = Field<uint32_t*>(pixels, 0x5C);
        std::memset(Field<uint8_t*>(pixels, 0x50) + offsets[0], 0xA5, offsets[1] - offsets[0]);
        ms_pErrorTexture = SourceTexture_Create(pixels, errorTexturePrefs);
        Ni::AddRef(ms_pErrorTexture);
    }
    return ms_pErrorTexture;
}

BullyTexturePalette* __cdecl GetTexturePalette() {
    if (ms_pTexturePalette) {
        return ms_pTexturePalette;
    }
    auto* palette = static_cast<BullyTexturePalette*>(NiMalloc(8));
    if (!palette) {
        // (the game then refs it anyway: a write to address 4)
        ms_pTexturePalette = nullptr;
        _InterlockedIncrement(reinterpret_cast<volatile long*>(uintptr_t(4)));
        return ms_pTexturePalette;
    }
    palette->vtable = refObjectVtable;
    palette->refCount = 0;
    _InterlockedIncrement(&Ni::ms_numRefObjects);
    palette->vtable = paletteVtable;
    ms_pTexturePalette = palette;
    Ni::AddRef(palette);
    return ms_pTexturePalette;
}

uint8_t __cdecl LoadTxdFromFile(int slot, const char* fileName) {
    const char* path = FixPath(fileName);
    alignas(16) uint8_t reader[0x40C];
    Reader_Construct(reader);
    if (!Reader_Load(reader, path)) {
        Reader_Destruct(reader);
        return 0;
    }
    uint8_t ok = Txd_LoadTxdFromReader(slot, reader, 0);
    Reader_Destruct(reader);
    return ok;
}

void __fastcall PoolInit(CPool<TxdDef>* pool, re::Edx) {
    uint32_t bytes;
    if (pool->unknown19) {
        uint64_t wanted = uint64_t(uint32_t(pool->size)) * 16;
        bytes = wanted > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(wanted);  // (new[]'s overflow check)
    } else {
        bytes = uint32_t(pool->size) << 4;
    }
    pool->ownsArrays = 1;
    pool->elemSize = 0x10;
    pool->objects = static_cast<TxdDef*>(Malloc(bytes));
    pool->flags = static_cast<uint8_t*>(Malloc(uint32_t(pool->size)));
    pool->lastUsed = 0;
    for (int i = 0; i < pool->size; i++) {
        pool->flags[i] |= 0x80;
        pool->flags[i] &= 0x80;
    }
}

TxdDef* __fastcall PoolNew(CPool<TxdDef>* pool, re::Edx) {
    uint8_t* flags = pool->flags;
    bool wrapped = false;
    if (!(flags[pool->lastUsed] & 0x80)) {
        do {
            pool->lastUsed++;
            if (pool->lastUsed >= pool->size) {
                if (wrapped) {
                    return nullptr;
                }
                pool->lastUsed = 0;
                wrapped = true;
            }
        } while (!(flags[pool->lastUsed] & 0x80));
    }
    // taken, and the reuse count up
    uint8_t& flag = pool->flags[pool->lastUsed];
    flag &= 0x7F;
    flag = uint8_t((flag & 0x80) | ((flag + 1) & 0x7F));
    auto* def = reinterpret_cast<TxdDef*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * pool->lastUsed);
    std::memset(def, 0, 16);
    return def;
}

} // namespace CTxdStore

// ---- registration -------------------------------------------------------------------------

#define TEX_REVERSED(name, address, fn)                                                              \
    static void* RE_CONCAT(tex_original_, __LINE__);                                                \
    REVERSED(name, address, fn, nullptr, &RE_CONCAT(tex_original_, __LINE__), re::Mode::Original)
#define TEX_FN(ns, fn, address) TEX_REVERSED(#ns "::" #fn, address, RE_BODY(&ns::fn))

TEX_REVERSED("NiRefObject::Release", 0x005F0F90, RE_BODY(&Ni::ReleaseChecked));
TEX_REVERSED("TextureArray::Destruct", 0x005F21A0, re::MemberAddress(&TextureArray::Destruct));
TEX_REVERSED("BullyTexturePalette::GetTexture", 0x005F1470, re::MemberAddress(&BullyTexturePalette::GetTexture));
TEX_REVERSED("BullyTexturePalette::Destruct", 0x005F14F0, re::MemberAddress(&BullyTexturePalette::Destruct));

TEX_FN(CTexDictionary, Create, 0x005F25B0);
TEX_FN(CTexDictionary, Destroy, 0x005F17B0);
TEX_FN(CTexDictionary, Read, 0x005F2750);
TEX_FN(CTexDictionary, AddTexture, 0x005F28F0);
TEX_FN(CTexDictionary, IsInUse, 0x005F1240);
TEX_FN(CTexDictionary, FindTexture, 0x005F12C0);
TEX_FN(CTexDictionary, ForAllTextures, 0x005F1590);
TEX_FN(CTexDictionary, FindNamedTexture, 0x005F1610);

TEX_FN(CTxdStore, GetTextureMemoryLeft, 0x005F0A40);
TEX_FN(CTxdStore, DeleteNameTree, 0x005F0A60);
TEX_FN(CTxdStore, SetCurrentTexDictionary, 0x005F0A80);
TEX_FN(CTxdStore, PushCurrentTxd, 0x005F0A90);
TEX_FN(CTxdStore, PopCurrentTxd, 0x005F0AA0);
TEX_FN(CTxdStore, SetTexturePath, 0x005F0B90);
TEX_FN(CTxdStore, SetUnknownB, 0x005F0FC0);
TEX_FN(CTxdStore, ClearAllUnknownB, 0x005F0FF0);
TEX_FN(CTxdStore, GetKey, 0x005F16B0);
TEX_FN(CTxdStore, FindTextureInAnyTxd, 0x005F1710);
TEX_FN(CTxdStore, FindTexture, 0x005F1770);
TEX_FN(CTxdStore, GetErrorTexture, 0x005F13F0);
TEX_FN(CTxdStore, GetTexturePalette, 0x005F1520);
TEX_FN(CTxdStore, LoadTxdFromFile, 0x005F2A80);
// (CPool<TxdDef>::Init 0x5F0F10 and New 0x5F1640 are registered in Pools.cpp)
