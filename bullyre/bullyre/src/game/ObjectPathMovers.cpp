// Objects moving along paths (the scripts' path followers: NativesObjects.cpp): 32
// entries of an entity (registered), a looping sound, its path reference (PathNetwork.cpp),
// a script callback for the path's nodes, and a spin about its up axis. 0x5EB910..0x5EC570.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct PathReference;

struct PathMover {
    re::Ptr32<uint8_t> entity;          // +0x0 (registered)
    re::Ptr32<void> sound;              // +0x4 (vf 0x34 stops it)
    re::Ptr32<PathReference> path;      // +0x8
    uint8_t alignToPath;      // +0xC
    uint8_t clearWhenDone;    // +0xD
    uint8_t padE[2];
    uint8_t callback[4];      // +0x10 (a script function reference)
    float heading;            // +0x14 (the entity's own, put back when it stops)
    float pitch;              // +0x18
    float spinTime;           // +0x1C
    float spin;               // +0x20
    float spinTarget;         // +0x24
    float spinRate;           // +0x28
};
static_assert(offsetof(PathMover, callback) == 0x10);
static_assert(sizeof(PathMover) == 0x2C);

struct CPathMovers {
    PathMover movers[32];     // +0x0
    int32_t count;            // +0x580
};
static_assert(offsetof(CPathMovers, count) == 0x580);

// A path reference (see PathNetwork.cpp)
struct PathReference {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x0
    re::Ptr32<uint8_t> path;            // +0x4
    re::Ptr32<void> owner;              // +0x8 (vf4 removes the reference)
    uint8_t atEnd;            // +0xC
    uint8_t padD[3];
    int32_t segment;          // +0x10
    float speed;              // +0x14
    float along;              // +0x18
};

constexpr uintptr_t kPathReferenceVtable = 0x0092CE50;
GAME_VAR(float, pm_speedScale, 0x00AEB94C);
GAME_VAR(float, pm_timeStep, 0x00C1A9A4);
GAME_VAR(int32_t, pm_skipModel, 0x00BF415C);
GAME_ARRAY(uint8_t*, pm_modelInfos, 0x00C67738);
inline void* const pm_soundManager = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const pm_scripts = reinterpret_cast<void*>(0x00D02850);
inline void* const pm_propTable = reinterpret_cast<void*>(0x020C7E98);

