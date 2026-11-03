// The immediate-mode 3D state (globals at 0xDA7518..0xDA753C): start-up, the pending
// transform (vertices, count, matrix, flags), its end, a few flags, and the shutdown of
// the two object pointer arrays. 0x5EFA70..0x5EFB50.

#include <cstdint>

#include "reversed.h"
#include "game/VCall.h"

#include "game/Crt.h"
namespace {

GAME_VAR(uint8_t, im_queueForAlpha, 0x00DA7518);
GAME_VAR(uint8_t, im_drawNow, 0x00DA7519);
GAME_VAR(uint8_t, im_started, 0x00DA751A);
GAME_VAR(uint32_t, im_value1C, 0x00DA751C);
GAME_VAR(uint32_t, im_value20, 0x00DA7520);
GAME_VAR(uint16_t, im_value24, 0x00DA7524);
GAME_VAR(uint32_t, im_value28, 0x00DA7528);
GAME_VAR(uint32_t, im_value2C, 0x00DA752C);
GAME_VAR(void*, im_vertices, 0x00DA7530);
GAME_VAR(int32_t, im_count, 0x00DA7534);
GAME_VAR(uint32_t, im_flags, 0x00DA7538);
GAME_VAR(void*, im_transform, 0x00DA753C);
inline re::Ptr32<void>* const im_objects = reinterpret_cast<re::Ptr32<void>*>(0x01B62F40);   // 0x400
inline re::Ptr32<void>* const im_objects2 = reinterpret_cast<re::Ptr32<void>*>(0x01B63F40);  // 0xC8
inline uint8_t* const im_table = reinterpret_cast<uint8_t*>(0x00D14558); // 0x800
inline void* const im_identity = reinterpret_cast<void*>(0x00B0A38C);

GAME_FN(void, __thiscall, IM_TransformMakeIdentity, 0x00770920, void* transform);
inline void* __cdecl IM_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }

// an object pointer released (its vf1 when the count drops to zero) and cleared
void ReleaseSlot(re::Ptr32<void>* slot) {
    uint8_t* object = static_cast<uint8_t*>(*slot);
    if (!object) {
        return;
    }
    if (vc::Decrement(object + 4) == 0) {
        using Fn = void(__thiscall*)(void*);
        re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[1])(object);
    }
    *slot = nullptr;
}

// 0x5EFA70: started once: the identity transform at 0xB0A38C made and the line flags
// (0xD14558) all 1
void __cdecl Start() {
    if (im_started) {
        return;
    }
    im_started = 1;
    IM_TransformMakeIdentity(im_identity);
    IM_Memset(im_table, 1, 0x800);
}

// 0x5EFAA0: the counters cleared
void __cdecl Clear() {
    im_value20 = 0;
    im_value24 = 0;
    im_value28 = 0;
    im_value2C = 0;
    im_value1C = 0;
}

// 0x5EFAC0
void __cdecl SetDrawNow(uint8_t value) {
    im_drawNow = value;
}

// 0x5EFAD0
uint8_t __cdecl GetDrawNow() {
    return im_drawNow;
}

// 0x5EFAE0
void __cdecl SetQueueForAlpha(uint8_t value) {
    im_queueForAlpha = value;
}

// 0x5EFAF0: the pending transform (flags with 0x18); returns the vertices
void* __cdecl Transform(void* vertices, int32_t count, void* transform, uint32_t flags) {
    im_count = count;
    im_vertices = vertices;
    im_flags = flags | 0x18;
    im_transform = transform;
    return vertices;
}

// 0x5EFB20: the pending transform ended (whether there was one)
uint32_t __cdecl End() {
    if (!im_vertices) {
        return 0;
    }
    im_vertices = nullptr;
    return 1;
}

// 0x5EFB50: when started: the two object arrays released
void __cdecl Shutdown() {
    if (!im_started) {
        return;
    }
    im_started = 0;
    for (re::Ptr32<void>* slot = im_objects; slot < im_objects + 0x400; slot++) {
        ReleaseSlot(slot);
    }
    for (re::Ptr32<void>* slot = im_objects2; slot < im_objects2 + 0xC8; slot++) {
        ReleaseSlot(slot);
    }
}

#define IM3DSTATE_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Im3DStateOriginal_, __LINE__);                                         \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Im3DStateOriginal_, __LINE__), \
             re::Mode::Original)

IM3DSTATE_REVERSED("Im3DState::Start", 0x005EFA70, &Start);
IM3DSTATE_REVERSED("Im3DState::Clear", 0x005EFAA0, &Clear);
IM3DSTATE_REVERSED("Im3DState::SetDrawNow", 0x005EFAC0, &SetDrawNow);
IM3DSTATE_REVERSED("Im3DState::GetDrawNow", 0x005EFAD0, &GetDrawNow);
IM3DSTATE_REVERSED("Im3DState::SetQueueForAlpha", 0x005EFAE0, &SetQueueForAlpha);
IM3DSTATE_REVERSED("Im3DState::Transform", 0x005EFAF0, &Transform);
IM3DSTATE_REVERSED("Im3DState::End", 0x005EFB20, &End);
IM3DSTATE_REVERSED("Im3DState::Shutdown", 0x005EFB50, &Shutdown);

} // namespace
