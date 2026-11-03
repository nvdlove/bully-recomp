// The tuning attribute sets (the *Butes tracks, ButesObjects.cpp / ButesFactories.cpp):
// each one's defaults (0x728570..0x72A5D3, the "Then" call of its factory's New), the
// destructors 0x72A5E0..0x72A821 (the vtable back, its name and text fields released,
// then the track base's), and three helpers on the fields once loaded: ObjectButes'
// bone and weapon lookups, ProjectileButes' (effects, sound, colour) and PropExtraButes'
// copy of its values.
//
// The defaults are tables of the game's stores in its order (from its code): a field is
// an int, a float (by its bits), a byte, a text field (+ name: emptied through a
// temporary name key, then set to the game's string, or none) or a name key (set from
// the game's string, or none).

#include <bit>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "reversed.h"

namespace {

using actc::At;

// a defaults entry
struct ButeOp {
    uint8_t kind;
    uint16_t offset;
    uint32_t value;  // the bits; a text's address (0: none)
};
enum : uint8_t { kOpI32, kOpU8, kOpText, kOpName };
constexpr ButeOp I(int offset, int32_t value) {
    return {kOpI32, uint16_t(offset), uint32_t(value)};
}
constexpr ButeOp F(int offset, float value) {
    return {kOpI32, uint16_t(offset), std::bit_cast<uint32_t>(value)};
}
constexpr ButeOp B(int offset, int value) {
    return {kOpU8, uint16_t(offset), uint32_t(value)};
}
constexpr ButeOp S(int offset, uint32_t text) {
    return {kOpText, uint16_t(offset), text};
}
constexpr ButeOp N(int offset, uint32_t text) {
    return {kOpName, uint16_t(offset), text};
}

GAME_FN(void, __thiscall, BD_NameKey_FromString, 0x005FA890, uint32_t* key, const char* name);
GAME_FN(uint32_t*, __thiscall, BD_NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, BD_HashPath_Set, 0x005FA8C0, void* path, int32_t count, uint32_t* hashes);
GAME_FN(uint8_t*, __cdecl, BD_FindBoneInfo, 0x0072AEA0, void* path, void* root);
GAME_FN(void, __stdcall, BD_VectorDestruct, 0x004017C0, void* array, uint32_t size, int32_t count, void* dtor);
GAME_FN(int32_t, __cdecl, BD_GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(uint32_t, __cdecl, BD_HashName, 0x00576ED0, const char* name);
GAME_FN(uint16_t, __thiscall, BD_EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(int32_t, __thiscall, BD_Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(uint8_t*, __thiscall, BD_RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void*, __cdecl, BD_Malloc, 0x005EEAA0, uint32_t size);

GAME_VAR(void*, bdEffectLibrary, 0x00B6F1E8);
inline void* const bdSounds = reinterpret_cast<void*>(0x00CF3DA0);
GAME_VAR(uint8_t*, bdDefaultBone, 0x020C88C8);       // an ObjectButes' bone when it names none
inline const uint32_t* const bdBoneName0 = reinterpret_cast<const uint32_t*>(0x01B6531C);   // the bone paths' keys
inline const uint32_t* const bdBoneName1 = reinterpret_cast<const uint32_t*>(0x01B65394);
inline const uint32_t* const bdBoneName2 = reinterpret_cast<const uint32_t*>(0x01B652EC);
inline void* const bdBoneRoot1 = reinterpret_cast<void*>(0x01B65398);
inline void* const bdBoneRoot2 = reinterpret_cast<void*>(0x01B652E8);
inline void* const bdNameKeyDestruct = reinterpret_cast<void*>(0x0049A650);

// a hashed path (HashPath::Set)
struct BonePath2 {
    int32_t count;
    re::Ptr32<uint32_t> names;
};
static_assert(sizeof(BonePath2) == 8);

// (thiscall) one Butes' defaults: its table applied in order
template <const auto& Ops>
void __fastcall ButeDefaults(uint8_t* self, re::Edx) {
    for (const ButeOp& op : Ops) {
        switch (op.kind) {
        case kOpI32:
            At<uint32_t>(self, op.offset) = op.value;
            break;
        case kOpU8:
            self[op.offset] = uint8_t(op.value);
            break;
        case kOpText:
            actc::ClearName(self, op.offset);
            actc::StringRef_Assign(&At<uint32_t>(self, op.offset), op.value);
            break;
        default:
            BD_NameKey_FromString(&At<uint32_t>(self, op.offset), reinterpret_cast<const char*>(uintptr_t(op.value)));
            break;
        }
    }
}

// a destructor's field: a text field (+ name, set to none) or a name key (kNameKey: set
// from no string)
constexpr int kNameKey = 0x10000;

// (thiscall) one Butes' destructor: the vtable back, its fields released in order and
// their keys destructed in reverse, then the track base's destructor
template <uintptr_t Vt, int... Keys>
void __fastcall ButesDestruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = uint32_t(Vt);
    if constexpr (sizeof...(Keys) > 0) {
        constexpr int keys[] = {Keys...};
        for (int key : keys) {
            if (key & kNameKey) {
                BD_NameKey_FromString(&At<uint32_t>(self, key & 0xFFFF), nullptr);
            } else {
                actc::StringRef_Assign(&At<uint32_t>(self, key), 0);
            }
        }
        for (int i = int(sizeof...(Keys)) - 1; i >= 0; i--) {
            actc::NameKey_Destruct(self + (keys[i] & 0xFFFF));
        }
    }
    actc::TrackBase_Destruct(self);
}

#define BUTESDEF_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

} // namespace

// ---- the fields' lookups once loaded ------------------------------------------------------------------

// 0x7282A0 (thiscall): ObjectButes' lookups, each once: its bone (+0x80: 1 until looked
// up; the default bone, or +0x50's under the two path keys), its weapon (+0x84: -2 until
// looked up; +0x54's index, -1 without) and its second bone (+0x88, +0x5C's under the
// third key)
void __fastcall ObjectButes_Resolve(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x80) == 1) {
        At<uint8_t*>(self, 0x80) = bdDefaultBone;
        if (At<const char*>(self, 0x50)) {
            uint32_t names[3];
            BD_NameKey_Copy(&names[0], bdBoneName0);
            BD_NameKey_Copy(&names[1], bdBoneName1);
            actc::NameKey_Construct(&names[2], At<const char*>(self, 0x50));
            BonePath2 path;
            BD_HashPath_Set(&path, 3, names);
            At<uint8_t*>(self, 0x80) = BD_FindBoneInfo(&path, bdBoneRoot1);
            BD_VectorDestruct(names, 4, 3, bdNameKeyDestruct);
        }
    }
    if (At<int32_t>(self, 0x84) == -2) {
        if (At<const char*>(self, 0x54)) {
            At<int32_t>(self, 0x84) = BD_GetWeaponIndex(At<const char*>(self, 0x54));
        } else {
            At<int32_t>(self, 0x84) = -1;
        }
    }
    if (At<uint8_t*>(self, 0x88) == nullptr && At<const char*>(self, 0x5C)) {
        uint32_t names[2];
        BD_NameKey_Copy(&names[0], bdBoneName2);
        actc::NameKey_Construct(&names[1], At<const char*>(self, 0x5C));
        BonePath2 path;
        BD_HashPath_Set(&path, 2, names);
        At<uint8_t*>(self, 0x88) = BD_FindBoneInfo(&path, bdBoneRoot2);
        BD_VectorDestruct(names, 4, 2, bdNameKeyDestruct);
    }
}
BUTESDEF_REVERSED("ObjectButes::Resolve", 0x007282A0, &ObjectButes_Resolve);

