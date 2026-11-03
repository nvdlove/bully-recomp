// C3dMarkers (0x5284A0..0x52AE0F): the 35 markers (0xCA0D50, 0x88 each: arrows over
// pickups and destinations, circles and icons on the ground, each maybe with a radar
// blip), the direction arrows stored for one frame (0xCA0650, 32 of 0x38) drawn through
// the render buffer, and the glow beams (0xCA1FE8, 32 of 0x58, pointing up from things).
//
// Found here: a marker's icon types (6..17) aren't meshes but lit-prop records (textured
// quads, 0x4EA700) with the marker's matrix; types 2, 3, 5 and 0x12 draw "pointer.nif"
// facing the camera, bobbing (2..5, 0x12) 0.195 * size up and down every 1024 ms.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Entity.h"
#include "reversed.h"

#include "game/Crt.h"
namespace markers17 {

using col::CVector;
using col::CMatrix;

struct C3dMarker {
    CMatrix m;                // +0
    int8_t type;              // +0x44 -1: none
    uint8_t isUsed;           // +0x45 placed this frame
    uint8_t placePending;     // +0x46 its matrix set: dropped to the ground once collision is in
    uint8_t pad47;
    int32_t blip;             // +0x48 radar blip, -1: none
    uint32_t color;           // +0x4C RGBA bytes
    uint32_t startTime;       // +0x50
    float size;               // +0x54
    uint8_t onGround;         // +0x58 (the line down hit something / a ped holds it)
    uint8_t pad59[3];
    float distToPlayer;       // +0x5C (2D)
    uint8_t unknown60;        // +0x60 (types 5 and 0x12)
    uint8_t pad61;
    int16_t blipSprite;       // +0x62 -1: no blip
    CVector pos;              // +0x64 where it was placed
    CVector dir;              // +0x70
    int16_t blipId;           // +0x7C
    uint8_t pad7E[2];
    uint32_t id;              // +0x80 its owner (an entity or a script id; 0: by position)
    re::Ptr32<void> atomic;             // +0x84 its pointer mesh
};
static_assert(offsetof(C3dMarker, type) == 0x44);
static_assert(offsetof(C3dMarker, blip) == 0x48);
static_assert(offsetof(C3dMarker, distToPlayer) == 0x5C);
static_assert(offsetof(C3dMarker, blipSprite) == 0x62);
static_assert(offsetof(C3dMarker, blipId) == 0x7C);
static_assert(offsetof(C3dMarker, atomic) == 0x84);
static_assert(sizeof(C3dMarker) == 0x88);

struct DirectionArrow {
    CVector pos;              // +0
    CVector v1;               // +0xC
    CVector v2;               // +0x18
    CVector v3;               // +0x24
    float dist;               // +0x30 from the camera
    uint8_t type;             // +0x34
    uint8_t red, green, blue; // +0x35
};
static_assert(sizeof(DirectionArrow) == 0x38);

struct GlowBeam {
    float m[16];              // +0 rows: right, forward, up, position
    re::Ptr32<void> attached;           // +0x40
    uint32_t color;           // +0x44
    float scale;              // +0x48
    float drawDistance;       // +0x4C
    re::Ptr32<void> texture;            // +0x50
    re::Ptr32<void> atomic;             // +0x54
};
static_assert(sizeof(GlowBeam) == 0x58);

// what 0x4EA700 draws (a textured, lit quad)
struct LitPropRecord {
    float m[16];              // +0
    uint32_t unknown40;       // +0x40
    uint32_t color;           // +0x44
    float unknown48;          // +0x48
    float unknown4C;          // +0x4C
    re::Ptr32<void> texture;            // +0x50
    uint32_t unknown54;       // +0x54
};
static_assert(sizeof(LitPropRecord) == 0x58);

struct BufferVertex {
    CVector pos;
    uint32_t unknownC[3];
    uint32_t color;           // +0x18
    float u, v;
};
static_assert(sizeof(BufferVertex) == 0x24);

GAME_ARRAY(C3dMarker, aMarkerArray, 0x00CA0D50);   // 35
GAME_ARRAY(DirectionArrow, ms_directionArrows, 0x00CA0650);
GAME_VAR(int32_t, NumActiveDirectionArrows, 0x00CA0640);
GAME_ARRAY(GlowBeam, ms_glowBeams, 0x00CA1FE8);
GAME_VAR(int32_t, NumGlowBeams, 0x00CA064C);
GAME_VAR(void*, glowBeamAtomic, 0x00CA0634);
GAME_ARRAY(void*, markerAtomics, 0x00CA0638);        // pointer.nif, pickup.nif
GAME_VAR(void*, pointerTexture, 0x00CA0644);
GAME_VAR(uint8_t, pointerTextureSet, 0x00CA0648);
GAME_VAR(void*, defaultPointerTexture, 0x00C66E18);
GAME_ARRAY(void*, markerTextures, 0x00CA05D8);       // 23 (0xCA0630 is 22)
GAME_VAR(void*, radar, 0x00C674CC);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(void*, cameraNode, 0x00C3CCF4);
GAME_VAR(void*, markerNode, 0x01B64884);             // the node the meshes are drawn at
inline void* const renderState = reinterpret_cast<void*>(0x00D140B8);
GAME_VAR(uint8_t, cutsceneRunning, 0x020C5BE4);
GAME_VAR(int32_t, widescreen, 0x00C3CD0C);
GAME_VAR(float, markerRotateX, 0x00A957A0);
GAME_VAR(float, markerBobScale, 0x00A957A4);
GAME_VAR(CEntity*, entityRefC2A8E8, 0x00C2A8E8);
GAME_VAR(int32_t, playerBlipTarget, 0x00BF4334);
inline CVector* const defaultMarkerDir = reinterpret_cast<CVector*>(0x00BCC564);
// the arrows' vertex tables
GAME_ARRAY(float, arrowTable0, 0x00A95560);   // 6
GAME_ARRAY(float, arrowTable1, 0x00A95578);   // 6
GAME_ARRAY(uint16_t, arrowIndices, 0x00A95590);   // 12
GAME_ARRAY(uint16_t, boxIndices, 0x00A95728);     // 36
GAME_ARRAY(float, faceTable0, 0x00A95770);    // 6
GAME_ARRAY(float, faceTable1, 0x00A95788);    // 6
// the insets (0x52AC90)
GAME_ARRAY(float, insetsA, 0x020DCCD4);
GAME_ARRAY(float, insetsB, 0x020DCCE4);

GAME_FN(void, __thiscall, Matrix_SetUnity, 0x00412770, CMatrix* m);
GAME_FN(void, __thiscall, Matrix_SetUp, 0x00413600, CMatrix* m, float x, float y, float z, float unused);
GAME_FN(void, __thiscall, Matrix_RotateZ, 0x00413220, CMatrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_Rotate, 0x00413320, CMatrix* m, const CVector* angles);
GAME_FN(void, __thiscall, Matrix_Scale, 0x004442F0, CMatrix* m, float scale);
GAME_FN(void, __thiscall, Matrix_ToNiTransform, 0x00412AC0, CMatrix* m, void* ni);
GAME_FN(void, __thiscall, Matrix_Assign, 0x00412720, void* m, const CMatrix* from);
GAME_FN(void, __thiscall, Radar_RemoveBlip, 0x0054A940, void* radar, int32_t blip);
GAME_FN(int32_t, __thiscall, Radar_AddBlip, 0x0054A410, void* radar, int type, float x, float y, float z, uint32_t colour, int a,
        int sprite, int arg);
GAME_FN(void, __thiscall, Radar_SetBlipScale, 0x00548BF0, void* radar, int32_t blip, int scale);
GAME_FN(void, __thiscall, Radar_SetBlipId, 0x00548A40, void* radar, int32_t blip, int id);
GAME_FN(void, __thiscall, Radar_SetBlipFlag4, 0x00548C30, void* radar, int32_t blip, int on);
GAME_FN(void, __thiscall, Radar_SetBlip28, 0x00548C80, void* radar, int32_t blip, int value);
GAME_FN(CVector*, __cdecl, FindPlayerCentreOfWorld, 0x004469E0, uintptr_t player);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub2, 0x00414BA0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(int, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void*, __cdecl, Atomic_Clone, 0x006C70D0, void* atomic);
GAME_FN(int, __cdecl, Atomic_Destroy, 0x006C72C0, void* atomic);
GAME_FN(void*, __cdecl, GetFrameOfObject, 0x006C7370, void* object);
GAME_FN(void, __cdecl, Atomic_SetBone, 0x006C6A10, void* atomic, void* bone);
GAME_FN(void, __cdecl, Bone_Destroy, 0x006CA260, void* bone);
GAME_FN(void, __cdecl, RenderAtomic, 0x006C69F0, void* atomic);
GAME_FN(void, __cdecl, UpdateFrame, 0x006C9AF0, void* frame);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);   // NiTransform (rotate[9], translate)
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(void, __cdecl, SetObjectWorldTransform, 0x004068E0, void* object, void* transform);
GAME_FN(void, __cdecl, SetupTreeTexturing, 0x0040A1A0, void* object);
GAME_FN(void, __cdecl, SetTreeColour, 0x004087E0, void* object, uint32_t rgba);
GAME_FN(void, __cdecl, SetTreeFlag800, 0x004089F0, void* object, int on);
GAME_FN(void*, __cdecl, LoadAtomicFile, 0x0042A8E0, const char* path);
GAME_FN(void*, __cdecl, Clump_GetFirstAtomic, 0x005EDFF0, void* clump);
GAME_FN(void, __cdecl, Clump_RemoveAtomic, 0x006C7340, void* clump, void* atomic);
GAME_FN(void, __cdecl, DestroyClump, 0x006C75C0, void* clump);
GAME_FN(void, __thiscall, RenderState_SetAmbientColour, 0x008A20A0, void* state, const void* colour);
GAME_FN(void, __cdecl, AddLitPropRecord, 0x004EA700, LitPropRecord* record);
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine, 0x004629C0, const CVector* point, float zStop, CColPoint* colPoint, re::Ptr32<CEntity>* entity,
        int lists, int arg);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(uint8_t, __cdecl, IsCollisionLoaded, 0x005701C0, const float* xy);
