// NiGeneralEntity (vtables 0x98C208 / 0x98C0B8 at +8, 0x34 bytes): the entity of Gamebryo's entity
// library, a named set of components (NiTObjectSet<NiEntityComponentInterfaceIPtr>) with a
// template ID, a master entity and a hidden flag. Its property interface asks its components in
// turn (the first that knows the property answers); Update and BuildVisibleSet go to every one
// (Update with the entity itself as the parent). Its interface methods get the interface pointer
// (+8). (GetMasterEntity, GetHidden, SetHidden and SupportsComponents are folded getters /
// setters elsewhere in the exe: 0x8B6120, 0x46FB90, 0x8992D0, 0x7FFDC0.)
//
// Also the static names of two classes with no other code in the exe (a lone "Scene Root
// Pointer", and the undo / redo errors of the command manager: a guess at the class), the
// NiTObjectSet<IPtr> helpers this file's code calls, NiUniqueID's operator==, and the folded
// `return [ecx+0xC]` getter.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::ec {
namespace {

// a set of interface pointers held by reference (AddReference / RemoveReference: slots 0 / 1)
using IPtrSet = NiTSet<void*>;

#pragma pack(push, 1)
struct GeneralEntity : NiRefObject {
    re::Ptr32<void> ifaceVtable;          // +0x8
    IPtrSet components;         // +0xC (NiEntityComponentInterface pointers)
    re::Ptr32<const char> name;         // +0x18
    re::Ptr32<void> masterEntity;         // +0x1C (NiEntityInterface, held)
    uint8_t hidden;             // +0x20
    NiUniqueID templateID;      // +0x21 (unaligned)
    uint8_t pad31[3];
};
#pragma pack(pop)
static_assert(offsetof(GeneralEntity, components) == 0xC);
static_assert(offsetof(GeneralEntity, name) == 0x18);
static_assert(offsetof(GeneralEntity, masterEntity) == 0x1C);
static_assert(offsetof(GeneralEntity, hidden) == 0x20);
static_assert(offsetof(GeneralEntity, templateID) == 0x21);
static_assert(sizeof(GeneralEntity) == 0x34);

constexpr uintptr_t GeneralEntity_vtable = 0x0098C208;
constexpr uintptr_t GeneralEntityIface_vtable = 0x0098C0B8;
constexpr uintptr_t EntityInterface_vtable = 0x0098BF68;
constexpr uintptr_t PropertyInterface_vtable = 0x00989CE8;

// the entity interface's own slots (NiEntity.h's, which the other pass owns)
namespace eslot {
enum : int {
    Clone = 69,
    AddComponent = 75,
    GetComponentCount = 76,
    GetComponentAt = 77,
    RemoveComponent = 79,
    SetMasterEntity = 83,
};
} // namespace eslot

GAME_VAR(NiFixedString, ms_kClassName, 0x020E5F30);   // "NiGeneralEntity"

GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __cdecl, NiDelete_, 0x00752540, void* p, uint32_t size);
GAME_FN(void, __thiscall, PtrSet_Realloc, 0x0083AD20, NiTSet<void*>* self, uint32_t size);   // NiTPrimitiveSet<T*>
GAME_FN(void, __thiscall, IPtrSet_Realloc, 0x008409C0, IPtrSet* self, uint32_t size);
GAME_FN(void, __thiscall, IPtrSet_RemoveAll, 0x0083E380, IPtrSet* self);
GAME_FN(void*, __cdecl, NiNewIPtrArray, 0x00856120, uint32_t count);   // (NiNew[] of pointers, cleared)

// this file's, through their addresses
GAME_FN(uint8_t, __thiscall, UniqueID_Equal, 0x00853060, const NiUniqueID* self, const NiUniqueID* other);
GAME_FN(void, __thiscall, IPtrSet_RemoveAt, 0x008542F0, IPtrSet* self, uint32_t index);
GAME_FN(void, __cdecl, IPtrArray_Delete, 0x008544E0, re::Ptr32<void>* base);
GAME_FN(void, __thiscall, IPtrSet_Add, 0x00854770, IPtrSet* self, const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, Entity_Destruct, 0x00854920, GeneralEntity* self);
GAME_FN(void*, __thiscall, Entity_DeletingDestructor, 0x008549A0, GeneralEntity* self, int flags);
GAME_FN(GeneralEntity*, __thiscall, Entity_ConstructWith, 0x008549C0, void* self, const re::Ptr32<const char>* name,
        const NiUniqueID* templateID, uint32_t componentCount);

