// CPed collision (0x47F0A0 .. 0x48012E): its collision with one entity (vf41: the col
// models tested, pushed out, sliding and bouncing, the ground normal kept, the player's
// ground contact), a sphere tested against everything near it, and the ped pointer check.

#include <cmath>

#include "game/Ped.h"
#include "game/Pool.h"
#include "reversed.h"

namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;
ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

constexpr int PED_TYPE_PLAYER = 0xD;

GAME_VAR(CPed*, PC_player, 0x00C1AEA8);
GAME_VAR(CPool<CPed>*, PC_pedPool, 0x00C0F5F0);
GAME_VAR(uint16_t, PC_scanCode, 0x00C1AE88);          // CWorld::ms_nCurrentScanCode
GAME_VAR(float, PC_fltMax, 0x009009B8);
GAME_VAR(uint8_t, PC_C2B109, 0x00C2B109);
GAME_VAR(uint8_t, PC_groundOnly, 0x00C2B10B);         // only the ground under the player is tested
GAME_VAR(uint32_t, PC_onGround, 0x00C2B10C);
GAME_VAR(int32_t, PC_C2B114, 0x00C2B114);
GAME_VAR(float, PC_maxGroundZ, 0x00C2B1A0);           // a normal's z below this isn't ground
GAME_VAR(float, PC_minGroundUp, 0x00C2B1A4);
GAME_VAR(uint8_t*, PC_pedColModel, 0x00CF3124);
GAME_VAR(uint8_t*, PC_feetColModel, 0x00CF3128);
GAME_VAR(float, PC_radius310C, 0x00CF310C);
GAME_VAR(float, PC_radius3110, 0x00CF3110);
GAME_VAR(float, PC_radius311C, 0x00CF311C);
GAME_ARRAY(uint8_t*, PC_modelInfoPtrs, 0x00C67738);
GAME_VAR(double, PC_d_0_025, 0x0090CED8);
GAME_VAR(float, PC_f_0_05, 0x009068B8);
GAME_VAR(float, PC_f_0_5, 0x009055A0);
GAME_VAR(float, PC_f_0_3, 0x00911E30);
GAME_VAR(float, PC_f_0_707, 0x0090D0BC);
GAME_VAR(float, PC_f_0_16, 0x0090D0B8);
GAME_VAR(float, PC_f_minus0_15, 0x00900E84);
GAME_VAR(double, PC_d_200, 0x0090D000);
GAME_VAR(double, PC_d_0_1, 0x00900128);
GAME_VAR(float, PC_f_0, 0x008FF1F0);
GAME_VAR(double, PC_d_1_2, 0x0090D0B0);
GAME_VAR(float, PC_f_1_2, 0x00900DF8);
GAME_VAR(float, PC_f_0_19, 0x0090D0AC);
GAME_VAR(float, PC_f_1_1, 0x0090CEC8);
GAME_VAR(float, PC_f_7, 0x00900E14);
GAME_VAR(float, PC_f_0_35, 0x0090D0A8);
GAME_VAR(double, PC_d_0_2, 0x00900DB8);
GAME_VAR(float, PC_f_0_0005, 0x0090D0A4);
GAME_VAR(float, PC_f_0_999, 0x0090D0A0);
GAME_VAR(double, PC_d_0_5, 0x008FF1F8);
GAME_VAR(ped::CVector, PC_up, 0x009E33DC);
inline void* const PC_C2B150 = reinterpret_cast<void*>(0x00C2B150);
inline void* const PC_A3393C = reinterpret_cast<void*>(0x00A3393C);

