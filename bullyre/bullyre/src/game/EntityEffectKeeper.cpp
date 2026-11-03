// CEntityEffectKeeper (0x74, in its pool at 0xC0F628): an entity's seven effect proxies,
// each attached to a bone of its animated object.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "reversed.h"

namespace {

using col::CVector;

// the audio/effects library's proxy (16 bytes)
struct EffectProxy {
    uint32_t unknown00;  // +0
    re::Ptr32<void> active;        // +4 (nullptr: free)
    re::Ptr32<uint8_t> attachment; // +8 (+0xC: the bone element)
    uint32_t unknown0C;  // +0xC
};
static_assert(sizeof(EffectProxy) == 0x10);

struct CEntityEffectKeeper {
    re::Ptr32<CEntity> owner;           // +0
    EffectProxy proxies[7];   // +4

    CEntityEffectKeeper* Construct();
    void Destruct();
    void StopAll();
    void Start(uint16_t type, int bone);
    void Stop(int type, uint32_t bone);
    EffectProxy* Find(int type, void* element);
    int FindNearestBone(const CVector* pos);
    void StartAtNearestBone(uint16_t type, const CVector* pos);
};
static_assert(sizeof(CEntityEffectKeeper) == 0x74);

GAME_VAR(void*, s_effectLibrary, 0x00B6F1E8);
GAME_VAR(void*, s_effectManager, 0x00B6F1D4);
inline void* const s_animManager = reinterpret_cast<void*>(0x020C4910);

GAME_FN(void, __thiscall, EffectProxy_Construct, 0x00668DA0, EffectProxy* proxy);
GAME_FN(void, __thiscall, EffectProxy_Destruct, 0x00668DC0, EffectProxy* proxy);
GAME_FN(void, __thiscall, Effect_Stop, 0x00668E40, EffectProxy* proxy);
GAME_FN(void, __thiscall, EffectProxy_Kill, 0x00668BA0, EffectProxy* proxy);
GAME_FN(void, __thiscall, EffectManager_Start, 0x00668290, void* manager, const uint32_t* type, EffectProxy* proxy, int arg);
GAME_FN(void*, __thiscall, Effect_GetNode, 0x00668E00, EffectProxy* proxy);
GAME_FN(int, __thiscall, AnimManager_Attach, 0x006C2070, void* manager, void* animObject, int bone, void* frame, int* handle, uintptr_t a,
        uintptr_t b, void* node);
GAME_FN(void, __thiscall, Effect_SetAttachment, 0x00668DE0, EffectProxy* proxy, void* animObject, uintptr_t handle);
GAME_FN(void*, __thiscall, EffectInfos_Get, 0x006679F0, void* library, int id);
GAME_FN(uint8_t*, __thiscall, EffectProxy_GetEffect, 0x0066B5D0, EffectProxy* proxy);
GAME_FN(void, __thiscall, AnimManager_GetBonePosition, 0x006C1190, void* manager, void* animObject, int bone, CVector* out, uintptr_t a);
GAME_FN(int, __thiscall, Keeper_FindNearestBone, 0x004689B0, CEntityEffectKeeper* keeper, const CVector* pos);
GAME_FN(void, __thiscall, Keeper_Start, 0x00468770, CEntityEffectKeeper* keeper, uint16_t type, int bone);

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

// (464EA0 inlined) the owner's skeleton's bone element i
uint8_t* BoneElement(CEntity* owner, uint32_t index) {
    uint8_t* skeleton = Field<uint8_t*>(owner->animObject, 0x94);
    uint32_t own = Field<uint32_t>(skeleton, 0x9C);
    if (index < own) {
        return Field<uint8_t*>(skeleton, 0xA0) + index * 0x28;
    }
    uint8_t* next = Field<uint8_t*>(skeleton, 0x10);
    if (next) {
        index -= own;
        if (index < Field<uint32_t>(next, 0x5C)) {
            uint8_t* element = Field<uint8_t*>(next, 0x60) + index * 0xC;
            if (element && element + 4) {
                return element;
            }
        }
    }
    return nullptr;
}

// 0x468740 (thiscall)
void CEntityEffectKeeper::StopAll() {
    for (EffectProxy& proxy : proxies) {
        Effect_Stop(&proxy);
        EffectProxy_Kill(&proxy);
    }
}

// 0x468770 (thiscall): an effect in the first free proxy, on the bone
void CEntityEffectKeeper::Start(uint16_t type, int bone) {
    int i = 0;
    while (proxies[i].active != nullptr) {
        if (++i >= 7) {
            return;
        }
    }
    EffectProxy* proxy = &proxies[i];
    uint32_t effectType = type;
    EffectManager_Start(s_effectManager, &effectType, proxy, 0);
    if (!proxy->active) {
        return;
    }
    void* node = Effect_GetNode(proxy);
    int handle = 0;
    if (AnimManager_Attach(s_animManager, owner->animObject, bone, nullptr, &handle, 0, 0, node)) {
        Effect_Stop(proxy);
        EffectProxy_Kill(proxy);
    }
    Effect_SetAttachment(proxy, owner->animObject, handle);
}

// 0x468820 (thiscall): the effects of the type on the bone killed
void CEntityEffectKeeper::Stop(int type, uint32_t bone) {
    uint8_t* element = BoneElement(owner, bone);
    void* info = EffectInfos_Get(s_effectLibrary, type);
    for (EffectProxy& proxy : proxies) {
        if (!proxy.active) {
            continue;
        }
        uint8_t* effect = EffectProxy_GetEffect(&proxy);
        uint8_t* attachment = proxy.attachment;
        if (effect && Field<uint8_t*>(effect, 4) && Field<void*>(Field<uint8_t*>(effect, 4), 0x50) == info && attachment &&
            Field<uint8_t*>(attachment, 0xC) == element) {
            EffectProxy_Kill(&proxy);
            Effect_Stop(&proxy);
        }
    }
}

// 0x4688E0 (thiscall): the proxy with an effect of the type on the element
EffectProxy* CEntityEffectKeeper::Find(int type, void* element) {
    void* info = EffectInfos_Get(s_effectLibrary, type);
    for (EffectProxy& proxy : proxies) {
        uint8_t* effect = EffectProxy_GetEffect(&proxy);
        if (effect && Field<uint8_t*>(effect, 4) && Field<void*>(Field<uint8_t*>(effect, 4), 0x50) == info && proxy.attachment &&
            Field<void*>(proxy.attachment, 0xC) == element) {
            return &proxy;
        }
    }
    return nullptr;
}

// 0x468950 (thiscall)
CEntityEffectKeeper* CEntityEffectKeeper::Construct() {
    for (EffectProxy& proxy : proxies) {
        EffectProxy_Construct(&proxy);
    }
    owner = nullptr;
    return this;
}

// 0x468980 (thiscall): the proxies, last first
void CEntityEffectKeeper::Destruct() {
    for (int i = 6; i >= 0; i--) {
        EffectProxy_Destruct(&proxies[i]);
    }
}

// 0x4689B0 (thiscall): the owner's bone nearest to pos (-1: none nearer than 1e7)
int CEntityEffectKeeper::FindNearestBone(const CVector* pos) {
    float nearest = 1.0e7f;
    int found = -1;
    int n = Field<int32_t>(Field<uint8_t*>(owner->animObject, 0x94), 0x9C);
    for (int i = 0; i < n; i++) {
        CVector bonePos{0.0f, 0.0f, 0.0f};
        AnimManager_GetBonePosition(s_animManager, owner->animObject, i, &bonePos, 0);
        CVector d{bonePos.x - pos->x, bonePos.y - pos->y, bonePos.z - pos->z};
        float sq = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
        float dist = float(std::sqrt(double(sq)));
        if (nearest > dist) {
            nearest = dist;
            found = i;
        }
    }
    return found;
}

// 0x468AB0 (thiscall): Start on the bone nearest to pos
void CEntityEffectKeeper::StartAtNearestBone(uint16_t type, const CVector* pos) {
    int bone = Keeper_FindNearestBone(this, pos);
    if (bone >= 0) {
        Keeper_Start(this, type, bone);
    }
}

} // namespace

