// NiStream: loading and saving NIF files (the header, the RTTI table, the object size table, the
// fixed string table, the object groups, the objects, the top-level objects, the link IDs),
// the large arrays and string maps it keeps, and the default texture palette.

#include "gamebryo/NiStream.h"

#include <cstring>

#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t kNiStreamVtable = 0x009517D8;
GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(uint32_t, ms_uiNifMinVersion, 0x00BC75B0);
GAME_VAR(uint32_t, ms_uiNifMaxVersion, 0x00BC75B4);
GAME_VAR(uint32_t, ms_uiNifMinUserDefinedVersion, 0x020E2484);
GAME_VAR(uint32_t, ms_uiNifMaxUserDefinedVersion, 0x020E2488);
GAME_VAR(void*, ms_kLoaders, 0x020E247C);
// the post-processing functions run over each top-level object after a load
// (NiTPrimitiveArray<void(*)(NiStream&, NiObject*)>)
GAME_VAR(NiTArray<void(__cdecl*)(NiStream*, void*)>*, ms_pkPostProcessFunctions, 0x020E2480);
// the streams' shared lock
inline NiCriticalSection& streamLock = *reinterpret_cast<NiCriticalSection*>(0x020E248C);
// NiTexture's list of all textures, and its lock
GAME_VAR(void*, ms_pkTextureListHead, 0x020E2178);
inline NiCriticalSection& textureListLock = *reinterpret_cast<NiCriticalSection*>(0x020E21B8);
GAME_VAR(uint32_t, kNiNodeClassIndex, 0x020E2550);
GAME_VAR(uint32_t, kNiAVObjectClassIndex, 0x020E251C);
GAME_VAR(uint32_t, kNiSourceTextureClassIndex, 0x020E2A18);

inline void* IsKindOf(void* object, uint32_t classIndex) {
    if (object == nullptr) {
        return nullptr;
    }
    const uint32_t* rtti = Virtual<const uint32_t*>(object, 2);
    return rtti[3 + (classIndex >> 5)] & (1u << (classIndex & 31)) ? object : nullptr;
}

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// the stream's reads and writes, as the library inlines them
inline void Read(NiStream* s, void* buffer, uint32_t bytes, uint32_t componentSize) {
    NiBinaryStream* in = s->istr;
    in->absoluteCurrentPos += in->read(in, buffer, bytes, &componentSize, 1);
}
inline void Write(NiStream* s, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    NiBinaryStream* out = s->ostr;
    out->absoluteCurrentPos += out->write(out, buffer, bytes, &componentSize, 1);
}
inline uint32_t ReadUint(NiStream* s) {
    uint32_t value;
    Read(s, &value, 4, 4);
    return value;
}
inline void WriteUint(NiStream* s, uint32_t value) {
    Write(s, &value, 4, 4);
}

// a large array's SetAt as inlined (the used count kept)
template <typename T>
inline void LargeSetAt(NiTLargeArray<T>* a, uint32_t index, T value) {
    if (index < a->size) {
        if (value == T()) {
            if (a->base[index] != T()) {
                a->esize--;
            }
        } else if (a->base[index] == T()) {
            a->esize++;
        }
    } else {
        a->size = index + 1;
        if (value != T()) {
            a->esize++;
        }
    }
    a->base[index] = value;
}

// the string maps (NiTStringPointerMap<unsigned short>, NiTStringMap<NiTexturePtr>): vtable,
// hash size, buckets, count, whether keys are copies; an item: next, key, value
struct MapItem {
    re::Ptr32<MapItem> next;   // +0x0
    re::Ptr32<char> key;       // +0x4
    uint32_t value;  // +0x8
};
struct StringMap {
    re::Ptr32<void> vtable;          // +0x0
    uint32_t hashSize;     // +0x4
    re::Ptr32<re::Ptr32<MapItem>> table;       // +0x8
    uint32_t count;        // +0xC
    uint8_t copyKeys;      // +0x10
};

GAME_FN(uint32_t, __cdecl, NiTStringMap_Hash, 0x005F09F0, const char* key, uint32_t hashSize);
GAME_FN(void, __thiscall, NiTMapBase_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, NiTMapAllocator_Deallocate, 0x00401B30, void* allocator, void* item);
GAME_FN(char*, __cdecl, NiStrcpy, 0x00747280, char* to, uint32_t size, const char* from);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, uintptr_t size, const char* format, ...);
GAME_FN(uint8_t, __cdecl, NiSystemDesc_IsLittleEndian, 0x007529F0);
GAME_FN(uint8_t, __cdecl, NiPath_Standardize, 0x00752C20, char* path);
GAME_FN(void*, __cdecl, NiFile_GetFile, 0x00753380, const char* path, int mode, uint32_t bufferSize);
GAME_FN(uint32_t, __thiscall, NiBinaryStream_GetLine, 0x00752B10, void* stream, char* buffer, uint32_t maxBytes);
GAME_FN(uint32_t, __thiscall, NiBinaryStream_PutS, 0x00752BD0, void* stream, const char* s);
GAME_FN(void, __thiscall, NiSearchPath_SetFilePath, 0x008C5440, void* searchPath, const char* path);
GAME_FN(void*, __thiscall, NiSearchPath_Construct, 0x008C53B0, void* self);
// NiMemStream (0x24 bytes)
GAME_FN(void, __thiscall, NiMemStream_ConstructRead, 0x008C52E0, void* self, void* buffer, uint32_t size);
GAME_FN(void, __thiscall, NiMemStream_ConstructWrite, 0x008C5320, void* self);
GAME_FN(void*, __thiscall, NiMemStream_Str, 0x008C5060, void* self);
GAME_FN(void, __thiscall, NiMemStream_Destruct, 0x008C5030, void* self);
GAME_FN(void, __thiscall, NiStreamThread_Pause, 0x008C5370, void* thread);
GAME_FN(void, __cdecl, NiStreamLoadBinary_Uint, 0x00755380, NiStream* stream, uint32_t* value);
GAME_FN(void, __cdecl, NiStreamLoadBinary_Bool, 0x0075A550, NiStream* stream, uint8_t* value);
GAME_FN(void, __thiscall, CharArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, CharArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(void*, __thiscall, NiObjectGroup_Construct, 0x007D1C90, void* self, uint32_t size);
GAME_FN(void, __thiscall, LinkIDSet_Grow, 0x0083AD20, NiLinkIDSet* set, uint32_t allocated);
GAME_FN(uint8_t, __thiscall, LoaderMap_GetAt, 0x00800470, void* map, const char* name, re::Ptr32<void>* create);
GAME_FN(void, __thiscall, NiFixedString_AssignChars, 0x00406730, re::Ptr32<char>* self, const char* s);
GAME_FN(void, __thiscall, NiFixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* array, uint32_t size, uint32_t count,
        void(__thiscall* construct)(void*));
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t size);
GAME_FN(void, __cdecl, NiArrayFree, 0x00836F80, void* p);
GAME_FN(void*, __cdecl, NiPointerArray_New, 0x00856120, uint32_t count);
GAME_FN(uint8_t, __thiscall, TextureMap_GetAt, 0x00845270, void* map, const char* name, re::Ptr32<NiRefObject>* texture);
GAME_FN(void, __thiscall, TextureMap_SetAt, 0x00747380, void* map, const char* name, NiRefObject* texture);
// NiStream's own, at their addresses
GAME_FN(uint32_t, __thiscall, NiStream_GetStringID, 0x00759D40, NiStream* self, const re::Ptr32<char>* s);
GAME_FN(void, __thiscall, Map_SetAt, 0x00759AB0, StringMap* self, const char* key, uint32_t value);
GAME_FN(uint8_t, __thiscall, Map_GetAt, 0x00759B50, StringMap* self, const char* key, uint16_t* value);
GAME_FN(void, __thiscall, Map_GetNext, 0x00759BC0, StringMap* self, re::Ptr32<MapItem>* position, re::Ptr32<const char>* key,
        uint16_t* value);
GAME_FN(void, __thiscall, Map_Destruct759C60, 0x00759C60, StringMap* self);
GAME_FN(void, __thiscall, Map_Destruct75A4E0, 0x0075A4E0, StringMap* self);
GAME_FN(void, __thiscall, Map_Destruct75A5A0, 0x0075A5A0, StringMap* self);
GAME_FN(void, __thiscall, UintArray_SetSize, 0x0075A400, NiTLargeArray<uint32_t>* self, uint32_t size);
GAME_FN(void, __thiscall, ObjectArray_SetAt, 0x0075B280, NiTLargeArray<NiRefObject*>* self, uint32_t index,
        const re::Ptr32<NiRefObject>* value);
GAME_FN(void, __thiscall, ObjectArray_RemoveAll, 0x0075B320, NiTLargeArray<NiRefObject*>* self);
GAME_FN(void, __thiscall, ObjectArray_SetSize, 0x0075B990, NiTLargeArray<NiRefObject*>* self, uint32_t size);
GAME_FN(void, __thiscall, StringArray_SetAt, 0x0075B370, NiTLargeArray<char*>* self, uint32_t index,
        const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, StringArray_RemoveAll, 0x0075B400, NiTLargeArray<char*>* self);
GAME_FN(void, __thiscall, StringArray_SetSize, 0x0075BAF0, NiTLargeArray<char*>* self, uint32_t size);
GAME_FN(re::Ptr32<char>*, __cdecl, NiFixedStringArray_New, 0x0075A760, uint32_t count);
GAME_FN(void*, __thiscall, NiFixedStringArray_Destroy, 0x0075B1B0, re::Ptr32<char>* self, int flags);
GAME_FN(void, __thiscall, NiStream_RemoveAllObjects, 0x0075B4D0, NiStream* self);
GAME_FN(void, __thiscall, NiStream_ResetLinks, 0x0075B930, NiStream* self);
GAME_FN(uint8_t, __thiscall, NiStream_LoadRTTI, 0x0075B5B0, NiStream* self);
GAME_FN(uint8_t, __thiscall, NiStream_LoadFixedStringTable, 0x0075C580, NiStream* self);
GAME_FN(void, __thiscall, NiStream_LoadObjectGroups, 0x0075AA50, NiStream* self);
GAME_FN(void, __thiscall, NiStream_FixUpOldObjects, 0x0075ABF0, NiStream* self);
GAME_FN(void, __thiscall, NiStream_FixUpOldFlags, 0x0075AB60, NiStream* self, void* object);
GAME_FN(void, __thiscall, NiStream_ReadLinkID, 0x0075A7B0, NiStream* self);
GAME_FN(void, __thiscall, NiStream_SaveRTTI, 0x0075CA60, NiStream* self);
GAME_FN(void, __thiscall, NiStream_SaveFixedStringTable, 0x0075AD60, NiStream* self);
GAME_FN(void, __thiscall, NiStream_PrepareObjectGroups, 0x0075B810, NiStream* self);
GAME_FN(void, __thiscall, NiStream_SaveObjectGroups, 0x0075A0D0, NiStream* self);
GAME_FN(void, __thiscall, NiStream_SaveCString, 0x0075AFB0, NiStream* self, const char* s);
GAME_FN(void, __thiscall, NiStream_Destruct, 0x0075C020, NiStream* self);
GAME_FN(void*, __thiscall, NiDefaultTexturePalette_Construct, 0x0075BD10, void* self, int searchGlobal);

