// Gamebryo code compiled into the game's first object files (0x4029B0 to 0x402E70):
// NiShadowMap and NiShadowCubeMap's destructors, the NiUI signal slots, NiActionData's
// deleting destructor, a bounded sprintf, and small NiPointer setters.

#include <cstdarg>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, size_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __thiscall, LightList_Add, 0x007843D0, void* list, re::Ptr32<uint8_t>* light);
GAME_FN(void, __thiscall, UIBaseSlot0_Unsubscribe, 0x00404130, void* slot, void* signal);
GAME_FN(void, __thiscall, NiUIBaseSlot0_Destruct, 0x00402CC0, void* slot);
GAME_FN(void, __thiscall, NiShadowCubeMap_Destruct, 0x00402A20, void* self);
inline int __cdecl vsnprintf_s_(char* buffer, size_t size, size_t count, const char* format, va_list args) { return crt::VsnprintfS(buffer, size, count, format, args); }
GAME_FN(uint8_t*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __thiscall, Fn_40E7A0, 0x0040E7A0, void* self);
GAME_VAR(uint8_t*, ms_pShadowManager, 0x020E2410);

constexpr uint32_t kNiShadowMapVtable = 0x008FF368;
constexpr uint32_t kNiShadowCubeMapVtable = 0x008FF374;
constexpr uint32_t kNiActionDataVtable = 0x008FF22C;
constexpr uint32_t kNiUIBaseSlot0Vtable = 0x008FF380;
constexpr uint32_t kNiUIMemberSlot0Vtable = 0x008FF38C;

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);   // DeleteThis
    }
}

uint32_t& U(uint8_t* p, int offset) {
    return *reinterpret_cast<uint32_t*>(p + offset);
}
re::Ptr32<uint8_t>& P(uint8_t* p, int offset) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(p + offset);
}

} // namespace

namespace NiShadowMap {

// 0x4029B0: NiShadowMap's RTTI
static void* __cdecl GetRTTI() {
    return reinterpret_cast<void*>(0x020E23BC);
}

// 0x4029C0 (thiscall): the deleting destructor (its texture +0xC and target +8 released)
static uint8_t* __fastcall Destroy(uint8_t* self, re::Edx, int flags) {
    U(self, 0) = kNiShadowMapVtable;
    Release(P(self, 0xC));
    Release(P(self, 8));
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x402C00 (thiscall): the destructor of an NiShadowMap held at +8 of its owner
static void __fastcall DestructMember(uint8_t* owner, re::Edx) {
    uint8_t* self = owner + 8;
    U(self, 0) = kNiShadowMapVtable;
    Release(P(self, 0xC));
    Release(P(self, 8));
}

} // namespace NiShadowMap

namespace NiShadowCubeMap {

// 0x402A20 (thiscall): the destructor: the six faces' targets (+0x10..+0x24, last
// first), then NiShadowMap's
static void __fastcall Destruct(uint8_t* self, re::Edx) {
    U(self, 0) = kNiShadowCubeMapVtable;
    for (int i = 5; i >= 0; i--) {
        Release(P(self, 0x10 + i * 4));
    }
    U(self, 0) = kNiShadowMapVtable;
    Release(P(self, 0xC));
    Release(P(self, 8));
}

// 0x402AB0: NiShadowCubeMap's RTTI
static void* __cdecl GetRTTI() {
    return reinterpret_cast<void*>(0x020E257C);
}

// 0x402AC0 (thiscall): the deleting destructor
static uint8_t* __fastcall Destroy(uint8_t* self, re::Edx, int flags) {
    NiShadowCubeMap_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x28);
    }
    return self;
}

} // namespace NiShadowCubeMap

