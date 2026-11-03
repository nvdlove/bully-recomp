// NiApplication's DirectX 9 device selection (Gamebryo's NiDX9Select, statics only): the dialog
// built in memory (an 11-control template: mode, adapter, resolution, the 24/32-bit Z buffer box,
// multisampling, OK and CANCEL), its procedure and the lists it fills from the renderer's system
// description, and the renderer's creation from the choices (or straight from the command line's
// when no dialog is wanted: NVPerfHUD's adapter when asked for). Plus the template's control
// writer the Direct3D 10 dialog uses too.
//
// The dialog's state is the statics the original keeps (0x20E598C.., 0xBC8F50).

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
GAME_VAR(uint8_t*, ms_pkDeviceDesc, 0x020E598C);   // NiDX9DeviceDesc (the adapter's HAL one)
GAME_VAR(uint32_t*, ms_pkMode, 0x020E5990);         // the resolution: width, height, bpp, format
GAME_VAR(uint8_t*, ms_pkAdapter, 0x020E5994);       // NiDX9AdapterDesc
GAME_VAR(uint32_t*, ms_pkSystemDesc, 0x020E5998);   // NiDX9SystemDesc: the adapters' count, array
GAME_VAR(uint8_t, ms_bFullscreen, 0x020E599C);
GAME_VAR(uint8_t, ms_bDepth32, 0x020E599D);
GAME_VAR(uint8_t, ms_bDialogOpen, 0x020E599E);
GAME_VAR(uint32_t, ms_uiMultisample, 0x020E59A0);
GAME_VAR(uint32_t, ms_uiDefaultWidth, 0x00BC8F50);
GAME_VAR(uint32_t, ms_uiDefaultHeight, 0x00BC8F54);

// Win32 through the game's import slots
constexpr uintptr_t kMultiByteToWideChar = 0x008FC074;
constexpr uintptr_t klstrlenA = 0x008FC0EC;
constexpr uintptr_t kMoveWindow = 0x008FC220;
constexpr uintptr_t kGetWindowRect = 0x008FC264;
constexpr uintptr_t kSetWindowTextA = 0x008FC270;
constexpr uintptr_t kSendDlgItemMessageA = 0x008FC284;
constexpr uintptr_t kCheckDlgButton = 0x008FC288;
constexpr uintptr_t kGetWindowLongA = 0x008FC290;
constexpr uintptr_t kDialogBoxIndirectParamA = 0x008FC294;
constexpr uintptr_t kEndDialog = 0x008FC298;

// combo box messages
constexpr uint32_t CB_ADDSTRING = 0x143;
constexpr uint32_t CB_RESETCONTENT = 0x14B;
constexpr uint32_t CB_SETCURSEL = 0x14E;
constexpr uint32_t CB_GETCURSEL = 0x147;
constexpr uint32_t CB_GETITEMDATA = 0x150;
constexpr uint32_t CB_SETITEMDATA = 0x151;
constexpr uint32_t BM_GETCHECK = 0xF2;
// the controls
constexpr int kAdapter = 1000;
constexpr int kMode = 1001;
constexpr int kResolution = 1002;
constexpr int kDepth = 1003;
constexpr int kMultisample = 1004;

struct RECT {
    int32_t left, top, right, bottom;
};

GAME_FN(void*, __cdecl, GameMalloc, 0x005EEAA0, uint32_t size);   // the game's operator new
GAME_FN(void, __cdecl, GameFree, 0x005EEAB0, void* p);            // and delete
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(uint32_t*, __cdecl, DX9Renderer_GetSystemDesc, 0x0087AC00);
GAME_FN(NiObject*, __cdecl, DX9Renderer_Create, 0x00882140, uint32_t width, uint32_t height, uint32_t flags,
        void* deviceWindow, void* focusWindow, uint32_t adapter, uint32_t deviceType, uint32_t frameBufferFormat,
        uint32_t depthStencilFormat, uint32_t presentationInterval, uint32_t swapEffect, uint32_t frameBufferMode,
        uint32_t backBuffers, uint32_t refreshRate);
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* self, NiRefObject* object);
GAME_FN(uint32_t, __thiscall, DeviceDesc_GetDefaultFormat, 0x008909A0, void* self, int windowed, uint32_t bits);
GAME_FN(uint32_t, __thiscall, FormatInfo_GetDepthStencilFormat, 0x00891150, void* self, uint32_t bits, uint32_t stencil);
GAME_FN(uint32_t, __thiscall, FormatInfo_GetMultisampleQualityLevels, 0x00890E50, void* self, uint32_t samples,
        int windowed, uint32_t format, uint32_t depthFormat);
