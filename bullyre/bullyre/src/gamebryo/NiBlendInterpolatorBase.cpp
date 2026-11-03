// NiBlendInterpolator's own slots (src/gamebryo/NiBlendInterpolator.h: vtable 0x9683D8, 0x30 bytes):
// the array of weighted, prioritised interpolators a blend works over (InterpArrayItem, 0x18
// each, NiNew[]'d with the count before it), adding and removing them, the normalized weights
// the kinds' BlendValues read (ComputeNormalizedWeights, the two-item case on its own),
// construction, cloning, streaming (the array and the cached sums; from 10.1.0.112 only when not
// manager controlled), comparison and viewer strings. Its kinds are the other chunk's
// (src/gamebryo/NiBlendInterpolators.cpp, NiBlendAccumTransformInterpolator.cpp).
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's order.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiBlendInterpolator.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::GetViewerStringBool;
using scene::GetViewerStringFloat;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBlendInterpolator_RTTI, 0x020E57A4);
GAME_VAR(uint8_t, ms_ucArrayGrowBy, 0x00BC8C64);

constexpr uintptr_t NiBlendInterpolator_vtable = 0x009683D8;
constexpr uintptr_t InterpArrayItem_Construct_address = 0x008272F0;

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(char*, __cdecl, GetViewerStringByte, 0x00785600, const char* prefix, uint8_t value);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
// NiInterpolator's (and the thunks to NiObject's the code calls)
GAME_FN(void, __thiscall, Interpolator_Construct, 0x00825DA0, void* self);
GAME_FN(void, __thiscall, Interpolator_Destruct, 0x00825DC0, void* self);
GAME_FN(void, __thiscall, Interpolator_LoadBinary, 0x007F79A0, void* self, void* stream);
GAME_FN(void, __thiscall, Interpolator_SaveBinary, 0x00806A50, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Interpolator_RegisterStreamables, 0x00807310, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Interpolator_IsEqual, 0x007769C0, void* self, void* other);
GAME_FN(void, __thiscall, Interpolator_CopyMembers, 0x007F7990, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Interpolator_GetViewerStrings, 0x00825E50, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Object_ProcessClone, 0x007553F0, void* self, void* cloning);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, BlendInterpolator_Destruct, 0x00826A30, void* self);
GAME_FN(void, __thiscall, BlendInterpolator_ComputeNormalizedWeightsFor2, 0x008265F0, void* self);
GAME_FN(InterpArrayItem*, __thiscall, InterpArrayItem_Assign, 0x00826580, InterpArrayItem* self,
        const InterpArrayItem* other);
GAME_FN(uint8_t, __thiscall, InterpArrayItem_IsEqual, 0x008268A0, const InterpArrayItem* self,
        const InterpArrayItem* other);
GAME_FN(void, __thiscall, InterpArrayItem_LoadBinary, 0x00826F70, InterpArrayItem* self, void* stream);
GAME_FN(void, __thiscall, InterpArrayItem_SaveBinary, 0x00827080, InterpArrayItem* self, void* stream);

// a NiNew[]'d array of `count` items (the count before them), each constructed; null when the
// allocation fails
InterpArrayItem* NewItems(uint32_t count) {
    auto* block = static_cast<uint32_t*>(NiNewArray(count * 0x18 + 4));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    InterpArrayItem* items = reinterpret_cast<InterpArrayItem*>(block + 1);
    VectorConstruct(items, 0x18, count, reinterpret_cast<void*>(InterpArrayItem_Construct_address));
    return items;
}

// delete[] of the items: each one's interpolator let go, last first
void DeleteItems(InterpArrayItem* items) {
    if (items == nullptr) {
        return;
    }
    int32_t count = reinterpret_cast<int32_t*>(items)[-1];
    for (int32_t i = count - 1; i >= 0; i--) {
        ReleaseRef(items[i].interpolator);
    }
    NiDeleteArray(reinterpret_cast<uint32_t*>(items) - 1, 0x18);
}

// the high and next-high priorities moved for one more priority (Add/RemoveInterpInfo's)
inline void TakePriority(NiBlendInterpolator* self, int8_t priority) {
    if (priority > self->nextHighPriority) {
        if (priority > self->highPriority) {
            self->nextHighPriority = self->highPriority;
            self->highPriority = priority;
        } else if (priority != self->highPriority) {
            self->nextHighPriority = priority;
        }
    }
}

// ---- InterpArrayItem

