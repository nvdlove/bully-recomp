#pragma once

// CFileLoader: the level files (Config/Dat/*.dat), the IDE images they list (the
// model definitions, in a binary form of GTA's IDE sections), texture dictionaries
// and the collision files. CFileMgr: the game's file access, on Win32 handles
// (through its imports, which the mod loader hooks) and on memory. (GTA San
// Andreas' names where the code matches.)

#include <cstddef>
#include <cstdint>

#include "reversed.h"

struct CBaseModelInfo;
struct CColModel;
struct TexDictionary;

// A file: the Win32 handle (nullptr: none; the game's files are never
// INVALID_HANDLE_VALUE, OpenFile turns that into nullptr).
using FileHandle = void*;

// What OpenStream returns (0x14 bytes)
struct FileStream {
    re::Ptr32<void> file;    // +0 kind 2 (a file); nullptr for memory
    re::Ptr32<uint8_t> data;      // +4 kind 3 (memory)
    uint32_t size;      // +8
    uint32_t position;  // +0xC
    int32_t mode;       // +0x10 1: read
};
static_assert(sizeof(FileStream) == 0x14);

// The memory a stream reads (OpenStream's argument for kind 3, CloseStream's out)
struct FileStreamBuffer {
    re::Ptr32<uint8_t> data;
    uint32_t size;
};

// A file read into memory, read from in order (0xC)
struct MemoryFile {
    re::Ptr32<uint8_t> data;      // +0 (the owner frees it)
    uint32_t size;      // +4
    uint32_t position;  // +8

    // 0x426610 (thiscall): the whole file ("rb"); false when it can't be read (the
    // memory's kept when the read comes up short)
    uint8_t Load(const char* path);
};
static_assert(sizeof(MemoryFile) == 0xC);

// An archive's directory: an entry's place by its name's hash (GTA's CDirectory;
// CDirectoryTemplate<CDirectoryInfo> in the game's RTTI)
struct CDirectoryInfo {
    uint32_t posn;      // +0 sectors
    uint32_t size;      // +4 sectors
    uint32_t nameHash;  // +8

    // 0x428FD0 (thiscall): from a .DIR entry (the name hashed)
    void Set(const struct DirEntry* entry);
};
static_assert(sizeof(CDirectoryInfo) == 0xC);

// A .DIR file's entry (and a .IMG's IDE directory's)
struct DirEntry {
    uint32_t posn;  // +0 sectors
    uint32_t size;  // +4 sectors
    char name[24];  // +8
};
static_assert(sizeof(DirEntry) == 0x20);

struct CDirectory {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0 (0x9017A0: its only entry, the deleting destructor)
    re::Ptr32<CDirectoryInfo> entries;  // +4
    int32_t capacity;         // +8
    int32_t count;            // +0xC

    // 0x4291B0 (thiscall)
    CDirectory* Construct(int32_t maxEntries);
    // 0x429020 (thiscall)
    void Destruct();
    // 0x429180 (thiscall, vf0): flags 1: and freed
    CDirectory* Destroy(uint8_t flags);
    // 0x4291F0 (thiscall): all of a .DIR file's entries
    void ReadDirFile(const char* path);
    // 0x429040 (thiscall): DebugBreak when full
    void AddItem(const DirEntry* entry);
    // 0x429070 (thiscall): -1 when it's not there
    int32_t FindItemIndex(uint32_t nameHash);
    // 0x4290A0 (thiscall)
    uint8_t FindItemByHash(uint32_t nameHash, uint32_t* posn, uint32_t* size);
    // 0x4290E0 (thiscall)
    uint8_t FindItem(const char* name, uint32_t* posn, uint32_t* size);
    // 0x429130 (thiscall)
    void GetItem(int32_t index, uint32_t* posn, uint32_t* size);
    // 0x429160 (thiscall)
    void SetItem(int32_t index, uint32_t posn, uint32_t size);
};
static_assert(sizeof(CDirectory) == 0x10);

