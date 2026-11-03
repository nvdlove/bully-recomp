// CPhysical's per-frame physics (GTA San Andreas' names): ProcessControl (vf12, with the
// stuck counter), ApplyAirResistance, ApplyTurnSpeed (vf38), ApplyGravity (vf36),
// ProcessCollision (vf13: moved, then pushed back out of what it hit, up to three times)
// and the sector list test it's built on (0x46C600).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "game/World.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

// A placeable's position (its matrix's, else its placement's)
CVector& PosOf(CPlaceable* p) {
    return p->GetPosition();
}
const CVector& MatrixPos(const CMatrix& m) {
    return *reinterpret_cast<const CVector*>(&m.m[12]);
}

constexpr uint8_t TYPE_VEHICLE = 2;
constexpr uint8_t TYPE_PED = 3;
constexpr uint8_t TYPE_OBJECT = 4;

GAME_VAR(float, PP_timeStep, 0x00C1A9A4);
GAME_VAR(uint16_t, PP_scanCode, 0x00C1AE88);           // CWorld::ms_nCurrentScanCode
GAME_VAR(float, PP_fltMax, 0x009009B8);
GAME_VAR(CVector, PP_zeroVector, 0x00BCC564);
GAME_VAR(uint8_t, PP_groundOnly, 0x00C2B10B);          // (set by CPed::ProcessCollision)
GAME_VAR(uint8_t, PP_chapter, 0x00A147E0);             // its bit in a model's +0xB flags
GAME_VAR(uint32_t*, PP_ptrNodes, 0x00C0F788);          // CPtrNode storage (4 bytes each)
GAME_ARRAY(uint8_t*, PP_modelInfos, 0x00C67738);
GAME_VAR(int32_t, PP_modelBF4354, 0x00BF4354);
GAME_VAR(int32_t, PP_modelBF4358, 0x00BF4358);
GAME_VAR(int32_t, PP_modelBF435C, 0x00BF435C);
GAME_VAR(int32_t, PP_modelBF439C, 0x00BF439C);         // (slides with a push along the ground)
GAME_VAR(int32_t, PP_modelBF44DC, 0x00BF44DC);         // (a projectile that sticks where it hits)
GAME_VAR(float, PP_slidePushUp, 0x00A24030);
GAME_VAR(float, PP_slidePushOut, 0x00A24034);
GAME_VAR(float, PP_slideMaxNormalZ, 0x00A24038);
GAME_VAR(float, PP_pedBounce, 0x00A2403C);
GAME_VAR(float, PP_pedFriction, 0x00A24040);

