// NiPropertyState (vtable 0x960784, 0x38 bytes): the twelve properties in effect at a geometry,
// one reference per property type (+0x8 .. +0x34), the defaults (each property class's
// ms_spDefault) by Reset. NiDynamicEffectState (vtable 0x951BEC, 0x20 bytes): the lights,
// projected lights and projected shadows in effect (lists of {next, effect} sorted by the
// effect's index), the environment and fog maps. And NiBound's own functions (streaming,
// equality, the bound of points, merging two, transforming one).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include <windows.h>

#include "game/HeapCheck.h"
#include "log.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kNiPropertyStateVtable = 0x00960784;
constexpr uintptr_t kNiDynamicEffectStateVtable = 0x00951BEC;
// (testing: the vtable while the destructor's body runs under heap_check; a virtual call on the
// state by another thread then crashes at this address, reported)
constexpr uintptr_t kDestructingVtable = 0x0BADDEAD;
GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(NiPoint3, NiPoint3_ZERO, 0x020E2D44);
GAME_VAR(float, NiBound_ms_fTolerance, 0x00BC8B34);   // 1e-6

// the property classes' defaults, in the state's order
inline const re::Ptr32<NiObject>* const kDefaultProperties[12] = {
    reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E2B24), reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E3EB0),
    reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E2F2C), reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E2D38),
    reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E3DD8), reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E2F64),
    reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E3C9C), reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E2F9C),
    reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E28F0), reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E303C),
    reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E3AF8), reinterpret_cast<const re::Ptr32<NiObject>*>(0x020E2B5C),
};

struct NiPropertyState : NiRefObject {
    re::Ptr32<NiObject> properties[12];   // +0x8 (references)
};
static_assert(sizeof(NiPropertyState) == 0x38);

// a node of the effect state's lists
struct EffectNode {
    re::Ptr32<EffectNode> next;   // +0x0
    re::Ptr32<uint8_t> effect;    // +0x4 (NiDynamicEffect: its index +0xC8)
};

struct NiDynamicEffectState : NiRefObject {
    uint8_t multiTexture;          // +0x8 (set while it has projections or maps)
    uint8_t pad9[3];
    re::Ptr32<EffectNode> lights;            // +0xC
    re::Ptr32<EffectNode> projectedLights;   // +0x10
    re::Ptr32<EffectNode> projectedShadows;  // +0x14
    re::Ptr32<uint8_t> environmentMap;       // +0x18
    re::Ptr32<uint8_t> fogMap;               // +0x1C
};
static_assert(sizeof(NiDynamicEffectState) == 0x20);

GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);
GAME_FN(NiPoint3*, __thiscall, Matrix3_MultiplyPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, PropertyState_Destruct, 0x007D2060, NiPropertyState* self);
GAME_FN(void, __thiscall, PropertyState_Reset, 0x007D20E0, NiPropertyState* self);
GAME_FN(void, __cdecl, EffectList_Copy, 0x007D24B0, const EffectNode* from, re::Ptr32<EffectNode>* to);
GAME_FN(void, __cdecl, EffectList_Insert, 0x007D2610, re::Ptr32<EffectNode>* list, uint8_t* effect);
GAME_FN(void, __cdecl, EffectList_Remove, 0x007D2690, re::Ptr32<EffectNode>* list, uint8_t* effect);

inline void Unref(NiObject* object) {
    if (object != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(&object->refCount)) == 0) {
        Virtual<void>(object, 1);
    }
}

inline int32_t IndexOf(const uint8_t* effect) {
    return At<int32_t>(effect, 0xC8);
}

inline void ConstructRefObject(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    _InterlockedIncrement(&ms_uiObjects);
}

// ---- NiPropertyState

// 0x7D1FB0 NiPropertyState(other): its properties held
NiPropertyState* __fastcall PropertyState_CopyConstructImpl(NiPropertyState* self, re::Edx,
                                                            const NiPropertyState* other) {
    ConstructRefObject(self);
    self->vtable = reinterpret_cast<void*>(kNiPropertyStateVtable);
    for (int i = 0; i < 12; i++) {
        self->properties[i] = nullptr;
    }
    for (int i = 0; i < 12; i++) {
        NiObject* old = self->properties[i];
        if (old != other->properties[i]) {
            Unref(old);
            NiObject* property = other->properties[i];
            self->properties[i] = property;
            AddRef(property);
        }
    }
    return self;
}

