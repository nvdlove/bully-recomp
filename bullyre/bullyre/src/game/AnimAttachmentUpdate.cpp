// AM_ attachments: placing an attached model (the attachment: +0 the transform to set,
// +4 its frame, +8 the child's socket, +0xC the parent's socket, +0x14 the child model)
// at the parent model's socket, moving the child's whole skeleton along when its socket
// is a joint of it. 0x6B5C60..0x6B6303.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};
// Gamebryo's NiTransform: a 3x3 rotation, the translation, the scale
struct Xform {
    float rotation[9];
    CVector translation;  // +0x24
    float scale;          // +0x30
};
static_assert(sizeof(Xform) == 0x34);

inline void* const kAnimPose = reinterpret_cast<void*>(0x020C4910);
GAME_ARRAY(uint32_t, att_zeroVector, 0x00BCC564);

GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, Xform* t);
GAME_FN(Xform*, __thiscall, NiTransform_Multiply, 0x00406830, const Xform* self, Xform* out, const Xform* other);
GAME_FN(void, __cdecl, Transform_PreMultiply, 0x00415E00, Xform* out, const Xform* a, const Xform* b);
GAME_FN(void, __cdecl, Transform_Invert, 0x00522440, Xform* out, const Xform* in);
GAME_FN(void, __thiscall, Quat_ToTransform, 0x00413C50, const float* quat, Xform* out);
GAME_FN(Xform*, __cdecl, Node_GetWorldTransform, 0x006CA0A0, void* node);
GAME_FN(void, __cdecl, Frame_Update, 0x006C9AF0, void* frame);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Pose_UpdateModel, 0x006C1910, void* sampler, uint8_t* model, float dt, int, uintptr_t);
GAME_FN(void, __thiscall, Model_Fn6B93A0, 0x006B93A0, uint8_t* model, float dt, int);

// a socket's offset (vtable slot 1 the rotation, slot 0 the translation) as a transform
void SocketTransform(void* socket, Xform* out) {
    using GetFloats = const float*(__thiscall*)(void*);
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(socket);
    const float* rotation = re::PtrCast<GetFloats>(vtable[1])(socket);
    Quat_ToTransform(rotation, out);
    out->scale = 1.0f;
    const float* pos = re::PtrCast<GetFloats>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(socket))[0])(socket);
    out->translation = CVector{pos[0], pos[1], pos[2]};
}

inline Xform* Joint(uint8_t* model, uint32_t index) {
    return reinterpret_cast<Xform*>(At<uint8_t*>(At<uint8_t*>(model, 0x14), 8) + index * 0x34);
}

