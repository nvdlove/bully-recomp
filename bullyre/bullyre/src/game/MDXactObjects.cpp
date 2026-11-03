// MDXact's objects (0x5AE9C0..0x5B06B5): the cue (0xA8 bytes around an IXACTCue: its
// fades and the speaker gains it sends as a matrix), the sound bank (0x18 bytes: an
// IXACTSoundBank and a std::vector of its cues, with the vector's checked-iterator code
// the compiler generated), the wave bank (0xC bytes), and the audio API that builds a
// command per call and runs it at once (see MDXactCommands.cpp).
//
// A cue: +0 IXACTCue*, +4 handle, +8 name (NiFixedString), +0xC index, +0x10 state (0
// stopped, 1 playing, 2 fading in, 3 fading out, 4 paused, 5 faded out), +0x14, +0x15
// fades, +0x18 speakers, +0x1C six {index, gain} (the gains, +0x7C count, +0x80 the
// table), +0x4C six more (the reverb sends, +0x84, +0x88), +0x94 fade start, +0x98 fade
// in, +0x9C fade out (ms), +0xA0 reverb mix, +0xA4 properties known.

#include <cstdint>
#include <initializer_list>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

GAME_VAR(int32_t, speakerMode, 0x00AE80E4);
GAME_VAR(int32_t, nextWaveBank, 0x00CFAD40);
GAME_VAR(int32_t, nextSoundBank, 0x00CFAD44);
GAME_VAR(int32_t, nextCue, 0x00CFAD48);
GAME_VAR(const char*, variableNameA, 0x00AE9080);
GAME_VAR(const char*, variableNameB, 0x00AE907C);
inline void* const speakerSection = reinterpret_cast<void*>(0x00CF7148);
inline const re::Ptr32<const char>* const emptyFixedString = reinterpret_cast<const re::Ptr32<const char>*>(0x00951028);

using SectionFn = void(__stdcall*)(void*);
inline void Lock() {
    re::GameFnAt<SectionFn>(uintptr_t(0x008FC17C))(speakerSection);
}
inline void Unlock() {
    re::GameFnAt<SectionFn>(uintptr_t(0x008FC178))(speakerSection);
}
using FreeMemoryFn = void(__stdcall*)(void*);
inline void CoTaskMemFree(void* p) {
    re::GameFnAt<FreeMemoryFn>(uintptr_t(0x008FC334))(p);
}