// 0x826580: operator=
InterpArrayItem* __fastcall Item_Assign(InterpArrayItem* self, re::Edx, const InterpArrayItem* other) {
    AssignReleasingFirst(self->interpolator, other->interpolator);
    self->weight = other->weight;
    self->normalizedWeight = other->normalizedWeight;
    self->priority = other->priority;
    self->easeSpinner = other->easeSpinner;
    self->updateTime = other->updateTime;
    return self;
}

// 0x8268A0: IsEqual (the interpolators both there and equal, or both missing; then the weight,
// the normalized weight, the priority and the ease spinner)
uint8_t __fastcall Item_IsEqual(const InterpArrayItem* self, re::Edx, const InterpArrayItem* other) {
    if (self->interpolator == nullptr) {
        if (other->interpolator != nullptr) {
            return 0;
        }
    } else if (other->interpolator == nullptr || !Virtual<uint8_t>(self->interpolator, 8, other->interpolator)) {
        return 0;
    }
    if (self->weight == other->weight && self->normalizedWeight == other->normalizedWeight &&
        self->priority == other->priority && self->easeSpinner == other->easeSpinner) {
        return 1;
    }
    return 0;
}

// 0x826F70: LoadBinary: the interpolator's link, the weights, the priority (4 bytes before
// 10.1.0.110, INT_MIN meaning -128), the ease spinner
void __fastcall Item_LoadBinary(InterpArrayItem* self, re::Edx, void* stream) {
    AssignReleasingFirst(self->interpolator, Stream_ReadLinkIDObject(stream));
    scene::LoadBytes(stream, &self->weight, 4);
    scene::LoadBytes(stream, &self->normalizedWeight, 4);
    if (scene::StreamVersion(stream) < 0x0A01006E) {
        int32_t priority;
        scene::LoadBytes(stream, &priority, 4);
        if (priority == INT32_MIN) {
            self->priority = -128;
        } else {
            self->priority = static_cast<int8_t>(priority);
        }
    } else {
        scene::LoadBytes(stream, &self->priority, 1);
    }
    scene::LoadBytes(stream, &self->easeSpinner, 4);
}

// 0x827080: SaveBinary
void __fastcall Item_SaveBinary(InterpArrayItem* self, re::Edx, void* stream) {
    Virtual<void>(stream, 12, self->interpolator);   // SaveLinkID
    scene::SaveBytes(stream, &self->weight, 4);
    scene::SaveBytes(stream, &self->normalizedWeight, 4);
    scene::SaveBytes(stream, &self->priority, 1);
    scene::SaveBytes(stream, &self->easeSpinner, 4);
}

// 0x8272F0: InterpArrayItem() (VectorConstruct's per-item constructor)
InterpArrayItem* __fastcall Item_Construct(InterpArrayItem* self, re::Edx) {
    self->weight = 0.0f;
    self->interpolator = nullptr;
    self->normalizedWeight = 0.0f;
    self->priority = 0;
    self->easeSpinner = 0.0f;
    self->updateTime = kInvalidFloat;
    return self;
}

// ---- NiBlendInterpolator

// 0x826400: NiBlendInterpolator() (for streaming: no array)
NiBlendInterpolator* __fastcall Blend_DefaultConstruct(NiBlendInterpolator* self, re::Edx) {
    Interpolator_Construct(self);
    self->weightThreshold = 0.0f;
    self->singleTime = kInvalidFloat;
    self->flags = 0;
    self->arraySize = 0;
    self->highSumOfWeights = kInvalidFloat;
    self->interpCount = 0;
    self->interpArray = nullptr;
    self->nextHighSumOfWeights = kInvalidFloat;
    self->singleInterpolator = nullptr;
    self->vtable = reinterpret_cast<void*>(NiBlendInterpolator_vtable);
    self->highEaseSpinner = kInvalidFloat;
    self->singleIdx = 0xFF;
    self->highPriority = -128;
    self->nextHighPriority = -128;
    return self;
}

// 0x826460
const RTTI* __fastcall Blend_GetRTTI(NiBlendInterpolator*, re::Edx) {
    return &NiBlendInterpolator_RTTI;
}

