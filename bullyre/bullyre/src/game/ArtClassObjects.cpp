// Art class (MGClassArt): the objects' and the deck's helpers before 0x6DD740
// (0x6DADC0..0x6DD736): the object base (its size from the texture, its update), the
// line crawlers' path along the lines, power-up icons, particles, the deck's reset /
// cursor / drawing / stepping / collision, and MGClassArt's object bookkeeping. The
// layouts are ArtClassArcade.cpp's (ArtObject +0x14 position, +0x1C size, +0x24 radius,
// +0x28 age, +0x2C state, +0x34 spin, +0x38 state time; the deck as ArtClassDeck.cpp
// describes). Positions are in play-area units (0..1 across the 4:3 area).
//
// The line path (0x44, at a line crawler's +0x70): +0 the deck, +4 / +8 the vertex, +0xC
// the length (cells), +0x10 the direction (0 up, 1 right, 2 down, 3 left), +0x14 the
// progress, +0x18 the cells passed, +0x1C how many cells it borders (1 or 2), +0x20 their
// first cells, +0x30 the cells now, +0x40 their filled flags.

#include <cmath>
#include <cstdint>
#include "game/Crt.h"

#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

GAME_VAR(float, aco_screenScaleX, 0x00A957A8);
GAME_VAR(float, aco_screenScaleY, 0x00A957AC);
constexpr uintptr_t VT_ArtGameObject = 0x009442CC;
constexpr uintptr_t VT_ArtExploder = 0x009442E0;
constexpr uintptr_t VT_ArtParticleArray = 0x009442B4;

inline int __cdecl ACO_Rand() { return crt::Rand(); }
GAME_FN(void*, __cdecl, ACO_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, ACO_Free, 0x005EEAB0, void* p);
inline void* __cdecl ACO_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(void, __cdecl, ACO_DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(uint8_t*, __thiscall, ACO_Rgba_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, ACO_WStrCpy, 0x0068FFA0, uint16_t* to, const uint16_t* from);
// (this file's, and the deck's)
GAME_FN(void, __thiscall, ACO_ArtObject_Construct, 0x006DC010, void* object, int kind, void* texture);
GAME_FN(void, __thiscall, ACO_LinePath_Set, 0x006DB010, void* path, int32_t col, int32_t row, int32_t length,
        int32_t direction);
GAME_FN(void, __thiscall, ACO_LinePath_Reverse, 0x006DB140, void* path);
GAME_FN(float*, __thiscall, ACO_Deck_GetCentre, 0x006DB950, void* deck, float* out);
GAME_FN(void, __thiscall, ACO_Deck_RandomCorner, 0x006DA770, void* deck, uint32_t* col, uint32_t* row);

// an unsigned dword as the x87 loads it (fild, +2^32 with the sign bit set)
float U2F(uint32_t value) {
    double d = double(int32_t(value));
    if (int32_t(value) < 0) {
        d = double(float(d + 4294967296.0));
    }
    return float(d);
}

// the x87's chopped conversion (fistp qword), its low dword
int32_t Chop(float value) {
    return int32_t(int64_t(value));
}

// the 4:3 play area on the screen
float AreaWidth() {
    return aco_screenScaleX * 480.0f;
}
float AreaHeight() {
    return float(double(aco_screenScaleY) * 360.142857);
}

// an object's size (and radius) from its texture's
void SizeFromTexture(uint8_t* object) {
    void* texture = At<void*>(object, 8);
    float sx = aco_screenScaleX;
    At<float>(object, 0x1C) = U2F(vc::Call<uint32_t>(texture, 0x48)) * sx / (sx * 480.0f);
    float sy = aco_screenScaleY;
    float height = U2F(vc::Call<uint32_t>(texture, 0x4C)) * sy / float(double(sy) * 360.142857);
    At<float>(object, 0x20) = height;
    float smaller = At<float>(object, 0x1C) < height ? At<float>(object, 0x1C) : height;
    At<float>(object, 0x24) = smaller * 0.5f * 0.7f;
}

} // namespace

#define ARTOBJ_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// 0x6DADC0 (thiscall): a power-up (kinds 9..12)
int __fastcall ArtObject_IsPowerUp(uint8_t* self, re::Edx) {
    int32_t kind = At<int32_t>(self, 4);
    return (kind >= 9 && kind <= 0xC) ? 1 : 0;
}
ARTOBJ_REVERSED("ArtObject::IsPowerUp", 0x006DADC0, &ArtObject_IsPowerUp);

// 0x6DADE0 (thiscall, slot 1): the base update: its size, age and spin; a fading object
// (state 3) is gone once its time's run out
int __fastcall ArtObject_Update(uint8_t* self, re::Edx, uint32_t ticks) {
    float sx = aco_screenScaleX;
    void* texture = At<void*>(self, 8);
    At<float>(self, 0x1C) = U2F(vc::Call<uint32_t>(texture, 0x48)) * sx / (sx * 480.0f);
    float sy = aco_screenScaleY;
    float height = U2F(vc::Call<uint32_t>(At<void*>(self, 8), 0x4C)) * sy / float(double(sy) * 360.142857);
    At<float>(self, 0x20) = height;
    float smaller = At<float>(self, 0x1C) < height ? At<float>(self, 0x1C) : height;
    At<uint32_t>(self, 0x28) += ticks;
    At<float>(self, 0x24) = smaller * 0.5f * 0.7f;
    At<float>(self, 0x34) = float(double(U2F(ticks)) * 0.06981317202250163) + At<float>(self, 0x34);
    int32_t state = At<int32_t>(self, 0x2C);
    if (state == 2) {
        At<uint32_t>(self, 0x38) += ticks;
        return 0;
    }
    if (state != 3) {
        return 0;
    }
    uint32_t fade = ticks * 4;
    uint32_t left = At<uint32_t>(self, 0x38);
    if (left == 0) {
        return 1;
    }
    At<uint32_t>(self, 0x38) = left > fade ? left - fade : 0;
    return 0;
}
ARTOBJ_REVERSED("ArtObject::Update", 0x006DADE0, &ArtObject_Update);

// 0x6DAF10 (thiscall, slot 0): a line crawler placed from a pre-instance entry (its
// vertex and its first direction)
void __fastcall ArtLineObject_Place(uint8_t* self, re::Edx, const uint8_t* entry) {
    At<uint32_t>(self, 0x5C) = At<uint32_t>(const_cast<uint8_t*>(entry), 8);
    At<uint32_t>(self, 0x60) = At<uint32_t>(const_cast<uint8_t*>(entry), 0xC);
    At<uint32_t>(self, 0x6C) = At<uint32_t>(const_cast<uint8_t*>(entry), 0x10);
}
ARTOBJ_REVERSED("ArtLineObject::Place", 0x006DAF10, &ArtLineObject_Place);

// 0x6DAF30 (thiscall, slot 3): the next direction at a vertex, from the runs open
// (left, up, right, down): the placed one first; not back the way it came unless that's
// the only way; at random among the rest
int __fastcall ArtLineObject_ChooseDirection(uint8_t* self, re::Edx, int left, int up, int right, int down) {
    bool only = ((left != 0 ? 1 : 0) + (up != 0 ? 1 : 0) + (right != 0 ? 1 : 0) + (down != 0 ? 1 : 0)) == 1;
    int32_t placed = At<int32_t>(self, 0x6C);
    if (placed != 5) {
        At<int32_t>(self, 0x6C) = 5;
        return placed;
    }
    int32_t choices[4];
    uint32_t count = 0;
    int32_t dir = At<int32_t>(self, 0x58);
    if (left != 0 && (only || dir != 1)) {
        choices[count++] = 3;
    }
    if (up != 0 && (only || dir != 2)) {
        choices[count++] = 0;
    }
    if (right != 0 && (only || dir != 3)) {
        choices[count++] = 1;
    }
    if (down != 0 && (only || dir != 0)) {
        choices[count++] = 2;
    }
    return choices[uint32_t(ACO_Rand()) % count];
}
ARTOBJ_REVERSED("ArtLineObject::ChooseDirection", 0x006DAF30, &ArtLineObject_ChooseDirection);

