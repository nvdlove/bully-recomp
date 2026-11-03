// 0x522180 to 0x522D70: CPedModelInfo (GTA's, with Bully's extras): the animation
// files, the model's hierarchy ("models\peds\" or "Cuts\Models\") and its six bones
// (hands, feet, pelvis, dummy), the texture and palette names, the unique-ped rule,
// the hit col model built from the skin's bones (GTA San Andreas's
// CreateHitColModelSkinned), and the seasonal texture switch (+0x7C..+0x94). Also the
// 3x4 matrix inverse it uses.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, ms_bNoPalettes_BF3809, 0x00BF3809);
GAME_VAR(void*, ms_paletteSentinel, 0x01B649DC);
GAME_VAR(void*, ms_pTxdIndex_CA2D5C, 0x00CA2D5C);
GAME_ARRAY(const char*, ms_pedBoneNames, 0x00A76CD8);   // RightHand, LeftHand, RightFoot, LeftFoot, Pelvis, dummy
inline void* const ms_animManager = reinterpret_cast<void*>(0x020C4910);
inline void* const ms_texturePalettes = reinterpret_cast<void*>(0x00D02850);

// the hit spheres: bone, piece, x and z offsets, radius (10)
struct ColNodeInfo {
    uint32_t bone;     // +0
    uint8_t piece;     // +4
    uint8_t pad[3];
    float x;           // +8
    float z;           // +0xC
    float radius;      // +0x10
    uint32_t unknown14;
};
static_assert(sizeof(ColNodeInfo) == 0x18);
GAME_ARRAY(ColNodeInfo, m_pColNodeInfos, 0x00A76BEC);

struct ColSphere {
    float center[3];   // +0
    float radius;      // +0xC
    uint8_t surface;   // +0x10
    uint8_t piece;     // +0x11
    uint8_t pad[2];
};
static_assert(sizeof(ColSphere) == 0x14);

GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void*, __thiscall, ElementTable_FindById, 0x00465580, void* table, uint32_t id, int32_t* outIndex);
GAME_FN(int32_t, __cdecl, AnimFileIndex, 0x0052DAD0, const char* name);
GAME_FN(void, __thiscall, ClumpModelInfo_SetAnimFile_50ED40, 0x0050ED40, void* self, const char* name);
GAME_FN(int, __thiscall, AnimManager_LoadHierarchy, 0x006C2AE0, void* manager, const char* path, void* out, uintptr_t);
GAME_FN(void, __thiscall, BaseModelInfo_Vf2_50EE70, 0x0050EE70, void* self);
GAME_FN(void, __thiscall, ClumpModelInfo_Vf3_510C40, 0x00510C40, void* self);
GAME_FN(void, __thiscall, ClumpModelInfo_Destruct, 0x00510D90, void* self);
GAME_FN(void, __thiscall, ClumpModelInfo_SetClump_510CB0, 0x00510CB0, void* self, void* clump, int);
GAME_FN(void, __thiscall, ColModel_Destruct, 0x0056E7D0, void* colModel);
GAME_FN(void, __cdecl, ColModel_Delete, 0x0056EB30, void* colModel);
GAME_FN(void*, __cdecl, ColModel_New, 0x0056E7C0, size_t size);
GAME_FN(uint8_t*, __thiscall, ColModel_Construct, 0x0056E7A0, void* self);
GAME_FN(void, __thiscall, ColModel_SetBoundingSphere, 0x0056EB80, void* self, float radius, const float* center);
GAME_FN(void, __thiscall, ColBox_Set, 0x00564830, void* box, const float* min, const float* max);
GAME_FN(void, __thiscall, ColData_Destruct, 0x0056DBD0, void* data);
GAME_FN(uint8_t*, __thiscall, ColData_Construct, 0x0056DAD0, void* data);
GAME_FN(void*, __cdecl, MemoryMgr_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, MemoryMgr_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, MemoryMgr_MallocTagged_5EEA50, 0x005EEA50, size_t size, int);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int, __cdecl, TxdStore_FindSlot_5F5B10, 0x005F5B10, const char* name, uintptr_t);
GAME_FN(uint8_t*, __cdecl, Palette_Find_5F6DB0, 0x005F6DB0, const char* name, uintptr_t, uintptr_t);
GAME_FN(void, __thiscall, TexturePalettes_Add, 0x005DBEB0, void* self, const char* name, void* palette, int);
GAME_FN(int32_t, __cdecl, TxdStore_FindSeasonal_5F2450, 0x005F2450, int32_t name, int);
GAME_FN(void*, __cdecl, GetAnimHierarchyFromSkinClump, 0x005EE1A0, void* clump);
GAME_FN(void*, __cdecl, RpClumpGetFrame, 0x006C7370, void* clump);
GAME_FN(const float*, __cdecl, RwFrameGetMatrix, 0x006C9A90, void* frame);
GAME_FN(void, __cdecl, MatrixInvert, 0x00522440, float* out, const float* in);
GAME_FN(uint8_t*, __cdecl, Hierarchy_GetMatrixArray, 0x006C7710, void* hierarchy);
GAME_FN(float*, __thiscall, NiTransform_Multiply, 0x00406830, const float* self, float* out, const float* other);
GAME_FN(float*, __thiscall, NiMatrix3_Transform, 0x00768A70, const float* self, float* out, const float* point);
GAME_FN(float*, __cdecl, VectorAdd, 0x004129A0, float* out, const float* a, const float* b);
GAME_FN(void, __cdecl, ForAllAtomics_6C7380, 0x006C7380, void* clump, void* callback, void* data);
GAME_FN(void, __cdecl, TxdStore_GetSlots_5F0AC0, 0x005F0AC0, const char* name, int* out1, int* out2);
GAME_FN(int32_t, __thiscall, TxdIndex_Find_429070, 0x00429070, void* self, uint32_t hash);
GAME_FN(void, __cdecl, ModelInfo_SetTxd_52E7F0, 0x0052E7F0, int32_t modelIndex, int32_t txd);
GAME_FN(void, __thiscall, CPedModelInfo_CreateHitColModelSkinned, 0x00522940, void* self, void* clump);

