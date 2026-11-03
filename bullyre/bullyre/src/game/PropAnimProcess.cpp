// CPropAnim's action tree (+0x228 its tree, +0x22C its use-point model, +0x230) and its
// per-frame processing: fading (+0x248 the fade timer, +0x24C its rate, +0x250 the alpha),
// the pulsing glow (+0x258 on, +0x25C the level, +0x260 rising), the glows of the
// special props, the door state it's placed with, and its resets.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVec {
    float x, y, z;
};

template <typename R, typename... A>
R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}

GAME_VAR(uint8_t*, pp_playerPed, 0x00C1AEA8);
GAME_ARRAY(uint8_t*, pp_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, pp_triggersOn, 0x020C8AB0);
GAME_VAR(uint32_t, pp_triggerAnimHash, 0x01B65504);
GAME_VAR(uint32_t, pp_signAnimHash, 0x01B65518);
GAME_VAR(void*, pp_kind3Tree, 0x00C8E900);
GAME_VAR(void*, pp_kind1Tree, 0x00C8E8FC);
GAME_VAR(void*, pp_openTree, 0x00C8E904);
GAME_VAR(void*, pp_closedTree, 0x00C8E908);
GAME_VAR(const char*, pp_openFullyAnimName, 0x00A76B5C);
GAME_VAR(uint32_t, pp_nodeIdGuard, 0x00C3C9D8);
GAME_VAR(uint16_t, pp_nodeId, 0x00C3C9D4);
GAME_VAR(uint16_t, pp_nextTypeId, 0x00BF3830);
GAME_VAR(int32_t, pp_triggerModel, 0x00BF4470);
GAME_VAR(int32_t, pp_lateModel, 0x00BF4480);
GAME_VAR(int32_t, pp_glowModel, 0x00BF44CC);
GAME_VAR(int32_t, pp_pulseModel, 0x00BF40E4);
GAME_VAR(int32_t, pp_widescreen, 0x00C3CD0C);
GAME_VAR(int32_t, pp_glowTexture, 0x00CA05F0);
GAME_VAR(uint8_t, pp_pulseActive, 0x00CF09E8);
inline void* const pp_actionTrees = reinterpret_cast<void*>(0x020C7E98);
inline void* const pp_triggers = reinterpret_cast<void*>(0x020C8A00);
inline void* const pp_useModels = reinterpret_cast<void*>(0x00C0F7B4);
inline void* const pp_doors = reinterpret_cast<void*>(0x00C3B808);
inline void* const pp_anyArea = reinterpret_cast<void*>(0x00A453F8);
inline void* const pp_openInput = reinterpret_cast<void*>(0x01B652D0);
inline void* const pp_closeInput = reinterpret_cast<void*>(0x01B652D4);
inline void* const pp_closingInput = reinterpret_cast<void*>(0x01B654E4);
inline void* const pp_pulseInput = reinterpret_cast<void*>(0x01B652CC);

