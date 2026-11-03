// The unlocks (Config\Dat2\Unlock.dat: a name and a number a line, 0x1C bytes each) and
// the player's unlock flag words they set (0x45BE70..0x45C170), with three item-kind
// ranges (0x45B000..0x45B020).

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct Unlock {
    char name[0x18];   // +0
    int32_t value;     // +0x18 (for unlock 0x12: added to the player's +0x1CA4)
};
static_assert(sizeof(Unlock) == 0x1C);

struct Unlocks {
    int32_t count;     // +0
    re::Ptr32<Unlock> unlocks;   // +4
};
static_assert(sizeof(Unlocks) == 8);

GAME_VAR(Unlocks*, ul_unlocks, 0x00C1AA04);
inline void* const ul_player = reinterpret_cast<void*>(0x00C1AEA8);   // the player info

GAME_FN(uint32_t, __thiscall, Player_GetUnlocks, 0x004466F0, void* self, int word);
GAME_FN(void, __thiscall, Player_SetUnlocks, 0x004466D0, void* self, int word, uint32_t flags);
GAME_FN(int32_t, __cdecl, Unlock_GetIndex, 0x00488BF0, const Unlock* unlock);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* p, size_t size, int count, void* ctor);
GAME_FN(void*, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(char*, __cdecl, File_ReadLine, 0x00429C10, void* file);
GAME_FN(void, __cdecl, File_Seek, 0x0042D300, void* file, int, int);
GAME_FN(void, __cdecl, File_Close, 0x0042D450, void* file);
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(void, __thiscall, Unlocks_Load, 0x0045BFC0, Unlocks* self);

// ---- item kinds -------------------------------------------------------------------------------

// 0x45B000 (cdecl): 0..20
uint8_t __cdecl ItemKind_IsFirst(uint32_t kind) {
    return kind <= 20;
}
// 0x45B010 (cdecl): 21
uint8_t __cdecl ItemKind_IsSecond(int32_t kind) {
    return kind == 21;
}
// 0x45B020 (cdecl): 22..24
uint8_t __cdecl ItemKind_IsThird(int32_t kind) {
    return uint32_t(kind - 22) <= 2;
}

// ---- the player's unlock words ------------------------------------------------------------------

void AddFlags(int word, uint32_t flags) {
    Player_SetUnlocks(ul_player, word, Player_GetUnlocks(ul_player, word) | flags);
}

// 0x45BE70 (stdcall): flags onto word 0x14, else 0x15, else 0x16 (the first non-zero)
uint8_t __fastcall Unlocks_AddFlags(void*, re::Edx, uint32_t flags14, uint32_t flags15, uint32_t flags16) {
    if (flags14) {
        AddFlags(0x14, flags14);
        return 1;
    }
    if (flags15) {
        AddFlags(0x15, flags15);
        return 1;
    }
    if (flags16) {
        AddFlags(0x16, flags16);
        return 1;
    }
    return 0;
}

// 0x45BEF0 (stdcall): flags onto word 0x1B
uint8_t __fastcall Unlocks_AddFlags1B(void*, re::Edx, uint32_t flags) {
    if (flags) {
        AddFlags(0x1B, flags);
        return 1;
    }
    return 0;
}

// 0x45BF20 (thiscall): the named unlock's word is non-zero
uint8_t __fastcall Unlocks_IsUnlocked(Unlocks* self, re::Edx, const char* name) {
    for (int i = 0; i < self->count; i++) {
        if (std::strcmp(name, self->unlocks[i].name) == 0) {
            return Player_GetUnlocks(ul_player, Unlock_GetIndex(&self->unlocks[i])) != 0;
        }
    }
    return 0;
}

// 0x45BFA0 (cdecl): every unlock word read (results unused)
void __cdecl Unlocks_ReadAll() {
    for (int i = 0; i < 0x1D; i++) {
        Player_GetUnlocks(ul_player, i);
    }
}

// ---- Unlock.dat -------------------------------------------------------------------------------

// 0x45BFC0 (thiscall): the lines counted, then read (count starts from what's there)
void __fastcall Unlocks_LoadImpl(Unlocks* self) {
    PushMemoryTag(0x34);
    void* file = File_Open(reinterpret_cast<const char*>(0x0090AD60), reinterpret_cast<const char*>(0x008FF548), 1);   // "Config\Dat2\Unlock.dat", "r"
    for (char* line = File_ReadLine(file); line; line = File_ReadLine(file)) {
        if (line[0] != '\0' && line[0] != '#') {
            self->count++;
        }
    }
    uint32_t count = uint32_t(self->count);
    uint64_t bytes = uint64_t(count) * 0x1C;
    Unlock* unlocks = static_cast<Unlock*>(Malloc(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes)));
    if (unlocks) {
        VectorConstruct(unlocks, 0x1C, int(count), reinterpret_cast<void*>(0x0045BE60));
    } else {
        unlocks = nullptr;
    }
    self->unlocks = unlocks;
    File_Seek(file, 0, 0);
    int offset = 0;
    for (char* line = File_ReadLine(file); line; line = File_ReadLine(file)) {
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        uint8_t* entry = re::PtrCast<uint8_t*>(self->unlocks) + offset;
        Sscanf(line, reinterpret_cast<const char*>(0x008FF970), entry);   // "%s"
        while (*line != '\0' && *line != ' ') {
            line++;
        }
        while (*line != '\0' && *line == ' ') {
            line++;
        }
        Sscanf(line, reinterpret_cast<const char*>(0x008FFF78), entry + 0x18);   // "%d"
        offset += 0x1C;
    }
    File_Close(file);
    PopMemoryTag();
}

