#pragma once

// What the scene-graph files (NiAVObject.cpp, NiNode.cpp, NiGeometry.cpp, NiCamera.cpp,
// NiObjectNET.cpp, NiTexturingProperty.cpp...) share: the code the library inlines all over
// (NiFixedString's reference counting, NiStream's binary reads and writes, NiTArray::Add on
// the viewer strings) and the callees from other parts of the library, at their addresses.
// Everything in ni::scene so nothing clashes with the other Gamebryo files.

#include <cstddef>
#include <cstdint>
#include <intrin.h>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni::scene {

using re::At;

// a virtual call by slot (thiscall), as the library's code makes them
template <typename R = void, typename... A>
inline R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

// ---- NiFixedString (the handle points at the characters; the reference count is 8 bytes
// before them, the length 2 bytes before them)

GAME_VAR(const char*, NullFixedString, 0x00951028);   // the empty handle the code compares with
GAME_VAR(void*, GlobalStringTable, 0x020E216C);
// NiGlobalStringTable::RemoveString (the last outside reference went)
GAME_FN(void, __thiscall, StringTable_RemoveString, 0x00754550, void* table, re::Ptr32<const char>* handle, uint32_t length);
GAME_FN(const char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);   // (a native return: a Ptr32 by value is returned through memory on x64)
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<const char>* self);

// a handle let go (NiFixedString's destructor, inlined)
inline void FixedStringRelease(re::Ptr32<const char>* handle) {
    const char* s = *handle;
    if (s != NullFixedString) {
        uint32_t length = *reinterpret_cast<const uint16_t*>(s - 2);
        if (_InterlockedDecrement(reinterpret_cast<volatile long*>(const_cast<char*>(s) - 8)) == 1) {
            StringTable_RemoveString(GlobalStringTable, handle, length);
        }
    }
}

// ---- NiStream: its version (+0x14) and the binary streams it reads from (+0x188) and writes
// to (+0x18C). Each read or write: the stream's function, then its position moved on

struct BinaryStream {
    re::Ptr32<void> vtable;
    uint32_t position;   // +0x4
    re::Ptr32<uint32_t __cdecl(BinaryStream* stream, void* buffer, uint32_t bytes, uint32_t* componentSizes, uint32_t components)> read;         // +0x8
    re::Ptr32<uint32_t __cdecl(BinaryStream* stream, const void* buffer, uint32_t bytes, uint32_t* componentSizes, uint32_t components)> write;   // +0xC
};

inline uint32_t StreamVersion(const void* stream) {
    return At<uint32_t>(stream, 0x14);
}

// NiStreamLoadBinary(stream, value): one component of `size` bytes
inline void LoadBytes(void* stream, void* buffer, uint32_t size) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t componentSize = size;
    in->position += in->read(in, buffer, size, &componentSize, 1);
}

inline void SaveBytes(void* stream, const void* buffer, uint32_t size) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t componentSize = size;
    out->position += out->write(out, buffer, size, &componentSize, 1);
}

// NiStream's own (the other chunk's)
GAME_FN(void, __thiscall, Stream_ReadLinkID, 0x0075A7B0, void* stream);
GAME_FN(void, __thiscall, Stream_ReadMultipleLinkIDs, 0x0075A870, void* stream);
GAME_FN(void*, __thiscall, Stream_ResolveLinkID, 0x00759D80, void* stream);
GAME_FN(uint32_t, __thiscall, Stream_GetNumberOfLinkIDs, 0x00759DB0, void* stream);
GAME_FN(void, __thiscall, Stream_AddLinkIDBlock, 0x0075A900, void* stream, uint32_t count);
GAME_FN(void, __thiscall, Stream_SaveFixedString, 0x0075A260, void* stream, const re::Ptr32<const char>* string);
GAME_FN(void, __thiscall, Stream_LoadInlineFixedString, 0x0075B0B0, void* stream, re::Ptr32<const char>* string);
GAME_FN(void, __thiscall, Stream_LoadFixedString, 0x0075B040, void* stream, re::Ptr32<const char>* string);
// (NiStream's virtuals the code calls by slot: 8 RegisterFixedString, 9 RegisterSaveObject,
// 12 SaveLinkID)

// ---- the viewer strings (NiViewerStringsArray: NiTPrimitiveArray<char*>, NiTArray's layout)

GAME_FN(void, __thiscall, StringArray_SetSize, 0x0084A110, NiTArray<char*>* array, uint32_t size);
GAME_FN(void, __thiscall, StringArray_SetAt, 0x00833C70, NiTArray<char*>* array, uint32_t index, const re::Ptr32<char>* value);
GAME_FN(uint32_t, __thiscall, StringArray_Add, 0x004099B0, NiTArray<char*>* array, const re::Ptr32<char>* value);
GAME_FN(char*, __cdecl, GetViewerString, 0x00785560, const char* text);
GAME_FN(char*, __cdecl, GetViewerStringBool, 0x007855A0, const char* prefix, int value);
GAME_FN(char*, __cdecl, GetViewerStringUint, 0x007856F0, const char* prefix, uint32_t value);
GAME_FN(char*, __cdecl, GetViewerStringFloat, 0x00785740, const char* prefix, float value);
GAME_FN(char*, __cdecl, GetViewerStringPointer, 0x00785790, const char* prefix, const void* value);
GAME_FN(char*, __cdecl, GetViewerStringString, 0x007857E0, const char* prefix, const char* value);
GAME_FN(char*, __thiscall, Point3_GetViewerString, 0x00776920, const NiPoint3* self, const char* prefix);
GAME_FN(char*, __thiscall, Matrix3_GetViewerString, 0x00769160, const NiMatrix3* self, const char* prefix);
GAME_FN(char*, __thiscall, Bound_GetViewerString, 0x007D2450, const NiBound* self, const char* prefix);

