// The action tree's cinematic tracks (0x683E80..0x6874C0, the conversation manager's
// members among them in Conversation.cpp): the cinematic camera and conversation shots,
// dialog lines (subtitles and speech, queued one ahead), the facial tracks (eyes, lids,
// brows, mouth of a conversation participant), script and mission calls, camera paths
// and fixed camera points, text; their construction tails (after the track base's
// constructor, from TrackFactories.cpp's New) and destructors.
//
// A track: +0 vtable, +8 keeps alive, +0xC start, +0x10 end, +0x18.. its fields (see each).
// The conversation's participants: *0xB9DB20[8] (peds; +0x1CC their facial animation),
// the current speaker's index 0xB9DB40.

#include <cmath>
#include <cstdint>

#include "game/ActionFactories.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
struct lua_State;

namespace {

using vc::At;
using namespace actfac;

struct V3 {
    float x, y, z;
};
static_assert(sizeof(V3) == 0xC);

// The conversation camera's shot (ConvCamParams, 0x18 bytes)
struct ConvShot {
    re::Ptr32<uint8_t> ped1;      // +0
    re::Ptr32<uint8_t> ped2;      // +4
    int32_t shot;       // +8
    uint8_t flagC;      // +C
    uint8_t flagD;      // +D
    uint8_t flagE;      // +E
    uint8_t padF;
    int32_t name;       // +10 (CinematicShot_Find of a name)
    int32_t list;       // +14 (the shot list of a name)
};
static_assert(sizeof(ConvShot) == 0x18);

// A speech line playing (one of two: 0x1B891A0, the current one 0x1B891C0)
struct SpeechSlot {
    uint8_t active;     // +0
    uint8_t pad1[3];
    re::Ptr32<uint8_t> ped;       // +4
    int32_t event;      // +8
    int32_t line;       // +C
};
static_assert(sizeof(SpeechSlot) == 0x10);

GAME_ARRAY(uint8_t*, cinParticipants, 0x00B9DB20);   // [8]
GAME_VAR(int32_t, cinSpeaker, 0x00B9DB40);
GAME_VAR(uint8_t, cinInCutscene, 0x00B9DB1C);
GAME_VAR(uint8_t*, cinCurrentDialog, 0x01B8919C);
GAME_VAR(uint8_t, cinCancel, 0x01B89198);             // the conversation's cancelled
GAME_VAR(int32_t, cinDialogCount, 0x01B891C4);        // dialog lines alive
GAME_ARRAY(SpeechSlot, cinSpeechSlots, 0x01B891A0);  // [2]
GAME_VAR(int32_t, cinSpeechSlot, 0x01B891C0);
GAME_VAR(uint32_t, cinTextFrame, 0x01B891C8);         // the last time a text track drew
GAME_VAR(uint32_t, cinDrawTextFrame, 0x01B891CC);
GAME_VAR(uint32_t, cinDialogTrackKey, 0x01B891E4);    // DialogLineTrack's name key
GAME_VAR(uint32_t, cinTimeMs, 0x00C1A9B4);
GAME_VAR(uint8_t*, cinPlayerPed, 0x00C1AEA8);
GAME_VAR(int32_t, cinGameMode, 0x00C3CCF8);
GAME_VAR(void*, cinNiCamera, 0x00C3CCF4);
GAME_VAR(float, cinWidescreenAmount, 0x00C3CD14);
GAME_VAR(int32_t, cinWidescreenSetting, 0x00C3CD0C);
GAME_VAR(int32_t, cinCamera_C3CE0C, 0x00C3CE0C);
GAME_VAR(int32_t, cinCamera_C3CED8, 0x00C3CED8);
GAME_VAR(uint8_t*, cinCameraTarget, 0x00C3CE14);
GAME_VAR(void*, cinFollowCamera, 0x00C3CE98);
GAME_VAR(uint8_t*, cinConvCam, 0x00C3CFC0);
GAME_VAR(void*, cinCutsceneCam, 0x00C3CFC4);
GAME_VAR(uint8_t*, cinConvSettings, 0x020C88DC);
GAME_VAR(uint8_t*, cinFade, 0x009E4F4C);
GAME_VAR(void*, cinPathStore, 0x00D13CB4);
GAME_VAR(void*, cinHudText, 0x00C67548);
GAME_VAR(uint8_t, cinSocialSubtitles, 0x00A33938);
GAME_VAR(int32_t, cinLastScriptedSpeech, 0x00CF4E00);
GAME_VAR(lua_State*, cinLuaState, 0x00D093B0);
GAME_ARRAY(const char*, cinSpeechNames, 0x00AE74E8);
GAME_VAR(float, cinYawSpeed, 0x00B9DBB0);
GAME_VAR(float, cinPitchSpeed, 0x00B9DBAC);
// the font's state (CFont's details)
GAME_ARRAY(uint8_t, cinFontColour, 0x0208B764);
GAME_VAR(int16_t, cinFontFlags, 0x0208B784);
GAME_VAR(int16_t, cinFontStyle, 0x0208B788);
GAME_ARRAY(int32_t, cinFontStyleLoaded, 0x0208B218);
inline void* const cinTheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const cinCameraTransition = reinterpret_cast<void*>(0x00A45500);
inline void* const cinUserInput = reinterpret_cast<void*>(0x00A45A30);
inline void* const cinPlayers = reinterpret_cast<void*>(0x00C1AEA8);
inline void* const cinMissions = reinterpret_cast<void*>(0x020C3CA0);
inline void* const cinLurStore = reinterpret_cast<void*>(0x00D02850);
inline void* const cinActionTrees = reinterpret_cast<void*>(0x020C7E98);
inline void* const cinSpeech = reinterpret_cast<void*>(0x00CF4378);
inline void* const cinSubtitles = reinterpret_cast<void*>(0x0208D388);
inline void* const cinText = reinterpret_cast<void*>(0x020C2FA4);
inline const uint16_t* const cinMissingText = reinterpret_cast<const uint16_t*>(0x0093F0E8);
inline const char* const cinCutsceneFormat = reinterpret_cast<const char*>(0x0093EBC4);   // "Cuts\%s.DAT"
inline const char* const cinReadMode = reinterpret_cast<const char*>(0x008FF548);          // "r"
inline const char* const cinMissionName = reinterpret_cast<const char*>(0x0093EBDC);       // "mission"
inline const char* const cinNumSpeechEvents = reinterpret_cast<const char*>(0x0091F114);   // "NUM_SPEECH_EVENTS"
inline const uint32_t* const cinShotKeyA = reinterpret_cast<const uint32_t*>(0x01B65350);
inline const uint32_t* const cinShotKeyB = reinterpret_cast<const uint32_t*>(0x01B65358);
inline void* const cinShotRoot = reinterpret_cast<void*>(0x01B65348);
inline void* const cinDialogLineCallback = reinterpret_cast<void*>(0x00686750);
inline void* const cinQueuedLineCallback = reinterpret_cast<void*>(0x006867B0);

GAME_FN(void, __thiscall, Camera_SetShot, 0x004F2C20, void* camera, int shot, int a, int instant, int b, int c);
GAME_FN(uint8_t*, __thiscall, Camera_GetCam, 0x004F3590, void* camera, int mode);
GAME_FN(void, __thiscall, Camera_ResetToPosition, 0x004F2BD0, void* camera, V3 position, int flag);
GAME_FN(V3*, __thiscall, Camera_GetViewDirection, 0x0041AA70, void* camera, V3* out);
GAME_FN(void, __thiscall, ConvCamParams_Construct, 0x004F4DE0, ConvShot* params);
GAME_FN(uint8_t, __thiscall, ConvCamCtrl_Set, 0x004F6D40, uint8_t* ctrl, ConvShot* params, int a, int b, int c);
GAME_FN(uint8_t, __thiscall, ConvCamCtrl_IsActive, 0x004F5740, uint8_t* ctrl);
GAME_FN(uint8_t*, __thiscall, ConvCamCtrl_GetPed, 0x004F56A0, uint8_t* ctrl, int i);
GAME_FN(int32_t, __cdecl, ConvCam_FindShotList, 0x004F56B0, uintptr_t name);
GAME_FN(int32_t*, __thiscall, Transition_GetController, 0x004F0B10, void* transition, int32_t* out, int end);
GAME_FN(int32_t, __thiscall, Transition_IsActive, 0x00856850, void* transition);
GAME_FN(void, __thiscall, CutsceneCam_LoadPath, 0x004F8820, void* controller, uintptr_t file);
GAME_FN(void, __thiscall, CutsceneCam_FreeTracks, 0x004F7AC0, void* controller);
GAME_FN(int32_t, __thiscall, CutsceneCam_IsPlaying, 0x00449720, void* controller);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __cdecl, Camera_SetNearClip, 0x006C8820, void* camera, float plane);
GAME_FN(void, __thiscall, ScriptCam_SetPath, 0x00507970, uint8_t* cam, void* path, int);
GAME_FN(void, __thiscall, ScriptCam_SetSpeed0, 0x00507830, uint8_t* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_SetSpeed1, 0x00507850, uint8_t* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_SetSpeed2, 0x00507870, uint8_t* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_LookAtXYZ, 0x00507700, uint8_t* cam, const V3* pos, int flag);
GAME_FN(void, __thiscall, ScriptCam_LookAt, 0x00507750, uint8_t* cam, int handle, int kind, int flag, float f);
GAME_FN(void, __thiscall, ScriptCam_SetFov, 0x00507890, uint8_t* cam, float fov);
GAME_FN(void, __thiscall, ScriptCam_SetXYZ_x64, 0x00507C60, uint8_t* cam, float, float, float, float, float, float, int a, int b);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void ScriptCam_SetXYZ(uint8_t* a0, V3 a1, V3 a2, int a3, int a4) {
    return ScriptCam_SetXYZ_x64(a0, a1.x, a1.y, a1.z, a2.x, a2.y, a2.z, a3, a4);
}
GAME_FN(float, __thiscall, FollowCam_GetFov, 0x004FBCE0, void* camera);
GAME_FN(void*, __thiscall, PathStore_Get, 0x005ED360, void* store, uintptr_t name);
GAME_FN(V3*, __cdecl, Script_GetPointPosition, 0x005BFE10, V3* out, uintptr_t name);
GAME_FN(uint8_t, __cdecl, Nis_IsSet, 0x005BB380);
GAME_FN(void, __thiscall, PlayerInfo_SetPaused, 0x00445DA0, void* info, int paused);
GAME_FN(void, __thiscall, UserInput_SetEnabled0, 0x0050DFA0, void* input, int enabled);
GAME_FN(void, __thiscall, UserInput_SetFlag1D, 0x0050DFB0, void* input, int flag);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(int, __cdecl, FileMgr_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(void, __cdecl, FileMgr_CloseFile, 0x0042D450, int handle);
GAME_FN(int32_t, __thiscall, MissionMgr_FindMission, 0x006A9FD0, void* mgr, void* key);
GAME_FN(void, __thiscall, MissionMgr_Start, 0x006AD100, void* mgr, int mission, int arg);
GAME_FN(void*, __thiscall, Missions_GetController0, 0x006AA680, void* mgr);
GAME_FN(void*, __thiscall, Missions_GetController1, 0x006AA690, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionSlot_Is, 0x006AA410, void* slot, int id);
GAME_FN(int32_t*, __thiscall, MissionMgr_Current, 0x006AA790, void* mgr);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetMission, 0x006AA660, void* mgr, int32_t index);
GAME_FN(uint8_t*, __thiscall, LurStore_FindScript, 0x005DBCC0, void* store, const char* name);
GAME_FN(uint8_t*, __thiscall, LurStore_LaunchScript, 0x005DC0F0, void* store, const char* name, int mainThread);
GAME_FN(uint8_t*, __thiscall, LurStore_GetScript, 0x005DB8D0, void* store, int index);
GAME_FN(int32_t, __thiscall, Script_CallFunction, 0x005D8200, uint8_t* script, const char* name, int args, int results);
GAME_FN(void, __cdecl, Script_PushInt, 0x005D85C0, lua_State* L, int n);
GAME_FN(void, __cdecl, Script_PushFloat, 0x005D85E0, lua_State* L, float n);
GAME_FN(int, __cdecl, GetPedHandle, 0x005C6230, uint8_t* ped);
GAME_FN(int32_t, __thiscall, ActionTrees_Get, 0x006D7A60, void* trees, void* key);
GAME_FN(void, __thiscall, Door_SetCurrent, 0x004ED940, uint8_t* door);
GAME_FN(void, __thiscall, Ctl_PlayNode, 0x005F45F0, uint8_t* controller, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget, 0x004D5AD0, uint8_t* targeting);
GAME_FN(void, __cdecl, Hud_ShowMessage, 0x00514340, uintptr_t text, float seconds);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, uintptr_t key);
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetStyle, 0x00688050, int style);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __thiscall, HUDText_SetText, 0x0055B130, void* text, int slot, uintptr_t key);
GAME_FN(void, __thiscall, Speech_Stop, 0x005A5AB0, void* speech, uint8_t* ped, int event, int line);
GAME_FN(void, __thiscall, Speech_Interrupt, 0x005A59D0, void* speech, uint8_t* ped, int a, int b);
GAME_FN(void, __thiscall, Speech_Play, 0x005A4E70, void* speech, void* out, uint8_t* ped, int event, int line,
        void* callback, void* user, int a, int b, int c, int d, uintptr_t e);
