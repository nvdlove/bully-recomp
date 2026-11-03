// The ped's action controllers (its intelligence, CPed +0x2E0, 0x5C0 bytes): three
// action controllers (+0x258 main, +0x290 second, +0x2C8 facial; 0x38 each) that run the
// action-tree nodes of three contexts (+0x300, +0x358, +0x3B0; 0x58 each). 0x484010 ..
// 0x485200.
//
// A context: +0 its controller, +4 the ped's anim (CPed +0x114), +8 the ped (a registered
// reference), +0x10 / +0x14 shared state (+0x408 / +0x474 of the intelligence), +0x1D a
// flag, +0x38 the ped's +0x157C, +0x40 the node, +0x44 an index. The node names of the
// main and second acts (+0x4C0 / +0x540) and their act files (+0x500 / +0x580) are 0x40
// chars each; +0x4B0 / +0x4B4 / +0x4BC the nodes found for the three.

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct V3 {
    float x, y, z;
};

using re::At;

V3* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<V3*>(matrix + 0x30) : &At<V3>(entity, 4);
}

GAME_VAR(uint8_t, PAC_globalSleep, 0x00C2B108);
GAME_VAR(V3, PAC_facingAxis, 0x009E33DC);          // (0, 0, 1)
GAME_VAR(float, PAC_headTurnDotLimit, 0x00C2B1D4);
GAME_VAR(float, PAC_headTurnDotRange, 0x00C2B1D0);
GAME_VAR(void*, PAC_cameraSource, 0x00C3CCF4);
GAME_VAR(void*, PAC_defaultSecondNode, 0x01B649B4);
GAME_VAR(void*, PAC_defaultFacialNode, 0x01B649BC);
GAME_VAR(uint8_t, PAC_skipZeroStep, 0x00C2B1CC);
// the models without the default facial node
GAME_VAR(int32_t, PAC_noFacialModel1, 0x00BF42D4);
GAME_VAR(int32_t, PAC_noFacialModel2, 0x00BF42D0);
GAME_VAR(int32_t, PAC_noFacialModel3, 0x00BF42CC);
GAME_VAR(int32_t, PAC_noFacialModel4, 0x00BF42DC);
inline void* const PAC_actFiles = reinterpret_cast<void*>(0x00D02850);
inline void* const PAC_obj_B9DAB8 = reinterpret_cast<void*>(0x00B9DAB8);
inline const char* const PAC_playerFacial = reinterpret_cast<const char*>(0x0090D1FC);   // "/Global/PlayerFacial"

