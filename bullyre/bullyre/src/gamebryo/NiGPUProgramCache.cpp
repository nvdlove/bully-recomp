// NiGPUProgramCache (src/gamebryo/NiGPUProgramCache.h), 0x8CA490..0x8CBDD0: the cache of
// compiled shader programs by name (with the shader constants each uses) and of the names that
// failed, its file (a header, the renderer's names, then an entry a program), its two
// NiTFixedStringMaps, and the vertex semantics' names. Also the singleton base the DX9
// renderer's shader program factory derives from (0x8CC6E0..0x8CC710).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiFile.h"
#include "gamebryo/NiGPUProgramCache.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using sys::Decrement;
using sys::FixedStringRelease;
using sys::GlobalStringTable_AddString;
using sys::Import;
using sys::Increment;
using sys::kNullFixedString;

using ProgramMap = FixedStringMap<GPUProgramDescriptor*>;
using ProgramNode = FixedStringMapNode<GPUProgramDescriptor*>;
using BoolMap = FixedStringMap<uint8_t>;
using BoolNode = FixedStringMapNode<uint8_t>;

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kNiGPUProgramCacheVtable = 0x009A19E0;
constexpr uintptr_t kMapBaseVtable = 0x009A1978;      // NiTFixedStringMapBase's
constexpr uintptr_t kProgramMapVtable = 0x009A19AC;
constexpr uintptr_t kBoolMapVtable = 0x009A19B8;
constexpr uintptr_t kProgramFactoryVtable = 0x009A1A60;
constexpr const char kCacheHeader[] = "NiGPUProgram Cache File";   // (0x9A19C4)

GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(void*, gProgramFactory, 0x020E9658);       // the one shader program factory
GAME_VAR(uint8_t, gRewriteOutdatedCache, 0x00BCBB18);   // an out-of-date cache file saved again

GAME_FN(uint8_t, __thiscall, FixedString_Equals, 0x00406480, const re::Ptr32<char>* self, const char* text);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(void, __thiscall, NiString_Construct, 0x007741A0, void* self, const char* text);
GAME_FN(re::Ptr32<char>*, __cdecl, NiFixedStringArray_New, 0x0075A760, uint32_t count);
GAME_FN(void*, __thiscall, NiFixedStringArray_Destroy, 0x0075B1B0, re::Ptr32<char>* self, int flags);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(uint8_t, __cdecl, NiFile_Access, 0x007533C0, const char* path, int mode);
GAME_FN(NiBinaryStream*, __cdecl, NiFile_GetFile, 0x00753380, const char* path, int mode, uint32_t bufferSize);
GAME_FN(uint8_t, __cdecl, NiFile_DirectoryExists, 0x00753440, const char* path);
GAME_FN(uint8_t, __cdecl, NiFile_CreateDirectoryRecursive, 0x007534E0, const char* path);
GAME_FN(uint8_t, __cdecl, NiPath_IsRelative, 0x00753B50, const char* path);
GAME_FN(uint8_t, __cdecl, NiPath_GetCurrentWorkingDirectory, 0x00753BA0, char* path, uint32_t bytes);
GAME_FN(uint32_t, __cdecl, NiPath_ConvertToAbsolute, 0x007530C0, char* out, uint32_t size, const char* relative,
        const char* base);
GAME_FN(void, __cdecl, Stream_LoadBytes, 0x008885F0, NiBinaryStream* stream, void* buffer, uint32_t count);
GAME_FN(void, __cdecl, Stream_SaveBytes, 0x008885C0, NiBinaryStream* stream, const void* buffer, uint32_t count);
GAME_FN(void, __cdecl, Stream_LoadUInt8, 0x00822810, NiBinaryStream* stream, uint8_t* value);
GAME_FN(void, __cdecl, Stream_LoadUInt32, 0x00822840, NiBinaryStream* stream, uint32_t* value);
GAME_FN(void, __thiscall, FixedStringMap_RemoveAll, 0x008C2230, void* map);
GAME_FN(uint8_t, __thiscall, ProgramMap_GetAt, 0x00518180, ProgramMap* map, const re::Ptr32<char>* key,
        re::Ptr32<GPUProgramDescriptor>* value);
// (the value an NiPointer passed by value: the map takes the reference)
GAME_FN(void, __thiscall, ProgramMap_SetAt, 0x008B9E50, ProgramMap* map, const re::Ptr32<char>* key,
        GPUProgramDescriptor* value);
