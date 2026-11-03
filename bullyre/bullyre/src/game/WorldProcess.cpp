// CWorld's frame (Process), its shutdown and restart, clearing an area, and the line
// tests over a sector list. (GTA's CWorld; the flags at 0xC1AE6C.. are GTA's
// bIncludeDeadPeds-style switches of the line tests.)

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "game/Collision.h"
#include "game/Entity.h"
#include "game/Pool.h"
#include "game/World.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using col::CMatrix;
using col::CVector;

// ---- the game's data ----------------------------------------------------------------------

GAME_VAR(CPtrNode*, s_ptrNodes, 0x00C0F788);
GAME_VAR(CPool<uint8_t>*, s_pedPool, 0x00C0F5F0);      // 0x1F44 each
GAME_VAR(CPool<uint8_t>*, s_vehiclePool, 0x00C0F5F4);  // 0x8C0 each
GAME_ARRAY(CSector, s_sectors, 0x00C1B178);
GAME_VAR(CPtrList, s_bigBuildings, 0x00C216B8);
GAME_VAR(CPtrList, s_movingList, 0x00C1AEA4);
GAME_VAR(CPtrNode*, s_nextMovingNode, 0x00C1AE84);
GAME_VAR(uint16_t, s_scanCode, 0x00C1AE88);
GAME_VAR(CEntity*, s_playerPed, 0x00C1AEA8);            // CWorld::Player's ped
inline void* const s_worldPlayer = reinterpret_cast<void*>(0x00C1AEA8);

// the switches of the line tests (GTA's CWorld statics)
GAME_VAR(int32_t, s_numIgnoredModels, 0x00C1AE68);
GAME_ARRAY(int32_t, s_ignoredModels, 0x00C1AA64);       // model ids (compared with +0x10E)
GAME_VAR(uint8_t, s_skipFlagC4, 0x00C1AE6C);            // entities with +0xC4 are skipped
GAME_VAR(uint8_t, s_flag6D, 0x00C1AE6D);
GAME_VAR(uint8_t, s_processCutsceneOnly, 0x00C1AE6E);
GAME_VAR(uint8_t, s_secondPass, 0x00C1AE6F);            // the moving list's second pass
GAME_VAR(CEntity*, s_ignoreEntity, 0x00C1AE70);         // pIgnoreEntity
GAME_VAR(uint8_t, s_includeDriverPeds, 0x00C1AE74);     // peds whose +0x1554 is in state 1
GAME_VAR(uint8_t, s_includeFlag1C98Peds, 0x00C1AE75);   // peds with +0x1C98
GAME_VAR(uint8_t, s_flag76, 0x00C1AE76);
GAME_VAR(uint8_t, s_skipPropAnims, 0x00C1AE80);
GAME_VAR(uint8_t, s_skipSomePropAnims, 0x00C1AE81);     // unless their anim model's +0x85
GAME_VAR(uint8_t, s_lineTestFlag, 0x00C1AE82);          // passed on to the collision tests
GAME_VAR(uint8_t, s_forceProcessControl, 0x00C1AE83);
GAME_VAR(uint8_t, s_chapter, 0x00A147E0);               // a model is there if bit `chapter` of its +0xB
GAME_VAR(int32_t, s_specialPedModel, 0x00BF42DC);       // (the peds' line test uses its own model)
GAME_VAR(CColModel*, s_pedLineColModel, 0x00CF3138);    // (CTempColModels': 0x5727D0 fills it for a ped)
GAME_VAR(CColModel*, s_worldColModel, 0x00C1AE90);
GAME_ARRAY(int32_t, s_verticalTestModels, 0x00BF3E88);  // [4]
GAME_ARRAY(char, s_debugText, 0x00C221A8);
inline const char* const s_buildingListNotEmpty = reinterpret_cast<const char*>(0x0090AF2C);
inline const char* const s_dummyListNotEmpty = reinterpret_cast<const char*>(0x0090AF10);

GAME_VAR(uint32_t, s_frameCounter, 0x00C1A99C);         // CTimer::m_FrameCounter
GAME_VAR(float, s_timeStep, 0x00C1A9A4);                // CTimer::ms_fTimeStep
GAME_VAR(float, s_timeStepScale, 0x00A14A94);
GAME_VAR(float, s_fallThroughZ, 0x00A14A8C);            // below it an entity has fallen out of the map
GAME_VAR(void*, s_rwCamera, 0x00C3CCF4);
GAME_VAR(uint8_t, s_cutsceneRunning, 0x020C5BE4);
GAME_ARRAY(CEntity*, s_cutsceneObjects, 0x020C5B68);    // [30]
GAME_VAR(uint8_t, s_cutsceneFlag, 0x020C5BE1);
GAME_VAR(void*, s_hud_C674F0, 0x00C674F0);
GAME_VAR(void*, s_population, 0x00C2C108);
GAME_VAR(void*, s_obj_C2C1A0, 0x00C2C1A0);
GAME_VAR(void*, s_animHandle1B65504, 0x01B65504);        // the prop anim handles 0x45F230 compares
GAME_VAR(void*, s_animHandle1B65508, 0x01B65508);
GAME_VAR(void*, s_animHandle1B6550C, 0x01B6550C);
GAME_VAR(void*, s_animHandle1B65518, 0x01B65518);
inline void* const s_propAnimMgr = reinterpret_cast<void*>(0x00C3B808);
inline void* const s_obj_D13730 = reinterpret_cast<void*>(0x00D13730);
inline void* const s_obj_20C8A00 = reinterpret_cast<void*>(0x020C8A00);
inline void* const s_obj_20C4910 = reinterpret_cast<void*>(0x020C4910);
inline void* const s_obj_C2C730 = reinterpret_cast<void*>(0x00C2C730);
inline void* const s_paths = reinterpret_cast<void*>(0x00BF6228);

// ---- the game's functions -----------------------------------------------------------------

GAME_FN(CEntity*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, uint32_t slot);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, CEntity* entity);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
GAME_FN(CColModel*, __cdecl, GetColModel, 0x0051AF50, CEntity* entity);
GAME_FN(uint8_t, __thiscall, Entity_GetIsTouching, 0x00465BA0, CEntity* entity, const CVector* center, float radius);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(uint8_t*, __thiscall, ModelInfo_GetAnimInfo, 0x005201D0, void* modelInfo);  // (+0x80, +0x85)
GAME_FN(uint8_t, __thiscall, ModelInfo_Fn520440, 0x00520440, void* modelInfo);
// the collision tests (GTA's CCollision::ProcessLineOfSight and friends)
GAME_FN(void, __thiscall, ColLine_Set, 0x00564950, void* line, const CVector* start, const CVector* end);
GAME_FN(uint8_t, __cdecl, Col_ProcessSphereCast, 0x0056A060, const CVector* start, const void* b, float c, CMatrix* m, CColModel* col,
        CColPoint* point, float* dist, uint32_t flags, int e, int f);
