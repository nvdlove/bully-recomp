// The scripted conversation (the object 0xB9DA60's members, 0x683EB0..0x6879E0): its
// participants (up to 8 peds), started from an act file and node (the participants
// placed around the first one kept by the char triggers, facing their centre, idling,
// the conversation camera on the first two) and ended (their actions, stats and camera
// back).
//
// The object: +0 the act file's name, +0x64 active, +0x68 the peds [8], +0x88 / +0x8C their
// count, +0x90 the centre, +0x9C their saved +0x1570 state [8], +0xA4 place them / ending,
// +0xA5 each has a vehicle-ish partner (+0x1554) [8], +0xB0 its saved node [8], +0xD0 the
// camera mode before. Its settings: *0x20C88DC (+0x50 / +0x54 the closest / farthest a
// participant stands, +0x58 the area cleared).

#include <cmath>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct V3 {
    float x, y, z;
};
static_assert(sizeof(V3) == 0xC);

// The conversation camera's shot (ConvCamParams)
struct ConvShot2 {
    re::Ptr32<uint8_t> ped1;
    re::Ptr32<uint8_t> ped2;
    int32_t shot;
    uint8_t flags[4];
    int32_t name;
    int32_t list;
};
static_assert(sizeof(ConvShot2) == 0x18);

GAME_VAR(uint8_t*, cvSettings, 0x020C88DC);
GAME_VAR(int32_t, cvGameMode, 0x00C3CCF8);
GAME_VAR(float, cvWidescreenAmount, 0x00C3CD14);
GAME_VAR(int32_t, cvWidescreenSetting, 0x00C3CD0C);
GAME_VAR(uint8_t*, cvPunishment, 0x00C0F78C);
GAME_VAR(uint8_t*, cvConvCam, 0x00C3CFC0);
GAME_VAR(void*, cvPopulation, 0x00C2C108);
GAME_VAR(float, cvIdleBlendRate, 0x00B9DAAC);
GAME_VAR(float, cvTimeStepNonClipped, 0x00C1A9A0);
inline void* const cvTheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const cvText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const cvCharTriggers = reinterpret_cast<void*>(0x00BCD098);
inline void* const cvPaths = reinterpret_cast<void*>(0x00BF6228);
inline void* const cvAnimManager = reinterpret_cast<void*>(0x020C4910);
inline const char* const cvShotListName = reinterpret_cast<const char*>(0x009143B8);   // "Conv"
inline const char* const cvIdleAnim = reinterpret_cast<const char*>(0x0093EBE4);       // "C_PLAYER\IDLE"

