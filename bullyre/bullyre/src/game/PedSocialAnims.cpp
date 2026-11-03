// A ped's social exchanges (0x48D560 .. 0x48E2B0): the kinds of the two sides' moves
// (+0xE0, +0xE4, +0xE8), the peds involved (+4 the ped, +0x6C / +0x70 the other), and
// the humiliation, authority and kiss anims (+0x200 / +0x204 / +0x208, -1 none) streamed
// in and blended on the ped. Also the five buttons the player answers with.

#include <cstdint>
#include <initializer_list>

#include "reversed.h"

namespace {

using re::At;

// the anim names: humiliations (0x20 each), authority (0x10 each), kisses (pointers)
inline const char* const PSA_humiliations = reinterpret_cast<const char*>(0x009113D0);   // "HUMIL_5-8_A", ...
inline const char* const PSA_authority = reinterpret_cast<const char*>(0x00911690);      // "SAUTH_A", ...
inline const re::Ptr32<const char>* const PSA_kisses = reinterpret_cast<const re::Ptr32<const char>*>(0x009121D4);   // "KISS1", ...
inline void* const PSA_input = reinterpret_cast<void*>(0x020C3CA0);
inline void* const PSA_animStore = reinterpret_cast<void*>(0x020C4910);
inline void* const PSA_actFiles = reinterpret_cast<void*>(0x00D02850);
GAME_VAR(uint16_t, PSA_nextClassId, 0x00BF3830);
GAME_VAR(uint32_t, PSA_classIdMade, 0x00C2C03C);
GAME_VAR(uint16_t, PSA_classId, 0x00C2C038);
GAME_VAR(void*, PSA_stimuli, 0x00C2C1A0);

GAME_FN(int, __thiscall, PSA_PedGetStat, 0x00477060, void* ped, int stat);
GAME_FN(uint8_t, __thiscall, PSA_HandleIsValid, 0x005D80E0, void* handle);
GAME_FN(void*, __thiscall, PSA_HandleGetFile, 0x005D80B0, void* handle);
GAME_FN(void*, __thiscall, PSA_HandleGetNode, 0x005D9C20, void* handle);
GAME_FN(void*, __thiscall, PSA_Handle5D80D0, 0x005D80D0, void* handle);
GAME_FN(void*, __thiscall, PSA_ActFilesCurrent, 0x005DB960, void* files);
GAME_FN(void*, __thiscall, PSA_ActFilesFind, 0x005DB8F0, void* files, void* file);
GAME_FN(void, __thiscall, PSA_ActFilesSetCurrent, 0x005DBDA0, void* files, void* file);
GAME_FN(void, __cdecl, PSA_Node5D8270, 0x005D8270, void* node);
GAME_FN(void, __cdecl, PSA_Node5D8670, 0x005D8670, void* node, void*);
GAME_FN(void, __cdecl, PSA_Node5D85C0, 0x005D85C0, void* node, void*);
GAME_FN(void, __cdecl, PSA_Node5D82A0, 0x005D82A0, void* node, int, int);
GAME_FN(void*, __thiscall, PSA_StimulusOwner, 0x004A5CD0, void* stimulus);
GAME_FN(void*, __thiscall, PSA_StimulusOther, 0x004A5CB0, void* stimulus);
GAME_FN(void*, __thiscall, PSA_StimuliFind, 0x004A6150, void* stimuli, void* ped, int type, int);
// the anims: the entry by name (its id out), the streaming slot, its info, loaded
GAME_FN(uint8_t*, __thiscall, PSA_AnimFind, 0x0042E350, void* anims, const char* name, uint32_t* id);
GAME_FN(int, __cdecl, PSA_AnimStreamId, 0x0052DBD0, void* anims, uint32_t id);
GAME_FN(uint8_t*, __cdecl, PSA_StreamingInfo, 0x0052DD30, int slot);
GAME_FN(uint8_t, __cdecl, PSA_Streaming450C00, 0x00450C00, int slot);
GAME_FN(uint8_t, __cdecl, PSA_Streaming4151C0, 0x004151C0, int slot);
GAME_FN(void, __cdecl, PSA_RequestModel, 0x00532020, int slot, int flags);
GAME_FN(void, __thiscall, PSA_AnimStore6BD010, 0x006BD010, void* store, void* anims, uint32_t id);
GAME_FN(void, __thiscall, PSA_Anims6B56B0, 0x006B56B0, void* anims, uint32_t id, int);
GAME_FN(void, __thiscall, PSA_PedAddBlend, 0x004739E0, void* ped, uint32_t id);
GAME_FN(void, __thiscall, PSA_PedRemoveBlend, 0x00473A50, void* ped, uint32_t id);
GAME_FN(uint8_t*, __thiscall, PSA_PedGetModelInfo, 0x00477DC0, void* ped);
GAME_FN(int*, __thiscall, PSA_ModelInfo522380, 0x00522380, void* info, void* out);
// the input: a control by name, its state, the two devices and theirs
GAME_FN(int, __thiscall, PSA_InputFindControl, 0x006A9FD0, void* input, void* name);
GAME_FN(uint8_t*, __thiscall, PSA_InputGetControl, 0x006AA7C0, void* input, int id);
GAME_FN(void*, __thiscall, PSA_InputDevice6AA680, 0x006AA680, void* input);
GAME_FN(void*, __thiscall, PSA_InputDevice6AA690, 0x006AA690, void* input);
GAME_FN(uint8_t, __thiscall, PSA_DeviceActive, 0x006AA3F0, void* device);
GAME_FN(uint8_t, __thiscall, PSA_DevicePressed, 0x006AA410, void* device, int id);

// The ped's anim hierarchy (CPed +0x114, +0x94, +0x10)
void* Anims(uint8_t* self) {
    return At<void*>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 4), 0x114), 0x94), 0x10);
}

