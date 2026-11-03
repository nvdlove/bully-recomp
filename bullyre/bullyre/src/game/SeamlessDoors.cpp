// Seamless doors (the remaster's, bullyre_x64; runflags.h). Step 1: no black at a door. While the
// player is near a door, what is behind it is kept loaded, and when the door is used after that
// has gone on for long enough the transition goes on without its fade to black
// (AreaTransitionManager2.cpp's state 4): the door opens, the area's scripts load (the area's
// script signals when: AreaSignalAreaTransitionReadyToLoad), and the player is through.
//   Into an interior: it's loaded beside the current area with the game's own "extra scene"
// (WorldStreamArea: its placement, collision and models, the area counted as current by
// rendering, streaming, population, props and shadows; scripts use it through
// AreaAddExtraScene, and one of theirs is left alone). The interiors are built away from the
// town (x -400 to -800), so the camera outside never sees one.
//   Out into the town: the town around the place outside is the extra scene's point for
// placement and collision only (not counted as current: its distant buildings would be drawn
// around the interior); its buildings', objects' and dummies' models there are asked for, so
// they stay loaded however long the player is inside (a loaded model asked for again is the most
// recently used, the last to be removed).
//   Through the door (SeamlessDoorSwitch, at the switch): the area left becomes the extra scene
// at once, the door back its doorway, so what the camera (still behind the player, on the old
// side) sees goes on being drawn around the doorway (bullygfx's PORTAL_AROUND) until the camera
// is through as well; the camera isn't pulled in by the walls meanwhile (SeamlessCameraFree).
//   The first frame in a new area is seen now: the light manager hands out the interior's lights
// at once rather than in the next frame's update.
//
// With --doors the doors of each area the player arrives in are listed (stderr), and
// AreaTransitionManager::Update times each transition's states. The area transition manager
// (0xBCC580; AreaTransitionManager.cpp has its fields): +8 the count and +0x10 [100] {trigger,
// route} of the current area's doors, +0x330 a flag each (0 walked into, 1 a door used); a route:
// +8 its id (its trigger's +0x2C), +0xC the area it leads to, +0x18 its flags; its first point the
// place there (+0..+8) and the heading (+0x1C, degrees: the game takes 90 off). A trigger: +4 its
// position, +0x1C the door prop, +0x38 its heading (radians), +0x3D its area.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include <windows.h>

#include "game/VCall.h"
#include "reversed.h"
#include "runflags.h"

namespace seamless {

using re::At;

GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_ARRAY(const char*, ms_areaNames, 0x00903058);   // [64]
GAME_FN(uint8_t*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(void, __thiscall, ATM_SetTarget, 0x004152B0, void* self, void* target);
GAME_VAR(uint8_t*, PED_player, 0x00C1AEA8);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
// WorldStreamArea (WorldStreamArea.cpp): Set(x, y, z, placement, collision, models, area, radius)
GAME_FN(void, __cdecl, ExtraScene_Set, 0x004298E0, float x, float y, float z, int placement, int collision, int models, int area,
        float radius);
GAME_FN(void, __cdecl, ExtraScene_Clear, 0x00429940);
GAME_FN(void, __cdecl, ExtraScene_RequestModels, 0x00429990);
GAME_VAR(uint8_t, sa_active, 0x00BD09EC);
GAME_VAR(uint8_t, sa_placement, 0x00BD09EA);   // (IplStore's extra point: its placement loaded)
GAME_VAR(int8_t, sa_area, 0x00BD09EB);
GAME_VAR(float, sa_x, 0x00BD09F0);
GAME_VAR(float, sa_y, 0x00BD09F4);
GAME_FN(uint8_t*, __cdecl, GetLightManager, 0x0040F7A0);
GAME_FN(void, __thiscall, LightManager_Update, 0x004107F0, void* self);
GAME_FN(uint8_t*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, uint32_t slot);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int model, int flags);
GAME_FN(int, __cdecl, Streaming_GetInfoIndex, 0x0052DC90, int model);
GAME_FN(void, __cdecl, SetModelIsDeletable, 0x0052FE40, int model);
inline uint8_t* const ms_aInfoForModel = reinterpret_cast<uint8_t*>(0x00CA39B8);   // 0x10 each: +4 its state, +5 its flags
GAME_VAR(uint32_t*, ms_pPtrNodes, 0x00C0F788);
inline uint8_t* const theSectors = reinterpret_cast<uint8_t*>(0x00C1B178);   // CWorld's 36 x 36, 5 lists each (0x14)
inline uint8_t* const theTransitionManager = reinterpret_cast<uint8_t*>(0x00BCC580);
// the cameras and the world drawn from another (as CReflectionManager::Render does for a mirror)
GAME_VAR(uint8_t*, ms_pMainRwCamera, 0x00C1AE7C);   // the main camera (an NiCamera underneath)
GAME_VAR(uint8_t*, ms_pTheCameraRwCamera, 0x00C3CCF4);
GAME_VAR(float, ms_fov, 0x00C3CD04);
GAME_VAR(float, ms_fovOverride, 0x00C3CDD0);
GAME_VAR(int32_t, ms_bFovOverride, 0x00C3CDD4);
GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_FN(void*, __cdecl, RwCameraCreate, 0x006C99E0);
GAME_FN(void*, __cdecl, RwFrameCreate, 0x006CA210);
GAME_FN(void, __cdecl, RwCameraSetFrame, 0x006C8720, void* camera, void* frame);
GAME_FN(void, __cdecl, RwCameraSetProjection, 0x006C8320, void* camera, int projection);
GAME_FN(void*, __cdecl, RwCameraGetRaster, 0x006C8740, void* camera);
GAME_FN(void, __cdecl, RwCameraSetRaster, 0x006C8750, void* camera, void* raster);
GAME_FN(uint8_t*, __cdecl, RwCameraGetFrame, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, RwFrameGetLocalTransform, 0x006C9A90, void* frame);
GAME_FN(const float*, __cdecl, RwFrameGetLTM, 0x006CA0A0, void* frame);
GAME_FN(void, __cdecl, RwFrameUpdateObjects, 0x006C9AF0, void* frame);
GAME_FN(void, __cdecl, RwCameraSetFarClipPlane, 0x006C8780, void* camera, float distance);
GAME_FN(void, __cdecl, RwCameraSetNearClipPlane, 0x006C8820, void* camera, float distance);
GAME_FN(const float*, __cdecl, RwCameraGetViewWindow, 0x006C8310, void* camera);
GAME_FN(void, __cdecl, RwCameraSetFogDistance, 0x006C8300, void* camera, int distance);
GAME_VAR(int16_t, ms_fogStart, 0x020C4822);
GAME_VAR(int16_t, ms_farClip, 0x020C4820);   // (the time cycle's, as MainLoop sets the main camera's)
// the sky (MdBullyMainViewRenderClick::RenderBackground, `this` unused) and what its domes move by
GAME_FN(void, __thiscall, MainViewClick_RenderBackground, 0x00515E20, void* self, void* camera);
GAME_VAR(float, skyDomeRotation, 0x00C6603C);
GAME_VAR(float, cloudDomeDrift, 0x00A55178);
GAME_FN(void, __cdecl, RwCameraSetViewWindow, 0x006C8920, void* camera, const float* window);
GAME_FN(void*, __cdecl, RwCameraBeginUpdate, 0x006C86A0, void* camera);
GAME_FN(void, __cdecl, RwCameraEndUpdate, 0x006C86C0, void* camera);
GAME_FN(float*, __thiscall, NiTransform_Multiply, 0x00406830, const float* self, float* out, const float* other);
GAME_FN(void, __thiscall, Camera_SetRwCamera, 0x004F1800, void* camera, void* rwCamera);
GAME_FN(void, __thiscall, NiCamera_SetViewFrustum, 0x00762D60, void* camera, const float* frustum);
GAME_FN(void, __thiscall, Camera_UpdateFrustumPlanes, 0x004F1D10, void* camera);
GAME_FN(void, __thiscall, Camera_Update_4F1F10, 0x004F1F10, void* camera);
GAME_FN(float, __thiscall, Camera_GetAspectRatio, 0x004F1360, void* camera);
GAME_FN(void, __cdecl, Renderer_ConstructRenderList, 0x00453A00);
GAME_FN(void, __cdecl, Renderer_PreRender, 0x00451530);
GAME_FN(void, __cdecl, Renderer_5EF890, 0x005EF890);
GAME_FN(void, __cdecl, Renderer_RenderWorld, 0x0043AED0);   // (the alpha lists started, the visible list drawn but roads)
GAME_FN(void, __cdecl, RwCamera_SetView, 0x005EF8A0, void* camera, uintptr_t rect, float tanHalfFov, float aspect);
GAME_FN(void, __cdecl, SetRenderWareCamera, 0x005F2EB0, void* camera);
GAME_FN(void, __cdecl, DefinedState, 0x005EE1D0);
GAME_FN(void, __cdecl, TimeCycle_Update, 0x006B0780);
GAME_FN(uint8_t*, __cdecl, NiIsKindOf, 0x00401FF0, void* rttiInfo, void* object);
inline void* const NiGeometry_RTTI = reinterpret_cast<void*>(0x020E25B4);
inline void* const NiNode_RTTI = reinterpret_cast<void*>(0x020E2548);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(void, __cdecl, SetupFade, 0x0040AAA0, void* object);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E240, void* self);
GAME_FN(void, __thiscall, MdBullyCullingProcess_Cull, 0x00515A10, void* self, void* camera, void* scene, void* accumulator,
        void* nearList);
GAME_FN(void, __cdecl, Renderer_SetCamera, 0x00407550, void* camera);   // (the renderer's camera data, when it can draw)
GAME_ARRAY(uint8_t*, rs_aVisibleEntityPtrs, 0x00C11FC0);   // CRenderer's visible list
GAME_VAR(int32_t, rs_nNoOfVisibleEntities, 0x00C13F00);
GAME_VAR(uint8_t, rs_bCameraInInterior, 0x00C936FA);   // (the main camera in an interior box: the scan kept to its sectors, heights)
GAME_ARRAY(float, rs_vecCameraPosition, 0x00C13F0C);

const char* AreaName(int area) {
    if (area < 0 || area >= 64 || ms_areaNames[area] == nullptr) {
        return "?";
    }
    return ms_areaNames[area];
}

constexpr double kNear = 18.0;          // (from the door's trigger, flat)
constexpr double kNearHeight = 6.0;
constexpr float kRadius = 60.0f;        // (around the place inside: the interior's sectors)
constexpr float kTownRadius = 80.0f;    // (around the place outside)
constexpr uint32_t kReadyMs = 1000;     // (loading this long: the door needs no fade)
constexpr uint32_t kModelsEveryMs = 500;

// what is kept loaded behind the door the player is near
struct Preload {
    bool on = false;
    int area = -1;          // (0: the town, its placement, collision and models; an interior: all of it)
    float x = 0, y = 0, z = 0;   // (the door's route's place there: which door)
    uint32_t since = 0;
    uint32_t modelsAt = 0;
    bool scene = false;     // (the extra scene ours, around (sx, sy))
    float sx = 0, sy = 0, sz = 0;
    bool adopt = false;     // (made at a switch, for the door back: its route's place not known yet)
    int from = -1;          // (the area the player was in when it was made)
};
Preload g_preload;
bool g_goingIn = false;   // (a door's transition going on without its fade)

