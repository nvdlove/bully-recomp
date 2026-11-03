// NiD3DRenderStateGroup (0x14 bytes, no vtable): a set of render states a pass or shader sets
// before drawing, in two lists of 0x10-byte entries (+0x0 the state, +0x4 the value, +0x8 next,
// +0xC previous; new ones at the head): +0x4/+0x8 the count and head of those set plainly,
// +0xC/+0x10 of those saved (set, and restored after the pass). +0x0 a byte: from the pool.
// Groups and entries come from two NiTObjectPools (0x20E73B8 groups, 8 at a time; 0x20E73BC
// entries, 16 at a time; doubling): +0x0 the free objects (an NiTObjectSet: array, allocated,
// count), +0xC the blocks (0xC each: the array, its count, the next block), +0x10 the next block's
// size, +0x14 the first's. The statics: 0x20E73B0 the renderer, 0x20E73B4 its render state
// (NiDX9RenderState: +0x120 the render states in force and saved, 8 bytes each; +0x2238 the
// device).
//
// Also NiDX9RenderState's material cache (+0xF8 of the render state: +0x0 the D3DMATERIAL9 in
// force, +0x4 whether it changed): each NiMaterialProperty's D3DMATERIAL9 (0x48 bytes: the
// material, then the property's revision it was made from) kept at the property's +0x58.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c41 {
using namespace ni::dx9;
namespace {

constexpr uint32_t kGroupSize = 0x14;
constexpr uint32_t kEntrySize = 0x10;
constexpr uintptr_t kEntryConstructor = 0x0089DBC0;
constexpr uintptr_t kGroupConstructor = 0x0089DBD0;
constexpr int kDevSetMaterial = 49;

GAME_VAR(uint8_t*, ms_pkGroupRenderer, 0x020E73B0);
GAME_VAR(uint8_t*, ms_pkGroupRenderState, 0x020E73B4);
GAME_VAR(uint8_t*, ms_pkGroupPool, 0x020E73B8);
GAME_VAR(uint8_t*, ms_pkEntryPool, 0x020E73BC);
GAME_VAR(uint8_t*, NiMaterialProperty_ms_pkDefault, 0x020E2D38);   // the default material property

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);   // NiMemObject::operator new[]
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count, uintptr_t ctor);
GAME_FN(void, __thiscall, Set_Realloc, 0x0083AD20, void* set, uint32_t allocated);
GAME_FN(void, __thiscall, Set_AddUnique, 0x00884380, void* set, const re::Ptr32<void>* element);

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, MaterialCache_Update, 0x0089DA20, void* material, void* property);
GAME_FN(void, __thiscall, MaterialCache_SetMaterial, 0x0089DB20, void* self, void* property);
GAME_FN(void, __thiscall, EntryBlock_Destruct, 0x0089DDF0, void* self);
GAME_FN(void, __thiscall, EntryBlock_DeletingDestructor, 0x0089DBA0, void* self, uint32_t flags);
GAME_FN(uint8_t*, __thiscall, Group_FindRenderState, 0x0089DDA0, void* self, uint32_t state, uint8_t* saved);
GAME_FN(void, __thiscall, Group_UnlinkSet, 0x0089DD20, void* self, uint8_t* entry);
GAME_FN(void, __thiscall, Group_UnlinkSaved, 0x0089DD60, void* self, uint8_t* entry);
GAME_FN(uint8_t*, __cdecl, Group_NewArray, 0x0089DE20, uint32_t count);
GAME_FN(void, __cdecl, Group_DeleteArray, 0x0089E520, uint8_t* array);
GAME_FN(uint8_t*, __thiscall, EntryBlock_Construct, 0x0089DEA0, void* self, uint32_t count);
GAME_FN(void, __thiscall, GroupPool_CreateNewObjects, 0x0089DF10, void* pool, uint32_t count);
GAME_FN(void, __thiscall, EntryPool_CreateNewObjects, 0x0089DFC0, void* pool, uint32_t count);
GAME_FN(void, __thiscall, Group_RemoveAll, 0x0089E0D0, void* self);
GAME_FN(void, __thiscall, GroupBlock_Destruct, 0x0089E560, void* self);

// D3DSAMPLERSTATETYPE / the device's SetRenderState
inline void DeviceSetRenderState(uint8_t* renderState, uint32_t state, uint32_t value) {
    Com<long>(At<void*>(renderState, 0x2238), kDevSetRenderState, state, value);
}

