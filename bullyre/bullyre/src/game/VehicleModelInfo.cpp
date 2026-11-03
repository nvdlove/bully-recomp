// CVehicleModelInfo (as CVehicleModel, its full layout: Vehicle.h): names, colours,
// extras, instances. And the three CWeaponModelInfo functions among them.

#include <cstdint>
#include <cstring>

#include "game/ModelInfo.h"
#include "game/Vehicle.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

using veh::CVector;

// ours, called where they are
GAME_FN(int, __cdecl, CountColourSlots, 0x00534E10, int bits);
GAME_FN(int, __cdecl, PickColour, 0x00534E40, int kind, int bits);
GAME_FN(int, __cdecl, GetAllowedColours, 0x00534D10, uint32_t flags, int count, int* out);
GAME_FN(int32_t, __thiscall, VehicleModel_ChooseColour1, 0x00534ED0, CVehicleModel* mi);
GAME_FN(int32_t, __thiscall, VehicleModel_ChooseColour2, 0x00534F90, CVehicleModel* mi);
GAME_FN(void, __thiscall, VehicleModel_SetUpExtras, 0x005359F0, CVehicleModel* mi);
GAME_FN(void, __thiscall, VehicleModel_5355D0, 0x005355D0, CVehicleModel* mi);

// CBaseModelInfo / CClumpModelInfo
GAME_FN(void, __thiscall, BaseModel_SetName, 0x0050ED40, void* mi, const char* name);
GAME_FN(void, __thiscall, ClumpModel_Construct, 0x005109E0, void* mi, uint8_t type);
GAME_FN(void, __thiscall, ClumpModel_Destruct, 0x00510D90, void* mi);
GAME_FN(void, __thiscall, ClumpModel_DeleteObjects, 0x00510C40, void* mi);
GAME_FN(void*, __thiscall, ClumpModel_CreateInstance, 0x00510D60, void* mi);
GAME_FN(void, __thiscall, ClumpModel_SetClump, 0x00510CB0, void* mi, void* clump, int arg);
GAME_FN(void, __thiscall, ClumpModel_SetFrameIds, 0x00510B70, void* mi, const void* list);
GAME_FN(void, __thiscall, ClumpModel_SetAtomicRendererCallbacks, 0x00510BF0, void* mi, void* node);
GAME_FN(void*, __cdecl, Clump_GetFrameById, 0x00510B00, void* clump, int id);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

// Gamebryo
GAME_FN(void*, __cdecl, Node_GetRoot, 0x006C7370, void* object);
GAME_FN(void, __cdecl, Node_Destroy, 0x006C72C0, void* object);
GAME_FN(void, __cdecl, Node_Release, 0x006CA260, void* node);
GAME_FN(void*, __cdecl, Node_Clone, 0x006C70D0, void* object);
GAME_FN(void*, __cdecl, Node_New, 0x006CA210);
GAME_FN(uint8_t*, __cdecl, Node_GetTransform, 0x006C9A90, void* node);
GAME_FN(void, __cdecl, Node_SetTransform, 0x006CA0F0, void* node, void* transform, int mode);
GAME_FN(void, __cdecl, Node_AttachChild, 0x006C6A10, void* parent, void* child);
GAME_FN(void, __cdecl, Clump_AddObject, 0x006C74A0, void* clump, void* object);
GAME_FN(void, __cdecl, Node_6C9D80, 0x006C9D80, void* root, void* node);
GAME_FN(void, __cdecl, Clump_ForAllObjects, 0x006C7380, void* clump, void* callback, void* data);

// animation groups
inline void* const animManager = reinterpret_cast<void*>(0x020C4910);
GAME_FN(int, __thiscall, AnimManager_LoadGroup, 0x006C2AE0, void* manager, const char* path, void* groups, uintptr_t);
GAME_FN(void*, __thiscall, AnimGroups_FindByHash, 0x00465580, void* groups, uint32_t hash, int32_t* index);   // (the entry, nullptr: none)
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_VAR(const char*, defaultVehicleAnimGroup, 0x00A959BC);
GAME_FN(int, __cdecl, FindAgrId, 0x0052DAD0, const char* name);

inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int min, int max);   // min + rand * (max - min - 1) / 32767
GAME_FN(CVehicle*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);

GAME_VAR(int16_t, newWeatherType, 0x00CE756C);   // CWeather::NewWeatherType (2: the colour rules apply)
GAME_VAR(int8_t, forcedColour1, 0x00A95810);     // -2: none (used once)
GAME_VAR(int8_t, forcedColour2, 0x00A95811);
GAME_VAR(float, unknownA95814, 0x00A95814);
GAME_VAR(uint8_t, lastExtra1, 0x00CE6D24);       // what CreateInstance picked (CVehicle::SetModelIndex reads them)
GAME_VAR(uint8_t, lastExtra2, 0x00CE6D25);
inline const int32_t* const frameTable = reinterpret_cast<const int32_t*>(0x00A95AD0);
inline const re::Ptr32<const void>* const frameLists = reinterpret_cast<const re::Ptr32<const void>*>(0x00A95AC8);   // by vehicle class
inline void* const vehicleModelVtable = reinterpret_cast<void*>(0x00918C94);

