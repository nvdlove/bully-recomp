// MDXact's commands and maps (0x5ABEE0..0x5AC623, 0x5AC630..0x5AC85F, 0x5ADD70..0x5AE29C):
// the commands the game's audio calls build and run at once (MDXactSimpleCommand, 0x2C
// bytes: a kind and up to seven arguments; and three specialised ones), the XACT
// engine's shutdown and reverb state, and the NiTPointerMap instantiations that hold the
// engine's banks, cues and their shared data (constructors, destructors, RemoveAll, GetAt).
//
// The globals (0xCFACC0..): +0 initialised, +0x20 the IXACTEngine, +0xC/+0x10/+0x14 the
// shared-data maps, +0x24.. the object maps, +0x44 the reverb type, +0x48 reverb forced.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

GAME_VAR(void*, xactInitialised, 0x00CFACC0);
GAME_VAR(uint8_t*, xactEngine, 0x00CFACE0);            // IXACTEngine*
GAME_VAR(void*, xactSettings, 0x00CFACFC);
GAME_VAR(int32_t, xactReverbType, 0x00CFAD04);
GAME_VAR(uint8_t, xactReverbForced, 0x00CFAD08);
GAME_ARRAY(float, xactReverbLevels, 0x009229C8);
inline void* const TheSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline const re::Ptr32<const char>* const emptyFixedString = reinterpret_cast<const re::Ptr32<const char>*>(0x00951028);

