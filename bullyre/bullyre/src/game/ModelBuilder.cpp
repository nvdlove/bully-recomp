// The player model builder's texture side ("ModelBuilder", 0x6D0B50..0x6D1CE7): a txd's
// textures loaded into a texture dictionary (renamed _d / _n / _s), tattoos composited
// onto the torso textures in a render target, the three "TORSOTORSOTORSO%d" textures put
// on every geometry of a node tree (base, normal and gloss maps), and the geometry array
// built from the body part models. Gamebryo objects by offset: NiRefObject's count at +4;
// NiTArray +4 data, +8 capacity, +0xA size, +0xE grow by; a texture dictionary starts
// with its NiTArray* of textures.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

// ---- game data ----
GAME_VAR(uint8_t*, mb_txdPool, 0x01B64654);            // CTxdStore's pool: +0 slots, +4 flags, +0xC slot size
GAME_VAR(uint8_t, mb_loadingScreenOn, 0x00BC7622);
GAME_VAR(void*, mb_renderedTexture, 0x020C7BDC);       // the compositing target (NiRenderedTexture)
GAME_VAR(void*, mb_renderTargetGroup, 0x020C7BD8);
GAME_VAR(void*, mb_renderer, 0x020E2380);
GAME_VAR(uint32_t, mb_sourceTextureRtti, 0x020E2250);   // (bit index: NiSourceTexture's)
GAME_VAR(uint32_t, mb_texturingRtti, 0x020E28C4);       // (NiTexturingProperty's)
GAME_VAR(uint32_t, mb_geometryRtti, 0x020E25BC);        // (NiGeometry's)
inline void* const mb_nodeRtti = reinterpret_cast<void*>(0x020E2548);   // NiNode::ms_RTTI
inline void* const mb_textureFormat = reinterpret_cast<void*>(0x00BC75B8);
GAME_VAR(uint32_t, mb_skinTxdHash, 0x020C7BCC);         // (the default skin's txd)
GAME_ARRAY(void*, mb_torsoTextures, 0x020C7BE0);        // [3] the last composited
GAME_VAR(char*, mb_emptyString, 0x00951028);
GAME_VAR(void*, mb_stringTable, 0x020E216C);
GAME_VAR(int32_t, mb_tempModel, 0x00A136D8);            // (the model id used to load a part)
GAME_ARRAY(uint8_t*, mb_modelInfoPtrs, 0x00C67738);
inline const char* const kSuffixes[3] = {reinterpret_cast<const char*>(0x0092D270),    // "_d"
                                          reinterpret_cast<const char*>(0x0092D268),    // "_n"
                                          reinterpret_cast<const char*>(0x0092D26C)};   // "_s"
constexpr uintptr_t VT_NiTObjectArrayNode = 0x00943144;
constexpr uintptr_t VT_NiTArrayNode = 0x0094313C;

GAME_FN(void, __thiscall, MB_NiTArray_SetAt, 0x00833C70, void* array, uint32_t index, re::Ptr32<void>* item);
GAME_FN(void, __thiscall, MB_Texturing_UpdateMap, 0x00765B00, void* property, void* map);
GAME_FN(void*, __thiscall, MB_GetProperty, 0x0075DC10, void* object, int type);
GAME_FN(void, __thiscall, MB_SetBaseTexture, 0x00407A60, void* property, void* texture);
GAME_FN(void, __thiscall, MB_Map_SetTexture, 0x00829350, void* map, void* texture);
GAME_FN(void*, __cdecl, MB_NiNew, 0x00752460, uint32_t size);
GAME_FN(void*, __thiscall, MB_Map_Construct, 0x00765BD0, void* map, void* texture, int uv, int clamp, int filter, uintptr_t);
GAME_FN(uint8_t*, __cdecl, MB_IsKindOf, 0x00401FF0, void* rtti, void* object);
GAME_FN(uint32_t, __cdecl, MB_RasterFromTexture, 0x0051A120, void* texture);
GAME_FN(uint8_t, __cdecl, MB_Render_GetFlag, 0x005EFAD0);
GAME_FN(void, __cdecl, MB_Render_SetFlag, 0x005EFAC0, int value);
GAME_FN(int32_t*, __cdecl, MB_GetDisplaySettings, 0x00405BA0, void* out);   // (+0 width, +4 height)
GAME_FN(void*, __cdecl, MB_RenderedTexture_Create, 0x0075D960, uint32_t width, uint32_t height, void* renderer,
        void* format, int flags);
