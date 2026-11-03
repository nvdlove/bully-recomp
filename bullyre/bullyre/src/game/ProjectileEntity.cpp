// CProjectile (0x24C bytes, vtable 0x9059C4, CPools::ms_pProjectilePool): a thrown or
// fired object: its constructor and destructor, its thrust, its gravity, lying flat
// when it comes to rest, and the peds its sphere sweeps through.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Object.h"
#include "reversed.h"

namespace c11projectile {

using veh::CVector;

// CObject's fields plus
//   +0x22C uint8, +0x22D bit 0, +0x230 float (1), +0x234 uint8, +0x238 its projectile
//   info's index, +0x23C low 3 bits, +0x240 bit 0, +0x244 float (the damage it does
//   to peds it passes), +0x248 its sound
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
static_assert(offsetof(CProjectile, unknown230) == 0x230);
static_assert(offsetof(CProjectile, infoIndex) == 0x238);
static_assert(offsetof(CProjectile, pedDamage) == 0x244);
static_assert(offsetof(CProjectile, sound) == 0x248);
static_assert(sizeof(CProjectile) == 0x24C);

// 0x4D15E0: the destructor's body
void __fastcall Destruct(CProjectile* self, re::Edx edx);
// vf36 0x4D1660: gravity (physical flag 2): its vf35 times the time step and scale
void __fastcall ApplyGravity(CProjectile* self, re::Edx edx);
// vf7 0x4D16B0: coming to rest, laid flat (its up made world up) when its info says so
void __fastcall SetIsStatic(CProjectile* self, re::Edx edx, int isStatic);
// 0x4D17C0: every ped in the sectors its bounds cover that its first collision sphere
// passes (within its radius plus 0.66 of the global, and within 1.1 radii in height)
// takes its damage
void __fastcall DamagePedsInPath(CProjectile* self, re::Edx edx, uint8_t* info);
// vf13 0x4D1A30
void __fastcall ProcessCollision(CProjectile* self, re::Edx edx, int arg);
// 0x4D2010: the thrust of its weapon while its info's time lasts (fading out after the
// weapon's +0x100 share of it)
void __fastcall ApplyThrust(CProjectile* self, re::Edx edx);
// 0x4D22B0: the constructor (the object as a mission object with the projectile's
// defaults; an animation data when its model has animations)
CProjectile* __fastcall Construct(CProjectile* self, re::Edx edx, int model, int infoIndex);

} // namespace c11projectile

namespace {

using namespace c11projectile;

GAME_FN(void, __thiscall, Projectile_DamagePedsInPath, 0x004D17C0, CProjectile* p, uint8_t* info);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, uint32_t* sound, uintptr_t id);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(void, __thiscall, AnimData_Destruct, 0x006B5A80, void* data);
GAME_FN(void, __thiscall, AnimData_Construct, 0x006BA240, void* data);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __thiscall, Fn_6D7610, 0x006D7610, void* self, void* entity);
GAME_FN(void, __thiscall, Object_Destruct, 0x004CFBA0, CObject* o);
GAME_FN(CObject*, __thiscall, Object_Construct, 0x004CF410, CObject* o);
GAME_FN(void, __thiscall, Object_SetIsStatic, 0x004CF2E0, CObject* o, int isStatic);
GAME_FN(void, __thiscall, Object_ProcessCollision, 0x004CF9C0, CObject* o, int arg);
GAME_FN(uint8_t*, __cdecl, GetProjectileInstance, 0x004D34B0, int index);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Matrix_Reorthogonalise, 0x00413A80, void* m);
GAME_FN(void*, __cdecl, GetColModel, 0x0051AF50, void* entity);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* TransformPoint(CVector* out, const void* m, const void* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CEntity*, __cdecl, GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_FN(void, __cdecl, Fn_52ABE0, 0x0052ABE0, const CVector* pos, int model, void* entity);
GAME_FN(void, __thiscall, Entity_SetModelIndex, 0x00468660, void* entity, int model, int arg);
GAME_FN(uint8_t*, __cdecl, GetObjectInfo, 0x004D0CA0, int model);

GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(float, pedSweepMargin, 0x00CF311C);
GAME_VAR(uint32_t*, ptrNodes, 0x00C0F788);
GAME_VAR(int32_t, modelBF4328, 0x00BF4328);
GAME_VAR(int32_t, modelBF4334, 0x00BF4334);
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
inline void* const shadowList = reinterpret_cast<void*>(0x020C7E98);   // (0x6D7610 forgets the entity there)
// the world's sectors (36 x 36, 0x14 bytes; the peds' list at +0xC)
inline uint8_t* const sectorPedLists = reinterpret_cast<uint8_t*>(0x00C1B184);
inline void* const projectileVtable = reinterpret_cast<void*>(0x009059C4);

using re::At;

template <typename R, typename... A>
R CallVirtual(void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(object, args...);
}

CVector& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVector*>(matrix + 0x30) : At<CVector>(entity, 4);
}

