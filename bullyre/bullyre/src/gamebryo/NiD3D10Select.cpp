// NiApplication's Direct3D 10 device selection (Gamebryo's NiD3D10Select, statics only): the
// 16-control dialog template (mode, adapter, display, resolution, format, multisample count and
// quality, OK and CANCEL), its procedure, the lists it fills from the D3D10 renderer's system
// description (resolutions kept once each, at their lowest refresh rate), and the renderer's
// creation from the choices or the command line's. The game never asks for D3D10 ("-d3d10"), so
// this is cold code; written exactly all the same.
//
// The dialog's state is the statics the original keeps (0x20E59A4.., 0xBC8F5C..).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::app {
namespace {

using sys::At;
using sys::Import;

// the dialog's statics
GAME_VAR(uint8_t, ms_bDialogOpen, 0x020E59A4);
GAME_VAR(uint8_t, ms_bFullscreen, 0x020E59A5);
GAME_VAR(uint8_t, ms_bRefRast, 0x020E59A6);
GAME_VAR(uint32_t, ms_uiQuality, 0x020E59A8);
GAME_VAR(uint8_t*, ms_pkAdapter, 0x020E59AC);    // NiD3D10AdapterDesc
GAME_VAR(uint8_t*, ms_pkDevice, 0x020E59B0);     // NiD3D10DeviceDesc
GAME_VAR(uint8_t*, ms_pkOutput, 0x020E59B4);     // NiD3D10OutputDesc
GAME_VAR(uint32_t*, ms_pkMode, 0x020E59B8);      // DXGI_MODE_DESC (0x1C)
GAME_VAR(uint32_t, ms_uiFormat, 0x020E59BC);     // DXGI_FORMAT
GAME_VAR(uint32_t, ms_uiDefaultWidth, 0x00BC8F5C);
GAME_VAR(uint32_t, ms_uiDefaultHeight, 0x00BC8F60);
GAME_VAR(uint32_t, ms_uiMultisampleCount, 0x00BC8F64);   // (1)

// Win32 through the game's import slots
constexpr uintptr_t kMultiByteToWideChar = 0x008FC074;
constexpr uintptr_t klstrlenA = 0x008FC0EC;
constexpr uintptr_t kWideCharToMultiByte = 0x008FC0F0;
constexpr uintptr_t kMoveWindow = 0x008FC220;
constexpr uintptr_t kGetWindowRect = 0x008FC264;
constexpr uintptr_t kSetWindowTextA = 0x008FC270;
constexpr uintptr_t kSendDlgItemMessageA = 0x008FC284;
constexpr uintptr_t kGetWindowLongA = 0x008FC290;
constexpr uintptr_t kDialogBoxIndirectParamA = 0x008FC294;
constexpr uintptr_t kEndDialog = 0x008FC298;

constexpr uint32_t CB_ADDSTRING = 0x143;
constexpr uint32_t CB_RESETCONTENT = 0x14B;
constexpr uint32_t CB_SETCURSEL = 0x14E;
constexpr uint32_t CB_GETCURSEL = 0x147;
constexpr uint32_t CB_GETITEMDATA = 0x150;
constexpr uint32_t CB_SETITEMDATA = 0x151;
// the controls
constexpr int kMode = 1000;
constexpr int kAdapter = 1001;
constexpr int kDisplay = 1002;
constexpr int kResolution = 1003;
constexpr int kFormat = 1004;
constexpr int kCount = 1005;
constexpr int kQuality = 1006;

constexpr uintptr_t NiTPrimitiveArray_vtable = 0x00961988;
constexpr uint32_t DXGI_FORMAT_R8G8B8A8_UNORM = 0x1C;

struct RECT {
    int32_t left, top, right, bottom;
};

// NiTPrimitiveArray<void*> as the lists build one on the stack
struct LocalArray {
    re::Ptr32<void> vtable;
    re::Ptr32<re::Ptr32<void>> base;
    uint16_t maxSize;
    uint16_t size;
    uint16_t esize;
    uint16_t growBy;
};
static_assert(sizeof(LocalArray) == 0x10);

// NiD3D10Renderer::CreationParameters (0x58: the adapter and output indices, the reference device,
// single-threaded, ..., the fullscreen mode at +0x1C, the sample count and quality at +0x38)
struct CreationParameters {
    uint8_t bytes[0x58];
};

GAME_FN(void*, __cdecl, NiMalloc_, 0x00752300, uint32_t size);
GAME_FN(void*, __cdecl, GameMalloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, GameFree, 0x005EEAB0, void* p);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, Array_SetSize, 0x0084A110, LocalArray* self, uint32_t size);
GAME_FN(void, __thiscall, Array_SetAt, 0x00833C70, LocalArray* self, uint32_t index, const re::Ptr32<void>* value);
// the D3D10 renderer's descriptions
GAME_FN(void, __cdecl, D3D10Renderer_GetSystemDesc, 0x008A8370, re::Ptr32<NiObject>* result);
GAME_FN(NiObject*, __cdecl, D3D10Renderer_Create, 0x008AF800, CreationParameters* params, re::Ptr32<NiObject>* result);
GAME_FN(uint8_t, __thiscall, SystemDesc_IsEnumerationValid, 0x008A7890, NiObject* self);
GAME_FN(void, __thiscall, SystemDesc_Enumerate, 0x008A8350, NiObject* self);
GAME_FN(uint32_t, __thiscall, SystemDesc_GetAdapterCount, 0x008A7A50, NiObject* self);
GAME_FN(uint8_t*, __thiscall, SystemDesc_GetAdapterDesc, 0x008A7A60, NiObject* self, uint32_t index);
GAME_FN(uint8_t*, __thiscall, AdapterDesc_GetHWDevice, 0x008A78A0, uint8_t* self);
GAME_FN(uint8_t*, __thiscall, AdapterDesc_GetRefDevice, 0x008A78B0, uint8_t* self);
GAME_FN(const uint16_t*, __thiscall, AdapterDesc_GetDesc, 0x0077C280, uint8_t* self);   // (its description)
GAME_FN(uint32_t, __thiscall, AdapterDesc_GetOutputCount, 0x008A7AE0, uint8_t* self);
GAME_FN(uint8_t*, __thiscall, AdapterDesc_GetOutputDesc, 0x008A7AF0, uint8_t* self, uint32_t index);
GAME_FN(const uint16_t*, __thiscall, OutputDesc_GetName, 0x008BAC30, uint8_t* self);
GAME_FN(uint32_t, __thiscall, OutputDesc_GetDisplayModeCount, 0x008963B0, uint8_t* self);
GAME_FN(uint32_t*, __thiscall, OutputDesc_GetDisplayModes, 0x008A7900, uint8_t* self);
GAME_FN(uint32_t, __thiscall, Desc_GetIndex, 0x00773DB0, uint8_t* self);
GAME_FN(uint32_t, __thiscall, DeviceDesc_GetMaxMultisampleCount, 0x008A78C0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, DeviceDesc_GetMultisampleQualityLevels, 0x008A78D0, uint8_t* self, uint32_t count,
        uint32_t* levels);