inline GeneralEntity* Self(void* iface) {
    return FromIface<GeneralEntity>(iface);
}

// the entity's own interface pointer as its code makes it (null for a null object)
inline void* SelfIface(void* iface) {
    return FromIface<GeneralEntity>(iface) != nullptr ? iface : nullptr;
}

inline void AddReference(void* iface) {
    V<void>(iface, slot::AddReference);
}
inline void RemoveReference(void* iface) {
    V<void>(iface, slot::RemoveReference);
}

// an IPtr slot assigned (NiPointer's order: the old one let go first)
inline void AssignIPtr(re::Ptr32<void>& slotRef, void* value) {
    void* old = slotRef;
    if (old != value) {
        if (old != nullptr) {
            RemoveReference(old);
        }
        slotRef = value;
        if (value != nullptr) {
            AddReference(value);
        }
    }
}

// ---- the static names

void __cdecl SceneRootPointerSDMInit() {
    FixedString_SetString(reinterpret_cast<re::Ptr32<const char>*>(0x020E5F24), reinterpret_cast<const char*>(0x0098AF48));
}

void __cdecl SceneRootPointerSDMShutdown() {
    FixedString_SetString(reinterpret_cast<re::Ptr32<const char>*>(0x020E5F24), nullptr);
}

void __cdecl CommandManagerSDMInit() {
    // "Undo/redo command not issued; undo frame open."
    FixedString_SetString(reinterpret_cast<re::Ptr32<const char>*>(0x020E5F28), reinterpret_cast<const char*>(0x0098BF34));
    // "Undo/redo commands cannot be issued while an undo frame is open."
    FixedString_SetString(reinterpret_cast<re::Ptr32<const char>*>(0x020E5F2C), reinterpret_cast<const char*>(0x0098BEF0));
}

void __cdecl CommandManagerSDMShutdown() {
    FixedString_SetString(reinterpret_cast<re::Ptr32<const char>*>(0x020E5F28), nullptr);
    FixedString_SetString(reinterpret_cast<re::Ptr32<const char>*>(0x020E5F2C), nullptr);
}

void __cdecl SDMInit() {
    FixedString_SetString(&ms_kClassName, reinterpret_cast<const char*>(0x008FFEB4));   // "NiGeneralEntity"
}

void __cdecl SDMShutdown() {
    FixedString_SetString(&ms_kClassName, nullptr);
}

// ---- the helpers

// NiUniqueID::operator== (the 16 bytes compared)
uint8_t __fastcall UniqueIDEqual(const NiUniqueID* self, re::Edx, const NiUniqueID* other) {
    return std::memcmp(self, other, sizeof(NiUniqueID)) == 0 ? 1 : 0;
}

// (folded: a getter of the dword at +0xC: GetComponentCount here, and others' elsewhere)
uint32_t __fastcall GetDwordC(void* self, re::Edx) {
    return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(self) + 0xC);
}

// NiTObjectSet<IPtr>::RemoveAt: the last one moved into the slot, the last slot cleared
void __fastcall IPtrSetRemoveAt(IPtrSet* self, re::Edx, uint32_t index) {
    self->used--;
    uint32_t last = self->used;
    re::Ptr32<void>* base = self->base;
    if (base[index] != base[last]) {
        if (base[index] != nullptr) {
            RemoveReference(base[index]);
        }
        base[index] = base[last];
        if (base[index] != nullptr) {
            AddReference(base[index]);
        }
    }
    re::Ptr32<void>* slotRef = &self->base[self->used];
    if (*slotRef != nullptr) {
        RemoveReference(*slotRef);
        *slotRef = nullptr;
    }
}

// NiTObjectSet<IPtr>::Add: grown (doubled, or to 1) when full
void __fastcall IPtrSetAdd(IPtrSet* self, re::Edx, const re::Ptr32<void>* value) {
    if (self->used == self->alloced) {
        IPtrSet_Realloc(self, self->alloced != 0 ? self->alloced * 2 : 1);
    }
    re::Ptr32<void>* slotRef = &self->base[self->used];
    self->used++;
    AssignIPtr(*slotRef, *value);
}

