// The dodgeball minigame's ball in flight and two of its script natives
// (0x6800A0..0x681720; the AI players' tracks around it are in DodgeballAIPlayers.cpp).
//
// A thrown ball (a projectile entity) moves by its own sweep: its velocity (+0x124), spin
// (+0x130) and "rolling" bit (+0x1BD bit 3) stepped by a small context on the stack
// (BallContext: the entities hit this step, and the physics butes for the ball's model,
// DodgeballPhysicsButes: +0x18 the speed it stops below, +0x2C friction, +0x30 bounce,
// +0x34 rolling drag, +0x38 the speed a hit counts from, +0x3C the hard-hit speed, +0x40 /
// +0x44 spin, +0x48 the hard bounce's scatter, +0x4C the top speed, +0x50 the top upward
// speed).

#include <cmath>
#include <cstdint>

#include "game/ScriptApi.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct V3 {
    float x, y, z;
};
static_assert(sizeof(V3) == 0xC);

// CColPoint (as the sweep fills it)
struct BallColPoint {
    V3 pos;               // +0x0
    re::Ptr32<uint8_t> entity;      // +0xC
    V3 normal;            // +0x10
    uint8_t unknown1C[6];
    uint8_t surface;      // +0x22 (the other's)
    uint8_t unknown23;
    float depth;          // +0x24 the share of the move made
};
static_assert(sizeof(BallColPoint) == 0x28);

// The step's context (0x30 bytes on the caller's stack)
struct BallContext {
    int32_t count;        // +0x0
    re::Ptr32<uint8_t> hits[10];    // +0x4 the entities hit this step
    re::Ptr32<uint8_t> butes;       // +0x2C
};
static_assert(sizeof(BallContext) == 0x30);

GAME_VAR(uint8_t*, dbbGame, 0x01B89150);
GAME_VAR(uint32_t, dbbTimeMs, 0x00C1A9B4);
GAME_VAR(float, dbbTimeStep, 0x00C1A9A4);     // CTimer::ms_fTimeStep
GAME_VAR(uint8_t*, dbbPlayerPed, 0x00C1AEA8);
GAME_ARRAY(float, dbbZeroVector, 0x00BCC564);
GAME_VAR(uint8_t*, dbbDefaultPhysics, 0x020C888C);
GAME_VAR(uint32_t, dbbPhysicsKey, 0x01B65374);
inline void* const dbbPhysicsRoot = reinterpret_cast<void*>(0x01B65378);
GAME_ARRAY(BallColPoint, dbbColPoints, 0x00C216C0);
GAME_VAR(uint8_t, dbbTestingSphere, 0x00C1AE8A);
// what the ball's sweep ignores: the thrower, the ball, the thrower's +0x1554, a model
GAME_ARRAY(uint8_t*, dbbIgnored, 0x01B8916C);
GAME_VAR(int32_t, dbbIgnoredModel, 0x00B9DA7C);
GAME_VAR(float, dbbDuckHeight, 0x00B9DA80);     // a ducking player's float above this dodges
GAME_ARRAY(uint8_t*, dbbModelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, dbbModelBallA, 0x00BF4398);
GAME_VAR(int32_t, dbbModelBallB, 0x00BF4394);
GAME_VAR(int32_t, dbbModelIgnoredSelf, 0x00BF4338);
GAME_VAR(int32_t, dbbModelTimed, 0x00BF432C);
GAME_VAR(int32_t, dbbModelNoVoxA, 0x00BF42DC);
GAME_VAR(int32_t, dbbModelNoVoxB, 0x00BF42D8);
GAME_VAR(int32_t, dbbModelEvents, 0x00BF43F0);
inline uint8_t* const dbbEvents = reinterpret_cast<uint8_t*>(0x01B89178);
inline void* const dbbSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline const char* const dbbAiActName = reinterpret_cast<const char*>(0x0093E96C);
inline const char* const dbbAiNodeName = reinterpret_cast<const char*>(0x0093E964);
inline const char* const dbbMaleHitVox = reinterpret_cast<const char*>(0x0090CFD4);     // "MaleHitVox"
inline const char* const dbbFemaleHitVox = reinterpret_cast<const char*>(0x0090CFE0);   // "FemaleHitVox"

