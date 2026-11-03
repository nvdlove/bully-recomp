// NiDX9LightManager (0x240, built by NiDX9Renderer::Initialize) and NiDX9LightEntry (0x74): the
// fixed-function lights. The manager: +0x0 the entries by light (NiTPointerMap<NiLight*,
// NiDX9LightEntry*>, 37 buckets), +0x10 the lights enabled now (NiTPointerList<NiLight*>), +0x20
// the device (a reference), +0x24 the render state, +0x28 the "no state" marker (-1), +0x2C the
// last dynamic effect state, +0x30 / +0x31 lighting then / now, +0x34 the texture apply mode,
// +0x38 the vertex colours' source then, +0x3C the next light index to try, +0x40 the indices in
// use (4096 bits). A light's entry is also kept at its +0x120.
//
// The entry: the D3DLIGHT9 (+0x0..0x68), +0x68 the light's revision it was made from, +0x6C its
// index, +0x70 "not seen this frame", +0x71 enabled.

#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kMapBaseVtable = 0x00950760;
constexpr uintptr_t kPointerMapVtable = 0x00965A9C;
constexpr uintptr_t kLightListVtable = 0x0095145C;   // NiTPointerList<NiLight*>
constexpr uintptr_t kOtherListVtable = 0x00952E10;
constexpr uintptr_t kListBaseVtable = 0x0099C748;

// IDirect3DDevice9's slots
enum : int {
    kSetRenderState = 57,
    kSetLight = 51,
    kLightEnable = 53,
};
// D3DRENDERSTATETYPE
enum : uint32_t {
    D3DRS_LIGHTING = 0x89,
    D3DRS_AMBIENT = 0x8B,
    D3DRS_DIFFUSEMATERIALSOURCE = 0x91,
    D3DRS_AMBIENTMATERIALSOURCE = 0x93,
    D3DRS_EMISSIVEMATERIALSOURCE = 0x94,
};

GAME_VAR(float, ms_fMaxRange, 0x020E7050);   // sqrt(FLT_MAX), a light's range

GAME_FN(void, __thiscall, RenderState_SetRenderState, 0x00406630, void* self, uint32_t state, uint32_t value);
GAME_FN(void*, __thiscall, PointerMap_GetFirstPos, 0x008CA4E0, void* map);
GAME_FN(void, __thiscall, PointerMap_RemoveAt, 0x008A5A10, void* map, void* key);
GAME_FN(void, __thiscall, PointerMap_SetAt, 0x008A5140, void* map, void* key, void* value);
GAME_FN(void, __thiscall, PointerMap_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, PointerList_RemovePos, 0x00771DA0, void* list, re::Ptr32<void>* position);
GAME_FN(void, __thiscall, PointerList_Remove, 0x0075F630, void* list, const re::Ptr32<void>* element);
// this file's (called where modes apply)
GAME_FN(uint32_t, __thiscall, LightManager_GetNextAvailableIndex, 0x00891920, void* self);
GAME_FN(uint8_t, __thiscall, LightEntry_Update, 0x00891980, void* self, uint8_t* light);
GAME_FN(void, __thiscall, LightManager_Reset, 0x00891BD0, void* self);
GAME_FN(void, __thiscall, LightList_AddHead, 0x00891D30, void* self, const re::Ptr32<void>* element);
GAME_FN(void*, __thiscall, LightEntry_Construct, 0x00891D80, void* self, uint8_t* light, uint32_t index);
GAME_FN(void, __thiscall, LightManager_SetLights, 0x00891EB0, void* self, uint8_t* effects);

struct Node {
    re::Ptr32<Node> next;
    re::Ptr32<Node> prev;
    re::Ptr32<void> element;
};
struct MapItem {   // (the game's layout: 0xC)
    re::Ptr32<MapItem> next;
    re::Ptr32<uint8_t> key;
    re::Ptr32<uint8_t> value;
};
static_assert(sizeof(MapItem) == 0xC);