// the action controllers'
GAME_FN(void, __thiscall, PAC_ControllerStop, 0x005F40A0, void* controller);
GAME_FN(void, __thiscall, PAC_ControllerPlay, 0x005F45F0, void* controller, void* instance);
GAME_FN(void, __thiscall, PAC_ControllerUpdate, 0x005F4B00, void* controller, float dt);
GAME_FN(void, __thiscall, PAC_Controller5F4110, 0x005F4110, void* controller, float, int);
GAME_FN(void, __thiscall, PAC_ControllerConstruct, 0x005F3E50, void* controller);
GAME_FN(void, __thiscall, PAC_ControllerDestruct, 0x005F40F0, void* controller);
GAME_FN(void, __thiscall, PAC_ContextSetNode, 0x005F3830, void* context, void* node);
// the act files: the node by name, whether a file is loaded, loading one
GAME_FN(void*, __cdecl, PAC_ActTreeFind, 0x005F5B10, const char* name, uintptr_t);
GAME_FN(uint8_t, __thiscall, PAC_ActFileIsLoaded, 0x005DBFC0, void* files, const char* file);
GAME_FN(void, __cdecl, PAC_ActFileLoad, 0x005F6DB0, const char* file, uintptr_t, uintptr_t);
GAME_FN(uint8_t*, __thiscall, PAC_PedGetModelInfo, 0x00477DC0, void* ped);
inline char* __cdecl PAC_Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(void, __thiscall, PAC_Obj684CF0, 0x00684CF0, void* self);
GAME_FN(void, __cdecl, PAC_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, PAC_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, PAC_Empty49A650, 0x0049A650, void* self);
// the intelligence's own (its base at +0)
GAME_FN(void, __thiscall, PAC_Intel5F3A40, 0x005F3A40, void* self);
GAME_FN(void, __thiscall, PAC_IntelBaseConstruct, 0x005F3BC0, void* self);
GAME_FN(void, __thiscall, PAC_Intel5F3880, 0x005F3880, void* self, float dt);
GAME_FN(void, __thiscall, PAC_Shared408Construct, 0x005F3C10, void* shared);
GAME_FN(void, __thiscall, PAC_Shared474Construct, 0x005F3B70, void* shared);
GAME_FN(void, __thiscall, PAC_Shared474Update, 0x005F3750, void* shared, float dt);
GAME_FN(void, __thiscall, PAC_Obj4EF630, 0x004EF630, void* self, float dt);
// the ped's
GAME_FN(V3*, __thiscall, PAC_PedGetForward, 0x004154A0, void* ped, V3* out);
GAME_FN(V3*, __thiscall, PAC_PedGetVec188C, 0x00484040, void* ped, V3* out);
GAME_FN(V3*, __thiscall, PAC_PedGetVec1898, 0x00484070, void* ped, V3* out);
GAME_FN(float, __thiscall, PAC_Normalise, 0x00414E10, V3* v);
GAME_FN(V3*, __cdecl, PAC_VectorSub2, 0x00414BA0, V3* out, const V3* a, const V3* b);
GAME_FN(void*, __cdecl, PAC_Camera6C82F0, 0x006C82F0, void* source);
GAME_FN(uint8_t*, __cdecl, PAC_Camera6C9A90, 0x006C9A90, void* camera);
// (this file's, called through the game's address)
GAME_FN(void, __thiscall, PAC_ContextReplay, 0x00484010, void* context);
GAME_FN(void*, __cdecl, PAC_LoadAndFind, 0x00484280, const char* file, const char* node);
GAME_FN(void, __thiscall, PAC_UpdateMain, 0x00484190, uint8_t* self, float dt);
GAME_FN(void, __thiscall, PAC_Update484210, 0x00484210, uint8_t* self);
GAME_FN(void, __thiscall, PAC_UpdateSecond, 0x004845E0, uint8_t* self, float dt);
GAME_FN(void, __thiscall, PAC_UpdateHead, 0x00484660, uint8_t* self, float dt);
GAME_FN(uint8_t, __thiscall, PAC_IsFacialVisible, 0x00484910, uint8_t* self);
GAME_FN(void, __thiscall, PAC_UpdateFacial, 0x00484DF0, uint8_t* self, float dt);
GAME_FN(void, __thiscall, PAC_UpdateControllers, 0x004851A0, uint8_t* self, float dt);

// (inlined): the context's node started again on its controller
void Replay(uint8_t* context) {
    void* node = At<void*>(context, 0x40);
    if (node) {
        At<int32_t>(context, 0x44) = 0;
        void* instance = reinterpret_cast<void*(__thiscall*)(void*, void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(node))[0x4C / 4]))(node, context);
        PAC_ControllerPlay(At<void*>(context, 0), instance);
    }
}

void* NodePlay(void* node, void* context) {
    return reinterpret_cast<void*(__thiscall*)(void*, void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(node))[0x4C / 4]))(node, context);
}

// (inlined): an entity reference set (the old one dropped, the new one registered)
void SetRef(re::Ptr32<void>* ref, void* entity) {
    if (entity != *ref) {
        if (*ref) {
            PAC_CleanUpOldReference(*ref, ref);
        }
        *ref = entity;
        PAC_RegisterReference(entity, ref);
    }
}

// (inlined 0x484280 with its result stored): the node, loading its file first when that
// isn't loaded (nullptr when the file was loaded and the node still isn't there)
void* FindLoading(const char* node, const char* file) {
    void* found = nullptr;
    if (!PAC_ActFileIsLoaded(PAC_actFiles, file)) {
        PAC_ActFileLoad(file, 0, 0);
        found = PAC_ActTreeFind(node, 0);
    }
    return found;
}

} // namespace

