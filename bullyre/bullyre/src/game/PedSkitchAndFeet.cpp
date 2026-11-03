// CPed (0x479B50 .. 0x47A6FF): grabbing a car to skitch behind, the level-ground probe,
// sliding on slopes, and the feet's placement (IK) each frame.

#include <cmath>

#include "game/Ped.h"
#include "reversed.h"

namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;
const ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

constexpr int PED_TYPE_PLAYER = 0xD;

GAME_VAR(uint32_t, PK_timeInMs, 0x00C1A9B4);
GAME_VAR(float, PK_fltMax, 0x009009B8);
// skitching
GAME_VAR(double, PK_d_1_5, 0x008FFA08);
GAME_VAR(double, PK_d_0_8, 0x00900AD0);
GAME_VAR(double, PK_d_0_4, 0x00900DE8);
GAME_VAR(float, PK_grabRadius, 0x0090048C);           // 0.4
GAME_VAR(double, PK_d_0_65, 0x0090CF90);
GAME_VAR(float, PK_castRadius, 0x0090CD88);           // 0.25
GAME_VAR(double, PK_d_minus0_8, 0x0090CF88);
GAME_VAR(double, PK_d_0_707, 0x0090CF80);
GAME_VAR(double, PK_d_0_85, 0x0090CF78);
GAME_VAR(double, PK_d_0_75, 0x00905408);
GAME_VAR(double, PK_d_0_02, 0x0090BFF8);
// ground and sliding
GAME_VAR(float, PK_minGroundUp, 0x00C2B1A4);          // a normal's z above this is level ground
GAME_VAR(float, PK_minGroundUpPlayer, 0x00C2B1A8);
GAME_VAR(int32_t, PK_C2B10C, 0x00C2B10C);
GAME_VAR(double, PK_d_0_1, 0x00900128);
GAME_VAR(double, PK_d_0_5, 0x008FF1F8);
GAME_VAR(float, PK_f_3, 0x00900C14);
GAME_VAR(double, PK_d_3, 0x00900988);
GAME_VAR(double, PK_d_1, 0x008FF1D8);
// feet
GAME_VAR(float, PK_boneRaise, 0x00C2B12C);
GAME_VAR(uint8_t, PK_raiseBones, 0x00C2B130);
GAME_VAR(int32_t, PK_probeFeet, 0x00C2B1C0);
GAME_VAR(int32_t, PK_updateAnims, 0x00C2B1BC);
GAME_VAR(float, PK_f_2p32, 0x00900AC8);
GAME_VAR(double, PK_d_0_001, 0x00900FA0);

GAME_FN(uint8_t, __cdecl, PK_ProcessSphereCast, 0x004616A0, const ped::CVector* start, const ped::CVector* end,
        float radius, void* colPoint, void* entity, int a, int flags, int b);
