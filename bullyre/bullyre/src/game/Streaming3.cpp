// CStreaming, part 3 (0x52DFD0..0x534B33): the Gamebryo objects of the world's sectors
// let go of (far away, behind the camera, outside the area), the models near a point
// requested, a model's NIF made into an atomic, the CD directories, and a debug dump of
// the streaming state (its text is built and never printed).
//
// Found here: a sector's lists 0, 1 and 4 (+0, +4, +0x10 of its 20 bytes) hold what's
// streamed; the camera's direction for "behind" is its transform's third column (+8,
// +0x14). Bully's areas: an entity's +0x111 must be the current area, 1 (everywhere),
// or the extra area (0xBD09EB while 0xBD09EC).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Pool.h"
#include "game/Streaming.h"
#include "reversed.h"

#include "game/Crt.h"
namespace streaming17 {

using col::CVector;

GAME_VAR(void*, cameraNode, 0x00C3CCF4);
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);
GAME_VAR(uint32_t*, sectorNodes, 0x00C0F788);
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, extraAreaActive, 0x00BD09EC);
GAME_VAR(int8_t, extraArea, 0x00BD09EB);
GAME_VAR(uint8_t*, gameSettings, 0x009E4F4C);   // (+0x85D: the long streaming range)
GAME_VAR(int32_t, sectorOldX, 0x00CA367C);
GAME_VAR(int32_t, sectorOldY, 0x00CA3678);
GAME_VAR(uint8_t, stopRequestingEarly, 0x00C13F04);
GAME_VAR(uint8_t, dumpRequested, 0x020C5BE1);
GAME_VAR(uint16_t, ms_numInfos, 0x00CA2D58);
GAME_VAR(uint32_t, cdImageLow, 0x00CA3890);
GAME_ARRAY(int32_t, cdImageSlots, 0x00CA3894);   // 3
GAME_VAR(uint32_t, cdDirectoryBytes, 0x00CA388C);
GAME_VAR(uint32_t, cdDirectory3888, 0x00CA3888);
GAME_VAR(int32_t, geometryRttiBit, 0x020E2550);
GAME_ARRAY(uint8_t, sectorBytes, 0x00C1B178);    // 36 x 36 sectors of 20 bytes
// the pools (see Pools.h)
GAME_VAR(CPool<uint8_t>*, ms_pBuildingPool, 0x00C0F5F8);
GAME_VAR(CPool<uint8_t>*, ms_pBuildingPool2, 0x00C0F5FC);
GAME_VAR(CPool<uint8_t>*, ms_pObjectPool, 0x00C0F614);
GAME_VAR(CPool<uint8_t>*, ms_pDummyPool, 0x00C0F600);
// InterlockedIncrement's import slot
using InterlockedIncrementFn = long(__stdcall*)(volatile long*);
GAME_VAR(InterlockedIncrementFn, ImportInterlockedIncrement, 0x008FC174);

GAME_FN(void*, __thiscall, Directory_Destruct, 0x00429020, void* self);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int, __cdecl, CdStream_GetNumImages, 0x0073A9A0);
GAME_FN(const char*, __cdecl, CdStream_GetImageName, 0x0073A910, int image);
inline char* __cdecl StrRChr(char* s, int c) { return crt::Strrchr((const char*)s, c); }
inline char* __cdecl StrNCpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(void, __cdecl, LoadCdDirectory, 0x0052E1B0, const char* path, int image);
GAME_FN(void, __cdecl, DeleteRwObjectsInList, 0x0052E4F0, void* list);
GAME_FN(void, __cdecl, DeleteRwObjectsNotNearInList, 0x0052E550, void* list);
GAME_FN(void, __cdecl, DeleteRwObjectsNotNearInList2, 0x0052E5E0, void* list);
GAME_FN(uint8_t, __cdecl, HasMemoryFor, 0x0052E650, int bytes, int id);
GAME_FN(uint8_t, __cdecl, DeleteRwObjectsBehindCameraInList, 0x0052F8B0, void* list, int bytes, int id, int force);
GAME_FN(uint8_t, __cdecl, DeleteRwObjectsNotInFrustumInList, 0x0052F9A0, void* list, int bytes, int id, int force);
GAME_FN(uint8_t, __cdecl, RemoveLeastUsedModel, 0x00530C70, uint32_t keepFlags);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(uint8_t*, __cdecl, GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, uint32_t flags);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(int, __thiscall, Pool_GetSizeChained, 0x00435BB0, void* pool);
GAME_FN(uint8_t*, __thiscall, Pool_GetAtChained, 0x00435BD0, void* pool, int slot, int flag);
GAME_FN(float, __thiscall, SimpleModelInfo_GetLodDistance, 0x00527420, void* mi);
GAME_FN(float, __thiscall, PAnimModelInfo_GetLodDistance, 0x00520320, void* mi);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsPx, 0x00520430, void* mi);
GAME_FN(uint8_t, __cdecl, IsPlayerHidingFromSwitchables, 0x004469A0);
GAME_FN(uint8_t, __cdecl, IsTimeInRange, 0x00425E40, int timeOn, int timeOff, int, int);
GAME_FN(void, __cdecl, ProcessEntitiesInSectorList_Game, 0x00533B30, void* list, float x, float y, float minX, float minY, float maxX,
        float maxY, uint32_t flags);
