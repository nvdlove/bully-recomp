// The audio environments (the reverb presets of 0xAE64A8, 0x38 bytes each, name first):
// audio\CONFIG\revxbox.txt gives each area its presets by trigger and time of day; drunk
// or tired, the player hears the preset of +0xC. 0x5936C0..0x593AC0.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct CVector {
    float x, y, z;
};

// an area's environment (0xC bytes)
struct ReverbRecord {
    int32_t preset;     // +0 into the presets (-1: none)
    uint32_t hash;      // +4 the trigger it's in (0: the whole area)
    uint8_t hourStart;  // +8
    uint8_t minStart;   // +9
    uint8_t hourEnd;    // +0xA
    uint8_t minEnd;     // +0xB
};
static_assert(sizeof(ReverbRecord) == 0xC);

// the areas' lists (0xCF5AA4, 64 of 0xC bytes)
struct AreaReverbs {
    uint8_t unknown0;
    uint8_t count;            // +1
    uint8_t pad2[6];
    re::Ptr32<ReverbRecord> records;    // +8
};
static_assert(sizeof(AreaReverbs) == 0xC);

// what revxbox.txt's lines give before they're sorted by area (the scratch buffer at 0xBD7800)
struct ReverbLine {
    ReverbRecord record;   // +0
    int32_t area;          // +0xC
};
static_assert(sizeof(ReverbLine) == 0x10);

struct AudioEnvironment {
    ReverbRecord normal;        // +0 (preset 0x16)
    ReverbRecord impaired;      // +0xC drunk or tired (0xB)
    ReverbRecord unknown18;     // +0x18 (0x1E)
    uint8_t state;              // +0x24 0x17 the area's, 0x20 a preset set here
    uint8_t pad25[3];
    int32_t mode;               // +0x28 0, 1 normal, 2 impaired
    re::Ptr32<ReverbRecord> current;      // +0x2C
    int32_t area;               // +0x30 (-1 while a preset of ours is set)
};
static_assert(offsetof(AudioEnvironment, state) == 0x24);
static_assert(offsetof(AudioEnvironment, area) == 0x30);

struct LineFile {
    re::Ptr32<char> data;     // +0 (freed after)
    int32_t left;   // +4
    int32_t end;    // +8
};
static_assert(sizeof(LineFile) == 0xC);

constexpr int kNumAreas = 0x40;
constexpr uintptr_t kPresets = 0x00AE64A8;
constexpr uintptr_t kPresetsEnd = 0x00AE6B70;
constexpr int kPresetSize = 0x38;

GAME_ARRAY(AreaReverbs, areaReverbs, 0x00CF5AA4);
GAME_ARRAY(ReverbLine, reverbLines, 0x00BD7800);
GAME_VAR(int32_t, skipFrames, 0x00CF3BC4);         // (updates every fourth frame)
GAME_VAR(int32_t, drunkActive, 0x00CF107C);
GAME_VAR(float, drunkLevel, 0x00CF1080);
GAME_VAR(int32_t, tiredActive, 0x00CF109C);
GAME_VAR(float, tiredLevel, 0x00CF10A0);
GAME_VAR(int32_t, currArea, 0x00BD1008);
inline void* const theSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const triggers = reinterpret_cast<void*>(0x020C7E98);

GAME_FN(void, __cdecl, Environment_Set, 0x005AFA10, void* preset, int arg);
GAME_FN(void, __cdecl, Environment_SetDefault, 0x005AFA50);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(uint8_t, __thiscall, LineFile_Load, 0x00426610, LineFile* file, const char* path);
GAME_FN(uint8_t, __cdecl, LineFile_ReadLine, 0x0042D600, LineFile* file, char* line, int size);
GAME_FN(const char*, __cdecl, GetAreaName, 0x004305B0, int area);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, Sounds_AddTrigger, 0x005960A0, void* sounds, uint32_t hash);
GAME_FN(int, __thiscall, Sounds_FindTrigger, 0x00596120, void* sounds, uint32_t hash);
GAME_FN(void*, __thiscall, Triggers_Get, 0x00415FD0, void* triggers, int index);
GAME_FN(uint8_t, __thiscall, Area_Contains, 0x006D6FA0, void* area, const CVector* point);
GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(uint8_t, __cdecl, IsTimeBetween, 0x00593E80, int hourStart, int hourEnd, int minStart, int minEnd);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// its own
GAME_FN(void, __thiscall, Environment_SetPreset, 0x005936C0, AudioEnvironment* self, ReverbRecord* record);
GAME_FN(void, __cdecl, Environment_FreeAreas, 0x00593700);
GAME_FN(int, __thiscall, Environment_FindPreset, 0x00593730, AudioEnvironment* self, const char* name);
GAME_FN(void, __stdcall, Environment_Apply, 0x005937A0, ReverbRecord* record);

