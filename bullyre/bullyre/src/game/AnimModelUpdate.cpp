// The AM_ manager (0x20C4910) updating a model (its anim object): the seven track slots
// (+0x18) each with a list of sub-tracks (+0x34 + slot * 0xC) stepped, finished ones
// dropped, the tracks sampled and blended into the model's pose (sub-tracks first, then the
// track, the root motion weighted by the tracks' masks), the pose made into the skeleton's
// transforms and the model's update callback run; one joint sampled from an anim; a bone's
// transform, position and axis. 0x6BE3B0, 0x6BE760, 0x6BE890, 0x6BF570, 0x6C0710,
// 0x6C0970, 0x6C1190, 0x6C11E0, 0x6C1840, 0x6C1910.
//
// A model: +4 its update callback (model, +8 its data), +0xC its object (+0x38, +0x6C
// vectors), +0x14 the skeleton (+8 the joints' transforms, 0x34 each; +0x1C its pose),
// +0x18 the tracks, +0x34 the sub-track lists, +0x94 the hierarchy object (+0x10 the HXD),
// +0x98 flags (1 blend, 2 frozen, 4 no root motion, 0x80 blended, 0x100 done, 0x200 no
// callback, 0x400 updated, 0x1000 in sub-tracks), +0xA4 / +0xA8 where the root motion goes.
// A track (AM_Track): vf 0xC step(dt) -> still going, 0x10 apply(model, pose, translation,
// rotation), 0x18 active; +8 its mask (a weight set: +0x20 weights by anim kind, +0x24
// their count), +0xC its weight.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

// ActionController.cpp's TrackList::Remove iterator
struct LinkIter {
    re::Ptr32<uint8_t> node;
    re::Ptr32<uint8_t> prev;
};

GAME_ARRAY(uint32_t, amupd_zeroVector, 0x00BCC564);
GAME_VAR(uint8_t*, amupd_samplingAnim, 0x020C4904);
GAME_VAR(uint32_t, amupd_samplingSocketCount, 0x020C4900);
GAME_VAR(uint32_t, amupd_samplingJoint, 0x020C4908);
GAME_VAR(int32_t, amupd_debugBlend, 0x00BB6460);
GAME_VAR(int32_t, amupd_debugUpdate, 0x00BB6464);
GAME_VAR(int32_t, amupd_debugOn, 0x020C48FC);
GAME_VAR(uint8_t*, amupd_debugModel, 0x020C48F8);
GAME_VAR(int32_t, amupd_rootMotionOn, 0x00BB645C);
GAME_VAR(int32_t, amupd_dropFinished, 0x020C4B14);
GAME_VAR(int32_t, amupd_noBlending, 0x020C4B10);
GAME_VAR(int32_t, amupd_noSkinning, 0x020C4B28);
GAME_VAR(void*, amupd_camera, 0x00C3CCF4);

GAME_FN(void, __cdecl, AmUpd_FindKeys9, 0x006BD5D0, uint8_t* pose, uint32_t keySize, int32_t time);
GAME_FN(void, __cdecl, AmUpd_FindKeys16, 0x006BD7C0, uint8_t* pose, uint32_t keySize, uint32_t time);
GAME_FN(CVector*, __cdecl, AmUpd_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, AmUpd_VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void*, __cdecl, AmUpd_CameraFrame, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, AmUpd_FrameLTM, 0x006C9A90, void* frame);
GAME_FN(void, __thiscall, AmUpd_EmptyList, 0x006B5AB0, uint8_t* model, uint32_t list);
GAME_FN(uint32_t, __cdecl, AmUpd_AnimKind, 0x006B1A70, uint8_t* anim);
GAME_FN(void, __thiscall, AmUpd_BlendPoses, 0x006BCDC0, void* manager, uint8_t* to, uint8_t* a, uint8_t* b,
        float weight, uint8_t* mask);
GAME_FN(void, __cdecl, AmUpd_FillJointTransforms, 0x006B44B0, uint8_t* skeleton);
GAME_FN(void*, __thiscall, AmUpd_GetBone, 0x00464EA0, void* hierarchy, int bone);
GAME_FN(void, __thiscall, AmUpd_QuatToMatrix, 0x00413C50, const float* quat, float* out);
GAME_FN(float*, __thiscall, AmUpd_TransformMultiply, 0x00406830, const float* self, float* out,
        const float* other);