// 0x6DB010 (thiscall): the path set along a run: the cells either side of it (one at
// the deck's edge) and their filled flags
void __fastcall ArtLinePath_Set(int32_t* self, re::Edx, int32_t col, int32_t row, int32_t length, int32_t direction) {
    uint8_t* deck = reinterpret_cast<uint8_t*>(self[0]);
    self[5] = 0;
    self[3] = length;
    self[4] = direction;
    self[6] = 0;
    self[1] = col;
    self[2] = row;
    int32_t size = At<int32_t>(deck, 0xC);
    if (direction == 3 || direction == 1) {
        int32_t cellCol = (direction != 3 ? 1 : 0) - 1 + col;
        if (row == 0) {
            self[7] = 1;
            self[9] = 0;
            self[8] = cellCol;
        } else if (row == size) {
            self[7] = 1;
            self[8] = cellCol;
            self[9] = size - 1;
        } else {
            self[0xB] = row;
            self[8] = cellCol;
            self[9] = row - 1;
            self[0xA] = cellCol;
            self[7] = 2;
        }
    } else {
        int32_t cellRow = (direction != 0 ? 1 : 0) - 1 + row;
        if (col == 0) {
            self[7] = 1;
            self[8] = 0;
            self[9] = cellRow;
        } else if (col != size) {
            self[8] = col - 1;
            self[9] = cellRow;
            self[0xA] = col;
            self[0xB] = cellRow;
            self[7] = 2;
        } else {
            self[7] = 1;
            self[8] = size - 1;
            self[9] = cellRow;
        }
    }
    for (uint32_t i = 0; i < uint32_t(self[7]); i++) {
        self[0xC + i * 2] = self[8 + i * 2];
        self[0xD + i * 2] = self[9 + i * 2];
        uint8_t* cells = At<uint8_t*>(deck, 0x10);
        At<uint8_t>(self, 0x40 + int(i)) =
            cells[(At<int32_t>(deck, 0xC) * self[0xD + i * 2] + self[0xC + i * 2]) * 2 + 1] & 1;
    }
}
// caller 0x6DB140 (ArtLinePath::Reverse) relies on ecx: turn on together with it
ARTOBJ_REVERSED("ArtLinePath::Set", 0x006DB010, &ArtLinePath_Set);

// 0x6DB140 (thiscall): the path turned round (from its other end, the way back)
void __fastcall ArtLinePath_Reverse(int32_t* self, re::Edx) {
    int32_t col = self[1];
    int32_t row = self[2];
    switch (self[4]) {
    case 0:
        row -= self[3];
        self[4] = 2;
        break;
    case 1:
        col += self[3];
        self[4] = 3;
        break;
    case 2:
        row += self[3];
        self[4] = 0;
        break;
    case 3:
        col -= self[3];
        self[4] = 1;
        break;
    default:
        break;
    }
    uint8_t* deck = reinterpret_cast<uint8_t*>(self[0]);
    float inverse = 1.0f / U2F(At<uint32_t>(deck, 0xC));
    int32_t length = self[3];
    float progress = At<float>(self, 0x14);
    float remaining = inverse * U2F(uint32_t(length)) - progress;
    int32_t passed = Chop(progress / inverse);
    ACO_LinePath_Set(self, col, row, length, self[4]);
    self[6] = passed;
    At<float>(self, 0x14) = remaining;
}
ARTOBJ_REVERSED("ArtLinePath::Reverse", 0x006DB140, &ArtLinePath_Reverse);

// 0x6DB230 (thiscall): moved on along the path: the cells it borders follow it
void __fastcall ArtLinePath_Advance(int32_t* self, re::Edx, float distance) {
    float progress = distance + At<float>(self, 0x14);
    At<float>(self, 0x14) = progress;
    uint8_t* deck = reinterpret_cast<uint8_t*>(self[0]);
    float inverse = 1.0f / U2F(At<uint32_t>(deck, 0xC));
    int32_t passed = Chop(progress / inverse);
    if (passed == self[6]) {
        return;
    }
    self[6] = passed;
    int32_t count = self[7];
    switch (self[4]) {
    case 0:
        for (uint32_t i = 0; i < uint32_t(count); i++) {
            self[0xD + i * 2] = self[9 + i * 2] - passed;
        }
        break;
    case 1:
        for (uint32_t i = 0; i < uint32_t(count); i++) {
            self[0xC + i * 2] = self[8 + i * 2] + passed;
        }
        break;
    case 2:
        for (uint32_t i = 0; i < uint32_t(count); i++) {
            self[0xD + i * 2] = self[9 + i * 2] + passed;
        }
        break;
    case 3:
        for (uint32_t i = 0; i < uint32_t(count); i++) {
            self[0xC + i * 2] = self[8 + i * 2] - passed;
        }
        break;
    default:
        break;
    }
    for (uint32_t i = 0; i < uint32_t(self[7]); i++) {
        uint8_t* cells = At<uint8_t*>(deck, 0x10);
        At<uint8_t>(self, 0x40 + int(i)) =
            cells[(self[0xD + i * 2] * At<int32_t>(deck, 0xC) + self[0xC + i * 2]) * 2 + 1] & 1;
    }
}
ARTOBJ_REVERSED("ArtLinePath::Advance", 0x006DB230, &ArtLinePath_Advance);

// 0x6DB360 (thiscall): a cell it borders was filled (or emptied) since
uint8_t __fastcall ArtLinePath_Cut(int32_t* self, re::Edx) {
    uint8_t* deck = reinterpret_cast<uint8_t*>(self[0]);
    for (uint32_t i = 0; i < uint32_t(self[7]); i++) {
        uint8_t* cells = At<uint8_t*>(deck, 0x10);
        uint8_t filled = cells[(self[0xD + i * 2] * At<int32_t>(deck, 0xC) + self[0xC + i * 2]) * 2 + 1] & 1;
        if (At<uint8_t>(self, 0x40 + int(i)) != filled) {
            return 1;
        }
    }
    return 0;
}
ARTOBJ_REVERSED("ArtLinePath::Cut", 0x006DB360, &ArtLinePath_Cut);

// 0x6DB3B0 (thiscall): a power-up icon's tick: its pulse, fading in, out at the last
// 0x3F ticks; false once gone
uint8_t __fastcall ArtPowerUpIcon_Update(uint8_t* self, re::Edx, uint32_t ticks) {
    if (At<uint8_t>(self, 0x4C) != 0) {
        return 0;
    }
    At<float>(self, 0x50) = float(double(U2F(ticks)) * 0.06981317202250163) + At<float>(self, 0x50);
    uint32_t left = At<uint32_t>(self, 0x40);
    if (left <= 0x3F) {
        At<int32_t>(self, 0x48) = 2;
    }
    uint32_t step = ticks * 4;
    switch (At<int32_t>(self, 0x48)) {
    case 1: {
        uint32_t alpha = At<uint32_t>(self, 0x44) + step;
        if (alpha > 0xFF) {
            At<uint32_t>(self, 0x40) = left - ticks;
            At<uint32_t>(self, 0x44) = 0xFF;
            At<int32_t>(self, 0x48) = 0;
            return 1;
        }
        At<uint32_t>(self, 0x44) = alpha;
        break;
    }
    case 2: {
        uint32_t alpha = At<uint32_t>(self, 0x44);
        if (alpha < step) {
            At<uint32_t>(self, 0x40) = left - ticks;
            At<uint32_t>(self, 0x44) = 0;
            At<int32_t>(self, 0x48) = 0;
            At<uint8_t>(self, 0x4C) = 1;
            return 1;
        }
        At<uint32_t>(self, 0x44) = alpha - step;
        break;
    }
    default:
        break;
    }
    At<uint32_t>(self, 0x40) = left - ticks;
    return 1;
}
// caller 0x6DC880 (MGClassArt::UpdateIcons) relies on ecx: turn on together with it
ARTOBJ_REVERSED("ArtPowerUpIcon::Update", 0x006DB3B0, &ArtPowerUpIcon_Update);

// 0x6DB460 (thiscall, slot 0): a flash's tick: false once past its life
uint8_t __fastcall ArtFlash_Update(uint8_t* self, re::Edx, uint32_t ticks) {
    uint32_t age = At<uint32_t>(self, 0x18) + ticks;
    if (age > At<uint32_t>(self, 0x1C)) {
        return 0;
    }
    At<uint32_t>(self, 0x18) = age;
    return 1;
}
ARTOBJ_REVERSED("ArtFlash::Update", 0x006DB460, &ArtFlash_Update);