GAME_FN(void*, __cdecl, MB_RenderTargetGroup_Create, 0x0075D720, void* buffer, void* renderer, int, int);
GAME_FN(void, __cdecl, MB_EndAndDisplayFrame, 0x004098D0);
GAME_FN(void, __cdecl, MB_BeginOffscreenFrame, 0x00409720);
GAME_FN(void, __cdecl, MB_Set2DStates, 0x005EE1D0);
GAME_FN(void, __cdecl, MB_BeginTargets, 0x004094E0, void* group, int clear);
GAME_FN(void, __cdecl, MB_Fn409960, 0x00409960);
GAME_FN(uint32_t, __cdecl, MB_RenderStateSet, 0x00519790, int state, uint32_t value);
GAME_FN(void, __cdecl, MB_DrawScreenQuad, 0x0073A550, int, int, void* texture);
GAME_FN(void, __cdecl, MB_EndTargets, 0x004096C0);
GAME_FN(void, __cdecl, MB_EndOffscreenFrame, 0x00409790);
GAME_FN(int32_t, __cdecl, MB_Txd_FindSlotByHash, 0x005F2450, uint32_t hash, int season);
GAME_FN(void, __cdecl, MB_RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, MB_RequestModelByHash, 0x005333E0, int id, uint32_t hash, int flags);
GAME_FN(void, __cdecl, MB_RemoveModel, 0x0052F640, int id);
GAME_FN(void, __cdecl, MB_LoadingScreen_Message, 0x0043C4F0, const char* title, const char* text, uintptr_t);
GAME_FN(void, __cdecl, MB_LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void*, __cdecl, MB_Dict_GetTexture, 0x005EE080, void* dict, int index);
GAME_FN(void, __thiscall, MB_Texture_GetNi, 0x00755430, void* texture, re::Ptr32<void>* out);
inline char* __cdecl MB_Strstr(const char* text, const char* part) { return crt::Strstr(text, part); }
template <typename... A>
inline int __cdecl MB_Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(char*, __cdecl, MB_FixedString_Create, 0x00754760, const char* s);
GAME_FN(void, __thiscall, MB_SetName, 0x0077C3A0, void* object, re::Ptr32<char>* name);
GAME_FN(void, __thiscall, MB_StringTable_Free, 0x00754550, void* table, re::Ptr32<char>* s, uint32_t length);
GAME_FN(void, __cdecl, MB_Dict_AddTexture, 0x005F28F0, void* dict, void* texture);
GAME_FN(void*, __thiscall, MB_Dict_FindTexture, 0x005F12C0, void* dict, const char* name);
GAME_FN(void, __thiscall, MB_NiTArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __thiscall, MB_NiTArray_SetAtGrow, 0x0040E800, void* array, uint32_t index, re::Ptr32<void>* item);
GAME_FN(void, __thiscall, MB_NiTArray_RemoveAt, 0x00818530, void* array, re::Ptr32<void>* out, uint32_t index);
GAME_FN(void, __thiscall, MB_NiTArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __cdecl, MB_Nothing, 0x0049A650);   // (an empty function)
GAME_FN(void, __cdecl, MB_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, MB_PopMemoryTag, 0x005EF100);
GAME_FN(void, __cdecl, MB_NiFree, 0x00836F80, void* p);
GAME_FN(void, __cdecl, MB_NiDelete, 0x00752540, void* p, uint32_t size);
GAME_FN(uint8_t*, __cdecl, MB_Clump_GetFirstAtomic, 0x005EDFF0, void* clump);
GAME_FN(uint8_t*, __cdecl, MB_Frame_FirstObject, 0x006C7700, void* frame);
GAME_FN(void*, __cdecl, MB_Fn6C7DD0, 0x006C7DD0, void* object, void* a, void* b);
GAME_FN(void*, __cdecl, MB_FrameCreate, 0x006CA210);
GAME_FN(void, __cdecl, MB_Frame_SetTable, 0x006C76D0, void* frame, void* table);
GAME_FN(void, __cdecl, MB_Frame_AddChild, 0x006C9D80, void* parent, void* child);
GAME_FN(uint8_t*, __cdecl, MB_ClumpCreate, 0x006C7540);
GAME_FN(void, __cdecl, MB_Clump_SetFrame, 0x006C7410, void* clump, void* frame);
GAME_FN(uint8_t*, __cdecl, MB_AtomicCreate, 0x006C6F50);
GAME_FN(void, __cdecl, MB_Atomic_SetGeometry, 0x006C6C60, void* atomic, void* geometry, int);   // (by NiPointer value)
GAME_FN(void, __cdecl, MB_Atomic_SetFrame, 0x006C6A10, void* atomic, void* frame);
GAME_FN(void, __cdecl, MB_Clump_AddAtomic, 0x006C74A0, void* clump, void* atomic);

