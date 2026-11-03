// The action tree's prop tracks of 0x656500..0x657330: the prop collision sweep, tagging
// (the gang's insult and colours picked, the tag's fraction blended, the stats counted),
// rotating a prop in place, and attaching a new prop animation to one.

#include <cstdint>
#include <cstdio>

#include "game/ActionFactories.h"
#include "game/PropTracks.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;
using proptr::NodeTransform;
using proptr::GetNodeTransform;
using proptr::UsePoint_GetEntity;
namespace af = actfac;

namespace {

// a hit (0x18 bytes, 16 of them at +0x80; CollisionTracks.cpp)
struct CollisionHit {
    re::Ptr32<uint8_t> entity;  // +0 a registered reference
    float age;        // +4
    V3 pos;           // +8
    int32_t sphere;   // +0x14
};
static_assert(sizeof(CollisionHit) == 0x18);

struct Rgba {
    uint8_t r, g, b, a;
};

GAME_FN(uint8_t, __thiscall, PropCollision_Test, 0x00655AF0, uint8_t* self, const V3* pos, float radius, float margin);
GAME_FN(void, __thiscall, Box_Set, 0x00564830, void* box, const V3* min, const V3* max);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(uint8_t*, __thiscall, Node_Get0C, 0x005F4F90, uint8_t* reference, int32_t key);
GAME_FN(uint8_t*, __cdecl, AsPed, 0x00468E10, void* entity);
GAME_FN(V3*, __cdecl, Normalised, 0x00472D40, V3* out, const V3* v);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const V3* a, const V3* b);
GAME_FN(uint16_t, __thiscall, SoundBank_GetId, 0x00667B60, void* bank, const char* name);
GAME_FN(int, __thiscall, SoundStreams_GetId, 0x00595B80, void* streams, const char* name);
GAME_FN(void, __thiscall, Stat_Add_4A74C0, 0x004A74C0, uint8_t* stat, float value);
GAME_FN(void, __thiscall, Stat_Add_4DAC30, 0x004DAC30, uint8_t* stat, float value);
GAME_FN(void, __thiscall, Track_AgeHits, 0x005F8AE0, uint8_t* track, float dt, float maxAge);
GAME_FN(void, __thiscall, PickTagName, 0x00656A60, uint8_t* self);
GAME_FN(int, __thiscall, GangFinder_72D0B0, 0x0072D0B0, void* self, int* out, int pedType, const V3* pos);
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int low, int high);
GAME_FN(float, __cdecl, Clamp, 0x0041A470, float value, float low, float high);
template <typename... A>
inline int __cdecl Sprintf(char* out, const char* format, A... rest) { return crt::Sprintf(out, format, rest...); }
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __thiscall, EventMgr_Send, 0x004A6670, void* self, int event, uint8_t* ped, uint8_t* other, const V3* pos,
        float, float);
GAME_FN(void, __thiscall, Tagging_StartTag, 0x00558750, uint8_t* tagging, int32_t shape);
GAME_FN(void, __thiscall, NameKey_FromString, 0x005FA890, uint32_t* key, const char* name);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(Rgba*, __thiscall, RGBA_Construct, 0x00510DE0, Rgba* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Stats_Increment, 0x00458480, int stat, int by);
GAME_FN(void, __thiscall, PropAnim_SetUnknown254, 0x004EA2A0, uint8_t* prop, float value);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, uint8_t* matrix, float angle);
GAME_FN(int, __cdecl, Prop_FindBone, 0x00655620, uint8_t* entity, int id);
GAME_FN(int, __cdecl, GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, NodeTransform* self);
GAME_FN(uint8_t*, __cdecl, CreateTriggerObject_44CE70, 0x0044CE70, int model, int area, NodeTransform* transform, float,
        void* trigger, int, int);
GAME_FN(void, __thiscall, PropAnim_4ED940, 0x004ED940, uint8_t* prop);
GAME_FN(void, __thiscall, PropAnim_AddChild, 0x004EE5B0, uint8_t* prop, int bone, uint8_t* child, int childBone);