inline uint8_t* RenderState(uint8_t* self) {
    return At<uint8_t*>(self, 0x24);
}
inline void* Device(uint8_t* self) {
    return At<void*>(self, 0x20);
}
// NiDX9RenderState::SetRenderState as inlined (its cache at +0x120, 8 bytes a state; the device at
// +0x2238)
inline void SetRenderState(uint8_t* renderState, uint32_t state, uint32_t value) {
    uint32_t& cached = At<uint32_t>(renderState, 0x120 + int(state) * 8);
    if (cached != value) {
        Com<long>(At<void*>(renderState, 0x2238), kSetRenderState, state, value);
        cached = value;
    }
}
inline void LightEnable(uint8_t* self, uint8_t* entry, int enable) {
    Com<long>(Device(self), kLightEnable, At<uint32_t>(entry, 0x6C), enable);
}
inline void ReleaseIndex(uint8_t* self, uint32_t index) {
    At<uint32_t>(self, 0x40 + int(index >> 5) * 4) &= ~(1u << (index & 0x1F));
}
inline uint8_t* EntryOf(void* light) {
    return At<uint8_t*>(light, 0x120);
}
// a colour channel times 255, chopped through a 64-bit integer, at most 255 (unsigned)
inline uint32_t ToByte(float c) {
    uint32_t v = uint32_t(crt::FloatToInt(double(float(double(c) * 255.0))));
    return v > 0xFF ? 0xFF : v;
}

} // namespace

// 0x891920 (thiscall): NiDX9LightManager::GetNextAvailableIndex (the next free index from where
// the last search stopped, wrapping at 4096; marked used. 0xFFFFFFFF when all are)
uint32_t __fastcall NiDX9LightManager_GetNextAvailableIndex(uint8_t* self, re::Edx) {
    uint32_t start = At<uint32_t>(self, 0x3C);
    uint32_t i = start;
    do {
        uint32_t bit = 1u << (i & 0x1F);
        uint32_t& word = At<uint32_t>(self, 0x40 + int(i >> 5) * 4);
        if ((word & bit) == 0) {
            word |= bit;
            At<uint32_t>(self, 0x3C) = (i + 1) & 0xFFF;
            return i;
        }
        i++;
        if (i == 0x1000) {
            i = 0;
        }
    } while (i != start);
    return 0xFFFFFFFF;
}

// 0x891980 (thiscall): NiDX9LightEntry::Update (the D3DLIGHT9 made again from the light when its
// revision changed: point, directional or spot; the colours times the dimmer. True when it was)
uint8_t __fastcall NiDX9LightEntry_Update(uint8_t* self, re::Edx, uint8_t* light) {
    uint32_t revision = At<uint32_t>(light, 0xD0);
    if (At<uint32_t>(self, 0x68) == revision) {
        return 0;
    }
    At<uint32_t>(self, 0x68) = revision;
    crt::Memset(self, 0, 0x68);
    float* d3d = reinterpret_cast<float*>(self);
    bool attenuation = false;
    switch (At<uint8_t>(light, 0xC5)) {
    case 2:
    case 5:   // point
        At<uint32_t>(self, 0) = 1;
        d3d[13] = At<float>(light, 0x84);
        d3d[14] = At<float>(light, 0x88);
        d3d[15] = At<float>(light, 0x8C);
        attenuation = true;
        break;
    case 3:
    case 6:   // directional
        At<uint32_t>(self, 0) = 3;
        d3d[16] = At<float>(light, 0x124);
        d3d[17] = At<float>(light, 0x128);
        d3d[18] = At<float>(light, 0x12C);
        break;
    case 4:
    case 7: {   // spot: the outer cone from the angle (degrees, the half angle doubled)
        At<uint32_t>(self, 0) = 2;
        d3d[13] = At<float>(light, 0x84);
        d3d[14] = At<float>(light, 0x88);
        d3d[15] = At<float>(light, 0x8C);
        d3d[16] = At<float>(light, 0x130);
        d3d[17] = At<float>(light, 0x134);
        d3d[18] = At<float>(light, 0x138);
        d3d[24] = 0.0f;
        float angle = float(double(At<float>(light, 0x13C)) * 3.1415927410125732);
        d3d[25] = float(double(angle) / 90.0);
        d3d[20] = At<float>(light, 0x144);
        attenuation = true;
        break;
    }
    default:
        break;
    }
    if (attenuation) {
        d3d[21] = At<float>(light, 0x124);
        d3d[22] = At<float>(light, 0x128);
        d3d[23] = At<float>(light, 0x12C);
        d3d[19] = ms_fMaxRange;
    }
    float dimmer = At<float>(light, 0xF8);
    d3d[9] = At<float>(light, 0xFC) * dimmer;   // ambient
    d3d[10] = At<float>(light, 0x100) * dimmer;
    d3d[11] = At<float>(light, 0x104) * dimmer;
    d3d[12] = 1.0f;
    d3d[1] = At<float>(light, 0x108) * dimmer;   // diffuse
    d3d[2] = At<float>(light, 0x10C) * dimmer;
    d3d[3] = At<float>(light, 0x110) * dimmer;
    d3d[4] = 1.0f;
    d3d[5] = At<float>(light, 0x114) * dimmer;   // specular
    d3d[6] = At<float>(light, 0x118) * dimmer;
    d3d[7] = At<float>(light, 0x11C) * dimmer;
    d3d[8] = 1.0f;
    return 1;
}

