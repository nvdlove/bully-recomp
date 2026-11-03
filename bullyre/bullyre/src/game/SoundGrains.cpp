// The cue engine's grains (0x5AA940..0x5ABEDE): a sound's program, one 8-byte object per
// step ({vtable, data}) built over the bank's data by the factory 0x5AB630. The data starts
// {type, wait ms}; each grain waits its ms, then acts on the instance (see SoundEngine.cpp).
// The vtables (0x9225BC..0x922810): vf0 destructor, vf1 data size, vf2 type, vf3 (0x609C90,
// true), vf4 execute (true: go on), vf5 set data.
//
// Registers: an instance's four (+0x24) for indices 0..3, the engine's globals for
// negative ones (0xCF7268 - index - 1).

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct Grain {
    re::Ptr32<void> vtable;
    re::Ptr32<uint8_t> data;
};
static_assert(sizeof(Grain) == 8);

inline uint8_t* const TheCueEngine = reinterpret_cast<uint8_t*>(0x00CF7268);
GAME_ARRAY(uint8_t*, bankStreams, 0x00CF5DD0);          // 8 bytes per bank: {?, stream}
inline void* const TheSounds = reinterpret_cast<void*>(0x00CF3DA0);
GAME_VAR(int32_t, grainStride, 0x00CF7260);

GAME_FN(void*, __cdecl, New, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int low, int high);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(void, __cdecl, Sound_Lock, 0x005EF0A0, int);
GAME_FN(void, __cdecl, Sound_Unlock, 0x005EF100);
GAME_FN(int32_t, __thiscall, Sounds_Fn594E60, 0x00594E60, void* sounds, int);
GAME_FN(int, __cdecl, VolumeToMillibels, 0x005A7E50, float volume);
// the sound engine (0x5AF...)
GAME_FN(void, __cdecl, Engine_Fn5AF330, 0x005AF330, int32_t handle, uint32_t* state);
GAME_FN(int32_t, __cdecl, Engine_CreateVoice, 0x005AF350, int32_t bank, const char* name);
GAME_FN(void, __cdecl, Engine_Fn5AF570, 0x005AF570, int32_t voice, int, int);
GAME_FN(void, __cdecl, Engine_SetVolumeDb, 0x005AF610, int32_t voice, float decibels);
GAME_FN(void, __cdecl, Engine_Fn5AF660, 0x005AF660, int32_t voice, float);
GAME_FN(void, __cdecl, Engine_SetPan, 0x005AF6B0, int32_t voice, float pan);
GAME_FN(void, __cdecl, Engine_Fn5AF700, 0x005AF700, int32_t voice, int32_t speakers);
GAME_FN(void, __cdecl, Engine_Fn5AF810, 0x005AF810, int32_t voice, int, int32_t speakers, float gain);
GAME_FN(void, __cdecl, Engine_Fn5AF8C0, 0x005AF8C0, int32_t voice, int);
GAME_FN(void, __cdecl, Engine_SetGains, 0x005AF910, int32_t voice, float, float, float, float, float, float);
// SoundEngine.cpp's
GAME_FN(uint8_t*, __thiscall, Sound_GrainAt, 0x005A9000, void* sound, int index);
GAME_FN(uint8_t*, __thiscall, Sound_PrevGrain, 0x005A9010, void* sound, void* grain);
GAME_FN(uint8_t*, __thiscall, Sound_NextGrain, 0x005A9040, void* sound, void* grain);
GAME_FN(uint8_t*, __thiscall, Instance_FreeVoiceSlot, 0x005A90C0, void* instance);
GAME_FN(int32_t, __thiscall, Instance_VoiceCount, 0x005A90E0, void* instance);
GAME_FN(void, __thiscall, Instance_SetGrain, 0x005A9130, void* instance, void* grain);
GAME_FN(re::Ptr32<float>*, __thiscall, Instance_RootGains, 0x005A9150, void* instance);
GAME_FN(void, __thiscall, Instance_AddChild, 0x005A9170, void* instance, void* child);
GAME_FN(int32_t, __thiscall, Engine_FreeVoice, 0x005A91B0, void* engine, re::Ptr32<int32_t>* out);
GAME_FN(uint8_t, __thiscall, Instance_Wait, 0x005A9270, void* instance, int32_t ms);
GAME_FN(uint8_t*, __thiscall, Instance_BankOf, 0x005A92E0, void* instance);
GAME_FN(uint8_t*, __thiscall, Instance_SoundOf, 0x005A9310, void* instance);
GAME_FN(float, __thiscall, Instance_Volume, 0x005A9350, void* instance);
GAME_FN(void, __thiscall, Instance_Skip, 0x005A9500, void* instance, int count, int step, int loopTo);
GAME_FN(void, __thiscall, Instance_ReleaseVoices, 0x005A9E00, void* instance);
GAME_FN(void*, __thiscall, Engine_NewInstance, 0x005A9E70, void* engine, int bank, int sound, void* parent);
GAME_FN(uint8_t, __thiscall, Instance_UpdateChildren, 0x005AA480, void* instance);
// this file's constructors
GAME_FN(Grain*, __thiscall, GrainWave_Construct, 0x005AA940, void* self, void* data);
GAME_FN(Grain*, __thiscall, Grain4_Construct, 0x005AA990, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainPlayChild_Construct, 0x005AAA00, void* self, void* data);
GAME_FN(Grain*, __thiscall, Grain14_Construct, 0x005AAAB0, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainLoopStart_Construct, 0x005AAB00, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainLoopEnd_Construct, 0x005AAB40, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainStopCue_Construct, 0x005AAC10, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainRandomPlay_Construct, 0x005AAC80, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainRandomDelay_Construct, 0x005AAD30, void* self, void* data);
GAME_FN(Grain*, __thiscall, Grain1B_Construct, 0x005AADD0, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainSetRegister_Construct, 0x005AAE20, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainSetRegisterRandom_Construct, 0x005AAEC0, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainDecRegister_Construct, 0x005AAFB0, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainTestRegister_Construct, 0x005AB050, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainMarker_Construct, 0x005AB160, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainGotoMarker_Construct, 0x005AB1B0, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainGotoRandomMarker_Construct, 0x005AB280, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainWaitForVoices_Construct, 0x005AB380, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainPlayCycle_Construct, 0x005AB400, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainAddToRegister_Construct, 0x005AB4B0, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainKeyOffVoices_Construct, 0x005AB550, void* self, void* data);
GAME_FN(Grain*, __thiscall, GrainOnStopMarker_Construct, 0x005AB5C0, void* self, void* data);

