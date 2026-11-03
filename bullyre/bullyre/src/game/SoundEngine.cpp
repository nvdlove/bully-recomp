// The cue engine (0x5A9000..0x5AA932): the engine at 0xCF7268 (+0 the global grain
// registers (bytes, indexed -1 downwards from +0), +0x20 128 voice handles, +0x220 32
// banks of 0x140 bytes, +0x2A20 64 sound instances of 0x40 bytes), the banks' loading
// from audio\xact\<name>.sea (asynchronously with overlapped reads, or at once), and the
// instances that run a sound's grain program (see SoundGrains.cpp).
//
// A bank's sounds (0x58 each): +0 grain count, +4 grains (8-byte objects), +8 hash, +0xC
// name, +0x4C instances, +0x50 cycle counter, +0x54 volume. The grain stride is the
// global at 0xCF7260 (8).

#include <cstdint>
#include <cstring>

#include "reversed.h"
#include "sysabi.h"

#include "game/Crt.h"
namespace {

using re::At;

// a sound instance (0x40 bytes)
struct SoundInstance {
    int32_t handle;          // +0
    uint8_t paused;          // +4
    uint8_t pad5;
    int16_t bank;            // +6
    int16_t sound;           // +8
    uint8_t loopAt;          // +0xA (a grain index)
    uint8_t loopTo;          // +0xB
    re::Ptr32<SoundInstance> children; // +0xC
    re::Ptr32<SoundInstance> parent;   // +0x10
    re::Ptr32<SoundInstance> next;     // +0x14
    re::Ptr32<void> grain;             // +0x18 (the current grain)
    int32_t state;           // +0x1C (0 run, 1 waiting, 2 timed wait, 3 children, 4, 5, 6 done)
    uint8_t used;            // +0x20
    uint8_t looping;         // +0x21
    uint8_t autoStop;        // +0x22
    uint8_t stopFlag;        // +0x23
    int8_t registers[4];     // +0x24
    uint32_t waitUntil;      // +0x28
    uint8_t voices[8];       // +0x2C (0xFF none)
    re::Ptr32<float> gains;            // +0x34
    uint32_t gainsCapacity;  // +0x38
    uint32_t gainsCount;     // +0x3C
};
static_assert(sizeof(SoundInstance) == 0x40);

inline uint8_t* const TheCueEngine = reinterpret_cast<uint8_t*>(0x00CF7268);
GAME_VAR(int32_t, grainStride, 0x00CF7260);
GAME_ARRAY(int32_t, voiceHandles, 0x00CF7288);          // TheCueEngine + 0x20
inline uint8_t* const cueBanks = reinterpret_cast<uint8_t*>(0x00CF7488);   // + 0x220
GAME_VAR(uint8_t*, bankTable, 0x00CF5DCC);              // 0x48-byte entries: name, +0x34 flags,
                                                          // +0x40 size, +0x44 file
inline char* const bankPath = reinterpret_cast<char*>(0x00CF7160);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint8_t, gamePaused1, 0x00C1A99A);
GAME_VAR(uint8_t, gamePaused2, 0x00C1A999);
GAME_VAR(uint8_t, gamePaused3, 0x00C1A998);
GAME_VAR(uint32_t, pausedTime, 0x00A14A50);

using CreateFileAFn = uintptr_t(__stdcall*)(const char*, uint32_t, uint32_t, void*, uint32_t, uint32_t, void*);
using ReadFileFn = int(__stdcall*)(uintptr_t, void*, uint32_t, uint32_t*, void*);
using GetOverlappedResultFn = int(__stdcall*)(uintptr_t, void*, uint32_t*, int);
using CreateEventAFn = uintptr_t(__stdcall*)(void*, int, int, const char*);
inline CreateFileAFn ImportCreateFileA() { return re::GameFnAt<CreateFileAFn>(uintptr_t(0x008FC064)); }
inline ReadFileFn ImportReadFile() { return re::GameFnAt<ReadFileFn>(uintptr_t(0x008FC06C)); }
inline GetOverlappedResultFn ImportGetOverlappedResult() {
    return re::GameFnAt<GetOverlappedResultFn>(uintptr_t(0x008FC090));
}
inline CreateEventAFn ImportCreateEventA() { return re::GameFnAt<CreateEventAFn>(uintptr_t(0x008FC08C)); }

