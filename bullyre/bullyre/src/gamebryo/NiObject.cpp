// NiObject's own slots and helpers (vtable 0x951038): construction, cloning (NiCloningProcess:
// the clone map, then the process map), the link-ID LoadBinary of old files, deep copies
// through a memory stream, IsEqual by class name, the viewer strings; and NiTexture's viewer
// strings.

#include "gamebryo/NiFile.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kNiObjectVtable = 0x00951038;
GAME_VAR(long, ms_uiObjects, 0x020E2168);   // NiRefObject's count of live objects
inline void* const ms_NiObjectRTTI = reinterpret_cast<void*>(0x020E2248);
GAME_VAR(const char*, ms_NiObjectRTTIName, 0x020E2248);
GAME_VAR(const char*, ms_NiTextureRTTIName, 0x020E2184);

// the viewer strings (NiViewerStringsArray: a NiTPrimitiveArray<char*>)
GAME_FN(char*, __cdecl, NiGetViewerString, 0x00785560, const char* text);
GAME_FN(char*, __cdecl, NiGetViewerStringPointer, 0x00785790, const char* prefix, const void* value);
GAME_FN(char*, __cdecl, NiGetViewerStringUint, 0x007856F0, const char* prefix, uint32_t value);
GAME_FN(void, __thiscall, CharArray_SetSize, 0x0084A110, NiTArray<char*>* self, uint32_t size);
GAME_FN(void, __thiscall, CharArray_SetAt, 0x00833C70, NiTArray<char*>* self, uint32_t index, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, NiObjectNET_GetViewerStrings, 0x00764CA0, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, NiObject_GetViewerStrings, 0x007555A0, void* self, NiTArray<char*>* strings);

// NiTArray::Add as the library inlines it
inline void AddViewerString(NiTArray<char*>* strings, re::Ptr32<char> text) {
    uint32_t size = strings->size;
    if (size >= strings->maxSize) {
        CharArray_SetSize(strings, size + strings->growBy);
    }
    CharArray_SetAt(strings, size, &text);
}

// the maps of a NiCloningProcess (NiTPointerMap: vtable, hash size, buckets, count; a node:
// next, key, value)
struct NiTMapItem {
    re::Ptr32<NiTMapItem> next;   // +0x0
    re::Ptr32<void> key;          // +0x4
    uint32_t value;     // +0x8
};
struct NiTPointerMap {
    re::Ptr32<void> vtable;          // +0x0 (1 SetValue(item, key, value), 3 NewItem)
    uint32_t hashSize;     // +0x4
    re::Ptr32<re::Ptr32<NiTMapItem>> table;    // +0x8
    uint32_t count;        // +0xC
};

GAME_FN(void, __thiscall, NiCloningProcess_Construct, 0x006C7000, void* self, uint32_t hashSize);
GAME_FN(void, __thiscall, NiTPointerMap_SetAtBool, 0x00755310, NiTPointerMap* self, void* key, int value);
GAME_FN(void, __thiscall, NiTPointerMap_SetAtObject, 0x008A5140, NiTPointerMap* self, void* key, void* value);
GAME_FN(void*, __thiscall, NiStream_GetGroupFromLinkID, 0x00759DD0, void* stream, uint32_t linkID);
GAME_FN(void, __thiscall, NiStream_Construct, 0x0075CE40, void* self);
GAME_FN(void, __thiscall, NiStream_Destruct, 0x0075C020, void* self);
GAME_FN(void, __thiscall, NiStream_InsertObject, 0x0075D0D0, void* self, void* object);
GAME_FN(uint8_t, __thiscall, NiStream_SaveToMemory, 0x0075A210, void* self, re::Ptr32<void>* buffer, uint32_t* size);
GAME_FN(uint8_t, __thiscall, NiStream_LoadFromMemory, 0x007599F0, void* self, void* buffer, uint32_t size);

} // namespace

// 0x7551A0: NiTexture::GetViewerStrings
void __fastcall NiTexture_GetViewerStrings(void* self, re::Edx, NiTArray<char*>* strings) {
    NiObjectNET_GetViewerStrings(self, strings);
    AddViewerString(strings, NiGetViewerString(ms_NiTextureRTTIName));
}