GAME_FN(void, __cdecl, PM_Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, PM_EntityRefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(float*, __thiscall, PM_GetForward, 0x004154A0, void* placeable, float* out);
GAME_FN(float, __cdecl, PM_PitchOf, 0x00433490, const float* v);
GAME_FN(void, __thiscall, PM_SetOrientation, 0x0046A690, void* placeable, float x, float y, float z);
GAME_FN(void, __cdecl, PM_RemoveFromWorld, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, PM_AddToWorld, 0x0045D6B0, void* entity);
GAME_FN(void, __thiscall, PM_SetMatrix, 0x0046DE90, void* placeable, const void* matrix, int);
GAME_FN(void, __thiscall, PM_UpdateRw, 0x00443AA0, void* entity);
GAME_FN(void*, __cdecl, PM_RwObjectFrame, 0x006C7370, void* object);
GAME_FN(void, __cdecl, PM_FrameDirty, 0x006C9AF0, void* frame);
GAME_FN(void, __thiscall, PM_Matrix_SetIdentity, 0x00412770, void* matrix);
GAME_FN(void, __thiscall, PM_Matrix_SetRotate, 0x00412D00, void* matrix, const float* angles);
GAME_FN(void, __thiscall, PM_Matrix_RotateZ, 0x00413220, void* matrix, float angle);
GAME_FN(void, __thiscall, PM_Matrix_RotateEuler, 0x00413320, void* matrix, const float* angles);
GAME_FN(void*, __thiscall, PM_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(void, __thiscall, PM_Matrix_Copy, 0x00412A10, void* self, const void* other);
GAME_FN(void, __cdecl, PM_MoveAttached, 0x00460E10, void* entity, const void* oldMatrix, const void* newMatrix);
GAME_FN(uint8_t*, __thiscall, PM_RefGet, 0x00773DB0, void* ref);
GAME_FN(void, __thiscall, PM_PropAnim_SetState, 0x004EA450, void* prop, int, int);
GAME_FN(uint8_t*, __thiscall, PM_ModelInfo_5201D0, 0x005201D0, void* modelInfo);
GAME_FN(int, __thiscall, PM_Sound_Find, 0x00595B80, void* manager, const char* name);
GAME_FN(void*, __thiscall, PM_Sound_Play, 0x0059B8A0, void* manager, int sound, const float* pos, int, const float* pos2,
        int, int, int, int, uintptr_t, void* entity, int, int, float, int);
GAME_FN(uint8_t, __thiscall, PM_Callback_IsSet, 0x005D80E0, void* callback);
GAME_FN(int, __thiscall, PM_Callback_Function, 0x005D80B0, void* callback);
GAME_FN(int, __thiscall, PM_Callback_State, 0x005D9C20, void* callback);
GAME_FN(int, __thiscall, PM_Callback_Env, 0x005D80D0, void* callback);
GAME_FN(int, __thiscall, PM_Scripts_Current, 0x005DB960, void* scripts);
GAME_FN(int, __thiscall, PM_Scripts_Find, 0x005DB8F0, void* scripts, int function);
GAME_FN(void, __thiscall, PM_Scripts_SetCurrent, 0x005DBDA0, void* scripts, uintptr_t script);
GAME_FN(void, __cdecl, PM_Lua_Push5D8270, 0x005D8270, uintptr_t state);
GAME_FN(void, __cdecl, PM_Lua_Push5D8670, 0x005D8670, uintptr_t state, int env);
GAME_FN(void, __cdecl, PM_Lua_PushInt, 0x005D85C0, uintptr_t state, int value);
GAME_FN(void, __cdecl, PM_Lua_Call, 0x005D82A0, uintptr_t state, int args, int results);
GAME_FN(int, __thiscall, PM_PropTable_Index, 0x006D79C0, void* table, void* prop);
GAME_FN(uint32_t, __thiscall, PM_Path_NodeCount, 0x005EC670, const uint8_t* path);
GAME_FN(void, __thiscall, PM_Ref_Orientation, 0x005ED680, PathReference* self, float* angles);
GAME_FN(void, __thiscall, PM_Ref_Position, 0x005ED5D0, PathReference* self, float* out);
GAME_FN(void, __thiscall, PM_Ref_Reset, 0x005ED850, PathReference* self);
GAME_FN(uint8_t, __thiscall, PM_Ref_SetSpeed, 0x005ED560, PathReference* self, float speed);
GAME_FN(void, __thiscall, PM_Ref_Advance, 0x005ED8B0, PathReference* self, float step, uint8_t* reached);
GAME_FN(int, __thiscall, PM_Ref_PathIndex, 0x005ED590, PathReference* self);
GAME_FN(int, __thiscall, PM_Ref_NodeIndex, 0x005ED5B0, PathReference* self);
// this file's
GAME_FN(PathMover*, __thiscall, PM_FindFree, 0x005EB9E0, CPathMovers* self);
GAME_FN(float*, __thiscall, PM_GetMoverMatrix, 0x005EBA70, PathMover* self, float* matrix);
GAME_FN(void, __thiscall, PM_SetSound, 0x005EBBE0, PathMover* self, const char* name);
GAME_FN(void, __thiscall, PM_Start, 0x005EBEC0, PathMover* self, uint8_t* entity, PathReference* path, int align);
GAME_FN(void, __thiscall, PM_Stop, 0x005EC040, PathMover* self, int restore);
GAME_FN(void, __thiscall, PM_Update, 0x005EC200, PathMover* self);

const float* EntityPos(const uint8_t* entity) {
    const uint8_t* matrix = *reinterpret_cast<const re::Ptr32<uint8_t>*>(entity + 0x14);
    return matrix ? reinterpret_cast<const float*>(matrix + 0x30) : reinterpret_cast<const float*>(entity + 4);
}
bool PathUsable(const PathReference* ref) {
    return ref && ref->path && PM_Path_NodeCount(ref->path) >= 2;
}
// a path reference's destructor and delete (inline in the game)
void DeleteReference(PathReference* ref) {
    ref->vtable = reinterpret_cast<re::Ptr32<void>*>(kPathReferenceVtable);
    if (ref->path && ref->owner) {
        using Fn = void(__thiscall*)(void*, void*);
        re::PtrCast<Fn>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(ref->owner))[1])(ref->owner, ref);
    }
    ref->path = nullptr;
    PM_Free(ref);
}
void StopSound(PathMover* m) {
    if (m->sound) {
        using Fn = void(__thiscall*)(void*);
        re::PtrCast<Fn>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(m->sound))[0x34 / 4])(m->sound);
        m->sound = nullptr;
    }
}
// the entity's frame marked dirty (its skeleton's, else its clump's or atomic's)
void DirtyFrame(uint8_t* entity) {
    uint8_t* anim = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x114);
    void* frame = nullptr;
    if (anim) {
        if (*reinterpret_cast<re::Ptr32<void>*>(anim + 0x94) == nullptr) {
            return;
        }
        frame = *reinterpret_cast<re::Ptr32<void>*>(anim + 0xC);
    } else {
        uint8_t* object = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x18);
        if (!object || (object[0] != 1 && object[0] != 2)) {
            return;
        }
        frame = PM_RwObjectFrame(object);
        if (!frame) {
            return;
        }
    }
    PM_FrameDirty(frame);
}
PathMover* Find(CPathMovers* self, const void* entity) {
    for (int i = 0; i < self->count; i++) {
        if (self->movers[i].entity == entity) {
            return &self->movers[i];
        }
    }
    return nullptr;
}

