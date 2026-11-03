// NiTask (vtable 0x965978, 0xC bytes: NiObject and a status) and NiGeomMorpherUpdateTask (vtable
// 0x9659D0, 0x14 bytes: the morpher controller and its target geometry, references), the task a
// geometry morpher's update is handed off as; and the pool the tasks come from
// (NiTObjectPool<NiGeomMorpherUpdateTask>, 0x18 bytes at ms_pkPool: the free tasks, the blocks
// made, the next block's size; locked by its critical section).
//
// Slots past NiObject's: 18 DoTask, 19 (true; shared), 20 Clear (NiTask's: whether it's done).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTask_RTTI, 0x020E94B4);
GAME_VAR(RTTI, NiGeomMorpherUpdateTask_RTTI, 0x020E4A54);

constexpr uintptr_t kTaskVtable = 0x00965978;
constexpr uintptr_t kGeomMorpherUpdateTaskVtable = 0x009659D0;
constexpr uintptr_t kGeomMorpherUpdateTaskConstruct = 0x007FFE40;

struct NiTask : NiObject {
    uint32_t status;   // +0x8 (3 completed, 4 aborted)
};
static_assert(sizeof(NiTask) == 0xC);

struct NiGeomMorpherUpdateTask : NiTask {
    re::Ptr32<NiRefObject> controller;   // +0xC (NiGeomMorpherControllerPtr)
    re::Ptr32<NiRefObject> target;       // +0x10 (its target, NiGeometryPtr)
};
static_assert(sizeof(NiGeomMorpherUpdateTask) == 0x14);

// a block of tasks the pool made (0xC bytes; the next one deleted with it)
struct PoolContainer {
    re::Ptr32<NiGeomMorpherUpdateTask> objects;   // +0x0 (NiNew[]: the count before them)
    uint32_t count;                     // +0x4
    re::Ptr32<PoolContainer> next;                // +0x8
};
static_assert(sizeof(PoolContainer) == 0xC);

struct NiTObjectPool {
    re::Ptr32<re::Ptr32<NiGeomMorpherUpdateTask>> freeObjects;   // +0x0 (NiTPrimitiveSet: base, allocated, count)
    uint32_t allocated;                      // +0x4
    uint32_t freeCount;                      // +0x8
    re::Ptr32<PoolContainer> containers;               // +0xC
    uint32_t growBy;                         // +0x10 (doubles each block)
    uint32_t initSize;                       // +0x14
};
static_assert(sizeof(NiTObjectPool) == 0x18);

GAME_VAR(NiTObjectPool*, ms_pkPool, 0x020E4A50);
GAME_VAR(NiCriticalSection, ms_kPoolCriticalSection, 0x020E4A88);

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, uint32_t size);     // the game's
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(void, __thiscall, Object_Construct, 0x00755260, void* self);   // NiObject()
GAME_FN(void, __thiscall, Object_Destruct, 0x007769D0, void* self);    // ~NiObject
GAME_FN(void, __thiscall, PoolContainer_DeletingDestructor, 0x008D2C80, PoolContainer* self, uint32_t flags);
GAME_FN(void, __thiscall, PrimitiveSet_Realloc, 0x0083AD20, void* set, uint32_t allocated);
GAME_FN(void, __thiscall, PrimitiveSet_Add, 0x00884380, void* set, const re::Ptr32<NiGeomMorpherUpdateTask>* value);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, Task_Destruct, 0x007FFE70, NiGeomMorpherUpdateTask* self);
GAME_FN(NiGeomMorpherUpdateTask*, __cdecl, Task_NewArray, 0x007FFF40, uint32_t count);
GAME_FN(void, __thiscall, Pool_Destruct, 0x00800050, NiTObjectPool* self);
GAME_FN(void, __thiscall, Pool_CreateNewObjects, 0x008000B0, NiTObjectPool* self, uint32_t count);
GAME_FN(void, __cdecl, Task_ReleaseObject, 0x008001A0, NiGeomMorpherUpdateTask* task);

inline void ReleaseImport(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// a reference slot assigned: the old one let go first, the new one stored, then held
inline void AssignReleasingFirst(re::Ptr32<NiRefObject>& slot, NiRefObject* value) {
    NiRefObject* old = slot;
    if (old == value) {
        return;
    }
    ReleaseImport(old);
    slot = value;
    if (value != nullptr) {
        Increment(&value->refCount);
    }
}

// a pool's blocks deleted (the first one's tasks with their array delete: the vector deleting
// destructor, flags 3; operator delete for an empty one; the rest through the block's own
// deleting destructor)
inline void DeleteContainers(PoolContainer* container) {
    NiGeomMorpherUpdateTask* objects = container->objects;
    if (objects != nullptr) {
        uint32_t* block = reinterpret_cast<uint32_t*>(objects) - 1;
        if (*block == 0) {
            OperatorDelete(block);
        } else {
            Virtual<void>(objects, 0, 3u);
        }
    }
    if (container->next != nullptr) {
        PoolContainer_DeletingDestructor(container->next, 1);
    }
    NiDelete(container, 0xC);
}

// ---- NiTask

// 0x7FFDB0 GetRTTI
const RTTI* __fastcall TaskGetRTTI(NiTask*, re::Edx) {
    return &NiTask_RTTI;
}

// 0x7FFDD0 Clear (slot 20): whether it's completed or aborted
uint8_t __fastcall TaskClear(NiTask* self, re::Edx) {
    return (self->status == 4 || self->status == 3) ? 1 : 0;
}

// 0x7FFE10 the deleting destructor
NiTask* __fastcall TaskDeletingDestructor(NiTask* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kTaskVtable);
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xC);
    }
    return self;
}