// 0x891BD0 (thiscall): NiDX9LightManager::Reset (ambient, lighting and the material sources
// off; no state seen)
void __fastcall NiDX9LightManager_Reset(uint8_t* self, re::Edx) {
    SetRenderState(RenderState(self), D3DRS_AMBIENT, 0);
    SetRenderState(RenderState(self), D3DRS_LIGHTING, 0);
    At<uint32_t>(self, 0x34) = 0;
    At<uint8_t>(self, 0x31) = 0;
    At<uint32_t>(self, 0x38) = 0;
    SetRenderState(RenderState(self), D3DRS_EMISSIVEMATERIALSOURCE, 0);
    SetRenderState(RenderState(self), D3DRS_AMBIENTMATERIALSOURCE, 0);
    SetRenderState(RenderState(self), D3DRS_DIFFUSEMATERIALSOURCE, 0);
    At<uint8_t>(self, 0x30) = 0;
    At<uint32_t>(self, 0x2C) = At<uint32_t>(self, 0x28);
}

// 0x891CC0 (thiscall): NiDX9LightManager::DisableLights (every enabled light turned off and taken
// off the list; no state seen)
void __fastcall NiDX9LightManager_DisableLights(uint8_t* self, re::Edx) {
    while (At<uint32_t>(self, 0x1C) != 0) {
        Node* node = At<Node*>(self, 0x14);
        Node* next = node->next;
        At<Node*>(self, 0x14) = next;
        if (next != nullptr) {
            next->prev = nullptr;
        } else {
            At<Node*>(self, 0x18) = nullptr;
        }
        void* light = node->element;
        Virtual<void>(self + 0x10, 2, node);   // the allocator frees the node
        At<uint32_t>(self, 0x1C)--;
        uint8_t* entry = EntryOf(light);
        LightEnable(self, entry, 0);
        At<uint8_t>(entry, 0x71) = 0;
    }
    At<uint32_t>(self, 0x2C) = At<uint32_t>(self, 0x28);
}

// 0x891D30 (thiscall): NiTPointerList<NiLight*>::AddHead
void __fastcall NiTPointerList_AddHead_891D30(uint8_t* self, re::Edx, const re::Ptr32<void>* element) {
    Node* node = Virtual<Node*>(self, 1);
    node->element = *element;
    node->prev = nullptr;
    node->next = At<Node*>(self, 4);
    if (At<Node*>(self, 4) != nullptr) {
        At<Node*>(self, 4)->prev = node;
        At<uint32_t>(self, 0xC)++;
        At<Node*>(self, 4) = node;
        return;
    }
    At<uint32_t>(self, 0xC)++;
    At<Node*>(self, 8) = node;
    At<Node*>(self, 4) = node;
}