GAME_FN(float, __cdecl, FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int arg);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, Texture_Destroy, 0x005F0F90, void* texture);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, RenderBuffer_StartStoring, 0x0052AE10, int numIndices, int numVerts, re::Ptr32<uint16_t>* indices, re::Ptr32<BufferVertex>* verts,
        int* base);
GAME_FN(void, __cdecl, RenderBuffer_StopStoring, 0x0049A650);
GAME_FN(void, __cdecl, RenderBuffer_ClearRenderBuffer, 0x0052AE80);
GAME_FN(void, __cdecl, RenderBuffer_RenderStuffInBuffer, 0x0052AE90);
GAME_FN(void, __thiscall, Marker_DeleteMarkerObject, 0x005284A0, C3dMarker* marker);
GAME_FN(void, __thiscall, Marker_AddMarker, 0x00528940, C3dMarker* marker, int type, int blipSprite, int unused, float size, int red,
        int green, int blue, uint32_t id, int blipId);
GAME_FN(void, __thiscall, Marker_Render, 0x0052A0E0, C3dMarker* marker);
GAME_FN(void, __cdecl, Marker_SetMatrix, 0x00528E70, C3dMarker* marker, const CVector* pos, const CVector* rot);
GAME_FN(void, __cdecl, Marker_UpdateBlip, 0x00528EF0, C3dMarker* marker, int red, int green, int blue, int sprite, int arg,
        const CVector* pos, int type);
