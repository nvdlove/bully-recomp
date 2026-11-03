// NiColor / NiColorA / NiPoint3 helpers that sit with the scene graph's: their binary streaming
// (a float at a time through NiStream's binary streams) and their viewer strings
// ("%s = (r,g,b[,a])" in a NiMalloc'd buffer the caller frees).

#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);

// NiPoint3::SaveBinary (NiColor's is the same code)
void __fastcall Point3SaveBinary(float* self, re::Edx, void* stream) {
    SaveBytes(stream, &self[0], 4);
    SaveBytes(stream, &self[1], 4);
    SaveBytes(stream, &self[2], 4);
}

// NiColorA::LoadBinary / SaveBinary
void __fastcall ColorALoadBinary(float* self, re::Edx, void* stream) {
    LoadBytes(stream, &self[0], 4);
    LoadBytes(stream, &self[1], 4);
    LoadBytes(stream, &self[2], 4);
    LoadBytes(stream, &self[3], 4);
}

void __fastcall ColorASaveBinary(float* self, re::Edx, void* stream) {
    SaveBytes(stream, &self[0], 4);
    SaveBytes(stream, &self[1], 4);
    SaveBytes(stream, &self[2], 4);
    SaveBytes(stream, &self[3], 4);
}

// NiColor::GetViewerString(pcPrefix)
char* __fastcall ColorGetViewerString(const float* self, re::Edx, const char* prefix) {
    size_t size = std::strlen(prefix) + 0x1C;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, "%s = (%5.3f,%5.3f,%5.3f)", prefix, double(self[0]), double(self[1]), double(self[2]));
    return text;
}

// NiColorA::GetViewerString(pcPrefix)
char* __fastcall ColorAGetViewerString(const float* self, re::Edx, const char* prefix) {
    size_t size = std::strlen(prefix) + 0x24;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, "%s = (%5.3f,%5.3f,%5.3f,%5.3f)", prefix, double(self[0]), double(self[1]),
              double(self[2]), double(self[3]));
    return text;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NICOLOR_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NICOLOR_REVERSED("NiPoint3::SaveBinary", 0x00762550, &ni::Point3SaveBinary);
NICOLOR_REVERSED("NiColorA::LoadBinary", 0x007625D0, &ni::ColorALoadBinary);
NICOLOR_REVERSED("NiColorA::SaveBinary", 0x00762670, &ni::ColorASaveBinary);
NICOLOR_REVERSED("NiColor::GetViewerString", 0x00762710, &ni::ColorGetViewerString);
NICOLOR_REVERSED("NiColorA::GetViewerString", 0x00762770, &ni::ColorAGetViewerString);
