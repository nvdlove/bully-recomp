// NiD3DXEffectFile (0x54 bytes, src/gamebryo/NiD3DXEffect.h): one D3DX effect, parsed into technique
// sets (by shader name) and parameters; the handles and the state manager linked once the effect
// is made on the device. Also the string maps it and the factory keep (NiTStringPointerMap of
// NiPointers or plain pointers: one copy of each kind's functions), and NiTPointerList::AddTail.

#include "gamebryo/NiD3DXEffect.h"

#include <cstring>
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::d3dx;

constexpr uintptr_t IID_ID3DXEffect = 0x009A2104;
constexpr uintptr_t TechniqueSetMap_vtable = 0x0095072C;     // NiTStringPointerMap<NiD3DXEffectTechniqueSetPtr>
constexpr uintptr_t TechniqueSetMapBase_vtable = 0x00950718;
constexpr uintptr_t MapBase_vtable = 0x0099ACD4;
constexpr uintptr_t PointerMap_vtable = 0x009507B0;          // NiTStringPointerMap<T*> (the library's)
constexpr uintptr_t PointerMapMiddle_vtable = 0x0098AD94;
constexpr uintptr_t PointerMapBase_vtable = 0x00950760;
constexpr uintptr_t PointerList_vtable = 0x00952E10;
constexpr uintptr_t PointerListBase_vtable = 0x0099C748;

GAME_FN(void, __cdecl, NiDelete_, 0x00752540, void* p, uint32_t size);
GAME_FN(void*, __cdecl, NiNew_, 0x00752460, uint32_t size);
GAME_FN(void*, __cdecl, NiMalloc_, 0x00752300, uint32_t size);
GAME_FN(void, __cdecl, NiFree_, 0x00752380, void* p);
GAME_FN(void, __cdecl, NiD3DUtility_SetString, 0x00888530, re::Ptr32<char>* to, uintptr_t size, const char* from);
GAME_FN(void, __cdecl, NiD3DRenderer_Error, 0x00888620, const char* format, ...);
GAME_FN(uint32_t, __cdecl, NiTStringMap_Hash, 0x005F09F0, const char* key, uint32_t hashSize);
GAME_FN(void, __thiscall, NiTMap_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, PointerList* list);
GAME_FN(void*, __thiscall, NiTPointerList_RemovePos, 0x00771DA0, PointerList* list, re::Ptr32<ListNode>* position);
GAME_FN(MapItem*, __thiscall, Map_GetFirstPos, 0x008CA4E0, StringMap* map);
GAME_FN(uint8_t, __thiscall, TechniqueSetMap_GetAt, 0x00845270, StringMap* map, const char* key,
        re::Ptr32<NiD3DXEffectTechniqueSet>* value);
// this library's, called where modes apply
GAME_FN(NiD3DXEffectFactory*, __cdecl, Factory_GetInstance, 0x00748CB0, int create);
GAME_FN(void, __cdecl, Factory_SetD3DRenderer, 0x00748980, void* renderer);
GAME_FN(void, __cdecl, Factory_LoadEffect, 0x00748FF0, const char* name);
GAME_FN(void, __thiscall, TechniqueSetMap_SetAt, 0x00747380, StringMap* map, const char* key,
        NiD3DXEffectTechniqueSet* value);
GAME_FN(void, __thiscall, TechniqueSetMap_GetNext, 0x007474A0, StringMap* map, re::Ptr32<MapItem>* position, re::Ptr32<char>* key,
        re::Ptr32<NiD3DXEffectTechniqueSet>* value);
GAME_FN(void, __thiscall, TechniqueSetMap_Destruct, 0x00747210, StringMap* map);
GAME_FN(void, __thiscall, PointerMap_Destruct, 0x00747FC0, StringMap* map);
GAME_FN(NiD3DXEffectTechnique*, __thiscall, File_GetNextTechnique, 0x00747550, NiD3DXEffectFile* self);
GAME_FN(void*, __thiscall, File_GetNextShaderDesc, 0x00747640, NiD3DXEffectFile* self);
GAME_FN(void, __thiscall, File_DestroyRendererData, 0x007476B0, NiD3DXEffectFile* self);
GAME_FN(void, __thiscall, File_AddTechnique, 0x00747790, NiD3DXEffectFile* self, NiD3DXEffectTechnique* technique,
        void* device);
