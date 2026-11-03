// Script natives 0x5C94F0..0x5CA310 for peds: weapons (set, now, destroy, clear),
// triggers, swapping models, ambient peds of a model, attacking, catch and pass, action
// and task nodes, conversations. Each is `int __cdecl Native_X(lua_State* L)`, returning
// how many results it pushed. The ped's parts reached here: +0x1C4 its weapon inventory,
// +0x1D0 the weapon held, +0x2E0 its action part (+0x18 the weapon selected, +0x258 /
// +0x290 action controllers), +0x5F0 its objectives.

#include <cstdint>
#include <cstring>

#include "game/NativesMPP.h"

namespace {

GAME_FN(void*, __cdecl, P3_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(uint8_t, __thiscall, P3_Objectives_Add, 0x00471390, void* manager, void* objective);
GAME_FN(uint8_t, __thiscall, P3_Objectives_Prepend, 0x00471470, void* manager, void* objective);
GAME_FN(void, __thiscall, P3_Objectives_Clear, 0x00470350, void* manager);
GAME_FN(uint8_t*, __cdecl, P3_GetWeaponInfo, 0x0051B450, int weapon);
GAME_FN(void, __thiscall, P3_Butes_Resolve, 0x007282A0, void* butes);
GAME_FN(int, __thiscall, Inventory_GetAmmo, 0x0045CF40, void* inventory, int weapon);
GAME_FN(void, __thiscall, Inventory_Give, 0x0045D030, void* inventory, int weapon, int ammo, int flag, int);
GAME_FN(void, __thiscall, Inventory_Remove, 0x0045D0C0, void* inventory, int weapon, int ammo);
GAME_FN(void, __thiscall, Inventory_Clear, 0x0045D190, void* inventory);
GAME_VAR(uint8_t*, p3_weaponHud, 0x00C674F0);
GAME_FN(void, __thiscall, WeaponHud_Select, 0x0055C840, void* hud, int weapon);
GAME_FN(void, __thiscall, WeaponHud_Unequip, 0x0055C8D0, void* hud);
GAME_FN(int, __cdecl, P3_PedSetWeapon, 0x005C94F0, lua_State* L);
GAME_FN(void*, __cdecl, P3_FindActionNode, 0x005F5B10, const char* path, uintptr_t);
GAME_FN(void*, __thiscall, ActionTree_Root, 0x005F5120, void* tree);
GAME_FN(void, __thiscall, ActionController_Play, 0x005F45F0, void* controller, void* node);
GAME_FN(void, __thiscall, ActionController_Update, 0x005F4B00, void* controller, float dt);
GAME_FN(void, __thiscall, ActionController_Stop, 0x005F40A0, void* controller);
GAME_FN(void, __thiscall, ActionContext_SetNode, 0x005F3830, void* context, void* node);
GAME_FN(uint8_t, __thiscall, ActionController_IsPlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(void, __thiscall, Weapon_4D9410, 0x004D9410, void* weapon);
GAME_FN(uint8_t, __thiscall, P3_Trigger_ContainsPoint, 0x006D6FA0, void* trigger, const float* point);
GAME_FN(void*, __cdecl, P3_GetModelInfo, 0x0051C190, const char* name, int* id);
GAME_FN(uint8_t*, __thiscall, Ally_GetTop, 0x00485560, void* ally);
GAME_FN(float, __cdecl, Distance2D, 0x00470110, const float* a, const float* b);
GAME_FN(uint8_t, __thiscall, Ped_ReleasePOI, 0x00473250, void* ped, int);
GAME_FN(int32_t*, __cdecl, MakeAttackKind, 0x00496AF0, int32_t* out, int first, int second);
GAME_FN(void*, __thiscall, AttackPedObjective_Construct, 0x00470D60, void* self, void* ped, void* target, int kind);
GAME_FN(void*, __thiscall, PlayCatchObjective_Construct, 0x004718C0, void* self, void* ped, void* other, int value);
GAME_FN(void*, __thiscall, PassBallObjective_Construct, 0x00471990, void* self, void* ped, void* other, int value);
GAME_FN(void*, __thiscall, ReceiveBallObjective_Construct, 0x00471A60, void* self, void* ped, void* other, int value);
GAME_FN(uint8_t, __thiscall, Ped_SetTaskNode, 0x004740E0, void* ped, const char* node, const char* file);
GAME_FN(uint8_t, __thiscall, Ped_SetActionNode, 0x004740F0, void* ped, const char* node, const char* file);
GAME_FN(void, __thiscall, P3_ActFiles_Load, 0x005DC1A0, void* store, void* a, const void* b, const void* c, void* d);
inline void* const conversations = reinterpret_cast<void*>(0x00B9DAB8);
GAME_FN(void, __thiscall, Conversation_Reset, 0x00683EC0, void* self);
GAME_FN(void, __thiscall, Conversation_AddPed, 0x00683ED0, void* self, void* ped);
GAME_FN(uint8_t, __thiscall, Conversation_Start, 0x00685C50, void* self, const char* a, const char* b);
GAME_FN(int, __cdecl, P3_PedHandleToSlot, 0x0046DF10, int handle);

uint8_t* P3_Ped(lua_State* L, int i) {
    return M_GetPedFromHandle(Script::GetInt(L, i), 2);
}

// A handle's ped, 0 the player's
uint8_t* PedOrPlayer(int handle) {
    if (handle != 0) {
        return M_GetPedFromHandle(handle, 2);
    }
    return m_playerPed;
}

// The action part's context (+0x300) back to its default node (+0x4B0) when set (+0x33C)
void ResetWeaponAction(uint8_t* ped) {
    uint8_t* part = At<uint8_t*>(ped, 0x2E0);
    uint8_t* context = part + 0x300;
    if (At<uint8_t>(part, 0x33C) != 0) {
        ActionController_Stop(part + 0x258);
        ActionContext_SetNode(context, At<void*>(At<uint8_t*>(ped, 0x2E0), 0x4B0));
        context[0x3C] = 0;
    }
}

// (ped (0 the player), weapon (-1 none), ammo, [equip = 1]): given with the ammo (its
// ammo weapon, +0x84 of its info, given first when missing) and selected
int __cdecl Native_PedSetWeapon(lua_State* L) {
    uint8_t* ped = PedOrPlayer(Script::GetInt(L, 0));
    if (ped == nullptr) {
        return 0;
    }
    int weapon = Script::GetInt(L, 1);
    uint8_t equip = 1;
    if (lua::gettop(L) >= 4) {
        equip = Script::GetBool(L, 3);
    }
    if (weapon == -1) {
        At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18) = weapon;
        if (At<int32_t>(ped, 0x1310) == 0xD) {
            WeaponHud_Select(p3_weaponHud, weapon);
        }
        return 0;
    }
    if (P3_GetWeaponInfo(weapon) == nullptr) {
        return 0;
    }
    uint8_t* info = P3_GetWeaponInfo(weapon);
    P3_Butes_Resolve(info);
    int ammoWeapon = At<int32_t>(info, 0x84);
    uint16_t ammo = uint16_t(Script::GetInt(L, 2));
    void* inventory = At<void*>(ped, 0x1C4);
    int target;
    if (ammoWeapon == -1) {
        target = weapon;
    } else {
        target = ammoWeapon;
        if (ammoWeapon != weapon && Inventory_GetAmmo(inventory, weapon) == 0) {
            Inventory_Give(At<void*>(ped, 0x1C4), weapon, 1, equip, 0);
        }
    }
    int had = Inventory_GetAmmo(At<void*>(ped, 0x1C4), target);
    Inventory_Remove(At<void*>(ped, 0x1C4), target, had);
    uint8_t flag = equip != 0 && target == weapon;
    Inventory_Give(At<void*>(ped, 0x1C4), target, ammo, flag, 0);
    At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18) = weapon;
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        WeaponHud_Select(p3_weaponHud, weapon);
    }
    return 0;
}