// A door's two sides as one doorway (step 2: the other side drawn through it). Each side's
// trigger is where its door is (the door prop's position and heading); the door model's box
// gives the doorway's width (its x, from the hinge), height and middle. "In" is the way
// through to the other side; the turn and the move map this side's doorway onto the other's.
struct Doorway {
    bool ok = false;
    float here[3];     // this side's doorway: its middle, on the floor
    float there[3];    // the other side's
    float along[2];    // across this side's doorway
    float in[2];       // through it
    float thereIn[2];  // and on the other side (into the area there)
    float width = 0, height = 0;
    float cosTurn = 1, sinTurn = 0;
    uint8_t* door = nullptr;   // (this side's door prop)
    float hinge[3];            // this side's trigger (its door's hinge), and the other side's
    float thereHinge[3];
    float out[3];              // where the other side's door leads back to, on this side (its route's place)
};
Doorway g_doorway;

// After a door taken without a fade, until the camera, behind the player, is through too: the
// area left drawn around the doorway, the camera not kept out of walls (the new area's, which it
// is outside of), and in the main view this side's door (shut: just made) and, in an interior,
// what is past its doorway (the street's backdrop) left out.
struct Reverse {
    bool on = false;
    uint32_t until = 0;
    std::vector<uint8_t*> hidden;
    std::vector<int16_t> pinned;   // (the side left's models kept in memory meanwhile: GAME_REQUIRED)
    uint32_t since = 0;            // (the switch)
};
Reverse g_reverse;
constexpr uint32_t kReverseMs = 60000;   // (at most: it ends when the camera is through, or the player has gone on)
// The main camera as the last frame drew it (its frame's LTM; its direction): the switch carries
// that through the doorway (by the switch, in the transition's update, the camera's frame has been
// moved: to the player's head)
float g_drawnLtm[13];
float g_drawnDir[3];
float g_drawnFov = 0;   // (and its fov: the area switch puts the camera's back to the default)
bool g_drawn = false;
// (--doors) each frame's camera traced from a door's use until a little after the switch
uint32_t g_traceUntil = 0;
// at the switch, where the camera was, carried through (for the camera controller's look-at)
bool g_cameraPending = false;
float g_cameraThrough[3];
// the portal camera set up by a pass (its view window, near and far: the second population pass's
// "in view" uses it), and the switch of a seamless door going on (the peds kept: both sides')
bool g_portalCameraReady = false;
bool g_keepPeds = false;

// The other side drawn through a door: an interior from anywhere, the town from an interior.
bool PortalArea(int area) {
    return area == 0 ? currArea > 1 : area > 1;
}

// the extra scene for what is behind a door: an interior counted as current (drawn, peopled,
// streamed), the town's placement and collision loaded around the place only
void SetScene(int area, const float* at) {
    if (area != 0) {
        ExtraScene_Set(at[0], at[1], at[2], 1, 1, 1, area, kRadius);
    } else {
        ExtraScene_Set(at[0], at[1], at[2], 1, 1, 0, 0, kTownRadius);
        sa_active = 0;
    }
    g_preload.scene = true;
    g_preload.sx = at[0];
    g_preload.sy = at[1];
    g_preload.sz = at[2];
}

// (our extra scene as we left it: not a script's)
bool OurScene() {
    const Preload& p = g_preload;
    if (!p.scene || sa_area != p.area || sa_x != p.sx || sa_y != p.sy) {
        return false;
    }
    return p.area != 0 ? sa_active != 0 : (sa_active == 0 && sa_placement != 0);
}

void Unhide() {
    for (uint8_t* e : g_reverse.hidden) {
        At<uint32_t>(e, 0xD0) = 0;   // (CEntity::bDontRender)
    }
    g_reverse.hidden.clear();
}

void EndReverse() {
    if (!g_reverse.on) {
        return;
    }
    Unhide();
    for (int16_t model : g_reverse.pinned) {
        SetModelIsDeletable(model);
    }
    g_reverse = Reverse{};
    if (re::DoorReport()) {
        std::fprintf(stderr, "doors: the camera is through at %u ms\n", m_snTimeInMilliseconds);
    }
}

// the door nearest the player that leads to another area (its route, or null)
uint8_t* NearestDoor(uint8_t* atm, const float* at) {
    uint8_t* best = nullptr;
    double bestDistance = kNear * kNear;
    for (int i = 0; i < At<int32_t>(atm, 8) && i < 100; i++) {
        uint8_t* trigger = At<uint8_t*>(atm, 0x10 + i * 8);
        uint8_t* route = At<uint8_t*>(atm, 0x14 + i * 8);
        if (!trigger || !route) {
            continue;
        }
        int area = int(int8_t(route[0xC]));
        if (area == 1 || area == currArea) {   // ("everywhere", or here: nothing to load)
            continue;
        }
        double dx = At<float>(trigger, 4) - at[0], dy = At<float>(trigger, 8) - at[1], dz = At<float>(trigger, 0xC) - at[2];
        if (dz < -kNearHeight || dz > kNearHeight) {
            continue;
        }
        if (dx * dx + dy * dy < bestDistance) {
            bestDistance = dx * dx + dy * dy;
            best = route;
        }
    }
    return best;
}

const float* EntityPosition(uint8_t* e) {
    uint8_t* matrix = At<uint8_t*>(e, 0x14);
    return matrix ? reinterpret_cast<const float*>(matrix + 0x30) : &At<float>(e, 4);
}

int SectorIndex(double v) {
    int i = int(v / 50.0 + 18.0);
    return i < 0 ? 0 : i > 35 ? 35 : i;
}

// each entity in the sectors around (x, y) within r of it, flat
template <typename Fn>
void ForEntitiesNear(float x, float y, double r, Fn&& fn) {
    for (int sy = SectorIndex(y - r); sy <= SectorIndex(y + r); sy++) {
        for (int sx = SectorIndex(x - r); sx <= SectorIndex(x + r); sx++) {
            re::Ptr32<uint32_t>* lists = reinterpret_cast<re::Ptr32<uint32_t>*>(theSectors + (sy * 36 + sx) * 0x14);
            for (int list = 0; list < 5; list++) {
                for (uint32_t* node = lists[list]; node;) {
                    uint32_t bits = *node;
                    uint32_t next = bits >> 18;
                    node = next == 0x3FFF ? nullptr : &ms_pPtrNodes[next];
                    uint8_t* e = GetEntityInPool(bits & 0xF, (bits >> 4) & 0x3FFF);
                    if (!e) {
                        continue;
                    }
                    const float* p = EntityPosition(e);
                    double dx = p[0] - x, dy = p[1] - y;
                    if (dx * dx + dy * dy <= r * r) {
                        fn(e);
                    }
                }
            }
        }
    }
}

// the town's buildings, objects and dummies within kTownRadius of (x, y): their models asked for
void RequestTownModels(float x, float y) {
    ForEntitiesNear(x, y, kTownRadius, [](uint8_t* e) {
        int type = e[0x108] & 7;   // (1 a building, 4 an object, 5 a dummy; not peds or vehicles)
        if ((type == 1 || type == 4 || type == 5) && e[0x111] <= 1) {
            RequestModel(At<int16_t>(e, 0x10E), 0);
        }
    });
}

// (--doors) the entities around a door: what its frame can be taken from
void ReportEntitiesNear(const char* what, const float* at, double r) {
    std::fprintf(stderr, "doors:   near %s (%.2f, %.2f, %.2f):\n", what, at[0], at[1], at[2]);
    ForEntitiesNear(at[0], at[1], r, [&](uint8_t* e) {
        const float* p = EntityPosition(e);
        uint8_t* matrix = At<uint8_t*>(e, 0x14);
        double heading = matrix ? std::atan2(-double(At<float>(matrix, 0x10)), double(At<float>(matrix, 0x14))) : At<float>(e, 0x10);
        int16_t model = At<int16_t>(e, 0x10E);
        uint8_t* col = model >= 0 && ms_modelInfoPtrs[model] ? At<uint8_t*>(ms_modelInfoPtrs[model], 0xC) : nullptr;
        float box[6] = {};
        if (col) {
            for (int i = 0; i < 3; i++) {
                box[i] = At<float>(col, 0x10 + i * 4);
                box[3 + i] = At<float>(col, 0x20 + i * 4);
            }
        }
        std::fprintf(stderr,
                     "doors:     type %d model %d area %d at (%.2f, %.2f, %.2f) heading %.1f%s rw %d box (%.2f, %.2f, %.2f)..(%.2f, %.2f, %.2f)\n",
                     e[0x108] & 7, model, e[0x111], p[0], p[1], p[2], heading * 57.29577951308232, matrix ? "" : " (placement)",
                     At<uint32_t>(e, 0x18) != 0, box[0], box[1], box[2], box[3], box[4], box[5]);
    });
}

// (--doors) a door's two sides: its trigger here, the trigger on the other side whose way back
// comes out near it, and the entities around each
void ReportDoorPair(uint8_t* atm, uint8_t* route) {
    uint8_t* here = nullptr;
    for (int i = 0; i < At<int32_t>(atm, 8) && i < 100; i++) {
        if (At<uint8_t*>(atm, 0x14 + i * 8) == route) {
            here = At<uint8_t*>(atm, 0x10 + i * 8);
        }
    }
    if (!here) {
        return;
    }
    const float* at = &At<float>(here, 4);
    ReportEntitiesNear("this side's trigger", at, 3.0);
    int area = int(int8_t(route[0xC]));
    for (int i = 0; i < At<int32_t>(atm, 8) && i < 100; i++) {
        uint8_t* trigger = At<uint8_t*>(atm, 0x10 + i * 8);
        uint8_t* back = At<uint8_t*>(atm, 0x14 + i * 8);
        if (!trigger || !back || int(int8_t(back[0xC])) != currArea || trigger == here) {
            continue;
        }
        const float* out = reinterpret_cast<const float*>(Route_First(back));
        if (!out) {
            continue;
        }
        double dx = out[0] - at[0], dy = out[1] - at[1];
        if (dx * dx + dy * dy < 25.0) {
            std::fprintf(stderr,
                         "doors:   the other side (area %d, this one's %d): trigger at (%.2f, %.2f, %.2f) heading %.1f, its way back out at"
                         " (%.2f, %.2f, %.2f)\n",
                         trigger[0x3D], area, At<float>(trigger, 4), At<float>(trigger, 8), At<float>(trigger, 0xC),
                         double(At<float>(trigger, 0x38)) * 57.29577951308232, out[0], out[1], out[2]);
            ReportEntitiesNear("the other side's trigger", &At<float>(trigger, 4), 3.0);
        }
    }
}

// a door's frame from its trigger: its middle (the model's box across from the hinge), across, and
// the normal through it (either way)
void DoorFrame(const float* p, float h, float middle, float* at, float* along, float* normal) {
    along[0] = float(std::cos(double(h)));
    along[1] = float(std::sin(double(h)));
    normal[0] = -along[1];
    normal[1] = along[0];
    at[0] = p[0] + middle * along[0];
    at[1] = p[1] + middle * along[1];
    at[2] = p[2];
}

