#include "game/Streaming.h"

#include <windows.h>

#include <cstring>

#include "game/ModelInfo.h"
#include "log.h"
#include "reversed.h"

// ---- CStreamingInfo ----------------------------------------------------------------

static uint16_t IndexOf(const CStreamingInfo* info) {
    return uint16_t(uint32_t(reinterpret_cast<uintptr_t>(info) - re::PtrCast<uintptr_t>(CStreaming::ms_pInfoBase)) >> 4);
}

static CStreamingInfo* AtIndex(uint16_t index) {
    return CStreaming::ms_pInfoBase + index;
}

void CStreamingInfo::AddToList(CStreamingInfo* head) {
    next = head->next;
    prev = IndexOf(head);
    head->next = IndexOf(this);
    AtIndex(next)->prev = IndexOf(this);
}

void CStreamingInfo::RemoveFromList() {
    AtIndex(next)->prev = prev;
    AtIndex(prev)->next = next;
    next = 0xFFFF;
    prev = 0xFFFF;
}

// (a member function's address, for the registry)
template <typename T>
static void* Address(T member) {
    return *reinterpret_cast<re::Ptr32<void>*>(&member);
}

static void(__thiscall* Original_AddToList)(CStreamingInfo*, CStreamingInfo*);

REVERSED("CStreamingInfo::AddToList", 0x0052D7B0, Address(&CStreamingInfo::AddToList), nullptr,
         reinterpret_cast<void**>(&Original_AddToList), re::Mode::Ours);

