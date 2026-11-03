// CCutsceneMgr (the NIS player; GTA San Andreas' names where the code matches): its
// setup (CUTS\CUTS.DIR, the action controller at 0x20C5C0C), the action tree node
// (/Global/CutScenes), the cutscene audio (.RSM streams) and its clock, the subtitles
// shown by frame, the cutscene camera's model tied in, the objects made and removed
// (DeleteCutsceneData), the per-frame update, the prop anims kept for the next cutscene,
// the pause handling and the tidy-up when a cutscene doesn't fit in memory.
// 0x6C3720..0x6C45D3.

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

GAME_VAR(uint8_t, cs_everythingRemoved, 0x020C5BE0);
GAME_VAR(uint8_t, cs_loaded, 0x020C5BE1);
GAME_VAR(uint8_t, cs_flag5BE2, 0x020C5BE2);
GAME_VAR(uint8_t, cs_flag5BE3, 0x020C5BE3);
GAME_VAR(uint8_t, cs_running, 0x020C5BE4);
GAME_VAR(uint8_t, cs_syncStarted, 0x020C5BE5);
GAME_VAR(uint8_t, cs_audioPlaying, 0x020C5BE6);
GAME_VAR(uint8_t, cs_audioLoaded, 0x020C5BE7);
GAME_VAR(uint8_t, cs_flag5BE8, 0x020C5BE8);
GAME_VAR(uint8_t, cs_audioStreaming, 0x020C5BE9);
GAME_VAR(uint8_t, cs_flag5BEA, 0x020C5BEA);
GAME_VAR(uint8_t, cs_clockStarted, 0x020C5BEB);
GAME_VAR(int32_t, cs_objectCount, 0x020C5BF0);
GAME_VAR(re::Ptr32<void>*, cs_hierarchies, 0x020C5BF4);
GAME_VAR(uint32_t, cs_hierarchyCount, 0x020C5BF8);
GAME_VAR(uint32_t, cs_lastClock, 0x020C5BFC);
GAME_VAR(int32_t, cs_subtitleCount, 0x020C5C00);
GAME_VAR(uint8_t*, cs_subtitles, 0x020C5C04);   // 0x20 each: first frame, last frame, text
GAME_VAR(uint8_t, cs_cameraUsed, 0x020C5C09);
GAME_VAR(uint8_t*, cs_controller, 0x020C5C0C);
GAME_ARRAY(void*, cs_propAnims, 0x020C5C20);    // 10
GAME_VAR(uint32_t, cs_propAnimCount, 0x020C5C48);
GAME_VAR(float, cs_savedTimeScale, 0x020C5C4C);
GAME_VAR(uint8_t, cs_paused, 0x020C5C50);
GAME_VAR(uint32_t, cs_createdCount, 0x020C5B18);
GAME_VAR(float, cs_time, 0x020C5B1C);
GAME_ARRAY(uint8_t*, cs_objects, 0x020C5B68);   // 30
GAME_ARRAY(char, cs_objectNames, 0x020C4C18);   // 0x40 each
GAME_VAR(void*, cs_node, 0x020C4B30);
GAME_VAR(void*, cs_directory, 0x020C4B34);
GAME_ARRAY(uint8_t*, cs_keptHxds, 0x020C4B38);  // 10, null-ended
GAME_VAR(int32_t, cs_subtitleIndex, 0x00BB64EC);
GAME_ARRAY(void*, cs_actionContext, 0x00BB6500);
GAME_VAR(float, cs_msPerStep, 0x00BB64F0);
GAME_VAR(float, cs_timeStep, 0x00C1A9A0);
GAME_VAR(float, cs_timeScale, 0x00C1A9A8);
GAME_VAR(uint8_t, cs_flagBCC120, 0x00BCC120);
GAME_VAR(uint8_t, cs_subtitlesOn, 0x00C1870C);
GAME_VAR(uint8_t, cs_subtitlesOff, 0x00C18720);
GAME_VAR(uint8_t, cs_frontEndA, 0x020C32E9);
GAME_VAR(uint8_t, cs_frontEndB, 0x020C32EA);
GAME_VAR(uint8_t, cs_pausedGame, 0x00C1A998);
GAME_VAR(uint8_t, cs_specialEntities, 0x00CA3984);
GAME_VAR(uint8_t, cs_flagC1AE6E, 0x00C1AE6E);
GAME_VAR(uint8_t*, cs_playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, cs_firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, cs_lastWeapon, 0x00A136B4);
GAME_VAR(int32_t, cs_firstModel, 0x00A136D0);
GAME_VAR(int32_t, cs_lastModel, 0x00A136D4);
GAME_ARRAY(uint8_t*, cs_modelInfos, 0x00C67738);
GAME_VAR(void*, cs_defaultColModel, 0x00CF312C);
GAME_VAR(uint8_t*, cs_camera, 0x00C3CFC4);   // the cutscene camera controller
GAME_VAR(void*, cs_hud, 0x00C67548);
GAME_VAR(void*, cs_radar, 0x00C674CC);
GAME_VAR(void*, cs_pedPopulation, 0x00C2C108);
GAME_VAR(uint8_t*, cs_vehiclePool, 0x00C0F5F4);
GAME_VAR(void*, cs_streamMgr, 0x00CF4290);
GAME_ARRAY(uint32_t, cs_zeroVector, 0x00BCC564);
inline void* const kClock = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const kStreams = reinterpret_cast<void*>(0x00CF4284);
inline void* const kManager = reinterpret_cast<void*>(0x020C4910);
inline void* const kTheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const kUserInput = reinterpret_cast<void*>(0x00A45A30);
inline void* const kPlayerInfo = reinterpret_cast<void*>(0x00C1AEA8);
inline void* const kText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const kCameraModelCallback = reinterpret_cast<void*>(0x004F7D10);
inline const char* const kTidyUp = "CCutsceneMgr::RemoveEverythingBecauseCutsceneDoesntFitInMemory()";
inline const char* const kDelete = "CCutsceneMgr::DeleteCutsceneData()";

