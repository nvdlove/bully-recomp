// The player's per-frame stats (0x446BC0, thiscall on an object whose +0 is the ped): the
// vehicle stunts (bikes: kind 0, both of 0x77C/0x780 at 1 with 0x774/0x778 below it; the
// other kind 1 ones on 0x634/0x638), each timed (ms, with a 500 ms grace in +0x80) and
// its distance summed, the best kept; the shown counter (+0x4C) stepping to +0x48; the road
// density round the ped every 16 frames; a count while the player rides upside down; the
// player's movement since the last frame.
//
// +0x48 the counter, +0x4C its shown value, +0x60 the upside down count, +0x64/+0x68 the
// first stunt's time and distance (+0x84/+0x88 its best, from 2 s), +0x6C a time below
// 3 of 0x84E, +0x70/+0x74 the second (+0x8C/+0x90, from 5 s), +0x78/+0x7C the third
// (+0x94/+0x98, from 2 s), +0x80 the grace, +0xA0 the road density factor, +0xC8 the
// player's last position, +0xD4 its movement (normalised), +0xE0 its length times
// 0.02 and the time step.

#include <cmath>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace {

GAME_VAR(uint32_t, ms_frameCounter, 0x00C1A99C);
GAME_VAR(float, ms_timeStep, 0x00C1A9A4);
GAME_VAR(uint8_t*, ms_pPlayerPed, 0x00C1AEA8);
GAME_VAR(float, ms_moveScale, 0x00A14550);
inline void* const ms_paths = reinterpret_cast<void*>(0x00BF6228);

GAME_FN(int, __cdecl, GetFrameMs, 0x00446870);
GAME_FN(float, __cdecl, MaxF, 0x00414490, float a, float b);
GAME_FN(float, __thiscall, Paths_CalcRoadDensity, 0x00441330, void* paths, float x, float y);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, void* v);
GAME_FN(void, __cdecl, Fn_45AA60, 0x0045AA60);
GAME_FN(void, __thiscall, Stats_Fn_446500, 0x00446500, void* self);
GAME_FN(void, __thiscall, Stats_Fn_445E90, 0x00445E90, void* self);
GAME_FN(col::CVector*, __cdecl, VectorSub, 0x00414B70, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, col::CVector* v);

using re::At;

col::CVector* EntityPos(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<col::CVector*>(matrix + 0x30) : reinterpret_cast<col::CVector*>(entity + 4);
}

// the grace time down by the frame's time times k (not below 0)
void DecayGrace(uint8_t* self, double k) {
    float grace = float(At<uint32_t>(self, 0x80));
    uint32_t ms = uint32_t(GetFrameMs());
    float value = float(double(grace) - double(ms) * k);
    At<uint32_t>(self, 0x80) = uint32_t(int64_t(MaxF(0.0f, value)));
}

} // namespace

