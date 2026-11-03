// The action tree's animation driver tracks with real logic: the struggle driver (button
// mashing, stick rotation or the tagging meter driving a parameter), the AI's aim point
// (leading the target), the reverse parametric animation and the rotation driver.
// 0x60B530, 0x60BBD0, 0x60C160, 0x60C400.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"
#include "reversed.h"

using namespace actc;

namespace {

// a Gamebryo transform (row major rotation, translation, scale)
struct NodeTransform {
    float rot[9];
    float translate[3];
    float scale;
};
static_assert(sizeof(NodeTransform) == 0x34);

constexpr double kPi = 3.1415927410125732;         // (float)pi as the game's double
constexpr double kQuarterPi = 0.7853981852531433;  // (float)(pi / 4)

GAME_VAR(uint8_t*, taggingMeter_C67538, 0x00C67538);   // +0x2860 its last total length
GAME_VAR(float, twoPi_B0B190, 0x00B0B190);

GAME_FN(uint8_t*, __thiscall, ActionInputs_GetRequest, 0x005F3720, uint8_t* inputs, int button);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int);
GAME_FN(float, __thiscall, Meter_TotalLength, 0x00557E40, uint8_t* meter);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, uint32_t duration, int strength, int flag);
GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, uint8_t* owner);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, uint8_t* entity, uint32_t node, NodeTransform* out, int flag);
GAME_FN(uint8_t*, __cdecl, Clump_GetHierarchy, 0x005EE1A0, void* clump);
GAME_FN(uint8_t*, __cdecl, Hierarchy_GetMatrices, 0x006C7710, uint8_t* hierarchy);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, uint8_t* blend, uint32_t name);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(void, __thiscall, AnimManager_SetSpeed, 0x006C0E60, void* manager, uint8_t* blend, uint8_t* anim, int,
        float speed);
GAME_FN(void, __thiscall, AnimManager_GetTime, 0x006C0F30, void* manager, uint8_t* blend, uint8_t* anim, int,
        float* out);
GAME_FN(void, __thiscall, AnimManager_Update, 0x006C0DC0, void* manager, uint8_t* blend, uint8_t* anim, int);
GAME_FN(void, __thiscall, AnimManager_SetTime, 0x006C0E90, void* manager, uint8_t* blend, uint8_t* anim, int,
        float time, int, int);
GAME_FN(float, __cdecl, RemapRange, 0x0072F500, float value, float from, float to, float outFrom, float outTo);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(float, __cdecl, AngleBetweenVectors, 0x00432BE0, const V3* a, const V3* b, int flag);
GAME_FN(float, __thiscall, AnimData_RotationTime, 0x006B90A0, uint8_t* data, float degrees, float time, float end,
        float* from, float* to, float* at);

// the clamp the drivers use: below lo gives lo, above hi hi, else (and NaN) the value
inline float ClampLoHi(float v, float lo, float hi) {
    if (lo > v) {
        return lo;
    }
    if (hi < v) {
        return hi;
    }
    return v;
}

// a skeleton matrix's translation (0x34 byte entries, +0x24)
inline V3 BoneTranslation(uint8_t* entity, int32_t bone) {
    uint8_t* matrices = Hierarchy_GetMatrices(Clump_GetHierarchy(At<void*>(entity, 0x18)));
    const float* t = reinterpret_cast<const float*>(matrices + bone * 0x34 + 0x24);
    return {t[0], t[1], t[2]};
}

