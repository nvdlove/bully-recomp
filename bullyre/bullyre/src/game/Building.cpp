// CBuilding (vtable 0x9131E4) and CTreadable (0x913274): the static world entities'
// constructor and destructor bodies.

#include <cstdint>

#include "game/Entity.h"
#include "reversed.h"

namespace c11building {

// 0x4CECA0: the destructor's body (CEntity's then)
void __fastcall Building_Destruct(CEntity* self, re::Edx edx);
// 0x4CECB0: the constructor: a building (type 1), uses collision, static, +0xC0 set
CEntity* __fastcall Building_Construct(CEntity* self, re::Edx edx);
// 0x4CEE00: the constructor
CEntity* __fastcall Treadable_Construct(CEntity* self, re::Edx edx);
// 0x4CEE20: the destructor's body (CBuilding's then)
void __fastcall Treadable_Destruct(CEntity* self, re::Edx edx);

} // namespace c11building

namespace {

GAME_FN(CEntity*, __thiscall, Entity_Construct, 0x004656A0, CEntity* self);
GAME_FN(void, __thiscall, Entity_Destruct, 0x00467E60, CEntity* self);
GAME_FN(CEntity*, __thiscall, Building_Construct_, 0x004CECB0, CEntity* self);
GAME_FN(void, __thiscall, Building_Destruct_, 0x004CECA0, CEntity* self);

inline re::Ptr32<void>* const buildingVtable = reinterpret_cast<re::Ptr32<void>*>(0x009131E4);
inline re::Ptr32<void>* const treadableVtable = reinterpret_cast<re::Ptr32<void>*>(0x00913274);

} // namespace

namespace c11building {

void __fastcall Building_Destruct(CEntity* self, re::Edx) {
    self->vtable = buildingVtable;
    Entity_Destruct(self);
}

CEntity* __fastcall Building_Construct(CEntity* self, re::Edx) {
    Entity_Construct(self);
    // (the type's bits 1 and 2 cleared and bit 0 set: type 1 when it was 0)
    uint8_t typeAndStatus = uint8_t((self->typeAndStatus & 0xF9) | 1);
    self->flagC0 = 1;
    self->bUsesCollision = 1;
    self->bIsStatic = 1;
    self->vtable = buildingVtable;
    self->typeAndStatus = typeAndStatus;
    return self;
}

CEntity* __fastcall Treadable_Construct(CEntity* self, re::Edx) {
    Building_Construct_(self);
    self->vtable = treadableVtable;
    return self;
}

void __fastcall Treadable_Destruct(CEntity* self, re::Edx) {
    self->vtable = treadableVtable;
    Building_Destruct_(self);
}

} // namespace c11building

// ---- registration ---------------------------------------------------------------------

#define BUILDING_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

BUILDING_REVERSED("CBuilding::Destruct", 0x004CECA0, &c11building::Building_Destruct);
BUILDING_REVERSED("CBuilding::Construct", 0x004CECB0, &c11building::Building_Construct);
BUILDING_REVERSED("CTreadable::Construct", 0x004CEE00, &c11building::Treadable_Construct);
BUILDING_REVERSED("CTreadable::Destruct", 0x004CEE20, &c11building::Treadable_Destruct);
