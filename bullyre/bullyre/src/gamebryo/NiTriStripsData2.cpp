// NiTriStripsData::CalculateNormals (slot 20): each strip's triangles (alternate ones wound the
// other way, degenerate ones skipped) add their unit face normal to their three vertices' normals,
// which are then unitized. The strips: their count +0x44, lengths +0x48, the indices +0x4C (all
// the strips' one after another).

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometryData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

// NiGeometryData::CreateNormals(initialize): the normals allocated when missing, zeroed when asked
GAME_FN(void, __thiscall, GeometryData_CreateNormals, 0x00779F90, NiGeometryData* self, int initialize);
// NiPoint3::UnitizeVector (the table-driven square root) and UnitizeVectors(points, count, stride)
GAME_FN(void, __cdecl, Point3_UnitizeVector, 0x007D56A0, NiPoint3* point);
GAME_FN(void, __cdecl, Point3_UnitizeVectors, 0x00770B10, NiPoint3* points, uint32_t count, uint32_t stride);

void __fastcall TriStripsDataCalculateNormals(NiGeometryData* self, re::Edx) {
    GeometryData_CreateNormals(self, 1);
    const uint16_t* strip = At<uint16_t*>(self, 0x4C);
    for (uint16_t s = 0; s < At<uint16_t>(self, 0x44); s++) {
        const uint16_t* lengths = At<uint16_t*>(self, 0x48);
        for (uint16_t j = 2; j < lengths[s]; j++) {
            uint16_t v0 = strip[j - 2];
            uint16_t v1;
            uint16_t v2;
            if ((j & 1) == 0) {
                v1 = strip[j - 1];
                v2 = strip[j];
            } else {
                v1 = strip[j];
                v2 = strip[j - 1];
            }
            if (v0 == v1 || v1 == v2 || v2 == v0) {
                continue;
            }
            const NiPoint3* p = self->vertices;
            NiPoint3 e1 = {p[v1].x - p[v0].x, p[v1].y - p[v0].y, p[v1].z - p[v0].z};
            NiPoint3 e2 = {p[v2].x - p[v1].x, p[v2].y - p[v1].y, p[v2].z - p[v1].z};
            NiPoint3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
            Point3_UnitizeVector(&n);
            NiPoint3* a = &self->normals[v0];
            a->x = a->x + n.x;
            a->y = a->y + n.y;
            a->z = a->z + n.z;
            NiPoint3* b = &self->normals[v1];
            b->x = b->x + n.x;
            b->y = b->y + n.y;
            b->z = b->z + n.z;
            NiPoint3* c = &self->normals[v2];
            c->x = c->x + n.x;
            c->y = c->y + n.y;
            c->z = c->z + n.z;
        }
        strip += At<uint16_t*>(self, 0x48)[s];
    }
    Point3_UnitizeVectors(self->normals, self->vertexCount, 0xC);
    self->dirtyFlags |= 2;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NITRISTRIPS2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NITRISTRIPS2_REVERSED("NiTriStripsData::CalculateNormals", 0x007F76F0, ni::TriStripsDataCalculateNormals);