// an object put in a pool's free set unless it's there (NiTObjectSet::AddUnique, inlined)
inline void AddUnique(uint8_t* set, void* object) {
    uint32_t count = At<uint32_t>(set, 8);
    for (uint32_t i = 0; i < count; i++) {
        if (At<re::Ptr32<void>*>(set, 0)[i] == object) {
            return;
        }
    }
    uint32_t allocated = At<uint32_t>(set, 4);
    if (count == allocated) {
        Set_Realloc(set, allocated != 0 ? allocated * 2 : 1);
    }
    At<re::Ptr32<void>*>(set, 0)[At<uint32_t>(set, 8)] = object;
    At<uint32_t>(set, 8)++;
}

// an object put in a pool's free set (NiTObjectSet::Add, inlined)
inline void SetAdd(uint8_t* set, void* object) {
    uint32_t allocated = At<uint32_t>(set, 4);
    if (At<uint32_t>(set, 8) == allocated) {
        Set_Realloc(set, allocated != 0 ? allocated * 2 : 1);
    }
    At<re::Ptr32<void>*>(set, 0)[At<uint32_t>(set, 8)] = object;
    At<uint32_t>(set, 8)++;
}

// the first free object taken (the last moved into its place)
inline uint8_t* TakeFree(uint8_t* pool) {
    re::Ptr32<void>* array = At<re::Ptr32<void>*>(pool, 0);
    uint8_t* object = static_cast<uint8_t*>(array[0]);
    At<uint32_t>(pool, 8)--;
    array[0] = array[At<uint32_t>(pool, 8)];
    return object;
}

inline void PushHead(uint8_t* self, int countOffset, int headOffset, uint8_t* entry) {
    if (At<uint8_t*>(self, headOffset) != nullptr) {
        At<uint8_t*>(At<uint8_t*>(self, headOffset), 0xC) = entry;
        At<uint8_t*>(entry, 8) = At<uint8_t*>(self, headOffset);
    }
    At<uint8_t*>(entry, 0xC) = nullptr;
    At<uint32_t>(self, countOffset)++;
    At<uint8_t*>(self, headOffset) = entry;
}

} // namespace

// 0x89DA20 (thiscall on the D3DMATERIAL9): made again from the material property when its
// revision (+0x54) changed (diffuse, ambient, specular, emissive colours with the property's
// alpha, the power); whether it was. (ecx is left as it came: 0x89DB20 relies on it.)
uint8_t __fastcall MaterialCache_UpdateImpl(uint8_t* material, re::Edx, const uint8_t* property) {
    uint32_t revision = At<uint32_t>(property, 0x54);
    if (At<uint32_t>(material, 0x44) == revision) {
        return 0;
    }
    At<uint32_t>(material, 0x44) = revision;
    float alpha = At<float>(property, 0x50);
    At<float>(material, 0x30) = At<float>(property, 0x40);   // emissive
    At<float>(material, 0x34) = At<float>(property, 0x44);
    At<float>(material, 0x38) = At<float>(property, 0x48);
    At<float>(material, 0x3C) = alpha;
    At<float>(material, 0x10) = At<float>(property, 0x1C);   // ambient
    At<float>(material, 0x14) = At<float>(property, 0x20);
    At<float>(material, 0x18) = At<float>(property, 0x24);
    At<float>(material, 0x1C) = alpha;
    At<float>(material, 0x00) = At<float>(property, 0x28);   // diffuse
    At<float>(material, 0x04) = At<float>(property, 0x2C);
    At<float>(material, 0x08) = At<float>(property, 0x30);
    At<float>(material, 0x0C) = alpha;
    At<float>(material, 0x20) = At<float>(property, 0x34);   // specular
    At<float>(material, 0x24) = At<float>(property, 0x38);
    At<float>(material, 0x28) = At<float>(property, 0x3C);
    At<float>(material, 0x2C) = alpha;
    At<float>(material, 0x40) = At<float>(property, 0x4C);   // power
    return 1;
}

// 0x89DAA0 (thiscall): the material set on the device when it changed
void __fastcall MaterialCache_Apply(uint8_t* self, re::Edx, void* device) {
    if (At<uint8_t>(self, 4) != 0) {
        At<uint8_t>(self, 4) = 0;
        Com<long>(device, kDevSetMaterial, At<void*>(self, 0));
    }
}

