// Cover points for peds (0x487E50 .. 0x488550): the cover types read from
// Config\Dat\PedCover.dat (0x48 bytes each), the cover points placed (0x80 each, up to
// 50 in a manager: +8 the points, +0xD0 their count), and the choice of one for a ped
// by distance, facing its target and the peds around.
//
// A cover type (0x48): +0 its name (0x18), +0x18..0x30 seven floats, +0x34..0x44 five
// ints (the last read over a flag byte).
// A cover point (0x80): +0 its owner, +8 its position, +0x14 a second position, +0x20
// its heading, +0x24 its type's values (as 0x487EA0 makes them), +0x6C its weight,
// +0x70 the ped in it (registered), +0x74 a timer freeing it.

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct V3 {
    float x, y, z;
};

using re::At;

GAME_FN(V3*, __cdecl, PCV_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
template <typename... A>
inline int __cdecl PCV_Sscanf(const char* text, const char* format, A... rest) { return crt::Sscanf(text, format, rest...); }
GAME_FN(void, __cdecl, PCV_Free, 0x005EEAB0, void* p);
inline int __cdecl PCV_Rand() { return crt::Rand(); }
GAME_FN(void*, __cdecl, PCV_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, PCV_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PCV_PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, PCV_FileOpen, 0x0042D260, const char* name, const char* mode, int);
GAME_FN(char*, __cdecl, PCV_FileReadLine, 0x00429C10, void* file);
GAME_FN(void, __cdecl, PCV_FileSeek, 0x0042D300, void* file, int, int);
GAME_FN(void, __cdecl, PCV_FileClose, 0x0042D450, void* file);
// the compiler's array constructor ("eh vector constructor iterator")
GAME_FN(void, __stdcall, PCV_ConstructArray, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __thiscall, PCV_RefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(void, __thiscall, PCV_TimerConstruct, 0x0045BD20, void* timer);
GAME_FN(void, __thiscall, PCV_TimerReset, 0x0045BD10, void* timer);
GAME_FN(uint8_t, __thiscall, PCV_TimerDone, 0x0045BDD0, void* timer);
GAME_FN(uint8_t*, __thiscall, PCV_PedGetTarget, 0x00497100, void* ped);
GAME_FN(V3*, __thiscall, PCV_GetPosition, 0x00415460, void* entity);
GAME_FN(float, __cdecl, PCV_HeadingBetween, 0x00433350, const V3* from, const V3* to);
GAME_FN(float, __cdecl, PCV_LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, PCV_Distance, 0x0041A950, const V3* a, const V3* b);
GAME_FN(uint8_t, __thiscall, PCV_Ped497180, 0x00497180, void* ped, void* other);
GAME_FN(uint8_t, __thiscall, PCV_Ped496F30, 0x00496F30, void* ped, void* other);
GAME_FN(uint8_t, __thiscall, PCV_Tether499840, 0x00499840, void* tether, const V3* pos, float);
// (this file's, through the game's address)
GAME_FN(void, __stdcall, PCV_SkipWord, 0x00487F00, re::Ptr32<const char>* text);
GAME_FN(float, __cdecl, PCV_Ramp, 0x00487FC0, float value, float full, float max);
GAME_FN(uint8_t*, __thiscall, PCV_PointConstruct, 0x00488010, void* point, void* owner);
GAME_FN(void, __thiscall, PCV_ParseType, 0x004880A0, void* type, const char* line);
GAME_FN(uint8_t*, __thiscall, PCV_PickWeighted, 0x004884A0, void* list);
inline void* const PCV_TypeConstruct = reinterpret_cast<void*>(0x00487EA0);
inline const char* const PCV_formatString = reinterpret_cast<const char*>(0x008FF970);   // "%s"
inline const char* const PCV_formatFloat = reinterpret_cast<const char*>(0x008FFF7C);    // "%f"
inline const char* const PCV_formatInt = reinterpret_cast<const char*>(0x008FFF78);      // "%d"
inline const char* const PCV_fileName = reinterpret_cast<const char*>(0x0090D3E4);       // "Config\Dat\PedCover.dat"
inline const char* const PCV_readMode = reinterpret_cast<const char*>(0x008FF548);       // "r"

float LengthSquared(const V3* v) {
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

// The weighted list the choice builds on the stack (0x488550): up to 50 points
struct WeightedList {
    int32_t count;       // +0
    float total;         // +4
    struct {
        float weight;
        re::Ptr32<uint8_t> point;
    } entries[50];       // +8
};
static_assert(sizeof(WeightedList) == 0x198);

} // namespace

namespace c07pcv {

// 0x487E50 (cdecl): the squared distance of a and b less r squared
float __cdecl DistanceSquaredLess(const V3* a, const V3* b, float r) {
    V3 d;
    V3* diff = PCV_VectorSub(&d, a, b);
    float lengthSq = diff->x * diff->x + diff->y * diff->y + diff->z * diff->z;
    return lengthSq - r * r;
}

// 0x487EA0: a cover type's defaults
uint8_t* __fastcall TypeConstruct(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.5f;
    At<float>(self, 0x1C) = 10.0f;
    At<uint8_t>(self, 0) = 0;
    At<int32_t>(self, 0x34) = 1;
    At<float>(self, 0x20) = 3.0f;
    At<int32_t>(self, 0x38) = 3;
    At<int32_t>(self, 0x3C) = 1;
    At<float>(self, 0x24) = 1.0f;
    At<int32_t>(self, 0x40) = 3;
    At<uint8_t>(self, 0x44) = 0;
    At<float>(self, 0x28) = 5.0f;
    At<float>(self, 0x2C) = 0.2f;
    At<float>(self, 0x30) = 0.2f;
    return self;
}

// 0x487F00 (stdcall): past the word and the spaces after it
void __stdcall SkipWord(re::Ptr32<const char>* text) {
    while (**text != 0 && **text != ' ') {
        ++*text;
    }
    while (**text != 0 && **text == ' ') {
        ++*text;
    }
}

// 0x487F50: the cover types freed
void __fastcall FreeTypes(uint8_t* self, re::Edx) {
    re::Ptr32<void>& types = At<void*>(self, 4);
    if (types) {
        PCV_Free(types);
        types = nullptr;
    }
}

// 0x487F70: the cover type by name (nullptr: none)
uint8_t* __fastcall FindType(uint8_t* self, re::Edx, const char* name) {
    int count = At<int32_t>(self, 0);
    uint8_t* type = At<uint8_t*>(self, 4);
    for (int i = 0; i < count; i++, type += 0x48) {
        const char* a = name;
        const char* b = reinterpret_cast<const char*>(type);
        while (*a != 0 && *a == *b) {
            a++;
            b++;
        }
        if (*a == 0 && *b == 0) {
            return type;
        }
    }
    return nullptr;
}

// 0x487FC0 (cdecl): max * (full - value) / full, within 0..max
float __cdecl Ramp(float value, float full, float max) {
    float t = max * (full - value) / full;
    if (t < 0.0f) {
        return 0.0f;
    }
    if (t > max) {
        return max;
    }
    return t;
}

// 0x488010: a cover point for an owner
uint8_t* __fastcall PointConstruct(uint8_t* self, re::Edx, void* owner) {
    for (uint32_t offset = 8; offset <= 0x1C; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    At<int32_t>(self, 0x58) = 1;
    At<int32_t>(self, 0x60) = 1;
    At<float>(self, 0x3C) = 0.5f;
    At<uint8_t>(self, 0x24) = 0;
    At<int32_t>(self, 0x5C) = 3;
    At<float>(self, 0x40) = 10.0f;
    At<int32_t>(self, 0x64) = 3;
    At<float>(self, 0x44) = 3.0f;
    At<uint8_t>(self, 0x68) = 0;
    At<float>(self, 0x48) = 1.0f;
    At<float>(self, 0x4C) = 5.0f;
    At<float>(self, 0x50) = 0.2f;
    At<float>(self, 0x54) = 0.2f;
    At<void*>(self, 0x70) = nullptr;
    PCV_TimerConstruct(self + 0x74);
    At<float>(self, 0x6C) = 1.0f;
    At<void*>(self, 0) = owner;
    At<int32_t>(self, 4) = 0;
    return self;
}

// 0x4880A0: a cover type from a line of the file
void __fastcall ParseType(uint8_t* self, re::Edx, re::Ptr32<const char> line) {
    PCV_Sscanf(line, PCV_formatString, self);
    for (uint32_t offset = 0x18; offset <= 0x30; offset += 4) {
        PCV_SkipWord(&line);
        PCV_Sscanf(line, PCV_formatFloat, self + offset);
    }
    for (uint32_t offset = 0x34; offset <= 0x44; offset += 4) {
        PCV_SkipWord(&line);
        PCV_Sscanf(line, PCV_formatInt, self + offset);
    }
}

// 0x488260: the cover types loaded (counted, made, read)
void __fastcall LoadTypes(uint8_t* self, re::Edx) {
    PCV_PushMemoryTag(0x32);
    void* file = PCV_FileOpen(PCV_fileName, PCV_readMode, 1);
    for (char* line = PCV_FileReadLine(file); line; line = PCV_FileReadLine(file)) {
        if (*line != 0 && *line != '#') {
            At<uint32_t>(self, 0)++;
        }
    }
    uint32_t count = At<uint32_t>(self, 0);
    uint64_t bytes = static_cast<uint64_t>(count) * 0x48;
    uint8_t* types = static_cast<uint8_t*>(PCV_Malloc((bytes >> 32) ? 0xFFFFFFFFu : static_cast<uint32_t>(bytes)));
    if (types) {
        PCV_ConstructArray(types, 0x48, count, PCV_TypeConstruct);
    } else {
        types = nullptr;
    }
    At<uint8_t*>(self, 4) = types;
    PCV_FileSeek(file, 0, 0);
    uint32_t offset = 0;
    for (char* line = PCV_FileReadLine(file); line; line = PCV_FileReadLine(file)) {
        if (*line != 0 && *line != '#') {
            PCV_ParseType(At<uint8_t*>(self, 4) + offset, line);
            offset += 0x48;
        }
    }
    PCV_FileClose(file);
    PCV_PopMemoryTag();
}

// 0x488350 (the manager): the points whose timer ran out freed
void __fastcall UpdatePoints(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xD0); i++) {
        uint8_t* point = At<uint8_t*>(self, 8 + i * 4);
        if (PCV_TimerDone(point + 0x74)) {
            PCV_RefSet(point + 0x70, nullptr);
            PCV_TimerReset(point + 0x74);
        }
    }
}

// 0x4883A0: a point added for an owner; returns it
uint8_t* __fastcall AddPoint(uint8_t* self, re::Edx, void* owner) {
    void* memory = PCV_Malloc(0x80);
    uint8_t* point = memory ? PCV_PointConstruct(memory, owner) : nullptr;
    At<uint8_t*>(self, 8 + At<int32_t>(self, 0xD0) * 4) = point;
    At<int32_t>(self, 0xD0)++;
    return point;
}

// 0x4883E0: the point is one of the manager's
uint8_t __fastcall HasPoint(uint8_t* self, re::Edx, uint8_t* point, uint32_t /*unused (pops 8)*/) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xD0); i++) {
        if (At<uint8_t*>(self, 8 + i * 4) == point) {
            return 1;
        }
    }
    return 0;
}

