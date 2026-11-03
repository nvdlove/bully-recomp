// CObjectData::Initialise: OBJECT.DAT (Config\Extra\OBJECT2.DAT with the extra object
// data, whose models are numbers) read into the object info table (0xC35200, 0x38
// bytes an entry) and the table of models sharing another model's entry (0xC34F58).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/FileLoader.h"
#include "reversed.h"

#include "game/Crt.h"
namespace c11objdata {

// 0x38 bytes an entry (the fields ObjectMisc.cpp found)
struct ObjectInfo {
    uint16_t id;                // +0x0 low 15 bits the id, bit 15 a flag
    uint16_t flags2;            // +0x2 bit 0
    uint16_t breakEffects[2];   // +0x4 area effects on breaking (0xFFFF: none)
    int16_t breakHeights[2];    // +0x8 (1/256)
    uint16_t hitEffect;         // +0xC
    uint16_t soundBank;         // +0xE
    int32_t bankRangeSq;        // +0x10 (the range squared)
    int32_t ambientSound;       // +0x14 a sound id, or a stream's hash (+0x21 bit 0)
    int32_t breakSound;         // +0x18
    int32_t hitSound;           // +0x1C
    uint8_t soundSizes;         // +0x20 low nibble the ambient sound's, high the others'
    uint8_t emitter;            // +0x21 bit 0 a stream (.rsm), the rest the volume (percent)
    uint8_t unknown22;          // +0x22 bit 0 "AttachToEmitter"
    uint8_t unknown23;
    uint32_t hash;              // +0x24
    int32_t model;              // +0x28 -1: none
    re::Ptr32<void> boneInfo;             // +0x2C something made on breaking
    int16_t height;             // +0x30 (1/256)
    uint8_t unknown32[0x2];
    int32_t extraModel;         // +0x34 -1: none
};
static_assert(offsetof(ObjectInfo, breakEffects) == 0x4);
static_assert(offsetof(ObjectInfo, breakHeights) == 0x8);
static_assert(offsetof(ObjectInfo, hitEffect) == 0xC);
static_assert(offsetof(ObjectInfo, soundBank) == 0xE);
static_assert(offsetof(ObjectInfo, bankRangeSq) == 0x10);
static_assert(offsetof(ObjectInfo, ambientSound) == 0x14);
static_assert(offsetof(ObjectInfo, soundSizes) == 0x20);
static_assert(offsetof(ObjectInfo, emitter) == 0x21);
static_assert(offsetof(ObjectInfo, unknown22) == 0x22);
static_assert(offsetof(ObjectInfo, hash) == 0x24);
static_assert(offsetof(ObjectInfo, model) == 0x28);
static_assert(offsetof(ObjectInfo, boneInfo) == 0x2C);
static_assert(offsetof(ObjectInfo, height) == 0x30);
static_assert(offsetof(ObjectInfo, extraModel) == 0x34);
static_assert(sizeof(ObjectInfo) == 0x38);

// A model using another model's entry (read as that model; the index after loading)
struct ObjectInfoOverride {
    int16_t model;   // -1: none
    int16_t index;
};
static_assert(sizeof(ObjectInfoOverride) == 4);

// The game's string tokenizer (0x14 bytes, on the stack)
struct Tokenizer {
    re::Ptr32<const char> cur;            // +0x0 the current token
    char delimiters[8];         // +0x4 (all 8 bytes compared; zeroed first)
    re::Ptr32<const char> start;          // +0xC
    int32_t unknown10;          // +0x10
};
static_assert(offsetof(Tokenizer, start) == 0xC);
static_assert(sizeof(Tokenizer) == 0x14);

constexpr int kObjectInfos = 335;   // (0xC35200 .. 0xC39B70)
constexpr int kOverrides = 170;

// 0x4D0DE0 (cdecl): every line (';' a comment) an entry: model, extra model ("None"),
// id, flag, flag, then either a model alone (this model uses that one's entry: an
// override) or: two break effects, their heights, hit effect, sound bank, its range,
// ambient sound (a sound, "name.rsm" a stream, "AttachToEmitter"), its volume (0..1),
// break sound, hit sound, two sound sizes, and optionally a hash name, a bone info
// name and a height. Then the overrides' models made entry indices, and (unless level
// 2 is skipped) every model's index set
void __cdecl Initialise(const char* path);

} // namespace c11objdata