// the extra: its copy under a new node at the extra's place, added to the instance
void AddExtra(void* instance, void* root, void* extra) {
    void* clone = Node_Clone(extra);
    void* node = Node_New();
    // (the game pushes the 0 before Node_GetRoot's argument: it's the transform's mode, replace)
    Node_SetTransform(node, Node_GetTransform(Node_GetRoot(extra)), 0);
    Node_AttachChild(clone, node);
    Clump_AddObject(instance, clone);
    Node_6C9D80(root, node);
}

} // namespace

// ---- free functions -------------------------------------------------------------------

// 0x534E10 (cdecl): the nibbles (low first, up to the highest set) that aren't 0xF
static int __cdecl CountColourSlotsImpl(int bits) {
    int count = 0;
    while (bits != 0) {
        if ((bits & 0xF) != 0xF) {
            count++;
        }
        bits >>= 4;   // (arithmetic: a negative value never gets to 0)
    }
    return count;
}

// 0x534E40 (cdecl): a colour from a weather rule's nibbles (kind 1, 2: any slot; 3: any,
// -1 if none; 4: 0..4)
static int __cdecl PickColourImpl(int kind, int bits) {
    switch (kind) {
    case 1:
    case 2: {
        int slot = RandomInt(0, CountColourSlots(bits));
        return (bits >> ((slot * 4) & 31)) & 0xF;
    }
    case 3: {
        int slot = RandomInt(-1, CountColourSlots(bits));
        if (slot == -1) {
            return -1;
        }
        return (bits >> ((slot * 4) & 31)) & 0xF;
    }
    case 4:
        return RandomInt(0, 5);
    default:
        return -1;
    }
}

// 0x534D10 (cdecl): the colour slots 0..count-1 the weather rules allow, into out; how
// many. (In rain, weather type 2, the rules' nibbles name slots taken out. A nibble
// past 5 writes past the game's 6-slot array: 16 here, for the same result otherwise.)
static int __cdecl GetAllowedColoursImpl(uint32_t flags, int count, int* out) {
    int slots[16] = {0, 1, 2, 3, 4, 5};
    auto takeOut = [&](uint32_t nibble) {
        if (nibble != 0xF) {
            slots[nibble] = 0xF;
        }
    };
    if (((flags >> 12) & 0xF) == 2 && newWeatherType == 2) {
        takeOut(flags & 0xF);
        takeOut((flags >> 4) & 0xF);
        takeOut((flags >> 8) & 0xF);
    }
    if ((flags >> 28) == 2 && newWeatherType == 2) {
        takeOut((flags >> 16) & 0xF);
        takeOut((flags >> 20) & 0xF);
        takeOut((flags >> 24) & 0xF);
    }
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (slots[i] != 0xF) {
            *out++ = i;
            found++;
        }
    }
    return found;
}

// 0x5359D0 (cdecl): the model's vehicle model info, nullptr if it isn't one
static CVehicleModel* __cdecl GetVehicleModelInfo(int model) {
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[model];
    if (!mi || mi->type != MODEL_INFO_VEHICLE) {
        return nullptr;
    }
    return reinterpret_cast<CVehicleModel*>(mi);
}

// ---- CVehicleModel --------------------------------------------------------------------

CVehicleModel* CVehicleModel::Construct() {
    ClumpModel_Construct(this, MODEL_INFO_VEHICLE);
    vtable = reinterpret_cast<re::Ptr32<void>*>(vehicleModelVtable);
    for (CVector& pos : dummyPos) {
        pos = {0.0f, 0.0f, 0.0f};
    }
    numColours = 0;
    animFile2 = -1;
    return this;
}

CVector* CVehicleModel::GetFrontSeatPos2() {
    return &dummyPos[4];
}

void CVehicleModel::SetAnimFile2(const char* name) {
    if (_stricmp(name, "null") != 0) {
        animFile2 = FindAgrId(name);
    }
}

void CVehicleModel::GetFramePosition(int frame, CVector* out) {
    uint8_t* transform = Node_GetTransform(Clump_GetFrameById(clump, frameTable[frame]));
    *out = *reinterpret_cast<CVector*>(transform + 0x24);
}

