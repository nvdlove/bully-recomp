// CPropAnim's per-frame work: its looping sound and sound bank by distance from the
// player, drawing (vf18), the glows it casts on the ground (lamps and signs), damage, and
// the weapon models it wants near the player. Offsets on the prop: +0x178 its sound bank
// loaded, +0x240 its playing sound, +0x244 the damage stage, +0x246 the damage taken,
// +0x274 the last hit's time, +0x3B4 the glow's fade, +0x3B7 lit indoors (0xFF: not known).

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

GAME_VAR(uint8_t*, pu_playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, pu_currArea, 0x00BD1008);
GAME_VAR(uint8_t, pu_otherAreaOn, 0x00BD09EC);
GAME_VAR(int8_t, pu_otherArea, 0x00BD09EB);
GAME_VAR(int32_t, pu_widescreen, 0x00C3CD0C);
GAME_VAR(float, pu_timeStep, 0x00C1A9A4);
GAME_VAR(float, pu_spinSpeed, 0x00A453E8);   // 0.02
GAME_VAR(uint32_t, pu_spinAnimHash, 0x01B65518);
GAME_VAR(uint32_t, pu_timeInMs, 0x00A14A50);
GAME_ARRAY(uint8_t*, pu_modelInfoPtrs, 0x00C67738);
GAME_ARRAY(uint8_t, pu_specialDrawn, 0x00C3B7FC);
GAME_VAR(int32_t, pu_specialModel0, 0x00BF4488);
GAME_VAR(int32_t, pu_specialModel1, 0x00BF447C);
GAME_VAR(int32_t, pu_specialModel2, 0x00BF4484);
GAME_VAR(int32_t, pu_lateModel, 0x00BF4480);
GAME_VAR(int32_t, pu_unlitModel, 0x00BF4180);
GAME_VAR(int32_t, pu_breakModel, 0x00BF410C);
GAME_VAR(int32_t, pu_statModel0, 0x00BF3E70);
GAME_VAR(int32_t, pu_statModel1, 0x00BF3E6C);
GAME_VAR(int32_t, pu_statModel2, 0x00BF3E68);
GAME_VAR(int32_t, pu_statModel3, 0x00BF3E64);
// the glow colours and textures
GAME_VAR(int32_t, pu_glowColour, 0x00913DD4);
GAME_VAR(int32_t, pu_glowColourIndoors, 0x00913DD0);
GAME_VAR(int32_t, pu_signColourOn, 0x00913DDC);
GAME_VAR(int32_t, pu_signColourOff, 0x00913DE0);
GAME_VAR(int32_t, pu_glowTexture, 0x00CA05FC);
GAME_VAR(int32_t, pu_signTextureOn, 0x00CA05F4);
GAME_VAR(int32_t, pu_signTextureOff, 0x00CA05F8);
GAME_VAR(void*, pu_stimuli, 0x00C2C1A0);
// (the ambience emitters, sound banks and sounds; the area table's radii, 0xB8 apart)
inline void* const pu_ambience = reinterpret_cast<void*>(0x00AE5F60);
inline void* const pu_soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const pu_sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const pu_memoryTag = reinterpret_cast<void*>(0x00D140B8);
inline void* const pu_renderer = reinterpret_cast<void*>(0x020C4910);
inline void* const pu_routeAreas = reinterpret_cast<void*>(0x00BCC580);
inline uint8_t* const pu_emitterTypes = reinterpret_cast<uint8_t*>(0x00AE6CDE);

GAME_FN(uint8_t*, __cdecl, GetObjectInfo, 0x004D0CA0, int model);
GAME_FN(CVec*, __cdecl, VectorSub, 0x00414B70, CVec* out, const CVec* a, const CVec* b);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVec* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVec* v);
GAME_FN(void, __thiscall, Ambience_PlayEmitter, 0x005870D0, void* ambience, int sound, float distSq, const CVec* pos,
        float volume, int type, int, int, int, int);