GAME_FN(void, __cdecl, ProcessEntitiesInSectorListNear_Game, 0x00533DA0, void* list, uint32_t flags);
// NIF loading (the stream is a local object of 0x3FC bytes)
GAME_FN(void, __thiscall, NiStream_Construct, 0x0075CE40, void* stream);
GAME_FN(void, __thiscall, NiStream_Destruct, 0x0075C020, void* stream);
GAME_FN(void*, __cdecl, GetTexturePalette, 0x005F1520);
GAME_FN(void, __thiscall, NiStream_SetTexturePalette, 0x0075A340, void* stream, void* palette);
GAME_FN(void, __thiscall, NiStream_Load, 0x007599F0, void* stream, void* data, int size);
GAME_FN(uint8_t*, __cdecl, Atomic_Create, 0x006C6F50);
GAME_FN(void, __cdecl, Atomic_SetGeometry, 0x006C6C60, void* atomic, void* geometryRef, int);
GAME_FN(void, __cdecl, Atomic_SetBone, 0x006C6A10, void* atomic, void* bone);
GAME_FN(void, __thiscall, NiAVObject_Update, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(void, __cdecl, PrepareObject_4093F0, 0x004093F0, void* object);
GAME_FN(void, __cdecl, SetObjectFlags_40AD10, 0x0040AD10, void* object, uintptr_t, uint32_t a, uint32_t b);
// the debug dump's helpers
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(uint32_t, __cdecl, GetIdFromInfo, 0x0052DD50, CStreamingInfo* info);
GAME_FN(int, __cdecl, GetInfoIndex, 0x0052DC90, int id);
GAME_FN(uint8_t, __cdecl, IsTxdWanted, 0x0052FA90, int txd);
GAME_FN(const char*, __cdecl, Txd_GetName, 0x005F1060, int slot);
GAME_FN(int, __cdecl, Txd_GetNumRefs, 0x005F11E0, int slot);
GAME_FN(int, __cdecl, Txd_GetMemory, 0x005F1210, int slot);
GAME_FN(const char*, __cdecl, ColStore_GetName, 0x0056EEE0, int slot);
GAME_FN(void*, __cdecl, GetAgrGroup, 0x0052DB50, int id, int* index);
GAME_FN(uint8_t*, __thiscall, AgrGroup_GetEntry, 0x00450B80, void* group, int index);
GAME_FN(const char*, __cdecl, GetFileName, 0x0052EAE0, int id);

using re::At;

static uint8_t* Sector(int x, int y) {
    return sectorBytes + (y * 36 + x) * 20;
}

static int SectorIndex(float coord) {
    return int(coord / 50.0f + 18.0f);
}

static bool IsInCurrentArea(uint8_t area) {
    return area == currArea || area == 1 || (extraAreaActive && area == extraArea);
}

// ---- small ones ------------------------------------------------------------------------

// 0x52DFD0 (thiscall): CDirectory's deleting destructor
void* __fastcall Directory_Destroy(void* self, re::Edx, uint8_t flags) {
    Directory_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x52E700: the memory tag of streamed data pushed
void __cdecl PushStreamingMemoryTag() {
    PushMemoryTag(8);
}

// 0x52E710: CMemoryMgr::PopMemoryTag's thunk (a lone jmp to 0x5EF100)
RE_UNHOOKED("CMemoryMgr::PopMemoryTag (thunk)", 0x0052E710);

// 0x52ECA0: the directories of every image (their .img names with "DIR" for the
// extension), last to first. (Patched in place at 0x52ED10: not hooked)
void LoadAllCdDirectories() {
    cdImageLow = 0;
    cdImageSlots[0] = -1;
    cdImageSlots[1] = -1;
    cdImageSlots[2] = -1;
    int image = CdStream_GetNumImages();
    while (image != 0) {
        image--;
        char path[128];
        const char* name = CdStream_GetImageName(image);
        char* d = path;
        while ((*d++ = *name++) != '\0') {
        }
        char* dot = StrRChr(path, '.');
        StrNCpy(dot + 1, "DIR", 3);
        LoadCdDirectory(path, image);
    }
    cdDirectoryBytes >>= 11;
    cdDirectory3888 = 0;
}
RE_UNHOOKED_FN("CStreaming::LoadAllCdDirectories", 0x0052ECA0, &LoadAllCdDirectories);

// ---- the sectors' objects --------------------------------------------------------------

static void DeleteInSector(int x, int y) {
    uint8_t* sector = Sector(x, y);
    DeleteRwObjectsInList(sector);
    DeleteRwObjectsInList(sector + 4);
    DeleteRwObjectsInList(sector + 0x10);
}

// 0x52EDC0: CStreaming::DeleteFarAwayRwObjects: as the point moves to another sector,
// the rings 2..10 sectors behind it on that side let go of
void __cdecl DeleteFarAwayRwObjects(const CVector* pos) {
    int posX = SectorIndex(pos->x);
    int posY = SectorIndex(pos->y);
    int oldX = sectorOldX;
    int oldY = sectorOldY;
    while (posX != oldX) {
        if (posX < oldX) {
            for (int r = 2, lo = -2; lo >= -10; r++, lo--) {
                int x = oldX + r;
                if (x < 0) {
                    continue;
                }
                if (x >= 36) {
                    break;
                }
                for (int i = lo; i <= r; i++) {
                    int y = sectorOldY + i;
                    if (y < 0) {
                        continue;
                    }
                    if (y >= 36) {
                        break;
                    }
                    DeleteInSector(x, y);
                }
                oldX = sectorOldX;
            }
            oldX--;
        } else {
            for (int r = 2, lo = -2; lo >= -10; r++, lo--) {
                int x = oldX - r;
                if (x < 0) {
                    break;
                }
                if (x >= 36) {
                    continue;
                }
                for (int i = lo; i <= r; i++) {
                    int y = sectorOldY + i;
                    if (y < 0) {
                        continue;
                    }
                    if (y >= 36) {
                        break;
                    }
                    DeleteInSector(x, y);
                }
                oldX = sectorOldX;
            }
            oldX++;
        }
        sectorOldX = oldX;
    }
    oldY = sectorOldY;
    while (posY != oldY) {
        if (posY < oldY) {
            for (int r = 2, lo = -2; lo >= -10; r++, lo--) {
                int y = oldY + r;
                if (y < 0) {
                    continue;
                }
                if (y >= 36) {
                    break;
                }
                for (int i = lo; i <= r; i++) {
                    int x = sectorOldX + i;
                    if (x < 0) {
                        continue;
                    }
                    if (x >= 36) {
                        break;
                    }
                    DeleteInSector(x, y);
                }
                oldY = sectorOldY;
            }
            oldY--;
        } else {
            for (int r = 2, lo = -2; lo >= -10; r++, lo--) {
                int y = oldY - r;
                if (y < 0) {
                    break;
                }
                if (y >= 36) {
                    continue;
                }
                for (int i = lo; i <= r; i++) {
                    int x = sectorOldX + i;
                    if (x < 0) {
                        continue;
                    }
                    if (x >= 36) {
                        break;
                    }
                    DeleteInSector(x, y);
                }
                oldY = sectorOldY;
            }
            oldY++;
        }
        sectorOldY = oldY;
    }
}

// 0x52F140: the sectors more than 3 away from the point's (in both x and y)
void __cdecl DeleteRwObjectsOutsideRange(const CVector* pos) {
    int posX = SectorIndex(pos->x);
    int posY = SectorIndex(pos->y);
    int dx = posX;
    for (int x = 0; x < 36; x++, dx--) {
        int dy = posY;
        for (int y = 0; y < 36; y++, dy--) {
            float fdx = std::fabs(float(dx));
            if (3.0f < fdx) {
                float fdy = std::fabs(float(dy));
                if (fdy > 3.0f) {
                    uint8_t* sector = Sector(x, y);
                    DeleteRwObjectsInList(sector);
                    DeleteRwObjectsInList(sector + 4);
                    DeleteRwObjectsInList(sector + 0x10);
                }
            }
        }
    }
}

// the sector rectangle of the streaming range (80, or 100 with the long range) around pos
static void RangeSectors(const CVector* pos, int* startX, int* startY, int* endX, int* endY) {
    float range = gameSettings[0x85D] ? 100.0f : 80.0f;
    float minX = pos->x - range;
    float maxX = pos->x + range;
    float minY = pos->y - range;
    float maxY = range + pos->y;
    auto clampLo = [](float f) { return !(f <= 0.0f) ? f : 0.0f; };
    auto clampHi = [](float f) { return f < 35.0f ? f : 35.0f; };
    *startX = int(clampLo(float(SectorIndex(minX))));
    *startY = int(clampLo(float(SectorIndex(minY))));
    *endX = int(clampHi(float(SectorIndex(maxX))));
    *endY = int(clampHi(float(SectorIndex(maxY))));
}

// 0x52F210 / 0x52F3D0: every sector in the streaming range through 0x52E550 / 0x52E5E0
template <void (*Fn)(void*)>
static void ForRange(const CVector* pos) {
    int startX, startY, endX, endY;
    RangeSectors(pos, &startX, &startY, &endX, &endY);
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            uint8_t* sector = Sector(x, y);
            Fn(sector);
            Fn(sector + 4);
            Fn(sector + 0x10);
        }
    }
}
static void CallNotNear(void* list) {
    DeleteRwObjectsNotNearInList(list);
}
static void CallNotNear2(void* list) {
    DeleteRwObjectsNotNearInList2(list);
}
void __cdecl DeleteRwObjectsNotNear(const CVector* pos) {
    ForRange<&CallNotNear>(pos);
}
void __cdecl DeleteRwObjectsNotNear2(const CVector* pos) {
    ForRange<&CallNotNear2>(pos);
}