constexpr uint32_t kPedModelInfoVtable = 0x00917C28;

using re::At;

void FreeHitColModel(uint8_t* self) {
    void* colModel = At<void*>(self, 0x58);
    if (colModel) {
        ColModel_Destruct(colModel);
        ColModel_Delete(colModel);
        At<void*>(self, 0x58) = nullptr;
    }
}

// the texture and palette names at +off / +off+4, copied; the palette registered
void SetTextureNames(uint8_t* self, int offset, const char* txd, const char* palette) {
    PushMemoryTag(0x2A);
    MemoryMgr_Free(At<void*>(self, offset));
    MemoryMgr_Free(At<void*>(self, offset + 4));
    size_t txdLength = std::strlen(txd) + 1;
    size_t paletteLength = std::strlen(palette) + 1;
    At<char*>(self, offset) = static_cast<char*>(MemoryMgr_Malloc(txdLength));
    At<char*>(self, offset + 4) = static_cast<char*>(MemoryMgr_Malloc(paletteLength));
    std::strcpy(At<char*>(self, offset), txd);
    std::strcpy(At<char*>(self, offset + 4), palette);
    if (!ms_bNoPalettes_BF3809 && !TxdStore_FindSlot_5F5B10(At<char*>(self, offset), 0)) {
        uint8_t* node = Palette_Find_5F6DB0(At<char*>(self, offset + 4), 0, 0);
        uint8_t* last = node;
        while (node != ms_paletteSentinel) {
            last = node;
            node = vc::Call<uint8_t*>(node, 0x34, -1);
        }
        TexturePalettes_Add(ms_texturePalettes, At<char*>(self, offset + 4), last, 0);
    }
    PopMemoryTag();
}

} // namespace

