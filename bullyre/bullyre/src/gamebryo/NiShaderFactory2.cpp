// NiShaderFactory's constructor and destructor (the singleton's: its libraries and shaders
// dropped through its slots 8 and 13, every global shader constant released, its map emptied).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiShaderFactory_vtable = 0x00951338;
constexpr uintptr_t ConstantMapBase_vtable = 0x009A1978;   // NiTFixedStringMap's base
constexpr uintptr_t ConstantMap_vtable = 0x009A162C;       // NiTFixedStringMap<NiGlobalConstantEntry*>

GAME_VAR(long, ms_uiObjects, 0x020E2168);

struct MapItem;
struct NiShaderFactory {
    re::Ptr32<void> vtable;              // +0x0
    uint32_t refCount;         // +0x4
    re::Ptr32<void> constantMapVtable;   // +0x8 (the global constants: NiTFixedStringMap, 37 buckets)
    uint32_t constantHashSize; // +0xC
    re::Ptr32<re::Ptr32<MapItem>> constantTable;   // +0x10
    uint32_t constantCount;    // +0x14
    re::Ptr32<void> classCreate;         // +0x18
    re::Ptr32<void> runParser;           // +0x1C
    re::Ptr32<void> errorCallback;       // +0x20
};
static_assert(sizeof(NiShaderFactory) == 0x24);
GAME_VAR(NiShaderFactory*, ms_pkShaderFactory, 0x020E23FC);

GAME_FN(void, __thiscall, ConstantMap_GetNext, 0x008C2290, void* map, re::Ptr32<MapItem>* position, re::Ptr32<char>* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, ConstantMap_RemoveAll, 0x008C2230, void* map);
GAME_FN(uint8_t, __cdecl, NiShaderFactory_ReleaseGlobalShaderConstant, 0x00756B40, const re::Ptr32<char>* key);
GAME_FN(void, __thiscall, NiShaderFactory_Destruct, 0x00756CC0, NiShaderFactory* self);

} // namespace

// 0x756920: the constructor
NiShaderFactory* __fastcall NiShaderFactory_Construct(NiShaderFactory* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiShaderFactory_vtable);
    self->constantMapVtable = reinterpret_cast<void*>(ConstantMapBase_vtable);
    self->constantHashSize = 0x25;
    self->constantCount = 0;
    self->constantTable = static_cast<re::Ptr32<MapItem>*>(NiMalloc(0x94));
    crt::Memset(self->constantTable, 0, self->constantHashSize * 4);
    self->constantMapVtable = reinterpret_cast<void*>(ConstantMap_vtable);
    self->classCreate = nullptr;
    self->runParser = nullptr;
    self->errorCallback = nullptr;
    return self;
}

// 0x756CC0: the destructor: the singleton's slots 8 and 13 (its libraries and shaders let go),
// each global constant released, the singleton forgotten when it's this one
void __fastcall NiShaderFactory_DestructImpl(NiShaderFactory* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShaderFactory_vtable);
    if (ms_pkShaderFactory != nullptr) {
        Virtual<void>(ms_pkShaderFactory, 8);
        if (ms_pkShaderFactory != nullptr) {
            Virtual<void>(ms_pkShaderFactory, 13);
        }
    }
    re::Ptr32<MapItem> position = nullptr;
    for (uint32_t i = 0; i < self->constantHashSize; i++) {
        if (self->constantTable[i] != nullptr) {
            position = self->constantTable[i];
            break;
        }
    }
    while (position != nullptr) {
        re::Ptr32<char> key = kNullFixedString;
        re::Ptr32<void> value = nullptr;
        ConstantMap_GetNext(&self->constantMapVtable, &position, &key, &value);
        if (value != nullptr) {
            NiShaderFactory_ReleaseGlobalShaderConstant(&key);
        }
        FixedStringRelease(&key);
    }
    if (ms_pkShaderFactory == self) {
        ms_pkShaderFactory = nullptr;
    }
    self->constantMapVtable = reinterpret_cast<void*>(ConstantMap_vtable);
    ConstantMap_RemoveAll(&self->constantMapVtable);
    self->constantMapVtable = reinterpret_cast<void*>(ConstantMapBase_vtable);
    ConstantMap_RemoveAll(&self->constantMapVtable);
    NiFree(self->constantTable);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    Decrement(&ms_uiObjects);
}

// 0x756E00: the deleting destructor
NiShaderFactory* __fastcall NiShaderFactory_DeletingDestructor(NiShaderFactory* self, re::Edx, uint32_t flags) {
    NiShaderFactory_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiShaderFactory));
    }
    return self;
}

#define NISHADERFACTORY2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NISHADERFACTORY2_REVERSED("NiShaderFactory::NiShaderFactory", 0x00756920, NiShaderFactory_Construct);
NISHADERFACTORY2_REVERSED("NiShaderFactory::~NiShaderFactory", 0x00756CC0, NiShaderFactory_DestructImpl);
NISHADERFACTORY2_REVERSED("NiShaderFactory::DeletingDestructor", 0x00756E00, NiShaderFactory_DeletingDestructor);

} // namespace ni