// 0x7551F0: NiObject::Clone(NiCloningProcess&): CreateClone (slot 3), then ProcessClone (slot 11)
void* __fastcall NiObject_CloneWith(void* self, re::Edx, void* cloning) {
    void* clone = Virtual<void*>(self, 3, cloning);
    Virtual<void>(self, 11, cloning);
    return clone;
}

// 0x755220: NiObject::CreateClone (Bully's: the object itself)
void* __fastcall NiObject_CreateClone(void* self, re::Edx, void*) {
    return self;
}

// 0x755230: NiObject::RegisterStreamables: the stream's RegisterSaveObject (its slot 9)
uint8_t __fastcall NiObject_RegisterStreamables(void* self, re::Edx, void* stream) {
    return Virtual<uint8_t>(stream, 9, self);
}

// 0x755250: NiObject::GetStreamableRTTI (GetRTTI)
void* __fastcall NiObject_GetStreamableRTTI(void* self, re::Edx) {
    return Virtual<void*>(self, 2);
}

// 0x755260: NiObject's constructor
NiObject* __fastcall NiObject_Construct(NiObject* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiObjectVtable);
    return self;
}

// 0x755290: NiObject::GetRTTI
void* __fastcall NiObject_GetRTTI(void*, re::Edx) {
    return ms_NiObjectRTTI;
}

// 0x7552A0: NiObject::IsEqual: of the same class (by the RTTIs' names)
uint8_t __fastcall NiObject_IsEqual(void* self, re::Edx, void* other) {
    if (other == nullptr) {
        return 0;
    }
    const char* otherName = *Virtual<re::Ptr32<const char>*>(other, 2);
    const char* name = *Virtual<re::Ptr32<const char>*>(self, 2);
    return crt::Strcmp(name, otherName) == 0 ? 1 : 0;
}

// 0x755310: NiTPointerMap<NiObject*, bool>::SetAt
void __fastcall NiTPointerMap_SetAtBoolImpl(NiTPointerMap* self, re::Edx, void* key, int value) {
    uint32_t bucket = reinterpret_cast<uintptr_t>(key) % self->hashSize;
    for (NiTMapItem* item = self->table[bucket]; item != nullptr; item = item->next) {
        if (item->key == key) {
            *reinterpret_cast<uint8_t*>(&item->value) = uint8_t(value);
            return;
        }
    }
    NiTMapItem* item = Virtual<NiTMapItem*>(self, 3);
    Virtual<void>(self, 1, item, key, value);
    item->next = self->table[bucket];
    self->table[bucket] = item;
    self->count++;
}

// 0x755380: NiStreamLoadBinary(stream, uint&): 4 bytes from the stream's input
void __cdecl NiStreamLoadBinary_Uint(void* stream, uint32_t* value) {
    NiBinaryStream* in = At<NiBinaryStream*>(stream, 0x188);
    uint32_t componentSize = 4;
    in->absoluteCurrentPos += in->read(in, value, 4, &componentSize, 1);
}

// 0x7553B0: NiObject::CreateSharedClone: the clone made already, else CreateClone
void* __fastcall NiObject_CreateSharedClone(void* self, re::Edx, re::Ptr32<NiTPointerMap>* cloning) {
    NiTPointerMap* map = *cloning;
    for (NiTMapItem* item = map->table[reinterpret_cast<uintptr_t>(self) % map->hashSize]; item != nullptr;
         item = item->next) {
        if (item->key == self) {
            return reinterpret_cast<void*>(item->value);
        }
    }
    return Virtual<void*>(self, 3, cloning);
}

// 0x7553F0: NiObject::ProcessClone: marked processed
void __fastcall NiObject_ProcessClone(void* self, re::Edx, re::Ptr32<NiTPointerMap>* cloning) {
    NiTPointerMap_SetAtBool(cloning[1], self, 1);
}

// 0x755410: NiObject::CopyMembers: the clone recorded against the original
void __fastcall NiObject_CopyMembers(void* self, re::Edx, void* clone, re::Ptr32<NiTPointerMap>* cloning) {
    NiTPointerMap_SetAtObject(*cloning, self, clone);
}