// 0x891D80 (thiscall): NiDX9LightEntry's constructor
void* __fastcall NiDX9LightEntry_Construct(uint8_t* self, re::Edx, uint8_t* light, uint32_t index) {
    At<uint32_t>(self, 0x6C) = index;
    At<uint32_t>(self, 0x68) = 0;
    At<uint8_t>(self, 0x70) = 0;
    At<uint8_t>(self, 0x71) = 0;
    LightEntry_Update(self, light);
    return self;
}

// 0x891DB0 (thiscall): NiDX9LightManager's destructor (every entry deleted, its index freed; the
// list emptied; the device let go)
void __fastcall NiDX9LightManager_Destruct(uint8_t* self, re::Edx) {
    while (At<uint32_t>(self, 0xC) != 0) {
        MapItem* item = static_cast<MapItem*>(PointerMap_GetFirstPos(self));
        uint8_t* light = item->key;
        uint8_t* entry = item->value;
        // (the original works out the next position here and drops it)
        PointerMap_RemoveAt(self, light);
        At<uint8_t*>(light, 0x120) = nullptr;
        ReleaseIndex(self, At<uint32_t>(entry, 0x6C));
        NiDelete(entry, 0x74);
    }
    while (At<uint32_t>(self, 0x1C) != 0) {
        Node* node = At<Node*>(self, 0x14);
        Node* next = node->next;
        At<Node*>(self, 0x14) = next;
        if (next != nullptr) {
            next->prev = nullptr;
        } else {
            At<Node*>(self, 0x18) = nullptr;
        }
        Virtual<void>(self + 0x10, 2, node);
        At<uint32_t>(self, 0x1C)--;
    }
    if (Device(self) != nullptr) {
        Com<long>(Device(self), 2);   // Release
        At<void*>(self, 0x20) = nullptr;
    }
    At<uint32_t>(self, 0x10) = kOtherListVtable;
    PointerList_RemoveAll(self + 0x10);
    At<uint32_t>(self, 0x10) = kListBaseVtable;
    At<uint32_t>(self, 0) = kPointerMapVtable;
    PointerMap_RemoveAll(self);
    At<uint32_t>(self, 0) = kMapBaseVtable;
    PointerMap_RemoveAll(self);
    NiFree(At<void*>(self, 8));
}