GAME_FN(void*, __cdecl, New, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline void* __cdecl Memcpy(void* dst, const void* src, uint32_t size) { return crt::Memcpy(dst, src, (size_t)size); }
inline int __cdecl StrcpyS(char* dst, uint32_t size, const char* src) { return crt::StrcpyS(dst, (size_t)size, src); }
GAME_FN(const char*, __cdecl, FixPath, 0x00829900, const char* path);
GAME_FN(void*, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(int, __cdecl, File_Seek, 0x0042D300, void* file, int offset, int origin);
GAME_FN(int, __cdecl, File_Tell, 0x0042D430, void* file);
GAME_FN(int, __cdecl, File_Close, 0x0042D450, void* file);
GAME_FN(int, __cdecl, File_GetFileLength, 0x0042D210, const char* path);
GAME_FN(void, __cdecl, Sound_Lock, 0x005EF0A0, int);
GAME_FN(void, __cdecl, Sound_Unlock, 0x005EF100);
GAME_FN(void*, __cdecl, Grain_Create, 0x005AB630, int type, void* data, void* placement);
// the sound engine (0x5AF...)
GAME_FN(int32_t, __cdecl, Engine_Fn5AF150, 0x005AF150, void* data, int32_t size, int, uint32_t flags);
GAME_FN(void, __cdecl, Engine_Fn5AF1D0, 0x005AF1D0, int32_t handle);
GAME_FN(int32_t, __cdecl, Engine_Fn5AF210, 0x005AF210, int32_t handle);
GAME_FN(int32_t, __cdecl, Engine_Fn5AF220, 0x005AF220, void* data, int32_t size, int, uint32_t flags);
GAME_FN(void, __cdecl, Engine_Fn5AF2F0, 0x005AF2F0, int32_t handle);
GAME_FN(int32_t, __cdecl, Engine_Fn5AF340, 0x005AF340, int32_t handle);
GAME_FN(void, __cdecl, Engine_Fn5AF4C0, 0x005AF4C0, int32_t bank, int32_t voice, int);
GAME_FN(uint8_t, __cdecl, Engine_Fn5AF550, 0x005AF550, int32_t handle, uint32_t* state);
GAME_FN(void, __cdecl, Engine_Pause, 0x005AF5C0, int32_t handle, int pause);
GAME_FN(int32_t, __cdecl, Engine_Fn5AF9A0, 0x005AF9A0, int32_t handle);
// this file's
GAME_FN(void, __thiscall, Instance_AppendSibling, 0x005A9080, void* self, void* instance);
GAME_FN(uint8_t*, __thiscall, Instance_BankOf, 0x005A92E0, void* self);
GAME_FN(uint8_t*, __thiscall, Instance_SoundOf, 0x005A9310, void* self);
GAME_FN(float, __thiscall, Instance_Volume, 0x005A9350, void* self);
GAME_FN(void, __thiscall, Instance_Release, 0x005A93B0, void* self);
GAME_FN(void, __thiscall, Instance_Pause, 0x005A9470, void* self, int pause);
GAME_FN(void, __thiscall, Instance_SetGains, 0x005A94D0, void* self, void* gains);
GAME_FN(void, __thiscall, Instance_FreeVoice, 0x005A9630, void* self, int slot);
GAME_FN(void, __thiscall, Bank_SetLoadStep, 0x005A9700, void* self, int32_t step);
GAME_FN(uint8_t, __thiscall, Bank_Parse, 0x005A98E0, void* self, void* file);
GAME_FN(void, __thiscall, Instance_CollectVoices, 0x005A9B90, void* self, void* list);
GAME_FN(void, __thiscall, Instance_Init, 0x005A9C70, void* self, int bank, int sound, void* parent);
GAME_FN(void, __thiscall, Instance_ReleaseVoices, 0x005A9E00, void* self);
GAME_FN(SoundInstance*, __thiscall, Engine_NewInstance, 0x005A9E70, void* self, int bank, int sound, void* parent);
GAME_FN(uint8_t, __thiscall, Bank_Load, 0x005A9EC0, void* self, int32_t index, int async);
GAME_FN(void, __thiscall, Bank_Unload, 0x005AA0A0, void* self);
GAME_FN(void, __thiscall, Bank_Update, 0x005AA110, void* self);
GAME_FN(void, __thiscall, Instance_Stop, 0x005AA200, void* self, int flag);
GAME_FN(uint8_t, __thiscall, Instance_UpdateChildren, 0x005AA480, void* self);
GAME_FN(uint8_t, __thiscall, Instance_Update, 0x005AA510, void* self);
GAME_FN(int32_t, __thiscall, Bank_FindSound, 0x005A9220, void* self, uint32_t hash);
GAME_FN(SoundInstance*, __thiscall, Bank_PlaySound, 0x005AA840, void* self, int sound, void* gains);

// ---- the sounds' grain lists (thiscall on a sound) ------------------------------------------

inline int32_t GrainIndex(uint8_t* sound, void* grain) {
    return int32_t(static_cast<uint8_t*>(grain) - At<uint8_t*>(sound, 4)) / grainStride;
}

// 0x5A9000 (thiscall, a sound): a grain by index (4 bytes apart, like the game)
uint8_t* __fastcall Sound_GrainAt(uint8_t* self, re::Edx, int index) {
    return At<uint8_t*>(self, 4) + index * 4;
}

// 0x5A9010 (thiscall, a sound): the grain before one
uint8_t* __fastcall Sound_PrevGrain(uint8_t* self, re::Edx, uint8_t* grain) {
    if (grain == nullptr) {
        return nullptr;
    }
    if (GrainIndex(self, grain) <= 0) {
        return nullptr;
    }
    return grain - grainStride;
}

// 0x5A9040 (thiscall, a sound): the grain after one
uint8_t* __fastcall Sound_NextGrain(uint8_t* self, re::Edx, uint8_t* grain) {
    if (grain == nullptr) {
        return nullptr;
    }
    int32_t stride = grainStride;
    if (int32_t(grain - At<uint8_t*>(self, 4)) / stride >= At<int32_t>(self, 0) - 1) {
        return nullptr;
    }
    return grain + stride;
}

uint8_t* NextGrainOf(uint8_t* sound, uint8_t* grain) {
    if (grain == nullptr) {
        return nullptr;
    }
    int32_t stride = grainStride;
    if (int32_t(grain - At<uint8_t*>(sound, 4)) / stride >= At<int32_t>(sound, 0) - 1) {
        return nullptr;
    }
    return grain + stride;
}

// ---- an instance ---------------------------------------------------------------------------

// 0x5A9080 (thiscall): appends to the end of its sibling chain
void __fastcall Instance_AppendSiblingImpl(SoundInstance* self, re::Edx, SoundInstance* instance) {
    if (self->next == nullptr) {
        self->next = instance;
        return;
    }
    SoundInstance* last = self->next;
    while (last->next != nullptr) {
        last = last->next;
    }
    last->next = instance;
}

// 0x5A90C0 (thiscall): a free voice slot
uint8_t* __fastcall Instance_FreeVoiceSlot(SoundInstance* self) {
    for (int i = 0; i < 8; ++i) {
        if (self->voices[i] == 0xFF) {
            return &self->voices[i];
        }
    }
    return nullptr;
}

// 0x5A90E0 (thiscall): its voices
int32_t __fastcall Instance_VoiceCount(SoundInstance* self) {
    int32_t count = 0;
    for (int i = 0; i < 8; ++i) {
        if (self->voices[i] != 0xFF) {
            ++count;
        }
    }
    return count;
}

// 0x5A9130 (thiscall): jumps to a grain
void __fastcall Instance_SetGrain(SoundInstance* self, re::Edx, void* grain) {
    self->grain = grain;
    self->state = 0;
}

// 0x5A9150 (thiscall): the root instance's gains (the field's address)
re::Ptr32<float>* __fastcall Instance_RootGains(SoundInstance* self) {
    SoundInstance* root = self;
    while (root->parent != nullptr) {
        root = root->parent;
    }
    return &root->gains;
}

// 0x5A9170 (thiscall): adds a child (it shares the handle); waits for the children
void __fastcall Instance_AddChild(SoundInstance* self, re::Edx, SoundInstance* child) {
    if (self->children == nullptr) {
        self->children = child;
    } else {
        Instance_AppendSibling(self->children, child);
    }
    child->handle = self->handle;
    self->state = 3;
}

// 0x5A91B0 (thiscall, the engine): a free voice handle slot (its index, 0xFFFF none)
int32_t __fastcall Engine_FreeVoice(uint8_t* self, re::Edx, re::Ptr32<int32_t>* out) {
    *out = nullptr;
    for (int32_t i = 0; i < 0x80; ++i) {
        if (At<int32_t>(self, 0x20 + i * 4) == -1) {
            *out = &At<int32_t>(self, 0x20 + i * 4);
            return i;
        }
    }
    return 0xFFFF;
}

// 0x5A91F0 (thiscall, the engine): a bank's index (-1 for none)
int32_t __fastcall Engine_BankIndex(uint8_t* self, re::Edx, uint8_t* bank) {
    if (bank == nullptr) {
        return -1;
    }
    return int32_t(bank - self - 0x220) / 0x140;
}

// 0x5A9220 (thiscall, a bank): a sound by hash (a binary search; 0xFFFF none)
int32_t __fastcall Bank_FindSoundImpl(uint8_t* self, re::Edx, uint32_t hash) {
    int32_t count = At<int32_t>(self, 0x24);
    uint8_t* sounds = At<uint8_t*>(self, 0x28);
    int32_t low = 0;
    int32_t step = count;
    do {
        step >>= 1;
        uint32_t key = At<uint32_t>(sounds, (step + low) * 0x58 + 8);
        if (key == hash) {
            return step + low;
        }
        if (key < hash) {
            if (step == 0) {
                step = 1;
            }
            low += step;
        }
    } while (step != 0 && low < count);
    return 0xFFFF;
}

// 0x5A9270 (thiscall): waits ms (paused game: its own clock): true when done
uint8_t __fastcall Instance_Wait(SoundInstance* self, re::Edx, int32_t ms) {
    uint32_t now = (gamePaused1 != 0 || gamePaused2 != 0 || gamePaused3 != 0) ? pausedTime : m_snTimeInMilliseconds;
    if (ms > 0) {
        if (self->state == 1) {
            self->state = 2;
            self->waitUntil = now + uint32_t(ms);
            return 0;
        }
        if (self->state == 2) {
            if (self->waitUntil >= now) {
                return 0;
            }
            self->state = 6;
            self->waitUntil = 0xFFFFFFFF;
        }
    }
    return 1;
}

// 0x5A92E0 (thiscall): its bank (loaded) or null
uint8_t* __fastcall Instance_BankOfImpl(SoundInstance* self) {
    int32_t bank = self->bank;
    if (uint32_t(bank) > 0x1F) {
        return nullptr;
    }
    uint8_t* entry = cueBanks + bank * 0x140;
    if (At<uint8_t>(entry, 0x21) == 0) {
        return nullptr;
    }
    return entry;
}

uint8_t* SoundOf(SoundInstance* self) {
    int32_t bank = self->bank;
    if (uint32_t(bank) > 0x1F) {
        return nullptr;
    }
    uint8_t* entry = cueBanks + bank * 0x140;
    if (At<uint8_t>(entry, 0x21) == 0) {
        return nullptr;
    }
    int32_t sound = self->sound;
    if (sound < 0 || sound >= At<int32_t>(entry, 0x24)) {
        return nullptr;
    }
    return At<uint8_t*>(entry, 0x28) + sound * 0x58;
}

// 0x5A9310 (thiscall): its sound or null
uint8_t* __fastcall Instance_SoundOfImpl(SoundInstance* self) {
    return SoundOf(self);
}

// 0x5A9350 (thiscall): its volume times its parents' (reads a missing sound at null)
float __fastcall Instance_VolumeImpl(SoundInstance* self) {
    float volume = At<float>(SoundOf(self), 0x54);
    if (self->parent != nullptr) {
        volume = Instance_Volume(self->parent) * volume;
    }
    return volume;
}

// 0x5A93B0 (thiscall): frees it (its gains when flagged) and drops its sound's count
void __fastcall Instance_ReleaseImpl(SoundInstance* self) {
    if (self->stopFlag != 0) {
        if (self->handle != -1) {
            self->handle = -1;
        }
        if (self->gains != nullptr) {
            Free(self->gains);
            self->gains = nullptr;
        }
        self->gainsCapacity = 0;
        self->gainsCount = 0;
    }
    uint8_t* sound = SoundOf(self);
    if (sound != nullptr) {
        At<int32_t>(sound, 0x4C) -= 1;
    }
    self->used = 0;
    self->paused = 0;
}

// 0x5A9470 (thiscall): pauses or resumes its voices and its children's
void __fastcall Instance_PauseImpl(SoundInstance* self, re::Edx, int pause) {
    for (int i = 0; i < 8; ++i) {
        uint8_t voice = self->voices[i];
        if (voice != 0xFF && voiceHandles[voice] != -1) {
            Engine_Pause(voiceHandles[voice], pause);
        }
    }
    for (SoundInstance* child = self->children; child != nullptr; child = child->next) {
        Instance_Pause(child, pause);
    }
    self->paused = uint8_t(pause);
}

// 0x5A94D0 (thiscall): copies speaker gains ({data, capacity, count})
void __fastcall Instance_SetGainsImpl(SoundInstance* self, re::Edx, uint8_t* gains) {
    if (gains == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < At<uint32_t>(gains, 8); ++i) {
        self->gains[i] = At<float*>(gains, 0)[i];
    }
}

// 0x5A9500 (thiscall): skips count * step grains, remembering a loop (+0xA: where,
// +0xB: to where)
void __fastcall Instance_Skip(SoundInstance* self, re::Edx, int count, int step, int loopTo) {
    uint8_t* sound = SoundOf(self);
    uint8_t* grain = static_cast<uint8_t*>(self->grain);
    uint8_t here = uint8_t(int32_t(grain - At<uint8_t*>(sound, 4)) >> 2);
    uint8_t stepByte = uint8_t(step);
    self->loopAt = uint8_t((uint8_t(uint8_t(count) + 1) * stepByte + 1) * 2 + here);
    self->loopTo = uint8_t(uint8_t(loopTo) * stepByte * 2 + here);
    uint8_t* next = NextGrainOf(SoundOf(self), grain);
    int32_t times = int32_t(int16_t(count)) * int32_t(int16_t(step));
    for (int32_t i = 0; i < times; ++i) {
        next = NextGrainOf(SoundOf(self), next);
    }
    self->grain = next;
    self->state = 0;
    self->looping = 1;
}

// 0x5A9630 (thiscall): releases one voice
void __fastcall Instance_FreeVoiceImpl(SoundInstance* self, re::Edx, int slot) {
    uint8_t voice = self->voices[slot];
    if (voice == 0xFF) {
        return;
    }
    int32_t* handle = &voiceHandles[voice];
    uint8_t* bank = Instance_BankOf(self);
    Engine_Fn5AF4C0(At<int32_t>(bank, 8), *handle, 0);
    *handle = -1;
    self->voices[slot] = 0xFF;
}

// 0x5A9690 (thiscall, the engine): frees every instance's gains
void __fastcall Engine_FreeGains(uint8_t* self) {
    for (int i = 0; i < 0x40; ++i) {
        SoundInstance* instance = reinterpret_cast<SoundInstance*>(self + 0x2A20) + i;
        if (instance->gains != nullptr) {
            Free(instance->gains);
            instance->gains = nullptr;
        }
        instance->gainsCapacity = 0;
        instance->gainsCount = 0;
    }
}

// 0x5A96D0 (thiscall, the engine): pauses or resumes every instance
void __fastcall Engine_PauseAll(uint8_t* self, re::Edx, int pause) {
    for (int i = 0; i < 0x40; ++i) {
        SoundInstance* instance = reinterpret_cast<SoundInstance*>(self + 0x2A20) + i;
        if (instance->used != 0) {
            Instance_Pause(instance, pause);
        }
    }
}

// ---- a bank (0x140 bytes) ----------------------------------------------------------------------

// audio\xact\<name>.sea into 0xCF7160
void BuildBankPath(int32_t index) {
    std::memcpy(bankPath, reinterpret_cast<const void*>(0x0091DF78), 12);   // "audio\xact\"
    std::strcat(bankPath, reinterpret_cast<const char*>(bankTable + index * 0x48));
    std::strcat(bankPath, reinterpret_cast<const char*>(0x0091E870));       // ".sea"
}

// 0x5A9700 (thiscall): a loading step: 0 opens the file (the handle and size cached in the
// bank table) and starts reading; 3 unloads unless reading; the others are just stored
void __fastcall Bank_SetLoadStepImpl(uint8_t* self, re::Edx, int32_t step) {
    if (step != 0) {
        if (step == 3 && At<int32_t>(self, 0x2C) != 1) {
            Bank_Unload(self);
            return;
        }
        At<int32_t>(self, 0x2C) = step;
        return;
    }
    BuildBankPath(At<int32_t>(self, 0x30));
    const char* path = FixPath(bankPath);
    uint8_t* entry = bankTable + At<int32_t>(self, 0x30) * 0x48;
    At<int32_t>(self, 0x34) = At<int32_t>(entry, 0x44);
    if (At<int32_t>(self, 0x34) == -1) {
        At<uint32_t>(self, 0x34) = ImportCreateFileA()(path, 0x80000000, 1, nullptr, 3, 0x70000001, nullptr);
        At<int32_t>(bankTable + At<int32_t>(self, 0x30) * 0x48, 0x44) = At<int32_t>(self, 0x34);
    }
    StrcpyS(reinterpret_cast<char*>(self + 0x38), 0x100, path);
    int32_t size = At<int32_t>(bankTable + At<int32_t>(self, 0x30) * 0x48, 0x40);
    if (size == -1) {
        size = File_GetFileLength(bankPath);
        At<int32_t>(bankTable + At<int32_t>(self, 0x30) * 0x48, 0x40) = size;
    }
    if (At<int32_t>(self, 0x34) == -1 || size == -1) {
        step = 4;
    } else {
        At<int32_t>(self, 0x13C) = size;
        At<int32_t>(self, 0x138) = 0;
        int32_t sectors = (size & 0x7FF) != 0 ? size / 0x800 + 1 : size / 0x800;
        At<int32_t>(self, 0x13C) = sectors;
        Sound_Lock(0x2E);
        At<void*>(self, 0) = New(uint32_t(sectors) << 11);
        Sound_Unlock();
        At<uint8_t>(self, 0x20) = 1;
        At<int32_t>(self, 0x14) = At<int32_t>(self, 0x138);
        At<uint32_t>(self, 0x1C) = ImportCreateEventA()(nullptr, 0, 0, nullptr);
        if (At<void*>(self, 0) == nullptr) {
            step = 4;
        }
    }
    At<int32_t>(self, 0x2C) = step;
}

// 0x5A98E0 (thiscall): parses a loaded bank: the wave bank (+4), the sound bank (+8), then
// each sound's grains (on a failure before the first allocation the two locks stay taken,
// like the game)
uint8_t __fastcall Bank_ParseImpl(uint8_t* self, re::Edx, void*) {
    int32_t* header = At<int32_t*>(self, 0);
    int32_t soundSize = header[0];
    int32_t waveOffset = header[1];
    int32_t waveSize = header[2];
    Sound_Lock(0x2E);
    Sound_Lock(0x2F);
    if (waveSize < 0) {
        return 0;
    }
    void* waves = New(uint32_t(waveSize));
    Sound_Unlock();
    Memcpy(waves, At<uint8_t*>(self, 0) + waveOffset + soundSize, uint32_t(waveSize));
    uint32_t keep = uint32_t(soundSize + waveOffset);
    void* copy = New(keep);
    Memcpy(copy, At<void*>(self, 0), keep);
    if (At<void*>(self, 0) != nullptr) {
        Free(At<void*>(self, 0));
    }
    At<void*>(self, 0) = nullptr;
    At<void*>(self, 0) = New(keep);
    Memcpy(At<void*>(self, 0), copy, keep);
    if (copy != nullptr) {
        Free(copy);
    }
    header = At<int32_t*>(self, 0);
    Sound_Unlock();
    uint8_t* soundData = reinterpret_cast<uint8_t*>(header) + header[0];
    if (header[2] <= 0 || header[1] <= 0) {
        if (At<void*>(self, 0) != nullptr) {
            Free(At<void*>(self, 0));
        }
        At<void*>(self, 0) = nullptr;
        if (waves != nullptr) {
            Free(waves);
        }
        return 0;
    }
    Sound_Lock(0x2E);
    At<int32_t>(self, 4) = Engine_Fn5AF220(waves, header[2], 0, 0x9A020004);
    Sound_Unlock();
    if (At<int32_t>(self, 4) == -1) {
        if (At<void*>(self, 0) != nullptr) {
            Free(At<void*>(self, 0));
        }
        At<void*>(self, 0) = nullptr;
        return 0;
    }
    Sound_Lock(0x2E);
    Sound_Lock(0x2F);
    void* sounds = New(uint32_t(header[1]));
    Sound_Unlock();
    Memcpy(sounds, soundData, uint32_t(header[1]));
    At<int32_t>(self, 8) = Engine_Fn5AF150(sounds, header[1], 0, 0x9A020004);
    Sound_Unlock();
    if (At<int32_t>(self, 8) == -1) {
        if (At<void*>(self, 0) != nullptr) {
            Free(At<void*>(self, 0));
        }
        At<void*>(self, 0) = nullptr;
        Engine_Fn5AF2F0(At<int32_t>(self, 4));
        At<int32_t>(self, 4) = -1;
        return 0;
    }
    int32_t count = header[3];
    uint8_t* list = At<uint8_t*>(self, 0) + 0x10;
    At<uint8_t*>(self, 0x28) = list;
    uint8_t* grainData = list + count * 0x58;
    At<int32_t>(self, 0x24) = count;
    for (int32_t k = 0; k < At<int32_t>(self, 0x24); ++k) {
        uint8_t* grains = static_cast<uint8_t*>(New(uint32_t(grainStride * At<int32_t>(list, k * 0x58))));
        for (int32_t j = 0; j < At<int32_t>(At<uint8_t*>(self, 0x28), k * 0x58); ++j) {
            void* grain = Grain_Create(int8_t(*grainData), grainData, grains + grainStride * j);
            int32_t size;
            using SizeFn = void*(__thiscall*)(void*, int32_t*);
            re::PtrCast<SizeFn>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(grain))[1])(grain, &size);
            grainData += size;
        }
        At<uint8_t*>(At<uint8_t*>(self, 0x28), k * 0x58 + 4) = grains;
    }
    return 1;
}