GAME_FN(uint8_t, __cdecl, Col_ProcessLineOfSight, 0x0056B4D0, const void* line, CMatrix* m, CColModel* col, CColPoint* point,
        float* dist, uint32_t flags, int surfaces, uintptr_t g);
GAME_FN(uint8_t, __cdecl, Col_ProcessLineOfSightPed, 0x0056B030, const void* line, CMatrix* m, CColModel* col, CColPoint* point,
        float* dist, uint32_t flags, uint8_t g);
GAME_FN(uint8_t, __cdecl, Col_TestLineOfSight, 0x0056ACD0, const void* line, CMatrix* m, CColModel* col, uint32_t flags, uint8_t g);
GAME_FN(void, __cdecl, Ped_SetupLineColModel, 0x005727D0, CEntity* ped);
GAME_FN(uint8_t, __thiscall, Entity_Fn41C540, 0x0041C540, CEntity* entity);
GAME_FN(uint8_t, __thiscall, Ped_Fn4473E0, 0x004473E0, CEntity* ped);
GAME_FN(void, __thiscall, Ped_MoveAgainstVehicle, 0x00483690, CEntity* ped, CEntity* entity, CColModel* col, CMatrix* matrix, CVector* offset, float radius);
GAME_FN(uint8_t, __thiscall, Ped_IsAmbient, 0x004765C0, CEntity* ped);
GAME_FN(uint8_t, __thiscall, Vehicle_CanBeCleared, 0x004CB7F0, CEntity* vehicle);
GAME_FN(void, __cdecl, Fn_4CFEF0_x64, 0x004CFEF0, float, float, float, float radius);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Fn_4CFEF0(CVector a0, float a1) {
    return Fn_4CFEF0_x64(a0.x, a0.y, a0.z, a1);
}
GAME_FN(void, __cdecl, RemoveTrailsInSphere_x64, 0x004D26F0, float, float, float, float radius);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void RemoveTrailsInSphere(CVector a0, float a1) {
    return RemoveTrailsInSphere_x64(a0.x, a0.y, a0.z, a1);
}
GAME_FN(void, __cdecl, ClearAllProjectiles, 0x004D4A90);
GAME_FN(void, __thiscall, AnimHandle_Set, 0x005FA870, re::Ptr32<void>* handle, void* value);
GAME_FN(void, __thiscall, AnimHandle_Release, 0x0049A650, re::Ptr32<void>* handle);
template <typename... A>
inline int __cdecl Sprintf(char* out, const char* format, A... rest) { return crt::Sprintf(out, format, rest...); }
GAME_FN(void, __thiscall, PtrList_Flush, 0x0044E070, CPtrList* list);
GAME_FN(void, __thiscall, ColModel_RemoveCollisionVolumes, 0x0056E7D0, CColModel* col);
GAME_FN(void, __cdecl, ColModel_Delete, 0x0056EB30, CColModel* col);
GAME_FN(void, __cdecl, Fn_4399E0, 0x004399E0);
GAME_FN(void, __cdecl, RemoveExtraScene, 0x00429940);
GAME_FN(void, __cdecl, StopCutscene, 0x006C4030);
GAME_FN(void, __thiscall, Fn_55BC80, 0x0055BC80, void* self);
GAME_FN(void, __cdecl, Fn_4CFDF0, 0x004CFDF0);
GAME_FN(void, __cdecl, Fn_4CFE70, 0x004CFE70);
GAME_FN(void, __thiscall, Population_Fn49E460, 0x0049E460, void* self);
GAME_FN(void, __thiscall, Fn_4A6940, 0x004A6940, void* self);
GAME_FN(void, __thiscall, Fn_4A7750, 0x004A7750, void* self);
GAME_FN(void, __cdecl, KeepLoadingScreenAlive, 0x0043C060, int a, int b);
GAME_FN(void, __cdecl, Fn_44CD60, 0x0044CD60);
// Process's
GAME_FN(void, __cdecl, PruneAllReferencesInWorld, 0x004509B0);
GAME_FN(void, __thiscall, Entity_Fn443AA0, 0x00443AA0, CEntity* entity);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00465760, CEntity* entity);
GAME_FN(int, __cdecl, Cutscene_NumPropAnims, 0x006C4000);
GAME_FN(CEntity*, __cdecl, Cutscene_GetPropAnim, 0x006C4010, int index);
GAME_FN(void, __thiscall, PropAnim_SetCurrent, 0x004ED940, CEntity* prop);
GAME_FN(void, __thiscall, ActionController_Update, 0x005F4B00, void* self, float dt);
GAME_FN(void, __thiscall, Physical_RemoveFromMovingList, 0x00469840, CEntity* entity);
GAME_FN(void*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(CMatrix*, __thiscall, Entity_GetMatrixLink, 0x008B6120, CEntity* entity);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, CMatrix* m);
GAME_FN(void, __thiscall, SimpleTransform_UpdateNiTransform, 0x0046D8F0, CSimpleTransform* t, void* out);
GAME_FN(void, __thiscall, Ped_Fn476DD0, 0x00476DD0, CEntity* ped);
GAME_FN(void, __thiscall, PropAnims_Fn4E9110, 0x004E9110, void* self);
GAME_FN(void, __thiscall, PropAnims_Fn4E92B0, 0x004E92B0, void* self);
GAME_FN(void*, __cdecl, Camera_GetObject, 0x006C82F0, void* camera);
GAME_FN(void, __thiscall, Player_Fn49AEB0, 0x0049AEB0, CEntity* ped);
GAME_FN(void*, __thiscall, Entity_GetObjectInfo, 0x004EA380, CEntity* entity);
GAME_FN(void, __thiscall, PropAnim_Fn4EB200, 0x004EB200, CEntity* prop);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __thiscall, PropAnim_Fn4EEFA0, 0x004EEFA0, CEntity* prop);
GAME_FN(void*, __thiscall, PropAnim_GetPoolElement, 0x004EAE90, CEntity* prop);
GAME_FN(void, __thiscall, PropAnim_Fn4EC3A0, 0x004EC3A0, CEntity* prop);
GAME_FN(void, __thiscall, PropAnim_Fn4EBE80, 0x004EBE80, CEntity* prop);
GAME_FN(uint8_t, __thiscall, PropAnim_HasSpecialAnim, 0x0045F230, CEntity* prop);
GAME_FN(void, __thiscall, Fn_72C9C0, 0x0072C9C0, void* self, CEntity* prop);
GAME_FN(void, __thiscall, PropAnim_Fn4ED410, 0x004ED410, CEntity* prop, CVector* cameraPos);
GAME_FN(void, __thiscall, PropAnim_Fn4EF630, 0x004EF630, CEntity* prop, float dt);
GAME_FN(void, __thiscall, PropAnim_Fn4EEBA0, 0x004EEBA0, CEntity* prop);
GAME_FN(void, __thiscall, PropAnim_Fn4EBCB0, 0x004EBCB0, CEntity* prop);
GAME_FN(void, __thiscall, Fn_6C0A00, 0x006C0A00, void* self);
GAME_FN(void, __thiscall, Fn_5EC500, 0x005EC500, void* self);
GAME_FN(void, __thiscall, Ped_Fn477E30, 0x00477E30, void* ped);
GAME_FN(void, __cdecl, Fn_4D4590, 0x004D4590);
GAME_FN(void, __thiscall, WorldPlayer_Fn446BC0, 0x00446BC0, void* player);
GAME_FN(uint8_t*, __thiscall, Paths_FindNodeClosestToCoors_x64, 0x00441950, void* paths, uintptr_t a, float, float, float, int b, float range, int c, int d, int e, int f);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* Paths_FindNodeClosestToCoors(void* a0, uintptr_t a1, CVector a2, int a3, float a4, int a5, int a6, int a7, int a8) {
    return Paths_FindNodeClosestToCoors_x64(a0, a1, a2.x, a2.y, a2.z, a3, a4, a5, a6, a7, a8);
}
GAME_FN(CEntity*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(CEntity*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, int seat);

// ---- helpers ------------------------------------------------------------------------------

inline CEntity* NodeEntity(const CPtrNode* node) {
    return GetEntityInPool(node->bits & 0xF, (node->bits >> 4) & 0x3FFF);
}
inline CPtrNode* NodeNext(const CPtrNode* node) {
    uint32_t next = node->bits >> 18;
    return next == 0x3FFF ? nullptr : &s_ptrNodes[next];
}

// (x64 bring-up check: a dead entity (vtable 0) on the moving list: its node, pool and slot)
inline void CheckLive(const CPtrNode* node, const CEntity* e) {
#if defined(_M_X64)
    if (*reinterpret_cast<const uint32_t*>(e) != 0) {
        return;
    }
    uint32_t bits = node->bits;
    uint32_t pool = bits & 0xF, slot = (bits >> 4) & 0x3FFF;
    static const uintptr_t kPoolVars[10] = {0x00C0F5F0, 0x00C0F5F4, 0x00C0F614, 0x00C0F618, 0x00C0F61C,
                                            0x00C0F600, 0x00C0F608, 0x00C0F5F8, 0x00C0F5FC, 0x00C0F60C};
    const uint32_t* p = pool < 10 ? reinterpret_cast<const uint32_t*>(uintptr_t(*reinterpret_cast<const uint32_t*>(kPoolVars[pool]))) : nullptr;
    uint8_t flag = p ? reinterpret_cast<const uint8_t*>(uintptr_t(p[1]))[slot] : 0xEE;
    std::fprintf(stderr, "x64 check: moving-list node %p bits 0x%08X: pool %u slot %u entity %p (pool size %u, slot flag 0x%02X)\n",
                 static_cast<const void*>(node), bits, pool, slot, static_cast<const void*>(e), p ? p[2] : 0, flag);
    std::fflush(stderr);
    __debugbreak();
#else
    (void)node;
    (void)e;
#endif
}

#if defined(_M_X64)
// (x64 bring-up check) every in-use ped of the pool has a vtable; else who was just called
inline void PedPoolCheck(void* object, int slot, uint32_t vtableBefore) {
    static bool reported = false;
    if (reported) {
        return;
    }
    const uint32_t* pool = reinterpret_cast<const uint32_t*>(uintptr_t(*reinterpret_cast<const uint32_t*>(0x00C0F5F0)));
    if (pool == nullptr) {
        return;
    }
    const uint8_t* objects = reinterpret_cast<const uint8_t*>(uintptr_t(pool[0]));
    const uint8_t* flags = reinterpret_cast<const uint8_t*>(uintptr_t(pool[1]));
    for (int i = 0; i < int(pool[2]); i++) {
        if ((flags[i] & 0x80) == 0 && *reinterpret_cast<const uint32_t*>(objects + i * 0x1F44) == 0) {
            reported = true;
            std::fprintf(stderr, "x64 check: ped slot %d (%p, flag 0x%02X) vtable 0 after vf %d of %p (its vtable 0x%08X before)\n", i,
                         static_cast<const void*>(objects + i * 0x1F44), flags[i], slot, object, vtableBefore);
            std::fflush(stderr);
            __debugbreak();
            return;
        }
    }
}
#endif

template <typename R, typename... A>
R VCall(void* object, int slot, A... args) {
#if defined(_M_X64)
    // (bring-up check: after each virtual the world loop calls, no in-use ped with vtable 0)
    uint32_t vtableBefore = *reinterpret_cast<const uint32_t*>(object);
    struct After {
        void* object;
        int slot;
        uint32_t vtable;
        ~After() { PedPoolCheck(object, slot, vtable); }
    } after{object, slot, vtableBefore};
#endif
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[slot]))(object, args...);
}
void DeleteEntity(CEntity* e) {
    VCall<void>(e, 0, 1);
}

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

