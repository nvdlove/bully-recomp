// The D3D10 renderer's description of the system (Gamebryo's NiD3D10SystemDesc and its parts),
// DXGI loaded at run time ("DXGI.dll", CreateDXGIFactory), the DXGI formats' names, sizes and
// pixel formats, and NiD3D10Renderer::CreationParameters. Bully never asks for D3D10: cold code,
// written exactly all the same (src/gamebryo/NiD3D10Select.cpp reads these).
//
// NiD3D10SystemDesc (vtable 0x99E6D0, an NiRefObject, 0x28 bytes, one instance 0x20E75B8): +0x8
// its adapters (NiTPrimitiveArray), +0x18 the DXGI module, +0x1C CreateDXGIFactory, +0x20 the
// IDXGIFactory, +0x24 the enumeration is valid. Guarded by the critical section 0x20E75C0.
// NiD3D10AdapterDesc (0x144): +0x0 its index, +0x4 the IDXGIAdapter, +0x8 DXGI_ADAPTER_DESC
// (0x124), +0x12C its outputs (NiTPrimitiveArray), +0x13C / +0x140 the hardware and reference
// device descriptions. NiD3D10DeviceDesc (0x8C): +0x0 its adapter, +0x4 the driver type, +0x8 the
// R8G8B8A8_UNORM quality levels for 1..32 samples, +0x88 the most samples. NiD3D10OutputDesc
// (0x70): +0x0 its index, +0x4 its adapter, +0x8 the IDXGIOutput, +0xC DXGI_OUTPUT_DESC (0x5C),
// +0x68 / +0x6C its display modes (DXGI_MODE_DESC, 0x1C each; NiMalloc'd) and their count.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c42 {
namespace {

using dx9::Com;
using sys::At;
using sys::Import;
using sys::NiCriticalSection;
using sys::Virtual;

constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kSystemDescVtable = 0x0099E6D0;
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;
constexpr uintptr_t kLoadLibraryA = 0x008FC0C8;
constexpr uintptr_t kGetProcAddress = 0x008FC0C4;
constexpr uintptr_t kFreeLibrary = 0x008FC0C0;
constexpr uintptr_t kInterlockedIncrement = 0x008FC174;
constexpr uintptr_t kInterlockedDecrement = 0x008FC170;
constexpr uintptr_t kGetClientRect = 0x008FC22C;
constexpr uint32_t DXGI_ERROR_NOT_FOUND = 0x887A0002;

// the game's strings
inline const char* Text(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}
constexpr uintptr_t kIIDFactory = 0x0099E59C;   // IID_IDXGIFactory

GAME_VAR(uint32_t, ms_uiObjects, 0x020E2168);
GAME_VAR(uint8_t*, ms_pkSystemDesc, 0x020E75B8);
GAME_VAR(NiCriticalSection, ms_kSystemDescLock, 0x020E75C0);
constexpr uintptr_t kPixelFormats = 0x020E75E0;   // NiPixelFormat [0x58] (0x48 each), by DXGI_FORMAT - 1

// IDXGIFactory 7 EnumAdapters; IDXGIAdapter 7 EnumOutputs, 8 GetDesc; IDXGIOutput 7 GetDesc, 8
// GetDisplayModeList; ID3D10Device 91 CheckMultisampleQualityLevels
constexpr int kRelease = 2;
constexpr int kEnumAdapters = 7;
constexpr int kEnumOutputs = 7;
constexpr int kAdapterGetDesc = 8;
constexpr int kOutputGetDesc = 7;
constexpr int kGetDisplayModeList = 8;
constexpr int kCheckMultisampleQualityLevels = 91;

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, PtrArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, PtrArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(uint8_t, __thiscall, PixelFormat_HasComponent, 0x007DD390, const void* self, uint32_t component);
// the D3D10 renderer's (this chunk's other files)
GAME_FN(void, __cdecl, D3D10Warning, 0x008AF9B0, const char* format, ...);
GAME_FN(void, __cdecl, D3D10Error, 0x008AFAC0, uint32_t code, const char* format, ...);
GAME_FN(void*, __cdecl, D3D10_CreateDevice, 0x008AD8F0, void* adapter, uint32_t driverType, uint32_t flags);
// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, OutputDesc_EnumerateDisplayModes, 0x008A7910, void* self);
GAME_FN(uint8_t, __thiscall, SystemDesc_ReleaseDXGI, 0x008A7A80, void* self);
GAME_FN(uint8_t, __thiscall, AdapterDesc_EnumerateDevices, 0x008A7B10, void* self, void* factory);
GAME_FN(uint8_t, __thiscall, SystemDesc_LoadDXGI, 0x008A7C60, void* self);
GAME_FN(void, __thiscall, AdapterDesc_Destruct, 0x008A7D40, void* self);
GAME_FN(uint8_t, __thiscall, SystemDesc_CreateFactory, 0x008A7DF0, void* self);
GAME_FN(uint8_t, __thiscall, SystemDesc_ReleaseAdapters, 0x008A7EB0, void* self);
GAME_FN(uint8_t, __thiscall, AdapterDesc_EnumerateOutputs, 0x008A7F30, void* self, void* factory);
GAME_FN(void, __thiscall, SystemDesc_Destruct, 0x008A80A0, void* self);
GAME_FN(uint8_t, __thiscall, SystemDesc_EnumerateAdapters, 0x008A8100, void* self);
GAME_FN(uint8_t*, __thiscall, SystemDesc_Construct, 0x008A82B0, void* self);
GAME_FN(const char*, __cdecl, D3D10_GetFormatName, 0x008A88B0, uint32_t format, int full);

inline void ComRelease(void* object) {
    if (object != nullptr) {
        Com<long>(object, kRelease);
    }
}

// an NiPointer let go as the library inlines it (DeleteThis when it was the last)
inline void ReleaseRef(void* object) {
    if (object != nullptr && Import<long>(kInterlockedDecrement, &At<uint32_t>(object, 4)) == 0) {
        Virtual<void>(object, 1);
    }
}

// NiTArray::Add at the end, as the enumerations inline it (grown by its growth step when full)
inline void ArrayAppend(uint8_t* array, re::Ptr32<void> value) {
    uint32_t size = At<uint16_t>(array, 0xA);
    if (size >= At<uint16_t>(array, 8)) {
        PtrArray_SetSize(array, At<uint16_t>(array, 0xE) + size);
    }
    PtrArray_SetAt(array, size, &value);
}

// an output description deleted
inline void DeleteOutput(uint8_t* output) {
    ComRelease(At<void*>(output, 8));
    NiFree(At<void*>(output, 0x68));
    NiDelete(output, 0x70);
}

constexpr uint32_t kModeFormats[4] = {0x1C, 0xA, 0x18, 0x1D};   // R8G8B8A8_UNORM, R16G16B16A16_FLOAT,
                                                               // R10G10B10A2_UNORM, R8G8B8A8_UNORM_SRGB

// the format names (the game's strings), DXGI_FORMAT 1..0x57
constexpr uintptr_t kFormatNames[0x57] = {
    0x0099F18C, 0x0099F16C, 0x0099F14C, 0x0099F12C, 0x0099F10C, 0x0099F0F0,
    0x0099F0D4, 0x0099F0B8, 0x0099F094, 0x0099F074, 0x0099F054, 0x0099F034,
    0x0099F014, 0x0099EFF4, 0x0099EFD8, 0x0099EFBC, 0x0099EFA4, 0x0099EF8C,
    0x0099EF70, 0x0099EF50, 0x0099EF2C, 0x0099EF04, 0x0099EEE0, 0x0099EEC0,
    0x0099EEA4, 0x0099EE88, 0x0099EE6C, 0x0099EE50, 0x0099EE30, 0x0099EE14,
    0x0099EDF8, 0x0099EDDC, 0x0099EDC0, 0x0099EDA4, 0x0099ED88, 0x0099ED6C,
    0x0099ED54, 0x0099ED3C, 0x0099ED24, 0x0099ED0C, 0x0099ECF0, 0x0099ECD4,
    0x0099ECBC, 0x0099ECA4, 0x0099EC88, 0x0099EC68, 0x0099EC48, 0x0099EC28,
    0x0099EC0C, 0x0099EBF4, 0x0099EBE0, 0x0099EBC8, 0x0099EBB4, 0x0099EB9C,
    0x0099EB84, 0x0099EB6C, 0x0099EB54, 0x0099EB3C, 0x0099EB24, 0x0099EB10,
    0x0099EAFC, 0x0099EAE8, 0x0099EAD4, 0x0099EAC0, 0x0099EAAC, 0x0099EA98,
    0x0099EA84, 0x0099EA6C, 0x0099EA48, 0x0099EA2C, 0x0099EA14, 0x0099E9F4,
    0x0099E9D8, 0x0099E9BC, 0x0099E99C, 0x0099E980, 0x0099E964, 0x0099E948,
    0x0099E92C, 0x0099E910, 0x0099E8F8, 0x0099E8E0, 0x0099E8C4, 0x0099E8AC,
    0x0099E894, 0x0099E878, 0x0099E85C,
};

} // namespace

