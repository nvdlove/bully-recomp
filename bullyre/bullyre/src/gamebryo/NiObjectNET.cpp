// NiObjectNET (vtable 0x951F10, 0x18 bytes: NiCore.h): the name, the controllers (a chain through
// NiTimeController's next, +0x30) and the extra data (an array kept sorted by the name handles'
// values, found by binary search). Streaming and cloning of all three.
//
// Also NiProperty's own few (Update, GetViewerStrings), NiTimeController::SetNext, and Bully's
// NiRTTI kind-of bitsets (each NiRTTI: name +0, base +4, index +8, bits +0xC, next +0x30).

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
    uint32_t index;        // +0x8 (Bully's)
    uint32_t bits[9];      // +0xC (the indices of the classes this one is a kind of)
    re::Ptr32<RTTI> next;            // +0x30 (all of them, from 0x20E2814)
};
static_assert(offsetof(RTTI, next) == 0x30);

GAME_VAR(RTTI, NiObjectNET_RTTI, 0x020E2774);
GAME_VAR(RTTI, NiProperty_RTTI, 0x020E2820);
GAME_VAR(RTTI*, NiRTTI_ms_pkHead, 0x020E2814);
GAME_VAR(uint8_t, NiRTTI_ms_bKindOfBuilt, 0x020E2818);
GAME_VAR(RTTI, ExtraDataDroppedAtLink_RTTI, 0x020E38D4);   // the extra data PostLinkObject drops

constexpr uintptr_t NiObjectNET_vtable = 0x00951F10;
constexpr uintptr_t PointerArray_vtable = 0x009893C0;   // NiTObjectArray<NiPointer<...>>

using Ptr = NiObject*;
using PtrArray = NiTArray<NiObject*>;

inline NiObject* Controller(NiObjectNET* object) {
    return re::PtrCast<NiObject*>(object->controller);
}
inline re::Ptr32<NiObject>& ControllerSlot(NiObjectNET* object) {
    return reinterpret_cast<re::Ptr32<NiObject>&>(object->controller);
}
inline re::Ptr32<NiObject>* ExtraData(NiObjectNET* object) {
    return re::PtrCast<re::Ptr32<NiObject>*>(object->extraData);
}
inline re::Ptr32<const char>& Name(NiObject* extraData) {
    return At<NiFixedString>(extraData, 8);
}

GAME_FN(void, __thiscall, Object_DoNothing, 0x008A20A0, void* self, void* stream);   // NiObject's empty ones
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
GAME_FN(void, __thiscall, ControllerPointer_Destruct, 0x007549D0, re::Ptr32<NiObject>* self);
GAME_FN(uint32_t, __thiscall, Controller_Count, 0x0077DC10, NiObject* self);
GAME_FN(void, __thiscall, PtrArray_SetSize, 0x00856170, PtrArray* self, uint32_t size);
GAME_FN(void, __thiscall, PtrArray_SetAt, 0x0040E800, PtrArray* self, uint32_t index, const re::Ptr32<NiObject>* value);
GAME_FN(void, __cdecl, PtrArray_Free, 0x00836F80, re::Ptr32<NiObject>* base);
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<const char>* self, const char* text);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(char*, __cdecl, NiStrncpy, 0x00401020, char* to, size_t size, const char* from, size_t count);
// (this chunk's, called where modes apply)
GAME_FN(uint8_t, __thiscall, ObjectNET_InsertExtraData, 0x007640A0, NiObjectNET* self, NiObject* data);
GAME_FN(uint8_t, __thiscall, ObjectNET_SetExtraDataSize, 0x007641D0, NiObjectNET* self, uint32_t size);
GAME_FN(void, __thiscall, ObjectNET_DeleteAllExtraData, 0x00764360, NiObjectNET* self);
GAME_FN(uint8_t, __thiscall, ObjectNET_AddExtraDataNamed, 0x00764990, NiObjectNET* self, NiObject* data);
GAME_FN(void, __thiscall, Controller_SetNext, 0x00764500, NiObject* self, NiObject* next);

