#pragma once

// NiStream (vtable 0x9517D8, 0x40C bytes, built on the stack by the game's loaders): the
// file's header values, the objects read or to write, their sizes, the top-level ones, the
// fixed strings, the binary streams in and out, the link IDs, the save registration map, the
// texture palette, the error. Layout from the constructor 0x75CE40.
//
// Slots: 0 the deleting destructor, 1 Load(NiBinaryStream*), 2 Load(buffer, size), 3
// Load(path), 4 Save(NiBinaryStream*), 5 Save(buffer&, size&), 6 Save(path), 8 / 9 the save
// registration, 10 ChangeObject, 11 GetLinkIDFromObject, 12 SaveLinkID, 13 LoadHeader, 14
// SaveHeader, 15 LoadStream, 16 SaveStream, 17 RegisterSaveObjects, 18 LoadTopLevelObjects, 19
// SaveTopLevelObjects, 20 LoadObject, 21 PreSaveObjectSizeTable, 22 SaveObjectSizeTable, 23
// LoadObjectSizeTable.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiFile.h"
#include "reversed.h"

namespace ni {

// NiTLargeArray (the large arrays: 32-bit counts)
template <typename T>
struct NiTLargeArray {
    re::Ptr32<void> vtable;        // +0x0
    re::Ptr32<re::Field<T>> base;  // +0x4
    uint32_t maxSize;    // +0x8
    uint32_t size;       // +0xC (the highest used slot + 1)
    uint32_t esize;      // +0x10 (slots in use)
    uint32_t growBy;     // +0x14
};
static_assert(sizeof(NiTLargeArray<void*>) == 0x18);

// a growing array of unsigned ints (the link IDs)
struct NiLinkIDSet {
    re::Ptr32<uint32_t> base;      // +0x0
    uint32_t allocated;  // +0x4
    uint32_t count;      // +0x8
};

struct NiStream {
    re::Ptr32<void> vtable;                                // +0x0
    NiTArray<void*> groups;                      // +0x4 (NiTPrimitiveArray<NiObjectGroup*>, vtable 0x961988)
    uint32_t version;                            // +0x14
    uint32_t userVersion;                        // +0x18
    char fileName[0x104];                        // +0x1C
    uint8_t saveLittleEndian;                    // +0x120
    uint8_t sourceIsLittleEndian;                // +0x121
    uint8_t pad122[2];
    re::Ptr32<void> searchPath;                            // +0x124 (NiSearchPath, 0x210 bytes)
    NiTLargeArray<NiRefObject*> objects;         // +0x128 (vtable 0x951914)
    NiTLargeArray<uint32_t> objectSizes;         // +0x140 (vtable 0x9516E8)
    NiTLargeArray<NiRefObject*> topObjects;      // +0x158 (vtable 0x951914)
    NiTLargeArray<char*> fixedStrings;           // +0x170 (vtable 0x951918)
    re::Ptr32<NiBinaryStream> istr;                        // +0x188
    re::Ptr32<NiBinaryStream> ostr;                        // +0x18C
    NiLinkIDSet linkIDs;                         // +0x190
    uint32_t linkIndex;                          // +0x19C
    NiLinkIDSet linkIDBlocks;                    // +0x1A0
    uint32_t linkBlockIndex;                     // +0x1AC
    re::Ptr32<void> registerMapVtable;                     // +0x1B0 (NiTPointerMap<NiObject*, unsigned int>)
    uint32_t registerHashSize;                   // +0x1B4
    re::Ptr32<re::Ptr32<void>> registerTable;                        // +0x1B8
    uint32_t registerCount;                      // +0x1BC
    re::Ptr32<NiRefObject> texturePalette;                 // +0x1C0
    uint32_t unknown1C4[2];
    uint32_t loadState;                          // +0x1CC (2: cancelled, 3: pause asked, 4: paused)
    uint32_t unknown1D0;
    uint32_t loadIndex;                          // +0x1D4 (the object being loaded)
    uint32_t linkObjectIndex;                    // +0x1D8
    uint32_t postLinkIndex;                      // +0x1DC
    uint32_t unknown1E0;                         // +0x1E0
    uint32_t unknown1E4;                         // +0x1E4 (1)
    re::Ptr32<void> semaphore;                             // +0x1E8
    uint32_t unknown1EC;                         // +0x1EC
    re::Ptr32<void> thread;                                // +0x1F0 (deleted with the stream)
    uint32_t unknown1F4;                         // +0x1F4 (2)
    int32_t unknown1F8;                          // +0x1F8 (-1)
    int32_t unknown1FC;                          // +0x1FC (-1)
    uint8_t unknown200[0x104];
    uint32_t errorCode;                          // +0x304
    char errorMessage[0x104];                    // +0x308
};
static_assert(offsetof(NiStream, version) == 0x14);
static_assert(offsetof(NiStream, searchPath) == 0x124);
static_assert(offsetof(NiStream, objects) == 0x128);
static_assert(offsetof(NiStream, objectSizes) == 0x140);
static_assert(offsetof(NiStream, topObjects) == 0x158);
static_assert(offsetof(NiStream, fixedStrings) == 0x170);
static_assert(offsetof(NiStream, istr) == 0x188);
static_assert(offsetof(NiStream, linkIDs) == 0x190);
static_assert(offsetof(NiStream, linkIDBlocks) == 0x1A0);
static_assert(offsetof(NiStream, registerMapVtable) == 0x1B0);
static_assert(offsetof(NiStream, texturePalette) == 0x1C0);
static_assert(offsetof(NiStream, loadState) == 0x1CC);
static_assert(offsetof(NiStream, loadIndex) == 0x1D4);
static_assert(offsetof(NiStream, semaphore) == 0x1E8);
static_assert(offsetof(NiStream, errorCode) == 0x304);
static_assert(sizeof(NiStream) == 0x40C);

} // namespace ni
