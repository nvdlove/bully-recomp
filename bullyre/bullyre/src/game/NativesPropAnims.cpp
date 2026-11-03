// Script natives 0x5C4220..0x5C5DB0 for prop anims (the animated props of the triggers:
// doors, lockers, breakables): playing action nodes, creating them where their triggers
// say, damage, doors opening around a point; and 0x5C6190..0x5C62E0 (the action file
// reference's class id, the jump objective's constructor, ped handles, speeds from arguments). Each native
// is `int __cdecl Native_X(lua_State* L)`, returning how many results it pushed.
//
// A trigger (the trigger pool, 0x20C7C48) describing a prop anim: +4 position, +0x10
// heading (degrees), +0x14 / +0x18 rotations about y and x (degrees), +0x1C its prop,
// +0x20 its model's (+0x10 the model index), +0x38 a heading correction (radians),
// +0x3C the area (signed byte), +0x3D a kind (5: never created), +0x3F bit 0 disabled.

#include <cmath>
#include <cstdint>

#include "game/NativesMPP.h"
#include "reversed.h"

namespace {

GAME_FN(uint8_t*, __thiscall, PA_PropAnims_Get, 0x006D7B10, void* wrapper, int id);
GAME_FN(uint8_t*, __thiscall, PA_Triggers_GetBySlot, 0x00415FD0, void* wrapper, int slot);
GAME_FN(uint8_t*, __thiscall, PA_Triggers_FromHash, 0x00436C10, void* wrapper, void* hash);
GAME_FN(uint8_t*, __cdecl, PA_GetPropAnimByHash, 0x0044A610, void* hash);
GAME_FN(uint8_t*, __cdecl, PA_GetEntityOfKind, 0x0044A660, int handle, int kind);
GAME_FN(int, __cdecl, PA_GetModelIndex, 0x0051C330, const char* name);
GAME_FN(uint8_t*, __cdecl, PA_FindNearestObjectOfTypeInRange, 0x004607A0, int model, const float* pos, float range,
        int flags);
GAME_FN(void, __thiscall, PropAnim_SetCurrent, 0x004ED940, void* prop);
GAME_FN(uint8_t, __thiscall, ActionController_IsPlaying, 0x005F3D50, void* controller, const char* node, int);
GAME_FN(uint8_t, __thiscall, ActionController_IsPlayingNode, 0x005F49A0, void* controller, const char* node, int, int);
inline void* const pa_pathFollowers = reinterpret_cast<void*>(0x00D13730);
GAME_FN(void, __thiscall, PathFollowers_GetPos, 0x005EBDC0, void* self, void* entity, float* pos);
GAME_FN(void, __thiscall, PathFollowers_Rotate, 0x005EC180, void* self, void* entity, float a, float b);
GAME_FN(uint8_t*, __cdecl, GetPropAnimArg, 0x005C4570, lua_State* L, int n, int* next);
GAME_FN(void*, __cdecl, PA_GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, PA_RequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __thiscall, PA_Script_AddModel, 0x005D9450, void* script, int id);
GAME_FN(void, __thiscall, Script_AddActionTree, 0x005D9AF0, void* script, int id);
GAME_FN(void, __cdecl, PA_LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);
GAME_FN(void, __cdecl, PA_LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(float*, __cdecl, CombineRotation, 0x00415EB0, float* out, const float* axis, float degrees, int mode);
GAME_FN(float*, __cdecl, PA_CombineTranslation, 0x00415F50, float* out, const float* translation, int mode);
GAME_FN(void, __thiscall, NiTransform_Construct, 0x00770920, float* self);
GAME_FN(float*, __thiscall, NiTransform_Multiply, 0x00406830, const float* self, float* out, const float* other);
GAME_FN(uint8_t*, __cdecl, PA_CreateTriggerObject, 0x0044CE70, int model, int area, float* transform, float, void* trigger,
        int, int);
GAME_FN(void, __cdecl, PA_World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __cdecl, PA_World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, PA_Entity_45E690, 0x0045E690, void* entity);
GAME_FN(uint8_t, __cdecl, ColStore_IsLoaded, 0x0056F450, int slot);
GAME_FN(uint8_t, __cdecl, PA_LoadCol, 0x0056FAC0, int slot, void* data, uintptr_t size);
GAME_FN(void, __thiscall, PropAnim_Reset, 0x004EF570, void* prop);
GAME_FN(void, __thiscall, Trigger_GetCentre, 0x006D6F10, void* trigger, float* out);
GAME_FN(float, __thiscall, Trigger_GetRadius, 0x006D6F40, void* trigger, float* centre);
GAME_FN(uint8_t, __thiscall, PA_Trigger_ContainsPoint, 0x006D6FA0, void* trigger, const float* point);
GAME_FN(void, __cdecl, FindObjectsOfTypeInRange, 0x0045E0D0, int model, const float* centre, float radius, int16_t* count,
        int max, re::Ptr32<uint8_t>* out, int flags);
GAME_ARRAY(uint8_t*, g_foundEntities, 0x00C1AA80);   // 250
GAME_FN(int, __thiscall, PA_Pool_ChainSize, 0x00435BB0, void* pool);
GAME_FN(uint8_t*, __thiscall, PA_Pool_ChainAt, 0x00435BD0, void* pool, int slot, int);
GAME_FN(void, __thiscall, Object_Init, 0x004CFCE0, void* object);
GAME_FN(void, __thiscall, PA_PlayAnim, 0x004EA450, void* prop, int anim, int);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(int, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int16_t, __thiscall, TriggerPool_GetSize, 0x006D70A0, void* wrapper);
GAME_FN(uint8_t*, __thiscall, ModelInfo_GetAnimInfo, 0x005201D0, void* mi);
GAME_VAR(uint8_t, g_chapterIndex, 0x00A147E0);
GAME_ARRAY(int32_t, g_chapterSeason, 0x00A147EC);
GAME_FN(int16_t, __cdecl, FindPAnimIndex, 0x00520960, const char* name, void* mi);
GAME_FN(void*, __cdecl, FindActionNode, 0x005F5B10, const char* path, uintptr_t);
GAME_FN(void, __thiscall, PropAnim_SetActionNode, 0x004EF340, void* prop, void* node, int group);
GAME_FN(uint8_t, __cdecl, PA_IsModelLoaded, 0x004151E0, int model);
GAME_FN(uint8_t, __cdecl, PA_IsModelRequested, 0x004151C0, int model);
GAME_FN(uint8_t, __cdecl, PA_IsModelLoading, 0x00450C00, int model);
GAME_FN(int16_t, __thiscall, PAnimMI_GetAnimIndex, 0x00520370, void* mi);
GAME_FN(void, __cdecl, PA_SetModelNeeded, 0x0052E720, int model);
GAME_FN(uint8_t*, __cdecl, GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_VAR(uint32_t*, ms_pPtrNodes, 0x00C0F788);
GAME_ARRAY(uint32_t*, ms_sectorLists, 0x00C1B17C);   // 36 x 36 sectors, 5 dwords each: [0] the list
GAME_VAR(uint16_t, ms_nCurrentScanCode, 0x00C1AE88);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(uint32_t, __thiscall, PAnimModel_520440, 0x00520440, void* mi);
GAME_FN(void, __thiscall, PropAnim_OpenDoor, 0x004EED10, void* prop, int, uintptr_t, int);
GAME_FN(float*, __cdecl, PA_VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(void*, __thiscall, Objective_Construct, 0x0046FA90, void* self, int type, void* ped);
GAME_VAR(int16_t, pa_nextTypeId, 0x00BF3830);
GAME_VAR(uint32_t, g_fileRefIdFlags, 0x00CFB378);
GAME_VAR(int16_t, g_fileRefId, 0x00CFB374);

void* PA_CurrentScript() {
    return M_Scripts_GetCurrent(m_lurStore);
}

// The rotation the game gives a trigger's prop: about z by heading - the correction
// (degrees), then y and x
void TriggerRotation(float* transform, uint8_t* trigger) {
    float axisY[3] = {0.0f, 1.0f, 0.0f};
    float axisZ[3] = {0.0f, 0.0f, 1.0f};
    float axisX[3] = {1.0f, 0.0f, 0.0f};
    float heading = float(double(At<float>(trigger, 0x10)) - double(At<float>(trigger, 0x38)) * 180.0 / 3.1415927410125732);
    CombineRotation(transform, axisZ, heading, 0);
    CombineRotation(transform, axisY, At<float>(trigger, 0x14), 2);
    CombineRotation(transform, axisX, At<float>(trigger, 0x18), 2);
}

// (prop, node path, flag) or (model, x, y, z, range, node path, flag): the prop's action
// controller (+0x128) plays the node (a path, starting with \ or /)
int __cdecl Native_PAnimIsPlaying(lua_State* L) {
    if (lua::gettop(L) == 3) {
        if (Script::IsNumber(L, 0) == 0 && Script::IsUserdata(L, 0) == 0) {
            return 0;
        }
        if (Script::IsString(L, 1) == 0 || Script::IsBoolean(L, 2) == 0) {
            return 0;
        }
        const char* node = Script::GetString(L, 1);
        if (node[0] != '\\' && node[0] != '/') {
            return 0;
        }
        uint8_t* prop;
        if (Script::IsNumber(L, 0) != 0) {
            prop = PA_PropAnims_Get(m_triggerPoolWrapper, Script::GetInt(L, 0));
        } else {
            if (Script::IsUserdata(L, 0) == 0) {
                return 0;
            }
            prop = PA_GetPropAnimByHash(Script::GetUserdata(L, 0));
        }
        if (prop == nullptr) {
            return 0;
        }
        PropAnim_SetCurrent(prop);
        uint8_t flag = Script::GetBool(L, 2);
        Script::PushBool(L, ActionController_IsPlaying(prop + 0x128, node, flag));
        return 1;
    }
    if (lua::gettop(L) != 7) {
        return 0;
    }
    int model;
    if (Script::IsNumber(L, 0) != 0) {
        model = Script::GetInt(L, 0);
    } else {
        if (Script::IsString(L, 0) == 0) {
            return 0;
        }
        model = PA_GetModelIndex(Script::GetString(L, 0));
    }
    if (Script::IsNumber(L, 1) == 0 || Script::IsNumber(L, 2) == 0 || Script::IsNumber(L, 3) == 0
        || Script::IsNumber(L, 4) == 0 || Script::IsString(L, 5) == 0 || Script::IsBoolean(L, 6) == 0) {
        return 0;
    }
    const char* node = Script::GetString(L, 5);
    if (node[0] != '\\' && node[0] != '/') {
        return 0;
    }
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    float range = Script::GetFloat(L, 4);
    float pos[3] = {x, y, z};
    uint8_t* prop = PA_FindNearestObjectOfTypeInRange(model, pos, range, 8);
    if (prop == nullptr || (At<uint8_t>(prop, 0x108) & 7) != 6) {
        return 0;
    }
    PropAnim_SetCurrent(prop);
    uint8_t flag = Script::GetBool(L, 6);
    Script::PushBool(L, ActionController_IsPlaying(prop + 0x128, node, flag));
    return 1;
}

// (prop) -> x, y, z (along its path, if following one)
int __cdecl Native_PAnimGetPosition(lua_State* L) {
    uint8_t* prop = PA_PropAnims_Get(m_triggerPoolWrapper, Script::GetInt(L, 0));
    if (prop == nullptr) {
        return 0;
    }
    float* p = EntityPos(prop);
    float pos[3] = {p[0], p[1], p[2]};
    PathFollowers_GetPos(pa_pathFollowers, prop, pos);
    Script::PushFloat(L, pos[0]);
    Script::PushFloat(L, pos[1]);
    Script::PushFloat(L, pos[2]);
    return 3;
}

// 0x5C4570: the prop-anim argument for a native taking n more: (prop | trigger hash)
// with n + 1 arguments (*next 1), (handle, kind) with n + 2 (*next 2), else (model, x,
// y, z) the nearest within 1 (*next 4)
uint8_t* __cdecl GetPropAnimArgImpl(lua_State* L, int n, int* next) {
    if (Script::GetArgCount(L) == n + 1) {
        if (Script::IsNumber(L, 0) != 0) {
            uint8_t* prop = PA_PropAnims_Get(m_triggerPoolWrapper, Script::GetInt(L, 0));
            *next = 1;
            return prop;
        }
        if (Script::IsUserdata(L, 0) != 0) {
            uint8_t* trigger = PA_Triggers_FromHash(m_triggerPoolWrapper, Script::GetUserdata(L, 0));
            uint8_t* prop = At<uint8_t*>(trigger, 0x1C);
            *next = 1;
            return prop;
        }
        *next = 0;
        return nullptr;
    }
    if (Script::GetArgCount(L) == n + 2) {
        int handle = Script::GetInt(L, 0);
        int kind = Script::GetInt(L, 1);
        uint8_t* e = PA_GetEntityOfKind(handle, kind);
        *next = 2;
        return e;
    }
    int model;
    if (Script::IsNumber(L, 0) != 0) {
        model = Script::GetInt(L, 0);
    } else {
        model = PA_GetModelIndex(Script::GetString(L, 0));
    }
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    uint8_t* prop = nullptr;
    if (model != -1) {
        float pos[3] = {x, y, z};
        prop = PA_FindNearestObjectOfTypeInRange(model, pos, 1.0f, 8);
    }
    *next = 4;
    return prop;
}

// (trigger) -> the model's loaded (and the collision's): else requested (true for none)
int __cdecl Native_PAnimRequest(lua_State* L) {
    uint8_t* trigger = PoolSlot(m_triggerPool, Script::GetInt(L, 0));
    // (a trigger not loaded (its area's not streamed in: the slot free or the index -1): the
    // game read its +0x20 through the null and stopped; nothing done)
    if (trigger == nullptr) {
        Script::PushBool(L, 1);
        return 1;
    }
    int model = At<int32_t>(At<uint8_t*>(trigger, 0x20), 0x10);
    if (model == -1) {
        Script::PushBool(L, 1);
        return 1;
    }
    uint8_t* mi = m_modelInfoPtrs[model];
    uint8_t loaded = 1;
    if (At<uint8_t>(PA_GetStreamingInfo(model), 4) != 1) {
        PA_RequestModel(model, 0x1E);
        loaded = 0;
    } else {
        uint8_t* col = At<uint8_t*>(mi, 0xC);
        if (col != nullptr && At<uint16_t>(col, 0x30) != 0
            && At<uint8_t>(PA_GetStreamingInfo(At<uint16_t>(col, 0x30) + 0x56B8), 4) != 1) {
            PA_RequestModel(At<uint16_t>(At<uint8_t*>(mi, 0xC), 0x30) + 0x56B8, 0x16);
            loaded = 0;
        }
    }
    PA_Script_AddModel(PA_CurrentScript(), model);
    Script::PushBool(L, loaded);
    return 1;
}

// (trigger, [a = 1]): its prop created at its transform (the model loaded now if need
// be), reset
int __cdecl Native_PAnimCreate(lua_State* L) {
    Script::GetArgCount(L);
    uint8_t* trigger = PoolSlot(m_triggerPool, Script::GetInt(L, 0));
    // (a trigger not loaded (its area's not streamed in: the slot free or the index -1): the
    // game read its +0x20 through the null and stopped; nothing done)
    if (trigger == nullptr) {
        return 0;
    }
    uint8_t a = 1;
    if (Script::GetArgCount(L) >= 3) {
        a = Script::GetBool(L, 2);
    }
    float rotation[13];   // an NiTransform
    rotation[12] = 1.0f;
    TriggerRotation(rotation, trigger);
    float placement[13];
    placement[12] = 1.0f;
    NiTransform_Construct(placement);
    placement[9] = At<float>(trigger, 4);
    placement[10] = At<float>(trigger, 8);
    placement[11] = At<float>(trigger, 0xC);
    float product[13];
    float* result = NiTransform_Multiply(placement, product, rotation);
    for (int i = 0; i < 13; i++) {
        rotation[i] = result[i];
    }
    int model = At<int32_t>(At<uint8_t*>(trigger, 0x20), 0x10);
    if (model == -1) {
        return 0;
    }
    uint8_t* prop = PA_CreateTriggerObject(model, At<int8_t>(trigger, 0x3C), rotation, 0.0f, trigger, a, 0);
    PA_World_Add(prop);
    model = At<int32_t>(At<uint8_t*>(trigger, 0x20), 0x10);
    uint8_t* mi = m_modelInfoPtrs[model];
    if (At<uint8_t>(PA_GetStreamingInfo(model), 4) != 1) {
        PA_RequestModel(model, 0x16);
        PA_LoadingScreen("cmdPAnimCreate()", "CStreaming::LoadAllRequestedModels()", 0);
        PA_LoadAllRequestedModels(0);
        uint16_t col = At<uint16_t>(At<uint8_t*>(mi, 0xC), 0x30);
        if (col != 0 && ColStore_IsLoaded(col) == 0) {
            PA_LoadCol(At<uint16_t>(At<uint8_t*>(mi, 0xC), 0x30), nullptr, 0);
        }
        PA_Script_AddModel(PA_CurrentScript(), model);
    }
    if (prop != nullptr) {
        PropAnim_Reset(prop);
    }
    return 0;
}

// (trigger): its prop deleted
int __cdecl Native_PAnimDelete(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    uint8_t* trigger = PoolSlot(m_triggerPool, slot);
    if (trigger == nullptr) {
        return 0;
    }
    uint8_t* prop = At<uint8_t*>(trigger, 0x1C);
    if (prop != nullptr) {
        PA_World_Remove(prop);
        PA_Entity_45E690(prop);
        VCall<void>(prop, 0, 1);
    }
    At<void*>(trigger, 0x1C) = nullptr;
    return 0;
}

// (trigger, [model name]) -> how many destroyed props (of the model) are in it
int __cdecl Native_PAnimNumDestroyed(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    int model = -1;
    if (lua::gettop(L) == 2) {
        const char* name = Script::GetString(L, 1);
        if (name != nullptr) {
            model = PA_GetModelIndex(name);
        }
    }
    uint8_t* trigger = PoolSlot(m_triggerPool, slot);
    // (a trigger not loaded (its area's not streamed in: the slot free or the index -1): the
    // game read its +0x20 through the null and stopped; nothing done)
    if (trigger == nullptr) {
        Script::PushInt(L, 0);
        return 1;
    }
    float centre2[2];
    Trigger_GetCentre(trigger, centre2);
    float radius = Trigger_GetRadius(trigger, centre2);
    float centre[3] = {centre2[0], centre2[1], 0.0f};
    int16_t count = -1;
    FindObjectsOfTypeInRange(model, centre, radius, &count, 0xFA, g_foundEntities, 0x409);
    int destroyed = 0;
    for (int i = 0; i < count; i++) {
        if (VCall<uint8_t>(g_foundEntities[i], 0x78) != 0
            && PA_Trigger_ContainsPoint(trigger, EntityPos(g_foundEntities[i])) != 0) {
            destroyed++;
        }
    }
    Script::PushInt(L, destroyed);
    return 1;
}

// ([all]): damaged objects re-initialised; damaged prop anims (or all) reset (all: +0xD0
// cleared). (The object count is the pool's ChainSize, taken again each step)
int __cdecl Native_PAnimResetAllDamageable(lua_State* L) {
    CPool<uint8_t>* props = m_propAnimPool;
    CPool<uint8_t>* objects = m_objectPool;
    for (int i = 0; i < PA_Pool_ChainSize(objects); i++) {
        uint8_t* object = nullptr;
        if (i < objects->size) {
            object = objects->objects + objects->elemSize * i;
        } else if (objects->Next() != nullptr) {
            object = PA_Pool_ChainAt(objects->Next(), i - objects->size, 1);
        }
        if ((objects->flags[i] & 0x80) == 0 && object != nullptr && VCall<uint8_t>(object, 0x7C) != 0) {
            Object_Init(object);
        }
    }
    uint8_t all = 0;
    if (Script::GetArgCount(L) == 1) {
        all = Script::GetBool(L, 0);
    }
    for (int i = 0; i < props->size; i++) {
        if ((props->flags[i] & 0x80) != 0) {
            continue;
        }
        uint8_t* prop = props->objects + props->elemSize * i;
        if (prop == nullptr) {
            continue;
        }
        if (VCall<uint8_t>(prop, 0x7C) != 0 || all != 0) {
            PropAnim_Reset(prop);
            if (all != 0) {
                At<int32_t>(prop, 0xD0) = 0;
            }
        }
    }
    return 0;
}

// (prop..., invulnerable): an object's bit 6 of +0x209, a prop anim's anim 2
int __cdecl Native_PAnimSetInvulnerable(lua_State* L) {
    int next = -1;
    uint8_t* e = GetPropAnimArg(L, 1, &next);
    if (e == nullptr) {
        return 0;
    }
    uint8_t on = Script::GetBool(L, next);
    uint8_t type = At<uint8_t>(e, 0x108) & 7;
    if (type == 4) {
        uint8_t& flags = At<uint8_t>(e, 0x209);
        flags ^= (uint8_t(on << 6) ^ flags) & 0x40;
        return 0;
    }
    if (type == 6) {
        PA_PlayAnim(e, 2, on);
    }
    return 0;
}

// (prop..., damage): its virtual +0x3C
int __cdecl Native_PAnimApplyDamage(lua_State* L) {
    int next = -1;
    uint8_t* e = GetPropAnimArg(L, 1, &next);
    if (e != nullptr) {
        int damage = Script::GetInt(L, next);
        VCall<void>(e, 0x3C, float(damage), 0.0f, 0, -1, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
    }
    return 0;
}

// (area): prop anims of other areas deleted (not the world trigger's, disabled ones or
// kind 5), then the area's created (those of this season's chapter), their memory
// tagged 2
int __cdecl Native_PAnimAreaProps(lua_State* L) {
    int area = Script::GetInt(L, 0);
    CPool<uint8_t>* props = m_propAnimPool;
    int i = props->size;
    uint8_t* world = (m_triggerPool->flags[0] & 0x80) == 0 ? m_triggerPool->objects : nullptr;
    while (i != 0) {
        i--;
        if ((props->flags[i] & 0x80) != 0) {
            continue;
        }
        uint8_t* prop = props->objects + props->elemSize * i;
        if (prop == nullptr) {
            continue;
        }
        uint8_t* trigger = At<uint8_t*>(prop, 0x23C);
        if (trigger == nullptr || trigger == world || (At<uint8_t>(trigger, 0x3F) & 1) != 0
            || At<int8_t>(trigger, 0x3C) == area || At<uint8_t>(trigger, 0x3D) == 5) {
            continue;
        }
        if (At<void*>(prop, 0x18) != nullptr) {
            Entity_DeleteRwObject(prop);
        }
        VCall<void>(prop, 0xC);
        PA_World_Remove(prop);
        PA_Entity_45E690(prop);
        VCall<void>(prop, 0, 1);
        At<void*>(trigger, 0x1C) = nullptr;
    }
    PushMemoryTag(2);
    int count = TriggerPool_GetSize(m_triggerPoolWrapper);
    for (int slot = 1; slot < count; slot++) {
        uint8_t* trigger = PoolSlot(m_triggerPool, slot);
        if (trigger == nullptr || (At<uint8_t>(trigger, 0x3F) & 1) != 0) {
            continue;
        }
        int model = At<int32_t>(At<uint8_t*>(trigger, 0x20), 0x10);
        if (model == -1 || At<int8_t>(trigger, 0x3C) != area || At<uint8_t>(trigger, 0x3D) == 5) {
            continue;
        }
        uint8_t* info = ModelInfo_GetAnimInfo(m_modelInfoPtrs[model]);
        int season = g_chapterSeason[g_chapterIndex];
        bool now = (At<uint8_t>(info, 0x74) != 0 && season == 0) || (At<uint8_t>(info, 0x75) != 0 && season == 1)
                   || (At<uint8_t>(info, 0x77) != 0 && season == 3) || (At<uint8_t>(info, 0x76) != 0 && season == 2);
        if (!now) {
            continue;
        }
        float transform[13];
        transform[12] = 1.0f;
        TriggerRotation(transform, trigger);
        PA_CombineTranslation(transform, reinterpret_cast<float*>(trigger + 4), 2);
        uint8_t* prop = PA_CreateTriggerObject(At<int32_t>(At<uint8_t*>(trigger, 0x20), 0x10), At<int8_t>(trigger, 0x3C),
                                               transform, 0.0f, trigger, 1, 0);
        PA_World_Add(prop);
    }
    PopMemoryTag();
    return 0;
}

// (prop..., a, b)
int __cdecl Native_PAnimRotate(lua_State* L) {
    int next = -1;
    uint8_t* e = GetPropAnimArg(L, 2, &next);
    if (e != nullptr) {
        float a = Script::GetFloat(L, next);
        float b = Script::GetFloat(L, next + 1);
        PathFollowers_Rotate(pa_pathFollowers, e, a, b);
    }
    return 0;
}

// (prop..., node) -> its action controller plays the node (1, 1)
int __cdecl Native_PAnimIsPlayingNode(lua_State* L) {
    uint8_t playing = 0;
    int next = -1;
    uint8_t* prop = GetPropAnimArg(L, 1, &next);
    if (prop != nullptr && (At<uint8_t>(prop, 0x108) & 7) == 6) {
        const char* node = Script::GetString(L, next);
        PropAnim_SetCurrent(prop);
        playing = ActionController_IsPlayingNode(prop + 0x128, node, 1, 1);
    }
    Script::PushBool(L, playing);
    return 1;
}

// (trigger | hash, node path, tree) or (model, x, y, z, range, node path, tree): the
// node (its tree loaded now if need be) set on the prop
int __cdecl Native_PAnimSetActionNode(lua_State* L) {
    uint8_t* prop;
    int i;
    if (lua::gettop(L) == 3) {
        if (Script::IsNumber(L, 0) == 0 && Script::IsUserdata(L, 0) == 0) {
            return 0;
        }
        if (Script::IsString(L, 1) == 0 || Script::IsString(L, 2) == 0) {
            return 0;
        }
        if (Script::IsNumber(L, 0) != 0) {
            uint8_t* trigger = PA_Triggers_GetBySlot(m_triggerPoolWrapper, Script::GetInt(L, 0));
            i = 1;
            if (trigger == nullptr) {
                return 0;
            }
            prop = At<uint8_t*>(trigger, 0x1C);
        } else {
            if (Script::IsUserdata(L, 0) == 0) {
                return 0;
            }
            prop = PA_GetPropAnimByHash(Script::GetUserdata(L, 0));
            i = 1;
        }
    } else {
        if (lua::gettop(L) != 7) {
            return 0;
        }
        int model;
        if (Script::IsNumber(L, 0) != 0) {
            model = Script::GetInt(L, 0);
        } else {
            if (Script::IsString(L, 0) == 0) {
                return 0;
            }
            model = PA_GetModelIndex(Script::GetString(L, 0));
        }
        if (Script::IsNumber(L, 1) == 0 || Script::IsNumber(L, 2) == 0 || Script::IsNumber(L, 3) == 0
            || Script::IsNumber(L, 4) == 0 || Script::IsString(L, 5) == 0 || Script::IsString(L, 6) == 0) {
            return 0;
        }
        float x = Script::GetFloat(L, 1);
        float y = Script::GetFloat(L, 2);
        float z = Script::GetFloat(L, 3);
        float range = Script::GetFloat(L, 4);
        i = 5;
        float pos[3] = {x, y, z};
        prop = PA_FindNearestObjectOfTypeInRange(model, pos, range, 8);
        if (prop == nullptr || (At<uint8_t>(prop, 0x108) & 7) != 6) {
            return 0;
        }
    }
    if (prop == nullptr) {
        return 0;
    }
    const char* path = Script::GetString(L, i);
    if (path[0] != '\\' && path[0] != '/') {
        return 0;
    }
    int group = FindPAnimIndex(Script::GetString(L, i + 1), nullptr);
    void* node = FindActionNode(path, 0);
    if (node == nullptr) {
        if (group == -1) {
            return 0;
        }
        PA_RequestModel(group + 0x5D41, 0x13);
        PA_LoadingScreen("cmdPAnimSetActionNode()", "CStreaming::LoadAllRequestedModels()", 0);
        PA_LoadAllRequestedModels(0);
        node = FindActionNode(path, 0);
        if (node == nullptr) {
            return 0;
        }
    }
    if (group != -1) {
        Script_AddActionTree(PA_CurrentScript(), group);
    }
    PropAnim_SetActionNode(prop, node, group);
    return 0;
}

// Loads a model now unless it's loaded, requested or loading
void LoadNow(int model, int flags, const char* title) {
    if (PA_IsModelLoaded(model) != 0) {
        return;
    }
    if (PA_IsModelRequested(model) == 0 && PA_IsModelLoading(model) == 0) {
        PA_RequestModel(model, flags);
    }
    PA_LoadingScreen(title, "CStreaming::LoadAllRequestedModels()", 0);
    PA_LoadAllRequestedModels(0);
}

// (model name or index, x, y, z, heading, area): a prop anim with no trigger, its model,
// collision and anims loaded now, raised by its collision's +0x18
int __cdecl Native_PAnimForceCreate(lua_State* L) {
    if (Script::GetArgCount(L) != 6) {
        return 0;
    }
    if (Script::IsNumber(L, 1) == 0 || Script::IsNumber(L, 2) == 0 || Script::IsNumber(L, 3) == 0
        || Script::IsNumber(L, 4) == 0 || Script::IsNumber(L, 5) == 0) {
        return 0;
    }
    int model;
    if (Script::IsString(L, 0) != 0) {
        model = PA_GetModelIndex(Script::GetString(L, 0));
    } else {
        if (Script::IsNumber(L, 0) == 0) {
            return 0;
        }
        model = Script::GetInt(L, 0);
    }
    uint8_t* mi = m_modelInfoPtrs[model];
    LoadNow(model, 0x16, "cmdPAnimForceCreate() - 1");
    uint8_t* col = At<uint8_t*>(mi, 0xC);
    float raise;
    if (col != nullptr) {
        if (At<uint16_t>(col, 0x30) != 0) {
            PA_RequestModel(At<uint16_t>(col, 0x30) + 0x56B8, 0x16);
            PA_LoadingScreen("cmdPAnimForceCreate() - 2", "CStreaming::LoadAllRequestedModels()", 0);
            PA_LoadAllRequestedModels(0);
        }
        raise = At<float>(col, 0x18);
    } else {
        raise = 0.0f;
    }
    float axis[3] = {0.0f, 0.0f, 1.0f};
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3) + raise;
    float heading = Script::GetFloat(L, 4);
    int area = Script::GetInt(L, 5);
    float pos[3] = {x, y, z};
    float transform[13];
    transform[12] = 1.0f;
    CombineRotation(transform, axis, heading, 0);
    PA_CombineTranslation(transform, pos, 2);
    uint8_t* prop = PA_CreateTriggerObject(model, area, transform, 0.0f, nullptr, 0, 0);
    PA_World_Add(prop);
    int anim = PAnimMI_GetAnimIndex(mi);
    if (anim != -1) {
        LoadNow(anim, 0, "cmdPAnimForceCreate() - 3");
    }
    int other = At<int32_t>(ModelInfo_GetAnimInfo(mi), 0x88);
    if (other != -1) {
        LoadNow(other, 0, "cmdPAnimForceCreate() - 4");
    }
    VCall<void>(prop, 0x28, 1, 1);
    return 0;
}

// (prop...): deleted
int __cdecl Native_PAnimForceDelete(lua_State* L) {
    int next;
    uint8_t* e = GetPropAnimArg(L, 0, &next);
    if (e != nullptr) {
        PA_World_Remove(e);
        PA_Entity_45E690(e);
        VCall<void>(e, 0, 1);
    }
    return 0;
}

// (trigger): its model marked needed (and the script's)
int __cdecl Native_PAnimModelNeeded(lua_State* L) {
    uint8_t* trigger = PoolSlot(m_triggerPool, Script::GetInt(L, 0));
    // (a trigger not loaded (its area's not streamed in: the slot free or the index -1): the
    // game read its +0x20 through the null and stopped; nothing done)
    if (trigger == nullptr) {
        return 0;
    }
    int model = At<int32_t>(At<uint8_t*>(trigger, 0x20), 0x10);
    if (model != -1) {
        PA_SetModelNeeded(model);
        PA_Script_AddModel(PA_CurrentScript(), model);
    }
    return 0;
}

// (model name, x, y, z, range) -> (handle, kind) of the nearest
int __cdecl Native_PAnimGetPoolIndex(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    float range = Script::GetFloat(L, 4);
    int model = PA_GetModelIndex(name);
    float pos[3] = {x, y, z};
    uint8_t* e = PA_FindNearestObjectOfTypeInRange(model, pos, range, 9);
    int handle = -1;
    int kind = -1;
    if (e != nullptr) {
        switch (At<uint8_t>(e, 0x108) & 7) {
        case 6:
            kind = HANDLE_PROPANIM;
            handle = PoolHandle(m_propAnimPool, e);
            break;
        case 4:
            handle = PoolHandle(m_objectPool, e);
            kind = HANDLE_OBJECT;
            break;
        case 1:
            kind = HANDLE_BUILDING;
            handle = PoolHandle(m_buildingPool, e);
            break;
        }
    }
    Script::PushInt(L, handle);
    Script::PushInt(L, kind);
    return 2;
}

// (x, y, z, [radius = 0], [model name]): the doors (prop anims whose model allows) in the
// sectors around opened: every one within the radius, or with radius 0 the nearest
int __cdecl Native_PAnimOpenDoors(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    int model = -1;
    float pos[3] = {x, y, z};
    float radius = 0.0f;
    if (Script::GetArgCount(L) >= 4) {
        radius = Script::GetFloat(L, 3);
    }
    if (Script::GetArgCount(L) >= 5) {
        model = PA_GetModelIndex(Script::GetString(L, 4));
    }
    uint8_t* nearest = nullptr;
    float nearestDist = 10000.0f;
    float x2 = pos[0] + radius;
    float y1 = pos[1] - radius;
    float y2 = pos[1] + radius;
    float x1 = pos[0] - radius;
    float f = float(int(double(x1) / 50.0 + 18.0));
    int sx1 = int(f > 0.0f ? f : 0.0f);
    f = float(int(double(y1) / 50.0 + 18.0));
    int sy1 = int(f > 0.0f ? f : 0.0f);
    f = float(int(double(x2) / 50.0 + 18.0));
    int sx2 = int(f < 35.0f ? f : 35.0f);
    f = float(int(double(y2) / 50.0 + 18.0));
    int sy2 = int(f < 35.0f ? f : 35.0f);
    if (ms_nCurrentScanCode >= 0xFFFF) {
        ClearScanCodes();
        ms_nCurrentScanCode = 1;
    } else {
        ms_nCurrentScanCode++;
    }
    for (int sy = sy1; sy <= sy2; sy++) {
        for (int sx = sx1; sx <= sx2; sx++) {
            uint32_t* node = ms_sectorLists[(sy * 36 + sx) * 5];
            while (node != nullptr) {
                uint8_t* e = GetEntityFromNode(*node & 0xF, (*node >> 4) & 0x3FFF);
                uint32_t next = *node >> 18;
                node = next == 0x3FFF ? nullptr : ms_pPtrNodes + next;
                if (model != -1 && model != At<int16_t>(e, 0x10E)) {
                    continue;
                }
                if (At<uint16_t>(e, 0x10A) == ms_nCurrentScanCode || (At<uint8_t>(e, 0x108) & 7) != 6) {
                    continue;
                }
                if (uint8_t(PAnimModel_520440(m_modelInfoPtrs[At<int16_t>(e, 0x10E)])) == 0) {
                    continue;
                }
                At<uint16_t>(e, 0x10A) = ms_nCurrentScanCode;
                float d[3];
                PA_VectorSub(d, EntityPos(e), pos);
                float dist = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
                dist = std::sqrt(std::fabs(dist));
                if (nearestDist > dist) {
                    nearestDist = dist;
                    nearest = e;
                }
                if (radius > dist) {
                    PropAnim_OpenDoor(e, 0, 0, 0);
                }
            }
        }
    }
    if (radius == 0.0f && nearest != nullptr) {
        PropAnim_OpenDoor(nearest, 0, 0, 0);
    }
    return 0;
}

// 0x5C6190: the action file reference nodes' class id (given out on first use; what
// their virtual +4 tests)
int16_t __cdecl ActionFileReference_GetClassId() {
    if ((g_fileRefIdFlags & 1) == 0) {
        g_fileRefIdFlags |= 1;
        g_fileRefId = pa_nextTypeId;
        pa_nextTypeId = int16_t(pa_nextTypeId + 1);
        return g_fileRefId;
    }
    return g_fileRefId;
}

// 0x5C61D0 (thiscall): JumpObjective(ped, target, a, b): type 0x1A, vtable 0x90F9E4
void* __fastcall JumpObjective_Construct(uint8_t* self, re::Edx, void* ped, const float* target, float a, float b) {
    Objective_Construct(self, 0x1A, ped);
    At<uint32_t>(self, 0) = 0x0090F9E4;
    At<float>(self, 0x10) = 0.0f;
    At<float>(self, 0x14) = 0.0f;
    At<float>(self, 0x18) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
    At<float>(self, 0x2C) = 0.0f;
    At<float>(self, 0x30) = 0.0f;
    At<float>(self, 0x10) = target[0];
    At<float>(self, 0x1C) = a;
    At<float>(self, 0x14) = target[1];
    At<float>(self, 0x18) = target[2];
    At<float>(self, 0x20) = b;
    At<uint8_t>(self, 0x24) = 0;
    return self;
}

// 0x5C6230: a ped's handle (its virtual +0xAC), 0 for none
int __cdecl GetPedHandleOrZero(void* ped) {
    if (ped == nullptr) {
        return 0;
    }
    return VCall<int>(ped, 0xAC);
}

// 0x5C6250 (the ped in ecx, L on the stack, the caller cleaning it up: no C++
// convention matches, so not hooked): pushes its handle, 0 for none
[[maybe_unused]] void PushPedHandle(void* ped, lua_State* L) {
    if (ped == nullptr) {
        Script::PushInt(L, 0);
        return;
    }
    Script::PushInt(L, VCall<int>(ped, 0xAC));
}

// 0x5C6280: a speed from argument i (0 walk .. 3 sprint, 4 -1; default 0.3)
float __cdecl GetSpeedArg(lua_State* L, int i) {
    switch (Script::GetIntOpt(L, i, -1)) {
    case 1:
        return 0.69999999f;
    case 2:
        return 1.1f;
    case 3:
        return 1.5f;
    case 4:
        return -1.0f;
    default:
        return 0.30000001f;
    }
}

// 0x5C62E0: the same with a default
float __cdecl GetSpeedArgOpt(lua_State* L, int i, float def) {
    switch (Script::GetIntOpt(L, i, -1)) {
    case 0:
        return 0.30000001f;
    case 1:
        return 0.69999999f;
    case 2:
        return 1.1f;
    case 3:
        return 1.5f;
    case 4:
        return -1.0f;
    default:
        return def;
    }
}

} // namespace

#define PROPANIMS_NATIVE(name, address)                                                \
    static decltype(&Native_##name) Original_PN_##name;                                  \
    REVERSED("Native_" #name, address, RE_BODY(&Native_##name), nullptr, \
             reinterpret_cast<void**>(&Original_PN_##name), re::Mode::Original)
#define PROPANIMS_REVERSED(name, address, fn)                                       \
    static void* RE_CONCAT(Original_PN_, address);                                  \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &RE_CONCAT(Original_PN_, address), \
             re::Mode::Original)

PROPANIMS_NATIVE(PAnimIsPlaying, 0x005C4220);
PROPANIMS_NATIVE(PAnimGetPosition, 0x005C44D0);
PROPANIMS_REVERSED("ScriptNatives::GetPropAnimArg_5C4570", 0x005C4570, GetPropAnimArgImpl);
PROPANIMS_NATIVE(PAnimRequest, 0x005C46F0);
PROPANIMS_NATIVE(PAnimCreate, 0x005C47E0);
PROPANIMS_NATIVE(PAnimDelete, 0x005C4A90);
PROPANIMS_NATIVE(PAnimNumDestroyed, 0x005C4B50);
PROPANIMS_NATIVE(PAnimResetAllDamageable, 0x005C4D70);
PROPANIMS_NATIVE(PAnimSetInvulnerable, 0x005C4E60);
PROPANIMS_NATIVE(PAnimApplyDamage, 0x005C4EE0);
PROPANIMS_NATIVE(PAnimAreaProps, 0x005C4FB0);
PROPANIMS_NATIVE(PAnimRotate, 0x005C52E0);
PROPANIMS_NATIVE(PAnimIsPlayingNode, 0x005C5380);
PROPANIMS_NATIVE(PAnimSetActionNode, 0x005C5400);
PROPANIMS_NATIVE(PAnimForceCreate, 0x005C5870);
PROPANIMS_NATIVE(PAnimForceDelete, 0x005C5B80);
PROPANIMS_NATIVE(PAnimModelNeeded, 0x005C5C50);
PROPANIMS_NATIVE(PAnimGetPoolIndex, 0x005C5CB0);
PROPANIMS_NATIVE(PAnimOpenDoors, 0x005C5DB0);
PROPANIMS_REVERSED("ActionFileReference::GetClassId_5C6190", 0x005C6190, ActionFileReference_GetClassId);
PROPANIMS_REVERSED("JumpObjective::JumpObjective_5C61D0", 0x005C61D0, JumpObjective_Construct);
PROPANIMS_REVERSED("ScriptNatives::GetPedHandleOrZero_5C6230", 0x005C6230, GetPedHandleOrZero);
RE_UNHOOKED("ScriptNatives::PushPedHandle_5C6250", 0x005C6250);
PROPANIMS_REVERSED("ScriptNatives::GetSpeedArg_5C6280", 0x005C6280, GetSpeedArg);
PROPANIMS_REVERSED("ScriptNatives::GetSpeedArgOpt_5C62E0", 0x005C62E0, GetSpeedArgOpt);
