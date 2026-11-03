// NiMaterialResourceBinding (vtable 0x965414, 0x1C bytes, a plain NiRefObject): an output
// resource of one material node bound to an input resource of another, with the variable name
// that carries it (NiMaterialConfigurator.cpp builds them). Also the +0x18 getter its accessors
// share with NiMaterialResource's default value, and NiStandardMaterialDescriptor's texture usage
// read, which sit with it.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

GAME_VAR(long, ms_uiObjects, 0x020E2168);
constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kBindingVtable = 0x00965414;

struct NiMaterialResourceBinding : NiRefObject {
    re::Ptr32<NiRefObject> outputResource;   // +0x8 (NiMaterialResourcePtr)
    re::Ptr32<NiRefObject> inputResource;    // +0xC
    re::Ptr32<NiRefObject> outputNode;       // +0x10 (NiMaterialNodePtr)
    re::Ptr32<NiRefObject> inputNode;        // +0x14
    re::Ptr32<char> variable;                // +0x18 (NiFixedString)
};
static_assert(offsetof(NiMaterialResourceBinding, outputNode) == 0x10);
static_assert(offsetof(NiMaterialResourceBinding, variable) == 0x18);
static_assert(sizeof(NiMaterialResourceBinding) == 0x1C);

GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, Binding_Destruct, 0x007F8010, NiMaterialResourceBinding* self);

// a reference slot assigned: the old one let go first, the new one stored, then held
inline void AssignReleasingFirst(re::Ptr32<NiRefObject>& slot, NiRefObject* value) {
    NiRefObject* old = slot;
    if (old == value) {
        return;
    }
    if (old != nullptr && Decrement(&old->refCount) == 0) {
        Virtual<void>(old, 1);
    }
    slot = value;
    if (value != nullptr) {
        Increment(&value->refCount);
    }
}

inline void ReleaseImport(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// 0x7F7EF0 NiMaterialResourceBinding(output resource, output node, input resource, input node)
NiMaterialResourceBinding* __fastcall Construct(NiMaterialResourceBinding* self, re::Edx, NiRefObject* outputResource,
                                                NiRefObject* outputNode, NiRefObject* inputResource,
                                                NiRefObject* inputNode) {
    self->vtable = reinterpret_cast<void*>(kRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kBindingVtable);
    self->outputResource = nullptr;
    self->inputResource = nullptr;
    self->outputNode = nullptr;
    self->inputNode = nullptr;
    self->variable = kNullFixedString;
    AssignReleasingFirst(self->outputResource, outputResource);
    AssignReleasingFirst(self->inputResource, inputResource);
    AssignReleasingFirst(self->outputNode, outputNode);
    AssignReleasingFirst(self->inputNode, inputNode);
    return self;
}

// 0x7F8010 ~NiMaterialResourceBinding
void __fastcall Destruct(NiMaterialResourceBinding* self, re::Edx) {
    FixedStringRelease(&self->variable);
    ReleaseImport(self->inputNode);
    ReleaseImport(self->outputNode);
    ReleaseImport(self->inputResource);
    ReleaseImport(self->outputResource);
    self->vtable = reinterpret_cast<void*>(kRefObjectVtable);
    Decrement(&ms_uiObjects);
}

// 0x7F80C0 SetVariable(name): the fixed string at +0x18 assigned (shared with
// NiMaterialResource::SetDefaultValue)
void __fastcall SetVariable(NiMaterialResourceBinding* self, re::Edx, const re::Ptr32<char>* name) {
    FixedString_Assign(&self->variable, name);
}

// 0x7F80D0 the deleting destructor
NiMaterialResourceBinding* __fastcall DeletingDestructor(NiMaterialResourceBinding* self, re::Edx, uint32_t flags) {
    Binding_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// 0x7F80F0 NiStandardMaterialDescriptor::GetTextureUsage(texture, uv set, texgen): texture n's
// four bits (a 3-bit UV set, a texgen flag); 0..5 in the descriptor's third word from bit 8,
// 6..11 in the fourth from bit 0. Nothing for any other
void __fastcall GetTextureUsage(const NiMaterialDescriptor* self, re::Edx, uint32_t texture, uint32_t* uvSet,
                                uint32_t* texGen) {
    if (texture > 11) {
        return;
    }
    uint32_t word = texture < 6 ? self->bits[2] : self->bits[3];
    uint32_t shift = texture < 6 ? 8 + texture * 4 : (texture - 6) * 4;
    *uvSet = (word >> shift) & 7;
    *texGen = (word >> (shift + 3)) & 1;
}

} // namespace

// 0x7F7EE0: lea eax, [ecx+0x18]; ret (4 bytes): the binding's variable, also
// NiMaterialResource::GetDefaultValue
// 0x007F7EE0: [8d 41 18 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void* __fastcall Tiny_007F7EE0(uint8_t* self, re::Edx) {
    return self + 0x18;
}
RE_UNHOOKED_FN("NiMaterialResourceBinding::GetVariable", 0x007F7EE0, &Tiny_007F7EE0);

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIMATBINDING_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIMATBINDING_REVERSED("NiMaterialResourceBinding::NiMaterialResourceBinding", 0x007F7EF0, ni::Construct);
NIMATBINDING_REVERSED("NiMaterialResourceBinding::~NiMaterialResourceBinding", 0x007F8010, ni::Destruct);
NIMATBINDING_REVERSED("NiMaterialResourceBinding::SetVariable", 0x007F80C0, ni::SetVariable);
NIMATBINDING_REVERSED("NiMaterialResourceBinding::DeletingDestructor", 0x007F80D0, ni::DeletingDestructor);
NIMATBINDING_REVERSED("NiStandardMaterialDescriptor::GetTextureUsage", 0x007F80F0, ni::GetTextureUsage);
