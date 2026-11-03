// The social menu (HUD_SOCIAL and its actions, HUD_SOCIAL_APOLOGIZE .. BURN) drawn: each
// action's icon round the target (greyed and hidden when it can't be done), which of
// them the menu offers, and the errands line. 0x553D10..0x5570B8 (their constructors,
// Setup and Reset are Hud.cpp's).
//
// Every action's Draw: the player's target must be a ped ((+0x108 & 7) == 3); the icon
// is greyed (alpha 0x80) and the action hidden when the player can't talk (+0x1440),
// the target is busy (0x490040 on its social data at +0xB44) or refuses that action
// (its byte at +0xB4C + the action).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using vc::At;

struct CVector {
    float x, y, z;
};
struct CVector2D {
    float x, y;
};

GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, g_hudFollow, 0x00C67534);   // the component the side icons line up with (+0x44 left, +0x48 right)
GAME_VAR(uint8_t*, g_hudErrands, 0x00C67558);
GAME_VAR(uint8_t*, g_classSkip, 0x00C0F78C);   // (+0x30: the trouble apologizing still works under)
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_errandsOffsetX, 0x00CF0A30);
GAME_VAR(float, g_errandsOffsetY, 0x00AB76EC);
GAME_VAR(uint32_t, g_fontColour, 0x0208B764);
GAME_VAR(float, g_fontWrapRight, 0x0208B774);
GAME_VAR(uint32_t, g_fontDropColour, 0x0208B780);
GAME_ARRAY(void*, g_giftIcons, 0x00C66DD0);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const kClothing = reinterpret_cast<void*>(0x020C5F70);
inline const uint32_t* const kGreetMission = reinterpret_cast<const uint32_t*>(0x01B65468);
GAME_VAR(uint32_t, g_giftItem, 0x01B6548C);

// the icons (textures)
GAME_VAR(void*, g_iconDismiss, 0x00C66E70);
GAME_VAR(void*, g_iconFollow, 0x00C66E74);
GAME_VAR(void*, g_iconShove, 0x00C66E78);
GAME_VAR(void*, g_iconTaunt, 0x00C66E7C);
GAME_VAR(void*, g_iconBurn, 0x00C66E80);
GAME_VAR(void*, g_iconShoveBack, 0x00C66E84);
GAME_VAR(void*, g_iconShoveBackGirl, 0x00C66E88);
GAME_VAR(void*, g_iconGreet, 0x00C66E8C);
GAME_VAR(void*, g_iconStop, 0x00C66E90);
GAME_VAR(void*, g_iconJoin, 0x00C66EBC);
GAME_VAR(void*, g_iconApologizeB, 0x00C66EE8);
GAME_VAR(void*, g_iconTalk, 0x00C66EEC);
GAME_VAR(void*, g_iconGreetMission, 0x00C66EF0);
GAME_VAR(void*, g_iconFlirt, 0x00C66EF4);
GAME_VAR(void*, g_iconKiss, 0x00C66EF8);
GAME_VAR(void*, g_iconHumiliate, 0x00C66EFC);
GAME_VAR(void*, g_iconGift, 0x00C66F18);
GAME_VAR(void*, g_iconErrands, 0x00C66F20);

GAME_FN(CRGBA*, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, void* ped);  // (+0x50: female)
GAME_FN(int32_t*, __thiscall, PedModelInfo_GetType, 0x00522380, void* info, int32_t* out);
GAME_FN(void, __thiscall, Ped_GetGift, 0x00474AA0, void* ped, int32_t* gift);
GAME_FN(uint8_t, __thiscall, Ped_WantsGift, 0x00474A30, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsStateIdle, 0x0041AA50, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_44E160, 0x0044E160, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_473220, 0x00473220, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_HasItem, 0x0049A850, void* ped, int item, int);
GAME_FN(uint8_t*, __thiscall, ObjectiveQueue_GetCurrent, 0x00470340, void* queue);
GAME_FN(void*, __thiscall, Objective_GetTarget, 0x008B6120, void* objective);
GAME_FN(uint8_t, __thiscall, Social_IsBusy, 0x00490040, void* social);
GAME_FN(uint8_t, __thiscall, Social_48E1F0, 0x0048E1F0, void* social);
GAME_FN(uint8_t, __thiscall, Social_48E070, 0x0048E070, void* social);
GAME_FN(uint8_t, __thiscall, Social_48E2B0, 0x0048E2B0, void* social);
GAME_FN(uint8_t, __thiscall, Social_48E6B0, 0x0048E6B0, void* social);
GAME_FN(uint8_t, __thiscall, Social_Is120, 0x0048D690, void* social, void* ped);
GAME_FN(uint8_t, __thiscall, Social_Is124, 0x0048D6B0, void* social, void* ped);
GAME_FN(uint8_t, __thiscall, Social_Is128, 0x0048D6D0, void* social, void* ped);
GAME_FN(uint8_t, __thiscall, Social_HasBothSides, 0x0048D6F0, void* social);
GAME_FN(uint8_t, __thiscall, Clothing_IsWearingOutfit, 0x006CA9F0, void* clothing, uint32_t hash);
GAME_FN(int, __thiscall, MissionMgr_Find, 0x006A9FD0, void* mgr, const uint32_t* key);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetStats, 0x006AA7C0, void* mgr, int id);  // +2 passes, +4 flags
GAME_FN(uint8_t*, __thiscall, MissionMgr_Primary, 0x006AA680, void* mgr);
GAME_FN(uint8_t*, __thiscall, MissionMgr_Secondary, 0x006AA690, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionSlot_IsActive, 0x006AA3F0, void* slot);
GAME_FN(uint8_t, __thiscall, MissionSlot_Is, 0x006AA410, void* slot, int id);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(int, __thiscall, Game_4F12F0, 0x004F12F0, void* self);
GAME_FN(uint8_t, __thiscall, Ped_4CE690, 0x004CE690, void* ped);
GAME_FN(uint8_t, __thiscall, Controls_5F4800, 0x005F4800, void* controls, void* key, int, int);
GAME_FN(uint8_t*, __thiscall, Ped_477C00, 0x00477C00, void* ped, int, int);
GAME_FN(float, __cdecl, Distance_472D00, 0x00472D00, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, Dot_414640, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, Vector_Normalised_472D40, 0x00472D40, CVector* out, const CVector* in);
GAME_FN(uint8_t*, __thiscall, Ped_46ECE0, 0x0046ECE0, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_4473E0, 0x004473E0, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_473D60, 0x00473D60, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_474B80, 0x00474B80, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_474BF0, 0x00474BF0, void* ped);
GAME_FN(int32_t, __thiscall, Ped_477060, 0x00477060, void* ped, int);
GAME_FN(void, __thiscall, SocialX_490F90, 0x00490F90, void* social, void* ped);
GAME_FN(int32_t*, __thiscall, Social_48EBC0, 0x0048EBC0, void* social, int32_t* out, void* ped, int, uintptr_t, uintptr_t);
GAME_FN(uint8_t, __thiscall, Social_48FB40, 0x0048FB40, void* social);
GAME_FN(uint8_t, __thiscall, Social_48E130, 0x0048E130, void* social);
GAME_FN(int, __cdecl, Objective_496B30, 0x00496B30, void* objective);
GAME_FN(uint8_t, __cdecl, Game_4D9790, 0x004D9790, int32_t, int);
GAME_FN(void, __thiscall, HUDComponent_53CF50, 0x0053CF50, HUDComponent* self);
GAME_FN(void*, __cdecl, GetButtonTexture, 0x00514EC0, int button);
GAME_VAR(int32_t, g_poiModel, 0x00BF42DC);
GAME_VAR(int32_t, g_punishmentModel, 0x00BF42D8);
GAME_VAR(int32_t, g_BF43F4, 0x00BF43F4);
GAME_VAR(int32_t, g_BF4274, 0x00BF4274);
GAME_VAR(void*, g_iconSideAction, 0x00C66F24);
GAME_VAR(uint8_t, g_widescreenText, 0x00C1870E);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __thiscall, PedFriends_485590, 0x00485590, void* self, void* ped);

