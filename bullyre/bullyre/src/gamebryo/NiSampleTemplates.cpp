// The sample framework's small pieces: the UI signals and slots (NiUISignal0/1, NiUIBaseSlot1,
// NiUIMemberSlot1: subscription both ways, emitting, the destructors taking themselves off every
// other end), the UI manager's input bindings, and the template instantiations the linker
// folded into this range (NiTArray destructors and SetAt, NiTPrimitiveSet and NiTObjectSet
// operations, NiTNewInterface's delete[] of NiPointers, NiViewRenderClick::AppendRenderView)
// with a few tiny helpers. Each is called by many classes' code: names after the first owner
// or Gamebryo's template.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSampleFramework.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::sample {
namespace {

GAME_FN(void, __thiscall, PointerList_AddTail, 0x007843D0, void* list, const re::Ptr32<void>* element);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t size);

constexpr uint32_t kStringArrayBaseVtable = 0x008FF274;    // NiTArray<NiString*>
constexpr uint32_t kCreatorArrayBaseVtable = 0x008FF27C;   // NiTArray<library creator>
constexpr uint32_t kParserArrayBaseVtable = 0x008FF284;    // NiTArray<parser runner>
constexpr uint32_t kGroupArrayBaseVtable = 0x008FF5AC;     // NiTArray<NiPointer<NiUIRenderGroup>>

// NiViewRenderClick::AppendRenderView(view) (folded: NiCursorRenderClick::AppendCursor too):
// the list at +0x4C gets a reference
void __fastcall AppendRenderView(uint8_t* self, re::Edx, NiRefObject* view) {
    NiRefObject* held = view;
    AddRef(view);
    PointerList_AddTail(self + 0x4C, re::PtrCast<const re::Ptr32<void>*>(&held));
    Release(view);
}

// NiUISignal1<unsigned char>'s destructor: off every slot (each slot's set loses it, then
// Unsubscribe takes the slot off this one's), the set freed. (A slot not listing this one
// loops for ever: the game's)
void __fastcall Signal1Uchar_DestructImpl(NiUISignal* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiUISignal1UcharVtable);
    while (self->set.used != 0) {
        NiUISignal* other = self->set.base[0];
        uint32_t count = other->set.used;
        for (uint32_t i = 0; i < count; i++) {
            if (other->set.base[i] == self) {
                other->set.used--;
                other->set.base[i] = other->set.base[other->set.used];
                BaseSlot0_Unsubscribe(self, other);
                break;
            }
        }
    }
    NiFree(self->set.base);
}

// the deleting destructors of the NiTPrimitiveArrays NiSample keeps (the base's vtable set:
// the derived destructor is empty)
template <uint32_t Vtable>
void* __fastcall PrimitiveArrayDeletingDestructor(NiTArray<void*>* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(Vtable);
    NiFree(self->base);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// NiUISignal1<unsigned char>'s deleting destructor
NiUISignal* __fastcall Signal1Uchar_DeletingDestructor(NiUISignal* self, re::Edx, int flags) {
    Signal1Uchar_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// NiTArray<NiPointer<NiUIRenderGroup>, NiTNewInterface>'s deleting destructor: delete[] of
// the references
void* __fastcall GroupArrayDeletingDestructor(NiTArray<void*>* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kGroupArrayBaseVtable);
    PointerArray_Delete(self->base);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// Subscribe(other) (NiUISignal1<unsigned char> and its slots, folded): added to this one's set
// (grown to double, or 1) unless there, then this one to the other's
template <uintptr_t Self>
void __fastcall SubscribeImpl(NiUISignal* self, re::Edx, NiUISignal* other) {
    uint32_t count = self->set.used;
    for (uint32_t i = 0; i < count; i++) {
        if (self->set.base[i] == other) {
            return;
        }
    }
    uint32_t alloced = self->set.alloced;
    if (count == alloced) {
        PrimitiveSet_Realloc(&self->set, alloced != 0 ? alloced * 2 : 1);
    }
    self->set.base[self->set.used] = other;
    self->set.used++;
    reinterpret_cast<void(__thiscall*)(NiUISignal*, NiUISignal*)>(Self)(other, self);
}

// NiUIManager's game pad binding (0x28): its constructor and deleting destructor
NiUIGamePadBinding* __fastcall GamePadBinding_Construct(NiUIGamePadBinding* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiUIGamePadBindingVtable);
    self->signal.vtable = reinterpret_cast<void*>(kNiUISignal1UcharVtable);
    self->signal.set.base = nullptr;
    self->signal.set.alloced = 0;
    self->signal.set.used = 0;
    return self;
}
NiUIGamePadBinding* __fastcall GamePadBinding_DeletingDestructor(NiUIGamePadBinding* self, re::Edx, int flags) {
    Signal1Uchar_Destruct(&self->signal);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x28);
    }
    return self;
}