// The doors seen in the areas' lists so far: a trigger (where its door is, its heading) and where
// its route goes. An interior's own doors are only in the list while it's the current area, so
// its side of a door is known from an earlier visit (or the table a test tour leaves):
// bullyre_x64_doorways.txt beside the exe, a line a door: x y z heading area x y z.
// (a ninth field since 2026-11-02: the door's own heading, as CreateAreaProps turns its prop:
// the trigger's +0x10 (degrees) less its +0x38 (radians). The trigger's +0x38 alone can be the
// other way round, the leaf then on the other side of the hinge)
struct KnownDoor {
    float at[3];
    float heading;
    int to;
    float out[3];
    float doorHeading = 0;
    bool hasDoorHeading = false;
};
std::vector<KnownDoor> g_known;
constexpr const char* kKnownFile = "bullyre_x64_doorways.txt";

// a door's own heading from its trigger (radians)
float DoorHeading(uint8_t* trigger) {
    return float(double(At<float>(trigger, 0x10)) * 0.017453292519943295 - double(At<float>(trigger, 0x38)));
}

void LoadKnownDoors() {
    static bool loaded = false;
    if (loaded) {
        return;
    }
    loaded = true;
    if (FILE* f = std::fopen(kKnownFile, "r")) {
        char line[256];
        while (std::fgets(line, sizeof line, f)) {
            KnownDoor k;
            int n = std::sscanf(line, "%f %f %f %f %d %f %f %f %f", &k.at[0], &k.at[1], &k.at[2], &k.heading, &k.to, &k.out[0], &k.out[1],
                                &k.out[2], &k.doorHeading);
            if (n >= 8) {
                k.hasDoorHeading = n == 9;
                g_known.push_back(k);
            }
        }
        std::fclose(f);
    }
}

void SaveKnownDoors() {
    if (FILE* f = std::fopen(kKnownFile, "w")) {
        for (const KnownDoor& k : g_known) {
            std::fprintf(f, "%.3f %.3f %.3f %.5f %d %.3f %.3f %.3f", k.at[0], k.at[1], k.at[2], k.heading, k.to, k.out[0], k.out[1], k.out[2]);
            if (k.hasDoorHeading) {
                std::fprintf(f, " %.5f", k.doorHeading);
            }
            std::fprintf(f, "\n");
        }
        std::fclose(f);
    }
}

// the current list's doors noted (new ones, and the door's heading of those noted without it:
// the table written again then)
void NoteDoors(uint8_t* atm) {
    LoadKnownDoors();
    bool changed = false;
    for (int i = 0; i < At<int32_t>(atm, 8) && i < 100; i++) {
        uint8_t* trigger = At<uint8_t*>(atm, 0x10 + i * 8);
        uint8_t* route = At<uint8_t*>(atm, 0x14 + i * 8);
        const float* out = route ? reinterpret_cast<const float*>(Route_First(route)) : nullptr;
        if (!trigger || !out) {
            continue;
        }
        KnownDoor k = {{At<float>(trigger, 4), At<float>(trigger, 8), At<float>(trigger, 0xC)}, At<float>(trigger, 0x38),
                       int(int8_t(route[0xC])), {out[0], out[1], out[2]}, DoorHeading(trigger), true};
        bool seen = false;
        for (KnownDoor& o : g_known) {
            if (o.to == k.to && std::fabs(o.at[0] - k.at[0]) < 0.05f && std::fabs(o.at[1] - k.at[1]) < 0.05f) {
                seen = true;
                if (!o.hasDoorHeading) {
                    o.doorHeading = k.doorHeading;
                    o.hasDoorHeading = true;
                    changed = true;
                }
            }
        }
        if (!seen) {
            g_known.push_back(k);
            changed = true;
        }
    }
    if (changed) {
        SaveKnownDoors();
    }
}

// the doorway of the door whose route is `route` (this side's trigger: the one on it)
Doorway FindDoorway(uint8_t* atm, uint8_t* route) {
    Doorway d;
    uint8_t* here = nullptr;
    for (int i = 0; i < At<int32_t>(atm, 8) && i < 100; i++) {
        if (At<uint8_t*>(atm, 0x14 + i * 8) == route) {
            here = At<uint8_t*>(atm, 0x10 + i * 8);
        }
    }
    uint8_t* door = here ? At<uint8_t*>(here, 0x1C) : nullptr;   // (the door prop)
    const float* inside = reinterpret_cast<const float*>(Route_First(route));
    if (!door || !inside) {
        return d;
    }
    int16_t model = At<int16_t>(door, 0x10E);
    uint8_t* col = model >= 0 && ms_modelInfoPtrs[model] ? At<uint8_t*>(ms_modelInfoPtrs[model], 0xC) : nullptr;
    if (!col) {
        return d;
    }
    float x0 = At<float>(col, 0x10), x1 = At<float>(col, 0x20), height = At<float>(col, 0x28);
    // (a double door: its other leaf is another door prop at the same place, hinged the other way)
    const float* hinge = EntityPosition(door);
    ForEntitiesNear(hinge[0], hinge[1], 0.5, [&](uint8_t* e) {
        const float* at = EntityPosition(e);
        int16_t m = At<int16_t>(e, 0x10E);
        uint8_t* c = m >= 0 && ms_modelInfoPtrs[m] ? At<uint8_t*>(ms_modelInfoPtrs[m], 0xC) : nullptr;
        if (e == door || (e[0x108] & 7) != 6 || !c || std::fabs(at[0] - hinge[0]) > 0.05f || std::fabs(at[1] - hinge[1]) > 0.05f) {
            return;
        }
        x0 = std::min(x0, At<float>(c, 0x10));
        x1 = std::max(x1, At<float>(c, 0x20));
        height = std::max(height, At<float>(c, 0x28));
    });
    d.width = x1 - x0;
    d.height = height;
    float middle = (x0 + x1) * 0.5f;
    // the other side: the trigger whose way back comes out by this door
    const float* p = &At<float>(here, 4);
    uint8_t* there = nullptr;
    const float* outside = nullptr;
    for (int i = 0; i < At<int32_t>(atm, 8) && i < 100; i++) {
        uint8_t* trigger = At<uint8_t*>(atm, 0x10 + i * 8);
        uint8_t* back = At<uint8_t*>(atm, 0x14 + i * 8);
        if (!trigger || !back || trigger == here || int(int8_t(back[0xC])) != currArea) {
            continue;
        }
        // (its way back out near this door, the trigger itself near the way in there; the nearest)
        const float* out = reinterpret_cast<const float*>(Route_First(back));
        const float* t = &At<float>(trigger, 4);
        float outBy = out ? (out[0] - p[0]) * (out[0] - p[0]) + (out[1] - p[1]) * (out[1] - p[1]) : 1e9f;
        float by = (t[0] - inside[0]) * (t[0] - inside[0]) + (t[1] - inside[1]) * (t[1] - inside[1]);
        if (outBy < 8.0f * 8.0f && by < 15.0f * 15.0f && (!there || outBy < (outside[0] - p[0]) * (outside[0] - p[0]) + (outside[1] - p[1]) * (outside[1] - p[1]))) {
            there = trigger;
            outside = out;
        }
    }
    // (this side's door prop itself, when it has a matrix: its x axis)
    float hereHeading = DoorHeading(here);
    if (uint8_t* m = At<uint8_t*>(door, 0x14)) {
        float fromProp = float(std::atan2(double(At<float>(m, 4)), double(At<float>(m, 0))));
        if (re::DoorReport()) {
            std::fprintf(stderr, "doors: door heading: the prop's %.1f, from its trigger %.1f (the trigger's own %.1f)\n", fromProp * 57.29578f,
                         hereHeading * 57.29578f, At<float>(here, 0x38) * 57.29578f);
        }
        hereHeading = fromProp;
    }
    float turned = hereHeading - At<float>(here, 0x38);   // (this side's door against its trigger's heading)
    float therePos[3] = {0, 0, 0}, thereHeading = 0;
    if (there) {
        std::memcpy(therePos, &At<float>(there, 4), sizeof(therePos));
        thereHeading = DoorHeading(there);
    } else {
        // (not in the list: an interior's own door, known from an earlier visit)
        LoadKnownDoors();
        float best = 8.0f * 8.0f;
        for (const KnownDoor& k : g_known) {
            float outBy = (k.out[0] - p[0]) * (k.out[0] - p[0]) + (k.out[1] - p[1]) * (k.out[1] - p[1]);
            float by = (k.at[0] - inside[0]) * (k.at[0] - inside[0]) + (k.at[1] - inside[1]) * (k.at[1] - inside[1]);
            if (k.to == currArea && outBy < best && by < 15.0f * 15.0f) {
                best = outBy;
                std::memcpy(therePos, k.at, sizeof(therePos));
                thereHeading = k.hasDoorHeading ? k.doorHeading : k.heading + turned;
                outside = k.out;
            }
        }
        if (!outside) {
            return d;
        }
    }
    float normal[2], thereAlong[2], thereNormal[2];
    DoorFrame(&At<float>(here, 4), hereHeading, middle, d.here, d.along, normal);
    DoorFrame(therePos, thereHeading, middle, d.there, thereAlong, thereNormal);   // (the same door model inside)
    // in: from the way back out towards the door here, from the door towards the way in there
    float s1 = (d.here[0] - outside[0]) * normal[0] + (d.here[1] - outside[1]) * normal[1] < 0.0f ? -1.0f : 1.0f;
    d.in[0] = normal[0] * s1;
    d.in[1] = normal[1] * s1;
    float s2 = (inside[0] - d.there[0]) * thereNormal[0] + (inside[1] - d.there[1]) * thereNormal[1] < 0.0f ? -1.0f : 1.0f;
    float* thereIn = d.thereIn;
    thereIn[0] = thereNormal[0] * s2;
    thereIn[1] = thereNormal[1] * s2;
    double turn = std::atan2(double(thereIn[1]), double(thereIn[0])) - std::atan2(double(d.in[1]), double(d.in[0]));
    d.cosTurn = float(std::cos(turn));
    d.sinTurn = float(std::sin(turn));
    d.door = door;
    std::memcpy(d.hinge, p, sizeof(d.hinge));
    std::memcpy(d.thereHinge, therePos, sizeof(d.thereHinge));
    std::memcpy(d.out, outside, sizeof(d.out));
    d.ok = true;
    return d;
}

// the world through the doorway: a point and a direction here, as they are on the other side
void ThroughPoint(const Doorway& d, const float* p, float* out) {
    float x = p[0] - d.here[0], y = p[1] - d.here[1];
    out[0] = d.cosTurn * x - d.sinTurn * y + d.there[0];
    out[1] = d.sinTurn * x + d.cosTurn * y + d.there[1];
    out[2] = p[2] - d.here[2] + d.there[2];
}

// The shaders' fog constants (BullyMain::UpdateFogConstants, 0x4013C0, which takes the camera in
// esi): the time cycle's fog colour and the camera's fog range. The frame sets them once, from
// the main camera: a pass through a doorway has the other area's own.
void FogConstantsFor(uint8_t* camera) {
#if defined(_M_IX86)
    __asm {
        mov esi, camera
        mov eax, 0x004013C0
        call eax
    }
#else
    re::Ours<void (*)(uint8_t*)>(0x004013C0)(camera);
#endif
}

