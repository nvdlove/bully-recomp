// A ped's social exchanges: setting a mood, playing the answers, and how the ped answers
// the player (0x48F1D0 .. 0x490488). See PedSocialMood.cpp for the object.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

inline const int32_t* const PSR_moods = reinterpret_cast<const int32_t*>(0x00A349E0);        // [11][11], a and b from -5
inline const int32_t* const PSR_answers = reinterpret_cast<const int32_t*>(0x00A34BC8);      // [mood][10]
inline const char* const PSR_humiliations = reinterpret_cast<const char*>(0x009113D0);
inline void* const PSR_input = reinterpret_cast<void*>(0x020C3CA0);
inline void* const PSR_actFiles = reinterpret_cast<void*>(0x00D02850);
inline void* const PSR_nodeA = reinterpret_cast<void*>(0x01B65480);
inline void* const PSR_nodeB = reinterpret_cast<void*>(0x01B6544C);
GAME_VAR(uint32_t, PSR_timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t*, PSR_playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, PSR_specialModel1, 0x00BF3F20);
GAME_VAR(int32_t, PSR_specialModel2, 0x00BF3F1C);
GAME_VAR(uint8_t, PSR_kissFlags, 0x020C5F7B);
GAME_VAR(int32_t, PSR_control1, 0x00A33A54);
GAME_VAR(int32_t, PSR_control2, 0x00A33A58);
GAME_VAR(int32_t, PSR_control3, 0x00A33A5C);
GAME_VAR(int32_t, PSR_control4, 0x00A33A60);
GAME_VAR(int32_t, PSR_control5, 0x00A33A64);