// (x87 unsigned-to-float: a negative int gets 2^32 added)
double UnsignedToDouble(uint32_t value) {
    return double(int32_t(value)) + (int32_t(value) < 0 ? 4294967296.0 : 0.0);
}

} // namespace

namespace c11projectile {

void __fastcall Destruct(CProjectile* self, re::Edx) {
    self->vtable = static_cast<re::Ptr32<void>*>(projectileVtable);
    if (self->sound) {
        CallVirtual<void>(reinterpret_cast<void*>(self->sound), 13);
        Sound_Set(&self->sound, 0);
    }
    Entity_DeleteRwObject(self);
    void* animData = self->animData;
    if (animData) {
        AnimData_Destruct(animData);
        Free(animData);
        self->animData = nullptr;
    }
    Fn_6D7610(shadowList, self);
    Sound_Set(&self->sound, 0);
    Object_Destruct(self);
}

void __fastcall ApplyGravity(CProjectile* self, re::Edx) {
    if (!(self->flags1BC & 2)) {
        return;
    }
    float ts = timeStep;
    double speedZ = self->moveSpeed.z;
    float gravity = CallVirtual<float>(self, 35);
    self->moveSpeed.z = float(speedZ - double(gravity) * ts * At<float>(self, 0x154));
}

void __fastcall SetIsStatic(CProjectile* self, re::Edx, int isStatic) {
    if (uint8_t(isStatic) && GetProjectileInstance(self->infoIndex)[0x15]) {
        float* m = Entity_GetMatrix(self);
        // up = (0, 0, 1); right = forward x up; forward = up x right (the zeros multiplied
        // out as the game does). (The game also stores uninitialised stack into the
        // matrix's +0x1C and +0x2C; ours leaves them)
        const double zero = 0.0;
        CVector right = {float(m[5] - m[6] * zero), float(m[6] * zero - m[4]), float(m[4] * zero - m[5] * zero)};
        m[8] = 0.0f;
        m[9] = 0.0f;
        m[10] = 1.0f;
        CVector forward = {float(right.z * zero - right.y), float(right.x - right.z * zero),
                           float(right.y * zero - right.x * zero)};
        m[0] = right.x;
        m[1] = right.y;
        m[2] = right.z;
        m[4] = forward.x;
        m[5] = forward.y;
        m[6] = forward.z;
        Matrix_Reorthogonalise(m);
    }
    Object_SetIsStatic(self, isStatic);
}

void __fastcall DamagePedsInPath(CProjectile* self, re::Edx, uint8_t* info) {
    float rect[4];
    CallVirtual<void>(self, 11, rect);   // GetBoundRect
    int x1 = int(double(rect[0]) / 50.0 + 18.0);
    int y1 = int(double(rect[3]) / 50.0 + 18.0);
    int x2 = int(double(rect[2]) / 50.0 + 18.0);
    int y2 = int(double(rect[1]) / 50.0 + 18.0);
    uint8_t* sphere = *At<re::Ptr32<uint8_t>*>(GetColModel(self), 0x38);   // the first collision sphere (data->spheres)
    struct { CVector v; float pad; } centrePad16;   // (the game's transform writes a 4th float)
    CVector& centre = centrePad16.v;
    TransformPoint(&centre, Entity_GetMatrix(self), sphere);
    float margin = float(pedSweepMargin * 0.6600000262260437);
    float radius;
    // (no clamping to the sector grid)
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            uint32_t* node = *reinterpret_cast<re::Ptr32<uint32_t>*>(sectorPedLists + (y * 36 + x) * 0x14);
            while (node) {
                uint32_t value = *node;
                CEntity* ped = GetEntityFromNode(value & 0xF, (value >> 4) & 0x3FFF);
                node = (value >> 18) == 0x3FFF ? nullptr : &ptrNodes[value >> 18];
                CVector c = centre;
                CVector d;
                VectorSub(&d, &Position(ped), &c);
                float distSq = float((double(d.x) * d.x + double(d.y) * d.y) + double(d.z) * d.z);
                float dist = float(std::sqrt(double(std::fabs(distSq))));
                radius = At<float>(sphere, 0xC);
                if (!(radius + margin > dist)) {
                    continue;
                }
                if (!(radius * 1.100000023841858 > std::fabs(d.z))) {
                    continue;
                }
                using DamageFn = void(__thiscall*)(CEntity*, float, float, int, int, int, int, int, int, int, int,
                                                   float, float, int, int, int, int, float, int);
                DamageFn damage = re::PtrCast<DamageFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[15]);
                damage(ped, At<float>(info, 0x1C), self->pedDamage, At<int32_t>(info, 4), At<int32_t>(info, 0), 0, 0,
                       0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0, 0, 0.0f, 3);
            }
        }
    }
}

