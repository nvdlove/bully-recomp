// FindNameInList (0x73AC90): a name's index in a list of names (the script enums use it).

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

inline int __cdecl Nl_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline int __cdecl Nl_Strcmp(const char* a, const char* b) { return crt::Strcmp(a, b); }

} // namespace

namespace namelist {

// the index of name in names (ignoreCase 1: _stricmp, else strcmp), or notFound
int __cdecl FindNameInList(const re::Ptr32<const char>* names, int count, const char* name, int notFound, int ignoreCase) {
    int(__cdecl * compare)(const char*, const char*) = ignoreCase == 1 ? Nl_Stricmp : Nl_Strcmp;
    for (int i = 0; i < count; i++) {
        if (compare(names[i], name) == 0) {
            return i;
        }
    }
    return notFound;
}

} // namespace namelist

#define NAMELIST_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

NAMELIST_REVERSED("FindNameInList", 0x0073AC90, &namelist::FindNameInList);
