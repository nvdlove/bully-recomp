#pragma once

// The registry of reversed functions. Each one replaces the original at its address,
// in one of three modes (bullyre.ini next to the game, "name = mode"):
//   original  the game's own code runs (ours is not installed)
//   shadow    both run on every call and the results are compared (mismatches go
//             to bullyre_log.txt); the game keeps using the original's result.
//             Only for functions whose shadow can run both safely
//   ours      our code runs instead of the original
//
// Register with REVERSED(...) at namespace scope next to the function.

#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

struct _RTL_CRITICAL_SECTION;   // (windows.h's CRITICAL_SECTION)

namespace re {

enum class Mode { Original, Shadow, Ours };

// The edx slot of a body written `__fastcall F(Self* self, re::Edx, args...)` for one of the
// game's __thiscall functions (this in ecx, the arguments on the stack). On Win32 it is the
// register's leftover (a void*: the hook is a __fastcall function in the thiscall's place).
// On x64 every convention is the same one, so a caller of the game's address passes (self,
// args...): the registration makes an adapter that puts the placeholder back (re::BodyOf).
#if defined(_WIN64)
struct Edx {
    Edx() = default;
    Edx(std::nullptr_t) {}
    Edx(const void*) {}   // (a body passing its own edx on)
    operator void*() const { return nullptr; }
};
#else
using Edx = void*;
#endif

// A registered body: ours (what our code and re::Ours call) and, on x64, the entry a caller of
// the game's address reaches (ours, or the adapter of a body with re::Edx). The same pointer
// on Win32.
struct Body {
    void* ours = nullptr;
    void* entry = nullptr;
    Body() = default;
    Body(std::nullptr_t) {}
    Body(void* p) : ours(p), entry(p) {}
    Body(void* o, void* e) : ours(o), entry(e) {}
#if defined(_WIN64)
    // A body taking re::Edx reaches x64 callers only through its adapter (BodyOf): register
    // it as RE_BODY(&fn). (Without this, MSVC turns `&fn` into a void* and the callers'
    // arguments land one register early.)
    template <typename R, typename S, typename... A>
    Body(R (*)(S, Edx, A...)) = delete;
#endif
    operator void*() const { return ours; }
};

struct Function {
    const char* name;
    uintptr_t address;
    Body ours;
    void* shadow;     // nullptr: no shadow mode
    void** original;  // set to a trampoline to the game's code (callable in any mode)
    Mode mode;        // the default, until bullyre.ini says otherwise
    bool unhooked = false;   // written but never installed (RE_UNHOOKED_FN): only for OursAt
};

bool Register(const Function& f, const char* file = nullptr);

// Our function for a game address (the registered `ours`, hooked or not), or nullptr: what
// the 64-bit build calls where the game's code called the address (the register-argument
// calls the 32-bit build makes in asm; GAME_FN, in time). Logs a miss.
void* OursAt(uintptr_t address);
template <typename F>
F Ours(uintptr_t address) {
    return reinterpret_cast<F>(OursAt(address));
}
// What a caller of the game's address reaches (Body::entry), or nullptr: the 64-bit dispatch
// (src/x64/dispatch.cpp) puts a jump to it at the address.
void* EntryAt(uintptr_t address);

// A critical section in the game's memory (0x18 bytes in its layouts: x86's CRITICAL_SECTION) as
// the system's: itself in the 32-bit build; on x64 (where one is 0x28 bytes) a real one of ours,
// its pointer kept in the game's first 4 bytes, made on first use (src/reversed.cpp).
// DeleteGameSection deletes it.
#if defined(_WIN64)
_RTL_CRITICAL_SECTION* GameSection(void* section);
void DeleteGameSection(void* section);
#else
inline _RTL_CRITICAL_SECTION* GameSection(void* section) {
    return static_cast<_RTL_CRITICAL_SECTION*>(section);
}
void DeleteGameSection(void* section);
#endif

// Everything registered, in registration order (the 64-bit dispatch walks it).
size_t RegisteredCount();
const Function& Registered(size_t index);

// Installs everything registered (from DllMain, before the game runs).
void InstallAll();

// Everything bullyre does before the game's code runs: the log, InstallAll as bullyre.ini
// says, the testing traps (src/startup.cpp). From DllMain, or from the host exe once it has
// mapped the game.
void Startup();

// Shadow mode found a difference.
void Mismatch(const char* name, const char* format, ...);

// Shadow mode compared one more call (logged at 1, 10, 100, ... calls).
void Compared(const char* name, uint32_t& count);

// A number from bullyre.ini ("key = 123" or hex), for testing (e.g. bisecting which
// cases of a function run ours); `fallback` when it's not there.
int IniInt(const char* key, int fallback);

} // namespace re