// 0x89DAC0 (thiscall): a material property's D3DMATERIAL9 deleted (no longer the one in force)
void __fastcall MaterialCache_PurgeMaterial(uint8_t* self, re::Edx, uint8_t* property) {
    if (property == nullptr) {
        return;
    }
    void* material = At<void*>(property, 0x58);
    if (material == nullptr) {
        return;
    }
    At<void*>(property, 0x58) = nullptr;
    if (material == At<void*>(self, 0)) {
        At<void*>(self, 0) = nullptr;
    }
    NiDelete(material, 0x48);
}

// 0x89DAF0 (thiscall): the cache's destructor (the default material property's purged)
void __fastcall MaterialCache_Destruct(uint8_t* self, re::Edx) {
    uint8_t* property = NiMaterialProperty_ms_pkDefault;
    if (property == nullptr) {
        return;
    }
    void* material = At<void*>(property, 0x58);
    if (material == nullptr) {
        return;
    }
    At<void*>(property, 0x58) = nullptr;
    if (material == At<void*>(self, 0)) {
        At<void*>(self, 0) = nullptr;
    }
    NiDelete(material, 0x48);
}

// 0x89DB20 (thiscall): a material property's D3DMATERIAL9 made in force (made first when it has
// none; brought up to date; marked changed when it was or isn't the one in force)
void __fastcall MaterialCache_SetMaterialImpl(uint8_t* self, re::Edx, uint8_t* property) {
    uint8_t* material = At<uint8_t*>(property, 0x58);
    if (material == nullptr) {
        material = static_cast<uint8_t*>(NiNew(0x48));
        if (material != nullptr) {
            At<uint32_t>(material, 0x44) = 0;
            MaterialCache_Update(material, property);
        } else {
            material = nullptr;
        }
        At<uint8_t*>(property, 0x58) = material;
        At<uint8_t>(self, 4) = 1;
    }
    if (MaterialCache_Update(material, property) != 0) {
        At<uint8_t>(self, 4) = 1;
    }
    if (material != At<uint8_t*>(self, 0)) {
        At<uint8_t>(self, 4) = 1;
        At<uint8_t*>(self, 0) = material;
    }
}

// 0x89DB80 (thiscall): the cache reset to the default material property
void __fastcall MaterialCache_Reset(uint8_t* self, re::Edx) {
    At<void*>(self, 0) = nullptr;
    MaterialCache_SetMaterial(self, NiMaterialProperty_ms_pkDefault);
    At<uint8_t>(self, 4) = 1;
}

// 0x89DBA0 (thiscall): an entry block's deleting destructor
void* __fastcall EntryBlock_DeletingDestructorImpl(uint8_t* self, re::Edx, uint32_t flags) {
    EntryBlock_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0xC);
    }
    return self;
}

// 0x89DBC0 (thiscall): an entry's constructor
void __fastcall Entry_Construct(uint32_t* self, re::Edx) {
    self[0] = 0;
    self[1] = 0;
    self[2] = 0;
    self[3] = 0;
}

// 0x89DBD0 (thiscall): a group's constructor
void __fastcall Group_Construct(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 0) = 0;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
}

// 0x89DBF0: NiD3DRenderStateGroup::SetD3DRenderer (the renderer and its render state)
void __cdecl Group_SetD3DRenderer(uint8_t* renderer) {
    ms_pkGroupRenderer = renderer;
    if (renderer != nullptr) {
        ms_pkGroupRenderState = At<uint8_t*>(renderer, kRenderState);
        return;
    }
    ms_pkGroupRenderState = nullptr;
}