// NiRefObject reference counting (+4), the game's way: InterlockedIncrement /
// Decrement, deleted through vtable slot 1 at zero
void AddRef(void* object) {
    vc::Increment(static_cast<uint8_t*>(object) + 4);
}
void Release(void* object) {
    if (vc::Decrement(static_cast<uint8_t*>(object) + 4) == 0) {
        vc::Call<void>(object, 4);
    }
}
// a NiPointer let go of (null allowed)
void ReleasePtr(void* object) {
    if (object != nullptr) {
        Release(object);
    }
}

// a NiFixedString let go of
void ReleaseFixedString(re::Ptr32<char>* s) {
    if (*s == mb_emptyString) {
        return;
    }
    uint32_t length = *reinterpret_cast<uint16_t*>(*s - 2);
    if (vc::Decrement(*s - 8) == 1) {
        MB_StringTable_Free(mb_stringTable, s, length);
    }
}

// sets an object's name
void SetName(void* object, const char* name) {
    re::Ptr32<char> fixed = MB_FixedString_Create(name);
    MB_SetName(object, &fixed);
    ReleaseFixedString(&fixed);
}

// NiDynamicCast by the RTTI bit set: `object` when its class has the bit
void* CastByBit(void* object, uint32_t bit) {
    if (object == nullptr) {
        return nullptr;
    }
    uint8_t* rtti = vc::Call<uint8_t*>(object, 8);
    uint32_t mask = (1u << (bit & 0x1F)) & At<uint32_t>(rtti, 0xC + (bit >> 5) * 4);
    return mask != 0 ? object : nullptr;
}

// the render target's size: 512, or 256 on a smaller screen
uint32_t TargetSize() {
    int32_t settings[6];
    int32_t* display = MB_GetDisplaySettings(settings);
    return (display[0] < 0x200 || display[1] < 0x200) ? 0x100 : 0x200;
}

// a rendered texture that has its buffer
bool Usable(void* texture) {
    if (texture == nullptr) {
        return false;
    }
    if (vc::Call<void*>(texture, 0x50) == nullptr) {
        return false;
    }
    return At<void*>(vc::Call<void*>(texture, 0x50), 0x10) != nullptr;
}

// a texturing property's map (index) replaced
void SetMap(uint8_t* property, int index, void* map) {
    void* old = At<re::Ptr32<void>*>(property, 0x20)[index];
    if (map != old) {
        if (old != nullptr) {
            vc::Call<void>(old, 0, 1);   // (the map's deleting destructor)
        }
        re::Ptr32<void> item = map;
        MB_NiTArray_SetAt(property + 0x1C, uint32_t(index), &item);
    }
    MB_Texturing_UpdateMap(property, map);
}

} // namespace

#define MODELBUILDER_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// 0x6D0B50 (thiscall): NiTexturingProperty::SetGlossMap (map 3)
void __fastcall Texturing_SetGlossMap(uint8_t* self, re::Edx, void* map) {
    SetMap(self, 3, map);
}
MODELBUILDER_REVERSED("NiTexturingProperty::SetGlossMap", 0x006D0B50, &Texturing_SetGlossMap);

// 0x6D0B90 (thiscall): NiTexturingProperty::SetNormalMap (map 6)
void __fastcall Texturing_SetNormalMap(uint8_t* self, re::Edx, void* map) {
    SetMap(self, 6, map);
}
MODELBUILDER_REVERSED("NiTexturingProperty::SetNormalMap", 0x006D0B90, &Texturing_SetNormalMap);