// As PedSetWeapon, the selection played at once (+0x1388 set; "/Global/Weapons/
// SelectActions" through the ped's +0x40 virtual's result)
int __cdecl Native_PedSetWeaponNow(lua_State* L) {
    uint8_t* ped = PedOrPlayer(Script::GetInt(L, 0));
    if (ped == nullptr) {
        return 0;
    }
    At<int32_t>(ped, 0x1388) = 1;
    int result = P3_PedSetWeapon(L);
    void* tree = P3_FindActionNode("/Global/Weapons/SelectActions", 0);
    void* node = VCall<void*>(tree, 0x4C, VCall<void*>(ped, 0x40));
    if (node == nullptr) {
        node = ActionTree_Root(tree);
    }
    ActionController_Play(At<uint8_t*>(ped, 0x2E0) + 0x258, node);
    ActionController_Update(At<uint8_t*>(ped, 0x2E0) + 0x258, 9.9999997e-05f);
    return result;
}

// (ped (0 the player), weapon): its ammo taken (1000); unequipped if held
int __cdecl Native_PedDestroyWeapon(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = M_GetPedFromHandle(handle, handle != 0 ? 2 : 3);
    if (ped == nullptr) {
        return 0;
    }
    int weapon = Script::GetInt(L, 1);
    Inventory_Remove(At<void*>(ped, 0x1C4), weapon, 1000);
    uint8_t* held = At<uint8_t*>(ped, 0x1D0);
    if (At<int16_t>(held, 0x10E) == weapon) {
        Weapon_4D9410(held);
        ResetWeaponAction(ped);
        At<int32_t>(ped, 0x1574) = -1;
        At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18) = -1;
    }
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        uint8_t* hud = p3_weaponHud;
        if (At<int32_t>(hud, 0xC0) == weapon) {
            WeaponHud_Select(hud, -1);
            WeaponHud_Unequip(hud);
        }
    }
    return 0;
}