GAME_FN(uint8_t, __thiscall, Speech_IsPedSayingEvent, 0x005A08F0, void* speech, uint8_t* ped, int event, int line);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(void, __thiscall, Subtitles_Setup, 0x0068F1D0, void* subtitles, const int32_t* rect, int, const float* scale, const uint8_t* colour);
GAME_FN(void, __thiscall, Subtitles_Add, 0x0068F160, void* subtitles, uintptr_t key, int time, int);
GAME_FN(void, __thiscall, Subtitles_SetFlag, 0x0068EFF0, void* subtitles, int flag);
GAME_FN(void, __thiscall, Facial_Fn68E660, 0x0068E660, void* part, int);
GAME_FN(void, __thiscall, EntityRef_Set, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(void, __thiscall, EntityRef_Assign, 0x00472CA0, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(void, __thiscall, Track_Construct, 0x0061A880, uint8_t* track);
GAME_FN(void, __thiscall, Track_Destruct, 0x0061A8B0, uint8_t* track);
GAME_FN(uint32_t*, __thiscall, NameKey_Default, 0x0066BEA0, uint32_t* key);
GAME_FN(uint32_t*, __thiscall, NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, uintptr_t name);
GAME_FN(uint32_t*, __thiscall, TrackDesc_GetKey, 0x005FA9B0, uint8_t* desc, uint32_t* out);
GAME_FN(void, __thiscall, TrackDesc_Apply, 0x005FA940, uint8_t* desc, uint8_t* track);
GAME_FN(void, __thiscall, HashPath_Set, 0x005FA8C0, int32_t* path, int32_t count, uint32_t* hashes);
GAME_FN(int32_t, __cdecl, FindButes, 0x0072AEA0, int32_t* path, void* root);
GAME_FN(float, __cdecl, AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float angle);
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, void* self, const void* other);
GAME_FN(void, __thiscall, Matrix_SetRotate, 0x00412D00, void* m, const V3* angles);
GAME_FN(void, __thiscall, Matrix_Multiply, 0x00412800, void* m, const void* by);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
GAME_FN(V3*, __cdecl, TransformPoint, 0x005E17D0, V3* out, const void* matrix, const V3* v);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x004734C0, uint8_t* ped, float heading);
GAME_FN(void, __thiscall, Ped_UpdateMoveDirection, 0x00474570, uint8_t* ped);
// the participants' facial parts
GAME_FN(void, __thiscall, Eye_LookAway, 0x0068E5A0, void* eyes, float amount);
GAME_FN(void, __thiscall, Eye_SetMode, 0x0068E890, void* eyes, int mode, float a, int time);
GAME_FN(void, __thiscall, Eye_SetLook, 0x0068E540, void* eyes, const V3* look);
GAME_FN(void, __thiscall, Lid_SetMode, 0x0068E5F0, void* lids, int mode, int time);
GAME_FN(void, __thiscall, Brow_SetMode, 0x0068EAD0, void* brows, int mode, int time);
GAME_FN(void, __thiscall, Mouth_SetMode, 0x0068E690, void* mouth, int mode, int time);
// ours, through the game's addresses
GAME_FN(int32_t, __cdecl, CinematicShot_Find, 0x00684000, int32_t name);
GAME_FN(void, __thiscall, Facial_SetLidTarget, 0x00683E80, uint8_t* facial, float target);
GAME_FN(void, __thiscall, DialogLine_Destruct, 0x00684160, uint8_t* self);
GAME_FN(void, __thiscall, DialogLine_QueueNext, 0x00686C50, uint8_t* self);

inline float* PosOf(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m ? reinterpret_cast<float*>(m + 0x30) : reinterpret_cast<float*>(e + 4);
}
inline float& F(uint8_t* p, int offset) {
    return At<float>(p, offset);
}
inline uint8_t* Facial(int participant) {
    return At<uint8_t*>(cinParticipants[participant], 0x1CC);
}
inline void CameraShot(int shot) {
    Camera_SetShot(cinTheCamera, shot, 0, 1, 0, 0);
}
// The facial channels' "go towards": the target kept, the speed signed towards it
inline void SteerChannel(float current, float target, float rate, float& speed) {
    speed = current < target ? rate : -rate;
}

// ---- the facial parts ------------------------------------------------------------------

// 0x683E80 (thiscall, ret 4): the eye lids towards a target (+0xC) at their rate (+0x20)
void __fastcall Facial_SetLidTargetImpl(uint8_t* facial, re::Edx, float target) {
    float rate = F(facial, 0x20);
    F(facial, 0xC) = target;
    SteerChannel(F(facial, 4), target, rate, F(facial, 8));
}

// 0x685500 (thiscall, vf2): FacialEyeTrack::Start: by its mode (+0x18) for the
// participant (+0x28): 1 idle, 2 look at (+0x1C, +0x20), 3 look away (+0x24)
uint8_t __fastcall FacialEye_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    switch (At<int32_t>(self, 0x18)) {   // (the participant's looked up in each case only)
    case 1:
        Eye_SetMode(Facial(At<int32_t>(self, 0x28)) + 0x24, 3, 0.0f, 100000);
        break;
    case 2: {
        Eye_SetMode(Facial(At<int32_t>(self, 0x28)) + 0x24, 4, 0.0f, 100000);
        V3 look{F(self, 0x1C), F(self, 0x20), 0.0f};
        Eye_SetLook(Facial(At<int32_t>(self, 0x28)) + 0x24, &look);
        break;
    }
    case 3:
        Eye_LookAway(Facial(At<int32_t>(self, 0x28)) + 0x24, F(self, 0x24));
        break;
    default:
        break;
    }
    return 1;
}

// 0x6855E0 (thiscall, vf4): FacialEyeTrack::End: a look ended (back to idle)
uint8_t __fastcall FacialEye_End(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x18) == 2) {
        V3 look{0.0f, 0.0f, 0.0f};
        Eye_SetLook(Facial(At<int32_t>(self, 0x28)) + 0x24, &look);
        Eye_SetMode(Facial(At<int32_t>(self, 0x28)) + 0x24, 3, 0.0f, 100000);
    }
    return 1;
}

// the lids opened fully again (mode 0, towards 0 at their rate)
inline void LidsReset(uint8_t* facial) {
    Lid_SetMode(facial + 4, 0, 100000);
    float rate = F(facial, 0x20);
    F(facial, 0xC) = 0.0f;
    SteerChannel(F(facial, 4), 0.0f, rate, F(facial, 8));
}

// 0x685650 (thiscall, vf2): FacialEyeLidTrack::Start: by its mode (+0x18) for the
// participant (+0x20): 1 open, 2 towards +0x1C, 3 the blink time (+0x14), 4 the rate
uint8_t __fastcall FacialLid_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    switch (At<int32_t>(self, 0x18)) {
    case 1:
        LidsReset(Facial(At<int32_t>(self, 0x20)));
        break;
    case 2:
        Lid_SetMode(Facial(At<int32_t>(self, 0x20)) + 4, 2, 100000);
        Facial_SetLidTarget(Facial(At<int32_t>(self, 0x20)), F(self, 0x1C));
        break;
    case 3:
        F(Facial(At<int32_t>(self, 0x20)), 0x14) = F(self, 0x1C);
        break;
    case 4:
        F(Facial(At<int32_t>(self, 0x20)), 0x20) = F(self, 0x1C);
        break;
    default:
        break;
    }
    return 1;
}

// 0x685760 (thiscall, vf4): FacialEyeLidTrack::End: mode 2 ended (the lids open again)
uint8_t __fastcall FacialLid_End(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x18) == 2) {
        LidsReset(Facial(At<int32_t>(self, 0x20)));
    }
    return 1;
}