namespace NiShadowManager {

// 0x402AE0: the shadow manager's texture `index` (its NiPointer array at +0xC8) set
static void __cdecl SetTexture(uint8_t* texture, int index) {
    re::Ptr32<uint8_t>& p = P(ms_pShadowManager, 0xC8 + index * 4);
    if (p == texture) {
        return;
    }
    Release(p);
    p = texture;
    if (texture) {
        vc::Increment(texture + 4);
    }
}

// 0x402B40 (thiscall): a light added to the list at +8 (held by a temporary NiPointer)
static void __fastcall AddLight(uint8_t* self, re::Edx, uint8_t* light) {
    re::Ptr32<uint8_t> held = light;
    if (light) {
        vc::Increment(light + 4);
    }
    LightList_Add(self + 8, &held);
    if (light && vc::Decrement(light + 4) == 0) {
        vc::Call<void>(light, 0x04);
    }
}

// 0x402B90 (thiscall): the totals over the list at +0xC (next at +0, the object at +8)
// of its vtable[5] (count), [6] and [7] (floats)
static void __fastcall SumStats(uint8_t* self, re::Edx, int* count, float* a, float* b) {
    *count = 0;
    *b = 0.0f;
    *a = 0.0f;
    for (uint8_t* node = P(self, 0xC); node;) {
        uint8_t* object = P(node, 8);
        node = P(node, 0);
        *count += vc::Call<int>(object, 0x14);
        *a = float(vc::Call<float>(object, 0x18) + *a);
        *b = float(vc::Call<float>(object, 0x1C) + *b);
    }
}

} // namespace NiShadowManager

namespace NiActionData {

// 0x402C50 (thiscall): the vector deleting destructor (0x1C each, the count before the
// array when flags & 2)
static uint8_t* __fastcall Destroy(uint8_t* self, re::Edx, int flags) {
    if (flags & 2) {
        uint8_t* block = self - 4;
        int count = *reinterpret_cast<int*>(block);
        uint8_t* p = self + count * 0x1C;
        while (--count >= 0) {
            p -= 0x1C;
            U(p, 0) = kNiActionDataVtable;
        }
        if (flags & 1) {
            NiFreeArray(block, 0x1C);
        }
        return block;
    }
    U(self, 0) = kNiActionDataVtable;
    if (flags & 1) {
        NiFreeSized(self, 0x1C);
    }
    return self;
}

} // namespace NiActionData

namespace NiUISlot {

// NiUIBaseSlot0: +4 the signals it's subscribed to (array), +0xC their count. A signal:
// +4 its slots (array), +0xC their count

// 0x402CC0 (thiscall): the destructor: unsubscribed from every signal (each removes it
// from its slots by moving its last one in), then the array freed
static void __fastcall Destruct(uint8_t* self, re::Edx) {
    U(self, 0) = kNiUIBaseSlot0Vtable;
    while (U(self, 0xC) != 0) {
        uint8_t* signal = *re::PtrCast<re::Ptr32<uint8_t>*>(P(self, 4));
        uint32_t count = U(signal, 0xC);
        re::Ptr32<uint8_t>* slots = re::PtrCast<re::Ptr32<uint8_t>*>(P(signal, 4));
        for (uint32_t i = 0; i < count; i++) {
            if (slots[i] == self) {
                U(signal, 0xC)--;
                slots = re::PtrCast<re::Ptr32<uint8_t>*>(P(signal, 4));
                slots[i] = slots[U(signal, 0xC)];
                UIBaseSlot0_Unsubscribe(self, signal);
                break;
            }
        }
        // (a signal not listing the slot loops for ever: the game's)
    }
    NiFree(P(self, 4));
}

// 0x402D30 (thiscall): a destructor: the NiPointer at +0xC released, the NiFixedString at
// +0x10 released, then +0xC's again (already null by then)
static void __fastcall DestructSample(uint8_t* self, re::Edx) {
    uint8_t* p = P(self, 0xC);
    if (p) {
        Release(p);
        P(self, 0xC) = nullptr;
    }
    char* s = re::PtrCast<char*>(P(self, 0x10));
    if (s) {
        uint16_t length = *reinterpret_cast<uint16_t*>(s - 2);
        if (vc::Decrement(s - 8) == 1) {
            reinterpret_cast<void(__thiscall*)(void*, re::Ptr32<char>*, uint16_t)>(0x00754550)(
                *reinterpret_cast<re::Ptr32<void>*>(0x020E216C), reinterpret_cast<re::Ptr32<char>*>(self + 0x10), length);
        }
    }
    Release(P(self, 0xC));
}

// 0x402DB0 (thiscall): NiUIMemberSlot0<NiSample>'s ReceiveSignal: the member function
// (+0x18) on the object (+0x1C) adjusted by +0x10
static void __fastcall ReceiveSignal(uint8_t* self, re::Edx) {
    void* object = P(self, 0x1C) + U(self, 0x10);
    re::PtrCast<void(__thiscall*)(void*)>(P(self, 0x18))(object);
}

// 0x402DC0 (thiscall): NiUIMemberSlot0<NiSample>'s deleting destructor
static uint8_t* __fastcall Destroy(uint8_t* self, re::Edx, int flags) {
    U(self, 0x10) = 0;
    U(self, 0x1C) = 0;
    U(self, 0) = kNiUIMemberSlot0Vtable;
    U(self, 0x18) = 0;
    NiUIBaseSlot0_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x20);
    }
    return self;
}

} // namespace NiUISlot

