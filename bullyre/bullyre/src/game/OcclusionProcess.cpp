// 0x51FDA0: COcclusion::ProcessBeforeRendering (GTA SA's): a few far-away occluders a
// frame move to the nearby list when near the camera, the nearby ones become this frame's
// active occluders (or go back to the far list), and active occluders hidden behind a
// nearer one are dropped.

#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace {

// COccluder: 0x24 bytes (+0x1A the next in its list, +0x20 its area)
constexpr uintptr_t kOccluders = 0x00C8B790;
constexpr int kOccluderSize = 0x24;
// the active occluders: 0x80 bytes each (+0x7C the distance), at most 48
constexpr uintptr_t kActiveOccluders = 0x00C89F90;
constexpr int kActiveOccluderSize = 0x80;

GAME_VAR(int32_t, NumActiveOccluders, 0x00C89F88);
GAME_VAR(int16_t, FarAwayList, 0x00A76B44);
GAME_VAR(int16_t, NearbyList, 0x00A76B48);
GAME_VAR(int16_t, ListWalkThroughFA, 0x00A76B4C);
GAME_VAR(int16_t, PreviousListWalkThroughFA, 0x00A76B50);
GAME_VAR(int32_t, currArea, 0x00BD1008);   // CGame::currArea

GAME_FN(uint8_t, __thiscall, COccluder_NearCamera, 0x0051DCE0, void* occluder);
GAME_FN(uint8_t, __thiscall, COccluder_ProcessOneOccluder, 0x0051E2E0, void* occluder, void* active);
GAME_FN(uint8_t, __cdecl, OccluderHidesBehind, 0x0051CFC0, void* active, void* other);

uint8_t* Occluder(int index) {
    return reinterpret_cast<uint8_t*>(kOccluders + index * kOccluderSize);
}
int16_t& Next(int index) {
    return *reinterpret_cast<int16_t*>(Occluder(index) + 0x1A);
}
int32_t Area(int index) {
    return *reinterpret_cast<int32_t*>(Occluder(index) + 0x20);
}
// (index -1 happens: see below)
uint8_t* Active(int index) {
    return reinterpret_cast<uint8_t*>(kActiveOccluders + index * kActiveOccluderSize);
}

} // namespace

namespace COcclusion {

static void __cdecl ProcessBeforeRendering() {
    NumActiveOccluders = 0;
    bool walkFarList = true;
    if (ListWalkThroughFA == -1) {
        PreviousListWalkThroughFA = -1;
        ListWalkThroughFA = FarAwayList;
        walkFarList = FarAwayList != -1;
    }
    // up to 16 of the far-away list, going on where the last frame stopped
    if (walkFarList) {
        for (int i = 0; i < 16; i++) {
            int16_t walk = ListWalkThroughFA;
            if (walk == -1) {
                break;
            }
            int16_t next;
            if (Area(walk) == currArea && COccluder_NearCamera(Occluder(walk))) {
                int16_t current = ListWalkThroughFA;
                if (PreviousListWalkThroughFA == -1) {
                    FarAwayList = Next(current);
                } else {
                    Next(PreviousListWalkThroughFA) = Next(current);
                }
                next = Next(current);
                Next(current) = NearbyList;
                NearbyList = current;
            } else {
                walk = ListWalkThroughFA;
                PreviousListWalkThroughFA = walk;
                next = Next(walk);
            }
            ListWalkThroughFA = next;
        }
    }

    // the nearby list: this frame's active occluders; those of this area no longer near
    // go back to the far list
    int count = NumActiveOccluders;
    int16_t previous = -1;
    int16_t index = NearbyList;
    while (index != -1) {
        if (count < 48) {
            uint8_t added = COccluder_ProcessOneOccluder(Occluder(index), Active(count));
            count = NumActiveOccluders;
            if (added) {
                NumActiveOccluders = ++count;
            }
        }
        if (Area(index) == currArea && !COccluder_NearCamera(Occluder(index))) {
            if (previous == -1) {
                NearbyList = Next(index);
            } else {
                Next(previous) = Next(index);
            }
            int16_t next = Next(index);
            count = NumActiveOccluders;
            Next(index) = FarAwayList;
            FarAwayList = index;
            index = next;
        } else {
            count = NumActiveOccluders;
            previous = index;
            index = Next(index);
        }
    }

    // an active occluder farther than one that hides it is dropped (the list closed up;
    // `i` steps back, to -1 for the first one, so the rest of the inner loop compares the
    // slot before the array; it goes on with the new count)
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < count; j++) {
            if (i == j) {
                continue;
            }
            float distI = *reinterpret_cast<float*>(Active(i) + 0x7C);
            float distJ = *reinterpret_cast<float*>(Active(j) + 0x7C);
            if (!(distI > distJ)) {
                continue;
            }
            if (!OccluderHidesBehind(Active(i), Active(j))) {
                continue;
            }
            int last = count - 1;
            if (i < last) {
                std::memmove(Active(i), Active(i + 1), size_t(last - i) * kActiveOccluderSize);
            }
            count = last;
            i--;
            NumActiveOccluders = count;
        }
    }
}

} // namespace COcclusion

#define C16F_OCC_REVERSED(name, fn, address)                                                        \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace COcclusion {
C16F_OCC_REVERSED("COcclusion::ProcessBeforeRendering", ProcessBeforeRendering, 0x0051FDA0);
} // namespace COcclusion
