// The action tree's animation tracks: AnimationTrack (+0x18 the animation's name, +0x1C
// its group, +0x24.. its blend settings, +0x4C the blend it plays into), the joint
// driver, hit react, steer / struggle / target sync / rotation drivers, the
// parametric controller and tracker, AI aim, the spud cannon and door controllers:
// their field initialisers, type ids, destructors and virtuals; and a few animation
// helpers. 0x607420..0x608630.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
using namespace actc;

namespace {

constexpr uint32_t kVtAnimationGroupReleaseTrack = 0x00930EF4;
constexpr uint32_t kVtAnimationTrack = 0x00930F0C;
constexpr uint32_t kVtHitReactTrack = 0x00930F28;
constexpr uint32_t kVtJointDriverTrack = 0x00930F40;
constexpr uint32_t kVtSteerTrack = 0x00930F64;
constexpr uint32_t kVtStruggleTrack = 0x00930F7C;
constexpr uint32_t kVtTargetSyncTrack = 0x00930F94;
constexpr uint32_t kVtParametricControllerTrack = 0x00930FAC;
constexpr uint32_t kVtParametricTrackerTrack = 0x00930FC4;
constexpr uint32_t kVtAIAimTrack = 0x00930FE4;
constexpr uint32_t kVtSpudCannonTrack = 0x00930FFC;
constexpr uint32_t kVtDoorControlTrack = 0x00931014;
constexpr uint32_t kVtRotationTrack = 0x0093102C;

GAME_VAR(int32_t, trackerCount_1B65030, 0x01B65030);     // ParametricTrackerTracks made
GAME_VAR(int32_t, trackerTurn_1B65034, 0x01B65034);      // whose turn it is
GAME_VAR(uint8_t, trackerSaved_1B6502C, 0x01B6502C);
GAME_VAR(uint8_t, parametricStatic_B0B134, 0x00B0B134);
GAME_VAR(uint32_t, aimBoneA_1B6531C, 0x01B6531C);        // name keys of the aim bone's path
GAME_VAR(uint32_t, aimBoneB_1B65394, 0x01B65394);
inline void* const aimBoneRoot_1B65398 = reinterpret_cast<void*>(0x01B65398);
GAME_VAR(float, rotationRate_B0B190, 0x00B0B190);

inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(float, __thiscall, Anim_GetBonePosition, 0x006B3E20, uint8_t* anim, int32_t bone, int, V3* out, int time);
GAME_FN(uint8_t*, __thiscall, AnimDict_Find, 0x006B1D60, void* dict, uint32_t name, uintptr_t, int32_t* index);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, uint8_t* blend, uint32_t name);
GAME_FN(void, __thiscall, AnimManager_GetTime, 0x006C0F30, void* manager, uint8_t* blend, uint8_t* anim, int,
        float* out);
GAME_FN(float, __cdecl, RemapRange, 0x0072F500, float value, float from, float to, float outFrom, float outTo);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C0E90, void* manager, uint8_t* blend, uint8_t* anim, int,
        float time, int, int);
GAME_FN(void, __thiscall, AnimManager_Update, 0x006C0DC0, void* manager, uint8_t* blend, uint8_t* anim, int);
GAME_FN(void, __thiscall, Track_Construct, 0x0061A880, uint8_t* track);
GAME_FN(uint32_t*, __thiscall, NameKey_FromHash, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, HashPath_Set, 0x005FA8C0, int32_t* path, int32_t count, uint32_t* hashes);
GAME_FN(uint8_t*, __cdecl, FindBoneInfo, 0x0072AEA0, int32_t* path, void* root);
GAME_FN(float, __cdecl, AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);

// 0x607420 (cdecl): e^x to the fourth power of its series
float __cdecl ExpSeries(float x) {
    float sq = x * x;
    float sum = x + 1.0f + sq * 0.5f;
    float cube = x * sq;
    sum = sum + cube * 0.16666667f;
    float fourth = sq * sq;
    return sum + fourth * 0.041666668f;
}

// 0x607480 (thiscall): an entry (0x2C bytes; +8 the entries, +0xC their count)
uint8_t* __fastcall Entries2C_GetAt(uint8_t* self, re::Edx, uint32_t i) {
    if (i >= At<uint32_t>(self, 0xC)) {
        return nullptr;
    }
    return At<uint8_t*>(self, 8) + i * 0x2C;
}

// 0x6074A0 (thiscall): the entry named name (case blind; its index out)
uint8_t* __fastcall Entries2C_Find(uint8_t* self, re::Edx, const char* name, uint32_t* index) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xC); i++) {
        if (Stricmp(reinterpret_cast<const char*>(At<uint8_t*>(self, 8) + i * 0x2C), name) == 0) {
            if (index) {
                *index = i;
            }
            return At<uint8_t*>(self, 8) + i * 0x2C;
        }
    }
    return nullptr;
}