// 0x7283C0 (thiscall): PropExtraButes' ten values (+0x18..+0x3C) copied once into a block
// of their own (+0x40; the allocation unchecked, as the game's)
void __fastcall PropExtraButes_SaveCopy(uint8_t* self, re::Edx) {
    if (At<uint32_t*>(self, 0x40) == nullptr) {
        At<void*>(self, 0x40) = BD_Malloc(0x28);
        for (int i = 0; i < 10; i++) {
            At<uint32_t*>(self, 0x40)[i] = At<uint32_t>(self, 0x18 + i * 4);
        }
    }
}
BUTESDEF_REVERSED("PropExtraButes::SaveCopy", 0x007283C0, &PropExtraButes_SaveCopy);

// 0x728430 (thiscall): ProjectileButes' lookups, each once: +0xB4's hash (+0x14C), the
// effects of +0xA4 / +0x54 / +0x58 (+0x14A / +0x150 / +0x152: -2 until looked up, -1
// without), +0xB8's sound (+0x154: its first byte 1 until looked up) and the colour from
// +0x1C..+0x28 (+0x146: while its alpha is 0)
void __fastcall ProjectileButes_Resolve(uint8_t* self, re::Edx) {
    if (At<uint32_t>(self, 0x14C) == 0 && At<const char*>(self, 0xB4)) {
        At<uint32_t>(self, 0x14C) = BD_HashName(At<const char*>(self, 0xB4));
    }
    if (At<int16_t>(self, 0x14A) == -2) {
        At<int16_t>(self, 0x14A) = -1;
        if (At<const char*>(self, 0xA4)) {
            At<uint16_t>(self, 0x14A) = BD_EffectLibrary_Find(bdEffectLibrary, At<const char*>(self, 0xA4));
        }
    }
    if (self[0x154] == 1) {
        At<int32_t>(self, 0x154) = 0;
        if (At<const char*>(self, 0xB8)) {
            At<int32_t>(self, 0x154) = BD_Sounds_GetId(bdSounds, At<const char*>(self, 0xB8));
        }
    }
    if (self[0x149] == 0) {
        uint8_t rgba[4];
        const uint8_t* c = BD_RGBA_Construct(rgba, self[0x1C], self[0x20], self[0x24], self[0x28]);
        self[0x146] = c[0];
        self[0x147] = c[1];
        self[0x148] = c[2];
        self[0x149] = c[3];
    }
    if (At<int16_t>(self, 0x150) == -2) {
        At<int16_t>(self, 0x150) = -1;
        if (At<const char*>(self, 0x54)) {
            At<uint16_t>(self, 0x150) = BD_EffectLibrary_Find(bdEffectLibrary, At<const char*>(self, 0x54));
        }
    }
    if (At<int16_t>(self, 0x152) == -2) {
        At<int16_t>(self, 0x152) = -1;
        if (At<const char*>(self, 0x58)) {
            At<uint16_t>(self, 0x152) = BD_EffectLibrary_Find(bdEffectLibrary, At<const char*>(self, 0x58));
        }
    }
}
BUTESDEF_REVERSED("ProjectileButes::Resolve", 0x00728430, &ProjectileButes_Resolve);

// ---- the defaults (thiscall) -------------------------------------------------------------------------