int32_t CVehicleModel::ChooseColour1() {
    if (forcedColour1 != -2) {
        int32_t colour = forcedColour1;
        forcedColour1 = -2;
        return colour;
    }
    if (((colourFlags >> 12) & 0xF) == 2 && newWeatherType == 2) {
        return PickColour((colourFlags >> 12) & 0xF, colourFlags & 0xFFF);
    }
    int32_t colour = -1;
    if (Rand() * 2 / 32767 < 2) {
        int allowed[6];
        int count = GetAllowedColours(colourFlags, numExtras, allowed);
        if (count) {
            colour = allowed[RandomInt(0, count)];
        }
    }
    return colour;
}

int32_t CVehicleModel::ChooseColour2() {
    if (forcedColour2 != -2) {
        int32_t colour = forcedColour2;
        forcedColour2 = -2;
        return colour;
    }
    if ((colourFlags >> 28) == 2 && newWeatherType == 2) {
        return PickColour(colourFlags >> 28, (colourFlags >> 16) & 0xFFF);
    }
    int32_t colour = -1;
    if (((colourFlags >> 12) & 0xF) == 2 && newWeatherType == 2) {
        if (RandomInt(0, 3) < 2) {
            int allowed[6];
            int count = GetAllowedColours(colourFlags, numExtras, allowed);
            if (count) {
                colour = allowed[RandomInt(0, count)];
            }
        }
    }
    return colour;
}

void CVehicleModel::AvoidLastColours(uint8_t* colour1, uint8_t* colour2) {
    if (numColours > 1) {
        for (int tries = 0; tries < 8; tries++) {
            if (*colour1 != lastColour1 || *colour2 != lastColour2) {
                break;
            }
            int index = Rand() * (numColours - 1) / 32767;
            *colour1 = colours1[index];
            *colour2 = colours2[index];
        }
    }
    lastColour1 = *colour1;
    lastColour2 = *colour2;
}

float CVehicleModel::GetUnknown58() {
    return unknown58 + unknownA95814;
}

void CVehicleModel::ChooseVehicleColour(uint8_t* colour1, uint8_t* colour2) {
    if (numColours == 0) {
        *colour1 = 0;
        *colour2 = 0;
        return;
    }
    lastColourIndex = uint8_t((lastColourIndex + 1) % numColours);
    *colour1 = colours1[lastColourIndex];
    *colour2 = colours2[lastColourIndex];
    if (numColours > 1) {
        CVehicle* player = FindPlayerVehicle(0);
        if (player && CModelInfo::ms_modelInfoPtrs[player->modelIndex] == reinterpret_cast<CBaseModelInfo*>(this) &&
            player->colour1 == *colour1 && player->colour2 == *colour2) {
            lastColourIndex = uint8_t((lastColourIndex + 1) % numColours);
            *colour1 = colours1[lastColourIndex];
            *colour2 = colours2[lastColourIndex];
        }
    }
}

CVehicleModel* CVehicleModel::Destroy(uint8_t flags) {
    ClumpModel_Destruct(this);
    if (flags & 1) {
        Free(this);
    }
    return this;
}

void CVehicleModel::SetName(const char* name) {
    BaseModel_SetName(this, name);
    // (the game's buffer is 0x20 bytes: long names would run over its stack)
    char path[0x40] = "models\\vehicles\\";
    std::strcat(path, name);
    if (AnimManager_LoadGroup(animManager, path, &animGroups, 0) == 0) {
        if (!AnimGroups_FindByHash(animGroups, HashName(defaultVehicleAnimGroup), &animGroup)) {
            animGroup = -1;
        }
    } else {
        animGroup = -1;
    }
}

void CVehicleModel::DeleteObjects() {
    for (int i = 0; i < numExtras; i++) {
        void* root = Node_GetRoot(extras[i]);
        Node_Destroy(extras[i]);
        extras[i] = nullptr;
        Node_Release(root);
    }
    numExtras = 0;
    ClumpModel_DeleteObjects(this);
}

int32_t CVehicleModel::GetDependency8() {
    return animFile2;
}

void CVehicleModel::SetClump(void* newClump, int) {
    ClumpModel_SetClump(this, newClump, 0);   // (the argument isn't passed on)
    Clump_ForAllObjects(clump, reinterpret_cast<void*>(0x00535520), clump);
    ClumpModel_SetFrameIds(this, frameLists[vehicleClass]);
    VehicleModel_SetUpExtras(this);
    VehicleModel_5355D0(this);
}

// vf4 0x5353A0 (thiscall): an instance, with the two extras ChooseColour1/2 pick
static void* __fastcall VehicleModel_CreateInstance(CVehicleModel* mi, re::Edx) {
    void* instance = ClumpModel_CreateInstance(mi);
    if (mi->numExtras == 0) {
        lastExtra1 = 0xFF;
        lastExtra2 = 0xFF;
        ClumpModel_SetAtomicRendererCallbacks(mi, instance);
        return instance;
    }
    void* root = Node_GetRoot(instance);
    int32_t extra1 = VehicleModel_ChooseColour1(mi);
    if (extra1 != -1 && mi->extras[extra1]) {
        AddExtra(instance, root, mi->extras[extra1]);
    }
    lastExtra1 = uint8_t(extra1);
    int32_t extra2 = VehicleModel_ChooseColour2(mi);
    if (extra2 != -1 && mi->extras[extra2]) {
        AddExtra(instance, root, mi->extras[extra2]);
    }
    lastExtra2 = uint8_t(extra2);
    ClumpModel_SetAtomicRendererCallbacks(mi, instance);
    return instance;
}

