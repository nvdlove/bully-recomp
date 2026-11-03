// A ped's facial animation (CPed +0x1CC, 0xA0 bytes: lids, eyes, mouth, brows), the lip
// store's slots it reads, and the ped's head-look range. 0x68E3C0..0x68EFF0.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

// Moves `value` towards `target` at `speed` a second (the sign of speed says which way)
struct FacialApproach {
    float value;              // +0x00
    float speed;              // +0x04
    float target;             // +0x08
};
static_assert(sizeof(FacialApproach) == 0xC);

struct FacialLids {
    float value;              // +0x00
    float unknown04;          // +0x04
    float unknown08;          // +0x08
    int32_t mode;             // +0x0C
    float rate;               // +0x10 (1)
    int32_t unknown14;        // +0x14
    int32_t time;             // +0x18
    float unknown1C;          // +0x1C (10)
};
static_assert(sizeof(FacialLids) == 0x20);

struct FacialEyes {
    FacialApproach x;         // +0x00
    FacialApproach y;         // +0x0C
    int32_t mode;             // +0x18
    float speed;              // +0x1C (6)
    uint32_t startTime;       // +0x20
    int32_t interval;         // +0x24
};
static_assert(sizeof(FacialEyes) == 0x28);

struct FacialMouth {
    uint32_t lip;             // +0x00 a lip store handle (index | serial << 16)
    FacialApproach open;      // +0x04
    float unknown10;          // +0x10 (5)
    int32_t mode;             // +0x14 1: follows the lip
};
static_assert(sizeof(FacialMouth) == 0x18);

struct FacialBrows {
    re::Ptr32<uint8_t> ped;             // +0x00
    int32_t unknown04;        // +0x04
    int32_t madAnim;          // +0x08
    int32_t surpriseAnim;     // +0x0C
    int32_t confusedAnim;     // +0x10
    int32_t unknown14;        // +0x14
    int32_t unknown18;        // +0x18 (2)
    float unknown1C[3];       // +0x1C
    float unknown28;          // +0x28 (1)
    uint8_t unknown2C;        // +0x2C
    uint8_t pad2D[3];
    uint32_t startTime;       // +0x30
    int32_t duration;         // +0x34 0: none
    int32_t mode;             // +0x38
};
static_assert(sizeof(FacialBrows) == 0x3C);

struct CFacialAnim {
    re::Ptr32<uint8_t> ped;             // +0x00
    FacialLids lids;          // +0x04
    FacialEyes eyes;          // +0x24
    FacialMouth mouth;        // +0x4C
    FacialBrows brows;        // +0x64
};
static_assert(offsetof(CFacialAnim, eyes) == 0x24);
static_assert(offsetof(CFacialAnim, mouth) == 0x4C);
static_assert(offsetof(CFacialAnim, brows) == 0x64);
static_assert(sizeof(CFacialAnim) == 0xA0);

// The lip store (one object at 0x208B9F0): two playing lips, their handles' serials
struct LipSlot {
    uint8_t unknown00[0xC];
    float value;              // +0x0C the mouth's opening now
    uint8_t unknown10[4];
};
static_assert(sizeof(LipSlot) == 0x14);
constexpr int kLipSlotsOffset = 0x1924;
constexpr int kLipSerialsOffset = 0x194C;
uint8_t* const kLipStore = reinterpret_cast<uint8_t*>(0x0208B9F0);

using re::At;

GAME_VAR(uint32_t, g_timeInMs, 0x00C1A9B4);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_facialFrameSeconds, 0x00BA5C70);  // (0.02)
GAME_VAR(int32_t, g_eyeIntervalMin, 0x00BA5C68);    // (200)
GAME_VAR(int32_t, g_eyeIntervalMax, 0x00BA5C6C);    // (3000)
GAME_VAR(int32_t, g_blinkIntervalMin, 0x00BA5C74);
GAME_VAR(int32_t, g_blinkIntervalMax, 0x00BA5C78);
GAME_VAR(uint8_t*, g_cameraSettings, 0x020C88B8);   // (+0xDC.. the head-look limits, degrees)
GAME_VAR(int32_t, g_headBoneId, 0x01B6530C);
GAME_ARRAY(float, g_upVector, 0x009E33DC);
void* const kAnimManager = reinterpret_cast<void*>(0x020C4910);