GAME_FN(void*, __cdecl, New, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline void* __cdecl Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(void*, __cdecl, NiMalloc, 0x00752300, uint32_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, uint32_t size);
GAME_FN(void, __cdecl, NiDelete, 0x00752540, void* p, uint32_t size);
GAME_FN(void, __thiscall, NiFixedString_Set, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(void, __thiscall, NiStringTable_Free, 0x00754550, void* table, re::Ptr32<const char>* string, uint32_t length);
GAME_FN(void, __thiscall, Sounds_Fn596360, 0x00596360, void* sounds);
// the handlers the commands run (MDXactManager.cpp)
GAME_FN(void, __cdecl, Xact_LoadWaveBank, 0x005AE690, int32_t, uintptr_t, int32_t, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_UnloadWaveBank, 0x005AE7B0, int32_t);
GAME_FN(void, __cdecl, Xact_LoadSoundBank, 0x005AE590, int32_t, uintptr_t, int32_t, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_UnloadSoundBank, 0x005AE630, int32_t);
GAME_FN(void, __cdecl, Xact_PrepareCue, 0x005AE8E0, int32_t, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_StopCueOf, 0x005ACF70, int32_t, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_DestroyCueOf, 0x005ACFE0, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_StopCueOf2, 0x005AD050, int32_t, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_PlayCue, 0x005AD0C0, int32_t, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_PauseCue, 0x005AD110, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_SetCueVariableB, 0x005AD150, int32_t, float);
GAME_FN(void, __cdecl, Xact_SetCueVariableA, 0x005AD1A0, int32_t, float);
GAME_FN(void, __cdecl, Xact_SetCueReverbMix, 0x005AD1F0, int32_t, float);
GAME_FN(void, __cdecl, Xact_SetCueSpeakers, 0x005AD240, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_SetCueLevel, 0x005AD290, int32_t, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_PanCueRotated, 0x005AD2D0, int32_t, int32_t, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_PanCueByAngle, 0x005AD310, int32_t, int32_t, int32_t, float);
GAME_FN(void, __cdecl, Xact_PanCue51, 0x005AD370, int32_t, int32_t, int32_t, float);
GAME_FN(void, __cdecl, Xact_PanCue51Speakers, 0x005AD3D0, int32_t, int32_t, float);
GAME_FN(void, __cdecl, Xact_SilenceCue, 0x005AD420, int32_t, int32_t);
GAME_FN(void, __cdecl, Xact_SetCueGains, 0x005AD460, int32_t, float, float, float, float, float, float);
GAME_FN(void, __cdecl, Xact_UpdateSoundBanksA, 0x005ADA90);
GAME_FN(void, __cdecl, Xact_UpdateSoundBanksB, 0x005ADB20);
GAME_FN(void, __cdecl, Xact_LoadStreamingWaveBank, 0x005AE730, int32_t handle, void* parameters);
GAME_FN(void, __cdecl, Xact_PrepareCueByName, 0x005AE810, int32_t, int32_t, const char*);
// this file's
GAME_FN(uint8_t, __cdecl, Xact_IsReverbForced, 0x005AC4E0);
GAME_FN(void, __cdecl, Xact_SetReverbType, 0x005AC590, int32_t, int32_t type);
GAME_FN(void, __cdecl, Xact_ForceReverb, 0x005AC5C0, int force);
GAME_FN(void, __thiscall, Map_RemoveAll, 0x005AC520, void* map);

// ---- the commands ------------------------------------------------------------------------------

// a simple command (0x2C bytes)
struct Command {
    re::Ptr32<void> vtable;        // +0
    int32_t unknown4;    // +4
    int32_t kind;        // +8
    uint32_t args[7];    // +0xC
    int32_t count;       // +0x28
};
static_assert(sizeof(Command) == 0x2C);

// 0x5ABEE0 (thiscall): the constructor
Command* __fastcall Command_Construct(Command* self, re::Edx, int32_t kind) {
    self->unknown4 = 0;
    self->vtable = reinterpret_cast<void*>(0x009228EC);
    self->kind = kind;
    for (uint32_t& arg : self->args) {
        arg = 0;
    }
    self->count = 0;
    return self;
}

// 0x5ABF10 (thiscall): a float argument
void __fastcall Command_AddFloat(Command* self, re::Edx, float value) {
    if (self->count < 7) {
        *reinterpret_cast<float*>(&self->args[self->count]) = value;
        self->count += 1;
    }
}

// 0x5ABF30 (thiscall): a byte argument (only its low byte written)
void __fastcall Command_AddByte(Command* self, re::Edx, int value) {
    if (self->count < 7) {
        *reinterpret_cast<uint8_t*>(&self->args[self->count]) = uint8_t(value);
        self->count += 1;
    }
}

// 0x5ABF50 (thiscall): an int argument
void __fastcall Command_AddInt(Command* self, re::Edx, uint32_t value) {
    if (self->count < 7) {
        self->args[self->count] = value;
        self->count += 1;
    }
}

inline float F(uint32_t bits) {
    return *reinterpret_cast<float*>(&bits);
}

// 0x5ABF70 (thiscall, vf1): runs the command
void __fastcall Command_Execute(Command* self) {
    const uint32_t* a = self->args;
    int32_t i0 = int32_t(a[0]);
    int32_t i1 = int32_t(a[1]);
    int32_t i2 = int32_t(a[2]);
    int32_t i3 = int32_t(a[3]);
    int32_t i4 = int32_t(a[4]);
    switch (self->kind) {
    case 0: Xact_LoadWaveBank(i0, i1, i2, i3, i4); return;
    case 1: Xact_UnloadWaveBank(i0); return;
    case 2: Xact_LoadSoundBank(i0, i1, i2, i3, i4); return;
    case 3: Xact_UnloadSoundBank(i0); return;
    case 4: Xact_PrepareCue(i0, i1, i2); return;
    case 5: Xact_StopCueOf(i0, i1, i2); return;
    case 6: Xact_DestroyCueOf(i0, i1); return;
    case 7: Xact_StopCueOf2(i0, i1, i2); return;
    case 8: Xact_PlayCue(i0, i1, i2); return;
    case 9: Xact_PauseCue(i0, i1); return;
    case 0xA: Xact_SetCueVariableB(i0, F(a[1])); return;
    case 0xB: Xact_SetCueVariableA(i0, F(a[1])); return;
    case 0xC: Xact_SetCueReverbMix(i0, F(a[1])); return;
    case 0xD: Xact_SetCueSpeakers(i0, i1); return;
    case 0xE: Xact_SetCueLevel(i0, i1, i2); return;
    case 0xF: Xact_PanCueRotated(i0, i1, i2, i3); return;
    case 0x10: Xact_PanCueByAngle(i0, i1, i2, F(a[3])); return;
    case 0x11: Xact_PanCue51(i0, i1, i2, F(a[3])); return;
    case 0x12: Xact_PanCue51Speakers(i0, i1, F(a[2])); return;
    case 0x13: Xact_SilenceCue(i0, i1); return;
    case 0x14: Xact_SetCueGains(i0, F(a[1]), F(a[2]), F(a[3]), F(a[4]), F(a[5]), F(a[6])); return;
    case 0x15: Xact_UpdateSoundBanksA(); return;
    case 0x16: Xact_UpdateSoundBanksB(); return;
    case 0x17: Xact_ForceReverb(0); return;
    default: return;
    }
}

// 0x5AC2C0 (thiscall): the streaming wave bank command (0x1C bytes: the handle, 16 bytes of
// parameters)
uint8_t* __fastcall StreamingWaveBankCommand_Construct(uint8_t* self, re::Edx, int32_t handle, const uint32_t* parameters) {
    At<int32_t>(self, 8) = handle;
    At<int32_t>(self, 4) = 0;
    At<uint32_t>(self, 0) = 0x009228F8;
    At<uint32_t>(self, 0xC) = parameters[0];
    At<uint32_t>(self, 0x10) = parameters[1];
    At<uint32_t>(self, 0x14) = parameters[2];
    At<uint32_t>(self, 0x18) = parameters[3];
    return self;
}

// 0x5AC300 (thiscall, vf1)
void __fastcall StreamingWaveBankCommand_Execute(uint8_t* self) {
    Xact_LoadStreamingWaveBank(At<int32_t>(self, 8), self + 0xC);
}

// 0x5AC320 (thiscall): the reverb command (0x10 bytes)
uint8_t* __fastcall ReverbCommand_Construct(uint8_t* self, re::Edx, int32_t a, int32_t type) {
    At<int32_t>(self, 4) = 0;
    At<uint32_t>(self, 0) = 0x00922904;
    At<int32_t>(self, 8) = a;
    At<int32_t>(self, 0xC) = type;
    return self;
}

// 0x5AC340 (thiscall, vf1): forces the reverb, then sets its type
void __fastcall ReverbCommand_Execute(uint8_t* self) {
    if (Xact_IsReverbForced() == 0) {
        Xact_ForceReverb(1);
    }
    Xact_SetReverbType(At<int32_t>(self, 8), At<int32_t>(self, 0xC));
}

// 0x5AC370 (thiscall, vf0): the base command's destructor
void* __fastcall ICommand_Destruct(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = 0x009228E0;
    if ((flags & 1) != 0) {
        Free(self);
    }
    return self;
}

// 0x5AC390 (thiscall, vf1): prepares a cue by name
void __fastcall PrepareByNameCommand_Execute(uint8_t* self) {
    Xact_PrepareCueByName(At<int32_t>(self, 8), At<int32_t>(self, 0xC), At<const char*>(self, 0x10));
}

// 0x5AC3B0 (thiscall): its constructor (0x14 bytes, the name a NiFixedString)
uint8_t* __fastcall PrepareByNameCommand_Construct(uint8_t* self, re::Edx, int32_t bank, int32_t cue, const char* name) {
    At<int32_t>(self, 4) = 0;
    At<uint32_t>(self, 0) = 0x00922910;
    At<const char*>(self, 0x10) = *emptyFixedString;
    At<int32_t>(self, 8) = bank;
    At<int32_t>(self, 0xC) = cue;
    NiFixedString_Set(&At<const char*>(self, 0x10), name);
    return self;
}

// a NiFixedString's release (its refcount 8 bytes before, the length 2 before)
void ReleaseFixedString(re::Ptr32<const char>* string) {
    const char* text = *string;
    if (text == *emptyFixedString) {
        return;
    }
    uint32_t length = *reinterpret_cast<const uint16_t*>(text - 2);
    using InterlockedFn = long(__stdcall*)(volatile long*);
    long left = re::GameFnAt<InterlockedFn>(uintptr_t(0x008FC170))(
        reinterpret_cast<volatile long*>(const_cast<char*>(text - 8)));
    if (left == 1) {
        NiStringTable_Free(*reinterpret_cast<re::Ptr32<void>*>(0x020E216C), string, length);
    }
}

// 0x5AC3F0 (thiscall, vf0): its destructor
void* __fastcall PrepareByNameCommand_Destruct(uint8_t* self, re::Edx, int flags) {
    ReleaseFixedString(&At<const char*>(self, 0x10));
    At<uint32_t>(self, 0) = 0x009228E0;
    if ((flags & 1) != 0) {
        Free(self);
    }
    return self;
}

// ---- the engine --------------------------------------------------------------------------------

using ComFn = long(__stdcall*)(void*);

// 0x5AC450: shuts XACT down
void __cdecl Xact_Shutdown() {
    Sounds_Fn596360(TheSounds);
    if (xactInitialised != nullptr) {
        xactInitialised = nullptr;
    }
    if (xactSettings != nullptr) {
        Free(xactSettings);
        xactSettings = nullptr;
    }
    uint8_t* engine = xactEngine;
    re::PtrCast<ComFn>(re::ComVtable(engine)[0x1C / 4])(engine);   // ShutDown
    engine = xactEngine;
    if (engine != nullptr) {
        re::PtrCast<ComFn>(re::ComVtable(engine)[2])(engine);   // Release
        xactEngine = nullptr;
    }
    using UninitializeFn = void(__stdcall*)();
    re::GameFnAt<UninitializeFn>(uintptr_t(0x008FC33C))();   // CoUninitialize
}

// 0x5AC4C0: runs a command and deletes it
void __cdecl Command_Run(void* command) {
    using ExecuteFn = void(__thiscall*)(void*);
    using DeleteFn = void*(__thiscall*)(void*, int);
    re::Ptr32<void>* vtable = *static_cast<re::Ptr32<re::Ptr32<void>>*>(command);
    re::PtrCast<ExecuteFn>(vtable[1])(command);
    re::PtrCast<DeleteFn>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(command))[0])(command, 1);
}

