// More of the pickups (see Pickups.cpp): GenerateNewOne and the ways to make one, the
// pickups in range (sorted), the colours file, and CMatrix's scale.

#include <cmath>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_ARRAY(uint8_t, ms_pickups, 0x00C0A100);   // 300 of 0x48
GAME_ARRAY(uint8_t, ms_colours, 0x00C0F590);   // r, g, b, a by colour
GAME_VAR(uint32_t, ms_timeInMs, 0x00C1A9B4);
GAME_VAR(uint32_t, ms_numInRange, 0x00C0A0A8);
GAME_ARRAY(int32_t, ms_inRange, 0x00C0A0B0);   // 20
GAME_ARRAY(void*, ms_list_C09960, 0x00C09960);
GAME_VAR(int32_t, ms_listCount_C09974, 0x00C09974);
GAME_VAR(void*, ms_pObj_C67554, 0x00C67554);
inline uint8_t* const ms_defaultButes = reinterpret_cast<uint8_t*>(0x00C0F560);   // PickupButes
inline void* const ms_random_CF356C = reinterpret_cast<void*>(0x00CF356C);

GAME_FN(uint8_t*, __cdecl, GetModelInfo, 0x0051B450, int model);
GAME_FN(void, __thiscall, ModelInfo_Fn_7282A0, 0x007282A0, uint8_t* mi);
GAME_FN(uint8_t, __cdecl, Pickups_RestoreCollected, 0x004438F0, uint8_t* p);
GAME_FN(int32_t, __cdecl, Pickups_GenerateNewOne, 0x00443C40, float x, float y, float z, int model, uint8_t* butes,
        int flag6, int value7, int flag8, int flag9);
