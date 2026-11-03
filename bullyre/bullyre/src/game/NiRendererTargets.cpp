// 0x4076F0 to 0x407C50: Gamebryo template code instantiated in the game: NiTArray and
// NiTPointerList helpers, NiRenderer's render target group begin/end (its lock), and the
// small property and extra data classes' constructors and destructors.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_FN(void*, __cdecl, NiMalloc, 0x00752300, size_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __cdecl, DebugPrint, 0x0049A650, const char* format);   // (does nothing)
GAME_FN(void, __thiscall, NiProperty_Construct, 0x00764550, void* self);
GAME_FN(void, __thiscall, NiProperty_Destruct, 0x00764590, void* self);
GAME_FN(void, __thiscall, NiExtraData_Destruct, 0x00765670, void* self);
GAME_FN(void, __thiscall, ShaderMap_Destruct, 0x00765C30, void* self);
GAME_FN(void*, __thiscall, TexturingMap_Construct, 0x00765BA0, void* self);
GAME_FN(void, __thiscall, NiTArray_SetAt, 0x00833C70, void* self, uint32_t index, re::Ptr32<void>* item);
GAME_FN(void, __thiscall, Renderer_UnlockTargets, 0x00407830, void* self);
GAME_FN(uint8_t, __thiscall, Renderer_BeginUsingRenderTargetGroup, 0x00407890, void* self, void* group, uint32_t flags);
GAME_VAR(uint32_t, ms_rtti_20E2250, 0x020E2250);
GAME_VAR(uint8_t*, ms_pRenderTargetAC, 0x00BCBBAC);

uint8_t* IsKindOfBit(void* object, uint32_t bit) {
    uint8_t* rtti = vc::Call<uint8_t*>(object, 0x08);
    uint32_t bits = *reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4);
    return (bits & (1u << (bit & 0x1F))) ? static_cast<uint8_t*>(object) : nullptr;
}

uint32_t& U(void* p, int offset) {
    return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + offset);
}
uint16_t& W(void* p, int offset) {
    return *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(p) + offset);
}

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

// NiRenderer: +0x30 its render target lock (CRITICAL_SECTION; +0x48 owner, +0x4C count),
// +0x90 "a target group in use", +0x94 the frame state (1, 2 or 4: inside a frame)
bool InFrame(uint8_t* renderer) {
    int32_t state = *reinterpret_cast<int32_t*>(renderer + 0x94);
    return state == 1 || state == 2 || state == 4;
}

void Unlock(uint8_t* renderer) {
    if (U(renderer, 0x4C) == 1) {
        vc::Call<void>(renderer, 0x144);
    }
    if (--U(renderer, 0x4C) == 0) {
        U(renderer, 0x48) = 0;
    }
    vc::Import<void>(vc::kLeaveCriticalSection, renderer + 0x30);
}

} // namespace

namespace NiRendererTargets {

// 0x4076F0: the object's +0x24 as the class 0x20E2250's, its +0x5C (0 otherwise)
static uint32_t __cdecl GetOwner5C(uint8_t* object) {
    void* p = *reinterpret_cast<re::Ptr32<void>*>(object + 0x24);
    if (p) {
        uint8_t* q = IsKindOfBit(p, ms_rtti_20E2250);
        if (q) {
            return U(q, 0x5C);
        }
    }
    return 0;
}

// 0x407740 (thiscall): NiTArray<T*>::Compact: the nulls squeezed out and the array
// reallocated to the item count (+4 the items, +8 the size, +0xA the used size, +0xC the
// item count)
static void __fastcall NiTArray_Compact(uint8_t* self, re::Edx) {
    uint16_t count = W(self, 0xC);
    if (count == W(self, 0xA)) {
        return;
    }
    if (count != 0) {
        uint16_t to = 0;
        for (uint16_t from = 0; from < W(self, 0xA); from++) {
            re::Ptr32<void>* items = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 4);
            void* item = items[from];
            if (item) {
                if (items[to] != item) {
                    items[to] = item;
                }
                to++;
            }
        }
    }
    count = W(self, 0xC);
    re::Ptr32<void>* old = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 4);
    W(self, 0xA) = count;
    W(self, 8) = count;
    if (count == 0) {
        *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 4) = nullptr;
        NiFree(old);
        return;
    }
    re::Ptr32<void>* items = static_cast<re::Ptr32<void>*>(NiMalloc(uint32_t(count) * 4));
    *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 4) = items;
    for (uint16_t i = 0; i < W(self, 0xA); i++) {
        items[i] = old[i];
    }
    NiFree(old);
}

