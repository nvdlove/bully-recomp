// Gamebryo's system library pieces left between the finished files: NiFile's default hooks (the
// file factory, Access, CreateDirectory, DirectoryExists: the table at 0xBC74EC) and its plain
// read/write functions, the default message box, NiPath::ConvertToRelative, NiStaticDataManager's
// library registration, NiGlobalStringTable's constructor, destructor and _SDMInit/_SDMShutdown,
// the texture replacement format's default, and one of NiRenderer's static purges.

#include <cstring>

#include "gamebryo/NiFile.h"
#include "gamebryo/NiGlobalStringTable.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t kCreateDirectoryA = 0x008FC0DC;
constexpr uintptr_t kGetFileAttributesA = 0x008FC0D4;
constexpr uintptr_t kMessageBoxA = 0x008FC240;
constexpr uintptr_t kStringArray_vtable = 0x00961988;   // NiTPrimitiveArray<char*>

GAME_FN(NiFile*, __thiscall, NiFile_Construct, 0x00752590, void* self, const char* path, int mode,
        uint32_t bufferSize);
GAME_FN(void, __thiscall, NiFile_Destruct, 0x00752720, NiFile* self);
GAME_FN(uint32_t, __thiscall, NiFile_Read, 0x007527E0, NiFile* self, void* buffer, uint32_t bytes);
GAME_FN(uint32_t, __thiscall, NiFile_Write, 0x007528A0, NiFile* self, const void* buffer, uint32_t bytes);
GAME_FN(void, __cdecl, NiPath_RemoveDotDots, 0x00752D90, char* path);
GAME_FN(const char*, __cdecl, NiPath_SkipDrive, 0x00753AE0, const char* path);
GAME_FN(NiGlobalStringTable*, __thiscall, GlobalStringTable_Construct, 0x007546F0, void* self);
GAME_FN(void, __thiscall, GlobalStringTable_Destruct, 0x00754610, NiGlobalStringTable* self);

// NiStaticDataManager's tables
using VoidFn = void(__cdecl*)();
GAME_VAR(VoidFn, sdm_preInitCallback, 0x020E213C);
GAME_VAR(VoidFn, sdm_postShutdownCallback, 0x020E2140);
GAME_VAR(uint32_t, sdm_numEntries, 0x020E2130);
GAME_ARRAY(VoidFn, sdm_initFunctions, 0x020E20F0);
GAME_ARRAY(VoidFn, sdm_shutdownFunctions, 0x020E20B0);

// NiTexture's replacement format and its default (NiPixelFormat, 0x44 bytes: 32-bit RGBA)
GAME_ARRAY(uint32_t, NiTexture_ms_kReplacementFormat, 0x020E2200);
GAME_ARRAY(const uint32_t, NiPixelFormat_RGBA32, 0x00BC7750);

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);

} // namespace

// ---- NiFile's defaults

// 0x7526E0: the default CreateDirectory hook
uint8_t __cdecl NiFile_DefaultCreateDirectory(const char* path) {
    return Import<int>(kCreateDirectoryA, path, nullptr) != 0 ? 1 : 0;
}

// 0x752700: the default DirectoryExists hook (no attributes: false)
uint8_t __cdecl NiFile_DefaultDirectoryExists(const char* path) {
    uint32_t attributes = Import<uint32_t>(kGetFileAttributesA, path);
    if (attributes == 0xFFFFFFFF) {
        return 0;
    }
    return uint8_t((attributes >> 4) & 1);   // FILE_ATTRIBUTE_DIRECTORY
}

// 0x753300 and 0x753320: the read and write functions without swapping (lone forwarders to
// Read and Write)
uint32_t __cdecl NiFile_FileRead(NiBinaryStream* stream, void* buffer, uint32_t bytes, uint32_t*, uint32_t) {
    return NiFile_Read(static_cast<NiFile*>(stream), buffer, bytes);
}
uint32_t __cdecl NiFile_FileWrite(NiBinaryStream* stream, const void* buffer, uint32_t bytes, uint32_t*, uint32_t) {
    return NiFile_Write(static_cast<NiFile*>(stream), buffer, bytes);
}

// 0x753390: the default file factory (an NiFile, NiNew'd)
NiFile* __cdecl NiFile_DefaultFileCreate(const char* path, int mode, uint32_t bufferSize) {
    void* memory = NiNew(sizeof(NiFile));
    if (memory == nullptr) {
        return nullptr;
    }
    return NiFile_Construct(memory, path, mode, bufferSize);
}

