// CFileLoader: level files, IDE images, texture dictionaries, collision files
// (see FileLoader.h)

#include "game/FileLoader.h"

#include <cstdio>
#include <cstring>

#include "game/Collision.h"
#include "game/ModelInfo.h"
#include "game/TxdStore.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {
// Not reversed yet: called where they are. (CFileLoader's and CFileMgr's own go
// through their address too, so bullyre.ini decides which runs.)
GAME_FN(FileHandle, __cdecl, File_OpenFile, 0x0042D260, const char* path, const char* mode);
GAME_FN(uint32_t, __cdecl, File_Read, 0x0042D2A0, FileHandle file, void* buffer, uint32_t size);
GAME_FN(uint8_t, __cdecl, File_ReadLine, 0x0042D330, FileHandle file, char* buffer, int size);
GAME_FN(int, __cdecl, File_CloseFile, 0x0042D450, FileHandle file);
GAME_FN(char*, __cdecl, FL_LoadLine, 0x00429C10, FileHandle file);
GAME_FN(char*, __cdecl, FL_LoadLineFromBuffer, 0x00429CA0, const char* data, int size, int* consumed);
GAME_FN(TexDictionary*, __cdecl, FL_LoadTexDictionary, 0x00429DC0, const char* path);
GAME_FN(void, __cdecl, FL_AddTexDictionaries, 0x0042CDE0, TexDictionary* dst, TexDictionary* src);
GAME_FN(void, __cdecl, FL_SetAtomicModelInfoFlags, 0x00429E80, CBaseModelInfo* mi, uint32_t ideFlags);
GAME_FN(uint8_t, __cdecl, FL_IsModelInSpecialRanges, 0x00429FC0, int id);
GAME_FN(void, __cdecl, FL_LoadIDEimage, 0x0042CC40, const char* name);
GAME_FN(void, __cdecl, FL_LoadIDEimageData, 0x0042CAC0, uint8_t* data, int size);
GAME_FN(uint8_t*, __cdecl, FL_LoadCollisionBounds, 0x0042AAC0, uint8_t* data, CColModel* col);
GAME_FN(void, __cdecl, FL_LoadCollisionModel, 0x0042BCB0, uint8_t* data, CColModel* col, int size, int format, int format2);
using SectionLoader = void(__cdecl*)(re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadObjectTypes, 0x0042AB70, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadTimeObjectTypes, 0x0042B0D0, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadWeaponTypes, 0x0042A030, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadAccessoryTypes, 0x0042A1D0, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadVehicleTypes, 0x0042A2B0, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadPAnimTypes, 0x0042A620, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadClothTypes, 0x0042A7F0, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadItemTypes, 0x0042B550, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadSimpleTypes, 0x0042B660, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_Load2dEffects, 0x0042B760, re::Ptr32<uint8_t>* cursor);
GAME_FN(void, __cdecl, FL_LoadPedTypes, 0x0042C120, re::Ptr32<uint8_t>* cursor);

// memory
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, AllocArray, 0x005EEA50, size_t size, int);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void, __cdecl, PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopTxdMemoryTag, 0x0052E710);

// the loading screen
GAME_FN(uint8_t, __cdecl, IsLoadingScreenOn, 0x0043AD40);
GAME_FN(void, __cdecl, LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);
inline const char* __cdecl StrRChr(const char* s, int c) { return (const char*)crt::Strrchr(s, c); }   // the CRT's

// paths, streaming, the level's extras
GAME_FN(char*, __cdecl, GetFullPath, 0x00829900, const char* path);
GAME_FN(void, __cdecl, AddIdRange, 0x0052DDA0, int firstId, int count);   // CStreaming
GAME_FN(void, __cdecl, SetImagePath, 0x005F0B90, const char* path);
GAME_FN(void, __cdecl, LoadSkyDome, 0x0050EF70, const char* name);
GAME_FN(void, __cdecl, Fn_463F60, 0x00463F60);   // (walks the dummy pool)
GAME_FN(uint8_t, __cdecl, LoadModelInfoFromReader, 0x004373F0, void* reader, int arg);

// texture dictionaries (Gamebryo): CTxdStore's and a file reader (0x40C bytes)
GAME_FN(TexDictionary*, __cdecl, TexDictionary_Create, 0x005F25B0);
GAME_FN(void, __cdecl, TexDictionary_Destroy, 0x005F17B0, TexDictionary* dict, int);
GAME_FN(uint8_t, __cdecl, TexDictionary_Read, 0x005F2750, void* reader, TexDictionary* dict, const char*, const char*);
GAME_FN(void, __cdecl, SetCurrentTxdDict, 0x005F0A80, TexDictionary* dict);
GAME_FN(void, __cdecl, PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, PopCurrentTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, FindTexture, 0x005F1770, const char* name, int);
GAME_FN(void, __thiscall, Reader_Construct, 0x0075CE40, void* reader);
GAME_FN(uint8_t, __thiscall, Reader_OpenFile, 0x00759E10, void* reader, const char* path);
GAME_FN(void, __thiscall, Reader_Close, 0x0075B4D0, void* reader);
GAME_FN(void, __thiscall, Reader_Destruct, 0x0075C020, void* reader);
const char* const noneString = reinterpret_cast<const char*>(0x00900434);   // "None"

// A dictionary's textures: Gamebryo's array of refcounted pointers
struct RefObject {
    re::Ptr32<re::Ptr32<void>> vtable;
    volatile long refCount;  // +4
};
struct TexArray {
    re::Ptr32<re::Ptr32<void>> vtable;
    re::Ptr32<re::Ptr32<RefObject>> items;    // +4
    uint16_t capacity;    // +8
    uint16_t size;        // +0xA
    uint16_t unknownC;    // +0xC
    uint16_t growBy;      // +0xE
};
static_assert(sizeof(TexArray) == 0x10);
GAME_FN(void, __thiscall, TexArray_SetCapacity, 0x00856170, TexArray* array, uint32_t capacity);
GAME_FN(void, __thiscall, TexArray_SetAt, 0x0040E800, TexArray* array, uint32_t index, re::Ptr32<RefObject>* object);
using InterlockedFn = long(__stdcall*)(volatile long*);
GAME_VAR(InterlockedFn, Import_InterlockedDecrement, 0x008FC170);
GAME_VAR(InterlockedFn, Import_InterlockedIncrement, 0x008FC174);