GAME_FN(void, __thiscall, NiPointer_AssignFrom, 0x00518100, re::Ptr32<NiRefObject>* self, const re::Ptr32<NiRefObject>* other);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, PrimitiveSet* set, const re::Ptr32<NiRefObject>* element);
GAME_FN(GPUProgramDescriptor*, __thiscall, Descriptor_Construct, 0x00898D80, void* self);
GAME_FN(NiRefObject*, __thiscall, ConstantEntry_Construct, 0x007E2C10, void* self);
// the constant entry's (0x2C) five names (+0x8..+0x18) and four values (+0x1C..+0x28)
GAME_FN(void, __thiscall, ConstantEntry_SetName8, 0x0077C3A0, NiRefObject* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, ConstantEntry_SetNameC, 0x007EAD00, NiRefObject* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, ConstantEntry_SetName10, 0x007E2D60, NiRefObject* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, ConstantEntry_SetName14, 0x007E2D70, NiRefObject* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, ConstantEntry_SetName18, 0x007F80C0, NiRefObject* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, ConstantEntry_SetValue1C, 0x008A2EF0, NiRefObject* self, uintptr_t value);
GAME_FN(void, __thiscall, ConstantEntry_SetValue20, 0x00670BA0, NiRefObject* self, uintptr_t value);
GAME_FN(void, __thiscall, ConstantEntry_SetValue24, 0x007E2BF0, NiRefObject* self, uint32_t value);
GAME_FN(void, __thiscall, ConstantEntry_SetValue28, 0x007E2C00, NiRefObject* self, uint32_t value);
GAME_FN(const re::Ptr32<char>*, __thiscall, ConstantEntry_GetName8, 0x0077C280, NiRefObject* self);
GAME_FN(const re::Ptr32<char>*, __thiscall, ConstantEntry_GetNameC, 0x008BAC30, NiRefObject* self);
GAME_FN(const re::Ptr32<char>*, __thiscall, ConstantEntry_GetName10, 0x007E2BC0, NiRefObject* self);
GAME_FN(const re::Ptr32<char>*, __thiscall, ConstantEntry_GetName14, 0x007E2BD0, NiRefObject* self);
GAME_FN(const re::Ptr32<char>*, __thiscall, ConstantEntry_GetName18, 0x007F7EE0, NiRefObject* self);
GAME_FN(uint32_t, __thiscall, ConstantEntry_GetValue1C, 0x0077E8E0, NiRefObject* self);
GAME_FN(uint32_t, __thiscall, ConstantEntry_GetValue20, 0x007E2BE0, NiRefObject* self);
GAME_FN(uint32_t, __thiscall, ConstantEntry_GetValue24, 0x008C3450, NiRefObject* self);
GAME_FN(uint32_t, __thiscall, ConstantEntry_GetValue28, 0x0077E8F0, NiRefObject* self);
// this file's, called where modes apply
GAME_FN(uint8_t, __thiscall, Cache_SaveString, 0x008CAF00, void* self, NiBinaryStream* stream, const char* text);
GAME_FN(uint8_t, __thiscall, Cache_LoadString, 0x008CAFA0, void* self, NiBinaryStream* stream, char* buffer,
        uint32_t size);
GAME_FN(void*, __thiscall, Map_GetFirstPos, 0x008CA4E0, const void* map);
GAME_FN(void, __thiscall, ObjectSet_OrderedRemoveAt, 0x008CA990, PrimitiveSet* set, uint32_t index);
GAME_FN(void, __thiscall, FixedStringSet_RemoveAll, 0x008CAA30, PrimitiveSet* set);
GAME_FN(void, __thiscall, ProgramMap_GetNext, 0x008CB020, ProgramMap* map, re::Ptr32<ProgramNode>* pos, re::Ptr32<char>* key,
        re::Ptr32<GPUProgramDescriptor>* value);
GAME_FN(void, __thiscall, BoolMap_SetAt, 0x008CB0D0, BoolMap* map, const re::Ptr32<char>* key, uint8_t value);
GAME_FN(void, __thiscall, BoolMap_GetNext, 0x008CB150, BoolMap* map, re::Ptr32<BoolNode>* pos, re::Ptr32<char>* key, uint8_t* value);
GAME_FN(void, __thiscall, Cache_Destruct, 0x008CBB90, NiGPUProgramCache* self);


template <typename R = void, typename... A>
R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

void Read(NiBinaryStream* file, void* buffer, uint32_t bytes, uint32_t componentSize) {
    uint32_t size = componentSize;
    file->absoluteCurrentPos += file->read(file, buffer, bytes, &size, 1);
}

void Write(NiBinaryStream* file, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    uint32_t size = componentSize;
    file->absoluteCurrentPos += file->write(file, buffer, bytes, &size, 1);
}

// a reference let go through the import (as these functions do)
void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual(object, 1);
    }
}

// a name as the cache file has it inlined: its length, then the characters (the terminator one
// past the end: the game's)
void ReadName(NiBinaryStream* file, char* buffer) {
    uint32_t length = 0;
    Read(file, &length, 4, 4);
    if (length == 0) {
        buffer[0] = 0;
    } else {
        Stream_LoadBytes(file, buffer, length);
        buffer[length + 1] = 0;
    }
}

// a name read as an NiFixedString and handed to an entry's setter
void ReadNameInto(NiBinaryStream* file, char* buffer, NiRefObject* entry,
                  void(__thiscall* set)(NiRefObject*, const re::Ptr32<char>*)) {
    ReadName(file, buffer);
    re::Ptr32<char> name = GlobalStringTable_AddString(buffer);
    set(entry, &name);
    FixedStringRelease(&name);
}

// ---- the maps and sets

// NiTMapBase::GetFirstPos: the first bucket's head
void* __fastcall MapGetFirstPos(const ProgramMap* self, re::Edx) {
    for (uint32_t i = 0; i < self->hashSize; i++) {
        if (self->buckets[i] != nullptr) {
            return self->buckets[i];
        }
    }
    return nullptr;
}

// NiTObjectSet<NiPointer>::OrderedRemoveAt: the ones after it moved down
void __fastcall ObjectSetOrderedRemoveAt(PrimitiveSet* self, re::Edx, uint32_t index) {
    auto* data = re::PtrCast<re::Ptr32<NiRefObject>*>(self->data);
    for (; index < self->count - 1; index++) {
        NiRefObject* current = data[index];
        if (current != data[index + 1]) {
            ReleaseRef(current);
            data[index] = data[index + 1];
            if (data[index] != nullptr) {
                Increment(&data[index]->refCount);
            }
        }
        data = re::PtrCast<re::Ptr32<NiRefObject>*>(self->data);
    }
    self->count--;
    re::Ptr32<NiRefObject>& last = re::PtrCast<re::Ptr32<NiRefObject>*>(self->data)[self->count];
    if (last != nullptr) {
        ReleaseRef(last);
        last = nullptr;
    }
}

// NiTFixedStringSet::RemoveAll: each name set to the empty one (from the table) and let go
void __fastcall FixedStringSetRemoveAll(PrimitiveSet* self, re::Edx) {
    for (uint32_t i = 0; i < self->alloced; i++) {
        re::Ptr32<char>& slot = re::PtrCast<re::Ptr32<char>*>(self->data)[i];
        if (slot != nullptr) {
            re::Ptr32<char> old = slot;
            slot = GlobalStringTable_AddString(nullptr);
            FixedStringRelease(&old);
        }
    }
    self->count = 0;
}

