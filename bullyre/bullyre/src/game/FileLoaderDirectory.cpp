// CDirectory (an archive's directory) and MemoryFile (see FileLoader.h)

#include <cstring>

#include "game/FileLoader.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
inline int __cdecl StrNCpyS(char* dst, size_t dstSize, const char* src, size_t count) { return crt::StrncpyS(dst, dstSize, src, count); }   // the CRT's strncpy_s
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
using DebugBreakFn = void(__stdcall*)();
GAME_VAR(DebugBreakFn, Import_DebugBreak, 0x008FC05C);
inline re::Ptr32<void>* const directoryVtable = reinterpret_cast<re::Ptr32<void>*>(0x009017A0);
// (their own, called where modes apply)
GAME_FN(FileHandle, __cdecl, File_OpenFile, 0x0042D260, const char* path, const char* mode);
GAME_FN(uint32_t, __cdecl, File_Read, 0x0042D2A0, FileHandle file, void* buffer, uint32_t size);
GAME_FN(uint8_t, __cdecl, File_Seek, 0x0042D300, FileHandle file, int32_t offset, uint32_t origin);
GAME_FN(uint32_t, __cdecl, File_Tell, 0x0042D430, FileHandle file);
GAME_FN(int, __cdecl, File_CloseFile, 0x0042D450, FileHandle file);
GAME_FN(uint32_t, __cdecl, File_ReadFromMemory, 0x0042D5B0, MemoryFile* file, void* buffer, uint32_t size);
GAME_FN(uint8_t, __thiscall, MemoryFile_Load, 0x00426610, MemoryFile* file, const char* path);
GAME_FN(void, __thiscall, DirectoryInfo_Set, 0x00428FD0, CDirectoryInfo* info, const DirEntry* entry);
} // namespace

// ---- MemoryFile -------------------------------------------------------------------------

uint8_t MemoryFile::Load(const char* path) {
    FileHandle file = File_OpenFile(path, "rb");
    if (!file) {
        return 0;
    }
    if (File_Seek(file, 0, 2)) {
        size = File_Tell(file);
        if (size != 0 && File_Seek(file, 0, 0)) {
            data = static_cast<uint8_t*>(Malloc(size));
            if (data && File_Read(file, data, size) == size) {
                File_CloseFile(file);
                position = 0;
                return 1;
            }
        }
    }
    File_CloseFile(file);
    return 0;
}

// ---- CDirectory ---------------------------------------------------------------------------

// (inlined in ReadDirFile)
static void SetInfo(CDirectoryInfo* info, const DirEntry* entry) {
    info->posn = entry->posn;
    info->size = entry->size;
    char name[25];
    std::memset(name, 0, sizeof(name));
    StrNCpyS(name, sizeof(name), entry->name, 24);
    info->nameHash = HashName(name);
}

void CDirectoryInfo::Set(const DirEntry* entry) {
    SetInfo(this, entry);
}

CDirectory* CDirectory::Construct(int32_t maxEntries) {
    capacity = maxEntries;
    vtable = directoryVtable;
    count = 0;
    // (new[]'s overflow check: all ones)
    uint64_t bytes = uint64_t(uint32_t(maxEntries)) * sizeof(CDirectoryInfo);
    entries = static_cast<CDirectoryInfo*>(Malloc(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : size_t(bytes)));
    return this;
}

void CDirectory::Destruct() {
    vtable = directoryVtable;
    Free(entries);
}

CDirectory* CDirectory::Destroy(uint8_t flags) {
    vtable = directoryVtable;
    Free(entries);
    if (flags & 1) {
        Free(this);
    }
    return this;
}

void CDirectory::ReadDirFile(const char* path) {
    MemoryFile file = {nullptr, 0, 0};
    MemoryFile_Load(&file, path);
    DirEntry entry;
    while (File_ReadFromMemory(&file, &entry, sizeof(entry)) == sizeof(entry)) {
        if (count < capacity) {
            CDirectoryInfo* info = &entries[count];
            count++;
            SetInfo(info, &entry);
        } else {
            Import_DebugBreak();
        }
    }
    if (file.data) {
        Free(file.data);
    }
}

void CDirectory::AddItem(const DirEntry* entry) {
    if (count >= capacity) {
        Import_DebugBreak();
        return;
    }
    CDirectoryInfo* info = &entries[count];
    count++;
    DirectoryInfo_Set(info, entry);
}

int32_t CDirectory::FindItemIndex(uint32_t nameHash) {
    for (int32_t i = 0; i < count; i++) {
        if (entries[i].nameHash == nameHash) {
            return i;
        }
    }
    return -1;
}

uint8_t CDirectory::FindItemByHash(uint32_t nameHash, uint32_t* posn, uint32_t* size) {
    for (int32_t i = 0; i < count; i++) {
        if (entries[i].nameHash == nameHash) {
            *posn = entries[i].posn;
            *size = entries[i].size;
            return 1;
        }
    }
    return 0;
}

uint8_t CDirectory::FindItem(const char* name, uint32_t* posn, uint32_t* size) {
    uint32_t nameHash = HashName(name);
    for (int32_t i = 0; i < count; i++) {
        if (entries[i].nameHash == nameHash) {
            *posn = entries[i].posn;
            *size = entries[i].size;
            return 1;
        }
    }
    return 0;
}

void CDirectory::GetItem(int32_t index, uint32_t* posn, uint32_t* size) {
    *posn = entries[index].posn;
    *size = entries[index].size;
}

void CDirectory::SetItem(int32_t index, uint32_t posn, uint32_t size) {
    entries[index].posn = posn;
    entries[index].size = size;
}

// Untested: the game's code runs until each is checked.
#define DIR_REVERSED(name, member, address) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, re::MemberAddress(member), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

DIR_REVERSED("MemoryFile::Load", &MemoryFile::Load, 0x00426610);
DIR_REVERSED("CDirectoryInfo::Set", &CDirectoryInfo::Set, 0x00428FD0);
DIR_REVERSED("CDirectory::Construct", &CDirectory::Construct, 0x004291B0);
DIR_REVERSED("CDirectory::Destruct", &CDirectory::Destruct, 0x00429020);
DIR_REVERSED("CDirectory::Destroy", &CDirectory::Destroy, 0x00429180);
DIR_REVERSED("CDirectory::ReadDirFile", &CDirectory::ReadDirFile, 0x004291F0);
DIR_REVERSED("CDirectory::AddItem", &CDirectory::AddItem, 0x00429040);
DIR_REVERSED("CDirectory::FindItemIndex", &CDirectory::FindItemIndex, 0x00429070);
DIR_REVERSED("CDirectory::FindItemByHash", &CDirectory::FindItemByHash, 0x004290A0);
DIR_REVERSED("CDirectory::FindItem", &CDirectory::FindItem, 0x004290E0);
DIR_REVERSED("CDirectory::GetItem", &CDirectory::GetItem, 0x00429130);
DIR_REVERSED("CDirectory::SetItem", &CDirectory::SetItem, 0x00429160);