// ---- a model's NIF ---------------------------------------------------------------------

struct MemoryBuffer {
    re::Ptr32<void> data;
    int size;
};

// 0x52FCD0: CStreaming::LoadAtomicModel: the NIF in the buffer read, its first object
// (a geometry) put in a new atomic, prepared and given to the model info
uint8_t __cdecl LoadAtomicModel(MemoryBuffer* buffer, int id, void* info, uint32_t unknown6C) {
    (void)info;
    alignas(16) uint8_t stream[0x40C];   // (NiStream: 0x40C bytes)
    NiStream_Construct(stream);
    NiStream_SetTexturePalette(stream, GetTexturePalette());
    PushMemoryTag(9);
    NiStream_Load(stream, buffer->data, buffer->size);
    PopMemoryTag();
    uint8_t* atomic = Atomic_Create();
    At<int>(atomic, 0x24) = id;
    // (the first object, when it's a geometry, as a counted reference)
    re::Ptr32<void>* objects = At<re::Ptr32<void>*>(stream, 0x15C);
    void* object = *objects;
    void* geometry = nullptr;
    if (object) {
        uint8_t* rtti = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[2]))(object);
        uint32_t bit = uint32_t(geometryRttiBit);
        if (reinterpret_cast<uint32_t*>(rtti + 0xC)[bit >> 5] & (1u << (bit & 31))) {
            geometry = object;
        }
    }
    if (geometry) {
        ImportInterlockedIncrement(reinterpret_cast<volatile long*>(static_cast<uint8_t*>(geometry) + 4));
    }
    Atomic_SetGeometry(atomic, geometry, 0);
    At<int>(atomic, 0x20) = 0;
    Atomic_SetBone(atomic, nullptr);
    NiAVObject_Update(At<void*>(atomic, 0x14), 0.0f, 1);
    PrepareObject_4093F0(At<void*>(atomic, 0x14));
    NiAVObject_Update(At<void*>(atomic, 0x14), 0.0f, 1);
    uint8_t* mi = modelInfoPtrs[id];
    uint32_t flags = At<uint32_t>(mi, 0x28);
    SetObjectFlags_40AD10(At<void*>(atomic, 0x14), 0, (flags >> 0x12) & 0xFFFFFF01, (flags >> 6) & 0xFFFFFF01);
    NiAVObject_Update(At<void*>(atomic, 0x14), 0.0f, 1);
    reinterpret_cast<void(__thiscall*)(void*, void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(mi))[12]))(mi, atomic);
    At<uint32_t>(atomic, 0x6C) = unknown6C;
    if (modelInfoPtrs[id]) {
        At<int>(atomic, 0x7C) = At<int16_t>(modelInfoPtrs[id], 0x16);
    }
    NiStream_Destruct(stream);
    return 1;
}

