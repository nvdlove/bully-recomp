// The action tree's AI bike-fleeing rides: FleeOnRoadOnBikeTrack (along the road nodes,
// away from the threat) and FleeOnPathOnBikeTrack's ride (along its patrol path).
// 0x621E60..0x621F10, 0x62F140.
// A flee-on-road track: +0x18 the owner, +0x1C a flag, +0x20 the road node headed for
// (+8/+0xA/+0xC its position in 1/8 units), +0x24 the one before.
// Both rides steer the ped by its stats: 0x1B / 0x1D the near / far distances when
// heading at the threat, 0x1C the distance when heading away, 0x18 / 0x19 / 0x1A /
// 0x23 / 0x25 speeds (in hundredths), 0xE the skill (< 0x33 sidesteps a closing threat,
// > 0x4A cuts across one alongside).
// Register-hazard check (tools/regcheck/regcheck2.py, chunk 31): no caller keeps ecx/edx.

#include <cstdint>

#include "game/ActionTracksCommon.h"

using namespace actc;

namespace {

GAME_VAR(void*, patrolPaths, 0x00D13CB4);
inline void* const roadPaths = reinterpret_cast<void*>(0x00BF6228);   // ThePaths

GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
GAME_FN(uint8_t*, __thiscall, Scanner_GetThreat, 0x0048C070, uint8_t* scanner);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(V3*, __thiscall, Entity_GetRight, 0x004736F0, uint8_t* entity, V3* out);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Paths_FindNodeFacing_x64, 0x00441E90, void* paths, float, float, float, int type, float dirX, float dirY);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* Paths_FindNodeFacing(void* a0, V3 a1, int a2, float a3, float a4) {
    return Paths_FindNodeFacing_x64(a0, a1.x, a1.y, a1.z, a2, a3, a4);
}
// the road node after `node` from `pos`, not back to `previous`
GAME_FN(uint8_t*, __thiscall, PathNode_PickNext_x64, 0x004401C0, uint8_t* node, float, float, float, uint8_t* previous);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* PathNode_PickNext(uint8_t* a0, V3 a1, uint8_t* a2) {
    return PathNode_PickNext_x64(a0, a1.x, a1.y, a1.z, a2);
}
GAME_FN(uint8_t*, __thiscall, PatrolPath_Get, 0x005ED3A0, void* pool, int32_t id);
GAME_FN(void, __thiscall, PathTrack_Aim, 0x00621B90, uint8_t* self, V3* out, uint8_t* ped, float scale);
GAME_FN(void, __thiscall, PathTrack_CheckPassed, 0x0062EC00, uint8_t* self);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(V3*, __cdecl, VectorScale, 0x00412350, V3* out, const V3* v, float s);
GAME_FN(V3*, __cdecl, Normalised, 0x00472D40, V3* out, const V3* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const V3* a, const V3* b);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const float* a, const float* b);
GAME_FN(float, __cdecl, DistanceSquared, 0x00472D00, const float* a, const float* b);
GAME_FN(void, __cdecl, ClampInPlace, 0x004A2A30, float* value, float low, float high);
GAME_FN(uint8_t, __cdecl, GetIsLineOfSightClear, 0x004617D0, const float* a, const float* b, int flags);

