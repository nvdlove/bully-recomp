// CEntity's Gamebryo object: CreateRwObject (vf10: from its model info, or from its
// animation dictionary when it has an animated object) and Render (vf18: an atomic with
// its sway, or a clump; faded peds and alpha-flagged models with their render states).

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "reversed.h"

namespace {

using col::CMatrix;

GAME_ARRAY(uint8_t*, s_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, s_ratPedModel, 0x00BF42DC);
GAME_VAR(int32_t, s_numBuildingObjects, 0x00C2A8EC);
GAME_VAR(int32_t, s_numRendered, 0x00C2A8F4);
GAME_VAR(void*, s_scratchFrame, 0x01B64884);
GAME_VAR(float, s_swayX, 0x00C66B2C);
GAME_VAR(float, s_swayY, 0x00C66B30);
GAME_ARRAY(float, s_oscPos, 0x00A2401C);
GAME_ARRAY(float, s_oscPhaseSum, 0x00C2A908);
inline void* const s_animManager = reinterpret_cast<void*>(0x020C4910);
inline void* const s_visibleList = reinterpret_cast<void*>(0x00D12AF8);
inline void* const s_camera = reinterpret_cast<void*>(0x00C3CC68);
inline const char* const s_ratPedBase = reinterpret_cast<const char*>(0x0090C038);    // "RAT_PED\BASE"
inline const char* const s_playerBase = reinterpret_cast<const char*>(0x0090C048);    // "C_PLAYER\BASE"
inline const char* const s_copBikeBase = reinterpret_cast<const char*>(0x0090C024);   // "PROPS\COPBIKE_BASE"
inline const char* const s_scooterBase = reinterpret_cast<const char*>(0x0090C010);   // "PROPS\SCOOTER_BASE"

GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(int, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int, __thiscall, AnimDict_Find, 0x006B1D60, void* dict, int arg, const char* name, int* out);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_Fn520430, 0x00520430, void* mi);
GAME_FN(int, __thiscall, PAnimModelInfo_Fn520420, 0x00520420, void* mi, int arg);
GAME_FN(void, __thiscall, PropAnim_Fn4EF520, 0x004EF520, CEntity* prop);
GAME_FN(uint8_t*, __thiscall, AgrGroup_GetEntry, 0x00450B80, void* group, uint32_t index);
GAME_FN(int, __cdecl, FindAgrId, 0x0052DAD0, const char* name);
GAME_FN(int, __cdecl, GetAgrGroup, 0x0052DB50, int id, uint32_t* index);
GAME_FN(int*, __thiscall, Vehicle_GetClass, 0x004CAF30, CEntity* vehicle, int* out);
GAME_FN(int, __thiscall, AnimManager_Create, 0x006C30A0, void* manager, void* animObject, int anim, void* dict, uint32_t flag, int arg);
GAME_FN(void*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(CMatrix*, __thiscall, Entity_GetMatrixLink, 0x008B6120, CEntity* entity);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, CMatrix* m);
GAME_FN(void, __thiscall, Entity_Fn443AA0, 0x00443AA0, CEntity* entity);
GAME_FN(int, __thiscall, ModelInfo_AddRef, 0x0050EB90, void* mi);
GAME_FN(void, __thiscall, Anim_Fn6C0970, 0x006C0970, void* manager, void* anim, float a, const float* offset, int b, int c);
GAME_FN(void, __thiscall, VisibilityList_AddVisible, 0x005E5480, void* list, CEntity* entity);
GAME_FN(void*, __cdecl, Frame_GetParent, 0x006C7370, void* frame);
// Render's
GAME_FN(int, __cdecl, GetObjectAlpha, 0x005F2B10, void* rwObject);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, uint32_t* out);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, uint32_t value);
GAME_FN(void, __thiscall, Matrix_ToNiTransform, 0x00412AC0, const CMatrix* m, void* ni);
GAME_FN(void, __thiscall, SimpleTransform_UpdateNiTransform, 0x0046D8F0, CSimpleTransform* t, void* out);
GAME_FN(void, __cdecl, UpdateFrame, 0x006C9AF0, void* frame);
GAME_FN(void, __cdecl, Node_AttachChild, 0x006C6A10, void* parent, void* child);
GAME_FN(uint8_t, __cdecl, Fn5274E0, 0x005274E0, int model);
GAME_FN(void, __cdecl, Fn527510, 0x00527510, int model, int arg);
GAME_FN(void, __cdecl, SetTreeFloats, 0x00408AE0, void* object, float* values);
GAME_FN(void, __cdecl, SetObjectWorldTransform, 0x004068E0, void* object, void* transform);
GAME_FN(void, __cdecl, RenderAtomic, 0x006C69F0, void* atomic);
GAME_FN(void, __cdecl, Clump_Render, 0x006C7420, void* clump);
GAME_FN(void, __cdecl, UpdateAttachedBoneMatrices, 0x00408380, void* clump);
GAME_FN(void, __cdecl, UpdateAttachedTransforms, 0x00406910, void* clump);
GAME_FN(float, __thiscall, Camera_GetMaxAlpha, 0x004F1630, void* camera, CEntity* entity);
GAME_FN(void, __thiscall, Entity_SetRwObjectAlpha, 0x00466160, CEntity* entity, int alpha);

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}
template <typename R, typename... A>
R VCall(void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[slot]))(object, args...);
}