// 0x6B5C60: places attachment `a` (with the skeleton: at the parent's socket joint, and
// the child's own socket joint moved onto it)
void __fastcall UpdateAttachment(uint8_t* self, re::Edx, re::Ptr32<uint8_t>* a, float dt, int withSkeleton) {
    if (a[0] == nullptr) {
        return;
    }
    Xform parent;   // F-0x138
    Xform socket;   // F-0x104
    parent.scale = 1.0f;
    socket.scale = 1.0f;
    if (withSkeleton == 0 && At<void*>(self, 0x14) == nullptr) {
        NiTransform_MakeIdentity(&socket);
        Xform product;
        parent = *NiTransform_Multiply(Node_GetWorldTransform(At<void*>(self, 0xC)), &product, &socket);
    } else {
        // (the rotation copied out first)
        using GetFloats = const float*(__thiscall*)(void*);
        void* parentSocket = a[3];
        const float* q = re::PtrCast<GetFloats>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(parentSocket))[1])(parentSocket);
        float rotation[4] = {q[0], q[1], q[2], q[3]};
        Quat_ToTransform(rotation, &socket);
        socket.scale = 1.0f;
        const float* pos = re::PtrCast<GetFloats>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(parentSocket))[0])(parentSocket);
        socket.translation = CVector{pos[0], pos[1], pos[2]};
        Xform product;
        if (withSkeleton == 0 || At<void*>(self, 0x14) == nullptr) {
            parent = *NiTransform_Multiply(Node_GetWorldTransform(At<void*>(self, 0xC)), &product, &socket);
        } else {
            parent = *NiTransform_Multiply(Joint(self, At<uint32_t>(parentSocket, 8)), &product, &socket);
        }
    }
    uint8_t moved = 0;
    Xform* out = re::PtrCast<Xform*>(a[0]);
    if (a[2] == nullptr || withSkeleton == 0) {
        *out = parent;
    } else {
        uint8_t* child = a[5];
        SocketTransform(a[2], &socket);
        Xform inverse;  // F-0x68
        Xform local;    // F-0x9c
        inverse.scale = 1.0f;
        local.scale = 1.0f;
        if (At<void*>(child, 0x14) == nullptr ||
            (At<uint32_t>(a[2], 8) == 0 && !(~(At<uint32_t>(child, 0x98) >> 7) & 1))) {
            // the child as a whole
            Transform_PreMultiply(&local, &socket, &parent);
            CVector localPos = local.translation;
            Xform placed = parent;  // F-0xD0
            CVector parentPos = parent.translation;
            CVector a1, a2;
            VectorAdd(&a2, &parent.translation, VectorSub(&a1, &parentPos, &localPos));
            placed.translation = a2;
            placed.translation.x = placed.translation.x - parent.translation.x;
            placed.translation.y = placed.translation.y - parent.translation.y;
            placed.translation.z = placed.translation.z - parent.translation.z;
            Transform_Invert(&inverse, &local);
            Transform_PreMultiply(&socket, &inverse, &parent);
            socket.translation.x = reinterpret_cast<const float&>(att_zeroVector[0]);
            socket.translation.y = reinterpret_cast<const float&>(att_zeroVector[1]);
            socket.translation.z = reinterpret_cast<const float&>(att_zeroVector[2]);
            Transform_PreMultiply(out, &placed, &socket);
            out->translation.x = parent.translation.x + out->translation.x;
            out->translation.y = out->translation.y + parent.translation.y;
            out->translation.z = parent.translation.z + out->translation.z;
        } else {
            // the child's socket joint onto the parent's socket: every joint moved
            *out = parent;
            Frame_Update(a[1]);
            At<uint32_t>(child, 0x98) = (At<uint32_t>(child, 0x98) & ~2u) | 0x200;
            moved = 1;
            Pose_UpdateModel(kAnimPose, child, dt, withSkeleton, 0);
            At<uint32_t>(child, 0x98) = (At<uint32_t>(child, 0x98) & ~0x200u) | 2;
            Transform_PreMultiply(&local, &socket, Joint(child, At<uint32_t>(a[2], 8)));
            local.translation.x = local.translation.x - parent.translation.x;
            local.translation.y = local.translation.y - parent.translation.y;
            local.translation.z = local.translation.z - parent.translation.z;
            Transform_Invert(&inverse, &local);
            Transform_PreMultiply(&socket, &inverse, &parent);
            socket.translation.x = reinterpret_cast<const float&>(att_zeroVector[0]);
            socket.translation.y = reinterpret_cast<const float&>(att_zeroVector[1]);
            socket.translation.z = reinterpret_cast<const float&>(att_zeroVector[2]);
            Transform_PreMultiply(out, &socket, &local);
            CVector parentPos = parent.translation;
            out->translation.x = out->translation.x + parent.translation.x;
            out->translation.y = out->translation.y + parentPos.y;
            out->translation.z = out->translation.z + parentPos.z;
            parentPos = parent.translation;
            CVector localPos = local.translation;
            CVector shift, back;  // F-0x148, F-0x164
            VectorAdd(&shift, &localPos, &parentPos);
            VectorSub(&back, &parent.translation, &shift);
            back.x = shift.x + back.x;
            back.y = back.y + shift.y;
            back.z = back.z + shift.z;
            uint32_t count = At<uint32_t>(At<uint8_t*>(child, 0x14), 4);
            for (uint32_t i = 0; i < count; i++) {
                Xform* joint = Joint(child, i);
                joint->translation.x = joint->translation.x - shift.x;
                joint->translation.y = joint->translation.y - shift.y;
                joint->translation.z = joint->translation.z - shift.z;
                Xform product;
                Xform turned = *NiTransform_Multiply(&socket, &product, joint);
                turned.translation.x = turned.translation.x + back.x;
                turned.translation.y = back.y + turned.translation.y;
                turned.translation.z = turned.translation.z + back.z;
                *joint = turned;
            }
            At<int32_t>(child, 0x90) = -1;
            At<int32_t>(child, 0x8C) = -1;
        }
    }
    if (a[1] != nullptr) {
        Frame_Update(a[1]);
        if (moved) {
            uint8_t* child = a[5];
            using Callback = void(__cdecl*)(uint8_t*, void*);
            Callback callback = At<Callback>(child, 4);
            if (callback) {
                callback(child, At<void*>(child, 8));
            }
            Model_Fn6B93A0(child, dt, 1);
        }
    }
}

} // namespace

#define AMATTACH_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMATTACH_REVERSED("AM_Model::UpdateAttachment", 0x006B5C60, &UpdateAttachment);