GAME_FN(const char*, __cdecl, D3D10_GetFormatName, 0x008A88B0, uint32_t format, int unknown);
GAME_FN(void, __thiscall, CreationParameters_Construct, 0x008A8ED0, CreationParameters* self, void* window);
GAME_FN(void, __thiscall, CreationParameters_ConstructDefault, 0x008A8E60, CreationParameters* self);
// the DX9 file's control writer
GAME_FN(void, __cdecl, AddDialogControl, 0x0082D990, re::Ptr32<uint8_t>* cursor, uint32_t style, int16_t x, int16_t y,
        int16_t cx, int16_t cy, uint16_t id, const char* className, const char* text);
// (this file's, called where modes apply)
GAME_FN(void, __cdecl, FillDisplays, 0x0082D610, void* dialog);
GAME_FN(uint8_t, __cdecl, ReadAdapter, 0x0082D730, void* dialog);
GAME_FN(uint8_t, __cdecl, ReadDisplay, 0x0082D7B0, void* dialog);
GAME_FN(uint8_t, __cdecl, ReadResolution, 0x0082D810, void* dialog);
GAME_FN(uint8_t, __cdecl, ReadFormat, 0x0082D870, void* dialog);
GAME_FN(uint8_t, __cdecl, ReadCount, 0x0082D8D0, void* dialog);
GAME_FN(uint8_t, __cdecl, ReadQuality, 0x0082D930, void* dialog);
GAME_FN(void*, __cdecl, BuildDialogTemplate, 0x0082DA40);
GAME_FN(void, __cdecl, FillAdapters, 0x0082DD90, void* dialog);
GAME_FN(void, __cdecl, FillResolutions, 0x0082DF40, void* dialog);
GAME_FN(void, __cdecl, FillFormats, 0x0082E2A0, void* dialog);
GAME_FN(void, __cdecl, FillCounts, 0x0082E520, void* dialog);
GAME_FN(void, __cdecl, FillQualities, 0x0082E670, void* dialog);
GAME_FN(uint32_t, __cdecl, Select, 0x0082EC10, void* window, uint32_t width, uint32_t height, int refRast,
        uint8_t* fullscreen, re::Ptr32<uint8_t>* adapter, re::Ptr32<uint8_t>* device, re::Ptr32<uint8_t>* output, re::Ptr32<uint32_t>* mode, uint32_t* format,
        uint32_t* count, uint32_t* quality);
GAME_FN(re::Ptr32<NiObject>*, __cdecl, SelectRenderer, 0x0082ED00, re::Ptr32<NiObject>* result, void* window, uint32_t width,
        uint32_t height, int refRast, int singleThreaded, uint8_t* fullscreen);

inline intptr_t Send(void* dialog, int control, uint32_t message, uintptr_t wParam, intptr_t lParam) {
    return Import<intptr_t>(kSendDlgItemMessageA, dialog, control, message, wParam, lParam);
}

