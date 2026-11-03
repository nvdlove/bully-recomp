// NiScreenElementsData::SetPolygonTextures(polygon, set, uvs): a polygon's texture coordinates in
// one set copied from an array (one NiPoint2 per vertex), the data's textures marked changed.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiScreenElementsData.h"
#include "reversed.h"

namespace ni {
namespace {

GAME_FN(float*, __thiscall, GeometryData_GetTextureSet, 0x0077A000, NiGeometryData* self, uint32_t set);
GAME_FN(void, __cdecl, NiD3DX_Memcpy, 0x0074D2A0, void* to, const void* from, uint32_t size);

} // namespace

// 0x789270
uint8_t __fastcall NiScreenElementsData_SetPolygonTextures(NiScreenElementsData* self, re::Edx, int32_t handle,
                                                           uint32_t set, const float* uvs) {
    if (self->textures == nullptr || handle < 0 || handle >= int32_t(self->maxPQuantity)) {
        return 0;
    }
    uint16_t index = self->polygonIndexer[handle];
    if (index == 0xFFFF || uvs == nullptr) {
        return 0;
    }
    NiScreenElementsPolygon* polygon = &self->polygons[index];
    if (polygon->numVertices == 0 || uint16_t(set) >= (self->dataFlags & 0x3F)) {
        return 0;
    }
    float* textures = GeometryData_GetTextureSet(self, set);
    NiD3DX_Memcpy(textures + uint32_t(polygon->vertexOffset) * 2, uvs, uint32_t(polygon->numVertices) * 8);
    self->dirtyFlags |= 8;
    return 1;
}

#define NISCREENDATA2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NISCREENDATA2_REVERSED("NiScreenElementsData::SetPolygonTextures", 0x00789270, NiScreenElementsData_SetPolygonTextures);

} // namespace ni
