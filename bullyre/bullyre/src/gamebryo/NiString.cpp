// NiString: a reference-counted, copy-on-write string. The object is one pointer (the handle) to
// the characters of a NiMalloc'd block whose 12-byte header is just before them: the block's
// size (+0), the reference count (+4, changed through the game's InterlockedIncrement /
// InterlockedDecrement import slots) and the length (+8). Buffer sizes round up through
// GetBestBufferSize. Concatenation, insertion, replacement and the like copy the block first when
// it's shared.

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using ni::sys::Decrement;
using ni::sys::Increment;

// the block's header, 12 bytes before the characters
struct Header {
    uint32_t bufferSize;   // +0x0 (the whole block, header included)
    uint32_t refCount;     // +0x4
    uint32_t length;       // +0x8
};
static_assert(sizeof(Header) == 0xC);

inline Header* HeaderOf(char* handle) {
    return reinterpret_cast<Header*>(handle - 0xC);
}

struct NiString {
    re::Ptr32<char> handle;
};

GAME_FN(char*, __cdecl, NiStrncpy, 0x00401020, char* to, size_t size, const char* from, size_t count);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// (this file's, called where modes apply)
GAME_FN(uint32_t, __cdecl, String_GetBestBufferSize, 0x00773C90, uint32_t size);
GAME_FN(void, __thiscall, String_CalcLength, 0x00773CF0, NiString* self);
GAME_FN(uint32_t, __thiscall, String_Length, 0x00773D10, NiString* self);
GAME_FN(char*, __cdecl, String_Allocate, 0x00773E30, uint32_t length);
GAME_FN(char*, __cdecl, String_AllocateAndCopy, 0x00773E70, const char* text);
GAME_FN(char*, __cdecl, String_AllocateAndCopyHandle, 0x00773EF0, char* handle);
GAME_FN(void, __cdecl, String_Deallocate, 0x00773F30, re::Ptr32<char>* handle);
GAME_FN(void, __cdecl, String_DecRefCount, 0x00773F50, re::Ptr32<char>* handle, int validate);
GAME_FN(void, __cdecl, String_Swap, 0x00773F90, re::Ptr32<char>* handle, const char* text, int validate);
GAME_FN(void, __thiscall, String_CopyHandle, 0x00774070, NiString* self, const NiString* other);
GAME_FN(NiString*, __thiscall, String_ConstructSized, 0x007740D0, NiString* self, uint32_t length);
GAME_FN(void, __thiscall, String_Destruct, 0x00774240, NiString* self);
GAME_FN(uint8_t, __thiscall, String_Resize, 0x007744D0, NiString* self, uint32_t delta);
GAME_FN(void, __thiscall, String_ResizeOwned, 0x00774710, NiString* self, uint32_t delta, int force);
GAME_FN(void, __thiscall, String_Concatenate, 0x00774770, NiString* self, const char* text);

// the length as the library inlines it (a strlen of the characters)
inline uint32_t Strlen(const char* s) {
    return uint32_t(std::strlen(s));
}

// CalcLength, inlined
inline void CalcLengthInline(char* handle) {
    if (handle != nullptr) {
        HeaderOf(handle)->length = Strlen(handle);
    }
}

// DecRefCount, inlined: the block freed when the count it re-reads is 0; the handle cleared
inline void DecRefCountInline(re::Ptr32<char>* handle) {
    if (*handle != nullptr) {
        uint32_t* count = &HeaderOf(*handle)->refCount;
        Decrement(count);
        if (*count == 0 && *handle != nullptr) {
            NiFree(*handle - 0xC);
            *handle = nullptr;
        }
        *handle = nullptr;
    }
}

// the old block let go after the handle moved to a copy (the count re-read, as inlined)
inline void ReleaseOld(char* old) {
    if (old != nullptr) {
        Header* header = HeaderOf(old);
        Decrement(&header->refCount);
        if (header->refCount == 0) {
            NiFree(header);
        }
    }
}