// 0x5A9B50 (thiscall, the engine): frees every instance's gains (from the last)
void __fastcall Engine_FreeGainsReverse(uint8_t* self) {
    for (int i = 0x3F; i >= 0; --i) {
        SoundInstance* instance = reinterpret_cast<SoundInstance*>(self + 0x2A20) + i;
        if (instance->gains != nullptr) {
            Free(instance->gains);
            instance->gains = nullptr;
        }
    }
}

// 0x5A9B90 (thiscall): adds its voice handles (and its children's) to a growing array
// ({data, capacity, count})
void __fastcall Instance_CollectVoicesImpl(SoundInstance* self, re::Edx, uint8_t* list) {
    for (int i = 0; i < 8; ++i) {
        uint8_t voice = self->voices[i];
        if (voice == 0xFF) {
            continue;
        }
        int32_t* handle = &voiceHandles[voice];
        uint32_t capacity = At<uint32_t>(list, 4);
        if (At<uint32_t>(list, 8) == capacity) {
            capacity = capacity != 0 ? capacity * 2 : 1;
            At<uint32_t>(list, 4) = capacity;
            int32_t* grown = nullptr;
            if (capacity != 0) {
                uint64_t bytes = uint64_t(capacity) * 4;
                grown = static_cast<int32_t*>(New(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes)));
            }
            if (At<int32_t*>(list, 0) != nullptr) {
                for (uint32_t j = 0; j < At<uint32_t>(list, 8); ++j) {
                    grown[j] = At<int32_t*>(list, 0)[j];
                }
                Free(At<int32_t*>(list, 0));
            }
            At<int32_t*>(list, 0) = grown;
        }
        At<int32_t*>(list, 0)[At<uint32_t>(list, 8)] = *handle;
        At<uint32_t>(list, 8) += 1;
    }
    for (SoundInstance* child = self->children; child != nullptr; child = child->next) {
        Instance_CollectVoices(child, list);
    }
}