// NiDelete[] of an IPtr array: let go from the last to the first, then the block (the count
// before the elements)
void __cdecl IPtrArrayDelete(re::Ptr32<void>* base) {
    if (base == nullptr) {
        return;
    }
    uint32_t* block = reinterpret_cast<uint32_t*>(base) - 1;
    int32_t count = int32_t(*block);
    re::Ptr32<void>* p = base + count;
    for (int32_t i = count - 1; i >= 0; i--) {
        p--;
        if (*p != nullptr) {
            RemoveReference(*p);
        }
    }
    NiDeleteArray(block, 4);
}

// ---- construction and destruction

GeneralEntity* __fastcall Construct(GeneralEntity* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    self->refCount = 0;
    ni::sys::Increment(const_cast<long*>(&RefObject_ms_uiObjects));
    self->ifaceVtable = reinterpret_cast<void*>(EntityInterface_vtable);
    self->vtable = reinterpret_cast<void*>(GeneralEntity_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(GeneralEntityIface_vtable);
    self->components.base = nullptr;
    self->components.alloced = 0;
    self->components.used = 0;
    self->name = NullFixedString;
    self->masterEntity = nullptr;
    self->hidden = 0;
    std::memset(&self->templateID, 0, sizeof(NiUniqueID));
    return self;
}

// (Clone's: the name, the template ID, room for the components)
GeneralEntity* __fastcall ConstructWith(GeneralEntity* self, re::Edx, const re::Ptr32<const char>* name,
                                        const NiUniqueID* templateID, uint32_t componentCount) {
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    self->refCount = 0;
    ni::sys::Increment(const_cast<long*>(&RefObject_ms_uiObjects));
    self->ifaceVtable = reinterpret_cast<void*>(EntityInterface_vtable);
    self->vtable = reinterpret_cast<void*>(GeneralEntity_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(GeneralEntityIface_vtable);
    self->components.base = componentCount > 0 ? static_cast<re::Ptr32<void>*>(NiNewIPtrArray(componentCount)) : nullptr;
    self->components.alloced = componentCount;
    self->components.used = 0;
    NiFixedString s = *name;
    if (s != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(s) - 8);
    }
    self->name = *name;
    self->masterEntity = nullptr;
    self->hidden = 0;
    std::memcpy(&self->templateID, templateID, sizeof(NiUniqueID));
    return self;
}

// the master let go (the slot not cleared), the name, the components and their array
void __fastcall Destruct(GeneralEntity* self, re::Edx) {
    if (self->masterEntity != nullptr) {
        RemoveReference(self->masterEntity);
    }
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->name));
    IPtrSet_RemoveAll(&self->components);
    IPtrArray_Delete(self->components.base);
    self->ifaceVtable = reinterpret_cast<void*>(PropertyInterface_vtable);
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    ni::sys::Decrement(const_cast<long*>(&RefObject_ms_uiObjects));
}

GeneralEntity* __fastcall DeletingDestructor(GeneralEntity* self, re::Edx, int flags) {
    Entity_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete_(self, sizeof(GeneralEntity));
    }
    return self;
}

void* __fastcall InterfaceDeletingDestructor(void* iface, re::Edx, int flags) {
    return Entity_DeletingDestructor(Self(iface), flags);
}