namespace CStreaming {

// ---- ids -----------------------------------------------------------------------------

// 0x52DC10: the game's binary search: the element's index, or -1 - where it would
// go. (Its comparison, 0x52DC70, subtracts the first 16-bit words.)
static int SearchIdRanges(uint16_t id) {
    const CStreamingIdRange* base = ms_aIdRanges;
    const CStreamingIdRange* low = base;
    for (uint32_t n = ms_numIdRanges; n != 0; n >>= 1) {
        const CStreamingIdRange* mid = low + (n >> 1);
        int order = int(id) - int(mid->firstId);
        if (order == 0) {
            return int(mid - base);
        }
        if (order > 0) {
            low = mid + 1;
            n--;
        }
    }
    return -1 - int(low - base);
}

int FindInfoIndex(int id) {
    int range = SearchIdRanges(uint16_t(id));
    if (range < 0) {
        range = -2 - range;  // not a range's first id: the range before where it'd go
    }
    if (range == -1) {
        return -1;
    }
    const CStreamingIdRange& r = ms_aIdRanges[range];
    if (id >= int(r.firstId) + int(r.count)) {
        return -1;
    }
    return id + r.indexDelta;
}

int __cdecl GetInfoIndex(int id) {
    if (int(ms_lastInfoId) == id) {
        return ms_lastInfoIndex;
    }
    int index = FindInfoIndex(id);
    if (index != -1) {
        ms_lastInfoId = uint16_t(id);
        ms_lastInfoIndex = uint16_t(index);
    }
    return index;
}

static int(__cdecl* Original_GetInfoIndex)(int);

static int __cdecl Shadow_GetInfoIndex(int id) {
    int theirs = Original_GetInfoIndex(id);
    int ours = FindInfoIndex(id);
    static uint32_t compared = 0;
    re::Compared("CStreaming::GetInfoIndex", compared);
    // (the game's may come from its cache: the same answer if both are right)
    if (theirs != ours) {
        re::Mismatch("CStreaming::GetInfoIndex", "id %d: game %d, ours %d", id, theirs, ours);
    }
    return theirs;
}

REVERSED("CStreaming::GetInfoIndex", 0x0052DC90, RE_BODY(&GetInfoIndex), reinterpret_cast<void*>(&Shadow_GetInfoIndex),
         reinterpret_cast<void**>(&Original_GetInfoIndex), re::Mode::Shadow);

uint32_t __cdecl GetIdFromInfo(CStreamingInfo* info) {
    // (the game leaves the last candidate's high bits when there's no match)
    uint32_t id = 0;
    int index = int(reinterpret_cast<uintptr_t>(info) - reinterpret_cast<uintptr_t>(ms_aInfoForModel)) >> 4;
    for (int i = 0; i < int(ms_numIdRanges); i++) {
        const CStreamingIdRange& r = ms_aIdRanges[i];
        id = uint32_t(index - r.indexDelta);
        if (id - r.firstId < r.count) {
            return id;
        }
    }
    return id & 0xFFFF0000;
}

static uint32_t(__cdecl* Original_GetIdFromInfo)(CStreamingInfo*);

static uint32_t __cdecl Shadow_GetIdFromInfo(CStreamingInfo* info) {
    uint32_t theirs = Original_GetIdFromInfo(info);
    uint32_t ours = GetIdFromInfo(info);
    static uint32_t compared = 0;
    re::Compared("CStreaming::GetIdFromInfo", compared);
    // callers use the low 16 bits
    if ((theirs & 0xFFFF) != (ours & 0xFFFF)) {
        re::Mismatch("CStreaming::GetIdFromInfo", "info %p: game %u, ours %u", info, theirs & 0xFFFF, ours & 0xFFFF);
    }
    return theirs;
}

REVERSED("CStreaming::GetIdFromInfo", 0x0052DD50, RE_BODY(&GetIdFromInfo), reinterpret_cast<void*>(&Shadow_GetIdFromInfo),
         reinterpret_cast<void**>(&Original_GetIdFromInfo), re::Mode::Shadow);

// ---- requests ------------------------------------------------------------------------

void __cdecl RequestTxd(int txd, uint32_t flags) {
    RequestModel(txd + TXD_ID_BASE, flags);
}

static void(__cdecl* Original_RequestTxd)(int, uint32_t);

REVERSED("CStreaming::RequestTxd", 0x0052D790, RE_BODY(&RequestTxd), nullptr,
         reinterpret_cast<void**>(&Original_RequestTxd), re::Mode::Ours);

void __cdecl RequestModel(int id, uint32_t flags) {
    CStreamingInfo& info = ms_aInfoForModel[GetInfoIndex(id)];
    uint8_t state = info.loadState;
    if (state == LOADSTATE_REQUESTED) {
        // already asked for: it can still become a priority
        if ((flags & STREAMING_PRIORITY) && !(info.flags & STREAMING_PRIORITY)) {
            ms_numPriorityRequests++;
            info.flags |= STREAMING_PRIORITY;
        }
    } else if (state != LOADSTATE_NOT_LOADED) {
        flags &= ~uint32_t(STREAMING_PRIORITY);
    }
    info.flags |= uint8_t(flags);

    if (state == LOADSTATE_LOADED) {
        if ((info.flags & STREAMING_0x10) && id < TXD_ID_BASE) {
            CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
            if (mi->type == MODEL_INFO_TYPE_1 || mi->type == MODEL_INFO_TYPE_3) {
                mi->unknown2C = 0xFF;
            }
        }
        // loaded: to the front of the loaded list (the last to be removed), unless
        // it's kept in memory anyway
        if (info.InList()) {
            info.RemoveFromList();
            if (!(info.flags & STREAMING_KEEP_IN_MEMORY)) {
                info.AddToList(ms_pStartLoadedList);
            }
        }
        return;
    }
    if (state == LOADSTATE_READING || state == LOADSTATE_REQUESTED || state == LOADSTATE_FINISHING) {
        return;
    }
    if (state == LOADSTATE_NOT_LOADED) {
        if (id < TXD_ID_BASE) {
            // a model: its texture dictionary and the files it needs come with it
            CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
            RequestTxd(mi->txdIndex, flags);
            if (mi->type == MODEL_INFO_PANIM) {
                game::PAnimModelInfo_UpdateAnimIndex(mi);
                int anim = mi->GetPAnimIndex();
                if (anim != -1 && game::Pool_C0F7B4_GetField4(game::pool_C0F7B4, anim) == 0) {
                    RequestModel(anim + CAT_ID_BASE, 0);
                }
            }
            // (each asked for in turn, as the game does)
            int (CBaseModelInfo::*const dependencies[])() = {&CBaseModelInfo::GetAnimFileIndex, &CBaseModelInfo::GetDependency8,
                                                             &CBaseModelInfo::GetDependency9, &CBaseModelInfo::GetDependency10};
            for (auto get : dependencies) {
                int dependency = (mi->*get)();
                if (dependency != -1) {
                    RequestModel(dependency, flags);
                }
            }
        } else if (unsigned(id - AGR_ID_BASE) < 0x244) {
            // kept in memory since it was first read: straight in
            if (ms_pPreloaded[id] != nullptr) {
                game::ConvertBufferToObject(ms_pPreloaded[id], id, false);
                return;
            }
        } else if (unsigned(id - COL_ID_BASE) < 500 && game::IsColAreaSetUp(id - COL_ID_BASE)) {
            game::ConvertBufferToObject(nullptr, id, false);
            return;
        }
        info.AddToList(ms_pStartRequestedList);
        ms_numModelsRequested++;
        if (flags & STREAMING_PRIORITY) {
            ms_numPriorityRequests++;
        }
    }
    info.loadState = LOADSTATE_REQUESTED;
    info.flags = uint8_t((info.flags & STREAMING_KEPT_BITS) | uint8_t(flags));
}

static void(__cdecl* Original_RequestModel)(int, uint32_t);

REVERSED("CStreaming::RequestModel", 0x00532020, RE_BODY(&RequestModel), nullptr,
         reinterpret_cast<void**>(&Original_RequestModel), re::Mode::Ours);

// ---- loading ---------------------------------------------------------------------------

bool __cdecl ConvertBufferToObject(void* data, int id, int priority) {
    CStreamingInfo& info = ms_aInfoForModel[GetInfoIndex(id)];
    size_t size = size_t(info.cdSize) << 11;
    game::MemoryBuffer buffer{data, size};
    int stream = game::OpenStream(3, 1, &buffer);
    CBaseModelInfo* mi = nullptr;

    if (id < TXD_ID_BASE) {
        mi = CModelInfo::ms_modelInfoPtrs[id];
        if (!game::LoadModel(&buffer, stream, id, &info, mi)) {
            return false;  // (the stream stays open, as in the game)
        }
    } else if (id < COL_ID_BASE) {
        int txd = id - TXD_ID_BASE;
        // no longer wanted by the time it arrived
        if (!(info.flags & (STREAMING_KEEP_IN_MEMORY | 4)) && !game::IsTxdWanted(txd)) {
            game::RemoveModel(id);
            game::CloseStream(stream, &buffer);
            return false;
        }
        game::PushMemoryTag(0xF);
        bool loaded = game::LoadTxd(txd, stream);
        game::PopMemoryTag();
        if (!loaded) {
            game::RemoveTxd(txd);
            game::RemoveModel(id);
            RequestModel(id, re::Flag(priority) ? info.flags | STREAMING_PRIORITY : info.flags);
            game::CloseStream(stream, &buffer);
            return false;
        }
    } else if (id < AGR_ID_BASE) {
        game::PushMemoryTag(0x12);
        bool loaded = game::LoadCol(id - COL_ID_BASE, data, size);
        game::PopMemoryTag();
        if (!loaded) {
            game::RemoveModel(id);
            RequestModel(id, info.flags);
            game::CloseStream(stream, &buffer);
            return false;
        }
    } else if (id < IPB_ID_BASE) {
        // animation groups: the first time, the data is kept (a copy) and the entry let go
        if (ms_pPreloaded[id] == nullptr) {
            game::PushMemoryTag(0x3F);
            void* copy = game::MemoryAlloc(size);
            ms_pPreloaded[id] = copy;
            memcpy(copy, data, size);
            game::PopMemoryTag();
            game::RemoveModel(id);
            game::CloseStream(stream, &buffer);
            return true;
        }
        if (!(info.flags & (STREAMING_KEEP_IN_MEMORY | 4)) && !game::IsAgrWanted(id)) {
            game::RemoveModel(id);
            game::CloseStream(stream, &buffer);
            return false;
        }
        game::PushMemoryTag(0x13);
        re::Ptr32<void> groupData = nullptr;
        int group = game::GetAgrGroup(id, &groupData);
        game::LoadAgr(game::animManager, group, groupData, stream);
        game::PopMemoryTag();
    } else if (id < LIP_ID_BASE) {
        game::PushMemoryTag(0x12);
        game::LoadIpb(id - IPB_ID_BASE, data, size, 0);
        game::PopMemoryTag();
    } else if (id < CAT_ID_BASE) {
        game::LoadLip(game::lipStore, id - LIP_ID_BASE, data);
    } else if (id < LUR_ID_BASE) {
        game::LoadCatData(size, data, 0);
        game::LoadCat(id - CAT_ID_BASE);
    } else {
        game::PushMemoryTag(0x23);
        game::LoadLur(game::lurStore, id - LUR_ID_BASE, data, size);
        game::PopMemoryTag();
    }
    game::CloseStream(stream, &buffer);

    // loaded: into the loaded list (most kinds), unless it's kept in memory anyway
    bool listed;
    if (id < TXD_ID_BASE) {
        if (mi->type == 7) {
            listed = false;
        } else {
            if ((mi->type == MODEL_INFO_TYPE_1 || mi->type == MODEL_INFO_TYPE_3) && !(uint8_t(mi->flags) & 0x10)) {
                mi->unknown2C = (info.flags & STREAMING_0x10) ? 0xFF : 0x00;
            }
            listed = true;
        }
    } else if (id < COL_ID_BASE) {
        listed = true;  // dictionaries
    } else {
        // animation groups and .lur files; not collision, placement, lip sync, prop animations
        listed = unsigned(id - AGR_ID_BASE) < 0x244 || id >= LUR_ID_BASE;
    }
    if (listed && !(info.flags & STREAMING_KEEP_IN_MEMORY)) {
        info.AddToList(ms_pStartLoadedList);
    }
    if (info.loadState != LOADSTATE_FINISHING) {
        info.loadState = LOADSTATE_LOADED;
    }
    return true;
}

// ---- removing ------------------------------------------------------------------------

// Frees what an entry made (not models: RemoveModel does those, when fully loaded).
static void FreeObject(int id) {
    if (id < TXD_ID_BASE) {
        return;
    }
    if (id < COL_ID_BASE) {
        game::RemoveTxd(id - TXD_ID_BASE);
    } else if (id < AGR_ID_BASE) {
        game::RemoveCol(id - COL_ID_BASE);
    } else if (id < IPB_ID_BASE) {
        re::Ptr32<void> data = nullptr;
        int group = game::GetAgrGroup(id, &data);
        game::RemoveAgr(game::animManager, group, data);
    } else if (id < LUR_ID_BASE) {
        if (id < LIP_ID_BASE) {
            game::RemoveIpb(id - IPB_ID_BASE);
        }
        // (lip sync and prop animations: nothing to free)
    } else {
        game::RemoveLur(game::lurStore, id - LUR_ID_BASE);
    }
}

void __cdecl RemoveModel(int id) {
    CStreamingInfo& info = ms_aInfoForModel[GetInfoIndex(id)];
    if (info.loadState == LOADSTATE_NOT_LOADED) {
        return;
    }
    if (info.loadState == LOADSTATE_LOADED) {
        if (id < TXD_ID_BASE) {
            CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
            game::ModelInfo_RemoveModel(id);
            if (mi->type == MODEL_INFO_PANIM) {
                int anim = mi->GetPAnimIndex();
                if (anim != -1 && game::Pool_C0F7B4_GetField4(game::pool_C0F7B4, anim) == 0) {
                    game::RemovePAnim1(anim);
                    game::RemovePAnim2(anim);
                }
            }
        } else {
            FreeObject(id);
        }
    }
    if (info.InList()) {
        if (info.loadState == LOADSTATE_REQUESTED) {
            ms_numModelsRequested--;
            if (info.flags & STREAMING_PRIORITY) {
                ms_numPriorityRequests--;
                info.flags &= ~STREAMING_PRIORITY;
            }
        }
        info.RemoveFromList();
    } else if (info.loadState == LOADSTATE_READING) {
        // being read: the channels forget it
        for (int i = 0; i < 4; i++) {
            for (int channel = 0; channel < 2; channel++) {
                if (ms_channel[channel].modelIds[i] == id) {
                    ms_channel[channel].modelIds[i] = -1;
                }
            }
        }
    }
    if (info.loadState == LOADSTATE_FINISHING && id >= TXD_ID_BASE) {
        FreeObject(id);
    }
    info.loadState = LOADSTATE_NOT_LOADED;
}

// ---- reading ---------------------------------------------------------------------------

// A model is ready to be read once its dictionary and the files it needs are loaded
// (or being read). Otherwise: -1 to wait (something it needs is already requested),
// or the id it's missing (to be requested).
enum { MODEL_READY = -2, MODEL_WAIT = -1 };

static int ModelDependencyState(int id) {
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
    uint8_t txdState = ms_aInfoForModel[GetInfoIndex(mi->txdIndex + TXD_ID_BASE)].loadState;
    if (txdState == LOADSTATE_REQUESTED) {
        return MODEL_WAIT;
    }
    if (txdState != LOADSTATE_LOADED && txdState != LOADSTATE_READING) {
        return mi->txdIndex + TXD_ID_BASE;
    }
    int (CBaseModelInfo::*const dependencies[])() = {&CBaseModelInfo::GetAnimFileIndex, &CBaseModelInfo::GetDependency8,
                                                     &CBaseModelInfo::GetDependency9, &CBaseModelInfo::GetDependency10};
    for (auto get : dependencies) {
        int dependency = (mi->*get)();
        if (dependency == -1) {
            continue;
        }
        uint8_t state = ms_aInfoForModel[GetInfoIndex(dependency)].loadState;
        if (state == LOADSTATE_REQUESTED) {
            return MODEL_WAIT;
        }
        if (state != LOADSTATE_LOADED && state != LOADSTATE_READING) {
            return dependency;
        }
    }
    return MODEL_READY;
}

int __cdecl GetNextFileOnCd(uint32_t posn, int priority) {
    for (;;) {
        uint32_t nearestPosn = 0xFFFFFFFF, firstPosn = 0xFFFFFFFF;
        int nearestId = -1, firstId = -1;
        CStreamingInfo* info = AtIndex(ms_pStartRequestedList->next);
        while (info != ms_pEndRequestedList) {
            int id = int(GetIdFromInfo(info) & 0xFFFF);
            CStreamingInfo* next = AtIndex(info->next);
            if (re::Flag(priority) && ms_numPriorityRequests != 0 && !(info->flags & STREAMING_PRIORITY)) {
                info = next;
                continue;
            }
            if (id < TXD_ID_BASE) {
                int state = ModelDependencyState(id);
                if (state == MODEL_WAIT) {
                    info = next;
                    continue;
                }
                if (state != MODEL_READY) {
                    // ask for what it's missing, and start over (the list changed)
                    RequestModel(state, ms_aInfoForModel[GetInfoIndex(id)].flags);
                    info = AtIndex(ms_pStartRequestedList->next);
                    continue;
                }
            }
            CStreamingInfo& entry = ms_aInfoForModel[GetInfoIndex(id)];
            if (entry.cdSize == 0) {
                // nothing to read: loaded as it is
                ms_numModelsRequested--;
                if (entry.flags & STREAMING_PRIORITY) {
                    ms_numPriorityRequests--;
                    entry.flags &= ~STREAMING_PRIORITY;
                }
                info->RemoveFromList();
                info->loadState = LOADSTATE_LOADED;
                info = next;
                continue;
            }
            if (entry.cdPosn < firstPosn) {
                firstPosn = entry.cdPosn;
                firstId = id;
            }
            if (entry.cdPosn < nearestPosn && entry.cdPosn >= posn) {
                nearestPosn = entry.cdPosn;
                nearestId = id;
            }
            info = next;
        }
        if (nearestId != -1) {
            return nearestId;
        }
        if (firstId != -1) {
            return firstId;
        }
        if (ms_numPriorityRequests == 0) {
            return -1;
        }
        // no priority request could go: all of them, then
        ms_numPriorityRequests = 0;
        priority = 0;
    }
}

void __cdecl RequestModelStream(int channel) {
    uint32_t total = 0;         // sectors so far
    bool haveType8 = false;     // (one model of type 8 per read)
    int cdPos = game::CdStreamGetPosition();
    int imageBase = game::GetImageBase(cdPos);
    if (ms_numModelsRequested == 0) {
        return;
    }
    int id = game::GetNextFileOnCd(uint32_t(cdPos - imageBase), true);
    if (id == -1) {
        return;
    }
    // (like the game, these carry the last entry with a size over entries without one)
    uint32_t firstPosn = 0, size = 0, posn = 0;

    // what nobody wants any more by now is dropped, and the one after it taken
    while (!(ms_aInfoForModel[GetInfoIndex(id)].flags & (STREAMING_KEEP_IN_MEMORY | 4)) &&
           ((unsigned(id - TXD_ID_BASE) < 10000 && !game::IsTxdWanted(id - TXD_ID_BASE)) ||
            (unsigned(id - AGR_ID_BASE) < 0x244 && !game::IsAgrWanted(id)))) {
        game::RemoveModel(id);
        const CStreamingInfo& dropped = ms_aInfoForModel[GetInfoIndex(id)];
        if (dropped.cdSize != 0) {
            firstPosn = dropped.cdPosn;
            size = dropped.cdSize;
        }
        id = game::GetNextFileOnCd(firstPosn + size, true);
        if (id == -1) {
            return;
        }
    }

    const CStreamingInfo& first = ms_aInfoForModel[GetInfoIndex(id)];
    if (first.cdSize != 0) {
        firstPosn = first.cdPosn;
        size = first.cdSize;
    }
    tStreamingChannel& ch = ms_channel[channel];
    int count = 0;
    for (; count < 4; count++) {
        if (id == -1) {
            break;
        }
        CStreamingInfo& info = ms_aInfoForModel[GetInfoIndex(id)];
        if (info.loadState != LOADSTATE_REQUESTED) {
            break;
        }
        if (info.cdSize != 0) {
            posn = info.cdPosn;
            size = info.cdSize;
        }
        // entries marked 0x40 must follow on in the archive; the rest are assumed to
        if ((info.flags & 0x40) && posn != firstPosn + total) {
            break;
        }
        if (ms_numPriorityRequests != 0 && !(info.flags & STREAMING_PRIORITY)) {
            break;
        }
        if (id < TXD_ID_BASE) {
            CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
            if (haveType8 && mi->type == 8) {
                break;
            }
            uint8_t txdState = ms_aInfoForModel[GetInfoIndex(mi->txdIndex + TXD_ID_BASE)].loadState;
            if (txdState != LOADSTATE_LOADED && txdState != LOADSTATE_READING) {
                break;
            }
            // (the game looks at the files it needs here, then reads it either way)
            int dependencies[4] = {mi->GetAnimFileIndex(), mi->GetDependency8(), mi->GetDependency9(), mi->GetDependency10()};
            for (int dependency : dependencies) {
                if (dependency != -1) {
                    uint8_t state = ms_aInfoForModel[GetInfoIndex(dependency)].loadState;
                    if (state != LOADSTATE_LOADED && state != LOADSTATE_READING) {
                        break;
                    }
                }
            }
        }
        ch.modelOffsets[count] = int32_t(total);
        ch.modelIds[count] = id;
        total += size;
        if (int32_t(total) > ms_streamingBufferSize && count > 0) {
            total -= size;  // doesn't fit: next time
            break;
        }
        if (id < TXD_ID_BASE && CModelInfo::ms_modelInfoPtrs[id]->type == 8) {
            haveType8 = true;
        }
        info.loadState = LOADSTATE_READING;
        info.RemoveFromList();
        ms_numModelsRequested--;
        if (info.flags & STREAMING_PRIORITY) {
            ms_numPriorityRequests--;
            info.flags &= ~STREAMING_PRIORITY;
        }
        id = info.nextOnCd;
    }
    for (int i = count; i < 4; i++) {
        ch.modelIds[i] = -1;
    }
    game::CdStreamRead(channel, ms_pStreamingBuffer[channel], firstPosn + imageBase, total);
    ch.state = 1;
    ch.stateTimer = 0;
    ch.cdSize = total;
    ch.cdPosn = firstPosn + imageBase;
    ch.tries = 0;
}

int __cdecl ProcessLoadingChannel(int channel, int finish) {
    tStreamingChannel& ch = ms_channel[channel];
    char* buffer = static_cast<char*>(ms_pStreamingBuffer[channel]);
    int status = game::CdStreamGetStatus(channel);
    if (status != 0) {
        if (status == 0xFF) {
            // still reading: after too long, it's treated as an error
            if (++ch.stateTimer > 1000) {
                ch.state = 3;
                ch.unknown34 = 0xFE;
                ch.tries = 3000;
                ms_channelError = channel;
            }
        } else if (status != 0xFA) {
            ch.unknown34 = status;
            ms_channelError = channel;
            ch.state = 3;
        }
        return 0;
    }
    if (ch.state == 2) {
        // a big entry half made: its second part
        int i = 0;
        while (ch.modelIds[i] == -1 || ms_aInfoForModel[GetInfoIndex(ch.modelIds[i])].loadState != LOADSTATE_FINISHING) {
            i++;
        }
        ch.state = 0;
        if (!game::FinishLoadingLargeFile(buffer + ch.modelOffsets[i] * 0x800, ch.modelIds[i], finish)) {
            ch.state = 2;
            return 1;
        }
        ch.modelIds[i] = -1;
        if (!re::Flag(finish)) {
            return 1;
        }
    }
    ch.state = 0;
    for (int i = 0; i < 4; i++) {
        if (ch.state == 2 && !re::Flag(finish)) {
            return 1;  // a big entry: the rest next time
        }
        int id = ch.modelIds[i];
        if (id == -1) {
            continue;
        }
        CStreamingInfo& info = ms_aInfoForModel[GetInfoIndex(id)];
        uint32_t size = info.cdSize;
        CBaseModelInfo* mi = id < TXD_ID_BASE ? CModelInfo::ms_modelInfoPtrs[id] : nullptr;
        // one ped model too many (and nothing needs it now): dropped instead
        if (mi && mi->type == 7 && ms_numPedsLoaded >= ms_desiredNumPeds && !game::PedModelsNeeded_530060() &&
            (!(info.flags & STREAMING_KEEP_IN_MEMORY) || game::Fn_52D9B0() == -1)) {
            int txd = mi->txdIndex;
            game::RemoveModel(id);
            if (!(info.flags & STREAMING_KEEP_IN_MEMORY)) {
                if (game::TxdGetNumRefs(txd) == 0) {
                    game::RemoveTxdUnused_530860(txd);
                }
            } else {
                game::RequestModel(id, info.flags);
            }
            continue;
        }
        if (id < CAT_ID_BASE) {
            game::AccountForRead(size << 11, id);
        }
        game::ConvertBufferToObject(buffer + ch.modelOffsets[i] * 0x800, id, false);
        ch.modelIds[i] = -1;
        ch.state = 0;
    }
    return 1;
}

bool __cdecl FinishLoadingLargeFile(void* data, int id, int finish) {
    CStreamingInfo& info = ms_aInfoForModel[GetInfoIndex(id)];
    bool done = false;
    if (info.loadState != LOADSTATE_FINISHING) {
        if (id < TXD_ID_BASE) {
            game::ModelInfo_50EBC0(CModelInfo::ms_modelInfoPtrs[id]);
        }
        return false;
    }
    size_t size = size_t(info.cdSize) << 11;
    game::MemoryBuffer buffer{data, size};
    int stream = game::OpenStream(3, 1, &buffer);
    if (id < TXD_ID_BASE) {
        CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
        game::PushMemoryTag(0xD);
        game::Txd_5F1100(mi->txdIndex);
        game::PopMemoryTag();
        game::ModelInfo_50EBC0(mi);
        game::Txd_5F11B0(mi->txdIndex);
        int (CBaseModelInfo::*const dependencies[])() = {&CBaseModelInfo::GetAnimFileIndex, &CBaseModelInfo::GetDependency8,
                                                         &CBaseModelInfo::GetDependency9, &CBaseModelInfo::GetDependency10};
        for (auto get : dependencies) {
            if ((mi->*get)() != -1) {
                re::Ptr32<void> groupData = nullptr;
                int group = game::GetAgrGroup((mi->*get)(), &groupData);
                game::AnimManager_6BD030(game::animManager, group, groupData, 1);
            }
        }
        // (nothing sets `done` for models: as the game has it, a model finished this
        // way is removed and asked for again)
    } else if (id < COL_ID_BASE) {
        int txd = id - TXD_ID_BASE;
        game::Txd_5F1130(txd);
        game::PushMemoryTag(0xF);
        done = game::FinishLoadingTxd(txd, stream);
        game::PopMemoryTag();
        game::Txd_5F11B0(txd);
    } else if (unsigned(id - IPB_ID_BASE) < 100) {
        game::PushMemoryTag(0x12);
        done = game::LoadIpbPart(id - IPB_ID_BASE, data, size, finish);
        game::PopMemoryTag();
    }
    game::CloseStream(stream, &buffer);
    info.loadState = LOADSTATE_LOADED;
    if (!done) {
        game::RemoveModel(id);
        game::RequestModel(id, info.flags);
        return false;
    }
    return true;
}

static uint8_t(__cdecl* Original_FinishLoadingLargeFile)(void*, int, int);

REVERSED("CStreaming::FinishLoadingLargeFile", 0x00532510, RE_BODY(&FinishLoadingLargeFile), nullptr,
         reinterpret_cast<void**>(&Original_FinishLoadingLargeFile), re::Mode::Ours);

void __cdecl RetryLoadFile(int channel) {
    tStreamingChannel& ch = ms_channel[channel];
    game::Fn_738780();
    if (ch.tries >= 3000) {
        ms_bLoadingStalled = 1;
    }
    switch (ch.state) {
    case 0:
        break;
    case 1:
        if (game::ProcessLoadingChannel(channel, false)) {
            ms_bLoadingStalled = 0;
            ms_channelError = -1;
        }
        return;
    case 3: {
        ch.tries++;
        int status = game::CdStreamGetStatus(channel);
        if (status == 0xFF) {
            if (game::CdStreamGetStatus(channel) == 0xFF) {
                return;  // still busy
            }
        } else if (status == 0xFA) {
            return;
        }
        break;
    }
    default:
        return;
    }
    game::CdStreamRead(channel, ms_pStreamingBuffer[channel], ch.cdPosn, ch.cdSize);
    ch.state = 1;
    ch.stateTimer = -600;
}

void __cdecl LoadAllRequestedModels(int priorityOnly) {
    // (the game brackets this with profiler markers, empty in the shipped exe)
    int budget = ms_numModelsRequested * 6;
    if (ms_bLoadingAll) {
        return;
    }
    ms_bLoadingAll = 1;
    if (re::Flag(priorityOnly)) {
        budget = ms_numPriorityRequests;
    }
    game::FlushChannels();
    int imageBase = game::GetImageBase(game::CdStreamGetPosition());
    const char* title = "LoadAllRequestedModels";
    game::LoadingScreen(title, "Start", 0);
    while (AtIndex(ms_pEndRequestedList->prev) != ms_pStartRequestedList && budget >= 1) {
        game::LoadingScreen(title, "In Request loop", 0);
        budget--;
        int id = game::GetNextFileOnCd(0, priorityOnly);
        if (id == -1) {
            break;
        }
        CStreamingInfo& info = ms_aInfoForModel[GetInfoIndex(id)];
        info.RemoveFromList();
        ms_numModelsRequested--;
        bool priority = (info.flags & STREAMING_PRIORITY) != 0;
        ms_channel[0].modelIds[0] = id;
        if (priority) {
            ms_numPriorityRequests--;
            info.flags &= ~STREAMING_PRIORITY;
        }
        if (unsigned(id - AGR_ID_BASE) < 0x244 && ms_pPreloaded[id] != nullptr) {
            game::ConvertBufferToObject(ms_pPreloaded[id], id, priority);
        } else if (info.loadState != LOADSTATE_LOADED) {
            uint32_t size = info.cdSize;
            if (size == 0) {
                info.loadState = LOADSTATE_LOADED;
                continue;
            }
            uint32_t posn = info.cdPosn;
            int read;
            do {
                do {
                    read = game::CdStreamRead(0, ms_pStreamingBuffer[0], posn + imageBase, size);
                } while (game::CdStreamSync(0) != 0);
            } while (read == 0);
            info.loadState = LOADSTATE_READING;
            game::LoadingScreen(title, "MakeSpaceForMemoryObject", 0);
            game::AccountForRead(size << 11, id);
            game::LoadingScreen(title, "ConvertBufferToObject", 0);
            game::ConvertBufferToObject(ms_pStreamingBuffer[0], id, priority);
            game::LoadingScreen(title, "FinishLoadingLargeFile", 0);
            if (id < TXD_ID_BASE) {
                CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
                if (mi->type == MODEL_INFO_TYPE_1 || mi->type == MODEL_INFO_TYPE_3) {
                    mi->unknown2C = 0xFF;
                }
            }
        }
    }
    game::LoadingScreen(title, "Done requests", 0);
    for (int i = 0; i < 4; i++) {
        ms_channel[1].modelIds[i] = -1;
    }
    ms_bSingleChannel = 0;
    ms_channel[1].state = 0;
    ms_bLoadingAll = 0;
}

static void(__cdecl* Original_LoadAllRequestedModels)(int);

// (testing: .time = 1 logs each call's duration and what it loaded; .useGame = 1 has
// the game's code do the work, to compare)
static void __cdecl LoadAllRequestedModels_Timed(int priorityOnly) {
    static const bool time = re::IniInt("CStreaming::LoadAllRequestedModels.time", 0) != 0;
    static const bool useGame = re::IniInt("CStreaming::LoadAllRequestedModels.useGame", 0) != 0;
    if (!time) {
        useGame ? Original_LoadAllRequestedModels(priorityOnly) : LoadAllRequestedModels(priorityOnly);
        return;
    }
    int before = ms_numModelsRequested;
    LARGE_INTEGER f, t0, t1;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t0);
    useGame ? Original_LoadAllRequestedModels(priorityOnly) : LoadAllRequestedModels(priorityOnly);
    QueryPerformanceCounter(&t1);
    double ms = double(t1.QuadPart - t0.QuadPart) * 1000.0 / double(f.QuadPart);
    if (ms >= 1.0) {
        re::Log("LoadAll %s priority %d: %.1f ms, requested %d -> %d", useGame ? "game" : "ours", int(re::Flag(priorityOnly)), ms, before, int(ms_numModelsRequested));
    }
}

