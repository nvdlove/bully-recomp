// The footsteps' sounds (the surface under a ped), the SFX items' shared behaviour (vtable
// 0x905A7C: reset, pan toward the listener, the distance to the camera), the table of
// references to the SFX items (sfxManager + 0x44, 0xCF6910) and the SFX manager's queue.
// 0x59C7D0..0x59D1D0 and on.

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

inline uint8_t* const theSounds = reinterpret_cast<uint8_t*>(0x00CF3DA0);
inline void* const sfxItemRefs = reinterpret_cast<void*>(0x00CF6910);   // sfxManager + 0x44

GAME_VAR(uint32_t, currentAreaHash, 0x020C5FAC);
GAME_VAR(uint32_t, footstepAreaHashA, 0x01B65548);
GAME_VAR(uint32_t, footstepAreaHashB, 0x01B6554C);
GAME_VAR(void*, playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t, lastFootstepSurface, 0x00CF7268);   // (the voice engine's (0xCF7268) first byte)
GAME_VAR(int32_t, footstepQuietModel1, 0x00BF42D4);
GAME_VAR(int32_t, footstepQuietModel2, 0x00BF42D0);
GAME_VAR(int32_t, footstepQuietModel3, 0x00BF42CC);
GAME_VAR(uint8_t, soundsPaused, 0x00CF3DA2);

