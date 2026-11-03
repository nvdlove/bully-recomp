// The ambient vehicle infos: CAmbientVehicleInfo (0x1C bytes, vtable 0x912624; a
// CObjectNumbersInfo of 3, archivable through its IArchivable at +0x18, vtable
// 0x912614), four of them in a CAreaAmbientVehicleInfo (0x74 bytes, in its own pool at
// 0xC2D15C, 11 slots).

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

namespace {

using re::At;

struct CAmbientVehicleInfo {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x0
    int32_t unknown4;         // +0x4 (read with the key at 0xA356A8)
    uint8_t seats[0xC];       // +0x8 a byte a seat kind (read by the seat kinds' names)
    int32_t numSeats;         // +0x14
    re::Ptr32<re::Ptr32<void>> archivable;        // +0x18 IArchivable
};
static_assert(offsetof(CAmbientVehicleInfo, numSeats) == 0x14);
static_assert(offsetof(CAmbientVehicleInfo, archivable) == 0x18);
static_assert(sizeof(CAmbientVehicleInfo) == 0x1C);

constexpr uintptr_t vtblAmbientVehicleInfo = 0x00912624;
constexpr uintptr_t vtblAmbientVehicleInfoArchivable = 0x00912614;
constexpr uintptr_t vtblIArchivable = 0x00900CDC;

GAME_VAR(const char*, keyInfo0, 0x00A35698);
GAME_VAR(const char*, keyInfo1, 0x00A356A0);
GAME_VAR(const char*, keyInfo2, 0x00A3569C);
GAME_VAR(const char*, keyInfo3, 0x00A356A4);
GAME_VAR(const char*, keyUnknown4, 0x00A356A8);
GAME_VAR(CPool<uint8_t>*, areaInfoPool, 0x00C2D15C);

GAME_FN(void, __thiscall, ObjectNumbersInfo_Construct, 0x004645C0, void* self, int count);
GAME_FN(void, __thiscall, ObjectNumbersInfo_Destruct, 0x004645E0, void* self);
GAME_FN(void, __thiscall, DataReader_Next, 0x005B0720, void* reader, void* archivable, const char* key);
GAME_FN(void, __thiscall, DataWriter_Write, 0x005B0730, void* writer, void* archivable, const char* key);
GAME_FN(const char*, __cdecl, SeatKindName, 0x004CE3C0, int kind);
GAME_FN(void, __thiscall, AreaInfo_Destruct, 0x0041A920, void* self);
GAME_FN(void, __thiscall, AreaInfo_Construct, 0x0041A8F0, void* self);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, size_t size, int count, void* constructor);
GAME_FN(uint8_t*, __thiscall, Pool_New, 0x00468AE0, CPool<uint8_t>* pool);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// (their own, called where modes apply)
GAME_FN(CAmbientVehicleInfo*, __thiscall, AVI_Destroy, 0x004A8170, CAmbientVehicleInfo* self, uint8_t flags);
GAME_FN(void, __thiscall, AreaInfoPool_Init, 0x004A8290, CPool<uint8_t>* pool);

// an archivable member of the area info (nullptr for a null member, as the game checks)
void* Archivable(uint8_t* self, int member) {
    uint8_t* info = self + member;
    return info ? info + 0x18 : nullptr;
}

} // namespace

namespace c09ambientvehicles {

// 0x4A7FD0: the constructor
CAmbientVehicleInfo* __fastcall Construct(CAmbientVehicleInfo* self, re::Edx) {
    ObjectNumbersInfo_Construct(self, 3);
    self->archivable = reinterpret_cast<re::Ptr32<void>*>(vtblIArchivable);
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(vtblAmbientVehicleInfo);
    self->archivable = reinterpret_cast<re::Ptr32<void>*>(vtblAmbientVehicleInfoArchivable);
    return self;
}

// 0x4A8000: the destructor's body
void __fastcall Destruct(CAmbientVehicleInfo* self, re::Edx) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(vtblAmbientVehicleInfo);
    self->archivable = reinterpret_cast<re::Ptr32<void>*>(vtblAmbientVehicleInfoArchivable);
    self->archivable = reinterpret_cast<re::Ptr32<void>*>(vtblIArchivable);
    ObjectNumbersInfo_Destruct(self);
}

