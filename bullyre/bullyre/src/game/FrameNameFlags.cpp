// A frame name's hash and flags (the vehicle/skinning setup reads them: _ok, _dam, _hi,
// extra*, windscreen*), their getters on a node (+0x9C, +0xA0), and two flags set.
// 0x5EF7B0..0x5EF890.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct FrameNameInfo {
    uint32_t hash;   // +0x0
    uint8_t flags;   // +0x4 (1 _ok, 2 _dam, 4 _hi, 8 extra.., 0x10 windscreen..)
};

GAME_FN(uint32_t, __cdecl, FN_HashName, 0x00576ED0, const char* name);
inline char* __cdecl FN_Strstr(const char* text, const char* part) { return crt::Strstr(text, part); }
GAME_VAR(uint32_t, fn_flagD1454C, 0x00D1454C);
GAME_VAR(uint8_t, fn_flagD14550, 0x00D14550);

// the name starting with a prefix (compared up to its length, or both ending)
bool StartsWith(const char* name, const char* prefix, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        if (name[i] != prefix[i]) {
            return false;
        }
        if (name[i] == '\0') {
            break;
        }
    }
    return true;
}

// 0x5EF7B0: a frame name's flags and hash
void __cdecl ParseFrameName(FrameNameInfo* out, const char* name) {
    uint8_t flags = FN_Strstr(name, "_ok") ? 1 : 0;
    if (FN_Strstr(name, "_dam")) {
        flags |= 2;
    }
    if (FN_Strstr(name, "_hi")) {
        flags |= 4;
    }
    if (StartsWith(name, "extra", 5)) {
        flags |= 8;
    }
    if (StartsWith(name, "windscreen", 10)) {
        flags |= 0x10;
    }
    out->flags = flags;
    out->hash = FN_HashName(name);
}

// 0x5EF860: the flag at 0xD1454C set (returns 1)
uint32_t __cdecl SetFlagD1454C() {
    fn_flagD1454C = 1;
    return 1;
}

// 0x5EF870: a node's name hash
uint32_t __cdecl NodeNameHash(const uint8_t* node) {
    return *reinterpret_cast<const uint32_t*>(node + 0x9C);
}

// 0x5EF880: a node's name flags
uint32_t __cdecl NodeNameFlags(const uint8_t* node) {
    return node[0xA0];
}

// 0x5EF890: the flag at 0xD14550 set
void __cdecl SetFlagD14550() {
    fn_flagD14550 = 1;
}

#define FRAMENAME_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(FrameNameOriginal_, __LINE__);                                         \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(FrameNameOriginal_, __LINE__), \
             re::Mode::Original)

FRAMENAME_REVERSED("FrameName::Parse", 0x005EF7B0, &ParseFrameName);
FRAMENAME_REVERSED("FrameName::SetFlagD1454C", 0x005EF860, &SetFlagD1454C);
FRAMENAME_REVERSED("FrameName::NodeHash", 0x005EF870, &NodeNameHash);
FRAMENAME_REVERSED("FrameName::NodeFlags", 0x005EF880, &NodeNameFlags);
FRAMENAME_REVERSED("FrameName::SetFlagD14550", 0x005EF890, &SetFlagD14550);

} // namespace