// ---- NiD3D10DeviceDesc / NiD3D10OutputDesc / NiD3D10AdapterDesc

// 0x8A78C0 (thiscall): NiD3D10DeviceDesc::GetMaxMultisampleCount
uint32_t __fastcall NiD3D10DeviceDesc_GetMaxMultisampleCount(uint8_t* self, re::Edx) {
    return At<uint32_t>(self, 0x88);
}

// 0x8A78D0 (thiscall): NiD3D10DeviceDesc::GetMultisampleQualityLevels (true when there are any)
uint8_t __fastcall NiD3D10DeviceDesc_GetMultisampleQualityLevels(uint8_t* self, re::Edx, uint32_t count,
                                                               uint32_t* levels) {
    *levels = 0;
    if (count - 1 < 0x20) {
        *levels = At<uint32_t>(self, 4 + int(count) * 4);
    }
    return *levels != 0 ? 1 : 0;
}

// 0x8A7900 (thiscall, 4 bytes): NiD3D10OutputDesc::GetDisplayModes
uint32_t* __fastcall NiD3D10OutputDesc_GetDisplayModes(uint8_t* self, re::Edx) {
    return At<uint32_t*>(self, 0x68);
}
RE_UNHOOKED_FN("NiD3D10OutputDesc::GetDisplayModes", 0x008A7900, &NiD3D10OutputDesc_GetDisplayModes);   // (4 bytes)

