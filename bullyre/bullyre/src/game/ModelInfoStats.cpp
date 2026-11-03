// 0x51A7B0 to 0x51AF50: the memory stats dump (MemStats.txt: heaps, a few sizes, the
// model info stores' usage), CEntity's collision model, and two small model info helpers.

#include <cstdint>

#include "game/Pool.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_FN(void*, __cdecl, CH_FileOpen, 0x0042D260, const char* name, const char* mode, int flag);
GAME_FN(void, __cdecl, CH_FilePrintf, 0x0042D460, void* file, const char* format, ...);
GAME_FN(void, __cdecl, CH_FileClose, 0x0042D450, void* file);
GAME_FN(void, __cdecl, CPools_PrintUsage, 0x0044C330, void* file);
GAME_FN(uint8_t, __thiscall, Entity_Fn4EA360, 0x004EA360, void* entity);

GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, ms_exeSize, 0x00D1415C);
GAME_VAR(int32_t, ms_mainHeapSize, 0x00D14204);
GAME_VAR(int32_t, ms_simpleHeapSize, 0x00D142D4);
// the model info stores: pools, or fixed arrays with a count
GAME_VAR(CPool<uint8_t>, ms_simpleModelStore, 0x00A76AD4);
GAME_VAR(CPool<uint8_t>, ms_timeModelStore, 0x00A76AF0);
GAME_VAR(CPool<uint8_t>, ms_panimModelStore, 0x00A76B0C);
GAME_VAR(CPool<uint8_t>, ms_2dEffectStore, 0x00A76B28);
GAME_VAR(int32_t, ms_weaponModelCount, 0x00C735E0);
GAME_VAR(int32_t, ms_accessoryModelCount, 0x00C76978);
GAME_VAR(int32_t, ms_clumpModelCount, 0x00C77060);
GAME_VAR(int32_t, ms_vehicleModelCount, 0x00C771E0);
GAME_VAR(int32_t, ms_pedModelCount, 0x00C7ADE8);
GAME_VAR(int32_t, ms_pclothModelCount, 0x00C89EE8);

int UsedSlots(const CPool<uint8_t>& pool) {
    int used = 0;
    for (int i = 0; i < pool.size; i++) {
        if (!(pool.flags[i] & 0x80)) {
            used++;
        }
    }
    return used;
}

} // namespace