GAME_FN(void*, __cdecl, Cs_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Cs_Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, Cs_DirectoryConstruct, 0x004291B0, void* self, int32_t capacity);
GAME_FN(void, __thiscall, Cs_DirectoryRead, 0x004291F0, void* self, const char* path);
GAME_FN(uint8_t*, __thiscall, Cs_ControllerConstruct, 0x005F3E50, void* self);
GAME_FN(void, __thiscall, Cs_ControllerDestruct, 0x005F40F0, void* self);
GAME_FN(void, __thiscall, Cs_ControllerStop, 0x005F40A0, void* self);
GAME_FN(void, __thiscall, Cs_ControllerSetNode, 0x005F45F0, void* self, void* node);
GAME_FN(void, __thiscall, Cs_ControllerUpdate, 0x005F4B00, void* self, float dt);
GAME_FN(void*, __cdecl, Cs_FindNode, 0x005F5B10, const char* path, uintptr_t);
GAME_FN(void, __cdecl, Cs_LoadActFile, 0x005F6DB0, const char* path, uintptr_t, uintptr_t);
inline char* __cdecl Cs_Strrchr(const char* s, int c) { return crt::Strrchr(s, c); }
inline int __cdecl Cs_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline int __cdecl Cs_Strnicmp(const char* a, const char* b, uint32_t n) { return crt::Strnicmp(a, b, (size_t)n); }
GAME_FN(void, __thiscall, Cs_LoadSound, 0x005951C0, void* clock, const char* name, float volume);
GAME_FN(int32_t, __thiscall, Cs_ClockMs, 0x00595B30, void* clock);
GAME_FN(void, __thiscall, Cs_ClockStart, 0x00595AD0, void* clock);
GAME_FN(void, __thiscall, Cs_ClockStop, 0x00595290, void* clock);
GAME_FN(void, __thiscall, Cs_ClockService, 0x00597610, void* clock);
GAME_FN(void, __thiscall, Cs_ShowSubtitle, 0x0055B130, void* hud, int, const void* text);
GAME_FN(void, __cdecl, Cs_StreamsPrepare, 0x005A5DB0);
GAME_FN(int, __thiscall, Cs_StreamState, 0x00773DB0, void* streams);
GAME_FN(void, __thiscall, Cs_StreamsService, 0x005A6EA0, void* streams);
GAME_FN(void, __thiscall, Cs_StreamsPlay, 0x005A6170, void* streams, const char* name, float volume, int, int);
GAME_FN(void, __thiscall, Cs_StreamsStop, 0x005A6210, void* streams, int);
GAME_FN(void, __thiscall, Cs_StreamsSetMode, 0x005A6D20, void* streams, uintptr_t, int);
GAME_FN(uint32_t, __cdecl, Cs_HashName, 0x00576ED0, const char* name);
GAME_FN(void*, __thiscall, Cs_FindJoint, 0x00465580, void* hierarchy, uint32_t hash, int32_t* index);
GAME_FN(int, __thiscall, Cs_AttachNode, 0x006C2070, void* manager, uint8_t* model, int32_t bone, void* pose,
        re::Ptr32<void>* out, void* data, uint8_t* child, void* transform);
