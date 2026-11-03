// NiApplication's construction and destruction (src/gamebryo/NiApplication.h): the constructor
// (window, timings, the render frame's names, the screen elements' view, the screen texture and
// visual tracker clicks, the culler), the destructor, the visual trackers it makes (performance and
// frame times), the input system's create parameters (and their classes), the input handling and
// window resizing virtuals; the RTTI and destructor slots of the render view and clicks it uses.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiApplication.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"
#include "runflags.h"

namespace ni::app {
namespace {

using sys::At;
using sys::Import;
using sys::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiInputSystem_CreateParams_RTTI, 0x020E97A4);
GAME_VAR(RTTI, NiDI8InputSystem_DI8CreateParams_RTTI, 0x020E9738);
GAME_VAR(RTTI, Ni2DRenderView_RTTI, 0x020E9588);
GAME_VAR(RTTI, NiScreenTextureRenderClick_RTTI, 0x020E95BC);
GAME_VAR(RTTI, NiVisualTrackerRenderClick_RTTI, 0x020E59CC);
GAME_VAR(uint8_t, ms_bRendererCreated, 0x020E2180);
GAME_VAR(float, kOneSixtieth, 0x009144C4);
GAME_VAR(float, kAppEndTime, 0x009687E4);        // FLT_MAX
GAME_VAR(float, kInvalidTime, 0x009009BC);       // -FLT_MAX
GAME_VAR(float, kNotYet, 0x009009AC);            // -1
GAME_VAR(float, kMinFramePeriod, 0x0090054C);    // 0.01
GAME_VAR(float, kHundred, 0x00900490);
GAME_VAR(float, kTenThousand, 0x008FF34C);
// the trackers' layout (fractions of the screen) and sampling
GAME_VAR(float, kTrackerLeft, 0x009068B8);       // 0.05
GAME_VAR(float, kTrackerRight, 0x0091C100);      // 0.95
GAME_VAR(float, kPerformanceTop, 0x00900E80);    // 0.15
GAME_VAR(float, kPerformanceBottom, 0x0090048C); // 0.4
GAME_VAR(float, kTimeTop, 0x0094D020);           // 0.425
GAME_VAR(float, kTimeBottom, 0x00968A80);        // 0.675
GAME_VAR(float, kSamplingInterval, 0x00900170);  // 0.1
GAME_VAR(float, kMilliseconds, 0x00900C04);      // 0.001

constexpr uintptr_t NiApplication_vtable = 0x00968988;
constexpr uintptr_t CreateParams_vtable = 0x00968974;
constexpr uintptr_t DI8CreateParams_vtable = 0x0096897C;
constexpr uintptr_t Ni2DRenderView_vtable = 0x00968A84;
constexpr uintptr_t NiScreenTextureRenderClick_vtable = 0x00968A98;
constexpr uintptr_t NiVisualTrackerRenderClick_vtable = 0x00968AB4;
constexpr uintptr_t NiTPointerList_vtable = 0x0095127C;
constexpr uintptr_t NiTPointerListBase_vtable = 0x0099C748;   // (the allocator's, left by the list's destructor)

// Win32 through the game's import slots
constexpr uintptr_t kGetVersionExA = 0x008FC0E8;
constexpr uintptr_t kMoveWindow = 0x008FC220;
constexpr uintptr_t kGetClientRect = 0x008FC22C;
constexpr uintptr_t kPostMessageA = 0x008FC250;
constexpr uintptr_t kGetWindowRect = 0x008FC264;
constexpr uintptr_t kSendMessageA = 0x008FC26C;

struct RECT {
    int32_t left, top, right, bottom;
};

GAME_FN(void*, __cdecl, NiMalloc_, 0x00752300, uint32_t size);
GAME_FN(void, __thiscall, VisibleArray_SetAllocatedSize, 0x00776B50, void* self, uint32_t size);
GAME_FN(NiObject*, __thiscall, CullingProcess_Construct, 0x00776A30, void* self, void* visible);
GAME_FN(void*, __thiscall, DevImageConverter_Construct, 0x008CA1D0, void* self);
GAME_FN(void*, __thiscall, ImageConverter_Construct, 0x007D3B50, void* self);
GAME_FN(void, __cdecl, ImageConverter_SetImageConverter, 0x007D3F40, void* converter);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(void, __thiscall, RenderView_Construct, 0x00755F90, void* self);
GAME_FN(void, __thiscall, RenderView_Destruct, 0x00756010, void* self);
GAME_FN(void, __thiscall, RenderClick_Construct, 0x00515C00, void* self);
GAME_FN(void, __thiscall, List_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, List_AddTail, 0x007843D0, void* list, const re::Ptr32<NiObject>* element);
GAME_FN(uint8_t, __thiscall, GamePad_ButtonIsDown, 0x008D0AF0, void* self, uint32_t button);
// this chunk's (the other files'), called where modes apply
GAME_FN(NiAppWindow*, __thiscall, AppWindow_Construct, 0x0082B7B0, void* self, const char* caption, uint32_t width,
        uint32_t height, uint32_t numStatusPanes, uint32_t windowStyle);
GAME_FN(void, __thiscall, AppWindow_Destruct, 0x0082BB90, NiAppWindow* self);
GAME_FN(void, __thiscall, Command_Destruct, 0x0082BBE0, NiCommand* self);
GAME_FN(NiObject*, __thiscall, VisualTracker_Construct, 0x0082FAD0, void* self, float maxValue, uint32_t unknown,
        float left, float right, float top, float bottom, const char* name, int show, uint32_t numGraphs);
GAME_FN(void, __thiscall, VisualTracker_SetShow, 0x0082F170, void* self, int show);
GAME_FN(void, __thiscall, VisualTracker_AddGraph, 0x00830AC0, void* self, void* callback, const char* name,
        const float* color, uint32_t numSamples, float samplingTime, int show);
GAME_FN(void*, __thiscall, FrameRateUpdate_Construct, 0x00830BC0, void* self, float* frameRate);
GAME_FN(void*, __thiscall, VisibleArraySize_Construct, 0x00830BE0, void* self, void* visible);
GAME_FN(void*, __thiscall, GenericFloatUpdate_Construct, 0x00830C20, void* self, float scale, float* value);
GAME_FN(void*, __thiscall, GenericUnsignedUpdate_Construct, 0x00830C40, void* self, float scale, uint32_t* value);
GAME_FN(NiDI8CreateParams*, __thiscall, DI8CreateParams_Construct, 0x0082A690, void* self);
GAME_FN(void, __thiscall, DI8CreateParams_Destruct, 0x0082A720, void* self);
GAME_FN(void, __thiscall, NiApplication_Destruct, 0x0082A780, void* self);

inline void Unref(NiRefObject* object) {
    if (object != nullptr && sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}
template <typename T, typename U>
inline void Assign(re::Ptr32<T>& slot, U* value) {
    T* v = reinterpret_cast<T*>(value);
    if (slot != v) {
        Unref(reinterpret_cast<NiRefObject*>(slot));
        slot = v;
        if (v != nullptr) {
            sys::Increment(&reinterpret_cast<NiRefObject*>(v)->refCount);
        }
    }
}

// the status bar's window (its panes asked for and made)
inline void* StatusWindow(const NiAppWindow* window) {
    if (window->numStatusPanes == 0 || window->statusWindow == nullptr) {
        return nullptr;
    }
    return window->statusWindow[0];
}

// ---- NiInputSystem::CreateParams, NiDI8InputSystem::DI8CreateParams

// 0x82A680
const RTTI* __fastcall CreateParams_GetRTTI(NiInputCreateParams*, re::Edx) {
    return &NiInputSystem_CreateParams_RTTI;
}

// 0x82A690: DI8CreateParams() (8 of each kind of device, axes -100..100, nothing named)
NiDI8CreateParams* __fastcall DI8Params_Construct(NiDI8CreateParams* self, re::Edx) {
    self->actionMap = nullptr;
    self->keyboardFlags = 0;
    self->axisLow = -100;
    self->axisHigh = 100;
    self->mouseFlags = 8;
    self->gamePadFlags = 8;
    self->renderer = nullptr;
    self->vtable = reinterpret_cast<void*>(DI8CreateParams_vtable);
    self->instance = nullptr;
    self->window = nullptr;
    for (int i = 0; i < 21; i++) {
        self->actionNames[i] = nullptr;
    }
    return self;
}

// 0x82A710
const RTTI* __fastcall DI8Params_GetRTTI(NiDI8CreateParams*, re::Edx) {
    return &NiDI8InputSystem_DI8CreateParams_RTTI;
}

// 0x82A720: ~DI8CreateParams (the names freed; then the base's: the renderer let go)
void __fastcall DI8Params_Destruct(NiDI8CreateParams* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(DI8CreateParams_vtable);
    for (int i = 0; i < 21; i++) {
        if (self->actionNames[i] != nullptr) {
            NiFree(self->actionNames[i]);
        }
    }
    self->vtable = reinterpret_cast<void*>(CreateParams_vtable);
    Unref(self->renderer);
}

// 0x82B170: CreateParams' deleting destructor
NiInputCreateParams* __fastcall CreateParams_DeletingDestructor(NiInputCreateParams* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(CreateParams_vtable);
    Unref(self->renderer);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x20);
    }
    return self;
}