// An anim of the ped's blended in when it's loaded, else requested
void BlendOrRequest(uint8_t* self, const char* name) {
    void* anims = Anims(self);
    uint32_t id = 0;
    PSA_AnimFind(anims, name, &id);
    int slot = PSA_AnimStreamId(anims, id);
    if (PSA_StreamingInfo(slot)[4] == 1) {
        PSA_PedAddBlend(At<void*>(self, 4), id);
        return;
    }
    if (PSA_StreamingInfo(slot)[4] != 2 && !PSA_Streaming450C00(slot)) {
        PSA_RequestModel(slot, 4);
        PSA_AnimStore6BD010(PSA_animStore, anims, id);
        PSA_Anims6B56B0(anims, id, 0x14);
    }
}

// An anim chosen (+offset remembered): blended when it's a blend-only one (+0x34), else
// requested when not loaded
void Choose(uint8_t* self, uint32_t offset, int index, const char* name) {
    uint8_t* anim = At<uint8_t*>(At<uint8_t*>(self, 4), 0x114);
    if (!anim) {
        return;
    }
    void* anims = At<void*>(At<uint8_t*>(anim, 0x94), 0x10);
    if (!anims) {
        return;
    }
    uint32_t id = static_cast<uint32_t>(index);   // (the game's id slot is the argument's)
    uint8_t* entry = PSA_AnimFind(anims, name, &id);
    if (!entry) {
        return;
    }
    At<int32_t>(self, offset) = index;
    if (entry[0x34]) {
        PSA_PedAddBlend(At<void*>(self, 4), id);
        return;
    }
    int slot = PSA_AnimStreamId(anims, id);
    if (PSA_StreamingInfo(slot)[4] != 1 && !PSA_Streaming4151C0(slot) && !PSA_Streaming450C00(slot)) {
        PSA_RequestModel(slot, 0);
        PSA_AnimStore6BD010(PSA_animStore, anims, id);
        PSA_Anims6B56B0(anims, id, 0x14);
    }
}

