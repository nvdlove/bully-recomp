// CCollectiblesMgr: the collectibles (their kinds, where they are, which are taken),
// with the game's tables of positions by kind; and the night-time helper next to it.
// 0x4261A0..0x4288C0.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
#define COLLECT_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {
struct CMVector {
    float x, y, z;
};
static_assert(sizeof(CMVector) == 0xC);

// A collectible: its kind (0..5), its name's hash, and where it is (from the kind's
// table, by the number at the end of its name)
struct CollectibleEntry {
    int32_t type;       // +0
    uint32_t nameHash;  // +4
    int32_t value;      // +8 (-1 when added)
    CMVector pos;       // +0xC
};
static_assert(sizeof(CollectibleEntry) == 0x18);

// Its state per collectible (2 bytes, kept in the save)
struct CollectibleState {
    uint8_t collected;  // +0
    uint8_t enabled;    // +1 (1 when added)
};
static_assert(sizeof(CollectibleState) == 2);

struct CCollectiblesMgr {
    re::Ptr32<re::Ptr32<void>> vtable;                  // +0 (0x901088)
    CollectibleState state[200];    // +4
    CollectibleEntry entries[200];  // +0x194
    int16_t countByType[7];         // +0x1454
    int16_t count;                  // +0x1462
    uint8_t unknown1464[0x320];     // +0x1464
    CMVector positions2[76];        // +0x1784 kind 2 ("%7s%2d" names)
    CMVector positions3[41];        // +0x1B14 kind 3 ("%8s%2d")
    CMVector positions1[7];         // +0x1D00 kind 1 ("%5s%2d")
    CMVector positions0[26];        // +0x1D54 kind 0 ("%5s%2d")
    uint32_t unknown1E8C;           // +0x1E8C
};
static_assert(offsetof(CCollectiblesMgr, state) == 0x4);
static_assert(offsetof(CCollectiblesMgr, entries) == 0x194);
static_assert(offsetof(CCollectiblesMgr, countByType) == 0x1454);
static_assert(offsetof(CCollectiblesMgr, count) == 0x1462);
static_assert(offsetof(CCollectiblesMgr, positions2) == 0x1784);
static_assert(offsetof(CCollectiblesMgr, positions3) == 0x1B14);
static_assert(offsetof(CCollectiblesMgr, positions1) == 0x1D00);
static_assert(offsetof(CCollectiblesMgr, positions0) == 0x1D54);
static_assert(offsetof(CCollectiblesMgr, unknown1E8C) == 0x1E8C);

// The collectibles' save block (0x21C bytes)
struct CollectiblesSave {
    int32_t version;                // +0 (1)
    int32_t size;                   // +4 (0x21C)
    int32_t count;                  // +8 (200)
    CollectibleState state[200];    // +0xC
    uint32_t padding[0x20];         // +0x19C (must be zero)
};
static_assert(offsetof(CollectiblesSave, padding) == 0x19C);
static_assert(sizeof(CollectiblesSave) == 0x21C);

GAME_VAR(uint8_t, cm_clockHour, 0x00BCEB4B);
GAME_VAR(int32_t, cm_limit0, 0x00BF4294);   // the numbers per kind (GetTotal)
GAME_VAR(int32_t, cm_limit1, 0x00BF4270);
GAME_VAR(int32_t, cm_limit2, 0x00BF4298);
GAME_VAR(int32_t, cm_limit3, 0x00BF426C);
GAME_VAR(int32_t, cm_limit4, 0x00BF4268);
GAME_VAR(int32_t, cm_limit5, 0x00BF4264);
GAME_FN(void, __cdecl, CM_SetGameClock, 0x00426070, int hours, int minutes);
GAME_FN(uint32_t, __cdecl, CM_HashName, 0x00576ED0, const char* name);
template <typename... A>
inline int __cdecl CM_sscanf(const char* str, const char* format, A... rest) { return crt::Sscanf(str, format, rest...); }
GAME_FN(void, __thiscall, CM_CopyState, 0x00733450, CollectibleState* dst, const CollectibleState* src);
GAME_FN(uint8_t, __cdecl, CM_Save_CheckValue, 0x00733320, int, int, int32_t value, int32_t expected);
GAME_FN(uint8_t, __cdecl, CM_Save_CheckBlock, 0x00733620, const char* name, void* data, int size);

