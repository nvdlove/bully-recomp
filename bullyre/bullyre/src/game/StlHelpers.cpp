// 0x40BAD0 to 0x40C900: MSVC 2005 STL instantiated in the game's code: std::string
// (assign, grow, tidy), std::logic_error / length_error, a list and hash_map of ints,
// and a vector of 8 byte items (checked iterators: a container pointer and a pointer).
// Written for their normal paths; where the STL throws, the game's own throwing
// functions are called.
//
// std::string: +4 the buffer (16 bytes) or, from a capacity of 16, a pointer; +0x14
// the size; +0x18 the capacity. The vector: +4 first, +8 last, +0xC end.

#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_FN(void*, __cdecl, CMemoryMgr_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, CMemoryMgr_Free, 0x005EEAB0, void* p);
inline void __cdecl InvalidParameter() { crt::InvalidParameter(); }   // _invalid_parameter_noinfo
inline void __cdecl Xran() { crt::Xran(); }               // std::_Xran (out_of_range)
inline void __cdecl Xlen() { crt::Xlen(); }               // std::_Xlen (length_error, strings)
GAME_FN(void, __cdecl, VectorXlen, 0x005B01A0);         // vector<T> too long
inline int __cdecl memcpy_s_(void* dst, size_t size, const void* src, size_t count) { return crt::MemcpyS(dst, size, src, count); }
GAME_FN(void*, __thiscall, String_Allocate, 0x0040B9E0, void* self, uint32_t count);
GAME_FN(void, __thiscall, String_Copy, 0x0040BB20, void* self, uint32_t newSize, uint32_t oldLength);
GAME_FN(void*, __thiscall, String_Erase, 0x0040BA40, void* self, uint32_t pos, uint32_t count);
GAME_FN(void*, __thiscall, String_Assign, 0x0040C0D0, void* self, const void* other, uint32_t pos, uint32_t count);
GAME_FN(void*, __thiscall, String_AssignChars, 0x0040C1B0, void* self, const char* s, uint32_t count);
GAME_FN(void, __thiscall, exception_Default, 0x0085BDD1, void* self);
GAME_FN(void, __thiscall, exception_Copy, 0x0085BE48, void* self, const void* other);
GAME_FN(void, __thiscall, exception_Destruct, 0x0085BEA0, void* self);
inline void __stdcall CxxThrowException_(void* object, const void* throwInfo) { crt::CxxThrowException(object, throwInfo); }
GAME_FN(void, __stdcall, Construct8, 0x0040BCA0, uint32_t* p, const uint32_t* value);
GAME_FN(uint8_t*, __cdecl, UninitializedCopy8, 0x0040BCC0, uint8_t* first, uint8_t* last, uint8_t* dest, void*, int);
GAME_FN(void, __cdecl, UninitializedFill8, 0x0040C010, uint8_t* dest, uint32_t count, const uint32_t* value, void*, int);
GAME_FN(void, __cdecl, Fill8, 0x0040BC70, uint8_t* first, uint8_t* last, const uint32_t* value);
GAME_FN(uint8_t*, __cdecl, CopyBackward8, 0x0040BFD0, uint8_t* first, uint8_t* last, uint8_t* destEnd);
GAME_FN(void*, __cdecl, Allocate8, 0x0040B7A0, uint32_t count, void*);
GAME_FN(uint8_t*, __thiscall, Vector_UninitFill, 0x0040C360, void* self, uint8_t* first, uint32_t count, const uint32_t* value);
GAME_FN(uint8_t*, __thiscall, Vector_UninitCopy, 0x0040C540, void* self, uint8_t* first, uint8_t* last, uint8_t* dest);
GAME_FN(void, __thiscall, Vector_Erase, 0x0040C2B0, void* self, void* result[2], void* cont, uint8_t* first, void* cont2,
        uint8_t* last);
GAME_FN(void, __thiscall, Vector_InsertN, 0x0040C570, void* self, void* cont, uint8_t* where, uint32_t count,
        const uint32_t* value);