// the failure map's NewItem / DeleteItem (the key's handle 0 to start with)
BoolNode* __fastcall BoolMapNewItem(BoolMap*, re::Edx) {
    auto* node = static_cast<BoolNode*>(NiNew(sizeof(BoolNode)));
    node->value = 0;
    node->key = nullptr;
    return node;
}

void __fastcall BoolMapDeleteItem(BoolMap*, re::Edx, BoolNode* node) {
    FixedString_AssignText(&node->key, nullptr);
    node->value = 0;
    NiDelete(node, sizeof(BoolNode));
}

// the maps' deleting destructors (each map kind, then the base: RemoveAll by its vtable)
void* MapDestroy(ProgramMap* self, uintptr_t vtable, uint32_t flags) {
    if (vtable != 0) {
        self->vtable = reinterpret_cast<void*>(vtable);
        FixedStringMap_RemoveAll(self);
    }
    self->vtable = reinterpret_cast<void*>(kMapBaseVtable);
    FixedStringMap_RemoveAll(self);
    NiFree(self->buckets);
    if (flags & 1) {
        NiDelete(self, sizeof(ProgramMap));
    }
    return self;
}

void* __fastcall ProgramMapDeletingDestructor(ProgramMap* self, re::Edx, uint32_t flags) {
    return MapDestroy(self, kProgramMapVtable, flags);
}

void* __fastcall MapBaseDeletingDestructor(ProgramMap* self, re::Edx, uint32_t flags) {
    return MapDestroy(self, 0, flags);
}

void* __fastcall BoolMapDeletingDestructor(ProgramMap* self, re::Edx, uint32_t flags) {
    return MapDestroy(self, kBoolMapVtable, flags);
}

// the program map's GetNext: the key and value copied out, the position moved on (to the next
// bucket with anything, by the key's hash)
void __fastcall ProgramMapGetNext(ProgramMap* self, re::Edx, re::Ptr32<ProgramNode>* pos, re::Ptr32<char>* key,
                                  re::Ptr32<GPUProgramDescriptor>* value) {
    ProgramNode* node = *pos;
    FixedString_Assign(key, &node->key);
    GPUProgramDescriptor* old = *value;
    if (old != node->value) {
        ReleaseRef(old);
        *value = node->value;
        if (node->value != nullptr) {
            Increment(&node->value->refCount);
        }
    }
    if (node->next != nullptr) {
        *pos = node->next;
        return;
    }
    for (uint32_t i = uint32_t(re::PtrCast<uintptr_t>(node->key)) % self->hashSize + 1; i < self->hashSize;
         i++) {
        if (self->buckets[i] != nullptr) {
            *pos = self->buckets[i];
            return;
        }
    }
    *pos = nullptr;
}

// the failure map's SetAt
void __fastcall BoolMapSetAt(BoolMap* self, re::Edx, const re::Ptr32<char>* key, uint8_t value) {
    uint32_t bucket = uint32_t(re::PtrCast<uintptr_t>(*key)) % self->hashSize;
    for (BoolNode* node = self->buckets[bucket]; node != nullptr; node = node->next) {
        if (*key == node->key) {
            FixedString_Assign(&node->key, key);
            node->value = value;
            return;
        }
    }
    auto* node = Virtual<BoolNode*>(self, 1);
    FixedString_Assign(&node->key, key);
    node->value = value;
    node->next = self->buckets[bucket];
    self->buckets[bucket] = node;
    self->count++;
}

void __fastcall BoolMapGetNext(BoolMap* self, re::Edx, re::Ptr32<BoolNode>* pos, re::Ptr32<char>* key, uint8_t* value) {
    BoolNode* node = *pos;
    FixedString_Assign(key, &node->key);
    *value = node->value;
    if (node->next != nullptr) {
        *pos = node->next;
        return;
    }
    for (uint32_t i = uint32_t(re::PtrCast<uintptr_t>(node->key)) % self->hashSize + 1; i < self->hashSize;
         i++) {
        if (self->buckets[i] != nullptr) {
            *pos = self->buckets[i];
            return;
        }
    }
    *pos = nullptr;
}

// the program map's DeleteItem: the key emptied, the descriptor let go
void __fastcall ProgramMapDeleteItem(ProgramMap*, re::Edx, ProgramNode* node) {
    FixedString_AssignText(&node->key, nullptr);
    if (node->value != nullptr) {
        ReleaseRef(node->value);
        node->value = nullptr;
    }
    if (node != nullptr) {
        ReleaseRef(node->value);
        NiDelete(node, sizeof(ProgramNode));
    }
}

// ---- NiGPUProgramCache

// vf13: the vertex semantics there are
uint32_t __fastcall GetSemanticCount(NiGPUProgramCache*, re::Edx) {
    return 0xD;
}
// the working directory there, made when it isn't
uint8_t __fastcall EnsureDirectory(NiGPUProgramCache* self, re::Edx) {
    if (self->directory == kNullFixedString) {
        return 0;
    }
    if (NiFile_DirectoryExists(self->directory) != 0) {
        return 1;
    }
    return NiFile_CreateDirectoryRecursive(self->directory);
}