// 0x407800 (thiscall): NiAlphaProperty's constructor (flags 0xEC, threshold 0)
static uint8_t* __fastcall AlphaProperty_Construct(uint8_t* self, re::Edx) {
    NiProperty_Construct(self);
    U(self, 0) = 0x008FFB9C;
    self[0x1A] = 0;
    W(self, 0x18) = 0xEC;
    return self;
}

// 0x407820: NiAlphaProperty's RTTI
static void* __cdecl AlphaProperty_GetRTTI() {
    return reinterpret_cast<void*>(0x020E2AF0);
}

// 0x407830 (thiscall): the render target lock released
static void __fastcall UnlockTargets(uint8_t* self, re::Edx) {
    Unlock(self);
}

// 0x407860 (thiscall): vtable[85] passed through, inside a frame with a group in use (or
// state 4). (A void function: eax is left as the frame state otherwise)
static uint32_t __fastcall ClearIfUsingGroup(uint8_t* self, re::Edx, uint32_t a, uint32_t b) {
    uint32_t state = U(self, 0x94);
    if (InFrame(self) && (self[0x90] == 1 || state == 4)) {
        return vc::Call<uint32_t>(self, 0x154, a, b);
    }
    return state;
}

// 0x407890 (thiscall): BeginUsingRenderTargetGroup: under the lock (its first taker calls
// vtable[80]), vtable[89]; the lock let go again when that fails
static uint8_t __fastcall BeginUsingRenderTargetGroup(uint8_t* self, re::Edx, void* group, uint32_t flags) {
    if (!InFrame(self) || (self[0x90] && U(self, 0x94) != 4)) {
        return 0;
    }
    if (!group) {
        DebugPrint("NiRenderer::BeginUsingRenderTargetGroup> Failed - NULL pointer for render target. Call "
                   "BeginUsingDefaultRenderTargetGroup if you intended to use the backbuffer instead.");
        return 0;
    }
    vc::Import<void>(vc::kEnterCriticalSection, self + 0x30);
    uint32_t thread = vc::Import<uint32_t>(vc::kGetCurrentThreadId);
    U(self, 0x4C)++;
    U(self, 0x48) = thread;
    if (U(self, 0x4C) == 1) {
        vc::Call<void>(self, 0x140);
    }
    uint8_t ok = vc::Call<uint8_t>(self, 0x164, group, flags);
    if (ok) {
        self[0x90] = 1;
        return ok;
    }
    Renderer_UnlockTargets(self);
    return 0;
}

// 0x407940 (thiscall): BeginUsingDefaultRenderTargetGroup
static uint8_t __fastcall BeginUsingDefaultRenderTargetGroup(uint8_t* self, re::Edx, uint32_t flags) {
    if (InFrame(self) && (!self[0x90] || U(self, 0x94) == 4)) {
        void* group = vc::Call<void*>(self, 0x74);   // GetDefaultRenderTargetGroup
        return Renderer_BeginUsingRenderTargetGroup(self, group, flags);
    }
    return 0;
}

// 0x407990 (thiscall): EndUsingRenderTargetGroup
static uint8_t __fastcall EndUsingRenderTargetGroup(uint8_t* self, re::Edx) {
    if (InFrame(self) && (self[0x90] == 1 || U(self, 0x94) == 4)) {
        if (vc::Call<uint8_t>(self, 0x168)) {
            Unlock(self);
            self[0x90] = 0;
            return 1;
        }
    }
    return 0;
}