constexpr uint32_t kLogicErrorVtable = 0x008FFFCC;
constexpr uint32_t kLengthErrorVtable = 0x008FFFD8;
inline const void* const kLengthErrorThrowInfo = reinterpret_cast<const void*>(0x009C1DE0);

uint32_t& Size(uint8_t* s) {
    return *reinterpret_cast<uint32_t*>(s + 0x14);
}
uint32_t& Capacity(uint8_t* s) {
    return *reinterpret_cast<uint32_t*>(s + 0x18);
}
char* Chars(uint8_t* s) {
    return Capacity(s) >= 0x10 ? static_cast<char*>(*reinterpret_cast<re::Ptr32<char>*>(s + 4)) : reinterpret_cast<char*>(s + 4);
}

// std::string::_Grow(n): 1 when there's something to copy
bool Grow(uint8_t* s, uint32_t n) {
    if (n == 0xFFFFFFFFu) {
        Xlen();
    }
    if (Capacity(s) < n) {
        String_Copy(s, n, Size(s));
    } else if (n == 0) {
        Size(s) = 0;
        Chars(s)[0] = 0;
        return false;
    }
    return n != 0;
}

re::Ptr32<uint8_t>& First(uint8_t* v) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(v + 4);
}
re::Ptr32<uint8_t>& Last(uint8_t* v) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(v + 8);
}
re::Ptr32<uint8_t>& End(uint8_t* v) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(v + 0xC);
}
uint32_t Count(uint8_t* v) {
    return First(v) ? uint32_t((Last(v) - First(v)) >> 3) : 0;
}

} // namespace

namespace StlHelpers {

// 0x40BAD0 (thiscall): a list's destructor: every node and the head freed (+4 the head,
// +8 the size)
static void __fastcall List_Destruct(uint8_t* self, re::Edx) {
    uint32_t* head = *reinterpret_cast<re::Ptr32<uint32_t>*>(self + 4);
    uint32_t* node = reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(head[0]));
    head[0] = reinterpret_cast<uintptr_t>(head);
    (*reinterpret_cast<re::Ptr32<uint32_t>*>(self + 4))[1] = static_cast<uint32_t>(re::PtrCast<uintptr_t>(*reinterpret_cast<re::Ptr32<uint32_t>*>(self + 4)));
    *reinterpret_cast<uint32_t*>(self + 8) = 0;
    uint32_t* end = *reinterpret_cast<re::Ptr32<uint32_t>*>(self + 4);
    while (node != end) {
        uint32_t* next = reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(node[0]));
        CMemoryMgr_Free(node);
        node = next;
        end = *reinterpret_cast<re::Ptr32<uint32_t>*>(self + 4);
    }
    CMemoryMgr_Free(*reinterpret_cast<re::Ptr32<void>*>(self + 4));
    *reinterpret_cast<re::Ptr32<void>*>(self + 4) = nullptr;
}

// 0x40BB20 (thiscall): std::string::_Copy: a bigger buffer (the size | 15, or half again
// the capacity), the first `oldLength` characters kept. (Its catch block, retrying the
// exact size when allocating fails, isn't reproduced)
static void __fastcall String_Copy_(uint8_t* self, re::Edx, uint32_t newSize, uint32_t oldLength) {
    uint32_t capacity = newSize | 0xF;
    if (capacity > 0xFFFFFFFEu) {
        capacity = newSize;
    } else {
        uint32_t half = Capacity(self) >> 1;
        if (capacity / 3 < half && Capacity(self) <= 0xFFFFFFFEu - half) {
            capacity = half + Capacity(self);
        }
    }
    char* data = static_cast<char*>(String_Allocate(self, capacity + 1));
    if (oldLength != 0) {
        memcpy_s_(data, capacity + 1, Chars(self), oldLength);
    }
    if (Capacity(self) >= 0x10) {
        CMemoryMgr_Free(*reinterpret_cast<re::Ptr32<void>*>(self + 4));
    }
    self[4] = 0;
    *reinterpret_cast<re::Ptr32<char>*>(self + 4) = data;
    Capacity(self) = capacity;
    Size(self) = oldLength;
    Chars(self)[oldLength] = 0;
}