namespace {

using namespace c11objdata;

GAME_FN(uint8_t, __thiscall, MemoryFile_Load, 0x00426610, MemoryFile* file, const char* path);
GAME_FN(uint8_t, __cdecl, File_ReadLineFromMemory, 0x0042D600, MemoryFile* file, char* buffer, int size);
GAME_FN(void, __thiscall, Tokenizer_Construct, 0x0061A470, Tokenizer* t, const char* text, const char* delimiters);
GAME_FN(void, __thiscall, Tokenizer_Next, 0x0061A4D0, Tokenizer* t);
GAME_FN(int32_t, __thiscall, Tokenizer_GetInt, 0x0061A590, Tokenizer* t);
GAME_FN(float, __thiscall, Tokenizer_GetFloat, 0x0061A5C0, Tokenizer* t);
GAME_FN(int, __thiscall, Tokenizer_CopyToken, 0x0061A3B0, Tokenizer* t, const char* at, char* buffer, int size, int stop,
        int useDelimiters);
GAME_FN(void*, __cdecl, GetModelInfo, 0x0051C190, const char* name, int32_t* id);
GAME_FN(uint16_t, __thiscall, AE_Library_Find, 0x00667B60, void* library, const char* name);
GAME_FN(int, __thiscall, SoundBanks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(int32_t, __cdecl, HashStreamName, 0x005A7D60, const char* name, int arg);
GAME_FN(void, __cdecl, DebugPrintf, 0x0049A650, int level, const char* format, const char* arg);   // (empty)
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
GAME_FN(uint8_t, __cdecl, GetSoundSizeFromName, 0x004D0BB0, void* name);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, CopyInt, 0x005FA880, uint32_t* dst, const uint32_t* src);
GAME_FN(void, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* path, int count, const uint32_t* names);
GAME_FN(void*, __cdecl, FindBoneInfo, 0x0072AEA0, void* path, void* names);
GAME_FN(void, __cdecl, SetModelObjectInfoIndices, 0x004D0D10);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

// the game's C runtime
inline int __cdecl Atoi(const char* s) { return (int)crt::Atol(s); }
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline char* __cdecl Strchr(const char* s, int c) { return crt::Strchr(s, c); }

GAME_ARRAY(ObjectInfo, objectInfos, 0x00C35200);
GAME_ARRAY(ObjectInfoOverride, objectInfoOverrides, 0x00C34F58);
GAME_VAR(int32_t, objectInfoCount, 0x00C34F54);
GAME_VAR(int32_t, overrideCount, 0x00C34F50);
GAME_VAR(uint8_t, ms_bExtraObjectData, 0x00BF380B);
GAME_VAR(uint8_t, ms_bSkipLevel2, 0x00BF3813);
GAME_VAR(uint32_t, defaultInfoHash, 0x01B654D4);
GAME_VAR(void*, effectLibrary, 0x00B6F1E8);
inline char* const nameBuffer = reinterpret_cast<char*>(0x00C34E50);   // 0x100
inline void* const soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline const uint32_t* const boneNameKey1 = reinterpret_cast<const uint32_t*>(0x01B653A0);
inline const uint32_t* const boneNameKey2 = reinterpret_cast<const uint32_t*>(0x01B6538C);
inline void* const boneNames = reinterpret_cast<void*>(0x01B65390);

// (an exact, case-sensitive compare of 5 bytes, as the game's repe cmpsb)
bool IsNone(const char* s) {
    static const char none[5] = {'N', 'o', 'n', 'e', '\0'};
    for (int i = 0; i < 5; i++) {
        if (s[i] != none[i]) {
            return false;
        }
    }
    return true;
}

// _ftol: truncated to 64 bits (NaN and out of range: the integer indefinite)
int64_t Ftol(double value) {
    if (!(value > -9223372036854775808.0 && value < 9223372036854775808.0)) {
        return INT64_MIN;
    }
    return int64_t(value);
}

void CopyToken(Tokenizer* tok) {
    Tokenizer_CopyToken(tok, tok->cur, nameBuffer, 0x100, 0, 1);
}

uint16_t FindEffect() {
    return IsNone(nameBuffer) ? uint16_t(0xFFFF) : AE_Library_Find(effectLibrary, nameBuffer);
}

} // namespace

