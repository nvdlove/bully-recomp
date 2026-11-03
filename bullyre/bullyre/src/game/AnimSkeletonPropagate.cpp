// AM_ skeletons (+0 flags, +4 the joint count, +8 the joints' transforms, 0x34 each,
// +0xC the joints, 0x14 each with +8 the push/pop flags and +0xC the joint's frame, +0x10
// the model's root node, +0x14 / +0x18 the parent skeleton and joint it hangs from):
// turning the joints' local transforms into model space in their depth-first order and
// handing them to the frames. 0x6B6310..0x6B66C0.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct Xform {
    float rotation[9];
    float translation[3];
    float scale;
};
static_assert(sizeof(Xform) == 0x34);

// skeleton flags
constexpr uint32_t kAttached = 1;         // hangs from a parent skeleton's joint
constexpr uint32_t kSetFrameLocals = 0x1000;
constexpr uint32_t kSetFrameWorlds = 0x2000;
constexpr uint32_t kModelSpace = 0x4000;  // relative to the model's node (identity parent)

GAME_VAR(uint8_t*, sk_dirtyNodes, 0x00BC5BC8);  // the head of the frames-to-update list

GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, Xform* t);
GAME_FN(Xform*, __thiscall, NiTransform_Multiply, 0x00406830, const Xform* self, Xform* out, const Xform* other);
GAME_FN(void*, __cdecl, Frame_GetParent, 0x006C7370, void* frame);
GAME_FN(int, __cdecl, Frame_Fn6C9BA0, 0x006C9BA0, void* frame);
GAME_FN(Xform*, __cdecl, Node_GetWorldTransform, 0x006CA0A0, void* node);
GAME_FN(Xform*, __cdecl, Node_GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __cdecl, Frame_Update, 0x006C9AF0, void* frame);
GAME_FN(void, __cdecl, Skeleton_Propagate, 0x006B6310, uint8_t* skeleton);