GAME_FN(uint8_t*, __thiscall, ModelInfo_GetAnimInfo, 0x005201D0, uint8_t* modelInfo);
GAME_FN(uint32_t*, __thiscall, Hash_Construct, 0x005FA870, uint32_t* hash, uint32_t value);
GAME_FN(void, __thiscall, Hash_Destruct, 0x0049A650, uint32_t* hash);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, re::Ptr32<void>* sound, uintptr_t id);
GAME_FN(void, __thiscall, Sound_Construct, 0x0042D6D0, re::Ptr32<void>* sound, void* handle);
GAME_FN(void, __thiscall, SoundBanks_Load, 0x0059A3E0, void* banks, int bank, int, int);
GAME_FN(void, __thiscall, SoundBanks_Unload, 0x0059A420, void* banks, int bank, int, int);
GAME_FN(uint8_t, __thiscall, SoundBanks_IsLoaded, 0x005995F0, void* banks, int bank);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVec* pos, int type, uintptr_t, int, int,
        int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, Sounds_PlayEntity, 0x0059C110, void* sounds, int id, void* entity, uintptr_t, int type);
GAME_FN(int, __cdecl, ModelNode, 0x00520380, int index);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, void* entity, int node, float* transform, int flag);
GAME_FN(void, __cdecl, RemoveLateProp, 0x004E8DE0, void* prop);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsHidden, 0x00520430, uint8_t* modelInfo);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsKind1, 0x00520440, uint8_t* modelInfo);
GAME_FN(void, __thiscall, MemoryTag_Set, 0x005EE570, void* self, int tag);
GAME_FN(void, __thiscall, Entity_SetAlpha, 0x00466160, void* entity, int alpha);
GAME_FN(void, __cdecl, RwObject_Update, 0x00408380, void* rwObject);
GAME_FN(void, __cdecl, RwObject_Render, 0x006C7420, void* rwObject);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int32_t* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(uint8_t, __cdecl, IsCurrentAreaInterior, 0x0040DF80);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(uint8_t, __thiscall, GetTriggerRouteArea, 0x004153B0, void* self, void* prop);
GAME_FN(void, __cdecl, GroundTransform, 0x00429520, float* out, float x, float y, float z, uint32_t w, float dirX,
        float dirY, float dirZ, uint32_t dirW, float zOffset);
GAME_FN(void, __stdcall, AddLitProp, 0x004EA770, const float* m, int colour, float size, int texture);
GAME_FN(int, __thiscall, PAnimModelInfo_GetDamageThreshold, 0x00520390, uint8_t* modelInfo, int stage);
GAME_FN(void, __thiscall, PAnimModelInfo_OnDamage, 0x005207C0, uint8_t* modelInfo, void* prop, int event, uintptr_t arg);
GAME_FN(int, __thiscall, PAnimModelInfo_GetStageAnim, 0x00520420, uint8_t* modelInfo, int stage);
GAME_FN(void, __thiscall, PAnimModelInfo_OnBroken, 0x005208C0, uint8_t* modelInfo, void* prop);
GAME_FN(void*, __thiscall, AnimObject_GetAnim, 0x006B1F50, void* animObject, int index);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C0D40, void* manager, void* animObject, uintptr_t index, void* anim, int,
        float, float);
GAME_FN(void, __cdecl, PropAnim_OnBroken, 0x005B2AD0, void* prop);
GAME_FN(uint8_t, __cdecl, IsModelInList25, 0x0043F420, int model);
GAME_FN(uint8_t, __cdecl, IsModelInList30, 0x0043F520, int model);
GAME_FN(void, __cdecl, IncrementStat, 0x00458480, int stat, int amount);
GAME_FN(void, __thiscall, Stimuli_Add, 0x004A6670, void* stimuli, int type, void* from, void* to, uintptr_t, float, float);
GAME_FN(uint8_t, __cdecl, RequestWeaponModels, 0x004D9790, int weapon, int flags);
GAME_FN(void, __cdecl, AddWeaponModelRefs, 0x004DA680, int weapon, int remove);

const CVec& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVec*>(matrix + 0x30) : At<CVec>(entity, 4);
}

float DistSq(const CVec& d) {
    return float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
}

// the low 4 bits of a byte, signed
int LowNibble(uint8_t b) {
    return int(int8_t(uint8_t(b << 4))) >> 4;
}

