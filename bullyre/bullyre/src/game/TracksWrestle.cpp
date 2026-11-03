// The action tree's tracks of 0x61DB40..0x61EAC0: the wrestle track (keeping a ped
// facing its look target inside the wrestling ring, picking a better spot when its own
// scores badly), the stimulus tracks (create / clear) and the target tracks (attitude
// towards an attacker, warning, punishable threats, giving the target to the focus and
// punishment points).

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"

using namespace actc;

namespace {

GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget, 0x004D7780, uint8_t* targeting);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(float, __cdecl, Distance2D, 0x00470110, const float* a, const float* b);
GAME_FN(V3*, __thiscall, Placeable_GetPosition, 0x00415460, uint8_t* placeable);
GAME_FN(uint8_t*, __thiscall, Scanner_GetThreat, 0x0048C070, uint8_t* scanner);
GAME_FN(void, __thiscall, Scanner_Alert, 0x0048C480, uint8_t* scanner, uint8_t* ped);
GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(void, __thiscall, Targeting_SetTarget, 0x004D79E0, uint8_t* targeting, uint8_t* target);
GAME_FN(void, __thiscall, EventMgr_Send, 0x004A6670, void* self, int event, uint8_t* ped, uint8_t* other,
        const V3* pos, float, float);
GAME_FN(uint8_t, __thiscall, Stimuli_RemoveAll, 0x004A67D0, void* self, uint8_t* ped);
GAME_FN(void, __thiscall, AttitudeTable_Worsen, 0x0046F930, void* table, int32_t pedType, int32_t otherType);
GAME_FN(int32_t*, __thiscall, Ped_GetAttitude, 0x00475380, uint8_t* ped, int32_t* out, int32_t pedType);
GAME_FN(void, __thiscall, Ped_SetPedToTypeAttitude, 0x004731D0, uint8_t* ped, int32_t pedType, int32_t attitude);
GAME_FN(uint32_t*, __thiscall, NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, HashPath_Set, 0x005FA8C0, int32_t* path, int32_t count, uint32_t* hashes);
GAME_FN(void*, __cdecl, FindBoneInfo, 0x0072AEA0, int32_t* path, void* names);
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* array, size_t size, int count, void* destructor);
GAME_FN(void, __thiscall, Punishment_AddPoints, 0x0044E850, uint8_t* self, uint8_t* ped, int32_t* points,
        void* level, int arg4, int arg5);

GAME_VAR(void*, events, 0x00C2C1A0);
GAME_VAR(void*, attitudes, 0x00C2AF9C);
GAME_VAR(uint8_t*, punishment, 0x00C0F78C);
// the wrestling ring (two corners, in either order)
GAME_ARRAY(float, ringA, 0x00CFB38C);
GAME_ARRAY(float, ringB, 0x00CFB398);
GAME_VAR(float, wrestleTargetScale, 0x00B0C24C);   // 1.0
GAME_VAR(float, wrestleTargetWeight, 0x00B0C248);  // 1.0
GAME_VAR(float, wrestleFacingWeight, 0x00B0C244);  // 0.5
GAME_VAR(float, wrestleMinScore, 0x00B0C250);      // -0.7
GAME_VAR(float, wrestleRegoalDist, 0x00B0C254);    // 0.5
GAME_VAR(uint32_t, wrestleRegoalTime, 0x00B0C258); // 1000 ms
inline uint32_t* const punishmentName = reinterpret_cast<uint32_t*>(0x01B652F8);      // "Punishment"
inline void* const punishmentButesName = reinterpret_cast<void*>(0x01B652FC);         // "PunishmentButes"

inline float Magnitude(const V3& v) {
    return SqrtAbs(v.x * v.x + v.y * v.y + v.z * v.z);
}
inline V3& Output(uint8_t* ped) {   // the ped's locomotion output: +0 heading, +0xC facing, +0x1C speed
    return *At<V3*>(ped, 0x2E0);
}

// WrestleTrack fields: +0x18 flee inside, +0x1C approach beyond, +0x20 speed, +0x24 /
// +0x28 the target's reach and weight, +0x2C / +0x30 the ring's edge margin and weight,
// +0x34 / +0x38 its corners' reach and weight, +0x40 the owner, +0x44 the last spot
// picked, +0x50 when

