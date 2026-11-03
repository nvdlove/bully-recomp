// The AM_ manager (0x20C4910) loading and releasing hierarchy objects and HXD files: an
// HXD found or taken from the pool and loaded (with its anim groups: the first only, or
// with +0x1FC set every one, noting when they all fit the recorded size), a hierarchy file
// likewise, their lookups by name (0x1E when found but not loaded), an HXD's anims looked
// up by their names' hashes, an HXD released once unused (its data freed and the HXD back
// in the pool), a hierarchy object released, and the loaded anim data moved by the memory
// manager's compaction (all at once, or a bit at a time: see MoveAnimData).
// 0x6C1230..0x6C179F, 0x6C1950..0x6C2062.

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

GAME_VAR(uint32_t, amhxd_moveGroup, 0x020C4B18);
GAME_VAR(uint32_t, amhxd_moveAnim, 0x020C4B1C);
GAME_VAR(uint8_t*, amhxd_moveLink, 0x020C4B20);
GAME_VAR(uint32_t, amhxd_moveInit, 0x020C4B24);
inline void* const kMemoryHeap = reinterpret_cast<void*>(0x00D14170);

GAME_FN(void, __cdecl, AmHxd_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, AmHxd_NiFreeArray, 0x00752570, void* p, uint32_t size);
GAME_FN(void*, __thiscall, AmHxd_MoveAllocation, 0x005EF4D0, void* heap, void* p);
inline void* __cdecl AmHxd_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(uint32_t, __cdecl, AmHxd_HashName, 0x00576ED0, const char* name);
GAME_FN(int, __thiscall, AmHxd_MakePath, 0x006BD0D0, void* manager, const char* name, char* out, int kind);
GAME_FN(int, __thiscall, AmHxd_LoadFile, 0x006B2EF0, void* hxd, const char* name, uintptr_t flags);
GAME_FN(int, __thiscall, AmHxd_LoadHierarchyFile, 0x006B5870, void* hierarchy, const char* name, uintptr_t a, uintptr_t b);
GAME_FN(void, __thiscall, AmHxd_HxdReset, 0x006B1B20, void* hxd);
GAME_FN(void, __thiscall, AmHxd_ListInsertRange, 0x006B2C20, void* list, uint8_t* before, uint8_t* first,
        uint8_t* last);
GAME_FN(void, __thiscall, AmHxd_ListMoveFrom, 0x006B2CA0, void* self, re::Ptr32<uint8_t>* before, void* source,
        re::Ptr32<uint8_t>* link);
GAME_FN(void*, __thiscall, AmHxd_TakeHierarchy, 0x006BF6C0, void* manager);
GAME_FN(void*, __thiscall, AmHxd_TakeHxd, 0x006BF790, void* manager);
GAME_FN(void, __thiscall, AmHxd_ReleaseHierarchyLink, 0x006BF700, void* manager, uint8_t* link);
GAME_FN(void, __thiscall, AmHxd_ReleaseHxdLink, 0x006BF7E0, void* manager, uint8_t* link);
GAME_FN(void, __thiscall, AmHxd_ReleaseHierarchy, 0x006BF740, void* manager, void* hierarchy);
GAME_FN(int, __thiscall, AmHxd_LoadAgr, 0x006BF500, void* manager, void* hxd, uint32_t group, void* stream);
GAME_FN(void, __thiscall, AmHxd_Release, 0x006C1230, void* manager, uint8_t* hxd);
GAME_FN(int, __thiscall, AmHxd_FindHierarchy, 0x006C17A0, void* manager, uint32_t hash, const char* name,
        re::Ptr32<void>* out);
GAME_FN(int, __thiscall, AmHxd_FindHxd, 0x006C17F0, void* manager, uint32_t hash, const char* name, re::Ptr32<void>* out);
GAME_FN(void, __cdecl, AmHxd_DestroyClump, 0x006C75C0, void* clump);