// 0x7D2060 (last to first)
void __fastcall PropertyState_DestructImpl(NiPropertyState* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiPropertyStateVtable);
    for (int i = 11; i >= 0; i--) {
        Unref(self->properties[i]);
    }
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    _InterlockedDecrement(&ms_uiObjects);
}

// 0x7D20C0 vf0
NiPropertyState* __fastcall PropertyState_DeletingDestructorImpl(NiPropertyState* self, re::Edx, uint32_t flags) {
    PropertyState_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x38);
    }
    return self;
}

// 0x7D20E0 Reset: each property the class's default
void __fastcall PropertyState_ResetImpl(NiPropertyState* self, re::Edx) {
    for (int i = 0; i < 12; i++) {
        NiObject* property = *kDefaultProperties[i];
        NiObject* old = self->properties[i];
        if (old != property) {
            Unref(old);
            self->properties[i] = property;
            AddRef(property);
        }
    }
}

// 0x7D2370 NiPropertyState: the defaults
NiPropertyState* __fastcall PropertyState_ConstructImpl(NiPropertyState* self, re::Edx) {
    ConstructRefObject(self);
    self->vtable = reinterpret_cast<void*>(kNiPropertyStateVtable);
    for (int i = 0; i < 12; i++) {
        self->properties[i] = nullptr;
    }
    PropertyState_Reset(self);
    return self;
}

// ---- NiBound

// 0x7D23D0 LoadBinary(stream)
void __fastcall Bound_LoadBinaryImpl(NiBound* self, re::Edx, void* stream) {
    Point3_LoadBinary(&self->center, stream);
    LoadBytes(stream, &self->radius, 4);
}

// 0x7D2410 SaveBinary(stream)
void __fastcall Bound_SaveBinaryImpl(NiBound* self, re::Edx, void* stream) {
    Point3_SaveBinary(&self->center, stream);
    SaveBytes(stream, &self->radius, 4);
}

// 0x7D2450 GetViewerString(prefix): "prefix = (x, y, z) , r" in a new block
char* __fastcall Bound_GetViewerStringImpl(const NiBound* self, re::Edx, const char* prefix) {
    size_t length = 0;
    while (prefix[length] != '\0') {
        length++;
    }
    size_t size = length + 0x41;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, "%s = (%g, %g, %g) , %g", prefix, double(self->center.x), double(self->center.y),
              double(self->center.z), double(self->radius));
    return text;
}

// 0x7D2840 operator==: the radius, then the center
uint32_t __fastcall Bound_EqualsImpl(const NiBound* self, re::Edx, const NiBound* other) {
    if (other->radius == self->radius && other->center.x == self->center.x && other->center.y == self->center.y &&
        other->center.z == self->center.z) {
        return 1;
    }
    return 0;
}

// 0x7D2890 ComputeFromData(count, points): the box's center, the farthest point's distance
// (none: the origin, radius 0)
void __fastcall Bound_ComputeFromDataImpl(NiBound* self, re::Edx, int count, const NiPoint3* points) {
    if (count <= 0) {
        self->center = NiPoint3_ZERO;
        self->radius = 0.0f;
        return;
    }
    NiPoint3 minimum = points[0];
    NiPoint3 maximum = points[0];
    for (int i = 1; i < count; i++) {
        const NiPoint3& p = points[i];
        if (p.x < minimum.x) {
            minimum.x = p.x;
        }
        if (p.y < minimum.y) {
            minimum.y = p.y;
        }
        if (p.z < minimum.z) {
            minimum.z = p.z;
        }
        if (p.x > maximum.x) {
            maximum.x = p.x;
        }
        if (p.y > maximum.y) {
            maximum.y = p.y;
        }
        if (p.z > maximum.z) {
            maximum.z = p.z;
        }
    }
    float sumX = maximum.x + minimum.x;
    float sumY = maximum.y + minimum.y;
    float sumZ = maximum.z + minimum.z;
    self->center.x = sumX * 0.5f;
    self->center.y = sumY * 0.5f;
    self->center.z = sumZ * 0.5f;
    float maxSquared = 0.0f;
    for (int i = 0; i < count; i++) {
        float dx = points[i].x - self->center.x;
        float dy = points[i].y - self->center.y;
        float dz = points[i].z - self->center.z;
        float squared = dy * dy + dx * dx + dz * dz;
        if (maxSquared < squared) {
            maxSquared = squared;
        }
    }
    self->radius = crt::Sqrtf(maxSquared);
}