GAME_FN(uint8_t*, __thiscall, ModelInfo_GetAnimInfo, 0x005201D0, uint8_t* modelInfo);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsHidden, 0x00520430, uint8_t* modelInfo);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsKind1, 0x00520440, uint8_t* modelInfo);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsKind3, 0x00520480, uint8_t* modelInfo);
GAME_FN(void, __thiscall, PAnimModelInfo_OnBroken, 0x005208C0, uint8_t* modelInfo, void* prop);
GAME_FN(uint32_t*, __thiscall, Hash_Construct, 0x005FA870, uint32_t* hash, uint32_t value);
GAME_FN(void, __thiscall, Hash_Destruct, 0x0049A650, uint32_t* hash);
GAME_FN(void*, __thiscall, ActionTrees_Get, 0x006D7A60, void* trees, void* key);
GAME_FN(uint8_t*, __thiscall, Triggers_Find, 0x0072C390, void* triggers, void* tree);
GAME_FN(void, __thiscall, Triggers_Update, 0x0072C570, void* triggers, void* prop);
GAME_FN(void*, __cdecl, CatStore_GetTree, 0x0052D810, int use);
GAME_FN(void, __thiscall, HashRefPool_AddRef, 0x004506F0, void* pool, int use);
GAME_FN(void, __thiscall, HashRefPool_RemoveRef, 0x00450720, void* pool, int use);
GAME_FN(int, __thiscall, HashRefPool_GetNumRefs, 0x00450760, void* pool, int use);
GAME_FN(void, __thiscall, Controller_Stop, 0x005F40A0, void* controller);
GAME_FN(uint8_t, __thiscall, Controller_IsNodePlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(void*, __thiscall, Controller_GetNode, 0x005F49E0, void* controller, int index);
GAME_FN(void, __thiscall, Controller_Play, 0x005F45F0, void* controller, void* node);
GAME_FN(void, __thiscall, Controller_Update, 0x005F4B00, void* controller, float dt);
GAME_FN(void, __thiscall, Controller_Settle, 0x005F4110, void* controller, float, int);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* path, int count, const void* names);
GAME_FN(void*, __cdecl, Path_Resolve, 0x005F4E60, void* path, uintptr_t);
GAME_FN(void*, __cdecl, ActTree_FindNode, 0x005F5B10, const char* name, void* node);
GAME_FN(void, __cdecl, PropAnim_OnOpen, 0x005B2F50, void* prop);
GAME_FN(void, __cdecl, Door_SetPathableToPeds, 0x005EB060, void* door, int pathable);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVec* v);
GAME_FN(float*, __thiscall, Entity_GetMatrixLink, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Placement_GetMatrix, 0x0046DCA0, void* placement, void* out);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412720, void* matrix, const void* from);
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(CVec*, __cdecl, VectorScale, 0x00412350, CVec* out, const CVec* v, float s);
GAME_FN(CVec*, __cdecl, VectorAdd, 0x004129A0, CVec* out, const CVec* a, const CVec* b);
GAME_FN(void, __thiscall, Vector_Assign, 0x004129D0, void* dst, const CVec* src);
// (the prop's own and the door manager's, through their addresses)
GAME_FN(void, __thiscall, Door_SetCurrent, 0x004ED940, void* door);
GAME_FN(void, __thiscall, Door_SetAnim, 0x004EE010, void* door, void* anim);
GAME_FN(void, __thiscall, Prop_SetOpenAmount, 0x004EA2A0, void* prop, float amount);
GAME_FN(void, __thiscall, Prop_SetupUse, 0x004EEFA0, void* prop);
GAME_FN(void, __thiscall, Prop_ResetUse, 0x004EF2A0, void* prop);
GAME_FN(void, __thiscall, Prop_ReleaseUseModel, 0x004ED8E0, void* prop);
GAME_FN(void, __thiscall, Prop_FireTrigger, 0x004EDEB0, void* prop, int force);
GAME_FN(void, __thiscall, Prop_UpdateClosing, 0x004EEBA0, void* prop);
GAME_FN(void, __thiscall, Prop_UpdateSignGlow, 0x004EBCB0, void* prop);
GAME_FN(void, __cdecl, AddLateProp, 0x004E8DB0, void* prop);
GAME_FN(void, __cdecl, AddLitPropRecord, 0x004EA700, uint8_t* record);
GAME_FN(uint8_t, __thiscall, DoorLock_IsClosing, 0x004EA400, void* lock);
GAME_FN(void, __thiscall, DM_GetStates, 0x004E9C10, void* doors, int id, int value, uint8_t* bit0, uint8_t* bit1,
        uint8_t* bit2);

const CVec& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVec*>(matrix + 0x30) : At<CVec>(entity, 4);
}

uint8_t* ModelInfoOf(uint8_t* entity) {
    return pp_modelInfoPtrs[At<int16_t>(entity, 0x10E)];
}

uint16_t NodeTypeId() {
    if (!(pp_nodeIdGuard & 1)) {
        pp_nodeIdGuard |= 1;
        pp_nodeId = pp_nextTypeId;
        pp_nextTypeId = uint16_t(pp_nextTypeId + 1);
    }
    return pp_nodeId;
}