// the keyboard / mouse binding (0x24)
NiUIKeyBinding* __fastcall KeyBinding_Construct(NiUIKeyBinding* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiUIKeyBindingVtable);
    self->signal.vtable = reinterpret_cast<void*>(kNiUISignal1UcharVtable);
    self->signal.set.base = nullptr;
    self->signal.set.alloced = 0;
    self->signal.set.used = 0;
    return self;
}
NiUIKeyBinding* __fastcall KeyBinding_DeletingDestructor(NiUIKeyBinding* self, re::Edx, int flags) {
    Signal1Uchar_Destruct(&self->signal);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x24);
    }
    return self;
}

// NiUISignal0::EmitSignal: every slot's ReceiveSignal (slot 1); the count read once
void __fastcall Signal0_EmitImpl(NiUISignal* self, re::Edx) {
    uint32_t count = self->set.used;
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(self->set.base[i], 1);
    }
}

// NiNavControlInput's operator== (every field)
uint8_t __fastcall ControlInput_EqualsImpl(const NiNavControlInput* self, re::Edx, const NiNavControlInput* other) {
    if (self->devices == other->devices && self->key == other->key && self->keyModifiers == other->keyModifiers &&
        self->mouseButton == other->mouseButton && self->mouseModifiers == other->mouseModifiers &&
        self->gamePadButton == other->gamePadButton && self->gamePadModifiers == other->gamePadModifiers &&
        self->mouseAxis == other->mouseAxis && self->gamePadAxis == other->gamePadAxis) {
        return 1;
    }
    return 0;
}