// ---- NiGeomMorpherUpdateTask

// 0x7FFDF0 DoTask (slot 18): the controller's mesh update (its slot 25) when the target is still
// referenced elsewhere, then Clear (slot 20, its result returned)
uint8_t __fastcall DoTask(NiGeomMorpherUpdateTask* self, re::Edx) {
    if (self->target->refCount > 1) {
        Virtual<void>(self->controller, 25);
    }
    return Virtual<uint8_t>(self, 20);
}

// 0x7FFE40 NiGeomMorpherUpdateTask()
NiGeomMorpherUpdateTask* __fastcall Construct(NiGeomMorpherUpdateTask* self, re::Edx) {
    Object_Construct(self);
    self->status = 0;
    self->vtable = reinterpret_cast<void*>(kGeomMorpherUpdateTaskVtable);
    self->controller = nullptr;
    self->target = nullptr;
    return self;
}

// 0x7FFE60 GetRTTI
const RTTI* __fastcall GetRTTI(NiGeomMorpherUpdateTask*, re::Edx) {
    return &NiGeomMorpherUpdateTask_RTTI;
}

// 0x7FFE70 ~NiGeomMorpherUpdateTask
void __fastcall Destruct(NiGeomMorpherUpdateTask* self, re::Edx) {
    ReleaseImport(self->target);
    ReleaseImport(self->controller);
    self->vtable = reinterpret_cast<void*>(kTaskVtable);
    Object_Destruct(self);
}

// 0x7FFEC0 SetController(controller): it and its target held
void __fastcall SetController(NiGeomMorpherUpdateTask* self, re::Edx, NiRefObject* controller) {
    AssignReleasingFirst(self->controller, controller);
    AssignReleasingFirst(self->target, *reinterpret_cast<re::Ptr32<NiRefObject>*>(reinterpret_cast<uint8_t*>(controller) + 0x2C));
}

// 0x7FFF40 NiNew[] of count tasks (for none, the game's operator new of the count alone)
NiGeomMorpherUpdateTask* __cdecl NewArray(uint32_t count) {
    uint32_t* block;
    if (count == 0) {
        block = static_cast<uint32_t*>(OperatorNew(4));
    } else {
        uint64_t product = uint64_t(count) * 0x14;
        uint32_t bytes = (product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product);
        block = static_cast<uint32_t*>(NiNewArray(bytes > 0xFFFFFFFB ? 0xFFFFFFFF : bytes + 4));
    }
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, 0x14, count, reinterpret_cast<void*>(kGeomMorpherUpdateTaskConstruct));
    return reinterpret_cast<NiGeomMorpherUpdateTask*>(block + 1);
}

// 0x7FFFA0 the vector deleting destructor: flags 2 an array (each task destroyed from the last,
// the block freed with flags 1), else the one
void* __fastcall VectorDeletingDestructor(NiGeomMorpherUpdateTask* self, re::Edx, uint32_t flags) {
    if ((flags & 2) != 0) {
        uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
        for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
            NiGeomMorpherUpdateTask* task = &self[i];
            ReleaseImport(task->target);
            ReleaseImport(task->controller);
            task->vtable = reinterpret_cast<void*>(kTaskVtable);
            Object_Destruct(task);
        }
        if ((flags & 1) != 0) {
            NiDeleteArray(block, 0x14);
        }
        return block;
    }
    Task_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x14);
    }
    return self;
}

// 0x800200 Clear (slot 20): the references let go, the task back to the pool
uint8_t __fastcall Clear(NiGeomMorpherUpdateTask* self, re::Edx) {
    if (self->controller != nullptr) {
        ReleaseImport(self->controller);
        self->controller = nullptr;
    }
    if (self->target != nullptr) {
        ReleaseImport(self->target);
        self->target = nullptr;
    }
    Task_ReleaseObject(self);
    return 1;
}

// ---- the pool

// 0x800050 ~NiTObjectPool: no free tasks, the blocks deleted, the free set's array freed
void __fastcall PoolDestruct(NiTObjectPool* self, re::Edx) {
    PoolContainer* container = self->containers;
    self->freeCount = 0;
    if (container != nullptr) {
        DeleteContainers(container);
    }
    NiFree(self->freeObjects);
}