GAME_FN(col::CVector*, __cdecl, VectorAdd, 0x004129A0, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(col::CVector*, __cdecl, VectorSub, 0x00414B70, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(float, __cdecl, GetHeading, 0x00433310, void* entity);
GAME_FN(float, __thiscall, Random_Float, 0x00577300, void* random);
GAME_FN(void, __thiscall, Fn_541320, 0x00541320, void* self, int32_t);
GAME_FN(void, __thiscall, FileBuffer_Load, 0x00426610, void* buffer, const char* path);
GAME_FN(const char*, __cdecl, FileBuffer_NextLine, 0x0042B980, void* buffer);
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(int, __cdecl, GetColourIndex, 0x005DAEA0, const char* name);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

using re::At;

uint8_t* Pickup(int i) {
    return ms_pickups + i * 0x48;
}

// the handle's slot (as GetActualPickupIndex, inlined; -1 when stale)
int HandleSlot(int32_t handle) {
    int slot = handle & 0xFFFF;
    if (handle == -1 || !(Pickup(slot)[0x46] & 8) || int16_t(uint32_t(handle) >> 16) != At<int16_t>(Pickup(slot), 0x40)) {
        return -1;
    }
    return slot;
}

// the 2D distance squared from the point to the pickup's position (+8)
float DistanceSq(const col::CVector* pos, uint8_t* p) {
    col::CVector d;
    VectorSub(&d, pos, reinterpret_cast<col::CVector*>(p + 8));
    return float(double(d.x) * d.x + double(d.y) * d.y);
}

} // namespace

namespace Pickups2 {

// 0x443B30: Config\Dat\colours.dat: "name r g b a" per line (# comments)
static void __cdecl LoadColours() {
    struct {
        re::Ptr32<void> data;
        re::Ptr32<const char> current;
        re::Ptr32<const char> end;
    } buffer = {nullptr, nullptr, nullptr};
    FileBuffer_Load(&buffer, "Config\\Dat\\colours.dat");
    while (buffer.current != buffer.end) {
        const char* line = FileBuffer_NextLine(&buffer);
        if (!line) {
            break;
        }
        char name[24];
        int r, g, b, a;
        if (*line != '\0' && *line != '#' && Sscanf(line, "%s %d %d %d %d", name, &r, &g, &b, &a) == 5) {
            int index = GetColourIndex(name);
            uint8_t rgba[4];
            uint8_t* c = RGBA_Construct(rgba, r, g, b, a);
            ms_colours[index * 4] = c[0];
            ms_colours[index * 4 + 1] = c[1];
            ms_colours[index * 4 + 2] = c[2];
            ms_colours[index * 4 + 3] = c[3];
        }
    }
    if (buffer.data) {
        Free(buffer.data);
    }
}

// 0x443C40: a new pickup (GTA's GenerateNewOne) in the first free slot: its butes (the
// model's, else the default), position (the model's height added), heading (the model's
// +0x40 degrees), flags; its regeneration time when its butes want one; its collected
// value back for kept ones. Its handle (-1: no free slot)
static int32_t __cdecl GenerateNewOne(float x, float y, float z, int model, uint8_t* butes, int flag6, int value7,
                                      int flag8, int flag9) {
    uint8_t* mi = GetModelInfo(model);
    if (!butes) {
        ModelInfo_Fn_7282A0(mi);
        if (At<uint8_t*>(mi, 0x88)) {
            ModelInfo_Fn_7282A0(mi);
            butes = At<uint8_t*>(mi, 0x88);
        } else {
            butes = ms_defaultButes;
        }
    }
    int slot = 0;
    while (slot < 300 && (Pickup(slot)[0x46] & 8)) {
        slot++;
    }
    if (slot >= 300) {
        return -1;
    }
    uint8_t* p = Pickup(slot);
    p[0x46] &= 0xFD;
    uint8_t flags = p[0x46];
    At<uint8_t*>(p, 0x20) = butes;
    At<uint32_t>(p, 0x24) = 0;
    p[0x46] = uint8_t(((butes[0x18] ^ flags) & 1) ^ flags);
    uint8_t keep = uint8_t(flag9);
    At<int32_t>(p, 0x30) = model;
    uint8_t six = uint8_t(flag6);
    At<int32_t>(p, 0x38) = 0;
    At<int32_t>(p, 0x18) = 0;
    p[0x44] = 1;
    uint8_t fresh = (keep == 0 && six == 0) ? 1 : 0;
    At<float>(p, 0x28) = 0.5f;
    At<float>(p, 0x2C) = 0.05f;
    uint8_t bits = uint8_t(((((keep & 1) << 1 | (six & 1)) << 2 | (fresh & 1) | 2) << 2) | (p[0x46] & 0xCB));
    At<int16_t>(p, 0x42) = -1;
    p[0x46] = bits;
    At<float>(p, 8) = x;
    At<float>(p, 0xC) = y;
    At<float>(p, 0x10) = At<float>(mi, 0x44) + z;
    At<int32_t>(p, 0x3C) = value7;
    p[0x45] = uint8_t(flag8);
    At<int32_t>(p, 0x1C) = 0;
    At<float>(p, 0x14) = float(double(At<int32_t>(mi, 0x40)) * 3.1415927410125732 / 180.0);
    bool restore;
    if (butes[0x20]) {
        if (six) {
            restore = false;
        } else {
            double now = double(ms_timeInMs);   // (unsigned)
            At<uint32_t>(p, 0x24) = uint32_t(int64_t(double(At<float>(butes, 0x24)) * 1000.0 + now));
            restore = true;
        }
    } else {
        restore = six == 0;
    }
    if (restore && keep && butes[0x18]) {
        Pickups_RestoreCollected(p);
    }
    uint16_t generation = At<uint16_t>(p, 0x40);
    At<uint16_t>(p, 0x40) = generation < 0xFFFE ? uint16_t(generation + 1) : uint16_t(1);
    int32_t handle = int32_t((uint32_t(At<uint16_t>(p, 0x40)) << 16) | uint32_t(slot));
    At<int32_t>(p, 0x34) = handle;
    return handle;
}

// 0x443EE0: a pickup (the model and butes) just above the entity (0.45), into its list
// (+0x1DA4, count +0x1DB0), bits 0 and 2 of its flags cleared; returns its handle
static int32_t __cdecl GenerateForEntity(uint8_t* entity, int model, uint8_t* butes) {
    col::CVector offset = {0.0f, 0.0f, 0.44999998807907104f};
    col::CVector* position = At<void*>(entity, 0x14) ? reinterpret_cast<col::CVector*>(At<uint8_t*>(entity, 0x14) + 0x30)
                                                       : reinterpret_cast<col::CVector*>(entity + 4);
    col::CVector pos;
    VectorAdd(&pos, position, &offset);
    int32_t handle = Pickups_GenerateNewOne(pos.x, pos.y, pos.z, model, butes, 0, 0, 0, 0);
    int slot = HandleSlot(handle);
    if (slot < 0) {
        return handle;
    }
    At<int32_t>(entity, 0x1DA4 + At<int32_t>(entity, 0x1DB0) * 4) = handle;
    At<int32_t>(entity, 0x1DB0)++;
    Pickup(slot)[0x46] &= 0xFA;
    return handle;
}

// 0x443FA0: a kept pickup (flag 9) with a value
static int32_t __cdecl GenerateKept(int value7, float x, float y, float z, int model, uint8_t* butes) {
    return Pickups_GenerateNewOne(x, y, z, model, butes, 0, value7, 0, 1);
}

// 0x443FE0: a pickup with flag 2 of its flags from `flag`, turned to the entity's heading
// plus up to half the spread either way (random)
static int32_t __cdecl GenerateFacing(float x, float y, float z, int model, int flag, uint8_t* butes, void* entity,
                                      int spread) {
    int32_t handle = Pickups_GenerateNewOne(x, y, z, model, butes, 0, 0, 0, 0);
    int slot = HandleSlot(handle);
    if (slot < 0) {
        return handle;
    }
    uint8_t* p = Pickup(slot);
    p[0x46] ^= (uint8_t(flag << 2) ^ p[0x46]) & 4;
    if (entity) {
        float heading = float(double(GetHeading(entity)) * 180.0 / 3.1415927410125732);
        float half = float(double(spread) * 0.5);
        float low = -half;
        float r = Random_Float(ms_random_CF356C);
        float angle = float((double(half) - low) * r + low);
        At<float>(p, 0x14) = float((double(angle) + heading) * 3.1415927410125732 / 180.0);
    }
    return handle;
}

// 0x4440F0: the pickups in use (with +0x44 set) within 2 in height and the radius (2D),
// up to 20 into 0xC0A0B0; sorted near first when asked. (The sort measures the pickups
// by their place in the list (pickup 0, 1, ...), not the listed ones, and doesn't update
// the compared distance after a swap)
static void __cdecl FindPickupsInRange(const col::CVector* pos, float radius, int sort) {
    uint32_t count = 0;
    ms_numInRange = 0;
    for (int i = 0; i < 300; i++) {
        uint8_t* p = Pickup(i);
        if (!(p[0x46] & 8) || p[0x44] != 1) {
            continue;
        }
        float dz = std::fabs(float(double(pos->z) - At<float>(p, 0x10)));
        if (dz > 2.0f) {
            continue;
        }
        col::CVector d;
        VectorSub(&d, pos, reinterpret_cast<col::CVector*>(p + 8));
        count = ms_numInRange;
        float lengthSq = float(double(d.x) * d.x + double(d.y) * d.y + 0.0);
        float diff = float(double(lengthSq) - double(radius) * radius);
        if (0.0f < diff || diff != diff) {
            continue;
        }
        ms_inRange[count] = i;
        count++;
        ms_numInRange = count;
        if (count == 20) {
            break;
        }
    }
    if (!uint8_t(sort) || count == 0) {
        return;
    }
    for (uint32_t k = 0; k < ms_numInRange; k++) {
        float dk = DistanceSq(pos, Pickup(int(k)));
        for (uint32_t j = k + 1; j < ms_numInRange; j++) {
            float dj = DistanceSq(pos, Pickup(int(j)));
            if (dk > dj) {
                int32_t t = ms_inRange[k];
                ms_inRange[k] = ms_inRange[j];
                ms_inRange[j] = t;
            }
        }
    }
}

// 0x4442A0: the object onto the list at 0xC09960 (5 at most, once), then 0x541320 with
// its +0x30
static void __cdecl AddToList(uint8_t* object) {
    int count = ms_listCount_C09974;
    for (int i = 0; i < count; i++) {
        if (object == ms_list_C09960[i]) {
            return;
        }
    }
    if (count >= 5) {
        return;
    }
    ms_list_C09960[count] = object;
    ms_listCount_C09974 = count + 1;
    Fn_541320(ms_pObj_C67554, At<int32_t>(object, 0x30));
}

// 0x4442F0 (thiscall): CMatrix's right, forward and up scaled
static void __fastcall Matrix_Scale(float* self, re::Edx, float scale) {
    self[0] = self[0] * scale;
    self[1] = self[1] * scale;
    self[2] = self[2] * scale;
    self[4] = self[4] * scale;
    self[5] = self[5] * scale;
    self[6] = self[6] * scale;
    self[8] = scale * self[8];
    self[9] = scale * self[9];
    self[10] = scale * self[10];
}

} // namespace Pickups2

// ---- registration ----------------------------------------------------------------------

#define PICKUPS2_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace Pickups2 {
PICKUPS2_REVERSED("CPickups::LoadColours", 0x00443B30, &LoadColours);
PICKUPS2_REVERSED("CPickups::GenerateNewOne", 0x00443C40, &GenerateNewOne);
PICKUPS2_REVERSED("CPickups::GenerateForEntity", 0x00443EE0, &GenerateForEntity);
PICKUPS2_REVERSED("CPickups::GenerateKept", 0x00443FA0, &GenerateKept);
PICKUPS2_REVERSED("CPickups::GenerateFacing", 0x00443FE0, &GenerateFacing);
PICKUPS2_REVERSED("CPickups::FindPickupsInRange", 0x004440F0, &FindPickupsInRange);
PICKUPS2_REVERSED("CPickups::AddToList", 0x004442A0, &AddToList);
PICKUPS2_REVERSED("CMatrix::Scale", 0x004442F0, &Matrix_Scale);
} // namespace Pickups2