REVERSED("CStreaming::LoadAllRequestedModels", 0x00533EE0, RE_BODY(&LoadAllRequestedModels_Timed), nullptr,
         reinterpret_cast<void**>(&Original_LoadAllRequestedModels), re::Mode::Ours);

static void(__cdecl* Original_RetryLoadFile)(int);

REVERSED("CStreaming::RetryLoadFile", 0x00533A50, RE_BODY(&RetryLoadFile), nullptr,
         reinterpret_cast<void**>(&Original_RetryLoadFile), re::Mode::Ours);

static int(__cdecl* Original_ProcessLoadingChannel)(int, int);

REVERSED("CStreaming::ProcessLoadingChannel", 0x00532750, RE_BODY(&ProcessLoadingChannel), nullptr,
         reinterpret_cast<void**>(&Original_ProcessLoadingChannel), re::Mode::Ours);

void __cdecl LoadRequestedModels() {
    if (ms_bSingleChannel) {
        ms_currentChannel = 0;
    }
    if (ms_channel[ms_currentChannel].state == 1 || ms_channel[ms_currentChannel].state == 2) {
        game::ProcessLoadingChannel(ms_currentChannel, false);
    }
    if (ms_channelError != -1) {
        return;  // (sorted out elsewhere)
    }
    if (ms_channel[ms_currentChannel].state == 0) {
        game::RequestModelStream(ms_currentChannel);
    }
    if (ms_channel[ms_currentChannel].state != 2) {
        ms_currentChannel = 1 - ms_currentChannel;
    }
}