// 0x5A9C70 (thiscall): starts an instance of a bank's sound (6 zeroed gains)
void __fastcall Instance_InitImpl(SoundInstance* self, re::Edx, int bank, int sound, SoundInstance* parent) {
    self->bank = int16_t(bank);
    self->loopAt = 0xFF;
    self->loopTo = 0xFF;
    self->sound = int16_t(sound);
    self->used = 1;
    self->parent = parent;
    self->waitUntil = 0xFFFFFFFF;
    self->state = 0;
    self->children = nullptr;
    self->next = nullptr;
    self->looping = 0;
    self->autoStop = 0;
    self->stopFlag = 0;
    self->paused = 0;
    *reinterpret_cast<int32_t*>(self->registers) = 0;
    std::memset(self->voices, 0xFF, 8);
    uint8_t* entry = SoundOf(self);
    if (entry != nullptr) {
        self->grain = At<void*>(entry, 4);
        At<int32_t>(entry, 0x4C) += 1;
    }
    if (self->gainsCapacity < 6) {
        self->gainsCapacity = 6;
        float* grown = static_cast<float*>(New(0x18));
        if (self->gains != nullptr) {
            for (uint32_t i = 0; i < self->gainsCount; ++i) {
                grown[i] = self->gains[i];
            }
            Free(self->gains);
        }
        self->gains = grown;
        self->gainsCount = self->gainsCapacity;
    }
    for (int i = 0; i < 6; ++i) {
        self->gains[i] = 0.0f;
    }
}