namespace c07pac {

// 0x484010 (thiscall): the context's node started again from its start
void __fastcall ContextReplay(uint8_t* self, re::Edx) {
    Replay(self);
}

// 0x484040 (CPed, thiscall): +0x188C
V3* __fastcall PedGetVec188C(uint8_t* self, re::Edx, V3* out) {
    out->x = At<float>(self, 0x188C);
    out->y = At<float>(self, 0x1890);
    out->z = At<float>(self, 0x1894);
    return out;
}

// 0x484070 (CPed, thiscall): +0x1898
V3* __fastcall PedGetVec1898(uint8_t* self, re::Edx, V3* out) {
    out->x = At<float>(self, 0x1898);
    out->y = At<float>(self, 0x189C);
    out->z = At<float>(self, 0x18A0);
    return out;
}

// 0x4840A0: the act names emptied
void __fastcall ClearActNames(uint8_t* self, re::Edx) {
    At<char>(self, 0x4C0) = 0;
    At<char>(self, 0x500) = 0;
    At<char>(self, 0x540) = 0;
    At<char>(self, 0x580) = 0;
}

// 0x4840C0: the three controllers stopped (second, main, facial)
void __fastcall StopControllers(uint8_t* self, re::Edx) {
    PAC_ControllerStop(self + 0x290);
    PAC_ControllerStop(self + 0x258);
    PAC_ControllerStop(self + 0x2C8);
}

// 0x4840F0: the nodes found started again
void __fastcall ReplayAll(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x4B0)) {
        Replay(self + 0x300);
    }
    if (At<void*>(self, 0x4B4)) {
        Replay(self + 0x358);
    }
    if (At<void*>(self, 0x4BC)) {
        Replay(self + 0x3B0);
    }
}

// 0x484190: the main controller stepped (its node restarted when it finished)
void __fastcall UpdateMain(uint8_t* self, re::Edx, float dt) {
    if (At<int32_t>(self, 0x268) == 0) {
        if (At<uint8_t>(self, 0x31D)) {
            PAC_Obj684CF0(PAC_obj_B9DAB8);
        }
        if (At<void*>(self, 0x4B0) && At<int32_t>(self, 0x268) == 0) {
            Replay(self + 0x300);
        }
    }
    PAC_ControllerUpdate(self + 0x258, dt);
}

// 0x484210: the same with the main controller's 0x5F4110 (0, 0)
void __fastcall Update484210(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x268) == 0) {
        if (At<uint8_t>(self, 0x31D)) {
            PAC_Obj684CF0(PAC_obj_B9DAB8);
        }
        if (At<void*>(self, 0x4B0) && At<int32_t>(self, 0x268) == 0) {
            Replay(self + 0x300);
        }
    }
    PAC_Controller5F4110(self + 0x258, 0.0f, 0);
}

// 0x484280: `node`, with its act file loaded first (nullptr when the file is loaded already)
void* __cdecl LoadAndFind(const char* file, const char* node) {
    if (!PAC_ActFileIsLoaded(PAC_actFiles, file)) {
        PAC_ActFileLoad(file, 0, 0);
        return PAC_ActTreeFind(node, 0);
    }
    return nullptr;
}

// 0x4842C0: the main node from the ped's model info (+0x64 node, +0x68 file)
void __fastcall SetDefaultMain(uint8_t* self, re::Edx, void* ped) {
    At<void*>(self, 0x4B0) = nullptr;
    uint8_t* info = PAC_PedGetModelInfo(ped);
    At<char>(self, 0x4C0) = 0;
    At<char>(self, 0x500) = 0;
    if (info) {
        At<void*>(self, 0x4B0) = PAC_ActTreeFind(At<const char*>(info, 0x64), 0);
        if (!At<void*>(self, 0x4B0)) {
            At<void*>(self, 0x4B0) = FindLoading(At<const char*>(info, 0x64), At<const char*>(info, 0x68));
        }
    }
    if (At<void*>(self, 0x4B0)) {
        PAC_ContextSetNode(self + 0x300, At<void*>(self, 0x4B0));
    }
}

// 0x484350: the second node from the ped's model info (+0x6C node, +0x70 file)
void __fastcall SetDefaultSecond(uint8_t* self, re::Edx, void* ped) {
    At<void*>(self, 0x4B4) = nullptr;
    uint8_t* info = PAC_PedGetModelInfo(ped);
    At<char>(self, 0x540) = 0;
    At<char>(self, 0x580) = 0;
    if (info) {
        At<void*>(self, 0x4B4) = PAC_ActTreeFind(At<const char*>(info, 0x6C), 0);
        if (!At<void*>(self, 0x4B4)) {
            At<void*>(self, 0x4B4) = FindLoading(At<const char*>(info, 0x6C), At<const char*>(info, 0x70));
        }
    }
    if (At<void*>(self, 0x4B4)) {
        PAC_ContextSetNode(self + 0x358, At<void*>(self, 0x4B4));
    }
}