// the navigation's out = origin + dir * t (a member that doesn't read this; the products first)
NiPoint3* __fastcall PointAlongAxisImpl(void*, re::Edx, const NiPoint3* dir, float t, const NiPoint3* origin, NiPoint3* out) {
    float x = dir->x * t;
    float y = dir->y * t;
    float z = t * dir->z;
    float rx = origin->x + x;
    float ry = origin->y + y;
    float rz = origin->z + z;
    out->x = rx;
    out->y = ry;
    out->z = rz;
    return out;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

// NiTArray<T*>::SetAt(index, element) (folded over every pointer type): the used count kept,
// the size grown past the index
void __fastcall PrimitiveArray_SetAtImpl(NiTArray<void*>* self, re::Edx, uint32_t index, const re::Ptr32<void>* element) {
    if (index >= self->size) {
        self->size = uint16_t(index + 1);
        if (*element != nullptr) {
            self->esize++;
        }
        self->base[index] = *element;
        return;
    }
    if (*element != nullptr) {
        if (self->base[index] == nullptr) {
            self->esize++;
        }
    } else if (self->base[index] != nullptr) {
        self->esize--;
    }
    self->base[index] = *element;
}

// NiInputGamePad::Button: pressed now and not before / released now (the modifiers out)
uint8_t __fastcall Button_WasPressed(const uint8_t* self, re::Edx, uint32_t* modifiers) {
    *modifiers = *reinterpret_cast<const uint32_t*>(self + 8);
    return (self[1] != 0 && self[3] == 0) ? 1 : 0;
}
uint8_t __fastcall Button_WasReleased(const uint8_t* self, re::Edx, uint32_t* modifiers) {
    *modifiers = *reinterpret_cast<const uint32_t*>(self + 8);
    return (self[1] == 0 && self[3] != 0) ? 1 : 0;
}

// NiRect<float>'s height: |top - bottom| (an x87 compare: NaN takes the second way)
float __fastcall Rect_GetHeight(const float* self, re::Edx) {
    if (self[3] < self[2]) {
        return self[2] - self[3];
    }
    return self[3] - self[2];
}

// NiTObjectSet<NiPointer<T>>::RemoveAll: each released and cleared
void __fastcall ObjectSet_RemoveAllImpl(NiTSet<NiRefObject*>* self, re::Edx) {
    for (uint32_t i = 0; i < self->alloced; i++) {
        re::Ptr32<NiRefObject>& element = self->base[i];
        if (element != nullptr) {
            Release(element);
            element = nullptr;
        }
    }
    self->used = 0;
}

// NiUISignal1<T>::EmitSignal(arg): every slot's ReceiveSignal(arg); the count read once
void __fastcall Signal1_EmitImpl(NiUISignal* self, re::Edx, uint32_t arg) {
    uint32_t count = self->set.used;
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(self->set.base[i], 1, arg);
    }
}

// NiTNewInterface<NiPointer<T>>::Deallocate: delete[] of references (the count before them),
// released from the last
void __cdecl PointerArray_DeleteImpl(re::Ptr32<NiRefObject>* base) {
    if (base == nullptr) {
        return;
    }
    int32_t count = reinterpret_cast<int32_t*>(base)[-1];
    for (int32_t i = count - 1; i >= 0; i--) {
        Release(base[i]);
    }
    NiDeleteArray(reinterpret_cast<int32_t*>(base) - 1, 4);
}

// the navigation's select: a when the flag's set, else b
float __cdecl Select(int flag, float a, float b) {
    return uint8_t(flag) != 0 ? a : b;
}

// NiUISignal::IsSubscribed(other): whether the other end's in its set
uint8_t __fastcall Signal_IsSubscribedImpl(const NiUISignal* self, re::Edx, const NiUISignal* other) {
    uint32_t index = 0;
    for (; index < self->set.used; index++) {
        if (self->set.base[index] == other) {
            break;
        }
    }
    if (index >= self->set.used) {
        index = 0xFFFFFFFF;
    }
    return index != 0xFFFFFFFF ? 1 : 0;
}

// NiTObjectSet<NiPointer<T>>::RemoveAt(index): the last moved into its place, the last cleared
void __fastcall ObjectSet_RemoveAtImpl(NiTSet<NiRefObject*>* self, re::Edx, uint32_t index) {
    self->used--;
    uint32_t last = self->used;
    re::Ptr32<NiRefObject>* base = self->base;
    NiRefObject* removed = base[index];
    if (removed != base[last]) {
        Release(removed);
        NiRefObject* moved = base[last];
        base[index] = moved;
        AddRef(moved);
    }
    re::Ptr32<NiRefObject>& slot = self->base[self->used];
    if (slot != nullptr) {
        Release(slot);
        slot = nullptr;
    }
}

// NiUIMemberSlot1<T, A>::ReceiveSignal(arg): the member function on the instance
void __fastcall MemberSlot1_ReceiveSignal(NiUIMemberSlot* self, re::Edx, uint32_t arg) {
    void* object = static_cast<uint8_t*>(self->instance) + self->adjust;
    re::PtrCast<void(__thiscall*)(void*, uint32_t)>(self->function)(object, arg);
}

