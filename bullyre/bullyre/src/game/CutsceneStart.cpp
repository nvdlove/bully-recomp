// CCutsceneMgr::StartCutscene (0x6C6370): once the streams are ready, each cutscene object
// gets its anim: the HXD group named after the cutscene (NAME or NAME_...), the anim named
// NAME\OBJECT (a _W object without its _W part) started in slot 0, and for an object with
// a facial set the NAME\F_OBJECT anim in slot 1 over the "Facial" weight set; the object's
// placed at the anim's root translation and added to the world. Then the camera model,
// the bounding boxes, the clock and the flags. (5-03 sets 0xBCC120.)

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct CVector {
    float x, y, z;
};

GAME_ARRAY(char, css_name, 0x020C5B20);
GAME_ARRAY(char, css_path, 0x00C221A8);
GAME_ARRAY(uint8_t, css_facial, 0x020C4B80);
GAME_ARRAY(int32_t, css_modelIds, 0x020C4BA0);
GAME_ARRAY(char, css_names, 0x020C4C18);
GAME_ARRAY(uint8_t*, css_objects, 0x020C5B68);
GAME_VAR(uint32_t, css_createdCount, 0x020C5B18);
GAME_VAR(int32_t, css_objectCount, 0x020C5BF0);
GAME_VAR(uint32_t, css_lastClock, 0x020C5BFC);
GAME_VAR(uint8_t, css_clockStarted, 0x020C5BEB);
GAME_VAR(uint8_t, css_running, 0x020C5BE4);
GAME_VAR(uint8_t, css_cameraUsed, 0x020C5C09);
GAME_VAR(float, css_time, 0x020C5B1C);
GAME_VAR(uint8_t, css_flagBCC120, 0x00BCC120);
GAME_VAR(uint8_t, css_flagC1AE6E, 0x00C1AE6E);
GAME_VAR(float, css_cameraFloat, 0x00C3CD14);
GAME_VAR(int32_t, css_cameraMode, 0x00C3CD0C);
GAME_VAR(void*, css_streamMgr, 0x00CF4290);
GAME_VAR(void*, css_playerPed, 0x00C1AEA8);
inline void* const kManager = reinterpret_cast<void*>(0x020C4910);
inline void* const kClock = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const kTheCamera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(void, __cdecl, Css_InitNode, 0x006C3940);
GAME_FN(int, __thiscall, Css_StreamsReady, 0x005A6DA0, void* streams);
GAME_FN(void, __thiscall, Css_ClockService, 0x00597610, void* clock);
GAME_FN(void, __thiscall, Css_ClockStart, 0x00595A70, void* clock);
GAME_FN(void, __thiscall, Css_CameraFade, 0x004F2C20, void* camera, int a, int b, int c, int d, int e);
inline int __cdecl Css_Strnicmp(const char* a, const char* b, uint32_t n) { return crt::Strnicmp(a, b, (size_t)n); }
inline int __cdecl Css_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(uint32_t, __cdecl, Css_HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, Css_StartTrack, 0x006C0D40, void* manager, uint8_t* model, uintptr_t anim, void* track,
        int a, float c, float d);
GAME_FN(int, __thiscall, Css_SetMaskTrack, 0x006C0B20, void* manager, uint8_t* model, uint8_t* mask,
        re::Ptr32<uint8_t>* out, float fade, float weight, int slot);
GAME_FN(void*, __thiscall, Css_SlotTrack, 0x006B1F50, uint8_t* model, int slot);
GAME_FN(uint32_t, __cdecl, Css_AnimKind, 0x006B1A70, uint8_t* anim);
GAME_FN(float, __thiscall, Css_GetTranslation, 0x006B3E20, uint8_t* hxd, uint32_t anim, uint32_t joint,
        CVector* out, int key);
GAME_FN(void, __cdecl, Css_WorldAdd, 0x0045D6B0, void* entity);
GAME_FN(void, __cdecl, Css_SetupCameraModel, 0x006C3C00);
GAME_FN(void, __cdecl, Css_GenerateAllBoundingBoxes, 0x0045B960, int);
GAME_FN(void, __thiscall, Css_PedFn477520, 0x00477520, void* ped, int);

// the last '_' after the first character cut off
inline void CutLastUnderscore(char* s) {
    for (int32_t k = int32_t(std::strlen(s)); k > 0; k--) {
        if (s[k] == '_') {
            s[k] = 0;
            break;
        }
    }
}

inline bool EndsWithWide(const char* name) {
    int32_t n = int32_t(std::strlen(name));
    return name[n - 2] == '_' && name[n - 1] == 'W';
}