// the extra data, as the library walks it: from the first, stopping at an empty slot
template <typename F>
void ForEachExtraData(NiObjectNET* self, F f) {
    if (self->extraDataSize == 0) {
        return;
    }
    NiObject* data = ExtraData(self)[0];
    if (data == nullptr) {
        return;
    }
    uint32_t next = 1;
    for (;;) {
        f(data);
        if (next >= self->extraDataSize) {
            return;
        }
        data = ExtraData(self)[next];
        next++;
        if (data == nullptr) {
            return;
        }
    }
}

// ---- construction

NiObjectNET* __fastcall Construct(NiObjectNET* self, re::Edx) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiObjectNET_vtable);
    self->name = NullFixedString;
    self->controller = nullptr;
    self->extraData = nullptr;
    self->extraDataSize = 0;
    self->extraDataMax = 0;
    return self;
}

void __fastcall Destruct(NiObjectNET* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiObjectNET_vtable);
    ObjectNET_DeleteAllExtraData(self);
    Release(Controller(self));
    FixedStringRelease(&self->name);
    Object_Destruct(self);
}

NiObjectNET* __fastcall DeletingDestructor(NiObjectNET* self, re::Edx, uint32_t flags) {
    ObjectNET_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x18);
    }
    return self;
}

const RTTI* __fastcall GetRTTI(NiObjectNET*, re::Edx) {
    return &NiObjectNET_RTTI;
}

// ---- extra data

// RemoveExtraDataAt(usIndex): the rest moved down
void __fastcall RemoveExtraDataAt(NiObjectNET* self, re::Edx, uint32_t indexArg) {
    uint16_t index = uint16_t(indexArg);
    if (index >= self->extraDataSize) {
        return;
    }
    re::Ptr32<NiObject>* data = ExtraData(self);
    if (data[index] != nullptr) {
        Release(data[index]);
    }
    uint32_t i = index;
    while (int32_t(i) < int32_t(self->extraDataSize) - 1) {
        ExtraData(self)[i] = ExtraData(self)[i + 1];
        i = uint16_t(i + 1);
    }
    self->extraDataSize--;
    ExtraData(self)[self->extraDataSize] = nullptr;
}

// InsertExtraData(pkExtraData): in name order (false, and taken out again, for a name that's
// there already)
uint8_t __fastcall InsertExtraData(NiObjectNET* self, re::Edx, NiObject* data) {
    if (data == nullptr) {
        return 0;
    }
    AddRef(data);
    if (self->extraDataMax < 1) {
        self->extraDataMax = 1;
        self->extraDataSize = 1;
        re::Ptr32<NiObject>* array = static_cast<re::Ptr32<NiObject>*>(NiMalloc(4));
        self->extraData = reinterpret_cast<re::Ptr32<NiExtraData>*>(array);
        array[0] = data;
        return 1;
    }
    if (self->extraDataSize == self->extraDataMax) {
        uint16_t max = uint16_t(self->extraDataMax * 2 + 1);
        self->extraDataMax = max;
        re::Ptr32<NiObject>* array = static_cast<re::Ptr32<NiObject>*>(NiMalloc(uint32_t(max) * 4));
        crt::MemcpyS(array, uint32_t(self->extraDataMax) * 4, self->extraData, uint32_t(self->extraDataSize) * 4);
        NiFree(self->extraData);
        self->extraData = reinterpret_cast<re::Ptr32<NiExtraData>*>(array);
    }
    ExtraData(self)[self->extraDataSize] = data;
    self->extraDataSize++;
    for (uint16_t i = self->extraDataSize; i < self->extraDataMax; i++) {
        ExtraData(self)[i] = nullptr;
    }
    uint16_t i = uint16_t(self->extraDataSize - 1);
    while (i != 0) {
        re::Ptr32<NiObject>* array = ExtraData(self);
        int32_t difference = int32_t(uint32_t(re::PtrCast<uintptr_t>(Name(array[i - 1]))) -
                                     uint32_t(re::PtrCast<uintptr_t>(Name(array[i]))));
        if (difference == 0) {
            ObjectNET_RemoveExtraDataAt(self, i);
            return 0;
        }
        if (difference <= 0) {
            break;
        }
        NiObject* previous = array[i - 1];
        array[i - 1] = array[i];
        i = uint16_t(i - 1);
        ExtraData(self)[i + 1] = previous;
    }
    return 1;
}

