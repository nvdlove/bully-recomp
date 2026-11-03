// CPed (0x47A700 .. 0x47B78F): pushing it out of what it touches, the nearby entities it
// keeps for its collision queries (+0x1588, up to 24), the sphere and line queries over
// them (or the world's sectors), its pre-render (vf17: fading its weapon, the vehicle
// bumps) and teleporting (vf14).

#include <cmath>

#include "game/Ped.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;
const ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

constexpr int PED_TYPE_PLAYER = 0xD;

GAME_ARRAY(uint8_t*, PQ_modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs
GAME_VAR(uint8_t, PQ_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, PQ_chapterAreaProps, 0x00A147EC);
GAME_VAR(int32_t, PQ_currArea, 0x00BD1008);
GAME_VAR(float, PQ_fltMax, 0x009009B8);
GAME_VAR(uint16_t, PQ_scanCode, 0x00C1AE88);          // CWorld::ms_nCurrentScanCode
GAME_VAR(uint32_t*, PQ_entryLinks, 0x00C0F788);       // the sectors' entity lists' links
GAME_VAR(float, PQ_f_1000, 0x00900F40);
GAME_VAR(float, PQ_f_1, 0x008FF1E0);
GAME_VAR(float, PQ_f_4, 0x00900B0C);
GAME_VAR(uint8_t, PQ_C2B10B, 0x00C2B10B);
// pushing out
GAME_VAR(double, PQ_d_0_005, 0x0090C740);
GAME_VAR(int32_t, PQ_pushFromPeds, 0x00A339C8);
GAME_VAR(float, PQ_pedRadius, 0x00CF311C);
GAME_VAR(double, PQ_d_0_75, 0x00905408);
GAME_VAR(double, PQ_d_0_25, 0x00900498);
GAME_VAR(double, PQ_d_0_02, 0x0090BFF8);
GAME_VAR(CEntity*, PQ_lastPushedBy, 0x00C2B118);
GAME_VAR(uint8_t, PQ_C2B109, 0x00C2B109);
GAME_VAR(float, PQ_minGroundUp, 0x00C2B1A4);
// vf17
GAME_VAR(int32_t, PQ_vehicleBumps, 0x00A339CC);
GAME_VAR(float, PQ_bump1, 0x00A33954);
GAME_VAR(float, PQ_bump2, 0x00A33958);
GAME_VAR(float, PQ_bump3, 0x00A3395C);
GAME_VAR(double, PQ_d_32767, 0x00900CE8);
GAME_VAR(int32_t, PQ_weapon4330, 0x00BF4330);
inline void* const PQ_camera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const PQ_D13730 = reinterpret_cast<void*>(0x00D13730);

GAME_FN(ped::CVector*, __cdecl, PQ_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PQ_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PQ_NormalisedCopy, 0x00472D40, ped::CVector* out, const ped::CVector* in);
GAME_FN(uint8_t, __thiscall, PQ_IsSwimming, 0x00476D70, CPed* ped);
GAME_FN(void, __thiscall, PQ_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(int, __thiscall, PQ_CameraGetFadeState, 0x004F12F0, void* camera);
GAME_FN(float, __thiscall, PQ_Fn465E10, 0x00465E10, CPed* ped);
GAME_FN(void, __thiscall, PQ_GetBoundingSphere, 0x00466CA0, CPed* ped, void* sphere);
GAME_FN(void, __cdecl, PQ_ClearScanCodes, 0x0045D840);
GAME_FN(uint8_t*, __cdecl, PQ_GetEntity, 0x0044A3E0, int type, int index);
GAME_FN(uint8_t, __thiscall, PQ_Fn5EBC70, 0x005EBC70, void* self, void* entity);
GAME_FN(uint8_t*, __cdecl, PQ_GetColModel, 0x0051AF50, void* entity);
GAME_FN(uint8_t, __thiscall, PQ_Fn512760, 0x00512760, void* entity);
GAME_FN(uint8_t, __thiscall, PQ_Fn41C540, 0x0041C540, void* entity);
GAME_FN(uint8_t*, __thiscall, PQ_ModelInfoFn5201D0, 0x005201D0, void* modelInfo);
GAME_FN(uint8_t, __thiscall, PQ_EntityTouchesSphere, 0x00465BA0, void* entity, const void* centre, float radius);
GAME_FN(uint8_t, __thiscall, PQ_EntityTouchesSphere2, 0x00465D10, void* entity, const void* centre, float radius);
GAME_FN(re::Ptr32<CEntity>*, __thiscall, PQ_EntityPtrSet, 0x00478550, re::Ptr32<CEntity>* self, CEntity* entity);
GAME_FN(void, __thiscall, PQ_ColPointAssign, 0x0046AA70, void* colPoint, const void* from);
GAME_FN(void, __thiscall, PQ_ColPointSetEntity, 0x0045F1F0, void* colPoint, void* entity);
GAME_FN(void, __thiscall, PQ_ColPointConstruct, 0x00415FA0, void* colPoint);
GAME_FN(void, __thiscall, PQ_ColPointDestruct, 0x00415500, void* colPoint);
GAME_FN(void, __cdecl, PQ_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __thiscall, PQ_PedUpdateNearbyEntities, 0x0047A9E0, CPed* ped);
GAME_FN(void*, __thiscall, PQ_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(uint8_t, __cdecl, PQ_SphereCastAgainst, 0x0056A060, const ped::CVector* start, const ped::CVector* end,
        float radius, void* matrix, void* colModel, void* colPoint, void* out, int a, int flags, int b);
GAME_FN(uint8_t, __cdecl, PQ_ProcessSphereCast, 0x004616A0, const ped::CVector* start, const ped::CVector* end,
        float radius, void* colPoint, void* entity, int a, int flags, int b);
GAME_FN(uint8_t, __thiscall, PQ_Ped476200, 0x00476200, CPed* ped, void* entity);
GAME_FN(uint8_t, __thiscall, PQ_PedCanUseEntity, 0x004762F0, CPed* ped, void* entity);
GAME_FN(void*, __thiscall, PQ_TargetingGetTarget, 0x004D5AD0, void* targeting);
GAME_FN(uint8_t, __thiscall, PQ_PedIsInVehicle, 0x004762B0, CPed* ped, void* entity);
GAME_FN(uint8_t, __thiscall, PQ_PedIsOwnEntity, 0x004763C0, CPed* ped, void* entity);
GAME_FN(uint8_t, __cdecl, PQ_Fn683A30, 0x00683A30);
GAME_FN(int, __cdecl, PQ_GetAlpha, 0x005F2B10, void* rwObject);
GAME_FN(void, __cdecl, PQ_SetAlpha, 0x005F2B00, void* rwObject, int alpha);
GAME_FN(ped::CVector*, __thiscall, PQ_GetForward, 0x004154A0, void* entity, ped::CVector* out);
inline int __cdecl PQ_rand() { return crt::Rand(); }
GAME_FN(void*, __cdecl, PQ_Fn5EE1A0, 0x005EE1A0, void* rwObject);
GAME_FN(uint8_t*, __cdecl, PQ_GetHierarchy, 0x006C7710, void* clump);
GAME_FN(void*, __cdecl, PQ_Fn475050, 0x00475050, void* out, const ped::CVector* v, int a);
GAME_FN(void, __thiscall, PQ_HeldFn4A7430, 0x004A7430, void* held);
GAME_FN(void, __thiscall, PQ_Fn4DAB90, 0x004DAB90, void* self);
GAME_FN(uint8_t, __cdecl, PQ_Fn430320, 0x00430320, int area);
GAME_FN(void, __thiscall, PQ_PedFn4781E0, 0x004781E0, CPed* ped);
GAME_FN(void, __cdecl, PQ_WorldRemove, 0x0045DD70, CPed* ped);
GAME_FN(void, __cdecl, PQ_WorldAdd, 0x0045D6B0, CPed* ped);
GAME_FN(void, __thiscall, PQ_MatrixCopy, 0x00412A10, void* self, const void* from);
GAME_FN(void, __thiscall, PQ_HandleDestruct, 0x0049A650, void* self);

// A col point (0x28 bytes, +0xC a registered entity, +0x10 the normal, +0x24 the depth)
void ColPointInit(uint8_t* cp) {
    At<ped::CVector>(cp, 0) = {0.0f, 0.0f, 0.0f};
    At<void*>(cp, 0xC) = nullptr;
    At<ped::CVector>(cp, 0x10) = {0.0f, 0.0f, 0.0f};
    At<float>(cp, 0x24) = PQ_fltMax;
}
void ColPointRelease(uint8_t* cp) {
    re::Ptr32<void>& entity = At<void*>(cp, 0xC);
    if (entity) {
        PQ_CleanUpOldReference(entity, &entity);
    }
}

// The checks the nearby-entity queries share: collidable, present in this chapter
bool IsQueryable(uint8_t* e) {
    return At<int32_t>(e, 0x20) != 0 && (At<int32_t>(e, 0x1C) != 0 || At<int32_t>(e, 0x60) != 0) &&
           (PQ_modelInfoPtrs[At<int16_t>(e, 0x10E)][0xB] & static_cast<uint8_t>(1 << (PQ_chapter & 0x1F))) != 0 &&
           At<int32_t>(e, 0xF4) == 0;
}

// 0x47A700 (thiscall): pushed out of a col point's surface (its depth less 0.005) around
// `matrix`'s position (and away from a ped it's inside, with 0xA339C8)
void __fastcall CPed_PushOut(CPed* self, re::Edx, uint8_t* colPoint, uint8_t* matrix) {
    ped::CVector centre = At<ped::CVector>(matrix, 0x30);
    ped::CVector d;
    PQ_VectorSub(&d, PosOf(self), &centre);
    float depth = static_cast<float>(At<float>(colPoint, 0x24) - PQ_d_0_005);
    ped::CVector k = {d.x * depth, d.y * depth, depth * d.z};
    if (PQ_pushFromPeds != 0) {
        uint8_t* other = At<uint8_t*>(colPoint, 0xC);
        if (At<int32_t>(self, 0x1318) != 0 && (other[0x108] & 7) == 3) {
            ped::CVector e;
            PQ_VectorSub(&e, PosOf(self), PosOf(other));
            e.z = 0.0f;
            float dist = static_cast<float>(std::sqrt(static_cast<double>(std::fabs(e.y * e.y + e.x * e.x + 0.0f * 0.0f))));
            if (PQ_d_0_75 * PQ_pedRadius > dist) {
                float q = static_cast<float>(PQ_pedRadius * PQ_d_0_25);
                ped::CVector n;
                PQ_NormalisedCopy(&n, &e);
                PQ_lastPushedBy = reinterpret_cast<CEntity*>(other);
                ped::CVector push = {q * n.x, n.y * q, q * n.z};
                d.x = push.x + d.x;
                d.y = push.y + d.y;
                d.z = push.z + d.z;
            }
        }
    }
    const ped::CVector& normal = At<ped::CVector>(colPoint, 0x10);
    d.x = d.x - k.x;
    d.y = d.y - k.y;
    d.z = d.z - k.z;
    ped::CVector back = {-d.x, -d.y, -d.z};
    float t = normal.y * back.y + normal.x * back.x + normal.z * back.z;
    t = static_cast<float>(t + PQ_d_0_02);
    ped::CVector along = {t * normal.x, t * normal.y, t * normal.z};
    ped::CVector r1;
    PQ_VectorAdd(&r1, &d, &along);
    ped::CVector r2;
    PQ_VectorAdd(&r2, &centre, &k);
    ped::CVector r3;
    PQ_VectorAdd(&r3, &r2, &r1);
    if (self->pedType == PED_TYPE_PLAYER && PQ_C2B109 && PQ_minGroundUp > normal.z && !PQ_IsSwimming(self)) {
        r3.z = centre.z;
    }
    PQ_SetPosition(self, r3.x, r3.y, r3.z);
}

// 0x47A9E0 (thiscall): the entities near it (+0x1588, +0x15F4 of them; +0x15F8 when there
// are more than 24, or the camera is fading), found again once it's moved a metre
void __fastcall CPed_UpdateNearbyEntities(CPed* self, re::Edx) {
    ped::CVector& scanPos = At<ped::CVector>(self, 0x15E8);
    if (PQ_CameraGetFadeState(PQ_camera) != 0) {
        At<uint8_t>(self, 0x15F8) = 1;
        scanPos = {PQ_f_1000, PQ_f_1000, PQ_f_1000};
        return;
    }
    ped::CVector d;
    PQ_VectorSub(&d, &scanPos, PosOf(self));
    float sq = d.x * d.x + d.y * d.y + d.z * d.z;
    if (sq < PQ_f_1) {
        return;
    }
    scanPos = *PosOf(self);
    PQ_Fn465E10(self);
    alignas(4) uint8_t sphere[0x10];
    PQ_GetBoundingSphere(self, sphere);
    At<int32_t>(self, 0x15F4) = 0;
    At<uint8_t>(self, 0x15F8) = 0;
    uint16_t scan;
    if (PQ_scanCode < 0xFFFF) {
        scan = static_cast<uint16_t>(PQ_scanCode + 1);
    } else {
        PQ_ClearScanCodes();
        scan = 1;
    }
    PQ_scanCode = scan;
    for (uint8_t* node = At<uint8_t*>(self, 0x178); node; node = At<uint8_t*>(node, 0x10)) {
        re::Ptr32<uint32_t>* lists = At<re::Ptr32<uint32_t>*>(node, 8);
        for (int k = 0; k < 2; k++) {
            uint32_t* link = lists[k];
            while (link) {
                uint32_t v = *link;
                uint8_t* e = PQ_GetEntity(v & 0xF, (v >> 4) & 0x3FFF);
                link = (v >> 18) == 0x3FFF ? nullptr : PQ_entryLinks + (v >> 18);
                if (At<int32_t>(e, 0xC4) != 0 || (e[0x108] & 7) == 7 || PQ_Fn5EBC70(PQ_D13730, e) ||
                    At<uint16_t>(e, 0x10A) == scan) {
                    continue;
                }
                At<uint16_t>(e, 0x10A) = scan;
                uint8_t* data = At<uint8_t*>(PQ_GetColModel(e), 0x38);
                if (!data ||
                    (At<int16_t>(data, 0x1C) == 0 && At<uint8_t>(data, 0x1E) == 0 && At<uint8_t>(data, 0x1F) == 0) ||
                    At<int32_t>(data, 0x18) != 0) {
                    continue;
                }
                uint8_t r = reinterpret_cast<uint8_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(e))[0x78 / 4]))(e);
                if (r == 1) {
                    uint8_t type = e[0x108] & 7;
                    if (type == 1) {
                        if (!PQ_Fn512760(e) || !PQ_Fn41C540(e)) {
                            continue;
                        }
                    } else if (type == 6) {
                        if (PQ_ModelInfoFn5201D0(PQ_modelInfoPtrs[At<int16_t>(e, 0x10E)])[0x2C] == 0) {
                            continue;
                        }
                    } else {
                        continue;
                    }
                } else if (r != 0) {
                    continue;
                }
                if (PQ_EntityTouchesSphere(e, sphere, PQ_f_4)) {
                    int32_t& count = At<int32_t>(self, 0x15F4);
                    if (count >= 0x18) {
                        At<uint8_t>(self, 0x15F8) = 1;
                        return;
                    }
                    PQ_EntityPtrSet(&At<CEntity*>(self, 0x1588 + count * 4), reinterpret_cast<CEntity*>(e));
                    count++;
                }
            }
        }
    }
}