GAME_FN(C3dMarker*, __cdecl, PlaceMarker_Game, 0x0052A600, int type, int blipSprite, int blipArg, CVector* pos, float size,
        const CVector* dir, int blipId, int red, int green, int blue, uint32_t id, int attach, int blipType);
GAME_FN(void, __cdecl, DirectionArrowsDraw_Game, 0x00529390);
GAME_FN(void, __cdecl, Render_Game, 0x0052A4F0);
GAME_FN(void, __cdecl, Init_Game, 0x00528B10);

using re::At;

// ---- C3dMarker -------------------------------------------------------------------------

// 0x5284A0 (thiscall): C3dMarker::DeleteMarkerObject: its blip and mesh gone, reset
void __fastcall DeleteMarkerObject(C3dMarker* self, re::Edx) {
    self->startTime = 0;
    self->isUsed = 0;
    self->placePending = 0;
    if (self->blip >= 0) {
        Radar_RemoveBlip(radar, self->blip);
    }
    self->blip = -1;
    self->type = -1;
    self->pos = CVector{0.0f, 0.0f, 0.0f};
    self->dir = CVector{0.0f, 0.0f, 0.0f};
    self->id = 0;
    if (self->atomic) {
        Atomic_Destroy(self->atomic);
        self->atomic = nullptr;
    }
}

// 0x528940 (thiscall): C3dMarker::AddMarker: its type, color and size (fixed colors and
// alpha for the icon types)
void __fastcall AddMarker(C3dMarker* self, re::Edx, int8_t type, int8_t blipSprite, int unused, float size, uint8_t red,
                          uint8_t green, uint8_t blue, uint32_t id, int16_t blipId) {
    (void)unused;
    self->type = type;
    Matrix_SetUnity(&self->m);
    self->blipSprite = blipSprite;
    self->blipId = blipId;
    self->unknown60 = 0;
    self->id = id;
    uint8_t* c = reinterpret_cast<uint8_t*>(&self->color);
    c[3] = 0xE4;
    auto fixed = [&](uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        self->size = 1.0f;
        c[0] = r;
        c[1] = g;
        c[2] = b;
        c[3] = a;
        self->startTime = timeInMs;
    };
    switch (type) {
    case 1: fixed(0x80, 0x80, 0, 100); return;
    case 5:
    case 0x12: self->unknown60 = 1; break;
    case 6:
    case 0x11: fixed(0x80, 0xFF, 0xFF, 0x40); return;
    case 7:
    case 0xB: fixed(0xFF, 0xFF, 0, 0x40); return;
    case 8: fixed(0, 0x80, 0xFF, 100); return;
    case 9: fixed(0xEF, 0x7F, 0, 100); return;
    case 10: fixed(0xFF, 0, 0, 0x40); return;
    case 0xC: fixed(0, 0xFF, 0, 0x37); return;
    case 0xD:
    case 0xE:
    case 0xF:
    case 0x10: fixed(0xFF, 0x4D, 0, 100); return;
    default: break;
    }
    self->size = size;
    c[0] = red;
    c[1] = green;
    c[2] = blue;
    self->startTime = timeInMs;
}

// 0x528E70: its matrix: upright, turned by rot.z, at pos. (The game also stores
// uninitialised stack into +0x3C; ours leaves it)
void __cdecl SetMarkerMatrix(C3dMarker* marker, const CVector* pos, const CVector* rot) {
    CVector p = *pos;
    Matrix_SetUnity(&marker->m);
    Matrix_SetUp(&marker->m, 0.0f, 0.0f, 1.0f, 0.0f);   // (the 4th float is whatever the stack had)
    Matrix_RotateZ(&marker->m, rot->z);
    marker->m.m[12] = p.x;
    marker->m.m[13] = p.y;
    marker->m.m[14] = p.z;
}

// 0x528EF0: its radar blip made (with a sprite), or its flags updated
void __cdecl UpdateBlip(C3dMarker* marker, uint8_t red, uint8_t green, uint8_t blue, int8_t sprite, int arg, const CVector* pos,
                        int type) {
    if (marker->blip == -1) {
        if (marker->blipSprite == -1) {
            return;
        }
        // (the colour's bytes in memory: r, g, b, 0xFF)
        uint32_t colour = uint32_t(red) | uint32_t(green) << 8 | uint32_t(blue) << 16 | 0xFF000000u;
        marker->blip = Radar_AddBlip(radar, type, pos->x, pos->y, pos->z, colour, 1, sprite, arg);
        Radar_SetBlipScale(radar, marker->blip, 5);
        if (marker->blipId >= 0) {
            Radar_SetBlipId(radar, marker->blip, marker->blipId);
        }
        return;
    }
    if (marker->blipSprite != -1) {
        Radar_SetBlipFlag4(radar, marker->blip, arg);
        Radar_SetBlip28(radar, marker->blip, sprite);
    }
}