#define KEEPER_C05_REVERSED(name, address, fn) REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)

KEEPER_C05_REVERSED("CEntityEffectKeeper::StopAll", 0x00468740, &CEntityEffectKeeper::StopAll);
KEEPER_C05_REVERSED("CEntityEffectKeeper::Start", 0x00468770, &CEntityEffectKeeper::Start);
KEEPER_C05_REVERSED("CEntityEffectKeeper::Stop", 0x00468820, &CEntityEffectKeeper::Stop);
KEEPER_C05_REVERSED("CEntityEffectKeeper::Find", 0x004688E0, &CEntityEffectKeeper::Find);
KEEPER_C05_REVERSED("CEntityEffectKeeper::CEntityEffectKeeper", 0x00468950, &CEntityEffectKeeper::Construct);
KEEPER_C05_REVERSED("CEntityEffectKeeper::~CEntityEffectKeeper", 0x00468980, &CEntityEffectKeeper::Destruct);
KEEPER_C05_REVERSED("CEntityEffectKeeper::FindNearestBone", 0x004689B0, &CEntityEffectKeeper::FindNearestBone);
KEEPER_C05_REVERSED("CEntityEffectKeeper::StartAtNearestBone", 0x00468AB0, &CEntityEffectKeeper::StartAtNearestBone);