namespace CFileMgr {

GAME_ARRAY(char, ms_rootDirName, 0x00BD0C78);

// 0x42D040: fopen, on CreateFileA: "r" read (shared), "w" create, "a" open or create
// (no path prefix; nullptr on failure)
FileHandle __cdecl OpenFileRaw(const char* path, const char* mode);
// 0x42D0E0: fread (returns the bytes read, not the items)
uint32_t __cdecl ReadItems(void* buffer, uint32_t size, uint32_t count, FileHandle file);
// 0x42D110: fwrite (the items written)
uint32_t __cdecl WriteItems(const void* buffer, uint32_t size, uint32_t count, FileHandle file);
// 0x42D140: fclose (returns 0)
int __cdecl CloseFileRaw(FileHandle file);
// 0x42D150: the root directory ("D:\" and a backslash)
void __cdecl Initialise();
// 0x42D180: a whole file into `buffer` (0-terminated), its size or -1
int __cdecl LoadFile(const char* path, uint8_t* buffer, int maxSize, const char* mode);
// 0x42D210: -1 when it can't be opened
int __cdecl GetFileLength(const char* path);
// 0x42D260: the path under the game's root (GetFullPath)
FileHandle __cdecl OpenFile(const char* path, const char* mode);
// 0x42D280: ("wb")
FileHandle __cdecl OpenFileForWriting(const char* path);
// 0x42D2A0: the bytes read
uint32_t __cdecl Read(FileHandle file, void* buffer, uint32_t size);
// 0x42D2D0: the bytes written
uint32_t __cdecl Write(FileHandle file, const void* buffer, uint32_t size);
// 0x42D300: SetFilePointer's origins
uint8_t __cdecl Seek(FileHandle file, int32_t offset, uint32_t origin);
// 0x42D330: a line (up to size - 1 characters, CR/LF and CRLF ending it);
// false at the end of the file with nothing read
uint8_t __cdecl ReadLine(FileHandle file, char* buffer, int size);
// 0x42D430
uint32_t __cdecl Tell(FileHandle file);
// 0x42D450: CloseHandle's result
int __cdecl CloseFile(FileHandle file);
// 0x42D460: printf into a file (1023 characters at most)
int __cdecl Printf(FileHandle file, const char* format, ...);

// 0x42D4C0: kind 2: a file (`arg` its path; mode 1 read, else write); kind 3:
// memory (`arg` a FileStreamBuffer; not for mode 3)
FileStream* __cdecl OpenStream(int kind, int mode, void* arg);
// 0x42D570: `out` gets the memory back (read before the null check: the game's)
int __cdecl CloseStream(FileStream* stream, FileStreamBuffer* out);
// 0x42D5B0: the bytes read
uint32_t __cdecl ReadFromMemory(MemoryFile* file, void* buffer, uint32_t size);
// 0x42D600: ReadLine on memory
uint8_t __cdecl ReadLineFromMemory(MemoryFile* file, char* buffer, int size);

} // namespace CFileMgr

namespace CGame {
// 0x42E450: the file manager and the archive reader (GTA's)
uint8_t __cdecl InitialiseOnceBeforeRW();
// 0x42E470: Config\Dat\PropHXDs.dat's first line: how many (+ 18) the streaming
// makes room for
void __cdecl LoadPropHXDsCount();
} // namespace CGame