uint8_t* ModelInfo(const CEntity* e) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(0x00C67738 + e->modelIndex * 4);
}

// (the line tests' common test) it collides, is used, is in this chapter, and isn't
// skipped by the line tests (+0xF4)
bool IsUsedForLineTests(CEntity* e) {
    return e->bUsesCollision && (e->flag1C || e->flag60) && (ModelInfo(e)[0xB] & uint8_t(1 << s_chapter)) && e->flagF4 == 0;
}

// the gun's time step
float ScaledTimeStep() {
    return s_timeStepScale * s_timeStep;
}

// (inlined three times in Process) its Gamebryo transform from its matrix, then its frame
void UpdateNiFrame(CEntity* e) {
    void* object;
    void* frame;
    if (e->rwObject && (object = Field<void*>(e->rwObject, 4)) && (frame = GetLocalTransform(object))) {
        if (!e->matrix) {
            SimpleTransform_UpdateNiTransform(&e->placement, frame);
        } else if (!Entity_GetMatrixLink(e)->attached) {
            void* attached = nullptr;
            if (e->rwObject && Field<void*>(e->rwObject, 4)) {
                attached = GetLocalTransform(Field<void*>(e->rwObject, 4));
            }
            CMatrix* m = Entity_GetMatrixLink(e);
            m->attached = attached;
            Matrix_UpdateRW(m);
        } else {
            Matrix_UpdateRW(Entity_GetMatrixLink(e));
        }
    }
    Entity_UpdateRwFrame(e);
}