namespace ModelInfoStats {

// 0x51A7B0 (thiscall, 4 bytes): CWeaponModelInfo's vf8: +0x4C
[[maybe_unused]] static uint32_t __fastcall WeaponModelInfo_Get4C(uint8_t* self, re::Edx) {
    return *reinterpret_cast<uint32_t*>(self + 0x4C);
}
RE_UNHOOKED_FN("CWeaponModelInfo::Get4C", 0x0051A7B0, &WeaponModelInfo_Get4C);

// 0x51AB20 (thiscall): the object's RTTI (a jump to its vtable[2])
static void* __fastcall GetRTTIOf(void* self, re::Edx) {
    return vc::Call<void*>(self, 0x08);
}

// 0x51AB30: MemStats.txt: the heaps, some sizes and the model info stores, then the pools
static void __cdecl DumpMemStats() {
    void* file = CH_FileOpen("MemStats.txt", "w", 1);
    CH_FilePrintf(file, "TOTAL MEMORY USED:\r\n");
    CH_FilePrintf(file, "--------\r\n");
    CH_FilePrintf(file, "exe size\t%d\r\n", ms_exeSize);
    CH_FilePrintf(file, "main heap\t%d\r\n", ms_mainHeapSize);
    CH_FilePrintf(file, "simple heap\t%d\r\n", ms_simpleHeapSize);
    CH_FilePrintf(file, "total\t%d\r\n", ms_simpleHeapSize + ms_mainHeapSize + ms_exeSize);
    CH_FilePrintf(file, "--------\r\n");
    CH_FilePrintf(file, "SOME SIZES\r\n");
    CH_FilePrintf(file, "--------\r\n");
    CH_FilePrintf(file, "sizeof CPed=%d\r\n", 0x1E90);
    CH_FilePrintf(file, "sizeof CColModel\t%d\r\n", 0x40);
    CH_FilePrintf(file, "sizeof CBaseModelInfo\t%d\r\n", 0x20);
    CH_FilePrintf(file, "sizeof ObjectButes\t%d\r\n", 0xA4);
    CH_FilePrintf(file, "sizeof ProjectileButes\t%d\r\n", 0x158);
    CH_FilePrintf(file, "sizeof CStreamingInfo\t%d\r\n", 0x10);
    CH_FilePrintf(file, "--------\r\n");
    CH_FilePrintf(file, "POOL USAGE:\r\n");
    CH_FilePrintf(file, "--------\r\n");
    CH_FilePrintf(file, "Name\tMaxNum\tUsedNum\tElSize\tTotalSize\r\n");
    int size = ms_simpleModelStore.size;
    CH_FilePrintf(file, "ms_simpleModelStore Pool\t%d\t%d\t%d\t%d\r\n", size, UsedSlots(ms_simpleModelStore), 0x34,
                  size * 0x34);
    size = ms_timeModelStore.size;
    CH_FilePrintf(file, "ms_timeModelStore Pool\t%d\t%d\t%d\t%d\r\n", size, UsedSlots(ms_timeModelStore), 0x40,
                  size << 6);
    CH_FilePrintf(file, "ms_weaponModelStore Pool\t%d\t%d\t%d\t%d\r\n", 0x96, ms_weaponModelCount, 0x58, 0x3390);
    CH_FilePrintf(file, "ms_accessoryModelStore Pool\t%d\t%d\t%d\t%d\r\n", 0x15, ms_accessoryModelCount, 0x54, 0x6E4);
    CH_FilePrintf(file, "ms_clumpModelStore Pool\t%d\t%d\t%d\t%d\r\n", 5, ms_clumpModelCount, 0x4C, 0x17C);
    CH_FilePrintf(file, "ms_vehicleModelStore Pool\t%d\t%d\t%d\t%d\r\n", 0x20, ms_vehicleModelCount, 0x1E0, 0x3C00);
    CH_FilePrintf(file, "ms_pedModelStore Pool\t%d\t%d\t%d\t%d\r\n", 0x123, ms_pedModelCount, 0xD4, 0xF0FC);
    size = ms_2dEffectStore.size;
    CH_FilePrintf(file, "ms_2dEffectStore Pool\t%d\t%d\t%d\t%d\r\n", size, UsedSlots(ms_2dEffectStore), 0x40,
                  size << 6);
    size = ms_panimModelStore.size;
    CH_FilePrintf(file, "ms_panimModelStore Pool\t%d\t%d\t%d\t%d\r\n", size, UsedSlots(ms_panimModelStore), 0x94,
                  size * 0x94);
    CH_FilePrintf(file, "ms_pclothModelStore Pool\t%d\t%d\t%d\t%d\r\n", 1, ms_pclothModelCount, 0x4C, 0x4C);
    CPools_PrintUsage(file);
    CH_FilePrintf(file, "--------------------------------\r\n");
    CH_FileClose(file);
}

// 0x51AF50: an entity's collision model: its model info's (+0xC), or for an object
// (type 6) with 0x4EA360 set, the model info's second one (+0x80) when it has one
static void* __cdecl GetColModel(uint8_t* entity) {
    uint8_t* modelInfo;
    if ((entity[0x108] & 7) == 6 && Entity_Fn4EA360(entity)) {
        modelInfo = ms_modelInfoPtrs[*reinterpret_cast<int16_t*>(entity + 0x10E)];
        void* col = *reinterpret_cast<re::Ptr32<void>*>(modelInfo + 0x80);
        if (col) {
            return col;
        }
    }
    modelInfo = ms_modelInfoPtrs[*reinterpret_cast<int16_t*>(entity + 0x10E)];
    return *reinterpret_cast<re::Ptr32<void>*>(modelInfo + 0xC);
}

} // namespace ModelInfoStats

// ---- registration ----------------------------------------------------------------------

#define C16D_MIS_REVERSED(name, fn, address)                                                        \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace ModelInfoStats {
C16D_MIS_REVERSED("NiObject::GetRTTIOf_51AB20", GetRTTIOf, 0x0051AB20);
C16D_MIS_REVERSED("CModelInfo::DumpMemStats", DumpMemStats, 0x0051AB30);
C16D_MIS_REVERSED("CEntity::GetColModel", GetColModel, 0x0051AF50);
} // namespace ModelInfoStats