// 0x8000B0 CreateNewObjects(count): a block of count tasks, each put in the free set (grown to
// twice its size, or 1, when full); the block put first on the list
void __fastcall PoolCreateNewObjects(NiTObjectPool* self, re::Edx, uint32_t count) {
    auto* container = static_cast<PoolContainer*>(NiNew(0xC));
    if (container != nullptr) {
        container->count = count;
        container->next = nullptr;
        container->objects = count != 0 ? Task_NewArray(count) : nullptr;
    }
    // (on a failed allocation the original reads through the null block)
    for (uint32_t i = 0; i < count; i++) {
        NiGeomMorpherUpdateTask* task = i < container->count ? &container->objects[i] : nullptr;
        if (self->freeCount == self->allocated) {
            PrimitiveSet_Realloc(self, self->allocated != 0 ? self->allocated * 2 : 1);
        }
        self->freeObjects[self->freeCount] = task;
        self->freeCount++;
    }
    container->next = self->containers;
    self->containers = container;
}

// 0x800160 _SDMInit: the pool (blocks of 8 at first)
void __cdecl SDMInit() {
    auto* pool = static_cast<NiTObjectPool*>(NiNew(0x18));
    if (pool == nullptr) {
        ms_pkPool = nullptr;
        return;
    }
    pool->freeObjects = nullptr;
    pool->allocated = 0;
    pool->freeCount = 0;
    pool->containers = nullptr;
    pool->growBy = 8;
    pool->initSize = 8;
    ms_pkPool = pool;
}

// 0x8001A0 ReleaseObject(task): back in the free set, under the lock
void __cdecl ReleaseObject(re::Ptr32<NiGeomMorpherUpdateTask> task) {
    Lock(ms_kPoolCriticalSection);
    PrimitiveSet_Add(ms_pkPool, &task);
    Unlock(ms_kPoolCriticalSection);
}

// 0x800260 _SDMShutdown: the blocks deleted, the pool reset and destroyed (the pointer kept)
void __cdecl SDMShutdown() {
    NiTObjectPool* pool = ms_pkPool;
    PoolContainer* container = pool->containers;
    pool->freeCount = 0;
    if (container != nullptr) {
        DeleteContainers(container);
    }
    pool->containers = nullptr;
    pool->growBy = pool->initSize;
    pool = ms_pkPool;
    if (pool != nullptr) {
        Pool_Destruct(pool);
        NiDelete(pool, 0x18);
    }
}

// 0x8002E0 GetFreeObject: the first free task (a new block of growBy made when there's none, the
// next one twice as big), its slot filled with the last
NiGeomMorpherUpdateTask* __cdecl GetFreeObject() {
    Lock(ms_kPoolCriticalSection);
    NiTObjectPool* pool = ms_pkPool;
    if (pool->freeCount == 0) {
        Pool_CreateNewObjects(pool, pool->growBy);
        pool->growBy *= 2;
    }
    re::Ptr32<NiGeomMorpherUpdateTask>* free = pool->freeObjects;
    NiGeomMorpherUpdateTask* task = free[0];
    pool->freeCount--;
    free[0] = free[pool->freeCount];
    Unlock(ms_kPoolCriticalSection);
    return task;
}

} // namespace

// 0x7FFDC0: mov al, 1; ret (3 bytes; slot 19, shared with other classes)
// 0x007FFDC0: [b0 01 c3 cc cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __cdecl Tiny_007FFDC0() {
    return 1;
}
RE_UNHOOKED_FN("NiTask::ReturnTrue", 0x007FFDC0, &Tiny_007FFDC0);

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIGEOMTASK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIGEOMTASK_REVERSED("NiTask::GetRTTI", 0x007FFDB0, ni::TaskGetRTTI);
NIGEOMTASK_REVERSED("NiTask::Clear", 0x007FFDD0, ni::TaskClear);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::DoTask", 0x007FFDF0, ni::DoTask);
NIGEOMTASK_REVERSED("NiTask::DeletingDestructor", 0x007FFE10, ni::TaskDeletingDestructor);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::NiGeomMorpherUpdateTask", 0x007FFE40, ni::Construct);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::GetRTTI", 0x007FFE60, ni::GetRTTI);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::~NiGeomMorpherUpdateTask", 0x007FFE70, ni::Destruct);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::SetController", 0x007FFEC0, ni::SetController);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::NewArray", 0x007FFF40, ni::NewArray);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::VectorDeletingDestructor", 0x007FFFA0, ni::VectorDeletingDestructor);
NIGEOMTASK_REVERSED("NiTObjectPool<NiGeomMorpherUpdateTask>::~NiTObjectPool", 0x00800050, ni::PoolDestruct);
NIGEOMTASK_REVERSED("NiTObjectPool<NiGeomMorpherUpdateTask>::CreateNewObjects", 0x008000B0, ni::PoolCreateNewObjects);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::_SDMInit", 0x00800160, ni::SDMInit);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::ReleaseObject", 0x008001A0, ni::ReleaseObject);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::Clear", 0x00800200, ni::Clear);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::_SDMShutdown", 0x00800260, ni::SDMShutdown);
NIGEOMTASK_REVERSED("NiGeomMorpherUpdateTask::GetFreeObject", 0x008002E0, ni::GetFreeObject);