// 0x4EB200 (thiscall): the prop's ambient emitter, spin and sound: its bank loaded within
// range of the player (dropped past twice that and a margin), its loop started when in
// the sound type's radius and stopped outside it
void __fastcall UpdateSound(uint8_t* self) {
    CVec d;
    VectorSub(&d, &Position(pu_playerPed), &Position(self));
    float distSq = DistSq(d);
    uint8_t* info = GetObjectInfo(At<int16_t>(self, 0x10E));
    if ((info[0x21] & 1) && !VCall<uint8_t>(self, 0x78)) {
        float volume = float(double(info[0x21] >> 1) / 100.0);
        Ambience_PlayEmitter(pu_ambience, At<int32_t>(info, 0x14), distSq, &Position(self), volume, LowNibble(info[0x20]),
                             0, 200, 2000, 1);
    }
    uint32_t hash;
    Hash_Construct(&hash, At<uint32_t>(ModelInfo_GetAnimInfo(pu_modelInfoPtrs[At<int16_t>(self, 0x10E)]), 0x80));
    bool spins = pu_spinAnimHash == hash;
    Hash_Destruct(&hash);
    if (spins) {
        float step = pu_spinSpeed * pu_timeStep;
        At<float>(self, 0x3A4) = step + At<float>(self, 0x3A4);
    }
    if (At<int16_t>(info, 0xE) == -1) {
        return;
    }
    float range = float(At<int32_t>(info, 0x10));
    float rangeOut = range <= 225.0f ? range + 25.0f : range + 100.0f;
    re::Ptr32<void>* sound = &At<void*>(self, 0x240);
    if (At<int32_t>(self, 0x178) != 0) {
        if (double(range) + rangeOut < distSq) {
            if (*sound != nullptr) {
                VCall<void>(*sound, 0x34);
                Sound_Set(sound, 0);
            }
            SoundBanks_Unload(pu_soundBanks, At<int16_t>(info, 0xE), 1, 1);
            At<int32_t>(self, 0x178) = 0;
        }
    } else if (distSq < range && !VCall<uint8_t>(self, 0x78)) {
        SoundBanks_Load(pu_soundBanks, At<int16_t>(info, 0xE), 1, 1);
        At<int32_t>(self, 0x178) = 1;
    }
    if (info[0x21] & 1) {
        return;
    }
    int type = LowNibble(info[0x20]);
    float radius = float(*reinterpret_cast<uint16_t*>(pu_emitterTypes + type * 0xB8));
    bool inRange = double(distSq) < double(radius) * radius;
    if (*sound == nullptr) {
        if (At<int32_t>(self, 0x178) != 0 && At<int32_t>(info, 0x14) != 0 &&
            SoundBanks_IsLoaded(pu_soundBanks, At<int16_t>(info, 0xE)) && inRange && !VCall<uint8_t>(self, 0x78)) {
            CVec pos = {0.0f, 0.0f, 0.0f};
            float transform[13];
            transform[12] = 1.0f;
            if (GetNodeTransform(self, ModelNode(0), transform, 1)) {
                pos.x = transform[9];
                pos.y = transform[10];
                pos.z = float(transform[11] + 0.20000000298023224);
            } else {
                pos = Position(self);
            }
            void* handle = Sounds_PlayAt(pu_sounds, At<int32_t>(info, 0x14), &pos, type, 0, 0, 0x78, 0, 0, 0, 0, 6, 1,
                                         1.0f, 0);
            Sound_Construct(sound, handle);
            return;
        }
        if (*sound == nullptr) {
            return;
        }
    }
    if (!inRange || VCall<uint8_t>(self, 0x78)) {
        VCall<void>(*sound, 0x34);
        Sound_Set(sound, 0);
    }
}