// 0x52A0E0 (thiscall): C3dMarker::Render: the pointer (fading in close to the player,
// turned to the camera) or the icon as a lit quad (fading in over 500 ms)
void __fastcall Render(C3dMarker* self, re::Edx) {
    if (self->unknown60 && self->type != 5 && self->type != 0x12) {
        return;
    }
    CVector pos = {self->m.m[12], self->m.m[13], self->m.m[14]};
    uint8_t* camera = GetLocalTransform(GetCameraNode(cameraNode));
    float dx = pos.x - At<float>(camera, 0x24);
    float dy = pos.y - At<float>(camera, 0x28);
    CVector toPlayer;
    CVector* d = VectorSub2(&toPlayer, &pos, FindPlayerCentreOfWorld(0));
    float fade = std::sqrt(std::fabs(d->y * d->y + d->x * d->x));
    if (!(2.5f > fade)) {
        fade = 2.5f;
    }
    fade = fade - 1.5f;
    if (fade < 0.0f) {
        fade = 0.0f;
    } else if (!(fade <= 1.0f)) {
        fade = 1.0f;
    }
    void* texture;
    switch (self->type) {
    case 2:
    case 3:
        self->size = 0.4f;
        [[fallthrough]];
    case 5:
    case 0x12: {
        void* tex = pointerTexture;
        if (!self->atomic) {
            self->atomic = Atomic_Clone(markerAtomics[0]);
        }
        SetRenderState(1, GetRaster(tex));
        float colour[4] = {0.7f, 0.7f, 0.7f, fade};
        Matrix_SetUnity(&self->m);
        Matrix_RotateZ(&self->m, static_cast<float>(std::atan2(static_cast<double>(dy), static_cast<double>(dx))));
        self->m.m[12] = self->m.m[12] + pos.x;
        self->m.m[13] = pos.y + self->m.m[13];
        self->m.m[14] = self->m.m[14] + pos.z;
        Matrix_Scale(&self->m, self->size);
        Matrix_ToNiTransform(&self->m, GetLocalTransform(markerNode));
        UpdateFrame(markerNode);
        Atomic_SetBone(self->atomic, markerNode);
        RenderState_SetAmbientColour(renderState, colour);
        SetObjectWorldTransform(self->atomic, static_cast<uint8_t*>(GetFrameOfObject(self->atomic)) + 0x14);
        SetupTreeTexturing(At<void*>(self->atomic, 0x14));
        SetTreeColour(At<void*>(self->atomic, 0x14), self->color);
        RenderAtomic(self->atomic);
        Atomic_SetBone(self->atomic, nullptr);
        return;
    }
    case 1: texture = markerTextures[10]; break;
    case 6: texture = markerTextures[22]; break;
    case 7: texture = markerTextures[19]; break;
    case 8: texture = markerTextures[18]; break;
    case 9: texture = markerTextures[20]; break;
    case 10: texture = markerTextures[15]; break;
    case 11: texture = markerTextures[16]; break;
    case 12: texture = markerTextures[17]; break;
    case 13: texture = markerTextures[14]; break;
    case 14: texture = markerTextures[13]; break;
    case 15: texture = markerTextures[12]; break;
    case 16: texture = markerTextures[11]; break;
    case 17: texture = markerTextures[21]; break;
    default: return;   // (4 too)
    }
    int elapsed = int(timeInMs - self->startTime);
    float t = float(elapsed);
    if (elapsed < 0) {
        t = t + 4294967296.0f;
    }
    t = t / 500.0f;
    if (t >= 1.0f) {
        t = 1.0f;
    }
    LitPropRecord record;
    record.unknown40 = 0;
    record.color = self->color;
    reinterpret_cast<uint8_t*>(&record.color)[3] = uint8_t(int(float(int(reinterpret_cast<uint8_t*>(&self->color)[3])) * t));
    Matrix_Assign(&record, &self->m);
    record.texture = texture;
    record.unknown54 = 0;
    record.unknown48 = 1.0f;
    record.unknown4C = 40.0f;
    AddLitPropRecord(&record);
}

// ---- C3dMarkers ------------------------------------------------------------------------

// 0x528540: C3dMarkers::Shutdown
void __cdecl Shutdown() {
    for (int i = 0; i < 35; i++) {
        Marker_DeleteMarkerObject(&aMarkerArray[i]);
    }
    auto destroy = [](re::Ptr32<void>& atomic) {
        if (atomic) {
            void* frame = GetFrameOfObject(atomic);
            Atomic_SetBone(atomic, nullptr);
            Atomic_Destroy(atomic);
            if (frame) {
                Bone_Destroy(frame);
            }
            atomic = nullptr;
        }
    };
    destroy(markerAtomics[0]);
    destroy(markerAtomics[1]);
    destroy(glowBeamAtomic);
    for (int i = 0; i < NumGlowBeams; i++) {
        if (ms_glowBeams[i].atomic) {
            Atomic_SetBone(ms_glowBeams[i].atomic, nullptr);
            Atomic_Destroy(ms_glowBeams[i].atomic);
            ms_glowBeams[i].atomic = nullptr;
        }
    }
    for (int i = 22; i >= 0; i--) {
        Texture_Destroy(markerTextures[i]);
    }
    pointerTexture = nullptr;
    pointerTextureSet = 0;
}

// 0x528750: within 150 of the player (2D)
uint32_t __cdecl IsNearPlayer(const CVector* pos) {
    CVector player = *FindPlayerCentreOfWorld(0);
    float dy = pos->y - player.y;
    float dx = pos->x - player.x;
    return dx * dx + dy * dy < 22500.0f ? 1 : 0;
}

// 0x5287B0: the markers owned by `id` let go (not drawn again)
void __cdecl ForgetMarkers(uint32_t id) {
    if (!id) {
        return;
    }
    for (int i = 0; i < 35; i++) {
        if (aMarkerArray[i].id && id == aMarkerArray[i].id) {
            aMarkerArray[i].id = 0;
            aMarkerArray[i].isUsed = 0;
        }
    }
}