// 0x891EB0 (thiscall): NiDX9LightManager::SetLights (with lighting on, the first 8 lights of the
// state that are on and not too dim (1/100) set and enabled, the ambient ones summed for
// D3DRS_AMBIENT instead; the lights of before no longer there disabled and dropped)
void __fastcall NiDX9LightManager_SetLights(uint8_t* self, re::Edx, uint8_t* effects) {
    float ambient[3] = {0.0f, 0.0f, 0.0f};
    for (Node* node = At<Node*>(self, 0x14); node != nullptr; node = node->next) {
        At<uint8_t>(EntryOf(node->element), 0x70) = 1;
    }
    if (At<uint8_t>(self, 0x31) != 0) {
        re::Ptr32<void>* item = effects != nullptr ? At<re::Ptr32<void>*>(effects, 0xC) : nullptr;
        uint32_t count = 0;
        while (item != nullptr && count < 8) {
            uint8_t* light = static_cast<uint8_t*>(item[1]);
            item = static_cast<re::Ptr32<void>*>(item[0]);
            if (At<uint8_t>(light, 0xC4) == 0 || At<float>(light, 0xF8) < 0.01f) {
                continue;
            }
            if (At<uint8_t>(light, 0xC5) == 0) {   // an ambient light
                float dimmer = At<float>(light, 0xF8);
                ambient[0] = ambient[0] + At<float>(light, 0xFC) * dimmer;
                ambient[1] = At<float>(light, 0x100) * dimmer + ambient[1];
                ambient[2] = dimmer * At<float>(light, 0x104) + ambient[2];
                continue;
            }
            uint8_t* entry = EntryOf(light);
            bool set;
            if (entry == nullptr) {
                uint32_t index = LightManager_GetNextAvailableIndex(self);
                if (index == 0xFFFFFFFF) {
                    continue;
                }
                void* memory = NiNew(0x74);
                entry = memory != nullptr ? static_cast<uint8_t*>(LightEntry_Construct(memory, light, index)) : nullptr;
                PointerMap_SetAt(self, light, entry);
                At<uint8_t*>(light, 0x120) = entry;
                set = true;
            } else {
                set = LightEntry_Update(entry, light) != 0;
            }
            if (set) {
                Com<long>(Device(self), kSetLight, At<uint32_t>(entry, 0x6C), entry);
            }
            if (At<uint8_t>(entry, 0x71) == 0) {
                re::Ptr32<void> held = light;
                LightList_AddHead(self + 0x10, &held);
            }
            count++;
            At<uint8_t>(entry, 0x70) = 0;
        }
    }
    for (Node* node = At<Node*>(self, 0x14); node != nullptr;) {
        uint8_t* entry = EntryOf(node->element);
        re::Ptr32<void> position = node;
        node = node->next;
        if (At<uint8_t>(entry, 0x70) != 0) {
            LightEnable(self, entry, 0);
            At<uint8_t>(entry, 0x71) = 0;
            PointerList_RemovePos(self + 0x10, &position);
        }
    }
    for (Node* node = At<Node*>(self, 0x14); node != nullptr;) {
        uint8_t* entry = EntryOf(node->element);
        node = node->next;
        if (At<uint8_t>(entry, 0x70) == 0 && At<uint8_t>(entry, 0x71) == 0) {
            LightEnable(self, entry, 1);
            At<uint8_t>(entry, 0x71) = 1;
        }
    }
    uint32_t r = ToByte(ambient[0]);
    uint32_t g = ToByte(ambient[1]);
    uint32_t b = ToByte(ambient[2]);
    uint32_t color = ((((r & 0xFF) | 0xFFFFFF00) << 8 | (g & 0xFF)) << 8) | (b & 0xFF);
    SetRenderState(RenderState(self), D3DRS_AMBIENT, color);
}

// 0x8921D0 (thiscall): NiDX9LightManager's constructor
void* __fastcall NiDX9LightManager_Construct(uint8_t* self, re::Edx, void* renderState, void* device) {
    At<uint32_t>(self, 0) = kMapBaseVtable;
    At<uint32_t>(self, 4) = 37;
    At<uint32_t>(self, 0xC) = 0;
    void* buckets = NiMalloc(0x94);
    At<void*>(self, 8) = buckets;
    crt::Memset(buckets, 0, At<uint32_t>(self, 4) * 4);
    At<uint32_t>(self, 0) = kPointerMapVtable;
    At<uint32_t>(self, 0x1C) = 0;
    At<void*>(self, 0x14) = nullptr;
    At<void*>(self, 0x18) = nullptr;
    At<uint32_t>(self, 0x10) = kLightListVtable;
    At<void*>(self, 0x24) = renderState;
    At<uint32_t>(self, 0x28) = 0xFFFFFFFF;
    At<void*>(self, 0x20) = device;
    Com<long>(device, 1);   // AddRef
    At<uint32_t>(self, 0x3C) = 0;
    crt::Memset(self + 0x40, 0, 0x200);
    LightManager_Reset(self);
    return self;
}