// 0x4EB5C0 (thiscall, vf18): draws the prop (in its area, or area 1, or the other area
// shown) with its alpha or its model's additive states, then its children; the special
// props mark themselves drawn
void __fastcall Render(uint8_t* self) {
    if (At<int32_t>(self, 0xD0) != 0) {
        return;
    }
    if (At<int16_t>(self, 0x10E) == pu_specialModel0) {
        pu_specialDrawn[0] = 1;
    }
    if (At<int16_t>(self, 0x10E) == pu_specialModel2) {
        pu_specialDrawn[2] = 1;
    }
    if (At<int16_t>(self, 0x10E) == pu_specialModel1) {
        pu_specialDrawn[1] = 1;
    }
    if (pu_currArea == 37 && At<int16_t>(self, 0x10E) == pu_lateModel) {
        RemoveLateProp(self);
    }
    uint8_t area = self[0x111];
    if (!(area == pu_currArea || area == 1 || (pu_otherAreaOn != 0 && area == pu_otherArea))) {
        return;
    }
    void* rwObject = At<void*>(self, 0x18);
    if (rwObject == nullptr) {
        return;
    }
    uint8_t* modelInfo = pu_modelInfoPtrs[At<int16_t>(self, 0x10E)];
    if (PAnimModelInfo_IsHidden(modelInfo)) {
        return;
    }
    At<int32_t>(self, 0x80) = 1;
    MemoryTag_Set(pu_memoryTag, 3);
    uint16_t alpha = At<uint16_t>(self, 0x250);
    if (alpha != 0xFF) {
        Entity_SetAlpha(self, int16_t(alpha));
        RwObject_Update(At<void*>(self, 0x18));
        RwObject_Render(At<void*>(self, 0x18));
        Entity_SetAlpha(self, 0xFF);
    } else {
        int32_t blendSrc = 0, blendDst = 0, alphaRef = 0, alphaFunc = 0;
        if (modelInfo[0x8F]) {
            GetRenderState(10, &blendSrc);
            GetRenderState(11, &blendDst);
            GetRenderState(0x1D, &alphaRef);
            GetRenderState(0x1E, &alphaFunc);
            SetRenderState(10, 3);
            SetRenderState(11, 1);
            SetRenderState(0x1D, 5);
            SetRenderState(0x1E, 200);
        }
        RwObject_Update(At<void*>(self, 0x18));
        RwObject_Render(At<void*>(self, 0x18));
        if (modelInfo[0x8F]) {
            SetRenderState(10, blendSrc);
            SetRenderState(11, blendDst);
            SetRenderState(0x1D, alphaRef);
            SetRenderState(0x1E, alphaFunc);
        }
    }
    At<int32_t>(self, 0x80) = 0;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x288); i++) {
        uint8_t* child = At<re::Ptr32<uint8_t>*>(self, 0x280)[i];
        if (child != nullptr) {
            VCall<void>(child, 0x48);
        }
    }
}

// 0x4EB7E0: the width of an entity's collision box
float __cdecl GetColWidth(uint8_t* entity) {
    uint8_t* col = At<uint8_t*>(pu_modelInfoPtrs[At<int16_t>(entity, 0x10E)], 0xC);
    float width = At<float>(col, 0x20) - At<float>(col, 0x10);
    return width;
}

// 0x4EB840 (thiscall): a lamp's glow on the ground (fading in and out with +0x19C), from
// its node 0, or else the nearer of nodes 3 and 4, within 20 of the player
void __fastcall UpdateLampGlow(uint8_t* self) {
    int16_t& fade = At<int16_t>(self, 0x3B4);
    float level;
    if (At<int32_t>(self, 0x19C) != 0) {
        fade = int16_t(fade - 0x41);
        level = float(fade);
        if (level < 0.0) {
            level = 0.0f;
        }
    } else {
        fade = int16_t(fade + 0x41);
        level = float(fade);
        if (!(level <= 255.0)) {
            level = 255.0f;
        }
    }
    fade = int16_t(int(level));
    if (At<int32_t>(self, 0x17C) != 0 || At<int32_t>(self, 0x184) != 0 || At<int16_t>(self, 0x10E) == pu_unlitModel ||
        At<int32_t>(self, 0x1AC) != 0 || pu_widescreen != 0) {
        return;
    }
    const CVec& pos = Position(self);
    const CVec& player = Position(pu_playerPed);
    CVec d = {pos.x - player.x, pos.y - player.y, pos.z - player.z};
    if (!(Vector_MagnitudeSqr(&d) < 400.0)) {
        return;
    }
    int colour = pu_glowColour;
    float transform[13];
    transform[12] = 1.0f;
    if (self[0x3B7] == 0xFF) {
        uint8_t interior = IsCurrentAreaInterior();
        uint8_t* trigger = At<uint8_t*>(self, 0x23C);
        if ((trigger[0x3F] & 8) && !interior) {
            uint8_t area = GetTriggerRouteArea(pu_routeAreas, trigger);
            if (area != 0xFF) {
                self[0x3B7] = IsAreaInterior(int8_t(area)) == 1;
            }
        } else {
            self[0x3B7] = 0;
        }
    }
    if (self[0x3B7] == 1) {
        colour = pu_glowColourIndoors;
    }
    // (the vectors' 4th floats are the game's uninitialised stack)
    if (GetNodeTransform(self, ModelNode(0), transform, 1)) {
        GroundTransform(transform, transform[9], transform[10], transform[11], 0, transform[1], transform[4], transform[7], 0,
                        0.0f);
        AddLitProp(transform, colour, 0.93f, pu_glowTexture);
        return;
    }
    float other[13];
    other[12] = 1.0f;
    if (!GetNodeTransform(self, ModelNode(3), transform, 1)) {
        return;
    }
    if (!GetNodeTransform(self, ModelNode(4), other, 1)) {
        return;
    }
    CVec a = {transform[9] - player.x, transform[10] - player.y, transform[11] - player.z};
    CVec b = {other[9] - player.x, other[10] - player.y, other[11] - player.z};
    float distA = Vector_Magnitude(&a);
    float distB = Vector_Magnitude(&b);
    // (the direction is node 3's either way)
    if (distA < distB) {
        GroundTransform(transform, transform[9], transform[10], transform[11], 0, transform[1], transform[4], transform[7],
                        0, 0.0f);
        AddLitProp(transform, colour, 0.93f, pu_glowTexture);
    } else {
        GroundTransform(other, other[9], other[10], other[11], 0, transform[1], transform[4], transform[7], 0, 0.0f);
        AddLitProp(other, colour, 0.93f, pu_glowTexture);
    }
}

