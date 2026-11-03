// 0x520040 to 0x520CD0: CPAnimModelInfo's pieces around its damage: the damage
// animations looked up by name ("<name>\IDLE", "\DAMAGE01".."\DAMAGE10",
// "DAMAGE_MAX"), the random debris table, the effects at the act data's bones, a
// bone's transform, and the prop butes' start-up.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_ARRAY(char, ms_damageAnimName, 0x00C221A8);      // the name being built
GAME_ARRAY(uint32_t, ms_propModelHashes, 0x00C8E8E8); // 5
GAME_ARRAY(float, ms_boneOffsets, 0x00C8E914);        // 2 CVectors
GAME_ARRAY(void*, ms_propModelTable, 0x00CF3144);     // 6, for 0x5204A0
GAME_VAR(void*, ms_pEffectManager, 0x00B6F1D4);
inline void* const ms_animManager = reinterpret_cast<void*>(0x020C4910);
inline void* const ms_random = reinterpret_cast<void*>(0x00CF356C);

GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
// the animations for 12 hashes into 12 dwords (the low bytes kept)
GAME_FN(void, __thiscall, AnimManager_FindAnims, 0x006C1EF0, void* manager, uintptr_t, const uint32_t* hashes,
        uint32_t* out, int count, void* group, int);
GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* generator);
GAME_FN(void*, __cdecl, GetModelInfo, 0x0051C190, const char* name, int* id);
GAME_FN(void, __cdecl, CreateDebris_443FE0, 0x00443FE0, float x, float y, float z, int modelIndex, int, uintptr_t, uintptr_t,
        int);
GAME_FN(void, __thiscall, EffectProxy_Construct, 0x00668E90, void* self);
GAME_FN(void, __thiscall, EffectProxy_Destruct, 0x00668EB0, void* self);
GAME_FN(void, __thiscall, EffectManager_Spawn, 0x00668290, void* manager, const void* id, void* proxy, int);
GAME_FN(void, __thiscall, EffectProxy_SetTransform, 0x00668EC0, void* self, const float* transform);
GAME_FN(void, __thiscall, EffectProxy_Attach, 0x00669040, void* self, void* target, int);
GAME_FN(void, __thiscall, EffectProxy_SetLife, 0x00668D00, void* self, float seconds);
GAME_FN(uint8_t, __cdecl, Entity_GetBoneTransform, 0x00467D80, void* entity, int bone, float* out, int);
GAME_FN(void*, __thiscall, Entity_GetRwObject_415460, 0x00415460, void* entity);
GAME_FN(float*, __cdecl, VectorAdd, 0x004129A0, float* out, const float* a, const float* b);
GAME_FN(void*, __thiscall, ElementTable_FindById, 0x00465580, void* table, int id, int* outIndex);
GAME_FN(void, __thiscall, Matrix3_CopyTo_413C50, 0x00413C50, const void* self, float* out);
GAME_FN(float*, __thiscall, NiTransform_Multiply, 0x00406830, const float* self, float* out, const float* other);
GAME_FN(void, __cdecl, AddLoadCallback, 0x0072B870, void* fn, void* object);
GAME_FN(const char*, __cdecl, PropModelName_488BD0, 0x00488BD0, int index);

using re::At;

// a weighted debris table: seven (model name, weight) pairs from +0x18
struct DebrisEntry {
    re::Ptr32<const char> model;
    float weight;
};

} // namespace