GAME_FN(int, __thiscall, Cs_ReleaseHierarchyObject, 0x006C1D00, void* manager, void* hierarchy);
GAME_FN(void, __cdecl, Cs_LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);
GAME_FN(void, __cdecl, Cs_DrasticTidyUpMemory, 0x004304E0, int);
GAME_FN(float*, __cdecl, Cs_FindPlayerCoors, 0x00446900, float* out);
GAME_FN(void, __thiscall, Cs_RemovePedsInRadius, 0x0049F020, void* population, float x, float y, float z,
        float radius);
GAME_FN(void, __cdecl, Cs_ClearArea, 0x0045EC50, const void* position, float radius, int);
GAME_FN(void, __cdecl, Cs_RemoveAllCollision, 0x0056F260);
GAME_FN(void, __thiscall, Cs_RadarClear, 0x005492E0, void* radar);
GAME_FN(int32_t, __thiscall, Cs_InventoryFind, 0x0045CF30, void* inventory, int32_t weapon);
GAME_FN(void, __cdecl, Cs_RemoveWeaponModel, 0x0052FE40, int32_t model);
GAME_FN(void, __cdecl, Cs_DeleteSectorRwObjects, 0x0052F0E0);
GAME_FN(void, __cdecl, Cs_RemoveUnusedModels, 0x00530B50);
GAME_FN(void, __cdecl, Cs_Fn45B820, 0x0045B820);
GAME_FN(void, __cdecl, Cs_GenerateAllBoundingBoxes, 0x0045B960, int);
GAME_FN(void, __cdecl, Cs_Fn738780, 0x00738780);
GAME_FN(void, __cdecl, Cs_Fn45B810, 0x0045B810);
GAME_FN(void, __cdecl, Cs_Fn530040, 0x00530040, uint32_t slot);
GAME_FN(void, __cdecl, Cs_WorldRemove, 0x0045DD70, void* entity);
GAME_FN(void, __thiscall, Cs_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(void, __cdecl, Cs_RemoveModel, 0x0052F640, int32_t id);
GAME_FN(void, __thiscall, Cs_PedSetName, 0x00522220, void* mi, const char* name, int);
GAME_FN(void, __thiscall, Cs_ColRemoveVolumes, 0x0056E7D0, void* col);
GAME_FN(void, __cdecl, Cs_ColDelete, 0x0056EB30, void* col);
GAME_FN(void, __thiscall, Cs_SetColModel, 0x0050EB30, void* mi, void* col, int owns);
GAME_FN(void, __thiscall, Cs_CameraInit, 0x004F3720, void* camera);
GAME_FN(void, __thiscall, Cs_CameraFreeTracks, 0x004F7AC0, void* controller);
GAME_FN(void, __thiscall, Cs_UserInputSetFlag, 0x0050DFB0, void* input, int flag);
GAME_FN(void, __thiscall, Cs_PlayerSetPaused, 0x00445DA0, void* info, int paused);
GAME_FN(void, __cdecl, Cs_Fn45B800, 0x0045B800);
GAME_FN(int, __thiscall, Cs_CameraFadeState, 0x004F12F0, void* camera);
GAME_FN(void, __thiscall, Cs_TextFn68F520, 0x0068F520, void* text);
GAME_FN(void, __cdecl, Cs_RequestModel, 0x00532020, int32_t id, int flags);
GAME_FN(void, __cdecl, Cs_PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, Cs_PopTxdMemoryTag, 0x0052E710);
GAME_FN(void*, __cdecl, Cs_ColNew, 0x0056E7C0, uintptr_t size);
GAME_FN(void, __thiscall, Cs_ColConstruct, 0x0056E7A0, void* col);
GAME_FN(void*, __cdecl, Cs_ObjectNew, 0x004CF0B0, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, Cs_ObjectConstruct, 0x004CEE60, void* self);
GAME_FN(void, __cdecl, Cs_InitNode, 0x006C3940);
GAME_FN(void, __cdecl, Cs_SetNode, 0x006C39A0, void* node);
GAME_FN(void, __cdecl, Cs_PauseCheck, 0x006C3F50);

template <typename R, typename... A>
inline R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}

