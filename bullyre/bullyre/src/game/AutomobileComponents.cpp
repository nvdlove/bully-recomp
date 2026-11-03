// CAutomobile (vtable 0x905D04): its suspension lines set up from the model's wheel
// positions (GTA's SetupSuspensionLines), a frame's rotation set (vf45) and a door
// opened on its frame (vf46, GTA's OpenDoor). The doors (CDoor, 0x24 bytes) are at
// +0x59C (+0x9 the axis, +0xC the angle); the wheels' suspension values at +0x7AC,
// +0x7D8, +0x7E8, +0x7F8.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Vehicle.h"
#include "reversed.h"

namespace {

using re::At;

struct Vec {
    float x, y, z;
};

// the game's CMatrix (0x44 bytes): right, forward, up, position (4 floats each), and
// the NiTransform it's attached to
struct Matrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(Matrix) == 0x44);

GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs
inline void* const TheSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const showDoorClosed = reinterpret_cast<void*>(0x00534BE0);
inline void* const hideDamagedChild = reinterpret_cast<void*>(0x00534BD0);

GAME_FN(void, __thiscall, VehicleModel_GetFramePosition, 0x00534CD0, void* modelInfo, int frame, Vec* out);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __thiscall, Matrix_FromTransform, 0x00412A70, Matrix* m, void* transform);
GAME_FN(void, __thiscall, Matrix_SetRotateX, 0x00412CA0, Matrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateY, 0x00412CC0, Matrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateZ, 0x00412CE0, Matrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotate, 0x00412D00, Matrix* m, const Vec* angles);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, Matrix* m);
GAME_FN(void, __cdecl, Node_ForEachChild, 0x006C9AA0, void* node, void* callback, uintptr_t arg);
GAME_FN(uint8_t, __thiscall, Door_IsClosed, 0x004C8160, void* door);
GAME_FN(void, __thiscall, Door_Open, 0x004C80B0, void* door, float ratio);
GAME_FN(float, __thiscall, Door_RetAngleWhenClosed, 0x004C8130, void* door);
GAME_FN(uint8_t, __thiscall, Damage_GetDoorStatus, 0x004C7E00, void* damage, int door);
GAME_FN(void, __thiscall, Damage_SetDoorStatus, 0x004C7DF0, void* damage, int door, int status);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const Vec* pos, int, const Vec* pos2, int, int, int,
        int, uintptr_t, uintptr_t, int, int, float, int);

const Vec* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(static_cast<uint8_t*>(entity) + 4);
}
float Length(const float* v) {
    return float(std::sqrt(std::fabs(float((double(v[0]) * v[0] + double(v[1]) * v[1]) + double(v[2]) * v[2]))));
}

} // namespace

namespace c09autocomponents {

// 0x4AAB00: the suspension lines (the col model's four lines) from the wheel
// positions (the right side the first's mirrored; bikes' spread by 1.04), the
// suspension's travel and the wheel size; the col model's box lowered to the lines and
// its radius grown to its corners'; bikes and class 6 then vf49 (their own col set-up)
void __fastcall SetupSuspensionLines(CAutomobile* self, re::Edx) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    uint8_t* mi = modelInfoPtrs[self->modelIndex];
    uint8_t* col = At<uint8_t*>(mi, 0xC);
    uint8_t* colData = At<uint8_t*>(col, 0x38);
    float travel = At<float>(self->handling, 0xB8);
    Vec first = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 4; i++) {
        Vec pos = {0.0f, 0.0f, 0.0f};
        VehicleModel_GetFramePosition(mi, i, &pos);
        int32_t vehicleClass;
        if (*Vehicle_GetVehicleClass(self, &vehicleClass) == 7) {
            pos.x = float(double(pos.x) * double(1.04f));
            pos.y = float(double(1.04f) * pos.y);
        }
        if (i == 0) {
            first = pos;
        } else {
            pos.x = i == 1 ? first.x : -first.x;
        }
        At<float>(p, 0x7AC + i * 4) = pos.z;
        pos.z = float(double(pos.z) + At<float>(self->handling, 0xB4));
        float* line = reinterpret_cast<float*>(At<uint8_t*>(colData, 4) + i * 0x20);
        line[0] = pos.x;
        line[1] = pos.y;
        line[2] = pos.z;
        float scale = At<float>(mi, 0x58);
        pos.z = float(((double(travel) - At<float>(self->handling, 0xB4)) - double(scale) * 0.5) + pos.z);
        line[4] = pos.x;
        line[5] = pos.y;
        line[6] = pos.z;
        At<float>(p, 0x7D8 + i * 4) = float(double(At<float>(self->handling, 0xB4)) - travel);
        float* lines = re::PtrCast<float*>(At<uint8_t*>(colData, 4));
        At<float>(p, 0x7E8 + i * 4) = float(double(lines[i * 8 + 2]) - lines[i * 8 + 6]);
    }
    float* lines = re::PtrCast<float*>(At<uint8_t*>(colData, 4));
    float scale = At<float>(mi, 0x58);
    float r = float((double(At<float>(p, 0x7D8)) - lines[2]) + double(scale) * 0.5);
    At<float>(p, 0x7F8) = r;
    At<float>(p, 0x7AC) = float(double(scale) * 0.5 - r);
    At<float>(p, 0x7B0) = float(double(At<float>(mi, 0x58)) * 0.5 - At<float>(p, 0x7F8));
    At<float>(p, 0x7B4) = float(double(At<float>(mi, 0x58)) * 0.5 - At<float>(p, 0x7F8));
    At<float>(p, 0x7B8) = float(0.5 * At<float>(mi, 0x58) - At<float>(p, 0x7F8));
    float boxMinZ = At<float>(col, 0x18);
    if (boxMinZ > lines[6]) {
        At<float>(col, 0x18) = lines[6];
    }
    float toMax = Length(&At<float>(col, 0x20));
    float toMin = Length(&At<float>(col, 0x10));
    float radius = toMax < toMin ? toMin : toMax;
    if (radius > At<float>(col, 0xC)) {
        At<float>(col, 0xC) = radius;
    }
    int32_t vehicleClass;
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) == 7 || *Vehicle_GetVehicleClass(self, &vehicleClass) == 6) {
        using Fn = void(__thiscall*)(void*, void*);
        re::PtrCast<Fn>(self->vtable[0xC4 / 4])(self, col);
    }
}