static void(__cdecl* Original_LoadRequestedModels)();

REVERSED("CStreaming::LoadRequestedModels", 0x005330A0, RE_BODY(&LoadRequestedModels), nullptr,
         reinterpret_cast<void**>(&Original_LoadRequestedModels), re::Mode::Ours);

static void(__cdecl* Original_RequestModelStream)(int);

REVERSED("CStreaming::RequestModelStream", 0x00532C80, RE_BODY(&RequestModelStream), nullptr,
         reinterpret_cast<void**>(&Original_RequestModelStream), re::Mode::Ours);

static int(__cdecl* Original_GetNextFileOnCd)(uint32_t, int);

REVERSED("CStreaming::GetNextFileOnCd", 0x00532A10, RE_BODY(&GetNextFileOnCd), nullptr,
         reinterpret_cast<void**>(&Original_GetNextFileOnCd), re::Mode::Ours);

static void(__cdecl* Original_RemoveModel)(int);

REVERSED("CStreaming::RemoveModel", 0x0052F640, RE_BODY(&RemoveModel), nullptr,
         reinterpret_cast<void**>(&Original_RemoveModel), re::Mode::Ours);

static uint8_t(__cdecl* Original_ConvertBufferToObject)(void*, int, int);

// (testing: bullyre.ini ConvertBufferToObject.oursFrom / .oursTo limit ours to a
// range of ids; the game's code does the rest)
static bool __cdecl ConvertBufferToObject_InRange(void* data, int id, int priority) {
    static const int from = re::IniInt("CStreaming::ConvertBufferToObject.oursFrom", 0);
    static const int to = re::IniInt("CStreaming::ConvertBufferToObject.oursTo", 0x10000);
    // (testing: .logFrom: every call from this id up, with what it left behind)
    static const int logFrom = re::IniInt("CStreaming::ConvertBufferToObject.logFrom", 0x10000);
    bool ours = id >= from && id < to;
    bool result = ours ? ConvertBufferToObject(data, id, priority) : Original_ConvertBufferToObject(data, id, priority) != 0;
    if (id >= logFrom) {
        const CStreamingInfo& info = ms_aInfoForModel[GetInfoIndex(id)];
        re::Log("CBTO %s id %d data %p -> %d state %d listed %d flags %02X preloaded %p", ours ? "ours" : "game", id, data, int(result),
                int(info.loadState), int(info.InList()), unsigned(info.flags), unsigned(id - AGR_ID_BASE) < 0x244 ? ms_pPreloaded[id] : nullptr);
    }
    return result;
}