// a camera's frame (its LTM, an NiTransform: rotation rows, position, scale) moved through the
// doorway: turned and moved as ThroughPoint moves a point
void MoveThrough(const Doorway& d, const float* ltm, float* moved) {
    float turn[13] = {d.cosTurn, -d.sinTurn, 0.0f, d.sinTurn, d.cosTurn, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    float origin[3] = {0.0f, 0.0f, 0.0f};
    float moved0[3];
    ThroughPoint(d, origin, moved0);
    turn[9] = moved0[0];
    turn[10] = moved0[1];
    turn[11] = moved0[2];
    NiTransform_Multiply(turn, moved, ltm);
}

// a camera's frame set to an LTM
void SetCameraFrame(uint8_t* camera, const float* ltm) {
    uint8_t* frame = RwCameraGetFrame(camera);
    std::memcpy(RwFrameGetLocalTransform(frame), ltm, 13 * sizeof(float));
    std::memcpy(frame + 0x48, ltm, 13 * sizeof(float));
    frame[3] = uint8_t((frame[3] & 0xFB) | 8);
    RwFrameUpdateObjects(frame);
}

// the camera that draws the other side (a mirror's, CReflectionManager::CreateCamera: the main
// camera's raster and view window)
uint8_t* PortalCamera() {
    static uint8_t* camera = nullptr;
    if (!camera) {
        camera = static_cast<uint8_t*>(RwCameraCreate());
        RwCameraSetFrame(camera, RwFrameCreate());
        RwCameraSetProjection(camera, 1);
        RwCameraSetRaster(camera, RwCameraGetRaster(ms_pTheCameraRwCamera));
    }
    return camera;
}

// bullygfx64's doorway (portal.rs; mode kIn: the other side drawn in it, kAround: the area the
// camera is still in drawn around it, kReady: only asked whether it would be): 0 when its renderer
// isn't drawing
constexpr uint32_t kIn = 0, kAround = 1, kReady = 2;
int SendDoorway(const float* corners, const float* camera, uint32_t mode = kIn) {
    using Fn = int(__cdecl*)(const float*, const float*);
    using Fn2 = int(__cdecl*)(const float*, const float*, uint32_t);
    static Fn send = nullptr;
    static Fn2 send2 = nullptr;
    static bool looked = false;
    if (!looked) {
        looked = true;
#if defined(_WIN64)
        HMODULE gfx = GetModuleHandleA("bullygfx64.dll");
#else
        HMODULE gfx = GetModuleHandleA("d3d9.dll");
#endif
        send = gfx ? reinterpret_cast<Fn>(reinterpret_cast<void*>(GetProcAddress(gfx, "bullygfx_portal"))) : nullptr;
        send2 = gfx ? reinterpret_cast<Fn2>(reinterpret_cast<void*>(GetProcAddress(gfx, "bullygfx_portal2"))) : nullptr;
    }
    if (send2) {
        return send2(corners, camera, mode);
    }
    return send && mode == kIn ? send(corners, camera) : 0;
}

// the doorway's corners: `back` past its plane (through it), a little bigger than it
void DoorwayCorners(const Doorway& d, float back, float* corners) {
    float half = d.width * 0.5f + 0.03f;
    float at[2] = {d.here[0] + d.in[0] * back, d.here[1] + d.in[1] * back};
    const float across[4][2] = {{-1, 0}, {1, 0}, {1, 1}, {-1, 1}};
    for (int i = 0; i < 4; i++) {
        corners[i * 3] = at[0] + across[i][0] * half * d.along[0];
        corners[i * 3 + 1] = at[1] + across[i][0] * half * d.along[1];
        corners[i * 3 + 2] = d.here[2] - 0.02f + across[i][1] * (d.height + 0.05f);
    }
}

// a camera as bullygfx takes it (NiCamera: the world rotation's columns direction, up, right at
// +0x60; the position at +0x84; the frustum at +0x104)
void CameraOf(uint8_t* camera, float* out) {
    for (int i = 0; i < 3; i++) {
        out[i] = At<float>(camera, 0x84 + i * 4);
        out[3 + i] = At<float>(camera, 0x60 + i * 0xC);
        out[6 + i] = At<float>(camera, 0x64 + i * 0xC);
        out[9 + i] = At<float>(camera, 0x68 + i * 0xC);
    }
    for (int i = 0; i < 6; i++) {
        out[12 + i] = At<float>(camera, 0x104 + i * 4);
    }
}

// the door props at a hinge (a double door's two leaves)
template <typename Fn>
void ForDoorsAt(const float* hinge, Fn&& fn) {
    ForEntitiesNear(hinge[0], hinge[1], 0.5, [&](uint8_t* e) {
        const float* at = EntityPosition(e);
        if ((e[0x108] & 7) == 6 && std::fabs(at[0] - hinge[0]) < 0.1f && std::fabs(at[1] - hinge[1]) < 0.1f &&
            std::fabs(at[2] - hinge[2]) < 1.0f) {
            fn(e);
        }
    });
}

// (the reverse) this side's door props and, in an interior, what is past its doorway, left out of
// the main view
void HideForReverse() {
    const Doorway& d = g_doorway;
    Unhide();
    auto hide = [](uint8_t* e) {
        if (At<uint32_t>(e, 0xD0) == 0) {
            At<uint32_t>(e, 0xD0) = 1;
            g_reverse.hidden.push_back(e);
        }
    };
    ForDoorsAt(d.hinge, hide);
    if (d.door) {
        hide(d.door);
    }
    if (currArea > 1) {
        ForEntitiesNear(d.here[0], d.here[1], 40.0, [&](uint8_t* e) {
            const float* p = EntityPosition(e);
            int type = e[0x108] & 7;
            float past = (p[0] - d.here[0]) * d.in[0] + (p[1] - d.here[1]) * d.in[1];
            if (e[0x111] == currArea && past > 0.05f && (type == 1 || type == 4 || type == 5)) {
                hide(e);
            }
        });
    }
}

// after a switch: until the camera is through the doorway too (or the player has gone on, or
// long enough), this side's door and an interior's backdrop past its doorway left out of the
// main view, which the camera, outside, would see first
void ReverseUpkeep(const float* at, uint32_t now) {
    if (g_reverse.on) {
        const Doorway& d = g_doorway;
        // (the camera as last drawn: in the update, by the switch, the camera's own frame is elsewhere)
        const float* eye = g_drawn ? g_drawnLtm + 9 : nullptr;
        float s = eye ? (eye[0] - d.here[0]) * d.in[0] + (eye[1] - d.here[1]) * d.in[1] : 0.0f;
        float ex = eye ? eye[0] - d.here[0] : 0.0f, ey = eye ? eye[1] - d.here[1] : 0.0f;
        bool nearby = ex * ex + ey * ey < 25.0f * 25.0f;   // (not yet drawn from the new side: as before the switch)
        float dx = at[0] - d.here[0], dy = at[1] - d.here[1];
        const char* why = !d.ok                                       ? "no doorway"
                          : nearby && s < -0.05f                      ? "the camera through"
                          : int32_t(now - g_reverse.until) > 0        ? "long enough"
                          : dx * dx + dy * dy > 8.0f * 8.0f           ? "the player gone on"
                                                                      : nullptr;
        if (why) {
            if (re::DoorReport()) {
                std::fprintf(stderr, "doors: the reverse ends at %u ms: %s (the camera %.2f through)\n", now, why, s);
            }
            EndReverse();
        } else {
            HideForReverse();
        }
    }
}



// IDirect3DDevice9::SetRenderState on the game's device, outside its state cache (196 isn't a
// D3D9 state: bullygfx reads it, portal.rs): 'PRT1' the draws kept to the doorway (or, around it,
// the sky), 'PRT2' those around it and in front of it, 0 the rest
constexpr uint32_t kPortalDraws = 0x31545250;         // 'PRT1'
constexpr uint32_t kPortalAroundDraws = 0x32545250;   // 'PRT2'
void SetPortalDraws(uint32_t value) {
    void* device = *reinterpret_cast<re::Ptr32<void>*>(ms_pRenderer + 0xA4);
    reinterpret_cast<long(__stdcall*)(void*, uint32_t, uint32_t)>(static_cast<void*>(re::ComVtable(device)[0xE4 / 4]))(
        device, 196, value);
}

// (--doors) the lights in the effect state of a tree's first geometry (-1: none found)
int GeometryLights(uint8_t* object, int depth = 0) {
    if (!object || depth > 6) {
        return -1;
    }
    if (NiIsKindOf(NiGeometry_RTTI, object)) {
        uint8_t* state = At<uint8_t*>(object, 0xC8);   // (NiGeometry's NiDynamicEffectState)
        int lights = 0;
        for (uint8_t* node = state ? At<uint8_t*>(state, 0xC) : nullptr; node && lights < 99; node = At<uint8_t*>(node, 0)) {
            lights++;
        }
        return state ? lights : -2;
    }
    if (NiIsKindOf(NiNode_RTTI, object)) {
        uint16_t size = At<uint16_t>(object, 0xCE);
        re::Ptr32<uint8_t>* children = At<re::Ptr32<re::Ptr32<uint8_t>>>(object, 0xC8);
        for (uint16_t i = 0; i < size && children; i++) {
            int n = GeometryLights(children[i], depth + 1);
            if (n != -1) {
                return n;
            }
        }
    }
    return -1;
}

void StopPreload() {
    EndReverse();
    if (!g_preload.on) {
        return;
    }
    if (OurScene()) {
        ExtraScene_Clear();
    }
    if (re::DoorReport()) {
        std::fprintf(stderr, "doors: preload of area %d (%s) stopped at %u ms\n", g_preload.area, AreaName(g_preload.area),
                     m_snTimeInMilliseconds);
    }
    g_preload = Preload{};
    if (g_doorway.door && re::PortalTest()) {
        At<uint32_t>(g_doorway.door, 0xD0) = 0;   // (CEntity::bDontRender)
    }
    g_doorway = Doorway{};
}

// (--doors, once a second) the peds there are by area, and those by the door's place on the other
// side (the preload's route point) and by the player
void ReportPeds() {
    static uint32_t said = 0;
    uint32_t now = m_snTimeInMilliseconds;
    if (now - said < 1000) {
        return;
    }
    said = now;
    uint8_t* pool = *reinterpret_cast<re::Ptr32<uint8_t>*>(0x00C0F5F0);   // (CPools::ms_pPedPool)
    if (!pool) {
        return;
    }
    int byArea[64] = {};
    int nearThere = 0, nearPlayer = 0, total = 0;
    const float* p = EntityPosition(PED_player);
    int count = At<int32_t>(pool, 8);
    const uint8_t* flags = At<uint8_t*>(pool, 4);
    for (int slot = 0; slot < count; slot++) {
        uint8_t* ped = (flags[slot] & 0x80) ? nullptr : GetEntityInPool(0, uint32_t(slot));
        if (!ped || ped == PED_player) {
            continue;
        }
        total++;
        int area = ped[0x111];
        byArea[area & 63]++;
        const float* at = EntityPosition(ped);
        double dx = at[0] - g_preload.x, dy = at[1] - g_preload.y;
        if (g_preload.on && dx * dx + dy * dy < 40.0 * 40.0) {   // (a ped's +0x111 is always 0: by place)
            nearThere++;
        }
        dx = at[0] - p[0];
        dy = at[1] - p[1];
        if (dx * dx + dy * dy < 30.0 * 30.0) {
            nearPlayer++;
        }
    }
    char text[256];
    int n = std::snprintf(text, sizeof text, "doors: peds %d of %d slots (area %d current):", total, count, currArea);
    for (int a = 0; a < 64 && n < int(sizeof text) - 16; a++) {
        if (byArea[a]) {
            n += std::snprintf(text + n, sizeof text - n, " area %d: %d", a, byArea[a]);
        }
    }
    std::fprintf(stderr, "%s; by the player %d; by the door there (area %d) %d at %u ms\n", text, nearPlayer,
                 g_preload.on ? g_preload.area : -1, nearThere, now);
    {
        // (the roster: each ped's slot, model and kind, to see who is made or let go)
        char roster[512];
        int r = std::snprintf(roster, sizeof roster, "doors:   roster:");
        for (int slot = 0; slot < count && r < int(sizeof roster) - 24; slot++) {
            uint8_t* ped = (flags[slot] & 0x80) ? nullptr : GetEntityInPool(0, uint32_t(slot));
            if (ped && ped != PED_player) {
                r += std::snprintf(roster + r, sizeof roster - r, " %d:%d/%d", slot, At<int16_t>(ped, 0x10E), ped[0x1570]);
            }
        }
        std::fprintf(stderr, "%s\n", roster);
    }
    // (every 10 s: each ped, what kind (+0x1570: 1 ambient, 2 a script's), its type and how far)
    static uint32_t listed = 0;
    if (now - listed < 10000) {
        return;
    }
    listed = now;
    for (int slot = 0; slot < count; slot++) {
        uint8_t* ped = (flags[slot] & 0x80) ? nullptr : GetEntityInPool(0, uint32_t(slot));
        if (!ped || ped == PED_player) {
            continue;
        }
        const float* at = EntityPosition(ped);
        double dx = at[0] - p[0], dy = at[1] - p[1];
        std::fprintf(stderr, "doors:   ped %2d: kind %d type %2d model %4d at (%.1f, %.1f, %.1f), %.0f m away, vehicle %d, group %d, spawner %d\n",
                     slot, ped[0x1570], At<int32_t>(ped, 0x1310), At<int16_t>(ped, 0x10E), at[0], at[1], at[2],
                     std::sqrt(dx * dx + dy * dy), At<uint8_t*>(ped, 0x1554) != nullptr, At<uint8_t*>(ped, 0x1D24) != nullptr,
                     At<int32_t>(ped, 0x1B8) != 0);
    }
}

} // namespace seamless