// 0x4843E0: the second act by name (kept) and file
void __fastcall SetSecond(uint8_t* self, re::Edx, const char* node, const char* file) {
    PAC_ControllerStop(self + 0x290);
    PAC_Strncpy(&At<char>(self, 0x540), node, 0x40);
    PAC_Strncpy(&At<char>(self, 0x580), file, 0x40);
    At<void*>(self, 0x4B4) = PAC_ActTreeFind(&At<char>(self, 0x540), 0);
    if (!At<void*>(self, 0x4B4)) {
        At<void*>(self, 0x4B4) = FindLoading(node, file);
    }
    if (At<void*>(self, 0x4B4)) {
        PAC_ContextSetNode(self + 0x358, At<void*>(self, 0x4B4));
    }
}

// 0x484480: the controllers stopped, the nodes forgotten
void __fastcall StopAll(uint8_t* self, re::Edx) {
    PAC_ControllerStop(self + 0x258);
    PAC_ControllerStop(self + 0x290);
    PAC_ControllerStop(self + 0x2C8);
    At<void*>(self, 0x4B0) = nullptr;
    At<void*>(self, 0x4B4) = nullptr;
    At<void*>(self, 0x4BC) = nullptr;
}

// 0x4844C0: a node played on the main controller
void __fastcall PlayMain(uint8_t* self, re::Edx, void* node) {
    At<int32_t>(self, 0x344) = 0;
    PAC_ControllerPlay(self + 0x258, NodePlay(node, self + 0x300));
}

// 0x4844F0: the destructor (the references dropped, the controllers destroyed)
void __fastcall Destruct(uint8_t* self, re::Edx) {
    static const uint32_t refs[] = {0x40C, 0x3BC, 0x3B8, 0x364, 0x360, 0x30C, 0x308};
    for (uint32_t offset : refs) {
        re::Ptr32<void>& ref = At<void*>(self, offset);
        if (ref) {
            PAC_CleanUpOldReference(ref, &ref);
        }
    }
    PAC_ControllerDestruct(self + 0x2C8);
    PAC_ControllerDestruct(self + 0x290);
    PAC_ControllerDestruct(self + 0x258);
    PAC_Empty49A650(self + 0x254);
}

// 0x4845E0: the second controller stepped; stopped when the ped's +0x1C98 is set, and the
// intelligence's 0x5F3A40 instead while the ped (+0x1358) or everyone sleeps
void __fastcall UpdateSecond(uint8_t* self, re::Edx, float dt) {
    uint8_t* ped = At<uint8_t*>(self, 0x308);
    bool asleep = At<int32_t>(ped, 0x1358) != 0 || PAC_globalSleep != 0;
    if (At<uint8_t>(ped, 0x1C98)) {
        PAC_ControllerStop(self + 0x290);
    } else if (!asleep) {
        if (At<int32_t>(self, 0x2A0) == 0 && At<void*>(self, 0x4B4)) {
            PAC_ContextReplay(self + 0x358);
        }
        PAC_ControllerUpdate(self + 0x290, dt);
        return;
    }
    PAC_Intel5F3A40(self);
}