// 0x528800: C3dMarkers::DirectionArrowSet: stored for this frame when within 60 of the
// camera (the distance is written into the next slot either way)
void __cdecl DirectionArrowSet(float x, float y, float z, float v1x, float v1y, float v1z, float v2x, float v2y, float v2z, float v3x,
                               float v3y, float v3z, uint8_t type, uint8_t red, uint8_t green, uint8_t blue) {
    if (NumActiveDirectionArrows >= 32) {
        return;
    }
    CVector pos = {x, y, z};
    CVector tmp;
    CVector* d = VectorSub(&tmp, &pos, reinterpret_cast<CVector*>(GetLocalTransform(GetCameraNode(cameraNode)) + 0x24));
    float dist = std::sqrt(std::fabs(d->y * d->y + d->x * d->x + d->z * d->z));
    DirectionArrow& arrow = ms_directionArrows[NumActiveDirectionArrows];
    arrow.dist = dist;
    if (dist <= 60.0f) {
        arrow.pos = pos;
        arrow.v1 = CVector{v1x, v1y, v1z};
        arrow.v2 = CVector{v2x, v2y, v2z};
        arrow.v3 = CVector{v3x, v3y, v3z};
        arrow.type = type;
        arrow.red = red;
        arrow.green = green;
        arrow.blue = blue;
        NumActiveDirectionArrows++;
    }
}

static void* LoadMeshAtomic(const char* path) {
    void* clump = LoadAtomicFile(path);
    void* atomic = Clump_GetFirstAtomic(clump);
    SetTreeFlag800(At<void*>(atomic, 0x14), 1);
    Clump_RemoveAtomic(clump, atomic);
    Atomic_SetBone(atomic, nullptr);
    DestroyClump(clump);
    return atomic;
}

// 0x528B10: C3dMarkers::Init: the markers reset, the meshes and textures loaded
void __cdecl Init() {
    for (int i = 0; i < 35; i++) {
        C3dMarker& marker = aMarkerArray[i];
        marker.size = 1.0f;
        marker.pos.x = 0.0f;
        At<float>(&marker, 0x5C) = 0.0f;
        marker.dir.x = 0.0f;
        marker.dir.y = 0.0f;
        marker.pos.y = 0.0f;
        marker.type = -1;
        marker.isUsed = 0;
        marker.placePending = 0;
        marker.blip = -1;
        marker.color = 0xFFFFFFFF;
        marker.startTime = 0;
        marker.id = 0;
        marker.pos.z = 0.0f;
        marker.dir.z = 0.0f;
        marker.blipId = -1;
        marker.atomic = nullptr;
    }
    if (!pointerTextureSet) {
        pointerTexture = defaultPointerTexture;
        pointerTextureSet = 1;
    }
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(Txd_FindTxdSlot("HUDElems", 0));
    PushMemoryTag(7);
    if (!markerAtomics[0]) {
        markerAtomics[0] = LoadMeshAtomic("Models\\pointer.nif");
    }
    if (!markerAtomics[1]) {
        markerAtomics[1] = LoadMeshAtomic("Models\\pickup.nif");
    }
    PopMemoryTag();
    Txd_SetCurrentTxd(Txd_FindTxdSlot("EffectTextures", 0));
    PushMemoryTag(7);
    glowBeamAtomic = LoadMeshAtomic("Models\\GlowBeam.nif");
    for (int i = 0; i < 32; i++) {
        ms_glowBeams[i].atomic = nullptr;
    }
    static const char* const names[23] = {"ActivePed", "ActivePed", "ActivePed", "ActivePed", "ActivePed", "C_Sleep",
                                          "C_Square",  "Locker_Unlocked", "Locker_Locked", "HandLogo", "C_Common", "C_Lawn",
                                          "C_Spray",   "C_Snow", "C_Punish", "C_Common", "C_Common", "C_Common", "C_Common",
                                          "C_Common",  "C_Common", "C_Hair", "C_Clothes"};
    for (int i = 22; i >= 0; i--) {
        markerTextures[i] = Txd_FindTexture(names[i], nullptr);
    }
    PopMemoryTag();
    Txd_PopCurrentTxd();
}

// 0x528FE0: the glow beams within their draw distance, as meshes at their matrices
void __cdecl RenderGlowBeams() {
    int zWrite, alphaEnable, zTest, cull;
    GetRenderState(0xC, &alphaEnable);
    GetRenderState(6, &zTest);
    GetRenderState(8, &zWrite);
    GetRenderState(0xE, &cull);
    SetRenderState(0xC, 1);
    SetRenderState(6, 1);
    SetRenderState(8, 0);
    SetRenderState(0xE, 0);
    float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    RenderState_SetAmbientColour(renderState, white);
    float* transform = reinterpret_cast<float*>(GetLocalTransform(markerNode));
    CVector cam = *reinterpret_cast<CVector*>(GetLocalTransform(GetCameraNode(cameraNode)) + 0x24);
    for (int i = 0; i < NumGlowBeams; i++) {
        GlowBeam& beam = ms_glowBeams[i];
        CVector pos = {beam.m[12], beam.m[13], beam.m[14]};
        float dx = pos.x - cam.x;
        float dy = pos.y - cam.y;
        float dz = pos.z - cam.z;
        float dist = std::sqrt(std::fabs(dy * dy + dx * dx + dz * dz));
        if (!(dist < beam.drawDistance)) {
            continue;
        }
        SetRenderState(1, GetRaster(beam.texture));
        float s = beam.scale;
        transform[9] = beam.m[4] * 0.01f + pos.x;
        transform[10] = beam.m[5] * 0.01f + pos.y;
        transform[11] = beam.m[6] * 0.01f + pos.z;
        transform[0] = beam.m[0] * s;
        transform[3] = beam.m[1] * s;
        transform[6] = beam.m[2] * s;
        transform[1] = beam.m[4] * s;
        transform[4] = beam.m[5] * s;
        transform[7] = beam.m[6] * s;
        float ns = -s;
        transform[2] = ns * beam.m[8];
        transform[5] = ns * beam.m[9];
        transform[8] = ns * beam.m[10];
        if (!beam.atomic) {
            beam.atomic = Atomic_Clone(glowBeamAtomic);
            if (!beam.atomic) {
                continue;
            }
        }
        UpdateFrame(markerNode);
        SetObjectWorldTransform(beam.atomic, transform);
        SetupTreeTexturing(At<void*>(beam.atomic, 0x14));
        SetTreeColour(At<void*>(beam.atomic, 0x14), beam.color);
        RenderAtomic(beam.atomic);
    }
    SetRenderState(0xE, cull);
    SetRenderState(0xC, alphaEnable);
    SetRenderState(6, zTest);
    SetRenderState(8, zWrite);
}

