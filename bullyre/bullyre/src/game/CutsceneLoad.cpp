// CCutsceneMgr::LoadCutsceneData (0x6C45E0): a cutscene's data from CUTS\CUTS.IMG, either
// one packed NAME.CUT (NAME_W.CUT in the chapter-2 area when there is one) read whole into
// the 0x1C000-byte buffer at 0xBD7800 (the object names, then SUB / HXD / MXD / DAT
// blocks: a 4-byte tag, a 0x40-byte name and the block's size before each), or the loose
// files (NAME.INF's object list, each object's .mxd (_W.mxd in chapter 2) with its HXD, the
// groups' .AGR, NAME.DAT, NAME.SUB). Object names starting f_ are facial sets: they mark
// the objects they're named after (0x20C4B80) and aren't objects themselves. The objects'
// models are requested as the special cutscene models (0xA136D0 on) and loaded (11 tries),
// the objects made, the camera's splines read, the player's clothes put on the CS_PLAY
// object, and the player's control taken.

#include <cstdint>
#include <cstring>

#include "game/FileLoader.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

GAME_ARRAY(char, csl_name, 0x020C5B20);          // the cutscene's name
GAME_ARRAY(char, csl_path, 0x00C221A8);          // a scratch path (0x100)
GAME_ARRAY(char, csl_modelName, 0x00C220A8);     // a model's name
GAME_ARRAY(char, csl_buffer, 0x00BD7800);        // 0x1C000
GAME_ARRAY(uint8_t, csl_wide, 0x020C4B60);       // per object: the _W version
GAME_ARRAY(uint8_t, csl_facial, 0x020C4B80);     // per object: has a facial set
GAME_ARRAY(int32_t, csl_modelIds, 0x020C4BA0);
GAME_ARRAY(char, csl_names, 0x020C4C18);         // 0x40 each
GAME_ARRAY(uint8_t*, csl_keptHxds, 0x020C4B38);  // 10
GAME_ARRAY(uint8_t*, csl_objects, 0x020C5B68);
GAME_VAR(uint8_t, csl_everythingRemoved, 0x020C5BE0);
GAME_VAR(uint8_t, csl_loaded, 0x020C5BE1);
GAME_VAR(uint8_t, csl_modelsLoaded, 0x020C5BE2);
GAME_VAR(uint8_t, csl_animsLoaded, 0x020C5BE3);
GAME_VAR(uint8_t, csl_audioPlaying, 0x020C5BE6);
GAME_VAR(uint8_t, csl_flag5BEA, 0x020C5BEA);
GAME_VAR(int32_t, csl_objectCount, 0x020C5BF0);
GAME_VAR(re::Ptr32<void>*, csl_hierarchies, 0x020C5BF4);
GAME_VAR(uint32_t, csl_hierarchyCount, 0x020C5BF8);
GAME_VAR(int32_t, csl_subtitleCount, 0x020C5C00);
GAME_VAR(uint8_t*, csl_subtitles, 0x020C5C04);
GAME_VAR(uint8_t, csl_keepZonePeds, 0x020C5C08);
GAME_VAR(uint8_t, csl_cameraUsed, 0x020C5C09);
GAME_VAR(float, csl_time, 0x020C5B1C);
GAME_VAR(int32_t, csl_subtitleIndex, 0x00BB64EC);
GAME_VAR(void*, csl_directory, 0x020C4B34);
GAME_VAR(uint8_t*, csl_playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t, csl_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, csl_areaProps, 0x00A147EC);
GAME_VAR(int32_t, csl_firstModel, 0x00A136D0);
GAME_VAR(void*, csl_camera, 0x00C3CFC4);
inline void* const kManager = reinterpret_cast<void*>(0x020C4910);
inline void* const kText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const kUserInput = reinterpret_cast<void*>(0x00A45A30);
inline void* const kPlayerInfo = reinterpret_cast<void*>(0x00C1AEA8);
inline const char* const kLoad = "CCutsceneMgr::LoadCutsceneData()";