namespace GbMisc {

// 0x402E00: sprintf_s with truncation (_vsnprintf_s, _TRUNCATE); 0 for an empty buffer
static int __cdecl FormatString(char* buffer, size_t size, const char* format, ...) {
    if (size == 0) {
        return 0;
    }
    va_list args;
    va_start(args, format);
    buffer[0] = 0;
    int n = vsnprintf_s_(buffer, size, size_t(-1), format, args);
    va_end(args);
    return n;
}

// 0x402E30 (thiscall): the node's children's +0x18 cleared, then 0x40E7A0 on +0xC4
static void __fastcall ClearChildren18(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < *reinterpret_cast<uint16_t*>(self + 0xCE); i++) {
        uint8_t* child = re::PtrCast<re::Ptr32<uint8_t>*>(P(self, 0xC8))[i];
        if (child) {
            U(child, 0x18) = 0;
        }
    }
    Fn_40E7A0(self + 0xC4);
}

// 0x402E70 (a virtual; `this` unused): MdShaderConstantManagerFactory's Create
static void* __fastcall CreateShaderConstantManager(void*, re::Edx, void* renderer, void* caps) {
    uint8_t* p = NiNew(0x8C);
    if (!p) {
        return nullptr;
    }
    return reinterpret_cast<void*(__thiscall*)(void*, void*, void*)>(0x00402190)(p, renderer, caps);
}

} // namespace GbMisc

// ---- registration ----------------------------------------------------------------------

#define C00_GBI_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace NiShadowMap {
C00_GBI_REVERSED("NiShadowMap::GetRTTI", GetRTTI, 0x004029B0);
C00_GBI_REVERSED("NiShadowMap::Destroy", Destroy, 0x004029C0);
C00_GBI_REVERSED("NiShadowMap::DestructMember", DestructMember, 0x00402C00);
} // namespace NiShadowMap
namespace NiShadowCubeMap {
C00_GBI_REVERSED("NiShadowCubeMap::Destruct", Destruct, 0x00402A20);
C00_GBI_REVERSED("NiShadowCubeMap::GetRTTI", GetRTTI, 0x00402AB0);
C00_GBI_REVERSED("NiShadowCubeMap::Destroy", Destroy, 0x00402AC0);
} // namespace NiShadowCubeMap
namespace NiShadowManager {
C00_GBI_REVERSED("NiShadowManager::SetTexture", SetTexture, 0x00402AE0);
C00_GBI_REVERSED("NiShadowManager::AddLight", AddLight, 0x00402B40);
C00_GBI_REVERSED("NiShadowManager::SumStats", SumStats, 0x00402B90);
} // namespace NiShadowManager
namespace NiActionData {
C00_GBI_REVERSED("NiActionData::Destroy", Destroy, 0x00402C50);
} // namespace NiActionData
namespace NiUISlot {
C00_GBI_REVERSED("NiUIBaseSlot0::Destruct", Destruct, 0x00402CC0);
C00_GBI_REVERSED("NiUISlot::DestructSample", DestructSample, 0x00402D30);
C00_GBI_REVERSED("NiUIMemberSlot0<NiSample>::ReceiveSignal", ReceiveSignal, 0x00402DB0);
C00_GBI_REVERSED("NiUIMemberSlot0<NiSample>::Destroy", Destroy, 0x00402DC0);
} // namespace NiUISlot
namespace GbMisc {
C00_GBI_REVERSED("GbMisc::FormatString", FormatString, 0x00402E00);
C00_GBI_REVERSED("GbMisc::ClearChildren18", ClearChildren18, 0x00402E30);
C00_GBI_REVERSED("MdShaderConstantManagerFactory::Create", CreateShaderConstantManager, 0x00402E70);
} // namespace GbMisc
