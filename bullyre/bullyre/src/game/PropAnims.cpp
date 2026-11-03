// CPropAnim bits around the door manager: the three special props (by model) and the
// area-37 prop list drawn late, the debug use-point shapes, a prop's animation hookup
// (+0x278..+0x290), the chapter's used props ("SaveData"), the glows cast on the ground
// (0xCA1FE8), and the prop pool's delete.

#include <cstdint>
#include <cstddef>

#include "reversed.h"

#include "game/Crt.h"
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

GAME_VAR(uint8_t*, pa_playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, pa_currArea, 0x00BD1008);
GAME_VAR(uint8_t, pa_chapter, 0x00A147E0);
GAME_VAR(void*, pa_camera, 0x00C3CCF4);
// the debug use-point drawing (+0x00 on, the node and the shape)
GAME_VAR(uint8_t, pa_debugUsePoints, 0x00C3B700);
GAME_VAR(int32_t, pa_debugNode, 0x00C3B6F8);
GAME_VAR(int32_t, pa_debugShape, 0x00C3B6FC);
// the three special props (models 0xBF4488, 0xBF447C, 0xBF4484), whether each was drawn
// already, and whether each is near the camera
GAME_ARRAY(uint8_t*, pa_specialProps, 0x00C3B7F0);
GAME_ARRAY(uint8_t, pa_specialDrawn, 0x00C3B7FC);
GAME_ARRAY(uint8_t, pa_specialNear, 0x00C3B800);
GAME_VAR(int32_t, pa_specialModel0, 0x00BF4488);
GAME_VAR(int32_t, pa_specialModel1, 0x00BF447C);
GAME_VAR(int32_t, pa_specialModel2, 0x00BF4484);
// the props drawn late in area 37 (up to 8, with a flag each)
GAME_ARRAY(uint8_t*, pa_lateProps, 0x00C3B7D0);
GAME_ARRAY(uint8_t, pa_latePropOn, 0x00C3B7C8);
GAME_VAR(int8_t, pa_numLateProps, 0x00C3B7FF);
// the chapter's used props (id, 1), up to 75
struct UsedProp {
    int32_t id;
    int32_t value;
};
GAME_VAR(int32_t, pa_numUsedProps, 0x00C3C9E0);
GAME_ARRAY(UsedProp, pa_usedProps, 0x00C3C9E8);
// the lit-prop records
GAME_VAR(int32_t, pa_numLitProps, 0x00CA064C);
GAME_ARRAY(uint8_t, pa_litProps, 0x00CA1FE8);   // 0x58 each, 32
struct PoolHeader {
    re::Ptr32<uint8_t> objects;
    re::Ptr32<uint8_t> flags;
    int32_t size;
    int32_t elemSize;
    int32_t unknown10;
    int32_t lastUsed;
};
GAME_VAR(PoolHeader*, pa_propAnimPool, 0x00C0F608);
inline void* const pa_renderer = reinterpret_cast<void*>(0x020C4910);
inline void* const pa_soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const pa_drawSpecialAtomic = reinterpret_cast<void*>(0x004E8D90);
inline const char* const pa_saveName = reinterpret_cast<const char*>(0x00913DF8);   // "SaveData"

GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, void* object);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, void* entity, int node, float* transform, int flag);
GAME_FN(void, __cdecl, DrawUsePointShape0, 0x004DC0F0, float* transform);
GAME_FN(void, __cdecl, DrawUsePointShape1, 0x004E04E0, float* transform);
GAME_FN(void, __cdecl, DrawUsePointShape2, 0x004E48E0, float* transform);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(CVec*, __cdecl, VectorSub, 0x00414B70, CVec* out, const CVec* a, const CVec* b);
GAME_FN(void, __cdecl, ForAllAtomics, 0x006C7380, void* clump, void* callback, void* data);
GAME_FN(void, __cdecl, RemoveModel, 0x0052F640, int index);
GAME_FN(uint8_t*, __cdecl, GetObjectInfo, 0x004D0CA0, int model);
GAME_FN(void, __thiscall, SoundBanks_Unload, 0x0059A420, void* banks, int bank, int, int);
GAME_FN(void*, __thiscall, AnimObject_GetBone, 0x00464EA0, void* animObject, uint32_t bone);
GAME_FN(void*, __cdecl, RwObject_GetNode, 0x006C7370, void* rwObject);
GAME_FN(void, __thiscall, Renderer_AddAnim, 0x006C2070, void* renderer, void* anim, int animId, void* node,
        void* handle, void* bone, void* animObject, uintptr_t);
