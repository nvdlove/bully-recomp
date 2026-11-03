// CPed skitching (0x47DB40): holding on to a car (+0x16C8) at an offset in its space
// (+0x16BC): the car stepped back over the frame in steps of 0.2 at most, the ped pulled
// towards its hold and to the side, its movement applied at each; then the offset
// updated from where the ped ended up, and the ped kept within reach of the hold.

#include <cmath>

#include "game/Ped.h"
#include "reversed.h"

namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;
ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

GAME_VAR(float, PK2_timeStep, 0x00C1A9A4);            // CTimer::ms_fTimeStep
GAME_VAR(double, PK2_d_0, 0x009009A0);
GAME_VAR(float, PK2_f_0_2, 0x00900484);
GAME_VAR(float, PK2_f_0_001, 0x00900C04);
GAME_VAR(double, PK2_d_0_6, 0x00900D50);
GAME_VAR(double, PK2_d_2, 0x00900980);
GAME_VAR(double, PK2_d_0_5, 0x008FF1F8);
GAME_VAR(float, PK2_pullStrength, 0x0090CD94);        // 8.5
GAME_VAR(float, PK2_sideStrength, 0x0090CD98);        // 14
GAME_VAR(double, PK2_d_1, 0x008FF1D8);
GAME_VAR(float, PK2_pullDamping, 0x0090CD9C);         // 0.94
GAME_VAR(float, PK2_f_0_3, 0x00911E30);
GAME_VAR(double, PK2_d_0_2, 0x00900DB8);
GAME_VAR(double, PK2_d_0_85, 0x0090CF78);
GAME_VAR(double, PK2_d_30, 0x00900478);
GAME_VAR(double, PK2_d_0_75, 0x00905408);
GAME_VAR(double, PK2_d_minus0_45, 0x0090D098);
GAME_VAR(float, PK2_f_minus0_45, 0x0090D090);
GAME_VAR(double, PK2_d_0_05, 0x00900C18);
GAME_VAR(float, PK2_f_0_05, 0x009068B8);
GAME_VAR(double, PK2_frontReach, 0x0090D088);         // 0.42
GAME_VAR(float, PK2_rearReach, 0x0090CDB0);           // 0.38
GAME_VAR(double, PK2_d_0_06, 0x0090D080);
GAME_VAR(float, PK2_slowSpeedSq, 0x009034A8);         // 0.005
GAME_VAR(double, PK2_d_0_93, 0x0090D078);
GAME_VAR(double, PK2_d_0_38, 0x0090D070);

