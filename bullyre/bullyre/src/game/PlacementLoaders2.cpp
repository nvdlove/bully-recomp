// More of the placement file reader (see PlacementLoaders.cpp): the rail loader's first
// record, and CIplStore's LoadIpb thunk.

#include <cstdint>

#include "game/Collision.h"
#include "game/Pool.h"
#include "reversed.h"

namespace {

GAME_VAR(CPool<uint8_t>*, ms_pTriggerPool, 0x020C7C48);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(void*, ms_sharedColModel, 0x00CF3120);   // not a model's own

GAME_FN(void*, __cdecl, PropAnim_New, 0x004EB180, size_t size);
GAME_FN(uint8_t*, __thiscall, PropAnim_Construct, 0x004EDB20, void* prop, int model, void* trigger);
GAME_FN(void, __thiscall, PropAnim_SetFlag, 0x004EA450, uint8_t* prop, int flag, int on);
GAME_FN(void, __cdecl, IplStore_IncludeEntity, 0x004384B0, int slot, uint8_t* entity);
GAME_FN(void, __thiscall, ModelInfo_SetColModel, 0x0050EB30, uint8_t* mi, void* col, int owns);
GAME_FN(uint8_t*, __cdecl, GetColModel, 0x0051AF50, uint8_t* entity);
GAME_FN(col::CRect*, __cdecl, ColStore_GetBoundingBox, 0x0056F0F0, int slot);
GAME_FN(void, __thiscall, Rect_Extend, 0x00414410, col::CRect* rect, const col::CRect* other);

constexpr int MI_RAIL = 0x2762;

using re::At;

} // namespace

// 0x439760: a lone jmp to CIplStore::LoadIpb (0x4390A0)
RE_UNHOOKED("CIplStore::LoadIpb_thunk", 0x00439760);

namespace IplLoaders2 {

// RailLoaderImpl vf0 (0x438F50): the rail's prop (model 0x2762; "BLNC..." names are
// balance beams: flag 0xC) for the file's slot (+0xC), into +4; its collision's .col
// area grown by its bounding rectangle. (The prop gets the trigger pool's first slot)
static void __fastcall Rail_Begin(void* self, re::Edx, const char* name, int, int flag, int) {
    uint8_t* prop = nullptr;
    if (void* memory = PropAnim_New(0x3B8)) {
        CPool<uint8_t>* pool = ms_pTriggerPool;
        void* trigger = (pool->flags[0] & 0x80) ? nullptr : pool->objects;
        prop = PropAnim_Construct(memory, MI_RAIL, trigger);
    }
    At<uint8_t*>(self, 4) = prop;
    size_t length = 0;
    while (name[length]) {
        length++;
    }
    int balance = length > 4 && name[0] == 'B' && name[1] == 'L' && name[2] == 'N' && name[3] == 'C';
    PropAnim_SetFlag(prop, 0xC, balance);
    At<uint8_t*>(self, 4)[0x110] = At<uint8_t>(self, 0xC);
    IplStore_IncludeEntity(At<int32_t>(self, 0xC), At<uint8_t*>(self, 4));
    At<uint8_t*>(self, 4)[0x111] = uint8_t(flag);
    At<uint8_t*>(self, 4)[0x3A8] = 0;
    uint8_t* entity = At<uint8_t*>(self, 4);
    reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[9]))(entity, MI_RAIL);   // SetModelIndex
    uint8_t* mi = ms_modelInfoPtrs[MI_RAIL];
    if (At<void*>(mi, 0xC) == nullptr) {
        ModelInfo_SetColModel(mi, ms_sharedColModel, 0);
    }
    uint16_t colSlot = At<uint16_t>(GetColModel(At<uint8_t*>(self, 4)), 0x30);
    entity = At<uint8_t*>(self, 4);
    col::CRect bounds;
    col::CRect* rect = reinterpret_cast<col::CRect*(__thiscall*)(void*, col::CRect*)>(
        static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[11]))(entity, &bounds);   // GetBoundRect
    Rect_Extend(ColStore_GetBoundingBox(colSlot), rect);
}

} // namespace IplLoaders2

// ---- registration ----------------------------------------------------------------------

#define LOADERS2_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace IplLoaders2 {
LOADERS2_REVERSED("RailLoaderImpl::vf0", 0x00438F50, &Rail_Begin);
} // namespace IplLoaders2