// 0x6857D0 (thiscall, vf2): FacialEyeBrowTrack::Start: 1 towards +0x1C at the brows' rate,
// 2 the rate, 3 an expression (0..4, +0x1C truncated)
uint8_t __fastcall FacialBrow_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    switch (At<int32_t>(self, 0x18)) {
    case 1: {
        uint8_t* facial = Facial(At<int32_t>(self, 0x20));
        uint8_t* brows = facial + 0x64;
        float rate = F(facial, 0x8C);
        float target = F(self, 0x1C);
        F(brows, 0x24) = target;
        SteerChannel(F(brows, 0x1C), target, rate, F(brows, 0x20));
        break;
    }
    case 2:
        F(Facial(At<int32_t>(self, 0x20)), 0x8C) = F(self, 0x1C);
        break;
    case 3: {
        const int32_t expressions[5] = {0, 1, 2, 3, 4};
        int32_t i = int32_t(F(self, 0x1C));
        if (uint32_t(i) <= 4) {
            Brow_SetMode(Facial(At<int32_t>(self, 0x20)) + 0x64, expressions[i], 100000);
        }
        break;
    }
    default:
        break;
    }
    return 1;
}

// 0x6858C0 (thiscall, vf4): FacialEyeBrowTrack::End: an expression ended (neutral again)
uint8_t __fastcall FacialBrow_End(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x18) == 3) {
        Brow_SetMode(Facial(At<int32_t>(self, 0x20)) + 0x64, 0, 100000);
    }
    return 1;
}

// the mouth towards a target at its rate (+0x5C)
inline void MouthSteer(uint8_t* facial, float target) {
    uint8_t* mouth = facial + 0x4C;
    float rate = F(facial, 0x5C);
    F(mouth, 0xC) = target;
    SteerChannel(F(mouth, 4), target, rate, F(mouth, 8));
}

// 0x6858F0 (thiscall, vf2): FacialMouthTrack::Start: 1 closed, 2 open towards +0x1C, 3 the
// rate (+0x1C, the target kept)
uint8_t __fastcall FacialMouth_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    switch (At<int32_t>(self, 0x18)) {
    case 1:
        Mouth_SetMode(Facial(At<int32_t>(self, 0x20)) + 0x4C, 0, 100000);
        MouthSteer(Facial(At<int32_t>(self, 0x20)), 0.0f);
        break;
    case 2:
        Mouth_SetMode(Facial(At<int32_t>(self, 0x20)) + 0x4C, 2, 100000);
        MouthSteer(Facial(At<int32_t>(self, 0x20)), F(self, 0x1C));
        break;
    case 3: {
        uint8_t* facial = Facial(At<int32_t>(self, 0x20));
        float rate = F(self, 0x1C);
        F(facial, 0x5C) = rate;
        uint8_t* mouth = facial + 0x4C;
        float target = F(mouth, 0xC);
        F(mouth, 0xC) = target;
        SteerChannel(F(mouth, 4), target, rate, F(mouth, 8));
        break;
    }
    default:
        break;
    }
    return 1;
}

// 0x685A30 (thiscall, vf4): FacialMouthTrack::End: an open mouth closed again
uint8_t __fastcall FacialMouth_End(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x18) == 2) {
        Mouth_SetMode(Facial(At<int32_t>(self, 0x20)) + 0x4C, 0, 100000);
        MouthSteer(Facial(At<int32_t>(self, 0x20)), 0.0f);
    }
    return 1;
}

// 0x684730 / 0x684770 (thiscall, vf3): done in a mode (3 / 2)
template <int32_t Mode>
uint8_t __fastcall Facial_ModeIs(uint8_t* self, re::Edx, float) {
    return At<int32_t>(self, 0x18) == Mode;
}

// ---- the cinematic camera ---------------------------------------------------------------

// 0x684000 (cdecl): a cinematic shot's butes by name (under two keys; 0 for no name)
int32_t __cdecl CinematicShot_FindImpl(int32_t name) {
    if (!name) {
        return 0;
    }
    uint32_t keys[3];
    NameKey_Copy(&keys[0], cinShotKeyA);
    NameKey_Copy(&keys[1], cinShotKeyB);
    NameKey_Construct(&keys[2], name);
    int32_t path[2];
    HashPath_Set(path, 3, keys);
    int32_t shot = FindButes(path, cinShotRoot);
    for (int i = 2; i >= 0; i--) {
        NameKey_Destruct(&keys[i]);
    }
    return shot;
}

// 0x684EC0 (thiscall, vf2): CinematicTrack::Start: the letterboxed conversation camera
// (+0x18: its widescreen +0x1A, its shot list +0x20) or the normal one back (+0x19); a
// shot of two participants (+0x1B: +0x24 / +0x28, the shot +0x2C, the named shot +0x1C)
uint8_t __fastcall CinematicTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x30) = owner;
    if (self[0x18]) {
        int32_t widescreen = self[0x1A];
        cinWidescreenAmount = 0.0f;
        cinWidescreenSetting = widescreen;
        CameraShot(10);
        At<int32_t>(cinConvCam, 0x170) = ConvCam_FindShotList(At<int32_t>(self, 0x20));
    } else if (self[0x19]) {
        CameraShot(1);
        cinWidescreenAmount = 0.0f;
        cinWidescreenSetting = 0;
    }
    if (self[0x1B]) {
        ConvShot shot;
        ConvCamParams_Construct(&shot);
        shot.ped1 = cinParticipants[At<int32_t>(self, 0x24)];
        shot.ped2 = cinParticipants[At<int32_t>(self, 0x28)];
        shot.shot = At<int32_t>(self, 0x2C);
        shot.name = CinematicShot_Find(At<int32_t>(self, 0x1C));
        shot.list = ConvCam_FindShotList(At<int32_t>(self, 0x20));
        shot.flagC = 1;
        shot.flagD = 0;
        ConvCamCtrl_Set(cinConvCam, &shot, 0, 1, 0);
    }
    return 1;
}

// 0x685030 (thiscall, vf2): DialogShotTrack::Start: in a conversation camera (or going to
// one), its shot (+0x18 a named shot, +0x1C its list, +0x20 the shot, +0x24..+0x28 flags)
// on the owner's ped and its partner (+0x26, or a named shot with the camera idle); the
// normal camera back where it couldn't be set (false)
uint8_t __fastcall DialogShot_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    bool inConversation = cinGameMode == 10;
    int32_t mode;
    uint8_t toConversation;
    if (*Transition_GetController(cinCameraTransition, &mode, 0) == 10) {
        toConversation = 1;
    } else {
        int32_t other;
        toConversation = *Transition_GetController(cinCameraTransition, &other, 1) == 10;
    }
    bool transition = Transition_IsActive(cinCameraTransition) != 0;
    bool go = inConversation || (transition && toConversation);
    if (At<int32_t>(self, 0x18) == 0 && !go) {
        return 1;
    }
    ConvShot shot;
    ConvCamParams_Construct(&shot);
    shot.name = CinematicShot_Find(At<int32_t>(self, 0x18));
    shot.flagC = self[0x24];
    shot.flagD = self[0x25];
    shot.shot = At<int32_t>(self, 0x20);
    shot.list = ConvCam_FindShotList(At<int32_t>(self, 0x1C));
    shot.flagE = self[0x28];
    bool useOwner = !ConvCamCtrl_IsActive(cinConvCam) && shot.name != 0;
    if (self[0x26] || useOwner) {
        uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x2C), 8);
        shot.ped1 = ped;
        if (At<uint8_t*>(ped, 0x155C)) {
            shot.ped2 = At<uint8_t*>(ped, 0x155C);
        } else {
            shot.ped2 = Targeting_GetTarget(At<uint8_t*>(ped, 0x1C8));
            if (!shot.ped2) {
                shot.ped2 = At<uint8_t*>(At<uint8_t*>(self, 0x2C), 8);
            }
        }
    }
    if (self[0x29] && shot.name == 0) {
        cinCamera_C3CE0C = 1;
        cinCamera_C3CED8 = 0;
    }
    if (!ConvCamCtrl_Set(cinConvCam, &shot, 1, self[0x27], 1)) {
        cinCamera_C3CE0C = 1;
        cinCamera_C3CED8 = 0;
        CameraShot(1);
        float* m = GetLocalTransform(GetCameraNode(cinNiCamera));
        Camera_ResetToPosition(cinTheCamera, V3{m[9], m[10], m[11]}, 1);
        return 0;
    }
    if (self[0x29] && shot.name != 0) {
        cinCamera_C3CE0C = 0;
        cinCamera_C3CED8 = 0;
    }
    return 1;
}

// ---- dialog lines -----------------------------------------------------------------------

// 0x684120 (thiscall): DialogLineTrack's constructor (counted while alive)
uint8_t* __fastcall DialogLine_Construct(uint8_t* self, re::Edx) {
    Track_Construct(self);
    At<uint32_t>(self, 0) = 0x0093EBB0;
    NameKey_Default(&At<uint32_t>(self, 0x20));
    NameKey_Default(&At<uint32_t>(self, 0x24));
    NameKey_Default(&At<uint32_t>(self, 0x38));
    At<uint8_t*>(self, 0x64) = nullptr;
    cinDialogCount++;
    return self;
}

// 0x684160 (thiscall): its destructor (the conversation's cancel cleared with the last)
void __fastcall DialogLine_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x0093EBB0;
    if (--cinDialogCount == 0) {
        cinCancel = 0;
    }
    EntityRef_Set(&At<uint8_t*>(self, 0x64), nullptr);
    actfac::NameKey_Destruct(self + 0x38);
    actfac::NameKey_Destruct(self + 0x24);
    actfac::NameKey_Destruct(self + 0x20);
    Track_Destruct(self);
}

// 0x684FD0 (thiscall, vf1): its deleting destructor
uint8_t* __fastcall DialogLine_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = 0x0093EBB0;
    if (--cinDialogCount == 0) {
        cinCancel = 0;
    }
    EntityRef_Set(&At<uint8_t*>(self, 0x64), nullptr);
    actfac::NameKey_Destruct(self + 0x38);
    actfac::NameKey_Destruct(self + 0x24);
    actfac::NameKey_Destruct(self + 0x20);
    Track_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// the speaker's ped by the line's speaker index (8: the owner's ped, 9: its partner)
inline uint8_t* SpeakerOf(int32_t speaker, uint8_t* owner) {
    if (speaker == 9) {
        return At<uint8_t*>(At<uint8_t*>(owner, 8), 0x155C);
    }
    return cinParticipants[speaker];
}
// the speech event's index by name, -1 when none ("NUM_SPEECH_EVENTS" ends the table)
inline bool FindSpeechEvent(const char* name, int32_t& index) {
    if (Stricmp(cinNumSpeechEvents, cinSpeechNames[0]) == 0) {
        return false;
    }
    for (;;) {
        if (Stricmp(name, cinSpeechNames[index]) == 0) {
            return true;
        }
        index++;
        if (Stricmp(cinNumSpeechEvents, cinSpeechNames[index]) == 0) {
            return false;
        }
    }
}
// the line's end time: now plus seconds (the unsigned time converted exactly, the sum
// rounded once)
inline int32_t EndTime(float seconds) {
    float ms = seconds * 1000.0f;
    return int32_t(float(double(ms) + double(cinTimeMs)));
}