// 0x4AADF0 (vf45): a frame's rotation set (degrees about x, y, z), its position kept
// (the rotations add it back)
void __fastcall SetComponentRotation(CAutomobile* self, re::Edx, int frame, float rx, float ry, float rz) {
    Matrix m;
    void* transform = GetLocalTransform(self->frames[frame]);
    m.attached = transform;
    Matrix_FromTransform(&m, transform);
    float saved[3] = {m.m[12], m.m[13], m.m[14]};
    Matrix_SetRotateX(&m, float(double(rx) * double(3.1415927f) / 180.0));
    Matrix_SetRotateY(&m, float(double(ry) * double(3.1415927f) / 180.0));
    Matrix_SetRotateZ(&m, float(double(rz) * double(3.1415927f) / 180.0));
    m.m[12] = m.m[12] + saved[0];
    m.m[13] = m.m[13] + saved[1];
    m.m[14] = m.m[14] + saved[2];
    Matrix_UpdateRW(&m);
}

// 0x4AAED0 (vf46): a door opened to the ratio on its frame: the open / close sounds
// as it leaves or comes back to closed (a swinging door, status 2, then set shut), the
// frame turned about the door's axis
void __fastcall OpenDoor(CAutomobile* self, re::Edx, int frame, int door, float ratio) {
    void* node = self->frames[frame];
    if (!node) {
        return;
    }
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    Matrix m;
    void* transform = GetLocalTransform(node);
    m.attached = transform;
    Matrix_FromTransform(&m, transform);
    // (laid out as the game's: an axis past 2 writes into the saved position)
    float locals[6] = {0.0f, 0.0f, 0.0f, m.m[12], m.m[13], m.m[14]};
    uint8_t* doorBase = p + door * 0x24;
    void* d = doorBase + 0x59C;
    bool wasClosed = false;
    if (Door_IsClosed(d)) {
        Node_ForEachChild(self->frames[frame], showDoorClosed, 0x800);
        wasClosed = true;
    }
    Door_Open(d, ratio);
    const char* sound = nullptr;
    if (wasClosed) {
        double angle = At<float>(doorBase, 0x5A8);
        if (angle != double(Door_RetAngleWhenClosed(d))) {
            Node_ForEachChild(self->frames[frame], hideDamagedChild, 0x800);
            sound = "Car_DoorOpen";
        }
    } else if (ratio == 0.0f) {
        void* damage = p + 0x580;
        if (Damage_GetDoorStatus(damage, door) == 2) {
            Damage_SetDoorStatus(damage, door, 0);
        }
        sound = "Car_DoorClose";
    }
    if (sound) {
        const Vec* pos = EntityPos(self);
        Sounds_PlayAt(TheSounds, Sounds_GetId(TheSounds, sound), pos, 2, nullptr, 0, 0, 0, 0, 0, 0, 6, 0, 1.0f, 0);
    }
    locals[At<uint8_t>(doorBase, 0x5A5)] = At<float>(doorBase, 0x5A8);
    Vec angles = {locals[0], locals[1], locals[2]};
    Matrix_SetRotate(&m, &angles);
    m.m[12] = m.m[12] + locals[3];
    m.m[13] = m.m[13] + locals[4];
    m.m[14] = m.m[14] + locals[5];
    Matrix_UpdateRW(&m);
}

} // namespace c09autocomponents

// ---- registration ---------------------------------------------------------------------

#define AUTOCOMPONENTS_REVERSED(name, address, fn)                                           \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTOCOMPONENTS_REVERSED("CAutomobile::SetupSuspensionLines", 0x004AAB00, &c09autocomponents::SetupSuspensionLines);
AUTOCOMPONENTS_REVERSED("CAutomobile::SetComponentRotation", 0x004AADF0, &c09autocomponents::SetComponentRotation);
AUTOCOMPONENTS_REVERSED("CAutomobile::OpenDoor", 0x004AAED0, &c09autocomponents::OpenDoor);