// the player's target when it's a ped
uint8_t* TargetPed() {
    uint8_t* target = Ped_GetTarget(g_playerPed);
    if (target && (target[0x108] & 7) == 3) {
        return target;
    }
    return nullptr;
}

// greyed (and hidden) unless the player can talk and the target takes this action
int ActionAlpha(HUDComponent* self, uint8_t* target, int refusalByte) {
    if (*reinterpret_cast<int32_t*>(g_playerPed + 0x1440) == 0 && !Social_IsBusy(target + 0xB44) &&
        target[refusalByte] == 0) {
        return 0xFF;
    }
    reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(self->vtable[5]))(self, 0);
    return 0x80;
}

void DrawIcon(void* texture, int alpha, float x1, float y1, float x2, float y2) {
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    CRGBA colour;
    RGBA_Construct(&colour, alpha, alpha, alpha, alpha);
    DrawRectUV(x1, y1, x2, y2, texture, &colour, &uv1, &uv2);
}

// the icon above the target (y is its bottom), below, and to the sides
void DrawTop(HUDSocialBase* s, void* texture, int alpha) {
    double d = double(s->iconY) + double(s->y);
    DrawIcon(texture, alpha, s->iconX + s->unknown3C, float(d + 2.0), float(double(s->width) + s->unknown3C + s->iconX),
             float(double(s->height) + d + 2.0));
}
void DrawBottom(HUDSocialBase* s, void* texture, int alpha) {
    DrawIcon(texture, alpha, s->iconX + s->unknown3C, float(double(s->iconY) - s->height - 2.0),
             float(double(s->width) + s->unknown3C + s->iconX), float(double(s->iconY) - 2.0));
}
void DrawRight(HUDSocialBase* s, void* texture, int alpha) {
    float left = *reinterpret_cast<float*>(g_hudFollow + 0x48);
    DrawIcon(texture, alpha, left, s->iconY + s->unknown38, s->width + left,
             float(double(s->height) + s->unknown38 + s->iconY));
}
void DrawLeft(HUDSocialBase* s, void* texture, int alpha) {
    float right = *reinterpret_cast<float*>(g_hudFollow + 0x44);
    DrawIcon(texture, alpha, right - s->width, s->iconY + s->unknown38, right,
             float(double(s->height) + s->unknown38 + s->iconY));
}

// A mission's done or running: passed, flagged 8, or in either slot now
bool MissionDoneOrRunning(const uint32_t* key) {
    int id = MissionMgr_Find(kMissionMgr, key);
    if (*reinterpret_cast<uint16_t*>(MissionMgr_GetStats(kMissionMgr, id) + 2) > 0) {
        return true;
    }
    if (MissionMgr_GetStats(kMissionMgr, id)[4] & 8) {
        return true;
    }
    if (MissionSlot_IsActive(MissionMgr_Primary(kMissionMgr)) && MissionSlot_Is(MissionMgr_Primary(kMissionMgr), id)) {
        return true;
    }
    if (MissionSlot_IsActive(MissionMgr_Secondary(kMissionMgr)) &&
        MissionSlot_Is(MissionMgr_Secondary(kMissionMgr), id)) {
        return true;
    }
    return false;
}

} // namespace