// a grain's constructor: over existing data, or its own default data {type}
Grain* Construct(Grain* self, void* data, uintptr_t vtable, uint32_t size, int32_t type) {
    self->vtable = reinterpret_cast<void*>(vtable);
    if (data != nullptr) {
        self->data = static_cast<uint8_t*>(data);
        return self;
    }
    self->data = static_cast<uint8_t*>(New(size));
    At<int32_t>(self->data, 0) = type;
    return self;
}

inline int32_t WaitOf(const Grain* self) {
    return At<int32_t>(self->data, 4);
}

int8_t& Register(uint8_t* instance, int8_t index) {
    if (index >= 0) {
        return At<int8_t>(instance, 0x24 + index);
    }
    return *reinterpret_cast<int8_t*>(TheCueEngine - index - 1);
}

int32_t TypeOf(void* grain) {
    using TypeFn = int32_t(__thiscall*)(void*);
    return re::PtrCast<TypeFn>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(grain))[2])(grain);
}

inline int RandBelow(int range) {
    return Rand() * range / 0x7FFF;
}

// ---- the constructors, sizes and common slots ------------------------------------------------

// 0x5AA940 (thiscall): a wave (type 1, 0x28 bytes)
Grain* __fastcall GrainWave_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009225BC, 0x28, 1);
}
// 0x5AA980 (thiscall, vf1): the wave's size
uint8_t* __fastcall GrainWave_Size(Grain* self, re::Edx, int32_t* size) {
    *size = 0x28;
    return self->data;
}
// 0x5AA990 (thiscall): type 4 (0x14 bytes, waits only)
Grain* __fastcall Grain4_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009225D8, 0x14, 4);
}
// 0x5AA9D0 (thiscall, vf1)
uint8_t* __fastcall Grain_Size14(Grain* self, re::Edx, int32_t* size) {
    *size = 0x14;
    return self->data;
}
// 0x5AA9E0 (thiscall, vf4): waits, nothing else
uint8_t __fastcall Grain_WaitOnly(Grain* self, re::Edx, uint8_t* instance) {
    return Instance_Wait(instance, WaitOf(self)) != 0 ? 1 : 0;
}
// 0x5AAA00 (thiscall): plays a child sound (type 5, 0x1C bytes)
Grain* __fastcall GrainPlayChild_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009225FC, 0x1C, 5);
}
// 0x5AAA40 (thiscall, vf1)
uint8_t* __fastcall Grain_Size1C(Grain* self, re::Edx, int32_t* size) {
    *size = 0x1C;
    return self->data;
}