// 0x5EB910 (thiscall, a path reference's vf0): the scalar deleting destructor
PathReference* __fastcall Reference_Delete(PathReference* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(kPathReferenceVtable);
    if (self->path && self->owner) {
        using Fn = void(__thiscall*)(void*, void*);
        re::PtrCast<Fn>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(self->owner))[1])(self->owner, self);
    }
    self->path = nullptr;
    if (flags & 1) {
        PM_Free(self);
    }
    return self;
}

// 0x5EB950 (thiscall): the constructor
CPathMovers* __fastcall Construct(CPathMovers* self) {
    for (int i = 0; i < 32; i++) {
        PathMover& m = self->movers[i];
        m.entity = nullptr;
        PM_EntityRefSet(&m.entity, nullptr);
        m.sound = nullptr;
        m.path = nullptr;
        m.alignToPath = 0;
        m.clearWhenDone = 0;
        *reinterpret_cast<uint32_t*>(m.callback) = 0;
        m.heading = 0.0f;
        m.pitch = 0.0f;
        m.spinTime = 0.0f;
        m.spin = 0.0f;
        m.spinTarget = 0.0f;
        m.spinRate = 0.0f;
    }
    return self;
}

// 0x5EB9A0 (thiscall): the entity's entry
PathMover* __fastcall FindEntry(CPathMovers* self, re::Edx, void* entity) {
    return Find(self, entity);
}

// 0x5EB9E0 (thiscall): a free entry (one more while under 32)
PathMover* __fastcall FindFree(CPathMovers* self) {
    int count = self->count;
    for (int i = 0; i < count; i++) {
        if (self->movers[i].entity == nullptr) {
            return &self->movers[i];
        }
    }
    if (count > 31) {
        return nullptr;
    }
    self->count = count + 1;
    return &self->movers[count];
}

// 0x5EBA30 (thiscall): the entity's callback (0x10 when it has no entry)
void* __fastcall GetCallback(CPathMovers* self, re::Edx, void* entity) {
    PathMover* m = Find(self, entity);
    return m ? static_cast<void*>(m->callback) : reinterpret_cast<void*>(0x10);
}

// 0x5EBA70 (thiscall): the entity's matrix: its heading and pitch, the spin, the path's
// orientation (when aligning) and position (else its own position)
float* __fastcall GetMoverMatrix(PathMover* self, re::Edx, float* matrix) {
    *reinterpret_cast<uint32_t*>(matrix + 0x10) = 0;
    PM_Matrix_SetIdentity(matrix);
    if (!self->entity) {
        return matrix;
    }
    float angles[3] = {self->pitch, 0.0f, self->heading};
    PM_Matrix_SetRotate(matrix, angles);
    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 0.0f;
    if (0.0f != self->spin) {
        PM_Matrix_RotateZ(matrix, self->spin);
    }
    if (PathUsable(self->path)) {
        if (self->alignToPath) {
            float orientation[3];
            PM_Ref_Orientation(self->path, orientation);
            PM_Matrix_RotateEuler(matrix, orientation);
        }
        float pos[3];
        PM_Ref_Position(self->path, pos);
        matrix[12] = matrix[12] + pos[0];
        matrix[13] = matrix[13] + pos[1];
        matrix[14] = matrix[14] + pos[2];
        return matrix;
    }
    const float* at = EntityPos(self->entity);
    float x = at[0];
    float y = at[1];
    float z = at[2];
    matrix[12] = matrix[12] + x;
    matrix[13] = y + matrix[13];
    matrix[14] = matrix[14] + z;
    return matrix;
}