// 0x5936C0 (thiscall): one of our presets (with the area's current one's index)
void __fastcall Environment_SetPresetImpl(AudioEnvironment* self, re::Edx, ReverbRecord* record) {
    int arg = self->current != nullptr ? self->current->preset : 0x1E;
    Environment_Set(reinterpret_cast<uint8_t*>(kPresets + 4) + record->preset * kPresetSize, arg);
    self->state = 0x20;
}

// 0x593700: the areas' lists freed
void __cdecl Environment_FreeAreasImpl() {
    for (int i = 0; i < kNumAreas; i++) {
        if (areaReverbs[i].records != nullptr) {
            Free(areaReverbs[i].records);
        }
        areaReverbs[i].count = 0;
        areaReverbs[i].records = nullptr;
    }
}

// 0x593730 (thiscall, ecx unused): a preset by name (-1: none)
int __fastcall Environment_FindPresetImpl(void*, re::Edx, const char* name) {
    int index = 0;
    for (uintptr_t preset = kPresets; preset < kPresetsEnd; preset += kPresetSize, index++) {
        if (Stricmp(name, *reinterpret_cast<re::Ptr32<const char>*>(preset)) == 0) {
            return index;
        }
    }
    return -1;
}

// 0x593780 (thiscall): back to the default environment
void __fastcall Environment_Reset(AudioEnvironment* self) {
    self->state = 0x17;
    self->mode = 0;
    Environment_SetDefault();
    self->current = nullptr;
}

// 0x5937A0 (thiscall, ecx unused): an area's record applied
void __stdcall Environment_ApplyImpl(ReverbRecord* record) {
    int preset = record->preset;
    if (preset == -1) {
        Environment_SetDefault();
        return;
    }
    Environment_Set(reinterpret_cast<uint8_t*>(kPresets + 4) + preset * kPresetSize, preset);
}

// 0x5937D0 (thiscall)
void __fastcall Environment_Init(AudioEnvironment* self) {
    self->mode = 0;
    Environment_SetDefault();
    self->unknown18.preset = 0x1E;
    self->normal.preset = 0x16;
    self->impaired.preset = 0xB;
    self->state = 0x17;
}

// 0x593800 (thiscall): revxbox.txt read ("area preset h:m h:m", or "area trigger preset
// h:m h:m")
void __fastcall Environment_Load(AudioEnvironment* self) {
    // (kept from line to line: a line that doesn't parse keeps the last one's)
    int32_t hourStart = 0;   // [esp+0x24]
    int32_t minStart = 0;    // [esp+0x20]
    int32_t hourEnd = 0;     // [esp+0x18]
    int32_t minEnd = 0;      // [esp+0x1C]
    int numLines = 0;    // [esp+0x14]
    Environment_FreeAreas();
    LineFile file{nullptr, 0, 0};
    char presetName[64];
    char areaName[64];
    char triggerName[64];
    char line[512];
    if (LineFile_Load(&file, "audio\\CONFIG\\revxbox.txt") != 0 && LineFile_ReadLine(&file, line, 0x200) != 0) {
        ReverbLine* out = reverbLines;
        do {
            if (line[0] != '#' && std::strlen(line) >= 8) {
                Sscanf(line, "%s\t%s\t%d:%d\t%d:%d\n", areaName, presetName, &hourStart, &minStart, &hourEnd,
                       &minEnd);
                int preset = Environment_FindPreset(self, presetName);
                uint32_t hash;
                if (preset == -1) {
                    Sscanf(line, "%s\t%s\t%s\t%d:%d\t%d:%d\n", areaName, triggerName, presetName, &hourStart, &minStart,
                           &hourEnd, &minEnd);
                    preset = Environment_FindPreset(self, presetName);
                    hash = HashName(triggerName);
                } else {
                    hash = 0;
                }
                for (int area = 0; area < kNumAreas; area++) {
                    if (std::strcmp(areaName, GetAreaName(area)) != 0) {
                        continue;
                    }
                    out->area = area;
                    out->record.preset = preset;
                    out->record.hash = hash;
                    if (hash != 0) {
                        Sounds_AddTrigger(theSounds, hash);
                    }
                    numLines++;
                    out->record.hourStart = uint8_t(hourStart);
                    out->record.minStart = uint8_t(minStart);
                    areaReverbs[area].count++;
                    out->record.hourEnd = uint8_t(hourEnd);
                    out->record.minEnd = uint8_t(minEnd);
                    out++;
                    break;
                }
            }
        } while (LineFile_ReadLine(&file, line, 0x200) != 0);
    }
    for (int area = 0; area < kNumAreas; area++) {
        AreaReverbs& reverbs = areaReverbs[area];
        if (reverbs.count == 0) {
            continue;
        }
        reverbs.records = static_cast<ReverbRecord*>(Malloc(reverbs.count * sizeof(ReverbRecord)));
        uint32_t filled = 0;
        for (int i = 0; i < numLines; i++) {
            if (reverbLines[i].area != area) {
                continue;
            }
            reverbs.records[filled] = reverbLines[i].record;
            filled++;
            if (filled == reverbs.count) {
                break;
            }
        }
    }
    if (file.data != nullptr) {
        Free(file.data);
    }
}