// (the moving list's first two passes) removed, or processed
void ProcessMovingEntity(CEntity* e) {
    if (e->bRemoveFromWorld) {
        if (e->GetType() == ENTITY_TYPE_PED && s_playerPed == e) {
            World_Remove(e);
        } else {
            World_Remove(e);
            DeleteEntity(e);
        }
        return;
    }
    VCall<void>(e, 12);  // ProcessControl
    if (e->bIsStatic || e->bIsStaticWaitingForCollision) {
        Physical_RemoveFromMovingList(e);
    }
}

// ---- CWorld -------------------------------------------------------------------------------

// 0x45D500: m * v into a 4-float vector. (The game fills the fourth from uninitialised
// stack; ours leaves it)
float* __cdecl MultiplyMatrixVector4(float* out, const float* m, const float* v) {
    out[0] = m[4] * v[1] + m[0] * v[0] + m[8] * v[2] + m[12];
    out[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13];
    out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
    return out;
}

// 0x45D580: GTA's CWorld::Initialise (the switches off)
void __cdecl Initialise() {
    s_flag76 = 0;
    s_includeFlag1C98Peds = 0;
    s_includeDriverPeds = 0;
    s_secondPass = 0;
    s_processCutsceneOnly = 0;
    s_flag6D = 0;
    s_skipFlagC4 = 0;
    s_skipPropAnims = 0;
    s_skipSomePropAnims = 0;
    s_lineTestFlag = 0;
    s_ignoreEntity = nullptr;
    s_numIgnoredModels = 0;
    Fn_4399E0();
    RemoveExtraScene();
}

// 0x45D600: GTA's CWorld::ClearForRestart
void __cdecl ClearForRestart() {
    if (s_cutsceneFlag) {
        StopCutscene();
    }
    Fn_55BC80(s_hud_C674F0);
    ClearAllProjectiles();
    Fn_4CFDF0();
    Fn_4CFE70();
    Population_Fn49E460(s_population);
    Fn_4A6940(s_obj_C2C1A0);
    Fn_4A7750(s_obj_C2C730);
    KeepLoadingScreenAlive(1, 0);
    Fn_44CD60();
}

// 0x45DE40: out of the world and deleted
void __cdecl RemoveAndDelete(CEntity* e) {
    World_Remove(e);
    if (e) {
        DeleteEntity(e);
    }
}

// 0x45EAE0: a vertical line through (x, y), from z + 50 down to -50, hits one of four
// models' collision (placed through their owners)
uint8_t __cdecl IsUnderSpecialModel(float x, float y, float z) {
    CVector bottom{x, y, -50.0f};
    CVector top{x, y, float(double(z) + 50.0)};
    float line[8];
    ColLine_Set(line, &top, &bottom);
    CColPoint point;
    point.point = {0.0f, 0.0f, 0.0f};
    point.entity = nullptr;
    point.normal = {0.0f, 0.0f, 0.0f};
    point.depth = 3.40282347e+38f;
    float dist = 1.0f;
    uint8_t hit = 0;
    for (int i = 0; i < 4; i++) {
        int model = i == 1 ? s_verticalTestModels[2] : i == 2 ? s_verticalTestModels[1] : i == 3 ? s_verticalTestModels[0] : s_verticalTestModels[3];
        CColModel* col = *reinterpret_cast<re::Ptr32<CColModel>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(0x00C67738 + model * 4) + 0xC);
        if (col->data && col->owner) {
            if (Col_ProcessLineOfSight(line, Placeable_GetMatrix(static_cast<CEntity*>(col->owner)), col, &point, &dist, 1, 0xF, 0)) {
                hit = 1;
                break;
            }
        }
    }
    // (the point's destructor)
    if (point.entity) {
        CleanUpOldReference(point.entity, &point.entity);
    }
    return hit;
}

// 0x45EC50: ambient peds and vehicles that can go within radius (2D) deleted, the
// trails and 0x4CFEF0's things in the sphere, and (flag) every projectile
void __cdecl ClearArea(const CVector* pos, float radius, int clearProjectiles) {
    CPool<uint8_t>* vehicles = s_vehiclePool;
    CPool<uint8_t>* peds = s_pedPool;
    int size = peds->size;
    for (int i = 0; i < size; i++) {
        if (peds->IsFreeSlot(i)) {
            continue;
        }
        CEntity* ped = reinterpret_cast<CEntity*>(peds->objects + peds->elemSize * i);
        if (!ped || !Ped_IsAmbient(ped) || Field<int32_t>(ped, 0x1DD4) != 0) {
            continue;
        }
        CVector p = ped->GetPosition();
        double dx = double(p.x) - pos->x;
        double dy = double(p.y) - pos->y;
        float distSq = float(dx * dx + dy * dy);
        if (distSq < double(radius) * radius) {
            World_Remove(ped);
            DeleteEntity(ped);
        }
    }
    size = vehicles->size;
    for (int i = 0; i < size; i++) {
        if (vehicles->IsFreeSlot(i)) {
            continue;
        }
        CEntity* v = reinterpret_cast<CEntity*>(vehicles->objects + vehicles->elemSize * i);
        if (!v) {
            continue;
        }
        CVector p = v->GetPosition();
        double dx = double(p.x) - pos->x;
        double dy = double(p.y) - pos->y;
        float distSq = float(dx * dx + dy * dy);
        if (distSq < double(radius) * radius && !(Field<uint8_t>(v, 0x33A) & 1) && Vehicle_CanBeCleared(v)) {
            World_Remove(v);
            DeleteEntity(v);
        }
    }
    Fn_4CFEF0(*pos, radius);
    RemoveTrailsInSphere(*pos, radius);
    if (re::Flag(clearProjectiles)) {
        ClearAllProjectiles();
    }
}

