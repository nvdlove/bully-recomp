// CVehicleModelInfo, part 2 (0x534C10..0x535FBF): its scene graph prepared (the
// component table by vehicle class: dummies' positions, extras taken out, flags on the
// atomics, wheels), the colour table (Config\Dat\CARCOLS.DAT) and a vehicle's colours.
//
// Found here: the component tables (0xA95AC8, by vehicleClass +0x50) are {name, id,
// flags} of 12 bytes, a null name ending them; flags 0x208 find a frame by name
// (8: a dummy, its position at +0x64 + id*12; else an extra atomic, kept at +0x1C0,
// counted at +0x61), the others by id (2: its damaged atomic's geometry from the ok
// one; 4: a wheel from model +0x5C scaled by +0x58). +0x60 counts flag 0x10. The
// colours: 256 RGBA at 0xCE7128; a model's pairs at +0x1AC / +0x1B4, their count
// at +0x1BC, the current pair at +0x1BE / +0x1BF.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

#include "game/Crt.h"
namespace vehiclemi17 {

using col::CVector;

struct ComponentEntry {
    re::Ptr32<const char> name;   // +0
    int32_t id;         // +4
    uint32_t flags;     // +8
};
static_assert(sizeof(ComponentEntry) == 12);

struct LineFile {
    re::Ptr32<char> data;     // +0 (freed after)
    int32_t left;   // +4
    int32_t end;    // +8
};
static_assert(sizeof(LineFile) == 0xC);

GAME_ARRAY(ComponentEntry*, componentTables, 0x00A95AC8);
GAME_ARRAY(uint32_t, vehicleColourTable, 0x00CE7128);    // RGBA
GAME_ARRAY(uint32_t, colourTableCE6D28, 0x00CE6D28);     // 0x100 words, cleared by the loader
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, maxExtras0, 0x00CE7528);
GAME_VAR(int32_t, maxExtras1, 0x00CE752C);
GAME_VAR(int32_t, vehicleIdFirst, 0x00A136A8);
GAME_VAR(int32_t, vehicleIdLast, 0x00A136AC);

// callbacks the game passes (its own code)
inline void* const cbSetAtomicFlag = reinterpret_cast<void*>(0x00534BD0);
inline void* const cbSetAtomicBone = reinterpret_cast<void*>(0x00534BB0);
inline void* const cbGetOkAndDamaged = reinterpret_cast<void*>(0x00534BF0);
inline void* const cbDestroyFrameTree = reinterpret_cast<void*>(0x005354F0);
inline void* const cbPrepareAtomic = reinterpret_cast<void*>(0x005355B0);
inline void* const cbFindFrameByName = reinterpret_cast<void*>(0x00510A40);
inline void* const cbFindFrameById = reinterpret_cast<void*>(0x00510A90);
inline void* const renderCbAlpha = reinterpret_cast<void*>(0x005F2D60);
inline void* const renderCbAlpha2 = reinterpret_cast<void*>(0x005F3610);
inline void* const renderCbWheel = reinterpret_cast<void*>(0x005F2CA0);

