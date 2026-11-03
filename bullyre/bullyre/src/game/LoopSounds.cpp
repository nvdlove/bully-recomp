// The looping sounds attached to entities or points (0xCF4274: the elevator, the ghost,
// the vehicles' loops...) and the rats' squeaks (0xCF427C), with the sounds' per-frame
// callbacks that set their voices' volume from the distance, 0x592390..0x593660.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct CVector {
    float x, y, z;
};

// a loop (0x24 bytes)
struct LoopSound {
    re::Ptr32<uint8_t> sound;      // +0 (set with Sound_Set; +0x5C: stop it)
    re::Ptr32<uint8_t> entity;     // +4 a reference: plays on it
    re::Ptr32<uint8_t> entity2;    // +8 a reference: plays at its position (or its first bone's)
    re::Ptr32<uint8_t> ped;        // +0xC a reference: the ped it's for (its +0x143C set meanwhile)
    re::Ptr32<uint8_t> vehicle;    // +0x10 a reference: the ped's vehicle then (+0x155C)
    int32_t slot;        // +0x14 (-1 free)
    CVector pos;         // +0x18
};
static_assert(sizeof(LoopSound) == 0x24);

// a rat's squeak (0x1C bytes)
struct RatSound {
    re::Ptr32<uint8_t> entity;     // +0 a registered reference
    re::Ptr32<uint8_t> sound;      // +4
    int32_t slot;        // +8 (-1 free)
    float unknownC;      // +0xC
    float unknown10;     // +0x10
    uint8_t unknown14[8];
};
static_assert(sizeof(RatSound) == 0x1C);

// the lists (the same shape for both)
template <typename T>
struct SoundList {
    re::Ptr32<T> items[10];        // +0
    int32_t freeHint;    // +0x28 the slot freed last (-1 none)
    int32_t count;       // +0x2C
    int32_t unknown30;   // +0x30
};
static_assert(offsetof(SoundList<LoopSound>, count) == 0x2C);

// a sound's voices (a vector: begin, end, count)
struct VoiceList {
    re::Ptr32<int32_t> voices;
    int32_t unknown4;
    uint32_t count;
};

inline void* const theSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const propQueries = reinterpret_cast<void*>(0x00D13730);
GAME_VAR(SoundList<LoopSound>*, loopSounds, 0x00CF4274);
GAME_VAR(SoundList<RatSound>*, ratSounds, 0x00CF427C);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(float, soundTimeStep, 0x00CF60F0);
GAME_VAR(void*, theCamera, 0x00C3CCF4);
GAME_ARRAY(int32_t, ratVolumes, 0x00AE6328);   // by the callback's first argument (0..10)

GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, void* soundRef, void* sound);
GAME_FN(void*, __thiscall, SoundRef_Assign, 0x0042D6D0, void* soundRef, void* sound);
GAME_FN(uint8_t, __thiscall, Sound_IsActive, 0x0059CF10, void* sound);
GAME_FN(void, __thiscall, Sound_GetVoices, 0x0059CF20, void* sound, VoiceList* out);
GAME_FN(uint8_t, __cdecl, Audio_Approach, 0x0059CF30, int32_t* value, int target, int step);
GAME_FN(uint8_t, __cdecl, Voice_Fn5AF530, 0x005AF530, int voice);
GAME_FN(void, __cdecl, Voice_Fn5AF870, 0x005AF870, int voice, int volume, float);
GAME_FN(void, __cdecl, Voice_Fn5AF7B0, 0x005AF7B0, int voice, int volume, int, int);
GAME_FN(void, __cdecl, Voice_Fn5AF660, 0x005AF660, int voice, float);
GAME_FN(float, __cdecl, Audio_Fn5A7F00, 0x005A7F00, float, float);
GAME_FN(float, __cdecl, Audio_Fn59E4F0, 0x0059E4F0, float value, int);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int a3,
        const CVector* pos2, int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12,
        float a13, int a14);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(uint8_t, __thiscall, PropQueries_Has, 0x005EBC70, void* queries, void* entity);