// 0x8A7910 (thiscall): NiD3D10OutputDesc::EnumerateDisplayModes (four formats' modes in one
// array). The game keeps a format's count uninitialised when its first query fails and reads it
// in the second pass; it's zero here
uint8_t __fastcall NiD3D10OutputDesc_EnumerateDisplayModes(uint8_t* self, re::Edx) {
    uint32_t counts[4] = {};
    uint32_t total = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t count = 0;
        long hr = Com<long>(At<void*>(self, 8), kGetDisplayModeList, kModeFormats[i], 0u, &count,
                            static_cast<void*>(nullptr));
        if (hr < 0) {
            if (uint32_t(hr) != DXGI_ERROR_NOT_FOUND) {
                D3D10Warning(Text(0x0099E460), hr);
            }
        } else {
            total += count;
            counts[i] = count;
        }
    }
    if (total == 0) {
        return 0;
    }
    auto* modes = static_cast<uint8_t*>(NiMalloc(total * 0x1C));
    At<uint8_t*>(self, 0x68) = modes;
    for (int i = 0; i < 4; i++) {
        uint32_t count = counts[i];
        if (count == 0) {
            continue;
        }
        uint32_t format = kModeFormats[i];
        long hr = Com<long>(At<void*>(self, 8), kGetDisplayModeList, format, 0u, &count, modes);
        if (hr < 0) {
            D3D10Warning(Text(0x0099E460), hr);
            NiFree(At<void*>(self, 0x68));
            At<void*>(self, 0x68) = nullptr;
            return 0;
        }
        if (count != counts[i]) {
            D3D10Warning(Text(0x0099E3F8), D3D10_GetFormatName(format, 0));
            NiFree(At<void*>(self, 0x68));
            At<void*>(self, 0x68) = nullptr;
            return 0;
        }
        modes += count * 0x1C;
    }
    At<uint32_t>(self, 0x6C) = total;
    return 1;
}

// 0x8A78A0 (thiscall): NiD3D10AdapterDesc::GetHWDevice
uint8_t* __fastcall NiD3D10AdapterDesc_GetHWDevice(uint8_t* self, re::Edx) {
    return At<uint8_t*>(self, 0x13C);
}

// 0x8A78B0 (thiscall): NiD3D10AdapterDesc::GetRefDevice
uint8_t* __fastcall NiD3D10AdapterDesc_GetRefDevice(uint8_t* self, re::Edx) {
    return At<uint8_t*>(self, 0x140);
}

// 0x8A7AE0 (thiscall): NiD3D10AdapterDesc::GetOutputCount
uint32_t __fastcall NiD3D10AdapterDesc_GetOutputCount(uint8_t* self, re::Edx) {
    return At<uint16_t>(self, 0x138);
}

// 0x8A7AF0 (thiscall): NiD3D10AdapterDesc::GetOutputDesc
uint8_t* __fastcall NiD3D10AdapterDesc_GetOutputDesc(uint8_t* self, re::Edx, uint32_t index) {
    if (index < At<uint16_t>(self, 0x136)) {
        return At<re::Ptr32<uint8_t>*>(self, 0x130)[index];
    }
    return nullptr;
}

// 0x8A7B10 (thiscall): NiD3D10AdapterDesc::EnumerateDevices (a hardware and a reference device
// made to read their multisample levels; true when either could be made)
uint8_t __fastcall NiD3D10AdapterDesc_EnumerateDevices(uint8_t* self, re::Edx, void* factory) {
    if (factory == nullptr) {
        D3D10Warning(Text(0x0099E4A4));
        return 0;
    }
    const uint32_t types[2] = {0, 1};   // D3D10_DRIVER_TYPE_HARDWARE, _REFERENCE
    for (uint32_t i = 0; i < 2; i++) {
        uint32_t type = types[i];
        void* device = D3D10_CreateDevice(At<void*>(self, 4), type, 0);
        if (device == nullptr) {
            continue;
        }
        auto* desc = static_cast<uint8_t*>(NiNew(0x8C));
        if (desc != nullptr) {
            At<void*>(desc, 0) = nullptr;
            At<uint32_t>(desc, 4) = 2;
            At<uint32_t>(desc, 0x88) = 0;
            crt::Memset(desc + 8, 0, 0x80);
        }
        At<uint32_t>(desc, 4) = type;
        At<uint8_t*>(desc, 0) = self;
        uint32_t* levels = &At<uint32_t>(desc, 8);
        for (uint32_t samples = 1; samples <= 0x20; samples++, levels++) {
            long hr = Com<long>(device, kCheckMultisampleQualityLevels, 0x1Cu, samples, levels);
            if (hr < 0) {
                *levels = 0;
            } else {
                At<uint32_t>(desc, 0x88) = samples;
            }
        }
        Com<long>(device, kRelease);
        if (types[i] == 0) {
            At<uint8_t*>(self, 0x13C) = desc;
        } else {
            At<uint8_t*>(self, 0x140) = desc;
        }
    }
    return At<void*>(self, 0x13C) != nullptr || At<void*>(self, 0x140) != nullptr ? 1 : 0;
}
RE_UNHOOKED("NiD3D10AdapterDesc::EnumerateDevices_8A7B33", 0x008A7B33);   // (0x8A7B10's loop, not
                                                                         //  an entry point)