void __fastcall ProcessCollision(CProjectile* self, re::Edx, int arg) {
    int model = self->modelIndex;
    if ((model == modelBF4328 || model == modelBF4334) &&
        (self->unknown28 || At<int32_t>(self, 0xAC) || self->unknownB0)) {
        Fn_52ABE0(&Position(self), model, self);
    }
    uint8_t* info = GetProjectileInstance(self->infoIndex);
    if (info[0x17] && info[0x18]) {
        Projectile_DamagePedsInPath(self, info);
    }
    Object_ProcessCollision(self, arg);
}

void __fastcall ApplyThrust(CProjectile* self, re::Edx) {
    uint8_t* info = GetProjectileInstance(self->infoIndex);
    uint32_t now = timeInMs;
    uint32_t start = At<uint32_t>(info, 8);
    uint32_t duration = At<uint32_t>(info, 0x10);
    if (now >= duration + start) {
        return;
    }
    float ts = timeStep;
    uint8_t* weapon = At<uint8_t*>(info, 0x30);
    float fade = 0.0f;
    uint32_t full = uint32_t(int(double(At<float>(weapon, 0x100)) * UnsignedToDouble(duration)));
    if (now > start + full) {
        fade = float(UnsignedToDouble(now - start - full) / int(duration - full));
        if (fade > 1.0f || fade == 1.0f) {
            return;
        }
    }
    float thrust = At<float>(weapon, 0xF8);
    CVector step = {self->moveSpeed.x * ts, self->moveSpeed.y * ts, self->moveSpeed.z * ts};
    CVector push = {step.x * thrust, step.y * thrust, step.z * thrust};
    float left = float(1.0 - fade);
    CVector add = {push.x * left, push.y * left, left * push.z};
    self->moveSpeed.x = self->moveSpeed.x + add.x;
    self->moveSpeed.y = self->moveSpeed.y + add.y;
    self->moveSpeed.z = self->moveSpeed.z + add.z;
    float lift = float((1.0 - fade) * (double(ts) * At<float>(weapon, 0xFC)));
    self->moveSpeed.x = self->moveSpeed.x + 0.0f;
    self->moveSpeed.y = self->moveSpeed.y + 0.0f;
    self->moveSpeed.z = self->moveSpeed.z + lift;
}

CProjectile* __fastcall Construct(CProjectile* self, re::Edx, int model, int infoIndex) {
    Object_Construct(self);
    self->vtable = static_cast<re::Ptr32<void>*>(projectileVtable);
    self->sound = 0;
    At<float>(self, 0x158) = 1.0f;   // mass
    At<float>(self, 0x15C) = 1.0f;   // turn mass
    self->infoIndex = infoIndex;
    self->unknown160 = 0.99999f;
    At<int32_t>(self, 0xC4) = 1;
    self->unknown22C = 0;
    self->unknown164 = 0.75f;
    At<float>(self, 0x168) = 0.0008f;
    self->pedDamage = 0.0f;
    self->unknown230 = 1.0f;
    Sound_Set(&self->sound, 0);
    self->unknown23C &= ~7u;
    self->unknown22D &= 0xFE;
    self->unknown240 &= 0xFE;
    self->unknown234 = 0;
    if (CallVirtual<int32_t>(modelInfoPtrs[model], 7) != -1) {
        void* memory = Malloc(0x104);
        if (memory) {
            AnimData_Construct(memory);
        }
        self->animData = static_cast<uint8_t*>(memory);
    }
    At<int32_t>(self, 0x44) = 1;
    Entity_SetModelIndex(self, model, 1);
    self->objectFlags &= ~0x20;
    Sound_Set(&At<uint32_t>(self, 0x20C), 0);
    uint8_t* objectInfo = GetObjectInfo(self->modelIndex);
    if (objectInfo) {
        uint16_t id = At<uint16_t>(objectInfo, 0);
        self->flags1BC &= ~2;
        self->unknown220 = int16_t(id & 0x7FFF);
    }
    self->objectType = 2;
    return self;
}

} // namespace c11projectile

// ---- registration ---------------------------------------------------------------------

#define PROJENT_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

PROJENT_REVERSED("CProjectile::Destruct", 0x004D15E0, &c11projectile::Destruct);
PROJENT_REVERSED("CProjectile::ApplyGravity", 0x004D1660, &c11projectile::ApplyGravity);
PROJENT_REVERSED("CProjectile::SetIsStatic", 0x004D16B0, &c11projectile::SetIsStatic);
PROJENT_REVERSED("CProjectile::DamagePedsInPath", 0x004D17C0, &c11projectile::DamagePedsInPath);
PROJENT_REVERSED("CProjectile::ProcessCollision", 0x004D1A30, &c11projectile::ProcessCollision);
PROJENT_REVERSED("CProjectile::ApplyThrust", 0x004D2010, &c11projectile::ApplyThrust);
PROJENT_REVERSED("CProjectile::Construct_4D22B0", 0x004D22B0, &c11projectile::Construct);
