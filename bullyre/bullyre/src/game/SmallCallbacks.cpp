// The game's small functions our code hands back to it by address (array constructors and
// destructors for the compiler's vector iterators, RenderWare's for-all callbacks, the Lua
// store's enum lookups, load callbacks): written here so the 64-bit dispatch has a body at
// each (the 32-bit build keeps the game's: they're registered, never hooked).

#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace smallcb {

using re::At;

GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void*, __thiscall, PopInfo_Construct, 0x004A0C70, void* self);
GAME_FN(void, __thiscall, Stimuli_LoadTypes, 0x004A53F0, void* self);
GAME_FN(void, __thiscall, PAnimLoadActData, 0x005205B0, void* mi);
GAME_FN(void, __cdecl, Atomic_SetFrame, 0x006C6A10, void* atomic, void* frame);
GAME_FN(void*, __cdecl, Frame_FirstObject, 0x006C7700, void* frame);
GAME_FN(const char*, __cdecl, Script_GetString, 0x005D8440, void* L, int index);
GAME_FN(void, __cdecl, Script_PushInt, 0x005D85C0, void* L, int32_t value);
GAME_FN(int, __thiscall, POIs_FindByName, 0x00448F80, void* manager, const char* name);
GAME_FN(int, __cdecl, GetModelIndex, 0x0051C330, const char* name);
GAME_VAR(re::Ptr32<void>, ms_poiManager, 0x00C0F5E4);

inline float GameFloat(uintptr_t address) {
    return *reinterpret_cast<const float*>(address);
}

// ---- constructors and destructors (thiscall, `this` back)

// 0x413DC0: RwMatrix's (its flag word +0x30 made 1.0f)
uint8_t* __fastcall Matrix_Construct(uint8_t* self, re::Edx) {
    At<float>(self, 0x30) = 1.0f;
    return self;
}

// 0x436BE0: a quad tree node's: its rectangle (left/bottom 0x903498's value, right/top
// 0x903494's) and no children
uint8_t* __fastcall QuadTreeNode_Construct(uint8_t* self, re::Edx) {
    float low = GameFloat(0x00903498);
    float high = GameFloat(0x00903494);
    At<float>(self, 0) = low;
    At<float>(self, 0xC) = low;
    At<float>(self, 8) = high;
    At<float>(self, 4) = high;
    At<uint32_t>(self, 0x10) = 0;
    At<uint32_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x18) = 0;
    return self;
}

// 0x4498A0: an entity reference's destructor (the entity told to forget it)
void __fastcall EntityRef_Destruct(re::Ptr32<void>* self, re::Edx) {
    if (*self != nullptr) {
        CleanUpOldReference(*self, self);
    }
}

// 0x45BE60: an unlock's (no name, +0x18 cleared)
uint8_t* __fastcall Unlock_Construct(uint8_t* self, re::Edx) {
    self[0] = 0;
    At<uint32_t>(self, 0x18) = 0;
    return self;
}

// 0x46D770: a matrix link's (+0x40 cleared)
uint8_t* __fastcall MatrixLink_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x40) = 0;
    return self;
}

// 0x49C190: an area's population info (its vtable 0x911DD8, four CPopulationInfo of 0x24)
uint8_t* __fastcall AreaPopulationInfo_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x00911DD8;
    for (int i = 0; i < 4; i++) {
        PopInfo_Construct(self + 4 + i * 0x24);
    }
    return self;
}

// 0x4D6380: a target entry's (+0x14 0x9139E4's value, +0 and +0x10 cleared)
uint8_t* __fastcall TargetEntry_Construct(uint8_t* self, re::Edx) {
    At<float>(self, 0x14) = GameFloat(0x009139E4);
    At<uint32_t>(self, 0) = 0;
    At<uint32_t>(self, 0x10) = 0;
    return self;
}

// 0x4F7A20: a cutscene camera key's (its nine floats from +4 zeroed)
uint8_t* __fastcall CameraKey_Construct(uint8_t* self, re::Edx) {
    for (int i = 0; i < 9; i++) {
        At<float>(self, 4 + i * 4) = 0.0f;
    }
    return self;
}

// 0x5A6E10: a stream channel's (kind 6, its levels and times zeroed)
uint8_t* __fastcall StreamChannel_Construct(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.0f;
    At<uint32_t>(self, 0x14) = 6;
    At<uint32_t>(self, 0x1C) = 0;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
    At<uint32_t>(self, 0x28) = 0;
    At<uint32_t>(self, 0x2C) = 0;
    At<float>(self, 0x44) = 0.0f;
    At<float>(self, 0x48) = 0.0f;
    At<uint32_t>(self, 0x40) = 0;
    At<float>(self, 0x4C) = 0.0f;
    return self;
}

