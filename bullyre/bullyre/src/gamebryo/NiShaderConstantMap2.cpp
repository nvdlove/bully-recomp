// NiShaderConstantMap's base class (vtable 0x954084, 0xC bytes: NiRefObject and its program
// type): constructor, deleting destructor, and its Add*Entry slots, which add nothing and return
// 0x30 (NiShaderError: entry not added). Also the {id, NiFixedString} pair its mapping tables are
// made of: the constructors and the assignment.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiShaderConstantMap_vtable = 0x00954084;
constexpr uint32_t kEntryNotAdded = 0x30;

GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(const char*, NullFixedString, 0x00951028);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);

struct NiShaderConstantMap : NiRefObject {
    uint32_t programType;   // +0x8
};
static_assert(sizeof(NiShaderConstantMap) == 0xC);

// a mapping: its id, its name
struct Mapping {
    uint32_t id;
    re::Ptr32<const char> name;
};

} // namespace

// 0x77F080: AddPredefinedEntry (slot 3: 4 arguments)
uint32_t __fastcall NiShaderConstantMap_AddPredefinedEntry(NiShaderConstantMap*, re::Edx, const char*, uint32_t,
                                                           uint32_t, const char*) {
    return kEntryNotAdded;
}

// 0x77F090: AddEntry (slots 2, 4, 5 and 6: 10 arguments; AddAttributeEntry, AddConstantEntry and
// AddGlobalEntry are the same function)
uint32_t __fastcall NiShaderConstantMap_AddEntry(NiShaderConstantMap*, re::Edx, const char*, uint32_t, uint32_t,
                                                 uint32_t, uint32_t, const char*, uint32_t, uint32_t, const void*,
                                                 int) {
    return kEntryNotAdded;
}

// 0x77F0A0: AddOperatorEntry (slot 7: 6 arguments)
uint32_t __fastcall NiShaderConstantMap_AddOperatorEntry(NiShaderConstantMap*, re::Edx, const char*, uint32_t,
                                                         uint32_t, uint32_t, uint32_t, const char*) {
    return kEntryNotAdded;
}

// 0x77F0B0: AddObjectEntry (slot 8: 5 arguments)
uint32_t __fastcall NiShaderConstantMap_AddObjectEntry(NiShaderConstantMap*, re::Edx, const char*, uint32_t,
                                                       const char*, uint32_t, uint32_t) {
    return kEntryNotAdded;
}

// 0x77F0C0: the constructor (the program type)
NiShaderConstantMap* __fastcall NiShaderConstantMap_Construct(NiShaderConstantMap* self, re::Edx, uint32_t type) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->programType = type;
    self->vtable = reinterpret_cast<void*>(NiShaderConstantMap_vtable);
    return self;
}

// 0x77F0F0: the deleting destructor (NiRefObject's destructor)
NiShaderConstantMap* __fastcall NiShaderConstantMap_DeletingDestructor(NiShaderConstantMap* self, re::Edx,
                                                                       uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    Decrement(&ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiShaderConstantMap));
    }
    return self;
}

// 0x77F5A0: the mapping pair's constructor (id, name)
Mapping* __fastcall NiShaderConstantMap_MappingConstruct(Mapping* self, re::Edx, uint32_t id, const re::Ptr32<const char>* name) {
    self->name = NullFixedString;
    self->id = id;
    FixedString_Assign(&self->name, name);
    return self;
}

// 0x77F5D0: the pair's default constructor (0, no name)
Mapping* __fastcall NiShaderConstantMap_MappingConstructDefault(Mapping* self, re::Edx) {
    self->name = NullFixedString;
    self->id = 0;
    FixedString_AssignText(&self->name, nullptr);
    return self;
}

// 0x77F5F0: the pair's assignment
Mapping* __fastcall NiShaderConstantMap_MappingAssign(Mapping* self, re::Edx, const Mapping* other) {
    self->id = other->id;
    FixedString_Assign(&self->name, &other->name);
    return self;
}

#define NICONSTMAP2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NICONSTMAP2_REVERSED("NiShaderConstantMap::AddPredefinedEntry", 0x0077F080, NiShaderConstantMap_AddPredefinedEntry);
NICONSTMAP2_REVERSED("NiShaderConstantMap::AddEntry", 0x0077F090, NiShaderConstantMap_AddEntry);
NICONSTMAP2_REVERSED("NiShaderConstantMap::AddOperatorEntry", 0x0077F0A0, NiShaderConstantMap_AddOperatorEntry);
NICONSTMAP2_REVERSED("NiShaderConstantMap::AddObjectEntry", 0x0077F0B0, NiShaderConstantMap_AddObjectEntry);
NICONSTMAP2_REVERSED("NiShaderConstantMap::NiShaderConstantMap", 0x0077F0C0, NiShaderConstantMap_Construct);
NICONSTMAP2_REVERSED("NiShaderConstantMap::DeletingDestructor", 0x0077F0F0, NiShaderConstantMap_DeletingDestructor);
NICONSTMAP2_REVERSED("NiTPair<uint,NiFixedString>::NiTPair", 0x0077F5A0, NiShaderConstantMap_MappingConstruct);
NICONSTMAP2_REVERSED("NiTPair<uint,NiFixedString>::NiTPairDefault", 0x0077F5D0, NiShaderConstantMap_MappingConstructDefault);
NICONSTMAP2_REVERSED("NiTPair<uint,NiFixedString>::operator=", 0x0077F5F0, NiShaderConstantMap_MappingAssign);

} // namespace ni