// the shared lock held, the objects and link IDs dropped
inline void ResetLocked(NiStream* s) {
    Lock(streamLock);
    ObjectArray_RemoveAll(&s->objects);
    s->linkIDs.count = 0;
    s->linkIDBlocks.count = 0;
    s->linkIndex = 0;
    s->linkBlockIndex = 0;
    Unlock(streamLock);
}

inline void SetError(NiStream* s, uint32_t code, const char* message) {
    s->errorCode = code;
    crt::StrcpyS(s->errorMessage, 0x104, message);
}

} // namespace

// ---- loading and saving: the entry points

// 0x7599F0: Load(buffer, size) (slot 2), through a memory stream
uint8_t __fastcall NiStream_LoadBuffer(NiStream* self, re::Edx, void* buffer, uint32_t size) {
    alignas(4) uint8_t memory[0x24];
    NiMemStream_ConstructRead(memory, buffer, size);
    uint8_t ok = Virtual<uint8_t>(self, 1, memory);
    NiMemStream_Destruct(memory);
    return ok;
}

// 0x759A30: Load(NiBinaryStream*) (slot 1): LoadStream (slot 15) from it, when it's good
uint8_t __fastcall NiStream_LoadBinaryStream(NiStream* self, re::Edx, NiBinaryStream* stream) {
    if (!Virtual<uint8_t>(stream, 1)) {
        self->istr = nullptr;
        return 0;
    }
    self->istr = stream;
    uint8_t ok = Virtual<uint8_t>(self, 15);
    self->istr = nullptr;
    return ok;
}

// 0x759A80: Save(NiBinaryStream*) (slot 4): SaveStream (slot 16)
uint8_t __fastcall NiStream_SaveBinaryStream(NiStream* self, re::Edx, NiBinaryStream* stream) {
    self->ostr = stream;
    uint8_t ok = Virtual<uint8_t>(self, 16);
    self->ostr = nullptr;
    return ok;
}

// ---- NiTStringPointerMap<unsigned short> (the RTTI names while saving)

// 0x759AB0: SetAt
void __fastcall NiTStringPointerMap_SetAt(StringMap* self, re::Edx, const char* key, uint32_t value) {
    uint32_t bucket = NiTStringMap_Hash(key, self->hashSize);
    for (MapItem* item = self->table[bucket]; item != nullptr; item = item->next) {
        if (std::strcmp(key, item->key) == 0) {
            *reinterpret_cast<uint16_t*>(&item->value) = uint16_t(value);
            return;
        }
    }
    MapItem* item = Virtual<MapItem*>(self, 3);
    Virtual<void>(self, 1, item, key, value);
    item->next = self->table[bucket];
    self->table[bucket] = item;
    self->count++;
}

// 0x759B50: GetAt
uint8_t __fastcall NiTStringPointerMap_GetAt(StringMap* self, re::Edx, const char* key, uint16_t* value) {
    for (MapItem* item = self->table[NiTStringMap_Hash(key, self->hashSize)]; item != nullptr; item = item->next) {
        if (std::strcmp(key, item->key) == 0) {
            *value = uint16_t(item->value);
            return 1;
        }
    }
    return 0;
}

// 0x759BC0: GetNext
void __fastcall NiTStringPointerMap_GetNext(StringMap* self, re::Edx, re::Ptr32<MapItem>* position, re::Ptr32<const char>* key,
                                           uint16_t* value) {
    MapItem* item = *position;
    *key = item->key;
    *value = uint16_t(item->value);
    if (item->next != nullptr) {
        *position = item->next;
        return;
    }
    uint32_t size = self->hashSize;
    for (uint32_t i = NiTStringMap_Hash(item->key, size) + 1; i < size; i++) {
        if (self->table[i] != nullptr) {
            *position = self->table[i];
            return;
        }
    }
    *position = nullptr;
}

// 0x759C40: SetValue (the key not copied)
void __fastcall NiTStringPointerMap_SetValue(StringMap*, re::Edx, MapItem* item, char* key, uint32_t value) {
    item->key = key;
    *reinterpret_cast<uint16_t*>(&item->value) = uint16_t(value);
}

namespace {
// a string map's destructor: its copied keys freed, the items, the buckets
inline void DestroyStringMap(StringMap* self, uintptr_t vtable, uintptr_t middle, uintptr_t base) {
    self->vtable = reinterpret_cast<void*>(vtable);
    if (self->copyKeys) {
        for (uint32_t i = 0; i < self->hashSize; i++) {
            for (MapItem* item = self->table[i]; item != nullptr;) {
                MapItem* next = item->next;
                NiFree(item->key);
                item = next;
            }
        }
    }
    self->vtable = reinterpret_cast<void*>(middle);
    NiTMapBase_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(base);
    NiTMapBase_RemoveAll(self);
    NiFree(self->table);
}
} // namespace

// 0x759C60: NiTStringMap<NiTexturePtr>'s destructor (the palette's)
void __fastcall NiTStringMap_Destruct759C60(StringMap* self, re::Edx) {
    DestroyStringMap(self, 0x00951784, 0x00951700, 0x0099ACD4);
}

// 0x759CD0: NiTLargePrimitiveArray<unsigned int>'s deleting destructor
void* __fastcall UintArray_Destroy(NiTLargeArray<uint32_t>* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x009516E8);
    NiFree(self->base);
    if (flags & 1) {
        NiDelete(self, sizeof(*self));
    }
    return self;
}

// 0x759D00: a NiTPointerMap's deleting destructor (vtables 0x951700, 0x99ACD4)
void* __fastcall NiTPointerMap_Destroy759D00(StringMap* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x00951700);
    NiTMapBase_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(0x0099ACD4);
    NiTMapBase_RemoveAll(self);
    NiFree(self->table);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// ---- link IDs, strings and groups

// 0x759D40: GetStringID (its index in the fixed strings; -1 for none)
uint32_t __fastcall NiStream_GetStringIDImpl(NiStream* self, re::Edx, const re::Ptr32<char>* s) {
    char* handle = *s;
    if (handle != kNullFixedString) {
        for (uint32_t i = 0; i < self->fixedStrings.size; i++) {
            if (handle == self->fixedStrings.base[i]) {
                return i;
            }
        }
    }
    return 0xFFFFFFFF;
}

// 0x759D80: GetObjectFromLinkID (the next link ID read, resolved)
void* __fastcall NiStream_ResolveLinkID(NiStream* self, re::Edx) {
    uint32_t id = self->linkIDs.base[self->linkIndex++];
    if (id == 0xFFFFFFFF) {
        return nullptr;
    }
    return self->objects.base[id];
}

// 0x759DB0: GetNumberOfLinkIDs (the next block's count)
uint32_t __fastcall NiStream_GetNumberOfLinkIDs(NiStream* self, re::Edx) {
    return self->linkIDBlocks.base[self->linkBlockIndex++];
}

// 0x759DD0: GetGroupFromLinkID
void* __fastcall NiStream_GetGroupFromLinkID(NiStream* self, re::Edx, uint32_t id) {
    return self->groups.base[id];
}

// 0x759DE0: GetLinkIDFromGroup (0 when it isn't there)
uint32_t __fastcall NiStream_GetLinkIDFromGroup(NiStream* self, re::Edx, void* group) {
    for (uint32_t i = 0; i < self->groups.size; i++) {
        if (group == self->groups.base[i]) {
            return i;
        }
    }
    return 0;
}

// 0x759E10: Load(path) (slot 3): the path standardized and given to the search path, the file
// read through slot 1
uint8_t __fastcall NiStream_LoadFile(NiStream* self, re::Edx, const char* path) {
    crt::StrcpyS(self->fileName, 0x104, path);
    NiPath_Standardize(self->fileName);
    NiSearchPath_SetFilePath(self->searchPath, self->fileName);
    void* file = NiFile_GetFile(self->fileName, 0, 0x8000);
    if (file != nullptr) {
        if (Virtual<uint8_t>(file, 1)) {
            uint8_t ok = Virtual<uint8_t>(self, 1, file);
            Virtual<void>(file, 0, 1);
            return ok;
        }
        Virtual<void>(file, 0, 1);
    }
    self->errorCode = 1;
    crt::StrcpyS(self->errorMessage, 0x104, "Cannot open file.");
    return 0;
}

// ---- saving

// 0x759F70: SaveTopLevelObjects (slot 19)
void __fastcall NiStream_SaveTopLevelObjects(NiStream* self, re::Edx) {
    uint32_t count = self->topObjects.size;
    WriteUint(self, count);
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(self, 12, self->topObjects.base[i]);
    }
}

// 0x759FE0: PreSaveObjectSizeTable (slot 21): room for the sizes; where it starts
uint32_t __fastcall NiStream_PreSaveObjectSizeTable(NiStream* self, re::Edx) {
    uint32_t position = Virtual<uint32_t>(self->ostr, 3);
    for (uint32_t i = 0; i < self->objects.size; i++) {
        WriteUint(self, 0);
    }
    return position;
}

// 0x75A050: SaveObjectSizeTable (slot 22): back to the room, the sizes written
uint8_t __fastcall NiStream_SaveObjectSizeTable(NiStream* self, re::Edx, uint32_t position) {
    uint32_t now = Virtual<uint32_t>(self->ostr, 3);
    Virtual<void>(self->ostr, 2, -int(now - position));
    for (uint32_t i = 0; i < self->objectSizes.size; i++) {
        WriteUint(self, self->objectSizes.base[i]);
    }
    return 1;
}

// 0x75A0D0: SaveObjectGroups (their count, then each one's size)
void __fastcall NiStream_SaveObjectGroupsImpl(NiStream* self, re::Edx) {
    WriteUint(self, uint32_t(self->groups.size) - 1);
    for (uint32_t i = 1; i < self->groups.size; i++) {
        WriteUint(self, *static_cast<uint32_t*>(self->groups.base[i]));
    }
}