// (inlined twice) the matrix's Gamebryo transform attached, or the placement's
void AttachTransform(CEntity* e, void* node) {
    if (e->matrix) {
        void* transform = GetLocalTransform(node);
        CMatrix* m = Entity_GetMatrixLink(e);
        m->attached = transform;
        Matrix_UpdateRW(m);
    } else {
        Entity_Fn443AA0(e);
    }
}

// 0x4662F0 vf10 (thiscall): its object. `addVisible`: with vf33 and collision, onto the
// visible list. (The flag passed on to the animation manager is the argument's slot with
// its low byte replaced: the slot is reused as a scratch before)
void __fastcall CreateRwObject(CEntity* self, re::Edx, int addVisible, int arg) {
    uint8_t* mi = s_modelInfoPtrs[self->modelIndex];
    uint32_t slot = uint32_t(addVisible);
    PushMemoryTag(0x27);
    uint8_t* dict = self->animObject ? Field<uint8_t*>(mi, 0x48) : nullptr;
    if (!dict) {
        self->rwObject = VCall<void*>(mi, 4);  // CreateInstance
        PopMemoryTag();
        if (!self->rwObject) {
            return;
        }
        if (self->GetType() == ENTITY_TYPE_BUILDING) {
            s_numBuildingObjects++;
        }
        uint8_t kind = *static_cast<uint8_t*>(self->rwObject);
        if (kind != 1 && kind == 2) {
            if (self->matrix) {
                void* transform = GetLocalTransform(Frame_GetParent(self->rwObject));
                CMatrix* m = Entity_GetMatrixLink(self);
                m->attached = transform;
                Matrix_UpdateRW(m);
            } else {
                Entity_Fn443AA0(self);
            }
        }
        ModelInfo_AddRef(mi);
        if (re::Flag(addVisible) && self->CallHasModelFlag10000() && self->bUsesCollision) {
            VisibilityList_AddVisible(s_visibleList, self);
        }
        return;
    }
    if (Field<int32_t>(dict, 8) == 0) {
        PopMemoryTag();
        return;
    }
    uint8_t* set = Field<uint8_t*>(dict, 0x10);
    int anim = -1;
    if (set) {
        uint8_t type = self->GetType();
        if (type == ENTITY_TYPE_PED) {
            slot = uint32_t(-1);
            AnimDict_Find(set, 0, self->modelIndex == s_ratPedModel ? s_ratPedBase : s_playerBase, reinterpret_cast<int*>(&slot));
            anim = int(slot);
        } else if (type == ENTITY_TYPE_PROPANIM) {
            uint8_t* propMi = s_modelInfoPtrs[self->modelIndex];
            if (PAnimModelInfo_Fn520430(propMi)) {
                PropAnim_Fn4EF520(self);
                PopMemoryTag();
                return;
            }
            anim = PAnimModelInfo_Fn520420(propMi, 0);
        } else {
            uint32_t group;
            uint32_t index = 0;
            if (VCall<int>(mi, 7) == -1) {
                if (FindAgrId(reinterpret_cast<const char*>(AgrGroup_GetEntry(set, 0) + 4)) == -1) {
                    uint32_t n = Field<uint32_t>(set, 0x68);
                    for (index = 0; index < n; index++) {
                        if (AgrGroup_GetEntry(set, index)[0x34]) {
                            break;
                        }
                    }
                    group = index;
                    slot = group;
                } else {
                    group = 0;
                    slot = 0;
                }
            } else {
                GetAgrGroup(VCall<int>(mi, 7), &slot);
                group = slot;
            }
            // the first entry of the group, or the first taken one
            uint32_t count = Field<uint32_t>(set, 4);
            uint32_t i = 0;
            while (i < count) {
                uint8_t* entry = Field<uint8_t*>(set, 0) + i * 0x40;
                if (Field<uint32_t>(entry, 0x20) == group || Field<int32_t>(entry, 0) != 0) {
                    break;
                }
                i++;
            }
            if (i >= count) {
                i = 0;
            }
            anim = int(i);
            if (type == ENTITY_TYPE_VEHICLE && Field<int32_t>(self, 0x46C) == 1) {
                int kind;
                int found;
                if (*Vehicle_GetClass(self, &kind) == 3) {
                    found = -1;
                    AnimDict_Find(set, 0, s_copBikeBase, &found);
                    anim = found;
                } else if (*Vehicle_GetClass(self, &kind) == 8) {
                    found = -1;
                    AnimDict_Find(set, 0, s_scooterBase, &found);
                    anim = found;
                }
            }
        }
    }
    slot = (slot & 0xFFFFFF00u) | (self->bAlwaysOnScreen ? 1u : 0u);
    if (AnimManager_Create(s_animManager, self->animObject, anim, dict, slot, arg) == 0) {
        self->rwObject = Field<void*>(self->animObject, 0x10);
        AttachTransform(self, Field<void*>(self->animObject, 0xC));
    }
    PopMemoryTag();
    ModelInfo_AddRef(mi);
    if (self->GetType() != ENTITY_TYPE_PROPANIM) {
        return;
    }
    PropAnim_Fn4EF520(self);
    uint32_t saved = self->flagE4;
    self->flagE4 = 1;
    VCall<void>(self, 19);
    self->flagE4 = saved != 0;
    Anim_Fn6C0970(s_animManager, self->animObject, 0.0f, nullptr, 1, 0);
    if (self->CallHasModelFlag10000() && self->bUsesCollision) {
        VisibilityList_AddVisible(s_visibleList, self);
    }
}