// 0x488420: an owner's points deleted (the last moved into their places)
void __fastcall RemovePoints(uint8_t* self, re::Edx, void* owner) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xD0); i++) {
        uint8_t* point = At<uint8_t*>(self, 8 + i * 4);
        if (At<void*>(point, 0) == owner) {
            PCV_RefSet(point + 0x70, nullptr);
            PCV_Free(point);
            At<uint8_t*>(self, 8 + i * 4) = At<uint8_t*>(self, 4 + At<int32_t>(self, 0xD0) * 4);
            At<int32_t>(self, 0xD0)--;
            i--;
        }
    }
}

// 0x4884A0 (a weighted list): one taken at random by weight (out of the list; its total
// stays), the first when the draw runs past them all
uint8_t* __fastcall PickWeighted(uint8_t* self, re::Edx) {
    WeightedList* list = reinterpret_cast<WeightedList*>(self);
    float total = list->total;
    int count = list->count;
    float draw = static_cast<float>(static_cast<double>(PCV_Rand()) / 32767.0 * (total - 0.0f) + 0.0f);
    int index = 0;
    for (int i = 0; i < count; i++) {
        draw = draw - list->entries[i].weight;
        if (draw < 0.0f || draw == 0.0f) {
            index = i;
            break;
        }
    }
    uint8_t* point = list->entries[index].point;
    list->count = count - 1;
    list->entries[index].weight = list->entries[count - 1].weight;
    list->entries[index].point = list->entries[count - 1].point;
    return point;
}