// 0x75A160: RegisterSaveObjects (slot 17): each top-level object's RegisterStreamables
void __fastcall NiStream_RegisterSaveObjects(NiStream* self, re::Edx) {
    for (uint32_t i = 0; i < self->topObjects.size; i++) {
        Virtual<void>(self->topObjects.base[i], 6, self);
    }
}

// 0x75A190: Save(path) (slot 6)
uint8_t __fastcall NiStream_SaveFile(NiStream* self, re::Edx, const char* path) {
    crt::StrcpyS(self->fileName, 0x104, path);
    NiPath_Standardize(self->fileName);
    NiSearchPath_SetFilePath(self->searchPath, self->fileName);
    void* file = NiFile_GetFile(self->fileName, 1, 0x8000);
    if (file != nullptr) {
        if (Virtual<uint8_t>(file, 1)) {
            uint8_t ok = Virtual<uint8_t>(self, 4, file);
            Virtual<void>(file, 0, 1);
            return ok;
        }
        Virtual<void>(file, 0, 1);
    }
    return 0;
}

// 0x75A210: Save(buffer&, size&) (slot 5), into a memory stream
uint8_t __fastcall NiStream_SaveBuffer(NiStream* self, re::Edx, re::Ptr32<void>* buffer, uint32_t* size) {
    alignas(4) uint8_t memory[0x24];
    NiMemStream_ConstructWrite(memory);
    uint8_t ok = Virtual<uint8_t>(self, 4, memory);
    *size = At<uint32_t>(memory, 0x18);
    *buffer = NiMemStream_Str(memory);
    NiMemStream_Destruct(memory);
    return ok;
}

// 0x75A260: SaveFixedString (its ID; -1 for the empty string)
void __fastcall NiStream_SaveFixedString(NiStream* self, re::Edx, const re::Ptr32<char>* s) {
    if (*s == kNullFixedString) {
        Write(self, reinterpret_cast<const void*>(0x009516E4), 4, 4);
        return;
    }
    uint32_t id = NiStream_GetStringID(self, s);
    WriteUint(self, id);
}

// 0x75A2C0: GetVersionFromString ("a.b.c.d": a byte each, the first at the top; strtok_s is
// ours: it keeps its state in the context)
uint32_t __cdecl NiStream_GetVersionFromString(const char* text) {
    char copy[0x10];
    crt::StrcpyS(copy, 0x10, text);
    uint32_t version = 0;
    int shift = 0x18;
    char* context = nullptr;
    for (char* token = strtok_s(copy, ".", &context); token != nullptr; token = strtok_s(nullptr, ".", &context)) {
        version |= uint32_t(crt::Atol(token)) << (shift & 31);
        shift -= 8;
    }
    return version;
}

// 0x75A340: SetTexturePalette (under the streams' lock)
void __fastcall NiStream_SetTexturePalette(NiStream* self, re::Edx, NiRefObject* palette) {
    Lock(streamLock);
    if (self->texturePalette != palette) {
        ReleaseRef(self->texturePalette);
        self->texturePalette = palette;
        if (palette != nullptr) {
            Increment(&palette->refCount);
        }
    }
    Unlock(streamLock);
}

// 0x75A3D0 and 0x75A3E0: the palette's SetTexture (slot 3) and GetTexture (slot 2; none
// without a palette)
void __fastcall NiStream_SetTexture(NiStream* self, re::Edx, const char* name, void* texture) {
    Virtual<void>(self->texturePalette, 3, name, texture);
}
void* __fastcall NiStream_GetTexture(NiStream* self, re::Edx, const char* name, void* exclude) {
    if (self->texturePalette == nullptr) {
        return nullptr;
    }
    return Virtual<void*>(self->texturePalette, 2, name, exclude);
}

// 0x75A400: NiTLargePrimitiveArray<unsigned int>::SetSize
void __fastcall UintArray_SetSizeImpl(NiTLargeArray<uint32_t>* self, re::Edx, uint32_t size) {
    if (size == self->maxSize) {
        return;
    }
    if (size < self->size) {
        for (uint32_t i = size; i < self->size; i++) {
            if (self->base[i] != 0) {
                self->base[i] = 0;
                self->esize--;
            }
        }
        self->size = size;
    }
    uint32_t* old = self->base;
    self->maxSize = size;
    if (size == 0) {
        self->base = nullptr;
    } else {
        self->base = static_cast<uint32_t*>(NiMalloc(size * 4));
        for (uint32_t i = 0; i < self->size; i++) {
            self->base[i] = old[i];
        }
        for (uint32_t i = self->size; i < self->maxSize; i++) {
            self->base[i] = 0;
        }
    }
    NiFree(old);
}

// 0x75A4C0 and 0x75A580: two maps' ClearValue (the value zeroed, the item given back)
void __fastcall NiTPointerMap_ClearValue(uint8_t* self, re::Edx, MapItem* item) {
    item->value = 0;
    NiTMapAllocator_Deallocate(self + 0xC, item);
}
void __fastcall NiTStringPointerMap_ClearValue(uint8_t* self, re::Edx, MapItem* item) {
    *reinterpret_cast<uint16_t*>(&item->value) = 0;
    NiTMapAllocator_Deallocate(self + 0xC, item);
}

// 0x75A4E0 and 0x75A5A0: two string maps' destructors
void __fastcall NiTStringMap_Destruct75A4E0(StringMap* self, re::Edx) {
    DestroyStringMap(self, 0x00951798, 0x00965A9C, 0x00950760);
}
void __fastcall NiTStringMap_Destruct75A5A0(StringMap* self, re::Edx) {
    DestroyStringMap(self, 0x009517AC, 0x00951748, 0x009516EC);
}

// 0x75A550: NiStreamLoadBinary(stream, bool&): one byte
void __cdecl NiStreamLoadBinary_BoolImpl(NiStream* stream, uint8_t* value) {
    Read(stream, value, 1, 1);
}

// 0x75A610: NiTStringPointerMap<unsigned short>::SetValue (a copy of the key when it copies)
void __fastcall NiTStringPointerMap_SetValueCopy(StringMap* self, re::Edx, MapItem* item, const char* key,
                                                uint32_t value) {
    if (self->copyKeys) {
        uint32_t bytes = uint32_t(std::strlen(key)) + 1;
        char* copy = static_cast<char*>(NiMalloc(bytes));
        item->key = copy;
        crt::StrcpyS(copy, bytes, key);
    } else {
        item->key = const_cast<char*>(key);
    }
    *reinterpret_cast<uint16_t*>(&item->value) = uint16_t(value);
}

// 0x75A670, 0x75A690, 0x75A6C0, 0x75A700, 0x75A740: deleting destructors
void* __fastcall NiTStringMap_Destroy75A670(StringMap* self, re::Edx, int flags) {
    Map_Destruct75A4E0(self);
    if (flags & 1) {
        NiDelete(self, 0x14);
    }
    return self;
}
void* __fastcall NiTPrimitiveArray_Destroy(NiTArray<void*>* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x00961988);
    NiFree(self->base);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}
void* __fastcall NiTPointerMap_Destroy75A6C0(StringMap* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x009516EC);
    NiTMapBase_RemoveAll(self);
    NiFree(self->table);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}
void* __fastcall NiTPointerMap_Destroy75A700(StringMap* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x00951748);
    NiTMapBase_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(0x009516EC);
    NiTMapBase_RemoveAll(self);
    NiFree(self->table);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}
void* __fastcall NiTStringMap_Destroy75A740(StringMap* self, re::Edx, int flags) {
    Map_Destruct75A5A0(self);
    if (flags & 1) {
        NiDelete(self, 0x14);
    }
    return self;
}

// 0x40B530 (thiscall): NiFixedString's default constructor: the null handle (0x951028's value)
re::Ptr32<char>* __fastcall NiFixedString_ConstructNull(re::Ptr32<char>* self, re::Edx) {
    *self = *reinterpret_cast<re::Ptr32<char>*>(0x00951028);
    return self;
}

// 0x75A760: new NiFixedString[count] (the count before them)
re::Ptr32<char>* __cdecl NiFixedStringArray_NewImpl(uint32_t count) {
    uint64_t total = uint64_t(count) * 4;
    uint32_t bytes = total >> 32 ? 0xFFFFFFFF : uint32_t(total);
    bytes = bytes > 0xFFFFFFFB ? 0xFFFFFFFF : bytes + 4;
    uint32_t* memory = static_cast<uint32_t*>(NiNewArray(bytes));
    if (memory == nullptr) {
        return nullptr;
    }
    *memory = count;
    VectorConstructor(memory + 1, 4, count, reinterpret_cast<void(__thiscall*)(void*)>(0x0040B530));
    return reinterpret_cast<re::Ptr32<char>*>(memory + 1);
}

// 0x75A7B0: ReadLinkID (kept for the link phase)
void __fastcall NiStream_ReadLinkIDImpl(NiStream* self, re::Edx) {
    uint32_t id = ReadUint(self);
    NiLinkIDSet* set = &self->linkIDs;
    if (set->count == set->allocated) {
        LinkIDSet_Grow(set, set->allocated != 0 ? set->allocated * 2 : 1);
    }
    set->base[set->count] = id;
    set->count++;
}

// 0x75A820: a link ID read and resolved at once
void* __fastcall NiStream_ReadLinkIDObject(NiStream* self, re::Edx) {
    int32_t id = int32_t(ReadUint(self));
    if (id == -1) {
        return nullptr;
    }
    return self->objects.base[id];
}

namespace {
inline void AddLinkIDBlock(NiStream* s, uint32_t count) {
    NiLinkIDSet* set = &s->linkIDBlocks;
    if (set->count == set->allocated) {
        LinkIDSet_Grow(set, set->allocated != 0 ? set->allocated * 2 : 1);
    }
    set->base[set->count] = count;
    set->count++;
}
} // namespace

// 0x75A870: ReadMultipleLinkIDs (their count kept as a block, and returned)
uint32_t __fastcall NiStream_ReadMultipleLinkIDs(NiStream* self, re::Edx) {
    uint32_t count = ReadUint(self);
    AddLinkIDBlock(self, count);
    for (uint32_t i = 0; i < count; i++) {
        NiStream_ReadLinkID(self);
    }
    return count;
}

// 0x75A900: a block count kept
void __fastcall NiStream_AddLinkIDBlock(NiStream* self, re::Edx, uint32_t count) {
    AddLinkIDBlock(self, count);
}