namespace PAnimModelEffects {

// 0x520040 (thiscall): the damage animations for `name` (the first 8 characters kept
// at +0x58): their low bytes into +0x4C.. (12)
static void __fastcall SetDamageAnims(uint8_t* self, re::Edx, const char* name) {
    std::strncpy(reinterpret_cast<char*>(self + 0x58), name, 8);
    self[0x60] = 0;
    static const char* const kSuffixes[12] = {"\\IDLE",     "\\DAMAGE01", "\\DAMAGE02", "\\DAMAGE03",
                                              "\\DAMAGE04", "\\DAMAGE05", "\\DAMAGE06", "\\DAMAGE07",
                                              "\\DAMAGE08", "\\DAMAGE09", "\\DAMAGE10", "DAMAGE_MAX"};
    uint32_t hashes[12];
    for (int i = 0; i < 12; i++) {
        std::strcpy(ms_damageAnimName, name);
        std::strcat(ms_damageAnimName, kSuffixes[i]);
        hashes[i] = HashName(ms_damageAnimName);
    }
    uint32_t anims[12];
    AnimManager_FindAnims(ms_animManager, 0, hashes, anims, 12, At<void*>(At<uint8_t*>(self, 0x48), 0x10), 0);
    for (int i = 0; i < 12; i++) {
        self[0x4C + i] = uint8_t(anims[i]);
    }
}

// 0x5201D0 (thiscall, 4 bytes): the act data (+0x70)
[[maybe_unused]] static void* __fastcall GetActData(uint8_t* self, re::Edx) {
    return At<void*>(self, 0x70);
}
RE_UNHOOKED_FN("CPAnimModelInfo::GetActData", 0x005201D0, &GetActData);

// 0x5201E0 (cdecl): a debris model picked at random by weight and created at `pos`
// (none when the weights run out)
static void __cdecl SpawnRandomDebris(uint8_t* table, const float* pos) {
    float r = float(double(Random_GetFloat(ms_random)) * (1.0 - 0.0) + 0.0);
    const DebrisEntry* entries = reinterpret_cast<const DebrisEntry*>(table + 0x18);
    float sum = float(0.0 + entries[0].weight);
    const char* model = nullptr;
    int i = 0;
    for (;;) {
        if (!(sum < r)) {
            model = entries[i].model;
            break;
        }
        if (i == 6) {
            return;
        }
        i++;
        sum = sum + entries[i].weight;
    }
    if (model) {
        int index = -1;
        GetModelInfo(model, &index);
        CreateDebris_443FE0(pos[0], pos[1], pos[2], index, 1, 0, 0, 0);
    }
}

// 0x520380 (cdecl): a prop model's name hash (of 5)
static uint32_t __cdecl GetPropModelHash(int index) {
    return ms_propModelHashes[index];
}

// 0x5204A0 (thiscall): +0x80 from the table for 1..6, else 0
static void __fastcall SetModelEntry80(uint8_t* self, re::Edx, int which) {
    if (which >= 1 && which <= 6) {
        At<void*>(self, 0x80) = ms_propModelTable[which - 1];
    } else {
        At<void*>(self, 0x80) = nullptr;
    }
}

// 0x5207C0 (thiscall): the damage effects of `kind`: the effect slots of that kind
// (+0x62/+0x64..) spawned at the act data's bones (+0x34 / +0x40 / +0x4C), attached to
// the entity (kind 2: to `target`) when the bone isn't found; each proxy given 5 seconds
static void __fastcall SpawnDamageEffects(uint8_t* self, re::Edx, void* entity, int kind, void* target) {
    uint8_t proxies[3][8];
    for (int i = 2; i >= 0; i--) {
        EffectProxy_Construct(proxies[2 - i]);
    }
    float transform[13];
    transform[12] = 1.0f;
    for (int i = 0; i < 3; i++) {
        if (self[0x64 + i * 4] == uint8_t(kind)) {
            EffectManager_Spawn(ms_pEffectManager, self + 0x62 + i * 4, proxies[i], 0);
            uint8_t* act = At<uint8_t*>(self, 0x70);
            int bone = i == 0 ? At<int>(act, 0x34) : i == 1 ? At<int>(act, 0x40) : At<int>(act, 0x4C);
            if (Entity_GetBoneTransform(entity, bone, transform, 1)) {
                EffectProxy_SetTransform(proxies[i], transform);
            } else {
                void* to = uint8_t(kind) == 2 ? target : Entity_GetRwObject_415460(entity);
                EffectProxy_Attach(proxies[i], to, 0);
            }
        }
        EffectProxy_SetLife(proxies[i], 5.0f);
    }
    for (int i = 2; i >= 0; i--) {
        EffectProxy_Destruct(proxies[i]);
    }
}

// 0x5208C0 (thiscall): debris from the table at +0x78: at the act data's bone (+0x58),
// else 0.2 above the entity
static void __fastcall SpawnDebris(uint8_t* self, re::Edx, uint8_t* entity) {
    float transform[13];
    transform[12] = 1.0f;
    if (!At<void*>(self, 0x78)) {
        return;
    }
    float offset[3] = {0.0f, 0.0f, 0.2f};
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    const float* position = matrix ? reinterpret_cast<const float*>(matrix + 0x30)
                                   : reinterpret_cast<const float*>(entity + 4);
    float pos[3];
    VectorAdd(pos, position, offset);
    if (Entity_GetBoneTransform(entity, At<int>(At<uint8_t*>(self, 0x70), 0x58), transform, 1)) {
        pos[0] = transform[9];
        pos[1] = transform[10];
        pos[2] = transform[11];
    }
    SpawnRandomDebris(At<uint8_t*>(self, 0x78), pos);
}

// 0x520BC0 (thiscall): a bone's transform: `inout` times the element's own (its
// rotation, its position plus the offset for its kind; kinds above 1: false)
static uint8_t __fastcall GetBoneTransform(uint8_t* self, re::Edx, int bone, float* inout) {
    uint8_t* element = static_cast<uint8_t*>(ElementTable_FindById(At<void*>(self, 0x48), bone, nullptr));
    if (!element) {
        return 0;
    }
    float local[13];
    local[12] = 1.0f;
    Matrix3_CopyTo_413C50(vc::Call<void*>(element, 0x04), local);   // (vf1: its rotation)
    local[12] = 1.0f;
    uint32_t kind = At<uint32_t>(element, 8);
    if (kind > 1) {
        return 0;
    }
    const float* position = vc::Call<const float*>(element, 0x00);   // (vf0: its position)
    float sum[3];
    VectorAdd(sum, position, &ms_boneOffsets[kind * 3]);
    local[9] = sum[0];
    local[10] = sum[1];
    local[11] = sum[2];
    float copy[13];
    std::memcpy(copy, inout, sizeof(copy));
    float result[13];
    std::memcpy(inout, NiTransform_Multiply(copy, result, local), sizeof(result));
    return 1;
}

// 0x520CD0 (cdecl): CPAnimModelInfo::InitPropButes: the load callback (0x520CA0), the
// bone offsets ((0, 0, 0) and (0, 0, 0.2009)) and the prop models' name hashes
static void __cdecl InitPropButes() {
    AddLoadCallback(reinterpret_cast<void*>(0x00520CA0), nullptr);
    for (int i = 0; i < 5; i++) {
        ms_boneOffsets[i] = 0.0f;
    }
    *reinterpret_cast<uint32_t*>(&ms_boneOffsets[5]) = 0x3E4DAE82;   // 0.200861
    for (int i = 0; i < 5; i++) {
        ms_propModelHashes[i] = HashName(PropModelName_488BD0(i));
    }
}

} // namespace PAnimModelEffects