namespace PedModelInfo {

// 0x522180 (thiscall): +0x5C = +0x5E
static void __fastcall ResetUnique(uint8_t* self, re::Edx) {
    At<int16_t>(self, 0x5C) = At<int16_t>(self, 0x5E);
}

// 0x522190 / 0x5221C0 / 0x5221F0 (thiscall): an animation file (+0xC8 / +0xCC / +0xD0)
// by name ("null": left as it is)
static void __fastcall SetAnimFile1(uint8_t* self, re::Edx, const char* name) {
    if (_stricmp(name, "null") != 0) {
        At<int32_t>(self, 0xC8) = AnimFileIndex(name);
    }
}
static void __fastcall SetAnimFile2(uint8_t* self, re::Edx, const char* name) {
    if (_stricmp(name, "null") != 0) {
        At<int32_t>(self, 0xCC) = AnimFileIndex(name);
    }
}
static void __fastcall SetAnimFile3(uint8_t* self, re::Edx, const char* name) {
    if (_stricmp(name, "null") != 0) {
        At<int32_t>(self, 0xD0) = AnimFileIndex(name);
    }
}

// 0x522220 (thiscall): the name (not for cutscene models) and the hierarchy from
// "models\peds\<name>" or "Cuts\Models\<name>" into +0x48, then the six bones' indices
// (+0xB0..; -1 when missing, all -1 without a hierarchy). No name: none (+0x48 cleared
// for a cutscene model)
static void __fastcall SetHierarchy(uint8_t* self, re::Edx, const char* name, int cutscene) {
    bool loaded = false;
    if (!name) {
        if (cutscene) {
            At<void*>(self, 0x48) = nullptr;
        }
        ClumpModelInfo_SetAnimFile_50ED40(self, nullptr);
    } else {
        char path[32];
        if (!cutscene) {
            ClumpModelInfo_SetAnimFile_50ED40(self, name);
            std::memcpy(path, "models\\peds\\", 13);
        } else {
            std::memcpy(path, "Cuts\\Models\\", 13);
        }
        std::strcat(path, name);
        loaded = AnimManager_LoadHierarchy(ms_animManager, path, self + 0x48, 0) == 0;
    }
    if (loaded) {
        for (int i = 0; i < 6; i++) {
            int32_t* index = &At<int32_t>(self, 0xB0 + i * 4);
            void* table = At<void*>(self, 0x48);
            if (!ElementTable_FindById(table, HashName(ms_pedBoneNames[i]), index)) {
                *index = -1;
            }
        }
        return;
    }
    for (int i = 0; i < 6; i++) {
        At<int32_t>(self, 0xB0 + i * 4) = -1;
    }
}

// 0x522370 (thiscall): SetPedType (+0x74)
static void __fastcall SetPedType(uint8_t* self, re::Edx, int32_t type) {
    At<int32_t>(self, 0x74) = type;
}

// 0x522380 (thiscall): GetPedType into *out
static int32_t* __fastcall GetPedType(uint8_t* self, re::Edx, int32_t* out) {
    *out = At<int32_t>(self, 0x74);
    return out;
}

// 0x522390 (thiscall) vf2: DeleteRwObject, the hit col model and the four names too
static void __fastcall DeleteRwObject(uint8_t* self, re::Edx) {
    BaseModelInfo_Vf2_50EE70(self);
    FreeHitColModel(self);
    for (int offset = 0x64; offset <= 0x70; offset += 4) {
        void* name = At<void*>(self, offset);
        if (name) {
            MemoryMgr_Free(name);
            At<void*>(self, offset) = nullptr;
        }
    }
}

// 0x522410 (thiscall): the destructor body
static void __fastcall Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kPedModelInfoVtable;
    ClumpModelInfo_Destruct(self);
}

// 0x522420 (thiscall): the seasonal texture dictionary (+0x90) for +0x80
static void __fastcall FindSeasonalTxd(uint8_t* self, re::Edx) {
    int32_t name = At<int32_t>(self, 0x80);
    if (name != 0) {
        At<int32_t>(self, 0x90) = TxdStore_FindSeasonal_5F2450(name, -1);
    }
}