namespace PlayerStunts {

// 0x446BC0 (thiscall)
static void __fastcall Update(uint8_t* self, re::Edx) {
    uint8_t* vehicle = At<uint8_t*>(At<uint8_t*>(self, 0), 0x1554);
    bool clearSecond = true;   // (+0x70, +0x78 cleared on the way out)
    if (!vehicle || (At<int32_t>(vehicle, 0x46C) != 0 && At<int32_t>(vehicle, 0x46C) != 1)) {
        At<uint32_t>(self, 0x80) = 0;
        At<uint32_t>(self, 0x6C) = 0;
        At<uint32_t>(self, 0x64) = 0;
    } else if (At<int32_t>(vehicle, 0x46C) == 0) {
        if (vehicle[0x84E] < 3) {
            At<uint32_t>(self, 0x6C) += GetFrameMs();
        } else {
            At<uint32_t>(self, 0x6C) = 0;
        }
        bool grace;
        if (At<float>(vehicle, 0x77C) == 1.0f && At<float>(vehicle, 0x780) == 1.0f) {
            if (1.0f > At<float>(vehicle, 0x774) && 1.0f > At<float>(vehicle, 0x778) && At<float>(vehicle, 0x1A0) == 0.0f) {
                At<uint32_t>(self, 0x64) += GetFrameMs();
                At<float>(self, 0x68) = At<float>(vehicle, 0x19C) + At<float>(self, 0x68);
                DecayGrace(self, 0.5);
                grace = false;
                goto end;
            }
            grace = true;
        } else {
            // (both at 1: the same test again, which fails)
            grace = At<float>(vehicle, 0x774) == 1.0f && At<float>(vehicle, 0x778) == 1.0f;
        }
        {
            uint32_t n = At<uint32_t>(self, 0x64);
            if (grace) {
                if (n != 0 && At<uint32_t>(self, 0x80) < 500) {
                    uint32_t old = At<uint32_t>(self, 0x80);
                    At<uint32_t>(self, 0x80) = GetFrameMs() + old;
                    goto end;
                }
            } else if (n == 0) {
                goto end;
            }
            if (n >= 2000) {
                At<uint32_t>(self, 0x84) = n;
                At<float>(self, 0x88) = At<float>(self, 0x68);
            }
            At<float>(self, 0x68) = 0.0f;
            At<uint32_t>(self, 0x64) = 0;
            At<uint32_t>(self, 0x80) = 0;
        }
    } else {
        clearSecond = false;
        if (At<float>(vehicle, 0x634) == 1.0f) {
            if (1.0f > At<float>(vehicle, 0x638) && At<float>(vehicle, 0x1A0) == 0.0f) {
                At<uint32_t>(self, 0x70) += GetFrameMs();
                At<float>(self, 0x74) = At<float>(vehicle, 0x19C) + At<float>(self, 0x74);
                At<uint32_t>(self, 0x64) = 0;
                At<uint32_t>(self, 0x6C) = 0;
                DecayGrace(self, 0.20000000298023224);
                goto tail;
            }
            uint32_t n = At<uint32_t>(self, 0x70);
            if (n != 0 && At<uint32_t>(self, 0x80) < 500) {
                uint32_t old = At<uint32_t>(self, 0x80);
                At<uint32_t>(self, 0x80) = GetFrameMs() + old;
                At<uint32_t>(self, 0x64) = 0;
                At<uint32_t>(self, 0x6C) = 0;
                goto tail;
            }
        } else if (At<uint32_t>(self, 0x70) == 0) {
            if (At<float>(vehicle, 0x638) == 1.0f && At<float>(vehicle, 0x1A0) == 0.0f) {
                At<uint32_t>(self, 0x78) += GetFrameMs();
                At<float>(self, 0x7C) = At<float>(vehicle, 0x19C) + At<float>(self, 0x7C);
                At<uint32_t>(self, 0x64) = 0;
                At<uint32_t>(self, 0x6C) = 0;
                DecayGrace(self, 0.20000000298023224);
                goto tail;
            }
            uint32_t n3 = At<uint32_t>(self, 0x78);
            if (n3 != 0 && At<uint32_t>(self, 0x80) < 500) {
                uint32_t old = At<uint32_t>(self, 0x80);
                At<uint32_t>(self, 0x80) = GetFrameMs() + old;
                At<uint32_t>(self, 0x64) = 0;
                At<uint32_t>(self, 0x6C) = 0;
                goto tail;
            }
            if (n3 >= 2000) {
                At<uint32_t>(self, 0x94) = n3;
                At<float>(self, 0x98) = At<float>(self, 0x7C);
            }
            At<float>(self, 0x7C) = 0.0f;
            At<uint32_t>(self, 0x78) = 0;
            At<uint32_t>(self, 0x80) = 0;
            At<uint32_t>(self, 0x64) = 0;
            At<uint32_t>(self, 0x6C) = 0;
            goto tail;
        }
        uint32_t n = At<uint32_t>(self, 0x70);
        if (n >= 5000) {
            At<uint32_t>(self, 0x8C) = n;
            At<float>(self, 0x90) = At<float>(self, 0x74);
        }
        At<float>(self, 0x74) = 0.0f;
        At<uint32_t>(self, 0x70) = 0;
        At<uint32_t>(self, 0x80) = 0;
        At<uint32_t>(self, 0x64) = 0;
        At<uint32_t>(self, 0x6C) = 0;
        goto tail;
    }
end:
    if (clearSecond) {
        At<uint32_t>(self, 0x70) = 0;
        At<uint32_t>(self, 0x78) = 0;
    }
tail:
    // the shown counter towards the counter (by 12345, 1234, 123, 42 or 1)
    int32_t shown = At<int32_t>(self, 0x4C);
    int32_t target = At<int32_t>(self, 0x48);
    if (shown != target) {
        int32_t diff = target - shown;
        int32_t away = int32_t(int64_t(std::fabs(float(diff))));
        int32_t step;
        if (away > 100000) {
            step = 12345;
        } else if (away > 10000) {
            step = 1234;
        } else if (away > 1000) {
            step = 123;
        } else {
            step = away <= 50 ? 1 : 42;
        }
        At<int32_t>(self, 0x4C) = diff < 0 ? shown - step : shown + step;
    }
    if (!(ms_frameCounter & 0xF)) {
        uint8_t* ped = At<uint8_t*>(self, 0);
        uint8_t* v = At<uint8_t*>(ped, 0x1554);
        col::CVector* pos = EntityPos(v ? v : ped);
        At<float>(self, 0xA0) = Paths_CalcRoadDensity(ms_paths, pos->x, pos->y);
    }
    float density = float((double(At<float>(self, 0xA0)) - 1.0) * 0.6000000238418579 + 1.0);
    At<float>(self, 0xA0) = density;
    if (density < 0.5f) {
        density = 0.5f;
    }
    At<float>(self, 0xA0) = density;
    if (density > 1.45f) {
        density = 1.45f;
    }
    At<float>(self, 0xA0) = density;
    if (!(ms_frameCounter & 0x1F)) {
        uint8_t* player = ms_pPlayerPed;
        bool upsideDown = false;
        if (player && At<int32_t>(player, 0x1310) == 0xD) {
            uint8_t* bike = At<uint8_t*>(player, 0x1554);
            if (bike && At<void*>(At<uint8_t*>(self, 0), 0x1554) && 0.0f > Entity_GetMatrix(bike)[10] &&
                Vector_Magnitude(bike + 0x124) < 0.05f && At<int32_t>(bike, 0x46C) == 0 && !(bike[0x1BC] & 0x10)) {
                upsideDown = true;
                if (-0.5f > Entity_GetMatrix(bike)[10]) {
                    At<int32_t>(self, 0x60) += 2;
                } else {
                    At<int32_t>(self, 0x60) += 1;
                }
            }
        }
        if (!upsideDown) {
            At<int32_t>(self, 0x60) = 0;
        }
    }
    Fn_45AA60();
    float count = float(At<int32_t>(self, 0x48));
    float capped = 1000000000.0f > count ? count : 1000000000.0f;
    At<int32_t>(self, 0x48) = int32_t(int64_t(capped));
    float shownF = float(At<int32_t>(self, 0x4C));
    capped = shownF < 1000000000.0f ? shownF : 1000000000.0f;
    At<int32_t>(self, 0x4C) = int32_t(int64_t(capped));
    Stats_Fn_446500(self);
    Stats_Fn_445E90(self);
    col::CVector* last = reinterpret_cast<col::CVector*>(self + 0xC8);
    col::CVector* move = reinterpret_cast<col::CVector*>(self + 0xD4);
    col::CVector d;
    *move = *VectorSub(&d, EntityPos(ms_pPlayerPed), last);
    *last = *EntityPos(ms_pPlayerPed);
    float length = float(std::sqrt(double(std::fabs(float(double(move->x) * move->x + double(move->y) * move->y)))));
    float scale = ms_moveScale * ms_timeStep;
    At<float>(self, 0xE0) = length * scale;
    Vector_Normalise(move);
}

} // namespace PlayerStunts

// ---- registration ----------------------------------------------------------------------

#define C03F_STUNTS_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace PlayerStunts {
C03F_STUNTS_REVERSED("CPlayerStats::Update", 0x00446BC0, &Update);
} // namespace PlayerStunts