// 0x529380: C3dMarkers::ReInit
void __cdecl ReInit() {
    NumActiveDirectionArrows = 0;
    Init_Game();
}

static float ArrowNoise() {
    float t = float(Rand()) / 32767.0f;
    t = t * 0.02f;
    return t - 0.01f;
}

// 0x529390: C3dMarkers::DirectionArrowsDraw: this frame's arrows through the render
// buffer (flat arrows, jittering boxes, camera-facing quads), then none stored
void __cdecl DirectionArrowsDraw() {
    if (NumActiveDirectionArrows == 0) {
        return;
    }
    int srcBlend, destBlend, alphaEnable, zTest, zWrite;
    GetRenderState(10, &srcBlend);
    GetRenderState(11, &destBlend);
    GetRenderState(0xC, &alphaEnable);
    GetRenderState(6, &zTest);
    GetRenderState(8, &zWrite);
    SetRenderState(0xC, 1);
    SetRenderState(8, 0);
    SetRenderState(6, 1);
    SetRenderState(10, 5);
    SetRenderState(11, 6);
    SetRenderState(1, 0);
    RenderBuffer_ClearRenderBuffer();
    uint8_t r = 0, g = 0, b = 0;   // (kept from the previous arrow for other types; never used then)
    for (int i = 0; i < NumActiveDirectionArrows; i++) {
        DirectionArrow& arrow = ms_directionArrows[i];
        float flicker = float(Rand() & 0xFF) * 0.2f;
        uint8_t type = arrow.type;
        switch (type) {
        case 1: g = 0xFF; r = uint8_t(int(flicker)); b = r; break;
        case 2: r = 0xFF; g = 0x80; b = uint8_t(int(flicker)); break;
        case 3: r = 0xFF; g = uint8_t(int(flicker)); b = g; break;
        case 4: case 5: case 6: case 7: r = g = b = 0xFF; break;
        case 8: case 9: case 10: case 11: r = 0xFF; g = uint8_t(int(flicker)); b = g; break;
        case 12: r = arrow.red; g = arrow.green; b = arrow.blue; break;
        default: break;
        }
        uint8_t alpha;
        if (45.0f <= arrow.dist) {
            alpha = uint8_t(int((1.0f - (arrow.dist - 45.0f) * 0.06666667f) * 255.0f));
        } else {
            alpha = 0xFF;
        }
        if (type >= 4 && type <= 11 && arrow.dist < 40.0f) {
            float q = arrow.dist / 40.0f;
            alpha = uint8_t(int(q * (254.0f * q) + 1.0f));
        }
        uint32_t color = uint32_t(alpha) << 24 | uint32_t(r) << 16 | uint32_t(g) << 8 | b;
        re::Ptr32<uint16_t> indices;
        re::Ptr32<BufferVertex> verts;
        int base;
        const float* t0;
        const float* t1;
        const float* t2;
        switch (type) {
        case 1:
        case 2:
        case 3:
            RenderBuffer_StartStoring(12, 6, &indices, &verts, &base);
            for (int k = 0; k < 6; k++) {
                verts[k].color = color;
                verts[k].pos.x = (arrowTable1[k] * arrow.v1.x + arrow.pos.x) + arrowTable0[k] * arrow.v2.x;
                verts[k].pos.y = (arrowTable1[k] * arrow.v1.y + arrow.pos.y) + arrowTable0[k] * arrow.v2.y;
                verts[k].pos.z = (arrowTable1[k] * arrow.v1.z + arrow.pos.z) + arrowTable0[k] * arrow.v2.z;
            }
            for (int k = 0; k < 12; k++) {
                indices[k] = uint16_t(arrowIndices[k] + base);
            }
            break;
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11: {
            int set = (type - 4) % 4;
            t0 = reinterpret_cast<const float*>(0x00A955A8) + set * 0x18;
            t2 = t0 + 8;
            t1 = t0 + 16;
            RenderBuffer_StartStoring(0x24, 8, &indices, &verts, &base);
            for (int k = 0; k < 8; k++) {
                verts[k].color = color;
                float n = ArrowNoise();
                verts[k].pos.x = ((n + arrow.pos.x) + t1[k] * arrow.v1.x + t0[k] * arrow.v2.x) + arrow.v3.x * t2[k];
                n = ArrowNoise();
                verts[k].pos.y = ((arrow.pos.y + n) + t1[k] * arrow.v1.y + t0[k] * arrow.v2.y) + arrow.v3.y * t2[k];
                n = ArrowNoise();
                verts[k].pos.z = ((arrow.pos.z + n) + t1[k] * arrow.v1.z + t0[k] * arrow.v2.z) + arrow.v3.z * t2[k];
            }
            for (int k = 0; k < 36; k++) {
                indices[k] = uint16_t(boxIndices[k] + base);
            }
            break;
        }
        case 12:
            RenderBuffer_StartStoring(12, 6, &indices, &verts, &base);
            for (int k = 0; k < 6; k++) {
                verts[k].color = color;
                // (the camera's transform fetched for every value)
                float r1 = At<float>(GetLocalTransform(GetCameraNode(cameraNode)), 4);
                float r0 = At<float>(GetLocalTransform(GetCameraNode(cameraNode)), 0);
                verts[k].pos.x = (r1 * faceTable1[k] + arrow.pos.x) + faceTable0[k] * r0;
                float r4 = At<float>(GetLocalTransform(GetCameraNode(cameraNode)), 0x10);
                float r3 = At<float>(GetLocalTransform(GetCameraNode(cameraNode)), 0xC);
                verts[k].pos.y = (r4 * faceTable1[k] + arrow.pos.y) + faceTable0[k] * r3;
                float r7 = At<float>(GetLocalTransform(GetCameraNode(cameraNode)), 0x1C);
                float r6 = At<float>(GetLocalTransform(GetCameraNode(cameraNode)), 0x18);
                verts[k].pos.z = (r7 * faceTable1[k] + arrow.pos.z) + r6 * faceTable0[k];
            }
            for (int k = 0; k < 12; k++) {
                indices[k] = uint16_t(arrowIndices[k] + base);
            }
            break;
        default:
            continue;
        }
        RenderBuffer_StopStoring();
    }
    RenderBuffer_RenderStuffInBuffer();
    SetRenderState(0xC, alphaEnable);
    SetRenderState(6, zTest);
    SetRenderState(8, zWrite);
    SetRenderState(10, srcBlend);
    SetRenderState(11, destBlend);
    NumActiveDirectionArrows = 0;
}