// SetExtraDataSize(usSize): the array reallocated (its contents kept)
uint8_t __fastcall SetExtraDataSize(NiObjectNET* self, re::Edx, uint32_t sizeArg) {
    uint16_t size = uint16_t(sizeArg);
    if (size == 0) {
        return 0;
    }
    uint16_t old = self->extraDataMax;
    self->extraDataMax = size;
    if (old == 0) {
        self->extraData = static_cast<re::Ptr32<NiExtraData>*>(NiMalloc(uint32_t(size) * 4));
        self->extraDataSize = 0;
        return 1;
    }
    re::Ptr32<NiObject>* array = static_cast<re::Ptr32<NiObject>*>(NiMalloc(uint32_t(size) * 4));
    crt::MemcpyS(array, uint32_t(self->extraDataMax) * 4, self->extraData, uint32_t(self->extraDataSize) * 4);
    NiFree(self->extraData);
    self->extraData = reinterpret_cast<re::Ptr32<NiExtraData>*>(array);
    return 1;
}

// the binary search by the name handle (16-bit signed indices, as the game's)
int32_t FindExtraData(NiObjectNET* self, NiFixedString key) {
    int16_t high = int16_t(self->extraDataSize - 1);
    int16_t low = 0;
    if (high < 0) {
        return -1;
    }
    re::Ptr32<NiObject>* array = ExtraData(self);
    do {
        int16_t middle = int16_t((int32_t(low) + int32_t(high)) >> 1);
        int32_t difference = int32_t(uint32_t(re::PtrCast<uintptr_t>(key)) -
                                     uint32_t(re::PtrCast<uintptr_t>(Name(array[middle]))));
        if (difference == 0) {
            return uint16_t(middle);
        }
        if (difference > 0) {
            low = int16_t(middle + 1);
        } else {
            high = int16_t(middle - 1);
        }
    } while (low <= high);
    return -1;
}

// GetExtraData(kKey)
NiObject* __fastcall GetExtraData(NiObjectNET* self, re::Edx, const re::Ptr32<const char>* key) {
    if (*key == NullFixedString) {
        return nullptr;
    }
    int32_t found = FindExtraData(self, *key);
    return found >= 0 ? ExtraData(self)[int16_t(found)] : nullptr;
}

// RemoveExtraData(kKey)
uint8_t __fastcall RemoveExtraData(NiObjectNET* self, re::Edx, const re::Ptr32<const char>* key) {
    if (self->extraDataSize == 0) {
        return 0;
    }
    if (*key == NullFixedString) {
        return 0;
    }
    int32_t found = FindExtraData(self, *key);
    if (found < 0) {
        return 0;
    }
    ObjectNET_RemoveExtraDataAt(self, uint32_t(found));
    return 1;
}

// DeleteAllExtraData
void __fastcall DeleteAllExtraData(NiObjectNET* self, re::Edx) {
    for (uint32_t i = 0; i < self->extraDataSize; i++) {
        Release(ExtraData(self)[i]);
        ExtraData(self)[i] = nullptr;
    }
    NiFree(self->extraData);
    self->extraData = nullptr;
    self->extraDataMax = 0;
    self->extraDataSize = 0;
}

// AddExtraData(kKey, pkExtraData): an unnamed one gets the key
uint8_t __fastcall AddExtraData(NiObjectNET* self, re::Edx, const re::Ptr32<const char>* key, NiObject* data) {
    NiFixedString k = *key;
    if (k == NullFixedString) {
        return 0;
    }
    if (data == nullptr) {
        return 0;
    }
    NiFixedString name = Name(data);
    if (name == NullFixedString) {
        FixedString_Assign(&Name(data), key);
        return ObjectNET_InsertExtraData(self, data);
    }
    if (k == name) {
        return ObjectNET_InsertExtraData(self, data);
    }
    return 0;
}

// the extra data inserted, named after its class ("<class>ED%03d") when it has no name
uint8_t __fastcall AddExtraDataNamed(NiObjectNET* self, re::Edx, NiObject* data) {
    if (data == nullptr) {
        return 0;
    }
    if (Name(data) == nullptr) {
        const char* className = Virtual<const RTTI*>(data, 2)->name;
        if (className != nullptr && std::strlen(className) != 0) {
            char suffix[6];
            NiSprintf(suffix, 6, "ED%03d", uint32_t(self->extraDataSize));
            const char* at = crt::Strstr(className, "ExtraData");
            size_t prefix = at > className ? size_t(at - className) : 0;
            size_t size = std::strlen(suffix) + prefix + 1;
            char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
            NiStrncpy(text, size, className, prefix);
            crt::StrcatS(text, size, suffix);
            NiFixedString made;
            FixedString_Construct(&made, text);
            FixedString_Assign(&Name(data), &made);
            FixedString_Destruct(&made);
            NiFree(text);
        }
    }
    return ObjectNET_InsertExtraData(self, data);
}