inline void Unref(NiObject* object) {
    if (object != nullptr && sys::Decrement(&object->refCount) == 0) {
        sys::Virtual<void>(object, 1);
    }
}

inline int WriteWide(uint16_t* to, const char* text) {
    int length = Import<int>(klstrlenA, text);
    int written = 0;
    if (length != 0) {
        written = Import<int>(kMultiByteToWideChar, 0u, 0u, text, length, to, length);
    }
    to[written] = 0;
    return written;
}

inline uint8_t* DeviceOf(uint8_t* adapter) {
    return ms_bRefRast ? AdapterDesc_GetRefDevice(adapter) : AdapterDesc_GetHWDevice(adapter);
}

// NiTArray::SetAt as the lists inline it on their local array
void LocalSetAt(LocalArray& array, uint32_t index, void* value) {
    if (index >= array.maxSize) {
        Array_SetSize(&array, array.growBy + index);
    }
    re::Ptr32<void>* base = array.base;
    if (index < array.size) {
        if (value == nullptr) {
            if (base[index] != nullptr) {
                array.esize--;
            }
        } else if (base[index] == nullptr) {
            array.esize++;
        }
    } else {
        array.size = uint16_t(index + 1);
        if (value != nullptr) {
            array.esize++;
        }
    }
    base[index] = value;
}

// ---- the selections read back (true when changed, or nothing is chosen)

// 0x82D730: the adapter (and its device: the reference one when asked)
uint8_t __cdecl ReadAdapterImpl(void* dialog) {
    intptr_t index = Send(dialog, kAdapter, CB_GETCURSEL, 0, 0);
    if (index < 0) {
        ms_pkDevice = nullptr;
        return 1;
    }
    auto* adapter = reinterpret_cast<uint8_t*>(Send(dialog, kAdapter, CB_GETITEMDATA, uintptr_t(index), 0));
    uint8_t changed = adapter != ms_pkAdapter ? 1 : 0;
    ms_pkAdapter = adapter;
    ms_pkDevice = DeviceOf(adapter);
    return changed;
}

// the other readers: one control's item data into one static (the multisample count's 1 when
// nothing is chosen)
template <typename T>
uint8_t ReadInto(void* dialog, int control, re::Field<T>& value, T none) {
    intptr_t index = Send(dialog, control, CB_GETCURSEL, 0, 0);
    if (index < 0) {
        value = none;
        return 1;
    }
    auto data = T(Send(dialog, control, CB_GETITEMDATA, uintptr_t(index), 0));
    uint8_t changed = data != value ? 1 : 0;
    value = data;
    return changed;
}

// 0x82D7B0
uint8_t __cdecl ReadDisplayImpl(void* dialog) {
    return ReadInto<uint8_t*>(dialog, kDisplay, ms_pkOutput, nullptr);
}

// 0x82D810
uint8_t __cdecl ReadResolutionImpl(void* dialog) {
    return ReadInto<uint32_t*>(dialog, kResolution, ms_pkMode, nullptr);
}

// 0x82D870
uint8_t __cdecl ReadFormatImpl(void* dialog) {
    return ReadInto<uint32_t>(dialog, kFormat, ms_uiFormat, 0);
}

// 0x82D8D0
uint8_t __cdecl ReadCountImpl(void* dialog) {
    return ReadInto<uint32_t>(dialog, kCount, ms_uiMultisampleCount, 1);
}

// 0x82D930
uint8_t __cdecl ReadQualityImpl(void* dialog) {
    return ReadInto<uint32_t>(dialog, kQuality, ms_uiQuality, 0);
}

// ---- the template

// 0x82DA40
void* __cdecl BuildDialogTemplateImpl() {
    auto* memory = static_cast<uint8_t*>(GameMalloc(900));
    if (memory == nullptr) {
        return nullptr;
    }
    crt::Memset(memory, 0, 900);
    At<uint32_t>(memory, 0) = 0x90C80BC4;
    At<uint32_t>(memory, 4) = 0;
    At<uint16_t>(memory, 8) = 0x10;
    At<uint16_t>(memory, 0xA) = 0;
    At<uint16_t>(memory, 0xC) = 0;
    At<uint16_t>(memory, 0xE) = 0xE0;
    At<uint16_t>(memory, 0x10) = 0xA0;
    auto* p = reinterpret_cast<uint16_t*>(memory + 0x12);
    *p++ = 0;
    *p++ = 0;
    p += WriteWide(p, "Select Desired Direct3D 10 Device") + 1;
    *p++ = 8;
    p += WriteWide(p, "Arial") + 1;
    re::Ptr32<uint8_t> cursor = reinterpret_cast<uint8_t*>(p);
    AddDialogControl(&cursor, 0x210003, 0x36, 9, 0xA0, 0x2D, kMode, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 0xB, 0xB, 0x26, 0xD, 0xFFFF, "STATIC", "Mode:");
    AddDialogControl(&cursor, 0x210003, 0x36, 0x1F, 0xA0, 0x2D, kAdapter, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 0xB, 0x21, 0x26, 0xD, 0xFFFF, "STATIC", "Adapter:");
    AddDialogControl(&cursor, 0x210003, 0x36, 0x34, 0xA0, 0x2D, kDisplay, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 0xB, 0x36, 0x26, 0xD, 0xFFFF, "STATIC", "Display:");
    AddDialogControl(&cursor, 0x210003, 0x36, 0x49, 0xA0, 0x2D, kResolution, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 0xB, 0x4B, 0x26, 0xD, 0xFFFF, "STATIC", "Resolution:");
    AddDialogControl(&cursor, 0x210003, 0x36, 0x5F, 0xA0, 0x2D, kFormat, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 0xB, 0x61, 0x26, 0xD, 0xFFFF, "STATIC", "Format:");
    AddDialogControl(&cursor, 0x210003, 0x55, 0x74, 0x25, 0x2D, kCount, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 0xB, 0x76, 0x4A, 0xD, 0xFFFF, "STATIC", "MultiSample    Count:");
    AddDialogControl(&cursor, 0x210003, 0xA5, 0x74, 0x25, 0x2D, kQuality, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 0x86, 0x76, 0x1E, 0xD, 0xFFFF, "STATIC", "Quality:");
    AddDialogControl(&cursor, 0x10000, 0x82, 0x89, 0x33, 0xE, 2, "BUTTON", "CANCEL");
    AddDialogControl(&cursor, 0x10000, 0x2A, 0x89, 0x33, 0xE, 1, "BUTTON", "OK");
    return memory;
}