// 0x826470: AddInterpInfo(interp, weight, priority, ease spinner) (slot 37): the first free item
// (the arrays grown, slot 40, when there's none: 0xFF if they can't be), the priorities moved,
// the only one's index and interpolator kept when it's the only one; the weights to normalize
// again and the sums cached invalid. Returns the index
uint8_t __fastcall Blend_AddInterpInfo(NiBlendInterpolator* self, re::Edx, NiInterpolator* interpolator, float weight,
                                       int priority, float easeSpinner) {
    uint8_t arraySize = self->arraySize;
    uint8_t index = 0;
    if (arraySize != 0) {
        while (self->interpArray[index].interpolator != nullptr) {
            index++;
            if (index >= self->arraySize) {
                break;
            }
        }
    }
    if (index == arraySize && !Virtual<uint8_t>(self, 40)) {
        return 0xFF;
    }
    InterpArrayItem& item = self->interpArray[index];
    AssignReleasingFirst(item.interpolator, interpolator);
    int8_t prio = static_cast<int8_t>(priority);
    item.weight = weight;
    item.priority = prio;
    item.easeSpinner = easeSpinner;
    TakePriority(self, prio);
    self->interpCount++;
    if (self->interpCount == 1) {
        self->singleIdx = index;
        self->singleInterpolator = interpolator;
    } else {
        self->singleIdx = 0xFF;
        self->singleTime = kInvalidFloat;
        self->singleInterpolator = nullptr;
    }
    self->flags |= 4;
    self->highSumOfWeights = kInvalidFloat;
    self->nextHighSumOfWeights = kInvalidFloat;
    self->highEaseSpinner = kInvalidFloat;
    return index;
}

// 0x8265F0: ComputeNormalizedWeightsFor2: the two items in use (the first two with an
// interpolator) weighted against each other: by priority (the higher one's ease spinner shares
// the rest out) or evenly; then only the highest kept, or those under the threshold dropped (the
// second made 1 when it's under: the game sets it 0 and then 1)
void __fastcall Blend_ComputeNormalizedWeightsFor2(NiBlendInterpolator* self, re::Edx) {
    InterpArrayItem* first = nullptr;
    InterpArrayItem* second = nullptr;
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem* item = &self->interpArray[i];
        if (item->interpolator != nullptr) {
            if (first != nullptr) {
                second = item;
                break;
            }
            first = item;
        }
    }
    float weight1 = first->weight * first->easeSpinner;
    float weight2 = second->weight * second->easeSpinner;
    if (weight1 == 0.0f && weight2 == 0.0f) {
        first->normalizedWeight = 0.0f;
        second->normalizedWeight = 0.0f;
        return;
    }
    if (first->priority > second->priority) {
        if (first->easeSpinner == 1.0f) {
            first->normalizedWeight = 1.0f;
            second->normalizedWeight = 0.0f;
            return;
        }
        float oneMinus = 1.0f - first->easeSpinner;
        weight1 = weight1 * first->easeSpinner;
        weight2 = weight2 * oneMinus;
        float invSum = 1.0f / (weight2 + weight1);
        first->normalizedWeight = weight1 * invSum;
        second->normalizedWeight = weight2 * invSum;
    } else if (first->priority < second->priority) {
        if (second->easeSpinner == 1.0f) {
            first->normalizedWeight = 0.0f;
            second->normalizedWeight = 1.0f;
            return;
        }
        float oneMinus = 1.0f - second->easeSpinner;
        weight1 = weight1 * oneMinus;
        float invSum = 1.0f / (second->easeSpinner * weight2 + weight1);
        first->normalizedWeight = weight1 * invSum;
        second->normalizedWeight = invSum * (weight2 * second->easeSpinner);
    } else {
        float invSum = 1.0f / (weight2 + weight1);
        first->normalizedWeight = weight1 * invSum;
        second->normalizedWeight = weight2 * invSum;
    }
    if ((self->flags & 2) != 0) {
        if (second->normalizedWeight <= first->normalizedWeight) {
            first->normalizedWeight = 1.0f;
            second->normalizedWeight = 0.0f;
        } else {
            first->normalizedWeight = 0.0f;
            second->normalizedWeight = 1.0f;
        }
        return;
    }
    if (self->weightThreshold > 0.0f) {
        bool firstDropped = false;
        if (first->normalizedWeight < self->weightThreshold) {
            first->normalizedWeight = 0.0f;
            firstDropped = true;
        }
        if (second->normalizedWeight < self->weightThreshold) {
            second->normalizedWeight = 0.0f;
            second->normalizedWeight = 1.0f;
            return;
        }
        if (firstDropped) {
            second->normalizedWeight = 1.0f;
        }
    }
}

// 0x8267E0: Collapse (slot 30): the only interpolator's, or every one's
void __fastcall Blend_Collapse(NiBlendInterpolator* self, re::Edx) {
    if (self->interpCount == 1) {
        Virtual<void>(self->singleInterpolator, 30);
        return;
    }
    for (uint8_t i = 0; i < self->arraySize; i++) {
        NiInterpolator* interpolator = self->interpArray[i].interpolator;
        if (interpolator != nullptr) {
            Virtual<void>(interpolator, 30);
        }
    }
}