// 0x4EEFA0 (thiscall): sets up the prop's action tree: its use-point model's (loaded and
// referenced), else its model's; a door's open or closed tree by its state. With a
// trigger saved for it, the trigger's node is played instead
void __fastcall SetupUse(uint8_t* self) {
    uint32_t saved = 0;
    Door_SetCurrent(self);
    void* controller = self + 0x128;
    Controller_Stop(controller);
    self[0x224] = 0;
    At<int32_t>(self, 0x220) = 0;
    self[0x1F0] = 0;
    uint8_t* modelInfo = ModelInfoOf(self);
    uint8_t* animInfo = ModelInfo_GetAnimInfo(modelInfo);
    uint8_t useTrigger = 0;
    saved = 0;
    bool matched = false;
    if (pp_triggersOn != 0) {
        uint32_t hash;
        Hash_Construct(&hash, At<uint32_t>(animInfo, 0x80));
        matched = pp_triggerAnimHash == hash;
        Hash_Destruct(&hash);
    }
    if (matched) {
        void* tree = ActionTrees_Get(pp_actionTrees, At<void*>(self, 0x23C));
        useTrigger = 1;
        saved = re::Addr32(Triggers_Find(pp_triggers, tree));
    }
    int use = At<int16_t>(self, 0x22C);
    bool haveTree = false;
    if (use != -1) {
        if (At<void*>(self, 0x228) != nullptr) {
            haveTree = true;
        } else {
            void* tree;
            if (use == VCall<int16_t>(modelInfo, 0x2C) || CatStore_GetTree(use) == nullptr) {
                tree = At<void*>(modelInfo, 0x88);
                if (tree == nullptr) {
                    return;
                }
            } else {
                tree = CatStore_GetTree(use);
            }
            At<void*>(self, 0x228) = tree;
            HashRefPool_AddRef(pp_useModels, use);
            self[0x3A9] = 1;
        }
    }
    if (!haveTree && At<void*>(self, 0x228) == nullptr) {
        if (PAnimModelInfo_IsKind3(modelInfo)) {
            At<void*>(self, 0x228) = pp_kind3Tree;
        } else if (PAnimModelInfo_IsKind1(modelInfo)) {
            At<void*>(self, 0x228) = pp_kind1Tree;
        }
    }
    if (PAnimModelInfo_IsKind1(modelInfo)) {
        if (At<int32_t>(self, 0x1A0) != 0) {
            Door_SetCurrent(self);
            if (!Controller_IsNodePlaying(controller, pp_openInput, 1, 1)) {
                At<void*>(self, 0x228) = pp_openTree;
                At<int32_t>(self, 0x184) = 1;
            }
        } else if (At<int32_t>(self, 0x184) == 0) {
            Door_SetCurrent(self);
            if (!Controller_IsNodePlaying(controller, pp_closeInput, 1, 1)) {
                At<void*>(self, 0x228) = pp_closedTree;
                At<int32_t>(self, 0x184) = 0;
            }
        }
    }
    void* tree = At<void*>(self, 0x228);
    if (tree == nullptr) {
        return;
    }
    uint8_t* trigger = reinterpret_cast<uint8_t*>(uintptr_t(saved));
    if (useTrigger != 1 || trigger == nullptr) {
        Door_SetAnim(self, VCall<void*>(tree, 0x4C, pp_anyArea));
        return;
    }
    Prop_SetOpenAmount(self, 1.0f);
    int count = At<int32_t>(trigger, 0x44);
    // (the path, a count and the names' address: 8 bytes, built where the game's trigger slot
    // was; ours has its own)
    uint32_t path[2] = {};
    Path_Build(path, count, trigger + (0x11 - count) * 4);
    void* node = Path_Resolve(path, 0);
    Door_SetAnim(self, VCall<void*>(node, 0x4C, pp_anyArea));
    At<int16_t>(self, 0x22C) = At<int16_t>(trigger, 0x48);
    Door_SetCurrent(self);
    for (int i = 0; i < At<int32_t>(controller, 0x34); i++) {
        uint8_t* child = static_cast<uint8_t*>(Controller_GetNode(controller, i));
        if (child == nullptr) {
            continue;
        }
        if (VCall<uint8_t>(child, 0, uint32_t(NodeTypeId()))) {
            At<int32_t>(child, 0x30) = At<int32_t>(trigger, 0x4C);
            return;
        }
    }
}