// (ped (0 the player))
int __cdecl Native_PedClearAllWeapons(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = M_GetPedFromHandle(handle, handle != 0 ? 2 : 3);
    if (ped == nullptr) {
        return 0;
    }
    Weapon_4D9410(At<void*>(ped, 0x1D0));
    ResetWeaponAction(ped);
    Inventory_Clear(At<void*>(ped, 0x1C4));
    At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18) = -1;
    At<int32_t>(ped, 0x1574) = -1;
    return 0;
}

// (ped, trigger)
int __cdecl Native_PedIsInTrigger(lua_State* L) {
    uint8_t* ped = P3_Ped(L, 0);
    if (ped == nullptr) {
        Script::PushBool(L, 0);
        return 1;
    }
    int slot = Script::GetInt(L, 1);
    uint8_t in = 0;
    if (slot != -1) {
        if (P3_Trigger_ContainsPoint(PoolSlot(m_triggerPool, slot), EntityPos(ped)) != 0) {
            in = 1;
        }
    }
    Script::PushBool(L, in);
    return 1;
}

// (ped, model name). (Copied unbounded into 256 bytes)
int __cdecl Native_PedSwapModel(lua_State* L) {
    uint8_t* ped = P3_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    char name[256];
    std::strcpy(name, Script::GetString(L, 1));
    int model = -1;
    P3_GetModelInfo(name, &model);
    VCall<void>(ped, 0x20, model, 0);
    At<int32_t>(ped, 0x13E8) = 0;
    return 0;
}