// 0x75A940: GetLinkIDFromObject (slot 11): the registration map's value; -1 for none
uint32_t __fastcall NiStream_GetLinkIDFromObject(NiStream* self, re::Edx, void* object) {
    if (object == nullptr) {
        return 0xFFFFFFFF;
    }
    uint32_t id = 0xFFFFFFFF;
    auto* table = re::PtrCast<re::Ptr32<MapItem>*>(self->registerTable);
    MapItem* item = table[reinterpret_cast<uintptr_t>(object) % self->registerHashSize];
    if (item != nullptr) {
        while (reinterpret_cast<uintptr_t>(object) != re::PtrCast<uintptr_t>(item->key)) {
            item = item->next;
            if (item == nullptr) {
                return 0xFFFFFFFF;
            }
        }
        id = item->value;
    }
    return id;
}

// 0x75A990: LoadObjectSizeTable (slot 23)
uint8_t __fastcall NiStream_LoadObjectSizeTable(NiStream* self, re::Edx) {
    UintArray_SetSize(&self->objectSizes, self->objects.size);
    for (uint32_t i = 0; i < self->objects.size; i++) {
        uint32_t size = 0;
        Read(self, &size, 4, 4);
        LargeSetAt(&self->objectSizes, i, size);
    }
    return 1;
}

// 0x75AA50: LoadObjectGroups (group 0 is none; each a NiObjectGroup of the size read)
void __fastcall NiStream_LoadObjectGroupsImpl(NiStream* self, re::Edx) {
    uint32_t count = ReadUint(self) + 1;
    CharArray_SetSize(&self->groups, count);
    re::Ptr32<void> none = nullptr;
    CharArray_SetAt(&self->groups, 0, &none);
    for (uint32_t i = 1; i < count; i++) {
        uint32_t size = ReadUint(self);
        void* memory = NiNew(0x10);
        void* group = memory != nullptr ? NiObjectGroup_Construct(memory, size) : nullptr;
        NiTArray<void*>& groups = self->groups;
        if (i < groups.size) {
            if (group != nullptr) {
                if (groups.base[i] == nullptr) {
                    groups.esize++;
                }
            } else if (groups.base[i] != nullptr) {
                groups.esize--;
            }
        } else {
            groups.size = uint16_t(i + 1);
            if (group != nullptr) {
                groups.esize++;
            }
        }
        groups.base[i] = group;
    }
}

// 0x75AB60: old files' flags fixed on a scene graph (bits 1..3 set, 4 cleared), down the
// children of nodes
void __fastcall NiStream_FixUpOldFlagsImpl(NiStream* self, re::Edx, uint8_t* object) {
    At<uint16_t>(object, 0xA8) = uint16_t((At<uint16_t>(object, 0xA8) & 0xFFEF) | 0xE);
    uint8_t* node = static_cast<uint8_t*>(IsKindOf(object, kNiNodeClassIndex));
    if (node == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < At<uint16_t>(node, 0xCE); i++) {
        void* child = At<re::Ptr32<void>*>(node, 0xC8)[i];
        if (child != nullptr) {
            NiStream_FixUpOldFlags(self, child);
        }
    }
}

// 0x75ABF0: files before 4.1.0.12: each root NiAVObject's flags fixed
void __fastcall NiStream_FixUpOldObjectsImpl(NiStream* self, re::Edx) {
    if (self->version >= 0x0401000C) {
        return;
    }
    for (uint32_t i = 0; i < self->objects.size; i++) {
        NiRefObject* object = self->objects.base[i];
        if (object != nullptr && IsKindOf(object, kNiAVObjectClassIndex) != nullptr &&
            At<void*>(object, 0x18) == nullptr) {
            NiStream_FixUpOldFlags(self, object);
        }
    }
}

// 0x75AC70: SaveHeader (slot 14): the text line, the version, the endianness, the user
// version, the object count; then the output swapped as the endianness says
void __fastcall NiStream_SaveHeader(NiStream* self, re::Edx) {
    Virtual<void>(self->ostr, 4, 0);
    NiBinaryStream_PutS(self->ostr, "Gamebryo File Format, Version 20.3.0.9\n");
    Write(self, &ms_uiNifMaxVersion, 4, 4);
    Write(self, &self->saveLittleEndian, 1, 1);
    Write(self, &ms_uiNifMaxUserDefinedVersion, 4, 4);
    WriteUint(self, self->objects.size);
    Virtual<void>(self->ostr, 4, uint32_t(self->saveLittleEndian ^ 1));
}

// 0x75AD60: SaveFixedStringTable (the count, the longest, then each string; 16-bit indices
// as the game counts them)
void __fastcall NiStream_SaveFixedStringTableImpl(NiStream* self, re::Edx) {
    WriteUint(self, self->fixedStrings.size);
    uint32_t longest = 0;
    for (uint16_t i = 0; i < self->fixedStrings.size; i++) {
        char* s = self->fixedStrings.base[i];
        if (s != kNullFixedString) {
            uint32_t length = *reinterpret_cast<uint16_t*>(s - 4);
            if (length > longest) {
                longest = length;
            }
        }
    }
    WriteUint(self, longest);
    for (uint16_t i = 0; i < self->fixedStrings.size; i++) {
        char* s = self->fixedStrings.base[i];
        uint32_t length = s == kNullFixedString ? 0 : *reinterpret_cast<uint16_t*>(s - 4);
        WriteUint(self, length);
        Write(self, s, length, 1);
    }
}

// 0x75AEA0: LoadCString(char*&) (NiMalloc'd; null when empty)
void __fastcall NiStream_LoadCStringAlloc(NiStream* self, re::Edx, re::Ptr32<char>* out) {
    int32_t length = int32_t(ReadUint(self));
    if (length > 0) {
        char* s = static_cast<char*>(NiMalloc(uint32_t(length) + 1));
        *out = s;
        Read(self, s, uint32_t(length), 1);
        (*out)[length] = 0;
        return;
    }
    *out = nullptr;
}

// 0x75AF40: LoadCString(char*) into a buffer
void __fastcall NiStream_LoadCStringInto(NiStream* self, re::Edx, char* buffer) {
    int32_t length = int32_t(ReadUint(self));
    Read(self, buffer, uint32_t(length), 1);
    buffer[length] = 0;
}

// 0x75AFB0: SaveCString
void __fastcall NiStream_SaveCStringImpl(NiStream* self, re::Edx, const char* s) {
    uint32_t length = s != nullptr ? uint32_t(std::strlen(s)) : 0;
    WriteUint(self, length);
    if (length != 0) {
        Write(self, s, length, 1);
    }
}

// 0x75B040: LoadFixedString (by its ID; -1 the empty one)
void __fastcall NiStream_LoadFixedString(NiStream* self, re::Edx, re::Ptr32<char>* out) {
    int32_t id = int32_t(ReadUint(self));
    if (id == -1) {
        NiFixedString_AssignChars(out, nullptr);
        return;
    }
    NiFixedString_Assign(out, &self->fixedStrings.base[id]);
}

// 0x75B0B0: a fixed string from an inline string (old files)
void __fastcall NiStream_LoadInlineFixedString(NiStream* self, re::Edx, re::Ptr32<char>* out) {
    char buffer[0x400];
    int32_t length = int32_t(ReadUint(self));
    if (length >= 0x400) {
        char* s = static_cast<char*>(NiMalloc(uint32_t(length) + 1));
        Read(self, s, uint32_t(length), 1);
        s[length] = 0;
        NiFixedString_AssignChars(out, s);
        NiFree(s);
        return;
    }
    if (length < 1) {
        NiFixedString_AssignChars(out, nullptr);
        return;
    }
    Read(self, buffer, uint32_t(length), 1);
    buffer[length] = 0;
    NiFixedString_AssignChars(out, buffer);
}

// 0x75B1B0: NiFixedString's vector deleting destructor
void* __fastcall NiFixedString_VectorDestroy(re::Ptr32<char>* self, re::Edx, int flags) {
    if (flags & 2) {
        uint32_t* header = reinterpret_cast<uint32_t*>(self) - 1;
        for (int32_t i = int32_t(*header) - 1; i >= 0; i--) {
            FixedStringRelease(&self[i]);
        }
        if (flags & 1) {
            NiDeleteArray(header, 4);
        }
        return header;
    }
    FixedStringRelease(self);
    if (flags & 1) {
        NiDelete(self, 4);
    }
    return self;
}

// 0x75B280: NiTLargeObjectArray<NiObjectPtr>::SetAt
void __fastcall ObjectArray_SetAtImpl(NiTLargeArray<NiRefObject*>* self, re::Edx, uint32_t index,
                                      const re::Ptr32<NiRefObject>* value) {
    if (index < self->size) {
        if (*value == nullptr) {
            if (self->base[index] != nullptr) {
                self->esize--;
            }
        } else if (self->base[index] == nullptr) {
            self->esize++;
        }
    } else {
        self->size = index + 1;
        if (*value != nullptr) {
            self->esize++;
        }
    }
    re::Ptr32<NiRefObject>& slot = self->base[index];
    if (slot != *value) {
        ReleaseRef(slot);
        slot = *value;
        if (slot != nullptr) {
            Increment(&slot->refCount);
        }
    }
}

// 0x75B320: NiTLargeObjectArray<NiObjectPtr>::RemoveAll
void __fastcall ObjectArray_RemoveAllImpl(NiTLargeArray<NiRefObject*>* self, re::Edx) {
    for (uint32_t i = 0; i < self->size; i++) {
        re::Ptr32<NiRefObject>& slot = self->base[i];
        if (slot != nullptr) {
            ReleaseRef(slot);
            slot = nullptr;
        }
    }
    self->esize = 0;
    self->size = 0;
}

// 0x75B370: NiTLargeObjectArray<NiFixedString>::SetAt
void __fastcall StringArray_SetAtImpl(NiTLargeArray<char*>* self, re::Edx, uint32_t index, const re::Ptr32<char>* value) {
    if (index < self->size) {
        if (*value == nullptr) {
            if (self->base[index] != nullptr) {
                self->esize--;
            }
        } else if (self->base[index] == nullptr) {
            self->esize++;
        }
    } else {
        self->size = index + 1;
        if (*value != nullptr) {
            self->esize++;
        }
    }
    NiFixedString_Assign(&self->base[index], value);
}