// 0x4EF2A0 (thiscall): the action tree let go (the use-point model's reference dropped,
// the model released when no longer referenced)
void __fastcall ResetUse(uint8_t* self) {
    uint8_t* modelInfo = ModelInfoOf(self);
    Door_SetCurrent(self);
    Prop_FireTrigger(self, 0);
    Controller_Stop(self + 0x128);
    if (PAnimModelInfo_IsHidden(modelInfo) && self[0x3A9] == 1 && At<uint16_t>(self, 0x22C) != 0xFFFF) {
        HashRefPool_RemoveRef(pp_useModels, At<int16_t>(self, 0x22C));
        if (HashRefPool_GetNumRefs(pp_useModels, At<int16_t>(self, 0x22C)) == 0) {
            Prop_ReleaseUseModel(self);
        }
    }
    self[0x3A9] = 0;
    At<int32_t>(self, 0x228) = 0;
    At<int32_t>(self, 0x230) = 0;
}

// 0x4EF340 (thiscall): a new action tree for a use-point model (just played when it's
// the same model and its animation info allows)
void __fastcall SetUseTree(uint8_t* self, re::Edx, void* tree, int use) {
    if (At<int16_t>(self, 0x22C) == use && At<int32_t>(ModelInfo_GetAnimInfo(ModelInfoOf(self)), 0x6C) != 0) {
        Door_SetAnim(self, tree);
        return;
    }
    if (At<void*>(self, 0x228) != nullptr && At<int16_t>(self, 0x22C) != -1) {
        Door_SetCurrent(self);
        Controller_Stop(self + 0x128);
        Prop_ResetUse(self);
    }
    At<void*>(self, 0x228) = tree;
    At<int16_t>(self, 0x22C) = int16_t(use);
    At<int32_t>(self, 0x230) = 0;
    if (tree == nullptr) {
        return;
    }
    if (int16_t(use) != -1) {
        Prop_SetupUse(self);
    }
    Door_SetAnim(self, VCall<void*>(At<void*>(self, 0x228), 0x4C, pp_anyArea));
}

// 0x4EF400 (thiscall): a door's saved state applied (+0x17C, +0x180; opened fully at once
// when saved open)
void __fastcall ApplyDoorState(uint8_t* self, re::Edx, int model, uint8_t* placement) {
    if (!PAnimModelInfo_IsKind1(pp_modelInfoPtrs[model])) {
        return;
    }
    // (outputs the door manager leaves untouched keep what the game's slots held: the
    // arguments' low bytes and this pointer's top byte)
    uint8_t bit0 = uint8_t(reinterpret_cast<uintptr_t>(placement));
    uint8_t bit1 = uint8_t(model);
    uint8_t bit2 = uint8_t(reinterpret_cast<uintptr_t>(self) >> 24);
    DM_GetStates(pp_doors, At<int32_t>(placement, 0x2C), (placement[0x3F] >> 1) & 1, &bit0, &bit1, &bit2);
    At<int32_t>(self, 0x17C) = bit0;
    At<int32_t>(self, 0x180) = bit1;
    if (bit2 != 0 && At<int32_t>(self, 0x184) == 0) {
        Door_SetCurrent(self);
        if (!Controller_IsNodePlaying(self + 0x128, pp_openInput, 1, 1)) {
            void* anim = ActTree_FindNode(pp_openFullyAnimName, nullptr);
            if (anim != nullptr) {
                At<float>(self, 0x3B0) = 1.0f;
                if (At<int32_t>(ModelInfoOf(self), 0x80) == 0) {
                    At<int32_t>(self, 0x20) = 0;
                    At<int32_t>(self, 0x3C) = 0;
                }
                At<int32_t>(self, 0x160) = 0;
                At<int32_t>(self, 0x19C) = 0;
                At<int32_t>(self, 0x184) = 1;
                PropAnim_OnOpen(self);
                Door_SetAnim(self, anim);
                VCall<void>(self, 0x4C);
            }
        }
    }
    if (bit1 != 0) {
        Door_SetPathableToPeds(self, 0);
    }
}