// 0x52A4F0: C3dMarkers::Render: the markers placed this frame drawn (within 150 of the
// player, not in cutscenes or widescreen), the others deleted
void __cdecl RenderMarkers() {
    int srcBlend, destBlend, alphaEnable, zTest, zWrite;
    GetRenderState(10, &srcBlend);
    GetRenderState(11, &destBlend);
    GetRenderState(0xC, &alphaEnable);
    GetRenderState(6, &zTest);
    GetRenderState(8, &zWrite);
    SetRenderState(6, 1);
    SetRenderState(8, 1);
    SetRenderState(0xC, 1);
    SetRenderState(10, 5);
    SetRenderState(11, 6);
    for (int i = 0; i < 35; i++) {
        C3dMarker& marker = aMarkerArray[i];
        if (!marker.isUsed) {
            Marker_DeleteMarkerObject(&marker);
            continue;
        }
        if (!cutsceneRunning && widescreen == 0 && marker.distToPlayer < 150.0f) {
            Marker_Render(&marker);
        }
        marker.isUsed = 0;
    }
    SetRenderState(0xC, alphaEnable);
    SetRenderState(6, zTest);
    SetRenderState(8, zWrite);
    SetRenderState(10, srcBlend);
    SetRenderState(11, destBlend);
}

static bool IsIconType(int8_t type) {
    return type == 9 || type == 6 || type == 1 || type == 7 || type == 8 || type == 12 || type == 11 || type == 10 || type == 13 ||
           type == 14 || type == 15 || type == 16 || type == 17;
}

// 0x52A600: C3dMarkers::PlaceMarker: this frame's marker of `id` (or at this position
// and direction), made in a free slot when there's none; its blip kept up. nullptr
// when there's no room
C3dMarker* __cdecl PlaceMarker(int8_t type, int8_t blipSprite, int blipArg, CVector* pos, float size, const CVector* dir, int16_t blipId,
                               uint8_t red, uint8_t green, uint8_t blue, uint32_t id, uint8_t attach, int blipType) {
    bool bobs = type == 3 || type == 2 || type == 5 || type == 0x12 || type == 4;
    if (bobs) {
        pos->z = pos->z + 0.75f;
    }
    C3dMarker* marker = nullptr;
    if (id) {
        for (int i = 0; i < 35; i++) {
            if (aMarkerArray[i].id && id == aMarkerArray[i].id && type == aMarkerArray[i].type) {
                marker = &aMarkerArray[i];
                break;
            }
        }
    } else {
        for (int i = 0; i < 35; i++) {
            C3dMarker& m = aMarkerArray[i];
            if (pos->x == m.pos.x && pos->y == m.pos.y && pos->z == m.pos.z && dir->x == m.dir.x && dir->y == m.dir.y &&
                dir->z == m.dir.z) {
                marker = &m;
                break;
            }
        }
    }
    if ((type == 12 || type == 11 || type == 10) && id && attach) {
        *pos = reinterpret_cast<CEntity*>(id)->GetPosition();
    }
    if (!marker) {
        // (the first free unused one; else the last unused one seen)
        for (int i = 0; i < 35; i++) {
            if (!aMarkerArray[i].isUsed) {
                marker = &aMarkerArray[i];
                if (aMarkerArray[i].type == -1) {
                    break;
                }
            }
        }
        if (!marker) {
            return nullptr;
        }
        Marker_DeleteMarkerObject(marker);
        Marker_AddMarker(marker, type, blipSprite, blipArg, size, red, green, blue, id, blipId);
        marker->onGround = 1;
        if ((IsIconType(type) || marker->unknown60) && type != 5 && type != 0x12) {
            marker->placePending = 1;
            Marker_SetMatrix(marker, pos, dir);
            if (blipSprite != -1) {
                Marker_UpdateBlip(marker, red, green, blue, blipSprite, blipArg, pos, blipType);
            }
        } else if (type == 2) {
            CVector angles = {markerRotateX, dir->x * 3.14159274f / 180.0f, 0.0f};
            Matrix_Rotate(&marker->m, &angles);
        } else if (bobs) {
            marker->size = size;
        }
        // (+0x3C gets uninitialised stack in the game; ours leaves it)
        marker->m.m[12] = pos->x;
        marker->m.m[13] = pos->y;
        marker->m.m[14] = pos->z;
        marker->pos = *pos;
        marker->dir = *dir;
        if (id && attach) {
            marker->onGround = (reinterpret_cast<CEntity*>(id)->typeAndStatus & 7) == 3;
        } else {
            CColPoint colPoint;
            colPoint.point = CVector{0.0f, 0.0f, 0.0f};
            colPoint.entity = nullptr;
            colPoint.normal = CVector{0.0f, 0.0f, 0.0f};
            colPoint.depth = 3.40282347e+38f;
            re::Ptr32<CEntity> hit = reinterpret_cast<CEntity*>(id);
            marker->onGround = ProcessVerticalLine(pos, pos->z - 2.0f, &colPoint, &hit, 4, 0);
            if (colPoint.entity) {
                CleanUpOldReference(colPoint.entity, &colPoint.entity);   // (the same whether it's 0xC2A8E8's or not)
            }
        }
    }
    CVector player = *FindPlayerCentreOfWorld(0);
    float dy = pos->y - player.y;
    float dx = pos->x - player.x;
    marker->distToPlayer = std::sqrt(std::fabs(dx * dx + dy * dy));
    marker->blipSprite = blipSprite;
    if (bobs) {
        marker->size = size;
        int t = int((timeInMs - marker->startTime) & 0x3FF);
        float phase = float(t) * 0.0009765625f * (markerBobScale + markerBobScale);
        float s = static_cast<float>(std::sin(static_cast<double>(phase)));
        pos->z = s * (size * 0.195f) + pos->z;
        marker->m.m[12] = pos->x;
        marker->m.m[13] = pos->y;
        marker->m.m[14] = pos->z;
    }
    if (marker->placePending && marker->distToPlayer < 100.0f) {
        float xy[2] = {pos->x, pos->y};
        if (IsCollisionLoaded(xy)) {
            float z = FindGroundZFor3DCoord(pos->x, pos->y, pos->z + 1.0f, nullptr, 1);
            if (z != 0.0f) {
                marker->m.m[14] = z;
            }
            marker->placePending = 0;
        }
    }
    if (blipSprite == -1) {
        if (marker->blip >= 0) {
            Radar_RemoveBlip(radar, marker->blip);
            marker->blip = -1;
        }
    } else {
        Marker_UpdateBlip(marker, red, green, blue, blipSprite, blipArg, pos, blipType);
    }
    marker->isUsed = 1;
    return marker;
}