// a shared block copied before a write (AllocateAndCopyHandle, inlined)
inline void CopyOnWriteInline(NiString* self) {
    char* old = self->handle;
    if (HeaderOf(old)->refCount > 1) {
        uint32_t length = HeaderOf(old)->length;
        uint32_t size = HeaderOf(old)->bufferSize;
        Header* header = static_cast<Header*>(NiMalloc(size));
        header->bufferSize = size;
        header->refCount = 1;
        header->length = length;
        char* chars = reinterpret_cast<char*>(header + 1);
        crt::MemcpyS(chars, length + 1, old, length + 1);
        self->handle = chars;
        ReleaseOld(old);
    }
}

// ResizeOwned(delta, false), inlined: the block grown, or copied when shared
inline void ResizeOwnedInline(NiString* self, uint32_t delta) {
    if (!String_Resize(self, delta)) {
        char* old = self->handle;
        if (old != nullptr && HeaderOf(old)->refCount > 1) {
            self->handle = String_AllocateAndCopyHandle(old);
            ReleaseOld(old);
        }
    }
}

// ---- the blocks

// GetBestBufferSize(size): the steps 0x20, 0x40, 0x80, 0xFF (sic), 0x200, 0x400; beyond, one more
uint32_t __cdecl GetBestBufferSize(uint32_t size) {
    if (size < 0x20) {
        return 0x20;
    }
    if (size < 0x40) {
        return 0x40;
    }
    if (size < 0x80) {
        return 0x80;
    }
    if (size < 0xFF) {
        return 0xFF;
    }
    if (size < 0x200) {
        return 0x200;
    }
    if (size < 0x400) {
        return 0x400;
    }
    return size + 1;
}

// Allocate(length): an empty block (count 1, length 0) for that many characters
char* __cdecl Allocate(uint32_t length) {
    if (length == 0) {
        length = 1;
    }
    uint32_t size = String_GetBestBufferSize(length + 0xD);
    Header* header = static_cast<Header*>(NiMalloc(size));
    header->bufferSize = size;
    header->refCount = 1;
    header->length = 0;
    return reinterpret_cast<char*>(header + 1);
}

// AllocateAndCopy(text): a new block holding the text (none for null)
char* __cdecl AllocateAndCopy(const char* text) {
    if (text == nullptr) {
        return nullptr;
    }
    uint32_t length = Strlen(text);
    uint32_t size = String_GetBestBufferSize((length != 0 ? length : 1) + 0xD);
    Header* header = static_cast<Header*>(NiMalloc(size));
    header->bufferSize = size;
    char* chars = reinterpret_cast<char*>(header + 1);
    header->refCount = 1;
    header->length = 0;
    if (chars == nullptr) {
        return nullptr;
    }
    crt::MemcpyS(chars, length + 1, text, length + 1);
    HeaderOf(chars)->length = length;
    return chars;
}

// AllocateAndCopyHandle(handle): a private copy of a block (same size)
char* __cdecl AllocateAndCopyHandle(char* handle) {
    if (handle == nullptr) {
        return nullptr;
    }
    uint32_t size = HeaderOf(handle)->bufferSize;
    uint32_t length = HeaderOf(handle)->length;
    Header* header = static_cast<Header*>(NiMalloc(size));
    header->bufferSize = size;
    char* chars = reinterpret_cast<char*>(header + 1);
    header->refCount = 1;
    header->length = length;
    crt::MemcpyS(chars, length + 1, handle, length + 1);
    return chars;
}

// Deallocate(handle&)
void __cdecl Deallocate(re::Ptr32<char>* handle) {
    if (*handle != nullptr) {
        NiFree(*handle - 0xC);
        *handle = nullptr;
    }
}

// DecRefCount(handle&, bValidate): the second argument isn't used
void __cdecl DecRefCount(re::Ptr32<char>* handle, int) {
    DecRefCountInline(handle);
}

