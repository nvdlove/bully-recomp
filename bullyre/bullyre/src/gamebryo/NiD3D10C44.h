#pragma once

// What the files of Gamebryo chunk 44 (0x8BB370..0x8C7040: the last of the D3D10 layer, the
// cursors, the task manager pieces, Ni2DRenderView and the rest of the range) share: COM calls by
// slot, the D3DX imports, the NiPointer / NiFixedString handling the library inlines (through the
// interlocked imports). Everything in ni::c44 so nothing clashes with the other Gamebryo files
// (NiDX9Renderer.h's and NiSystem.h's names are used read-only).
//
// Bully renders with DX9: the D3D10 code is only reached through its vtables and NiApplication's
// CreateD3D10Renderer ("-d3d10"), which the game never asks for; the cursors are the sample
// framework's (Bully stubs its CreateCursor). Cold code, written exactly all the same.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c44 {

using ni::dx9::Com;
using ni::sys::At;
using ni::sys::Decrement;
using ni::sys::Import;
using ni::sys::Increment;
using ni::sys::Virtual;

// ---- the statics every NiRefObject counts in, its vtable, the empty NiFixedString handle
GAME_VAR(uint32_t, ms_uiObjects, 0x020E2168);
constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;

inline const char* Text(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}

// ---- NiPointer as the library inlines it (InterlockedIncrement / Decrement through the imports;
// DeleteThis, slot 1, when it was the last)
inline void ReleaseRef(void* object) {
    if (object != nullptr && Decrement(&At<uint32_t>(object, 4)) == 0) {
        Virtual<void>(object, 1);
    }
}
inline void AddRefRef(void* object) {
    if (object != nullptr) {
        Increment(&At<uint32_t>(object, 4));
    }
}
// NiPointer::operator= (only when it differs: the old let go, the new held)
inline void AssignRef(re::Ptr32<void>& slot, void* value) {
    if (slot != value) {
        ReleaseRef(slot);
        slot = value;
        AddRefRef(value);
    }
}

// ---- NiFixedString as the library inlines it (the count 8 bytes before the characters, the
// length 4 before them, the hash bucket 2 before them; the empty handle *0x951028)
inline char* NullString() {
    return ni::sys::kNullFixedString;
}
inline char* StringAddRef(char* s) {
    if (s != NullString()) {
        Increment(s - 8);
    }
    return s;
}
inline void StringRelease(re::Ptr32<char>* handle) {
    ni::sys::FixedStringRelease(handle);
}
inline uint32_t StringLength(const char* s) {
    return *reinterpret_cast<const uint16_t*>(s - 4);
}
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(char*, __cdecl, FixedString_Add, 0x00754760, const char* text);

// a handle slot set from text through a temporary, as the library's NiFixedString(const char*) and
// operator= inline it
inline void SetString(re::Ptr32<char>* slot, const char* text) {
    re::Ptr32<char> temp = text != nullptr ? FixedString_Add(text) : NullString();
    FixedString_Assign(slot, &temp);
    StringRelease(&temp);
}

// ---- D3DX (the game calls them through jmp thunks to these import slots: 0x85AC6C..)
constexpr uintptr_t kD3DXVec4Transform = 0x008FC2D4;
constexpr uintptr_t kD3DXMatrixMultiply = 0x008FC2E4;
constexpr uintptr_t kD3DXMatrixTranspose = 0x008FC2EC;
constexpr uintptr_t kD3DXMatrixInverse = 0x008FC304;

inline void MatrixMultiply(float* out, const float* a, const float* b) {
    Import<void*>(kD3DXMatrixMultiply, out, a, b);
}
inline void MatrixTranspose(float* out, const float* m) {
    Import<void*>(kD3DXMatrixTranspose, out, m);
}
inline void MatrixInverse(float* out, const float* m) {
    Import<void*>(kD3DXMatrixInverse, out, static_cast<float*>(nullptr), m);
}
inline void Vec4Transform(float* out, const float* v, const float* m) {
    Import<void*>(kD3DXVec4Transform, out, v, m);
}

// ---- the memory and the library's helpers
using ni::NiDelete;
using ni::NiFree;
using ni::NiMalloc;
using ni::NiNew;
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);

// the D3D10 renderer (NiRenderer::ms_pkRenderer when it's the one made)
GAME_FN(uint8_t*, __cdecl, D3D10Renderer_GetRenderer, 0x008A9300);

// ---- NiShaderConstantMapEntry (vtable 0x99C2E4, 0x34 bytes; the DX9 maps' too) as the D3D10
// constant maps use it: the shader register and register count packed (0x1RCXXXXX: R the first
// component, C a flag, X the register; the count the same with the components)
struct ConstantMapEntry {
    re::Ptr32<void> vtable;          // +0x0
    uint32_t refCount;     // +0x4
    re::Ptr32<char> key;             // +0x8 (NiFixedString)
    uint32_t mapping;      // +0xC (the internal id: predefined / object mapping)
    uint32_t flags;        // +0x10 (the kind in the top nibble: 1 constant, 2 predefined, 3
                           //  attribute, 4 global, 5 operator, 6 object; the low byte the
                           //  attribute type; the second byte the object type)
    uint32_t extra;        // +0x14 (an operator's operands and operation)
    uint32_t reg;          // +0x18 (packed; -1 unplaced)
    uint32_t count;        // +0x1C (packed)
    re::Ptr32<char> variable;        // +0x20 (NiFixedString: the shader variable's name)
    uint32_t size;         // +0x24 (the data's)
    uint32_t stride;       // +0x28
    re::Ptr32<void> data;            // +0x2C
    uint8_t ownsData;      // +0x30
    uint8_t placed;        // +0x31 (D3D10: its register set)
    uint8_t columnMajor;   // +0x32 (D3D10: the variable a column major matrix, from the shader's
                           //  reflection: the matrices transposed the other way)
    uint8_t pad33;
};
static_assert(offsetof(ConstantMapEntry, reg) == 0x18);
static_assert(offsetof(ConstantMapEntry, ownsData) == 0x30);
static_assert(sizeof(ConstantMapEntry) == 0x34);

// the attribute types' global types (by the flags' low byte), the object types (by the flags'
// second byte), the object mappings (three per mapping: count, type, ...)
GAME_ARRAY(const uint32_t, ms_auiGlobalTypes, 0x020E408C);
GAME_ARRAY(const uint32_t, ms_auiObjectTypes, 0x020E4040);
GAME_ARRAY(const uint8_t, ms_aucAttributeTypes, 0x020E407C);
GAME_ARRAY(const uint16_t, ms_ausObjectFlags, 0x020E4020);
GAME_ARRAY(const uint32_t, ms_auiObjectMappings, 0x020E330C);

} // namespace ni::c44
