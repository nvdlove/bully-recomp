// NiKFMTool (src/gamebryo/NiActorManager.h), its file side and editing: loading a .kfm (the
// header line's version and kind; the ASCII format of the old versions, the binary one of the
// later), the binary and ASCII readers, the sequences, transitions, blend pairs, chains and
// sequence groups they add, the chain checks, the full paths of the model and of a sequence's KF
// file, and the set templates and string readers these use.
//
// KFM_RC results: 0 success, 2 no such sequence, 3 no such transition, 4 the wrong kind of
// transition, 6 a blend pair without keys, 10 a chain that loops or runs too long, 11 no such
// sequence group, 13 the file not opened, 14 a syntax error (or a non-binary file of a binary
// version), 15 not a KFM file (or a newer version), 16 the wrong endianness.
//
// A SequenceGroup's fields past its name are a NiTPrimitiveSet<SequenceInfo>: base, allocated,
// count (NiActorManager.h names them sequences, count and unknown10).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiActorManager.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::actor {
namespace {

using scene::BinaryStream;
using sys::Decrement;
using sys::FixedStringRelease;
using sys::Increment;
using sys::kNullFixedString;

// NiKFMTool::SequenceGroup::SequenceInfo (0x18 bytes)
struct SequenceInfo {
    uint32_t sequenceID;               // +0x0
    int32_t priority;                  // +0x4
    float weight;                      // +0x8
    float easeInTime;                  // +0xC
    float easeOutTime;                 // +0x10
    uint32_t synchronizeSequenceID;    // +0x14 (0xFFFFFFFE: none)
};
static_assert(sizeof(SequenceInfo) == 0x18);

constexpr uint32_t kNoSyncSequence = 0xFFFFFFFE;
constexpr uintptr_t kMapVtable = 0x00965A9C;        // NiTPointerMap<unsigned int, T*>
constexpr uintptr_t kMapBaseVtable = 0x00950760;
constexpr uintptr_t kSequenceInfoConstruct = 0x00822700;

GAME_VAR(const char*, ms_szKFMCurrentVersion, 0x00BC8C38);   // the newest version this reads

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(void, __thiscall, Map_SetAt, 0x008A5140, PointerMap* self, uint32_t key, void* value);
GAME_FN(void, __thiscall, Map_GetNext, 0x008976C0, PointerMap* self, re::Ptr32<MapNode>* pos, uint32_t* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, PtrSet_Realloc, 0x0083AD20, void* set, uint32_t allocated);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(uint8_t, __cdecl, NiPath_IsRelative, 0x00753B50, const char* path);
GAME_FN(uint32_t, __cdecl, NiPath_ConvertToAbsolute, 0x007530C0, char* out, uint32_t size, const char* relative,
        const char* base);
GAME_FN(uint8_t, __cdecl, NiPath_Standardize, 0x00752C20, char* path);
GAME_FN(void, __cdecl, SourceTexture_StandardizeFilename, 0x00769740, re::Ptr32<char>* filename);
GAME_FN(void, __cdecl, NiStrcpy, 0x00747280, char* dest, uint32_t size, const char* src);
GAME_FN(void, __cdecl, NiStrcat, 0x00749150, char* dest, uint32_t size, const char* src);
GAME_FN(uint32_t, __thiscall, BinaryStream_GetLine, 0x00752B10, BinaryStream* self, char* buffer, uint32_t maxBytes);
GAME_FN(BinaryStream*, __cdecl, NiFile_GetFile, 0x00753380, const char* path, int mode, uint32_t bufferSize);
GAME_FN(uint8_t, __cdecl, NiSystemDesc_IsLittleEndian, 0x007529F0);
GAME_FN(uint32_t, __cdecl, NiStream_GetVersionFromString, 0x0075A2C0, const char* text);
GAME_FN(ChainInfo*, __cdecl, ChainInfo_NewArray, 0x00800A60, uint32_t count);
GAME_FN(Transition*, __thiscall, KFMTool_GetTransitionFromSequence, 0x00800AA0, NiKFMTool* self, uint32_t desID,
        Sequence* sequence);
GAME_FN(Transition*, __thiscall, KFMTool_GetTransition, 0x00801160, NiKFMTool* self, uint32_t srcID, uint32_t desID);
GAME_FN(Transition*, __thiscall, TransitionConstruct, 0x00801510, void* self, uint32_t type, float duration);
GAME_FN(void, __thiscall, Transition_ClearBlendPairs, 0x00801C90, Transition* self);
GAME_FN(void, __thiscall, Sequence_Destruct, 0x00802420, Sequence* self);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, PtrSet_AddCall, 0x008227D0, void* set, const void* value);
GAME_FN(void, __cdecl, Stream_LoadUInt8, 0x00822810, BinaryStream* stream, uint8_t* value);
GAME_FN(SequenceInfo*, __cdecl, SequenceInfo_NewArray, 0x00822870, uint32_t count);
GAME_FN(void, __thiscall, ChainSet_Realloc, 0x008228B0, void* set, uint32_t allocated);
GAME_FN(void, __thiscall, SequenceInfoSet_Realloc, 0x00822910, void* set, uint32_t allocated);
GAME_FN(void, __cdecl, KFMTool_StripLeadingBackslashes, 0x00822990, re::Ptr32<char>* path);
GAME_FN(void, __thiscall, KFMTool_LoadCString, 0x00822BC0, NiKFMTool* self, BinaryStream* stream, re::Ptr32<char>* out);
GAME_FN(void, __thiscall, KFMTool_LoadFixedString, 0x00822C40, NiKFMTool* self, BinaryStream* stream, re::Ptr32<char>* out);
GAME_FN(void, __thiscall, ChainSet_Add, 0x00822CF0, void* set, const ChainInfo* value);
GAME_FN(void, __thiscall, SequenceInfoSet_Add, 0x00822D30, void* set, const SequenceInfo* value);
GAME_FN(Sequence*, __thiscall, Sequence_Construct, 0x00822D90, void* self, uint32_t id, const re::Ptr32<char>* filename,
        int32_t animIndex);
GAME_FN(BlendPair*, __thiscall, BlendPair_Construct, 0x00822E10, void* self, const re::Ptr32<char>* startKey,
        const re::Ptr32<char>* targetKey);
GAME_FN(uint32_t, __thiscall, KFMTool_AddBlendPair, 0x00822E50, NiKFMTool* self, uint32_t srcID, uint32_t desID,
        const re::Ptr32<char>* startKey, const re::Ptr32<char>* targetKey);
GAME_FN(uint32_t, __thiscall, KFMTool_AddSequenceToGroup, 0x00822F20, NiKFMTool* self, uint32_t groupID,
        uint32_t sequenceID, int32_t priority, float weight, float easeIn, float easeOut, uint32_t syncID);
GAME_FN(void, __thiscall, KFMTool_FillChainSequenceIDs, 0x00822FB0, NiKFMTool* self, uint32_t srcID, uint32_t desID,
        Transition* transition, NiTSet<uint32_t>* ids);