// 0x607500 (thiscall): how far a bone moves over the animation (its end less its start)
int __fastcall Anim_GetBoneTravel(uint8_t* self, re::Edx, int32_t bone, V3* out) {
    V3 start = {0.0f, 0.0f, 0.0f};
    V3 end = {0.0f, 0.0f, 0.0f};
    Anim_GetBonePosition(self, bone, 1, &start, 0);
    Anim_GetBonePosition(self, bone, 1, &end, -1);
    V3 d;
    V3* v = VectorSub(&d, &end.x, &start.x);
    *out = *v;
    return 0;
}

// 0x607580 (thiscall): a request (+0x20 who, +0x24 what, +0x28 / +0x2C its time;
// -1: keep the time); 0x16: nobody, 0x2B: the one already busy (+0x30)
int __fastcall Request_Set(uint8_t* self, re::Edx, uint8_t* who, int32_t what, float time) {
    if (!who) {
        return 0x16;
    }
    if (At<uint8_t*>(self, 0x30) && At<uint8_t*>(self, 0x30) == who) {
        return 0x2B;
    }
    if (time > 0.0f) {
        At<float>(self, 0x2C) = time;
        At<float>(self, 0x28) = time;
    } else if (time == -1.0f) {
        At<int32_t>(self, 0x24) = what;
        At<uint8_t*>(self, 0x20) = who;
        return 0;
    } else {
        At<float>(self, 0x28) = 0.0f;
    }
    At<int32_t>(self, 0x24) = what;
    At<uint8_t*>(self, 0x20) = who;
    return 0;
}

// 0x607660 (thiscall): AnimationTrack's fields
void __fastcall AnimationTrack_InitFields(uint8_t* self, re::Edx) {
    SetNameRaw(self, 0x18, emptyString);
    At<uint32_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0x24) = 0;
    ClearName(self, 0x1C);
    At<float>(self, 0x28) = 0.0f;
    self[0x3C] = 0;
    self[0x3D] = 0;
    At<float>(self, 0x2C) = -1.0f;
    self[8] = 1;
    At<float>(self, 0x30) = 1.0f;
    At<float>(self, 0x34) = -1.0f;
    At<float>(self, 0x38) = -1.0f;
}

// 0x607770 (cdecl): an animation's index in a dictionary (-1: none)
int32_t __cdecl AnimDict_IndexOf(const uint32_t* name, void* dict) {
    int32_t index = 0;
    if (!AnimDict_Find(dict, *name, 0, &index)) {
        return -1;
    }
    return index;
}

// 0x6077A0 (thiscall): JointDriverTrack's animation's length (-1: none)
float __fastcall JointDriverTrack_GetLength(uint8_t* self, re::Edx) {
    if (!At<uint8_t*>(self, 0x4C)) {
        vc::Call<void>(self, acttree::kTrackResolve);
    }
    void* dict = At<void*>(At<uint8_t*>(At<uint8_t*>(self, 0x4C), 0x94), 0x10);
    uint8_t* anim = AnimDict_Find(dict, At<uint32_t>(self, 0x18), 0, nullptr);
    if (anim) {
        return At<float>(anim, 0x10);
    }
    return -1.0f;
}

// 0x6077E0 / 0x607800 (thiscall): two animation tracks' fields
void __fastcall AnimationTrack_InitFields_6077E0(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x58) = 0;
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00607660)(self);
    self[8] = 0;
    self[0x5C] = 0;
    self[0x5D] = 1;
}
void __fastcall AnimationTrack_InitFields_607800(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00607660)(self);
    At<float>(self, 0x58) = 0.0f;
    self[0x5C] = 1;
    self[0x5D] = 0;
    At<float>(self, 0x60) = -1.0f;
    At<float>(self, 0x64) = -1.0f;
    At<float>(self, 0x68) = -1.0f;
}

// 0x607830 (thiscall): HitAnimationTrack's blend out (+0x70 once past +0x58 into the
// animation while +0x5C and +0x6C > 0; else +0x30)
float __fastcall HitAnimationTrack_GetBlendOut(uint8_t* self, re::Edx) {
    if (self[0x5C] != 0 && 0.0f < At<float>(self, 0x6C)) {
        float after = At<float>(self, 0x58);
        uint8_t* anim = Blend_GetAnim(At<uint8_t*>(self, 0x4C), At<uint32_t>(self, 0x24));
        float time;
        AnimManager_GetTime(animManager, At<uint8_t*>(self, 0x4C), anim, 0, &time);
        if (after < time || after == time) {
            return At<float>(self, 0x70);
        }
    }
    return At<float>(self, 0x30);
}

