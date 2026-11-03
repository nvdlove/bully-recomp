#pragma once

// CStreaming: the world's models, texture dictionaries, collision and the rest,
// streamed in from Stream\World.img. (Bully keeps GTA's design; the names follow
// GTA San Andreas where the code matches.)

#include <cstddef>
#include <cstdint>

#include "reversed.h"

struct CBaseModelInfo;

enum eStreamingLoadState : uint8_t {
    LOADSTATE_NOT_LOADED = 0,
    LOADSTATE_LOADED = 1,
    LOADSTATE_REQUESTED = 2,
    LOADSTATE_READING = 3,
    LOADSTATE_FINISHING = 4,
};

enum eStreamingFlags : uint8_t {
    STREAMING_GAME_REQUIRED = 0x01,
    STREAMING_MISSION_REQUIRED = 0x02,
    STREAMING_KEEP_IN_MEMORY = STREAMING_GAME_REQUIRED | STREAMING_MISSION_REQUIRED,
    STREAMING_PRIORITY = 0x08,
    STREAMING_0x10 = 0x10,           // (set on models: RequestModel marks model info +0x2C)
    STREAMING_KEPT_BITS = 0xC0,      // survive a new request
};

// The kinds of ids, by range (the archive's file extensions, from LoadCdDirectory).
enum : int {
    TXD_ID_BASE = 0x2FA8,    // .nft texture dictionaries (CTxdStore slots)
    COL_ID_BASE = 0x56B8,    // .col collision (500)
    AGR_ID_BASE = 0x58AC,    // .agr animation groups (0x244): kept in memory once read, see ms_pPreloaded
    IPB_ID_BASE = 0x5AF0,    // .ipb/.ipc object placement
    LIP_ID_BASE = 0x5B54,    // .lip lip sync
    CAT_ID_BASE = 0x5D41,    // .cat prop animations
    LUR_ID_BASE = 0x5DB9,    // .lur
};

// One archive entry the game can stream: CStreaming::ms_aInfoForModel[] (0xCA39B8).
// The loaded and requested lists link them by index from ms_pInfoBase.
struct CStreamingInfo {
    uint16_t next;       // +0  in the loaded or requested list (-1: none)
    uint16_t prev;       // +2
    uint8_t loadState;   // +4  eStreamingLoadState
    uint8_t flags;       // +5  eStreamingFlags
    int16_t nextOnCd;    // +6  the next entry in the archive (read with it in one go), -1: none
    uint32_t cdPosn;     // +8  2048-byte sectors into World.img
    uint32_t cdSize;     // +0xC sectors

    // 0x52D7B0: into a list, after its head
    void AddToList(CStreamingInfo* head);
    // (inlined in the game)
    void RemoveFromList();
    bool InList() const { return next != 0xFFFF; }
};
static_assert(sizeof(CStreamingInfo) == 16);

// Ids are spread over a few kinds (see TXD_ID_BASE ...). A sorted table of ranges
// maps an id to its CStreamingInfo.
struct CStreamingIdRange {
    uint16_t firstId;
    uint16_t count;
    int16_t indexDelta;  // CStreamingInfo index = id + indexDelta
};
static_assert(sizeof(CStreamingIdRange) == 6);

// A read in flight: up to four entries in one go (see RequestModelStream).
struct tStreamingChannel {
    int32_t modelIds[4];      // -1: none
    int32_t modelOffsets[4];  // sectors into the buffer
    int32_t state;
    int32_t stateTimer;
    uint32_t cdPosn;
    uint32_t cdSize;
    int32_t tries;
    int32_t unknown34;
};
static_assert(sizeof(tStreamingChannel) == 0x38);

