// The effect definitions the game code uses by id, looked up once by name in the effect
// library (0xB6F1E8): 18 16-bit ids from 0x20C9014 down to 0x20C8FD0. 0x72E610.

#include <cstdint>

#include "reversed.h"

namespace {

GAME_VAR(void*, eidEffectLibrary, 0x00B6F1E8);
GAME_FN(uint16_t, __thiscall, Eid_LibraryFind, 0x00667B60, void* library, const char* name);

} // namespace

namespace effectids {

// 0x72E610
void __cdecl Init() {
    static const char* const kNames[18] = {
        "BikeBrake", "CarLightSmash", "GlassShatter", "BreakageSmall", "CarDamage", "ThrusterSmall",
        "Thruster", "LightShatter", "LawnmowerGrass", "RainCircles", "Snow", "rain",
        "rain_Hurricane", "Snow_Blizzard", "leaves", "MissileTrail", "BikeOnBigWet", "PropEffect",
    };
    for (int i = 0; i < 18; i++) {
        *reinterpret_cast<uint16_t*>(0x020C9014 - 4 * i) = Eid_LibraryFind(eidEffectLibrary, kNames[i]);
    }
}

} // namespace effectids

REVERSED("EffectIds::Init", 0x0072E610, RE_BODY(&effectids::Init), nullptr, nullptr, re::Mode::Original);