// The game's flags and "bool" returns are bytes that mean true when non-zero (its
// code tests the low byte): never C++ bool across the boundary, whose 0/1 the
// compiler may rely on. Game functions return uint8_t; flags the game passes in
// arrive as int and are read with Flag().
namespace re {
inline bool Flag(int value) {
    return uint8_t(value) != 0;
}
} // namespace re

// A pointer field of a struct laid out as the game's. The game's data layout keeps its
// 4-byte pointers in the 64-bit build (the data is mapped below 4 GB, so the 32 bits are
// the address itself); on Win32 it's the pointer type. Structs declare their pointer
// members as re::Ptr32<T> so their static_asserts hold on both.
namespace re {
#if defined(_WIN64)
// A pointer stored into a 4-byte field must fit: below 4 GB, or one of the system's negative
// values ((void*)-1, handles). The 64-bit build is large-address-aware: what the game's data
// points at comes from the low heap (src/x64/lowheap.cpp, below 2 GB), and a pointer from
// anywhere else stops the game right at the store, with this exception (parameter: the
// pointer) instead of a truncated pointer used later. Nothing is ever allocated between 2 and
// 4 GB (the low heap holds that band empty), so a value there is a 32-bit number passed as a
// pointer (a map's key, flags), kept as it is.
constexpr unsigned long kPointerTooWide = 0xE0B6A002;
[[noreturn]] void PointerTooWide(uintptr_t value);   // src/x64/lowheap.cpp
inline uint32_t Narrow32(uintptr_t value) {
    if ((value >> 32) != 0 && uintptr_t(intptr_t(int32_t(uint32_t(value)))) != value) [[unlikely]] {
        PointerTooWide(value);
    }
    return uint32_t(value);
}

template <typename T>
struct Ptr32 {
    uint32_t bits = 0;
    using element_type = T;
    Ptr32() = default;
    Ptr32(T* p) : bits(Narrow32(reinterpret_cast<uintptr_t>(p))) {}
    Ptr32(std::nullptr_t) : bits(0) {}
    // the conversions the pointer has (Ptr32<Derived> to Ptr32<Base>, Ptr32<char> to
    // Ptr32<const char>, any to Ptr32<void>)
    template <typename U>
        requires(!std::is_same_v<U, T> && std::is_convertible_v<U*, T*>)
    Ptr32(const Ptr32<U>& o) : bits(o.bits) {}
    template <typename U>
        requires(!std::is_same_v<U, T> && std::is_convertible_v<U*, T*>)
    Ptr32& operator=(const Ptr32<U>& o) {
        bits = o.bits;
        return *this;
    }
    // (sign-extended, as the system widens a 32-bit handle (LongToHandle): this process's
    // pointers are all below 2 GB, so they read back unchanged, while the values with the top
    // bit set come back as the system made them: D3DX's handles (~pointer), (void*)-1 and
    // INVALID_HANDLE_VALUE)
    T* get() const { return reinterpret_cast<T*>(intptr_t(int32_t(bits))); }
    operator T*() const { return get(); }
    // static_cast<U*>(p): the pointer as another type (related types as a static_cast of the
    // pointer would, others as a reinterpret_cast: what the same cast of a T* allows)
    template <typename U>
    explicit operator U*() const requires(!std::is_same_v<U, T>) {
        if constexpr (std::is_void_v<T> || std::is_void_v<U> || std::is_base_of_v<std::remove_cv_t<T>, std::remove_cv_t<U>> ||
                      std::is_base_of_v<std::remove_cv_t<U>, std::remove_cv_t<T>>) {
            return static_cast<U*>(get());
        } else {
            return reinterpret_cast<U*>(get());
        }
    }
    T* operator->() const { return get(); }
    std::add_lvalue_reference_t<T> operator*() const { return *get(); }
    explicit operator uintptr_t() const { return bits; }   // (uintptr_t(p): the address)
    Ptr32& operator=(T* p) {
        bits = Narrow32(reinterpret_cast<uintptr_t>(p));
        return *this;
    }
    explicit operator bool() const { return bits != 0; }
    // the pointer's subscript and arithmetic (MSVC doesn't reach them through the conversion)
    std::add_lvalue_reference_t<T> operator[](ptrdiff_t i) const requires(!std::is_void_v<T> && !std::is_function_v<T>) {
        return get()[i];
    }
    T* operator+(ptrdiff_t n) const requires(!std::is_void_v<T> && !std::is_function_v<T>) { return get() + n; }
    T* operator-(ptrdiff_t n) const requires(!std::is_void_v<T> && !std::is_function_v<T>) { return get() - n; }
    friend ptrdiff_t operator-(const Ptr32& a, const Ptr32& b) requires(!std::is_void_v<T> && !std::is_function_v<T>) {
        return a.get() - b.get();
    }
    friend ptrdiff_t operator-(const Ptr32& a, T* b) requires(!std::is_void_v<T> && !std::is_function_v<T>) { return a.get() - b; }
    friend ptrdiff_t operator-(T* a, const Ptr32& b) requires(!std::is_void_v<T> && !std::is_function_v<T>) { return a - b.get(); }
    // comparisons (== gives != too)
    friend bool operator==(const Ptr32& a, const Ptr32& b) { return a.bits == b.bits; }
    friend bool operator==(const Ptr32& a, std::nullptr_t) { return a.bits == 0; }
    template <typename U>
    friend bool operator==(const Ptr32& a, U* b) {
        return a.get() == b;
    }
    template <typename U>
    friend bool operator==(const Ptr32& a, const Ptr32<U>& b) requires(!std::is_same_v<U, T>) {
        return a.get() == b.get();
    }
    friend auto operator<=>(const Ptr32& a, const Ptr32& b) { return a.get() <=> b.get(); }
    template <typename U>
    friend auto operator<=>(const Ptr32& a, U* b) {
        return a.get() <=> b;
    }
    // pointer arithmetic on the field itself (p += n, p++): in units of T
    Ptr32& operator+=(ptrdiff_t n) requires(!std::is_void_v<T> && !std::is_function_v<T>) { bits = uint32_t(bits + n * ptrdiff_t(sizeof(T))); return *this; }
    Ptr32& operator-=(ptrdiff_t n) requires(!std::is_void_v<T> && !std::is_function_v<T>) { bits = uint32_t(bits - n * ptrdiff_t(sizeof(T))); return *this; }
    Ptr32& operator++() requires(!std::is_void_v<T> && !std::is_function_v<T>) { return *this += 1; }
    Ptr32& operator--() requires(!std::is_void_v<T> && !std::is_function_v<T>) { return *this -= 1; }
    Ptr32 operator++(int) requires(!std::is_void_v<T> && !std::is_function_v<T>) { Ptr32 old = *this; *this += 1; return old; }
    Ptr32 operator--(int) requires(!std::is_void_v<T> && !std::is_function_v<T>) { Ptr32 old = *this; *this -= 1; return old; }
};
static_assert(sizeof(Ptr32<int>) == 4);
template <typename X>
struct IsPtr32 : std::false_type {};
template <typename U>
struct IsPtr32<Ptr32<U>> : std::true_type {};
#else
template <typename T>
using Ptr32 = T*;
template <typename X>
struct IsPtr32 : std::false_type {};
inline uint32_t Narrow32(uintptr_t value) {
    return uint32_t(value);
}
#endif
// a pointer as the 4 bytes a game field holds (checked on x64: re::Narrow32)
inline uint32_t Addr32(const void* p) {
    return Narrow32(reinterpret_cast<uintptr_t>(p));
}

// A deduced argument's type as a call through a function pointer passes it (the files' call
// helpers: `reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(slot)(object, args...)`): a
// Ptr32 as its pointer. On x64 a Ptr32 by value is a 4-byte struct, its register's upper half
// undefined, where the callee reads a pointer. The type itself on Win32.
template <typename T, bool = IsPtr32<std::remove_cvref_t<T>>::value>
struct ArgOf {
#if defined(_M_X64)
    // (a struct by value through a game call: on x86 its bytes were stack slots, the callee's
    // floats or ints; on x64 it goes by hidden pointer (or a register, up to 8 bytes): pass its
    // members as the body takes them)
    static_assert(!std::is_class_v<std::remove_cvref_t<T>> || sizeof(T) <= 4, "a struct by value through a game call");
#endif
    using type = T;
};
template <typename T>
struct ArgOf<T, true> {
    using type = typename std::remove_cvref_t<T>::element_type*;
};
template <typename T>
using Arg = typename ArgOf<T>::type;

// A pointer reinterpreted (reinterpret_cast<To>(p) for a pointer; a Ptr32 can't be the
// operand of one): re::PtrCast<uint8_t*>(self->next)
template <typename To, typename T>
To PtrCast(T* p) {
    if constexpr (std::is_integral_v<To>) {
        return To(reinterpret_cast<uintptr_t>(p));
    } else if constexpr (IsPtr32<To>::value) {
        return To((typename To::element_type*)(p));   // (a Ptr32 target: the pointer, reinterpreted)
    } else {
        return (To)(p);   // (reinterpret_cast, const_cast or both: what the site's cast did)
    }
}
template <typename To, typename T>
    requires std::is_integral_v<T>
To PtrCast(T address) {
    if constexpr (std::is_integral_v<To>) {
        return To(address);
    } else if constexpr (IsPtr32<To>::value) {
        return To(reinterpret_cast<typename To::element_type*>(uintptr_t(address)));
    } else {
        return reinterpret_cast<To>(uintptr_t(address));
    }
}
#if defined(_WIN64)
template <typename To, typename T>
To PtrCast(Ptr32<T> p) {
    return PtrCast<To>(p.get());
}
#endif

// A field of a game object by offset: At<T>(object, 0x14). The one definition for the
// files' own copies (all the same: base + offset, reinterpreted). A pointer field
// (At<T*>) is the game's 4-byte one: on x64 it's read and written as a Ptr32<T>.
template <typename T>
struct AtType {
    using type = T;
};
#if defined(_WIN64)
template <typename U>
struct AtType<U*> {
    using type = Ptr32<U>;
};
template <typename U>
struct AtType<U* const> {   // (a table of const pointers: GAME_ARRAY(const char* const, ...))
    using type = const Ptr32<U>;
};
#endif
// The type a field of type T has in the game's layout: T, or a Ptr32 when T is a pointer
// (for the templated layouts whose parameter may be a pointer: NiTArray<NiAVObject*>'s
// elements are the game's 4-byte pointers)
template <typename T>
using Field = typename AtType<T>::type;
template <typename T>
inline typename AtType<T>::type& At(const void* base, intptr_t offset) {
    return *reinterpret_cast<typename AtType<T>::type*>(reinterpret_cast<uintptr_t>(base) + uintptr_t(offset));
}
} // namespace re