GAME_FN(void, __thiscall, PSR_RefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(uint8_t, __thiscall, PSR_HandleIsValid, 0x005D80E0, void* handle);
GAME_FN(void*, __thiscall, PSR_HandleGetFile, 0x005D80B0, void* handle);
GAME_FN(void*, __thiscall, PSR_HandleGetNode, 0x005D9C20, void* handle);
GAME_FN(void*, __thiscall, PSR_Handle5D80D0, 0x005D80D0, void* handle);
GAME_FN(void*, __thiscall, PSR_ActFilesCurrent, 0x005DB960, void* files);
GAME_FN(void*, __thiscall, PSR_ActFilesFind, 0x005DB8F0, void* files, void* file);
GAME_FN(void, __thiscall, PSR_ActFilesSetCurrent, 0x005DBDA0, void* files, void* file);
GAME_FN(void, __cdecl, PSR_Node5D8270, 0x005D8270, void* node);
GAME_FN(void, __cdecl, PSR_Node5D8670, 0x005D8670, void* node, void*);
GAME_FN(void, __cdecl, PSR_Node5D85C0, 0x005D85C0, void* node, void*);
GAME_FN(void, __cdecl, PSR_Node5D82A0, 0x005D82A0, void* node, int, int);
GAME_FN(uint8_t*, __thiscall, PSR_PedGetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, PSR_Targeting4D7780, 0x004D7780, void* targeting);
GAME_FN(uint8_t*, __thiscall, PSR_AnimFind, 0x0042E350, void* anims, const char* name, uint32_t* id);
GAME_FN(int, __cdecl, PSR_AnimStreamId, 0x0052DBD0, void* anims, uint32_t id);
GAME_FN(uint8_t*, __cdecl, PSR_StreamingInfo, 0x0052DD30, int slot);
GAME_FN(void, __thiscall, PSR_PedAddBlend, 0x004739E0, void* ped, uint32_t id);
GAME_FN(uint8_t*, __thiscall, PSR_PedGetModelInfo, 0x00477DC0, void* ped);
GAME_FN(int*, __thiscall, PSR_ModelInfo522380, 0x00522380, void* info, int* out);
GAME_FN(int, __thiscall, PSR_InputFindControl, 0x006A9FD0, void* input, void* name);
GAME_FN(uint8_t*, __thiscall, PSR_InputGetControl, 0x006AA7C0, void* input, int id);
GAME_FN(void*, __thiscall, PSR_InputDevice6AA680, 0x006AA680, void* input);
GAME_FN(void*, __thiscall, PSR_InputDevice6AA690, 0x006AA690, void* input);
GAME_FN(uint8_t, __thiscall, PSR_DeviceActive, 0x006AA3F0, void* device);
GAME_FN(uint8_t, __thiscall, PSR_DevicePressed, 0x006AA410, void* device, int id);
GAME_FN(uint8_t, __thiscall, PSR_Ped474B80, 0x00474B80, void* ped);
GAME_FN(uint8_t, __thiscall, PSR_Ped44E160, 0x0044E160, void* ped);
GAME_FN(void, __thiscall, PSR_Ped474A90, 0x00474A90, void* ped, int);
GAME_FN(void, __thiscall, PSR_Ped474B30, 0x00474B30, void* ped, int);
GAME_FN(void, __thiscall, PSR_Ped47D1B0, 0x0047D1B0, void* ped, int);
GAME_FN(void, __thiscall, PSR_Ref448230, 0x00448230, void* ref, void* from);
GAME_FN(int, __cdecl, PSR_RandomRange, 0x0040DA80, int low, int high);
GAME_FN(uint8_t, __thiscall, PSR_ControllerIsPlaying, 0x005F4800, void* controller, void* node, int, int);
// (the social object's own, through the game's address)
GAME_FN(int*, __thiscall, PSR_GetMood, 0x0048EBC0, uint8_t* self, int* out, void* other, int fresh, int* outA, int* outB);
GAME_FN(void, __thiscall, PSR_SetMood, 0x0048F1D0, uint8_t* self, void* other, int mood, int keep, int keepKept);
GAME_FN(void, __thiscall, PSR_DropHumiliation, 0x0048DDB0, uint8_t* self);
GAME_FN(void, __thiscall, PSR_ChooseKiss, 0x0048DC50, uint8_t* self, int index);
GAME_FN(uint8_t, __thiscall, PSR_Check6CAAC0, 0x0048E6B0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSR_Is124, 0x0048D6B0, uint8_t* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSR_Is128, 0x0048D6D0, uint8_t* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSR_Is120, 0x0048D690, uint8_t* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSR_CanAttack48FD00, 0x0048FD00, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSR_OtherMoveIsFriendly, 0x0048D580, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSR_OtherMoveIsA, 0x0048D5C0, uint8_t* self);
GAME_FN(uint8_t, __cdecl, PSR_Button1, 0x0048DEF0);
GAME_FN(uint8_t, __cdecl, PSR_Button5, 0x0048E1F0);

// The node of a handle played (inlined everywhere): with `switchFile` in its own act file
void PlayHandle(void* handle, void* ped, bool switchFile) {
    void* saved = nullptr;
    if (switchFile) {
        saved = PSR_ActFilesCurrent(PSR_actFiles);
        void* file = PSR_ActFilesFind(PSR_actFiles, PSR_HandleGetFile(handle));
        if (!file) {
            return;
        }
        PSR_ActFilesSetCurrent(PSR_actFiles, file);
    }
    void* node = PSR_HandleGetNode(handle);
    PSR_Node5D8270(node);
    PSR_Node5D8670(node, PSR_Handle5D80D0(handle));
    void* value = reinterpret_cast<void*(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0xAC / 4]))(ped);
    PSR_Node5D85C0(node, value);
    PSR_Node5D82A0(node, 1, 0);
    if (switchFile) {
        PSR_ActFilesSetCurrent(PSR_actFiles, saved);
    }
}

bool ControlDown(int32_t control) {
    if (At<uint16_t>(PSR_InputGetControl(PSR_input, control), 2) > 0 || (PSR_InputGetControl(PSR_input, control)[4] & 8)) {
        return true;
    }
    if (PSR_DeviceActive(PSR_InputDevice6AA680(PSR_input)) && PSR_DevicePressed(PSR_InputDevice6AA680(PSR_input), control)) {
        return true;
    }
    return PSR_DeviceActive(PSR_InputDevice6AA690(PSR_input)) && PSR_DevicePressed(PSR_InputDevice6AA690(PSR_input), control);
}

void InitControl(int32_t& control, uintptr_t name) {
    if (control == -1) {
        control = PSR_InputFindControl(PSR_input, reinterpret_cast<void*>(name));
    }
}

int Clique(void* ped) {
    int clique;
    return *PSR_ModelInfo522380(PSR_PedGetModelInfo(ped), &clique);
}