GAME_FN(uint8_t, __cdecl, Dodgeball_IsActive, 0x00683A30);
GAME_FN(void, __cdecl, Dodgeball_SetPed, 0x00683A10, void* setup);
GAME_FN(void, __thiscall, Dodgeball_SetHelp, 0x0067B620, uint8_t* game, int on);
GAME_FN(int, __thiscall, Dodgeball_PlayerIndex, 0x0067AF20, uint8_t* game, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Dodgeball_PlayerOf, 0x0067B210, uint8_t* game, uint8_t* ped);
GAME_FN(float, __thiscall, Dodgeball_PlayerDuck, 0x0067B260, uint8_t* game, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Dodgeball_SameTeam, 0x0067B100, uint8_t* game, uint8_t* a, uint8_t* b);
GAME_FN(uint32_t*, __thiscall, Dodgeball_GetFlags, 0x0067B0E0, uint8_t* game, uint32_t* out);
GAME_FN(void, __thiscall, Dodgeball_SetFlags, 0x0067B0F0, uint8_t* game, uint32_t flags);
GAME_FN(int, __thiscall, Dodgeball_TeamCurrent, 0x0067B060, uint8_t* game, int i);
GAME_FN(void, __cdecl, Dodgeball_GiveBall, 0x006730E0, uint8_t* ped);

GAME_FN(uint8_t*, __cdecl, Projectile_Info, 0x004D34B0, int index);
GAME_FN(uint32_t*, __thiscall, NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, HashPath_Set, 0x005FA8C0, int32_t* path, int32_t count, uint32_t* hashes);
GAME_FN(uint8_t*, __cdecl, FindButes, 0x0072AEA0, int32_t* path, void* root);
GAME_FN(void, __stdcall, VectorDestruct, 0x004017C0, void* array, size_t size, int count, void* dtor);
inline void* const nameKeyDestruct = reinterpret_cast<void*>(0x0049A650);
GAME_FN(void, __thiscall, Sweep_Construct, 0x00563060, void* self, const V3* from, const V3* to, float radius,
        int flags, void* hit, void* filter, int a, int b, void* c);
GAME_FN(int32_t, __thiscall, Sweep_Test, 0x00564080, void* self);
GAME_FN(void, __thiscall, Sweep_Destruct, 0x0049A650, void* self);   // (empty)
GAME_FN(float, __thiscall, Entity_GetBoundRadius, 0x00465E10, uint8_t* entity);
GAME_FN(float, __thiscall, Entity_GetHeading, 0x00415470, uint8_t* entity);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const V3* v);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(void, __thiscall, Physical_ApplyCollision, 0x0046B870, uint8_t* physical, uint8_t* entity, BallColPoint* colPoint, float arg);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(uint8_t, __thiscall, Ped_4473E0, 0x004473E0, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, uint8_t* ped);
GAME_FN(int32_t, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(int32_t, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, int32_t sound, const float* pos, int, const float* pos2,
        int volume, int, int, int, void* callback, void* entity, int, int, float, int);
GAME_FN(void, __thiscall, Sounds_Collision, 0x0044C060, void* sounds, const V3* pos, float volume, int surface, int model);   // (empty)
GAME_FN(void, __cdecl, Stat_AddToInt, 0x00458480, int32_t stat, int32_t value);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, uint32_t duration, int strength, int flag);
GAME_FN(uint8_t, __thiscall, Events_IsOn, 0x005D80E0, uint8_t* events);
GAME_FN(void*, __thiscall, Events_New, 0x005D9C20, uint8_t* events);
GAME_FN(int32_t, __thiscall, Events_GetId, 0x005D80D0, uint8_t* events);
GAME_FN(void, __cdecl, Event_Begin, 0x005D8270, void* event);
GAME_FN(void, __cdecl, Event_SetId, 0x005D8670, void* event, int32_t id);
GAME_FN(void, __cdecl, Event_AddInt, 0x005D85C0, void* event, int32_t value);
GAME_FN(void, __cdecl, Event_Send, 0x005D82A0, void* event, int kind, int arg);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const void* a, const void* b);
GAME_FN(V3*, __cdecl, VectorSub, 0x00414B70, V3* out, const void* a, const void* b);
GAME_FN(V3*, __cdecl, CrossProduct, 0x00414730, V3* out, const void* a, const void* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, V3* v);
inline int __cdecl CrtRand() { return crt::Rand(); }
// ours, through the game's addresses
GAME_FN(void, __thiscall, Physics_SetButes, 0x006802A0, BallContext* self, uint8_t* model);
GAME_FN(void, __stdcall, Physics_BounceSound, 0x00680340, uint8_t* ball, BallColPoint* colPoint, float speed);
GAME_FN(float, __thiscall, Physics_Bounce, 0x00680450, BallContext* self, float radius, BallColPoint* colPoint, V3* pos,
        V3* velocity, float moveX, float moveY, float moveZ, uint8_t* rolling, V3* spin, uint8_t* model, uint8_t* ball,
        int first, V3* lastNormal);