// 0x6C3720: the setup: the flags cleared, CUTS\CUTS.DIR read (0x360 entries), the kept
// HXDs and objects cleared, the action controller made
void __cdecl Init() {
    cs_loaded = 0;
    cs_running = 0;
    cs_syncStarted = 0;
    cs_flag5BE3 = 0;
    cs_audioPlaying = 0;
    cs_audioLoaded = 0;
    cs_flag5BE8 = 0;
    cs_flag5BEA = 0;
    cs_flag5BE2 = 0;
    cs_objectCount = 0;
    cs_createdCount = 0;
    void* directory = Cs_Malloc(0x10);
    if (directory != nullptr) {
        Cs_DirectoryConstruct(directory, 0x360);
        *static_cast<uint32_t*>(directory) = 0x009186D0;
    }
    cs_directory = directory;
    Cs_DirectoryRead(directory, "CUTS\\CUTS.DIR");
    for (int i = 0; i < 10; i++) {
        cs_keptHxds[i] = nullptr;
    }
    for (int i = 0; i < 30; i++) {
        cs_objects[i] = nullptr;
    }
    void* controller = Cs_Malloc(0x38);
    if (controller != nullptr) {
        cs_controller = Cs_ControllerConstruct(controller);
        return;
    }
    cs_controller = nullptr;
}

// 0x6C38A0: the action tree node forgotten
void __cdecl ForgetNode() {
    cs_node = nullptr;
}

// 0x6C38B0: the cutscene's sound (its file name, extension .rsm) loaded
void __cdecl LoadCutsceneSound(const char* path) {
    char* back = Cs_Strrchr(path, '\\');
    char* forward = Cs_Strrchr(path, '/');
    if (forward > back) {
        back = forward;
    }
    const char* name = back != nullptr ? back + 1 : path;
    char local[0x100];
    std::strcpy(local, name);
    std::strcat(local, ".rsm");
    Cs_LoadSound(kClock, local, 1.0f);
    cs_audioLoaded = 1;
}

// the /Global/CutScenes node (loading CutScene.act when it's missing), the controller
// given the action context
inline void FindNode() {
    if (cs_node != nullptr) {
        return;
    }
    cs_node = Cs_FindNode("/Global/CutScenes", 0);
    if (cs_node == nullptr) {
        Cs_LoadActFile("/Act/Globals/CutScene.act", 0, 0);
        cs_node = Cs_FindNode("/Global/CutScenes", 0);
    }
    cs_actionContext[0] = cs_controller;
    At<void*>(cs_controller, 0x18) = cs_actionContext;
}

// 0x6C3940: the clock's restart pending, the node found
void __cdecl InitNode() {
    cs_clockStarted = 0;
    FindNode();
}

// 0x6C39A0: the controller set to the node's (the node's vf 0x4C of the action context)
void __cdecl SetNode(void* node) {
    FindNode();
    void* target = VCall<void*>(node, 0x4C, static_cast<void*>(cs_actionContext));
    Cs_ControllerSetNode(cs_controller, target);
}

// 0x6C3A10: the subtitle for the audio clock's frame (30 a second) shown, stepping to the
// next one past the current one's end
void __cdecl UpdateSubtitles() {
    if (cs_running == 0) {
        return;
    }
    if (cs_subtitlesOn == 0 && cs_subtitlesOff != 0) {
        return;
    }
    if (cs_subtitles == nullptr) {
        return;
    }
    int32_t index = cs_subtitleIndex;
    if (index < 0 || index >= cs_subtitleCount) {
        return;
    }
    uint8_t* subtitle = cs_subtitles + index * 0x20;
    int32_t ms = Cs_ClockMs(kClock);
    double t = double(ms);
    if (ms < 0) {
        t = double(float(t + 4294967296.0));
    }
    float seconds = float(t / 1000.0);
    int32_t frame = int32_t(int64_t(float(double(seconds) * 30.0)));
    if (frame > At<int32_t>(subtitle, 4)) {
        cs_subtitleIndex = cs_subtitleIndex + 1;
        subtitle = cs_subtitles + cs_subtitleIndex * 0x20;
    }
    if (cs_subtitleIndex < cs_subtitleCount && frame >= At<int32_t>(subtitle, 0) &&
        frame <= At<int32_t>(subtitle, 4)) {
        Cs_ShowSubtitle(cs_hud, 0, subtitle + 8);
    }
}

// 0x6C3AD0: the cutscene's time in ms
int32_t __cdecl GetCutsceneTimeMs() {
    float ms = cs_time * 1000.0f;
    return int32_t(int64_t(ms));
}

// 0x6C3B10: the cutscene audio streamed (once the streams are idle: serviced every 5 ms)
void __cdecl PlayCutsceneAudio(const char* name, float volume, int a) {
    Cs_StreamsPrepare();
    while (Cs_StreamState(cs_streamMgr) != 2) {
        Cs_StreamsService(cs_streamMgr);
        Cs_ClockService(kClock);
        re::PtrCast<void(__stdcall*)(uint32_t)>(*reinterpret_cast<re::Ptr32<void>*>(0x008FC16C))(5);
    }
    Cs_StreamsPlay(kStreams, name, volume, 2, a);
    cs_audioStreaming = 1;
    cs_audioPlaying = 1;
    cs_audioLoaded = 1;
}