REVERSED("CStreaming::ConvertBufferToObject", 0x00531CB0, RE_BODY(&ConvertBufferToObject_InRange), nullptr,
         reinterpret_cast<void**>(&Original_ConvertBufferToObject), re::Mode::Ours);

// ---- IsTxdWanted -----------------------------------------------------------------------

static bool UsesTxd(int id, int txd) {
    return id != -1 && id < TXD_ID_BASE && CModelInfo::ms_modelInfoPtrs[id]->txdIndex == txd;
}

uint8_t __cdecl IsTxdWanted(int txd) {
    for (uint16_t i = ms_pStartRequestedList->next; AtIndex(i) != ms_pEndRequestedList; i = AtIndex(i)->next) {
        if (UsesTxd(int(GetIdFromInfo(AtIndex(i)) & 0xFFFF), txd)) {
            return 1;
        }
    }
    // (the game goes through the two channels side by side: the same answer)
    for (int channel = 0; channel < 2; channel++) {
        for (int id : ms_channel[channel].modelIds) {
            if (UsesTxd(id, txd)) {
                return 1;
            }
        }
    }
    return 0;
}

static uint8_t(__cdecl* Original_IsTxdWanted)(int);

static uint8_t __cdecl Shadow_IsTxdWanted(int txd) {
    uint8_t theirs = Original_IsTxdWanted(txd);
    uint8_t ours = IsTxdWanted(txd);
    static uint32_t compared = 0;
    re::Compared("CStreaming::IsTxdWanted", compared);
    if (re::Flag(theirs) != re::Flag(ours)) {
        re::Mismatch("CStreaming::IsTxdWanted", "txd %d: game %d, ours %d", txd, theirs, ours);
    }
    return theirs;
}

REVERSED("CStreaming::IsTxdWanted", 0x0052FA90, RE_BODY(&IsTxdWanted), reinterpret_cast<void*>(&Shadow_IsTxdWanted),
         reinterpret_cast<void**>(&Original_IsTxdWanted), re::Mode::Ours);

} // namespace CStreaming