// 0x7533D0: the default Access hook: whether the file opens in that mode (one on the stack, no
// buffer)
uint8_t __cdecl NiFile_DefaultFileAccess(const char* path, int mode) {
    NiFile file;
    NiFile_Construct(&file, path, mode, 0);
    uint8_t good = file.good;
    NiFile_Destruct(&file);
    return good;
}

// 0x753640: the default message box (MB_TOPMOST); 0
int __cdecl NiMessageBox_Default(const char* text, const char* caption) {
    Import<int>(kMessageBoxA, nullptr, text, caption, 0x40000u);
    return 0;
}

// ---- NiPath

// 0x752E70: ConvertToRelative: the absolute path relative to the directory (the same drive
// only: ".\" then a "..\" per directory up from it to the common part, then the rest); its
// length, 0 when it can't be made
uint32_t __cdecl NiPath_ConvertToRelative(char* relativePath, uint32_t bytes, const char* absolutePath,
                                          const char* relativeToHere) {
    uint32_t length = uint32_t(std::strlen(absolutePath));
    if (bytes <= length || length >= 0x104) {
        return 0;
    }
    char absolute[0x104];
    crt::StrcpyS(absolute, 0x104, absolutePath);
    char here[0x104];
    crt::StrcpyS(here, 0x104, relativeToHere);
    if (here[std::strlen(here) - 1] != '\\') {
        crt::StrcatS(here, 0x104, "\\");
    }
    NiPath_RemoveDotDots(absolute);
    NiPath_RemoveDotDots(here);
    const char* absoluteRest = NiPath_SkipDrive(absolute);
    const char* hereRest = NiPath_SkipDrive(here);
    int driveLength = int(absoluteRest - absolute);
    if (driveLength != int(hereRest - here)) {
        return 0;
    }
    if (crt::Strnicmp(absolute, here, size_t(driveLength)) != 0) {
        return 0;
    }
    crt::StrcpyS(relativePath, bytes, ".\\");
    char common[0x104];
    crt::StrcpyS(common, 0x104, hereRest);
    size_t commonLength = std::strlen(common);
    if (commonLength >= 1 && common[commonLength - 1] == '\\') {
        common[commonLength - 1] = 0;
    }
    while (crt::Strnicmp(absoluteRest, common, std::strlen(common)) != 0) {
        crt::StrcatS(relativePath, bytes, "..\\");
        char* slash = crt::Strrchr(common, '\\');
        if (slash == nullptr) {
            common[0] = 0;
            break;
        }
        *slash = 0;
    }
    size_t skip = std::strlen(common);
    if (absoluteRest[skip] == '\\') {
        skip++;
    }
    crt::StrcatS(relativePath, bytes, absoluteRest + skip);
    return uint32_t(std::strlen(relativePath));
}

// ---- NiStaticDataManager

// 0x753660: the callbacks run before the libraries' inits and after their shutdowns
void __cdecl NiStaticDataManager_SetCallbacks(VoidFn preInit, VoidFn postShutdown) {
    sdm_preInitCallback = preInit;
    sdm_postShutdownCallback = postShutdown;
}

// 0x753680: AddLibrary (a library's init and shutdown, from its static initializer)
void __cdecl NiStaticDataManager_AddLibrary(VoidFn init, VoidFn shutdown) {
    uint32_t index = sdm_numEntries;
    sdm_initFunctions[index] = init;
    sdm_shutdownFunctions[index] = shutdown;
    sdm_numEntries = index + 1;
}

// ---- NiGlobalStringTable

// 0x754610: the destructor: the lock deleted, each bucket's array freed (last first)
void __fastcall NiGlobalStringTable_Destruct(NiGlobalStringTable* self, re::Edx) {
    Import<void>(kDeleteCriticalSection, &self->lock.section);
    for (int i = 512; i >= 0; i--) {
        re::Ptr32<char>* base = self->buckets[i].base;
        self->buckets[i].vtable = reinterpret_cast<void*>(kStringArray_vtable);
        NiFree(base);
    }
}

// 0x7546C0: _SDMShutdown (the table deleted)
void __cdecl NiGlobalStringTable_SDMShutdown() {
    NiGlobalStringTable* table = ms_pkGlobalStringTable;
    if (table != nullptr) {
        GlobalStringTable_Destruct(table);
        NiDelete(table, sizeof(NiGlobalStringTable));
    }
    ms_pkGlobalStringTable = nullptr;
}