GAME_FN(uint8_t, __stdcall, Physics_CanHit, 0x00680C80, uint8_t* ball, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, Physics_Hit, 0x00680D90, BallContext* self, uint8_t* ball, BallColPoint* colPoint);
GAME_FN(uint8_t, __thiscall, Physics_Process, 0x006810B0, BallContext* self, uint8_t* ball);
inline void* const sweepFilter = reinterpret_cast<void*>(0x00680D10);

inline float* PosOf(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m ? reinterpret_cast<float*>(m + 0x30) : reinterpret_cast<float*>(e + 4);
}
inline bool IsPed(uint8_t* e) {
    return (e[0x108] & 7) == 3;
}
inline float& F(uint8_t* p, int offset) {
    return At<float>(p, offset);
}
// 2 * rand() / 32767 - 1
inline float RandomSigned() {
    float r = float(CrtRand()) / 32767.0f;
    return (r + r) - 1.0f;
}
inline float Length(const V3& v) {
    return std::sqrt(std::fabs((v.x * v.x + v.y * v.y) + v.z * v.z));
}

// ---- natives ----------------------------------------------------------------------------

// The setup DodgeballSetPed hands the game (0x30 bytes)
struct PedSetup {
    int32_t team;             // +0x0
    int32_t slot;             // +0x4
    re::Ptr32<void> ped;                // +0x8
    re::Ptr32<const char> args3[2];     // +0xC
    re::Ptr32<const char> actFile;      // +0x14 "DB_AI.act"
    re::Ptr32<const char> node;         // +0x18
    re::Ptr32<const char> args5[5];     // +0x1C
};
static_assert(sizeof(PedSetup) == 0x30);

// 0x6800A0: DodgeballSetPed(team, slot, ped handle (0: the player), 7 names)
int __cdecl Native_DodgeballSetPed(lua_State* L) {
    PedSetup setup;
    setup.team = Script::GetInt(L, 0);
    setup.slot = Script::GetInt(L, 1);
    setup.args3[0] = Script::GetString(L, 3);
    setup.args3[1] = Script::GetString(L, 4);
    setup.actFile = dbbAiActName;
    setup.node = dbbAiNodeName;
    setup.args5[0] = Script::GetString(L, 5);
    setup.args5[1] = Script::GetString(L, 6);
    setup.args5[2] = Script::GetString(L, 7);
    setup.args5[3] = Script::GetString(L, 8);
    setup.args5[4] = Script::GetString(L, 9);
    int handle = Script::GetInt(L, 2);
    setup.ped = handle ? ScriptGame::GetPedFromHandle(handle, 2) : dbbPlayerPed;
    Dodgeball_SetPed(&setup);
    return 0;
}

// 0x680270: DodgeballHelpMsg(on)
int __cdecl Native_DodgeballHelpMsg(lua_State* L) {
    uint8_t on = Script::GetBool(L, 0);
    if (dbbGame) {
        Dodgeball_SetHelp(dbbGame, on);
    }
    return 0;
}

// ---- the ball's step --------------------------------------------------------------------