GAME_FN(void, __thiscall, Camera_SetShot, 0x004F2C20, void* camera, int shot, int a, int instant, int b, int c);
GAME_FN(void, __thiscall, Camera_SetDefaultFov, 0x004F1660, void* camera);
GAME_FN(uint8_t, __cdecl, Nis_IsSet, 0x005BB380);
GAME_FN(void, __thiscall, Ped_Fn68EBB0, 0x0068EBB0, uint8_t* ped, int a);
GAME_FN(void, __thiscall, Ctl_Stop, 0x005F40A0, uint8_t* controller);
GAME_FN(void, __thiscall, Ctl_Play, 0x005F4A50, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_Update, 0x005F4B00, uint8_t* controller, float dt);
GAME_FN(void, __thiscall, Ped_Set1570, 0x0047B7E0, uint8_t* ped, int state, int a, int b);
GAME_FN(void, __thiscall, Ped_SetGrapplePartner, 0x004744B0, uint8_t* ped, uint8_t* partner);
GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(uint8_t, __thiscall, Ped_PlayAction, 0x004740F0, uint8_t* ped, const char* node, const char* file);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x004734C0, uint8_t* ped, float heading);
GAME_FN(void, __thiscall, Ped_UpdateMoveDirection, 0x00474570, uint8_t* ped);
GAME_FN(void, __thiscall, Paths_ClearArea_x64, 0x004423C0, void* paths, float, float, float, float radius);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Paths_ClearArea(void* a0, V3 a1, float a2) {
    return Paths_ClearArea_x64(a0, a1.x, a1.y, a1.z, a2);
}
GAME_FN(void, __thiscall, Population_ClearArea, 0x0049F020, void* population, V3 centre, float radius);
GAME_FN(void, __thiscall, Text_LoadBlock, 0x0068FD50, void* text, const char* name);
GAME_FN(void, __thiscall, Text_Fn68F520, 0x0068F520, void* text);
inline char* __cdecl Strrchr(const char* s, int c) { return crt::Strrchr(s, c); }
inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(uint8_t, __thiscall, CharTriggers_Contains, 0x004245E0, void* triggers, uint8_t* ped);
GAME_FN(V3*, __thiscall, Placeable_GetPosition, 0x00415460, uint8_t* placeable);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const void* a, const void* b);
GAME_FN(V3*, __cdecl, VectorSub, 0x00414B70, V3* out, const void* a, const void* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, V3* v);
GAME_FN(float, __cdecl, FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int flags);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, uint8_t* m);
GAME_FN(void, __thiscall, SimpleTransform_UpdateNi, 0x0046D8F0, void* transform, void* out);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00465760, uint8_t* entity);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float angle);
GAME_FN(float, __cdecl, AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(void, __thiscall, ConvCamParams_Construct, 0x004F4DE0, ConvShot2* params);
GAME_FN(uint8_t, __thiscall, ConvCamCtrl_Set, 0x004F6D40, uint8_t* ctrl, ConvShot2* params, int a, int b, int c);
GAME_FN(int32_t, __cdecl, ConvCam_FindShotList, 0x004F56B0, const char* name);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, void* blend, uint32_t name);
GAME_FN(int32_t, __thiscall, AnimDict_Find, 0x006B1D60, void* dict, int, const char* name, int32_t* index);
GAME_FN(uint8_t*, __thiscall, Anim_GetSlot, 0x006B9380, void* clump, int type);
GAME_FN(void, __thiscall, AnimManager_Blend, 0x006C0D40, void* manager, void* clump, uintptr_t anim, void* slot, int a, float b, float c);
GAME_FN(void, __thiscall, AnimManager_SetSpeed, 0x006C0E60, void* manager, void* clump, void* slot, int a, float speed);
GAME_FN(void, __thiscall, AnimManager_SetTime, 0x006C0E90, void* manager, void* clump, void* slot, int a, float time, int b, int c);
GAME_FN(void, __thiscall, AnimManager_Update, 0x006C1910, void* manager, void* clump, float dt, int a, uintptr_t b);
GAME_FN(void, __thiscall, AnimManager_Fn6C0A00, 0x006C0A00, void* manager);
inline int __cdecl CrtRand() { return crt::Rand(); }
// ours, through the game's addresses
GAME_FN(uint8_t, __thiscall, Conversation_Start, 0x00685C50, uint8_t* self, const char* file, const char* node);

inline re::Ptr32<uint8_t>& Participant(uint8_t* self, int i) {
    return At<uint8_t*>(self, 0x68 + i * 4);
}
inline int32_t& Count(uint8_t* self) {
    return At<int32_t>(self, 0x8C);
}
inline float* PosOf(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m ? reinterpret_cast<float*>(m + 0x30) : reinterpret_cast<float*>(e + 4);
}
inline float& F(uint8_t* p, int offset) {
    return At<float>(p, offset);
}

// 0x683EB0 (thiscall): the "place them" / ending flag (+0xA4)
void __fastcall Conversation_SetPlace(uint8_t* self, re::Edx, int flag) {
    self[0xA4] = uint8_t(flag);
}

// 0x683EC0 (thiscall): no participants
void __fastcall Conversation_Clear(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x88) = 0;
    Count(self) = 0;
}

// 0x683ED0 (thiscall): a participant added (up to 8)
void __fastcall Conversation_Add(uint8_t* self, re::Edx, uint8_t* ped) {
    if (Count(self) < 8) {
        Participant(self, Count(self)) = ped;
        Count(self)++;
    }
}