// 0x6DB480 (thiscall): a pointer array's capacity set (the entries kept; `setCount`:
// the count becomes the capacity)
void __fastcall ArtArray_Reserve(uint32_t* self, re::Edx, uint32_t capacity, int setCount) {
    // (the capacity first: CWeapon::GetChild grows its child array through this and tests the
    // capacity against the count; without this store the array grew once and every further
    // child was written past its end, into the neighbouring heap item: the 2026-10-30 soaks'
    // heap corruption)
    self[1] = capacity;
    re::Ptr32<void>* data = nullptr;
    if (capacity != 0) {
        uint64_t bytes = uint64_t(capacity) * 4;
        data = static_cast<re::Ptr32<void>*>(ACO_Malloc((bytes >> 32) != 0 ? 0xFFFFFFFF : uint32_t(bytes)));
    }
    if (self[0] != 0) {
        for (uint32_t i = 0; i < self[2]; i++) {
            data[i] = reinterpret_cast<re::Ptr32<void>*>(self[0])[i];
        }
        ACO_Free(reinterpret_cast<void*>(self[0]));
    }
    self[0] = re::Addr32(data);
    if (re::Flag(setCount)) {
        self[2] = self[1];
    }
}
ARTOBJ_REVERSED("ArtArray::Reserve", 0x006DB480, &ArtArray_Reserve);

// 0x6DB4F0 (thiscall): a stretch of the deck drawn with the texture's matching part:
// columns col..col+1, rows row..row2
void __fastcall ArtDeck_DrawCells(uint8_t* self, re::Edx, uint32_t col, uint32_t row, uint32_t row2, void* texture,
                                  const uint8_t* colour) {
    float inverse = 1.0f / U2F(At<uint32_t>(self, 0xC));
    float uv1[2];
    float uv2[2];
    uv1[0] = U2F(col) * inverse;
    uv1[1] = U2F(row) * inverse;
    uv2[0] = U2F(col + 1) * inverse;
    uv2[1] = U2F(row2) * inverse;
    float height = AreaHeight();
    float top = (480.0f - height) * 0.5f;
    float width = 480.0f * aco_screenScaleX;
    float left = (640.0f - width) * 0.5f;
    float y2 = (uv2[1] * height + top) - 10.0f;
    float x2 = uv2[0] * width + left;
    float y1 = (top + height * uv1[1]) - 10.0f;
    float x1 = left + width * uv1[0];
    ACO_DrawRectUV(x1, y1, x2, y2, texture, colour, uv1, uv2);
}
ARTOBJ_REVERSED("ArtDeck::DrawCells", 0x006DB4F0, &ArtDeck_DrawCells);

// 0x6DB630 (thiscall): ParticleArray's deleting destructor
void* __fastcall ArtParticleArray_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = VT_ArtParticleArray;
    if ((flags & 1) != 0) {
        ACO_Free(self);
    }
    return self;
}
ARTOBJ_REVERSED("MGClassArt::ParticleArray::DeletingDestructor", 0x006DB630, &ArtParticleArray_DeletingDestructor);

// 0x6DB650 (thiscall): MGClassArt: the three power-up icons told to go, every object
// (and, with `player`, the player) set fading out
void __fastcall MGClassArt_FadeAll(uint8_t* self, re::Edx, int player) {
    for (int offset = 0x5FC; offset <= 0x604; offset += 4) {
        if (At<uint8_t*>(self, offset) != nullptr) {
            At<uint8_t>(At<uint8_t*>(self, offset), 0x5C) = 1;
            At<uint8_t*>(self, offset) = nullptr;
        }
    }
    if (re::Flag(player)) {
        uint8_t* object = At<uint8_t*>(self, 0x520);
        At<uint32_t>(object, 0x38) = 0xFF;
        At<int32_t>(object, 0x2C) = 3;
    }
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x654); i++) {
        uint8_t* object = At<re::Ptr32<uint8_t>*>(self, 0x64C)[i];
        if (At<int32_t>(object, 0x2C) != 3) {
            At<uint32_t>(object, 0x38) = 0xFF;
            At<int32_t>(object, 0x2C) = 3;
        }
    }
}
ARTOBJ_REVERSED("MGClassArt::FadeAll", 0x006DB650, &MGClassArt_FadeAll);

// 0x6DB6E0 (thiscall): MGClassArt: the objects (not debris; power-ups only with `all`;
// not the fading) stilled or set moving
void __fastcall MGClassArt_SetObjectsStill(uint8_t* self, re::Edx, int still, int all) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x654); i++) {
        uint8_t* object = At<re::Ptr32<uint8_t>*>(self, 0x64C)[i];
        int32_t kind = At<int32_t>(object, 4);
        if (kind == 7 || (!re::Flag(all) && kind >= 9 && kind <= 0xC) || At<int32_t>(object, 0x2C) == 3) {
            continue;
        }
        At<int32_t>(object, 0x2C) = re::Flag(still) ? 1 : 0;
    }
    At<uint8_t>(self, 0x5E0) = uint8_t(still);
}
ARTOBJ_REVERSED("MGClassArt::SetObjectsStill", 0x006DB6E0, &MGClassArt_SetObjectsStill);

// 0x6DB770 (thiscall): MGClassArt: an object's going counted off (enemies, power-ups,
// per kind)
void __fastcall MGClassArt_CountRemoved(uint8_t* self, re::Edx, const uint8_t* object) {
    int32_t kind = At<int32_t>(const_cast<uint8_t*>(object), 4);
    if (kind >= 1 && kind <= 8) {
        At<int32_t>(self, 0x5D4) -= 1;
        static const int offsets[9] = {0, 0x558, 0x568, 0x528, 0x538, 0x548, 0x588, 0, 0x578};
        if (kind != 7) {
            At<int32_t>(self, offsets[kind]) -= 1;
        }
        return;
    }
    if (kind >= 9 && kind <= 0xC) {
        At<int32_t>(self, 0x5D8) -= 1;
        static const int offsets[4] = {0x598, 0x5B8, 0x5A8, 0x5C8};
        At<int32_t>(self, offsets[kind - 9]) -= 1;
    }
}
ARTOBJ_REVERSED("MGClassArt::CountRemoved", 0x006DB770, &MGClassArt_CountRemoved);

// 0x6DB860 (thiscall): the deck emptied: only its border lines
void __fastcall ArtDeck_Reset(uint8_t* self, re::Edx) {
    ACO_Memset(At<void*>(self, 0x10), 0, At<uint32_t>(self, 0xC) * At<uint32_t>(self, 0xC) * 2);
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xC); i++) {
        uint16_t* cells = At<uint16_t*>(self, 0x10);
        uint32_t size = At<uint32_t>(self, 0xC);
        if (i == 0) {
            cells[0] = 3;
            cells[(size - 1) * size] = 9;
        } else if (i == size - 1) {
            cells[i] = 6;
            cells[(size - 1) * size + i] = 0xC;
        } else {
            cells[i] = 2;
            cells[(size - 1) * size + i] = 8;
            cells[size * i] = 1;
            cells[(i + 1) * size - 1] = 4;
        }
    }
    uint32_t size = At<uint32_t>(self, 0xC);
    At<uint32_t>(self, 0x18) = 0;
    At<uint8_t>(self, 0x1C) = 0;
    At<uint8_t>(self, 0x1D) = 0;
    At<uint8_t>(self, 0x24) = 0;
    At<uint32_t>(self, 0x14) = size >> 1;
    At<int32_t>(self, 0x20) = 5;
    At<uint32_t>(self, 0x3C) = size * size;
    At<int32_t>(self, 0x40) = int32_t(size >> 1) - 1;
    At<int32_t>(self, 0x44) = int32_t(size >> 1) - 1;
}
ARTOBJ_REVERSED("ArtDeck::Reset", 0x006DB860, &ArtDeck_Reset);

// 0x6DB950 (thiscall): the centre cell's middle
float* __fastcall ArtDeck_GetCentre(uint8_t* self, re::Edx, float* out) {
    float size = U2F(At<uint32_t>(self, 0xC));
    float half = (1.0f / size) * 0.5f;
    out[0] = U2F(At<uint32_t>(self, 0x40)) / size + half;
    out[1] = U2F(At<uint32_t>(self, 0x44)) / size + half;
    return out;
}
ARTOBJ_REVERSED("ArtDeck::GetCentre", 0x006DB950, &ArtDeck_GetCentre);