// the extra data and property classes' deleting destructors
static uint8_t* __fastcall BooleanExtraData_Destroy(uint8_t* self, re::Edx, int flags) {   // 0x407A00
    U(self, 0) = 0x008FFA24;
    NiExtraData_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}
static uint8_t* __fastcall ShaderMap_Destroy(uint8_t* self, re::Edx, int flags) {   // 0x407A30
    U(self, 0) = 0x008FFA78;
    ShaderMap_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x14);
    }
    return self;
}
static uint8_t* __fastcall FloatExtraData_Destroy(uint8_t* self, re::Edx, int flags) {   // 0x407AE0
    U(self, 0) = 0x008FFA8C;
    NiExtraData_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}
static uint8_t* __fastcall IntegerExtraData_Destroy(uint8_t* self, re::Edx, int flags) {   // 0x407B10
    U(self, 0) = 0x008FFAE4;
    NiExtraData_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x407A60 (thiscall): the base map's texture set (the map made when there's none)
static void __fastcall SetBaseTexture(uint8_t* self, re::Edx, uint8_t* texture) {
    void* map = **reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 0x20);
    if (!map) {
        void* p = NiNew(0x10);
        map = p ? TexturingMap_Construct(p) : nullptr;
        re::Ptr32<void> item = map;
        NiTArray_SetAt(self + 0x1C, 0, &item);
    }
    re::Ptr32<uint8_t>& current = *reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<uint8_t*>(map) + 8);
    if (current == texture) {
        return;
    }
    Release(current);
    current = texture;
    if (texture) {
        vc::Increment(texture + 4);
    }
}

// 0x407B40 (thiscall): NiZBufferProperty's constructor (test and write on, less-equal)
static uint8_t* __fastcall ZBufferProperty_Construct(uint8_t* self, re::Edx) {
    NiProperty_Construct(self);
    *reinterpret_cast<float*>(self + 0x1C) = 0.0f;
    U(self, 0) = 0x008FFCA4;
    *reinterpret_cast<float*>(self + 0x20) = 0.0f;
    U(self, 0x24) = 0;
    W(self, 0x18) = 0xF;
    *reinterpret_cast<float*>(self + 0x28) = 1.0f;
    *reinterpret_cast<float*>(self + 0x2C) = 1.0f;
    return self;
}

// 0x407B70, 0x407B80: NiZBufferProperty's RTTI and type
static void* __cdecl ZBufferProperty_GetRTTI() {
    return reinterpret_cast<void*>(0x020E2B28);
}
static uint32_t __cdecl ZBufferProperty_GetType() {
    return 0xB;
}

// 0x407B90 (thiscall): its deleting destructor
static uint8_t* __fastcall ZBufferProperty_Destroy(uint8_t* self, re::Edx, int flags) {
    NiProperty_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x30);
    }
    return self;
}

// 0x407BB0: a new reference to the render target group at 0xBCBBAC
static re::Ptr32<uint8_t>* __cdecl GetRenderTargetAC(re::Ptr32<uint8_t>* out) {
    uint8_t* target = ms_pRenderTargetAC;
    *out = target;
    if (target) {
        vc::Increment(target + 4);
    }
    return out;
}

// 0x407BD0 (thiscall): NiTPointerList::AddHead (+4 head, +8 tail, +0xC count; a node
// is next, previous, element)
static void __fastcall PointerList_AddHead(uint8_t* self, re::Edx, const re::Ptr32<uint8_t>* element) {
    uint8_t* node = vc::Call<uint8_t*>(self, 0x04);   // a new node
    re::Ptr32<uint8_t>& held = *reinterpret_cast<re::Ptr32<uint8_t>*>(node + 8);
    if (held != *element) {
        Release(held);
        held = *element;
        if (held) {
            vc::Increment(held + 4);
        }
    }
    U(node, 4) = 0;
    U(node, 0) = U(self, 4);
    uint8_t* head = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 4);
    if (head) {
        *reinterpret_cast<re::Ptr32<uint8_t>*>(head + 4) = node;
        U(self, 0xC)++;
        *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 4) = node;
        return;
    }
    U(self, 0xC)++;
    *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 8) = node;
    *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 4) = node;
}