namespace re {

// (RunScheduled, each frame) the current area's doors, when they change
void ReportDoors() {
    using namespace seamless;
    static int lastArea = -2;
    static int lastCount = -1;
    uint8_t* atm = theTransitionManager;
    int count = At<int32_t>(atm, 8);
    if ((currArea == lastArea && count == lastCount) || At<int32_t>(atm, 0x864) != 0) {
        return;
    }
    lastArea = currArea;
    lastCount = count;
    std::fprintf(stderr, "doors: area %d (%s), %d doors, game time %u\n", currArea, AreaName(currArea), count,
                 m_snTimeInMilliseconds);
    for (int i = 0; i < count && i < 100; i++) {
        uint8_t* trigger = At<uint8_t*>(atm, 0x10 + i * 8);
        uint8_t* route = At<uint8_t*>(atm, 0x14 + i * 8);
        if (!trigger || !route) {
            continue;
        }
        const float* at = &At<float>(trigger, 4);
        const float* to = reinterpret_cast<const float*>(Route_First(route));
        int area = int(int8_t(route[0xC]));
        std::fprintf(stderr,
                     "doors:  %2d %s at (%.2f, %.2f, %.2f) heading %.1f%s -> area %d (%s) at (%.2f, %.2f, %.2f) heading %.1f"
                     " (route %d flags %#x)\n",
                     i, At<uint8_t>(atm, 0x330 + i) ? "door" : "walk", at[0], at[1], at[2],
                     double(At<float>(trigger, 0x38)) * 57.29577951308232, At<uint8_t*>(trigger, 0x1C) ? " (prop)" : "", area,
                     AreaName(area), to ? to[0] : 0.0f, to ? to[1] : 0.0f, to ? to[2] : 0.0f, to ? double(to[7]) - 90.0 : 0.0,
                     At<int32_t>(route, 8), At<uint32_t>(route, 0x18));
    }
    // (what is around the doors near the player)
    if (uint8_t* player = PED_player) {
        const float* p = EntityPosition(player);
        for (int i = 0; i < count && i < 100; i++) {
            uint8_t* trigger = At<uint8_t*>(atm, 0x10 + i * 8);
            double dx = trigger ? At<float>(trigger, 4) - p[0] : 1e9, dy = trigger ? At<float>(trigger, 8) - p[1] : 1e9;
            if (dx * dx + dy * dy < 30.0 * 30.0) {
                ReportEntitiesNear("door", &At<float>(trigger, 4), 3.0);
            }
        }
    }
}

// (AreaTransitionManager::Update, each frame) what is behind the nearest door kept loaded
void SeamlessDoorsFrame() {
    using namespace seamless;
    uint8_t* atm = theTransitionManager;
    uint8_t* player = PED_player;
    if (!player || At<uint8_t*>(player, 0x14) == nullptr) {
        return;
    }
    if (DoorReport()) {
        ReportPeds();
    }
    if (!SeamlessDoors()) {
        return;
    }
    // (a transition going on: what was loaded stays until it's done)
    if (At<int32_t>(atm, 0x864) != 0 || At<uint8_t>(atm, 0x86C) != 0 || At<int32_t>(atm, 0x868) != 0 ||
        At<uint8_t>(atm, 0x86D) != 0) {
        return;
    }
    {
        static int notedArea = -2, notedCount = -1;
        int count = At<int32_t>(atm, 8);
        if (currArea != notedArea || count != notedCount) {
            notedArea = currArea;
            notedCount = count;
            NoteDoors(atm);
        }
    }
    // (our extra scene replaced by a script's: theirs now, left alone; cleared by the game itself (an
    // area's load clears the town's placement point): made again)
    if (g_preload.on && g_preload.scene && !OurScene()) {
        bool theirs = sa_active && (sa_area != g_preload.area || sa_x != g_preload.sx || sa_y != g_preload.sy || g_preload.area == 0);
        if (theirs) {
            if (DoorReport()) {
                std::fprintf(stderr, "doors: a script's extra scene (area %d at %.2f, %.2f, active %d) at %u ms\n", sa_area, sa_x, sa_y, sa_active,
                             m_snTimeInMilliseconds);
            }
            EndReverse();
            g_preload = Preload{};
        } else {
            float at[3] = {g_preload.sx, g_preload.sy, g_preload.sz};
            SetScene(g_preload.area, at);
            if (DoorReport()) {
                std::fprintf(stderr, "doors: our extra scene for area %d made again at %u ms\n", g_preload.area, m_snTimeInMilliseconds);
            }
        }
    }
    const float* at = &At<float>(At<uint8_t*>(player, 0x14), 0x30);
    uint8_t* route = NearestDoor(atm, at);
    const float* to = route ? reinterpret_cast<const float*>(Route_First(route)) : nullptr;
    if (!to && g_preload.on && g_preload.adopt && int32_t(m_snTimeInMilliseconds - g_reverse.since) < 1500) {
        ReverseUpkeep(at, m_snTimeInMilliseconds);   // (the new area's doors not listed yet: the preload made at the switch kept)
        return;
    }
    if (!to) {
        if (DoorReport() && g_preload.on) {
            std::fprintf(stderr, "doors: no door near at %u ms\n", m_snTimeInMilliseconds);
        }
        StopPreload();
        return;
    }
    int area = int(int8_t(route[0xC]));
    uint32_t now = m_snTimeInMilliseconds;
    if (g_preload.on && g_preload.adopt && g_preload.area == area) {
        // (just through a door: the preload of the way back made at the switch; its route and this
        // side's door prop, made by the switch, known now)
        g_preload.adopt = false;
        g_preload.x = to[0];
        g_preload.y = to[1];
        g_preload.z = to[2];
        for (int i = 0; i < At<int32_t>(atm, 8) && i < 100; i++) {
            if (At<uint8_t*>(atm, 0x14 + i * 8) == route && At<uint8_t*>(atm, 0x10 + i * 8)) {
                g_doorway.door = At<uint8_t*>(At<uint8_t*>(atm, 0x10 + i * 8), 0x1C);
            }
        }
    }
    if (!g_preload.on || g_preload.area != area || g_preload.x != to[0] || g_preload.y != to[1]) {
        if (DoorReport() && g_preload.on) {
            std::fprintf(stderr, "doors: another door at %u ms: area %d at (%.2f, %.2f) (was %d at (%.2f, %.2f)%s)\n", m_snTimeInMilliseconds, area,
                         to[0], to[1], g_preload.area, g_preload.x, g_preload.y, g_preload.adopt ? ", to adopt" : "");
        }
        StopPreload();
        if (sa_active) {
            return;   // (a script's extra scene)
        }
        if (PortalArea(area)) {
            SetScene(area, to);
            g_doorway = FindDoorway(atm, route);
            if (g_doorway.door && PortalTest()) {
                At<uint32_t>(g_doorway.door, 0xD0) = 1;
            }
            if (DoorReport()) {
                const Doorway& d = g_doorway;
                std::fprintf(stderr, "doors: doorway %s: (%.2f, %.2f, %.2f) in (%.2f, %.2f) -> (%.2f, %.2f, %.2f), turn %.1f, %.2f x %.2f\n",
                             d.ok ? "found" : "not found", d.here[0], d.here[1], d.here[2], d.in[0], d.in[1], d.there[0], d.there[1],
                             d.there[2], std::atan2(double(d.sinTurn), double(d.cosTurn)) * 57.29577951308232, d.width, d.height);
            }
        }
        bool scene = g_preload.scene;
        float sx = g_preload.sx, sy = g_preload.sy, sz = g_preload.sz;
        g_preload = Preload{true, area, to[0], to[1], to[2], now, 0, scene, sx, sy, sz, false, currArea};
        if (DoorReport()) {
            std::fprintf(stderr, "doors: preload of area %d (%s) around (%.2f, %.2f, %.2f) at %u ms\n", area, AreaName(area), to[0],
                         to[1], to[2], now);
            ReportDoorPair(atm, route);
        }
    }
    if (now - g_preload.modelsAt >= kModelsEveryMs) {
        g_preload.modelsAt = now;
        if (area == 0) {
            RequestTownModels(to[0], to[1]);
        } else {
            ExtraScene_RequestModels();
        }
    }
    ReverseUpkeep(at, now);
}

// (MdBullyMainViewRenderClick::RenderScene, after the world) step 2: the interior the player is by drawn through its
// open door. The doorway (a little behind the door, so a shut door hides it) goes to bullygfx,
// which marks where it is seen; then the world is drawn again from the main camera moved
// through the door (the render list made from there, as for a mirror), kept to that mark.
void SeamlessDoorsDraw(void* culler, void* scene, void* sorter, void* accumulator) {
    using namespace seamless;
    const Doorway& d = g_doorway;
    uint8_t* main = ms_pMainRwCamera;
    if (main) {
        std::memcpy(g_drawnLtm, RwFrameGetLTM(RwCameraGetFrame(main)), sizeof(g_drawnLtm));
        g_drawnDir[0] = At<float>(main, 0x60);
        g_drawnDir[1] = At<float>(main, 0x6C);
        g_drawnDir[2] = At<float>(main, 0x78);
        g_drawnFov = ms_fov;
        g_drawn = true;
    }
    if (DoorReport() && main && int32_t(g_traceUntil - m_snTimeInMilliseconds) > 0 && d.ok) {
        const float* ltm = RwFrameGetLTM(RwCameraGetFrame(main));
        const float* world = &At<float>(main, 0x84);
        const float* p = PED_player ? EntityPosition(PED_player) : world;
        auto side = [&](const float* v) { return (v[0] - d.here[0]) * d.in[0] + (v[1] - d.here[1]) * d.in[1]; };
        std::fprintf(stderr, "doors:   drawn at %u ms (state %d, area %d): frame %.2f world %.2f, the player %.2f through, fov %.1f%s, frustum %.3f\n",
                     m_snTimeInMilliseconds, At<int32_t>(theTransitionManager, 0x864), currArea, side(ltm + 9), side(world), side(p),
                     ms_fov, ms_bFovOverride ? " (overridden)" : "", At<float>(main, 0x104 + 4));
    }
    if (!SeamlessDoors() || !Portals() || !d.ok || !g_preload.on || !PortalArea(g_preload.area) || !main || !culler || !scene || !sorter ||
        !accumulator || m_snTimeInMilliseconds - g_preload.since < kReadyMs) {
        return;
    }
    uint8_t* atm = theTransitionManager;
    if (At<int32_t>(atm, 0x864) >= 9) {
        return;   // (the player is through: the switch has been made)
    }
    // the main camera, on this side of the doorway (the other side drawn in it), or after a switch
    // still through it, on the side left (that drawn around it)
    const float* eye = &At<float>(main, 0x84);
    float through = (eye[0] - d.here[0]) * d.in[0] + (eye[1] - d.here[1]) * d.in[1];
    bool around = g_reverse.on && through > 0.0f && through < 25.0f;
    if (!around && (through > -0.1f || through < -25.0f)) {
        return;
    }
    float camera[18];
    CameraOf(main, camera);
    // the doorway's corners: in, 0.15 past the door's plane (a shut door hides it); around, 0.15
    // this side of it (what the side left has up to there drawn in front of what's past it)
    float corners[12];
    DoorwayCorners(d, around ? -0.15f : 0.15f, corners);
    if (PortalFull() && !around) {
        // (a quad right in front of the camera, wider than the view)
        const float k[4][2] = {{-3, -3}, {3, -3}, {3, 3}, {-3, 3}};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                corners[i * 3 + j] = camera[j] + camera[3 + j] * 0.5f + camera[9 + j] * k[i][0] + camera[6 + j] * k[i][1];
            }
        }
    }
    int sent = SendDoorway(corners, camera, around ? kReady : kIn);
    if (DoorReport() && g_reverse.on) {
        std::fprintf(stderr, "doors: pass at %u ms: %s, the camera %.2f through, bullygfx %d\n", m_snTimeInMilliseconds,
                     around ? "around" : "in", through, sent);
    }
    if (!sent) {
        return;
    }

    // the main camera moved through the doorway: its LTM (NiTransform: rotation rows, position,
    // scale) turned and moved
    float moved[13];
    MoveThrough(d, RwFrameGetLTM(RwCameraGetFrame(main)), moved);
    uint8_t* camera2 = PortalCamera();
    SetCameraFrame(camera2, moved);

    // lit as inside: the time cycle's interior values and the area's own lights, for the pass
    // (both made again for here after it)
    // (--doors: the pass's parts timed, ms a frame)
    static double spent[9];
    static int timed = 0;
    LARGE_INTEGER frequency, tick;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&tick);
    int64_t last = tick.QuadPart;
    auto mark = [&](int part) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        spent[part] += double(now.QuadPart - last) * 1000.0 / double(frequency.QuadPart);
        last = now.QuadPart;
    };
    int32_t here = currArea;
    currArea = g_preload.area;
    TimeCycle_Update();
    // (its near plane the main camera's; its far plane and fog the time cycle's there, as MainLoop
    // sets the main camera's)
    RwCameraSetNearClipPlane(camera2, At<float>(main, 0x104 + 0x10));
    RwCameraSetFarClipPlane(camera2, std::max(float(int32_t(ms_farClip)), At<float>(main, 0x104 + 0x14)));
    RwCameraSetFogDistance(camera2, ms_fogStart);
    float fov = ms_bFovOverride ? ms_fovOverride : ms_fov;
    RwCamera_SetView(camera2, 0, float(std::tan(double(fov) * 0.008726646502812704)), Camera_GetAspectRatio(TheCamera));
    // (and as near as the main camera's: RenderWare's keeps it at least far / the screen's height,
    // the NiCamera's limits, which the main camera has as the game set them)
    At<float>(camera2, 0x120) = At<float>(main, 0x120);
    At<float>(camera2, 0x124) = At<float>(main, 0x124);
    float frustum2[7];
    std::memcpy(frustum2, camera2 + 0x104, sizeof(frustum2));
    frustum2[4] = At<float>(main, 0x104 + 0x10);
    NiCamera_SetViewFrustum(camera2, frustum2);
    g_portalCameraReady = true;
    static uint32_t saidFog = 0;
    if (DoorReport() && m_snTimeInMilliseconds - saidFog > 1000) {
        saidFog = m_snTimeInMilliseconds;
        std::fprintf(stderr, "doors:   area %d's time cycle: fog from %d, far %d; the main camera's near %.2f far %.1f fog %.1f\n",
                     g_preload.area, int(ms_fogStart), int(ms_farClip), At<float>(main, 0x104 + 0x10), At<float>(main, 0x104 + 0x14),
                     At<float>(main, 0x1BC));
    }
    if (g_preload.area == 0) {
        FogConstantsFor(camera2);   // (the town's fog through an interior's door; an interior's own is its time cycle's too, A/B)
    }
    mark(0);
    // (the main camera's view window as the frame set it: CCamera's updates set it from the fov)
    float window[2];
    std::memcpy(window, RwCameraGetViewWindow(main), sizeof(window));
    Camera_SetRwCamera(TheCamera, camera2);
    Camera_UpdateFrustumPlanes(TheCamera);
    Camera_Update_4F1F10(TheCamera);
    // what the area there has by its doorway given its Gamebryo objects again where streaming let
    // them go (far from the main camera, or for room) and their models are still loaded: the render
    // list makes a building's again, not an object's or a dummy's (CStreaming::CreateRwObjectsInList's
    // way: CEntity vtable[10] (0, 1))
    // (the main camera's interior box, if it's in one, isn't this camera's: the scan would keep to
    // its sectors and heights)
    uint8_t inInterior = rs_bCameraInInterior;
    rs_bCameraInInterior = 0;
    int remade = 0;
    ForEntitiesNear(d.there[0], d.there[1], 40.0, [&](uint8_t* e) {
        int type = e[0x108] & 7;
        int16_t model = At<int16_t>(e, 0x10E);
        if (e[0x111] != uint8_t(g_preload.area) || !(type == 1 || type == 4 || type == 5) || model < 0) {
            return;
        }
        // (seen at once, not faded in from nothing as a model just made is: it was in view a moment
        // ago, from the other side; the area's models are its own, nothing of the main view's)
        if (uint8_t* mi = ms_modelInfoPtrs[model]) {
            mi[0x2C] = 0xFF;
        }
        if (remade >= 48 || At<uint8_t*>(e, 0x18) != nullptr || At<uint32_t>(e, 0xD0) != 0) {
            return;
        }
        int index = Streaming_GetInfoIndex(model);
        if (index < 0 || ms_aInfoForModel[index * 0x10 + 4] != 1) {
            return;
        }
        using Fn = void(__thiscall*)(uint8_t*, int, int);
        re::GameFnAt<Fn>(At<uint8_t*>(e, 0) + 0x28)(e, 0, 1);
        remade++;
    });
    if (remade && DoorReport()) {
        std::fprintf(stderr, "doors:   %d objects of area %d made again by the doorway\n", remade, g_preload.area);
    }
    SetPortalScan(true);
    Renderer_ConstructRenderList();
    SetPortalScan(false);
    mark(1);
    static uint32_t saidAt = 0;
    bool say = DoorReport() && m_snTimeInMilliseconds - saidAt > 1000;
    if (say) {
        saidAt = m_snTimeInMilliseconds;
        std::fprintf(stderr, "doors: through the doorway: camera (%.2f, %.2f, %.2f), %d visible\n", rs_vecCameraPosition[0],
                     rs_vecCameraPosition[1], rs_vecCameraPosition[2], rs_nNoOfVisibleEntities);
        int shown = 0;
        for (int i = 0; i < rs_nNoOfVisibleEntities && shown < 12; i++) {
            uint8_t* e = rs_aVisibleEntityPtrs[i];
            const float* at = EntityPosition(e);
            float dx = at[0] - rs_vecCameraPosition[0], dy = at[1] - rs_vecCameraPosition[1];
            if (dx * dx + dy * dy > 30.0f * 30.0f) {
                continue;   // (the nearest: what could stand in the doorway's way)
            }
            shown++;
            uint8_t* object = At<uint8_t*>(e, 0x18);
            uint8_t* mi = ms_modelInfoPtrs[At<int16_t>(e, 0x10E)];
            float into = (at[0] - d.there[0]) * d.thereIn[0] + (at[1] - d.there[1]) * d.thereIn[1];
            uint8_t* col = mi ? At<uint8_t*>(mi, 0xC) : nullptr;
            std::fprintf(stderr, "doors:   type %d model %d area %d at (%.2f, %.2f, %.2f), %.2f into, radius %.1f, object kind %d hidden %u\n",
                         e[0x108] & 7, At<int16_t>(e, 0x10E), e[0x111], at[0], at[1], at[2], into, col ? At<float>(col, 0xC) : -1.0f,
                         object ? object[0] : -1, At<uint32_t>(e, 0xD0));
        }
    }
    Renderer_PreRender();
    Renderer_5EF890();
    mark(2);
    SetRenderWareCamera(camera2);
    // the visible list "rendered": its geometry placed in the world, marked seen by the main view
    // (+0xB0 8) and put under the scene's Lit Objects (RpAtomic::DefaultRender), not drawn: the
    // click's culler draws the scene. The scene's update has been made this frame: again, for
    // what has just come in (its world transforms and bounds)
    // (what an interior has on the far side of its own doorway is left out: the backdrop of the
    // street the door shows from inside, the step outside it; the camera here looks at its back.
    // The town's buildings there are left: one's middle may be behind its door, its porch before)
    uint8_t* hidden[64];
    int hiddenCount = 0;
    for (int i = 0; i < rs_nNoOfVisibleEntities && hiddenCount < 64 && g_preload.area != 0 && !around; i++) {
        uint8_t* e = rs_aVisibleEntityPtrs[i];
        const float* at = EntityPosition(e);
        float into = (at[0] - d.there[0]) * d.thereIn[0] + (at[1] - d.there[1]) * d.thereIn[1];
        if (e[0x111] == g_preload.area && into < -0.05f && At<uint32_t>(e, 0xD0) == 0) {
            At<uint32_t>(e, 0xD0) = 1;   // (CEntity::bDontRender)
            hidden[hiddenCount++] = e;
        }
    }
    // (the other side's own door, if its placement has made it: shut, it would stand in the doorway;
    // the one seen is this side's)
    ForDoorsAt(d.thereHinge, [&](uint8_t* e) {
        if (hiddenCount < 64 && At<uint32_t>(e, 0xD0) == 0) {
            At<uint32_t>(e, 0xD0) = 1;
            hidden[hiddenCount++] = e;
        }
    });
    if (RwCameraBeginUpdate(camera2)) {
        DefinedState();
        Renderer_RenderWorld();
        RwCameraEndUpdate(camera2);
    }
    rs_bCameraInInterior = inInterior;
    for (int i = 0; i < hiddenCount; i++) {
        At<uint32_t>(hidden[i], 0xD0) = 0;
    }
    mark(3);
    // the area's lights, measured from where the player would be there, given to what is now under
    // Lit Objects (the light manager attaches them to the nodes they reach: after the world's render,
    // as in the frame)
    float playerThere[3];
    ThroughPoint(d, EntityPosition(PED_player), playerThere);
    // (at least a metre into the area there: the player outside its doorway would be out of its
    // lights' reach, as when just through the other way)
    float into = (playerThere[0] - d.there[0]) * d.thereIn[0] + (playerThere[1] - d.there[1]) * d.thereIn[1];
    if (into < 1.0f) {
        playerThere[0] += d.thereIn[0] * (1.0f - into);
        playerThere[1] += d.thereIn[1] * (1.0f - into);
    }
    SetLightsFrom(playerThere);
    if (uint8_t* lights = GetLightManager()) {
        LightManager_Update(lights);
        static uint32_t saidLights = 0;
        if (DoorReport() && m_snTimeInMilliseconds - saidLights > 1000) {
            saidLights = m_snTimeInMilliseconds;
            uint8_t* level = lights + 0x30 + g_preload.area * 0x14;   // (+4 loaded, +8 states 0x24 each, +0x10 count)
            uint32_t count = At<uint32_t>(level, 0x10), on = 0;
            uint8_t* states = At<uint8_t*>(level, 8);
            for (uint32_t i = 0; i < count && states; i++) {
                on += states[i * 0x24 + 0x10] != 0;
            }
            std::fprintf(stderr, "doors:   lights of area %d from (%.2f, %.2f, %.2f): loaded %d, %u lights, %u on\n", g_preload.area,
                         playerThere[0], playerThere[1], playerThere[2], level[4], count, on);
        }
    }
    SetLightsFrom(nullptr);
    mark(4);
    // what came in brought up to date as the scene's update does Lit Objects' children (world
    // transform and bound, the lights on it, the fade), alone: the town's are left as they are
    // (their effect states made again would have their materials made again, each frame)
    for (int i = 0; i < rs_nNoOfVisibleEntities; i++) {
        uint8_t* e = rs_aVisibleEntityPtrs[i];
        uint8_t* object = At<uint8_t*>(e, 0x18);
        if (e[0x111] != g_preload.area || !object || object[0] != 1) {
            continue;
        }
        uint8_t* node = At<uint8_t*>(object, 0x14);   // (the atomic's tree, under Lit Objects)
        if (node) {
            AVObject_Update(node, 0.0f, 0);
            AVObject_UpdateEffects(node);
            SetupFade(node);
        }
    }
    mark(5);
    // the scene culled from this camera into the accumulator, which draws it (as the click's
    // RenderScene does; the camera made the renderer's: its close-up layer makes the main one
    // its again)
    uint32_t drawsBefore = 0, failedBefore = 0, drawsAfter = 0, failedAfter = 0;
    DrawCounts(&drawsBefore, &failedBefore);
    Renderer_SetCamera(camera2);
    if (around) {
        // the doorway marked around: its corners where the side left has them, seen from this
        // camera (the one the draws are made with now)
        float cornersThere[12], camera2Data[18];
        for (int i = 0; i < 4; i++) {
            ThroughPoint(d, corners + i * 3, cornersThere + i * 3);
        }
        CameraOf(camera2, camera2Data);
        int marked = SendDoorway(cornersThere, camera2Data, kAround);
        if (DoorReport()) {
            std::fprintf(stderr, "doors:   around: marked %d, %d visible, camera there (%.2f, %.2f, %.2f) looking (%.2f, %.2f, %.2f), near %.3f far %.1f\n",
                         marked, rs_nNoOfVisibleEntities, camera2Data[0], camera2Data[1], camera2Data[2], camera2Data[3], camera2Data[4],
                         camera2Data[5], camera2Data[16], camera2Data[17]);
        }
    }
    if (around || g_preload.area == 0) {
        // the sky there: the time cycle's colours and domes, kept to the doorway or, around it, to
        // what's around it (the domes' turn and drift as they were: the main view's turned them)
        float rotation = skyDomeRotation, drift = cloudDomeDrift;
        SetPortalDraws(kPortalDraws);
        MainViewClick_RenderBackground(nullptr, camera2);
        SetPortalDraws(0);
        skyDomeRotation = rotation;
        cloudDomeDrift = drift;
        Renderer_SetCamera(camera2);
    }
    vc::Call<void>(sorter, 0x48, camera2);
    MdBullyCullingProcess_Cull(culler, camera2, scene, accumulator, nullptr);
    mark(6);
    if (say) {
        for (int i = 0; i < rs_nNoOfVisibleEntities && i < 6; i++) {
            uint8_t* e = rs_aVisibleEntityPtrs[i];
            uint8_t* object = At<uint8_t*>(e, 0x18);
            if (e[0x111] == g_preload.area && object && object[0] == 1) {
                std::fprintf(stderr, "doors:   model %d: its geometry has %d lights\n", At<int16_t>(e, 0x10E),
                             GeometryLights(At<uint8_t*>(object, 0x14)));
            }
        }
    }
    SetPortalDraws(around ? kPortalAroundDraws : kPortalDraws);
    vc::Call<void>(sorter, 0x4C);
    SetPortalDraws(0);
    mark(7);
    DrawCounts(&drawsAfter, &failedAfter);
    if (say) {
        std::fprintf(stderr, "doors:   %u draws (%u failed)\n", drawsAfter - drawsBefore, failedAfter - failedBefore);
    }
    DefinedState();
    SetRenderWareCamera(main);
    Camera_SetRwCamera(TheCamera, main);
    Camera_UpdateFrustumPlanes(TheCamera);
    Camera_Update_4F1F10(TheCamera);
    RwCameraSetViewWindow(main, window);
    currArea = here;
    TimeCycle_Update();
    FogConstantsFor(main);
    if (uint8_t* lights = GetLightManager()) {
        LightManager_Update(lights);
    }
    RwCameraBeginUpdate(main);   // (the main camera's update goes on)
    mark(8);
    timed++;
    if (say) {
        static const char* const kParts[9] = {"time cycle", "scan", "prerender", "world", "lights", "nodes", "cull", "draw", "restore"};
        char line[400];
        int n = std::snprintf(line, sizeof line, "doors:   ms a frame over %d:", timed);
        for (int i = 0; i < 9; i++) {
            n += std::snprintf(line + n, sizeof line - size_t(n), " %s %.2f", kParts[i], spent[i] / timed);
            spent[i] = 0;
        }
        std::fprintf(stderr, "%s\n", line);
        timed = 0;
    }
}

