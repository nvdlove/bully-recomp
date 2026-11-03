// The tagging (graffiti) object at 0x20C8A00: the tag textures (C<n>_<nn> in the tag
// dictionaries, streamed one dictionary ahead), the props with tag triggers, the tagged
// props drawn as textured quads each frame, the gang a spot belongs to, the tags just
// finished. 0x72BEF0..0x72E090.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/ActionTree.h"
#include "game/Pool.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

// a tag texture slot
struct TagSlot {
    re::Ptr32<void> texture;    // +0x0 (taken from its dictionary)
    int16_t txdSlot;  // +0x4 (-1: none)
    uint8_t wanted;   // +0x6 (used this frame)
    uint8_t pad7;
};
static_assert(sizeof(TagSlot) == 0x8);

// a tag just finished (handed to the script once)
struct TagFinished {
    uint32_t userdata;  // +0x0
    uint32_t ped;       // +0x4
    float timer;        // +0x8 (> 0: not taken yet)
};
static_assert(sizeof(TagFinished) == 0xC);

// a prop's trigger: its action node path (up to 16 name keys, from +0x40 down)
struct TagTrigger {
    re::Ptr32<void> tree;           // +0x0
    uint32_t keys[16];    // +0x4
    int32_t numKeys;      // +0x44
    uint32_t action;      // +0x48
    uint32_t node;        // +0x4C
};
static_assert(sizeof(TagTrigger) == 0x50);

struct Tagging {
    TagSlot slots[16];         // +0x0
    TagFinished finished[4];   // +0x80
    re::Ptr32<TagTrigger> triggers;      // +0xB0 (new[] of 16, the count before them)
    int16_t glowTxd;           // +0xB4 "GlowL2"
    uint8_t padB6[2];
    re::Ptr32<void> glowTexture;         // +0xB8
    re::Ptr32<void> infoTexture;         // +0xBC "C_Info"
    uint8_t unknownC0;         // +0xC0
    uint8_t unknownC1;         // +0xC1 (a tag slot waits for its dictionary)
    int8_t nextFinished;       // +0xC2
    uint8_t padC3;
    int32_t users;             // +0xC4
    re::Ptr32<void> tagTexture;          // +0xC8
    int32_t prefix;            // +0xCC
    int32_t set;               // +0xD0
    int32_t frame;             // +0xD4
    float frameTimer;          // +0xD8
    int32_t txd;               // +0xDC the tag dictionary loaded
    int32_t nextTxd;           // +0xE0 the next one (streaming)
    uint32_t stream;           // +0xE4 its music stream
    re::Ptr32<uint8_t> highlighted;      // +0xE8 (registered reference)
    re::Ptr32<void> sound;               // +0xEC
    re::Ptr32<uint8_t> props[40];        // +0xF0 (registered references)
    int32_t numProps;          // +0x190
    int32_t numTriggers;       // +0x194
    int32_t mission;           // +0x198
    uint8_t unknown19C;        // +0x19C
    uint8_t unknown19D;        // +0x19D
};
static_assert(offsetof(Tagging, finished) == 0x80);
static_assert(offsetof(Tagging, triggers) == 0xB0);
static_assert(offsetof(Tagging, users) == 0xC4);
static_assert(offsetof(Tagging, highlighted) == 0xE8);
static_assert(offsetof(Tagging, numProps) == 0x190);
static_assert(offsetof(Tagging, unknown19D) == 0x19D);

// NiTransform (rotation rows, translation, scale)
struct TagTransform {
    float m[9];    // +0x0
    float t[3];    // +0x24
    float scale;   // +0x30
};
static_assert(sizeof(TagTransform) == 0x34);

// an Im3D vertex
struct TagVertex {
    float pos[3];     // +0x0
    float normal[3];  // +0xC
    uint32_t colour;  // +0x18 (ARGB)
    float u, v;       // +0x1C
};
static_assert(sizeof(TagVertex) == 0x24);

// a stream emitter's offer (StreamEmitter::OfferCandidate)
struct TagEmitterOffer {
    float pos[3];      // +0x0
    float volume;      // +0xC
    uint8_t flags;     // +0x10 (bits 1-2 cleared, bit 3 set; the rest as the stack was)
    uint8_t pad11[3];
    uint32_t flags2;   // +0x14 (bits 1-3 cleared, bit 0 set; the rest as the stack was)
    uint32_t stream;   // +0x18
    int16_t range1;    // +0x1C
    int16_t range2;    // +0x1E
};
static_assert(sizeof(TagEmitterOffer) == 0x20);

// a lit prop record (AddLitPropRecord)
struct TagLitRecord {
    float right[3];     // +0x0
    uint32_t unused0C;
    float back[3];      // +0x10
    uint32_t unused1C;
    float up[3];        // +0x20
    uint32_t unused2C;
    float pos[3];       // +0x30
    uint32_t unused3C;
    int32_t unknown40;  // +0x40
    uint32_t colour;    // +0x44
    float unknown48;    // +0x48
    float unknown4C;    // +0x4C
    re::Ptr32<void> texture;      // +0x50
};
static_assert(sizeof(TagLitRecord) == 0x54);

GAME_VAR(Tagging, tagging, 0x020C8A00);
GAME_ARRAY(const char*, tagPrefixes, 0x00BC720C);
GAME_ARRAY(char, tagNameBuffer, 0x00C221A8);
GAME_VAR(CPool<re::Ptr32<uint8_t>>*, tagTxdPool, 0x01B64654);   // CTxdStore::ms_pTxdPool (its first 4 bytes the dictionary)
GAME_VAR(CPool<uint8_t>*, tagTriggerPool, 0x020C7C48);
GAME_ARRAY(uint8_t*, tagModelInfos, 0x00C67738);          // CModelInfo::ms_modelInfoPtrs
GAME_VAR(uint8_t*, tagCamera, 0x00C3CCF4);
GAME_VAR(uint8_t*, tagButes, 0x020C8908);                 // TaggingButes
GAME_VAR(int32_t, tagModelHighlight1, 0x00BF446C);
GAME_VAR(int32_t, tagModelHighlight2, 0x00BF4468);
GAME_VAR(int32_t, tagModelHighlight3, 0x00BF4464);
GAME_VAR(int32_t, tagModelTv, 0x00BF4470);
GAME_VAR(int32_t, tagModelBulletinBoard, 0x00BF3EA0);
GAME_VAR(uint32_t, tagNodeHash, 0x01B65504);
GAME_VAR(int32_t, tagCurrArea, 0x00BD1008);               // CGame::currArea
GAME_VAR(int32_t, tagWidescreen, 0x00C3CD0C);
GAME_VAR(int32_t, tagWidescreenTarget, 0x00C3CD10);
GAME_VAR(uint8_t, tagIs169, 0x00C1870E);
GAME_VAR(uint8_t, tagMenuPaused, 0x020C5BE6);
GAME_VAR(uint32_t, tagTimeInMs, 0x00C1A9B4);
GAME_VAR(float, tagTimeStep, 0x00C1A9A4);                 // CTimer::ms_fTimeStep
GAME_VAR(float, tagFadeRate, 0x00BC7210);
GAME_VAR(int32_t, tagMusicFading, 0x00CF4324);            // the music stream (its first field)
GAME_VAR(uint32_t, tagMusicStream, 0x00CF432C);
GAME_VAR(uint32_t, tagMusicPreloaded, 0x00AE5F9C);
inline void* const tagMusic = reinterpret_cast<void*>(0x00CF4324);
inline void* const tagMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const tagEmitters = reinterpret_cast<void*>(0x00AE5F60);
inline void* const tagAudio = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const tagTriggerNames = reinterpret_cast<void*>(0x020C7E98);
inline const uint32_t* const tagAnimTagging = reinterpret_cast<const uint32_t*>(0x01B652D8);
inline const uint32_t* const tagAnimSpraying = reinterpret_cast<const uint32_t*>(0x01B65510);
inline const uint32_t* const tagAnimHidden = reinterpret_cast<const uint32_t*>(0x01B652C8);
constexpr int kTxdModelBase = 0x2FA8;   // a dictionary's streaming id
constexpr uint32_t kRefNull = 0;