GAME_FN(void, __cdecl, ForAllAtomicsOfFrame, 0x006C9AA0, void* frame, void* callback, uintptr_t data);
GAME_FN(void*, __cdecl, ForAllChildren, 0x006C9B50, void* frame, void* callback, void* data);
GAME_FN(void, __cdecl, Bone_Destroy, 0x006CA260, void* frame);
GAME_FN(void*, __cdecl, GetFrameOfObject, 0x006C7370, void* object);
GAME_FN(uint32_t, __cdecl, GetFrameFlags, 0x005EF880, void* frame);
GAME_FN(void*, __cdecl, PrepareGeometry, 0x00824EC0, void* atomic);
GAME_FN(void, __cdecl, Atomic_Set6C6BA0, 0x006C6BA0, void* atomic, void* value);
GAME_FN(void, __cdecl, Node_SetFlags, 0x006C69D0, void* node, int flags);
GAME_FN(void, __cdecl, ForAllAtomics, 0x006C7380, void* clump, void* callback, void* data);
GAME_FN(void, __cdecl, Material_SetColours, 0x00411A50, void* material, const uint32_t* colour1, const uint32_t* colour2);
GAME_FN(void, __thiscall, LineFile_Load, 0x00426610, LineFile* file, const char* name);
GAME_FN(uint8_t, __cdecl, LineFile_ReadLine, 0x0042D600, LineFile* file, char* line, int size);
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(uint8_t*, __cdecl, GetModelInfoInRange, 0x0051C200, const char* name, int first, int last);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void*, __cdecl, Frame_GetAtomic, 0x005EE030, void* frame);
GAME_FN(void, __cdecl, Clump_RemoveAtomic, 0x006C7340, void* clump, void* atomic);
GAME_FN(void, __cdecl, Frame_Detach, 0x006C9CB0, void* frame);
GAME_FN(float*, __thiscall, NiMatrix3_MultiplyVector, 0x00768A70, const void* self, float* out, const float* v);
GAME_FN(void, __cdecl, UpdateFrame, 0x006C9AF0, void* frame);
GAME_FN(void*, __cdecl, Atomic_GetGeometry, 0x006C6A30, void* atomic);
GAME_FN(void, __cdecl, Atomic_SetBone, 0x006C6A10, void* atomic, void* bone);
GAME_FN(void, __cdecl, Clump_AddAtomic, 0x006C74A0, void* clump, void* atomic);
GAME_FN(void, __cdecl, Frame_Scale, 0x006CA120, void* frame, const float* scale, int combine);
GAME_FN(void, __thiscall, SetVehicleAtomicFlags_Game, 0x00534C10, void* self, void* frame, uint32_t flags);

using re::At;

// 0x534C10 (thiscall, `this` unused): the component flags as atomic flags (+0x6C of
// each atomic under the frame)
void __fastcall SetVehicleAtomicFlags(void* self, re::Edx, void* frame, uint32_t flags) {
    (void)self;
    if (flags & 0x800) {
        ForAllAtomicsOfFrame(frame, cbSetAtomicFlag, 0x80);
    }
    if (flags & 0x1000) {
        ForAllAtomicsOfFrame(frame, cbSetAtomicFlag, 0x100);
    }
    if (flags & 0x80) {
        ForAllAtomicsOfFrame(frame, cbSetAtomicFlag, 0x10);
    } else if ((flags & 0x100) && !(flags & 0x60)) {
        ForAllAtomicsOfFrame(frame, cbSetAtomicFlag, 0x20);
    } else if (flags & 0x20) {
        ForAllAtomicsOfFrame(frame, cbSetAtomicFlag, 4);
    } else if (flags & 0x40) {
        ForAllAtomicsOfFrame(frame, cbSetAtomicFlag, 8);
    }
    if (flags & 0x2000) {
        ForAllAtomicsOfFrame(frame, cbSetAtomicFlag, 0x200);
    } else if (flags & 0x4000) {
        ForAllAtomicsOfFrame(frame, cbSetAtomicFlag, 0x400);
    }
    if (flags & 0x400) {
        ForAllAtomicsOfFrame(frame, cbSetAtomicFlag, 0x40);
    }
}

// 0x5354F0: a frame's children (itself as the callback), its atomics given `data` as
// their bone, then the frame destroyed
void* __cdecl DestroyFrameTreeCB(void* frame, void* data) {
    ForAllChildren(frame, cbDestroyFrameTree, data);
    ForAllAtomicsOfFrame(frame, cbSetAtomicBone, reinterpret_cast<uintptr_t>(data));
    Bone_Destroy(frame);
    return frame;
}