// an empty 64-byte name (memset the way the game does it)
inline void ClearName(char* name) {
    name[0] = 0;
    AmHxd_Memset(name + 1, 0, 0x3F);
}

// 0x6C1230 (thiscall): an HXD's user gone (+0x14 counted down unless kept, flag 4); at none
// (and not locked, flag 2) its groups' data freed; unless kept, all of it freed too and the
// HXD reset and back in the free pool (the used list's last link when it isn't listed)
void __fastcall Release(uint8_t* self, re::Edx, uint8_t* hxd) {
    uint32_t flags = At<uint32_t>(hxd, 0x10);
    if (!(flags & 4) && At<uint32_t>(hxd, 0x14) != 0) {
        At<uint32_t>(hxd, 0x14) -= 1;
    }
    if (At<uint32_t>(hxd, 0x14) != 0 || (flags & 2)) {
        return;
    }
    uint8_t* anims = nullptr;
    for (uint32_t g = 0; g < At<uint32_t>(hxd, 0x68); g++) {
        uint8_t* group = At<uint8_t*>(hxd, 0x6C) + g * 0x3C;
        if (At<uint8_t>(group, 0x34) != 0) {
            if (At<void*>(group, 0x38) != nullptr) {
                AmHxd_Free(At<void*>(group, 0x38));
                At<void*>(At<uint8_t*>(hxd, 0x6C) + g * 0x3C, 0x38) = nullptr;
                for (uint32_t i = 0; i < At<uint32_t>(hxd, 4); i++) {
                    anims = At<uint8_t*>(hxd, 0);
                    if (At<uint32_t>(anims + i * 0x40, 0x20) == g) {
                        At<void*>(anims + i * 0x40, 4) = nullptr;
                        At<void*>(At<uint8_t*>(hxd, 0) + i * 0x40, 0) = nullptr;
                    }
                }
            } else {
                for (uint32_t i = 0; i < At<uint32_t>(hxd, 4); i++) {
                    uint8_t* anim = At<uint8_t*>(hxd, 0) + i * 0x40;
                    if (At<uint32_t>(anim, 0x20) == g) {
                        AmHxd_Free(At<void*>(anim, 4));
                        At<void*>(At<uint8_t*>(hxd, 0) + i * 0x40, 4) = nullptr;
                        At<void*>(At<uint8_t*>(hxd, 0) + i * 0x40, 0) = nullptr;
                    }
                }
            }
            At<uint16_t>(At<uint8_t*>(hxd, 0x6C) + g * 0x3C, 0x24) &= 0xFFFE;
        }
        At<uint8_t>(At<uint8_t*>(hxd, 0x6C) + g * 0x3C, 0x34) = 0;
    }
    if (At<uint32_t>(hxd, 0x10) & 4) {
        return;
    }
    if (At<uint8_t*>(hxd, 0) != nullptr) {
        for (uint32_t i = 0; i < At<uint32_t>(hxd, 4); i++) {
            uint8_t* anim = At<uint8_t*>(hxd, 0) + i * 0x40;
            if (At<uint16_t>(anim, 0x28) == 0) {
                continue;
            }
            for (uint32_t e = 0; e < At<uint16_t>(At<uint8_t*>(hxd, 0) + i * 0x40, 0x28); e++) {
                uint8_t* event = At<uint8_t*>(At<uint8_t*>(hxd, 0) + i * 0x40, 0x24) + e * 0x10;
                if (At<int32_t>(event, 8) == 1) {
                    AmHxd_Free(At<void*>(event, 0xC));
                }
            }
            AmHxd_Free(At<void*>(At<uint8_t*>(hxd, 0) + i * 0x40, 0x24));
        }
        AmHxd_Free(At<void*>(hxd, 0));
    }
    AmHxd_Free(At<void*>(hxd, 0x60));
    for (uint32_t i = 0; i < At<uint32_t>(hxd, 0xC); i++) {
        AmHxd_Free(At<void*>(At<uint8_t*>(hxd, 8) + i * 0x2C, 0x20));
    }
    AmHxd_Free(At<void*>(hxd, 8));
    AmHxd_Free(At<void*>(hxd, 0x6C));
    AmHxd_NiFreeArray(At<void*>(hxd, 0x70), 0xC);
    if (At<void*>(hxd, 0x74) != nullptr) {
        AmHxd_Free(At<void*>(hxd, 0x74));
    }
    uint8_t* link = At<uint8_t*>(self, 0x60);
    while (link && At<uint8_t*>(link, 0) != hxd) {
        link = At<uint8_t*>(link, 8);
    }
    AmHxd_HxdReset(hxd);
    if (link == nullptr) {
        link = At<uint8_t*>(self, 0x64);
    }
    uint8_t* next = At<uint8_t*>(link, 8);
    if (link == At<uint8_t*>(self, 0x60)) {
        At<uint8_t*>(self, 0x60) = next;
    } else {
        At<uint8_t*>(At<uint8_t*>(link, 4), 8) = next;
    }
    uint8_t* prev = At<uint8_t*>(link, 4);
    if (link == At<uint8_t*>(self, 0x64)) {
        At<uint8_t*>(self, 0x64) = prev;
    } else {
        At<uint8_t*>(At<uint8_t*>(link, 8), 4) = prev;
    }
    At<int32_t>(self, 0x74) -= 1;
    AmHxd_ListInsertRange(self + 0x88, nullptr, link, link);
    At<int32_t>(self + 0x88, 0x18) += 1;
    (void)anims;
}

