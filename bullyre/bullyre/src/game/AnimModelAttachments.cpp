// AM_ models' attachments (a list: +0xB0 the head, +0xC4 the count; nodes +0 the
// attachment, +8 the next; an attachment's +0xC parent socket, +0x10 its node, +0x14 the
// child model) and per-slot object lists (+0x34 + slot * 0xC: head, tail, count; the
// slot's factory at +0x18 + slot * 4). 0x6B9290..0x6B960C.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

struct ListIter {
    re::Ptr32<uint8_t> node;
    re::Ptr32<uint8_t> previous;
};

inline void* const kAnimPose = reinterpret_cast<void*>(0x020C4910);
constexpr uintptr_t kDebugBreak = 0x008FC05C;

GAME_VAR(int32_t, att_slotListLimit, 0x00BB6128);
GAME_VAR(int32_t, att_breakEnabled, 0x00BB6268);
GAME_VAR(int32_t, att_breakArmed, 0x020C48FC);
GAME_VAR(uint8_t*, att_breakModel, 0x020C48F8);

GAME_FN(uint8_t*, __thiscall, List_Remove, 0x005F3DE0, uint8_t* list, ListIter* it);
GAME_FN(void, __thiscall, Model_UpdateAttachment, 0x006B5C60, uint8_t* model, uint8_t* attachment, float dt,
        int withSkeleton);
GAME_FN(void, __thiscall, Model_UpdateAttachments, 0x006B93A0, uint8_t* model, float dt, int withSkeleton);
GAME_FN(void, __thiscall, Model_CentreSphereOnRoot, 0x006B4530, uint8_t* model);
GAME_FN(void, __thiscall, Pose_UpdateModel, 0x006C1910, void* sampler, uint8_t* model, float dt, int, uintptr_t);

// 0x6B9290: adds an object to slot `slot`'s list (made by the slot's factory when null);
// a full list first loses its object with the lowest +0xC
void __fastcall Model_AddToSlot(uint8_t* self, re::Edx, int slot, uint8_t* object) {
    if (At<void*>(self, 0x18 + slot * 4) == nullptr) {
        return;
    }
    uint8_t* list = self + 0x34 + slot * 0xC;
    if (At<int32_t>(list, 8) >= att_slotListLimit - 1) {
        float lowest = 3.40282347e+38f;
        ListIter it{At<uint8_t*>(list, 0), nullptr};
        uint8_t* previous = nullptr;
        for (uint8_t* n = At<uint8_t*>(list, 0); n; n = At<uint8_t*>(n, 0)) {
            uint8_t* owner = n ? n - 4 : nullptr;
            if (At<float>(owner, 0xC) < lowest) {
                lowest = At<float>(owner, 0xC);
                it.node = n;
                it.previous = previous;
            }
            previous = n;
        }
        uint8_t* removed = List_Remove(list, &it);
        uint8_t* owner = removed ? removed - 4 : nullptr;
        vc::Call<void>(owner, 4);
    }
    if (object == nullptr) {
        object = vc::Call<uint8_t*>(At<void*>(self, 0x18 + slot * 4), 8);
    }
    uint8_t* link = object ? object + 4 : nullptr;
    uint8_t* head = At<uint8_t*>(list, 0);
    if (head) {
        At<uint8_t*>(link, 0) = head;
        At<int32_t>(list, 8) += 1;
        At<uint8_t*>(list, 0) = link;
        return;
    }
    At<uint8_t*>(link, 0) = nullptr;
    At<int32_t>(list, 8) += 1;
    At<uint8_t*>(list, 4) = link;
    At<uint8_t*>(list, 0) = link;
}