// 0x45EEC0: a sphere cast (start, b, c) against a sector list's entities whose bounding
// sphere touches the start's (radius): the nearest hit below *dist
uint8_t __cdecl ProcessSphereCastSectorList(CPtrList* list, const CVector* start, const void* b, float c, float radius, CColPoint* point,
                                            float* dist, re::Ptr32<CEntity>* entity, uint32_t flags, int e, int f) {
    float localDist = *dist;
    uint16_t scanCode = s_scanCode;
    uint8_t hit = 0;
    for (CPtrNode* node = list->first; node;) {
        CEntity* ent = NodeEntity(node);
        node = NodeNext(node);
        if (ent == s_ignoreEntity) {
            continue;
        }
        if (flags & 8) {
            if (s_skipFlagC4 && ent->flagC4) {
                continue;
            }
            if (ent->GetType() == ENTITY_TYPE_PROPANIM) {
                if (s_skipPropAnims) {
                    continue;
                }
                if (s_skipSomePropAnims) {
                    uint8_t* info = ModelInfo_GetAnimInfo(ModelInfo(ent));
                    if (info && !info[0x85]) {
                        continue;
                    }
                }
            }
        }
        if ((flags & 4) && ent->GetType() == ENTITY_TYPE_PED && !s_includeFlag1C98Peds && Field<uint8_t>(ent, 0x1C98)) {
            continue;
        }
        if (!IsUsedForLineTests(ent) || ent->scanCode == scanCode) {
            continue;
        }
        ent->scanCode = scanCode;
        CVector center = *start;
        if (!Entity_GetIsTouching(ent, &center, radius)) {
            continue;
        }
        CColModel* col = GetColModel(ent);
        if (Col_ProcessSphereCast(start, b, c, Placeable_GetMatrix(ent), col, point, &localDist, flags & ~0x100u, e, f) &&
            localDist < *dist) {
            *dist = localDist;
            *entity = ent;
            hit = 1;
        }
    }
    return hit;
}

// 0x45F0E0: the peds of a sector list (not yet scanned) given to CPed::MoveAgainstVehicle (flags: 2
// none, 4 peds)
void __cdecl ForPedsInSectorList(CPtrList* list, uint8_t flags, CEntity* entity, CColModel* col, CMatrix* matrix, CVector* offset,
                                  float radius) {
    uint16_t scanCode = s_scanCode;
    for (CPtrNode* node = list->first; node;) {
        CEntity* ent = NodeEntity(node);
        node = NodeNext(node);
        if (!IsUsedForLineTests(ent) || ent->scanCode == scanCode) {
            continue;
        }
        ent->scanCode = scanCode;
        if (!(flags & 2) && (flags & 4) && ent->GetType() == ENTITY_TYPE_PED) {
            Ped_MoveAgainstVehicle(ent, entity, col, matrix, offset, radius);
        }
    }
}

// 0x45F230 (thiscall): a prop anim whose animation is one of three
uint8_t __fastcall HasSpecialAnim(CEntity* self, re::Edx) {
    void* anim = *reinterpret_cast<re::Ptr32<void>*>(ModelInfo_GetAnimInfo(ModelInfo(self)) + 0x80);
    // (handles, made and released around each compare)
    re::Ptr32<void> h1;
    re::Ptr32<void> h2;
    re::Ptr32<void> h3;
    int made = 0;
    uint8_t result = 1;
    AnimHandle_Set(&h1, anim);
    made = 1;
    if (s_animHandle1B65504 != h1) {
        AnimHandle_Set(&h2, anim);
        made = 3;
        if (s_animHandle1B65508 != h2) {
            AnimHandle_Set(&h3, anim);
            made = 7;
            result = s_animHandle1B6550C == h3;
        }
    }
    if (made & 4) {
        AnimHandle_Release(&h3);
    }
    if (made & 2) {
        AnimHandle_Release(&h2);
    }
    AnimHandle_Release(&h1);
    return result;
}

// 0x45F2F0: GTA's CWorld::ShutDown: every entity in the sectors and the big buildings
// deleted, the lists emptied
void __cdecl ShutDown() {
    static const int order[5] = {SECTOR_BUILDINGS, SECTOR_VEHICLES, SECTOR_PEDS, SECTOR_OBJECTS, SECTOR_DUMMIES};
    for (CSector* s = s_sectors; s < s_sectors + 36 * 36; s++) {
        for (int list : order) {
            for (CPtrNode* node = s->lists[list].first; node;) {
                CPtrNode* next = NodeNext(node);
                World_Remove(NodeEntity(node));
                // (the node is read again after Remove freed it)
                CEntity* e = NodeEntity(node);
                node = next;
                if (e) {
                    DeleteEntity(e);
                }
            }
        }
        PtrList_Flush(&s->lists[SECTOR_BUILDINGS]);
        PtrList_Flush(&s->lists[SECTOR_DUMMIES]);
    }
    for (CPtrNode* node = s_bigBuildings.first; node;) {
        CPtrNode* next = NodeNext(node);
        CEntity* e = NodeEntity(node);
        node = next;
        if (e) {
            DeleteEntity(e);
        }
    }
    PtrList_Flush(&s_bigBuildings);
    int i = 0;
    for (CSector* s = s_sectors; s < s_sectors + 36 * 36; s++, i++) {
        if (s->lists[SECTOR_BUILDINGS].first) {
            Sprintf(s_debugText, s_buildingListNotEmpty, i % 36, i / 36);
            PtrList_Flush(&s->lists[SECTOR_BUILDINGS]);
        }
        if (s->lists[SECTOR_DUMMIES].first) {
            Sprintf(s_debugText, s_dummyListNotEmpty, i % 36, i / 36);
            PtrList_Flush(&s->lists[SECTOR_DUMMIES]);
        }
    }
    PtrList_Flush(&s_movingList);
    if (CColModel* col = s_worldColModel) {
        ColModel_RemoveCollisionVolumes(col);
        ColModel_Delete(col);
    }
}