// 0x826830: GuaranteeTimeRange(start, end) (slot 32): the only interpolator's, or every one's
void __fastcall Blend_GuaranteeTimeRange(NiBlendInterpolator* self, re::Edx, float start, float end) {
    if (self->interpCount == 1) {
        Virtual<void>(self->singleInterpolator, 32, start, end);
        return;
    }
    for (uint8_t i = 0; i < self->arraySize; i++) {
        NiInterpolator* interpolator = self->interpArray[i].interpolator;
        if (interpolator != nullptr) {
            Virtual<void>(interpolator, 32, start, end);
        }
    }
}

// 0x826910: RegisterStreamables (the items' interpolators unless manager controlled)
uint8_t __fastcall Blend_RegisterStreamables(NiBlendInterpolator* self, re::Edx, void* stream) {
    if (!Interpolator_RegisterStreamables(self, stream)) {
        return 0;
    }
    if ((self->flags & 1) == 0) {
        for (uint8_t i = 0; i < self->arraySize; i++) {
            NiInterpolator* interpolator = self->interpArray[i].interpolator;
            if (interpolator != nullptr) {
                Virtual<uint8_t>(interpolator, 6, stream);
            }
        }
    }
    return 1;
}

// 0x826960: IsEqual: the same array size and items, flags, threshold, counts, the only one (equal
// and the same pointer), its time and the priorities
uint8_t __fastcall Blend_IsEqual(NiBlendInterpolator* self, re::Edx, NiBlendInterpolator* other) {
    if (!Interpolator_IsEqual(self, other)) {
        return 0;
    }
    if (self->arraySize != other->arraySize) {
        return 0;
    }
    for (uint8_t i = 0; i < self->arraySize; i++) {
        if (!InterpArrayItem_IsEqual(&self->interpArray[i], &other->interpArray[i])) {
            return 0;
        }
    }
    if (self->flags != other->flags || !(self->weightThreshold == other->weightThreshold) ||
        self->interpCount != other->interpCount || self->singleIdx != other->singleIdx) {
        return 0;
    }
    if (self->singleInterpolator != nullptr &&
        !Virtual<uint8_t>(self->singleInterpolator, 8, other->singleInterpolator)) {
        return 0;
    }
    if (self->singleInterpolator != other->singleInterpolator || !(self->singleTime == other->singleTime) ||
        self->highPriority != other->highPriority || self->nextHighPriority != other->nextHighPriority) {
        return 0;
    }
    return 1;
}

// 0x826A30: ~NiBlendInterpolator (the array deleted)
void __fastcall Blend_Destruct(NiBlendInterpolator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiBlendInterpolator_vtable);
    DeleteItems(self->interpArray);
    Interpolator_Destruct(self);
}

// 0x826AB0: RemoveInterpInfo(index) (slot 38): the item's interpolator returned (a NiInterpolatorPtr
// by value: held for the caller) and the item cleared; the priorities found again when it had
// one of them; the only one's index, interpolator and time when one is left (with two slots,
// the other one), none when none is; the weights to normalize again and the sums invalid
re::Ptr32<NiInterpolator>* __fastcall Blend_RemoveInterpInfo(NiBlendInterpolator* self, re::Edx, re::Ptr32<NiInterpolator>* result,
                                                   int index) {
    uint8_t slot = static_cast<uint8_t>(index);
    InterpArrayItem& item = self->interpArray[slot];
    *result = item.interpolator;
    AddRefImport(item.interpolator);
    int8_t priority = item.priority;
    if (item.interpolator != nullptr) {
        ReleaseRef(item.interpolator);
        item.interpolator = nullptr;
    }
    item.priority = 0;
    item.weight = 0.0f;
    item.normalizedWeight = 0.0f;
    item.easeSpinner = 0.0f;
    item.updateTime = kInvalidFloat;
    if (priority == self->highPriority || priority == self->nextHighPriority) {
        self->nextHighPriority = -128;
        self->highPriority = -128;
        for (uint8_t i = 0; i < self->arraySize; i++) {
            InterpArrayItem& other = self->interpArray[i];
            if (other.interpolator != nullptr) {
                TakePriority(self, other.priority);
            }
        }
    }
    self->interpCount--;
    if (self->interpCount == 1) {
        if (self->arraySize == 2) {
            self->singleIdx = slot == 0 ? 1 : 0;
        } else {
            for (uint8_t i = 0; i < self->arraySize; i++) {
                if (self->interpArray[i].interpolator != nullptr) {
                    self->singleIdx = i;
                }
            }
        }
        self->singleInterpolator = self->interpArray[self->singleIdx].interpolator;
        self->singleTime = self->interpArray[self->singleIdx].updateTime;
    } else if (self->interpCount == 0) {
        self->singleIdx = 0xFF;
        self->singleInterpolator = nullptr;
        self->singleTime = kInvalidFloat;
    }
    self->flags |= 4;
    self->highSumOfWeights = kInvalidFloat;
    self->nextHighSumOfWeights = kInvalidFloat;
    self->highEaseSpinner = kInvalidFloat;
    return result;
}