GAME_VAR(uint8_t*, sweepColModel, 0x00CF3140);   // the swept sphere's collision model
GAME_VAR(void*, soundBanks, 0x00B6F1E8);
GAME_VAR(uint8_t*, tagging, 0x00C67538);         // the HUD's tagging meter (+0x2858 the gang's shape)
GAME_VAR(void*, events, 0x00C2C1A0);
GAME_VAR(int32_t, currentArea, 0x00BD1008);
GAME_VAR(float, tagColourBlend_C66B20, 0x00C66B20);
GAME_VAR(float, tagColourDay_20C47E0, 0x020C47E0);
GAME_VAR(int32_t, breakableTagModel, 0x00BF4470);
GAME_VAR(int32_t, weaponModelFirst, 0x00A136A0);
GAME_VAR(int32_t, weaponModelLast, 0x00A136A4);
inline void* const soundStreams = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const gangFinder = reinterpret_cast<void*>(0x020C8A00);
// the tag colours: 5 per gang shape and style (a 6th: the breakable model's), two sets
GAME_ARRAY(Rgba, tagColoursA0, 0x01B65A54);
GAME_ARRAY(Rgba, tagColoursA1, 0x01B65A68);
GAME_ARRAY(Rgba, tagColoursB0, 0x01B65A7C);
GAME_ARRAY(Rgba, tagColoursB1, 0x01B65A90);
GAME_ARRAY(Rgba, tagColoursC, 0x01B65AA4);   // two

inline CollisionHit* Hits(uint8_t* self) {
    return reinterpret_cast<CollisionHit*>(self + 0x80);
}

// 0x656500 (thiscall): PropCollisionTrack::Update: before its start the last position
// follows the bone; until its end (none: 0 or less) the bone swept from the last
// position, every new hit damaged, its sounds / effects set and the ped's stats hit
uint8_t __fastcall PropCollision_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* prop = At<uint8_t*>(At<uint8_t*>(self, 0x20C), 0xC);
    if (!prop || !At<void*>(prop, 0x114)) {
        return 0;
    }
    NodeTransform transform;
    transform.scale = 1.0f;
    if (!GetNodeTransform(prop, At<int32_t>(self, 0x210), &transform, 1)) {
        return 0;
    }
    V3& last = At<V3>(self, 0x200);
    float time = At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x20C), 0), 8);
    float start = At<float>(self, 0x18);
    if (start > time) {
        if (GetNodeTransform(prop, At<int32_t>(self, 0x210), &transform, 1)) {
            last = transform.pos;
        }
        return 1;
    }
    float end = At<float>(self, 0x1C);
    if (end < time && 0.0f < end) {
        return 1;
    }
    V3 from = *reinterpret_cast<V3*>(Pos(prop));
    if (GetNodeTransform(prop, At<int32_t>(self, 0x210), &transform, 1)) {
        from = transform.pos;
    }
    float radius = At<float>(self, 0x5C);
    At<float>(sweepColModel, 0xC) = radius;
    At<float>(At<uint8_t*>(At<uint8_t*>(sweepColModel, 0x38), 0), 0xC) = radius;
    V3 boxMax{radius, radius, radius};
    float negative = -radius;
    V3 boxMin{negative, negative, negative};
    Box_Set(sweepColModel + 0x10, &boxMin, &boxMax);
    V3 d;
    VectorSub(&d, &from.x, &last.x);
    float length = Vector_Magnitude(&d);
    if (PropCollision_Test(self, &from, length + (radius + radius), radius)) {
        for (int i = 0; i < 16; i++) {
            CollisionHit& hit = Hits(self)[i];
            uint8_t* entity = hit.entity;
            float age = hit.age;
            if (!entity) {
                continue;
            }
            if (!(age == 0.0f || At<float>(self, 0x30) > age)) {
                continue;
            }
            uint8_t* node = Node_Get0C(self + 0x70, -1);
            uint8_t* ped = AsPed(entity);
            // (the hit's controller asked without testing it's a ped)
            if (node && !vc::Call<uint8_t*>(node, acttree::kNodeFindPlayable, At<uint8_t*>(ped, 0x2E0) + 0x300)) {
                break;
            }
            hit.age = 0.0f;
            uint8_t type = entity[0x108] & 7;
            float damage = At<float>(self, 0x34);
            if (type == 6 || type == 4) {
                damage = At<float>(self, 0x50);
            }
            V3 dir{0.0f, 0.0f, 0.0f};
            V3 hitPos = hit.pos;
            if (self[0x214] != 0) {
                // away from the prop's facing side
                V3 to, toNorm, forward;
                VectorSub(&to, Pos(entity), &from.x);
                Normalised(&toNorm, &to);
                Placeable_GetForward(prop, &forward);
                if (0.0f < DotProduct(&toNorm, &forward)) {
                    dir = forward;
                } else {
                    dir = V3{-forward.x, -forward.y, -forward.z};
                }
            } else {
                V3 back;
                VectorSub(&back, &from.x, &last.x);
                dir = back;
            }
            float f3c = At<float>(self, 0x3C);
            float f38 = At<float>(self, 0x38);
            // (the byte flags go on the stack as dwords whose upper bytes are stale)
            int b58 = self[0x58];
            int b216 = self[0x216];
            using DamageFn = uint8_t(__thiscall*)(uint8_t*, float, float, uint8_t*, int, V3*, V3*, uint8_t*, int, int32_t, int,
                                                  float, float, int, int, int, int, float, int);
            DamageFn damageFn = re::PtrCast<DamageFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[0x3C / 4]);
            if (!damageFn(entity, damage, 0.0f, nullptr, -1, &hitPos, &dir, node, b216, At<int32_t>(self, 0x2C), 0, f38, f3c,
                          b58, 1, 0, 0, 0.0f, 0)) {
                continue;
            }
            uint8_t* info = vc::Call<uint8_t*>(entity, 0x40);
            if (info) {
                uint8_t* sounds = At<uint8_t*>(info, 0x10);
                if (At<const char*>(self, 0x28)) {
                    At<int32_t>(sounds, 0x48) = SoundBank_GetId(soundBanks, At<const char*>(self, 0x28));
                }
                if (At<const char*>(self, 0x20)) {
                    At<int32_t>(sounds, 0x40) = SoundStreams_GetId(soundStreams, At<const char*>(self, 0x20));
                    sounds[0x44] = self[0x24];
                }
                At<float>(sounds, 0x54) = At<float>(self, 0x44);
                At<float>(sounds, 0x58) = At<float>(self, 0x40);
            }
            if (ped) {
                float f4c = At<float>(self, 0x4C);
                Stat_Add_4A74C0(At<uint8_t*>(ped, 0x1D8), f4c);
                Stat_Add_4DAC30(At<uint8_t*>(ped, 0x1D4), f4c);
            }
        }
    }
    Track_AgeHits(self, dt, At<float>(self, 0x30));
    last = from;
    return 1;
}