// 0x5EFB40: NiColorA's (all zero; the game leaves eax as it was)
float* __fastcall ColorA_Construct(float* self, re::Edx) {
    self[0] = self[1] = self[2] = self[3] = 0.0f;
    return self;
}

// 0x5F4CB0: an action node's parent pair (no key, no parent)
uint8_t* __fastcall ParentPair_Construct(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0) = -1;
    At<uint32_t>(self, 4) = 0;
    return self;
}

// ---- callbacks (cdecl)

// 0x4890E0: the objective queue's predicate: a jump objective (type 0x1A at +8)
uint8_t __cdecl IsJumpObjective(const uint8_t* objective) {
    return objective != nullptr && At<int32_t>(objective, 8) == 0x1A ? 1 : 0;
}

// 0x4A5E50: the stimulus types' load callback (the manager's LoadTypes)
void __cdecl Stimuli_LoadTypesCallback(void* manager) {
    Stimuli_LoadTypes(manager);
}

// 0x4B97D0: a bike atomic's render setup: its alpha kept (+0x34 into +0x44), drawn
// specially (flag 1 of +3 cleared), the colour (+0x38) from `rgb`
uint8_t* __cdecl BikeAtomic_Setup(uint8_t* atomic, const uint32_t* rgb) {
    At<float>(atomic, 0x44) = At<float>(atomic, 0x34);
    atomic[3] &= 0xFE;
    At<uint32_t>(atomic, 0x38) = rgb[0];
    At<uint32_t>(atomic, 0x3C) = rgb[1];
    At<uint32_t>(atomic, 0x40) = rgb[2];
    return atomic;
}

// 0x4E8D90: a prop atomic's special draw (flag 1 of +3 cleared, the colour from `rgb`)
uint8_t* __cdecl PropAtomic_DrawSpecial(uint8_t* atomic, const uint32_t* rgb) {
    atomic[3] &= 0xFE;
    At<uint32_t>(atomic, 0x38) = rgb[0];
    At<uint32_t>(atomic, 0x3C) = rgb[1];
    At<uint32_t>(atomic, 0x40) = rgb[2];
    return atomic;
}

// 0x520CA0: the load callback: every prop animation model info (kind 0xA) in the table at
// 0xC67738..0xC735D8 loads its act data
void __cdecl PAnimModels_LoadActData() {
    for (uintptr_t p = 0x00C67738; p < 0x00C735D8; p += 4) {
        uint8_t* mi = *reinterpret_cast<re::Ptr32<uint8_t>*>(p);
        if (mi != nullptr && mi[8] == 0xA) {
            PAnimLoadActData(mi);
        }
    }
}

// 0x534BB0: an atomic given the frame `data` (RpClumpForAllAtomics)
void* __cdecl Atomic_SetBoneCallback(void* atomic, void* frame) {
    Atomic_SetFrame(atomic, frame);
    return atomic;
}

// 0x534BD0 and 0x534BE0: an atomic's flags (+0x6C) or'd with / cleared of `bits`
uint8_t* __cdecl Atomic_SetFlags(uint8_t* atomic, uint32_t bits) {
    At<uint32_t>(atomic, 0x6C) |= bits;
    return atomic;
}
uint8_t* __cdecl Atomic_ClearFlags(uint8_t* atomic, uint32_t bits) {
    At<uint32_t>(atomic, 0x6C) &= ~bits;
    return atomic;
}

// 0x534BF0: a vehicle part's atomics: the ok one (flag 1) into out[0], the damaged one
// (flag 2) into out[1]
uint8_t* __cdecl Atomic_GetOkAndDamaged(uint8_t* atomic, re::Ptr32<uint8_t>* out) {
    uint32_t flags = At<uint32_t>(atomic, 0x6C);
    if (flags & 1) {
        out[0] = atomic;
    } else if (flags & 2) {
        out[1] = atomic;
    }
    return atomic;
}

// 0x5DB2D0 and 0x5DB330: the Lua store's enum lookups: the name (argument 1, past its first
// character: the enum's prefix) as a POI's index / a model's index, pushed
int __cdecl Lua_PoiEnum(void* L) {
    char name[0x80];
    std::strcpy(name, Script_GetString(L, 1) + 1);   // (unbounded: the game's)
    Script_PushInt(L, POIs_FindByName(ms_poiManager, name));
    return 1;
}
int __cdecl Lua_ModelEnum(void* L) {
    char name[0x80];
    std::strcpy(name, Script_GetString(L, 1) + 1);
    Script_PushInt(L, GetModelIndex(name));
    return 1;
}