// 0x488550: a free point (or the ped's own) for the ped: scored by how near it is (up to
// 10), facing the ped's target (then within 20 of it), and away from the peds around
// that attack it or it attacks; one picked at random by score its tether allows
uint8_t* __fastcall ChoosePoint(uint8_t* self, re::Edx, uint8_t* ped) {
    WeightedList list;
    list.total = 0.0f;
    list.count = 0;
    uint8_t* target = PCV_PedGetTarget(ped);
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xD0); i++) {
        uint8_t* point = At<uint8_t*>(self, 8 + i * 4);
        void* occupant = At<void*>(point, 0x70);
        if (occupant && occupant != ped) {
            continue;
        }
        bool usable = true;
        V3* pointPos = &At<V3>(point, 8);
        V3 d;
        V3* diff = PCV_VectorSub(&d, PCV_GetPosition(ped), pointPos);
        float distance = static_cast<float>(std::sqrt(std::fabs(LengthSquared(diff))));
        if (distance > 20.0f) {
            continue;
        }
        float score = static_cast<float>((10.0 - distance) * 50.0 / 10.0);
        if (score < 0.0f) {
            score = 0.0f;
        } else if (score > 50.0f) {
            score = 50.0f;
        }
        score = static_cast<float>(score + 0.0);
        if (target) {
            float heading = PCV_HeadingBetween(&At<V3>(point, 0x14), PCV_GetPosition(target));
            float angle = std::fabs(PCV_LimitAngle(heading - At<float>(point, 0x20)));
            if (angle > 1.5707963705062866) {
                continue;
            }
            float toTarget = PCV_Distance(PCV_GetPosition(target), pointPos);
            if (20.0f < toTarget) {
                continue;
            }
            float facing = PCV_Ramp(angle, 0.785398185f, 50.0f) + score;
            score = PCV_Ramp(toTarget, 10.0f, 50.0f) + facing;
        }
        for (int j = 0; j < At<int32_t>(ped, 0x3C0); j++) {
            uint8_t* other = At<uint8_t*>(ped, 0x3C4 + j * 4);
            if (!other) {
                continue;
            }
            V3 at = At<V3>(other, 0x1E18);
            V3 e;
            if (static_cast<float>(LengthSquared(PCV_VectorSub(&e, &at, pointPos)) - 1.0) < 0.0f) {
                usable = false;
            }
            if (static_cast<float>(LengthSquared(PCV_VectorSub(&e, &at, &At<V3>(point, 0x14))) - 1.0) < 0.0f) {
                usable = false;
            }
            float t;
            if (PCV_Ped497180(ped, other)) {
                if (static_cast<float>(LengthSquared(PCV_VectorSub(&e, &at, pointPos)) - 56.25) < 0.0f) {
                    usable = false;
                }
                if (static_cast<float>(LengthSquared(PCV_VectorSub(&e, &at, &At<V3>(point, 0x14))) - 56.25) < 0.0f) {
                    usable = false;
                }
                float away = static_cast<float>(std::sqrt(std::fabs(LengthSquared(PCV_VectorSub(&e, &at, pointPos)))));
                t = static_cast<float>(away * 20.0 / 20.0);
            } else if (PCV_Ped496F30(ped, other)) {
                float away = PCV_Distance(&at, pointPos);
                t = static_cast<float>((20.0 - away) * 20.0 / 20.0);
            } else {
                continue;
            }
            if (t < 0.0f) {
                score = 0.0f + score;
            } else if (20.0f < t) {
                score = 20.0f + score;
            } else {
                score = t + score;
            }
        }
        score = At<float>(point, 0x6C) * score;
        if (usable && score > 0.0f && list.count < 50) {
            list.entries[list.count].weight = score;
            list.entries[list.count].point = point;
            list.count++;
            list.total = score + list.total;
        }
    }
    while (list.count > 0) {
        uint8_t* point = PCV_PickWeighted(&list);
        if (PCV_Tether499840(ped + 0x358, &At<V3>(point, 8), 0.0f)) {
            return point;
        }
    }
    return nullptr;
}

} // namespace c07pcv