// 0x6DBDF0 (thiscall): the drawing cursor's position (a cell further on with the half
// step flags)
float* __fastcall ArtDeck_GetCursor(uint8_t* self, re::Edx, float* out) {
    float inverse = 1.0f / U2F(At<uint32_t>(self, 0xC));
    float x = U2F(At<uint32_t>(self, 0x14)) * inverse;
    out[0] = x;
    float y = U2F(At<uint32_t>(self, 0x18)) * inverse;
    out[1] = y;
    if (At<uint8_t>(self, 0x1C) != 0) {
        out[0] = x + inverse;
    }
    if (At<uint8_t>(self, 0x1D) != 0) {
        out[1] = y + inverse;
    }
    return out;
}
ARTOBJ_REVERSED("ArtDeck::GetCursor", 0x006DBDF0, &ArtDeck_GetCursor);

// 0x6DBE80 (thiscall): the deck's two colours' alphas (their alpha / 255 * alpha)
void __fastcall ArtDeck_SetAlpha(uint8_t* self, re::Edx, int alpha) {
    uint8_t a[4];
    uint8_t b[4];
    ACO_Rgba_Set(a, 0x24, 0x97, 0xFF, 0xFF);
    ACO_Rgba_Set(b, 0xFF, 0x97, 0x24, 0xFF);
    float value = float(int32_t(uint8_t(alpha)));
    At<uint8_t>(self, 0x57) = uint8_t(int32_t(float(int32_t(a[3])) / 255.0f * value));
    At<uint8_t>(self, 0x5F) = uint8_t(int32_t(float(int32_t(b[3])) / 255.0f * value));
}
// caller 0x6E2750 (MGClassArt::UpdateGame, ArtClassArcade.cpp) relies on ecx and edx: turn on together with it
ARTOBJ_REVERSED("ArtDeck::SetAlpha", 0x006DBE80, &ArtDeck_SetAlpha);

// 0x6DC010 (thiscall): an object's constructor (its size from its texture)
uint8_t* __fastcall ArtObject_Construct(uint8_t* self, re::Edx, int kind, void* texture) {
    At<float>(self, 0x14) = 0.0f;
    At<float>(self, 0x18) = 0.0f;
    At<int32_t>(self, 4) = kind;
    At<uint32_t>(self, 0) = VT_ArtGameObject;
    At<void*>(self, 8) = texture;
    At<void*>(self, 0xC) = nullptr;
    At<uint32_t>(self, 0x10) = 0;
    SizeFromTexture(self);
    At<uint32_t>(self, 0x38) = 0;
    At<uint32_t>(self, 0x28) = 0;
    At<int32_t>(self, 0x2C) = 0;
    At<float>(self, 0x34) = 0.0f;
    return self;
}
ARTOBJ_REVERSED("ArtObject::ArtObject", 0x006DC010, &ArtObject_Construct);

// 0x6DC110 (thiscall): the player's power-up over (a reversal's textures swapped back)
void __fastcall ArtPlayer_EndPowerUp(uint8_t* self, re::Edx) {
    At<void*>(self, 0xC) = nullptr;
    uint8_t powered = At<uint8_t>(self, 0x74);
    At<uint8_t>(self, 0x74) = 0;
    if (powered != 0 && At<int32_t>(self, 0x7C) == 1) {
        void* texture = At<void*>(self, 8);
        At<void*>(self, 8) = At<void*>(self, 0x3C);
        At<void*>(self, 0x3C) = texture;
    }
}
ARTOBJ_REVERSED("ArtPlayer::EndPowerUp", 0x006DC110, &ArtPlayer_EndPowerUp);

// 0x6DC150 (thiscall): a line crawler with no placed start: a random corner not the
// cursor's
void __fastcall ArtLineObject_PickStart(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x6C) != 5) {
        return;
    }
    uint8_t* deck = At<uint8_t*>(self, 0x3C);
    ACO_Deck_RandomCorner(deck, &At<uint32_t>(self, 0x5C), &At<uint32_t>(self, 0x60));
    deck = At<uint8_t*>(self, 0x3C);
    float inverse = 1.0f / U2F(At<uint32_t>(deck, 0xC));
    At<float>(self, 0x14) = inverse * U2F(At<uint32_t>(self, 0x5C));
    inverse = 1.0f / U2F(At<uint32_t>(deck, 0xC));
    At<float>(self, 0x18) = inverse * U2F(At<uint32_t>(self, 0x60));
    At<int32_t>(self, 0x58) = 5;
    At<float>(self, 0x68) = 0.0f;
    uint8_t* player = At<uint8_t*>(self, 0x44);
    At<float>(self, 0x48) = At<float>(player, 0x14);
    At<float>(self, 0x4C) = At<float>(player, 0x18);
    At<uint32_t>(self, 0x28) = 0;
}
ARTOBJ_REVERSED("ArtLineObject::PickStart", 0x006DC150, &ArtLineObject_PickStart);

// 0x6DC210 (thiscall): a line crawler turned back along its segment
void __fastcall ArtLineObject_TurnBack(uint8_t* self, re::Edx) {
    int32_t dir = At<int32_t>(self, 0x58);
    if (dir >= 0 && dir <= 3) {
        static const int32_t opposite[4] = {2, 3, 0, 1};
        At<int32_t>(self, 0x58) = opposite[dir];
    }
    int32_t col = At<int32_t>(self, 0x5C);
    int32_t row = At<int32_t>(self, 0x60);
    At<float>(self, 0x68) = At<float>(self, 0x64) - At<float>(self, 0x68);
    At<int32_t>(self, 0x5C) = At<int32_t>(self, 0x50);
    At<int32_t>(self, 0x60) = At<int32_t>(self, 0x54);
    At<int32_t>(self, 0x50) = col;
    At<int32_t>(self, 0x54) = row;
    ACO_LinePath_Reverse(self + 0x70);
}
ARTOBJ_REVERSED("ArtLineObject::TurnBack", 0x006DC210, &ArtLineObject_TurnBack);

// 0x6DC280 (thiscall): the exploder's constructor: at the deck's centre (its timeout a
// float, chopped)
uint8_t* __fastcall ArtExploder_Construct(uint8_t* self, re::Edx, void* texture, uint8_t* deck, float timeOut,
                                          void* player) {
    ACO_ArtObject_Construct(self, 6, texture);
    At<uint32_t>(self, 0) = VT_ArtExploder;
    At<uint8_t*>(self, 0x3C) = deck;
    At<void*>(self, 0x7C) = player;
    At<uint8_t>(self, 0x68) = 0;
    float centre[2];
    float* c = ACO_Deck_GetCentre(deck, centre);
    At<float>(self, 0x14) = c[0];
    At<float>(self, 0x18) = c[1];
    At<float>(self, 0x60) = At<float>(self, 0x14);
    At<float>(self, 0x64) = At<float>(self, 0x18);
    At<int32_t>(self, 0x40) = Chop(timeOut);
    uint8_t* d = At<uint8_t*>(self, 0x3C);
    float inverse = 1.0f / U2F(At<uint32_t>(d, 0xC));
    At<float>(self, 0x4C) = inverse * 0.5f;
    At<uint8_t>(self, 0x5C) = 0xFF;
    At<float>(self, 0x50) = 1.0f / U2F(At<uint32_t>(d, 0xC));
    At<float>(self, 0x44) = 0.33f;
    At<float>(self, 0x48) = 5.0f;
    At<float>(self, 0x54) = 3.14159274f;
    At<float>(self, 0x58) = 18.849556f;
    At<float>(self, 0x6C) = 0.0f;
    At<float>(self, 0x70) = 0.0f;
    At<float>(self, 0x74) = 0.0f;
    At<float>(self, 0x78) = 0.0f;
    return self;
}
ARTOBJ_REVERSED("ArtExploder::ArtExploder", 0x006DC280, &ArtExploder_Construct);

// 0x6DC380 (thiscall, slot 0): the exploder placed from a pre-instance entry (its
// circle's centre too)
void __fastcall ArtExploder_Place(uint8_t* self, re::Edx, const uint8_t* entry) {
    At<float>(self, 0x14) = At<float>(const_cast<uint8_t*>(entry), 8);
    At<float>(self, 0x18) = At<float>(const_cast<uint8_t*>(entry), 0xC);
    At<float>(self, 0x60) = At<float>(self, 0x14);
    At<float>(self, 0x64) = At<float>(self, 0x18);
}
ARTOBJ_REVERSED("ArtExploder::Place", 0x006DC380, &ArtExploder_Place);