// Swap(handle&, text, bValidate): the handle set to the text, in place when this was the only
// reference and the block is big enough
void __cdecl Swap(re::Ptr32<char>* handle, const char* text, int) {
    if (text == nullptr) {
        DecRefCountInline(handle);
        return;
    }
    char* chars = *handle;
    if (chars == nullptr) {
        *handle = String_AllocateAndCopy(text);
        return;
    }
    Header* header = HeaderOf(chars);
    Decrement(&header->refCount);
    if (header->refCount != 0) {
        *handle = String_AllocateAndCopy(text);
        return;
    }
    uint32_t length = Strlen(text);
    if (header->bufferSize < length + 0xD) {
        String_Deallocate(handle);
        *handle = String_Allocate(length);
        header = HeaderOf(*handle);
    } else {
        header->refCount = 1;
    }
    crt::MemcpyS(*handle, length + 1, text, length + 1);
    header->length = length;
}

// ---- the object

// CalcLength
void __fastcall CalcLength(NiString* self, re::Edx) {
    CalcLengthInline(self->handle);
}

uint32_t __fastcall Length(NiString* self, re::Edx) {
    return self->handle != nullptr ? HeaderOf(self->handle)->length : 0;
}

uint8_t __fastcall IsEmpty(NiString* self, re::Edx) {
    if (self->handle == nullptr) {
        return 1;
    }
    return HeaderOf(self->handle)->length == 0 ? 1 : 0;
}

char __fastcall GetAt(NiString* self, re::Edx, uint32_t index) {
    char* chars = self->handle;
    if (chars == nullptr || index >= HeaderOf(chars)->length) {
        return 0;
    }
    return chars[index];
}

// strcmp's result for two strings (bytes compared unsigned); with a null on one side, the other's
// first character (signed)
inline int CompareChars(const char* a, const char* b) {
    if (b == nullptr) {
        return a != nullptr ? int(static_cast<signed char>(*a)) : 0;
    }
    if (a == nullptr) {
        return -int(static_cast<signed char>(*b));
    }
    const uint8_t* p = reinterpret_cast<const uint8_t*>(a);
    const uint8_t* q = reinterpret_cast<const uint8_t*>(b);
    for (;;) {
        if (p[0] != q[0]) {
            return p[0] < q[0] ? -1 : 1;
        }
        if (p[0] == 0) {
            return 0;
        }
        if (p[1] != q[1]) {
            return p[1] < q[1] ? -1 : 1;
        }
        p += 2;
        q += 2;
        if (p[-1] == 0) {
            return 0;
        }
    }
}

int __fastcall Compare(NiString* self, re::Edx, const char* text) {
    return CompareChars(self->handle, text);
}

// operator const char* (3 bytes: unhooked)
const char* __fastcall Chars(NiString* self, re::Edx) {
    return self->handle;
}

// operator==(const NiString&, const char*)
uint8_t __cdecl EqualsText(const NiString* string, const char* text) {
    return CompareChars(string->handle, text) == 0 ? 1 : 0;
}

// the handle shared with another string's (operator='s body)
void __fastcall CopyHandle(NiString* self, re::Edx, const NiString* other) {
    if (other->handle != self->handle) {
        DecRefCountInline(&self->handle);
        if (other->handle != nullptr) {
            Increment(&HeaderOf(other->handle)->refCount);
        }
        self->handle = other->handle;
    }
}

// NiString(length): an empty string with room for that many
NiString* __fastcall ConstructSized(NiString* self, re::Edx, uint32_t length) {
    if (length == 0) {
        length = 1;
    }
    uint32_t size = String_GetBestBufferSize(length + 0xD);
    Header* header = static_cast<Header*>(NiMalloc(size));
    header->bufferSize = size;
    header->refCount = 1;
    header->length = 0;
    self->handle = reinterpret_cast<char*>(header + 1);
    self->handle[0] = 0;
    CalcLengthInline(self->handle);
    return self;
}

