// NArea::CAreaRegistry: 64 areas (0x6C bytes each, a vtable first); and the areas'
// names, a table of 0x6C-byte strings at 0x9E3440 by area id.

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace c01area {

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline char* __cdecl Strncpy(char* dst, const char* src, size_t count) { return crt::Strncpy(dst, src, count); }

inline re::Ptr32<void>* const CArea_vftable = reinterpret_cast<re::Ptr32<void>*>(0x009009C4);
inline re::Ptr32<void>* const CAreaRegistry_vftable = reinterpret_cast<re::Ptr32<void>*>(0x009009CC);

struct CArea {
    re::Ptr32<re::Ptr32<void>> vtable;           // +0
    uint8_t unknown4[0x68];
};
static_assert(sizeof(CArea) == 0x6C);

struct CAreaRegistry {
    re::Ptr32<re::Ptr32<void>> vtable;      // +0
    CArea areas[64];    // +4
};
static_assert(sizeof(CAreaRegistry) == 0x1B04);

// The names, 0x6C bytes each
GAME_ARRAY(char, areaNames, 0x009E3440);

// 0x415050: an area's name
char* __cdecl GetAreaName(const int32_t* area) {
    return areaNames + *area * 0x6C;
}

// 0x415060: an area's name set (up to 100 characters; copied without its terminator)
void __cdecl SetAreaName(const int32_t* area, const char* name) {
    if (!name) {
        return;
    }
    size_t length = std::strlen(name);
    if (static_cast<int>(length) <= 100) {
        Strncpy(areaNames + *area * 0x6C, name, length);
    }
}

// 0x4150A0 (thiscall, vf0): CArea's scalar deleting destructor
CArea* __fastcall Area_Destroy(CArea* self, re::Edx, uint8_t flags) {
    self->vtable = CArea_vftable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x4150C0 (thiscall, vf0): the registry's (the areas' destructors inlined, last first)
CAreaRegistry* __fastcall Registry_Destroy(CAreaRegistry* self, re::Edx, uint8_t flags) {
    self->vtable = CAreaRegistry_vftable;
    for (int i = 63; i >= 0; i--) {
        self->areas[i].vtable = CArea_vftable;
    }
    if (flags & 1) {
        Free(self);
    }
    return self;
}

} // namespace c01area

// ---- registration ----------------------------------------------------------------------

#define AREA_REVERSED_C01(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01area {
AREA_REVERSED_C01("NArea::GetAreaName", 0x00415050, &GetAreaName);
AREA_REVERSED_C01("NArea::SetAreaName", 0x00415060, &SetAreaName);
AREA_REVERSED_C01("NArea::CAreaRegistry::CArea::~CArea", 0x004150A0, &Area_Destroy);
AREA_REVERSED_C01("NArea::CAreaRegistry::~CAreaRegistry", 0x004150C0, &Registry_Destroy);
} // namespace c01area