// 0x45F670: GTA's ProcessLineOfSightSectorList: the nearest hit below *dist in a list
// (ignoreSomeObjects: simple models with flag 0x80000 skipped, through flags bit 7)
uint8_t __cdecl ProcessLineOfSightSectorList(CPtrList* list, const void* line, CColPoint* point, float* dist, re::Ptr32<CEntity>* entity,
                                             uint32_t flags, int pedsOwnModel) {
    CPtrNode* node = list->first;
    if (!node) {
        return 0;
    }
    uint8_t skipFlag80000 = (flags >> 7) & 1;
    float localDist = *dist;
    uint8_t hit = 0;
    uint8_t firstIsPed1C98 = 0;
    uint8_t firstIsPedDriver = 0;
    if (s_includeFlag1C98Peds && NodeEntity(node)->GetType() == ENTITY_TYPE_PED) {
        firstIsPed1C98 = 1;
    }
    if (s_includeDriverPeds && NodeEntity(node)->GetType() == ENTITY_TYPE_PED) {
        firstIsPedDriver = 1;
    }
    uint16_t scanCode = s_scanCode;
    do {
        CEntity* ent = NodeEntity(node);
        node = NodeNext(node);
        if (ent->scanCode == scanCode || ent == s_ignoreEntity) {
            continue;
        }
        if (s_numIgnoredModels) {
            bool ignored = false;
            for (int i = 0; i < s_numIgnoredModels; i++) {
                if (s_ignoredModels[i] == ent->modelIndex) {
                    ignored = true;
                    break;
                }
            }
            if (ignored) {
                continue;
            }
        }
        if (!IsUsedForLineTests(ent) && !firstIsPed1C98 && !firstIsPedDriver) {
            continue;
        }
        ent->scanCode = scanCode;
        if (s_skipFlagC4 && ent->flagC4) {
            continue;
        }
        if (skipFlag80000) {
            uint8_t* mi = ModelInfo(ent);
            if (mi[8] == 1 && ((Field<uint32_t>(mi, 0x28) >> 19) & 1)) {
                continue;
            }
        }
        CColModel* col;
        if (ent->GetType() == ENTITY_TYPE_PED) {
            if (!Entity_Fn41C540(ent)) {
                if (!(firstIsPed1C98 && Field<uint8_t>(ent, 0x1C98))) {
                    if (!firstIsPedDriver) {
                        continue;
                    }
                    uint8_t* driving = Field<uint8_t*>(ent, 0x1554);
                    if (!driving || Field<int32_t>(driving, 0x46C) != 1) {
                        continue;
                    }
                }
            }
            if (re::Flag(pedsOwnModel) && !ent->flagA4 && ent->modelIndex != s_specialPedModel && !Ped_Fn4473E0(ent)) {
                Ped_SetupLineColModel(ent);
                col = s_pedLineColModel;
            } else {
                col = GetColModel(ent);
            }
        } else {
            if (!Entity_Fn41C540(ent)) {
                continue;
            }
            col = GetColModel(ent);
        }
        if (!col) {
            continue;
        }
        if (Col_ProcessLineOfSightPed(line, Placeable_GetMatrix(ent), col, point, &localDist, flags, s_lineTestFlag) &&
            localDist < *dist) {
            *dist = localDist;
            *entity = ent;
            hit = 1;
        }
    } while (node);
    return hit;
}

// 0x45F940: the same for a list of buildings (no switches): the point keeps the entity
uint8_t __cdecl ProcessVerticalLineSectorList(CPtrList* list, const void* line, CColPoint* point, float* dist, re::Ptr32<CEntity>* entity,
                                              uint32_t flags, int g) {
    float localDist = *dist;
    uint16_t scanCode = s_scanCode;
    uint8_t hit = 0;
    for (CPtrNode* node = list->first; node;) {
        CEntity* ent = NodeEntity(node);
        node = NodeNext(node);
        if (!IsUsedForLineTests(ent) || ent->scanCode == scanCode) {
            continue;
        }
        ent->scanCode = scanCode;
        CColModel* col = GetColModel(ent);
        if (Col_ProcessLineOfSight(line, Placeable_GetMatrix(ent), col, point, &localDist, flags & ~0x100u, 0xF, g) && localDist < *dist) {
            *dist = localDist;
            point->depth = localDist;
            *entity = ent;
            if (ent != point->entity) {
                if (point->entity) {
                    CleanUpOldReference(point->entity, &point->entity);
                }
                point->entity = ent;
                RegisterReference(ent, &point->entity);
            }
            hit = 1;
        }
    }
    return hit;
}

// 0x460670: GTA's GetIsLineOfSightSectorListClear
uint8_t __cdecl GetIsLineOfSightSectorListClear(CPtrList* list, const void* line, uint32_t flags, int ignoreSomeObjects) {
    uint16_t scanCode = s_scanCode;
    for (CPtrNode* node = list->first; node;) {
        CEntity* ent = NodeEntity(node);
        node = NodeNext(node);
        if (!IsUsedForLineTests(ent) || ent == s_ignoreEntity) {
            continue;
        }
        uint8_t* mi = ModelInfo(ent);
        if (s_skipFlagC4 && ent->flagC4) {
            continue;
        }
        if (re::Flag(ignoreSomeObjects) && mi[8] == 1 && ((Field<uint32_t>(mi, 0x28) >> 19) & 1)) {
            continue;
        }
        if (ent->scanCode == scanCode) {
            continue;
        }
        ent->scanCode = scanCode;
        CColModel* col = GetColModel(ent);
        if (Col_TestLineOfSight(line, Placeable_GetMatrix(ent), col, flags & ~0x100u, s_lineTestFlag)) {
            return 0;
        }
    }
    return 1;
}

// (0x45E770, a chunk of Process) peds fallen below the map: back on the nearest path
// node, or deleted
void ProcessFallenPeds() {
    CPool<uint8_t>* pool = s_pedPool;
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->IsFreeSlot(i)) {
            continue;
        }
        CEntity* ped = reinterpret_cast<CEntity*>(pool->objects + pool->elemSize * i);
        if (!ped) {
            continue;
        }
        CVector* posRef = &ped->GetPosition();
        CVector pos = *posRef;
        if (!(s_fallThroughZ > pos.z)) {
            continue;
        }
        if (Field<uint8_t>(ped, 0x1570) == 1 && Field<int32_t>(ped, 0x1310) != 13) {
            DeleteEntity(ped);  // (not taken out of the world first)
            continue;
        }
        uint8_t* node = Paths_FindNodeClosestToCoors(s_paths, 0, *posRef, 1, 1000000.0f, 0, 0, 0, 0);
        if (!node) {
            continue;
        }
        float x = float(Field<int16_t>(node, 8) * 0.125);
        float y = float(Field<int16_t>(node, 0xA) * 0.125);
        float z = float(Field<int16_t>(node, 0xC) * 0.125);
        CVector to{x, y, float(double(z) + 2.0)};
        VCall<void>(ped, 14, to.x, to.y, to.z);  // Teleport (the floats)
        static_cast<CPhysical*>(ped)->moveSpeed = {0.0f, 0.0f, 0.0f};
    }
}