namespace hudsocial {

// 0x5550B0 (HUDSocialApologize's vf8)
void __fastcall Apologize_Draw(HUDSocialBase* self, re::Edx) {
    uint8_t* target = TargetPed();
    if (!target) {
        return;
    }
    uint8_t* player = g_playerPed;
    void* texture;
    if (Social_48E1F0(player + 0xB44)) {
        texture = g_iconTalk;
    } else {
        int32_t state = *reinterpret_cast<int32_t*>(target + 0x1310);
        if ((state == 0 || state == 8 || state == 7) &&
            (!Social_48E070(player + 0xB44) ||
             (!Social_48E1F0(g_playerPed + 0xB44) && *reinterpret_cast<int32_t*>(target + 0x1310) == 7))) {
            texture = g_iconGreet;
        } else {
            texture = g_iconApologizeB;
        }
    }
    bool chasingPlayer = false;
    uint8_t* objective = ObjectiveQueue_GetCurrent(target + 0x5F0);
    if (objective && *reinterpret_cast<int32_t*>(objective + 8) == 7 && Objective_GetTarget(objective) == g_playerPed) {
        chasingPlayer = true;
    }
    int alpha = 0xFF;
    player = g_playerPed;
    bool off = false;
    if (*reinterpret_cast<int32_t*>(player + 0x1440) != 0 || target[0xB4C] != 0 || Social_IsBusy(target + 0xB44) ||
        (chasingPlayer && !Ped_IsStateIdle(target))) {
        off = true;
    } else {
        bool check = true;
        if (!Social_48E2B0(target + 0xB44)) {
            if (Social_Is124(target + 0xB44, g_playerPed)) {
                off = true;
                check = false;
            } else {
                uint8_t* info =
                    reinterpret_cast<uint8_t*(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(target))[0x40 / 4]))(target);
                if (*reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(info + 0x10) + 4) == g_playerPed) {
                    off = true;
                    check = false;
                }
            }
        }
        if (check) {
            player = g_playerPed;
            if (objective && *reinterpret_cast<int32_t*>(objective + 8) == 0x16) {
                off = true;
            } else if (Ped_IsStateIdle(target)) {
                if (*reinterpret_cast<int32_t*>(player + 0x1D40) > *reinterpret_cast<int32_t*>(g_classSkip + 0x30) ||
                    *reinterpret_cast<int32_t*>(player + 0x14AC) != 0 || *reinterpret_cast<int32_t*>(player + 0x14B0) != 0 ||
                    *reinterpret_cast<int32_t*>(player + 0x14B4) != 0 || !Social_48E070(player + 0xB44) ||
                    (!Social_48E1F0(g_playerPed + 0xB44) && *reinterpret_cast<int32_t*>(target + 0x1310) == 7)) {
                    off = true;
                }
            }
        }
    }
    if (off) {
        reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(self->vtable[5]))(self, 0);
        alpha = 0x80;
    }
    DrawTop(self, texture, alpha);
}

// 0x5553A0 (HUDSocialSmoothTalk's vf8)
void __fastcall SmoothTalk_Draw(HUDSocialBase* self, re::Edx) {
    void* texture = g_iconTalk;
    if (uint8_t* target = TargetPed()) {
        DrawTop(self, texture, ActionAlpha(self, target, 0xB4D));
    }
}

// 0x555500 (HUDSocialFlirt's vf8)
void __fastcall Flirt_Draw(HUDSocialBase* self, re::Edx) {
    void* texture = g_iconFlirt;
    if (uint8_t* target = TargetPed()) {
        DrawTop(self, texture, ActionAlpha(self, target, 0xB4E));
    }
}

// 0x555780 (HUDSocialKiss's vf8)
void __fastcall Kiss_Draw(HUDSocialBase* self, re::Edx) {
    void* texture = g_iconKiss;
    if (uint8_t* target = TargetPed()) {
        DrawTop(self, texture, ActionAlpha(self, target, 0xB4F));
    }
}

// 0x5558E0 (HUDSocialFollow's vf8)
void __fastcall Follow_Draw(HUDSocialBase* self, re::Edx) {
    void* texture = g_iconFollow;
    if (uint8_t* target = TargetPed()) {
        DrawLeft(self, texture, ActionAlpha(self, target, 0xB50));
    }
}

// 0x555A30 (HUDSocialTaunt's vf8)
void __fastcall Taunt_Draw(HUDSocialBase* self, re::Edx) {
    void* texture = g_iconTaunt;
    if (uint8_t* target = TargetPed()) {
        DrawRight(self, texture, ActionAlpha(self, target, 0xB51));
    }
}

// 0x555B80 (HUDSocialShove's vf8): shoving from behind (the two facing the same way,
// the target ahead) shows the push in the back (a girl's own icon)
void __fastcall Shove_Draw(HUDSocialBase* self, re::Edx) {
    uint8_t* target = TargetPed();
    if (!target) {
        return;
    }
    uint8_t* player = g_playerPed;
    CVector playerForward, targetForward;
    Placeable_GetForward(player, &playerForward);
    Placeable_GetForward(target, &targetForward);
    float facing = float(double(targetForward.y) * playerForward.y + double(playerForward.x) * targetForward.x +
                         double(targetForward.z) * playerForward.z);
    uint8_t* playerMatrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(player + 0x14);
    const CVector* playerPos = playerMatrix ? reinterpret_cast<CVector*>(playerMatrix + 0x30) : reinterpret_cast<CVector*>(player + 4);
    uint8_t* targetMatrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(target + 0x14);
    const CVector* targetPos = targetMatrix ? reinterpret_cast<CVector*>(targetMatrix + 0x30) : reinterpret_cast<CVector*>(target + 4);
    CVector toTarget;
    VectorSub(&toTarget, targetPos, playerPos);
    void* texture = g_iconShove;
    if (double(facing) > 0.8660253882408142) {
        float ahead = float(double(toTarget.y) * playerForward.y + double(toTarget.x) * playerForward.x +
                            double(toTarget.z) * playerForward.z);
        if (ahead > 0.0f) {
            texture = Ped_GetModelInfo(target)[0x50] ? g_iconShoveBackGirl : g_iconShoveBack;
        }
    }
    DrawRight(self, texture, ActionAlpha(self, target, 0xB52));
}

// 0x555DA0 (HUDSocialHumiliate's vf8)
void __fastcall Humiliate_Draw(HUDSocialBase* self, re::Edx) {
    void* texture = g_iconHumiliate;
    if (uint8_t* target = TargetPed()) {
        DrawRight(self, texture, ActionAlpha(self, target, 0xB53));
    }
}

// 0x555EF0 (HUDSocialStop's vf8)
void __fastcall Stop_Draw(HUDSocialBase* self, re::Edx) {
    void* texture = g_iconStop;
    if (uint8_t* target = TargetPed()) {
        DrawRight(self, texture, ActionAlpha(self, target, 0xB54));
    }
}

// 0x556100 (HUDSocialGift's vf8): the gift the target wants (its icon, from the
// component's table by gift), shown when the player has it; the outfit gift
struct HUDSocialGift : HUDSocialBase {
    int32_t giftIcons[0x19];  // +0x40 an icon index per gift (from +0x48 for gift 2)
};