// CFacialAnim::Update's function statics (guard bits at 0x208D384)
GAME_VAR(uint32_t, s_facialGuard, 0x0208D384);
GAME_ARRAY(float, s_zero, 0x0208D378);
GAME_ARRAY(float, s_zAxis1, 0x0208D36C);
GAME_ARRAY(float, s_jawAxis, 0x0208D358);
GAME_VAR(float, s_jawOpenAngle, 0x0208D368);
GAME_VAR(float, s_eyesOpenAngle, 0x0208D364);
GAME_VAR(float, s_jawClosedAngle, 0x0208D350);
GAME_VAR(float, s_eyesClosedAngle, 0x0208D348);
GAME_VAR(int32_t, s_jawBone, 0x0208D354);
GAME_VAR(int32_t, s_eyesBone, 0x0208D34C);

inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, LipSlot_Update, 0x0068E300, LipSlot* slot);
GAME_FN(float, __thiscall, LipStore_GetValue, 0x0068DFD0, void* store, uint32_t handle);
GAME_FN(void, __thiscall, Approach_Update, 0x0068E470, FacialApproach* self, float seconds);
GAME_FN(void, __thiscall, Mouth_SetLip, 0x0068E660, FacialMouth* mouth, uint32_t handle);
GAME_FN(void, __thiscall, Mouth_Update, 0x0068E950, FacialMouth* mouth);
GAME_FN(void, __thiscall, Brows_LoadAnims, 0x0068E9E0, FacialBrows* brows);
GAME_FN(CFacialAnim*, __thiscall, FacialAnim_Construct, 0x0068EB00, CFacialAnim* self, uint8_t* ped);
GAME_FN(void, __thiscall, FacialAnim_Update, 0x0068EC20, CFacialAnim* self);
GAME_FN(void, __thiscall, Ped_UpdateHeadLookRange, 0x0068E6A0, uint8_t* ped);
GAME_FN(int32_t, __cdecl, Clump_FindBone, 0x0068E4E0, void* clump, const char* name);
GAME_FN(int, __thiscall, AnimDict_Find, 0x006B1D60, void* dict, int unknown, const char* name, int32_t* index);
GAME_FN(int32_t, __cdecl, Entity_FindBone, 0x00655620, void* entity, int32_t id);
GAME_FN(void, __thiscall, AnimManager_GetBoneMatrix, 0x006BF570, void* manager, void* clump, int32_t bone, void* out, uintptr_t);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const float* v);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);
GAME_FN(void*, __cdecl, Transform_SetRotation, 0x00415EB0, void* matrix, const float* axis, float angle, int degrees);
GAME_FN(void, __thiscall, Constraints_Add, 0x0072EE50, void* constraints, void* constraint, int32_t kind);
GAME_FN(void, __thiscall, Constraints_Remove, 0x0072EEB0, void* constraints, void* constraint);
GAME_FN(void, __thiscall, Constraints_Update, 0x007331B0, void* constraints, float seconds);
GAME_FN(uint8_t, __thiscall, Ally_IsFollower, 0x00485320, void* ally);
GAME_FN(void, __thiscall, Ped_UpdateFeet, 0x0047A4C0, uint8_t* ped);

// rand() scaled to lo .. hi - 1 (the game's integer maths: / 32767, towards 0)
int RandomBetween(int lo, int hiExclusive) {
    int span = hiExclusive - 1 - lo;
    return Rand() * span / 32767 + lo;
}

// a ped's IK chain object's virtual (thiscall)
template <int Slot, typename... A>
void ChainCall(void* chain, A... args) {
    reinterpret_cast<void(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(chain))[Slot / 4]))(chain, args...);
}

} // namespace