// ---- controllers

// SetControllers(pkControl): the first controller replaced (NiSkinInstance::SetSkinPartition is
// the same code: its partition is at +0xC too)
void __fastcall SetControllers(NiObjectNET* self, re::Edx, NiObject* controller) {
    NiObject* old = Controller(self);
    if (old != controller) {
        Release(old);
        ControllerSlot(self) = controller;
        AddRef(controller);
    }
}

// PrependController: the controller put first
void __fastcall PrependController(NiObjectNET* self, re::Edx, NiObject* controller) {
    if (controller == nullptr) {
        return;
    }
    re::Ptr32<NiObject>& next = At<NiObject*>(controller, 0x30);
    NiObject* first = Controller(self);
    if (next != first) {
        Release(next);
        next = first;
        AddRef(first);
    }
    NiObject* old = Controller(self);
    if (old != controller) {
        Release(old);
        ControllerSlot(self) = controller;
        AddRef(controller);
    }
}

void __fastcall RemoveController(NiObjectNET* self, re::Edx, NiObject* controller) {
    if (controller == nullptr) {
        return;
    }
    NiObject* first = Controller(self);
    if (first == nullptr) {
        return;
    }
    if (first == controller) {
        _InterlockedIncrement(reinterpret_cast<volatile long*>(&controller->refCount));
        Pointer_Assign(&ControllerSlot(self), At<NiObject*>(controller, 0x30));
        re::Ptr32<NiObject>& next = At<NiObject*>(controller, 0x30);
        if (next != nullptr) {
            Release(next);
            next = nullptr;
        }
        if (_InterlockedDecrement(reinterpret_cast<volatile long*>(&controller->refCount)) == 0) {
            Virtual<void>(controller, 1);
        }
        return;
    }
    NiObject* previous = first;
    NiObject* current = At<NiObject*>(first, 0x30);
    while (current != nullptr && current != controller) {
        previous = current;
        current = At<NiObject*>(current, 0x30);
    }
    if (current == nullptr) {
        return;
    }
    re::Ptr32<NiObject> held = controller;
    AddRef(controller);
    Controller_SetNext(previous, At<NiObject*>(controller, 0x30));
    re::Ptr32<NiObject>& next = At<NiObject*>(controller, 0x30);
    if (next != nullptr) {
        Release(next);
        next = nullptr;
    }
    ControllerPointer_Destruct(&held);
}

// GetController(kRTTI): the first that is that kind
NiObject* __fastcall GetController(NiObjectNET* self, re::Edx, const RTTI* rtti) {
    for (NiObject* controller = Controller(self); controller != nullptr;
         controller = At<NiObject*>(controller, 0x30)) {
        const RTTI* its = Virtual<const RTTI*>(controller, 2);
        uint32_t index = rtti->index;
        if (its->bits[index >> 5] & (1u << (index & 0x1F))) {
            return controller;
        }
    }
    return nullptr;
}

// NiTimeController::SetNext
void __fastcall ControllerSetNext(NiObject* self, re::Edx, NiObject* next) {
    re::Ptr32<NiObject>& slot = At<NiObject*>(self, 0x30);
    if (slot != next) {
        Release(slot);
        slot = next;
        AddRef(next);
    }
}

// NiIsExactKindOf(pkRTTI, pkObject)
uint8_t __cdecl IsExactKindOf(const RTTI* rtti, NiObject* object) {
    if (object == nullptr) {
        return 0;
    }
    return Virtual<const RTTI*>(object, 2) == rtti ? 1 : 0;
}

// ---- streaming and cloning

// vf4
void __fastcall LoadBinary(NiObjectNET* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    if (StreamVersion(stream) < 0x14010001) {
        Stream_LoadInlineFixedString(stream, &self->name);
    } else {
        Stream_LoadFixedString(stream, &self->name);
    }
    Stream_ReadMultipleLinkIDs(stream);   // the extra data
    Stream_ReadLinkID(stream);            // the controller
}