// 0x4EF520 (thiscall): reset (the tree set up again, fading and damage cleared)
void __fastcall Reset(uint8_t* self) {
    Prop_SetupUse(self);
    At<float>(self, 0x248) = -1.0f;
    At<int32_t>(self, 0x48) = 1;
    At<float>(self, 0x24C) = 0.0f;
    At<uint16_t>(self, 0x246) = 0;
    At<int32_t>(self, 0x274) = 0;
    At<int32_t>(self, 0x238) = 0;
    At<int32_t>(self, 0x188) = 0;
    At<int32_t>(self, 0x170) = 0;
    At<int32_t>(self, 0x174) = 0;
}

// 0x4EF570 (thiscall): a fuller reset (alpha, the damage stage and visibility too)
void __fastcall ResetFull(uint8_t* self) {
    Prop_SetupUse(self);
    At<float>(self, 0x248) = -1.0f;
    At<float>(self, 0x24C) = 0.0f;
    At<int32_t>(self, 0x48) = 1;
    At<uint16_t>(self, 0x246) = 0;
    At<int32_t>(self, 0x274) = 0;
    At<int32_t>(self, 0x238) = 0;
    At<int32_t>(self, 0x188) = 0;
    At<int32_t>(self, 0x170) = 0;
    At<int32_t>(self, 0x174) = 0;
    At<uint16_t>(self, 0x250) = 0xFF;
    At<int32_t>(self, 0x160) = 0;
    At<int32_t>(self, 0x164) = 0;
    At<int32_t>(self, 0x16C) = 0;
    At<int32_t>(self, 0x18C) = 0;
    uint8_t* modelInfo = ModelInfoOf(self);
    if (modelInfo != nullptr) {
        At<int32_t>(self, 0x168) = ModelInfo_GetAnimInfo(modelInfo)[0x8C];
    }
    At<uint16_t>(self, 0x120) = 0;
    if (!PAnimModelInfo_IsKind1(ModelInfoOf(self))) {
        self[0x244] = 1;
        At<int32_t>(self, 0x20) = 1;
        At<int32_t>(self, 0x3C) = 1;
    }
}

// a glow record from the prop's matrix: right scaled, forward and up as they are, the
// position raised along up (the 4th floats and +0x54 are the game's uninitialised stack)
void AddGlow(uint8_t* self, float rightScale, float lift, bool lifted, uint32_t colour) {
    alignas(16) uint8_t matrix[0x48] = {};
    At<int32_t>(matrix, 0x40) = 0;
    if (At<void*>(self, 0x14) != nullptr) {
        Matrix_Copy(matrix, Entity_GetMatrixLink(self));
    } else {
        Placement_GetMatrix(self + 4, matrix);
    }
    (void)colour;
    alignas(16) uint8_t record[0x58] = {};
    At<int32_t>(record, 0x40) = 0;
    CVec right = At<CVec>(matrix, 0);
    CVec up = At<CVec>(matrix, 0x20);
    CVec forward = At<CVec>(matrix, 0x10);
    CVec scaled;
    Vector_Assign(record, VectorScale(&scaled, &right, rightScale));
    At<CVec>(record, 0x20) = *VectorScale(&scaled, &up, 1.0f);
    At<CVec>(record, 0x10) = *VectorScale(&scaled, &forward, 1.0f);
    if (lifted) {
        CVec raise;
        VectorScale(&raise, &up, lift);
        CVec pos;
        At<CVec>(record, 0x30) = *VectorAdd(&pos, &At<CVec>(matrix, 0x30), &raise);
    } else {
        At<CVec>(record, 0x30) = At<CVec>(matrix, 0x30);
        At<int32_t>(record, 0x3C) = At<int32_t>(matrix, 0x3C);
    }
}