// NiString(int length): negative counts as 0
NiString* __fastcall ConstructInt(NiString* self, re::Edx, int length) {
    uint32_t n = length < 0 ? 0 : uint32_t(length);
    if (n == 0) {
        n = 1;
    }
    uint32_t size = String_GetBestBufferSize(n + 0xD);
    Header* header = static_cast<Header*>(NiMalloc(size));
    header->bufferSize = size;
    header->refCount = 1;
    header->length = 0;
    self->handle = reinterpret_cast<char*>(header + 1);
    self->handle[0] = 0;
    CalcLengthInline(self->handle);
    return self;
}

// NiString(const char*)
NiString* __fastcall ConstructText(NiString* self, re::Edx, const char* text) {
    self->handle = String_AllocateAndCopy(text);
    CalcLengthInline(self->handle);
    return self;
}

// NiString(const NiFixedString&)
NiString* __fastcall ConstructFixed(NiString* self, re::Edx, const re::Ptr32<const char>* text) {
    self->handle = String_AllocateAndCopy(*text);
    CalcLengthInline(self->handle);
    return self;
}

// NiString(const NiString&): the block shared
NiString* __fastcall ConstructCopy(NiString* self, re::Edx, const NiString* other) {
    char* chars = other->handle;
    self->handle = chars;
    if (chars != nullptr) {
        Increment(&HeaderOf(chars)->refCount);
    }
    return self;
}

void __fastcall Destruct(NiString* self, re::Edx) {
    DecRefCountInline(&self->handle);
}

// operator=(const NiString&)
NiString* __fastcall AssignString(NiString* self, re::Edx, const NiString* other) {
    if (other->handle != self->handle) {
        String_CopyHandle(self, other);
    }
    return self;
}

// operator=(const char*)
NiString* __fastcall AssignText(NiString* self, re::Edx, const char* text) {
    if (text != self->handle) {
        String_Swap(&self->handle, text, 1);
    }
    return self;
}

// operator=(char): a one-character string (built in the argument's own stack slot)
NiString* __fastcall AssignChar(NiString* self, re::Edx, char c) {
    char text[2] = {c, 0};
    if (text != self->handle) {
        String_Swap(&self->handle, text, 1);
    }
    return self;
}

// SetAt(index, c): a terminator written there shortens the length
void __fastcall SetAt(NiString* self, re::Edx, uint32_t index, char c) {
    char* chars = self->handle;
    if (chars == nullptr || index >= HeaderOf(chars)->length) {
        return;
    }
    CopyOnWriteInline(self);
    self->handle[index] = c;
    if (c == 0 && self->handle != nullptr) {
        HeaderOf(self->handle)->length = index;
    }
}

// GetSubstring(begin, end): [begin, end) (empty when out of range)
NiString* __fastcall GetSubstring(NiString* self, re::Edx, NiString* result, uint32_t begin, uint32_t end) {
    if (begin < end && self->handle != nullptr) {
        uint32_t length = HeaderOf(self->handle)->length;
        if (begin < length && end <= length) {
            NiString part;
            String_ConstructSized(&part, end - begin + 2);
            char* chars = part.handle;
            uint32_t room = chars == nullptr ? 0 : HeaderOf(chars)->bufferSize - 0xC;
            NiStrncpy(chars, room - 1, self->handle + begin, end - begin);
            chars[end - begin] = 0;
            String_CalcLength(&part);
            result->handle = chars;
            if (chars != nullptr) {
                Increment(&HeaderOf(chars)->refCount);
            }
            String_Destruct(&part);
            return result;
        }
    }
    result->handle = nullptr;
    return result;
}

// Find(c, start): the index of the first c from start, -1 for none
int __fastcall FindChar(NiString* self, re::Edx, char c, uint32_t start) {
    char* chars = self->handle;
    if (chars != nullptr && start < HeaderOf(chars)->length) {
        char* found = crt::Strchr(chars + start, c);
        if (found != nullptr && found - self->handle >= 0) {
            return int(found - self->handle);
        }
    }
    return -1;
}