// 0x61DB40 (thiscall, the spot by value): WrestleTrack::Score: how good a spot is (the
// look target's and the ring's edges and corners too close, the ring's centre too far,
// facing away from the target)
float __fastcall WrestleTrack_Score(uint8_t* self, re::Edx, float x, float y, float z) {
    V3 p{x, y, z};
    float score = 0.0f;
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
    uint8_t* target = Targeting_GetLookTarget(At<uint8_t*>(ped, 0x1C8));
    if (target) {
        V3 tpos = *reinterpret_cast<V3*>(Pos(target));
        float reach = At<float>(self, 0x24);
        V3 d;
        V3* to = VectorSub(&d, &tpos.x, &p.x);
        float in = reach - Magnitude(*to);
        if (in > 0.0f) {
            score = 0.0f - in / At<float>(self, 0x24) * At<float>(self, 0x28);
        }
    }
    float loX = ringB[0] > ringA[0] ? ringA[0] : ringB[0];
    float loY = ringB[1] > ringA[1] ? ringA[1] : ringB[1];
    float hiX = ringA[0] > ringB[0] ? ringA[0] : ringB[0];
    float hiY = ringA[1] > ringB[1] ? ringA[1] : ringB[1];
    float edges[4] = {
        (loX + At<float>(self, 0x2C)) - x,
        (loY + At<float>(self, 0x2C)) - y,
        (x + At<float>(self, 0x2C)) - hiX,
        (y + At<float>(self, 0x2C)) - hiY,
    };
    for (float in : edges) {
        if (in > 0.0f) {
            score = score - in / At<float>(self, 0x2C) * At<float>(self, 0x30);
        }
    }
    V3 corners[4] = {{loX, loY, z}, {hiX, loY, z}, {hiX, hiY, z}, {loX, hiY, z}};
    for (V3& c : corners) {
        float reach = At<float>(self, 0x34);
        V3 d;
        float in = reach - Magnitude(*VectorSub(&d, &c.x, &p.x));
        if (in > 0.0f) {
            score = score - in / At<float>(self, 0x34) * At<float>(self, 0x38);
        }
    }
    V3 pedPos = *reinterpret_cast<V3*>(Pos(ped));
    if (target) {
        // how close the target comes to the line from the ped through the spot
        V3 tpos = *reinterpret_cast<V3*>(Pos(target));
        V3 dir, toTarget;
        VectorSub(&dir, &p.x, &pedPos.x);
        VectorSub(&toTarget, &tpos.x, &pedPos.x);
        Vector_Normalise(&dir);
        float along = dir.x * toTarget.x + dir.y * toTarget.y + dir.z * toTarget.z;
        // (the game compares with the direction's length, not the spot's distance)
        if (along > 0.0f && Magnitude(dir) > along) {
            V3 step{along * dir.x, dir.y * along, along * dir.z};
            V3 nearest;
            VectorAdd(&nearest, &pedPos.x, &step.x);
            float reach = At<float>(self, 0x24) * wrestleTargetScale;
            V3 d;
            float in = reach - Magnitude(*VectorSub(&d, &tpos.x, &nearest.x));
            if (in > 0.0f) {
                score = score - in / At<float>(self, 0x24) * At<float>(self, 0x28) * wrestleTargetWeight;
            }
        }
    }
    V3 sum;
    V3* s = VectorAdd(&sum, ringA, ringB);
    V3 centre{s->x * 0.5f, s->y * 0.5f, z};
    V3 toCentre;
    VectorSub(&toCentre, &centre.x, &p.x);
    // (the game's 0.01 is a double: the product rounded once, to single precision)
    score = score - float(double(Magnitude(toCentre)) * 0.01);
    if (target) {
        // (the game measures from the centre offset and the spot, not the ped: as is)
        V3 tpos = *reinterpret_cast<V3*>(Pos(target));
        V3 a, b;
        VectorSub(&a, &toCentre.x, &tpos.x);
        VectorSub(&b, &p.x, &toCentre.x);
        Vector_Normalise(&a);
        Vector_Normalise(&b);
        float facing = a.y * b.y + b.x * a.x + a.z * b.z;
        score = facing * wrestleFacingWeight + score;
    }
    return score;
}