// 0x4A8020 (vf1) / 0x4A80C0 (vf2): its four infos read / written through their
// IArchivables (the infos at +0x4, +0x20, +0x3C, +0x58)
void __fastcall AreaRead(uint8_t* self, re::Edx, void* reader) {
    DataReader_Next(reader, Archivable(self, 4), keyInfo0);
    DataReader_Next(reader, Archivable(self, 0x20), keyInfo1);
    DataReader_Next(reader, Archivable(self, 0x3C), keyInfo2);
    DataReader_Next(reader, Archivable(self, 0x58), keyInfo3);
}
void __fastcall AreaWrite(uint8_t* self, re::Edx, void* writer) {
    DataWriter_Write(writer, Archivable(self, 4), keyInfo0);
    DataWriter_Write(writer, Archivable(self, 0x20), keyInfo1);
    DataWriter_Write(writer, Archivable(self, 0x3C), keyInfo2);
    DataWriter_Write(writer, Archivable(self, 0x58), keyInfo3);
}

// 0x4A8160: vf0 through the IArchivable (this adjusted by -0x18)
CAmbientVehicleInfo* __fastcall DestroyFromArchivable(uint8_t* self, re::Edx, uint8_t flags) {
    return AVI_Destroy(reinterpret_cast<CAmbientVehicleInfo*>(self - 0x18), flags);
}

// 0x4A8170: vf0, the deleting destructor
CAmbientVehicleInfo* __fastcall Destroy(CAmbientVehicleInfo* self, re::Edx, uint8_t flags) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(vtblAmbientVehicleInfo);
    self->archivable = reinterpret_cast<re::Ptr32<void>*>(vtblAmbientVehicleInfoArchivable);
    self->archivable = reinterpret_cast<re::Ptr32<void>*>(vtblIArchivable);
    ObjectNumbersInfo_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x4A81B0: IArchivable's read (this is the IArchivable): +0x4, then a byte for each
// of the base's (vf1) kinds, by the seat kinds' names. (A value the reader doesn't write
// is the low byte of `this` in the game, which it's first set to)
void __fastcall ArchivableRead(uint8_t* self, re::Edx, void* reader) {
    using ReadInt = void(__thiscall*)(void*, void*, const char*);
    uint8_t* base = self - 0x18;
    re::PtrCast<ReadInt>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reader))[14])(reader, base + 4, keyUnknown4);
    using Count = int(__thiscall*)(void*);
    int count = re::PtrCast<Count>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(base))[1])(base);
    uint32_t value = uint32_t(reinterpret_cast<uintptr_t>(self));
    for (int i = 0; i < count; i++) {
        const char* name = SeatKindName(i);
        re::PtrCast<ReadInt>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reader))[14])(reader, &value, name);
        At<uint8_t>(base, 8 + At<int32_t>(base, 0x14)) = uint8_t(value);
        At<int32_t>(base, 0x14)++;
    }
}

// 0x4A8280: operator new: a slot from the area infos' pool (the size, its cdecl
// argument, unread)
uint8_t* __cdecl FlushPool() {
    return Pool_New(areaInfoPool);
}

// 0x4A8290: CPool<CAreaAmbientVehicleInfo>::Init (as a constructed array when +0x19
// says so; the size saturating on overflow)
void __fastcall PoolInit(CPool<uint8_t>* self, re::Edx) {
    self->ownsArrays = 1;
    self->elemSize = 0x74;
    if (self->unknown19) {
        uint32_t count = uint32_t(self->size);
        uint64_t bytes = uint64_t(count) * 0x74;
        uint32_t size = (bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes);
        size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
        uint32_t* block = static_cast<uint32_t*>(Malloc(size));
        if (block) {
            *block = count;
            ConstructArray(block + 1, 0x74, int(count), reinterpret_cast<void*>(0x0041A8F0));
            self->objects = reinterpret_cast<uint8_t*>(block + 1);
        } else {
            self->objects = nullptr;
        }
    } else {
        self->objects = static_cast<uint8_t*>(Malloc(size_t(self->size) * 0x74));
    }
    self->flags = static_cast<uint8_t*>(Malloc(size_t(self->size)));
    self->lastUsed = 0;
    for (int i = 0; i < self->size; i++) {
        self->flags[i] |= 0x80;
        self->flags[i] &= 0x80;
    }
}