namespace facial {

// 0x68E3C0: a lip stopped: its slot's value 0, a ped's mouth let go of it, its serial
// cleared (when it's still that lip's)
void __fastcall LipStore_Stop(uint8_t* store, re::Edx, uint32_t handle, uint8_t* entity) {
    if (handle == 0) {
        return;
    }
    uint32_t index = handle & 0xFFFF;
    uint32_t serial = handle >> 16;
    uint32_t& slotSerial = At<uint32_t>(store, kLipSerialsOffset + int(index) * 4);
    if (slotSerial != serial || index == 0xFFFFFFFFu) {
        return;
    }
    At<float>(store, 0x1930 + int(index) * 0x14) = 0.0f;
    if (entity != nullptr && (entity[0x108] & 7) == 3) {
        CFacialAnim* anim = At<CFacialAnim*>(entity, 0x1CC);
        if (anim != nullptr) {
            Mouth_SetLip(&anim->mouth, 0);
        }
    }
    if (slotSerial == serial) {
        slotSerial = 0;
    }
}

// 0x68E440: the playing lips updated
void __cdecl LipStore_UpdateAll() {
    for (int i = 0; i < 2; i++) {
        if (At<uint32_t>(kLipStore, kLipSerialsOffset + i * 4) != 0) {
            LipSlot_Update(reinterpret_cast<LipSlot*>(kLipStore + kLipSlotsOffset + i * 0x14));
        }
    }
}

// 0x68E470: stops at the target (speed 0 then)
void __fastcall Approach_UpdateImpl(FacialApproach* self, re::Edx, float seconds) {
    if (self->speed > 0.0f) {
        self->value = seconds * self->speed + self->value;
        if (!(self->target > self->value)) {
            self->value = self->target;
            self->speed = 0.0f;
        }
    } else if (self->speed < 0.0f) {
        self->value = seconds * self->speed + self->value;
        if (!(self->target < self->value)) {
            self->value = self->target;
            self->speed = 0.0f;
        }
    }
}

// 0x68E4E0: a bone's index in a clump's hierarchy (by its name's hash); -1: none
int32_t __cdecl Clump_FindBoneImpl(uint8_t* clump, const char* name) {
    uint8_t* hierarchy = At<uint8_t*>(At<uint8_t*>(clump, 0x94), 0x10);
    int32_t count = At<int32_t>(hierarchy, 0x5C);
    uint32_t hash = HashName(name);
    for (int32_t i = 0; i < count; i++) {
        // (the game's bounds-checked element: null past the count)
        uint32_t* entry = uint32_t(i) < At<uint32_t>(hierarchy, 0x5C)
                              ? reinterpret_cast<uint32_t*>(At<uint8_t*>(hierarchy, 0x60) + i * 0xC + 4)
                              : nullptr;
        if (hash == *entry) {
            return i;
        }
    }
    return -1;
}

// towards a target: the eyes' speed with the sign that gets there
static float Towards(float value, float target, float speed) {
    return value < target ? speed : -speed;
}

// 0x68E540: the eyes look at (x, y)
void __fastcall Eyes_SetLook(FacialEyes* self, re::Edx, const float* look) {
    float speed = self->speed;
    self->x.target = look[0];
    self->x.speed = Towards(self->x.value, self->x.target, speed);
    speed = self->speed;
    self->y.target = look[1];
    self->y.speed = Towards(self->y.value, self->y.target, speed);
}

// 0x68E5A0: the eyes' speed (both ways re-aimed)
void __fastcall Eyes_SetSpeed(FacialEyes* self, re::Edx, float speed) {
    self->speed = speed;
    self->x.speed = Towards(self->x.value, self->x.target, speed);
    self->y.speed = Towards(self->y.value, self->y.target, speed);
}

// 0x68E5F0: mode 0 (blinking): the time to the next blink at random; otherwise `time`
void __fastcall Lids_SetMode(FacialLids* self, re::Edx, int mode, int time) {
    self->mode = mode;
    if (mode == 0) {
        int ms = RandomBetween(g_blinkIntervalMin, g_blinkIntervalMax);
        self->time = int32_t(float(ms) / self->rate);  // (_ftol)
    } else {
        self->time = time;
    }
}

// 0x68E660: the lip the mouth follows (0: none)
void __fastcall Mouth_SetLipImpl(FacialMouth* self, re::Edx, uint32_t handle) {
    self->open.speed = 0.0f;
    self->open.target = 0.0f;
    self->lip = handle;
    self->open.value = 0.0f;
    self->mode = handle != 0 ? 1 : 0;
}

// 0x68E690 (the time unused)
void __fastcall Mouth_SetMode(FacialMouth* self, re::Edx, int mode, int) {
    self->mode = mode;
}

// 0x68E6A0: where the camera is against the head's facing, as 0 .. 1 across the
// camera settings' limits (0.5 each when the ped has no head bone)
void __fastcall Ped_UpdateHeadLookRangeImpl(uint8_t* ped, re::Edx) {
    float& horizontal = At<float>(ped, 0x1E7C);
    float& vertical = At<float>(ped, 0x1E80);
    horizontal = 0.5f;
    vertical = 0.5f;
    int32_t bone = Entity_FindBone(ped, g_headBoneId);
    void* clump = At<void*>(ped, 0x114);
    if (bone == -1 || clump == nullptr) {
        return;
    }
    float transform[13];  // an NiTransform: rotation, translation, scale (preset 1)
    transform[12] = 1.0f;
    AnimManager_GetBoneMatrix(kAnimManager, clump, bone, transform, 0);
    // (the rotation's second column)
    float column[3] = {transform[1], transform[4], transform[7]};
    float* matrix = Entity_GetMatrix(ped);
    float forward[3] = {matrix[4], matrix[5], matrix[6]};
    double headHeading = HeadingFromVector(column);
    float yaw = LimitAngle(float(headHeading - double(HeadingFromVector(forward))));
    float dot = column[1] * g_upVector[1] + g_upVector[0] * column[0] + column[2] * g_upVector[2];
    float pitch = LimitAngle(float(1.5707963705062866 - double(SafeAcos(dot))));
    uint8_t* cam = g_cameraSettings;
    const double pi = 3.1415927410125732;
    float yawMin = float(double(float(double(At<float>(cam, 0xDC)) * pi)) / 180.0);
    float pitchMin = float(double(float(double(At<float>(cam, 0xE4)) * pi)) / 180.0);
    float pitchMax = float(double(float(double(At<float>(cam, 0xE8)) * pi)) / 180.0);
    float yawMax = float(double(float(double(At<float>(cam, 0xE0)) * pi)) / 180.0);
    horizontal = (yaw - yawMin) / (yawMax - yawMin);
    if (horizontal < 0.0f) {
        horizontal = 0.0f;
    } else if (horizontal > 1.0f) {
        horizontal = 1.0f;
    }
    vertical = (pitch - pitchMin) / (pitchMax - pitchMin);
    if (vertical < 0.0f) {
        vertical = 0.0f;
    } else if (vertical > 1.0f) {
        vertical = 1.0f;
    }
}

// 0x68E890: a mode, looking `amount` to one side (the other side about half the time),
// the time to the next look at random (`time` itself unused: only -1 picks one;
// mode 0 triples it)
void __fastcall Eyes_SetMode(FacialEyes* self, re::Edx, int mode, float amount, int time) {
    self->mode = mode;
    if (Rand() * 99 / 32767 <= 50) {
        amount = -amount;
    }
    float speed = self->speed;
    self->x.target = amount;
    self->x.speed = Towards(self->x.value, amount, speed);
    self->startTime = g_timeInMs;
    if (time == -1) {
        self->interval = RandomBetween(g_eyeIntervalMin, g_eyeIntervalMax);
    }
    if (mode == 0) {
        self->interval = self->interval * 3;
    }
}

// 0x68E950: the opening moves; following a lip: the lip's value
void __fastcall Mouth_UpdateImpl(FacialMouth* self, re::Edx) {
    // (this frame in whole milliseconds, back in seconds)
    int32_t ms = int32_t(int64_t(float(double(g_facialFrameSeconds * g_timeStep) * 1000.0)));
    Approach_Update(&self->open, float(double(ms) * 0.001));
    if (self->mode != 0 && self->mode == 1) {
        self->open.value = LipStore_GetValue(kLipStore, self->lip);
        self->open.target = 0.0f;
    }
}

// the brows' expression animations (by name, in the ped's anim dictionary; -1: none)
static int32_t BrowAnim(FacialBrows* self, const char* name) {
    void* dict = At<void*>(At<uint8_t*>(At<uint8_t*>(self->ped, 0x114), 0x94), 0x10);
    if (dict == nullptr) {
        return -1;
    }
    int32_t index = -1;
    if (AnimDict_Find(dict, 0, name, &index) == 0) {
        return -1;
    }
    return index;
}

// 0x68E9E0
void __fastcall Brows_LoadAnimsImpl(FacialBrows* self, re::Edx) {
    self->unknown04 = 0;
    self->unknown14 = 0;
    if (self->ped == nullptr) {
        self->madAnim = 0;
        self->surpriseAnim = 0;
        self->confusedAnim = 0;
        return;
    }
    self->confusedAnim = BrowAnim(self, "C_PLAYER\\EBROW_CONFUSED");
    self->madAnim = BrowAnim(self, "C_PLAYER\\EBROW_MAD");
    self->surpriseAnim = BrowAnim(self, "C_PLAYER\\EBROW_SURPRISE");
}

// 0x68EAD0 (mode 4 draws a random number it doesn't use)
void __fastcall Brows_SetMode(FacialBrows* self, re::Edx, int mode, int time) {
    if (mode == 4) {
        Rand();
    }
    self->mode = mode;
    self->startTime = g_timeInMs;
    self->duration = time;
}

// 0x68EB00
CFacialAnim* __fastcall FacialAnim_ConstructImpl(CFacialAnim* self, re::Edx, uint8_t* ped) {
    self->lids.value = 0.0f;
    self->lids.unknown04 = 0.0f;
    self->lids.mode = 0;
    self->lids.unknown08 = 0.0f;
    self->lids.unknown14 = 0;
    self->lids.time = 0;
    self->lids.unknown1C = 10.0f;
    self->lids.rate = 1.0f;
    self->eyes.x = {0.0f, 0.0f, 0.0f};
    self->eyes.y = {0.0f, 0.0f, 0.0f};
    self->eyes.mode = 0;
    self->eyes.speed = 6.0f;
    self->eyes.startTime = 0;
    self->eyes.interval = 0;
    self->mouth.open = {0.0f, 0.0f, 0.0f};
    self->mouth.lip = 0;
    self->mouth.unknown10 = 5.0f;
    self->mouth.mode = 0;
    FacialBrows* brows = &self->brows;
    brows->unknown1C[0] = 0.0f;
    brows->unknown1C[1] = 0.0f;
    brows->unknown1C[2] = 0.0f;
    brows->unknown18 = 2;
    brows->mode = 0;
    brows->unknown28 = 1.0f;
    self->ped = ped;
    brows->ped = ped;
    Brows_LoadAnims(brows);
    brows->mode = 0;
    brows->startTime = g_timeInMs;
    brows->duration = 0;
    brows->unknown2C = 0;
    return self;
}

// 0x68EBB0: the ped's facial animation made or deleted
void __fastcall Ped_SetFacialAnim(uint8_t* ped, re::Edx, int on) {
    re::Ptr32<CFacialAnim>& anim = At<CFacialAnim*>(ped, 0x1CC);
    if (!re::Flag(on)) {
        if (anim != nullptr) {
            Free(anim);
            anim = nullptr;
        }
    } else if (anim == nullptr) {
        void* memory = Malloc(sizeof(CFacialAnim));
        anim = memory != nullptr ? FacialAnim_Construct(static_cast<CFacialAnim*>(memory), ped) : nullptr;
    }
}

// 0x68EC20: the jaw and eye bones turned by the mouth's opening; the brows' expression
// over when its time is up
void __fastcall FacialAnim_UpdateImpl(CFacialAnim* self, re::Edx) {
    if ((s_facialGuard & 1) == 0) {
        s_zero[0] = 0.0f;
        s_facialGuard |= 1;
        s_zero[1] = 0.0f;
        s_zero[2] = 0.0f;
    }
    if ((s_facialGuard & 2) == 0) {
        s_facialGuard |= 2;
        s_zAxis1[0] = 0.0f;
        s_zAxis1[1] = 0.0f;
        s_zAxis1[2] = 1.0f;
    }
    uint8_t* bones = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self->ped, 0x114), 0x14), 8);
    if ((s_facialGuard & 0x10) == 0) {
        s_jawOpenAngle = At<float>(g_cameraSettings, 0xF4);
        s_facialGuard |= 0x10;
    }
    if ((s_facialGuard & 0x20) == 0) {
        s_eyesOpenAngle = At<float>(g_cameraSettings, 0xF8);
        s_facialGuard |= 0x20;
    }
    if ((s_facialGuard & 0x40) == 0) {
        s_facialGuard |= 0x40;
        s_jawAxis[0] = 0.0f;
        s_jawAxis[1] = 0.0f;
        s_jawAxis[2] = 1.0f;
    }
    Mouth_Update(&self->mouth);
    float open = self->mouth.open.value;
    if ((s_facialGuard & 0x80) == 0) {
        s_facialGuard |= 0x80;
        s_jawBone = Clump_FindBone(At<void*>(self->ped, 0x114), "Jaw");
    }
    Transform_SetRotation(bones + s_jawBone * 0x34, s_jawAxis,
                          s_jawClosedAngle + open * (s_jawOpenAngle - s_jawClosedAngle), 1);
    if ((s_facialGuard & 0x100) == 0) {
        s_facialGuard |= 0x100;
        s_eyesBone = Clump_FindBone(At<void*>(self->ped, 0x114), "Eyes");
    }
    uint8_t* eyes = bones + s_eyesBone * 0x34;
    float axis[3] = {At<float>(eyes, 0), At<float>(eyes, 0xC), At<float>(eyes, 0x18)};
    Transform_SetRotation(eyes, axis, s_eyesClosedAngle + open * (s_eyesOpenAngle - s_eyesClosedAngle), 1);
    FacialBrows& brows = self->brows;
    if (brows.duration != 0 && int32_t(g_timeInMs) >= int32_t(brows.startTime + brows.duration)) {
        brows.mode = 0;
        if (brows.unknown2C == 0) {
            brows.startTime = g_timeInMs;
            brows.duration = 0;
            return;
        }
        brows.startTime = g_timeInMs;
        brows.unknown2C = 0;
        brows.duration = 0;
    }
}