// vf11: a vertex semantic's name as the shader declares it (an NiString; unknown ones and
// TexCoord: "TEXCOORD")
void* __fastcall GetSemanticName(NiGPUProgramCache*, re::Edx, void* result, const re::Ptr32<char>* semantic) {
    static const char* const kNames[][2] = {
        {"Position", "POSITION"}, {"BlendWeight", "BLENDWEIGHT"}, {"BlendIndices", "BLENDINDICES"},
        {"Normal", "NORMAL"},     {"PSize", "PSIZE"},
    };
    static const char* const kLater[][2] = {
        {"Tangent", "TANGENT"}, {"Binormal", "BINORMAL"}, {"TessFactor", "TESSFACTOR"}, {"PositionT", "POSITIONT"},
        {"Color", "COLOR"},     {"Fog", "FOG"},           {"Depth", "DEPTH"},           {"Sample", "SAMPLE"},
    };
    for (const auto& name : kNames) {
        if (FixedString_Equals(semantic, name[0]) != 0) {
            NiString_Construct(result, name[1]);
            return result;
        }
    }
    if (FixedString_Equals(semantic, "TexCoord") == 0) {
        for (const auto& name : kLater) {
            if (FixedString_Equals(semantic, name[0]) != 0) {
                NiString_Construct(result, name[1]);
                return result;
            }
        }
    }
    NiString_Construct(result, "TEXCOORD");
    return result;
}

// vf12: a vertex semantic's index (0 Position .. 13 Sample; unknown: 6, Tangent's)
uint32_t __fastcall GetSemanticIndex(NiGPUProgramCache*, re::Edx, const re::Ptr32<char>* semantic) {
    static const char* const kNames[] = {"Position", "BlendWeight", "BlendIndices", "Normal", "PSize",
                                         "TexCoord", "Tangent",     "Binormal",     "TessFactor", "PositionT",
                                         "Color",    "Fog",         "Depth"};
    for (uint32_t i = 0; i < 13; i++) {
        if (FixedString_Equals(semantic, kNames[i]) != 0) {
            return i;
        }
    }
    return FixedString_Equals(semantic, "Sample") != 0 ? 13 : 6;
}

// vf22 AppendEntry: one program's entry added to the end of the cache file (no file yet: the
// whole cache saved first)
uint8_t __fastcall AppendEntry(NiGPUProgramCache* self, re::Edx, GPUProgramDescriptor* program, const re::Ptr32<char>* name) {
    if (self->path == kNullFixedString) {
        return 0;
    }
    if (NiFile_Access(self->path, 0) == 0) {
        if (NiFile_Access(self->path, 1) == 0 || Virtual<uint8_t>(self, 15) == 0) {
            return 0;
        }
    }
    NiBinaryStream* file = NiFile_GetFile(self->path, 2, 0x8000);
    if (file == nullptr) {
        return 0;
    }
    if (Virtual<uint8_t>(file, 1) != 1) {
        Virtual(file, 0, 1u);
        return 0;
    }
    uint8_t result = Virtual<uint8_t>(self, 23, file, program, name);
    Virtual(file, 0, 1u);
    return result;
}

// vf20: the constants whose name (+0x14) the program (vf3 count, vf4 name) doesn't use removed
uint8_t __fastcall RemoveUnusedConstants(NiGPUProgramCache*, re::Edx, void* program, PrimitiveSet* constants) {
    uint32_t count = constants->count;
    uint32_t used = Virtual<uint32_t>(program, 3);
    uint8_t keep[0x400];
    crt::Memset(keep, 0, sizeof(keep));
    for (uint32_t i = 0; i < count; i++) {
        auto* entry = static_cast<NiRefObject*>(constants->data[i]);
        if (entry == nullptr) {
            continue;
        }
        const char* name = *ConstantEntry_GetName14(entry);
        for (uint32_t j = 0; j < used; j++) {
            if (std::strcmp(name, Virtual<const char*>(program, 4, j)) == 0) {
                keep[i] = 1;
                break;
            }
        }
    }
    for (int32_t i = int32_t(count) - 1; i >= 0; i--) {
        if (keep[i] == 0) {
            ObjectSet_OrderedRemoveAt(constants, uint32_t(i));
        }
    }
    return 1;
}

// vf16 Clear: both maps emptied, not loaded
uint8_t __fastcall Clear(NiGPUProgramCache* self, re::Edx) {
    FixedStringMap_RemoveAll(&self->programs);
    FixedStringMap_RemoveAll(&self->failed);
    self->loaded = 0;
    return 1;
}

// vf8 SetWorkingDirectory: made absolute from the current directory when relative (failing:
// 0, the directory as it was); a change updates the path (vf21)
uint8_t __fastcall SetWorkingDirectory(NiGPUProgramCache* self, re::Edx, const re::Ptr32<char>* directory) {
    re::Ptr32<char> old = sys::FixedStringAddRef(self->directory);
    if (*directory != kNullFixedString && NiPath_IsRelative(*directory) != 0) {
        char current[0x104];
        char absolute[0x104];
        if (NiPath_GetCurrentWorkingDirectory(current, 0x104) == 0 ||
            NiPath_ConvertToAbsolute(absolute, 0x104, *directory, current) == 0) {
            FixedString_Destruct(&old);
            return 0;
        }
        FixedString_AssignText(&self->directory, absolute);
    } else {
        FixedString_Assign(&self->directory, directory);
    }
    if (old != self->directory) {
        Virtual(self, 21);
    }
    FixedStringRelease(&old);
    return 1;
}

// vf21 UpdatePath: the file's path from the directory and file name (either missing: none,
// not loaded); a change clears the cache (vf16)
void __fastcall UpdatePath(NiGPUProgramCache* self, re::Edx) {
    if (self->directory == kNullFixedString || self->fileName == kNullFixedString) {
        FixedString_AssignText(&self->path, nullptr);
        self->loaded = 0;
        return;
    }
    re::Ptr32<char> old = sys::FixedStringAddRef(self->path);
    char path[0x104];
    NiPath_ConvertToAbsolute(path, 0x104, self->fileName, self->directory);
    FixedString_AssignText(&self->path, path);
    if (old != self->path) {
        Virtual(self, 16);
    }
    FixedStringRelease(&old);
}