// 0x60B530 (thiscall): AnimationDriverStruggleTrack::Update: the driven parameter (the
// owner's +0x38, and the ped's action context's) moves by a velocity (+0x4C, clamped to
// +0x34..+0x38) that grows by the action: +0x28 below 0x26 a button (+0x2C held, +0x30
// not), 0x26..0x29 turning a stick (either way; +0x50.. the rotation state), 0x2A up the
// tagging meter. The player feels it as rumble
uint8_t __fastcall Struggle_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* owner = At<uint8_t*>(self, 0x40);
    uint8_t* ped = At<uint8_t*>(owner, 8);
    uint8_t* entity = At<uint8_t*>(owner, 0xC);
    int32_t kind = At<int32_t>(self, 0x28);
    if (!ped && entity && At<uint8_t*>(entity, 0x234)) {
        ped = At<uint8_t*>(entity, 0x234);
    }
    bool pressed = false;
    float delta;
    if (kind < 0x26) {
        if (ped && (*ActionInputs_GetRequest(At<uint8_t*>(ped, 0x2E0), kind) & 1)) {
            delta = At<float>(self, 0x2C);
            pressed = true;
        } else {
            delta = At<float>(self, 0x30);
        }
    } else if (kind < 0x2A) {
        float a;
        float b;
        if (kind == 0x26 || kind == 0x27) {
            a = Pad_GetValue(0x10, 0);
            b = Pad_GetValue(0x11, 0);
        } else {
            a = Pad_GetValue(0x12, 0);
            b = Pad_GetValue(0x13, 0);
        }
        At<float>(self, 0x58) = At<float>(self, 0x54);
        float m = 0.0f;
        if (ped) {
            m = b * b + a * a;
        }
        if (ped && double(m) > 0.809999942779541) {
            float angle = trk::CrtAtan2(a, b);
            At<float>(self, 0x54) = angle;
            if (!self[0x50]) {
                At<float>(self, 0x58) = angle;
                At<float>(self, 0x60) = 0.0f;
                At<float>(self, 0x64) = 0.0f;
                At<float>(self, 0x68) = 0.0f;
            }
            float old = At<float>(self, 0x5C);
            float g = twoPi_B0B190;
            float d = angle - At<float>(self, 0x58);
            if (kPi < double(std::fabs(d))) {
                d = d < 0.0f ? d + g : d - g;
            }
            float rate = d / dt;
            At<float>(self, 0x5C) = rate;
            if ((old < 0.0f && rate > 0.0f) || (old > 0.0f && rate < 0.0f) || rate == 0.0f) {
                self[0x50] = 1;
                At<float>(self, 0x60) = 0.0f;
                At<float>(self, 0x64) = 0.0f;
                At<float>(self, 0x68) = 0.0f;
            } else {
                self[0x50] = 1;
                At<float>(self, 0x60) = dt + At<float>(self, 0x60);
                float total = std::fabs(d) + At<float>(self, 0x68);
                At<float>(self, 0x68) = total;
                At<float>(self, 0x64) = total / g;
            }
        } else {
            self[0x50] = 0;
            At<float>(self, 0x54) = 0.0f;
            At<float>(self, 0x5C) = 0.0f;
            At<float>(self, 0x60) = 0.0f;
        }
        float rate = At<float>(self, 0x5C);
        if (self[0x50] && (((kind == 0x26 || kind == 0x28) && double(rate) <= -kQuarterPi) ||
                           ((kind == 0x27 || kind == 0x29) && double(rate) >= kQuarterPi))) {
            float t = std::fabs(rate);
            if (double(t) > kPi) {
                t = 3.1415927f;
            }
            float u = float(double(float(double(t) - kQuarterPi)) / kPi);
            delta = u * At<float>(self, 0x2C);
        } else {
            delta = At<float>(self, 0x30);
        }
    } else {
        uint8_t* meter = taggingMeter_C67538;
        float grown = Meter_TotalLength(meter) - At<float>(meter, 0x2860);
        At<float>(meter, 0x2860) = Meter_TotalLength(meter);
        delta = At<float>(self, 0x2C) * grown;
    }
    float v = At<float>(self, 0x4C) + delta;
    At<float>(self, 0x4C) = v;
    float velocity = ClampLoHi(v, At<float>(self, 0x34), At<float>(self, 0x38));
    At<float>(self, 0x4C) = velocity;
    float current = *At<float*>(At<uint8_t*>(self, 0x40), 0x38);
    float step = pressed ? 0.4f : dt;
    float value = ClampLoHi(velocity * step + current, At<float>(self, 0x48), At<float>(self, 0x44));
    if (kind == 0x2A) {
        float length = Meter_TotalLength(taggingMeter_C67538);
        value = (At<float>(self, 0x44) - At<float>(self, 0x48)) * length + At<float>(self, 0x48);
    }
    if (At<float>(self, 0x48) == value && At<float>(self, 0x3C) > At<float>(self, 0x4C)) {
        At<float>(self, 0x4C) = At<float>(self, 0x3C);
    }
    *At<float*>(At<uint8_t*>(self, 0x40), 0x38) = value;
    if (ped) {
        auto context = reinterpret_cast<uint8_t*(__thiscall*)(uint8_t*)>(static_cast<void*>(At<re::Ptr32<void>*>(ped, 0)[16]));
        *At<float*>(context(ped), 0x38) = value;
    }
    if (ped == playerPed && 0.0f < At<float>(self, 0x4C)) {
        int32_t level = int32_t(double(At<float>(self, 0x4C)) * 50.0);
        // (an 8 bit signed multiply: the strength wraps above 127 levels)
        uint16_t strength = uint16_t(int16_t(int8_t(level) * 0x19));
        Pad_Rumble(0, 0x19, strength, 1);
    }
    return 1;
}