// FindReverse(c): the last c (never the terminator), -1 for none
int __fastcall FindReverse(NiString* self, re::Edx, char c) {
    if (self->handle == nullptr) {
        return -1;
    }
    char* found = crt::Strrchr(self->handle, c);
    if (found == nullptr || c == 0) {
        return -1;
    }
    int index = int(found - self->handle);
    return index < 0 ? -1 : index;
}

// ToLower: A..Z only (Length called for every character, through its address)
void __fastcall ToLower(NiString* self, re::Edx) {
    if (self->handle == nullptr) {
        return;
    }
    CopyOnWriteInline(self);
    for (uint32_t i = 0; i < String_Length(self); i++) {
        char* p = self->handle + i;
        if (uint8_t(*p - 'A') <= 0x19) {
            *p = char(*p + 0x20);
        }
    }
}

// Resize(delta): a bigger private block when the length plus delta doesn't fit; true when it
// made one
uint8_t __fastcall Resize(NiString* self, re::Edx, uint32_t delta) {
    char* chars = self->handle;
    uint32_t newLength = (chars == nullptr ? 0 : HeaderOf(chars)->length) + delta;
    uint32_t room = chars == nullptr ? 0 : HeaderOf(chars)->bufferSize - 0xC;
    if (newLength + 1 <= room) {
        return 0;
    }
    uint32_t size = String_GetBestBufferSize((newLength != 0 ? newLength : 1) + 0xD);
    Header* header = static_cast<Header*>(NiMalloc(size));
    header->bufferSize = size;
    header->refCount = 1;
    header->length = 0;
    char* copy = reinterpret_cast<char*>(header + 1);
    chars = self->handle;
    uint32_t copied = chars != nullptr ? HeaderOf(chars)->length : 0;
    crt::MemcpyS(copy, newLength, chars, chars == nullptr ? 0 : HeaderOf(chars)->length);
    copy[copied] = 0;
    DecRefCountInline(&self->handle);
    self->handle = copy;
    CalcLengthInline(copy);
    return 1;
}

// RemoveRange(start, count)
void __fastcall RemoveRange(NiString* self, re::Edx, uint32_t start, uint32_t count) {
    char* chars = self->handle;
    if (chars == nullptr) {
        return;
    }
    uint32_t length = HeaderOf(chars)->length;
    if (start >= length) {
        return;
    }
    if (count + start > length) {
        count = length - start;
    }
    CopyOnWriteInline(self);
    char* at = self->handle + start;
    crt::MemmoveS(at, length - start + 1, at + count, length - start - count + 1);
    CalcLengthInline(self->handle);
}

// the block grown by delta, or (when Resize didn't) copied when it's shared or `force` says so
void __fastcall ResizeOwned(NiString* self, re::Edx, uint32_t delta, int force) {
    if (String_Resize(self, delta)) {
        return;
    }
    char* old = self->handle;
    if ((old != nullptr && HeaderOf(old)->refCount > 1) || uint8_t(force) != 0) {
        self->handle = String_AllocateAndCopyHandle(old);
        ReleaseOld(old);
    }
}

// Concatenate(text): appending the string to itself keeps the old block alive meanwhile
void __fastcall Concatenate(NiString* self, re::Edx, const char* text) {
    if (text == nullptr) {
        return;
    }
    uint32_t length = Strlen(text);
    if (length == 0) {
        return;
    }
    re::Ptr32<char> old = self->handle;
    bool own = false;
    if (text == old) {
        if (old != nullptr) {
            Increment(&HeaderOf(old)->refCount);
        }
        String_ResizeOwned(self, length, 1);
        own = true;
    } else {
        ResizeOwnedInline(self, length);
    }
    char* chars = self->handle;
    uint32_t at = chars == nullptr ? 0 : HeaderOf(chars)->length;
    crt::StrcpyS(chars + at, length + 1, text);
    CalcLengthInline(self->handle);
    if (own) {
        String_DecRefCount(&old, 1);
    }
}