GAME_FN(void, __thiscall, Renderer_RemoveAnim, 0x006C2200, void* renderer, void* anim, void* handle);
inline void __cdecl Memset(void* p, int value, size_t size) { crt::Memset(p, value, size); }
GAME_FN(void, __cdecl, ClearUsedProps, 0x004EA630);
GAME_FN(uint8_t, __cdecl, Save_CheckValue, 0x00733320, int, int, int value, int expected);
GAME_FN(uint8_t, __cdecl, Save_CheckBlock, 0x00733620, const char* name, void* data, int size);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412720, void* matrix, const void* from);
GAME_FN(void, __cdecl, CallAddLitPropRecord, 0x004EA700, uint8_t* record);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<void>* ref, void* entity);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

const CVec& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVec*>(matrix + 0x30) : At<CVec>(entity, 4);
}

// 0x4E8CE0: the debug shape of the player's use-point node (0 box, 1 and 2 others)
void __cdecl DrawDebugUsePoint() {
    if (pa_debugUsePoints != 1 || pa_playerPed == nullptr) {
        return;
    }
    uint8_t* entity = UsePoint_GetEntity(VCall<void*>(pa_playerPed, 0x40));
    float transform[13];
    transform[12] = 1.0f;   // the scale
    if (!GetNodeTransform(entity, pa_debugNode, transform, 0)) {
        return;
    }
    switch (pa_debugShape) {
    case 0:
        DrawUsePointShape0(transform);
        break;
    case 1:
        DrawUsePointShape1(transform);
        break;
    case 2:
        DrawUsePointShape2(transform);
        break;
    }
}

// 0x4E8DB0: a prop for area 37's late drawing (the last slot reused past 8)
void __cdecl AddLateProp(uint8_t* prop) {
    int i = pa_numLateProps;
    int8_t count = int8_t(pa_numLateProps + 1);
    pa_lateProps[i] = prop;
    pa_latePropOn[i] = 1;
    pa_numLateProps = count;
    if (count >= 8) {
        pa_numLateProps = 7;
    }
}

// 0x4E8DE0
void __cdecl RemoveLateProp(uint8_t* prop) {
    for (int i = 0; i < pa_numLateProps; i++) {
        if (pa_lateProps[i] == prop) {
            pa_latePropOn[i] = 0;
            return;
        }
    }
}

// 0x4E8E10: which special props are within 120 of the camera
void __cdecl UpdateSpecialPropsNear() {
    for (int i = 0; i < 3; i++) {
        uint8_t near = 0;
        if (pa_specialProps[i] != nullptr) {
            uint8_t* camera = GetLocalTransform(GetCameraNode(pa_camera));
            CVec d;
            VectorSub(&d, &Position(pa_specialProps[i]), reinterpret_cast<CVec*>(camera + 0x24));
            float distSq = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
            if (distSq < 14400.0) {
                near = 1;
            }
        }
        pa_specialNear[i] = near;
    }
}

// 0x4E8EB0: draws the special props not drawn yet (the first with its atomics lit from
// its node's +0x58), area 37's late props, and the player's prop when it's the first
// special model (the player drawn with it, flags forced)
void __cdecl DrawSpecialProps() {
    for (int i = 0; i < 3; i++) {
        if (pa_specialNear[i] && !pa_specialDrawn[i] && pa_specialProps[i] != nullptr) {
            VCall<void>(pa_specialProps[i], 0x5C);
            if (i == 0 && At<void*>(pa_specialProps[0], 0x18) != nullptr) {
                uint8_t* prop = pa_specialProps[0];
                uint8_t* node = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(prop, 0x114), 0x14), 8);
                CVec data = At<CVec>(node, 0x58);
                ForAllAtomics(At<void*>(prop, 0x18), pa_drawSpecialAtomic, &data);
            }
            VCall<void>(pa_specialProps[i], 0x48);
        }
        pa_specialDrawn[i] = 0;
    }
    if (pa_currArea == 37) {
        for (int i = 0; i < pa_numLateProps; i++) {
            if (pa_latePropOn[i]) {
                VCall<void>(pa_lateProps[i], 0x5C);
                VCall<void>(pa_lateProps[i], 0x48);
            }
        }
        pa_numLateProps = 0;
    }
    uint8_t* ped = pa_playerPed;
    uint8_t* prop = At<uint8_t*>(ped, 0x548);
    if (prop != nullptr && At<int16_t>(prop, 0x10E) == pa_specialModel0) {
        int32_t wasD0 = At<int32_t>(ped, 0xD0) != 0;
        int32_t was48 = At<int32_t>(ped, 0x48) != 0;
        At<int32_t>(ped, 0xD0) = 0;
        At<int32_t>(ped, 0x48) = 1;
        VCall<void>(ped, 0x48);
        At<int32_t>(ped, 0xD0) = wasD0;
        At<int32_t>(ped, 0x48) = was48;
    }
}

