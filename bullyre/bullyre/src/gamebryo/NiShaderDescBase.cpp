// NiShaderDescBase (vtable 0x95F79C, 0x10 bytes): the shader library descriptions' base (src/
// gamebryo/NiShaderDesc.cpp, NiShaderAttributeDesc2.cpp derive from it), an NiRefObject with a
// name and a description, each an NiMalloc'd copy. NiShaderLibrary (vtable 0x95F7B4, 0xC bytes):
// a shader library's base, an NiRefObject with a name (its SetName the same code as the
// description's). Also NiGlobalConstantEntry::SetData (src/gamebryo/NiShaderFactory.cpp).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiShaderDescBase_vtable = 0x0095F79C;
constexpr uintptr_t NiShaderLibrary_vtable = 0x0095F7B4;
GAME_VAR(long, ms_uiObjects, 0x020E2168);

struct NiShaderDescBase {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t refCount;   // +0x4
    re::Ptr32<char> name;          // +0x8
    re::Ptr32<char> description;   // +0xC
};
static_assert(sizeof(NiShaderDescBase) == 0x10);

struct NiShaderLibrary {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t refCount;   // +0x4
    re::Ptr32<char> name;          // +0x8
};
static_assert(sizeof(NiShaderLibrary) == 0xC);

struct NiGlobalConstantEntry {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t refCount;   // +0x4
    re::Ptr32<char> key;           // +0x8
    int32_t type;        // +0xC
    uint32_t dataSize;   // +0x10
    re::Ptr32<void> data;          // +0x14 (NiMalloc'd)
};
static_assert(sizeof(NiGlobalConstantEntry) == 0x18);

GAME_FN(void, __thiscall, ShaderDescBase_SetName, 0x007AB730, void* self, const char* name);

// a string field set to an NiMalloc'd copy (none for an empty one)
void SetCopy(re::Ptr32<char>* field, const char* text) {
    NiFree(*field);
    *field = nullptr;
    if (text != nullptr && *text != '\0') {
        size_t length = 0;
        while (text[length] != '\0') {
            length++;
        }
        size_t size = length + 1;
        char* copy = static_cast<char*>(NiMalloc(uint32_t(size)));
        *field = copy;
        crt::StrcpyS(copy, size, text);
    }
}

// ---- NiShaderDescBase

NiShaderDescBase* __fastcall Construct(NiShaderDescBase* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiShaderDescBase_vtable);
    self->name = nullptr;
    self->description = nullptr;
    return self;
}

void __fastcall Destruct(NiShaderDescBase* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShaderDescBase_vtable);
    NiFree(self->name);
    NiFree(self->description);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    Decrement(&ms_uiObjects);
}

NiShaderDescBase* __fastcall DeletingDestructor(NiShaderDescBase* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiShaderDescBase_vtable);
    NiFree(self->name);
    NiFree(self->description);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    Decrement(&ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// slot 4 SetName (NiShaderLibrary's too)
void __fastcall SetName(NiShaderDescBase* self, re::Edx, const char* name) {
    SetCopy(&self->name, name);
}

// slot 5 SetDescription
void __fastcall SetDescription(NiShaderDescBase* self, re::Edx, const char* description) {
    SetCopy(&self->description, description);
}

// ---- NiShaderLibrary

NiShaderLibrary* __fastcall LibraryConstruct(NiShaderLibrary* self, re::Edx, const char* name) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiShaderLibrary_vtable);
    self->name = nullptr;
    ShaderDescBase_SetName(self, name);
    return self;
}

void __fastcall LibraryDestruct(NiShaderLibrary* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShaderLibrary_vtable);
    NiFree(self->name);
    self->name = nullptr;
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    Decrement(&ms_uiObjects);
}

NiShaderLibrary* __fastcall LibraryDeletingDestructor(NiShaderLibrary* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiShaderLibrary_vtable);
    NiFree(self->name);
    self->name = nullptr;
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    Decrement(&ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xC);
    }
    return self;
}

// ---- NiGlobalConstantEntry

// SetData(size, data): the buffer grown to the size when smaller (made when there's none), the
// data copied in when given
void __fastcall SetData(NiGlobalConstantEntry* self, re::Edx, uint32_t size, const void* data) {
    if (self->dataSize < size) {
        NiFree(self->data);
        self->data = nullptr;
        self->dataSize = size;
    }
    if (self->data == nullptr) {
        self->data = NiMalloc(self->dataSize);
    }
    if (data != nullptr) {
        crt::MemcpyS(self->data, self->dataSize, data, size);
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISHADERDESCBASE_REVERSED(name, address, fn)                                           \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISHADERDESCBASE_REVERSED("NiShaderDescBase::NiShaderDescBase", 0x007AB6B0, &ni::Construct);
NISHADERDESCBASE_REVERSED("NiShaderDescBase::~NiShaderDescBase", 0x007AB6F0, &ni::Destruct);
NISHADERDESCBASE_REVERSED("NiShaderDescBase::SetName", 0x007AB730, &ni::SetName);
NISHADERDESCBASE_REVERSED("NiShaderDescBase::SetDescription", 0x007AB790, &ni::SetDescription);
NISHADERDESCBASE_REVERSED("NiShaderDescBase::DeletingDestructor", 0x007AB7F0, &ni::DeletingDestructor);
NISHADERDESCBASE_REVERSED("NiShaderLibrary::~NiShaderLibrary", 0x007AB840, &ni::LibraryDestruct);
NISHADERDESCBASE_REVERSED("NiShaderLibrary::NiShaderLibrary", 0x007AB870, &ni::LibraryConstruct);
NISHADERDESCBASE_REVERSED("NiShaderLibrary::DeletingDestructor", 0x007AB8B0, &ni::LibraryDeletingDestructor);
NISHADERDESCBASE_REVERSED("NiGlobalConstantEntry::SetData", 0x007AB900, &ni::SetData);