// 0x656A10 (thiscall): PropCollisionTrack::End: the prop's flag (+0xE4) cleared
uint8_t __fastcall PropCollision_End(uint8_t* self, re::Edx) {
    At<uint32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x20C), 0xC), 0xE4) = 0;
    return 1;
}

// a gang's insult: its counter (-1: a random start) stepped on (0..8), the speech name
// made and the insult stimulus sent from the tagger
void Insult(char* name, int32_t& counter, const char* prefix, int stimulus, uint8_t* ped) {
    int32_t n = counter;
    if (n == -1) {
        // (the random range's low end is the counter's -1)
        n = int32_t(Clamp(float(RandomInt(n, 10)), 0.0f, 8.0f));
    }
    n = n + 1;
    if (n == 9) {
        n = 0;
    }
    counter = n;
    Sprintf(name, reinterpret_cast<const char*>(0x0093BDF4), prefix, n);   // "%s%d"
    EventMgr_Send(events, stimulus, ped, nullptr, nullptr, -1.0f, -1.0f);
}

// 0x656A60 (thiscall): PropTaggedTrack's tag name (+0x30): the given one (+0x18), else the
// tagger's gang insult (the player's: the meter set to the gang's shape)
void __fastcall PropTagged_PickName(uint8_t* self, re::Edx) {
    uint8_t* prop = At<uint8_t*>(At<uint8_t*>(self, 0x2C), 0xC);
    bool player = At<uint8_t*>(prop, 0x234) == playerPed;
    if (player) {
        At<int32_t>(tagging, 0x2858) = -1;
    }
    if (At<uint32_t>(self, 0x18) != 0) {
        At<uint32_t>(self, 0x30) = At<uint32_t>(self, 0x18);
        return;
    }
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x2C), 0xC), 0x234);
    if (!ped) {
        return;
    }
    V3 pos = *reinterpret_cast<V3*>(Pos(ped));
    int gang;
    GangFinder_72D0B0(gangFinder, &gang, At<int32_t>(ped, 0x1310), &pos);
    char name[0x20];   // (uninitialised when the gang has no insult)
    int shape = -1;
    switch (gang) {
    case 1:
        Insult(name, *reinterpret_cast<int32_t*>(0x00B226F0), reinterpret_cast<const char*>(0x0093BDBC), 0x52, ped);
        shape = 0;   // "Nerds_insult0"
        break;
    case 2:
        Insult(name, *reinterpret_cast<int32_t*>(0x00B226F8), reinterpret_cast<const char*>(0x0093BDE0), 0x50, ped);
        shape = 1;   // "Jocks_insult0"
        break;
    case 4:
        Insult(name, *reinterpret_cast<int32_t*>(0x00B132CC), reinterpret_cast<const char*>(0x0093BDA8), 0x51, ped);
        shape = 3;   // "Greasers_insult0"
        break;
    case 5:
        Insult(name, *reinterpret_cast<int32_t*>(0x00B226F4), reinterpret_cast<const char*>(0x0093BDCC), 0x53, ped);
        shape = 2;   // "Preppies_insult0"
        break;
    default:
        break;
    }
    if (shape >= 0 && player) {
        uint8_t* meter = tagging;
        Tagging_StartTag(meter, shape);
        At<int32_t>(meter, 0x2858) = shape;
    }
    NameKey_FromString(reinterpret_cast<uint32_t*>(self + 0x30), name);
}