GAME_FN(void, __thiscall, PropQueries_GetPos, 0x005EBDC0, void* queries, void* entity, CVector* out);
GAME_FN(void*, __cdecl, Camera_Object, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, Node_WorldTransform, 0x006C9A90, void* node);   // (+0x24 the position)
GAME_FN(int, __cdecl, BoneTable_Count, 0x005191C0);     // (5)
GAME_FN(int, __cdecl, BoneTable_Get, 0x00520380, int index);
GAME_FN(uint8_t, __cdecl, GetBoneTransform, 0x00467D80, void* entity, int bone, void* out, int flag);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
inline int __cdecl Rand() { return crt::Rand(); }
// its own (called where modes apply)
GAME_FN(void, __thiscall, LoopSounds_StopAll, 0x00592390, void* list);
GAME_FN(void, __thiscall, LoopSounds_Stop, 0x00592400, void* list, void* entity, void* entity2, int id);
GAME_FN(uint8_t, __thiscall, LoopSounds_Has, 0x005925A0, void* list, void* entity, void* entity2, int id);
GAME_FN(void, __thiscall, LoopSound_Update, 0x005926B0, void* loop);
GAME_FN(void, __thiscall, LoopSound_Start, 0x00592CF0, void* loop, void* entity, void* entity2, const CVector* pos,
        int slot, int soundId, int a6, void* ped);
GAME_FN(void*, __thiscall, RatSound_Construct, 0x00593100, void* rat);
GAME_FN(void, __thiscall, RatSound_Destruct, 0x00593080, void* rat);
GAME_FN(void, __thiscall, RatSound_Stop, 0x00593150, void* rat);
GAME_FN(void, __thiscall, RatSound_StopIfGone, 0x005931B0, void* rat);
GAME_FN(void, __thiscall, RatSound_Start, 0x005933C0, void* rat, void* entity, int slot);
GAME_FN(void, __thiscall, RatSounds_StopAll, 0x00593510, void* list);
GAME_FN(void, __thiscall, RatSounds_Remove, 0x005935A0, void* list, void* entity);

const CVector* PositionOf(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : entity + 4);
}

// a sound still playing, or its vf 0x50
bool SoundBusy(uint8_t* sound) {
    return Sound_IsActive(sound) != 0 || vc::Call<uint8_t>(sound, 0x50) != 0;
}

// (inlined everywhere) a loop freed: its references dropped, its sound told to stop
void Release(LoopSound* loop) {
    SetRef(&loop->entity, nullptr);
    SetRef(&loop->entity2, nullptr);
    if (loop->sound != nullptr && SoundBusy(loop->sound)) {
        loop->sound[0x5C] = 1;
    }
    Sound_Set(&loop->sound, nullptr);
    loop->slot = -1;
}

// the length of a vector (the game's order of the sum)
float Length(const CVector& v) {
    float sq = std::fabs(v.x * v.x + v.y * v.y + v.z * v.z);
    return std::sqrt(sq);
}

// what the callbacks share: each voice of the sound at a volume (and its pitch from p2)
void SetVoices(uint8_t* sound, int volume, float pitch) {
    VoiceList list{nullptr, 0, 0};
    Sound_GetVoices(sound, &list);
    for (uint32_t i = 0; i < list.count; i++) {
        if (list.voices[i] == -1) {
            continue;
        }
        int arg = vc::Call<int>(sound, 0xB8);
        if (Voice_Fn5AF530(list.voices[i]) != 0) {
            Voice_Fn5AF870(list.voices[i], volume, 0.0f);
        } else {
            Voice_Fn5AF7B0(list.voices[i], volume, arg, 1);
        }
        Voice_Fn5AF660(list.voices[i], Audio_Fn5A7F00(pitch, soundTimeStep));
    }
    if (list.voices != nullptr) {
        Free(list.voices);
    }
}

// ---- the loops ---------------------------------------------------------------------------------

// 0x592390 (thiscall)
void __fastcall LoopSounds_StopAllImpl(SoundList<LoopSound>* self) {
    for (int i = 0; i < self->count; i++) {
        LoopSound* loop = self->items[i];
        if (loop != nullptr) {
            Release(loop);
        }
    }
}