// 0x826C10: ComputeNormalizedWeights (when they're to be normalized): 1 for the only one; the
// two-item case; else the sums of the high and next-high priorities' weights (made again when
// invalid) give each item its share (the high ones eased in by the highest ease spinner, the
// next ones by the rest, the others nothing); then those under the threshold dropped and the
// rest normalized again, or only the highest kept (none at index 0xFF: the game writes there)
void __fastcall Blend_ComputeNormalizedWeights(NiBlendInterpolator* self, re::Edx) {
    if ((self->flags & 4) == 0) {
        return;
    }
    self->flags &= 0xFB;
    if (self->interpCount == 1) {
        self->interpArray[self->singleIdx].normalizedWeight = 1.0f;
        return;
    }
    if (self->interpCount == 2) {
        BlendInterpolator_ComputeNormalizedWeightsFor2(self);
        return;
    }
    if (self->highSumOfWeights == kInvalidFloat) {
        self->highSumOfWeights = 0.0f;
        self->nextHighSumOfWeights = 0.0f;
        self->highEaseSpinner = 0.0f;
        for (uint8_t i = 0; i < self->arraySize; i++) {
            InterpArrayItem& item = self->interpArray[i];
            if (item.interpolator == nullptr) {
                continue;
            }
            float weight = item.weight * item.easeSpinner;
            if (item.priority == self->highPriority) {
                self->highSumOfWeights = self->highSumOfWeights + weight;
                if (self->highEaseSpinner < item.easeSpinner) {
                    self->highEaseSpinner = item.easeSpinner;
                }
            } else if (item.priority == self->nextHighPriority) {
                self->nextHighSumOfWeights = weight + self->nextHighSumOfWeights;
            }
        }
    }
    float oneMinusHighEase = 1.0f - self->highEaseSpinner;
    float total = self->nextHighSumOfWeights * oneMinusHighEase + self->highSumOfWeights * self->highEaseSpinner;
    float invTotal = total > 0.0f ? 1.0f / total : 0.0f;
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem& item = self->interpArray[i];
        if (item.interpolator == nullptr) {
            continue;
        }
        if (item.priority == self->highPriority) {
            item.normalizedWeight = item.weight * self->highEaseSpinner * item.easeSpinner * invTotal;
        } else if (item.priority == self->nextHighPriority) {
            item.normalizedWeight = item.weight * oneMinusHighEase * item.easeSpinner * invTotal;
        } else {
            item.normalizedWeight = 0.0f;
        }
    }
    if (self->weightThreshold > 0.0f) {
        float sum = 0.0f;
        for (uint8_t i = 0; i < self->arraySize; i++) {
            InterpArrayItem& item = self->interpArray[i];
            if (item.interpolator != nullptr && item.normalizedWeight != 0.0f) {
                if (item.normalizedWeight < self->weightThreshold) {
                    item.normalizedWeight = 0.0f;
                }
                sum = item.normalizedWeight + sum;
            }
        }
        if (sum != 1.0f) {
            float invSum = sum > 0.0f ? 1.0f / sum : 0.0f;
            // (every item, used or not)
            for (uint8_t i = 0; i < self->arraySize; i++) {
                InterpArrayItem& item = self->interpArray[i];
                if (item.normalizedWeight != 0.0f) {
                    item.normalizedWeight = item.normalizedWeight * invSum;
                }
            }
        }
    }
    if ((self->flags & 2) != 0) {
        float highest = -1.0f;
        uint8_t best = 0xFF;
        for (uint8_t i = 0; i < self->arraySize; i++) {
            InterpArrayItem& item = self->interpArray[i];
            if (highest < item.normalizedWeight) {
                highest = item.normalizedWeight;
                best = i;
            }
            item.normalizedWeight = 0.0f;
        }
        self->interpArray[best].normalizedWeight = 1.0f;
    }
}

