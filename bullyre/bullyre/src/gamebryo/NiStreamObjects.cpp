// Three of NiStream's (the rest of the class is the other Gamebryo files'): registering an
// object and a fixed string for saving, and creating an object by its class name while loading.
//
// The parts of NiStream used here, by offset: the objects in load/save order (+0x128, an
// NiTLargeObjectArray<NiObjectPtr>: base +0x12C, allocated +0x130, size +0x134, growth +0x13C),
// the fixed strings to save (+0x170, an NiTLargePrimitiveArray: base +0x174, size +0x17C), the
// objects registered for saving (+0x1B0, an NiTPointerMap<NiObject*, unsigned int>: buckets
// count +0x1B4, buckets +0x1B8), the last error (+0x304) and its text (+0x308, 0x104 chars).

#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

GAME_FN(void, __thiscall, ObjectMap_SetAt, 0x008A5140, void* map, NiObject* key, uintptr_t value);
GAME_FN(void, __thiscall, ObjectArray_SetSize, 0x0075B990, void* array, uint32_t size);
GAME_FN(void, __thiscall, ObjectArray_SetAt, 0x0075B280, void* array, uint32_t index, const re::Ptr32<NiObject>* object);
GAME_FN(void, __thiscall, FixedStringArray_Add, 0x0075CD80, void* array, const re::Ptr32<const char>* string);
GAME_FN(void, __thiscall, Stream_LoadRTTIString, 0x0075AF40, void* stream, char* name);
GAME_FN(uint8_t, __thiscall, StringPointerMap_GetAt, 0x00800470, void* map, const char* key, re::Ptr32<void>* value);
GAME_VAR(void*, NiStream_ms_pkLoaders, 0x020E247C);   // class name -> create function

// the object put at the end of the object array (NiTLargeObjectArray::Add, inlined)
void AddObject(void* stream, NiObject* object) {
    void* array = static_cast<uint8_t*>(stream) + 0x128;
    re::Ptr32<NiObject> held = object;
    AddRef(object);
    uint32_t index = At<uint32_t>(array, 0xC);
    if (index >= At<uint32_t>(array, 8)) {
        ObjectArray_SetSize(array, At<uint32_t>(array, 0x14) + index);
    }
    ObjectArray_SetAt(array, index, &held);
    Release(object);
}

// RegisterSaveObject(pkObject): false when it was registered already
uint8_t __fastcall RegisterSaveObject(void* self, re::Edx, NiObject* object) {
    void* map = static_cast<uint8_t*>(self) + 0x1B0;
    re::Ptr32<void>* buckets = At<re::Ptr32<void>*>(map, 8);
    for (void* item = buckets[reinterpret_cast<uintptr_t>(object) % At<uint32_t>(map, 4)]; item != nullptr;
         item = At<void*>(item, 0)) {
        if (At<NiObject*>(item, 4) == object) {
            return 0;
        }
    }
    ObjectMap_SetAt(map, object, At<uint32_t>(self, 0x134));
    AddObject(self, object);
    return 1;
}

// RegisterFixedString(kString): added unless it's there already (the empty one always is added)
uint8_t __fastcall RegisterFixedString(void* self, re::Edx, const re::Ptr32<const char>* string) {
    void* array = static_cast<uint8_t*>(self) + 0x170;
    NiFixedString s = *string;
    if (s != NullFixedString) {
        uint32_t count = At<uint32_t>(self, 0x17C);
        re::Ptr32<const char>* strings = At<re::Ptr32<const char>*>(self, 0x174);
        for (uint32_t i = 0; i < count; i++) {
            if (s == strings[i]) {
                return 1;
            }
        }
    }
    FixedStringArray_Add(array, string);
    return 1;
}

// LoadObject: the class name read, its create function run, the object added and loaded
uint8_t __fastcall LoadObject(void* self, re::Edx) {
    char name[0x100];
    Stream_LoadRTTIString(self, name);
    re::Ptr32<void> create;
    if (!StringPointerMap_GetAt(NiStream_ms_pkLoaders, name, &create)) {
        char* error = static_cast<char*>(self) + 0x308;
        At<uint32_t>(self, 0x304) = 5;
        crt::StrcpyS(error, 0x104, name);
        crt::StrcatS(error, 0x104, ": cannot find create function.");
        return 0;
    }
    NiObject* object = reinterpret_cast<NiObject*(__cdecl*)()>(static_cast<void*>(create))();
    AddObject(self, object);
    Virtual<void>(object, 4, self);   // LoadBinary
    return 1;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTREAMOBJ_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTREAMOBJ_REVERSED("NiStream::RegisterSaveObject", 0x0075D140, &ni::RegisterSaveObject);
NISTREAMOBJ_REVERSED("NiStream::RegisterFixedString", 0x0075D1E0, &ni::RegisterFixedString);
NISTREAMOBJ_REVERSED("NiStream::LoadObject", 0x0075D240, &ni::LoadObject);
