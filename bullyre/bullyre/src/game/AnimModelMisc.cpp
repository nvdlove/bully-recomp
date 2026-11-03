// AM_ model helpers: its teardown (the pose sampler told, the embedded lists' vtables
// reset), emptying one of its node lists, stepping its faders, and two rotation helpers
// (a point turned about an axis; the rotation from one direction to another).
// 0x6B5A80..0x6B5C56.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

constexpr float kPi = 3.14159274f;

inline void* const kAnimPose = reinterpret_cast<void*>(0x020C4910);
inline void* const kBaseListVtable = reinterpret_cast<void*>(0x0092D0AC);

GAME_FN(void, __thiscall, Pose_ReleaseModel, 0x006C2C00, void* sampler, uint8_t* model, int);
GAME_FN(void, __thiscall, Fader_Step, 0x006B1FC0, uint8_t* fader, float dt);
GAME_FN(void, __cdecl, Transform_SetRotation, 0x00415EB0, void* matrix, const CVector* axis, float degrees, int mode);
GAME_FN(float*, __thiscall, NiMatrix3_MultiplyVector, 0x00768A70, const void* self, CVector* out, const CVector* v);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);

// 0x6B5A80: the model's teardown (the lists at +0xAC, +0xD8 back to RV::BaseList)
void __fastcall Model_Teardown(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x94) != nullptr) {
        Pose_ReleaseModel(kAnimPose, self, 0);
    }
    At<void*>(self, 0xD8) = kBaseListVtable;
    At<void*>(self, 0xAC) = kBaseListVtable;
}

// 0x6B5AB0: empties node list `list` (+0x34 + list * 0xC: head, tail, count), each node's
// owner (4 bytes before the link) destroyed through its vtable slot 1
void __fastcall Model_EmptyList(uint8_t* self, re::Edx, int list) {
    re::Ptr32<uint8_t>* head = reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x34 + list * 0xC);
    while (*head) {
        uint8_t* node = *head;
        uint8_t* next = At<uint8_t*>(node, 0);
        *head = next;
        if (next == nullptr) {
            head[1] = nullptr;
        }
        At<uint8_t*>(node, 0) = nullptr;
        reinterpret_cast<int32_t*>(head)[2] -= 1;
        void* owner = node - 4;
        using Destroy = void(__thiscall*)(void*);
        re::PtrCast<Destroy>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(owner))[1])(owner);
    }
}

// 0x6B5AF0: steps every fader of the list at +0xDC (count at +0xF0) by dt
void __fastcall Model_StepFaders(uint8_t* self, re::Edx, float dt) {
    uint8_t* fader = At<int32_t>(self, 0xF0) != 0 ? *At<re::Ptr32<uint8_t>*>(self, 0xDC) : nullptr;
    while (fader) {
        Fader_Step(fader, dt);
        uint8_t* link = At<uint8_t*>(fader, 0x34);
        if (link == nullptr) {
            return;
        }
        re::Ptr32<uint8_t>* next = At<re::Ptr32<uint8_t>*>(link, 8);
        if (next == nullptr) {
            return;
        }
        fader = *next;
    }
}

// 0x6B5B40: `point` turned about `axis` by `radians`
CVector* __cdecl RotateAboutAxis(CVector* out, const CVector* point, const CVector* axis, float radians) {
    float matrix[13];
    matrix[12] = 1.0f;
    float degrees = radians * 180.0f / kPi;
    Transform_SetRotation(matrix, axis, degrees, 0);
    CVector turned;
    const float* r = NiMatrix3_MultiplyVector(matrix, &turned, point);
    out->x = r[0];
    out->y = r[1];
    out->z = r[2];
    return out;
}

// 0x6B5BB0: `out` set to the rotation turning direction a onto b (unchanged when they're
// the same)
void __cdecl RotationBetween(void* out, const CVector* a, const CVector* b, int mode) {
    float dot = (a->x * b->x + a->y * b->y) + a->z * b->z;
    if (!(dot < 1.0f)) {
        return;
    }
    CVector axis;
    axis.x = b->z * a->y - a->z * b->y;
    axis.y = a->z * b->x - a->x * b->z;
    axis.z = a->x * b->y - b->x * a->y;
    float degrees = SafeAcos(dot) * 180.0f / kPi;
    Transform_SetRotation(out, &axis, degrees, mode);
}

} // namespace

#define AMMODEL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMMODEL_REVERSED("AM_Model::Teardown", 0x006B5A80, &Model_Teardown);
AMMODEL_REVERSED("AM_Model::EmptyList", 0x006B5AB0, &Model_EmptyList);
AMMODEL_REVERSED("AM_Model::StepFaders", 0x006B5AF0, &Model_StepFaders);
AMMODEL_REVERSED("AM::RotateAboutAxis", 0x006B5B40, &RotateAboutAxis);
AMMODEL_REVERSED("AM::RotationBetween", 0x006B5BB0, &RotationBetween);