// (0x45E900, a chunk of Process) the same for vehicles: the player's, a driven one, or
// a parked one back on a node; the others deleted
void ProcessFallenVehicles() {
    CPool<uint8_t>* pool = s_vehiclePool;
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->IsFreeSlot(i)) {
            continue;
        }
        CEntity* v = reinterpret_cast<CEntity*>(pool->objects + pool->elemSize * i);
        if (!v) {
            continue;
        }
        CVector pos = v->GetPosition();
        if (!(s_fallThroughZ > pos.z)) {
            continue;
        }
        uint8_t status = Field<uint8_t>(v, 0x290);
        bool rescue = (status == 2 && v->flag50 == 0) || v == FindPlayerVehicle(0);
        if (!rescue) {
            void* seats = reinterpret_cast<uint8_t*>(v) + 0x1C4;
            if (Seats_GetOccupant(seats, 0) && Field<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 13) {
                rescue = true;
            }
        }
        if (!rescue) {
            status = Field<uint8_t>(v, 0x290);
            if (status == 1 || status == 3) {
                World_Remove(v);
                DeleteEntity(v);
            }
            continue;
        }
        uint8_t* node = Paths_FindNodeClosestToCoors(s_paths, 0, pos, 0, 1000000.0f, 0, 0, 0, 0);
        if (!node) {
            continue;
        }
        float x = float(Field<int16_t>(node, 8) * 0.125);
        float y = float(Field<int16_t>(node, 0xA) * 0.125);
        float z = float(Field<int16_t>(node, 0xC) * 0.125);
        CVector to{x, y, float(double(z) + 3.0)};
        VCall<void>(v, 14, to.x, to.y, to.z);  // Teleport (the floats)
        static_cast<CPhysical*>(v)->moveSpeed = {0.0f, 0.0f, 0.0f};
    }
}

CVector* CameraPosition() {
    return reinterpret_cast<CVector*>(reinterpret_cast<uint8_t*>(GetLocalTransform(Camera_GetObject(s_rwCamera))) + 0x24);
}

// (Process) the prop anims in the sectors around the camera (49 units)
void ProcessPropAnimsAroundCamera() {
    CVector* cam = CameraPosition();
    float right = float(double(cam->x) + 49.0);
    float bottom = float(double(cam->y) - 49.0);
    float top = float(double(cam->y) + 49.0);
    float left = float(double(cam->x) - 49.0);
    float f = float(int(left / 50.0 + 18.0));
    int minX = int(f > 0.0f ? f : 0.0f);
    f = float(int(bottom / 50.0 + 18.0));
    int minY = int(f > 0.0f ? f : 0.0f);
    f = float(int(right / 50.0 + 18.0));
    int maxX = int(f < 35.0f ? f : 35.0f);
    f = float(int(top / 50.0 + 18.0));
    int maxY = int(f < 35.0f ? f : 35.0f);

    uint16_t scanCode;
    if (s_scanCode >= 0xFFFF) {
        ClearScanCodes();
        scanCode = 1;
    } else {
        scanCode = uint16_t(s_scanCode + 1);
    }
    s_scanCode = scanCode;
    CVector* cameraPos = CameraPosition();
    Player_Fn49AEB0(s_playerPed);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            CPtrNode* node = s_sectors[y * 36 + x].lists[SECTOR_OBJECTS].first;
            while (node) {
                CEntity* e = NodeEntity(node);
                node = NodeNext(node);
                if (!e || e->GetType() != ENTITY_TYPE_PROPANIM || e->scanCode == scanCode) {
                    continue;
                }
                e->scanCode = scanCode;
                uint8_t* mi = ModelInfo(e);
                if (Entity_GetObjectInfo(e)) {
                    PropAnim_Fn4EB200(e);
                }
                if (Field<int32_t>(e, 0x228) == 0) {
                    uint16_t anim = Field<uint16_t>(e, 0x22C);
                    if (anim != 0xFFFF) {
                        int id = int16_t(anim) + 0x5D41;
                        if (GetStreamingInfo(id)[4] != 2 && GetStreamingInfo(id)[4] != 1) {
                            RequestModel(id, 0x1F);
                            continue;
                        }
                        if (GetStreamingInfo(id)[4] != 1) {
                            continue;
                        }
                    }
                    PropAnim_Fn4EEFA0(e);
                }
                if (e->modelIndex == 0x2762) {
                    if (PropAnim_GetPoolElement(e)) {
                        if (Field<int32_t>(e, 0x190)) {
                            PropAnim_Fn4EC3A0(e);
                        } else {
                            PropAnim_Fn4EBE80(e);
                        }
                    }
                } else if (PropAnim_HasSpecialAnim(e)) {
                    Fn_72C9C0(s_obj_20C8A00, e);
                }
                PropAnim_Fn4ED410(e, cameraPos);
                if (Field<int32_t>(e, 0x188) == 0 && Field<int32_t>(e, 0x228) != 0) {
                    if (Field<int32_t>(e, 0x1A8) == 0) {
                        PropAnim_Fn4EF630(e, ScaledTimeStep());
                    } else {
                        VCall<void>(e, 20);
                    }
                    Field<int32_t>(e, 0x1A8) = 0;
                } else if (e->rwObject) {
                    if (ModelInfo_Fn520440(ModelInfo(e))) {
                        PropAnim_Fn4EEBA0(e);
                    } else {
                        re::Ptr32<void> handle;
                        AnimHandle_Set(&handle, *reinterpret_cast<re::Ptr32<void>*>(ModelInfo_GetAnimInfo(mi) + 0x80));
                        bool same = s_animHandle1B65518 == handle;
                        AnimHandle_Release(&handle);
                        if (same) {
                            PropAnim_Fn4EBCB0(e);
                        }
                    }
                }
            }
        }
    }
}