// 0x755430: NiObject::CreateDeepCopy (an NiObjectPtr returned): saved to memory and loaded back
re::Ptr32<NiRefObject>* __fastcall NiObject_CreateDeepCopy(void* self, re::Edx, re::Ptr32<NiRefObject>* result) {
    alignas(4) uint8_t stream[0x40C];
    NiStream_Construct(stream);
    NiStream_InsertObject(stream, self);
    re::Ptr32<void> buffer = nullptr;
    uint32_t size = 0;
    NiStream_SaveToMemory(stream, &buffer, &size);
    NiStream_LoadFromMemory(stream, buffer, size);
    NiRefObject* copy = *At<re::Ptr32<NiRefObject>*>(stream, 0x15C);   // the first top-level object
    *result = copy;
    if (copy != nullptr) {
        Increment(&copy->refCount);
    }
    NiFree(buffer);
    NiStream_Destruct(stream);
    return result;
}

// 0x7554E0: NiObject::LoadBinary: files before 10.1.0.114 have a group link here (SetGroup,
// slot 17)
void __fastcall NiObject_LoadBinary(void* self, re::Edx, void* stream) {
    if (At<uint32_t>(stream, 0x14) < 0x0A010072) {
        uint32_t linkID;
        NiStreamLoadBinary_Uint(stream, &linkID);
        Virtual<void>(self, 17, NiStream_GetGroupFromLinkID(stream, linkID));
    }
}

// 0x755540: NiObject::Clone() (with its own NiCloningProcess, 37 buckets)
void* __fastcall NiObject_Clone(void* self, re::Edx) {
    alignas(4) void* cloning[6];
    NiCloningProcess_Construct(cloning, 0x25);
    void* clone = Virtual<void*>(self, 3, cloning);
    Virtual<void>(self, 11, cloning);
    if (cloning[0] != nullptr) {
        Virtual<void>(cloning[0], 0, 1);
    }
    if (cloning[1] != nullptr) {
        Virtual<void>(cloning[1], 0, 1);
    }
    return clone;
}

// 0x7555A0: NiObject::GetViewerStrings (the class, "this", the reference count)
void __fastcall NiObject_GetViewerStringsImpl(NiRefObject* self, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, NiGetViewerString(ms_NiObjectRTTIName));
    AddViewerString(strings, NiGetViewerStringPointer("this", self));
    AddViewerString(strings, NiGetViewerStringUint("m_uiRefCount", self->refCount));
}

#define NIOBJECT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIOBJECT_REVERSED("NiTexture::GetViewerStrings", 0x007551A0, NiTexture_GetViewerStrings);
NIOBJECT_REVERSED("NiObject::Clone(NiCloningProcess&)", 0x007551F0, NiObject_CloneWith);
NIOBJECT_REVERSED("NiObject::CreateClone", 0x00755220, NiObject_CreateClone);
NIOBJECT_REVERSED("NiObject::RegisterStreamables", 0x00755230, NiObject_RegisterStreamables);
NIOBJECT_REVERSED("NiObject::GetStreamableRTTI", 0x00755250, NiObject_GetStreamableRTTI);
NIOBJECT_REVERSED("NiObject::NiObject", 0x00755260, NiObject_Construct);
NIOBJECT_REVERSED("NiObject::GetRTTI", 0x00755290, NiObject_GetRTTI);
NIOBJECT_REVERSED("NiObject::IsEqual", 0x007552A0, NiObject_IsEqual);
NIOBJECT_REVERSED("NiTPointerMap<NiObject*,bool>::SetAt", 0x00755310, NiTPointerMap_SetAtBoolImpl);
NIOBJECT_REVERSED("NiStreamLoadBinary(uint)", 0x00755380, NiStreamLoadBinary_Uint);
NIOBJECT_REVERSED("NiObject::CreateSharedClone", 0x007553B0, NiObject_CreateSharedClone);
NIOBJECT_REVERSED("NiObject::ProcessClone", 0x007553F0, NiObject_ProcessClone);
NIOBJECT_REVERSED("NiObject::CopyMembers", 0x00755410, NiObject_CopyMembers);
NIOBJECT_REVERSED("NiObject::CreateDeepCopy", 0x00755430, NiObject_CreateDeepCopy);
NIOBJECT_REVERSED("NiObject::LoadBinary", 0x007554E0, NiObject_LoadBinary);
NIOBJECT_REVERSED("NiObject::Clone", 0x00755540, NiObject_Clone);
NIOBJECT_REVERSED("NiObject::GetViewerStrings", 0x007555A0, NiObject_GetViewerStringsImpl);

} // namespace ni