// 0x5EBBE0 (thiscall): the looping sound (none for null)
void __fastcall SetSound(PathMover* self, re::Edx, const char* name) {
    if (!name) {
        if (self->sound) {
            StopSound(self);
            self->sound = nullptr;
        }
        return;
    }
    StopSound(self);
    const float* pos = EntityPos(self->entity);
    int sound = PM_Sound_Find(pm_soundManager, name);
    self->sound = PM_Sound_Play(pm_soundManager, sound, pos, 9, pos, 0, 0, 0, 0, 0, self->entity, 6, 1, 1.0f, 0);
}

// 0x5EBC70 (thiscall): has the entity an entry?
uint8_t __fastcall Has(CPathMovers* self, re::Edx, void* entity) {
    return Find(self, entity) != nullptr;
}

// 0x5EBCB0 (thiscall): the entity's path back to its start
void __fastcall Reset(CPathMovers* self, re::Edx, void* entity) {
    PathMover* m = Find(self, entity);
    if (m && m->path) {
        PM_Ref_Reset(m->path);
    }
}

// 0x5EBD00 (thiscall): the entity's speed along its path (0 without an entry)
float __fastcall GetSpeed(CPathMovers* self, re::Edx, void* entity) {
    PathMover* m = Find(self, entity);
    float speed = 0.0f;
    if (m) {
        speed = m->path->speed;
    }
    return speed;
}

// 0x5EBD50 (thiscall): the entity's speed along its path
void __fastcall SetSpeed(CPathMovers* self, re::Edx, void* entity, float speed) {
    PathMover* m = Find(self, entity);
    if (m && PathUsable(m->path)) {
        PM_Ref_SetSpeed(m->path, speed);
    }
}

// 0x5EBDC0 (thiscall): where the entity is (on its path, else its own position)
void __fastcall GetPosition(CPathMovers* self, re::Edx, void* entity, float* out) {
    PathMover* m = Find(self, entity);
    if (!m) {
        return;
    }
    if (m->path) {
        PM_Ref_Position(m->path, out);
        return;
    }
    const float* at = EntityPos(m->entity);
    out[0] = at[0];
    out[1] = at[1];
    out[2] = at[2];
}

// 0x5EBE30 (thiscall): stop at the path's end
void __fastcall SetClearWhenDone(CPathMovers* self, re::Edx, void* entity, int clear) {
    PathMover* m = Find(self, entity);
    if (m) {
        m->clearWhenDone = uint8_t(clear);
    }
}

// 0x5EBE70 (thiscall): the entity's looping sound
void __fastcall SetSoundLoop(CPathMovers* self, re::Edx, void* entity, const char* name) {
    PathMover* m = Find(self, entity);
    if (m) {
        PM_SetSound(m, name);
    }
}

// 0x5EBEC0 (thiscall): an entity put on a path (its heading and pitch kept to put back)
void __fastcall Start(PathMover* self, re::Edx, uint8_t* entity, PathReference* path, int align) {
    if (self->path) {
        DeleteReference(self->path);
        self->path = nullptr;
    }
    if (self->entity == nullptr) {
        float forward[3];
        PM_GetForward(entity, forward);
        self->pitch = PM_PitchOf(forward);
        const uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x14);
        if (matrix) {
            float x = -*reinterpret_cast<const float*>(matrix + 0x10);
            float y = *reinterpret_cast<const float*>(matrix + 0x14);
            self->heading = float(std::atan2(double(x), double(y)));
        } else {
            self->heading = *reinterpret_cast<float*>(entity + 0x10);
        }
    } else {
        PM_SetOrientation(self->entity, self->pitch, 0.0f, self->heading);
    }
    self->spinTime = 0.0f;
    self->spin = 0.0f;
    self->spinTarget = 0.0f;
    self->spinRate = 0.0f;
    PM_EntityRefSet(&self->entity, entity);
    PM_RemoveFromWorld(self->entity);
    self->alignToPath = uint8_t(align);
    self->path = path;
    self->clearWhenDone = 0;
    StopSound(self);
    uint8_t* e = self->entity;
    alignas(16) float matrix[0x12];
    PM_SetMatrix(e, PM_GetMoverMatrix(self, matrix), 0);
    PM_UpdateRw(self->entity);
    DirtyFrame(self->entity);
    PM_AddToWorld(self->entity);
}