bool SeamlessDoorHoldsCamera(const char* what) {
    using namespace seamless;
    const Doorway& d = g_doorway;
    uint8_t* player = PED_player;
    bool hold = false;
    if (SeamlessDoors() && Portals() && d.ok && g_preload.on && PortalArea(g_preload.area) && player &&
        m_snTimeInMilliseconds - g_preload.since >= kReadyMs) {
        const float* at = EntityPosition(player);
        float dx = at[0] - d.here[0], dy = at[1] - d.here[1];
        hold = dx * dx + dy * dy < 3.0f * 3.0f;
    }
    if (DoorReport()) {
        std::fprintf(stderr, "doors: %s at %u ms: %s\n", what, m_snTimeInMilliseconds, hold ? "left out (seamless door)" : "as before");
    }
    return hold;
}

bool SeamlessDoorCrossed() {
    using namespace seamless;
    const Doorway& d = g_doorway;
    uint8_t* player = PED_player;
    if (!SeamlessDoors() || !Portals() || !d.ok || !player) {
        return false;
    }
    const float* at = EntityPosition(player);
    float through = (at[0] - d.here[0]) * d.in[0] + (at[1] - d.here[1]) * d.in[1];
    g_traceUntil = m_snTimeInMilliseconds + 600;
    if (DoorReport() && ms_pMainRwCamera) {
        const float* eye = &At<float>(ms_pMainRwCamera, 0x84);
        std::fprintf(stderr, "doors:   countdown at %u ms: the player %.2f through, the camera %.2f\n", m_snTimeInMilliseconds, through,
                     (eye[0] - d.here[0]) * d.in[0] + (eye[1] - d.here[1]) * d.in[1]);
    }
    if (through > 0.15f && DoorReport()) {
        std::fprintf(stderr, "doors: the player is through the doorway at %u ms\n", m_snTimeInMilliseconds);
    }
    return through > 0.15f;
}