GAME_FN(uint8_t, __thiscall, File_LinkHandles, 0x007478D0, NiD3DXEffectFile* self);
GAME_FN(uint8_t, __thiscall, File_ParseEffect, 0x00747AA0, NiD3DXEffectFile* self, void* device);
GAME_FN(void, __thiscall, PointerList_AddTail, 0x00748170, PointerList* list, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, TechniqueSet_ReleaseShaders, 0x0074DCA0, NiD3DXEffectTechniqueSet* self);
GAME_FN(void, __thiscall, TechniqueSet_SetName, 0x0074D9B0, NiD3DXEffectTechniqueSet* self, const char* name);
GAME_FN(void, __thiscall, TechniqueSet_AddTechnique, 0x0074DE00, NiD3DXEffectTechniqueSet* self,
        NiD3DXEffectTechnique* technique);
GAME_FN(void*, __thiscall, TechniqueSet_GetShaderDesc, 0x0074D9D0, NiD3DXEffectTechniqueSet* self,
        NiD3DXEffectAttributeTable* table);
GAME_FN(NiD3DXEffectTechniqueSet*, __thiscall, TechniqueSet_Construct, 0x0074DD90, void* self);
GAME_FN(NiD3DXEffectTechnique*, __thiscall, Technique_Construct, 0x0074A360, void* self);
GAME_FN(void, __thiscall, Technique_Initialize, 0x00749BD0, NiD3DXEffectTechnique* self, TechniqueDesc* desc);
GAME_FN(void, __thiscall, Technique_AddParameter, 0x0074A2D0, NiD3DXEffectTechnique* self,
        NiD3DXEffectParameter* parameter);
GAME_FN(void, __thiscall, Technique_GetPassDescs, 0x00749D80, NiD3DXEffectTechnique* self, void* effect);
GAME_FN(NiD3DXEffectParameter*, __thiscall, Parameter_Construct, 0x0074A810, void* self);
GAME_FN(void, __thiscall, Parameter_Destruct, 0x0074A850, NiD3DXEffectParameter* self);
GAME_FN(uint8_t, __thiscall, Parameter_Initialize, 0x0074A8D0, NiD3DXEffectParameter* self, ParameterDesc* desc,
        NiD3DXEffectAttributeTable* table);
GAME_FN(void, __thiscall, Parameter_ReleaseTexture, 0x0074A3C0, NiD3DXEffectParameter* self);
GAME_FN(void, __thiscall, Parameter_LinkHandle, 0x0074A460, NiD3DXEffectParameter* self, void* effect);
GAME_FN(void, __thiscall, AttributeTable_Construct, 0x00751980, NiD3DXEffectAttributeTable* self);
GAME_FN(void, __thiscall, AttributeTable_Destruct, 0x007510F0, NiD3DXEffectAttributeTable* self);
// the parser's
GAME_FN(uint8_t, __cdecl, Parser_GetEffectDesc, 0x0074DE70, void* effect, uint32_t* parameters, uint32_t* techniques);
GAME_FN(void, __cdecl, Parser_ResetCounts, 0x0074DF20);
GAME_FN(ParameterDesc*, __cdecl, Parser_GetParameter, 0x00751070, void* effect, uint32_t index);
GAME_FN(TechniqueDesc*, __cdecl, Parser_GetTechnique, 0x0074FBC0, void* effect, uint32_t index);
GAME_FN(uint8_t, __cdecl, Parser_IsParameterUsed, 0x0074DEB0, void* effect, const char* parameter,
        const char* technique);

// the first item of a map (GetFirstPos, inlined)
inline MapItem* FirstItem(const StringMap* map) {
    for (uint32_t i = 0; i < map->hashSize; i++) {
        if (map->table[i] != nullptr) {
            return map->table[i];
        }
    }
    return nullptr;
}

// a string map's copied keys freed (the most derived destructor's part)
inline void FreeCopiedKeys(StringMap* self) {
    if (self->copyKeys) {
        for (uint32_t i = 0; i < self->hashSize; i++) {
            for (MapItem* item = self->table[i]; item != nullptr;) {
                MapItem* next = item->next;
                NiFree_(item->key);
                item = next;
            }
        }
    }
}

// the empty map the library builds: 37 buckets, keys copied
inline void ConstructTechniqueSetMap(StringMap* map) {
    map->vtable = reinterpret_cast<void*>(MapBase_vtable);
    map->hashSize = 0x25;
    map->count = 0;
    map->table = static_cast<re::Ptr32<MapItem>*>(NiMalloc_(0x94));
    crt::Memset(map->table, 0, map->hashSize * 4);
    map->copyKeys = 1;
    map->vtable = reinterpret_cast<void*>(TechniqueSetMap_vtable);
}

} // namespace

// ---- NiD3DXEffectFile

// 0x7471B0: SetName (a copy)
void __fastcall NiD3DXEffectFile_SetName(NiD3DXEffectFile* self, re::Edx, const char* name) {
    NiD3DUtility_SetString(&self->name, 0, name);
}