GAME_FN(uint8_t, __thiscall, FormatInfo_IsMultisampleSupported, 0x00890ED0, void* self, int windowed,
        uint32_t samples, uint32_t format, uint32_t depthFormat);
// (this file's, called where modes apply)
GAME_FN(uint32_t, __cdecl, GetD3DFormat, 0x0082C6B0, uint32_t format);
GAME_FN(uint32_t, __cdecl, GetNiFormat, 0x0082C780, uint32_t format);
GAME_FN(void, __cdecl, AddDialogControl, 0x0082D990, re::Ptr32<uint8_t>* cursor, uint32_t style, int16_t x, int16_t y,
        int16_t cx, int16_t cy, uint16_t id, const char* className, const char* text);
GAME_FN(void*, __cdecl, BuildDialogTemplate, 0x0082C430);
GAME_FN(uint8_t, __cdecl, ReadAdapter, 0x0082C300, void* dialog);
GAME_FN(uint8_t, __cdecl, ReadResolution, 0x0082C370, void* dialog);
GAME_FN(uint8_t, __cdecl, ReadMultisample, 0x0082C3D0, void* dialog);
GAME_FN(void, __cdecl, FillAdapters, 0x0082C8D0, void* dialog);
GAME_FN(void, __cdecl, FillResolutions, 0x0082C9E0, void* dialog);
GAME_FN(void, __cdecl, FillMultisamples, 0x0082CB80, void* dialog);
GAME_FN(uint32_t, __cdecl, Select, 0x0082D130, void* window, uint32_t width, uint32_t height, uint8_t* fullscreen,
        uint8_t* depth32, re::Ptr32<uint8_t>* adapter, re::Ptr32<uint8_t>* device, re::Ptr32<uint32_t>* mode, uint32_t* multisample);
GAME_FN(NiObject*, __cdecl, SelectRenderer, 0x0082D1F0, void* window, void* renderWindow, uint32_t width,
        uint32_t height, int stencil, int multiThread, int refRast, int swVertex, uint8_t* nvPerfHUD,
        uint8_t* fullscreen);

inline intptr_t Send(void* dialog, int control, uint32_t message, uintptr_t wParam, intptr_t lParam) {
    return Import<intptr_t>(kSendDlgItemMessageA, dialog, control, message, wParam, lParam);
}

// a string written into the template as UTF-16 (its terminator after it); returns its characters
inline int WriteWide(uint16_t* to, const char* text) {
    int length = Import<int>(klstrlenA, text);
    int written = 0;
    if (length != 0) {
        written = Import<int>(kMultiByteToWideChar, 0u, 0u, text, length, to, length);
    }
    to[written] = 0;
    return written;
}

// the adapter's device description when it has one (the HAL one: its +4 set)
inline uint8_t* DeviceOf(uint8_t* adapter) {
    uint8_t* device = At<uint8_t*>(adapter, 0x460);
    return At<uint32_t>(device, 4) != 0 ? device : nullptr;
}

// ---- the template