// 0x6D0BD0 (cdecl, `over` in eax): draws `over` on `under` into the render target and
// copies the result into `into` (a new rendered texture when null); returns it, or null
// when a target can't be made. (Reusing the target recreates it: the first time,
// mb_renderedTexture was null.)
void* ModelBuilder_Composite(void* over, void* under, void* into) {
    MB_RasterFromTexture(under);
    MB_RasterFromTexture(over);
    MB_RasterFromTexture(under);
    MB_RasterFromTexture(over);
    uint8_t savedFlag = MB_Render_GetFlag();
    MB_Render_SetFlag(1);
    bool hadTarget = false;
    if (mb_renderedTexture != nullptr) {
        hadTarget = true;
        vc::Call<void>(mb_renderedTexture, 0, 1);
        mb_renderedTexture = nullptr;
    }
    uint32_t size = TargetSize();
    mb_renderedTexture = MB_RenderedTexture_Create(size, size, mb_renderer, mb_textureFormat, 0x100);
    if (!Usable(mb_renderedTexture)) {
        return nullptr;   // (the render flag stays set)
    }
    if (hadTarget) {
        if (mb_renderTargetGroup != nullptr) {
            vc::Call<void>(mb_renderTargetGroup, 0, 1);
            mb_renderTargetGroup = nullptr;
        }
    } else if (mb_renderTargetGroup != nullptr) {
        goto haveGroup;
    }
    mb_renderTargetGroup =
        MB_RenderTargetGroup_Create(vc::Call<void*>(mb_renderedTexture, 0x50), mb_renderer, 1, 0);
    if (mb_renderTargetGroup == nullptr) {
        MB_Render_SetFlag(savedFlag);
        return nullptr;
    }
haveGroup:
    MB_EndAndDisplayFrame();
    MB_BeginOffscreenFrame();
    MB_Set2DStates();
    MB_BeginTargets(mb_renderTargetGroup, 7);
    MB_Fn409960();
    MB_RenderStateSet(6, 0);
    MB_RenderStateSet(0xC, 0);
    uint8_t* source = static_cast<uint8_t*>(CastByBit(At<void*>(over, 0x24), mb_sourceTextureRtti));
    MB_DrawScreenQuad(0, 0, At<void*>(source, 0x5C));
    MB_RenderStateSet(0xC, 1);
    MB_RenderStateSet(0xA, 5);
    MB_RenderStateSet(0xB, 6);
    source = static_cast<uint8_t*>(CastByBit(At<void*>(under, 0x24), mb_sourceTextureRtti));
    MB_DrawScreenQuad(0, 0, At<void*>(source, 0x5C));
    MB_EndTargets();
    MB_EndOffscreenFrame();
    if (into == nullptr) {
        size = TargetSize();
        into = MB_RenderedTexture_Create(size, size, mb_renderer, mb_textureFormat, 0x100);
        if (!Usable(into)) {
            MB_Render_SetFlag(savedFlag);
            return nullptr;
        }
    }
    void* renderer = mb_renderer;
    void* to = vc::Call<void*>(into, 0x50);
    void* from = vc::Call<void*>(mb_renderedTexture, 0x50);
    vc::Call<void>(renderer, 0xE8, from, to, 0, 0, 0);   // (a fast copy)
    MB_Render_SetFlag(savedFlag);
    return into;
}
RE_UNHOOKED_FN("ModelBuilder::Composite", 0x006D0BD0, &ModelBuilder_Composite);

// 0x6D0EB0 (thiscall): clears a pointer
void __fastcall ModelBuilder_ClearPointer(uint32_t* self, re::Edx) {
    *self = 0;
}
MODELBUILDER_REVERSED("ModelBuilder::ClearPointer", 0x006D0EB0, &ModelBuilder_ClearPointer);

namespace {
GAME_FN(void, __thiscall, MB_SetGlossMap, 0x006D0B50, void* property, void* map);
GAME_FN(void, __thiscall, MB_SetNormalMap, 0x006D0B90, void* property, void* map);
}

// 0x6D0EC0 (cdecl, the geometry in ecx, `normal` in ebx): a geometry's texturing gets
// the base texture, the normal map and the gloss map (maps made when missing)
void ModelBuilder_SetGeometryTextures(void* geometry, void* base, void* normal, void* gloss) {
    uint8_t* property = static_cast<uint8_t*>(CastByBit(MB_GetProperty(geometry, 8), mb_texturingRtti));
    if (base != nullptr) {
        MB_SetBaseTexture(property, base);
    }
    if (normal != nullptr) {
        void* map = At<re::Ptr32<void>*>(property, 0x20)[6];
        if (map != nullptr) {
            MB_Map_SetTexture(map, normal);
        } else {
            void* memory = MB_NiNew(0x10);
            void* created = memory != nullptr ? MB_Map_Construct(memory, normal, 0, 3, 1, 0) : nullptr;
            MB_SetNormalMap(property, created);
        }
    }
    if (gloss != nullptr) {
        void* map = At<re::Ptr32<void>*>(property, 0x20)[3];
        if (map != nullptr) {
            MB_Map_SetTexture(map, gloss);
            return;
        }
        void* memory = MB_NiNew(0x10);
        if (memory != nullptr) {
            MB_SetGlossMap(property, MB_Map_Construct(memory, gloss, 0, 3, 1, 0));
            return;
        }
        MB_SetGlossMap(property, nullptr);
    }
}
RE_UNHOOKED_FN("ModelBuilder::SetGeometryTextures", 0x006D0EC0, &ModelBuilder_SetGeometryTextures);