// 0x6078A0 (thiscall): HitReactTrack's fields
void __fastcall HitReactTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = -1.0f;
    At<float>(self, 0x28) = -1.0f;
    At<float>(self, 0x2C) = -1.0f;
    At<float>(self, 0x24) = 0.0f;
    ClearName(self, 0x18);
    At<uint32_t>(self, 0x34) = 0;
    self[8] = 1;
}

// 0x607920 (thiscall): JointDriverTrack's fields
void __fastcall JointDriverTrack_InitFields(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00607660)(self);
    At<uint32_t>(self, 0x58) = 0;
    ClearName(self, 0x5C);
    At<int32_t>(self, 0x60) = 1;
    ClearName(self, 0x64);
    self[0x68] = 0;
    At<uint32_t>(self, 0x78) = 0;
    At<uint32_t>(self, 0x6C) = 0;
    self[0x70] = 0;
}

// 0x6079E0 (thiscall): JointDriverTrack::Resolve: the blend of its joint (+0x74, its +0x2C)
void __fastcall JointDriverTrack_Resolve(uint8_t* self, re::Edx) {
    if (At<uint8_t*>(self, 0x74)) {
        At<uint8_t*>(self, 0x4C) = At<uint8_t*>(At<uint8_t*>(self, 0x74), 0x2C);
    }
}

// 0x6079F0 / 0x607A00 (thiscall): two joint driver tracks' fields
void __fastcall JointDriverTrack_InitFields_6079F0(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00607920)(self);
    self[0x7C] = 1;
}
void __fastcall JointDriverTrack_InitFields_607A00(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00607920)(self);
    At<float>(self, 0x7C) = 0.0f;
    At<float>(self, 0x80) = 1.0f;
}

// the owner's parametric value (+0x38 points at it)
float OwnerParameter(uint8_t* owner) {
    return *At<float*>(owner, 0x38);
}

// 0x607A60 (thiscall): JointDriverParametricTrack::Update: its animation played at the
// time the owner's parameter maps to (+0x7C..+0x80 onto +0x28..+0x2C, the animation's
// length when that's below 0)
uint8_t __fastcall JointDriverParametricTrack_Update(uint8_t* self, re::Edx, float) {
    vc::Call<void>(self, acttree::kTrackResolve);
    uint8_t* blend = At<uint8_t*>(self, 0x4C);
    if (!blend || !At<uint8_t*>(blend, 0x94)) {
        return 0;
    }
    uint8_t* anim = Blend_GetAnim(blend, At<uint32_t>(self, 0x24));
    if (!anim) {
        return 0;
    }
    float from = At<float>(self, 0x28);
    float to = At<float>(self, 0x2C);
    if (to < 0.0f) {
        to = At<float>(At<uint8_t*>(anim, 0x18), 0xC);
    }
    float time = RemapRange(OwnerParameter(At<uint8_t*>(self, 0x40)), At<float>(self, 0x7C), At<float>(self, 0x80),
                            from, to);
    AnimManager_Play(animManager, At<uint8_t*>(self, 0x4C), anim, 0, time, 0, 1);
    AnimManager_Update(animManager, At<uint8_t*>(self, 0x4C), anim, 0);
    return 1;
}

// 0x607B40 (thiscall): AnimationDriverSteerTrack's fields
void __fastcall SteerTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 0.0f;
    At<float>(self, 0x1C) = 1.0f;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 3.14159274f;
}

// 0x607B90 (thiscall): AnimationDriverStruggleTrack's fields
void __fastcall StruggleTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.0f;
    self[8] = 0;
    self[0x20] = 0;
    At<float>(self, 0x1C) = 1.0f;
    At<int32_t>(self, 0x28) = 3;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x2C) = 0.2f;
    At<float>(self, 0x30) = -0.05f;
    At<float>(self, 0x34) = -0.8f;
    At<float>(self, 0x38) = 1.0f;
    At<float>(self, 0x3C) = -0.1f;
}

