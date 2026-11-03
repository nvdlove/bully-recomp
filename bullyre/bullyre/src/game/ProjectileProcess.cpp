// CProjectile's ProcessControl (vf12): its sound bank kept loaded, the fly-by sound
// when it passes within 2 m of the player's head, then what its info says it does:
// rolling on the ground, thrust, the weapon's +0x124 behaviour, or a shadow.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Object.h"
#include "reversed.h"

namespace c11projproc {

using veh::CVector;

// CObject's fields plus (the layout of ProjectileEntity.cpp's)
//   +0x22C uint8, +0x22D bit 0 (the fly-by sound played), +0x230 float, +0x234 uint8,
//   +0x238 its projectile info's index, +0x23C low 3 bits, +0x240 bit 0, +0x244 float
//   (the damage it does to peds it passes), +0x248 its sound
struct CProjectile : CObject {
    uint8_t unknown22C;
    uint8_t unknown22D;
    uint8_t unknown22E[0x2];
    float unknown230;
    uint8_t unknown234;
    uint8_t unknown235[0x3];
    int32_t infoIndex;          // +0x238
    uint32_t unknown23C;
    uint8_t unknown240;
    uint8_t unknown241[0x3];
    float pedDamage;            // +0x244
    uint32_t sound;             // +0x248
};
static_assert(offsetof(CProjectile, unknown22C) == 0x22C);
static_assert(offsetof(CProjectile, unknown22D) == 0x22D);
static_assert(offsetof(CProjectile, infoIndex) == 0x238);
static_assert(offsetof(CProjectile, pedDamage) == 0x244);
static_assert(offsetof(CProjectile, sound) == 0x248);
static_assert(sizeof(CProjectile) == 0x24C);

// vf12 0x4D23E0: nothing while +0xB0 is set. Its model's sound bank requested (8 s for
// one model, else 3 s); once, when its weapon has a fly-by sound and it's within 2 m of
// the player's head (1.5 above the player's position), the sound played at the point of
// its path nearest the player. Then: a frisbee's flight (info +0x15; only with a Gamebryo
// object: its collision state reset, then FrisbeeFlight), thrust (info +0x10: CObject's, then
// ApplyThrust), the weapon's +0x124 (CObject's, then 0x4D21E0), else in the shadow list
// and CObject's
void __fastcall ProcessControl(CProjectile* self, re::Edx edx);

// 0x4D1AB0: a frisbee's flight (the FrisbeeButes' values: below)
void __fastcall FrisbeeFlight(CProjectile* self, re::Edx edx);

} // namespace c11projproc

namespace {

using namespace c11projproc;

GAME_FN(int, __thiscall, SoundBanks_ForModel, 0x00598180, void* banks, int model);
GAME_FN(void, __thiscall, SoundBanks_Request, 0x0059A860, void* banks, int bank, int timeMs, int arg);
GAME_FN(uint8_t*, __cdecl, GetProjectileInstance, 0x004D34B0, int index);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, Sqrtf, 0x0040DF90, float value);
GAME_FN(void, __thiscall, Vector_Div, 0x0041A4A0, CVector* v, float d);
GAME_FN(float, __thiscall, Vector_DotProduct, 0x00468CD0, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Vector_Scale, 0x0088A480, CVector* v, float s);
GAME_FN(void, __thiscall, Vector_Add, 0x00415200, CVector* v, const CVector* add);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int, uintptr_t, int, int, int,
        int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, Physical_ResetCollisionState, 0x00468DF0, CObject* o);
GAME_FN(void, __thiscall, Projectile_FrisbeeFlight, 0x004D1AB0, CProjectile* p);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);   // (its +0x14)
GAME_FN(void, __thiscall, Matrix_FromNiTransform, 0x00412A70, uint8_t* matrix, const void* transform);
GAME_FN(void, __thiscall, Matrix_Reorthogonalise, 0x00413A80, uint8_t* matrix);
GAME_FN(void, __thiscall, Object_ProcessControl, 0x004CF4A0, CObject* o);
GAME_FN(void, __thiscall, Projectile_ApplyThrust, 0x004D2010, CProjectile* p);
GAME_FN(void, __thiscall, Projectile_4D21E0, 0x004D21E0, CProjectile* p);
GAME_FN(void, __thiscall, ShadowList_Add, 0x006D7370, void* list, void* entity);

