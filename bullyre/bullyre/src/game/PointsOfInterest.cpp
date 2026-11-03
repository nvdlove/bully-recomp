// The points of interest: CStaticPOI's type filter and the POI pool (0xA4-byte points,
// the pool at 0x20C7C84). 0x458330..0x4583F0.

#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_VAR(CPool<uint8_t>*, poiPool, 0x020C7C84);

GAME_FN(uint8_t, __thiscall, POI_vf1, 0x004476F0, void* self, uintptr_t arg);   // CPOI's
inline void* __cdecl Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }

// ours, through the game
GAME_FN(void*, __thiscall, PoiPool_NewCall, 0x00458360, CPool<uint8_t>* pool);

} // namespace

namespace poi {

// 0x458330 (thiscall): CStaticPOI::vf1: not for types 3..15 (else CPOI's)
uint8_t __fastcall StaticPOI_vf1(uint8_t* self, re::Edx, int arg) {
    if (arg == 0) {
        return 0;
    }
    int32_t type = *reinterpret_cast<int32_t*>(self + 0x70);
    if (type == -1 || (type > 2 && type < 16)) {
        return 0;
    }
    return POI_vf1(self, arg);
}

// 0x458360 (thiscall): CPool::New for the POIs: the next free slot from the last one
// used, round once; zeroed
void* __fastcall PoiPool_New(CPool<uint8_t>* self, re::Edx) {
    uint8_t* flags = self->flags;
    bool wrapped = false;
    if (!(flags[self->lastUsed] & 0x80)) {
        do {
            self->lastUsed++;
            if (self->lastUsed >= self->size) {
                if (wrapped) {
                    return nullptr;
                }
                self->lastUsed = 0;
                wrapped = true;
            }
        } while (!(flags[self->lastUsed] & 0x80));
    }
    flags[self->lastUsed] &= 0x7F;
    // (the reuse count in the low 7 bits, one up)
    uint8_t& flag = self->flags[self->lastUsed];
    flag = uint8_t(flag ^ (((flag + 1) ^ flag) & 0x7F));
    void* point = self->objects + self->elemSize * self->lastUsed;
    Memset(point, 0, 0xA4);
    return point;
}

// 0x4583E0: PoiPointsLoaderImpl: a new point from the pool
void* __cdecl NewPoint() {
    return PoiPool_NewCall(poiPool);
}

// 0x4583F0: a point back to the pool
void __cdecl DeletePoint(void* point) {
    CPool<uint8_t>* pool = poiPool;
    int index = pool->GetIndex(static_cast<uint8_t*>(point));
    pool->flags[index] |= 0x80;
    if (index < pool->lastUsed) {
        pool->lastUsed = index;
    }
}

} // namespace poi

// ---- registration ---------------------------------------------------------------------

#define C04D_POI_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04D_POI_REVERSED("CStaticPOI::vf1", 0x00458330, &poi::StaticPOI_vf1);
C04D_POI_REVERSED("CPool<POI>::New", 0x00458360, &poi::PoiPool_New);
C04D_POI_REVERSED("PoiPointsLoaderImpl::NewPoint", 0x004583E0, &poi::NewPoint);
C04D_POI_REVERSED("CPool<POI>::Delete", 0x004583F0, &poi::DeletePoint);