// 0x89DC20 (thiscall): NiD3DRenderStateGroup::SetRenderStates (each state set where it differs;
// the saved ones' values in force kept as their saved ones first)
uint32_t __fastcall Group_SetRenderStates(uint8_t* self, re::Edx) {
    uint8_t* renderState = ms_pkGroupRenderState;
    for (uint8_t* entry = At<uint8_t*>(self, 8); entry != nullptr; entry = At<uint8_t*>(entry, 8)) {
        uint32_t state = At<uint32_t>(entry, 0);
        uint32_t value = At<uint32_t>(entry, 4);
        if (At<uint32_t>(renderState, 0x120 + int(state) * 8) != value) {
            DeviceSetRenderState(renderState, state, value);
            At<uint32_t>(renderState, 0x120 + int(state) * 8) = value;
            renderState = ms_pkGroupRenderState;
        }
    }
    for (uint8_t* entry = At<uint8_t*>(self, 0x10); entry != nullptr; entry = At<uint8_t*>(entry, 8)) {
        uint32_t state = At<uint32_t>(entry, 0);
        uint32_t value = At<uint32_t>(entry, 4);
        uint32_t* cache = &At<uint32_t>(renderState, 0x120 + int(state) * 8);
        if (cache[0] != value) {
            DeviceSetRenderState(renderState, state, value);
        }
        cache[1] = cache[0];
        cache[0] = value;
        renderState = ms_pkGroupRenderState;
    }
    return 0;
}

// 0x89DCC0 (thiscall): NiD3DRenderStateGroup::RestoreRenderStates (the saved ones put back)
uint32_t __fastcall Group_RestoreRenderStates(uint8_t* self, re::Edx) {
    uint8_t* renderState = ms_pkGroupRenderState;
    for (uint8_t* entry = At<uint8_t*>(self, 0x10); entry != nullptr; entry = At<uint8_t*>(entry, 8)) {
        uint32_t state = At<uint32_t>(entry, 0);
        uint32_t saved = At<uint32_t>(renderState, 0x124 + int(state) * 8);
        if (At<uint32_t>(renderState, 0x120 + int(state) * 8) != saved) {
            DeviceSetRenderState(renderState, state, saved);
            At<uint32_t>(renderState, 0x120 + int(state) * 8) = saved;
            renderState = ms_pkGroupRenderState;
        }
    }
    return 0;
}

// 0x89DD20 (thiscall): an entry taken out of the plain list
void __fastcall Group_UnlinkSetImpl(uint8_t* self, re::Edx, uint8_t* entry) {
    uint8_t* next = At<uint8_t*>(entry, 8);
    uint8_t* previous = At<uint8_t*>(entry, 0xC);
    if (next != nullptr) {
        At<uint8_t*>(next, 0xC) = previous;
    }
    if (previous != nullptr) {
        At<uint8_t*>(previous, 8) = next;
    }
    if (entry == At<uint8_t*>(self, 8)) {
        At<uint8_t*>(self, 8) = next;
    }
    At<uint32_t>(self, 4)--;
    At<uint8_t*>(entry, 8) = nullptr;
    At<uint8_t*>(entry, 0xC) = nullptr;
}

// 0x89DD60 (thiscall): an entry taken out of the saved list
void __fastcall Group_UnlinkSavedImpl(uint8_t* self, re::Edx, uint8_t* entry) {
    uint8_t* next = At<uint8_t*>(entry, 8);
    uint8_t* previous = At<uint8_t*>(entry, 0xC);
    if (next != nullptr) {
        At<uint8_t*>(next, 0xC) = previous;
    }
    if (previous != nullptr) {
        At<uint8_t*>(previous, 8) = next;
    }
    if (entry == At<uint8_t*>(self, 0x10)) {
        At<uint8_t*>(self, 0x10) = next;
    }
    At<uint32_t>(self, 0xC)--;
    At<uint8_t*>(entry, 8) = nullptr;
    At<uint8_t*>(entry, 0xC) = nullptr;
}

// 0x89DDA0 (thiscall): a state's entry (the plain list first) and whether it's saved (only
// written when found)
uint8_t* __fastcall Group_FindRenderStateImpl(uint8_t* self, re::Edx, uint32_t state, uint8_t* saved) {
    for (uint8_t* entry = At<uint8_t*>(self, 8); entry != nullptr; entry = At<uint8_t*>(entry, 8)) {
        if (At<uint32_t>(entry, 0) == state) {
            *saved = 0;
            return entry;
        }
    }
    for (uint8_t* entry = At<uint8_t*>(self, 0x10); entry != nullptr; entry = At<uint8_t*>(entry, 8)) {
        if (At<uint32_t>(entry, 0) == state) {
            *saved = 1;
            return entry;
        }
    }
    return nullptr;
}