// Game data at its original address. In the standalone (64-bit) build these become
// our own storage; until then they are the game's. A pointer variable or array is of the
// game's 4-byte pointers (re::Field: a Ptr32 on x64), as the fields are.
#define GAME_VAR(type, name, address) inline re::Field<type>& name = *reinterpret_cast<re::Field<type>*>(address)
#define GAME_ARRAY(type, name, address) inline re::Field<type>* const name = reinterpret_cast<re::Field<type>*>(address)
// A game variable holding what only a full pointer holds in the 64-bit build (a system DLL's
// handle or one of its functions: DLLs load above 4 GB there): ours, native-width storage in
// that build (one per address, whichever file declares it), the game's slot in the 32-bit one.
// Only our code reads these (no game code runs in the 64-bit build).
namespace re {
template <uintptr_t Address, typename T>
struct NativeSlot {
    static inline T value{};
};
} // namespace re
#if defined(_WIN64)
#define GAME_VAR_NATIVE(type, name, address) inline type& name = re::NativeSlot<address, type>::value
#else
#define GAME_VAR_NATIVE(type, name, address) GAME_VAR(type, name, address)
#endif
// A game function not reversed yet, called where it is.
#define GAME_FN(ret, conv, name, address, ...) inline ret(conv* const name)(__VA_ARGS__) = reinterpret_cast<ret(conv*)(__VA_ARGS__)>(address)
// A library function built from its source (third_party), under the declaration the code
// calling it was written against (the game's copy of the library is replaced: no address).
#define LIB_FN(ret, conv, name, fn, ...) inline ret(conv* const name)(__VA_ARGS__) = reinterpret_cast<ret(conv*)(__VA_ARGS__)>(&fn)