// 0x6DC3A0 (thiscall): a power-up icon's constructor (its time in ticks, fading in)
uint8_t* __fastcall ArtPowerUpIcon_Construct(uint8_t* self, re::Edx, void* texture, const uint16_t* text, void* glow,
                                             const uint8_t* colour, float time) {
    At<void*>(self, 0) = texture;
    ACO_WStrCpy(reinterpret_cast<uint16_t*>(self + 4), text);
    At<void*>(self, 0x38) = glow;
    At<uint8_t>(self, 0x3C) = colour[0];
    At<uint8_t>(self, 0x3D) = colour[1];
    At<uint8_t>(self, 0x3E) = colour[2];
    At<uint8_t>(self, 0x3F) = colour[3];
    At<int32_t>(self, 0x48) = 1;
    At<int32_t>(self, 0x40) = Chop(time * 60.0f);
    At<uint32_t>(self, 0x44) = 0;
    At<uint8_t>(self, 0x4C) = 0;
    At<float>(self, 0x50) = 0.0f;
    return self;
}
ARTOBJ_REVERSED("ArtPowerUpIcon::ArtPowerUpIcon", 0x006DC3A0, &ArtPowerUpIcon_Construct);

// 0x6DC430 (thiscall, slot 1): a particle ring's tick: its particles moved; false past
// its life
uint8_t __fastcall ArtParticleRing_Update(uint8_t* self, re::Edx, uint32_t ticks) {
    uint32_t age = At<uint32_t>(self, 0x14) + ticks;
    if (age > At<uint32_t>(self, 0x1C)) {
        return 0;
    }
    At<uint32_t>(self, 0x14) = age;
    float t = U2F(ticks);
    for (uint32_t i = 0; i < At<uint32_t>(self, 4); i++) {
        float* particle = reinterpret_cast<float*>(At<uint8_t*>(self, 0x18) + i * 0x10);
        float dx = particle[2] * t;
        float dy = particle[3] * t;
        particle[0] = particle[0] + dx;
        particle[1] = particle[1] + dy;
    }
    return 1;
}
ARTOBJ_REVERSED("MGClassArt::ParticleRing::Update", 0x006DC430, &ArtParticleRing_Update);

// 0x6DC4C0 (thiscall): the particles cleared: the flashes freed, the rings deleted
void __fastcall ArtParticles_Clear(uint32_t* self, re::Edx) {
    for (uint32_t i = 0; i < self[2]; i++) {
        ACO_Free(reinterpret_cast<re::Ptr32<void>*>(self[0])[i]);
    }
    if (self[0] != 0) {
        ACO_Free(reinterpret_cast<void*>(self[0]));
    }
    self[0] = 0;
    self[1] = 0;
    self[2] = 0;
    for (uint32_t i = 0; i < self[5]; i++) {
        void* ring = reinterpret_cast<re::Ptr32<void>*>(self[3])[i];
        if (ring != nullptr) {
            vc::Call<void>(ring, 0, 1);
        }
    }
    if (self[3] != 0) {
        ACO_Free(reinterpret_cast<void*>(self[3]));
    }
    self[3] = 0;
    self[5] = 0;
    self[4] = 0;
}
ARTOBJ_REVERSED("ArtParticles::Clear", 0x006DC4C0, &ArtParticles_Clear);

// 0x6DC540 (thiscall): a pointer array's entry removed (shrunk to half its capacity
// when under half full)
void __fastcall ArtArray_Remove(uint32_t* self, re::Edx, uint32_t index) {
    for (uint32_t i = index; i < self[2] - 1; i++) {
        reinterpret_cast<re::Ptr32<void>*>(self[0])[i] = reinterpret_cast<re::Ptr32<void>*>(self[0])[i + 1];
    }
    self[2] -= 1;
    if (self[2] == 0) {
        if (self[0] != 0) {
            ACO_Free(reinterpret_cast<void*>(self[0]));
        }
        self[0] = 0;
        self[1] = 0;
        self[2] = 0;
    }
    uint32_t half = self[1] >> 1;
    if (self[2] >= half) {
        return;
    }
    self[1] = half;
    re::Ptr32<void>* data = nullptr;
    if (half != 0) {
        uint64_t bytes = uint64_t(half) * 4;
        data = static_cast<re::Ptr32<void>*>(ACO_Malloc((bytes >> 32) != 0 ? 0xFFFFFFFF : uint32_t(bytes)));
    }
    if (self[0] != 0) {
        for (uint32_t i = 0; i < self[2]; i++) {
            data[i] = reinterpret_cast<re::Ptr32<void>*>(self[0])[i];
        }
        ACO_Free(reinterpret_cast<void*>(self[0]));
    }
    self[0] = re::Addr32(data);
}
ARTOBJ_REVERSED("ArtArray::Remove", 0x006DC540, &ArtArray_Remove);