// 0x535520: an atomic prepared by its frame's flags (render callback for 0xC, flag 2:
// hidden, atomic flag 2; flag 1: atomic flag 1)
void* __cdecl SetupAtomicCB(uint8_t* atomic) {
    uint32_t flags = GetFrameFlags(GetFrameOfObject(atomic));
    PrepareGeometry(atomic);
    if (flags & 0xC) {
        Atomic_Set6C6BA0(atomic, (flags & 0x10) ? renderCbAlpha2 : renderCbAlpha);
    }
    if (atomic) {
        void* frame = GetFrameOfObject(atomic);
        if (GetFrameFlags(frame) & 2) {
            Node_SetFlags(atomic, 0);
            At<uint32_t>(atomic, 0x6C) |= 2;
        } else if (GetFrameFlags(frame) & 1) {
            At<uint32_t>(atomic, 0x6C) |= 1;
        }
    }
    return atomic;
}

// 0x5355B0: an atomic's geometry prepared
void* __cdecl PrepareAtomicCB(void* atomic) {
    PrepareGeometry(atomic);
    return atomic;
}

// 0x5355D0 (thiscall): its atomics and extras prepared, the colours unset. (The
// callback counts nothing, so +0xE4[0] and +0x150[0] are what get cleared)
void __fastcall PrepareAtomics(uint8_t* self, re::Edx) {
    struct {
        re::Ptr32<uint8_t> mi;
        int32_t count0;
        int32_t count1;
    } ctx = {self, 0, 0};
    ForAllAtomics(At<void*>(self, 0x20), cbPrepareAtomic, &ctx);
    for (int i = 0; i < int8_t(self[0x61]); i++) {
        PrepareGeometry(At<void*>(self, 0x1C0 + i * 4));
    }
    At<uint32_t>(self, 0xE4 + ctx.count0 * 4) = 0;
    At<uint32_t>(self, 0x150 + ctx.count1 * 4) = 0;
    if (ctx.count0 > maxExtras0) {
        maxExtras0 = ctx.count0;
    }
    if (ctx.count1 > maxExtras1) {
        maxExtras1 = ctx.count1;
    }
    self[0x1BF] = 0xFF;
    self[0x1BE] = 0xFF;
}

// 0x535680 (thiscall): a vehicle's materials given colours c1 and c2 (from the table)
void __fastcall SetVehicleColour(uint8_t* self, re::Edx, uint8_t c1, uint8_t c2, uint8_t* materials) {
    if (!materials) {
        return;
    }
    uint32_t colour1 = vehicleColourTable[c1];
    uint32_t colour2 = vehicleColourTable[c2];
    uint8_t* head = materials + 8;
    for (uint8_t* node = At<uint8_t*>(materials, 8); node != head; node = *reinterpret_cast<re::Ptr32<uint8_t>*>(node)) {
        Material_SetColours(*reinterpret_cast<re::Ptr32<void>*>(node - 0x38), &colour1, &colour2);
    }
    self[0x1BE] = c1;
    self[0x1BF] = c2;
}