GAME_FN(CMatrix*, __thiscall, PP_GetMatrixLink, 0x008B6120, CEntity* entity);
GAME_FN(CMatrix*, __thiscall, PP_Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
GAME_FN(CVector*, __thiscall, PP_Placeable_GetPosition, 0x00415460, CEntity* entity);
GAME_FN(float, __thiscall, PP_Placeable_GetHeading, 0x00415470, CEntity* entity);
GAME_FN(CVector*, __thiscall, PP_GetForward, 0x004154A0, CEntity* entity, CVector* out);
GAME_FN(void, __thiscall, PP_Physical_ApplyAirResistance, 0x00469A10, CPhysical* self);
GAME_FN(void, __thiscall, PP_Physical_ApplyTurnForce, 0x0046A950, CPhysical* self, float jx, float jy, float jz, float px,
        float py, float pz);
GAME_FN(void, __thiscall, PP_Physical_ApplyCollisionPair, 0x0046B870, CPhysical* self, CEntity* other, CColPoint* point,
        float angle);
GAME_FN(uint8_t, __thiscall, PP_Physical_NearestWheelHit, 0x0046CAC0, CPhysical* self, CMatrix* matrix, CSector* sector,
        CColPoint* point, int a, int b);
GAME_FN(void, __thiscall, PP_Physical_SetOrientation, 0x0046A690, CPhysical* self, float x, float y, float z);
GAME_FN(void, __thiscall, PP_Physical_RemoveAndAdd, 0x00469470, CPhysical* self);
GAME_FN(void, __thiscall, PP_Physical_GetBoundingSphere, 0x00466CA0, CPhysical* self, void* sphere);
GAME_FN(float, __thiscall, PP_Physical_GetBoundRadius, 0x00465E10, CPhysical* self);
GAME_FN(uint8_t, __thiscall, PP_Entity_GetIsTouching, 0x00465D10, CEntity* entity, const CVector* centre, float radius);
GAME_FN(void, __thiscall, PP_Entity_SetPosition, 0x0041C4E0, CEntity* entity, float x, float y, float z);
GAME_FN(void, __thiscall, PP_Entity_Fn443AA0, 0x00443AA0, CEntity* entity);
GAME_FN(CEntity*, __thiscall, PP_Seats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(uint8_t, __thiscall, PP_Ped_Fn476200, 0x00476200, CEntity* ped, CEntity* other);
GAME_FN(uint8_t, __thiscall, PP_Ped_Fn4762B0, 0x004762B0, CEntity* ped, CEntity* vehicle);
GAME_FN(uint8_t, __thiscall, PP_Ped_Fn4763C0, 0x004763C0, CEntity* ped, CEntity* vehicle);
GAME_FN(uint8_t, __thiscall, PP_Ped_Fn4762F0, 0x004762F0, CEntity* ped, CEntity* other);
GAME_FN(uint8_t, __cdecl, PP_Fn46A060, 0x0046A060, CEntity* a, CEntity* b);
GAME_FN(CEntity*, __cdecl, PP_GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_FN(CColModel*, __cdecl, PP_GetColModel, 0x0051AF50, CEntity* entity);
GAME_FN(uint8_t, __cdecl, PP_ProcessVerticalLine, 0x004629C0, const CVector* point, float zStop, CColPoint* colPoint,
        re::Ptr32<CEntity>* entity, int buildings, int vehicles);
GAME_FN(void, __cdecl, PP_ClearScanCodes, 0x0045D840);
GAME_FN(void, __cdecl, PP_CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(CVector*, __cdecl, PP_VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, PP_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, PP_CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, PP_DotProduct, 0x00414640, const CVector* a, const CVector* b);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* PP_Multiply3x3(CVector* out, const CMatrix* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
GAME_FN(CVector*, __cdecl, PP_VectorScale, 0x00412350, CVector* out, const CVector* v, float s);
GAME_FN(void, __thiscall, PP_VectorAddTo, 0x00415200, CVector* v, const CVector* other);
GAME_FN(float, __thiscall, PP_VectorMagnitude, 0x0040DFB0, const CVector* v);
GAME_FN(float, __thiscall, PP_VectorNormalise, 0x00414E10, CVector* v);
GAME_FN(CMatrix*, __thiscall, PP_Matrix_Copy, 0x00412A10, CMatrix* m, const CMatrix* from);
GAME_FN(CMatrix*, __thiscall, PP_Matrix_Assign, 0x00412720, CMatrix* m, const CMatrix* from);
GAME_FN(void, __thiscall, PP_Matrix_UpdateRWFull, 0x00413A80, CMatrixLink* m);
GAME_FN(CColPoint*, __thiscall, PP_ColPoint_Construct, 0x00415FA0, CColPoint* point);
GAME_FN(void, __thiscall, PP_ColPoint_Destruct, 0x00415500, CColPoint* point);
GAME_FN(CColPoint*, __thiscall, PP_ColPoint_Assign, 0x0046AA70, CColPoint* point, const CColPoint* other);
GAME_FN(void, __thiscall, PP_ColPoint_SetEntity, 0x0045F1F0, CColPoint* point, CEntity* entity);
GAME_FN(float, __cdecl, PP_GetHeadingOfVector, 0x00433310, const CVector* v);
GAME_FN(float, __cdecl, PP_AngleDifference, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, PP_RandomFloat, 0x0041A5B0, float min, float max);
GAME_FN(void, __cdecl, PP_Fn701640, 0x00701640, const CVector* pos);
GAME_FN(void, __cdecl, PP_Fn701FC0, 0x00701FC0, const CVector* pos, const CVector* normal);

template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

void NextScanCode() {
    if (PP_scanCode < 0xFFFF) {
        PP_scanCode = uint16_t(PP_scanCode + 1);
    } else {
        PP_ClearScanCodes();
        PP_scanCode = 1;
    }
}

void ColPointClear(CColPoint* cp) {
    // (the game leaves +0x1C..+0x23 uninitialised)
    cp->point = {0.0f, 0.0f, 0.0f};
    cp->entity = nullptr;
    cp->normal = {0.0f, 0.0f, 0.0f};
    cp->piece = 0;
    cp->surfaceA = 0;
    cp->surfaceB = 0;
    cp->depth = PP_fltMax;
}

void ReleaseEntity(CColPoint* cp) {
    if (cp->entity) {
        PP_CleanUpOldReference(cp->entity, &cp->entity);
    }
}

float SquaredLength(const CVector& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

// 0x469A10 (thiscall): GTA's ApplyAirResistance. Its +0x160 over 0.1: the speeds scaled
// by +0x160 to the power of the time step; otherwise by 1 / (+0x160 * 0.5 * speed^2 + 1)
// to that power, the turn speed by 0.99
void __fastcall ApplyAirResistance(CPhysical* self, re::Edx) {
    float air = Field<float>(self, 0x160);
    if (air > double(0.1f)) {
        float f = float(std::pow(double(air), double(PP_timeStep)));
        self->moveSpeed.x = f * self->moveSpeed.x;
        self->moveSpeed.y = self->moveSpeed.y * f;
        self->moveSpeed.z = self->moveSpeed.z * f;
        self->turnSpeed.x = self->turnSpeed.x * f;
        self->turnSpeed.y = self->turnSpeed.y * f;
        self->turnSpeed.z = self->turnSpeed.z * f;
        return;
    }
    float sq = SquaredLength(self->moveSpeed);
    float k = float(double(air) * 0.5 * sq);
    float base = float(1.0f / std::fabs(float(k + 1.0f)));
    float f = float(std::pow(double(base), double(PP_timeStep)));
    self->moveSpeed.x = f * self->moveSpeed.x;
    self->moveSpeed.y = self->moveSpeed.y * f;
    self->moveSpeed.z = f * self->moveSpeed.z;
    self->turnSpeed.x = float(self->turnSpeed.x * double(0.99f));
    self->turnSpeed.y = float(self->turnSpeed.y * double(0.99f));
    self->turnSpeed.z = float(double(0.99f) * self->turnSpeed.z);
}

// 0x469B70 vf12 (thiscall): GTA's CPhysical::ProcessControl. The frame's flags reset;
// objects and peds (not +0x30) whose averaged speeds stay tiny for more than 10 frames
// are made static (+0xB0 set: never); then the air resistance
void __fastcall ProcessControl(CPhysical* self, re::Edx) {
    uint8_t type = self->typeAndStatus & 7;
    if (type != TYPE_PED) {
        self->physicalFlags &= 0xEF;
    }
    Field<float>(self, 0x1A0) = 0.0f;
    self->flag2C = 0;
    self->flag38 = 0;
    self->flag40 = 0;
    self->flag7C = 0;
    self->unknown1B4 = 0;
    self->unknown1A4 = 0;
    if ((self->typeAndStatus & 0xF8) == 0x10) {
        return;
    }
    self->numCollisionRecords = 0;
    self->flag4C = 0;
    if (self->flag34 == 0 && (type == TYPE_OBJECT || (type == TYPE_PED && self->flag30 == 0))) {
        CVector sum;
        PP_VectorAdd(&sum, &self->frictionMoveSpeed, &self->moveSpeed);
        self->frictionMoveSpeed = {float(sum.x * 0.5), float(sum.y * 0.5), float(0.5 * sum.z)};
        PP_VectorAdd(&sum, &self->frictionTurnSpeed, &self->turnSpeed);
        self->frictionTurnSpeed = {float(sum.x * 0.5), float(sum.y * 0.5), float(0.5 * sum.z)};
        float step = float(PP_timeStep * double(0.003f));
        float stepSq = step * step;
        if (SquaredLength(self->frictionMoveSpeed) < stepSq && SquaredLength(self->frictionTurnSpeed) < stepSq) {
            if (self->flagB0 == 0) {
                self->stuckCount = uint8_t(self->stuckCount + 1);
                if (self->stuckCount > 10) {
                    self->stuckCount = 10;
                    self->CallSetIsStatic(1);
                    self->moveSpeed = {0.0f, 0.0f, 0.0f};
                    self->turnSpeed = {0.0f, 0.0f, 0.0f};
                    return;
                }
            }
        } else {
            self->stuckCount = 0;
        }
    }
    PP_Physical_ApplyAirResistance(self);
}

// 0x469DC0 vf38 (thiscall): GTA's ApplyTurnSpeed: the matrix's axes turned by the turn
// speed over the time step (its rows' fourth floats the game writes from uninitialised
// stack; ours leaves them), or without a matrix its heading
void __fastcall ApplyTurnSpeed(CPhysical* self, re::Edx) {
    if (self->physicalFlags2 & 1) {
        self->turnSpeed.z = 0.0f;
        self->turnSpeed.y = 0.0f;
        self->turnSpeed.x = 0.0f;
        return;
    }
    float ts = PP_timeStep;
    CVector t = {self->turnSpeed.x * ts, self->turnSpeed.y * ts, ts * self->turnSpeed.z};
    if (!self->matrix) {
        CVector forward;
        CVector c;
        PP_CrossProduct(&c, &t, PP_GetForward(self, &forward));
        // (0x85CF1A: the CRT's x87 atan2)
        float angle = float(std::atan2(double(-c.x), double(c.y)));
        self->placement.heading = angle + self->placement.heading;
        return;
    }
    const float* r = PP_GetMatrixLink(self)->m;
    CVector c = {t.y * r[2] - t.z * r[1], t.z * r[0] - t.x * r[2], t.x * r[1] - t.y * r[0]};
    const float* m = PP_GetMatrixLink(self)->m;
    CVector right = {m[0] + c.x, m[1] + c.y, m[2] + c.z};
    float* w = PP_GetMatrixLink(self)->m;
    w[0] = right.x;
    w[1] = right.y;
    w[2] = right.z;
    CVector f0, f1, f2;
    CVector f = {PP_GetForward(self, &f0)->x, PP_GetForward(self, &f1)->y, PP_GetForward(self, &f2)->z};
    c = {t.y * f.z - t.z * f.y, t.z * f.x - t.x * f.z, t.x * f.y - t.y * f.x};
    m = PP_GetMatrixLink(self)->m;
    CVector forward = {m[4] + c.x, m[5] + c.y, m[6] + c.z};
    w = PP_GetMatrixLink(self)->m;
    w[4] = forward.x;
    w[5] = forward.y;
    w[6] = forward.z;
    const float* u = PP_GetMatrixLink(self)->m + 8;
    c = {t.y * u[2] - t.z * u[1], t.z * u[0] - t.x * u[2], t.x * u[1] - u[0] * t.y};
    m = PP_GetMatrixLink(self)->m;
    CVector up = {m[8] + c.x, m[9] + c.y, m[10] + c.z};
    w = PP_GetMatrixLink(self)->m;
    w[8] = up.x;
    w[9] = up.y;
    w[10] = up.z;
}

// 0x46ABE0 vf36 (thiscall): GTA's ApplyGravity (with physicalFlags 2). Unless flagged
// (0x10) or a vehicle the player drives, it needs ground within 10 below one of three
// points 100 up around it, or its speeds are zeroed instead
void __fastcall ApplyGravity(CPhysical* self, re::Edx) {
    if (!(self->physicalFlags & 2)) {
        return;
    }
    uint8_t playerDriving = 0;
    if ((self->typeAndStatus & 7) == TYPE_VEHICLE) {
        void* seats = reinterpret_cast<uint8_t*>(self) + 0x1C4;
        if (PP_Seats_GetOccupant(seats, 0) && Field<int32_t>(PP_Seats_GetOccupant(seats, 0), 0x1310) == 0xD) {
            playerDriving = 1;
        }
    }
    CColPoint cp;
    ColPointClear(&cp);
    if (!((self->physicalFlags >> 4) & 1) && !playerDriving) {
        re::Ptr32<CEntity> hit;  // (uninitialised in the game)
        const float offsets[3][2] = {{0.0f, 0.0f}, {0.1f, 0.0f}, {0.0f, 0.1f}};
        bool grounded = false;
        for (int i = 0; i < 3 && !grounded; i++) {
            CVector offset = {offsets[i][0], offsets[i][1], 100.0f};
            CVector end;
            PP_VectorAdd(&end, &PosOf(self), &offset);
            grounded = PP_ProcessVerticalLine(&end, -10.0f, &cp, &hit, 1, 0) != 0;
        }
        if (!grounded) {
            self->moveSpeed = PP_zeroVector;
            self->turnSpeed = PP_zeroVector;
            ReleaseEntity(&cp);
            return;
        }
    }
    float ts = PP_timeStep;
    double speedZ = self->moveSpeed.z;
    float gravity = VCall<float>(self, 0x8C);
    self->moveSpeed.z = float(speedZ - double(gravity) * ts * self->timeScale);
    CVector com = self->centreOfMass;
    struct { CVector v; float pad; } armPad16;   // (the game's transform writes a 4th float)
    CVector& arm = armPad16.v;
    PP_Multiply3x3(&arm, PP_GetMatrixLink(self), &com);
    ts = PP_timeStep;
    float force = -VCall<float>(self, 0x8C) * ts;
    PP_Physical_ApplyTurnForce(self, 0.0f, 0.0f, force, arm.x, arm.y, arm.z);
    ReleaseEntity(&cp);
}

// 0x46C600 (thiscall): the entities of a sector list tested against it (with the matrix
// it moved from, through its vf41), each once per scan code: those it can collide with
// (the ped, vehicle and projectile exclusions), near enough its bounding sphere (grown
// by its speed when not static); the deepest point into `out`. Any hit
uint8_t __fastcall ProcessCollisionSectorList(CPhysical* self, re::Edx, CMatrix* matrix, const CPtrList* list, CColPoint* out,
                                              int flag) {
    uint16_t scan = PP_scanCode;
    uint8_t result = 0;
    float radius = PP_Physical_GetBoundRadius(self);
    struct {
        CVector centre;
        float radius;
    } sphere;
    PP_Physical_GetBoundingSphere(self, &sphere);
    if (self->bIsStatic == 0) {
        // (0x85C500: the CRT's x87 sqrt)
        float speed = float(std::sqrt(double(std::fabs(SquaredLength(self->moveSpeed)))));
        radius = speed * PP_timeStep + radius;
    }
    for (const CPtrNode* node = list->first; node;) {
        uint32_t bits = node->bits;
        CEntity* other = PP_GetEntityFromNode(int(bits & 0xF), int((bits >> 4) & 0x3FFF));
        node = (bits >> 18) == 0x3FFF ? nullptr : reinterpret_cast<const CPtrNode*>(PP_ptrNodes + (bits >> 18));
        if (other->scanCode == scan) {
            continue;
        }
        other->scanCode = scan;
        CColModel* col = PP_GetColModel(other);
        uint8_t selfType = self->typeAndStatus & 7;
        if (selfType == TYPE_PED) {
            if (other->flagC4 != 0) {
                continue;
            }
            uint8_t otherType = other->typeAndStatus & 7;
            if (otherType == TYPE_PED) {
                if (PP_groundOnly == 1 || re::Flag(flag) || PP_Ped_Fn476200(self, other)) {
                    continue;
                }
            } else if (otherType == TYPE_VEHICLE) {
                if (PP_Ped_Fn4762B0(self, other) || PP_Ped_Fn4763C0(self, other)) {
                    continue;
                }
            }
        } else if (selfType == TYPE_VEHICLE && other->flagC4 != 0) {
            continue;
        }
        if (other == self) {
            continue;
        }
        bool collides = other->bUsesCollision != 0 && (other->flag1C != 0 || other->flag60 != 0) &&
                        (PP_modelInfos[other->modelIndex][0xB] & uint8_t(1u << (PP_chapter & 31))) != 0 &&
                        other->flagF4 == 0;
        if (!collides && other->flag100 == 0) {
            continue;
        }
        CCollisionData* data = col->data;
        if (!data || (data->numTriangles == 0 && data->numSpheres == 0 && data->numBoxes == 0) || data->unknown18) {
            continue;
        }
        if (!PP_Entity_GetIsTouching(other, &sphere.centre, radius)) {
            continue;
        }
        uint8_t ok = 1;
        uint8_t okVehicle = 1;
        uint8_t okAngle = 1;
        if (self->flagC4 != 0) {
            uint8_t otherType = other->typeAndStatus & 7;
            if (otherType == TYPE_PED) {
                ok = PP_Ped_Fn4762F0(other, self) == 0;
            } else if (otherType == TYPE_VEHICLE) {
                okVehicle = PP_Fn46A060(other, self) == 0;
            }
            if (self->modelIndex == other->modelIndex) {
                int model = self->modelIndex;
                if (model == PP_modelBF435C || model == PP_modelBF4358 || model == PP_modelBF4354) {
                    // two of the same kind heading the same way (within 30 degrees) pass through
                    CVector f0, f1, f2, f3;
                    double otherLength = PP_VectorMagnitude(PP_GetForward(other, &f0));
                    CVector* otherForward = PP_GetForward(other, &f1);
                    float dot = PP_DotProduct(PP_GetForward(self, &f2), otherForward);
                    float lengths = float(PP_VectorMagnitude(PP_GetForward(self, &f3)) * otherLength);
                    float cosine = dot / lengths;
                    if (cosine > double(0.866f)) {
                        okAngle = 0;
                    }
                }
            }
        } else if (selfType == TYPE_VEHICLE && other->flagC4 != 0) {
            ok = PP_Fn46A060(self, other) == 0;
        }
        if (!ok || !okVehicle || !okAngle) {
            continue;
        }
        CColPoint point;
        PP_ColPoint_Construct(&point);
        if (VCall<int>(self, 0xA4, matrix, other, &point, flag) != 0) {
            PP_ColPoint_SetEntity(&point, other);
            if (out->depth > point.depth) {
                PP_ColPoint_Assign(out, &point);
                PP_ColPoint_SetEntity(out, other);
                result = 1;
            }
        }
        PP_ColPoint_Destruct(&point);
    }
    return result;
}

// Its sector lists tested from `old` (the entry-info nodes' sectors): the deepest point
// in `best`; any hit
uint8_t TestSectors(CPhysical* self, CMatrix* old, CColPoint* best, CColPoint* current) {
    NextScanCode();
    best->depth = PP_fltMax;
    uint8_t hit = 0;
    for (CEntryInfoNode* node = self->entryInfoList.first; node; node = node->next) {
        if (PP_Physical_NearestWheelHit(self, old, node->sector, current, 0, 1)) {
            hit = 1;
            if (best->depth > current->depth) {
                PP_ColPoint_Assign(best, current);
            }
        }
    }
    return hit;
}

// Back towards `old`'s position along its move, to just short of the hit
void MoveToHit(CPhysical* self, const CMatrix& old, const CColPoint& best, CVector* moved, CVector* step) {
    PP_VectorSub(moved, PP_Placeable_GetPosition(self), &MatrixPos(old));
    float k = float(best.depth - double(0.005f));
    *step = {k * moved->x, moved->y * k, moved->z * k};
    CVector p;
    PP_VectorAdd(&p, &MatrixPos(old), step);
    PP_Entity_SetPosition(self, p.x, p.y, p.z);
}

// 0x46CB90 vf13 (thiscall; its argument unused): GTA's CPhysical::ProcessCollision. Turned
// and moved by its speeds, then while it hits something (from where it was): the
// collision applied, it's moved to the hit and slid along the surface (up to three
// times; projectiles instead stick (one kind) or bounce off). A step up it took is
// undone and the ground under it found again. +0x19C: how far it moved
void __fastcall ProcessCollision(CPhysical* self, re::Edx, int) {
    Field<float>(self, 0x19C) = 0.0f;
    if (self->flagB0 != 0 || self->flag1C == 0) {
        return;
    }
    const CVector& ms = self->moveSpeed;
    const CVector& tsp = self->turnSpeed;
    if (0.0f == ms.x && 0.0f == ms.y && 0.0f == ms.z && 0.0f == tsp.x && 0.0f == tsp.y && 0.0f == tsp.z &&
        (self->typeAndStatus & 0xF8) != 0) {
        Field<float>(self, 0x19C) = 0.0f;
        self->flag34 = 0;
        self->flag38 = 1;
        return;
    }
    CMatrix before;
    PP_Matrix_Copy(&before, PP_Placeable_GetMatrix(self));
    CMatrix old;
    VCall<CMatrix*>(self, 0xA0, &old);
    VCall<void>(self, 0x98);  // ApplyTurnSpeed
    if (self->matrix) {
        PP_Matrix_UpdateRWFull(self->matrix);
    }
    VCall<void>(self, 0x94);  // ApplyMoveSpeed
    CMatrix after;            // (copied, unused)
    PP_Matrix_Copy(&after, PP_Placeable_GetMatrix(self));
    float drop = 0.0f;
    if (old.m[14] > PosOf(self).z && self->flagC4 == 0) {
        // it stepped up: up to where it was (the drop undone below)
        drop = float((PosOf(self).z - old.m[14]) - 0.05);
        PosOf(self).z = float(old.m[14] + 0.05);
    }
    CColPoint best;
    CColPoint current;
    ColPointClear(&best);
    ColPointClear(&current);
    for (int iteration = 0;;) {
        if (!TestSectors(self, &old, &best, &current)) {
            break;
        }
        CEntity* other = best.entity;
        float angle = 0.0f;
        if (self->flagC4 != 0 && (other->typeAndStatus & 7) == TYPE_PED) {
            float heading;
            if (other->matrix) {
                // (0x85CF1A: the CRT's x87 atan2)
                const float* m = other->matrix->m.m;
                heading = float(std::atan2(double(-m[4]), double(m[5])));
            } else {
                heading = other->placement.heading;
            }
            CVector back = {-self->moveSpeed.x, -self->moveSpeed.y, -self->moveSpeed.z};
            float backHeading = PP_GetHeadingOfVector(&back);
            angle = PP_AngleDifference(heading, backHeading);
        }
        PP_Physical_ApplyCollisionPair(self, other, &best, angle);
        CVector moved;
        CVector step;
        MoveToHit(self, old, best, &moved, &step);
        if (iteration == 2) {
            break;
        }
        CVector rest = {moved.x - step.x, moved.y - step.y, moved.z - step.z};
        if (self->flagC4 != 0) {
            if (self->modelIndex == PP_modelBF44DC) {
                // sticks: a tenth into what it hit, static, turned to face along it
                CVector v = self->moveSpeed;
                self->moveSpeed = {0.0f, 0.0f, 0.0f};
                PP_VectorNormalise(&v);
                v = {float(v.x * double(0.1f)), float(v.y * double(0.1f)), float(v.z * double(0.1f))};
                const CVector& p = PosOf(self);
                PP_Entity_SetPosition(self, p.x + v.x, p.y + v.y, p.z + v.z);
                self->physicalFlags &= 0xFD;
                self->bIsStatic = 1;
                self->bUsesCollision = 0;
                float heading = PP_Placeable_GetHeading(self);
                float pitch = PP_RandomFloat(-3.14159274f, 3.14159274f);   // (a random pitch)
                PP_Physical_SetOrientation(self, 0.0f, pitch, heading);
                PP_Fn701640(&PosOf(self));
                PP_Fn701FC0(&PosOf(self), &best.normal);
            } else {
                // bounces: the speed along the normal reversed (elasticity), along the
                // surface kept (friction); peds have their own factors
                uint8_t otherType = other->typeAndStatus & 7;
                CVector n = best.normal;
                if (other->bIsPhysical != 0 && 0.0f < best.normal.z) {
                    n.z = 0.0f;
                }
                PP_VectorNormalise(&n);
                float friction = otherType == TYPE_PED ? PP_pedFriction : Field<float>(self, 0x164);
                float bounce = -(otherType == TYPE_PED ? PP_pedBounce : Field<float>(self, 0x164));
                CVector& v = self->moveSpeed;
                float dot = (n.y * v.y + n.x * v.x) + n.z * v.z;
                CVector along = {dot * n.x, dot * n.y, dot * n.z};
                CVector across;
                PP_VectorSub(&across, &v, &along);
                CVector b = {bounce * along.x, along.y * bounce, along.z * bounce};
                CVector c = {friction * across.x, across.y * friction, across.z * friction};
                CVector sum;
                v = *PP_VectorAdd(&sum, &c, &b);
                if (self->modelIndex == PP_modelBF439C) {
                    float nz = std::fabs(n.z);
                    if (PP_slideMaxNormalZ > nz) {
                        // pushed out along the surface and up, the more the flatter it hit
                        float s = (PP_slideMaxNormalZ - nz) / PP_slideMaxNormalZ;
                        CVector t1, t2;
                        PP_VectorAddTo(&v, PP_VectorScale(&t2, PP_VectorScale(&t1, &n, PP_slidePushOut), s));
                        CVector up = {0.0f, 0.0f, 1.0f};
                        PP_VectorAddTo(&v, PP_VectorScale(&up, PP_VectorScale(&t1, &up, PP_slidePushUp), s));
                    }
                }
            }
            break;
        }
        // slid along the surface: the rest of the move less its part into the normal
        const CVector& n = best.normal;
        CVector neg = {-rest.x, -rest.y, -rest.z};
        float dot = (n.y * neg.y + n.x * neg.x) + n.z * neg.z;
        float t = float(dot + double(0.001f));
        CVector push = {t * n.x, n.y * t, t * n.z};
        rest = {rest.x + push.x, rest.y + push.y, rest.z + push.z};
        CVector q;
        PP_VectorAdd(&q, PP_Placeable_GetPosition(self), &rest);
        PP_Entity_SetPosition(self, q.x, q.y, q.z);
        iteration++;
        if (!(iteration < 3)) {
            break;
        }
    }
    self->physicalFlags2 &= 0xEF;
    if (0.0f > drop) {
        // the step undone: down to the ground below (+0x1BD bit 4 when it found it)
        PP_Matrix_Assign(&old, PP_Placeable_GetMatrix(self));
        PosOf(self).z = PosOf(self).z + drop;
        if (TestSectors(self, &old, &best, &current)) {
            CVector moved;
            CVector step;
            MoveToHit(self, old, best, &moved, &step);
            self->physicalFlags2 |= 0x10;
        }
    }
    PP_Entity_Fn443AA0(self);
    self->flag34 = 0;
    self->flag38 = 1;
    CVector moved;
    PP_VectorSub(&moved, &PosOf(self), &MatrixPos(before));
    // (0x85C500: the CRT's x87 sqrt)
    Field<float>(self, 0x19C) = float(std::sqrt(double(std::fabs(SquaredLength(moved)))));
    PP_Physical_RemoveAndAdd(self);
    ReleaseEntity(&current);
    ReleaseEntity(&best);
}

} // namespace

#define PHYSP_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PHYSP_REVERSED("CPhysical::ApplyAirResistance", 0x00469A10, &ApplyAirResistance);
PHYSP_REVERSED("CPhysical::ProcessControl", 0x00469B70, &ProcessControl);
PHYSP_REVERSED("CPhysical::ApplyTurnSpeed", 0x00469DC0, &ApplyTurnSpeed);
PHYSP_REVERSED("CPhysical::ApplyGravity", 0x0046ABE0, &ApplyGravity);
PHYSP_REVERSED("CPhysical::ProcessCollisionSectorList", 0x0046C600, &ProcessCollisionSectorList);
PHYSP_REVERSED("CPhysical::ProcessCollision", 0x0046CB90, &ProcessCollision);
