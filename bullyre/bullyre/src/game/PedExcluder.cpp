// The ped excluder (data/excluder.dat): trigger zones peds of a kind (female, male, a
// +0x1570 state, ...) are kept out of. Each line pairs a kind with a zone name; zones
// register into the kinds' buckets by their name's hash.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using col::CVector;

struct ExcluderEntry {
    uint32_t zoneHash;  // +0
    int32_t kind;       // +4 the bucket (PedEnums::PointUserFromName)
};

struct ExcluderBucket {
    re::Ptr32<uint8_t> zones[32];  // +0 triggers
    uint32_t count;      // +0x80
};
static_assert(sizeof(ExcluderBucket) == 0x84);

struct PedExcluder {
    re::Ptr32<ExcluderEntry> entries;  // +0
    uint32_t capacity;       // +4
    uint32_t count;          // +8
    ExcluderBucket buckets[8];  // +0xC

    void FreeEntries();
    void AddZone(uint8_t* zone);
    void RemoveZone(uint8_t* zone);
    void Load();
    uint8_t IsExcluded(CEntity* ped, const CVector* pos, int queryOnly);
    uint8_t IsInSocialZone(CEntity* ped, const CVector* pos);
};
static_assert(offsetof(PedExcluder, buckets) == 0xC);

GAME_ARRAY(char, s_fileBuffer, 0x00BD7800);   // 0x1C000
inline const char* const s_fileName = reinterpret_cast<const char*>(0x0090C974);  // "data/excluder.dat"
inline const char* const s_readMode = reinterpret_cast<const char*>(0x008FF548);  // "r"
GAME_VAR(uint32_t, s_delimiters0, 0x0090C96C);  // " ,\t\r"
GAME_VAR(uint16_t, s_delimiters1, 0x0090C970);  // "\n"

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(int, __cdecl, LoadFile, 0x0042D180, const char* path, char* buffer, int size, const char* mode);
inline char* __cdecl Strtok(char* s, const char* delimiters) { return crt::Strtok(s, delimiters); }
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(int, __cdecl, PointUserFromName, 0x00488C30, const char* name);
GAME_FN(uint8_t, __thiscall, Trigger_Contains, 0x006D6FA0, uint8_t* trigger, const CVector* pos);
GAME_FN(CEntity*, __thiscall, Ped_GetTarget, 0x00497100, CEntity* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, CEntity* ped);
GAME_FN(void*, __thiscall, ObjectiveQueue_GetCurrent, 0x00470340, void* queue);
GAME_FN(uint8_t, __thiscall, ObjectiveQueue_RemoveFirst, 0x004702C0, void* queue);
GAME_FN(void, __thiscall, PedSocial_Fn48F380, 0x0048F380, void* social);
GAME_FN(void, __thiscall, PedSocial_Stop, 0x004910A0, void* social);
GAME_FN(void, __thiscall, Ped_SetAttitude, 0x004731D0, CEntity* ped, int pedType, int attitude);
GAME_FN(uint8_t, __stdcall, PedIsOfKind_, 0x0046EDD0, CEntity* ped, int kind);

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

// 0x46ECE0 (thiscall on a ped): its target, if a ped
CEntity* __fastcall GetTargetPed(CEntity* self, re::Edx) {
    CEntity* target = Ped_GetTarget(self);
    if (!target) {
        return nullptr;
    }
    return target->GetType() == ENTITY_TYPE_PED ? target : nullptr;
}

// 0x46ED00 (thiscall)
void PedExcluder::FreeEntries() {
    if (entries) {
        Free(entries);
        entries = nullptr;
    }
    count = 0;
    capacity = 0;
}

// 0x46ED20 (thiscall): a zone (+0x2C its name's hash) into the buckets of its entries
void PedExcluder::AddZone(uint8_t* zone) {
    for (uint32_t i = 0; i < count; i++) {
        if (entries[i].zoneHash == Field<uint32_t>(zone, 0x2C)) {
            ExcluderBucket& bucket = buckets[entries[i].kind];
            bucket.zones[bucket.count] = zone;
            bucket.count++;
        }
    }
}

// 0x46ED70 (thiscall): the zone out of every bucket (the last moved into its place)
void PedExcluder::RemoveZone(uint8_t* zone) {
    for (ExcluderBucket& bucket : buckets) {
        for (uint32_t i = 0; i < bucket.count; i++) {
            if (bucket.zones[i] == zone) {
                bucket.zones[i] = bucket.zones[bucket.count - 1];
                bucket.count--;
                break;
            }
        }
    }
}

// 0x46EDD0 (stdcall): the ped is of the kind: 0/5 female, 1/6 male, 2/3 +0x1570 1/2,
// 4 any, 7 in state 13
uint8_t __stdcall PedIsOfKind(CEntity* ped, int kind) {
    switch (kind) {
    case 0:
    case 5:
        return Ped_GetModelInfo(ped)[0x50];
    case 1:
    case 6:
        return Ped_GetModelInfo(ped)[0x50] == 0;
    case 2:
        return Field<uint8_t>(ped, 0x1570) == 1;
    case 3:
        return Field<uint8_t>(ped, 0x1570) == 2;
    case 4:
        return 1;
    case 7:
        return Field<int32_t>(ped, 0x1310) == 13;
    default:
        return 0;
    }
}