// 0x522440 (cdecl): the inverse of a rotation and translation (the rotation by its
// cofactors; a zero determinant counts as 1)
static void __cdecl Invert(float* out, const float* in) {
    float m0 = in[0], m1 = in[1], m2 = in[2];
    float m3 = in[3], m4 = in[4], m5 = in[5];
    float m6 = in[6], m7 = in[7], m8 = in[8];
    float t0 = in[9], t1 = in[10], t2 = in[11];
    float c0 = float(double(m8) * m4 - double(m5) * m7);
    float c1 = float(-(double(m3) * m8 - double(m6) * m5));
    float c2 = float(double(m3) * m7 - double(m6) * m4);
    float det = float((double(m1) * c1 + double(m0) * c0) + double(m2) * c2);
    float inv = det == 0.0f ? 1.0f : float(1.0 / det);
    c0 = inv * c0;
    c1 = inv * c1;
    c2 = inv * c2;
    float ninv = -inv;
    float r1 = float((double(m1) * m8 - double(m2) * m7) * ninv);
    float r4 = float((double(m0) * m8 - double(m2) * m6) * inv);
    float r7 = float((double(m0) * m7 - double(m1) * m6) * ninv);
    float r2 = float((double(m1) * m5 - double(m2) * m4) * inv);
    float r5 = float((double(m0) * m5 - double(m3) * m2) * ninv);
    float r8 = float((double(m4) * m0 - double(m3) * m1) * inv);
    out[9] = float(-((double(r1) * t1 + double(c0) * t0) + double(t2) * r2));
    out[10] = float(-((double(t0) * c1 + double(t1) * r4) + double(t2) * r5));
    out[11] = float(-((double(r7) * t1 + double(c2) * t0) + double(r8) * t2));
    out[0] = c0;
    out[3] = c1;
    out[6] = c2;
    out[1] = r1;
    out[4] = r4;
    out[7] = r7;
    out[2] = r2;
    out[5] = r5;
    out[8] = r8;
}

// 0x5226A0 (thiscall) vf3: the clump's and the hit col model's
static void __fastcall DeleteCollision(uint8_t* self, re::Edx) {
    ClumpModelInfo_Vf3_510C40(self);
    void* colModel = At<void*>(self, 0x58);
    if (colModel) {
        ColModel_Destruct(colModel);
        ColModel_Delete(colModel);
    }
    At<void*>(self, 0x58) = nullptr;
}

// 0x5226D0 / 0x5227D0 (thiscall): the texture and palette names (+0x64/+0x68, +0x6C/+0x70)
static void __fastcall SetTextures1(uint8_t* self, re::Edx, const char* txd, const char* palette) {
    SetTextureNames(self, 0x64, txd, palette);
}
static void __fastcall SetTextures2(uint8_t* self, re::Edx, const char* txd, const char* palette) {
    SetTextureNames(self, 0x6C, txd, palette);
}

// 0x5228D0 (thiscall): how many of it may be about (+0x5C; -1: none): 1 in areas 16,
// 26, 29, 30, 33, 34, 39 and 46; none in area 14 for +0x50 models or area 35 for the
// others
static int16_t __fastcall GetUniqueStatus(uint8_t* self, re::Edx) {
    int16_t status = At<int16_t>(self, 0x5C);
    if (status < 0) {
        return status;
    }
    int32_t area = currArea;
    if (area == 0x10 || area == 0x1A || area == 0x1D || area == 0x1E || area == 0x21 || area == 0x22 ||
        area == 0x27 || area == 0x2E) {
        return 1;
    }
    if ((self[0x50] && area == 0xE) || (!self[0x50] && area == 0x23)) {
        return -1;
    }
    return status;
}