// 0x7D2E60 Merge(other): the smallest sphere holding both (one inside the other: that one)
void __fastcall Bound_MergeImpl(NiBound* self, re::Edx, const NiBound* other) {
    float dx = self->center.x - other->center.x;
    float dy = self->center.y - other->center.y;
    float dz = self->center.z - other->center.z;
    float lengthSquared = dx * dx + dy * dy + dz * dz;
    float dr = other->radius - self->radius;
    float drSquared = dr * dr;
    float length;
    if (dr >= 0.0f) {
        if (lengthSquared <= drSquared) {
            self->center = other->center;
            self->radius = other->radius;
            return;
        }
        length = crt::Sqrtf(lengthSquared);
    } else {
        if (!(lengthSquared > drSquared)) {
            return;
        }
        length = crt::Sqrtf(lengthSquared);
    }
    if (NiBound_ms_fTolerance < length) {
        float t = (length - dr) / (length + length);
        float ox = dx * t;
        float oy = dy * t;
        float oz = t * dz;
        self->center.x = other->center.x + ox;
        self->center.y = oy + other->center.y;
        self->center.z = other->center.z + oz;
    }
    self->radius = (length + other->radius + self->radius) * 0.5f;
}

// 0x7D2FD0 Update(bound, transform): the bound moved by the transform
void __fastcall Bound_UpdateImpl(NiBound* self, re::Edx, const NiBound* bound, const NiTransform* transform) {
    NiPoint3 rotated;
    const NiPoint3* p = Matrix3_MultiplyPoint(&transform->rotate, &rotated, &bound->center);
    float scale = transform->scale;
    float x = p->x * scale;
    float y = p->y * scale;
    float z = scale * p->z;
    self->center.x = transform->translate.x + x;
    self->center.y = transform->translate.y + y;
    self->center.z = transform->translate.z + z;
    self->radius = bound->radius * transform->scale;
}

// ---- NiDynamicEffectState

// 0x7D24B0 (static) a list copied into new nodes
void __cdecl EffectList_CopyImpl(const EffectNode* from, re::Ptr32<EffectNode>* to) {
    if (from == nullptr) {
        return;
    }
    heapcheck::CheckItem(from, nullptr, "NiDynamicEffectState::ListCopy (the first node)");   // (testing)
    EffectNode* node = static_cast<EffectNode*>(NiNew(8));
    *to = node;
    node->effect = from->effect;
    EffectNode* last = *to;
    while (from->next != nullptr) {
        heapcheck::CheckItem(from->next, from, "NiDynamicEffectState::ListCopy");
        EffectNode* added = static_cast<EffectNode*>(NiNew(8));
        last->next = added;
        from = from->next;
        last = added;
        added->effect = from->effect;
    }
    last->next = nullptr;
}