namespace {
GAME_FN(void, __cdecl, MB_SetNodeTextures, 0x006D0FA0, uint8_t* node, void* base, void* normal, void* gloss);
}

// 0x6D0FA0 (cdecl): the textures on every geometry under a node (a skinned object's
// partitions each, a geometry, or a node's children)
void __cdecl ModelBuilder_SetNodeTextures(uint8_t* node, void* base, void* normal, void* gloss) {
    uint16_t parts = At<uint16_t>(node, 0xBE);
    if (parts > 0) {
        for (uint32_t i = 0; i < At<uint16_t>(node, 0xBE); i++) {
            ModelBuilder_SetGeometryTextures(At<re::Ptr32<void>*>(node, 0xB8)[i], base, normal, gloss);
        }
        return;
    }
    void* geometry = CastByBit(node, mb_geometryRtti);
    if (geometry != nullptr) {
        ModelBuilder_SetGeometryTextures(geometry, base, normal, gloss);
        return;
    }
    uint8_t* parent = MB_IsKindOf(mb_nodeRtti, node);
    if (parent == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < At<uint16_t>(parent, 0xCE); i++) {
        uint8_t* child = At<uint16_t>(parent, 0xCE) > i ? At<re::Ptr32<uint8_t>*>(parent, 0xC8)[i] : nullptr;
        MB_SetNodeTextures(child, base, normal, gloss);
    }
}
MODELBUILDER_REVERSED("ModelBuilder::SetNodeTextures", 0x006D0FA0, &ModelBuilder_SetNodeTextures);

// 0x6D10A0 (cdecl, the txd's hash in eax): loads a txd and adds its textures to `dict`:
// only the first (single), or its _d / _n / _s ones renamed `rename` + "_d" / "_n" / "_s"
void ModelBuilder_LoadTexture(uint32_t txdHash, void* dict, const char* rename, int single) {
    int32_t slot = MB_Txd_FindSlotByHash(txdHash, -1);
    if (slot == -1) {
        return;
    }
    int32_t id = slot + 0x2FA8;
    MB_RequestModel(id, 9);
    uint8_t loadingScreen = mb_loadingScreenOn;
    mb_loadingScreenOn = 0;
    MB_LoadingScreen_Message("ModelBuilder - LoadTexture()", "CStreaming::LoadAllRequestedModels()", 0);
    MB_LoadAllRequestedModels(1);
    uint8_t* pool = mb_txdPool;
    mb_loadingScreenOn = loadingScreen;
    re::Ptr32<void>* txd = (At<uint8_t*>(pool, 4)[slot] & 0x80) != 0
                     ? nullptr
                     : reinterpret_cast<re::Ptr32<void>*>(At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * slot);
    void* rwDict = *txd;
    bool isSingle = re::Flag(single);
    void* found[3] = {nullptr, nullptr, nullptr};
    int count = isSingle ? 1 : 3;
    for (int i = 0; i < count; i++) {
        void* texture = MB_Dict_GetTexture(rwDict, i);
        if (texture == nullptr) {
            continue;
        }
        re::Ptr32<void> ni;
        MB_Texture_GetNi(texture, &ni);
        AddRef(ni);
        if (isSingle) {
            found[0] = ni;
        } else {
            const char* name = At<const char*>(ni, 8);
            if (MB_Strstr(name, "_d.") != nullptr || MB_Strstr(name, "_D.") != nullptr) {
                found[0] = ni;
            } else if (MB_Strstr(name, "_n.") != nullptr || MB_Strstr(name, "_N.") != nullptr) {
                found[1] = ni;
            } else if (MB_Strstr(name, "_s.") != nullptr || MB_Strstr(name, "_S.") != nullptr) {
                found[2] = ni;
            }
        }
        ReleasePtr(ni);
    }
    MB_RemoveModel(id);
    for (int i = 0; i < count; i++) {
        void* texture = found[i];
        if (texture == nullptr) {
            continue;
        }
        if (rename != nullptr) {
            char buffer[0x100];
            char* out = buffer;
            for (const char* in = rename; (*out++ = *in++) != 0;) {
            }
            out = buffer;
            while (*out != 0) {
                out++;
            }
            for (const char* in = kSuffixes[i]; (*out++ = *in++) != 0;) {
            }
            SetName(texture, buffer);
        }
        MB_Dict_AddTexture(dict, texture);
        Release(texture);
    }
}
RE_UNHOOKED_FN("ModelBuilder::LoadTexture", 0x006D10A0, &ModelBuilder_LoadTexture);