namespace re {
// The game's vector helpers 0x4146B0 (3x3 times a vector), 0x414C80, 0x41A9A0 (a matrix
// times a point) and 0x45D500 store 16 bytes through their first argument: the three floats,
// then a word copied from their own uninitialized stack (their vector type is padded). Our
// vectors are 12 bytes, so the game's own code writing that 4th word into one smashes what
// follows (a stack cookie, the next field). Their declarations call them through this: the
// game writes into a 16-byte buffer, the 12 bytes of the result are copied out. (Ours, at
// those addresses, write 12 bytes.) Returns `out`, as the game's do.
template <uintptr_t Address, typename R, typename Out, typename... A>
R CallVec16(Out* out, A... args) {
    alignas(16) float wide[4];
    reinterpret_cast<void(__cdecl*)(float*, re::Arg<A>...)>(Address)(wide, args...);
    std::memcpy(out, wide, 12);
    if constexpr (!std::is_void_v<R>) {
        return reinterpret_cast<R>(out);
    }
}
} // namespace re

namespace re {
// A system COM object's vtable: D3D9's, D3DX's, DirectDraw's, DirectInput's, DirectShow's, the
// XACT shim's (src/game/XactShim.cpp). Its slots are the process's own pointers (8 bytes on
// x64), unlike a game object's vtable in the game's .rdata (4-byte slots: VtableOf, Ptr32).
// The slot index is the same either way: ComVtable(device)[0x2C / 4].
inline void* const* ComVtable(const void* object) {
    return *static_cast<void* const* const*>(object);
}

// A system COM call by vtable slot (stdcall, the interface first). On x64 a game slot passed
// for a pointer (re::Ptr32<T>*: an out-pointer, the interface the call makes) goes to the call
// as a native pointer and the slot is written after; a const one (one pointer in) as a native
// copy. The 32-bit build calls straight through.
#if defined(_M_X64)
template <typename A>
struct ComArg {
    Arg<A> value;
    explicit ComArg(A a) : value(a) {}
    Arg<A> get() const { return value; }
    using type = Arg<A>;
};
template <typename U>
struct ComArg<Ptr32<U>*> {
    Ptr32<U>* slot;
    U* native;
    explicit ComArg(Ptr32<U>* s) : slot(s), native(s ? static_cast<U*>(*s) : nullptr) {}
    ComArg(const ComArg&) = delete;
    ~ComArg() {
        if (slot) {
            *slot = native;
        }
    }
    U** get() { return slot ? &native : nullptr; }
    using type = U**;
};
template <typename U>
struct ComArg<const Ptr32<U>*> {
    U* native;
    bool null;
    explicit ComArg(const Ptr32<U>* s) : native(s ? static_cast<U*>(*s) : nullptr), null(s == nullptr) {}
    U* const* get() const { return null ? nullptr : &native; }
    using type = U* const*;
};
template <typename R, typename Fn, typename... H>
inline R ComInvoke(Fn fn, const void* object, H&&... held) {
    return fn(object, held.get()...);
}
template <typename R = long, typename... A>
inline R ComCall(const void* object, int slot, A... args) {
    using Fn = R(__stdcall*)(const void*, typename ComArg<A>::type...);
    Fn fn = reinterpret_cast<Fn>(ComVtable(object)[slot]);
    return ComInvoke<R>(fn, object, ComArg<A>(args)...);
}
#else
template <typename R = long, typename... A>
inline R ComCall(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__stdcall*)(const void*, Arg<A>...)>(ComVtable(object)[slot])(object, args...);
}
#endif