// the loaded groups of an HXD whose data the memory manager moved: the group's block (its
// anims' data pointers shifted by as much) or each anim's own block
inline void MoveBlock(uint8_t* hxd, uint8_t* group, uint32_t g, uint32_t animCount) {
    uint8_t* moved = static_cast<uint8_t*>(AmHxd_MoveAllocation(kMemoryHeap, At<void*>(group, 0x38)));
    if (moved == At<uint8_t*>(group, 0x38)) {
        return;
    }
    for (uint32_t i = 0; i < animCount; i++) {
        uint8_t* anim = At<uint8_t*>(hxd, 0) + i * 0x40;
        if (At<uint32_t>(anim, 0x20) == g) {
            At<uint8_t*>(anim, 0) += moved - At<uint8_t*>(group, 0x38);
            uint8_t* data = At<uint8_t*>(anim, 0);
            At<uint8_t*>(data, 0x10) = data + 0x18;
        }
    }
    At<uint8_t*>(group, 0x38) = moved;
}
inline void MoveAnim(uint8_t* hxd, uint32_t i) {
    uint8_t* moved = static_cast<uint8_t*>(
        AmHxd_MoveAllocation(kMemoryHeap, At<void*>(At<uint8_t*>(hxd, 0) + i * 0x40, 4)));
    uint8_t* anim = At<uint8_t*>(hxd, 0) + i * 0x40;
    if (moved != At<uint8_t*>(anim, 4)) {
        At<uint8_t*>(anim, 4) = moved;
        At<uint8_t*>(anim, 0) = moved;
        At<uint8_t*>(moved, 0x10) = moved + 0x18;
    }
}