namespace {
// dict's texture array (its size, and entry i)
uint16_t DictSize(void* dict) {
    return At<uint16_t>(*static_cast<re::Ptr32<void>*>(dict), 0xA);
}
void* DictEntry(void* dict, int i) {
    return At<re::Ptr32<void>*>(*static_cast<re::Ptr32<void>*>(dict), 4)[i];
}
// the texture at `index` taken out of dict's array (and let go of)
void RemoveAndRelease(void* dict, uint32_t index) {
    re::Ptr32<void> removed;
    MB_NiTArray_RemoveAt(*static_cast<re::Ptr32<void>*>(dict), &removed, index);
    ReleasePtr(removed);
}
} // namespace

// 0x6D1370 (stdcall; ecx unused): loads the body parts' txds into dict; with the torso
// (part 1): the skin, the tattoos composited onto the torso's diffuse texture, and the
// three torso textures composited over the skin ("TORSOTORSOTORSO0..2"; with `reuse`,
// into the last ones made)
void __stdcall ModelBuilder_BuildTextures(void* dict, const uint32_t* partTxds, const uint32_t* tattooTxds,
                                          int tattooCount, void*, int partCount, void*, void*, int reuse) {
    for (int part = 0; part < partCount; part++) {
        if (partTxds[part] == 0) {
            continue;
        }
        ModelBuilder_LoadTexture(partTxds[part], dict, nullptr, 0);
        if (part != 1) {
            continue;
        }
        if (partTxds[1] != mb_skinTxdHash) {
            ModelBuilder_LoadTexture(mb_skinTxdHash, dict, "skinbase", 0);
        }
        char name[64];
        for (int i = 0; i < tattooCount; i++) {
            if (tattooTxds[i] == 0) {
                continue;
            }
            ModelBuilder_LoadTexture(tattooTxds[i], dict, "tattoo", 1);
            uint32_t n = DictSize(dict);
            void* tattoo = DictEntry(dict, int(n) - 1);
            void* torso = DictEntry(dict, int(n) - 4);
            if (tattoo == nullptr || torso == nullptr) {
                break;
            }
            void* made = ModelBuilder_Composite(torso, tattoo, i == 0 ? nullptr : torso);
            if (made == nullptr) {
                RemoveAndRelease(dict, n - 1);
                MB_Nothing();
                break;
            }
            AddRef(made);
            MB_Sprintf(name, "TORSOTORSOTORSO%d", 0);
            SetName(made, name);
            re::Ptr32<void> item = nullptr;
            MB_NiTArray_SetAtGrow(*static_cast<re::Ptr32<void>*>(dict), n - 4, &item);
            item = made;
            AddRef(made);
            MB_NiTArray_SetAtGrow(*static_cast<re::Ptr32<void>*>(dict), n - 4, &item);
            Release(made);
            RemoveAndRelease(dict, n - 1);
            MB_Nothing();
            Release(made);
        }
        if (partTxds[1] == mb_skinTxdHash) {
            continue;
        }
        void* made[3] = {nullptr, nullptr, nullptr};
        uint32_t n = DictSize(dict);
        for (int k = 0; k < 3; k++) {
            void* under = DictEntry(dict, int(n) - 6 + k);
            void* over = DictEntry(dict, int(n) - 3 + k);
            if (re::Flag(reuse) && mb_torsoTextures[k] != nullptr) {
                void* result = ModelBuilder_Composite(over, under, mb_torsoTextures[k]);
                if (made[k] != result) {
                    ReleasePtr(made[k]);
                    made[k] = result;
                    if (result != nullptr) {
                        AddRef(result);
                    }
                }
            } else {
                void* result = ModelBuilder_Composite(over, under, nullptr);
                void* old = made[k];
                mb_torsoTextures[k] = result;
                if (old != result) {
                    ReleasePtr(old);
                    made[k] = result;
                    if (result != nullptr) {
                        AddRef(result);
                    }
                }
            }
            if (made[k] == nullptr) {
                for (int j = k - 1; j >= 0; j--) {
                    if (made[j] != nullptr) {
                        Release(made[j]);
                        made[j] = nullptr;
                    }
                }
                MB_Nothing();
                for (int j = 2; j >= 0; j--) {
                    ReleasePtr(made[j]);
                }
                return;
            }
            MB_Sprintf(name, "TORSOTORSOTORSO%d", k);
            SetName(made[k], name);
        }
        for (int32_t index = int32_t(n) - 1; index >= int32_t(n) - 6; index--) {
            RemoveAndRelease(dict, uint32_t(index));
        }
        MB_Nothing();
        for (int k = 0; k < 3; k++) {
            MB_Dict_AddTexture(dict, made[k]);
        }
        for (int j = 2; j >= 0; j--) {
            ReleasePtr(made[j]);
        }
    }
}
MODELBUILDER_REVERSED("ModelBuilder::BuildTextures", 0x006D1370, &ModelBuilder_BuildTextures);