// 0x7471D0 / 0x7471F0: HandleLostDevice / HandleResetDevice: the effect's OnLostDevice /
// OnResetDevice, once it's on the device
void __fastcall NiD3DXEffectFile_HandleLostDevice(NiD3DXEffectFile* self, re::Edx) {
    if (self->effect != nullptr && self->rendererDataCreated) {
        Com<long>(self->effect, kOnLostDevice);
    }
}
void __fastcall NiD3DXEffectFile_HandleResetDevice(NiD3DXEffectFile* self, re::Edx) {
    if (self->effect != nullptr && self->rendererDataCreated) {
        Com<long>(self->effect, kOnResetDevice);
    }
}

// ---- NiTStringPointerMap<NiD3DXEffectTechniqueSetPtr> (values are references)

// 0x747210: the destructor
void __fastcall TechniqueSetMap_DestructImpl(StringMap* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(TechniqueSetMap_vtable);
    FreeCopiedKeys(self);
    self->vtable = reinterpret_cast<void*>(TechniqueSetMapBase_vtable);
    NiTMap_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(MapBase_vtable);
    NiTMap_RemoveAll(self);
    NiFree_(self->table);
}

// 0x747280: NiStrcpy (strcpy_s, the destination returned)
char* __cdecl NiStrcpyImpl(char* to, uint32_t size, const char* from) {
    crt::StrcpyS(to, size, from);
    return to;
}

// 0x7472A0: SetValue (the key copied when the map copies; the value, an NiPointer passed by value,
// assigned, then let go)
void __fastcall TechniqueSetMap_SetValue(StringMap* self, re::Edx, MapItem* item, const char* key, void* value) {
    if (self->copyKeys) {
        uint32_t size = uint32_t(std::strlen(key)) + 1;
        char* copy = static_cast<char*>(NiMalloc_(size));
        item->key = copy;
        crt::StrcpyS(copy, size, key);
    } else {
        item->key = const_cast<char*>(key);
    }
    void* old = item->value;
    if (old != value) {
        ReleaseRef(old);
        item->value = value;
        if (value == nullptr) {
            return;
        }
        Increment(static_cast<uint8_t*>(value) + 4);
    }
    ReleaseRef(value);   // (the by-value argument's destructor)
}

// 0x747340: NiTStringMap<NiD3DXEffectTechniqueSetPtr>'s deleting destructor
StringMap* __fastcall TechniqueSetMapBase_DeletingDestructor(StringMap* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(TechniqueSetMapBase_vtable);
    NiTMap_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(MapBase_vtable);
    NiTMap_RemoveAll(self);
    NiFree_(self->table);
    if (flags & 1) {
        NiDelete_(self, 0x10);
    }
    return self;
}

// 0x747380: SetAt (the value an NiPointer passed by value: assigned to the item with the key, or a
// new item given a copy of it; then let go)
void __fastcall TechniqueSetMap_SetAtImpl(StringMap* self, re::Edx, const char* key, void* value) {
    uint32_t bucket = NiTStringMap_Hash(key, self->hashSize);
    for (MapItem* item = self->table[bucket]; item != nullptr; item = item->next) {
        if (crt::Strcmp(key, item->key) == 0) {
            void* old = item->value;
            if (old != value) {
                ReleaseRef(old);
                item->value = value;
                if (value == nullptr) {
                    return;
                }
                Increment(static_cast<uint8_t*>(value) + 4);
            }
            ReleaseRef(value);
            return;
        }
    }
    MapItem* item = Virtual<MapItem*>(self, 3);   // NewItem
    AddRefRef(value);                             // (the copy passed on)
    Virtual<void>(self, 1, item, key, value);     // SetValue
    item->next = self->table[bucket];
    self->table[bucket] = item;
    self->count++;
    ReleaseRef(value);
}

// 0x7474A0: GetNext (the value an NiPointer assigned)
void __fastcall TechniqueSetMap_GetNextImpl(StringMap* self, re::Edx, re::Ptr32<MapItem>* position, re::Ptr32<char>* key, re::Ptr32<void>* value) {
    MapItem* item = *position;
    *key = item->key;
    if (*value != item->value) {
        ReleaseRef(*value);
        *value = item->value;
        AddRefRef(*value);
    }
    if (item->next != nullptr) {
        *position = item->next;
        return;
    }
    uint32_t size = self->hashSize;
    for (uint32_t i = NiTStringMap_Hash(item->key, size) + 1; i < size; i++) {
        if (self->table[i] != nullptr) {
            *position = self->table[i];
            return;
        }
    }
    *position = nullptr;
}

// ---- iterating the techniques and their descriptions