// 0x5AC4E0: the reverb is forced
uint8_t __cdecl Xact_IsReverbForcedImpl() {
    return xactReverbForced;
}

// 0x5AC4F0: the reverb type's level
float __cdecl Xact_ReverbLevel() {
    return xactReverbLevels[xactReverbType];
}

// 0x5AC500: the engine's work
void __cdecl Xact_DoWork() {
    uint8_t* engine = xactEngine;
    if (engine != nullptr) {
        re::PtrCast<ComFn>(re::ComVtable(engine)[0x20 / 4])(engine);
    }
}

// a map item {next, key, value}
struct MapItem {   // (the game's layout: 0xC)
    re::Ptr32<MapItem> next;
    uint32_t key;
    re::Ptr32<void> value;
};
static_assert(sizeof(MapItem) == 0xC);

// 0x5AC520 (thiscall, a NiTPointerMap): removes every item (vf2 clears, vf4 deletes)
void __fastcall Map_RemoveAllImpl(uint8_t* self) {
    using ItemFn = void(__thiscall*)(void*, MapItem*);
    for (uint32_t i = 0; i < At<uint32_t>(self, 4); ++i) {
        while (At<re::Ptr32<MapItem>*>(self, 8)[i] != nullptr) {
            MapItem* item = At<re::Ptr32<MapItem>*>(self, 8)[i];
            At<re::Ptr32<MapItem>*>(self, 8)[i] = item->next;
            re::PtrCast<ItemFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self))[2])(self, item);
            re::PtrCast<ItemFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self))[4])(self, item);
        }
    }
    At<uint32_t>(self, 0xC) = 0;
}