// 0x607C10 (thiscall): AnimationDriverStruggleTrack::Start: the parameter from +0x18
// (+0x20: +0x24), its range +0x18..+0x1C ordered into +0x48 / +0x44
uint8_t __fastcall StruggleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x40) = owner;
    if (self[0x20] == 0) {
        *At<float*>(owner, 0x38) = At<float>(self, 0x18);
    } else {
        *At<float*>(owner, 0x38) = At<float>(self, 0x24);
    }
    if (At<float>(self, 0x1C) > At<float>(self, 0x18)) {
        At<float>(self, 0x48) = At<float>(self, 0x18);
        At<float>(self, 0x44) = At<float>(self, 0x1C);
    } else {
        At<float>(self, 0x48) = At<float>(self, 0x1C);
        At<float>(self, 0x44) = At<float>(self, 0x18);
    }
    self[0x50] = 0;
    At<uint32_t>(self, 0x4C) = 0;
    At<uint32_t>(self, 0x54) = 0;
    At<uint32_t>(self, 0x58) = 0;
    At<uint32_t>(self, 0x5C) = 0;
    At<uint32_t>(self, 0x60) = 0;
    return 1;
}

// 0x607CA0 (thiscall): AnimationDriverTargetSyncTrack's fields
void __fastcall TargetSyncTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x20) = 1.0f;
    self[0x18] = 1;
    self[0x19] = 0;
    At<float>(self, 0x24) = 0.0f;
    self[0x1A] = 0;
    At<float>(self, 0x2C) = 0.0f;
    self[0x1B] = 0;
    self[0x1C] = 0;
    self[0x1D] = 0;
    self[8] = 0;
    At<float>(self, 0x28) = 1.0f;
}

// 0x607D00 (thiscall): ParametricAnimationControllerTrack::Start
uint8_t __fastcall ParametricControllerTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x34) = owner;
    At<float>(self, 0x30) = At<float>(self, 0x2C);
    return 1;
}

// 0x607D20 (thiscall): ParametricTrackerTrack's fields
void __fastcall ParametricTrackerTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 0.05f;
    At<float>(self, 0x1C) = 0.05f;
}

// 0x607D40 (thiscall): ParametricTrackerTrack's constructor (counted)
uint8_t* __fastcall ParametricTrackerTrack_Construct(uint8_t* self, re::Edx) {
    Track_Construct(self);
    At<uint32_t>(self, 0) = kVtParametricTrackerTrack;
    trackerCount_1B65030 += 1;
    return self;
}

// 0x607D60 (thiscall): ParametricTrackerTrack::Start
uint8_t __fastcall ParametricTrackerTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    At<float>(self, 0x28) = OwnerParameter(owner);
    self[0x24] = 1;
    At<float>(self, 0x2C) = 0.0f;
    return 1;
}

// 0x607D80 (thiscall): ParametricTrackerTrack::Update: follows how fast the owner's
// parameter moves (+0x2C smoothed); settled (+0x24) until it moves more than +0x18, then
// moving until it slows below +0x1C. The trackers take turns saving and setting the
// parametric static flag (0xB0B134)
uint8_t __fastcall ParametricTrackerTrack_Update(uint8_t* self, re::Edx, float) {
    int32_t turn = trackerTurn_1B65034;
    if (turn == 0) {
        trackerSaved_1B6502C = parametricStatic_B0B134;
        parametricStatic_B0B134 = 1;
    }
    turn += 1;
    trackerTurn_1B65034 = turn;
    if (turn >= trackerCount_1B65030) {
        trackerTurn_1B65034 = 0;
    }
    uint8_t* owner = At<uint8_t*>(self, 0x20);
    float delta = At<float>(self, 0x28) - OwnerParameter(owner);
    if (std::fabs(delta) > 0.5f) {
        delta = float(double(std::fabs(delta)) - 1.0);
    }
    float speed = std::fabs(delta);
    if (At<float>(self, 0x2C) < speed) {
        At<float>(self, 0x2C) = speed;
    } else {
        At<float>(self, 0x2C) = float(double(At<float>(self, 0x2C)) * 0.9) + float(double(speed) * 0.1);
    }
    if (self[0x24] != 0) {
        if (At<float>(self, 0x18) < speed) {
            parametricStatic_B0B134 = 0;
            self[0x24] = 0;
            At<float>(self, 0x28) = OwnerParameter(At<uint8_t*>(self, 0x20));
            At<float>(self, 0x2C) = 0.0f;
        }
        return 1;
    }
    if (!(0.0f == speed) && !(At<float>(self, 0x1C) > At<float>(self, 0x2C))) {
        parametricStatic_B0B134 = 0;
        At<float>(self, 0x28) = OwnerParameter(At<uint8_t*>(self, 0x20));
        return 1;
    }
    self[0x24] = 1;
    At<float>(self, 0x28) = OwnerParameter(owner);
    return 1;
}