// 0x5AAA50 (thiscall, vf4): starts the sound at +8 of the same bank as a child
uint8_t __fastcall GrainPlayChild_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    if (Instance_BankOf(instance) != nullptr) {
        void* child = Engine_NewInstance(TheCueEngine, At<uint16_t>(instance, 6), At<uint16_t>(self->data, 8),
                                         instance);
        if (child != nullptr) {
            Instance_AddChild(instance, child);
            Instance_UpdateChildren(instance);
        }
    }
    return 1;
}

// 0x5AAAB0 (thiscall): type 0x14 (8 bytes, waits only)
Grain* __fastcall Grain14_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x00922618, 8, 0x14);
}
// 0x5AAAF0 (thiscall, vf2): its type
int32_t __fastcall Grain_Type(Grain* self) {
    return At<int32_t>(self->data, 0);
}
// 0x5AAB00 (thiscall): a loop start (0x15, 8 bytes)
Grain* __fastcall GrainLoopStart_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x00922634, 8, 0x15);
}
// 0x5AAB40 (thiscall): a loop end (0x16, 8 bytes)
Grain* __fastcall GrainLoopEnd_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x00922650, 8, 0x16);
}

// 0x5AAB80 (thiscall, vf4): back to the start when a loop start comes before it (the
// program's first grain, not the loop start's); the end otherwise
uint8_t __fastcall GrainLoopEnd_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    uint8_t* grain = Sound_PrevGrain(Instance_SoundOf(instance), self);
    while (grain != nullptr) {
        if (TypeOf(grain) == 0x15) {
            Instance_SetGrain(instance, Sound_GrainAt(Instance_SoundOf(instance), 0));
            return 1;
        }
        grain = Sound_PrevGrain(Instance_SoundOf(instance), grain);
    }
    Instance_SetGrain(instance, nullptr);
    return 1;
}

// 0x5AAC10 (thiscall): stops the cue (0x18, 8 bytes)
Grain* __fastcall GrainStopCue_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x0092266C, 8, 0x18);
}
// 0x5AAC50 (thiscall, vf4): ends the program
uint8_t __fastcall GrainStopCue_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    Instance_SetGrain(instance, nullptr);
    return 1;
}
// 0x5AAC80 (thiscall): plays one of n (0x19, 0xC bytes)
Grain* __fastcall GrainRandomPlay_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x00922688, 0xC, 0x19);
}

// 0x5AACC0 (thiscall, vf4): skips to one of +8 choices of +9 grains
uint8_t __fastcall GrainRandomPlay_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    int choice = RandBelow(int(At<int8_t>(self->data, 8)) - 1);
    Instance_Skip(instance, uint16_t(choice), At<int8_t>(self->data, 9), At<int8_t>(self->data, 8));
    return 1;
}

// 0x5AAD30 (thiscall): a random delay (0x1A, 0xC bytes)
Grain* __fastcall GrainRandomDelay_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009226A4, 0xC, 0x1A);
}

// 0x5AAD70 (thiscall, vf4): waits its ms plus a random share of +8 (in 1/240 s units,
// negated like the game)
uint8_t __fastcall GrainRandomDelay_Execute(Grain* self, re::Edx, uint8_t* instance) {
    int range = int(double(At<int32_t>(self->data, 8)) * -1000.0 / 240.0);
    int extra = RandBelow(-1 - range);
    return Instance_Wait(instance, At<int32_t>(self->data, 4) + extra) != 0 ? 1 : 0;
}