bool SeamlessDoorPlacement(float* position, float* heading, float* look) {
    using namespace seamless;
    const Doorway& d = g_doorway;
    uint8_t* player = PED_player;
    uint8_t* main = ms_pMainRwCamera;
    if (!g_goingIn || !d.ok || !player || !main || At<uint8_t*>(player, 0x14) == nullptr) {
        return false;
    }
    // (in the doorway's opening and at least half a metre through it: the countdown may have ended
    // before the player was through, and the player is kept clear of the door on the other side)
    const float* at = EntityPosition(player);
    float rel[2] = {at[0] - d.here[0], at[1] - d.here[1]};
    float across = rel[0] * d.along[0] + rel[1] * d.along[1];
    float through = rel[0] * d.in[0] + rel[1] * d.in[1];
    float room = std::max(d.width * 0.5f - 0.3f, 0.0f);
    across = std::min(std::max(across, -room), room);
    through = std::max(through, 0.5f);
    float here[3] = {d.here[0] + d.along[0] * across + d.in[0] * through, d.here[1] + d.along[1] * across + d.in[1] * through, at[2]};
    ThroughPoint(d, here, position);
    // (CPed's heading: from its matrix's forward, as AreaTransitionManager's HeadingOf)
    const uint8_t* matrix = At<uint8_t*>(player, 0x14);
    double h = std::atan2(-double(At<float>(matrix, 0x10)), double(At<float>(matrix, 0x14))) +
               std::atan2(double(d.sinTurn), double(d.cosTurn));
    *heading = float(std::remainder(h, 6.283185307179586));
    // (the main camera's direction as last drawn: its world rotation's first column, NiCamera +0x60)
    float dir[3] = {At<float>(main, 0x60), At<float>(main, 0x6C), At<float>(main, 0x78)};
    if (g_drawn) {
        std::memcpy(dir, g_drawnDir, sizeof(dir));
    }
    look[0] = d.cosTurn * dir[0] - d.sinTurn * dir[1];
    look[1] = d.sinTurn * dir[0] + d.cosTurn * dir[1];
    look[2] = dir[2];
    if (DoorReport()) {
        std::fprintf(stderr, "doors: the player goes on from (%.2f, %.2f, %.2f) heading %.1f (from (%.2f, %.2f, %.2f), %.2f through)\n",
                     position[0], position[1], position[2], double(*heading) * 57.29577951308232, at[0], at[1], at[2], double(through));
    }
    return true;
}