// 0x8A7D40 (thiscall): NiD3D10AdapterDesc::~NiD3D10AdapterDesc (its adapter, outputs and devices)
void __fastcall NiD3D10AdapterDesc_Destruct(uint8_t* self, re::Edx) {
    ComRelease(At<void*>(self, 4));
    uint32_t count = At<uint16_t>(self, 0x136);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* output = At<re::Ptr32<uint8_t>*>(self, 0x130)[i];
        if (output != nullptr) {
            DeleteOutput(output);
        }
    }
    if (At<void*>(self, 0x13C) != nullptr) {
        NiDelete(At<void*>(self, 0x13C), 0x8C);
    }
    if (At<void*>(self, 0x140) != nullptr) {
        NiDelete(At<void*>(self, 0x140), 0x8C);
    }
    At<uint32_t>(self, 0x12C) = kPrimitiveArrayVtable;
    NiFree(At<void*>(self, 0x130));
}

// 0x8A7F30 (thiscall): NiD3D10AdapterDesc::EnumerateOutputs (each output described with its
// modes; true when it has any)
uint8_t __fastcall NiD3D10AdapterDesc_EnumerateOutputs(uint8_t* self, re::Edx, void* factory) {
    if (factory == nullptr) {
        D3D10Warning(Text(0x0099E698));
        return 0;
    }
    void* output = nullptr;
    uint32_t index = 0;
    long hr = Com<long>(At<void*>(self, 4), kEnumOutputs, 0u, &output);
    while (hr >= 0) {
        auto* desc = static_cast<uint8_t*>(NiNew(0x70));
        if (desc != nullptr) {
            At<uint32_t>(desc, 0) = 0;
            At<void*>(desc, 4) = nullptr;
            At<void*>(desc, 8) = nullptr;
            At<void*>(desc, 0x68) = nullptr;
            At<uint32_t>(desc, 0x6C) = 0;
            crt::Memset(desc + 0xC, 0, 0x5C);
        }
        At<uint32_t>(desc, 0) = index;
        At<uint8_t*>(desc, 4) = self;
        At<void*>(desc, 8) = output;
        long described = Com<long>(output, kOutputGetDesc, desc + 0xC);
        index++;
        if (described < 0) {
            D3D10Warning(Text(0x0099E640), described);
            DeleteOutput(desc);
        } else if (OutputDesc_EnumerateDisplayModes(desc) == 0) {
            D3D10Warning(Text(0x0099E5F0));
            DeleteOutput(desc);
        } else {
            ArrayAppend(self + 0x12C, desc);
        }
        output = nullptr;
        hr = Com<long>(At<void*>(self, 4), kEnumOutputs, index, &output);
    }
    if (uint32_t(hr) != DXGI_ERROR_NOT_FOUND) {
        D3D10Warning(Text(0x0099E5AC), hr);
    }
    return At<uint16_t>(self, 0x136) != 0 ? 1 : 0;
}

// ---- NiD3D10SystemDesc

// 0x8A7890 (thiscall, 4 bytes): NiD3D10SystemDesc::IsEnumerationValid
uint8_t __fastcall NiD3D10SystemDesc_IsEnumerationValid(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, 0x24);
}
RE_UNHOOKED_FN("NiD3D10SystemDesc::IsEnumerationValid", 0x008A7890, &NiD3D10SystemDesc_IsEnumerationValid);   // (4 bytes)

// 0x8A7A50 (thiscall): NiD3D10SystemDesc::GetAdapterCount
uint32_t __fastcall NiD3D10SystemDesc_GetAdapterCount(uint8_t* self, re::Edx) {
    return At<uint16_t>(self, 0x14);
}

// 0x8A7A60 (thiscall): NiD3D10SystemDesc::GetAdapterDesc
uint8_t* __fastcall NiD3D10SystemDesc_GetAdapterDesc(uint8_t* self, re::Edx, uint32_t index) {
    if (index < At<uint16_t>(self, 0x12)) {
        return At<re::Ptr32<uint8_t>*>(self, 0xC)[index];
    }
    return nullptr;
}

// 0x8A7A80 (thiscall): NiD3D10SystemDesc::ReleaseDXGI (the module freed)
uint8_t __fastcall NiD3D10SystemDesc_ReleaseDXGI(uint8_t* self, re::Edx) {
    sys::Lock(ms_kSystemDescLock);
    if (At<void*>(self, 0x18) != nullptr) {
        Import<int>(kFreeLibrary, At<void*>(self, 0x18));
        At<void*>(self, 0x18) = nullptr;
        At<void*>(self, 0x1C) = nullptr;
    }
    sys::Unlock(ms_kSystemDescLock);
    return 1;
}