// 0x68EE40 (cdecl; the first argument unused): a ped's face and head each frame: the
// facial animation, the head-look range, the IK chains (+0x16F0 head, +0x16F4 / +0x16F8
// the arms) on or off by what the ped does, then the feet
void __cdecl Ped_UpdateFaceAndChains(void*, uint8_t* ped) {
    if (At<CFacialAnim*>(ped, 0x1CC) != nullptr) {
        FacialAnim_Update(At<CFacialAnim*>(ped, 0x1CC));
    }
    Ped_UpdateHeadLookRange(ped);
    ChainCall<0x54>(At<void*>(ped, 0x16F0), 1);
    if (At<int32_t>(ped, 0x13A4) != 0) {
        Constraints_Add(At<void*>(ped, 0x2EC), At<void*>(ped, 0x16F0), -1);
        ChainCall<0x50>(At<void*>(ped, 0x16F0), 1);
    } else if (At<int32_t>(ped, 0x1478) != 0) {
        ChainCall<0x50>(At<void*>(ped, 0x16F0), 0);
    } else {
        ChainCall<0x48>(At<void*>(ped, 0x16F0));
    }
    void* last;
    if (At<int32_t>(ped, 0x153C) != 0) {
        ChainCall<0x54>(At<void*>(ped, 0x16F4), 0);
        ChainCall<0x54>(At<void*>(ped, 0x16F8), 0);
        bool follower = Ally_IsFollower(ped + 0x2F0) != 0;
        bool other = At<int32_t>(ped, 0x1540) != 0;
        if (follower == other) {
            // (following with +0x1540 set, or neither: the left one)
            Constraints_Add(At<void*>(ped, 0x2EC), At<void*>(ped, 0x16F8), 0x16);
            last = At<void*>(ped, 0x16F4);
        } else {
            Constraints_Add(At<void*>(ped, 0x2EC), At<void*>(ped, 0x16F4), 0x1E);
            last = At<void*>(ped, 0x16F8);
        }
        ChainCall<0x48>(last);
    } else if (At<int32_t>(ped, 0x148C) != 0) {
        ChainCall<0x54>(At<void*>(ped, 0x16F4), 0);
        ChainCall<0x54>(At<void*>(ped, 0x16F8), 0);
        if (At<int32_t>(ped, 0x1488) != 0) {
            Constraints_Remove(At<void*>(ped, 0x2EC), At<void*>(ped, 0x16F4));
            Constraints_Add(At<void*>(ped, 0x2EC), At<void*>(ped, 0x16F4), 0x1E);
        } else {
            Constraints_Remove(At<void*>(ped, 0x2EC), At<void*>(ped, 0x16F8));
            Constraints_Add(At<void*>(ped, 0x2EC), At<void*>(ped, 0x16F8), 0x16);
        }
    } else {
        ChainCall<0x48>(At<void*>(ped, 0x16F4));
        ChainCall<0x48>(At<void*>(ped, 0x16F8));
    }
    if (At<void*>(ped, 0x2EC) != nullptr) {
        Constraints_Update(At<void*>(ped, 0x2EC), g_facialFrameSeconds * g_timeStep);
    }
    Ped_UpdateFeet(ped);
}

} // namespace facial