// 0x7D2500
void __fastcall EffectState_DestructImpl(NiDynamicEffectState* self, re::Edx) {
    // (testing, bullyre.ini "heap_check": a state destructed twice is reported with both stacks:
    // the 2026-10-30 soaks. The vtable is swapped atomically: only a live state's is its own
    // (another destructor in progress leaves kDestructingVtable, one finished kNiRefObjectVtable))
    if (heapcheck::g_validateFrees) {
#if defined(_WIN64)
        // (the vtable slot is the game's 4-byte pointer: a 32-bit exchange)
        void* seen = reinterpret_cast<void*>(uintptr_t(uint32_t(_InterlockedCompareExchange(
            reinterpret_cast<volatile long*>(&self->vtable), long(kDestructingVtable), long(kNiDynamicEffectStateVtable)))));
#else
        void* seen = InterlockedCompareExchangePointer(&self->vtable, reinterpret_cast<void*>(kDestructingVtable),
                                                       reinterpret_cast<void*>(kNiDynamicEffectStateVtable));
#endif
        if (seen != reinterpret_cast<void*>(kNiDynamicEffectStateVtable)) {
            const char* why = seen == reinterpret_cast<void*>(kDestructingVtable) ? "another thread is in its destructor now"
                              : seen == reinterpret_cast<void*>(kNiRefObjectVtable) ? "its destructor finished before"
                                                                                     : "not a state";
            re::Log("heap_check: NiDynamicEffectState %p destructed again on thread %u (vtable was %p: %s; refCount %d)",
                    static_cast<void*>(self), unsigned(GetCurrentThreadId()), seen, why, int(self->refCount));
            re::Log("    this thread's stack:");
            heapcheck::LogStack();
            heapcheck::Report(self);
            heapcheck::Trace(self, heapcheck::kFree);
            *static_cast<volatile int*>(nullptr) = 0;
        }
        heapcheck::Trace(self, heapcheck::kFree);
    } else {
        self->vtable = reinterpret_cast<void*>(kNiDynamicEffectStateVtable);
    }
    re::Ptr32<EffectNode>* const lists[3] = {&self->lights, &self->projectedLights, &self->projectedShadows};
    heapcheck::SetContext(self);
    for (int i = 0; i < 3; i++) {
        int position = 0;
        EffectNode* previous = nullptr;
        EffectNode previousWords = {nullptr, nullptr};
        EffectNode* walked[32];
        EffectNode walkedWords[32];
        while (*lists[i] != nullptr) {
            EffectNode* node = *lists[i];
            // (testing, "heap_check": a node that can't be one (freed and poisoned, or garbage)
            // is reported with the state's history before it's read, and the node before it,
            // whose next it was, with the free that may have put it in the quarantine)
            if (heapcheck::g_validateFrees) {
                heapcheck::CheckItem(node, previous, "NiDynamicEffectState::~NiDynamicEffectState");
                uintptr_t v = reinterpret_cast<uintptr_t>(node);
                if ((v & 3) != 0 || v < 0x10000 || (v & 0xFFFFFF00) == 0xDDDDDD00) {
                    re::Log("heap_check: NiDynamicEffectState %p: list %d node %d is %p (refCount %d) on thread %u",
                            static_cast<void*>(self), i, position, static_cast<void*>(node), int(self->refCount),
                            unsigned(GetCurrentThreadId()));
                    if (previous != nullptr) {
                        re::Log("    the node before it, %p, read as next %p effect %p", static_cast<void*>(previous),
                                static_cast<void*>(previousWords.next), static_cast<void*>(previousWords.effect));
                    }
                    for (int w = 0; w < position && w < 32; w++) {
                        re::Log("    node %d at %p: next %p effect %p", w, static_cast<void*>(walked[w]),
                                static_cast<void*>(walkedWords[w].next), static_cast<void*>(walkedWords[w].effect));
                        heapcheck::ReportCommonBlock(walked[w]);
                        heapcheck::ReportHistory(walked[w]);
                    }
                    re::Log("    this thread's stack:");
                    heapcheck::LogStack();
                    heapcheck::Report(self);
                    *static_cast<volatile int*>(nullptr) = 0;
                }
            }
            previousWords = *node;   // (kept for the report above: the quarantine fills the node)
            previous = node;
            if (position < 32) {
                walked[position] = node;
                walkedWords[position] = *node;
            }
            *lists[i] = node->next;
            NiDelete(node, 8);
            position++;
        }
    }
    heapcheck::SetContext(nullptr);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    _InterlockedDecrement(&ms_uiObjects);
    heapcheck::Trace(self, heapcheck::kFreed);
}

