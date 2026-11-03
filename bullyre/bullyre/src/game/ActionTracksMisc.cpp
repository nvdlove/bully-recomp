// The odd ones among the action tree's small class members of 0x630E80..0x64B1F0 (the
// rest share the shapes of ActionFactories.h): the mission tracks' and a few others'
// IsA, the collision / weapon animation tracks' constructors with their 16 hits, the
// dice roll, the opportunity tracks' end, two float getters.

#include <cstddef>
#include <cstdint>

#include "game/ActionFactories.h"
#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

using actfac::At;

inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __thiscall, Track_Construct, 0x0061A880, uint8_t* track);
GAME_FN(void, __thiscall, NameKey_Default, 0x0066BEA0, uint8_t* key);
GAME_FN(void, __thiscall, NodeReference_Construct, 0x005F53E0, uint8_t* reference);
GAME_FN(void, __thiscall, AnimationTrack_Destruct, 0x00608240, uint8_t* track);

// a collision hit (0x18 bytes, 16 of them; CollisionTracks.cpp)
struct CollisionHit {
    re::Ptr32<uint8_t> entity;  // +0 a registered reference
    float age;        // +4
    float pos[3];     // +8
    int32_t sphere;   // +0x14
};
static_assert(sizeof(CollisionHit) == 0x18);
static_assert(offsetof(CollisionHit, pos) == 8);

// the hits' constructor loop: no entity, the position zero (the age and sphere left)
void ClearHits(uint8_t* self, uint32_t at) {
    CollisionHit* hits = reinterpret_cast<CollisionHit*>(self + at);
    for (int i = 0; i < 16; i++) {
        hits[i].pos[0] = 0.0f;
        hits[i].entity = nullptr;
        hits[i].pos[1] = 0.0f;
        hits[i].pos[2] = 0.0f;
    }
}

// 0x63DF60 (thiscall): BaseCollisionTrack's constructor
uint8_t* __fastcall BaseCollisionTrack_Construct(uint8_t* self, re::Edx) {
    Track_Construct(self);
    At<uint32_t>(self, 0) = 0x0092DEA8;
    NameKey_Default(self + 0x20);
    NameKey_Default(self + 0x28);
    NameKey_Default(self + 0x2C);
    NodeReference_Construct(self + 0x60);
    NodeReference_Construct(self + 0x70);
    ClearHits(self, 0x80);
    At<float>(self, 0x200) = 0.0f;
    At<float>(self, 0x204) = 0.0f;
    At<float>(self, 0x208) = 0.0f;
    return self;
}

// 0x642DB0 (thiscall): WeaponAnimationTrack's constructor (an AnimationTrack with hits
// at +0x58)
uint8_t* __fastcall WeaponAnimationTrack_Construct(uint8_t* self, re::Edx) {
    Track_Construct(self);
    At<uint32_t>(self, 0) = 0x00930F0C;  // AnimationTrack
    NameKey_Default(self + 0x18);
    NameKey_Default(self + 0x1C);
    At<uint32_t>(self, 0) = 0x009382D4;
    ClearHits(self, 0x58);
    for (uint32_t at = 0x1D8; at <= 0x1EC; at += 4) {
        At<float>(self, at) = 0.0f;
    }
    return self;
}

// 0x64A680 (thiscall): WeaponAnimationTrack's destructor: the hits' references let go,
// last first, then AnimationTrack's
void __fastcall WeaponAnimationTrack_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x009382D4;
    CollisionHit* hits = reinterpret_cast<CollisionHit*>(self + 0x58);
    for (int i = 15; i >= 0; i--) {
        if (hits[i].entity) {
            actc::Entity_CleanUpOldReference(hits[i].entity, &hits[i].entity);
        }
    }
    AnimationTrack_Destruct(self);
}

// 0x632970 (thiscall): RandomDiceRollTrack::Start: a random 0..1 into the owner's slot
// (+0x28, by +0x18); still running: no
uint8_t __fastcall RandomDiceRollTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    float r = float(Rand()) / 32767.0f;
    // (the game's lo + (hi - lo) * r, with 0 and 1)
    At<float>(owner, 0x28 + At<int32_t>(self, 0x18) * 4) = 0.0f + (1.0f - 0.0f) * r;
    return 0;
}

// 0x63F8A0 (thiscall): the opportunity tracks' end set: the first time (+0x28 still -1)
// to t, and remembered; after that to -1 (never)
void __fastcall OpportunityTrack_SetEnd(uint8_t* self, re::Edx, float t) {
    if (At<float>(self, 0x28) == -1.0f) {
        At<float>(self, 0x10) = t;
        At<float>(self, 0x28) = t;
    } else {
        At<float>(self, 0x10) = -1.0f;
    }
}

// 0x633850 (thiscall): WanderTrack's +0x2C
float __fastcall WanderTrack_Get2C(uint8_t* self, re::Edx) {
    return At<float>(self, 0x2C);
}
RE_UNHOOKED_FN("WanderTrack::Get2C", 0x00633850, &WanderTrack_Get2C);   // (4 bytes)