// 0x8A7C60 (thiscall): NiD3D10SystemDesc::LoadDXGI ("DXGI.dll" and its CreateDXGIFactory)
uint8_t __fastcall NiD3D10SystemDesc_LoadDXGI(uint8_t* self, re::Edx) {
    sys::Lock(ms_kSystemDescLock);
    if (At<void*>(self, 0x18) == nullptr) {
        void* module = Import<void*>(kLoadLibraryA, Text(0x0099E544));
        At<void*>(self, 0x18) = module;
        if (module == nullptr) {
            D3D10Error(3, nullptr);
            sys::Unlock(ms_kSystemDescLock);
            return 0;
        }
        void* create = Import<void*>(kGetProcAddress, module, Text(0x0099E530));
        At<void*>(self, 0x1C) = create;
        if (create == nullptr) {
            D3D10Error(3, Text(0x0099E4E0));
            sys::Unlock(ms_kSystemDescLock);
            SystemDesc_ReleaseDXGI(self);
            return 0;
        }
    }
    sys::Unlock(ms_kSystemDescLock);
    return 1;
}

// 0x8A7DF0 (thiscall): NiD3D10SystemDesc::CreateFactory (the error reported, the factory let go
// on failure; quirk kept: the message is passed as the error's format)
uint8_t __fastcall NiD3D10SystemDesc_CreateFactory(uint8_t* self, re::Edx) {
    re::Ptr32<void>* factory = &At<void*>(self, 0x20);
    auto create = At<long(__stdcall*)(const void*, re::Ptr32<void>*)>(self, 0x1C);
    long hr = create(reinterpret_cast<const void*>(kIIDFactory), factory);
    char message[0x400];
    if (hr >= 0) {
        if (*factory != nullptr) {
            return 1;
        }
        NiSprintf(message, 0x400, Text(0x0099E550));
    } else {
        NiSprintf(message, 0x400, Text(0x0099E584), hr);
    }
    D3D10Error(5, message);
    if (*factory != nullptr) {
        Com<long>(*factory, kRelease);
        *factory = nullptr;
    }
    return 0;
}

// 0x8A7EB0 (thiscall): NiD3D10SystemDesc::ReleaseAdapters (each deleted and taken out)
uint8_t __fastcall NiD3D10SystemDesc_ReleaseAdapters(uint8_t* self, re::Edx) {
    uint32_t count = At<uint16_t>(self, 0x12);
    At<uint8_t>(self, 0x24) = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* adapter = At<re::Ptr32<uint8_t>*>(self, 0xC)[i];
        if (adapter != nullptr) {
            AdapterDesc_Destruct(adapter);
            NiDelete(adapter, 0x144);
        }
        if (i < At<uint16_t>(self, 0x12)) {
            re::Ptr32<uint8_t>* base = At<re::Ptr32<uint8_t>*>(self, 0xC);
            uint8_t* element = base[i];
            base[i] = nullptr;
            if (element != nullptr) {
                At<uint16_t>(self, 0x14)--;
            }
            uint16_t last = uint16_t(At<uint16_t>(self, 0x12) - 1);
            if (i == last) {
                At<uint16_t>(self, 0x12) = last;
            }
        }
    }
    return 1;
}

// 0x8A80A0 (thiscall): NiD3D10SystemDesc::~NiD3D10SystemDesc (the instance forgotten)
void __fastcall NiD3D10SystemDesc_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kSystemDescVtable;
    ms_pkSystemDesc = nullptr;
    SystemDesc_ReleaseAdapters(self);
    if (At<void*>(self, 0x20) != nullptr) {
        Com<long>(At<void*>(self, 0x20), kRelease);
        At<void*>(self, 0x20) = nullptr;
    }
    SystemDesc_ReleaseDXGI(self);
    At<uint32_t>(self, 8) = kPrimitiveArrayVtable;
    NiFree(At<void*>(self, 0xC));
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Import<long>(kInterlockedDecrement, &ms_uiObjects);
}