bool IsSpecialModel(void* ped) {
    int model = At<int16_t>(ped, 0x10E);
    return model == PSR_specialModel1 || model == PSR_specialModel2;
}

// the timer of +0x218 started if it isn't
void Start218(uint8_t* self) {
    if (At<uint32_t>(self, 0x218) == 0) {
        At<uint32_t>(self, 0x218) = PSR_timeInMs;
    }
}

} // namespace

namespace c07psr {

// 0x48F1D0: the mood towards `other` made `mood`: when it isn't already, the nearest
// cell of that mood (from the ped's two scores) kept as the adjustment for +0x74; with
// `keep` (or `keepKept` and one kept) it's kept at +0x12C
void __fastcall SetMood(uint8_t* self, re::Edx, uint8_t* other, int mood, int keep, int keepKept) {
    if (!other) {
        return;
    }
    if (At<int32_t>(other, 0x1310) == 13) {
        At<int32_t>(self, 0x130) = mood;
    }
    int now, a, b;
    PSR_GetMood(self, &now, other, 1, &a, &b);
    At<uint8_t>(self, 0x224) |= 0x20;
    if (now == mood) {
        if (At<uint8_t*>(self, 0x74) != other) {
            At<int32_t>(self, 0x110) = 0;
            At<int32_t>(self, 0x114) = 0;
        }
        PSR_RefSet(self + 0x74, other);
    } else {
        int rowAt = a + 5;
        int colAt = b + 5;
        int bestRow = -1;
        int bestCol = -1;
        for (int row = 0; row < 11; row++) {
            for (int col = 0; col < 11; col++) {
                if (mood != PSR_moods[row * 11 + col]) {
                    continue;
                }
                if (bestRow == -1 || (col - colAt) * (col - colAt) + (row - rowAt) * (row - rowAt) <
                                         (bestCol - colAt) * (bestCol - colAt) + (bestRow - rowAt) * (bestRow - rowAt)) {
                    bestRow = row;
                    bestCol = col;
                }
            }
        }
        if (bestRow != -1) {
            int dA = bestRow - rowAt;
            int dB = bestCol - colAt;
            if (At<uint8_t*>(self, 0x74) == other) {
                At<int32_t>(self, 0x110) += dA;
                At<int32_t>(self, 0x114) += dB;
            } else {
                At<int32_t>(self, 0x110) = dA;
                At<int32_t>(self, 0x114) = dB;
            }
            PSR_RefSet(self + 0x74, other);
        }
    }
    if (!re::Flag(keep) && (!re::Flag(keepKept) || At<int32_t>(self, 0x12C) == -1)) {
        At<int32_t>(self, 0x12C) = -1;
        return;
    }
    At<int32_t>(self, 0x12C) = mood;
}

// 0x48F380: the ped's pending exchange (+0x1D30) with the other side marked done
void __fastcall EndPending(uint8_t* self, re::Edx) {
    uint8_t* ped = At<uint8_t*>(self, 4);
    uint8_t* pending = At<uint8_t*>(ped, 0x1D30);
    if (pending && At<uint8_t*>(pending, 8) == ped && At<void*>(self, 0x70) && At<void*>(pending, 4) == At<void*>(self, 0x70)) {
        At<uint8_t*>(ped, 0x1D30) = nullptr;   // (the inlined 0x48D710's compare is always true)
        At<uint8_t>(pending, 0x2A) = 1;
    }
}

// 0x48F3C0: an answer (handle 0x19: the current, +0xE0) played: kind 0 in place (moving
// the exchange on: to the player 2 / 5, else 1 -> 2 and 3 -> 6 with the other told), kind
// 1 in its act file; the flags (+0x1D8) must match
uint8_t __fastcall PlayAnswer(uint8_t* self, re::Edx, int kind, int index) {
    if (index == 0x19) {
        index = At<int32_t>(self, 0xE0);
    }
    uint8_t* handle = self + 0x170 + index * 4;
    if (!PSR_HandleIsValid(handle)) {
        return 0;
    }
    if (At<int32_t>(At<uint8_t*>(self, 4), 0x1444) != 0) {
        return 1;
    }
    uint8_t mode = static_cast<uint8_t>(kind);
    if (mode == 0) {
        if (At<uint8_t>(self, 0x1D8 + index) != 0) {
            return 0;
        }
        PlayHandle(handle, At<void*>(self, 4), false);
        uint8_t* other = At<uint8_t*>(self, 0x70);
        At<uint8_t>(self, 0x224) |= 4;
        if (other && At<int32_t>(other, 0x1310) == 13) {
            At<uint32_t>(self, 0xF4) = PSR_timeInMs;
            At<int32_t>(self, 0x6C) = At<int32_t>(self, 0x6C) != 3 ? 2 : 5;
            At<int32_t>(At<uint8_t*>(self, 4), 0x1494) = 0;
            return 1;
        }
        if (At<int32_t>(self, 0x6C) == 1) {
            At<uint32_t>(self, 0xF4) = PSR_timeInMs;
            At<int32_t>(self, 0x6C) = 2;
            if (other) {
                At<int32_t>(other, 0xBB0) = 3;
                At<int32_t>(At<uint8_t*>(self, 4), 0x1494) = 0;
                return 1;
            }
        } else if (At<int32_t>(self, 0x6C) == 3) {
            At<uint32_t>(self, 0xF4) = PSR_timeInMs;
            At<int32_t>(self, 0x6C) = 6;
            if (other) {
                At<int32_t>(other, 0xBB0) = 5;
            }
        }
        At<int32_t>(At<uint8_t*>(self, 4), 0x1494) = 0;
        return 1;
    }
    if (mode == 1 && At<uint8_t>(self, 0x1D8 + index) == 1) {
        PlayHandle(handle, At<void*>(self, 4), true);
        At<int32_t>(At<uint8_t*>(self, 4), 0x1494) = 0;
        return 1;
    }
    return 0;
}

// 0x48F5D0: the other side (the ped's ped target, else its targeting's) plays its answer
// (0x17..0x24) in its act file
uint8_t __fastcall OtherPlaysAnswer(uint8_t* self, re::Edx, int kind, int answer) {
    if (static_cast<uint32_t>(answer - 0x17) > 0xD) {
        return 0;
    }
    int index = answer - 0x17;
    uint8_t* target = PSR_PedGetTarget(At<void*>(self, 4));
    if (target && (At<uint8_t>(target, 0x108) & 7) != 3) {
        target = nullptr;
    }
    if (!target) {
        target = PSR_Targeting4D7780(At<void*>(At<uint8_t*>(self, 4), 0x1C8));
        if (!target || (At<uint8_t>(target, 0x108) & 7) != 3) {
            target = nullptr;
        }
    }
    PSR_RefSet(self + 0x70, target);
    uint8_t* other = At<uint8_t*>(self, 0x70);
    if (!other || !PSR_HandleIsValid(other + 0xB68 + index * 4)) {
        return 0;
    }
    if (static_cast<uint8_t>(kind) != 1 || At<uint8_t>(At<uint8_t*>(self, 0x70), 0xBA0 + index) != 1) {
        return 0;
    }
    uint8_t* handle = At<uint8_t*>(self, 0x70) + 0xB68 + index * 4;
    PlayHandle(handle, At<void*>(self, 0x70), true);
    return 1;
}

// 0x48F720: a humiliation anim chosen (the old one dropped) and blended when loaded
void __fastcall ChooseHumiliation(uint8_t* self, re::Edx, int index) {
    uint8_t* ped = At<uint8_t*>(self, 4);
    if (!At<void*>(ped, 0x114)) {
        return;
    }
    int current = At<int32_t>(self, 0x200);
    if (current != -1 && current != index) {
        PSR_DropHumiliation(self);
    }
    At<int32_t>(self, 0x200) = index;
    void* anims = At<void*>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 4), 0x114), 0x94), 0x10);
    uint32_t id = 0;
    PSR_AnimFind(anims, PSR_humiliations + index * 0x20, &id);
    if (PSR_StreamingInfo(PSR_AnimStreamId(anims, id))[4] == 1) {
        PSR_PedAddBlend(At<void*>(self, 4), id);
    }
}