// 0x6B93A0: places the attachments (those hanging from a changed joint, from a model
// without a skeleton, or of a child without one), then their children; repeats while
// placing them changed joints again (flag 0x40 guards against re-entry)
void __fastcall Model_UpdateAttachmentsImpl(uint8_t* self, re::Edx, float dt, int withSkeleton) {
    if (At<uint8_t>(self, 0x98) & 0x40) {
        return;
    }
    do {
        At<uint32_t>(self, 0x98) |= 0x440;
        uint32_t changed[2] = {At<uint32_t>(self, 0x8C), At<uint32_t>(self, 0x90)};
        At<uint32_t>(self, 0x90) = 0;
        At<uint32_t>(self, 0x8C) = 0;
        if (At<int32_t>(self, 0xC4) != 0) {
            for (uint8_t* node = At<uint8_t*>(self, 0xB0); node; node = At<uint8_t*>(node, 8)) {
                uint8_t* attachment = At<uint8_t*>(node, 0);
                uint8_t* child = At<uint8_t*>(attachment, 0x14);
                if (att_breakEnabled != 0 && att_breakArmed != 0 && att_breakModel == self) {
                    vc::Import<void>(kDebugBreak);
                }
                uint8_t* socket = At<uint8_t*>(attachment, 0xC);
                if (socket) {
                    uint32_t joint = At<uint32_t>(socket, 8);
                    const uint32_t* bits = changed;
                    if (int32_t(joint) >= 32) {
                        bits += joint >> 5;
                        joint -= (joint >> 5) << 5;
                    }
                    bool jointChanged = (*bits & (1u << joint)) != 0;
                    if (!jointChanged && child && At<void*>(child, 0x14) != nullptr) {
                        continue;
                    }
                }
                Model_UpdateAttachment(self, attachment, dt, withSkeleton);
                if (child == nullptr) {
                    continue;
                }
                uint8_t* hierarchy = At<uint8_t*>(child, 0x94);
                if (hierarchy && At<void*>(hierarchy, 0x10)) {
                    At<uint32_t>(child, 0x98) &= ~2u;
                    Pose_UpdateModel(kAnimPose, child, dt, withSkeleton, 0);
                    At<uint32_t>(child, 0x98) |= 2;
                } else {
                    Model_UpdateAttachments(child, dt, withSkeleton);
                    At<uint32_t>(child, 0x98) |= 0x400;
                }
            }
        }
        At<uint32_t>(self, 0x98) &= ~0x40u;
        if (At<uint32_t>(self, 0x8C) == 0 && At<uint32_t>(self, 0x90) == 0) {
            return;
        }
    } while (!(At<uint8_t>(self, 0x98) & 0x40));
}

// 0x6B9550: places the one attachment holding `child` (then the child's pose)
void __fastcall Model_UpdateChild(uint8_t* self, re::Edx, uint8_t* child, float dt, int) {
    uint8_t* attachment = At<int32_t>(self, 0xC4) != 0 ? *At<re::Ptr32<uint8_t>*>(self, 0xB0) : nullptr;
    if (attachment == nullptr) {
        return;
    }
    while (At<uint8_t*>(attachment, 0x14) != child) {
        uint8_t* link = At<uint8_t*>(attachment, 0x10);
        if (link == nullptr) {
            return;
        }
        re::Ptr32<uint8_t>* next = At<re::Ptr32<uint8_t>*>(link, 8);
        if (next == nullptr) {
            return;
        }
        attachment = *next;
        if (attachment == nullptr) {
            return;
        }
    }
    Model_UpdateAttachment(self, attachment, dt, 1);
    if (At<void*>(child, 0x14) != nullptr && !(At<uint32_t>(child, 0x98) & 0x400)) {
        At<uint32_t>(child, 0x98) &= ~2u;
        Pose_UpdateModel(kAnimPose, child, dt, 1, 0);
        At<uint32_t>(child, 0x98) |= 2;
    }
}

// 0x6B95E0: when joints changed, the attachments placed and the bounding sphere moved
void __fastcall Model_UpdateIfChanged(uint8_t* self, re::Edx, float dt) {
    if (At<uint32_t>(self, 0x8C) != 0 || At<uint32_t>(self, 0x90) != 0) {
        Model_UpdateAttachments(self, dt, 1);
        Model_CentreSphereOnRoot(self);
    }
}

} // namespace

#define AMMODELATT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMMODELATT_REVERSED("AM_Model::AddToSlot", 0x006B9290, &Model_AddToSlot);
AMMODELATT_REVERSED("AM_Model::UpdateAttachments", 0x006B93A0, &Model_UpdateAttachmentsImpl);
AMMODELATT_REVERSED("AM_Model::UpdateChild", 0x006B9550, &Model_UpdateChild);
AMMODELATT_REVERSED("AM_Model::UpdateIfChanged", 0x006B95E0, &Model_UpdateIfChanged);