// 0x82C430: the dialog's template (the game's allocator; null when that fails)
void* __cdecl BuildDialogTemplateImpl() {
    auto* memory = static_cast<uint8_t*>(GameMalloc(900));
    if (memory == nullptr) {
        return nullptr;
    }
    crt::Memset(memory, 0, 900);
    At<uint32_t>(memory, 0) = 0x90C80BC4;   // the style
    At<uint32_t>(memory, 4) = 0;
    At<uint16_t>(memory, 8) = 0xB;          // controls
    At<uint16_t>(memory, 0xA) = 0;
    At<uint16_t>(memory, 0xC) = 0;
    At<uint16_t>(memory, 0xE) = 200;
    At<uint16_t>(memory, 0x10) = 0x8C;
    auto* p = reinterpret_cast<uint16_t*>(memory + 0x12);
    *p++ = 0;   // no menu
    *p++ = 0;   // the default class
    p += WriteWide(p, "Select Desired DirectX9 Device") + 1;
    *p++ = 8;   // the font's size
    p += WriteWide(p, "Arial") + 1;
    re::Ptr32<uint8_t> cursor = reinterpret_cast<uint8_t*>(p);
    AddDialogControl(&cursor, 0x210003, 0x37, 5, 0x85, 0x2D, kMode, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 5, 5, 0x29, 0xD, 0xFFFF, "STATIC", "Mode:");
    AddDialogControl(&cursor, 0x210003, 0x37, 0x1B, 0x85, 0x2D, kAdapter, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 5, 0x1B, 0x29, 0xD, 0xFFFF, "STATIC", "Adapter:");
    AddDialogControl(&cursor, 0x210003, 0x37, 0x31, 0x85, 0x2D, kResolution, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 5, 0x31, 0x29, 0xD, 0xFFFF, "STATIC", "Resolution:");
    AddDialogControl(&cursor, 0x10003, 0x28, 0x42, 0x78, 0xE, kDepth, "BUTTON", "Use 24/32-bit ZBuffer if available");
    AddDialogControl(&cursor, 0x210003, 0x37, 0x58, 0x85, 0x2D, kMultisample, "COMBOBOX", "");
    AddDialogControl(&cursor, 0, 5, 0x58, 0x29, 0xD, 0xFFFF, "STATIC", "MultiSample:");
    AddDialogControl(&cursor, 0x10000, 0x7B, 0x6E, 0x33, 0xE, 2, "BUTTON", "CANCEL");
    AddDialogControl(&cursor, 0x10000, 0x28, 0x6E, 0x33, 0xE, 1, "BUTTON", "OK");
    return memory;
}

// 0x82D990: a control written at the cursor (DWORD aligned; visible child), its class and text,
// the cursor moved past them and the creation data's (empty) word
void __cdecl AddDialogControlImpl(re::Ptr32<uint8_t>* cursor, uint32_t style, int16_t x, int16_t y, int16_t cx, int16_t cy,
                                  uint16_t id, const char* className, const char* text) {
    auto* item = reinterpret_cast<uint8_t*>((re::PtrCast<uintptr_t>(*cursor) + 3) & ~uintptr_t(3));
    At<uint32_t>(item, 0) = style | 0x50000000;
    At<int16_t>(item, 8) = x;
    At<int16_t>(item, 0xA) = y;
    At<int16_t>(item, 0xC) = cx;
    At<uint32_t>(item, 4) = 0;
    At<int16_t>(item, 0xE) = cy;
    At<uint16_t>(item, 0x10) = id;
    *cursor = item + 0x12;
    int written = WriteWide(re::PtrCast<uint16_t*>(*cursor), className);
    *cursor += written * 2 + 2;
    written = WriteWide(re::PtrCast<uint16_t*>(*cursor), text);
    *cursor += written * 2 + 4;
}

// ---- the selections read back

// 0x82C300: the adapter chosen (and its device); true when it changed (or none is chosen)
uint8_t __cdecl ReadAdapterImpl(void* dialog) {
    intptr_t index = Send(dialog, kAdapter, CB_GETCURSEL, 0, 0);
    if (index < 0) {
        ms_pkDeviceDesc = nullptr;
        return 1;
    }
    auto* adapter = reinterpret_cast<uint8_t*>(Send(dialog, kAdapter, CB_GETITEMDATA, uintptr_t(index), 0));
    uint8_t changed = adapter != ms_pkAdapter ? 1 : 0;
    ms_pkAdapter = adapter;
    ms_pkDeviceDesc = DeviceOf(adapter);
    return changed;
}

// 0x82C370: the resolution chosen; true when it changed (or none is chosen)
uint8_t __cdecl ReadResolutionImpl(void* dialog) {
    intptr_t index = Send(dialog, kResolution, CB_GETCURSEL, 0, 0);
    if (index < 0) {
        ms_pkMode = nullptr;
        return 1;
    }
    auto* mode = reinterpret_cast<uint32_t*>(Send(dialog, kResolution, CB_GETITEMDATA, uintptr_t(index), 0));
    uint8_t changed = mode != ms_pkMode ? 1 : 0;
    ms_pkMode = mode;
    return changed;
}