// a name written as the file has it: its length with the terminator, then the characters
// (none: a zero length)
uint8_t __fastcall SaveString(void*, re::Edx, NiBinaryStream* stream, const char* text) {
    if (text == nullptr) {
        uint32_t zero = 0;
        Write(stream, &zero, 4, 4);
        return 1;
    }
    uint32_t length = uint32_t(std::strlen(text)) + 1;
    Write(stream, &length, 4, 4);
    Write(stream, text, length, 1);
    return 1;
}

// a name read back (no buffer: 0); the terminator one past the end
uint8_t __fastcall LoadString(void*, re::Edx, NiBinaryStream* stream, char* buffer, uint32_t size) {
    if (size == 0 || buffer == nullptr) {
        return 0;
    }
    uint32_t length = 0;
    Read(stream, &length, 4, 4);
    if (length == 0) {
        buffer[0] = 0;
        return 1;
    }
    Stream_LoadBytes(stream, buffer, length);
    buffer[length + 1] = 0;
    return 1;
}

// vf14 Load: the cache file read once (the directory made if needed): its header, version (older
// than 6: closed, saved again when asked), the renderer's two names and the profile must match;
// then every entry (any out of date: saved again when asked)
uint8_t __fastcall Load(NiGPUProgramCache* self, re::Edx) {
    if (self->directory == kNullFixedString ||
        (NiFile_DirectoryExists(self->directory) == 0 && NiFile_CreateDirectoryRecursive(self->directory) == 0)) {
        return 0;
    }
    if (self->loaded != 0) {
        return 1;
    }
    NiBinaryStream* file = NiFile_GetFile(self->path, 0, 0x8000);
    if (file == nullptr) {
        return 0;
    }
    if (Virtual<uint8_t>(file, 1) != 1) {
        Virtual(file, 0, 1u);
        return 0;
    }
    char buffer[0x200];
    Stream_LoadBytes(file, buffer, uint32_t(std::strlen(kCacheHeader)) + 1);
    if (NiStricmp(buffer, kCacheHeader) != 0) {
        Virtual(file, 0, 1u);
        return 0;
    }
    uint8_t endian;
    uint32_t version;
    Stream_LoadUInt8(file, &endian);
    Stream_LoadUInt32(file, &version);
    if (version < 6) {
        Virtual(file, 0, 1u);
        if (gRewriteOutdatedCache != 0) {
            Virtual<uint8_t>(self, 15);
        }
        return 0;
    }
    Cache_LoadString(self, file, buffer, 0x200);
    if (NiStricmp(buffer, Virtual<const char*>(self, 10)) != 0) {
        Virtual(file, 0, 1u);
        return 0;
    }
    Cache_LoadString(self, file, buffer, 0x200);
    if (NiStricmp(buffer, Virtual<const char*>(self, 9)) != 0) {
        Virtual(file, 0, 1u);
        return 0;
    }
    Cache_LoadString(self, file, buffer, 0x200);
    if (NiStricmp(buffer, *Virtual<re::Ptr32<char>*>(self, 6)) != 0) {
        Virtual(file, 0, 1u);
        return 0;
    }
    bool outdated = false;
    while (true) {
        uint32_t size = Virtual<uint32_t>(file, 6);
        if (Virtual<uint32_t>(file, 3) >= size) {
            break;
        }
        uint8_t entryOutdated = 0;
        if (Virtual<uint8_t>(self, 24, file, &entryOutdated) == 0) {
            Virtual(file, 0, 1u);
            return 0;
        }
        if (entryOutdated != 0) {
            outdated = true;
        }
    }
    Virtual(file, 0, 1u);
    self->loaded = 1;
    if (outdated && gRewriteOutdatedCache != 0) {
        Virtual<uint8_t>(self, 15);
    }
    return 1;
}

// vf19 SetFailed: a name marked as failing to compile (appended to the file when asked)
void __fastcall SetFailed(NiGPUProgramCache* self, re::Edx, const char* name) {
    re::Ptr32<char> key = name != nullptr ? GlobalStringTable_AddString(name) : static_cast<char*>(kNullFixedString);
    BoolMap_SetAt(&self->failed, &key, 1);
    if (self->autoAppend != 0) {
        Virtual<uint8_t>(self, 22, static_cast<GPUProgramDescriptor*>(nullptr), &key);
    }
    FixedStringRelease(&key);
}

// vf15 Save: the whole cache written (header, little-endian flag, version 6, the renderer's
// names and the profile, every program, every failure). (A file that isn't good is left open.)
uint8_t __fastcall Save(NiGPUProgramCache* self, re::Edx) {
    if (self->directory == kNullFixedString ||
        (NiFile_DirectoryExists(self->directory) == 0 && NiFile_CreateDirectoryRecursive(self->directory) == 0)) {
        return 0;
    }
    NiBinaryStream* file = NiFile_GetFile(self->path, 1, 0x8000);
    if (file == nullptr || Virtual<uint8_t>(file, 1) != 1) {
        return 0;
    }
    Stream_SaveBytes(file, kCacheHeader, uint32_t(std::strlen(kCacheHeader)) + 1);
    uint8_t endian = 1;
    Write(file, &endian, 1, 1);
    uint32_t version = 6;
    Write(file, &version, 4, 4);
    Cache_SaveString(self, file, Virtual<const char*>(self, 10));
    Cache_SaveString(self, file, Virtual<const char*>(self, 9));
    Cache_SaveString(self, file, *Virtual<re::Ptr32<char>*>(self, 6));
    for (re::Ptr32<ProgramNode> pos = static_cast<ProgramNode*>(Map_GetFirstPos(&self->programs)); pos != nullptr;) {
        re::Ptr32<char> key = kNullFixedString;
        re::Ptr32<GPUProgramDescriptor> program = nullptr;
        ProgramMap_GetNext(&self->programs, &pos, &key, &program);
        Virtual<uint8_t>(self, 23, file, program, &key);
        FixedStringRelease(&key);
        ReleaseRef(program);
    }
    for (re::Ptr32<BoolNode> pos = static_cast<BoolNode*>(Map_GetFirstPos(&self->failed)); pos != nullptr;) {
        re::Ptr32<char> key = kNullFixedString;
        uint8_t value;
        BoolMap_GetNext(&self->failed, &pos, &key, &value);
        Virtual<uint8_t>(self, 23, file, static_cast<GPUProgramDescriptor*>(nullptr), &key);
        FixedStringRelease(&key);
    }
    Virtual(file, 0, 1u);
    return 1;
}