GAME_FN(void, __thiscall, KFMTool_FixChainDurations, 0x008230A0, NiKFMTool* self);
GAME_FN(uint32_t, __thiscall, KFMTool_AddSequenceGroup, 0x00823280, NiKFMTool* self, uint32_t groupID,
        const re::Ptr32<char>* name);
GAME_FN(uint8_t, __thiscall, KFMTool_IsValidChainTransition, 0x00823360, NiKFMTool* self, uint32_t srcID,
        uint32_t desID, Transition* transition);
GAME_FN(uint32_t, __thiscall, KFMTool_ReadBinary, 0x008233C0, NiKFMTool* self, BinaryStream* stream,
        uint32_t version);
GAME_FN(uint32_t, __thiscall, KFMTool_AddSequenceToChain, 0x00823E40, NiKFMTool* self, uint32_t srcID,
        uint32_t desID, uint32_t sequenceID, float duration);
GAME_FN(uint32_t, __thiscall, KFMTool_SetTransition, 0x00823F30, NiKFMTool* self, uint32_t srcID, uint32_t desID,
        uint32_t type, float duration);
GAME_FN(uint32_t, __thiscall, KFMTool_AddSequence, 0x00824040, NiKFMTool* self, uint32_t id, const re::Ptr32<char>* filename,
        int32_t animIndex);
GAME_FN(uint32_t, __thiscall, KFMTool_ReadAscii, 0x008240D0, NiKFMTool* self, BinaryStream* file, uint32_t version);

// a binary stream read: `bytes` bytes of `componentSize` components
inline void Read(BinaryStream* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    uint32_t size = componentSize;
    stream->position += stream->read(stream, buffer, bytes, &size, 1);
}

inline void Read4(BinaryStream* stream, void* buffer) {
    Read(stream, buffer, 4, 4);
}

// a sequence group's info set
inline NiTSet<SequenceInfo>* InfoSet(SequenceGroup* group) {
    return reinterpret_cast<NiTSet<SequenceInfo>*>(&group->sequences);
}

inline NiTSet<BlendPair*>* PairSet(Transition* transition) {
    return reinterpret_cast<NiTSet<BlendPair*>*>(&transition->blendPairs);
}

inline NiTSet<ChainInfo>* ChainSet(Transition* transition) {
    return reinterpret_cast<NiTSet<ChainInfo>*>(&transition->chain);
}

// a map made as the constructors inline it: 37 empty buckets
inline void MakeMap(PointerMap& map) {
    map.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    map.hashSize = 0x25;
    map.count = 0;
    map.table = static_cast<re::Ptr32<MapNode>*>(NiMalloc(0x94));
    crt::Memset(map.table, 0, map.hashSize * 4);
    map.vtable = reinterpret_cast<void*>(kMapVtable);
}

// a lookup's node (the value may be null)
inline MapNode* FindNode(const PointerMap& map, uint32_t key) {
    for (MapNode* node = map.table[key % map.hashSize]; node != nullptr; node = node->next) {
        if (key == node->key) {
            return node;
        }
    }
    return nullptr;
}

// NiFixedString's assignment from another handle as it's inlined (the new held first)
inline void AssignHandle(re::Ptr32<char>* slot, char* value) {
    if (*slot != value) {
        if (value != kNullFixedString) {
            Increment(value - 8);
        }
        FixedStringRelease(slot);
        *slot = value;
    }
}

// NiTPrimitiveSet<T>::Add of a 4-byte element as these inline it (Realloc called)
template <typename T>
inline void AddInline(NiTSet<T>* set, T value) {
    static_assert(sizeof(re::Field<T>) == 4);
    if (set->count == set->allocated) {
        PtrSet_Realloc(set, set->allocated != 0 ? set->allocated * 2 : 1);
    }
    set->base[set->count] = value;
    set->count++;
}

// ---- the set templates and helpers

// 0x822700: SequenceInfo() (no weight, no synchronization)
SequenceInfo* __fastcall SequenceInfo_Construct(SequenceInfo* self, re::Edx) {
    self->weight = 0.0f;
    self->sequenceID = 0;
    self->easeInTime = 0.0f;
    self->priority = 0;
    self->easeOutTime = 0.0f;
    self->synchronizeSequenceID = kNoSyncSequence;
    return self;
}

// 0x822730: GetSequenceIDs(ids&, count&): a NiMalloc'd array of the sequences' IDs (none: null)
void __fastcall KFMTool_GetSequenceIDsImpl(NiKFMTool* self, re::Edx, re::Ptr32<uint32_t>* ids, uint32_t* count) {
    uint32_t n = self->sequences.count;
    *count = n;
    if (n == 0) {
        *ids = nullptr;
        return;
    }
    *ids = static_cast<uint32_t*>(NiMalloc(n * 4));
    re::Ptr32<MapNode> pos = MapFirst(self->sequences);
    uint32_t i = 0;
    while (pos != nullptr) {
        uint32_t key;
        re::Ptr32<void> value;
        Map_GetNext(&self->sequences, &pos, &key, &value);
        (*ids)[i++] = key;
    }
}

// 0x8227D0: NiTPrimitiveSet<T*>::Add(value) (doubled when full)
void __fastcall PtrSet_Add(NiTSet<void*>* self, re::Edx, const re::Ptr32<void>* value) {
    if (self->count == self->allocated) {
        PtrSet_Realloc(self, self->allocated != 0 ? self->allocated * 2 : 1);
    }
    self->base[self->count] = *value;
    self->count++;
}

// 0x822810: NiStreamLoadBinary(stream, unsigned char&)
void __cdecl LoadUInt8(BinaryStream* stream, uint8_t* value) {
    Read(stream, value, 1, 1);
}

// 0x822840: NiStreamLoadBinary(stream, unsigned int&)
void __cdecl LoadUInt32(BinaryStream* stream, uint32_t* value) {
    Read(stream, value, 4, 4);
}

// 0x822870: NiNew[] of count SequenceInfos (no count before them)
SequenceInfo* __cdecl SequenceInfo_NewArrayImpl(uint32_t count) {
    uint64_t product = uint64_t(count) * 0x18;
    void* array = NiNewArray((product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product));
    if (array == nullptr) {
        return nullptr;
    }
    VectorConstruct(array, 0x18, count, reinterpret_cast<void*>(kSequenceInfoConstruct));
    return static_cast<SequenceInfo*>(array);
}

// 0x8228B0: NiTPrimitiveSet<ChainInfo>::Realloc(allocated): the used ones copied over
void __fastcall ChainSet_ReallocImpl(NiTSet<ChainInfo>* self, re::Edx, uint32_t allocated) {
    if (allocated == self->allocated) {
        return;
    }
    ChainInfo* base = nullptr;
    if (allocated != 0) {
        base = ChainInfo_NewArray(allocated);
        for (uint32_t i = 0; i < self->count; i++) {
            base[i] = self->base[i];
        }
    }
    NiDeleteArray(self->base, 8);
    self->base = base;
    self->allocated = allocated;
}

