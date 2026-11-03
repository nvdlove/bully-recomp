// A ped's prop finder (at CPed+0x34C, 0xC bytes; symbols call it JitterMovementInfo):
// each update it scans the sectors around the ped for objects to pick up (type 4) and
// prop anims to use (type 6), keeps the nearest in the ped's +0x444 (registered, with a
// value at +0x448 and the distance at +0x44C), and for the player shows the button
// prompt ("USE_PROP", "USE_PICKFLOWERS", ...). 0x496A70..0x499330.
//
// The ped's fields used here (not in Ped.h's CPed):
//   +0x438 EntityRef  the prop in use (0x478760 copies +0x444 into it); +0x43C its value
//   +0x444 EntityRef  the nearest usable prop, +0x448 its value, +0x44C float its distance
//   +0x450 PropCandidate[+0x4C8]  props found by others (0x497B00 searches them)
//   +0x548 CEntity*   a prop held (nothing is looked for then)

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Ped.h"
#include "reversed.h"

namespace {

template <typename T>
re::Field<T>& F(const void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uintptr_t>(base) + offset);
}
uint8_t EntityType(const void* entity) {
    return F<uint8_t>(entity, 0x108) & 7;
}
int16_t ModelOf(const void* entity) {
    return F<int16_t>(entity, 0x10E);
}
const ped::CVector* EntityPos(const void* entity) {
    uint8_t* matrix = F<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const ped::CVector*>(matrix + 0x30) : &F<ped::CVector>(entity, 4);
}
// a virtual call with the object as `this`
template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    using Fn = R(__thiscall*)(void*, re::Arg<A>...);
    return re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index])(object, args...);
}

// What the finder keeps of a prop (a registered pointer, a value (a node id of the
// prop's model, 0x520380), the distance)
struct PropCandidate {
    re::Ptr32<CEntity> entity;  // +0 (registered)
    int32_t value;    // +4
    float dist;       // +8
};
static_assert(sizeof(PropCandidate) == 0xC);

struct PedPropFinder {
    uint8_t busy;      // +0 (cleared when Update is done)
    uint8_t pad1[3];
    re::Ptr32<CPed> ped;         // +4
    uint8_t canUse;    // +8 (Update: set, cleared when the prompt is for a prop)
    uint8_t pad9[3];
};
static_assert(sizeof(PedPropFinder) == 0xC);

// A track that carries a prop's use text (0x20 on the stack: Track's 0x18, the text)
struct PropUseTextTrack {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0 0x911978
    uint8_t unknown4[0x14];   // +4 (Track's)
    re::Ptr32<const char> text;         // +0x18 (a string object: 0x66BEA0 its constructor)
    uint32_t unknown1C;       // +0x1C
};
static_assert(offsetof(PropUseTextTrack, text) == 0x18);
static_assert(sizeof(PropUseTextTrack) == 0x20);

// the model info table (CModelInfo::ms_modelInfoPtrs)
GAME_ARRAY(uint8_t*, PF_modelInfos, 0x00C67738);
GAME_VAR(CPed*, PF_player, 0x00C1AEA8);                  // FindPlayerPed's
GAME_VAR(uint16_t, PF_scanCode, 0x00C1AE88);             // CWorld::ms_nCurrentScanCode
GAME_VAR(uint32_t*, PF_ptrNodes, 0x00C0F788);            // the sectors' packed list nodes
inline uint8_t* const PF_sectorObjectLists = reinterpret_cast<uint8_t*>(0x00C1B17C);  // ms_aSectors[0].lists[1]
GAME_VAR(uint8_t, PF_cutsceneRunning, 0x020C5BE4);
GAME_VAR(uint8_t, PF_onlyShowMissionTags, 0x020C8B9C);
GAME_VAR(int32_t, PF_currentMission, 0x020C8B98);
GAME_VAR(int32_t, PF_gameMode, 0x00C3CCF8);             // (2: nothing is offered)
GAME_VAR(uint8_t, PF_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, PF_chapterFlags, 0x00A147EC);       // by chapter (2: no flowers)
GAME_VAR(float, PF_aimScale, 0x00CF311C);
// hashed names (the prop anims' kinds, their uses)
GAME_VAR(uint32_t, PF_hash1B65504, 0x01B65504);
GAME_VAR(uint32_t, PF_hash1B65518, 0x01B65518);
GAME_VAR(uint32_t, PF_hash1B654D4, 0x01B654D4);
GAME_VAR(uint32_t, PF_hash1B65AC0, 0x01B65AC0);
// models
GAME_VAR(int32_t, PF_modelHighlight1, 0x00BF446C);
GAME_VAR(int32_t, PF_modelHighlight2, 0x00BF4468);
GAME_VAR(int32_t, PF_modelHighlight3, 0x00BF4464);
GAME_VAR(int32_t, PF_modelHighlight4, 0x00BF3EA0);
GAME_VAR(int32_t, PF_modelFlower1, 0x00BF4254);
GAME_VAR(int32_t, PF_modelFlower2, 0x00BF4250);
GAME_VAR(int32_t, PF_itemFlowers, 0x00BF4278);
GAME_VAR(int32_t, PF_weaponFlowers, 0x00BF4274);
GAME_VAR(int32_t, PF_modelBF445C, 0x00BF445C);
GAME_VAR(int32_t, PF_modelBF4460, 0x00BF4460);
GAME_VAR(int32_t, PF_modelBF4140, 0x00BF4140);
inline void* const PF_highlight = reinterpret_cast<void*>(0x020C8AE8);
inline void* const PF_objA45A30 = reinterpret_cast<void*>(0x00A45A30);
inline void* const PF_objC2AB70 = reinterpret_cast<void*>(0x00C2AB70);
inline void* const PF_missionMgr = reinterpret_cast<void*>(0x020C3CA0);
// the input names (0x5F4800 on the intelligence's +0x258 or a prop's +0x128)
inline const void* const PF_input1B654E0 = reinterpret_cast<const void*>(0x01B654E0);
inline const void* const PF_input1B654F4 = reinterpret_cast<const void*>(0x01B654F4);
inline const void* const PF_input1B654DC = reinterpret_cast<const void*>(0x01B654DC);
inline const void* const PF_input1B654D8 = reinterpret_cast<const void*>(0x01B654D8);
inline const void* const PF_input1B652C8 = reinterpret_cast<const void*>(0x01B652C8);