// 0x592400 (thiscall): the first loop of that sound id on the entity (or at the position
// of entity2; neither: a free-standing one)
void __fastcall LoopSounds_StopImpl(SoundList<LoopSound>* self, re::Edx, uint8_t* entity, uint8_t* entity2, int id) {
    for (int i = 0; i < self->count; i++) {
        LoopSound* loop = self->items[i];
        uint8_t* sound = loop->sound;
        if (sound == nullptr) {
            continue;
        }
        bool found;
        if (entity != nullptr) {
            found = vc::Call<int>(sound, 0x58) == id && loop->entity == entity;
        } else if (entity2 != nullptr) {
            found = vc::Call<int>(sound, 0x58) == id && loop->entity2 == entity2;
        } else {
            found = loop->entity2 == nullptr && loop->entity == nullptr && vc::Call<int>(loop->sound, 0x58) == id;
        }
        if (found) {
            Release(loop);
            self->freeHint = i;
            return;
        }
    }
}

// 0x5924E0 (thiscall): the ped's loops stopped (with keepInVehicle: not those it started
// in the vehicle it's still in); none left: its +0x143C cleared
void __fastcall LoopSounds_StopForPed(SoundList<LoopSound>* self, re::Edx, uint8_t* ped, int keepInVehicle) {
    uint8_t cleared = 1;
    if (ped == nullptr) {
        return;
    }
    for (int i = 0; i < self->count; i++) {
        LoopSound* loop = self->items[i];
        if (loop->ped != ped) {
            continue;
        }
        if (re::Flag(keepInVehicle) && At<uint8_t*>(ped, 0x155C) == loop->vehicle) {
            cleared = 0;
            continue;
        }
        Release(loop);
        self->freeHint = i;
    }
    if (cleared) {
        At<int32_t>(ped, 0x143C) = 0;
    }
}

// 0x5925A0 (thiscall): a loop of that id on the entity (or entity2) plays
uint8_t __fastcall LoopSounds_HasImpl(SoundList<LoopSound>* self, re::Edx, uint8_t* entity, uint8_t* entity2, int id) {
    for (int i = 0; i < self->count; i++) {
        LoopSound* loop = self->items[i];
        uint8_t* onEntity = loop->entity;
        uint8_t* atEntity = loop->entity2;
        if (loop->sound == nullptr) {
            continue;
        }
        int soundId = vc::Call<int>(loop->sound, 0x58);
        if (onEntity != nullptr) {
            if (entity != onEntity) {
                continue;
            }
        } else if (atEntity != nullptr) {
            if (entity2 != atEntity) {
                continue;
            }
        }
        if (id == soundId) {
            return 1;
        }
    }
    return 0;
}

// 0x592600 (thiscall): the list made with `count` loops
SoundList<LoopSound>* __fastcall LoopSounds_Init(SoundList<LoopSound>* self, re::Edx, int count, int32_t value) {
    self->count = count;
    self->unknown30 = value;
    if (count <= 0) {
        self->freeHint = 0;
        return self;
    }
    for (int i = 0; i < count; i++) {
        LoopSound* loop = static_cast<LoopSound*>(Malloc(sizeof(LoopSound)));
        if (loop != nullptr) {
            loop->sound = nullptr;
            loop->pos.x = 0.0f;
            loop->entity = nullptr;
            loop->pos.y = 0.0f;
            loop->pos.z = 0.0f;
            loop->entity2 = nullptr;
            loop->ped = nullptr;
            loop->vehicle = nullptr;
            Sound_Set(&loop->sound, nullptr);
            SetRef(&loop->entity, nullptr);
            SetRef(&loop->entity2, nullptr);
            loop->slot = -1;
        }
        self->items[i] = loop;
    }
    self->freeHint = 0;
    return self;
}

