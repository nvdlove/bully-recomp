// The save points (0x20CEB78): up to ten places the player can save at, each keyed by its
// area object, with a position, a direction and a heading. 0x737C90..0x737F30.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct SpVec {
    float x, y, z;
};

struct SavePoint {
    re::Ptr32<void> key;       // +0x0
    SpVec pos;       // +0x4
    SpVec dir;       // +0x10
    float heading;   // +0x1C
    int32_t area;    // +0x20
};
static_assert(sizeof(SavePoint) == 0x24);

struct SavePoints {
    SavePoint points[10];   // +0x0
    uint32_t count;         // +0x168
};
static_assert(offsetof(SavePoints, count) == 0x168);

inline SavePoints* const spPoints = reinterpret_cast<SavePoints*>(0x020CEB78);

GAME_FN(uint8_t, __cdecl, Sp_Has, 0x00737DD0, void* key);

SavePoint* Find(void* key) {
    for (uint32_t i = 0; i < spPoints->count; i++) {
        if (spPoints->points[i].key == key) {
            return &spPoints->points[i];
        }
    }
    return nullptr;
}

} // namespace

namespace savepoints {

// 0x737C90: the static's constructor
SavePoints* __fastcall Construct(SavePoints* self, re::Edx) {
    for (SavePoint& p : self->points) {
        p.pos.x = p.pos.y = p.pos.z = 0.0f;
        p.dir.x = p.dir.y = p.dir.z = 0.0f;
    }
    self->count = 0;
    return self;
}

// 0x737DD0
uint8_t __cdecl Has(void* key) {
    return Find(key) != nullptr;
}

// 0x737E20
SpVec* __cdecl GetPosition(SpVec* out, void* key) {
    SavePoint* p = Find(key);
    out->x = out->y = out->z = 0.0f;
    if (p != nullptr) {
        *out = p->pos;
    }
    return out;
}

// 0x737E80
SpVec* __cdecl GetDirection(SpVec* out, void* key) {
    SavePoint* p = Find(key);
    out->x = out->y = out->z = 0.0f;
    if (p != nullptr) {
        *out = p->dir;
    }
    return out;
}

// 0x737EE0
float __cdecl GetHeading(void* key) {
    SavePoint* p = Find(key);
    float heading = 0.0f;
    if (p != nullptr) {
        heading = p->heading;
    }
    return heading;
}

// 0x737F30 (no check for room: the game has ten)
void __cdecl Add(void* key, const SpVec* pos, float heading, int area, const SpVec* dir) {
    if (Sp_Has(key)) {
        return;
    }
    SavePoint p;
    p.key = key;
    p.pos = *pos;
    p.dir = *dir;
    p.heading = heading;
    p.area = area;
    spPoints->points[spPoints->count] = p;
    spPoints->count++;
}

} // namespace savepoints

#define SAVEPOINTS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

SAVEPOINTS_REVERSED("SavePoints::Construct", 0x00737C90, &savepoints::Construct);
SAVEPOINTS_REVERSED("SavePoints::Has", 0x00737DD0, &savepoints::Has);
SAVEPOINTS_REVERSED("SavePoints::GetPosition", 0x00737E20, &savepoints::GetPosition);
SAVEPOINTS_REVERSED("SavePoints::GetDirection", 0x00737E80, &savepoints::GetDirection);
SAVEPOINTS_REVERSED("SavePoints::GetHeading", 0x00737EE0, &savepoints::GetHeading);
SAVEPOINTS_REVERSED("SavePoints::Add", 0x00737F30, &savepoints::Add);