// 0x4EBCB0 (thiscall): a sign's glow (on or off by +0x17C) within 20 of the player
void __fastcall UpdateSignGlow(uint8_t* self) {
    const CVec& pos = Position(self);
    const CVec& player = Position(pu_playerPed);
    CVec d = {pos.x - player.x, pos.y - player.y, pos.z - player.z};
    float distSq = DistSq(d);
    if (!(distSq < 400.0) || At<int32_t>(self, 0x234) != 0 || At<int32_t>(self, 0x1AC) != 0 || pu_widescreen != 0) {
        return;
    }
    float transform[13];
    transform[12] = 1.0f;
    if (!GetNodeTransform(self, ModelNode(0), transform, 1)) {
        return;
    }
    GroundTransform(transform, transform[9], transform[10], transform[11], 0, transform[1], transform[4], transform[7], 0,
                    0.002f);
    if (At<int32_t>(self, 0x17C) == 0) {
        AddLitProp(transform, pu_signColourOn, 0.33f, pu_signTextureOn);
    } else {
        AddLitProp(transform, pu_signColourOff, 0.33f, pu_signTextureOff);
    }
}

// 0x4ED0B0 (thiscall): damage to the prop: past its threshold (and at most every 100 ms)
// it goes a stage on, playing the stage's animation; after the last it's broken (the
// player's breaking some kinds counts in the stats); hits make sounds and stimuli
void __fastcall TakeDamage(uint8_t* self, re::Edx, int arg, int amount, uint8_t* attacker) {
    uint8_t* modelInfo = pu_modelInfoPtrs[At<int16_t>(self, 0x10E)];
    uint8_t* info = GetObjectInfo(At<int16_t>(self, 0x10E));
    if (VCall<uint8_t>(self, 0x78)) {
        return;
    }
    if (PAnimModelInfo_IsKind1(modelInfo)) {
        if (info != nullptr && At<int32_t>(info, 0x1C) != 0) {
            Sounds_PlayEntity(pu_sounds, At<int32_t>(info, 0x1C), self, arg, int8_t(info[0x20]) >> 4);
        }
        return;
    }
    if (At<int32_t>(self, 0x168) != 0) {
        return;
    }
    if (amount <= At<int32_t>(ModelInfo_GetAnimInfo(modelInfo), 0x18)) {
        return;
    }
    uint16_t minimum = At<uint16_t>(self, 0x122);
    if (minimum != 0 && amount <= int(minimum)) {
        return;
    }
    if (At<void*>(At<uint8_t*>(self, 0x114), 0x94) == nullptr) {
        return;
    }
    uint32_t now = pu_timeInMs;
    if (now - At<uint32_t>(self, 0x274) <= 100) {
        return;
    }
    At<int16_t>(self, 0x246) = int16_t(At<int16_t>(self, 0x246) + amount);
    At<uint32_t>(self, 0x274) = now;
    uint16_t limit = At<uint16_t>(self, 0x120);
    bool nextStage;
    if (limit != 0) {
        nextStage = At<int16_t>(self, 0x246) > int(limit);
    } else {
        nextStage = At<int16_t>(self, 0x246) > PAnimModelInfo_GetDamageThreshold(modelInfo, int8_t(self[0x244]));
    }
    if (!nextStage) {
        if (info != nullptr && At<int32_t>(info, 0x1C) != 0) {
            Sounds_PlayEntity(pu_sounds, At<int32_t>(info, 0x1C), self, arg, int8_t(info[0x20]) >> 4);
        }
        PAnimModelInfo_OnDamage(modelInfo, self, 2, arg);
        return;
    }
    At<int16_t>(self, 0x246) = 0;
    PAnimModelInfo_OnDamage(modelInfo, self, int8_t(self[0x244]) + 2, arg);
    int anim = PAnimModelInfo_GetStageAnim(modelInfo, int8_t(self[0x244]));
    if (anim != -1) {
        void* animData = AnimObject_GetAnim(At<void*>(self, 0x114), 0);
        AnimManager_Play(pu_renderer, At<void*>(self, 0x114), anim, animData, 1, 1.0f, -1.0f);
        self[0x244]++;
    }
    int sound;
    if (PAnimModelInfo_GetStageAnim(modelInfo, int8_t(self[0x244])) == -1) {
        At<int32_t>(self, 0x174) = 1;
        PropAnim_OnBroken(self);
        if (At<int16_t>(self, 0x10E) == pu_breakModel) {
            self[0x3B6] = 1;
        } else {
            PAnimModelInfo_OnBroken(modelInfo, self);
        }
        At<int32_t>(self, 0x238) = attacker != nullptr ? VCall<int32_t>(attacker, 0xAC) : 0;
        if (ModelInfo_GetAnimInfo(modelInfo)[0x24] != 0) {
            float speed = At<float>(ModelInfo_GetAnimInfo(modelInfo), 0x28);
            At<float>(self, 0x248) = speed;
            At<float>(self, 0x24C) = float(1.0 / speed);
        }
        if (ModelInfo_GetAnimInfo(modelInfo)[0x2C] == 0) {
            At<int32_t>(self, 0x20) = 0;
            At<int32_t>(self, 0x3C) = 0;
        }
        if (attacker == pu_playerPed) {
            int model = At<int16_t>(self, 0x10E);
            if (IsModelInList25(model)) {
                IncrementStat(0x15, 1);
            } else if (IsModelInList30(model)) {
                IncrementStat(0x14, 1);
            } else if (model == pu_breakModel) {
                IncrementStat(0x1A, 1);
            } else {
                int m = At<int16_t>(self, 0x10E);
                if (m == pu_statModel0 || m == pu_statModel1 || m == pu_statModel2 || m == pu_statModel3) {
                    IncrementStat(0x1B, 1);
                }
            }
        }
        if (info == nullptr) {
            goto stimulus;
        }
        sound = At<int32_t>(info, 0x18);
    } else {
        if (info == nullptr) {
            goto stimulus;
        }
        sound = At<int32_t>(info, 0x1C);
    }
    if (sound != 0) {
        Sounds_PlayEntity(pu_sounds, sound, self, arg, int8_t(info[0x20]) >> 4);
    }
stimulus:
    if (attacker != nullptr) {
        Stimuli_Add(pu_stimuli, 2, attacker, self, 0, -1.0f, -1.0f);
    }
    PAnimModelInfo_OnDamage(modelInfo, self, 2, arg);
}