GAME_FN(void, __thiscall, PF_SetHighlight, 0x005D7F10, void* slot, CEntity* entity);
GAME_FN(void, __cdecl, PF_SetPrompt, 0x005143D0, const char* text);   // (nullptr: none)
GAME_FN(const char*, __cdecl, PF_GetPrompt, 0x00514510);
GAME_FN(uint8_t, __thiscall, PF_Fn50DF50, 0x0050DF50, void* object);
GAME_FN(uint8_t, __thiscall, PF_Fn4CE690, 0x004CE690, CPed* ped);
GAME_FN(uint8_t, __thiscall, PF_Input, 0x005F4800, void* inputs, const void* name, int a, int b);
GAME_FN(float, __cdecl, PF_PadValue, 0x00738840, int a, int b);
GAME_FN(uint8_t, __cdecl, PF_DodgeballActive, 0x00683A30);
GAME_FN(uint8_t*, __thiscall, PF_AnimInfo, 0x005201D0, uint8_t* modelInfo);
GAME_FN(void, __thiscall, PF_PropAnimSetCurrent, 0x004ED940, CEntity* prop);
GAME_FN(uint32_t*, __thiscall, PF_HashConstruct, 0x005FA870, uint32_t* hash, uint32_t value);
GAME_FN(void, __thiscall, PF_HashDestruct, 0x0049A650, uint32_t* hash);
GAME_FN(uint32_t*, __thiscall, PF_UseGetName, 0x005FA9B0, void* use, uint32_t* out);
GAME_FN(void, __thiscall, PF_UseGetText, 0x005FA940, void* use, PropUseTextTrack* track);
GAME_FN(void, __thiscall, PF_Track6099C0, 0x006099C0, PropUseTextTrack* track);
GAME_FN(void, __thiscall, PF_TrackConstruct, 0x0061A880, void* track);
GAME_FN(void*, __thiscall, PF_StringConstruct, 0x0066BEA0, void* string);
GAME_FN(void, __thiscall, PF_TrackDestruct, 0x00655D50, void* track);
GAME_FN(void, __cdecl, PF_Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t*, __cdecl, PF_GetWeaponInfo, 0x0051B450, int model);
GAME_FN(int, __cdecl, PF_ItemsGetCount, 0x00541E00, int item);
GAME_FN(uint8_t, __cdecl, PF_RequestWeaponModels, 0x004D9790, int weapon, int flags);
GAME_FN(ped::CVector*, __cdecl, PF_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a, const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PF_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a, const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PF_VectorScale, 0x00412350, ped::CVector* out, const ped::CVector* v, float scale);
GAME_FN(float, __thiscall, PF_MagnitudeSqr, 0x0040D9F0, const ped::CVector* v);
GAME_FN(float, __thiscall, PF_Magnitude, 0x0040DFB0, const ped::CVector* v);
GAME_FN(float, __thiscall, PF_Normalise, 0x004294A0, ped::CVector* v);
GAME_FN(uint8_t, __thiscall, PF_Fn46EFB0, 0x0046EFB0, void* object, CPed* ped, const ped::CVector* pos, int flag);
GAME_FN(void, __cdecl, PF_RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, PF_CleanUpReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __thiscall, PF_RefCopy, 0x00478760, void* ref, const void* from);
GAME_FN(void*, __thiscall, PF_RefSet, 0x00478550, void* ref, CEntity* entity);
GAME_FN(void, __thiscall, PF_EntityRefSet, 0x00471300, ped::EntityRef* ref, CEntity* entity, int value);
GAME_FN(uint32_t, __thiscall, PF_GetObjectValue, 0x00467B80, CEntity* entity);
GAME_FN(uint8_t, __thiscall, PF_MIIsKind1, 0x00520440, uint8_t* modelInfo);
GAME_FN(uint8_t, __thiscall, PF_MIIsKind2, 0x00520460, uint8_t* modelInfo);
GAME_FN(uint8_t, __thiscall, PF_MIIsKind3, 0x00520480, uint8_t* modelInfo);
GAME_FN(int, __cdecl, PF_ModelNode, 0x00520380, int index);
GAME_FN(int, __cdecl, PF_NumModelNodes, 0x005191C0);   // (5)
GAME_FN(uint8_t, __cdecl, PF_GetNodeTransform, 0x00467D80, CEntity* entity, int node, float* transform, int flag);
GAME_FN(uint8_t, __thiscall, PF_PropStateIsKind2Near, 0x004EA400, void* state);
GAME_FN(uint8_t, __thiscall, PF_PropAnimGet17C, 0x004EA3A0, CEntity* prop);
GAME_FN(uint8_t, __thiscall, PF_Fn48BFD0, 0x0048BFD0, void* object3BC, int model);
GAME_FN(void, __thiscall, PF_Fn48C800, 0x0048C800, void* object3BC, CEntity* entity, int node, float dist);
GAME_FN(uint8_t, __thiscall, PF_Is437C90, 0x00437C90, CEntity* entity);
GAME_FN(ped::CVector*, __thiscall, PF_GetBoundCentre, 0x00466C70, CEntity* entity, ped::CVector* out);
GAME_FN(ped::CVector*, __thiscall, PF_GetRight, 0x004736F0, CEntity* entity, ped::CVector* out);
GAME_FN(ped::CVector*, __thiscall, PF_GetForward, 0x004154A0, CPed* ped, ped::CVector* out);
GAME_FN(float, __cdecl, PF_ModelHeight, 0x004EB7E0, CEntity* entity);
GAME_FN(CEntity*, __cdecl, PF_GetEntityInPool, 0x0044A3E0, int pool, int slot);
GAME_FN(void, __cdecl, PF_ClearScanCodes, 0x0045D840);
GAME_FN(uint8_t*, __thiscall, PF_MissionMgrGetStats, 0x006AA7C0, void* mgr, int mission);
GAME_FN(void*, __thiscall, PF_MissionMgr6AA680, 0x006AA680, void* mgr);
GAME_FN(uint8_t, __thiscall, PF_Mission6AA410, 0x006AA410, void* mission, int id);
// (the target selection's, 0x4969A0)
GAME_FN(uint8_t, __cdecl, PF_IsWithinAngle, 0x004969A0, const ped::CVector* a, const ped::CVector* b, float degrees, int negate);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, PF_IsMissionProp, 0x00496B50, CEntity* prop);
GAME_FN(uint8_t, __thiscall, PF_IsKind2, 0x00497E10, CEntity* prop);
GAME_FN(uint8_t, __thiscall, PF_IsKind1504, 0x00497E30, CEntity* prop);
GAME_FN(void, __thiscall, PF_ConsiderObject, 0x00497A10, PedPropFinder* finder, CEntity* object,
        const ped::CVector* from, const ped::CVector* at, PropCandidate* best, float* bestDist, uint8_t* found);