// model infos: made by id, and their setters
GAME_FN(CBaseModelInfo*, __cdecl, AddAtomicModel, 0x0051C740, int id, const char* name);
GAME_FN(CBaseModelInfo*, __cdecl, AddTimeModel, 0x0051C7A0, int id, const char* name);
GAME_FN(CBaseModelInfo*, __cdecl, AddWeaponModel, 0x0051C800, int id, const char* name);
GAME_FN(CBaseModelInfo*, __cdecl, AddAccessoryModel, 0x0051C860, int id, const char* name);
GAME_FN(CBaseModelInfo*, __cdecl, AddVehicleModel, 0x0051C8C0, int id, const char* name, int);
GAME_FN(CBaseModelInfo*, __cdecl, AddPedModel, 0x0051C960, int id, const char* name, int special);
GAME_FN(CBaseModelInfo*, __cdecl, RedefinePedModel, 0x0051B360, int id, const char* name, int special);   // (an existing one of type 8)
GAME_FN(CBaseModelInfo*, __cdecl, AddPAnimModel, 0x0051C9D0, int id, const char* name);
GAME_FN(CBaseModelInfo*, __cdecl, AddClothModel, 0x0051CA60, int id, const char* name);
GAME_FN(void, __cdecl, AddModelName, 0x0043F370, const char* name, int id);
GAME_FN(void, __cdecl, Fn_527510, 0x00527510, int id, int);
GAME_FN(void, __cdecl, Fn_425350, 0x00425350);   // (after each ped)
GAME_FN(void, __cdecl, Fn_425370, 0x00425370);   // (after each vehicle)
GAME_FN(CBaseModelInfo*, __cdecl, GetModelInfo, 0x0051C190, const char* name, int* id);
GAME_FN(CBaseModelInfo*, __cdecl, GetModelInfoInRange, 0x0051C200, const char* name, int first, int last);
GAME_FN(void, __thiscall, ModelInfo_SetDrawDistance, 0x00527440, CBaseModelInfo* mi, float distance);
GAME_FN(void, __thiscall, ModelInfo_SetTexDictionary, 0x0050E900, CBaseModelInfo* mi, const char* name, int);
GAME_FN(void, __thiscall, ModelInfo_SetColModel, 0x0050EB30, CBaseModelInfo* mi, CColModel* col, int owns);
GAME_FN(void, __thiscall, ModelInfo_Add2dEffect, 0x0050EB70, CBaseModelInfo* mi, void* effect);
GAME_FN(uint8_t*, __thiscall, TimeModel_GetTimeInfo, 0x00824EC0, CBaseModelInfo* mi);   // (+0x3C gets the id)
GAME_FN(void, __thiscall, WeaponModel_535F80, 0x00535F80, CBaseModelInfo* mi, const char*);
GAME_FN(void, __thiscall, WeaponModel_535FB0, 0x00535FB0, CBaseModelInfo* mi, int, int);
GAME_FN(void, __thiscall, VehicleModel_534B00, 0x00534B00, CBaseModelInfo* mi, const char* vehicleClass);
GAME_FN(uint16_t, __thiscall, Handling_FindIndex, 0x004C9BE0, void* handling, const char* name);
inline void* const handlingData = reinterpret_cast<void*>(0x00C2DED0);
GAME_FN(int, __cdecl, Fn_4CE3E0, 0x004CE3E0, const char*);
GAME_FN(void, __thiscall, PAnimModel_520040, 0x00520040, CBaseModelInfo* mi, const char* anim);
GAME_FN(void, __thiscall, PAnimModel_520180, 0x00520180, CBaseModelInfo* mi, const char*);
GAME_FN(void, __thiscall, PAnimModel_5204A0, 0x005204A0, CBaseModelInfo* mi, int);
GAME_FN(void, __thiscall, PedModel_522C30, 0x00522C30, CBaseModelInfo* mi, int id, const char*);
GAME_FN(void, __thiscall, PedModel_522190, 0x00522190, CBaseModelInfo* mi, const char*);
GAME_FN(void, __thiscall, PedModel_5221C0, 0x005221C0, CBaseModelInfo* mi, const char*);
GAME_FN(void, __thiscall, PedModel_5221F0, 0x005221F0, CBaseModelInfo* mi, const char*);
GAME_FN(void, __thiscall, PedModel_522370, 0x00522370, CBaseModelInfo* mi, int);
GAME_FN(void, __thiscall, PedModel_5226D0, 0x005226D0, CBaseModelInfo* mi, const char*, const char*);
GAME_FN(void, __thiscall, PedModel_5227D0, 0x005227D0, CBaseModelInfo* mi, const char*, const char*);
GAME_FN(int, __cdecl, Fn_488B70, 0x00488B70, const char*);
GAME_FN(int, __cdecl, Fn_488B10, 0x00488B10, const char*);
GAME_FN(int, __thiscall, Fn_49A0F0, 0x0049A0F0, void* object, const char*);
GAME_VAR(void*, object_C2C084, 0x00C2C084);
GAME_VAR(CColModel*, ms_pedColModel, 0x00CF312C);
GAME_VAR(float, itemDrawDistance, 0x00900E68);   // 30
GAME_FN(void*, __cdecl, Effect_New, 0x00527620, size_t size);

// collision
GAME_FN(CColModel*, __cdecl, ColModel_New, 0x0056E7C0, size_t size);
GAME_FN(CColModel*, __thiscall, ColModel_Construct, 0x0056E7A0, CColModel* col);
GAME_FN(CCollisionData*, __thiscall, CollisionData_Construct, 0x0056DAD0, CCollisionData* data);
GAME_FN(void, __thiscall, CollisionData_RemoveCollisionVolumes, 0x0056DBD0, CCollisionData* data);
GAME_FN(void, __thiscall, ColSphere_Set, 0x0056EBA0, CColSphere* sphere, float radius, const void* center, uint8_t surface, uint8_t piece);
GAME_FN(void, __thiscall, ColBox_Set, 0x005648A0, void* box, const void* min, const void* max, uint8_t surface, uint8_t piece);
GAME_FN(void*, __thiscall, TrianglePlanes_Construct, 0x005617E0, void* planes);
GAME_FN(void, __thiscall, TrianglePlanes_Load, 0x005618E0, void* planes, int numTriangles, const void*, const void*, int count, const void*);
GAME_FN(void, __cdecl, ColStore_SetModelSlot, 0x0056F120, int slot, int id);
GAME_FN(void, __cdecl, ColStore_56F420, 0x0056F420, int slot, int);
GAME_FN(int, __cdecl, ColStore_GetFirstModel, 0x0056F3C0, int slot);
GAME_FN(int, __cdecl, ColStore_GetLastModel, 0x0056F3F0, int slot);
GAME_FN(CColModel*, __cdecl, FindColModel, 0x0051A960, int id);
GAME_FN(void, __cdecl, Fn_5EB8B0, 0x005EB8B0, CColModel* col);
GAME_VAR(CColModel*, ms_sharedColModel, 0x00CF3120);   // not a model's own

// Gamebryo models: a stream reader's first object, made into the game's objects
GAME_FN(void*, __cdecl, Fn_5F1520, 0x005F1520);
GAME_FN(void, __thiscall, Reader_75A340, 0x0075A340, void* reader, void*);
GAME_FN(void, __thiscall, Reader_OpenMemory, 0x007599F0, void* reader, void* data, uint32_t size);
GAME_VAR(uint32_t, niClassBit_20E2550, 0x020E2550);   // the class a loaded object must be
GAME_FN(uint8_t*, __cdecl, Fn_6C7540, 0x006C7540);
GAME_FN(void*, __cdecl, Fn_6CA210, 0x006CA210);
GAME_FN(void, __cdecl, Fn_6C7410, 0x006C7410, uint8_t*, void*);
GAME_FN(uint8_t*, __cdecl, Fn_6C6F50, 0x006C6F50);
GAME_FN(void, __cdecl, Fn_6C6C60, 0x006C6C60, uint8_t* node, RefObject* object, int);   // (takes a smart pointer: releases it)
GAME_FN(void, __cdecl, Fn_6C74A0, 0x006C74A0, uint8_t*, uint8_t* node);
GAME_FN(void, __cdecl, Fn_6C6A10, 0x006C6A10, uint8_t* node, void*);
GAME_FN(void, __cdecl, Fn_6C9D80, 0x006C9D80, void*, void*);
GAME_FN(void, __cdecl, Fn_40AD10, 0x0040AD10, void*, uintptr_t, int, int);
GAME_FN(void*, __cdecl, Fn_40AD80, 0x0040AD80, RefObject* object, int id, int category, const char* name);
// Gamebryo's NiFixedString: refcounted (-8) and sized (-2), in a pool
GAME_FN(char*, __cdecl, FixedString_Create, 0x00754760, const char* s);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00406730, re::Ptr32<char>* string, const char* s);
GAME_FN(uint8_t, __thiscall, FixedString_Equals, 0x00406510, re::Ptr32<char>* string, const char* s);
GAME_FN(void, __thiscall, FixedStringPool_Free, 0x00754550, void* pool, re::Ptr32<char>* string, uint32_t length);
GAME_VAR(void*, fixedStringPool, 0x020E216C);
GAME_VAR(char*, emptyFixedString, 0x00951028);

// NiDynamicCast: the object if it's of the class with bit `niClassBit_20E2550`
RefObject* DynamicCast(RefObject* object) {
    if (!object) {
        return nullptr;
    }
    const uint8_t* rtti = re::PtrCast<const uint8_t*(__thiscall*)(RefObject*)>(object->vtable[2])(object);
    uint32_t bit = niClassBit_20E2550;
    uint32_t word;
    std::memcpy(&word, rtti + 0xC + (bit >> 5) * 4, 4);
    return (word & (1u << (bit & 31))) ? object : nullptr;
}

void ReleaseFixedString(re::Ptr32<char>* string) {
    char* s = *string;
    if (s == emptyFixedString) {
        return;
    }
    uint16_t length;
    std::memcpy(&length, s - 2, 2);
    if (Import_InterlockedDecrement(reinterpret_cast<volatile long*>(s - 8)) == 1) {
        FixedStringPool_Free(fixedStringPool, string, length);
    }
}

// A field not in our structs yet, at its offset
template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(object) + offset);
}