// (Render's) the object at the camera's alpha for it, then opaque again
template <typename F>
void RenderWithCameraAlpha(CEntity* e, F render) {
    float alpha = Camera_GetMaxAlpha(s_camera, e);
    Entity_SetRwObjectAlpha(e, int(alpha * 255.0 + 0.5));
    render();
    Entity_SetRwObjectAlpha(e, 0xFF);
}

// the models that never sway
bool NeverSways(int16_t model) {
    switch (model) {
    case 0x2258:
    case 0x225C:
    case 0x2E2:
    case 0x2E7:
    case 0x601:
    case 0x604:
    case 0x30A:
    case 0x30C:
    case 0x3E1:
    case 0x41C:
    case 0x2EC8:
        return true;
    default:
        return model >= 0x2ED2 && model <= 0x2ED6;
    }
}

// 0x466D40 vf18 (thiscall): GTA's CEntity::Render
void __fastcall Render(CEntity* self, re::Edx) {
    if (!self->rwObject || self->bDontRender) {
        return;
    }
    self->bIsBeingRendered = 1;
    s_numRendered++;
    uint8_t* mi = s_modelInfoPtrs[self->modelIndex];
    uint8_t restoreStates = 0;
    uint8_t faded = 0;
    uint8_t zWrite = 1;
    uint32_t s8 = 0, s6 = 0, sC = 0, sA = 0, sB = 0, s7 = 0, sE = 0;
    uint8_t type = Field<uint8_t>(mi, 8);
    if (type == 1 || type == 3) {
        restoreStates = (Field<uint32_t>(mi, 0x28) >> 7) & 1;
        if (restoreStates) {
            GetRenderState(8, &s8);
            GetRenderState(6, &s6);
            GetRenderState(0xC, &sC);
            GetRenderState(0xA, &sA);
            GetRenderState(0xB, &sB);
            GetRenderState(7, &s7);
            GetRenderState(0xE, &sE);
            SetRenderState(8, 0);
            SetRenderState(6, 1);
            SetRenderState(0xC, 1);
            SetRenderState(0xA, 2);
            SetRenderState(0xB, 2);
            SetRenderState(7, 2);
            SetRenderState(0xE, 0);
            SetRenderState(1, 0);
        }
    } else if (type == 8) {
        int alpha = GetObjectAlpha(self->rwObject);
        if (alpha < 0xFF) {
            faded = 1;
            if (alpha < 0x60 && Field<uint8_t>(self, 0x130E) == 1) {
                zWrite = 0;
            }
        }
    }
    uint8_t* object = static_cast<uint8_t*>(self->rwObject);
    if (*object == 1) {
        // an atomic: placed through the scratch frame, swayed, rendered
        if (self->matrix) {
            void* transform = GetLocalTransform(s_scratchFrame);
            Matrix_ToNiTransform(Entity_GetMatrixLink(self), transform);
        } else {
            SimpleTransform_UpdateNiTransform(&self->placement, GetLocalTransform(s_scratchFrame));
        }
        UpdateFrame(s_scratchFrame);
        Node_AttachChild(object, s_scratchFrame);
        s_swayY = 0.0f;
        s_swayX = 0.0f;
        uint8_t* info = s_modelInfoPtrs[self->modelIndex];
        bool sway = false;
        if (info[8] == 1 && !Fn5274E0(self->modelIndex)) {
            Fn527510(self->modelIndex, 1);
            int8_t period = int8_t(info[0x2F]);
            if (period == -1) {
                const double k = 0.007874015718698502;
                s_swayX = float(int8_t(info[0x2D]) * k * s_oscPhaseSum[0]);
                s_swayY = float(int8_t(info[0x2E]) * k * -s_oscPos[0]);
            } else {
                bool step;
                if (period == 0) {
                    step = true;
                } else {
                    int counter = int8_t(info[0x32]) + 1;
                    step = counter == period;
                    if (step) {
                        counter = 0;
                    }
                    info[0x32] = uint8_t(counter);
                }
                int x = int8_t(info[0x30]);
                int y = int8_t(info[0x31]);
                if (step) {
                    x += int8_t(info[0x2D]);
                    if (x > 0x80) {
                        x -= 0x80;
                    } else if (x < -0x80) {
                        x += 0x80;
                    }
                    info[0x30] = uint8_t(x);
                    y += int8_t(info[0x2E]);
                    if (y > 0x80) {
                        y -= 0x80;
                    } else if (y < -0x80) {
                        y += 0x80;
                    }
                    info[0x31] = uint8_t(y);
                }
                s_swayX = float(x * 0.0078125);
                s_swayY = float(0.0078125 * y);
            }
            sway = !NeverSways(self->modelIndex);
        }
        if (!sway) {
            s_swayX = 0.0f;
            s_swayY = 0.0f;
        }
        SetTreeFloats(Field<void*>(object, 0x14), &s_swayX);
        SetObjectWorldTransform(object, static_cast<uint8_t*>(Frame_GetParent(object)) + 0x14);
        if (Field<uint32_t>(mi, 0x28) & 0x20000) {
            if (self->flagE8 == 0) {
                uint32_t savedA, savedB;
                GetRenderState(0xA, &savedA);
                GetRenderState(0xB, &savedB);
                SetRenderState(0xA, 3);
                SetRenderState(0xB, 1);
                SetRenderState(0x1D, 5);
                SetRenderState(0x1E, 200);
                RenderAtomic(object);
                SetRenderState(0xA, savedA);
                SetRenderState(0xB, savedB);
            } else {
                RenderWithCameraAlpha(self, [&] { RenderAtomic(object); });
            }
        } else if (self->flagE8) {
            RenderWithCameraAlpha(self, [&] { RenderAtomic(object); });
        } else {
            RenderAtomic(object);
        }
        Node_AttachChild(object, nullptr);
    } else {
        // a clump: its bones and attachments updated, rendered (faded peds twice: their
        // depth first)
        if (object[0x34]) {
            UpdateAttachedBoneMatrices(object);
        } else {
            UpdateAttachedTransforms(object);
        }
        GetRenderState(8, &s8);
        SetRenderState(8, zWrite ? 1 : 0);
        if (faded && !restoreStates) {
            GetRenderState(6, &s6);
            GetRenderState(0xA, &sA);
            GetRenderState(0xB, &sB);
            SetRenderState(0x1D, 8);
            SetRenderState(6, 1);
            SetRenderState(0xA, 1);
            SetRenderState(0xB, 2);
            if (self->flagE8) {
                RenderWithCameraAlpha(self, [&] { Clump_Render(self->rwObject); });
            } else {
                Clump_Render(self->rwObject);
            }
            SetRenderState(0xA, 5);
            SetRenderState(0xB, 6);
        }
        if (self->flagE8) {
            RenderWithCameraAlpha(self, [&] { Clump_Render(self->rwObject); });
        } else {
            Clump_Render(self->rwObject);
        }
        SetRenderState(8, s8);
        if (faded && !restoreStates) {
            SetRenderState(0x1D, 7);
            SetRenderState(0x1E, 0x80);
            SetRenderState(6, s6);
            SetRenderState(0xA, sA);
            SetRenderState(0xB, sB);
            self->bIsBeingRendered = 0;
            return;
        }
    }
    if (restoreStates) {
        SetRenderState(8, s8);
        SetRenderState(6, s6);
        SetRenderState(0xC, sC);
        SetRenderState(0xA, sA);
        SetRenderState(0xB, sB);
        SetRenderState(7, s7);
        SetRenderState(0xE, sE);
    }
    self->bIsBeingRendered = 0;
}

} // namespace

REVERSED("CEntity::CreateRwObject", 0x004662F0, RE_BODY(&CreateRwObject), nullptr, nullptr, re::Mode::Original);
REVERSED("CEntity::Render", 0x00466D40, RE_BODY(&Render), nullptr, nullptr, re::Mode::Original);