// A game slot (re::Ptr32<T>*) handed to a system function that writes a pointer through it
// (D3DX's creators' out-pointers): on x64 the call gets a native pointer, written to the slot
// once the call is done (the temporary lives to the end of the full expression), where the
// system's 8 bytes would also clear the 4 after the slot. Use: D3DXCreateTexture(...,
// re::OutPtr(&slot)) with the parameter void**. The slot itself on Win32.
#if defined(_M_X64)
template <typename U>
class OutPtr {
public:
    explicit OutPtr(Ptr32<U>* slot) : slot_(slot), native_(slot ? static_cast<U*>(*slot) : nullptr) {}
    OutPtr(const OutPtr&) = delete;
    ~OutPtr() {
        if (slot_) {
            *slot_ = native_;
        }
    }
    operator void**() { return slot_ ? reinterpret_cast<void**>(&native_) : nullptr; }

private:
    Ptr32<U>* slot_;
    U* native_;
};
#else
template <typename U>
inline void** OutPtr(U** slot) {
    return reinterpret_cast<void**>(slot);
}
#endif

// A game object's field holding what only a full pointer holds in the 64-bit build (memory the
// system mapped: a locked vertex buffer's bits, which nothing makes come back below 4 GB): on
// x64 kept beside the object (one native slot per field address, reversed.cpp), the field
// itself on Win32. Only our code reads these.
#if defined(_M_X64)
void*& NativeFieldSlot(const void* field);
template <typename T>
inline T*& NativeField(const void* base, intptr_t offset) {
    return reinterpret_cast<T*&>(NativeFieldSlot(static_cast<const uint8_t*>(base) + offset));
}
#else
template <typename T>
inline T*& NativeField(const void* base, intptr_t offset) {
    return *reinterpret_cast<T**>(reinterpret_cast<uintptr_t>(base) + uintptr_t(offset));
}
#endif