// 0x407C50 (thiscall): NiTArray::UpdateSize: trailing nulls dropped from the used size
static void __fastcall NiTArray_UpdateSize(uint8_t* self, re::Edx) {
    while (W(self, 0xA) != 0) {
        if ((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 4))[W(self, 0xA) - 1] != nullptr) {
            return;
        }
        W(self, 0xA)--;
    }
}

} // namespace NiRendererTargets

// ---- registration ----------------------------------------------------------------------

#define C00_TARGETS_REVERSED(name, fn, address)                                                     \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace NiRendererTargets {
C00_TARGETS_REVERSED("NiRendererTargets::GetOwner5C", GetOwner5C, 0x004076F0);
C00_TARGETS_REVERSED("NiTArray::Compact", NiTArray_Compact, 0x00407740);
C00_TARGETS_REVERSED("NiAlphaProperty::Construct", AlphaProperty_Construct, 0x00407800);
C00_TARGETS_REVERSED("NiAlphaProperty::GetRTTI", AlphaProperty_GetRTTI, 0x00407820);
C00_TARGETS_REVERSED("NiRenderer::UnlockTargets", UnlockTargets, 0x00407830);
C00_TARGETS_REVERSED("NiRenderer::ClearIfUsingGroup", ClearIfUsingGroup, 0x00407860);
C00_TARGETS_REVERSED("NiRenderer::BeginUsingRenderTargetGroup", BeginUsingRenderTargetGroup, 0x00407890);
C00_TARGETS_REVERSED("NiRenderer::BeginUsingDefaultRenderTargetGroup", BeginUsingDefaultRenderTargetGroup, 0x00407940);
C00_TARGETS_REVERSED("NiRenderer::EndUsingRenderTargetGroup", EndUsingRenderTargetGroup, 0x00407990);
C00_TARGETS_REVERSED("NiBooleanExtraData::Destroy", BooleanExtraData_Destroy, 0x00407A00);
C00_TARGETS_REVERSED("NiTexturingProperty::ShaderMap::Destroy", ShaderMap_Destroy, 0x00407A30);
C00_TARGETS_REVERSED("NiTexturingProperty::SetBaseTexture", SetBaseTexture, 0x00407A60);
C00_TARGETS_REVERSED("NiFloatExtraData::Destroy", FloatExtraData_Destroy, 0x00407AE0);
C00_TARGETS_REVERSED("NiIntegerExtraData::Destroy", IntegerExtraData_Destroy, 0x00407B10);
C00_TARGETS_REVERSED("NiZBufferProperty::Construct", ZBufferProperty_Construct, 0x00407B40);
C00_TARGETS_REVERSED("NiZBufferProperty::GetRTTI", ZBufferProperty_GetRTTI, 0x00407B70);
C00_TARGETS_REVERSED("NiZBufferProperty::GetType", ZBufferProperty_GetType, 0x00407B80);
C00_TARGETS_REVERSED("NiZBufferProperty::Destroy", ZBufferProperty_Destroy, 0x00407B90);
C00_TARGETS_REVERSED("NiRendererTargets::GetRenderTargetAC", GetRenderTargetAC, 0x00407BB0);
C00_TARGETS_REVERSED("NiTPointerList::AddHead", PointerList_AddHead, 0x00407BD0);
C00_TARGETS_REVERSED("NiTArray::UpdateSize", NiTArray_UpdateSize, 0x00407C50);
} // namespace NiRendererTargets