// operator+=(const char*)
NiString* __fastcall AppendText(NiString* self, re::Edx, const char* text) {
    String_Concatenate(self, text);
    return self;
}

// operator+=(const NiString&)
NiString* __fastcall AppendString(NiString* self, re::Edx, const NiString* other) {
    String_Concatenate(self, other->handle);
    return self;
}

// Replace(what, with): every occurrence, counted first (the search is Find(what, start) inlined)
void __fastcall Replace(NiString* self, re::Edx, const char* what, const char* with) {
    if (what == nullptr || with == nullptr) {
        return;
    }
    uint32_t whatLength = Strlen(what);
    uint32_t withLength = Strlen(with);
    uint32_t count = 0;
    uint32_t length = self->handle == nullptr ? 0 : HeaderOf(self->handle)->length;
    uint32_t at = 0;
    if (length == 0) {
        return;
    }
    do {
        if (at == 0xFFFFFFFF) {
            break;
        }
        char* chars = self->handle;
        if (chars == nullptr || at >= HeaderOf(chars)->length || *what == 0) {
            at = 0xFFFFFFFF;
        } else {
            char* found = crt::Strstr(chars + at, what);
            if (found == nullptr) {
                at = 0xFFFFFFFF;
            } else {
                int index = int(found - self->handle);
                if (index < 0) {
                    at = 0xFFFFFFFF;
                } else {
                    at = uint32_t(index);
                    if (at != 0xFFFFFFFF) {
                        count++;
                        at += whatLength;
                    }
                }
            }
        }
    } while (at < length);
    if (count == 0) {
        return;
    }
    uint32_t newLength = (withLength - whatLength) * count + length;
    if (newLength > length) {
        ResizeOwnedInline(self, newLength - length);
    } else if (self->handle != nullptr) {
        CopyOnWriteInline(self);
    }
    uint32_t offset = 0;
    while (offset < newLength && count != 0) {
        char* found = crt::Strstr(self->handle + offset, what);
        char* rest = found + whatLength;
        std::memmove(found + withLength, rest, Strlen(rest) + 1);   // memmove (0x85E490)
        crt::Memcpy(found, with, withLength);                         // memcpy (0x85D080)
        count--;
        offset = uint32_t(found - self->handle) + withLength;
    }
    self->handle[newLength] = 0;
    CalcLengthInline(self->handle);
}

// Insert(text, index): at the end when the index is past it
void __fastcall Insert(NiString* self, re::Edx, const char* text, uint32_t index) {
    if (text == nullptr) {
        return;
    }
    uint32_t length = self->handle == nullptr ? 0 : HeaderOf(self->handle)->length;
    uint32_t textLength = Strlen(text);
    if (index >= length) {
        String_Concatenate(self, text);
        return;
    }
    uint32_t total = length + textLength + 1;
    ResizeOwnedInline(self, textLength);
    char* at = self->handle + index;
    crt::MemmoveS(self->handle + textLength + index, total - textLength - index, at, length - index + 1);
    crt::MemmoveS(at, total - index, text, textLength);
    CalcLengthInline(self->handle);
}

// FromInt(value): "%d"
NiString* __cdecl FromInt(NiString* result, int value) {
    char buffer[0x20];
    NiSprintf(buffer, 0x20, "%d", value);
    result->handle = String_AllocateAndCopy(buffer);
    CalcLengthInline(result->handle);
    return result;
}

} // namespace