// 0x535700: CVehicleModelInfo::LoadVehicleColours: CARCOLS.DAT's "col" section into
// the table, its "car" section's colour pairs into the models
void __cdecl LoadVehicleColours() {
    for (int i = 0; i < 0x100; i++) {
        colourTableCE6D28[i] = 0;
    }
    int section = 0;
    LineFile file = {nullptr, 0, 0};
    LineFile_Load(&file, "Config\\Dat\\CARCOLS.DAT");
    char line[1024];
    uint8_t* colour = reinterpret_cast<uint8_t*>(0x00CE7128);
    while (LineFile_ReadLine(&file, line, 0x400)) {
        int i = 0;
        while (static_cast<signed char>(line[i]) <= ' ' && line[i] != '\0' && line[i] != '\n') {
            i++;
        }
        char* start = &line[i];
        int j = i;
        while (line[j] != '\n' && line[j] != '\0') {
            if (line[j] == ',' || line[j] == '\r') {
                line[j] = ' ';
            }
            j++;
        }
        line[j] = '\0';
        char c = *start;
        if (c == '#' || c == '\0') {
            continue;
        }
        if (section == 0) {
            if (c == 'c' && start[1] == 'o' && start[2] == 'l') {
                section = 1;
            } else if (c == 'c' && start[1] == 'a' && start[2] == 'r') {
                section = 2;
            }
        } else if (c == 'e' && start[1] == 'n' && start[2] == 'd') {
            section = 0;
        } else if (section == 1) {
            int r, g, b;
            Sscanf(line, "%d %d %d", &r, &g, &b);
            colour[0] = uint8_t(r);
            colour[1] = uint8_t(g);
            colour[2] = uint8_t(b);
            colour[3] = 0xFF;
            colour += 4;
        } else if (section == 2) {
            char name[64];
            int v[16];
            int n = Sscanf(line, "%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", name, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5],
                           &v[6], &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14], &v[15]);
            uint8_t* mi = GetModelInfoInRange(name, vehicleIdFirst, vehicleIdLast);
            if (!mi) {
                continue;
            }
            uint8_t count = uint8_t((n - 1) / 2);
            mi[0x1BC] = count;
            if (count != 0) {
                int k = 0;
                do {
                    mi[0x1AC + k] = uint8_t(v[k * 2]);
                    mi[0x1B4 + k] = uint8_t(v[k * 2 + 1]);
                    k++;
                } while (k < mi[0x1BC]);
            }
        }
    }
    if (file.data) {
        Free(file.data);
    }
}

// 0x5359F0 (thiscall): CVehicleModelInfo::PreprocessHierarchy
void __fastcall PreprocessHierarchy(uint8_t* self, re::Edx) {
    re::Ptr32<ComponentEntry>* table = &componentTables[At<int32_t>(self, 0x50)];
    self[0x60] = 0;
    self[0x61] = 0;
    void* clump = nullptr;
    if ((*table)[0].name) {
        for (int i = 0; (*table)[i].name; i++) {
            ComponentEntry& entry = (*table)[i];
            if (!(entry.flags & 0x208)) {
                continue;
            }
            struct {
                uint32_t hash;
                re::Ptr32<void> found;
            } ctx;
            ctx.found = nullptr;
            ctx.hash = HashName(entry.name);
            clump = At<void*>(self, 0x20);
            ForAllChildren(GetFrameOfObject(clump), cbFindFrameByName, &ctx);
            void* frame = ctx.found;
            if (!frame) {
                continue;
            }
            if ((*table)[i].flags & 0x10) {
                self[0x60]++;
            }
            if ((*table)[i].flags & 8) {
                // a dummy: its position in the clump's space
                uint8_t* t = GetLocalTransform(frame);
                float* pos = reinterpret_cast<float*>(self + 0x64 + (*table)[i].id * 12);
                pos[0] = At<float>(t, 0x24);
                pos[1] = At<float>(t, 0x28);
                pos[2] = At<float>(t, 0x2C);
                void* parent = GetFrameOfObject(frame);
                if (parent && GetFrameOfObject(parent)) {
                    do {
                        uint8_t* pt = GetLocalTransform(parent);
                        float scale = At<float>(pt, 0x30);
                        float* p = reinterpret_cast<float*>(self + 0x64 + (*table)[i].id * 12);
                        float rotated[3];
                        float* v = NiMatrix3_MultiplyVector(pt, rotated, p);
                        float sx = scale * v[0];
                        float sy = v[1] * scale;
                        float sz = scale * v[2];
                        p[0] = At<float>(pt, 0x24) + sx;
                        p[1] = At<float>(pt, 0x28) + sy;
                        p[2] = At<float>(pt, 0x2C) + sz;
                        parent = GetFrameOfObject(parent);
                    } while (GetFrameOfObject(parent));
                }
            } else {
                // an extra: its atomic out of the clump
                void* atomic = Frame_GetAtomic(frame);
                Clump_RemoveAtomic(At<void*>(self, 0x20), atomic);
                Frame_Detach(frame);
                SetVehicleAtomicFlags_Game(self, frame, (*table)[i].flags);
                At<void*>(self, 0x1C0 + int8_t(self[0x61]) * 4) = atomic;
                self[0x61]++;
            }
        }
    }
    if (!(*table)[0].name) {
        return;
    }
    for (int i = 0; (*table)[i].name; i++) {
        if ((*table)[i].flags & 0x208) {
            continue;
        }
        struct {
            int32_t id;
            re::Ptr32<void> found;
        } ctx;
        ctx.found = nullptr;
        ctx.id = (*table)[i].id;
        ForAllChildren(GetFrameOfObject(At<void*>(self, 0x20)), cbFindFrameById, &ctx);
        void* frame = ctx.found;
        if (!frame) {
            continue;
        }
        if ((*table)[i].flags & 0x10) {
            self[0x60]++;
        }
        if ((*table)[i].flags & 2) {
            // the damaged atomic gets the ok one's geometry
            struct {
                re::Ptr32<void> ok;
                re::Ptr32<void> damaged;
            } parts = {nullptr, nullptr};
            ForAllChildren(frame, cbDestroyFrameTree, frame);
            UpdateFrame(frame);
            ForAllAtomicsOfFrame(frame, cbGetOkAndDamaged, reinterpret_cast<uintptr_t>(&parts));
            if (parts.ok && parts.damaged) {
                Atomic_Set6C6BA0(parts.damaged, Atomic_GetGeometry(parts.ok));
            }
        }
        SetVehicleAtomicFlags_Game(self, frame, (*table)[i].flags);
        if ((*table)[i].flags & 4) {
            int16_t wheelModel = At<int16_t>(self, 0x5C);
            if (wheelModel == -1) {
                Bone_Destroy(frame);
            } else {
                uint8_t* wheelMi = modelInfoPtrs[wheelModel];
                void* wheel = reinterpret_cast<void*(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(wheelMi))[4]))(wheelMi);
                void* oldFrame = GetFrameOfObject(wheel);
                if (oldFrame) {
                    Bone_Destroy(oldFrame);
                }
                Atomic_SetBone(wheel, frame);
                Clump_AddAtomic(At<void*>(self, 0x20), wheel);
                Atomic_Set6C6BA0(wheel, renderCbWheel);
                float scale[3] = {At<float>(self, 0x58), At<float>(self, 0x58), At<float>(self, 0x58)};
                Frame_Scale(frame, scale, 1);
            }
        }
    }
}