// 0x5AADD0 (thiscall): type 0x1B (8 bytes; its execute is always true)
Grain* __fastcall Grain1B_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009226C0, 8, 0x1B);
}
// 0x5AAE10 (thiscall, vf1)
uint8_t* __fastcall Grain_Size8(Grain* self, re::Edx, int32_t* size) {
    *size = 8;
    return self->data;
}
// 0x5AAE20 (thiscall): sets a register (0x1E, 0xC bytes)
Grain* __fastcall GrainSetRegister_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009226DC, 0xC, 0x1E);
}

// 0x5AAE60 (thiscall, vf4): register +8 = +9
uint8_t __fastcall GrainSetRegister_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    Register(instance, At<int8_t>(self->data, 8)) = At<int8_t>(self->data, 9);
    return 1;
}

// 0x5AAEC0 (thiscall): sets a register at random (0x1F, 0xC bytes)
Grain* __fastcall GrainSetRegisterRandom_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009226F8, 0xC, 0x1F);
}

// 0x5AAF00 (thiscall, vf4): register +8 = a value from +9 below +0xA
uint8_t __fastcall GrainSetRegisterRandom_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    int low = At<int8_t>(self->data, 9);
    int high = int(At<int8_t>(self->data, 0xA)) - 1;
    int value = RandBelow(high - low) + low;
    Register(instance, At<int8_t>(self->data, 8)) = int8_t(value);
    return 1;
}

// 0x5AAFB0 (thiscall): decrements a register (0x21, 0xC bytes)
Grain* __fastcall GrainDecRegister_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x00922714, 0xC, 0x21);
}

// 0x5AAFF0 (thiscall, vf4)
uint8_t __fastcall GrainDecRegister_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    Register(instance, At<int8_t>(self->data, 8)) -= 1;
    return 1;
}

// 0x5AB050 (thiscall): tests a register (0x22, 0xC bytes)
Grain* __fastcall GrainTestRegister_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x00922730, 0xC, 0x22);
}

// 0x5AB090 (thiscall, vf4): compares register +8 with +0xA (0 below, 1 equal, 2 above);
// not +9: skips the next grain
uint8_t __fastcall GrainTestRegister_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    uint8_t* data = self->data;
    int8_t value = Register(instance, At<int8_t>(data, 8));
    int32_t comparison = 0;
    if (value > At<int8_t>(data, 0xA)) {
        comparison = 2;
    } else if (Register(instance, At<int8_t>(data, 8)) == At<int8_t>(data, 0xA)) {
        comparison = 1;
    }
    if (int32_t(At<int8_t>(data, 9)) == comparison) {
        return 1;
    }
    uint8_t* next = Sound_NextGrain(Instance_SoundOf(instance), self);
    Instance_SetGrain(instance, Sound_NextGrain(Instance_SoundOf(instance), next));
    return 1;
}

// 0x5AB160 (thiscall): a marker (0x23, 0xC bytes; waits only)
Grain* __fastcall GrainMarker_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x0092274C, 0xC, 0x23);
}
// 0x5AB1A0 (thiscall, vf5): its data
void __fastcall Grain_SetData(Grain* self, re::Edx, uint8_t* data) {
    self->data = data;
}
// 0x5AB1B0 (thiscall): jumps to a marker (0x24, 0xC bytes)
Grain* __fastcall GrainGotoMarker_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x00922768, 0xC, 0x24);
}
// 0x5AB1F0 (thiscall, vf1)
uint8_t* __fastcall Grain_SizeC(Grain* self, re::Edx, int32_t* size) {
    *size = 0xC;
    return self->data;
}

// 0x5AB200 (thiscall, vf4): to the marker with its +8
uint8_t __fastcall GrainGotoMarker_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    uint8_t* grain = Sound_GrainAt(Instance_SoundOf(instance), 0);
    while (grain != nullptr) {
        if (TypeOf(grain) == 0x23 &&
            At<int32_t>(reinterpret_cast<Grain*>(grain)->data, 8) == At<int32_t>(self->data, 8)) {
            Instance_SetGrain(instance, grain);
            return 1;
        }
        grain = Sound_NextGrain(Instance_SoundOf(instance), grain);
    }
    return 1;
}

// 0x5AB280 (thiscall): jumps to a random marker (0x25, 0x10 bytes)
Grain* __fastcall GrainGotoRandomMarker_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x00922784, 0x10, 0x25);
}
// 0x5AB2C0 (thiscall, vf1)
uint8_t* __fastcall Grain_Size10(Grain* self, re::Edx, int32_t* size) {
    *size = 0x10;
    return self->data;
}