namespace {
GAME_FN(void, __stdcall, MB_BuildTextures, 0x006D1370, void* dict, const uint32_t* partTxds, const uint32_t* tattooTxds,
        int tattooCount, void*, int partCount, void*, void*, int reuse);

// the three torso textures from dict, put on the node's geometries
void ApplyTorso(void* dict, uint8_t* node) {
    char name[64];
    void* textures[3];
    for (int i = 0; i < 3; i++) {
        MB_Sprintf(name, "TORSOTORSOTORSO%d", i);
        textures[i] = MB_Dict_FindTexture(dict, name);
    }
    MB_SetNodeTextures(node, textures[0], textures[1], textures[2]);
}
} // namespace

// 0x6D1840 (thiscall; ecx unused): rebuilds the textures of a model already built and
// puts the torso ones on its root (+0x18 -> +8 -> [0] - 0x38)
void __fastcall ModelBuilder_Retexture(void*, re::Edx, uint8_t* model, void*, void* dict, void*, const uint32_t* partTxds,
                                       void* a6, int partCount, const uint32_t* tattooTxds, int tattooCount, void* a10,
                                       void* a11) {
    MB_PushMemoryTag(8);
    MB_NiTArray_RemoveAll(*static_cast<re::Ptr32<void>*>(dict));
    MB_Nothing();
    MB_BuildTextures(dict, partTxds, tattooTxds, tattooCount, a6, partCount, a10, a11, 0);
    uint8_t* node = At<uint8_t*>(*At<re::Ptr32<uint8_t>*>(At<uint8_t*>(model, 0x18), 8), -0x38);
    if (node != nullptr) {
        ApplyTorso(dict, node);
    }
    MB_PopMemoryTag();
}
MODELBUILDER_REVERSED("ModelBuilder::Retexture", 0x006D1840, &ModelBuilder_Retexture);

// 0x6D1910 (thiscall): NiTArray<NiPointer<NiNode>>'s deleting destructor
void* __fastcall NiNodeArray_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = VT_NiTArrayNode;
    MB_NiFree(At<void*>(self, 4));
    if ((flags & 1) != 0) {
        MB_NiDelete(self, 0x10);
    }
    return self;
}
MODELBUILDER_REVERSED("NiTArray<NiPointer<NiNode>>::DeletingDestructor", 0x006D1910, &NiNodeArray_DeletingDestructor);

// 0x6D1940 (thiscall; ecx unused): the geometry of each part model (loaded by hash
// through the temp model id, using txd `txd`) added to the array; null for the parts
// without one
void __fastcall ModelBuilder_ConstructGeometryArray(void*, re::Edx, uint8_t* array, const uint32_t* partModels,
                                                    int count, int16_t txd) {
    for (int i = 0; i < count; i++) {
        uint32_t hash = partModels[i];
        if (hash != 0) {
            MB_RequestModelByHash(mb_tempModel, hash, 9);
            uint8_t* mi = mb_modelInfoPtrs[mb_tempModel];
            int16_t savedTxd = At<int16_t>(mi, 0x16);
            At<int16_t>(mi, 0x16) = txd;
            MB_LoadingScreen_Message("ModelBuilder::ConstructGeometryArray", "CStreaming::LoadAllRequestedModels()", 0);
            MB_LoadAllRequestedModels(1);
            if (At<void*>(mi, 0x20) != nullptr) {
                uint8_t* atomic = MB_Clump_GetFirstAtomic(At<void*>(mi, 0x20));
                uint32_t size = At<uint16_t>(array, 0xA);
                if (At<uint16_t>(array, 8) <= size) {
                    MB_NiTArray_SetSize(array, At<uint16_t>(array, 0xE) + size);
                }
                MB_NiTArray_SetAtGrow(array, size, reinterpret_cast<re::Ptr32<void>*>(atomic + 0x14));
                MB_Frame_FirstObject(At<void*>(atomic, 4));
                MB_RemoveModel(mb_tempModel);
                At<int16_t>(mi, 0x16) = savedTxd;
                continue;
            }
        }
        uint32_t size = At<uint16_t>(array, 0xA);
        re::Ptr32<void> none = nullptr;
        if (At<uint16_t>(array, 8) <= size) {
            MB_NiTArray_SetSize(array, At<uint16_t>(array, 0xE) + size);
        }
        MB_NiTArray_SetAtGrow(array, size, &none);
    }
}
MODELBUILDER_REVERSED("ModelBuilder::ConstructGeometryArray", 0x006D1940, &ModelBuilder_ConstructGeometryArray);