// vf5
void __fastcall LinkObject(NiObjectNET* self, re::Edx, void* stream) {
    Object_DoNothing(self, stream);
    uint32_t count = Stream_GetNumberOfLinkIDs(stream);
    ObjectNET_SetExtraDataSize(self, count);
    for (; count != 0; count--) {
        ObjectNET_AddExtraDataNamed(self, static_cast<NiObject*>(Stream_ResolveLinkID(stream)));
    }
    NiObject* controller = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
    NiObject* old = Controller(self);
    if (old != controller) {
        Release(old);
        ControllerSlot(self) = controller;
        AddRef(controller);
    }
}

// vf6: the name, the streamable extra data (its slot 18), the controller
uint8_t __fastcall RegisterStreamables(NiObjectNET* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (!Virtual<uint8_t>(stream, 8, &self->name)) {   // RegisterFixedString
        return 0;
    }
    for (uint16_t i = 0; i < self->extraDataSize; i++) {
        NiObject* data = ExtraData(self)[i];
        if (data != nullptr && Virtual<uint8_t>(data, 18)) {
            Virtual<uint8_t>(data, 6, stream);
        }
    }
    if (Controller(self) != nullptr) {
        Virtual<uint8_t>(Controller(self), 6, stream);
    }
    return 1;
}

// vf7: the streamable extra data, then the first streamable controller (its slot 26)
void __fastcall SaveBinary(NiObjectNET* self, re::Edx, void* stream) {
    Object_DoNothing(self, stream);
    Stream_SaveFixedString(stream, &self->name);
    uint32_t count = 0;
    for (uint16_t i = 0; i < self->extraDataSize; i++) {
        if (Virtual<uint8_t>(ExtraData(self)[i], 18)) {
            count++;
        }
    }
    SaveBytes(stream, &count, 4);
    for (uint16_t i = 0; i < self->extraDataSize; i++) {
        NiObject* data = ExtraData(self)[i];
        if (Virtual<uint8_t>(data, 18)) {
            Virtual<void>(stream, 12, data);   // SaveLinkID
        }
    }
    NiObject* controller = Controller(self);
    while (controller != nullptr && !Virtual<uint8_t>(controller, 26)) {
        controller = At<NiObject*>(controller, 0x30);
    }
    Virtual<void>(stream, 12, controller);
}

// vf12 PostLinkObject: the extra data of one class dropped
void __fastcall PostLinkObject(NiObjectNET* self, re::Edx, void* stream) {
    Object_DoNothing(self, stream);
    if (self->extraDataSize == 0) {
        return;
    }
    NiObject* data = ExtraData(self)[0];
    if (data == nullptr) {
        return;
    }
    uint32_t index = 0;
    uint32_t next = 1;
    for (;;) {
        if (Virtual<const RTTI*>(data, 2) == &ExtraDataDroppedAtLink_RTTI && index < self->extraDataSize) {
            ObjectNET_RemoveExtraDataAt(self, index);
            index--;
            next--;
        }
        if (next >= self->extraDataSize) {
            return;
        }
        data = ExtraData(self)[index + 1];
        index++;
        next++;
        if (data == nullptr) {
            return;
        }
    }
}