namespace CStreaming {

GAME_ARRAY(CStreamingInfo, ms_aInfoForModel, 0x00CA39B8);  // 17200, then the four list heads
GAME_VAR(CStreamingInfo*, ms_pInfoBase, 0x00CA397C);       // list links are indices from here
GAME_VAR(CStreamingInfo*, ms_pStartLoadedList, 0x00CA3884);
GAME_VAR(CStreamingInfo*, ms_pEndLoadedList, 0x00CA3880);
GAME_VAR(CStreamingInfo*, ms_pStartRequestedList, 0x00CA387C);
GAME_VAR(CStreamingInfo*, ms_pEndRequestedList, 0x00CA3878);
GAME_ARRAY(CStreamingIdRange, ms_aIdRanges, 0x00CA2B00);   // 100
GAME_VAR(uint16_t, ms_numIdRanges, 0x00CA2AF8);
GAME_VAR(uint16_t, ms_lastInfoId, 0x00CA2AF0);            // the last lookup
GAME_VAR(uint16_t, ms_lastInfoIndex, 0x00CA2AF2);
GAME_VAR(int32_t, ms_numModelsRequested, 0x00CA3988);
GAME_VAR(int32_t, ms_numPriorityRequests, 0x00CA3680);
// animation groups' data, kept in memory once read (indexed by id: AGR_ID_BASE..)
inline re::Ptr32<void>* const ms_pPreloaded = reinterpret_cast<re::Ptr32<void>*>(0x00C8CAB0);

// 0x52DC90: the CStreamingInfo index for an id, or -1.
int __cdecl GetInfoIndex(int id);
// The same, without the one-entry cache (for comparing).
int FindInfoIndex(int id);
// 0x52DD50: the id of a CStreamingInfo (in the low 16 bits).
uint32_t __cdecl GetIdFromInfo(CStreamingInfo* info);
// 0x532020
void __cdecl RequestModel(int id, uint32_t flags);
// 0x52D790
void __cdecl RequestTxd(int txd, uint32_t flags);
// 0x531CB0: makes the game object from an entry's data (a model, a dictionary, ...);
// false when it couldn't (then it's asked for again or dropped)
bool __cdecl ConvertBufferToObject(void* data, int id, int priority);
// 0x52F640: frees an entry's data and takes it out of the lists and the channels
void __cdecl RemoveModel(int id);
// 0x532A10: the requested entry to read next: the nearest at or after `posn` in the
// archive (else the first), -1 if there's none; with `priority`, only priority
// requests while there are any. Models wait for their dictionary and the files
// they need, which it asks for.
int __cdecl GetNextFileOnCd(uint32_t posn, int priority);
// 0x532C80: starts a read on a channel: the next file and the ones after it in the
// archive (up to four, as long as they're requested and fit the buffer)
void __cdecl RequestModelStream(int channel);

// 0x532750: a channel's read: on error or taking too long, the channel is flagged;
// once done, each entry is made into its object (finish: all of them now)
int __cdecl ProcessLoadingChannel(int channel, int finish);
// 0x5330A0: once a frame: the two channels take turns reading
void __cdecl LoadRequestedModels();
// 0x532510: the second part of a big entry's loading (LOADSTATE_FINISHING)
bool __cdecl FinishLoadingLargeFile(void* data, int id, int finish);
// 0x533A50: a channel that failed: read again (and flag the loading as stalled
// after 3000 tries)
void __cdecl RetryLoadFile(int channel);

// 0x52FA90: a model that's requested or being read uses this dictionary
uint8_t __cdecl IsTxdWanted(int txd);

GAME_VAR(uint8_t, ms_bLoadingStalled, 0x00C1A999);
// 0x533EE0: everything requested, now (loading screens): read one by one on channel 0
void __cdecl LoadAllRequestedModels(int priorityOnly);
GAME_VAR(uint8_t, ms_bLoadingAll, 0x00CE6D1C);   // (it doesn't nest)

GAME_VAR(int32_t, ms_streamingBufferSize, 0x00CA3978);   // sectors (per channel)
GAME_ARRAY(void*, ms_pStreamingBuffer, 0x00CA3670);       // one per channel
GAME_VAR(int32_t, ms_currentChannel, 0x00CE6D18);
GAME_VAR(uint8_t, ms_bSingleChannel, 0x00CA2AF6);         // only channel 0
GAME_VAR(int32_t, ms_channelError, 0x00CA3920);           // the channel that failed, -1: none
GAME_VAR(int32_t, ms_numPedsLoaded, 0x00CA3924);
GAME_VAR(int32_t, ms_desiredNumPeds, 0x00A957D8);         // (12, from Init)

GAME_ARRAY(tStreamingChannel, ms_channel, 0x00CA38B0);

} // namespace CStreaming