// 0x6802A0 (thiscall): the physics butes for the ball's model (by its name under the
// physics key; the defaults for none)
void __fastcall Physics_SetButesImpl(BallContext* self, re::Edx, uint8_t* model) {
    if (!model) {
        self->butes = dbbDefaultPhysics;
        return;
    }
    self->butes = nullptr;
    if (At<const char*>(model, 0x12C)) {
        uint32_t keys[2];
        NameKey_Copy(&keys[0], &dbbPhysicsKey);
        NameKey_Construct(&keys[1], At<const char*>(model, 0x12C));
        int32_t path[2];
        HashPath_Set(path, 2, keys);
        self->butes = FindButes(path, dbbPhysicsRoot);
        VectorDestruct(keys, 4, 2, nameKeyDestruct);
    }
    if (!self->butes) {
        self->butes = dbbDefaultPhysics;
    }
}

// 0x680340 (thiscall, `this` unused): a bounce's sound (a stub now) at its speed x 255
// (10..127), and for the event ball on a surface 0x13 / 0x22 a script event
void __stdcall Physics_BounceSoundImpl(uint8_t* ball, BallColPoint* colPoint, float speed) {
    float volume = speed * 255.0f;
    uint8_t surface = colPoint->surface;
    if (volume > 127.0f) {
        volume = 127.0f;
    } else if (10.0f > volume) {
        volume = 10.0f;
    }
    if (At<int16_t>(ball, 0x10E) == dbbModelEvents && Events_IsOn(dbbEvents)) {
        void* event = Events_New(dbbEvents);
        Event_Begin(event);
        Event_SetId(event, Events_GetId(dbbEvents));
        if (surface == 0x13) {
            Event_AddInt(event, 1);
        } else if (surface == 0x22) {
            Event_AddInt(event, 0);
        }
        Event_Send(event, 1, 0);
    }
    V3 pos = colPoint->pos;
    Sounds_Collision(dbbSounds, &pos, volume, surface, At<int16_t>(ball, 0x10E));
}