// 0x5A9E00 (thiscall): releases all its voices
void __fastcall Instance_ReleaseVoicesImpl(SoundInstance* self) {
    for (int i = 0; i < 8; ++i) {
        uint8_t voice = self->voices[i];
        if (voice == 0xFF) {
            continue;
        }
        int32_t* handle = &voiceHandles[voice];
        uint8_t* bank = Instance_BankOfImpl(self);
        Engine_Fn5AF4C0(At<int32_t>(bank, 8), *handle, 0);
        *handle = -1;
        self->voices[i] = 0xFF;
    }
}

// 0x5A9E70 (thiscall, the engine): a free instance started for a sound
SoundInstance* __fastcall Engine_NewInstanceImpl(uint8_t* self, re::Edx, int bank, int sound, SoundInstance* parent) {
    for (int i = 0; i < 0x40; ++i) {
        SoundInstance* instance = reinterpret_cast<SoundInstance*>(self + 0x2A20) + i;
        if (instance->used == 0) {
            Instance_Init(instance, bank, sound, parent);
            return instance;
        }
    }
    return nullptr;
}

// 0x5A9EC0 (thiscall): loads a bank from the table (async: through the steps)
uint8_t __fastcall Bank_LoadImpl(uint8_t* self, re::Edx, int32_t index, int async) {
    At<void*>(self, 0) = nullptr;
    At<int32_t>(self, 4) = -1;
    At<int32_t>(self, 8) = -1;
    At<int32_t>(self, 0x28) = 0;
    At<int32_t>(self, 0xC) = 0;
    At<int32_t>(self, 0x10) = 0;
    At<int32_t>(self, 0x14) = 0;
    At<int32_t>(self, 0x18) = 0;
    At<int32_t>(self, 0x1C) = 0;
    At<uint8_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x30) = index;
    if (uint8_t(async) != 0) {
        At<uint8_t>(self, 0x21) = 1;
        Bank_SetLoadStep(self, 0);
        return 1;
    }
    BuildBankPath(index);
    void* file = File_Open(bankPath, reinterpret_cast<const char*>(0x008FF548), 0);   // "r"
    if (file == nullptr) {
        return 0;
    }
    File_Seek(file, 0, 2);
    int size = File_Tell(file);
    File_Seek(file, 0, 0);
    Sound_Lock(0x2E);
    At<void*>(self, 0) = New(uint32_t(size));
    Sound_Unlock();
    if (File_Read(file, At<void*>(self, 0), size) != 0 && Bank_Parse(self, file) != 0) {
        At<uint8_t>(self, 0x21) = 1;
        At<int32_t>(self, 0x2C) = 2;
        File_Close(file);
        At<uint8_t>(bankTable + index * 0x48, 0x34) |= 1;
        return 1;
    }
    File_Close(file);
    return 0;
}