// 0x892260 (thiscall): NiDX9LightManager::SetState (lighting off for a replace-mode base texture;
// the material sources from the vertex colour property (or lighting off when it's emissive-only
// without its "lit" bit... by its source mode); then the lights when the state or lighting changed.
// Quirk kept: the sources are reset to the material only from the "emissive" source)
void __fastcall NiDX9LightManager_SetState(uint8_t* self, re::Edx, uint8_t* effects, uint8_t* texturing,
                                           uint8_t* vertexColor) {
    uint32_t apply = (At<uint8_t>(texturing, 0x18) >> 1) & 7;
    At<uint32_t>(self, 0x34) = apply;
    uint8_t* base = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[0];
    if (base != nullptr && At<void*>(base, 8) != nullptr && apply == 0) {
        RenderState_SetRenderState(RenderState(self), D3DRS_LIGHTING, apply);
        return;
    }
    uint16_t flags = At<uint16_t>(vertexColor, 0x18);
    uint32_t source = (flags >> 4) & 3;
    if ((flags & 8) == 0) {
        if (source == 1 || source == 3) {
            SetRenderState(RenderState(self), D3DRS_LIGHTING, 0);
            return;
        }
        At<uint8_t>(self, 0x31) = 0;
        if (At<uint32_t>(self, 0x38) == 1) {
            RenderState_SetRenderState(RenderState(self), D3DRS_EMISSIVEMATERIALSOURCE, 0);
            RenderState_SetRenderState(RenderState(self), D3DRS_AMBIENTMATERIALSOURCE, 0);
            RenderState_SetRenderState(RenderState(self), D3DRS_DIFFUSEMATERIALSOURCE, 0);
            At<uint32_t>(self, 0x38) = 0;
        }
    } else {
        if (source != At<uint32_t>(self, 0x38)) {
            switch (source) {
            case 0:
                RenderState_SetRenderState(RenderState(self), D3DRS_EMISSIVEMATERIALSOURCE, 0);
                RenderState_SetRenderState(RenderState(self), D3DRS_AMBIENTMATERIALSOURCE, 0);
                RenderState_SetRenderState(RenderState(self), D3DRS_DIFFUSEMATERIALSOURCE, 0);
                break;
            case 1:
                RenderState_SetRenderState(RenderState(self), D3DRS_EMISSIVEMATERIALSOURCE, 1);
                RenderState_SetRenderState(RenderState(self), D3DRS_AMBIENTMATERIALSOURCE, 0);
                RenderState_SetRenderState(RenderState(self), D3DRS_DIFFUSEMATERIALSOURCE, 0);
                break;
            case 2:
                RenderState_SetRenderState(RenderState(self), D3DRS_EMISSIVEMATERIALSOURCE, 0);
                RenderState_SetRenderState(RenderState(self), D3DRS_AMBIENTMATERIALSOURCE, 1);
                RenderState_SetRenderState(RenderState(self), D3DRS_DIFFUSEMATERIALSOURCE, 1);
                break;
            default:
                break;
            }
            At<uint32_t>(self, 0x38) = source;
        }
        At<uint8_t>(self, 0x31) = 1;
    }
    SetRenderState(RenderState(self), D3DRS_LIGHTING, 1);
    uint8_t* last = At<uint8_t*>(self, 0x2C);
    if (last != At<uint8_t*>(self, 0x28) && effects == last && At<uint8_t>(self, 0x31) == At<uint8_t>(self, 0x30)) {
        return;
    }
    LightManager_SetLights(self, effects);
    At<uint8_t*>(self, 0x2C) = effects;
    At<uint8_t>(self, 0x30) = At<uint8_t>(self, 0x31);
}

// 0x892410 (thiscall): NiDX9LightManager::RemoveLight (its entry disabled and deleted, its index
// freed. Quirk kept: a light whose +0x120 is clear but that the map still has loses its index but
// not its entry)
void __fastcall NiDX9LightManager_RemoveLight(uint8_t* self, re::Edx, uint8_t* light) {
    uint8_t* entry = EntryOf(light);
    MapItem* item = At<re::Ptr32<MapItem>*>(self, 8)[reinterpret_cast<uintptr_t>(light) % At<uint32_t>(self, 4)];
    while (item != nullptr && item->key != light) {
        item = item->next;
    }
    re::Ptr32<void> held = light;
    if (entry != nullptr) {
        LightEnable(self, entry, 0);
        At<uint8_t>(entry, 0x71) = 0;
        PointerMap_RemoveAt(self, light);
        PointerList_Remove(self + 0x10, &held);
        At<uint8_t*>(light, 0x120) = nullptr;
        ReleaseIndex(self, At<uint32_t>(entry, 0x6C));
        NiDelete(entry, 0x74);
        return;
    }
    if (item == nullptr) {
        return;
    }
    entry = item->value;
    PointerMap_RemoveAt(self, light);
    PointerList_Remove(self + 0x10, &held);
    At<uint8_t*>(light, 0x120) = nullptr;
    ReleaseIndex(self, At<uint32_t>(entry, 0x6C));
}