// ---- the debug dump --------------------------------------------------------------------

static void AppendFlags(char* text, uint8_t flags) {
    if (flags & 1) std::strcat(text, " DONT_DELETE");
    if (flags & 2) std::strcat(text, " MISSION");
    if (flags & 4) std::strcat(text, " FORCE");
    if (flags & 8) std::strcat(text, " PRIORITY");
    if (flags & 0x10) std::strcat(text, " DONTFADE");
    if (flags & 0x20) std::strcat(text, " LOADSCENE");
    if (flags & 0xC0) std::strcat(text, " SEASONAL");
    if (flags & 0x40) std::strcat(text, " WINTER");
    if (flags & 0x80) std::strcat(text, " SPRING");
}

static const char* GameString(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}

// 0x530100: the requested and loaded entries described, one line each (the text is
// made and dropped: its output was compiled out). (With nothing cached yet the
// requested lines use whatever posn and size the previous entry had.)
void __cdecl DumpStreamingState() {
    char text[256];
    if (dumpRequested) {
        CStreamingInfo* info = &CStreaming::ms_pInfoBase[CStreaming::ms_pStartRequestedList->next];
        uint32_t posn = 0;   // (uninitialised in the game)
        uint32_t size = 0;
        while (info != CStreaming::ms_pEndRequestedList) {
            int id = GetIdFromInfo(info) & 0xFFFF;
            CStreamingInfo& entry = CStreaming::ms_aInfoForModel[GetInfoIndex(id)];
            if (entry.cdSize != 0) {
                posn = entry.cdPosn;
                size = entry.cdSize;
            }
            Sprintf(text, GameString(0x009189C0), id, posn, size);
            AppendFlags(text, entry.flags);
            info = &CStreaming::ms_pInfoBase[info->next];
        }
    }
    for (int i = 0; i < ms_numInfos; i++) {
        CStreamingInfo& info = CStreaming::ms_aInfoForModel[i];
        if (info.loadState == 0 || info.cdSize == 0) {
            continue;
        }
        uint32_t size = info.cdSize;
        int id = GetIdFromInfo(&info) & 0xFFFF;
        if (id >= AGR_ID_BASE && id < LIP_ID_BASE) {
            continue;   // (animations and placement skipped)
        }
        if ((id >= COL_ID_BASE && id < AGR_ID_BASE) || (id >= LIP_ID_BASE && id < CAT_ID_BASE) || id >= LUR_ID_BASE) {
            continue;
        }
        if (id < TXD_ID_BASE) {
            uint8_t* mi = modelInfoPtrs[id];
            int refs = At<int16_t>(mi, 0x14);
            Sprintf(text, GameString(0x00918928), id, refs, size * 2, info.flags, Txd_GetName(At<int16_t>(mi, 0x16)));
        } else if (id < COL_ID_BASE) {
            int slot = id - TXD_ID_BASE;
            int memory = Txd_GetMemory(slot) / 1024;
            int refs = Txd_GetNumRefs(slot);
            Sprintf(text, GameString(0x009188E8), id, Txd_GetName(slot), refs, size * 2, memory, info.flags);
        } else if (id < AGR_ID_BASE) {
            Sprintf(text, GameString(0x009188BC), id, ColStore_GetName(id - COL_ID_BASE), size * 2, info.flags);
        } else if (id < IPB_ID_BASE) {
            int index;
            void* group = GetAgrGroup(id, &index);
            int refs = *reinterpret_cast<uint16_t*>(AgrGroup_GetEntry(group, index) + 0x2C);
            Sprintf(text, GameString(0x00918888), id, AgrGroup_GetEntry(group, index) + 4, refs, size * 2, info.flags);
        } else {
            const char* name = GetFileName(id);
            if (id < LIP_ID_BASE) {
                Sprintf(text, GameString(0x00918860), id, name, size * 2, info.flags);
            } else if (id < CAT_ID_BASE) {
                Sprintf(text, GameString(0x00918838), id, name, size * 2, info.flags);
            } else {
                Sprintf(text, GameString(0x00918810), id, GameString(0x00918720), name, size * 2, info.flags);
            }
        }
        AppendFlags(text, info.flags);
        if (uint32_t(id - TXD_ID_BASE) <= 9999 && IsTxdWanted(id - TXD_ID_BASE)) {
            std::strcat(text, " FOR_REQUESTED");
        }
        std::strcat(text, info.next == 0xFFFF ? " (not in loaded linked-list)" : " (in loaded linked-list)");
        std::strcat(text, "\n");
    }
    // (the requested list walked again, to no effect)
    CStreamingInfo* info = &CStreaming::ms_pInfoBase[CStreaming::ms_pStartRequestedList->next];
    while (info != CStreaming::ms_pEndRequestedList) {
        info = &CStreaming::ms_pInfoBase[info->next];
    }
}