// 0x826EC0: ProcessClone (slot 11): the items' interpolators', and the clone's only interpolator
// made this one's clone of it (itself when it has none; the clone found in the cloning's map, a
// null one written through all the same)
void __fastcall Blend_ProcessClone(NiBlendInterpolator* self, re::Edx, re::Ptr32<void>* cloning) {
    Object_ProcessClone(self, cloning);
    struct MapNode {   // (the game's layouts)
        re::Ptr32<MapNode> next;
        re::Ptr32<void> key;
        re::Ptr32<void> value;
    };
    struct CloneMap {
        re::Ptr32<void> vtable;
        uint32_t hashSize;
        re::Ptr32<re::Ptr32<MapNode>> buckets;
    };
    auto* map = static_cast<CloneMap*>(*cloning);
    NiBlendInterpolator* clone = nullptr;
    for (MapNode* node = map->buckets[reinterpret_cast<uintptr_t>(self) % map->hashSize]; node != nullptr;
         node = node->next) {
        if (node->key == self) {
            clone = static_cast<NiBlendInterpolator*>(node->value);
            break;
        }
    }
    for (uint8_t i = 0; i < self->arraySize; i++) {
        NiInterpolator* interpolator = self->interpArray[i].interpolator;
        if (interpolator != nullptr) {
            Virtual<void>(interpolator, 11, cloning);
        }
    }
    NiInterpolator* single = self->singleInterpolator;
    if (single == nullptr) {
        return;
    }
    map = static_cast<CloneMap*>(*cloning);
    for (MapNode* node = map->buckets[reinterpret_cast<uintptr_t>(single) % map->hashSize]; node != nullptr;
         node = node->next) {
        if (node->key == single) {
            clone->singleInterpolator = static_cast<NiInterpolator*>(node->value);
            return;
        }
    }
    clone->singleInterpolator = single;
}

// 0x827130: SaveBinary: the flags, the array size and the threshold; unless manager controlled
// the counts, priorities, cached sums, the items and the only interpolator's link
void __fastcall Blend_SaveBinary(NiBlendInterpolator* self, re::Edx, void* stream) {
    Interpolator_SaveBinary(self, stream);
    scene::SaveBytes(stream, &self->flags, 1);
    scene::SaveBytes(stream, &self->arraySize, 1);
    scene::SaveBytes(stream, &self->weightThreshold, 4);
    if ((self->flags & 1) != 0) {
        return;
    }
    scene::SaveBytes(stream, &self->interpCount, 1);
    scene::SaveBytes(stream, &self->singleIdx, 1);
    scene::SaveBytes(stream, &self->highPriority, 1);
    scene::SaveBytes(stream, &self->nextHighPriority, 1);
    scene::SaveBytes(stream, &self->singleTime, 4);
    scene::SaveBytes(stream, &self->highSumOfWeights, 4);
    scene::SaveBytes(stream, &self->nextHighSumOfWeights, 4);
    scene::SaveBytes(stream, &self->highEaseSpinner, 4);
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem_SaveBinary(&self->interpArray[i], stream);
    }
    Virtual<void>(stream, 12, self->singleInterpolator);   // SaveLinkID
}

// 0x827320: NiBlendInterpolator(manager controlled, weight threshold, array size)
NiBlendInterpolator* __fastcall Blend_Construct(NiBlendInterpolator* self, re::Edx, int managerControlled,
                                                float weightThreshold, int arraySize) {
    Interpolator_Construct(self);
    self->weightThreshold = weightThreshold;
    self->singleTime = kInvalidFloat;
    self->highSumOfWeights = kInvalidFloat;
    self->highPriority = -128;
    self->nextHighPriority = -128;
    self->nextHighSumOfWeights = kInvalidFloat;
    self->highEaseSpinner = kInvalidFloat;
    self->vtable = reinterpret_cast<void*>(NiBlendInterpolator_vtable);
    uint8_t size = static_cast<uint8_t>(arraySize);
    self->arraySize = size;
    self->interpCount = 0;
    self->singleIdx = 0xFF;
    self->interpArray = nullptr;
    self->singleInterpolator = nullptr;
    self->flags = re::Flag(managerControlled) ? 1 : 0;
    if (size != 0) {
        self->interpArray = NewItems(size);
    }
    return self;
}

// 0x8273E0: the deleting destructor
NiBlendInterpolator* __fastcall Blend_DeletingDestructor(NiBlendInterpolator* self, re::Edx, uint32_t flags) {
    BlendInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x30);
    }
    return self;
}

// 0x827400: ReallocateArrays (slot 40): the array grown by ms_ucArrayGrowBy (false when that's 0;
// the size wraps as a byte), the items copied over, the old array deleted
uint8_t __fastcall Blend_ReallocateArrays(NiBlendInterpolator* self, re::Edx) {
    uint8_t growBy = ms_ucArrayGrowBy;
    if (growBy == 0) {
        return 0;
    }
    uint8_t newSize = static_cast<uint8_t>(self->arraySize + growBy);
    InterpArrayItem* items = NewItems(newSize);
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem_Assign(&items[i], &self->interpArray[i]);
    }
    DeleteItems(self->interpArray);
    self->interpArray = items;
    self->arraySize = newSize;
    return 1;
}