// 0x61E1B0 (thiscall): WrestleTrack::Update: the ped faces its (ped) look target and
// keeps its distance (approaching, holding or backing off), heading for a better spot
// when the current one scores badly (or when backing off)
uint8_t __fastcall WrestleTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
    uint8_t* target = Targeting_GetLookTarget(At<uint8_t*>(ped, 0x1C8));
    if (!target || !IsPedEntity(target)) {
        return 1;
    }
    V3 pedPos = *reinterpret_cast<V3*>(Pos(ped));
    V3 dir;
    VectorSub(&dir, Pos(target), &pedPos.x);
    float dist = Magnitude(dir);
    Vector_Normalise(&dir);
    V3& last = At<V3>(self, 0x44);
    uint8_t heading = (timeInMs - At<uint32_t>(self, 0x50)) < wrestleRegoalTime;
    if (heading) {
        V3 d;
        if (wrestleRegoalDist > Vector_Magnitude(VectorSub(&d, &pedPos.x, &last.x))) {
            heading = 0;   // arrived
        }
    }
    uint8_t backOff = 0;
    if (heading) {
        At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) = At<float>(self, 0x20);
        V3& out = Output(ped);
        V3 d;
        out = *VectorSub(&d, &last.x, &pedPos.x);
        Vector_Normalise(&Output(ped));
        At<V3>(At<uint8_t*>(ped, 0x2E0), 0xC) = dir;
    } else {
        At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) = At<float>(self, 0x20);
        At<V3>(At<uint8_t*>(ped, 0x2E0), 0xC) = dir;
        if (At<float>(self, 0x1C) < dist) {
            Output(ped) = dir;
        } else if (At<float>(self, 0x18) > dist) {
            backOff = 1;
            Output(ped) = V3{-dir.x, -dir.y, -dir.z};
        } else {
            Output(ped) = V3{0.0f, 0.0f, 0.0f};
            At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) = 0.0f;
        }
    }
    float here = WrestleTrack_Score(self, nullptr, pedPos.x, pedPos.y, pedPos.z);
    if (!(wrestleMinScore > here) && !backOff) {
        return 1;
    }
    // the best of 8 directions at 1.5 and 3 away
    constexpr float kD = 0.70710677f;
    static const float dirs[8][2] = {
        {0.0f, -1.0f}, {kD, -kD}, {-kD, -kD}, {1.0f, 0.0f},
        {-1.0f, 0.0f}, {kD, kD}, {-kD, kD}, {0.0f, 1.0f},
    };
    static const float radii[2] = {1.5f, 3.0f};
    V3 bestPos{0.0f, 0.0f, 0.0f};
    float best = -1e11f;
    for (int i = 0; i < 2; ++i) {
        float r = radii[i];
        float rz = r * 0.0f;
        for (int j = 0; j < 8; ++j) {
            V3 off{dirs[j][0] * r, r * dirs[j][1], rz};
            V3 spot;
            VectorAdd(&spot, &pedPos.x, &off.x);
            float s = WrestleTrack_Score(self, nullptr, spot.x, spot.y, spot.z);
            if (best < s) {
                best = s;
                bestPos = spot;
            }
        }
    }
    V3& out = Output(ped);
    V3 d;
    out = *VectorSub(&d, &bestPos.x, &pedPos.x);
    Vector_Normalise(&Output(ped));
    last = bestPos;
    At<uint32_t>(self, 0x50) = timeInMs;
    return 1;
}

// 0x61E630 (thiscall): WrestleTrack::End: the ped stopped and no longer wrestling
uint8_t __fastcall WrestleTrack_End(uint8_t* self, re::Edx) {
    Output(OwnerPed(At<uint8_t*>(self, 0x40))) = V3{0.0f, 0.0f, 0.0f};
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
    At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) = 0.0f;
    At<int32_t>(ped, 0x1340) = 0;
    return 1;
}