void __fastcall Gift_Draw(HUDSocialGift* self, re::Edx) {
    int alpha = 0x80;
    uint8_t wearing = Clothing_IsWearingOutfit(kClothing, g_giftItem);
    uint8_t* target = Ped_GetTarget(g_playerPed);
    if (target && (target[0x108] & 7) != 3) {
        target = nullptr;
    }
    void* texture = nullptr;
    int32_t gift = 0;
    auto hide = [self]() { reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(self->vtable[5]))(self, 0); };
    bool near = false;
    if (target) {
        uint8_t* targetMatrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(target + 0x14);
        const CVector* targetPos =
            targetMatrix ? reinterpret_cast<CVector*>(targetMatrix + 0x30) : reinterpret_cast<CVector*>(target + 4);
        uint8_t* player = g_playerPed;
        uint8_t* playerMatrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(player + 0x14);
        const CVector* playerPos =
            playerMatrix ? reinterpret_cast<CVector*>(playerMatrix + 0x30) : reinterpret_cast<CVector*>(player + 4);
        CVector d;
        VectorSub(&d, playerPos, targetPos);
        // (the distance squared kept where the gift goes)
        float distSq = float(double(d.z) * d.z + (double(d.x) * d.x + double(d.y) * d.y));
        std::memcpy(&gift, &distSq, 4);
        if (distSq > 36.0f) {
            target = nullptr;
        } else {
            near = true;
        }
    }
    if (!near) {
        if (!wearing) {
            return;
        }
        if (!target) {
            texture = g_iconGift;
            alpha = 0xFF;
            hide();
            goto draw;
        }
    }
    Ped_GetGift(target, &gift);
    if (wearing && *reinterpret_cast<int32_t*>(target + 0x1310) == 2) {
        texture = g_iconGift;
        alpha = 0xFF;
        hide();
        goto draw;
    }
    if (*reinterpret_cast<int32_t*>(target + 0x1468) == 0 && gift == 0) {
        uint8_t* player = g_playerPed;
        if (!Ped_WantsGift(target) && Social_HasBothSides(target + 0xB44) &&
            *reinterpret_cast<re::Ptr32<uint8_t>*>(target + 0xBB4) == player &&
            (!Social_Is120(target + 0xB44, player) || Social_Is128(target + 0xB44, player)) && Ped_HasItem(player, 2, 1)) {
            gift = 2;
            texture = g_giftIcons[self->giftIcons[2]];
            alpha = 0xFF;
            goto draw;
        }
    }
    if (gift == 0) {
        return;
    }
    if (!Ped_WantsGift(target)) {
        return;
    }
    texture = g_giftIcons[self->giftIcons[gift]];
    if (Ped_HasItem(g_playerPed, gift, 1)) {
        alpha = 0xFF;
        goto draw;
    }
    hide();
draw:
    if (*reinterpret_cast<int32_t*>(g_playerPed + 0x1440) != 0 ||
        (target && (Social_IsBusy(target + 0xB44) || target[0xB55] != 0))) {
        alpha = 0x80;
        hide();
    }
    DrawTop(self, texture, alpha);
}

// 0x556400 (HUDSocialDismiss's vf8)
void __fastcall Dismiss_Draw(HUDSocialBase* self, re::Edx) {
    void* texture = g_iconDismiss;
    if (uint8_t* target = TargetPed()) {
        DrawBottom(self, texture, ActionAlpha(self, target, 0xB56));
    }
}

// 0x556560 (HUDSocialJoin's vf8)
void __fastcall Join_Draw(HUDSocialBase* self, re::Edx) {
    void* texture = g_iconJoin;
    if (uint8_t* target = TargetPed()) {
        DrawBottom(self, texture, ActionAlpha(self, target, 0xB57));
    }
}

// 0x5566C0 (HUDSocialGreet's vf8): the errand icon when the menu flags it (+0x40);
// else a girl's greeting once her mission's done (the plain greeting otherwise)
void __fastcall Greet_Draw(HUDSocialBase* self, re::Edx) {
    uint8_t* errands = g_hudErrands;
    uint8_t* target = Ped_GetTarget(g_playerPed);
    if (!target || (target[0x108] & 7) != 3) {
        return;
    }
    void* texture;
    if (At<uint8_t>(self, 0x40)) {
        texture = g_iconErrands;
    } else {
        int32_t type;
        PedModelInfo_GetType(Ped_GetModelInfo(target), &type);
        bool girl = Ped_GetModelInfo(target)[0x50] != 0 && (type == 1 || type == 3) && !Ped_44E160(target);
        if (girl && target[0x1570] == 1) {
            uint8_t* social = target + 0xB44;
            girl = !Social_48E6B0(social) && !Social_Is124(social, g_playerPed) && !Social_Is120(social, g_playerPed);
        }
        texture = girl && MissionDoneOrRunning(kGreetMission) ? g_iconGreetMission : g_iconGreet;
    }
    int32_t gift;
    Ped_GetGift(target, &gift);
    int alpha;
    if ((gift == 2 || gift == 1 || gift == 0x16 || gift == 0x17 || gift == 0x18 || gift == 0) &&
        *reinterpret_cast<int32_t*>(target + 0x1524) == 0 && *reinterpret_cast<int32_t*>(g_playerPed + 0x1440) == 0 &&
        !Social_IsBusy(target + 0xB44) && target[0xB58] == 0) {
        alpha = 0xFF;
        if (errands[0x49] && Ped_473220(target) && *reinterpret_cast<int32_t*>(target + 0x1E28) <= 0) {
            alpha = 0x80;
        }
    } else {
        reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(self->vtable[5]))(self, 0);
        alpha = 0x80;
    }
    DrawTop(self, texture, alpha);
}

// 0x5569B0 (HUDSocialBurn's vf8): as the right-hand icons, its offset a short
void __fastcall Burn_Draw(HUDSocialBurn* self, re::Edx) {
    void* texture = g_iconBurn;
    uint8_t* target = TargetPed();
    if (!target) {
        return;
    }
    int alpha = ActionAlpha(self, target, 0xB59);
    float left = *reinterpret_cast<float*>(g_hudFollow + 0x48);
    double offset = double(self->unknown38);
    DrawIcon(texture, alpha, left, float(offset + self->iconY), self->width + left,
             float(double(self->height) + offset + self->iconY));
}

// 0x556C30: the errands line: shown (the icon), its text key and number (a new text
// or number flashes it), a flag at +0x48; fading in from nothing
void __fastcall Errands_Set(HUDSocialErrands* self, re::Edx, int shown, const char* key, int flag, float number) {
    uint8_t* raw = reinterpret_cast<uint8_t*>(self);
    raw[0x49] = uint8_t(shown);
    raw[0x48] = uint8_t(flag);
    char* text = reinterpret_cast<char*>(raw + 0x5C);
    if (!key || key[0] == '\0') {
        text[0] = '\0';
    } else if (std::strcmp(text, key) != 0 || !(number == At<float>(raw, 0x44))) {
        std::strncpy(text, key, 0x20);
        At<float>(raw, 0x58) = 1.0f;
    }
    At<float>(raw, 0x44) = number;
    raw[0x4A] = 0;
    At<float>(raw, 0x4C) = 0.0f;
}