GAME_FN(uint8_t*, __thiscall, AmUpd_ListRemove, 0x005F3DE0, void* list, LinkIter* it);
GAME_FN(void, __thiscall, AmUpd_BlendOut, 0x006BD480, void* manager, uint8_t* model, uint8_t* track,
        float time);
GAME_FN(void, __thiscall, AmUpd_StepFaders, 0x006B5AF0, uint8_t* model, float dt);
GAME_FN(void, __thiscall, AmUpd_PoseFromTracks, 0x006BD2B0, void* manager, uint8_t* model, float dt);
GAME_FN(int, __thiscall, AmUpd_BlendTracks, 0x006BE890, void* manager, uint8_t* model, float dt,
        const CVector* offset);
GAME_FN(void, __thiscall, AmUpd_RunCallbacks, 0x006BE760, void* manager, uint8_t* model);
GAME_FN(void, __thiscall, AmUpd_UpdateSkeleton, 0x006B6640, uint8_t* model);
GAME_FN(void, __thiscall, AmUpd_ApplySkin, 0x006B95E0, uint8_t* model, float dt);
GAME_FN(void, __thiscall, AmUpd_ApplyUnskinned, 0x006B93A0, uint8_t* model, float dt, int noHierarchy);
GAME_FN(int, __thiscall, AmUpd_GetBoneTransform, 0x006BF570, void* manager, uint8_t* model, int bone,
        float* out, void* node);
GAME_FN(void, __thiscall, AmUpd_UpdateTracks, 0x006C0710, void* manager, uint8_t* model, float dt,
        int blend, const CVector* offset);
GAME_FN(void, __thiscall, AmUpd_Finish, 0x006C1840, void* manager, uint8_t* model, float dt, int blend,
        const CVector* offset);

inline void DebugBreakImport() {
    re::PtrCast<void(__stdcall*)()>(*reinterpret_cast<re::Ptr32<void>*>(0x008FC05C))();
}

template <typename R, typename... A>
inline R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}

// a mask's weight for an anim's kind (-1 past its weights)
inline float MaskWeight(uint8_t* mask, uint8_t* anim) {
    uint32_t kind = AmUpd_AnimKind(anim);
    if (kind < At<uint32_t>(mask, 0x24)) {
        return At<float*>(mask, 0x20)[kind];
    }
    return -1.0f;
}

inline CVector Scaled(const CVector& v, float s) {
    return {v.x * s, v.y * s, v.z * s};
}