// 0x5926B0 (thiscall): a loop stopped when its entity is gone (a dead ped's, the player's
// vehicle's) or 30.3 m from the camera
void __fastcall LoopSound_UpdateImpl(LoopSound* self) {
    CVector camera;
    GetCameraPos(&camera);
    uint8_t* entity = self->entity;
    if (entity == nullptr) {
        entity = self->entity2;
        if (entity == nullptr) {
            return;
        }
    }
    CVector delta;
    float distance = Length(*VectorSub(&delta, PositionOf(entity), &camera));
    uint8_t* on = self->entity;
    if (on != nullptr) {
        if ((on[0x108] & 7) == 3 && At<uint8_t>(self->entity, 0x1C98) != 0) {
            uint8_t* dead = self->entity;
            int id = vc::Call<int>(self->sound, 0x58);
            LoopSounds_Stop(loopSounds, dead, nullptr, id);
            return;
        }
        on = self->entity;
        if (on == At<uint8_t*>(playerPed, 0x155C)) {
            int id = vc::Call<int>(self->sound, 0x58);
            LoopSounds_Stop(loopSounds, on, nullptr, id);
            return;
        }
    }
    if (double(distance) > double(30.3f) && self->sound != nullptr) {
        uint8_t* at = self->entity2;
        int id = vc::Call<int>(self->sound, 0x58);
        LoopSounds_Stop(loopSounds, on, at, id);
    }
}

// 0x592810 (thiscall): everything stopped and freed
void __fastcall LoopSounds_Shutdown(SoundList<LoopSound>* self) {
    LoopSounds_StopAll(self);
    for (int i = 0; i < self->count; i++) {
        LoopSound* loop = self->items[i];
        if (loop != nullptr) {
            SetRef(&loop->vehicle, nullptr);
            SetRef(&loop->ped, nullptr);
            SetRef(&loop->entity2, nullptr);
            SetRef(&loop->entity, nullptr);
            Sound_Set(&loop->sound, nullptr);
            Free(loop);
        }
        self->items[i] = nullptr;
    }
}

// 0x592880 (thiscall)
void __fastcall LoopSounds_Update(SoundList<LoopSound>* self) {
    for (int i = 0; i < self->count; i++) {
        LoopSound* loop = self->items[i];
        if (loop == nullptr || (loop->entity == nullptr && loop->entity2 == nullptr)) {
            continue;
        }
        if (loop->sound == nullptr) {
            Release(loop);
        }
        LoopSound_Update(loop);
    }
}

// 0x592910 (cdecl, a sound's callback): the loop at entity2 (its position from the prop
// queries when they have it) at the volume the sound gives for its distance
void __cdecl LoopSound_AtEntityCallback(float, float pitch, uint8_t* entity, uint8_t* sound) {
    if (sound == nullptr) {
        return;
    }
    if (entity != nullptr) {
        CVector pos{0.0f, 0.0f, 0.0f};
        if (PropQueries_Has(propQueries, entity) != 0) {
            PropQueries_GetPos(propQueries, entity, &pos);
            vc::Call<void>(sound, 0x84, &pos);
        } else {
            pos = *PositionOf(entity);   // (unused)
        }
        float distance = vc::Call<float>(sound, 0xB0);
        float volume = float(vc::Call<int>(sound, 0xB4, int32_t(distance)));
        if (0.0f < volume) {
            SetVoices(sound, int32_t(volume), pitch);
            return;
        }
    }
    sound[0x5C] = 1;
}

// 0x592AE0 (cdecl, a sound's callback; missing from the function lists): the same with
// the distance from the camera
void __cdecl LoopSound_OnEntityCallback(float, float pitch, uint8_t* entity, uint8_t* sound) {
    if (entity == nullptr) {
        if (sound != nullptr) {
            sound[0x5C] = 1;
        }
        return;
    }
    if (sound == nullptr) {
        return;
    }
    CVector pos = *PositionOf(entity);
    const float* cameraPos = reinterpret_cast<const float*>(Node_WorldTransform(Camera_Object(theCamera)) + 0x24);
    CVector camera{cameraPos[0], cameraPos[1], cameraPos[2]};
    CVector delta;
    VectorSub(&delta, &pos, &camera);
    float distance = Length(delta);
    float volume = float(vc::Call<int>(sound, 0xB4, int32_t(distance)));
    if (!(0.0f < volume)) {
        sound[0x5C] = 1;
        return;
    }
    SetVoices(sound, int32_t(volume), pitch);
}