// 0x522940 (thiscall): CreateHitColModelSkinned: ten spheres at the skin's bones
// (offset in x and z; a bone past the hierarchy's: an empty sphere of surface 0x41),
// bounding sphere 2 and box (-0.5, -0.5, -0.2)..(0.5, 0.5, 2)
static void __fastcall CreateHitColModelSkinned(uint8_t* self, re::Edx, void* clump) {
    uint8_t* hierarchy = static_cast<uint8_t*>(GetAnimHierarchyFromSkinClump(clump));
    void* memory = ColModel_New(0x40);
    uint8_t* colModel = memory ? ColModel_Construct(memory) : nullptr;
    auto* spheres = static_cast<ColSphere*>(MemoryMgr_MallocTagged_5EEA50(200, 0));
    float zero[3] = {0.0f, 0.0f, 0.0f};
    float inverse[13];
    inverse[12] = 1.0f;
    MatrixInvert(inverse, RwFrameGetMatrix(RpClumpGetFrame(clump)));
    for (int i = 0; i < 10; i++) {
        const ColNodeInfo& node = m_pColNodeInfos[i];
        ColSphere& sphere = spheres[i];
        if (node.bone < At<uint32_t>(hierarchy, 4)) {
            float point[3] = {zero[0], zero[1], zero[2]};
            float transform[13];
            std::memcpy(transform, inverse, sizeof(transform));
            const float* bone =
                reinterpret_cast<const float*>(Hierarchy_GetMatrixArray(hierarchy) + node.bone * 0x34);
            float product[13];
            std::memcpy(transform, NiTransform_Multiply(transform, product, bone), sizeof(transform));
            float scale = transform[12];
            float rotated[3];
            const float* r = NiMatrix3_Transform(transform, rotated, point);
            float scaled[3] = {r[0] * scale, r[1] * scale, scale * r[2]};
            float centre[3] = {transform[9] + scaled[0], transform[10] + scaled[1], transform[11] + scaled[2]};
            float offset[3] = {node.x, 0.0f, node.z};
            float out[3];
            const float* c = VectorAdd(out, centre, offset);
            sphere.center[0] = c[0];
            sphere.center[1] = c[1];
            sphere.center[2] = c[2];
            sphere.radius = node.radius;
            sphere.surface = 0x1A;
            sphere.piece = node.piece;
        } else {
            sphere.radius = 0.0f;
            sphere.center[0] = zero[0];
            sphere.center[1] = zero[1];
            sphere.center[2] = zero[2];
            sphere.surface = 0x41;
            sphere.piece = 0;
        }
    }
    void* data = At<void*>(colModel, 0x38);
    if (data) {
        ColData_Destruct(data);
        MemoryMgr_Free(data);
    }
    void* dataMemory = MemoryMgr_Malloc(0x24);
    uint8_t* colData = dataMemory ? ColData_Construct(dataMemory) : nullptr;
    float centre[3] = {0.0f, 0.0f, 0.0f};
    At<void*>(colModel, 0x38) = colData;
    At<ColSphere*>(colData, 0) = spheres;
    colData[0x1E] = 10;
    ColModel_SetBoundingSphere(colModel, 2.0f, centre);
    float max[3] = {0.5f, 0.5f, 2.0f};
    float min[3] = {-0.5f, -0.5f, -0.2f};
    ColBox_Set(colModel + 0x10, min, max);
    At<void*>(self, 0x58) = colModel;
}

// 0x522C30 (thiscall): the seasonal texture switch for a model: its model index, its
// texture dictionaries now (+0x8C, +0x90), and the named one's hash and slots
static void __fastcall SetSeasonalTxd(uint8_t* self, re::Edx, int32_t modelIndex, const char* name) {
    At<int32_t>(self, 0x94) = 0;
    At<int32_t>(self, 0x84) = modelIndex;
    At<uint32_t>(self, 0x7C) = 0;
    At<int32_t>(self, 0x80) = 0;
    At<int32_t>(self, 0x88) = -1;
    At<int32_t>(self, 0x8C) = At<int16_t>(ms_modelInfoPtrs[modelIndex], 0x16);
    At<int32_t>(self, 0x90) = At<int16_t>(ms_modelInfoPtrs[modelIndex], 0x16);
    if (name) {
        At<uint32_t>(self, 0x7C) = HashName(name);
        TxdStore_GetSlots_5F0AC0(name, &modelIndex, &At<int32_t>(self, 0x80));
    }
}