// 0x484660: +0x1C (a blend, at least 0.7, the player's 0.485) eased back towards its
// floor, the faster the more the ped faces along 0x9E33DC's axis, at most 1.5
void __fastcall UpdateHead(uint8_t* self, re::Edx, float dt) {
    uint8_t* ped = At<uint8_t*>(self, 0x308);
    float current = At<float>(self, 0x1C);
    if (At<uint8_t>(ped, 0x4E4)) {
        At<float>(self, 0x1C) = 0.0f;
        return;
    }
    bool isPlayer = At<int32_t>(ped, 0x1310) == 13;
    float floor = isPlayer ? 0.485f : 0.7f;
    if (floor > current) {
        return;
    }
    if (!(0.0f >= At<float>(ped, 0x18D8))) {
        if (At<uint8_t>(ped, 0x130D)) {
            At<float>(self, 0x1C) = floor;
        }
        return;
    }
    V3 dir;
    V3 tmp;
    if (isPlayer && At<int32_t>(ped, 0x131C) != 0) {
        if (At<uint8_t>(ped, 0x18A4)) {
            dir = *PAC_PedGetVec188C(ped, &tmp);
        } else if (At<uint8_t>(ped, 0x18A5)) {
            dir = *PAC_PedGetVec1898(ped, &tmp);
        } else {
            return;
        }
    } else {
        if (!At<uint8_t>(ped, 0x18A4)) {
            return;
        }
        dir = At<V3>(ped, 0x188C);
    }
    float along = PAC_facingAxis.y * dir.y + PAC_facingAxis.x * dir.x + PAC_facingAxis.z * dir.z;
    if (PAC_headTurnDotLimit < along || PAC_headTurnDotLimit == along) {
        return;
    }
    V3 forward;
    V3* f = PAC_PedGetForward(ped, &forward);
    V3 back = {-f->x, -f->y, -f->z};
    PAC_Normalise(&back);
    float dot = std::fabs(back.y * dir.y + back.x * dir.x + back.z * dir.z);
    if (dot < 0.0f || dot == 0.0f) {
        return;
    }
    float target = floor;
    if (PAC_headTurnDotRange > dot) {
        float angle = static_cast<float>(std::acos(static_cast<float>(1.0f - dot / PAC_headTurnDotRange)));
        if (1.0f < angle) {
            angle = 1.0f;
        }
        target = floor + angle * (current - floor);
    }
    float blend = static_cast<float>(target + (current - target) * (dt * 3.0));
    if (1.5f < blend) {
        blend = 1.5f;
    }
    At<float>(self, 0x1C) = blend;
}

// 0x484910: the facial node found, and the ped (with its +0x48) within 8 of the camera
uint8_t __fastcall IsFacialVisible(uint8_t* self, re::Edx) {
    uint8_t* ped = At<uint8_t*>(self, 0x308);
    if (!At<void*>(self, 0x4BC) || At<int32_t>(ped, 0x48) == 0) {
        return 0;
    }
    V3 pos = *EntityPos(ped);
    uint8_t* camera = PAC_Camera6C9A90(PAC_Camera6C82F0(PAC_cameraSource));
    V3 diff;
    V3* d = PAC_VectorSub2(&diff, reinterpret_cast<V3*>(camera + 0x24), &pos);
    float distSq = d->x * d->x + d->y * d->y + d->z * d->z;
    return distSq < 64.0f ? 1 : 0;
}

// 0x4849B0: the main act by name and file (either nullptr: empty; both empty: the model
// info's)
void __fastcall SetMain(uint8_t* self, re::Edx, const char* node, const char* file) {
    PAC_ControllerStop(self + 0x258);
    char* nodeName = &At<char>(self, 0x4C0);
    if (!node) {
        *nodeName = 0;
    } else {
        PAC_Strncpy(nodeName, node, 0x40);
    }
    char* fileName = &At<char>(self, 0x500);
    if (!file) {
        *fileName = 0;
    } else {
        PAC_Strncpy(fileName, file, 0x40);
    }
    uint8_t* info = PAC_PedGetModelInfo(At<void*>(self, 0x308));
    if (*nodeName && *fileName) {
        At<void*>(self, 0x4B0) = PAC_ActTreeFind(nodeName, 0);
        if (!At<void*>(self, 0x4B0)) {
            At<void*>(self, 0x4B0) = PAC_LoadAndFind(fileName, nodeName);
        }
    } else if (info) {
        At<void*>(self, 0x4B0) = PAC_ActTreeFind(At<const char*>(info, 0x64), 0);
    }
    if (At<void*>(self, 0x4B0)) {
        PAC_ContextSetNode(self + 0x300, At<void*>(self, 0x4B0));
    }
}

// 0x484A70: a node (loaded from `file` when needed) played on the second controller
uint8_t __fastcall PlaySecondNode(uint8_t* self, re::Edx, const char* nodeName, const char* file) {
    void* node = PAC_ActTreeFind(nodeName, 0);
    if (!node) {
        if (!file) {
            return 0;
        }
        node = FindLoading(nodeName, file);
        if (!node) {
            return 0;
        }
    }
    // (unlike the main's, the context's index is left alone)
    PAC_ControllerPlay(self + 0x290, NodePlay(node, self + 0x358));
    return 1;
}