// The chosen anim dropped (its blend removed when loaded)
void Drop(uint8_t* self, uint32_t offset, const char* name) {
    uint8_t* anim = At<uint8_t*>(At<uint8_t*>(self, 4), 0x114);
    if (anim) {
        void* anims = At<void*>(At<uint8_t*>(anim, 0x94), 0x10);
        // (the game's id slot starts out holding `self`)
        uint32_t id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self));
        if (anims && PSA_AnimFind(anims, name, &id)) {
            int slot = PSA_AnimStreamId(anims, id);
            if (PSA_StreamingInfo(slot)[4] == 1 && !PSA_Streaming4151C0(slot) && !PSA_Streaming450C00(slot)) {
                PSA_PedRemoveBlend(At<void*>(self, 4), id);
            }
        }
    }
    At<int32_t>(self, offset) = -1;
}

// A button pressed on either device (or held: the control's count, flag 8)
uint8_t ButtonPressed(int32_t& control, void* name) {
    if (control == -1) {
        control = PSA_InputFindControl(PSA_input, name);
    }
    if (At<uint16_t>(PSA_InputGetControl(PSA_input, control), 2) > 0) {
        return 1;
    }
    if (PSA_InputGetControl(PSA_input, control)[4] & 8) {
        return 1;
    }
    if (PSA_DeviceActive(PSA_InputDevice6AA680(PSA_input)) && PSA_DevicePressed(PSA_InputDevice6AA680(PSA_input), control)) {
        return 1;
    }
    if (PSA_DeviceActive(PSA_InputDevice6AA690(PSA_input)) && PSA_DevicePressed(PSA_InputDevice6AA690(PSA_input), control)) {
        return 1;
    }
    return 0;
}

GAME_VAR(int32_t, PSA_button1, 0x00A33A40);
GAME_VAR(int32_t, PSA_button2, 0x00A33A44);
GAME_VAR(int32_t, PSA_button3, 0x00A33A48);
GAME_VAR(int32_t, PSA_button4, 0x00A33A4C);
GAME_VAR(int32_t, PSA_button5, 0x00A33A50);

bool OneOf(int value, std::initializer_list<int> values) {
    for (int v : values) {
        if (value == v) {
            return true;
        }
    }
    return false;
}

} // namespace