// ---- registration ----------------------------------------------------------------------

#define C16_PANIMFX_REVERSED(name, fn, address)                                                      \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace PAnimModelEffects {

C16_PANIMFX_REVERSED("CPAnimModelInfo::SetDamageAnims", SetDamageAnims, 0x00520040);
C16_PANIMFX_REVERSED("CPAnimModelInfo::SpawnRandomDebris", SpawnRandomDebris, 0x005201E0);
C16_PANIMFX_REVERSED("CPAnimModelInfo::GetPropModelHash", GetPropModelHash, 0x00520380);
C16_PANIMFX_REVERSED("CPAnimModelInfo::SetModelEntry80", SetModelEntry80, 0x005204A0);
C16_PANIMFX_REVERSED("CPAnimModelInfo::SpawnDamageEffects", SpawnDamageEffects, 0x005207C0);
C16_PANIMFX_REVERSED("CPAnimModelInfo::SpawnDebris", SpawnDebris, 0x005208C0);
C16_PANIMFX_REVERSED("CPAnimModelInfo::GetBoneTransform", GetBoneTransform, 0x00520BC0);
C16_PANIMFX_REVERSED("CPAnimModelInfo::InitPropButes", InitPropButes, 0x00520CD0);

} // namespace PAnimModelEffects
