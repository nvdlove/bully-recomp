// The animation blend drivers' small virtuals (BlendDriverBasic, FloorMotionDriver,
// ReachDriver: their class ids) and two constraint constructors built on 0x72F620.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

GAME_VAR(int16_t, bd_nextTypeId, 0x00BF3830);
GAME_FN(void*, __thiscall, Constraint_Construct, 0x0072F620, void* self);
GAME_FN(uint8_t, __cdecl, BlendDriver_IsA, 0x0044A120, int id);   // (ours below)

int16_t ClassId(uintptr_t guardAddress, uintptr_t idAddress) {
    uint32_t& guard = *reinterpret_cast<uint32_t*>(guardAddress);
    int16_t& id = *reinterpret_cast<int16_t*>(idAddress);
    if (!(guard & 1)) {
        guard |= 1;
        id = bd_nextTypeId;
        bd_nextTypeId = int16_t(bd_nextTypeId + 1);
    }
    return id;
}
int16_t RootId() { return ClassId(0x00C0F67C, 0x00C0F678); }
int16_t BlendDriverId() { return ClassId(0x00C0F684, 0x00C0F680); }
int16_t MotionDriverId() { return ClassId(0x00C0F68C, 0x00C0F688); }
int16_t FloorMotionDriverId() { return ClassId(0x00C0F694, 0x00C0F690); }
int16_t ReachDriverId() { return ClassId(0x00C0F69C, 0x00C0F698); }

// 0x44A120 (cdecl): a blend driver, or the root class
uint8_t __cdecl BlendDriver_IsAImpl(int id) {
    if (int16_t(id) == BlendDriverId()) {
        return 1;
    }
    return int16_t(id) == RootId();
}

// 0x44A270 (thiscall, vf0)
uint8_t __fastcall FloorMotionDriver_IsA(void*, re::Edx, int id) {
    if (int16_t(id) == FloorMotionDriverId()) {
        return 1;
    }
    if (int16_t(id) == MotionDriverId()) {
        return 1;
    }
    return BlendDriver_IsA(id);
}

// 0x44A310 (thiscall, vf0)
uint8_t __fastcall ReachDriver_IsA(void*, re::Edx, int id) {
    if (int16_t(id) == ReachDriverId()) {
        return 1;
    }
    if (int16_t(id) == MotionDriverId()) {
        return 1;
    }
    return BlendDriver_IsA(id);
}

// ---- BlendDriverBasic's fields ---------------------------------------------------------------

// 0x44A190 (vf21)
void __fastcall BlendDriver_SetC(void* self, re::Edx, int value) { At<uint8_t>(self, 0xC) = uint8_t(value); }
// 0x44A1A0 (NiDefaultClickRenderStep vf6, shared)
[[maybe_unused]] float __fastcall Get24(void* self) { return At<float>(self, 0x24); }
// 0x44A1B0 (vf9)
void __fastcall BlendDriver_Set20(void* self, re::Edx, float value) { At<float>(self, 0x20) = value; }
// 0x44A1C0 (vf10)
void __fastcall BlendDriver_Set10(void* self, re::Edx, int32_t value) { At<int32_t>(self, 0x10) = value; }
// 0x44A1D0 (vf12)
void __fastcall BlendDriver_Set14(void* self, re::Edx, float value) { At<float>(self, 0x14) = value; }
// 0x44A1E0 (FaceHeadingObjective vf4, shared)
[[maybe_unused]] float __fastcall Get14(void* self) { return At<float>(self, 0x14); }
// 0x44A1F0 (vf14)
void __fastcall BlendDriver_Set18(void* self, re::Edx, float value) { At<float>(self, 0x18) = value; }
// 0x44A200 (vf15)
[[maybe_unused]] float __fastcall BlendDriver_Get18(void* self) { return At<float>(self, 0x18); }

// 0x44A210 (vf16): the state (+0x28) is 1 or 2
int32_t __fastcall BlendDriver_IsActive(void* self) {
    int32_t state = At<int32_t>(self, 0x28);
    return (state == 1 || state == 2) ? 1 : 0;
}

// 0x44A230 (vf17): to state 2 (through vf20) unless 1 or 2 already
void __fastcall BlendDriver_Start(void* self) {
    int32_t state = At<int32_t>(self, 0x28);
    if (state == 1 || state == 2) {
        return;
    }
    vc::Call<void>(self, 0x50, 2);
}

// 0x44A250 (vf18): to state 3 (through vf20) unless 0 or 3
void __fastcall BlendDriver_Stop(void* self) {
    int32_t state = At<int32_t>(self, 0x28);
    if (state == 0 || state == 3) {
        return;
    }
    vc::Call<void>(self, 0x50, 3);
}