// 0x686900 (thiscall, vf2): DialogLineTrack::Start: the current line; its speaker the
// current participant (+0x30 below 8: +0x2D sets it, +0x2C swaps); its subtitle (+0x20,
// +0x35) and speech (+0x24 the event, +0x28 the line, +0x36; to the speaker, +0x64) with
// the ends (+0x50, +0x54 2 s); a conversation shot (+0x38 its name, +0x3C the
// participants', +0x40 the shot); +0x58 the skip button not held. False without either.
uint8_t __fastcall DialogLine_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x48) = owner;
    cinCurrentDialog = self;
    int32_t speaker = At<int32_t>(self, 0x30);
    self[0x4C] = 1;
    self[0x5B] = 1;
    self[0x5A] = 0;
    if (speaker < 8) {
        if (self[0x2D]) {
            cinSpeaker = speaker;
        } else if (self[0x2C]) {
            cinSpeaker = cinSpeaker == 0;
        }
    }
    int32_t key = At<int32_t>(self, 0x20);
    if (key == 0 || Text_Get(cinText, key) == cinMissingText) {
        self[0x35] = 0;
        if (!self[0x36]) {
            return 0;
        }
    }
    const char* eventName = At<const char*>(self, 0x24);
    At<int32_t>(self, 0x54) = 2000;
    if (self[0x36]) {
        bool playing = false;
        if (eventName) {
            At<int32_t>(self, 0x60) = At<int32_t>(self, 0x28);
            At<int32_t>(self, 0x5C) = 0;
            if (FindSpeechEvent(eventName, At<int32_t>(self, 0x5C))) {
                re::Ptr32<uint8_t>* speakerRef = &At<uint8_t*>(self, 0x64);
                self[0x59] = 0;
                EntityRef_Set(speakerRef, nullptr);
                if (speaker == 8) {
                    EntityRef_Set(speakerRef, At<uint8_t*>(owner, 8));
                } else {
                    EntityRef_Assign(speakerRef, SpeakerOf(speaker, owner));
                }
                uint8_t* ped = *speakerRef;
                if (ped) {
                    playing = true;
                    SpeechSlot& slot = cinSpeechSlots[cinSpeechSlot];
                    if (!slot.active || ped != slot.ped || At<int32_t>(self, 0x5C) != slot.event ||
                        At<int32_t>(self, 0x60) != slot.line) {
                        uint8_t out[0x30];
                        Speech_Play(cinSpeech, out, ped, At<int32_t>(self, 0x5C), At<int32_t>(self, 0x60),
                                    cinDialogLineCallback, self, 0, self[0x46], 0, -1, 0);
                        if (cinLastScriptedSpeech != 0) {
                            self[0x36] = 0;
                            actfac::NameKey_Destruct(cinSpeech);   // (an empty call on the speech manager)
                        }
                    }
                }
            }
        }
        if (!playing) {
            self[0x36] = 0;
        }
    }
    if (!self[0x36]) {
        if (!self[0x35]) {
            return 0;
        }
        self[0x36] = 0;
        uint8_t* settings = cinConvSettings;
        int32_t rect[4] = {At<int32_t>(settings, 0x18), At<int32_t>(settings, 0x1C),
                           At<int32_t>(settings, 0x20) - At<int32_t>(settings, 0x18),
                           At<int32_t>(settings, 0x24) - At<int32_t>(settings, 0x1C)};
        float scale[3] = {F(settings, 0x28), F(settings, 0x2C), 0.0f};
        uint8_t colour[4];
        RGBA_Construct(colour, settings[0x30], settings[0x34], settings[0x38], settings[0x3C]);
        Subtitles_Setup(cinSubtitles, rect, 0, scale, colour);
        Subtitles_Add(cinSubtitles, key, At<int32_t>(self, 0x54), 0);
        self[0x59] = 1;
    }
    At<int32_t>(self, 0x50) = EndTime(F(self, 0x18));
    ConvShot shot;
    ConvCamParams_Construct(&shot);
    int32_t shotName = At<int32_t>(self, 0x38);
    bool named = false;
    if (shotName) {
        shot.name = CinematicShot_Find(shotName);
        named = true;
    }
    bool set = false;
    if (self[0x3C]) {
        if (shot.name == 0) {
            shot.name = At<int32_t>(cinConvCam, 0x28);
        }
        shot.ped2 = ConvCamCtrl_GetPed(cinConvCam, 0);
        shot.ped1 = ConvCamCtrl_GetPed(cinConvCam, 1);
        set = true;
    } else if (named) {
        set = true;
    }
    if (set) {
        shot.shot = At<int32_t>(self, 0x40);
        ConvCamCtrl_Set(cinConvCam, &shot, 0, 1, 0);
    }
    self[0x58] = 1;
    if (!(Pad_GetValue(7, 0) == 0.0f)) {
        self[0x58] = 0;
    }
    return 1;
}

// 0x686750 (cdecl; missing from Ghidra's list: the line's speech callback): the line's
// time (+0x50) runs out; the first call after it sets the hold (+0x1C seconds) instead
uint8_t __cdecl DialogLine_SpeechDone() {
    uint8_t* line = cinCurrentDialog;
    if (!line) {
        return 1;
    }
    if (int32_t(cinTimeMs) < At<int32_t>(line, 0x50)) {
        return 0;
    }
    if (line[0x59]) {
        At<uint32_t>(line, 0x50) = cinTimeMs;
        return 1;
    }
    line[0x59] = 1;
    At<int32_t>(line, 0x50) = EndTime(F(line, 0x1C));
    return 0;
}

// the queued speech stopped (its slot freed)
inline uint8_t StopQueuedSpeech() {
    SpeechSlot& slot = cinSpeechSlots[cinSpeechSlot];
    Speech_Interrupt(cinSpeech, slot.ped, 4, 0x19);
    cinSpeechSlots[cinSpeechSlot].active = 0;
    return 0;
}

// 0x6867B0 (cdecl; missing from Ghidra's list: the queued line's speech callback): while
// the current line is the queued one, as 0x686750; else the queued speech stopped
uint8_t __cdecl DialogLine_QueuedSpeechDone() {
    uint8_t* line = cinCurrentDialog;
    if (!line) {
        return 0;
    }
    if (!line[0x4C]) {
        line[0x5B] = 1;
        return 0;
    }
    SpeechSlot& slot = cinSpeechSlots[cinSpeechSlot];
    if (At<int32_t>(line, 0x28) != slot.line || At<uint8_t*>(line, 0x64) != slot.ped) {
        return StopQueuedSpeech();
    }
    const char* name = At<const char*>(line, 0x24);
    int32_t index = 0;
    if (!FindSpeechEvent(name, index) || index != cinSpeechSlots[cinSpeechSlot].event) {
        return StopQueuedSpeech();
    }
    line = cinCurrentDialog;
    if (int32_t(cinTimeMs) < At<int32_t>(line, 0x50)) {
        return 0;
    }
    if (!line[0x59]) {
        line[0x59] = 1;
        At<int32_t>(line, 0x50) = EndTime(F(line, 0x1C));
        return 0;
    }
    At<uint32_t>(line, 0x50) = cinTimeMs;
    cinSpeechSlots[cinSpeechSlot].active = 0;
    return 1;
}

// 0x686C50 (thiscall): the next line queued: when the owner's node has one child, that
// child's first DialogLineTrack (built on the stack from its description) has its speech
// started now into the other slot
void __fastcall DialogLine_QueueNextImpl(uint8_t* self, re::Edx) {
    if (!self[0x45] || At<int32_t>(self, 0x30) >= 8) {
        return;
    }
    uint8_t* node = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x48), 0), 0x10);
    if (vc::Call<int32_t>(node, 0x2C) != 1) {
        return;
    }
    uint8_t* child = vc::Call<uint8_t*>(node, 0x30, 0);
    uint8_t* playable = vc::Call<uint8_t*>(child, 0x4C, At<uint8_t*>(self, 0x48));
    if (!playable) {
        return;
    }
    int32_t count = vc::Call<int32_t>(playable, 0x40);
    uint8_t* desc = nullptr;
    for (int32_t i = 0; i < count; i++) {
        desc = vc::Call<uint8_t*>(playable, 0x48, i);
        uint32_t key;
        bool other = *TrackDesc_GetKey(desc, &key) != cinDialogTrackKey;
        actfac::NameKey_Destruct(&key);
        if (!other) {
            break;
        }
        if (i + 1 >= count) {
            return;
        }
    }
    if (count <= 0) {
        return;
    }
    alignas(4) uint8_t line[0x68];
    Track_Construct(line);
    At<uint32_t>(line, 0) = 0x0093EBB0;
    NameKey_Default(&At<uint32_t>(line, 0x20));
    NameKey_Default(&At<uint32_t>(line, 0x24));
    NameKey_Default(&At<uint32_t>(line, 0x38));
    cinDialogCount++;
    uint32_t name;
    At<uint8_t*>(line, 0x64) = nullptr;
    actfac::NameKey_FromValue(&name, 0);
    actfac::StringRef_Assign(&At<uint32_t>(line, 0x24), name);
    actfac::NameKey_Destruct(&name);
    TrackDesc_Apply(desc, line);
    const char* eventName = At<const char*>(line, 0x24);
    int32_t lineId = At<int32_t>(line, 0x28);
    int32_t event = 0;
    if (line[0x36] && eventName && FindSpeechEvent(eventName, event)) {
        int32_t speaker = At<int32_t>(line, 0x30);
        uint8_t* ped;
        if (speaker == 8) {
            ped = At<uint8_t*>(At<uint8_t*>(self, 0x48), 8);
        } else if (At<int32_t>(self, 0x30) == 9) {
            ped = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x48), 8), 0x155C);   // (the current line's 9)
        } else {
            ped = cinParticipants[speaker];
        }
        if (ped) {
            uint8_t out[0x30];
            Speech_Play(cinSpeech, out, ped, event, lineId, cinQueuedLineCallback, reinterpret_cast<void*>(cinSpeechSlot == 0),
                        1, self[0x46], 0, -1, 0);
            if (cinLastScriptedSpeech == 0) {
                self[0x5B] = 0;
                cinSpeechSlot = cinSpeechSlot == 0;
                SpeechSlot& slot = cinSpeechSlots[cinSpeechSlot];
                slot.active = 1;
                slot.ped = ped;
                slot.event = event;
                slot.line = lineId;
            }
        }
    }
    DialogLine_Destruct(line);
}