// 0x5AC590: the reverb type (0..30), when running
void __cdecl Xact_SetReverbTypeImpl(int32_t, int32_t type) {
    if (xactEngine != nullptr && xactInitialised != nullptr && uint32_t(type) <= 0x1E) {
        xactReverbType = type;
    }
}

// 0x5AC5C0: forces the reverb (or its type back to 0)
void __cdecl Xact_ForceReverbImpl(int force) {
    if (xactEngine == nullptr || xactInitialised == nullptr) {
        return;
    }
    if (uint8_t(force) != 0) {
        xactReverbForced = 1;
        return;
    }
    xactReverbType = 0;
}

// 0x5AC5F0 (thiscall, a NiTPointerMap): the value of a key
uint8_t __fastcall Map_GetAt(uint8_t* self, re::Edx, uint32_t key, re::Ptr32<void>* value) {
    for (MapItem* item = At<re::Ptr32<MapItem>*>(self, 8)[key % At<uint32_t>(self, 4)]; item != nullptr; item = item->next) {
        if (item->key == key) {
            *value = item->value;
            return 1;
        }
    }
    return 0;
}

// ---- the NiTPointerMap instantiations -----------------------------------------------------------

// the base (NiTMapBase) destructor: RemoveAll, the buckets, the object (0x10 bytes)
void* MapBaseDestruct(uint8_t* self, int flags, uintptr_t vtable) {
    At<uint32_t>(self, 0) = vtable;
    Map_RemoveAll(self);
    NiFree(At<void*>(self, 8));
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}
// 0x5AC630.. (thiscall, vf0): the nine bases' destructors
void* __fastcall MapBase_Destruct0(uint8_t* self, re::Edx, int flags) { return MapBaseDestruct(self, flags, 0x00922A48); }
void* __fastcall MapBase_Destruct1(uint8_t* self, re::Edx, int flags) { return MapBaseDestruct(self, flags, 0x00922A60); }
void* __fastcall MapBase_Destruct2(uint8_t* self, re::Edx, int flags) { return MapBaseDestruct(self, flags, 0x00922A78); }
void* __fastcall MapBase_Destruct3(uint8_t* self, re::Edx, int flags) { return MapBaseDestruct(self, flags, 0x00922A90); }
void* __fastcall MapBase_Destruct4(uint8_t* self, re::Edx, int flags) { return MapBaseDestruct(self, flags, 0x00922AA8); }
void* __fastcall MapBase_Destruct5(uint8_t* self, re::Edx, int flags) { return MapBaseDestruct(self, flags, 0x00922AC0); }
void* __fastcall MapBase_Destruct6(uint8_t* self, re::Edx, int flags) { return MapBaseDestruct(self, flags, 0x00922AD8); }
void* __fastcall MapBase_Destruct7(uint8_t* self, re::Edx, int flags) { return MapBaseDestruct(self, flags, 0x00922AF0); }
void* __fastcall MapBase_Destruct8(uint8_t* self, re::Edx, int flags) { return MapBaseDestruct(self, flags, 0x00922B08); }