// 0x46EE70 (thiscall): the file read: after a first word, pairs of a kind and a zone name
void PedExcluder::Load() {
    if (entries) {
        Free(entries);
        entries = nullptr;
    }
    capacity = 0;
    count = 0;
    LoadFile(s_fileName, s_fileBuffer, 0x1C000, s_readMode);
    char delimiters[8];
    *reinterpret_cast<uint32_t*>(delimiters) = s_delimiters0;
    *reinterpret_cast<uint16_t*>(delimiters + 4) = s_delimiters1;
    Strtok(s_fileBuffer, delimiters);
    char* kindName = Strtok(nullptr, delimiters);
    while (kindName) {
        char* zoneName = Strtok(nullptr, delimiters);
        if (!zoneName) {
            break;
        }
        uint32_t hash = HashName(zoneName);
        int kind = PointUserFromName(kindName);
        if (count == capacity) {
            capacity = capacity ? capacity * 2 : 1;
            ExcluderEntry* grown = nullptr;
            if (capacity) {
                uint64_t bytes = uint64_t(capacity) * 8;
                grown = static_cast<ExcluderEntry*>(Malloc(bytes > 0xFFFFFFFFu ? size_t(-1) : size_t(bytes)));
            }
            if (entries) {
                for (uint32_t i = 0; i < count; i++) {
                    grown[i] = entries[i];
                }
                Free(entries);
            }
            entries = grown;
        }
        entries[count].zoneHash = hash;
        entries[count].kind = kind;
        count++;
        kindName = Strtok(nullptr, delimiters);
    }
}

// 0x46EFB0 (thiscall): the position is in a zone of one of the ped's kinds. (Unless
// queryOnly: a ped chasing a ped into or out of the zone gives up the chase and its
// social state; kinds 5-7 set +0x14AC)
uint8_t PedExcluder::IsExcluded(CEntity* ped, const CVector* pos, int queryOnly) {
    if (!ped || Field<int32_t>(ped, 0x1498)) {
        return 0;
    }
    int kind = 0;
    uint8_t* zone = nullptr;
    for (; kind < 8; kind++) {
        if (!PedIsOfKind_(ped, kind)) {
            continue;
        }
        ExcluderBucket& bucket = buckets[kind];
        for (uint32_t i = 0; i < bucket.count; i++) {
            if (Trigger_Contains(bucket.zones[i], pos)) {
                zone = bucket.zones[i];
                break;
            }
        }
        if (zone) {
            break;
        }
    }
    if (!zone) {
        return 0;
    }
    if (re::Flag(queryOnly)) {
        return 1;
    }
    uint8_t pedInside = Trigger_Contains(zone, &ped->GetPosition());
    CEntity* target = Ped_GetTarget(ped);
    if (target && target->GetType() == ENTITY_TYPE_PED) {
        uint8_t targetInside = Trigger_Contains(zone, &target->GetPosition());
        int state = Field<int32_t>(ped, 0x1310);
        bool idle = state == 0 || state == 8 || state == 7;
        if (idle && targetInside) {
            return 0;
        }
        if (!idle) {
            if (!pedInside && !targetInside) {
                return 1;
            }
            void* queue = reinterpret_cast<uint8_t*>(ped) + 0x5F0;
            if (ObjectiveQueue_GetCurrent(queue) && Field<int32_t>(ObjectiveQueue_GetCurrent(queue), 8) == 7) {
                ObjectiveQueue_RemoveFirst(queue);
            }
            if (!(Field<uint8_t>(ped, 0xD69) & 1)) {
                Field<uint8_t>(ped, 0xD69) |= 1;
            }
            void* social = reinterpret_cast<uint8_t*>(ped) + 0xB44;
            PedSocial_Fn48F380(social);
            PedSocial_Stop(social);
            if (Field<int32_t>(target, 0x1310) == 13) {
                Ped_SetAttitude(ped, Field<int32_t>(target, 0x1310), 2);
            }
        }
    }
    if (!pedInside) {
        return 1;
    }
    if (kind == 5 || kind == 6 || kind == 7) {
        Field<int32_t>(ped, 0x14AC) = 1;
    }
    return 0;
}

// 0x46F190 (thiscall): an active ped's position in a zone of kinds 5-7
uint8_t PedExcluder::IsInSocialZone(CEntity* ped, const CVector* pos) {
    if (!ped || Field<int32_t>(ped, 0x1498)) {
        return 0;
    }
    int state = Field<int32_t>(ped, 0x1310);
    if (state == 0 || state == 8 || state == 7) {
        return 0;
    }
    for (int kind = 0; kind < 8; kind++) {
        if ((kind == 5 || kind == 6 || kind == 7) && PedIsOfKind_(ped, kind)) {
            ExcluderBucket& bucket = buckets[kind];
            for (uint32_t i = 0; i < bucket.count; i++) {
                if (Trigger_Contains(bucket.zones[i], pos)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

} // namespace

#define EXCL_C05_REVERSED(name, address, fn) REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)

REVERSED("CPed::GetTargetPed", 0x0046ECE0, RE_BODY(&GetTargetPed), nullptr, nullptr, re::Mode::Original);
EXCL_C05_REVERSED("PedExcluder::FreeEntries", 0x0046ED00, &PedExcluder::FreeEntries);
EXCL_C05_REVERSED("PedExcluder::AddZone", 0x0046ED20, &PedExcluder::AddZone);
EXCL_C05_REVERSED("PedExcluder::RemoveZone", 0x0046ED70, &PedExcluder::RemoveZone);
REVERSED("PedExcluder::PedIsOfKind", 0x0046EDD0, RE_BODY(&PedIsOfKind), nullptr, nullptr, re::Mode::Original);
EXCL_C05_REVERSED("PedExcluder::Load", 0x0046EE70, &PedExcluder::Load);
EXCL_C05_REVERSED("PedExcluder::IsExcluded", 0x0046EFB0, &PedExcluder::IsExcluded);
EXCL_C05_REVERSED("PedExcluder::IsInSocialZone", 0x0046F190, &PedExcluder::IsInSocialZone);