// 0x6C14B0 (thiscall): the used HXDs' loaded anim data moved by the memory manager's
// compaction. `all` (the game's only call): every HXD at once. Else a bit at a time from
// where the last call stopped (0x20C4B18..0x20C4B24): the game never uses that, and it
// never moves past the first listed HXD it reaches (spinning until it has moved 11 anims:
// a hang on an HXD with none, kept as the game has it)
void __fastcall MoveAnimData(uint8_t* self, re::Edx, int all) {
    if (uint8_t(all) != 0) {
        for (uint8_t* link = At<uint8_t*>(self, 0x60); link; link = At<uint8_t*>(link, 8)) {
            uint8_t* hxd = At<uint8_t*>(link, 0);
            if (hxd == nullptr || (At<uint8_t>(hxd, 0x10) & 2)) {
                continue;
            }
            uint32_t animCount = At<uint32_t>(hxd, 4);
            for (uint32_t g = 0; g < At<uint32_t>(hxd, 0x68); g++) {
                uint8_t* group = At<uint8_t*>(hxd, 0x6C) + g * 0x3C;
                if (At<uint8_t>(group, 0x34) == 0) {
                    continue;
                }
                if (At<void*>(group, 0x38) != nullptr) {
                    MoveBlock(hxd, group, g, animCount);
                } else {
                    for (uint32_t i = 0; i < animCount; i++) {
                        if (At<uint32_t>(At<uint8_t*>(hxd, 0) + i * 0x40, 0x20) == g) {
                            MoveAnim(hxd, i);
                        }
                    }
                }
            }
        }
        return;
    }
    uint8_t* link;
    if (!(amhxd_moveInit & 1)) {
        link = At<uint8_t*>(self, 0x60);
        amhxd_moveInit |= 1;
        amhxd_moveLink = link;
    } else {
        link = amhxd_moveLink;
    }
    uint32_t animCursor = amhxd_moveAnim;
    int moved = 0;
    for (;;) {
        while (link) {
            uint8_t* hxd;
            for (;;) {   // (the game spins here on a missing or locked HXD)
                hxd = re::LoadVolatile<uint8_t*>(link);
                if (hxd != nullptr && !(*reinterpret_cast<volatile uint8_t*>(hxd + 0x10) & 2)) {
                    break;
                }
            }
            uint32_t groupCursor = amhxd_moveGroup;
            uint32_t animCount = At<uint32_t>(hxd, 4);
            for (uint32_t g = groupCursor; g < At<uint32_t>(hxd, 0x68); g++) {
                uint8_t* group = g < At<uint32_t>(hxd, 0x68) ? At<uint8_t*>(hxd, 0x6C) + g * 0x3C : nullptr;
                if (At<uint8_t>(group, 0x34) != 0) {
                    if (At<void*>(group, 0x38) != nullptr) {
                        MoveBlock(hxd, group, g, animCount);
                    } else {
                        for (uint32_t i = animCursor; i < animCount; i++) {
                            if (At<uint32_t>(At<uint8_t*>(hxd, 0) + i * 0x40, 0x20) == g) {
                                MoveAnim(hxd, i);
                                moved++;
                                if (moved > 10) {
                                    return;
                                }
                                animCursor = amhxd_moveAnim;
                            }
                            animCursor++;
                            amhxd_moveAnim = animCursor;
                        }
                    }
                    groupCursor = amhxd_moveGroup;
                    animCursor = 0;
                    amhxd_moveAnim = 0;
                }
                groupCursor++;
                amhxd_moveGroup = groupCursor;
            }
            link = re::LoadVolatile<uint8_t*>(&amhxd_moveLink);
            animCursor = 0;
            amhxd_moveAnim = 0;
            amhxd_moveGroup = 0;
        }
        moved++;
        link = At<uint8_t*>(self, 0x60);
        animCursor = 0;
        amhxd_moveAnim = 0;
        amhxd_moveGroup = 0;
        amhxd_moveLink = link;
        if (moved >= 10) {
            return;
        }
    }
}