void UnloadBank(uint8_t* self) {
    At<int32_t>(self, 0x2C) = 4;
    if (At<int32_t>(self, 8) != -1) {
        Engine_Fn5AF1D0(At<int32_t>(self, 8));
        At<int32_t>(self, 8) = -1;
    }
    At<int32_t>(self, 0x28) = 0;
    At<int32_t>(self, 0x24) = 0;
    if (At<int32_t>(self, 0x34) != -1) {
        At<int32_t>(self, 0x34) = -1;
    }
    if (At<int32_t>(self, 4) != -1) {
        Engine_Fn5AF2F0(At<int32_t>(self, 4));
        At<int32_t>(self, 4) = -1;
    }
    if (At<void*>(self, 0) != nullptr) {
        Free(At<void*>(self, 0));
        At<void*>(self, 0) = nullptr;
    }
    At<uint8_t>(self, 0x21) = 0;
}

// 0x5AA0A0 (thiscall): unloads it
void __fastcall Bank_UnloadImpl(uint8_t* self) {
    if (At<uint8_t>(self, 0x21) != 0) {
        UnloadBank(self);
    }
}

// 0x5AA110 (thiscall): advances an asynchronous load (0 read, 1 wait and parse, 3 unload
// once the read is done)
void __fastcall Bank_UpdateImpl(uint8_t* self) {
    switch (At<int32_t>(self, 0x2C)) {
    case 0:
        ImportReadFile()(At<uint32_t>(self, 0x34), At<void*>(self, 0), uint32_t(At<int32_t>(self, 0x13C)) << 11,
                         nullptr, re::SystemOverlapped(self + 0xC));
        re::SyncOverlapped(self + 0xC);
        At<int32_t>(self, 0x2C) = 1;
        return;
    case 1: {
        uint32_t read = 0;
        int got = ImportGetOverlappedResult()(At<uint32_t>(self, 0x34), re::SystemOverlapped(self + 0xC), &read, 0);
        re::SyncOverlapped(self + 0xC);
        if (got == 0 ||
            At<void*>(self, 0) == nullptr) {
            return;
        }
        if (Bank_Parse(self, At<void*>(self, 0x34)) == 0) {
            if (At<int32_t>(self, 0x34) != -1) {
                At<int32_t>(self, 0x34) = -1;
            }
            At<int32_t>(self, 0x2C) = 4;
            return;
        }
        if (At<int32_t>(self, 0x34) != -1) {
            At<int32_t>(self, 0x34) = -1;
        }
        At<uint8_t>(bankTable + At<int32_t>(self, 0x30) * 0x48, 0x34) |= 1;
        At<uint8_t>(bankTable + At<int32_t>(self, 0x30) * 0x48, 0x34) |= 2;
        At<int32_t>(self, 0x2C) = 2;
        return;
    }
    case 3:
        re::SyncOverlapped(self + 0xC);
        if (At<int32_t>(self, 0xC) != 0x103) {   // (not STATUS_PENDING)
            Bank_Unload(self);
        }
        return;
    default:
        return;
    }
}

// 0x5AA200 (thiscall): stops it and its children: past its voices, a grain of type 0x2B
// moves on, anything else ends it
void __fastcall Instance_StopImpl(SoundInstance* self, re::Edx, int flag) {
    for (SoundInstance* child = self->children; child != nullptr; child = child->next) {
        Instance_Stop(child, 0);
    }
    Instance_ReleaseVoices(self);
    void* grain = self->grain;
    using TypeFn = int32_t(__thiscall*)(void*);
    if (grain != nullptr && re::PtrCast<TypeFn>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(grain))[2])(grain) == 0x2B) {
        uint8_t* current = static_cast<uint8_t*>(self->grain);
        uint8_t* sound = Instance_SoundOf(self);
        self->grain = NextGrainOf(sound, current);
        self->state = 0;
    } else {
        self->grain = nullptr;
        self->state = 6;
    }
    self->parent = nullptr;
    self->autoStop = 1;
    self->stopFlag = uint8_t(flag);
}

// 0x5AA2B0 (thiscall, the engine): the constructor
uint8_t* __fastcall Engine_Construct(uint8_t* self) {
    for (int i = 0; i < 0x20; ++i) {
        uint8_t* bank = self + 0x220 + i * 0x140;
        At<int32_t>(bank, 0) = 0;
        At<int32_t>(bank, 4) = -1;
        At<int32_t>(bank, 8) = -1;
        At<uint8_t>(bank, 0x20) = 0;
        At<int32_t>(bank, 0x24) = 0;
        At<int32_t>(bank, 0x28) = 0;
        At<int32_t>(bank, 0x2C) = 0;
        At<int32_t>(bank, 0x30) = -1;
        At<int32_t>(bank, 0x138) = 0;
        At<int32_t>(bank, 0x13C) = 0;
        At<uint8_t>(bank, 0x21) = 0;
        At<int32_t>(bank, 0x34) = -1;
    }
    for (int i = 0; i < 0x40; ++i) {
        SoundInstance* instance = reinterpret_cast<SoundInstance*>(self + 0x2A20) + i;
        instance->gains = nullptr;
        instance->gainsCapacity = 0;
        instance->gainsCount = 0;
        instance->used = 0;
        instance->handle = -1;
    }
    for (int i = 0; i < 8; ++i) {
        At<int32_t>(self, i * 4) = 0;
    }
    for (int i = 0; i < 0x80; ++i) {
        At<int32_t>(self, 0x20 + i * 4) = -1;
    }
    grainStride = 8;
    return self;
}

// 0x5AA350 (thiscall, the engine): loads a bank into a free entry
uint8_t* __fastcall Engine_LoadBank(uint8_t* self, re::Edx, int32_t index, int async) {
    for (int i = 0; i < 0x20; ++i) {
        uint8_t* bank = self + 0x220 + i * 0x140;
        if (At<uint8_t>(bank, 0x21) == 0) {
            return Bank_Load(bank, index, async) != 0 ? bank : nullptr;
        }
    }
    return nullptr;
}