// 0x593AC0 (thiscall): every fourth frame: drunk or tired past 0.8 the impaired preset,
// else the area's record whose time and trigger hold for the camera
void __fastcall Environment_Update(AudioEnvironment* self) {
    if (skipFrames != 0) {
        int32_t next = skipFrames + 1;
        skipFrames = next;
        if (next > 3) {
            skipFrames = 0;
        }
        return;
    }
    float level = 0.0f;
    CVector camera;
    GetCameraPos(&camera);
    int32_t drunk = drunkActive;
    int32_t tired = tiredActive;
    bool impaired = false;
    if (drunk == 1) {
        if (tired == 1) {
            level = drunkLevel < tiredLevel ? tiredLevel : drunkLevel;
        } else {
            level = drunkLevel;
        }
        impaired = true;
    } else if (tired == 1) {
        level = tiredLevel;
        impaired = true;
    }
    if (impaired && self->mode == 0) {
        if (double(level) < 0.8) {
            Environment_SetPreset(self, &self->normal);
            self->mode = 1;
            return;
        }
        Environment_SetPreset(self, &self->impaired);
        self->mode = 2;
        return;
    }
    int32_t mode = self->mode;
    if (mode > 0) {
        if (drunk == 0 && tired == 0) {
            self->state = 0x17;
            self->mode = 0;
            Environment_SetDefault();
            self->current = nullptr;
            return;
        }
        if (mode == 1) {
            if (0.8 <= double(level)) {
                Environment_SetPreset(self, &self->impaired);
                self->mode = 2;
                return;
            }
        } else if (mode == 2) {
            if (double(level) < 0.8) {
                Environment_SetPreset(self, &self->normal);
                self->mode = 1;
                return;
            }
        }
    }
    if (self->state != 0x17) {
        self->area = -1;
        return;
    }
    int32_t area = currArea;
    AreaReverbs& reverbs = areaReverbs[area];
    for (int i = 0; i < reverbs.count; i++) {
        ReverbRecord* record = &reverbs.records[i];
        if (IsTimeBetween(record->hourStart, record->hourEnd, record->minStart, record->minEnd) == 0) {
            continue;
        }
        uint32_t hash = reverbs.records[i].hash;
        if (hash != 0) {
            int index = Sounds_FindTrigger(theSounds, hash);
            if (index == -1) {
                continue;
            }
            void* trigger = Triggers_Get(triggers, index);
            if (trigger == nullptr || Area_Contains(trigger, &camera) == 0) {
                continue;
            }
        }
        ReverbRecord* found = &reverbs.records[i];
        if (self->current == found) {
            self->area = area;
            return;
        }
        if (self->current == nullptr) {
            Environment_Apply(found);
            self->current = &reverbs.records[i];
            self->area = area;
            return;
        }
        break;
    }
    if (self->current != nullptr) {
        self->current = nullptr;
        Environment_SetDefault();
    }
    self->area = area;
}

// ---- registration -----------------------------------------------------------------------------

#define AUDIOENV_REVERSED(name, address, fn)                                                            \
    static void* RE_CONCAT(audioenv_original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(audioenv_original_, __LINE__), \
             re::Mode::Original)

AUDIOENV_REVERSED("AudioEnvironment::SetPreset", 0x005936C0, &Environment_SetPresetImpl);
AUDIOENV_REVERSED("AudioEnvironment::FreeAreas", 0x00593700, &Environment_FreeAreasImpl);
AUDIOENV_REVERSED("AudioEnvironment::FindPreset", 0x00593730, &Environment_FindPresetImpl);
AUDIOENV_REVERSED("AudioEnvironment::Reset", 0x00593780, &Environment_Reset);
AUDIOENV_REVERSED("AudioEnvironment::Apply", 0x005937A0, &Environment_ApplyImpl);
AUDIOENV_REVERSED("AudioEnvironment::Init", 0x005937D0, &Environment_Init);
AUDIOENV_REVERSED("AudioEnvironment::Load", 0x00593800, &Environment_Load);
AUDIOENV_REVERSED("AudioEnvironment::Update", 0x00593AC0, &Environment_Update);

} // namespace