V3 PosOf(uint8_t* e) {
    const float* p = Pos(e);
    return {p[0], p[1], p[2]};
}
V3 NodePos(uint8_t* node) {
    return {At<int16_t>(node, 8) * 0.125f, At<int16_t>(node, 0xA) * 0.125f, At<int16_t>(node, 0xC) * 0.125f};
}
// the threat's (or its vehicle's) velocity
V3 ThreatVelocity(uint8_t* threat) {
    uint8_t* vehicle = At<uint8_t*>(threat, 0x1554);
    uint8_t* e = vehicle ? vehicle : threat;
    return {At<float>(e, 0x124), At<float>(e, 0x128), At<float>(e, 0x12C)};
}
float StatSpeed(uint8_t* ped, int32_t stat) {
    return float(Ped_GetStat(ped, stat)) / 100.0f;
}
void Clamp01(float& t) {
    if (t < 0.0f) {
        t = 0.0f;
    } else if (1.0f < t) {
        t = 1.0f;
    }
}
// from `p` toward `s` 3 units clear of the world
bool LeadIsClear(const V3& p, const V3& s) {
    V3 d, n, lead, end;
    VectorSub(&d, &s.x, &p.x);
    VectorScale(&lead, Normalised(&n, &d), 3.0f);
    return GetIsLineOfSightClear(&p.x, &VectorAdd(&end, &p.x, &lead.x)->x, 1) != 0;
}
// `dist` along the threat's right (side -1 its left) when 2 units that side of the ped are
// clear: to the right when the ped's right faces the threat and it's clear, else left
bool SidestepAcross(uint8_t* ped, const V3& p, const V3& toThreat, const V3& from, float rightX, float rightY,
                    float dist, V3& out) {
    const float* m = reinterpret_cast<const float*>(Entity_GetMatrix(ped));
    V3 pedRight = {m[0], m[1], m[2]};
    V3 r, s, end;
    if (DotProduct(&pedRight, &toThreat) > 0.0f) {
        VectorScale(&s, Entity_GetRight(ped, &r), 2.0f);
        if (GetIsLineOfSightClear(&p.x, &VectorSub(&end, &p.x, &s.x)->x, 1)) {
            out.x = from.x - rightX * dist * 0.3f;
            out.y = from.y - rightY * dist * 0.3f;
            return true;
        }
    }
    VectorScale(&s, Entity_GetRight(ped, &r), 2.0f);
    if (GetIsLineOfSightClear(&p.x, &VectorAdd(&end, &p.x, &s.x)->x, 1)) {
        out.x = rightX * dist * 0.3f + from.x;
        out.y = rightY * dist * 0.3f + from.y;
        return true;
    }
    return false;
}
// the threat 30 steps ahead, 0.75 off its side away from the ped
V3 CutAcross(uint8_t* ped, const V3& t, const V3& v, const V3& toThreat, float rightX, float rightY) {
    V3 w = {v.x * 30.0f, v.y * 30.0f, v.z * 30.0f};
    V3 s;
    VectorAdd(&s, &t.x, &w.x);
    const float* m = reinterpret_cast<const float*>(Entity_GetMatrix(ped));
    V3 pedRight = {m[0], m[1], m[2]};
    if (pedRight.y * toThreat.y + pedRight.x * toThreat.x + pedRight.z * toThreat.z > 0.0f) {
        s.x = s.x - rightX * 0.75f;
        s.y = s.y - rightY * 0.75f;
    } else {
        s.x = rightX * 0.75f + s.x;
        s.y = 0.75f * rightY + s.y;
    }
    return s;
}

// 0x621E60 (thiscall): FleeOnRoadOnBikeTrack::Start: the owner kept; the road node
// nearest facing from the threat to the ped
uint8_t __fastcall FleeOnRoadOnBikeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    At<uint8_t>(self, 0x1C) = 0;
    uint8_t* ped = OwnerPed(owner);
    V3 p = PosOf(ped);
    uint8_t* threat = Scanner_GetThreat(ped + 0x3BC);
    V3 d;
    VectorSub(&d, Pos(threat), &p.x);
    At<uint8_t*>(self, 0x20) = Paths_FindNodeFacing(roadPaths, p, 0, d.x, d.y);
    At<uint8_t*>(self, 0x24) = nullptr;
    return 0;
}