// 0x5AB2D0 (thiscall, vf4): to the marker with a number from +8 below +0xC
uint8_t __fastcall GrainGotoRandomMarker_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    uint8_t* grain = Sound_GrainAt(Instance_SoundOf(instance), 0);
    int32_t low = At<int32_t>(self->data, 8);
    int32_t high = At<int32_t>(self->data, 0xC) - 1;
    int32_t marker = RandBelow(high - low) + low;
    while (grain != nullptr) {
        if (TypeOf(grain) == 0x23 && At<int32_t>(reinterpret_cast<Grain*>(grain)->data, 8) == marker) {
            Instance_SetGrain(instance, grain);
            return 1;
        }
        grain = Sound_NextGrain(Instance_SoundOf(instance), grain);
    }
    return 1;
}

// 0x5AB380 (thiscall): waits for the voices (0x26, 8 bytes)
Grain* __fastcall GrainWaitForVoices_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009227A0, 8, 0x26);
}

// 0x5AB3C0 (thiscall, vf4): true once no voice plays (waiting again meanwhile)
uint8_t __fastcall GrainWaitForVoices_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    if (Instance_VoiceCount(instance) != 0) {
        At<int32_t>(instance, 0x1C) = 1;
        return 0;
    }
    return 1;
}

// 0x5AB400 (thiscall): plays the choices in turn (0x27, 0xC bytes)
Grain* __fastcall GrainPlayCycle_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009227BC, 0xC, 0x27);
}

// 0x5AB440 (thiscall, vf4): the next of +8 choices (the counter in the sound, +0x50)
uint8_t __fastcall GrainPlayCycle_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    int8_t cycle = int8_t(At<int8_t>(Instance_SoundOf(instance), 0x50) + 1);
    if (cycle >= At<int8_t>(self->data, 8)) {
        cycle = 0;
    }
    At<int8_t>(Instance_SoundOf(instance), 0x50) = cycle;
    Instance_Skip(instance, cycle, At<int8_t>(self->data, 9), At<int8_t>(self->data, 8));
    return 1;
}

// 0x5AB4B0 (thiscall): adds to a register (0x28, 0xC bytes)
Grain* __fastcall GrainAddToRegister_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009227D8, 0xC, 0x28);
}

// 0x5AB4F0 (thiscall, vf4): register +9 += +8
uint8_t __fastcall GrainAddToRegister_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    Register(instance, At<int8_t>(self->data, 9)) += At<int8_t>(self->data, 8);
    return 1;
}

// 0x5AB550 (thiscall): releases the voices (0x29, 8 bytes)
Grain* __fastcall GrainKeyOffVoices_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x009227F4, 8, 0x29);
}

// 0x5AB590 (thiscall, vf4)
uint8_t __fastcall GrainKeyOffVoices_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    Instance_ReleaseVoices(instance);
    return 1;
}

// 0x5AB5C0 (thiscall): where a stopped cue goes on (0x2B, 8 bytes)
Grain* __fastcall GrainOnStopMarker_ConstructImpl(Grain* self, re::Edx, void* data) {
    return Construct(self, data, 0x00922810, 8, 0x2B);
}

// 0x5AB600 (thiscall, vf4): reached while playing: the instance stops here (state 4)
uint8_t __fastcall GrainOnStopMarker_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) != 0) {
        At<int32_t>(instance, 0x1C) = 4;
    }
    return 0;
}