namespace {
GAME_FN(int, __thiscall, ACO_Deck_FloodFill, 0x006DABC0, void* deck, int32_t col, int32_t row, uint8_t* marks);
GAME_FN(void, __thiscall, ACO_Deck_GetStartCell, 0x006DAB60, void* deck, int otherSide, int32_t* col, int32_t* row);
GAME_FN(void, __thiscall, ACO_Deck_ClearMarks, 0x006DAD80, void* deck);
GAME_FN(int, __thiscall, ACO_Deck_DrawStep, 0x006DA9C0, void* deck, uint32_t col, uint32_t row, uint32_t direction,
        uint32_t stop);
GAME_FN(void, __thiscall, ACO_Deck_CloseLine, 0x006DBF50, void* deck);
GAME_FN(void, __thiscall, ACO_Deck_Reset, 0x006DB860, void* deck);
inline void* __cdecl ACO_Memcpy(void* to, const void* from, uint32_t size) { return crt::Memcpy(to, from, (size_t)size); }
GAME_FN(void, __cdecl, ACO_DrawRect, 0x0052C470, const float* rect, const uint8_t* colour);
GAME_FN(void, __thiscall, ACO_FadeAll, 0x006DB650, void* game, int player);
GAME_FN(uint8_t*, __thiscall, ACO_Sounds_Play, 0x0059B8A0, void* sounds, uint32_t id, const float* pos, int, uintptr_t, int,
        int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(uint8_t, __thiscall, ACO_PowerUpIcon_Update, 0x006DB3B0, void* icon, uint32_t ticks);
GAME_FN(void, __thiscall, ACO_Array_Remove, 0x006DC540, void* array, uint32_t index);
GAME_FN(void, __thiscall, ACO_LineObject_TurnBack, 0x006DC210, void* object);
GAME_FN(uint8_t, __cdecl, ACO_IntersectSegments, 0x00414930, const float* p, const float* d, const float* q,
        const float* e, float* s, float* t);
inline void* const aco_sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline uint8_t* const aco_scratch = reinterpret_cast<uint8_t*>(0x00BD7800);   // (0x1C000: the flood fill's)
constexpr uintptr_t VT_ArtDeck = 0x009442C4;

// a line's rectangle (GTA's CRect: left, bottom, right, top) from columns / rows,
// the deck's line width (+0x58) either side
void LineRect(uint8_t* deck, float* rect, float col1, float row1, float col2, float row2) {
    float inverse = 1.0f / U2F(At<uint32_t>(deck, 0xC));
    float width = aco_screenScaleX * 480.0f;
    float left = (640.0f - width) * 0.5f;
    float height = AreaHeight();
    float top = (480.0f - height) * 0.5f;
    float line = At<float>(deck, 0x58);
    rect[0] = (col1 * inverse - line) * width + left;
    rect[3] = ((row1 * inverse - line) * height + top) - 10.0f;
    rect[2] = left + width * (col2 * inverse + line);
    rect[1] = (top + height * (row2 * inverse + line)) - 10.0f;
}
} // namespace

// 0x6DB9C0 (thiscall): a move against a cell's lines (left, top, right, bottom in turn):
// the first crossed gives the hit point and which line
uint8_t __fastcall ArtDeck_HitCellLines(uint8_t* self, re::Edx, const float* pos, const float* move, int32_t col,
                                        int32_t row, uint16_t* line, float* hit) {
    uint16_t cell = At<uint16_t*>(self, 0x10)[At<int32_t>(self, 0xC) * row + col];
    if ((cell & 0xF) == 0) {
        return 0;
    }
    float inverse = 1.0f / U2F(At<uint32_t>(self, 0xC));
    struct Edge {
        uint16_t bit;
        int32_t dc, dr;   // the start's offset (cells)
        bool vertical;
    };
    static const Edge edges[4] = {{1, 0, 0, true}, {2, 0, 0, false}, {4, 1, 0, true}, {8, 0, 1, false}};
    for (const Edge& edge : edges) {
        if ((At<uint16_t*>(self, 0x10)[At<int32_t>(self, 0xC) * row + col] & edge.bit) == 0) {
            continue;
        }
        float step = 1.0f / U2F(At<uint32_t>(self, 0xC));
        float start[2];
        float direction[2];
        if (edge.vertical) {
            start[0] = step * U2F(uint32_t(col + edge.dc));
            start[1] = U2F(uint32_t(row)) * step - 0.0f;
            direction[0] = 0.0f;
            direction[1] = inverse;
        } else {
            start[0] = step * U2F(uint32_t(col)) - 0.0f;
            start[1] = U2F(uint32_t(row + edge.dr)) * step;
            direction[0] = inverse;
            direction[1] = 0.0f;
        }
        float s;
        float t[2];
        if (ACO_IntersectSegments(pos, move, start, direction, &s, t) != 0) {
            float my = move[1];
            float py = pos[1];
            hit[0] = pos[0] + t[0] * move[0];
            hit[1] = py + t[0] * my;
            *line = edge.bit;
            return 1;
        }
    }
    return 0;
}
ARTOBJ_REVERSED("ArtDeck::HitCellLines", 0x006DB9C0, &ArtDeck_HitCellLines);

// 0x6DBF50 (thiscall): a line closed: the side the start reached by flood fill gets
// filled if it's at most half what's left, else the other side
void __fastcall ArtDeck_CloseLine(uint8_t* self, re::Edx) {
    uint32_t bytes = At<uint32_t>(self, 0xC) * At<uint32_t>(self, 0xC) * 2;
    ACO_Memcpy(aco_scratch, At<void*>(self, 0x10), bytes);
    uint32_t pad = bytes & 3;
    At<uint8_t*>(self, 0x48) = aco_scratch + pad + bytes;
    At<uint32_t>(self, 0x50) = (0x1C000 - pad - bytes) >> 2;
    int reached = ACO_Deck_FloodFill(self, At<int32_t>(self, 0x28), At<int32_t>(self, 0x2C), aco_scratch);
    uint8_t filledStart;
    if (uint32_t(reached) <= At<uint32_t>(self, 0x3C) >> 1) {
        ACO_Memcpy(At<void*>(self, 0x10), aco_scratch, bytes);
        filledStart = 1;
    } else {
        int32_t col = At<int32_t>(self, 0x28);
        int32_t row = At<int32_t>(self, 0x2C);
        if ((At<uint8_t>(self, 0x38) & 1) != 0) {
            col--;
        } else {
            row--;
        }
        reached = ACO_Deck_FloodFill(self, col, row, At<uint8_t*>(self, 0x10));
        filledStart = 0;
    }
    At<uint32_t>(self, 0x3C) -= uint32_t(reached);
    ACO_Deck_GetStartCell(self, filledStart == 0 ? 1 : 0, &At<int32_t>(self, 0x40), &At<int32_t>(self, 0x44));
    ACO_Deck_ClearMarks(self);
    At<uint8_t>(self, 0x25) = 0;
}
ARTOBJ_REVERSED("ArtDeck::CloseLine", 0x006DBF50, &ArtDeck_CloseLine);

// 0x6DC600 (thiscall): a vertical line drawn: column col, rows row1..row2
void __fastcall ArtDeck_DrawVerticalLine(uint8_t* self, re::Edx, uint32_t col, uint32_t row1, uint32_t row2,
                                         const uint8_t* colour) {
    float rect[4];
    LineRect(self, rect, U2F(col), U2F(row1), U2F(col), U2F(row2));
    ACO_DrawRect(rect, colour);
}
ARTOBJ_REVERSED("ArtDeck::DrawVerticalLine", 0x006DC600, &ArtDeck_DrawVerticalLine);

// 0x6DC6F0 (thiscall): a horizontal line drawn: columns col1..col2, row row
void __fastcall ArtDeck_DrawHorizontalLine(uint8_t* self, re::Edx, uint32_t col1, uint32_t col2, uint32_t row,
                                           const uint8_t* colour) {
    float rect[4];
    LineRect(self, rect, U2F(col1), U2F(row), U2F(col2), U2F(row));
    ACO_DrawRect(rect, colour);
}
ARTOBJ_REVERSED("ArtDeck::DrawHorizontalLine", 0x006DC6F0, &ArtDeck_DrawHorizontalLine);

// 0x6DC7D0 (thiscall): the game over: the icons dropped, everything fading, the result
// (4 won, 5 lost) and its sound. (The game keeps `this` in ecx across 0x6DB650.)
void __fastcall MGClassArt_Finish(uint8_t* self, re::Edx, int won, int32_t score) {
    if (At<uint8_t*>(self, 0x5F8) != nullptr) {
        At<uint8_t>(At<uint8_t*>(self, 0x5F8), 0x5C) = 1;
        At<uint8_t*>(self, 0x5F8) = nullptr;
    }
    if (At<uint8_t*>(self, 0x608) != nullptr) {
        At<uint8_t>(At<uint8_t*>(self, 0x608), 0x5C) = 1;
        At<uint8_t*>(self, 0x608) = nullptr;
    }
    ACO_FadeAll(self, 1);
    At<int32_t>(self, 0x610) = score;
    uint32_t sound;
    if (re::Flag(won)) {
        At<int32_t>(self, 0x5E4) = 4;
        sound = At<uint32_t>(At<uint8_t*>(self, 0x4C), 0x3C);
    } else {
        At<int32_t>(self, 0x5E4) = 5;
        sound = At<uint32_t>(At<uint8_t*>(self, 0x4C), 0x38);
    }
    ACO_Sounds_Play(aco_sounds, sound, nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}
ARTOBJ_REVERSED("MGClassArt::Finish", 0x006DC7D0, &MGClassArt_Finish);

// 0x6DC880 (thiscall): the power-up icons ticked, the gone ones freed. (The game
// tests the icon in ecx after 0x6DB3B0: it relies on that keeping ecx.)
void __fastcall MGClassArt_UpdateIcons(uint8_t* self, re::Edx, uint32_t ticks) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x648); i++) {
        void* icon = At<re::Ptr32<void>*>(self, 0x640)[i];
        if (ACO_PowerUpIcon_Update(icon, ticks) != 0) {
            continue;
        }
        if (icon != nullptr) {
            ACO_Free(icon);
        }
        ACO_Array_Remove(self + 0x640, i);
        i--;
    }
}
ARTOBJ_REVERSED("MGClassArt::UpdateIcons", 0x006DC880, &MGClassArt_UpdateIcons);

// 0x6DC8E0 (thiscall): the line runners (crawlers, fast crawlers, speed boosts and
// reversals; and kind 5) turned back
void __fastcall MGClassArt_TurnBackRunners(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x654); i++) {
        uint8_t* object = At<re::Ptr32<uint8_t>*>(self, 0x64C)[i];
        int32_t kind = At<int32_t>(object, 4);
        if (kind == 3 || kind == 4 || kind == 5 || kind == 9 || kind == 0xB) {
            ACO_LineObject_TurnBack(object);
        }
    }
}
ARTOBJ_REVERSED("MGClassArt::TurnBackRunners", 0x006DC8E0, &MGClassArt_TurnBackRunners);

// 0x6DC930 (stdcall): a texture drawn grey in a rectangle (its outer half texel trimmed)
void __fastcall MGClassArt_DrawTexture(void*, re::Edx, float x, float y, float w, float h, void* texture) {
    float texWidth = U2F(vc::Call<uint32_t>(texture, 0x48));
    float texHeight = U2F(vc::Call<uint32_t>(texture, 0x4C));
    uint8_t grey[4];
    ACO_Rgba_Set(grey, 0x80, 0x80, 0x80, 0xFF);
    float uv1[2];
    float uv2[2];
    uv1[0] = 0.5f / texWidth;
    uv1[1] = 0.5f / texHeight;
    uv2[0] = 1.0f - uv1[0];
    uv2[1] = 1.0f - uv1[1];
    ACO_DrawRectUV(x, y, x + w, y + h, texture, grey, uv1, uv2);
}
ARTOBJ_REVERSED("MGClassArt::DrawTexture", 0x006DC930, &MGClassArt_DrawTexture);