// 0x60BBD0 (thiscall): AIAimParametricTrack's aim vector: from the shooter's aim node
// (+0x38 a node of the owner's (+0x44) use point entity, +0x3C a bone, else its position)
// to the target's (+0x40 a bone) led by its velocity over the bullet's travel time (+0x48
// the weapon: +0x64 speed; +0x2C the lead), dropped by gravity (+0x80)
V3* __fastcall AIAim_AimVector(uint8_t* self, re::Edx, V3* out, uint8_t* shooter, uint8_t* target) {
    V3 from = {0.0f, 0.0f, 0.0f};
    if (At<uint32_t>(self, 0x38) != 0) {
        uint8_t* entity = UsePoint_GetEntity(At<uint8_t*>(self, 0x44));
        if (entity) {
            NodeTransform transform;
            transform.scale = 1.0f;
            GetNodeTransform(entity, At<uint32_t>(self, 0x38), &transform, 0);
            from = {transform.translate[0], transform.translate[1], transform.translate[2]};
        }
    } else if (At<int32_t>(self, 0x3C) != 0) {
        from = BoneTranslation(shooter, At<int32_t>(self, 0x3C));
    } else {
        float* p = Pos(shooter);
        from = {p[0], p[1], p[2]};
    }
    V3 to = {0.0f, 0.0f, 0.0f};
    if (At<int32_t>(self, 0x40) != 0) {
        to = BoneTranslation(target, At<int32_t>(self, 0x40));
    } else {
        float* p = Pos(target);
        to = {p[0], p[1], p[2]};
    }
    V3 tmp;
    V3* d = VectorSub(&tmp, &to.x, &from.x);
    float dist = SqrtAbs(d->x * d->x + d->y * d->y + d->z * d->z);
    float t = dist / At<float>(At<uint8_t*>(self, 0x48), 0x64);
    V3 lead = {0.0f, 0.0f, 0.0f};
    uint8_t type = target[0x108] & 7;
    if (type == 3) {
        uint8_t* vehicle = At<uint8_t*>(target, 0x1554);
        if (vehicle) {
            lead = {At<float>(vehicle, 0x124) * t, At<float>(vehicle, 0x128) * t, t * At<float>(vehicle, 0x12C)};
        } else {
            float inv = 1.0f / timeStep;
            V3 move = {At<float>(target, 0x1714), At<float>(target, 0x1718), At<float>(target, 0x171C)};
            V3 speed = {move.x * inv, move.y * inv, inv * move.z};
            lead = {speed.x * t, speed.y * t, t * speed.z};
        }
    } else if (type == 2) {
        lead = {At<float>(target, 0x124) * t, At<float>(target, 0x128) * t, t * At<float>(target, 0x12C)};
    }
    float k = At<float>(self, 0x2C);
    V3 scaled = {k * lead.x, lead.y * k, k * lead.z};
    to.x = scaled.x + to.x;
    to.y = scaled.y + to.y;
    to.z = scaled.z + to.z;
    d = VectorSub(&tmp, &to.x, &from.x);
    float dist2 = SqrtAbs(d->x * d->x + d->y * d->y + d->z * d->z);
    uint8_t* weapon = At<uint8_t*>(self, 0x48);
    float t2 = dist2 / float(double(At<float>(weapon, 0x64)) * 50.0);
    double gravity = double(At<float>(weapon, 0x80)) * 8.0 * 0.5;
    float t2sq = t2 * t2;
    float drop = float(gravity * double(t2sq));
    to.z = drop + to.z;
    VectorSub(out, &to.x, &from.x);
    return out;
}