// 0x822910: NiTPrimitiveSet<SequenceInfo>::Realloc(allocated)
void __fastcall SequenceInfoSet_ReallocImpl(NiTSet<SequenceInfo>* self, re::Edx, uint32_t allocated) {
    if (allocated == self->allocated) {
        return;
    }
    SequenceInfo* base = nullptr;
    if (allocated != 0) {
        base = SequenceInfo_NewArray(allocated);
        for (uint32_t i = 0; i < self->count; i++) {
            base[i] = self->base[i];
        }
    }
    NiDeleteArray(self->base, 0x18);
    self->base = base;
    self->allocated = allocated;
}

// 0x822990: a path's leading backslash dropped (two for "\\x" without another backslash after;
// a UNC "\\server\..." kept)
void __cdecl KFMTool_StripLeadingBackslashesImpl(re::Ptr32<char>* path) {
    char* s = *path;
    const char* p = s;
    if (s[0] == '\\') {
        if (s[1] == '\\') {
            p = s + 1;
        }
        if (p - s == 1) {
            if (crt::Strrchr(p, '\\') == p) {
                p = p + 1;
            } else {
                p = s;
            }
        } else {
            p = p + 1;
        }
    }
    re::Ptr32<char> old = *path;
    if (old != kNullFixedString) {
        Increment(old - 8);
    }
    FixedString_AssignText(path, p);
    FixedStringRelease(&old);
}

// 0x822A20: GetFullModelPath: the model's path made absolute against the base path (kept in +0xC)
// when it's relative; else the path itself
re::Ptr32<char>* __fastcall KFMTool_GetFullModelPathImpl(NiKFMTool* self, re::Edx) {
    if (self->modelPath != kNullFixedString && NiPath_IsRelative(self->modelPath)) {
        char buffer[0x104];
        NiPath_ConvertToAbsolute(buffer, 0x104, self->modelPath, self->basePath);
        re::Ptr32<char> full = StringTable_AddString(buffer);
        FixedString_Assign(&self->unknownC, &full);
        FixedStringRelease(&full);
        return &self->unknownC;
    }
    return &self->modelPath;
}

// 0x822AF0: GetFullKFFilename(id): the sequence's KF file made absolute against the base path
// when relative; none for an unknown sequence or a null filename
re::Ptr32<char>* __fastcall KFMTool_GetFullKFFilenameImpl(NiKFMTool* self, re::Edx, re::Ptr32<char>* result, uint32_t id) {
    MapNode* node = FindNode(self->sequences, id);
    auto* sequence = node != nullptr ? static_cast<Sequence*>(node->value) : nullptr;
    if (sequence == nullptr || sequence->filename == nullptr) {
        *result = kNullFixedString;
        return result;
    }
    if (!NiPath_IsRelative(sequence->filename)) {
        if (sequence->filename != kNullFixedString) {
            Increment(sequence->filename - 8);
        }
        *result = sequence->filename;
        return result;
    }
    char buffer[0x104];
    NiPath_ConvertToAbsolute(buffer, 0x104, sequence->filename, self->basePath);
    *result = StringTable_AddString(buffer);
    return result;
}

// 0x822BC0: LoadCStringAsBinary(stream, out&): a length then the characters (NiMalloc'd,
// terminated); none for a length under 1
void __fastcall KFMTool_LoadCStringImpl(NiKFMTool*, re::Edx, BinaryStream* stream, re::Ptr32<char>* out) {
    int32_t length;
    Read4(stream, &length);
    if (length > 0) {
        char* text = static_cast<char*>(NiMalloc(uint32_t(length) + 1));
        *out = text;
        Read(stream, text, uint32_t(length), 1);
        (*out)[length] = 0;
        return;
    }
    *out = nullptr;
}

// 0x822C40: LoadFixedStringAsBinary(stream, out&): the same into a NiFixedString (through a
// 0x400-byte buffer, a longer string overrunning it as the game has it); none for a length under 1
void __fastcall KFMTool_LoadFixedStringImpl(NiKFMTool*, re::Edx, BinaryStream* stream, re::Ptr32<char>* out) {
    char buffer[0x400];
    int32_t length;
    Read4(stream, &length);
    const char* text = nullptr;
    if (length > 0) {
        Read(stream, buffer, uint32_t(length), 1);
        buffer[length] = 0;
        text = buffer;
    }
    FixedString_AssignText(out, text);
}

// 0x822CF0: NiTPrimitiveSet<ChainInfo>::Add(value)
void __fastcall ChainSet_AddImpl(NiTSet<ChainInfo>* self, re::Edx, const ChainInfo* value) {
    if (self->count == self->allocated) {
        ChainSet_Realloc(self, self->allocated != 0 ? self->allocated * 2 : 1);
    }
    self->base[self->count] = *value;
    self->count++;
}

// 0x822D30: NiTPrimitiveSet<SequenceInfo>::Add(value)
void __fastcall SequenceInfoSet_AddImpl(NiTSet<SequenceInfo>* self, re::Edx, const SequenceInfo* value) {
    if (self->count == self->allocated) {
        SequenceInfoSet_Realloc(self, self->allocated != 0 ? self->allocated * 2 : 1);
    }
    self->base[self->count] = *value;
    self->count++;
}

// 0x822D90: Sequence(id, filename, anim index): no transitions
Sequence* __fastcall Sequence_ConstructImpl(Sequence* self, re::Edx, uint32_t id, const re::Ptr32<char>* filename,
                                            int32_t animIndex) {
    self->sequenceID = id;
    if (*filename != kNullFixedString) {
        Increment(*filename - 8);
    }
    self->filename = *filename;
    self->animIndex = uint32_t(animIndex);
    MakeMap(self->transitions);
    return self;
}

// 0x822E10: BlendPair(start key, target key)
BlendPair* __fastcall BlendPair_ConstructImpl(BlendPair* self, re::Edx, const re::Ptr32<char>* startKey, const re::Ptr32<char>* targetKey) {
    self->startKey = kNullFixedString;
    self->targetKey = kNullFixedString;
    FixedString_Assign(&self->startKey, startKey);
    FixedString_Assign(&self->targetKey, targetKey);
    return self;
}

// 0x822E50: AddBlendPair(source, destination, start key, target key): onto a blend transition
uint32_t __fastcall KFMTool_AddBlendPairImpl(NiKFMTool* self, re::Edx, uint32_t srcID, uint32_t desID,
                                             const re::Ptr32<char>* startKey, const re::Ptr32<char>* targetKey) {
    MapNode* node = FindNode(self->sequences, srcID);
    if (node == nullptr || node->value == nullptr) {
        return 2;
    }
    Transition* transition = KFMTool_GetTransitionFromSequence(self, desID, static_cast<Sequence*>(node->value));
    if (transition == nullptr) {
        return 3;
    }
    if (transition->type != 0) {
        return 4;
    }
    if (*startKey == kNullFixedString && *targetKey == kNullFixedString) {
        return 6;
    }
    void* memory = NiNew(8);
    BlendPair* pair = memory != nullptr ? BlendPair_Construct(memory, startKey, targetKey) : nullptr;
    PtrSet_AddCall(PairSet(transition), &pair);
    return 0;
}