// 0x63FF40 (thiscall): RideAroundTrack's +0x30 (the same code serves SFXItem's vf29)
float __fastcall RideAroundTrack_Get30(uint8_t* self, re::Edx) {
    return At<float>(self, 0x30);
}
RE_UNHOOKED_FN("RideAroundTrack::Get30", 0x0063FF40, &RideAroundTrack_Get30);   // (4 bytes)

// 0x64AEF0 (thiscall): MenuTrack's constructor (the bounds at +0x20 as empty: min 1e6,
// max -1e6; the empty call on +0x20 is the game's)
constexpr auto MenuTrack_Construct =
    &actfac::Construct<actfac::TrackBase, actfac::Vt(0x0093838C), actfac::Key(0x18), actfac::Key(0x1C),
                       actfac::F32(0x20, 1000000.0f), actfac::F32(0x2C, 1000000.0f), actfac::F32(0x28, -1000000.0f),
                       actfac::F32(0x24, -1000000.0f), actfac::Zero(0x44), actfac::Zero(0x48), actfac::Zero(0x4C),
                       actfac::KeyDtor(0x20)>;

// 0x642520 (cdecl): SetMyTargetToMyLeadersTargetTrack allocated (the call on the result
// made on a failed allocation too)
constexpr auto SetMyTargetToMyLeadersTargetTrack_New =
    &actfac::New<0x1C, actfac::TrackBase, actfac::Vt(0x009358D8), actfac::Then, actfac::Call(0x0061C320)>;

// 0x640025: not a function: Ghidra's "FEDisplaySettings::vf7" is the UTF-16 text "%d"
// after FEDisplaySettings' vtable (0x93F5E8 has 7 entries), and lands inside
// SequenceTrack::IsA (0x640000)
RE_UNHOOKED("SequenceTrack::IsA (not a function)", 0x00640025);

} // namespace

#define ACTMISC32_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

// IsA (thiscall, the id): its own id, else its base's
ACTMISC32_REVERSED("CMissionCharTriggerTrack::IsA", 0x00631FA0, &actc::IsA<0x01B65998, 0x01B65994, 0x004437B0>);
ACTMISC32_REVERSED("CMissionVehicleTriggerTrack::IsA", 0x00632020, &actc::IsA<0x01B659A0, 0x01B6599C, 0x004437B0>);
ACTMISC32_REVERSED("CMissionDoorLockTrack::IsA", 0x006320C0, &actc::IsA<0x01B659A8, 0x01B659A4, 0x004437B0>);
ACTMISC32_REVERSED("CMissionDoorTriggerTrack::IsA", 0x00632160, &actc::IsA<0x01B659B0, 0x01B659AC, 0x004437B0>);
ACTMISC32_REVERSED("CMissionButtonTriggerTrack::IsA", 0x00632220, &actc::IsA<0x01B659B8, 0x01B659B4, 0x004437B0>);
ACTMISC32_REVERSED("PropTaggedTrack::IsA", 0x00633A20, &actc::IsA<0x00C3C9D8, 0x00C3C9D4, 0x004437B0>);
ACTMISC32_REVERSED("CTimeCond::IsA", 0x0063A360, &actc::IsA<0x01B659C0, 0x01B659BC, 0x005FC700>);
ACTMISC32_REVERSED("ExecuteTrack::IsA", 0x0063E350, &actc::IsA<0x01B659C8, 0x01B659C4, 0x005F7C60>);
ACTMISC32_REVERSED("OpportunityTrack::IsA", 0x0063F850, &actc::IsA<0x01B659D0, 0x01B659CC, 0x005F7C60>);  // (all four opportunity tracks)
ACTMISC32_REVERSED("SequenceTrack::IsA", 0x00640000, &actc::IsA<0x01B659D8, 0x01B659D4, 0x005F7C60>);

ACTMISC32_REVERSED("RandomDiceRollTrack::Start", 0x00632970, &RandomDiceRollTrack_Start);
ACTMISC32_REVERSED("OpportunityTrack::SetEnd", 0x0063F8A0, &OpportunityTrack_SetEnd);  // (all four opportunity tracks)
ACTMISC32_REVERSED("BaseCollisionTrack::Construct", 0x0063DF60, &BaseCollisionTrack_Construct);
ACTMISC32_REVERSED("WeaponAnimationTrack::Construct", 0x00642DB0, &WeaponAnimationTrack_Construct);
ACTMISC32_REVERSED("WeaponAnimationTrack::Destruct", 0x0064A680, &WeaponAnimationTrack_Destruct);
ACTMISC32_REVERSED("MenuTrack::Construct", 0x0064AEF0, MenuTrack_Construct);
ACTMISC32_REVERSED("SetMyTargetToMyLeadersTargetTrack::New", 0x00642520, SetMyTargetToMyLeadersTargetTrack_New);