// the constructor: the base's vtable, the zeroed buckets, then the map's vtable
uint8_t* MapConstruct(uint8_t* self, uint32_t size, uintptr_t baseVtable, uintptr_t vtable) {
    At<uint32_t>(self, 4) = size;
    At<uint32_t>(self, 0) = baseVtable;
    At<uint32_t>(self, 0xC) = 0;
    void* buckets = NiMalloc(size * 4);
    At<void*>(self, 8) = buckets;
    Memset(buckets, 0, At<uint32_t>(self, 4) * 4);
    At<uint32_t>(self, 0) = vtable;
    return self;
}
// 0x5ADD70.. (thiscall)
uint8_t* __fastcall Map_Construct0(uint8_t* self, re::Edx, uint32_t size) { return MapConstruct(self, size, 0x00922A48, 0x00922B20); }
uint8_t* __fastcall Map_Construct1(uint8_t* self, re::Edx, uint32_t size) { return MapConstruct(self, size, 0x00922A60, 0x00922B38); }
uint8_t* __fastcall Map_Construct2(uint8_t* self, re::Edx, uint32_t size) { return MapConstruct(self, size, 0x00922A78, 0x00922B50); }
uint8_t* __fastcall Map_Construct3(uint8_t* self, re::Edx, uint32_t size) { return MapConstruct(self, size, 0x00922A90, 0x00922B68); }
uint8_t* __fastcall Map_Construct4(uint8_t* self, re::Edx, uint32_t size) { return MapConstruct(self, size, 0x00922AA8, 0x00922B80); }
uint8_t* __fastcall Map_Construct5(uint8_t* self, re::Edx, uint32_t size) { return MapConstruct(self, size, 0x00922AC0, 0x00922B98); }
uint8_t* __fastcall Map_Construct6(uint8_t* self, re::Edx, uint32_t size) { return MapConstruct(self, size, 0x00922AD8, 0x00922BB0); }
uint8_t* __fastcall Map_Construct7(uint8_t* self, re::Edx, uint32_t size) { return MapConstruct(self, size, 0x00922AF0, 0x00922BC8); }
uint8_t* __fastcall Map_Construct8(uint8_t* self, re::Edx, uint32_t size) { return MapConstruct(self, size, 0x00922B08, 0x00922BE0); }

// 0x5ADF50 (thiscall, vf3): a new item (its value cleared)
MapItem* __fastcall Map_NewItem(void*) {
    MapItem* item = static_cast<MapItem*>(NiNew(0xC));
    item->value = nullptr;
    return item;
}

