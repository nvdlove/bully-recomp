// A collision model given its collision data: a new CCollisionData, the one being built
// moved into it (and cleared). 0x42AA30.

#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace {

GAME_FN(void*, __cdecl, CD2_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(CCollisionData*, __thiscall, CD2_Construct, 0x0056DAD0, CCollisionData* data);

} // namespace

namespace Collision2 {

// 0x42AA30 (the data to move in esi, the model on the stack): the model's +0x38 a new
// CCollisionData with `from`'s volumes; `from` emptied. (A null allocation is written
// through, as the game does)
void __cdecl SetCollisionDataFrom(uint8_t* model, CCollisionData* from) {
    void* memory = CD2_Malloc(sizeof(CCollisionData));
    CCollisionData* data = memory ? CD2_Construct(static_cast<CCollisionData*>(memory)) : nullptr;
    *reinterpret_cast<re::Ptr32<CCollisionData>*>(model + 0x38) = data;
    data->spheres = from->spheres;
    data->unknown04 = from->unknown04;
    data->boxes = from->boxes;
    data->vertices = from->vertices;
    data->triangles = from->triangles;
    data->planes = from->planes;
    data->unknown18 = from->unknown18;
    data->numTriangles = from->numTriangles;
    data->numSpheres = from->numSpheres;
    data->numBoxes = from->numBoxes;
    data->unknown20 = from->unknown20;
    from->spheres = nullptr;
    from->unknown04 = nullptr;
    from->boxes = nullptr;
    from->vertices = nullptr;
    from->triangles = nullptr;
    from->planes = nullptr;
    from->unknown18 = nullptr;
    from->numTriangles = 0;
    from->numSpheres = 0;
    from->numBoxes = 0;
    from->unknown20 = 0;
}
// (esi carries an argument: not hookable)
RE_UNHOOKED_FN("SetCollisionDataFrom", 0x0042AA30, &SetCollisionDataFrom);

} // namespace Collision2