// 0x40BC70: std::fill of 8 byte items
static void __cdecl Fill8_(uint32_t* first, uint32_t* last, const uint32_t* value) {
    for (; first != last; first += 2) {
        first[0] = value[0];
        first[1] = value[1];
    }
}

// 0x40BCA0 (stdcall): an 8 byte item constructed (unless the place is null)
static void __stdcall Construct8_(uint32_t* p, const uint32_t* value) {
    if (p) {
        p[0] = value[0];
        p[1] = value[1];
    }
}

// 0x40BCC0: std::uninitialized_copy of 8 byte items; returns the destination's end
static uint8_t* __cdecl UninitializedCopy8_(uint8_t* first, uint8_t* last, uint8_t* dest, void*, int) {
    for (; first != last; first += 8) {
        Construct8(reinterpret_cast<uint32_t*>(dest), reinterpret_cast<const uint32_t*>(first));
        dest += 8;
    }
    return dest;
}

// 0x40BD30 (thiscall): std::string::_Tidy(true)
static void __fastcall String_Tidy(uint8_t* self, re::Edx) {
    if (Capacity(self) >= 0x10) {
        CMemoryMgr_Free(*reinterpret_cast<re::Ptr32<void>*>(self + 4));
    }
    Capacity(self) = 0xF;
    Size(self) = 0;
    self[4] = 0;
}

// 0x40BD60 (thiscall): a hash_map<int, ...>'s find: the key's bucket (Park-Miller of key
// ^ 0xDEADBEEF, masked; +0x20 the mask, +0x24 the bucket count), then its sorted list
// (+4 the list; +0x14.. the buckets, each an iterator); an iterator (list, node) out
static uint32_t* __fastcall HashMap_Find(uint8_t* self, re::Edx, uint32_t* result, const int32_t* key) {
    int32_t x = int32_t(uint32_t(*key) ^ 0xDEADBEEFu);
    int32_t q = x / 127773;
    int32_t r = x % 127773;
    int32_t h = r * 16807 - q * 2836;
    if (h < 0) {
        h += 0x7FFFFFFF;
    }
    uint32_t mask = *reinterpret_cast<uint32_t*>(self + 0x20);
    uint32_t bucket = mask & uint32_t(h);
    if (*reinterpret_cast<uint32_t*>(self + 0x24) <= bucket) {
        bucket += 0xFFFFFFFFu - (mask >> 1);
    }
    auto buckets = [&]() { return *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x14); };
    auto bucketCount = [&]() {
        return uint32_t((*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x18) - buckets()) >> 3);
    };
    if (!buckets() || bucket >= bucketCount()) {
        InvalidParameter();
    }
    uint32_t* node = *reinterpret_cast<re::Ptr32<uint32_t>*>(buckets() + bucket * 8 + 4);
    uint8_t* list = self + 4;
    uint32_t* head = *reinterpret_cast<re::Ptr32<uint32_t>*>(self + 8);
    if (!list) {
        InvalidParameter();
    }
    uint32_t next = bucket + 1;
    while (true) {
        if (!buckets() || next >= bucketCount()) {
            InvalidParameter();
        }
        if (*reinterpret_cast<re::Ptr32<uint32_t>*>(buckets() + next * 8 + 4) == node) {
            result[0] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(list));
            result[1] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head));
            return result;
        }
        if (node == head) {
            InvalidParameter();
        }
        if (int32_t(node[2]) >= *key) {
            break;
        }
        node = reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(node[0]));
    }
    if (node == head) {
        InvalidParameter();
    }
    result[0] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(list));
    result[1] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(*key < int32_t(node[2]) ? head : node));
    return result;
}

// 0x40BEB0: a list's head node (linked to itself)
static uint32_t* __cdecl List_BuyHead() {
    uint32_t* node = static_cast<uint32_t*>(CMemoryMgr_Malloc(0x10));
    if (node) {
        node[0] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(node));
    }
    if (node + 1) {
        node[1] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(node));
    }
    return node;
}