// 0x60C160 (thiscall): AnimationReverseParametricTrack::Update: while playing (+0x44;
// +0x20 the mode: 2 never plays) the animation (+0x24) runs at +0x30 scaled by the ped's
// stat 0x14 until +0x2C (negative: its end), then stops (mode 1 resets it); the parameter
// is the animation's time within +0x28..+0x2C (+0x60..+0x64) remapped to +0x58..+0x5C
uint8_t __fastcall ReverseParametric_Update(uint8_t* self, re::Edx, float dt) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(static_cast<void*>(At<re::Ptr32<void>*>(self, 0)[5]))(self);
    uint8_t* blend = At<uint8_t*>(self, 0x4C);
    if (!blend || !At<void*>(blend, 0x94)) {
        return 0;
    }
    int32_t mode = At<int32_t>(self, 0x20);
    if (self[0x44] && mode != 2) {
        uint8_t* anim = Blend_GetAnim(blend, At<uint32_t>(self, 0x24));
        if (!anim) {
            return 0;
        }
        float rate = At<float>(self, 0x30);
        float scale = 1.0f;
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
        if (ped) {
            scale = float(double(Ped_GetStat(OwnerPed(At<uint8_t*>(self, 0x40)), 0x14)) / 100.0);
        }
        float speed = scale * rate;
        AnimManager_SetSpeed(animManager, At<uint8_t*>(self, 0x4C), anim, 0, speed);
        float time;
        AnimManager_GetTime(animManager, At<uint8_t*>(self, 0x4C), anim, 0, &time);
        time = speed * dt + time;
        bool atLength = At<float>(At<uint8_t*>(anim, 0x18), 0xC) <= time;
        float end = At<float>(self, 0x2C);
        bool finished = end < 0.0f ? atLength : end <= time;
        if (finished) {
            self[0x44] = 0;
            if (mode == 1) {
                AnimManager_Update(animManager, At<uint8_t*>(self, 0x4C), anim, -1);
            }
        }
    }
    uint8_t* anim = Blend_GetAnim(At<uint8_t*>(self, 0x4C), At<uint32_t>(self, 0x24));
    if (!anim) {
        return 0;
    }
    float lo = At<float>(self, 0x28);
    float hi = At<float>(self, 0x2C);
    if (hi < 0.0f) {
        hi = At<float>(At<uint8_t*>(anim, 0x18), 0xC);
    }
    float from = At<float>(self, 0x60);
    float to = At<float>(self, 0x64);
    float outFrom = At<float>(self, 0x58);
    float outTo = At<float>(self, 0x5C);
    float mn = outTo;
    float mx = outFrom;
    if (outTo > outFrom) {
        mn = outFrom;
        mx = outTo;
    }
    if (lo > from) {
        from = lo;
    }
    if (to > hi) {
        to = hi;
    }
    if (0.0f > to) {
        to = At<float>(At<uint8_t*>(anim, 0x18), 0xC);
    }
    float time;
    AnimManager_GetTime(animManager, At<uint8_t*>(self, 0x4C), anim, 0, &time);
    float value = RemapRange(time, from, to, outFrom, outTo);
    *At<float*>(At<uint8_t*>(self, 0x40), 0x38) = ClampLoHi(value, mn, mx);
    return (self[0x44] || mode != 0) ? 1 : 0;
}