GAME_FN(void, __thiscall, PF_ConsiderProp, 0x00497FE0, PedPropFinder* finder, CEntity* prop, uint8_t* pedModelInfo,
        float range, float zRange, int checkZ, int anyPed, const ped::CVector* from, const ped::CVector* forward,
        const ped::CVector* at, PropCandidate* best, float* bestDist, uint8_t* found);
GAME_FN(uint8_t, __thiscall, PF_Scan, 0x00498B40, PedPropFinder* finder, float range, int checkZ, float zRange, int anyPed);
GAME_FN(PropUseTextTrack*, __thiscall, PF_PropUseTextTrackConstruct, 0x00496A70, PropUseTextTrack* track);

uint8_t* ModelInfoOf(const void* entity) {
    return PF_modelInfos[ModelOf(entity)];
}
// the models that get the highlight when offered
bool IsHighlightModel(int model) {
    return model == PF_modelHighlight1 || model == PF_modelHighlight2 || model == PF_modelHighlight3 ||
           model == PF_modelHighlight4;
}
// (inlined in the game) a registered pointer set
void SetRef(re::Ptr32<CEntity>* slot, CEntity* entity) {
    if (entity != *slot) {
        if (*slot) {
            PF_CleanUpReference(*slot, slot);
        }
        *slot = entity;
        if (entity) {
            PF_RegisterReference(entity, slot);
        }
    }
}
// the player's (+0x1488 and +0x1470): the angle checks take the other way round
int Reversed1488(CPed* ped) {
    return F<int32_t>(ped, 0x1488) != 0 && ped->unknown1470 != 0 ? 1 : 0;
}

// ---------------------------------------------------------------------------------
// PropUseTextTrack

// 0x496A70: the constructor
PropUseTextTrack* __fastcall PropUseTextTrack_Construct(PropUseTextTrack* self) {
    PF_TrackConstruct(self);
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00911978);
    PF_StringConstruct(&self->text);
    return self;
}

// 0x496A90 vf1: the destructor (flags 1: and freed)
PropUseTextTrack* __fastcall PropUseTextTrack_Destroy(PropUseTextTrack* self, re::Edx, int flags) {
    PF_TrackDestruct(self);
    if (flags & 1) {
        PF_Free(self);
    }
    return self;
}

// ---------------------------------------------------------------------------------
// Props

// 0x496B50: a prop anim to offer for the mission (only mission tags shown: its +0x23C's
// +0x3D is 2; otherwise the current mission is on or has been passed)
uint8_t __fastcall IsMissionProp(CEntity* self) {
    if (PF_onlyShowMissionTags) {
        return F<uint8_t>(F<uint8_t*>(self, 0x23C), 0x3D) == 2;
    }
    uint8_t* stats = PF_MissionMgrGetStats(PF_missionMgr, PF_currentMission);
    int id = PF_currentMission;
    void* mission = PF_MissionMgr6AA680(PF_missionMgr);
    if (PF_Mission6AA410(mission, id)) {
        return 1;
    }
    return F<uint16_t>(stats, 2) != 0;
}