// 0x5AB630: builds a grain of a type over its data (in place, or allocated): null for the
// unknown types
Grain* __cdecl Grain_Create(int type, void* data, void* placement) {
    using Constructor = Grain*(__thiscall*)(void*, void*);
    Constructor constructor;
    switch (type) {
    case 0x01: constructor = GrainWave_Construct; break;
    case 0x04: constructor = Grain4_Construct; break;
    case 0x05: constructor = GrainPlayChild_Construct; break;
    case 0x14: constructor = Grain14_Construct; break;
    case 0x15: constructor = GrainLoopStart_Construct; break;
    case 0x16: constructor = GrainLoopEnd_Construct; break;
    case 0x18: constructor = GrainStopCue_Construct; break;
    case 0x19: constructor = GrainRandomPlay_Construct; break;
    case 0x1A: constructor = GrainRandomDelay_Construct; break;
    case 0x1B: constructor = Grain1B_Construct; break;
    case 0x1E: constructor = GrainSetRegister_Construct; break;
    case 0x1F: constructor = GrainSetRegisterRandom_Construct; break;
    case 0x21: constructor = GrainDecRegister_Construct; break;
    case 0x22: constructor = GrainTestRegister_Construct; break;
    case 0x23: constructor = GrainMarker_Construct; break;
    case 0x24: constructor = GrainGotoMarker_Construct; break;
    case 0x25: constructor = GrainGotoRandomMarker_Construct; break;
    case 0x26: constructor = GrainWaitForVoices_Construct; break;
    case 0x27: constructor = GrainPlayCycle_Construct; break;
    case 0x28: constructor = GrainAddToRegister_Construct; break;
    case 0x29: constructor = GrainKeyOffVoices_Construct; break;
    case 0x2B: constructor = GrainOnStopMarker_Construct; break;
    default: return nullptr;
    }
    if (placement != nullptr) {
        return constructor(placement, data);
    }
    void* memory = New(8);
    if (memory == nullptr) {
        return nullptr;
    }
    return constructor(memory, data);
}

// 0x5ABB20 (thiscall, vf4): starts a wave on a free voice: the bank's wave "<sound name><n>"
// (+0x24), its fades (+0x14, +0x18 in 1/127 s), the volume from +8 (0: +0xC/127, 1:
// random, 2..5: the instance's registers, 6..0x25: the globals; /127) times the
// instance's, divided by +0x20 voices, the pan (+0x1C), and the speaker gains (+0x10: a
// speaker set, else the root's six gains)
uint8_t __fastcall GrainWave_Execute(Grain* self, re::Edx, uint8_t* instance) {
    if (Instance_Wait(instance, WaitOf(self)) == 0) {
        return 0;
    }
    uint8_t* data = self->data;
    if (At<int32_t>(data, 0xC) <= 0) {
        return 1;
    }
    float instanceVolume = Instance_Volume(instance);
    if (instanceVolume <= 0.0f) {
        return 1;
    }
    if (Sounds_Fn594E60(TheSounds, 4) <= 0) {
        return 1;
    }
    uint8_t* slot = Instance_FreeVoiceSlot(instance);
    if (slot == nullptr) {
        return 1;
    }
    re::Ptr32<int32_t> voice = nullptr;
    char name[0x100];
    const char* soundName = reinterpret_cast<const char*>(Instance_SoundOf(instance) + 0xC);
    int length = 0;
    while ((name[length] = soundName[length]) != 0) {
        ++length;
    }
    Sprintf(name + length, reinterpret_cast<const char*>(0x008FFF78), At<int32_t>(self->data, 0x24));   // "%d"
    uint8_t* stream = bankStreams[At<int16_t>(instance, 6) * 2 + 1];
    if (stream == nullptr) {
        return 1;
    }
    int32_t streamHandle = At<int32_t>(stream, 4);
    if (streamHandle == -1) {
        return 1;
    }
    uint32_t state = 0;
    Engine_Fn5AF330(streamHandle, &state);
    if ((state & 4) == 0) {
        return 0;
    }
    int32_t index = int16_t(Engine_FreeVoice(TheCueEngine, &voice));
    if (voice == nullptr) {
        return 0;
    }
    Sound_Lock(0x2E);
    *voice = Engine_CreateVoice(At<int32_t>(Instance_BankOf(instance), 8), name);
    Sound_Unlock();
    if (*voice == -1) {
        return 1;
    }
    Engine_Fn5AF8C0(*voice, 1);
    Engine_Fn5AF660(*voice, 0.0f);
    data = self->data;
    int fadeIn = 0;
    int fadeOut = 0;
    if (At<int32_t>(data, 0x14) > 0) {
        fadeIn = int(double(At<int32_t>(data, 0x14)) / 127.0 * 1000.0);
    }
    if (At<int32_t>(data, 0x18) > 0) {
        fadeOut = int(double(At<int32_t>(data, 0x18)) / 127.0 * 1000.0);
    }
    Engine_Fn5AF570(*voice, fadeIn, fadeOut);
    float level = 1.0f;
    int32_t source = At<int32_t>(self->data, 8);
    if (source <= 0) {
        level = float(double(At<int32_t>(self->data, 0xC)) / 127.0);
    } else if (source == 1) {
        level = float(double(RandomInt(0, 0xFFFF)) / 65535.0);
    } else if (source <= 5) {
        level = float(double(At<int8_t>(instance, source + 0x22)) / 127.0);
    } else if (source <= 0x25) {
        level = float(double(*reinterpret_cast<int8_t*>(0x00CF7262 + source)) / 127.0);
    }
    level = Instance_Volume(instance) * level;
    int32_t voices = At<int32_t>(self->data, 0x20);
    if (voices > 0) {
        level = float(1.0 / double(voices) * double(level));
    }
    int millibels = VolumeToMillibels(level);
    float asFloat = float(millibels);
    Engine_SetVolumeDb(*voice, float(double(asFloat) * 0.009999999776482582));
    Engine_SetPan(*voice, At<int32_t>(self->data, 0x1C) != 0 ? 1.0f : 0.0f);
    re::Ptr32<float>* gains = Instance_RootGains(instance);
    int32_t speakers = At<int32_t>(self->data, 0x10);
    if (speakers != -1 && speakers != 0) {
        float first = (*gains)[0];
        Engine_Fn5AF700(*voice, speakers);
        Engine_Fn5AF810(*voice, 0, At<int32_t>(self->data, 0x10), first);
    } else {
        float* g = *gains;
        Engine_SetGains(*voice, g[0], g[1], g[2], g[3], g[4], g[5]);
    }
    *slot = uint8_t(index);
    return 1;
}