// 0x52ABE0: a destination marker (type 2) over pos; yellow for the player's target,
// blue with a blip sprite otherwise
void __cdecl PlaceDestinationMarker(const CVector* pos, int target, uint32_t id) {
    bool other = target != playerBlipTarget;
    uint8_t r, g, b;
    if (!other) {
        r = 0xFF;
        g = 0xFF;
        b = 0;
    } else {
        r = 0;
        g = 0x72;
        b = 0xBC;
    }
    CVector p = *pos;
    PlaceMarker_Game(2, other ? 1 : 0, 0, &p, 0.5f, defaultMarkerDir, -1, r, g, b, id, 1, 4);
}

// 0x52AC70: the arrows, then the markers unless `skipMarkers`
void __cdecl RenderAll(uint8_t skipMarkers) {
    DirectionArrowsDraw_Game();
    if (!skipMarkers) {
        Render_Game();
    }
}

// 0x52AC90: two rectangles' edges moved half a pixel in (not below 0)
void __cdecl SetInsetRects(float a0, float a1, float a2, float a3, float b0, float b1, float b2, float b3) {
    auto clamp = [](float v) { return 0.0f <= v ? v : 0.0f; };
    insetsA[0] = clamp(a0 + 0.5f);
    insetsA[1] = clamp(a1 + 0.5f);
    insetsA[2] = clamp(a2 - 0.5f);
    insetsA[3] = clamp(a3 + 0.5f);
    insetsB[0] = clamp(b0 + 0.5f);
    insetsB[1] = clamp(b1 - 0.5f);
    insetsB[2] = clamp(b2 - 0.5f);
    insetsB[3] = clamp(b3 - 0.5f);
}

} // namespace markers17

// ---- registration ----------------------------------------------------------------------

#define MARKERS17_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace markers17 {
MARKERS17_REVERSED("C3dMarker::DeleteMarkerObject", 0x005284A0, &DeleteMarkerObject);
MARKERS17_REVERSED("C3dMarkers::Shutdown", 0x00528540, &Shutdown);
MARKERS17_REVERSED("C3dMarkers::IsNearPlayer", 0x00528750, &IsNearPlayer);
MARKERS17_REVERSED("C3dMarkers::ForgetMarkers", 0x005287B0, &ForgetMarkers);
MARKERS17_REVERSED("C3dMarkers::DirectionArrowSet", 0x00528800, &DirectionArrowSet);
MARKERS17_REVERSED("C3dMarker::AddMarker", 0x00528940, &AddMarker);
MARKERS17_REVERSED("C3dMarkers::Init", 0x00528B10, &Init);
MARKERS17_REVERSED("C3dMarker::SetMatrix", 0x00528E70, &SetMarkerMatrix);
MARKERS17_REVERSED("C3dMarker::UpdateBlip", 0x00528EF0, &UpdateBlip);
MARKERS17_REVERSED("C3dMarkers::RenderGlowBeams", 0x00528FE0, &RenderGlowBeams);
MARKERS17_REVERSED("C3dMarkers::ReInit", 0x00529380, &ReInit);
MARKERS17_REVERSED("C3dMarkers::DirectionArrowsDraw", 0x00529390, &DirectionArrowsDraw);
MARKERS17_REVERSED("C3dMarker::Render", 0x0052A0E0, &Render);
MARKERS17_REVERSED("C3dMarkers::Render", 0x0052A4F0, &RenderMarkers);
MARKERS17_REVERSED("C3dMarkers::PlaceMarker", 0x0052A600, &PlaceMarker);
MARKERS17_REVERSED("C3dMarkers::PlaceDestinationMarker", 0x0052ABE0, &PlaceDestinationMarker);
MARKERS17_REVERSED("C3dMarkers::RenderAll", 0x0052AC70, &RenderAll);
MARKERS17_REVERSED("SetInsetRects_52AC90", 0x0052AC90, &SetInsetRects);
} // namespace markers17