namespace CFileLoader {

// LoadLine's line (350 bytes)
GAME_ARRAY(char, ms_line, 0x00BD0A08);
GAME_ARRAY(char, ms_loadingText, 0x00C221A8);

// The model id ranges the IDE sections filled (-1: none yet)
GAME_VAR(int32_t, ms_firstPAnimModel, 0x00A136A0);
GAME_VAR(int32_t, ms_lastPAnimModel, 0x00A136A4);
GAME_VAR(int32_t, ms_firstVehicleModel, 0x00A136A8);
GAME_VAR(int32_t, ms_lastVehicleModel, 0x00A136AC);
GAME_VAR(int32_t, ms_firstWeaponModel, 0x00A136B0);
GAME_VAR(int32_t, ms_lastWeaponModel, 0x00A136B4);
GAME_VAR(int32_t, ms_firstItemModel, 0x00A136B8);
GAME_VAR(int32_t, ms_lastItemModel, 0x00A136BC);
GAME_VAR(int32_t, ms_firstAccessoryModel, 0x00A136C0);
GAME_VAR(int32_t, ms_lastAccessoryModel, 0x00A136C4);
GAME_VAR(int32_t, ms_firstPedModel, 0x00A136C8);
GAME_VAR(int32_t, ms_lastNormalPedModel, 0x00A136CC);   // the last before "spfirst"
GAME_VAR(int32_t, ms_firstSpecialPedModel, 0x00A136D0); // "spfirst"
GAME_VAR(int32_t, ms_lastSpecialPedModel, 0x00A136D4);  // "splast"
GAME_VAR(int32_t, ms_firstClothModel, 0x00A136D8);
GAME_VAR(int32_t, ms_lastClothModel, 0x00A136DC);       // the highest
GAME_VAR(int32_t, ms_firstBikeModel, 0x00A136E0);
GAME_VAR(int32_t, ms_lastBikeModel, 0x00A136E4);

// 0x429C10: a line of a text file, cleaned: control characters and commas are
// spaces, leading blanks skipped (in ms_line; nullptr at the end)
char* __cdecl LoadLine(FileHandle file);
// 0x429CA0: the same from memory; `consumed` gets the bytes the line took
char* __cdecl LoadLineFromBuffer(const char* data, int size, int* consumed);
// 0x42B980: the same from a memory file (which moves past the line)
char* __cdecl LoadLineFromMemory(MemoryFile* file);
// 0x429D70: "Loading <file>" on the loading screen, when it's up
void __cdecl LoadingScreenLoadingFile(const char* path);
// 0x429DC0: a texture dictionary from a file (empty when it can't be read)
TexDictionary* __cdecl LoadTexDictionary(const char* path);
// 0x42CDE0: `src`'s textures into `dst` (both referencing them)
void __cdecl AddTexDictionaries(TexDictionary* dst, TexDictionary* src);
// 0x429E30: Config\Extra\shared.bin
uint8_t __cdecl LoadModelInfoFile(const char* path, int arg);
// 0x42A8E0: a model file into a new object (nullptr when it can't be read)
// (GTA's name; what it makes isn't known yet)
uint8_t* __cdecl LoadAtomicFile2Return(const char* path);
// 0x42B9D0: a streamed model's file into its model info (the kind of model from the
// directory it came from)
uint8_t __cdecl LoadClumpFile(FileStreamBuffer* buffer, int id);
// 0x429E80: an atomic model's IDE flags into its model info flags (+0x28)
void __cdecl SetAtomicModelInfoFlags(CBaseModelInfo* mi, uint32_t ideFlags);
// 0x429FC0: model ids in some hardcoded ranges (their objects get flag 0x100)
uint8_t __cdecl IsModelInSpecialRanges(int id);

// The IDE image sections: each gets the cursor (moved past the section).
// 0x42AB70 "objs"
void __cdecl LoadObjectTypes(re::Ptr32<uint8_t>* cursor);
// 0x42B0D0 "tobj"
void __cdecl LoadTimeObjectTypes(re::Ptr32<uint8_t>* cursor);
// 0x42A030 "weap"
void __cdecl LoadWeaponTypes(re::Ptr32<uint8_t>* cursor);
// 0x42A1D0 "accs"
void __cdecl LoadAccessoryTypes(re::Ptr32<uint8_t>* cursor);
// 0x42A2B0 "cars"
void __cdecl LoadVehicleTypes(re::Ptr32<uint8_t>* cursor);
// 0x42A620 "panm"
void __cdecl LoadPAnimTypes(re::Ptr32<uint8_t>* cursor);
// 0x42A7F0 "clth"
void __cdecl LoadClothTypes(re::Ptr32<uint8_t>* cursor);
// 0x42B550 "item"
void __cdecl LoadItemTypes(re::Ptr32<uint8_t>* cursor);
// 0x42B660 "cash" and "scnd"
void __cdecl LoadSimpleTypes(re::Ptr32<uint8_t>* cursor);
// 0x42B760 "2dfx": lights and particles (textures from the "particle" dictionary)
void __cdecl Load2dEffects(re::Ptr32<uint8_t>* cursor);
// 0x42C120 "peds"
void __cdecl LoadPedTypes(re::Ptr32<uint8_t>* cursor);

// 0x42CAC0: an IDE image entry: sections, each followed by the model id range it
// used (added to the streaming's)
void __cdecl LoadIDEimageData(uint8_t* data, int size);
// 0x42CC40: <name>.DIR and <name>.IMG, all their entries
void __cdecl LoadIDEimage(const char* name);
// 0x42CE80: a level file's lines (IMAGEPATH, TEXDICTION, IMGIDE, SKY_DOME; COLFILE,
// MODELFILE and HIERFILE are ignored; EXIT ends it)
void __cdecl LoadLevel(const char* path);

// 0x42AAC0: a collision model's bounds; returns what follows them
uint8_t* __cdecl LoadCollisionBounds(uint8_t* data, CColModel* col);
// 0x42BCB0: a collision model's volumes (format: COL2's version / COL3's first
// word, format2 COL3's second)
void __cdecl LoadCollisionModel(uint8_t* data, CColModel* col, int size, int format, int format2);
// 0x42C6C0: a .col file into the models' collision (made where missing)
uint8_t __cdecl LoadCollisionFile(uint8_t* data, uint32_t size, int colSlot);
// 0x42C8E0: a .col file the first time: new collision for each model
uint8_t __cdecl LoadCollisionFileFirstTime(uint8_t* data, uint32_t size, int colSlot);

} // namespace CFileLoader
