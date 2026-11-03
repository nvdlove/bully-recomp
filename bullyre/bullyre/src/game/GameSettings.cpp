// The settings' defaults (the block at 0xC18700) with the language from the
// installer's registry key, and the controls (the button map at 0xA14810): its
// defaults, the "ControllerSettings" file, the temporary key sets.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"
#include "runflags.h"

#include "game/Crt.h"
namespace {

// ---- the settings ----

struct GsSettings {
    uint8_t unknown0;      // +0
    uint8_t unknown1;      // +1
    uint8_t pad2[2];
    int32_t unknown4;      // +4
    int32_t unknown8;      // +8 (0xD7)
    uint8_t subtitles;     // +0xC (on for languages other than English)
    uint8_t unknownD;      // +0xD
    uint8_t widescreen;    // +0xE (0xC1870E; not set here)
    uint8_t unknownF;      // +0xF
    uint8_t unknown10;     // +0x10
    uint8_t unknown11;     // +0x11
    uint8_t unknown12;     // +0x12
    uint8_t unknown13;     // +0x13
    uint8_t pad14[4];
    int32_t volume18;      // +0x18
    int32_t volume1C;      // +0x1C
    uint8_t volume20;      // +0x20
    uint8_t unknown21;     // +0x21
    uint8_t pad22[2];
    int32_t language;      // +0x24 0 US English, 1 French, 2 German, 3 Italian, 4 Spanish,
                           //       5 English (elsewhere), 6 Russian, 7 Japanese
    int32_t textLanguage;  // +0x28 (the same)
    float unknown2C;       // +0x2C
    float unknown30;       // +0x30
    uint8_t unknown34;     // +0x34
    uint8_t unknown35;     // +0x35 (off for German and French)
    uint8_t unknown36;     // +0x36
};
static_assert(offsetof(GsSettings, language) == 0x24);
static_assert(offsetof(GsSettings, unknown36) == 0x36);

GAME_VAR(uint8_t, gs_bWidescreen, 0x00C1870E);
// the screen's scales: x, 1, 1/x, 1 and y, 1, 1/y, 1
GAME_ARRAY(float, gs_screenScales, 0x00A957A8);

// the registry, through the game's imports
using RegOpenKeyA_t = long(__stdcall*)(void* key, const char* subKey, void** result);   // (a system handle: the system's pointer size)
using RegQueryValueExA_t = long(__stdcall*)(void* key, const char* name, uint32_t* reserved, uint32_t* type, void* data,
                                            uint32_t* size);
using RegCloseKey_t = long(__stdcall*)(void* key);
GAME_VAR(RegOpenKeyA_t, gs_RegOpenKeyA, 0x008FC00C);
GAME_VAR(RegQueryValueExA_t, gs_RegQueryValueExA, 0x008FC000);
GAME_VAR(RegCloseKey_t, gs_RegCloseKey, 0x008FC014);
inline const char* const gs_keyName = reinterpret_cast<const char*>(0x00906604);    // "SOFTWARE\Rockstar Games\Bully Scholarship Edition"
inline const char* const gs_valueName = reinterpret_cast<const char*>(0x009065F8);  // "LANGUAGE"
inline void* const gs_HKEY_LOCAL_MACHINE = reinterpret_cast<void*>(uintptr_t(0x80000002));

GAME_FN(uint32_t, __cdecl, GS_GetInstallLanguage, 0x00454630);

// ---- the controls ----

struct GsButtonMap {
    int32_t masks[16];      // +0 the pad's button bits
    int32_t keys[25];       // +0x40 the keyboard's
    int32_t savedKeys[25];  // +0xA4 (the other set while a temporary one is in)
    uint8_t temporary;      // +0x108
    uint8_t pad109[3];
    float unknown10C;       // +0x10C
    float saved10C;         // +0x110
    uint8_t unknown114;     // +0x114
    uint8_t unknown115;     // +0x115
    uint8_t unknown116;     // +0x116
};
static_assert(offsetof(GsButtonMap, temporary) == 0x108);
static_assert(offsetof(GsButtonMap, unknown116) == 0x116);

// the "ControllerSettings" file (0xEC)
struct GsControllerFile {
    int32_t masks[16];      // +0
    int32_t keys[25];       // +0x40
    float unknown10C;       // +0xA4
    uint8_t unknown114;     // +0xA8
    uint8_t unknown115;     // +0xA9
    uint8_t unknown116;     // +0xAA
    uint8_t padAB[0x41];
};
static_assert(sizeof(GsControllerFile) == 0xEC);

GAME_VAR(GsButtonMap, gs_buttonMap, 0x00A14810);
GAME_VAR(int32_t, gs_BA6890, 0x00BA6890);
GAME_VAR(uint8_t, gs_20C32E9, 0x020C32E9);
GAME_VAR(void*, gs_fileSystem, 0x020CE75C);
inline const char* const gs_controllerFile = reinterpret_cast<const char*>(0x00906638);   // "ControllerSettings"
inline const char* const gs_modeRead = reinterpret_cast<const char*>(0x008FF548);         // "r"
inline const char* const gs_modeWrite = reinterpret_cast<const char*>(0x00901AE8);        // "wb"

GAME_FN(int, __thiscall, GS_FileSystem_Open, 0x00735750, void* fs, const char* name, const char* mode);
inline void* __cdecl GS_memset(void* dest, int value, size_t count) { return crt::Memset(dest, value, count); }
GAME_FN(int, __cdecl, GS_File_Read, 0x0042D2A0, int file, void* buffer, int size);
GAME_FN(int, __cdecl, GS_File_Write, 0x0042D2D0, int file, const void* buffer, int size);
GAME_FN(void, __cdecl, GS_File_Close, 0x0042D450, int file);
GAME_FN(void*, __cdecl, GS_Input_Get, 0x00738070);
GAME_FN(uint8_t, __cdecl, GS_Input_738500, 0x00738500, void* input);
GAME_FN(void, __cdecl, GS_Input_738650, 0x00738650, int);

} // namespace

