// NiCamera (src/gamebryo/NiCamera.h): the world-to-camera matrix from the world transform and
// the frustum, LookAtWorldPoint, the frustum's limits, streaming and cloning.
//
// Main-thread float maths at the game's 24-bit x87 precision: each product and sum below is a
// float operation in the game's order (its grouping kept; the operands of a single add or
// multiply commute exactly).

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCamera.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiCamera_RTTI, 0x020E273C);
constexpr uintptr_t NiCamera_vtable = 0x00951DF0;

GAME_FN(void, __thiscall, AVObject_UpdateWorldData, 0x00779EB0, NiAVObject* self);
GAME_FN(void, __thiscall, Frustum_Construct, 0x00771890, NiFrustum* self, int ortho);
GAME_FN(char*, __thiscall, Frustum_GetViewerString, 0x007718E0, const NiFrustum* self, const char* prefix);
GAME_FN(float, __thiscall, Point3_Unitize, 0x00415130, NiPoint3* self);
GAME_FN(void, __thiscall, Matrix3_SetCols, 0x00768780, NiMatrix3* self, const NiPoint3* col0, const NiPoint3* col1,
        const NiPoint3* col2);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_TransposeTimes, 0x00768F60, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// (this chunk's, called where modes apply)
GAME_FN(NiPoint3*, __thiscall, Point3_UnitCross, 0x00762F30, const NiPoint3* self, NiPoint3* result,
        const NiPoint3* other);
GAME_FN(void, __thiscall, Camera_WorldToCameraMatrix, 0x007631D0, NiCamera* self);
GAME_FN(void, __thiscall, Camera_CopyMembers, 0x00763580, NiCamera* self, NiCamera* dest, void* cloning);
GAME_FN(NiCamera*, __thiscall, Camera_Construct, 0x00763C20, void* self);
GAME_FN(char*, __thiscall, Rect_GetViewerString, 0x00763BC0, const NiRectFloat* self, const char* prefix);

void __fastcall Destruct(NiCamera* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiCamera_vtable);
    AVObject_Destruct(self);
}

NiCamera* __fastcall DeletingDestructor(NiCamera* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiCamera_vtable);
    AVObject_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x13C);
    }
    return self;
}

// SetViewFrustum(kFrustum): the near plane kept within the far plane's ratio and the minimum
void __fastcall SetViewFrustum(NiCamera* self, re::Edx, const NiFrustum* frustum) {
    float nearDist = frustum->nearDist;
    self->frustum.nearDist = nearDist;
    float limit = frustum->farDist / self->maxFarNearRatio;
    if (limit > nearDist) {
        self->frustum.nearDist = limit;
    }
    if (self->minNearPlaneDist > self->frustum.nearDist) {
        self->frustum.nearDist = self->minNearPlaneDist;
    }
    self->frustum.left = frustum->left;
    self->frustum.right = frustum->right;
    self->frustum.top = frustum->top;
    self->frustum.bottom = frustum->bottom;
    self->frustum.farDist = frustum->farDist;
    self->frustum.ortho = frustum->ortho;
}

// vf8: the frustum (not its ortho flag) and the viewport
uint8_t __fastcall IsEqual(NiCamera* self, re::Edx, NiCamera* other) {
    if (!AVObject_IsEqual(self, other)) {
        return 0;
    }
    const float* a = &self->frustum.left;
    const float* b = &other->frustum.left;
    for (int i = 0; i < 6; i++) {
        if (!(a[i] == b[i])) {
            return 0;
        }
    }
    const float* c = &self->port.left;
    const float* d = &other->port.left;
    for (int i = 0; i < 5; i++) {   // the port and the LOD adjustment
        if (!(c[i] == d[i])) {
            return 0;
        }
    }
    return 1;
}

// NiPoint3::UnitCross(pt): this x pt, normalized (zero when shorter than 1e-6)
NiPoint3* __fastcall UnitCross(const NiPoint3* self, re::Edx, NiPoint3* result, const NiPoint3* pt) {
    float x = pt->z * self->y - pt->y * self->z;
    float y = self->z * pt->x - self->x * pt->z;
    float z = pt->y * self->x - pt->x * self->y;
    float lengthSquared = (y * y + x * x) + z * z;
    float length = crt::Sqrtf(lengthSquared);
    if (length > 1e-06f) {
        float inverse = 1.0f / length;
        result->x = inverse * x;
        result->y = inverse * y;
        result->z = inverse * z;
    } else {
        result->x = 0.0f;
        result->y = 0.0f;
        result->z = 0.0f;
    }
    return result;
}