// ---- constraints (0x72F620's) -----------------------------------------------------------------

// 0x44A100 (thiscall): vtable 0x905B4C
void* __fastcall Constraint905B4C_Construct(void* self) {
    Constraint_Construct(self);
    At<uint32_t>(self, 0) = 0x00905B4C;
    return self;
}

// 0x44A2F0 (thiscall): vtable 0x905BAC, +0x34 a zero vector
void* __fastcall Constraint905BAC_Construct(void* self) {
    Constraint_Construct(self);
    At<uint32_t>(self, 0) = 0x00905BAC;
    At<float>(self, 0x34) = 0.0f;
    At<float>(self, 0x38) = 0.0f;
    At<float>(self, 0x3C) = 0.0f;
    return self;
}

// 0x44BEA0 / 0x44BF00 (thiscall, vf1): the vector deleting destructors (the elements'
// destructor only resets the vtable to 0x905B3C; 0x40 and 0x50 bytes)
template <int Size, uintptr_t Delete>
void* __fastcall ResetVtableDeletingDtor(void* self, re::Edx, int flags) {
    if (flags & 2) {
        int32_t* block = static_cast<int32_t*>(self) - 1;
        for (int i = block[0] - 1; i >= 0; i--) {
            At<uint32_t>(static_cast<uint8_t*>(self) + i * Size, 0) = 0x00905B3C;
        }
        if (flags & 1) {
            reinterpret_cast<void(__cdecl*)(void*)>(0x005EEAB0)(block);
        }
        return block;
    }
    At<uint32_t>(self, 0) = 0x00905B3C;
    if (flags & 1) {
        reinterpret_cast<void(__cdecl*)(void*)>(Delete)(self);
    }
    return self;
}

// ---- registration -------------------------------------------------------------------------

#define C04A_BLEND(name, address, fn)                                                          \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04A_BLEND("Constraint905B4C::Construct", 0x0044A100, &Constraint905B4C_Construct);
C04A_BLEND("BlendDriver::IsA", 0x0044A120, &BlendDriver_IsAImpl);
C04A_BLEND("BlendDriverBasic::SetC", 0x0044A190, &BlendDriver_SetC);
// 0x0044A1A0: [d9 41 24 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static float __fastcall Tiny_0044A1A0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const float*>(self + 0x24);
}
RE_UNHOOKED_FN("NiDefaultClickRenderStep::vf6", 0x0044A1A0, &Tiny_0044A1A0);
C04A_BLEND("BlendDriverBasic::Set20", 0x0044A1B0, &BlendDriver_Set20);
C04A_BLEND("BlendDriverBasic::Set10", 0x0044A1C0, &BlendDriver_Set10);
C04A_BLEND("BlendDriverBasic::Set14", 0x0044A1D0, &BlendDriver_Set14);
// 0x0044A1E0: [d9 41 14 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static float __fastcall Tiny_0044A1E0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const float*>(self + 0x14);
}
RE_UNHOOKED_FN("FaceHeadingObjective::vf4", 0x0044A1E0, &Tiny_0044A1E0);
C04A_BLEND("BlendDriverBasic::Set18", 0x0044A1F0, &BlendDriver_Set18);
// 0x0044A200: [d9 41 18 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static float __fastcall Tiny_0044A200(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const float*>(self + 0x18);
}
RE_UNHOOKED_FN("BlendDriverBasic::Get18", 0x0044A200, &Tiny_0044A200);
C04A_BLEND("BlendDriverBasic::IsActive", 0x0044A210, &BlendDriver_IsActive);
C04A_BLEND("BlendDriverBasic::Start", 0x0044A230, &BlendDriver_Start);
C04A_BLEND("BlendDriverBasic::Stop", 0x0044A250, &BlendDriver_Stop);
C04A_BLEND("FloorMotionDriver::IsA", 0x0044A270, &FloorMotionDriver_IsA);
C04A_BLEND("Constraint905BAC::Construct", 0x0044A2F0, &Constraint905BAC_Construct);
C04A_BLEND("ReachDriver::IsA", 0x0044A310, &ReachDriver_IsA);
C04A_BLEND("FloorMotionDriver::DeletingDtor", 0x0044BEA0, (&ResetVtableDeletingDtor<0x40, 0x00731780>));
C04A_BLEND("ReachDriver::DeletingDtor", 0x0044BF00, (&ResetVtableDeletingDtor<0x50, 0x007317B0>));

} // namespace