template <typename... A>
inline int __cdecl TagSprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(int, __cdecl, TagFindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(int, __cdecl, TagFindTxdSeasonal, 0x005F2450, uint32_t hash, int seasonal);
GAME_FN(int, __cdecl, TagTxdNameHash, 0x005F1060, int slot);
GAME_FN(void, __cdecl, TagPushTxd, 0x005F0A90);
GAME_FN(void, __cdecl, TagSetTxd, 0x005F1100, int slot);
GAME_FN(void, __cdecl, TagPopTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, TagPushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, TagPopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, TagFindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void*, __cdecl, TagDictFindTexture, 0x005F1610, void* dictionary, const char* name);
GAME_FN(void, __cdecl, TagForAllTextures, 0x005F1590, void* dictionary, void* callback, void* data);
GAME_FN(void, __cdecl, TagReleaseTexture, 0x005F0F90, void* texture);
GAME_FN(void, __cdecl, TagRemoveModel, 0x0052F640, int id);
GAME_FN(void, __cdecl, TagRequestModel, 0x00532020, int id, int flags);
GAME_FN(uint8_t*, __cdecl, TagStreamingInfo, 0x0052DD30, int id);
GAME_FN(uint32_t*, __thiscall, TagKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, TagKey_Destruct, 0x0049A650, uint32_t* key);
GAME_FN(void, __thiscall, TagKey_Default, 0x0066BEA0, uint32_t* key);
GAME_FN(int32_t, __thiscall, TagMission_Find, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, TagMission_Record, 0x006AA7C0, void* mgr, int32_t id);
GAME_FN(uint8_t*, __thiscall, TagMission_Slot0, 0x006AA680, void* mgr);
GAME_FN(uint8_t, __thiscall, TagMissionSlot_Is, 0x006AA410, uint8_t* slot, int32_t id);
GAME_FN(void, __thiscall, TagRef_Set, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(uint8_t*, __thiscall, TagRef_Get, 0x00773DB0, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, TagSound_Set, 0x0059D110, re::Ptr32<void>* sound, void* value);
GAME_FN(int32_t*, __thiscall, TagNode_GetId, 0x00655B80, void* node);
GAME_FN(void, __thiscall, TagTrack_SetSlot, 0x00884C70, void* track, int slot);
GAME_FN(int8_t, __thiscall, TagTrack_GetSlot, 0x00884C80, void* track);
GAME_FN(uint32_t*, __thiscall, TagTrack_GetColour, 0x00656E20, void* track, uint32_t* out);
GAME_FN(void*, __cdecl, TagCameraNode, 0x006C82F0, void* camera);
GAME_FN(TagTransform*, __cdecl, TagWorldTransform, 0x006C9A90, void* node);
GAME_FN(uint8_t, __cdecl, TagNodeTransform, 0x00467D80, uint8_t* entity, uint32_t node, TagTransform* out, int flag);
GAME_FN(uint8_t, __thiscall, TagCtl_IsPlaying, 0x005F4800, uint8_t* ctl, const uint32_t* name, int ancestors, int nested);
GAME_FN(void*, __thiscall, TagCtl_GetActiveTrack, 0x005F49E0, uint8_t* ctl, int index);
GAME_FN(void, __thiscall, TagCtl_PlayNode, 0x005F45F0, uint8_t* ctl, void* node);
GAME_FN(void*, __cdecl, TagActionTree_Find, 0x005F5B10, const char* path, void* from);
GAME_FN(void, __thiscall, TagProp_SetCurrent, 0x004ED940, uint8_t* prop);
GAME_FN(void, __thiscall, TagEmitters_Offer, 0x00587080, void* emitters, TagEmitterOffer* offer, float distanceSq);
GAME_FN(uint8_t*, __thiscall, TagRgba_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(uint8_t*, __thiscall, TagRgba_Copy, 0x00424450, uint8_t* dst, uint8_t* src);
GAME_FN(int, __cdecl, TagGetRaster, 0x0051A120, void* texture);
GAME_FN(void, __cdecl, TagSetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, TagGetRenderState, 0x00519290, int state, int* value);
GAME_FN(void*, __cdecl, TagIm3D_Transform, 0x005EFAF0, void* vertices, int count, void* transform, uint32_t flags);
GAME_FN(uint8_t, __cdecl, TagIm3D_RenderIndexed, 0x005F0170, int type, uint16_t* indices, int count, int unused);
GAME_FN(uint8_t, __cdecl, TagIm3D_End, 0x005EFB20);
GAME_FN(uint8_t, __cdecl, TagAreaIsExterior, 0x00430320, int area);
GAME_FN(int, __thiscall, TagTriggerNames_Find, 0x006D7A10, void* names, const char* name);
GAME_FN(uint8_t, __thiscall, TagTrigger_Contains2D, 0x006D6EB0, uint8_t* trigger, const float* point);
inline int __cdecl TagRand() { return crt::Rand(); }
GAME_FN(int, __thiscall, TagAudio_GetSoundId, 0x00595B80, void* audio, const char* name);
GAME_FN(void, __cdecl, TagSoundSlot_Play, 0x00597310, re::Ptr32<void>* slot, int id, const float* pos, int a, int b, int c, int d);
GAME_FN(float, __cdecl, TagMin, 0x00414470, float a, float b);
GAME_FN(void, __thiscall, TagVector_Assign, 0x004129D0, float* dst, const float* src);
GAME_FN(void, __cdecl, TagAddLitProp, 0x004EA700, TagLitRecord* record);
GAME_FN(uint8_t, __thiscall, TagMusic_IsPreloaded, 0x0058EE40, void* music);
GAME_FN(void, __thiscall, TagMusic_PlayPreloaded, 0x005905E0, void* music, int mode);
GAME_FN(uint32_t, __cdecl, TagHashStreamName, 0x005A7D60, const char* name, int flag);
GAME_FN(void*, __cdecl, TagMalloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, TagFree, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, TagVectorConstruct, 0x00401240, void* array, uint32_t size, int count, void* ctor);
GAME_FN(void, __stdcall, TagVectorDestruct, 0x004017C0, void* array, uint32_t size, int count, void* dtor);
GAME_FN(void*, __stdcall, TagTextureFor, 0x0072CB30, int txd, int prefix, int set, int frame, int create);
GAME_FN(int, __stdcall, TagSetTxdSlot, 0x0072C140, int prefix, int set);
GAME_FN(TagTrigger*, __thiscall, Tagging_FindTrigger, 0x0072C390, Tagging* self, void* tree);
GAME_FN(void, __thiscall, Tagging_ReleaseStream, 0x0072C180, Tagging* self);
GAME_FN(void, __thiscall, Tagging_WantSlot, 0x0072C1F0, Tagging* self, void* track);
GAME_FN(float, __stdcall, Tagging_Width, 0x0072C270, uint8_t* prop);
GAME_FN(float, __stdcall, Tagging_Height, 0x0072C2E0, uint8_t* prop);
GAME_FN(void, __stdcall, Tagging_DrawQuadAt, 0x0072C6B0, void* texture, const float* a, const float* b, const float* c, const float* d,
        uint32_t rgba);
GAME_FN(void, __thiscall, Tagging_DrawTag, 0x0072CBE0, Tagging* self, TagTransform* transform, void* texture, uint32_t rgba,
        float height, float width, float fade, int mode);
GAME_FN(void, __thiscall, Tagging_NextFrame, 0x0072DD20, Tagging* self, int txd, int nextTxd, int* prefix, int* set, int* frame);
GAME_FN(void, __thiscall, Tagging_UpdateStream, 0x0072DDF0, Tagging* self);

constexpr uintptr_t kTakeTextureCB = 0x005EE160;
constexpr uintptr_t kTriggerConstruct = 0x0072C3E0;
constexpr uintptr_t kKeyDestruct = 0x0049A650;

// a pool slot's object as the game reads it (no check of the index: -1 reads before the
// flags)
template <typename T>
T* PoolAt(CPool<T>* pool, int index) {
    return (pool->flags[index] & 0x80) != 0
               ? nullptr
               : re::PtrCast<T*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * index);
}
#if defined(_WIN64)
template <typename T>
T* PoolAt(re::Ptr32<CPool<T>> pool, int index) {
    return PoolAt(pool.get(), index);
}
#endif
// a named trigger (null when there's none)
uint8_t* FindNamedTrigger(const char* name) {
    int index = TagTriggerNames_Find(tagTriggerNames, name);
    return index == -1 ? nullptr : PoolAt(tagTriggerPool, index);
}
const float* EntityPos(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m != nullptr ? reinterpret_cast<float*>(m + 0x30) : reinterpret_cast<float*>(e + 4);
}
// a dictionary's streaming state (3, 4: requested / loading)
bool IsStreaming(int id) {
    return TagStreamingInfo(id)[4] == 3 || TagStreamingInfo(id)[4] == 4;
}
// the squared distance from the camera
float CameraDistanceSq(const float* pos) {
    TagTransform* cam = TagWorldTransform(TagCameraNode(tagCamera));
    float dx = pos[0] - cam->t[0];
    float dy = pos[1] - cam->t[1];
    float dz = pos[2] - cam->t[2];
    return dx * dx + dy * dy + dz * dz;
}

} // namespace

namespace tagging {

// 0x72BEF0 (thiscall): the constructor
Tagging* __fastcall Construct(Tagging* self, re::Edx) {
    for (int i = 0; i < 16; i++) {
        self->slots[i].texture = nullptr;
        self->slots[i].txdSlot = -1;
        self->slots[i].wanted = 0;
    }
    for (int i = 0x80; i <= 0xAC; i += 4) {
        At<uint32_t>(self, i) = 0;   // (the timers as 0.0f)
    }
    for (int i = 0xE8; i <= 0x18C; i += 4) {
        At<uint32_t>(self, i) = 0;
    }
    return self;
}

// 0x72C060 (thiscall): the destructor (the references released)
void __fastcall Destruct(Tagging* self, re::Edx) {
    for (int i = 39; i >= 0; i--) {
        TagRef_Set(&self->props[i], nullptr);
    }
    TagSound_Set(&self->sound, nullptr);
    TagRef_Set(&self->highlighted, nullptr);
}

// 0x72C0A0 (thiscall): a tag just finished (kept 4 seconds for the script)
void __fastcall AddFinished(Tagging* self, re::Edx, uint32_t userdata, uint32_t ped) {
    self->finished[self->nextFinished].userdata = userdata;
    self->finished[self->nextFinished].ped = ped;
    self->finished[self->nextFinished].timer = 4.0f;
    self->nextFinished++;
    if (self->nextFinished == 4) {
        self->nextFinished = 0;
    }
}

// 0x72C100 (thiscall): the first finished tag not taken yet
uint8_t __fastcall TakeFinished(Tagging* self, re::Edx, uint32_t* userdata, uint32_t* ped) {
    for (int i = 0; i < 4; i++) {
        TagFinished& f = self->finished[i];
        if (0.0f < f.timer) {
            *userdata = f.userdata;
            *ped = f.ped;
            f.timer = 0.0f;
            return 1;
        }
    }
    return 0;
}

// 0x72C140 (ecx unused): the dictionary slot of a tag set ("<prefix>C<set>")
int __stdcall SetTxdSlot(int prefix, int set) {
    TagSprintf(tagNameBuffer, "%sC%d", tagPrefixes[prefix], set);
    return TagFindTxdSlot(tagNameBuffer, 0);
}

// 0x72C180 (thiscall): the tag texture and the streamed dictionaries released
void __fastcall ReleaseStream(Tagging* self, re::Edx) {
    if (self->tagTexture != nullptr) {
        TagReleaseTexture(self->tagTexture);
        self->tagTexture = nullptr;
    }
    if (self->txd != -1) {
        TagRemoveModel(self->txd + kTxdModelBase);
        self->txd = -1;
    }
    if (self->nextTxd != -1) {
        TagRemoveModel(self->nextTxd + kTxdModelBase);
        self->nextTxd = -1;
    }
}

// 0x72C1F0 (thiscall): a tagged track's dictionary wanted this frame (a slot found or
// taken for it; none when all 16 are busy)
void __fastcall WantSlot(Tagging* self, re::Edx, void* track) {
    int txd = TagFindTxdSeasonal(uint32_t(*TagNode_GetId(track)), -1);
    int slot = 0;
    for (; slot < 16; slot++) {
        if (self->slots[slot].txdSlot == txd) {
            break;
        }
    }
    if (slot == 16) {
        for (slot = 0; slot < 16; slot++) {
            if (self->slots[slot].wanted == 0 && self->slots[slot].txdSlot == -1) {
                break;
            }
        }
        if (slot == 16) {
            return;
        }
        self->slots[slot].txdSlot = int16_t(txd);
    }
    self->slots[slot].wanted = 1;
    TagTrack_SetSlot(track, slot);
}

// 0x72C270 (ecx unused): a tag's width by the prop's model (TaggingButes)
float __stdcall Width(uint8_t* prop) {
    int model = At<int16_t>(prop, 0x10E);
    if (model == tagModelHighlight2) {
        return At<float>(tagButes, 0x18);
    }
    if (model == tagModelHighlight1) {
        return At<float>(tagButes, 0x20);
    }
    if (model == tagModelHighlight3) {
        return At<float>(tagButes, 0x28);
    }
    if (model == tagModelTv) {
        return 0.425f;
    }
    return At<float>(tagButes, 0x28);
}

// 0x72C2E0 (ecx unused): its height
float __stdcall Height(uint8_t* prop) {
    int model = At<int16_t>(prop, 0x10E);
    if (model == tagModelHighlight2) {
        return At<float>(tagButes, 0x1C);
    }
    if (model == tagModelHighlight1) {
        return At<float>(tagButes, 0x24);
    }
    if (model == tagModelHighlight3) {
        return At<float>(tagButes, 0x2C);
    }
    if (model == tagModelTv) {
        return 0.4f;
    }
    return At<float>(tagButes, 0x2C);
}

// 0x72C350 (node in eax, the trigger on the stack): the trigger's path, the node's keys
// from the end up through its parents (vf2 its key, vf13(-1) its parent)
void FillPath(void* node, TagTrigger* trigger) {
    int count = 0;
    uint32_t* key = &trigger->keys[15];
    while (node != nullptr) {
        *key = *vc::Call<uint32_t*>(node, 0x8);
        key--;
        count++;
        node = vc::Call<void*>(node, 0x34, -1);
    }
    trigger->numKeys = count;
}

// 0x72C390 (thiscall): the trigger of a tree
TagTrigger* __fastcall FindTrigger(Tagging* self, re::Edx, void* tree) {
    for (int i = 0; i < self->numTriggers; i++) {
        if (self->triggers[i].tree == tree) {
            return &self->triggers[i];
        }
    }
    return nullptr;
}

// 0x72C3E0 (thiscall, not in Ghidra's list): a trigger's constructor (its keys empty)
TagTrigger* __fastcall Trigger_Construct(TagTrigger* self, re::Edx) {
    for (int i = 0; i < 16; i++) {
        TagKey_Default(&self->keys[i]);
    }
    return self;
}

// 0x72C410 (thiscall): set up (on a new game): the glow and info textures, the mission
void __fastcall Init(Tagging* self, re::Edx) {
    self->nextFinished = 0;
    self->glowTxd = int16_t(TagFindTxdSlot("GlowL2", 0));
    self->frameTimer = 0.0f;
    self->glowTexture = nullptr;
    self->unknownC1 = 0;
    self->numProps = 0;
    self->unknown19C = 0;
    self->numTriggers = 0;
    self->triggers = nullptr;
    self->unknown19D = 0;
    self->users = 0;
    self->tagTexture = nullptr;
    self->prefix = -1;
    self->set = -1;
    self->frame = -1;
    self->txd = -1;
    self->nextTxd = -1;
    self->stream = 0;
    self->unknownC0 = 0;
    TagPushTxd();
    TagSetTxd(TagFindTxdSlot("EffectTextures", 0));
    TagPushMemoryTag(7);
    self->infoTexture = TagFindTexture("C_Info", nullptr);
    TagPopMemoryTag();
    TagPopTxd();
    uint32_t key;
    TagKey_Construct(&key, "3_S10");
    self->mission = TagMission_Find(tagMissionMgr, &key);
    TagKey_Destruct(&key);
}

// 0x72C510 (thiscall): one user less: with none left, the tag stream released
void __fastcall RemoveUser(Tagging* self, re::Edx) {
    if (--self->users != 0) {
        return;
    }
    Tagging_ReleaseStream(self);
    self->frameTimer = 0.0f;
    self->users = 0;
    self->tagTexture = nullptr;
    self->prefix = -1;
    self->set = -1;
    self->frame = -1;
    self->txd = -1;
    self->nextTxd = -1;
    self->stream = 0;
    self->unknownC0 = 0;
}

// 0x72C570 (thiscall): a prop being tagged: its music stream offered at the tag
void __fastcall UpdateProp(Tagging* self, re::Edx, uint8_t* prop) {
    TagProp_SetCurrent(prop);
    if (!TagCtl_IsPlaying(prop + 0x128, tagAnimTagging, 1, 1)) {
        return;
    }
    TagTransform node;
    node.scale = 1.0f;
    TagNodeTransform(prop, tagNodeHash, &node, 1);
    uint32_t stream = self->stream;
    if (stream == 0 || self->unknownC0 != 0) {
        return;
    }
    // (the flags' other bits are the game's uninitialised stack: zero here)
    TagEmitterOffer offer{};
    const float* pos = EntityPos(prop);
    offer.pos[0] = pos[0];
    offer.pos[1] = pos[1];
    offer.pos[2] = pos[2];
    offer.volume = 1.0f;
    offer.flags = uint8_t((offer.flags & 0xF9) | 8);
    offer.flags2 = (offer.flags2 & 0xFFFFFFF1) | 1;
    offer.stream = stream;
    offer.range1 = 200;
    offer.range2 = 200;
    TagEmitters_Offer(tagEmitters, &offer, CameraDistanceSq(node.t));
}

// 0x72C6B0 (stdcall; the caller's ecx unused): a textured quad (a, b, d, c) in one colour
void __stdcall DrawQuadAt(void* texture, const float* a, const float* b, const float* c, const float* d, uint32_t rgba) {
    uint16_t indices[6] = {2, 1, 0, 0, 2, 3};
    uint32_t argb = ((((rgba >> 24) << 8 | (rgba & 0xFF)) << 8 | ((rgba >> 8) & 0xFF)) << 8) | ((rgba >> 16) & 0xFF);
    const float* corners[4] = {a, b, d, c};
    const float uvs[4][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    TagVertex v[4];
    for (int i = 0; i < 4; i++) {
        v[i].pos[0] = corners[i][0];
        v[i].pos[1] = corners[i][1];
        v[i].pos[2] = corners[i][2];
        v[i].normal[0] = 0.0f;
        v[i].normal[1] = 0.0f;
        v[i].normal[2] = 1.0f;
        v[i].colour = argb;
        v[i].u = uvs[i][0];
        v[i].v = uvs[i][1];
    }
    TagSetRenderState(1, TagGetRaster(texture));
    TagIm3D_Transform(v, 4, nullptr, 0x19);
    TagIm3D_RenderIndexed(3, indices, 6, 0);
    TagIm3D_End();
}

// 0x72C840 (thiscall): a prop's trigger fired: its node path and ids kept (16 at most)
void __fastcall FireTrigger(Tagging* self, re::Edx, void* tree, void* node, uint32_t action, uint32_t nodeId) {
    TagTrigger* trigger = Tagging_FindTrigger(self, tree);
    if (trigger != nullptr) {
        FillPath(node, trigger);
        trigger->action = action;
        trigger->node = nodeId;
        return;
    }
    if (self->numTriggers < 16) {
        self->triggers[self->numTriggers].tree = tree;
        FillPath(node, &self->triggers[self->numTriggers]);
        self->triggers[self->numTriggers].action = action;
        self->triggers[self->numTriggers].node = nodeId;
        self->numTriggers++;
    }
}

// 0x72C8F0 (thiscall): the persistent tags' triggers made (new[] of 16)
void __fastcall StartPersistentTag(Tagging* self, re::Edx) {
    if (self->triggers != nullptr) {
        return;
    }
    int32_t* block = static_cast<int32_t*>(TagMalloc(0x504));
    if (block == nullptr) {
        self->triggers = nullptr;
        self->numTriggers = 0;
        return;
    }
    block[0] = 16;
    TagVectorConstruct(block + 1, sizeof(TagTrigger), 16, reinterpret_cast<void*>(kTriggerConstruct));
    self->triggers = reinterpret_cast<TagTrigger*>(block + 1);
    self->numTriggers = 0;
}

// 0x72C950 (thiscall): and deleted (delete[])
void __fastcall StopPersistentTag(Tagging* self, re::Edx) {
    TagTrigger* triggers = self->triggers;
    if (triggers != nullptr) {
        int32_t* block = reinterpret_cast<int32_t*>(triggers) - 1;
        for (int i = block[0] - 1; i >= 0; i--) {
            TagVectorDestruct(triggers[i].keys, 4, 16, reinterpret_cast<void*>(kKeyDestruct));
        }
        TagFree(block);
    }
    self->numTriggers = 0;
    self->triggers = nullptr;
}

// 0x72C9C0 (thiscall): a prop with a tag seen this frame (within 60 m, 40 at most); a
// TV not tagged plays its show
void __fastcall AddProp(Tagging* self, re::Edx, uint8_t* prop) {
    if (At<uint32_t>(tagModelInfos[At<int16_t>(prop, 0x10E)], 0x88) == 0) {
        return;
    }
    TagTransform* cam = TagWorldTransform(TagCameraNode(tagCamera));
    float camPos[3] = {cam->t[0], cam->t[1], cam->t[2]};
    const float* pos = EntityPos(prop);
    float dx = pos[0] - camPos[0];
    float dy = pos[1] - camPos[1];
    float dz = pos[2] - camPos[2];
    float distanceSq = dx * dx + dy * dy + dz * dz;
    if (!(distanceSq < 3600.0) || self->numProps >= 40) {
        return;
    }
    int index = self->numProps++;
    TagRef_Set(&self->props[index], prop);
    TagProp_SetCurrent(prop);
    uint8_t* ctl = prop + 0x128;
    if (At<int16_t>(prop, 0x10E) != tagModelTv || self->unknownC0 != 1) {
        return;
    }
    if (TagCtl_IsPlaying(ctl, tagAnimTagging, 1, 1)) {
        return;
    }
    At<uint32_t>(prop, 0x188) = 0;
    void* show = TagActionTree_Find("PlayShow", nullptr);
    if (show != nullptr) {
        TagCtl_PlayNode(ctl, vc::Call<void*>(show, 0x4C, At<uint32_t>(ctl, 0x18)));
    }
}

// 0x72CB30 (stdcall; the caller's ecx unused): a tag frame's texture
// ("<prefix>C<set>_<frame>") from a dictionary, created or just found
void* __stdcall TextureFor(int txd, int prefix, int set, int frame, int create) {
    if (txd == -1) {
        return nullptr;
    }
    void* dictionary = *PoolAt(tagTxdPool, txd);
    if (dictionary == nullptr) {
        return nullptr;
    }
    TagPushTxd();
    TagSetTxd(txd);
    TagSprintf(tagNameBuffer, "%sC%d_%.2d", tagPrefixes[prefix], set, frame);
    void* texture;
    if (re::Flag(create)) {
        texture = TagFindTexture(tagNameBuffer, nullptr);
    } else {
        texture = TagDictFindTexture(dictionary, tagNameBuffer);
    }
    TagPopTxd();
    return texture;
}

// 0x72CBE0 (thiscall): a tag drawn on a prop: a quad `width` either side of its x axis
// and `height` either side along z, faded by `fade`; mode 0 lit (colour scaled), 1 and 2
// alpha-blended (2 with the colour's own alpha), else the colour is the fade's bits
void __fastcall DrawTag(Tagging*, re::Edx, TagTransform* m, void* texture, uint32_t rgba, float height, float width, float fade,
                        int mode) {
    float side[3] = {m->m[0] * -1.0f * width, m->m[3] * -1.0f * width, m->m[6] * -1.0f * width};
    float up[3] = {m->m[2] * height, m->m[5] * height, m->m[8] * height};
    float top[3] = {up[0] + m->t[0], m->t[1] + up[1], m->t[2] + up[2]};
    float a[3] = {top[0] - side[0], top[1] - side[1], top[2] - side[2]};
    float b[3] = {top[0] + side[0], top[1] + side[1], top[2] + side[2]};
    float bottom[3] = {m->t[0] - up[0], m->t[1] - up[1], m->t[2] - up[2]};
    float c[3] = {bottom[0] - side[0], bottom[1] - side[1], bottom[2] - side[2]};
    float d[3] = {bottom[0] + side[0], bottom[1] + side[1], bottom[2] + side[2]};
    float f = float(float(fade * 0.6) + 0.4);
    uint8_t r, g, bl, alpha;
    if (mode == 0) {
        TagSetRenderState(10, 5);
        TagSetRenderState(11, 2);
        alpha = uint8_t(int(float(f * 255.0)));
        r = uint8_t(int(float(rgba & 0xFF) * f));
        g = uint8_t(int(float((rgba >> 8) & 0xFF) * f));
        bl = uint8_t(int(float((rgba >> 16) & 0xFF) * f));
    } else if (mode == 1) {
        TagSetRenderState(10, 5);
        TagSetRenderState(11, 6);
        alpha = uint8_t(int(float(f * 255.0)));
        r = uint8_t(rgba);
        g = uint8_t(rgba >> 8);
        bl = uint8_t(rgba >> 16);
    } else if (mode == 2) {
        TagSetRenderState(10, 5);
        TagSetRenderState(11, 6);
        alpha = uint8_t(rgba >> 24);
        r = uint8_t(rgba);
        g = uint8_t(rgba >> 8);
        bl = uint8_t(rgba >> 16);
    } else {
        // (the game's quirk: the colour is the bytes of the float f)
        uint32_t bits;
        static_assert(sizeof(bits) == sizeof(f));
        std::memcpy(&bits, &f, 4);
        r = uint8_t(bits);
        g = uint8_t(bits >> 8);
        bl = uint8_t(bits >> 16);
        alpha = uint8_t(bits >> 24);
    }
    // (the colour doubled, saturated)
    int r2 = r * 2 > 0xFF ? 0xFF : r * 2;
    int g2 = g * 2 > 0xFF ? 0xFF : g * 2;
    int b2 = bl * 2 > 0xFF ? 0xFF : bl * 2;
    uint32_t colour = uint32_t(r2) | uint32_t(g2) << 8 | uint32_t(b2) << 16 | uint32_t(alpha) << 24;
    uint32_t fog = uint32_t(int64_t(float(float(1.0f - fade) * 255.0)));
    if (fog > 0xFF) {
        fog = 0xFF;
    }
    TagSetRenderState(0x1E, int(fog & 0xFF));
    Tagging_DrawQuadAt(texture, a, b, c, d, colour);
}

// 0x72D0B0 (thiscall, this unused): the gang whose turf a point is on (1, 2, 4 or 5; a
// random one outside every turf, inside the carnival or indoors), never `not`
int32_t* __fastcall GangAt(Tagging*, re::Edx, int32_t* out, int32_t notThis, const float* pos) {
    uint8_t* carnival = FindNamedTrigger("ZoneCarnival");
    *out = 0xC;
    if (TagAreaIsExterior(tagCurrArea) == 1) {
        float point[2] = {pos[0], pos[1]};
        if (!TagTrigger_Contains2D(carnival, point)) {
            uint8_t* poor = FindNamedTrigger("ZonePoor");
            uint8_t* rich = FindNamedTrigger("ZoneRich");
            uint8_t* jocks = FindNamedTrigger("Jocks");
            uint8_t* football = FindNamedTrigger("Jocks_FootballField");
            uint8_t* nerds = FindNamedTrigger("Nerds");
            uint8_t* preps = FindNamedTrigger("Preps");
            uint8_t* greasers = FindNamedTrigger("Greasers");
            struct Turf {
                uint8_t* trigger;
                int32_t gang;
            };
            const Turf turfs[] = {{rich, 5}, {poor, 4}, {jocks, 2}, {football, 2}, {nerds, 1}, {preps, 5}, {greasers, 4}};
            for (const Turf& turf : turfs) {
                float p[2] = {pos[0], pos[1]};
                if (TagTrigger_Contains2D(turf.trigger, p)) {
                    *out = turf.gang;
                    goto found;
                }
            }
        }
    }
    if (*out == 0xC) {
        // rand() * 5 / 32767 - 1, clamped to 0..4 (as a float through _ftol: exact)
        int pick = TagRand() * 5 / 32767 - 1;
        if (pick < 0) {
            pick = 0;
        } else if (pick > 4) {
            pick = 4;
        }
        switch (pick) {
        case 1: *out = 2; break;
        case 2: *out = 4; break;
        case 3: *out = 5; break;
        default: *out = 1; break;
        }
    }
found:
    if (*out == notThis) {
        if (notThis == 1) {
            *out = 2;
        } else if (notThis == 2) {
            *out = 1;
        } else {
            *out = (notThis == 4) + 4;
        }
    }
    return out;
}

// 0x72D520 (thiscall): the tagged props drawn (tags, the TV's static, the bulletin
// board's light, the spots still to tag); `keep`: the props stay listed
void __fastcall Render(Tagging* self, re::Edx, int keep) {
    static const int kStates[6] = {0xC, 0x1D, 0x1E, 6, 8, 0x14};
    int saved[6];
    for (int i = 0; i < 6; i++) {
        TagGetRenderState(kStates[i], &saved[i]);
    }
    TagSetRenderState(0xC, 1);
    TagSetRenderState(0x1D, 5);
    TagSetRenderState(6, 1);
    TagSetRenderState(8, 0);
    TagSetRenderState(0x14, 1);
    uint8_t* record = TagMission_Record(tagMissionMgr, self->mission);
    uint8_t showSpots;
    if ((TagMissionSlot_Is(TagMission_Slot0(tagMissionMgr), self->mission) != 0 || At<uint16_t>(record, 2) != 0) &&
        tagWidescreen == 0) {
        showSpots = 1;
    } else {
        showSpots = 0;
    }
    TagTransform node;
    node.scale = 1.0f;   // (set once: later props keep what the last call left)
    for (int i = 0; i < self->numProps; i++) {
        uint8_t* prop = self->props[i];
        if (prop == nullptr || At<uint32_t>(prop, 0xA4) != 0) {
            continue;
        }
        TagNodeTransform(prop, tagNodeHash, &node, 1);
        float lift[3] = {float(node.m[1] * 0.03), float(node.m[4] * 0.03), float(node.m[7] * 0.03)};
        node.t[0] = lift[0] + node.t[0];
        node.t[1] = node.t[1] + lift[1];
        node.t[2] = node.t[2] + lift[2];
        TagProp_SetCurrent(prop);
        uint8_t* ctl = prop + 0x128;
        int model = At<int16_t>(prop, 0x10E);
        if (model == tagModelTv) {
            if (self->tagTexture != nullptr && TagCtl_IsPlaying(ctl, tagAnimTagging, 1, 1) == 1) {
                node.t[0] = float(node.t[0] - 0.025);
                node.t[1] = float(node.t[1] + 0.19);
                node.t[2] = float(node.t[2] + 0.8);
                uint8_t grey[4];
                uint32_t colour = *reinterpret_cast<uint32_t*>(TagRgba_Set(grey, 0x80, 0x80, 0x80, 0xFF));
                float width = Tagging_Width(prop);
                float height = Tagging_Height(prop);
                Tagging_DrawTag(self, &node, self->tagTexture, colour, height, width, 1.0f, 0);
            } else if (TagCtl_IsPlaying(ctl, tagAnimTagging, 1, 1)) {
                float pos[3] = {node.t[0], node.t[1], node.t[2]};
                TagSoundSlot_Play(&self->sound, TagAudio_GetSoundId(tagAudio, "TV_STATIC"), pos, 2, 0, 1, 0);
            }
        } else if (model == tagModelBulletinBoard) {
            if (!(tagWidescreen != 1 && tagWidescreen == tagWidescreenTarget) && tagIs169 == 0) {
                continue;
            }
            if (tagMenuPaused != 0) {
                continue;
            }
            float glow;
            if (self->unknown19D != 0) {
                // (the time as unsigned, through float; a pulse every 375 ms * 2 pi)
                float t = float(double(tagTimeInMs) / 375.0);
                float s = trk::CrtSin(t);
                glow = float(float(TagMin(s < 0 ? -s : s, 1.0f) * 0.575) + 0.125);
            } else {
                glow = 0.07875f;
            }
            // (the record's fourth floats are the game's uninitialised stack: zero here)
            TagLitRecord lit{};
            float right[3] = {node.m[0], node.m[3], node.m[6]};
            TagVector_Assign(lit.right, right);
            lit.unknown40 = 0;
            lit.up[0] = node.m[1];
            lit.up[1] = node.m[4];
            lit.up[2] = node.m[7];
            lit.back[0] = node.m[2] * -1.0f;
            lit.back[1] = node.m[5] * -1.0f;
            lit.back[2] = node.m[8] * -1.0f;
            float zero[3] = {node.m[1] * 0.0f, node.m[4] * 0.0f, node.m[7] * 0.0f};
            lit.pos[0] = zero[0] + node.t[0];
            lit.pos[1] = zero[1] + node.t[1];
            lit.pos[2] = zero[2] + node.t[2];
            uint8_t yellow[4];
            lit.colour = *reinterpret_cast<uint32_t*>(TagRgba_Set(yellow, 0xAA, 0xAA, 0x32, uint8_t(int(float(glow * 255.0)))));
            lit.unknown48 = 0.4f;
            lit.unknown4C = 20.0f;
            lit.texture = self->infoTexture;
            TagAddLitProp(&lit);
        } else if (TagCtl_IsPlaying(ctl, tagAnimSpraying, 1, 1)) {
            for (int t = 0; t < At<int32_t>(ctl, 0x34); t++) {
                void* track = TagCtl_GetActiveTrack(ctl, t);
                if (track == nullptr) {
                    continue;
                }
                int16_t taggedId = acttree::TypeIdAt<0x00C3C9D8, 0x00C3C9D4>();   // PropTaggedTrack
                if (!vc::Call<uint8_t>(track, 0, uint32_t(uint16_t(taggedId)))) {
                    continue;
                }
                int8_t slotIndex = TagTrack_GetSlot(track);
                if (slotIndex != -1) {
                    TagSlot& slot = self->slots[TagTrack_GetSlot(track)];
                    if (slot.txdSlot != -1) {
                        int32_t id = *TagNode_GetId(track);
                        if (TagTxdNameHash(slot.txdSlot) == id) {
                            slot.wanted = 1;
                            if (slot.texture != nullptr) {
                                int32_t kind = At<int32_t>(track, 0x1C);
                                float fade = At<float>(prop, 0x254);
                                uint32_t tmp;
                                uint32_t colour = *TagTrack_GetColour(track, &tmp);
                                float width = Tagging_Width(prop);
                                float height = Tagging_Height(prop);
                                Tagging_DrawTag(self, &node, slot.texture, colour, height, width, fade, kind == 1);
                            }
                            break;
                        }
                    }
                }
                Tagging_WantSlot(self, track);
                break;
            }
        } else if (showSpots == 1 && !re::Flag(keep) && !TagCtl_IsPlaying(ctl, tagAnimHidden, 1, 1) &&
                   (self->unknown19C == 0 || At<uint8_t>(At<uint8_t*>(prop, 0x23C), 0x3D) == 2)) {
            int alpha = (self->highlighted != nullptr && TagRef_Get(&self->highlighted) == prop) ? 0x80 : 0x40;
            uint8_t white[4];
            uint8_t colour[4];
            TagRgba_Copy(colour, TagRgba_Set(white, 0xFF, 0xFF, 0xFF, alpha));
            Tagging_DrawTag(self, &node, self->glowTexture, *reinterpret_cast<uint32_t*>(colour), 0.65f, 0.65f, 1.0f, 2);
        }
    }
    if (!re::Flag(keep)) {
        self->numProps = 0;
    }
    for (int i = 0; i < 6; i++) {
        TagSetRenderState(kStates[i], saved[i]);
    }
}

// 0x72DD20 (thiscall): the next frame of the tag animation: the next frame of this set,
// else the next set's first, else nothing (0, 0, 0)
void __fastcall NextFrame(Tagging* self, re::Edx, int txd, int, int* prefix, int* set, int* frame) {
    if (self->prefix != -1 && self->set != -1 && self->frame != -1) {
        int next = self->frame + 1;
        if (TagTextureFor(txd, self->prefix, self->set, next, 0) != nullptr) {
            *prefix = self->prefix;
            *set = self->set;
            *frame = next;
            return;
        }
        int nextSet = self->set + 1;
        if (TagSetTxdSlot(self->prefix, nextSet) != -1) {
            *prefix = self->prefix;
            *set = nextSet;
            *frame = 0;
            return;
        }
    }
    *prefix = 0;
    *set = 0;
    *frame = 0;
}

// 0x72DDF0 (thiscall): the tag animation stepped every 10 ticks, its next dictionary
// streamed and its music started with it
void __fastcall UpdateStream(Tagging* self, re::Edx) {
    if (self->unknownC0 == 0 && (tagMusicFading == 0 || tagMusicStream != self->stream)) {
        if (self->tagTexture != nullptr) {
            TagReleaseTexture(self->tagTexture);
            self->tagTexture = nullptr;
        }
        self->frameTimer = 0.0f;
    }
    if (0.0f < self->frameTimer) {
        float step = tagFadeRate * tagTimeStep;
        self->frameTimer = self->frameTimer - step;
        return;
    }
    uint8_t ready = 0;
    int prefix, set, frame;
    Tagging_NextFrame(self, self->txd, self->nextTxd, &prefix, &set, &frame);
    if ((tagMusicPreloaded == self->stream && tagMusicStream == self->stream && TagMusic_IsPreloaded(tagMusic)) ||
        self->unknownC0 != 0) {
        ready = 1;
    }
    if (prefix == self->prefix && set == self->set) {
        if (self->tagTexture != nullptr) {
            TagReleaseTexture(self->tagTexture);
        }
        self->frame = frame;
        self->tagTexture = TagTextureFor(self->txd, prefix, set, frame, 1);
        self->frameTimer = 10.0f;
        return;
    }
    if (self->nextTxd == -1) {
        TagSprintf(tagNameBuffer, "%sC%d", tagPrefixes[prefix], set);
        self->nextTxd = TagFindTxdSlot(tagNameBuffer, 0);
        TagSprintf(tagNameBuffer, "%sC%d.rsm", tagPrefixes[prefix], set + 1);
        self->stream = TagHashStreamName(tagNameBuffer, 1);
    }
    int id = self->nextTxd + kTxdModelBase;
    if (TagStreamingInfo(id)[4] == 1 && ready) {
        if (self->tagTexture != nullptr) {
            TagReleaseTexture(self->tagTexture);
        }
        self->tagTexture = TagTextureFor(self->nextTxd, prefix, set, frame, 1);
        if (self->unknownC0 == 0) {
            TagMusic_PlayPreloaded(tagMusic, 2);
        }
        if (self->txd != -1) {
            TagRemoveModel(self->txd + kTxdModelBase);
        }
        self->frameTimer = 10.0f;
        self->txd = self->nextTxd;
        self->nextTxd = -1;
        self->prefix = prefix;
        self->set = set;
        self->frame = frame;
        return;
    }
    if (TagStreamingInfo(id)[4] == 3) {
        return;
    }
    if (TagStreamingInfo(id)[4] == 4) {
        return;
    }
    TagRequestModel(id, 1);
}

// a slot's texture taken from its loaded dictionary, else the dictionary requested
void TakeOrRequest(int16_t txdSlot, re::Ptr32<void>* texture, bool checkNone) {
    int slot = txdSlot;
    void* dictionary = *PoolAt(tagTxdPool, slot);
    if (dictionary != nullptr) {
        TagPushTxd();
        TagSetTxd(txdSlot);
        TagForAllTextures(dictionary, reinterpret_cast<void*>(kTakeTextureCB), texture);
        TagPopTxd();
        return;
    }
    if (checkNone && txdSlot == -1) {
        return;
    }
    if (!IsStreaming(slot + kTxdModelBase)) {
        TagRequestModel(txdSlot + kTxdModelBase, 0xD);
    }
}

// 0x72E090 (thiscall): every frame: the tag stream, the glow texture, the finished tags'
// timers, the tag slots (textures taken when wanted, released when not)
void __fastcall Update(Tagging* self, re::Edx) {
    if (self->users > 0) {
        Tagging_UpdateStream(self);
    }
    if (self->glowTexture == nullptr) {
        TakeOrRequest(self->glowTxd, &self->glowTexture, false);
    }
    float rate = tagFadeRate;
    for (int i = 0; i < 4; i++) {
        float& timer = self->finished[i].timer;
        if (0.0f < timer) {
            float step = rate * tagTimeStep;
            timer = timer - step;
            if (timer < 0.0f) {
                timer = 0.0f;
            }
        }
    }
    self->unknownC1 = 0;
    for (int i = 0; i < 16; i++) {
        TagSlot& slot = self->slots[i];
        if (slot.wanted == 0) {
            if (slot.texture != nullptr) {
                TagReleaseTexture(slot.texture);
                slot.texture = nullptr;
                slot.txdSlot = -1;
            }
        } else if (slot.texture == nullptr) {
            self->unknownC1 = 1;
            TakeOrRequest(slot.txdSlot, &slot.texture, true);
        }
        slot.wanted = 0;
    }
}

} // namespace tagging

RE_UNHOOKED("Tagging::FillPath", 0x0072C350);   // (the node in eax)

#define TAGGING_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TAGGING_REVERSED("Tagging::Construct", 0x0072BEF0, &tagging::Construct);
TAGGING_REVERSED("Tagging::Destruct", 0x0072C060, &tagging::Destruct);
TAGGING_REVERSED("Tagging::AddFinished", 0x0072C0A0, &tagging::AddFinished);
TAGGING_REVERSED("Tagging::TakeFinished", 0x0072C100, &tagging::TakeFinished);
TAGGING_REVERSED("Tagging::SetTxdSlot", 0x0072C140, &tagging::SetTxdSlot);
TAGGING_REVERSED("Tagging::ReleaseStream", 0x0072C180, &tagging::ReleaseStream);
TAGGING_REVERSED("Tagging::WantSlot", 0x0072C1F0, &tagging::WantSlot);
TAGGING_REVERSED("Tagging::Width", 0x0072C270, &tagging::Width);
TAGGING_REVERSED("Tagging::Height", 0x0072C2E0, &tagging::Height);
TAGGING_REVERSED("Tagging::FindTrigger", 0x0072C390, &tagging::FindTrigger);
TAGGING_REVERSED("TagTrigger::Construct", 0x0072C3E0, &tagging::Trigger_Construct);
TAGGING_REVERSED("Tagging::Init", 0x0072C410, &tagging::Init);
TAGGING_REVERSED("Tagging::RemoveUser", 0x0072C510, &tagging::RemoveUser);
TAGGING_REVERSED("Tagging::UpdateProp", 0x0072C570, &tagging::UpdateProp);
TAGGING_REVERSED("Tagging::DrawQuadAt", 0x0072C6B0, &tagging::DrawQuadAt);
TAGGING_REVERSED("Tagging::FireTrigger", 0x0072C840, &tagging::FireTrigger);
TAGGING_REVERSED("Tagging::StartPersistentTag", 0x0072C8F0, &tagging::StartPersistentTag);
TAGGING_REVERSED("Tagging::StopPersistentTag", 0x0072C950, &tagging::StopPersistentTag);
TAGGING_REVERSED("Tagging::AddProp", 0x0072C9C0, &tagging::AddProp);
TAGGING_REVERSED("Tagging::TextureFor", 0x0072CB30, &tagging::TextureFor);
TAGGING_REVERSED("Tagging::DrawTag", 0x0072CBE0, &tagging::DrawTag);
TAGGING_REVERSED("Tagging::GangAt", 0x0072D0B0, &tagging::GangAt);
TAGGING_REVERSED("Tagging::Render", 0x0072D520, &tagging::Render);
TAGGING_REVERSED("Tagging::NextFrame", 0x0072DD20, &tagging::NextFrame);
TAGGING_REVERSED("Tagging::UpdateStream", 0x0072DDF0, &tagging::UpdateStream);
TAGGING_REVERSED("Tagging::Update", 0x0072E090, &tagging::Update);
