// Track / TrackOrCondition class ids, and the mission-data track (vtable 0x9063D8, a
// Track: 0x906484): its destructor, class id and copy constructor.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

GAME_VAR(int16_t, mt_nextTypeId, 0x00BF3830);
GAME_FN(void, __thiscall, String_Destruct, 0x0049A650, void* self);   // (empty)
GAME_FN(void, __thiscall, Track_Destruct, 0x0061A8B0, void* self);
GAME_FN(uint8_t, __cdecl, BaseTrack_IsA, 0x004437B0, int id);

int16_t ClassId(uintptr_t guardAddress, uintptr_t idAddress) {
    uint32_t& guard = *reinterpret_cast<uint32_t*>(guardAddress);
    int16_t& id = *reinterpret_cast<int16_t*>(idAddress);
    if (!(guard & 1)) {
        guard |= 1;
        id = mt_nextTypeId;
        mt_nextTypeId = int16_t(mt_nextTypeId + 1);
    }
    return id;
}
int16_t TrackOrConditionId() { return ClassId(0x00C0F5B4, 0x00C0F5B0); }
int16_t TrackId() { return ClassId(0x00C0F5BC, 0x00C0F5B8); }
int16_t MissionDataId() { return ClassId(0x00C0F7A4, 0x00C0F7A0); }

// 0x44E320 (thiscall, vf0)
uint8_t __fastcall TrackOrCondition_IsA(void*, re::Edx, int id) {
    return int16_t(id) == TrackOrConditionId();
}

// 0x44EA60 (thiscall, vf0)
uint8_t __fastcall Track_IsA(void*, re::Edx, int id) {
    if (int16_t(id) == TrackId()) {
        return 1;
    }
    return int16_t(id) == TrackOrConditionId();
}

// 0x44E110 (thiscall, vf0)
uint8_t __fastcall MissionData_IsA(void*, re::Edx, int id) {
    if (int16_t(id) == MissionDataId()) {
        return 1;
    }
    return BaseTrack_IsA(id);
}

// 0x44E0D0 (thiscall): the destructor (its four strings, then Track's)
void __fastcall MissionData_Destruct(void* self) {
    uint8_t* p = static_cast<uint8_t*>(self);
    At<uint32_t>(self, 0) = 0x009063D8;
    String_Destruct(p + 0x44);
    String_Destruct(p + 0x20);
    String_Destruct(p + 0x1C);
    String_Destruct(p + 0x18);
    Track_Destruct(self);
}

// 0x44F390 (thiscall): the copy constructor (through its bases' vtables; a null source
// is read at 4 all the same)
void* __fastcall MissionData_Copy(void* self, re::Edx, uint8_t* from) {
    At<uint32_t>(self, 0) = 0x00906418;
    int32_t value4 = *reinterpret_cast<volatile int32_t*>(from ? from + 4 : nullptr);
    At<uint32_t>(self, 0) = 0x00906484;
    At<int32_t>(self, 4) = value4;
    At<uint8_t>(self, 8) = At<uint8_t>(from, 8);
    At<float>(self, 0xC) = At<float>(from, 0xC);
    At<float>(self, 0x10) = At<float>(from, 0x10);
    At<int32_t>(self, 0x14) = At<int32_t>(from, 0x14);
    At<uint32_t>(self, 0) = 0x009063D8;
    At<int32_t>(self, 0x18) = At<int32_t>(from, 0x18);
    At<int32_t>(self, 0x1C) = At<int32_t>(from, 0x1C);
    At<int32_t>(self, 0x20) = At<int32_t>(from, 0x20);
    At<uint8_t>(self, 0x24) = At<uint8_t>(from, 0x24);
    At<uint8_t>(self, 0x25) = At<uint8_t>(from, 0x25);
    At<uint8_t>(self, 0x26) = At<uint8_t>(from, 0x26);
    At<int32_t>(self, 0x28) = At<int32_t>(from, 0x28);
    for (int i = 0x2C; i <= 0x36; i++) {
        At<uint8_t>(self, i) = At<uint8_t>(from, i);
    }
    At<int32_t>(self, 0x38) = At<int32_t>(from, 0x38);
    At<uint8_t>(self, 0x3C) = At<uint8_t>(from, 0x3C);
    At<int32_t>(self, 0x40) = At<int32_t>(from, 0x40);
    At<int32_t>(self, 0x44) = At<int32_t>(from, 0x44);
    At<uint8_t>(self, 0x48) = At<uint8_t>(from, 0x48);
    At<uint8_t>(self, 0x49) = At<uint8_t>(from, 0x49);
    At<uint8_t>(self, 0x4A) = At<uint8_t>(from, 0x4A);
    return self;
}

#define C04A_MIS(name, address, fn)                                                            \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04A_MIS("CMissionData::Destruct", 0x0044E0D0, &MissionData_Destruct);
C04A_MIS("CMissionData::IsA", 0x0044E110, &MissionData_IsA);
C04A_MIS("TrackOrCondition::IsA", 0x0044E320, &TrackOrCondition_IsA);
C04A_MIS("Track::IsA", 0x0044EA60, &Track_IsA);
C04A_MIS("CMissionData::Copy", 0x0044F390, &MissionData_Copy);

} // namespace