// 0x89DDF0 (thiscall): an entry block's destructor (its array and the blocks after it deleted)
void __fastcall EntryBlock_DestructImpl(uint8_t* self, re::Edx) {
    NiDeleteArray(At<void*>(self, 0), kEntrySize);
    uint8_t* next = At<uint8_t*>(self, 8);
    if (next != nullptr) {
        EntryBlock_Destruct(next);
        NiDelete(next, 0xC);
    }
}

// the byte count of `count` objects of `size` and NiNew[]'s count before them (0xFFFFFFFF on
// overflow, as the compiler's checked multiply)
inline uint32_t CheckedSize(uint32_t count, uint32_t size, uint32_t extra) {
    uint64_t bytes = uint64_t(count) * size;
    uint32_t total = (bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes);
    if (extra != 0) {
        total = total > 0xFFFFFFFFu - extra ? 0xFFFFFFFFu : total + extra;
    }
    return total;
}

// 0x89DE20: an array of `count` groups (NiNew[] with its count before it)
uint8_t* __cdecl Group_NewArrayImpl(uint32_t count) {
    uint32_t* memory = static_cast<uint32_t*>(NiNewArray(CheckedSize(count, kGroupSize, 4)));
    if (memory == nullptr) {
        return nullptr;
    }
    *memory = count;
    VectorConstructor(memory + 1, kGroupSize, count, kGroupConstructor);
    return reinterpret_cast<uint8_t*>(memory + 1);
}

// 0x89DE70 (thiscall): NiD3DRenderStateGroup::GetRenderState (its value and whether it's saved;
// false when the group hasn't it)
uint8_t __fastcall Group_GetRenderState(uint8_t* self, re::Edx, uint32_t state, uint32_t* value, uint8_t* saved) {
    uint8_t* entry = Group_FindRenderState(self, state, saved);
    if (entry == nullptr) {
        return 0;
    }
    *value = At<uint32_t>(entry, 4);
    return 1;
}

// 0x89DEA0 (thiscall): an entry block made (`count` entries in an NiNew[] array)
uint8_t* __fastcall EntryBlock_ConstructImpl(uint8_t* self, re::Edx, uint32_t count) {
    At<uint32_t>(self, 4) = count;
    At<void*>(self, 8) = nullptr;
    if (count == 0) {
        At<void*>(self, 0) = nullptr;
        return self;
    }
    void* array = NiNewArray(CheckedSize(count, kEntrySize, 0));
    if (array != nullptr) {
        VectorConstructor(array, kEntrySize, count, kEntryConstructor);
        At<void*>(self, 0) = array;
        return self;
    }
    At<void*>(self, 0) = nullptr;
    return self;
}

// 0x89DF10 (thiscall): the group pool's next block of `count` made, its groups made free
void __fastcall GroupPool_CreateNewObjectsImpl(uint8_t* pool, re::Edx, uint32_t count) {
    uint8_t* block = static_cast<uint8_t*>(NiNew(0xC));
    if (block != nullptr) {
        At<uint32_t>(block, 4) = count;
        At<void*>(block, 8) = nullptr;
        At<uint8_t*>(block, 0) = count == 0 ? nullptr : Group_NewArray(count);
    }
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* group = i < At<uint32_t>(block, 4) ? At<uint8_t*>(block, 0) + i * kGroupSize : nullptr;
        SetAdd(pool, group);
    }
    At<uint8_t*>(block, 8) = At<uint8_t*>(pool, 0xC);
    At<uint8_t*>(pool, 0xC) = block;
}

// 0x89DFC0 (thiscall): the entry pool's next block of `count` made, its entries made free
void __fastcall EntryPool_CreateNewObjectsImpl(uint8_t* pool, re::Edx, uint32_t count) {
    void* memory = NiNew(0xC);
    uint8_t* block = memory != nullptr ? EntryBlock_Construct(memory, count) : nullptr;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* entry = i < At<uint32_t>(block, 4) ? At<uint8_t*>(block, 0) + i * kEntrySize : nullptr;
        SetAdd(pool, entry);
    }
    At<uint8_t*>(block, 8) = At<uint8_t*>(pool, 0xC);
    At<uint8_t*>(pool, 0xC) = block;
}