// 0x47ACC0 (thiscall): the nearest hit (vf41) of a sphere against the nearby entities
uint8_t __fastcall CPed_TestSphereNearby(CPed* self, re::Edx, int a, const void* centre, float radius, uint8_t* out,
                                         int b) {
    At<float>(out, 0x24) = PQ_fltMax;
    uint8_t hit = 0;
    for (int i = 0; i < At<int32_t>(self, 0x15F4); i++) {
        uint8_t* e = At<uint8_t*>(self, 0x1588 + i * 4);
        if (!e || !IsQueryable(e) || !PQ_EntityTouchesSphere(e, centre, radius)) {
            continue;
        }
        alignas(4) uint8_t cp[0x28] = {};
        ColPointInit(cp);
        int r = reinterpret_cast<int(__thiscall*)(CPed*, int, void*, void*, int)>(static_cast<void*>(self->vtable[0xA4 / 4]))(self, a, e, cp,
                                                                                                          b);
        if (r != 0 && At<float>(out, 0x24) > At<float>(cp, 0x24)) {
            PQ_ColPointAssign(out, cp);
            PQ_ColPointSetEntity(out, e);
            hit = 1;
        }
        ColPointRelease(cp);
    }
    return hit;
}

// 0x47AE30 (thiscall): a sphere swept from `start` to `end` against the nearby buildings
// (the world's sphere cast when there are too many)
uint8_t __fastcall CPed_SphereCastNearby(CPed* self, re::Edx, const ped::CVector* start, const ped::CVector* end,
                                         float radius, uint8_t* out) {
    if (!At<uint8_t>(self, 0x15F8)) {
        if (At<int32_t>(self, 0x15F4) == 0) {
            PQ_PedUpdateNearbyEntities(self);
        }
        if (!At<uint8_t>(self, 0x15F8)) {
            float best = PQ_fltMax;
            At<float>(out, 0x24) = PQ_fltMax;
            ped::CVector d;
            PQ_VectorSub(&d, end, start);
            float length = static_cast<float>(std::sqrt(static_cast<double>(std::fabs(d.x * d.x + d.y * d.y + d.z * d.z))));
            float reach = length + radius;
            uint8_t hit = 0;
            for (int i = 0; i < At<int32_t>(self, 0x15F4); i++) {
                uint8_t* e = At<uint8_t*>(self, 0x1588 + i * 4);
                if (!e || !IsQueryable(e) || (e[0x108] & 7) != 1) {
                    continue;
                }
                ped::CVector from = *start;
                if (!PQ_EntityTouchesSphere(e, &from, reach)) {
                    continue;
                }
                alignas(4) uint8_t cp[0x28] = {};
                ColPointInit(cp);
                uint8_t* colModel = PQ_GetColModel(e);
                if (PQ_SphereCastAgainst(start, end, radius, PQ_GetMatrix(e), colModel, cp, &best, 0x1F, 0xF, 1) &&
                    At<float>(out, 0x24) > best) {
                    PQ_ColPointAssign(out, cp);
                    hit = 1;
                }
                ColPointRelease(cp);
            }
            return hit;
        }
    }
    float entity = 0.0f;  // (an entity out the game passes as a float slot)
    return PQ_ProcessSphereCast(start, end, radius, out, &entity, 1, 0xF, 1);
}