// the destructor: RemoveAll as the map and as the base, the buckets, the object
void* MapDestruct(uint8_t* self, int flags, uintptr_t vtable, uintptr_t baseVtable) {
    At<uint32_t>(self, 0) = vtable;
    Map_RemoveAll(self);
    At<uint32_t>(self, 0) = baseVtable;
    Map_RemoveAll(self);
    NiFree(At<void*>(self, 8));
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}
// 0x5AE060.. (thiscall, vf0)
void* __fastcall Map_Destruct0(uint8_t* self, re::Edx, int flags) { return MapDestruct(self, flags, 0x00922B20, 0x00922A48); }
void* __fastcall Map_Destruct1(uint8_t* self, re::Edx, int flags) { return MapDestruct(self, flags, 0x00922B38, 0x00922A60); }
void* __fastcall Map_Destruct2(uint8_t* self, re::Edx, int flags) { return MapDestruct(self, flags, 0x00922B50, 0x00922A78); }
void* __fastcall Map_Destruct3(uint8_t* self, re::Edx, int flags) { return MapDestruct(self, flags, 0x00922B68, 0x00922A90); }
void* __fastcall Map_Destruct4(uint8_t* self, re::Edx, int flags) { return MapDestruct(self, flags, 0x00922B80, 0x00922AA8); }
void* __fastcall Map_Destruct5(uint8_t* self, re::Edx, int flags) { return MapDestruct(self, flags, 0x00922B98, 0x00922AC0); }
void* __fastcall Map_Destruct6(uint8_t* self, re::Edx, int flags) { return MapDestruct(self, flags, 0x00922BB0, 0x00922AD8); }
void* __fastcall Map_Destruct7(uint8_t* self, re::Edx, int flags) { return MapDestruct(self, flags, 0x00922BC8, 0x00922AF0); }
void* __fastcall Map_Destruct8(uint8_t* self, re::Edx, int flags) { return MapDestruct(self, flags, 0x00922BE0, 0x00922B08); }

// ---- registration ---------------------------------------------------------------------------

#define MDXCMD_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(mdxcmd_original_, __LINE__);                                          \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(mdxcmd_original_, __LINE__), \
             re::Mode::Original)