// 0x747550: GetNextTechnique: the next technique of the current set, the sets in map order
NiD3DXEffectTechnique* __fastcall NiD3DXEffectFile_GetNextTechnique(NiD3DXEffectFile* self, re::Edx) {
    while (self->setIterator != nullptr) {
        if (self->currentSet == nullptr) {
            re::Ptr32<char> key;
            re::Ptr32<NiD3DXEffectTechniqueSet> set = nullptr;
            TechniqueSetMap_GetNext(&self->techniqueSets, &self->setIterator, &key, &set);
            self->currentSet = set;
            ReleaseRef(set);
        }
        NiD3DXEffectTechniqueSet* set = self->currentSet;
        while (self->techniqueIterator < set->techniques.size) {
            uint32_t index = self->techniqueIterator++;
            if (index < set->techniques.size && set->techniques.base[index] != nullptr) {
                return set->techniques.base[index];
            }
        }
        self->currentSet = nullptr;
        self->techniqueIterator = 0;
    }
    return nullptr;
}

// 0x7475F0: GetTechniqueSet (by shader name; not held)
NiD3DXEffectTechniqueSet* __fastcall NiD3DXEffectFile_GetTechniqueSet(NiD3DXEffectFile* self, re::Edx,
                                                                      const char* name) {
    re::Ptr32<NiD3DXEffectTechniqueSet> set = nullptr;
    TechniqueSetMap_GetAt(&self->techniqueSets, name, &set);
    ReleaseRef(set);
    return set;
}

// 0x747640: GetNextShaderDesc: the next set's NiShaderDesc
void* __fastcall NiD3DXEffectFile_GetNextShaderDesc(NiD3DXEffectFile* self, re::Edx) {
    if (self->setIterator == nullptr) {
        return nullptr;
    }
    re::Ptr32<char> key;
    re::Ptr32<NiD3DXEffectTechniqueSet> set = nullptr;
    TechniqueSetMap_GetNext(&self->techniqueSets, &self->setIterator, &key, &set);
    if (set == nullptr) {
        return nullptr;
    }
    void* desc = TechniqueSet_GetShaderDesc(set, &self->attributeTable);
    ReleaseRef(set);
    return desc;
}

// 0x7476B0: DestroyRendererData: the effect released (its state manager unset first), the
// techniques' shaders and the parameters' textures let go
void __fastcall NiD3DXEffectFile_DestroyRendererData(NiD3DXEffectFile* self, re::Edx) {
    if (self->effect == nullptr) {
        return;
    }
    if (self->rendererDataCreated) {
        Com<long>(self->effect, kSetStateManager, static_cast<void*>(nullptr));
    }
    Com<unsigned long>(self->effect, kRelease);
    self->effect = nullptr;
    self->rendererDataCreated = 0;
    re::Ptr32<MapItem> position = FirstItem(&self->techniqueSets);
    while (position != nullptr) {
        re::Ptr32<char> key;
        re::Ptr32<NiD3DXEffectTechniqueSet> set = nullptr;
        TechniqueSetMap_GetNext(&self->techniqueSets, &position, &key, &set);
        TechniqueSet_ReleaseShaders(set);
        ReleaseRef(set);
    }
    for (ListNode* node = self->parameters.head; node != nullptr;) {
        NiD3DXEffectParameter* parameter = static_cast<NiD3DXEffectParameter*>(node->element);
        node = node->next;
        Parameter_ReleaseTexture(parameter);
    }
}

// 0x747790: AddTechnique: the parameters it uses (by the effect, named) marked used and added to
// it; it goes in the set of its shader name (made if there's none). (`device` isn't used)
void __fastcall NiD3DXEffectFile_AddTechnique(NiD3DXEffectFile* self, re::Edx, NiD3DXEffectTechnique* technique,
                                              void*) {
    for (ListNode* node = self->parameters.head; node != nullptr;) {
        NiD3DXEffectParameter* parameter = static_cast<NiD3DXEffectParameter*>(node->element);
        node = node->next;
        const char* techniqueName = technique->techniqueName;
        if (techniqueName == nullptr) {
            techniqueName = Virtual<const char*>(technique->requirements, 2);   // GetName
        }
        if (Parser_IsParameterUsed(self->effect, parameter->name, techniqueName)) {
            parameter->used = 1;
            Technique_AddParameter(technique, parameter);
        }
    }
    const char* name = Virtual<const char*>(technique->requirements, 2);   // GetName
    re::Ptr32<NiD3DXEffectTechniqueSet> set = nullptr;
    if (!TechniqueSetMap_GetAt(&self->techniqueSets, name, &set) || set == nullptr) {
        void* memory = NiNew_(0x3C);
        NiD3DXEffectTechniqueSet* created = memory != nullptr ? TechniqueSet_Construct(memory) : nullptr;
        AssignRef(set, created);
        TechniqueSet_SetName(set, name);
        AddRefRef(set);   // (the NiPointer passed by value)
        TechniqueSetMap_SetAt(&self->techniqueSets, name, set);
    }
    TechniqueSet_AddTechnique(set, technique);
    ReleaseRef(set);
}