// 0x7546F0: the constructor: 513 empty buckets (growing by 1, then by 10), the lock
NiGlobalStringTable* __fastcall NiGlobalStringTable_Construct(NiGlobalStringTable* self, re::Edx) {
    for (int i = 0; i <= 512; i++) {
        NiTArray<char*>& bucket = self->buckets[i];
        bucket.maxSize = 0;
        bucket.growBy = 1;
        bucket.size = 0;
        bucket.esize = 0;
        bucket.base = nullptr;
        bucket.vtable = reinterpret_cast<void*>(kStringArray_vtable);
    }
    self->lock.owner = 0;
    self->lock.lockCount = 0;
    Import<void>(kInitializeCriticalSection, &self->lock.section);
    for (int i = 0; i < 513; i++) {
        self->buckets[i].growBy = 10;
    }
    return self;
}

// 0x754870: _SDMInit (the table made)
void __cdecl NiGlobalStringTable_SDMInit() {
    void* memory = NiNew(sizeof(NiGlobalStringTable));
    if (memory != nullptr) {
        ms_pkGlobalStringTable = GlobalStringTable_Construct(memory);
        return;
    }
    ms_pkGlobalStringTable = nullptr;
}

// ---- NiTexture

// 0x7548A0: the replacement format reset to its default (32-bit RGBA)
void __cdecl NiTexture_ResetReplacementFormat() {
    for (int i = 0; i < 0x11; i++) {
        NiTexture_ms_kReplacementFormat[i] = NiPixelFormat_RGBA32[i];
    }
}

// ---- NiRenderer

// 0x755700: the renderer's slot 53 on an object it keeps data for (what ~NiSkinInstance calls)
void __cdecl NiRenderer_PurgeSkinInstanceData(void* object) {
    if (NiRenderer_ms_pkRenderer != nullptr) {
        Virtual<void>(NiRenderer_ms_pkRenderer, 53, object);
    }
}

#define NISYSLIB_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NISYSLIB_REVERSED("NiFile::DefaultCreateDirectory", 0x007526E0, NiFile_DefaultCreateDirectory);
NISYSLIB_REVERSED("NiFile::DefaultDirectoryExists", 0x00752700, NiFile_DefaultDirectoryExists);
NISYSLIB_REVERSED("NiPath::ConvertToRelative", 0x00752E70, NiPath_ConvertToRelative);
NISYSLIB_REVERSED("NiFile::FileRead", 0x00753300, NiFile_FileRead);
NISYSLIB_REVERSED("NiFile::FileWrite", 0x00753320, NiFile_FileWrite);
NISYSLIB_REVERSED("NiFile::DefaultFileCreate", 0x00753390, NiFile_DefaultFileCreate);
NISYSLIB_REVERSED("NiFile::DefaultFileAccess", 0x007533D0, NiFile_DefaultFileAccess);
NISYSLIB_REVERSED("NiMessageBox::Default", 0x00753640, NiMessageBox_Default);
NISYSLIB_REVERSED("NiStaticDataManager::SetCallbacks", 0x00753660, NiStaticDataManager_SetCallbacks);
NISYSLIB_REVERSED("NiStaticDataManager::AddLibrary", 0x00753680, NiStaticDataManager_AddLibrary);
NISYSLIB_REVERSED("NiGlobalStringTable::~NiGlobalStringTable", 0x00754610, NiGlobalStringTable_Destruct);
NISYSLIB_REVERSED("NiGlobalStringTable::_SDMShutdown", 0x007546C0, NiGlobalStringTable_SDMShutdown);
NISYSLIB_REVERSED("NiGlobalStringTable::NiGlobalStringTable", 0x007546F0, NiGlobalStringTable_Construct);
NISYSLIB_REVERSED("NiGlobalStringTable::_SDMInit", 0x00754870, NiGlobalStringTable_SDMInit);
NISYSLIB_REVERSED("NiTexture::ResetReplacementFormat", 0x007548A0, NiTexture_ResetReplacementFormat);
NISYSLIB_REVERSED("NiRenderer::PurgeSkinInstanceData", 0x00755700, NiRenderer_PurgeSkinInstanceData);

} // namespace ni