// 0x75B400: NiTLargeObjectArray<NiFixedString>::RemoveAll
void __fastcall StringArray_RemoveAllImpl(NiTLargeArray<char*>* self, re::Edx) {
    for (uint32_t i = 0; i < self->size; i++) {
        re::Ptr32<char>* slot = &self->base[i];
        re::Ptr32<char> empty = nullptr;
        if (*slot != nullptr) {
            FixedStringRelease(slot);
            *slot = nullptr;
            if (empty != nullptr) {
                FixedStringRelease(&empty);
            }
        }
    }
    self->esize = 0;
    self->size = 0;
}

// 0x75B4B0: the palette's map's deleting destructor
void* __fastcall NiTStringMap_Destroy75B4B0(StringMap* self, re::Edx, int flags) {
    Map_Destruct759C60(self);
    if (flags & 1) {
        NiDelete(self, 0x14);
    }
    return self;
}

// 0x75B4D0: RemoveAllObjects (under the streams' lock: the objects, the fixed strings, the
// sizes)
void __fastcall NiStream_RemoveAllObjectsImpl(NiStream* self, re::Edx) {
    Lock(streamLock);
    ObjectArray_RemoveAll(&self->objects);
    StringArray_RemoveAll(&self->fixedStrings);
    for (uint32_t i = 0; i < self->objectSizes.size; i++) {
        self->objectSizes.base[i] = 0;
    }
    self->objectSizes.size = 0;
    self->objectSizes.esize = 0;
    Unlock(streamLock);
}

// 0x75B550: ChangeObject (slot 10): the object being loaded replaced
void __fastcall NiStream_ChangeObject(NiStream* self, re::Edx, NiRefObject* object) {
    re::Ptr32<NiRefObject> held = object;
    if (object != nullptr) {
        Increment(&object->refCount);
    }
    ObjectArray_SetAt(&self->objects, self->loadIndex, &held);
    ReleaseRef(object);
}

// 0x75B5B0: LoadRTTI: the class names (each looked up in the loaders), then each object made
// by its class's loader (a class without one fails the load, unless the object is marked
// skippable). (Names over 255 characters overrun the buffer, as the game's do.)
uint8_t __fastcall NiStream_LoadRTTIImpl(NiStream* self, re::Edx) {
    uint16_t count;
    Read(self, &count, 2, 2);
    using CreateFn = NiRefObject*(__cdecl*)();
    // (the game's 4-byte entries)
    re::Ptr32<void>* table = static_cast<re::Ptr32<void>*>(NiMalloc(uint32_t(count) * 4));
    for (uint32_t i = 0; i < count; i++) {
        char name[0x100];
        uint32_t length = ReadUint(self);
        Read(self, name, length, 1);
        name[length] = 0;
        if (!LoaderMap_GetAt(ms_kLoaders, name, &table[i])) {
            self->errorCode = 5;
            crt::StrcpyS(self->errorMessage, 0x104, name);
            crt::StrcatS(self->errorMessage, 0x104, ": cannot find create function.");
            table[i] = nullptr;
        }
    }
    for (uint32_t i = 0; i < self->objects.maxSize; i++) {
        uint16_t type;
        Read(self, &type, 2, 2);
        bool skippable = false;
        if (self->version >= 0x14020005 && (type & 0x8000)) {
            type &= 0x7FFF;
            skippable = true;
        }
        CreateFn create = reinterpret_cast<CreateFn>(static_cast<void*>(table[type]));
        if (create == nullptr) {
            if (!skippable) {
                NiFree(table);
                return 0;
            }
            re::Ptr32<NiRefObject> none = nullptr;
            ObjectArray_SetAt(&self->objects, i, &none);
        } else {
            NiRefObject* object = create();
            re::Ptr32<NiRefObject> held = object;
            if (object != nullptr) {
                Increment(&object->refCount);
            }
            ObjectArray_SetAt(&self->objects, i, &held);
            ReleaseRef(object);
        }
    }
    NiFree(table);
    return 1;
}

// 0x75B810: the objects' groups gathered (each counted once) and their sizes summed
void __fastcall NiStream_PrepareObjectGroupsImpl(NiStream* self, re::Edx) {
    NiTArray<void*>* groups = &self->groups;
    CharArray_SetSize(groups, 1);
    re::Ptr32<void> none = nullptr;
    CharArray_SetAt(groups, 0, &none);
    for (uint32_t i = 0; i < self->objects.size; i++) {
        NiRefObject* object = self->objects.base[i];
        if (object == nullptr) {
            continue;
        }
        re::Ptr32<void> group = Virtual<void*>(object, 16);
        if (group == nullptr) {
            continue;
        }
        uint32_t found = 0;
        bool present = false;
        for (uint32_t j = 0; j < groups->size; j++) {
            if (group == groups->base[j]) {
                found = j;
                present = true;
                break;
            }
        }
        if (present && found != 0) {
            continue;
        }
        uint32_t size = groups->size;
        if (size >= groups->maxSize) {
            CharArray_SetSize(groups, size + groups->growBy);
        }
        CharArray_SetAt(groups, size, &group);
        *static_cast<uint32_t*>(group) = 0;
    }
    for (uint32_t i = 0; i < self->objects.size; i++) {
        NiRefObject* object = self->objects.base[i];
        if (object == nullptr) {
            continue;
        }
        uint32_t* group = Virtual<uint32_t*>(object, 16);
        if (group != nullptr) {
            uint32_t size = *group;
            *group = Virtual<uint32_t>(object, 15) + size;
        }
    }
}

// 0x75B930: ResetLinks (under the streams' lock)
void __fastcall NiStream_ResetLinksImpl(NiStream* self, re::Edx) {
    ResetLocked(self);
}

// 0x75B990: NiTLargeObjectArray<NiObjectPtr>::SetSize
void __fastcall ObjectArray_SetSizeImpl(NiTLargeArray<NiRefObject*>* self, re::Edx, uint32_t size) {
    if (size == self->maxSize) {
        return;
    }
    if (size < self->size) {
        for (uint32_t i = size; i < self->size; i++) {
            re::Ptr32<NiRefObject>& slot = self->base[i];
            if (slot != nullptr) {
                ReleaseRef(slot);
                slot = nullptr;
                self->esize--;
            }
        }
        self->size = size;
    }
    re::Ptr32<NiRefObject>* old = self->base;
    self->maxSize = size;
    if (size == 0) {
        self->base = nullptr;
    } else {
        self->base = static_cast<re::Ptr32<NiRefObject>*>(NiPointerArray_New(size));
        for (uint32_t i = 0; i < self->size; i++) {
            re::Ptr32<NiRefObject>& slot = self->base[i];
            if (slot != old[i]) {
                ReleaseRef(slot);
                slot = old[i];
                if (slot != nullptr) {
                    Increment(&slot->refCount);
                }
            }
        }
        for (uint32_t i = self->size; i < self->maxSize; i++) {
            re::Ptr32<NiRefObject>& slot = self->base[i];
            if (slot != nullptr) {
                ReleaseRef(slot);
                slot = nullptr;
            }
        }
    }
    NiArrayFree(old);
}

// 0x75BAF0: NiTLargeObjectArray<NiFixedString>::SetSize
void __fastcall StringArray_SetSizeImpl(NiTLargeArray<char*>* self, re::Edx, uint32_t size) {
    if (size == self->maxSize) {
        return;
    }
    if (size < self->size) {
        for (uint32_t i = size; i < self->size; i++) {
            if (self->base[i] != kNullFixedString) {
                re::Ptr32<char> empty = kNullFixedString;
                NiFixedString_Assign(&self->base[i], &empty);
                FixedStringRelease(&empty);
                self->esize--;
            }
        }
        self->size = size;
    }
    re::Ptr32<char>* old = self->base;
    self->maxSize = size;
    if (size == 0) {
        self->base = nullptr;
    } else {
        self->base = NiFixedStringArray_New(size);
        for (uint32_t i = 0; i < self->size; i++) {
            char* value = old[i];
            re::Ptr32<char>* slot = &self->base[i];
            if (*slot != value) {
                FixedStringAddRef(value);
                FixedStringRelease(slot);
                *slot = value;
            }
        }
        for (uint32_t i = self->size; i < self->maxSize; i++) {
            re::Ptr32<char>* slot = &self->base[i];
            re::Ptr32<char> empty = nullptr;
            if (*slot != nullptr) {
                FixedStringRelease(slot);
                *slot = nullptr;
                if (empty != nullptr) {
                    FixedStringRelease(&empty);
                }
            }
        }
    }
    if (old != nullptr) {
        uint32_t* header = reinterpret_cast<uint32_t*>(old) - 1;
        for (int32_t i = int32_t(*header) - 1; i >= 0; i--) {
            FixedStringRelease(&old[i]);
        }
        NiDeleteArray(header, 4);
    }
}

// ---- NiDefaultTexturePalette (vtable 0x9517C0, 0x20 bytes: a NiTStringMap<NiTexturePtr>
// copying its keys, and whether to search all textures)

// 0x75BD10: its constructor
void* __fastcall NiDefaultTexturePalette_ConstructImpl(uint8_t* self, re::Edx, int searchGlobal) {
    At<uint32_t>(self, 0) = 0x008FEFD4;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    At<uint32_t>(self, 0) = 0x009517C0;
    StringMap* map = reinterpret_cast<StringMap*>(self + 8);
    map->vtable = reinterpret_cast<void*>(0x0099ACD4);
    map->hashSize = 0x25;
    map->count = 0;
    map->table = static_cast<re::Ptr32<MapItem>*>(NiMalloc(0x94));
    crt::Memset(map->table, 0, map->hashSize * 4);
    map->copyKeys = 1;
    map->vtable = reinterpret_cast<void*>(0x00951784);
    At<uint8_t>(self, 0x1C) = uint8_t(searchGlobal);
    return self;
}

// 0x75BD80: GetTexture: the palette's own, else (when it searches) a source texture of that
// file name among all textures but `exclude`
void* __fastcall NiDefaultTexturePalette_GetTexture(uint8_t* self, re::Edx, const char* name, void* exclude) {
    re::Ptr32<NiRefObject> found = nullptr;
    if (TextureMap_GetAt(self + 8, name, &found)) {
        ReleaseRef(found);
        return found;
    }
    if (At<uint8_t>(self, 0x1C)) {
        Lock(textureListLock);
        for (uint8_t* texture = static_cast<uint8_t*>(ms_pkTextureListHead); texture != nullptr;
             texture = At<uint8_t*>(texture, 0x2C)) {
            if (texture != exclude && IsKindOf(texture, kNiSourceTextureClassIndex) != nullptr &&
                At<char*>(texture, 0x30) != nullptr && crt::Stricmp(At<char*>(texture, 0x30), name) == 0) {
                Unlock(textureListLock);
                ReleaseRef(found);
                return texture;
            }
        }
        Unlock(textureListLock);
    }
    ReleaseRef(found);
    return nullptr;
}