// a copy under a new name: inheriting, this entity is its master and its components are clones
// inheriting from these; else the same master and plain copies of the components. (A failed
// allocation reads through null, as the game's does)
void* __fastcall Clone(void* iface, re::Edx, const re::Ptr32<const char>* newName, int inherit) {
    GeneralEntity* self = Self(iface);
    void* memory = NiNew(sizeof(GeneralEntity));
    GeneralEntity* clone =
        memory != nullptr ? Entity_ConstructWith(memory, newName, &self->templateID, self->components.used) : nullptr;
    if (re::Flag(inherit)) {
        AssignIPtr(clone->masterEntity, SelfIface(iface));
    } else {
        AssignIPtr(clone->masterEntity, self->masterEntity);
    }
    uint32_t count = self->components.used;
    for (uint32_t i = 0; i < count; i++) {
        void* component = V<void*>(self->components.base[i], slot::Clone, inherit);
        if (component != nullptr) {
            AddReference(component);   // (the IPtr the clone is held in)
        }
        IPtrSet& to = clone->components;
        if (to.used == to.alloced) {
            IPtrSet_Realloc(&to, to.alloced != 0 ? to.alloced * 2 : 1);
        }
        re::Ptr32<void>* slotRef = &to.base[to.used];
        to.used++;
        void* old = *slotRef;
        if (old != component) {
            if (old != nullptr) {
                RemoveReference(old);
            }
            *slotRef = component;
            if (component == nullptr) {
                continue;
            }
            AddReference(component);
        }
        if (component != nullptr) {
            RemoveReference(component);
        }
    }
    return clone != nullptr ? IfaceOf(clone) : nullptr;
}

// ---- the entity's own

re::Ptr32<const char>* __fastcall GetClassName_(void*, re::Edx, re::Ptr32<const char>* result) {
    NiFixedString s = ms_kClassName;
    if (s != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(s) - 8);
        s = ms_kClassName;
    }
    *result = s;
    return result;
}

re::Ptr32<const char>* __fastcall GetName_(void* iface, re::Edx, re::Ptr32<const char>* result) {
    GeneralEntity* self = Self(iface);
    if (self->name != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(self->name) - 8);
    }
    *result = self->name;
    return result;
}

uint8_t __fastcall SetName_(void* iface, re::Edx, const re::Ptr32<const char>* name) {
    FixedString_Assign(&Self(iface)->name, name);
    return 1;
}

NiUniqueID* __fastcall GetTemplateID_(void* iface, re::Edx, NiUniqueID* result) {
    std::memcpy(result, &Self(iface)->templateID, sizeof(NiUniqueID));
    return result;
}

uint8_t __fastcall SetTemplateID_(void* iface, re::Edx, const NiUniqueID* id) {
    std::memcpy(&Self(iface)->templateID, id, sizeof(NiUniqueID));
    return 1;
}

void __fastcall SetMasterEntity(void* iface, re::Edx, void* master) {
    AssignIPtr(Self(iface)->masterEntity, master);
}

void* __fastcall GetComponentAt(void* iface, re::Edx, uint32_t index) {
    return Self(iface)->components.base[index];
}

// the first component whose template ID is this one
void* __fastcall GetComponentByTemplateID(void* iface, re::Edx, const NiUniqueID* id) {
    GeneralEntity* self = Self(iface);
    for (uint32_t i = 0; i < self->components.used; i++) {
        void* component = self->components.base[i];
        NiUniqueID local;
        NiUniqueID* componentID = V<NiUniqueID*>(component, slot::GetTemplateID, &local);
        if (UniqueID_Equal(componentID, id)) {
            return component;
        }
    }
    return nullptr;
}

// a component added (after asking it whether it can be, when told to check)
uint8_t __fastcall AddComponent(void* iface, re::Edx, void* component, int check) {
    if (re::Flag(check)) {
        uint8_t ok = V(component, slot::CanAttachToEntity, SelfIface(iface));
        if (!ok) {
            return ok;
        }
    }
    re::Ptr32<void> held = component;   // (the IPtr, in the argument's slot)
    if (component != nullptr) {
        AddReference(component);
    }
    IPtrSet_Add(&Self(iface)->components, &held);
    if (component != nullptr) {
        RemoveReference(component);
    }
    return 1;
}

// a component taken out (after asking it, when told to check)
uint8_t __fastcall RemoveComponent(void* iface, re::Edx, void* component, int check) {
    GeneralEntity* self = Self(iface);
    if (component != nullptr) {
        AddReference(component);   // (an IPtr held while it's looked for)
    }
    uint32_t count = self->components.used;
    int32_t index = -1;
    for (uint32_t i = 0; i < count; i++) {
        if (self->components.base[i] == component) {
            index = int32_t(i);
            break;
        }
    }
    if (component != nullptr) {
        RemoveReference(component);
    }
    if (index == -1) {
        return 0;
    }
    if (re::Flag(check) && !V(component, slot::CanDetachFromEntity, SelfIface(iface))) {
        return 0;
    }
    IPtrSet_RemoveAt(&self->components, uint32_t(index));
    return 1;
}

