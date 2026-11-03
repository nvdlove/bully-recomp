// CStats: the stats table (ints or floats by index, at 0xC1A338), its increments that
// complete mini objectives, and the stats screen's layout items. 0x458420..0x4585D0.

#include <cstdint>

#include "reversed.h"

namespace {

GAME_ARRAY(int32_t, statsInt, 0x00C1A338);
GAME_ARRAY(float, statsFloat, 0x00C1A338);
GAME_VAR(uint16_t, numLayoutItems, 0x00C1A748);
inline void* const playerInfo = reinterpret_cast<void*>(0x00C1AEA8);

GAME_FN(uint8_t, __thiscall, PlayerInfo_GetMiniObjectiveComplete, 0x00446860, void* self, int id);
GAME_FN(void, __thiscall, PlayerInfo_SetMiniObjectiveComplete, 0x004467F0, void* self, int id, uint8_t complete);

// the mini objective completed when the stat first reaches `at`
void CompleteAt(int id, int value, int at) {
    if (!PlayerInfo_GetMiniObjectiveComplete(playerInfo, id) && value >= at) {
        PlayerInfo_SetMiniObjectiveComplete(playerInfo, id, 1);
    }
}

} // namespace

namespace stats {

// A layout item of the stats screen (8 bytes, at 0xC193E8): its kind at +2
// 0x458420 / 0x458430 / 0x458440 (thiscall; all of eax 0 / 1)
uint32_t __fastcall LayoutItem_IsKind0(uint8_t* self, re::Edx) {
    return self[2] == 0;
}
uint32_t __fastcall LayoutItem_IsKind1(uint8_t* self, re::Edx) {
    return self[2] == 1;
}
uint32_t __fastcall LayoutItem_IsKind2(uint8_t* self, re::Edx) {
    return self[2] == 2;
}

// 0x458450: the layout emptied
void __cdecl ClearLayout() {
    numLayoutItems = 0;
}

// 0x458460
void __cdecl SetStatInt(int stat, int value) {
    statsInt[stat] = value;
}

// 0x458470
void __cdecl SetStatFloat(int stat, float value) {
    statsFloat[stat] = value;
}

// 0x458480: an int stat increased; some complete a mini objective at a count
void __cdecl IncrementStat(int stat, int amount) {
    int value = statsInt[stat] + amount;
    switch (stat) {
    case 0x28:
        CompleteAt(2, value, 500);
        break;
    case 0xC2:
        CompleteAt(1, value, 100);
        break;
    case 0x1F:
        CompleteAt(8, value, 20);
        break;
    case 0x42:
        CompleteAt(0x11, value, 0x107);
        break;
    case 0x6C:
        CompleteAt(0x14, value, 1000);
        break;
    }
    statsInt[stat] = value;
}

// 0x4585B0
void __cdecl IncrementStatFloat(int stat, float amount) {
    statsFloat[stat] = statsFloat[stat] + amount;
}

// 0x4585D0: a of b in percent, 0..100 (0 when b is 0)
float __cdecl Percentage(float a, float b) {
    float percent = b == 0.0f ? 0.0f : float(double(a) / double(b) * 100.0);
    if (percent < 0.0f) {
        return 0.0f;
    }
    if (100.0f < percent) {
        return 100.0f;
    }
    return percent;
}

} // namespace stats

// ---- registration ---------------------------------------------------------------------

#define C04D_STATS_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04D_STATS_REVERSED("CStatLayoutItem::IsKind0", 0x00458420, &stats::LayoutItem_IsKind0);
C04D_STATS_REVERSED("CStatLayoutItem::IsKind1", 0x00458430, &stats::LayoutItem_IsKind1);
C04D_STATS_REVERSED("CStatLayoutItem::IsKind2", 0x00458440, &stats::LayoutItem_IsKind2);
C04D_STATS_REVERSED("CStats::ClearLayout", 0x00458450, &stats::ClearLayout);
C04D_STATS_REVERSED("CStats::SetStatInt", 0x00458460, &stats::SetStatInt);
C04D_STATS_REVERSED("CStats::SetStatFloat", 0x00458470, &stats::SetStatFloat);
C04D_STATS_REVERSED("CStats::IncrementStat", 0x00458480, &stats::IncrementStat);
C04D_STATS_REVERSED("CStats::IncrementStatFloat", 0x004585B0, &stats::IncrementStatFloat);
C04D_STATS_REVERSED("CStats::Percentage", 0x004585D0, &stats::Percentage);