// 0x6C1950 (thiscall): an HXD by name or hash (or the one in *out) loaded, found or taken
// from the pool, and its anim groups loaded (released again when that fails); `lock` sets
// flag 2. 0xD: the pool's empty
int __fastcall LoadHxd(uint8_t* self, re::Edx, const char* name, uint32_t hash, int lock, re::Ptr32<uint8_t>* out) {
    if (name == nullptr && hash == 0) {
        return 0x16;
    }
    char local[0x40];
    ClearName(local);
    if (hash == 0) {
        int result = AmHxd_MakePath(self, name, local, 2);
        if (result != 0) {
            return result;
        }
        hash = AmHxd_HashName(local);
    } else if (name != nullptr) {
        std::strcpy(local, name);
    }
    uint8_t* hxd = nullptr;
    re::Ptr32<void> found = nullptr;
    bool have = false;
    if (out != nullptr && *out != nullptr) {
        hxd = *out;
        have = true;
    } else if (name != nullptr) {
        if (AmHxd_FindHxd(self, hash, local, &found) != 0) {
            hxd = static_cast<uint8_t*>(found);
            have = true;
        }
    } else if (hash != 0) {
        for (uint8_t* link = At<uint8_t*>(self, 0x60); link; link = At<uint8_t*>(link, 8)) {
            if (At<uint32_t>(At<uint8_t*>(link, 0), 0x18) == hash) {
                hxd = At<uint8_t*>(link, 0);
                have = true;
                break;
            }
        }
    }
    if (have) {
        if (At<uint8_t>(hxd, 0x10) & 1) {
            if (out) {
                *out = hxd;
            }
            return 0;
        }
    } else {
        hxd = static_cast<uint8_t*>(AmHxd_TakeHxd(self));
        if (hxd == nullptr) {
            return 0xD;
        }
        int result = AmHxd_LoadFile(hxd, local, 0);
        if (result != 0) {
            AmHxd_ReleaseHxdLink(self, At<uint8_t*>(self, 0x64));
            return result;
        }
    }
    if (At<uint32_t>(self, 0x1FC) == 0) {
        int result = AmHxd_LoadAgr(self, hxd, 0, nullptr);
        if (result != 0) {
            AmHxd_Release(self, hxd);
            return result;
        }
    } else {
        uint32_t recorded = At<uint32_t>(hxd, 0x64);
        At<uint32_t>(hxd, 0x64) = 0;
        for (uint32_t g = 0; g < At<uint32_t>(hxd, 0x68); g++) {
            int result = AmHxd_LoadAgr(self, hxd, g, nullptr);
            if (result != 0) {
                AmHxd_Release(self, hxd);
                return result;
            }
            At<uint32_t>(hxd, 0x64) += At<uint32_t>(At<uint8_t*>(hxd, 0x6C) + g * 0x3C, 0x30);
        }
        if (recorded == At<uint32_t>(hxd, 0x64)) {
            At<uint32_t>(hxd, 0x10) |= 0x10;
        }
    }
    At<uint32_t>(hxd, 0x10) |= 1;
    if (lock != 0) {
        At<uint32_t>(hxd, 0x10) |= 2;
    }
    if (out) {
        *out = hxd;
    }
    return 0;
}

// 0x6C1B40 (thiscall): a hierarchy file loaded (or found, unless `named` gives the name
// as it is), marked kept (flag 4). 0xC: the pool's empty
int __fastcall LoadHierarchy(uint8_t* self, re::Edx, const char* name, re::Ptr32<void>* out, int named, int b) {
    re::Ptr32<void> found = nullptr;
    if (name == nullptr) {
        return 0x16;
    }
    char local[0x40];
    uint8_t* hierarchy = nullptr;
    bool have = false;
    if (named == 0) {
        int result = AmHxd_MakePath(self, name, local, 0);
        if (result != 0) {
            return result;
        }
        if (AmHxd_FindHierarchy(self, 0, local, &found) != 0) {
            hierarchy = static_cast<uint8_t*>(found);
            have = true;
        }
    } else {
        std::strcpy(local, name);
    }
    if (!have) {
        hierarchy = static_cast<uint8_t*>(AmHxd_TakeHierarchy(self));
        if (hierarchy == nullptr) {
            return 0xC;
        }
        int result = AmHxd_LoadHierarchyFile(hierarchy, local, named, b);
        if (result != 0) {
            AmHxd_ReleaseHierarchyLink(self, At<uint8_t*>(self, 8));
            return result;
        }
    }
    At<uint32_t>(hierarchy, 4) |= 4;
    if (out) {
        *out = hierarchy;
    }
    return 0;
}