namespace c11objdata {

void __cdecl Initialise(const char* path) {
    for (int i = 0; i < kObjectInfos; i++) {
        objectInfos[i].model = -1;
    }
    for (int i = 0; i < kOverrides; i++) {
        objectInfoOverrides[i].model = -1;
    }
    objectInfoCount = 0;
    overrideCount = 0;
    MemoryFile file = {nullptr, 0, 0};
    MemoryFile_Load(&file, path);   // (its result not checked)
    char line[0x200];
    // (no bounds checks: past 335 entries or 170 overrides the tables overflow)
    while (File_ReadLineFromMemory(&file, line, 0x200)) {
        if (line[0] == ';') {
            continue;
        }
        Tokenizer tok;
        Tokenizer_Construct(&tok, line, " \t\r\n");
        if (*tok.cur == '\0') {
            continue;
        }
        ObjectInfo* info = &objectInfos[objectInfoCount];
        info->model = -1;
        if (ms_bExtraObjectData) {
            info->model = Tokenizer_GetInt(&tok);
        } else {
            CopyToken(&tok);
            GetModelInfo(nameBuffer, &info->model);
        }
        Tokenizer_Next(&tok);
        if (ms_bExtraObjectData) {
            info->extraModel = Tokenizer_GetInt(&tok);
        } else {
            CopyToken(&tok);
            if (IsNone(nameBuffer)) {
                info->extraModel = -1;
            } else {
                GetModelInfo(nameBuffer, &info->extraModel);
            }
        }
        Tokenizer_Next(&tok);
        int32_t value = Tokenizer_GetInt(&tok);
        info->id = uint16_t((info->id & 0x8000) | (value & 0x7FFF));
        Tokenizer_Next(&tok);
        value = Tokenizer_GetInt(&tok);
        info->id = uint16_t((uint32_t(value) << 15) | (info->id & 0x7FFF));
        Tokenizer_Next(&tok);
        value = Tokenizer_GetInt(&tok);
        info->flags2 = uint16_t((info->flags2 & ~1) | (value & 1));
        Tokenizer_Next(&tok);
        CopyToken(&tok);
        Tokenizer_Next(&tok);
        if (*tok.cur == '\0') {
            // an override: this model uses the named model's entry. (The entry isn't
            // counted: the next line reuses it, but after the last line its model and
            // first fields stay, so SetModelObjectInfoIndices sees that model there)
            ObjectInfoOverride* entry = &objectInfoOverrides[overrideCount];
            entry->model = int16_t(info->model);
            int32_t other = -1;
            if (ms_bExtraObjectData) {
                int16_t index = int16_t(Atoi(nameBuffer));
                overrideCount++;
                entry->index = index;
            } else {
                GetModelInfo(nameBuffer, &other);
                overrideCount++;
                entry->index = int16_t(other);
            }
            continue;
        }
        info->breakEffects[0] = FindEffect();
        CopyToken(&tok);
        info->breakEffects[1] = FindEffect();
        Tokenizer_Next(&tok);
        info->breakHeights[0] = int16_t(Ftol(double(Tokenizer_GetFloat(&tok)) * 256.0));
        Tokenizer_Next(&tok);
        info->breakHeights[1] = int16_t(Ftol(double(Tokenizer_GetFloat(&tok)) * 256.0));
        Tokenizer_Next(&tok);
        CopyToken(&tok);
        info->hitEffect = FindEffect();
        Tokenizer_Next(&tok);
        CopyToken(&tok);
        info->soundBank = IsNone(nameBuffer) ? uint16_t(0xFFFF) : uint16_t(SoundBanks_Find(soundBanks, nameBuffer));
        Tokenizer_Next(&tok);
        value = Tokenizer_GetInt(&tok);
        info->bankRangeSq = value * value;
        Tokenizer_Next(&tok);
        CopyToken(&tok);
        info->emitter &= 0xFE;
        info->unknown22 &= 0xFE;
        if (Stricmp(nameBuffer, "None") == 0) {
            info->ambientSound = 0;
        } else if (char* dot = Strchr(nameBuffer, '.')) {
            if (Stricmp(dot + 1, "rsm") == 0) {
                info->emitter |= 1;
                info->ambientSound = HashStreamName(nameBuffer, 1);
            } else {
                // (the ambient sound left as it was)
                DebugPrintf(0, "Warning! %s is invalid stream name in object.dat.\n", nameBuffer);
            }
        } else if (Stricmp(nameBuffer, "AttachToEmitter") == 0) {
            info->unknown22 |= 1;   // (the ambient sound left as it was)
        } else {
            info->ambientSound = Sounds_GetId(sounds, nameBuffer);
        }
        Tokenizer_Next(&tok);
        float volume = Tokenizer_GetFloat(&tok);
        if (volume < 0.0f) {
            volume = 0.0f;
        } else if (1.0f < volume) {
            volume = 1.0f;
        }   // (NaN kept)
        // (fistp truncating; the product rounded to float as the x87's single precision)
        float scaled = float(double(volume) * 100.0);
        int32_t percent = std::isnan(scaled) ? INT32_MIN : int32_t(scaled);
        info->emitter = uint8_t(uint8_t(percent) * 2 | (info->emitter & 1));
        Tokenizer_Next(&tok);
        CopyToken(&tok);
        info->breakSound = IsNone(nameBuffer) ? 0 : Sounds_GetId(sounds, nameBuffer);
        Tokenizer_Next(&tok);
        CopyToken(&tok);
        info->hitSound = IsNone(nameBuffer) ? 0 : Sounds_GetId(sounds, nameBuffer);
        Tokenizer_Next(&tok);
        uint8_t size = GetSoundSizeFromName(&tok);   // (the tokenizer's first dword: the token)
        info->soundSizes = uint8_t((info->soundSizes & 0xF0) | (size & 0xF));
        Tokenizer_Next(&tok);
        size = GetSoundSizeFromName(&tok);
        info->soundSizes = uint8_t((info->soundSizes & 0xF) | uint8_t(size << 4));
        info->hash = defaultInfoHash;
        info->boneInfo = nullptr;
        info->height = 0x33;
        Tokenizer_Next(&tok);
        if (*tok.cur != '\0') {
            CopyToken(&tok);
            info->hash = HashName(nameBuffer);
            Tokenizer_Next(&tok);
            if (*tok.cur != '\0') {
                CopyToken(&tok);
                uint32_t names[3];
                uint8_t bonePath[8];
                CopyInt(&names[0], boneNameKey1);
                CopyInt(&names[1], boneNameKey2);
                NameKey_Construct(&names[2], nameBuffer);
                Path_Build(bonePath, 3, names);
                info->boneInfo = FindBoneInfo(bonePath, boneNames);
                for (int i = 2; i >= 0; i--) {
                    EmptyThiscall(&names[i]);   // (the names' destructors)
                }
            }
            Tokenizer_Next(&tok);
            if (*tok.cur != '\0') {
                info->height = int16_t(Ftol(double(Tokenizer_GetFloat(&tok)) * 256.0));
            }
        }
        objectInfoCount++;
    }
    // the overrides' models made the index of the entry with that model (-1: none)
    for (int k = 0; k < kOverrides; k++) {
        int32_t model = objectInfoOverrides[k].index;
        int16_t index = -1;
        for (int i = 0; i < kObjectInfos; i++) {
            if (objectInfos[i].model == model) {
                index = int16_t(i);
                break;
            }
        }
        objectInfoOverrides[k].index = index;
    }
    if (!ms_bSkipLevel2) {
        SetModelObjectInfoIndices();
    }
    if (file.data) {
        Free(file.data);
    }
}

} // namespace c11objdata

// ---- registration ---------------------------------------------------------------------

#define OBJDATA_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

OBJDATA_REVERSED("CObjectData::Initialise", 0x004D0DE0, &c11objdata::Initialise);