// 0x497E10: its model info's kind 2
uint8_t __fastcall IsKind2(CEntity* self) {
    return PF_MIIsKind2(ModelInfoOf(self));
}

// 0x497E30: its anim's name is 0x1B65504's
uint8_t __fastcall IsKind1504(CEntity* self) {
    uint8_t* anim = PF_AnimInfo(ModelInfoOf(self));
    uint32_t hash;
    PF_HashConstruct(&hash, F<uint32_t>(anim, 0x80));
    uint8_t result = PF_hash1B65504 == hash;
    PF_HashDestruct(&hash);
    return result;
}

// ---------------------------------------------------------------------------------
// PedPropFinder (thiscall, as __fastcall)

// 0x497570: the player's prompt for the prop in +0x444 (busy set meanwhile... cleared)
void __fastcall Update(PedPropFinder* self) {
    PF_SetHighlight(PF_highlight, nullptr);
    CPed* ped = self->ped;
    self->canUse = 1;
    CEntity* held = F<CEntity*>(ped, 0x548);
    if (held) {
        if (IsHighlightModel(ModelOf(held))) {
            PF_SetHighlight(PF_highlight, held);
        }
        PF_SetPrompt(nullptr);
        self->busy = 0;
        return;
    }
    bool blocked = PF_cutsceneRunning || !PF_Fn50DF50(PF_objA45A30) || PF_Fn4CE690(self->ped) ||
                   PF_Input(self->ped->intelligence + 0x258, PF_input1B654E0, 1, 1) ||
                   PF_Input(self->ped->intelligence + 0x258, PF_input1B654F4, 1, 1) ||
                   PF_Input(self->ped->intelligence + 0x258, PF_input1B654DC, 1, 1) || PF_PadValue(10, 0) > 0.0f;
    if (blocked) {
        // (the game's repe cmpsb: 12 bytes)
        if (std::memcmp(PF_GetPrompt(), "BOXING_INST", 12) == 0) {
            self->busy = 0;
            return;
        }
        if (PF_DodgeballActive()) {
            self->busy = 0;
            return;
        }
        CEntity* prop = ped->entityRef444.entity;
        if (prop && EntityType(prop) == 6 && IsHighlightModel(ModelOf(prop))) {
            PF_SetHighlight(PF_highlight, prop);
        }
        PF_SetPrompt(nullptr);
        self->busy = 0;
        return;
    }
    CEntity* prop = ped->entityRef444.entity;
    if (!prop || self->ped->pedType != 13 || PF_gameMode == 2) {
        self->busy = 0;
        return;
    }
    if (EntityType(prop) == 6) {
        uint8_t* anim = PF_AnimInfo(ModelInfoOf(prop));
        PF_PropAnimSetCurrent(prop);
        bool constructed = false;
        bool use;
        uint32_t hash;
        if (PF_Input(&F<uint8_t>(prop, 0x128), PF_input1B654D8, 1, 1)) {
            use = false;
        } else if (F<int32_t>(prop, 0x17C) == 0) {
            use = true;
        } else {
            PF_HashConstruct(&hash, F<uint32_t>(anim, 0x80));
            constructed = true;
            use = PF_hash1B65518 == hash;
        }
        if (constructed) {
            PF_HashDestruct(&hash);
        }
        if (use) {
            self->canUse = 0;
            if (!anim || F<uint8_t>(anim, 0x8E) != 0) {
                PF_SetPrompt("USE_PROP");
            }
            if (IsHighlightModel(ModelOf(prop))) {
                PF_SetHighlight(PF_highlight, prop);
            }
            void* uses = F<void*>(prop, 0x230);
            if (uses) {
                int pedKind = VCall<int>(self->ped, 16);
                void* list = VCall<void*>(uses, 19, pedKind);
                if (list) {
                    int count = VCall<int>(list, 16);
                    for (int i = 0; i < count; i++) {
                        void* item = VCall<void*>(list, 18, i);
                        uint32_t name;
                        bool match = *PF_UseGetName(item, &name) == PF_hash1B65AC0;
                        PF_HashDestruct(&name);
                        if (match) {
                            PropUseTextTrack track;
                            PF_PropUseTextTrackConstruct(&track);
                            PF_Track6099C0(&track);
                            PF_UseGetText(item, &track);
                            PF_SetPrompt(track.text);
                            PF_TrackDestruct(&track);
                            self->busy = 0;
                            return;
                        }
                    }
                }
            }
        }
        self->busy = 0;
        return;
    }
    if (EntityType(prop) == 4) {
        // flowers: near enough, fewer than the weapon's +0x34 of them held
        if (PF_chapterFlags[PF_chapter] == 2 || !VCall<uint8_t>(prop, 31)) {
            self->busy = 0;
            return;
        }
        int model = ModelOf(prop);
        if (model != PF_modelFlower1 && model != PF_modelFlower2) {
            self->busy = 0;
            return;
        }
        uint8_t* info = PF_GetWeaponInfo(PF_modelFlower1);
        if (!info || PF_ItemsGetCount(PF_itemFlowers) >= F<int32_t>(info, 0x34)) {
            self->busy = 0;
            return;
        }
        ped::CVector d;
        PF_VectorSub(&d, EntityPos(self->ped), EntityPos(prop));
        float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
        if (!(double(distSq) <= 100.0) || !PF_RequestWeaponModels(PF_weaponFlowers, 0) || 1.0f < distSq) {
            self->busy = 0;
            return;
        }
        PF_SetPrompt("USE_PICKFLOWERS");
    }
    self->busy = 0;
}