namespace c07psa {

// 0x48D560: the other's move (+0xE4) is 0, 6 or 0x15
uint32_t __fastcall OtherMoveIs0615(uint8_t* self, re::Edx) {
    return OneOf(At<int32_t>(self, 0xE4), {0, 6, 0x15}) ? 1 : 0;
}

// 0x48D580: +0xE4 is 0, 1, 6, 0x15, 8, 7 or 0x11
uint32_t __fastcall OtherMoveIsFriendly(uint8_t* self, re::Edx) {
    return OneOf(At<int32_t>(self, 0xE4), {0, 1, 6, 0x15, 8, 7, 0x11}) ? 1 : 0;
}

// 0x48D5C0: +0xE4 is 1, 3, 8, 9 or 7
uint32_t __fastcall OtherMoveIsA(uint8_t* self, re::Edx) {
    return OneOf(At<int32_t>(self, 0xE4), {1, 3, 8, 9, 7}) ? 1 : 0;
}

// 0x48D5F0: +0xE8 is 0, 1, 6, 0x15, 8 or 7
uint32_t __fastcall MoveIsFriendly(uint8_t* self, re::Edx) {
    return OneOf(At<int32_t>(self, 0xE8), {0, 1, 6, 0x15, 8, 7}) ? 1 : 0;
}

// 0x48D620: +0xE8 is 1, 3, 8, 9 or 7
uint32_t __fastcall MoveIsA(uint8_t* self, re::Edx) {
    return OneOf(At<int32_t>(self, 0xE8), {1, 3, 8, 9, 7}) ? 1 : 0;
}

// 0x48D650: +0xE0 is 3, 9, 0xE, 0xF, 0x10, 0x12, 0x17 or 0x18
uint32_t __fastcall StateIsHostile(uint8_t* self, re::Edx) {
    return OneOf(At<int32_t>(self, 0xE0), {3, 9, 0xE, 0xF, 0x10, 0x12, 0x17, 0x18}) ? 1 : 0;
}

// 0x48D690
uint32_t __fastcall Is120(uint8_t* self, re::Edx, void* entity) {
    return (entity && entity == At<void*>(self, 0x120)) ? 1 : 0;
}

// 0x48D6B0
uint32_t __fastcall Is124(uint8_t* self, re::Edx, void* entity) {
    return (entity && entity == At<void*>(self, 0x124)) ? 1 : 0;
}

// 0x48D6D0
uint32_t __fastcall Is128(uint8_t* self, re::Edx, void* entity) {
    return (entity && entity == At<void*>(self, 0x128)) ? 1 : 0;
}

// 0x48D6F0: both sides there
uint32_t __fastcall HasBothSides(uint8_t* self, re::Edx) {
    return (At<void*>(self, 0x6C) && At<void*>(self, 0x70)) ? 1 : 0;
}

// 0x48D710 (CPed): its +0x1D30 cleared when it's `value`
void __fastcall PedClear1D30(uint8_t* self, re::Edx, void* value) {
    if (At<void*>(self, 0x1D30) == value) {
        At<void*>(self, 0x1D30) = nullptr;
    }
}

// 0x48D730 (static): the class id, assigned on first use
uint16_t __cdecl GetClassId() {
    if (!(PSA_classIdMade & 1)) {
        uint16_t id = PSA_nextClassId;
        PSA_classIdMade |= 1;
        PSA_classId = id;
        PSA_nextClassId = static_cast<uint16_t>(id + 1);
        return id;
    }
    return PSA_classId;
}

// 0x48D770: a score: 10 x `base`, plus the ped's stats 0xE and 6 less the other's
int __fastcall Score(uint8_t* self, re::Edx, int base) {
    int own14 = PSA_PedGetStat(At<void*>(self, 4), 0xE);
    int own6 = PSA_PedGetStat(At<void*>(self, 4), 6);
    int other14 = 0;
    int other6 = 0;
    if (At<void*>(self, 0x70)) {
        other14 = PSA_PedGetStat(At<void*>(self, 0x70), 0xE);
        other6 = PSA_PedGetStat(At<void*>(self, 0x70), 6);
    }
    return base * 10 - other14 - own6 + other6 + own14;
}

// 0x48D7F0: the ped's current node played again in its act file (with +0x141C and +0x1420
// 1)
void __fastcall Reapply(uint8_t* self, re::Edx) {
    uint8_t* ped = At<uint8_t*>(self, 4);
    if (At<int32_t>(ped, 0x141C) == 0 || !PSA_HandleIsValid(ped + 0x18E8) || At<int32_t>(At<uint8_t*>(self, 4), 0x1420) != 1) {
        return;
    }
    void* saved = PSA_ActFilesCurrent(PSA_actFiles);
    uint8_t* handle = At<uint8_t*>(self, 4) + 0x18E8;
    void* file = PSA_ActFilesFind(PSA_actFiles, PSA_HandleGetFile(handle));
    if (!file) {
        return;
    }
    PSA_ActFilesSetCurrent(PSA_actFiles, file);
    void* node = PSA_HandleGetNode(handle);
    PSA_Node5D8270(node);
    PSA_Node5D8670(node, PSA_Handle5D80D0(handle));
    uint8_t* owner = At<uint8_t*>(self, 4);
    void* value = reinterpret_cast<void*(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(owner))[0xAC / 4]))(owner);
    PSA_Node5D85C0(node, value);
    PSA_Node5D82A0(node, 1, 0);
    PSA_ActFilesSetCurrent(PSA_actFiles, saved);
}