namespace GameSettings {

// 0x454630: the installer's LANGUAGE value (a Windows LANGID; 9 when missing)
uint32_t __cdecl GetInstallLanguage() {
    void* key;
    if (gs_RegOpenKeyA(gs_HKEY_LOCAL_MACHINE, gs_keyName, &key) != 0) {
        return 9;
    }
    uint32_t size = 4;
    uint32_t type;
    uint32_t value;
    long result = gs_RegQueryValueExA(key, gs_valueName, nullptr, &type, &value, &size);
    gs_RegCloseKey(key);
    if (result != 0 || type != 4 || size != 4) {
        return 9;
    }
    return value;
}

// 0x4546B0 (thiscall): the defaults; the language from the installer's (by its
// LANGID), the screen scales (0.75 in widescreen)
void __fastcall SetDefaults(GsSettings* self, re::Edx) {
    self->unknown2C = 0.0f;
    self->unknown30 = 0.0f;
    self->volume18 = 8;
    self->volume1C = 8;
    self->volume20 = 8;
    self->unknown21 = 1;
    self->unknown8 = 0xD7;
    self->unknownD = 1;
    self->unknownF = 0;
    self->unknown10 = 1;
    self->unknown11 = 1;
    self->unknown13 = 1;
    self->unknown0 = 1;
    self->unknown1 = 1;
    self->unknown4 = 0;
    self->unknown34 = 0xFF;
    self->unknown36 = 0;
    self->unknown35 = 1;
    int language;
    switch (GS_GetInstallLanguage()) {
    case 0x07: case 0x407: case 0x807: case 0xC07: case 0x1007: case 0x1407:
        language = 2;
        self->unknown35 = 0;
        break;
    case 0x0C: case 0x40C: case 0x80C: case 0xC0C: case 0x100C: case 0x140C:
        self->language = 1;
        self->textLanguage = 1;
        self->unknown35 = 0;
        self->subtitles = 1;
        goto scales;
    case 0x10: case 0x410: case 0x810:
        language = 3;
        break;
    case 0x11: case 0x411:
        self->language = 7;
        self->textLanguage = 7;
        self->subtitles = 1;
        goto scales;
    case 0x19: case 0x419:
        language = 6;
        break;
    case 0x0A: case 0x40A: case 0x80A: case 0xC0A: case 0x100A: case 0x140A: case 0x180A: case 0x1C0A: case 0x200A:
    case 0x240A: case 0x280A: case 0x2C0A: case 0x300A: case 0x340A: case 0x380A: case 0x3C0A: case 0x400A: case 0x440A:
    case 0x480A: case 0x4C0A: case 0x500A:
        language = 4;
        break;
    case 0x809: case 0xC09: case 0x1009: case 0x1409: case 0x1809: case 0x1C09: case 0x2009: case 0x2409: case 0x2809:
    case 0x2C09:
        self->language = 5;
        self->textLanguage = 5;
        self->subtitles = 0;
        goto scales;
    default:
        self->language = 0;
        self->textLanguage = 0;
        self->subtitles = 0;
        goto scales;
    }
    self->textLanguage = language;
    self->language = language;
    self->subtitles = 1;
scales:
    float x;
    float y;
    if (gs_bWidescreen) {
        x = float(0.75 / re::WiderThan169());
        y = x;
    } else {
        x = 1.0f;
        y = 1.0f;
    }
    gs_screenScales[0] = x;
    gs_screenScales[1] = 1.0f;
    gs_screenScales[2] = 1.0f / x;
    gs_screenScales[3] = 1.0f;
    gs_screenScales[4] = y;
    gs_screenScales[5] = 1.0f;
    gs_screenScales[6] = 1.0f / y;
    gs_screenScales[7] = 1.0f;
}

} // namespace GameSettings