// +0x1784, entries 1..75 (entry 0 stays zero)
const float kTable2[75][3] = {
    {533.039001f, 512.559998f, 19.6170006f},
    {420.390991f, 299.710999f, 9.38000011f},
    {447.949005f, 188.054993f, 9.17000008f},
    {487.535004f, 76.2360001f, 4.03999996f},
    {218.417007f, 256.220001f, 3.41499996f},
    {549.804993f, -17.7460003f, 11.1560001f},
    {648.263977f, -87.612999f, 28.6760006f},
    {529.229004f, -160.921997f, 15.9370003f},
    {525.049011f, -273.835999f, 2.30900002f},
    {429.888f, -462.773987f, 2.99000001f},
    {236.595001f, -286.584015f, 3.56699991f},
    {69.4079971f, -400.446014f, 1.04499996f},
    {551.400024f, -251.600006f, 7.54500008f},
    {256.730988f, 372.792999f, 20.0179996f},
    {343.399994f, -63.5f, 4.30000019f},
    {166.539001f, 378.243988f, 1.56599998f},
    {149.891006f, 88.0439987f, 2.5079999f},
    {228.792007f, -363.429993f, 2.671f},
    {-65.9000015f, -325.200012f, 4.19999981f},
    {162.953995f, -418.240997f, 2.61500001f},
    {585.39502f, -476.786987f, 4.48999977f},
    {309.403015f, 374.06601f, 24.6000004f},
    {567.169006f, 385.85199f, 14.8000002f},
    {572.46698f, -133.529999f, 5.87200022f},
    {39.3660011f, 26.0809994f, 6.12099981f},
    {-2.70900011f, -371.505005f, 7.76900005f},
    {193.233002f, -467.992004f, 9.375f},
    {32.0124016f, 297.424011f, 4.06899977f},
    {172.229996f, -81.8399963f, 12.75f},
    {323.471008f, -340.510986f, 3.39899993f},
    {438.799988f, 384.600006f, 17.2350006f},
    {475.600006f, 273.399994f, 20.0550003f},
    {599.099976f, -89.3000031f, 6.0f},
    {487.0f, -74.3000031f, 5.42000008f},
    {628.599976f, 203.300003f, 18.5f},
    {337.299988f, 239.399994f, 6.96000004f},
    {401.200012f, 119.800003f, 5.375f},
    {418.269989f, 154.979996f, 11.4399996f},
    {429.609985f, 173.419998f, 6.73999977f},
    {193.970001f, -73.4100037f, 6.76999998f},
    {492.399994f, -277.899994f, 2.5f},
    {257.475006f, -414.011993f, 2.72000003f},
    {305.523987f, -231.610001f, 0.239999995f},
    {521.780029f, -405.488007f, 2.27999997f},
    {170.830002f, -69.1500015f, 6.8499999f},
    {431.635986f, 550.239014f, 24.9710007f},
    {6.93654013f, 0.581117988f, 7.04099989f},
    {509.839996f, -58.8199997f, 3.38000011f},
    {290.200012f, -131.5f, 5.94999981f},
    {210.399994f, -42.2999992f, 8.69499969f},
    {-28.2999992f, -21.7999992f, 2.02999997f},
    {55.9980011f, -106.086998f, 7.13899994f},
    {148.300003f, -153.300003f, 7.30000019f},
    {247.5f, -15.3000002f, 6.26000023f},
    {119.469002f, -15.3663998f, 7.26000023f},
    {48.2999992f, -49.2000008f, 5.0999999f},
    {247.699997f, -124.800003f, 6.03999996f},
    {101.800003f, -74.5f, 7.5999999f},
    {23.8799992f, -84.4100037f, 12.1999998f},
    {165.600006f, -17.0f, 6.40999985f},
    {173.300003f, 11.5f, 6.0999999f},
    {45.7999992f, -224.800003f, 2.63499999f},
    {334.700012f, 325.0f, 13.1499996f},
    {229.119995f, 450.76001f, 3.63000011f},
    {260.5f, -436.299988f, 3.64499998f},
    {629.700012f, -63.5f, 9.09500027f},
    {523.299988f, 195.899994f, 16.5450001f},
    {181.440002f, -74.3499985f, 4.23000002f},
    {194.309998f, -171.820007f, 7.57999992f},
    {279.709991f, -15.8299999f, 7.86999989f},
    {252.919998f, -18.4099998f, 8.18999958f},
    {587.599976f, 454.100006f, 18.8199997f},
    {488.100006f, 492.600006f, 21.4050007f},
    {214.600006f, -403.399994f, 2.78500009f},
    {-9.10000038f, -224.899994f, 2.1500001f},
};
// +0x1B14, entries 1..40 (entry 0 stays zero)
const float kTable3[40][3] = {
    {165.807007f, -77.4219971f, 14.8170004f},
    {507.937988f, -58.4420013f, 2.42000008f},
    {34.7000008f, -150.300003f, 3.0999999f},
    {165.360001f, -346.764008f, 3.00200009f},
    {173.839996f, -181.149994f, 6.96000004f},
    {70.2509995f, -102.0f, 7.04899979f},
    {142.789993f, -354.940002f, 4.15999985f},
    {31.2416992f, 188.951004f, 2.35931993f},
    {253.789993f, -22.5100002f, 7.59000015f},
    {41.3600006f, -75.5f, 2.61999989f},
    {448.502014f, -205.207001f, 3.34699988f},
    {532.588013f, -115.388f, 5.5f},
    {110.791f, 5.73600006f, 6.125f},
    {233.300003f, 443.399994f, 4.5999999f},
    {559.909973f, -307.850006f, 3.78999996f},
    {446.200012f, -438.200012f, 3.95000005f},
    {253.899994f, -339.299988f, 2.7650001f},
    {283.125f, -371.803009f, 2.73000002f},
    {445.897003f, -247.0f, 5.00263977f},
    {592.0f, -122.199997f, 11.2849998f},
    {167.690994f, 461.325012f, 6.70998001f},
    {417.200012f, 315.100006f, 12.2849998f},
    {433.600006f, 480.600006f, 23.3349991f},
    {352.100006f, 206.0f, 4.7249999f},
    {501.0f, -438.5f, 6.80000019f},
    {561.099976f, -161.300003f, 7.44500017f},
    {513.929993f, -59.8199997f, 6.42000008f},
    {-27.0f, -248.699997f, 5.78499985f},
    {202.0f, 16.7000008f, 5.82000017f},
    {279.800995f, -469.596985f, 4.03000021f},
    {168.75f, 434.200012f, 5.96999979f},
    {229.899994f, 302.700012f, 1.40999997f},
    {326.299988f, 100.300003f, 4.5f},
    {120.599998f, -401.0f, 7.69999981f},
    {121.900002f, -440.5f, 3.0999999f},
    {507.799988f, -24.8999996f, 6.46000004f},
    {583.0f, -388.399994f, 5.43499994f},
    {476.299988f, -217.100006f, 7.13999987f},
    {206.0f, 172.5f, 0.524999976f},
    {617.799988f, 160.699997f, 19.9349995f},
};
// +0x1D00, entries 1..6 (entry 0 stays zero)
const float kTable1[6][3] = {
    {648.737f, 83.6541977f, 8.5710001f},
    {582.599976f, -472.899994f, 4.46500015f},
    {489.100006f, -111.099998f, 5.03999996f},
    {197.639999f, -84.5100021f, 2.3900001f},
    {512.0f, -57.1699982f, 9.18999958f},
    {123.450996f, -9.05848026f, 12.6575003f},
};
// +0x1D54, entries 1..25 (entry 0 stays zero)
const float kTable0[25][3] = {
    {604.096008f, -115.375999f, 5.89699984f},
    {619.598999f, 205.569f, 18.4650002f},
    {478.42099f, 480.311005f, 19.7080002f},
    {463.187012f, 487.063995f, 22.8899994f},
    {344.334991f, -357.714996f, 2.67700005f},
    {554.492004f, 377.261993f, 16.9979992f},
    {532.137024f, 270.944f, 16.8670006f},
    {366.282013f, 458.851013f, 23.5189991f},
    {143.462997f, -131.531006f, 6.82000017f},
    {572.137024f, 477.838989f, 18.8250008f},
    {438.64801f, 536.890015f, 23.5930004f},
    {308.757996f, 402.403015f, 24.7859993f},
    {479.63501f, 383.763f, 15.9969997f},
    {479.269989f, 382.295013f, 16.0209999f},
    {-110.144997f, -345.972992f, 4.49700022f},
    {537.031006f, 446.643005f, 18.0160007f},
    {65.3209991f, 225.602997f, 3.1170001f},
    {603.55603f, 383.165009f, 16.4430008f},
    {346.408997f, -231.768997f, 2.39199996f},
    {406.130005f, 395.838013f, 21.3260002f},
    {346.661011f, 385.90799f, 21.6459999f},
    {528.143005f, 174.455002f, 16.2490005f},
    {484.464996f, 417.459991f, 17.0410004f},
    {495.412994f, 431.473999f, 17.3990002f},
    {442.299988f, 341.600006f, 16.7590008f},
};