// 0x656E20 (thiscall): PropTaggedTrack's tag colour now: its two colours (+0x20, +0x24)
// blended by the time of day (outside) or the blend as is (inside)
Rgba* __fastcall PropTagged_GetColour(uint8_t* self, re::Edx, Rgba* out) {
    float blend = tagColourBlend_C66B20;
    if (!IsAreaInterior(currentArea)) {
        float day = tagColourDay_20C47E0;
        blend = day + (1.0f - day) * (1.0f - tagColourBlend_C66B20);
    }
    float rest = 1.0f - blend;
    auto mix = [&](int at) {
        float v = float(self[0x24 + at]) * blend + float(self[0x20 + at]) * rest;
        return int(uint8_t(int32_t(v)));
    };
    int b = mix(2);
    int g = mix(1);
    int r = mix(0);
    return RGBA_Construct(out, r, g, b, 0xFF);
}

// 0x656F50 (thiscall): PropTaggedTrack::Start: the tag's colours by the style (+0x1C:
// 0 / 1 a random gang's, the breakable tag model its own; 2 fixed) and its name picked
uint8_t __fastcall PropTagged_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    self[0x34] = 0xFF;
    NameKey_FromString(reinterpret_cast<uint32_t*>(self + 0x30), nullptr);
    uint8_t* prop = UsePoint_GetEntity(At<uint8_t*>(self, 0x2C));
    int index;
    if (prop && At<int16_t>(prop, 0x10E) == breakableTagModel) {
        index = 5;
    } else {
        index = Rand() * 3 / 0x7FFF;
    }
    switch (At<int32_t>(self, 0x1C)) {
    case 0:
        At<Rgba>(self, 0x20) = tagColoursA0[index];
        At<Rgba>(self, 0x24) = tagColoursA1[index];
        break;
    case 1:
        At<Rgba>(self, 0x20) = tagColoursB0[index];
        At<Rgba>(self, 0x24) = tagColoursB1[index];
        break;
    case 2:
        At<Rgba>(self, 0x20) = tagColoursC[0];
        At<Rgba>(self, 0x24) = tagColoursC[1];
        break;
    default:
        break;
    }
    PickTagName(self);
    return 0;
}

// 0x6570F0 (thiscall): PropTagCompletedTrack::Start: the player's tags counted (all, and
// the gang's shape's)
uint8_t __fastcall PropTagCompleted_Start(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (ped && At<int32_t>(ped, 0x1310) == 0xD) {
        Stats_Increment(0xC8, 1);
        int32_t shape = At<int32_t>(tagging, 0x2858);
        if (shape == 0) {
            Stats_Increment(0xC9, 1);
        } else if (shape == 2) {
            Stats_Increment(0xCA, 1);
        } else if (shape == 3) {
            Stats_Increment(0xCB, 1);
        } else if (shape == 1) {
            Stats_Increment(0xCC, shape);
        }
    }
    return 1;
}

// 0x657190 (thiscall): PropTagEffectTrack::Update: the prop's tagged fraction by the mode
// (+0x18: 2 the ped's spray (+0x157C, at most 1), 3 its rest, 4 / 5 the blend on (and
// its rest)); running until the blend (+0x20) is 1
uint8_t __fastcall PropTagEffect_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* prop = UsePoint_GetEntity(At<uint8_t*>(self, 0x24));
    if (!prop) {
        return 0;
    }
    switch (At<uint32_t>(self, 0x18)) {
    case 0:
    case 1:
        return 0;
    case 2: {
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
        if (ped) {
            float v = At<float>(ped, 0x157C);
            PropAnim_SetUnknown254(prop, (1.0f > v || v != v) ? v : 1.0f);
        }
        break;
    }
    case 3: {
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
        if (ped) {
            PropAnim_SetUnknown254(prop, 1.0f - At<float>(ped, 0x157C));
        }
        break;
    }
    case 4: {
        float t = At<float>(self, 0x1C) * dt + At<float>(self, 0x20);
        At<float>(self, 0x20) = t;
        PropAnim_SetUnknown254(prop, t);
        break;
    }
    case 5: {
        float t = At<float>(self, 0x1C) * dt + At<float>(self, 0x20);
        At<float>(self, 0x20) = t;
        PropAnim_SetUnknown254(prop, 1.0f - t);
        break;
    }
    default:
        break;
    }
    return At<float>(self, 0x20) == 1.0f ? 0 : 1;
}