// 0x680450 (thiscall, ret 0x34): a bounce off the sweep's hit: the ball moved to the hit
// (the share `depth` of this move); a hard hit (faster into the surface than +0x3C)
// bounces off a scattered normal (friction along, restitution across, capped) and spins;
// a soft one slides (rolling from then on: dragged to a stop). Its velocity kept from
// heading back into the last surface. Returns the share of the move made.
float __fastcall Physics_BounceImpl(BallContext* self, re::Edx, float radius, BallColPoint* colPoint, V3* pos,
                                    V3* velocity, float moveX, float moveY, float moveZ, uint8_t* rolling, V3* spin,
                                    uint8_t*, uint8_t* ball, int first, V3* lastNormal) {
    uint8_t* butes = self->butes;
    const V3& n = colPoint->normal;
    float speed = Length(*velocity);
    float into = (n.y * velocity->y + n.x * velocity->x) + n.z * velocity->z;
    float depth = colPoint->depth;
    V3 moved{moveX * depth, moveY * depth, depth * moveZ};
    float movedLength = std::sqrt(std::fabs((moved.y * moved.y + moved.x * moved.x) + moved.z * moved.z));
    float moveLength = std::sqrt(std::fabs((moveY * moveY + moveX * moveX) + moveZ * moveZ));
    float share = movedLength / moveLength;
    pos->x = pos->x + moved.x;
    pos->y = pos->y + moved.y;
    pos->z = moved.z + pos->z;
    V3 up{0.0f, 0.0f, 1.0f};
    V3 cross;
    V3 tmp;
    if (F(butes, 0x3C) < -into) {
        float scatter = F(butes, 0x48);
        float r1 = RandomSigned();
        float r2 = RandomSigned();
        float r3 = RandomSigned();
        V3 jitter{r3 * scatter, r2 * scatter, scatter * r1};
        V3 bounceNormal;
        VectorAdd(&bounceNormal, &n, &jitter);
        V3 across{bounceNormal.x * into, bounceNormal.y * into, into * bounceNormal.z};
        V3 along;
        VectorSub(&along, velocity, &across);
        float restitution = -F(butes, 0x30);
        V3 back{across.x * restitution, across.y * restitution, restitution * across.z};
        float friction = F(butes, 0x2C);
        V3 slide{along.x * friction, along.y * friction, friction * along.z};
        *velocity = *VectorAdd(&tmp, &slide, &back);
        if (F(butes, 0x4C) < Length(*velocity)) {
            Vector_Normalise(velocity);
            float cap = F(butes, 0x4C);
            *velocity = V3{cap * velocity->x, cap * velocity->y, cap * velocity->z};
        }
        if (F(butes, 0x50) < velocity->z) {
            velocity->z = F(butes, 0x50);
        }
        *rolling = 0;
        float spinScale = F(butes, 0x40);
        CrossProduct(&cross, &up, velocity);
        V3 turn{cross.x * radius, cross.y * radius, radius * cross.z};
        *spin = V3{turn.x * spinScale, turn.y * spinScale, spinScale * turn.z};
        Physics_BounceSound(ball, colPoint, speed);
    } else {
        V3 across{n.x * into, n.y * into, into * n.z};
        V3 along;
        VectorSub(&along, velocity, &across);
        if (*rolling == 0) {
            V3 lift{n.x * 9.999999747378752e-05f, n.y * 9.999999747378752e-05f, 9.999999747378752e-05f * n.z};
            float friction = F(butes, 0x2C);
            V3 slide{along.x * friction, along.y * friction, friction * along.z};
            *velocity = *VectorAdd(&tmp, &slide, &lift);
            *rolling = 1;
            float spinScale = F(butes, 0x40);
            CrossProduct(&cross, &up, velocity);
            V3 turn{radius * cross.x, cross.y * radius, radius * cross.z};
            *spin = V3{turn.x * spinScale, turn.y * spinScale, spinScale * turn.z};
        } else {
            V3 against{along.x * -1.0f, along.y * -1.0f, -1.0f * along.z};
            Vector_Normalise(&against);
            float drag = F(butes, 0x34) * 0.00800000037997961f * dbbTimeStep;
            V3 slow{against.x * drag, against.y * drag, drag * against.z};
            V3 slowed;
            VectorAdd(&slowed, &along, &slow);
            float same = (slowed.y * along.y + slowed.x * along.x) + slowed.z * along.z;
            if (0.0f < same) {
                V3 lift{n.x * 0.0010000000474974513f, n.y * 0.0010000000474974513f, 0.0010000000474974513f * n.z};
                *velocity = *VectorAdd(&tmp, &slowed, &lift);
            } else {
                *velocity = V3{dbbZeroVector[0], dbbZeroVector[1], dbbZeroVector[2]};
            }
            float spinScale = F(butes, 0x44);
            CrossProduct(&cross, &up, velocity);
            V3 turn{cross.x * radius, cross.y * radius, radius * cross.z};
            *spin = V3{turn.x * spinScale, turn.y * spinScale, spinScale * turn.z};
        }
    }
    if (!re::Flag(first)) {
        float back = (lastNormal->x * velocity->x + velocity->y * lastNormal->y) + velocity->z * lastNormal->z;
        if (back < 0.0f) {
            float k = -back;
            V3 out{k * lastNormal->x, k * lastNormal->y, k * lastNormal->z};
            velocity->x = out.x + velocity->x;
            velocity->y = out.y + velocity->y;
            velocity->z = out.z + velocity->z;
        }
    }
    *lastNormal = n;
    return share;
}

// 0x680C80 (stdcall): the ball may hit the entity: always outside the game; else not
// the thrower's team-mates
uint8_t __stdcall Physics_CanHitImpl(uint8_t* ball, uint8_t* entity) {
    if (!Dodgeball_IsActive()) {
        return 1;
    }
    uint8_t* thrower = At<uint8_t*>(Projectile_Info(At<int32_t>(ball, 0x238)), 4);
    return Dodgeball_SameTeam(dbbGame, thrower, entity) == 0;
}

// 0x680CC0 (stdcall): in the game, a ped player ducking low enough to dodge
uint8_t __stdcall Dodgeball_IsDodgingImpl(uint8_t* entity) {
    if (!Dodgeball_IsActive() || !entity || !IsPed(entity)) {
        return 0;
    }
    float duck = Dodgeball_PlayerDuck(dbbGame, entity);
    return dbbDuckHeight < duck;
}