// 0x82B1C0: DI8CreateParams' deleting destructor
NiDI8CreateParams* __fastcall DI8Params_DeletingDestructor(NiDI8CreateParams* self, re::Edx, uint32_t flags) {
    DI8CreateParams_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x7C);
    }
    return self;
}

// ---- NiApplication

// 0x82A780: ~NiApplication: the default image converter put back, the command line and the
// window deleted, the statics cleared, the names and references let go
void __fastcall Destruct(NiApplication* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiApplication_vtable);
    void* memory = NiNew(0x438);
    ImageConverter_SetImageConverter(memory != nullptr ? ImageConverter_Construct(memory) : nullptr);
    NiCommand* command = ms_pkCommand;
    if (command != nullptr) {
        Command_Destruct(command);
        NiDelete(command, 0x20);
    }
    NiAppWindow* window = self->appWindow;
    if (window != nullptr) {
        AppWindow_Destruct(window);
        NiDelete(window, 0x30);
    }
    ms_pkApplication = nullptr;
    ms_pkCommand = nullptr;
    for (int i = 9; i >= 0; i--) {
        scene::FixedStringRelease(&self->names[i]);
    }
    Unref(self->culler);
    NiFree(self->visible);
    Unref(self->inputSystem);
    Unref(self->renderer);
    Unref(re::PtrCast<NiRefObject*>(self->accumulatorProcessor));
    Unref(self->visualTrackerClick);
    Unref(self->screenTextureClick);
    Unref(self->screenElementsView);
    Unref(self->mainSceneView);
    Unref(self->frame);
    Unref(self->camera);
    Unref(self->scene);
}

