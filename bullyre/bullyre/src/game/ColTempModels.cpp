// CTempColModels (0x571440): the built-in collision models at 0xCF3120..0xCF3158 (the
// empty one, the ped shapes as spheres, the ped whose spheres follow its bones, a bike,
// a ball, and door/bumper boxes), and 0x5727D0, which moves the animated ped model's six
// spheres to a ped's bones (ped-relative, turned by its heading). Also the two matrix
// helpers those use (0x570F90, 0x5713D0).

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

// A box as CColBox::Set fills it (0x24): min, max (16-byte vectors), surface, piece
struct TempColBox {
    CVector min;
    float unknown0C;
    CVector max;
    float unknown1C;
    uint8_t surface;
    uint8_t piece;
    uint8_t pad[2];
};
static_assert(sizeof(TempColBox) == 0x24);

// A line (0x20): start and end as 16-byte vectors
struct TempColLine {
    CVector start;
    float unknown0C;
    CVector end;
    float unknown1C;
};
static_assert(sizeof(TempColLine) == 0x20);

struct Vector4 {
    float x, y, z, w;
};

GAME_FN(CColModel*, __cdecl, TCM_ColModelNew, 0x0056E7C0, size_t size);
GAME_FN(CColModel*, __thiscall, TCM_ColModelConstruct, 0x0056E7A0, CColModel* model);
GAME_FN(CCollisionData*, __thiscall, TCM_CollisionDataConstruct, 0x0056DAD0, CCollisionData* data);
GAME_FN(void, __thiscall, TCM_SphereSet, 0x0056EB80, void* sphere, float radius, const CVector* center);
GAME_FN(void, __thiscall, TCM_BoxSetMinMax, 0x00564830, void* box, const CVector* min, const CVector* max);
GAME_FN(void, __thiscall, TCM_BoxSet, 0x005648A0, TempColBox* box, const CVector* min, const CVector* max, uint8_t surface, uint8_t piece);
GAME_FN(void, __thiscall, TCM_LineSet, 0x00564920, TempColLine* line, const CVector* start, const CVector* end);
GAME_FN(void*, __cdecl, TCM_AllocArray, 0x005EEA50, size_t size, int);
GAME_FN(void*, __cdecl, TCM_Malloc, 0x005EEAA0, size_t size);
GAME_FN(CVector*, __cdecl, TCM_VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
// the ped bits (0x5727D0)
GAME_FN(void, __thiscall, TCM_NameCopy, 0x005FA880, uint32_t* dst, const uint32_t* src);
GAME_FN(void, __thiscall, TCM_NameSet, 0x005FA870, uint32_t* dst, uint32_t value);
GAME_FN(void, __thiscall, TCM_PathBuild, 0x005FA8C0, void* path, int count, const uint32_t* names);
GAME_FN(uint8_t*, __cdecl, TCM_ButesFind, 0x0072AEA0, const void* path, const void* root);
GAME_FN(void, __thiscall, TCM_NameDestruct, 0x0049A650, uint32_t* name);  // (empty)
GAME_FN(uint8_t*, __thiscall, TCM_GetModelInfo, 0x00477DC0, void* ped);
GAME_FN(void, __thiscall, TCM_MatrixSetTranslate, 0x004127C0, CMatrix* m, const Vector4* pos);  // (reads 4 floats)
GAME_FN(void, __thiscall, TCM_MatrixRotateZ, 0x00413220, CMatrix* m, float angle);
GAME_FN(float*, __thiscall, TCM_MatrixCopy16, 0x005713D0, float* dst, const float* src);
GAME_FN(void, __thiscall, TCM_TransformPoints4, 0x00570F90, const float* m, Vector4* out, int count, const Vector4* in);

// the sizes the ped shapes are built from
GAME_VAR(float, g_pedHeadHeight, 0x00CF310C);     // 1.55
GAME_VAR(float, g_pedChestHeight, 0x00CF3110);    // 0.95
GAME_VAR(float, g_pedChestHeight2, 0x00CF3114);   // 0.95
GAME_VAR(float, g_pedFatRadius, 0x00CF3118);      // 0.54
GAME_VAR(float, g_pedRadius, 0x00CF311C);         // 0.35
GAME_ARRAY(CColModel*, g_tempColModels, 0x00CF3120);  // 15
GAME_VAR(CCollisionData*, g_pedStandingData, 0x00CF315C);
GAME_VAR(CColSphere*, g_pedBoneSpheres, 0x00CF3160);  // the animated ped model's six
GAME_VAR(uint8_t*, g_defaultPedSpheres, 0x020C88A0);
inline const uint32_t* const kPedSpheresNode = reinterpret_cast<const uint32_t*>(0x01B65310);
inline const void* const kPedSpheresRoot = reinterpret_cast<const void*>(0x01B65314);

CColModel* NewColModel() {
    CColModel* model = TCM_ColModelNew(0x40);
    return model ? TCM_ColModelConstruct(model) : nullptr;
}

CCollisionData* NewCollisionData() {
    void* memory = TCM_Malloc(0x24);
    return memory ? TCM_CollisionDataConstruct(static_cast<CCollisionData*>(memory)) : nullptr;
}

void SetSphere(CColSphere* s, float x, float y, float z, float radius) {
    s->center = {x, y, z};
    s->radius = radius;
}

// the bounding sphere and box of a model
void SetBounds(CColModel* model, float radius, const CVector& center, const CVector& min, const CVector& max) {
    TCM_SphereSet(model, radius, &center);
    TCM_BoxSetMinMax(&model->boxMin, &min, &max);
}

// the standing ped shapes: spheres up the middle, a sphere of 1 round (0, 0, 1)
void SetPedBounds(CColModel* model, float top) {
    CVector center = {0.0f, 0.0f, 1.0f};
    TCM_SphereSet(model, 1.0f, &center);
    CVector max = {g_pedRadius, g_pedRadius, top};
    float negative = -g_pedRadius;
    CVector min = {negative, negative, 0.0f};
    TCM_BoxSetMinMax(&model->boxMin, &min, &max);
}

void SetSurfaces(CColSphere* spheres, int count) {
    for (int i = 0; i < count; i++) {
        spheres[i].surface = 0x1A;
        spheres[i].piece = 0;
    }
}

} // namespace