void __cdecl StartCutscene() {
    if (std::memcmp(css_name, "5-03", 5) == 0) {
        css_flagBCC120 = 1;
    }
    css_lastClock = 0;
    css_clockStarted = 0;
    Css_InitNode();
    while (Css_StreamsReady(css_streamMgr) == 0) {
        Css_ClockService(kClock);
    }
    if (css_cameraUsed != 0) {
        Css_CameraFade(kTheCamera, 3, 0, 1, 0, 0);
        css_cameraFloat = 0.0f;
        css_cameraMode = 1;
    }
    css_flagC1AE6E = 1;
    uint32_t nameLength = uint32_t(std::strlen(css_name));
    for (uint32_t o = 0; o < css_createdCount; o++) {
        uint8_t* object = css_objects[o];
        uint8_t* model = At<uint8_t*>(object, 0x114);
        uint8_t* hxd = At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10);
        uint32_t group = 0;
        uint32_t groups = At<uint32_t>(hxd, 0x68);
        if (groups != 0) {
            for (; group < At<uint32_t>(hxd, 0x68); group++) {
                auto groupName = [&]() -> char* {
                    return (group < At<uint32_t>(hxd, 0x68) ? reinterpret_cast<char*>(At<uint8_t*>(hxd, 0x6C) +
                                                                                   group * 0x3C)
                                                           : nullptr) + 4;
                };
                if (Css_Strnicmp(css_name, groupName(), nameLength) != 0) {
                    continue;
                }
                if (groupName()[nameLength] == '_' || groupName()[nameLength] == 0) {
                    break;
                }
            }
        }
        uint32_t body = 0;
        uint32_t facialAnim = 0;
        bool foundBody = false;
        bool foundFacial = false;
        for (int32_t k = 0; k < css_objectCount; k++) {
            const char* objectName = css_names + k * 0x40;
            if (int32_t(At<int16_t>(object, 0x10E)) == css_modelIds[k]) {
                for (uint32_t a = 0; a < At<uint32_t>(hxd, 4); a++) {
                    auto anim = [&]() -> uint8_t* {
                        return a < At<uint32_t>(hxd, 4) ? At<uint8_t*>(hxd, 0) + a * 0x40 : nullptr;
                    };
                    if (At<uint32_t>(anim(), 0x20) != group) {
                        continue;
                    }
                    char name[0x40];
                    if (!foundBody) {
                        std::strcpy(name, css_name);
                        std::strcat(name, "\\");
                        if (EndsWithWide(objectName)) {
                            std::strcpy(css_path, objectName);
                            CutLastUnderscore(css_path);
                            std::strcat(name, css_path);
                        } else {
                            std::strcat(name, objectName);
                        }
                        uint32_t hash = At<uint32_t>(anim(), 0x14);
                        if (hash == Css_HashName(name)) {
                            body = a;
                            foundBody = true;
                        }
                    }
                    if (!foundFacial && css_facial[k] != 0) {
                        std::strcpy(name, css_name);
                        std::strcat(name, "\\F_");
                        std::strcat(name, objectName);
                        if (EndsWithWide(objectName)) {
                            CutLastUnderscore(name);
                        }
                        uint32_t hash = At<uint32_t>(anim(), 0x14);
                        if (hash == Css_HashName(name)) {
                            facialAnim = a;
                            foundFacial = true;
                        }
                    }
                }
            }
            if (foundBody && (foundFacial || css_facial[k] == 0)) {
                break;
            }
        }
        Css_StartTrack(kManager, At<uint8_t*>(object, 0x114), body, nullptr, 1, 1.0f, -1.0f);
        if (foundFacial) {
            uint8_t* hxd2 = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(object, 0x114), 0x94), 0x10);
            uint8_t* mask = nullptr;
            for (uint32_t i = 0; i < At<uint32_t>(hxd2, 0xC); i++) {
                if (Css_Stricmp(reinterpret_cast<char*>(At<uint8_t*>(hxd2, 8) + i * 0x2C), "Facial") == 0) {
                    mask = At<uint8_t*>(hxd2, 8) + i * 0x2C;
                    break;
                }
            }
            re::Ptr32<uint8_t> track;
            Css_SetMaskTrack(kManager, At<uint8_t*>(object, 0x114), mask, &track, 0.0f, -1.0f, 1);
            void* slot = Css_SlotTrack(At<uint8_t*>(object, 0x114), 1);
            Css_StartTrack(kManager, At<uint8_t*>(object, 0x114), facialAnim, slot, 1, 1.0f, -1.0f);
        }
        CVector root = {0.0f, 0.0f, 0.0f};
        uint8_t* anim = body < At<uint32_t>(hxd, 4) ? At<uint8_t*>(hxd, 0) + body * 0x40 : nullptr;
        uint32_t kind = Css_AnimKind(anim);
        Css_GetTranslation(hxd, body, kind, &root, 0);
        uint8_t* matrix = At<uint8_t*>(object, 0x14);
        if (matrix != nullptr) {
            At<CVector>(matrix, 0x30) = root;
            At<uint32_t>(matrix, 0x3C) = 0;   // (the game copies an unset stack word here)
        }
        At<CVector>(object, 4) = root;
        At<uint32_t>(object, 0x338) = body;
        Css_WorldAdd(object);
    }
    Css_SetupCameraModel();
    Css_GenerateAllBoundingBoxes(0);
    Css_GenerateAllBoundingBoxes(0);
    css_time = 0.0f;
    css_running = 1;
    Css_PedFn477520(css_playerPed, 0);
    Css_ClockStart(kClock);
}

} // namespace

#define CUTSSTART_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CUTSSTART_REVERSED("CCutsceneMgr::StartCutscene", 0x006C6370, &StartCutscene);