// 0x556D00 (HUDSocialErrands' vf8): the line faded in / out (+0x4A in, +0x49 held),
// its colour flashing to white on a change; the icon while shown
void __fastcall Errands_Draw(HUDSocialErrands* self, re::Edx) {
    uint8_t* raw = reinterpret_cast<uint8_t*>(self);
    float& fade = At<float>(raw, 0x4C);
    if (raw[0x4A]) {
        float v = float((1.0 - double(fade)) * 0.30000001192092896 + fade);
        fade = v;
        if (!(v < 1.0f)) {
            fade = 1.0f;
        }
    } else if (raw[0x49]) {
        fade = 1.0f;
    } else {
        float v = float(-double(fade) * 0.30000001192092896 + fade);
        fade = v;
        if (v <= 0.0f) {
            fade = 0.0f;
        }
    }
    if (g_language == 7) {
        g_errandsOffsetX = 15.0f;
        g_errandsOffsetY = 5.0f;
    }
    const char* key = reinterpret_cast<const char*>(raw + 0x5C);
    if (key[0] != '\0') {
        uint32_t colour = At<uint32_t>(raw, 0x38);
        int32_t alphaInt = int32_t(double(fade) * 255.0);
        uint8_t alpha = uint8_t(alphaInt);
        if (alpha > 1) {
            Font_Begin();
            Font_SetJustify(0);
            if (g_language == 7) {
                Font_SetScale(0.4f);
            } else {
                Font_SetSizePreset(1);
            }
            float flash = At<float>(raw, 0x54);
            int r = int(colour & 0xFF);
            int g = int(colour >> 8 & 0xFF);
            int b = int(colour >> 16 & 0xFF);
            uint8_t fr = uint8_t(int32_t(double(0xFF - r) * flash + r));
            uint8_t fg = uint8_t(int32_t(double(0xFF - g) * flash + g));
            uint8_t fb = uint8_t(int32_t(double(0xFF - b) * flash + b));
            float next = float((double(At<float>(raw, 0x58)) - At<float>(raw, 0x54)) * 0.4000000059604645 + At<float>(raw, 0x54));
            At<float>(raw, 0x54) = next;
            float away = std::fabs(float(1.0 - double(next)));
            if (double(away) <= 0.05000000074505806) {
                At<float>(raw, 0x58) = 0.0f;
            }
            g_fontColour = uint32_t(fr) | uint32_t(fg) << 8 | uint32_t(fb) << 16 | uint32_t(alpha) << 24;
            CRGBA drop;
            RGBA_Construct(&drop, 0x10, 0x10, 0x10, alphaInt);
            std::memcpy(&g_fontDropColour, &drop, 4);
            g_fontWrapRight = float(640.0 - double(g_screenScaleX) * 45.0);
            Font_AddNumber(At<float>(raw, 0x44));
            uint16_t line[0x80];
            Font_FormatText(line, Text_Get(kTheText, key), 0x80);
            float y = float(double(self->y) + g_errandsOffsetY);
            Font_PrintString(float(double(self->x) - g_errandsOffsetX), y, line, 1);
            Font_Render();
        }
    }
    void* icon = g_iconErrands;
    if (raw[0x49] && icon) {
        CVector2D uv2 = {1.0f, 1.0f};
        CVector2D uv1 = {0.0f, 0.0f};
        CRGBA white;
        RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
        float offset = g_errandsOffsetX;
        DrawRectUV(float(double(self->x28) - offset), self->y28, float(double(self->width) + self->x28 - offset),
                   float(double(self->height) + self->y28), icon, &white, &uv1, &uv2);
    }
}

// the social menu's components (+0x4C..+0x80)
enum SocialSlot {
    SOCIAL_4C = 0x4C, SOCIAL_50 = 0x50, SOCIAL_54 = 0x54, SOCIAL_58 = 0x58, SOCIAL_5C = 0x5C, SOCIAL_60 = 0x60,
    SOCIAL_64 = 0x64, SOCIAL_68 = 0x68, SOCIAL_6C = 0x6C, SOCIAL_70 = 0x70, SOCIAL_74 = 0x74, SOCIAL_78 = 0x78,
    SOCIAL_7C = 0x7C, SOCIAL_80 = 0x80,
};

HUDComponent* Component(HUDSocial* self, int slot) {
    return At<HUDComponent*>(self, slot);
}
uint8_t ComponentVisible(HUDSocial* self, int slot) {
    HUDComponent* c = Component(self, slot);
    return reinterpret_cast<uint8_t(__thiscall*)(HUDComponent*)>(static_cast<void*>(c->vtable[4]))(c);
}
void ShowComponent(HUDSocial* self, int slot, int visible) {
    HUDComponent* c = Component(self, slot);
    reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(c->vtable[5]))(c, visible);
}

const CVector* EntityPos(uint8_t* entity) {
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x14);
    return matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : reinterpret_cast<const CVector*>(entity + 4);
}

// a button's icon scaled to the menu's height (35%), no bigger than `limit`
float ButtonSize(HUDSocial* self, void* texture, float limit) {
    uint32_t width = reinterpret_cast<uint32_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture))[0x48 / 4]))(texture);
    double scaled = double(width) * (double(self->y) * 0.3499999940395355);
    uint32_t height = reinterpret_cast<uint32_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture))[0x4C / 4]))(texture);
    float size = float(scaled / double(height));
    size = size * g_screenScaleX;
    if (!(limit > size)) {
        size = limit;
    }
    return size;
}