// vf23 SaveEntry: the version, the name, whether it failed (no program); a program: written by
// the renderer (vf25), then its constants (five names, four values each)
uint8_t __fastcall SaveEntry(NiGPUProgramCache* self, re::Edx, NiBinaryStream* file, GPUProgramDescriptor* program,
                             const re::Ptr32<char>* name) {
    Write(file, &self->version, 4, 4);
    Cache_SaveString(self, file, *name);
    uint8_t failed = program == nullptr;
    Write(file, &failed, 1, 1);
    if (failed == 1) {
        return 1;
    }
    if (Virtual<uint8_t>(self, 25, file, program->program) == 0) {
        return 0;
    }
    uint32_t count = program->constants.count;
    Write(file, &count, 4, 4);
    for (uint32_t i = 0; i < count; i++) {
        auto* entry = static_cast<NiRefObject*>(program->constants.data[i]);
        Cache_SaveString(self, file, *ConstantEntry_GetName8(entry));
        Cache_SaveString(self, file, *ConstantEntry_GetNameC(entry));
        Cache_SaveString(self, file, *ConstantEntry_GetName10(entry));
        Cache_SaveString(self, file, *ConstantEntry_GetName14(entry));
        Cache_SaveString(self, file, *ConstantEntry_GetName18(entry));
        uint32_t value = ConstantEntry_GetValue1C(entry);
        Write(file, &value, 4, 4);
        value = ConstantEntry_GetValue20(entry);
        Write(file, &value, 4, 4);
        value = ConstantEntry_GetValue24(entry);
        Write(file, &value, 4, 4);
        value = ConstantEntry_GetValue28(entry);
        Write(file, &value, 4, 4);
    }
    return 1;
}

// vf3 GetProgram: a cached program (its constants added to the set; not referenced), or none
// (then whether it's known to fail)
NiRefObject* __fastcall GetProgram(NiGPUProgramCache* self, re::Edx, const char* name, PrimitiveSet* constants,
                                   uint8_t* failed) {
    re::Ptr32<char> key = name != nullptr ? GlobalStringTable_AddString(name) : static_cast<char*>(kNullFixedString);
    re::Ptr32<GPUProgramDescriptor> program = nullptr;
    *failed = 0;
    if (ProgramMap_GetAt(&self->programs, &key, &program) != 0) {
        for (uint32_t i = 0; i < program->constants.count; i++) {
            ObjectSet_Add(constants, re::PtrCast<const re::Ptr32<ni::NiRefObject>*>(&program->constants.data[i]));
        }
        NiRefObject* result = program->program;
        if (Decrement(&program->refCount) == 0) {
            Virtual(program, 1);
        }
        FixedStringRelease(&key);
        return result;
    }
    for (BoolNode* node = self->failed.buckets[uint32_t(re::PtrCast<uintptr_t>(key)) % self->failed.hashSize];
         node != nullptr; node = node->next) {
        if (key == node->key) {
            *failed = node->value;
            break;
        }
    }
    ReleaseRef(program);
    FixedStringRelease(&key);
    return nullptr;
}

// the destructor
void __fastcall Destruct(NiGPUProgramCache* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiGPUProgramCacheVtable);
    FixedStringSet_RemoveAll(&self->semantics);
    if (self->semantics.data != nullptr) {
        NiFixedStringArray_Destroy(re::PtrCast<re::Ptr32<char>*>(self->semantics.data), 3);
    }
    FixedStringRelease(&self->profile);
    FixedStringRelease(&self->path);
    FixedStringRelease(&self->directory);
    FixedStringRelease(&self->fileName);
    self->failed.vtable = reinterpret_cast<void*>(kBoolMapVtable);
    FixedStringMap_RemoveAll(&self->failed);
    self->failed.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    FixedStringMap_RemoveAll(&self->failed);
    NiFree(self->failed.buckets);
    self->programs.vtable = reinterpret_cast<void*>(kProgramMapVtable);
    FixedStringMap_RemoveAll(&self->programs);
    self->programs.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    FixedStringMap_RemoveAll(&self->programs);
    NiFree(self->programs.buckets);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    Decrement(&ms_uiObjects);
}

// the constructor: the entries' version, whether failures are appended, and +0x4A; two maps of
// 37 buckets, no names, room for 10 semantics
NiGPUProgramCache* __fastcall Construct(NiGPUProgramCache* self, re::Edx, uint32_t version, uint8_t autoAppend,
                                        uint8_t field4A) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiGPUProgramCacheVtable);
    self->programs.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    self->programs.hashSize = 0x25;
    self->programs.count = 0;
    self->programs.buckets = static_cast<re::Ptr32<ProgramNode>*>(NiMalloc(0x94));
    crt::Memset(self->programs.buckets, 0, self->programs.hashSize * 4);
    self->programs.vtable = reinterpret_cast<void*>(kProgramMapVtable);
    self->failed.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    self->failed.hashSize = 0x25;
    self->failed.count = 0;
    self->failed.buckets = static_cast<re::Ptr32<BoolNode>*>(NiMalloc(0x94));
    crt::Memset(self->failed.buckets, 0, self->failed.hashSize * 4);
    self->failed.vtable = reinterpret_cast<void*>(kBoolMapVtable);
    self->fileName = kNullFixedString;
    self->directory = kNullFixedString;
    self->path = kNullFixedString;
    self->profile = kNullFixedString;
    self->semantics.data = reinterpret_cast<re::Ptr32<void>*>(NiFixedStringArray_New(10));
    self->semantics.count = 0;
    self->semantics.alloced = 10;
    self->field4A = field4A;
    self->loaded = 0;
    self->version = version;
    self->autoAppend = autoAppend;
    return self;
}