// 0x484AF0: a node (loaded from `file` when needed) played on the main controller
uint8_t __fastcall PlayMainNode(uint8_t* self, re::Edx, const char* nodeName, const char* file) {
    void* node = PAC_ActTreeFind(nodeName, 0);
    if (!node) {
        if (!file) {
            return 0;
        }
        node = FindLoading(nodeName, file);
        if (!node) {
            return 0;
        }
    }
    At<int32_t>(self, 0x344) = 0;
    PAC_ControllerPlay(self + 0x258, NodePlay(node, self + 0x300));
    return 1;
}

// 0x484C00: the constructor
uint8_t* __fastcall Construct(uint8_t* self, re::Edx) {
    PAC_IntelBaseConstruct(self);
    PAC_ControllerConstruct(self + 0x258);
    PAC_ControllerConstruct(self + 0x290);
    PAC_ControllerConstruct(self + 0x2C8);
    static const uint32_t contexts[] = {0x300, 0x358, 0x3B0};
    for (uint32_t context : contexts) {
        At<int32_t>(self, context + 0x0) = 0;
        At<int32_t>(self, context + 0x4) = 0;
        At<int32_t>(self, context + 0x8) = 0;
        At<int32_t>(self, context + 0xC) = 0;
        At<int32_t>(self, context + 0x24) = 0;
        At<int32_t>(self, context + 0x10) = 0;
        At<int32_t>(self, context + 0x14) = 0;
        At<int32_t>(self, context + 0x18) = 0;
        At<uint8_t>(self, context + 0x1C) = 0;
        At<uint8_t>(self, context + 0x1D) = 0;
        At<int32_t>(self, context + 0x20) = 0;
        At<int32_t>(self, context + 0x34) = 0;
        At<int32_t>(self, context + 0x38) = 0;
        At<uint8_t>(self, context + 0x3C) = 0;
        At<int32_t>(self, context + 0x40) = 0;
        At<int32_t>(self, context + 0x44) = 0;
        At<int32_t>(self, context + 0x50) = 0;
        At<int32_t>(self, context + 0x54) = 0;
        At<int32_t>(self, context + 0x48) = -1;
        At<float>(self, context + 0x28) = 0.0f;
        At<float>(self, context + 0x2C) = 0.0f;
        At<float>(self, context + 0x30) = 0.0f;
    }
    PAC_Shared408Construct(self + 0x408);
    PAC_Shared474Construct(self + 0x474);
    At<char>(self, 0x4C0) = 0;
    At<char>(self, 0x500) = 0;
    At<char>(self, 0x540) = 0;
    At<char>(self, 0x580) = 0;
    return self;
}

// 0x484DF0: the facial controller stepped while it's visible (stopped when not)
void __fastcall UpdateFacial(uint8_t* self, re::Edx, float dt) {
    if (!PAC_IsFacialVisible(self)) {
        if (At<int32_t>(self, 0x2D8) != 0) {
            PAC_ControllerStop(self + 0x2C8);
        }
        return;
    }
    if (At<int32_t>(self, 0x2D8) == 0) {
        Replay(self + 0x3B0);
    }
    PAC_ControllerUpdate(self + 0x2C8, dt);
}