// 0x592CF0 (thiscall): start a loop in its slot: on `entity`, at `entity2` (its first
// bone, half a metre up), or at `pos`; the ped it's for noted with its vehicle
void __fastcall LoopSound_StartImpl(LoopSound* self, re::Edx, uint8_t* entity, uint8_t* entity2, const CVector* pos,
                                    int slot, int soundId, int a6, uint8_t* ped) {
    if (slot <= -1) {
        return;
    }
    self->slot = slot;
    SetRef(&self->entity, entity);
    SetRef(&self->entity2, entity2);
    SetRef(&self->ped, ped);
    if (self->ped != nullptr) {
        At<int32_t>(self->ped, 0x143C) = 1;
        SetRef(&self->vehicle, At<uint8_t*>(self->ped, 0x155C));
    }
    if (self->entity != nullptr) {
        uint8_t* on = self->entity;
        const CVector* at = PositionOf(on);
        void* sound = Sounds_PlayAt(theSounds, soundId, at, a6, at, 0, 0, 0, 0,
                                    reinterpret_cast<void*>(0x00592AE0), on, 6, 1, 1.0f, 0);
        Sound_Set(&self->sound, sound);
        if (Sounds_GetId(theSounds, "ChemElevator") == soundId) {
            vc::Call<void>(self->sound, 0x98, 0);
            vc::Call<void>(self->sound, 0xA0, 0);
        }
        return;
    }
    if (self->entity2 == nullptr) {
        void* sound = Sounds_PlayAt(theSounds, soundId, pos, 1, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 6, 1,
                                    1.0f, 0);
        Sound_Set(&self->sound, sound);
        return;
    }
    // (an NiTransform: rotation, translation, scale)
    struct {
        float rotation[9];
        CVector translation;
        float scale;
    } transform;
    transform.scale = 1.0f;
    bool placed = false;
    if (BoneTable_Count() != 0) {
        if (GetBoneTransform(entity2, BoneTable_Get(0), &transform, 1) != 0) {
            transform.translation.z = float(double(transform.translation.z) + 0.5);
            self->pos = transform.translation;
            placed = true;
        }
    }
    if (!placed) {
        self->pos = *PositionOf(self->entity2);
    }
    uint8_t* at = self->entity2;
    void* sound = Sounds_PlayAt(theSounds, soundId, &self->pos, a6, &self->pos, 0, 0, 0, 0,
                                reinterpret_cast<void*>(0x00592910), at, 6, 1, 1.0f, 0);
    Sound_Set(&self->sound, sound);
    if (self->sound != nullptr && Sounds_GetId(theSounds, "Ghost") == soundId) {
        vc::Call<void>(self->sound, 0x98, 0);
    }
}

// the free slot the starts take: the last freed, else the first without a sound
int FreeLoopSlot(SoundList<LoopSound>* self) {
    int slot = self->freeHint;
    if (slot > -1) {
        self->freeHint = -1;
        return slot;
    }
    for (int i = 0; i < self->count; i++) {
        LoopSound* loop = self->items[i];
        if (loop != nullptr && loop->sound == nullptr) {
            return i;
        }
    }
    return -1;
}

// 0x592F30 (thiscall): a loop on an entity (unless it has that one already)
void __fastcall LoopSounds_PlayOn(SoundList<LoopSound>* self, re::Edx, uint8_t* entity, int soundId, int a6,
                                  uint8_t* ped) {
    if (LoopSounds_Has(self, entity, nullptr, soundId) != 0) {
        return;
    }
    int slot = FreeLoopSlot(self);
    if (slot > -1) {
        LoopSound_Start(self->items[slot], entity, nullptr, nullptr, slot, soundId, a6, ped);
    }
}