// 0x47B0B0 (thiscall): the nearest hit (vf41) of a sphere against what's in its sectors'
// lists (not itself, its vehicle, what it holds or targets...); none with +0x1E25
uint8_t __fastcall CPed_TestSphereSectors(CPed* self, re::Edx, int a, const void* centre, float radius, uint8_t* out,
                                          int b) {
    if (At<uint8_t>(self, 0x1E25) != 0) {
        return 0;
    }
    At<float>(out, 0x24) = PQ_fltMax;
    uint16_t scan = PQ_scanCode;
    uint8_t hit = 0;
    for (uint8_t* node = At<uint8_t*>(self, 0x178); node; node = At<uint8_t*>(node, 0x10)) {
        re::Ptr32<uint32_t>* lists = At<re::Ptr32<uint32_t>*>(node, 8) + 1;
        for (int k = 0; k < 3; k++) {
            uint32_t* link = lists[k];
            while (link) {
                uint32_t v = *link;
                uint8_t* e = PQ_GetEntity(v & 0xF, (v >> 4) & 0x3FFF);
                link = (v >> 18) == 0x3FFF ? nullptr : PQ_entryLinks + (v >> 18);
                if ((e[0x108] & 7) == 3) {
                    if (e == Bytes(self) || PQ_C2B10B == 1 || static_cast<uint8_t>(b) != 0) {
                        continue;
                    }
                } else if (reinterpret_cast<uint8_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(e))[0x10 / 4]))(e) &&
                           self->unknown1C98 != 0) {
                    continue;
                }
                uint8_t type = e[0x108] & 7;
                if (type == 7 || At<int32_t>(e, 0xC4) != 0) {
                    continue;
                }
                if ((type == 6 || type == 4) && !PQ_Fn5EBC70(PQ_D13730, e)) {
                    continue;
                }
                if (At<uint16_t>(e, 0x10A) == scan) {
                    continue;
                }
                At<uint16_t>(e, 0x10A) = scan;
                if (!IsQueryable(e) || !PQ_EntityTouchesSphere2(e, centre, radius)) {
                    continue;
                }
                // (the game relies on these two leaving ecx as the ped)
                if (PQ_Ped476200(self, e) || PQ_PedCanUseEntity(self, e)) {
                    continue;
                }
                if (At<int32_t>(self, 0x13B4) == 0 && PQ_TargetingGetTarget(self->targeting) == e) {
                    continue;
                }
                if (PQ_PedIsInVehicle(self, e) || PQ_PedIsOwnEntity(self, e)) {
                    continue;
                }
                if (PQ_Fn683A30() && type == 4 && At<uint8_t>(e, 0x20A) == 7) {
                    continue;
                }
                alignas(4) uint8_t cp[0x28];
                PQ_ColPointConstruct(cp);
                int r = reinterpret_cast<int(__thiscall*)(CPed*, int, void*, void*, int)>(static_cast<void*>(self->vtable[0xA4 / 4]))(self, a, e,
                                                                                                                  cp, b);
                if (r != 0 && !(At<float>(out, 0x24) <= At<float>(cp, 0x24))) {
                    PQ_ColPointAssign(out, cp);
                    PQ_ColPointSetEntity(out, e);
                    hit = 1;
                }
                PQ_ColPointDestruct(cp);
            }
        }
    }
    return hit;
}