// 0x686EC0 (thiscall, vf3): DialogLineTrack::Update: the two participants facing each
// other (+0x34); the next line queued once; running until its time and speech are over
uint8_t __fastcall DialogLine_Update(uint8_t* self, re::Edx, float) {
    if (!self[0x36] && !self[0x35]) {
        return 0;
    }
    if (cinCancel) {
        return 0;
    }
    if (self[0x34] && At<int32_t>(self, 0x30) < 8) {
        uint8_t* b = cinParticipants[1];
        uint8_t* a = cinParticipants[0];
        float* pb = PosOf(b);
        float* pa = PosOf(a);
        float angle = AngleBetweenPoints(pa[0], pa[1], pb[0], pb[1]);
        float headingA = LimitAngle(angle + 3.1415927410125732f);
        if (At<uint8_t*>(a, 0x14)) {
            Matrix_SetRotateZOnly(At<uint8_t*>(a, 0x14), headingA);
        }
        F(a, 0x10) = headingA;
        Ped_SetHeading(a, headingA);
        Ped_UpdateMoveDirection(a);
        float headingB = LimitAngle(angle);
        if (At<uint8_t*>(b, 0x14)) {
            Matrix_SetRotateZOnly(At<uint8_t*>(b, 0x14), headingB);
        }
        F(b, 0x10) = headingB;
        Ped_SetHeading(b, headingB);
        Ped_UpdateMoveDirection(b);
    }
    if (self[0x4C] && !cinSpeechSlots[cinSpeechSlot].active) {
        self[0x4C] = 0;
        DialogLine_QueueNext(self);
    }
    if (!self[0x59] || int32_t(cinTimeMs) < At<int32_t>(self, 0x50)) {
        return 1;
    }
    uint32_t limit = At<uint32_t>(self, 0x54) + At<uint32_t>(self, 0x50);
    bool done = false;
    if (self[0x36]) {
        if (cinTimeMs > limit) {
            uint8_t* ped = At<uint8_t*>(self, 0x64);
            if (!ped || !Speech_IsPedSayingEvent(cinSpeech, ped, At<int32_t>(self, 0x5C), At<int32_t>(self, 0x60))) {
                done = true;
            }
        }
        return !done;
    }
    if (cinTimeMs > limit || !self[0x35]) {
        done = true;
    }
    return !done;
}

// 0x6841B0 (thiscall, vf4): DialogLineTrack::End: no longer current; its speech stopped;
// the speaker's mouth (+0x34) and the subtitles off; then (not cancelled, +0x44, a
// participant) the owner's node's only child played on, or when cancelled (or +0x5A) the
// queued speech interrupted
uint8_t __fastcall DialogLine_End(uint8_t* self, re::Edx) {
    if (cinCurrentDialog == self) {
        cinCurrentDialog = nullptr;
    }
    if (At<uint8_t*>(self, 0x64)) {
        Speech_Stop(cinSpeech, At<uint8_t*>(self, 0x64), At<int32_t>(self, 0x5C), At<int32_t>(self, 0x60));
    }
    if (self[0x34] && At<int32_t>(self, 0x30) < 8) {
        uint8_t* facial = Facial(cinSpeaker);
        if (facial) {
            Facial_Fn68E660(facial + 0x4C, 0);
        }
    }
    Subtitles_SetFlag(cinSubtitles, 0);
    bool interrupt;
    if (self[0x5A] == 0) {
        if (cinCancel == 0) {
            if (!self[0x44] || At<int32_t>(self, 0x30) >= 8) {
                return 1;
            }
            uint8_t* controller = At<uint8_t*>(At<uint8_t*>(self, 0x48), 0);
            uint8_t* node = At<uint8_t*>(controller, 0x10);
            if (vc::Call<int32_t>(node, 0x2C) != 1) {
                return 1;
            }
            uint8_t* child = vc::Call<uint8_t*>(node, 0x30, 0);
            uint8_t* playable = vc::Call<uint8_t*>(child, 0x4C, At<uint8_t*>(self, 0x48));
            if (playable) {
                Ctl_PlayNode(controller, playable);
            }
            return 1;
        }
        interrupt = true;
    } else {
        interrupt = cinCancel != 0;
    }
    if (!interrupt) {
        return 1;
    }
    SpeechSlot& slot = cinSpeechSlots[cinSpeechSlot];
    if (!slot.active) {
        return 1;
    }
    Speech_Interrupt(cinSpeech, slot.ped, 4, 0x19);
    cinSpeechSlots[cinSpeechSlot].active = 0;
    return 1;
}

// 0x6842C0 (cdecl): the conversation cancelled (while lines are alive)
void __cdecl Conversation_Cancel() {
    if (cinDialogCount > 0) {
        cinCancel = 1;
    }
}

// 0x6842E0 (thiscall, vf2): CancelConversationTrack::Start
uint8_t __fastcall CancelConversation_Start(uint8_t*, re::Edx, uint8_t*) {
    if (cinDialogCount > 0) {
        cinCancel = 1;
    }
    return 1;
}

// ---- script and mission calls -----------------------------------------------------------

// 0x6845C0 (thiscall, vf2): ScriptTrack::Start
uint8_t __fastcall ScriptTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x30) = owner;
    At<int32_t>(self, 0x3C) = 0;
    return 1;
}

// 0x6845E0 (thiscall, vf3): ScriptTrack::Update: one frame
uint8_t __fastcall ScriptTrack_Update(uint8_t* self, re::Edx, float) {
    if (At<int32_t>(self, 0x3C) == 1) {
        return 0;
    }
    At<int32_t>(self, 0x3C)++;
    return 1;
}

// the controller a script track's owner plays on: its ped's, its prop's, or its own
inline uint8_t* OwnerController(uint8_t* owner) {
    if (At<uint8_t*>(owner, 8)) {
        return At<uint8_t*>(At<uint8_t*>(owner, 8), 0x2E0) + 0x258;
    }
    uint8_t* prop = At<uint8_t*>(owner, 0xC);
    if (prop) {
        Door_SetCurrent(prop);
        return prop + 0x128;
    }
    return At<uint8_t*>(owner, 0);
}

// 0x685300 (thiscall, vf4): ScriptTrack::End: a script function (+0x28) called in its
// script (+0x24: "mission" the current mission's, else by name, launched when not
// running; else the first script): with arguments (+0x18: the ped's handle (+0x20) or
// the owner's action tree (+0x21 == 1), then +0x1C), or without, its result (+0x2C) the
// index of the owner's node's child to play (false when out of range)
uint8_t __fastcall ScriptTrack_End(uint8_t* self, re::Edx) {
    const char* function = At<const char*>(self, 0x28);
    const char* scriptName = At<const char*>(self, 0x24);
    if (!function && !scriptName) {
        return 1;
    }
    At<uint8_t*>(self, 0x38) = nullptr;
    bool isMission = true;
    for (int i = 0; i < 8; i++) {
        if (scriptName[i] != cinMissionName[i]) {
            isMission = false;
            break;
        }
    }
    if (isMission) {
        int32_t current = *MissionMgr_Current(cinMissions);
        if (current == -1) {
            return 1;
        }
        At<uint8_t*>(self, 0x38) = LurStore_FindScript(cinLurStore, At<const char*>(MissionMgr_GetMission(cinMissions, current), 0x1C));
    } else {
        At<uint8_t*>(self, 0x38) = LurStore_FindScript(cinLurStore, scriptName);
        if (!At<uint8_t*>(self, 0x38)) {
            At<uint8_t*>(self, 0x38) = LurStore_LaunchScript(cinLurStore, scriptName, 0);
        }
    }
    if (!At<uint8_t*>(self, 0x38)) {
        At<uint8_t*>(self, 0x38) = LurStore_GetScript(cinLurStore, 0);
    }
    uint8_t* script = At<uint8_t*>(self, 0x38);
    if (!function || !script) {
        return 1;
    }
    if (self[0x18]) {
        int args = 1;
        if (self[0x20]) {
            Script_PushInt(cinLuaState, GetPedHandle(At<uint8_t*>(At<uint8_t*>(self, 0x30), 8)));
            args = 2;
        } else if (self[0x21] == 1) {
            uint8_t* owner = At<uint8_t*>(self, 0x30);
            int32_t tree = -1;
            if (At<uint8_t*>(owner, 0xC)) {
                void* key = At<void*>(At<uint8_t*>(owner, 0xC), 0x23C);
                if (key) {
                    tree = ActionTrees_Get(cinActionTrees, key);
                }
            } else if (At<uint8_t*>(owner, 8) && At<uint8_t*>(At<uint8_t*>(owner, 8), 0x548)) {
                void* key = At<void*>(At<uint8_t*>(At<uint8_t*>(owner, 8), 0x548), 0x23C);
                if (key) {
                    tree = ActionTrees_Get(cinActionTrees, key);
                }
            }
            Script_PushInt(cinLuaState, tree);
            args = 2;
        }
        Script_PushFloat(cinLuaState, F(self, 0x1C));
        Script_CallFunction(script, function, args, 1);
        return 1;
    }
    if (!self[0x2C]) {
        Script_CallFunction(script, function, 0, 1);
        return 1;
    }
    int32_t index = Script_CallFunction(script, function, 0, 1);
    uint8_t* controller = OwnerController(At<uint8_t*>(self, 0x30));
    uint8_t* node = At<uint8_t*>(controller, 0x10);
    int32_t count = vc::Call<int32_t>(node, 0x2C);
    if (index < 0 || index >= count) {
        return 0;
    }
    uint8_t* child = vc::Call<uint8_t*>(node, 0x30, index);
    uint8_t* playable = vc::Call<uint8_t*>(child, 0x4C, At<uint8_t*>(self, 0x30));
    if (playable) {
        Ctl_PlayNode(controller, playable);
    }
    return 1;
}

// 0x684640 (thiscall, vf2): MissionTrack::Start: its mission (+0x18) started (+0x1C), its
// index kept (+0x24); false without one
uint8_t __fastcall MissionTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    if (At<int32_t>(self, 0x18) == 0) {
        return 0;
    }
    int32_t mission = MissionMgr_FindMission(cinMissions, self + 0x18);
    if (mission != -1) {
        MissionMgr_Start(cinMissions, mission, self[0x1C]);
        At<int32_t>(self, 0x24) = mission;
    }
    return 1;
}

// 0x684690 (thiscall, vf3): MissionTrack::Update: running while either controller runs it
uint32_t __fastcall MissionTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t first = MissionSlot_Is(Missions_GetController0(cinMissions), At<int32_t>(self, 0x24));
    uint8_t second = MissionSlot_Is(Missions_GetController1(cinMissions), At<int32_t>(self, 0x24));
    return (first || second) ? 1 : 0;
}