// 0x6C3B90: a synced (scripted) cutscene started: the node, the streams, the clock
void __cdecl BeginSync() {
    if (cs_running != 0) {
        return;
    }
    Cs_InitNode();
    Cs_StreamsSetMode(cs_streamMgr, 0, 1);
    Cs_ClockStart(kClock);
    cs_running = 1;
    cs_syncStarted = 1;
}

// 0x6C3BD0: a synced cutscene ended
void __cdecl EndSync() {
    Cs_StreamsStop(kStreams, 2);
    cs_audioStreaming = 0;
    cs_running = 0;
    cs_audioPlaying = 0;
    cs_audioLoaded = 0;
    cs_syncStarted = 0;
}

// 0x6C3C00: the jd_cam model's anim driving the cutscene camera (its callback), and a
// cs_car tied to it (its joint1 to the camera's joint0)
void __cdecl SetupCameraModel() {
    uint8_t* camera = nullptr;
    for (int i = 0; i < 30; i++) {
        uint8_t* object = cs_objects[i];
        if (object == nullptr) {
            continue;
        }
        uint8_t* model = At<uint8_t*>(object, 0x114);
        if (model == nullptr) {
            continue;
        }
        uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
        if (At<uint32_t>(hierarchy, 0x14) == Cs_HashName("Cuts\\Models\\jd_cam")) {
            At<void*>(model, 4) = kCameraModelCallback;
            At<uint8_t*>(model, 8) = cs_camera;
            At<uint8_t*>(cs_camera, 0xB8) = model;
            camera = model;
            At<uint32_t>(object, 0xD0) = 1;
        }
    }
    if (camera == nullptr) {
        return;
    }
    for (int i = 0; i < 30; i++) {
        uint8_t* object = cs_objects[i];
        if (object == nullptr) {
            continue;
        }
        uint8_t* model = At<uint8_t*>(object, 0x114);
        if (model == nullptr) {
            continue;
        }
        uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
        if (Cs_Stricmp(reinterpret_cast<char*>(hierarchy + 0x1C), "Cuts\\Models\\cs_car") != 0) {
            continue;
        }
        uint32_t joint0 = Cs_HashName("joint0");
        uint32_t joint1 = Cs_HashName("joint1");
        int32_t index;
        void* cameraJoint = Cs_FindJoint(At<void*>(camera, 0x94), joint0, &index);
        void* carJoint = Cs_FindJoint(hierarchy, joint1, &index);
        if (carJoint != nullptr && cameraJoint != nullptr) {
            re::Ptr32<void> node = nullptr;
            Cs_AttachNode(kManager, model, index, nullptr, &node, cameraJoint, camera, nullptr);
        }
    }
}

// 0x6C3D50: the (last) object named CS_PLAY* while the audio plays
uint8_t* __cdecl GetPlayObject() {
    if (cs_audioPlaying == 0) {
        return nullptr;
    }
    int32_t found = -1;
    for (int32_t i = 0; i < cs_objectCount; i++) {
        if (Cs_Strnicmp(cs_objectNames + i * 0x40, "CS_PLAY", 7) == 0) {
            found = i;
        }
    }
    if (found > -1) {
        return cs_objects[found];
    }
    return nullptr;
}

// 0x6C3DB0: the shutdown: the directory and the controller deleted
void __cdecl Shutdown() {
    if (cs_directory != nullptr) {
        VCall<void>(cs_directory, 0, 1);
    }
    void* controller = cs_controller;
    if (controller != nullptr) {
        Cs_ControllerDestruct(controller);
        Cs_Free(controller);
    }
}

// 0x6C3DE0: CCutsceneMgr::RemoveEverythingBecauseCutsceneDoesntFitInMemory: once, peds
// near the player, the area, collision, the radar and the player's weapon models removed
void __cdecl RemoveEverythingBecauseCutsceneDoesntFitInMemory() {
    if (cs_everythingRemoved != 0) {
        Cs_LoadingScreen(kTidyUp, "bEverythingRemoved", 0);
        Cs_DrasticTidyUpMemory(1);
        return;
    }
    Cs_LoadingScreen(kTidyUp, "Start", 0);
    float coors[3];
    const float* at = Cs_FindPlayerCoors(coors);
    Cs_RemovePedsInRadius(cs_pedPopulation, at[0], at[1], at[2], 120.0f);
    Cs_ClearArea(cs_zeroVector, 4000.0f, 1);
    Cs_RemoveAllCollision();
    if (cs_radar != nullptr) {
        Cs_RadarClear(cs_radar);
    }
    cs_everythingRemoved = 1;
    if (cs_playerPed != nullptr) {
        void* inventory = At<void*>(cs_playerPed, 0x1C4);
        for (int32_t weapon = cs_firstWeapon; weapon <= cs_lastWeapon; weapon++) {
            if (Cs_InventoryFind(inventory, weapon) != -1) {
                Cs_RemoveWeaponModel(weapon);
            }
        }
    }
    Cs_DeleteSectorRwObjects();
    Cs_RemoveUnusedModels();
    Cs_LoadingScreen(kTidyUp, "CGame::DrasticTidyUpMemory()", 0);
    Cs_DrasticTidyUpMemory(1);
    Cs_LoadingScreen(kTidyUp, "End", 0);
}

