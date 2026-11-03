#pragma once

// NiGPUProgramCache as Bully's exe has it (vtable 0x9A19E0, 0x4C; the DX9 renderer's cache
// derives from it, vtable 0x99D220, and D3D10's, 0x9A1078): compiled shader programs kept by
// name, with the shader constants each needs, saved to and loaded from a cache file ("NiGPUProgram
// Cache File", version 6) in a working directory; the names that failed to compile are kept
// too. Layout from the constructor 0x8CBCD0.
//
// Its slots: 3 GetProgram(name, constants&, failed&), 5 the semantics' set, 6 the profile name,
// 8 SetWorkingDirectory, 11 GetSemanticName, 12 GetSemanticIndex, 13 the semantic count (13),
// 14 Load, 15 Save, 16 Clear, 17 IsLoaded, 19 SetFailed, 20 RemoveUnusedConstants, 21
// UpdatePath, 22 AppendEntry, 23 SaveEntry, 24 LoadEntry; 9/10 the platform names and 25/26
// saving and loading a program are the renderer's.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

// NiTFixedStringMap<T>'s node (0xC): the next in the bucket, the key, the value
template <typename T>
struct FixedStringMapNode {
    re::Ptr32<FixedStringMapNode> next;   // +0x0
    re::Ptr32<char> key;                  // +0x4 (an NiFixedString handle: also the hash, by its address)
    re::Field<T> value;                   // +0x8
};

// NiTFixedStringMap<T> (0x10): its vtable (0 the deleting destructor, 1 NewItem, 2
// DeleteItem), the bucket count and buckets, how many it holds
template <typename T>
struct FixedStringMap {
    re::Ptr32<void> vtable;                       // +0x0
    uint32_t hashSize;                  // +0x4
    re::Ptr32<re::Ptr32<FixedStringMapNode<T>>> buckets;    // +0x8 (NiMalloc'd)
    uint32_t count;                     // +0xC
};
static_assert(sizeof(FixedStringMap<void*>) == 0x10);

// NiTPrimitiveSet / NiTObjectSet (0xC): the elements, how many there's room for, how many there are
struct PrimitiveSet {
    re::Ptr32<re::Ptr32<void>> data;        // +0x0
    uint32_t alloced;   // +0x4
    uint32_t count;     // +0x8
};
static_assert(sizeof(PrimitiveSet) == 0xC);

// a cached program's description (0x18, constructed by 0x898D80): the program and the shader
// constants it uses (NiTObjectSet of 0x2C-byte constant entries: five names, four values)
struct GPUProgramDescriptor : NiRefObject {
    re::Ptr32<NiRefObject> program;    // +0x8 (a reference)
    PrimitiveSet constants;  // +0xC
};
static_assert(sizeof(GPUProgramDescriptor) == 0x18);

struct NiGPUProgramCache : NiRefObject {
    FixedStringMap<GPUProgramDescriptor*> programs;   // +0x8 (vtable 0x9A19AC; references held)
    FixedStringMap<uint8_t> failed;                    // +0x18 (vtable 0x9A19B8)
    re::Ptr32<char> fileName;          // +0x28 (NiFixedStrings, as are the next three)
    re::Ptr32<char> directory;         // +0x2C (absolute)
    re::Ptr32<char> path;              // +0x30 (the directory and the file name)
    re::Ptr32<char> profile;           // +0x34 (the third name in the file's header)
    PrimitiveSet semantics;  // +0x38 (NiFixedStrings: 10 allocated)
    uint32_t version;        // +0x44 (entries of another version are out of date)
    uint8_t autoAppend;      // +0x48 (each failure appended to the file)
    uint8_t loaded;          // +0x49
    uint8_t field4A;         // +0x4A
    uint8_t pad4B;
};
static_assert(offsetof(NiGPUProgramCache, failed) == 0x18 && offsetof(NiGPUProgramCache, semantics) == 0x38);
static_assert(sizeof(NiGPUProgramCache) == 0x4C);

} // namespace ni