// 0x82AB20: CreateVisualTrackers (slot 36): the performance tracker (frame rate, visible objects:
// the frame system's drawn count or the visible array's) and the time tracker (update, cull,
// render in ms), both hidden, added to the visual tracker click
uint8_t __fastcall CreateVisualTrackers(NiApplication* self, re::Edx) {
    float yellow[3] = {1.0f, 1.0f, 0.0f};
    float red[3] = {1.0f, 0.0f, 0.0f};
    float blue[3] = {0.0f, 0.0f, 1.0f};

    void* memory = NiNew(0x178);
    NiObject* tracker = memory != nullptr
                            ? VisualTracker_Construct(memory, self->vtPerformanceMax, 0, kTrackerLeft, kTrackerRight,
                                                      kPerformanceTop, kPerformanceBottom, "Performance", 1, 2)
                            : nullptr;
    VisualTracker_SetShow(tracker, 0);
    memory = NiNew(8);
    void* callback = memory != nullptr ? FrameRateUpdate_Construct(memory, &self->frameRateValue) : nullptr;
    VisualTracker_AddGraph(tracker, callback, "Frame-Rate", red, 100, kSamplingInterval, 1);
    if (self->useFrameSystem) {
        memory = NiNew(0xC);
        callback = memory != nullptr ? GenericUnsignedUpdate_Construct(memory, 1.0f, &self->numObjectsDrawn) : nullptr;
    } else {
        memory = NiNew(8);
        callback = memory != nullptr ? VisibleArraySize_Construct(memory, &self->visible) : nullptr;
    }
    VisualTracker_AddGraph(tracker, callback, "Vis Objects", yellow, 100, kSamplingInterval, 1);
    re::Ptr32<NiObject> held = tracker;
    if (held != nullptr) {
        sys::Increment(&held->refCount);
    }
    List_AddTail(static_cast<uint8_t*>(static_cast<void*>(self->visualTrackerClick)) + 0x4C, &held);
    Unref(tracker);

    memory = NiNew(0x178);
    tracker = memory != nullptr ? VisualTracker_Construct(memory, self->vtFrameRateMax, 0, kTrackerLeft, kTrackerRight,
                                                          kTimeTop, kTimeBottom, "Time (ms)", 1, 3)
                                : nullptr;
    VisualTracker_SetShow(tracker, 0);
    memory = NiNew(0xC);
    callback = memory != nullptr ? GenericFloatUpdate_Construct(memory, kMilliseconds, &self->updateTime) : nullptr;
    VisualTracker_AddGraph(tracker, callback, "Update", red, 100, kSamplingInterval, 1);
    memory = NiNew(0xC);
    callback = memory != nullptr ? GenericFloatUpdate_Construct(memory, kMilliseconds, &self->cullTime) : nullptr;
    VisualTracker_AddGraph(tracker, callback, "Cull", yellow, 100, kSamplingInterval, 1);
    memory = NiNew(0xC);
    callback = memory != nullptr ? GenericFloatUpdate_Construct(memory, kMilliseconds, &self->renderTime) : nullptr;
    VisualTracker_AddGraph(tracker, callback, "Render", blue, 100, kSamplingInterval, 1);
    held = tracker;
    if (held != nullptr) {
        sys::Increment(&held->refCount);
    }
    List_AddTail(static_cast<uint8_t*>(static_cast<void*>(self->visualTrackerClick)) + 0x4C, &held);
    Unref(tracker);
    return 1;
}