namespace ButtonMap {

// 0x4549E0 (thiscall): the pad's button bits
void __fastcall SetDefaultMasks(GsButtonMap* self, re::Edx) {
    static const int32_t masks[16] = {4, 8, 1, 2, 0x20, 0x10, 0x4000, 0x1000, 0x2000, 0x8000, 0x400, 0x100, 0x800, 0x200, 0x40, 0x80};
    for (int i = 0; i < 16; i++) {
        self->masks[i] = masks[i];
    }
}

// 0x454A50: the "ControllerSettings" file read in (when it's whole)
void __fastcall Load(GsButtonMap* self, re::Edx) {
    int file = GS_FileSystem_Open(gs_fileSystem, gs_controllerFile, gs_modeRead);
    if (file <= 0) {
        return;
    }
    GsControllerFile data;
    GS_memset(&data, 0, sizeof(data));
    if (GS_File_Read(file, &data, sizeof(data)) == int(sizeof(data))) {
        self->unknown10C = data.unknown10C;
        self->unknown114 = data.unknown114;
        self->unknown115 = data.unknown115;
        self->unknown116 = data.unknown116;
        for (int i = 0; i < 16; i++) {
            self->masks[i] = data.masks[i];
        }
        for (int i = 0; i < 25; i++) {
            self->keys[i] = data.keys[i];
        }
    }
    GS_File_Close(file);
}

// 0x454B70: the file written, from the map at 0xA14810 (the keys: the temporary set
// when this one has one in, else the current)
void __fastcall Save(GsButtonMap* self, re::Edx) {
    int file = GS_FileSystem_Open(gs_fileSystem, gs_controllerFile, gs_modeWrite);
    if (file <= 0) {
        return;
    }
    GsControllerFile data;
    GS_memset(&data, 0, sizeof(data));
    data.unknown10C = gs_buttonMap.unknown10C;
    data.unknown114 = GS_Input_738500(GS_Input_Get());
    data.unknown115 = gs_buttonMap.unknown115;
    data.unknown116 = gs_buttonMap.unknown116;
    for (int i = 0; i < 16; i++) {
        data.masks[i] = gs_buttonMap.masks[i];
    }
    uint8_t temporary = self->temporary;
    for (int i = 0; i < 25; i++) {
        data.keys[i] = temporary ? gs_buttonMap.savedKeys[i] : gs_buttonMap.keys[i];
    }
    GS_File_Write(file, &data, sizeof(data));
    GS_File_Close(file);
}

// 0x454CD0: the keys in use: the saved set while a temporary one is in, when asked
// for or 0xBA6890 isn't -1 or 0x20C32E9 is set
int32_t* __fastcall GetKeys(GsButtonMap* self, re::Edx, int saved) {
    if (self->temporary && (uint8_t(saved) || gs_BA6890 != -1 || gs_20C32E9)) {
        return self->savedKeys;
    }
    return self->keys;
}

// 0x454D10: a temporary key set in (the current one saved)
void __fastcall SetTemporaryKeys(GsButtonMap* self, re::Edx, const int32_t* keys) {
    for (int i = 0; i < 25; i++) {
        self->savedKeys[i] = self->keys[i];
        self->keys[i] = keys[i];
    }
    self->temporary = 1;
    if (!GS_Input_738500(GS_Input_Get())) {
        GS_Input_738650(0);
    }
}

// 0x454D60: the saved key set back
void __fastcall RestoreKeys(GsButtonMap* self, re::Edx) {
    if (!self->temporary) {
        return;
    }
    if (!GS_Input_738500(GS_Input_Get())) {
        GS_Input_738650(1);
    }
    self->temporary = 0;
    std::memcpy(self->keys, self->savedKeys, sizeof(self->keys));
}

// 0x454DB0: +0x10C raised to at least `value` (the old one kept)
void __fastcall Raise10C(GsButtonMap* self, re::Edx, float value) {
    self->saved10C = self->unknown10C;
    if (self->unknown10C < value) {
        self->unknown10C = value;
    }
}

// 0x454DE0: +0x10C back
void __fastcall Restore10C(GsButtonMap* self, re::Edx) {
    self->unknown10C = self->saved10C;
    self->saved10C = -1.0f;
}

// 0x454E00: the default keys (DirectInput scan codes; 0x100+ the mouse), into the
// set in use
void __fastcall SetDefaultKeys(GsButtonMap* self, re::Edx) {
    int32_t* keys = self->temporary ? self->savedKeys : self->keys;
    keys[0] = 0xCB;
    keys[1] = 0xCD;
    keys[2] = 0xC8;
    keys[3] = 0xD0;
    keys[4] = 0xF;
    keys[5] = 1;
    keys[6] = 0x2A;
    keys[7] = 0x38;
    keys[8] = 0x39;
    keys[9] = 0x13;
    keys[10] = 0x101;
    keys[11] = 0x104;
    keys[12] = 0x100;
    keys[13] = 0x105;
    keys[14] = 0x2D;
    keys[15] = 0x2E;
    keys[16] = 0x1E;
    keys[17] = 0x11;
    keys[20] = 0x20;
    keys[21] = 0x1F;
    keys[18] = 0x108;
    keys[19] = 0x106;
    keys[22] = 0x109;
    keys[23] = 0x107;
    keys[24] = 0x102;
}

} // namespace ButtonMap