// 0x40BF30 (thiscall): a list node (next, previous, an 8 byte value)
static uint32_t* __fastcall List_BuyNode(void*, re::Edx, uint32_t next, uint32_t previous, const uint32_t* value) {
    uint32_t* node = static_cast<uint32_t*>(CMemoryMgr_Malloc(0x10));
    if (node) {
        node[0] = next;
    }
    if (node + 1) {
        node[1] = previous;
    }
    Construct8(node + 2, value);
    return node;
}

// 0x40BFD0: std::copy_backward of 8 byte items; returns the destination's start
static uint8_t* __cdecl CopyBackward8_(uint8_t* first, uint8_t* last, uint8_t* destEnd) {
    uint8_t* result = destEnd - ((last - first) >> 3) * 8;
    while (last != first) {
        last -= 8;
        destEnd -= 8;
        reinterpret_cast<uint32_t*>(destEnd)[0] = reinterpret_cast<uint32_t*>(last)[0];
        reinterpret_cast<uint32_t*>(destEnd)[1] = reinterpret_cast<uint32_t*>(last)[1];
    }
    return result;
}

// 0x40C010: std::uninitialized_fill_n of 8 byte items
static void __cdecl UninitializedFill8_(uint8_t* dest, uint32_t count, const uint32_t* value, void*, int) {
    for (; count != 0; count--) {
        Construct8(reinterpret_cast<uint32_t*>(dest), value);
        dest += 8;
    }
}

// 0x40C080 (thiscall): std::logic_error's destructor (its message at +0xC)
static void __fastcall LogicError_Destruct(uint8_t* self, re::Edx) {
    *reinterpret_cast<uint32_t*>(self) = kLogicErrorVtable;
    String_Tidy(self + 0xC, nullptr);
    exception_Destruct(self);
}

// 0x40C0C0 (thiscall): std::logic_error::what
static const char* __fastcall LogicError_What(uint8_t* self, re::Edx) {
    return Chars(self + 0xC);
}

// 0x40C0D0 (thiscall): std::string::assign(other, pos, count)
static uint8_t* __fastcall String_Assign_(uint8_t* self, re::Edx, uint8_t* other, uint32_t pos, uint32_t count) {
    if (Size(other) < pos) {
        Xran();
    }
    uint32_t n = Size(other) - pos;
    if (count < n) {
        n = count;
    }
    if (self == other) {
        String_Erase(self, n + pos, 0xFFFFFFFFu);
        String_Erase(self, 0, pos);
        return self;
    }
    if (Grow(self, n)) {
        memcpy_s_(Chars(self), Capacity(self), Chars(other) + pos, n);
        Size(self) = n;
        Chars(self)[n] = 0;
    }
    return self;
}

// 0x40C1B0 (thiscall): std::string::assign(chars, count) (its own characters through
// the other assign)
static uint8_t* __fastcall String_AssignChars_(uint8_t* self, re::Edx, const char* s, uint32_t count) {
    const char* data = Chars(self);
    if (data <= s && s < data + Size(self)) {
        return static_cast<uint8_t*>(String_Assign(self, self, uint32_t(s - Chars(self)), count));
    }
    if (Grow(self, count)) {
        memcpy_s_(Chars(self), Capacity(self), s, count);
        Size(self) = count;
        Chars(self)[count] = 0;
    }
    return self;
}

// 0x40C280 (thiscall): a vector's _Tidy
static void __fastcall Vector_Tidy(uint8_t* self, re::Edx) {
    if (First(self)) {
        CMemoryMgr_Free(First(self));
    }
    First(self) = nullptr;
    Last(self) = nullptr;
    End(self) = nullptr;
}