// 0x5EC040 (thiscall): the entry emptied (restore: the entity's heading and pitch back)
void __fastcall Stop(PathMover* self, re::Edx, int restore) {
    if (self->path) {
        DeleteReference(self->path);
        self->path = nullptr;
    }
    if (self->entity && uint8_t(restore) != 0) {
        PM_SetOrientation(self->entity, self->pitch, 0.0f, self->heading);
    }
    PM_EntityRefSet(&self->entity, nullptr);
    self->heading = 0.0f;
    self->pitch = 0.0f;
    self->spinTime = 0.0f;
    self->clearWhenDone = 0;
    self->spin = 0.0f;
    self->spinTarget = 0.0f;
    self->spinRate = 0.0f;
    StopSound(self);
}

// 0x5EC0E0 (thiscall): the entity's entry stopped (its orientation not put back)
void __fastcall StopEntity(CPathMovers* self, re::Edx, void* entity) {
    PathMover* m = Find(self, entity);
    if (m) {
        PM_Stop(m, 0);
    }
}

// 0x5EC130 (thiscall): an entity put on a path (its entry or a free one)
void __fastcall StartEntity(CPathMovers* self, re::Edx, uint8_t* entity, PathReference* path, int align) {
    PathMover* m = Find(self, entity);
    if (!m) {
        m = PM_FindFree(self);
        if (!m) {
            return;
        }
    }
    PM_Start(m, entity, path, align);
}

// 0x5EC180 (thiscall): an entity spun to a target angle at a rate (degrees)
void __fastcall SetSpin(CPathMovers* self, re::Edx, uint8_t* entity, float target, float rate) {
    PathMover* m = Find(self, entity);
    if (!m) {
        m = PM_FindFree(self);
        if (!m) {
            return;
        }
        PM_Start(m, entity, nullptr, 0);
    }
    m->spinTime = 0.0f;
    m->spin = 0.0f;
    m->spinTarget = float(target * 3.1415927410125732 / 180.0);
    m->spinRate = float(3.1415927410125732 * rate / 180.0);
}

// 0x5EC200 (thiscall): a frame: along the path and the spin, the entity moved (its
// attached things with it), the script's callback run at a node, stopped at the end
void __fastcall Update(PathMover* self) {
    PM_RemoveFromWorld(self->entity);
    uint8_t reached = 0;
    if (self->path) {
        PM_Ref_Advance(self->path, float(pm_speedScale * pm_timeStep), &reached);
    }
    if (0.0f != self->spinRate) {
        float previous = self->spin;
        float step = float(pm_speedScale * pm_timeStep);
        self->spinTime = step + self->spinTime;
        float step2 = float(pm_speedScale * pm_timeStep);
        self->spin = float(double(step2) * self->spinRate + self->spin);
        float target = std::fabs(self->spinTarget);
        float before = std::fabs(previous);
        if (target > before) {
            float now = std::fabs(self->spin);
            if (now > target) {
                self->spin = self->spinTarget;
            }
        }
    }
    alignas(16) float matrix[0x12];
    PM_GetMoverMatrix(self, matrix);
    uint8_t* entity = self->entity;
    if (*reinterpret_cast<int16_t*>(entity + 0x10E) != pm_skipModel) {
        bool move = true;
        if ((entity[0x108] & 7) == 6) {
            uint8_t* prop = PM_RefGet(self);
            PM_PropAnim_SetState(prop, 10, 0);
            *reinterpret_cast<uint32_t*>(prop + 0xE4) = 1;
            uint8_t* info = PM_ModelInfo_5201D0(pm_modelInfos[*reinterpret_cast<int16_t*>(prop + 0x10E)]);
            move = info[0x84] != 0;
            entity = self->entity;
        }
        if (move) {
            alignas(16) float old[0x12];
            PM_Matrix_Copy(old, PM_GetMatrix(entity));
            PM_MoveAttached(self->entity, old, matrix);
        }
    }
    PM_SetMatrix(self->entity, matrix, 0);
    PM_UpdateRw(self->entity);
    DirtyFrame(self->entity);
    PM_AddToWorld(self->entity);
    if (reached && PM_Callback_IsSet(self->callback)) {
        int current = PM_Scripts_Current(pm_scripts);
        int script = PM_Scripts_Find(pm_scripts, PM_Callback_Function(self->callback));
        if (script) {
            PM_Scripts_SetCurrent(pm_scripts, script);
            int state = PM_Callback_State(self->callback);
            PM_Lua_Push5D8270(state);
            PM_Lua_Push5D8670(state, PM_Callback_Env(self->callback));
            int propIndex = -1;
            if ((self->entity[0x108] & 7) == 6) {
                uint8_t* prop = PM_RefGet(self);
                uint8_t* anim = *reinterpret_cast<re::Ptr32<uint8_t>*>(prop + 0x23C);
                propIndex = PM_PropTable_Index(pm_propTable, *reinterpret_cast<re::Ptr32<void>*>(anim + 0x2C));
            }
            int pathIndex = -1;
            int nodeIndex = -1;
            if (self->path) {
                pathIndex = PM_Ref_PathIndex(self->path);
                nodeIndex = PM_Ref_NodeIndex(self->path);
            }
            PM_Lua_PushInt(state, propIndex);
            PM_Lua_PushInt(state, pathIndex);
            PM_Lua_PushInt(state, nodeIndex);
            PM_Lua_Call(state, 3, 0);
            PM_Scripts_SetCurrent(pm_scripts, current);
        }
    }
    if (self->clearWhenDone && self->spin == self->spinTarget) {
        PM_Stop(self, 0);
    }
}