// 0x4A8340: CAreaAmbientVehicleInfo's vf0 (the vector deleting destructor: flag 2 an
// array; one of them goes back to its pool)
void* __fastcall AreaDestroy(uint8_t* self, re::Edx, uint8_t flags) {
    if (flags & 2) {
        int32_t* header = reinterpret_cast<int32_t*>(self) - 1;
        for (int i = *header - 1; i >= 0; i--) {
            AreaInfo_Destruct(self + i * 0x74);
        }
        if (flags & 1) {
            Free(header);
        }
        return header;
    }
    AreaInfo_Destruct(self);
    if (flags & 1) {
        CPool<uint8_t>* pool = areaInfoPool;
        int slot = int((self - pool->objects) / pool->elemSize);
        pool->flags[slot] |= 0x80;
        if (slot < pool->lastUsed) {
            pool->lastUsed = slot;
        }
    }
    return self;
}

// 0x4A83C0: the area infos' pool made (11 slots)
void __cdecl InitPool() {
    CPool<uint8_t>* pool = static_cast<CPool<uint8_t>*>(Malloc(0x1C));
    if (!pool) {
        areaInfoPool = nullptr;
        return;
    }
    pool->unknown10 = 0;
    pool->objects = nullptr;
    pool->flags = nullptr;
    pool->lastUsed = 0;
    pool->ownsArrays = 1;
    pool->size = 0xB;
    pool->unknown19 = 0;
    AreaInfoPool_Init(pool);
    areaInfoPool = pool;
}

} // namespace c09ambientvehicles

// ---- registration ---------------------------------------------------------------------

#define AMBIENTVEHICLES_REVERSED(name, address, fn)                                          \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AMBIENTVEHICLES_REVERSED("CAmbientVehicleInfo::Construct", 0x004A7FD0, &c09ambientvehicles::Construct);
AMBIENTVEHICLES_REVERSED("CAmbientVehicleInfo::Destruct", 0x004A8000, &c09ambientvehicles::Destruct);
AMBIENTVEHICLES_REVERSED("CAreaAmbientVehicleInfo::Read", 0x004A8020, &c09ambientvehicles::AreaRead);
AMBIENTVEHICLES_REVERSED("CAreaAmbientVehicleInfo::Write", 0x004A80C0, &c09ambientvehicles::AreaWrite);
AMBIENTVEHICLES_REVERSED("CAmbientVehicleInfo::Destroy (IArchivable thunk)", 0x004A8160, &c09ambientvehicles::DestroyFromArchivable);
AMBIENTVEHICLES_REVERSED("CAmbientVehicleInfo::Destroy", 0x004A8170, &c09ambientvehicles::Destroy);
AMBIENTVEHICLES_REVERSED("CAmbientVehicleInfo::ArchivableRead", 0x004A81B0, &c09ambientvehicles::ArchivableRead);
AMBIENTVEHICLES_REVERSED("CAreaAmbientVehicleInfo::operator new", 0x004A8280, &c09ambientvehicles::FlushPool);
AMBIENTVEHICLES_REVERSED("CPool<CAreaAmbientVehicleInfo>::Init", 0x004A8290, &c09ambientvehicles::PoolInit);
AMBIENTVEHICLES_REVERSED("CAreaAmbientVehicleInfo::Destroy", 0x004A8340, &c09ambientvehicles::AreaDestroy);
AMBIENTVEHICLES_REVERSED("CAreaAmbientVehicleInfo::InitPool", 0x004A83C0, &c09ambientvehicles::InitPool);