namespace ColTempModels {

// 0x570F90 (thiscall on 16 floats): count points (16 bytes apart) through the matrix;
// the w written is the game's uninitialised stack (0 here)
void __fastcall TransformPoints4(const float* m, re::Edx, Vector4* out, int count, const Vector4* in) {
    for (int i = 0; i < count; i++) {
        float x = in[i].x;
        float y = in[i].y;
        float z = in[i].z;
        out[i].x = x * m[0] + y * m[4] + z * m[8] + m[12];
        out[i].y = x * m[1] + y * m[5] + z * m[9] + m[13];
        out[i].z = x * m[2] + y * m[6] + z * m[10] + m[14];
        out[i].w = 0.0f;
    }
}

// 0x5713D0 (thiscall): 16 floats copied in
float* __fastcall MatrixCopy16(float* self, re::Edx, const float* src) {
    for (int i = 0; i < 16; i++) {
        self[i] = src[i];
    }
    return self;
}

// 0x571440: CTempColModels::Initialise
void __cdecl Initialise() {
    for (int i = 0; i < 15; i++) {
        g_tempColModels[i] = NewColModel();
    }
    g_pedRadius = 0.35f;
    g_pedFatRadius = 0.54f;
    g_pedChestHeight2 = 0.95f;
    g_pedChestHeight = 0.95f;
    g_pedHeadHeight = 1.55f;

    // [0] the empty model (for models without collision)
    SetBounds(g_tempColModels[0], 10.0f, CVector{0.0f, 0.0f, 0.0f}, CVector{-10.0f, -10.0f, -10.0f}, CVector{10.0f, 10.0f, 10.0f});
    g_tempColModels[0]->colSlot = 0;

    // [1] a standing ped: four spheres up the middle, and two lines forward from the
    // chest (counted as none)
    CColSphere* spheres = static_cast<CColSphere*>(TCM_AllocArray(0x50, 0));
    spheres[0].radius = g_pedRadius;
    spheres[1].radius = g_pedRadius;
    spheres[2].radius = g_pedRadius;
    spheres[3].radius = g_pedFatRadius;
    spheres[0].center = {0.0f, 0.0f, g_pedRadius};
    spheres[1].center = {0.0f, 0.0f, g_pedChestHeight};
    spheres[2].center = {0.0f, 0.0f, g_pedHeadHeight};
    spheres[3].center = {0.0f, 0.0f, 1.1f};
    SetSurfaces(spheres, 4);
    CVector chest = spheres[1].center;
    TempColLine* lines = static_cast<TempColLine*>(TCM_AllocArray(0x40, 0));
    CVector tmp;
    CVector left = {-0.707f, 0.707f, 0.0f};
    TCM_LineSet(&lines[0], &chest, TCM_VectorAdd(&tmp, &chest, &left));
    CVector right = {0.707f, 0.707f, 0.0f};
    TCM_LineSet(&lines[1], &chest, TCM_VectorAdd(&tmp, &chest, &right));
    SetPedBounds(g_tempColModels[1], g_pedHeadHeight + g_pedRadius);
    g_tempColModels[1]->colSlot = 0;
    g_pedStandingData = NewCollisionData();
    g_tempColModels[1]->data = g_pedStandingData;
    g_pedStandingData->numSpheres = 4;
    g_pedStandingData->spheres = spheres;
    g_pedStandingData->unknown20 = 0;
    g_pedStandingData->unknown04 = lines;

    // [2] two spheres (chest and head)
    spheres = static_cast<CColSphere*>(TCM_AllocArray(0x28, 0));
    spheres[0].radius = g_pedRadius;
    spheres[1].radius = g_pedRadius;
    spheres[0].center = {0.0f, 0.0f, g_pedChestHeight};
    spheres[1].center = {0.0f, 0.0f, g_pedHeadHeight};
    SetSurfaces(spheres, 2);
    SetPedBounds(g_tempColModels[2], g_pedHeadHeight + g_pedRadius);
    CCollisionData* data = NewCollisionData();
    g_tempColModels[2]->data = data;
    data->numSpheres = 2;
    data->spheres = spheres;
    g_tempColModels[2]->colSlot = 0;

    // [3] three spheres (feet, chest, head)
    spheres = static_cast<CColSphere*>(TCM_AllocArray(0x3C, 0));
    spheres[0].radius = g_pedRadius;
    spheres[1].radius = g_pedRadius;
    spheres[2].radius = g_pedRadius;
    spheres[0].center = {0.0f, 0.0f, g_pedRadius};
    spheres[1].center = {0.0f, 0.0f, g_pedChestHeight};
    spheres[2].center = {0.0f, 0.0f, g_pedHeadHeight};
    SetSurfaces(spheres, 3);
    SetPedBounds(g_tempColModels[3], g_pedHeadHeight + g_pedRadius);
    data = NewCollisionData();
    g_tempColModels[3]->data = data;
    data->numSpheres = 3;
    data->spheres = spheres;
    g_tempColModels[3]->colSlot = 0;

    // [5] one sphere on the ground
    spheres = static_cast<CColSphere*>(TCM_AllocArray(0x14, 0));
    spheres[0].radius = g_pedRadius;
    spheres[0].center = {0.0f, 0.0f, g_pedRadius};
    spheres[0].surface = 0x1A;
    spheres[0].piece = 0;
    SetPedBounds(g_tempColModels[5], g_pedRadius);
    data = NewCollisionData();
    g_tempColModels[5]->data = data;
    data->numSpheres = 1;
    data->spheres = spheres;
    g_tempColModels[5]->colSlot = 0;

    // [4] two spheres, low (lying or crouched)
    spheres = static_cast<CColSphere*>(TCM_AllocArray(0x28, 0));
    spheres[0].radius = g_pedRadius;
    spheres[1].radius = g_pedRadius;
    spheres[0].center = {0.0f, 0.0f, g_pedRadius};
    spheres[1].center = {0.0f, 0.0f, 0.9f};
    SetSurfaces(spheres, 2);
    SetPedBounds(g_tempColModels[4], 0.9f + g_pedRadius);
    data = NewCollisionData();
    g_tempColModels[4]->data = data;
    data->numSpheres = 2;
    data->spheres = spheres;
    g_tempColModels[4]->colSlot = 0;

    // [6] the animated ped: six spheres 0x5727D0 places on the bones
    g_pedBoneSpheres = static_cast<CColSphere*>(TCM_AllocArray(0x78, 0));
    for (int i = 0; i < 6; i++) {
        g_pedBoneSpheres[i].center = {0.0f, 0.0f, 0.0f};
        g_pedBoneSpheres[i].radius = g_pedRadius;
        g_pedBoneSpheres[i].surface = 0x1A;
        g_pedBoneSpheres[i].piece = 0;
    }
    SetPedBounds(g_tempColModels[6], 0.9f + g_pedRadius);
    data = NewCollisionData();
    g_tempColModels[6]->data = data;
    data->numSpheres = 6;
    data->spheres = g_pedBoneSpheres;
    g_tempColModels[6]->colSlot = 0;

    // [7] a bike: four spheres along it (the pieces: 4 front wheel, 1, 0, 6 back)
    spheres = static_cast<CColSphere*>(TCM_AllocArray(0x50, 0));
    spheres[0].radius = 0.35f;
    spheres[1].radius = 0.35f;
    spheres[2].radius = 0.35f;
    spheres[3].radius = 0.3f;
    spheres[0].center = {0.0f, -0.39999998f, 0.1f};
    spheres[1].center = {0.0f, -0.100000024f, 0.1f};
    spheres[2].center = {0.0f, 0.25f, 0.1f};
    spheres[3].center = {0.0f, 0.65f, 0.1f};
    for (int i = 0; i < 4; i++) {
        spheres[i].surface = 0x1A;
    }
    spheres[0].piece = 4;
    spheres[1].piece = 1;
    spheres[2].piece = 0;
    spheres[3].piece = 6;
    SetBounds(g_tempColModels[7], 2.0f, CVector{0.0f, 0.0f, 0.0f}, CVector{-0.4f, -1.0f, -0.25f}, CVector{0.4f, 1.2f, 0.5f});
    data = NewCollisionData();
    g_tempColModels[7]->data = data;
    data->numSpheres = 4;
    data->spheres = spheres;
    g_tempColModels[7]->colSlot = 0;

    // [8] a ball (surface 0x12)
    CColSphere* ball = static_cast<CColSphere*>(TCM_Malloc(0x14));
    if (ball) {
        ball->center = {0.0f, 0.0f, 0.0f};
    }
    ball->surface = 0x12;
    ball->radius = 0.35f;
    ball->piece = 0;
    ball->center = {0.0f, 0.0f, 0.0f};
    SetBounds(g_tempColModels[8], 0.9f, CVector{0.0f, 0.0f, 0.0f}, CVector{-0.6f, -0.6f, -0.6f}, CVector{0.6f, 0.6f, 0.6f});
    data = NewCollisionData();
    data->numSpheres = 1;
    data->spheres = ball;
    g_tempColModels[8]->data = data;
    g_tempColModels[8]->colSlot = 0;

    // [9]..[14]: boxes (surface 0x12): the left and right doors, the bonnet, the bumper,
    // the boot lid, the boot
    TempColBox* box = static_cast<TempColBox*>(TCM_Malloc(0x24));
    CVector boxMin = {-1.4f, 0.0f, 0.0f};
    CVector boxMax = {-1.2f, 1.35f, 2.50715f};
    TCM_BoxSet(box, &boxMin, &boxMax, 0x12, 0);
    data = NewCollisionData();
    data->numBoxes = 1;
    data->boxes = reinterpret_cast<CColBox*>(box);
    g_tempColModels[9]->data = data;
    SetBounds(g_tempColModels[9], 1.46f, CVector{-1.3f, 0.747161f, 1.25356f}, CVector{-1.37f, 0.0f, 0.0f}, CVector{-1.26f, 1.3f, 2.50715f});
    g_tempColModels[9]->colSlot = 0;

    box = static_cast<TempColBox*>(TCM_Malloc(0x24));
    boxMin = {1.2f, 0.0f, 0.0f};
    boxMax = {1.4f, 1.35f, 2.50715f};
    TCM_BoxSet(box, &boxMin, &boxMax, 0x12, 0);
    data = NewCollisionData();
    data->numBoxes = 1;
    data->boxes = reinterpret_cast<CColBox*>(box);
    SetBounds(g_tempColModels[10], 1.46f, CVector{1.3f, 0.7f, 1.25356f}, CVector{1.23f, 0.0f, 0.0f}, CVector{1.37f, 1.3f, 2.50715f});
    g_tempColModels[10]->data = data;
    g_tempColModels[10]->colSlot = 0;

    box = static_cast<TempColBox*>(TCM_Malloc(0x24));
    boxMin = {-0.045f, 0.0f, 0.0f};
    boxMax = {0.045f, 1.49426f, 2.50715f};
    TCM_BoxSet(box, &boxMin, &boxMax, 0x12, 0);
    data = NewCollisionData();
    data->numBoxes = 1;
    data->boxes = reinterpret_cast<CColBox*>(box);
    SetBounds(g_tempColModels[11], 1.46f, CVector{0.0f, 0.747161f, 1.25356f}, CVector{-0.109f, 0.0f, 0.0f}, CVector{0.109f, 1.49426f, 2.50715f});
    g_tempColModels[11]->data = data;
    g_tempColModels[11]->colSlot = 0;

    box = static_cast<TempColBox*>(TCM_AllocArray(0x48, 0));
    boxMin = {-1.3f, 0.0f, 0.0f};
    boxMax = {-1.173f, 1.262f, 2.546f};
    TCM_BoxSet(&box[0], &boxMin, &boxMax, 0x12, 0);
    boxMin = {1.185f, 0.0f, 0.0f};
    boxMax = {1.29f, 1.29f, 2.546f};
    TCM_BoxSet(&box[1], &boxMin, &boxMax, 0x12, 0);
    data = NewCollisionData();
    data->numBoxes = 2;
    data->boxes = reinterpret_cast<CColBox*>(box);
    SetBounds(g_tempColModels[12], 2.0f, CVector{0.0f, 0.7f, 1.3f}, CVector{-1.34f, -0.1f, -0.1f}, CVector{1.34f, 1.34f, 2.57f});
    g_tempColModels[12]->data = data;
    g_tempColModels[12]->colSlot = 0;

    box = static_cast<TempColBox*>(TCM_Malloc(0x24));
    boxMin = {0.546f, 0.0f, -0.2f};
    boxMax = {0.755f, 1.524f, 1.864f};
    TCM_BoxSet(box, &boxMin, &boxMax, 0x12, 0);
    data = NewCollisionData();
    data->numBoxes = 1;
    data->boxes = reinterpret_cast<CColBox*>(box);
    SetBounds(g_tempColModels[13], 1.6f, CVector{0.673f, 0.763f, 0.85f}, CVector{0.546f, 0.0f, -0.2f}, CVector{0.755f, 1.524f, 1.864f});
    g_tempColModels[13]->data = data;
    g_tempColModels[13]->colSlot = 0;

    box = static_cast<TempColBox*>(TCM_AllocArray(0x48, 0));
    boxMin = {-1.0f, 0.0f, 0.0f};
    boxMax = {-0.8f, 1.0f, 2.4f};
    TCM_BoxSet(&box[0], &boxMin, &boxMax, 0x12, 0);
    boxMin = {0.8f, 0.0f, 0.0f};
    boxMax = {1.0f, 1.0f, 2.4f};
    TCM_BoxSet(&box[1], &boxMin, &boxMax, 0x12, 0);
    data = NewCollisionData();
    data->numBoxes = 2;
    data->boxes = reinterpret_cast<CColBox*>(box);
    SetBounds(g_tempColModels[14], 1.5f, CVector{0.0f, 0.5f, 1.4f}, CVector{-1.1f, -0.1f, 0.0f}, CVector{1.0f, 1.1f, 2.5f});
    g_tempColModels[14]->data = data;
    g_tempColModels[14]->colSlot = 0;
}

// 0x5727D0: the animated ped model's six spheres on the ped's bones (the bones and
// radii from the model's sphere set, "the default's" without one), relative to the ped
// and turned back by its heading; the bounds round the farthest (the box's bottom at 0)
void __cdecl AnimatePedColModel(uint8_t* ped) {
    CColSphere* spheres = g_pedBoneSpheres;
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x14);
    const float* position = reinterpret_cast<const float*>(matrix ? matrix + 0x30 : ped + 4);
    float px = position[0];
    float py = position[1];
    float pz = position[2];

