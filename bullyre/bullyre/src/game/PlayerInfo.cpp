// CPlayerInfo (CWorld::Players at 0xC1AEA8; +0 the player ped): its reset, the player's
// vehicle or ped, the control switch, and the tiredness (0 awake, 1 tired after 1:00, 2
// exhausted after 1:30 until 8:00: the collapse at 2:00).

#include <cstdint>

#include "reversed.h"

namespace {

GAME_VAR(uint8_t*, ms_pPlayerPed, 0x00C1AEA8);   // Players[0].ped
inline void* const ms_padControl = reinterpret_cast<void*>(0x00A45A30);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_VAR(int32_t, ms_tirednessActive, 0x00CF109C);
GAME_VAR(float, ms_tirednessLevel, 0x00CF10A0);
GAME_VAR(float, ms_exhaustedScale, 0x00C0F5DC);    // (0.5, made on first use)
GAME_VAR(uint32_t, ms_exhaustedScaleGuard, 0x00C0F5E0);
GAME_VAR(uint8_t, ms_bNoTiredness_C0F5C8, 0x00C0F5C8);
GAME_VAR(int32_t, ms_widescreenSetting, 0x00C3CD0C);
GAME_VAR(uint8_t, ms_bCutsceneRunning, 0x020C5BE4);
GAME_VAR(uint8_t, ms_bClockPaused, 0x00BCEB42);
GAME_VAR(uint8_t, ms_clockMinute, 0x00BCEB4A);
GAME_VAR(uint8_t, ms_clockHour, 0x00BCEB4B);
GAME_VAR(int32_t, ms_currentDay, 0x020C3640);
GAME_VAR(uint8_t, ms_bLateNight_A45A50, 0x00A45A50);
GAME_VAR(uint8_t*, ms_pTirednessTimes, 0x020C88B8);   // (+0xC8 tired, +0xCC exhausted, +0xD0 flag 1's)
GAME_VAR(int32_t, ms_tickRate, 0x009E51A4);
GAME_VAR(float, ms_fadeAlpha, 0x00C3CD28);
GAME_VAR(void*, ms_pWeaponHud, 0x00C674F0);
inline void* const ms_ladderAnimSet = reinterpret_cast<void*>(0x01B654F0);

GAME_FN(void, __thiscall, PlayerInfo_Fn_5D9C50, 0x005D9C50, void* self);
GAME_FN(void, __thiscall, Pad_SetDisabled, 0x0050DFA0, void* pad, int disabled);
GAME_FN(void, __cdecl, Fn_738780, 0x00738780);
GAME_FN(void, __cdecl, Fn_45EE40, 0x0045EE40, int);
GAME_FN(uint8_t, __cdecl, Fn_425320, 0x00425320);
GAME_FN(uint8_t, __cdecl, Fn_425E40, 0x00425E40, int, int, int, int);
GAME_FN(int, __thiscall, Camera_GetFadeState, 0x004F12F0, void* camera);
GAME_FN(int, __cdecl, GetTimeStepInMs, 0x0045B830);
GAME_FN(void, __thiscall, Ped_Fn_477470, 0x00477470, void* ped);
GAME_FN(void, __thiscall, Fn_5F40A0, 0x005F40A0, void* self);
GAME_FN(uint8_t, __thiscall, Fn_5F4800, 0x005F4800, void* self, void* set, int, int);
GAME_FN(void, __thiscall, Ped_PlayAction, 0x004740F0, void* ped, const char* action, const char* file);
GAME_FN(uint8_t, __thiscall, Ped_Fn_4CE690, 0x004CE690, void* ped);
GAME_FN(void, __cdecl, Clock_SetGameClock, 0x00426070, int hour, int minute);
GAME_FN(uint8_t, __thiscall, Fn_6AA7E0, 0x006AA7E0, void* self);
GAME_FN(void, __thiscall, WeaponHud_Fn_55C8D0, 0x0055C8D0, void* hud);
// (ours, called where modes apply)
GAME_FN(void, __thiscall, PlayerInfo_SetTiredness, 0x00445E30, uint8_t* self, int level);

using re::At;
template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

} // namespace