// ---- behind the camera -----------------------------------------------------------------

static float MaxZero(float f) {
    return !(f <= 0.0f) ? f : 0.0f;
}
static float Min35(float f) {
    return f < 35.0f ? f : 35.0f;
}

// three lists of a sector through `fn`; true when it says enough is free
template <typename Fn>
static bool Sweep(int x, int y, Fn fn) {
    uint8_t* sector = Sector(x, y);
    return fn(sector) || fn(sector + 0x10) || fn(sector + 4);
}

// 0x530DB0: CStreaming::DeleteRwObjectsBehindCamera: until there's room for `bytes` (of
// `id`): the sectors behind the camera (from 10 back to 2, or 6 when forced), then (not
// forced) those ahead within 10 that are out of view, then those again; then the
// least used models
void __cdecl DeleteRwObjectsBehindCamera(int bytes, int id, uint8_t force) {
    if (!force && HasMemoryFor(bytes, id)) {
        return;
    }
    int ix = SectorIndex(*reinterpret_cast<float*>(GetLocalTransform(GetCameraNode(cameraNode)) + 0x24));
    int iy = SectorIndex(*reinterpret_cast<float*>(GetLocalTransform(GetCameraNode(cameraNode)) + 0x28));
    float fx = *reinterpret_cast<float*>(GetLocalTransform(GetCameraNode(cameraNode)) + 8);
    float fy = *reinterpret_cast<float*>(GetLocalTransform(GetCameraNode(cameraNode)) + 0x14);
    int off = force ? 6 : 2;
    auto behind = [&](void* list) { return DeleteRwObjectsBehindCameraInList(list, bytes, id, force) != 0; };
    auto behindUnforced = [&](void* list) { return DeleteRwObjectsBehindCameraInList(list, bytes, id, 0) != 0; };
    auto notInView = [&](void* list) { return DeleteRwObjectsNotInFrustumInList(list, bytes, id, 0) != 0; };
    if (std::fabs(fy) < std::fabs(fx)) {
        int ymin = int(MaxZero(float(iy - 10)));
        int ymax = int(Min35(float(iy + 10)));
        int xStart, xEnd, inc;
        if (fx > 0.0f) {
            xStart = int(MaxZero(float(ix - 10)));
            xEnd = int(MaxZero(float(ix - off)));
            inc = 1;
        } else {
            xEnd = int(Min35(float(off + ix)));
            xStart = int(Min35(float(ix + 10)));
            inc = -1;
        }
        for (int x = xStart; x != xEnd; x += inc) {
            for (int y = ymin; y <= ymax; y++) {
                if (Sweep(x, y, behind)) {
                    return;
                }
            }
        }
        if (force) {
            return;
        }
        int xA, xB;
        if (fx > 0.0f) {
            xA = int(MaxZero(float(ix - off)));
            xB = int(Min35(float(ix + 10)));
            inc = 1;
        } else {
            xA = int(Min35(float(off + ix)));
            xB = int(MaxZero(float(ix - 10)));
            inc = -1;
        }
        for (int x = xB; x != xA; x -= inc) {
            for (int y = ymin; y <= ymax; y++) {
                if (Sweep(x, y, notInView)) {
                    return;
                }
            }
        }
        for (int x = xB; x != xA; x -= inc) {
            for (int y = ymin; y <= ymax; y++) {
                if (Sweep(x, y, behindUnforced)) {
                    return;
                }
            }
        }
    } else {
        int xmin = int(MaxZero(float(ix - 10)));
        int xmax = int(Min35(float(ix + 10)));
        int yStart, yEnd, inc;
        if (fy > 0.0f) {
            yStart = int(MaxZero(float(iy - 10)));
            yEnd = int(MaxZero(float(iy - off)));
            inc = 1;
        } else {
            yEnd = int(Min35(float(off + iy)));
            yStart = int(Min35(float(iy + 10)));
            inc = -1;
        }
        for (int y = yStart; y != yEnd; y += inc) {
            for (int x = xmin; x <= xmax; x++) {
                if (Sweep(x, y, behind)) {
                    return;
                }
            }
        }
        if (force) {
            return;
        }
        int yA, yB;
        if (fy > 0.0f) {
            yA = int(MaxZero(float(iy - off)));
            yB = int(Min35(float(iy + 10)));
            inc = 1;
        } else {
            yA = int(Min35(float(off + iy)));
            yB = int(MaxZero(float(iy - 10)));
            inc = -1;
        }
        for (int y = yB; y != yA; y -= inc) {
            for (int x = xmin; x <= xmax; x++) {
                if (Sweep(x, y, notInView)) {
                    return;
                }
            }
        }
        for (int y = yB; y != yA; y -= inc) {
            for (int x = xmin; x <= xmax; x++) {
                if (Sweep(x, y, behindUnforced)) {
                    return;
                }
            }
        }
    }
    if (HasMemoryFor(bytes, id)) {
        return;
    }
    while (RemoveLeastUsedModel(0)) {
        if (HasMemoryFor(bytes, id)) {
            return;
        }
    }
}