NiGPUProgramCache* __fastcall DeletingDestructor(NiGPUProgramCache* self, re::Edx, uint32_t flags) {
    Cache_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiGPUProgramCache));
    }
    return self;
}

// vf24 LoadEntry: one entry read back: its version (another: out of date), name, whether it
// failed (then only remembered); else the program read by the renderer (vf26; failing: 0) and
// its constants, kept when the entry is current and the program came back (out of date, or the
// name cached already: read past)
uint8_t __fastcall LoadEntry(NiGPUProgramCache* self, re::Edx, NiBinaryStream* file, uint8_t* outdatedOut) {
    char buffer[0x200];
    uint32_t version;
    Read(file, &version, 4, 4);
    ReadName(file, buffer);
    re::Ptr32<char> name = GlobalStringTable_AddString(buffer);
    // (the game pushes this flag as a whole dword whose upper bytes it never set)
    uint8_t outdated = 0;
    if (version != self->version) {
        *outdatedOut = 1;
        outdated = 1;
    }
    re::Ptr32<GPUProgramDescriptor> cached = nullptr;
    if (ProgramMap_GetAt(&self->programs, &name, &cached) != 0) {
        outdated = 1;
    }
    ReleaseRef(cached);
    uint8_t failed = 0;
    Read(file, &failed, 1, 1);
    if (failed == 1) {
        BoolMap_SetAt(&self->failed, &name, 1);
        FixedStringRelease(&name);
        return 1;
    }
    re::Ptr32<NiRefObject> program = nullptr;
    if (Virtual<uint8_t>(self, 26, file, &name, &program, uint32_t(outdated)) == 0) {
        ReleaseRef(program);
        FixedStringRelease(&name);
        return 0;
    }
    GPUProgramDescriptor* descriptor = nullptr;
    if (outdated == 0) {
        void* p = NiNew(sizeof(GPUProgramDescriptor));
        descriptor = p != nullptr ? Descriptor_Construct(p) : nullptr;
        NiPointer_AssignFrom(&descriptor->program, &program);
    }
    uint32_t count = 0;
    Read(file, &count, 4, 4);
    for (uint32_t i = 0; i < count; i++) {
        if (outdated != 0) {
            for (int n = 0; n < 5; n++) {
                ReadName(file, buffer);
            }
            uint32_t value;
            for (int n = 0; n < 4; n++) {
                Read(file, &value, 4, 4);
            }
            continue;
        }
        void* p = NiNew(0x2C);
        NiRefObject* entry = p != nullptr ? ConstantEntry_Construct(p) : nullptr;
        ReadNameInto(file, buffer, entry, ConstantEntry_SetName8);
        ReadNameInto(file, buffer, entry, ConstantEntry_SetNameC);
        ReadNameInto(file, buffer, entry, ConstantEntry_SetName10);
        ReadNameInto(file, buffer, entry, ConstantEntry_SetName14);
        ReadNameInto(file, buffer, entry, ConstantEntry_SetName18);
        uint32_t value;
        Read(file, &value, 4, 4);
        ConstantEntry_SetValue1C(entry, value);
        Read(file, &value, 4, 4);
        ConstantEntry_SetValue20(entry, value);
        Read(file, &value, 4, 4);
        ConstantEntry_SetValue24(entry, value);
        Read(file, &value, 4, 4);
        ConstantEntry_SetValue28(entry, value);
        re::Ptr32<NiRefObject> held = entry;
        if (entry != nullptr) {
            Increment(&entry->refCount);
        }
        ObjectSet_Add(&descriptor->constants, &held);
        ReleaseRef(entry);
    }
    if (outdated == 0 && descriptor->program != nullptr) {
        Increment(&descriptor->refCount);   // (the NiPointer passed by value)
        ProgramMap_SetAt(&self->programs, &name, descriptor);
    } else if (descriptor != nullptr) {
        Increment(&descriptor->refCount);
        if (Decrement(&descriptor->refCount) == 0) {
            Virtual(descriptor, 1);
        }
    }
    ReleaseRef(program);
    FixedStringRelease(&name);
    return 1;
}

// ---- the shader program factory's base (vtable 0x9A1A60): its one instance kept globally

void* __fastcall FactoryConstruct(void* self, re::Edx) {
    *static_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(kProgramFactoryVtable);
    gProgramFactory = self;
    return self;
}

void __fastcall FactoryDestruct(void* self, re::Edx) {
    *static_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(kProgramFactoryVtable);
    gProgramFactory = nullptr;
}