// 0x6B6310
void __cdecl Propagate(uint8_t* s) {
    Xform modelWorld;   // F-0x184
    Xform identity;     // F-0x11C
    Xform local;        // F-0x150
    Xform product;      // F-0xE8
    modelWorld.scale = 1.0f;
    identity.scale = 1.0f;
    local.scale = 1.0f;
    uint32_t flags = At<uint32_t>(s, 0);
    const Xform* parent;
    void* node = nullptr;
    bool findWorld = false;
    if ((flags & kAttached) && At<int32_t>(s, 0x18) != -1) {
        uint8_t* parentSkeleton = At<uint8_t*>(s, 0x14);
        parent = reinterpret_cast<Xform*>(At<uint8_t*>(parentSkeleton, 8) + At<int32_t>(s, 0x18) * 0x34);
        if ((flags & kSetFrameWorlds) && (flags & kModelSpace)) {
            node = At<void*>(parentSkeleton, 0x10);
            findWorld = true;
        }
    } else if (flags & kModelSpace) {
        NiTransform_MakeIdentity(&identity);
        parent = &identity;
        if (flags & kSetFrameWorlds) {
            node = At<void*>(s, 0x10);
            findWorld = true;
        }
    } else {
        parent = &modelWorld;
        node = At<void*>(s, 0x10);
        findWorld = true;
    }
    if (findWorld) {
        // the model node's parent's world transform (built up the parents when it's stale)
        if (node == nullptr || Frame_GetParent(node) == nullptr) {
            NiTransform_MakeIdentity(&modelWorld);
        } else {
            void* p = Frame_GetParent(node);
            if (Frame_Fn6C9BA0(p) == 0) {
                modelWorld = *Node_GetWorldTransform(p);
            } else {
                Xform below;  // F-0xB4
                below.scale = 1.0f;
                modelWorld = *Node_GetLocalTransform(p);
                for (void* q = Frame_GetParent(p); q; q = Frame_GetParent(q)) {
                    below = modelWorld;
                    modelWorld = *NiTransform_Multiply(Node_GetLocalTransform(q), &product, &below);
                }
            }
        }
    }
    uint32_t setWorlds = flags & kSetFrameWorlds;
    if (setWorlds) {
        uint8_t* entry = At<uint8_t*>(At<uint8_t*>(s, 0x10), 0x8C);
        if (!(entry[3] & 3)) {
            // onto the dirty list
            At<uint8_t*>(entry, 0xC) = sk_dirtyNodes;
            At<re::Ptr32<uint8_t>*>(At<uint8_t*>(At<uint8_t*>(s, 0x10), 0x8C), 0x10) = &sk_dirtyNodes;
            At<uint8_t*>(sk_dirtyNodes, 4) = At<uint8_t*>(At<uint8_t*>(s, 0x10), 0x8C) + 0xC;
            sk_dirtyNodes = At<uint8_t*>(At<uint8_t*>(s, 0x10), 0x8C) + 0xC;
            At<uint8_t*>(At<uint8_t*>(s, 0x10), 0x8C)[3] |= 2;
        }
    }
    Xform* t = At<Xform*>(s, 8);
    re::Ptr32<const Xform> stack[31];
    re::Ptr32<const Xform>* top = stack;
    const uint32_t* jointFlags = reinterpret_cast<const uint32_t*>(At<uint8_t*>(s, 0xC) + 8);
    for (int i = 0; i < At<int32_t>(s, 4); i++, t++, jointFlags += 5) {
        local = *t;
        *t = *NiTransform_Multiply(parent, &product, &local);
        void* frame = reinterpret_cast<const re::Ptr32<void>*>(jointFlags)[1];
        if (frame) {
            bool toWorld = false;
            if (flags & kSetFrameLocals) {
                *Node_GetLocalTransform(frame) = local;
                if (setWorlds) {
                    toWorld = true;
                } else {
                    Frame_Update(frame);
                }
            } else if (setWorlds) {
                toWorld = true;
            }
            if (toWorld) {
                const Xform* world = (flags & kModelSpace) ? NiTransform_Multiply(&modelWorld, &product, t) : t;
                At<Xform>(frame, 0x48) = *world;
                At<uint8_t>(frame, 3) = uint8_t((At<uint8_t>(frame, 3) & 0xFB) | 8);
            }
        }
        switch (*jointFlags & 3) {
        case 0:
            parent = t;
            break;
        case 1:
            parent = *--top;
            break;
        case 2:
            *top++ = parent;
            parent = t;
            break;
        default:
            break;
        }
    }
}

// 0x6B6640: the model's skeleton propagated when it changed (flag 0x80 of +0x98), the root
// joint's position handed to the object with flag 4
void __fastcall Model_UpdateSkeleton(uint8_t* self, re::Edx) {
    uint8_t* skeleton = At<uint8_t*>(self, 0x14);
    if (skeleton == nullptr || !(At<uint8_t>(self, 0x98) & 0x80)) {
        return;
    }
    Skeleton_Propagate(skeleton);
    At<uint32_t>(self, 0x98) &= ~0x80u;
    if (At<uint8_t>(self, 0x98) & 4) {
        const uint32_t* root = reinterpret_cast<const uint32_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x14), 8) + 0x58);
        uint8_t* object = At<uint8_t*>(self, 0xC);
        At<uint32_t>(object, 0x6C) = root[0];
        At<uint32_t>(object, 0x70) = root[1];
        At<uint32_t>(object, 0x74) = root[2];
        root = reinterpret_cast<const uint32_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x14), 8) + 0x58);
        object = At<uint8_t*>(self, 0xC);
        At<uint32_t>(object, 0x38) = root[0];
        At<uint32_t>(object, 0x3C) = root[1];
        At<uint32_t>(object, 0x40) = root[2];
    }
    At<int32_t>(self, 0x90) = -1;
    At<int32_t>(self, 0x8C) = -1;
}

} // namespace

#define AMSKELPROP_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMSKELPROP_REVERSED("AM_Skeleton::Propagate", 0x006B6310, &Propagate);
AMSKELPROP_REVERSED("AM_Model::UpdateSkeleton", 0x006B6640, &Model_UpdateSkeleton);