// 0x75BEE0: SetTexture (a reference passed by value to the map)
void __fastcall NiDefaultTexturePalette_SetTexture(uint8_t* self, re::Edx, const char* name, NiRefObject* texture) {
    if (texture != nullptr) {
        Increment(&texture->refCount);
    }
    TextureMap_SetAt(self + 8, name, texture);
}

// 0x75BF10: its deleting destructor
void* __fastcall NiDefaultTexturePalette_Destroy(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = 0x009517C0;
    Map_Destruct759C60(reinterpret_cast<StringMap*>(self + 8));
    At<uint32_t>(self, 0) = 0x008FEFD4;
    Decrement(&ms_uiObjects);
    if (flags & 1) {
        NiDelete(self, 0x20);
    }
    return self;
}

// 0x75BF50: NiTLargeObjectArray<NiFixedString>'s deleting destructor
void* __fastcall StringArray_Destroy(NiTLargeArray<char*>* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x00951918);
    if (self->base != nullptr) {
        NiFixedStringArray_Destroy(self->base, 3);
    }
    if (flags & 1) {
        NiDelete(self, sizeof(*self));
    }
    return self;
}

// ---- NiStream itself

// 0x75C020: ~NiStream
void __fastcall NiStream_DestructImpl(NiStream* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiStreamVtable);
    if (self->istr != nullptr) {
        Virtual<void>(self->istr, 0, 1);
    }
    if (self->ostr != nullptr) {
        Virtual<void>(self->ostr, 0, 1);
    }
    NiStream_RemoveAllObjects(self);
    if (self->texturePalette != nullptr) {
        ReleaseRef(self->texturePalette);
        self->texturePalette = nullptr;
    }
    if (self->searchPath != nullptr) {
        Virtual<void>(self->searchPath, 0, 1);
    }
    if (self->thread != nullptr) {
        Virtual<void>(self->thread, 0, 1);
    }
    Import<int>(kCloseHandle, self->semaphore);
    ReleaseRef(self->texturePalette);
    self->registerMapVtable = reinterpret_cast<void*>(0x00965A9C);
    NiTMapBase_RemoveAll(&self->registerMapVtable);
    self->registerMapVtable = reinterpret_cast<void*>(0x00950760);
    NiTMapBase_RemoveAll(&self->registerMapVtable);
    NiFree(self->registerTable);
    NiFree(self->linkIDBlocks.base);
    NiFree(self->linkIDs.base);
    self->fixedStrings.vtable = reinterpret_cast<void*>(0x00951918);
    if (self->fixedStrings.base != nullptr) {
        NiFixedStringArray_Destroy(self->fixedStrings.base, 3);
    }
    self->topObjects.vtable = reinterpret_cast<void*>(0x00951914);
    NiArrayFree(self->topObjects.base);
    self->objectSizes.vtable = reinterpret_cast<void*>(0x009516E8);
    NiFree(self->objectSizes.base);
    self->objects.vtable = reinterpret_cast<void*>(0x00951914);
    NiArrayFree(self->objects.base);
    self->groups.vtable = reinterpret_cast<void*>(0x00961988);
    NiFree(self->groups.base);
}

// 0x75C190: SaveLinkID (slot 12)
void __fastcall NiStream_SaveLinkID(NiStream* self, re::Edx, void* object) {
    uint32_t id = Virtual<uint32_t>(self, 11, object);
    WriteUint(self, id);
}

// 0x75C1E0: LoadHeader (slot 13): the text line, the version (within the supported ones),
// the endianness, the user version, the object count
uint8_t __fastcall NiStream_LoadHeader(NiStream* self, re::Edx) {
    char line[0x80];
    NiBinaryStream_GetLine(self->istr, line, 0x80);
    if (crt::Strstr(line, "File Format") == nullptr) {
        SetError(self, 2, "Not a NIF file");
        return 0;
    }
    Virtual<void>(self->istr, 4, 0);
    Read(self, &self->version, 4, 4);
    uint32_t version = self->version;
    if (version < ms_uiNifMinVersion) {
        SetError(self, 3, "NIF version is too old.");
        return 0;
    }
    if (version > ms_uiNifMaxVersion) {
        SetError(self, 4, "Unknown NIF version.");
        return 0;
    }
    self->sourceIsLittleEndian = 1;
    if (version >= 0x14000003) {
        NiStreamLoadBinary_Bool(self, &self->sourceIsLittleEndian);
    }
    if (self->sourceIsLittleEndian != 1) {
        if (NiSystemDesc_IsLittleEndian()) {
            self->errorCode = 6;
            NiStrcpy(self->errorMessage, 0x104, "Endian mismatch.");
            return 0;
        }
        NiSprintf(self->errorMessage, 0x104, "Warning: %s has to be endian swapped.\n", self->fileName);
    }
    if (self->version >= 0x0A000108) {
        NiStreamLoadBinary_Uint(self, &self->userVersion);
    }
    if (self->userVersion < ms_uiNifMinUserDefinedVersion) {
        SetError(self, 3, "NIF user defined version is too old.");
        return 0;
    }
    if (self->userVersion > ms_uiNifMaxUserDefinedVersion) {
        self->errorCode = 4;
        NiStrcpy(self->errorMessage, 0x104, "Unknown NIF user defined version.");
        return 0;
    }
    uint32_t count;
    NiStreamLoadBinary_Uint(self, &count);
    ObjectArray_SetSize(&self->objects, count);
    Virtual<void>(self->istr, 4, uint32_t(self->sourceIsLittleEndian != 1));
    return 1;
}

// 0x75C4A0: LoadTopLevelObjects (slot 18)
void __fastcall NiStream_LoadTopLevelObjects(NiStream* self, re::Edx) {
    uint32_t count = ReadUint(self);
    ObjectArray_SetSize(&self->topObjects, count);
    for (uint32_t i = 0; i < count; i++) {
        int32_t id = int32_t(ReadUint(self));
        NiRefObject* object = id == -1 ? nullptr : self->objects.base[id];
        re::Ptr32<NiRefObject> held = object;
        if (object != nullptr) {
            Increment(&object->refCount);
        }
        ObjectArray_SetAt(&self->topObjects, i, &held);
        ReleaseRef(object);
    }
}

// 0x75C580: LoadFixedStringTable (16-bit indices as the game counts them)
uint8_t __fastcall NiStream_LoadFixedStringTableImpl(NiStream* self, re::Edx) {
    uint32_t count = ReadUint(self);
    StringArray_SetSize(&self->fixedStrings, count);
    uint32_t longest = ReadUint(self);
    char* buffer = static_cast<char*>(NiMalloc(longest + 1));
    uint32_t counter = 0;
    for (uint32_t i = 0; i < count; i = uint16_t(++counter)) {
        uint32_t length = 0;
        Read(self, &length, 4, 4);
        Read(self, buffer, length, 1);
        buffer[length] = 0;
        re::Ptr32<char> handle = buffer != nullptr ? GlobalStringTable_AddString(buffer) : static_cast<char*>(kNullFixedString);
        StringArray_SetAt(&self->fixedStrings, i, &handle);
        FixedStringRelease(&handle);
    }
    NiFree(buffer);
    return 1;
}

// 0x75C6F0: LoadStream (slot 15): the header, the RTTI, the sizes, the strings, the groups;
// each object's LoadBinary (an object of an unknown, skippable class skipped by its size), the
// top-level objects, LinkObject, PostLinkObject, the post-processing functions. The load can
// be paused (state 3 -> 4, the loading thread's pause) or cancelled (state 2) between objects.
namespace {
inline void CheckPause(NiStream* s) {
    if (s->loadState == 3) {
        s->loadState = 4;
        NiStreamThread_Pause(At<void*>(s, 0x1EC));
    }
}
} // namespace
uint8_t __fastcall NiStream_LoadStream(NiStream* self, re::Edx) {
    if (!Virtual<uint8_t>(self, 13)) {
        return 0;
    }
    self->postLinkIndex = 0;
    self->linkObjectIndex = 0;
    self->loadIndex = 0;
    NiStream_RemoveAllObjects(self);
    bool hasRTTI = self->version >= 0x05000001;
    if (hasRTTI && !NiStream_LoadRTTI(self)) {
        return 0;
    }
    if (self->version >= 0x14020005 && !Virtual<uint8_t>(self, 23)) {
        return 0;
    }
    if (self->version >= 0x14010001 && !NiStream_LoadFixedStringTable(self)) {
        return 0;
    }
    if (self->version >= 0x05000006) {
        NiStream_LoadObjectGroups(self);
    }
    uint32_t count = self->objects.maxSize;
    for (; self->loadIndex < count; self->loadIndex++) {
        if (self->loadState == 2) {
            NiStream_ResetLinks(self);
            return 0;
        }
        CheckPause(self);
        if (hasRTTI) {
            NiRefObject* object = self->objects.base[self->loadIndex];
            if (object != nullptr) {
                Virtual<void>(object, 4, self);
            } else {
                Virtual<void>(self->istr, 2, self->objectSizes.base[self->loadIndex]);
            }
        } else if (!Virtual<uint8_t>(self, 20)) {
            NiStream_ResetLinks(self);
            return 0;
        }
    }
    Virtual<void>(self, 18);
    for (; self->linkObjectIndex < count; self->linkObjectIndex++) {
        CheckPause(self);
        NiRefObject* object = self->objects.base[self->linkObjectIndex];
        if (object != nullptr) {
            Virtual<void>(object, 5, self);
        }
    }
    for (; self->postLinkIndex < count; self->postLinkIndex++) {
        CheckPause(self);
        NiRefObject* object = self->objects.base[self->postLinkIndex];
        if (object != nullptr) {
            Virtual<void>(object, 12, self);
        }
    }
    if (self->loadState != 2) {
        CheckPause(self);
        auto functions = ms_pkPostProcessFunctions;
        if (functions->esize != 0) {
            for (uint32_t i = 0; i < self->topObjects.size; i++) {
                NiRefObject* object = self->topObjects.base[i];
                if (object == nullptr) {
                    continue;
                }
                for (uint32_t j = 0; j < functions->size; j++) {
                    auto function = functions->base[j];
                    if (function != nullptr) {
                        function(self, object);
                        functions = ms_pkPostProcessFunctions;
                    }
                }
            }
        }
        if (self->loadState != 2) {
            CheckPause(self);
            NiStream_FixUpOldObjects(self);
            ResetLocked(self);
            return 1;
        }
    }
    ResetLocked(self);
    return 0;
}