// 0x7478D0: LinkHandles: once on the device, every parameter's handle and every technique's passes
// read from the effect (each technique's description parsed again on the way, unused); whether it was
uint8_t __fastcall NiD3DXEffectFile_LinkHandles(NiD3DXEffectFile* self, re::Edx) {
    if (self->effect == nullptr || !self->rendererDataCreated) {
        return 0;
    }
    void* effect;
    Com<long>(self->effect, kQueryInterface, reinterpret_cast<const void*>(IID_ID3DXEffect), &effect);
    for (ListNode* node = self->parameters.head; node != nullptr;) {
        NiD3DXEffectParameter* parameter = static_cast<NiD3DXEffectParameter*>(node->element);
        node = node->next;
        Parameter_LinkHandle(parameter, effect);
    }
    re::Ptr32<NiD3DXEffectTechniqueSet> set = nullptr;
    re::Ptr32<MapItem> position = Map_GetFirstPos(&self->techniqueSets);
    while (position != nullptr) {
        re::Ptr32<char> key;
        TechniqueSetMap_GetNext(&self->techniqueSets, &position, &key, &set);
        if (set != nullptr) {
            uint32_t count = set->techniques.size;
            for (uint32_t i = 0; i < count; i++) {
                Parser_GetTechnique(effect, i);
                if (i < set->techniques.size && set->techniques.base[i] != nullptr) {
                    Technique_GetPassDescs(set->techniques.base[i], effect);
                }
            }
        }
    }
    Com<unsigned long>(effect, kRelease);
    ReleaseRef(set);
    return 1;
}

// 0x7479D0: GetFirstTechnique
NiD3DXEffectTechnique* __fastcall NiD3DXEffectFile_GetFirstTechnique(NiD3DXEffectFile* self, re::Edx) {
    self->setIterator = FirstItem(&self->techniqueSets);
    if (self->setIterator == nullptr) {
        return nullptr;
    }
    self->currentSet = nullptr;
    self->techniqueIterator = 0;
    return File_GetNextTechnique(self);
}

// 0x747A20: GetFirstShaderDesc
void* __fastcall NiD3DXEffectFile_GetFirstShaderDesc(NiD3DXEffectFile* self, re::Edx) {
    self->setIterator = FirstItem(&self->techniqueSets);
    return File_GetNextShaderDesc(self);
}

// 0x747A60: LoadEffect: the factory given the renderer and asked to load this file (by name), then
// the handles linked
void __fastcall NiD3DXEffectFile_LoadEffect(NiD3DXEffectFile* self, re::Edx, void* renderer) {
    Factory_GetInstance(1);
    Factory_SetD3DRenderer(renderer);
    Factory_GetInstance(1);
    Factory_LoadEffect(self->name);
    File_LinkHandles(self);
}

// 0x747AA0: ParseEffect (once): every parameter and technique the parser describes made (a
// parameter that fails to initialize is dropped, leaked); then the parameters no technique uses
// freed. True when there's no effect too
uint8_t __fastcall NiD3DXEffectFile_ParseEffect(NiD3DXEffectFile* self, re::Edx, void* device) {
    if (self->effect == nullptr || self->parsed) {
        return 1;
    }
    uint32_t parameterCount = 0;
    uint32_t techniqueCount = 0;
    uint8_t ok = Parser_GetEffectDesc(self->effect, &parameterCount, &techniqueCount);
    if (!ok) {
        return ok;
    }
    Parser_ResetCounts();
    for (uint32_t i = 0; i < parameterCount; i++) {
        ParameterDesc* desc = Parser_GetParameter(self->effect, i);
        if (desc == nullptr) {
            continue;
        }
        void* memory = NiNew_(sizeof(NiD3DXEffectParameter));
        NiD3DXEffectParameter* parameter = memory != nullptr ? Parameter_Construct(memory) : nullptr;
        if (Parameter_Initialize(parameter, desc, &self->attributeTable)) {
            PointerList_AddTail(&self->parameters, re::PtrCast<const re::Ptr32<void>*>(&parameter));
        }
    }
    for (uint32_t i = 0; i < techniqueCount; i++) {
        TechniqueDesc* desc = Parser_GetTechnique(self->effect, i);
        if (desc == nullptr) {
            continue;
        }
        void* memory = NiNew_(sizeof(NiD3DXEffectTechnique));
        NiD3DXEffectTechnique* technique = memory != nullptr ? Technique_Construct(memory) : nullptr;
        Technique_Initialize(technique, desc);
        technique->file = self;
        File_AddTechnique(self, technique, device);
    }
    for (ListNode* node = self->parameters.head; node != nullptr;) {
        NiD3DXEffectParameter* parameter = static_cast<NiD3DXEffectParameter*>(node->element);
        ListNode* next = node->next;
        node = next;
        if (parameter->used) {
            continue;
        }
        Parameter_Destruct(parameter);
        NiDelete_(parameter, sizeof(NiD3DXEffectParameter));
        if (next != nullptr && next->prev != nullptr) {
            re::Ptr32<ListNode> position = next->prev;
            NiTPointerList_RemovePos(&self->parameters, &position);
            continue;
        }
        // the tail removed (RemoveTail, inlined)
        ListNode* tail = self->parameters.tail;
        ListNode* previous = tail->prev;
        self->parameters.tail = previous;
        if (previous != nullptr) {
            previous->next = nullptr;
        } else {
            self->parameters.head = nullptr;
        }
        Virtual<void>(&self->parameters, 2, tail);   // the node freed
        self->parameters.count--;
    }
    self->parsed = 1;
    return 1;
}