namespace {

// 0x728570
constexpr ButeOp kFightCamDefaults[] = {
    F(0x18, 0.0f), B(0x28, 1), F(0x1C, -2.5f), B(0x34, 0), B(0x7C, 1), F(0x20, 1.5f), B(0x7D, 1), B(0x7E, 1),
    F(0x24, -0.1f), I(0xA0, 0), F(0x2C, 3.0f), F(0x30, 5.0f), F(0x38, 2.5f), F(0x3C, 2.5f), F(0x40, 1.8f),
    F(0x44, 0.65f), F(0x48, 1.0f), F(0x50, 1.0f), F(0x4C, 3.0f), F(0x54, 30.0f), F(0x58, 1.0f), F(0x5C, 60.0f),
    F(0x60, 1.0f), F(0x64, 1.0f), F(0x68, 1.0f), F(0x6C, 2.0f), F(0x70, 0.6f), F(0x74, 0.6f), F(0x78, 0.27f),
    F(0x80, 1.0f), F(0x84, 0.2f), F(0x88, 0.5f), F(0x90, 0.5f), F(0x8C, 0.0f), F(0x94, 0.05f), F(0x98, 85.0f),
    F(0x9C, 0.2f), F(0xA4, 2.0f), F(0xA8, 5.0f), F(0xAC, 1.0f), F(0xB0, 3.5f), F(0xB4, 4.0f), F(0xB8, 6.0f),
    F(0xC0, 6.0f), F(0xBC, 2.0f), F(0xC4, 10.0f), F(0xC8, 0.75f), F(0xCC, 1.0f),
};
BUTESDEF_REVERSED("FightCamButes::SetDefaults", 0x00728570, &ButeDefaults<kFightCamDefaults>);

// 0x7286E0
constexpr ButeOp kCoasterCamDefaults[] = {
    F(0x1C, 10.6f), I(0x18, 10), I(0x28, 0), F(0x20, 10.7f), I(0x2C, 0), I(0x30, 255), F(0x24, 20.9f),
    B(0x34, 0), I(0x38, 220), F(0x60, 0.8f), B(0x3C, 0), I(0x40, 3), F(0x64, 0.4f), I(0x44, 30), I(0x48, 70),
    F(0x68, 0.6f), I(0x4C, 85), I(0x50, 150), F(0x6C, 0.8f), I(0x54, 50), I(0x58, 105), F(0x70, 0.2f),
    B(0x5C, 1), F(0x74, 0.4f), F(0x78, 0.4f), F(0x7C, 0.3f), F(0x80, 0.3f), F(0x84, 0.3f),
};
BUTESDEF_REVERSED("CoasterCamButes::SetDefaults", 0x007286E0, &ButeDefaults<kCoasterCamDefaults>);

// 0x7287A0
constexpr ButeOp kVehicleCamDefaults[] = {
    B(0x70, 0), F(0x18, 4.7f), F(0x1C, 4.7f), F(0x20, 0.95f), F(0x24, 0.95f), F(0x28, 0.9f), F(0x2C, 2.0f),
    F(0x30, 1.0f), F(0x34, 1.5f), F(0x38, 0.2f), F(0x3C, 0.2f), F(0x40, 0.2f), F(0x44, 0.3f), F(0x48, 0.9f),
    F(0x4C, 0.6f), F(0x50, 0.1f), F(0x54, 1.2f), F(0x58, 5.0f), F(0x5C, 6.0f), F(0x60, 85.0f), F(0x64, 64.0f),
    F(0x68, 0.2f), F(0x6C, 0.4f), F(0x74, 0.15f), F(0x78, -0.98f), F(0x7C, 0.9f), F(0x80, 0.05f),
    F(0x84, 10.0f),
};
BUTESDEF_REVERSED("VehicleCamButes::SetDefaults", 0x007287A0, &ButeDefaults<kVehicleCamDefaults>);

// 0x728880
constexpr ButeOp kFollowCam3Defaults[] = {
    F(0x18, 0.3f), F(0x1C, 0.3f), B(0xBC, 0), B(0xBD, 1), F(0x20, 0.5f), B(0xBE, 0), B(0xBF, 0), F(0x24, 85.0f),
    B(0xC0, 0), B(0xC1, 0), F(0x28, 0.2f), B(0xC2, 1), B(0xC3, 0), F(0x2C, 0.5f), F(0x30, 0.0f), F(0x34, 0.0f),
    F(0x38, 0.25f), F(0x3C, 8.0f), F(0x40, 8.0f), F(0x44, 3.9f), F(0x48, 2.5f), F(0x4C, 4.5f), F(0x50, 1.0f),
    F(0x54, 1.0f), F(0x58, -3.0f), F(0x5C, 4.0f), F(0x60, 1.0f), F(0x64, 0.65f), F(0x68, 0.65f), F(0x6C, 0.65f),
    F(0x70, -0.5f), F(0x74, 2.0f), F(0x78, 0.5f), F(0x7C, 0.1f), F(0x80, 1.5f), F(0x84, 0.5f), F(0x88, 0.5f),
    F(0x90, 0.5f), F(0x8C, 1.0f), F(0x94, -30.0f), F(0x98, 30.0f), F(0x9C, 1.0f), F(0xA0, 2.0f), F(0xA4, 0.5f),
    F(0xA8, 1.5f), F(0xAC, 2.0f), F(0xB0, 0.5f), F(0xB4, 0.3f), F(0xB8, 0.05f), B(0xC4, 0), F(0xC8, 0.5f),
    B(0xEC, 0), F(0xCC, 0.45f), F(0xD0, 720.0f), F(0xD4, 0.003f), F(0xD8, 360.0f), F(0xDC, 0.1f),
    F(0xE0, 0.25f), F(0xE4, 0.25f), F(0xE8, 0.65f), F(0xF0, 1.0f), F(0xF4, 1.0f),
};
BUTESDEF_REVERSED("FollowCam3Butes::SetDefaults", 0x00728880, &ButeDefaults<kFollowCam3Defaults>);

// 0x728A60
constexpr ButeOp kDodgeballCamDefaults[] = {
    F(0x18, 6.0f), F(0x1C, 11.0f), F(0x20, 1.0f), F(0x24, 1.0f), F(0x28, 0.75f), F(0x2C, 0.0f), F(0x30, 2.0f),
    F(0x34, 0.6f), F(0x38, 0.25f), F(0x3C, 104.0f),
};
BUTESDEF_REVERSED("DodgeballCamButes::SetDefaults", 0x00728A60, &ButeDefaults<kDodgeballCamDefaults>);

// 0x728AB0
constexpr ButeOp kDodgeballPhysicsDefaults[] = {
    F(0x18, 0.0015f), F(0x1C, 0.05f), F(0x20, 0.99f), F(0x24, 0.985f), F(0x28, 0.3f), F(0x2C, 0.6f),
    F(0x30, 0.6f), F(0x34, 0.97f), F(0x38, 0.1f), F(0x3C, 0.07f), F(0x40, -10.0f), F(0x44, 75.0f),
    F(0x48, 0.01f), F(0x4C, 0.4f), F(0x50, 0.15f),
};
BUTESDEF_REVERSED("DodgeballPhysicsButes::SetDefaults", 0x00728AB0, &ButeDefaults<kDodgeballPhysicsDefaults>);

// 0x728B40
constexpr ButeOp kDodgeballAIDefaults[] = {
    I(0x2C, 4000), F(0x18, 20.0f), I(0x30, 3000), I(0x60, 50), F(0x1C, 1.0f), I(0x64, 200), F(0x20, 1.0f),
    I(0x68, 30), I(0xAC, 5000), F(0x24, 0.5f), F(0x28, 0.7f), F(0x34, 2.0f), F(0x38, 8.0f), F(0x3C, 6.0f),
    F(0x40, 5.0f), F(0x44, 1.0f), F(0x48, 1.0f), F(0x4C, 1.0f), F(0x50, 1.0f), F(0x54, 1.0f), F(0x58, 0.0f),
    F(0x5C, 0.0f), F(0x6C, 1.0f), F(0x70, 1.0f), F(0x74, 1.0f), F(0x78, 1.0f), F(0x7C, 1.0f), F(0x80, 1.0f),
    F(0x90, 1.0f), F(0x94, 1.0f), F(0x84, 0.0f), F(0x88, 0.0f), F(0x9C, 0.0f), F(0x8C, 0.5f), F(0x98, 2.0f),
    F(0xA0, -0.1f), F(0xA4, 0.1f), F(0xA8, 0.5f), F(0xB0, 0.5f), F(0xB4, 0.5f), F(0xB8, 0.5f),
};
BUTESDEF_REVERSED("DodgeballAIButes::SetDefaults", 0x00728B40, &ButeDefaults<kDodgeballAIDefaults>);

// 0x728C50
constexpr ButeOp kDodgeballAIRampDefaults[] = {
    F(0x18, 1.0f), F(0x1C, 1.0f), F(0x20, 0.0f), F(0x24, 1.0f),
};
BUTESDEF_REVERSED("DodgeballAIRampButes::SetDefaults", 0x00728C50, &ButeDefaults<kDodgeballAIRampDefaults>);

// 0x728C70
constexpr ButeOp kDodgeballGameDefaults[] = {
    F(0x1C, 50.0f), B(0x18, 0), F(0x24, 60.0f), B(0x20, 1), B(0x21, 1), F(0x2C, 0.0f), B(0x22, 1),
    F(0x30, 0.0f), B(0x23, 0), F(0x34, 0.0f), B(0x28, 0), B(0x58, 0), F(0x38, 1.0f), B(0x59, 0), F(0x3C, 1.0f),
    F(0x40, 1.0f), F(0x44, 0.001f), F(0x54, 0.001f), F(0x48, 1.0f), F(0x4C, 1.0f), F(0x50, 1.0f),
};
BUTESDEF_REVERSED("DodgeballGameButes::SetDefaults", 0x00728C70, &ButeDefaults<kDodgeballGameDefaults>);

// 0x728CD0
constexpr ButeOp kAIDefaults[] = {
    F(0x28, 0.5f), I(0x3C, 1), F(0x2C, 1.0f), I(0x44, 1), I(0x48, 1), F(0x30, 2.0f), I(0x6C, 1), F(0x34, 3.5f),
    I(0x18, 2), I(0x1C, 2), F(0x38, 5.0f), I(0x20, 3), I(0x24, 10), F(0x4C, 2.0f), I(0x40, 2), I(0x84, 50),
    F(0x50, 20.0f), I(0x88, 50), I(0x8C, 5), I(0x90, 50), F(0x54, 2.0f), I(0x94, 2), I(0x98, 3),
    F(0x58, 0.866f), F(0x5C, 1.5f), F(0x60, 0.8f), F(0x64, 7.0f), F(0x68, 30.0f), F(0x70, 1.0f), F(0x74, 3.0f),
    F(0x78, 5.0f), F(0x7C, 8.0f), F(0x80, 10.0f), F(0x9C, 30.0f), F(0xA0, 30.0f), F(0xA4, 1.5f),
};
BUTESDEF_REVERSED("AIButes::SetDefaults", 0x00728CD0, &ButeDefaults<kAIDefaults>);

// 0x728DE0
constexpr ButeOp kFightCollisionDefaults[] = {
    F(0x1C, 0.35f), I(0x18, 2), F(0x24, 0.35f), I(0x20, 11), I(0x28, 16), F(0x2C, 0.2f), I(0x30, 2), I(0x38, 4),
    F(0x34, 0.1f), I(0x40, 7), F(0x3C, 0.2f), F(0x44, 0.2f),
};
BUTESDEF_REVERSED("FightCollisionButes::SetDefaults", 0x00728DE0, &ButeDefaults<kFightCollisionDefaults>);

// 0x728E30
constexpr ButeOp kButtonTriggerDefaults[] = {
    I(0x18, 215), I(0x1C, 215), I(0x20, 215), I(0x24, 255),
};
BUTESDEF_REVERSED("ButtonTriggerButes::SetDefaults", 0x00728E30, &ButeDefaults<kButtonTriggerDefaults>);

// 0x728E50
constexpr ButeOp kTimePeriodDefaults[] = {
    S(0x18, 0),
    S(0x1C, 0x0094CE98),  // "F_Start"
    S(0x20, 0x0094CE90),  // "F_End"
    I(0x24, 0), I(0x28, 0), I(0x30, 0), I(0x34, 0), I(0x2C, 24),
};
BUTESDEF_REVERSED("TimePeriodButes::SetDefaults", 0x00728E50, &ButeDefaults<kTimePeriodDefaults>);

// 0x728F00
constexpr ButeOp kTimeCycleDefaults[] = {
    I(0x18, 5),
    S(0x1C, 0x0094CED0),  // "STimeCycle.lua"
    S(0x20, 0x0094CEC0),  // "F_AttendedClass"
    S(0x24, 0x0094CEB0),  // "F_MissedClass"
    S(0x28, 0x0094CEA0),  // "F_MissedCurfew"
};
BUTESDEF_REVERSED("TimeCycleButes::SetDefaults", 0x00728F00, &ButeDefaults<kTimeCycleDefaults>);

// 0x728FD0
constexpr ButeOp kDodgeballArenaDefaults[] = {
    S(0x18, 0x0094CF08),  // "DODGEBALL_CORNER1"
    S(0x1C, 0x0094CEF4),  // "DODGEBALL_CORNER2"
    S(0x20, 0x0094CEE0),  // "DODGEBALL_CENTER"
    F(0x24, 0.2f), F(0x28, 0.2f), F(0x2C, 0.2f), F(0x30, 0.2f),
};
BUTESDEF_REVERSED("DodgeballArenaButes::SetDefaults", 0x00728FD0, &ButeDefaults<kDodgeballArenaDefaults>);

// 0x729080
constexpr ButeOp kDebugPlayerDefaults[] = {
    B(0x18, 0), B(0x19, 0), B(0x1A, 0), B(0x1B, 0), B(0x1C, 0), B(0x1D, 0), B(0x1E, 0), B(0x1F, 0), B(0x20, 0),
    B(0x21, 0), B(0x22, 0), B(0x23, 0), B(0x24, 0), B(0x25, 0), B(0x26, 0), B(0x27, 0), B(0x28, 0), B(0x29, 0),
    B(0x2A, 1), B(0x2B, 0), B(0x2C, 1),
};
BUTESDEF_REVERSED("DebugPlayerButes::SetDefaults", 0x00729080, &ButeDefaults<kDebugPlayerDefaults>);

// 0x7290D0
constexpr ButeOp kPlayerDefaults[] = {
    F(0x20, 1.2f), F(0x24, 0.4f), I(0x18, 1000), I(0x1C, 175), F(0x2C, 160.0f), I(0x28, 333), F(0x30, 160.0f),
    I(0x68, 1000), I(0x6C, 85), F(0x34, 1.0f), I(0x70, 200), F(0x38, 4.1f), F(0x3C, 1.0f), F(0x40, 0.1f),
    F(0x44, 0.75f), F(0x48, 2.0f), F(0x4C, 0.5f), F(0x50, 0.05f), F(0x54, 1.0f), F(0x58, 5.0f), F(0x5C, 0.1f),
    F(0x60, 20.0f), F(0x64, 0.15f), F(0x74, 4.0f), F(0x78, 3.0f), F(0x7C, 16.0f), F(0x80, 9.0f),
    F(0x84, 102.0f), F(0x88, 8.0f), F(0x8C, 3.0f), F(0x90, 0.2f), F(0x94, 0.1f), F(0x98, 0.001f),
    F(0x9C, 0.01f), F(0xA0, 0.05f), F(0xA4, 0.2f), F(0xA8, 1.0f), F(0xAC, 1.0f), F(0xB0, 0.75f), F(0xB4, 0.25f),
    F(0xB8, 8.0f), F(0xBC, 5.0f), F(0xC0, 60.0f), F(0xC4, 50.0f), F(0xD4, -0.5f), I(0xC8, 180), I(0xCC, 180),
    F(0xD8, 20.0f), I(0xD0, 60), F(0xDC, -102.5f), F(0xE0, 102.5f), F(0xE4, -40.0f), F(0xE8, 40.0f),
    F(0xEC, -0.3f), F(0xF0, 4.0f), F(0xF4, 7.0f), F(0xF8, -1.75f),
    S(0xFC, 0x0094CF1C),  // "meleetarget"
    F(0x100, 2.0f), F(0x104, 0.5f), F(0x108, 2.0f), N(0x10C, 0), F(0x110, 0.1f), N(0x114, 0), F(0x118, 0.1f),
    N(0x11C, 0), F(0x120, 0.1f),
};
BUTESDEF_REVERSED("PlayerButes::SetDefaults", 0x007290D0, &ButeDefaults<kPlayerDefaults>);

// 0x729380
constexpr ButeOp kFrisbeeDefaults[] = {
    F(0x18, 1.0f), F(0x1C, 15.0f), F(0x20, 0.02f), F(0x24, 0.99f), F(0x28, 0.9f), F(0x2C, -9.8f), F(0x30, 0.2f),
    F(0x34, 3.0f), F(0x38, 0.05f), F(0x3C, 3.0f),
};
BUTESDEF_REVERSED("FrisbeeButes::SetDefaults", 0x00729380, &ButeDefaults<kFrisbeeDefaults>);

// 0x7293E0
constexpr ButeOp kFirstPersonCamDefaults[] = {
    F(0x18, 0.9f), F(0x1C, 0.9f), F(0x20, 35.0f), F(0x24, 160.0f), F(0x28, 0.45f), F(0x2C, 1.64f),
    F(0x30, 0.4f), F(0x34, 0.25f), F(0x38, 2.0f),
};
BUTESDEF_REVERSED("FirstPersonCamButes::SetDefaults", 0x007293E0, &ButeDefaults<kFirstPersonCamDefaults>);

// 0x729430
constexpr ButeOp kOffsetCamDefaults[] = {
    F(0x18, 0.9f), F(0x1C, 0.9f), F(0x20, 35.0f), F(0x24, 160.0f), F(0x28, 0.45f), F(0x2C, 0.0f), F(0x30, 0.0f),
    F(0x34, 0.0f), F(0x38, 0.0f), F(0x3C, 1.64f), F(0x40, -0.5f), F(0x44, -0.8f), F(0x48, 0.8f), F(0x4C, 0.25f),
    F(0x50, -0.9f), F(0x54, 1.6f), S(0x58, 0), S(0x5C, 0), B(0x60, 0), S(0x64, 0), F(0x6C, 85.0f), B(0x68, 0),
    B(0x70, 0), F(0x74, 1.0f), B(0x78, 0), B(0x79, 1), B(0x7A, 0), B(0x7B, 0), I(0x7C, -1), S(0x80, 0),
    F(0x84, 3.0f), F(0x88, 0.0f), F(0x8C, 5.0f), F(0x90, -1.0f), F(0x94, 0.004f), F(0x98, 0.4f),
};
BUTESDEF_REVERSED("OffsetCamButes::SetDefaults", 0x00729430, &ButeDefaults<kOffsetCamDefaults>);

// 0x7295D0
constexpr ButeOp kCrosshairDefaults[] = {
    S(0x18, 0), F(0x1C, 1.0f), I(0x20, 255), I(0x24, 255), I(0x28, 255), I(0x2C, 255), I(0x30, 255),
    I(0x34, 255), I(0x38, 255), I(0x3C, 255), I(0x40, -1),
};
BUTESDEF_REVERSED("CrosshairButes::SetDefaults", 0x007295D0, &ButeDefaults<kCrosshairDefaults>);

// 0x729630
constexpr ButeOp kActionMapDefaults[] = {
    I(0x18, 0), I(0x1C, 0), I(0x20, 0), I(0x24, 0), I(0x28, 0), I(0x2C, 0), I(0x30, 0), I(0x34, 0), I(0x38, 0),
    I(0x3C, 0), I(0x40, 0), I(0x44, 0), I(0x48, 0), I(0x4C, 0), I(0x50, 0), I(0x54, 0), I(0x58, 0), I(0x5C, 0),
    I(0x60, 0), I(0x64, 0),
};
BUTESDEF_REVERSED("ActionMapButes::SetDefaults", 0x00729630, &ButeDefaults<kActionMapDefaults>);

// 0x729670
constexpr ButeOp kPickupDefaults[] = {
    F(0x1C, 30.0f), F(0x24, 30.0f), B(0x18, 0), B(0x20, 1), I(0x28, 1), B(0x2C, 0), B(0x2D, 1),
};
BUTESDEF_REVERSED("PickupButes::SetDefaults", 0x00729670, &ButeDefaults<kPickupDefaults>);

// 0x7296A0
constexpr ButeOp kObjectDefaults[] = {
    I(0x18, 0), S(0x1C, 0), F(0x2C, 1.0f), F(0x30, 0.5f), I(0x20, 2), B(0x24, 1), F(0x44, 0.1f), B(0x25, 0),
    B(0x26, 1), B(0x27, 0), B(0x28, 0), B(0x29, 1), B(0x2A, 0), B(0x2B, 0), I(0x34, 10), I(0x38, 0), I(0x3C, 0),
    I(0x40, 0), S(0x48, 0), S(0x4C, 0), S(0x50, 0), S(0x54, 0), S(0x58, 0), S(0x5C, 0), F(0x6C, 1.0f),
    F(0x70, 0.0f), I(0x60, 0), I(0x64, 0), B(0x68, 0), B(0x69, 1), B(0x6A, 0), S(0x74, 0), S(0x78, 0),
    I(0x7C, 1), I(0x80, 1), I(0x84, -2), I(0x88, 0), S(0x8C, 0), N(0x90, 0), N(0x94, 0), I(0x98, 0), N(0x9C, 0),
    N(0xA0, 0),
};
BUTESDEF_REVERSED("ObjectButes::SetDefaults", 0x007296A0, &ButeDefaults<kObjectDefaults>);

// 0x729910
constexpr ButeOp kCinematicDefaults[] = {
    F(0x28, 0.8f), I(0x18, 70), I(0x1C, 350), F(0x2C, 1.0f), I(0x20, 610), I(0x24, 443), F(0x40, 0.2f),
    I(0x30, 225), F(0x48, 0.2f), I(0x34, 225), I(0x38, 225), I(0x3C, 90), F(0x44, 1.0f), F(0x4C, 0.5f),
    F(0x50, 1.0f), F(0x54, 1.0f), F(0x58, 5.0f),
};
BUTESDEF_REVERSED("CinematicButes::SetDefaults", 0x00729910, &ButeDefaults<kCinematicDefaults>);

// 0x729980
constexpr ButeOp kMenuDefaults[] = {
    F(0x78, 0.7f), I(0x18, 447), F(0x7C, 0.9f), I(0x1C, 300), I(0x20, 80), I(0x24, 620), I(0x28, 255),
    I(0x2C, 255), I(0x30, 255), I(0x34, 255), I(0x38, 0), I(0x3C, 0), I(0x40, 0), I(0x44, 255), I(0x48, 255),
    I(0x4C, 255), I(0x50, 255), I(0x54, 90), I(0x58, 40), I(0x5C, 40), I(0x60, 40), I(0x64, 200), I(0x68, 255),
    I(0x6C, 255), I(0x70, 255), I(0x74, 255), I(0x80, 0), I(0x84, 17), B(0x88, 0), B(0x89, 0), B(0x8A, 1),
};
BUTESDEF_REVERSED("MenuButes::SetDefaults", 0x00729980, &ButeDefaults<kMenuDefaults>);

// 0x729A30
constexpr ButeOp kCinematicShotDefaults[] = {
    F(0x18, 0.0f), F(0x1C, 1.0f), F(0x20, 1.0f), F(0x28, 1.0f), F(0x24, 0.0f), F(0x2C, 0.0f), F(0x30, 0.0f),
    F(0x34, 0.0f), F(0x38, 30.0f), F(0x3C, 0.0f), F(0x40, 85.0f), F(0x44, 0.0f), F(0x48, 0.0f), F(0x4C, 0.0f),
    F(0x50, 0.0f),
};
BUTESDEF_REVERSED("CinematicShotButes::SetDefaults", 0x00729A30, &ButeDefaults<kCinematicShotDefaults>);

// 0x729A70
constexpr ButeOp kCinematicShotGroupDefaults[] = {
    S(0x18, 0), S(0x1C, 0), S(0x20, 0), S(0x24, 0), S(0x28, 0), S(0x2C, 0), S(0x30, 0), S(0x34, 0), S(0x38, 0),
    S(0x3C, 0),
};
BUTESDEF_REVERSED("CinematicShotGroupButes::SetDefaults", 0x00729A70, &ButeDefaults<kCinematicShotGroupDefaults>);

// 0x729C40
constexpr ButeOp kStimulusDefaults[] = {
    F(0x20, 0.0f), I(0x18, 87), F(0x24, 0.0f), I(0x1C, 0), F(0x28, 0.0f), B(0x34, 0), F(0x2C, 0.0f), B(0x35, 0),
    F(0x30, 0.0f), B(0x36, 0),
};
BUTESDEF_REVERSED("StimulusButes::SetDefaults", 0x00729C40, &ButeDefaults<kStimulusDefaults>);

// 0x729C70
constexpr ButeOp kPunishmentDefaults[] = {
    I(0x18, 87), I(0x1C, 0), B(0x20, 0), B(0x21, 0), I(0x24, 7), I(0x28, 0), S(0x2C, 0),
};
BUTESDEF_REVERSED("PunishmentButes::SetDefaults", 0x00729C70, &ButeDefaults<kPunishmentDefaults>);

// 0x729CC0
constexpr ButeOp kPropDefaults[] = {
    F(0x28, 4.0f), I(0x18, 0), I(0x1C, 1), I(0x20, 1), B(0x24, 1), B(0x2C, 0), N(0x30, 0), N(0x34, 0),
    I(0x38, 0), N(0x3C, 0), N(0x40, 0), I(0x44, 0), N(0x48, 0), N(0x4C, 0), I(0x50, 0), S(0x54, 0), N(0x58, 0),
    N(0x5C, 0), N(0x60, 0), B(0x64, 0), S(0x68, 0), S(0x6C, 0), S(0x70, 0), F(0x78, 2.0f), B(0x74, 1),
    B(0x75, 1), B(0x76, 1), B(0x77, 1), B(0x7C, 0), B(0x7D, 1), B(0x7E, 1), N(0x80, 0), B(0x84, 0), B(0x85, 0),
    B(0x86, 0), B(0x8C, 0), B(0x8D, 0), I(0x88, -1), B(0x8E, 1),
};
BUTESDEF_REVERSED("PropButes::SetDefaults", 0x00729CC0, &ButeDefaults<kPropDefaults>);

// 0x729E50
constexpr ButeOp kPropExtraDefaults[] = {
    I(0x18, 1), I(0x1C, 1), I(0x20, 1), I(0x24, 1), I(0x28, 1), I(0x2C, 1), I(0x30, 1), I(0x34, 1), I(0x38, 1),
    I(0x3C, 1), I(0x40, 0),
};
BUTESDEF_REVERSED("PropExtraButes::SetDefaults", 0x00729E50, &ButeDefaults<kPropExtraDefaults>);

// 0x729E80
constexpr ButeOp kPickupListDefaults[] = {
    S(0x18, 0), F(0x1C, 1.0f), S(0x20, 0), F(0x24, 0.0f), S(0x28, 0), F(0x2C, 0.0f), S(0x30, 0), F(0x34, 0.0f),
    S(0x38, 0), F(0x3C, 0.0f), S(0x40, 0), F(0x44, 0.0f), S(0x48, 0), F(0x4C, 0.0f),
};
BUTESDEF_REVERSED("PickupListButes::SetDefaults", 0x00729E80, &ButeDefaults<kPickupListDefaults>);

// 0x729FE0
constexpr ButeOp kBikeDefaults[] = {
    F(0x18, 0.2f), F(0x1C, 0.6f), F(0x20, 1000.0f), F(0x24, 0.01f), F(0x28, 0.7f), F(0x2C, 0.2f), F(0x30, 4.0f),
    F(0x34, 45.0f), F(0x38, 0.2f), F(0x3C, 0.2f), F(0x40, 3.0f), F(0x44, 0.2f), F(0x48, 0.08f), F(0x4C, 0.1f),
    F(0x50, 0.2f), F(0x54, 0.3f), F(0x58, 0.04f), F(0x5C, 1.15f), F(0x60, 0.5f), F(0x64, 0.5f), F(0x68, 0.08f),
    F(0x70, 0.08f), F(0x6C, 0.01f), F(0x74, 0.01f), F(0x78, 0.18f), F(0x7C, 0.0f), F(0x80, 0.0f),
    F(0x84, 250.0f), F(0x88, 72.0f),
};
BUTESDEF_REVERSED("BikeButes::SetDefaults", 0x00729FE0, &ButeDefaults<kBikeDefaults>);

// 0x72A0B0
constexpr ButeOp kPhotoDefaults[] = {
    I(0x18, 100), I(0x1C, 100), I(0x20, 100), I(0x30, 370), I(0x40, 370), I(0x2C, 420), I(0x34, 420),
    I(0x5C, 50), I(0x60, 50), I(0x24, 540), I(0x28, 348), I(0x38, 400), I(0x3C, 320), I(0x44, 270),
    I(0x48, 260), I(0x4C, 128), I(0x50, 22), I(0x54, 6), I(0x58, 200), I(0x64, 200), I(0x68, 180), I(0x6C, 180),
    I(0x70, 180), I(0x74, 200), I(0x78, 180), I(0x7C, 180), I(0x80, 180), I(0x84, 200),
};
BUTESDEF_REVERSED("PhotoButes::SetDefaults", 0x0072A0B0, &ButeDefaults<kPhotoDefaults>);

// 0x72A150
constexpr ButeOp kTaggingDefaults[] = {
    F(0x18, 0.7f), F(0x1C, 0.35f), F(0x20, 1.5f), F(0x24, 0.75f), F(0x28, 4.0f), F(0x2C, 2.0f), F(0x30, 10.0f),
    F(0x34, 0.333f), F(0x38, 60.0f), F(0x3C, 3.0f),
};
BUTESDEF_REVERSED("TaggingButes::SetDefaults", 0x0072A150, &ButeDefaults<kTaggingDefaults>);

// 0x72A1B0
constexpr ButeOp kSurfaceSoundRangeDefaults[] = {
    F(0x18, 2.0f), F(0x1C, 2.0f), F(0x20, 2.0f), F(0x24, 1.0f), F(0x28, 3.0f), F(0x54, 3.0f), F(0x2C, 2.0f),
    F(0x30, 2.0f), F(0x34, 2.0f), F(0x38, 2.0f), F(0x3C, 2.0f), F(0x40, 2.0f), F(0x44, 2.0f), F(0x48, 2.0f),
    F(0x4C, 2.0f), F(0x50, 2.0f), F(0x5C, 2.0f), F(0x60, 2.0f), F(0x64, 2.0f), F(0x68, 2.0f), F(0x6C, 2.0f),
    F(0x58, 1.0f), F(0x70, 4.0f), F(0x74, 2.0f), F(0x78, 2.0f), F(0x84, 2.0f), F(0x88, 2.0f), F(0x8C, 2.0f),
    F(0x9C, 2.0f), F(0xA0, 2.0f), F(0xB0, 2.0f), F(0xB4, 2.0f), F(0xB8, 2.0f), F(0xBC, 2.0f), F(0xC0, 2.0f),
    F(0xC4, 2.0f), F(0xC8, 2.0f), F(0x7C, 3.0f), F(0x80, 3.0f), F(0x90, 3.0f), F(0x94, 3.0f), F(0x98, 3.0f),
    F(0xA4, 3.0f), F(0xA8, 3.0f), F(0xAC, 3.0f), F(0xCC, 2.5f), F(0xD0, 2.0f), F(0xD4, 2.0f), F(0xD8, 2.0f),
    F(0xDC, 2.0f), F(0xE0, 2.0f), F(0xE4, 2.0f), F(0xE8, 2.0f), F(0xEC, 2.0f), F(0xF0, 2.0f), F(0xF4, 2.0f),
    F(0xF8, 4.0f), F(0xFC, 1.0f), F(0x100, 1.5f), F(0x104, 3.0f), F(0x10C, 3.0f), F(0x108, 2.0f),
    F(0x110, 1.0f), F(0x114, 1.0f), F(0x118, 1.0f), F(0x11C, 1.0f),
};
BUTESDEF_REVERSED("SurfaceSoundRangeButes::SetDefaults", 0x0072A1B0, &ButeDefaults<kSurfaceSoundRangeDefaults>);

// 0x72A320
constexpr ButeOp kAreaTransitionDefaults[] = {
    F(0x18, 1.0f), F(0x1C, 1.5f), F(0x20, 200.0f), F(0x24, 0.35f),
};
BUTESDEF_REVERSED("AreaTransitionButes::SetDefaults", 0x0072A320, &ButeDefaults<kAreaTransitionDefaults>);

// 0x72A350
constexpr ButeOp kTrafficDefaults[] = {
    I(0x18, 2), F(0x20, 8.0f), I(0x1C, 5000), F(0x24, 0.17f),
};
BUTESDEF_REVERSED("TrafficButes::SetDefaults", 0x0072A350, &ButeDefaults<kTrafficDefaults>);

// 0x72A380
constexpr ButeOp kPlayerAwarenessDefaults[] = {
    I(0x24, 15), F(0x18, 0.3f), F(0x1C, 3.0f), F(0x20, 5.0f), F(0x28, 0.2f), F(0x2C, 0.4f), F(0x30, 0.4f),
    F(0x34, 1.0f),
};
BUTESDEF_REVERSED("PlayerAwarenessButes::SetDefaults", 0x0072A380, &ButeDefaults<kPlayerAwarenessDefaults>);

// 0x72A3C0
constexpr ButeOp kStoreCamDefaults[] = {
    F(0x1C, 0.005f), B(0x18, 1), B(0x28, 1), F(0x20, 0.0f), I(0x2C, 0), F(0x24, 0.08f), F(0x30, 1.0f),
};
BUTESDEF_REVERSED("StoreCamButes::SetDefaults", 0x0072A3C0, &ButeDefaults<kStoreCamDefaults>);

// 0x72A3F0
constexpr ButeOp kSoccerPDefaults[] = {
    F(0x18, 2.0f), F(0x1C, 2.0f), F(0x20, -1.0f), F(0x24, 1.0f), F(0x2C, 1.0f), F(0x28, -1.0f), F(0x30, 0.0f),
    F(0x34, 0.0f), F(0x38, 100.0f), F(0x3C, 15.0f), F(0x40, 1.0f), F(0x44, 300.0f), F(0x4C, 300.0f),
    F(0x48, 100.0f), F(0x50, 150.0f), F(0x54, 300.0f), F(0x58, 250.0f),
};
BUTESDEF_REVERSED("SoccerPButes::SetDefaults", 0x0072A3F0, &ButeDefaults<kSoccerPDefaults>);

// 0x72A460
constexpr ButeOp kRadarDefaults[] = {
    I(0x24, 5), F(0x18, 3000.0f), F(0x1C, 3000.0f), F(0x20, 0.5f), F(0x28, 10.0f), F(0x2C, 0.1f),
};
BUTESDEF_REVERSED("RadarButes::SetDefaults", 0x0072A460, &ButeDefaults<kRadarDefaults>);

// 0x72A490
constexpr ButeOp kArmWrestlingDefaults[] = {
    F(0x18, 30.0f), F(0x1C, 0.1f), F(0x20, 30.0f), F(0x24, 60.0f),
};
BUTESDEF_REVERSED("ArmWrestlingButes::SetDefaults", 0x0072A490, &ButeDefaults<kArmWrestlingDefaults>);

// 0x72A4B0
constexpr ButeOp kMiscGameDefaults[] = {
    F(0x18, 1.0f),
};
BUTESDEF_REVERSED("MiscGameButes::SetDefaults", 0x0072A4B0, &ButeDefaults<kMiscGameDefaults>);

// 0x72A4C0
constexpr ButeOp kArcadeRaceCameraDefaults[] = {
    F(0x18, 0.9f), F(0x1C, 60.0f), F(0x20, 0.0f),
};
BUTESDEF_REVERSED("ArcadeRaceCameraButes::SetDefaults", 0x0072A4C0, &ButeDefaults<kArcadeRaceCameraDefaults>);

// 0x72A4E0
constexpr ButeOp kGlobalSpineControlDefaults[] = {
    F(0x18, -10.0f), F(0x1C, 10.0f), F(0x20, -5.0f), F(0x24, 5.0f), F(0x28, -15.0f), F(0x2C, 15.0f),
    F(0x30, -10.0f), F(0x34, 10.0f), F(0x38, -20.0f), F(0x3C, 20.0f), F(0x4C, 20.0f), F(0x5C, 20.0f),
    F(0x40, -15.0f), F(0x50, -15.0f), F(0x60, -15.0f), F(0x44, 15.0f), F(0x54, 15.0f), F(0x64, 15.0f),
    F(0x48, -20.0f), F(0x58, -20.0f), F(0x68, 30.0f), F(0x6C, 40.0f), F(0x70, 2.0f), F(0x74, 15.0f),
    F(0x78, 150.0f),
};
BUTESDEF_REVERSED("GlobalSpineControlButes::SetDefaults", 0x0072A4E0, &ButeDefaults<kGlobalSpineControlDefaults>);

// 0x72A580
constexpr ButeOp kSpineControlWeightDefaults[] = {
    I(0x3C, 14), F(0x18, 500.0f), F(0x1C, 200.0f), F(0x20, 0.3f), F(0x24, 0.3f), F(0x28, 0.4f), F(0x2C, 0.4f),
    F(0x30, 0.4f), F(0x34, 0.5f), F(0x38, 0.5f), F(0x40, 120.0f), F(0x44, -120.0f),
};
BUTESDEF_REVERSED("SpineControlWeightButes::SetDefaults", 0x0072A580, &ButeDefaults<kSpineControlWeightDefaults>);

} // namespace