// 0x592FA0 (thiscall): a loop at an entity's position
void __fastcall LoopSounds_PlayAtEntity(SoundList<LoopSound>* self, re::Edx, uint8_t* entity2, int soundId, int a6,
                                        uint8_t* ped) {
    if (LoopSounds_Has(self, nullptr, entity2, soundId) != 0) {
        return;
    }
    int slot = FreeLoopSlot(self);
    if (slot > -1) {
        LoopSound_Start(self->items[slot], nullptr, entity2, nullptr, slot, soundId, a6, ped);
    }
}

// 0x593010 (thiscall): a loop at a point
void __fastcall LoopSounds_PlayAt(SoundList<LoopSound>* self, re::Edx, int soundId, const CVector* pos, int a6,
                                  uint8_t* ped) {
    if (LoopSounds_Has(self, nullptr, nullptr, soundId) != 0) {
        return;
    }
    int slot = FreeLoopSlot(self);
    if (slot > -1) {
        LoopSound_Start(self->items[slot], nullptr, nullptr, pos, slot, soundId, a6, ped);
    }
}

// ---- the rats ------------------------------------------------------------------------------------

// 0x593080 (thiscall): the destructor
void __fastcall RatSound_DestructImpl(RatSound* self) {
    Sound_Set(&self->sound, nullptr);
    if (self->entity != nullptr) {
        CleanUpOldReference(self->entity, &self->entity);
    }
}

// 0x5930A0 (cdecl): a rat died: its squeak and its loop gone
void __cdecl RatSound_Die(uint8_t* rat) {
    const CVector* pos = PositionOf(rat);
    Sounds_PlayAt(theSounds, Sounds_GetId(theSounds, "RatDie"), pos, 0, pos, 0, 0, 0, 0, nullptr, nullptr, 6, 1,
                  1.0f, 0);
    RatSounds_Remove(ratSounds, rat);
}

// 0x593100 (thiscall): the constructor
RatSound* __fastcall RatSound_ConstructImpl(RatSound* self) {
    self->entity = nullptr;
    self->sound = nullptr;
    if (self->entity != nullptr) {
        CleanUpOldReference(self->entity, &self->entity);
        self->entity = nullptr;
    }
    Sound_Set(&self->sound, nullptr);
    self->unknownC = -1.0f;
    self->unknown10 = -1.0f;
    self->slot = -1;
    return self;
}

// 0x593150 (thiscall)
void __fastcall RatSound_StopImpl(RatSound* self) {
    if (self->sound != nullptr && SoundBusy(self->sound)) {
        self->sound[0x5C] = 1;
    }
    Sound_Set(&self->sound, nullptr);
    if (self->entity != nullptr) {
        CleanUpOldReference(self->entity, &self->entity);
        self->entity = nullptr;
    }
    self->unknownC = -1.0f;
    self->slot = -1;
    self->unknown10 = -1.0f;
}

// 0x5931B0 (thiscall): stopped when its entity isn't a ped any more
void __fastcall RatSound_StopIfGoneImpl(RatSound* self) {
    if (self->entity != nullptr && (self->entity[0x108] & 7) != 3) {
        RatSound_Stop(self);
    }
}

// 0x5931D0 (cdecl, the squeak's callback): louder the faster the rat runs (its step
// from the table by `index`, 11 and on: stop)
void __cdecl RatSound_Callback(float index, float pitch, uint8_t* rat, uint8_t* sound) {
    if (rat == nullptr || (rat[0x108] & 7) != 3 || sound == nullptr) {
        return;
    }
    int32_t i = int32_t(index);
    float volume = 0.0f;
    if (i > 10) {
        sound[0x5C] = 1;
    } else {
        volume = float(double(ratVolumes[i]) * 0.8);
    }
    float vx = At<float>(rat, 0x124);
    float vy = At<float>(rat, 0x128);
    float speed = std::sqrt(std::fabs(vx * vx + vy * vy));
    if (speed == 0.0f) {
        volume = 0.0f;
    }
    volume = Audio_Fn59E4F0(volume, 4);
    int32_t current = vc::Call<int32_t>(sound, 0x10);
    if (Audio_Approach(&current, int32_t(volume), 0x20) != 0) {
        vc::Call<void>(sound, 0x14, current);
    }
    SetVoices(sound, current, float(double(pitch) * 3.5));
}