// 0x484E60: set up for a ped: the contexts pointed at it and at the controllers, the
// nodes found (the acts named, else the model info's; the player's facial node)
void __fastcall Init(uint8_t* self, re::Edx, uint8_t* ped) {
    PAC_ControllerStop(self + 0x258);
    PAC_ControllerStop(self + 0x290);
    PAC_ControllerStop(self + 0x2C8);
    At<void*>(self, 0x4B0) = nullptr;
    At<void*>(self, 0x4B4) = nullptr;
    At<void*>(self, 0x4BC) = nullptr;
    PAC_Intel5F3A40(self);
    At<void*>(self, 0x304) = At<void*>(ped, 0x114);
    At<void*>(self, 0x35C) = At<void*>(ped, 0x114);
    At<void*>(self, 0x3B4) = At<void*>(ped, 0x114);
    SetRef(&At<void*>(self, 0x308), ped);
    SetRef(&At<void*>(self, 0x360), ped);
    SetRef(&At<void*>(self, 0x3B8), ped);
    uint8_t* state = ped + 0x157C;
    At<uint8_t*>(self, 0x338) = state;
    At<uint8_t*>(self, 0x390) = state;
    At<uint8_t*>(self, 0x3E8) = state;
    At<uint8_t>(self, 0x470) = 0;
    At<int32_t>(self, 0x46C) = 0;
    At<uint8_t>(self, 0x43C) = 0;
    At<uint8_t*>(self, 0x310) = self + 0x408;
    At<uint8_t*>(self, 0x368) = self + 0x408;
    At<uint8_t*>(self, 0x3C0) = self + 0x408;
    At<uint8_t*>(self, 0x36C) = self + 0x474;
    At<uint8_t*>(self, 0x314) = self + 0x474;
    At<uint8_t*>(self, 0x3C4) = self + 0x474;
    At<uint8_t*>(self, 0x270) = self + 0x300;
    At<uint8_t*>(self, 0x300) = self + 0x258;
    At<uint8_t*>(self, 0x358) = self + 0x290;
    At<uint8_t*>(self, 0x2A8) = self + 0x358;
    At<uint8_t*>(self, 0x2E0) = self + 0x3B0;
    At<uint8_t*>(self, 0x3B0) = self + 0x2C8;
    At<uint8_t>(self, 0x31D) = 0;
    At<uint8_t>(self, 0x375) = 0;
    At<int32_t>(ped, 0x1344) = 0;
    At<int32_t>(ped, 0x1348) = 0;
    At<void*>(self, 0x4B0) = nullptr;
    At<void*>(self, 0x4B4) = nullptr;
    At<void*>(self, 0x4B8) = PAC_defaultSecondNode;
    At<void*>(self, 0x4BC) = nullptr;
    uint8_t* info = PAC_PedGetModelInfo(ped);
    char* secondNode = &At<char>(self, 0x540);
    char* secondFile = &At<char>(self, 0x580);
    if (*secondNode && *secondFile) {
        At<void*>(self, 0x4B4) = PAC_ActTreeFind(secondNode, 0);
        if (!At<void*>(self, 0x4B4)) {
            At<void*>(self, 0x4B4) = PAC_LoadAndFind(secondFile, secondNode);
        }
    } else if (info) {
        At<void*>(self, 0x4B4) = PAC_ActTreeFind(At<const char*>(info, 0x6C), 0);
        if (!At<void*>(self, 0x4B4)) {
            At<void*>(self, 0x4B4) = PAC_LoadAndFind(At<const char*>(info, 0x70), At<const char*>(info, 0x6C));
        }
    }
    char* mainNode = &At<char>(self, 0x4C0);
    char* mainFile = &At<char>(self, 0x500);
    if (*mainNode && *mainFile) {
        At<void*>(self, 0x4B0) = PAC_ActTreeFind(mainNode, 0);
        if (!At<void*>(self, 0x4B0)) {
            At<void*>(self, 0x4B0) = PAC_LoadAndFind(mainFile, mainNode);
        }
    } else if (info) {
        At<void*>(self, 0x4B0) = PAC_ActTreeFind(At<const char*>(info, 0x64), 0);
        if (!At<void*>(self, 0x4B0)) {
            At<void*>(self, 0x4B0) = PAC_LoadAndFind(At<const char*>(info, 0x68), At<const char*>(info, 0x64));
        }
    }
    if (At<int32_t>(ped, 0x1310) == 13) {
        At<void*>(self, 0x4BC) = PAC_ActTreeFind(PAC_playerFacial, 0);
    } else {
        int model = At<int16_t>(ped, 0x10E);
        if (model != PAC_noFacialModel1 && model != PAC_noFacialModel2 && model != PAC_noFacialModel3 &&
            model != PAC_noFacialModel4) {
            At<void*>(self, 0x4BC) = PAC_defaultFacialNode;
        }
    }
    if (At<void*>(self, 0x4B0)) {
        PAC_ContextSetNode(self + 0x300, At<void*>(self, 0x4B0));
    }
    if (At<void*>(self, 0x4B4)) {
        PAC_ContextSetNode(self + 0x358, At<void*>(self, 0x4B4));
    }
    if (At<void*>(self, 0x4BC)) {
        PAC_ContextSetNode(self + 0x3B0, At<void*>(self, 0x4BC));
    }
}