// ---- the lists

// 0x82D610: the adapter's displays (their names; the buffer is left as it was for one without a
// name), the first chosen
void __cdecl FillDisplaysImpl(void* dialog) {
    Send(dialog, kDisplay, CB_RESETCONTENT, 0, 0);
    uint32_t count = AdapterDesc_GetOutputCount(ms_pkAdapter);
    bool any = false;
    char name[0x20];   // (uninitialised until a display has a name, as the game's)
    if (count != 0) {
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* output = AdapterDesc_GetOutputDesc(ms_pkAdapter, i);
            const uint16_t* wide = OutputDesc_GetName(output);
            if (wide != nullptr) {
                Import<int>(kWideCharToMultiByte, 0u, 0u, wide, -1, name, 0x20, static_cast<void*>(nullptr),
                            static_cast<void*>(nullptr));
                name[0x1F] = 0;
            }
            intptr_t index = Send(dialog, kDisplay, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(name));
            Send(dialog, kDisplay, CB_SETITEMDATA, uintptr_t(index), reinterpret_cast<intptr_t>(output));
        }
        any = true;
    }
    Send(dialog, kDisplay, CB_SETCURSEL, 0, 0);
    if (any) {
        ms_pkOutput = reinterpret_cast<uint8_t*>(Send(dialog, kDisplay, CB_GETITEMDATA, 0, 0));
        return;
    }
    ms_pkOutput = nullptr;
}

// 0x82DD90: the adapters with a device ("name   [index]"), the first chosen
void __cdecl FillAdaptersImpl(void* dialog) {
    Send(dialog, kAdapter, CB_RESETCONTENT, 0, 0);
    re::Ptr32<NiObject> systemDesc = nullptr;   // (a NiPointer)
    D3D10Renderer_GetSystemDesc(&systemDesc);
    uint32_t count = SystemDesc_GetAdapterCount(systemDesc);
    bool any = false;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* adapter = SystemDesc_GetAdapterDesc(systemDesc, i);
        if (DeviceOf(adapter) == nullptr) {
            continue;
        }
        char name[0x80];
        const uint16_t* wide = AdapterDesc_GetDesc(adapter);
        if (wide != nullptr) {
            Import<int>(kWideCharToMultiByte, 0u, 0u, wide, -1, name, 0x80, static_cast<void*>(nullptr),
                        static_cast<void*>(nullptr));
            name[0x7F] = 0;
        }
        char text[0x8C];
        NiSprintf(text, 0x8C, "%s   [%1d]", name, i);
        intptr_t index = Send(dialog, kAdapter, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
        Send(dialog, kAdapter, CB_SETITEMDATA, uintptr_t(index), reinterpret_cast<intptr_t>(adapter));
        any = true;
    }
    Send(dialog, kAdapter, CB_SETCURSEL, 0, 0);
    if (any) {
        ms_pkAdapter = reinterpret_cast<uint8_t*>(Send(dialog, kAdapter, CB_GETITEMDATA, 0, 0));
        ms_pkDevice = DeviceOf(ms_pkAdapter);
    } else {
        ms_pkAdapter = nullptr;
        ms_pkDevice = nullptr;
    }
    Unref(systemDesc);
}

// a DXGI_MODE_DESC's refresh rate
inline float RefreshRate(const uint32_t* mode) {
    return float(double(mode[2])) / float(double(mode[3]));
}