// LookAtWorldPoint(kWorldPt, kWorldUp): false when the point is (nearly) the camera's or the up
// vector lines up with the direction
uint8_t __fastcall LookAtWorldPoint(NiCamera* self, re::Edx, const NiPoint3* target, const NiPoint3* up) {
    NiPoint3 dir;
    dir.x = self->world.translate.x - target->x;
    dir.y = self->world.translate.y - target->y;
    dir.z = self->world.translate.z - target->z;
    float lengthSquared = (dir.x * dir.x + dir.y * dir.y) + dir.z * dir.z;
    if (lengthSquared < 1e-10f) {
        return 0;
    }
    Point3_Unitize(&dir);
    NiPoint3 right;
    Point3_UnitCross(up, &right, &dir);
    if ((right.y * right.y + right.x * right.x) + right.z * right.z < 0.5f) {
        return 0;
    }
    NiPoint3 newUp;
    Point3_UnitCross(&dir, &newUp, &right);
    if ((newUp.y * newUp.y + newUp.x * newUp.x) + newUp.z * newUp.z < 0.5f) {
        return 0;
    }
    NiPoint3 forward;
    forward.x = -dir.x;
    forward.y = -dir.y;
    forward.z = -dir.z;
    NiMatrix3 rotate;
    Matrix3_SetCols(&rotate, &forward, &newUp, &right);
    if (self->parent != nullptr) {
        NiMatrix3 local;
        rotate = *Matrix3_TransposeTimes(&self->parent->world.rotate, &local, &rotate);
    }
    self->local.rotate = rotate;
    Virtual<void>(self, 30);   // UpdateWorldData
    Virtual<void>(self, 31);   // UpdateWorldBound
    return 1;
}

// the world-to-camera (and projection) matrix: the camera's axes (direction, up, right: the
// world rotation's columns) against its position, scaled into the frustum
void __fastcall WorldToCameraMatrix(NiCamera* self, re::Edx) {
    const NiMatrix3& r = self->world.rotate;
    float tx = self->world.translate.x;
    float ty = self->world.translate.y;
    float tz = self->world.translate.z;
    float dx = r.m[0][0], dy = r.m[1][0], dz = r.m[2][0];
    float ux = r.m[0][1], uy = r.m[1][1], uz = r.m[2][1];
    float rx = r.m[0][2], ry = r.m[1][2], rz = r.m[2][2];
    float kR = -((tx * rx + ty * ry) + tz * rz);
    float kU = -((tx * ux + ty * uy) + tz * uz);
    float kD = -((tx * dx + ty * dy) + tz * dz);
    const NiFrustum& f = self->frustum;
    float sumW = f.right + f.left;
    float sumH = f.bottom + f.top;
    float invW = 1.0f / (f.right - f.left);
    float invH = 1.0f / (f.top - f.bottom);
    float invD = 1.0f / (f.farDist - f.nearDist);
    float twoInvW = invW * 2.0f;
    float(&m)[4][4] = self->worldToCam;
    if (f.ortho != 0) {
        float twoInvH = invH * 2.0f;
        m[0][0] = rx * twoInvW;
        m[0][1] = ry * twoInvW;
        m[0][2] = rz * twoInvW;
        float a = invW * sumW;
        m[0][3] = twoInvW * kR + a;
        m[1][0] = ux * twoInvH;
        m[1][1] = uy * twoInvH;
        m[1][2] = uz * twoInvH;
        float b = invH * sumH;
        m[1][3] = twoInvH * kU + b;
        m[2][0] = dx * invD;
        m[2][1] = dy * invD;
        m[2][2] = dz * invD;
        float c = -f.nearDist * invD;
        m[2][3] = kD * invD + c;
        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = 0.0f;
        m[3][3] = 1.0f;
        return;
    }
    float a = invW * -sumW;
    float twoInvH = invH * 2.0f;
    float b = invH * -sumH;
    float depth = f.farDist * invD;
    m[0][0] = rx * twoInvW + a * dx;
    m[0][1] = a * dy + twoInvW * ry;
    m[0][2] = a * dz + twoInvW * rz;
    m[0][3] = twoInvW * kR + a * kD;
    m[1][0] = b * dx + twoInvH * ux;
    m[1][1] = b * dy + twoInvH * uy;
    m[1][2] = b * dz + twoInvH * uz;
    m[1][3] = b * kD + twoInvH * kU;
    m[2][0] = depth * dx;
    m[2][1] = depth * dy;
    m[2][2] = depth * dz;
    float c = -f.nearDist * depth;
    m[2][3] = kD * depth + c;
    m[3][0] = dx;
    m[3][1] = dy;
    m[3][2] = dz;
    m[3][3] = kD;
}