// 0x683EF0 (thiscall): a participant's saved node dropped
void __fastcall Conversation_Forget(uint8_t* self, re::Edx, uint8_t* ped) {
    for (int i = 0; i < Count(self); i++) {
        if (self[0xA5 + i] && Participant(self, i) == ped) {
            self[0xA5 + i] = 0;
            At<uint8_t*>(self, 0xB0 + i * 4) = nullptr;
        }
    }
}

// 0x683F40 (thiscall): the ped takes part in the active conversation
uint8_t __fastcall Conversation_IsInOne(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!self[0x64]) {
        return 0;
    }
    for (int i = 0; i < Count(self); i++) {
        if (Participant(self, i) == ped) {
            return 1;
        }
    }
    return 0;
}

// an entity's frame synced to its new matrix (the renderer's node, or the attached one)
inline void SyncFrame(uint8_t* e) {
    uint8_t* object = At<uint8_t*>(e, 0x18);
    if (object && At<void*>(object, 4)) {
        float* node = GetLocalTransform(At<void*>(object, 4));
        if (node) {
            if (At<uint8_t*>(e, 0x14)) {
                if (At<void*>(Entity_GetMatrix(e), 0x40) == nullptr) {
                    void* attach = nullptr;
                    uint8_t* o = At<uint8_t*>(e, 0x18);
                    if (o && At<void*>(o, 4)) {
                        attach = GetLocalTransform(At<void*>(o, 4));
                    }
                    uint8_t* m = Entity_GetMatrix(e);
                    At<void*>(m, 0x40) = attach;
                    Matrix_UpdateRW(m);
                } else {
                    Matrix_UpdateRW(Entity_GetMatrix(e));
                }
            } else {
                SimpleTransform_UpdateNi(e + 4, node);
            }
        }
    }
    Entity_UpdateRwFrame(e);
}

// the participant's (or its partner's) heading and position written back (the matrix's
// fourth word from the game's uninitialised stack, here 0)
inline void PlaceAt(uint8_t* e, float heading, const V3& pos) {
    if (At<uint8_t*>(e, 0x14)) {
        Matrix_SetRotateZOnly(At<uint8_t*>(e, 0x14), heading);
    }
    F(e, 0x10) = heading;
    uint8_t* m = At<uint8_t*>(e, 0x14);
    if (m) {
        F(m, 0x30) = pos.x;
        F(m, 0x34) = pos.y;
        F(m, 0x38) = pos.z;
        At<int32_t>(m, 0x3C) = 0;
    }
    F(e, 4) = pos.x;
    F(e, 8) = pos.y;
    F(e, 0xC) = pos.z;
}