// 0x48F7B0: the kiss chosen from the buttons held and the clique (none while one's
// chosen)
void __fastcall ChooseKissFromButtons(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x208) != -1) {
        return;
    }
    int clique = Clique(At<void*>(self, 4));
    InitControl(PSR_control1, 0x01B65468);
    InitControl(PSR_control2, 0x01B6546C);
    InitControl(PSR_control3, 0x01B65470);
    InitControl(PSR_control4, 0x01B65474);
    if (PSR_kissFlags & 8) {
        PSR_ChooseKiss(self, 0);
        return;
    }
    if (clique == 3) {
        PSR_ChooseKiss(self, 5);
        return;
    }
    if ((clique == 2 || clique == 1) && !At<uint8_t>(PSR_PedGetModelInfo(At<void*>(self, 4)), 0x50) && ControlDown(PSR_control3)) {
        PSR_ChooseKiss(self, 2);
        return;
    }
    if (ControlDown(PSR_control4)) {
        PSR_ChooseKiss(self, 3);
        return;
    }
    if (ControlDown(PSR_control3)) {
        PSR_ChooseKiss(self, 2);
        return;
    }
    if (ControlDown(PSR_control2)) {
        PSR_ChooseKiss(self, At<uint8_t>(PSR_PedGetModelInfo(At<void*>(self, 4)), 0x50) ? 1 : 4);
        return;
    }
    PSR_ChooseKiss(self, 0);
}