// 0x747C30: NiTStringPointerMap<NiD3DXEffectTechniqueSetPtr>'s deleting destructor
StringMap* __fastcall TechniqueSetMap_DeletingDestructor(StringMap* self, re::Edx, int flags) {
    TechniqueSetMap_Destruct(self);
    if (flags & 1) {
        NiDelete_(self, 0x14);
    }
    return self;
}

// 0x747C50: the constructor
NiD3DXEffectFile* __fastcall NiD3DXEffectFile_Construct(NiD3DXEffectFile* self, re::Edx) {
    self->name = nullptr;
    self->unknown4 = 0;
    StringMap* map = &self->techniqueSets;
    map->vtable = reinterpret_cast<void*>(MapBase_vtable);
    map->hashSize = 0x25;
    map->count = 0;
    map->table = static_cast<re::Ptr32<MapItem>*>(NiMalloc_(0x94));
    crt::Memset(map->table, 0, map->hashSize * 4);
    map->copyKeys = 1;
    map->vtable = reinterpret_cast<void*>(TechniqueSetMap_vtable);
    self->parameters.count = 0;
    self->parameters.head = nullptr;
    self->parameters.tail = nullptr;
    self->parameters.vtable = reinterpret_cast<void*>(PointerList_vtable);
    self->setIterator = nullptr;
    self->techniqueIterator = 0;
    self->currentSet = nullptr;
    AttributeTable_Construct(&self->attributeTable);
    self->effect = nullptr;
    self->rendererDataCreated = 0;
    self->parsed = 0;
    return self;
}

// 0x747CD0: the destructor: out of the factory's map, the renderer data destroyed, the name freed,
// the parameters freed. (The technique sets are walked in a new empty map built for it, so the
// loop never runs; the sets go with the member map's destructor)
void __fastcall NiD3DXEffectFile_Destruct(NiD3DXEffectFile* self, re::Edx) {
    NiD3DRenderer_Error("Killing NiD3DXEffectFile %s\n", self->name);
    NiD3DXEffectFactory* factory = Factory_GetInstance(0);
    if (factory != nullptr) {
        Virtual<void>(factory, 2, self->name);   // RemoveD3DXEffectFileFromMap
    }
    File_DestroyRendererData(self);
    NiFree_(self->name);
    StringMap emptied;
    ConstructTechniqueSetMap(&emptied);
    self->setIterator = FirstItem(&emptied);
    while (self->setIterator != nullptr) {
        re::Ptr32<char> key;
        re::Ptr32<NiD3DXEffectTechniqueSet> set = nullptr;
        TechniqueSetMap_GetNext(&emptied, &self->setIterator, &key, &set);
        TechniqueSetMap_SetAt(&emptied, key, nullptr);
        ReleaseRef(set);
    }
    NiTMap_RemoveAll(&emptied);
    for (ListNode* node = self->parameters.head; node != nullptr;) {
        NiD3DXEffectParameter* parameter = static_cast<NiD3DXEffectParameter*>(node->element);
        node = node->next;
        if (parameter != nullptr) {
            Parameter_Destruct(parameter);
            NiDelete_(parameter, sizeof(NiD3DXEffectParameter));
        }
    }
    NiTPointerList_RemoveAll(&self->parameters);
    TechniqueSetMap_Destruct(&emptied);
    AttributeTable_Destruct(&self->attributeTable);
    self->parameters.vtable = reinterpret_cast<void*>(PointerList_vtable);
    NiTPointerList_RemoveAll(&self->parameters);
    self->parameters.vtable = reinterpret_cast<void*>(PointerListBase_vtable);
    TechniqueSetMap_Destruct(&self->techniqueSets);
}