// the folded signal / slot destructor (NiUISignal0, NiUISignal1<T*>; NiUIBaseSlot1<bool>):
// off every other end (its set loses this one, this one's loses it), then the other's
// Unsubscribe (which finds nothing left), the set freed
template <uint32_t Vtable>
void __fastcall SignalDestructImpl(NiUISignal* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(Vtable);
    while (self->set.used != 0) {
        NiUISignal* other = self->set.base[0];
        uint32_t count = other->set.used;
        uint32_t i = 0;
        for (; i < count; i++) {
            if (other->set.base[i] == self) {
                break;
            }
        }
        if (i >= count) {
            continue;   // (not listed: loops for ever, the game's)
        }
        other->set.used--;
        other->set.base[i] = other->set.base[other->set.used];
        count = self->set.used;
        uint32_t j = 0;
        for (; j < count; j++) {
            if (self->set.base[j] == other) {
                break;
            }
        }
        if (j >= count) {
            continue;
        }
        self->set.used--;
        self->set.base[j] = self->set.base[self->set.used];
        BaseSlot0_Unsubscribe(other, self);
    }
    NiFree(self->set.base);
}

// NiTPrimitiveSet<T>::Realloc(alloced): copied into a new array of that size (none: 0)
void __fastcall PrimitiveSet_ReallocImpl(NiTSet<void*>* self, re::Edx, uint32_t alloced) {
    if (alloced == self->alloced) {
        return;
    }
    re::Ptr32<void>* base = nullptr;
    if (alloced != 0) {
        base = static_cast<re::Ptr32<void>*>(NiMalloc(alloced * 4));
        for (uint32_t i = 0; i < self->used; i++) {
            base[i] = self->base[i];
        }
    }
    NiFree(self->base);
    self->base = base;
    self->alloced = alloced;
}

// NiUIBaseSlot1<bool>'s deleting destructor
NiUISignal* __fastcall BaseSlot1Bool_DeletingDestructor(NiUISignal* self, re::Edx, int flags) {
    BaseSlot1Bool_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// NiUIMemberSlot1<NiUIGroup, bool>'s deleting destructor
NiUIMemberSlot* __fastcall MemberSlot1Group_DeletingDestructor(NiUIMemberSlot* self, re::Edx, int flags) {
    self->instance = nullptr;
    self->adjust = 0;
    self->vtable = reinterpret_cast<void*>(kNiUIMemberSlot1GroupVtable);
    self->function = nullptr;
    BaseSlot1Bool_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x20);
    }
    return self;
}

} // namespace

#define NISAMPLETPL_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISAMPLETPL_REVERSED("NiViewRenderClick::AppendRenderView", 0x008316F0, &AppendRenderView);
NISAMPLETPL_REVERSED("NiUISignal1<unsigned char>::~NiUISignal1", 0x00831EF0, &Signal1Uchar_DestructImpl);
NISAMPLETPL_REVERSED("NiTPrimitiveArray<NiString*>::DeletingDestructor", 0x00831F60,
                     &PrimitiveArrayDeletingDestructor<kStringArrayBaseVtable>);
NISAMPLETPL_REVERSED("NiTPrimitiveArray<LibraryCreator>::DeletingDestructor", 0x00831F90,
                     &PrimitiveArrayDeletingDestructor<kCreatorArrayBaseVtable>);
NISAMPLETPL_REVERSED("NiTPrimitiveArray<ParserRunner>::DeletingDestructor", 0x00831FC0,
                     &PrimitiveArrayDeletingDestructor<kParserArrayBaseVtable>);
NISAMPLETPL_REVERSED("NiUISignal1<unsigned char>::DeletingDestructor", 0x00831FF0, &Signal1Uchar_DeletingDestructor);
NISAMPLETPL_REVERSED("NiTArray<NiPointer<NiUIRenderGroup>>::DeletingDestructor", 0x008322A0,
                     &GroupArrayDeletingDestructor);