// The IDE image's data: 32-bit words; a string is in whole words, up to the one
// whose last byte is 0.
int32_t ReadInt(re::Ptr32<uint8_t>& p) {
    int32_t value;
    std::memcpy(&value, p, 4);
    p += 4;
    return value;
}

float ReadFloat(re::Ptr32<uint8_t>& p) {
    float value;
    std::memcpy(&value, p, 4);
    p += 4;
    return value;
}

void ReadString(re::Ptr32<uint8_t>& p, char* out) {
    do {
        std::memcpy(out, p, 4);
        p += 4;
        out += 4;
    } while (out[-1] != 0);
}

// The model's animation file (vf6)
void SetAnimFile(CBaseModelInfo* mi, const char* name) {
    reinterpret_cast<void(__thiscall*)(CBaseModelInfo*, const char*)>(static_cast<void*>(mi->vtable[6]))(mi, name);
}

constexpr uint32_t Tag(char a, char b, char c, char d) {
    return uint32_t(uint8_t(a)) << 24 | uint32_t(uint8_t(b)) << 16 | uint32_t(uint8_t(c)) << 8 | uint8_t(d);
}
} // namespace

namespace CFileLoader {

// ---- text lines -------------------------------------------------------------------------

static constexpr int LINE_SIZE = 0x15E;

// leading blanks (and anything below '!', signed: bytes over 0x7F too)
static char* SkipBlanks(char* p) {
    while (int8_t(*p) <= ' ' && *p != 0) {
        p++;
    }
    return p;
}

char* __cdecl LoadLine(FileHandle file) {
    std::memset(ms_line, 0, LINE_SIZE);
    if (!File_ReadLine(file, ms_line, LINE_SIZE)) {
        return nullptr;
    }
    for (int i = 0; ms_line[i] != 0; i++) {
        char c = ms_line[i];
        if (c == '\n') {
            ms_line[i] = 0;
            break;
        }
        // (signed: bytes over 0x7F become spaces too)
        if (int8_t(c) < ' ' || c == ',') {
            ms_line[i] = ' ';
        }
    }
    return SkipBlanks(ms_line);
}

char* __cdecl LoadLineFromBuffer(const char* data, int size, int* consumed) {
    // (strncpy: not terminated when the line fills it)
    std::strncpy(ms_line, data, size < LINE_SIZE ? size_t(size) : size_t(LINE_SIZE));
    *consumed = 0;
    int i = 0;
    if (ms_line[0] != 0) {
        for (;;) {
            char c = ms_line[i];
            if (c == '\n' || c == '\r' || *consumed >= size) {
                break;
            }
            if (int8_t(c) < ' ' || c == ',') {
                ms_line[i] = ' ';
            }
            (*consumed)++;
            i++;
            if (ms_line[i] == 0) {
                break;
            }
        }
    }
    if (ms_line[i] == '\r' && *consumed < size && ms_line[i + 1] == '\n') {
        ms_line[i + 1] = 0;
        (*consumed)++;
    }
    ms_line[i] = 0;
    (*consumed)++;
    return SkipBlanks(ms_line);
}

char* __cdecl LoadLineFromMemory(MemoryFile* stream) {
    int consumed = 0;
    char* line = FL_LoadLineFromBuffer(re::PtrCast<const char*>(stream->data) + stream->position,
                                       int(stream->size - stream->position), &consumed);
    if (consumed > 0 && stream->position + uint32_t(consumed) <= stream->size) {
        stream->position += uint32_t(consumed);
    }
    return line;
}

void __cdecl LoadingScreenLoadingFile(const char* path) {
    if (!IsLoadingScreenOn()) {
        return;
    }
    const char* name = StrRChr(path, '\\');
    name = name ? name + 1 : path;
    std::sprintf(ms_loadingText, "Loading %s", name);
    LoadingScreen("Loading the Game", ms_loadingText, 0);
}

// ---- texture dictionaries -------------------------------------------------------------

TexDictionary* __cdecl LoadTexDictionary(const char* path) {
    const char* fullPath = GetFullPath(path);
    alignas(4) uint8_t reader[0x40C];
    Reader_Construct(reader);
    uint8_t opened = Reader_OpenFile(reader, fullPath);
    // (made either way: empty when the file can't be read)
    TexDictionary* dict = TexDictionary_Create();
    if (opened) {
        TexDictionary_Read(reader, dict, noneString, noneString);
        Reader_Close(reader);
    }
    Reader_Destruct(reader);
    return dict;
}

void __cdecl AddTexDictionaries(TexDictionary* dst, TexDictionary* src) {
    for (uint32_t i = 0; i < re::PtrCast<TexArray*>(src->textures)->size; i++) {
        RefObject* texture = re::PtrCast<TexArray*>(src->textures)->items[i];
        if (!texture) {
            continue;
        }
        // (a smart pointer's copy, released after the add)
        re::Ptr32<RefObject> ref = texture;
        Import_InterlockedIncrement(&texture->refCount);
        TexArray* array = re::PtrCast<TexArray*>(dst->textures);
        uint32_t count = array->size;
        if (count >= array->capacity) {
            TexArray_SetCapacity(array, array->growBy + count);
        }
        TexArray_SetAt(array, count, &ref);
        if (Import_InterlockedDecrement(&texture->refCount) == 0) {
            re::PtrCast<void(__thiscall*)(RefObject*)>(texture->vtable[1])(texture);
        }
    }
}

uint8_t __cdecl LoadModelInfoFile(const char* path, int arg) {
    // the reader: the file, and whether it took it over (then it's closed there)
    struct {
        FileHandle file;
        int32_t keepOpen;
        int32_t unknown8;
        int32_t unknownC;
    } reader = {File_OpenFile(path, "rb"), 0, 0, 0};
    uint8_t result = LoadModelInfoFromReader(&reader, arg);
    if (reader.keepOpen == 0) {
        File_CloseFile(reader.file);
    }
    return result;
}

// A 2dfx entry's effect (0x40, from its pool)
struct Effect2d {
    float pos[3];         // +0
    uint8_t color[4];     // +0xC
    int16_t next;         // +0x10 the model's next effect (see C2dEffect)
    uint8_t pad12[2];
    re::Ptr32<void> texture14;      // +0x14
    re::Ptr32<void> texture18;      // +0x18
    float unknown1C[4];   // +0x1C
    float unknown2C;      // +0x2C
    float unknown30;      // +0x30
    float unknown34;      // +0x34
    uint8_t unknown38;    // +0x38
    uint8_t unknown39;    // +0x39
    uint8_t unknown3A;    // +0x3A
    uint8_t unknown3B;    // +0x3B
    uint8_t flags;        // +0x3C
    uint8_t pad3D[3];
};
static_assert(offsetof(Effect2d, next) == 0x10);
static_assert(offsetof(Effect2d, unknown1C) == 0x1C);
static_assert(offsetof(Effect2d, flags) == 0x3C);
static_assert(sizeof(Effect2d) == 0x40);

// ---- models ---------------------------------------------------------------------------

uint8_t* __cdecl LoadAtomicFile2Return(const char* path) {
    alignas(4) uint8_t reader[0x40C];
    Reader_Construct(reader);
    const char* fullPath = GetFullPath(path);
    Reader_75A340(reader, Fn_5F1520());
    PushMemoryTag(10);
    uint8_t opened = Reader_OpenFile(reader, fullPath);
    PopMemoryTag();
    if (!opened) {
        Reader_Destruct(reader);
        return nullptr;
    }
    uint8_t* object = Fn_6C7540();
    object[0x34] = 0;
    Fn_6C7410(object, Fn_6CA210());
    uint8_t* node = Fn_6C6F50();
    // the file's first object (a smart pointer's copy, handed over)
    RefObject* loaded = DynamicCast(Field<re::Ptr32<RefObject>*>(reader, 0x15C)[0]);
    if (loaded) {
        Import_InterlockedIncrement(&loaded->refCount);
    }
    Fn_6C6C60(node, loaded, 0);
    Field<int32_t>(node, 0x20) = 0;
    Fn_6C74A0(object, node);
    if (void* existing = Field<void*>(Field<void*>(object, 4), 0x84)) {
        Fn_6C6A10(node, existing);
    } else {
        Fn_6C6A10(node, Fn_6CA210());
        Fn_6C9D80(Field<void*>(object, 4), Field<void*>(node, 4));
    }
    Fn_40AD10(Field<void*>(node, 0x14), 0, 0, 0);
    Reader_Destruct(reader);
    return object;
}

uint8_t __cdecl LoadClumpFile(FileStreamBuffer* buffer, int id) {
    alignas(4) uint8_t reader[0x40C];
    Reader_Construct(reader);
    Reader_75A340(reader, Fn_5F1520());
    PushMemoryTag(10);
    Reader_OpenMemory(reader, buffer->data, buffer->size);
    PopMemoryTag();
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
    // what kind of model it is, by the path it came from (+0x48's +0x1C)
    const char* source;
    if (void* info = Field<void*>(mi, 0x48)) {
        source = static_cast<const char*>(info) + 0x1C;
    } else {
        source = mi->type == 11 ? "Clothing" : "NULL";
    }
    // (the game's null check here can't fail)
    re::Ptr32<char> path = source ? FixedString_Create(source) : static_cast<char*>(emptyFixedString);
    re::Ptr32<char> name = FixedString_Create("NULL");
    const char* slash = StrRChr(path, '\\');
    FixedString_Assign(&name, slash ? slash + 1 : path);
    int category;
    if (FixedString_Equals(&path, "Cuts")) {
        category = 3;
    } else if (FixedString_Equals(&path, "Peds")) {
        category = 0;
    } else if (FixedString_Equals(&path, "Vehicles")) {
        category = 1;
    } else if (FixedString_Equals(&path, "Wprops")) {
        // (the carnival's are like peds)
        if (FixedString_Equals(&path, "WPROPS\\CARNI01") || FixedString_Equals(&path, "WPROPS\\CARNI02") ||
            FixedString_Equals(&path, "WPROPS\\CARNI03") || FixedString_Equals(&path, "WPROPS\\CARNIKID1") ||
            FixedString_Equals(&path, "WPROPS\\CARNIKID2") || FixedString_Equals(&path, "WPROPS\\CARNIKID3") ||
            FixedString_Equals(&path, "WPROPS\\CARNIPINKY")) {
            category = 0;
        } else {
            category = 2;
        }
    } else if (FixedString_Equals(&path, "Accssory")) {
        category = 5;
    } else if (FixedString_Equals(&path, "Clothing")) {
        category = 6;
    } else {
        category = FixedString_Equals(&path, "Objects") ? 7 : 8;
    }
    RefObject* loaded = DynamicCast(Field<re::Ptr32<RefObject>*>(reader, 0x15C)[0]);
    void* made = Fn_40AD80(loaded, id, category, name);
    // vf13: the model's object
    reinterpret_cast<void(__thiscall*)(CBaseModelInfo*, void*, int)>(static_cast<void*>(mi->vtable[13]))(mi, made, 1);
    ReleaseFixedString(&name);
    ReleaseFixedString(&path);
    Reader_Destruct(reader);
    return 1;
}

// 0x42A8C0 (thiscall): what a 2dfx effect is made with
static Effect2d* __fastcall ConstructEffect2d(Effect2d* effect, re::Edx) {
    effect->pos[0] = 0.0f;
    effect->next = -1;
    effect->pos[1] = 0.0f;
    effect->pos[2] = 0.0f;
    return effect;
}

// ---- model infos --------------------------------------------------------------------------

void __cdecl SetAtomicModelInfoFlags(CBaseModelInfo* mi, uint32_t ideFlags) {
    uint32_t& flags = mi->flags;
    auto set = [&flags](uint32_t on, uint32_t bit) { flags = on != 0 ? flags | bit : flags & ~bit; };
    set(ideFlags & 1, 0x4);
    set(ideFlags & 2, 0x20);
    set(ideFlags & 0xC, 0x40);
    set(ideFlags & 8, 0x80);
    flags &= ~0x100u;
    set(ideFlags & 0x40, 0x400);
    set(ideFlags & 0x80, 0x800);
    set(ideFlags & 0x100, 0x1000);
    set(ideFlags & 0x200, 0x2000);
    set(ideFlags & 0x400, 0x4000);
    set(ideFlags & 0x10, 0x40000);
    set(ideFlags & 0x1000, 0x10000);
    set(ideFlags & 0x2000, 0x20000);
    set(ideFlags & 0x4000, 0x80000);
    set(ideFlags & 0x10000, 0x200000);
    set(ideFlags & 0x20000, 0x2000000);
}

uint8_t __cdecl IsModelInSpecialRanges(int id) {
    return (id >= 0x281 && id <= 0x6D0) || (id >= 0x6F0 && id <= 0x717) || (id >= 0xDED && id <= 0x23B3) ||
           (id >= 0x23F1 && id <= 0x2580) || (id >= 0x2838 && id <= 0x28D1) || (id >= 0x297D && id <= 0x2A58) ||
           uint32_t(id - 0x2A95) <= 0x44A;
}

// "objs" and "tobj" entries. The kind (0..5) says how many draw distances there are
// (kind / 2 + 1; the first is used) and whether three more bytes follow (even kinds).
// What's read is kept from entry to entry: an unknown kind makes one more of the
// last entry, odd kinds keep the last bytes.
struct AtomicEntry {
    int32_t id = 0;
    char name[256] = {};
    char txd[256] = {};
    float drawDistance = 0.0f;
    uint32_t ideFlags = 0;
    uint8_t unknown0B = 0;
    uint8_t unknown2D = 0;
    uint8_t unknown2E = 0;
    uint8_t unknown2F = 0;
};

static void ReadAtomicEntry(re::Ptr32<uint8_t>& p, uint32_t kind, AtomicEntry& e) {
    e.id = ReadInt(p);
    ReadString(p, e.name);
    ReadString(p, e.txd);
    p += 4;  // (the mesh count)
    e.drawDistance = ReadFloat(p);
    p += 4 * (kind / 2);  // (the other draw distances)
    e.ideFlags = uint32_t(ReadInt(p));
    p += 4 * 3;
    e.unknown0B = uint8_t(ReadInt(p));
    if (kind % 2 == 0) {
        e.unknown2D = uint8_t(ReadInt(p));
        e.unknown2E = uint8_t(ReadInt(p));
        e.unknown2F = uint8_t(ReadInt(p));
    }
}

static void SetAtomicBytes(CBaseModelInfo* mi, const AtomicEntry& e) {
    Field<uint8_t>(mi, 0x2D) = e.unknown2D;
    mi->unknown0B = e.unknown0B;
    Field<uint8_t>(mi, 0x2E) = e.unknown2E;
    Field<uint8_t>(mi, 0x2F) = e.unknown2F;
    Field<uint8_t>(mi, 0x32) = 0;
    Field<uint8_t>(mi, 0x31) = 0;
    Field<uint8_t>(mi, 0x30) = 0;
}

void __cdecl LoadObjectTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    if (count <= 0) {
        return;
    }
    // (the game's registers start with the count)
    AtomicEntry e;
    e.ideFlags = uint32_t(count);
    e.unknown0B = uint8_t(count);
    for (int left = count; left != 0; left--) {
        uint32_t kind = uint32_t(ReadInt(p));
        if (kind <= 5) {
            ReadAtomicEntry(p, kind, e);
        }
        CBaseModelInfo* mi = AddAtomicModel(e.id, e.name);
        ModelInfo_SetDrawDistance(mi, e.drawDistance);
        ModelInfo_SetTexDictionary(mi, e.txd, 0);
        FL_SetAtomicModelInfoFlags(mi, e.ideFlags);
        // the name says where peds can't go / can walk
        if (!std::strncmp(e.name, "nogo_", 5) || !std::strncmp(e.name, "NOGO_", 5) || !std::strncmp(e.name, "walkable_", 9) ||
            !std::strncmp(e.name, "WALKABLE_", 9)) {
            mi->flags |= 0x1000000;
        } else {
            mi->flags &= ~0x1000000u;
        }
        if (FL_IsModelInSpecialRanges(e.id)) {
            mi->flags |= 0x100;
        }
        SetAtomicBytes(mi, e);
        Fn_527510(e.id, 0);
        AddModelName(e.name, e.id);
    }
}