// 0x6C3F00: the controller set to a node (loading its act file first when needed)
uint8_t __cdecl SetActionNode(const char* path, const char* actFile) {
    void* node = Cs_FindNode(path, 0);
    if (node == nullptr) {
        if (actFile == nullptr) {
            return 0;
        }
        Cs_LoadActFile(actFile, 0, 0);
        node = Cs_FindNode(path, 0);
        if (node == nullptr) {
            return 0;
        }
    }
    Cs_SetNode(node);
    return 1;
}

// 0x6C3F50: the game paused during a cutscene (the time scale saved and zeroed), resumed
void __cdecl CheckPause() {
    uint8_t paused = cs_paused;
    if (cs_audioPlaying == 0 && paused == 0) {
        return;
    }
    if (cs_frontEndA == 0 && cs_frontEndB == 0 && cs_pausedGame == 0) {
        if (paused == 0) {
            return;
        }
        Cs_Fn45B820();
        Cs_GenerateAllBoundingBoxes(0);
        cs_timeScale = cs_savedTimeScale;
        cs_paused = 0;
        return;
    }
    if (paused != 0) {
        return;
    }
    Cs_Fn738780();
    Cs_Fn45B810();
    cs_savedTimeScale = cs_timeScale;
    cs_paused = 1;
    cs_timeScale = 0.0f;
}

// 0x6C3FD0: a prop anim kept for the next cutscene (10 at most)
void __cdecl AddPropAnim(void* prop) {
    if (prop != nullptr && cs_propAnimCount != 10) {
        cs_propAnims[cs_propAnimCount] = prop;
        cs_propAnimCount += 1;
    }
}

// 0x6C4000
uint32_t __cdecl NumPropAnims() {
    return cs_propAnimCount;
}

// 0x6C4010
void* __cdecl GetPropAnim(uint32_t index) {
    if (index < cs_propAnimCount) {
        return cs_propAnims[index];
    }
    return nullptr;
}