// 0x40C2B0 (thiscall): vector::erase(first, last) (the items after moved down); returns
// the iterator at `first`
static re::Ptr32<void>* __fastcall Vector_Erase_(uint8_t* self, re::Edx, re::Ptr32<void> result[2], void* cont, uint8_t* first, void* cont2,
                                       uint8_t* last) {
    if (!cont || cont != cont2) {
        InvalidParameter();
    }
    if (first != last) {
        uint8_t* end = Last(self);
        uint8_t* newLast = first + ((end - last) >> 3) * 8;
        for (uint8_t* p = last; p != end; p += 8) {
            reinterpret_cast<uint32_t*>(first + (p - last))[0] = reinterpret_cast<uint32_t*>(p)[0];
            reinterpret_cast<uint32_t*>(first + (p - last))[1] = reinterpret_cast<uint32_t*>(p)[1];
        }
        Last(self) = newLast;
    }
    result[1] = first;
    result[0] = cont;
    return result;
}

// 0x40C320 (thiscall): std::string from a C string
static uint8_t* __fastcall String_Construct(uint8_t* self, re::Edx, const char* s) {
    Capacity(self) = 0xF;
    Size(self) = 0;
    self[4] = 0;
    String_AssignChars(self, s, uint32_t(strlen(s)));
    return self;
}

// 0x40C360 (thiscall, on the allocator): uninitialized_fill_n; returns the end
static uint8_t* __fastcall Vector_UninitFill_(void* self, re::Edx, uint8_t* first, uint32_t count, const uint32_t* value) {
    UninitializedFill8(first, count, value, self, 0);
    return first + count * 8;
}

// 0x40C3A0 (thiscall): std::logic_error(message)
static uint8_t* __fastcall LogicError_Construct(uint8_t* self, re::Edx, const void* message) {
    exception_Default(self);
    *reinterpret_cast<uint32_t*>(self) = kLogicErrorVtable;
    Size(self + 0xC) = 0;
    Capacity(self + 0xC) = 0xF;
    self[0x10] = 0;
    String_Assign(self + 0xC, message, 0, 0xFFFFFFFFu);
    return self;
}

// 0x40C3E0 (thiscall): std::logic_error's deleting destructor
static uint8_t* __fastcall LogicError_Destroy(uint8_t* self, re::Edx, int flags) {
    LogicError_Destruct(self, nullptr);
    if (flags & 1) {
        CMemoryMgr_Free(self);
    }
    return self;
}

// 0x40C430 (thiscall): list::_Incsize (length_error "list<T> too long" past 0x1FFFFFFF)
static void __fastcall List_IncSize(uint8_t* self, re::Edx, uint32_t count) {
    uint32_t& size = *reinterpret_cast<uint32_t*>(self + 8);
    if (0x1FFFFFFFu - size < count) {
        uint8_t message[0x1C];
        Capacity(message) = 0xF;
        Size(message) = 0;
        message[4] = 0;
        String_AssignChars(message, "list<T> too long", 0x10);
        uint8_t error[0x28];
        exception_Default(error);
        *reinterpret_cast<uint32_t*>(error) = kLogicErrorVtable;
        Capacity(error + 0xC) = 0xF;
        Size(error + 0xC) = 0;
        error[0x10] = 0;
        String_Assign(error + 0xC, message, 0, 0xFFFFFFFFu);
        *reinterpret_cast<uint32_t*>(error) = kLengthErrorVtable;
        CxxThrowException_(error, kLengthErrorThrowInfo);
    }
    size += count;
}

// 0x40C4C0 (thiscall): std::logic_error's copy constructor
static uint8_t* __fastcall LogicError_Copy(uint8_t* self, re::Edx, uint8_t* other) {
    exception_Copy(self, other);
    *reinterpret_cast<uint32_t*>(self) = kLogicErrorVtable;
    Capacity(self + 0xC) = 0xF;
    Size(self + 0xC) = 0;
    self[0x10] = 0;
    String_Assign(self + 0xC, other + 0xC, 0, 0xFFFFFFFFu);
    return self;
}