uint8_t __fastcall RemoveComponentAt(void* iface, re::Edx, uint32_t index, int check) {
    GeneralEntity* self = Self(iface);
    void* component = self->components.base[index];
    if (re::Flag(check)) {
        uint8_t ok = V(component, slot::CanDetachFromEntity, SelfIface(iface));
        if (!ok) {
            return ok;
        }
    }
    IPtrSet_RemoveAt(&self->components, index);
    return 1;
}

void __fastcall RemoveAllComponents(void* iface, re::Edx) {
    IPtrSet_RemoveAll(&Self(iface)->components);
}

// whether one of its components has the property (written only when found)
uint8_t __fastcall IsComponentProperty(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* found) {
    StringSet names{nullptr, 0, 0};
    V<void>(iface, slot::GetPropertyNames, &names);
    for (uint32_t i = 0; i < names.used; i++) {
        if (names.base[i] == *name) {
            *found = 1;
            if (names.base != nullptr) {
                FixedStringArray_Delete(names.base, 3);
            }
            return 1;
        }
    }
    if (names.base != nullptr) {
        FixedStringArray_Delete(names.base, 3);
    }
    return 0;
}

// a new master: none, the components lose theirs; else the components with no counterpart (by
// template ID) in the master go, the rest take theirs as their masters, and the master's other
// components are added as clones inheriting from them. Then the master is kept
void __fastcall ReplaceMasterEntity(void* iface, re::Edx, void* master) {
    GeneralEntity* self = Self(iface);
    if (master == nullptr) {
        for (uint32_t i = 0; i < self->components.used; i++) {
            V<void>(self->components.base[i], slot::SetMasterComponent, static_cast<void*>(nullptr));
        }
    } else {
        NiTSet<void*> unmatched{nullptr, 0, 0};
        uint32_t count = self->components.used;
        for (uint32_t i = 0; i < count; i++) {
            void* component = self->components.base[i];
            bool matched = false;
            for (uint32_t j = 0; j < V<uint32_t>(master, eslot::GetComponentCount); j++) {
                void* masterComponent = V<void*>(master, eslot::GetComponentAt, j);
                NiUniqueID a;
                NiUniqueID b;
                NiUniqueID* masterID = V<NiUniqueID*>(masterComponent, slot::GetTemplateID, &a);
                NiUniqueID* componentID = V<NiUniqueID*>(component, slot::GetTemplateID, &b);
                if (UniqueID_Equal(componentID, masterID)) {
                    matched = true;
                    break;
                }
            }
            if (matched) {
                continue;
            }
            if (unmatched.used == unmatched.alloced) {
                PtrSet_Realloc(&unmatched, unmatched.alloced != 0 ? unmatched.alloced * 2 : 1);
            }
            unmatched.base[unmatched.used] = component;
            unmatched.used++;
        }
        uint32_t unmatchedCount = unmatched.used;
        for (uint32_t k = 0; k < unmatchedCount; k++) {
            V(iface, eslot::RemoveComponent, unmatched.base[k], 0);
        }
        for (uint32_t j = 0; j < V<uint32_t>(master, eslot::GetComponentCount); j++) {
            void* masterComponent = V<void*>(master, eslot::GetComponentAt, j);
            bool matched = false;
            for (uint32_t i = 0; i < self->components.used; i++) {
                void* component = self->components.base[i];
                NiUniqueID a;
                NiUniqueID b;
                NiUniqueID* componentID = V<NiUniqueID*>(component, slot::GetTemplateID, &a);
                NiUniqueID* masterID = V<NiUniqueID*>(masterComponent, slot::GetTemplateID, &b);
                if (UniqueID_Equal(masterID, componentID)) {
                    V<void>(component, slot::SetMasterComponent, masterComponent);
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                void* clone = V<void*>(masterComponent, slot::Clone, 1);
                V(iface, eslot::AddComponent, clone, 0);
            }
        }
        NiFree(unmatched.base);
    }
    V<void>(iface, eslot::SetMasterEntity, master);
}

// ---- the property interface: every component in turn

uint8_t __fastcall IsAnimated(void* iface, re::Edx) {
    GeneralEntity* self = Self(iface);
    for (uint32_t i = 0; i < self->components.used; i++) {
        if (V(self->components.base[i], slot::IsAnimated)) {
            return 1;
        }
    }
    return 0;
}

// (the entity itself the components' parent; the given one not looked at)
void __fastcall Update(void* iface, re::Edx, void*, float time, void* errors, void* assets) {
    GeneralEntity* self = Self(iface);
    for (uint32_t i = 0; i < self->components.used; i++) {
        V<void>(self->components.base[i], slot::Update, SelfIface(iface), time, errors, assets);
    }
}

// (nothing while hidden)
void __fastcall BuildVisibleSet(void* iface, re::Edx, void* context, void* errors) {
    GeneralEntity* self = Self(iface);
    if (self->hidden != 0) {
        return;
    }
    for (uint32_t i = 0; i < self->components.used; i++) {
        V<void>(self->components.base[i], slot::BuildVisibleSet, context, errors);
    }
}

void __fastcall GetPropertyNames(void* iface, re::Edx, StringSet* names) {
    GeneralEntity* self = Self(iface);
    for (uint32_t i = 0; i < self->components.used; i++) {
        V<void>(self->components.base[i], slot::GetPropertyNames, names);
    }
}

// the first component answering (true) decides; the same slot of the component interface
template <int Slot, typename... A>
uint8_t __fastcall ForwardFirst(void* iface, re::Edx, A... args) {
    GeneralEntity* self = Self(iface);
    for (uint32_t i = 0; i < self->components.used; i++) {
        if (V(self->components.base[i], Slot, args...)) {
            return 1;
        }
    }
    return 0;
}

template <int Slot>
constexpr auto Forward1 = &ForwardFirst<Slot, void*>;
template <int Slot>
constexpr auto Forward2 = &ForwardFirst<Slot, void*, void*>;
template <int Slot>
constexpr auto Forward3 = &ForwardFirst<Slot, void*, void*, void*>;
template <int Slot>
constexpr auto Forward4 = &ForwardFirst<Slot, void*, void*, void*, void*>;
// SetPropertyData(float): the value passed on as a float
constexpr auto ForwardSetFloat = &ForwardFirst<slot::SetFloat, void*, float, uint32_t>;

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NIGENENTITY_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace ge = ni::ec;
namespace gs = ni::ec::slot;

// (the class: a guess. Nothing else reads the name)
NIGENENTITY_REVERSED("NiEntitySceneRootPointer::_SDMInit_852FC0", 0x00852FC0, &ge::SceneRootPointerSDMInit);
NIGENENTITY_REVERSED("NiEntitySceneRootPointer::_SDMShutdown_852FD0", 0x00852FD0, &ge::SceneRootPointerSDMShutdown);
// (the undo / redo errors: NiEntityCommandManager's, a guess; nothing else reads them)
NIGENENTITY_REVERSED("NiEntityCommandManager::_SDMInit", 0x00852FE0, &ge::CommandManagerSDMInit);
NIGENENTITY_REVERSED("NiEntityCommandManager::_SDMShutdown", 0x00853000, &ge::CommandManagerSDMShutdown);
NIGENENTITY_REVERSED("NiGeneralEntity::SetTemplateID", 0x00853020, &ge::SetTemplateID_);
NIGENENTITY_REVERSED("NiGeneralEntity::GetTemplateID", 0x00853040, &ge::GetTemplateID_);
NIGENENTITY_REVERSED("NiUniqueID::operator==", 0x00853060, &ge::UniqueIDEqual);
NIGENENTITY_REVERSED("NiGeneralEntity::SetMasterEntity", 0x00853110, &ge::SetMasterEntity);
// (4 bytes: too short to hook; folded, GetComponentCount here)
// 0x00853140: [8b 41 0c c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_00853140(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0xC);
}
RE_UNHOOKED_FN("NiGeneralEntity::GetComponentCount", 0x00853140, &Tiny_00853140);
NIGENENTITY_REVERSED("NiGeneralEntity::GetComponentAt", 0x00853150, &ge::GetComponentAt);
NIGENENTITY_REVERSED("NiGeneralEntity::GetComponentByTemplateID", 0x00853160, &ge::GetComponentByTemplateID);
NIGENENTITY_REVERSED("NiGeneralEntity::IsAnimated", 0x008531C0, &ge::IsAnimated);
NIGENENTITY_REVERSED("NiGeneralEntity::Update", 0x00853200, &ge::Update);
NIGENENTITY_REVERSED("NiGeneralEntity::BuildVisibleSet", 0x00853250, &ge::BuildVisibleSet);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyNames", 0x00853290, &ge::GetPropertyNames);
NIGENENTITY_REVERSED("NiGeneralEntity::CanResetProperty", 0x008532C0, ge::Forward2<gs::CanResetProperty>);
NIGENENTITY_REVERSED("NiGeneralEntity::ResetProperty", 0x00853310, ge::Forward1<gs::ResetProperty>);
NIGENENTITY_REVERSED("NiGeneralEntity::MakePropertyUnique", 0x00853350, ge::Forward2<gs::MakePropertyUnique>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetDisplayName", 0x008533A0, ge::Forward2<gs::GetDisplayName>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetDisplayName", 0x008533F0, ge::Forward2<gs::SetDisplayName>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPrimitiveType", 0x00853440, ge::Forward2<gs::GetPrimitiveType>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPrimitiveType", 0x00853490, ge::Forward2<gs::SetPrimitiveType>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetSemanticType", 0x008534E0, ge::Forward2<gs::GetSemanticType>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetSemanticType", 0x00853530, ge::Forward2<gs::SetSemanticType>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetDescription", 0x00853580, ge::Forward2<gs::GetDescription>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetDescription", 0x008535D0, ge::Forward2<gs::SetDescription>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetCategory", 0x00853620, ge::Forward2<gs::GetCategory>);
NIGENENTITY_REVERSED("NiGeneralEntity::IsPropertyReadOnly", 0x00853670, ge::Forward2<gs::IsPropertyReadOnly>);
NIGENENTITY_REVERSED("NiGeneralEntity::IsPropertyUnique", 0x008536C0, ge::Forward2<gs::IsPropertyUnique>);
NIGENENTITY_REVERSED("NiGeneralEntity::IsPropertySerializable", 0x00853710, ge::Forward2<gs::IsPropertySerializable>);
NIGENENTITY_REVERSED("NiGeneralEntity::IsPropertyInheritable", 0x00853760, ge::Forward2<gs::IsPropertyInheritable>);
NIGENENTITY_REVERSED("NiGeneralEntity::IsExternalAssetPath", 0x008537B0, ge::Forward3<gs::IsExternalAssetPath>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetElementCount", 0x00853800, ge::Forward2<gs::GetElementCount>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetElementCount", 0x00853850, ge::Forward3<gs::SetElementCount>);
NIGENENTITY_REVERSED("NiGeneralEntity::IsCollection", 0x008538A0, ge::Forward2<gs::IsCollection>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(float)", 0x008538F0, ge::Forward3<gs::GetFloat>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(bool)", 0x00853940, ge::Forward3<gs::GetBool>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(int)", 0x00853990, ge::Forward3<gs::GetInt>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(unsigned int)", 0x008539E0, ge::Forward3<gs::GetUInt>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(short)", 0x00853A30, ge::Forward3<gs::GetShort>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(unsigned short)", 0x00853A80, ge::Forward3<gs::GetUShort>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(NiFixedString)", 0x00853AD0, ge::Forward3<gs::GetString>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(NiPoint2)", 0x00853B20, ge::Forward3<gs::GetPoint2>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(NiPoint3)", 0x00853B70, ge::Forward3<gs::GetPoint3>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(NiQuaternion)", 0x00853BC0, ge::Forward3<gs::GetQuaternion>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(NiMatrix3)", 0x00853C10, ge::Forward3<gs::GetMatrix3>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(NiColor)", 0x00853C60, ge::Forward3<gs::GetColor>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(NiColorA)", 0x00853CB0, ge::Forward3<gs::GetColorA>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(NiObject*)", 0x00853D00, ge::Forward3<gs::GetObject>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(NiEntityInterface*)", 0x00853D50, ge::Forward3<gs::GetEntity>);
NIGENENTITY_REVERSED("NiGeneralEntity::GetPropertyData(void*)", 0x00853DA0, ge::Forward4<gs::GetVoid>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(float)", 0x00853DF0, ge::ForwardSetFloat);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(bool)", 0x00853E40, ge::Forward3<gs::SetBool>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(int)", 0x00853E90, ge::Forward3<gs::SetInt>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(unsigned int)", 0x00853EE0, ge::Forward3<gs::SetUInt>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(short)", 0x00853F30, ge::Forward3<gs::SetShort>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(unsigned short)", 0x00853F80, ge::Forward3<gs::SetUShort>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(NiFixedString)", 0x00853FD0, ge::Forward3<gs::SetString>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(NiPoint2)", 0x00854020, ge::Forward3<gs::SetPoint2>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(NiPoint3)", 0x00854070, ge::Forward3<gs::SetPoint3>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(NiQuaternion)", 0x008540C0, ge::Forward3<gs::SetQuaternion>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(NiMatrix3)", 0x00854110, ge::Forward3<gs::SetMatrix3>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(NiColor)", 0x00854160, ge::Forward3<gs::SetColor>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(NiColorA)", 0x008541B0, ge::Forward3<gs::SetColorA>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(NiObject*)", 0x00854200, ge::Forward3<gs::SetObject>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(NiEntityInterface*)", 0x00854250, ge::Forward3<gs::SetEntity>);
NIGENENTITY_REVERSED("NiGeneralEntity::SetPropertyData(void*)", 0x008542A0, ge::Forward4<gs::SetVoid>);
NIGENENTITY_REVERSED("NiTObjectSet<NiEntityComponentInterfaceIPtr>::RemoveAt", 0x008542F0, &ge::IPtrSetRemoveAt);
NIGENENTITY_REVERSED("NiGeneralEntity::_SDMInit", 0x00854350, &ge::SDMInit);
NIGENENTITY_REVERSED("NiGeneralEntity::_SDMShutdown", 0x00854360, &ge::SDMShutdown);
NIGENENTITY_REVERSED("NiGeneralEntity::RemoveComponent", 0x00854370, &ge::RemoveComponent);
NIGENENTITY_REVERSED("NiGeneralEntity::RemoveComponentAt", 0x00854400, &ge::RemoveComponentAt);
NIGENENTITY_REVERSED("NiGeneralEntity::RemoveAllComponents", 0x00854450, &ge::RemoveAllComponents);
NIGENENTITY_REVERSED("NiGeneralEntity::GetClassName", 0x00854460, &ge::GetClassName_);
NIGENENTITY_REVERSED("NiGeneralEntity::GetName", 0x00854490, &ge::GetName_);
NIGENENTITY_REVERSED("NiGeneralEntity::SetName", 0x008544C0, &ge::SetName_);
NIGENENTITY_REVERSED("NiDelete[]<NiEntityComponentInterfaceIPtr>", 0x008544E0, &ge::IPtrArrayDelete);
NIGENENTITY_REVERSED("NiGeneralEntity::ReplaceMasterEntity", 0x00854530, &ge::ReplaceMasterEntity);
NIGENENTITY_REVERSED("NiTObjectSet<NiEntityComponentInterfaceIPtr>::Add", 0x00854770, &ge::IPtrSetAdd);
NIGENENTITY_REVERSED("NiGeneralEntity::AddComponent", 0x008547D0, &ge::AddComponent);
NIGENENTITY_REVERSED("NiGeneralEntity::IsComponentProperty", 0x00854830, &ge::IsComponentProperty);
NIGENENTITY_REVERSED("NiGeneralEntity::NiGeneralEntity", 0x008548B0, &ge::Construct);
NIGENENTITY_REVERSED("NiGeneralEntity::InterfaceDeletingDestructor", 0x00854910, &ge::InterfaceDeletingDestructor);
NIGENENTITY_REVERSED("NiGeneralEntity::~NiGeneralEntity", 0x00854920, &ge::Destruct);
NIGENENTITY_REVERSED("NiGeneralEntity::DeletingDestructor", 0x008549A0, &ge::DeletingDestructor);
NIGENENTITY_REVERSED("NiGeneralEntity::NiGeneralEntity(values)", 0x008549C0, &ge::ConstructWith);
NIGENENTITY_REVERSED("NiGeneralEntity::Clone", 0x00854A60, &ge::Clone);
