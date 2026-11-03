// The stop signs (StopSigns.dat): placed objects of kind 3, each with an area a car
// stops in (0x45B030..0x45B180).

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct StopSign {
    re::Ptr32<uint8_t> object;   // +0 (its +0x20: the area it covers)
    re::Ptr32<void> ref;         // +4 a registered reference
};
static_assert(sizeof(StopSign) == 8);

struct StopSigns {
    re::Ptr32<StopSign> signs;   // +0
    uint32_t count;    // +4
};
static_assert(sizeof(StopSigns) == 8);

// the placed objects' store: +0 its CPool
inline void* const ss_placed = reinterpret_cast<void*>(0x020C7C48);
GAME_VAR(uint32_t, ss_timeInMs, 0x00C1A9B4);

GAME_FN(void, __thiscall, Placed_Load, 0x006D3C70, void* self, const char* file, int kind);
GAME_FN(void, __thiscall, Placed_Unload, 0x006D2210, void* self, int kind);
GAME_FN(void*, __cdecl, Alloc, 0x005EEA50, size_t size, int);
GAME_FN(void, __cdecl, AllocFree, 0x005EEA60, void* p);
GAME_FN(void, __thiscall, Ref_Set, 0x005D7F10, re::Ptr32<void>* ref, void* entity);
GAME_FN(float*, __thiscall, Entity_GetPosition, 0x00415460, void* entity);
GAME_FN(uint8_t, __thiscall, Area_Contains, 0x006D6EB0, void* area, const float* point);
GAME_FN(void, __cdecl, Vehicle_Fn41A150, 0x0041A150, void* vehicle);
inline int __cdecl Rand() { return crt::Rand(); }

// 0x45B030 (thiscall): let go of the signs and unload them
void __fastcall StopSigns_Shutdown(StopSigns* self) {
    if (!self->count) {
        return;
    }
    AllocFree(self->signs);
    Placed_Unload(ss_placed, 3);
}

// 0x45B060 (thiscall): the vehicle is in a sign's area: it waits there (800 ms, a bike
// 1200..3199 ms) the first time it comes into it. 1: stopping
uint8_t __fastcall StopSigns_ShouldStop(StopSigns* self, re::Edx, uint8_t* vehicle) {
    if (!self->count) {
        return 0;
    }
    re::Ptr32<StopSign>& lastSign = *reinterpret_cast<re::Ptr32<StopSign>*>(vehicle + 0x370);
    for (uint32_t i = 0; i < self->count; i++) {
        StopSign* sign = &self->signs[i];
        uint8_t* object = sign->object;
        if (!*reinterpret_cast<re::Ptr32<void>*>(object + 0x20)) {
            continue;
        }
        float y = Entity_GetPosition(vehicle)[1];
        float point[2] = {Entity_GetPosition(vehicle)[0], y};
        if (Area_Contains(object, point)) {
            if (vehicle[0x2DB] == 1) {
                Vehicle_Fn41A150(vehicle);
            }
            if (lastSign == sign) {
                return 0;
            }
            if (*reinterpret_cast<int32_t*>(vehicle + 0x46C) == 1) {
                vehicle[0x2DB] = 0xB;
                uint32_t now = ss_timeInMs;
                int r = Rand();
                lastSign = sign;
                *reinterpret_cast<uint32_t*>(vehicle + 0x2DC) = uint32_t(r % 2000) + now + 1200;
                return 1;
            }
            vehicle[0x2DB] = 1;
            lastSign = sign;
            *reinterpret_cast<uint32_t*>(vehicle + 0x2DC) = ss_timeInMs + 800;
            return 1;
        }
        if (lastSign == sign) {
            lastSign = nullptr;
        }
    }
    lastSign = nullptr;
    return 0;
}

// 0x45B180 (thiscall): load StopSigns.dat, and keep every placed object of kind 3
void __fastcall StopSigns_Setup(StopSigns* self) {
    Placed_Load(ss_placed, reinterpret_cast<const char*>(0x0090ACDC), 3);   // "StopSigns.dat"
    uint8_t* pool = *static_cast<re::Ptr32<uint8_t>*>(ss_placed);
    int32_t size = *reinterpret_cast<int32_t*>(pool + 8);
    auto at = [pool](int slot) -> uint8_t* {
        uint8_t* flags = *reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4);
        if (flags[slot] & 0x80) {
            return nullptr;
        }
        return *reinterpret_cast<re::Ptr32<uint8_t>*>(pool) + *reinterpret_cast<int32_t*>(pool + 0xC) * slot;
    };
    self->count = 0;
    // (slot 0 skipped)
    for (int slot = 1; slot < size; slot++) {
        uint8_t* object = at(slot);
        if (object && object[0x3D] == 3) {
            self->count++;
        }
    }
    if (!self->count) {
        return;
    }
    StopSign* signs = static_cast<StopSign*>(Alloc(self->count * 8, 0));
    self->count = 0;
    self->signs = signs;
    for (int slot = 1; slot < size; slot++) {
        uint8_t* object = at(slot);
        if (object && object[0x3D] == 3) {
            StopSign* sign = &self->signs[self->count];
            if (sign) {
                sign->object = object;
                sign->ref = nullptr;
                Ref_Set(&sign->ref, nullptr);
            }
            self->count++;
        }
    }
}

// 0x45B075: 0x45B060's loop (Ghidra's split: entered with eax, ecx and the caller's frame)
RE_UNHOOKED("StopSigns::ShouldStop_loop", 0x0045B075);

#define C04E_STOP(name, address, fn)                                                           \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04E_STOP("StopSigns::Shutdown", 0x0045B030, &StopSigns_Shutdown);
C04E_STOP("StopSigns::ShouldStop", 0x0045B060, &StopSigns_ShouldStop);
C04E_STOP("StopSigns::Setup", 0x0045B180, &StopSigns_Setup);

} // namespace