// 0x5EC500 (thiscall): every entry's frame; all emptied when none is in use
void __fastcall UpdateAll(CPathMovers* self) {
    if (self->count <= 0) {
        return;
    }
    bool none = true;
    for (int i = 0; i < self->count; i++) {
        if (self->movers[i].entity) {
            PM_Update(&self->movers[i]);
            none = false;
        }
    }
    if (!none) {
        return;
    }
    for (int i = 0; i < 32; i++) {
        PM_Stop(&self->movers[i], 1);
    }
    self->count = 0;
}

// 0x5EC570 (thiscall): a path node's constructor (all zero but its id, +0x18)
uint32_t* __fastcall Node_Construct(uint32_t* self) {
    for (int i = 0; i < 9; i++) {
        if (i != 6) {
            self[i] = 0;
        }
    }
    return self;
}

#define PATHMOVER_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(PathMoverOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(PathMoverOriginal_, __LINE__), \
             re::Mode::Original)

PATHMOVER_REVERSED("PathReference::Delete", 0x005EB910, &Reference_Delete);
PATHMOVER_REVERSED("CPathMovers::Construct", 0x005EB950, &Construct);
PATHMOVER_REVERSED("CPathMovers::FindEntry", 0x005EB9A0, &FindEntry);
PATHMOVER_REVERSED("CPathMovers::FindFree", 0x005EB9E0, &FindFree);
PATHMOVER_REVERSED("CPathMovers::GetCallback", 0x005EBA30, &GetCallback);
PATHMOVER_REVERSED("PathMover::GetMatrix", 0x005EBA70, &GetMoverMatrix);
PATHMOVER_REVERSED("PathMover::SetSound", 0x005EBBE0, &SetSound);
PATHMOVER_REVERSED("CPathMovers::Has", 0x005EBC70, &Has);
PATHMOVER_REVERSED("CPathMovers::Reset", 0x005EBCB0, &Reset);
PATHMOVER_REVERSED("CPathMovers::GetSpeed", 0x005EBD00, &GetSpeed);
PATHMOVER_REVERSED("CPathMovers::SetSpeed", 0x005EBD50, &SetSpeed);
PATHMOVER_REVERSED("CPathMovers::GetPosition", 0x005EBDC0, &GetPosition);
PATHMOVER_REVERSED("CPathMovers::SetClearWhenDone", 0x005EBE30, &SetClearWhenDone);
PATHMOVER_REVERSED("CPathMovers::SetSoundLoop", 0x005EBE70, &SetSoundLoop);
PATHMOVER_REVERSED("PathMover::Start", 0x005EBEC0, &Start);
PATHMOVER_REVERSED("PathMover::Stop", 0x005EC040, &Stop);
PATHMOVER_REVERSED("CPathMovers::StopEntity", 0x005EC0E0, &StopEntity);
PATHMOVER_REVERSED("CPathMovers::StartEntity", 0x005EC130, &StartEntity);
PATHMOVER_REVERSED("CPathMovers::SetSpin", 0x005EC180, &SetSpin);
PATHMOVER_REVERSED("PathMover::Update", 0x005EC200, &Update);
PATHMOVER_REVERSED("CPathMovers::UpdateAll", 0x005EC500, &UpdateAll);
PATHMOVER_REVERSED("PathNode::Construct", 0x005EC570, &Node_Construct);

} // namespace