// 0x4ED410 (thiscall; its argument unused): the prop's weapon models requested once,
// within 15 of the player
void __fastcall RequestWeapon(uint8_t* self, re::Edx, int) {
    if (At<int32_t>(self, 0x1B0) != 0) {
        return;
    }
    int weapon = At<int32_t>(ModelInfo_GetAnimInfo(pu_modelInfoPtrs[At<int16_t>(self, 0x10E)]), 0x88);
    if (weapon == -1) {
        return;
    }
    CVec d;
    VectorSub(&d, &Position(self), &Position(pu_playerPed));
    float distSq = DistSq(d);
    if (distSq <= 225.0 && RequestWeaponModels(weapon, 0) == 1) {
        AddWeaponModelRefs(weapon, 0);
        At<int32_t>(self, 0x1B0) = 1;
    }
}

#define C13C_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C13C_REVERSED("CPropAnim::UpdateSound", 0x004EB200, &UpdateSound);
C13C_REVERSED("CPropAnim::Render", 0x004EB5C0, &Render);
C13C_REVERSED("CEntity::GetColWidth", 0x004EB7E0, &GetColWidth);
C13C_REVERSED("CPropAnim::UpdateLampGlow", 0x004EB840, &UpdateLampGlow);
C13C_REVERSED("CPropAnim::UpdateSignGlow", 0x004EBCB0, &UpdateSignGlow);
C13C_REVERSED("CPropAnim::TakeDamage", 0x004ED0B0, &TakeDamage);
C13C_REVERSED("CPropAnim::RequestWeapon", 0x004ED410, &RequestWeapon);

} // namespace