// a pool made (0x18 bytes: the free set empty, no blocks, the block size)
inline uint8_t* NewPool(uint32_t blockSize) {
    uint8_t* pool = static_cast<uint8_t*>(NiNew(0x18));
    if (pool == nullptr) {
        return nullptr;
    }
    At<void*>(pool, 0) = nullptr;
    At<uint32_t>(pool, 4) = 0;
    At<uint32_t>(pool, 8) = 0;
    At<void*>(pool, 0xC) = nullptr;
    At<uint32_t>(pool, 0x10) = blockSize;
    At<uint32_t>(pool, 0x14) = blockSize;
    return pool;
}

// 0x89E060: NiD3DRenderStateGroup::CreatePools (groups 8 at a time, entries 16)
void __cdecl Group_CreatePools() {
    ms_pkGroupPool = NewPool(8);
    ms_pkEntryPool = NewPool(0x10);
}

// 0x89E0D0 (thiscall): every entry given back to the entry pool, the lists emptied (their counts
// left as they were)
void __fastcall Group_RemoveAllImpl(uint8_t* self, re::Edx) {
    for (uint8_t* entry = At<uint8_t*>(self, 8); entry != nullptr;) {
        uint8_t* next = At<uint8_t*>(entry, 8);
        AddUnique(ms_pkEntryPool, entry);
        entry = next;
    }
    for (uint8_t* entry = At<uint8_t*>(self, 0x10); entry != nullptr;) {
        uint8_t* next = At<uint8_t*>(entry, 8);
        AddUnique(ms_pkEntryPool, entry);
        entry = next;
    }
    At<uint8_t*>(self, 8) = nullptr;
    At<uint8_t*>(self, 0x10) = nullptr;
}

// 0x89E1F0 (thiscall): NiD3DRenderStateGroup::SetRenderState (a new entry from the pool at the
// head of its list; an existing one given the value and moved when it changes list)
void __fastcall Group_SetRenderState(uint8_t* self, re::Edx, uint32_t state, uint32_t value, int save) {
    uint8_t saved = 0;
    uint8_t* entry = Group_FindRenderState(self, state, &saved);
    if (entry == nullptr) {
        uint8_t* pool = ms_pkEntryPool;
        if (At<uint32_t>(pool, 8) == 0) {
            EntryPool_CreateNewObjects(pool, At<uint32_t>(pool, 0x10));
            At<uint32_t>(pool, 0x10) *= 2;
        }
        entry = TakeFree(pool);
        At<uint8_t*>(entry, 8) = nullptr;
        At<uint8_t*>(entry, 0xC) = nullptr;
        At<uint32_t>(entry, 0) = state;
        At<uint32_t>(entry, 4) = value;
        if (uint8_t(save) != 0) {
            PushHead(self, 0xC, 0x10, entry);
        } else {
            PushHead(self, 4, 8, entry);
        }
        return;
    }
    At<uint32_t>(entry, 4) = value;
    if (saved == uint8_t(save)) {
        return;
    }
    if (saved != 0) {
        Group_UnlinkSaved(self, entry);
        PushHead(self, 4, 8, entry);
    } else {
        Group_UnlinkSet(self, entry);
        PushHead(self, 0xC, 0x10, entry);
    }
}

// 0x89E310 (thiscall): NiD3DRenderStateGroup::RemoveRenderState (the entry unlinked, given back)
void __fastcall Group_RemoveRenderState(uint8_t* self, re::Edx, uint32_t state) {
    uint8_t saved = 0;
    uint8_t* entry = Group_FindRenderState(self, state, &saved);
    if (entry == nullptr) {
        return;
    }
    uint8_t* next = At<uint8_t*>(entry, 8);
    uint8_t* previous = At<uint8_t*>(entry, 0xC);
    if (next != nullptr) {
        At<uint8_t*>(next, 0xC) = previous;
    }
    if (previous != nullptr) {
        At<uint8_t*>(previous, 8) = next;
    }
    if (saved != 0) {
        if (entry == At<uint8_t*>(self, 0x10)) {
            At<uint8_t*>(self, 0x10) = next;
        }
        At<uint32_t>(self, 0xC)--;
    } else {
        if (entry == At<uint8_t*>(self, 8)) {
            At<uint8_t*>(self, 8) = next;
        }
        At<uint32_t>(self, 4)--;
    }
    At<uint8_t*>(entry, 0xC) = nullptr;
    At<uint8_t*>(entry, 8) = nullptr;
    Set_AddUnique(ms_pkEntryPool, re::PtrCast<const re::Ptr32<void>*>(&entry));
}