// ---- areas and requests ----------------------------------------------------------------

// the entities of a pool with a Gamebryo object that aren't in `area` (or everywhere)
static bool OutsideArea(uint8_t* e, int area) {
    if (!e || !At<void*>(e, 0x18)) {
        return false;
    }
    uint8_t a = e[0x111];
    return a != area && a != 1;
}

static void RemoveBuildingsInPool(CPool<uint8_t>* pool, int area) {
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* e = pool->objects + pool->elemSize * i;
        if (!OutsideArea(e, area)) {
            continue;
        }
        if (At<uint32_t>(e, 0x58)) {   // (a big building: kept loaded while +0x5C)
            if (!At<uint32_t>(e, 0x5C)) {
                continue;
            }
            if (At<uint32_t>(e, 0x58)) {
                RequestModel(At<int16_t>(e, 0x10E), 0);
            }
        }
        if (!At<uint32_t>(e, 0x80)) {
            Entity_DeleteRwObject(e);
        }
    }
}

// 0x532330: CStreaming::RemoveBuildingsNotInArea: the Gamebryo objects of the
// buildings, objects (their +0x208 1) and dummies outside the area let go of
void __cdecl RemoveBuildingsNotInArea(int area) {
    RemoveBuildingsInPool(ms_pBuildingPool, area);
    RemoveBuildingsInPool(ms_pBuildingPool2, area);
    CPool<uint8_t>* objects = ms_pObjectPool;
    int count = objects->size;
    if (objects->Next()) {
        count += Pool_GetSizeChained(objects->Next());
    }
    while (count != 0) {
        count--;
        uint8_t* e = nullptr;
        if (count < objects->size) {
            e = objects->objects + objects->elemSize * count;
        } else if (objects->Next()) {
            e = Pool_GetAtChained(objects->Next(), count - objects->size, 1);
        }
        // (the flag read is this pool's, even past its end)
        if ((objects->flags[count] & 0x80) == 0 && OutsideArea(e, area) && !At<uint32_t>(e, 0x80) && e[0x208] == 1) {
            Entity_DeleteRwObject(e);
        }
    }
    CPool<uint8_t>* dummies = ms_pDummyPool;
    for (int i = dummies->size; i != 0;) {
        i--;
        if (dummies->flags[i] & 0x80) {
            continue;
        }
        uint8_t* e = dummies->objects + dummies->elemSize * i;
        if (OutsideArea(e, area) && !At<uint32_t>(e, 0x80)) {
            Entity_DeleteRwObject(e);
        }
    }
}