// ---- CWeaponModelInfo -----------------------------------------------------------------

// vf13 0x535EC0 (thiscall)
static void __fastcall WeaponModel_SetClump(CWeaponModelInfo* mi, re::Edx, void* newClump, int) {
    ClumpModel_SetClump(mi, newClump, 0);
    Clump_ForAllObjects(mi->clump, reinterpret_cast<void*>(0x006CA7D0), reinterpret_cast<void*>(0x005F2DD0));
}

// vf1 0x535EF0 (thiscall): the name, and "models\objects\<name>"'s animation group
static void __fastcall WeaponModel_SetName(CWeaponModelInfo* mi, re::Edx, const char* name) {
    BaseModel_SetName(mi, name);
    char path[0x40] = "models\\objects\\";
    std::strcat(path, name);
    AnimManager_LoadGroup(animManager, path, &mi->unknown48, 0);
}

// 0x535F80 (thiscall): its second animation file by name ("null": left as it is)
static void __fastcall WeaponModel_SetAnimFile2(CWeaponModelInfo* mi, re::Edx, const char* name) {
    if (_stricmp(name, "null") != 0) {
        mi->animFile2 = FindAgrId(name);
    }
}

// ---- registration ---------------------------------------------------------------------

#define VMI_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)
#define VMI_MEMBER(name, address, member) VMI_REVERSED(name, address, member)

VMI_REVERSED("CVehicleModelInfo::CountColourSlots", 0x00534E10, &CountColourSlotsImpl);
VMI_REVERSED("CVehicleModelInfo::PickColour", 0x00534E40, &PickColourImpl);
VMI_REVERSED("CVehicleModelInfo::GetAllowedColours", 0x00534D10, &GetAllowedColoursImpl);
VMI_REVERSED("CVehicleModelInfo::Get", 0x005359D0, &GetVehicleModelInfo);
VMI_MEMBER("CVehicleModelInfo::Construct", 0x00535110, &CVehicleModel::Construct);
VMI_MEMBER("CVehicleModelInfo::GetFrontSeatPos2", 0x00534B40, &CVehicleModel::GetFrontSeatPos2);
VMI_MEMBER("CVehicleModelInfo::SetAnimFile2", 0x00534B00, &CVehicleModel::SetAnimFile2);
VMI_MEMBER("CVehicleModelInfo::GetFramePosition", 0x00534CD0, &CVehicleModel::GetFramePosition);
VMI_MEMBER("CVehicleModelInfo::ChooseColour1", 0x00534ED0, &CVehicleModel::ChooseColour1);
VMI_MEMBER("CVehicleModelInfo::ChooseColour2", 0x00534F90, &CVehicleModel::ChooseColour2);
VMI_MEMBER("CVehicleModelInfo::AvoidLastColours", 0x00535060, &CVehicleModel::AvoidLastColours);
VMI_MEMBER("CVehicleModelInfo::GetUnknown58", 0x005350F0, &CVehicleModel::GetUnknown58);
VMI_MEMBER("CVehicleModelInfo::ChooseVehicleColour", 0x00535D90, &CVehicleModel::ChooseVehicleColour);
VMI_MEMBER("CVehicleModelInfo::Destroy", 0x00535380, &CVehicleModel::Destroy);
VMI_MEMBER("CVehicleModelInfo::SetName", 0x00534A30, &CVehicleModel::SetName);
VMI_MEMBER("CVehicleModelInfo::DeleteObjects", 0x00534B50, &CVehicleModel::DeleteObjects);
VMI_MEMBER("CVehicleModelInfo::GetDependency8", 0x00535370, &CVehicleModel::GetDependency8);
VMI_MEMBER("CVehicleModelInfo::SetClump", 0x00535E70, &CVehicleModel::SetClump);
VMI_REVERSED("CVehicleModelInfo::CreateInstance", 0x005353A0, &VehicleModel_CreateInstance);
VMI_REVERSED("CWeaponModelInfo::SetClump", 0x00535EC0, &WeaponModel_SetClump);
VMI_REVERSED("CWeaponModelInfo::SetName", 0x00535EF0, &WeaponModel_SetName);
VMI_REVERSED("CWeaponModelInfo::SetAnimFile2", 0x00535F80, &WeaponModel_SetAnimFile2);