NISAMPLETPL_REVERSED("NiUISignal1<unsigned char>::Subscribe", 0x008322D0, &SubscribeImpl<0x008322D0>);
NISAMPLETPL_REVERSED("NiUIManager::GamePadBinding::GamePadBinding", 0x00832390, &GamePadBinding_Construct);
NISAMPLETPL_REVERSED("NiUIManager::GamePadBinding::DeletingDestructor", 0x008323D0,
                     &GamePadBinding_DeletingDestructor);
NISAMPLETPL_REVERSED("NiUIManager::KeyBinding::KeyBinding", 0x00832410, &KeyBinding_Construct);
NISAMPLETPL_REVERSED("NiUIManager::KeyBinding::DeletingDestructor", 0x00832450, &KeyBinding_DeletingDestructor);
NISAMPLETPL_REVERSED("NiUISignal0::EmitSignal", 0x00833080, &Signal0_EmitImpl);
NISAMPLETPL_REVERSED("NiNavBaseController::ControlInput::operator==", 0x00833BA0, &ControlInput_EqualsImpl);
NISAMPLETPL_REVERSED("NiNavBaseController::PointAlongAxis", 0x00833C00, &PointAlongAxisImpl);
NISAMPLETPL_REVERSED("NiTArray<T*>::SetAt", 0x00833C70, &PrimitiveArray_SetAtImpl);
NISAMPLETPL_REVERSED("NiInputGamePad::Button::WasPressed", 0x00836880, &Button_WasPressed);
NISAMPLETPL_REVERSED("NiInputGamePad::Button::WasReleased", 0x008368A0, &Button_WasReleased);
NISAMPLETPL_REVERSED("NiRect<float>::GetHeight", 0x00836910, &Rect_GetHeight);
NISAMPLETPL_REVERSED("NiTObjectSet<NiPointer>::RemoveAll", 0x00836940, &ObjectSet_RemoveAllImpl);
NISAMPLETPL_REVERSED("NiUISignal1::EmitSignal", 0x00836F50, &Signal1_EmitImpl);
NISAMPLETPL_REVERSED("NiTNewInterface<NiPointer>::Deallocate", 0x00836F80, &PointerArray_DeleteImpl);
NISAMPLETPL_REVERSED("NiNavBaseController::Select", 0x008382D0, &Select);
NISAMPLETPL_REVERSED("NiUISignal::IsSubscribed", 0x00838650, &Signal_IsSubscribedImpl);
NISAMPLETPL_REVERSED("NiTObjectSet<NiPointer>::RemoveAt", 0x00838680, &ObjectSet_RemoveAtImpl);
NISAMPLETPL_REVERSED("NiUIMemberSlot1::ReceiveSignal", 0x008388F0, &MemberSlot1_ReceiveSignal);
NISAMPLETPL_REVERSED("NiUISignal0::~NiUISignal0", 0x0083A110, &SignalDestructImpl<kNiUISignalVtable>);
NISAMPLETPL_REVERSED("NiTPrimitiveSet::Realloc", 0x0083AD20, &PrimitiveSet_ReallocImpl);
NISAMPLETPL_REVERSED("NiUIBaseSlot1<bool>::~NiUIBaseSlot1", 0x0083AE70, &SignalDestructImpl<kNiUIBaseSlot1BoolVtable>);
NISAMPLETPL_REVERSED("NiUIBaseSlot1<bool>::DeletingDestructor", 0x0083AF10, &BaseSlot1Bool_DeletingDestructor);
NISAMPLETPL_REVERSED("NiUIMemberSlot1<NiUIGroup,bool>::DeletingDestructor", 0x0083AF30,
                     &MemberSlot1Group_DeletingDestructor);
NISAMPLETPL_REVERSED("NiUIBaseSlot1<bool>::Subscribe", 0x0083AF70, &SubscribeImpl<0x0083AF70>);

} // namespace ni::sample