MDXCMD_REVERSED("MDXactSimpleCommand::MDXactSimpleCommand", 0x005ABEE0, &Command_Construct);
MDXCMD_REVERSED("MDXactSimpleCommand::AddFloat", 0x005ABF10, &Command_AddFloat);
MDXCMD_REVERSED("MDXactSimpleCommand::AddByte", 0x005ABF30, &Command_AddByte);
MDXCMD_REVERSED("MDXactSimpleCommand::AddInt", 0x005ABF50, &Command_AddInt);
MDXCMD_REVERSED("MDXactSimpleCommand::Execute", 0x005ABF70, &Command_Execute);
MDXCMD_REVERSED("MDXactCreateStreamingWavebankCommand::Construct", 0x005AC2C0, &StreamingWaveBankCommand_Construct);
MDXCMD_REVERSED("MDXactCreateStreamingWavebankCommand::Execute", 0x005AC300, &StreamingWaveBankCommand_Execute);
MDXCMD_REVERSED("MDXactSetReverbTypeCommand::Construct", 0x005AC320, &ReverbCommand_Construct);
MDXCMD_REVERSED("MDXactSetReverbTypeCommand::Execute", 0x005AC340, &ReverbCommand_Execute);
MDXCMD_REVERSED("IMDXactCommand::~IMDXactCommand", 0x005AC370, &ICommand_Destruct);
MDXCMD_REVERSED("MDXactPrepareSoundCueByNameCommand::Execute", 0x005AC390, &PrepareByNameCommand_Execute);
MDXCMD_REVERSED("MDXactPrepareSoundCueByNameCommand::Construct", 0x005AC3B0, &PrepareByNameCommand_Construct);
MDXCMD_REVERSED("MDXactPrepareSoundCueByNameCommand::~Destruct", 0x005AC3F0, &PrepareByNameCommand_Destruct);
MDXCMD_REVERSED("MDXact::Shutdown", 0x005AC450, &Xact_Shutdown);
MDXCMD_REVERSED("MDXact::RunCommand", 0x005AC4C0, &Command_Run);
MDXCMD_REVERSED("MDXact::IsReverbForced", 0x005AC4E0, &Xact_IsReverbForcedImpl);
MDXCMD_REVERSED("MDXact::ReverbLevel", 0x005AC4F0, &Xact_ReverbLevel);
MDXCMD_REVERSED("MDXact::DoWork", 0x005AC500, &Xact_DoWork);
MDXCMD_REVERSED("MDXactMap::RemoveAll", 0x005AC520, &Map_RemoveAllImpl);
MDXCMD_REVERSED("MDXact::SetReverbType", 0x005AC590, &Xact_SetReverbTypeImpl);
MDXCMD_REVERSED("MDXact::ForceReverb", 0x005AC5C0, &Xact_ForceReverbImpl);
MDXCMD_REVERSED("MDXactMap::GetAt", 0x005AC5F0, &Map_GetAt);
MDXCMD_REVERSED("MDXactMapBase0::~Destruct", 0x005AC630, &MapBase_Destruct0);
MDXCMD_REVERSED("MDXactMapBase1::~Destruct", 0x005AC670, &MapBase_Destruct1);
MDXCMD_REVERSED("MDXactMapBase2::~Destruct", 0x005AC6B0, &MapBase_Destruct2);
MDXCMD_REVERSED("MDXactMapBase3::~Destruct", 0x005AC6F0, &MapBase_Destruct3);
MDXCMD_REVERSED("MDXactMapBase4::~Destruct", 0x005AC730, &MapBase_Destruct4);
MDXCMD_REVERSED("MDXactMapBase5::~Destruct", 0x005AC770, &MapBase_Destruct5);
MDXCMD_REVERSED("MDXactMapBase6::~Destruct", 0x005AC7B0, &MapBase_Destruct6);
MDXCMD_REVERSED("MDXactMapBase7::~Destruct", 0x005AC7F0, &MapBase_Destruct7);
MDXCMD_REVERSED("MDXactMapBase8::~Destruct", 0x005AC830, &MapBase_Destruct8);
MDXCMD_REVERSED("MDXactMap0::Construct", 0x005ADD70, &Map_Construct0);
MDXCMD_REVERSED("MDXactMap1::Construct", 0x005ADDC0, &Map_Construct1);
MDXCMD_REVERSED("MDXactMap2::Construct", 0x005ADE10, &Map_Construct2);
MDXCMD_REVERSED("MDXactMap3::Construct", 0x005ADE60, &Map_Construct3);
MDXCMD_REVERSED("MDXactMap4::Construct", 0x005ADEB0, &Map_Construct4);
MDXCMD_REVERSED("MDXactMap5::Construct", 0x005ADF00, &Map_Construct5);
MDXCMD_REVERSED("MDXactMap::NewItem", 0x005ADF50, &Map_NewItem);
MDXCMD_REVERSED("MDXactMap6::Construct", 0x005ADF70, &Map_Construct6);
MDXCMD_REVERSED("MDXactMap7::Construct", 0x005ADFC0, &Map_Construct7);
MDXCMD_REVERSED("MDXactMap8::Construct", 0x005AE010, &Map_Construct8);
MDXCMD_REVERSED("MDXactMap0::~Destruct", 0x005AE060, &Map_Destruct0);
MDXCMD_REVERSED("MDXactMap1::~Destruct", 0x005AE0A0, &Map_Destruct1);
MDXCMD_REVERSED("MDXactMap2::~Destruct", 0x005AE0E0, &Map_Destruct2);
MDXCMD_REVERSED("MDXactMap3::~Destruct", 0x005AE120, &Map_Destruct3);
MDXCMD_REVERSED("MDXactMap4::~Destruct", 0x005AE160, &Map_Destruct4);
MDXCMD_REVERSED("MDXactMap5::~Destruct", 0x005AE1A0, &Map_Destruct5);
MDXCMD_REVERSED("MDXactMap6::~Destruct", 0x005AE1E0, &Map_Destruct6);
MDXCMD_REVERSED("MDXactMap7::~Destruct", 0x005AE220, &Map_Destruct7);
MDXCMD_REVERSED("MDXactMap8::~Destruct", 0x005AE260, &Map_Destruct8);

} // namespace