// NiTArray::Add as most callers have it: grown by growBy when full, then SetAt
inline void AddViewerString(NiTArray<char*>* strings, re::Ptr32<char> value) {
    uint32_t index = strings->size;
    if (strings->maxSize <= index) {
        StringArray_SetSize(strings, strings->growBy + index);
    }
    StringArray_SetAt(strings, index, &value);
}

// the same with SetAt inlined (as some callers have it)
inline void AddViewerStringInline(NiTArray<char*>* strings, char* value) {
    uint16_t size = strings->size;
    uint32_t index = size;
    if (strings->maxSize <= index) {
        StringArray_SetSize(strings, strings->growBy + index);
    }
    if (index < strings->size) {
        if (value == nullptr) {
            if (strings->base[index] != nullptr) {
                strings->esize--;
            }
        } else if (strings->base[index] == nullptr) {
            strings->esize++;
        }
    } else {
        strings->size = uint16_t(size + 1);
        if (value != nullptr) {
            strings->esize++;
        }
    }
    strings->base[index] = value;
}

// ---- NiObject's base implementations (the other chunk's)

GAME_FN(void, __thiscall, Object_LoadBinary, 0x007554E0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Object_RegisterStreamables, 0x00755230, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Object_IsEqual, 0x007552A0, void* self, void* other);
GAME_FN(void, __thiscall, Object_GetViewerStrings, 0x007555A0, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Object_CopyMembers, 0x00755410, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Object_ProcessClone, 0x007553F0, void* self, void* cloning);
GAME_FN(void, __thiscall, RefObject_Construct, 0x00755260, void* self);
GAME_FN(void, __thiscall, Object_Destruct, 0x007769D0, void* self);   // ~NiObject

// NiObjectNET's (this chunk's, called where modes apply)
GAME_FN(void, __thiscall, ObjectNET_Construct, 0x00764550, void* self);
GAME_FN(void, __thiscall, ObjectNET_Destruct, 0x00764590, void* self);
GAME_FN(void, __thiscall, ObjectNET_LoadBinary, 0x00764410, void* self, void* stream);
GAME_FN(void, __thiscall, ObjectNET_LinkObject, 0x00764AC0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, ObjectNET_RegisterStreamables, 0x00764460, void* self, void* stream);
GAME_FN(void, __thiscall, ObjectNET_SaveBinary, 0x00764820, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, ObjectNET_IsEqual, 0x00764D90, void* self, void* other);
GAME_FN(void, __thiscall, ObjectNET_GetViewerStrings, 0x00764CA0, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, ObjectNET_CopyMembers, 0x00764B40, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, ObjectNET_ProcessClone, 0x00763FB0, void* self, void* cloning);
GAME_FN(void, __thiscall, ObjectNET_PostLinkObject, 0x00764900, void* self, void* stream);
GAME_FN(void, __thiscall, ObjectNET_RemoveExtraDataAt, 0x00764010, void* self, uint32_t index);

// NiAVObject's (this chunk's)
GAME_FN(void, __thiscall, AVObject_Construct, 0x0075E750, void* self);
GAME_FN(void, __thiscall, AVObject_Destruct, 0x0075DEF0, void* self);
GAME_FN(void, __thiscall, AVObject_LoadBinary, 0x0075E350, void* self, void* stream);
GAME_FN(void, __thiscall, AVObject_LinkObject, 0x0075E3F0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, AVObject_RegisterStreamables, 0x0075DCC0, void* self, void* stream);
GAME_FN(void, __thiscall, AVObject_SaveBinary, 0x0075E4C0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, AVObject_IsEqual, 0x0075E5E0, void* self, void* other);
GAME_FN(void, __thiscall, AVObject_GetViewerStrings, 0x0075EB60, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, AVObject_CopyMembers, 0x0075EAE0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, AVObject_ProcessClone, 0x0075E310, void* self, void* cloning);
GAME_FN(void, __thiscall, AVObject_UpdateObjectControllers, 0x0075DD20, void* self, float time, int properties);
GAME_FN(void, __thiscall, AVObject_SetSelectiveUpdateFlags, 0x0075E980, void* self, uint8_t* selectiveUpdate,
        int selectiveUpdateTransforms, uint8_t* rigid);
GAME_FN(void, __thiscall, AVObject_PurgeRendererData, 0x0075DCA0, void* self, void* renderer);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, AVObject_PushLocalProperties, 0x0075E140, void* self, re::Ptr32<NiObject>* result,
        NiObject* parentState, int copy);
GAME_FN(void, __thiscall, AVObject_SetCollisionObject, 0x0075DD80, void* self, NiObject* collision);
GAME_FN(void, __thiscall, AVObject_AttachParent, 0x0075DBA0, void* self, void* parent);
GAME_FN(NiObject*, __thiscall, AVObject_GetObjectByName, 0x0075DC70, void* self, const re::Ptr32<const char>* name);
GAME_FN(uint8_t, __thiscall, AVObject_HasTransformController, 0x0075E0E0, void* self);
GAME_FN(uint8_t, __thiscall, AVObject_HasPropertyController, 0x0075E110, void* self);

} // namespace ni::scene