void __cdecl LoadTimeObjectTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    if (count <= 0) {
        return;
    }
    AtomicEntry e;
    e.ideFlags = uint32_t(count);
    e.unknown0B = uint8_t(count);
    int32_t timeOn = 0;
    int32_t timeOff = 0;
    for (int left = count; left != 0; left--) {
        uint32_t kind = uint32_t(ReadInt(p));
        if (kind <= 5) {
            ReadAtomicEntry(p, kind, e);
            timeOn = ReadInt(p);
            timeOff = ReadInt(p);
        }
        CBaseModelInfo* mi = AddTimeModel(e.id, e.name);
        ModelInfo_SetDrawDistance(mi, e.drawDistance);
        Field<int32_t>(mi, 0x34) = timeOn;
        Field<int32_t>(mi, 0x38) = timeOff;
        ModelInfo_SetTexDictionary(mi, e.txd, 0);
        FL_SetAtomicModelInfoFlags(mi, e.ideFlags);
        if (uint8_t* timeInfo = TimeModel_GetTimeInfo(mi)) {
            Field<int32_t>(timeInfo, 0x3C) = e.id;
        }
        SetAtomicBytes(mi, e);
        Fn_527510(e.id, 0);
        AddModelName(e.name, e.id);
    }
}

void __cdecl LoadWeaponTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    for (int left = count; left > 0; left--) {
        int id = ReadInt(p);
        char name[256], txd[256], anim[256], unknown[256];
        ReadString(p, name);
        ReadString(p, txd);
        ReadString(p, anim);
        ReadString(p, unknown);
        p += 4;
        float unknown50 = ReadFloat(p);
        int a = ReadInt(p);
        int b = ReadInt(p);
        if (ms_firstWeaponModel == -1) {
            ms_firstWeaponModel = id;
        }
        ms_lastWeaponModel = id;
        CBaseModelInfo* mi = AddWeaponModel(id, name);
        ModelInfo_SetTexDictionary(mi, txd, 0);
        SetAnimFile(mi, anim);
        WeaponModel_535F80(mi, unknown);
        Field<float>(mi, 0x50) = unknown50;
        WeaponModel_535FB0(mi, a, b);
        AddModelName(name, id);
    }
}