// ---- the rest of CStreaming (Streaming2.cpp) ------------------------------------------

namespace game {
struct MemoryBuffer;
}

// An animation group's file (.agr): its animations are ids AGR_ID_BASE + ...
struct AgrEntry {
    uint8_t unknown00[4];
    char name[0x28];        // +0x04
    uint16_t numRefs;       // +0x2C
    uint8_t unknown2E[6];
    uint8_t loaded;         // +0x34
    uint8_t unknown35[7];
};
static_assert(sizeof(AgrEntry) == 0x3C);
static_assert(offsetof(AgrEntry, numRefs) == 0x2C);
static_assert(offsetof(AgrEntry, loaded) == 0x34);

struct AgrGroup {
    uint8_t unknown00[0x68];
    uint32_t numEntries;    // +0x68
    re::Ptr32<AgrEntry> entries;      // +0x6C
};
static_assert(offsetof(AgrGroup, entries) == 0x6C);

// An archive directory's entry (the .dir files, 0x20 bytes each)
struct CdDirEntry {
    uint32_t posn;   // sectors
    uint32_t size;   // sectors
    char name[24];
};
static_assert(sizeof(CdDirEntry) == 0x20);

// A memory-loaded file (CFileMgr): {data, size, position}
struct LoadedFile {
    re::Ptr32<void> data;
    uint32_t size;
    uint32_t position;
};