// 0x82C3D0: the multisampling chosen; true when it changed (or none is chosen)
uint8_t __cdecl ReadMultisampleImpl(void* dialog) {
    intptr_t index = Send(dialog, kMultisample, CB_GETCURSEL, 0, 0);
    if (index < 0) {
        ms_uiMultisample = 0;
        return 1;
    }
    auto value = uint32_t(Send(dialog, kMultisample, CB_GETITEMDATA, uintptr_t(index), 0));
    uint8_t changed = value != ms_uiMultisample ? 1 : 0;
    ms_uiMultisample = value;
    return changed;
}

// ---- format maps

// 0x82C6B0: the renderer's frame buffer format (1..17) as Direct3D's (D3DFMT_R8G8B8.. and the
// floating-point ones); 0 for the rest
uint32_t __cdecl GetD3DFormatImpl(uint32_t format) {
    if (format >= 1 && format <= 11) {
        return format + 0x13;
    }
    if (format >= 12 && format <= 17) {
        return format + 0x63;
    }
    return 0;
}

// 0x82C780: Direct3D's format as the renderer's (the inverse); 0 for the rest
uint32_t __cdecl GetNiFormatImpl(uint32_t format) {
    if (format >= 0x14 && format <= 0x1E) {
        return format - 0x13;
    }
    if (format >= 0x6F && format <= 0x74) {
        return format - 0x63;
    }
    return 0;
}

// ---- the lists

// 0x82C8D0: the adapters with a device (fullscreen, or windowed when the device can be), the first
// chosen
void __cdecl FillAdaptersImpl(void* dialog) {
    Send(dialog, kAdapter, CB_RESETCONTENT, 0, 0);
    uint32_t count = ms_pkSystemDesc[0];
    bool any = false;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* adapter = nullptr;
        if (i < re::PtrCast<uint16_t*>(ms_pkSystemDesc)[7]) {
            adapter = reinterpret_cast<re::Ptr32<uint8_t>*>(ms_pkSystemDesc[2])[i];
        }
        uint8_t* device = At<uint8_t*>(adapter, 0x460);
        if (At<uint32_t>(device, 4) == 0 || device == nullptr) {
            continue;
        }
        if (!ms_bFullscreen && (device == nullptr || At<uint8_t>(device, 0x144) == 0)) {
            continue;
        }
        intptr_t index = Send(dialog, kAdapter, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(adapter + 0x204));
        Send(dialog, kAdapter, CB_SETITEMDATA, uintptr_t(index), reinterpret_cast<intptr_t>(adapter));
        any = true;
    }
    Send(dialog, kAdapter, CB_SETCURSEL, 0, 0);
    if (!any) {
        ms_pkAdapter = nullptr;
        ms_pkDeviceDesc = nullptr;
        return;
    }
    ms_pkAdapter = reinterpret_cast<uint8_t*>(Send(dialog, kAdapter, CB_GETITEMDATA, 0, 0));
    ms_pkDeviceDesc = DeviceOf(ms_pkAdapter);
}