// 0x685C50 (thiscall, ret 8): the conversation started from an act file (its name kept
// at +0: the text block of its base name loaded) and node: the camera mode saved, the
// punishments off, each participant's partner (+0x1554) frozen; with +0xA4 the others
// placed within the settings' distance of the one the char triggers keep (else the first)
// on the ground, all facing the centre; their states saved, the area cleared, each other's
// targets, the conversation camera, the first one's action played, idles started
uint8_t __fastcall Conversation_StartImpl(uint8_t* self, re::Edx, const char* file, const char* node) {
    if (self[0x64] || !file) {
        return 0;
    }
    char* name = reinterpret_cast<char*>(self);
    for (int i = 0;; i++) {
        name[i] = file[i];
        if (!file[i]) {
            break;
        }
    }
    const char* base;
    const char* slash = Strrchr(file, '\\');
    if (!slash) {
        slash = Strrchr(file, '/');
        base = slash ? slash + 1 : file;
    } else {
        const char* forward = Strrchr(slash, '/');
        base = forward ? forward + 1 : slash + 1;
    }
    size_t length = 0;
    while (base[length]) {
        length++;
    }
    char block[260];
    Strncpy(block, base, length - 4);
    block[length - 4] = 0;
    Text_LoadBlock(cvText, block);
    At<int32_t>(self, 0xD0) = cvGameMode;
    if (cvGameMode == 2) {
        Camera_SetShot(cvTheCamera, 1, 0, 1, 0, 0);
        At<int32_t>(self, 0xD0) = cvGameMode;
    }
    cvPunishment[0] = 1;
    cvPunishment[1] = 1;
    self[0x64] = 1;
    vc::Call<uint8_t*>(Participant(self, 0), 0x40)[0x1D] = 1;
    At<int32_t>(self, 0x88) = 0;
    for (int i = 0; i < Count(self); i++) {
        uint8_t has = At<uint8_t*>(Participant(self, i), 0x1554) != nullptr;
        self[0xA5 + i] = has;
        if (!has) {
            At<uint8_t*>(self, 0xB0 + i * 4) = nullptr;
            continue;
        }
        uint8_t* ped = Participant(self, i);
        At<uint8_t*>(self, 0xB0 + i * 4) = At<uint8_t*>(At<uint8_t*>(ped, 0x2E0), 0x268);
        uint8_t* partner = At<uint8_t*>(ped, 0x1554);
        if (partner) {
            uint8_t flags = partner[0x1BC];
            partner[0x6B8] |= 2;
            partner[0x2E4] = 0;
            partner[0x1BC] = uint8_t((flags & 0xFD) | 0xC);
            uint8_t* anim = Blend_GetAnim(At<void*>(partner, 0x114), 0);
            AnimManager_Blend(cvAnimManager, At<void*>(partner, 0x114), 0, anim, 0, 1.0f, -1.0f);
            AnimManager_SetSpeed(cvAnimManager, At<void*>(partner, 0x114), anim, 0, 1.0f);
        }
    }
    if (self[0xA4]) {
        uint8_t* anchor = Participant(self, 0);
        for (int i = 0; i < Count(self); i++) {
            if (CharTriggers_Contains(cvCharTriggers, Participant(self, i))) {
                anchor = Participant(self, i);
                break;
            }
        }
        for (int i = 0; i < Count(self); i++) {
            uint8_t* ped = Participant(self, i);
            if (ped == anchor) {
                continue;
            }
            // (the target stays at the origin for a participant already within range)
            V3 target{0.0f, 0.0f, 0.0f};
            float* pos = PosOf(ped);
            V3 d;
            VectorSub(&d, pos, Placeable_GetPosition(anchor));
            d.z = 0.0f;
            float zero = 0.0f;
            float distance = std::sqrt(std::fabs((d.x * d.x + d.y * d.y) + zero * zero));
            int limit = 0;   // (too close: +0x50, too far: +0x54)
            if (F(cvSettings, 0x50) > distance) {
                limit = 0x50;
            } else if (F(cvSettings, 0x54) < distance) {
                limit = 0x54;
            }
            if (limit) {
                Vector_Normalise(&d);
                float k = F(cvSettings, limit);
                V3 offset{k * d.x, d.y * k, k * d.z};
                V3 sum;
                target = *VectorAdd(&sum, Placeable_GetPosition(anchor), &offset);
            }
            uint8_t found;
            float z = target.z + 1.5f;
            float ground = FindGroundZFor3DCoord(target.x, target.y, z, &found, 1);
            if (found) {
                target.z = ground + 0.30000001192092896f;
            }
            uint8_t* partner = At<uint8_t*>(ped, 0x1554);
            uint8_t* moved = partner ? partner : ped;
            vc::Call<void>(moved, 0x38, target.x, target.y, target.z);   // (Teleport takes the floats)
            SyncFrame(moved);
        }
    }
    int32_t count = Count(self);
    V3 sum{0.0f, 0.0f, 0.0f};
    for (int i = 0; i < count; i++) {
        float* pos = PosOf(Participant(self, i));
        sum.x = pos[0] + sum.x;
        sum.y = pos[1] + sum.y;
        sum.z = pos[2] + sum.z;
    }
    float inverse = 1.0f / float(count);
    V3 centre{inverse * sum.x, inverse * sum.y, inverse * sum.z};
    if (self[0xA4]) {
        for (int i = 0; i < Count(self); i++) {
            uint8_t* ped = Participant(self, i);
            float* p = PosOf(ped);
            V3 pos{p[0], p[1], p[2]};
            float angle = AngleBetweenPoints(centre.x, centre.y, pos.x, pos.y);
            float heading = LimitAngle(angle);
            Ped_SetHeading(ped, heading);
            Ped_UpdateMoveDirection(Participant(self, i));
            uint8_t* partner = At<uint8_t*>(Participant(self, i), 0x1554);
            PlaceAt(partner ? static_cast<uint8_t*>(partner) : static_cast<uint8_t*>(Participant(self, i)), heading, pos);
        }
    }
    for (int i = 0; i < Count(self); i++) {
        if (At<int32_t>(Participant(self, i), 0x1310) != 13) {
            Ped_Fn68EBB0(Participant(self, i), 1);
        }
    }
    for (int i = 0; i < Count(self); i++) {
        self[0x9C + i] = Participant(self, i)[0x1570];
        Ped_Set1570(Participant(self, i), 2, 1, 1);
    }
    Population_ClearArea(cvPopulation, centre, F(cvSettings, 0x58));
    Paths_ClearArea(cvPaths, centre, F(cvSettings, 0x58));
    At<V3>(self, 0x90) = centre;
    Ped_SetTargetEntity(Participant(self, 0), Participant(self, 1));
    Ped_SetGrapplePartner(Participant(self, 0), Participant(self, 1));
    for (int i = 1; i < Count(self); i++) {
        Ped_SetTargetEntity(Participant(self, i), Participant(self, 0));
        Ped_SetGrapplePartner(Participant(self, i), Participant(self, 0));
    }
    ConvShot2 shot;
    ConvCamParams_Construct(&shot);
    shot.ped1 = Participant(self, 0);
    shot.ped2 = Participant(self, 1);
    shot.list = ConvCam_FindShotList(cvShotListName);
    ConvCamCtrl_Set(cvConvCam, &shot, 0, 1, 0);
    uint8_t played = Ped_PlayAction(Participant(self, 0), node, name);
    uint8_t* controller = At<uint8_t*>(Participant(self, 0), 0x2E0) + 0x258;
    if (controller[0] == 0) {
        Ctl_Update(controller, 9.999999747378752e-05f);
    }
    for (int i = 0; i < Count(self); i++) {
        uint8_t* ped = Participant(self, i);
        void* clump = At<void*>(ped, 0x114);
        if (At<void*>(At<uint8_t*>(clump, 0x94), 0x10)) {
            int32_t anim = -1;
            if (AnimDict_Find(At<void*>(At<uint8_t*>(clump, 0x94), 0x10), 0, cvIdleAnim, &anim) && anim != -1) {
                uint8_t* slot = Anim_GetSlot(At<void*>(Participant(self, i), 0x114), 0);
                AnimManager_Blend(cvAnimManager, At<void*>(Participant(self, i), 0x114), anim, slot, 3, 1.0f, -1.0f);
                float r = float(CrtRand()) / 32767.0f;
                float time = r * (0.30000001192092896f - 0.0f) + 0.0f;
                AnimManager_SetTime(cvAnimManager, At<void*>(Participant(self, i), 0x114), slot, 0, time, 0, 0);
                float r2 = float(CrtRand()) / 32767.0f;
                float speed = r2 * 0.20000004768371582f + 0.8999999761581421f;
                AnimManager_SetSpeed(cvAnimManager, At<void*>(Participant(self, i), 0x114), slot, 0, speed);
            }
        }
        float dt = cvIdleBlendRate * cvTimeStepNonClipped;
        AnimManager_Update(cvAnimManager, At<void*>(Participant(self, i), 0x114), dt, 1, 0);
    }
    AnimManager_Fn6C0A00(cvAnimManager);
    return played;
}