// 0x4EA2F0: a prop animation's model removed (the ids after 0x5D41)
void __cdecl RemovePropAnimModel(int index) {
    RemoveModel(index + 0x5D41);
}

// 0x4EA310 (thiscall): the prop's sound bank unloaded, when it has one loaded
void __fastcall UnloadSoundBank(uint8_t* self) {
    if (At<int32_t>(self, 0x178) == 0) {
        return;
    }
    uint8_t* info = GetObjectInfo(At<int16_t>(self, 0x10E));
    if (info != nullptr && At<int16_t>(info, 0xE) != -1) {
        SoundBanks_Unload(pa_soundBanks, At<int16_t>(info, 0xE), 1, 1);
        At<int32_t>(self, 0x178) = 0;
    }
}

// 0x4EA4C0 (thiscall): starts the prop's animation (+0x290, id +0x278) when not running:
// on its animation object's bone +0x27C, or on its RW object's node
void __fastcall StartAnim(uint8_t* self) {
    if (At<void*>(self, 0x28C) != nullptr) {
        return;
    }
    void* rwObject = At<void*>(self, 0x18);
    if (rwObject == nullptr) {
        return;
    }
    uint32_t bone = At<uint32_t>(self, 0x27C);
    void* boneNode = nullptr;
    if (bone != 0xFFFFFFFF) {
        uint8_t* animObject = At<uint8_t*>(At<uint8_t*>(self, 0x114), 0x94);
        if (animObject != nullptr && bone < At<uint32_t>(animObject, 0x9C)) {
            boneNode = AnimObject_GetBone(animObject, bone);
        }
    }
    uint8_t* node = At<uint8_t*>(self, 0x114);
    if (At<void*>(node, 0x94) != nullptr) {
        Renderer_AddAnim(pa_renderer, At<void*>(self, 0x290), At<int32_t>(self, 0x278), nullptr, &At<void*>(self, 0x28C),
                         boneNode, node, 0);
        return;
    }
    Renderer_AddAnim(pa_renderer, At<void*>(self, 0x290), At<int32_t>(self, 0x278), RwObject_GetNode(rwObject),
                     &At<void*>(self, 0x28C), nullptr, nullptr, 0);
}

// 0x4EA570 (thiscall): stops it
void __fastcall StopAnim(uint8_t* self) {
    if (At<void*>(self, 0x290) != nullptr && At<void*>(self, 0x28C) != nullptr) {
        Renderer_RemoveAnim(pa_renderer, At<void*>(self, 0x290), At<void*>(self, 0x28C));
        At<void*>(self, 0x290) = nullptr;
        At<void*>(self, 0x28C) = nullptr;
    }
}

// 0x4EAEC0 (thiscall): a new animation (the running one stopped)
void __fastcall SetAnim(uint8_t* self, re::Edx, void* anim, int animId, uint32_t bone) {
    if (At<void*>(self, 0x290) != nullptr && At<void*>(self, 0x28C) != nullptr) {
        Renderer_RemoveAnim(pa_renderer, At<void*>(self, 0x290), At<void*>(self, 0x28C));
        At<void*>(self, 0x290) = nullptr;
        At<void*>(self, 0x28C) = nullptr;
    }
    At<void*>(self, 0x290) = anim;
    At<int32_t>(self, 0x278) = animId;
    At<uint32_t>(self, 0x27C) = bone;
}