// ---- registration ---------------------------------------------------------------------

#define C04C_GS_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04C_GS_REVERSED("GameSettings::GetInstallLanguage", 0x00454630, &GameSettings::GetInstallLanguage);
C04C_GS_REVERSED("GameSettings::SetDefaults", 0x004546B0, &GameSettings::SetDefaults);
C04C_GS_REVERSED("ButtonMap::SetDefaultMasks", 0x004549E0, &ButtonMap::SetDefaultMasks);
C04C_GS_REVERSED("ButtonMap::Load", 0x00454A50, &ButtonMap::Load);
C04C_GS_REVERSED("ButtonMap::Save", 0x00454B70, &ButtonMap::Save);
C04C_GS_REVERSED("ButtonMap::GetKeys", 0x00454CD0, &ButtonMap::GetKeys);
C04C_GS_REVERSED("ButtonMap::SetTemporaryKeys", 0x00454D10, &ButtonMap::SetTemporaryKeys);
C04C_GS_REVERSED("ButtonMap::RestoreKeys", 0x00454D60, &ButtonMap::RestoreKeys);
C04C_GS_REVERSED("ButtonMap::Raise10C", 0x00454DB0, &ButtonMap::Raise10C);
C04C_GS_REVERSED("ButtonMap::Restore10C", 0x00454DE0, &ButtonMap::Restore10C);
C04C_GS_REVERSED("ButtonMap::SetDefaultKeys", 0x00454E00, &ButtonMap::SetDefaultKeys);