// 0x6C4030: CCutsceneMgr::DeleteCutsceneData: the controller stopped; for a loaded
// cutscene the objects removed, the models and HXDs let go, the collision of the cutscene
// models' infos put back, the camera reset, the player given back control and his weapon
// models requested
void __cdecl DeleteCutsceneData() {
    cs_flagBCC120 = 0;
    Cs_LoadingScreen(kDelete, "Start", 0);
    cs_everythingRemoved = 0;
    Cs_ControllerStop(cs_controller);
    if (cs_loaded == 0) {
        return;
    }
    cs_audioPlaying = 0;
    cs_audioLoaded = 0;
    cs_flag5BE8 = 0;
    Cs_ClearArea(cs_zeroVector, 4000.0f, 1);
    cs_propAnimCount = 0;
    for (uint32_t i = 0; i < 30; i++) {
        Cs_Fn530040(i);
        if (cs_objects[i] != nullptr) {
            Cs_WorldRemove(cs_objects[i]);
            Cs_DeleteRwObject(cs_objects[i]);
            if (cs_objects[i] != nullptr) {
                VCall<void>(cs_objects[i], 0, 1);
            }
            cs_objects[i] = nullptr;
        }
    }
    for (uint32_t i = 0; i < 30; i++) {
        Cs_RemoveModel(cs_firstModel + int32_t(i));
        Cs_PedSetName(cs_modelInfos[cs_firstModel + int32_t(i)], nullptr, 1);
    }
    if (cs_createdCount < 30) {
        for (uint32_t i = cs_createdCount; i < 30; i++) {
            cs_objects[i] = nullptr;
        }
    }
    cs_createdCount = 0;
    for (int i = 0; cs_keptHxds[i] != nullptr; i++) {
        At<uint32_t>(cs_keptHxds[i], 0x10) &= 0xFFFFFFFBu;
        cs_keptHxds[i] = nullptr;
    }
    for (uint32_t i = 0; i < cs_hierarchyCount; i++) {
        uint8_t* hierarchy = static_cast<uint8_t*>(cs_hierarchies[i]);
        if (At<void*>(hierarchy, 8) != nullptr) {
            At<uint32_t>(hierarchy, 4) &= 0xFFFFFFFBu;
        } else {
            At<uint32_t>(hierarchy, 4) &= 0xFFFFFFF3u;
        }
        Cs_ReleaseHierarchyObject(kManager, cs_hierarchies[i]);
    }
    if (cs_hierarchies != nullptr) {
        Cs_Free(cs_hierarchies);
    }
    cs_hierarchies = nullptr;
    cs_hierarchyCount = 0;
    for (int32_t id = cs_firstModel; id <= cs_lastModel; id++) {
        uint8_t* mi = cs_modelInfos[id];
        void* col = At<void*>(mi, 0xC);
        if (col == cs_defaultColModel) {
            continue;
        }
        if (col != nullptr) {
            Cs_ColRemoveVolumes(col);
            Cs_ColDelete(col);
        }
        Cs_SetColModel(mi, cs_defaultColModel, 0);
    }
    cs_flag5BE3 = 0;
    if (cs_cameraUsed != 0) {
        Cs_CameraInit(kTheCamera);
        Cs_CameraFreeTracks(cs_camera);
    }
    cs_loaded = 0;
    cs_running = 0;
    if (cs_playerPed != nullptr) {
        At<uint32_t>(cs_playerPed, 0x48) = 1;
        Cs_UserInputSetFlag(kUserInput, 1);
        Cs_PlayerSetPaused(kPlayerInfo, 0);
    }
    cs_specialEntities = 0;
    cs_flagC1AE6E = 0;
    Cs_Fn45B800();
    if (cs_cameraUsed != 0) {
        int fade = Cs_CameraFadeState(kTheCamera);
        Cs_LoadingScreen(kDelete, "CGame::DrasticTidyUpMemory()", 0);
        Cs_DrasticTidyUpMemory(fade == 1 ? 1 : 0);
    }
    if (cs_subtitles != nullptr) {
        Cs_Free(cs_subtitles);
        cs_subtitles = nullptr;
    }
    cs_subtitleCount = 0;
    cs_subtitleIndex = -1;
    Cs_TextFn68F520(kText);
    if (cs_playerPed != nullptr) {
        void* inventory = At<void*>(cs_playerPed, 0x1C4);
        for (int32_t weapon = cs_firstWeapon; weapon <= cs_lastWeapon; weapon++) {
            if (Cs_InventoryFind(inventory, weapon) != -1) {
                Cs_RequestModel(weapon, 1);
            }
        }
    }
    Cs_GenerateAllBoundingBoxes(0);
    Cs_ClockStop(kClock);
    Cs_LoadingScreen(kDelete, "End", 0);
}

// 0x6C4370: a cutscene object made for a model (a cutscene model's info gets a 2-unit
// collision box, its own one made first when it has the default)
uint8_t* __cdecl CreateCutsceneObject(int32_t id) {
    Cs_PushTxdMemoryTag();
    if (id >= cs_firstModel && id <= cs_lastModel) {
        uint8_t* mi = cs_modelInfos[id];
        if (At<void*>(mi, 0xC) == cs_defaultColModel) {
            void* made = Cs_ColNew(0x40);
            uint8_t* col = nullptr;
            if (made != nullptr) {
                Cs_ColConstruct(made);
                col = static_cast<uint8_t*>(made);
            }
            At<float>(col, 0xC) = 2.0f;
            At<float>(col, 0) = 0.0f;
            At<float>(col, 4) = 0.0f;
            At<float>(col, 8) = 0.0f;
            Cs_SetColModel(mi, col, 1);
        }
        uint8_t* col = At<uint8_t*>(mi, 0xC);
        At<float>(col, 0xC) = 2.0f;
        At<float>(col, 0x10) = -2.0f;
        At<float>(col, 0x14) = -2.0f;
        At<float>(col, 0x18) = -2.0f;
        At<uint32_t>(col, 0x1C) = 0;   // (the game copies an unset stack word here)
        At<float>(col, 0x20) = 2.0f;
        At<float>(col, 0x24) = 2.0f;
        At<float>(col, 0x28) = 2.0f;
        At<uint32_t>(col, 0x2C) = 0;   // (and here)
    }
    void* made = Cs_ObjectNew(0x33C);
    uint8_t* object = made != nullptr ? Cs_ObjectConstruct(made) : nullptr;
    VCall<void>(object, 0x20, id, 1);
    cs_objects[cs_createdCount] = object;
    cs_createdCount += 1;
    Cs_PopTxdMemoryTag();
    return object;
}