void __cdecl LoadAccessoryTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    for (int left = count; left > 0; left--) {
        int id = ReadInt(p);
        char name[256], txd[256];
        ReadString(p, name);
        ReadString(p, txd);
        CBaseModelInfo* mi = AddAccessoryModel(id, name);
        ModelInfo_SetTexDictionary(mi, txd, 0);
        AddModelName(name, id);
        if (ms_firstAccessoryModel == -1) {
            ms_firstAccessoryModel = id;
        }
        ms_lastAccessoryModel = id;
    }
}

void __cdecl LoadVehicleTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    for (int left = count; left > 0; left--) {
        int id = ReadInt(p);
        char name[256], txd[256], type[256], handling[256], gameName[256], anim[256], vehicleClass[256], unknown[256];
        ReadString(p, name);
        ReadString(p, txd);
        ReadString(p, type);
        ReadString(p, handling);
        ReadString(p, gameName);
        ReadString(p, anim);
        ReadString(p, vehicleClass);
        ReadString(p, unknown);
        int unknown62 = ReadInt(p);
        p += 4;
        int unknownDC = ReadInt(p);
        int wheels = ReadInt(p);  // (a car's +0x5C, a bike's +0xE0 as a float)
        float unknown58 = ReadFloat(p);
        CBaseModelInfo* mi = AddVehicleModel(id, name, 0);
        ModelInfo_SetTexDictionary(mi, txd, 0);
        SetAnimFile(mi, anim);
        VehicleModel_534B00(mi, vehicleClass);
        // (after the first character; the name isn't used after that)
        for (char* c = gameName; *c != 0;) {
            c++;
            if (*c == '_') {
                *c = ' ';
            }
        }
        Field<int32_t>(mi, 0xDC) = unknownDC;
        if (!std::memcmp(type, "car", 4)) {
            Field<float>(mi, 0x58) = unknown58;
            Field<uint16_t>(mi, 0x5C) = uint16_t(wheels);
            Field<int32_t>(mi, 0x50) = 0;
        } else if (!std::memcmp(type, "bike", 5)) {
            Field<int32_t>(mi, 0x50) = 1;
            Field<float>(mi, 0xE0) = float(wheels);
            Field<float>(mi, 0x58) = unknown58;
            if (ms_firstBikeModel == -1) {
                ms_firstBikeModel = id;
            }
            ms_lastBikeModel = id;
        }
        AddModelName(name, id);
        Field<uint16_t>(mi, 0x5E) = Handling_FindIndex(handlingData, handling);
        int unknown54 = Fn_4CE3E0(unknown);
        Field<int32_t>(mi, 0x54) = unknown54;
        if (unknown54 != -1) {
            Field<uint16_t>(mi, 0x62) = uint16_t(unknown62);
        }
        if (ms_firstVehicleModel == -1) {
            ms_firstVehicleModel = id;
        }
        ms_lastVehicleModel = id;
        Fn_425370();
    }
}

void __cdecl LoadPAnimTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    for (int left = count; left > 0; left--) {
        int id = ReadInt(p);
        char name[256], txd[256], anim[256], unknown[256];
        ReadString(p, name);
        ReadString(p, txd);
        ReadString(p, anim);
        ReadString(p, unknown);
        int unknown8F = ReadInt(p);
        int unknown5204A0 = ReadInt(p);
        int unknown90 = ReadInt(p);
        CBaseModelInfo* mi = AddPAnimModel(id, name);
        PAnimModel_520040(mi, anim);
        ModelInfo_SetTexDictionary(mi, txd, 0);
        SetAnimFile(mi, anim);
        PAnimModel_520180(mi, unknown);
        Field<uint8_t>(mi, 0x8F) = unknown8F != 0;
        if (unknown5204A0 != 0) {
            PAnimModel_5204A0(mi, unknown5204A0);
        }
        Field<uint8_t>(mi, 0x90) = unknown90 != 0;
        AddModelName(name, id);
        if (ms_firstPAnimModel == -1) {
            ms_firstPAnimModel = id;
        }
        ms_lastPAnimModel = id;
    }
}

void __cdecl LoadClothTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    for (int left = count; left > 0; left--) {
        int id = ReadInt(p);
        char name[256], txd[256];
        ReadString(p, name);
        ReadString(p, txd);
        CBaseModelInfo* mi = AddClothModel(id, name);
        ModelInfo_SetTexDictionary(mi, txd, 0);
        // (no name registered for these)
        if (ms_firstClothModel == -1) {
            ms_firstClothModel = id;
        }
        if (id > ms_lastClothModel) {
            ms_lastClothModel = id;
        }
    }
}

// "item", "cash" and "scnd": atomic models with a fixed draw distance
static void LoadFixedAtomic(re::Ptr32<uint8_t>& p, int* id) {
    *id = ReadInt(p);
    char name[256], txd[256];
    ReadString(p, name);
    ReadString(p, txd);
    CBaseModelInfo* mi = AddAtomicModel(*id, name);
    ModelInfo_SetTexDictionary(mi, txd, 0);
    Field<float>(mi, 0x24) = itemDrawDistance;
    mi->unknown0B = 0xFF;
    Field<uint8_t>(mi, 0x2D) = 0;
    Field<uint8_t>(mi, 0x2E) = 0;
    Field<uint8_t>(mi, 0x2F) = 0;
    Field<uint8_t>(mi, 0x32) = 0;
    Field<uint8_t>(mi, 0x31) = 0;
    Field<uint8_t>(mi, 0x30) = 0;
    Fn_527510(*id, 0);
    mi->flags |= 0x400000;
    AddModelName(name, *id);
}

void __cdecl LoadItemTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    for (int left = count; left > 0; left--) {
        int id;
        LoadFixedAtomic(p, &id);
        if (ms_firstItemModel == -1) {
            ms_firstItemModel = id;
        }
        ms_lastItemModel = id;
    }
}

void __cdecl LoadSimpleTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    for (int left = count; left > 0; left--) {
        int id;
        LoadFixedAtomic(p, &id);
    }
}