// 0x82AE50: GetInputSystemCreateParams (slot 37): DirectInput's, with the renderer, the
// keyboard non-exclusive, the mouse exclusive when asked, the window and instance (a failed
// allocation written through all the same)
NiDI8CreateParams* __fastcall GetInputSystemCreateParams(NiApplication* self, re::Edx) {
    void* memory = NiNew(0x7C);
    NiDI8CreateParams* params = memory != nullptr ? DI8CreateParams_Construct(memory) : nullptr;
    Assign(params->renderer, self->renderer);
    params->keyboardFlags = 0x90000000;
    params->gamePadFlags = 2;
    params->axisLow = -100;
    params->axisHigh = 100;
    params->mouseFlags = self->exclusiveMouse != 0 ? 0x50000000 : 0x90000000;
    params->instance = ms_pInstance;
    params->window = self->appWindow->window;
    return params;
}

// 0x82AF00: ProcessInput (slot 6): the trackers' input; Escape quits, F2 configures the devices;
// a pad's buttons 14 and 15 together quit
void __fastcall ProcessInput(NiApplication* self, re::Edx) {
    Virtual<void>(self, 7);
    const uint8_t* keyboard = At<const uint8_t*>(self->inputSystem, 0x40);
    if (keyboard != nullptr) {
        if ((keyboard[0x21] & 2) != 0 && (keyboard[0x41] & 2) == 0) {
            Import<int>(kPostMessageA, self->appWindow->window, 2u, 0u, 0);
        } else if ((keyboard[0x28] & 0x10) != 0 && (keyboard[0x48] & 0x10) == 0) {
            Virtual<uint32_t>(self, 38);
        }
    }
    for (uint32_t i = 0, offset = 0x48; offset < 0x68; offset += 4, i++) {
        if (i >= 8) {
            continue;
        }
        void* pad = At<void*>(self->inputSystem, int(offset));
        if (pad != nullptr && GamePad_ButtonIsDown(pad, 0xE) && GamePad_ButtonIsDown(pad, 0xF)) {
            Import<int>(kPostMessageA, self->appWindow->window, 2u, 0u, 0);
        }
    }
}