// 0x747E60: SetD3DXEffect (a different effect: the old one released, the new one parsed and, with a
// device, linked); whether it was parsed (and linked)
uint8_t __fastcall NiD3DXEffectFile_SetD3DXEffect(NiD3DXEffectFile* self, re::Edx, void* effect, void* device) {
    if (self->effect == effect) {
        return 0;
    }
    if (self->effect != nullptr) {
        Com<unsigned long>(self->effect, kRelease);
        self->effect = nullptr;
        self->rendererDataCreated = 0;
    }
    self->effect = effect;
    if (!File_ParseEffect(self, device)) {
        return 0;
    }
    if (device == nullptr) {
        return 1;
    }
    self->rendererDataCreated = 1;
    return File_LinkHandles(self);
}

// ---- NiTStringPointerMap<T*> (plain values: the factory's files, the library's technique sets,
// the shadow manager's loaders)

// 0x747F00: SetAt
void __fastcall PointerMap_SetAt(StringMap* self, re::Edx, const char* key, void* value) {
    uint32_t bucket = NiTStringMap_Hash(key, self->hashSize);
    for (MapItem* item = self->table[bucket]; item != nullptr; item = item->next) {
        if (crt::Strcmp(key, item->key) == 0) {
            item->value = value;
            return;
        }
    }
    MapItem* item = Virtual<MapItem*>(self, 3);   // NewItem
    Virtual<void>(self, 1, item, key, value);     // SetValue
    item->next = self->table[bucket];
    self->table[bucket] = item;
    self->count++;
}

// 0x747FA0: an item freed (NiTPointerMap's allocator: the value cleared)
void __fastcall PointerMap_DeleteItem(void*, re::Edx, MapItem* item) {
    item->value = nullptr;
    NiDelete_(item, sizeof(MapItem));
}

// 0x747FC0: the destructor
void __fastcall PointerMap_DestructImpl(StringMap* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(PointerMap_vtable);
    FreeCopiedKeys(self);
    self->vtable = reinterpret_cast<void*>(PointerMapMiddle_vtable);
    NiTMap_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(PointerMapBase_vtable);
    NiTMap_RemoveAll(self);
    NiFree_(self->table);
}

// 0x748030: SetValue (the key copied when the map copies)
void __fastcall PointerMap_SetValue(StringMap* self, re::Edx, MapItem* item, const char* key, void* value) {
    if (self->copyKeys) {
        uint32_t size = uint32_t(std::strlen(key)) + 1;
        char* copy = static_cast<char*>(NiMalloc_(size));
        item->key = copy;
        crt::StrcpyS(copy, size, key);
        item->value = value;
        return;
    }
    item->key = const_cast<char*>(key);
    item->value = value;
}

// 0x748090: NiTMapBase's deleting destructor (vtable 0x950760)
StringMap* __fastcall PointerMapBase_DeletingDestructor(StringMap* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(PointerMapBase_vtable);
    NiTMap_RemoveAll(self);
    NiFree_(self->table);
    if (flags & 1) {
        NiDelete_(self, 0x10);
    }
    return self;
}

// 0x7480D0: the deleting destructor
StringMap* __fastcall PointerMap_DeletingDestructor(StringMap* self, re::Edx, int flags) {
    PointerMap_Destruct(self);
    if (flags & 1) {
        NiDelete_(self, 0x14);
    }
    return self;
}