// 0x89E3A0: NiD3DRenderStateGroup::GetFreeRenderStateGroup (from the pool, made more when it's
// empty; marked as pooled)
uint8_t* __cdecl Group_GetFree() {
    uint8_t* pool = ms_pkGroupPool;
    if (At<uint32_t>(pool, 8) == 0) {
        GroupPool_CreateNewObjects(pool, At<uint32_t>(pool, 0x10));
        At<uint32_t>(pool, 0x10) *= 2;
    }
    uint8_t* group = TakeFree(pool);
    if (At<uint8_t>(group, 0) == 0) {
        At<uint8_t>(group, 0) = 1;
    }
    return group;
}

// 0x89E3E0: NiD3DRenderStateGroup::ReleasePools (every block deleted with its objects, the pools
// deleted; the pointers left as they were)
void __cdecl Group_DestroyPools() {
    uint8_t* pool = ms_pkGroupPool;
    if (pool != nullptr) {
        uint8_t* block = At<uint8_t*>(pool, 0xC);
        At<uint32_t>(pool, 8) = 0;
        if (block != nullptr) {
            GroupBlock_Destruct(block);
            NiDelete(block, 0xC);
        }
        NiFree(At<void*>(pool, 0));
        NiDelete(pool, 0x18);
    }
    pool = ms_pkEntryPool;
    if (pool != nullptr) {
        uint8_t* block = At<uint8_t*>(pool, 0xC);
        At<uint32_t>(pool, 8) = 0;
        if (block != nullptr) {
            NiDeleteArray(At<void*>(block, 0), kEntrySize);
            if (At<void*>(block, 8) != nullptr) {
                EntryBlock_DeletingDestructor(At<void*>(block, 8), 1);
            }
            NiDelete(block, 0xC);
        }
        NiFree(At<void*>(pool, 0));
        NiDelete(pool, 0x18);
    }
}

// 0x89E480: NiD3DRenderStateGroup::ReleaseRenderStateGroup (a pooled group's entries and itself
// given back to the pools; another one emptied and deleted)
void __cdecl Group_Release(uint8_t* group) {
    if (group == nullptr) {
        return;
    }
    if (At<uint8_t>(group, 0) != 0) {
        for (uint8_t* entry = At<uint8_t*>(group, 8); entry != nullptr;) {
            uint8_t* next = At<uint8_t*>(entry, 8);
            Set_AddUnique(ms_pkEntryPool, re::PtrCast<const re::Ptr32<void>*>(&entry));
            entry = next;
        }
        At<uint8_t*>(group, 8) = nullptr;
        At<uint32_t>(group, 4) = 0;
        for (uint8_t* entry = At<uint8_t*>(group, 0x10); entry != nullptr;) {
            uint8_t* next = At<uint8_t*>(entry, 8);
            Set_AddUnique(ms_pkEntryPool, re::PtrCast<const re::Ptr32<void>*>(&entry));
            entry = next;
        }
        At<uint8_t*>(group, 0x10) = nullptr;
        At<uint32_t>(group, 0xC) = 0;
        Set_AddUnique(ms_pkGroupPool, re::PtrCast<const re::Ptr32<void>*>(&group));
        return;
    }
    Group_RemoveAll(group);
    NiDelete(group, kGroupSize);
}

// 0x89E520: an array of groups deleted (each emptied, last first)
void __cdecl Group_DeleteArrayImpl(uint8_t* array) {
    if (array == nullptr) {
        return;
    }
    int32_t count = At<int32_t>(array, -4);
    for (int32_t i = count - 1; i >= 0; i--) {
        Group_RemoveAll(array + i * kGroupSize);
    }
    NiDeleteArray(array - 4, kGroupSize);
}

// 0x89E560 (thiscall): a group block's destructor (its groups and the blocks after it deleted)
void __fastcall GroupBlock_DestructImpl(uint8_t* self, re::Edx) {
    Group_DeleteArray(At<uint8_t*>(self, 0));
    uint8_t* next = At<uint8_t*>(self, 8);
    if (next != nullptr) {
        GroupBlock_Destruct(next);
        NiDelete(next, 0xC);
    }
}

#define C41D_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