// 0x607EB0 (thiscall): AIAimParametricTrack's fields
void __fastcall AIAimTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 1.0f;
    self[0x18] = 1;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x28) = 360.0f;
    At<float>(self, 0x2C) = 1.0f;
    SetNameRaw(self, 0x30, emptyString);
    At<uint32_t>(self, 0x34) = 0;
    self[8] = 0;
    At<uint32_t>(self, 0x3C) = 0;
    At<uint32_t>(self, 0x40) = 0;
}

// 0x607F50 (thiscall): AIAimParametricTrack::Start: the aim bone (the path of the two
// global names and +0x30)
uint8_t __fastcall AIAimTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<float>(self, 0x4C) = At<float>(self, 0x34);
    At<uint8_t*>(self, 0x44) = owner;
    uint32_t names[3];
    NameKey_FromHash(&names[0], &aimBoneA_1B6531C);
    NameKey_FromHash(&names[1], &aimBoneB_1B65394);
    NameKey_FromHash(&names[2], &At<uint32_t>(self, 0x30));
    int32_t path[2];
    HashPath_Set(path, 3, names);
    At<uint8_t*>(self, 0x48) = FindBoneInfo(path, aimBoneRoot_1B65398);
    for (int i = 2; i >= 0; i--) {
        NameKey_Destruct(&names[i]);
    }
    return 1;
}

// 0x607FE0 (thiscall): SpudCannonFireControllerTrack's fields
void __fastcall SpudCannonTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 5.0f;
    At<int32_t>(self, 0x1C) = 200;
    At<float>(self, 0x20) = 0.5f;
}

// 0x608030 (thiscall): SpudCannonFireControllerTrack::Start
uint8_t __fastcall SpudCannonTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    float first = At<float>(self, 0x18);
    At<uint8_t*>(self, 0x3C) = owner;
    At<float>(self, 0x28) = first;
    self[0x34] = 0;
    self[0x35] = 0;
    At<float>(self, 0x2C) = first;
    self[0x36] = 0;
    self[0x37] = 0;
    self[0x38] = 0;
    self[0x39] = 0;
    self[0x3A] = 0;
    return 1;
}

// 0x608070 (thiscall): AnimationDoorControlTrack's fields
void __fastcall DoorControlTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x1C) = 0.0f;
    At<uint32_t>(self, 0x18) = 0;
    At<float>(self, 0x20) = 0.0f;
}

// 0x6080C0 (thiscall): the door's state for a control (0..5)
int32_t __fastcall DoorControlTrack_StateFor(uint8_t*, re::Edx, int32_t control) {
    switch (control) {
    case 0:
        return 0x11;
    case 1:
        return 0x12;
    case 2:
        return 0xF;
    case 3:
        return 0xB;
    case 4:
        return 0x10;
    case 5:
        return 0xC;
    default:
        return 0;
    }
}

// 0x608120 / 0x608210 (thiscall): the parametric animation tracks' fields
void __fastcall AnimationTrack_InitFields_608120(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00607660)(self);
    At<float>(self, 0x58) = 0.0f;
    self[8] = 1;
    At<float>(self, 0x5C) = 1.0f;
}
void __fastcall AnimationTrack_InitFields_608210(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00607660)(self);
    At<float>(self, 0x58) = 0.0f;
    self[8] = 1;
    At<float>(self, 0x5C) = 1.0f;
    At<float>(self, 0x60) = 0.0f;
    At<float>(self, 0x64) = -1.0f;
}

// 0x608140 (thiscall): AnimationParametricTrack::Update: the animation's time set from
// the owner's parameter (+0x58..+0x5C onto +0x28..+0x2C), its previous time kept
uint8_t __fastcall AnimationParametricTrack_Update(uint8_t* self, re::Edx, float) {
    vc::Call<void>(self, acttree::kTrackResolve);
    uint8_t* blend = At<uint8_t*>(self, 0x4C);
    if (!blend || !At<uint8_t*>(blend, 0x94)) {
        return 0;
    }
    uint8_t* anim = Blend_GetAnim(blend, At<uint32_t>(self, 0x24));
    if (!anim) {
        return 0;
    }
    float from = At<float>(self, 0x28);
    float to = At<float>(self, 0x2C);
    if (to < 0.0f) {
        to = At<float>(At<uint8_t*>(anim, 0x18), 0xC);
    }
    float previous = At<float>(anim, 0x1C);
    At<float>(anim, 0x1C) = RemapRange(OwnerParameter(At<uint8_t*>(self, 0x40)), At<float>(self, 0x58),
                                       At<float>(self, 0x5C), from, to);
    At<float>(anim, 0x20) = previous;
    AnimManager_Update(animManager, At<uint8_t*>(self, 0x4C), anim, 0);
    return 1;
}