// 0x827510: CopyMembers: the flags, size and threshold, a new array (the clone's old one, if any,
// overwritten), and unless manager controlled the items (their interpolators cloned), counts,
// the only one's index and time and the priorities (not the only interpolator: ProcessClone's)
void __fastcall Blend_CopyMembers(NiBlendInterpolator* self, re::Edx, NiBlendInterpolator* dest, void* cloning) {
    Interpolator_CopyMembers(self, dest, cloning);
    dest->flags = self->flags;
    dest->arraySize = self->arraySize;
    dest->interpArray = NewItems(self->arraySize);
    dest->weightThreshold = self->weightThreshold;
    if ((self->flags & 1) != 0) {
        return;
    }
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem& from = self->interpArray[i];
        InterpArrayItem& to = dest->interpArray[i];
        if (from.interpolator != nullptr) {
            auto* clone = Virtual<NiInterpolator*>(from.interpolator, 3, cloning);
            AssignReleasingFirst(to.interpolator, clone);
        }
        to.weight = from.weight;
        to.normalizedWeight = from.normalizedWeight;
        to.priority = from.priority;
        to.easeSpinner = from.easeSpinner;
        to.updateTime = from.updateTime;
    }
    dest->interpCount = self->interpCount;
    dest->singleIdx = self->singleIdx;
    dest->singleTime = self->singleTime;
    dest->highPriority = self->highPriority;
    dest->nextHighPriority = self->nextHighPriority;
}

// a priority from before 10.1.0.110 (4 bytes, INT_MIN meaning -128)
int8_t LoadOldPriority(void* stream) {
    int32_t priority;
    scene::LoadBytes(stream, &priority, 4);
    return priority == INT32_MIN ? int8_t(-128) : static_cast<int8_t>(priority);
}

// 0x827670: LoadBinary. From 10.1.0.112: the flags (not the to-normalize bit), the size, the
// array, the threshold and (unless manager controlled) the counts, priorities, sums, items and
// the only interpolator's link. Before: the size (2 bytes before 10.1.0.110, the grow-by after it
// read and dropped), the items, the two flags, the threshold, the counts (2 bytes each before
// 10.1.0.110), the only interpolator's link and time from 10.1.0.108, the priorities
void __fastcall Blend_LoadBinary(NiBlendInterpolator* self, re::Edx, void* stream) {
    Interpolator_LoadBinary(self, stream);
    if (scene::StreamVersion(stream) >= 0x0A010070) {
        scene::LoadBytes(stream, &self->flags, 1);
        self->flags &= 0xFB;
    }
    if (scene::StreamVersion(stream) < 0x0A01006E) {
        uint16_t size = 0;
        scene::LoadBytes(stream, &size, 2);
        self->arraySize = static_cast<uint8_t>(size);
        uint16_t growBy;
        scene::LoadBytes(stream, &growBy, 2);
    } else {
        scene::LoadBytes(stream, &self->arraySize, 1);
    }
    self->interpArray = NewItems(self->arraySize);
    if (scene::StreamVersion(stream) >= 0x0A010070) {
        scene::LoadBytes(stream, &self->weightThreshold, 4);
        if ((self->flags & 1) == 0) {
            scene::LoadBytes(stream, &self->interpCount, 1);
            scene::LoadBytes(stream, &self->singleIdx, 1);
            scene::LoadBytes(stream, &self->highPriority, 1);
            scene::LoadBytes(stream, &self->nextHighPriority, 1);
            scene::LoadBytes(stream, &self->singleTime, 4);
            scene::LoadBytes(stream, &self->highSumOfWeights, 4);
            scene::LoadBytes(stream, &self->nextHighSumOfWeights, 4);
            scene::LoadBytes(stream, &self->highEaseSpinner, 4);
            for (uint8_t i = 0; i < self->arraySize; i++) {
                InterpArrayItem_LoadBinary(&self->interpArray[i], stream);
            }
            self->singleInterpolator = static_cast<NiInterpolator*>(Stream_ReadLinkIDObject(stream));
        }
        return;
    }
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem_LoadBinary(&self->interpArray[i], stream);
    }
    uint8_t managerControlled;
    scene::LoadBytes(stream, &managerControlled, 1);
    if (managerControlled != 0) {
        self->flags |= 1;
    } else {
        self->flags &= 0xFE;
    }
    scene::LoadBytes(stream, &self->weightThreshold, 4);
    uint8_t onlyHighest;
    scene::LoadBytes(stream, &onlyHighest, 1);
    if (onlyHighest != 0) {
        self->flags |= 2;
    } else {
        self->flags &= 0xFD;
    }
    if (scene::StreamVersion(stream) < 0x0A01006E) {
        uint16_t count;
        scene::LoadBytes(stream, &count, 2);
        self->interpCount = static_cast<uint8_t>(count);
        uint16_t singleIdx;
        scene::LoadBytes(stream, &singleIdx, 2);
        self->singleIdx = static_cast<uint8_t>(singleIdx);
    } else {
        scene::LoadBytes(stream, &self->interpCount, 1);
        scene::LoadBytes(stream, &self->singleIdx, 1);
    }
    if (scene::StreamVersion(stream) >= 0x0A01006C) {
        self->singleInterpolator = static_cast<NiInterpolator*>(Stream_ReadLinkIDObject(stream));
        scene::LoadBytes(stream, &self->singleTime, 4);
    }
    if (scene::StreamVersion(stream) < 0x0A01006E) {
        self->highPriority = LoadOldPriority(stream);
        self->nextHighPriority = LoadOldPriority(stream);
        return;
    }
    scene::LoadBytes(stream, &self->highPriority, 1);
    scene::LoadBytes(stream, &self->nextHighPriority, 1);
}