// 0x6BE3B0 (thiscall): an anim's joint at a time, into the manager's pose (+0x154): the
// time clamped to the anim, the codec's keys found (by 9-bit or 16-bit time) and the
// joint's entry interpolated (the last, looping root joint's with no socket count), its
// translation and rotation copied out
void __fastcall SampleJoint(uint8_t* self, re::Edx, uint8_t* anim, float time, uint32_t joint, uint8_t* hierarchy,
                            uint32_t* translation, uint32_t* rotation) {
    uint8_t* pose = At<uint8_t*>(self, 0x154);
    uint8_t* hxd = At<uint8_t*>(hierarchy, 0x10);
    if (At<float>(anim, 0xC) < time) {
        time = At<float>(anim, 0xC);
    }
    At<uint32_t>(pose, 0x2C) = At<uint32_t>(hxd, 0x5C);
    At<float>(pose, 4) = 0.0f;
    At<uint8_t*>(pose, 0) = At<uint8_t*>(anim, 0);
    uint8_t* data = At<uint8_t*>(anim, 0);
    if (data == nullptr) {
        return;
    }
    uint32_t* codec = At<uint32_t*>(data, 0);
    At<uint32_t>(pose, 0x24) = codec[1];
    At<uint32_t>(pose, 0x3C) = codec[3];
    At<uint32_t>(pose, 0x40) = codec[4];
    At<uint32_t>(pose, 0x44) = codec[5];
    At<uint32_t>(pose, 0x48) = codec[6];
    uint32_t sockets = At<uint32_t>(hierarchy, 0xA8);
    if (sockets != 0 && At<uint32_t>(hierarchy, 0xA4) == At<uint32_t>(hxd, 0x5C)) {
        amupd_samplingSocketCount = sockets;
    } else {
        amupd_samplingSocketCount = At<uint32_t>(hxd, 0x70);
    }
    uint32_t count = At<uint32_t>(pose, 0x2C);
    uint32_t keySize;
    switch (codec[0]) {
    case 1001: keySize = 0xC; break;
    case 1002: keySize = 8; break;
    case 1000: keySize = 0x14; break;
    case 1003: keySize = 0x14; break;
    case 1004: keySize = 0xC; break;
    case 999: keySize = 0x20; break;
    default: keySize = codec[1]; break;
    }
    time = time / At<float>(anim, 0xC);
    At<float>(pose, 4) = time;
    amupd_samplingAnim = anim;
    if (At<uint32_t>(anim, 0x18) & 2) {
        float t = time * 511.0f;
        AmUpd_FindKeys9(pose, keySize, int32_t(int64_t(t)) << 7);
    } else {
        float t = std::fabs(time);
        t = t * 65535.0f;
        AmUpd_FindKeys16(pose, keySize, uint16_t(int32_t(t)));
    }
    uint32_t flags = At<uint32_t>(anim, 0x18);
    uint32_t stride = codec[1];
    bool loopingRoot = (flags & 0x40) && (flags & 8);
    if (loopingRoot) {
        count--;
    }
    uint32_t* entry = reinterpret_cast<uint32_t*>(pose + 0x4C + stride * joint);
    bool sample = false;
    if (joint < count) {
        amupd_samplingJoint = joint;
        sample = true;
    } else if (joint == count && loopingRoot) {
        amupd_samplingSocketCount = 0;
        sample = true;
    }
    if (sample) {
        using Interpolate = void(__cdecl*)(uint32_t*, uint32_t, uint32_t, float, uint32_t);
        uint8_t* poseData = At<uint8_t*>(pose, 0);
        At<Interpolate>(pose, 0x44)(entry, entry[0], entry[1], At<float>(pose, 4), At<uint32_t>(poseData, 0x14));
    }
    uint8_t* at = pose + At<uint32_t>(pose, 0x24) * joint;
    translation[0] = At<uint32_t>(at, 0x64);
    translation[1] = At<uint32_t>(at, 0x68);
    translation[2] = At<uint32_t>(at, 0x6C);
    at = pose + At<uint32_t>(pose, 0x24) * joint;
    rotation[0] = At<uint32_t>(at, 0x54);
    rotation[1] = At<uint32_t>(at, 0x58);
    rotation[2] = At<uint32_t>(at, 0x5C);
    rotation[3] = At<uint32_t>(at, 0x60);
}

// 0x6BE760 (thiscall): the pending callbacks (+0x158 of them at +0x15C, their data 0x50
// further): kind 0 calls the model's callback (+0) with its data (+8); kind 1 works out
// the object's offset from the camera and drops it. The count's cleared.
void __fastcall RunCallbacks(uint8_t* self, re::Edx, uint8_t* model) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x158); i++) {
        uint8_t* entry = At<uint8_t*>(self, 0x15C + i * 4);
        int32_t kind = At<int32_t>(entry, 8);
        using Callback = void(__cdecl*)(uint8_t*, uint32_t, uint32_t);
        Callback callback = At<Callback>(model, 0);
        if (kind == 0 && callback != nullptr) {
            callback(entry, At<uint32_t>(self, 0x15C + i * 4 + 0x50), At<uint32_t>(model, 8));
        } else if (kind == 1) {
            CVector position = At<CVector>(At<uint8_t*>(model, 0xC), 0x6C);
            uint8_t* ltm = AmUpd_FrameLTM(AmUpd_CameraFrame(amupd_camera));
            CVector camera = At<CVector>(ltm, 0x24);
            CVector offset;
            AmUpd_VectorSub(&offset, &position, &camera);   // (unused)
        }
    }
    At<uint32_t>(self, 0x158) = 0;
}

