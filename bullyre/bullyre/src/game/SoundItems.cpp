// The sound objects' accessors: SoundThing's flags (+0x10) and SFXItem's fields
// (vtable 0x905A7C, 0x74 bytes), and SFXItem's constructor.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

GAME_FN(void*, __thiscall, SoundBase_Construct, 0x0059E730, void* self);

// ---- SoundThing: flags at +0x10 --------------------------------------------------------
// (the setters test for exactly 1, not non-zero)

// 0x449670 (thiscall, vf15): bit 0 (the whole of eax)
uint32_t __fastcall SoundThing_GetFlag0(void* self) {
    return At<uint32_t>(self, 0x10) & 1;
}

// 0x449680 (thiscall, vf14)
void __fastcall SoundThing_SetFlag0(void* self, re::Edx, int on) {
    if (uint8_t(on) == 1) {
        At<uint32_t>(self, 0x10) |= 1;
        return;
    }
    At<uint32_t>(self, 0x10) &= ~1u;
}

// 0x4496A0 (thiscall, vf17): bit 1 (in al)
uint8_t __fastcall SoundThing_GetFlag1(void* self) {
    return uint8_t(At<uint32_t>(self, 0x10) >> 1) & 1;
}

// 0x4496B0 (thiscall, vf16)
void __fastcall SoundThing_SetFlag1(void* self, re::Edx, int on) {
    if (uint8_t(on) == 1) {
        At<uint32_t>(self, 0x10) |= 2;
        return;
    }
    At<uint32_t>(self, 0x10) &= ~2u;
}

// 0x4496D0 (thiscall, vf19): bit 2 (in al)
uint8_t __fastcall SoundThing_GetFlag2(void* self) {
    return uint8_t(At<uint32_t>(self, 0x10) >> 2) & 1;
}

// 0x4496E0 (thiscall, vf18)
void __fastcall SoundThing_SetFlag2(void* self, re::Edx, int on) {
    if (uint8_t(on) == 1) {
        At<uint32_t>(self, 0x10) |= 4;
        return;
    }
    At<uint32_t>(self, 0x10) &= ~4u;
}

// ---- SFXItem ------------------------------------------------------------------------------

// 0x449640 (thiscall): the constructor
void* __fastcall SFXItem_Construct(void* self) {
    SoundBase_Construct(self);
    At<uint32_t>(self, 0) = 0x00905A7C;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
    At<float>(self, 0x40) = 0.0f;
    At<int32_t>(self, 0x48) = 0;
    At<int32_t>(self, 0x4C) = 0;
    At<float>(self, 0x44) = 0.0f;
    At<int32_t>(self, 0x64) = 0;
    return self;
}

// 0x449700 (vf21)
void __fastcall SFXItem_Set18(void* self, re::Edx, int32_t value) { At<int32_t>(self, 0x18) = value; }
// 0x449710 (vf25)
void __fastcall SFXItem_Set54(void* self, re::Edx, int32_t value) { At<int32_t>(self, 0x54) = value; }
// 0x449720 (vf27)
[[maybe_unused]] int32_t __fastcall SFXItem_Get58(void* self) { return At<int32_t>(self, 0x58); }
// 0x449730 (vf28)
void __fastcall SFXItem_Set58(void* self, re::Edx, int32_t value) { At<int32_t>(self, 0x58) = value; }
// 0x449740 (vf32)
void __fastcall SFXItem_Set72(void* self, re::Edx, int value) { At<uint8_t>(self, 0x72) = uint8_t(value); }
// 0x449750 (vf31)
[[maybe_unused]] uint8_t __fastcall SFXItem_Get72(void* self) { return At<uint8_t>(self, 0x72); }
// 0x449760 (vf33)
void __fastcall SFXItem_Set2C(void* self, re::Edx, int32_t value) { At<int32_t>(self, 0x2C) = value; }
// 0x449770 (vf35): the position (+0x20)
void __fastcall SFXItem_SetPosition(void* self, re::Edx, const int32_t* pos) {
    At<int32_t>(self, 0x20) = pos[0];
    At<int32_t>(self, 0x24) = pos[1];
    At<int32_t>(self, 0x28) = pos[2];
}
// 0x449790 (vf37)
void __fastcall SFXItem_Set5E(void* self, re::Edx, int value) { At<uint8_t>(self, 0x5E) = uint8_t(value); }
// 0x4497A0 (vf36)
[[maybe_unused]] uint8_t __fastcall SFXItem_Get5E(void* self) { return At<uint8_t>(self, 0x5E); }
// 0x4497B0 (vf39)
[[maybe_unused]] uint8_t __fastcall SFXItem_Get6C(void* self) { return At<uint8_t>(self, 0x6C); }
// 0x4497C0 (vf38)
void __fastcall SFXItem_Set6C(void* self, re::Edx, int value) { At<uint8_t>(self, 0x6C) = uint8_t(value); }
// 0x4497D0 (vf41)
[[maybe_unused]] uint8_t __fastcall SFXItem_Get6D(void* self) { return At<uint8_t>(self, 0x6D); }
// 0x4497E0 (vf40)
void __fastcall SFXItem_Set6D(void* self, re::Edx, int value) { At<uint8_t>(self, 0x6D) = uint8_t(value); }
// 0x4497F0 (vf43)
[[maybe_unused]] uint8_t __fastcall SFXItem_Get6E(void* self) { return At<uint8_t>(self, 0x6E); }
// 0x449800 (vf42)
void __fastcall SFXItem_Set6E(void* self, re::Edx, int value) { At<uint8_t>(self, 0x6E) = uint8_t(value); }
// 0x449810 (vf9)
void __fastcall SFXItem_SetC(void* self, re::Edx, float value) { At<float>(self, 0xC) = value; }
// 0x449820 (vf8)
[[maybe_unused]] float __fastcall SFXItem_GetC(void* self) { return At<float>(self, 0xC); }