void __cdecl Load2dEffects(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    for (int left = count; left > 0; left--) {
        // (the game writes through nullptr when the pool is full)
        Effect2d* effect = static_cast<Effect2d*>(Effect_New(sizeof(Effect2d)));
        if (effect) {
            effect->next = -1;
            effect->pos[0] = 0.0f;
            effect->pos[1] = 0.0f;
            effect->pos[2] = 0.0f;
        }
        int id = ReadInt(p);
        float x = ReadFloat(p);
        float y = ReadFloat(p);
        float z = ReadFloat(p);
        int r = ReadInt(p);
        int g = ReadInt(p);
        int b = ReadInt(p);
        int a = ReadInt(p);
        p += 4;
        char texture14[256], texture18[256];
        ReadString(p, texture14);
        ReadString(p, texture18);
        for (float& f : effect->unknown1C) {
            f = ReadFloat(p);
        }
        int unknown3B = ReadInt(p);
        int unknown39 = ReadInt(p);
        effect->unknown2C = ReadFloat(p);
        effect->unknown30 = ReadFloat(p);
        effect->unknown34 = ReadFloat(p);
        p += 4;
        int unknown3A = ReadInt(p);
        uint32_t flags = uint32_t(ReadInt(p));
        effect->unknown38 = ReadInt(p) != 0;
        // its textures come from the "particle" dictionary
        PushCurrentTxd();
        SetCurrentTxd(FindTxdSlot("particle", 0));
        ModelInfo_Add2dEffect(CModelInfo::ms_modelInfoPtrs[id], effect);
        effect->pos[0] = x;
        effect->pos[1] = y;
        effect->pos[2] = z;
        effect->color[0] = uint8_t(r);
        effect->color[1] = uint8_t(g);
        effect->color[2] = uint8_t(b);
        effect->color[3] = uint8_t(a);
        effect->texture14 = FindTexture(texture14, 0);
        effect->texture18 = FindTexture(texture18, 0);
        effect->unknown3B = uint8_t(unknown3B);
        effect->unknown39 = uint8_t(unknown39);
        effect->unknown3A = uint8_t(unknown3A);
        if (flags & 4) {
            flags &= ~2u;
        }
        effect->flags = uint8_t(flags);
        PopCurrentTxd();
    }
}

void __cdecl LoadPedTypes(re::Ptr32<uint8_t>* cursor) {
    re::Ptr32<uint8_t>& p = *cursor;
    int count = ReadInt(p);
    if (count <= 0) {
        return;
    }
    // (kept from entry to entry: see below)
    CBaseModelInfo* mi = nullptr;
    for (int left = count; left != 0; left--) {
        int id = ReadInt(p);
        char name[256], s1[256], s2[256], s3[256], s4[256], anim[256], s6[256], s7[256], s8[256];
        char s9[256], s10[256], s11[256], s12[256], s13[256];
        ReadString(p, name);
        ReadString(p, s1);
        int unknown50 = ReadInt(p);
        ReadString(p, s2);
        ReadString(p, s3);
        ReadString(p, s4);
        ReadString(p, anim);
        ReadString(p, s6);
        ReadString(p, s7);
        ReadString(p, s8);
        int unknown5C = ReadInt(p);
        ReadString(p, s9);
        ReadString(p, s10);
        ReadString(p, s11);
        ReadString(p, s12);
        ReadString(p, s13);
        // the special peds: from "spfirst" to "splast"
        if (ms_firstSpecialPedModel == -1 && !std::memcmp(name, "spfirst", 8)) {
            ms_firstSpecialPedModel = id;
        }
        if (ms_lastSpecialPedModel == -1 && !std::memcmp(name, "splast", 7)) {
            ms_lastSpecialPedModel = id;
        }
        int firstSpecial = ms_firstSpecialPedModel;
        uint8_t special = !(id < firstSpecial || firstSpecial == -1);
        CBaseModelInfo* existing = CModelInfo::ms_modelInfoPtrs[id];
        if (!existing) {
            mi = AddPedModel(id, name, special);
        } else if (existing->type == 8) {
            mi = RedefinePedModel(id, name, special);
        }
        // (any other: the game goes on with the last entry's model info)
        Field<uint8_t>(mi, 0x78) = uint8_t(id);
        ModelInfo_SetTexDictionary(mi, name, 0);  // (the ped's own name)
        PedModel_522C30(mi, id, s1);
        SetAnimFile(mi, anim);
        PedModel_522190(mi, s6);
        PedModel_5221C0(mi, s7);
        PedModel_5221F0(mi, s8);
        Fn_488B70(s2);  // (twice: the first result unused)
        PedModel_522370(mi, Fn_488B70(s2));
        ModelInfo_SetColModel(mi, ms_pedColModel, 0);
        Field<int32_t>(mi, 0x4C) = Fn_488B10(s3);
        Field<uint8_t>(mi, 0x50) = unknown50 != 0;
        Field<int32_t>(mi, 0x54) = Fn_49A0F0(object_C2C084, s4);
        Field<uint16_t>(mi, 0x5C) = uint16_t(unknown5C);
        Field<uint16_t>(mi, 0x5E) = uint16_t(unknown5C);
        Field<int32_t>(mi, 0x60) = 0;
        PedModel_5226D0(mi, s9, s10);
        PedModel_5227D0(mi, s11, s12);
        std::strncpy(&Field<char>(mi, 0x98), s13, 0x18);
        if (ms_firstPedModel == -1) {
            ms_firstPedModel = id;
        }
        if (ms_firstSpecialPedModel == -1) {
            ms_lastNormalPedModel = id;
        }
        Fn_425350();
        AddModelName(name, id);
    }
}

// ---- IDE images and levels ---------------------------------------------------------------

void __cdecl LoadIDEimageData(uint8_t* data, int size) {
    uint8_t* end = data + size;
    re::Ptr32<uint8_t> p = data;
    while (p < end) {
        uint32_t tag = uint32_t(ReadInt(p));
        SectionLoader load = nullptr;
        switch (tag) {
        case Tag('o', 'b', 'j', 's'): load = FL_LoadObjectTypes; break;
        case Tag('t', 'o', 'b', 'j'): load = FL_LoadTimeObjectTypes; break;
        case Tag('w', 'e', 'a', 'p'): load = FL_LoadWeaponTypes; break;
        case Tag('c', 'a', 's', 'h'): load = FL_LoadSimpleTypes; break;
        case Tag('s', 'c', 'n', 'd'): load = FL_LoadSimpleTypes; break;
        case Tag('a', 'c', 'c', 's'): load = FL_LoadAccessoryTypes; break;
        case Tag('i', 't', 'e', 'm'): load = FL_LoadItemTypes; break;
        case Tag('c', 'a', 'r', 's'): load = FL_LoadVehicleTypes; break;
        case Tag('p', 'e', 'd', 's'): load = FL_LoadPedTypes; break;
        case Tag('2', 'd', 'f', 'x'): load = FL_Load2dEffects; break;
        case Tag('p', 'a', 'n', 'm'): load = FL_LoadPAnimTypes; break;
        case Tag('c', 'l', 't', 'h'): load = FL_LoadClothTypes; break;
        case Tag('p', 'a', 't', 'h'): p += 4; break;  // (skipped)
        default: break;                               // (unknown: only its range)
        }
        if (load) {
            load(&p);
        }
        // the model ids the section used, -1: none
        int first = ReadInt(p);
        int last = ReadInt(p);
        if (first != -1) {
            AddIdRange(first, last - first + 1);
        }
    }
}

void __cdecl LoadIDEimage(const char* name) {
    // (up to 80 entries; the game doesn't check, and leaks it when a file's missing)
    PushTxdMemoryTag();
    DirEntry* dir = static_cast<DirEntry*>(Malloc(0xA00));
    PopTxdMemoryTag();
    char path[32];
    std::strcpy(path, name);
    std::strcat(path, ".DIR");
    FileHandle file = File_OpenFile(path, "r");
    if (!file) {
        return;
    }
    uint32_t sectors = 0;
    for (DirEntry* entry = dir; File_Read(file, entry, sizeof(DirEntry)) != 0; entry++) {
        sectors += entry->size;
    }
    File_CloseFile(file);
    int total = int(sectors << 11);
    std::strcpy(path, name);
    std::strcat(path, ".IMG");
    file = File_OpenFile(path, "r");
    if (!file) {
        return;
    }
    PushTxdMemoryTag();
    uint8_t* image = static_cast<uint8_t*>(Malloc(size_t(total)));
    PopTxdMemoryTag();
    File_Read(file, image, uint32_t(total));
    File_CloseFile(file);
    // each entry: its size, then its sections
    if (total > 0) {
        uint8_t* data = image;
        const DirEntry* entry = dir;
        int done = 0;
        do {
            int size = int(entry->size << 11);
            LoadingScreen("CFileLoader::LoadIDEimage", name, 0);
            uint32_t length;
            std::memcpy(&length, data, 4);
            FL_LoadIDEimageData(data + 4, int(length));
            done += size;
            data += size;
            entry++;
        } while (done < total);
    }
    Free(dir);
    Free(image);
}