// ([model = -1], [range = 50]) -> the first ambient ped of the model (walking the pool
// down) within the range of the player, idle, not in a vehicle or group; stopped and
// its objectives cleared; -1
int __cdecl Native_PedFindAmbientPedOfModelID(lua_State* L) {
    int model = Script::GetIntOpt(L, 0, -1);
    float range = Script::GetFloatOpt(L, 1, 50.0f);
    CPool<uint8_t>* pool = m_pedPool;
    int i = pool->size;
    uint8_t* ped = nullptr;
    bool found = false;
    while (i != 0) {
        i--;
        if ((pool->flags[i] & 0x80) != 0) {
            ped = nullptr;
            continue;
        }
        ped = pool->objects + pool->elemSize * i;
        if (ped == nullptr || At<void*>(ped, 0x1554) != nullptr || At<void*>(ped, 0x548) != nullptr) {
            continue;
        }
        uint8_t* ai = At<uint8_t*>(ped, 0x1D24);
        if (ai != nullptr) {
            int mode = At<int32_t>(ai, 0x70);
            if (mode > 2 && mode < 0x10) {
                continue;
            }
        }
        if (At<int16_t>(ped, 0x10E) != model || At<int32_t>(ped, 0x14EC) != 0) {
            continue;
        }
        if (Ally_GetTop(ped + 0x2F0) != nullptr) {
            continue;
        }
        float distance = Distance2D(EntityPos(m_playerPed), EntityPos(ped));
        if (!(range >= distance)) {
            continue;
        }
        uint8_t playing = ActionController_IsPlaying(At<uint8_t*>(ped, 0x2E0) + 0x258, reinterpret_cast<void*>(0x01B652F0),
                                                     1, 1);
        if ((At<int32_t>(ped, 0x142C) == 0) & (playing != 0)) {
            found = true;
            break;
        }
    }
    if (found && ped != nullptr) {
        ActionController_Stop(At<uint8_t*>(ped, 0x2E0) + 0x290);
        P3_Objectives_Clear(ped + 0x5F0);
        if (At<void*>(ped, 0x1D24) != nullptr) {
            Ped_ReleasePOI(ped, 1);
        }
        Script::PushInt(L, VCall<int>(ped, 0xAC));
        return 1;
    }
    Script::PushInt(L, -1);
    return 1;
}

// (ped, target (0 the player), kind | first, [first-now | second], [flag]): attack
// (+0x1404 the flag)
int __cdecl Native_PedAttack(lua_State* L) {
    uint8_t* ped = P3_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    uint8_t* target = PedOrPlayer(Script::GetInt(L, 1));
    if (target == nullptr) {
        return 0;
    }
    uint8_t first = 0;
    int kind;
    if (Script::IsNumber(L, 2) != 0) {
        kind = Script::GetInt(L, 2);
        first = Script::GetBoolOpt(L, 3, 0);
    } else {
        uint8_t a = Script::GetBoolOpt(L, 2, 1);
        uint8_t b = Script::GetBoolOpt(L, 3, 1);
        int32_t made;
        kind = *MakeAttackKind(&made, a, b);
    }
    if (Script::IsBoolean(L, 4) != 0) {
        At<uint32_t>(ped, 0x1404) = Script::GetBoolOpt(L, 4, 1);
    } else {
        At<uint32_t>(ped, 0x1404) = 0;
    }
    void* memory = P3_Malloc(0x1C);
    void* objective = memory != nullptr ? AttackPedObjective_Construct(memory, ped, target, kind) : nullptr;
    if (first != 0) {
        P3_Objectives_Prepend(ped + 0x5F0, objective);
        return 0;
    }
    P3_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, other, value) for the catch/pass/receive objectives
