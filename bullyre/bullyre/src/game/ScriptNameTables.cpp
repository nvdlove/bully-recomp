// The name tables scripts pass enums by (0x5DADE0..0x5DAF20): a name to its index
// (FindNameInList, -1 when not there) or an index to its name ("INVALID" past the end).
// Each is named after its table's first entry.

#include <cstdint>

#include "reversed.h"

namespace {

GAME_FN(int, __cdecl, sntFindNameInList, 0x0073AC90, const re::Ptr32<const char>* names, int count, const char* name, int fallback, int flag);

const re::Ptr32<const char>* sntTable(uintptr_t address) {
    return reinterpret_cast<const re::Ptr32<const char>*>(address);
}
inline const char* const sntInvalid = reinterpret_cast<const char*>(0x00903168);  // "INVALID"

// 0x5DADE0: COMMAND_LEFT.. (5)
int __cdecl CommandFromName(const char* name) {
    return sntFindNameInList(sntTable(0x0092C57C), 5, name, -1, 0);
}
// 0x5DAE00: UP_HOBO1.. (72)
const char* __cdecl UpgradeName(uint32_t index) {
    return index <= 0x47 ? static_cast<const char*>(sntTable(0x0092C6A8)[index]) : sntInvalid;
}
// 0x5DAE20
int __cdecl UpgradeFromName(const char* name) {
    return sntFindNameInList(sntTable(0x0092C6A8), 0x48, name, -1, 0);
}
// 0x5DAE40: UH_HOBO.. (11)
const char* __cdecl UpgradeHudName(uint32_t index) {
    return index <= 0xA ? static_cast<const char*>(sntTable(0x0092C7CC)[index]) : sntInvalid;
}
// 0x5DAE60: DPadL.. (25)
const char* __cdecl ButtonName(uint32_t index) {
    return index <= 0x18 ? static_cast<const char*>(sntTable(0x0092C860)[index]) : sntInvalid;
}
// 0x5DAE80: SubtitleSmall.. (8)
const char* __cdecl TextStyleName(uint32_t index) {
    return index <= 7 ? static_cast<const char*>(sntTable(0x0092C83C)[index]) : sntInvalid;
}
// 0x5DAEA0: Weapon.. (8)
int __cdecl ColourFromName(const char* name) {
    return sntFindNameInList(sntTable(0x0092C534), 8, name, -1, 0);
}
// 0x5DAEC0: Outdoor.. (7)
int __cdecl AreaKindFromName(const char* name) {
    return sntFindNameInList(sntTable(0x0092C67C), 7, name, -1, 0);
}
// 0x5DAEE0: Gnome.. (7)
int __cdecl CollectibleFromName(const char* name) {
    return sntFindNameInList(sntTable(0x0092C7FC), 7, name, -1, 0);
}
// 0x5DAF00: horizontal alignment (3)
int __cdecl HAlignFromName(const char* name) {
    return sntFindNameInList(sntTable(0x0092C81C), 3, name, -1, 0);
}
// 0x5DAF20: vertical alignment (3)
int __cdecl VAlignFromName(const char* name) {
    return sntFindNameInList(sntTable(0x0092C82C), 3, name, -1, 0);
}

} // namespace

#define SNT_REVERSED(name, address, fn)                   \
    static void* RE_CONCAT(sntOriginal_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(sntOriginal_, __LINE__), re::Mode::Original)

SNT_REVERSED("ScriptNames::CommandFromName", 0x005DADE0, &CommandFromName);
SNT_REVERSED("ScriptNames::UpgradeName", 0x005DAE00, &UpgradeName);
SNT_REVERSED("ScriptNames::UpgradeFromName", 0x005DAE20, &UpgradeFromName);
SNT_REVERSED("ScriptNames::UpgradeHudName", 0x005DAE40, &UpgradeHudName);
SNT_REVERSED("ScriptNames::ButtonName", 0x005DAE60, &ButtonName);
SNT_REVERSED("ScriptNames::TextStyleName", 0x005DAE80, &TextStyleName);
SNT_REVERSED("ScriptNames::ColourFromName", 0x005DAEA0, &ColourFromName);
SNT_REVERSED("ScriptNames::AreaKindFromName", 0x005DAEC0, &AreaKindFromName);
SNT_REVERSED("ScriptNames::CollectibleFromName", 0x005DAEE0, &CollectibleFromName);
SNT_REVERSED("ScriptNames::HAlignFromName", 0x005DAF00, &HAlignFromName);
SNT_REVERSED("ScriptNames::VAlignFromName", 0x005DAF20, &VAlignFromName);