// 0x522CB0 (thiscall): the switch to state 0 (the model's own dictionary, +0x8C) or 1
// (+0x90 unless -1), the index (+0x88) looked up from the hash when needed
static void __fastcall SwitchSeasonalTxd(uint8_t* self, re::Edx, int32_t state) {
    if (At<int32_t>(self, 0x94) == state) {
        return;
    }
    At<int32_t>(self, 0x94) = state;
    if (At<int32_t>(self, 0x88) == -1 && At<uint32_t>(self, 0x7C) != 0) {
        int32_t index = TxdIndex_Find_429070(ms_pTxdIndex_CA2D5C, At<uint32_t>(self, 0x7C));
        At<int32_t>(self, 0x88) = index;
        if (index == -1) {
            return;
        }
    }
    int32_t modelIndex = At<int32_t>(self, 0x84);
    if (modelIndex <= 0 || At<int32_t>(self, 0x88) < 0) {
        return;
    }
    if (state == 0) {
        At<int16_t>(ms_modelInfoPtrs[modelIndex], 0x16) = At<int16_t>(self, 0x8C);
        ModelInfo_SetTxd_52E7F0(At<int32_t>(self, 0x84), At<int32_t>(self, 0x88));
    } else if (state == 1) {
        int32_t txd = At<int32_t>(self, 0x90);
        if (txd != -1) {
            At<int16_t>(ms_modelInfoPtrs[modelIndex], 0x16) = int16_t(txd);
        }
        ModelInfo_SetTxd_52E7F0(At<int32_t>(self, 0x84), At<int32_t>(self, 0x88));
    }
}

// 0x522D70 (thiscall) vf13: SetClump: the clump's, the hit col model if none yet, and a
// pass over the atomics (0x6CA7D0 with 0x5F2DD0)
static void __fastcall SetClump(uint8_t* self, re::Edx, void* clump, int) {
    ClumpModelInfo_SetClump_510CB0(self, clump, 1);
    if (!At<void*>(self, 0x58)) {
        CPedModelInfo_CreateHitColModelSkinned(self, clump);
    }
    ForAllAtomics_6C7380(At<void*>(self, 0x20), reinterpret_cast<void*>(0x006CA7D0), reinterpret_cast<void*>(0x005F2DD0));
}

} // namespace PedModelInfo

// ---- registration ----------------------------------------------------------------------

#define C16_PEDMI_REVERSED(name, fn, address)                                                        \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace PedModelInfo {

C16_PEDMI_REVERSED("CPedModelInfo::ResetUnique", ResetUnique, 0x00522180);
C16_PEDMI_REVERSED("CPedModelInfo::SetAnimFile1", SetAnimFile1, 0x00522190);
C16_PEDMI_REVERSED("CPedModelInfo::SetAnimFile2", SetAnimFile2, 0x005221C0);
C16_PEDMI_REVERSED("CPedModelInfo::SetAnimFile3", SetAnimFile3, 0x005221F0);
C16_PEDMI_REVERSED("CPedModelInfo::SetHierarchy", SetHierarchy, 0x00522220);
C16_PEDMI_REVERSED("CPedModelInfo::SetPedType", SetPedType, 0x00522370);
C16_PEDMI_REVERSED("CPedModelInfo::GetPedType", GetPedType, 0x00522380);
C16_PEDMI_REVERSED("CPedModelInfo::DeleteRwObject", DeleteRwObject, 0x00522390);
C16_PEDMI_REVERSED("CPedModelInfo::Destruct", Destruct, 0x00522410);
C16_PEDMI_REVERSED("CPedModelInfo::FindSeasonalTxd", FindSeasonalTxd, 0x00522420);
C16_PEDMI_REVERSED("CMatrix::Invert_522440", Invert, 0x00522440);
C16_PEDMI_REVERSED("CPedModelInfo::DeleteCollision", DeleteCollision, 0x005226A0);
C16_PEDMI_REVERSED("CPedModelInfo::SetTextures1", SetTextures1, 0x005226D0);
C16_PEDMI_REVERSED("CPedModelInfo::SetTextures2", SetTextures2, 0x005227D0);
C16_PEDMI_REVERSED("CPedModelInfo::GetUniqueStatus", GetUniqueStatus, 0x005228D0);
C16_PEDMI_REVERSED("CPedModelInfo::CreateHitColModelSkinned", CreateHitColModelSkinned, 0x00522940);
C16_PEDMI_REVERSED("CPedModelInfo::SetSeasonalTxd", SetSeasonalTxd, 0x00522C30);
C16_PEDMI_REVERSED("CPedModelInfo::SwitchSeasonalTxd", SwitchSeasonalTxd, 0x00522CB0);
C16_PEDMI_REVERSED("CPedModelInfo::SetClump", SetClump, 0x00522D70);

} // namespace PedModelInfo