// 0x75CA60: SaveRTTI: the classes' names (each once, numbered in order of first use), then
// each object's class number (0x8000 when it can be skipped)
void __fastcall NiStream_SaveRTTIImpl(NiStream* self, re::Edx) {
    StringMap map;
    map.vtable = reinterpret_cast<void*>(0x009516EC);
    map.hashSize = 0x25;
    map.count = 0;
    map.table = static_cast<re::Ptr32<MapItem>*>(NiMalloc(0x94));
    crt::Memset(map.table, 0, map.hashSize * 4);
    map.copyKeys = 0;
    map.vtable = reinterpret_cast<void*>(0x009517AC);
    uint16_t index;
    for (uint32_t i = 0; i < self->objects.size; i++) {
        const char* name = *Virtual<re::Ptr32<const char>*>(self->objects.base[i], 14);
        if (!Map_GetAt(&map, name, &index)) {
            Map_SetAt(&map, name, map.count);
        }
    }
    uint16_t count = uint16_t(map.count);
    re::Ptr32<const char>* names = static_cast<re::Ptr32<const char>*>(NiMalloc(uint32_t(count) * 4));
    re::Ptr32<MapItem> position = nullptr;
    for (uint32_t i = 0; i < map.hashSize; i++) {
        if (map.table[i] != nullptr) {
            position = map.table[i];
            break;
        }
    }
    while (position != nullptr) {
        re::Ptr32<const char> key;
        uint16_t value;
        Map_GetNext(&map, &position, &key, &value);
        names[value] = key;
    }
    Write(self, &count, 2, 2);
    for (uint32_t i = 0; i < count; i++) {
        NiStream_SaveCString(self, names[i]);
    }
    NiFree(names);
    for (uint32_t i = 0; i < self->objects.size; i++) {
        NiRefObject* object = self->objects.base[i];
        const char* name = *Virtual<re::Ptr32<const char>*>(object, 14);
        uint32_t type = 0;
        Map_GetAt(&map, name, reinterpret_cast<uint16_t*>(&type));
        if (Virtual<uint8_t>(object, 13)) {
            type |= 0x8000;
        }
        Write(self, &type, 2, 2);
    }
    Map_Destruct75A5A0(&map);
}

// 0x75CC60: SaveStream (slot 16): registration, the header, the RTTI, room for the sizes, the
// strings, the groups, each object (its size measured), the top-level objects, the sizes
uint8_t __fastcall NiStream_SaveStream(NiStream* self, re::Edx) {
    Virtual<void>(self, 17);
    UintArray_SetSize(&self->objectSizes, self->objects.size);
    Virtual<void>(self, 14);
    NiStream_SaveRTTI(self);
    uint32_t sizeTable = Virtual<uint32_t>(self, 21);
    NiStream_SaveFixedStringTable(self);
    NiStream_PrepareObjectGroups(self);
    NiStream_SaveObjectGroups(self);
    for (uint32_t i = 0; i < self->objects.size; i++) {
        NiRefObject* object = self->objects.base[i];
        uint32_t start = Virtual<uint32_t>(self->ostr, 3);
        Virtual<void>(object, 7, self);
        uint32_t size = Virtual<uint32_t>(self->ostr, 3) - start;
        LargeSetAt(&self->objectSizes, i, size);
    }
    Virtual<void>(self, 19);
    Virtual<void>(self, 22, sizeTable);
    ObjectArray_RemoveAll(&self->objects);
    NiTMapBase_RemoveAll(&self->registerMapVtable);
    return 1;
}

// 0x75CD80: NiTLargeObjectArray<NiFixedString>::AddFirstEmpty (-1 for the empty string)
uint32_t __fastcall StringArray_AddFirstEmpty(NiTLargeArray<char*>* self, re::Edx, const re::Ptr32<char>* value) {
    if (*value == nullptr) {
        return 0xFFFFFFFF;
    }
    uint32_t size = self->size;
    for (uint32_t i = 0; i < size; i++) {
        if (self->base[i] == nullptr) {
            NiFixedString_Assign(&self->base[i], value);
            self->esize++;
            return i;
        }
    }
    if (size >= self->maxSize) {
        StringArray_SetSize(self, self->growBy + size);
    }
    StringArray_SetAt(self, size, value);
    return size;
}

// 0x75CE10: NiTLargeObjectArray<NiObjectPtr>'s deleting destructor
void* __fastcall ObjectArray_Destroy(NiTLargeArray<NiRefObject*>* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x00951914);
    NiArrayFree(self->base);
    if (flags & 1) {
        NiDelete(self, sizeof(*self));
    }
    return self;
}

// 0x75CE40: NiStream's constructor
NiStream* __fastcall NiStream_Construct(NiStream* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiStreamVtable);
    self->groups.maxSize = 0;
    self->groups.size = 0;
    self->groups.esize = 0;
    self->groups.base = nullptr;
    self->groups.vtable = reinterpret_cast<void*>(0x00961988);
    self->groups.growBy = 1;
    auto initObjects = [](NiTLargeArray<NiRefObject*>& a, uint32_t growBy) {
        a.maxSize = 0;
        a.growBy = growBy;
        a.size = 0;
        a.esize = 0;
        a.base = nullptr;
        a.vtable = reinterpret_cast<void*>(0x00951914);
    };
    initObjects(self->objects, 0x400);
    self->objectSizes.maxSize = 0;
    self->objectSizes.growBy = 1;
    self->objectSizes.size = 0;
    self->objectSizes.esize = 0;
    self->objectSizes.base = nullptr;
    self->objectSizes.vtable = reinterpret_cast<void*>(0x009516E8);
    initObjects(self->topObjects, 1);
    self->fixedStrings.maxSize = 0;
    self->fixedStrings.growBy = 0x80;
    self->fixedStrings.size = 0;
    self->fixedStrings.esize = 0;
    self->fixedStrings.base = nullptr;
    self->fixedStrings.vtable = reinterpret_cast<void*>(0x00951918);
    self->linkIDs = {nullptr, 0, 0};
    self->linkIDBlocks = {nullptr, 0, 0};
    self->registerMapVtable = reinterpret_cast<void*>(0x00950760);
    self->registerHashSize = 0x25;
    self->registerCount = 0;
    self->registerTable = static_cast<re::Ptr32<void>*>(NiMalloc(0x94));
    crt::Memset(self->registerTable, 0, self->registerHashSize * 4);
    self->registerMapVtable = reinterpret_cast<void*>(0x00965A9C);
    self->texturePalette = nullptr;
    self->unknown1E0 = 0;
    self->unknown1E4 = 1;
    self->semaphore = Import<void*>(kCreateSemaphoreA, static_cast<void*>(nullptr), 0L, 1L,
                                    static_cast<const char*>(nullptr));
    self->unknown1EC = 0;
    self->thread = nullptr;
    self->unknown1F4 = 2;
    self->unknown1F8 = -1;
    self->unknown1FC = -1;
    self->errorCode = 0;
    self->errorMessage[0] = 0;
    self->istr = nullptr;
    self->ostr = nullptr;
    self->version = ms_uiNifMaxVersion;
    self->userVersion = ms_uiNifMaxUserDefinedVersion;
    self->saveLittleEndian = 1;
    self->sourceIsLittleEndian = 1;
    self->fileName[0] = 0;
    void* memory = NiNew(0x20);
    NiRefObject* palette =
        memory != nullptr ? static_cast<NiRefObject*>(NiDefaultTexturePalette_Construct(memory, 0)) : nullptr;
    if (self->texturePalette != palette) {
        ReleaseRef(self->texturePalette);
        self->texturePalette = palette;
        if (palette != nullptr) {
            Increment(&palette->refCount);
        }
    }
    self->linkIndex = 0;
    self->linkBlockIndex = 0;
    memory = NiNew(0x210);
    self->searchPath = memory != nullptr ? NiSearchPath_Construct(memory) : nullptr;
    self->loadState = 0;
    self->errorCode = 0;
    self->errorMessage[0] = 0;
    return self;
}

// 0x75D0A0: NiStream's deleting destructor
NiStream* __fastcall NiStream_Destroy(NiStream* self, re::Edx, int flags) {
    NiStream_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiStream));
    }
    return self;
}

// 0x75D0D0: InsertObject (a top-level object added)
void __fastcall NiStream_InsertObject(NiStream* self, re::Edx, NiRefObject* object) {
    re::Ptr32<NiRefObject> held = object;
    if (object != nullptr) {
        Increment(&object->refCount);
    }
    uint32_t size = self->topObjects.size;
    if (self->topObjects.maxSize <= size) {
        ObjectArray_SetSize(&self->topObjects, self->topObjects.growBy + size);
    }
    ObjectArray_SetAt(&self->topObjects, size, &held);
    ReleaseRef(object);
}