// 0x827B90: GetViewerStrings
void __fastcall Blend_GetViewerStrings(NiBlendInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    Interpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBlendInterpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringByte("m_ucArraySize", self->arraySize));
    AddViewerString(strings, GetViewerStringByte("ms_ucArrayGrowBy", ms_ucArrayGrowBy));
    AddViewerString(strings, GetViewerStringBool("ManagerControlled", self->flags & 1));
    AddViewerString(strings, GetViewerStringFloat("m_fWeightThreshold", self->weightThreshold));
    AddViewerString(strings, GetViewerStringBool("m_bOnlyUseHighestWeight", (self->flags >> 1) & 1));
    AddViewerString(strings, GetViewerStringByte("m_ucInterpCount", self->interpCount));
    AddViewerString(strings, GetViewerStringByte("m_ucSingleIdx", self->singleIdx));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define C33_BLENDBASE_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_BLENDBASE_REVERSED("NiBlendInterpolator::NiBlendInterpolator()", 0x00826400,
                       &ni::anim::Blend_DefaultConstruct);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::GetRTTI", 0x00826460, &ni::anim::Blend_GetRTTI);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::AddInterpInfo", 0x00826470, &ni::anim::Blend_AddInterpInfo);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::InterpArrayItem::operator=", 0x00826580, &ni::anim::Item_Assign);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::ComputeNormalizedWeightsFor2", 0x008265F0,
                       &ni::anim::Blend_ComputeNormalizedWeightsFor2);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::Collapse", 0x008267E0, &ni::anim::Blend_Collapse);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::GuaranteeTimeRange", 0x00826830, &ni::anim::Blend_GuaranteeTimeRange);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::InterpArrayItem::IsEqual", 0x008268A0, &ni::anim::Item_IsEqual);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::RegisterStreamables", 0x00826910,
                       &ni::anim::Blend_RegisterStreamables);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::IsEqual", 0x00826960, &ni::anim::Blend_IsEqual);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::~NiBlendInterpolator", 0x00826A30, &ni::anim::Blend_Destruct);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::RemoveInterpInfo", 0x00826AB0, &ni::anim::Blend_RemoveInterpInfo);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::ComputeNormalizedWeights", 0x00826C10,
                       &ni::anim::Blend_ComputeNormalizedWeights);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::ProcessClone", 0x00826EC0, &ni::anim::Blend_ProcessClone);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::InterpArrayItem::LoadBinary", 0x00826F70, &ni::anim::Item_LoadBinary);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::InterpArrayItem::SaveBinary", 0x00827080, &ni::anim::Item_SaveBinary);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::SaveBinary", 0x00827130, &ni::anim::Blend_SaveBinary);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::InterpArrayItem::InterpArrayItem", 0x008272F0,
                       &ni::anim::Item_Construct);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::NiBlendInterpolator", 0x00827320, &ni::anim::Blend_Construct);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::DeletingDestructor", 0x008273E0, &ni::anim::Blend_DeletingDestructor);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::ReallocateArrays", 0x00827400, &ni::anim::Blend_ReallocateArrays);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::CopyMembers", 0x00827510, &ni::anim::Blend_CopyMembers);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::LoadBinary", 0x00827670, &ni::anim::Blend_LoadBinary);
C33_BLENDBASE_REVERSED("NiBlendInterpolator::GetViewerStrings", 0x00827B90, &ni::anim::Blend_GetViewerStrings);