// the checks both sector scans make before requesting (false: skip the entity)
static bool Requestable(uint8_t* mi) {
    if ((mi[0xB] & uint8_t(1u << (chapter & 31))) == 0) {
        return false;
    }
    if (mi[8] == 3 && !IsTimeInRange(At<int>(mi, 0x34), At<int>(mi, 0x38), 0, 0)) {
        return false;
    }
    if (mi[8] == 10 && PAnimModelInfo_IsPx(mi)) {
        return false;
    }
    return true;
}

// 0x533B30: CStreaming::ProcessEntitiesInSectorList: the models of a list's entities in
// the rectangle and within their LOD distance of (x, y) requested
void __cdecl ProcessEntitiesInSectorList(re::Ptr32<uint32_t>* list, float x, float y, float minX, float minY, float maxX, float maxY,
                                         uint32_t flags) {
    uint16_t scanCode = currentScanCode;
    uint32_t* node = *list;
    while (node) {
        uint32_t value = *node;
        uint8_t* e = GetEntityFromNode(value & 0xF, (value >> 4) & 0x3FFF);
        node = (value >> 18) == 0x3FFF ? nullptr : sectorNodes + (value >> 18);
        if (At<uint16_t>(e, 0x10A) == scanCode) {
            continue;
        }
        At<uint16_t>(e, 0x10A) = scanCode;
        if (At<uint32_t>(e, 0x74) || At<uint32_t>(e, 0xB4) || !At<uint32_t>(e, 0x48) || !IsInCurrentArea(e[0x111])) {
            continue;
        }
        uint8_t* mi = modelInfoPtrs[At<int16_t>(e, 0x10E)];
        float lod;
        uint8_t type = mi[8];
        if (type == 1 || type == 3) {
            lod = SimpleModelInfo_GetLodDistance(mi);
            if ((At<uint32_t>(mi, 0x28) & 0x2000000) && IsPlayerHidingFromSwitchables()) {
                continue;
            }
        } else if (type == 10) {
            lod = PAnimModelInfo_GetLodDistance(mi);
        } else {
            continue;
        }
        if (!Requestable(mi)) {
            continue;
        }
        float lodSq = lod * lod;
        if (gameSettings[0x85D]) {
            if (!(10000.0f <= lodSq)) {
                lodSq = 10000.0f;
            }
        } else if (lodSq < 6400.0f) {
            lodSq = 6400.0f;
        }
        const float* pos = At<uint8_t*>(e, 0x14) ? reinterpret_cast<float*>(At<uint8_t*>(e, 0x14) + 0x30) : reinterpret_cast<float*>(e + 4);
        float ex = pos[0];
        float ey = pos[1];
        if (!(minX < ex) || !(ex < maxX) || !(minY < ey) || !(ey < maxY)) {
            continue;
        }
        float dx = x - ex;
        float dy = y - ey;
        float distSq = dx * dx + dy * dy;
        if (distSq < lodSq) {
            RequestModel(At<int16_t>(e, 0x10E), flags);
        }
    }
}

// 0x533DA0: the same for the sectors right at the point (no distance check)
void __cdecl ProcessEntitiesInSectorListNear(re::Ptr32<uint32_t>* list, uint32_t flags) {
    uint32_t* node = *list;
    while (node) {
        uint32_t value = *node;
        uint8_t* e = GetEntityFromNode(value & 0xF, (value >> 4) & 0x3FFF);
        node = (value >> 18) == 0x3FFF ? nullptr : sectorNodes + (value >> 18);
        if (At<uint16_t>(e, 0x10A) == currentScanCode) {
            continue;
        }
        At<uint16_t>(e, 0x10A) = currentScanCode;
        if (At<uint32_t>(e, 0x74) || !IsInCurrentArea(e[0x111]) || At<uint32_t>(e, 0xB4) || !At<uint32_t>(e, 0x48)) {
            continue;
        }
        uint8_t* mi = modelInfoPtrs[At<int16_t>(e, 0x10E)];
        if ((mi[0xB] & uint8_t(1u << (chapter & 31))) == 0) {
            continue;
        }
        if (mi[8] == 1 && (At<uint32_t>(mi, 0x28) & 0x2000000) && IsPlayerHidingFromSwitchables()) {
            continue;
        }
        if (mi[8] == 3 && !IsTimeInRange(At<int>(mi, 0x34), At<int>(mi, 0x38), 0, 0)) {
            continue;
        }
        if (mi[8] == 10 && PAnimModelInfo_IsPx(mi)) {
            continue;
        }
        RequestModel(At<int16_t>(e, 0x10E), flags);
    }
}