// 0x82AFB0: OnWindowResize(width, height, size type, window) (slot 27): the status bar resized and
// its panes spread evenly; the render window fitted above it; the window's size kept
uint8_t __fastcall OnWindowResize(NiApplication* self, re::Edx, int32_t width, int32_t height, uint32_t sizeType,
                                  void*) {
    NiAppWindow* window = self->appWindow;
    window->width = uint32_t(width);
    window->height = uint32_t(height);
    window = self->appWindow;
    uint32_t panes = window->numStatusPanes;
    RECT rect;
    if (panes != 0 && window->statusPanes) {
        void* status = StatusWindow(window);
        uint32_t size = (uint32_t(uint16_t(height)) << 16) | uint16_t(width);
        Import<intptr_t>(kSendMessageA, status, 5u, uintptr_t(sizeType), intptr_t(size));
        Import<int>(kGetClientRect, self->appWindow->window, &rect);
        auto* widths = static_cast<int32_t*>(NiMalloc_(panes * 4));
        for (uint32_t i = 0; i < panes - 1; i++) {
            widths[i] = int32_t(uint32_t((i + 1) * uint32_t(rect.right)) / panes);
        }
        widths[panes - 1] = rect.right;
        Import<intptr_t>(kSendMessageA, status, 0x404u, uintptr_t(panes), reinterpret_cast<intptr_t>(widths));
        Import<intptr_t>(kSendMessageA, status, 0xFu, uintptr_t(0), intptr_t(0));
        NiFree(widths);
    }
    window = self->appWindow;
    void* renderWindow = window->renderWindow;
    if (renderWindow == nullptr) {
        return 1;
    }
    RECT client;
    Import<int>(kGetClientRect, window->window, &client);
    int32_t clientWidth = client.right - client.left;
    int32_t clientHeight = client.bottom - client.top;
    if (self->appWindow->statusPanes) {
        RECT statusRect;
        Import<int>(kGetWindowRect, StatusWindow(self->appWindow), &statusRect);
        clientHeight += statusRect.top - statusRect.bottom;
    }
    Import<int>(kMoveWindow, renderWindow, 0, 0, clientWidth, clientHeight, 0);
    if (self->appWindow->statusPanes) {
        Import<intptr_t>(kSendMessageA, StatusWindow(self->appWindow), 0xFu, uintptr_t(0), intptr_t(0));
    }
    Import<int>(kGetWindowRect, renderWindow, &rect);
    self->appWindow->parentWidth = uint32_t(rect.right - rect.left);
    self->appWindow->parentHeight = uint32_t(rect.bottom - rect.top);
    return 1;
}

// 0x82B1E0: the deleting destructor
NiApplication* __fastcall DeletingDestructor(NiApplication* self, re::Edx, uint32_t flags) {
    NiApplication_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xFC);
    }
    return self;
}

// a click holding a list of references (the screen texture and visual tracker clicks, 0x64) as the
// constructor makes it
NiObject* NewListClick(uintptr_t vtable) {
    void* memory = NiNew(0x64);
    if (memory == nullptr) {
        return nullptr;
    }
    RenderClick_Construct(memory);
    At<uint32_t>(memory, 0) = vtable;
    At<uint32_t>(memory, 0x58) = 0;
    At<uint32_t>(memory, 0x50) = 0;
    At<uint32_t>(memory, 0x54) = 0;
    At<uint32_t>(memory, 0x4C) = NiTPointerList_vtable;
    At<float>(memory, 0x60) = 0.0f;
    At<uint32_t>(memory, 0x5C) = 0;
    return static_cast<NiObject*>(memory);
}