// 0x6BE890 (thiscall): the model's tracks blended into its pose (the manager's +0x144):
// each track's sub-tracks first (those active weigh in; applied oldest first, each blended
// into the pose by its weight, their root motion mixed by weight or mask), then the track
// itself at its weight; the root motion summed through the masks and handed out when
// root motion's on and time moves, `offset` added to joint 1's translation. Returns 1.
int __fastcall BlendTracks(uint8_t* self, re::Edx, uint8_t* model, float dt, const CVector* offset) {
    uint8_t* skeleton = At<uint8_t*>(model, 0x14);
    if (skeleton == nullptr || At<void*>(At<uint8_t*>(model, 0x94), 0x10) == nullptr) {
        return 1;
    }
    uint8_t* pose = At<uint8_t*>(skeleton, 0x1C);
    CVector total = {0.0f, 0.0f, 0.0f};
    At<uint8_t*>(self, 0x144) = pose;
    uint8_t* original = pose;
    float rotation[4] = {1.0f, 0.0f, 0.0f, 0.0f};   // w, x, y, z
    if (amupd_debugBlend != 0 && amupd_debugOn != 0 && amupd_debugModel == model) {
        DebugBreakImport();
    }
    for (uint32_t i = 0; i < 7; i++) {
        CVector moved = {0.0f, 0.0f, 0.0f};
        uint8_t* track = At<uint8_t*>(model, 0x18 + i * 4);
        if (track == nullptr) {
            continue;
        }
        re::Ptr32<uint8_t>* list = reinterpret_cast<re::Ptr32<uint8_t>*>(model + 0x34 + i * 0xC);
        float weight = At<float>(track, 0xC);
        bool anyActive = false;
        CVector sub = {0.0f, 0.0f, 0.0f};
        float subWeight = 0.0f;
        if (!(weight < 1.0f)) {
            AmUpd_EmptyList(model, i);
        } else if (list[0] != nullptr) {
            At<uint32_t>(model, 0x98) |= 0x1000;
            // taken off the list, newest first, into a chain back to front
            uint8_t* prev = nullptr;
            while (list[0]) {
                uint8_t* node = list[0];
                uint8_t* next = At<uint8_t*>(node, 0);
                list[0] = next;
                if (next == nullptr) {
                    list[1] = nullptr;
                }
                At<uint8_t*>(node, 0) = nullptr;
                reinterpret_cast<int32_t*>(list)[2] -= 1;
                uint8_t* sub_ = node - 4;
                if (VCall<uint8_t>(sub_, 0x18)) {
                    subWeight = At<float>(sub_, 0xC) + subWeight;
                    anyActive = true;
                }
                At<uint8_t*>(node, 0) = prev;
                prev = node;
            }
            // applied oldest first and put back at the front
            uint8_t* node = prev;
            do {
                CVector subMoved = {0.0f, 0.0f, 0.0f};
                float subRotation[4] = {1.0f, 0.0f, 0.0f, 0.0f};
                uint8_t* next = At<uint8_t*>(node, 0);
                At<uint8_t*>(node, 0) = nullptr;
                uint8_t* sub_ = node - 4;
                float w = At<float>(sub_, 0xC);
                uint8_t* mask = At<uint8_t*>(sub_, 8);
                VCall<void>(sub_, 0x10, model, pose, &subMoved, subRotation);
                if (pose == original) {
                    pose = At<uint8_t*>(self, 0x148);
                } else {
                    AmUpd_BlendPoses(self, original, original, pose, w, mask);
                }
                if (mask == nullptr) {
                    float keep = 1.0f - w;
                    CVector a = Scaled(sub, keep);
                    CVector b = Scaled(subMoved, w);
                    CVector out;
                    sub = *AmUpd_VectorAdd(&out, &b, &a);
                } else {
                    float mw = MaskWeight(mask, At<uint8_t*>(sub_, 0x18)) * w;
                    CVector b = Scaled(subMoved, mw);
                    float keep = 1.0f - weight;
                    CVector a = Scaled(sub, keep);
                    CVector out;
                    sub = *AmUpd_VectorAdd(&out, &a, &b);
                }
                uint8_t* head = list[0];
                if (head == nullptr) {
                    At<uint8_t*>(node, 0) = nullptr;
                    list[1] = node;
                } else {
                    At<uint8_t*>(node, 0) = head;
                }
                reinterpret_cast<int32_t*>(list)[2] += 1;
                list[0] = node;
                node = next;
            } while (node != nullptr);
            At<uint32_t>(model, 0x98) &= ~0x1000u;
        }
        uint8_t* mask = At<uint8_t*>(track, 8);
        VCall<void>(track, 0x10, model, pose, &moved, rotation);
        if (pose == original) {
            pose = At<uint8_t*>(self, 0x148);
        } else {
            AmUpd_BlendPoses(self, original, original, pose, weight, mask);
        }
        float absWeight = std::fabs(subWeight);
        if (!(absWeight < 0.001f) || anyActive) {
            float keep = 1.0f - weight;
            CVector a = Scaled(sub, keep);
            CVector b = Scaled(moved, weight);
            CVector out;
            moved = *AmUpd_VectorAdd(&out, &b, &a);
        } else {
            moved = Scaled(moved, weight);
        }
        if (mask == nullptr) {
            total = moved;
        } else {
            float mw = MaskWeight(mask, At<uint8_t*>(track, 0x18)) * weight;
            CVector b = Scaled(moved, mw);
            float keep = 1.0f - mw;
            CVector a = Scaled(total, keep);
            CVector out;
            total = *AmUpd_VectorAdd(&out, &a, &b);
        }
    }
    if (amupd_rootMotionOn != 0 && 0.0f < dt) {
        CVector* translation = At<CVector*>(model, 0xA4);
        if (translation) {
            *translation = total;
        }
        float* quat = At<float*>(model, 0xA8);
        if (quat) {
            quat[0] = rotation[1];
            quat[1] = rotation[2];
            quat[2] = rotation[3];
            quat[3] = rotation[0];
        }
    }
    if (offset) {
        uint8_t* current = At<uint8_t*>(self, 0x144);
        uint8_t* entry = current + At<uint32_t>(current, 0x24) + 0x4C;   // joint 1
        At<float>(entry, 0x18) = At<float>(current, At<int32_t>(current, 0x24) + 0x64) + offset->x;
        At<float>(entry, 0x1C) = offset->y + At<float>(entry, 0x1C);
        At<float>(entry, 0x20) = offset->z + At<float>(entry, 0x20);
    }
    if (At<uint32_t>(model, 0x98) & 4) {
        uint8_t* object = At<uint8_t*>(model, 0xC);
        At<uint32_t>(object, 0x6C) = amupd_zeroVector[0];
        At<uint32_t>(object, 0x70) = amupd_zeroVector[1];
        At<uint32_t>(object, 0x74) = amupd_zeroVector[2];
        object = At<uint8_t*>(model, 0xC);
        At<uint32_t>(object, 0x38) = amupd_zeroVector[0];
        At<uint32_t>(object, 0x3C) = amupd_zeroVector[1];
        At<uint32_t>(object, 0x40) = amupd_zeroVector[2];
    }
    AmUpd_FillJointTransforms(skeleton);
    At<uint32_t>(model, 0x98) |= 0x80;
    return 1;
}

