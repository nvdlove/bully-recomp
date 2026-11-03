// NiGlobalStringTable: the fixed strings (NiFixedString) every name in the library is, and
// NiPointer's release (the reference let go, the object's DeleteThis when it was the last).

#include "gamebryo/NiGlobalStringTable.h"

#include <cstring>
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

GAME_FN(uint32_t, __cdecl, NiGlobalStringTable_HashFunction, 0x007543D0, const char* s, uint32_t length);
GAME_FN(char*, __thiscall, NiGlobalStringTable_FindString, 0x00754460, NiGlobalStringTable* self, const char* s,
        uint32_t length);
GAME_FN(void, __thiscall, NiGlobalStringTable_InsertString, 0x00754650, NiGlobalStringTable* self, re::Ptr32<char>* handle,
        uint32_t hash);
// NiTPrimitiveArray<char*>::SetAt and AddFirstEmpty
GAME_FN(void, __thiscall, CharArray_SetAt, 0x00833C70, NiTArray<char*>* self, uint32_t index, const re::Ptr32<char>* value);
GAME_FN(uint32_t, __thiscall, CharArray_AddFirstEmpty, 0x008A4D20, NiTArray<char*>* self, const re::Ptr32<char>* value);

inline long& RefCount(char* s) {
    return *reinterpret_cast<long*>(s - 8);
}
inline uint16_t Length(const char* s) {
    return *reinterpret_cast<const uint16_t*>(s - 4);
}

} // namespace

// 0x7543D0: the bucket of a string (hash * 33 + character over its characters, 4 at a time for
// the first length & ~3, then on to the terminator; the characters signed), modulo 513
uint32_t __cdecl NiGlobalStringTable_HashFunctionImpl(const char* s, uint32_t length) {
    uint32_t hash = 0;
    uint32_t groups = length & ~3u;
    if (groups != 0) {
        for (uint32_t n = ((groups - 1) >> 2) + 1; n != 0; n--) {
            hash = hash * 33 + int32_t(int8_t(s[0]));
            hash = hash * 33 + int32_t(int8_t(s[1]));
            hash = hash * 33 + int32_t(int8_t(s[2]));
            hash = hash * 33 + int32_t(int8_t(s[3]));
            s += 4;
        }
    }
    for (; *s != 0; s++) {
        hash = hash * 33 + int32_t(int8_t(*s));
    }
    return hash % 513;
}

// 0x754460: NiGlobalStringTable::FindString, the table's copy of a string (the same pointer, or
// the same length and characters), or null
char* __fastcall NiGlobalStringTable_FindStringImpl(NiGlobalStringTable* self, re::Edx, const char* s,
                                                     uint32_t length) {
    uint32_t hash = NiGlobalStringTable_HashFunction(s, length);
    Lock(self->lock);
    NiTArray<char*>& bucket = self->buckets[hash];
    uint32_t size = bucket.size;
    re::Ptr32<char>* entries = bucket.base;
    char* found = nullptr;
    for (uint32_t i = 0; i < size; i++) {
        char* entry = entries[i];
        if (entry == s) {
            found = entry;
            break;
        }
        if (entry != nullptr && Length(entry) == length && std::strcmp(entry, s) == 0) {
            found = entry;
            break;
        }
    }
    Unlock(self->lock);
    return found;
}

// 0x754550: NiGlobalStringTable::RemoveString, once the last outside reference went (the
// table's own one dropped: freed when that was the last)
void __fastcall NiGlobalStringTable_RemoveString(NiGlobalStringTable* self, re::Edx, re::Ptr32<char>* handle, uint32_t hash) {
    char* s = *handle;
    Lock(self->lock);
    NiTArray<char*>& bucket = self->buckets[hash];
    uint32_t size = bucket.size;
    for (uint32_t i = 0; i < size; i++) {
        if (bucket.base[i] == s) {
            char* current = *handle;
            if (current != nullptr && RefCount(current) == 1 && Decrement(current - 8) == 0) {
                NiFree(*handle - 8);
                re::Ptr32<char> empty = nullptr;
                CharArray_SetAt(&bucket, i, &empty);
                ms_uiNumStrings--;
            }
            break;
        }
    }
    Unlock(self->lock);
}

// 0x754650: NiGlobalStringTable::InsertString
void __fastcall NiGlobalStringTable_InsertStringImpl(NiGlobalStringTable* self, re::Edx, re::Ptr32<char>* handle,
                                                     uint32_t hash) {
    Lock(self->lock);
    NiTArray<char*>& bucket = self->buckets[hash];
    if (bucket.esize != 0) {
        ms_uiNumCollisions++;
    }
    CharArray_AddFirstEmpty(&bucket, handle);
    ms_uiNumStrings++;
    Unlock(self->lock);
}

// 0x754760: NiGlobalStringTable::AddString (NiFixedString's constructor): the table's copy with
// one more reference, or a new one (two references: the table's and the caller's)
char* __cdecl NiGlobalStringTable_AddString(const char* s) {
    if (s == nullptr) {
        return nullptr;
    }
    uint32_t length = uint32_t(std::strlen(s));
    Lock(ms_pkGlobalStringTable->lock);
    char* found = NiGlobalStringTable_FindString(ms_pkGlobalStringTable, s, length);
    if (found != nullptr) {
        Increment(found - 8);
        Unlock(ms_pkGlobalStringTable->lock);
        return found;
    }
    uint32_t size = length + 9;
    if (size & 3) {
        size += 4 - (size & 3);
    }
    uint32_t* block = static_cast<uint32_t*>(NiMalloc(size));
    re::Ptr32<char> handle = reinterpret_cast<char*>(block + 2);
    uint32_t hash = NiGlobalStringTable_HashFunction(s, length);
    block[0] = 2;
    block[1] = hash << 16 | length;
    crt::Memcpy(handle, s, length + 1);
    NiGlobalStringTable_InsertString(ms_pkGlobalStringTable, &handle, hash);
    Unlock(ms_pkGlobalStringTable->lock);
    return handle;
}

// 0x7549D0: NiPointer's destructor (the reference released)
void __fastcall NiPointer_Release(re::Ptr32<NiRefObject>* self, re::Edx) {
    NiRefObject* object = *self;
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

#define NISTRING_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISTRING_REVERSED("NiGlobalStringTable::HashFunction", 0x007543D0, NiGlobalStringTable_HashFunctionImpl);
NISTRING_REVERSED("NiGlobalStringTable::FindString", 0x00754460, NiGlobalStringTable_FindStringImpl);
NISTRING_REVERSED("NiGlobalStringTable::RemoveString", 0x00754550, NiGlobalStringTable_RemoveString);
NISTRING_REVERSED("NiGlobalStringTable::InsertString", 0x00754650, NiGlobalStringTable_InsertStringImpl);
NISTRING_REVERSED("NiGlobalStringTable::AddString", 0x00754760, NiGlobalStringTable_AddString);
NISTRING_REVERSED("NiPointer::~NiPointer", 0x007549D0, NiPointer_Release);

} // namespace ni