#define PCV_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PCV_REVERSED("PedCover::DistanceSquaredLess", 0x00487E50, &c07pcv::DistanceSquaredLess);
PCV_REVERSED("CoverType::Construct", 0x00487EA0, &c07pcv::TypeConstruct);
PCV_REVERSED("PedCover::SkipWord", 0x00487F00, &c07pcv::SkipWord);
PCV_REVERSED("CoverTypes::Free", 0x00487F50, &c07pcv::FreeTypes);
PCV_REVERSED("CoverTypes::Find", 0x00487F70, &c07pcv::FindType);
PCV_REVERSED("PedCover::Ramp", 0x00487FC0, &c07pcv::Ramp);
PCV_REVERSED("CoverPoint::Construct", 0x00488010, &c07pcv::PointConstruct);
PCV_REVERSED("CoverType::Parse", 0x004880A0, &c07pcv::ParseType);
PCV_REVERSED("CoverTypes::Load", 0x00488260, &c07pcv::LoadTypes);
PCV_REVERSED("CoverPoints::Update", 0x00488350, &c07pcv::UpdatePoints);
PCV_REVERSED("CoverPoints::Add", 0x004883A0, &c07pcv::AddPoint);
PCV_REVERSED("CoverPoints::Has", 0x004883E0, &c07pcv::HasPoint);
PCV_REVERSED("CoverPoints::RemoveOwner", 0x00488420, &c07pcv::RemovePoints);
PCV_REVERSED("WeightedList::Pick", 0x004884A0, &c07pcv::PickWeighted);
PCV_REVERSED("CoverPoints::Choose", 0x00488550, &c07pcv::ChoosePoint);