GAME_FN(uint32_t*, __thiscall, Sfx_HashConstruct, 0x005FA870, uint32_t* hash, uint32_t value);
GAME_FN(uint8_t, __cdecl, Sfx_DodgeballIsActive, 0x00683A30);
GAME_FN(int, __thiscall, Sfx_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(uint8_t*, __thiscall, Sfx_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int a3,
        const CVector* pos2, int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12,
        float a13, int a14);
GAME_FN(int32_t, __thiscall, Sfx_Pan, 0x0059B740, void* sounds, const CVector* pos, const CVector* listener);
GAME_FN(CVector*, __cdecl, Sfx_GetCameraPos, 0x00595630, CVector* out);
GAME_FN(CVector*, __cdecl, Sfx_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(int, __cdecl, Sfx_ItemIndex, 0x0044CB50, void* item);
GAME_FN(void, __thiscall, Sfx_SetRef, 0x005D7F10, re::Ptr32<void>* ref, void* entity);
GAME_FN(void, __thiscall, Sfx_SoundBaseDestruct, 0x0059E780, void* self);
GAME_FN(void, __thiscall, Sfx_VoiceCollect, 0x005A9B90, void* voice, void* out);
// its own
GAME_FN(uint8_t, __cdecl, Sfx_FootstepsQuiet, 0x0059C7D0);
GAME_FN(int, __stdcall, Sfx_FootstepSurface, 0x0059C850, void* ped, float a2, int a3);
GAME_FN(uint8_t, __cdecl, Sfx_Approach, 0x0059CF30, int32_t* value, int target, int step);
GAME_FN(void, __thiscall, SfxRefs_Add, 0x0059CDF0, void* refs, re::Ptr32<void>* slot);
GAME_FN(void, __thiscall, SfxRefs_Remove, 0x0059CE40, void* refs, re::Ptr32<void>* slot);
GAME_FN(uint8_t, __thiscall, SFXItem_UpdateOcclusion, 0x0059D230, void* item);
GAME_FN(int32_t, __thiscall, SFXItem_VolumeAt, 0x0059D440, void* item, float distance);
GAME_FN(void, __thiscall, SfxFade_Start, 0x0059E3C0, void* fade, float from, float to, uint32_t start, int duration);
GAME_FN(uint8_t, __thiscall, SfxFade_Apply, 0x0059E600, void* fade, float* value);
GAME_FN(int, __cdecl, Sfx_DistanceVolume, 0x0059E3E0, float distance, int curve);
GAME_FN(float, __cdecl, Sfx_Rolloff, 0x0059E4F0, float volume, int curve);
GAME_FN(void, __thiscall, SfxQueue_Remove, 0x0059DE30, void* queue, void* item);
inline int __cdecl Sfx_Rand() { return crt::Rand(); }
GAME_FN(uint8_t, __cdecl, Sfx_ProcessLineOfSight, 0x00461EA0, const CVector* start, const CVector* end,
        void* colPoint, re::Ptr32<void>* entity, int flags, int, int);
GAME_FN(void, __cdecl, Sfx_CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(uint8_t, __cdecl, Sfx_IsAreaInterior, 0x00430320, int area);
GAME_FN(void, __thiscall, Sfx_VoiceStop, 0x005AA200, void* voice, int);
GAME_FN(void, __thiscall, Sfx_VoiceListFree, 0x00587170, void* list);
GAME_FN(void, __cdecl, Sfx_Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t, __cdecl, Sfx_VoiceIsPlaying, 0x005AF530, int voice);
GAME_FN(void, __cdecl, Sfx_VoiceSetVolume, 0x005AF870, int voice, int volume, float);
GAME_FN(void, __cdecl, Sfx_VoiceStart, 0x005AF7B0, int voice, int volume, int pan, int);
GAME_FN(void, __cdecl, Sfx_VoiceSetVolumeOnly, 0x005AF760, int voice, int volume, int);
GAME_FN(void, __cdecl, Sfx_VoiceSetPitch, 0x005AF660, int voice, float);
GAME_FN(float, __cdecl, Sfx_BalanceToPan, 0x005A7F00, float, float);
GAME_VAR(void*, alwaysValidEntity, 0x00C2A8E8);
GAME_VAR(uint32_t, timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(float, soundTimeStep, 0x00CF60F0);

// the table of references (0xCF6910): per SFX item (pool index) a count and 4 slots
struct SfxItemRefs {
    uint16_t freed[48];     // +0x00 (bumped each time the item is freed)
    re::Ptr32<re::Ptr32<void>> slots[48][4];    // +0x60 the references pointing at the item
};
static_assert(offsetof(SfxItemRefs, slots) == 0x60);

// ---- footsteps ------------------------------------------------------------------------------

// 0x59C7D0 (cdecl): the current area's hash is one of the two quiet-footstep areas
uint8_t __cdecl Sfx_FootstepsQuietImpl() {
    uint32_t hash = 0;
    Sfx_HashConstruct(&hash, currentAreaHash);
    if (footstepAreaHashA == hash) {
        return 1;
    }
    uint32_t hash2;
    Sfx_HashConstruct(&hash2, currentAreaHash);
    return footstepAreaHashB == hash2 ? 1 : 0;
}

// 0x59CAF8's table: the surface (1..0x42) to the sound's class
constexpr uint8_t kSurfaceSound[0x42] = {
    0x14, 0x48, 0x46, 0x47, 0x49, 0x1C, 0x15, 0x17, 0x16, 0x18, 0x19, 0x1A, 0x1B, 0x23, 0x0D, 0x3E,
    0x00, 0x00, 0x1E, 0x22, 0x26, 0x27, 0x00, 0x24, 0x20, 0x01, 0x4A, 0x28, 0x29, 0x5A, 0x2B, 0x03,
    0x00, 0x02, 0x04, 0x05, 0x06, 0x35, 0x33, 0x34, 0x07, 0x08, 0x4B, 0x4C, 0x50, 0x1F, 0x00, 0x00,
    0x0A, 0x0B, 0x0C, 0x00, 0x25, 0x00, 0x00, 0x21, 0x3C, 0x3D, 0x09, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x51,
};

// 0x59C850 (thiscall, ecx unused, ret 0xC): the footstep's surface class under an entity (a
// ped: the vehicle it's on, +0x1554, when there is one)
int __stdcall Sfx_FootstepSurfaceImpl(uint8_t* entity, float, int) {
    uint8_t type = At<uint8_t>(entity, 0x108) & 7;
    uint8_t surface = At<uint8_t>(entity, 0x1BE);
    uint8_t special = At<uint8_t>(entity, 0x1BF);
    if (type == 3 && At<uint8_t*>(entity, 0x1554) != nullptr) {
        uint8_t* on = At<uint8_t*>(entity, 0x1554);
        surface = At<uint8_t>(on, 0x1BE);
        special = At<uint8_t>(on, 0x1BF);
    }
    switch (special) {
    case 1: {
        uint8_t* matrix = At<uint8_t*>(entity, 0x14);
        const CVector* pos = matrix != nullptr ? reinterpret_cast<CVector*>(matrix + 0x30)
                                               : reinterpret_cast<CVector*>(entity + 4);
        return pos->z < 0.0f ? 0x5A : 0x2A;   // (NaN: 0x2A)
    }
    case 2:
        return 0x49;
    case 3:
        return 0x51;
    default:
        break;
    }
    int result = 0;
    unsigned index = unsigned(surface) - 1;
    if (index <= 0x41) {
        result = kSurfaceSound[index];
    }
    if (type == 3) {
        int model = At<int16_t>(entity, 0x10E);
        if (model == footstepQuietModel1 || model == footstepQuietModel2 || model == footstepQuietModel3) {
            return 0x46;
        }
    }
    return result;
}

// 0x59CC00 (thiscall, ecx unused, ret 0x14): a ped's footstep (its volume from the speed)
void __fastcall Sfx_PlayFootstep(void*, re::Edx, uint8_t* ped, int id, int soundClass, float a4, int a5) {
    if (ped == nullptr) {
        return;
    }
    float vx = At<float>(ped, 0x1708);
    float vy = At<float>(ped, 0x170C);
    float vz = At<float>(ped, 0x1710);
    float sum = (vx * vx + vy * vy) + vz * vz;
    float speed = float(std::sqrt(double(std::fabs(sum))));   // (0x85C500)
    lastFootstepSurface = uint8_t(Sfx_FootstepSurface(ped, a4, a5));
    int cls = At<int32_t>(ped, 0x1398) != 0 ? 3 : soundClass;
    float s;
    if (Sfx_DodgeballIsActive() != 0) {
        s = float(double(speed) * 3.0);
        cls = 10;
    } else {
        s = float(double(speed) * 4.0);
    }
    if (double(s) < 0.2) {
        s = 0.2f;
    }
    if (double(s) > 0.7) {
        s = 0.7f;
    }
    float scaled = float(double(s) * 127.0);
    int volume = int(float(double(scaled) * 1.5));
    if (playerPed == ped && Sfx_FootstepsQuiet() != 0) {
        volume = int(float(double(volume) / 2.5));
        if (id == Sfx_GetId(theSounds, "Walk") || id == Sfx_GetId(theSounds, "P_Walk") ||
            id == Sfx_GetId(theSounds, "F_Walk") || id == Sfx_GetId(theSounds, "Run") ||
            id == Sfx_GetId(theSounds, "P_Run") || id == Sfx_GetId(theSounds, "Fast_Run")) {
            id = Sfx_GetId(theSounds, "S_Walk");
        }
    }
    uint8_t* matrix = At<uint8_t*>(ped, 0x14);
    const CVector* pos = matrix != nullptr ? reinterpret_cast<CVector*>(matrix + 0x30)
                                           : reinterpret_cast<CVector*>(ped + 4);
    Sfx_PlayAt(theSounds, id, pos, cls, nullptr, volume, volume, 0, 0x63, nullptr, nullptr, 6, 1, 1.0f, 0);
}

// ---- the references to the SFX items (0xCF6910) ----------------------------------------------

// 0x59CDF0 (thiscall): a reference into the item's first free slot (none free: not kept)
void __fastcall SfxRefs_AddImpl(SfxItemRefs* self, re::Edx, re::Ptr32<void>* slot) {
    unsigned index = unsigned(Sfx_ItemIndex(*slot));
    if (index > 0x2F) {
        return;
    }
    for (int i = 0; i < 4; ++i) {
        if (self->slots[index][i] == nullptr) {
            self->slots[index][i] = slot;
            return;
        }
    }
}

// 0x59CE40 (thiscall)
void __fastcall SfxRefs_RemoveImpl(SfxItemRefs* self, re::Edx, re::Ptr32<void>* slot) {
    if (slot == nullptr || *slot == nullptr) {
        return;
    }
    unsigned index = unsigned(Sfx_ItemIndex(*slot));
    if (index > 0x2F) {
        return;
    }
    for (int i = 0; i < 4; ++i) {
        if (self->slots[index][i] == slot) {
            self->slots[index][i] = nullptr;
            return;
        }
    }
}

// 0x59D110 (thiscall on the reference)
void __fastcall SfxRef_SetImpl(re::Ptr32<void>* self, re::Edx, void* item) {
    if (*self == item) {
        return;
    }
    SfxRefs_Remove(sfxItemRefs, self);
    *self = item;
    if (item != nullptr) {
        SfxRefs_Add(sfxItemRefs, self);
    }
}

// 0x59D1D0 (thiscall): an item freed: its count bumped and its references cleared (through
// 0xCF6910 whatever this is)
void __fastcall SfxRefs_OnItemFreed(SfxItemRefs* self, re::Edx, void* item) {
    unsigned index = unsigned(Sfx_ItemIndex(item));
    if (index > 0x2F) {
        return;
    }
    self->freed[index] = uint16_t(self->freed[index] + 1);
    for (int i = 0; i < 4; ++i) {
        re::Ptr32<void>* slot = self->slots[index][i];
        if (slot != nullptr && *slot != nullptr) {
            SfxRefs_Remove(sfxItemRefs, slot);
            *slot = nullptr;
        }
    }
}

// ---- SFXItem ----------------------------------------------------------------------------------

// 0x59CEA0 (thiscall): the destructor
void __fastcall SFXItem_DestructImpl(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x00905A7C;
    Sfx_SetRef(reinterpret_cast<re::Ptr32<void>*>(self + 0x64), nullptr);
    Sfx_SoundBaseDestruct(self);
}

// 0x59CEC0 (thiscall): back to the defaults
void __fastcall SFXItem_Reset(uint8_t* self) {
    At<uint32_t>(self, 0x10) &= ~1u;
    At<int32_t>(self, 0x14) = 0;
    At<uint8_t>(self, 0x3D) = 0x40;
    At<uint16_t>(self, 0x3E) = 0x20;
    At<int32_t>(self, 0x1C) = 1;
    At<uint8_t>(self, 0x5C) = 0;
    At<int32_t>(self, 0x68) = 0;
    At<uint8_t>(self, 0x6E) = 1;
    At<uint8_t>(self, 0x6C) = 1;
    At<int32_t>(self, 0x48) = 0;
    At<uint8_t>(self, 0x50) = 0;
    At<uint8_t>(self, 0x5D) = 0;
    At<uint8_t>(self, 0x72) = 1;
    At<uint8_t>(self, 0x71) = 0;
    At<uint8_t>(self, 0x6F) = 0;
    At<uint8_t>(self, 0x70) = 1;
    At<uint8_t>(self, 0x5E) = 0;
    At<uint8_t>(self, 0x5F) = 0;
    if (soundsPaused != 0) {
        At<int32_t>(self, 0x1C) = 0;
    }
}

// 0x59CF10 (thiscall)
uint8_t __fastcall SFXItem_IsActiveImpl(uint8_t* self) {
    if (At<int32_t>(self, 0x1C) == 0) {
        return 0;
    }
    return At<uint8_t>(self, 0x70);
}

// 0x59CF20 (thiscall): the voice's (+0x14) channels into out
void __fastcall SFXItem_CollectVoices(uint8_t* self, re::Edx, void* out) {
    void* voice = At<void*>(self, 0x14);
    if (voice != nullptr) {
        Sfx_VoiceCollect(voice, out);
    }
}

// 0x59CF30 (cdecl): value a step toward target (not past it); 0 when already there
uint8_t __cdecl Sfx_ApproachImpl(int32_t* value, int target, int step) {
    int32_t current = *value;
    if (target == current) {
        return 0;
    }
    float t = float(target);
    if (target > current) {
        float next = float(double(current) + double(step));
        *value = t > next ? int32_t(next) : int32_t(t);
    } else {
        float next = float(double(current) - double(step));
        *value = t < next ? int32_t(next) : int32_t(t);
    }
    return 1;
}

// 0x59CFC0 (cdecl): the float one (NaN: stepping down)
uint8_t __cdecl Sfx_ApproachF(float* value, float target, float step) {
    if (*value == target) {
        return 0;
    }
    float current = *value;
    if (current < target) {
        float next = current + step;
        *value = next < target ? next : target;
    } else {
        float next = current - step;
        *value = next > target ? next : target;
    }
    return 1;
}

// 0x59D030 (thiscall): the pan (vf 0x18/0x1C) a step (+0x3D) toward the position's, the short
// way round; the pan
int32_t __fastcall SFXItem_UpdatePan(uint8_t* self) {
    int32_t current = vc::Call<int32_t>(self, 0x18);
    const CVector* pos = vc::Call<const CVector*>(self, 0x88);
    int32_t target = Sfx_Pan(theSounds, pos, nullptr);
    float diff = std::fabs(float(target - current));
    if (diff > 180.0f) {
        if (current > target) {
            target += 0x168;
        } else {
            target -= 0x168;
        }
    }
    if (Sfx_Approach(&current, target, At<uint8_t>(self, 0x3D)) == 0) {
        return current;
    }
    if (current >= 0x168) {
        current -= 0x168;
    } else if (current < 0) {
        current += 0x168;
    }
    vc::Call<void>(self, 0x1C, current);
    return current;
}

// 0x59D100 (thiscall)
void __fastcall SFXItem_SetStopping(uint8_t* self) {
    At<uint8_t>(self, 0x5C) = 1;
}

// 0x59D140 (thiscall): the distance from the camera (no position: 0)
float __fastcall SFXItem_CameraDistance(uint8_t* self) {
    if (vc::Call<const CVector*>(self, 0x88) == nullptr) {
        return 0.0f;
    }
    CVector camera;
    Sfx_GetCameraPos(&camera);
    const CVector* pos = vc::Call<const CVector*>(self, 0x88);
    CVector d;
    Sfx_VectorSub(&d, pos, &camera);
    float sum = (d.x * d.x + d.y * d.y) + d.z * d.z;
    return float(std::sqrt(double(std::fabs(sum))));
}

// a collision point as the line of sight fills it (0x28)
struct SfxColPoint {
    CVector point;    // +0
    re::Ptr32<void> entity;     // +0xC (a registered reference)
    CVector normal;   // +0x10
    int32_t piece;    // +0x1C (the game leaves it uninitialised)
    int32_t surfaces; // +0x20 (the game leaves it uninitialised)
    float depth;      // +0x24
};
static_assert(sizeof(SfxColPoint) == 0x28);

// the voices an item plays on (0x5A9B90 fills it)
struct SfxVoiceList {   // (the game's layout)
    re::Ptr32<int32_t> data;     // +0
    uint32_t capacity; // +4
    uint32_t count;    // +8
};

// 0x59D230 (thiscall): is the player's view of the item blocked (checked every 9..11 frames,
// +0x3C the countdown; a change fades +0x40 between 1 and 0.5 over a second)
uint8_t __fastcall SFXItem_UpdateOcclusionImpl(uint8_t* self) {
    if (vc::Call<uint8_t>(self, 0x9C) == 0) {
        return 0;
    }
    if (At<uint8_t>(self, 0x3C) != 0) {
        return At<uint8_t>(self, 0x50);
    }
    int r = Sfx_Rand() * 3;
    uint8_t was = At<uint8_t>(self, 0x50);
    SfxColPoint col;
    col.point = {0.0f, 0.0f, 0.0f};
    col.entity = nullptr;
    col.normal = {0.0f, 0.0f, 0.0f};
    col.piece = 0;      // (uninitialised in the game)
    col.surfaces = 0;   // (uninitialised in the game)
    col.depth = 3.40282347e+38f;
    At<uint8_t>(self, 0x3C) = uint8_t(r / 0x7FFF + 9);
    uint8_t* player = static_cast<uint8_t*>(playerPed);
    uint8_t* matrix = At<uint8_t*>(player, 0x14);
    CVector start = matrix != nullptr ? *reinterpret_cast<CVector*>(matrix + 0x30)
                                      : *reinterpret_cast<CVector*>(player + 4);
    CVector end = *vc::Call<const CVector*>(self, 0x88);
    float dz = std::fabs(end.z - start.z);
    if (dz < 0.02f) {
        end.z = float(double(end.z) + double(0.1f));
    }
    start.z = float(double(start.z) + double(0.9f));
    re::Ptr32<void> hitEntity = nullptr;   // (uninitialised in the game)
    uint8_t blocked = Sfx_ProcessLineOfSight(&start, &end, &col, &hitEntity, 0x41, 0, 0);
    At<uint8_t>(self, 0x50) = blocked;
    if (was != blocked) {
        if (blocked != 0) {
            SfxFade_Start(self + 0x40, 1.0f, 0.5f, timeInMilliseconds, 1000);
        } else {
            SfxFade_Start(self + 0x40, 0.5f, 1.0f, timeInMilliseconds, 1000);
        }
    }
    if (col.entity != nullptr) {
        Sfx_CleanUpOldReference(col.entity, &col.entity);
    }
    return At<uint8_t>(self, 0x50);
}

// 0x59D3F0 (thiscall): the occlusion's scale (0.5 blocked) through the fade (+0x40)
float __fastcall SFXItem_OcclusionScale(uint8_t* self) {
    float scale = 1.0f;
    if (SFXItem_UpdateOcclusion(self) != 0) {
        scale = 0.5f;
    }
    if (At<int32_t>(self, 0x48) > 0) {
        SfxFade_Apply(self + 0x40, &scale);
    }
    return scale;
}

// 0x59D440 (thiscall): the volume at a distance (the curve +0x54), outdoors quieter the
// further above or below the player (when +0x6D), and occluded
int32_t __fastcall SFXItem_VolumeAtImpl(uint8_t* self, re::Edx, float distance) {
    int base = Sfx_DistanceVolume(distance, At<int32_t>(self, 0x54));
    float height = 1.0f;
    uint8_t* player = static_cast<uint8_t*>(playerPed);
    if (player != nullptr && vc::Call<const CVector*>(self, 0x88) != nullptr && Sfx_IsAreaInterior(currArea) == 0) {
        uint8_t* matrix = At<uint8_t*>(player, 0x14);
        const CVector* playerPos = matrix != nullptr ? reinterpret_cast<CVector*>(matrix + 0x30)
                                                     : reinterpret_cast<CVector*>(player + 4);
        const CVector* pos = vc::Call<const CVector*>(self, 0x88);
        float dz = std::fabs(pos->z - playerPos->z);
        if (dz > 1.0f && At<uint8_t>(self, 0x6D) != 0) {
            if (dz > 4.0f) {
                dz = 4.0f;
            }
            float t = dz - 1.0f;
            t = float(double(t) / 3.0);
            t = float(double(t) * 0.8);
            height = 1.0f - t;
        } else {
            height = 1.0f;
        }
    }
    if (base <= 0x10) {
        return int32_t(float(base) * height);
    }
    float scale = 1.0f;
    if (SFXItem_UpdateOcclusion(self) != 0) {
        scale = 0.5f;
    }
    if (At<int32_t>(self, 0x48) > 0) {
        SfxFade_Apply(self + 0x40, &scale);
    }
    if (SFXItem_UpdateOcclusion(self) == 0) {
        height = float((double(height) + 1.0) * 0.5);
    }
    height = scale * height;
    return int32_t(float(base) * height);
}

// 0x59D5C0 (thiscall): the volume (vf 0x10/0x14) a step (+0x3E) toward the distance's
int32_t __fastcall SFXItem_UpdateVolume(uint8_t* self, re::Edx, int distance) {
    int32_t current = vc::Call<int32_t>(self, 0x10);
    int32_t target = SFXItem_VolumeAt(self, float(distance));
    if (Sfx_Approach(&current, target, At<uint16_t>(self, 0x3E)) == 0) {
        return current;
    }
    vc::Call<void>(self, 0x14, current);
    return current;
}

using SfxItemCallback = void(__cdecl*)(float value, float delta, void* user, void* item);

// 0x59D620 (thiscall, vf1): the frame's update: stopping (+0x5C) hands the voice back, then
// the volume and pan (or the 2D volume) onto every voice it plays on
void __fastcall SFXItem_Update(uint8_t* self) {
    if (At<uint8_t>(self, 0x3C) != 0) {
        At<uint8_t>(self, 0x3C) = uint8_t(At<uint8_t>(self, 0x3C) - 1);
    }
    if (soundsPaused != 0) {
        return;
    }
    if (At<uint8_t>(self, 0x5D) != 0) {
        SfxQueue_Remove(At<void*>(self, 0x38), self);
        return;
    }
    uint8_t* voice = At<uint8_t*>(self, 0x14);
    if (voice == nullptr || (At<int32_t>(voice, 0x1C) == 6 && At<int32_t>(voice, 0x18) == 0)) {
        At<uint8_t>(self, 0x5C) = 1;
    }
    if (At<uint8_t>(self, 0x5C) != 0) {
        At<uint8_t>(self, 0x5D) = 1;
        if (voice != nullptr) {
            Sfx_VoiceStop(voice, 1);
            At<void*>(self, 0x14) = nullptr;
        }
        return;
    }
    if (playerPed == nullptr) {
        return;
    }
    SfxVoiceList voices = {nullptr, 0, 0};
    if (voice != nullptr) {
        Sfx_VoiceCollect(voice, &voices);
    }
    if (vc::Call<uint8_t>(self, 0x4C) == 0) {
        float a = vc::Call<float>(self, 0xB0);
        float b = vc::Call<float>(self, 0x74);
        float delta = b - a;
        vc::Call<void>(self, 0x78, a);
        SfxItemCallback callback = At<SfxItemCallback>(self, 0x60);
        if (callback != nullptr) {
            callback(a, delta, At<void*>(self, 0x64), self);
        } else {
            if (At<uint8_t>(self, 0x5E) != 0) {
                Sfx_VoiceListFree(&voices);
                return;
            }
            int32_t volume = vc::Call<int32_t>(self, 0xB4, int32_t(a));
            int curve = At<int32_t>(self, 0x58);
            if (curve != 6) {
                volume = int32_t(Sfx_Rolloff(float(volume), curve));
            }
            if (volume <= 0) {
                if (At<uint8_t>(self, 0x6E) != 0) {
                    At<uint8_t>(self, 0x5C) = 1;
                }
                Sfx_VoiceListFree(&voices);
                return;
            }
            int pan = vc::Call<int>(self, 0xB8);
            for (uint32_t i = 0; i < voices.count; ++i) {
                if (voices.data[i] == -1) {
                    continue;
                }
                if (Sfx_VoiceIsPlaying(voices.data[i]) != 0) {
                    Sfx_VoiceSetVolume(voices.data[i], volume, 0.0f);
                } else {
                    Sfx_VoiceStart(voices.data[i], volume, pan, 1);
                }
            }
            if (vc::Call<uint8_t>(self, 0x7C) != 0) {
                float pitch = Sfx_BalanceToPan(delta, soundTimeStep);
                for (uint32_t i = 0; i < voices.count; ++i) {
                    if (voices.data[i] != -1) {
                        Sfx_VoiceSetPitch(voices.data[i], pitch);
                    }
                }
            }
        }
    } else {
        if (At<uint8_t>(self, 0x5E) != 0) {
            Sfx_VoiceListFree(&voices);
            return;
        }
        int curve = At<int32_t>(self, 0x58);
        if (curve != 6) {
            float volume = Sfx_Rolloff(float(vc::Call<int32_t>(self, 0x10)), curve);
            for (uint32_t i = 0; i < voices.count; ++i) {
                if (voices.data[i] == -1) {
                    continue;
                }
                if (Sfx_VoiceIsPlaying(voices.data[i]) != 0) {
                    Sfx_VoiceSetVolume(voices.data[i], int32_t(volume), 0.0f);
                } else {
                    Sfx_VoiceSetVolumeOnly(voices.data[i], int32_t(volume), 1);
                }
            }
        }
    }
    if (voices.data != nullptr) {
        Sfx_Free(voices.data);
    }
}

// the speakers' gains a voice starts with (0x5A89F0/0x5A8230 fill it)
struct SfxGains {
    re::Ptr32<float> data;       // +0
    uint32_t capacity; // +4
    uint32_t count;    // +8
};

// what the SFX manager's second play takes (0x24 bytes; see SoundsPlay.cpp)
struct SfxPlayParams {
    int32_t volume;      // +0 (flag 1)
    int32_t unknown4;    // +4
    CVector pos;         // +8 (flag 2)
    uint8_t occluded;    // +0x14 (flag 4)
    uint8_t pad15[3];
    float distance;      // +0x18 (flag 8)
    int32_t pan;         // +0x1C (flag 0x10)
    uint32_t flags;      // +0x20
};
static_assert(sizeof(SfxPlayParams) == 0x24);

inline void* const voiceEngine = reinterpret_cast<void*>(0x00CF7268);
GAME_FN(void*, __cdecl, Sfx_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Sfx_GainsByAngle, 0x005A89F0, SfxGains* gains, int volume, int pan);
GAME_FN(void, __cdecl, Sfx_GainsForLevel, 0x005A8230, SfxGains* gains, int volume);
GAME_FN(void*, __thiscall, Sfx_VoiceEngineStart, 0x005AA8C0, void* engine, int id, int, SfxGains* gains);
GAME_FN(void, __thiscall, SfxBase_Construct, 0x0059E750, void* self, int a1, float a2, int a3);
GAME_FN(void, __thiscall, SFXItem_ResetFn, 0x0059CEC0, void* item);
GAME_FN(float, __thiscall, SFXItem_CameraDistanceFn, 0x0059D140, void* item);
GAME_FN(int32_t, __thiscall, SFXItem_UpdatePanFn, 0x0059D030, void* item);

// the inlined reserve(6) of an empty vector (the old contents, none, copied and freed)
void SfxGains_Reserve6(SfxGains* gains) {
    gains->data = nullptr;
    gains->count = 0;
    gains->capacity = 6;
    gains->data = static_cast<float*>(Sfx_Malloc(6 * sizeof(float)));
}

// the common head of the two constructors
void SFXItem_InitFields(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x00905A7C;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
    At<float>(self, 0x40) = 0.0f;
    At<int32_t>(self, 0x48) = 0;
    At<float>(self, 0x44) = 0.0f;
    At<int32_t>(self, 0x4C) = 0;
    At<void*>(self, 0x64) = nullptr;
    SFXItem_ResetFn(self);
}

// 0x59D950 (thiscall, ret 0x38): the constructor for a sound at a position (null: 2D), the
// voice started at once (unless paused); a4, a5 and a7..a10 unused
uint8_t* __fastcall SFXItem_ConstructAt(uint8_t* self, re::Edx, int id, int a2, int a3, int, int, const CVector* pos,
                                        int, int, int, int, void* callback, void* user, int curve, int rolloff) {
    SfxBase_Construct(self, a2, 1.0f, a3);
    SFXItem_InitFields(self);
    At<int32_t>(self, 0x18) = id;
    At<void*>(self, 0x60) = callback;
    Sfx_SetRef(reinterpret_cast<re::Ptr32<void>*>(self + 0x64), user);
    At<int32_t>(self, 0x54) = curve;
    At<int32_t>(self, 0x58) = rolloff == 6 ? 4 : rolloff;
    if (pos == nullptr) {
        At<uint32_t>(self, 0x10) |= 4;
    } else {
        At<uint32_t>(self, 0x10) &= ~4u;
    }
    At<const CVector*>(self, 0x2C) = pos;
    if (pos != nullptr) {
        At<uint8_t>(self, 0x3C) = 0;
        SFXItem_UpdateOcclusion(self);
        At<int32_t>(self, 0x48) = 0;
    } else {
        At<uint8_t>(self, 0x3C) = uint8_t(Sfx_Rand() * 0x1D / 0x7FFF);
    }
    SfxGains gains;
    SfxGains_Reserve6(&gains);
    int32_t volume = At<int32_t>(self, 4);
    if ((At<uint32_t>(self, 0x10) >> 2 & 1) == 0) {
        float distance = SFXItem_CameraDistanceFn(self);
        At<float>(self, 0x30) = distance;
        volume = SFXItem_VolumeAt(self, distance);
        At<int32_t>(self, 4) = volume;
        if (At<int32_t>(self, 0x58) != 6) {
            volume = int32_t(Sfx_Rolloff(float(volume), At<int32_t>(self, 0x58)));
        }
        int32_t pan = SFXItem_UpdatePanFn(self);
        Sfx_GainsByAngle(&gains, volume, pan);
    } else {
        if (At<int32_t>(self, 0x58) != 6) {
            volume = int32_t(Sfx_Rolloff(float(volume), At<int32_t>(self, 0x58)));
        }
        Sfx_GainsForLevel(&gains, volume);
    }
    if (soundsPaused == 0) {
        At<void*>(self, 0x14) = Sfx_VoiceEngineStart(voiceEngine, id, 0, &gains);
    }
    if (gains.data != nullptr) {
        Sfx_Free(gains.data);
    }
    return self;
}

// 0x59DB60 (thiscall, ret 0x34): the constructor from the play's parameters (their flags
// saying which of volume, position, occlusion, distance and pan are given); a3, a4 and
// a6..a9 unused
uint8_t* __fastcall SFXItem_ConstructWithParams(uint8_t* self, re::Edx, int id, const SfxPlayParams* params, int, int,
                                                const CVector* pos, int, int, int, int, void* callback, void* user,
                                                int curve, int rolloff) {
    SfxBase_Construct(self, 0, 1.0f, 0);
    SFXItem_InitFields(self);
    At<void*>(self, 0x60) = callback;
    At<int32_t>(self, 0x18) = id;
    Sfx_SetRef(reinterpret_cast<re::Ptr32<void>*>(self + 0x64), user);
    At<int32_t>(self, 0x54) = curve;
    At<uint8_t>(self, 0x6D) = 1;
    At<int32_t>(self, 0x58) = rolloff == 6 ? 4 : rolloff;
    At<uint32_t>(self, 0x10) |= 4;
    if ((params->flags >> 1 & 1) != 0) {
        At<CVector>(self, 0x20) = params->pos;
        At<uint32_t>(self, 0x10) &= ~4u;
        if (pos == nullptr) {
            At<const CVector*>(self, 0x2C) = reinterpret_cast<CVector*>(self + 0x20);
        } else {
            At<uint32_t>(self, 0x10) &= ~4u;
            At<const CVector*>(self, 0x2C) = pos;
        }
    } else if (pos != nullptr) {
        At<uint32_t>(self, 0x10) &= ~4u;
        At<const CVector*>(self, 0x2C) = pos;
    }
    if ((params->flags >> 2 & 1) != 0) {
        At<uint8_t>(self, 0x50) = params->occluded;
    } else if (At<const CVector*>(self, 0x2C) != nullptr) {
        At<uint8_t>(self, 0x3C) = 0;
        SFXItem_UpdateOcclusion(self);
        At<int32_t>(self, 0x48) = 0;
    } else {
        At<uint8_t>(self, 0x3C) = uint8_t(Sfx_Rand() * 0x1D / 0x7FFF);
    }
    SfxGains gains;
    SfxGains_Reserve6(&gains);
    int32_t volume = At<int32_t>(self, 4);
    if ((params->flags & 1) != 0) {
        volume = params->volume;
        At<int32_t>(self, 4) = volume;
    }
    int32_t pan = (params->flags >> 4 & 1) != 0 ? params->pan : SFXItem_UpdatePanFn(self);
    At<int32_t>(self, 8) = pan;
    if ((At<uint32_t>(self, 0x10) >> 2 & 1) == 0) {
        float distance = (params->flags >> 3 & 1) != 0 ? params->distance : SFXItem_CameraDistanceFn(self);
        At<float>(self, 0x30) = distance;
        volume = (params->flags & 1) != 0 ? params->volume : SFXItem_VolumeAt(self, distance);
        At<int32_t>(self, 4) = volume;
        if (At<int32_t>(self, 0x58) != 6) {
            volume = int32_t(Sfx_Rolloff(float(volume), At<int32_t>(self, 0x58)));
        }
        Sfx_GainsByAngle(&gains, volume, At<int32_t>(self, 8));
    } else {
        if (At<int32_t>(self, 0x58) != 6) {
            volume = int32_t(Sfx_Rolloff(float(At<int32_t>(self, 4)), At<int32_t>(self, 0x58)));
        }
        Sfx_GainsForLevel(&gains, volume);
    }
    if (soundsPaused == 0) {
        At<void*>(self, 0x14) = Sfx_VoiceEngineStart(voiceEngine, id, 0, &gains);
    }
    if (gains.data != nullptr) {
        Sfx_Free(gains.data);
    }
    return self;
}

// ---- the SFX manager's queue (sfxManager, 0xCF68CC) -------------------------------------------

struct SfxQueue {
    re::Ptr32<uint8_t> items[40];  // +0x00
    int32_t freeHint;    // +0xA0 the slot last emptied (-1 none)
    int32_t count;       // +0xA4
    int32_t unknownA8;   // +0xA8
};
static_assert(offsetof(SfxQueue, freeHint) == 0xA0);
static_assert(sizeof(SfxQueue) == 0xAC);

GAME_FN(int, __thiscall, SfxQueue_FindFreeFn, 0x0059DF50, void* queue);
GAME_FN(void*, __cdecl, SfxItem_New, 0x0059D430, size_t size);
GAME_FN(uint8_t*, __thiscall, SFXItem_ConstructAtFn, 0x0059D950, void* self, int id, int a2, int a3, int a4, int a5,
        const CVector* pos, int a7, int a8, int a9, int a10, void* callback, void* user, int curve, int rolloff);
GAME_FN(uint8_t*, __thiscall, SFXItem_ConstructWithParamsFn, 0x0059DB60, void* self, int id,
        const SfxPlayParams* params, int a3, int a4, const CVector* pos, int a6, int a7, int a8, int a9,
        void* callback, void* user, int curve, int rolloff);

// 0x59DE00 (thiscall): every item's frame (vf1)
void __fastcall SfxQueue_UpdateAll(SfxQueue* self) {
    for (int i = 0; i < self->count; ++i) {
        if (self->items[i] != nullptr) {
            vc::Call<void>(self->items[i], 4);
        }
    }
}

// 0x59DE30 (thiscall): an item deleted and its slot emptied (not found: nothing; a null item
// matches the first empty slot)
void __fastcall SfxQueue_RemoveImpl(SfxQueue* self, re::Edx, uint8_t* item) {
    for (int i = 0; i < self->count; ++i) {
        if (item == self->items[i]) {
            if (item != nullptr) {
                vc::Call<void>(item, 0, 1);
            }
            self->freeHint = i;
            self->items[i] = nullptr;
            return;
        }
    }
}

// 0x59DE80 (thiscall): every item told to stop (+0x5C), then updated until all are gone
void __fastcall SfxQueue_StopAllImpl(SfxQueue* self) {
    bool any = false;
    for (int i = 0; i < self->count; ++i) {
        if (self->items[i] != nullptr) {
            self->items[i][0x5C] = 1;
            any = true;
        }
    }
    if (!any) {
        return;
    }
    bool updated;
    do {
        updated = false;
        for (int i = 0; i < self->count; ++i) {
            if (self->items[i] != nullptr) {
                vc::Call<void>(self->items[i], 4);
                updated = true;
            }
        }
    } while (updated);
}

// 0x59DEF0 (thiscall)
SfxQueue* __fastcall SfxQueue_ConstructImpl(SfxQueue* self, re::Edx, int count, int a2) {
    self->unknownA8 = a2;
    self->count = count;
    self->freeHint = -1;
    // (the count is re-read each time, as the game does: a count past the array zeroes it
    // and ends the loop; volatile keeps the compiler from hoisting the read)
    for (int i = 0; i < *reinterpret_cast<volatile int32_t*>(&self->count); ++i) {
        re::StoreVolatile(&reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<void*>(self->items))[i], nullptr);
    }
    return self;
}

// 0x59DF50 (thiscall): a free slot (the last emptied, else the first empty); none: every item
// is stopped and deleted, and -1 anyway
int __fastcall SfxQueue_FindFree(SfxQueue* self) {
    int hint = self->freeHint;
    if (hint != -1) {
        self->freeHint = -1;
        return hint;
    }
    int count = self->count;
    for (int i = 0; i < count; ++i) {
        if (self->items[i] == nullptr) {
            return i;
        }
    }
    for (int i = 0; i < self->count; ++i) {
        uint8_t* item = self->items[i];
        if (item == nullptr) {
            continue;
        }
        vc::Call<void>(item, 0x38, 0);
        item[0x5C] = 1;
        vc::Call<void>(item, 0x14, 0);
        vc::Call<void>(item, 0x78, 1000.0f);
        vc::Call<void>(item, 0xA8, 1);
        if (At<void*>(item, 0x14) != nullptr) {
            Sfx_VoiceStop(At<void*>(item, 0x14), 1);
        }
        vc::Call<void>(item, 4);
        for (int j = 0; j < self->count; ++j) {
            if (item == self->items[j]) {
                vc::Call<void>(item, 0, 1);
                self->freeHint = j;
                self->items[j] = nullptr;
                break;
            }
        }
    }
    return -1;
}

// 0x59E060 (thiscall, ret 0x38): a new item at a position into a free slot (none: null)
uint8_t* __fastcall SfxQueue_Play(SfxQueue* self, re::Edx, int id, int a2, int a3, int a4, int a5, const CVector* pos,
                                  int a7, int a8, int a9, int a10, void* callback, void* user, int curve,
                                  int rolloff) {
    int slot = SfxQueue_FindFreeFn(self);
    if (slot <= -1) {
        return nullptr;
    }
    void* memory = SfxItem_New(0x74);
    if (memory == nullptr) {
        return nullptr;
    }
    uint8_t* item = SFXItem_ConstructAtFn(memory, id, a2, a3, a4, a5, pos, a7, a8, a9, a10, callback, user, curve,
                                          rolloff);
    if (item == nullptr) {
        return nullptr;
    }
    At<SfxQueue*>(item, 0x38) = self;
    self->items[slot] = item;
    return item;
}

// 0x59E0F0 (thiscall, ret 0x34): the same from the play's parameters
uint8_t* __fastcall SfxQueue_PlayWithParams(SfxQueue* self, re::Edx, int id, const SfxPlayParams* params, int a3, int a4,
                                            const CVector* pos, int a6, int a7, int a8, int a9, void* callback,
                                            void* user, int curve, int rolloff) {
    int slot = SfxQueue_FindFreeFn(self);
    if (slot <= -1) {
        return nullptr;
    }
    void* memory = SfxItem_New(0x74);
    if (memory == nullptr) {
        return nullptr;
    }
    uint8_t* item = SFXItem_ConstructWithParamsFn(memory, id, params, a3, a4, pos, a6, a7, a8, a9, callback, user,
                                                  curve, rolloff);
    if (item == nullptr) {
        return nullptr;
    }
    At<SfxQueue*>(item, 0x38) = self;
    self->items[slot] = item;
    return item;
}

// ---- the volume curves and categories ---------------------------------------------------------

inline int __cdecl Sfx_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(uint8_t, __thiscall, Sfx_AreaTransitionFade, 0x00415300, void* transition, float* scale);
GAME_FN(int, __thiscall, Sfx_CameraFadeState, 0x004F12F0, void* camera);
GAME_FN(int32_t, __stdcall, Sfx_CompareDifferences, 0x005A0780, int32_t a, int32_t b, int32_t c, int32_t d);
GAME_FN(void, __cdecl, Sfx_FreeArray, 0x005EEA60, void* p);
GAME_VAR(uint8_t, everythingCloser, 0x00CF6C70);
GAME_VAR(uint8_t, categoriesMuted, 0x00CF6C71);
GAME_VAR(float, cameraFadeAlpha, 0x00C3CD28);
GAME_VAR(int32_t, fadeClockMs, 0x00A14A50);
GAME_ARRAY(uint8_t, categoryFollowsFade, 0x00AE74C8);   // [6]
GAME_ARRAY(float, categoryVolume, 0x00AE74D0);          // [6]
inline void* const areaTransition = reinterpret_cast<void*>(0x00BCC580);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
constexpr uintptr_t kVolumeCurves = 0x00AE6C28;   // 12 of 0xB8 bytes: 0x5A words, a word, the range (+0xB6)

// 0x59E170 (cdecl): a curve's name (either spelling) to its index (unknown: 0; the second
// "large" never reached)
int __cdecl Sfx_CurveFromName(const char* name) {
    struct Entry {
        const char* a;
        const char* b;
        int curve;
    };
    static const Entry kNames[] = {
        {"small", "vt_emittersmall", 1},    {"medium", "vt_emittermed", 2},
        {"large", "vt_emitterlarge", 3},    {"generic", "vt_generic", 0},
        {"speech", "vt_speech", 4},         {"xtralarge", "vt_xtralarge", 5},
        {"belltower", "vt_belltower", 6},   {"medlarge", "vt_emittermedlarge", 7},
        {"large", "vt_jumbo", 8},           {"supersize", "vt_supersize", 9},
        {"genericx3", "vt_genericx3", 10},  {"speechx3", "vt_speechx3", 11},
    };
    for (const Entry& e : kNames) {
        if (Sfx_Stricmp(name, e.a) == 0 || Sfx_Stricmp(name, e.b) == 0) {
            return e.curve;
        }
    }
    return 0;
}

// a fade between two values over a time (0x10)
struct SfxFade {
    float from;        // +0
    float to;          // +4
    int32_t start;     // +8 (0: done)
    int32_t duration;  // +0xC
};
static_assert(sizeof(SfxFade) == 0x10);

// 0x59E3C0 (thiscall)
void __fastcall SfxFade_StartImpl(SfxFade* self, re::Edx, float from, float to, int32_t start, int32_t duration) {
    self->from = from;
    self->to = to;
    self->start = start;
    self->duration = duration;
}

// 0x59E3E0 (cdecl): a curve's volume at a distance (metres; x3 curves and "everything closer"
// a third), interpolated; 90 and beyond 0
int __cdecl Sfx_DistanceVolumeImpl(float distance, int curve) {
    float d = distance;
    if (curve >= 10 || everythingCloser != 0) {
        d = float(double(d) / 3.0);
    }
    int i = int(d);
    if (i >= 0x5A) {
        return 0;
    }
    const uint16_t* words = reinterpret_cast<const uint16_t*>(kVolumeCurves) + (curve * 0x5C + i);
    int a = words[0];
    int diff = a - int(words[1]);
    float frac = d - float(i);
    float step = frac * float(diff);
    float v = float(a) - step;
    return int32_t(int64_t(v));
}

// 0x59E490 (cdecl): each curve's range, its non-zero words (the x3 ones tripled)
void __cdecl Sfx_InitCurveRanges() {
    for (int curve = 0; curve < 12; ++curve) {
        uint16_t* words = reinterpret_cast<uint16_t*>(kVolumeCurves + curve * 0xB8);
        int n = 0;
        while (n < 0x5A && words[n] != 0) {
            ++n;
        }
        words[0x5B] = uint16_t(n);
        if (curve >= 10) {
            words[0x5B] = uint16_t(n * 3);
        }
    }
}

// 0x59E4E0 (cdecl)
float __cdecl Sfx_GetCategoryVolume(int category) {
    return categoryVolume[category];
}

// 0x59E4F0 (cdecl): a volume through its category's (0..5; others and muted: 0) and, for the
// ones that follow it, the screen's fade
float __cdecl Sfx_RolloffImpl(float volume, int category) {
    if (unsigned(category) > 5 || categoriesMuted != 0) {
        return 0.0f;
    }
    float scale = 1.0f;
    if (categoryFollowsFade[category] != 0) {
        bool clamp = true;
        if (Sfx_AreaTransitionFade(areaTransition, &scale) == 0) {
            switch (Sfx_CameraFadeState(theCamera)) {
            case 0:
                scale = 1.0f;
                clamp = false;
                break;
            case 1:
                scale = 0.0f;
                clamp = false;
                break;
            case 2:
                scale = 1.0f - float(double(cameraFadeAlpha) / 255.0);
                break;
            default:
                break;
            }
        }
        if (clamp) {
            if (scale < 0.0f) {
                scale = 0.0f;
            } else if (1.0f < scale) {
                scale = 1.0f;
            }
        }
    }
    float v = categoryVolume[category] * volume;
    return v * scale;
}

// 0x59E5B0 (thiscall, ret 0x14): a fade started and kept (+8)
void __fastcall SfxFadeRef_Start(uint8_t* self, re::Edx, SfxFade* fade, float from, float to, int32_t start,
                                 int32_t duration) {
    if (fade == nullptr) {
        return;
    }
    At<SfxFade*>(self, 8) = fade;
    fade->from = from;
    fade->to = to;
    fade->start = start;
    fade->duration = duration;
}

// 0x59E5E0 (cdecl)
void __cdecl Sfx_SetCategoryFollowsFade(int category, int follows) {
    categoryFollowsFade[category] = uint8_t(follows);
}

// 0x59E5F0 (cdecl)
void __cdecl Sfx_SetCategoryVolume(int category, float volume) {
    categoryVolume[category] = volume;
}

// 0x59E600 (thiscall): the value now (the clock 0xA14A50); 1 when the fade has ended
uint8_t __fastcall SfxFade_ApplyImpl(SfxFade* self, re::Edx, float* out) {
    float span = std::fabs(self->from - self->to);
    if (double(span) < double(9.999999747378752e-05)) {
        *out = self->to;
        self->start = 0;
        return 1;
    }
    int32_t now = fadeClockMs;
    if (now <= self->start) {
        *out = self->from;
        return 0;
    }
    if (now >= self->duration + self->start) {
        *out = self->to;
        self->start = 0;
        return 1;
    }
    float t = float(double(now - self->start) / double(self->duration));
    float range = self->to - self->from;
    float scaled = t * range;
    *out = scaled + self->from;
    return 0;
}

// 0x59E6B0 (thiscall, ret 0x10): a fade on from where it is now toward `to`, its time set for
// the rate (to - from) / duration
void __fastcall SfxFade_Retarget(SfxFade* self, re::Edx, float from, float to, int32_t start, int32_t duration) {
    float current;
    SfxFade_Apply(self, &current);
    self->from = current;
    self->start = start;
    self->to = to;
    float remaining = to - current;
    float rate = float(double(to - from) / double(duration));
    float time = std::fabs(float(remaining / rate));
    self->duration = int32_t(time);
}

// 0x59E710 (thiscall): the kept fade (+8) into +4, dropped when done
void __fastcall SfxFadeRef_Update(uint8_t* self) {
    SfxFade* fade = At<SfxFade*>(self, 8);
    if (fade != nullptr && SfxFade_Apply(fade, reinterpret_cast<float*>(self + 4)) != 0) {
        At<SfxFade*>(self, 8) = nullptr;
    }
}

// ---- the sound objects' base (vtable 0x91F0C4) ----------------------------------------------

// 0x59E730 (thiscall)
uint8_t* __fastcall SoundBase_ConstructDefault(uint8_t* self) {
    At<float>(self, 0xC) = 0.0f;
    At<uint32_t>(self, 0) = 0x0091F0C4;
    At<int32_t>(self, 4) = 0;
    At<int32_t>(self, 8) = 0;
    At<int32_t>(self, 0x10) = 0;
    return self;
}

// 0x59E750 (thiscall)
uint8_t* __fastcall SoundBase_ConstructImpl(uint8_t* self, re::Edx, int32_t a1, float a2, int32_t a3) {
    At<float>(self, 0xC) = a2;
    At<uint32_t>(self, 0) = 0x0091F0C4;
    At<int32_t>(self, 4) = a1;
    At<int32_t>(self, 8) = a3;
    At<int32_t>(self, 0x10) = 0;
    return self;
}

// 0x59E780 (thiscall)
void __fastcall SoundBase_DestructImpl(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x0091F0C4;
}

// 0x59E790 (thiscall)
uint8_t* __fastcall SoundBase_DeletingDestruct(uint8_t* self, re::Edx, uint8_t flags) {
    At<uint32_t>(self, 0) = 0x0091F0C4;
    if ((flags & 1) != 0) {
        Sfx_Free(self);
    }
    return self;
}

// ---- speech helpers -------------------------------------------------------------------------

// 0x59E7B0 (thiscall, ecx unused): two events by their type (bits 17..24 of +0) and their
// channel (+0xC & 3)
int32_t __fastcall SpeechEvent_CompareImpl(void*, re::Edx, const uint32_t* a, const uint32_t* b) {
    return Sfx_CompareDifferences(int32_t(a[0] >> 0x11 & 0xFF), int32_t(a[3] & 3), int32_t(b[0] >> 0x11 & 0xFF),
                                  int32_t(b[3] & 3));
}

// 0x59E7F0 (thiscall): SpeechLib's destructor (vtable 0x9211A8): its buffer (+0xC8C)
void __fastcall SpeechLib_DestructImpl(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x009211A8;
    if (At<void*>(self, 0xC8C) != nullptr) {
        Sfx_Free(At<void*>(self, 0xC8C));
    }
    At<void*>(self, 0xC8C) = nullptr;
}

// 0x59E820 (thiscall): SpeechLib's tables freed: the list at +0xC90 (a count, then 0xC-byte
// entries each owning +8), +0x498 (an array) and +0xC8C
void __fastcall SpeechLib_FreeTables(uint8_t* self) {
    uint32_t* list = At<uint32_t*>(self, 0xC90);
    if (list == nullptr) {
        return;
    }
    if (list[1] != 0) {
        for (uint32_t i = 0; i < At<uint32_t*>(self, 0xC90)[0]; ++i) {
            uint8_t* entries = reinterpret_cast<uint8_t*>(uintptr_t(At<uint32_t*>(self, 0xC90)[1]));
            void* owned = At<void*>(entries, int(i * 0xC + 8));
            if (owned != nullptr) {
                Sfx_Free(owned);
            }
        }
        Sfx_Free(reinterpret_cast<void*>(uintptr_t(At<uint32_t*>(self, 0xC90)[1])));
    }
    Sfx_Free(At<void*>(self, 0xC90));
    At<void*>(self, 0xC90) = nullptr;
    Sfx_FreeArray(At<void*>(self, 0x498));
    At<void*>(self, 0x498) = nullptr;
    Sfx_Free(At<void*>(self, 0xC8C));
    At<void*>(self, 0xC8C) = nullptr;
}

// 0x59E8D0 (thiscall)
void __fastcall SpeechLib_SetFlag4CD(uint8_t* self) {
    At<uint8_t>(self, 0x4CD) = 1;
}

// 0x59E8E0's table: the models (globals) in the order compared; a new group starts at each
// marked one (the game keeps the group's first in a register and compares the other against it)
struct SpeechModelEntry {
    uint32_t address;
    uint8_t startsGroup;
};
constexpr SpeechModelEntry kSpeechModels[] = {
    {0x00BF40BC, 1}, {0x00BF40B8, 0}, {0x00BF40B4, 1}, {0x00BF40B0, 0},
    {0x00BF40AC, 0}, {0x00BF40A8, 1}, {0x00BF42AC, 0}, {0x00BF40A4, 1},
    {0x00BF40A0, 0}, {0x00BF409C, 1}, {0x00BF4098, 0}, {0x00BF3EBC, 1},
    {0x00BF3EB8, 0}, {0x00BF4094, 1}, {0x00BF4090, 0}, {0x00BF408C, 1},
    {0x00BF4088, 0}, {0x00BF4084, 0}, {0x00BF4080, 1}, {0x00BF407C, 0},
    {0x00BF4078, 0}, {0x00BF4074, 1}, {0x00BF4070, 0}, {0x00BF406C, 1},
    {0x00BF4068, 0}, {0x00BF4064, 0}, {0x00BF4058, 1}, {0x00BF405C, 0},
    {0x00BF4060, 0}, {0x00BF4054, 1}, {0x00BF4050, 0}, {0x00BF404C, 1},
    {0x00BF4048, 0}, {0x00BF4044, 0}, {0x00BF4040, 1}, {0x00BF403C, 0},
    {0x00BF4038, 1}, {0x00BF4034, 0}, {0x00BF4030, 1}, {0x00BF402C, 0},
    {0x00BF4028, 1}, {0x00BF4024, 0}, {0x00BF401C, 1}, {0x00BF4018, 0},
    {0x00BF4014, 0}, {0x00BF4010, 0}, {0x00BF400C, 1}, {0x00BF4008, 0},
    {0x00BF4004, 1}, {0x00BF4000, 0}, {0x00BF3FFC, 1}, {0x00BF3FF8, 0},
    {0x00BF3FF4, 0}, {0x00BF3FF0, 1}, {0x00BF3FEC, 0}, {0x00BF3FE8, 1},
    {0x00BF3FE4, 0}, {0x00BF3FE0, 1}, {0x00BF3FDC, 0}, {0x00BF3FD8, 1},
    {0x00BF3FD4, 0}, {0x00BF3FD0, 1}, {0x00BF3FCC, 0}, {0x00BF3FC8, 1},
    {0x00BF3FC4, 0}, {0x00BF3FC0, 1}, {0x00BF3FBC, 0}, {0x00BF3FB8, 1},
    {0x00BF3FB4, 0}, {0x00BF3FB0, 0}, {0x00BF3FAC, 1}, {0x00BF3FA8, 0},
    {0x00BF3FA4, 1}, {0x00BF3FA0, 0}, {0x00BF3F9C, 0}, {0x00BF3F98, 1},
    {0x00BF42A8, 0}, {0x00BF3F94, 1}, {0x00BF3F90, 0}, {0x00BF3F8C, 1},
    {0x00BF3F88, 0}, {0x00BF3F84, 1}, {0x00BF42B4, 0}, {0x00BF3F80, 0},
    {0x00BF3F78, 1}, {0x00BF3F74, 0}, {0x00BF3F70, 1}, {0x00BF3F6C, 0},
    {0x00BF3F68, 1}, {0x00BF3F64, 0}, {0x00BF3F60, 0}, {0x00BF3F5C, 0},
    {0x00BF3F58, 1}, {0x00BF3F54, 0}, {0x00BF3F50, 1}, {0x00BF3F4C, 0},
    {0x00BF3F48, 1}, {0x00BF3F44, 0}, {0x00BF3F40, 0}, {0x00BF3F3C, 1},
    {0x00BF42B0, 0}, {0x00BF3F38, 0}, {0x00BF3F34, 0}, {0x00BF3F30, 0},
    {0x00BF3F2C, 1}, {0x00BF3F28, 0}, {0x00BF3F24, 0}, {0x00BF3F20, 1},
    {0x00BF3F1C, 0}, {0x00BF3F18, 1}, {0x00BF3F14, 0}, {0x00BF3F10, 1},
    {0x00BF3F0C, 0}, {0x00BF3F08, 0}, {0x00BF3F04, 1}, {0x00BF3F00, 0},
    {0x00BF3EFC, 0}, {0x00BF3EF8, 1}, {0x00BF3EF4, 0}, {0x00BF3EF0, 0},
    {0x00BF3EEC, 0}, {0x00BF3EE8, 1}, {0x00BF3EE4, 0}, {0x00BF3EE0, 1},
    {0x00BF3EDC, 0}, {0x00BF3ED8, 0}, {0x00BF3ED4, 1}, {0x00BF3ED0, 0},
    {0x00BF3ECC, 0}, {0x00BF3EC8, 1}, {0x00BF3EC4, 0},
};
static_assert(sizeof(kSpeechModels) / sizeof(kSpeechModels[0]) == 131);

// 0x59E8E0 (cdecl): the same model, or the other being the first of the model's group
uint8_t __cdecl Speech_ModelsMatch(int model, int other) {
    uint8_t a = uint8_t(model);
    uint8_t b = uint8_t(other);
    if (a == b) {
        return 1;
    }
    uint32_t group = 0;
    for (const SpeechModelEntry& e : kSpeechModels) {
        uint32_t value = *reinterpret_cast<const uint32_t*>(uintptr_t(e.address));
        if (e.startsGroup != 0) {
            group = value;
        }
        if (uint32_t(a) == value) {
            return uint32_t(b) == group ? 1 : 0;
        }
    }
    return 0;
}

// ---- registration -----------------------------------------------------------------------------

#define SFXITEM_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(sfxitem_original_, __LINE__);                                    \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(sfxitem_original_, __LINE__), \
             re::Mode::Original)

SFXITEM_REVERSED("Footsteps::QuietArea", 0x0059C7D0, &Sfx_FootstepsQuietImpl);
SFXITEM_REVERSED("Footsteps::SurfaceSound", 0x0059C850, &Sfx_FootstepSurfaceImpl);
SFXITEM_REVERSED("Footsteps::Play", 0x0059CC00, &Sfx_PlayFootstep);
SFXITEM_REVERSED("SfxItemRefs::Add", 0x0059CDF0, &SfxRefs_AddImpl);
SFXITEM_REVERSED("SfxItemRefs::Remove", 0x0059CE40, &SfxRefs_RemoveImpl);
SFXITEM_REVERSED("SFXItem::Destruct", 0x0059CEA0, &SFXItem_DestructImpl);
SFXITEM_REVERSED("SFXItem::Reset", 0x0059CEC0, &SFXItem_Reset);
SFXITEM_REVERSED("SFXItem::IsActive", 0x0059CF10, &SFXItem_IsActiveImpl);
SFXITEM_REVERSED("SFXItem::CollectVoices", 0x0059CF20, &SFXItem_CollectVoices);
SFXITEM_REVERSED("Audio::Approach", 0x0059CF30, &Sfx_ApproachImpl);
SFXITEM_REVERSED("Audio::ApproachF", 0x0059CFC0, &Sfx_ApproachF);
SFXITEM_REVERSED("SFXItem::UpdatePan", 0x0059D030, &SFXItem_UpdatePan);
SFXITEM_REVERSED("SFXItem::SetStopping", 0x0059D100, &SFXItem_SetStopping);
SFXITEM_REVERSED("SfxItemRef::Set", 0x0059D110, &SfxRef_SetImpl);
SFXITEM_REVERSED("SFXItem::CameraDistance", 0x0059D140, &SFXItem_CameraDistance);
SFXITEM_REVERSED("SfxItemRefs::OnItemFreed", 0x0059D1D0, &SfxRefs_OnItemFreed);

SFXITEM_REVERSED("SFXItem::UpdateOcclusion", 0x0059D230, &SFXItem_UpdateOcclusionImpl);
SFXITEM_REVERSED("SFXItem::OcclusionScale", 0x0059D3F0, &SFXItem_OcclusionScale);
SFXITEM_REVERSED("SFXItem::VolumeAt", 0x0059D440, &SFXItem_VolumeAtImpl);
SFXITEM_REVERSED("SFXItem::UpdateVolume", 0x0059D5C0, &SFXItem_UpdateVolume);
SFXITEM_REVERSED("SFXItem::Update", 0x0059D620, &SFXItem_Update);
SFXITEM_REVERSED("SFXItem::ConstructAt", 0x0059D950, &SFXItem_ConstructAt);
SFXITEM_REVERSED("SFXItem::ConstructWithParams", 0x0059DB60, &SFXItem_ConstructWithParams);
SFXITEM_REVERSED("SfxQueue::UpdateAll", 0x0059DE00, &SfxQueue_UpdateAll);
SFXITEM_REVERSED("SfxQueue::Remove", 0x0059DE30, &SfxQueue_RemoveImpl);
SFXITEM_REVERSED("SfxQueue::StopAll", 0x0059DE80, &SfxQueue_StopAllImpl);
SFXITEM_REVERSED("SfxQueue::Construct", 0x0059DEF0, &SfxQueue_ConstructImpl);
SFXITEM_REVERSED("SfxQueue::FindFree", 0x0059DF50, &SfxQueue_FindFree);
SFXITEM_REVERSED("SfxQueue::Play", 0x0059E060, &SfxQueue_Play);
SFXITEM_REVERSED("SfxQueue::PlayWithParams", 0x0059E0F0, &SfxQueue_PlayWithParams);
SFXITEM_REVERSED("Sfx::CurveFromName", 0x0059E170, &Sfx_CurveFromName);
SFXITEM_REVERSED("SfxFade::Start", 0x0059E3C0, &SfxFade_StartImpl);
SFXITEM_REVERSED("Sfx::DistanceVolume", 0x0059E3E0, &Sfx_DistanceVolumeImpl);
SFXITEM_REVERSED("Sfx::InitCurveRanges", 0x0059E490, &Sfx_InitCurveRanges);
SFXITEM_REVERSED("Sfx::GetCategoryVolume", 0x0059E4E0, &Sfx_GetCategoryVolume);
SFXITEM_REVERSED("Sfx::CategoryRolloff", 0x0059E4F0, &Sfx_RolloffImpl);
SFXITEM_REVERSED("SfxFadeRef::Start", 0x0059E5B0, &SfxFadeRef_Start);
SFXITEM_REVERSED("Sfx::SetCategoryFollowsFade", 0x0059E5E0, &Sfx_SetCategoryFollowsFade);
SFXITEM_REVERSED("Sfx::SetCategoryVolume", 0x0059E5F0, &Sfx_SetCategoryVolume);
SFXITEM_REVERSED("SfxFade::Apply", 0x0059E600, &SfxFade_ApplyImpl);
SFXITEM_REVERSED("SfxFade::Retarget", 0x0059E6B0, &SfxFade_Retarget);
SFXITEM_REVERSED("SfxFadeRef::Update", 0x0059E710, &SfxFadeRef_Update);
SFXITEM_REVERSED("SoundBase::ConstructDefault", 0x0059E730, &SoundBase_ConstructDefault);
SFXITEM_REVERSED("SoundBase::Construct", 0x0059E750, &SoundBase_ConstructImpl);
SFXITEM_REVERSED("SoundBase::Destruct", 0x0059E780, &SoundBase_DestructImpl);
SFXITEM_REVERSED("SoundBase::DeletingDestruct", 0x0059E790, &SoundBase_DeletingDestruct);
SFXITEM_REVERSED("SpeechEvent::Compare", 0x0059E7B0, &SpeechEvent_CompareImpl);
SFXITEM_REVERSED("SpeechLib::Destruct", 0x0059E7F0, &SpeechLib_DestructImpl);
SFXITEM_REVERSED("SpeechLib::FreeTables", 0x0059E820, &SpeechLib_FreeTables);
SFXITEM_REVERSED("SpeechLib::SetFlag4CD", 0x0059E8D0, &SpeechLib_SetFlag4CD);
SFXITEM_REVERSED("Speech::ModelsMatch", 0x0059E8E0, &Speech_ModelsMatch);
} // namespace