// 0x822F20: AddSequenceToGroup(group, sequence, priority, weight, ease in, ease out, synchronize)
uint32_t __fastcall KFMTool_AddSequenceToGroupImpl(NiKFMTool* self, re::Edx, uint32_t groupID, uint32_t sequenceID,
                                                   int32_t priority, float weight, float easeIn, float easeOut,
                                                   uint32_t syncID) {
    MapNode* node = FindNode(self->sequenceGroups, groupID);
    if (node == nullptr || node->value == nullptr) {
        return 0xB;
    }
    SequenceInfo info;
    info.sequenceID = sequenceID;
    info.priority = priority;
    info.weight = weight;
    info.easeInTime = easeIn;
    info.easeOutTime = easeOut;
    info.synchronizeSequenceID = syncID;
    SequenceInfoSet_Add(InfoSet(static_cast<SequenceGroup*>(node->value)), &info);
    return 0;
}

// 0x822FB0: a chain transition's sequences into the set (the source, each step's (a step that is
// itself a chain from the one before expanded), the destination); stopped past 1000
void __fastcall KFMTool_FillChainSequenceIDsImpl(NiKFMTool* self, re::Edx, uint32_t srcID, uint32_t desID,
                                                 Transition* transition, NiTSet<uint32_t>* ids) {
    if (transition->type != 3 || ids->count > 1000) {
        return;
    }
    uint32_t steps = transition->chainCount;
    if (steps == 0) {
        return;
    }
    PtrSet_AddCall(ids, &srcID);
    uint32_t previous = srcID;
    for (uint32_t i = 0; i < steps; i++) {
        uint32_t id = transition->chain[i].sequenceID;
        Transition* step = KFMTool_GetTransition(self, previous, id);
        if (step != nullptr && step->type == 3) {
            KFMTool_FillChainSequenceIDs(self, previous, id, step, ids);
        } else {
            AddInline(ids, id);
        }
        previous = id;
    }
    AddInline(ids, desID);
}

// 0x8230A0: after an ASCII load, a chain step to its sequence's end (-1) whose next transition is
// a blend with one pair starting at "end" made a step of no time
void __fastcall KFMTool_FixChainDurationsImpl(NiKFMTool* self, re::Edx) {
    re::Ptr32<MapNode> pos = MapFirst(self->sequences);
    while (pos != nullptr) {
        uint32_t key;
        re::Ptr32<void> value;
        Map_GetNext(&self->sequences, &pos, &key, &value);
        auto* sequence = static_cast<Sequence*>(value);
        MapNode* node = MapFirst(sequence->transitions);
        while (node != nullptr) {
            uint32_t desID = node->key;
            auto* transition = static_cast<Transition*>(node->value);
            node = MapNext(sequence->transitions, node);
            if (transition->type != 3) {
                continue;
            }
            for (uint32_t i = 0; i < transition->chainCount; i++) {
                ChainInfo& step = transition->chain[i];
                if (!(step.duration == -1.0f)) {
                    continue;
                }
                uint32_t nextID = i + 1 < transition->chainCount ? transition->chain[i + 1].sequenceID : desID;
                MapNode* stepNode = FindNode(self->sequences, step.sequenceID);
                if (stepNode == nullptr || stepNode->value == nullptr) {
                    continue;
                }
                MapNode* nextNode = FindNode(static_cast<Sequence*>(stepNode->value)->transitions, nextID);
                if (nextNode == nullptr) {
                    continue;
                }
                auto* next = static_cast<Transition*>(nextNode->value);
                if (next == nullptr || next->type != 0 || next->blendPairCount != 1) {
                    continue;
                }
                const char* start = next->blendPairs[0]->startKey;
                if (start != nullptr && crt::Strcmp(start, "end") == 0) {
                    step.duration = 0.0f;
                }
            }
        }
    }
}

// 0x823280: AddSequenceGroup(group, name): one replacing a group of that ID (deleted)
uint32_t __fastcall KFMTool_AddSequenceGroupImpl(NiKFMTool* self, re::Edx, uint32_t groupID, const re::Ptr32<char>* name) {
    auto* group = static_cast<SequenceGroup*>(NiNew(0x14));
    if (group != nullptr) {
        group->groupID = groupID;
        group->name = kNullFixedString;
        group->sequences = nullptr;
        group->count = 0;
        group->unknown10 = 0;
        FixedString_Assign(&group->name, name);
    } else {
        group = nullptr;
    }
    MapNode* node = FindNode(self->sequenceGroups, groupID);
    if (node != nullptr && node->value != nullptr) {
        auto* old = static_cast<SequenceGroup*>(node->value);
        NiDeleteArray(old->sequences, 0x18);
        FixedStringRelease(&old->name);
        NiDelete(old, 0x14);
    }
    Map_SetAt(&self->sequenceGroups, groupID, group);
    return 0;
}

// 0x823360: IsValidChainTransition(source, destination, transition): its sequences at most 1000
uint8_t __fastcall KFMTool_IsValidChainTransitionImpl(NiKFMTool* self, re::Edx, uint32_t srcID, uint32_t desID,
                                                      Transition* transition) {
    NiTSet<uint32_t> ids = {nullptr, 0, 0};
    KFMTool_FillChainSequenceIDs(self, srcID, desID, transition, &ids);
    if (ids.count > 1000) {
        NiFree(ids.base);
        return 0;
    }
    NiFree(ids.base);
    return 1;
}

// the KF path prefix applied to a name (strcpy_s / strcat_s into a 0x104 buffer)
inline char* AddPrefixed(const char* prefix, const char* name) {
    char buffer[0x104];
    crt::StrcpyS(buffer, 0x104, prefix);
    crt::StrcatS(buffer, 0x104, name);
    return StringTable_AddString(buffer);
}