// 0x5933C0 (thiscall): a squeak (one of four at random) on the rat
void __fastcall RatSound_StartImpl(RatSound* self, re::Edx, uint8_t* rat, int slot) {
    if (rat == nullptr || slot <= -1) {
        return;
    }
    self->slot = slot;
    if (rat != self->entity) {
        if (self->entity != nullptr) {
            CleanUpOldReference(self->entity, &self->entity);
        }
        self->entity = rat;
        RegisterReference(rat, &self->entity);
    }
    const CVector* pos = PositionOf(self->entity);
    const char* name;
    switch ((Rand() * 2) / 0x7FFF) {
    case 0: name = "Rat1"; break;
    case 1: name = "Rat2"; break;
    case 2: name = "Rat3"; break;
    case 3: name = "Rat4"; break;
    default: return;
    }
    uint8_t* on = self->entity;
    void* sound = Sounds_PlayAt(theSounds, Sounds_GetId(theSounds, name), pos, 0, pos, 0, 0x7D, 0x7E, 0x7F,
                                reinterpret_cast<void*>(0x005931D0), on, 6, 1, 1.0f, 0);
    SoundRef_Assign(&self->sound, sound);
}

// 0x593510 (thiscall)
void __fastcall RatSounds_StopAllImpl(SoundList<RatSound>* self) {
    for (int i = 0; i < self->count; i++) {
        RatSound* rat = self->items[i];
        if (rat != nullptr && rat->slot != -1) {
            RatSound_Stop(rat);
        }
    }
}

// 0x593540 (thiscall)
SoundList<RatSound>* __fastcall RatSounds_Init(SoundList<RatSound>* self, re::Edx, int count, int32_t value) {
    self->count = count;
    self->unknown30 = value;
    if (count <= 0) {
        self->freeHint = 0;
        return self;
    }
    for (int i = 0; i < count; i++) {
        void* p = Malloc(sizeof(RatSound));
        self->items[i] = p != nullptr ? static_cast<RatSound*>(RatSound_Construct(p)) : nullptr;
    }
    self->freeHint = 0;
    return self;
}

// 0x5935A0 (thiscall): the rat's squeak stopped
void __fastcall RatSounds_RemoveImpl(SoundList<RatSound>* self, re::Edx, uint8_t* rat) {
    for (int i = 0; i < self->count; i++) {
        RatSound* item = self->items[i];
        if (item != nullptr && item->entity == rat) {
            RatSound_Stop(item);
            self->freeHint = i;
            return;
        }
    }
}

// 0x5935E0 (thiscall)
void __fastcall RatSounds_Shutdown(SoundList<RatSound>* self) {
    RatSounds_StopAll(self);
    for (int i = 0; i < self->count; i++) {
        RatSound* rat = self->items[i];
        if (rat != nullptr) {
            RatSound_Destruct(rat);
            Free(rat);
        }
        self->items[i] = nullptr;
    }
}

// 0x593620 (thiscall)
void __fastcall RatSounds_Update(SoundList<RatSound>* self) {
    for (int i = 0; i < self->count; i++) {
        RatSound* rat = self->items[i];
        if (rat == nullptr) {
            continue;
        }
        if (rat->entity == nullptr) {
            if (rat->slot > -1) {
                RatSound_Stop(rat);
            }
        } else if (rat->sound == nullptr) {
            RatSound_Stop(rat);
        } else {
            RatSound_StopIfGone(rat);
        }
    }
}