// 0x48D8C0: the stimulus of a type between the ped and the other (`cached` first; the
// found one stored back)
void* __fastcall FindStimulus(uint8_t* self, re::Edx, int type, re::Ptr32<int>* cached) {
    if (cached && *cached && **cached == type && PSA_StimulusOwner(*cached) == At<void*>(self, 4)) {
        void* other = At<void*>(self, 0x70);
        if (PSA_StimulusOther(*cached) == other) {
            return *cached;
        }
    }
    void* other = At<void*>(self, 0x70);
    if (other) {
        void* found = PSA_StimuliFind(PSA_stimuli, other, type, -1);
        if (found && PSA_StimulusOwner(found) == At<void*>(self, 4)) {
            if (cached) {
                *cached = static_cast<int*>(found);
            }
            return found;
        }
    }
    return nullptr;
}

// 0x48D940: the humiliation anim blended in (requested when not loaded)
void __fastcall BlendHumiliation(uint8_t* self, re::Edx) {
    int index = At<int32_t>(self, 0x200);
    if (index != -1) {
        BlendOrRequest(self, PSA_humiliations + index * 0x20);
    }
}

// 0x48DA00: the authority anim blended in
void __fastcall BlendAuthority(uint8_t* self, re::Edx) {
    int index = At<int32_t>(self, 0x204);
    if (index != -1) {
        BlendOrRequest(self, PSA_authority + index * 0x10);
    }
}

// 0x48DAC0: the kiss anim blended in
void __fastcall BlendKiss(uint8_t* self, re::Edx) {
    int index = At<int32_t>(self, 0x208);
    if (index != -1) {
        BlendOrRequest(self, PSA_kisses[index]);
    }
}

// 0x48DB80: an authority anim chosen
void __fastcall ChooseAuthority(uint8_t* self, re::Edx, int index) {
    Choose(self, 0x204, index, PSA_authority + index * 0x10);
}

// 0x48DC50: a kiss anim chosen
void __fastcall ChooseKiss(uint8_t* self, re::Edx, int index) {
    Choose(self, 0x208, index, PSA_kisses[index]);
}

// 0x48DD10: the authority anim dropped
void __fastcall DropAuthority(uint8_t* self, re::Edx) {
    int index = At<int32_t>(self, 0x204);
    if (index != -1) {
        Drop(self, 0x204, PSA_authority + index * 0x10);
    }
}

// 0x48DDB0: the humiliation anim dropped
void __fastcall DropHumiliation(uint8_t* self, re::Edx) {
    int index = At<int32_t>(self, 0x200);
    if (index != -1) {
        Drop(self, 0x200, PSA_humiliations + index * 0x20);
    }
}

// 0x48DE50: the kiss anim dropped
void __fastcall DropKiss(uint8_t* self, re::Edx) {
    int index = At<int32_t>(self, 0x208);
    if (index != -1) {
        Drop(self, 0x208, PSA_kisses[index]);
    }
}

// 0x48DEF0 .. 0x48E1F0 (static): the five answer buttons
uint32_t __cdecl Button1() {
    return ButtonPressed(PSA_button1, reinterpret_cast<void*>(0x01B65454));
}
uint32_t __cdecl Button2() {
    return ButtonPressed(PSA_button2, reinterpret_cast<void*>(0x01B65458));
}
uint32_t __cdecl Button3() {
    return ButtonPressed(PSA_button3, reinterpret_cast<void*>(0x01B6545C));
}
uint32_t __cdecl Button4() {
    return ButtonPressed(PSA_button4, reinterpret_cast<void*>(0x01B65460));
}
uint32_t __cdecl Button5() {
    return ButtonPressed(PSA_button5, reinterpret_cast<void*>(0x01B65464));
}