// A function pointer the game keeps in a 4-byte slot (a vtable's, an import table's, a
// struct's callback), as a callable Fn: GameFnAt<Fn>(slot). On x64 a native function-pointer
// type would read 8 bytes there.
template <typename Fn>
inline Fn GameFnAt(const void* slot) {
    return reinterpret_cast<Fn>(static_cast<void*>(*static_cast<const Ptr32<void>*>(slot)));
}
template <typename Fn>
inline Fn GameFnAt(uintptr_t slot) {
    return GameFnAt<Fn>(reinterpret_cast<const void*>(slot));
}

// A game pointer slot (4 bytes) read or written as volatile (where the game re-reads it, or the
// compiler mustn't drop the store): LoadVolatile<T*>(slot), StoreVolatile(slot, p)
template <typename P>
inline P LoadVolatile(const void* slot) {
    return reinterpret_cast<P>(uintptr_t(*static_cast<const volatile uint32_t*>(slot)));
}
inline void StoreVolatile(void* slot, const void* p) {
    *static_cast<volatile uint32_t*>(slot) = Addr32(p);
}

// A 3-float vector by value where a body takes its own vector type (CVector, NiPoint3...): the
// same 12 bytes (tools/x64_vector_args.py's wrappers)
struct Float3 {
    float x, y, z;
};

// N floats by value (a 4- or 8-float struct's stand-in: x64_vector_args.py), and the i-th float of a
// struct of floats
template <int N>
struct Floats {
    float v[N];
};
template <typename T>
inline float FloatAt(const T& s, int i) {
    float f;
    std::memcpy(&f, reinterpret_cast<const char*>(&s) + 4 * i, 4);
    return f;
}