// 0x608270 (thiscall): AnimationDriverRotationTrack's fields
void __fastcall RotationTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x20) = 0.0f;
    At<uint32_t>(self, 0x1C) = 0;
    self[0x18] = 0;
    At<float>(self, 0x24) = 1.0f;
    self[0x19] = 1;
    At<float>(self, 0x28) = 0.0f;
    At<float>(self, 0x2C) = -1.0f;
    At<float>(self, 0x30) = 1.0f;
}

// 0x6082D0 (thiscall): AnimationDriverRotationTrack::Start (+0x19: the parameter from
// +0x20)
uint8_t __fastcall RotationTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x34) = owner;
    self[0x38] = 0;
    if (self[0x19] != 0) {
        *At<float*>(owner, 0x38) = At<float>(self, 0x20);
    }
    return 1;
}

// 0x6082F0 (thiscall): a rotation track's fields
void __fastcall RotationTrack_InitFields_6082F0(uint8_t* self, re::Edx) {
    float rate = rotationRate_B0B190;
    At<float>(self, 0x2C) = rate;
    self[0x28] = 1;
    At<float>(self, 0x40) = 0.0f;
    At<float>(self, 0x30) = 3.14159274f;
    self[0x38] = 0;
    self[0x39] = 0;
    At<float>(self, 0x34) = rate;
    self[0x3A] = 0;
    self[0x4C] = 0;
    At<float>(self, 0x18) = -1.0f;
    self[0x4D] = 0;
    At<float>(self, 0x1C) = -1.0f;
    At<float>(self, 0x24) = -1.0f;
    At<float>(self, 0x20) = -1.0f;
    At<float>(self, 0x44) = -1.0f;
    At<float>(self, 0x48) = -1.0f;
    At<float>(self, 0x3C) = 0.0f;
    At<float>(self, 0x50) = 100.0f;
    At<float>(self, 0x54) = 100.0f;
    At<float>(self, 0x58) = 100.0f;
    At<float>(self, 0x5C) = 0.9f;
    At<float>(self, 0x60) = 1.0f;
    At<float>(self, 0x64) = 1.0f;
    At<float>(self, 0x68) = 1.0f;
    At<float>(self, 0x6C) = 0.7f;
    At<float>(self, 0x70) = 0.5f;
    At<float>(self, 0x74) = 0.1f;
    At<float>(self, 0x78) = 0.0f;
}

// 0x608380 (cdecl): a heading turned from (x, y)'s towards to's (unset: none) by at most
// the angle between them: speed accelerates by rate (dir * accel) up to +-max
float __cdecl TurnHeading(float x, float y, float z, float toX, float toY, float toZ, float accel, float max,
                          float rate, float* speed) {
    float heading = AngleBetweenPoints(x, y, 0.0f, 0.0f);
    if (zeroVector[0] == toX && zeroVector[1] == toY && zeroVector[2] == toZ) {
        return heading;
    }
    V3 side = {y, -x, 0.0f};
    Vector_Normalise(&side);
    float c = toX * x + toY * y + z * toZ;
    float lateral = side.x * toX + side.y * toY + side.z * toZ;
    float angle = 0.0f;
    if (c < -1.0f) {
        c = -1.0f;
    } else if (1.0f < c) {
        c = 1.0f;
    }
    if (std::fabs(c - 1.0f) >= 1e-5f) {
        angle = SafeAcos(c);
    }
    float sign = 0.0f < lateral ? -1.0f : 1.0f;
    float s = sign * rate * accel + *speed;
    *speed = s;
    int32_t direction = s >= 0.0f ? 1 : -1;
    if (max < std::fabs(s) || max == std::fabs(s)) {
        *speed = float(direction) * max;
    }
    float step = max * sign * accel;
    if (!(std::fabs(step) < angle)) {
        step = angle * sign;
    }
    float target = AngleBetweenPoints(toX, toY, 0.0f, 0.0f);
    float diff = target - heading;
    if (std::fabs(diff) < std::fabs(step)) {
        step = diff;
    }
    return heading + step;
}

// 0x608630 (thiscall): +0x7C: the value mapped from +0x1C..+0x18 onto +0x20..+0x24
// (clamped); else -1
float __fastcall RangeMap_Get(uint8_t* self, re::Edx, float value) {
    if (self[0x7C] == 0) {
        return -1.0f;
    }
    float outFrom = At<float>(self, 0x20);
    float from = At<float>(self, 0x1C);
    float to = At<float>(self, 0x18);
    float outTo = At<float>(self, 0x24);
    if (to < value || to == value) {
        return outTo;
    }
    if (from < value) {
        return (value - from) / (to - from) * (outTo - outFrom) + outFrom;
    }
    return outFrom;
}

} // namespace