// 0x40C500 (thiscall): std::length_error's copy constructor
static uint8_t* __fastcall LengthError_Copy(uint8_t* self, re::Edx, uint8_t* other) {
    exception_Copy(self, other);
    *reinterpret_cast<uint32_t*>(self) = kLogicErrorVtable;
    Capacity(self + 0xC) = 0xF;
    Size(self + 0xC) = 0;
    self[0x10] = 0;
    String_Assign(self + 0xC, other + 0xC, 0, 0xFFFFFFFFu);
    *reinterpret_cast<uint32_t*>(self) = kLengthErrorVtable;
    return self;
}

// 0x40C540 (thiscall, on the allocator): uninitialized_copy; returns the end
static uint8_t* __fastcall Vector_UninitCopy_(void* self, re::Edx, uint8_t* first, uint8_t* last, uint8_t* dest) {
    return UninitializedCopy8(first, last, dest, self, 0);
}

// 0x40C570 (thiscall): vector::_Insert_n(where, count, value): reallocating (half again
// the capacity, or what's needed) or moving the tail up in place
static void __fastcall Vector_InsertN_(uint8_t* self, re::Edx, void* /*cont*/, uint8_t* where, uint32_t count,
                                       const uint32_t* value) {
    uint32_t copy[2] = {value[0], value[1]};
    uint32_t capacity = First(self) ? uint32_t((End(self) - First(self)) >> 3) : 0;
    if (count == 0) {
        return;
    }
    if (0x1FFFFFFFu - Count(self) < count) {
        VectorXlen();
    }
    if (capacity < Count(self) + count) {
        capacity = (0x1FFFFFFFu - (capacity >> 1) < capacity) ? 0 : capacity + (capacity >> 1);
        if (capacity < Count(self) + count) {
            capacity = Count(self) + count;
        }
        uint8_t* p = static_cast<uint8_t*>(Allocate8(capacity, nullptr));
        uint8_t* q = Vector_UninitCopy(self, First(self), where, p);
        q = Vector_UninitFill(self, q, count, copy);
        Vector_UninitCopy(self, where, Last(self), q);
        uint32_t size = count + Count(self);
        if (First(self)) {
            CMemoryMgr_Free(First(self));
        }
        End(self) = p + capacity * 8;
        Last(self) = p + size * 8;
        First(self) = p;
        return;
    }
    uint8_t* last = Last(self);
    if (uint32_t((last - where) >> 3) < count) {
        Vector_UninitCopy(self, where, last, where + count * 8);
        Vector_UninitFill(self, Last(self), count - uint32_t((Last(self) - where) >> 3), copy);
        Last(self) += count * 8;
        Fill8(where, Last(self) - count * 8, copy);
        return;
    }
    uint8_t* from = last - count * 8;
    Last(self) = Vector_UninitCopy(self, from, last, last);
    CopyBackward8(where, from, last);
    Fill8(where, where + count * 8, copy);
}

// 0x40C7B0 (thiscall): vector::resize(n, value)
static void __fastcall Vector_Resize(uint8_t* self, re::Edx, uint32_t n, uint32_t value0, uint32_t value1) {
    uint32_t value[2] = {value0, value1};
    uint32_t size = Count(self);
    if (size < n) {
        if (First(self) > Last(self)) {
            InvalidParameter();
        }
        Vector_InsertN(self, self, Last(self), n - size, value);
        return;
    }
    if (!First(self) || n >= Count(self)) {
        return;
    }
    if (First(self) > Last(self)) {
        InvalidParameter();
    }
    uint8_t* first = First(self);
    if (first > Last(self)) {
        InvalidParameter();
    }
    uint8_t* at = first + n * 8;
    if (at > Last(self) || at < First(self)) {
        InvalidParameter();
    }
    void* result[2];
    Vector_Erase(self, result, self, at, self, Last(self));
}

// 0x40C860 (thiscall): vector(n, value)
static uint8_t* __fastcall Vector_Construct(uint8_t* self, re::Edx, uint32_t n, const uint32_t* value) {
    First(self) = nullptr;
    Last(self) = nullptr;
    End(self) = nullptr;
    if (n != 0) {
        if (n > 0x1FFFFFFF) {
            VectorXlen();
        }
        uint8_t* p = static_cast<uint8_t*>(Allocate8(n, nullptr));
        First(self) = p;
        Last(self) = p;
        End(self) = p + n * 8;
        Last(self) = Vector_UninitFill(self, First(self), n, value);
    }
    return self;
}