// 0x680D10 (cdecl; missing from Ghidra's list: the sweep's filter): skip dodging players,
// the ignored entities and the ignored model
uint8_t __cdecl Physics_SweepFilter(uint8_t* entity) {
    if (Dodgeball_IsActive() && entity && IsPed(entity)) {
        float duck = Dodgeball_PlayerDuck(dbbGame, entity);
        if (dbbDuckHeight < duck) {
            return 1;
        }
    }
    for (int i = 0; i < 3; i++) {
        if (entity == dbbIgnored[i]) {
            return 1;
        }
    }
    if (dbbIgnoredModel != -1 && dbbIgnoredModel == At<int16_t>(entity, 0x10E)) {
        return 1;
    }
    return 0;
}

// 0x680D90 (thiscall, ret 8): the ball touched an entity: a player who tried to catch in
// the last 150 ms catches it (false: the step ends); otherwise a hit fast enough (+0x38)
// on something not hit yet this step and allowed: the game's hit flag (0x40), for a
// player its hit (out; stats, the rumble for the first team's current), the collision
// applied, a hit voice by the speed
uint8_t __fastcall Physics_HitImpl(BallContext* self, re::Edx, uint8_t* ball, BallColPoint* colPoint) {
    uint8_t* entity = colPoint->entity;
    if (Dodgeball_IsActive() && IsPed(entity) && Dodgeball_PlayerIndex(dbbGame, entity) != -1) {
        uint8_t* player = Dodgeball_PlayerOf(dbbGame, entity);
        if (dbbTimeMs - At<uint32_t>(player, 0x238) < 150) {
            Dodgeball_GiveBall(At<uint8_t*>(player, 0));
            At<uint32_t>(player, 0x238) = 0;
            return 0;
        }
    }
    V3* velocity = &At<V3>(ball, 0x124);
    const V3& n = colPoint->normal;
    float into = std::fabs((velocity->y * n.y + velocity->x * n.x) + velocity->z * n.z);
    if (F(self->butes, 0x38) > into) {
        return 1;
    }
    for (int i = 0; i < self->count; i++) {
        if (self->hits[i] == entity) {
            return 1;
        }
    }
    if (!Physics_CanHit(ball, entity)) {
        return 1;
    }
    if (Dodgeball_IsActive()) {
        uint32_t flags;
        Dodgeball_GetFlags(dbbGame, &flags);
        flags |= 0x40;
        Dodgeball_SetFlags(dbbGame, flags);
    }
    if (IsPed(entity)) {
        if (Dodgeball_IsActive()) {
            int index = Dodgeball_PlayerIndex(dbbGame, entity);
            uint8_t* player = Dodgeball_PlayerOf(dbbGame, entity);
            At<uint32_t>(player, 4) &= 0xFFFFCF3F;
            F(player, 8) = 0.0f;
            if (index == Dodgeball_TeamCurrent(dbbGame, 0)) {
                Stat_AddToInt(0xD1, 1);
                Pad_Rumble(0, 0x3C0, 0xFF, 0);
            } else {
                Stat_AddToInt(0xD3, 1);
            }
        }
        float heading = Entity_GetHeading(entity);
        float normalHeading = HeadingFromVector(&n);
        float diff = AngleDiff(heading, normalHeading);
        Physical_ApplyCollision(ball, entity, colPoint, diff);
        float volume = Vector_Magnitude(velocity) * 127.0f;
        if (volume > 127.0f) {
            volume = 127.0f;
        }
        if (volume > 31.5f && !Ped_4473E0(entity) && At<int16_t>(entity, 0x10E) != dbbModelNoVoxA &&
            At<int16_t>(entity, 0x10E) != dbbModelNoVoxB) {
            int32_t sound = Sounds_GetId(dbbSounds, dbbMaleHitVox);
            if (Ped_GetModelInfo(entity)[0x50]) {
                sound = Sounds_GetId(dbbSounds, dbbFemaleHitVox);
            }
            const float* pos = PosOf(entity);
            // (the volume truncated, its low byte)
            Sounds_Play(dbbSounds, sound, pos, 5, pos, int(volume) & 0xFF, 0, 0, 0, nullptr, nullptr, 6, 0, 1.0f, 0);
        }
    } else {
        Physical_ApplyCollision(ball, entity, colPoint, 0.0f);
    }
    self->hits[self->count] = entity;
    self->count++;
    return 1;
}