// A float's 32 bits as an integer and back (a declaration passing a float where the body takes
// the same 4 stack bytes as an int, or the other way: tools/x64_vector_args.py's wrappers)
inline uint32_t BitsOf(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
inline float FloatOf(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

// A game object's virtual function (its vtable's 4-byte slot `index`), as a callable Fn
template <typename Fn>
inline Fn GameVirtual(const void* object, int index) {
    const Ptr32<void>* vtable = *static_cast<const Ptr32<Ptr32<void>>*>(object);
    return reinterpret_cast<Fn>(static_cast<void*>(vtable[index]));
}

// A member function's address (for registering it)
template <typename T>
void* MemberAddress(T member) {
    return *reinterpret_cast<void**>(&member);
}

// The Body of a function given at compile time (RE_BODY at a registration): the function
// itself; on x64, for a body with re::Edx in the second place, the entry is an adapter taking
// the game's (self, args...) and calling the body with the placeholder; a member function is
// its own entry (x64's member call passes this first, as the game's callers do).
template <auto F>
struct BodyOf {
    static Body Get() { return Body(reinterpret_cast<void*>(F)); }
};
template <typename R, typename C, typename... A, R (C::*F)(A...)>
struct BodyOf<F> {
    static Body Get() { return Body(MemberAddress(F)); }
};
template <typename R, typename C, typename... A, R (C::*F)(A...) const>
struct BodyOf<F> {
    static Body Get() { return Body(MemberAddress(F)); }
};
#if defined(_WIN64)
// An argument as it arrives at an adapter from the game's callers (their x86 code had 4-byte
// stack slots): from the fifth on (a stack slot on x64) a pointer or a 64-bit integer the caller
// declared as a 32-bit int has an undefined upper half, so it's taken from its low 32 bits as
// Ptr32 does (sign-extended: the game's (T*)-1; every pointer here is below 2 GB). Registers
// (the first four) come from 32-bit moves, their upper halves clear.
template <size_t I, typename T>
inline T StackArg(T v) {
    if constexpr (I >= 4 && std::is_pointer_v<T>) {
        return reinterpret_cast<T>(intptr_t(int32_t(uint32_t(reinterpret_cast<uintptr_t>(v)))));
    } else if constexpr (I >= 4 && std::is_integral_v<T> && sizeof(T) == 8) {
        return std::is_signed_v<T> ? T(int32_t(uint32_t(v))) : T(uint32_t(v));
    } else {
        return v;
    }
}
template <typename... A>
constexpr bool kNeedsStackArgs = sizeof...(A) > 4;

// (a body returning a Ptr32 by value would be called through a hidden return pointer on x64,
// while the game's convention, and every other body, returns a pointer in rax: return T*)
template <typename R, typename... A, R (*F)(A...)>
struct BodyOf<F> {
    static_assert(!IsPtr32<R>::value, "a registered body returns a re::Ptr32 by value: return the pointer (T*)");
    template <size_t... I>
    static R Call(std::index_sequence<I...>, A... args) { return F(StackArg<I>(args)...); }
    static R Adapter(A... args) { return Call(std::index_sequence_for<A...>{}, args...); }
    static Body Get() {
        if constexpr (kNeedsStackArgs<A...>) {
            return Body(reinterpret_cast<void*>(F), reinterpret_cast<void*>(&Adapter));
        } else {
            return Body(reinterpret_cast<void*>(F));
        }
    }
};
template <typename R, typename S, typename... A, R (*F)(S, Edx, A...)>
struct BodyOf<F> {
    static_assert(!IsPtr32<R>::value, "a registered body returns a re::Ptr32 by value: return the pointer (T*)");
    // (the adapter's own arguments: self first, then A at positions 1...)
    template <size_t... I>
    static R Call(S self, std::index_sequence<I...>, A... args) { return F(self, Edx{}, StackArg<I + 1>(args)...); }
    static R Adapter(S self, A... args) { return Call(self, std::index_sequence_for<A...>{}, args...); }
    static Body Get() { return Body(reinterpret_cast<void*>(F), reinterpret_cast<void*>(&Adapter)); }
};
#endif
} // namespace re

// The body at a registration: REVERSED(name, address, RE_BODY(&Function), ...)
#define RE_BODY(...) (::re::BodyOf<__VA_ARGS__>::Get())

// A function written here but not hooked (too short for a hook, arguments in registers
// other than ecx, or patched in place by bullymenu/bullygfx): counted as done by
// tools/remaining.py. `name` is a string literal.
#define RE_UNHOOKED(name, address) static_assert((address) != 0, name)
// The same, with the body: registered but never installed, so re::Ours finds it by the
// address (the 64-bit build's calls)
#define RE_UNHOOKED_FN(name, address, fn) \
    static const bool RE_CONCAT(re_unhooked_, __LINE__) = \
        re::Register(re::Function{name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original, true}, __FILE__)

#define RE_CONCAT2(a, b) a##b
#define RE_CONCAT(a, b) RE_CONCAT2(a, b)
#define REVERSED(...) static const bool RE_CONCAT(re_registered_, __LINE__) = re::Register(re::Function{__VA_ARGS__}, __FILE__)