// 0x497A10: an object (type 4) nearer than *bestDist that the ped may reach (the player
// always) becomes the best
void __fastcall ConsiderObject(PedPropFinder* self, re::Edx, CEntity* object, const ped::CVector* from,
                               const ped::CVector* at, PropCandidate* best, float* bestDist, uint8_t* found) {
    float dx = from->x - at->x;
    float dy = from->y - at->y;
    float dz = from->z - at->z;
    float lengthSq = float(double(dx) * dx + double(dy) * dy + double(dz) * dz);
    float dist = std::sqrt(std::fabs(lengthSq));
    if (self->ped->pedType != 13 && PF_Fn46EFB0(PF_objC2AB70, self->ped, at, 0)) {
        return;
    }
    if (!(dist < *bestDist)) {
        return;
    }
    SetRef(&best->entity, object);
    best->dist = dist;
    best->value = 0;
    *bestDist = dist;
    *found = 1;
}

// 0x497B00: whether the ped's +0x444 (or one of the props others found, within
// maxDist) is the one wanted (by model, or by name: its object value, or a prop anim of
// kind 0x1B654D4 that's usable); it's then put in +0x438
uint8_t __fastcall FindWanted(PedPropFinder* self, re::Edx, uint32_t name, float maxDist, int model, int anyType,
                              int ignore17C, int ignore180) {
    CPed* ped = self->ped;
    CEntity* prop = ped->entityRef444.entity;
    uint8_t result = 0;
    if (prop) {
        bool rejected = false;
        if (EntityType(prop) == 6) {
            uint32_t hash;
            PF_HashConstruct(&hash, F<uint32_t>(PF_AnimInfo(ModelInfoOf(prop)), 0x80));
            bool other = PF_hash1B65518 != hash;
            PF_HashDestruct(&hash);
            if (other) {
                bool ok = !(F<int32_t>(prop, 0x17C) != 0 && !re::Flag(ignore17C) && self->ped == PF_player);
                if (F<int32_t>(prop, 0x180) != 0 && !re::Flag(ignore180) && self->ped != PF_player) {
                    rejected = true;
                } else if (!ok) {
                    rejected = true;
                }
            }
        }
        if (!rejected && (maxDist == 0.0f || F<float>(self->ped, 0x44C) < maxDist)) {
            do {
                if (!re::Flag(anyType) && EntityType(prop) != 6) {
                    break;
                }
                if (EntityType(prop) == 6) {
                    PF_PropAnimSetCurrent(prop);
                    if (PF_Input(&F<uint8_t>(prop, 0x128), PF_input1B652C8, 1, 1)) {
                        break;
                    }
                }
                if (model != -1) {
                    if (model != ModelOf(prop)) {
                        break;
                    }
                } else if (name != 0) {
                    uint32_t hash;
                    PF_HashConstruct(&hash, name);
                    bool match = PF_hash1B654D4 == hash && VCall<uint8_t>(prop, 31);
                    PF_HashDestruct(&hash);
                    if (!match) {
                        result = name == PF_GetObjectValue(prop);
                        if (!result) {
                            break;
                        }
                    }
                }
                CPed* p = self->ped;
                PF_RefCopy(&F<uint8_t>(p, 0x438), &F<uint8_t>(p, 0x444));
                F<int32_t>(self->ped, 0x43C) = F<int32_t>(self->ped, 0x448);
                return 1;
            } while (false);
        }
    }
    if (maxDist == 0.0f) {
        return result;
    }
    for (uint32_t i = 0; i < F<uint32_t>(self->ped, 0x4C8); i++) {
        PropCandidate* candidate = &F<PropCandidate>(self->ped, 0x450 + int(i) * 0xC);
        CEntity* entity = candidate->entity;
        if (!entity) {
            continue;
        }
        if (!re::Flag(anyType) && EntityType(entity) != 6) {
            continue;
        }
        if (!(candidate->dist < maxDist)) {
            continue;
        }
        if (model != -1 && model != ModelOf(entity)) {
            continue;
        }
        uint32_t hash;
        PF_HashConstruct(&hash, name);
        bool skip = PF_hash1B654D4 == hash && !VCall<uint8_t>(candidate->entity, 31);
        PF_HashDestruct(&hash);
        if (skip) {
            continue;
        }
        if (PF_GetObjectValue(candidate->entity) == name) {
            PF_RefCopy(&F<uint8_t>(self->ped, 0x438), candidate);
            F<int32_t>(self->ped, 0x43C) = candidate->value;
            return 1;
        }
    }
    return result;
}