// 0x534190: CStreaming::AddModelsToRequestList: the sectors around pos: the nearest
// without a distance check, a ring further out with one
void __cdecl AddModelsToRequestList(const CVector* pos, uint32_t flags) {
    int ring = 1;
    float range;
    if (gameSettings[0x85D]) {
        range = 100.0f;
        ring = 2;
    } else {
        range = 80.0f;
    }
    float minX = pos->x - range;
    float maxX = pos->x + range;
    float minY = pos->y - range;
    float maxY = range + pos->y;
    int cx = SectorIndex(pos->x);
    int cy = SectorIndex(pos->y);
    int startX = int(MaxZero(float(SectorIndex(minX))));
    int startY = int(MaxZero(float(SectorIndex(minY))));
    int endX = int(Min35(float(SectorIndex(maxX))));
    int endY = int(Min35(float(SectorIndex(maxY))));
    if (currentScanCode >= 0xFFFF) {
        ClearScanCodes();
        currentScanCode = 1;
    } else {
        currentScanCode++;
    }
    int dy = startY - cy;
    for (int y = startY; y <= endY; y++, dy++) {
        int dx = startX - cx;
        for (int x = startX; x <= endX; x++, dx++) {
            if (stopRequestingEarly && CStreaming::ms_numModelsRequested > 5) {
                return;
            }
            uint8_t* sector = Sector(x, y);
            int distSq = dx * dx + dy * dy;
            if (distSq <= (ring - 1) * (ring - 1)) {
                ProcessEntitiesInSectorListNear_Game(sector, flags);
                ProcessEntitiesInSectorListNear_Game(sector + 4, flags);
                ProcessEntitiesInSectorListNear_Game(sector + 0x10, flags);
            } else if (distSq <= (ring + 2) * (ring + 2)) {
                ProcessEntitiesInSectorList_Game(sector, pos->x, pos->y, minX, minY, maxX, maxY, flags);
                ProcessEntitiesInSectorList_Game(sector + 4, pos->x, pos->y, minX, minY, maxX, maxY, flags);
                ProcessEntitiesInSectorList_Game(sector + 0x10, pos->x, pos->y, minX, minY, maxX, maxY, flags);
            }
        }
    }
}

// 0x534B30 (thiscall): &this->+0x7C (4 bytes: not hookable)
void* __fastcall Get7C(uint8_t* self, re::Edx) {
    return self + 0x7C;
}
RE_UNHOOKED_FN("Get7C_534B30", 0x00534B30, &Get7C);

} // namespace streaming17

// ---- registration ----------------------------------------------------------------------

#define STREAMING17_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace streaming17 {
STREAMING17_REVERSED("CDirectory::DeletingDestructor_52DFD0", 0x0052DFD0, &Directory_Destroy);
STREAMING17_REVERSED("CStreaming::PushStreamingMemoryTag", 0x0052E700, &PushStreamingMemoryTag);
STREAMING17_REVERSED("CStreaming::DeleteFarAwayRwObjects", 0x0052EDC0, &DeleteFarAwayRwObjects);
STREAMING17_REVERSED("CStreaming::DeleteRwObjectsOutsideRange", 0x0052F140, &DeleteRwObjectsOutsideRange);
STREAMING17_REVERSED("CStreaming::DeleteRwObjectsNotNear", 0x0052F210, &DeleteRwObjectsNotNear);
STREAMING17_REVERSED("CStreaming::DeleteRwObjectsNotNear2", 0x0052F3D0, &DeleteRwObjectsNotNear2);
STREAMING17_REVERSED("CStreaming::LoadAtomicModel", 0x0052FCD0, &LoadAtomicModel);
STREAMING17_REVERSED("CStreaming::DumpStreamingState", 0x00530100, &DumpStreamingState);
STREAMING17_REVERSED("CStreaming::DeleteRwObjectsBehindCamera", 0x00530DB0, &DeleteRwObjectsBehindCamera);
STREAMING17_REVERSED("CStreaming::RemoveBuildingsNotInArea", 0x00532330, &RemoveBuildingsNotInArea);
STREAMING17_REVERSED("CStreaming::ProcessEntitiesInSectorList", 0x00533B30, &ProcessEntitiesInSectorList);
STREAMING17_REVERSED("CStreaming::ProcessEntitiesInSectorListNear", 0x00533DA0, &ProcessEntitiesInSectorListNear);
STREAMING17_REVERSED("CStreaming::AddModelsToRequestList", 0x00534190, &AddModelsToRequestList);
} // namespace streaming17