// 0x5AA3B0 (thiscall, the engine): finishes every bank's loading
void __fastcall Engine_WaitForBanks(uint8_t* self) {
    bool done;
    do {
        done = true;
        for (int i = 0; i < 0x20; ++i) {
            uint8_t* bank = self + 0x220 + i * 0x140;
            if (At<uint8_t>(bank, 0x21) != 0 && At<int32_t>(bank, 0x2C) != 2) {
                Bank_Update(bank);
                done = false;
            }
        }
    } while (!done);
}

// 0x5AA400 (thiscall, the engine): stops a bank's instances and unloads it (later if it's
// being read)
void __fastcall Engine_UnloadBank(uint8_t* self, re::Edx, uint8_t* bank) {
    for (int i = 0; i < 0x40; ++i) {
        SoundInstance* instance = reinterpret_cast<SoundInstance*>(self + 0x2A20) + i;
        if (instance->used != 0 && Instance_BankOfImpl(instance) == bank) {
            Instance_Stop(instance, 0);
            Instance_Release(instance);
        }
    }
    if (bank == nullptr) {
        return;
    }
    if (At<int32_t>(bank, 0x2C) != 1) {
        Bank_Unload(bank);
        return;
    }
    At<int32_t>(bank, 0x2C) = 3;
}

// 0x5AA480 (thiscall): updates the children, dropping the finished ones: true when all are
// done (state 6 when none are left)
uint8_t __fastcall Instance_UpdateChildrenImpl(SoundInstance* self) {
    uint8_t allDone = 1;
    SoundInstance* child = self->children;
    while (child != nullptr) {
        SoundInstance* next;
        if (Instance_Update(child) != 0) {
            next = child->next;
            SoundInstance* first = self->children;
            Instance_Stop(child, 0);
            if (child != first && first->next != nullptr) {
                SoundInstance* previous = first;
                while (previous->next != child) {
                    if (previous->next->next == nullptr) {
                        previous = nullptr;
                        break;
                    }
                    previous = previous->next;
                }
                if (previous != nullptr) {
                    previous->next = child->next;
                }
            }
            if (self->children == child) {
                self->children = next;
            }
        } else {
            next = child->next;
            allDone = 0;
        }
        child = next;
    }
    if (self->children == nullptr) {
        self->state = 6;
    }
    return allDone;
}

// 0x5AA510 (thiscall): runs its grains (at most 10 a frame) and drops finished voices:
// true when it's done
uint8_t __fastcall Instance_UpdateImpl(SoundInstance* self) {
    int budget = 10;
    bool voicesPlaying = false;
    for (int i = 0; i < 8; ++i) {
        uint8_t voice = self->voices[i];
        if (voice == 0xFF) {
            continue;
        }
        int32_t handle = voiceHandles[voice];
        if (handle == -1) {
            continue;
        }
        if (Engine_Fn5AF9A0(handle) == 1) {
            Instance_FreeVoice(self, i);
            continue;
        }
        uint32_t state = 0;
        if (Engine_Fn5AF550(handle, &state) != 0 && (state & 0x20) != 0) {
            Instance_FreeVoice(self, i);
        } else {
            voicesPlaying = true;
        }
    }
    if (self->children != nullptr) {
        Instance_UpdateChildren(self);
        self->state = 0;
    }
    bool hadNoGrain = self->grain == nullptr;
    if (self->grain != nullptr) {
        do {
            if (self->paused != 0) {
                return 0;
            }
            if (self->state == 0) {
                self->state = 1;
            }
            void* grain = self->grain;
            using ExecuteFn = uint8_t(__thiscall*)(void*, void*);
            if (re::PtrCast<ExecuteFn>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(grain))[4])(grain, self) == 0) {
                return 0;
            }
            if (self->state != 0) {
                uint8_t* current = static_cast<uint8_t*>(self->grain);
                self->grain = NextGrainOf(Instance_SoundOf(self), current);
                self->state = 0;
            }
            uint8_t* current = static_cast<uint8_t*>(self->grain);
            if (current != nullptr && self->looping == 1) {
                uint8_t* sound = Instance_SoundOf(self);
                if (int32_t(current - At<uint8_t*>(sound, 4)) >> 2 == int32_t(self->loopAt)) {
                    uint8_t* target = At<uint8_t*>(Instance_SoundOf(self), 4) + self->loopTo * 4;
                    self->grain = target;
                    self->state = 0;
                    self->grain = NextGrainOf(Instance_SoundOf(self), target);
                    self->looping = 0;
                }
            }
            if (--budget == 0) {
                return 0;
            }
        } while (self->grain != nullptr);
    }
    if (self->children != nullptr) {
        if (self->state == 6) {
            self->state = 3;
        }
        return 0;
    }
    if (!voicesPlaying && hadNoGrain) {
        if (self->grain == nullptr) {
            if (self->autoStop != 0) {
                Instance_Stop(self, 0);
            }
            self->state = 6;
        }
        return 1;
    }
    if (self->grain == nullptr) {
        self->state = 5;
    }
    return 0;
}

// 0x5AA710 (thiscall, the engine): the per-frame update: loading banks, the root
// instances, and banks whose engine data failed
void __fastcall Engine_Update(uint8_t* self) {
    for (int i = 0; i < 0x20; ++i) {
        uint8_t* bank = self + 0x220 + i * 0x140;
        if (At<uint8_t>(bank, 0x21) != 0 && At<int32_t>(bank, 0x2C) != 2) {
            Bank_Update(bank);
        }
    }
    for (int i = 0; i < 0x40; ++i) {
        SoundInstance* instance = reinterpret_cast<SoundInstance*>(self + 0x2A20) + i;
        if (instance->used == 0) {
            continue;
        }
        if (instance->autoStop != 0 && instance->state == 6 && instance->grain == nullptr) {
            Instance_Release(instance);
        } else if (instance->parent == nullptr) {
            Instance_Update(instance);
        }
    }
    for (int i = 0; i < 0x20; ++i) {
        uint8_t* bank = self + 0x220 + i * 0x140;
        if (At<uint8_t>(bank, 0x21) == 0 || At<int32_t>(bank, 0x2C) != 2) {
            continue;
        }
        bool failed = (At<int32_t>(bank, 4) != -1 && Engine_Fn5AF340(At<int32_t>(bank, 4)) == 1) ||
                      (At<int32_t>(bank, 8) != -1 && Engine_Fn5AF210(At<int32_t>(bank, 8)) == 1);
        if (failed && At<uint8_t>(bank, 0x21) != 0) {
            UnloadBank(bank);
        }
    }
}

