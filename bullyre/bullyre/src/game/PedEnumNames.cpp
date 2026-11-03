// The name tables of the peds' and the AI's enums (0x488AF0 .. 0x488C90): a value's name
// ("INVALID" out of range) and a name's value (-1 unknown), through 0x73AC90.

#include <cstdint>

#include "reversed.h"

namespace {

GAME_FN(int, __cdecl, PEN_FindName, 0x0073AC90, const re::Ptr32<const char>* names, int count, const char* name, int fallback,
        int ignoreCase);
inline const char* const PEN_invalid = reinterpret_cast<const char*>(0x00903168);   // "INVALID"

const re::Ptr32<const char>* Table(uintptr_t address) {
    return reinterpret_cast<const re::Ptr32<const char>*>(address);
}

// The tables (count, address)
constexpr uintptr_t kFactions = 0x0090E7D0;        // 15: PREFECT, NERD, JOCK, ...
constexpr uintptr_t kCliques = 0x0090E6DC;         // 6: NERD, JOCK, DROPOUT, GREASER, PREPPY, BULLY
constexpr uintptr_t kFightStyles = 0x0090E810;     // 5: Generic, Striker, Grappler, Ranged, Melee
constexpr uintptr_t kSizes = 0x0090E828;           // 5: Small, Medium, Large, Fat, Huge
constexpr uintptr_t kPointTypes = 0x0090E9E0;      // 16: Sitting_Spot, Spectator, Hang_Out, ...
constexpr uintptr_t kGenders = 0x0090EA5C;         // 3: Both, Male, Female
constexpr uintptr_t kFeetSyncs = 0x0090EA6C;       // 5: FeetSync, FeetSyncTop, ...
constexpr uintptr_t kStats = 0x0090EBF8;           // 29: NUM_TIME_SLEPT_ON_STREET, ...
constexpr uintptr_t kCrimes = 0x0090ECF8;          // 46: CRIME_PEDATTACKSTUDENT, ...
constexpr uintptr_t kPointUsers = 0x0090EB14;      // 8: MaleOnly, FemaleOnly, ...
constexpr uintptr_t kAttitudes = 0x0090EB84;       // 5: Abhor, Averse, Dispassionate, Dig, Adore
constexpr uintptr_t kClothingTypes = 0x0090EC94;   // 11: CASUAL, STREET, UNIFORM, ...
constexpr uintptr_t kFeelings = 0x0090ECC4;        // 3: LIKE, NEUTRAL, DISLIKE

} // namespace

namespace c07pen {

// 0x488AF0
const char* __cdecl FactionName(uint32_t value) {
    return value > 14 ? PEN_invalid : static_cast<const char*>(Table(kFactions)[value]);
}

// 0x488B10
int __cdecl FactionFromName(const char* name) {
    return PEN_FindName(Table(kFactions), 15, name, -1, 0);
}

// 0x488B30
const char* __cdecl CliqueName(uint32_t value) {
    return value > 5 ? PEN_invalid : static_cast<const char*>(Table(kCliques)[value]);
}

// 0x488B50
int __cdecl FightStyleFromName(const char* name) {
    return PEN_FindName(Table(kFightStyles), 5, name, -1, 0);
}

// 0x488B70
int __cdecl SizeFromName(const char* name) {
    return PEN_FindName(Table(kSizes), 5, name, -1, 0);
}

// 0x488B90
int __cdecl PointTypeFromName(const char* name) {
    return PEN_FindName(Table(kPointTypes), 16, name, -1, 0);
}

// 0x488BB0
int __cdecl GenderFromName(const char* name) {
    return PEN_FindName(Table(kGenders), 3, name, -1, 0);
}

// 0x488BD0
const char* __cdecl FeetSyncName(uint32_t value) {
    return value > 4 ? PEN_invalid : static_cast<const char*>(Table(kFeetSyncs)[value]);
}

// 0x488BF0
int __cdecl StatFromName(const char* name) {
    return PEN_FindName(Table(kStats), 29, name, -1, 0);
}

// 0x488C10
int __cdecl CrimeFromName(const char* name) {
    return PEN_FindName(Table(kCrimes), 46, name, -1, 0);
}

// 0x488C30
int __cdecl PointUserFromName(const char* name) {
    return PEN_FindName(Table(kPointUsers), 8, name, -1, 0);
}

// 0x488C50
int __cdecl AttitudeFromName(const char* name) {
    return PEN_FindName(Table(kAttitudes), 5, name, -1, 0);
}

// 0x488C70
int __cdecl ClothingTypeFromName(const char* name) {
    return PEN_FindName(Table(kClothingTypes), 11, name, -1, 0);
}

// 0x488C90 (the case ignored)
int __cdecl FeelingFromName(const char* name) {
    return PEN_FindName(Table(kFeelings), 3, name, -1, 1);
}

} // namespace c07pen

#define PEN_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEN_REVERSED("PedEnums::FactionName", 0x00488AF0, &c07pen::FactionName);
PEN_REVERSED("PedEnums::FactionFromName", 0x00488B10, &c07pen::FactionFromName);
PEN_REVERSED("PedEnums::CliqueName", 0x00488B30, &c07pen::CliqueName);
PEN_REVERSED("PedEnums::FightStyleFromName", 0x00488B50, &c07pen::FightStyleFromName);
PEN_REVERSED("PedEnums::SizeFromName", 0x00488B70, &c07pen::SizeFromName);
PEN_REVERSED("PedEnums::PointTypeFromName", 0x00488B90, &c07pen::PointTypeFromName);
PEN_REVERSED("PedEnums::GenderFromName", 0x00488BB0, &c07pen::GenderFromName);
PEN_REVERSED("PedEnums::FeetSyncName", 0x00488BD0, &c07pen::FeetSyncName);
PEN_REVERSED("PedEnums::StatFromName", 0x00488BF0, &c07pen::StatFromName);
PEN_REVERSED("PedEnums::CrimeFromName", 0x00488C10, &c07pen::CrimeFromName);
PEN_REVERSED("PedEnums::PointUserFromName", 0x00488C30, &c07pen::PointUserFromName);
PEN_REVERSED("PedEnums::AttitudeFromName", 0x00488C50, &c07pen::AttitudeFromName);
PEN_REVERSED("PedEnums::ClothingTypeFromName", 0x00488C70, &c07pen::ClothingTypeFromName);
PEN_REVERSED("PedEnums::FeelingFromName", 0x00488C90, &c07pen::FeelingFromName);