// ---- cameras ----------------------------------------------------------------------------

// 0x684830 (thiscall, vf2): CameraPathTrack::Start (+0x18): the player paused, a script
// camera along a path (+0x1C) looking at a point (+0x20), letterboxed
uint8_t __fastcall CameraPath_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    if (self[0x18]) {
        PlayerInfo_SetPaused(cinPlayers, 0);
        UserInput_SetEnabled0(cinUserInput, 0);
        void* path = PathStore_Get(cinPathStore, At<int32_t>(self, 0x1C));
        uint8_t* cam = Camera_GetCam(cinTheCamera, 4);
        ScriptCam_SetPath(cam, path, 1);
        ScriptCam_SetSpeed0(cam, 1.2000000476837158f);
        ScriptCam_SetSpeed1(cam, 1.0f);
        ScriptCam_SetSpeed2(cam, 1.0f);
        cinWidescreenAmount = 0.0f;
        cinWidescreenSetting = 1;
        V3 point;
        Script_GetPointPosition(&point, At<int32_t>(self, 0x20));
        ScriptCam_LookAtXYZ(cam, &point, 1);
    }
    return 1;
}

// 0x684900 (thiscall, vf4): CameraPathTrack::End (+0x19): the normal camera and input back
uint8_t __fastcall CameraPath_End(uint8_t* self, re::Edx) {
    if (self[0x19]) {
        cinWidescreenAmount = 0.0f;
        cinWidescreenSetting = 0;
        CameraShot(1);
        UserInput_SetEnabled0(cinUserInput, 1);
    }
    return 1;
}

// 0x6843F0 (thiscall, vf2): CameraCutscenePathTrack::Start (+0x1C): the cutscene camera's
// path loaded from Cuts\<+0x18>.DAT; the player paused (+0x20: hidden too)
uint8_t __fastcall CutscenePath_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    if (self[0x1C] && At<const char*>(self, 0x18)) {
        char path[128];
        Sprintf(path, cinCutsceneFormat, At<const char*>(self, 0x18));
        int file = FileMgr_OpenFile(path, cinReadMode, 1);
        CutsceneCam_LoadPath(cinCutsceneCam, file);
        FileMgr_CloseFile(file);
        CameraShot(3);
        uint8_t* player = cinPlayerPed;
        cinWidescreenAmount = 0.0f;
        cinWidescreenSetting = 1;
        UserInput_SetFlag1D(cinUserInput, 0);
        PlayerInfo_SetPaused(cinPlayers, 1);
        if (self[0x20]) {
            At<int32_t>(player, 0x48) = 0;
            At<int32_t>(player, 0xD0) = 1;
        }
    }
    return 1;
}

// 0x685230 (thiscall, vf3): CameraCutscenePathTrack::Update: while the cutscene camera
// plays; the player kept at the camera less its view direction (+0x1F); its near plane
// (+0x24, unless -1)
uint8_t __fastcall CutscenePath_Update(uint8_t* self, re::Edx, float) {
    if (cinGameMode != 3) {
        return 0;
    }
    if (CutsceneCam_IsPlaying(cinCutsceneCam) != 0) {
        return 0;
    }
    if (self[0x1F]) {
        float* m = GetLocalTransform(GetCameraNode(cinNiCamera));
        float* pos = m + 9;
        V3 view;
        V3* dir = Camera_GetViewDirection(cinTheCamera, &view);
        V3 at{pos[0] - dir->x, pos[1] - dir->y, pos[2] - dir->z};
        vc::Call<void>(cinPlayerPed, 0x38, at.x, at.y, at.z);   // (Teleport takes the floats)
    }
    if (!(-1.0f == F(self, 0x24))) {
        Camera_SetNearClip(cinNiCamera, F(self, 0x24));
    }
    return 1;
}

// 0x6844D0 (thiscall, vf4): CameraCutscenePathTrack::End (+0x1D): the normal camera (+0x1E
// instant), the path freed, the player back (+0x20: shown again)
uint8_t __fastcall CutscenePath_End(uint8_t* self, re::Edx) {
    if (self[0x1D]) {
        Camera_SetShot(cinTheCamera, 1, 0, self[0x1E], 0, 0);
        cinWidescreenAmount = 0.0f;
        cinWidescreenSetting = 0;
        CutsceneCam_FreeTracks(cinCutsceneCam);
        uint8_t* player = cinPlayerPed;
        At<int32_t>(player, 0x48) = 1;
        UserInput_SetFlag1D(cinUserInput, 1);
        PlayerInfo_SetPaused(cinPlayers, 0);
        if (self[0x20]) {
            At<int32_t>(player, 0x48) = 1;
            At<int32_t>(player, 0xD0) = 0;
        }
    }
    return 1;
}

// A CMatrix on the stack (0x48 bytes; the game's locals are 0x44)
struct CinMatrix {
    uint8_t bytes[0x48];
};
// the point `p` in the frame of the entity (its matrix, turned to +0x124 unless 1e11 for
// a vehicle kept at its position; +0x3C of the matrix left as the game's uninitialised
// stack word, here 0)
inline void EntityFrame(CinMatrix& m, uint8_t* entity, bool vehicle) {
    Matrix_CopyConstruct(&m, Placeable_GetMatrix(entity));
    if (vehicle && !(99999997952.0f == F(entity, 0x124))) {
        Matrix_SetRotateZOnly(&m, F(entity, 0x124));
        float* pos = PosOf(entity);
        At<float>(m.bytes, 0x30) = pos[0];
        At<float>(m.bytes, 0x34) = pos[1];
        At<float>(m.bytes, 0x38) = pos[2];
        At<int32_t>(m.bytes, 0x3C) = 0;
    }
}
// the camera target's vehicle (Start also takes the object it uses)
inline uint8_t* TargetVehicle(uint8_t* target, bool orObject) {
    uint8_t* vehicle = At<uint8_t*>(target, 0x548);
    if (!vehicle && orObject) {
        uint8_t* object = At<uint8_t*>(target, 0x438);
        if (object && (object[0x108] & 7) == 6) {
            vehicle = object;
        }
    }
    return vehicle;
}

// 0x6870D0 (thiscall, vf2): CameraXYZTrack::Start: its angle limits to radians; the
// player paused (+0x18); a script camera from +0x1C to +0x28 (+0x50: from the current
// camera along its view; +0x38 -2: in the camera target's frame, else in the frame of
// the target's vehicle of model +0x38); its field of view (+0x35: the follow camera's)
uint8_t __fastcall CameraXYZ_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x54) = owner;
    F(self, 0x40) = F(self, 0x40) * 3.1415927410125732f / 180.0f;
    F(self, 0x44) = F(self, 0x44) * 3.1415927410125732f / 180.0f;
    F(self, 0x48) = F(self, 0x48) * 3.1415927410125732f / 180.0f;
    F(self, 0x4C) = 3.1415927410125732f * F(self, 0x4C) / 180.0f;
    if (self[0x18] && !Nis_IsSet()) {
        PlayerInfo_SetPaused(cinPlayers, 0);
        UserInput_SetEnabled0(cinUserInput, 0);
        cinWidescreenAmount = 0.0f;
        cinWidescreenSetting = self[0x34];
    }
    V3 from{F(self, 0x1C), F(self, 0x20), F(self, 0x24)};
    V3 to{F(self, 0x28), F(self, 0x2C), F(self, 0x30)};
    if (self[0x50]) {
        float* m = GetLocalTransform(GetCameraNode(cinNiCamera));
        V3 pos{m[9], m[10], m[11]};
        float* m2 = GetLocalTransform(GetCameraNode(cinNiCamera));
        V3 dir{m2[2], m2[5], m2[8]};
        float* m3 = GetLocalTransform(GetCameraNode(cinNiCamera));
        from = pos;
        to = V3{m3[9] + dir.x, m3[10] + dir.y, m3[11] + dir.z};
    } else {
        int32_t model = At<int32_t>(self, 0x38);
        uint8_t* target = cinCameraTarget;
        uint8_t* frameOf = nullptr;
        bool vehicle = false;
        if (model == -2) {
            if (target && (target[0x108] & 7) == 3) {
                frameOf = target;
            }
        } else if (model != -1 && target && (target[0x108] & 7) == 3) {
            uint8_t* v = TargetVehicle(target, true);
            if (v && At<int16_t>(v, 0x10E) == model) {
                frameOf = v;
                vehicle = true;
            }
        }
        if (frameOf) {
            CinMatrix m;
            EntityFrame(m, frameOf, vehicle);
            V3 point = from;
            V3 out;
            from = *TransformPoint(&out, &m, &point);
            point = to;
            to = *TransformPoint(&out, &m, &point);
        }
    }
    uint8_t* cam = Camera_GetCam(cinTheCamera, 4);
    ScriptCam_SetFov(cam, 0.0f);
    if (self[0x35]) {
        ScriptCam_SetFov(cam, FollowCam_GetFov(cinFollowCamera));
    }
    ScriptCam_SetXYZ(cam, from, to, 1, self[0x3C]);
    return 1;
}

// the stick's turn smoothed (+Smooth) into an angle (+Angle) kept within [Low, High]
template <int Smooth, int Angle, int Low, int High>
inline void StickTurn(uint8_t* self, int control, float speed) {
    float input = Pad_GetValue(control, 0) * speed;
    if (input == 0.0f) {
        float d = std::fabs(input - F(self, Smooth));
        if (d < 0.10000000149011612f) {
            F(self, Smooth) = input;
        }
    }
    F(self, Smooth) = (input - F(self, Smooth)) * 0.6000000238418579f + F(self, Smooth);
    if (!(0.0f == F(self, Smooth))) {
        F(self, Angle) = F(self, Smooth) + F(self, Angle);
    }
    float t = F(self, Angle);
    float low = F(self, Low);
    float high = F(self, High);
    float clamped;
    if (low > t) {
        clamped = low;
    } else if (high < t) {
        clamped = high;
    } else {
        clamped = t;
    }
    F(self, Angle) = clamped;
    F(self, Angle) = LimitAngle(clamped);
}