// 0x497FE0: a prop anim (type 6): the nearest of its nodes (or itself) in reach, in
// front of the player, becomes the best; the others are told to +0x3BC (0x48C800). An
// unusable one that is the ped's +0x444 is dropped from it
void __fastcall ConsiderProp(PedPropFinder* self, re::Edx, CEntity* prop, uint8_t* pedModelInfo, float range,
                             float zRange, int checkZ, int anyPed, const ped::CVector* from,
                             const ped::CVector* forward, const ped::CVector* at, PropCandidate* best,
                             float* bestDist, uint8_t* found) {
    // a node's NiTransform (3x3 rotation, translation, scale)
    float transform[13];
    transform[12] = 1.0f;
    uint8_t* modelInfo = ModelInfoOf(prop);
    uint8_t claimed = 0;
    if (PF_PropStateIsKind2Near(&F<uint8_t>(prop, 0x398))) {
        return;
    }
    if (self->ped->pedType == 13) {
        bool ok = true;
        if (PF_IsKind1504(prop) && !PF_IsMissionProp(prop)) {
            ok = false;
        }
        PF_PropAnimSetCurrent(prop);
        if (PF_Input(&F<uint8_t>(prop, 0x128), PF_input1B652C8, 1, 1) || !ok) {
            ped::EntityRef* ref = &self->ped->entityRef444;
            if (prop != ref->entity) {
                return;
            }
            if (ref->entity) {
                PF_CleanUpReference(ref->entity, &ref->entity);
                ref->entity = nullptr;
            }
            ref->value = 0;
            return;
        }
        if (PF_MIIsKind1(modelInfo) && PF_PropAnimGet17C(prop)) {
            claimed = 1;
        }
        if (ModelOf(prop) == PF_modelBF445C) {
            PF_PropAnimSetCurrent(prop);
            if (!PF_Input(&F<uint8_t>(prop, 0x128), PF_input1B652C8, 1, 1)) {
                ped::CVector d;
                PF_VectorSub(&d, at, from);
                float lengthSq = PF_MagnitudeSqr(&d);
                float radius = F<float>(PF_AnimInfo(modelInfo), 0x78);
                float radiusSq = radius * radius;
                if (lengthSq < radiusSq) {
                    PF_RefSet(best, prop);
                    best->value = PF_ModelNode(0);
                    best->dist = lengthSq;
                    *bestDist = lengthSq;
                    *found = 1;
                    return;
                }
            }
        }
    } else if (!re::Flag(anyPed)) {
        bool ok = !PF_Fn46EFB0(PF_objC2AB70, self->ped, at, 0);
        if (F<int32_t>(prop, 0x18C) != 0 && self->ped->seatState != 2) {
            ok = false;
        } else if (ok) {
            if (F<uint8_t>(PF_AnimInfo(modelInfo), 0x7C) == 0) {
                ok = false;
            } else if (pedModelInfo[0x50] == 0 && F<uint8_t>(PF_AnimInfo(modelInfo), 0x7D) == 0) {
                ok = false;
            } else if (pedModelInfo[0x50] != 0 && F<uint8_t>(PF_AnimInfo(modelInfo), 0x7E) == 0) {
                ok = false;
            } else if (PF_Fn48BFD0(self->ped->unknown3BC, ModelOf(prop))) {
                ok = false;
            }
        }
        PF_PropAnimSetCurrent(prop);
        if (PF_Input(&F<uint8_t>(prop, 0x128), PF_input1B652C8, 1, 1) || !ok) {
            if (prop != self->ped->entityRef444.entity) {
                return;
            }
            PF_EntityRefSet(&self->ped->entityRef444, nullptr, 0);
            return;
        }
    }

    PF_PropAnimSetCurrent(prop);
    void* uses = F<void*>(prop, 0x138);
    PF_PropAnimSetCurrent(prop);
    if (PF_Input(&F<uint8_t>(prop, 0x128), PF_input1B652C8, 1, 1)) {
        return;
    }
    if (uses) {
        int pedKind = VCall<int>(self->ped, 16);
        if (!VCall<void*>(uses, 19, pedKind)) {
            return;
        }
    }
    float reach = range;
    if (range == 0.0f) {
        reach = F<float>(PF_AnimInfo(modelInfo), 0x78);
    }

    float dist;
    if (!PF_MIIsKind3(modelInfo) && !claimed) {
        int model = ModelOf(prop);
        if (model != PF_modelBF4460 && model != PF_modelBF4140) {
            // by its nodes
            for (int i = 0; i < PF_NumModelNodes(); i++) {
                int node = PF_ModelNode(i);
                if (!PF_GetNodeTransform(prop, node, transform, 0)) {
                    continue;
                }
                ped::CVector nodePos = {transform[9], transform[10], transform[11]};
                ped::CVector d;
                PF_VectorSub(&d, from, &nodePos);
                if (re::Flag(checkZ) && zRange < std::fabs(d.z)) {
                    continue;
                }
                float nodeDist = PF_Magnitude(&d);
                if (!(nodeDist < reach) || !(nodeDist < *bestDist)) {
                    PF_Fn48C800(self->ped->unknown3BC, prop, node, nodeDist);
                    continue;
                }
                // the node's forward tip, seen from behind the ped
                ped::CVector tip = {transform[1] + transform[9], transform[4] + transform[10],
                                    transform[7] + transform[11]};
                ped::CVector back, eye, toTip;
                PF_VectorScale(&back, forward, PF_aimScale);
                PF_VectorSub(&eye, from, &back);
                PF_VectorSub(&toTip, &tip, &eye);
                PF_Normalise(&toTip);
                if (!PF_Is437C90(prop) && !PF_IsKind2(prop)) {
                    if (self->ped->pedType == 13 &&
                        !PF_IsWithinAngle(&toTip, forward, 60.0f, Reversed1488(self->ped))) {
                        continue;
                    }
                } else {
                    // (two more directions are made here and not used: kept, for their calls)
                    ped::CVector tmp;
                    const ped::CVector* pedPos = EntityPos(self->ped);
                    ped::CVector centre;
                    centre.x = PF_GetBoundCentre(prop, &tmp)->x;
                    centre.y = PF_GetBoundCentre(prop, &tmp)->y;
                    centre.z = pedPos->z;
                    ped::CVector right;
                    right.x = PF_GetRight(prop, &tmp)->x;
                    right.y = PF_GetRight(prop, &tmp)->y;
                    right.z = PF_GetRight(prop, &tmp)->z;
                    PF_Normalise(&right);
                    float half = float(double(PF_ModelHeight(prop)) * 0.5);
                    ped::CVector a, b, c, e, unused1, unused2;
                    PF_VectorScale(&a, forward, 0.15f);
                    PF_VectorSub(&b, from, &a);
                    PF_VectorScale(&c, &right, half);
                    PF_VectorSub(&e, &centre, &c);
                    PF_VectorSub(&unused1, &e, &b);
                    PF_Normalise(&unused1);
                    PF_VectorScale(&a, forward, 0.15f);
                    PF_VectorSub(&b, from, &a);
                    PF_VectorScale(&c, &right, half);
                    PF_VectorAdd(&e, &centre, &c);
                    PF_VectorSub(&unused2, &e, &b);
                    PF_Normalise(&unused2);
                    if (!PF_IsWithinAngle(&toTip, forward, 60.0f, Reversed1488(self->ped))) {
                        continue;
                    }
                }
                PF_RefSet(best, prop);
                best->dist = nodeDist;
                best->value = node;
                *found = 1;
                *bestDist = nodeDist;
            }
            return;
        }
        // by its position (reach 2 with a partner, else 1.2, when none is given)
        if (range == 0.0f) {
            reach = self->ped->partner ? 2.0f : 1.2f;
        }
        ped::CVector d;
        PF_VectorSub(&d, at, from);
        dist = PF_Magnitude(&d);
        PF_Normalise(&d);
        if (self->ped->pedType == 13 && !PF_IsWithinAngle(&d, forward, 60.0f, Reversed1488(self->ped))) {
            return;
        }
        if (!(dist < reach)) {
            PF_Fn48C800(self->ped->unknown3BC, prop, PF_ModelNode(0), dist);
            return;
        }
        PF_RefSet(best, prop);
        best->value = PF_ModelNode(0);
    } else {
        // (kind 3, or claimed) by its position, facing it
        ped::CVector d = {at->x - from->x, at->y - from->y, at->z - from->z};
        dist = PF_Magnitude(&d);
        PF_Normalise(&d);
        bool facing = false;
        if (!PF_MIIsKind1(ModelInfoOf(prop))) {
            facing = PF_IsWithinAngle(&d, forward, 60.0f, Reversed1488(self->ped)) != 0;
        }
        if (!facing) {
            if (!PF_MIIsKind1(ModelInfoOf(prop))) {
                return;
            }
            if (!PF_IsWithinAngle(&d, forward, 60.0f, Reversed1488(self->ped))) {
                return;
            }
        }
        if (!(dist < reach)) {
            PF_Fn48C800(self->ped->unknown3BC, prop, 0, dist);
            return;
        }
        PF_RefSet(best, prop);
        best->value = 0;
    }
    best->dist = dist;
    *bestDist = dist;
    *found = 1;
}