// 0x60C400 (thiscall): AnimationDriverRotationTrack::Update: the animation (+0x1C) placed
// once (+0x38) at the parameter remapped from +0x20..+0x24 to +0x28..+0x2C; then the time
// the ped's turn from its facing to its move direction reaches (steps of +0x30 scaled by
// the ped's stat 0x14; +0x18 jumps straight there), remapped back into the parameter
uint8_t __fastcall DriverRotation_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x34));
    uint8_t* blend = At<uint8_t*>(ped, 0x114);
    if (!blend || !At<void*>(blend, 0x94)) {
        return 0;
    }
    uint8_t* anim = Blend_GetAnim(blend, At<uint32_t>(self, 0x1C));
    if (!anim) {
        return 0;
    }
    uint8_t* data = At<uint8_t*>(anim, 0x18);
    float length = At<float>(data, 0xC);
    float a = At<float>(self, 0x20);
    float b = At<float>(self, 0x24);
    float c = At<float>(self, 0x28);
    float d = At<float>(self, 0x2C);
    if (!self[0x38]) {
        self[0x38] = 1;
        float time = RemapRange(*At<float*>(At<uint8_t*>(self, 0x34), 0x38), a, b, c, d);
        AnimManager_SetTime(animManager, blend, anim, 0, time, 0, 0);
    }
    float scale = 1.0f;
    if (OwnerPed(At<uint8_t*>(self, 0x34))) {
        int32_t stat = Ped_GetStat(OwnerPed(At<uint8_t*>(self, 0x34)), 0x14);
        scale = float(double(At<float>(self, 0x30) * float(stat)) / 100.0);
    }
    float current = At<uint8_t*>(anim, 0x18) ? At<float>(anim, 0x1C) : -1.0f;
    float step = scale * dt;
    double stepD = double(step);
    float target = step + current;
    float result = target;
    V3 fwd;
    Placeable_GetForward(ped, &fwd);
    const float* m = At<float*>(ped, 0x2E0);
    V3 move = {m[0], m[1], m[2]};
    Vector_Normalise(&fwd);
    Vector_Normalise(&move);
    float angle = AngleBetweenVectors(&fwd, &move, 0);
    if (kPi < double(angle)) {
        angle = float(double(angle) - 6.2831854820251465);
    }
    float degrees = float(double(float(double(angle) * 180.0)) / kPi);
    float from;
    float to;
    float at;
    float r = AnimData_RotationTime(data, degrees, current, d, &from, &to, &at);
    if (r < 0.0f) {
        if (0.0f == from && 360.0f == to) {
            from = 360.0f;
        }
        float span = at - from;
        if (span != 0.0f) {
            bool keep = span > 0.0f ? angle >= 0.0f : angle < 0.0f;
            if (!keep) {
                result = float(double(current) - stepD);
            }
        }
    } else if (target > r) {
        result = float(double(current) - stepD);
        if (result < r) {
            result = r;
        }
    }
    if (self[0x18]) {
        result = r;
    }
    if (0.0f > d) {
        d = length;
    }
    *At<float*>(At<uint8_t*>(self, 0x34), 0x38) = RemapRange(result, c, d, a, b);
    return 1;
}

} // namespace

#define ACTTRACKSDRIVERS_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

ACTTRACKSDRIVERS_REVERSED("AnimationDriverStruggleTrack::Update", 0x0060B530, &Struggle_Update);
ACTTRACKSDRIVERS_REVERSED("AIAimParametricTrack::AimVector", 0x0060BBD0, &AIAim_AimVector);
ACTTRACKSDRIVERS_REVERSED("AnimationReverseParametricTrack::Update", 0x0060C160, &ReverseParametric_Update);
ACTTRACKSDRIVERS_REVERSED("AnimationDriverRotationTrack::Update", 0x0060C400, &DriverRotation_Update);