// 0x553D10 (HUDSocial's vf8 Draw): which of the social menu's actions to offer for
// the ped nearest the player (all hidden first, the chosen ones shown), then its
// buttons: the two for the left and right choices (greyed with none), the ones for
// the side actions
void __fastcall Social_Draw(HUDSocial* self, re::Edx) {
    self->unknown40 = 0;
    uint8_t* errands = g_hudErrands;
    uint8_t wasFollowing = ComponentVisible(self, SOCIAL_68) ? 1 : ComponentVisible(self, SOCIAL_64);
    for (int slot = SOCIAL_4C; slot <= SOCIAL_80; slot += 4) {
        ShowComponent(self, slot, 0);
    }
    errands[0x4A] = 0;
    if (Game_4F12F0(reinterpret_cast<void*>(0x00C3CC68)) == 2) {
        return;
    }
    uint8_t* player = g_playerPed;
    if (*reinterpret_cast<int32_t*>(player + 0x1468) == 1 || *reinterpret_cast<int32_t*>(player + 0x153C) == 1 ||
        Ped_4CE690(player)) {
        return;
    }
    player = g_playerPed;
    if (*reinterpret_cast<int32_t*>(player + 0x155C) != 0) {
        return;
    }
    if (Controls_5F4800(*reinterpret_cast<re::Ptr32<uint8_t>*>(player + 0x2E0) + 0x258, reinterpret_cast<void*>(0x01B6542C), 1, 1)) {
        return;
    }
    player = g_playerPed;
    if (*reinterpret_cast<int32_t*>(player + 0x14EC) == 1) {
        if (uint8_t* held = Ped_477C00(g_playerPed, 0x10, 0)) {
            if (double(Distance_472D00(EntityPos(g_playerPed), EntityPos(held))) < 3.239999771118164) {
                uint8_t* p = g_playerPed;
                CVector d;
                VectorSub(&d, EntityPos(held), EntityPos(p));
                CVector forward;
                if (0.0f < Dot_414640(Placeable_GetForward(p, &forward), &d)) {
                    return;
                }
            }
        }
    }
    uint8_t wearing = Clothing_IsWearingOutfit(kClothing, g_giftItem);
    if (wearing && !MissionDoneOrRunning(reinterpret_cast<const uint32_t*>(0x01B65488))) {
        return;
    }
    uint8_t* ped = Ped_46ECE0(g_playerPed);
    float dist = 10000.0f;
    bool menu = false;
    if (ped) {
        dist = Distance_472D00(EntityPos(g_playerPed), EntityPos(ped));
        if (!(36.0f < dist)) {
            int32_t model = int32_t(*reinterpret_cast<int16_t*>(ped + 0x10E));
            if (model != g_poiModel && !Ped_4473E0(ped) && model != g_punishmentModel) {
                menu = true;
            }
        } else {
            ped = nullptr;
        }
    }
    if (!menu) {
        if (wearing == 1 && !ped && Pad_GetValue(10, 0) > 0.0f) {
            uint8_t* target = Ped_GetTarget(g_playerPed);
            if (!target || (Ped_GetTarget(g_playerPed)[0x108] & 7) == 3) {
                ShowComponent(self, SOCIAL_70, 1);
                HUDComponent_53CF50(Component(self, SOCIAL_70));
                goto buttons;
            }
        }
        wearing = 0;
    } else {
        SocialX_490F90(g_playerPed + 0xB44, ped);
        bool chasing = false;
        uint8_t* objective = ObjectiveQueue_GetCurrent(ped + 0x5F0);
        if (objective && *reinterpret_cast<int32_t*>(objective + 8) == 7 && Objective_GetTarget(objective) == g_playerPed) {
            chasing = true;
        }
        bool hisTarget = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xC7C) == g_playerPed;
        int32_t type;
        PedModelInfo_GetType(Ped_GetModelInfo(ped), &type);
        int32_t gift;
        Ped_GetGift(ped, &gift);
        bool busy = false;
        if (gift == 2 || gift == 1 || gift == 0x16 || gift == 0x17 || gift == 0x18 || gift == 0) {
            if (*reinterpret_cast<int32_t*>(ped + 0x1524) != 0) {
                busy = true;
            }
        } else {
            busy = true;
        }
        MissionDoneOrRunning(kGreetMission);  // (not used)
        bool missionDone = MissionDoneOrRunning(reinterpret_cast<const uint32_t*>(0x01B65478));
        int queued = Objective_496B30(ObjectiveQueue_GetCurrent(ped + 0x5F0));
        bool canTalk = !(*reinterpret_cast<int32_t*>(ped + 0x1468) != 0 || Ped_473D60(ped) || ped[0x1C98] != 0 ||
                         Ped_4CE690(ped) || *reinterpret_cast<int32_t*>(ped + 0x155C) != 0 ||
                         *reinterpret_cast<int32_t*>(ped + 0x1348) != 0 || *reinterpret_cast<int32_t*>(ped + 0x153C) != 0 ||
                         queued != 0 || *reinterpret_cast<int32_t*>(ped + 0x1550) != 0);
        uint8_t* social = ped + 0xB44;
        int32_t out;
        int32_t level = *Social_48EBC0(social, &out, g_playerPed, 0, 0, 0);
        // the left choice
        int left = -1;
        bool skipLeft = false;
        if (busy) {
            left = SOCIAL_7C;
        } else if (wearing == 1 && (*reinterpret_cast<int32_t*>(ped + 0x1310) == 2 || Ped_4473E0(ped))) {
            left = SOCIAL_70;
        } else if (Ped_4473E0(ped)) {
            left = SOCIAL_7C;
        } else if (canTalk && Social_48FB40(social) && 1.0f > dist) {
            left = SOCIAL_58;
        } else if (PedFriends_485590(g_playerPed + 0x2F0, ped)) {
            skipLeft = true;
        } else if (chasing) {
            left = SOCIAL_4C;
        } else if (canTalk && gift != 0 && Ped_WantsGift(ped) && !(ped[0xD6A] & 2) && !chasing && (ped[0xD68] & 0x40) &&
                   Ped_HasItem(g_playerPed, gift, 1)) {
            left = SOCIAL_70;
        } else {
            uint8_t* p = g_playerPed;
            bool pair = level <= 2 &&
                        (*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xC64) == p || *reinterpret_cast<re::Ptr32<uint8_t>*>(p + 0xC64) == ped) &&
                        *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xC6C) != p &&
                        (ped[0x1570] == 2 || *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xBB4) == p);
            if (!pair && Ped_IsStateIdle(ped) && *reinterpret_cast<int32_t*>(p + 0x1D40) > 0) {
                pair = !Ped_473220(ped);
                p = g_playerPed;
            }
            if (pair) {
                left = SOCIAL_4C;
            } else if (*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xBB4) == p && *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xC64) == p &&
                       *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xC6C) != p && level <= 6) {
                left = SOCIAL_4C;
            } else {
                bool giftShown = false;
                if (canTalk && gift != 0 && Ped_WantsGift(ped) && !(ped[0xD6A] & 2) && !chasing && !hisTarget &&
                    (*reinterpret_cast<int32_t*>(ped + 0x1428) == 1 ||
                     (*reinterpret_cast<int32_t*>(ped + 0x1E28) > 0 && *reinterpret_cast<int32_t*>(ped + 0x1430) == 1) ||
                     (ped[0xD69] & 2) || (ped[0xD68] & 0x40))) {
                    giftShown = true;
                    if ((gift == 2 || gift == 1) && ped[0x1570] == 1) {
                        uint8_t* pp = g_playerPed;
                        if (Social_48E6B0(social) || !Game_4D9790(g_BF4274, 0)) {
                            giftShown = false;
                        } else {
                            pp = g_playerPed;
                            if (Social_Is124(social, pp)) {
                                giftShown = false;
                            } else if (Social_Is120(social, pp) && !Social_Is128(social, pp)) {
                                giftShown = false;
                            }
                        }
                    }
                }
                if (giftShown) {
                    left = SOCIAL_70;
                } else {
                    bool greet = false;
                    if (Ped_GetModelInfo(ped)[0x50] && (type == 1 || type == 3) && !Ped_44E160(ped) && missionDone &&
                        !chasing && !hisTarget) {
                        greet = true;
                        if (ped[0x1570] == 1) {
                            uint8_t* pp = g_playerPed;
                            if (Social_48E6B0(social) || Social_Is124(social, pp) || Social_Is120(social, pp)) {
                                greet = false;
                            }
                        }
                    }
                    left = greet ? SOCIAL_54 : -2;
                }
            }
        }
        if (left == -2) {
            // the errands (a ped with one): the greeting marks it, the line shown
            HUDComponent* greetComponent = Component(self, SOCIAL_7C);
            if (Ped_473220(ped)) {
                reinterpret_cast<uint8_t*>(greetComponent)[0x40] = 1;
                HUDComponent* e = reinterpret_cast<HUDComponent*>(errands);
                reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(e->vtable[5]))(e, 1);
                errands[0x4A] = 1;
            } else {
                reinterpret_cast<uint8_t*>(greetComponent)[0x40] = 0;
                if (!errands[0x49]) {
                    HUDComponent* e = reinterpret_cast<HUDComponent*>(errands);
                    reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(e->vtable[5]))(e, 0);
                }
            }
            uint8_t* p = g_playerPed;
            left = SOCIAL_7C;
            if (*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xC64) == p && *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xC6C) != p &&
                (gift == 2 || gift == 1 || Ped_474B80(ped)) && ped[0x1570] == 1) {
                left = SOCIAL_4C;
            }
        }
        if (!skipLeft) {
            ShowComponent(self, left, 1);
        }
        // the right choice
        int32_t stat = Ped_477060(ped, 0x3F);
        int right = SOCIAL_60;
        bool skipRight = false;
        if (busy) {
            right = SOCIAL_60;
        } else if (PedFriends_485590(g_playerPed + 0x2F0, ped)) {
            skipRight = true;
        } else if (Ped_4473E0(ped)) {
            right = SOCIAL_60;
        } else {
            bool female = Ped_GetModelInfo(ped)[0x50] != 0;
            uint8_t* p = g_playerPed;
            bool chosen = false;
            if (!female) {
                double health = double(*reinterpret_cast<float*>(ped + 0x1CB8)) / *reinterpret_cast<float*>(ped + 0x1CA4);
                float threshold = float(double(stat) / 100.0);
                bool weak = double(threshold) > health || level == 4;
                if (weak && *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xC7C) != p) {
                    bool inRange;
                    if (wasFollowing == 1) {
                        inRange = !(3.0f < dist);
                    } else if (wasFollowing == 0) {
                        inRange = !(2.46f < dist);
                    } else {
                        inRange = true;
                    }
                    if (inRange && canTalk) {
                        right = SOCIAL_68;
                        chosen = true;
                    }
                }
            }
            if (!chosen) {
                if (!(1.56f < dist) && canTalk) {
                    right = SOCIAL_64;
                } else if (!chasing && *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xBB4) == p && level <= 2 &&
                           (*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0xC64) == p || *reinterpret_cast<re::Ptr32<uint8_t>*>(p + 0xC64) == ped) &&
                           !Ped_IsStateIdle(ped) && Social_48E130(social)) {
                    right = SOCIAL_80;
                } else {
                    right = SOCIAL_60;
                }
            }
        }
        if (!skipRight) {
            ShowComponent(self, right, 1);
        }
        for (int slot = SOCIAL_4C; slot <= SOCIAL_80; slot += 4) {
            HUDComponent_53CF50(Component(self, slot));
        }
        goto check;
    }