// 0x00773DB0: [8b 01 c3 cc cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_00773DB0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x0);
}
RE_UNHOOKED_FN("NiString::operator const char*", 0x00773DB0, &Tiny_00773DB0);   // 3 bytes (Chars above)

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTRING_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTRING_REVERSED("NiString::GetBestBufferSize", 0x00773C90, &ni::GetBestBufferSize);
NISTRING_REVERSED("NiString::CalcLength", 0x00773CF0, &ni::CalcLength);
// (callers rely on edx: ToLower 0x774430 keeps its loop counter there across the call; turn on
// together with NiString::ToLower)
NISTRING_REVERSED("NiString::Length", 0x00773D10, &ni::Length);
NISTRING_REVERSED("NiString::IsEmpty", 0x00773D20, &ni::IsEmpty);
NISTRING_REVERSED("NiString::GetAt", 0x00773D40, &ni::GetAt);
NISTRING_REVERSED("NiString::Compare", 0x00773D60, &ni::Compare);
NISTRING_REVERSED("NiString::operator==", 0x00773DC0, &ni::EqualsText);
NISTRING_REVERSED("NiString::Allocate", 0x00773E30, &ni::Allocate);
NISTRING_REVERSED("NiString::AllocateAndCopy", 0x00773E70, &ni::AllocateAndCopy);
NISTRING_REVERSED("NiString::AllocateAndCopyHandle", 0x00773EF0, &ni::AllocateAndCopyHandle);
NISTRING_REVERSED("NiString::Deallocate", 0x00773F30, &ni::Deallocate);
NISTRING_REVERSED("NiString::DecRefCount", 0x00773F50, &ni::DecRefCount);
NISTRING_REVERSED("NiString::Swap", 0x00773F90, &ni::Swap);
NISTRING_REVERSED("NiString::CopyHandle", 0x00774070, &ni::CopyHandle);
NISTRING_REVERSED("NiString::NiString(size_t)", 0x007740D0, &ni::ConstructSized);
NISTRING_REVERSED("NiString::NiString(int)", 0x00774130, &ni::ConstructInt);
NISTRING_REVERSED("NiString::NiString(const char*)", 0x007741A0, &ni::ConstructText);
NISTRING_REVERSED("NiString::NiString(const NiFixedString&)", 0x007741E0, &ni::ConstructFixed);
NISTRING_REVERSED("NiString::NiString(const NiString&)", 0x00774220, &ni::ConstructCopy);
NISTRING_REVERSED("NiString::~NiString", 0x00774240, &ni::Destruct);
NISTRING_REVERSED("NiString::SetAt", 0x00774280, &ni::SetAt);
NISTRING_REVERSED("NiString::GetSubstring", 0x00774310, &ni::GetSubstring);
NISTRING_REVERSED("NiString::Find", 0x007743B0, &ni::FindChar);
NISTRING_REVERSED("NiString::FindReverse", 0x007743F0, &ni::FindReverse);
NISTRING_REVERSED("NiString::ToLower", 0x00774430, &ni::ToLower);
NISTRING_REVERSED("NiString::Resize", 0x007744D0, &ni::Resize);
NISTRING_REVERSED("NiString::operator=(const NiString&)", 0x007745C0, &ni::AssignString);
NISTRING_REVERSED("NiString::operator=(const char*)", 0x007745E0, &ni::AssignText);
NISTRING_REVERSED("NiString::operator=(char)", 0x00774600, &ni::AssignChar);
NISTRING_REVERSED("NiString::RemoveRange", 0x00774630, &ni::RemoveRange);
NISTRING_REVERSED("NiString::ResizeOwned", 0x00774710, &ni::ResizeOwned);
NISTRING_REVERSED("NiString::Concatenate", 0x00774770, &ni::Concatenate);
NISTRING_REVERSED("NiString::operator+=(const char*)", 0x00774870, &ni::AppendText);
NISTRING_REVERSED("NiString::operator+=(const NiString&)", 0x00774890, &ni::AppendString);
NISTRING_REVERSED("NiString::Replace", 0x007748B0, &ni::Replace);
NISTRING_REVERSED("NiString::Insert", 0x00774AE0, &ni::Insert);
NISTRING_REVERSED("NiString::FromInt", 0x00774BF0, &ni::FromInt);