// 0x48FB40: the ped wants to fight the player (starting the +0x218 timer)
uint8_t __fastcall WantsToFight(uint8_t* self, re::Edx) {
    InitControl(PSR_control1, 0x01B65468);
    uint8_t* ped = At<uint8_t*>(self, 4);
    uint8_t* player = PSR_playerPed;
    if (At<int32_t>(ped, 0x1464) != 0 && (!player || player != At<uint8_t*>(self, 0x120) || player == At<uint8_t*>(self, 0x128)) &&
        (At<uint8_t>(ped, 0x1570) != 1 || ((!player || player != At<uint8_t*>(self, 0x124)) && !PSR_Check6CAAC0(self)))) {
        Start218(self);
        return 1;
    }
    if (!At<uint8_t>(PSR_PedGetModelInfo(At<void*>(self, 4)), 0x50)) {
        return 0;
    }
    int clique = Clique(At<void*>(self, 4));
    if (clique != 1 && clique != 3) {
        return 0;
    }
    uint8_t* other = At<uint8_t*>(self, 0x70);
    if (At<uint8_t>(At<uint8_t*>(self, 4), 0x1570) != 1 || other != PSR_playerPed) {
        return 0;
    }
    if (other && other == At<uint8_t*>(self, 0x120) && !PSR_Is128(self, other)) {
        return 0;
    }
    if (PSR_Is124(self, other) || PSR_Check6CAAC0(self) || At<int32_t>(self, 0xEC) < 7 || !(At<uint8_t>(self, 0x225) & 0x40)) {
        return 0;
    }
    if (PSR_Ped474B80(At<void*>(self, 4))) {
        Start218(self);
        return 1;
    }
    InitControl(PSR_control5, 0x01B65478);
    if (At<uint16_t>(PSR_InputGetControl(PSR_input, PSR_control5), 2) > 0 || (PSR_InputGetControl(PSR_input, PSR_control5)[4] & 8)) {
        Start218(self);
        return 1;
    }
    return 0;
}

// 0x48FD00: the other side (not the player) is fighting this ped (its +0xBB0/+0xBB4, its
// mood 4..6) and it's the ped's +0x120, the ped's mood below 3
uint8_t __fastcall BeingFought(uint8_t* self, re::Edx) {
    uint8_t* other = At<uint8_t*>(self, 0x70);
    if (!At<void*>(self, 0x6C) || !other || At<int32_t>(other, 0x1310) == 13 || At<int32_t>(self, 0xEC) >= 3) {
        return 0;
    }
    uint8_t* ped = At<uint8_t*>(self, 4);
    int type = At<int32_t>(ped, 0x1310);
    if (type == 0 || type == 8 || type == 7 || At<int32_t>(other, 0xBB0) == 0 || At<void*>(other, 0xBB4) == nullptr ||
        At<uint8_t*>(other, 0xBB4) != ped) {
        return 0;
    }
    if (At<int32_t>(other, 0xC30) > 3 && At<int32_t>(other, 0xC30) < 7 && PSR_Is120(self, ped)) {
        return 1;
    }
    return 0;
}