// 0x621F10 (thiscall): FleeOnRoadOnBikeTrack's ride: on to the next road node when
// near the one headed for (3x the reach seated); a low-skill ped with the threat
// closing within 3.5 (moving toward the node, the ped not ahead of it) swerves to its
// side (flags 0x18); a skilled one with it alongside within 5 cuts across it (flags 8,
// radius 1.2); else to the node at a speed by the distance (flags 0x1C). The steering's
// result.
uint8_t __fastcall FleeOnRoadOnBikeTrack_RideImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* threat = Scanner_GetThreat(ped + 0x3BC);
    V3 p = PosOf(ped);
    V3 t = PosOf(threat);
    const float* tm = reinterpret_cast<const float*>(Entity_GetMatrix(threat));
    float rightX = tm[0];
    float rightY = tm[1];
    float nearDist = float(Ped_GetStat(ped, 0x1B));
    float farDist = float(Ped_GetStat(ped, 0x1D));
    float awayDist = float(Ped_GetStat(ped, 0x1C));
    float speed18 = StatSpeed(ped, 0x18);
    float speed19 = StatSpeed(ped, 0x19);
    float speed1A = StatSpeed(ped, 0x1A);
    float speed23 = StatSpeed(ped, 0x23);
    float speed25 = StatSpeed(ped, 0x25);
    V3 away, toThreat;
    VectorSub(&away, &p.x, &t.x);
    VectorSub(&toThreat, &t.x, &p.x);
    float dist = SqrtAbs(away.x * away.x + away.y * away.y);
    V3 fwd;
    Placeable_GetForward(ped, &fwd);
    Vector_Normalise(&fwd);
    float facing = fwd.y * away.y + fwd.x * away.x + fwd.z * away.z;
    V3 n = NodePos(At<uint8_t*>(self, 0x20));
    V3 d;
    VectorSub(&d, &n.x, &p.x);
    float nodeDist = SqrtAbs(d.x * d.x + d.y * d.y);
    float reach = Ped_IsSeatedInVehicle(ped) ? 3.0f : 1.0f;
    if (At<float>(ped, 0x51C) * reach > nodeDist) {
        uint8_t* next = PathNode_PickNext(At<uint8_t*>(self, 0x20), p, At<uint8_t*>(self, 0x24));
        At<uint8_t*>(self, 0x24) = At<uint8_t*>(self, 0x20);
        At<uint8_t*>(self, 0x20) = next;
        n = NodePos(next);
    }
    V3 v = ThreatVelocity(threat);
    V3 toNode;
    VectorSub(&toNode, &n.x, &p.x);
    V3 dir = toNode;
    Vector_Normalise(&dir);
    uint8_t* steering = ped + 0x654;
    if (Ped_GetStat(ped, 0xE) < 0x33 && dist < 3.5f && v.x * v.x + v.y * v.y + v.z * v.z > 0.01f &&
        v.y * toNode.y + v.x * toNode.x + toNode.z * v.z > 0.0f &&
        v.y * toThreat.y + v.x * toThreat.x + v.z * toThreat.z < 0.0f) {
        V3 tn;
        if (DotProduct(&dir, Normalised(&tn, &toThreat)) > -0.8f &&
            DotProduct(&dir, Normalised(&tn, &toThreat)) < 0.8f) {
            float nd = Distance(&n.x, &p.x);
            V3 s = n;
            SidestepAcross(ped, p, toThreat, n, rightX, rightY, nd, s);
            if (LeadIsClear(p, s)) {
                n = s;
            }
            return Steering_GoTo(steering, n.x, n.y, n.z, 0x18, speed19, 0.3f, nullptr, 0);
        }
    }
    if (Ped_GetStat(ped, 0xE) > 0x4A) {
        V3 q;
        VectorSub(&q, &t.x, &p.x);
        if (q.x * q.x + q.y * q.y + q.z * q.z < 25.0f) {
            V3 vn = v;
            Vector_Normalise(&vn);
            if (dir.y * vn.y + vn.x * dir.x + dir.z * vn.z > 0.707f) {
                V3 tn = toThreat;
                Vector_Normalise(&tn);
                if (tn.y * dir.y + tn.x * dir.x + tn.z * dir.z > -0.1f) {
                    V3 s = CutAcross(ped, t, v, toThreat, rightX, rightY);
                    if (LeadIsClear(p, s)) {
                        n = s;
                    }
                    return Steering_GoTo(steering, n.x, n.y, n.z, 8, speed19, 1.2f, nullptr, 0);
                }
            }
        }
    }
    float speed;
    if (facing < 0.0f) {   // heading at the threat
        if (farDist < dist) {
            speed = speed1A;
        } else if (nearDist < dist && nearDist + 0.5f < farDist) {
            float f = (dist - nearDist) / (farDist - nearDist);
            ClampInPlace(&f, 0.0f, 1.0f);
            speed = speed18 - (speed18 - speed1A) * f;
        } else {
            float f = dist / (nearDist + 0.1f);
            Clamp01(f);
            speed = speed23 - f * (speed23 - speed18);
        }
    } else if (awayDist < dist) {
        float f = (dist - awayDist) / awayDist;
        Clamp01(f);
        speed = f * (speed25 - speed23) + speed23;
    } else {
        speed = speed23;
    }
    return Steering_GoTo(steering, n.x, n.y, n.z, 0x1C, speed, 0.3f, nullptr, 0);
}