// the glow colour: grey by the pulse level
uint32_t GlowColour(uint8_t* self) {
    uint8_t rgba[4];
    uint8_t level = uint8_t(int(double(At<float>(self, 0x25C)) * 196.0));
    RGBA_Construct(rgba, level, level, level, 100);
    return *reinterpret_cast<uint32_t*>(rgba);
}

void AddGlowRecord(uint8_t* self, float rightScale, float lift, bool lifted) {
    uint32_t colour = GlowColour(self);
    alignas(16) uint8_t matrix[0x48] = {};
    if (At<void*>(self, 0x14) != nullptr) {
        Matrix_Copy(matrix, Entity_GetMatrixLink(self));
    } else {
        Placement_GetMatrix(self + 4, matrix);
    }
    alignas(16) uint8_t record[0x58] = {};
    CVec right = At<CVec>(matrix, 0);
    CVec up = At<CVec>(matrix, 0x20);
    CVec forward = At<CVec>(matrix, 0x10);
    CVec scaled;
    Vector_Assign(record, VectorScale(&scaled, &right, rightScale));
    At<CVec>(record, 0x20) = *VectorScale(&scaled, &up, 1.0f);
    At<CVec>(record, 0x10) = *VectorScale(&scaled, &forward, 1.0f);
    if (lifted) {
        CVec raise;
        VectorScale(&raise, &up, lift);
        CVec pos;
        At<CVec>(record, 0x30) = *VectorAdd(&pos, &At<CVec>(matrix, 0x30), &raise);
    } else {
        At<CVec>(record, 0x30) = At<CVec>(matrix, 0x30);
        At<int32_t>(record, 0x3C) = At<int32_t>(matrix, 0x3C);
    }
    At<int32_t>(record, 0x40) = 0;
    At<uint32_t>(record, 0x44) = colour;
    At<float>(record, 0x48) = 0.75f;
    At<float>(record, 0x4C) = 20.0f;
    At<int32_t>(record, 0x50) = pp_glowTexture;
    AddLitPropRecord(record);
}

bool PlayerWithin20(uint8_t* self) {
    const CVec& pos = Position(self);
    const CVec& player = Position(pp_playerPed);
    CVec d = {pos.x - player.x, pos.y - player.y, pos.z - player.z};
    return Vector_MagnitudeSqr(&d) < 400.0;
}