// 0x6810B0 (thiscall, ret 4): the ball's step: gravity over the time step, then up to 5
// sweeps along the move, each hit bouncing (Physics_Bounce) and touching (Physics_Hit)
// with the rest of the step; stops when stuck or slower than the butes' +0x18 (a sixth
// slow step flags the ball, +0xB0, and times out the timed model's projectile). The
// position, velocity, rolling bit and spin written back, then its virtuals +0x98 / +0x90.
// (A catch ends the step early without writing back.)
uint8_t __fastcall Physics_ProcessImpl(BallContext* self, re::Edx, uint8_t* ball) {
    uint8_t* info = Projectile_Info(At<int32_t>(ball, 0x238));
    Physics_SetButes(self, At<uint8_t*>(info, 0x30));
    float step = F(ball, 0x154) * dbbTimeStep;
    float scale = F(ball, 0x230);
    float fall = vc::Call<float>(ball, 0x8C) * 0.5f * scale;
    V3 velocity = At<V3>(ball, 0x124);
    float stepSquared = step * step;
    fall = fall * stepSquared;
    V3 pos;
    {
        const float* p = PosOf(ball);
        pos = V3{p[0], p[1], p[2]};
    }
    float along = step * velocity.z;
    V3 move{velocity.x * step, velocity.y * step, along - fall};
    V3 target;
    VectorAdd(&target, &pos, &move);
    uint8_t rolling = (ball[0x1BD] >> 3) & 1;
    V3 spin = At<V3>(ball, 0x130);
    V3 d;
    VectorSub(&d, &target, &pos);
    V3 back{d.x * 0.0010000000474974513f, d.y * 0.0010000000474974513f, 0.0010000000474974513f * d.z};
    int model = At<int16_t>(ball, 0x10E);
    pos.x = pos.x - back.x;
    pos.y = pos.y - back.y;
    pos.z = pos.z - back.z;
    float radius;
    if (model == dbbModelBallA || model == dbbModelBallB) {
        uint8_t* col = At<uint8_t*>(At<uint8_t*>(dbbModelInfoPtrs[model], 0xC), 0x38);
        radius = F(At<uint8_t*>(col, 0), 0xC);
    } else {
        radius = Entity_GetBoundRadius(ball);
    }
    radius = radius + 9.999999747378752e-05f;
    dbbTestingSphere = 1;
    dbbIgnored[0] = At<uint8_t*>(info, 4);
    dbbIgnored[1] = ball;
    dbbIgnored[2] = nullptr;
    float done = 0.0f;
    uint8_t* thrower = At<uint8_t*>(info, 4);
    if (thrower && IsPed(thrower)) {
        dbbIgnored[2] = At<uint8_t*>(thrower, 0x1554);
    }
    V3 lastNormal{0.0f, 0.0f, 0.0f};
    dbbIgnoredModel = -1;
    if (model == dbbModelIgnoredSelf) {
        dbbIgnoredModel = dbbModelIgnoredSelf;
    }
    alignas(4) uint8_t sweep[0x54];
    int bounces = 0;
    for (;;) {
        Sweep_Construct(sweep, &pos, &target, radius, 0x9F, dbbColPoints, sweepFilter, 1, 1, ball);
        int32_t hit = Sweep_Test(sweep);
        if (hit) {
            float share = Physics_Bounce(self, radius, dbbColPoints, &pos, &velocity, move.x, move.y, move.z, &rolling,
                                         &spin, At<uint8_t*>(info, 0x30), ball, bounces == 0, &lastNormal);
            bounces++;
            done = share * (1.0f - done) + done;
            float remaining = (1.0f - done) * dbbTimeStep;
            if (bounces > 1 && done < 0.009999999776482582f) {
                // stuck: stopped where it is
                velocity = V3{dbbZeroVector[0], dbbZeroVector[1], dbbZeroVector[2]};
                target = pos;
                Sweep_Destruct(sweep);
                break;
            }
            if (F(self->butes, 0x18) > Length(velocity)) {
                ball[0x234]++;
                if (ball[0x234] > 5) {
                    At<int32_t>(ball, 0xB0) = 1;
                    if (At<int16_t>(ball, 0x10E) == dbbModelTimed) {
                        At<uint32_t>(info, 0xC) = dbbTimeMs - At<uint32_t>(info, 8) + 5000;
                    }
                }
                target = pos;
                Sweep_Destruct(sweep);
                break;
            }
            if (bounces < 5 && remaining > 0.0f) {
                move = V3{velocity.x * remaining, velocity.y * remaining, remaining * velocity.z};
                V3 next;
                target = *VectorAdd(&next, &pos, &move);
                VectorSub(&d, &target, &pos);
                Vector_Normalise(&d);
                V3 nudge{d.x * 0.0010000000474974513f, d.y * 0.0010000000474974513f, 0.0010000000474974513f * d.z};
                pos.x = pos.x - nudge.x;
                pos.y = pos.y - nudge.y;
                pos.z = pos.z - nudge.z;
            } else {
                target = pos;
                ball[0x234] = 0;
                hit = 0;
            }
            if (!Physics_Hit(self, ball, dbbColPoints) || At<uint8_t>(info, 0x16) != 0) {
                dbbTestingSphere = 0;
                Sweep_Destruct(sweep);
                return 1;
            }
        }
        Sweep_Destruct(sweep);
        if (!hit || bounces >= 5) {
            break;
        }
    }
    dbbTestingSphere = 0;
    dbbIgnored[0] = nullptr;
    dbbIgnored[1] = nullptr;
    dbbIgnored[2] = nullptr;
    float* p = PosOf(ball);
    p[0] = target.x;
    p[1] = target.y;
    p[2] = target.z;
    At<V3>(ball, 0x124) = velocity;
    ball[0x1BD] ^= uint8_t((rolling * 8) ^ ball[0x1BD]) & 8;
    At<V3>(ball, 0x130) = spin;
    vc::Call<void>(ball, 0x98);
    vc::Call<void>(ball, 0x90);
    return 1;
}