GAME_FN(void*, __thiscall, PC_Fn8B6120, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, PC_MatrixCopy, 0x00412A10, void* self, const void* from);
GAME_FN(void, __thiscall, PC_MatrixFn413600_x64, 0x00413600, void* matrix, float x, float y, float z, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void PC_MatrixFn413600(void* a0, float a1, float a2, float a3, uint32_t a4) {
    return PC_MatrixFn413600_x64(a0, a1, a2, a3, re::FloatOf(uint32_t(a4)));
}
GAME_FN(void*, __thiscall, PC_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(void*, __cdecl, PC_GetColModel, 0x0051AF50, void* entity);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline ped::CVector* PC_TransformPoint(ped::CVector* out, const void* matrix, const ped::CVector* v) { return re::CallVec16<0x0045D500, ped::CVector*>(out, matrix, v); }
GAME_FN(uint8_t, __cdecl, PC_ProcessColModels, 0x0056C8C0, const void* matrixA, const void* matrixA2, void* colModelA,
        const void* matrixB, const void* matrixB2, void* colModelB, void* colPoints, int a, int b, int c, int d, int e,
        int flags, void* out, int f);
GAME_FN(ped::CVector*, __cdecl, PC_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PC_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PC_VectorScale, 0x00412350, ped::CVector* out, const ped::CVector* in, float s);
GAME_FN(ped::CVector*, __cdecl, PC_VectorScale2, 0x00412320, ped::CVector* out, float s, const ped::CVector* in);
GAME_FN(float, __thiscall, PC_VectorNormalise, 0x00414E10, ped::CVector* v);
GAME_FN(float, __cdecl, PC_DotProduct, 0x00414640, const ped::CVector* a, const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PC_NormalisedCopy, 0x00472D40, ped::CVector* out, const ped::CVector* in);
GAME_FN(ped::CVector*, __cdecl, PC_Normalised, 0x00478820, ped::CVector* out, const ped::CVector* in);
GAME_FN(ped::CVector*, __cdecl, PC_CrossProduct, 0x0041A570, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(float, __cdecl, PC_Clamp, 0x0041A470, float v, float min, float max);
GAME_FN(float, __cdecl, PC_Max414490, 0x00414490, float a, float b);
GAME_FN(void, __thiscall, PC_SetPositionV, 0x0041EE20, CPed* ped, const ped::CVector* pos);
GAME_FN(void, __thiscall, PC_SetMoveSpeed, 0x0041A440, CPed* ped, const ped::CVector* speed);
GAME_FN(uint8_t, __thiscall, PC_Has1470OrWeapon4330, 0x00473E10, CPed* ped);
GAME_FN(void, __thiscall, PC_BikeFn4C0EC0, 0x004C0EC0, void* bike);
GAME_FN(void, __thiscall, PC_PedTouch, 0x0047CAD0, CPed* ped, CPed* other);
GAME_FN(void, __cdecl, PC_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, PC_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, PC_PhysicalFn46B870, 0x0046B870, CPed* ped, void* entity, void* colPoint, float a);
GAME_FN(uint8_t, __cdecl, PC_Fn683A30, 0x00683A30);
GAME_FN(uint8_t, __thiscall, PC_EntityFn45D3C0, 0x0045D3C0, void* entity);
GAME_FN(ped::CVector*, __thiscall, PC_GetForward, 0x004154A0, void* entity, ped::CVector* out);
GAME_FN(float, __thiscall, PC_VectorMagnitude2D, 0x004246B0, const ped::CVector* v);
GAME_FN(float, __thiscall, PC_VectorMagnitude, 0x0040D9F0, const ped::CVector* v);
GAME_FN(void, __thiscall, PC_ColPointSetEntity, 0x0045F1F0, void* colPoint, void* entity);
GAME_FN(void, __thiscall, PC_ColPointAssign, 0x0046AA70, void* colPoint, const void* from);
GAME_FN(void, __cdecl, PC_Rumble, 0x00738680, int pad, int duration, int strength, int a);
GAME_FN(void, __thiscall, PC_PedPushOut, 0x0047A700, CPed* ped, void* colPoint, void* matrix);
GAME_FN(void, __thiscall, PC_PhysicalBounce, 0x0046B9D0, CPed* ped, void* colPoint, float* out, float a, float b,
        const ped::CVector* dir);
GAME_FN(void, __thiscall, PC_MatrixFromRight, 0x004129D0, void* matrix, const ped::CVector* right);
GAME_FN(void, __cdecl, PC_Fn569DC0, 0x00569DC0, const void* matrixA, void* colModelA, const void* matrixB,
        void* colModelB, void* a, void* b);
GAME_FN(void, __cdecl, PC_ClearScanCodes, 0x0045D840);
GAME_FN(uint8_t, __thiscall, PC_TestSphereNearby, 0x0047ACC0, CPed* ped, int a, const void* centre, float radius,
        void* out, int b);
GAME_FN(uint8_t, __thiscall, PC_TestSphereSectors, 0x0047B0B0, CPed* ped, int a, const void* centre, float radius,
        void* out, int b);
GAME_FN(uint8_t, __thiscall, PC_TestSphereList, 0x0046C600, CPed* ped, uintptr_t a, void* list, void* colPoint, int b);
GAME_FN(uint8_t, __cdecl, PC_IsEntityPointerValid, 0x004657F0, void* entity);

// A col point (0x28 bytes, +0xC a registered entity, +0x10 the normal, +0x24 the depth)
void ColPointInit(uint8_t* cp) {
    At<ped::CVector>(cp, 0) = {0.0f, 0.0f, 0.0f};
    At<void*>(cp, 0xC) = nullptr;
    At<ped::CVector>(cp, 0x10) = {0.0f, 0.0f, 0.0f};
    At<float>(cp, 0x24) = PC_fltMax;
}
void ColPointRelease(uint8_t* cp) {
    re::Ptr32<void>& entity = At<void*>(cp, 0xC);
    if (entity) {
        PC_CleanUpOldReference(entity, &entity);
    }
}

// Rumble strength: a byte of `v` times 25 (8-bit signed multiply, as the game does)
int RumbleStrength(double v) {
    int8_t low = static_cast<int8_t>(static_cast<int>(v));
    return static_cast<uint16_t>(static_cast<int16_t>(low * static_cast<int8_t>(0x19)));
}

// vf41 0x47F0A0: its collision with an entity (`matrix` where it's tested); `noResponse`
// only finds the contact (and the ground flags +0x130C/+0x130D)
uint8_t __fastcall CPed_ProcessEntityCollision(CPed* self, re::Edx, uint8_t* matrix, uint8_t* entity, uint8_t* colPoint,
                                               int noResponse) {
    // (several fourth floats the game writes come from uninitialised stack; ours are 0)
    uint8_t result = 0;
    At<int32_t>(self, 0x24) = 1;
    alignas(4) uint8_t frame[0x44];
    void* frameSrc = PC_Fn8B6120(self);
    PC_MatrixCopy(frame, frameSrc);
    PC_MatrixFn413600(frame, 0.0f, 0.0f, 1.0f, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(frameSrc)));
    alignas(4) uint8_t entityMatrix[0x44];
    PC_MatrixCopy(entityMatrix, PC_GetMatrix(entity));
    bool special = self->pedType == PED_TYPE_PLAYER &&
                   (PC_C2B109 != 0 || ((entity[0x108] & 7) == 3 && At<int32_t>(entity, 0x13F4) == 0));
    void* entityColModel;
    if (At<int32_t>(self, 0x13F4) == 0 && (entity[0x108] & 7) == 3 && At<int32_t>(entity, 0x1310) == PED_TYPE_PLAYER) {
        entityColModel = PC_pedColModel;
    } else {
        entityColModel = PC_GetColModel(entity);
    }
    if (PC_groundOnly) {
        uint8_t type = entity[0x108] & 7;
        if (type == 3 || type == 7) {
            return 0;
        }
        ped::CVector feet = {At<float>(self, 0x160C), At<float>(self, 0x1610), At<float>(self, 0x1614)};
        ped::CVector at = At<ped::CVector>(matrix, 0x30);
        ped::CVector w;
        PC_TransformPoint(&w, matrix, &feet);
        w.z = static_cast<float>(at.z - PC_d_0_025);
        alignas(4) uint8_t cp[0x28] = {};
        ColPointInit(cp);
        alignas(4) uint8_t feetMatrix[0x44];
        PC_MatrixCopy(feetMatrix, frame);
        At<ped::CVector>(feetMatrix, 0x30) = w;
        At<float>(feetMatrix, 0x3C) = 0.0f;
        uint8_t* spheres = *At<re::Ptr32<uint8_t>*>(PC_feetColModel, 0x38);
        At<float>(spheres, 0x0) = 0.0f;
        At<float>(spheres, 0x4) = 0.0f;
        At<float>(spheres, 0x8) = At<float>(self, 0x1604);
        At<float>(spheres, 0x14) = 0.0f;
        At<float>(spheres, 0x18) = 0.0f;
        At<float>(spheres, 0x1C) = At<float>(self, 0x1614);
        At<float>(spheres, 0xC) = At<float>(self, 0x1608);
        At<float>(*At<re::Ptr32<uint8_t>*>(PC_feetColModel, 0x38), 0x20) = At<float>(self, 0x1618);
        if (PC_ProcessColModels(matrix, feetMatrix, PC_feetColModel, entityMatrix, entityMatrix, entityColModel, cp, 0,
                                0, 0, 0, 1, 0xF, nullptr, 0)) {
            ped::CVector n = At<ped::CVector>(cp, 0x10);
            if (PC_maxGroundZ > std::fabs(n.z)) {
                result = 1;
                ped::CVector d;
                ped::CVector t = w;
                PC_VectorSub(&d, &t, &at);
                float lift = At<int32_t>(self, 0x131C) != 0 ? PC_f_0_05 : 0.0f;
                float k = 1.0f - lift - At<float>(cp, 0x24);
                ped::CVector s;
                ped::CVector q;
                PC_VectorSub(&q, &at, PC_VectorScale(&s, &d, k));
                PC_SetPositionV(self, &q);
                ped::CVector& ground = At<ped::CVector>(self, 0x1898);
                if (At<uint8_t>(self, 0x18A5) == 0) {
                    ground = n;
                } else {
                    ped::CVector sum;
                    ped::CVector half;
                    ped::CVector* h = PC_VectorScale(&half, PC_VectorAdd(&sum, &ground, &n), PC_f_0_5);
                    ground = *h;
                    PC_VectorNormalise(&ground);
                }
                At<uint8_t>(self, 0x18A5) = 1;
                if (self->pedType == PED_TYPE_PLAYER && At<int32_t>(self, 0x131C) != 0 && !PC_Has1470OrWeapon4330(self)) {
                    PC_onGround = 1;
                }
            }
        }
        ColPointRelease(cp);
        return result;
    }
    uint8_t notPlayerOrNoResponse = (re::Flag(noResponse) || self->pedType != PED_TYPE_PLAYER) ? 1 : 0;
    void* selfColModel = special ? static_cast<void*>(PC_pedColModel)
                                 : static_cast<void*>(At<void*>(PC_modelInfoPtrs[self->modelIndex], 0xC));
    if (entity && (entity[0x108] & 7) == 2 && At<int32_t>(entity, 0x46C) == 1) {
        PC_BikeFn4C0EC0(entity);
    }
    struct {
        uint8_t flag0, flag1, flag2, pad;
        ped::CVector v;
    } contact = {0, 0, 0, 0, {0.0f, 0.0f, 0.0f}};
    uint8_t solid = reinterpret_cast<uint8_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[0x14 / 4]))(entity);
    int kind = (solid && self->vehicle == nullptr) ? 10 : 15;
    uint8_t hit = PC_ProcessColModels(matrix, frame, selfColModel, entityMatrix, entityMatrix, entityColModel, colPoint,
                                      notPlayerOrNoResponse, 0, 0, 0, 1, kind, &contact, 1);
    if (hit) {
        result = 1;
        if ((entity[0x108] & 7) == 3) {
            PC_PedTouch(self, reinterpret_cast<CPed*>(entity));
        }
        re::Ptr32<void>& hitEntity = At<void*>(colPoint, 0xC);
        if (entity != hitEntity) {
            if (hitEntity) {
                PC_CleanUpOldReference(hitEntity, &hitEntity);
            }
            hitEntity = entity;
            PC_RegisterReference(entity, &hitEntity);
        }
        PC_PhysicalFn46B870(self, entity, colPoint, 0.0f);
        ped::CVector n = At<ped::CVector>(colPoint, 0x10);
        if (re::Flag(noResponse)) {
            if (contact.flag2) {
                At<uint8_t>(self, 0x130D) = PC_minGroundUp > contact.v.z ? 1 : 0;
                At<uint8_t>(self, 0x130C) = (contact.flag0 == 0 && contact.flag1 == 0) ? 1 : 0;
            }
        } else {
            // a wall: kept (+0x1864) to slide along
            if (!PC_Fn683A30() &&
                !reinterpret_cast<uint8_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[0x7C / 4]))(entity) &&
                ((entity[0x108] & 7) == 1 || PC_EntityFn45D3C0(entity)) && std::fabs(n.z) < PC_f_0_3) {
                At<int32_t>(self, 0x7C) = 1;
                if (At<int32_t>(self, 0x13A8) == 0 && At<int32_t>(self, 0x13AC) == 0) {
                    ped::CVector forward;
                    PC_GetForward(self, &forward);
                    ped::CVector flat = {n.x, n.y, 0.0f};
                    PC_VectorNormalise(&flat);
                    float facing = std::fabs(PC_DotProduct(&flat, &forward));
                    if (facing > PC_f_0_707) {
                        ped::CVector* speed = &At<ped::CVector>(self, 0x124);
                        if (At<int32_t>(self, 0x1474) != 0) {
                            if (PC_VectorMagnitude2D(speed) >= PC_f_0_16 && PC_DotProduct(speed, &flat) < PC_f_minus0_15) {
                                At<ped::CVector>(self, 0x1864) = At<ped::CVector>(colPoint, 0);
                                At<ped::CVector>(self, 0x1874) = n;
                                PC_ColPointSetEntity(Bytes(self) + 0x1864, entity);
                                At<int32_t>(self, 0x13A8) = 1;
                            }
                            if (self->pedType == PED_TYPE_PLAYER) {
                                PC_Rumble(0, 0x4B, RumbleStrength(PC_VectorMagnitude(speed) * PC_d_200), 1);
                            }
                        } else {
                            At<ped::CVector>(self, 0x1864) = At<ped::CVector>(colPoint, 0);
                            At<ped::CVector>(self, 0x1874) = n;
                            PC_ColPointSetEntity(Bytes(self) + 0x1864, entity);
                            At<int32_t>(self, 0x13A8) = 1;
                        }
                    }
                }
            }
            // pushed out (a held car pulls it along instead)
            int32_t sliding = At<int32_t>(self, 0x1474);
            if (sliding != 0 && At<int32_t>(self, 0x148C) != 0 && At<uint8_t*>(self, 0x16C8) == entity) {
                ped::CVector at = At<ped::CVector>(matrix, 0x30);
                ped::CVector d;
                PC_VectorSub(&d, PosOf(self), &at);
                float k = static_cast<float>(At<float>(colPoint, 0x24) - PC_d_0_1);
                float r = PC_Max414490(0.0f, k);
                ped::CVector s;
                PC_VectorScale(&s, &d, r);
                ped::CVector p;
                PC_SetPositionV(self, PC_VectorAdd(&p, &at, &s));
            } else if (sliding == 0 || 0.0f < At<float>(colPoint, 0x24)) {
                PC_PedPushOut(self, colPoint, matrix);
            }
            // bounced off while sliding
            if (At<int32_t>(self, 0x1474) != 0 &&
                !(At<int32_t>(self, 0x148C) != 0 && At<uint8_t*>(self, 0x16C8) == entity)) {
                ped::CVector push = {0.0f, 0.0f, 0.0f};
                ped::CVector* speed = &At<ped::CVector>(self, 0x124);
                ped::CVector tmp;
                if (PC_DotProduct(speed, PC_GetForward(self, &tmp)) > PC_f_0) {
                    ped::CVector* f = PC_GetForward(self, &tmp);
                    ped::CVector back = {-f->x, -f->y, -f->z};
                    push.x = static_cast<float>(back.x * PC_d_1_2);
                    push.y = static_cast<float>(back.y * PC_d_1_2);
                    push.z = static_cast<float>(PC_d_1_2 * back.z);
                } else {
                    ped::CVector s;
                    push = *PC_VectorScale(&s, PC_GetForward(self, &tmp), PC_f_1_2);
                }
                push.z = At<float>(colPoint, 8) - PosOf(self)->z;
                float out = 0.0f;  // (an uninitialised local in the game)
                if (At<int32_t>(self, 0x13A8) != 0 && PC_VectorMagnitude2D(speed) <= PC_f_0_19) {
                    PC_PhysicalBounce(self, colPoint, &out, PC_f_1_1, 1.0f, &push);
                } else {
                    PC_PhysicalBounce(self, colPoint, &out, PC_f_0_35, PC_f_7, &push);
                    if (std::fabs(At<float>(colPoint, 0x18)) < PC_d_0_2 && PC_VectorMagnitude(speed) > PC_f_0_0005) {
                        ped::CVector flat = {speed->x, speed->y, 0.0f};
                        ped::CVector u;
                        PC_NormalisedCopy(&u, &flat);
                        ped::CVector away = {-u.x, -u.y, -u.z};
                        float amount = PC_Clamp(PC_DotProduct(&away, &At<ped::CVector>(colPoint, 0x10)), 0.0f, PC_f_0_999);
                        ped::CVector a;
                        ped::CVector b;
                        ped::CVector c;
                        PC_VectorScale(&b, PC_VectorScale2(&a, amount, &flat), PC_f_0_3);
                        PC_SetMoveSpeed(self, PC_VectorSub(&c, speed, &b));
                    }
                }
            }
            // the ground normal (+0x188C) kept, averaged
            ped::CVector& ground = At<ped::CVector>(self, 0x188C);
            if (At<uint8_t>(self, 0x18A4) == 0) {
                ground = n;
            } else {
                ped::CVector sum;
                ped::CVector* s = PC_VectorAdd(&sum, &ground, &n);
                float x = static_cast<float>(s->x * PC_d_0_5);
                float y = static_cast<float>(s->y * PC_d_0_5);
                float z = static_cast<float>(PC_d_0_5 * s->z);
                ground = {x, y, z};
                PC_VectorNormalise(&ground);
            }
            At<uint8_t>(self, 0x18A4) = 1;
            if (self->pedType != PED_TYPE_PLAYER) {
                return result;
            }
            if (PC_C2B109 == 0 && !PC_Has1470OrWeapon4330(self)) {
                PC_onGround = 1;
                if (PC_maxGroundZ > n.z) {
                    PC_C2B114 = PC_C2B114 + 1;
                    if (PC_C2B114 == -1) {
                        PC_C2B114 = 1;
                    }
                }
            }
        }
    }
    // the player standing on something: its col model tested against the entity's
    uint8_t type = entity[0x108] & 7;
    if (self->pedType != PED_TYPE_PLAYER || re::Flag(noResponse) || PC_C2B109 != 0 || PC_Has1470OrWeapon4330(self) ||
        PC_onGround == 0 || type == 3 || type == 2 || type == 7) {
        return result;
    }
    uint8_t* pedColModel = PC_pedColModel;
    uint8_t* data = At<uint8_t*>(pedColModel, 0x38);
    data[0x20] = 2;
    if (hit) {
        float r;
        int32_t which = At<int32_t>(colPoint, 0x1C);
        if (which == 1) {
            r = PC_radius3110;
        } else if (which == 2) {
            r = PC_radius310C;
        } else {
            r = PC_radius311C;
        }
        uint8_t* shapes = At<uint8_t*>(data, 4);
        At<float>(shapes, 0x18) = r;
        At<float>(shapes, 0x08) = r;
        At<float>(shapes, 0x38) = r;
        At<float>(shapes, 0x28) = r;
    }
    ped::CVector flat = {At<float>(self, 0x124), At<float>(self, 0x128), 0.0f};
    ped::CVector dirN;
    ped::CVector* dir = PC_Normalised(&dirN, &flat);
    ped::CVector d = *dir;
    ped::CVector up = PC_up;
    ped::CVector right;
    PC_CrossProduct(&right, &d, &up);
    alignas(4) uint8_t move[0x44] = {};
    At<ped::CVector>(move, 0x10) = d;
    PC_MatrixFromRight(move, &right);
    At<ped::CVector>(move, 0x20) = up;
    At<ped::CVector>(move, 0x30) = At<ped::CVector>(matrix, 0x30);
    At<float>(move, 0x3C) = At<float>(matrix, 0x3C);
    PC_Fn569DC0(move, pedColModel, entityMatrix, entityColModel, PC_C2B150, PC_A3393C);
    data[0x20] = 0;
    return result;
}