// 0x6BF570 (thiscall): a bone's transform (0x34: rotation, translation, scale). Once
// blended (0x100 clear) with a skeleton: the skeleton's first joint transform as it is
// (whatever the bone). Else the node's (null: the hierarchy's bone) local rotation and
// translation, scale 1, put under its joint's transform (or the object's at +0x48).
// Returns 0.
int __fastcall GetBoneTransform(void* self, re::Edx, uint8_t* model, int bone, uint32_t* out, void* node) {
    (void)self;
    if (!(At<uint32_t>(model, 0x98) & 0x100) && At<uint8_t*>(model, 0x14) != nullptr) {
        uint32_t* from = At<uint32_t*>(At<uint8_t*>(model, 0x14), 8);
        for (int i = 0; i < 13; i++) {
            out[i] = from[i];
        }
        return 0;
    }
    if (node == nullptr) {
        node = AmUpd_GetBone(At<void*>(model, 0x94), bone);
    }
    float local[13];
    local[12] = 1.0f;
    const uint32_t* q = VCall<const uint32_t*>(node, 4);
    uint32_t quat[4] = {q[0], q[1], q[2], q[3]};
    AmUpd_QuatToMatrix(reinterpret_cast<float*>(quat), local);
    local[12] = 1.0f;
    const float* t = VCall<const float*>(node, 0);
    local[9] = t[0];
    local[10] = t[1];
    local[11] = t[2];
    float result[13];
    const float* parent;
    if (At<uint8_t*>(model, 0x14) != nullptr) {
        parent = reinterpret_cast<const float*>(At<uint8_t*>(At<uint8_t*>(model, 0x14), 8) +
                                                At<int32_t>(node, 8) * 0x34);
    } else {
        parent = reinterpret_cast<const float*>(At<uint8_t*>(model, 0xC) + 0x48);
    }
    const uint32_t* from = reinterpret_cast<const uint32_t*>(AmUpd_TransformMultiply(parent, result, local));
    for (int i = 0; i < 13; i++) {
        out[i] = from[i];
    }
    return 0;
}

