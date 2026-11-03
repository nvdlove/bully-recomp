// An area whose objects' models are kept requested (a centre, a radius and four flags
// at 0xBD09E4..0xBD09FB), and the one-byte block at 0xBD09E0 made and freed with it.
// 0x4298A0..0x429C04.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#define STREAMAREA_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

GAME_VAR(void*, sa_block, 0x00BD09E0);
GAME_VAR(float, sa_radius, 0x00BD09E4);
GAME_VAR(uint8_t, sa_requestModels, 0x00BD09E8);   // the area's objects' models requested (0x429990)
GAME_VAR(uint8_t, sa_flag9, 0x00BD09E9);
GAME_VAR(uint8_t, sa_flagA, 0x00BD09EA);
GAME_VAR(uint8_t, sa_flagB, 0x00BD09EB);
GAME_VAR(uint8_t, sa_active, 0x00BD09EC);
GAME_VAR(float, sa_x, 0x00BD09F0);
GAME_VAR(float, sa_y, 0x00BD09F4);
GAME_VAR(float, sa_z, 0x00BD09F8);
GAME_VAR(uint8_t, sa_specialEntitiesCreated, 0x00CA3984);
GAME_VAR(uint32_t*, sa_ptrNodes, 0x00C0F788);      // CPools::ms_pPtrNodes
inline uint8_t* const sa_sectors = reinterpret_cast<uint8_t*>(0x00C1B178);  // CWorld's 36 x 36, 5 lists each

GAME_FN(void*, __cdecl, SA_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, SA_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, SA_GetEntityInPool, 0x0044A3E0, uint32_t pool, uint32_t slot);
GAME_FN(int, __cdecl, SA_SetupEntityVisibility, 0x00451660, void* entity);   // (CRenderer's: 4 VIS_STREAMME)
GAME_FN(void, __cdecl, SA_RequestModel, 0x00532020, int model, int flags);

// (the game truncates to 32 bits, then keeps 16)
uint16_t SectorIndex(double v) {
    return uint16_t(int32_t(v / 50.0 + 18.0));
}

} // namespace

namespace WorldStreamArea {

// 0x4298A0: the one-byte block made, if it isn't
void __cdecl CreateBlock() {
    if (!sa_block) {
        sa_block = SA_Malloc(1);
    }
}

// 0x4298C0: the block freed
void __cdecl DeleteBlock() {
    if (sa_block) {
        SA_Free(sa_block);
        sa_block = nullptr;
    }
}

// 0x4298E0: the area set (and on)
void __cdecl Set(float x, float y, float z, int flagA, int flag9, int requestModels, int flagB, float radius) {
    sa_radius = radius;
    sa_x = x;
    sa_y = y;
    sa_z = z;
    sa_flagB = uint8_t(flagB);
    sa_active = 1;
    sa_flagA = uint8_t(flagA);
    sa_flag9 = uint8_t(flag9);
    sa_requestModels = uint8_t(requestModels);
}

// 0x429940: the area off (its flag B back to 1)
void __cdecl Clear() {
    sa_active = 0;
    sa_x = 0.0f;
    sa_y = 0.0f;
    sa_z = 0.0f;
    sa_flagB = 1;
    sa_flagA = 0;
    sa_flag9 = 0;
    sa_requestModels = 0;
}

// 0x429990: the models of the entities in the sectors the area covers that ask to be
// streamed (CRenderer::SetupEntityVisibility's VIS_STREAMME: the extra area's, with its models
// flag) requested (unless the special entities are made). (The indices are clamped to
// -900..900 as if they were positions: a negative index, wrapped to 16 bits, isn't)
void __cdecl RequestAreaModels() {
    if (!sa_requestModels) {
        return;
    }
    double x = sa_x;
    double r = sa_radius;
    uint32_t xs = SectorIndex(x - r);
    uint32_t xe = SectorIndex(r + x);
    double y = sa_y;
    uint32_t ys = SectorIndex(y - r);
    uint32_t ye = SectorIndex(y + r);
    if (double(xs) < -900.0) {
        xs = 0xFC7C;
    }
    if (double(xe) > 900.0) {
        xe = 900;
    }
    if (double(ys) < -900.0) {
        ys = 0xFC7C;
    }
    if (double(ye) > 900.0) {
        ye = 900;
    }
    for (uint32_t sy = uint16_t(ys); int(sy) <= int(uint16_t(ye)); sy++) {
        for (uint32_t sx = uint16_t(xs); int(sx) <= int(uint16_t(xe)); sx++) {
            re::Ptr32<uint32_t>* lists = reinterpret_cast<re::Ptr32<uint32_t>*>(sa_sectors + (sy * 36 + sx) * 0x14);
            for (int list = 0; list < 5; list++) {
                uint32_t* node = lists[list];
                while (node) {
                    uint32_t bits = *node;
                    void* entity = SA_GetEntityInPool(bits & 0xF, (bits >> 4) & 0x3FFF);
                    uint32_t next = *node >> 18;
                    node = next == 0x3FFF ? nullptr : &sa_ptrNodes[next];
                    if (SA_SetupEntityVisibility(entity) == 4 && !sa_specialEntitiesCreated) {
                        SA_RequestModel(*reinterpret_cast<int16_t*>(static_cast<uint8_t*>(entity) + 0x10E), 0);
                    }
                }
            }
        }
    }
}

STREAMAREA_REVERSED("WorldStreamArea::CreateBlock", 0x004298A0, &CreateBlock);
STREAMAREA_REVERSED("WorldStreamArea::DeleteBlock", 0x004298C0, &DeleteBlock);
STREAMAREA_REVERSED("WorldStreamArea::Set", 0x004298E0, &Set);
STREAMAREA_REVERSED("WorldStreamArea::Clear", 0x00429940, &Clear);
STREAMAREA_REVERSED("WorldStreamArea::RequestAreaModels", 0x00429990, &RequestAreaModels);

} // namespace WorldStreamArea