    uint32_t names[2];
    TCM_NameCopy(&names[0], kPedSpheresNode);
    TCM_NameSet(&names[1], *reinterpret_cast<uint32_t*>(TCM_GetModelInfo(ped) + 4));
    uint32_t path[2];
    TCM_PathBuild(path, 2, names);
    uint8_t* sphereSet = TCM_ButesFind(path, kPedSpheresRoot);
    if (sphereSet == nullptr) {
        sphereSet = g_defaultPedSpheres;
    }
    float heading;
    if (*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x14) != nullptr) {
        const float* m = re::PtrCast<const float*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x14));
        heading = float(std::atan2(double(-m[4]), double(m[5])));
    } else {
        heading = *reinterpret_cast<float*>(ped + 0x10);
    }

    // (w: the game's uninitialised stack; 0 here)
    Vector4 back = {px * -1.0f, py * -1.0f, pz * -1.0f, 0.0f};
    CMatrix toPed;
    toPed.attached = nullptr;
    TCM_MatrixSetTranslate(&toPed, &back);
    TCM_MatrixRotateZ(&toPed, -heading);
    float copy[16];
    TCM_MatrixCopy16(copy, toPed.m);

    // the six bones' positions (w uninitialised in the game: 0 here)
    uint8_t* bones = *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x114) + 0x14) + 8);
    Vector4 in[6];
    for (int i = 0; i < 6; i++) {
        const float* bone = reinterpret_cast<const float*>(bones + *reinterpret_cast<int32_t*>(sphereSet + 0x18 + i * 8) * 0x34 + 0x24);
        in[i] = {bone[0], bone[1], bone[2], 0.0f};
    }
    Vector4 out[6];
    TCM_TransformPoints4(copy, out, 6, in);

    // the centers, and the farthest from the ped's origin (the first counts only when
    // it isn't at the origin: its index is then the length's bits, 0)
    float farthestSq = 0.0f;
    int farthest;
    spheres[0].center = {out[0].x, out[0].y, out[0].z};
    float lengthSq = spheres[0].center.x * spheres[0].center.x + spheres[0].center.y * spheres[0].center.y +
                     spheres[0].center.z * spheres[0].center.z;
    if (lengthSq > 0.0f) {
        farthestSq = lengthSq;
        farthest = 0;
    } else {
        farthest = std::bit_cast<int32_t>(lengthSq);
    }
    for (int i = 1; i < 6; i++) {
        spheres[i].center = {out[i].x, out[i].y, out[i].z};
        lengthSq = spheres[i].center.x * spheres[i].center.x + spheres[i].center.y * spheres[i].center.y +
                   spheres[i].center.z * spheres[i].center.z;
        if (farthestSq < lengthSq) {
            farthestSq = lengthSq;
            farthest = i;
        }
    }
    for (int i = 0; i < 6; i++) {
        spheres[i].radius = *reinterpret_cast<float*>(sphereSet + 0x1C + i * 8);
    }
    float radius = std::sqrt(std::fabs(farthestSq)) + spheres[farthest].radius;
    CColModel* model = g_tempColModels[6];
    CVector zero = {0.0f, 0.0f, 0.0f};
    TCM_SphereSet(model, radius, &zero);
    CVector max = {radius, radius, radius};
    CVector min = {-radius, -radius, 0.0f};
    TCM_BoxSetMinMax(&model->boxMin, &min, &max);
    for (int i = 1; i >= 0; i--) {
        TCM_NameDestruct(&names[i]);
    }
}

} // namespace ColTempModels

// ---- registration: all original until tested ---------------------------------------

#define COLTEMP_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(coltemp_original_, __LINE__);                                     \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(coltemp_original_, __LINE__), re::Mode::Original)

COLTEMP_REVERSED("CMatrix::TransformPoints4", 0x00570F90, &ColTempModels::TransformPoints4);
COLTEMP_REVERSED("CMatrix::Copy16", 0x005713D0, &ColTempModels::MatrixCopy16);
COLTEMP_REVERSED("CTempColModels::Initialise", 0x00571440, &ColTempModels::Initialise);
COLTEMP_REVERSED("CTempColModels::AnimatePedColModel", 0x005727D0, &ColTempModels::AnimatePedColModel);