GAME_VAR(CEntity*, playerPed, 0x00C1AEA8);
GAME_VAR(float, timeStep, 0x00C1A9A4);           // CTimer::ms_fTimeStep
GAME_VAR(uint8_t*, frisbeeButes, 0x020C88BC);    // the FrisbeeButes
GAME_VAR(int32_t, modelBF4394, 0x00BF4394);   // (its bank is kept for 8 s)
inline void* const soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const shadowList = reinterpret_cast<void*>(0x020C7E98);

using re::At;

CVector& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVector*>(matrix + 0x30) : At<CVector>(entity, 4);
}

} // namespace

namespace c11projproc {

// 0x4D1AB0 (thiscall): a frisbee's flight, from the FrisbeeButes (0x20C88BC: +0x18 a time
// scale, +0x1C the speed the lift's log is taken against, +0x20 the base lift, +0x24 the
// lift's cap, +0x28 the damping of the speed along its up axis, +0x2C gravity, +0x30 the
// air's drag, +0x34 the ground's friction). In units per second (the move speed is per
// 1/50 s, divided by the scale; none with a scale of 0) over the frame's time (at most
// 0.0333 s) times the scale: unless it's on something (+0xB0), a force along its up axis
// (the lift, 0.5 log10(speed across it * 100 / +0x1C + 1) over the base, capped, times
// -gravity) less the damped speed along the axis; gravity; the drag; on something, no
// vertical speed, the friction, and +0x20 cleared once the speed is under 0.01. Then its
// matrix taken from its Gamebryo transform (+0x40) and reorthogonalised, and the move speed
// stored back (per 1/50 s, times the scale).
void __fastcall FrisbeeFlight(CProjectile* self, re::Edx) {
    const uint8_t* butes = frisbeeButes;
    // (float arithmetic, in the game's order: its x87 runs in single precision)
    CVector v{self->moveSpeed.x * 50.0f, self->moveSpeed.y * 50.0f, self->moveSpeed.z * 50.0f};
    uint8_t* matrix = Entity_GetMatrix(self);   // (the game calls it twice: the first result unused)
    CVector up = At<CVector>(matrix, 0x20);
    float dt = timeStep / 50.0f;
    float scale = At<float>(butes, 0x18);
    if (0.0333f < dt) {   // (the float at 0x9137D8)
        dt = 0.0333f;
    }
    dt = scale * dt;
    if (scale == 0.0f) {
        v = CVector{0.0f, 0.0f, 0.0f};
    } else {
        float inverse = 1.0f / scale;
        v.x = v.x * inverse;
        v.y = v.y * inverse;
        v.z = inverse * v.z;
    }
    CVector force{0.0f, 0.0f, 0.0f};
    bool resting = self->unknownB0 != 0;
    if (!resting) {
        float along = (up.x * v.x + up.y * v.y) + up.z * v.z;
        CVector normal{up.x * along, up.y * along, up.z * along};
        CVector across;
        VectorSub(&across, &v, &normal);
        // (the CRT's x87 sqrt, 0x85C500, of the float sum's fabs)
        float speed =
            float(std::sqrt(double(std::fabs((across.x * across.x + across.y * across.y) + across.z * across.z))));
        float lift = At<float>(butes, 0x20);
        if (speed > 1.0e-5f) {   // (the float at 0x8FF230)
            float x = speed * 100.0f / At<float>(butes, 0x1C) + 1.0f;
            float log = float(std::log10(double(x)));   // (the CRT's _CIlog10, 0x85E020)
            lift = log * 0.5f + lift;
        }
        if (At<float>(butes, 0x24) < lift) {
            lift = At<float>(butes, 0x24);
        }
        lift = -At<float>(butes, 0x2C) * lift;
        float damping = At<float>(butes, 0x28);
        force.x = up.x * lift - normal.x * damping;
        force.y = up.y * lift - normal.y * damping;
        force.z = up.z * lift - normal.z * damping;
    }
    v.z = v.z + At<float>(butes, 0x2C) * dt;
    v.x = force.x * dt + v.x;
    v.y = force.y * dt + v.y;
    v.z = force.z * dt + v.z;
    float drag = At<float>(butes, 0x30);
    float dragX = -v.x * drag, dragY = -v.y * drag, dragZ = drag * -v.z;
    v.x = v.x + dragX * dt;
    v.y = v.y + dragY * dt;
    v.z = v.z + dragZ * dt;
    if (resting) {
        v.z = 0.0f;
        float horizontal = float(std::sqrt(double(std::fabs(v.y * v.y + v.x * v.x))));
        if (horizontal < 0.01f) {   // (the float at 0x90054C)
            self->unknown20 = 0;
        }
        float friction = At<float>(butes, 0x34);
        float frictionX = -v.x * friction, frictionY = -v.y * friction;
        float frictionZ = friction * -0.0f;   // (the game's -v.z with v.z just 0: folded)
        v.x = v.x + frictionX * dt;
        v.y = v.y + frictionY * dt;
        v.z = dt * frictionZ + 0.0f;
    }
    matrix = Entity_GetMatrix(self);
    Matrix_FromNiTransform(matrix, At<void*>(matrix, 0x40));
    Matrix_Reorthogonalise(Entity_GetMatrix(self));
    self->moveSpeed.x = v.x * 0.02f;
    self->moveSpeed.y = v.y * 0.02f;
    self->moveSpeed.z = 0.02f * v.z;
    self->moveSpeed.x = self->moveSpeed.x * scale;
    self->moveSpeed.y = scale * self->moveSpeed.y;
    self->moveSpeed.z = scale * self->moveSpeed.z;
}

void __fastcall ProcessControl(CProjectile* self, re::Edx) {
    if (self->unknownB0 != 0) {
        return;
    }
    int bank = SoundBanks_ForModel(soundBanks, self->modelIndex);
    if (bank != -1) {
        SoundBanks_Request(soundBanks, bank, self->modelIndex == modelBF4394 ? 8000 : 3000, 0);
    }
    uint8_t* info = GetProjectileInstance(self->infoIndex);
    if (At<const char*>(At<uint8_t*>(info, 0x30), 0xA8) && !(self->unknown22D & 1) && playerPed) {
        CVector diff;
        VectorSub(&diff, &Position(playerPed), &Position(self));
        // (float arithmetic, in the game's order: its x87 runs in single precision)
        diff.z = diff.z + 1.5f;
        float distSq = (diff.y * diff.y + diff.x * diff.x) + diff.z * diff.z;
        if (distSq <= 4.0f) {   // (NaN: no sound)
            // the point of its path (its direction through its position) nearest the head
            CVector nearest = self->moveSpeed;
            Vector_Normalise(&nearest);
            float dist = Sqrtf(distSq);
            Vector_Div(&diff, dist);
            float along = Vector_DotProduct(&nearest, &diff) * dist;
            Vector_Scale(&nearest, along);
            Vector_Add(&nearest, &Position(self));
            int id = Sounds_GetId(sounds, At<const char*>(At<uint8_t*>(info, 0x30), 0xA8));
            Sounds_PlayAt(sounds, id, &nearest, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 0, 1.0f, 0);
            self->unknown22D |= 1;
        }
    }
    if (info[0x15]) {
        if (!self->rwObject) {
            return;
        }
        self->flags1BC &= 0xEF;
        At<int32_t>(self, 0x2C) = 0;
        At<int32_t>(self, 0x38) = 0;
        At<int32_t>(self, 0x40) = 0;
        At<int32_t>(self, 0x7C) = 0;
        At<uint8_t>(self, 0x181) = 0;
        At<int32_t>(self, 0x4C) = 0;
        Physical_ResetCollisionState(self);
        Projectile_FrisbeeFlight(self);
        return;
    }
    if (At<int32_t>(info, 0x10) != 0) {
        Object_ProcessControl(self);
        Projectile_ApplyThrust(self);
        return;
    }
    if (At<uint8_t>(At<uint8_t*>(info, 0x30), 0x124)) {
        Object_ProcessControl(self);
        Projectile_4D21E0(self);
        return;
    }
    ShadowList_Add(shadowList, self);
    Object_ProcessControl(self);
}

} // namespace c11projproc

// ---- registration ---------------------------------------------------------------------

#define PROJPROC_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

PROJPROC_REVERSED("CProjectile::ProcessControl", 0x004D23E0, &c11projproc::ProcessControl);
PROJPROC_REVERSED("CProjectile::FrisbeeFlight", 0x004D1AB0, &c11projproc::FrisbeeFlight);