// 0x8233C0: ReadBinary(stream, version): the default paths (before 1.2.3.0), the model's path
// (standardized; before 2.2.0.0 its leading backslashes dropped; the NIF path before it) and
// root, the default transitions' types and durations (from 1.2.2.0), the sequences with their
// transitions (blend pairs, chains: before 1.2.4.0 a chain's first step dropped), the sequence
// groups. (A failed allocation is written through all the same, as the game has it)
uint32_t __fastcall KFMTool_ReadBinaryImpl(NiKFMTool* self, re::Edx, BinaryStream* stream, uint32_t version) {
    re::Ptr32<char> nifPath = nullptr;
    re::Ptr32<char> kfPath = nullptr;
    if (version < 0x01020300) {
        uint8_t hasPaths;
        Read(stream, &hasPaths, 1, 1);
        if (hasPaths != 0) {
            KFMTool_LoadCString(self, stream, &nifPath);
            KFMTool_LoadCString(self, stream, &kfPath);
        }
    }
    KFMTool_LoadFixedString(self, stream, &self->modelPath);
    SourceTexture_StandardizeFilename(&self->modelPath);
    if (version < 0x02020000) {
        KFMTool_StripLeadingBackslashes(&self->modelPath);
    }
    if (nifPath != nullptr) {
        re::Ptr32<char> full = AddPrefixed(nifPath, self->modelPath);
        FixedString_Assign(&self->modelPath, &full);
        FixedStringRelease(&full);
    }
    KFMTool_LoadFixedString(self, stream, &self->modelRoot);
    if (version >= 0x01020200) {
        uint32_t type;
        Read4(stream, &type);
        self->defaultSyncTransition->type = type;
        Read4(stream, &type);
        self->defaultNonSyncTransition->type = type;
        Read4(stream, &self->defaultSyncTransition->duration);
        Read4(stream, &self->defaultNonSyncTransition->duration);
    }
    uint32_t numSequences;
    Read4(stream, &numSequences);
    for (uint32_t s = 0; s < numSequences; s++) {
        auto* sequence = static_cast<Sequence*>(NiNew(0x1C));
        if (sequence != nullptr) {
            sequence->sequenceID = 0;
            sequence->filename = kNullFixedString;
            sequence->animIndex = 0;
            MakeMap(sequence->transitions);
        } else {
            sequence = nullptr;
        }
        Read4(stream, &sequence->sequenceID);
        if (version < 0x01020500) {
            re::Ptr32<char> oldName = nullptr;
            KFMTool_LoadCString(self, stream, &oldName);
            NiFree(oldName);
        }
        KFMTool_LoadFixedString(self, stream, &sequence->filename);
        char buffer[0x104];
        crt::StrcpyS(buffer, 0x104, sequence->filename);
        NiPath_Standardize(buffer);
        if (sequence->filename != buffer) {
            re::Ptr32<char> old = sequence->filename;
            sequence->filename = StringTable_AddString(buffer);
            FixedStringRelease(&old);
        }
        if (version < 0x02020000) {
            KFMTool_StripLeadingBackslashes(&sequence->filename);
        }
        if (kfPath != nullptr) {
            re::Ptr32<char> full = AddPrefixed(kfPath, sequence->filename);
            AssignHandle(&sequence->filename, full);
            FixedStringRelease(&full);
        }
        Read4(stream, &sequence->animIndex);
        uint32_t numTransitions;
        Read4(stream, &numTransitions);
        for (uint32_t t = 0; t < numTransitions; t++) {
            uint32_t desID;
            Read4(stream, &desID);
            uint32_t type;
            Read4(stream, &type);
            Transition* transition;
            if (type == 4) {
                transition = self->defaultSyncTransition;
            } else if (type == 5) {
                transition = self->defaultNonSyncTransition;
            } else {
                transition = static_cast<Transition*>(NiNew(0x24));
                if (transition != nullptr) {
                    transition->duration = 0.0f;
                    transition->type = 0;
                    transition->blendPairs = nullptr;
                    transition->blendAllocated = 0;
                    transition->blendPairCount = 0;
                    transition->chain = nullptr;
                    transition->chainAllocated = 0;
                    transition->chainCount = 0;
                    transition->storedType = 6;
                } else {
                    transition = nullptr;
                }
                transition->type = type;
                Read4(stream, &transition->duration);
                uint32_t numPairs;
                Read4(stream, &numPairs);
                for (uint32_t p = 0; p < numPairs; p++) {
                    auto* pair = static_cast<BlendPair*>(NiNew(8));
                    if (pair != nullptr) {
                        pair->startKey = kNullFixedString;
                        pair->targetKey = kNullFixedString;
                    } else {
                        pair = nullptr;
                    }
                    KFMTool_LoadFixedString(self, stream, &pair->startKey);
                    KFMTool_LoadFixedString(self, stream, &pair->targetKey);
                    AddInline(PairSet(transition), pair);
                }
                uint32_t numSteps;
                Read4(stream, &numSteps);
                if (version < 0x01020400 && numSteps != 0) {
                    ChainInfo dropped = {0, 0.0f};
                    Read4(stream, &dropped.sequenceID);
                    Read4(stream, &dropped.duration);
                    numSteps--;
                }
                for (uint32_t c = 0; c < numSteps; c++) {
                    ChainInfo step = {0, 0.0f};
                    Read4(stream, &step.sequenceID);
                    Read4(stream, &step.duration);
                    NiTSet<ChainInfo>* chain = ChainSet(transition);
                    if (chain->count == chain->allocated) {
                        uint32_t allocated = chain->allocated != 0 ? chain->allocated * 2 : 1;
                        if (allocated != chain->allocated) {
                            ChainInfo* base = nullptr;
                            if (allocated != 0) {
                                uint64_t product = uint64_t(allocated) * 8;
                                base = static_cast<ChainInfo*>(
                                    NiNewArray((product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product)));
                                if (base != nullptr) {
                                    for (uint32_t k = 0; k < allocated; k++) {
                                        base[k].duration = 0.0f;
                                        base[k].sequenceID = 0;
                                    }
                                }
                                for (uint32_t k = 0; k < chain->count; k++) {
                                    base[k] = chain->base[k];
                                }
                            }
                            NiDeleteArray(chain->base, 8);
                            chain->base = base;
                            chain->allocated = allocated;
                        }
                    }
                    chain->base[chain->count] = step;
                    chain->count++;
                }
            }
            Map_SetAt(&sequence->transitions, desID, transition);
        }
        Map_SetAt(&self->sequences, sequence->sequenceID, sequence);
    }
    uint32_t numGroups;
    Read4(stream, &numGroups);
    for (uint32_t g = 0; g < numGroups; g++) {
        auto* group = static_cast<SequenceGroup*>(NiNew(0x14));
        if (group != nullptr) {
            group->groupID = 0;
            group->name = kNullFixedString;
            group->sequences = nullptr;
            group->count = 0;
            group->unknown10 = 0;
        } else {
            group = nullptr;
        }
        Read4(stream, &group->groupID);
        KFMTool_LoadFixedString(self, stream, &group->name);
        uint32_t numInfos;
        Read4(stream, &numInfos);
        for (uint32_t i = 0; i < numInfos; i++) {
            SequenceInfo info;
            info.weight = 0.0f;
            info.easeInTime = 0.0f;
            info.easeOutTime = 0.0f;
            info.sequenceID = 0;
            info.priority = 0;
            info.synchronizeSequenceID = kNoSyncSequence;
            Read4(stream, &info.sequenceID);
            Read4(stream, &info.priority);
            Read4(stream, &info.weight);
            Read4(stream, &info.easeInTime);
            Read4(stream, &info.easeOutTime);
            if (version >= 0x01020100) {
                Read4(stream, &info.synchronizeSequenceID);
            } else {
                info.synchronizeSequenceID = kNoSyncSequence;
            }
            NiTSet<SequenceInfo>* infos = InfoSet(group);
            if (infos->count == infos->allocated) {
                SequenceInfoSet_Realloc(infos, infos->allocated != 0 ? infos->allocated * 2 : 1);
            }
            infos->base[infos->count] = info;
            infos->count++;
        }
        Map_SetAt(&self->sequenceGroups, group->groupID, group);
    }
    NiFree(nifPath);
    NiFree(kfPath);
    return 0;
}