#define FACIAL_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(facial_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(facial_original_, __LINE__), \
             re::Mode::Original)

FACIAL_REVERSED("CLipStore::StopLip", 0x0068E3C0, &facial::LipStore_Stop);
FACIAL_REVERSED("CLipStore::UpdateAll", 0x0068E440, &facial::LipStore_UpdateAll);
FACIAL_REVERSED("FacialApproach::Update", 0x0068E470, &facial::Approach_UpdateImpl);
FACIAL_REVERSED("Clump::FindBoneByName_68E4E0", 0x0068E4E0, &facial::Clump_FindBoneImpl);
FACIAL_REVERSED("FacialEyes::SetLook", 0x0068E540, &facial::Eyes_SetLook);
FACIAL_REVERSED("FacialEyes::SetSpeed", 0x0068E5A0, &facial::Eyes_SetSpeed);
FACIAL_REVERSED("FacialLids::SetMode", 0x0068E5F0, &facial::Lids_SetMode);
FACIAL_REVERSED("FacialMouth::SetLip", 0x0068E660, &facial::Mouth_SetLipImpl);
FACIAL_REVERSED("FacialMouth::SetMode", 0x0068E690, &facial::Mouth_SetMode);
FACIAL_REVERSED("CPed::UpdateHeadLookRange", 0x0068E6A0, &facial::Ped_UpdateHeadLookRangeImpl);
FACIAL_REVERSED("FacialEyes::SetMode", 0x0068E890, &facial::Eyes_SetMode);
FACIAL_REVERSED("FacialMouth::Update", 0x0068E950, &facial::Mouth_UpdateImpl);
FACIAL_REVERSED("FacialBrows::LoadAnims", 0x0068E9E0, &facial::Brows_LoadAnimsImpl);
FACIAL_REVERSED("FacialBrows::SetMode", 0x0068EAD0, &facial::Brows_SetMode);
FACIAL_REVERSED("CFacialAnim::CFacialAnim", 0x0068EB00, &facial::FacialAnim_ConstructImpl);
FACIAL_REVERSED("CPed::SetFacialAnim", 0x0068EBB0, &facial::Ped_SetFacialAnim);
FACIAL_REVERSED("CFacialAnim::Update", 0x0068EC20, &facial::FacialAnim_UpdateImpl);
FACIAL_REVERSED("CPed::UpdateFaceAndChains", 0x0068EE40, &facial::Ped_UpdateFaceAndChains);