// 0x6C44B0: the per-frame update: the controller stepped by the audio clock's time since
// last frame, every cutscene object and the vehicles in cutscene state (+0x46C == 1)
// processed, the cutscene time advanced; then the pause check
void __cdecl Update() {
    if (cs_running != 0 && cs_frontEndA == 0 && cs_frontEndB == 0) {
        int32_t ms;
        if (cs_clockStarted != 0) {
            ms = Cs_ClockMs(kClock) - int32_t(cs_lastClock);
        } else {
            cs_clockStarted = 1;
            ms = Cs_ClockMs(kClock);
        }
        double t = double(ms);
        if (ms < 0) {
            t = double(float(t + 4294967296.0));
        }
        float dt = float(t / 1000.0);
        cs_lastClock = uint32_t(Cs_ClockMs(kClock));
        Cs_ControllerUpdate(cs_controller, dt);
        for (int i = 0; i < 30; i++) {
            if (cs_objects[i] != nullptr) {
                VCall<void>(cs_objects[i], 0x4C);
            }
        }
        uint8_t* pool = cs_vehiclePool;
        int32_t n = At<int32_t>(pool, 8);
        while (n != 0) {
            n--;
            if (At<uint8_t*>(pool, 4)[n] & 0x80) {
                continue;
            }
            uint8_t* vehicle = At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * n;
            if (vehicle == nullptr || At<int32_t>(vehicle, 0x46C) != 1) {
                continue;
            }
            VCall<void>(vehicle, 0x4C);
            VCall<void>(vehicle, 0x50);
        }
        float step = cs_msPerStep * cs_timeStep;
        cs_time = step + cs_time;
    }
    Cs_PauseCheck();
}

} // namespace

#define CUTSMGR_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CUTSMGR_REVERSED("CCutsceneMgr::Init", 0x006C3720, &Init);
CUTSMGR_REVERSED("CCutsceneMgr::ForgetNode", 0x006C38A0, &ForgetNode);
CUTSMGR_REVERSED("CCutsceneMgr::LoadCutsceneSound", 0x006C38B0, &LoadCutsceneSound);
CUTSMGR_REVERSED("CCutsceneMgr::InitNode", 0x006C3940, &InitNode);
CUTSMGR_REVERSED("CCutsceneMgr::SetNode", 0x006C39A0, &SetNode);
CUTSMGR_REVERSED("CCutsceneMgr::UpdateSubtitles", 0x006C3A10, &UpdateSubtitles);
CUTSMGR_REVERSED("CCutsceneMgr::GetCutsceneTimeMs", 0x006C3AD0, &GetCutsceneTimeMs);
CUTSMGR_REVERSED("CCutsceneMgr::PlayCutsceneAudio", 0x006C3B10, &PlayCutsceneAudio);
CUTSMGR_REVERSED("CCutsceneMgr::BeginSync", 0x006C3B90, &BeginSync);
CUTSMGR_REVERSED("CCutsceneMgr::EndSync", 0x006C3BD0, &EndSync);
CUTSMGR_REVERSED("CCutsceneMgr::SetupCameraModel", 0x006C3C00, &SetupCameraModel);
CUTSMGR_REVERSED("CCutsceneMgr::GetPlayObject", 0x006C3D50, &GetPlayObject);
CUTSMGR_REVERSED("CCutsceneMgr::Shutdown", 0x006C3DB0, &Shutdown);
CUTSMGR_REVERSED("CCutsceneMgr::RemoveEverythingBecauseCutsceneDoesntFitInMemory", 0x006C3DE0,
                 &RemoveEverythingBecauseCutsceneDoesntFitInMemory);
CUTSMGR_REVERSED("CCutsceneMgr::SetActionNode", 0x006C3F00, &SetActionNode);
CUTSMGR_REVERSED("CCutsceneMgr::CheckPause", 0x006C3F50, &CheckPause);
CUTSMGR_REVERSED("CCutsceneMgr::AddPropAnim", 0x006C3FD0, &AddPropAnim);
CUTSMGR_REVERSED("CCutsceneMgr::NumPropAnims", 0x006C4000, &NumPropAnims);
CUTSMGR_REVERSED("CCutsceneMgr::GetPropAnim", 0x006C4010, &GetPropAnim);
CUTSMGR_REVERSED("CCutsceneMgr::DeleteCutsceneData", 0x006C4030, &DeleteCutsceneData);
CUTSMGR_REVERSED("CCutsceneMgr::CreateCutsceneObject", 0x006C4370, &CreateCutsceneObject);
CUTSMGR_REVERSED("CCutsceneMgr::Update", 0x006C44B0, &Update);