// 0x82B280: NiApplication(caption, width, height, use frame system, menu, status panes, bit
// depth): fullscreen when a bit depth is given; the developer image converter installed; the
// render frame's names; the screen elements' view, the screen texture and tracker clicks and the
// culler made
NiApplication* __fastcall Construct(NiApplication* self, re::Edx, const char* caption, uint32_t width, uint32_t height,
                                    int useFrameSystem, uint32_t menuID, uint32_t numStatusPanes,
                                    uint32_t bitDepth) {
    self->vtable = reinterpret_cast<void*>(NiApplication_vtable);
    self->scene = nullptr;
    self->camera = nullptr;
    self->frame = nullptr;
    self->mainSceneView = nullptr;
    self->screenElementsView = nullptr;
    self->screenTextureClick = nullptr;
    self->visualTrackerClick = nullptr;
    self->accumulatorProcessor = nullptr;
    self->renderer = nullptr;
    self->inputSystem = nullptr;
    self->visible = nullptr;
    self->visibleCount = 0;
    self->visibleAllocated = 0;
    self->visibleGrowBy = 0x400;
    VisibleArray_SetAllocatedSize(&self->visible, 0x400);
    self->culler = nullptr;
    for (int i = 0; i < 10; i++) {
        self->names[i] = scene::NullFixedString;
    }
    self->fixedTimeIncrement = kOneSixtieth;
    self->appEndTime = kAppEndTime;
    self->useFrameSystem = static_cast<uint8_t>(useFrameSystem);
    self->useFixedTime = 0;
    self->screenshotPeriod = kInvalidTime;
    self->screenshots = 0;
    self->lastScreenshotTime = kInvalidTime;
    self->screenshotCount = 0;
    void* memory = NiNew(0x30);
    self->appWindow = memory != nullptr ? AppWindow_Construct(memory, caption, width, height, numStatusPanes, 0xCA0000)
                                        : nullptr;
    self->lastTime = kNotYet;
    self->menuID = menuID;
    self->accumTime = 0.0f;
    self->currentTime = 0.0f;
    self->frameTime = 0.0f;
    self->frameRateValue = 0.0f;
    self->lastFrameRateTime = 0.0f;
    self->bitDepth = bitDepth;
    self->minFramePeriod = kMinFramePeriod;
    self->clicks = 0;
    self->timer = 1;
    self->maxTimer = 0x1E;
    self->lastFrame = 0.0f;
    self->lastFrameRateClicks = 0;
    self->cullTime = 0.0f;
    self->numObjectsDrawn = 0;
    self->renderTime = 0.0f;
    self->fullscreen = bitDepth != 0 && !re::ForceWindowed() ? 1 : 0;
    self->updateTime = 0.0f;
    self->stencil = 0;
    self->beginUpdate = 0.0f;
    self->rendererDialog = 1;
    self->beginCull = 0.0f;
    self->multiThread = 0;
    self->beginRender = 0.0f;
    self->frameRate = nullptr;
    self->frameRateEnabled = 0;
    struct OSVERSIONINFOA {
        uint32_t size;
        uint8_t rest[0x90];
    } version;
    version.size = 0x94;
    Import<int>(kGetVersionExA, &version);   // (the result isn't used)
    memory = NiNew(0x658);
    ImageConverter_SetImageConverter(memory != nullptr ? DevImageConverter_Construct(memory) : nullptr);
    ms_bRendererCreated = 1;
    ms_pkApplication = self;
    FixedString_AssignText(&self->names[0], "NiApplication Render Frame");
    FixedString_AssignText(&self->names[1], "NiApplication Shadow Render Step");
    FixedString_AssignText(&self->names[2], "NiApplication Main Scene Render Step");
    FixedString_AssignText(&self->names[3], "NiApplication Main Scene Render Click");
    FixedString_AssignText(&self->names[4], "NiApplication Main Scene Render View");
    FixedString_AssignText(&self->names[5], "NiApplication Screen Space Render Step");
    FixedString_AssignText(&self->names[6], "NiApplication Screen Elements Render Click");
    FixedString_AssignText(&self->names[7], "NiApplication Screen Elements Render View");
    FixedString_AssignText(&self->names[8], "NiApplication Screen Texture Render Click");
    FixedString_AssignText(&self->names[9], "NiApplication Visual Tracker Render Click");
    self->vtPerformanceMax = kHundred;
    self->d3d10 = 0;
    self->vtTimeMax = kTenThousand;
    self->refRast = 0;
    self->swVertex = 0;
    self->vtFrameRateMax = kHundred;
    self->exclusiveMouse = 0;
    self->nvPerfHUD = 0;
    self->showAllTrackers = 0;

    memory = NiNew(0x30);
    NiObject* view = nullptr;
    if (memory != nullptr) {
        RenderView_Construct(memory);
        At<uint32_t>(memory, 0) = Ni2DRenderView_vtable;
        At<uint32_t>(memory, 0x2C) = 0;
        At<uint32_t>(memory, 0x24) = 0;
        At<uint32_t>(memory, 0x28) = 0;
        At<uint32_t>(memory, 0x20) = NiTPointerList_vtable;   // its screen elements' list
        view = static_cast<NiObject*>(memory);
    }
    Assign(self->screenElementsView, view);
    scene::FixedString_Assign(&At<NiFixedString>(self->screenElementsView, 0x1C), &self->names[7]);

    Assign(self->screenTextureClick, NewListClick(NiScreenTextureRenderClick_vtable));
    scene::FixedString_Assign(&At<NiFixedString>(self->screenTextureClick, 0x48), &self->names[8]);
    Assign(self->visualTrackerClick, NewListClick(NiVisualTrackerRenderClick_vtable));
    scene::FixedString_Assign(&At<NiFixedString>(self->visualTrackerClick, 0x48), &self->names[9]);

    memory = NiNew(0x94);
    NiObject* culler = memory != nullptr ? CullingProcess_Construct(memory, &self->visible) : nullptr;
    Assign(self->culler, culler);
    return self;
}