namespace {
GAME_FN(void, __thiscall, MB_ConstructGeometryArray, 0x006D1940, void* self, uint8_t* array, const uint32_t* partModels,
        int count, int txd);

// a NiTObjectArray<NiPointer<NiNode>> on the stack
struct NodeArray {
    uint32_t vtable;   // +0
    re::Ptr32<re::Ptr32<void>> data;        // +4
    uint16_t capacity;  // +8
    uint16_t size;      // +0xA
    uint16_t esize;     // +0xC
    uint16_t growBy;    // +0xE
};
static_assert(sizeof(NodeArray) == 0x10);
} // namespace

// 0x6D1A60 (thiscall; ecx unused): builds the player's clump: the textures into dict,
// the part geometries (with dict's txd) as atomics under a new frame tree copied from
// `skeleton`'s, the torso textures on the torso; returns the clump
uint8_t* __fastcall ModelBuilder_Build(void*, re::Edx, void* skeleton, void* dict, const uint32_t* partModels,
                                       const uint32_t* partTxds, void* a5, int partCount, const uint32_t* tattooTxds,
                                       int tattooCount, void* a9, void* a10) {
    MB_PushMemoryTag(8);
    MB_NiTArray_RemoveAll(*static_cast<re::Ptr32<void>*>(dict));
    MB_Nothing();
    MB_BuildTextures(dict, partTxds, tattooTxds, tattooCount, a5, partCount, a9, a10, 0);
    int32_t txd = -1;
    for (int32_t i = 0; i < 10000; i++) {
        uint8_t* pool = mb_txdPool;
        if ((At<uint8_t*>(pool, 4)[i] & 0x80) == 0) {
            re::Ptr32<void>* slot = reinterpret_cast<re::Ptr32<void>*>(At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * i);
            if (slot != nullptr && *slot == dict) {
                txd = i;
                break;
            }
        }
    }
    NodeArray array;
    array.capacity = 0;
    array.growBy = 1;
    array.size = 0;
    array.esize = 0;
    array.data = nullptr;
    array.vtable = VT_NiTObjectArrayNode;
    MB_ConstructGeometryArray(nullptr, reinterpret_cast<uint8_t*>(&array), partModels, partCount, txd);
    if ((partTxds[1] != mb_skinTxdHash || tattooCount > 0) && array.data[1] != nullptr) {
        ApplyTorso(dict, static_cast<uint8_t*>(array.data[1]));
    }
    uint8_t* atomic = MB_Clump_GetFirstAtomic(skeleton);
    uint8_t* object = MB_Frame_FirstObject(At<void*>(atomic, 4));
    void* table = MB_Fn6C7DD0(object, At<void*>(object, 0), At<void*>(At<uint8_t*>(object, 0x1C), 0x20));
    void* frame = MB_FrameCreate();
    MB_Frame_SetTable(frame, table);
    void* root = MB_FrameCreate();
    MB_Frame_AddChild(root, frame);
    uint8_t* clump = MB_ClumpCreate();
    MB_Clump_SetFrame(clump, root);
    for (uint32_t i = 0; i < array.size; i++) {
        if (array.data[i] == nullptr) {
            continue;
        }
        uint8_t* part = MB_AtomicCreate();
        void* geometry = array.data[i];
        if (geometry != nullptr) {
            AddRef(geometry);
        }
        MB_Atomic_SetGeometry(part, geometry, 0);
        MB_Atomic_SetFrame(part, frame);
        MB_Frame_SetTable(At<void*>(part, 4), table);
        MB_Clump_AddAtomic(clump, part);
        float* bounds = reinterpret_cast<float*>(At<uint8_t*>(part, 0x14) + 0x1C);
        bounds[0] = 0.0f;
        bounds[1] = 0.0f;
        bounds[2] = 0.0f;
        bounds[3] = 10000.0f;
    }
    MB_NiTArray_RemoveAll(&array);
    MB_PopMemoryTag();
    MB_NiFree(array.data);
    return clump;
}
MODELBUILDER_REVERSED("ModelBuilder::Build", 0x006D1A60, &ModelBuilder_Build);