// (0x89DA20 leaves ecx as it came and 0x89DB20 relies on it: ours doesn't keep it, so turn the
// two on together)
C41D_REVERSED("NiDX9RenderState::MaterialCache::Update", 0x0089DA20, MaterialCache_UpdateImpl);
C41D_REVERSED("NiDX9RenderState::MaterialCache::Apply", 0x0089DAA0, MaterialCache_Apply);
C41D_REVERSED("NiDX9RenderState::MaterialCache::PurgeMaterial", 0x0089DAC0, MaterialCache_PurgeMaterial);
C41D_REVERSED("NiDX9RenderState::MaterialCache::~MaterialCache", 0x0089DAF0, MaterialCache_Destruct);
C41D_REVERSED("NiDX9RenderState::MaterialCache::SetMaterial", 0x0089DB20, MaterialCache_SetMaterialImpl);
C41D_REVERSED("NiDX9RenderState::MaterialCache::Reset", 0x0089DB80, MaterialCache_Reset);
C41D_REVERSED("NiD3DRenderStateGroup::EntryBlock::DeletingDestructor", 0x0089DBA0, EntryBlock_DeletingDestructorImpl);
C41D_REVERSED("NiD3DRenderStateGroup::Entry::Entry", 0x0089DBC0, Entry_Construct);
C41D_REVERSED("NiD3DRenderStateGroup::NiD3DRenderStateGroup", 0x0089DBD0, Group_Construct);
C41D_REVERSED("NiD3DRenderStateGroup::SetD3DRenderer", 0x0089DBF0, Group_SetD3DRenderer);
C41D_REVERSED("NiD3DRenderStateGroup::SetRenderStates", 0x0089DC20, Group_SetRenderStates);
C41D_REVERSED("NiD3DRenderStateGroup::RestoreRenderStates", 0x0089DCC0, Group_RestoreRenderStates);
C41D_REVERSED("NiD3DRenderStateGroup::UnlinkSet", 0x0089DD20, Group_UnlinkSetImpl);
C41D_REVERSED("NiD3DRenderStateGroup::UnlinkSaved", 0x0089DD60, Group_UnlinkSavedImpl);
C41D_REVERSED("NiD3DRenderStateGroup::FindRenderState", 0x0089DDA0, Group_FindRenderStateImpl);
C41D_REVERSED("NiD3DRenderStateGroup::EntryBlock::~EntryBlock", 0x0089DDF0, EntryBlock_DestructImpl);
C41D_REVERSED("NiD3DRenderStateGroup::NewArray", 0x0089DE20, Group_NewArrayImpl);
C41D_REVERSED("NiD3DRenderStateGroup::GetRenderState", 0x0089DE70, Group_GetRenderState);
C41D_REVERSED("NiD3DRenderStateGroup::EntryBlock::EntryBlock", 0x0089DEA0, EntryBlock_ConstructImpl);
C41D_REVERSED("NiD3DRenderStateGroup::GroupPool::CreateNewObjects", 0x0089DF10, GroupPool_CreateNewObjectsImpl);
C41D_REVERSED("NiD3DRenderStateGroup::EntryPool::CreateNewObjects", 0x0089DFC0, EntryPool_CreateNewObjectsImpl);
C41D_REVERSED("NiD3DRenderStateGroup::CreatePools", 0x0089E060, Group_CreatePools);
C41D_REVERSED("NiD3DRenderStateGroup::RemoveAllRenderStates", 0x0089E0D0, Group_RemoveAllImpl);
C41D_REVERSED("NiD3DRenderStateGroup::SetRenderState", 0x0089E1F0, Group_SetRenderState);
C41D_REVERSED("NiD3DRenderStateGroup::RemoveRenderState", 0x0089E310, Group_RemoveRenderState);
C41D_REVERSED("NiD3DRenderStateGroup::GetFreeRenderStateGroup", 0x0089E3A0, Group_GetFree);
C41D_REVERSED("NiD3DRenderStateGroup::ReleasePools", 0x0089E3E0, Group_DestroyPools);
C41D_REVERSED("NiD3DRenderStateGroup::ReleaseRenderStateGroup", 0x0089E480, Group_Release);
C41D_REVERSED("NiD3DRenderStateGroup::DeleteArray", 0x0089E520, Group_DeleteArrayImpl);
C41D_REVERSED("NiD3DRenderStateGroup::GroupBlock::~GroupBlock", 0x0089E560, GroupBlock_DestructImpl);

} // namespace ni::c41