// 0x6874C0 (thiscall, vf3): CameraXYZTrack::Update (+0x3D: the stick turns the frame):
// yaw (+0x58, smoothed +0x5C, within +0x40..+0x44) and pitch (+0x60, +0x64, +0x48..+0x4C)
// turn the points in the camera target's frame, or its vehicle's (+0x38)
uint8_t __fastcall CameraXYZ_Update(uint8_t* self, re::Edx, float) {
    if (!self[0x3D]) {
        return 1;
    }
    StickTurn<0x5C, 0x58, 0x40, 0x44>(self, 0x12, cinYawSpeed);
    StickTurn<0x64, 0x60, 0x48, 0x4C>(self, 0x13, cinPitchSpeed);
    V3 from{F(self, 0x1C), F(self, 0x20), F(self, 0x24)};
    V3 to{F(self, 0x28), F(self, 0x2C), F(self, 0x30)};
    if (self[0x50]) {
        float* m = GetLocalTransform(GetCameraNode(cinNiCamera));
        from = V3{m[9], m[10], m[11]};
        float* m2 = GetLocalTransform(GetCameraNode(cinNiCamera));
        V3 view;
        V3* dir = Camera_GetViewDirection(cinTheCamera, &view);
        to = V3{dir->x + m2[9], m2[10] + dir->y, m2[11] + dir->z};
    } else {
        int32_t model = At<int32_t>(self, 0x38);
        uint8_t* target = cinCameraTarget;
        uint8_t* frameOf = nullptr;
        bool vehicle = false;
        if (model == -2) {
            if (target && (target[0x108] & 7) == 3) {
                frameOf = target;
            }
        } else if (model != -1 && target && (target[0x108] & 7) == 3) {
            uint8_t* v = TargetVehicle(target, false);
            if (v && At<int16_t>(v, 0x10E) == model) {
                frameOf = v;
                vehicle = true;
            }
        }
        if (frameOf) {
            CinMatrix m;
            EntityFrame(m, frameOf, vehicle);
            V3 angles{F(self, 0x60), 0.0f, F(self, 0x58)};
            CinMatrix turn;
            At<int32_t>(turn.bytes, 0x40) = 0;
            Matrix_SetRotate(&turn, &angles);
            Matrix_Multiply(&m, &turn);
            V3 point = from;
            V3 out;
            from = *TransformPoint(&out, &m, &point);
            point = to;
            to = *TransformPoint(&out, &m, &point);
        }
    }
    uint8_t* cam = Camera_GetCam(cinTheCamera, 4);
    ScriptCam_SetFov(cam, 0.0f);
    if (self[0x35]) {
        ScriptCam_SetFov(cam, FollowCam_GetFov(cinFollowCamera));
    }
    ScriptCam_SetXYZ(cam, from, to, 1, self[0x3C]);
    return 1;
}

// 0x684BA0 (thiscall, vf4): CameraXYZTrack::End (+0x19, not in a NIS): the normal camera
// (+0x1A instant) and input back
uint8_t __fastcall CameraXYZ_End(uint8_t* self, re::Edx) {
    if (self[0x19] && !Nis_IsSet()) {
        cinWidescreenSetting = 0;
        cinWidescreenAmount = 0.0f;
        Camera_SetShot(cinTheCamera, 1, 0, self[0x1A], 0, 0);
        UserInput_SetEnabled0(cinUserInput, 1);
    }
    return 1;
}

// 0x684C20 (thiscall, vf2): CameraFadeSetupTrack::Start: the fade's settings
uint8_t __fastcall CameraFade_Start(uint8_t* self, re::Edx, uint8_t*) {
    cinFade[0x8DC] = self[0x18];
    At<int32_t>(cinFade, 0x8E0) = At<int32_t>(self, 0x1C);
    At<int32_t>(cinFade, 0x8E4) = At<int32_t>(self, 0x20);
    At<int32_t>(cinFade, 0x8E8) = At<int32_t>(self, 0x24);
    return 1;
}

// 0x684CA0 (thiscall, vf2): CameraLookatPlayerTrack::Start: the script camera looks at the
// player (+0x1C, +0x18 its flag)
uint8_t __fastcall LookatPlayer_Start(uint8_t* self, re::Edx, uint8_t*) {
    uint8_t* cam = Camera_GetCam(cinTheCamera, 4);
    ScriptCam_LookAt(cam, 0, 3, 1, F(self, 0x1C));
    cam[0x100] = self[0x18];
    return 1;
}

// ---- text -------------------------------------------------------------------------------

// 0x6849A0 (thiscall, vf2): TextLineTrack::Start: its text (+0x18) shown for its length
uint8_t __fastcall TextLine_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    float seconds = F(self, 0x10) - F(self, 0xC);
    Hud_ShowMessage(At<int32_t>(self, 0x18), seconds);
    return 1;
}

// 0x6849D0 (thiscall, vf3): TextLineTrack::Update: until the skip button
uint8_t __fastcall TextLine_Update(uint8_t*, re::Edx, float) {
    return !(Pad_GetValue(7, 0) > 0.10000000149011612f);
}

// 0x684A70 / 0x684AF0 (thiscall, vf2): TextTrack / DrawTextTrack::Start: the owner kept,
// the text looked up (unused)
template <int OwnerAt>
uint8_t __fastcall TextTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, OwnerAt) = owner;
    Text_Get(cinText, At<int32_t>(self, 0x18));
    return 1;
}

// the text tracks run for the player, or a ped not talking to someone else
inline bool TextFor(uint8_t* ped) {
    return At<int32_t>(ped, 0x1310) == 13 || At<int32_t>(ped, 0xBB0) == 0 || At<uint8_t*>(ped, 0xBB4) == cinPlayerPed;
}

// 0x685AC0 (thiscall, vf3): TextTrack::Update: its text (+0x18) printed once a frame at
// +0x30, +0x34 (font +0x1C when loaded, centred +0x3D, +0x3C its flag), red
uint8_t __fastcall TextTrack_Update(uint8_t* self, re::Edx, float) {
    if (!TextFor(At<uint8_t*>(At<uint8_t*>(self, 0x40), 8))) {
        return 0;
    }
    if (cinTextFrame != cinTimeMs) {
        cinTextFrame = cinTimeMs;
        uint8_t colour[4];
        RGBA_Construct(colour, 200, 0, 0, 0xFF);
        Font_Begin();
        cinFontColour[0] = colour[0];
        cinFontColour[1] = colour[1];
        cinFontColour[2] = colour[2];
        cinFontColour[3] = colour[3];
        int16_t style = int16_t(At<int32_t>(self, 0x1C));
        if (cinFontStyleLoaded[style] != 0) {
            cinFontStyle = style;
            if (style != 0) {
                cinFontFlags = 0;
            }
        }
        Font_SetJustify(self[0x3D] ? 2 : 0);
        Font_SetStyle(4);
        cinFontFlags = self[0x3C] ? 2 : 0;
        int32_t y = At<int32_t>(self, 0x34);
        int32_t x = At<int32_t>(self, 0x30);
        const uint16_t* text = Text_Get(cinText, At<int32_t>(self, 0x18));
        Font_PrintString(float(x), float(y), text, 0);
    }
    return 1;
}

// 0x685BE0 (thiscall, vf3): DrawTextTrack::Update: its text (+0x18) into the HUD's slot
// +0x1C once a frame (with subtitles on, the player's own conversations too)
uint8_t __fastcall DrawText_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x20), 8);
    if (At<int32_t>(ped, 0x1310) != 13 && At<int32_t>(ped, 0xBB0) != 0) {
        uint8_t* partner = At<uint8_t*>(ped, 0xBB4);
        if (partner && (partner != cinPlayerPed || !cinSocialSubtitles)) {
            return 0;
        }
    }
    if (cinDrawTextFrame != cinTimeMs) {
        cinDrawTextFrame = cinTimeMs;
        HUDText_SetText(cinHudText, At<int32_t>(self, 0x1C), At<int32_t>(self, 0x18));
    }
    return 1;
}

// ---- the conversation condition ---------------------------------------------------------

// 0x685AA0 (thiscall, vf1): CConversationActiveCond::Evaluate: in a cutscene conversation
// and the owner has no ped (the game ANDs the whole dword at 0xB9DB1C; the byte counts)
uint8_t __fastcall ConversationActive_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t mask = At<uint8_t*>(owner, 8) != nullptr ? 0 : 0xFF;
    return uint8_t(mask & cinInCutscene);
}

// 0x684790 (thiscall, vf4): its type descriptor
const void* __fastcall ConversationActive_Type(uint8_t*, re::Edx) {
    return reinterpret_cast<const void*>(0x0093914C);
}

#define CINTRK_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