check:
    if (!ped || int32_t(*reinterpret_cast<int16_t*>(ped + 0x10E)) == g_poiModel || Ped_4473E0(ped) ||
        int32_t(*reinterpret_cast<int16_t*>(ped + 0x10E)) == g_punishmentModel) {
        if (!wearing) {
            return;
        }
    }
buttons:
    self->unknown40 = 1;
    int leftAlpha = 0xFF;
    int rightAlpha = 0xFF;
    if (!ComponentVisible(self, SOCIAL_4C) && !ComponentVisible(self, SOCIAL_50) && !ComponentVisible(self, SOCIAL_54) &&
        !ComponentVisible(self, SOCIAL_58) && !ComponentVisible(self, SOCIAL_5C) && !ComponentVisible(self, SOCIAL_7C) &&
        !ComponentVisible(self, SOCIAL_70) && !wearing) {
        leftAlpha = 0x80;
    }
    if (!ComponentVisible(self, SOCIAL_60) && !ComponentVisible(self, SOCIAL_64) && !ComponentVisible(self, SOCIAL_68) &&
        !ComponentVisible(self, SOCIAL_6C) && !ComponentVisible(self, SOCIAL_80)) {
        rightAlpha = 0x80;
    }
    if (errands[0x49] && Ped_473220(ped) && *reinterpret_cast<int32_t*>(ped + 0x1E28) <= 0) {
        leftAlpha = 0x80;
    }
    float limit = g_widescreenText ? 50.0f : 64.0f;
    float size7 = ButtonSize(self, GetButtonTexture(7), limit);
    float size9 = ButtonSize(self, GetButtonTexture(9), limit);
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    {
        double top = double(self->iconY) + self->y;
        double cx = double(self->x) * 0.5 + self->iconX;
        double hw = 0.5 * size7;
        CRGBA colour;
        RGBA_Construct(&colour, leftAlpha, leftAlpha, leftAlpha, leftAlpha);
        void* texture = GetButtonTexture(7);
        DrawRectUV(float(cx - hw), float(top - double(self->y) * 0.3499999940395355 + 2.0), float(cx + hw),
                   float(top + 2.0), texture, &colour, &uv1, &uv2);
    }
    {
        uv2 = {1.0f, 1.0f};
        uv1 = {0.0f, 0.0f};
        double cx = double(self->x) * 0.5 + self->iconX;
        double hw = 0.5 * size9;
        CRGBA grey;
        RGBA_Construct(&grey, 0x80, 0x80, 0x80, 0x80);
        void* texture = GetButtonTexture(9);
        double bottom = double(self->y) * 0.3499999940395355 + self->iconY;
        DrawRectUV(float(cx - hw), float(double(self->iconY) - 2.0), float(cx + hw), float(bottom - 2.0), texture, &grey,
                   &uv1, &uv2);
    }
    int sideAlpha = 0x80;
    bool sideShown = false;
    if (ped && Ped_474BF0(g_playerPed)) {
        uint8_t* p = g_playerPed;
        uint8_t* weapon = *reinterpret_cast<re::Ptr32<uint8_t>*>(p + 0x1D0);
        if (int32_t(*reinterpret_cast<int16_t*>(weapon + 0x10E)) == g_BF43F4) {
            sideShown = true;
            if (4.0f > dist) {
                CVector pedForward, playerForward;
                const CVector* a = Placeable_GetForward(ped, &pedForward);
                const CVector* b = Placeable_GetForward(p, &playerForward);
                if (Dot_414640(b, a) > 0.707f) {
                    CVector d;
                    VectorSub(&d, EntityPos(ped), EntityPos(p));
                    CVector n;
                    const CVector* dir = Vector_Normalised_472D40(&n, &d);
                    CVector forward;
                    if (Dot_414640(Placeable_GetForward(p, &forward), dir) > 0.707f) {
                        sideAlpha = 0xFF;
                    }
                }
            }
        }
    }
    void* texture6 = GetButtonTexture(6);
    float size6 = ButtonSize(self, texture6, limit);
    void* texture8 = GetButtonTexture(8);
    float size8 = ButtonSize(self, texture8, limit);
    // (the side icons line up with these: +0x44 the left's right edge, +0x48 the right's left)
    At<float>(self, 0x44) = float(double(self->iconX) + double(self->x) * 0.3499999940395355 - size6);
    At<float>(self, 0x48) = float(double(self->iconX) + self->x - double(self->x) * 0.3499999940395355 + size8);
    {
        uv2 = {1.0f, 1.0f};
        uv1 = {0.0f, 0.0f};
        double cy = double(self->y) * 0.5 + self->iconY;
        double hh = double(self->y) * 0.3499999940395355 * 0.5;
        double cx = double(self->x) * 0.3499999940395355 + self->iconX;
        CRGBA colour;
        RGBA_Construct(&colour, sideAlpha, sideAlpha, sideAlpha, sideAlpha);
        DrawRectUV(float(cx - size6), float(cy - hh), float(cx), float(cy + hh), texture6, &colour, &uv1, &uv2);
    }
    {
        uv2 = {1.0f, 1.0f};
        uv1 = {0.0f, 0.0f};
        double cy = double(self->y) * 0.5 + self->iconY;
        double hh = double(self->y) * 0.3499999940395355 * 0.5;
        double cx = double(self->iconX) + self->x - double(self->x) * 0.3499999940395355;
        CRGBA colour;
        RGBA_Construct(&colour, rightAlpha, rightAlpha, rightAlpha, rightAlpha);
        float x1 = float(cx);
        DrawRectUV(x1, float(cy - hh), float(cx + size8), float(cy + hh), texture8, &colour, &uv1, &uv2);
    }
    if (sideShown) {
        uv2 = {1.0f, 1.0f};
        uv1 = {0.0f, 0.0f};
        double cy = double(self->y) * 0.5 + self->iconY;
        double hh = double(self->y) * 0.3499999940395355 * 0.5;
        CRGBA colour;
        RGBA_Construct(&colour, sideAlpha, sideAlpha, sideAlpha, sideAlpha);
        DrawRectUV(float(double(At<float>(self, 0x44)) - self->width), float(cy - hh), At<float>(self, 0x44), float(cy + hh),
                   g_iconSideAction, &colour, &uv1, &uv2);
    }
}

} // namespace hudsocial