// 0x4EF630 (thiscall): the prop's per-frame processing
void __fastcall ProcessControl(uint8_t* self, re::Edx, float dt) {
    At<int32_t>(self, 0x1A8) = 1;
    uint8_t* modelInfo = ModelInfoOf(self);
    uint8_t* animInfo = ModelInfo_GetAnimInfo(modelInfo);
    int model = At<int16_t>(self, 0x10E);
    if (model == pp_triggerModel) {
        Triggers_Update(pp_triggers, self);
    }
    if (PAnimModelInfo_IsKind1(ModelInfoOf(self))) {
        Prop_UpdateClosing(self);
        Door_SetCurrent(self);
        if (Controller_IsNodePlaying(self + 0x128, pp_closingInput, 1, 1)) {
            return;
        }
    } else {
        uint32_t hash;
        Hash_Construct(&hash, At<uint32_t>(animInfo, 0x80));
        bool sign = pp_signAnimHash == hash;
        Hash_Destruct(&hash);
        if (sign) {
            Prop_UpdateSignGlow(self);
        } else if (model == pp_lateModel) {
            AddLateProp(self);
        } else if (At<int16_t>(self, 0x10E) == pp_glowModel) {
            if (At<int32_t>(self, 0xD0) == 0 && At<int32_t>(self, 0x1AC) == 0 && pp_widescreen == 0 && PlayerWithin20(self)) {
                AddGlowRecord(self, 1.2f, -0.05f, true);
            }
        } else if (model == pp_pulseModel) {
            Door_SetCurrent(self);
            if (Controller_IsNodePlaying(self + 0x128, pp_pulseInput, 1, 1) == 1 && !DoorLock_IsClosing(self + 0x398)) {
                pp_pulseActive = 1;
                self[0x258] = 1;
                if (PlayerWithin20(self)) {
                    AddGlowRecord(self, 1.5f, 0.0f, false);
                }
            } else {
                pp_pulseActive = 0;
                self[0x258] = 0;
            }
        }
    }
    // fading out
    if (-1.0f < At<float>(self, 0x248)) {
        At<float>(self, 0x248) = At<float>(self, 0x248) - dt;
        if (self[0x3B6] != 0 && double(At<int16_t>(self, 0x250)) < 60.0) {
            PAnimModelInfo_OnBroken(modelInfo, self);
            self[0x3B6] = 0;
        }
        int16_t alpha = int16_t(int(double(At<float>(self, 0x24C)) * At<float>(self, 0x248) * 255.0));
        At<int16_t>(self, 0x250) = alpha;
        float level = float(alpha);
        if (!(level < 255.0f)) {
            level = 255.0f;
        }
        alpha = int16_t(int(level));
        At<int16_t>(self, 0x250) = alpha;
        if (alpha < 0) {
            At<int16_t>(self, 0x250) = 0;
            At<float>(self, 0x248) = -1.0f;
            At<int32_t>(self, 0x48) = 0;
        }
    }
    Door_SetCurrent(self);
    void* tree = At<void*>(self, 0x228);
    if (tree != nullptr && At<int32_t>(self, 0x138) == 0) {
        Controller_Play(self + 0x128, VCall<void*>(tree, 0x4C, pp_anyArea));
    }
    At<float>(self, 0x208) = 0.0f;
    At<float>(self, 0x210) = 0.0f;
    Door_SetCurrent(self);
    Controller_Update(self + 0x128, dt);
    Controller_Settle(self + 0x128, 0.0f, 0);
    if (-1.0f != At<float>(self, 0x1E8)) {
        At<float>(self, 0x1E8) = At<float>(self, 0x1E8) + dt;
    }
    if (-1.0f != At<float>(self, 0x1EC)) {
        At<float>(self, 0x1EC) = At<float>(self, 0x1EC) + dt;
    }
    // the pulsing glow, between 0.25 and 0.5
    if (self[0x258] != 0) {
        if (VCall<uint8_t>(self, 0x78)) {
            self[0x258] = 0;
        }
        float step = self[0x260] != 0 ? 0.03f : -0.03f;
        float level = At<float>(self, 0x25C) + step;
        At<float>(self, 0x25C) = level;
        if (0.5f < level) {
            self[0x260] = 0;
            At<float>(self, 0x25C) = 0.5f;
        } else if (!(0.25f <= level)) {
            At<float>(self, 0x25C) = 0.25f;
            self[0x260] = 1;
        }
        At<float>(self, 0x264) = At<float>(self, 0x25C);
        At<float>(self, 0x268) = At<float>(self, 0x25C);
        At<float>(self, 0x26C) = At<float>(self, 0x25C);
        At<float>(self, 0x270) = 1.0f;
    }
    if (!PAnimModelInfo_IsHidden(ModelInfoOf(self))) {
        VCall<void>(self, 0x4C);
    }
    VCall<void>(self, 0x50);
}

#define C13D_PROC_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C13D_PROC_REVERSED("CPropAnim::SetupUse", 0x004EEFA0, &SetupUse);
C13D_PROC_REVERSED("CPropAnim::ResetUse", 0x004EF2A0, &ResetUse);
C13D_PROC_REVERSED("CPropAnim::SetUseTree", 0x004EF340, &SetUseTree);
C13D_PROC_REVERSED("CPropAnim::ApplyDoorState", 0x004EF400, &ApplyDoorState);
C13D_PROC_REVERSED("CPropAnim::Reset", 0x004EF520, &Reset);
C13D_PROC_REVERSED("CPropAnim::ResetFull", 0x004EF570, &ResetFull);
C13D_PROC_REVERSED("CPropAnim::ProcessControl", 0x004EF630, &ProcessControl);

} // namespace