// 0x8A8100 (thiscall): NiD3D10SystemDesc::EnumerateAdapters (each adapter with its devices and
// outputs; valid when there's any)
uint8_t __fastcall NiD3D10SystemDesc_EnumerateAdapters(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 0x24) = 0;
    if (At<void*>(self, 0x20) == nullptr) {
        return 0;
    }
    void* adapter = nullptr;
    uint32_t index = 0;
    long hr = Com<long>(At<void*>(self, 0x20), kEnumAdapters, 0u, &adapter);
    while (hr >= 0) {
        auto* desc = static_cast<uint8_t*>(NiNew(0x144));
        if (desc != nullptr) {
            At<uint32_t>(desc, 0) = 0xFFFFFFFF;
            At<void*>(desc, 4) = nullptr;
            At<uint16_t>(desc, 0x134) = 0;
            At<uint16_t>(desc, 0x13A) = 1;
            At<uint16_t>(desc, 0x136) = 0;
            At<uint16_t>(desc, 0x138) = 0;
            At<void*>(desc, 0x130) = nullptr;
            At<uint32_t>(desc, 0x12C) = kPrimitiveArrayVtable;
            At<void*>(desc, 0x13C) = nullptr;
            At<void*>(desc, 0x140) = nullptr;
            crt::Memset(desc + 8, 0, 0x124);
        }
        At<void*>(desc, 4) = adapter;
        At<uint32_t>(desc, 0) = index;
        long described = Com<long>(adapter, kAdapterGetDesc, desc + 8);
        index++;
        bool keep = false;
        if (described < 0) {
            D3D10Warning(Text(0x0099E7B8), described);
        } else if (AdapterDesc_EnumerateDevices(desc, At<void*>(self, 0x20)) == 0) {
            D3D10Warning(Text(0x0099E768));
        } else if (AdapterDesc_EnumerateOutputs(desc, At<void*>(self, 0x20)) == 0) {
            D3D10Warning(Text(0x0099E718));
        } else {
            keep = true;
        }
        if (keep) {
            ArrayAppend(self + 8, desc);
        } else {
            AdapterDesc_Destruct(desc);
            NiDelete(desc, 0x144);
        }
        adapter = nullptr;
        hr = Com<long>(At<void*>(self, 0x20), kEnumAdapters, index, &adapter);
    }
    if (uint32_t(hr) != DXGI_ERROR_NOT_FOUND) {
        D3D10Warning(Text(0x0099E6D8), hr);
    }
    if (At<uint16_t>(self, 0x12) != 0) {
        At<uint8_t>(self, 0x24) = 1;
    }
    return At<uint8_t>(self, 0x24);
}

// 0x8A82B0 (thiscall): NiD3D10SystemDesc::NiD3D10SystemDesc (DXGI loaded, the factory made,
// the adapters enumerated)
uint8_t* __fastcall NiD3D10SystemDesc_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Import<long>(kInterlockedIncrement, &ms_uiObjects);
    At<uint32_t>(self, 0) = kSystemDescVtable;
    At<uint16_t>(self, 0x10) = 0;
    At<uint16_t>(self, 0x16) = 1;
    At<uint16_t>(self, 0x12) = 0;
    At<uint16_t>(self, 0x14) = 0;
    At<void*>(self, 0xC) = nullptr;
    At<uint32_t>(self, 8) = kPrimitiveArrayVtable;
    At<void*>(self, 0x18) = nullptr;
    At<void*>(self, 0x1C) = nullptr;
    At<void*>(self, 0x20) = nullptr;
    At<uint8_t>(self, 0x24) = 0;
    if (SystemDesc_LoadDXGI(self) != 0) {
        if (SystemDesc_CreateFactory(self) == 0) {
            SystemDesc_ReleaseDXGI(self);
            return self;
        }
        SystemDesc_EnumerateAdapters(self);
    }
    return self;
}

// 0x8A8330 (thiscall): NiD3D10SystemDesc's deleting destructor
void* __fastcall NiD3D10SystemDesc_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    SystemDesc_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x28);
    }
    return self;
}

// 0x8A8350 (thiscall): NiD3D10SystemDesc::Enumerate (again)
uint8_t __fastcall NiD3D10SystemDesc_Enumerate(uint8_t* self, re::Edx) {
    SystemDesc_ReleaseAdapters(self);
    SystemDesc_EnumerateAdapters(self);
    return At<uint8_t>(self, 0x24);
}

// 0x8A8370: NiD3D10Renderer::GetSystemDesc (the instance, made the first time; the reference
// assigned: the old one let go first)
void __cdecl NiD3D10Renderer_GetSystemDesc(re::Ptr32<uint8_t>* result) {
    sys::Lock(ms_kSystemDescLock);
    if (ms_pkSystemDesc == nullptr) {
        void* memory = NiNew(0x28);
        uint8_t* desc = memory != nullptr ? SystemDesc_Construct(memory) : nullptr;
        if (*result != desc) {
            ReleaseRef(*result);
            *result = desc;
            if (desc != nullptr) {
                Import<long>(kInterlockedIncrement, &At<uint32_t>(desc, 4));
            }
        }
        ms_pkSystemDesc = *result;
    } else if (*result != ms_pkSystemDesc) {
        uint8_t* desc = ms_pkSystemDesc;
        ReleaseRef(*result);
        *result = desc;
        if (desc != nullptr) {
            Import<long>(kInterlockedIncrement, &At<uint32_t>(desc, 4));
        }
    }
    sys::Unlock(ms_kSystemDescLock);
}

// ---- the DXGI formats

// 0x8A8450: NiD3D10PixelFormat::GetPixelFormat (the NiPixelFormat for a DXGI_FORMAT, 1..0x58)
const void* __cdecl NiD3D10PixelFormat_GetPixelFormat(uint32_t format) {
    if (format - 1 < 0x58) {
        return reinterpret_cast<const void*>(kPixelFormats + (format - 1) * 0x48);
    }
    return nullptr;
}