// 0x498B40: the objects and prop anims within 10 of the ped (its sectors' object lists,
// each entity once), the nearest into the ped's +0x444 (a prop anim before an object,
// unless the player's +0x1758 says so); whether any was found
uint8_t __fastcall Scan(PedPropFinder* self, re::Edx, float range, int checkZ, float zRange, int anyPed) {
    uint8_t* pedModelInfo = ModelInfoOf(self->ped);
    uint8_t found = 0;
    CPed* ped = self->ped;
    ped::CVector tmp, tmp2, tmp3;
    ped::CVector forward;
    forward.x = PF_GetForward(self->ped, &tmp)->x;
    forward.y = PF_GetForward(ped, &tmp2)->y;
    forward.z = PF_GetForward(ped, &tmp3)->z;
    PF_Normalise(&forward);
    ped::CVector pos = *EntityPos(ped);
    uint8_t objectsFirst = 0;
    // (the distances start uninitialised in the game; only read once set)
    PropCandidate bestProp = {nullptr, 0, 0.0f};
    PropCandidate bestObject = {nullptr, 0, 0.0f};
    float bestObjectDist = 100.0f;
    float bestPropDist = 100.0f;
    // (the game also tests ped+0x174C against null)
    if (ped->pedType == 13) {
        void* state = F<void*>(ped, 0x1758);
        if (state && VCall<uint8_t>(state, 4)) {
            objectsFirst = 1;
        }
    }
    const ped::CVector* at = EntityPos(self->ped);
    float minX = float(double(at->x) - 10.0);
    float maxX = float(double(at->x) + 10.0);
    float minY = float(double(at->y) - 10.0);
    float maxY = float(double(at->y) + 10.0);
    float v = float(int(double(minX) / 50.0 + 18.0));
    int x0 = int(v > 0.0f ? v : 0.0f);
    v = float(int(double(minY) / 50.0 + 18.0));
    int y0 = int(v > 0.0f ? v : 0.0f);
    v = float(int(double(maxX) / 50.0 + 18.0));
    int x1 = int(v < 35.0f ? v : 35.0f);
    v = float(int(double(maxY) / 50.0 + 18.0));
    int y1 = int(v < 35.0f ? v : 35.0f);

    if (PF_scanCode >= 0xFFFF) {
        PF_ClearScanCodes();
        PF_scanCode = 1;
    } else {
        PF_scanCode++;
    }
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            uint32_t* node = *reinterpret_cast<re::Ptr32<uint32_t>*>(PF_sectorObjectLists + (y * 36 + x) * 20);
            while (node) {
                uint32_t bits = *node;
                CEntity* entity = PF_GetEntityInPool(int(bits & 0xF), int((bits >> 4) & 0x3FFF));
                node = (bits >> 18) == 0x3FFF ? nullptr : PF_ptrNodes + (bits >> 18);
                if (F<uint16_t>(entity, 0x10A) == PF_scanCode) {
                    continue;
                }
                F<uint16_t>(entity, 0x10A) = PF_scanCode;
                // (the game copies a fourth float from uninitialised stack with it)
                ped::CVector entityPos = *EntityPos(entity);
                uint8_t type = EntityType(entity);
                if (type == 4) {
                    if (!VCall<uint8_t>(entity, 31) || VCall<uint8_t>(entity, 30)) {
                        continue;
                    }
                    if (F<int32_t>(entity, 0x104) != 0 && self->ped->pedType != 13) {
                        continue;
                    }
                    PF_ConsiderObject(self, entity, &pos, &entityPos, &bestObject, &bestObjectDist, &found);
                } else if (type == 6) {
                    if (F<int32_t>(entity, 0x1B8) != 0 || F<void*>(entity, 0x230) == nullptr) {
                        continue;
                    }
                    PF_ConsiderProp(self, entity, pedModelInfo, range, zRange, checkZ, anyPed, &pos, &forward,
                                    &entityPos, &bestProp, &bestPropDist, &found);
                }
            }
        }
    }

    ped::EntityRef* nearest = &self->ped->entityRef444;
    if (bestProp.entity && !objectsFirst) {
        SetRef(&nearest->entity, bestProp.entity);
        nearest->value = bestProp.value;
        F<float>(nearest, 8) = bestProp.dist;
    } else if (bestObject.entity) {
        SetRef(&nearest->entity, bestObject.entity);
        nearest->value = bestObject.value;
        F<float>(nearest, 8) = bestObject.dist;
    } else if (self->ped->pedType == 13) {
        ped::EntityRef* ref = &self->ped->entityRef444;
        if (ref->entity) {
            PF_CleanUpReference(ref->entity, &ref->entity);
            ref->entity = nullptr;
        }
        self->ped->entityRef444.value = 0;
    }
    // (the game compares each with 0xC2A8E8 first; both ways do the same)
    if (bestObject.entity) {
        PF_CleanUpReference(bestObject.entity, &bestObject.entity);
    }
    if (bestProp.entity) {
        PF_CleanUpReference(bestProp.entity, &bestProp.entity);
    }
    return found;
}