void __cdecl LoadLevel(const char* path) {
    // the level's shared textures go into the current dictionary (made if there's none)
    TexDictionary* dict = CTxdStore::ms_pCurrentTxd;
    if (!dict) {
        dict = TexDictionary_Create();
        SetCurrentTxdDict(dict);
    }
    FileHandle file = File_OpenFile(path, "r");
    for (char* line = FL_LoadLine(file); line; line = FL_LoadLine(file)) {
        if (*line == '#') {
            continue;
        }
        if (!std::strncmp("EXIT", line, 4)) {
            LoadingScreen(path, "Done loading level", 0);
            break;
        }
        LoadingScreen(path, line, 0);
        if (!std::strncmp("IMAGEPATH", line, 9)) {
            SetImagePath(line + 10);
        } else if (!std::strncmp("TEXDICTION", line, 10)) {
            PushMemoryTag(0x17);
            char txdPath[64];
            std::strcpy(txdPath, line + 11);
            TexDictionary* txd = FL_LoadTexDictionary(txdPath);
            FL_AddTexDictionaries(dict, txd);
            TexDictionary_Destroy(txd, 1);
            PopMemoryTag();
        } else if (!std::strncmp("COLFILE", line, 7) || !std::strncmp("MODELFILE", line, 9) ||
                   !std::strncmp("HIERFILE", line, 8)) {
            // (not used: models and collision are streamed)
        } else if (!std::strncmp("IMGIDE", line, 6)) {
            FL_LoadIDEimage(line + 7);
        } else if (!std::strncmp("SKY_DOME", line, 8)) {
            PushMemoryTag(0x2D);
            LoadSkyDome(line + 9);
            PopMemoryTag();
        }
    }
    File_CloseFile(file);
    SetCurrentTxdDict(dict);
    Fn_463F60();
}

// ---- collision ----------------------------------------------------------------------------

uint8_t* __cdecl LoadCollisionBounds(uint8_t* data, CColModel* col) {
    // (the vectors' fourth floats: the file's are skipped, and the game writes
    // stale stack into the model's; left as they are here)
    float in[12];
    std::memcpy(in, data, sizeof(in));
    col->sphereCenter = {in[0], in[1], in[2]};
    col->sphereRadius = in[3];
    col->boxMin = {in[4], in[5], in[6]};
    col->boxMax = {in[8], in[9], in[10]};
    return data + 0x30;
}

// 0x42AA30 (the source in esi: not hookable): the volumes into a new
// CCollisionData of the model's; `from` is left empty
static void MoveCollisionData(CCollisionData* from, CColModel* col) {
    void* memory = Malloc(sizeof(CCollisionData));
    CCollisionData* data = memory ? CollisionData_Construct(static_cast<CCollisionData*>(memory)) : nullptr;
    col->data = data;
    *data = *from;
    std::memset(from, 0, sizeof(CCollisionData));
}

template <typename T>
static T* AllocVolumes(int count) {
    PushMemoryTag(0x12);
    T* array = static_cast<T*>(AllocArray(size_t(count) * sizeof(T), 0));
    PopMemoryTag();
    return array;
}

// A box as the game stores it (0x24)
struct ColBoxData {
    uint8_t data[0x24];
};

void __cdecl LoadCollisionModel(uint8_t* data, CColModel* col, int size, int format, int format2) {
    re::Ptr32<uint8_t> p = FL_LoadCollisionBounds(data, col);
    // (the old data is freed but left in the model until the new one's set)
    if (CCollisionData* old = col->data) {
        CollisionData_RemoveCollisionVolumes(old);
        Free(old);
    }
    CCollisionData volumes;
    CollisionData_Construct(&volumes);

    int8_t numSpheres = int8_t(*p);
    p += 4;
    volumes.numSpheres = uint8_t(numSpheres);
    if (numSpheres > 0) {
        volumes.spheres = AllocVolumes<CColSphere>(numSpheres);
        for (int i = 0; i < numSpheres; i++) {
            float radius;
            std::memcpy(&radius, p + 0xC, 4);
            ColSphere_Set(&volumes.spheres[i], radius, p, p[0x10], p[0x11]);
            p += 0x14;
        }
    } else {
        volumes.spheres = nullptr;
    }

    // (lines: skipped)
    int8_t numLines = int8_t(*p);
    p += 4;
    if (numLines > 0) {
        p += numLines * 32;
    }
    volumes.unknown20 = 0;
    volumes.unknown04 = nullptr;

    int8_t numBoxes = int8_t(*p);
    p += 4;
    volumes.numBoxes = uint8_t(numBoxes);
    if (numBoxes > 0) {
        ColBoxData* boxes = AllocVolumes<ColBoxData>(numBoxes);
        volumes.boxes = reinterpret_cast<CColBox*>(boxes);
        for (int i = 0; i < numBoxes; i++) {
            ColBox_Set(&boxes[i], p, p + 0x10, p[0x20], p[0x21]);
            p += 0x24;
        }
    } else {
        volumes.boxes = nullptr;
    }

    int32_t numVertices = ReadInt(p);
    if (numVertices > 0) {
        volumes.vertices = AllocVolumes<CompressedVector>(numVertices);
        int bytes = numVertices * int(sizeof(CompressedVector));
        if (format > 0) {
            // already compressed (padded to 4 bytes)
            std::memcpy(volumes.vertices, p, size_t(bytes));
            p += (bytes + 3) & ~3;
        } else {
            for (int i = 0; i < numVertices; i++) {
                float v[3];
                std::memcpy(v, p, sizeof(v));
                // (_ftol: truncated)
                volumes.vertices[i].x = int16_t(int64_t(double(v[0]) * 128.0));
                volumes.vertices[i].y = int16_t(int64_t(double(v[1]) * 128.0));
                volumes.vertices[i].z = int16_t(int64_t(double(v[2]) * 128.0));
                p += 12;
            }
        }
    } else {
        volumes.vertices = nullptr;
    }

    int16_t numTriangles = int16_t(ReadInt(p));
    volumes.numTriangles = numTriangles;
    if (numTriangles > 0) {
        volumes.triangles = AllocVolumes<CColTriangle>(numTriangles);
        // before format 3 the entries are 16 or 20 bytes: 20 from format 1 on, and in
        // format 0 when the first one looks like it (bytes 0x12/0x13 0xFF, 0xF set)
        bool wide = false;
        for (int i = 0; i < numTriangles; i++) {
            uint16_t a, b, c;
            uint8_t surface;
            std::memcpy(&a, p, 2);
            if (format >= 3) {
                std::memcpy(&b, p + 2, 2);
                std::memcpy(&c, p + 4, 2);
                surface = p[6];
                p += format2 == 3 && format == 3 ? 12 : 8;
            } else {
                bool thisWide;
                if (i == 0) {
                    wide = p[0x12] == 0xFF && p[0x13] == 0xFF && p[0xF] != 0;
                    thisWide = wide || format > 0;
                } else {
                    thisWide = wide || format > 0;
                }
                std::memcpy(&b, p + 4, 2);
                std::memcpy(&c, p + 8, 2);
                surface = p[0xC];
                p += thisWide ? 0x14 : 0x10;
            }
            CColTriangle& t = volumes.triangles[i];
            t.a = a;
            t.b = b;
            t.c = c;
            t.surface = surface;
            t.light = 1;
        }
    } else {
        volumes.triangles = nullptr;
    }
    col->unknown34 = int(numTriangles) + int8_t(volumes.numBoxes) + int8_t(volumes.unknown20) + int8_t(volumes.numSpheres) != 0;

    // optional chunks: the triangles' planes, then "LIMK"
    if (p - data != size) {
        uint32_t tag;
        std::memcpy(&tag, p, 4);
        if (tag == 0xEF5DCB33) {
            p += 4;
            if (!volumes.planes) {
                void* memory = Malloc(0x28);
                volumes.planes = static_cast<TrianglePlanes*>(memory ? TrianglePlanes_Construct(memory) : nullptr);
            }
            int32_t planeCount;
            std::memcpy(&planeCount, p + 0x20, 4);
            TrianglePlanes_Load(volumes.planes, numTriangles, p, p + 0x10, planeCount, p + 0x24);
            p += 0x24 + Field<int32_t>(volumes.planes, 0x20) * 12;
        }
        if (p - data != size) {
            std::memcpy(&tag, p, 4);
            if (tag == Tag('K', 'M', 'I', 'L')) {  // "LIMK" in the file
                std::memcpy(&col->unknown32, p + 4, 2);
                p += 8;
                uint8_t* extra = static_cast<uint8_t*>(AllocVolumes<uint8_t>(volumes.numTriangles * 12));
                volumes.unknown18 = extra;
                for (int i = 0; i < volumes.numTriangles; i++) {
                    volumes.triangles[i].light = 1;
                    std::memcpy(extra + i * 12, p, 12);
                    p += 12;
                }
            }
        }
    }
    MoveCollisionData(&volumes, col);
    CollisionData_RemoveCollisionVolumes(&volumes);
}