// 0x8A87E0: NiD3D10PixelFormat::GetBitsPerPixel (0xFFFFFFFF when unknown; 0 the block formats)
uint32_t __cdecl NiD3D10PixelFormat_GetBitsPerPixel(uint32_t format) {
    if (format >= 1 && format <= 4) {
        return 0x80;
    }
    if (format >= 5 && format <= 8) {
        return 0x60;
    }
    if (format >= 9 && format <= 0x16) {
        return 0x40;
    }
    if ((format >= 0x17 && format <= 0x2F) || format == 0x43 || format == 0x57 || format == 0x58) {
        return 0x20;
    }
    if ((format >= 0x30 && format <= 0x3B) || format == 0x44 || format == 0x45 || format == 0x55 ||
        format == 0x56) {
        return 0x10;
    }
    if (format >= 0x3C && format <= 0x41) {
        return 8;
    }
    if (format == 0x42) {
        return 1;
    }
    if (format >= 0x46 && format <= 0x54) {
        return 0;
    }
    return 0xFFFFFFFF;
}

// 0x8A88B0: NiD3D10Renderer::GetFormatName ("DXGI_FORMAT_" left off unless full)
const char* __cdecl NiD3D10Renderer_GetFormatName(uint32_t format, int full) {
    const char* name = format - 1 < 0x57 ? Text(kFormatNames[format - 1]) : Text(0x0099E820);   // "Unknown format"
    if (!re::Flag(full) && crt::Strstr(name, Text(0x0099E810)) != nullptr) {
        return name + 0xC;
    }
    return name;
}

// 0x8A8D80: NiD3D10PixelFormat::DetermineDXGIFormat (the format's renderer hint, else from its
// kind: the block formats (sRGB or not), the depth formats by their bits; 0 when unknown)
uint32_t __cdecl NiD3D10PixelFormat_DetermineDXGIFormat(const uint8_t* pixelFormat) {
    uint32_t hint = At<uint32_t>(pixelFormat, 0xC);
    if (hint != 0xFFFFFFFF) {
        return hint;
    }
    uint8_t bits = At<uint8_t>(pixelFormat, 1);
    uint32_t kind = At<uint32_t>(pixelFormat, 4);
    // (the first unknown-type component searched for, the result unused)
    if (At<uint32_t>(pixelFormat, 0x14) == 0x13) {
        return 0;
    }
    if (kind > 0xF) {
        return 0;
    }
    static constexpr uint8_t kCases[16] = {0, 0, 4, 4, 0, 1, 2, 4, 0, 0, 4, 0, 0, 0, 0, 3};
    bool srgb = At<uint8_t>(pixelFormat, 2) != 0;
    switch (kCases[kind]) {
    case 0:
        return (srgb ? 1u : 0u) + 0x47;   // BC1
    case 1:
        return (srgb ? 1u : 0u) + 0x4A;   // BC2
    case 2:
        return (srgb ? 1u : 0u) + 0x4D;   // BC3
    case 3:
        if (bits == 0x40) {
            return 0x14;   // D32_FLOAT_S8X24_UINT
        }
        if (bits == 0x20) {
            return PixelFormat_HasComponent(pixelFormat, 0x12) != 0 ? 0x2D : 0x28;   // D24_UNORM_S8_UINT / D32_FLOAT
        }
        if (bits == 0x10) {
            return 0x37;   // D16_UNORM
        }
        return 0;
    default:
        return 0;
    }
}

// ---- NiD3D10Renderer::CreationParameters (0x58)

namespace {

// the defaults both constructors set
void SetCreationDefaults(uint8_t* self, uint8_t windowed) {
    At<uint32_t>(self, 0) = 0;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0xC) = 0;
    At<uint8_t>(self, 0x10) = windowed;
    At<uint8_t>(self, 0x11) = 1;
    At<uint8_t>(self, 0x12) = windowed;
    At<uint32_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0x280;   // the swap chain's DXGI_SWAP_CHAIN_DESC: 640 x 480, 60 Hz,
    At<uint32_t>(self, 0x20) = 0x1E0;   //  R8G8B8A8_UNORM, one sample, render target output, one
    At<uint32_t>(self, 0x24) = 0x3C;    //  buffer, windowed, discard
    At<uint32_t>(self, 0x28) = 1;
    At<uint32_t>(self, 0x2C) = 0x1C;
    At<uint32_t>(self, 0x30) = 1;
    At<uint32_t>(self, 0x34) = 0;
    At<uint32_t>(self, 0x38) = 1;
    At<uint32_t>(self, 0x3C) = 0;
    At<uint32_t>(self, 0x40) = 0x60;
    At<uint32_t>(self, 0x44) = 1;
    At<uint32_t>(self, 0x48) = 0;
    At<uint32_t>(self, 0x4C) = 0;
    At<uint32_t>(self, 0x50) = 0;
    At<uint32_t>(self, 0x54) = 2;
}

} // namespace

// 0x8A8E60 (thiscall): NiD3D10Renderer::CreationParameters::CreationParameters
uint8_t* __fastcall NiD3D10CreationParameters_Construct(uint8_t* self, re::Edx) {
    SetCreationDefaults(self, 0);
    return self;
}