GAME_FN(void, __cdecl, Csl_LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);
GAME_FN(void, __cdecl, Csl_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, Csl_PopMemoryTag, 0x005EF100);
GAME_FN(void, __cdecl, Csl_PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, Csl_PopTxdMemoryTag, 0x0052E710);
GAME_FN(void*, __cdecl, Csl_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Csl_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, Csl_Fn45B800, 0x0045B800);
GAME_FN(void, __cdecl, Csl_RemoveZonePeds, 0x0052D9A0);
GAME_FN(uint8_t, __cdecl, Csl_RemoveLoadedPed, 0x00530060);
GAME_FN(void, __cdecl, Csl_RemoveUnusedModels, 0x00530B50);
GAME_FN(void, __cdecl, Csl_DrasticTidyUpMemory, 0x004304E0, int);
template <typename... A>
inline int __cdecl Csl_Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
template <typename... A>
inline int __cdecl Csl_Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline char* __cdecl Csl_Strlwr(char* s) { return crt::Strlwr(s); }   // _strlwr (lower case, in place)
inline int __cdecl Csl_Strnicmp(const char* a, const char* b, uint32_t n) { return crt::Strnicmp(a, b, (size_t)n); }
inline int __cdecl Csl_Strncmp(const char* a, const char* b, uint32_t n) { return crt::Strncmp(a, b, (size_t)n); }
inline int __cdecl Csl_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline char* __cdecl Csl_Strrchr(const char* s, int c) { return crt::Strrchr(s, c); }
inline void* __cdecl Csl_Memcpy(void* dst, const void* src, uint32_t n) { return crt::Memcpy(dst, src, (size_t)n); }
GAME_FN(void, __thiscall, Csl_TextPreload, 0x0068FD50, void* text, const char* name);
GAME_FN(void*, __cdecl, Csl_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(uint8_t, __cdecl, Csl_Seek, 0x0042D300, void* file, int32_t offset, uint32_t origin);
GAME_FN(int, __cdecl, Csl_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(uint8_t, __cdecl, Csl_ReadLine, 0x0042D330, void* file, char* buffer, int size);
GAME_FN(int32_t, __cdecl, Csl_Tell, 0x0042D430, void* file);
GAME_FN(void, __cdecl, Csl_Close, 0x0042D450, void* file);
GAME_FN(uint32_t, __cdecl, Csl_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, void* file);
GAME_FN(FileStream*, __cdecl, Csl_OpenStream, 0x0042D4C0, int kind, int mode, void* arg);
GAME_FN(int, __cdecl, Csl_CloseStream, 0x0042D570, FileStream* stream, FileStreamBuffer* out);
GAME_FN(uint8_t, __cdecl, Csl_StreamReadLine, 0x004F7A40, FileStream* stream, char* buffer, int size);
GAME_FN(uint8_t, __thiscall, Csl_FindItem, 0x004290E0, void* directory, const char* name, uint32_t* posn,
        uint32_t* size);
GAME_FN(uint32_t, __cdecl, Csl_HashName, 0x00576ED0, const char* name);
GAME_FN(int, __thiscall, Csl_LoadHxdFile, 0x006C1C10, void* manager, uint32_t hash, const char* name,
        re::Ptr32<uint8_t>* out, void* stream);
GAME_FN(int, __thiscall, Csl_LoadHierarchy, 0x006C1B40, void* manager, const char* name, re::Ptr32<void>* out,
        void* stream, char* hxdName);
GAME_FN(int, __thiscall, Csl_FindLoadedHxd, 0x006C1E30, void* manager, const char* name, re::Ptr32<uint8_t>* out,
        uint32_t hash);
GAME_FN(int, __thiscall, Csl_SetHierarchyHxd, 0x006C2450, void* manager, void* hierarchy, uint8_t* hxd);
GAME_FN(int, __thiscall, Csl_ReleaseHierarchyObject, 0x006C1D00, void* manager, void* hierarchy);
GAME_FN(int, __thiscall, Csl_LoadAgr, 0x006BF500, void* manager, uint8_t* hxd, uint32_t group, void* stream);
GAME_FN(void, __cdecl, Csl_MakeSpace, 0x00531840, uint32_t bytes);
GAME_FN(void, __cdecl, Csl_RequestSpecialModel, 0x00533200, int32_t id, const char* name, int flags);
GAME_FN(void, __cdecl, Csl_LoadAllRequestedModels, 0x00533EE0, int priorityOnly);
GAME_FN(uint8_t*, __cdecl, Csl_GetStreamingInfo, 0x0052DD30, int32_t id);
GAME_FN(uint8_t*, __cdecl, Csl_CreateCutsceneObject, 0x006C4370, int32_t id);
GAME_FN(void, __thiscall, Csl_CameraLoadStream, 0x004F8980, void* camera, FileStream* stream);
GAME_FN(void, __thiscall, Csl_CameraLoadFile, 0x004F8820, void* camera, void* file);
GAME_FN(void, __cdecl, Csl_GenerateAllBoundingBoxes, 0x0045B960, int);
GAME_FN(void, __cdecl, Csl_OverlayBegin, 0x006CB550);
GAME_FN(void*, __cdecl, Csl_OverlayGet, 0x006CA7C0);
GAME_FN(void, __thiscall, Csl_OverlayBuildPlayer, 0x006CC7C0, void* overlay, void* object);
GAME_FN(void, __cdecl, Csl_OverlayEnd, 0x006CB620);
GAME_FN(void, __cdecl, Csl_LoadCutsceneSound, 0x006C38B0, const char* name);
GAME_FN(void, __thiscall, Csl_UserInputSetFlag, 0x0050DFB0, void* input, int flag);
GAME_FN(void, __thiscall, Csl_PlayerSetPaused, 0x00445DA0, void* info, int paused);
GAME_FN(void, __cdecl, Csl_LoadScene, 0x00534510, const void* position);

// the inline stream reads (a file's through the file manager, memory's copied)
inline void ReadWord(FileStream* s, void* to) {
    if (s->file) {
        Csl_ReadItems(to, 4, 1, s->file);
    } else if (s->data) {
        *static_cast<uint32_t*>(to) = *reinterpret_cast<uint32_t*>(s->data + s->position);
        s->position += 4;
    }
}
inline void ReadName(FileStream* s, char* to) {
    if (s->file) {
        Csl_ReadItems(to, 0x40, 1, s->file);
    } else if (s->data) {
        Csl_Memcpy(to, s->data + s->position, 0x40);
        s->position += 0x40;
    }
}
// forward by `offset` from where it is (nothing on a null stream)
inline void SeekForward(FileStream* s, int32_t offset) {
    if (s == nullptr) {
        return;
    }
    if (s->file) {
        Csl_Seek(s->file, offset, 1);
    } else if (s->data) {
        s->position += offset;
    }
}

// a packed file's block header: tag, name, size (the block's data starts after it)
struct Block {
    char tag[4];
    char name[0x40];
    uint32_t size;
    uint32_t start;
};
inline void ReadHeader(FileStream* s, Block& b) {
    ReadWord(s, b.tag);
    ReadName(s, b.name);
    ReadWord(s, &b.size);
}
// on to the next block's header
inline void NextBlock(FileStream* s, Block& b) {
    uint32_t position = s->position;
    int32_t skip = int32_t(b.size + (b.start - position));
    if (s->file) {
        Csl_Seek(s->file, skip, 1);
    } else if (s->data) {
        s->position = uint32_t(skip) + position;
    }
    ReadHeader(s, b);
    b.start = s->position;
}
inline bool TagIs(const Block& b, const char* tag) {
    return std::memcmp(b.tag, tag, 4) == 0;
}

// the hxd a hierarchy file names: CUTSCENE_ plus its base name plus .hxd
inline void HxdNameFor(char* out, const char* cutscene, const char* hxdName) {
    std::strcpy(out, cutscene);
    std::strcat(out, "_");
    char* back = Csl_Strrchr(hxdName, '\\');
    char* forward = Csl_Strrchr(hxdName, '/');
    if (forward > back) {
        back = forward;
    }
    const char* base = back != nullptr ? back + 1 : hxdName;
    std::strcat(out, base);
    std::strcat(out, ".hxd");
}

// the kept HXDs' groups named after the cutscene (NAME or NAME_...) loaded from their .AGR
void LoadGroups(const char* name, uint32_t nameLength) {
    for (int k = 0; k < 10; k++) {
        uint8_t* hxd = csl_keptHxds[k];
        if (hxd == nullptr) {
            break;
        }
        uint32_t groups = At<uint32_t>(hxd, 0x68);
        for (uint32_t g = 0; g < groups; g++) {
            auto group = [&]() -> char* {
                return g < At<uint32_t>(hxd, 0x68) ? reinterpret_cast<char*>(At<uint8_t*>(hxd, 0x6C) + g * 0x3C) : nullptr;
            };
            if (Csl_Strnicmp(name, group() + 4, nameLength) != 0) {
                continue;
            }
            char next = (group() + 4)[nameLength];
            if (next != 0 && (group() + 4)[nameLength] != '_') {
                continue;
            }
            FileStream* stream = Csl_OpenStream(2, 1, const_cast<char*>("CUTS\\CUTS.IMG"));
            Csl_Sprintf(csl_path, "%s.AGR", group() + 4);
            uint32_t posn, size;
            if (Csl_FindItem(csl_directory, csl_path, &posn, &size)) {
                Csl_MakeSpace(size << 10);
                SeekForward(stream, int32_t(posn << 11));
                Csl_PushTxdMemoryTag();
                Csl_LoadAgr(kManager, hxd, g, stream);
                Csl_PopTxdMemoryTag();
                csl_animsLoaded = 1;
            }
            Csl_CloseStream(stream, nullptr);
        }
    }
}

// an object's model name (its name, _W when wide)
inline void ModelName(int i) {
    std::strcpy(csl_modelName, csl_names + i * 0x40);
    if (csl_wide[i] != 0) {
        std::strcat(csl_modelName, "_W");
    }
}

// how many of the objects' models are loaded
inline int32_t LoadedModels() {
    int32_t loaded = 0;
    for (int32_t i = 0; i < csl_objectCount; i++) {
        if (Csl_GetStreamingInfo(csl_modelIds[i])[4] == 1) {
            loaded++;
        }
    }
    return loaded;
}

// the player's clothes put on the CS_PLAY object
inline void BuildPlayer(int32_t play) {
    Csl_PushTxdMemoryTag();
    if (play != -1 && csl_modelsLoaded == 1) {
        Csl_OverlayBegin();
        uint8_t* object = csl_objects[play];
        void* overlay = Csl_OverlayGet();
        Csl_OverlayBuildPlayer(overlay, object);
        Csl_OverlayEnd();
    }
    Csl_PopTxdMemoryTag();
}

// the names list read (f_ names moved to the end in reverse, the rest counted), then the
// facial sets matched to the objects they name; the count of the rest returned
template <typename ReadNext>
int32_t ReadNames(int32_t count, ReadNext readNext, bool packed) {
    int32_t objects = 0;
    char* end = csl_names + (count - 1) * 0x40;
    int at = 0;
    for (int32_t i = 0; i < count; i++) {
        char* name = csl_names + at * 0x40;
        readNext(name);
        Csl_Strlwr(name);
        if (packed) {
            uint32_t n = uint32_t(std::strlen(name));
            if (name[int32_t(n) - 2] == '_' && name[int32_t(n) - 1] == 'w') {
                name[n - 1] = 'W';
            }
        }
        if (Csl_Strnicmp(name, "f_", 2) == 0) {
            std::strcpy(end, name);
            end -= 0x40;
        } else {
            objects++;
            at++;
        }
        csl_facial[i] = 0;
    }
    return objects;
}

void MatchFacialSets(int32_t objects, int32_t count, bool packed) {
    if (objects >= count) {
        return;
    }
    for (int32_t f = objects; f < count; f++) {
        const char* facial = csl_names + f * 0x40 + 2;
        for (int32_t j = 0; j < objects; j++) {
            int same = packed ? Csl_Strncmp(facial, csl_names + j * 0x40, uint32_t(std::strlen(facial)))
                              : std::strcmp(facial, csl_names + j * 0x40);
            if (same == 0) {
                csl_facial[j] = 1;
            }
        }
    }
}

// the objects' models requested (the CS_PLAY one noted)
inline void RequestObjectModels(int32_t& index, int32_t& play) {
    for (int32_t i = 0; i < csl_objectCount; i++) {
        ModelName(i);
        Csl_RequestSpecialModel(csl_firstModel + index, csl_modelName, 0xE);
        csl_modelIds[i] = csl_firstModel + index;
        if (Csl_Strnicmp(csl_names + i * 0x40, "CS_PLAY", 7) == 0) {
            play = index;
        }
        index++;
    }
}

// the objects' models retried until all are loaded (11 tries)
template <typename OnPlay>
void LoadObjectModels(const char* firstLog, const char* retryLog, OnPlay onPlay, bool logLoad) {
    Csl_LoadingScreen(firstLog, "CStreaming::LoadAllRequestedModels()", 0);
    Csl_LoadAllRequestedModels(1);
    if (!logLoad) {
        Csl_LoadingScreen(kLoad, "Read Model indices", 0);
    }
    for (int tries = 0; tries <= 10; tries++) {
        if (logLoad) {
            Csl_LoadingScreen(kLoad, "Load Models", 0);
        }
        int32_t loaded = LoadedModels();
        if (loaded == csl_objectCount && loaded != 0) {
            csl_modelsLoaded = 1;
            return;
        }
        Csl_LoadingScreen(kLoad, logLoad ? "Load Object names" : "Request Models", 0);
        onPlay(-1, true);
        for (int32_t i = 0; i < csl_objectCount; i++) {
            ModelName(i);
            if (Csl_GetStreamingInfo(csl_modelIds[i])[4] != 1) {
                Csl_RequestSpecialModel(csl_modelIds[i], csl_modelName, 0xE);
            }
            if (logLoad && Csl_Strnicmp(csl_names + i * 0x40, "CS_PLAY", 7) == 0) {
                onPlay(i, false);
            }
        }
        Csl_LoadingScreen(retryLog, "CStreaming::LoadAllRequestedModels()", 0);
        Csl_LoadAllRequestedModels(0);
    }
}

// the objects made for the loaded models
inline void CreateObjects(uint32_t count) {
    Csl_LoadingScreen(kLoad, "Create Objects", 0);
    for (uint32_t i = 0; i < count; i++) {
        if (Csl_GetStreamingInfo(csl_modelIds[i])[4] == 1) {
            Csl_CreateCutsceneObject(csl_firstModel + int32_t(i));
        }
    }
}

// the packed NAME.CUT
void LoadPacked(void* file, const char* name, uint32_t posn, uint32_t size) {
    Csl_LoadingScreen(kLoad, "Load One File", 0);
    Csl_Seek(file, int32_t(posn << 11), 0);
    Csl_Read(file, csl_buffer, int(size << 11));
    Csl_Close(file);
    FileStreamBuffer memory{reinterpret_cast<uint8_t*>(csl_buffer), 0x1C000};
    FileStream* stream = Csl_OpenStream(3, 1, &memory);
    if (stream != nullptr) {
        ReadWord(stream, &csl_objectCount);
    }
    int32_t count = csl_objectCount;
    int32_t objects = 0;
    if (count > 0) {
        objects = ReadNames(
            count,
            [&](char* to) {
                if (stream != nullptr) {
                    ReadName(stream, to);
                }
            },
            true);
    }
    MatchFacialSets(objects, csl_objectCount, true);
    csl_objectCount = objects;

    Block block{};
    if (stream != nullptr) {
        ReadHeader(stream, block);
    }
    block.start = stream->position;
    csl_subtitleCount = 0;
    csl_subtitleIndex = -1;
    Csl_LoadingScreen(kLoad, "Read Subtitles", 0);
    if (TagIs(block, "SUB")) {
        if (csl_subtitles != nullptr) {
            Csl_Free(csl_subtitles);
            csl_subtitles = nullptr;
        }
        uint32_t at = stream->position;
        Csl_StreamReadLine(stream, csl_path, 0x100);
        while (Csl_StreamReadLine(stream, csl_path, 0x100)) {
            csl_subtitleCount += 1;
        }
        uint64_t bytes = uint64_t(uint32_t(csl_subtitleCount)) * 0x20;
        csl_subtitles = static_cast<uint8_t*>(Csl_Malloc((bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes)));
        if (csl_subtitles != nullptr) {
            stream->position = at;
            Csl_StreamReadLine(stream, csl_path, 0x100);
            for (int32_t i = 0; i < csl_subtitleCount; i++) {
                uint8_t* subtitle = csl_subtitles + i * 0x20;
                Csl_StreamReadLine(stream, csl_path, 0x100);
                Csl_Sscanf(csl_path, "{%d}{%d}%s", subtitle, subtitle + 4, subtitle + 8);
            }
            csl_subtitleIndex = 0;
        }
        NextBlock(stream, block);
    }
    Csl_LoadingScreen(kLoad, "Read HXDs", 0);
    if (TagIs(block, "HXD")) {
        re::Ptr32<uint8_t>* kept = csl_keptHxds;
        do {
            re::Ptr32<uint8_t> hxd = nullptr;   // (the game's is whatever the load leaves there)
            Csl_LoadHxdFile(kManager, Csl_HashName(block.name), block.name, &hxd, stream);
            *kept++ = hxd;
            NextBlock(stream, block);
        } while (TagIs(block, "HXD"));
    }
    uint64_t bytes = uint64_t(uint32_t(csl_objectCount)) * 4;
    csl_hierarchies = static_cast<re::Ptr32<void>*>(Csl_Malloc((bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes)));
    csl_hierarchyCount = 0;
    Csl_LoadingScreen(kLoad, "Read MXDs", 0);
    if (TagIs(block, "MXD")) {
        do {
            char hxdName[0x60];
            char local[0x100];
            Csl_Sprintf(csl_path, "CUTS\\MODELS\\%s", block.name);
            Csl_LoadHierarchy(kManager, csl_path, &csl_hierarchies[csl_hierarchyCount], stream, hxdName);
            HxdNameFor(local, name, hxdName);
            uint32_t hash = Csl_HashName(local);
            re::Ptr32<uint8_t> hxd = nullptr;
            Csl_FindLoadedHxd(kManager, nullptr, &hxd, hash);
            Csl_SetHierarchyHxd(kManager, csl_hierarchies[csl_hierarchyCount], hxd);
            csl_hierarchyCount += 1;
            NextBlock(stream, block);
        } while (TagIs(block, "MXD"));
    }
    uint32_t nameLength = uint32_t(std::strlen(name));
    Csl_LoadingScreen(kLoad, "Read Hierarchies", 0);
    LoadGroups(name, nameLength);

    csl_modelsLoaded = 0;
    Csl_LoadingScreen(kLoad, "Read Objects", 0);
    int32_t index = 0;
    int32_t play = -1;
    RequestObjectModels(index, play);
    LoadObjectModels("CCutsceneMgr::LoadCutsceneData() - 1", "CCutsceneMgr::LoadCutsceneData() - 2",
                     [](int32_t, bool) {}, false);
    csl_loaded = 1;
    CreateObjects(uint32_t(index));
    Csl_LoadingScreen(kLoad, "Read Splines", 0);
    if (TagIs(block, "DAT")) {
        Csl_PushTxdMemoryTag();
        Csl_CameraLoadStream(csl_camera, stream);
        Csl_PopTxdMemoryTag();
        csl_cameraUsed = 1;
    } else {
        csl_cameraUsed = 0;
    }
    Csl_CloseStream(stream, nullptr);
    csl_time = 0.0f;
    Csl_GenerateAllBoundingBoxes(0);
    Csl_LoadingScreen(kLoad, "Build player", 0);
    BuildPlayer(play);
}

// the loose files (NAME.INF: a count, then the object names a line each)
void LoadLoose(void* file, const char* name) {
    uint32_t posn, size;
    Csl_Sprintf(csl_path, "%s.INF", csl_name);
    if (Csl_FindItem(csl_directory, csl_path, &posn, &size)) {
        char line[0xC0];
        Csl_Seek(file, int32_t(posn << 11), 0);
        Csl_ReadLine(file, line, 0xC0);
        csl_objectCount = 0;
        Csl_Sscanf(line, "%d", &csl_objectCount);
        Csl_LoadingScreen(kLoad, "Read Object names", 0);
        int32_t objects = 0;
        int32_t count = csl_objectCount;
        if (count > 0) {
            objects = ReadNames(
                count,
                [&](char* to) {
                    Csl_ReadLine(file, line, 0xC0);
                    Csl_Sscanf(line, "%s", to);
                },
                false);
        }
        Csl_LoadingScreen(kLoad, "Read facial weight set", 0);
        MatchFacialSets(objects, csl_objectCount, false);
        csl_objectCount = objects;
    }
    Csl_Close(file);
    uint64_t bytes = uint64_t(uint32_t(csl_objectCount)) * 4;
    csl_hierarchies = static_cast<re::Ptr32<void>*>(Csl_Malloc((bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes)));
    uint32_t nameLength = uint32_t(std::strlen(name));
    csl_hierarchyCount = 0;
    Csl_LoadingScreen(kLoad, "Cutscene objects", 0);
    re::Ptr32<uint8_t> hxd = nullptr;   // (kept from the object before when a .hxd isn't there; unset at first)
    for (uint32_t i = 0; i < uint32_t(csl_objectCount); i++) {
        const char* object = csl_names + i * 0x40;
        FileStream* stream = Csl_OpenStream(2, 1, const_cast<char*>("CUTS\\CUTS.IMG"));
        csl_wide[i] = 0;
        bool wide = false;
        if (csl_areaProps[csl_chapter] == 2 && Csl_Stricmp(object, "CS_PLAY") != 0) {
            Csl_Sprintf(csl_path, "%s_W.mxd", object);
            bool peter = Csl_Stricmp(name, "3-0") == 0 && Csl_Stricmp(object, "CS_Peter") == 0;
            if (!peter && Csl_FindItem(csl_directory, csl_path, &posn, &size)) {
                csl_wide[i] = 1;
                wide = true;
            }
        }
        if (!wide) {
            Csl_Sprintf(csl_path, "%s.mxd", object);
        }
        if (Csl_FindItem(csl_directory, csl_path, &posn, &size)) {
            SeekForward(stream, int32_t(posn << 11));
            std::strcpy(csl_modelName, "Cuts\\Models\\");
            std::strcat(csl_modelName, object);
            if (csl_wide[i] != 0) {
                std::strcat(csl_modelName, "_W");
            }
            char hxdName[0x60];
            char local[0x100];
            Csl_LoadHierarchy(kManager, csl_modelName, &csl_hierarchies[csl_hierarchyCount], stream, hxdName);
            HxdNameFor(local, name, hxdName);
            uint32_t hash = Csl_HashName(local);
            FileStream* hxdStream = Csl_OpenStream(2, 1, const_cast<char*>("CUTS\\CUTS.IMG"));
            if (Csl_FindItem(csl_directory, local, &posn, &size)) {
                SeekForward(hxdStream, int32_t(posn << 11));
                Csl_LoadHxdFile(kManager, hash, local, &hxd, hxdStream);
            }
            Csl_CloseStream(hxdStream, nullptr);
            Csl_SetHierarchyHxd(kManager, csl_hierarchies[csl_hierarchyCount], hxd);
            uint8_t* own = At<uint8_t*>(csl_hierarchies[csl_hierarchyCount], 0x10);
            uint32_t groups = At<uint32_t>(own, 0x68);
            Csl_LoadingScreen(kLoad, "Cutscene animations", 0);
            bool used = false;
            for (uint32_t g = 0; g < groups; g++) {
                char* group = g < At<uint32_t>(own, 0x68)
                                  ? reinterpret_cast<char*>(At<uint8_t*>(own, 0x6C) + g * 0x3C)
                                  : nullptr;
                if (Csl_Strnicmp(name, group + 4, nameLength) == 0) {
                    used = true;
                    break;
                }
            }
            if (used) {
                csl_hierarchyCount += 1;
                Csl_LoadingScreen(kLoad, "Cutscene hierarchies", 0);
                uint32_t k = 0;
                for (;;) {
                    uint8_t* kept = csl_keptHxds[k];
                    if (kept == nullptr) {
                        csl_keptHxds[k] = own;
                        break;
                    }
                    if (kept == own) {
                        break;
                    }
                    k++;
                    if (k >= 10) {
                        csl_keptHxds[k] = own;   // (one past the ten: over the _W flags)
                        break;
                    }
                }
            } else {
                uint8_t* hierarchy = static_cast<uint8_t*>(csl_hierarchies[csl_hierarchyCount]);
                if (At<void*>(hierarchy, 8) != nullptr) {
                    At<uint32_t>(hierarchy, 4) &= 0xFFFFFFFBu;
                } else {
                    At<uint32_t>(hierarchy, 4) &= 0xFFFFFFF3u;
                }
                At<uint32_t>(own, 0x10) &= 0xFFFFFFFBu;
                Csl_ReleaseHierarchyObject(kManager, csl_hierarchies[csl_hierarchyCount]);
            }
        }
        Csl_CloseStream(stream, nullptr);
    }
    Csl_LoadingScreen(kLoad, "Read hierarchies for groups", 0);
    LoadGroups(name, nameLength);

    csl_modelsLoaded = 0;
    Csl_LoadingScreen(kLoad, "Read object names", 0);
    int32_t index = 0;
    int32_t play = -1;
    RequestObjectModels(index, play);
    csl_loaded = 1;
    int32_t count = index;
    LoadObjectModels("CCutsceneMgr::LoadCutsceneData() - 3", "CCutsceneMgr::LoadCutsceneData() - 4",
                     [&](int32_t i, bool reset) {
                         if (reset) {
                             play = -1;
                         } else {
                             (void)i;
                             play = count;   // (the game notes the object count, not the index)
                         }
                     },
                     true);
    CreateObjects(uint32_t(count));
    void* dat = Csl_OpenFile("CUTS\\CUTS.IMG", "rb", 1);
    Csl_Sprintf(csl_path, "%s.DAT", name);
    if (Csl_FindItem(csl_directory, csl_path, &posn, &size)) {
        Csl_PushTxdMemoryTag();
        Csl_Seek(dat, int32_t(posn << 11), 0);
        Csl_CameraLoadFile(csl_camera, dat);
        Csl_PopTxdMemoryTag();
        csl_cameraUsed = 1;
    } else {
        csl_cameraUsed = 0;
    }
    Csl_Close(dat);
    csl_time = 0.0f;
    Csl_GenerateAllBoundingBoxes(0);
    BuildPlayer(play);
    csl_subtitleCount = 0;
    csl_subtitleIndex = -1;
    if (csl_subtitles != nullptr) {
        Csl_Free(csl_subtitles);
        csl_subtitles = nullptr;
    }
    Csl_Sprintf(csl_path, "%s.SUB", name);
    if (!Csl_FindItem(csl_directory, csl_path, &posn, &size)) {
        csl_subtitleCount = 0;
        return;
    }
    void* sub = Csl_OpenFile("CUTS\\CUTS.IMG", "rb", 1);
    int32_t start = int32_t(posn << 11);
    int32_t end = start + int32_t(size << 11);
    Csl_Seek(sub, start, 0);
    Csl_ReadLine(sub, csl_buffer, 0x100);
    uint8_t more = Csl_ReadLine(sub, csl_buffer, 0x100);
    while (more != 0 && csl_buffer[0] != 0 && Csl_Tell(sub) < end) {
        csl_subtitleCount += 1;
        more = Csl_ReadLine(sub, csl_buffer, 0x100);
    }
    uint64_t subBytes = uint64_t(uint32_t(csl_subtitleCount)) * 0x20;
    csl_subtitles = static_cast<uint8_t*>(Csl_Malloc((subBytes >> 32) ? 0xFFFFFFFFu : uint32_t(subBytes)));
    if (csl_subtitles != nullptr) {
        Csl_Seek(sub, start, 0);
        Csl_ReadLine(sub, csl_buffer, 0x100);
        Csl_LoadingScreen(kLoad, "Load Subtitles", 0);
        for (int32_t i = 0; i < csl_subtitleCount; i++) {
            uint8_t* subtitle = csl_subtitles + i * 0x20;
            Csl_ReadLine(sub, csl_buffer, 0x100);
            Csl_Sscanf(csl_buffer, "{%d}{%d}%s", subtitle, subtitle + 4, subtitle + 8);
        }
        csl_subtitleIndex = 0;
    }
    Csl_Close(sub);
}

// 0x6C45E0 (cdecl; the game's caller passes more arguments, unused)
void __cdecl LoadCutsceneData(const char* name, int loadSound) {
    Csl_LoadingScreen(kLoad, "Start", 0);
    Csl_PushMemoryTag(0x33);
    csl_audioPlaying = 1;
    csl_flag5BEA = 0;
    csl_objectCount = 0;
    Csl_Fn45B800();
    Csl_LoadingScreen(kLoad, "Remove Zone Peds", 0);
    if (csl_keepZonePeds == 0) {
        Csl_RemoveZonePeds();
    }
    Csl_LoadingScreen(kLoad, "Remove Vehicles", 0);
    while (Csl_RemoveLoadedPed() != 0) {
    }
    Csl_LoadingScreen(kLoad, "CStreaming::RemoveUnusedModelsInLoadedList()", 0);
    Csl_RemoveUnusedModels();
    Csl_LoadingScreen(kLoad, "RwFreeListPurgeAllFreeLists()", 0);
    Csl_LoadingScreen(kLoad, "CGame::DrasticTidyUpMemory()", 0);
    Csl_DrasticTidyUpMemory(1);
    std::strcpy(csl_name, name);
    char subtitles[0x40];
    Csl_Sprintf(subtitles, "%s_SUB", csl_name);
    Csl_TextPreload(kText, subtitles);
    void* file = Csl_OpenFile("CUTS\\CUTS.IMG", "r", 1);
    Csl_LoadingScreen(kLoad, "FindMainPed()", 0);
    uint8_t* mainPed = csl_playerPed;
    Csl_Sprintf(csl_path, "%s.CUT", csl_name);
    uint32_t posn, size;
    bool found = Csl_FindItem(csl_directory, csl_path, &posn, &size) != 0;
    bool packed = found;
    if (csl_areaProps[csl_chapter] == 2) {
        Csl_Sprintf(csl_path, "%s_W.CUT", csl_name);
        if (Csl_FindItem(csl_directory, csl_path, &posn, &size)) {
            packed = true;
        }
    }
    if (packed) {
        LoadPacked(file, name, posn, size);
    } else {
        LoadLoose(file, name);
    }
    if (uint8_t(loadSound) != 0) {
        Csl_LoadCutsceneSound(name);
    }
    Csl_LoadingScreen(kLoad, "Setup variables", 0);
    At<uint32_t>(mainPed, 0x48) = 0;
    Csl_UserInputSetFlag(kUserInput, 0);
    Csl_PlayerSetPaused(kPlayerInfo, 1);
    Csl_PopMemoryTag();
    if (csl_everythingRemoved != 0) {
        uint8_t* matrix = At<uint8_t*>(mainPed, 0x14);
        Csl_LoadScene(matrix != nullptr ? matrix + 0x30 : mainPed + 4);
    }
    Csl_LoadingScreen(kLoad, "End", 0);
}

} // namespace

#define CUTSLOAD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CUTSLOAD_REVERSED("CCutsceneMgr::LoadCutsceneData", 0x006C45E0, &LoadCutsceneData);