// vf31 UpdateWorldBound: a point at the camera
void __fastcall UpdateWorldBound(NiCamera* self, re::Edx) {
    self->worldBound.center = self->world.translate;
}

// vf30 UpdateWorldData: NiAVObject's, then the matrix
void __fastcall UpdateWorldData(NiCamera* self, re::Edx) {
    AVObject_UpdateWorldData(self);
    Camera_WorldToCameraMatrix(self);
}

void __fastcall CopyMembers(NiCamera* self, re::Edx, NiCamera* dest, void* cloning) {
    AVObject_CopyMembers(self, dest, cloning);
    crt::MemcpyS(dest->worldToCam, 0x40, self->worldToCam, 0x40);
    dest->world.translate = self->world.translate;
    std::memcpy(&dest->frustum, &self->frustum, sizeof(NiFrustum));
    dest->port = self->port;
    dest->lodAdjust = self->lodAdjust;
    dest->minNearPlaneDist = self->minNearPlaneDist;
    dest->maxFarNearRatio = self->maxFarNearRatio;
}

NiCamera* __fastcall Construct(NiCamera* self, re::Edx) {
    AVObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiCamera_vtable);
    Frustum_Construct(&self->frustum, 0);
    self->port = {0.0f, 0.0f, 0.0f, 0.0f};
    self->frustum.ortho = 0;
    self->frustum.nearDist = 1.0f;
    self->frustum.farDist = 2.0f;
    self->frustum.top = 0.5f;
    self->frustum.bottom = -0.5f;
    self->frustum.left = -0.5f;
    self->frustum.right = 0.5f;
    self->minNearPlaneDist = 0.1f;
    self->maxFarNearRatio = 10000.0f;
    self->port.top = 1.0f;
    self->port.right = 1.0f;
    self->lodAdjust = 1.0f;
    self->port.bottom = 0.0f;
    self->port.left = 0.0f;
    AVObject_UpdateWorldData(self);
    Camera_WorldToCameraMatrix(self);
    self->worldBound.center = self->world.translate;
    return self;
}

// vf3 CreateClone
NiCamera* __fastcall CreateClone(NiCamera* self, re::Edx, void* cloning) {
    void* p = NiNew(0x13C);
    if (p != nullptr) {
        NiCamera* clone = Camera_Construct(p);
        Camera_CopyMembers(self, clone, cloning);
        return clone;
    }
    Camera_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

// vf4: (a 16-bit word skipped) the frustum, the port, the LOD adjustment; three old links
void __fastcall LoadBinary(NiCamera* self, re::Edx, void* stream) {
    AVObject_LoadBinary(self, stream);
    uint16_t unused;
    LoadBytes(stream, &unused, 2);
    LoadBytes(stream, &self->frustum.left, 4);
    LoadBytes(stream, &self->frustum.right, 4);
    LoadBytes(stream, &self->frustum.top, 4);
    LoadBytes(stream, &self->frustum.bottom, 4);
    LoadBytes(stream, &self->frustum.nearDist, 4);
    LoadBytes(stream, &self->frustum.farDist, 4);
    uint8_t ortho;
    LoadBytes(stream, &ortho, 1);
    self->frustum.ortho = ortho != 0 ? 1 : 0;
    LoadBytes(stream, &self->port.left, 4);
    LoadBytes(stream, &self->port.right, 4);
    LoadBytes(stream, &self->port.top, 4);
    LoadBytes(stream, &self->port.bottom, 4);
    LoadBytes(stream, &self->lodAdjust, 4);
    Stream_ReadLinkID(stream);
    Stream_ReadMultipleLinkIDs(stream);
    Stream_ReadMultipleLinkIDs(stream);
}

// vf5: the old links resolved and dropped
void __fastcall LinkObject(NiCamera* self, re::Edx, void* stream) {
    AVObject_LinkObject(self, stream);
    NiObject* scene = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
    AddRef(scene);
    for (int list = 0; list < 2; list++) {
        for (uint32_t count = Stream_GetNumberOfLinkIDs(stream); count != 0; count--) {
            NiObject* object = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
            AddRef(object);
            Release(object);
        }
    }
    Release(scene);
}

// vf7
void __fastcall SaveBinary(NiCamera* self, re::Edx, void* stream) {
    AVObject_SaveBinary(self, stream);
    uint16_t zero = 0;
    SaveBytes(stream, &zero, 2);
    SaveBytes(stream, &self->frustum.left, 4);
    SaveBytes(stream, &self->frustum.right, 4);
    SaveBytes(stream, &self->frustum.top, 4);
    SaveBytes(stream, &self->frustum.bottom, 4);
    SaveBytes(stream, &self->frustum.nearDist, 4);
    SaveBytes(stream, &self->frustum.farDist, 4);
    uint8_t ortho = self->frustum.ortho;
    SaveBytes(stream, &ortho, 1);
    SaveBytes(stream, &self->port.left, 4);
    SaveBytes(stream, &self->port.right, 4);
    SaveBytes(stream, &self->port.top, 4);
    SaveBytes(stream, &self->port.bottom, 4);
    SaveBytes(stream, &self->lodAdjust, 4);
    Virtual<void>(stream, 12, static_cast<void*>(nullptr));   // SaveLinkID
    uint32_t none = 0;
    SaveBytes(stream, &none, 4);
    SaveBytes(stream, &none, 4);
}

// NiRect<float>::GetViewerString(pcPrefix)
char* __fastcall RectGetViewerString(const NiRectFloat* self, re::Edx, const char* prefix) {
    size_t size = std::strlen(prefix) + 0x40;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, "%s = (L=%g,R=%g,T=%g,B=%g)", prefix, double(self->left), double(self->right),
              double(self->top), double(self->bottom));
    return text;
}