#define NISTREAM_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISTREAM_REVERSED("NiFixedString::NiFixedString()", 0x0040B530, NiFixedString_ConstructNull);
NISTREAM_REVERSED("NiStream::Load(buffer)", 0x007599F0, NiStream_LoadBuffer);
NISTREAM_REVERSED("NiStream::Load(NiBinaryStream*)", 0x00759A30, NiStream_LoadBinaryStream);
NISTREAM_REVERSED("NiStream::Save(NiBinaryStream*)", 0x00759A80, NiStream_SaveBinaryStream);
NISTREAM_REVERSED("NiTStringPointerMap<ushort>::SetAt", 0x00759AB0, NiTStringPointerMap_SetAt);
NISTREAM_REVERSED("NiTStringPointerMap<ushort>::GetAt", 0x00759B50, NiTStringPointerMap_GetAt);
NISTREAM_REVERSED("NiTStringPointerMap<ushort>::GetNext", 0x00759BC0, NiTStringPointerMap_GetNext);
NISTREAM_REVERSED("NiTStringPointerMap<ushort>::SetValue", 0x00759C40, NiTStringPointerMap_SetValue);
NISTREAM_REVERSED("NiTStringMap<NiTexturePtr>::~NiTStringMap", 0x00759C60, NiTStringMap_Destruct759C60);
NISTREAM_REVERSED("NiTLargePrimitiveArray<uint>::Destroy", 0x00759CD0, UintArray_Destroy);
NISTREAM_REVERSED("NiTPointerMap::Destroy (0x759D00)", 0x00759D00, NiTPointerMap_Destroy759D00);
NISTREAM_REVERSED("NiStream::GetStringID", 0x00759D40, NiStream_GetStringIDImpl);
NISTREAM_REVERSED("NiStream::ResolveLinkID", 0x00759D80, NiStream_ResolveLinkID);
NISTREAM_REVERSED("NiStream::GetNumberOfLinkIDs", 0x00759DB0, NiStream_GetNumberOfLinkIDs);
NISTREAM_REVERSED("NiStream::GetGroupFromLinkID", 0x00759DD0, NiStream_GetGroupFromLinkID);
NISTREAM_REVERSED("NiStream::GetLinkIDFromGroup", 0x00759DE0, NiStream_GetLinkIDFromGroup);
NISTREAM_REVERSED("NiStream::Load(path)", 0x00759E10, NiStream_LoadFile);
NISTREAM_REVERSED("NiStream::SaveTopLevelObjects", 0x00759F70, NiStream_SaveTopLevelObjects);
NISTREAM_REVERSED("NiStream::PreSaveObjectSizeTable", 0x00759FE0, NiStream_PreSaveObjectSizeTable);
NISTREAM_REVERSED("NiStream::SaveObjectSizeTable", 0x0075A050, NiStream_SaveObjectSizeTable);
NISTREAM_REVERSED("NiStream::SaveObjectGroups", 0x0075A0D0, NiStream_SaveObjectGroupsImpl);
NISTREAM_REVERSED("NiStream::RegisterSaveObjects", 0x0075A160, NiStream_RegisterSaveObjects);
NISTREAM_REVERSED("NiStream::Save(path)", 0x0075A190, NiStream_SaveFile);
NISTREAM_REVERSED("NiStream::Save(buffer)", 0x0075A210, NiStream_SaveBuffer);
NISTREAM_REVERSED("NiStream::SaveFixedString", 0x0075A260, NiStream_SaveFixedString);
NISTREAM_REVERSED("NiStream::GetVersionFromString", 0x0075A2C0, NiStream_GetVersionFromString);
NISTREAM_REVERSED("NiStream::SetTexturePalette", 0x0075A340, NiStream_SetTexturePalette);
NISTREAM_REVERSED("NiStream::SetTexture", 0x0075A3D0, NiStream_SetTexture);
NISTREAM_REVERSED("NiStream::GetTexture", 0x0075A3E0, NiStream_GetTexture);
NISTREAM_REVERSED("NiTLargePrimitiveArray<uint>::SetSize", 0x0075A400, UintArray_SetSizeImpl);
NISTREAM_REVERSED("NiTPointerMap<NiObject*,uint>::ClearValue", 0x0075A4C0, NiTPointerMap_ClearValue);
NISTREAM_REVERSED("NiTStringMap::~NiTStringMap (0x75A4E0)", 0x0075A4E0, NiTStringMap_Destruct75A4E0);
NISTREAM_REVERSED("NiStreamLoadBinary(bool)", 0x0075A550, NiStreamLoadBinary_BoolImpl);
NISTREAM_REVERSED("NiTStringPointerMap<ushort>::ClearValue", 0x0075A580, NiTStringPointerMap_ClearValue);
NISTREAM_REVERSED("NiTStringMap::~NiTStringMap (0x75A5A0)", 0x0075A5A0, NiTStringMap_Destruct75A5A0);
NISTREAM_REVERSED("NiTStringPointerMap<ushort>::SetValue (copy)", 0x0075A610, NiTStringPointerMap_SetValueCopy);
NISTREAM_REVERSED("NiTStringMap::Destroy (0x75A670)", 0x0075A670, NiTStringMap_Destroy75A670);
NISTREAM_REVERSED("NiTPrimitiveArray<NiObjectGroup*>::Destroy", 0x0075A690, NiTPrimitiveArray_Destroy);
NISTREAM_REVERSED("NiTPointerMap::Destroy (0x75A6C0)", 0x0075A6C0, NiTPointerMap_Destroy75A6C0);
NISTREAM_REVERSED("NiTPointerMap::Destroy (0x75A700)", 0x0075A700, NiTPointerMap_Destroy75A700);
NISTREAM_REVERSED("NiTStringMap::Destroy (0x75A740)", 0x0075A740, NiTStringMap_Destroy75A740);
NISTREAM_REVERSED("NiFixedString::operator new[]", 0x0075A760, NiFixedStringArray_NewImpl);
NISTREAM_REVERSED("NiStream::ReadLinkID", 0x0075A7B0, NiStream_ReadLinkIDImpl);
NISTREAM_REVERSED("NiStream::ReadLinkIDObject", 0x0075A820, NiStream_ReadLinkIDObject);
NISTREAM_REVERSED("NiStream::ReadMultipleLinkIDs", 0x0075A870, NiStream_ReadMultipleLinkIDs);
NISTREAM_REVERSED("NiStream::AddLinkIDBlock", 0x0075A900, NiStream_AddLinkIDBlock);
NISTREAM_REVERSED("NiStream::GetLinkIDFromObject", 0x0075A940, NiStream_GetLinkIDFromObject);
NISTREAM_REVERSED("NiStream::LoadObjectSizeTable", 0x0075A990, NiStream_LoadObjectSizeTable);
NISTREAM_REVERSED("NiStream::LoadObjectGroups", 0x0075AA50, NiStream_LoadObjectGroupsImpl);
NISTREAM_REVERSED("NiStream::FixUpOldFlags", 0x0075AB60, NiStream_FixUpOldFlagsImpl);
NISTREAM_REVERSED("NiStream::FixUpOldObjects", 0x0075ABF0, NiStream_FixUpOldObjectsImpl);
NISTREAM_REVERSED("NiStream::SaveHeader", 0x0075AC70, NiStream_SaveHeader);
NISTREAM_REVERSED("NiStream::SaveFixedStringTable", 0x0075AD60, NiStream_SaveFixedStringTableImpl);
NISTREAM_REVERSED("NiStream::LoadCString(alloc)", 0x0075AEA0, NiStream_LoadCStringAlloc);
NISTREAM_REVERSED("NiStream::LoadCString(buffer)", 0x0075AF40, NiStream_LoadCStringInto);
NISTREAM_REVERSED("NiStream::SaveCString", 0x0075AFB0, NiStream_SaveCStringImpl);
NISTREAM_REVERSED("NiStream::LoadFixedString", 0x0075B040, NiStream_LoadFixedString);
NISTREAM_REVERSED("NiStream::LoadInlineFixedString", 0x0075B0B0, NiStream_LoadInlineFixedString);
NISTREAM_REVERSED("NiFixedString::VectorDestroy", 0x0075B1B0, NiFixedString_VectorDestroy);
NISTREAM_REVERSED("NiTLargeObjectArray<NiObjectPtr>::SetAt", 0x0075B280, ObjectArray_SetAtImpl);
NISTREAM_REVERSED("NiTLargeObjectArray<NiObjectPtr>::RemoveAll", 0x0075B320, ObjectArray_RemoveAllImpl);
NISTREAM_REVERSED("NiTLargeObjectArray<NiFixedString>::SetAt", 0x0075B370, StringArray_SetAtImpl);
NISTREAM_REVERSED("NiTLargeObjectArray<NiFixedString>::RemoveAll", 0x0075B400, StringArray_RemoveAllImpl);
NISTREAM_REVERSED("NiTStringMap<NiTexturePtr>::Destroy", 0x0075B4B0, NiTStringMap_Destroy75B4B0);
NISTREAM_REVERSED("NiStream::RemoveAllObjects", 0x0075B4D0, NiStream_RemoveAllObjectsImpl);
NISTREAM_REVERSED("NiStream::ChangeObject", 0x0075B550, NiStream_ChangeObject);
NISTREAM_REVERSED("NiStream::LoadRTTI", 0x0075B5B0, NiStream_LoadRTTIImpl);
NISTREAM_REVERSED("NiStream::PrepareObjectGroups", 0x0075B810, NiStream_PrepareObjectGroupsImpl);
NISTREAM_REVERSED("NiStream::ResetLinks", 0x0075B930, NiStream_ResetLinksImpl);
NISTREAM_REVERSED("NiTLargeObjectArray<NiObjectPtr>::SetSize", 0x0075B990, ObjectArray_SetSizeImpl);
NISTREAM_REVERSED("NiTLargeObjectArray<NiFixedString>::SetSize", 0x0075BAF0, StringArray_SetSizeImpl);
NISTREAM_REVERSED("NiDefaultTexturePalette::NiDefaultTexturePalette", 0x0075BD10, NiDefaultTexturePalette_ConstructImpl);
NISTREAM_REVERSED("NiDefaultTexturePalette::GetTexture", 0x0075BD80, NiDefaultTexturePalette_GetTexture);
NISTREAM_REVERSED("NiDefaultTexturePalette::SetTexture", 0x0075BEE0, NiDefaultTexturePalette_SetTexture);
NISTREAM_REVERSED("NiDefaultTexturePalette::Destroy", 0x0075BF10, NiDefaultTexturePalette_Destroy);
NISTREAM_REVERSED("NiTLargeObjectArray<NiFixedString>::Destroy", 0x0075BF50, StringArray_Destroy);
NISTREAM_REVERSED("NiStream::~NiStream", 0x0075C020, NiStream_DestructImpl);
NISTREAM_REVERSED("NiStream::SaveLinkID", 0x0075C190, NiStream_SaveLinkID);
NISTREAM_REVERSED("NiStream::LoadHeader", 0x0075C1E0, NiStream_LoadHeader);
NISTREAM_REVERSED("NiStream::LoadTopLevelObjects", 0x0075C4A0, NiStream_LoadTopLevelObjects);
NISTREAM_REVERSED("NiStream::LoadFixedStringTable", 0x0075C580, NiStream_LoadFixedStringTableImpl);
NISTREAM_REVERSED("NiStream::LoadStream", 0x0075C6F0, NiStream_LoadStream);
NISTREAM_REVERSED("NiStream::SaveRTTI", 0x0075CA60, NiStream_SaveRTTIImpl);
NISTREAM_REVERSED("NiStream::SaveStream", 0x0075CC60, NiStream_SaveStream);
NISTREAM_REVERSED("NiTLargeObjectArray<NiFixedString>::AddFirstEmpty", 0x0075CD80, StringArray_AddFirstEmpty);
NISTREAM_REVERSED("NiTLargeObjectArray<NiObjectPtr>::Destroy", 0x0075CE10, ObjectArray_Destroy);
NISTREAM_REVERSED("NiStream::NiStream", 0x0075CE40, NiStream_Construct);
NISTREAM_REVERSED("NiStream::Destroy", 0x0075D0A0, NiStream_Destroy);
NISTREAM_REVERSED("NiStream::InsertObject", 0x0075D0D0, NiStream_InsertObject);

} // namespace ni