// 0x45C0F0 (cdecl): made and loaded once
void __cdecl Unlocks_Init() {
    if (ul_unlocks) {
        return;
    }
    Unlocks* unlocks = static_cast<Unlocks*>(Malloc(8));
    if (unlocks) {
        unlocks->count = 0;
        unlocks->unlocks = nullptr;
        ul_unlocks = unlocks;
        Unlocks_Load(unlocks);
        return;
    }
    ul_unlocks = nullptr;
    // (loads into nullptr)
    Unlocks_Load(nullptr);
}

// 0x45C130 (cdecl)
void __cdecl Unlocks_Shutdown() {
    Unlocks* unlocks = ul_unlocks;
    if (!unlocks) {
        return;
    }
    if (unlocks->unlocks) {
        Free(unlocks->unlocks);
        unlocks->unlocks = nullptr;
    }
    Free(unlocks);
    ul_unlocks = nullptr;
}

// 0x45C170 (thiscall): every unlock of that name set (unlock 0x12 also adds its value to
// the player ped's +0x1CA4)
void __fastcall Unlocks_Unlock(Unlocks* self, re::Edx, const char* name) {
    for (int i = 0; i < self->count; i++) {
        Unlock* unlock = &self->unlocks[i];
        if (std::strcmp(name, unlock->name) != 0) {
            continue;
        }
        int32_t index = Unlock_GetIndex(unlock);
        Player_SetUnlocks(ul_player, index, 1);
        uint8_t* ped = *static_cast<re::Ptr32<uint8_t>*>(ul_player);
        if (index >= 0 && index < 0x1D && index == 0x12) {
            float& value = *reinterpret_cast<float*>(ped + 0x1CA4);
            value = float(unlock->value) + value;
        }
    }
}

#define C04E_UNLOCK(name, address, fn)                                                         \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04E_UNLOCK("ItemKind::IsFirst", 0x0045B000, &ItemKind_IsFirst);
C04E_UNLOCK("ItemKind::IsSecond", 0x0045B010, &ItemKind_IsSecond);
C04E_UNLOCK("ItemKind::IsThird", 0x0045B020, &ItemKind_IsThird);
C04E_UNLOCK("Unlocks::AddFlags", 0x0045BE70, &Unlocks_AddFlags);
C04E_UNLOCK("Unlocks::AddFlags1B", 0x0045BEF0, &Unlocks_AddFlags1B);
C04E_UNLOCK("Unlocks::IsUnlocked", 0x0045BF20, &Unlocks_IsUnlocked);
C04E_UNLOCK("Unlocks::ReadAll", 0x0045BFA0, &Unlocks_ReadAll);
C04E_UNLOCK("Unlocks::Load", 0x0045BFC0, &Unlocks_LoadImpl);
C04E_UNLOCK("Unlocks::Init", 0x0045C0F0, &Unlocks_Init);
C04E_UNLOCK("Unlocks::Shutdown", 0x0045C130, &Unlocks_Shutdown);
C04E_UNLOCK("Unlocks::Unlock", 0x0045C170, &Unlocks_Unlock);

} // namespace