GAME_FN(void*, __cdecl, New, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline int __cdecl MemmoveS(void* dst, uint32_t size, const void* src, uint32_t count) { return crt::MemmoveS(dst, (size_t)size, src, (size_t)count); }
inline void __cdecl InvalidParameter() { crt::InvalidParameter(); }
inline void __stdcall CxxThrowException(void* object, void* info) { crt::CxxThrowException(object, (const void*)info); }
GAME_FN(void, __thiscall, Exception_Construct, 0x0085BDE2, void* self, const re::Ptr32<const char>* message);
GAME_FN(void, __thiscall, Exception_ConstructDefault, 0x0085BDD1, void* self);
GAME_FN(void, __thiscall, String_Assign, 0x0040C1B0, void* self, const char* text, uint32_t length);
GAME_FN(void, __thiscall, String_AssignFrom, 0x0040C0D0, void* self, const void* other, uint32_t position,
        uint32_t count);
GAME_FN(void, __thiscall, NiFixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(const char*, __cdecl, NiFixedString_Create, 0x00754760, const char* text);
GAME_FN(void, __thiscall, NiStringTable_Free, 0x00754550, void* table, re::Ptr32<const char>* string, uint32_t length);
GAME_FN(void, __cdecl, Command_Run, 0x005AC4C0, void* command);
GAME_FN(void*, __thiscall, Command_Construct, 0x005ABEE0, void* self, int32_t kind);
GAME_FN(void, __thiscall, Command_AddFloat, 0x005ABF10, void* self, float value);
GAME_FN(void, __thiscall, Command_AddByte, 0x005ABF30, void* self, int value);
GAME_FN(void, __thiscall, Command_AddInt, 0x005ABF50, void* self, uint32_t value);
GAME_FN(void*, __thiscall, StreamingWaveBankCommand_Construct, 0x005AC2C0, void* self, int32_t handle,
        const void* parameters);
GAME_FN(void*, __thiscall, PrepareByNameCommand_Construct, 0x005AC3B0, void* self, int32_t bank, int32_t cue,
        const char* name);
GAME_FN(void*, __thiscall, ReverbCommand_Construct, 0x005AC320, void* self, int32_t a, int32_t type);
GAME_FN(uint8_t, __cdecl, Xact_IsReverbForced, 0x005AC4E0);
GAME_FN(float, __cdecl, Xact_ReverbLevel, 0x005AC4F0);
GAME_FN(void, __cdecl, Data_AddWaveBank, 0x005AC9C0, int32_t handle);
GAME_FN(void, __cdecl, Data_AddSoundBank, 0x005ACA10, int32_t handle);
GAME_FN(void, __cdecl, Data_AddCue, 0x005ACA50, int32_t handle);
GAME_FN(void, __cdecl, Data_SetCuePaused, 0x005ACAA0, int32_t handle, int paused);
GAME_FN(void, __cdecl, Data_SetCueSpeakers, 0x005ACB00, int32_t handle, int speakers);
GAME_FN(uint8_t, __cdecl, Data_GetCuePaused, 0x005ACBD0, int32_t handle, uint8_t* paused);
GAME_FN(uint8_t, __cdecl, Data_GetCueSpeakers, 0x005ACC40, int32_t handle, uint8_t* speakers);
GAME_FN(void, __cdecl, Ref_ReleaseWaveBank, 0x005AD880, void* bank);
GAME_FN(void, __cdecl, Ref_ReleaseSoundBank, 0x005AD950, void* bank);
GAME_FN(void, __cdecl, Ref_AddCue, 0x005AD9B0, void* cue);
GAME_FN(void, __cdecl, Ref_ReleaseCue, 0x005ADA20, void* cue);
GAME_FN(void, __cdecl, Cues_Remove, 0x005AE990, void* cue);
// this file's
GAME_FN(uint8_t, __thiscall, Cue_Destroy, 0x005AE9C0, void* cue);
GAME_FN(void, __thiscall, Cue_GetState, 0x005AEA10, void* cue, uint32_t* state);
GAME_FN(void, __thiscall, Cue_Pause, 0x005AEA30, void* cue, int pause);
GAME_FN(void, __thiscall, Cue_Stop, 0x005AEA60, void* cue, int32_t flags);
GAME_FN(uint8_t, __thiscall, Cue_GetProperties, 0x005AEB90, void* cue, uint8_t* out);
GAME_FN(void, __thiscall, Cue_ApplyGains, 0x005AEC00, void* cue);
GAME_FN(void, __thiscall, Cue_UpdateFade, 0x005AED50, void* cue, uint32_t now);
GAME_FN(void*, __thiscall, Cue_Construct, 0x005AEE70, void* cue);
GAME_FN(void, __thiscall, Cue_Destruct, 0x005AEEC0, void* cue);
GAME_FN(void, __thiscall, Cue_Init, 0x005AEF00, void* cue, void* xactCue, const re::Ptr32<const char>* name, int index);
GAME_FN(uint32_t*, __cdecl, Vector_Allocate, 0x005AFAC0, uint32_t count, int);
GAME_FN(void, __cdecl, Vector_Fill, 0x005AFD30, uint32_t* first, uint32_t* last, const uint32_t* value);
GAME_FN(uint32_t*, __cdecl, Vector_CopyBackward, 0x005AFD50, uint32_t* first, uint32_t* last, uint32_t* dest);
GAME_FN(void, __thiscall, SoundBank_DestroyCue, 0x005AFDB0, void* bank, void* cue);
GAME_FN(uint32_t*, __thiscall, Vector_Umove, 0x005B0060, void* vector, uint32_t* first, uint32_t* last, uint32_t* dest);
GAME_FN(uint32_t*, __thiscall, Vector_Ufill, 0x005B0170, void* vector, uint32_t* dest, uint32_t count,
        const uint32_t* value);
GAME_FN(void, __cdecl, Vector_Xlen, 0x005B01A0);
GAME_FN(void, __thiscall, Vector_InsertN, 0x005B0210, void* vector, void* proxy, uint32_t* where, uint32_t count,
        const uint32_t* value);
GAME_FN(void*, __thiscall, Vector_Insert, 0x005B0450, void* vector, void* out, void* proxy, uint32_t* where,
        const uint32_t* value);
GAME_FN(void, __thiscall, Vector_PushBack, 0x005B0500, void* vector, const uint32_t* value);
GAME_FN(uint8_t*, __thiscall, SoundBank_PrepareCue, 0x005B0570, void* bank, const re::Ptr32<const char>* name, int index);

inline void Check(bool ok) {
    if (!ok) {
        InvalidParameter();
    }
}

using ComFn = long(__stdcall*)(void*);
// (XACT's objects: the shim's, native slots)
inline void* const* VtableOf(void* object) {
    return re::ComVtable(object);
}

// ---- a cue ------------------------------------------------------------------------------------

// 0x5AE9C0 (thiscall): stops and releases its XACT cue, forgets it
uint8_t __fastcall Cue_DestroyImpl(uint8_t* self) {
    void* xactCue = At<void*>(self, 0);
    if (xactCue != nullptr) {
        uint32_t state;
        using GetStateFn = long(__stdcall*)(void*, uint32_t*);
        re::PtrCast<GetStateFn>(VtableOf(xactCue)[2])(xactCue, &state);
        if ((state & 8) != 0) {
            using StopFn = long(__stdcall*)(void*, uint32_t);
            void* cue = At<void*>(self, 0);
            re::PtrCast<StopFn>(VtableOf(cue)[1])(cue, 1);
        }
        Ref_ReleaseCue(At<void*>(self, 0));
        At<int32_t>(self, 0x10) = 0;
        At<void*>(self, 0) = nullptr;
        Cues_Remove(self);
    }
    return 1;
}

// 0x5AEA10 (thiscall): XACT's state
void __fastcall Cue_GetStateImpl(uint8_t* self, re::Edx, uint32_t* state) {
    void* xactCue = At<void*>(self, 0);
    if (xactCue != nullptr) {
        using GetStateFn = long(__stdcall*)(void*, uint32_t*);
        re::PtrCast<GetStateFn>(VtableOf(xactCue)[2])(xactCue, state);
    }
}

// 0x5AEA30 (thiscall): pauses (state 4) or resumes (1)
void __fastcall Cue_PauseImpl(uint8_t* self, re::Edx, int pause) {
    void* xactCue = At<void*>(self, 0);
    if (xactCue == nullptr) {
        return;
    }
    using PauseFn = long(__stdcall*)(void*, uint32_t);
    re::PtrCast<PauseFn>(VtableOf(xactCue)[0x20 / 4])(xactCue, uint8_t(pause));
    At<int32_t>(self, 0x10) = uint8_t(pause) != 0 ? 4 : 1;
}

// 0x5AEA60 (thiscall): stops (fading out when it has a fade and isn't faded out yet)
void __fastcall Cue_StopImpl(uint8_t* self, re::Edx, int32_t flags) {
    void* xactCue = At<void*>(self, 0);
    if (xactCue == nullptr) {
        return;
    }
    uint32_t state = 0;
    using GetStateFn = long(__stdcall*)(void*, uint32_t*);
    re::PtrCast<GetStateFn>(VtableOf(xactCue)[2])(xactCue, &state);
    if ((state & 0x48) == 0) {
        return;
    }
    if (At<int32_t>(self, 0x9C) > 0 && At<int32_t>(self, 0x10) != 5) {
        At<int32_t>(self, 0x10) = 3;
        At<int32_t>(self, 0x94) = 0;
        return;
    }
    void* cue = At<void*>(self, 0);
    using StopFn = long(__stdcall*)(void*, uint32_t);
    re::PtrCast<StopFn>(VtableOf(cue)[1])(cue, uint32_t(flags));
    At<int32_t>(self, 0x10) = 0;
}

// 0x5AEAD0 (thiscall): the reverb mix
void __fastcall Cue_SetReverbMix(uint8_t* self, re::Edx, float mix) {
    At<float>(self, 0xA0) = mix;
}

// 0x5AEAE0 (thiscall): the six speaker gains
void __fastcall Voice_SetGains(uint8_t* self, re::Edx, float a, float b, float c, float d, float e, float f) {
    At<float>(self, 0x20) = a;
    At<float>(self, 0x28) = b;
    At<float>(self, 0x30) = c;
    At<float>(self, 0x38) = d;
    At<float>(self, 0x40) = e;
    At<float>(self, 0x48) = f;
}

using GetVariableIndexFn = uint16_t(__stdcall*)(void*, const char*);
using SetVariableFn = long(__stdcall*)(void*, uint32_t, float);

// 0x5AEB10 (thiscall): the variable named at 0xAE9080
void __fastcall Cue_SetVariableA(uint8_t* self, re::Edx, float value) {
    void* xactCue = At<void*>(self, 0);
    if (xactCue == nullptr) {
        return;
    }
    uint16_t index = re::PtrCast<GetVariableIndexFn>(VtableOf(xactCue)[0x14 / 4])(xactCue, variableNameA);
    void* cue = At<void*>(self, 0);
    re::PtrCast<SetVariableFn>(VtableOf(cue)[0x18 / 4])(cue, index, value);
}

// 0x5AEB50 (thiscall): the variable named at 0xAE907C (when it exists)
void __fastcall Cue_SetVariableB(uint8_t* self, re::Edx, float value) {
    void* xactCue = At<void*>(self, 0);
    if (xactCue == nullptr) {
        return;
    }
    uint16_t index = re::PtrCast<GetVariableIndexFn>(VtableOf(xactCue)[0x14 / 4])(xactCue, variableNameB);
    if (index == 0xFFFF) {
        return;
    }
    void* cue = At<void*>(self, 0);
    re::PtrCast<SetVariableFn>(VtableOf(cue)[0x18 / 4])(cue, index, value);
}

// 0x5AEB90 (thiscall): the properties' word at +0x12C (when prepared or playing)
uint8_t __fastcall Cue_GetPropertiesImpl(uint8_t* self, re::Edx, uint16_t* out) {
    uint8_t* properties = nullptr;
    uint8_t found = 0;
    void* xactCue = At<void*>(self, 0);
    if (xactCue == nullptr) {
        return 0;
    }
    uint32_t state;
    using GetStateFn = long(__stdcall*)(void*, uint32_t*);
    re::PtrCast<GetStateFn>(VtableOf(xactCue)[2])(xactCue, &state);
    void* cue = At<void*>(self, 0);
    if (cue == nullptr || state == 1 || state == 2) {
        return 0;
    }
    using GetPropertiesFn = long(__stdcall*)(void*, uint8_t**);
    long result = re::PtrCast<GetPropertiesFn>(VtableOf(cue)[0x24 / 4])(cue, &properties);
    if (result >= 0) {
        *out = At<uint16_t>(properties, 0x12C);
        found = 1;
    }
    CoTaskMemFree(properties);
    return found;
}

// 0x5AEC00 (thiscall): sends the gains as the output matrix (2 columns: the gains, the reverb
// sends; the speaker count's rows)
void __fastcall Cue_ApplyGainsImpl(uint8_t* self) {
    if (At<void*>(self, 0) == nullptr) {
        return;
    }
    Lock();
    int32_t mode = speakerMode;
    Unlock();
    int32_t rows = mode == 0 ? 1 : mode == 1 ? 2 : 6;
    float* matrix = static_cast<float*>(New(uint32_t(rows * 2) * 4));   // (the overflow check can't trip)
    for (int32_t i = 0; i < rows; ++i) {
        for (int j = 0; j < 2; ++j) {
            if (j == 0) {
                matrix[i * 2] = At<float*>(self, 0x80)[i * 2 + 1];
                continue;
            }
            float gain = At<float>(self, 0x20 + i * 8);
            At<uint8_t>(self, 0x4C + i * 8) = uint8_t(i);
            if (!(0.0f >= gain || gain != gain) && Xact_IsReverbForced() != 0) {
                At<float>(self, 0x50 + i * 8) = Xact_ReverbLevel() * At<float>(self, 0xA0) * gain;
            } else {
                At<float>(self, 0x50 + i * 8) = 0.0f;
            }
            matrix[j + i * 2] = At<float>(self, 0x50 + i * 8);
        }
    }
    void* cue = At<void*>(self, 0);
    using SetMatrixFn = long(__stdcall*)(void*, uint32_t, uint32_t, float*);
    re::PtrCast<SetMatrixFn>(VtableOf(cue)[0x10 / 4])(cue, 2, uint32_t(rows), matrix);
    Free(matrix);
}

// 0x5AED50 (thiscall): the fade in (2, to 1) or out (3, to 5) at the time given, scaling the
// gains, then sends them
void __fastcall Cue_UpdateFadeImpl(uint8_t* self, re::Edx, uint32_t now) {
    int32_t state = At<int32_t>(self, 0x10);
    if (state != 2 && state != 3 && state != 1) {
        return;
    }
    if (now != 0) {
        void* xactCue = At<void*>(self, 0);
        uint16_t index = re::PtrCast<GetVariableIndexFn>(VtableOf(xactCue)[0x14 / 4])(xactCue, variableNameB);
        if (index != 0xFFFF) {
            float value;
            using GetVariableFn = long(__stdcall*)(void*, uint32_t, float*);
            void* cue = At<void*>(self, 0);
            re::PtrCast<GetVariableFn>(VtableOf(cue)[0x1C / 4])(cue, index, &value);
        }
        state = At<int32_t>(self, 0x10);
        if (state == 2 || state == 3) {
            if (At<uint32_t>(self, 0x94) == 0) {
                At<uint32_t>(self, 0x94) = now;
            }
            int32_t elapsed = int32_t(now - At<uint32_t>(self, 0x94));
            float scale;
            if (state == 2) {
                scale = float(double(elapsed) / double(At<int32_t>(self, 0x98)));
                if (!(scale <= 1.0f || scale != scale)) {
                    At<int32_t>(self, 0x10) = 1;
                    scale = 1.0f;
                }
            } else {
                scale = float(1.0 - double(elapsed) / double(At<int32_t>(self, 0x9C)));
                if (!(scale >= 0.0f || scale != scale)) {
                    At<int32_t>(self, 0x10) = 5;
                    scale = 0.0f;
                }
            }
            for (int i = 0; i < 6; ++i) {
                At<float>(self, 0x20 + i * 8) = scale * At<float>(self, 0x20 + i * 8);
            }
        }
    }
    Cue_ApplyGains(self);
}

// 0x5AEE70 (thiscall): the constructor
uint8_t* __fastcall Cue_ConstructImpl(uint8_t* self) {
    At<const char*>(self, 8) = *emptyFixedString;
    At<float>(self, 0xA0) = 0.0f;
    At<void*>(self, 0) = nullptr;
    At<int32_t>(self, 4) = -1;
    At<uint16_t>(self, 0xC) = 0;
    At<int32_t>(self, 0x10) = 0;
    At<uint8_t>(self, 0x14) = 0;
    At<int32_t>(self, 0x18) = -1;
    At<int32_t>(self, 0x98) = 0;
    At<int32_t>(self, 0x9C) = 0;
    At<int32_t>(self, 0x94) = 0;
    At<uint8_t>(self, 0x15) = 0;
    At<uint8_t>(self, 0xA4) = 0;
    return self;
}

// 0x5AEEC0 (thiscall): the destructor
void __fastcall Cue_DestructImpl(uint8_t* self) {
    Cue_Destroy(self);
    re::Ptr32<const char>* name = &At<const char*>(self, 8);
    const char* text = *name;
    if (text == *emptyFixedString) {
        return;
    }
    uint32_t length = *reinterpret_cast<const uint16_t*>(text - 2);
    using InterlockedFn = long(__stdcall*)(volatile long*);
    long left = re::GameFnAt<InterlockedFn>(uintptr_t(0x008FC170))(
        reinterpret_cast<volatile long*>(const_cast<char*>(text - 8)));
    if (left == 1) {
        NiStringTable_Free(*reinterpret_cast<re::Ptr32<void>*>(0x020E216C), name, length);
    }
}

// 0x5AEF00 (thiscall): takes an XACT cue: the name, the two gain tables (zeroed), whether
// its properties can be read (+0x90 keeps a pointer to the stack table, like the game)
void __fastcall Cue_InitImpl(uint8_t* self, re::Edx, void* xactCue, const re::Ptr32<const char>* name, int index) {
    if (At<void*>(self, 0) != nullptr) {
        Cue_Destroy(self);
    }
    uint16_t cueIndex = uint16_t(index);
    At<void*>(self, 0) = xactCue;
    if (cueIndex == 0xFFFF) {
        return;
    }
    NiFixedString_Assign(&At<const char*>(self, 8), name);
    struct Entry {
        uint8_t index;
        uint8_t speaker;
        uint16_t pad;
        float gain;
    } table[6];
    for (int i = 0; i < 6; ++i) {
        table[i].gain = 0.0f;
        table[i].index = 0;
        table[i].speaker = uint8_t(i);
    }
    At<void*>(self, 0x90) = table;
    At<uint8_t*>(self, 0x80) = self + 0x1C;
    At<uint8_t>(self, 0x8C) = 6;
    At<uint8_t>(self, 0x7C) = 6;
    for (uint32_t i = 0; i < At<uint8_t>(self, 0x7C); ++i) {
        At<uint8_t>(self, 0x1C + int(i) * 8) = uint8_t(i);
        At<float>(self, 0x20 + int(i) * 8) = 0.0f;
    }
    At<uint8_t>(self, 0x84) = 6;
    At<uint8_t*>(self, 0x88) = self + 0x4C;
    for (uint32_t i = 0; i < At<uint8_t>(self, 0x84); ++i) {
        At<uint8_t>(self, 0x4C + int(i) * 8) = uint8_t(i);
        At<float>(self, 0x50 + int(i) * 8) = 0.0f;
    }
    At<uint16_t>(self, 0xC) = cueIndex;
    uint8_t* properties = nullptr;
    uint8_t found = 0;
    void* cue = At<void*>(self, 0);
    if (cue == nullptr) {
        return;
    }
    uint32_t state;
    using GetStateFn = long(__stdcall*)(void*, uint32_t*);
    re::PtrCast<GetStateFn>(VtableOf(cue)[2])(cue, &state);
    cue = At<void*>(self, 0);
    if (cue == nullptr || state == 1 || state == 2) {
        return;
    }
    using GetPropertiesFn = long(__stdcall*)(void*, uint8_t**);
    if (re::PtrCast<GetPropertiesFn>(VtableOf(cue)[0x24 / 4])(cue, &properties) >= 0) {
        found = 1;
    }
    CoTaskMemFree(properties);
    if (found != 0) {
        At<uint8_t>(self, 0xA4) = 1;
    }
}

// 0x5AF060 (thiscall): plays it (a fade in when it has fades): XACT's result, -1 when it
// isn't prepared
int32_t __fastcall Cue_PlayImpl(uint8_t* self, re::Edx, int32_t fadeIn, int32_t fadeOut) {
    if (At<void*>(self, 0) == nullptr) {
        return -1;
    }
    uint32_t state = 0;
    if (At<uint8_t>(self, 0xA4) == 0) {
        uint8_t properties[3];
        properties[2] = 0;
        if (Cue_GetProperties(self, properties) != 0) {
            At<uint8_t>(self, 0xA4) = 1;
            if (properties[2] > 0) {   // (never: only a word is written)
                At<uint8_t>(self, 0x15) = 1;
            }
        }
    }
    At<int32_t>(self, 0x10) = 1;
    if (At<uint8_t>(self, 0x15) != 0) {
        At<int32_t>(self, 0x98) = fadeIn;
        At<int32_t>(self, 0x9C) = fadeOut;
    }
    Cue_ApplyGains(self);
    void* xactCue = At<void*>(self, 0);
    using GetStateFn = long(__stdcall*)(void*, uint32_t*);
    re::PtrCast<GetStateFn>(VtableOf(xactCue)[2])(xactCue, &state);
    if ((state & 4) == 0) {
        At<int32_t>(self, 0x10) = 0;
        return -1;
    }
    void* cue = At<void*>(self, 0);
    long result = re::PtrCast<ComFn>(VtableOf(cue)[0])(cue);
    if (result < 0) {
        At<int32_t>(self, 0x10) = 0;
        return result;
    }
    if (At<int32_t>(self, 0x98) > 0) {
        At<int32_t>(self, 0x94) = 0;
        At<int32_t>(self, 0x10) = 2;
    }
    return result;
}

// ---- the API: a command per call, run at once ----------------------------------------------------

void* NewCommand(int32_t kind) {
    void* memory = New(0x2C);
    return memory != nullptr ? Command_Construct(memory, kind) : nullptr;
}

// 0x5AF150: loads a sound bank: its handle
int32_t __cdecl Audio_LoadSoundBank(int32_t data, int32_t size, int32_t a, int32_t b) {
    int32_t handle = nextSoundBank;
    nextSoundBank = handle + 1;
    Data_AddSoundBank(handle);
    void* command = NewCommand(2);
    Command_AddInt(command, uint32_t(handle));
    Command_AddInt(command, uint32_t(data));
    Command_AddInt(command, uint32_t(size));
    Command_AddInt(command, uint32_t(a));
    Command_AddInt(command, uint32_t(b));
    Command_Run(command);
    return handle;
}

// 0x5AF1D0: unloads a sound bank
void __cdecl Audio_UnloadSoundBank(int32_t handle) {
    void* command = NewCommand(3);
    Command_AddInt(command, uint32_t(handle));
    Command_Run(command);
}

// 0x5AF220: loads a wave bank: its handle
int32_t __cdecl Audio_LoadWaveBank(int32_t data, int32_t size, int32_t flags, int32_t allocation) {
    int32_t handle = nextWaveBank;
    nextWaveBank = handle + 1;
    Data_AddWaveBank(handle);
    void* command = NewCommand(0);
    Command_AddInt(command, uint32_t(handle));
    Command_AddInt(command, uint32_t(data));
    Command_AddInt(command, uint32_t(size));
    Command_AddInt(command, uint32_t(flags));
    Command_AddInt(command, uint32_t(allocation));
    Command_Run(command);
    return handle;
}

// 0x5AF2A0: creates a streaming wave bank: its handle
int32_t __cdecl Audio_LoadStreamingWaveBank(const void* parameters) {
    int32_t handle = nextWaveBank;
    nextWaveBank = handle + 1;
    Data_AddWaveBank(handle);
    void* memory = New(0x1C);
    void* command = memory != nullptr ? StreamingWaveBankCommand_Construct(memory, handle, parameters) : nullptr;
    Command_Run(command);
    return handle;
}

// 0x5AF2F0: unloads a wave bank
void __cdecl Audio_UnloadWaveBank(int32_t handle) {
    void* command = NewCommand(1);
    Command_AddInt(command, uint32_t(handle));
    Command_Run(command);
}

// 0x5AF350: prepares a cue by name: its handle
int32_t __cdecl Audio_PrepareCueByName(int32_t bank, const char* name) {
    int32_t handle = nextCue;
    nextCue = handle + 1;
    Data_AddCue(handle);
    void* memory = New(0x14);
    void* command = memory != nullptr ? PrepareByNameCommand_Construct(memory, bank, handle, name) : nullptr;
    Command_Run(command);
    return handle;
}

// 0x5AF3B0: prepares a cue by index: its handle
int32_t __cdecl Audio_PrepareCue(int32_t bank, int index) {
    int32_t handle = nextCue;
    nextCue = handle + 1;
    Data_AddCue(handle);
    void* command = NewCommand(4);
    Command_AddInt(command, uint32_t(bank));
    Command_AddInt(command, uint32_t(handle));
    Command_AddInt(command, uint16_t(index));
    Command_Run(command);
    return handle;
}

void RunInts(int32_t kind, std::initializer_list<uint32_t> values) {
    void* command = NewCommand(kind);
    for (uint32_t value : values) {
        Command_AddInt(command, value);
    }
    Command_Run(command);
}

// 0x5AF420, 0x5AF470, 0x5AF4C0, 0x5AF570: the sound bank's cue calls, the cue's play
void __cdecl Audio_StopCueOf(int32_t bank, int32_t cue, int32_t flags) {
    RunInts(5, {uint32_t(bank), uint32_t(cue), uint32_t(flags)});
}
void __cdecl Audio_DestroyCueOf(int32_t bank, int32_t cue) {
    RunInts(6, {uint32_t(bank), uint32_t(cue)});
}
void __cdecl Audio_StopCueIfDone(int32_t bank, int32_t cue, int32_t flags) {
    RunInts(7, {uint32_t(bank), uint32_t(cue), uint32_t(flags)});
}
void __cdecl Audio_PlayCue(int32_t cue, int32_t fadeIn, int32_t fadeOut) {
    RunInts(8, {uint32_t(cue), uint32_t(fadeIn), uint32_t(fadeOut)});
}

// 0x5AF510: a cue's paused flag (0 unknown)
uint8_t __cdecl Audio_IsCuePaused(int32_t cue) {
    uint8_t paused = 0;
    return Data_GetCuePaused(cue, &paused) != 0 ? paused : 0;
}

// 0x5AF530: a cue's speakers flag
uint8_t __cdecl Audio_CueHasSpeakers(int32_t cue) {
    uint8_t speakers = 0;
    return Data_GetCueSpeakers(cue, &speakers) != 0 ? speakers : 0;
}

// 0x5AF5C0: pauses a cue
void __cdecl Audio_PauseCue(int32_t cue, int pause) {
    Data_SetCuePaused(cue, pause);
    void* command = NewCommand(9);
    Command_AddInt(command, uint32_t(cue));
    Command_AddByte(command, pause);
    Command_Run(command);
}

void RunIntFloat(int32_t kind, int32_t cue, float value) {
    void* command = NewCommand(kind);
    Command_AddInt(command, uint32_t(cue));
    Command_AddFloat(command, value);
    Command_Run(command);
}
// 0x5AF610, 0x5AF660, 0x5AF6B0: a cue's variables and reverb mix
void __cdecl Audio_SetCueVariableB(int32_t cue, float value) {
    RunIntFloat(0xA, cue, value);
}
void __cdecl Audio_SetCueVariableA(int32_t cue, float value) {
    RunIntFloat(0xB, cue, value);
}
void __cdecl Audio_SetCueReverbMix(int32_t cue, float value) {
    RunIntFloat(0xC, cue, value);
}

// 0x5AF700: a cue's speaker set (flagged in the shared data when positive)
void __cdecl Audio_SetCueSpeakers(int32_t cue, int32_t speakers) {
    if (speakers > 0) {
        Data_SetCueSpeakers(cue, 1);
    }
    RunInts(0xD, {uint32_t(cue), uint32_t(speakers)});
}

// 0x5AF760: a cue's level
void __cdecl Audio_SetCueLevel(int32_t cue, int32_t level, int flag) {
    void* command = NewCommand(0xE);
    Command_AddInt(command, uint32_t(cue));
    Command_AddInt(command, uint32_t(level));
    Command_AddByte(command, flag);
    Command_Run(command);
}

// 0x5AF7B0: a cue's rotated pan
void __cdecl Audio_PanCueRotated(int32_t cue, int32_t level, int32_t angle, int flag) {
    void* command = NewCommand(0xF);
    Command_AddInt(command, uint32_t(cue));
    Command_AddInt(command, uint32_t(level));
    Command_AddInt(command, uint32_t(angle));
    Command_AddByte(command, flag);
    Command_Run(command);
}

// 0x5AF810: a cue's 5.1 pan by angle
void __cdecl Audio_PanCue51(int32_t cue, int32_t level, int32_t angle, float volume) {
    void* command = NewCommand(0x11);
    Command_AddInt(command, uint32_t(cue));
    Command_AddInt(command, uint32_t(level));
    Command_AddInt(command, uint32_t(angle));
    Command_AddFloat(command, volume);
    Command_Run(command);
}

// 0x5AF870: a cue's 5.1 pan by its speaker set
void __cdecl Audio_PanCue51Speakers(int32_t cue, int32_t level, float volume) {
    void* command = NewCommand(0x12);
    Command_AddInt(command, uint32_t(cue));
    Command_AddInt(command, uint32_t(level));
    Command_AddFloat(command, volume);
    Command_Run(command);
}

// 0x5AF8C0: silences a cue
void __cdecl Audio_SilenceCue(int32_t cue, int flag) {
    void* command = NewCommand(0x13);
    Command_AddInt(command, uint32_t(cue));
    Command_AddByte(command, flag);
    Command_Run(command);
}

// 0x5AF910: a cue's six gains
void __cdecl Audio_SetCueGains(int32_t cue, float a, float b, float c, float d, float e, float f) {
    void* command = NewCommand(0x14);
    Command_AddInt(command, uint32_t(cue));
    Command_AddFloat(command, a);
    Command_AddFloat(command, b);
    Command_AddFloat(command, c);
    Command_AddFloat(command, d);
    Command_AddFloat(command, e);
    Command_AddFloat(command, f);
    Command_Run(command);
}

// 0x5AF9B0, 0x5AF9E0, 0x5AFA50: the argument-less commands
void __cdecl Audio_PauseSoundBanks() {
    Command_Run(NewCommand(0x15));
}
void __cdecl Audio_ResumeSoundBanks() {
    Command_Run(NewCommand(0x16));
}
void __cdecl Audio_UnforceReverb() {
    Command_Run(NewCommand(0x17));
}

// 0x5AFA10: the reverb type
void __cdecl Audio_SetReverbType(int32_t a, int32_t type) {
    void* memory = New(0x10);
    Command_Run(memory != nullptr ? ReverbCommand_Construct(memory, a, type) : nullptr);
}

// ---- a sound bank ----------------------------------------------------------------------------
// +0 IXACTSoundBank*, +4 the vector's proxy, +8 first, +0xC last, +0x10 end, +0x14 handle

inline re::Ptr32<uint32_t>& First(uint8_t* bank) {
    return At<uint32_t*>(bank, 8);
}
inline re::Ptr32<uint32_t>& Last(uint8_t* bank) {
    return At<uint32_t*>(bank, 0xC);
}
inline uint32_t SizeOf(uint8_t* bank) {
    return uint32_t(Last(bank) - First(bank));
}

// 0x5AFAA0 (thiscall): stops one of its cues
void __fastcall SoundBank_StopCue(uint8_t* self, re::Edx, void* cue, int32_t flags) {
    if (At<void*>(self, 0) != nullptr && cue != nullptr) {
        Cue_Stop(cue, flags);
    }
}

// 0x5AFAC0: a pointer vector's storage (bad_alloc beyond 0x3FFFFFFF)
uint32_t* __cdecl Vector_AllocateImpl(uint32_t count, int) {
    if (count > 0 && 0xFFFFFFFFu / count < 4) {
        alignas(4) uint8_t error[0xC];
        re::Ptr32<const char> message = nullptr;
        Exception_Construct(error, &message);
        At<uint32_t>(error, 0) = 0x008FFF1C;   // std::bad_alloc
        CxxThrowException(error, reinterpret_cast<void*>(0x009C1BA0));
    }
    return static_cast<uint32_t*>(New(count * 4));
}

// 0x5AFB20 (thiscall): pauses its cues that have fades
void __fastcall SoundBank_PauseCues(uint8_t* self) {
    if (At<void*>(self, 0) == nullptr || First(self) == nullptr || SizeOf(self) == 0) {
        return;
    }
    for (uint32_t i = 0; First(self) != nullptr && i < SizeOf(self); ++i) {
        uint8_t* cue = reinterpret_cast<uint8_t*>(First(self)[i]);
        if (cue != nullptr && At<uint8_t>(cue, 0x15) != 0) {
            Cue_Pause(cue, 1);
        }
    }
}

// 0x5AFBE0 (thiscall): resumes them (the paused ones)
void __fastcall SoundBank_ResumeCues(uint8_t* self) {
    if (At<void*>(self, 0) == nullptr || First(self) == nullptr || SizeOf(self) == 0) {
        return;
    }
    for (uint32_t i = 0; First(self) != nullptr && i < SizeOf(self); ++i) {
        uint8_t* cue = reinterpret_cast<uint8_t*>(First(self)[i]);
        if (cue != nullptr && At<uint8_t>(cue, 0x15) != 0) {
            uint32_t state = 0;
            Cue_GetState(cue, &state);
            if (state == 4) {
                Cue_Pause(reinterpret_cast<void*>(First(self)[i]), 0);
            }
        }
    }
}

// 0x5AFCE0 (thiscall): a cue's name (its friendly name from XACT, "" failing)
re::Ptr32<const char>* __fastcall SoundBank_GetCueName(uint8_t* self, re::Edx, re::Ptr32<const char>* out, int32_t index) {
    void* bank = At<void*>(self, 0);
    alignas(4) char properties[0x10C];
    const char* text = reinterpret_cast<const char*>(0x008FEFBB);   // ""
    if (bank != nullptr) {
        using GetCuePropertiesFn = long(__stdcall*)(void*, int32_t, void*);
        if (re::PtrCast<GetCuePropertiesFn>(VtableOf(bank)[2])(bank, index, properties) == 0) {
            text = properties;
        }
    }
    *out = NiFixedString_Create(text);
    return out;
}

// 0x5AFD30: fills pointers
void __cdecl Vector_FillImpl(uint32_t* first, uint32_t* last, const uint32_t* value) {
    for (; first != last; ++first) {
        *first = *value;
    }
}

// 0x5AFD50: copies backwards (memmove): the destination's start
uint32_t* __cdecl Vector_CopyBackwardImpl(uint32_t* first, uint32_t* last, uint32_t* dest) {
    int32_t count = int32_t(last - first);
    uint32_t* start = dest - count;
    if (count > 0) {
        MemmoveS(start, uint32_t(count) * 4, first, uint32_t(count) * 4);
    }
    return start;
}

// 0x5AFD80 (thiscall): frees the vector
void __fastcall SoundBank_Tidy(uint8_t* self) {
    if (First(self) != nullptr) {
        Free(First(self));
    }
    First(self) = nullptr;
    Last(self) = nullptr;
    At<uint32_t*>(self, 0x10) = nullptr;
}

// the vector's find with its iterator checks (the vector object at +4)
uint32_t* FindCue(uint8_t* self, void* cue) {
    uint32_t* it = First(self);
    Check(it <= Last(self));
    for (;;) {
        uint32_t* last = Last(self);
        Check(First(self) <= last);
        if (it == last) {
            return nullptr;
        }
        Check(it < Last(self));
        if (reinterpret_cast<void*>(*it) == cue) {
            return it;
        }
        Check(it < Last(self));
        ++it;
    }
}
void EraseAt(uint8_t* self, uint32_t* it) {
    int32_t after = int32_t(Last(self) - (it + 1));
    if (after > 0) {
        MemmoveS(it, uint32_t(after) * 4, it + 1, uint32_t(after) * 4);
    }
    Last(self) -= 1;
}

// 0x5AFDB0 (thiscall): destroys one of its cues
void __fastcall SoundBank_DestroyCueImpl(uint8_t* self, re::Edx, void* cue) {
    uint32_t* it = FindCue(self, cue);
    if (it == nullptr) {
        return;
    }
    Check(it < Last(self));
    if (Cue_Destroy(reinterpret_cast<void*>(*it)) == 0) {
        return;
    }
    Check(it < Last(self));
    void* object = reinterpret_cast<void*>(*it);
    if (object != nullptr) {
        Cue_Destruct(object);
        Free(object);
    }
    Check(it < Last(self));
    *it = 0;
    EraseAt(self, it);
}

// 0x5AFE80 (thiscall): drops one of its cues (not destroyed)
void __fastcall SoundBank_RemoveCue(uint8_t* self, re::Edx, void* cue) {
    uint32_t* it = FindCue(self, cue);
    if (it != nullptr) {
        EraseAt(self, it);
    }
}

// 0x5AFF10 (thiscall): stops a cue, destroying it unless it fades
void __fastcall SoundBank_StopCueIfDone(uint8_t* self, re::Edx, uint8_t* cue, int32_t flags) {
    if (At<void*>(self, 0) == nullptr || cue == nullptr) {
        return;
    }
    Cue_Stop(cue, flags);
    int32_t state = At<int32_t>(cue, 0x10);
    if (state != 2 && state != 3) {
        SoundBank_DestroyCue(self, cue);
    }
}

// 0x5AFF50 (thiscall): its cues' fades (from the last), destroying the faded-out ones
void __fastcall SoundBank_Update(uint8_t* self, re::Edx, uint32_t now) {
    if (At<void*>(self, 0) == nullptr || First(self) == nullptr || SizeOf(self) == 0) {
        return;
    }
    for (int32_t i = int32_t(SizeOf(self)) - 1; i >= 0; --i) {
        Check(First(self) != nullptr && uint32_t(i) < SizeOf(self));
        if (First(self)[i] == 0) {
            continue;
        }
        Cue_UpdateFade(reinterpret_cast<void*>(First(self)[i]), now);
        Check(First(self) != nullptr && uint32_t(i) < SizeOf(self));
        if (At<int32_t>(reinterpret_cast<void*>(First(self)[i]), 0x10) != 5) {
            continue;
        }
        uint8_t* cue = reinterpret_cast<uint8_t*>(First(self)[i]);
        if (At<void*>(self, 0) == nullptr || cue == nullptr) {
            continue;
        }
        Cue_Stop(cue, 1);
        int32_t state = At<int32_t>(cue, 0x10);
        if (state != 2 && state != 3) {
            SoundBank_DestroyCue(self, cue);
        }
    }
}

// 0x5B0060 (thiscall, a vector): moves pointers (memmove): the destination's end
uint32_t* __fastcall Vector_UmoveImpl(void*, re::Edx, uint32_t* first, uint32_t* last, uint32_t* dest) {
    uint32_t bytes = uint32_t(last - first) * 4;
    uint32_t* end = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(dest) + bytes);
    if (bytes != 0) {
        MemmoveS(dest, bytes, first, bytes);
    }
    return end;
}

// 0x5B0090 (thiscall): destroys all its cues and releases the XACT sound bank
uint8_t __fastcall SoundBank_Release(uint8_t* self) {
    if (At<void*>(self, 0) == nullptr) {
        return 1;
    }
    if (First(self) != nullptr && SizeOf(self) != 0) {
        for (int32_t i = int32_t(SizeOf(self)) - 1; i >= 0; --i) {
            Check(First(self) != nullptr && uint32_t(i) < SizeOf(self));
            void* cue = reinterpret_cast<void*>(First(self)[i]);
            if (cue != nullptr && Cue_Destroy(cue) != 0) {
                Cue_Destruct(cue);
                Free(cue);
            }
        }
        uint32_t* last = Last(self);
        Check(First(self) <= Last(self));
        uint32_t* first = First(self);
        Check(first <= Last(self));
        if (first != last) {
            int32_t after = int32_t(Last(self) - last);
            uint32_t* end = first + after;
            if (after > 0) {
                MemmoveS(first, uint32_t(after) * 4, last, uint32_t(after) * 4);
            }
            Last(self) = end;
        }
    }
    Ref_ReleaseSoundBank(At<void*>(self, 0));
    At<void*>(self, 0) = nullptr;
    return 1;
}

// 0x5B0170 (thiscall, a vector): fills count pointers: the end
uint32_t* __fastcall Vector_UfillImpl(void*, re::Edx, uint32_t* dest, uint32_t count, const uint32_t* value) {
    for (uint32_t i = 0; i < count; ++i) {
        dest[i] = *value;
    }
    return dest + count;
}

// 0x5B01A0: throws std::length_error("vector<T> too long")
void __cdecl Vector_XlenImpl() {
    alignas(4) uint8_t message[0x1C];
    At<uint32_t>(message, 0x18) = 0xF;
    At<uint32_t>(message, 0x14) = 0;
    message[4] = 0;
    String_Assign(message, reinterpret_cast<const char*>(0x008FFFF4), 0x12);
    alignas(4) uint8_t error[0x28];
    Exception_ConstructDefault(error);
    At<uint32_t>(error, 0) = 0x008FFFCC;   // std::logic_error
    At<uint32_t>(error, 0x24) = 0xF;
    At<uint32_t>(error, 0x20) = 0;
    error[0x10] = 0;
    String_AssignFrom(error + 0xC, message, 0, 0xFFFFFFFF);
    At<uint32_t>(error, 0) = 0x008FFFD8;   // std::length_error
    CxxThrowException(error, reinterpret_cast<void*>(0x009C1DE0));
}

// 0x5B0210 (thiscall, a vector at +4..): inserts count copies of *value at where (the game's
// catch that frees the new storage can't be reached: pointer copies don't throw)
void __fastcall Vector_InsertNImpl(uint8_t* self, re::Edx, void*, uint32_t* where, uint32_t count,
                                   const uint32_t* value) {
    uint32_t copy = *value;
    re::Ptr32<uint32_t>& first = At<uint32_t*>(self, 4);
    re::Ptr32<uint32_t>& last = At<uint32_t*>(self, 8);
    re::Ptr32<uint32_t>& end = At<uint32_t*>(self, 0xC);
    uint32_t capacity = first == nullptr ? 0 : uint32_t(end - first);
    if (count == 0) {
        return;
    }
    uint32_t size = first == nullptr ? 0 : uint32_t(last - first);
    if (0x3FFFFFFF - size < count) {
        Vector_Xlen();
    }
    size = first == nullptr ? 0 : uint32_t(last - first);
    if (capacity < size + count) {
        uint32_t half = capacity >> 1;
        capacity = 0x3FFFFFFF - half < capacity ? 0 : capacity + half;
        size = first == nullptr ? 0 : uint32_t(last - first);
        if (capacity < size + count) {
            capacity = (first == nullptr ? 0 : uint32_t(last - first)) + count;
        }
        uint32_t* storage = Vector_Allocate(capacity, 0);
        uint32_t* p = Vector_Umove(self, first, where, storage);
        p = Vector_Ufill(self, p, count, &copy);
        Vector_Umove(self, where, last, p);
        count += first == nullptr ? 0 : uint32_t(last - first);
        if (first != nullptr) {
            Free(first);
        }
        end = storage + capacity;
        last = storage + count;
        first = storage;
        return;
    }
    uint32_t* oldLast = last;
    if (uint32_t(oldLast - where) < count) {
        Vector_Umove(self, where, oldLast, where + count);
        Vector_Ufill(self, last, count - uint32_t(last - where), &copy);
        last += count;
        Vector_Fill(where, last - count, &copy);
        return;
    }
    uint32_t* tail = oldLast - count;
    last = Vector_Umove(self, tail, oldLast, oldLast);
    Vector_CopyBackward(where, tail, oldLast);
    Vector_Fill(where, where + count, &copy);
}

// 0x5B0450 (thiscall, a vector): inserts one pointer: the iterator at it
re::Ptr32<uint32_t>* __fastcall Vector_InsertImpl(uint8_t* self, re::Edx, re::Ptr32<uint32_t>* out, void* proxy, uint32_t* where,
                                         const uint32_t* value) {
    uint32_t* first = At<uint32_t*>(self, 4);
    uint32_t offset;
    if (first == nullptr || At<uint32_t*>(self, 8) - first == 0) {
        offset = 0;
    } else {
        Check(first <= At<uint32_t*>(self, 8));
        Check(proxy != nullptr && proxy == self);
        offset = uint32_t(where - first);
    }
    Vector_InsertN(self, proxy, where, 1, value);
    first = At<uint32_t*>(self, 4);
    Check(first <= At<uint32_t*>(self, 8));
    uint32_t* it = first + offset;
    Check(!(it > At<uint32_t*>(self, 8)) && !(it < At<uint32_t*>(self, 4)));
    out[1] = it;
    out[0] = reinterpret_cast<uint32_t*>(self);
    return out;
}

// 0x5B04E0 (thiscall): the sound bank's constructor
uint8_t* __fastcall SoundBank_Construct(uint8_t* self, re::Edx, void* xactBank, int32_t handle) {
    At<void*>(self, 8) = nullptr;
    At<void*>(self, 0xC) = nullptr;
    At<void*>(self, 0x10) = nullptr;
    At<void*>(self, 0) = xactBank;
    At<int32_t>(self, 0x14) = handle;
    return self;
}

// 0x5B0500 (thiscall, a vector): push_back
void __fastcall Vector_PushBackImpl(uint8_t* self, re::Edx, const uint32_t* value) {
    uint32_t* first = At<uint32_t*>(self, 4);
    uint32_t size = first == nullptr ? 0 : uint32_t(At<uint32_t*>(self, 8) - first);
    if (first != nullptr && size < uint32_t(At<uint32_t*>(self, 0xC) - first)) {
        *At<uint32_t*>(self, 8) = *value;
        At<uint32_t*>(self, 8) += 1;
        return;
    }
    uint32_t* last = At<uint32_t*>(self, 8);
    Check(first <= last);
    uint32_t* iterator[2];
    Vector_Insert(self, iterator, self, last, value);
}

// 0x5B0570 (thiscall): prepares a cue by index (0xFFFF none): a new cue object, kept
uint8_t* __fastcall SoundBank_PrepareCueImpl(uint8_t* self, re::Edx, const re::Ptr32<const char>* name, int index) {
    uint16_t cueIndex = uint16_t(index);
    if (cueIndex == 0xFFFF) {
        return nullptr;
    }
    void* bank = At<void*>(self, 0);
    if (bank == nullptr) {
        return nullptr;
    }
    void* xactCue = nullptr;
    using PrepareFn = long(__stdcall*)(void*, uint32_t, uint32_t, uint32_t, void**);
    if (re::PtrCast<PrepareFn>(VtableOf(bank)[3])(bank, uint32_t(index), 0, 0, &xactCue) != 0) {
        return nullptr;
    }
    if (xactCue == nullptr) {
        return nullptr;
    }
    Ref_AddCue(xactCue);
    void* memory = New(0xA8);
    uint8_t* cue = memory != nullptr ? static_cast<uint8_t*>(Cue_Construct(memory)) : nullptr;
    if (cue == nullptr) {
        Ref_ReleaseCue(xactCue);
        return cue;
    }
    Cue_Init(cue, xactCue, name, index);
    uint32_t kept = uint32_t(reinterpret_cast<uintptr_t>(cue));
    Vector_PushBack(self + 4, &kept);
    return cue;
}

// 0x5B0630 (thiscall): prepares a cue by name
uint8_t* __fastcall SoundBank_PrepareCueByName(uint8_t* self, re::Edx, const re::Ptr32<const char>* name) {
    void* bank = At<void*>(self, 0);
    if (bank == nullptr) {
        return nullptr;
    }
    using GetCueIndexFn = uint16_t(__stdcall*)(void*, const char*);
    uint16_t index = re::PtrCast<GetCueIndexFn>(VtableOf(bank)[0])(bank, *name);
    return SoundBank_PrepareCue(self, name, index);
}

// ---- a wave bank ----------------------------------------------------------------------------

// 0x5B0660 (thiscall): the constructor
uint8_t* __fastcall WaveBank_Construct(uint8_t* self, re::Edx, void* xactBank, int streaming, int32_t handle) {
    At<void*>(self, 0) = xactBank;
    At<uint8_t>(self, 4) = uint8_t(streaming);
    At<int32_t>(self, 8) = handle;
    return self;
}

// 0x5B0680 (thiscall): releases the XACT wave bank
uint8_t __fastcall WaveBank_Release(uint8_t* self) {
    if (At<void*>(self, 0) != nullptr) {
        Ref_ReleaseWaveBank(At<void*>(self, 0));
        At<void*>(self, 0) = nullptr;
    }
    return 1;
}

// 0x5B06A0 (thiscall): XACT's state
void __fastcall WaveBank_GetState(uint8_t* self, re::Edx, uint32_t* state) {
    void* bank = At<void*>(self, 0);
    if (bank != nullptr) {
        using GetStateFn = long(__stdcall*)(void*, uint32_t*);
        re::PtrCast<GetStateFn>(VtableOf(bank)[0x1C / 4])(bank, state);
    }
}

// ---- registration ---------------------------------------------------------------------------

#define MDXOBJ_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(mdxobj_original_, __LINE__);                                          \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(mdxobj_original_, __LINE__), \
             re::Mode::Original)

