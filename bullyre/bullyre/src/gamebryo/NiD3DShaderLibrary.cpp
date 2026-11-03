// NiD3DShaderLibrary (vtable 0x99C0E4, 0x18 bytes: NiShaderLibrary's 0xC, then +0xC, +0x10 and the
// shader program factory's reference at +0x14), NiD3DShaderProgramFactory's statics (the shader
// directory at 0x20E6E8C, the renderer at 0x20E6EA8 and its device at 0x20E6EA4) and a matrix copy
// helper.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kLibraryVtable = 0x0099C0E4;
GAME_VAR(char*, ms_pcShaderProgramDirectory, 0x020E6E8C);
GAME_VAR(void*, ms_pkProgramFactoryRenderer, 0x020E6EA8);
GAME_VAR(void*, ms_pkProgramFactoryDevice, 0x020E6EA4);

GAME_FN(void*, __thiscall, ShaderLibrary_ConstructBase, 0x007AB870, void* self, const char* name);
GAME_FN(void, __thiscall, ShaderLibrary_DestructBase, 0x007AB840, void* self);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);   // the game's operator delete
GAME_FN(void, __thiscall, Library_Destruct, 0x00889E70, void* self);

} // namespace

// 0x889E40 (thiscall): NiD3DShaderLibrary::NiD3DShaderLibrary
uint8_t* __fastcall Library_Construct(uint8_t* self, re::Edx, const char* name) {
    ShaderLibrary_ConstructBase(self, name);
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<uint32_t>(self, 0) = kLibraryVtable;
    At<void*>(self, 0x14) = nullptr;
    return self;
}

// 0x889E70 (thiscall): NiD3DShaderLibrary::~NiD3DShaderLibrary
void __fastcall Library_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kLibraryVtable;
    if (At<void*>(self, 0x14) != nullptr) {
        ReleaseRef(At<void*>(self, 0x14));
        At<void*>(self, 0x14) = nullptr;
    }
    At<uint32_t>(self, 0x10) = 0;
    ReleaseRef(At<void*>(self, 0x14));
    ShaderLibrary_DestructBase(self);
}

// 0x889ED0: NiD3DShaderProgramFactory::SetShaderProgramDirectory (the old one freed with the
// game's delete though it came from NiMalloc, and left dangling when no new one is given)
void __cdecl ProgramFactory_SetDirectory(const char* directory) {
    Free(ms_pcShaderProgramDirectory);
    if (directory != nullptr && *directory != '\0') {
        size_t size = 0;
        while (directory[size] != '\0') {
            size++;
        }
        size++;
        ms_pcShaderProgramDirectory = static_cast<char*>(NiMalloc(uint32_t(size)));
        crt::StrcpyS(ms_pcShaderProgramDirectory, size, directory);
    }
}

// 0x889F20 (thiscall): NiD3DShaderLibrary::SetShaderProgramFactory (a reference)
void __fastcall Library_SetProgramFactory(uint8_t* self, re::Edx, void* factory) {
    void* old = At<void*>(self, 0x14);
    if (old != factory) {
        ReleaseRef(old);
        At<void*>(self, 0x14) = factory;
        AddRefRef(factory);
    }
}

// 0x889F70 (thiscall): NiD3DShaderLibrary's deleting destructor
uint8_t* __fastcall Library_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    Library_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x889F90: NiD3DShaderProgramFactory::SetD3DRenderer (the renderer, its device held)
void __cdecl ProgramFactory_SetD3DRenderer(uint8_t* renderer) {
    ms_pkProgramFactoryRenderer = renderer;
    if (renderer != nullptr) {
        void* device = At<void*>(renderer, kDevice);
        if (ms_pkProgramFactoryDevice != nullptr) {
            Com<long>(ms_pkProgramFactoryDevice, kRelease);
        }
        ms_pkProgramFactoryDevice = device;
        if (device != nullptr) {
            Com<long>(device, kAddRef);
        }
        return;
    }
    if (ms_pkProgramFactoryDevice != nullptr) {
        Com<long>(ms_pkProgramFactoryDevice, kRelease);
    }
    ms_pkProgramFactoryDevice = nullptr;
}

// 0x889FF0: a 4x4 matrix copied (the source in eax, the destination in ecx: not hooked)
void CopyMatrix4(uint32_t* to, const uint32_t* from) {
    for (int i = 0; i < 16; i++) {
        to[i] = from[i];
    }
}
RE_UNHOOKED_FN("NiD3DUtility::CopyMatrix4", 0x00889FF0, &CopyMatrix4);

#define C39N_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39N_REVERSED("NiD3DShaderLibrary::NiD3DShaderLibrary", 0x00889E40, Library_Construct);
C39N_REVERSED("NiD3DShaderLibrary::~NiD3DShaderLibrary", 0x00889E70, Library_DestructImpl);
C39N_REVERSED("NiD3DShaderProgramFactory::SetShaderProgramDirectory", 0x00889ED0, ProgramFactory_SetDirectory);
C39N_REVERSED("NiD3DShaderLibrary::SetShaderProgramFactory", 0x00889F20, Library_SetProgramFactory);
C39N_REVERSED("NiD3DShaderLibrary::DeletingDestructor", 0x00889F70, Library_DeletingDestructor);
C39N_REVERSED("NiD3DShaderProgramFactory::SetD3DRenderer", 0x00889F90, ProgramFactory_SetD3DRenderer);

} // namespace ni::c39