// 0x61E680 (thiscall): CreateStimulusTrack::Start: a stimulus (+0x18) sent from the ped
// (the player when none) towards its other (+0x1C set: its grapple partner, target, or
// as the player its look target); not when both are non-player peds and the ped's
// in-group pair (+0xBB0, +0xBB4) isn't the other's
uint8_t __fastcall CreateStimulusTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* other = nullptr;
    if (At<uint8_t>(self, 0x1C) != 0) {
        other = At<uint8_t*>(ped, 0x155C);
        if (!other) {
            other = Ped_GetTarget(ped);
            if (!other && At<int32_t>(ped, 0x1310) == 0xD) {
                other = Targeting_GetLookTarget(At<uint8_t*>(ped, 0x1C8));
            }
        }
        if (other && IsPedEntity(other) && At<int32_t>(ped, 0x1310) != 0xD &&
            At<int32_t>(other, 0x1310) != 0xD && At<int32_t>(ped, 0xBB0) != 0 &&
            At<int32_t>(ped, 0xBB4) != 0 && At<uint8_t*>(other, 0xBB4) != ped) {
            return 0;
        }
    }
    if (!ped) {
        ped = playerPed;
    }
    V3 pos = *reinterpret_cast<V3*>(Pos(ped));
    EventMgr_Send(events, At<int32_t>(self, 0x18), ped, other, &pos, -1.0f, -1.0f);
    return 0;
}

// 0x61E760 (thiscall): ClearStimulusTrack::Start: the ped's stimuli removed
uint8_t __fastcall ClearStimulusTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    Stimuli_RemoveAll(events, OwnerPed(owner));
    return 1;
}

// 0x61E780 (thiscall): ClearCurrentStimulusTrack::Start: the ped's current stimulus
// (+0x1D30) marked done (+0x2A)
uint8_t __fastcall ClearCurrentStimulusTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped && At<uint8_t*>(ped, 0x1D30)) {
        At<uint8_t>(At<uint8_t*>(ped, 0x1D30), 0x2A) = 1;
    }
    return 1;
}

// 0x61E7B0 (thiscall): DeteriorateAttitudeToAttackerTrack::Update: hit by the player:
// the ped type's attitude to it worsened, and the ped's own lowered by one
uint8_t __fastcall DeteriorateAttitudeToAttackerTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x18);
    uint8_t* ped = OwnerPed(owner);
    uint8_t* attacker = At<uint8_t*>(At<uint8_t*>(owner, 0x10), 4);
    if (attacker && At<int32_t>(attacker, 0x1310) == 0xD) {
        AttitudeTable_Worsen(attitudes, At<int32_t>(ped, 0x1310), At<int32_t>(attacker, 0x1310));
        int32_t tmp;
        if (*Ped_GetAttitude(ped, &tmp, At<int32_t>(attacker, 0x1310)) > 0) {
            int32_t now = *Ped_GetAttitude(ped, &tmp, At<int32_t>(attacker, 0x1310));
            Ped_SetPedToTypeAttitude(ped, At<int32_t>(attacker, 0x1310), now - 1);
        }
    }
    return 0;
}

// 0x61E830 (thiscall): WarnTargetTrack::Update: the ped's (ped) target warned (+0x137C),
// and with +0x18 set, every ped (not of types 0, 7 or 8) within +0x1C of it (flat)
uint8_t __fastcall WarnTargetTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* target = Ped_GetTarget(OwnerPed(At<uint8_t*>(self, 0x20)));
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    At<int32_t>(target, 0x137C) = 1;
    if (At<uint8_t>(self, 0x18) == 0) {
        return 0;
    }
    CPool<uint8_t>* pool = pedPool;
    for (int i = pool->size; i != 0;) {
        --i;
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* p = re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * i;
        if (!p) {
            continue;
        }
        int32_t type = At<int32_t>(p, 0x1310);
        if (type == 0 || type == 8 || type == 7) {
            continue;
        }
        float* pos = Pos(p);
        float dist = Distance2D(pos, &Placeable_GetPosition(target)->x);
        if (!(At<float>(self, 0x1C) < dist)) {
            At<int32_t>(p, 0x137C) = 1;
        }
    }
    return 0;
}