// 0x4EAF20 (thiscall): the index of the child prop (+0x280, +0x288 of them) running an
// animation id, or -1
uint32_t __fastcall FindChildByAnim(uint8_t* self, re::Edx, int animId) {
    uint32_t count = At<uint32_t>(self, 0x288);
    re::Ptr32<uint8_t>* children = At<re::Ptr32<uint8_t>*>(self, 0x280);
    for (uint32_t i = 0; i < count; i++) {
        if (children[i] != nullptr && At<int32_t>(children[i], 0x278) == animId) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

// the used props' save block
struct UsedPropSave {
    int32_t version;         // +0 (1)
    int32_t size;            // +4 (0x330)
    int32_t capacity;        // +8 (75)
    int32_t chapter;         // +0xC
    UsedProp props[75];      // +0x10
    uint8_t unknown268[200]; // +0x268 "SaveData"
};
static_assert(offsetof(UsedPropSave, unknown268) == 0x268);

// 0x4EA5B0
void __cdecl SaveUsedProps(UsedPropSave* out) {
    int count = pa_numUsedProps;
    out->version = 1;
    out->size = 0x330;
    out->capacity = 75;
    out->chapter = pa_chapter;
    if (count == 0) {
        Memset(out->props, 0, 600);
        Memset(out->unknown268, 0, 200);
        return;
    }
    for (int i = 0; i < count; i++) {
        out->props[i] = pa_usedProps[i];
    }
}

// 0x4EAF70: loaded back when saved in this chapter (the empty entries dropped)
void __cdecl LoadUsedProps(UsedPropSave* from) {
    if (!Save_CheckValue(0, 0, from->version, 1) || !Save_CheckValue(0, 0, from->size, 0x330) ||
        !Save_CheckValue(0, 0, from->capacity, 75) || !Save_CheckBlock(pa_saveName, from->unknown268, 0x32)) {
        return;
    }
    ClearUsedProps();
    if (from->chapter != pa_chapter) {
        return;
    }
    int count = 0;
    for (int i = 0; i < 75; i++) {
        if (from->props[i].id != 0) {
            pa_usedProps[count] = from->props[i];
            count++;
        }
    }
    pa_numUsedProps = count;
}

// 0x4EA6A0 (thiscall): marks the prop used this chapter (by its +0x23C's id)
void __fastcall MarkUsed(uint8_t* self) {
    int count = pa_numUsedProps;
    if (count >= 75) {
        return;
    }
    int32_t id = At<int32_t>(At<uint8_t*>(self, 0x23C), 0x2C);
    for (int i = 0; i < count; i++) {
        if (id == pa_usedProps[i].id) {
            return;
        }
    }
    At<int32_t>(self, 0x1B4) = 1;
    pa_usedProps[count].id = id;
    pa_usedProps[count].value = 1;
    pa_numUsedProps = count + 1;
}

// 0x4EA700: a lit-prop record (a matrix and its parameters; the slot's +0x54 is kept)
void __cdecl AddLitPropRecord(uint8_t* record) {
    int i = pa_numLitProps;
    if (i >= 32) {
        return;
    }
    uint8_t* slot = pa_litProps + i * 0x58;
    int32_t kept = At<int32_t>(slot, 0x54);
    Matrix_Copy(slot, record);
    At<int32_t>(slot, 0x44) = At<int32_t>(record, 0x44);
    At<float>(slot, 0x48) = At<float>(record, 0x48);
    At<float>(slot, 0x4C) = At<float>(record, 0x4C);
    At<int32_t>(slot, 0x50) = At<int32_t>(record, 0x50);
    At<int32_t>(slot, 0x54) = At<int32_t>(record, 0x54);
    At<int32_t>(pa_litProps + pa_numLitProps * 0x58, 0x54) = kept;
    pa_numLitProps++;
}

// 0x4EA770 (stdcall): a glow from a transform (its axes reordered: right, -at, up), with
// a range of 20
void __stdcall AddLitProp(const float* m, int colour, float size, int texture) {
    // (the matrix's pads and the record's +0x54 are the game's uninitialised stack)
    alignas(4) uint8_t record[0x58] = {};
    float* r = reinterpret_cast<float*>(record);
    r[0] = m[0];
    r[1] = m[3];
    r[2] = m[6];
    r[4] = float(m[2] * -1.0);
    r[5] = float(m[5] * -1.0);
    r[6] = float(m[8] * -1.0);
    r[8] = m[1];
    r[9] = m[4];
    r[10] = m[7];
    r[12] = m[9];
    r[13] = m[10];
    r[14] = m[11];
    r[15] = m[12];
    At<int32_t>(record, 0x40) = 0;
    At<int32_t>(record, 0x44) = colour;
    At<float>(record, 0x48) = size;
    At<float>(record, 0x4C) = 20.0f;
    At<int32_t>(record, 0x50) = texture;
    CallAddLitPropRecord(record);
}

// 0x4EB0A0 (thiscall): an array of references made with new[] (count before it), let go
struct RefArray {
    re::Ptr32<re::Ptr32<void>> refs;     // +0
    int32_t count;   // +4
    int32_t unknown8;
};

void __fastcall RefArray_Free(RefArray* self) {
    re::Ptr32<void>* refs = self->refs;
    if (refs == nullptr) {
        self->count = 0;
        self->unknown8 = 0;
        return;
    }
    int32_t* block = reinterpret_cast<int32_t*>(refs) - 1;
    for (int i = *block - 1; i >= 0; i--) {
        SetRef(&refs[i], nullptr);
    }
    Free(block);
    self->refs = nullptr;
    self->count = 0;
    self->unknown8 = 0;
}

// 0x4EB190: CPropAnim's operator delete (a special prop forgotten)
void __cdecl PropAnim_Delete(uint8_t* prop) {
    if (At<int16_t>(prop, 0x10E) == pa_specialModel0) {
        pa_specialProps[0] = nullptr;
    }
    if (At<int16_t>(prop, 0x10E) == pa_specialModel2) {
        pa_specialProps[2] = nullptr;
    }
    if (At<int16_t>(prop, 0x10E) == pa_specialModel1) {
        pa_specialProps[1] = nullptr;
    }
    PoolHeader* pool = pa_propAnimPool;
    int slot = int((prop - pool->objects) / pool->elemSize);
    pool->flags[slot] |= 0x80;
    if (slot < pool->lastUsed) {
        pool->lastUsed = slot;
    }
}

#define C13B_PA_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C13B_PA_REVERSED("CPropAnim::DrawDebugUsePoint", 0x004E8CE0, &DrawDebugUsePoint);
C13B_PA_REVERSED("CPropAnim::AddLateProp", 0x004E8DB0, &AddLateProp);
C13B_PA_REVERSED("CPropAnim::RemoveLateProp", 0x004E8DE0, &RemoveLateProp);
C13B_PA_REVERSED("CPropAnim::UpdateSpecialPropsNear", 0x004E8E10, &UpdateSpecialPropsNear);
C13B_PA_REVERSED("CPropAnim::DrawSpecialProps", 0x004E8EB0, &DrawSpecialProps);
C13B_PA_REVERSED("CPAnim::RemoveModel", 0x004EA2F0, &RemovePropAnimModel);
C13B_PA_REVERSED("CPropAnim::UnloadSoundBank", 0x004EA310, &UnloadSoundBank);
C13B_PA_REVERSED("CPropAnim::StartAnim", 0x004EA4C0, &StartAnim);
C13B_PA_REVERSED("CPropAnim::StopAnim", 0x004EA570, &StopAnim);
C13B_PA_REVERSED("CPropAnim::SaveUsedProps", 0x004EA5B0, &SaveUsedProps);
C13B_PA_REVERSED("CPropAnim::MarkUsed", 0x004EA6A0, &MarkUsed);
C13B_PA_REVERSED("LitProps::AddRecord", 0x004EA700, &AddLitPropRecord);
C13B_PA_REVERSED("LitProps::Add", 0x004EA770, &AddLitProp);
C13B_PA_REVERSED("CPropAnim::SetAnim", 0x004EAEC0, &SetAnim);
C13B_PA_REVERSED("CPropAnim::FindChildByAnim", 0x004EAF20, &FindChildByAnim);
C13B_PA_REVERSED("CPropAnim::LoadUsedProps", 0x004EAF70, &LoadUsedProps);
C13B_PA_REVERSED("RefArray::Free_4EB0A0", 0x004EB0A0, &RefArray_Free);
C13B_PA_REVERSED("CPropAnim::operator delete", 0x004EB190, &PropAnim_Delete);

} // namespace