// 0x8A8ED0 (thiscall): NiD3D10Renderer::CreationParameters::CreationParameters(window) (the
// window's client size)
uint8_t* __fastcall NiD3D10CreationParameters_ConstructWindow(uint8_t* self, re::Edx, void* window) {
    SetCreationDefaults(self, 1);
    if (window != nullptr) {
        At<void*>(self, 0x48) = window;
        At<uint32_t>(self, 0x4C) = 1;
        int32_t rect[4];
        Import<int>(kGetClientRect, window, rect);
        At<uint32_t>(self, 0x1C) = uint32_t(rect[2] - rect[0]);
        At<uint32_t>(self, 0x20) = uint32_t(rect[3] - rect[1]);
    }
    return self;
}

#define C42C_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42C_REVERSED("NiD3D10AdapterDesc::GetHWDevice", 0x008A78A0, NiD3D10AdapterDesc_GetHWDevice);
C42C_REVERSED("NiD3D10AdapterDesc::GetRefDevice", 0x008A78B0, NiD3D10AdapterDesc_GetRefDevice);
C42C_REVERSED("NiD3D10DeviceDesc::GetMaxMultisampleCount", 0x008A78C0, NiD3D10DeviceDesc_GetMaxMultisampleCount);
C42C_REVERSED("NiD3D10DeviceDesc::GetMultisampleQualityLevels", 0x008A78D0,
              NiD3D10DeviceDesc_GetMultisampleQualityLevels);
C42C_REVERSED("NiD3D10OutputDesc::EnumerateDisplayModes", 0x008A7910, NiD3D10OutputDesc_EnumerateDisplayModes);
C42C_REVERSED("NiD3D10SystemDesc::GetAdapterCount", 0x008A7A50, NiD3D10SystemDesc_GetAdapterCount);
C42C_REVERSED("NiD3D10SystemDesc::GetAdapterDesc", 0x008A7A60, NiD3D10SystemDesc_GetAdapterDesc);
C42C_REVERSED("NiD3D10SystemDesc::ReleaseDXGI", 0x008A7A80, NiD3D10SystemDesc_ReleaseDXGI);
C42C_REVERSED("NiD3D10AdapterDesc::GetOutputCount", 0x008A7AE0, NiD3D10AdapterDesc_GetOutputCount);
C42C_REVERSED("NiD3D10AdapterDesc::GetOutputDesc", 0x008A7AF0, NiD3D10AdapterDesc_GetOutputDesc);
C42C_REVERSED("NiD3D10AdapterDesc::EnumerateDevices", 0x008A7B10, NiD3D10AdapterDesc_EnumerateDevices);
C42C_REVERSED("NiD3D10SystemDesc::LoadDXGI", 0x008A7C60, NiD3D10SystemDesc_LoadDXGI);
C42C_REVERSED("NiD3D10AdapterDesc::~NiD3D10AdapterDesc", 0x008A7D40, NiD3D10AdapterDesc_Destruct);
C42C_REVERSED("NiD3D10SystemDesc::CreateFactory", 0x008A7DF0, NiD3D10SystemDesc_CreateFactory);
C42C_REVERSED("NiD3D10SystemDesc::ReleaseAdapters", 0x008A7EB0, NiD3D10SystemDesc_ReleaseAdapters);
C42C_REVERSED("NiD3D10AdapterDesc::EnumerateOutputs", 0x008A7F30, NiD3D10AdapterDesc_EnumerateOutputs);
C42C_REVERSED("NiD3D10SystemDesc::~NiD3D10SystemDesc", 0x008A80A0, NiD3D10SystemDesc_Destruct);
C42C_REVERSED("NiD3D10SystemDesc::EnumerateAdapters", 0x008A8100, NiD3D10SystemDesc_EnumerateAdapters);
C42C_REVERSED("NiD3D10SystemDesc::NiD3D10SystemDesc", 0x008A82B0, NiD3D10SystemDesc_Construct);
C42C_REVERSED("NiD3D10SystemDesc::DeletingDestructor", 0x008A8330, NiD3D10SystemDesc_DeletingDestructor);
C42C_REVERSED("NiD3D10SystemDesc::Enumerate", 0x008A8350, NiD3D10SystemDesc_Enumerate);
C42C_REVERSED("NiD3D10Renderer::GetSystemDesc", 0x008A8370, NiD3D10Renderer_GetSystemDesc);
C42C_REVERSED("NiD3D10PixelFormat::GetPixelFormat", 0x008A8450, NiD3D10PixelFormat_GetPixelFormat);
C42C_REVERSED("NiD3D10PixelFormat::GetBitsPerPixel", 0x008A87E0, NiD3D10PixelFormat_GetBitsPerPixel);
C42C_REVERSED("NiD3D10Renderer::GetFormatName", 0x008A88B0, NiD3D10Renderer_GetFormatName);
C42C_REVERSED("NiD3D10PixelFormat::DetermineDXGIFormat", 0x008A8D80, NiD3D10PixelFormat_DetermineDXGIFormat);
C42C_REVERSED("NiD3D10Renderer::CreationParameters::CreationParameters", 0x008A8E60,
              NiD3D10CreationParameters_Construct);
C42C_REVERSED("NiD3D10Renderer::CreationParameters::CreationParameters_Window", 0x008A8ED0,
              NiD3D10CreationParameters_ConstructWindow);

} // namespace ni::c42