// 0x823E40: AddSequenceToChain(source, destination, sequence, duration): onto a chain
// transition; a step making the chain invalid taken off again (the set's RemoveAt(count): the
// last copied one past the end, as the game has it)
uint32_t __fastcall KFMTool_AddSequenceToChainImpl(NiKFMTool* self, re::Edx, uint32_t srcID, uint32_t desID,
                                                   uint32_t sequenceID, float duration) {
    MapNode* node = FindNode(self->sequences, srcID);
    if (node == nullptr || node->value == nullptr) {
        return 2;
    }
    Transition* transition = KFMTool_GetTransitionFromSequence(self, desID, static_cast<Sequence*>(node->value));
    if (transition == nullptr) {
        return 3;
    }
    if (transition->type != 3) {
        return 4;
    }
    ChainInfo step;
    step.sequenceID = sequenceID;
    step.duration = duration;
    ChainSet_Add(ChainSet(transition), &step);
    if (KFMTool_IsValidChainTransition(self, srcID, desID, transition)) {
        return 0;
    }
    uint32_t index = transition->chainCount;
    transition->chainCount--;
    transition->chain[index] = transition->chain[transition->chainCount];
    return 10;
}

// 0x823F30: SetTransition(source, destination, type, duration): the defaults for types 4 and 5,
// else a new transition; one it replaces deleted (but the defaults)
uint32_t __fastcall KFMTool_SetTransitionImpl(NiKFMTool* self, re::Edx, uint32_t srcID, uint32_t desID, uint32_t type,
                                              float duration) {
    MapNode* node = FindNode(self->sequences, srcID);
    if (node == nullptr || node->value == nullptr) {
        return 2;
    }
    auto* sequence = static_cast<Sequence*>(node->value);
    Transition* transition;
    if (type == 4) {
        transition = self->defaultSyncTransition;
    } else if (type == 5) {
        transition = self->defaultNonSyncTransition;
    } else {
        void* memory = NiNew(0x24);
        transition = memory != nullptr ? TransitionConstruct(memory, type, duration) : nullptr;
    }
    MapNode* existing = FindNode(sequence->transitions, desID);
    if (existing != nullptr) {
        auto* old = static_cast<Transition*>(existing->value);
        if (old != self->defaultSyncTransition && old != self->defaultNonSyncTransition && old != nullptr) {
            Transition_ClearBlendPairs(old);
            NiDeleteArray(old->chain, 8);
            NiFree(old->blendPairs);
            NiDelete(old, 0x24);
        }
    }
    Map_SetAt(&sequence->transitions, desID, transition);
    return 0;
}

// 0x824040: AddSequence(id, filename, anim index): one replacing a sequence of that ID (deleted)
uint32_t __fastcall KFMTool_AddSequenceImpl(NiKFMTool* self, re::Edx, uint32_t id, const re::Ptr32<char>* filename,
                                            int32_t animIndex) {
    void* memory = NiNew(0x1C);
    Sequence* sequence = memory != nullptr ? Sequence_Construct(memory, id, filename, animIndex) : nullptr;
    MapNode* node = FindNode(self->sequences, id);
    if (node != nullptr && node->value != nullptr) {
        auto* old = static_cast<Sequence*>(node->value);
        Sequence_Destruct(old);
        NiDelete(old, 0x1C);
    }
    Map_SetAt(&self->sequences, id, sequence);
    return 0;
}

inline const char* NullOr(const char* token) {
    return crt::Strcmp(token, "NULL") == 0 ? nullptr : token;
}

// a copy NiMalloc'd (NiStrcpy)
inline char* CopyText(const char* text) {
    uint32_t size = uint32_t(std::strlen(text)) + 1;
    char* copy = static_cast<char*>(NiMalloc(size));
    NiStrcpy(copy, size, text);
    return copy;
}