// Three factors around 1 (1 -+ k), random within
ped::CVector RandomAround(double k) {
    float lo = static_cast<float>(1.0 - k);
    float hi = static_cast<float>(k + 1.0);
    double range = static_cast<double>(hi - lo);
    ped::CVector v;
    v.x = static_cast<float>(PQ_rand() / PQ_d_32767 * range + lo);
    v.y = static_cast<float>(PQ_rand() / PQ_d_32767 * range + lo);
    v.z = static_cast<float>(PQ_rand() / PQ_d_32767 * range + lo);
    return v;
}

// vf17 0x47B360: before rendering: the player's weapon faded with it, the bones shaken
// by the vehicle's speed (0xA339CC), its weapon's and held items' own, and 0x4781E0 where
// the chapter's area needs it
void __fastcall CPed_PreRender(CPed* self, re::Edx) {
    int alpha = PQ_GetAlpha(self->rwObject);
    if (self->pedType == PED_TYPE_PLAYER) {
        uint8_t* weaponRw = At<uint8_t*>(self->weapon, 0x18);
        if (weaponRw && weaponRw[0] == 2) {
            int a = alpha;
            if (alpha < 0xFF) {
                a = alpha - (0xFF - alpha) / 4;
                if (a < 0) {
                    a = 0;
                }
            }
            PQ_SetAlpha(weaponRw, a);
        }
    }
    PQ_SetAlpha(self->rwObject, alpha);
    uint8_t* vehicle = re::PtrCast<uint8_t*>(self->vehicle);
    if (vehicle && PQ_vehicleBumps != 0) {
        ped::CVector tmp;
        ped::CVector* f = PQ_GetForward(vehicle, &tmp);
        float speed = At<float>(vehicle, 0x128) * f->y + At<float>(vehicle, 0x124) * f->x + At<float>(vehicle, 0x12C) * f->z;
        ped::CVector v = RandomAround(static_cast<double>(PQ_bump1) * speed);
        void* clump = PQ_Fn5EE1A0(self->rwObject);
        PQ_Fn475050(PQ_GetHierarchy(clump) + 0x2A4, &v, 1);
        v = RandomAround(static_cast<double>(PQ_bump2) * speed);
        PQ_Fn475050(PQ_GetHierarchy(clump) + 0x3DC, &v, 1);
        PQ_Fn475050(PQ_GetHierarchy(clump) + 0x57C, &v, 1);
        PQ_Fn475050(PQ_GetHierarchy(clump) + 0x208, &v, 1);
        v = RandomAround(static_cast<double>(PQ_bump3) * speed);
        PQ_Fn475050(PQ_GetHierarchy(clump) + 0x410, &v, 1);
        PQ_Fn475050(PQ_GetHierarchy(clump) + 0x5B0, &v, 1);
    }
    reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(self->weapon))[0x44 / 4]))(self->weapon);
    PQ_HeldFn4A7430(At<void*>(self, 0x1D8));
    PQ_Fn4DAB90(At<void*>(self, 0x1D4));
    if ((self->unknown1470 != 0 || At<int16_t>(self->weapon, 0x10E) == PQ_weapon4330) &&
        PQ_chapterAreaProps[PQ_chapter] == 2 && PQ_Fn430320(PQ_currArea) == 1) {
        PQ_PedFn4781E0(self);
    }
}