namespace CStreaming {

GAME_VAR(uint16_t, ms_numInfos, 0x00CA2D58);            // CStreamingInfos used by the id ranges
GAME_VAR(uint16_t, ms_lastAddedRange, 0x00CA2AF4);
// the archive images' starts (sectors, -1: none) and the one the last read was in
GAME_ARRAY(uint32_t, ms_imageOffsets, 0x00CA3890);      // 4, preceded by ms_imageSize
GAME_VAR(uint32_t, ms_imageSize, 0x00CA388C);
GAME_VAR(int32_t, ms_currentImage, 0x00CA3888);
GAME_ARRAY(int32_t, ms_aUnknownCA38A0, 0x00CA38A0);    // 4, per image (see RemoveAgr)
// animation groups: the files and their first ids
GAME_VAR(re::Ptr32<AgrGroup>*, ms_pAgrGroups, 0x00CA3990);
GAME_VAR(uint32_t*, ms_pAgrFirstIds, 0x00CA3994);
GAME_VAR(uint32_t, ms_numAgrGroups, 0x00CA3998);
// the ped models loaded (a ring, -1: free)
GAME_ARRAY(int32_t, ms_pedsLoaded, 0x00CA3928);         // 20
GAME_VAR(int32_t, ms_lastPedSlotRemoved, 0x00CA3694);
GAME_ARRAY(int32_t, ms_aCatNodes, 0x00CA3698);         // 0x78: one per prop animation file (.cat)
GAME_VAR(void*, ms_pExtraObjectsDir, 0x00CA2D5C);       // CDirectory: archive entries for no model yet
GAME_VAR(uint8_t, ms_bUseIpc, 0x00BF3811);              // placement files are .ipc, not .ipb
GAME_VAR(uint8_t, ms_bModelNamesHashed, 0x00BF380C);
GAME_VAR(int32_t*, ms_pFreeMemory, 0x00D141A8);         // the heap's free bytes
GAME_VAR(int32_t, ms_unknownCA398C, 0x00CA398C);
GAME_VAR(uint8_t, ms_bDisableStreaming, 0x00CA3984);   // no requests around the player (Update)
GAME_VAR(uint8_t, ms_bStreamingInitialised, 0x00CA3985);
GAME_VAR(uint32_t*, ms_pEntityListNodes, 0x00C0F788);   // the sectors' entity lists: packed nodes (see Streaming2.cpp)
GAME_ARRAY(int32_t, ms_playerModels, 0x00BF42E0);       // 6, always kept loaded

// 0x52DC10: the CRT's bsearch, but returning -1 - (where it'd go) when not found
int __cdecl BinarySearch(const void* key, const void* base, uint32_t count, uint32_t size, int(__cdecl* compare)(const void*, const void*));
// 0x52DC70
int __cdecl CompareIdRange(const void* a, const void* b);
// 0x52DD30
CStreamingInfo* __cdecl GetInfo(int id);
// 0x52DDA0: `count` ids from `firstId` get CStreamingInfos (overlaps with ranges
// there already are skipped)
void __cdecl AddIdRange(int firstId, int count);
// 0x52D8C0: the start of the archive image a position is in (the nearest)
uint32_t __cdecl GetImageBase(uint32_t posn);
// 0x52D800 / 0x52D810
void __cdecl SetCatNode(int cat, int value);
int __cdecl GetCatNode(int cat);
// 0x52D820
void __cdecl Shutdown();
// 0x52D8A0: the texture heap has `bytes` free
uint8_t __cdecl HasTextureMemoryFor(int bytes);
// 0x52D9A0
void __cdecl ClearUnknownCA3688();
// 0x52D9B0: a free slot in ms_pedsLoaded, -1: none
int __cdecl FindFreePedSlot();
// 0x52D9D0 / 0x52DA60: the animation group files (once)
uint8_t __cdecl InitAgrGroups(uint32_t count);
uint8_t __cdecl AddAgrGroup(AgrGroup* group, uint32_t index);
// 0x52DAD0: an animation's id by name, -1: none
int __cdecl FindAgrId(const char* name);
// 0x52DB50: the group an animation id is in (and its index there), nullptr: none
AgrGroup* __cdecl GetAgrGroup(uint32_t id, uint32_t* index);
// 0x52DBD0: the id of a group's `index`th animation, 0 if the group isn't there
int __cdecl GetAgrId(AgrGroup* group, int index);
// 0x52DFF0: a collision file's entry marked loaded (or not), without loading anything
void __cdecl SetColLoaded(int col, int loaded);
// 0x52E1B0: an archive's directory: the entries for the ids they're for
void __cdecl LoadCdDirectory(const char* path);
// 0x52E020: the placement files (.ipb/.ipc) of an archive, read in now
void __cdecl LoadCdDirectoryIPB(const char* path);
// 0x52ED40: those of every image
void __cdecl LoadCdDirectoryIPBFiles();
// 0x52E650: the heap has room for `bytes` (for a dictionary: the texture heap too)
uint8_t __cdecl HasMemoryFor(int bytes, int id);
// 0x52E720 / 0x52E780: a loaded entry marked mission / game required
void __cdecl SetLoadedMissionRequired(int id);
void __cdecl SetLoadedGameRequired(int id);
// 0x52E7F0: an entry's archive position swapped with an extra entry's (by its index)
void __cdecl SwapWithExtraEntry(int id, int extra);
// 0x52E870: an entry's archive position added as a named extra entry
void __cdecl AddExtraEntry(int id, const char* name);
// 0x52EAE0: an entry's file name (a static buffer)
const char* __cdecl GetFileName(int id);
// 0x52FB40: an animation is used or wanted by a model being requested or read
uint8_t __cdecl IsAgrWanted(int id);
// 0x52FE40 / 0x52FF40: an entry (and what a model needs) no longer game / mission
// required: into the loaded list, or dropped if it isn't loaded
void __cdecl SetModelIsDeletable(int id);
void __cdecl SetMissionDoesntRequireModel(int id);
// 0x530040: the same for a special model (ids from 0xA136D0)
void __cdecl SetMissionDoesntRequireSpecialModel(int index);
// 0x530060: the next loaded ped model nothing uses, removed (true if one was)
uint8_t __cdecl RemoveLoadedPed();
// 0x530860
void __cdecl RemoveTxdModel(int txd);
// 0x530880
void __cdecl RemoveAgr(int id);
// 0x5308D0: the world's objects deleted, and their models when unused
void __cdecl DeleteAllRwObjects();
// 0x530A70: every unused model removed (showProgress: keeps the loading screen alive)
void __cdecl RemoveAllUnusedModels(int showProgress);
// 0x530B50
void __cdecl RemoveUnusedModelsInLoadedList();
// 0x530C70: the oldest loaded entry nothing uses (and without `keepFlags`) removed
uint8_t __cdecl RemoveLeastUsedModel(uint32_t keepFlags);
// 0x531840
void __cdecl MakeSpaceFor(int bytes);
// 0x531880: room made for an entry about to be made from its data (game::AccountForRead)
void __cdecl MakeSpaceForMemoryObject(int bytes, int id);
// 0x531930
void __cdecl DeleteAllRwObjectsIf(int doIt);
// 0x532270 / 0x5322C0: the big buildings (LOD) requested (near `pos`)
void __cdecl RequestBigBuildings();
void __cdecl RequestBigBuildingsNear(int unused, const float* pos);
// 0x533030: loaded: the flags set; else requested (with 0x14), false
uint8_t __cdecl RequestModelIfNotLoaded(int id, uint32_t flags);
// 0x533130: the channels' reads finished now
void __cdecl FlushChannels();
// 0x5331B0: every request dropped
void __cdecl FlushRequestList();
// 0x533460
void __cdecl RequestPlayerModels();

// 0x531950: a model from its data, once its dictionary and animations are there
// (else it's asked for again): false if it wasn't made
uint8_t __cdecl LoadModel(game::MemoryBuffer* buffer, int stream, int id, CStreamingInfo* info, CBaseModelInfo* mi);
// 0x5317F0: by model type
uint8_t __cdecl LoadModelData(game::MemoryBuffer* buffer, int id, CStreamingInfo* info, CBaseModelInfo* mi);
// 0x533200: a model given another model's data (by name), taken from its entities first
void __cdecl RequestSpecialModel(int id, const char* name, uint32_t flags);
// 0x5333E0: a model given an extra entry's data (found by its name hash)
void __cdecl RequestModelFromExtraEntry(int id, uint32_t hash, uint32_t flags);
// 0x5334C0
void __cdecl Init();
// 0x533880: everything not kept dropped, the player's models asked for again
void __cdecl ReInit();
// 0x533980: the mission's models let go, the world's objects deleted
void __cdecl RemoveMissionModels();
// 0x534510: the world around `pos` loaded now (loading screens)
void __cdecl LoadScene(const float* pos);
// 0x5346B0: once a frame
void __cdecl Update();

// The world sectors' entity lists (36 x 36 sectors, five lists each): a list is a
// pointer to its first node; a node is packed: pool (4 bits), slot (14), the next
// node's index in ms_pEntityListNodes (14, 0x3FFF: none).
// 0x52E4F0: their objects deleted (not those kept)
void __cdecl DeleteRwObjectsInList(re::Ptr32<uint32_t>* list);
// 0x52E550: objects made for those in the current area
void __cdecl CreateRwObjectsInList(re::Ptr32<uint32_t>* list);
// 0x52E5E0
void __cdecl ProcessEntitiesInList(re::Ptr32<uint32_t>* list);
// 0x52F080 / 0x52F0E0: every sector's (0x52F0E0: lists 0, 1 and 4)
void __cdecl DeleteAllSectorRwObjects();
void __cdecl DeleteSectorRwObjects();
// 0x52F590: the big buildings' objects near `pos` made
void __cdecl InstanceBigBuildingsNear(int unused, const float* pos);
// 0x52F600
void __cdecl DeletePropAnimRwObjects();
// 0x52F8B0 / 0x52F9A0: a list's objects deleted and their models removed until
// there's room (force: one is enough); true if there is
uint8_t __cdecl DeleteRwObjectsBehindCameraInList(re::Ptr32<uint32_t>* list, int bytes, int id, int force);
uint8_t __cdecl DeleteRwObjectsNotInFrustumInList(re::Ptr32<uint32_t>* list, int bytes, int id, int force);
// 0x52E8D0: the seasonal dictionaries (and their models) flagged, at Init
void __cdecl SetSeasonalFlags();
// 0x5348D0: a new season: the unused models dropped, the models' dictionaries swapped
void __cdecl SwapSeasonalTextures();

} // namespace CStreaming