// 0x5ABEC0 (thiscall, vf0): the destructor
Grain* __fastcall Grain_Destruct(Grain* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x009225A0);
    if ((flags & 1) != 0) {
        Free(self);
    }
    return self;
}

// ---- registration ---------------------------------------------------------------------------

#define GRAINS_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(grains_original_, __LINE__);                                          \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(grains_original_, __LINE__), \
             re::Mode::Original)

GRAINS_REVERSED("GrainWave::GrainWave", 0x005AA940, &GrainWave_ConstructImpl);
GRAINS_REVERSED("GrainWave::Size", 0x005AA980, &GrainWave_Size);
GRAINS_REVERSED("Grain4::Grain4", 0x005AA990, &Grain4_ConstructImpl);
GRAINS_REVERSED("Grain::Size14", 0x005AA9D0, &Grain_Size14);
GRAINS_REVERSED("Grain::WaitOnly", 0x005AA9E0, &Grain_WaitOnly);
GRAINS_REVERSED("GrainPlayChild::GrainPlayChild", 0x005AAA00, &GrainPlayChild_ConstructImpl);
GRAINS_REVERSED("Grain::Size1C", 0x005AAA40, &Grain_Size1C);
GRAINS_REVERSED("GrainPlayChild::Execute", 0x005AAA50, &GrainPlayChild_Execute);
GRAINS_REVERSED("Grain14::Grain14", 0x005AAAB0, &Grain14_ConstructImpl);
GRAINS_REVERSED("Grain::Type", 0x005AAAF0, &Grain_Type);
GRAINS_REVERSED("GrainLoopStart::GrainLoopStart", 0x005AAB00, &GrainLoopStart_ConstructImpl);
GRAINS_REVERSED("GrainLoopEnd::GrainLoopEnd", 0x005AAB40, &GrainLoopEnd_ConstructImpl);
GRAINS_REVERSED("GrainLoopEnd::Execute", 0x005AAB80, &GrainLoopEnd_Execute);
GRAINS_REVERSED("GrainStopCue::GrainStopCue", 0x005AAC10, &GrainStopCue_ConstructImpl);
GRAINS_REVERSED("GrainStopCue::Execute", 0x005AAC50, &GrainStopCue_Execute);
GRAINS_REVERSED("GrainRandomPlay::GrainRandomPlay", 0x005AAC80, &GrainRandomPlay_ConstructImpl);
GRAINS_REVERSED("GrainRandomPlay::Execute", 0x005AACC0, &GrainRandomPlay_Execute);
GRAINS_REVERSED("GrainRandomDelay::GrainRandomDelay", 0x005AAD30, &GrainRandomDelay_ConstructImpl);
GRAINS_REVERSED("GrainRandomDelay::Execute", 0x005AAD70, &GrainRandomDelay_Execute);
GRAINS_REVERSED("Grain1B::Grain1B", 0x005AADD0, &Grain1B_ConstructImpl);
GRAINS_REVERSED("Grain::Size8", 0x005AAE10, &Grain_Size8);
GRAINS_REVERSED("GrainSetRegister::GrainSetRegister", 0x005AAE20, &GrainSetRegister_ConstructImpl);
GRAINS_REVERSED("GrainSetRegister::Execute", 0x005AAE60, &GrainSetRegister_Execute);
GRAINS_REVERSED("GrainSetRegisterRandom::GrainSetRegisterRandom", 0x005AAEC0, &GrainSetRegisterRandom_ConstructImpl);
GRAINS_REVERSED("GrainSetRegisterRandom::Execute", 0x005AAF00, &GrainSetRegisterRandom_Execute);
GRAINS_REVERSED("GrainDecRegister::GrainDecRegister", 0x005AAFB0, &GrainDecRegister_ConstructImpl);
GRAINS_REVERSED("GrainDecRegister::Execute", 0x005AAFF0, &GrainDecRegister_Execute);
GRAINS_REVERSED("GrainTestRegister::GrainTestRegister", 0x005AB050, &GrainTestRegister_ConstructImpl);
GRAINS_REVERSED("GrainTestRegister::Execute", 0x005AB090, &GrainTestRegister_Execute);
GRAINS_REVERSED("GrainMarker::GrainMarker", 0x005AB160, &GrainMarker_ConstructImpl);
GRAINS_REVERSED("Grain::SetData", 0x005AB1A0, &Grain_SetData);
GRAINS_REVERSED("GrainGotoMarker::GrainGotoMarker", 0x005AB1B0, &GrainGotoMarker_ConstructImpl);
GRAINS_REVERSED("Grain::SizeC", 0x005AB1F0, &Grain_SizeC);
GRAINS_REVERSED("GrainGotoMarker::Execute", 0x005AB200, &GrainGotoMarker_Execute);
GRAINS_REVERSED("GrainGotoRandomMarker::GrainGotoRandomMarker", 0x005AB280, &GrainGotoRandomMarker_ConstructImpl);
GRAINS_REVERSED("Grain::Size10", 0x005AB2C0, &Grain_Size10);
GRAINS_REVERSED("GrainGotoRandomMarker::Execute", 0x005AB2D0, &GrainGotoRandomMarker_Execute);
GRAINS_REVERSED("GrainWaitForVoices::GrainWaitForVoices", 0x005AB380, &GrainWaitForVoices_ConstructImpl);
GRAINS_REVERSED("GrainWaitForVoices::Execute", 0x005AB3C0, &GrainWaitForVoices_Execute);
GRAINS_REVERSED("GrainPlayCycle::GrainPlayCycle", 0x005AB400, &GrainPlayCycle_ConstructImpl);
GRAINS_REVERSED("GrainPlayCycle::Execute", 0x005AB440, &GrainPlayCycle_Execute);
GRAINS_REVERSED("GrainAddToRegister::GrainAddToRegister", 0x005AB4B0, &GrainAddToRegister_ConstructImpl);
GRAINS_REVERSED("GrainAddToRegister::Execute", 0x005AB4F0, &GrainAddToRegister_Execute);
GRAINS_REVERSED("GrainKeyOffVoices::GrainKeyOffVoices", 0x005AB550, &GrainKeyOffVoices_ConstructImpl);
GRAINS_REVERSED("GrainKeyOffVoices::Execute", 0x005AB590, &GrainKeyOffVoices_Execute);
GRAINS_REVERSED("GrainOnStopMarker::GrainOnStopMarker", 0x005AB5C0, &GrainOnStopMarker_ConstructImpl);
GRAINS_REVERSED("GrainOnStopMarker::Execute", 0x005AB600, &GrainOnStopMarker_Execute);
GRAINS_REVERSED("Grain::Create", 0x005AB630, &Grain_Create);
GRAINS_REVERSED("GrainWave::Execute", 0x005ABB20, &GrainWave_Execute);
GRAINS_REVERSED("Grain::~Grain", 0x005ABEC0, &Grain_Destruct);

} // namespace