// 0x6C1190 (thiscall): a bone's position (0x16: no out)
int __fastcall GetBonePosition(void* self, re::Edx, uint8_t* model, int bone, CVector* out, void* node) {
    if (out == nullptr) {
        return 0x16;
    }
    float t[13];
    int result = AmUpd_GetBoneTransform(self, model, bone, t, node);
    if (result == 0) {
        out->x = t[9];
        out->y = t[10];
        out->z = t[11];
        result = 0;
    }
    return result;
}

// 0x6C11E0 (thiscall): a bone's third axis (the rotation's last column)
int __fastcall GetBoneAxis(void* self, re::Edx, uint8_t* model, int bone, CVector* out, void* node) {
    if (out == nullptr) {
        return 0x16;
    }
    float t[13];
    int result = AmUpd_GetBoneTransform(self, model, bone, t, node);
    if (result == 0) {
        out->x = t[2];
        out->y = t[5];
        out->z = t[8];
    }
    return result;
}

// 0x6C0710 (thiscall): the model's tracks stepped: each slot's finished sub-tracks
// dropped (never the list's first; only while dropping's on), an empty slot's list
// emptied, a slot above 0 whose track stopped blended out; the faders stepped; the pose
// made (blended when the model blends and `blend`, else straight from the tracks); the
// pending callbacks run. Not for models without a hierarchy, frozen or updated already.
void __fastcall UpdateTracks(uint8_t* self, re::Edx, uint8_t* model, float dt, int blend, const CVector* offset) {
    At<uint32_t>(self, 0x158) = 0;
    uint32_t flags = At<uint32_t>(model, 0x98);
    if (!(flags & 1)) {
        blend = 0;
    }
    uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
    if (hierarchy == nullptr || (flags & 0x400) || (flags & 2)) {
        return;
    }
    if (At<void*>(hierarchy, 0x10) == nullptr) {
        At<uint32_t>(model, 0x8C) |= 1;
        At<uint32_t>(model, 0x98) |= 0x400;
        return;
    }
    if (amupd_debugUpdate != 0 && amupd_debugOn != 0 && amupd_debugModel == model) {
        DebugBreakImport();
    }
    for (uint32_t i = 0; i < 7; i++) {
        re::Ptr32<uint8_t>* list = reinterpret_cast<re::Ptr32<uint8_t>*>(model + 0x34 + i * 0xC);
        uint8_t* track = At<uint8_t*>(model, 0x18 + i * 4);
        if (track == nullptr) {
            AmUpd_EmptyList(model, i);
            continue;
        }
        uint8_t* cur = list[0];   // esi
        uint8_t* prev = nullptr;  // edi
        uint8_t* prevPrev = nullptr;   // ebx
        uint8_t* visited = cur;   // ebp
        bool seenOne = false;
        LinkIter it = {cur, nullptr};
        while (cur) {
            bool erasedHead = false;
            uint8_t* sub = cur ? cur - 4 : nullptr;
            if (VCall<uint8_t>(sub, 0x18) && !VCall<uint8_t>(sub, 0xC, dt) && amupd_dropFinished != 0 && seenOne) {
                erasedHead = cur == list[0];
                AmUpd_ListRemove(list, &it);
                if (erasedHead) {
                    cur = list[0];
                    prev = nullptr;
                    visited = cur;
                    prevPrev = nullptr;
                } else {
                    cur = visited;
                    prev = prevPrev;
                }
                it.prev = prev;
                it.node = cur;
                VCall<void>(sub, 4);
            }
            if (cur == nullptr) {
                break;
            }
            if (!erasedHead) {
                prevPrev = prev;
                prev = cur;
                visited = cur;
                cur = At<uint8_t*>(cur, 0);
                seenOne = true;
                it.prev = prev;
                it.node = cur;
            }
        }
        if (!VCall<uint8_t>(track, 0xC, dt) && i != 0) {
            AmUpd_BlendOut(self, model, track, 0.0f);
        }
    }
    AmUpd_StepFaders(model, dt);
    if (blend != 0 && amupd_noBlending == 0) {
        AmUpd_BlendTracks(self, model, dt, offset);
    } else {
        AmUpd_PoseFromTracks(self, model, dt);
    }
    if (At<uint32_t>(self, 0x158) != 0) {
        AmUpd_RunCallbacks(self, model);
    }
    At<uint32_t>(model, 0x98) |= 0x400;
}