// 0x7D2580 Copy: a new state with copies of the lists (no allocation check)
NiDynamicEffectState* __fastcall EffectState_CopyImpl(const NiDynamicEffectState* self, re::Edx) {
    NiDynamicEffectState* copy = static_cast<NiDynamicEffectState*>(NiNew(0x20));
    if (copy != nullptr) {
        ConstructRefObject(copy);
        copy->vtable = reinterpret_cast<void*>(kNiDynamicEffectStateVtable);
        copy->multiTexture = 0;
        copy->lights = nullptr;
        copy->projectedLights = nullptr;
        copy->projectedShadows = nullptr;
        copy->environmentMap = nullptr;
        copy->fogMap = nullptr;
    }
    copy->multiTexture = self->multiTexture;
    EffectList_Copy(self->lights, &copy->lights);
    EffectList_Copy(self->projectedLights, &copy->projectedLights);
    EffectList_Copy(self->projectedShadows, &copy->projectedShadows);
    copy->environmentMap = self->environmentMap;
    copy->fogMap = self->fogMap;
    heapcheck::Trace(copy, heapcheck::kAlloc);   // (testing: see EffectState_DestructImpl)
    return copy;
}

// 0x7D2610 (static) the effect inserted in index order (not when its index is there)
void __cdecl EffectList_InsertImpl(re::Ptr32<EffectNode>* list, uint8_t* effect) {
    int32_t index = IndexOf(effect);
    EffectNode* head = *list;
    heapcheck::CheckItem(head, nullptr, "NiDynamicEffectState::ListInsertSorted (the head)");   // (testing)
    if (head == nullptr || index < IndexOf(head->effect)) {
        EffectNode* node = static_cast<EffectNode*>(NiNew(8));
        node->effect = effect;
        node->next = *list;
        *list = node;
        return;
    }
    if (index == IndexOf(head->effect)) {
        return;
    }
    EffectNode* previous = head;
    EffectNode* current = head->next;
    for (; current != nullptr; current = current->next) {
        heapcheck::CheckItem(current, previous, "NiDynamicEffectState::ListInsertSorted");
        if (index <= IndexOf(current->effect)) {
            if (index == IndexOf(current->effect)) {
                return;
            }
            break;
        }
        previous = current;
    }
    EffectNode* node = static_cast<EffectNode*>(NiNew(8));
    node->effect = effect;
    node->next = current;
    previous->next = node;
}

// 0x7D2690 (static) the node with the effect's index taken out
void __cdecl EffectList_RemoveImpl(re::Ptr32<EffectNode>* list, uint8_t* effect) {
    int32_t index = IndexOf(effect);
    EffectNode* head = *list;
    heapcheck::CheckItem(head, nullptr, "NiDynamicEffectState::ListRemove (the head)");   // (testing)
    if (head == nullptr || index < IndexOf(head->effect)) {
        return;
    }
    if (index == IndexOf(head->effect)) {
        *list = head->next;
        heapcheck::SetContext(list);   // (testing: the list field, 0xC/0x10/0x14 into its state)
        NiDelete(head, 8);
        heapcheck::SetContext(nullptr);
        return;
    }
    EffectNode* previous = head;
    for (EffectNode* current = head->next; current != nullptr; current = current->next) {
        heapcheck::CheckItem(current, previous, "NiDynamicEffectState::ListRemove");
        if (index <= IndexOf(current->effect)) {
            if (index != IndexOf(current->effect)) {
                return;
            }
            previous->next = current->next;
            heapcheck::SetContext(list);
            NiDelete(current, 8);
            heapcheck::SetContext(nullptr);
            return;
        }
        previous = current;
    }
}

// 0x7D2700 AddEffect(effect): a light (type < 8) to the lights; a texture effect by its kind
// (+0x164: 0 projected light, 1 projected shadow, 2 environment map, 3 fog map)
void __fastcall EffectState_AddEffectImpl(NiDynamicEffectState* self, re::Edx, uint8_t* effect) {
    if (effect == nullptr) {
        return;
    }
    if (effect[0xC5] <= 7) {
        EffectList_Insert(&self->lights, effect);
        return;
    }
    switch (At<uint32_t>(effect, 0x164)) {
    case 0:
        EffectList_Insert(&self->projectedLights, effect);
        break;
    case 1:
        EffectList_Insert(&self->projectedShadows, effect);
        break;
    case 2:
        self->environmentMap = effect;
        break;
    case 3:
        self->fogMap = effect;
        break;
    default:
        break;
    }
    self->multiTexture = 1;
}