uint8_t* Bytes(CCollectiblesMgr* self) {
    return reinterpret_cast<uint8_t*>(self);
}
} // namespace

namespace CCollectibles {

// 0x4261A0: in the day (6:00..18:59), the clock to 19:05
void __cdecl SetClockToEvening() {
    if (uint8_t(cm_clockHour - 6) <= 12) {
        CM_SetGameClock(0x13, 5);
    }
}

// 0x4261C0 (thiscall): none added
void __fastcall Clear(CCollectiblesMgr* self, re::Edx) {
    self->count = 0;
}

// 0x4261D0 (thiscall): one more, of the kind, named: not taken, enabled, where the
// kind's table says for the number the name ends in. (The number stays -1 when the
// name doesn't have one: the table's entry before the first is read; a name part
// longer than the buffer overflows it, as in the game)
void __fastcall Add(CCollectiblesMgr* self, re::Edx, int32_t type, const char* name) {
    int index = self->count;
    CollectibleEntry* entry = &self->entries[index];
    self->state[index].collected = 0;
    self->state[index].enabled = 1;
    entry->type = type;
    entry->value = -1;
    entry->nameHash = CM_HashName(name);
    char prefix[16];
    int number = -1;
    CMVector* table;
    if (entry->type == 2) {
        CM_sscanf(name, "%7s%2d", prefix, &number);
        table = reinterpret_cast<CMVector*>(Bytes(self) + 0x1784) + number;
    } else if (entry->type == 3) {
        CM_sscanf(name, "%8s%2d", prefix, &number);
        table = reinterpret_cast<CMVector*>(Bytes(self) + 0x1B14) + number;
    } else if (entry->type == 1) {
        CM_sscanf(name, "%5s%2d", prefix, &number);
        table = reinterpret_cast<CMVector*>(Bytes(self) + 0x1D00) + number;
    } else if (entry->type == 0) {
        CM_sscanf(name, "%5s%2d", prefix, &number);
        table = reinterpret_cast<CMVector*>(Bytes(self) + 0x1D54) + number;
    } else {
        table = nullptr;
    }
    if (table) {
        entry->pos = *table;
    }
    reinterpret_cast<int16_t*>(Bytes(self) + 0x1454)[type]++;
    self->count++;
}

// 0x426310 (thiscall): where the collectible is
CMVector* __fastcall GetPosition(CCollectiblesMgr* self, re::Edx, CMVector* out, int16_t index) {
    *out = self->entries[index].pos;
    return out;
}

// 0x426340 / 0x426350 (thiscall)
uint8_t __fastcall IsCollected(CCollectiblesMgr* self, re::Edx, int16_t index) {
    return self->state[index].collected;
}
uint8_t __fastcall IsEnabled(CCollectiblesMgr* self, re::Edx, int16_t index) {
    return self->state[index].enabled;
}

// 0x426360 / 0x426380 / 0x4263A0 (thiscall): of kind 0, 4, 5
uint8_t __fastcall IsType0(CCollectiblesMgr* self, re::Edx, int16_t index) {
    return self->entries[index].type == 0;
}
uint8_t __fastcall IsType4(CCollectiblesMgr* self, re::Edx, int16_t index) {
    return self->entries[index].type == 4;
}
uint8_t __fastcall IsType5(CCollectiblesMgr* self, re::Edx, int16_t index) {
    return self->entries[index].type == 5;
}

// 0x4263C0 (thiscall): the one with the name's hash; none: 0xFFFF (only the low word
// set over the count)
int32_t __fastcall FindByHash(CCollectiblesMgr* self, re::Edx, uint32_t hash) {
    int count = self->count;
    int i = 0;
    for (; i < count; i++) {
        if (self->entries[i].nameHash == hash) {
            return i;
        }
    }
    return int32_t((uint32_t(i) & 0xFFFF0000) | 0xFFFF);
}

// 0x4263F0 / 0x426410 (thiscall)
int32_t __fastcall GetValue(CCollectiblesMgr* self, re::Edx, int16_t index) {
    return self->entries[index].value;
}
void __fastcall SetValue(CCollectiblesMgr* self, re::Edx, int16_t index, int32_t value) {
    self->entries[index].value = value;
}

// 0x426430 (thiscall): how many of the kind were added (in ax; eax's high word is the
// argument's)
uint32_t __fastcall GetCountOfType(CCollectiblesMgr* self, re::Edx, int32_t type) {
    uint16_t count = uint16_t(reinterpret_cast<int16_t*>(Bytes(self) + 0x1454)[type]);
    return (uint32_t(type) & 0xFFFF0000) | count;
}

// 0x426440 (thiscall): how many of the kind are taken
int32_t __fastcall CountCollected(CCollectiblesMgr* self, re::Edx, int32_t type) {
    int found = 0;
    int count = self->count;
    for (int i = 0; i < count; i++) {
        if (self->entries[i].type == type && self->state[int16_t(i)].collected) {
            found++;
        }
    }
    return found;
}

// 0x426490 (thiscall): every one of the kind enabled or not
void __fastcall SetTypeEnabled(CCollectiblesMgr* self, re::Edx, int32_t type, int enabled) {
    for (int i = 0; i < self->count; i++) {
        if (self->entries[i].type == type) {
            self->state[i].enabled = uint8_t(enabled);
        }
    }
}

// 0x4264E0 (thiscall): into the save block
void __fastcall Save(CCollectiblesMgr* self, re::Edx, CollectiblesSave* save) {
    save->version = 1;
    save->size = 0x21C;
    save->count = 200;
    for (int i = 0; i < 200; i++) {
        CM_CopyState(&save->state[i], &self->state[i]);
    }
}

// 0x426520 (thiscall): from the save block, if its header and padding check out
void __fastcall Load(CCollectiblesMgr* self, re::Edx, CollectiblesSave* save) {
    if (!CM_Save_CheckValue(0, 0, save->version, 1) || !CM_Save_CheckValue(0, 0, save->size, 0x21C) ||
        !CM_Save_CheckValue(0, 0, save->count, 200) || !CM_Save_CheckBlock("CollectiblesSaveData", save->padding, 0x20)) {
        return;
    }
    for (int i = 0; i < 200; i++) {
        self->state[i].collected = save->state[i].collected;
        self->state[i].enabled = save->state[i].enabled;
    }
}

// 0x4265B0 (ret 4, ecx unused): the number of the kind there are (-1: no such kind)
int32_t __fastcall GetTotal(void* /*ecx*/, re::Edx, int32_t type) {
    switch (type) {
    case 0:
        return cm_limit0;
    case 1:
        return cm_limit1;
    case 2:
        return cm_limit2;
    case 3:
        return cm_limit3;
    case 4:
        return cm_limit4;
    case 5:
        return cm_limit5;
    default:
        return -1;
    }
}

// 0x4266B0 (thiscall): the constructor: every position zero, then the kinds' tables
// filled (their entry 0 left zero)
CCollectiblesMgr* __fastcall Construct(CCollectiblesMgr* self, re::Edx) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00901088);
    for (int i = 0; i < 200; i++) {
        self->entries[i].pos = {0.0f, 0.0f, 0.0f};
    }
    for (int i = 0; i < 76; i++) {
        self->positions2[i] = {0.0f, 0.0f, 0.0f};
    }
    for (int i = 0; i < 41; i++) {
        self->positions3[i] = {0.0f, 0.0f, 0.0f};
    }
    for (int i = 0; i < 7; i++) {
        self->positions1[i] = {0.0f, 0.0f, 0.0f};
    }
    for (int i = 0; i < 26; i++) {
        self->positions0[i] = {0.0f, 0.0f, 0.0f};
    }
    self->unknown1E8C = 0;
    for (int i = 0; i < 75; i++) {
        self->positions2[i + 1] = {kTable2[i][0], kTable2[i][1], kTable2[i][2]};
    }
    for (int i = 0; i < 40; i++) {
        self->positions3[i + 1] = {kTable3[i][0], kTable3[i][1], kTable3[i][2]};
    }
    for (int i = 0; i < 6; i++) {
        self->positions1[i + 1] = {kTable1[i][0], kTable1[i][1], kTable1[i][2]};
    }
    for (int i = 0; i < 25; i++) {
        self->positions0[i + 1] = {kTable0[i][0], kTable0[i][1], kTable0[i][2]};
    }
    return self;
}