template <typename F>
int BallObjective(lua_State* L, F construct, bool valueIsFloat) {
    uint8_t* ped = P3_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    uint8_t* other = P3_Ped(L, 1);
    if (other == nullptr) {
        return 0;
    }
    int value = valueIsFloat ? int(Script::GetFloat(L, 2)) : Script::GetInt(L, 2);
    void* memory = P3_Malloc(0x18);
    void* objective = memory != nullptr ? construct(memory, ped, other, value) : nullptr;
    P3_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

int __cdecl Native_PedPlayCatch(lua_State* L) {
    return BallObjective(L, PlayCatchObjective_Construct, false);
}

int __cdecl Native_PedPassBall(lua_State* L) {
    return BallObjective(L, PassBallObjective_Construct, false);
}

// (the value a float, truncated)
int __cdecl Native_PedReceiveBall(lua_State* L) {
    return BallObjective(L, ReceiveBallObjective_Construct, true);
}

// (ped, node, file) -> set
int __cdecl Native_PedSetTaskNode(lua_State* L) {
    uint8_t* ped = P3_Ped(L, 0);
    if (ped == nullptr) {
        Script::PushBool(L, 0);
        return 1;
    }
    const char* node = Script::GetString(L, 1);
    const char* file = Script::GetString(L, 2);
    Script::PushBool(L, Ped_SetTaskNode(ped, node, file));
    return 1;
}

// (ped, node, file) -> set; the script told (0x5DC1A0)
int __cdecl Native_PedSetActionNode(lua_State* L) {
    uint8_t* ped = P3_Ped(L, 0);
    if (ped == nullptr) {
        Script::PushBool(L, 0);
        return 1;
    }
    const char* node = Script::GetString(L, 1);
    const char* file = Script::GetString(L, 2);
    Script::PushBool(L, Ped_SetActionNode(ped, node, file));
    P3_ActFiles_Load(m_lurStore, ped, node, file, nullptr);
    return 1;
}

// (a, b, peds... (0 the player)) -> started (false for -1 or a stale ped)
int __cdecl Native_PedStartConversation(lua_State* L) {
    const char* a = Script::GetString(L, 0);
    const char* b = Script::GetString(L, 1);
    Conversation_Reset(conversations);
    for (int i = 2; i < lua::gettop(L); i++) {
        int handle = Script::GetInt(L, i);
        if (handle == -1) {
            Script::PushBool(L, 0);
            return 1;
        }
        uint8_t* ped = m_playerPed;
        if (handle != 0) {
            int slot = P3_PedHandleToSlot(handle);
            if (slot > 0x17) {
                Script::PushBool(L, 0);
                return 1;
            }
            CPool<uint8_t>* pool = m_pedPool;
            if ((pool->flags[slot] & 0x80) == 0) {
                ped = pool->objects + pool->elemSize * slot;
                if (ped != nullptr && VCall<int>(ped, 0xAC) != handle) {
                    Script::PushBool(L, 0);
                    return 1;
                }
            } else {
                ped = nullptr;
            }
        }
        if (ped == nullptr) {
            Script::PushBool(L, 0);
            return 1;
        }
        Conversation_AddPed(conversations, ped);
    }
    Script::PushBool(L, Conversation_Start(conversations, b, a));
    return 1;
}

} // namespace

#define PEDSCRIPT3_NATIVE(name, address)                                               \
    static decltype(&Native_##name) Original_P3_##name;                                  \
    REVERSED("Native_" #name, address, RE_BODY(&Native_##name), nullptr, \
             reinterpret_cast<void**>(&Original_P3_##name), re::Mode::Original)

PEDSCRIPT3_NATIVE(PedSetWeapon, 0x005C94F0);
PEDSCRIPT3_NATIVE(PedSetWeaponNow, 0x005C9660);
PEDSCRIPT3_NATIVE(PedDestroyWeapon, 0x005C9790);
PEDSCRIPT3_NATIVE(PedClearAllWeapons, 0x005C9880);
PEDSCRIPT3_NATIVE(PedIsInTrigger, 0x005C9A90);
PEDSCRIPT3_NATIVE(PedSwapModel, 0x005C9BD0);
PEDSCRIPT3_NATIVE(PedFindAmbientPedOfModelID, 0x005C9C60);
PEDSCRIPT3_NATIVE(PedAttack, 0x005C9E00);
PEDSCRIPT3_NATIVE(PedPlayCatch, 0x005C9F40);
PEDSCRIPT3_NATIVE(PedPassBall, 0x005C9FE0);
PEDSCRIPT3_NATIVE(PedReceiveBall, 0x005CA080);
PEDSCRIPT3_NATIVE(PedSetTaskNode, 0x005CA1B0);
PEDSCRIPT3_NATIVE(PedSetActionNode, 0x005CA230);
PEDSCRIPT3_NATIVE(PedStartConversation, 0x005CA310);