// 0x6C1C10 (thiscall): an HXD file loaded (not its groups) or found, marked kept (flag 4)
int __fastcall LoadHxdFile(uint8_t* self, re::Edx, uint32_t hash, const char* name, re::Ptr32<void>* out, int flags) {
    char local[0x40];
    ClearName(local);
    if (hash == 0) {
        if (name == nullptr) {
            return 0x16;
        }
        int result = AmHxd_MakePath(self, name, local, 2);
        if (result != 0) {
            return result;
        }
        hash = AmHxd_HashName(local);
    } else if (name != nullptr) {
        std::strcpy(local, name);
    }
    if (AmHxd_FindHxd(self, hash, local, out) == 0) {
        void* hxd = AmHxd_TakeHxd(self);
        *out = hxd;
        if (hxd == nullptr) {
            return 0xD;
        }
        int result = AmHxd_LoadFile(hxd, local, flags);
        if (result != 0) {
            AmHxd_ReleaseHxdLink(self, At<uint8_t*>(self, 0x64));
            return result;
        }
    }
    At<uint32_t>(*out, 0x10) |= 4;
    return 0;
}

// 0x6C1D00 (thiscall): a hierarchy object's user gone (+0, unless flag 8); at none and
// not locked (2): its clump destroyed, its HXD released and, unless kept (4), it goes back
int __fastcall ReleaseHierarchyObject(uint8_t* self, re::Edx, uint8_t* hierarchy) {
    if (hierarchy == nullptr) {
        return 0x16;
    }
    uint32_t flags = At<uint32_t>(hierarchy, 4);
    if (flags & 8) {
        return 0;
    }
    if (At<uint32_t>(hierarchy, 0) != 0) {
        At<uint32_t>(hierarchy, 0) -= 1;
        if (At<uint32_t>(hierarchy, 0) != 0) {
            return 0;
        }
    }
    if (flags & 2) {
        return 0;
    }
    if (At<void*>(hierarchy, 8) != nullptr) {
        AmHxd_DestroyClump(At<void*>(hierarchy, 8));
        At<uint32_t>(hierarchy, 4) &= ~1u;
        At<void*>(hierarchy, 8) = nullptr;
    }
    if (At<uint8_t*>(hierarchy, 0x10) != nullptr) {
        AmHxd_Release(self, At<uint8_t*>(hierarchy, 0x10));
    }
    if (!(At<uint8_t>(hierarchy, 4) & 4)) {
        AmHxd_ReleaseHierarchy(self, hierarchy);
    }
    return 0;
}

// a used object by name (its path made) or hash: 8 not there; else 0 when loaded (flag 1
// of +FlagsAt), 0x1E when not
template <int List, int HashAt, int FlagsAt, int PathKind>
int FindLoaded(uint8_t* self, const char* name, re::Ptr32<void>* out, uint32_t hash) {
    if (name == nullptr && hash == 0) {
        return 0x16;
    }
    re::Ptr32<void> found = nullptr;
    if (name != nullptr) {
        char local[0x40];
        int result = AmHxd_MakePath(self, name, local, PathKind);
        if (result != 0) {
            return result;
        }
        if (hash == 0) {
            hash = AmHxd_HashName(local);
        }
        int ok = List == 0 ? AmHxd_FindHierarchy(self, hash, local, &found)
                           : AmHxd_FindHxd(self, hash, local, &found);
        if (ok == 0) {
            return 8;
        }
    } else {
        uint8_t* link = At<uint8_t*>(self, List + 4);
        for (; link; link = At<uint8_t*>(link, 8)) {
            if (At<uint32_t>(At<uint8_t*>(link, 0), HashAt) == hash) {
                break;
            }
        }
        if (link == nullptr) {
            return 8;
        }
        found = At<void*>(link, 0);
    }
    if (out) {
        *out = found;
    }
    return (At<uint8_t>(found, FlagsAt) & 1) ? 0 : 0x1E;
}