#define HUDSOCIALMENU_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(hudsocialmenu_original_, __LINE__);                                       \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudsocialmenu_original_, __LINE__), \
             re::Mode::Original)

HUDSOCIALMENU_REVERSED("HUDSocial::Draw", 0x00553D10, &hudsocial::Social_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialApologize::Draw", 0x005550B0, &hudsocial::Apologize_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialSmoothTalk::Draw", 0x005553A0, &hudsocial::SmoothTalk_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialFlirt::Draw", 0x00555500, &hudsocial::Flirt_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialKiss::Draw", 0x00555780, &hudsocial::Kiss_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialFollow::Draw", 0x005558E0, &hudsocial::Follow_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialTaunt::Draw", 0x00555A30, &hudsocial::Taunt_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialShove::Draw", 0x00555B80, &hudsocial::Shove_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialHumiliate::Draw", 0x00555DA0, &hudsocial::Humiliate_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialStop::Draw", 0x00555EF0, &hudsocial::Stop_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialGift::Draw", 0x00556100, &hudsocial::Gift_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialDismiss::Draw", 0x00556400, &hudsocial::Dismiss_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialJoin::Draw", 0x00556560, &hudsocial::Join_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialGreet::Draw", 0x005566C0, &hudsocial::Greet_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialBurn::Draw", 0x005569B0, &hudsocial::Burn_Draw);
HUDSOCIALMENU_REVERSED("HUDSocialErrands::Set", 0x00556C30, &hudsocial::Errands_Set);
HUDSOCIALMENU_REVERSED("HUDSocialErrands::Draw", 0x00556D00, &hudsocial::Errands_Draw);