MDXOBJ_REVERSED("MDXact_Cue::Destroy", 0x005AE9C0, &Cue_DestroyImpl);
MDXOBJ_REVERSED("MDXact_Cue::GetState", 0x005AEA10, &Cue_GetStateImpl);
MDXOBJ_REVERSED("MDXact_Cue::Pause", 0x005AEA30, &Cue_PauseImpl);
MDXOBJ_REVERSED("MDXact_Cue::Stop", 0x005AEA60, &Cue_StopImpl);
MDXOBJ_REVERSED("MDXact_Cue::SetReverbMix", 0x005AEAD0, &Cue_SetReverbMix);
MDXOBJ_REVERSED("MDXact_Cue::SetGains", 0x005AEAE0, &Voice_SetGains);
MDXOBJ_REVERSED("MDXact_Cue::SetVariableA", 0x005AEB10, &Cue_SetVariableA);
MDXOBJ_REVERSED("MDXact_Cue::SetVariableB", 0x005AEB50, &Cue_SetVariableB);
MDXOBJ_REVERSED("MDXact_Cue::GetProperties", 0x005AEB90, &Cue_GetPropertiesImpl);
MDXOBJ_REVERSED("MDXact_Cue::ApplyGains", 0x005AEC00, &Cue_ApplyGainsImpl);
MDXOBJ_REVERSED("MDXact_Cue::UpdateFade", 0x005AED50, &Cue_UpdateFadeImpl);
MDXOBJ_REVERSED("MDXact_Cue::MDXact_Cue", 0x005AEE70, &Cue_ConstructImpl);
MDXOBJ_REVERSED("MDXact_Cue::~MDXact_Cue", 0x005AEEC0, &Cue_DestructImpl);
MDXOBJ_REVERSED("MDXact_Cue::Init", 0x005AEF00, &Cue_InitImpl);
MDXOBJ_REVERSED("MDXact_Cue::Play", 0x005AF060, &Cue_PlayImpl);
MDXOBJ_REVERSED("Audio::LoadSoundBank", 0x005AF150, &Audio_LoadSoundBank);
MDXOBJ_REVERSED("Audio::UnloadSoundBank", 0x005AF1D0, &Audio_UnloadSoundBank);
MDXOBJ_REVERSED("Audio::LoadWaveBank", 0x005AF220, &Audio_LoadWaveBank);
MDXOBJ_REVERSED("Audio::LoadStreamingWaveBank", 0x005AF2A0, &Audio_LoadStreamingWaveBank);
MDXOBJ_REVERSED("Audio::UnloadWaveBank", 0x005AF2F0, &Audio_UnloadWaveBank);
MDXOBJ_REVERSED("Audio::PrepareCueByName", 0x005AF350, &Audio_PrepareCueByName);
MDXOBJ_REVERSED("Audio::PrepareCue", 0x005AF3B0, &Audio_PrepareCue);
MDXOBJ_REVERSED("Audio::StopCueOf", 0x005AF420, &Audio_StopCueOf);
MDXOBJ_REVERSED("Audio::DestroyCueOf", 0x005AF470, &Audio_DestroyCueOf);
MDXOBJ_REVERSED("Audio::StopCueIfDone", 0x005AF4C0, &Audio_StopCueIfDone);
MDXOBJ_REVERSED("Audio::IsCuePaused", 0x005AF510, &Audio_IsCuePaused);
MDXOBJ_REVERSED("Audio::CueHasSpeakers", 0x005AF530, &Audio_CueHasSpeakers);
MDXOBJ_REVERSED("Audio::PlayCue", 0x005AF570, &Audio_PlayCue);
MDXOBJ_REVERSED("Audio::PauseCue", 0x005AF5C0, &Audio_PauseCue);
MDXOBJ_REVERSED("Audio::SetCueVariableB", 0x005AF610, &Audio_SetCueVariableB);
MDXOBJ_REVERSED("Audio::SetCueVariableA", 0x005AF660, &Audio_SetCueVariableA);
MDXOBJ_REVERSED("Audio::SetCueReverbMix", 0x005AF6B0, &Audio_SetCueReverbMix);
MDXOBJ_REVERSED("Audio::SetCueSpeakers", 0x005AF700, &Audio_SetCueSpeakers);
MDXOBJ_REVERSED("Audio::SetCueLevel", 0x005AF760, &Audio_SetCueLevel);
MDXOBJ_REVERSED("Audio::PanCueRotated", 0x005AF7B0, &Audio_PanCueRotated);
MDXOBJ_REVERSED("Audio::PanCue51", 0x005AF810, &Audio_PanCue51);
MDXOBJ_REVERSED("Audio::PanCue51Speakers", 0x005AF870, &Audio_PanCue51Speakers);
MDXOBJ_REVERSED("Audio::SilenceCue", 0x005AF8C0, &Audio_SilenceCue);
MDXOBJ_REVERSED("Audio::SetCueGains", 0x005AF910, &Audio_SetCueGains);
MDXOBJ_REVERSED("Audio::PauseSoundBanks", 0x005AF9B0, &Audio_PauseSoundBanks);
MDXOBJ_REVERSED("Audio::ResumeSoundBanks", 0x005AF9E0, &Audio_ResumeSoundBanks);
MDXOBJ_REVERSED("Audio::SetReverbType", 0x005AFA10, &Audio_SetReverbType);
MDXOBJ_REVERSED("Audio::UnforceReverb", 0x005AFA50, &Audio_UnforceReverb);
MDXOBJ_REVERSED("MDXact_SoundBank::StopCue", 0x005AFAA0, &SoundBank_StopCue);
MDXOBJ_REVERSED("MDXactCueVector::Allocate", 0x005AFAC0, &Vector_AllocateImpl);
MDXOBJ_REVERSED("MDXact_SoundBank::PauseCues", 0x005AFB20, &SoundBank_PauseCues);
MDXOBJ_REVERSED("MDXact_SoundBank::ResumeCues", 0x005AFBE0, &SoundBank_ResumeCues);
MDXOBJ_REVERSED("MDXact_SoundBank::GetCueName", 0x005AFCE0, &SoundBank_GetCueName);
MDXOBJ_REVERSED("MDXactCueVector::Fill", 0x005AFD30, &Vector_FillImpl);
MDXOBJ_REVERSED("MDXactCueVector::CopyBackward", 0x005AFD50, &Vector_CopyBackwardImpl);
MDXOBJ_REVERSED("MDXact_SoundBank::Tidy", 0x005AFD80, &SoundBank_Tidy);
MDXOBJ_REVERSED("MDXact_SoundBank::DestroyCue", 0x005AFDB0, &SoundBank_DestroyCueImpl);
MDXOBJ_REVERSED("MDXact_SoundBank::RemoveCue", 0x005AFE80, &SoundBank_RemoveCue);
MDXOBJ_REVERSED("MDXact_SoundBank::StopCueIfDone", 0x005AFF10, &SoundBank_StopCueIfDone);
MDXOBJ_REVERSED("MDXact_SoundBank::Update", 0x005AFF50, &SoundBank_Update);
MDXOBJ_REVERSED("MDXactCueVector::Umove", 0x005B0060, &Vector_UmoveImpl);
MDXOBJ_REVERSED("MDXact_SoundBank::Release", 0x005B0090, &SoundBank_Release);
MDXOBJ_REVERSED("MDXactCueVector::Ufill", 0x005B0170, &Vector_UfillImpl);
MDXOBJ_REVERSED("MDXactCueVector::Xlen", 0x005B01A0, &Vector_XlenImpl);
// (regcheck: callers 0x40C5DA, 0x5B0274 use ecx/edx after it, but it never returns: it throws)
MDXOBJ_REVERSED("MDXactCueVector::InsertN", 0x005B0210, &Vector_InsertNImpl);
MDXOBJ_REVERSED("MDXactCueVector::Insert", 0x005B0450, &Vector_InsertImpl);
MDXOBJ_REVERSED("MDXact_SoundBank::MDXact_SoundBank", 0x005B04E0, &SoundBank_Construct);
MDXOBJ_REVERSED("MDXactCueVector::PushBack", 0x005B0500, &Vector_PushBackImpl);
MDXOBJ_REVERSED("MDXact_SoundBank::PrepareCue", 0x005B0570, &SoundBank_PrepareCueImpl);
MDXOBJ_REVERSED("MDXact_SoundBank::PrepareCueByName", 0x005B0630, &SoundBank_PrepareCueByName);
MDXOBJ_REVERSED("MDXact_WaveBank::MDXact_WaveBank", 0x005B0660, &WaveBank_Construct);
MDXOBJ_REVERSED("MDXact_WaveBank::Release", 0x005B0680, &WaveBank_Release);
MDXOBJ_REVERSED("MDXact_WaveBank::GetState", 0x005B06A0, &WaveBank_GetState);

} // namespace