// 0x6DCA20 (thiscall): the deck's constructor (its cells, colours and line width)
uint8_t* __fastcall ArtDeck_Construct(uint8_t* self, re::Edx, uint32_t size, void* backTexture, void* frameTexture) {
    At<void*>(self, 4) = backTexture;
    At<uint32_t>(self, 0xC) = size;
    At<void*>(self, 8) = frameTexture;
    uint64_t bytes = uint64_t(size * size) * 2;
    At<uint32_t>(self, 0) = VT_ArtDeck;
    At<void*>(self, 0x10) = ACO_Malloc((bytes >> 32) != 0 ? 0xFFFFFFFF : uint32_t(bytes));
    const uint8_t colours[24] = {0x24, 0x97, 0xFF, 0x00, 0, 0, 0, 0, 0xFF, 0x97, 0x24, 0x00,
                                 0x80, 0x80, 0x80, 0xFF, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00};
    for (int i = 0; i < 24; i++) {
        if (i >= 4 && i < 8) {
            continue;   // (+0x58 the line width)
        }
        At<uint8_t>(self, 0x54 + i) = colours[i];
    }
    float sy = aco_screenScaleY;
    At<float>(self, 0x58) = (sy + sy) / float(double(sy) * 360.142857) * 0.5f;
    ACO_Deck_Reset(self);
    return self;
}
ARTOBJ_REVERSED("ArtDeck::ArtDeck", 0x006DCA20, &ArtDeck_Construct);

// 0x6DCAD0 (thiscall): the deck's deleting destructor
void* __fastcall ArtDeck_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = VT_ArtDeck;
    ACO_Free(At<void*>(self, 0x10));
    if ((flags & 1) != 0) {
        ACO_Free(self);
    }
    return self;
}
ARTOBJ_REVERSED("ArtDeck::DeletingDestructor", 0x006DCAD0, &ArtDeck_DeletingDestructor);

// 0x6DCEE0 (thiscall): the drawing cursor stepped (0 up, 1 right, 2 down, 3 left; never
// straight back while drawing): the line drawn on, closed (3, the area filled), or
// refused (0)
int __fastcall ArtDeck_Step(uint8_t* self, re::Edx, uint32_t direction) {
    if (At<uint8_t>(self, 0x24) != 0) {
        static const int32_t opposite[4] = {2, 3, 0, 1};
        if (direction <= 3 && At<int32_t>(self, 0x20) == opposite[direction]) {
            return 0;
        }
    }
    uint32_t col = direction;
    uint32_t row = direction;
    uint32_t bits = direction;
    uint32_t stop = direction;
    uint32_t newCol = 0;   // (uninitialised stack in the game for a bad direction)
    uint32_t newRow = direction;
    uint8_t halfX = uint8_t(direction);
    uint8_t halfY = 0;     // (likewise)
    switch (direction) {
    case 3:   // left
        col = At<uint32_t>(self, 0x14);
        if (col == 0) {
            return 0;
        }
        if (At<uint8_t>(self, 0x1C) == 0) {
            col--;
        }
        halfY = At<uint8_t>(self, 0x1D);
        row = At<uint32_t>(self, 0x18);
        halfX = 0;
        bits = halfY != 0 ? 8 : 2;
        stop = 1;
        newRow = row;
        newCol = col;
        break;
    case 0:   // up
        row = At<uint32_t>(self, 0x18);
        if (row == 0) {
            return 0;
        }
        col = At<uint32_t>(self, 0x14);
        if (At<uint8_t>(self, 0x1D) == 0) {
            row--;
        }
        halfX = At<uint8_t>(self, 0x1C);
        halfY = 0;
        bits = halfX != 0 ? 4 : 1;
        stop = 2;
        newRow = row;
        newCol = col;
        break;
    case 1:   // right
        col = At<uint32_t>(self, 0x14);
        if (col == At<uint32_t>(self, 0xC) - 1) {
            if (At<uint8_t>(self, 0x1C) != 0) {
                return 0;
            }
            halfX = 1;
            newCol = col;
        } else {
            newCol = col + 1;
            halfX = 0;
        }
        halfY = At<uint8_t>(self, 0x1D);
        row = At<uint32_t>(self, 0x18);
        bits = halfY != 0 ? 8 : 2;
        stop = 4;
        newRow = row;
        break;
    case 2:   // down
        row = At<uint32_t>(self, 0x18);
        if (row == At<uint32_t>(self, 0xC) - 1) {
            if (At<uint8_t>(self, 0x1D) != 0) {
                return 0;
            }
            halfY = 1;
            newRow = row;
        } else {
            newRow = row + 1;
            halfY = 0;
        }
        halfX = At<uint8_t>(self, 0x1C);
        col = At<uint32_t>(self, 0x14);
        bits = halfX != 0 ? 4 : 1;
        stop = 8;
        newCol = col;
        break;
    default:
        break;
    }
    int result = ACO_Deck_DrawStep(self, col, row, bits, stop);
    if (result == 3) {
        ACO_Deck_CloseLine(self);
    } else if (result == 4) {
        if (At<uint32_t>(self, 0x30) == newCol && At<uint32_t>(self, 0x34) == newRow && At<uint8_t>(self, 0x25) != 0) {
            ACO_Deck_CloseLine(self);
            result = 3;
        }
    } else if (result == 0) {
        return 0;
    }
    At<uint32_t>(self, 0x20) = direction;
    At<uint32_t>(self, 0x14) = newCol;
    At<uint32_t>(self, 0x18) = newRow;
    At<uint8_t>(self, 0x1C) = halfX;
    At<uint8_t>(self, 0x1D) = halfY;
    return result;
}
ARTOBJ_REVERSED("ArtDeck::Step", 0x006DCEE0, &ArtDeck_Step);

#include "game/TrackMath.h"

namespace {
GAME_FN(uint8_t, __thiscall, ACO_Deck_HitCellLines, 0x006DB9C0, void* deck, const float* pos, const float* move,
        int32_t col, int32_t row, uint16_t* line, float* hit);
GAME_FN(void, __thiscall, ACO_Deck_DrawCells, 0x006DB4F0, void* deck, uint32_t col, uint32_t row, uint32_t row2,
        void* texture, const uint8_t* colour);
GAME_FN(void, __thiscall, ACO_Deck_DrawVerticalLine, 0x006DC600, void* deck, uint32_t col, uint32_t row1,
        uint32_t row2, const uint8_t* colour);
GAME_FN(void, __thiscall, ACO_Deck_DrawHorizontalLine, 0x006DC6F0, void* deck, uint32_t col1, uint32_t col2,
        uint32_t row, const uint8_t* colour);
GAME_VAR(uint8_t*, aco_minigame, 0x020C8704);

// the hit's line kind and whether it's the line being drawn
uint8_t ReportHit(uint8_t* deck, uint16_t line, int32_t col, int32_t row, uint8_t* hitVertical, uint8_t* hitLine) {
    *hitVertical = (line & 0xA) != 0 ? 1 : 0;
    uint16_t cell = At<uint16_t*>(deck, 0x10)[At<int32_t>(deck, 0xC) * row + col];
    *hitLine = ((uint32_t(line) << 4) & cell) != 0 ? 1 : 0;
    return 1;
}
} // namespace