GAME_FN(ped::CVector*, __cdecl, PK2_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(ped::CVector*, __cdecl, PK2_VectorAdd, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(float, __cdecl, PK2_Min, 0x00414470, float a, float b);
GAME_FN(float*, __thiscall, PK2_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(ped::CVector*, __thiscall, PK2_GetPosition, 0x00415460, void* entity);
GAME_FN(float, __thiscall, PK2_VectorNormalise, 0x00414E10, ped::CVector* v);
GAME_FN(ped::CVector*, __thiscall, PK2_SimpleTransformGetRight, 0x00473600, void* transform, ped::CVector* out);
GAME_FN(void, __thiscall, PK2_ApplyPush, 0x004734E0, CPed* ped, float strength, const ped::CVector* dir,
        const ped::CVector* point, float fraction, float scale, int onlyWhenBelow);
GAME_FN(ped::CVector*, __cdecl, PK2_CrossProduct, 0x00414730, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(void, __thiscall, PK2_Fn46C3C0, 0x0046C3C0, CPed* ped, float a, const ped::CVector* dir,
        const ped::CVector* point, const ped::CVector* velocity);
GAME_FN(void*, __cdecl, PK2_MatrixInvert, 0x00413770, const void* matrix, void* out);
GAME_FN(void, __thiscall, PK2_MatrixCopy, 0x00412A10, void* self, const void* from);
GAME_FN(ped::CVector*, __thiscall, PK2_GetForward, 0x004154A0, void* entity, ped::CVector* out);

// A placeable's forward: its matrix's, or from its heading
ped::CVector Forward(uint8_t* e) {
    uint8_t* matrix = At<uint8_t*>(e, 0x14);
    if (matrix) {
        return At<ped::CVector>(matrix, 0x10);
    }
    float heading = At<float>(e, 0x10);
    ped::CVector f;
    f.x = -static_cast<float>(std::sin(static_cast<double>(heading)));
    f.y = static_cast<float>(std::cos(static_cast<double>(heading)));
    f.z = 0.0f;
    return f;
}
// ... and its right (0x473600 on its simple transform without a matrix)
ped::CVector Right(uint8_t* e) {
    uint8_t* matrix = At<uint8_t*>(e, 0x14);
    if (matrix) {
        return At<ped::CVector>(matrix, 0);
    }
    ped::CVector r;
    PK2_SimpleTransformGetRight(e + 4, &r);
    return r;
}
// Its position moved (the matrix's fourth float the game writes from uninitialised
// stack; ours leaves it)
void SetPos(uint8_t* e, float x, float y, float z) {
    uint8_t* matrix = At<uint8_t*>(e, 0x14);
    if (matrix) {
        At<float>(matrix, 0x30) = x;
        At<float>(matrix, 0x34) = y;
        At<float>(matrix, 0x38) = z;
    }
    At<float>(e, 4) = x;
    At<float>(e, 8) = y;
    At<float>(e, 0xC) = z;
}
// A vector turned by a matrix (rotation only)
ped::CVector Rotate(const float* m, const ped::CVector& o) {
    ped::CVector w;
    w.x = o.y * m[4] + o.x * m[0] + o.z * m[8];
    w.y = m[5] * o.y + m[1] * o.x + m[9] * o.z;
    w.z = o.y * m[6] + o.x * m[2] + o.z * m[10];
    return w;
}
float Length2D(const ped::CVector& v) {
    return static_cast<float>(std::sqrt(static_cast<double>(std::fabs(v.y * v.y + v.x * v.x))));
}

// The two ends: the ped snapped back to `reach` of the hold (its height kept)
void SnapToHold(CPed* self, const ped::CVector& forward, const ped::CVector& hold, const ped::CVector& along,
                const ped::CVector& lift, double reach) {
    ped::CVector back = {-forward.x, -forward.y, -forward.z};
    ped::CVector k;
    k.x = static_cast<float>(forward.x * reach);
    k.y = static_cast<float>(forward.y * reach);
    k.z = static_cast<float>(forward.z * reach);
    float d = back.x * along.x + back.y * along.y + static_cast<float>(back.z * PK2_d_0);
    ped::CVector e = {back.x * d, d * back.y, d * back.z};
    ped::CVector x;
    ped::CVector y;
    ped::CVector z;
    ped::CVector n;
    PK2_VectorAdd(&n, PK2_VectorSub(&z, PK2_VectorSub(&y, PK2_VectorAdd(&x, &hold, &along), &e), &k), &lift);
    SetPos(Bytes(self), n.x, n.y, n.z);
}

// Pushed along with the car when slower than it (by how aligned they are)
void MatchCarSpeed(CPed* self, uint8_t* car) {
    ped::CVector t1;
    ped::CVector t2;
    ped::CVector* carForward = PK2_GetForward(car, &t1);
    ped::CVector* forward = PK2_GetForward(self, &t2);
    float align = std::fabs(forward->y * carForward->y + forward->x * carForward->x + forward->z * carForward->z);
    const ped::CVector& carSpeed = At<ped::CVector>(car, 0x124);
    ped::CVector b;
    b.x = static_cast<float>(carSpeed.x * PK2_d_0_06);
    b.y = static_cast<float>(carSpeed.y * PK2_d_0_06);
    b.z = static_cast<float>(PK2_d_0_06 * carSpeed.z);
    ped::CVector a = {b.x * align, align * b.y, align * b.z};
    float ts = PK2_timeStep;
    ped::CVector t = {a.x * ts, a.y * ts, ts * a.z};
    ped::CVector& speed = At<ped::CVector>(self, 0x124);
    speed.x = t.x + speed.x;
    speed.y = speed.y + t.y;
    speed.z = speed.z + t.z;
}

// 0x47DB40 (thiscall)
void __fastcall CPed_UpdateSkitching(CPed* self, re::Edx) {
    uint8_t* car = At<uint8_t*>(self, 0x16C8);
    ped::CVector carStart = *PosOf(car);
    float frame = PK2_timeStep;
    const ped::CVector& carSpeed0 = At<ped::CVector>(car, 0x124);
    ped::CVector step = {frame * carSpeed0.x, carSpeed0.y * frame, carSpeed0.z * frame};
    ped::CVector base;
    PK2_VectorSub(&base, &carStart, &step);
    float total = PK2_timeStep;
    float elapsed = 0.0f;
    double totalD = total;
    if (total > PK2_d_0) {
        for (;;) {
            if (elapsed < total) {
                PK2_timeStep = PK2_Min(total, PK2_f_0_2);
            } else {
                PK2_timeStep = elapsed - total;
                if (PK2_timeStep < PK2_f_0_001) {
                    break;
                }
            }
            float ts = PK2_timeStep;
            car = At<uint8_t*>(self, 0x16C8);
            const ped::CVector& carSpeed = At<ped::CVector>(car, 0x124);
            ped::CVector move = {ts * carSpeed.x, carSpeed.y * ts, carSpeed.z * ts};
            ped::CVector moved;
            ped::CVector* p = PK2_VectorAdd(&moved, &base, &move);
            SetPos(car, p->x, p->y, p->z);
            // the hold, in the world
            ped::CVector offset = At<ped::CVector>(self, 0x16BC);
            ped::CVector w = Rotate(PK2_GetMatrix(At<uint8_t*>(self, 0x16C8)), offset);
            car = At<uint8_t*>(self, 0x16C8);
            ped::CVector hold;
            PK2_VectorAdd(&hold, PosOf(car), &w);
            ped::CVector f = Forward(car);
            ped::CVector ahead;
            ahead.x = static_cast<float>(f.x * PK2_d_0_6);
            ahead.y = static_cast<float>(f.y * PK2_d_0_6);
            ahead.z = static_cast<float>(PK2_d_0_6 * f.z);
            ped::CVector* pos = PK2_GetPosition(self);
            ped::CVector target;
            PK2_VectorAdd(&target, &hold, &ahead);
            ped::CVector d;
            PK2_VectorSub(&d, &target, pos);
            // the pull: the car's forward, turned towards the target
            ped::CVector pull = Forward(At<uint8_t*>(self, 0x16C8));
            ped::CVector dn = d;
            PK2_VectorNormalise(&dn);
            ped::CVector f3 = Forward(At<uint8_t*>(self, 0x16C8));
            ped::CVector d2;
            d2.x = static_cast<float>(d.x * PK2_d_2);
            d2.y = static_cast<float>(d.y * PK2_d_2);
            d2.z = static_cast<float>(PK2_d_2 * d.z);
            float off = std::fabs(f3.x * dn.x + dn.y * f3.y + dn.z * f3.z);
            float k = 1.0f - off;
            ped::CVector kv = {d2.x * k, k * d2.y, k * d2.z};
            pull.x = kv.x + pull.x;
            pull.y = kv.y + pull.y;
            pull.z = kv.z + pull.z;
            PK2_VectorNormalise(&pull);
            ped::CVector flatPull = pull;
            float along = d.x * pull.x + d.y * pull.y + d.z * pull.z;
            flatPull.z = 0.0f;
            PK2_VectorNormalise(&flatPull);
            ped::CVector r = Right(At<uint8_t*>(self, 0x16C8));
            float side = d.x * r.x + r.y * d.y + r.z * d.z;
            ped::CVector r2 = Right(At<uint8_t*>(self, 0x16C8));
            r2.z = 0.0f;
            PK2_VectorNormalise(&r2);
            // where on the ped it's pulled: half a step behind or ahead of it
            ped::CVector point;
            ped::CVector own = Forward(Bytes(self));
            if (At<int32_t>(self, 0x1488) != 0) {
                ped::CVector back = {-own.x, -own.y, -own.z};
                point.x = static_cast<float>(back.x * PK2_d_0_5);
                point.y = static_cast<float>(back.y * PK2_d_0_5);
                point.z = static_cast<float>(PK2_d_0_5 * back.z);
            } else {
                point.x = static_cast<float>(own.x * PK2_d_0_5);
                point.y = static_cast<float>(own.y * PK2_d_0_5);
                point.z = static_cast<float>(PK2_d_0_5 * own.z);
            }
            PK2_ApplyPush(self, PK2_pullStrength, &flatPull, &point, along, 1.0f, 1);
            float sideAmount = static_cast<float>(side + PK2_d_1);
            PK2_ApplyPush(self, PK2_sideStrength, &r2, &point, sideAmount, 1.0f, 0);
            ped::CVector arm = point;
            ped::CVector spin;
            PK2_CrossProduct(&spin, &At<ped::CVector>(self, 0x130), &arm);
            ped::CVector pointSpeed;
            PK2_VectorAdd(&pointSpeed, &spin, &At<ped::CVector>(self, 0x124));
            PK2_Fn46C3C0(self, PK2_pullDamping, &flatPull, &point, &pointSpeed);
            PK2_Fn46C3C0(self, PK2_f_0_3, &r2, &point, &pointSpeed);
            reinterpret_cast<void(__thiscall*)(CPed*)>(static_cast<void*>(self->vtable[0x94 / 4]))(self);
            elapsed = static_cast<float>(elapsed + PK2_d_0_2);
            if (!(elapsed < totalD)) {
                break;
            }
        }
    }
    car = At<uint8_t*>(self, 0x16C8);
    SetPos(car, carStart.x, carStart.y, carStart.z);
    PK2_timeStep = total;
    float z = PosOf(self)->z;
    // the offset: across the car from where the ped is, kept within -0.45 .. 0.05
    alignas(4) uint8_t inverse[0x44] = {};
    alignas(4) uint8_t local[0x44];
    PK2_MatrixCopy(local, PK2_MatrixInvert(PK2_GetMatrix(At<uint8_t*>(self, 0x16C8)), inverse));
    ped::CVector rel;
    PK2_VectorSub(&rel, PosOf(self), PosOf(At<uint8_t*>(self, 0x16C8)));
    const float* li = reinterpret_cast<const float*>(local);
    float across = li[0] * rel.x + li[4] * rel.y + li[8] * rel.z;
    if (std::fabs(across) < PK2_d_0_85) {
        const ped::CVector& speed = At<ped::CVector>(self, 0x124);
        float s1 = Length2D(speed);
        ped::CVector f6 = Forward(Bytes(self));
        ped::CVector a6 = {f6.x * s1, s1 * f6.y, s1 * f6.z};
        car = At<uint8_t*>(self, 0x16C8);
        float s2 = Length2D(At<ped::CVector>(car, 0x124));
        ped::CVector r6 = Right(car);
        ped::CVector b6 = {s2 * r6.x, s2 * r6.y, s2 * r6.z};
        float cross = std::fabs(b6.y * a6.y + b6.x * a6.x + b6.z * a6.z);
        At<float>(self, 0x16BC) = static_cast<float>(PK2_d_30 * across * cross + across * PK2_d_0_75);
    }
    ped::CVector offset = At<ped::CVector>(self, 0x16BC);
    ped::CVector w = Rotate(PK2_GetMatrix(At<uint8_t*>(self, 0x16C8)), offset);
    car = At<uint8_t*>(self, 0x16C8);
    ped::CVector hold;
    PK2_VectorAdd(&hold, PosOf(car), &w);
    ped::CVector toPed;
    PK2_VectorSub(&toPed, PosOf(self), &hold);
    ped::CVector f7 = Forward(At<uint8_t*>(self, 0x16C8));
    f7.z = 0.0f;
    PK2_VectorNormalise(&f7);
    ped::CVector lift = {0.0f, 0.0f, toPed.z};
    toPed.z = 0.0f;
    float v = At<float>(self, 0x16BC);
    if (v < PK2_d_minus0_45) {
        v = PK2_f_minus0_45;
    } else if (v > PK2_d_0_05) {
        v = PK2_f_0_05;
    }
    At<float>(self, 0x16BC) = v;
    ped::CVector back = {-f7.x, -f7.y, -f7.z};
    float behind = back.x * toPed.x + back.y * toPed.y + static_cast<float>(back.z * PK2_d_0);
    car = At<uint8_t*>(self, 0x16C8);
    const ped::CVector& carSpeed = At<ped::CVector>(car, 0x124);
    const ped::CVector& speed = At<ped::CVector>(self, 0x124);
    if (PK2_frontReach < behind) {
        float carAlong = carSpeed.y * f7.y + f7.x * carSpeed.x + carSpeed.z * f7.z;
        float along = f7.x * speed.x + speed.y * f7.y + speed.z * f7.z;
        if (along < carAlong) {
            MatchCarSpeed(self, car);
        }
        SnapToHold(self, f7, hold, toPed, lift, PK2_frontReach);
    } else if (behind < PK2_rearReach) {
        float carAlong = carSpeed.y * f7.y + f7.x * carSpeed.x + carSpeed.z * f7.z;
        float along = f7.y * speed.y + f7.x * speed.x + speed.z * f7.z;
        if (along < carAlong) {
            MatchCarSpeed(self, car);
        } else if (carSpeed.y * carSpeed.y + carSpeed.x * carSpeed.x < PK2_slowSpeedSq) {
            float damping = static_cast<float>(std::pow(PK2_d_0_93, static_cast<double>(PK2_timeStep)));
            ped::CVector& s = At<ped::CVector>(self, 0x124);
            s.x = damping * s.x;
            s.y = damping * s.y;
            s.z = damping * s.z;
        }
        SnapToHold(self, f7, hold, toPed, lift, PK2_d_0_38);
    }
    // (the height it had before the snaps)
    uint8_t* matrix = self->matrix;
    if (matrix) {
        At<float>(matrix, 0x38) = z;
    } else {
        self->position.z = z;
    }
}

REVERSED("CPed::UpdateSkitching", 0x0047DB40, RE_BODY(&CPed_UpdateSkitching), nullptr, nullptr,
         re::Mode::Original);

} // namespace