namespace PlayerInfo {

// 0x445AE0 (thiscall): CPlayerInfo::Clear: everything back to its start (its +0x44
// entity told (+0x33A bit 7 cleared)), then 0x5D9C50 on its +0x2CC
static void __fastcall PlayerInfo_Clear(uint8_t* self, re::Edx) {
    uint8_t* linked = At<uint8_t*>(self, 0x44);
    At<int32_t>(self, 0) = 0;
    if (linked) {
        linked[0x33A] &= 0x7F;
        At<int32_t>(self, 0x44) = 0;
    }
    self[0xC4] &= 0xF0;
    At<float>(self, 0xA0) = 1.0f;
    At<int32_t>(self, 0x4C) = 0;
    At<int32_t>(self, 0x48) = 0;
    At<float>(self, 0x68) = 0.0f;
    At<uint8_t>(self, 0x58) = 0;
    At<float>(self, 0x74) = 0.0f;
    At<int32_t>(self, 0x5C) = 0;
    At<float>(self, 0x7C) = 0.0f;
    At<int16_t>(self, 0x9C) = 0;
    At<float>(self, 0x88) = 0.0f;
    At<int32_t>(self, 0x50) = 0;
    At<float>(self, 0x90) = 0.0f;
    At<int32_t>(self, 0x54) = 3;
    At<float>(self, 0x98) = 0.0f;
    At<int32_t>(self, 0x60) = 0;
    At<int32_t>(self, 0x64) = 0;
    At<int32_t>(self, 0x6C) = 0;
    At<int32_t>(self, 0x70) = 0;
    At<int32_t>(self, 0x78) = 0;
    At<int32_t>(self, 0x80) = 0;
    At<int32_t>(self, 0x84) = 0;
    At<int32_t>(self, 0x8C) = 0;
    At<int32_t>(self, 0x94) = 0;
    At<uint8_t>(self, 0xA6) = 0;
    At<uint8_t>(self, 0xA7) = 0;
    At<uint8_t>(self, 0xA4) = 0;
    At<uint8_t>(self, 0xA5) = 0;
    At<int32_t>(self, 0xA8) = 0;
    At<int32_t>(self, 0xAC) = 0;
    At<int32_t>(self, 0xB0) = 0;
    At<int32_t>(self, 0xB8) = 0;
    At<int32_t>(self, 0xBC) = -1;
    At<int32_t>(self, 0xC0) = 0;
    At<int32_t>(self, 0x110) = 0;
    At<int32_t>(self, 0x114) = 0;
    At<int32_t>(self, 0x118) = 0;
    At<int32_t>(self, 0x11C) = 0;
    At<int32_t>(self, 0x120) = 0;
    At<int32_t>(self, 0x124) = 0;
    At<int32_t>(self, 0x128) = 0;
    At<int32_t>(self, 0x12C) = 0;
    At<int32_t>(self, 0x130) = 0;
    At<int32_t>(self, 0x134) = 0;
    At<int32_t>(self, 0x138) = 0;
    At<int32_t>(self, 0x13C) = 0;
    At<int32_t>(self, 0x140) = 0;
    At<int32_t>(self, 0x144) = 0;
    At<int32_t>(self, 0x148) = 0;
    At<int32_t>(self, 0x14C) = 0;
    At<int32_t>(self, 0x150) = 0;
    At<int32_t>(self, 0x154) = 0;
    At<int32_t>(self, 0x158) = 0;
    At<int32_t>(self, 0x15C) = 0;
    At<int32_t>(self, 0x160) = 0;
    At<int32_t>(self, 0x164) = 0;
    At<int32_t>(self, 0x168) = 0;
    At<int32_t>(self, 0x16C) = 0;
    At<int32_t>(self, 0x170) = 0;
    At<int32_t>(self, 0x174) = 0;
    At<int32_t>(self, 0x178) = 0;
    At<int32_t>(self, 0x17C) = 0;
    At<int32_t>(self, 0x180) = 0;
    At<int32_t>(self, 0x184) = 0;
    At<int32_t>(self, 0x188) = 0;
    At<int32_t>(self, 0x18C) = 0;
    At<int32_t>(self, 0x190) = 0;
    At<int32_t>(self, 0x194) = 0;
    At<int32_t>(self, 0x198) = 0;
    At<int32_t>(self, 0x19C) = 0;
    At<int32_t>(self, 0x1A0) = 0;
    At<int32_t>(self, 0x1A4) = 0;
    At<int32_t>(self, 0x1A8) = 0;
    At<uint8_t>(self, 0x1AC) = 0;
    At<int32_t>(self, 0x1B0) = 0;
    At<int32_t>(self, 0x1B4) = 0;
    At<int32_t>(self, 0x1B8) = 0;
    At<int32_t>(self, 0x1BC) = 0;
    At<int32_t>(self, 0x1C0) = 0;
    At<int32_t>(self, 0x1C4) = 0;
    At<int32_t>(self, 0x1C8) = 0;
    At<int32_t>(self, 0x1CC) = 0;
    At<int32_t>(self, 0x1D0) = 0;
    At<int32_t>(self, 0x1D4) = 0;
    At<int32_t>(self, 0x1D8) = 0;
    At<int32_t>(self, 0x1DC) = 0;
    At<int32_t>(self, 0x1E0) = 0;
    At<int32_t>(self, 0x1E4) = 0;
    At<int32_t>(self, 0x1E8) = 0;
    for (int i = 0; i < 0x32; i++) {
        At<int16_t>(self, 0x1EC + i * 4) = 0;
        At<int16_t>(self, 0x1EE + i * 4) = 0;
    }
    At<int32_t>(self, 0x2B4) = 0;
    At<int32_t>(self, 0x2B8) = 0;
    At<int32_t>(self, 0x2BC) = 0;
    At<int32_t>(self, 0x2C0) = 0;
    At<int32_t>(self, 0x2C4) = 0;
    At<uint8_t>(self, 0x2C8) = 0;
    At<uint8_t>(self, 0x2C9) = 0;
    At<uint8_t>(self, 0x2CA) = 0;
    At<uint8_t>(self, 0x2CB) = 0;
    At<uint8_t>(self, 0xE4) = 0;
    PlayerInfo_Fn_5D9C50(self + 0x2CC);
}

// 0x445D60: the player's vehicle's +0x124, else the ped's
static uint8_t* __cdecl GetPlayer124() {
    uint8_t* player = ms_pPlayerPed;
    uint8_t* vehicle = At<uint8_t*>(player, 0x1554);
    return vehicle ? vehicle + 0x124 : player + 0x124;
}

// 0x445D80: the player's vehicle, else the ped
static uint8_t* __cdecl GetPlayerEntity() {
    uint8_t* player = ms_pPlayerPed;
    uint8_t* vehicle = At<uint8_t*>(player, 0x1554);
    return vehicle ? vehicle : player;
}

// 0x445DA0 (thiscall): the player's control switched off (flag) or on again
static void __fastcall SetControlDisabled(uint8_t* self, re::Edx, int disable) {
    if (re::Flag(disable)) {
        Pad_SetDisabled(ms_padControl, 0);
        Fn_738780();
        uint8_t* ped = At<uint8_t*>(self, 0);
        At<int32_t>(ped, 0x70) = 1;
        At<int32_t>(At<uint8_t*>(self, 0), 0x44) = 1;
        At<int32_t>(At<uint8_t*>(self, 0), 0x68) = 1;
        At<int32_t>(At<uint8_t*>(self, 0), 0x6C) = 1;
        At<uint8_t*>(self, 0)[0x1E90] = 0;
        Fn_45EE40(0);
    } else {
        Pad_SetDisabled(ms_padControl, 1);
        At<int32_t>(At<uint8_t*>(self, 0), 0x70) = 0;
        At<int32_t>(At<uint8_t*>(self, 0), 0x44) = 0;
        At<int32_t>(At<uint8_t*>(self, 0), 0x68) = 0;
        At<int32_t>(At<uint8_t*>(self, 0), 0x6C) = 0;
        At<uint8_t*>(self, 0)[0x1E90] = 1;
        Fn_45EE40(1);
    }
    self[0xE4] = 0;
}

// 0x445E30 (thiscall): the tiredness level (0-2), its state reset
static void __fastcall SetTiredness(uint8_t* self, re::Edx, int level) {
    float value = float(int8_t(level));
    if (value < 0.0f) {
        value = 0.0f;
    } else if (2.0f < value) {
        value = 2.0f;
    }
    int32_t result = int32_t(int64_t(value));
    self[0xC4] &= 0xFC;
    self[0xA4] = uint8_t(result);
    self[0xA5] = 0;
}

// 0x445E90 (thiscall): the tiredness each frame (none while paused, in cutscenes or
// widescreen): tired at 1:00, exhausted from 1:30 until 8:00; the exhaustion builds over
// the day's time (+0xB0; the tired level +0xAC), at its end the player collapses (a
// ladder fall, the collapse action, or vf 0x3C) and wakes at 2:00. The screen's level
// (0xCF10A0) and whether it shows (0xCF109C) while not fading
static void __fastcall ProcessTiredness(uint8_t* self, re::Edx) {
    ms_tirednessActive = 0;
    if (!(ms_exhaustedScaleGuard & 1)) {
        ms_exhaustedScaleGuard |= 1;
        ms_exhaustedScale = 0.5f;
    }
    if (ms_bNoTiredness_C0F5C8 || ms_widescreenSetting != 0 || ms_bCutsceneRunning) {
        return;
    }
    if (ms_bClockPaused) {
        if (self[0xA4] != 0) {
            PlayerInfo_SetTiredness(self, 0);
        }
        return;
    }
    if (Fn_425320() || (self[0xC4] & 4)) {
        if (self[0xA4] != 0) {
            uint8_t bit = (self[0xC4] >> 2) & 1;
            PlayerInfo_SetTiredness(self, 0);
            self[0xC4] ^= (uint8_t(bit << 2) ^ self[0xC4]) & 4;
        }
        uint8_t flags = self[0xC4];
        if (!(flags & 4) || ms_currentDay < At<int32_t>(self, 0xBC) || int(ms_clockHour) < At<int32_t>(self, 0xB8)) {
            return;
        }
        At<int32_t>(self, 0xB8) = 0;
        self[0xC4] = flags & 0xFB;
        At<int32_t>(self, 0xBC) = -1;
        return;
    }
    float level = 0.0f;
    int32_t duration = 0;
    if (Fn_425E40(2, 7, 0, 0) && self[0xA4] == 1) {
        PlayerInfo_SetTiredness(self, 2);
    }
    uint8_t* ped = At<uint8_t*>(self, 0);
    if (!ped || ped[0x1C98] || ms_bCutsceneRunning || Camera_GetFadeState(TheCamera) != 0) {
        return;
    }
    int minute = ms_clockMinute;
    int hour = ms_clockHour;
    if (hour == 1) {
        if (minute <= 30) {
            if (self[0xA4] != 1) {
                PlayerInfo_SetTiredness(self, hour);
            }
        } else if (self[0xA4] != 2) {
            PlayerInfo_SetTiredness(self, 2);
        }
        ms_bLateNight_A45A50 = 1;
    } else if (hour < 2) {
        ms_bLateNight_A45A50 = 1;
    } else {
        if (minute > 1 && hour < 8 && self[0xA4] != 2) {
            PlayerInfo_SetTiredness(self, 2);
        }
        ms_bLateNight_A45A50 = hour >= 8 ? 1 : 0;
    }
    uint8_t flags = self[0xC4];
    bool flag1 = (flags & 1) != 0;
    if (flag1) {
        duration = At<int32_t>(ms_pTirednessTimes, 0xD0) * ms_tickRate;
    } else if (int8_t(self[0xA4]) == 1) {
        duration = At<int32_t>(ms_pTirednessTimes, 0xC8) * ms_tickRate;
    } else if (int8_t(self[0xA4]) == 2) {
        duration = At<int32_t>(ms_pTirednessTimes, 0xCC) * ms_tickRate;
    }
    switch (int8_t(self[0xA4])) {
    case 0:
        if (self[0xA5] != 0) {
            self[0xA5] = 0;
            At<int32_t>(self, 0xA8) = 0;
            self[0xC4] = flags & 0xFD;
            return;
        }
        if (!flag1) {
            break;
        }
        At<int32_t>(self, 0xA8) += GetTimeStepInMs();
        {
            float progress = float(1.0 / double(duration) * At<int32_t>(self, 0xA8));
            if (progress > 1.0f) {
                PlayerInfo_SetTiredness(self, uint8_t(self[0xA4] + 1));
            }
        }
        return;
    case 1:
        if (self[0xA5] != 1) {
            level = 0.0f;
            self[0xA5] = 1;
            At<int32_t>(self, 0xAC) = 0;
        } else {
            At<int32_t>(self, 0xAC) += GetTimeStepInMs();
            float progress = float(1.0 / double(duration) * At<int32_t>(self, 0xAC));
            if (1.0f < progress) {
                PlayerInfo_SetTiredness(self, uint8_t(self[0xA4] + 1));
                progress = 1.0f;
            }
            level = float(double(progress) * 0.5);
        }
        break;
    case 2:
        if (self[0xA5] != 2) {
            level = 0.5f;
            self[0xA5] = 2;
            At<int32_t>(self, 0xB0) = 0;
            self[0xC4] = flags & 0xFD;
            break;
        }
        if (hour >= 2 && minute > 1 && hour < 8) {
            At<int32_t>(self, 0xB0) = duration;
        }
        At<int32_t>(self, 0xB0) += GetTimeStepInMs();
        {
            float progress = float(1.0 / double(duration) * At<int32_t>(self, 0xB0));
            if (!(1.0f < progress)) {
                level = float(double(ms_exhaustedScale) * progress + 0.5);
                break;
            }
            progress = 1.0f;
            ped = At<uint8_t*>(self, 0);
            if (!(self[0xC4] & 2)) {
                uint8_t* vehicle = At<uint8_t*>(ped, 0x155C);
                if (vehicle) {
                    if (At<void*>(vehicle, 0x155C)) {
                        Ped_Fn_477470(vehicle);
                    }
                    Ped_Fn_477470(At<void*>(self, 0));
                    Fn_5F40A0(At<uint8_t*>(At<uint8_t*>(self, 0), 0x2E0) + 0x258);
                }
                uint8_t* actions = At<uint8_t*>(At<uint8_t*>(self, 0), 0x2E0) + 0x258;
                uint8_t onLadder = Fn_5F4800(actions, ms_ladderAnimSet, 1, 1);
                ped = At<uint8_t*>(self, 0);
                if (onLadder) {
                    Ped_PlayAction(ped, "/Global/Ladder/Ladder_Damage/Fall/Into_Fall", "Props/Ladder.act");
                    At<int32_t>(self, 0xB0) += -400;
                    self[0xC4] |= 2;
                    level = float(double(ms_exhaustedScale) * progress + 0.5);
                    break;
                }
                if (ped[0x16CC] == 0 && !Ped_Fn_4CE690(ped)) {
                    ped = At<uint8_t*>(self, 0);
                    if (ped[0x1E91] == 0 && At<int32_t>(ped, 0x153C) == 0) {
                        Ped_PlayAction(ped,
                                       "/Global/Player/Default_KEY/Locomotion/GlobalLoco/Locomotion/LocomotionExecutes/"
                                       "Exhausted_Collapse/Exhausted_Collapse_LoadCheck",
                                       "Act/Player.act");
                        At<int32_t>(self, 0xB0) += -0x578;
                        self[0xC4] |= 2;
                        level = float(double(ms_exhaustedScale) * progress + 0.5);
                        break;
                    }
                } else {
                    ped = At<uint8_t*>(self, 0);
                    VCall<void>(ped, 0x3C, 0.0f, 0.0f, 0, -1, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
                }
                self[0xC4] |= 2;
                level = float(double(ms_exhaustedScale) * progress + 0.5);
                break;
            }
            At<int32_t>(ped, 0x14C4) = 1;
            PlayerInfo_SetTiredness(self, 0);
            if (hour != 2) {
                Clock_SetGameClock(2, 0);
            }
            VCall<void>(At<void*>(self, 0), 0xA8, 0, 1);
            if (!Fn_6AA7E0(reinterpret_cast<void*>(0x020C3CA0))) {
                self[0x58] = 7;
            }
            if (ms_pWeaponHud) {
                WeaponHud_Fn_55C8D0(ms_pWeaponHud);
            }
            At<float>(At<uint8_t*>(At<uint8_t*>(self, 0), 0x2E0), 0x1C) = 0.0f;
            Pad_SetDisabled(ms_padControl, 0);
            level = float(double(ms_exhaustedScale) * progress + 0.5);
        }
        break;
    default:
        break;
    }
    if (!(0.009999999776482582f > ms_fadeAlpha)) {
        return;
    }
    ms_tirednessLevel = level;
    if (level > 0.009999999776482582f) {
        ms_tirednessActive = 1;
        return;
    }
    ms_tirednessActive = 0;
}

} // namespace PlayerInfo

// ---- registration ----------------------------------------------------------------------

#define PLAYERINFO_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace PlayerInfo {
PLAYERINFO_REVERSED("CPlayerInfo::Clear", 0x00445AE0, &PlayerInfo_Clear);
PLAYERINFO_REVERSED("CPlayerInfo::GetPlayer124", 0x00445D60, &GetPlayer124);
PLAYERINFO_REVERSED("CPlayerInfo::GetPlayerEntity", 0x00445D80, &GetPlayerEntity);
PLAYERINFO_REVERSED("CPlayerInfo::SetControlDisabled", 0x00445DA0, &SetControlDisabled);
PLAYERINFO_REVERSED("CPlayerInfo::SetTiredness", 0x00445E30, &SetTiredness);
PLAYERINFO_REVERSED("CPlayerInfo::ProcessTiredness", 0x00445E90, &ProcessTiredness);
} // namespace PlayerInfo