// A .col file's model header (after the chunk's tag and size, and COL2/COL3's word)
struct ColFileModelName {
    char name[20];
    int32_t modelId;  // when the name is empty
};
static_assert(sizeof(ColFileModelName) == 0x18);

// The chunks: "PATH" (skipped), "COLL"/"COL2"/"COL3" a model each. Anything else
// ends the file: fine if less than a sector's left.
template <typename OnModel>
static uint8_t ForEachColModel(uint8_t* data, uint32_t size, bool col3Signed, OnModel onModel) {
    int id = -1;  // (kept between models)
    while (size > 8) {
        uint32_t tag;
        int32_t chunkSize;
        std::memcpy(&tag, data, 4);
        std::memcpy(&chunkSize, data + 4, 4);
        uint8_t* p = data + 8;
        size -= 8;
        if (tag == Tag('H', 'T', 'A', 'P')) {
            size -= uint32_t(chunkSize);
            data = p + chunkSize;
            continue;
        }
        if (tag != Tag('L', 'L', 'O', 'C') && tag != Tag('2', 'L', 'O', 'C') && tag != Tag('3', 'L', 'O', 'C')) {
            return size < 0x800;
        }
        int header = 0;
        int format = 0;
        int format2 = 1;
        if (tag == Tag('2', 'L', 'O', 'C')) {
            std::memcpy(&format, p, 4);
            p += 4;
            header = 4;
            size -= 4;
        } else if (tag == Tag('3', 'L', 'O', 'C')) {
            uint16_t words[2];
            std::memcpy(words, p, 4);
            format = col3Signed ? int(int16_t(words[0])) : int(words[0]);
            format2 = col3Signed ? int(int16_t(words[1])) : int(words[1]);
            p += 4;
            header = 4;
            size -= 4;
        }
        ColFileModelName name;
        std::memcpy(&name, p, sizeof(name));
        p += sizeof(name);
        header += int(sizeof(name));
        size -= sizeof(name);
        int modelSize = chunkSize - header;
        size += uint32_t(header - chunkSize);
        data = p + modelSize;
        onModel(name, id, p, modelSize, format, format2);
    }
    return 1;
}

uint8_t __cdecl LoadCollisionFile(uint8_t* data, uint32_t size, int colSlot) {
    uint16_t slot = uint16_t(colSlot);
    // (COL3's words zero-extended here)
    return ForEachColModel(data, size, false, [slot](ColFileModelName& name, int& id, uint8_t* model, int modelSize, int format, int format2) {
        CBaseModelInfo* mi = nullptr;
        if (name.name[0] == 0) {
            id = name.modelId;
            if (id > 0) {
                mi = CModelInfo::ms_modelInfoPtrs[id];
            }
        } else {
            int last = ColStore_GetLastModel(slot);
            int first = ColStore_GetFirstModel(slot);
            mi = GetModelInfoInRange(name.name, first, last);
        }
        if (!mi) {
            return;
        }
        CColModel* col = static_cast<CColModel*>(mi->colModel);
        if (col && col != ms_sharedColModel) {
            // (another file's: left alone)
            if (col->colSlot != slot) {
                return;
            }
            FL_LoadCollisionModel(model, col, modelSize, format, format2);
        } else {
            col = FindColModel(id);
            if (col) {
                FL_LoadCollisionModel(model, col, modelSize, format, format2);
            } else {
                CColModel* memory = ColModel_New(0x40);
                col = memory ? ColModel_Construct(memory) : nullptr;
                col->colSlot = slot;
                FL_LoadCollisionModel(model, col, modelSize, format, format2);
                ModelInfo_SetColModel(mi, col, 1);
            }
        }
        if (col->data && col->data->unknown18) {
            Fn_5EB8B0(col);
        }
    });
}

uint8_t __cdecl LoadCollisionFileFirstTime(uint8_t* data, uint32_t size, int colSlot) {
    uint16_t slot = uint16_t(colSlot);
    return ForEachColModel(data, size, true, [slot](ColFileModelName& name, int& id, uint8_t* model, int modelSize, int format, int format2) {
        CBaseModelInfo* mi = nullptr;
        if (name.name[0] == 0) {
            id = name.modelId;
            if (id > 0) {
                mi = CModelInfo::ms_modelInfoPtrs[id];
            }
        } else {
            mi = GetModelInfo(name.name, &id);
        }
        if (!mi) {
            return;
        }
        ColStore_SetModelSlot(slot, id);
        CColModel* memory = ColModel_New(0x40);
        CColModel* col = memory ? ColModel_Construct(memory) : nullptr;
        // the first 500 models' collision stays: it's in no slot
        bool permanent = id < 500;
        if (permanent) {
            PushMemoryTag(0x18);
            ColStore_56F420(slot, 1);
            col->colSlot = 0;
        } else {
            col->colSlot = slot;
        }
        FL_LoadCollisionModel(model, col, modelSize, format, format2);
        ModelInfo_SetColModel(mi, col, 1);
        if (permanent) {
            PopMemoryTag();
        }
    });
}

} // namespace CFileLoader

// Untested: the game's code runs until each is checked.
#define FL_REVERSED(fn, address)            \
    static void* Original_##fn;             \
    REVERSED("CFileLoader::" #fn, address, RE_BODY(&CFileLoader::fn), nullptr, &Original_##fn, re::Mode::Original)

FL_REVERSED(LoadLine, 0x00429C10);
FL_REVERSED(LoadLineFromBuffer, 0x00429CA0);
FL_REVERSED(LoadLineFromMemory, 0x0042B980);
FL_REVERSED(LoadingScreenLoadingFile, 0x00429D70);
FL_REVERSED(LoadTexDictionary, 0x00429DC0);
FL_REVERSED(AddTexDictionaries, 0x0042CDE0);
FL_REVERSED(LoadModelInfoFile, 0x00429E30);
FL_REVERSED(SetAtomicModelInfoFlags, 0x00429E80);
FL_REVERSED(IsModelInSpecialRanges, 0x00429FC0);
FL_REVERSED(LoadObjectTypes, 0x0042AB70);
FL_REVERSED(LoadTimeObjectTypes, 0x0042B0D0);
FL_REVERSED(LoadWeaponTypes, 0x0042A030);
FL_REVERSED(LoadAccessoryTypes, 0x0042A1D0);
FL_REVERSED(LoadVehicleTypes, 0x0042A2B0);
FL_REVERSED(LoadPAnimTypes, 0x0042A620);
FL_REVERSED(LoadClothTypes, 0x0042A7F0);
FL_REVERSED(LoadItemTypes, 0x0042B550);
FL_REVERSED(LoadSimpleTypes, 0x0042B660);
FL_REVERSED(Load2dEffects, 0x0042B760);
FL_REVERSED(LoadPedTypes, 0x0042C120);
FL_REVERSED(LoadIDEimageData, 0x0042CAC0);
FL_REVERSED(LoadIDEimage, 0x0042CC40);
FL_REVERSED(LoadLevel, 0x0042CE80);
FL_REVERSED(LoadCollisionBounds, 0x0042AAC0);
FL_REVERSED(LoadCollisionModel, 0x0042BCB0);
FL_REVERSED(LoadCollisionFile, 0x0042C6C0);
FL_REVERSED(LoadCollisionFileFirstTime, 0x0042C8E0);
FL_REVERSED(LoadAtomicFile2Return, 0x0042A8E0);
FL_REVERSED(LoadClumpFile, 0x0042B9D0);
static void* Original_ConstructEffect2d;
REVERSED("CFileLoader::ConstructEffect2d", 0x0042A8C0, RE_BODY(&CFileLoader::ConstructEffect2d), nullptr,
         &Original_ConstructEffect2d, re::Mode::Original);