// 0x62F140 (thiscall): FleeOnPathOnBikeTrack's ride (vf6): none without its path; past
// nodes caught up; as the road ride but aimed along the path (0x621B90, scaled by the
// speed): a low-skill ped swerves off the aim point beyond 6 (only onto a point near the
// path's line), a skilled one cuts across (flags 0xC000, radius 1.2, only onto a point
// near the line beyond 6); else flags 0xC010. The steering's result.
uint8_t __fastcall FleeOnPathOnBikeTrack_RideImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (!PatrolPath_Get(patrolPaths, At<int32_t>(self, 0x28))) {
        return 0;
    }
    uint8_t* threat = Scanner_GetThreat(ped + 0x3BC);
    V3 p = PosOf(ped);
    V3 t = PosOf(threat);
    const float* tm = reinterpret_cast<const float*>(Entity_GetMatrix(threat));
    float rightX = tm[0];
    float rightY = tm[1];
    float nearDist = float(Ped_GetStat(ped, 0x1B));
    float farDist = float(Ped_GetStat(ped, 0x1D));
    float awayDist = float(Ped_GetStat(ped, 0x1C));
    float speed18 = StatSpeed(ped, 0x18);
    float speed19 = StatSpeed(ped, 0x19);
    float speed1A = StatSpeed(ped, 0x1A);
    float speed23 = StatSpeed(ped, 0x23);
    float speed25 = StatSpeed(ped, 0x25);
    V3 toThreat;
    VectorSub(&toThreat, &t.x, &p.x);
    V3 fwd;
    Placeable_GetForward(ped, &fwd);
    Vector_Normalise(&fwd);
    float dist = SqrtAbs(toThreat.x * toThreat.x + toThreat.y * toThreat.y);
    float facing = fwd.y * toThreat.y + fwd.x * toThreat.x + fwd.z * toThreat.z;
    PathTrack_CheckPassed(self);
    V3 g = {0.0f, 0.0f, 0.0f};   // (not aimed yet: the checks below see the origin)
    V3 v = ThreatVelocity(threat);
    uint8_t* steering = ped + 0x654;
    if (Ped_GetStat(ped, 0xE) < 0x33 && dist < 3.5f && v.x * v.x + v.y * v.y + v.z * v.z > 0.01f) {
        V3 gp;
        VectorSub(&gp, &g.x, &p.x);
        if (gp.y * v.y + gp.x * v.x + gp.z * v.z > 0.0f && DotProduct(&v, &toThreat) < 0.0f) {
            V3 tn, dn;
            Normalised(&tn, &toThreat);
            VectorSub(&gp, &g.x, &p.x);
            if (DotProduct(Normalised(&dn, &gp), &tn) > -0.8f) {
                Normalised(&tn, &toThreat);
                VectorSub(&gp, &g.x, &p.x);
                if (DotProduct(Normalised(&dn, &gp), &tn) < 0.8f) {
                    PathTrack_Aim(self, &g, ped, speed23);
                    float gd = Distance(&g.x, &p.x);
                    V3 s = g;
                    SidestepAcross(ped, p, toThreat, g, rightX, rightY, gd, s);
                    if (gd > 6.0f) {
                        V3 sp, gp2;
                        VectorSub(&sp, &s.x, &p.x);
                        if (DotProduct(VectorSub(&gp2, &g.x, &p.x), &sp) > 0.92f && LeadIsClear(p, s)) {
                            g = s;
                        }
                    }
                    return Steering_GoTo(steering, g.x, g.y, g.z, 0xC010, speed23, 0.3f, nullptr, 0);
                }
            }
        }
    }
    if (Ped_GetStat(ped, 0xE) > 0x4A) {
        V3 q;
        VectorSub(&q, &t.x, &p.x);
        if (q.x * q.x + q.y * q.y + q.z * q.z < 25.0f) {
            V3 vn = v;
            Vector_Normalise(&vn);
            V3 dn;
            VectorSub(&dn, &g.x, &p.x);
            Vector_Normalise(&dn);
            if (dn.y * vn.y + vn.x * dn.x + dn.z * vn.z > 0.707f) {
                V3 tn, gp;
                const V3* a = Normalised(&tn, &toThreat);
                const V3* b = Normalised(&dn, VectorSub(&gp, &g.x, &p.x));
                if (b->y * a->y + b->x * a->x + b->z * a->z > -0.1f) {
                    PathTrack_Aim(self, &g, ped, 1.2f);
                    V3 s = CutAcross(ped, t, v, toThreat, rightX, rightY);
                    if (DistanceSquared(&g.x, &p.x) > 36.0f) {
                        V3 sp, gp2;
                        VectorSub(&sp, &s.x, &p.x);
                        if (DotProduct(VectorSub(&gp2, &g.x, &p.x), &sp) > 0.92f && LeadIsClear(p, s)) {
                            g = s;
                        }
                    }
                    return Steering_GoTo(steering, g.x, g.y, g.z, 0xC000, speed19, 1.2f, nullptr, 0);
                }
            }
        }
    }
    float speed;
    if (facing < 0.0f) {   // heading away from the threat
        if (farDist < dist) {
            speed = speed1A;
        } else if (nearDist < dist && nearDist + 0.5f < farDist) {
            float f = (dist - nearDist) / (farDist - nearDist);
            ClampInPlace(&f, 0.0f, 1.0f);
            speed = speed18 - (speed18 - speed1A) * f;
        } else {
            float f = dist / (nearDist + 0.1f);
            ClampInPlace(&f, 0.0f, 1.0f);
            speed = speed23 - (speed23 - speed18) * f;
        }
    } else if (awayDist < dist) {
        float f = (dist - awayDist) / awayDist;
        Clamp01(f);
        speed = f * (speed25 - speed23) + speed23;
    } else {
        speed = speed23;
    }
    PathTrack_Aim(self, &g, ped, speed);
    return Steering_GoTo(steering, g.x, g.y, g.z, 0xC010, speed, 0.3f, nullptr, 0);
}

}  // namespace

#define AIBIKETRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AIBIKETRK_REVERSED("FleeOnRoadOnBikeTrack::Start", 0x00621E60, &FleeOnRoadOnBikeTrack_Start);
AIBIKETRK_REVERSED("FleeOnRoadOnBikeTrack::Ride", 0x00621F10, &FleeOnRoadOnBikeTrack_RideImpl);
AIBIKETRK_REVERSED("FleeOnPathOnBikeTrack::Ride", 0x0062F140, &FleeOnPathOnBikeTrack_RideImpl);
