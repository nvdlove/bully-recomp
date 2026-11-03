// CPed collision processing (0x482070): each frame, the ped moved (or skitched), its
// collision sphere tested against what's near it until it's clear (up to 4 times, back
// to its last good place when it can't be), then its ground found (stepping down onto
// it, landing, sliding off steep ground), its Gamebryo transform updated, and for the
// player the ground-contact tallies kept.

#include <cmath>

#include "game/Ped.h"
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

GAME_VAR(uint32_t, PX_timeInMs, 0x00C1A9B4);
GAME_VAR(uint16_t, PX_scanCode, 0x00C1AE88);          // CWorld::ms_nCurrentScanCode
GAME_VAR(float, PX_fltMax, 0x009009B8);
GAME_VAR(uint8_t, PX_C2B109, 0x00C2B109);
GAME_VAR(uint8_t, PX_C2B10A, 0x00C2B10A);
GAME_VAR(uint8_t, PX_groundOnly, 0x00C2B10B);
GAME_VAR(uint32_t, PX_onGround, 0x00C2B10C);           // ground contacts in a row
GAME_VAR(uint32_t, PX_slopeContacts, 0x00C2B110);
GAME_VAR(int32_t, PX_C2B114, 0x00C2B114);
GAME_VAR(CEntity*, PX_lastPushedBy, 0x00C2B118);
GAME_VAR(float, PX_speedFactor, 0x00C2B1C4);
GAME_ARRAY(float, PX_contactFraction, 0x00A3393C);     // [2]
GAME_VAR(uint32_t, PX_maxSlopeContacts, 0x00A33944);
GAME_VAR(ped::CVector, PX_groundNormal, 0x00C2B144);
inline uint8_t* const PX_contacts = reinterpret_cast<uint8_t*>(0x00C2B160);  // 2, 0x28 apart: their normals
GAME_VAR(float, PX_maxGroundZ, 0x00C2B1A0);
GAME_VAR(float, PX_minGroundUpPlayer, 0x00C2B1A8);
GAME_VAR(ped::CVector, PX_zeroVector, 0x00BCC564);
GAME_VAR(re::Ptr32<uint8_t>*, PX_CF315C, 0x00CF315C);           // the player's col model (+0x1E, [0]+0x48)
GAME_VAR(float, PX_CF3114, 0x00CF3114);
GAME_VAR(float, PX_CF3118, 0x00CF3118);
GAME_VAR(float, PX_pedRadius, 0x00CF311C);
GAME_VAR(uint8_t*, PX_feetColModel, 0x00CF3128);
GAME_VAR(int32_t, PX_weapon4418, 0x00BF4418);
GAME_VAR(int32_t, PX_weapon4330, 0x00BF4330);
GAME_VAR(int32_t, PX_C3CCF4, 0x00C3CCF4);
GAME_VAR(int32_t, PX_C3CCF8, 0x00C3CCF8);
GAME_VAR(float, PX_lodScale, 0x00C3CD00);
GAME_VAR(float, PX_f_0_35, 0x0090D0A8);
GAME_VAR(float, PX_f_0_55, 0x0090D1D8);
GAME_VAR(double, PX_d_0_15, 0x009051E0);
GAME_VAR(float, PX_f_0_7, 0x00900E00);
GAME_VAR(double, PX_d_0_1, 0x00900128);
GAME_VAR(double, PX_d_60, 0x0090CE08);
GAME_VAR(double, PX_d_0_5, 0x008FF1F8);
GAME_VAR(double, PX_d_0_05, 0x00900C18);
GAME_VAR(double, PX_d_1_1, 0x0090CEE0);
GAME_VAR(double, PX_d_0_005, 0x0090C740);
GAME_VAR(float, PX_f_minus0_3, 0x0090CF08);
GAME_VAR(double, PX_d_minus0, 0x0090D1D0);
GAME_VAR(double, PX_d_0_3, 0x00900D78);
GAME_VAR(float, PX_f_minus0_707, 0x00900D48);
GAME_VAR(float, PX_f_minus0_15, 0x00900E84);
GAME_VAR(double, PX_d_0_25, 0x00900498);
GAME_VAR(float, PX_f_1e5, 0x008FF230);
inline void* const PX_C3B808 = reinterpret_cast<void*>(0x00C3B808);
inline void* const PX_camera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(uint8_t, __thiscall, PX_IsInVehicleSeat, 0x004CE690, CPed* ped);
GAME_FN(int, __cdecl, PX_Fn6CA7C0, 0x006CA7C0);
GAME_FN(void, __cdecl, PX_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, PX_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, PX_Fn4E9DF0, 0x004E9DF0, void* self, int a);
GAME_FN(void, __thiscall, PX_Fn4E9EF0, 0x004E9EF0, void* self, int a);
GAME_FN(float, __thiscall, PX_VectorMagnitude, 0x0040DFB0, const ped::CVector* v);
GAME_FN(void*, __thiscall, PX_Fn8B6120, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, PX_MatrixCopy, 0x00412A10, void* self, const void* from);
GAME_FN(void, __thiscall, PX_MatrixAssign, 0x00412720, void* self, const void* from);
GAME_FN(void, __thiscall, PX_MatrixFn413600_x64, 0x00413600, void* matrix, float x, float y, float z, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void PX_MatrixFn413600(void* a0, float a1, float a2, float a3, uint32_t a4) {
    return PX_MatrixFn413600_x64(a0, a1, a2, a3, re::FloatOf(uint32_t(a4)));
}
GAME_FN(void, __thiscall, PX_MatrixFn413A80, 0x00413A80, void* matrix);
GAME_FN(void, __thiscall, PX_MatrixFn413A70, 0x00413A70, void* matrix);
GAME_FN(void, __thiscall, PX_PedUpdateSkitching, 0x0047DB40, CPed* ped);
GAME_FN(int, __thiscall, PX_CameraGetFadeState, 0x004F12F0, void* camera);
GAME_FN(void*, __cdecl, PX_Fn6C82F0, 0x006C82F0, uintptr_t a);
GAME_FN(uint8_t*, __cdecl, PX_Fn6C9A90, 0x006C9A90, void* a);
GAME_FN(ped::CVector*, __cdecl, PX_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PX_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(void, __thiscall, PX_ColModelSetSphere, 0x0056EB80, void* colModel, float radius, const ped::CVector* centre);
GAME_FN(void, __thiscall, PX_BoxSet, 0x00564830, void* box, const ped::CVector* min, const ped::CVector* max);
GAME_FN(void, __thiscall, PX_PedUpdateNearbyEntities, 0x0047A9E0, CPed* ped);
GAME_FN(void, __cdecl, PX_ClearScanCodes, 0x0045D840);
GAME_FN(void, __thiscall, PX_GetBoundingSphere, 0x00466CA0, CPed* ped, void* sphere);
GAME_FN(float, __thiscall, PX_Fn465E10, 0x00465E10, CPed* ped);
GAME_FN(uint8_t, __thiscall, PX_TestSphereNearby, 0x0047ACC0, CPed* ped, void* matrix, const void* centre,
        float radius, void* out, int b);
GAME_FN(uint8_t, __thiscall, PX_TestSphereSectors, 0x0047B0B0, CPed* ped, void* matrix, const void* centre,
        float radius, void* out, int b);
GAME_FN(uint8_t, __thiscall, PX_TestSphereList, 0x0046CAC0, CPed* ped, void* matrix, void* list, void* colPoint,
        int a, int b);
GAME_FN(uint8_t, __thiscall, PX_IsSwimming, 0x00476D70, CPed* ped);
GAME_FN(uint8_t, __thiscall, PX_TestSphere, 0x0047FF00, CPed* ped, void* matrix, const void* centre, float radius,
        void* out, int nearbyOnly);
GAME_FN(void, __thiscall, PX_Fn485A80, 0x00485A80, void* self, CPed* attacker);
GAME_FN(void, __thiscall, PX_Fn48BF50, 0x0048BF50, void* self, CPed* attacker);
GAME_FN(void, __thiscall, PX_CameraFn4F2C20, 0x004F2C20, void* camera, int a, int b, int c, int d, int e);
GAME_FN(uint8_t, __cdecl, PX_Fn425700, 0x00425700);
GAME_FN(void, __thiscall, PX_TakeHealth, 0x004783F0, CPed* ped, float amount);
GAME_FN(float, __thiscall, PX_VectorNormalise, 0x00414E10, ped::CVector* v);
GAME_FN(float, __cdecl, PX_DotProduct, 0x00414640, const ped::CVector* a, const ped::CVector* b);
GAME_FN(ped::CVector*, __thiscall, PX_GetRight, 0x004736F0, CPed* ped, ped::CVector* out);
GAME_FN(void, __thiscall, PX_VectorScaleBy, 0x0088A480, ped::CVector* v, float s);
GAME_FN(void, __thiscall, PX_SimpleTransformUpdateNi, 0x0046D8F0, void* transform, void* out);
GAME_FN(uint8_t, __cdecl, PX_NearlyEqual, 0x00432A60, float a, float b, float epsilon);

void ReleaseRef(void* self, uint32_t offset) {
    re::Ptr32<void>& ref = At<void*>(self, offset);
    if (ref) {
        PX_CleanUpOldReference(ref, &ref);
        ref = nullptr;
    }
}
void ColPointInit(uint8_t* cp) {
    At<ped::CVector>(cp, 0) = {0.0f, 0.0f, 0.0f};
    At<void*>(cp, 0xC) = nullptr;
    At<ped::CVector>(cp, 0x10) = {0.0f, 0.0f, 0.0f};
    At<float>(cp, 0x24) = PX_fltMax;
}
void ColPointRelease(uint8_t* cp) {
    re::Ptr32<void>& entity = At<void*>(cp, 0xC);
    if (entity) {
        PX_CleanUpOldReference(entity, &entity);
    }
}
void NextScanCode() {
    if (PX_scanCode < 0xFFFF) {
        PX_scanCode = static_cast<uint16_t>(PX_scanCode + 1);
    } else {
        PX_ClearScanCodes();
        PX_scanCode = 1;
    }
}
// Its position moved (the matrix's fourth float the game writes from uninitialised
// stack; ours leaves it)
void SetPos(CPed* self, float x, float y, float z) {
    if (self->matrix) {
        At<float>(self->matrix, 0x30) = x;
        At<float>(self->matrix, 0x34) = y;
        At<float>(self->matrix, 0x38) = z;
    }
    self->position = {x, y, z};
}
bool IsBumpySurface(uint8_t s) {
    return s == 8 || s == 0x23 || s == 0x2A || s == 0x38 || s == 0x3A;
}

// 0x482070 (thiscall): `inVehicleToo` processes it even when seated in a vehicle;
// `move` applies its movement (or skitching) first
void __fastcall CPed_ProcessCollision(CPed* self, re::Edx, int inVehicleToo, int move) {
    if (!re::Flag(inVehicleToo) && (PX_IsInVehicleSeat(self) || PX_Fn6CA7C0() != 0)) {
        ReleaseRef(self, 0x1758);
        return;
    }
    // the player's col model: its feet, by how fast it's going
    if (self->pedType == PED_TYPE_PLAYER) {
        PX_Fn4E9DF0(PX_C3B808, 1);
        uint8_t* colModel = re::PtrCast<uint8_t*>(PX_CF315C);
        if (PX_C2B109) {
            colModel[0x1E] = 4;
            At<float>(*PX_CF315C, 0x48) = PX_CF3114;
        } else {
            uint32_t sinceHit = PX_timeInMs - At<uint32_t>(self, 0x4CC);
            uint32_t since550 = PX_timeInMs - At<uint32_t>(self, 0x550);
            bool slow;
            if (At<int32_t>(self, 0x151C) != 0) {
                slow = true;
            } else if (PX_C2B10A != 0) {
                slow = false;
            } else {
                slow = since550 < 4000 || sinceHit < 4000 || At<int16_t>(self->weapon, 0x10E) == PX_weapon4418;
            }
            if (slow) {
                float limit = At<int32_t>(self, 0x131C) != 0 ? PX_f_0_35 : PX_f_0_55;
                float speed = std::fabs(PX_VectorMagnitude(&At<ped::CVector>(self, 0x124)));
                float r = static_cast<float>(speed / PX_d_0_15);
                if (r < 0.0f) {
                    r = 0.0f;
                } else if (r > PX_f_0_7) {
                    r = 1.0f;
                }
                PX_speedFactor = static_cast<float>((r - PX_speedFactor) * PX_d_0_1 + PX_speedFactor);
                if (!(PX_speedFactor <= limit)) {  // (NaN: 3)
                    colModel[0x1E] = 3;
                } else {
                    colModel[0x1E] = 4;
                    At<float>(*PX_CF315C, 0x48) = PX_CF3118;
                }
            } else {
                colModel[0x1E] = 3;
            }
        }
        PX_C2B114 = 0;
        PX_contactFraction[0] = 1.0f;
        PX_contactFraction[1] = 1.0f;
    } else {
        PX_Fn4E9EF0(PX_C3B808, 1);
    }
    int count = 0;
    PX_lastPushedBy = nullptr;
    // (the matrices' fourth floats and some col point fields are uninitialised stack in
    // the game; ours keep the copied value or are 0)
    alignas(4) uint8_t frame[0x44];
    void* frameSrc = PX_Fn8B6120(self);
    PX_MatrixCopy(frame, frameSrc);
    PX_MatrixFn413600(frame, 0.0f, 0.0f, 1.0f, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(frameSrc)));
    alignas(4) uint8_t test[0x44];
    PX_MatrixCopy(test, frame);
    ped::CVector& testPos = At<ped::CVector>(test, 0x30);
    testPos = At<ped::CVector>(self, 0x1564);
    if (self->matrix) {
        PX_MatrixFn413A80(self->matrix);
    }
    if (re::Flag(move)) {
        if (At<int32_t>(self, 0x148C) != 0 && At<void*>(self, 0x16C8) != nullptr) {
            PX_PedUpdateSkitching(self);
        } else {
            reinterpret_cast<void(__thiscall*)(CPed*)>(static_cast<void*>(self->vtable[0x94 / 4]))(self);
        }
    }
    ReleaseRef(self, 0x1758);
    ped::CVector moved = *PosOf(self);
    // near the camera: the feet's col model fitted to its feet (+0x15FC .. +0x160C)
    uint8_t near = 0;
    if (self->unknownA4 == 0 && At<int32_t>(self, 0xA8) == 0 &&
        (At<int32_t>(self, 0x13E8) != 0 || (self->pedType == PED_TYPE_PLAYER && At<int32_t>(self, 0x131C) != 0)) &&
        PX_CameraGetFadeState(PX_camera) == 0) {
        uint8_t* cam = PX_Fn6C9A90(PX_Fn6C82F0(PX_C3CCF4));
        ped::CVector camPos = At<ped::CVector>(cam, 0x24);
        ped::CVector d;
        ped::CVector* v = PX_VectorSub(&d, &camPos, &moved);
        float sq = v->x * v->x + v->y * v->y + v->z * v->z;
        double range = PX_lodScale * PX_d_60;
        if (sq < range * range) {
            near = 1;
            ped::CVector span;
            PX_VectorSub(&span, &At<ped::CVector>(self, 0x160C), &At<ped::CVector>(self, 0x15FC));
            ped::CVector half;
            half.x = static_cast<float>(span.x * PX_d_0_5);
            half.y = static_cast<float>(span.y * PX_d_0_5);
            half.z = static_cast<float>(PX_d_0_5 * span.z);
            ped::CVector centre;
            PX_VectorAdd(&centre, &At<ped::CVector>(self, 0x15FC), &half);
            float radius = PX_VectorMagnitude(&half) + PX_pedRadius;
            uint8_t* feet = PX_feetColModel;
            PX_ColModelSetSphere(feet, radius, &centre);
            ped::CVector max = {radius, radius, radius + radius};
            ped::CVector min = {-radius, -radius, 0.0f};
            PX_BoxSet(feet + 0x10, &min, &max);
        }
    }
    float drop = 0.0f;
    PX_PedUpdateNearbyEntities(self);
    if (testPos.z > moved.z) {
        drop = static_cast<float>((moved.z - testPos.z) - PX_d_0_05);
        PosOf(self)->z = static_cast<float>(testPos.z + PX_d_0_05);
    }
    int tries = At<int32_t>(self, 0x13C0) != 0 ? 4 : 0;
    if (At<int32_t>(self, 0x13A8) != 0 && At<int32_t>(self, 0x13AC) == 0) {
        At<int32_t>(self, 0x13A8) = 0;
        At<ped::CVector>(self, 0x1864) = PX_zeroVector;
        At<ped::CVector>(self, 0x1874) = PX_zeroVector;
        ReleaseRef(self, 0x1870);
    }
    // pushed clear of what's around it
    alignas(4) uint8_t cp[0x28] = {};
    ColPointInit(cp);
    for (int i = 0; i < tries; i++) {
        NextScanCode();
        alignas(4) uint8_t sphere[0x10];
        PX_GetBoundingSphere(self, sphere);
        float radius = PX_Fn465E10(self);
        uint8_t hit = 0;
        bool anyHit;
        if (At<uint8_t>(self, 0x15F8) == 0) {
            if (near) {
                PX_groundOnly = 1;
                if (PX_TestSphereNearby(self, test, sphere, radius, cp, 0)) {
                    hit = 1;
                }
                if (PX_TestSphereSectors(self, test, sphere, radius, cp, 0)) {
                    hit = 1;
                }
                NextScanCode();
                PX_groundOnly = 0;
            }
            if (PX_TestSphereNearby(self, test, sphere, radius, cp, 0)) {
                hit = 1;
            }
            anyHit = PX_TestSphereSectors(self, test, sphere, radius, cp, 0) || hit;
        } else {
            if (near) {
                PX_groundOnly = 1;
                for (uint8_t* node = At<uint8_t*>(self, 0x178); node; node = At<uint8_t*>(node, 0x10)) {
                    At<float>(cp, 0x24) = PX_fltMax;
                    if (PX_TestSphereList(self, test, At<void*>(node, 8), cp, 0, 1)) {
                        hit = 1;
                    }
                }
                NextScanCode();
                PX_groundOnly = 0;
            }
            for (uint8_t* node = At<uint8_t*>(self, 0x178); node; node = At<uint8_t*>(node, 0x10)) {
                At<float>(cp, 0x24) = PX_fltMax;
                if (PX_TestSphereList(self, test, At<void*>(node, 8), cp, 0, 1)) {
                    hit = 1;
                }
            }
            anyHit = hit != 0;
        }
        if (!anyHit) {
            break;
        }
        count++;
        if (i == tries - 1 || (self->pedType == PED_TYPE_PLAYER && PX_slopeContacts > PX_maxSlopeContacts &&
                               PX_onGround > 0 && !PX_IsSwimming(self))) {
            // can't get clear: back where it was
            ped::CVector back = testPos;
            SetPos(self, back.x, back.y, back.z);
            count = 0;
            break;
        }
    }
    At<int32_t>(self, 0x1318) = 0;
    if (0.0f > drop) {
        // stepping down: its ground found below it
        ped::CVector before = testPos;
        PX_MatrixAssign(test, PX_Fn8B6120(self));
        PX_MatrixFn413600(test, 0.0f, 0.0f, 1.0f, 0);
        if (before.z > testPos.z) {
            testPos.z = before.z;
        }
        PosOf(self)->z = PosOf(self)->z + drop;
        alignas(4) uint8_t ground[0x28] = {};
        ColPointInit(ground);
        alignas(4) uint8_t sphere[0x10];
        PX_GetBoundingSphere(self, sphere);
        float radius = PX_Fn465E10(self);
        bool found = PX_TestSphere(self, test, sphere, radius, ground, 0) != 0;
        if (!found && count == 0 && !PX_IsSwimming(self)) {
            ped::CVector u = testPos;
            float height = reinterpret_cast<float(__thiscall*)(CPed*)>(static_cast<void*>(self->vtable[0x70 / 4]))(self);
            float top = static_cast<float>(height * PX_d_1_1 + u.z);
            testPos = {u.x, u.y, top};
            found = PX_TestSphere(self, test, sphere, radius, ground, 1) != 0;
        }
        if (!found) {
            At<int32_t>(self, 0x1324) = 1;
            if (!(self->flags1E24 & 1)) {
                PosOf(self)->z = At<float>(frame, 0x38);
            }
        } else {
            ped::CVector q = testPos;
            ped::CVector d;
            PX_VectorSub(&d, PosOf(self), &q);
            float k = static_cast<float>(At<float>(ground, 0x24) - PX_d_0_005);
            ped::CVector step = {k * d.x, k * d.y, k * d.z};
            ped::CVector p;
            PX_VectorAdd(&p, &q, &step);
            SetPos(self, p.x, p.y, p.z);
            At<uint8_t>(self, 0x1BE) = ground[0x22];
            At<ped::CVector>(self, 0x174C) = At<ped::CVector>(ground, 0);
            void* entity = At<void*>(ground, 0xC);
            re::Ptr32<void>& ref = At<void*>(self, 0x1758);
            if (entity != ref) {
                if (ref) {
                    PX_CleanUpOldReference(ref, &ref);
                }
                ref = entity;
                if (entity) {
                    PX_RegisterReference(entity, &ref);
                }
            }
            At<float>(self, 0x1770) = At<float>(ground, 0x24);
            At<int32_t>(self, 0x1768) = At<int32_t>(ground, 0x1C);
            At<uint8_t>(self, 0x176C) = ground[0x20];
            At<uint8_t>(self, 0x176D) = ground[0x21];
            At<uint8_t>(self, 0x176E) = ground[0x22];
            At<uint8_t>(self, 0x176F) = ground[0x23];
            At<ped::CVector>(self, 0x173C) = At<ped::CVector>(ground, 0x10);
            uint8_t* e = At<uint8_t*>(ground, 0xC);
            if (!reinterpret_cast<uint8_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(e))[0x14 / 4]))(e) ||
                PX_minGroundUpPlayer < At<float>(self, 0x1744)) {
                At<ped::CVector>(self, 0x175C) = At<ped::CVector>(self, 0x173C);
            }
            ped::CVector& speed = At<ped::CVector>(self, 0x124);
            if (!(0.0f > speed.z)) {
                At<int32_t>(self, 0x1324) = 0;
            } else {
                ped::CVector n = At<ped::CVector>(ground, 0x10);
                float newZ = 0.0f;
                if (At<int32_t>(self, 0x1474) != 0 && 0.0f < n.z) {
                    // landed while sliding: a hard landing hurts
                    if (At<int32_t>(self, 0x1318) == 0 && PX_f_minus0_3 > speed.z) {
                        PX_Fn485A80(Bytes(self) + 0x2F0, nullptr);
                        At<uint32_t>(self, 0x4CC) = PX_timeInMs;  // (the game tests +0x3BC for null)
                        PX_Fn48BF50(self->unknown3BC, nullptr);
                        if (At<int32_t>(self, 0x1344) == 0 && At<int32_t>(self, 0x138C) == 0) {
                            if (self->pedType == PED_TYPE_PLAYER &&
                                (PX_C3CCF8 == 2 || PX_C3CCF8 == 0xD || PX_C3CCF8 == 0xA || PX_C3CCF8 == 4)) {
                                PX_CameraFn4F2C20(PX_camera, 1, 0, 1, 0, 0);
                            }
                            PX_Fn425700();
                            PX_TakeHealth(self, 1.0f);
                        }
                    }
                    ped::CVector push = n;
                    float vz = static_cast<float>(speed.z - PX_d_0_1 * n.z * speed.z);
                    speed.z = vz;
                    ped::CVector impact = {0.0f, 0.0f, vz};
                    float up = -vz;
                    float amount = static_cast<float>(n.x * PX_d_minus0 + PX_d_minus0 * n.y + up * n.z);
                    ped::CVector flat = {speed.x, speed.y, 0.0f};
                    PX_VectorNormalise(&flat);
                    ped::CVector tmp;
                    if (IsBumpySurface(At<uint8_t>(self, 0x1BE))) {
                        float s = static_cast<float>(amount * PX_d_0_3);
                        push = {push.x * s, s * push.y, s * push.z};
                        float into = n.x * flat.x + n.y * flat.y + n.z * flat.z;
                        if (into < PX_f_minus0_15) {
                            float a = std::fabs(into);
                            ped::CVector* r = PX_GetRight(self, &tmp);
                            float side = std::fabs(r->y * n.y + n.x * r->x + r->z * n.z);
                            if (side * PX_d_0_5 < a) {
                                At<int32_t>(self, 0x1490) = 1;
                            }
                        }
                    } else {
                        float d1 = PX_DotProduct(&flat, &n);
                        double factor = PX_d_0_3;
                        if (!(d1 > 0.0f)) {
                            double a1 = std::fabs(d1);
                            float side = std::fabs(PX_DotProduct(PX_GetRight(self, &tmp), &n));
                            if (!(side > a1)) {
                                float d2 = PX_DotProduct(&flat, &n);
                                factor = PX_d_0_1;
                                if (d2 > PX_f_minus0_707) {
                                    double a2 = std::fabs(d2);
                                    float side2 = std::fabs(PX_DotProduct(PX_GetRight(self, &tmp), &n));
                                    if (side2 * PX_d_0_5 < a2) {
                                        factor = PX_d_0_005;
                                    }
                                }
                            }
                        }
                        PX_VectorScaleBy(&push, static_cast<float>(amount * factor));
                    }
                    push.z = 0.0f;
                    speed.z = 0.0f;
                    ped::CVector sum;
                    ped::CVector* s = PX_VectorAdd(&sum, &impact, &push);
                    speed.x = s->x + speed.x;
                    speed.y = s->y + speed.y;
                    speed.z = s->z + speed.z;
                    float v = speed.z;
                    newZ = v < 0.0f ? v : 0.0f;
                }
                speed.z = newZ;
                At<int32_t>(self, 0x1318) = 1;
                At<int32_t>(self, 0x1324) = 0;
            }
        }
        ColPointRelease(ground);
    }
    // its Gamebryo transform
    void* rw = self->rwObject;
    if (rw && At<void*>(rw, 4) && PX_Fn6C9A90(At<void*>(rw, 4))) {
        uint8_t* node = PX_Fn6C9A90(At<void*>(rw, 4));
        if (self->matrix) {
            uint8_t* m = static_cast<uint8_t*>(PX_Fn8B6120(self));
            if (At<void*>(m, 0x40) == nullptr) {
                void* target = nullptr;
                if (self->rwObject && At<void*>(self->rwObject, 4)) {
                    target = PX_Fn6C9A90(At<void*>(self->rwObject, 4));
                }
                m = static_cast<uint8_t*>(PX_Fn8B6120(self));
                At<void*>(m, 0x40) = target;
                PX_MatrixFn413A70(m);
            } else {
                PX_MatrixFn413A70(PX_Fn8B6120(self));
            }
        } else {
            PX_SimpleTransformUpdateNi(Bytes(self) + 4, node);
        }
    }
    // how far it moved this frame
    ped::CVector start = At<ped::CVector>(frame, 0x30);
    At<int32_t>(self, 0x34) = 0;
    At<int32_t>(self, 0x38) = 1;
    ped::CVector d;
    At<ped::CVector>(self, 0x1714) = *PX_VectorSub(&d, PosOf(self), &start);
    const ped::CVector& m = At<ped::CVector>(self, 0x1714);
    At<float>(self, 0x19C) =
        static_cast<float>(std::sqrt(static_cast<double>(std::fabs(m.x * m.x + m.y * m.y + m.z * m.z))));
    // the player's ground contacts
    if (self->pedType == PED_TYPE_PLAYER) {
        PX_Fn4E9DF0(PX_C3B808, 0);
        bool none = true;
        bool slope = false;
        if (PX_C2B109 == 0 && self->unknown1470 == 0 && At<int16_t>(self->weapon, 0x10E) != PX_weapon4330) {
            for (int i = 0; i < 2; i++) {
                if (!(1.0f > PX_contactFraction[i])) {
                    continue;
                }
                ped::CVector n = At<ped::CVector>(PX_contacts, i * 0x28);
                if (PX_maxGroundZ < n.z || PX_maxGroundZ == n.z) {
                    continue;
                }
                PX_onGround++;
                none = false;
                if (PX_onGround == 0xFFFFFFFF) {
                    PX_onGround = 1;
                }
                ped::CVector q;
                q.x = static_cast<float>(n.x * PX_d_0_25);
                q.y = static_cast<float>(n.y * PX_d_0_25);
                q.z = static_cast<float>(n.z * PX_d_0_25);
                ped::CVector sum;
                PX_groundNormal = *PX_VectorAdd(&sum, &PX_groundNormal, &q);
                PX_VectorNormalise(&PX_groundNormal);
                if (i > 0 && 1.0f > PX_contactFraction[i - 1]) {
                    ped::CVector p = At<ped::CVector>(PX_contacts, (i - 1) * 0x28);
                    if (PX_maxGroundZ > p.z) {
                        float dx = std::fabs(n.x - p.x);
                        bool differ;
                        if (!(dx <= PX_f_1e5)) {  // (NaN: they differ)
                            differ = true;
                        } else {
                            float dy = std::fabs(n.y - p.y);
                            differ = !(dy <= PX_f_1e5) || !PX_NearlyEqual(n.z, p.z, PX_f_1e5);
                        }
                        if (differ) {
                            PX_slopeContacts++;
                            if (PX_slopeContacts == 0xFFFFFFFF) {
                                PX_slopeContacts = 1;
                            }
                            slope = true;
                        }
                    }
                }
            }
        }
        if (none) {
            PX_groundNormal = PX_zeroVector;
            PX_onGround = 0;
            PX_slopeContacts = 0;
        } else if (!slope) {
            // on flat ground and moving: the slope tally starts again
            const ped::CVector& speed = At<ped::CVector>(self, 0x124);
            if (speed.x != 0.0f || speed.y != 0.0f) {
                PX_slopeContacts = 0;
            }
        }
    } else {
        PX_Fn4E9EF0(PX_C3B808, 0);
    }
    ColPointRelease(cp);
}

REVERSED("CPed::ProcessCollision", 0x00482070, RE_BODY(&CPed_ProcessCollision), nullptr, nullptr,
         re::Mode::Original);

} // namespace