// ---- the render view and clicks the application uses

// 0x82B210
const RTTI* __fastcall Ni2DRenderView_GetRTTI(void*, re::Edx) {
    return &Ni2DRenderView_RTTI;
}

// 0x82B220: Ni2DRenderView's deleting destructor (its list emptied, then ~NiRenderView)
void* __fastcall Ni2DRenderView_DeletingDestructor(void* self, re::Edx, uint32_t flags) {
    void* list = static_cast<uint8_t*>(self) + 0x20;
    At<uint32_t>(list, 0) = NiTPointerList_vtable;
    List_RemoveAll(list);
    At<uint32_t>(list, 0) = NiTPointerListBase_vtable;
    RenderView_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x30);
    }
    return self;
}

// 0x82B260
const RTTI* __fastcall NiScreenTextureRenderClick_GetRTTI(void*, re::Edx) {
    return &NiScreenTextureRenderClick_RTTI;
}

// 0x82B270
const RTTI* __fastcall NiVisualTrackerRenderClick_GetRTTI(void*, re::Edx) {
    return &NiVisualTrackerRenderClick_RTTI;
}

}   // namespace
}   // namespace ni::app

// ---- registration ---------------------------------------------------------------------

#define C33_APP2_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_APP2_REVERSED("NiInputSystem::CreateParams::GetRTTI", 0x0082A680, &ni::app::CreateParams_GetRTTI);
C33_APP2_REVERSED("NiDI8InputSystem::DI8CreateParams::DI8CreateParams", 0x0082A690, &ni::app::DI8Params_Construct);
C33_APP2_REVERSED("NiDI8InputSystem::DI8CreateParams::GetRTTI", 0x0082A710, &ni::app::DI8Params_GetRTTI);
C33_APP2_REVERSED("NiDI8InputSystem::DI8CreateParams::~DI8CreateParams", 0x0082A720, &ni::app::DI8Params_Destruct);
C33_APP2_REVERSED("NiApplication::~NiApplication", 0x0082A780, &ni::app::Destruct);
C33_APP2_REVERSED("NiApplication::CreateVisualTrackers", 0x0082AB20, &ni::app::CreateVisualTrackers);
C33_APP2_REVERSED("NiApplication::GetInputSystemCreateParams", 0x0082AE50, &ni::app::GetInputSystemCreateParams);
C33_APP2_REVERSED("NiApplication::ProcessInput", 0x0082AF00, &ni::app::ProcessInput);
C33_APP2_REVERSED("NiApplication::OnWindowResize", 0x0082AFB0, &ni::app::OnWindowResize);
C33_APP2_REVERSED("NiInputSystem::CreateParams::DeletingDestructor", 0x0082B170,
                  &ni::app::CreateParams_DeletingDestructor);
C33_APP2_REVERSED("NiDI8InputSystem::DI8CreateParams::DeletingDestructor", 0x0082B1C0,
                  &ni::app::DI8Params_DeletingDestructor);
C33_APP2_REVERSED("NiApplication::DeletingDestructor", 0x0082B1E0, &ni::app::DeletingDestructor);
C33_APP2_REVERSED("Ni2DRenderView::GetRTTI", 0x0082B210, &ni::app::Ni2DRenderView_GetRTTI);
C33_APP2_REVERSED("Ni2DRenderView::DeletingDestructor", 0x0082B220, &ni::app::Ni2DRenderView_DeletingDestructor);
C33_APP2_REVERSED("NiScreenTextureRenderClick::GetRTTI", 0x0082B260, &ni::app::NiScreenTextureRenderClick_GetRTTI);
C33_APP2_REVERSED("NiVisualTrackerRenderClick::GetRTTI", 0x0082B270, &ni::app::NiVisualTrackerRenderClick_GetRTTI);
C33_APP2_REVERSED("NiApplication::NiApplication", 0x0082B280, &ni::app::Construct);