// 0x593660 (thiscall): a squeak for the rat (unless it has one)
void __fastcall RatSounds_Add(SoundList<RatSound>* self, re::Edx, uint8_t* rat) {
    int count = self->count;
    for (int i = 0; i < count; i++) {
        if (self->items[i]->entity == rat) {   // (no null check)
            return;
        }
    }
    int slot = self->freeHint;
    if (slot > -1) {
        self->freeHint = -1;
    } else {
        slot = -1;
        for (int i = 0; i < self->count; i++) {
            RatSound* item = self->items[i];
            if (item != nullptr && item->entity == nullptr) {
                slot = i;
                break;
            }
        }
        if (slot == -1) {
            return;
        }
    }
    if (slot > -1) {
        RatSound_Start(self->items[slot], rat, slot);
    }
}

// ---- registration -----------------------------------------------------------------------------

#define LOOPSOUNDS_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(loopsounds_original_, __LINE__);                                              \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(loopsounds_original_, __LINE__), \
             re::Mode::Original)

LOOPSOUNDS_REVERSED("LoopSounds::StopAll", 0x00592390, &LoopSounds_StopAllImpl);
LOOPSOUNDS_REVERSED("LoopSounds::Stop", 0x00592400, &LoopSounds_StopImpl);
LOOPSOUNDS_REVERSED("LoopSounds::StopForPed", 0x005924E0, &LoopSounds_StopForPed);
LOOPSOUNDS_REVERSED("LoopSounds::Has", 0x005925A0, &LoopSounds_HasImpl);
LOOPSOUNDS_REVERSED("LoopSounds::Init", 0x00592600, &LoopSounds_Init);
LOOPSOUNDS_REVERSED("LoopSound::Update", 0x005926B0, &LoopSound_UpdateImpl);
LOOPSOUNDS_REVERSED("LoopSounds::Shutdown", 0x00592810, &LoopSounds_Shutdown);
LOOPSOUNDS_REVERSED("LoopSounds::Update", 0x00592880, &LoopSounds_Update);
LOOPSOUNDS_REVERSED("LoopSound::AtEntityCallback", 0x00592910, &LoopSound_AtEntityCallback);
LOOPSOUNDS_REVERSED("LoopSound::OnEntityCallback", 0x00592AE0, &LoopSound_OnEntityCallback);
LOOPSOUNDS_REVERSED("LoopSound::Start", 0x00592CF0, &LoopSound_StartImpl);
LOOPSOUNDS_REVERSED("LoopSounds::PlayOn", 0x00592F30, &LoopSounds_PlayOn);
LOOPSOUNDS_REVERSED("LoopSounds::PlayAtEntity", 0x00592FA0, &LoopSounds_PlayAtEntity);
LOOPSOUNDS_REVERSED("LoopSounds::PlayAt", 0x00593010, &LoopSounds_PlayAt);
LOOPSOUNDS_REVERSED("RatSound::Destruct", 0x00593080, &RatSound_DestructImpl);
LOOPSOUNDS_REVERSED("RatSound::Die", 0x005930A0, &RatSound_Die);
LOOPSOUNDS_REVERSED("RatSound::Construct", 0x00593100, &RatSound_ConstructImpl);
LOOPSOUNDS_REVERSED("RatSound::Stop", 0x00593150, &RatSound_StopImpl);
LOOPSOUNDS_REVERSED("RatSound::StopIfGone", 0x005931B0, &RatSound_StopIfGoneImpl);
LOOPSOUNDS_REVERSED("RatSound::Callback", 0x005931D0, &RatSound_Callback);
LOOPSOUNDS_REVERSED("RatSound::Start", 0x005933C0, &RatSound_StartImpl);
LOOPSOUNDS_REVERSED("RatSounds::StopAll", 0x00593510, &RatSounds_StopAllImpl);
LOOPSOUNDS_REVERSED("RatSounds::Init", 0x00593540, &RatSounds_Init);
LOOPSOUNDS_REVERSED("RatSounds::Remove", 0x005935A0, &RatSounds_RemoveImpl);
LOOPSOUNDS_REVERSED("RatSounds::Shutdown", 0x005935E0, &RatSounds_Shutdown);
LOOPSOUNDS_REVERSED("RatSounds::Update", 0x00593620, &RatSounds_Update);
LOOPSOUNDS_REVERSED("RatSounds::Add", 0x00593660, &RatSounds_Add);

} // namespace