// vf8
uint8_t __fastcall IsEqual(NiObjectNET* self, re::Edx, NiObjectNET* other) {
    if (!Object_IsEqual(self, other) || self->name != other->name) {
        return 0;
    }
    PtrArray mine{reinterpret_cast<void*>(PointerArray_vtable), nullptr, 0, 0, 0, 1};
    PtrArray theirs{reinterpret_cast<void*>(PointerArray_vtable), nullptr, 0, 0, 0, 1};
    auto collect = [](NiObjectNET* object, PtrArray* array) {
        ForEachExtraData(object, [array](NiObject* data) {
            if (Virtual<uint8_t>(data, 18)) {
                re::Ptr32<NiObject> held = data;
                AddRef(data);
                uint32_t index = array->size;
                if (array->maxSize <= index) {
                    PtrArray_SetSize(array, array->growBy + index);
                }
                PtrArray_SetAt(array, index, &held);
                Release(data);
            }
        });
    };
    collect(self, &mine);
    collect(other, &theirs);
    uint8_t equal = 0;
    if (mine.size == theirs.size) {
        bool same = true;
        for (uint16_t i = 0; i < mine.size; i++) {
            if (!Virtual<uint8_t>(mine.base[i], 8, theirs.base[i])) {
                same = false;
                break;
            }
        }
        if (same) {
            NiObject* a = Controller(self);
            NiObject* b = Controller(other);
            uint32_t countA = a != nullptr ? Controller_Count(a) : 0;
            uint32_t countB = b != nullptr ? Controller_Count(b) : 0;
            if (countA == countB) {
                equal = 1;
                while (a != nullptr && b != nullptr) {
                    if (!Virtual<uint8_t>(a, 8, b)) {
                        equal = 0;
                        break;
                    }
                    a = At<NiObject*>(a, 0x30);
                    b = At<NiObject*>(b, 0x30);
                }
            }
        }
    }
    PtrArray_Free(theirs.base);
    PtrArray_Free(mine.base);
    return equal;
}

// vf9
void __fastcall GetViewerStrings(NiObjectNET* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiObjectNET_RTTI.name));
    AddViewerString(strings, GetViewerStringString("m_kName", self->name));
    ForEachExtraData(self, [strings](NiObject* data) { Virtual<void>(data, 9, strings); });
    for (NiObject* controller = Controller(self); controller != nullptr;
         controller = At<NiObject*>(controller, 0x30)) {
        Virtual<void>(controller, 9, strings);
    }
}

// vf11 ProcessClone
void __fastcall ProcessClone(NiObjectNET* self, re::Edx, void* cloning) {
    Object_ProcessClone(self, cloning);
    ForEachExtraData(self, [cloning](NiObject* data) { Virtual<void>(data, 11, cloning); });
    if (Controller(self) != nullptr) {
        Virtual<void>(Controller(self), 11, cloning);
    }
}

// CopyMembers(pkDest, kCloning): the name as the cloning says (+8: 1 the same, 2 with the append
// character +0x14), the cloneable extra data's clones (slot 19), the controllers' clone
void __fastcall CopyMembers(NiObjectNET* self, re::Edx, NiObjectNET* dest, void* cloning) {
    Object_CopyMembers(self, dest, cloning);
    uint32_t copyType = At<uint32_t>(cloning, 8);
    if (copyType == 1) {
        FixedString_Assign(&dest->name, &self->name);
    } else if (copyType == 2 && self->name != nullptr) {
        size_t size = std::strlen(self->name) + 2;
        char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
        crt::StrcpyS(text, size, self->name);
        text[size - 2] = At<char>(cloning, 0x14);
        text[size - 1] = 0;
        NiFixedString made = text != nullptr ? StringTable_AddString(text) : static_cast<const char*>(NullFixedString);
        FixedString_Assign(&dest->name, &made);
        FixedString_Destruct(&made);
        NiFree(text);
    }
    ForEachExtraData(self, [dest, cloning](NiObject* data) {
        if (Virtual<uint8_t>(data, 19)) {
            ObjectNET_AddExtraDataNamed(dest, Virtual<NiObject*>(data, 3, cloning));
        }
    });
    if (Controller(self) != nullptr) {
        NiObject* clone = Virtual<NiObject*>(Controller(self), 3, cloning);
        NiObject* old = Controller(dest);
        if (old != clone) {
            Release(old);
            ControllerSlot(dest) = clone;
            AddRef(clone);
        }
    }
}

// ---- NiProperty's

// Update(fTime): its controllers
void __fastcall PropertyUpdate(NiObjectNET* self, re::Edx, float time) {
    for (NiObject* controller = Controller(self); controller != nullptr;
         controller = At<NiObject*>(controller, 0x30)) {
        Virtual<void>(controller, 20, time);
    }
}

void __fastcall PropertyGetViewerStrings(NiObjectNET* self, re::Edx, NiTArray<char*>* strings) {
    ObjectNET_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiProperty_RTTI.name));
}