// 0x4851A0: the controllers stepped (and the ped's +0x548's)
void __fastcall UpdateControllers(uint8_t* self, re::Edx, float dt) {
    PAC_UpdateSecond(self, dt);
    PAC_UpdateFacial(self, dt);
    PAC_UpdateHead(self, dt);
    void* other = At<void*>(At<uint8_t*>(self, 0x308), 0x548);
    if (other) {
        PAC_Obj4EF630(other, dt);
    }
    PAC_UpdateMain(self, dt);
}

// 0x485200: a step (none of dt 0 while 0xC2B1CC is 1); the two timers (+0x434, +0x438)
// run unless -1
void __fastcall Process(uint8_t* self, re::Edx, float dt) {
    if (dt == 0.0f && PAC_skipZeroStep == 1) {
        return;
    }
    PAC_UpdateControllers(self, dt);
    if (At<float>(self, 0x434) != -1.0f) {
        At<float>(self, 0x434) = dt + At<float>(self, 0x434);
    }
    if (At<float>(self, 0x438) != -1.0f) {
        At<float>(self, 0x438) = At<float>(self, 0x438) + dt;
    }
    PAC_Intel5F3880(self, dt);
    PAC_Shared474Update(self + 0x474, dt);
    PAC_Update484210(self);
}

} // namespace c07pac

#define PAC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PAC_REVERSED("ActionContext::Replay", 0x00484010, &c07pac::ContextReplay);
PAC_REVERSED("CPed::GetVec188C", 0x00484040, &c07pac::PedGetVec188C);
PAC_REVERSED("CPed::GetVec1898", 0x00484070, &c07pac::PedGetVec1898);
PAC_REVERSED("CPedIntelligence::ClearActNames", 0x004840A0, &c07pac::ClearActNames);
PAC_REVERSED("CPedIntelligence::StopControllers", 0x004840C0, &c07pac::StopControllers);
PAC_REVERSED("CPedIntelligence::ReplayAll", 0x004840F0, &c07pac::ReplayAll);
PAC_REVERSED("CPedIntelligence::UpdateMain", 0x00484190, &c07pac::UpdateMain);
PAC_REVERSED("CPedIntelligence::Update484210", 0x00484210, &c07pac::Update484210);
PAC_REVERSED("ActFile::LoadAndFind", 0x00484280, &c07pac::LoadAndFind);
PAC_REVERSED("CPedIntelligence::SetDefaultMain", 0x004842C0, &c07pac::SetDefaultMain);
PAC_REVERSED("CPedIntelligence::SetDefaultSecond", 0x00484350, &c07pac::SetDefaultSecond);
PAC_REVERSED("CPedIntelligence::SetSecond", 0x004843E0, &c07pac::SetSecond);
PAC_REVERSED("CPedIntelligence::StopAll", 0x00484480, &c07pac::StopAll);
PAC_REVERSED("CPedIntelligence::PlayMain", 0x004844C0, &c07pac::PlayMain);
PAC_REVERSED("CPedIntelligence::Destruct", 0x004844F0, &c07pac::Destruct);
PAC_REVERSED("CPedIntelligence::UpdateSecond", 0x004845E0, &c07pac::UpdateSecond);
PAC_REVERSED("CPedIntelligence::UpdateHead", 0x00484660, &c07pac::UpdateHead);
PAC_REVERSED("CPedIntelligence::IsFacialVisible", 0x00484910, &c07pac::IsFacialVisible);
PAC_REVERSED("CPedIntelligence::SetMain", 0x004849B0, &c07pac::SetMain);
PAC_REVERSED("CPedIntelligence::PlaySecondNode", 0x00484A70, &c07pac::PlaySecondNode);
PAC_REVERSED("CPedIntelligence::PlayMainNode", 0x00484AF0, &c07pac::PlayMainNode);
PAC_REVERSED("CPedIntelligence::Construct", 0x00484C00, &c07pac::Construct);
PAC_REVERSED("CPedIntelligence::UpdateFacial", 0x00484DF0, &c07pac::UpdateFacial);
PAC_REVERSED("CPedIntelligence::Init", 0x00484E60, &c07pac::Init);
PAC_REVERSED("CPedIntelligence::UpdateControllers", 0x004851A0, &c07pac::UpdateControllers);
PAC_REVERSED("CPedIntelligence::Process", 0x00485200, &c07pac::Process);