// 0x5AA840 (thiscall, a bank): plays one of its sounds with speaker gains
SoundInstance* __fastcall Bank_PlaySoundImpl(uint8_t* self, re::Edx, int sound, void* gains) {
    int32_t index = self == nullptr ? -1 : int32_t(self - cueBanks) / 0x140;
    if (int16_t(sound) == -1 || int16_t(index) == -1) {
        return nullptr;
    }
    SoundInstance* instance = Engine_NewInstance(TheCueEngine, index, sound, nullptr);
    if (instance != nullptr) {
        Instance_SetGains(instance, gains);
        if (At<int32_t>(self, 8) != -1) {
            instance->handle = At<int32_t>(self, 8);
            Instance_Update(instance);
        }
    }
    return instance;
}

// 0x5AA8C0 (thiscall, the engine): plays a sound by hash from the loaded banks (autoStop
// for flag)
SoundInstance* __fastcall Engine_Play(uint8_t* self, re::Edx, uint32_t hash, int flag, void* gains) {
    for (int i = 0; i < 0x20; ++i) {
        uint8_t* bank = self + 0x220 + i * 0x140;
        if (At<uint8_t>(bank, 0x21) == 0 || At<int32_t>(bank, 0x2C) != 2) {
            continue;
        }
        uint16_t sound = uint16_t(Bank_FindSound(bank, hash));
        if (sound == 0xFFFF) {
            continue;
        }
        SoundInstance* instance = Bank_PlaySound(self + 0x220 + i * 0x140, sound, gains);
        if (uint8_t(flag) != 0 && instance != nullptr) {
            instance->autoStop = 1;
        }
        return instance;
    }
    return nullptr;
}

// ---- registration ---------------------------------------------------------------------------

#define CUEENGINE_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(cueengine_original_, __LINE__);                                       \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(cueengine_original_, __LINE__), \
             re::Mode::Original)

CUEENGINE_REVERSED("CueSound::GrainAt", 0x005A9000, &Sound_GrainAt);
CUEENGINE_REVERSED("CueSound::PrevGrain", 0x005A9010, &Sound_PrevGrain);
CUEENGINE_REVERSED("CueSound::NextGrain", 0x005A9040, &Sound_NextGrain);
CUEENGINE_REVERSED("CueInstance::AppendSibling", 0x005A9080, &Instance_AppendSiblingImpl);
CUEENGINE_REVERSED("CueInstance::FreeVoiceSlot", 0x005A90C0, &Instance_FreeVoiceSlot);
CUEENGINE_REVERSED("CueInstance::VoiceCount", 0x005A90E0, &Instance_VoiceCount);
CUEENGINE_REVERSED("CueInstance::SetGrain", 0x005A9130, &Instance_SetGrain);
CUEENGINE_REVERSED("CueInstance::RootGains", 0x005A9150, &Instance_RootGains);
CUEENGINE_REVERSED("CueInstance::AddChild", 0x005A9170, &Instance_AddChild);
CUEENGINE_REVERSED("CueEngine::FreeVoice", 0x005A91B0, &Engine_FreeVoice);
CUEENGINE_REVERSED("CueEngine::BankIndex", 0x005A91F0, &Engine_BankIndex);
CUEENGINE_REVERSED("CueBank::FindSound", 0x005A9220, &Bank_FindSoundImpl);
CUEENGINE_REVERSED("CueInstance::Wait", 0x005A9270, &Instance_Wait);
CUEENGINE_REVERSED("CueInstance::BankOf", 0x005A92E0, &Instance_BankOfImpl);
CUEENGINE_REVERSED("CueInstance::SoundOf", 0x005A9310, &Instance_SoundOfImpl);
CUEENGINE_REVERSED("CueInstance::Volume", 0x005A9350, &Instance_VolumeImpl);
CUEENGINE_REVERSED("CueInstance::Release", 0x005A93B0, &Instance_ReleaseImpl);
CUEENGINE_REVERSED("CueInstance::Pause", 0x005A9470, &Instance_PauseImpl);
CUEENGINE_REVERSED("CueInstance::SetGains", 0x005A94D0, &Instance_SetGainsImpl);
CUEENGINE_REVERSED("CueInstance::Skip", 0x005A9500, &Instance_Skip);
CUEENGINE_REVERSED("CueInstance::FreeVoice", 0x005A9630, &Instance_FreeVoiceImpl);
CUEENGINE_REVERSED("CueEngine::FreeGains", 0x005A9690, &Engine_FreeGains);
CUEENGINE_REVERSED("CueEngine::PauseAll", 0x005A96D0, &Engine_PauseAll);
CUEENGINE_REVERSED("CueBank::SetLoadStep", 0x005A9700, &Bank_SetLoadStepImpl);
CUEENGINE_REVERSED("CueBank::Parse", 0x005A98E0, &Bank_ParseImpl);
CUEENGINE_REVERSED("CueEngine::FreeGainsReverse", 0x005A9B50, &Engine_FreeGainsReverse);
CUEENGINE_REVERSED("CueInstance::CollectVoices", 0x005A9B90, &Instance_CollectVoicesImpl);
CUEENGINE_REVERSED("CueInstance::Init", 0x005A9C70, &Instance_InitImpl);
CUEENGINE_REVERSED("CueInstance::ReleaseVoices", 0x005A9E00, &Instance_ReleaseVoicesImpl);
CUEENGINE_REVERSED("CueEngine::NewInstance", 0x005A9E70, &Engine_NewInstanceImpl);
CUEENGINE_REVERSED("CueBank::Load", 0x005A9EC0, &Bank_LoadImpl);
CUEENGINE_REVERSED("CueBank::Unload", 0x005AA0A0, &Bank_UnloadImpl);
CUEENGINE_REVERSED("CueBank::Update", 0x005AA110, &Bank_UpdateImpl);
CUEENGINE_REVERSED("CueInstance::Stop", 0x005AA200, &Instance_StopImpl);
CUEENGINE_REVERSED("CueEngine::CueEngine", 0x005AA2B0, &Engine_Construct);
CUEENGINE_REVERSED("CueEngine::LoadBank", 0x005AA350, &Engine_LoadBank);
CUEENGINE_REVERSED("CueEngine::WaitForBanks", 0x005AA3B0, &Engine_WaitForBanks);
CUEENGINE_REVERSED("CueEngine::UnloadBank", 0x005AA400, &Engine_UnloadBank);
CUEENGINE_REVERSED("CueInstance::UpdateChildren", 0x005AA480, &Instance_UpdateChildrenImpl);
CUEENGINE_REVERSED("CueInstance::Update", 0x005AA510, &Instance_UpdateImpl);
CUEENGINE_REVERSED("CueEngine::Update", 0x005AA710, &Engine_Update);
CUEENGINE_REVERSED("CueBank::PlaySound", 0x005AA840, &Bank_PlaySoundImpl);
CUEENGINE_REVERSED("CueEngine::Play", 0x005AA8C0, &Engine_Play);

} // namespace