// ---- Bully's NiRTTI kind-of bits: each class's bits get the indices of itself and its bases
void __cdecl BuildKindOfBits() {
    NiRTTI_ms_bKindOfBuilt = 1;
    for (RTTI* rtti = NiRTTI_ms_pkHead; rtti != nullptr; rtti = rtti->next) {
        for (RTTI* other = NiRTTI_ms_pkHead; other != nullptr; other = other->next) {
            for (const RTTI* base = rtti; base != nullptr; base = base->base) {
                if (base == other) {
                    rtti->bits[other->index >> 5] |= 1u << (other->index & 0x1F);
                    break;
                }
            }
        }
    }
}

} // namespace

RE_UNHOOKED("NiProperty::CopyMembers", 0x007654C0);          // a jmp to NiObjectNET's
RE_UNHOOKED("NiProperty::RegisterStreamables", 0x007654D0);  // a jmp
RE_UNHOOKED("NiProperty::SaveBinary", 0x007654E0);           // a jmp
RE_UNHOOKED("NiProperty::IsEqual", 0x007654F0);              // a jmp

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NINET_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NINET_REVERSED("NiObjectNET::ProcessClone", 0x00763FB0, &ni::ProcessClone);
NINET_REVERSED("NiObjectNET::RemoveExtraDataAt", 0x00764010, &ni::RemoveExtraDataAt);
NINET_REVERSED("NiObjectNET::InsertExtraData", 0x007640A0, &ni::InsertExtraData);
NINET_REVERSED("NiObjectNET::SetExtraDataSize", 0x007641D0, &ni::SetExtraDataSize);
NINET_REVERSED("NiObjectNET::GetExtraData", 0x00764250, &ni::GetExtraData);
NINET_REVERSED("NiObjectNET::RemoveExtraData", 0x007642D0, &ni::RemoveExtraData);
NINET_REVERSED("NiObjectNET::DeleteAllExtraData", 0x00764360, &ni::DeleteAllExtraData);
NINET_REVERSED("NiObjectNET::SetControllers", 0x007643C0, &ni::SetControllers);
NINET_REVERSED("NiObjectNET::LoadBinary", 0x00764410, &ni::LoadBinary);
NINET_REVERSED("NiObjectNET::RegisterStreamables", 0x00764460, &ni::RegisterStreamables);
NINET_REVERSED("NiIsExactKindOf", 0x007644E0, &ni::IsExactKindOf);
NINET_REVERSED("NiTimeController::SetNext", 0x00764500, &ni::ControllerSetNext);
NINET_REVERSED("NiObjectNET::NiObjectNET", 0x00764550, &ni::Construct);
NINET_REVERSED("NiObjectNET::GetRTTI", 0x00764580, &ni::GetRTTI);
NINET_REVERSED("NiObjectNET::~NiObjectNET", 0x00764590, &ni::Destruct);
NINET_REVERSED("NiObjectNET::AddExtraData", 0x00764600, &ni::AddExtraData);
NINET_REVERSED("NiObjectNET::PrependController", 0x00764660, &ni::PrependController);
NINET_REVERSED("NiObjectNET::RemoveController", 0x007646E0, &ni::RemoveController);
NINET_REVERSED("NiObjectNET::GetController", 0x007647D0, &ni::GetController);
NINET_REVERSED("NiObjectNET::SaveBinary", 0x00764820, &ni::SaveBinary);
NINET_REVERSED("NiObjectNET::PostLinkObject", 0x00764900, &ni::PostLinkObject);
NINET_REVERSED("NiObjectNET::DeletingDestructor", 0x00764970, &ni::DeletingDestructor);
NINET_REVERSED("NiObjectNET::AddExtraDataNamed", 0x00764990, &ni::AddExtraDataNamed);
NINET_REVERSED("NiObjectNET::LinkObject", 0x00764AC0, &ni::LinkObject);
NINET_REVERSED("NiObjectNET::CopyMembers", 0x00764B40, &ni::CopyMembers);
NINET_REVERSED("NiObjectNET::GetViewerStrings", 0x00764CA0, &ni::GetViewerStrings);
NINET_REVERSED("NiObjectNET::IsEqual", 0x00764D90, &ni::IsEqual);
NINET_REVERSED("NiRTTI::BuildKindOfBits", 0x00765460, &ni::BuildKindOfBits);
NINET_REVERSED("NiProperty::Update", 0x00765500, &ni::PropertyUpdate);
NINET_REVERSED("NiProperty::GetViewerStrings", 0x00765530, &ni::PropertyGetViewerStrings);