// vf14 0x47B6C0: Teleport (out of the world and back in at the place; its scans reset)
void __fastcall CPed_Teleport(CPed* self, re::Edx, float x, float y, float z) {
    PQ_WorldRemove(self);
    PQ_SetPosition(self, x, y, z);
    At<int32_t>(self, 0x1318) = 0;
    At<int32_t>(self, 0x1A4) = 0;
    At<ped::CVector>(self, 0x15E8) = {PQ_f_1000, PQ_f_1000, PQ_f_1000};
    PQ_WorldAdd(self);
    alignas(4) uint8_t matrix[0x44];
    PQ_MatrixCopy(matrix, PQ_GetMatrix(self));
    At<ped::CVector>(self, 0x1564) = At<ped::CVector>(matrix, 0x30);
    PQ_HandleDestruct(Bytes(self) + 0x654);
    At<float>(self, 0x1DD8) = x;
    At<float>(self, 0x1DDC) = y;
    At<float>(self, 0x1DE0) = z;
}

#define PEDQUERY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDQUERY_REVERSED("CPed::PushOut", 0x0047A700, &CPed_PushOut);
PEDQUERY_REVERSED("CPed::UpdateNearbyEntities", 0x0047A9E0, &CPed_UpdateNearbyEntities);
PEDQUERY_REVERSED("CPed::TestSphereNearby", 0x0047ACC0, &CPed_TestSphereNearby);
PEDQUERY_REVERSED("CPed::SphereCastNearby", 0x0047AE30, &CPed_SphereCastNearby);
PEDQUERY_REVERSED("CPed::TestSphereSectors", 0x0047B0B0, &CPed_TestSphereSectors);
PEDQUERY_REVERSED("CPed::PreRender", 0x0047B360, &CPed_PreRender);
PEDQUERY_REVERSED("CPed::Teleport", 0x0047B6C0, &CPed_Teleport);

} // namespace