void SeamlessDoorSwitch() {
    using namespace seamless;
    if (!g_goingIn || !g_doorway.ok || !g_preload.on) {
        return;
    }
    const Doorway d = g_doorway;
    // the area left (currArea is the new one already)
    int left = g_preload.area == currArea ? g_preload.from : -1;
    if (left < 0 || left == 1 || !PortalArea(left)) {
        return;
    }
    g_keepPeds = TwoAreas();   // (PedArrivals::AreaChanged, in PlacePlayer next: both sides' peds go on)
    // the doorway the other way: from this side's door (the old other side) back
    Doorway back;
    back.ok = true;
    std::memcpy(back.here, d.there, sizeof(back.here));
    std::memcpy(back.there, d.here, sizeof(back.there));
    std::memcpy(back.hinge, d.thereHinge, sizeof(back.hinge));
    std::memcpy(back.thereHinge, d.hinge, sizeof(back.thereHinge));
    back.in[0] = -d.thereIn[0];
    back.in[1] = -d.thereIn[1];
    back.thereIn[0] = -d.in[0];
    back.thereIn[1] = -d.in[1];
    back.along[0] = d.cosTurn * d.along[0] - d.sinTurn * d.along[1];
    back.along[1] = d.sinTurn * d.along[0] + d.cosTurn * d.along[1];
    back.width = d.width;
    back.height = d.height;
    back.cosTurn = d.cosTurn;
    back.sinTurn = -d.sinTurn;
    back.door = nullptr;   // (made by the switch: found when the preload is adopted)
    back.out[0] = g_preload.x;   // (the way in: where the door here led)
    back.out[1] = g_preload.y;
    back.out[2] = g_preload.z;
    // the main camera moved through as well (the camera controller is made to look the same way
    // after this; the frame drawn before it next has the camera where it was, seen from the new side)
    if (uint8_t* main = ms_pMainRwCamera) {
        float moved[13];
        MoveThrough(d, g_drawn ? g_drawnLtm : RwFrameGetLTM(RwCameraGetFrame(main)), moved);
        SetCameraFrame(main, moved);
        std::memcpy(g_cameraThrough, moved + 9, sizeof(g_cameraThrough));
        g_cameraPending = true;
    }
    // what the area left has around its door (loaded: it was just drawn) kept in memory while the
    // camera is still on that side: the new area's load would make room by letting it go
    std::vector<int16_t> pinned;
    ForEntitiesNear(d.here[0], d.here[1], 40.0, [&](uint8_t* e) {
        int type = e[0x108] & 7;
        int16_t model = At<int16_t>(e, 0x10E);
        if (e[0x111] != uint8_t(left) || !(type == 1 || type == 4 || type == 5) || model < 0) {
            return;
        }
        int index = Streaming_GetInfoIndex(model);
        uint8_t* info = index >= 0 ? ms_aInfoForModel + index * 0x10 : nullptr;
        if (info && info[4] == 1 && (info[5] & 3) == 0) {
            RequestModel(model, 1);   // (GAME_REQUIRED: not let go of)
            pinned.push_back(model);
        }
    });
    // the area left kept loaded around its door, as the extra scene, ready at once
    if (OurScene()) {
        ExtraScene_Clear();
    }
    g_preload = Preload{};
    SetScene(left, d.out);
    uint32_t now = m_snTimeInMilliseconds;
    g_preload.on = true;
    g_preload.area = left;
    g_preload.x = d.out[0];
    g_preload.y = d.out[1];
    g_preload.z = d.out[2];
    g_preload.since = now - kReadyMs;
    g_preload.adopt = true;
    g_preload.from = currArea;
    g_doorway = back;
    EndReverse();
    g_reverse = Reverse{};
    g_reverse.on = true;
    g_reverse.until = now + kReverseMs;
    g_reverse.since = now;
    g_reverse.pinned = std::move(pinned);
    if (DoorReport()) {
        std::fprintf(stderr, "doors: switched at %u ms: area %d (%s) kept around its door at (%.2f, %.2f, %.2f) (%zu models pinned), drawn around"
                     " the doorway at (%.2f, %.2f, %.2f) while the camera is behind\n",
                     now, left, AreaName(left), d.here[0], d.here[1], d.here[2], g_reverse.pinned.size(), back.here[0], back.here[1],
                     back.here[2]);
    }
}

bool SeamlessCameraFree() {
    return seamless::g_reverse.on;
}

bool SeamlessCameraPlace(float* position) {
    using namespace seamless;
    if (!g_cameraPending) {
        return false;
    }
    g_cameraPending = false;
    if (DoorReport()) {
        std::fprintf(stderr, "doors: the camera goes on from (%.2f, %.2f, %.2f) (not (%.2f, %.2f, %.2f))\n", g_cameraThrough[0],
                     g_cameraThrough[1], g_cameraThrough[2], position[0], position[1], position[2]);
    }
    std::memcpy(position, g_cameraThrough, sizeof(g_cameraThrough));
    return true;
}

// (AreaTransitionManager::UpdateTransition) a door into `area` needs no fade: what is behind it has
// been kept loaded for long enough
bool SeamlessDoorSkipsFade(int area) {
    using namespace seamless;
    bool skip = SeamlessDoors() && g_preload.on && g_preload.area == area && m_snTimeInMilliseconds - g_preload.since >= kReadyMs;
    g_goingIn = skip;
    if (skip) {
        g_traceUntil = m_snTimeInMilliseconds + 600;
    }
    if (DoorReport()) {
        std::fprintf(stderr, "doors: the door into area %d %s at %u ms\n", area, skip ? "goes on without a fade" : "fades as before",
                     m_snTimeInMilliseconds);
    }
    return skip;
}

// (AreaTransitionManager::UpdateTransition, state 9) the player is in the new area: without the
// fade this frame is seen, so an interior's lights are handed out now, not in the next frame's
void SeamlessDoorArrived() {
    using namespace seamless;
    g_keepPeds = false;
    g_cameraPending = false;   // (the camera's look-at made: it took it, or there was none)
    if (g_reverse.on && g_drawn && g_drawnFov > 0.0f && !ms_bFovOverride) {
        ms_fov = g_drawnFov;   // (as it was drawn: the follow camera eases it to the new area's from there)
    }
    if (g_reverse.on) {
        HideForReverse();   // (the doors the switch has just made, shut, in front of the camera)
    }
    if (!g_goingIn) {
        return;
    }
    g_goingIn = false;
    if (currArea == 0) {
        return;   // (the town's lighting is the time cycle's, already this frame's)
    }
    if (uint8_t* lights = GetLightManager()) {
        LightManager_Update(lights);
    }
}

bool SeamlessPopulationGhost(int* area, float* at) {
    using namespace seamless;
    uint8_t* player = PED_player;
    const Doorway& d = g_doorway;
    if (!SeamlessDoors() || !TwoAreas() || !Portals() || !g_preload.on || !d.ok || !PortalArea(g_preload.area) || !player ||
        At<uint8_t*>(player, 0x14) == nullptr || g_preload.area == currArea) {
        return false;
    }
    // (loaded for long enough: its collision there, or its peds would fall through)
    if (m_snTimeInMilliseconds - g_preload.since < kReadyMs) {
        return false;
    }
    const float* p = EntityPosition(player);
    ThroughPoint(d, p, at);
    // (the two places far apart: else the two would be one population, counted twice)
    float dx = at[0] - p[0], dy = at[1] - p[1];
    if (dx * dx + dy * dy < 120.0f * 120.0f) {
        return false;
    }
    *area = g_preload.area;
    return true;
}

bool SeamlessGhostCamera(bool on) {
    using namespace seamless;
    static uint8_t* saved = nullptr;
    uint8_t* camera2 = PortalCamera();
    if (on) {
        uint8_t* main = At<uint8_t*>(TheCamera, 0x8C);   // (CCamera's Gamebryo camera: 0xC3CCF4)
        if (!g_portalCameraReady || !g_doorway.ok || !main || main == camera2) {
            return false;
        }
        saved = main;
        float moved[13];
        MoveThrough(g_doorway, RwFrameGetLTM(RwCameraGetFrame(main)), moved);
        SetCameraFrame(camera2, moved);
        Camera_SetRwCamera(TheCamera, camera2);
    } else {
        if (!saved) {
            return false;
        }
        Camera_SetRwCamera(TheCamera, saved);
        saved = nullptr;
    }
    Camera_UpdateFrustumPlanes(TheCamera);
    Camera_Update_4F1F10(TheCamera);
    return true;
}

bool SeamlessKeepsPeds() {
    return seamless::g_keepPeds;
}

bool SeamlessSeenThroughDoorway(const float* p, float radius) {
    using namespace seamless;
    const Doorway& d = g_doorway;
    uint8_t* main = ms_pMainRwCamera;
    if (!d.ok || !main) {
        return true;
    }
    // the main camera, here; past this side's doorway (the camera behind the player, still on the
    // side left, after a switch): the area there is seen all round it, not through a doorway
    const float* eye = &At<float>(main, 0x84);
    if ((eye[0] - d.here[0]) * d.in[0] + (eye[1] - d.here[1]) * d.in[1] > 0.0f) {
        return true;
    }
    float a[3];
    ThroughPoint(d, eye, a);
    // where the line from the camera (there) to the place crosses the doorway's plane there
    float na = (a[0] - d.there[0]) * d.thereIn[0] + (a[1] - d.there[1]) * d.thereIn[1];
    float nb = (p[0] - d.there[0]) * d.thereIn[0] + (p[1] - d.there[1]) * d.thereIn[1];
    if (nb + radius <= 0.0f) {
        return false;   // (this side of the doorway there: not in the area)
    }
    if (na >= 0.0f || nb <= 0.0f) {
        return true;    // (the camera's place there past its doorway, or the place in the plane: as seen)
    }
    float t = na / (na - nb);
    float c[3] = {a[0] + (p[0] - a[0]) * t, a[1] + (p[1] - a[1]) * t, a[2] + (p[2] - a[2]) * t};
    float alongThere[2] = {d.cosTurn * d.along[0] - d.sinTurn * d.along[1], d.sinTurn * d.along[0] + d.cosTurn * d.along[1]};
    float across = (c[0] - d.there[0]) * alongThere[0] + (c[1] - d.there[1]) * alongThere[1];
    float up = c[2] - d.there[2];
    // (the sphere's radius, scaled to the crossing, widens the opening)
    float slack = radius * t;
    return std::fabs(across) <= d.width * 0.5f + slack && up >= -slack && up <= d.height + slack;
}

void UseDoor(int area) {
    using namespace seamless;
    uint8_t* atm = theTransitionManager;
    uint8_t* player = PED_player;
    const float* at = &At<float>(At<uint8_t*>(player, 0x14), 0x30);
    uint8_t* best = nullptr;
    double bestDistance = 1e30;
    for (int i = 0; i < At<int32_t>(atm, 8) && i < 100; i++) {
        uint8_t* trigger = At<uint8_t*>(atm, 0x10 + i * 8);
        uint8_t* route = At<uint8_t*>(atm, 0x14 + i * 8);
        if (!trigger || !route || int(int8_t(route[0xC])) != area) {
            continue;
        }
        double dx = At<float>(trigger, 4) - at[0], dy = At<float>(trigger, 8) - at[1];
        if (dx * dx + dy * dy < bestDistance) {
            bestDistance = dx * dx + dy * dy;
            best = trigger;
        }
    }
    if (!best) {
        std::fprintf(stderr, "doors: no door to area %d here\n", area);
        return;
    }
    std::fprintf(stderr, "doors: the door to area %d used at %u ms\n", area, m_snTimeInMilliseconds);
    ATM_SetTarget(atm, best);
}

} // namespace re