// construction tails and destructors (the shapes of ActionFactories.h)
CINTRK_REVERSED("CinematicTrack::Init_683F80", 0x00683F80, &Fields<U8(0x8, 1), U8(0x18, 0), U8(0x19, 0), U8(0x1A, 0), U8(0x1B, 1), ClearName(0x20), I32(0x24, 0), I32(0x2C, 0), I32(0x28, 1)>);
CINTRK_REVERSED("CinematicTrack::Destruct_683FD0", 0x00683FD0, &Fields<Vt(0x00935A28), KeyDtor(0x20), KeyDtor(0x1C), TrackBaseDtor>);
CINTRK_REVERSED("DialogLineTrack::Init_684090", 0x00684090, &Fields<ClearName(0x20), Zero(0x18), U8(0x8, 1), U8(0x2C, 1), U8(0x2D, 0), I32(0x30, 0), U8(0x36, 1), U8(0x35, 1), U8(0x34, 1), U8(0x3C, 1), I32(0x40, 0), ClearName(0x24), U8(0x44, 1), Zero(0x1C), U8(0x45, 1), I32(0x28, 0), U8(0x46, 0), U8(0x59, 0)>);
CINTRK_REVERSED("DialogShotTrack::Init_684300", 0x00684300, &Fields<U8(0x8, 0), I32(0x20, 1000), U8(0x24, 1), U8(0x25, 0), U8(0x26, 0), ClearName(0x1C), U8(0x29, 0), U8(0x27, 1), U8(0x28, 1)>);
CINTRK_REVERSED("DialogShotTrack::Destruct_684350", 0x00684350, &Fields<Vt(0x00935A5C), KeyDtor(0x1C), KeyDtor(0x18), TrackBaseDtor>);
CINTRK_REVERSED("CameraCutscenePathTrack::Init_684380", 0x00684380, &Fields<U8(0x8, 1), ClearName(0x18), U8(0x1C, 1), F32(0x24, -1.0f), U8(0x1D, 1), U8(0x1F, 1), U8(0x20, 1), U8(0x1E, 0)>);
CINTRK_REVERSED("CameraCutscenePathTrack::Destruct_6843D0", 0x006843D0, &Fields<Vt(0x00935B04), KeyDtor(0x18), TrackBaseDtor>);
CINTRK_REVERSED("ScriptTrack::Init_684570", 0x00684570, &Fields<Zero(0x1C), U8(0x8, 1), U8(0x2C, 0), U8(0x18, 0), U8(0x20, 0), U8(0x21, 0)>);
CINTRK_REVERSED("ScriptTrack::Destruct_684590", 0x00684590, &Fields<Vt(0x00935AD4), KeyDtor(0x28), KeyDtor(0x24), TrackBaseDtor>);
CINTRK_REVERSED("MissionTrack::Init_684600", 0x00684600, &Fields<U8(0x8, 1), F32(0x10, -1.0f), U8(0x1C, 0), I32(0x24, 0xFFFFFFFF)>);
CINTRK_REVERSED("MissionTrack::Destruct_684620", 0x00684620, &Fields<Vt(0x00934D2C), KeyDtor(0x18), TrackBaseDtor>);
CINTRK_REVERSED("FacialEyeTrack::Init_6846E0", 0x006846E0, &Fields<Zero(0x1C), U8(0x8, 0), Zero(0x20), I32(0x18, 0), Zero(0x24), I32(0x28, 0)>);
CINTRK_REVERSED("FacialEyeTrack::Destruct_684700", 0x00684700, &Fields<Vt(0x00935A74), TrackBaseDtor>);
CINTRK_REVERSED("FacialEyeLidTrack::Destruct_684710", 0x00684710, &Fields<Vt(0x00935A8C), TrackBaseDtor>);
CINTRK_REVERSED("FacialEyeBrowTrack::Destruct_684720", 0x00684720, &Fields<Vt(0x00935AA4), TrackBaseDtor>);
CINTRK_REVERSED("FacialTrack::Init_684740", 0x00684740, &Fields<Zero(0x1C), U8(0x8, 0), I32(0x18, 0), I32(0x20, 0)>);
CINTRK_REVERSED("FacialMouthTrack::Destruct_684760", 0x00684760, &Fields<Vt(0x00935ABC), TrackBaseDtor>);
CINTRK_REVERSED("CConversationActiveCond::Destruct_684780", 0x00684780, &Fields<Vt(0x00937318)>);
CINTRK_REVERSED("CameraPathTrack::Init_6847A0", 0x006847A0, &Fields<U8(0x8, 0), U8(0x18, 0), U8(0x19, 0), U8(0x1A, 0), ClearName(0x1C), ClearName(0x20)>);
CINTRK_REVERSED("CameraPathTrack::Destruct_684800", 0x00684800, &Fields<Vt(0x00935AEC), KeyDtor(0x20), KeyDtor(0x1C), TrackBaseDtor>);
CINTRK_REVERSED("TextLineTrack::Init_684950", 0x00684950, &Fields<U8(0x8, 1), ClearName(0x18)>);
CINTRK_REVERSED("TextLineTrack::Destruct_684980", 0x00684980, &Fields<Vt(0x00935B1C), KeyDtor(0x18), TrackBaseDtor>);
CINTRK_REVERSED("TextTrack::Init_684A00", 0x00684A00, &Fields<U8(0x8, 0), ClearName(0x18), I32(0x20, 0xFF), I32(0x24, 0xFF), I32(0x28, 0xFF), I32(0x2C, 0xFF), I32(0x1C, 0), I32(0x30, 0), I32(0x34, 0), U8(0x3C, 1), U8(0x3D, 1)>);
CINTRK_REVERSED("TextTrack::Destruct_684A50", 0x00684A50, &Fields<Vt(0x00935B34), KeyDtor(0x18), TrackBaseDtor>);
CINTRK_REVERSED("DrawTextTrack::Init_684A90", 0x00684A90, &Fields<U8(0x8, 0), ClearName(0x18), I32(0x1C, 0)>);
CINTRK_REVERSED("DrawTextTrack::Destruct_684AD0", 0x00684AD0, &Fields<Vt(0x00935B4C), KeyDtor(0x18), TrackBaseDtor>);
CINTRK_REVERSED("CameraXYZTrack::Init_684B10", 0x00684B10, &Fields<F32(0x1C, 0.30000001192092896f), F32(0x20, -3.0f), U8(0x8, 1), U8(0x18, 0), U8(0x19, 0), F32(0x24, 0.5f), U8(0x34, 1), I32(0x38, 0xFFFFFFFF), Zero(0x28), U8(0x1A, 1), Zero(0x2C), U8(0x35, 0), Zero(0x30), U8(0x3C, 0), U8(0x3D, 0), F32(0x40, -360.0f), U8(0x50, 0), F32(0x44, 360.0f), F32(0x48, -75.0f), F32(0x4C, 75.0f), Zero(0x58), Zero(0x5C), Zero(0x60), Zero(0x64)>);
CINTRK_REVERSED("CameraXYZTrack::Destruct_684B90", 0x00684B90, &Fields<Vt(0x00935B64), TrackBaseDtor>);
CINTRK_REVERSED("CameraFadeSetupTrack::Init_684C00", 0x00684C00, &Fields<U8(0x8, 1), U8(0x18, 0), I32(0x1C, 0x1E), I32(0x20, 0), I32(0x24, 0xB4)>);
CINTRK_REVERSED("CameraLookatPlayerTrack::Init_684C70", 0x00684C70, &Fields<Zero(0xC), U8(0x8, 0), Zero(0x1C), U8(0x18, 0)>);
CINTRK_REVERSED("CameraLookatPlayerTrack::Destruct_684C90", 0x00684C90, &Fields<Vt(0x009360DC), TrackBaseDtor>);
CINTRK_REVERSED("MenuTrack::Destruct_6870A0", 0x006870A0, &Fields<Vt(0x0093838C), KeyDtor(0x1C), KeyDtor(0x18), TrackBaseDtor>);

// the rest
CINTRK_REVERSED("Facial::SetLidTarget_683E80", 0x00683E80, &Facial_SetLidTargetImpl);
CINTRK_REVERSED("CinematicShot::Find_684000", 0x00684000, &CinematicShot_FindImpl);
CINTRK_REVERSED("DialogLineTrack::Construct_684120", 0x00684120, &DialogLine_Construct);
CINTRK_REVERSED("DialogLineTrack::Destruct_684160", 0x00684160, &DialogLine_DestructImpl);
CINTRK_REVERSED("DialogLineTrack::End", 0x006841B0, &DialogLine_End);
CINTRK_REVERSED("Conversation::Cancel_6842C0", 0x006842C0, &Conversation_Cancel);
CINTRK_REVERSED("CancelConversationTrack::Start", 0x006842E0, &CancelConversation_Start);
CINTRK_REVERSED("CameraCutscenePathTrack::Start", 0x006843F0, &CutscenePath_Start);
CINTRK_REVERSED("CameraCutscenePathTrack::End", 0x006844D0, &CutscenePath_End);
CINTRK_REVERSED("ScriptTrack::Start", 0x006845C0, &ScriptTrack_Start);
CINTRK_REVERSED("ScriptTrack::Update", 0x006845E0, &ScriptTrack_Update);
CINTRK_REVERSED("MissionTrack::Start", 0x00684640, &MissionTrack_Start);
CINTRK_REVERSED("MissionTrack::Update", 0x00684690, &MissionTrack_Update);
CINTRK_REVERSED("FacialEyeBrowTrack::Update", 0x00684730, &Facial_ModeIs<3>);
CINTRK_REVERSED("FacialTrack::UpdateMode2_684770", 0x00684770, &Facial_ModeIs<2>);
CINTRK_REVERSED("CConversationActiveCond::Type", 0x00684790, &ConversationActive_Type);
CINTRK_REVERSED("CameraPathTrack::Start", 0x00684830, &CameraPath_Start);
CINTRK_REVERSED("CameraPathTrack::End", 0x00684900, &CameraPath_End);
CINTRK_REVERSED("TextLineTrack::Start", 0x006849A0, &TextLine_Start);
CINTRK_REVERSED("TextLineTrack::Update", 0x006849D0, &TextLine_Update);
CINTRK_REVERSED("TextTrack::Start", 0x00684A70, &TextTrack_Start<0x40>);
CINTRK_REVERSED("DrawTextTrack::Start", 0x00684AF0, &TextTrack_Start<0x20>);
CINTRK_REVERSED("CameraXYZTrack::End", 0x00684BA0, &CameraXYZ_End);
CINTRK_REVERSED("CameraFadeSetupTrack::Start", 0x00684C20, &CameraFade_Start);
CINTRK_REVERSED("CameraLookatPlayerTrack::Start", 0x00684CA0, &LookatPlayer_Start);
CINTRK_REVERSED("CinematicTrack::Start", 0x00684EC0, &CinematicTrack_Start);
CINTRK_REVERSED("DialogLineTrack::DeletingDestruct", 0x00684FD0, &DialogLine_DeletingDestruct);
CINTRK_REVERSED("DialogShotTrack::Start", 0x00685030, &DialogShot_Start);
CINTRK_REVERSED("CameraCutscenePathTrack::Update", 0x00685230, &CutscenePath_Update);
CINTRK_REVERSED("ScriptTrack::End", 0x00685300, &ScriptTrack_End);
CINTRK_REVERSED("FacialEyeTrack::Start", 0x00685500, &FacialEye_Start);
CINTRK_REVERSED("FacialEyeTrack::End", 0x006855E0, &FacialEye_End);
CINTRK_REVERSED("FacialEyeLidTrack::Start", 0x00685650, &FacialLid_Start);
CINTRK_REVERSED("FacialEyeLidTrack::End", 0x00685760, &FacialLid_End);
CINTRK_REVERSED("FacialEyeBrowTrack::Start", 0x006857D0, &FacialBrow_Start);
CINTRK_REVERSED("FacialEyeBrowTrack::End", 0x006858C0, &FacialBrow_End);
CINTRK_REVERSED("FacialMouthTrack::Start", 0x006858F0, &FacialMouth_Start);
CINTRK_REVERSED("FacialMouthTrack::End", 0x00685A30, &FacialMouth_End);
CINTRK_REVERSED("CConversationActiveCond::Evaluate", 0x00685AA0, &ConversationActive_Evaluate);
CINTRK_REVERSED("TextTrack::Update", 0x00685AC0, &TextTrack_Update);
CINTRK_REVERSED("DrawTextTrack::Update", 0x00685BE0, &DrawText_Update);
CINTRK_REVERSED("DialogLineTrack::SpeechDone_686750", 0x00686750, &DialogLine_SpeechDone);
CINTRK_REVERSED("DialogLineTrack::QueuedSpeechDone_6867B0", 0x006867B0, &DialogLine_QueuedSpeechDone);
CINTRK_REVERSED("DialogLineTrack::Start", 0x00686900, &DialogLine_Start);
CINTRK_REVERSED("DialogLineTrack::QueueNext", 0x00686C50, &DialogLine_QueueNextImpl);
CINTRK_REVERSED("DialogLineTrack::Update", 0x00686EC0, &DialogLine_Update);
CINTRK_REVERSED("CameraXYZTrack::Start", 0x006870D0, &CameraXYZ_Start);
CINTRK_REVERSED("CameraXYZTrack::Update", 0x006874C0, &CameraXYZ_Update);

} // namespace