// 0x748170: NiTPointerList<T*>::AddTail (a node from the list's allocator, slot 1)
void __fastcall PointerList_AddTailImpl(PointerList* self, re::Edx, const re::Ptr32<void>* element) {
    ListNode* node = Virtual<ListNode*>(self, 1);
    node->element = *element;
    node->next = nullptr;
    node->prev = self->tail;
    if (self->tail != nullptr) {
        self->tail->next = node;
        self->count++;
        self->tail = node;
        return;
    }
    self->count++;
    self->head = node;
    self->tail = node;
}

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NID3DXFILE_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                                 \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NID3DXFILE_REVERSED("NiD3DXEffectFile::SetName", 0x007471B0, &ni::NiD3DXEffectFile_SetName);
NID3DXFILE_REVERSED("NiD3DXEffectFile::HandleLostDevice", 0x007471D0, &ni::NiD3DXEffectFile_HandleLostDevice);
NID3DXFILE_REVERSED("NiD3DXEffectFile::HandleResetDevice", 0x007471F0, &ni::NiD3DXEffectFile_HandleResetDevice);
NID3DXFILE_REVERSED("NiTStringPointerMap<NiD3DXEffectTechniqueSetPtr>::~NiTStringPointerMap", 0x00747210, &ni::TechniqueSetMap_DestructImpl);
NID3DXFILE_REVERSED("NiD3DXEffect::NiStrcpy", 0x00747280, &ni::NiStrcpyImpl);
NID3DXFILE_REVERSED("NiTStringPointerMap<NiD3DXEffectTechniqueSetPtr>::SetValue", 0x007472A0, &ni::TechniqueSetMap_SetValue);
NID3DXFILE_REVERSED("NiTStringMap<NiD3DXEffectTechniqueSetPtr>::DeletingDestructor", 0x00747340, &ni::TechniqueSetMapBase_DeletingDestructor);
NID3DXFILE_REVERSED("NiTStringPointerMap<NiD3DXEffectTechniqueSetPtr>::SetAt", 0x00747380, &ni::TechniqueSetMap_SetAtImpl);
NID3DXFILE_REVERSED("NiTStringPointerMap<NiD3DXEffectTechniqueSetPtr>::GetNext", 0x007474A0, &ni::TechniqueSetMap_GetNextImpl);
NID3DXFILE_REVERSED("NiD3DXEffectFile::GetNextTechnique", 0x00747550, &ni::NiD3DXEffectFile_GetNextTechnique);
NID3DXFILE_REVERSED("NiD3DXEffectFile::GetTechniqueSet", 0x007475F0, &ni::NiD3DXEffectFile_GetTechniqueSet);
NID3DXFILE_REVERSED("NiD3DXEffectFile::GetNextShaderDesc", 0x00747640, &ni::NiD3DXEffectFile_GetNextShaderDesc);
NID3DXFILE_REVERSED("NiD3DXEffectFile::DestroyRendererData", 0x007476B0, &ni::NiD3DXEffectFile_DestroyRendererData);
NID3DXFILE_REVERSED("NiD3DXEffectFile::AddTechnique", 0x00747790, &ni::NiD3DXEffectFile_AddTechnique);
NID3DXFILE_REVERSED("NiD3DXEffectFile::LinkHandles", 0x007478D0, &ni::NiD3DXEffectFile_LinkHandles);
NID3DXFILE_REVERSED("NiD3DXEffectFile::GetFirstTechnique", 0x007479D0, &ni::NiD3DXEffectFile_GetFirstTechnique);
NID3DXFILE_REVERSED("NiD3DXEffectFile::GetFirstShaderDesc", 0x00747A20, &ni::NiD3DXEffectFile_GetFirstShaderDesc);
NID3DXFILE_REVERSED("NiD3DXEffectFile::LoadEffect", 0x00747A60, &ni::NiD3DXEffectFile_LoadEffect);
NID3DXFILE_REVERSED("NiD3DXEffectFile::ParseEffect", 0x00747AA0, &ni::NiD3DXEffectFile_ParseEffect);
NID3DXFILE_REVERSED("NiTStringPointerMap<NiD3DXEffectTechniqueSetPtr>::DeletingDestructor", 0x00747C30, &ni::TechniqueSetMap_DeletingDestructor);
NID3DXFILE_REVERSED("NiD3DXEffectFile::NiD3DXEffectFile", 0x00747C50, &ni::NiD3DXEffectFile_Construct);
NID3DXFILE_REVERSED("NiD3DXEffectFile::~NiD3DXEffectFile", 0x00747CD0, &ni::NiD3DXEffectFile_Destruct);
NID3DXFILE_REVERSED("NiD3DXEffectFile::SetD3DXEffect", 0x00747E60, &ni::NiD3DXEffectFile_SetD3DXEffect);
NID3DXFILE_REVERSED("NiTStringPointerMap<T*>::SetAt", 0x00747F00, &ni::PointerMap_SetAt);
NID3DXFILE_REVERSED("NiTPointerMap::DeleteItem_747FA0", 0x00747FA0, &ni::PointerMap_DeleteItem);
NID3DXFILE_REVERSED("NiTStringPointerMap<T*>::~NiTStringPointerMap", 0x00747FC0, &ni::PointerMap_DestructImpl);
NID3DXFILE_REVERSED("NiTStringPointerMap<T*>::SetValue", 0x00748030, &ni::PointerMap_SetValue);
NID3DXFILE_REVERSED("NiTMapBase<T*>::DeletingDestructor_748090", 0x00748090, &ni::PointerMapBase_DeletingDestructor);
NID3DXFILE_REVERSED("NiTStringPointerMap<T*>::DeletingDestructor", 0x007480D0, &ni::PointerMap_DeletingDestructor);
NID3DXFILE_REVERSED("NiTPointerList<T*>::AddTail", 0x00748170, &ni::PointerList_AddTailImpl);