// 0x7D2790 RemoveEffect(effect)
void __fastcall EffectState_RemoveEffectImpl(NiDynamicEffectState* self, re::Edx, uint8_t* effect) {
    if (effect == nullptr) {
        return;
    }
    if (effect[0xC5] <= 7) {
        EffectList_Remove(&self->lights, effect);
        return;
    }
    switch (At<uint32_t>(effect, 0x164)) {
    case 0:
        EffectList_Remove(&self->projectedLights, effect);
        break;
    case 1:
        EffectList_Remove(&self->projectedShadows, effect);
        break;
    case 2:
        if (effect == self->environmentMap) {
            self->environmentMap = nullptr;
        }
        break;
    case 3:
        if (effect == self->fogMap) {
            self->fogMap = nullptr;
        }
        break;
    default:
        break;
    }
    if (self->projectedLights == nullptr && self->projectedShadows == nullptr && self->environmentMap == nullptr &&
        self->fogMap == nullptr) {
        self->multiTexture = 0;
        return;
    }
    self->multiTexture = 1;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPROPSTATE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIPROPSTATE_REVERSED("NiPropertyState::NiPropertyState(const&)", 0x007D1FB0, ni::PropertyState_CopyConstructImpl);
NIPROPSTATE_REVERSED("NiPropertyState::~NiPropertyState", 0x007D2060, ni::PropertyState_DestructImpl);
NIPROPSTATE_REVERSED("NiPropertyState::DeletingDestructor", 0x007D20C0, ni::PropertyState_DeletingDestructorImpl);
NIPROPSTATE_REVERSED("NiPropertyState::Reset", 0x007D20E0, ni::PropertyState_ResetImpl);
NIPROPSTATE_REVERSED("NiPropertyState::NiPropertyState", 0x007D2370, ni::PropertyState_ConstructImpl);
NIPROPSTATE_REVERSED("NiBound::LoadBinary", 0x007D23D0, ni::Bound_LoadBinaryImpl);
NIPROPSTATE_REVERSED("NiBound::SaveBinary", 0x007D2410, ni::Bound_SaveBinaryImpl);
NIPROPSTATE_REVERSED("NiBound::GetViewerString", 0x007D2450, ni::Bound_GetViewerStringImpl);
NIPROPSTATE_REVERSED("NiDynamicEffectState::ListCopy", 0x007D24B0, ni::EffectList_CopyImpl);
NIPROPSTATE_REVERSED("NiDynamicEffectState::~NiDynamicEffectState", 0x007D2500, ni::EffectState_DestructImpl);
NIPROPSTATE_REVERSED("NiDynamicEffectState::Copy", 0x007D2580, ni::EffectState_CopyImpl);
NIPROPSTATE_REVERSED("NiDynamicEffectState::ListInsertSorted", 0x007D2610, ni::EffectList_InsertImpl);
NIPROPSTATE_REVERSED("NiDynamicEffectState::ListRemove", 0x007D2690, ni::EffectList_RemoveImpl);
NIPROPSTATE_REVERSED("NiDynamicEffectState::AddEffect", 0x007D2700, ni::EffectState_AddEffectImpl);
NIPROPSTATE_REVERSED("NiDynamicEffectState::RemoveEffect", 0x007D2790, ni::EffectState_RemoveEffectImpl);
NIPROPSTATE_REVERSED("NiBound::operator==", 0x007D2840, ni::Bound_EqualsImpl);
NIPROPSTATE_REVERSED("NiBound::ComputeFromData", 0x007D2890, ni::Bound_ComputeFromDataImpl);
NIPROPSTATE_REVERSED("NiBound::Merge", 0x007D2E60, ni::Bound_MergeImpl);
NIPROPSTATE_REVERSED("NiBound::Update", 0x007D2FD0, ni::Bound_UpdateImpl);