void* __fastcall FactoryDeletingDestructor(void* self, re::Edx, uint32_t flags) {
    *static_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(kProgramFactoryVtable);
    gProgramFactory = nullptr;
    if (flags & 1) {
        NiDelete(self, 4);
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIGPUCACHE_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// 0x008CA490: [8d 41 38 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void* __fastcall Tiny_008CA490(uint8_t* self, re::Edx) {
    return self + 0x38;
}
RE_UNHOOKED_FN("NiGPUProgramCache::GetSemantics", 0x008CA490, &Tiny_008CA490);      // 4 bytes: lea eax, [ecx+0x38]
// 0x008CA4A0: [8d 41 34 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void* __fastcall Tiny_008CA4A0(uint8_t* self, re::Edx) {
    return self + 0x34;
}
RE_UNHOOKED_FN("NiGPUProgramCache::GetProfile", 0x008CA4A0, &Tiny_008CA4A0);        // 4 bytes
NIGPUCACHE_REVERSED("NiGPUProgramCache::GetSemanticCount", 0x008CA4B0, &ni::GetSemanticCount);
// 0x008CA4C0: [8a 41 49 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_008CA4C0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x49);
}
RE_UNHOOKED_FN("NiGPUProgramCache::IsLoaded", 0x008CA4C0, &Tiny_008CA4C0);          // 4 bytes
// 0x008CA4D0: [8a 41 4a c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_008CA4D0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x4A);
}
RE_UNHOOKED_FN("NiGPUProgramCache::GetField4A", 0x008CA4D0, &Tiny_008CA4D0);        // 4 bytes
NIGPUCACHE_REVERSED("NiTFixedStringMap::GetFirstPos_8CA4E0", 0x008CA4E0, &ni::MapGetFirstPos);
NIGPUCACHE_REVERSED("NiGPUProgramCache::EnsureDirectory", 0x008CA510, &ni::EnsureDirectory);
NIGPUCACHE_REVERSED("NiGPUProgramCache::GetSemanticName", 0x008CA550, &ni::GetSemanticName);
NIGPUCACHE_REVERSED("NiGPUProgramCache::GetSemanticIndex", 0x008CA770, &ni::GetSemanticIndex);
NIGPUCACHE_REVERSED("NiGPUProgramCache::AppendEntry", 0x008CA8E0, &ni::AppendEntry);
NIGPUCACHE_REVERSED("NiTObjectSet::OrderedRemoveAt_8CA990", 0x008CA990, &ni::ObjectSetOrderedRemoveAt);
NIGPUCACHE_REVERSED("NiTFixedStringSet::RemoveAll", 0x008CAA30, &ni::FixedStringSetRemoveAll);
NIGPUCACHE_REVERSED("NiTFixedStringMap<bool>::NewItem", 0x008CAAB0, &ni::BoolMapNewItem);
NIGPUCACHE_REVERSED("NiTFixedStringMap<bool>::DeleteItem", 0x008CAAD0, &ni::BoolMapDeleteItem);
NIGPUCACHE_REVERSED("NiTFixedStringMap<NiGPUProgramDescriptorPtr>::DeletingDestructor", 0x008CAB00,
                    &ni::ProgramMapDeletingDestructor);
NIGPUCACHE_REVERSED("NiTFixedStringMapBase::DeletingDestructor_8CAB40", 0x008CAB40, &ni::MapBaseDeletingDestructor);
NIGPUCACHE_REVERSED("NiTFixedStringMap<bool>::DeletingDestructor", 0x008CAB80, &ni::BoolMapDeletingDestructor);
NIGPUCACHE_REVERSED("NiGPUProgramCache::RemoveUnusedConstants", 0x008CABC0, &ni::RemoveUnusedConstants);
NIGPUCACHE_REVERSED("NiGPUProgramCache::Clear", 0x008CACE0, &ni::Clear);
NIGPUCACHE_REVERSED("NiGPUProgramCache::SetWorkingDirectory", 0x008CAD00, &ni::SetWorkingDirectory);
NIGPUCACHE_REVERSED("NiGPUProgramCache::UpdatePath", 0x008CAE10, &ni::UpdatePath);
NIGPUCACHE_REVERSED("NiGPUProgramCache::SaveString", 0x008CAF00, &ni::SaveString);
NIGPUCACHE_REVERSED("NiGPUProgramCache::LoadString", 0x008CAFA0, &ni::LoadString);
NIGPUCACHE_REVERSED("NiTFixedStringMap<NiGPUProgramDescriptorPtr>::GetNext", 0x008CB020, &ni::ProgramMapGetNext);
NIGPUCACHE_REVERSED("NiTFixedStringMap<bool>::SetAt", 0x008CB0D0, &ni::BoolMapSetAt);
NIGPUCACHE_REVERSED("NiTFixedStringMap<bool>::GetNext", 0x008CB150, &ni::BoolMapGetNext);
NIGPUCACHE_REVERSED("NiGPUProgramCache::Load", 0x008CB1C0, &ni::Load);
NIGPUCACHE_REVERSED("NiGPUProgramCache::SetFailed", 0x008CB490, &ni::SetFailed);
NIGPUCACHE_REVERSED("NiGPUProgramCache::Save", 0x008CB510, &ni::Save);
NIGPUCACHE_REVERSED("NiGPUProgramCache::SaveEntry", 0x008CB780, &ni::SaveEntry);
NIGPUCACHE_REVERSED("NiGPUProgramCache::GetProgram", 0x008CB990, &ni::GetProgram);
NIGPUCACHE_REVERSED("NiTFixedStringMap<NiGPUProgramDescriptorPtr>::DeleteItem", 0x008CBB20,
                    &ni::ProgramMapDeleteItem);
NIGPUCACHE_REVERSED("NiGPUProgramCache::~NiGPUProgramCache", 0x008CBB90, &ni::Destruct);
NIGPUCACHE_REVERSED("NiGPUProgramCache::NiGPUProgramCache", 0x008CBCD0, &ni::Construct);
NIGPUCACHE_REVERSED("NiGPUProgramCache::DeletingDestructor", 0x008CBDB0, &ni::DeletingDestructor);
NIGPUCACHE_REVERSED("NiGPUProgramCache::LoadEntry", 0x008CBDD0, &ni::LoadEntry);
NIGPUCACHE_REVERSED("NiShaderProgramFactory::NiShaderProgramFactory", 0x008CC6E0, &ni::FactoryConstruct);
NIGPUCACHE_REVERSED("NiShaderProgramFactory::~NiShaderProgramFactory", 0x008CC6F0, &ni::FactoryDestruct);
NIGPUCACHE_REVERSED("NiShaderProgramFactory::DeletingDestructor", 0x008CC710, &ni::FactoryDeletingDestructor);