// 0x535FB0 (thiscall): +0x54 and +0x55
void __fastcall SetBytes54(uint8_t* self, re::Edx, uint8_t a, uint8_t b) {
    self[0x54] = a;
    self[0x55] = b;
}

} // namespace vehiclemi17

// ---- registration ----------------------------------------------------------------------

#define VEHICLEMI17_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace vehiclemi17 {
VEHICLEMI17_REVERSED("CVehicleModelInfo::SetVehicleAtomicFlags", 0x00534C10, &SetVehicleAtomicFlags);
VEHICLEMI17_REVERSED("CVehicleModelInfo::DestroyFrameTreeCB", 0x005354F0, &DestroyFrameTreeCB);
VEHICLEMI17_REVERSED("CVehicleModelInfo::SetupAtomicCB", 0x00535520, &SetupAtomicCB);
VEHICLEMI17_REVERSED("CVehicleModelInfo::PrepareAtomicCB", 0x005355B0, &PrepareAtomicCB);
VEHICLEMI17_REVERSED("CVehicleModelInfo::PrepareAtomics", 0x005355D0, &PrepareAtomics);
VEHICLEMI17_REVERSED("CVehicleModelInfo::SetVehicleColour", 0x00535680, &SetVehicleColour);
VEHICLEMI17_REVERSED("CVehicleModelInfo::LoadVehicleColours", 0x00535700, &LoadVehicleColours);
VEHICLEMI17_REVERSED("CVehicleModelInfo::PreprocessHierarchy", 0x005359F0, &PreprocessHierarchy);
VEHICLEMI17_REVERSED("CVehicleModelInfo::SetBytes54", 0x00535FB0, &SetBytes54);
} // namespace vehiclemi17