GAME_FN(void*, __cdecl, PK_GetColModel, 0x0051AF50, void* entity);
GAME_FN(void*, __thiscall, PK_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(uint8_t, __cdecl, PK_SphereCastAgainst, 0x0056A060, const ped::CVector* start, const ped::CVector* end,
        float radius, void* matrix, void* colModel, void* colPoint, void* out, int a, int flags, int b);
GAME_FN(ped::CVector*, __thiscall, PK_GetForward, 0x004154A0, void* entity, ped::CVector* out);
GAME_FN(ped::CVector*, __thiscall, PK_GetUp, 0x00473730, void* entity, ped::CVector* out);
GAME_FN(float, __cdecl, PK_DotProduct, 0x00414640, const ped::CVector* a, const ped::CVector* b);
GAME_FN(void*, __cdecl, PK_MatrixInvert, 0x0041A890, void* out, const void* matrix);
GAME_FN(ped::CVector*, __cdecl, PK_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PK_Multiply3x3, 0x0041C490, ped::CVector* out, const void* matrix,
        const ped::CVector* v);
GAME_FN(void*, __thiscall, PK_HandleConstruct, 0x00472CA0, void* handle, void* entity);
GAME_FN(void, __thiscall, PK_HandleSet, 0x005D7F10, void* handle, uintptr_t value);
GAME_FN(void, __cdecl, PK_Fn5143D0, 0x005143D0, const char* name);
GAME_FN(void, __thiscall, PK_ColPointDestruct, 0x00415500, void* colPoint);
GAME_FN(void, __cdecl, PK_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(ped::CVector*, __cdecl, PK_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(uint8_t, __cdecl, PK_ProcessLineOfSight, 0x00461EA0, const ped::CVector* start, const ped::CVector* end,
        void* colPoint, void* entity, int a, int b, int c);
GAME_FN(ped::CVector*, __cdecl, PK_NormalisedCopy, 0x00472D40, ped::CVector* out, const ped::CVector* in);
GAME_FN(void, __cdecl, PK_ProbeGround, 0x00479FC0, ped::CVector pos, ped::CVector dir, uint8_t* out);
GAME_FN(void, __thiscall, PK_TrackConstruct, 0x0061A880, void* track);
GAME_FN(void, __thiscall, PK_SlideTrackInit, 0x00608740, void* track);
GAME_FN(void, __thiscall, PK_TrackSetOwner, 0x0061AAA0, void* track, void* owner);
GAME_FN(void, __thiscall, PK_SlideTrackUpdate, 0x00615570, void* track, float dt);
GAME_FN(void, __thiscall, PK_TrackFn7FFDC0, 0x007FFDC0, void* track);
GAME_FN(void, __thiscall, PK_SlideTrackDestruct, 0x00608760, void* track);
GAME_FN(uint8_t, __cdecl, PK_ProcessVerticalLineAtBone, 0x00473760, void* object, int bone, void* colPoint, float up,
        float down);
GAME_FN(void, __thiscall, PK_ColPointAssign, 0x0046AA70, void* colPoint, const void* from);
GAME_FN(void, __cdecl, PK_PlaceFoot, 0x00475F80, CPed* ped, void* plane, void* foot);
GAME_FN(void, __thiscall, PK_AnimFootFn6BA380, 0x006BA380, void* animBlend, void* foot, float a);
GAME_FN(void, __thiscall, PK_AnimFn6B93A0, 0x006B93A0, void* animBlend, float time, int a);

inline void* const PK_slideTrackVtable = reinterpret_cast<void*>(0x0090CE38);  // LocomotionSlideTrack

// A CColPoint as the code here uses it (0x28 bytes; +0xC a registered entity)
void ColPointInit(uint8_t* cp) {
    At<ped::CVector>(cp, 0) = {0.0f, 0.0f, 0.0f};
    At<void*>(cp, 0xC) = nullptr;
    At<ped::CVector>(cp, 0x10) = {0.0f, 0.0f, 0.0f};
    At<float>(cp, 0x24) = PK_fltMax;
}
void ColPointRelease(uint8_t* cp) {
    re::Ptr32<void>& entity = At<void*>(cp, 0xC);
    if (entity) {
        PK_CleanUpOldReference(entity, &entity);
    }
}

// 0x479B50 (thiscall): skitching: a car ahead (behind with +0x1488) within reach, its
// back facing the ped, and the ped behind its middle: its offset in the car's space kept
// (+0x16BC) and the car held (+0x16C8)
uint8_t __fastcall CPed_TryGrabVehicle(CPed* self, re::Edx) {
    // (the vectors' fourth floats and some col point fields are uninitialised stack in
    // the game; ours are 0)
    alignas(4) uint8_t colPoint[0x28] = {};
    alignas(4) uint8_t colPoint2[0x28] = {};
    alignas(4) uint8_t inverse[0x44] = {};
    uint8_t castOut[4] = {};
    void* entity = nullptr;
    ped::CVector pos = *PosOf(self);
    float s = static_cast<float>(std::sin(static_cast<double>(self->desiredHeading)));
    float dx = static_cast<float>(-s * PK_d_1_5);
    float c = static_cast<float>(std::cos(static_cast<double>(self->desiredHeading)));
    float dy = static_cast<float>(c * PK_d_1_5);
    ped::CVector end;
    float z0;
    if (At<int32_t>(self, 0x1488) != 0) {
        end.x = pos.x - dx;
        end.y = pos.y - dy;
        z0 = pos.z - 0.0f;
    } else {
        end.x = dx + pos.x;
        end.y = dy + pos.y;
        z0 = pos.z + 0.0f;
    }
    ped::CVector start = {pos.x, pos.y, static_cast<float>(pos.z + PK_d_0_8 + PK_d_0_4)};
    end.z = z0 + 0.0f;
    ColPointInit(colPoint);
    if (!PK_ProcessSphereCast(&start, &end, PK_grabRadius, colPoint, &entity, 2, 0xF, 1)) {
        PK_HandleSet(Bytes(self) + 0x16C8, 0);
        ColPointRelease(colPoint);
        return 0;
    }
    void* colModel = PK_GetColModel(entity);
    ColPointInit(colPoint2);
    ped::CVector hit = At<ped::CVector>(colPoint, 0);
    float top = static_cast<float>(pos.z + PK_d_0_65);
    float z = top > hit.z ? hit.z : top;
    start = {pos.x, pos.y, z};
    end = {hit.x, hit.y, z};
    bool grabbed = false;
    if (PK_SphereCastAgainst(&start, &end, PK_castRadius, PK_GetMatrix(entity), colModel, colPoint2, castOut, 8, 0xF,
                             1) &&
        At<int32_t>(entity, 0x46C) == 0 && entity != At<void*>(self, 0x1758)) {
        const ped::CVector& n = At<ped::CVector>(colPoint2, 0x10);
        ped::CVector tmp;
        ped::CVector* f = PK_GetForward(entity, &tmp);
        float facing = f->y * n.y + f->x * n.x + f->z * n.z;
        bool ok = true;
        if (!(facing < PK_d_minus0_8)) {
            ok = PK_DotProduct(&n, PK_GetUp(entity, &tmp)) > PK_d_0_707;
        }
        if (ok) {
            PK_MatrixInvert(inverse, PK_GetMatrix(entity));
            ped::CVector diff;
            PK_VectorSub(&diff, &At<ped::CVector>(colPoint2, 0), PosOf(entity));
            ped::CVector local;
            ped::CVector* l = PK_Multiply3x3(&local, inverse, &diff);
            ped::CVector& offset = At<ped::CVector>(self, 0x16BC);
            offset = *l;
            if (std::fabs(offset.x) < PK_d_0_85) {
                offset.x = static_cast<float>(offset.x * PK_d_0_75);
                offset.y = static_cast<float>(offset.y + PK_d_0_02);
                PK_HandleConstruct(Bytes(self) + 0x16C8, entity);
                PK_Fn5143D0(reinterpret_cast<const char*>(0x0090CF68));  // "USE_SKITCH"
                PK_ColPointDestruct(colPoint2);
                PK_ColPointDestruct(colPoint);
                grabbed = true;
            }
        }
    }
    if (grabbed) {
        return 1;
    }
    ColPointRelease(colPoint2);
    PK_HandleSet(Bytes(self) + 0x16C8, 0);
    ColPointRelease(colPoint);
    return 0;
}

// 0x479FC0 (cdecl): `*out` 0 when the ground ahead (a line from 1 above `pos` to 1 below
// `pos + dir`) and to both sides is level with `pos` (within 0.25) and one of the three
// is level ground; 1 otherwise
void __cdecl ProbeGroundAhead(ped::CVector pos, ped::CVector dir, uint8_t* out) {
    alignas(4) uint8_t colPoint[0x28] = {};
    void* entity = nullptr;  // (written by the lines, never released)
    float z = pos.z;
    *out = 1;
    ped::CVector end;
    PK_VectorAdd(&end, &pos, &dir);
    pos.z = pos.z + 1.0f;
    end.z = end.z - 1.0f;
    ColPointInit(colPoint);
    const ped::CVector& hit = At<ped::CVector>(colPoint, 0);
    const float& normalZ = At<float>(colPoint, 0x18);
    if (!PK_ProcessLineOfSight(&pos, &end, colPoint, &entity, 9, 0, 0) || !(std::fabs(z - hit.z) < PK_castRadius)) {
        ColPointRelease(colPoint);
        return;
    }
    bool levelA = PK_minGroundUp < normalZ;
    ped::CVector side = {-dir.y, dir.x, 0.0f};
    ped::CVector left;
    PK_VectorAdd(&left, &pos, &side);
    left.z = end.z;
    At<float>(colPoint, 0x24) = PK_fltMax;
    if (!PK_ProcessLineOfSight(&pos, &left, colPoint, &entity, 9, 0, 0) || !(std::fabs(z - hit.z) < PK_castRadius)) {
        ColPointRelease(colPoint);
        return;
    }
    bool levelB = PK_minGroundUp < normalZ;
    side = {dir.y, -dir.x, 0.0f};
    ped::CVector right;
    PK_VectorAdd(&right, &pos, &side);
    right.z = end.z;
    At<float>(colPoint, 0x24) = PK_fltMax;
    if (PK_ProcessLineOfSight(&pos, &right, colPoint, &entity, 9, 0, 0) && std::fabs(z - hit.z) < PK_castRadius) {
        bool levelC = PK_minGroundUp < normalZ;
        if (levelA || levelB || levelC) {
            *out = 0;
        }
    }
    ColPointRelease(colPoint);
}

// 0x47A280 (thiscall): the slide timer (+0x18D8, up to 3): grows on a slope while too
// slow (+0x1744) and moving onto sloped ground, decays otherwise; while it runs, a
// LocomotionSlideTrack is run on the ped
void __fastcall CPed_UpdateSlide(CPed* self, re::Edx, float dt) {
    float& slide = At<float>(self, 0x18D8);
    if (At<int32_t>(self, 0x1530) != 0) {
        slide = 0.0f;
        return;
    }
    float speed = At<float>(self, 0x1744);
    float limit = (self->pedType == PED_TYPE_PLAYER && At<uint8_t>(self, 0x130C)) ? PK_minGroundUpPlayer : PK_minGroundUp;
    if (At<int32_t>(self, 0x1758) == 0 || !(limit > speed) || At<uint8_t>(self, 0x16CC) != 0) {
        float r = static_cast<float>(slide - (slide / PK_d_3 + PK_d_1) * dt);
        slide = r;
        if (r < 0.0f) {
            slide = 0.0f;
        }
    } else {
        uint8_t sloped = 0;
        bool notPlayerOrMoving = self->pedType != PED_TYPE_PLAYER || PK_C2B10C != 0;
        bool probe;
        if (At<uint8_t>(self, 0x130C) == 0) {
            probe = At<uint8_t>(self, 0x130D) != 0;
        } else {
            probe = notPlayerOrMoving || At<float>(self->intelligence, 0x1C) < PK_d_0_1;
        }
        if (probe) {
            ped::CVector forward;
            ped::CVector unit;
            ped::CVector* n = PK_NormalisedCopy(&unit, PK_GetForward(self, &forward));
            ped::CVector dir;
            dir.x = static_cast<float>(n->x * PK_d_0_5);
            dir.y = static_cast<float>(PK_d_0_5 * n->y);
            dir.z = static_cast<float>(PK_d_0_5 * n->z);
            PK_ProbeGround(*PosOf(self), dir, &sloped);
        }
        if (probe && sloped) {
            float r = dt + slide;
            slide = r;
            if (PK_f_3 < r) {
                slide = PK_f_3;
            }
        } else {
            slide = 0.0f;
        }
    }
    if (0.0f < slide) {
        alignas(4) uint8_t track[0x24];
        PK_TrackConstruct(track);
        At<void*>(track, 0) = PK_slideTrackVtable;
        PK_SlideTrackInit(track);
        PK_TrackSetOwner(track, self->intelligence + 0x300);
        PK_SlideTrackUpdate(track, dt);
        PK_TrackFn7FFDC0(track);
        PK_SlideTrackDestruct(track);
    }
}

// 0x47A4C0 (thiscall): the feet each frame (+0x16E0 right, +0x16E4 left): released
// without +0x13A0; else (optionally the bones raised) each foot's ground found below it
// (or +0x174C's), the foot placed, and the anim told
void __fastcall CPed_UpdateFeet(CPed* self, re::Edx) {
    if (At<int32_t>(self, 0x13A0) == 0) {
        static const uint32_t offsets[] = {0x16E0, 0x16E4};
        for (uint32_t offset : offsets) {
            uint8_t* foot = At<uint8_t*>(self, offset);
            if (foot) {
                At<uint32_t>(foot, 0x38) &= ~1u;
                At<float>(foot, 0x2C) = 0.0f;
                At<float>(foot, 0x28) = 0.0f;
            }
        }
        return;
    }
    if (PK_raiseBones) {
        float raise = PK_boneRaise;
        uint8_t* bones = At<uint8_t*>(self->animBlend, 0x14);
        int count = At<int32_t>(bones, 4);
        uint8_t* array = At<uint8_t*>(bones, 8);
        for (int i = 0; i < count; i++) {
            At<float>(array, i * 0x34 + 0x2C) = At<float>(array, i * 0x34 + 0x2C) + raise;
        }
    }
    static const uint32_t feet[2][2] = {{0x16E0, 0x183C}, {0x16E4, 0x1814}};
    for (const auto& pair : feet) {
        uint8_t* foot = At<uint8_t*>(self, pair[0]);
        if (!foot) {
            continue;
        }
        uint8_t* plane = Bytes(self) + pair[1];
        if (PK_probeFeet == 0 ||
            !PK_ProcessVerticalLineAtBone(self->animBlend, At<int32_t>(At<uint8_t*>(foot, 0), 0x24) + 1, plane, 1.0f,
                                          1.0f)) {
            PK_ColPointAssign(plane, Bytes(self) + 0x174C);
        }
        PK_PlaceFoot(self, plane, At<uint8_t*>(self, pair[0]));
        foot = At<uint8_t*>(self, pair[0]);
        if (At<uint8_t>(foot, 0x38) & 1) {
            PK_AnimFootFn6BA380(self->animBlend, foot, 0.0f);
        }
    }
    if (PK_updateAnims != 0) {
        double now = static_cast<double>(static_cast<int32_t>(PK_timeInMs));
        if (static_cast<int32_t>(PK_timeInMs) < 0) {
            now = now + PK_f_2p32;
        }
        PK_AnimFn6B93A0(self->animBlend, static_cast<float>(now * PK_d_0_001), 1);
    }
}

#define PEDSKITCH_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDSKITCH_REVERSED("CPed::TryGrabVehicle", 0x00479B50, &CPed_TryGrabVehicle);
PEDSKITCH_REVERSED("ProbeGroundAhead", 0x00479FC0, &ProbeGroundAhead);
PEDSKITCH_REVERSED("CPed::UpdateSlide", 0x0047A280, &CPed_UpdateSlide);
PEDSKITCH_REVERSED("CPed::UpdateFeet", 0x0047A4C0, &CPed_UpdateFeet);

} // namespace