// 0x6C1D70 (thiscall): a hierarchy object by name or hash
int __fastcall FindLoadedHierarchy(uint8_t* self, re::Edx, const char* name, re::Ptr32<void>* out, uint32_t hash) {
    return FindLoaded<0, 0x14, 4, 0>(self, name, out, hash);
}

// 0x6C1E30 (thiscall): an HXD by name or hash
int __fastcall FindLoadedHxd(uint8_t* self, re::Edx, const char* name, re::Ptr32<void>* out, uint32_t hash) {
    return FindLoaded<0x5C, 0x18, 0x10, 2>(self, name, out, hash);
}

// 0x6C1EF0 (thiscall): anims' indices in an HXD (given, or by name / hash, loaded for the
// look-up and given back after when it wasn't there) by their names' hashes (-1: none)
int __fastcall FindAnims(uint8_t* self, re::Edx, const char* name, const uint32_t* hashes, int32_t* out,
                         uint32_t count, uint8_t* hxd, uint32_t hash) {
    if ((hashes == nullptr && out == nullptr) || (name == nullptr && hxd == nullptr && hash == 0)) {
        return 0x16;
    }
    int found = 1;
    if (hxd == nullptr) {
        char local[0x40];
        int result = AmHxd_MakePath(self, name, local, 2);
        if (result != 0) {
            return result;
        }
        if (hash == 0) {
            hash = AmHxd_HashName(local);
        }
        re::Ptr32<void> existing = nullptr;
        found = AmHxd_FindHxd(self, hash, local, &existing);
        if (found != 0) {
            hxd = static_cast<uint8_t*>(existing);
        } else {
            hxd = static_cast<uint8_t*>(AmHxd_TakeHxd(self));
            if (hxd == nullptr) {
                return 0xD;
            }
            result = AmHxd_LoadFile(hxd, local, 0);
            if (result != 0) {
                AmHxd_ReleaseHxdLink(self, At<uint8_t*>(self, 0x64));
                return result;
            }
        }
    }
    uint32_t animCount = At<uint32_t>(hxd, 4);
    for (uint32_t k = 0; k < count; k++) {
        uint32_t j = 0;
        bool hit = false;
        for (; j < animCount; j++) {
            if (At<uint32_t>(At<uint8_t*>(hxd, 0) + j * 0x40, 0x14) == hashes[k]) {
                hit = true;
                break;
            }
        }
        if (hit) {
            out[k] = int32_t(j);
        } else {
            out[k] = -1;
        }
    }
    if (found == 0) {
        re::Ptr32<uint8_t> link = At<uint8_t*>(self, 0x64);
        if (link) {
            AmHxd_HxdReset(At<void*>(link, 0));
        }
        re::Ptr32<uint8_t> before = nullptr;
        AmHxd_ListMoveFrom(self + 0x88, &before, self + 0x5C, &link);
    }
    return 0;
}

} // namespace

#define AMHXDLOAD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMHXDLOAD_REVERSED("AM_Manager::ReleaseHxd", 0x006C1230, &Release);
AMHXDLOAD_REVERSED("AM_Manager::MoveAnimData", 0x006C14B0, &MoveAnimData);
AMHXDLOAD_REVERSED("AM_Manager::LoadHxd", 0x006C1950, &LoadHxd);
AMHXDLOAD_REVERSED("AM_Manager::LoadHierarchy", 0x006C1B40, &LoadHierarchy);
AMHXDLOAD_REVERSED("AM_Manager::LoadHxdFile", 0x006C1C10, &LoadHxdFile);
AMHXDLOAD_REVERSED("AM_Manager::ReleaseHierarchyObject", 0x006C1D00, &ReleaseHierarchyObject);
AMHXDLOAD_REVERSED("AM_Manager::FindLoadedHierarchy", 0x006C1D70, &FindLoadedHierarchy);
AMHXDLOAD_REVERSED("AM_Manager::FindLoadedHxd", 0x006C1E30, &FindLoadedHxd);
AMHXDLOAD_REVERSED("AM_Manager::FindAnims", 0x006C1EF0, &FindAnims);