#define ACTTRACKSANIM_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

ACTTRACKSANIM_REVERSED("ActionTracks::ExpSeries_607420", 0x00607420, &ExpSeries);
ACTTRACKSANIM_REVERSED("Entries2C::GetAt_607480", 0x00607480, &Entries2C_GetAt);
ACTTRACKSANIM_REVERSED("Entries2C::Find_6074A0", 0x006074A0, &Entries2C_Find);
ACTTRACKSANIM_REVERSED("Anim::GetBoneTravel_607500", 0x00607500, &Anim_GetBoneTravel);
ACTTRACKSANIM_REVERSED("Request::Set_607580", 0x00607580, &Request_Set);
ACTTRACKSANIM_REVERSED("ActionTracks::TypeId_6075F0", 0x006075F0, &TypeIdFn<0x01B65254, 0x01B65250>);
ACTTRACKSANIM_REVERSED("AnimationGroupReleaseTrack::DeletingDestruct", 0x00607630,
                       &TrackDeletingDtor<kVtAnimationGroupReleaseTrack, 0x18>);
ACTTRACKSANIM_REVERSED("AnimationTrack::InitFields", 0x00607660, &AnimationTrack_InitFields);
ACTTRACKSANIM_REVERSED("AnimationTrack::IsA", 0x006076E0, &IsA<0x01B6525C, 0x01B65258, 0x004437B0>);
ACTTRACKSANIM_REVERSED("AnimationTrack::DeletingDestruct", 0x00607730, &TrackDeletingDtor<kVtAnimationTrack, 0x1C, 0x18>);
ACTTRACKSANIM_REVERSED("AnimDict::IndexOf_607770", 0x00607770, &AnimDict_IndexOf);
ACTTRACKSANIM_REVERSED("JointDriverTrack::GetLength", 0x006077A0, &JointDriverTrack_GetLength);
ACTTRACKSANIM_REVERSED("AnimationTrack::InitFields_6077E0", 0x006077E0, &AnimationTrack_InitFields_6077E0);
ACTTRACKSANIM_REVERSED("AnimationTrack::InitFields_607800", 0x00607800, &AnimationTrack_InitFields_607800);
ACTTRACKSANIM_REVERSED("HitAnimationTrack::GetBlendOut", 0x00607830, &HitAnimationTrack_GetBlendOut);
ACTTRACKSANIM_REVERSED("HitReactTrack::InitFields", 0x006078A0, &HitReactTrack_InitFields);
ACTTRACKSANIM_REVERSED("HitReactTrack::DeletingDestruct", 0x006078F0, &TrackDeletingDtor<kVtHitReactTrack, 0x18>);
ACTTRACKSANIM_REVERSED("JointDriverTrack::InitFields", 0x00607920, &JointDriverTrack_InitFields);
ACTTRACKSANIM_REVERSED("JointDriverTrack::DeletingDestruct", 0x00607990,
                       &TrackDeletingDtor<kVtJointDriverTrack, 0x64, 0x5C, kVtAnimationTrack, 0x1C, 0x18>);
ACTTRACKSANIM_REVERSED("JointDriverTrack::Resolve", 0x006079E0, &JointDriverTrack_Resolve);
ACTTRACKSANIM_REVERSED("JointDriverTrack::InitFields_6079F0", 0x006079F0, &JointDriverTrack_InitFields_6079F0);
ACTTRACKSANIM_REVERSED("JointDriverTrack::InitFields_607A00", 0x00607A00, &JointDriverTrack_InitFields_607A00);
ACTTRACKSANIM_REVERSED("JointDriverTrack::Destruct", 0x00607A20,
                       &TrackDtor<kVtJointDriverTrack, 0x64, 0x5C, kVtAnimationTrack, 0x1C, 0x18>);
ACTTRACKSANIM_REVERSED("JointDriverParametricTrack::Update", 0x00607A60, &JointDriverParametricTrack_Update);
ACTTRACKSANIM_REVERSED("AnimationDriverSteerTrack::InitFields", 0x00607B40, &SteerTrack_InitFields);
ACTTRACKSANIM_REVERSED("AnimationDriverSteerTrack::DeletingDestruct", 0x00607B60, &TrackDeletingDtor<kVtSteerTrack>);
ACTTRACKSANIM_REVERSED("AnimationDriverStruggleTrack::InitFields", 0x00607B90, &StruggleTrack_InitFields);
ACTTRACKSANIM_REVERSED("AnimationDriverStruggleTrack::DeletingDestruct", 0x00607BE0, &TrackDeletingDtor<kVtStruggleTrack>);
ACTTRACKSANIM_REVERSED("AnimationDriverStruggleTrack::Start", 0x00607C10, &StruggleTrack_Start);
ACTTRACKSANIM_REVERSED("AnimationDriverTargetSyncTrack::DeletingDestruct", 0x00607C70,
                       &TrackDeletingDtor<kVtTargetSyncTrack>);