// vf9
void __fastcall GetViewerStrings(NiCamera* self, re::Edx, NiTArray<char*>* strings) {
    AVObject_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiCamera_RTTI.name));
    AddViewerString(strings, Frustum_GetViewerString(&self->frustum, "m_kViewFrustum"));
    AddViewerString(strings, Rect_GetViewerString(&self->port, "m_kPort"));
    const NiMatrix3& r = self->world.rotate;
    NiPoint3 axis{r.m[0][0], r.m[1][0], r.m[2][0]};
    AddViewerString(strings, Point3_GetViewerString(&axis, "m_kWorldDir"));
    axis = {r.m[0][1], r.m[1][1], r.m[2][1]};
    AddViewerString(strings, Point3_GetViewerString(&axis, "m_kWorldUp"));
    axis = {r.m[0][2], r.m[1][2], r.m[2][2]};
    AddViewerString(strings, Point3_GetViewerString(&axis, "m_kWorldRight"));
    AddViewerString(strings, GetViewerStringFloat("m_fMinNearPlaneDist", self->minNearPlaneDist));
    AddViewerString(strings, GetViewerStringFloat("m_fMaxFarNearRatio", self->maxFarNearRatio));
}

} // namespace

RE_UNHOOKED("NiCamera::RegisterStreamables", 0x00762E00);   // a jmp to NiAVObject's

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NICAM_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NICAM_REVERSED("NiCamera::~NiCamera", 0x00762D50, &ni::Destruct);
NICAM_REVERSED("NiCamera::SetViewFrustum", 0x00762D60, &ni::SetViewFrustum);
NICAM_REVERSED("NiCamera::IsEqual", 0x00762E10, &ni::IsEqual);
NICAM_REVERSED("NiPoint3::UnitCross", 0x00762F30, &ni::UnitCross);
NICAM_REVERSED("NiCamera::DeletingDestructor", 0x00762FF0, &ni::DeletingDestructor);
NICAM_REVERSED("NiCamera::LookAtWorldPoint", 0x00763020, &ni::LookAtWorldPoint);
NICAM_REVERSED("NiCamera::UpdateWorldBound", 0x007631B0, &ni::UpdateWorldBound);
NICAM_REVERSED("NiCamera::WorldToCameraMatrix", 0x007631D0, &ni::WorldToCameraMatrix);
NICAM_REVERSED("NiCamera::CopyMembers", 0x00763580, &ni::CopyMembers);
NICAM_REVERSED("NiCamera::UpdateWorldData", 0x00763650, &ni::UpdateWorldData);
NICAM_REVERSED("NiCamera::LoadBinary", 0x00763660, &ni::LoadBinary);
NICAM_REVERSED("NiCamera::LinkObject", 0x00763880, &ni::LinkObject);
NICAM_REVERSED("NiCamera::SaveBinary", 0x00763960, &ni::SaveBinary);
NICAM_REVERSED("NiRect<float>::GetViewerString", 0x00763BC0, &ni::RectGetViewerString);
NICAM_REVERSED("NiCamera::NiCamera", 0x00763C20, &ni::Construct);
NICAM_REVERSED("NiCamera::CreateClone", 0x00763D00, &ni::CreateClone);
NICAM_REVERSED("NiCamera::GetViewerStrings", 0x00763D70, &ni::GetViewerStrings);