// Not reversed yet: called where they are. (Reversed ones called through their
// address too, so bullyre.ini decides which runs.)
namespace game {
GAME_FN(uint8_t, __cdecl, ConvertBufferToObject, 0x00531CB0, void* data, int id, int priority);
GAME_FN(int, __cdecl, GetNextFileOnCd, 0x00532A10, uint32_t posn, int priority);
// the archive reader thread (CdStream)
GAME_FN(int, __cdecl, CdStreamGetPosition, 0x0073A680);
GAME_FN(int, __cdecl, GetImageBase, 0x0052D8C0, int posn);   // the start of the archive a position is in
GAME_FN(int, __cdecl, CdStreamRead, 0x0073ABA0, int channel, void* buffer, uint32_t sector, uint32_t sectors);
GAME_FN(int, __cdecl, CdStreamGetStatus, 0x0073A630, int channel);   // 0 done, 0xFF busy, 0xFA (retrying), else an error
GAME_FN(int, __cdecl, ProcessLoadingChannel, 0x00532750, int channel, int finish);
GAME_FN(void, __cdecl, RequestModelStream, 0x00532C80, int channel);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, uint32_t flags);
// 0x532510: the second part of a big entry's loading (LOADSTATE_FINISHING)
GAME_FN(uint8_t, __cdecl, FinishLoadingLargeFile, 0x00532510, void* data, int id, int finish);
GAME_FN(void, __cdecl, AccountForRead, 0x00531880, uint32_t bytes, int id);   // (before an entry is made)
GAME_FN(uint8_t, __cdecl, PedModelsNeeded_530060, 0x00530060);
GAME_FN(int, __cdecl, Fn_52D9B0, 0x0052D9B0);
GAME_FN(int, __cdecl, TxdGetNumRefs, 0x005F11E0, int txd);
GAME_FN(void, __cdecl, RemoveTxdUnused_530860, 0x00530860, int txd);
// FinishLoadingLargeFile's
GAME_FN(void, __thiscall, ModelInfo_50EBC0, 0x0050EBC0, CBaseModelInfo* mi);
GAME_FN(void, __cdecl, Txd_5F1100, 0x005F1100, int txd);
GAME_FN(void, __cdecl, Txd_5F11B0, 0x005F11B0, int txd);
GAME_FN(void, __cdecl, Txd_5F1130, 0x005F1130, int txd);
GAME_FN(uint8_t, __cdecl, FinishLoadingTxd, 0x005F10C0, int txd, int stream);
GAME_FN(void, __thiscall, AnimManager_6BD030, 0x006BD030, void* animManager, uintptr_t group, void* data, int);
GAME_FN(uint8_t, __cdecl, LoadIpbPart, 0x00439760, int ipb, void* data, size_t size, int finish);
// RetryLoadFile's
GAME_FN(void, __cdecl, Fn_738780, 0x00738780);
// LoadAllRequestedModels'
GAME_FN(void, __cdecl, LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);
GAME_FN(void, __cdecl, FlushChannels, 0x00533130);
GAME_FN(int, __cdecl, CdStreamSync, 0x0073A690, int channel);   // waits; 0 when the read went through
// the game's memory manager: allocation, and the tag new allocations are counted under
GAME_FN(void*, __cdecl, MemoryAlloc, 0x005EE830, size_t size);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
// a stream over memory: {data, size}; kind 3, mode 1 (read)
struct MemoryBuffer {
    re::Ptr32<void> data;
    uint32_t size;
};
GAME_FN(int, __cdecl, OpenStream, 0x0042D4C0, int kind, int mode, MemoryBuffer* buffer);
GAME_FN(void, __cdecl, CloseStream, 0x0042D570, uintptr_t stream, MemoryBuffer* buffer);
// the loaders, by kind of entry
GAME_FN(uint8_t, __cdecl, LoadModel, 0x00531950, MemoryBuffer* buffer, int stream, int id, CStreamingInfo* info, CBaseModelInfo* mi);
GAME_FN(uint8_t, __cdecl, IsTxdWanted, 0x0052FA90, int txd);   // (when nothing keeps it in memory)
GAME_FN(uint8_t, __cdecl, LoadTxd, 0x005F29F0, int txd, uintptr_t stream);
GAME_FN(void, __cdecl, RemoveTxd, 0x005F1830, int txd);
GAME_FN(uint8_t, __cdecl, LoadCol, 0x0056FAC0, int col, void* data, size_t size);
GAME_FN(uint8_t, __cdecl, IsAgrWanted, 0x0052FB40, int id);
GAME_FN(int, __cdecl, GetAgrGroup, 0x0052DB50, int id, re::Ptr32<void>* out);
// (thiscall on the animation manager at 0x20C4910)
GAME_FN(void, __thiscall, LoadAgr, 0x006BF500, void* animManager, uintptr_t group, void* data, uintptr_t stream);
inline void* const animManager = reinterpret_cast<void*>(0x020C4910);
// (through its thunk at 0x439760, as the game calls it)
GAME_FN(uint8_t, __cdecl, LoadIpb, 0x00439760, int ipb, void* data, size_t size, int finish);   // (the same as LoadIpbPart; callers here ignore the result, as the game does)
// (thiscall on the lip sync store at 0x208B9F0)
GAME_FN(void, __thiscall, LoadLip, 0x0068E0B0, void* lipStore, int lip, void* data);
inline void* const lipStore = reinterpret_cast<void*>(0x0208B9F0);
GAME_FN(void, __cdecl, LoadCatData, 0x005F6E60, size_t size, void* data, uintptr_t);
GAME_FN(void, __cdecl, LoadCat, 0x0051B880, int cat);
// (thiscall on the store at 0xD02850)
GAME_FN(void, __thiscall, LoadLur, 0x005DB0D0, void* lurStore, int lur, void* data, size_t size);
inline void* const lurStore = reinterpret_cast<void*>(0x00D02850);
GAME_FN(void, __cdecl, RemoveModel, 0x0052F640, int id);
// the removers, by kind of entry
GAME_FN(void, __cdecl, ModelInfo_RemoveModel, 0x0051CAC0, int id);   // frees a model's objects
GAME_FN(void, __cdecl, RemovePAnim1, 0x004EA2F0, int anim);
GAME_FN(void, __cdecl, RemovePAnim2, 0x0051B980, int anim);
GAME_FN(void, __cdecl, RemoveCol, 0x0056FC40, int col);
GAME_FN(void, __thiscall, RemoveAgr, 0x006BD0B0, void* animManager, uintptr_t group, void* data);
GAME_FN(void, __cdecl, RemoveIpb, 0x00437D20, int ipb);
GAME_FN(void, __thiscall, RemoveLur, 0x005DB110, void* lurStore, int lur);
// 0x56F480: whether a collision file's area is set up
GAME_FN(uint8_t, __cdecl, IsColAreaSetUp, 0x0056F480, int col);
// 0x450760 (thiscall on the pool at 0xC0F7B4): a slot's field +4, -1 without a pool
GAME_FN(int, __thiscall, Pool_C0F7B4_GetField4, 0x00450760, void* pool, int slot);
inline void* const pool_C0F7B4 = reinterpret_cast<void*>(0x00C0F7B4);
}