// ---- registration -------------------------------------------------------------------------

#define C04A_SND(name, address, fn)                                                            \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04A_SND("SFXItem::Construct", 0x00449640, &SFXItem_Construct);
C04A_SND("SoundThing::GetFlag0", 0x00449670, &SoundThing_GetFlag0);
C04A_SND("SoundThing::SetFlag0", 0x00449680, &SoundThing_SetFlag0);
C04A_SND("SoundThing::GetFlag1", 0x004496A0, &SoundThing_GetFlag1);
C04A_SND("SoundThing::SetFlag1", 0x004496B0, &SoundThing_SetFlag1);
C04A_SND("SoundThing::GetFlag2", 0x004496D0, &SoundThing_GetFlag2);
C04A_SND("SoundThing::SetFlag2", 0x004496E0, &SoundThing_SetFlag2);
C04A_SND("SFXItem::Set18", 0x00449700, &SFXItem_Set18);
C04A_SND("SFXItem::Set54", 0x00449710, &SFXItem_Set54);
// 0x00449720: [8b 41 58 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_00449720(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x58);
}
RE_UNHOOKED_FN("SFXItem::Get58", 0x00449720, &Tiny_00449720);
C04A_SND("SFXItem::Set58", 0x00449730, &SFXItem_Set58);
C04A_SND("SFXItem::Set72", 0x00449740, &SFXItem_Set72);
// 0x00449750: [8a 41 72 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_00449750(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x72);
}
RE_UNHOOKED_FN("SFXItem::Get72", 0x00449750, &Tiny_00449750);
C04A_SND("SFXItem::Set2C", 0x00449760, &SFXItem_Set2C);
C04A_SND("SFXItem::SetPosition", 0x00449770, &SFXItem_SetPosition);
C04A_SND("SFXItem::Set5E", 0x00449790, &SFXItem_Set5E);
// 0x004497A0: [8a 41 5e c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_004497A0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x5E);
}
RE_UNHOOKED_FN("SFXItem::Get5E", 0x004497A0, &Tiny_004497A0);
// 0x004497B0: [8a 41 6c c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_004497B0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x6C);
}
RE_UNHOOKED_FN("SFXItem::Get6C", 0x004497B0, &Tiny_004497B0);
C04A_SND("SFXItem::Set6C", 0x004497C0, &SFXItem_Set6C);
// 0x004497D0: [8a 41 6d c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_004497D0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x6D);
}
RE_UNHOOKED_FN("SFXItem::Get6D", 0x004497D0, &Tiny_004497D0);
C04A_SND("SFXItem::Set6D", 0x004497E0, &SFXItem_Set6D);
// 0x004497F0: [8a 41 6e c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_004497F0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x6E);
}
RE_UNHOOKED_FN("SFXItem::Get6E", 0x004497F0, &Tiny_004497F0);
C04A_SND("SFXItem::Set6E", 0x00449800, &SFXItem_Set6E);
C04A_SND("SFXItem::SetC", 0x00449810, &SFXItem_SetC);
// 0x00449820: [d9 41 0c c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static float __fastcall Tiny_00449820(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const float*>(self + 0xC);
}
RE_UNHOOKED_FN("SFXItem::GetC", 0x00449820, &Tiny_00449820);

} // namespace