// 0x40C900 (thiscall): vector::assign(n, value)
static void __fastcall Vector_Assign(uint8_t* self, re::Edx, uint32_t n, const uint32_t* value) {
    uint32_t copy[2] = {value[0], value[1]};
    uint8_t* last = Last(self);
    if (First(self) > last) {
        InvalidParameter();
    }
    if (First(self) > Last(self)) {
        InvalidParameter();
    }
    void* result[2];
    Vector_Erase(self, result, self, First(self), self, last);
    if (First(self) > Last(self)) {
        InvalidParameter();
    }
    Vector_InsertN(self, self, First(self), n, copy);
}

} // namespace StlHelpers

// ---- registration ----------------------------------------------------------------------

#define C00_STL_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace StlHelpers {
C00_STL_REVERSED("std::list::Destruct", List_Destruct, 0x0040BAD0);
C00_STL_REVERSED("std::string::_Copy", String_Copy_, 0x0040BB20);
C00_STL_REVERSED("std::fill<8>", Fill8_, 0x0040BC70);
C00_STL_REVERSED("std::_Construct<8>", Construct8_, 0x0040BCA0);
C00_STL_REVERSED("std::uninitialized_copy<8>", UninitializedCopy8_, 0x0040BCC0);
C00_STL_REVERSED("std::string::_Tidy", String_Tidy, 0x0040BD30);
C00_STL_REVERSED("stdext::hash_map::find", HashMap_Find, 0x0040BD60);
C00_STL_REVERSED("std::list::_Buyhead", List_BuyHead, 0x0040BEB0);
C00_STL_REVERSED("std::list::_Buynode", List_BuyNode, 0x0040BF30);
C00_STL_REVERSED("std::copy_backward<8>", CopyBackward8_, 0x0040BFD0);
C00_STL_REVERSED("std::uninitialized_fill_n<8>", UninitializedFill8_, 0x0040C010);
C00_STL_REVERSED("std::logic_error::Destruct", LogicError_Destruct, 0x0040C080);
C00_STL_REVERSED("std::logic_error::what", LogicError_What, 0x0040C0C0);
C00_STL_REVERSED("std::string::assign", String_Assign_, 0x0040C0D0);
C00_STL_REVERSED("std::string::assign_chars", String_AssignChars_, 0x0040C1B0);
C00_STL_REVERSED("std::vector<8>::_Tidy", Vector_Tidy, 0x0040C280);
C00_STL_REVERSED("std::vector<8>::erase", Vector_Erase_, 0x0040C2B0);
C00_STL_REVERSED("std::string::Construct", String_Construct, 0x0040C320);
C00_STL_REVERSED("std::vector<8>::_Ufill", Vector_UninitFill_, 0x0040C360);
C00_STL_REVERSED("std::logic_error::Construct", LogicError_Construct, 0x0040C3A0);
C00_STL_REVERSED("std::logic_error::Destroy", LogicError_Destroy, 0x0040C3E0);
C00_STL_REVERSED("std::list::_Incsize", List_IncSize, 0x0040C430);
C00_STL_REVERSED("std::logic_error::Copy", LogicError_Copy, 0x0040C4C0);
C00_STL_REVERSED("std::length_error::Copy", LengthError_Copy, 0x0040C500);
C00_STL_REVERSED("std::vector<8>::_Ucopy", Vector_UninitCopy_, 0x0040C540);
C00_STL_REVERSED("std::vector<8>::_Insert_n", Vector_InsertN_, 0x0040C570);
C00_STL_REVERSED("std::vector<8>::resize", Vector_Resize, 0x0040C7B0);
C00_STL_REVERSED("std::vector<8>::Construct", Vector_Construct, 0x0040C860);
C00_STL_REVERSED("std::vector<8>::assign", Vector_Assign, 0x0040C900);
} // namespace StlHelpers