// 0x47FF00 (thiscall): a sphere against the nearby entities and its sectors (with its
// own scan code), or against its sectors' lists (2, or 4 without `nearbyOnly`), when there
// are too many nearby; the nearest hit into `out`
uint8_t __fastcall CPed_TestSphere(CPed* self, re::Edx, int a, const void* centre, float radius, uint8_t* out,
                                   int nearbyOnly) {
    alignas(4) uint8_t cp[0x28] = {};
    ColPointInit(cp);
    uint8_t found = 0;
    if (PC_scanCode < 0xFFFF) {
        PC_scanCode = static_cast<uint16_t>(PC_scanCode + 1);
    } else {
        PC_ClearScanCodes();
        PC_scanCode = 1;
    }
    if (At<uint8_t>(self, 0x15F8) == 0) {
        if (PC_TestSphereNearby(self, a, centre, radius, cp, 1)) {
            found = 1;
            if (At<float>(out, 0x24) > At<float>(cp, 0x24)) {
                PC_ColPointAssign(out, cp);
            }
        }
        if (static_cast<uint8_t>(nearbyOnly) == 0 && PC_TestSphereSectors(self, a, centre, radius, cp, 1)) {
            found = 1;
            if (At<float>(out, 0x24) > At<float>(cp, 0x24)) {
                PC_ColPointAssign(out, cp);
            }
        }
    } else {
        int lists = static_cast<uint8_t>(nearbyOnly) == 0 ? 4 : 2;
        for (uint8_t* node = At<uint8_t*>(self, 0x178); node; node = At<uint8_t*>(node, 0x10)) {
            At<float>(cp, 0x24) = PC_fltMax;
            uint8_t* list = At<uint8_t*>(node, 8);
            for (int k = 0; k < lists; k++) {
                if (PC_TestSphereList(self, a, list + k * 4, cp, 1)) {
                    found = 1;
                    if (At<float>(out, 0x24) > At<float>(cp, 0x24)) {
                        PC_ColPointAssign(out, cp);
                    }
                }
            }
        }
    }
    ColPointRelease(cp);
    return found;
}

// 0x4800D0 (cdecl): a ped pointer still good: one of the first 25 slots, in the world
// (or the player); one in a vehicle is as good as its vehicle
uint8_t __cdecl IsPedPointerValid(CPed* ped) {
    if (!ped) {
        return 0;
    }
    int slot = PC_pedPool->GetIndex(ped);
    int handle = PC_pedPool->flags[slot] + (slot << 8);
    if (static_cast<uint32_t>(handle >> 8) > 0x18) {
        return 0;
    }
    if (ped->vehicle) {
        return PC_IsEntityPointerValid(ped->vehicle);
    }
    if (At<void*>(ped, 0x178) == nullptr && ped != PC_player) {
        return 0;
    }
    return 1;
}

#define PEDCOLLIDE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDCOLLIDE_REVERSED("CPed::ProcessEntityCollision", 0x0047F0A0, &CPed_ProcessEntityCollision);
PEDCOLLIDE_REVERSED("CPed::TestSphere", 0x0047FF00, &CPed_TestSphere);
PEDCOLLIDE_REVERSED("IsPedPointerValid", 0x004800D0, &IsPedPointerValid);

} // namespace