// 0x45FAB0: GTA's CWorld::Process: the moving entities (removed, processed, collided,
// moved), the prop anims around the camera, the peds; every 8 frames the peds (or
// vehicles) that fell out of the map
void __cdecl Process() {
    if ((s_frameCounter & 0x3F) == 0) {
        PruneAllReferencesInWorld();
    }
    bool cutsceneOnly = false;
    if (s_cutsceneRunning) {
        for (re::Ptr32<CEntity>* p = s_cutsceneObjects; p < s_cutsceneObjects + 30; p++) {
            CEntity* e = *p;
            if (e && Field<int32_t>(e, 0x178)) {
                VCall<void>(e, 19);
                VCall<void>(e, 12);
                Entity_Fn443AA0(e);
                Entity_UpdateRwFrame(e);
            }
        }
        int n = Cutscene_NumPropAnims();
        for (int i = 0; i < n; i++) {
            CEntity* e = Cutscene_GetPropAnim(i);
            if (!e) {
                continue;
            }
            PropAnim_SetCurrent(e);
            void* controller = reinterpret_cast<uint8_t*>(e) + 0x128;
            if (controller) {
                ActionController_Update(controller, ScaledTimeStep());
            }
            VCall<void>(e, 19);
            VCall<void>(e, 12);
            Entity_Fn443AA0(e);
            Entity_UpdateRwFrame(e);
        }
        cutsceneOnly = s_processCutsceneOnly != 0;
    }
    if (!cutsceneOnly) {
        // the peds' positions before they move
        CPool<uint8_t>* peds = s_pedPool;
        for (int i = peds->size; i != 0;) {
            i--;
            if (peds->IsFreeSlot(i)) {
                continue;
            }
            CEntity* ped = reinterpret_cast<CEntity*>(peds->objects + peds->elemSize * i);
            if (ped && ped->flagA8 == 0) {
                Field<CVector>(ped, 0x1564) = ped->GetPosition();
            }
        }
        s_forceProcessControl = 1;
        s_nextMovingNode = s_movingList.first;
        while (s_nextMovingNode) {
            CEntity* e = NodeEntity(s_nextMovingNode);
            s_nextMovingNode = NodeNext(s_nextMovingNode);
            ProcessMovingEntity(e);
        }
        s_secondPass = 1;
        s_nextMovingNode = s_movingList.first;
        while (s_nextMovingNode) {
            CEntity* e = NodeEntity(s_nextMovingNode);
            s_nextMovingNode = NodeNext(s_nextMovingNode);
            if (e->flag40) {
                ProcessMovingEntity(e);
            }
        }
        s_nextMovingNode = nullptr;
        s_secondPass = 0;
        s_forceProcessControl = 0;
        for (CPtrNode* node = s_movingList.first; node;) {
            CEntity* e = NodeEntity(node);
            CheckLive(node, e);
            node = NodeNext(node);
            if (!VCall<uint8_t>(e, 21) && !e->bRemoveFromWorld) {
                VCall<void>(e, 19);  // ProcessCollision
            }
        }
        for (CPtrNode* node = s_movingList.first; node;) {
            CEntity* e = NodeEntity(node);
            CheckLive(node, e);
            node = NodeNext(node);
            if (!VCall<uint8_t>(e, 21) && !e->flagC4) {
                VCall<void>(e, 13, 0);  // ProcessShift
                UpdateNiFrame(e);
            }
        }
        for (CPtrNode* node = s_movingList.first; node;) {
            CEntity* e = NodeEntity(node);
            CheckLive(node, e);
            node = NodeNext(node);
            if (e->GetType() == ENTITY_TYPE_PED &&
                (Field<int32_t>(e, 0x1554) || (Field<int32_t>(e, 0x1310) == 13 && Field<int32_t>(e, 0x1E94)))) {
                Ped_Fn476DD0(e);
                UpdateNiFrame(e);
            }
        }
        for (CPtrNode* node = s_movingList.first; node;) {
            CEntity* e = NodeEntity(node);
            CheckLive(node, e);
            node = NodeNext(node);
            if (!VCall<uint8_t>(e, 21) && !e->bRemoveFromWorld) {
                VCall<void>(e, 20);
            }
        }
        PropAnims_Fn4E9110(s_propAnimMgr);
    }

    ProcessPropAnimsAroundCamera();

    if (s_processCutsceneOnly) {
        Fn_6C0A00(s_obj_20C4910);
        return;
    }
    PropAnims_Fn4E92B0(s_propAnimMgr);
    Fn_5EC500(s_obj_D13730);
    for (CPtrNode* node = s_movingList.first; node;) {
        CEntity* e = NodeEntity(node);
        CheckLive(node, e);
        node = NodeNext(node);
        if (VCall<uint8_t>(e, 21)) {
            VCall<void>(e, 19);
            VCall<void>(e, 20);
        }
    }
    CPool<uint8_t>* peds = s_pedPool;
    for (int i = peds->size; i != 0;) {
        i--;
        if (peds->IsFreeSlot(i)) {
            continue;
        }
        uint8_t* ped = peds->objects + peds->elemSize * i;
        if (ped) {
            Ped_Fn477E30(ped);
        }
    }
    for (CPtrNode* node = s_movingList.first; node;) {
        CEntity* e = NodeEntity(node);
        CheckLive(node, e);
        node = NodeNext(node);
        if (VCall<uint8_t>(e, 21)) {
            VCall<void>(e, 13, 1);
            UpdateNiFrame(e);
        }
    }
    Fn_4D4590();
    WorldPlayer_Fn446BC0(s_worldPlayer);
    uint32_t phase = s_frameCounter & 7;
    if (phase == 1) {
        ProcessFallenPeds();
    } else if (phase == 5) {
        ProcessFallenVehicles();
    }
}

} // namespace

#define WORLDPROC_C05_REVERSED(name, address, fn) REVERSED(name, address, fn, nullptr, nullptr, re::Mode::Original)

WORLDPROC_C05_REVERSED("MultiplyMatrixVector4", 0x0045D500, RE_BODY(&MultiplyMatrixVector4));
WORLDPROC_C05_REVERSED("CWorld::Initialise", 0x0045D580, RE_BODY(&Initialise));
WORLDPROC_C05_REVERSED("CWorld::ClearForRestart", 0x0045D600, RE_BODY(&ClearForRestart));
WORLDPROC_C05_REVERSED("CWorld::RemoveAndDelete", 0x0045DE40, RE_BODY(&RemoveAndDelete));
WORLDPROC_C05_REVERSED("CWorld::IsUnderSpecialModel", 0x0045EAE0, RE_BODY(&IsUnderSpecialModel));
WORLDPROC_C05_REVERSED("CWorld::ClearArea", 0x0045EC50, RE_BODY(&ClearArea));
WORLDPROC_C05_REVERSED("CWorld::ProcessSphereCastSectorList", 0x0045EEC0, RE_BODY(&ProcessSphereCastSectorList));
WORLDPROC_C05_REVERSED("CWorld::ForPedsInSectorList", 0x0045F0E0, RE_BODY(&ForPedsInSectorList));
WORLDPROC_C05_REVERSED("CPropAnim::HasSpecialAnim", 0x0045F230, RE_BODY(&HasSpecialAnim));
WORLDPROC_C05_REVERSED("CWorld::ShutDown", 0x0045F2F0, RE_BODY(&ShutDown));
WORLDPROC_C05_REVERSED("CWorld::ProcessLineOfSightSectorList", 0x0045F670, RE_BODY(&ProcessLineOfSightSectorList));
WORLDPROC_C05_REVERSED("CWorld::ProcessVerticalLineSectorList", 0x0045F940, RE_BODY(&ProcessVerticalLineSectorList));
WORLDPROC_C05_REVERSED("CWorld::Process", 0x0045FAB0, RE_BODY(&Process));
WORLDPROC_C05_REVERSED("CWorld::GetIsLineOfSightSectorListClear", 0x00460670, RE_BODY(&GetIsLineOfSightSectorListClear));