// 0x681720 (cdecl): a dodgeball's step (the context built on the stack: nothing hit yet,
// the default butes; the step's result returned: CObject's processing tests it)
uint8_t __cdecl Ball_ProcessImpl(uint8_t* ball) {
    BallContext context;
    context.count = 0;
    context.butes = dbbDefaultPhysics;
    return Physics_Process(&context, ball);
}
// 0x683B40: a lone jmp to 0x681720 (missing from Ghidra's list)
RE_UNHOOKED_FN("Dodgeball::ProcessBall_683B40", 0x00683B40, &Ball_ProcessImpl);

#define DBBALL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

DBBALL_REVERSED("Native_DodgeballSetPed", 0x006800A0, &Native_DodgeballSetPed);
DBBALL_REVERSED("Native_DodgeballHelpMsg", 0x00680270, &Native_DodgeballHelpMsg);
DBBALL_REVERSED("DodgeballPhysics::SetButes", 0x006802A0, &Physics_SetButesImpl);
DBBALL_REVERSED("DodgeballPhysics::BounceSound", 0x00680340, &Physics_BounceSoundImpl);
DBBALL_REVERSED("DodgeballPhysics::Bounce", 0x00680450, &Physics_BounceImpl);
DBBALL_REVERSED("DodgeballPhysics::CanHit", 0x00680C80, &Physics_CanHitImpl);
DBBALL_REVERSED("Dodgeball::IsDodging_680CC0", 0x00680CC0, &Dodgeball_IsDodgingImpl);
DBBALL_REVERSED("DodgeballPhysics::SweepFilter", 0x00680D10, &Physics_SweepFilter);
DBBALL_REVERSED("DodgeballPhysics::Hit", 0x00680D90, &Physics_HitImpl);
DBBALL_REVERSED("DodgeballPhysics::Process", 0x006810B0, &Physics_ProcessImpl);
DBBALL_REVERSED("Dodgeball::ProcessBall_681720", 0x00681720, &Ball_ProcessImpl);

} // namespace