// 0x82DF40: the display's resolutions (fullscreen only), each size once (the lowest refresh rate
// kept), 640 x 480 and up; the default size chosen
void __cdecl FillResolutionsImpl(void* dialog) {
    Send(dialog, kResolution, CB_RESETCONTENT, 0, 0);
    if (!ms_bFullscreen) {
        return;
    }
    uint32_t modeCount = OutputDesc_GetDisplayModeCount(ms_pkOutput);
    uint32_t* modes = OutputDesc_GetDisplayModes(ms_pkOutput);
    bool any = false;
    intptr_t selection = 0;
    LocalArray array;
    array.maxSize = 0x18;
    array.growBy = 4;
    array.size = 0;
    array.esize = 0;
    array.base = static_cast<re::Ptr32<void>*>(NiMalloc_(0x60));
    array.vtable = reinterpret_cast<void*>(NiTPrimitiveArray_vtable);
    re::Ptr32<void> first = modes;
    Array_SetAt(&array, 0, &first);
    uint32_t unique = 1;
    if (modeCount > 1) {
        uint32_t* mode = modes + 7;
        for (uint32_t left = modeCount - 1; left != 0; left--, mode += 7) {
            uint32_t found = unique;
            for (uint32_t i = 0; i < unique; i++) {
                auto* entry = static_cast<uint32_t*>(array.base[i]);
                if (entry[0] == mode[0] && entry[1] == mode[1]) {
                    found = i;
                    break;
                }
            }
            if (found != unique) {
                float rate = RefreshRate(mode);
                float existing = RefreshRate(static_cast<uint32_t*>(array.base[found]));
                if (rate < existing) {
                    LocalSetAt(array, found, mode);
                }
                continue;
            }
            LocalSetAt(array, unique, mode);
            unique++;
        }
    }
    for (uint32_t j = 0; j < unique; j++) {
        auto* entry = static_cast<uint32_t*>(array.base[j]);
        uint32_t width = entry[0];
        uint32_t height = entry[1];
        if (width < 0x280 || height < 0x1E0) {
            continue;
        }
        char text[0x14];
        NiSprintf(text, 0x14, "%u x %u", width, height);
        intptr_t index = Send(dialog, kResolution, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
        Send(dialog, kResolution, CB_SETITEMDATA, uintptr_t(index), re::PtrCast<intptr_t>(array.base[j]));
        if (width == ms_uiDefaultWidth && height == ms_uiDefaultHeight) {
            selection = index;
        }
        any = true;
    }
    Send(dialog, kResolution, CB_SETCURSEL, uintptr_t(selection), 0);
    if (any) {
        ms_pkMode = reinterpret_cast<uint32_t*>(Send(dialog, kResolution, CB_GETITEMDATA, uintptr_t(selection), 0));
    } else {
        ms_pkMode = nullptr;
    }
    NiFree(array.base);
}

// 0x82E2A0: the formats the chosen size comes in (fullscreen only), R8G8B8A8 chosen (and its mode
// made the resolution's)
void __cdecl FillFormatsImpl(void* dialog) {
    Send(dialog, kFormat, CB_RESETCONTENT, 0, 0);
    if (!ms_bFullscreen) {
        return;
    }
    uint32_t modeCount = OutputDesc_GetDisplayModeCount(ms_pkOutput);
    uint32_t* mode = OutputDesc_GetDisplayModes(ms_pkOutput);
    uint32_t* chosenMode = ms_pkMode;
    bool any = false;
    intptr_t selection = 0;
    LocalArray array;
    array.maxSize = 4;
    array.growBy = 4;
    array.size = 0;
    array.esize = 0;
    array.base = static_cast<re::Ptr32<void>*>(NiMalloc_(0x10));
    array.vtable = reinterpret_cast<void*>(NiTPrimitiveArray_vtable);
    uint32_t unique = 0;
    for (uint32_t left = modeCount; left != 0; left--, mode += 7) {
        if (ms_pkMode[0] != mode[0] || ms_pkMode[1] != mode[1]) {
            continue;
        }
        uint32_t found = unique;
        for (uint32_t i = 0; i < unique; i++) {
            if (static_cast<uint32_t*>(array.base[i])[4] == mode[4]) {
                found = i;
                break;
            }
        }
        if (found != unique) {
            continue;
        }
        LocalSetAt(array, unique, mode);
        unique++;
    }
    for (uint32_t j = 0; j < unique; j++) {
        any = true;
        uint32_t format = static_cast<uint32_t*>(array.base[j])[4];
        char text[0x8C];
        NiSprintf(text, 0x8C, "%s", D3D10_GetFormatName(format, 0));
        intptr_t index = Send(dialog, kFormat, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
        Send(dialog, kFormat, CB_SETITEMDATA, uintptr_t(index), intptr_t(format));
        if (format == DXGI_FORMAT_R8G8B8A8_UNORM) {
            selection = index;
            chosenMode = static_cast<uint32_t*>(array.base[j]);
        }
    }
    Send(dialog, kFormat, CB_SETCURSEL, uintptr_t(selection), 0);
    if (any) {
        ms_uiFormat = uint32_t(Send(dialog, kFormat, CB_GETITEMDATA, uintptr_t(selection), 0));
        ms_pkMode = chosenMode;
        intptr_t resolution = Send(dialog, kResolution, CB_GETCURSEL, 0, 0);
        if (resolution >= 0) {
            Send(dialog, kResolution, CB_SETITEMDATA, uintptr_t(resolution), reinterpret_cast<intptr_t>(chosenMode));
        }
    } else {
        ms_uiFormat = 0;
    }
    NiFree(array.base);
}

// 0x82E520: the multisample counts the device has quality levels for (1 always listed first and
// chosen)
void __cdecl FillCountsImpl(void* dialog) {
    Send(dialog, kCount, CB_RESETCONTENT, 0, 0);
    if (ms_pkDevice == nullptr) {
        return;
    }
    uint32_t maxCount = DeviceDesc_GetMaxMultisampleCount(ms_pkDevice);
    bool any = false;
    char text[6];
    NiSprintf(text, 6, "%d", 1);
    intptr_t first = Send(dialog, kCount, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
    Send(dialog, kCount, CB_SETITEMDATA, uintptr_t(first), 1);
    for (uint32_t count = 2; count < maxCount; count++) {
        uint32_t levels = 0;
        if (!DeviceDesc_GetMultisampleQualityLevels(ms_pkDevice, count, &levels)) {
            continue;
        }
        NiSprintf(text, 6, "%d", count);
        intptr_t index = Send(dialog, kCount, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
        Send(dialog, kCount, CB_SETITEMDATA, uintptr_t(index), intptr_t(count));
        any = true;
    }
    Send(dialog, kCount, CB_SETCURSEL, uintptr_t(first), 0);
    if (any) {
        ms_uiMultisampleCount = uint32_t(Send(dialog, kCount, CB_GETITEMDATA, uintptr_t(first), 0));
        return;
    }
    ms_uiMultisampleCount = 1;
}

// 0x82E670: the count's quality levels (0 always listed and chosen)
void __cdecl FillQualitiesImpl(void* dialog) {
    Send(dialog, kQuality, CB_RESETCONTENT, 0, 0);
    if (ms_pkDevice == nullptr) {
        return;
    }
    bool any = false;
    char text[6];
    NiSprintf(text, 6, "%d", 0);
    intptr_t index = Send(dialog, kQuality, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
    Send(dialog, kQuality, CB_SETITEMDATA, uintptr_t(index), 0);
    uint32_t levels = 0;
    if (DeviceDesc_GetMultisampleQualityLevels(ms_pkDevice, ms_uiMultisampleCount, &levels) && levels > 1) {
        any = true;
        for (uint32_t i = 1; i < levels; i++) {
            NiSprintf(text, 6, "%d", i);
            index = Send(dialog, kQuality, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
            Send(dialog, kQuality, CB_SETITEMDATA, uintptr_t(index), intptr_t(i));
        }
    }
    Send(dialog, kQuality, CB_SETCURSEL, 0, 0);
    if (any) {
        ms_uiQuality = uint32_t(Send(dialog, kQuality, CB_GETITEMDATA, 0, 0));
        return;
    }
    ms_uiQuality = 0;
}

void ResetAll(void* dialog) {
    for (int control = kMode; control <= kQuality; control++) {
        Send(dialog, control, CB_RESETCONTENT, 0, 0);
    }
}

// 0x82E7B0: the dialog procedure (each choice refills the lists after it)
intptr_t __stdcall DialogProc(void* dialog, uint32_t message, uintptr_t wParam, intptr_t) {
    if (message == 0x110) {
        Import<int>(kSetWindowTextA, dialog, "Select Direct3D 10 Display Device");
        Send(dialog, kMode, CB_RESETCONTENT, 0, 0);
        intptr_t index = Send(dialog, kMode, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>("Windowed"));
        Send(dialog, kMode, CB_SETITEMDATA, uintptr_t(index), 0);
        Send(dialog, kMode, CB_SETCURSEL, 0, 0);
        ms_bFullscreen = 0;
        index = Send(dialog, kMode, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>("Fullscreen"));
        Send(dialog, kMode, CB_SETITEMDATA, uintptr_t(index), 1);
        ms_bDialogOpen = 1;
        FillAdapters(dialog);
        FillDisplays(dialog);
        FillResolutions(dialog);
        FillFormats(dialog);
        FillCounts(dialog);
        FillQualities(dialog);
        return 1;
    }
    if (message != 0x111) {
        return 0;
    }
    uint16_t id = uint16_t(wParam);
    switch (id) {
    case 1:
        ReadAdapter(dialog);
        ReadDisplay(dialog);
        ReadResolution(dialog);
        ReadFormat(dialog);
        ReadCount(dialog);
        ReadQuality(dialog);
        ms_bDialogOpen = 0;
        ResetAll(dialog);
        Import<int>(kEndDialog, dialog, intptr_t(1));
        return 1;
    case 2:
        ms_bDialogOpen = 0;
        ResetAll(dialog);
        Import<int>(kEndDialog, dialog, intptr_t(0));
        return 1;
    case kMode:
        if (ms_bDialogOpen) {
            intptr_t index = Send(dialog, kMode, CB_GETCURSEL, 0, 0);
            uint8_t old = ms_bFullscreen;
            if (index >= 0) {
                ms_bFullscreen = Send(dialog, kMode, CB_GETITEMDATA, uintptr_t(index), 0) != 0 ? 1 : 0;
                if (old != ms_bFullscreen) {
                    FillAdapters(dialog);
                    FillDisplays(dialog);
                    FillResolutions(dialog);
                    FillFormats(dialog);
                    FillCounts(dialog);
                    FillQualities(dialog);
                }
            }
        }
        return 1;
    case kAdapter:
        if (ms_bDialogOpen && ReadAdapter(dialog)) {
            FillDisplays(dialog);
            FillResolutions(dialog);
            FillFormats(dialog);
            FillCounts(dialog);
            FillQualities(dialog);
        }
        return 1;
    case kDisplay:
        if (ms_bDialogOpen && ReadDisplay(dialog)) {
            FillResolutions(dialog);
            FillFormats(dialog);
            FillCounts(dialog);
            FillQualities(dialog);
        }
        return 1;
    case kResolution:
        if (ms_bDialogOpen && ReadResolution(dialog)) {
            FillFormats(dialog);
            FillCounts(dialog);
            FillQualities(dialog);
        }
        return 1;
    case kFormat:
        if (ms_bDialogOpen && ReadFormat(dialog)) {
            FillCounts(dialog);
            FillQualities(dialog);
        }
        return 1;
    case kCount:
        if (ms_bDialogOpen && ReadCount(dialog)) {
            FillQualities(dialog);
        }
        return 1;
    default:
        return 0;
    }
}

// 0x82EC10: Select(window, default width, default height, reference device, fullscreen&,
// adapter&, device&, display&, mode&, format&, count&, quality&): 0 with the choices (mode and
// format 0 windowed), 2 when cancelled
uint32_t __cdecl SelectImpl(void* window, uint32_t width, uint32_t height, int refRast, uint8_t* fullscreen,
                            re::Ptr32<uint8_t>* adapter, re::Ptr32<uint8_t>* device, re::Ptr32<uint8_t>* output, re::Ptr32<uint32_t>* mode, uint32_t* format,
                            uint32_t* count, uint32_t* quality) {
    *fullscreen = 0;
    ms_uiDefaultWidth = width;
    ms_uiDefaultHeight = height;
    ms_bRefRast = static_cast<uint8_t>(refRast);
    void* instance = reinterpret_cast<void*>(Import<intptr_t>(kGetWindowLongA, window, -6));
    void* dialogTemplate = BuildDialogTemplate();
    if (dialogTemplate == nullptr) {
        return 2;
    }
    intptr_t result = Import<intptr_t>(kDialogBoxIndirectParamA, instance, dialogTemplate, window,
                                       reinterpret_cast<void*>(0x0082E7B0), intptr_t(0));
    GameFree(dialogTemplate);
    if (result == 0) {
        return 2;
    }
    *fullscreen = ms_bFullscreen;
    *adapter = ms_pkAdapter;
    *device = ms_pkDevice;
    *output = ms_pkOutput;
    if (ms_bFullscreen) {
        *mode = ms_pkMode;
        *format = ms_uiFormat;
    } else {
        *mode = nullptr;
        *format = 0;
    }
    *count = ms_uiMultisampleCount;
    *quality = ms_uiQuality;
    return 0;
}

// 0x82ED00: SelectRenderer(result&, window, width, height, reference device, single threaded,
// fullscreen&): the dialog until a renderer is made from its choices (the window put back after a
// failure); none when cancelled or nothing usable was chosen
re::Ptr32<NiObject>* __cdecl SelectRendererImpl(re::Ptr32<NiObject>* result, void* window, uint32_t width, uint32_t height, int refRast,
                                      int singleThreaded, uint8_t* fullscreen) {
    re::Ptr32<NiObject> renderer = nullptr;   // (a NiPointer)
    RECT rect;
    Import<int>(kGetWindowRect, window, &rect);
    while (renderer == nullptr) {
        re::Ptr32<uint8_t> adapter = nullptr;
        re::Ptr32<uint8_t> device;
        re::Ptr32<uint8_t> output = nullptr;
        re::Ptr32<uint32_t> mode = nullptr;
        uint32_t format;
        uint32_t count = 1;
        uint32_t quality = 0;
        uint32_t chosen = Select(window, width, height, refRast, fullscreen, &adapter, &device, &output, &mode, &format,
                                 &count, &quality);
        if (chosen == 2 || adapter == nullptr || output == nullptr) {
            *result = nullptr;
            Unref(renderer);
            return result;
        }
        CreationParameters params;
        CreationParameters_Construct(&params, window);
        At<uint32_t>(&params, 0xC) = uint32_t(singleThreaded);
        At<uint32_t>(&params, 8) = re::Flag(refRast) ? 1 : 0;
        At<uint32_t>(&params, 0) = Desc_GetIndex(adapter);
        At<uint32_t>(&params, 4) = Desc_GetIndex(output);
        if (*fullscreen) {
            if (mode == nullptr) {
                *result = nullptr;
                Unref(renderer);
                return result;
            }
            crt::Memcpy(&At<uint32_t>(&params, 0x1C), mode, 0x1C);
            At<uint32_t>(&params, 0x4C) = 0;
        }
        At<uint32_t>(&params, 0x38) = count;
        At<uint32_t>(&params, 0x3C) = quality;
        D3D10Renderer_Create(&params, &renderer);
        if (renderer == nullptr) {
            Import<int>(kMoveWindow, window, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 1);
        }
    }
    *result = renderer;
    sys::Increment(&renderer->refCount);
    Unref(renderer);   // (the local pointer's)
    return result;
}

// 0x82EF00: CreateD3D10Renderer(result&, window, dialog, width, height, multi-thread, reference
// device, fullscreen&): none without an enumerated system with a usable device; through the dialog
// when asked, else straight (fullscreen with the default parameters, or in the window). Returns
// the result (a NiPointer)
re::Ptr32<NiObject>* __cdecl CreateD3D10Renderer(re::Ptr32<NiObject>* result, void* window, int dialog, uint32_t width, uint32_t height,
                                       int multiThread, int refRast, uint8_t* fullscreen) {
    re::Ptr32<NiObject> systemDesc = nullptr;   // (a NiPointer)
    D3D10Renderer_GetSystemDesc(&systemDesc);
    if (systemDesc == nullptr) {
        *result = nullptr;
        return result;
    }
    if (!SystemDesc_IsEnumerationValid(systemDesc)) {
        SystemDesc_Enumerate(systemDesc);
        if (!SystemDesc_IsEnumerationValid(systemDesc)) {
            *result = nullptr;
            Unref(systemDesc);
            return result;
        }
    }
    uint32_t count = SystemDesc_GetAdapterCount(systemDesc);
    bool any = false;
    bool failed = count == 0;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* adapter = SystemDesc_GetAdapterDesc(systemDesc, i);
        if (adapter == nullptr) {
            failed = true;   // (even when an earlier one had a device)
            break;
        }
        if (DeviceOf(adapter) != nullptr) {
            any = true;
        }
    }
    if (failed || !any) {
        *result = nullptr;
        Unref(systemDesc);
        return result;
    }
    re::Ptr32<NiObject> renderer = nullptr;   // (a NiPointer)
    uint32_t singleThreaded = re::Flag(multiThread) ? 0 : 1;
    if (re::Flag(dialog)) {
        SelectRenderer(result, window, width, height, refRast, int(singleThreaded), fullscreen);
        Unref(renderer);
        Unref(systemDesc);
        return result;
    }
    CreationParameters params;
    if (*fullscreen) {
        CreationParameters_ConstructDefault(&params);
    } else {
        CreationParameters_Construct(&params, window);
    }
    At<uint32_t>(&params, 8) = re::Flag(refRast) ? 1 : 0;
    At<uint32_t>(&params, 0xC) = singleThreaded;
    D3D10Renderer_Create(&params, &renderer);
    *result = renderer;
    if (renderer != nullptr) {
        sys::Increment(&renderer->refCount);
        Unref(renderer);
    }
    Unref(systemDesc);
    return result;
}

}   // namespace
}   // namespace ni::app

// ---- registration ---------------------------------------------------------------------

#define C33_D3D10SELECT_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_D3D10SELECT_REVERSED("NiD3D10Select::FillDisplays", 0x0082D610, &ni::app::FillDisplaysImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::ReadAdapter", 0x0082D730, &ni::app::ReadAdapterImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::ReadDisplay", 0x0082D7B0, &ni::app::ReadDisplayImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::ReadResolution", 0x0082D810, &ni::app::ReadResolutionImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::ReadFormat", 0x0082D870, &ni::app::ReadFormatImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::ReadCount", 0x0082D8D0, &ni::app::ReadCountImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::ReadQuality", 0x0082D930, &ni::app::ReadQualityImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::BuildDialogTemplate", 0x0082DA40, &ni::app::BuildDialogTemplateImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::FillAdapters", 0x0082DD90, &ni::app::FillAdaptersImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::FillResolutions", 0x0082DF40, &ni::app::FillResolutionsImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::FillFormats", 0x0082E2A0, &ni::app::FillFormatsImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::FillCounts", 0x0082E520, &ni::app::FillCountsImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::FillQualities", 0x0082E670, &ni::app::FillQualitiesImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::DialogProc", 0x0082E7B0, &ni::app::DialogProc);
C33_D3D10SELECT_REVERSED("NiD3D10Select::Select", 0x0082EC10, &ni::app::SelectImpl);
C33_D3D10SELECT_REVERSED("NiD3D10Select::SelectRenderer", 0x0082ED00, &ni::app::SelectRendererImpl);
C33_D3D10SELECT_REVERSED("NiApplication::CreateD3D10Renderer", 0x0082EF00, &ni::app::CreateD3D10Renderer);