COLLECT_REVERSED("CCollectiblesMgr::SetClockToEvening", 0x004261A0, &SetClockToEvening);
COLLECT_REVERSED("CCollectiblesMgr::Clear", 0x004261C0, &Clear);
COLLECT_REVERSED("CCollectiblesMgr::Add", 0x004261D0, &Add);
COLLECT_REVERSED("CCollectiblesMgr::GetPosition", 0x00426310, &GetPosition);
COLLECT_REVERSED("CCollectiblesMgr::IsCollected", 0x00426340, &IsCollected);
COLLECT_REVERSED("CCollectiblesMgr::IsEnabled", 0x00426350, &IsEnabled);
COLLECT_REVERSED("CCollectiblesMgr::IsType0", 0x00426360, &IsType0);
COLLECT_REVERSED("CCollectiblesMgr::IsType4", 0x00426380, &IsType4);
COLLECT_REVERSED("CCollectiblesMgr::IsType5", 0x004263A0, &IsType5);
COLLECT_REVERSED("CCollectiblesMgr::FindByHash", 0x004263C0, &FindByHash);
COLLECT_REVERSED("CCollectiblesMgr::GetValue", 0x004263F0, &GetValue);
COLLECT_REVERSED("CCollectiblesMgr::SetValue", 0x00426410, &SetValue);
COLLECT_REVERSED("CCollectiblesMgr::GetCountOfType", 0x00426430, &GetCountOfType);
COLLECT_REVERSED("CCollectiblesMgr::CountCollected", 0x00426440, &CountCollected);
COLLECT_REVERSED("CCollectiblesMgr::SetTypeEnabled", 0x00426490, &SetTypeEnabled);
COLLECT_REVERSED("CCollectiblesMgr::Save", 0x004264E0, &Save);
COLLECT_REVERSED("CCollectiblesMgr::Load", 0x00426520, &Load);
COLLECT_REVERSED("CCollectiblesMgr::GetTotal", 0x004265B0, &GetTotal);
COLLECT_REVERSED("CCollectiblesMgr::Construct", 0x004266B0, &Construct);

} // namespace CCollectibles