// 0x6572B0 (cdecl): an entity turned to the heading in place (out of the world and back)
void __cdecl Entity_SetHeadingInPlace(uint8_t* entity, float heading) {
    vc::Call<void>(entity, 0xC);
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    if (matrix) {
        Matrix_SetRotateZOnly(matrix, heading);
    }
    At<float>(entity, 0x10) = heading;
    vc::Call<void>(entity, 0x8);
}

// 0x6572F0 (thiscall): PropSetTargetSocketTrack::Start: the ped's prop in use (an object)
// given the socket (+0x18) as the ped's target socket (+0x43C)
uint8_t __fastcall PropSetTargetSocket_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        uint8_t* prop = At<uint8_t*>(ped, 0x438);
        if (prop && (prop[0x108] & 7) == 6) {
            At<uint32_t>(ped, 0x43C) = At<uint32_t>(self, 0x18);
        }
    }
    return 0;
}

// 0x657330 (thiscall): PropAnimAttachToPropAnimTrack::Start: a new object of the model
// (+0x18) made in the prop's area and attached at the prop's bone (+0x1C, its index
// +0x2C) by its own (+0x20, +0x30)
uint8_t __fastcall PropAnimAttachToPropAnim_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (!prop) {
        return 0;
    }
    uint8_t* anim = At<uint8_t*>(prop, 0x114);
    if (!anim || !At<void*>(anim, 0x94)) {
        return 0;
    }
    At<int32_t>(self, 0x2C) = Prop_FindBone(prop, At<int32_t>(self, 0x1C));
    At<uint8_t*>(self, 0x28) = At<uint8_t*>(prop, 0x114);
    int model = GetModelIndexByHash(At<uint32_t>(self, 0x18), weaponModelFirst, weaponModelLast + 1);
    NodeTransform transform;
    transform.scale = 1.0f;
    NiTransform_MakeIdentity(&transform);
    uint8_t* object = CreateTriggerObject_44CE70(model, prop[0x111], &transform, 0.0f, nullptr, 0, 0);
    vc::Call<void>(object, 0x28, 1, 1);
    PropAnim_4ED940(prop);
    int bone = Prop_FindBone(object, At<int32_t>(self, 0x20));
    At<int32_t>(self, 0x30) = bone;
    PropAnim_AddChild(prop, At<int32_t>(self, 0x2C), object, bone);
    return 1;
}

} // namespace

// (variadic: the template arguments' commas)
#define PROPTRK2_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

PROPTRK2_REVERSED("PropCollisionTrack::Update", 0x00656500, &PropCollision_Update);
PROPTRK2_REVERSED("PropCollisionTrack::End", 0x00656A10, &PropCollision_End);
PROPTRK2_REVERSED("PropTaggedTrack::Destruct", 0x00656A30, &af::Fields<af::Vt(0x009353E0), af::KeyDtor(0x30), af::KeyDtor(0x18), af::TrackBaseDtor>);
PROPTRK2_REVERSED("PropTaggedTrack::PickName", 0x00656A60, &PropTagged_PickName);
PROPTRK2_REVERSED("PropTaggedTrack::GetColour", 0x00656E20, &PropTagged_GetColour);
PROPTRK2_REVERSED("PropTaggedTrack::Start", 0x00656F50, &PropTagged_Start);
PROPTRK2_REVERSED("PropTagCompletedTrack::Start", 0x006570F0, &PropTagCompleted_Start);
PROPTRK2_REVERSED("PropTagEffectTrack::Update", 0x00657190, &PropTagEffect_Update);
PROPTRK2_REVERSED("Entity::SetHeadingInPlace_6572B0", 0x006572B0, &Entity_SetHeadingInPlace);
PROPTRK2_REVERSED("PropSetTargetSocketTrack::Start", 0x006572F0, &PropSetTargetSocket_Start);
PROPTRK2_REVERSED("PropAnimAttachToPropAnimTrack::Start", 0x00657330, &PropAnimAttachToPropAnim_Start);