ACTTRACKSANIM_REVERSED("AnimationDriverTargetSyncTrack::InitFields", 0x00607CA0, &TargetSyncTrack_InitFields);
ACTTRACKSANIM_REVERSED("ParametricAnimationControllerTrack::DeletingDestruct", 0x00607CD0,
                       &TrackDeletingDtor<kVtParametricControllerTrack>);
ACTTRACKSANIM_REVERSED("ParametricAnimationControllerTrack::Start", 0x00607D00, &ParametricControllerTrack_Start);
ACTTRACKSANIM_REVERSED("ParametricTrackerTrack::InitFields", 0x00607D20, &ParametricTrackerTrack_InitFields);
ACTTRACKSANIM_REVERSED("ParametricTrackerTrack::Construct", 0x00607D40, &ParametricTrackerTrack_Construct);
ACTTRACKSANIM_REVERSED("ParametricTrackerTrack::Start", 0x00607D60, &ParametricTrackerTrack_Start);
ACTTRACKSANIM_REVERSED("ParametricTrackerTrack::Update", 0x00607D80, &ParametricTrackerTrack_Update);
ACTTRACKSANIM_REVERSED("AIAimParametricTrack::InitFields", 0x00607EB0, &AIAimTrack_InitFields);
ACTTRACKSANIM_REVERSED("AIAimParametricTrack::DeletingDestruct", 0x00607F10, &TrackDeletingDtor<kVtAIAimTrack, 0x38, 0x30>);
ACTTRACKSANIM_REVERSED("AIAimParametricTrack::Start", 0x00607F50, &AIAimTrack_Start);
ACTTRACKSANIM_REVERSED("SpudCannonFireControllerTrack::InitFields", 0x00607FE0, &SpudCannonTrack_InitFields);
ACTTRACKSANIM_REVERSED("SpudCannonFireControllerTrack::DeletingDestruct", 0x00608000,
                       &TrackDeletingDtor<kVtSpudCannonTrack>);
ACTTRACKSANIM_REVERSED("SpudCannonFireControllerTrack::Start", 0x00608030, &SpudCannonTrack_Start);
ACTTRACKSANIM_REVERSED("AnimationDoorControlTrack::InitFields", 0x00608070, &DoorControlTrack_InitFields);
ACTTRACKSANIM_REVERSED("AnimationDoorControlTrack::DeletingDestruct", 0x00608090, &TrackDeletingDtor<kVtDoorControlTrack>);
// (regcheck: its caller 0x60C060 (AnimationDoorControlTrack::Update, ActionTracksAnim2.cpp)
// relies on ecx and edx surviving the call: turn on together with that one)
ACTTRACKSANIM_REVERSED("AnimationDoorControlTrack::StateFor", 0x006080C0, &DoorControlTrack_StateFor);
ACTTRACKSANIM_REVERSED("AnimationTrack::InitFields_608120", 0x00608120, &AnimationTrack_InitFields_608120);
ACTTRACKSANIM_REVERSED("AnimationParametricTrack::Update", 0x00608140, &AnimationParametricTrack_Update);
ACTTRACKSANIM_REVERSED("AnimationTrack::InitFields_608210", 0x00608210, &AnimationTrack_InitFields_608210);
ACTTRACKSANIM_REVERSED("AnimationTrack::Destruct", 0x00608240, &TrackDtor<kVtAnimationTrack, 0x1C, 0x18>);
ACTTRACKSANIM_REVERSED("AnimationDriverRotationTrack::InitFields", 0x00608270, &RotationTrack_InitFields);
ACTTRACKSANIM_REVERSED("AnimationDriverRotationTrack::DeletingDestruct", 0x006082A0, &TrackDeletingDtor<kVtRotationTrack>);
ACTTRACKSANIM_REVERSED("AnimationDriverRotationTrack::Start", 0x006082D0, &RotationTrack_Start);
ACTTRACKSANIM_REVERSED("AnimationDriverRotationTrack::InitFields_6082F0", 0x006082F0, &RotationTrack_InitFields_6082F0);
ACTTRACKSANIM_REVERSED("ActionTracks::TurnHeading_608380", 0x00608380, &TurnHeading);
ACTTRACKSANIM_REVERSED("ActionTracks::RangeMap_608630", 0x00608630, &RangeMap_Get);