// 0x684CF0 (thiscall): the conversation ended (+0xA4 set): the participants' facial
// animation stopped and (the first one's, once each) actions stopped; the camera mode
// back; the partners' saved nodes played again and unfrozen; states, grapple and targets
// cleared; the area's paths back; the punishments on; the default fov; the text block
void __fastcall Conversation_End(uint8_t* self, re::Edx) {
    self[0xA4] = 1;
    if (!self[0x64]) {
        return;
    }
    for (int i = 0; i < Count(self); i++) {
        uint8_t* ped = Participant(self, i);
        if (At<int32_t>(ped, 0x1310) != 13) {
            Ped_Fn68EBB0(ped, 0);
        }
        Ctl_Stop(At<uint8_t*>(Participant(self, 0), 0x2E0) + 0x258);   // (always the first one's)
    }
    Camera_SetShot(cvTheCamera, At<int32_t>(self, 0xD0), 0, 1, 0, 0);
    if (!Nis_IsSet()) {
        cvWidescreenSetting = 0;
        cvWidescreenAmount = 0.0f;
    }
    vc::Call<uint8_t*>(Participant(self, 0), 0x40)[0x1D] = 0;
    for (int i = 0; i < Count(self); i++) {
        if (self[0xA5 + i] && At<uint8_t*>(Participant(self, i), 0x1554)) {
            uint8_t* ped = Participant(self, i);
            Ctl_Play(At<uint8_t*>(ped, 0x2E0) + 0x258, At<uint8_t*>(self, 0xB0 + i * 4));
            uint8_t* partner = At<uint8_t*>(Participant(self, i), 0x1554);
            uint8_t flags = partner[0x1BC];
            partner[0x6B8] &= 0xFD;
            partner[0x1BC] = uint8_t((flags & 0xF3) | 2);
        }
        self[0xA5 + i] = 0;
        At<uint8_t*>(self, 0xB0 + i * 4) = nullptr;
    }
    for (int i = 0; i < Count(self); i++) {
        Ped_Set1570(Participant(self, i), self[0x9C + i], 1, 1);
        Ped_SetGrapplePartner(Participant(self, i), nullptr);
        Ped_SetTargetEntity(Participant(self, i), nullptr);
    }
    Paths_ClearArea(cvPaths, At<V3>(self, 0x90), F(cvSettings, 0x58));
    cvPunishment[0] = 0;
    cvPunishment[1] = 0;
    Camera_SetDefaultFov(cvTheCamera);
    Text_Fn68F520(cvText);
    self[0x64] = 0;
}