// 0x8240D0: ReadAscii(file, version): the old text format, a line at a time (";" a comment; "#"
// ends a value): DEFAULTPATHS, MODEL, ANIMATION, \tTRANSITION (after its ANIMATION: blend or
// morph, a duration, the intermediate animations of a chain, the text key pairs), LAYERGROUP,
// END_KFM_FILE. 14 at the first unexpected token (what was read so far kept, the paths not freed);
// else the chains' steps fixed up. (strtok_s is ours: its state is in the context)
uint32_t __fastcall KFMTool_ReadAsciiImpl(NiKFMTool* self, re::Edx, BinaryStream* file, uint32_t) {
    const char kDelimiters[] = "#\n";
    char line[0x100];
    char path[0x104];
    char* context = nullptr;
    char* nifPath = nullptr;
    char* kfPath = nullptr;
    uint32_t currentID = 0;
    while (BinaryStream_GetLine(file, line, 0x100) != 0) {
        char* token = strtok_s(line, kDelimiters, &context);
        if (token == nullptr || *token == ';') {
            continue;
        }
        if (crt::Strcmp(token, "END_KFM_FILE") == 0) {
            break;
        }
        if (crt::Strcmp(token, "DEFAULTPATHS") == 0) {
            token = strtok_s(nullptr, kDelimiters, &context);
            if (token == nullptr) {
                return 0xE;
            }
            if (crt::Atol(token) != 1) {
                continue;
            }
            if (BinaryStream_GetLine(file, line, 0x100) == 0 ||
                (token = strtok_s(line, kDelimiters, &context)) == nullptr ||
                crt::Strcmp(token, "DEFAULTNIFPATH") != 0 ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            if (crt::Strcmp(token, "NULL") != 0) {
                nifPath = CopyText(token);
            }
            if (BinaryStream_GetLine(file, line, 0x100) == 0 || (token = strtok_s(line, "#", &context)) == nullptr ||
                crt::Strcmp(token, "DEFAULTKFPATH") != 0 ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            if (crt::Strcmp(token, "NULL") != 0) {
                kfPath = CopyText(token);
            }
            continue;
        }
        if (crt::Strcmp(token, "MODEL") == 0) {
            token = strtok_s(nullptr, kDelimiters, &context);
            if (token == nullptr) {
                return 0xE;
            }
            const char* model = NullOr(token);
            re::Ptr32<char> name;
            if (nifPath != nullptr) {
                crt::StrcpyS(path, 0x104, nifPath);
                crt::StrcatS(path, 0x104, model);
                FixedString_Construct(&name, path);
            } else {
                FixedString_Construct(&name, model);
            }
            FixedString_Assign(&self->modelPath, &name);
            FixedString_Destruct(&name);
            token = strtok_s(nullptr, kDelimiters, &context);
            if (token == nullptr) {
                return 0xE;
            }
            re::Ptr32<char> root;
            FixedString_Construct(&root, NullOr(token));
            FixedString_Assign(&self->modelRoot, &root);
            FixedString_Destruct(&root);
            continue;
        }
        if (crt::Strcmp(token, "ANIMATION") == 0) {
            if ((token = strtok_s(nullptr, " \n", &context)) == nullptr || crt::Strcmp(token, "EVENTCODE") != 0 ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            uint32_t id = uint32_t(crt::Atol(token));
            if (strtok_s(nullptr, kDelimiters, &context) == nullptr ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            const char* filename = NullOr(token);
            if (kfPath != nullptr) {
                NiStrcpy(path, 0x104, kfPath);
                NiStrcat(path, 0x104, filename);
            } else {
                NiStrcpy(path, 0x104, filename);
            }
            if ((token = strtok_s(nullptr, " \n", &context)) == nullptr || crt::Strcmp(token, "INDEX") != 0 ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            int32_t index = int32_t(crt::Atol(token));
            re::Ptr32<char> name;
            FixedString_Construct(&name, path);
            KFMTool_AddSequence(self, id, &name, index);
            FixedString_Destruct(&name);
            currentID = id;
            continue;
        }
        if (crt::Strcmp(token, "\tTRANSITION") == 0) {
            if ((token = strtok_s(nullptr, " \n", &context)) == nullptr || crt::Strcmp(token, "EVENTCODE") != 0 ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            uint32_t desID = uint32_t(crt::Atol(token));
            if ((token = strtok_s(nullptr, " \n", &context)) == nullptr || crt::Strcmp(token, "TYPE") != 0 ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            bool blend;
            if (crt::Strcmp(token, "BLEND") == 0) {
                blend = true;
            } else if (crt::Strcmp(token, "MORPH") == 0) {
                blend = false;
            } else {
                return 0xE;
            }
            if ((token = strtok_s(nullptr, " \n", &context)) == nullptr || crt::Strcmp(token, "DURATION") != 0 ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            float duration = float(crt::Atof(token));
            KFMTool_SetTransition(self, currentID, desID, blend ? 0 : 1, duration);
            if ((token = strtok_s(nullptr, " \n", &context)) == nullptr ||
                crt::Strcmp(token, "INTERMEDIATEANIMS") != 0 ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            uint32_t numSteps = uint32_t(crt::Atol(token));
            if (numSteps != 0) {
                KFMTool_GetTransition(self, currentID, desID)->type = 3;
            }
            for (uint32_t i = 0; i < numSteps; i++) {
                token = strtok_s(nullptr, kDelimiters, &context);
                if (token == nullptr) {
                    return 0xE;
                }
                KFMTool_AddSequenceToChain(self, currentID, desID, uint32_t(crt::Atol(token)), -1.0f);
            }
            if ((token = strtok_s(nullptr, " \n", &context)) == nullptr ||
                crt::Strcmp(token, "TEXTKEYPAIRS") != 0 ||
                (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
                return 0xE;
            }
            uint32_t numPairs = uint32_t(crt::Atol(token));
            for (uint32_t i = 0; i < numPairs; i++) {
                char* start = strtok_s(nullptr, kDelimiters, &context);
                if (start == nullptr) {
                    return 0xE;
                }
                const char* startKey = NullOr(start);
                char* target = strtok_s(nullptr, kDelimiters, &context);
                if (target == nullptr) {
                    return 0xE;
                }
                const char* targetKey = NullOr(target);
                if (startKey != nullptr || targetKey != nullptr) {
                    re::Ptr32<char> targetName;
                    FixedString_Construct(&targetName, targetKey);
                    re::Ptr32<char> startName;
                    FixedString_Construct(&startName, startKey);
                    KFMTool_AddBlendPair(self, currentID, desID, &startName, &targetName);
                    FixedString_Destruct(&startName);
                    FixedString_Destruct(&targetName);
                }
            }
            continue;
        }
        if (crt::Strcmp(token, "LAYERGROUP") != 0) {
            return 0xE;
        }
        if ((token = strtok_s(nullptr, " \n", &context)) == nullptr || crt::Strcmp(token, "GROUPID") != 0 ||
            (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
            return 0xE;
        }
        uint32_t groupID = uint32_t(crt::Atol(token));
        token = strtok_s(nullptr, kDelimiters, &context);
        if (token == nullptr) {
            return 0xE;
        }
        re::Ptr32<char> groupName;
        FixedString_Construct(&groupName, NullOr(token));
        KFMTool_AddSequenceGroup(self, groupID, &groupName);
        FixedString_Destruct(&groupName);
        if ((token = strtok_s(nullptr, " \n", &context)) == nullptr || crt::Strcmp(token, "LAYERS") != 0 ||
            (token = strtok_s(nullptr, kDelimiters, &context)) == nullptr) {
            return 0xE;
        }
        uint32_t numLayers = uint32_t(crt::Atol(token));
        for (uint32_t i = 0; i < numLayers; i++) {
            token = strtok_s(nullptr, kDelimiters, &context);
            if (token == nullptr) {
                return 0xE;
            }
            uint32_t sequenceID = uint32_t(crt::Atol(token));
            token = strtok_s(nullptr, kDelimiters, &context);
            if (token == nullptr) {
                return 0xE;
            }
            int32_t priority = int32_t(crt::Atol(token));
            KFMTool_AddSequenceToGroup(self, groupID, sequenceID, priority, 1.0f, 0.0f, 0.0f, kNoSyncSequence);
        }
    }
    NiFree(nifPath);
    NiFree(kfPath);
    KFMTool_FixChainDurations(self);
    return 0;
}

// 0x824AE0: LoadFile(filename): the file opened (standardized), its header line
// ";Gamebryo KFM File Version x.x.x.x[b]" read; before 1.2.0.0 the ASCII reader, else (when not
// newer than this build's) the binary one for a "b" file after its endianness byte (from
// 1.2.6.0). The base path set from the filename's folder first
uint32_t __fastcall KFMTool_LoadFileImpl(NiKFMTool* self, re::Edx, const char* filename) {
    char path[0x104];
    crt::StrcpyS(path, 0x104, filename);
    NiPath_Standardize(path);
    BinaryStream* file = NiFile_GetFile(path, 0, 0x8000);
    if (file == nullptr) {
        return 0xD;
    }
    if (!sys::Virtual<uint8_t>(file, 1)) {   // the file opened
        sys::Virtual<void*>(file, 0, 1);
        return 0xD;
    }
    sys::Virtual<void>(file, 4, 0);   // SetEndianSwap(false)
    char line[0x100];
    BinaryStream_GetLine(file, line, 0x100);
    if (crt::Strncmp(line, ";Gamebryo KFM File Version ", 0x1B) != 0) {
        sys::Virtual<void*>(file, 0, 1);
        return 0xF;
    }
    uint32_t length = uint32_t(std::strlen(line));
    uint8_t binary = line[length - 1] == 'b' ? 1 : 0;
    uint32_t count = length - (0x1B + (binary != 0 ? 1 : 0));
    char versionText[0x10];
    crt::StrncpyS(versionText, 0x10, line + 0x1B, count);
    versionText[count] = 0;
    uint8_t ascii = 0;
    uint32_t version = NiStream_GetVersionFromString(versionText);
    if (version < 0x01020000) {
        ascii = 1;
    } else if (version > NiStream_GetVersionFromString(ms_szKFMCurrentVersion)) {
        sys::Virtual<void*>(file, 0, 1);
        return 0xF;
    }
    char* separator = crt::Strrchr(path, '\\');
    re::Ptr32<char> basePath;
    if (separator != nullptr) {
        separator[1] = 0;
        FixedString_Construct(&basePath, path);
    } else {
        basePath = kNullFixedString;
    }
    FixedString_Assign(&self->basePath, &basePath);
    FixedString_Destruct(&basePath);
    if (ascii) {
        uint32_t result = KFMTool_ReadAscii(self, file, version);
        sys::Virtual<void*>(file, 0, 1);
        return result;
    }
    if (!binary) {
        sys::Virtual<void*>(file, 0, 1);
        return 0xE;
    }
    uint8_t littleEndian = 1;
    if (version >= 0x01020600) {
        Stream_LoadUInt8(file, &littleEndian);
        if (littleEndian != 1 && NiSystemDesc_IsLittleEndian()) {
            sys::Virtual<void*>(file, 0, 1);
            return 0x10;
        }
    }
    sys::Virtual<void>(file, 4, littleEndian != 1 ? 1 : 0);   // SetEndianSwap
    uint32_t result = KFMTool_ReadBinary(self, file, version);
    sys::Virtual<void*>(file, 0, 1);
    return result;
}

}   // namespace
}   // namespace ni::actor

// ---- registration ---------------------------------------------------------------------

#define NIC32_KFM_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC32_KFM_REVERSED("NiKFMTool::SequenceGroup::SequenceInfo::SequenceInfo", 0x00822700,
                   &ni::actor::SequenceInfo_Construct);
NIC32_KFM_REVERSED("NiKFMTool::GetSequenceIDs", 0x00822730, &ni::actor::KFMTool_GetSequenceIDsImpl);
NIC32_KFM_REVERSED("NiTPrimitiveSet<T*>::Add_8227D0", 0x008227D0, &ni::actor::PtrSet_Add);
NIC32_KFM_REVERSED("NiStreamLoadBinary(NiBinaryStream,unsigned char)", 0x00822810, &ni::actor::LoadUInt8);
NIC32_KFM_REVERSED("NiStreamLoadBinary(NiBinaryStream,unsigned int)", 0x00822840, &ni::actor::LoadUInt32);
NIC32_KFM_REVERSED("NiKFMTool::SequenceGroup::SequenceInfo::NewArray", 0x00822870,
                   &ni::actor::SequenceInfo_NewArrayImpl);
NIC32_KFM_REVERSED("NiTPrimitiveSet<NiKFMTool::Transition::ChainInfo>::Realloc", 0x008228B0,
                   &ni::actor::ChainSet_ReallocImpl);
NIC32_KFM_REVERSED("NiTPrimitiveSet<NiKFMTool::SequenceGroup::SequenceInfo>::Realloc", 0x00822910,
                   &ni::actor::SequenceInfoSet_ReallocImpl);
NIC32_KFM_REVERSED("NiKFMTool::StripLeadingBackslashes", 0x00822990,
                   &ni::actor::KFMTool_StripLeadingBackslashesImpl);
NIC32_KFM_REVERSED("NiKFMTool::GetFullModelPath", 0x00822A20, &ni::actor::KFMTool_GetFullModelPathImpl);
NIC32_KFM_REVERSED("NiKFMTool::GetFullKFFilename", 0x00822AF0, &ni::actor::KFMTool_GetFullKFFilenameImpl);
NIC32_KFM_REVERSED("NiKFMTool::LoadCStringAsBinary", 0x00822BC0, &ni::actor::KFMTool_LoadCStringImpl);
NIC32_KFM_REVERSED("NiKFMTool::LoadFixedStringAsBinary", 0x00822C40, &ni::actor::KFMTool_LoadFixedStringImpl);
NIC32_KFM_REVERSED("NiTPrimitiveSet<NiKFMTool::Transition::ChainInfo>::Add", 0x00822CF0,
                   &ni::actor::ChainSet_AddImpl);
NIC32_KFM_REVERSED("NiTPrimitiveSet<NiKFMTool::SequenceGroup::SequenceInfo>::Add", 0x00822D30,
                   &ni::actor::SequenceInfoSet_AddImpl);
NIC32_KFM_REVERSED("NiKFMTool::Sequence::Sequence", 0x00822D90, &ni::actor::Sequence_ConstructImpl);
NIC32_KFM_REVERSED("NiKFMTool::Transition::BlendPair::BlendPair", 0x00822E10, &ni::actor::BlendPair_ConstructImpl);
NIC32_KFM_REVERSED("NiKFMTool::AddBlendPair", 0x00822E50, &ni::actor::KFMTool_AddBlendPairImpl);
NIC32_KFM_REVERSED("NiKFMTool::AddSequenceToGroup", 0x00822F20, &ni::actor::KFMTool_AddSequenceToGroupImpl);
NIC32_KFM_REVERSED("NiKFMTool::FillChainSequenceIDs", 0x00822FB0, &ni::actor::KFMTool_FillChainSequenceIDsImpl);
NIC32_KFM_REVERSED("NiKFMTool::FixChainDurations", 0x008230A0, &ni::actor::KFMTool_FixChainDurationsImpl);
NIC32_KFM_REVERSED("NiKFMTool::AddSequenceGroup", 0x00823280, &ni::actor::KFMTool_AddSequenceGroupImpl);
NIC32_KFM_REVERSED("NiKFMTool::IsValidChainTransition", 0x00823360,
                   &ni::actor::KFMTool_IsValidChainTransitionImpl);
NIC32_KFM_REVERSED("NiKFMTool::ReadBinary", 0x008233C0, &ni::actor::KFMTool_ReadBinaryImpl);
NIC32_KFM_REVERSED("NiKFMTool::AddSequenceToChain", 0x00823E40, &ni::actor::KFMTool_AddSequenceToChainImpl);
NIC32_KFM_REVERSED("NiKFMTool::SetTransition", 0x00823F30, &ni::actor::KFMTool_SetTransitionImpl);
NIC32_KFM_REVERSED("NiKFMTool::AddSequence", 0x00824040, &ni::actor::KFMTool_AddSequenceImpl);
NIC32_KFM_REVERSED("NiKFMTool::ReadAscii", 0x008240D0, &ni::actor::KFMTool_ReadAsciiImpl);
NIC32_KFM_REVERSED("NiKFMTool::LoadFile", 0x00824AE0, &ni::actor::KFMTool_LoadFileImpl);