// 0x499330: the scan, unless the ped holds a prop (the prompt cleared), its +0x444 is a
// held object (+0x209 bit 0), or the player has +0x13C4 (then its +0x1E98 is the one)
uint8_t __fastcall FindNearest(PedPropFinder* self, re::Edx, float range, int checkZ, float zRange, int anyPed) {
    if (F<CEntity*>(self->ped, 0x548)) {
        PF_SetPrompt(nullptr);
        return 1;
    }
    F<uint32_t>(self->ped, 0x4C8) = 0;
    CPed* ped = self->ped;
    CEntity* prop = ped->entityRef444.entity;
    if (prop && EntityType(prop) == 4 && (F<uint8_t>(prop, 0x209) & 1)) {
        return 0;
    }
    if (ped->pedType == 13 && F<int32_t>(ped, 0x13C4) != 0) {
        PF_RefSet(&ped->entityRef444, F<CEntity*>(ped, 0x1E98));
        self->ped->entityRef444.value = 0;
        return 1;
    }
    return PF_Scan(self, range, checkZ, zRange, anyPed);
}

} // namespace

#define C08P3_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P3_REVERSED("PropUseTextTrack::Construct", 0x00496A70, &PropUseTextTrack_Construct);
C08P3_REVERSED("PropUseTextTrack::Destroy", 0x00496A90, &PropUseTextTrack_Destroy);
C08P3_REVERSED("CPropAnim::IsMissionProp", 0x00496B50, &IsMissionProp);
C08P3_REVERSED("CPropAnim::IsKind2", 0x00497E10, &IsKind2);
C08P3_REVERSED("CPropAnim::IsKind1504", 0x00497E30, &IsKind1504);
C08P3_REVERSED("PedPropFinder::Update", 0x00497570, &Update);
C08P3_REVERSED("PedPropFinder::ConsiderObject", 0x00497A10, &ConsiderObject);
C08P3_REVERSED("PedPropFinder::FindWanted", 0x00497B00, &FindWanted);
C08P3_REVERSED("PedPropFinder::ConsiderProp", 0x00497FE0, &ConsiderProp);
C08P3_REVERSED("PedPropFinder::Scan", 0x00498B40, &Scan);
C08P3_REVERSED("PedPropFinder::FindNearest", 0x00499330, &FindNearest);