// 0x6C0970 (thiscall): a model's pose finished by hand (blended again when `reblend`):
// only once blended, or when forced; the skeleton updated, the callback run, the skin
// applied. 0: no skeleton
int __fastcall FinishPose(void* self, re::Edx, uint8_t* model, float dt, const CVector* offset, int reblend,
                          int force) {
    if (At<void*>(model, 0x14) == nullptr) {
        return 0;
    }
    if ((At<uint8_t>(model, 0x98) & 0x80) || uint8_t(force) != 0) {
        if (uint8_t(reblend) != 0) {
            AmUpd_BlendTracks(self, model, dt, offset);
        }
        AmUpd_UpdateSkeleton(model);
        if (At<uint32_t>(model, 0x98) & 0x200) {
            At<uint32_t>(model, 0x98) |= 0x100;
            return 1;
        }
        using Callback = void(__cdecl*)(uint8_t*, uint32_t);
        Callback callback = At<Callback>(model, 4);
        if (callback) {
            callback(model, At<uint32_t>(model, 8));
        }
    }
    if (!(At<uint32_t>(model, 0x98) & 0x200)) {
        AmUpd_ApplySkin(model, dt);
    }
    At<uint32_t>(model, 0x98) |= 0x100;
    return 1;
}

// 0x6C1840 (thiscall): after the tracks: without an HXD (or blending off, or not
// blending) the unskinned update; else, once blended (0x80), the skeleton, the callback
// and the skin, as FinishPose
void __fastcall Finish(void* self, re::Edx, uint8_t* model, float dt, int blend, const CVector*) {
    (void)self;
    uint32_t flags = At<uint32_t>(model, 0x98);
    int doBlend = (flags & 1) ? blend : 0;
    uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
    if (hierarchy == nullptr) {
        return;
    }
    if (doBlend == 0 && (flags & 0x80)) {
        doBlend = 1;
    }
    if (At<void*>(hierarchy, 0x10) == nullptr) {
        if (!(flags & 2)) {
            AmUpd_ApplyUnskinned(model, dt, 1);
        }
        return;
    }
    if (doBlend == 0 || amupd_noSkinning != 0) {
        if (!(flags & 2)) {
            AmUpd_ApplyUnskinned(model, dt, 0);
        }
        return;
    }
    if (At<void*>(model, 0x14) == nullptr) {
        return;
    }
    if (flags & 0x80) {
        AmUpd_UpdateSkeleton(model);
        if (At<uint32_t>(model, 0x98) & 0x200) {
            At<uint32_t>(model, 0x98) |= 0x100;
            return;
        }
        using Callback = void(__cdecl*)(uint8_t*, uint32_t);
        Callback callback = At<Callback>(model, 4);
        if (callback) {
            callback(model, At<uint32_t>(model, 8));
        }
    }
    if (!(At<uint32_t>(model, 0x98) & 0x200)) {
        AmUpd_ApplySkin(model, dt);
    }
    At<uint32_t>(model, 0x98) |= 0x100;
}

// 0x6C1910 (thiscall): a model's update: its tracks, then the rest
void __fastcall Update(void* self, re::Edx, uint8_t* model, float dt, int blend, const CVector* offset) {
    AmUpd_UpdateTracks(self, model, dt, blend, offset);
    AmUpd_Finish(self, model, dt, blend, offset);
}

} // namespace

#define AMUPD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMUPD_REVERSED("AM_Manager::SampleJoint", 0x006BE3B0, &SampleJoint);
AMUPD_REVERSED("AM_Manager::RunCallbacks", 0x006BE760, &RunCallbacks);
AMUPD_REVERSED("AM_Manager::BlendTracks", 0x006BE890, &BlendTracks);
AMUPD_REVERSED("AM_Manager::GetBoneTransform", 0x006BF570, &GetBoneTransform);
AMUPD_REVERSED("AM_Manager::GetBonePosition", 0x006C1190, &GetBonePosition);
AMUPD_REVERSED("AM_Manager::GetBoneAxis", 0x006C11E0, &GetBoneAxis);
AMUPD_REVERSED("AM_Manager::UpdateTracks", 0x006C0710, &UpdateTracks);
AMUPD_REVERSED("AM_Manager::FinishPose", 0x006C0970, &FinishPose);
AMUPD_REVERSED("AM_Manager::Finish", 0x006C1840, &Finish);
AMUPD_REVERSED("AM_Manager::Update", 0x006C1910, &Update);