// ---- the destructors (thiscall) ----------------------------------------------------------------------

BUTESDEF_REVERSED("CoasterCamButes::Destruct", 0x0072A5E0, &ButesDestruct<0x00934480>);
BUTESDEF_REVERSED("VehicleCamButes::Destruct", 0x0072A5F0, &ButesDestruct<0x00914740>);
BUTESDEF_REVERSED("FollowCam3Butes::Destruct", 0x0072A600, &ButesDestruct<0x00914580>);
BUTESDEF_REVERSED("DodgeballCamButes::Destruct", 0x0072A610, &ButesDestruct<0x00934498>);
BUTESDEF_REVERSED("DodgeballPhysicsButes::Destruct", 0x0072A620, &ButesDestruct<0x009344B0>);
BUTESDEF_REVERSED("DodgeballAIButes::Destruct", 0x0072A630, &ButesDestruct<0x009344C8>);
BUTESDEF_REVERSED("DodgeballAIRampButes::Destruct", 0x0072A640, &ButesDestruct<0x009344E0>);
BUTESDEF_REVERSED("DodgeballGameButes::Destruct", 0x0072A650, &ButesDestruct<0x009344F8>);
BUTESDEF_REVERSED("AIButes::Destruct", 0x0072A660, &ButesDestruct<0x00934510>);
BUTESDEF_REVERSED("FightCollisionButes::Destruct", 0x0072A670, &ButesDestruct<0x00934528>);
BUTESDEF_REVERSED("ButtonTriggerButes::Destruct", 0x0072A680, &ButesDestruct<0x00934540>);
BUTESDEF_REVERSED("TimePeriodButes::Destruct", 0x0072A690, &ButesDestruct<0x00934558, 0x18, 0x1C, 0x20>);
BUTESDEF_REVERSED("TimeCycleButes::Destruct", 0x0072A6E0, &ButesDestruct<0x00934570, 0x1C, 0x20, 0x24, 0x28>);
BUTESDEF_REVERSED("DodgeballArenaButes::Destruct", 0x0072A750, &ButesDestruct<0x00934588, 0x18, 0x1C, 0x20>);
BUTESDEF_REVERSED("DebugPlayerButes::Destruct", 0x0072A7A0, &ButesDestruct<0x009345A0>);
BUTESDEF_REVERSED("PlayerButes::Destruct", 0x0072A7B0, &ButesDestruct<0x009345B8, 0xFC, kNameKey | 0x10C, kNameKey | 0x114, kNameKey | 0x11C>);