// 0x61E920 / 0x61E950 (thiscall): WarnTarget2Track / PunishableTargetThreatenedTrack
// ::Update: a field of the ped's (ped) target set
template <int Field, int32_t Value>
uint8_t __fastcall SetTargetField_Update(uint8_t* self, re::Edx, float) {
    uint8_t* target = Ped_GetTarget(OwnerPed(At<uint8_t*>(self, 0x18)));
    if (target && IsPedEntity(target)) {
        At<int32_t>(target, Field) = Value;
    }
    return 0;
}

// 0x61E980 (thiscall): GiveTargetToFocusTrack::Update: the ped's (ped) target handed to
// the ped its scanner is most threatened by: alerted to it, targeting it
uint8_t __fastcall GiveTargetToFocusTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* target = Ped_GetTarget(ped);
    if (target && !IsPedEntity(target)) {
        target = nullptr;
    }
    uint8_t* focus = Scanner_GetThreat(ped + 0x3BC);
    if (target && focus && target != focus) {
        Scanner_Alert(focus + 0x3BC, target);
        Ped_SetTargetEntity(focus, target);
        Targeting_SetTarget(At<uint8_t*>(focus, 0x1C8), target);
    }
    return 0;
}

// 0x61E9F0 (thiscall): GivePunishmentPointsToTargetTrack::Update: the ped's (ped) target
// punished: through the named PunishmentButes level (+0x1C) when it is found, else its
// vf45 given the points (+0x18)
uint8_t __fastcall GivePunishmentPointsToTargetTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* target = Ped_GetTarget(OwnerPed(At<uint8_t*>(self, 0x20)));
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    if (At<const char*>(self, 0x1C) != nullptr) {
        uint32_t keys[2];
        int32_t path[2];   // (the game doesn't destroy it)
        NameKey_Copy(&keys[0], punishmentName);
        NameKey_Construct(&keys[1], At<const char*>(self, 0x1C));
        HashPath_Set(path, 2, keys);
        void* level = FindBoneInfo(path, punishmentButesName);
        DestructArray(keys, 4, 2, reinterpret_cast<void*>(0x0049A650));
        if (level) {
            int32_t points = 0;
            Punishment_AddPoints(punishment, target, &points, level, 1, 0);
            return 0;
        }
    }
    vc::Call<void>(target, 0xB4, At<int32_t>(self, 0x18));
    return 0;
}

} // namespace

#define TRWRESTLE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TRWRESTLE_REVERSED("WrestleTrack::Score_61DB40", 0x0061DB40, &WrestleTrack_Score);
TRWRESTLE_REVERSED("WrestleTrack::Update", 0x0061E1B0, &WrestleTrack_Update);
TRWRESTLE_REVERSED("WrestleTrack::End", 0x0061E630, &WrestleTrack_End);
TRWRESTLE_REVERSED("CreateStimulusTrack::Start", 0x0061E680, &CreateStimulusTrack_Start);
TRWRESTLE_REVERSED("ClearStimulusTrack::Start", 0x0061E760, &ClearStimulusTrack_Start);
TRWRESTLE_REVERSED("ClearCurrentStimulusTrack::Start", 0x0061E780, &ClearCurrentStimulusTrack_Start);
TRWRESTLE_REVERSED("DeteriorateAttitudeToAttackerTrack::Update", 0x0061E7B0, &DeteriorateAttitudeToAttackerTrack_Update);
TRWRESTLE_REVERSED("WarnTargetTrack::Update", 0x0061E830, &WarnTargetTrack_Update);
TRWRESTLE_REVERSED("WarnTarget2Track::Update", 0x0061E920, (&SetTargetField_Update<0x13D8, 1>));
TRWRESTLE_REVERSED("PunishableTargetThreatenedTrack::Update", 0x0061E950, (&SetTargetField_Update<0x14E0, 0>));
TRWRESTLE_REVERSED("GiveTargetToFocusTrack::Update", 0x0061E980, &GiveTargetToFocusTrack_Update);
TRWRESTLE_REVERSED("GivePunishmentPointsToTargetTrack::Update", 0x0061E9F0, &GivePunishmentPointsToTargetTrack_Update);