// 0x48FD80: the same, for a non-student ped the other side is after
uint8_t __fastcall BeingFoughtByOther(uint8_t* self, re::Edx) {
    uint8_t* other = At<uint8_t*>(self, 0x70);
    if (!At<void*>(self, 0x6C) || !other || At<int32_t>(other, 0x1310) == 13) {
        return 0;
    }
    int type = At<int32_t>(At<uint8_t*>(self, 4), 0x1310);
    if (type != 0 && type != 8 && type != 7 && At<void*>(other, 0xBB4) == At<void*>(self, 4) && PSR_CanAttack48FD00(self)) {
        return 1;
    }
    return 0;
}

// 0x48FDD0: a +0x50 model, clique neither 3 nor 4, not 0x44E160, not type 1
uint8_t __fastcall IsKind48FDD0(uint8_t* self, re::Edx) {
    if (!At<uint8_t>(PSR_PedGetModelInfo(At<void*>(self, 4)), 0x50)) {
        return 0;
    }
    if (Clique(At<void*>(self, 4)) != 3 && Clique(At<void*>(self, 4)) != 4 && !PSR_Ped44E160(At<void*>(self, 4)) &&
        At<int32_t>(At<uint8_t*>(self, 4), 0x1310) != 1) {
        return 1;
    }
    return 0;
}

// 0x48FE40: type 4, 2, 0xB or 3, no +0x50, clique not 3
uint8_t __fastcall IsKind48FE40(uint8_t* self, re::Edx) {
    int type = At<int32_t>(At<uint8_t*>(self, 4), 0x1310);
    if ((type == 4 || type == 2 || type == 0xB || type == 3) && !At<uint8_t>(PSR_PedGetModelInfo(At<void*>(self, 4)), 0x50) &&
        Clique(At<void*>(self, 4)) != 3) {
        return 1;
    }
    return 0;
}

// 0x48FEA0: type 1, no +0x50, clique not 3
uint8_t __fastcall IsKind48FEA0(uint8_t* self, re::Edx) {
    if (At<int32_t>(At<uint8_t*>(self, 4), 0x1310) == 1 && !At<uint8_t>(PSR_PedGetModelInfo(At<void*>(self, 4)), 0x50) &&
        Clique(At<void*>(self, 4)) != 3) {
        return 1;
    }
    return 0;
}

// 0x48FEF0: type 0 or 7 (with +0x50 only clique 4), clique not 3
uint8_t __fastcall IsKind48FEF0(uint8_t* self, re::Edx) {
    int type = At<int32_t>(At<uint8_t*>(self, 4), 0x1310);
    if (type != 0 && type != 7) {
        return 0;
    }
    if (At<uint8_t>(PSR_PedGetModelInfo(At<void*>(self, 4)), 0x50) && Clique(At<void*>(self, 4)) != 4) {
        return 0;
    }
    return Clique(At<void*>(self, 4)) != 3 ? 1 : 0;
}

// 0x48FF60: none of the types 4, 2, 0xB, 3, 1, 0, 7 (with +0x50 only clique 4), clique
// not 3, not 0x44E160
uint8_t __fastcall IsKind48FF60(uint8_t* self, re::Edx) {
    int type = At<int32_t>(At<uint8_t*>(self, 4), 0x1310);
    if (type == 4 || type == 2 || type == 0xB || type == 3 || type == 1 || type == 0 || type == 7) {
        return 0;
    }
    if (At<uint8_t>(PSR_PedGetModelInfo(At<void*>(self, 4)), 0x50) && Clique(At<void*>(self, 4)) != 4) {
        return 0;
    }
    if (Clique(At<void*>(self, 4)) != 3 && !PSR_Ped44E160(At<void*>(self, 4))) {
        return 1;
    }
    return 0;
}

// 0x48FFF0: type 1 with +0x50, clique not 3
uint8_t __fastcall IsKind48FFF0(uint8_t* self, re::Edx) {
    if (At<int32_t>(At<uint8_t*>(self, 4), 0x1310) == 1 && At<uint8_t>(PSR_PedGetModelInfo(At<void*>(self, 4)), 0x50) &&
        Clique(At<void*>(self, 4)) != 3) {
        return 1;
    }
    return 0;
}