// 0x82C9E0: the adapter's resolutions (fullscreen only), the default size's chosen (32 bits, else
// 16); the first item's data taken as the choice
void __cdecl FillResolutionsImpl(void* dialog) {
    Send(dialog, kResolution, CB_RESETCONTENT, 0, 0);
    if (!ms_bFullscreen || ms_pkAdapter == nullptr) {
        return;
    }
    uint32_t count = At<uint16_t>(ms_pkAdapter, 0x45A);
    intptr_t index32 = -1;
    intptr_t index16 = -1;
    bool any = false;
    if (count != 0) {
        any = true;
        for (uint32_t i = 0; i < count; i++) {
            auto mode = At<re::Ptr32<uint32_t>*>(ms_pkAdapter, 0x454)[i];
            char text[0x100];
            NiSprintf(text, 0x100, "%u x %u x %u bpp", mode[0], mode[1], mode[2]);
            intptr_t index = Send(dialog, kResolution, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
            Send(dialog, kResolution, CB_SETITEMDATA, uintptr_t(index), re::PtrCast<intptr_t>(mode));
            if (mode[0] == ms_uiDefaultWidth && mode[1] == ms_uiDefaultHeight) {
                if (mode[2] == 32) {
                    index32 = index;
                } else if (mode[2] == 16) {
                    index16 = index;
                }
            }
        }
    }
    intptr_t selection = index32 > -1 ? index32 : index16;
    Send(dialog, kResolution, CB_SETCURSEL, uintptr_t(selection), 0);
    if (any) {
        ms_pkMode = reinterpret_cast<uint32_t*>(Send(dialog, kResolution, CB_GETITEMDATA, 0, 0));
        return;
    }
    ms_pkMode = nullptr;
}

// 0x82CB80: the multisampling the device's format allows (none; the nonmaskable levels; 2..16
// samples and their levels), "No Multisampling" chosen
void __cdecl FillMultisamplesImpl(void* dialog) {
    Send(dialog, kMultisample, CB_RESETCONTENT, 0, 0);
    uint8_t* device = ms_pkDeviceDesc;
    if (device == nullptr) {
        return;
    }
    uint32_t format;
    if (ms_pkMode != nullptr) {
        format = ms_pkMode[3];
    } else {
        uint32_t bits = ms_bDepth32 ? 0x20 : 0x10;
        format = GetD3DFormat(DeviceDesc_GetDefaultFormat(device, ms_bFullscreen == 0, bits));
    }
    void* info = nullptr;
    struct ListNode {
        re::Ptr32<ListNode> next;
        re::Ptr32<ListNode> prev;
        re::Ptr32<uint32_t> element;
    };
    for (ListNode* node = At<ListNode*>(device, 0x138); node != nullptr; node = node->next) {
        if (node->element[0] == format) {
            info = node->element;
            break;
        }
    }
    uint32_t depthFormat = FormatInfo_GetDepthStencilFormat(info, ms_bDepth32 ? 0x20 : 0x10, 0);
    char text[0x100];
    NiSprintf(text, 0x100, "No Multisampling");
    intptr_t none = Send(dialog, kMultisample, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
    Send(dialog, kMultisample, CB_SETITEMDATA, uintptr_t(none), 0);
    uint32_t levels = FormatInfo_GetMultisampleQualityLevels(info, 1, ms_bFullscreen == 0, format, depthFormat);
    for (uint32_t i = 0; i < levels; i++) {
        NiSprintf(text, 0x100, "Nonmaskable Level %d", i);
        intptr_t index = Send(dialog, kMultisample, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
        Send(dialog, kMultisample, CB_SETITEMDATA, uintptr_t(index), intptr_t(i | 0x80000000));
    }
    uint32_t samples = 2;
    for (int n = 15; n != 0; n--, samples++) {
        if (!FormatInfo_IsMultisampleSupported(info, ms_bFullscreen == 0, samples, format, depthFormat)) {
            continue;
        }
        levels = FormatInfo_GetMultisampleQualityLevels(info, samples, ms_bFullscreen == 0, format, depthFormat);
        uint32_t base = (samples << 16) - 0x10000;
        for (uint32_t i = 0; i < levels; i++) {
            if (levels > 1) {
                NiSprintf(text, 0x100, "%d Multisamples, Level %d", samples, i);
            } else {
                NiSprintf(text, 0x100, "%d Multisamples", samples);
            }
            intptr_t index = Send(dialog, kMultisample, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>(text));
            Send(dialog, kMultisample, CB_SETITEMDATA, uintptr_t(index), intptr_t(base | i));
        }
    }
    Send(dialog, kMultisample, CB_SETCURSEL, uintptr_t(none), 0);
    ms_uiMultisample = 0;
}

// the controls emptied and the dialog ended (OK and CANCEL)
void EndWith(void* dialog, intptr_t result) {
    Send(dialog, kMode, CB_RESETCONTENT, 0, 0);
    Send(dialog, kAdapter, CB_RESETCONTENT, 0, 0);
    Send(dialog, kResolution, CB_RESETCONTENT, 0, 0);
    Send(dialog, kMultisample, CB_RESETCONTENT, 0, 0);
    Import<int>(kEndDialog, dialog, result);
}

// 0x82CE40: the dialog procedure: WM_INITDIALOG fills the lists (windowed first), WM_COMMAND
// follows the choices (each list refilled from the one that changed) and ends on OK / CANCEL
intptr_t __stdcall DialogProc(void* dialog, uint32_t message, uintptr_t wParam, intptr_t) {
    if (message == 0x110) {
        Import<int>(kSetWindowTextA, dialog, "Select DX9 Display Device");
        Send(dialog, kMode, CB_RESETCONTENT, 0, 0);
        intptr_t index = Send(dialog, kMode, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>("Windowed"));
        Send(dialog, kMode, CB_SETITEMDATA, uintptr_t(index), 0);
        Send(dialog, kMode, CB_SETCURSEL, 0, 0);
        ms_bFullscreen = 0;
        index = Send(dialog, kMode, CB_ADDSTRING, 0, reinterpret_cast<intptr_t>("Fullscreen"));
        Send(dialog, kMode, CB_SETITEMDATA, uintptr_t(index), 1);
        ms_bDialogOpen = 1;
        FillAdapters(dialog);
        FillResolutions(dialog);
        FillMultisamples(dialog);
        Import<int>(kCheckDlgButton, dialog, kDepth, 1u);
        return 1;
    }
    if (message != 0x111) {
        return 0;
    }
    uint16_t id = uint16_t(wParam);
    switch (id) {
    case 1:
        ReadAdapter(dialog);
        ReadResolution(dialog);
        ReadMultisample(dialog);
        ms_bDialogOpen = 0;
        ms_bDepth32 = uint8_t(Send(dialog, kDepth, BM_GETCHECK, 0, 0)) & 1;
        EndWith(dialog, 1);
        return 1;
    case 2:
        ms_bDialogOpen = 0;
        EndWith(dialog, 0);
        return 1;
    case kAdapter:
        if (ms_bDialogOpen && ReadAdapter(dialog)) {
            FillResolutions(dialog);
            FillMultisamples(dialog);
        }
        return 1;
    case kMode:
        if (ms_bDialogOpen) {
            intptr_t index = Send(dialog, kMode, CB_GETCURSEL, 0, 0);
            uint8_t old = ms_bFullscreen;
            if (index >= 0) {
                ms_bFullscreen = Send(dialog, kMode, CB_GETITEMDATA, uintptr_t(index), 0) != 0 ? 1 : 0;
                if (old != ms_bFullscreen) {
                    FillAdapters(dialog);
                    FillResolutions(dialog);
                    FillMultisamples(dialog);
                }
            }
        }
        return 1;
    case kResolution:
        if (ms_bDialogOpen && ReadResolution(dialog)) {
            FillMultisamples(dialog);
        }
        return 1;
    default:
        return 0;
    }
}

// 0x82D130: Select(window, default width, default height, fullscreen&, depth32&, adapter&, device&,
// mode&, multisample&): the dialog run; 0 with the choices (no mode windowed), 2 when cancelled
// (or the template can't be made)
uint32_t __cdecl SelectImpl(void* window, uint32_t width, uint32_t height, uint8_t* fullscreen, uint8_t* depth32,
                            re::Ptr32<uint8_t>* adapter, re::Ptr32<uint8_t>* device, re::Ptr32<uint32_t>* mode, uint32_t* multisample) {
    *fullscreen = 0;
    ms_uiDefaultWidth = width;
    ms_uiDefaultHeight = height;
    void* instance = reinterpret_cast<void*>(Import<intptr_t>(kGetWindowLongA, window, -6));
    void* dialogTemplate = BuildDialogTemplate();
    if (dialogTemplate == nullptr) {
        return 2;
    }
    intptr_t result = Import<intptr_t>(kDialogBoxIndirectParamA, instance, dialogTemplate, window,
                                       reinterpret_cast<void*>(0x0082CE40), intptr_t(0));
    GameFree(dialogTemplate);
    if (result == 0) {
        return 2;
    }
    *adapter = ms_pkAdapter;
    *device = ms_pkDeviceDesc;
    *fullscreen = ms_bFullscreen;
    *mode = ms_bFullscreen != 0 ? ms_pkMode : nullptr;
    *depth32 = ms_bDepth32;
    *multisample = ms_uiMultisample;
    return 0;
}

// the device type for the flags: the reference rasterizer (4) when asked or the only device;
// software vertex processing (3); else the hardware's (0)
inline uint32_t DeviceType(int refRast, const uint8_t* device, int swVertex) {
    if (re::Flag(refRast) || At<uint32_t>(device, 0) == 2) {
        return 4;
    }
    return re::Flag(swVertex) ? 3 : 0;
}

// 0x82D1F0: SelectRenderer: the dialog until a renderer is made from its choices (the window put
// back after a failure); null when cancelled. NVPerfHUD's adapter takes its own device types
NiObject* __cdecl SelectRendererImpl(void* window, void* renderWindow, uint32_t width, uint32_t height, int stencil,
                                     int multiThread, int refRast, int swVertex, uint8_t* nvPerfHUD,
                                     uint8_t* fullscreen) {
    RECT rect;
    Import<int>(kGetWindowRect, window, &rect);
    for (;;) {
        uint32_t flags = 0;
        if (re::Flag(stencil)) {
            flags = 8;
        }
        if (re::Flag(multiThread)) {
            flags |= 0x20;
        }
        re::Ptr32<uint8_t> adapter = nullptr;
        re::Ptr32<uint8_t> device = nullptr;
        re::Ptr32<uint32_t> mode = nullptr;
        uint8_t depth32 = 0;
        uint32_t multisample = 0;
        if (Select(window, width, height, fullscreen, &depth32, &adapter, &device, &mode, &multisample) == 2) {
            return nullptr;
        }
        if (!depth32) {
            flags |= 0x10;
        }
        NiObject* renderer;
        if (*fullscreen) {
            flags |= 4;
            if (adapter == nullptr || device == nullptr || mode == nullptr) {
                renderer = DX9Renderer_Create(0x280, 0x1E0, flags, window, window, 0, 0, 0, 0, 1, 0, 0, 1, 0);
            } else {
                uint32_t type = DeviceType(refRast, device, swVertex);
                if (crt::Strstr(reinterpret_cast<const char*>(adapter + 0x204), "NVPerfHUD") != nullptr) {
                    *nvPerfHUD = 1;
                    type = (re::Flag(refRast) || re::Flag(swVertex)) ? 4 : 5;
                }
                uint32_t format = GetNiFormat(mode[3]);
                renderer = DX9Renderer_Create(mode[0], mode[1], flags, window, window, *re::PtrCast<uint32_t*>(adapter),
                                              type, format, 0, 0, 0, multisample, 1, 0);
            }
        } else {
            uint32_t type = DeviceType(refRast, device, swVertex);
            if (crt::Strstr(reinterpret_cast<const char*>(adapter + 0x204), "NVPerfHUD") != nullptr) {
                type = re::Flag(refRast) ? 4 : (re::Flag(swVertex) ? 6 : 5);
            }
            renderer = DX9Renderer_Create(0, 0, flags, renderWindow, window, *re::PtrCast<uint32_t*>(adapter), type,
                                          0, 0, 0, 0, multisample, 1, 0);
        }
        if (renderer != nullptr) {
            return renderer;
        }
        Import<int>(kMoveWindow, window, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 1);
    }
}

// 0x82D420: CreateDX9Renderer(result&, window, render window, dialog, bit depth, width, height,
// stencil, multi-thread, reference rasterizer, software vertices, NVPerfHUD&, fullscreen&): the
// renderer through the dialog when asked (and no bit depth given), else straight: fullscreen at
// the size (16 or 32 bits given), or in the render window; NVPerfHUD's adapter when asked for.
// Returns the result (a NiPointer)
re::Ptr32<NiObject>* __cdecl CreateDX9Renderer(re::Ptr32<NiObject>* result, void* window, void* renderWindow, int dialog,
                                     uint32_t bitDepth, uint32_t width, uint32_t height, int stencil, int multiThread,
                                     int refRast, int swVertex, uint8_t* nvPerfHUD, uint8_t* fullscreen) {
    ms_pkSystemDesc = DX9Renderer_GetSystemDesc();
    if (ms_pkSystemDesc == nullptr) {
        *result = nullptr;
        return result;
    }
    NiObject* renderer = nullptr;   // (a NiPointer)
    NiObject* made;
    if (re::Flag(dialog) && bitDepth == 0) {
        made = SelectRenderer(window, renderWindow, width, height, stencil, multiThread, refRast, swVertex, nvPerfHUD,
                              fullscreen);
        Pointer_Assign(reinterpret_cast<re::Ptr32<NiRefObject>*>(&renderer), made);
    } else {
        uint32_t flags = re::Flag(stencil) ? 8 : 0;
        if (re::Flag(multiThread)) {
            flags |= 0x20;
        }
        uint32_t adapterIndex = 0;
        uint32_t type = 0;
        if (*nvPerfHUD) {
            uint32_t count = ms_pkSystemDesc[0];
            for (uint32_t i = 0; i < count; i++) {
                uint8_t* adapter = nullptr;
                if (i < re::PtrCast<uint16_t*>(ms_pkSystemDesc)[7]) {
                    adapter = reinterpret_cast<re::Ptr32<uint8_t>*>(ms_pkSystemDesc[2])[i];
                }
                if (crt::Strstr(reinterpret_cast<const char*>(adapter + 0x204), "NVPerfHUD") != nullptr) {
                    *nvPerfHUD = 1;
                    adapterIndex = i;
                    type = re::Flag(refRast) ? 4 : (re::Flag(swVertex) ? 6 : 5);
                    break;
                }
            }
        }
        if (bitDepth == 16 || bitDepth == 32) {
            flags |= 4;
            if (bitDepth == 16) {
                flags |= 0x10;
            }
            made = DX9Renderer_Create(width, height, flags, window, window, adapterIndex, type, 0, 0, 0, 0, 0, 1, 0);
            if (made != nullptr) {
                renderer = made;   // (the local pointer made from it: held)
                sys::Increment(&made->refCount);
            }
        } else {
            if (*fullscreen) {
                made = DX9Renderer_Create(width, height, flags | 4, window, window, adapterIndex, type, 0, 0, 0, 0, 0,
                                          1, 0);
            } else {
                made = DX9Renderer_Create(0, 0, flags, renderWindow, renderWindow, adapterIndex, type, 0, 0, 0, 0, 0, 1,
                                          0);
            }
            Pointer_Assign(reinterpret_cast<re::Ptr32<NiRefObject>*>(&renderer), made);
        }
    }
    *result = renderer;
    if (renderer != nullptr) {
        sys::Increment(&renderer->refCount);
        if (sys::Decrement(&renderer->refCount) == 0) {   // (the local pointer's)
            sys::Virtual<void>(renderer, 1);
        }
    }
    return result;
}

}   // namespace
}   // namespace ni::app

// ---- registration ---------------------------------------------------------------------

#define C33_DX9SELECT_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_DX9SELECT_REVERSED("NiDX9Select::ReadAdapter", 0x0082C300, &ni::app::ReadAdapterImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::ReadResolution", 0x0082C370, &ni::app::ReadResolutionImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::ReadMultisample", 0x0082C3D0, &ni::app::ReadMultisampleImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::BuildDialogTemplate", 0x0082C430, &ni::app::BuildDialogTemplateImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::GetD3DFormat", 0x0082C6B0, &ni::app::GetD3DFormatImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::GetNiFormat", 0x0082C780, &ni::app::GetNiFormatImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::FillAdapters", 0x0082C8D0, &ni::app::FillAdaptersImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::FillResolutions", 0x0082C9E0, &ni::app::FillResolutionsImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::FillMultisamples", 0x0082CB80, &ni::app::FillMultisamplesImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::DialogProc", 0x0082CE40, &ni::app::DialogProc);
C33_DX9SELECT_REVERSED("NiDX9Select::Select", 0x0082D130, &ni::app::SelectImpl);
C33_DX9SELECT_REVERSED("NiDX9Select::SelectRenderer", 0x0082D1F0, &ni::app::SelectRendererImpl);
C33_DX9SELECT_REVERSED("NiApplication::CreateDX9Renderer", 0x0082D420, &ni::app::CreateDX9Renderer);
C33_DX9SELECT_REVERSED("NiDX9Select::AddDialogControl", 0x0082D990, &ni::app::AddDialogControlImpl);