// 0x48E2B0: the ped's model info has no +0x50 and its 0x522380 is set
uint32_t __fastcall HasModel522380(uint8_t* self, re::Edx) {
    if (At<uint8_t>(PSA_PedGetModelInfo(At<void*>(self, 4)), 0x50)) {
        return 0;
    }
    uint32_t out;
    return *PSA_ModelInfo522380(PSA_PedGetModelInfo(At<void*>(self, 4)), &out) != 0 ? 1 : 0;
}

} // namespace c07psa

#define PSA_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PSA_REVERSED("PedSocial::OtherMoveIs0615", 0x0048D560, &c07psa::OtherMoveIs0615);
PSA_REVERSED("PedSocial::OtherMoveIsFriendly", 0x0048D580, &c07psa::OtherMoveIsFriendly);
PSA_REVERSED("PedSocial::OtherMoveIsA", 0x0048D5C0, &c07psa::OtherMoveIsA);
PSA_REVERSED("PedSocial::MoveIsFriendly", 0x0048D5F0, &c07psa::MoveIsFriendly);
PSA_REVERSED("PedSocial::MoveIsA", 0x0048D620, &c07psa::MoveIsA);
PSA_REVERSED("PedSocial::StateIsHostile", 0x0048D650, &c07psa::StateIsHostile);
PSA_REVERSED("PedSocial::Is120", 0x0048D690, &c07psa::Is120);
PSA_REVERSED("PedSocial::Is124", 0x0048D6B0, &c07psa::Is124);
PSA_REVERSED("PedSocial::Is128", 0x0048D6D0, &c07psa::Is128);
PSA_REVERSED("PedSocial::HasBothSides", 0x0048D6F0, &c07psa::HasBothSides);
PSA_REVERSED("CPed::Clear1D30", 0x0048D710, &c07psa::PedClear1D30);
PSA_REVERSED("PedSocial::GetClassId", 0x0048D730, &c07psa::GetClassId);
PSA_REVERSED("PedSocial::Score", 0x0048D770, &c07psa::Score);
PSA_REVERSED("PedSocial::Reapply", 0x0048D7F0, &c07psa::Reapply);
PSA_REVERSED("PedSocial::FindStimulus", 0x0048D8C0, &c07psa::FindStimulus);
PSA_REVERSED("PedSocial::BlendHumiliation", 0x0048D940, &c07psa::BlendHumiliation);
PSA_REVERSED("PedSocial::BlendAuthority", 0x0048DA00, &c07psa::BlendAuthority);
PSA_REVERSED("PedSocial::BlendKiss", 0x0048DAC0, &c07psa::BlendKiss);
PSA_REVERSED("PedSocial::ChooseAuthority", 0x0048DB80, &c07psa::ChooseAuthority);
PSA_REVERSED("PedSocial::ChooseKiss", 0x0048DC50, &c07psa::ChooseKiss);
PSA_REVERSED("PedSocial::DropAuthority", 0x0048DD10, &c07psa::DropAuthority);
PSA_REVERSED("PedSocial::DropHumiliation", 0x0048DDB0, &c07psa::DropHumiliation);
PSA_REVERSED("PedSocial::DropKiss", 0x0048DE50, &c07psa::DropKiss);
PSA_REVERSED("PedSocial::Button1", 0x0048DEF0, &c07psa::Button1);
PSA_REVERSED("PedSocial::Button2", 0x0048DFB0, &c07psa::Button2);
PSA_REVERSED("PedSocial::Button3", 0x0048E070, &c07psa::Button3);
PSA_REVERSED("PedSocial::Button4", 0x0048E130, &c07psa::Button4);
PSA_REVERSED("PedSocial::Button5", 0x0048E1F0, &c07psa::Button5);
PSA_REVERSED("PedSocial::HasModel522380", 0x0048E2B0, &c07psa::HasModel522380);