// 0x490040: the player is engaged with the ped (kept for 0.4 seconds after)
uint8_t __fastcall PlayerEngaged(uint8_t* self, re::Edx) {
    uint8_t* ped = At<uint8_t*>(self, 4);
    uint8_t* intelligence = At<uint8_t*>(ped, 0x2E0);
    uint8_t* other = At<uint8_t*>(self, 0x70);
    if (other && At<int32_t>(other, 0x1310) == 13) {
        bool engaged = false;
        if ((At<int32_t>(self, 0x6C) == 3 || At<int32_t>(ped, 0x1494) != 0) && !(At<uint8_t>(self, 0x224) & 1)) {
            engaged = true;
        } else {
            int state = At<int32_t>(self, 0xE0);
            if ((state == 0x13 || state == 0x14 || state == 0x15 || state == 0x17 || state == 6 || state == 9 || state == 0xE ||
                 state == 3) &&
                (PSR_ControllerIsPlaying(intelligence + 0x258, PSR_nodeA, 1, 1) ||
                 PSR_ControllerIsPlaying(intelligence + 0x290, PSR_nodeB, 1, 1))) {
                engaged = true;
            } else if (At<void*>(At<uint8_t*>(self, 4), 0x155C) == At<void*>(self, 0x70)) {
                engaged = true;
            }
        }
        if (engaged) {
            At<uint32_t>(self, 0x220) = PSR_timeInMs;
            return 1;
        }
    }
    return PSR_timeInMs < At<uint32_t>(self, 0x220) + 400 ? 1 : 0;
}

// 0x490120: the ped's answer to the player chosen (+0xE0)
void __fastcall ChooseAnswer(uint8_t* self, re::Edx, int arg1, int arg2, int arg3) {
    uint8_t flags = At<uint8_t>(self, 0x224);
    uint8_t bit1 = (At<uint8_t>(self, 0x226) >> 1) & 1;
    uint8_t first = static_cast<uint8_t>(arg1);
    bool leave = (flags & 0x80) != 0;
    if (!leave && bit1 && (At<uint8_t>(At<uint8_t*>(self, 4), 0x1570) == 1 || IsSpecialModel(At<void*>(self, 4)))) {
        leave = true;
    }
    if (!leave && (static_cast<uint8_t>(arg2) == 1 || static_cast<uint8_t>(arg3) == 1) && At<int32_t>(self, 0xE8) != 8 &&
        (!first || IsSpecialModel(At<void*>(self, 4)))) {
        leave = true;
    }
    bool stopFight = false;
    if (leave) {
        At<int32_t>(self, 0xE0) = 0x15;
        if (!(flags & 0x40)) {
            return;
        }
        stopFight = true;
    } else if (bit1 == 1) {
        int move = At<int32_t>(self, 0xE8);
        if (move == 0x16 || move == 2) {
            PSR_RefSet(self + 0x128, At<void*>(self, 0x70));
            if (PSR_Button5()) {
                At<int32_t>(self, 0xE0) = 0x17;
            } else {
                At<int32_t>(self, 0xE0) = PSR_Button1() ? 9 : 6;
            }
            if (!(At<uint8_t>(self, 0x225) & 2) && At<int32_t>(At<uint8_t*>(self, 4), 0x141C) == 0 &&
                At<int32_t>(self, 0x1FC) <= At<int32_t>(self, 0xEC)) {
                return;
            }
            PSR_SetMood(self, At<void*>(self, 0x70), At<int32_t>(self, 0x1FC), 0, (At<uint8_t>(self, 0x226) >> 2) & 1);
            At<int32_t>(self, 0xEC) = At<int32_t>(self, 0x1FC);
            return;
        }
        At<int32_t>(self, 0x134) += 1;
        At<int32_t>(self, 0xE0) = 0;
        return;
    } else {
        bool fight = false;
        bool calm = false;
        bool pick = false;
        if (At<uint8_t>(self, 0x225) & 1) {
            if (flags & 0x40) {
                At<int32_t>(self, 0xE0) = 0x15;
                stopFight = true;
            } else if (At<int32_t>(self, 0xE8) == 5) {
                PSR_Ref448230(self + 0x128, self + 0x70);
                At<int32_t>(self, 0xE0) = PSR_Button1() ? 9 : 6;
                return;
            } else if (PSR_OtherMoveIsA(self)) {
                At<int32_t>(self, 0xE0) = 0;
                return;
            } else if (first) {
                calm = true;
            } else if (PSR_OtherMoveIsFriendly(self) || PSR_OtherMoveIsA(self)) {
                fight = true;
            } else {
                calm = true;
            }
        } else {
            if (flags & 8) {
                At<int32_t>(self, 0xE0) = 9;
                return;
            }
            uint8_t bit6 = (flags >> 6) & 1;
            if (bit6 && At<int32_t>(self, 0xE8) == 0x16) {
                At<int32_t>(self, 0xE0) = 6;
                stopFight = true;
            } else if (At<int32_t>(self, 0xE8) == 2) {
                At<int32_t>(self, 0xE0) = 0x17;
                stopFight = true;
            } else if (!first && !bit6 && (PSR_OtherMoveIsFriendly(self) || PSR_OtherMoveIsA(self))) {
                if (!PSR_Button1() || At<int32_t>(self, 0xE8) != 0x16) {
                    fight = true;
                } else {
                    At<int32_t>(self, 0xE0) = 9;
                    return;
                }
            } else {
                pick = true;
            }
        }
        if (pick) {
            uint8_t* ped = At<uint8_t*>(self, 4);
            if (At<uint8_t*>(At<uint8_t*>(self, 0x70), 0xC64) == ped) {
                calm = true;
            } else {
                int type = At<int32_t>(ped, 0x1310);
                int r = (type == 5 || type == 4) ? PSR_RandomRange(2, 9) : PSR_RandomRange(0, 5);
                At<int32_t>(self, 0xE0) = PSR_answers[r + At<int32_t>(self, 0xEC) * 10];
                return;
            }
        }
        if (calm) {
            At<int32_t>(self, 0xE0) = 6;
            return;
        }
        if (fight) {
            At<int32_t>(self, 0xE0) = 0x11;
            PSR_Ped474B30(At<void*>(self, 4), 0x16);
            PSR_Ped474A90(At<void*>(self, 4), 1);
            PSR_Ped47D1B0(At<void*>(self, 4), 0);
            At<uint8_t>(self, 0x224) |= 0x40;
            return;
        }
    }
    if (stopFight) {
        PSR_Ped474A90(At<void*>(self, 4), 0);
        PSR_Ped474B30(At<void*>(self, 4), 0);
        At<uint8_t>(self, 0x224) &= 0xBF;
    }
}

} // namespace c07psr