// 0x5EE020: the first object (*data = it; stop)
void* __cdecl StoreFirst(void* object, re::Ptr32<void>* data) {
    *data = object;
    return nullptr;
}

// 0x5EE060: {n, result}: the n-th object (counted down; the 0th stops the walk)
void* __cdecl CountDown(void* object, int32_t* data) {
    if (data[0] > 0) {
        data[0]--;
        return object;
    }
    *reinterpret_cast<re::Ptr32<void>*>(data + 1) = object;
    return nullptr;
}

// 0x5EE180: the first object's frame's first object (its +4: the frame)
void* __cdecl FrameOfFirst(uint8_t* object, re::Ptr32<void>* data) {
    *data = Frame_FirstObject(At<void*>(object, 4));
    return nullptr;
}

} // namespace smallcb

RE_UNHOOKED_FN("RwMatrix::RwMatrix", 0x00413DC0, &smallcb::Matrix_Construct);
RE_UNHOOKED_FN("QuadTreeNode::QuadTreeNode", 0x00436BE0, &smallcb::QuadTreeNode_Construct);
RE_UNHOOKED_FN("EntityRef::~EntityRef", 0x004498A0, &smallcb::EntityRef_Destruct);
RE_UNHOOKED_FN("Unlock::Unlock", 0x0045BE60, &smallcb::Unlock_Construct);
RE_UNHOOKED_FN("MatrixLink::MatrixLink", 0x0046D770, &smallcb::MatrixLink_Construct);
RE_UNHOOKED_FN("ObjectiveQueue::IsJumpObjective", 0x004890E0, &smallcb::IsJumpObjective);
RE_UNHOOKED_FN("AreaPopulationInfo::AreaPopulationInfo", 0x0049C190, &smallcb::AreaPopulationInfo_Construct);
RE_UNHOOKED_FN("CStimulusManager::LoadTypes (callback)", 0x004A5E50, &smallcb::Stimuli_LoadTypesCallback);
RE_UNHOOKED_FN("BikeAtomic::Setup", 0x004B97D0, &smallcb::BikeAtomic_Setup);
RE_UNHOOKED_FN("TargetEntry::TargetEntry", 0x004D6380, &smallcb::TargetEntry_Construct);
RE_UNHOOKED_FN("PropAtomic::DrawSpecial", 0x004E8D90, &smallcb::PropAtomic_DrawSpecial);
RE_UNHOOKED_FN("CutsceneCameraKey::CutsceneCameraKey", 0x004F7A20, &smallcb::CameraKey_Construct);
RE_UNHOOKED_FN("PAnimModels::LoadActData (load callback)", 0x00520CA0, &smallcb::PAnimModels_LoadActData);
RE_UNHOOKED_FN("Atomic::SetBone (callback)", 0x00534BB0, &smallcb::Atomic_SetBoneCallback);
RE_UNHOOKED_FN("Atomic::SetFlags (callback)", 0x00534BD0, &smallcb::Atomic_SetFlags);
RE_UNHOOKED_FN("Atomic::ClearFlags (callback)", 0x00534BE0, &smallcb::Atomic_ClearFlags);
RE_UNHOOKED_FN("Atomic::GetOkAndDamaged (callback)", 0x00534BF0, &smallcb::Atomic_GetOkAndDamaged);
RE_UNHOOKED_FN("StreamChannel::StreamChannel", 0x005A6E10, &smallcb::StreamChannel_Construct);
RE_UNHOOKED_FN("LurStore::PoiEnum", 0x005DB2D0, &smallcb::Lua_PoiEnum);
RE_UNHOOKED_FN("LurStore::ModelEnum", 0x005DB330, &smallcb::Lua_ModelEnum);
RE_UNHOOKED_FN("RwObject::StoreFirst (callback)", 0x005EE020, &smallcb::StoreFirst);
RE_UNHOOKED_FN("RwObject::CountDown (callback)", 0x005EE060, &smallcb::CountDown);
RE_UNHOOKED_FN("RwObject::FrameOfFirst (callback)", 0x005EE180, &smallcb::FrameOfFirst);
RE_UNHOOKED_FN("NiColorA::NiColorA", 0x005EFB40, &smallcb::ColorA_Construct);
RE_UNHOOKED_FN("ParentPair::ParentPair", 0x005F4CB0, &smallcb::ParentPair_Construct);