// 0x892500 (thiscall): NiDX9LightManager::ReleaseLights (every entry disabled and deleted, its
// light's list node dropped. Quirk kept: the indices stay marked used)
void __fastcall NiDX9LightManager_ReleaseLights(uint8_t* self, re::Edx) {
    uint32_t size = At<uint32_t>(self, 4);
    re::Ptr32<MapItem>* buckets = At<re::Ptr32<MapItem>*>(self, 8);
    MapItem* item = nullptr;
    for (uint32_t i = 0; i < size; i++) {
        if (buckets[i] != nullptr) {
            item = buckets[i];
            break;
        }
    }
    while (item != nullptr) {
        uint8_t* entry = item->value;
        uint8_t* light = item->key;
        MapItem* next = item->next;
        if (next == nullptr) {
            uint32_t count = At<uint32_t>(self, 4);
            for (uint32_t i = reinterpret_cast<uintptr_t>(light) % count + 1; i < count; i++) {
                if (At<re::Ptr32<MapItem>*>(self, 8)[i] != nullptr) {
                    next = At<re::Ptr32<MapItem>*>(self, 8)[i];
                    break;
                }
            }
        }
        if (entry != nullptr) {
            LightEnable(self, entry, 0);
            At<uint8_t>(entry, 0x71) = 0;
            PointerMap_RemoveAt(self, light);
            Node* node = At<Node*>(self, 0x14);
            while (node != nullptr && node->element != light) {
                node = node->next;
            }
            if (node != nullptr) {
                re::Ptr32<void> position = node;
                PointerList_RemovePos(self + 0x10, &position);
            }
            At<uint8_t*>(light, 0x120) = nullptr;
            NiDelete(entry, 0x74);
        }
        item = next;
    }
}

#define C40C_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40C_REVERSED("NiDX9LightManager::GetNextAvailableIndex", 0x00891920, NiDX9LightManager_GetNextAvailableIndex);
C40C_REVERSED("NiDX9LightEntry::Update", 0x00891980, NiDX9LightEntry_Update);
C40C_REVERSED("NiDX9LightManager::Reset", 0x00891BD0, NiDX9LightManager_Reset);
C40C_REVERSED("NiDX9LightManager::DisableLights", 0x00891CC0, NiDX9LightManager_DisableLights);
C40C_REVERSED("NiTPointerList<NiLight*>::AddHead_891D30", 0x00891D30, NiTPointerList_AddHead_891D30);
C40C_REVERSED("NiDX9LightEntry::NiDX9LightEntry", 0x00891D80, NiDX9LightEntry_Construct);
C40C_REVERSED("NiDX9LightManager::~NiDX9LightManager", 0x00891DB0, NiDX9LightManager_Destruct);
C40C_REVERSED("NiDX9LightManager::SetLights", 0x00891EB0, NiDX9LightManager_SetLights);
C40C_REVERSED("NiDX9LightManager::NiDX9LightManager", 0x008921D0, NiDX9LightManager_Construct);
C40C_REVERSED("NiDX9LightManager::SetState", 0x00892260, NiDX9LightManager_SetState);
C40C_REVERSED("NiDX9LightManager::RemoveLight", 0x00892410, NiDX9LightManager_RemoveLight);
C40C_REVERSED("NiDX9LightManager::ReleaseLights", 0x00892500, NiDX9LightManager_ReleaseLights);

} // namespace ni::c40