// 0x6879E0 (thiscall, ret 0x10): a two-person conversation started
uint8_t __fastcall Conversation_StartTwo(uint8_t* self, re::Edx, uint8_t* a, uint8_t* b, const char* file, const char* node) {
    Count(self) = 0;
    At<int32_t>(self, 0x88) = 0;
    Participant(self, 0) = a;
    Count(self) += 1;
    if (Count(self) < 8) {
        Participant(self, Count(self)) = b;
        Count(self) += 1;
    }
    return Conversation_Start(self, file, node);
}

#define CONV_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CONV_REVERSED("Conversation::SetPlace", 0x00683EB0, &Conversation_SetPlace);
CONV_REVERSED("Conversation::Clear", 0x00683EC0, &Conversation_Clear);
CONV_REVERSED("Conversation::Add", 0x00683ED0, &Conversation_Add);
CONV_REVERSED("Conversation::Forget", 0x00683EF0, &Conversation_Forget);
CONV_REVERSED("Conversation::IsInOne", 0x00683F40, &Conversation_IsInOne);
CONV_REVERSED("Conversation::End", 0x00684CF0, &Conversation_End);
CONV_REVERSED("Conversation::Start", 0x00685C50, &Conversation_StartImpl);
CONV_REVERSED("Conversation::StartTwo", 0x006879E0, &Conversation_StartTwo);

} // namespace