// 0x6DCB00 (thiscall): a move against the deck's lines: the cells along it, a cell a
// step (the last step the remainder), each tested with the whole move; leaving the
// area ends it with one more test, of the cell under the point clamped (unsigned) to
// the last column / row
uint8_t __fastcall ArtDeck_Collide(uint8_t* self, re::Edx, const float* pos, const float* move, float* hit,
                                   uint8_t* hitVertical, uint8_t* hitLine) {
    float mx2 = move[0] * move[0];
    float inverse = 1.0f / U2F(At<uint32_t>(self, 0xC));
    float length = crt::Sqrtf(std::fabs(move[1] * move[1] + mx2));
    int32_t steps = Chop(length / inverse);
    float step[2] = {move[0], move[1]};
    float scale = inverse / length;
    step[0] = step[0] * scale;
    step[1] = scale * step[1];
    float rest = length - U2F(uint32_t(steps)) * inverse;
    float scaleRest = rest / length;
    float last[2] = {move[0] * scaleRest, scaleRest * move[1]};
    float point[2] = {pos[0], pos[1]};
    uint32_t limit = uint32_t(steps + 1);
    uint16_t line;
    float a;
    float b;
    for (uint32_t i = 0;; i++) {
        float size = U2F(At<uint32_t>(self, 0xC));
        int32_t col = Chop(size * point[0]);
        int32_t row = Chop(size * point[1]);
        if (ACO_Deck_HitCellLines(self, pos, move, col, row, &line, hit) != 0) {
            return ReportHit(self, line, col, row, hitVertical, hitLine);
        }
        const float* by = i == limit - 1 ? last : step;
        point[0] = by[0] + point[0];
        point[1] = by[1] + point[1];
        float px = point[0];
        float py = point[1];
        if (px < 0.0f || px >= 1.0f || 0.0f > py || py >= 1.0f) {
            a = px;
            b = py;
            break;
        }
        if (i + 1 > limit) {
            return 0;
        }
    }
    uint32_t sizeMinus1 = At<uint32_t>(self, 0xC) - 1;
    float size = U2F(At<uint32_t>(self, 0xC));
    uint32_t col = uint32_t(Chop(size * a));
    uint32_t row = uint32_t(Chop(size * b));
    if (col > sizeMinus1) {
        col = At<uint32_t>(self, 0xC) - 1;
    }
    if (row > At<uint32_t>(self, 0xC) - 1) {
        row = At<uint32_t>(self, 0xC) - 1;
    }
    if (ACO_Deck_HitCellLines(self, pos, move, int32_t(col), int32_t(row), &line, hit) == 0) {
        return 0;
    }
    return ReportHit(self, line, int32_t(col), int32_t(row), hitVertical, hitLine);
}
ARTOBJ_REVERSED("ArtDeck::Collide", 0x006DCB00, &ArtDeck_Collide);

namespace {
// runs down each column (or along each row) of the cells passing `test`, each drawn
template <typename Test, typename Draw>
void Runs(uint8_t* deck, bool& open, uint32_t& start, bool byRow, Test test, Draw draw) {
    for (uint32_t outer = 0; outer < At<uint32_t>(deck, 0xC); outer++) {
        for (uint32_t inner = 0; inner < At<uint32_t>(deck, 0xC); inner++) {
            uint32_t size = At<uint32_t>(deck, 0xC);
            uint16_t cell = byRow ? At<uint16_t*>(deck, 0x10)[size * outer + inner]
                                  : At<uint16_t*>(deck, 0x10)[size * inner + outer];
            if (test(cell)) {
                if (!open) {
                    open = true;
                    start = inner;
                }
            } else if (open) {
                draw(outer, start, inner);
                open = false;
            }
        }
        if (open) {
            draw(outer, start, At<uint32_t>(deck, 0xC));
            open = false;
        }
    }
}
} // namespace

// 0x6DD110 (thiscall): the deck drawn: a black backdrop, the back texture, the filled
// cells (in the frame texture's colours), the unfilled ones when shown, the line being
// drawn and the lines, the edges, the minigame's overlay texture on top
void __fastcall ArtDeck_Draw(uint8_t* self, re::Edx) {
    float backdrop[4] = {0.0f, 480.0f, 640.0f, 0.0f};
    bool open = false;
    uint8_t black[4];
    ACO_DrawRect(backdrop, ACO_Rgba_Set(black, 0, 0, 0, At<uint8_t>(self, 0x6B)));
    void* back = At<void*>(self, 4);
    float u = 0.5f / U2F(vc::Call<uint32_t>(back, 0x48));
    float v = 0.5f / U2F(vc::Call<uint32_t>(At<void*>(self, 4), 0x4C));
    float uv2[2] = {1.0f - u, 1.0f - v};
    float height = AreaHeight();
    float top = (480.0f - height) * 0.5f;
    float width = aco_screenScaleX * 480.0f;
    float left = (640.0f - width) * 0.5f;
    float uv1[2] = {u, v};
    ACO_DrawRectUV(width * 0.0f + left, (height * 0.0f + top) - 10.0f, width + left, (height + top) - 10.0f,
                   At<void*>(self, 4), self + 0x68, uv1, uv2);
    uint32_t start = 0;
    Runs(self, open, start, false, [](uint16_t cell) { return (cell & 0x100) != 0; },
         [&](uint32_t col, uint32_t from, uint32_t to) {
             ACO_Deck_DrawCells(self, col, from, to, At<void*>(self, 8), self + 0x60);
         });
    if (At<uint8_t>(self, 0x67) != 0) {
        Runs(self, open, start, false, [](uint16_t cell) { return (cell & 0x100) == 0; },
             [&](uint32_t col, uint32_t from, uint32_t to) {
                 ACO_Deck_DrawCells(self, col, from, to, At<void*>(self, 8), self + 0x64);
             });
    }
    // the line being drawn
    Runs(self, open, start, false, [](uint16_t cell) { return (uint8_t(cell) & 0x10) != 0; },
         [&](uint32_t col, uint32_t from, uint32_t to) { ACO_Deck_DrawVerticalLine(self, col, from, to, self + 0x5C); });
    Runs(self, open, start, true, [](uint16_t cell) { return (uint8_t(cell) & 0x20) != 0; },
         [&](uint32_t row, uint32_t from, uint32_t to) { ACO_Deck_DrawHorizontalLine(self, from, to, row, self + 0x5C); });
    // the lines: left edges, the right edge column, top edges, the bottom edge row
    Runs(self, open, start, false, [](uint16_t cell) { return (uint8_t(cell) & 0x11) == 1; },
         [&](uint32_t col, uint32_t from, uint32_t to) { ACO_Deck_DrawVerticalLine(self, col, from, to, self + 0x54); });
    uint32_t edge = At<uint32_t>(self, 0xC) - 1;
    for (uint32_t row = 0; row < At<uint32_t>(self, 0xC); row++) {
        uint8_t cell = uint8_t(At<uint16_t*>(self, 0x10)[At<uint32_t>(self, 0xC) * row + edge]);
        if ((cell & 0x44) == 4) {
            if (!open) {
                open = true;
                start = row;
            }
        } else if (open) {
            ACO_Deck_DrawVerticalLine(self, edge, start, row, self + 0x54);
            open = false;
        }
    }
    if (open) {
        ACO_Deck_DrawVerticalLine(self, At<uint32_t>(self, 0xC), start, At<uint32_t>(self, 0xC), self + 0x54);
    }
    open = false;
    Runs(self, open, start, true, [](uint16_t cell) { return (uint8_t(cell) & 0x22) == 2; },
         [&](uint32_t row, uint32_t from, uint32_t to) { ACO_Deck_DrawHorizontalLine(self, from, to, row, self + 0x54); });
    edge = At<uint32_t>(self, 0xC) - 1;
    for (uint32_t col = 0; col < At<uint32_t>(self, 0xC); col++) {
        uint8_t cell = uint8_t(At<uint16_t*>(self, 0x10)[At<uint32_t>(self, 0xC) * edge + col]);
        if ((cell & 0x88) == 8) {
            if (!open) {
                open = true;
                start = col;
            }
        } else if (open) {
            ACO_Deck_DrawHorizontalLine(self, start, col, edge, self + 0x54);
            open = false;
        }
    }
    if (open) {
        ACO_Deck_DrawHorizontalLine(self, start, At<uint32_t>(self, 0xC), At<uint32_t>(self, 0xC), self + 0x54);
    }
    float w = U2F(vc::Call<uint32_t>(At<void*>(self, 4), 0x48));
    float h = U2F(vc::Call<uint32_t>(At<void*>(self, 4), 0x4C));
    float v2 = 0.5f / h;
    float uv4[2] = {1.0f - 0.5f / w, 1.0f - v2};
    re::Ptr32<void>* textures = At<re::Ptr32<void>*>(aco_minigame, 0x1C);
    void* overlay = textures != nullptr ? textures[0x12] : nullptr;
    height = AreaHeight();
    top = (480.0f - height) * 0.5f;
    width = aco_screenScaleX * 480.0f;
    left = (640.0f - width) * 0.5f;
    float uv3[2] = {0.5f / w, v2};
    ACO_DrawRectUV(width * 0.0f + left, (height * 0.0f + top) - 10.0f, width + left, (height + top) - 10.0f, overlay,
                   self + 0x64, uv3, uv4);
}
ARTOBJ_REVERSED("ArtDeck::Draw", 0x006DD110, &ArtDeck_Draw);