#define PSR_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PSR_REVERSED("PedSocial::SetMood", 0x0048F1D0, &c07psr::SetMood);
PSR_REVERSED("PedSocial::EndPending", 0x0048F380, &c07psr::EndPending);
PSR_REVERSED("PedSocial::PlayAnswer", 0x0048F3C0, &c07psr::PlayAnswer);
PSR_REVERSED("PedSocial::OtherPlaysAnswer", 0x0048F5D0, &c07psr::OtherPlaysAnswer);
PSR_REVERSED("PedSocial::ChooseHumiliation", 0x0048F720, &c07psr::ChooseHumiliation);
PSR_REVERSED("PedSocial::ChooseKissFromButtons", 0x0048F7B0, &c07psr::ChooseKissFromButtons);
PSR_REVERSED("PedSocial::WantsToFight", 0x0048FB40, &c07psr::WantsToFight);
PSR_REVERSED("PedSocial::BeingFought", 0x0048FD00, &c07psr::BeingFought);
PSR_REVERSED("PedSocial::BeingFoughtByOther", 0x0048FD80, &c07psr::BeingFoughtByOther);
PSR_REVERSED("PedSocial::IsKind48FDD0", 0x0048FDD0, &c07psr::IsKind48FDD0);
PSR_REVERSED("PedSocial::IsKind48FE40", 0x0048FE40, &c07psr::IsKind48FE40);
PSR_REVERSED("PedSocial::IsKind48FEA0", 0x0048FEA0, &c07psr::IsKind48FEA0);
PSR_REVERSED("PedSocial::IsKind48FEF0", 0x0048FEF0, &c07psr::IsKind48FEF0);
PSR_REVERSED("PedSocial::IsKind48FF60", 0x0048FF60, &c07psr::IsKind48FF60);
PSR_REVERSED("PedSocial::IsKind48FFF0", 0x0048FFF0, &c07psr::IsKind48FFF0);
PSR_REVERSED("PedSocial::PlayerEngaged", 0x00490040, &c07psr::PlayerEngaged);
PSR_REVERSED("PedSocial::ChooseAnswer", 0x00490120, &c07psr::ChooseAnswer);
