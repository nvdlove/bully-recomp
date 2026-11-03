// NiApplication's frame (src/gamebryo/NiApplication.h): the per-frame virtuals (OnIdle's timing,
// update, cull, render, display, screenshots, the visual trackers), the creation virtuals
// Initialize calls (scene, camera, render frame, renderer, input system), the window procedure,
// the message pump and the command line's flags; NiAccumulatorProcessor (the main scene's render
// list processor) and the small helpers of other classes that sit among them.
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's order.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiApplication.h"
#include "gamebryo/NiCamera.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::app {
namespace {

using sys::At;
using sys::Import;
using sys::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiAccumulatorProcessor_RTTI, 0x020E9554);
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(void*, NiShadowManager_ms_pkShadowManager, 0x020E2410);
GAME_VAR(float, kNotYet, 0x009009AC);           // -1
GAME_VAR(float, kOneSixtieth, 0x009144C4);      // 1/60
GAME_VAR(float, kTestEndTime, 0x00900D38);      // 10
GAME_VAR(float, kTestScreenshotPeriod, 0x00900BFC);   // 2
GAME_VAR(float, kFarPlane, 0x00900F40);         // 1000
GAME_VAR(double, kHalfFieldOfView, 0x00968820); // 10 degrees in radians
GAME_VAR(float, kBackgroundGrey, 0x009055A0);   // 0.5

using MessageBoxFn = void(__cdecl*)(const char* text, const char* caption, uint32_t flags);
GAME_VAR(MessageBoxFn, NiMessageBoxFunction, 0x00BC74FC);

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiAccumulatorProcessor_vtable = 0x009687A8;
constexpr uintptr_t NiDefaultClickRenderStep_vtable = 0x008FF5B4;
constexpr uintptr_t NiTPointerList_RenderClick_vtable = 0x008FF508;
constexpr float kMaxFloat = 3.40282347e+38f;
constexpr float kInvalidFloat = -3.40282347e+38f;
constexpr double kInvalidDouble = -3.4028234663852886e+38;   // (0x953E90: -FLT_MAX as a double)

// Win32 through the game's import slots
constexpr uintptr_t kGetVersionExA = 0x008FC0E8;
constexpr uintptr_t kTranslateMessage = 0x008FC210;
constexpr uintptr_t kDispatchMessageA = 0x008FC214;
constexpr uintptr_t kPeekMessageA = 0x008FC218;
constexpr uintptr_t kShowCursor = 0x008FC244;
constexpr uintptr_t kPostQuitMessage = 0x008FC248;
constexpr uintptr_t kDefWindowProcA = 0x008FC24C;
constexpr uintptr_t kPostMessageA = 0x008FC250;
constexpr uintptr_t kTranslateAcceleratorA = 0x008FC260;

struct MSG {
    re::Ptr32<void> hwnd;
    uint32_t message;
    uint32_t wParam;
    int32_t lParam;
    uint32_t time;
    int32_t ptX;
    int32_t ptY;
};
static_assert(sizeof(MSG) == 0x1C);

GAME_FN(float, __cdecl, NiGetCurrentTimeInSec, 0x007537B0);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __cdecl, NiPath_Standardize, 0x00752C20, char* path);
GAME_FN(void, __cdecl, SetShaderDirectory, 0x00762A40, const char* path);
GAME_FN(const char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);   // (a native return: a Ptr32 by value is returned through memory on x64)
GAME_FN(void, __cdecl, ShadowManager_SetActiveShadowClickGenerator, 0x00757DA0, const re::Ptr32<const char>* name);
GAME_FN(void, __cdecl, ShadowManager_SetSceneCamera, 0x00757DF0, void* camera);
GAME_FN(void, __cdecl, ShadowManager_SetCullingProcess, 0x00757E90, void* culler);
GAME_FN(uint32_t, __cdecl, ShadowManager_Initialize, 0x00759570);
GAME_FN(void, __cdecl, ShadowManager_Shutdown, 0x00759930);
GAME_FN(void*, __cdecl, ShadowManager_GenerateRenderClicks, 0x007572F0);   // the shadow clicks' list
GAME_FN(void, __thiscall, List_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, List_AddTail, 0x007843D0, void* list, const re::Ptr32<NiObject>* element);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, List_Remove, 0x0075E6B0, void* list, re::Ptr32<NiObject>* result, const re::Ptr32<NiObject>* element);
GAME_FN(void, __thiscall, ClickRenderStep_Construct, 0x007650D0, void* self);
GAME_FN(NiObject*, __thiscall, Ni3DRenderView_Construct, 0x007561A0, void* self, void* camera, void* culler,
        int alwaysUseCameraViewport);
GAME_FN(NiObject*, __thiscall, ViewRenderClick_Construct, 0x00518D00, void* self);
GAME_FN(void, __thiscall, ViewRenderClick_SetProcessor, 0x007CF610, void* self, void* processor, void* data);
GAME_FN(NiObject*, __thiscall, RenderFrame_Construct, 0x00404510, void* self);
GAME_FN(void, __thiscall, RenderFrame_Draw, 0x007652F0, void* self);
GAME_FN(void, __thiscall, RenderFrame_GatherStatistics, 0x00402B90, void* self, uint32_t* numObjects, float* cullTime,
        float* renderTime);
GAME_FN(void, __thiscall, RenderFrame_Display, 0x007653D0, void* self);
GAME_FN(NiNode*, __thiscall, Node_Construct, 0x00760780, void* self, uint32_t childCount);
GAME_FN(NiObject*, __thiscall, Camera_Construct, 0x00763C20, void* self);
GAME_FN(void, __thiscall, Camera_SetViewFrustum, 0x00762D60, void* self, const NiFrustum* frustum);
GAME_FN(NiFrustum*, __thiscall, Frustum_Construct, 0x007718A0, NiFrustum* self, float left, float right, float top,
        float bottom, float nearDist, float farDist, int ortho);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, void* self);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E240, void* self);
GAME_FN(void, __cdecl, NiCullScene, 0x008C5760, void* camera, void* scene, void* culler, void* visible, int reset);
GAME_FN(void, __cdecl, NiDrawVisibleArray, 0x008C5850, void* camera, void* visible);
GAME_FN(uint8_t, __thiscall, Renderer_BeginUsingRenderTargetGroup, 0x00407890, void* self, void* group, uint32_t clear);
GAME_FN(uint8_t, __thiscall, Renderer_EndUsingRenderTargetGroup, 0x00407990, void* self);
GAME_FN(void, __thiscall, ScreenElements_Draw, 0x007AB310, void* self, void* renderer);
GAME_FN(void, __thiscall, ScreenTexture_Draw, 0x007DB6C0, void* self, void* renderer);
GAME_FN(NiObject*, __cdecl, InputSystem_Create, 0x008D2B40, void* params);
GAME_FN(uint8_t, __thiscall, GamePad_ButtonIsDown, 0x008D0AF0, void* self, uint32_t button);
GAME_FN(uint8_t, __thiscall, GamePad_ButtonWasPressed, 0x008D0B50, void* self, uint32_t button);
GAME_FN(NiObject*, __thiscall, Command_Construct, 0x0082BCD0, void* self, const char* commandLine);
GAME_FN(uint32_t, __thiscall, Command_Boolean, 0x0082BE90, NiCommand* self, const char* name);
GAME_FN(void, __thiscall, AppWindow_SetStatusPaneText, 0x0082B950, NiAppWindow* self, uint32_t pane, const char* text);
GAME_FN(void, __thiscall, FrameRate_TakeSample, 0x0082BEB0, NiFrameRate* self);
GAME_FN(void, __thiscall, FrameRate_Update, 0x0082C040, NiFrameRate* self);
GAME_FN(void, __thiscall, FrameRate_Shutdown, 0x0082C240, NiFrameRate* self);
GAME_FN(void, __thiscall, FrameRate_Destruct, 0x0082C280, NiFrameRate* self);
GAME_FN(void, __thiscall, VisualTracker_SetShow, 0x0082F170, void* self, int show);
GAME_FN(void, __thiscall, VisualTracker_Draw, 0x0082F2F0, void* self);
GAME_FN(void, __thiscall, VisualTracker_Update, 0x0082F7B0, void* self);
GAME_FN(re::Ptr32<NiObject>*, __cdecl, CreateDX9Renderer, 0x0082D420, re::Ptr32<NiObject>* result, void* window, void* renderWindow,
        int dialog, uint32_t bitDepth, uint32_t width, uint32_t height, int stencil, int multiThread, int refRast,
        int swVertex, uint8_t* nvPerfHUD, uint8_t* fullscreen);
GAME_FN(re::Ptr32<NiObject>*, __cdecl, CreateD3D10Renderer, 0x0082EF00, re::Ptr32<NiObject>* result, void* window, int dialog,
        uint32_t width, uint32_t height, int multiThread, int refRast, uint8_t* fullscreen);
// (this file's, called where modes apply)
GAME_FN(const char*, __cdecl, NiApplication_ConvertMediaFilename, 0x00829900, const char* filename);
GAME_FN(void, __thiscall, NiApplication_QuitApplication, 0x0082A5E0, NiApplication* self);
GAME_FN(void, __thiscall, NiApplication_SetShowAllTrackers, 0x008293A0, NiApplication* self, int show);
GAME_FN(void, __thiscall, Ni3DRenderView_SetCamera, 0x00829200, void* self, NiObject* camera);
GAME_FN(void, __thiscall, AccumulatorProcessor_SetAccumulator, 0x00829350, void* self, NiObject* accumulator);
GAME_FN(NiAccumulatorProcessor*, __thiscall, AccumulatorProcessor_Construct, 0x00829250, void* self,
        NiObject* accumulator, int unknown);
GAME_FN(void, __thiscall, AccumulatorProcessor_Destruct, 0x00829300, void* self);
GAME_FN(uint8_t, __cdecl, ShadowRenderStepPre, 0x00829710, void* step, void* data);

// a reference let go through the import (DeleteThis when it was the last), and held
inline void Unref(NiRefObject* object) {
    if (object != nullptr && sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}
inline void Ref(NiRefObject* object) {
    if (object != nullptr) {
        sys::Increment(&object->refCount);
    }
}
// NiPointer::operator= as these functions have it: the old one let go first
template <typename T, typename U>
inline void Assign(re::Ptr32<T>& slot, U* value) {
    T* v = reinterpret_cast<T*>(value);
    if (slot != v) {
        Unref(reinterpret_cast<NiRefObject*>(slot));
        slot = v;
        Ref(reinterpret_cast<NiRefObject*>(v));
    }
}
// a member reference let go and cleared (NiPointer = 0)
template <typename T>
inline void Clear(re::Ptr32<T>& slot) {
    if (slot != nullptr) {
        Unref(re::PtrCast<NiRefObject*>(slot));
        slot = nullptr;
    }
}

// an element added to a NiTPointerList of references (held for the call: a NiPointer passed)
inline void AddTailHeld(void* list, NiObject* element, bool checkNull = true) {
    re::Ptr32<NiObject> held = element;
    if (!checkNull || held != nullptr) {
        sys::Increment(&held->refCount);
    }
    List_AddTail(list, &held);
    if (!checkNull || held != nullptr) {
        if (sys::Decrement(&element->refCount) == 0) {
            Virtual<void>(element, 1);
        }
    }
}

// a NiDefaultClickRenderStep (0x3C) as the code constructs it inline
NiObject* NewClickRenderStep() {
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        return nullptr;
    }
    ClickRenderStep_Construct(memory);
    At<uint32_t>(memory, 0) = NiDefaultClickRenderStep_vtable;
    At<uint32_t>(memory, 0x38) = 0;
    At<uint32_t>(memory, 0x30) = 0;
    At<uint32_t>(memory, 0x34) = 0;
    At<uint32_t>(memory, 0x2C) = NiTPointerList_RenderClick_vtable;   // its clicks' list
    return static_cast<NiObject*>(memory);
}

inline void SetName(void* object, int offset, const re::Ptr32<const char>* name) {
    scene::FixedString_Assign(&At<NiFixedString>(object, offset), name);
}

// the renderer's frame state (+0x94: 0 outside a frame, 1 in one, 2/4 ..., 3 ended)
inline uint32_t& FrameState(NiObject* renderer) {
    return At<uint32_t>(renderer, 0x94);
}

// a list's elements walked (the NiTPointerList head at +offset of the object)
template <typename F>
inline void ForEach(void* object, int headOffset, F f) {
    ListNode* node = At<ListNode*>(object, headOffset);
    while (node != nullptr) {
        void* element = node->element;
        node = node->next;
        f(element);
    }
}

// ---- statics

// 0x828B00: SetCommandLine(command line): the command object made from it (none when empty)
void __cdecl SetCommandLine(const char* commandLine) {
    if (commandLine == nullptr) {
        return;
    }
    size_t length = 0;
    while (commandLine[length] != '\0') {
        length++;
    }
    if (length == 0) {
        return;
    }
    void* memory = NiNew(0x20);
    if (memory != nullptr) {
        ms_pkCommand = reinterpret_cast<NiCommand*>(Command_Construct(memory, commandLine));
        return;
    }
    ms_pkCommand = nullptr;
}

// 0x829900: ConvertMediaFilename(filename): the media path and the filename, standardized, in
// the static buffer
const char* __cdecl ConvertMediaFilename(const char* filename) {
    NiSprintf(ms_acTempMediaFilename, 0x104, "%s%s", ms_acMediaPath, filename);
    NiPath_Standardize(ms_acTempMediaFilename);
    return ms_acTempMediaFilename;
}

// 0x829EE0: ParseCommandLine (a guess): the renderer and test options read from the command line
void __cdecl ParseCommandLine() {
    if (ms_pkCommand == nullptr) {
        return;
    }
    if (Command_Boolean(ms_pkCommand, "d3d10") != 0) {
        ms_pkApplication->d3d10 = 1;
    }
    if (Command_Boolean(ms_pkCommand, "full") != 0) {
        ms_pkApplication->fullscreen = 1;
        ms_pkApplication->rendererDialog = 0;
    }
    if (Command_Boolean(ms_pkCommand, "refrast") != 0) {
        ms_pkApplication->refRast = 1;
    } else if (Command_Boolean(ms_pkCommand, "swvertex") != 0) {
        ms_pkApplication->swVertex = 1;
    }
    if (Command_Boolean(ms_pkCommand, "nvperf") != 0) {
        ms_pkApplication->nvPerfHUD = 1;
    }
    if (Command_Boolean(ms_pkCommand, "test") != 0) {
        ms_pkApplication->useFixedTime = 1;
        ms_pkApplication->fixedTimeIncrement = kOneSixtieth;
        ms_pkApplication->appEndTime = kTestEndTime;
        ms_pkApplication->screenshots = 1;
        ms_pkApplication->screenshotPeriod = kTestScreenshotPeriod;
    }
}

// 0x82A000: ShowPointer
void __cdecl ShowPointer() {
    Import<int>(kShowCursor, 1);
}

// 0x82A010: the window procedure: to DefWindowProc without an application; WM_DESTROY:
// OnWindowDestroy, quit; WM_SIZE: OnWindowResize; anything else: OnDefault; handled is 0
intptr_t __stdcall WndProc(void* hwnd, uint32_t message, uintptr_t wParam, intptr_t lParam) {
    NiApplication* app = ms_pkApplication;
    if (app == nullptr) {
        return Import<intptr_t>(kDefWindowProcA, hwnd, message, wParam, lParam);
    }
    if (message == 2) {
        Virtual<uint8_t>(app, 28, 0, 0);
        Import<void>(kPostQuitMessage, 0);
        return 0;
    }
    uint8_t handled;
    if (message == 5) {
        handled = Virtual<uint8_t>(app, 27, uint32_t(uint16_t(lParam)), uint32_t(uint32_t(lParam) >> 16), wParam,
                                   static_cast<void*>(nullptr));
    } else {
        struct EventRecord {   // NiEventRecord (the game's layout: its handlers read +4 and +8)
            re::Ptr32<void> hwnd;
            uint32_t message;
            uint32_t wParam;
            int32_t lParam;
        } record = {hwnd, message, uint32_t(wParam), int32_t(lParam)};
        static_assert(sizeof(EventRecord) == 0x10);
        handled = Virtual<uint8_t>(app, 29, &record);
    }
    if (handled != 0) {
        return 0;
    }
    return Import<intptr_t>(kDefWindowProcA, hwnd, message, wParam, lParam);
}

// 0x82A5C0: SetMediaPath(path)
void __cdecl SetMediaPath(const char* path) {
    crt::StrcpyS(ms_acMediaPath, 0x104, path);
}

// 0x829710: the shadow render step's pre-processing callback: its clicks made the shadow
// manager's
uint8_t __cdecl ShadowRenderStepPreImpl(void* step, void*) {
    void* clicks = ShadowManager_GenerateRenderClicks();
    void* list = static_cast<uint8_t*>(step) + 0x2C;
    List_RemoveAll(list);
    ListNode* node = At<ListNode*>(clicks, 4);
    while (node != nullptr) {
        auto* click = static_cast<NiObject*>(node->element);
        node = node->next;
        AddTailHeld(list, click);
    }
    return 1;
}

// ---- NiApplication's virtuals

// 0x829820: EnableFrameRate(enable) (slot 1): the frame-rate digits put in the screen elements'
// view or taken out of it
void __fastcall EnableFrameRate(NiApplication* self, re::Edx, int enable) {
    NiFrameRate* frameRate = self->frameRate;
    if (frameRate == nullptr) {
        return;
    }
    uint8_t on = static_cast<uint8_t>(enable);
    if (self->frameRateEnabled != on) {
        void* list = static_cast<uint8_t*>(static_cast<void*>(self->screenElementsView)) + 0x20;
        if (on != 0) {
            AddTailHeld(list, frameRate->element);
        } else {
            re::Ptr32<NiObject> element = frameRate->element;
            Ref(element);
            re::Ptr32<NiObject> removed;
            List_Remove(list, &removed, &element);
            Unref(removed);
            Unref(element);
        }
    }
    self->frameRateEnabled = on;
}

// 0x828B50: MeasureTime (slot 39): the clock moved on (by the fixed increment when testing);
// false while the frame comes sooner than the minimum period (or, under NVPerfHUD, no time passed:
// true then, nothing moved)
uint8_t __fastcall MeasureTime(NiApplication* self, re::Edx) {
    if (self->useFixedTime) {
        self->currentTime = self->fixedTimeIncrement + self->currentTime;
        float accum = float(self->clicks) * self->fixedTimeIncrement;
        self->accumTime = accum;
        self->frameTime = accum - self->lastFrame;
        self->lastFrame = accum;
        return 1;
    }
    if (kNotYet == self->lastTime) {
        self->lastTime = NiGetCurrentTimeInSec();
        self->clicks = 0;
        self->accumTime = 0.0f;
    }
    float now = NiGetCurrentTimeInSec();
    self->currentTime = now;
    float delta = now - self->lastTime;
    if (self->nvPerfHUD && delta == 0.0f) {
        return 1;
    }
    if (delta < 0.0f) {
        delta = 0.0f;
    }
    self->lastTime = now;
    float accum = delta + self->accumTime;
    self->accumTime = accum;
    if (self->minFramePeriod + self->lastFrame > accum) {
        return 0;
    }
    self->frameTime = accum - self->lastFrame;
    self->lastFrame = self->accumTime;
    return 1;
}

// 0x828C60: OnIdle (slot 8): a frame when MeasureTime says so: timings reset, update, begin
// frame, render, end frame, display, the trackers and metrics; then quit once past the end time
void __fastcall OnIdle(NiApplication* self, re::Edx) {
    if (!Virtual<uint8_t>(self, 39)) {
        return;
    }
    Virtual<void>(self, 9);
    Virtual<void>(self, 11);
    Virtual<void>(self, 12);
    Virtual<void>(self, 13);
    Virtual<void>(self, 10);
    Virtual<void>(self, 18);
    Virtual<void>(self, 22);
    Virtual<void>(self, 24);
    Virtual<void>(self, 25);
    Virtual<void>(self, 26);
    self->clicks++;
    if (self->accumTime >= self->appEndTime && self->appEndTime != kMaxFloat) {
        NiApplication_QuitApplication(self);
    }
}

// 0x828D10: ResetFrameTimings (slot 9)
void __fastcall ResetFrameTimings(NiApplication* self, re::Edx) {
    self->updateTime = 0.0f;
    self->cullTime = 0.0f;
    self->renderTime = 0.0f;
}

// 0x828D20: BeginUpdate (slot 11)
void __fastcall BeginUpdate(NiApplication* self, re::Edx) {
    self->beginUpdate = NiGetCurrentTimeInSec();
}

// 0x828D30: EndUpdate (slot 13)
void __fastcall EndUpdate(NiApplication* self, re::Edx) {
    float now = NiGetCurrentTimeInSec();
    self->updateTime = now - self->beginUpdate + self->updateTime;
    self->beginUpdate = 0.0f;
}

// 0x828D60: BeginCull (slot 14)
void __fastcall BeginCull(NiApplication* self, re::Edx) {
    self->beginCull = NiGetCurrentTimeInSec();
}

// 0x828D70: EndCull (slot 16)
void __fastcall EndCull(NiApplication* self, re::Edx) {
    float now = NiGetCurrentTimeInSec();
    self->cullTime = now - self->beginCull + self->cullTime;
    self->beginCull = 0.0f;
}

// 0x828DA0: BeginRender (slot 17)
void __fastcall BeginRender(NiApplication* self, re::Edx) {
    self->beginRender = NiGetCurrentTimeInSec();
}

// 0x828DB0: EndRender (slot 19)
void __fastcall EndRender(NiApplication* self, re::Edx) {
    float now = NiGetCurrentTimeInSec();
    self->renderTime = now - self->beginRender + self->renderTime;
    self->beginRender = 0.0f;
}

// 0x828DE0: CheckScreenshot (slot 23, a guess): a screenshot every screenshotPeriod seconds while
// they're on
void __fastcall CheckScreenshot(NiApplication* self, re::Edx) {
    if (!self->screenshots) {
        return;
    }
    if (kInvalidDouble == double(self->screenshotPeriod)) {
        return;
    }
    if (!(double(self->lastScreenshotTime) == kInvalidDouble)) {
        if (self->accumTime < self->lastScreenshotTime + self->screenshotPeriod) {
            return;
        }
    }
    Virtual<void>(self, 41);
    self->lastScreenshotTime = self->accumTime;
}

// 0x828E50: NiInputKeyboard::KeyWasPressed(key): down now and not before (key 0 never)
uint8_t __fastcall Keyboard_KeyWasPressed(const uint8_t* self, re::Edx, int32_t key) {
    if (key == 0) {
        return 0;
    }
    const uint8_t* bytes = self + (key >> 3);
    uint32_t bit = uint32_t(key) & 7;
    if (((bytes[0x21] >> bit) & 1) != 0 && ((bytes[0x41] >> bit) & 1) == 0) {
        return 1;
    }
    return 0;
}

// 0x828E90: CreateScene (slot 31): an empty node
uint8_t __fastcall CreateScene(NiApplication* self, re::Edx) {
    void* memory = NiNew(0xF4);
    NiNode* scene = memory != nullptr ? Node_Construct(memory, 0) : nullptr;
    Assign(self->scene, scene);
    return 1;
}

// 0x828EF0: ConfigureInputDevices (slot 38): the input system's ConfigureDevices (its slot 22)
uint32_t __fastcall ConfigureInputDevices(NiApplication* self, re::Edx) {
    return Virtual<uint32_t>(self->inputSystem, 22);
}

// 0x828F00: Terminate (slot 3): the frame rate deleted, everything let go (the shadow manager
// shut down when the frame system was used)
void __fastcall Terminate(NiApplication* self, re::Edx) {
    if (self->frameRate != nullptr) {
        FrameRate_Shutdown(self->frameRate);
        NiFrameRate* frameRate = self->frameRate;
        if (frameRate != nullptr) {
            FrameRate_Destruct(frameRate);
            NiDelete(frameRate, 0xB0);
        }
        self->frameRate = nullptr;
    }
    Clear(self->culler);
    Clear(self->mainSceneView);
    Clear(self->accumulatorProcessor);
    Clear(self->screenElementsView);
    Clear(self->screenTextureClick);
    Clear(self->visualTrackerClick);
    Clear(self->frame);
    Clear(self->inputSystem);
    Clear(self->scene);
    Clear(self->camera);
    if (self->useFrameSystem) {
        ShadowManager_Shutdown();
    }
    Clear(self->renderer);
}

// 0x829090: UpdateInput (slot 5): the action map's update when mapped, else the devices'; the
// device changes handled when one was lost (0x80000006)
void __fastcall UpdateInput(NiApplication* self, re::Edx) {
    NiObject* input = self->inputSystem;
    if (input == nullptr) {
        return;
    }
    uint32_t result;
    if (At<uint32_t>(input, 0x10) == 1) {
        result = Virtual<uint32_t>(input, 10);   // UpdateActionMap
    } else {
        result = Virtual<uint32_t>(input, 9);    // UpdateAllDevices
    }
    if (result == 0x80000006) {
        Virtual<uint32_t>(self->inputSystem, 18);   // HandleDeviceChanges
    }
}

// 0x8290E0: UpdateFrame (slot 12): input, the frame-rate counter, the frame rate
void __fastcall UpdateFrame(NiApplication* self, re::Edx) {
    Virtual<void>(self, 5);
    Virtual<void>(self, 6);
    if (self->frameRateEnabled && self->frameRate != nullptr) {
        FrameRate_TakeSample(self->frameRate);
        FrameRate_Update(self->frameRate);
    }
    self->frameRateValue = 1.0f / self->frameTime;
}

// 0x829120: CullFrame (slot 15): the scene culled into the visible array
void __fastcall CullFrame(NiApplication* self, re::Edx) {
    Virtual<void>(self, 14);
    NiCullScene(self->camera, self->scene, self->culler, &self->visible, 1);
    Virtual<void>(self, 16);
}

// 0x829160: SaveScreenshot (slot 41): the renderer's screenshot in the format's file (counted
// when saved)
void __fastcall SaveScreenshot(NiApplication* self, re::Edx) {
    const char* extension = "";
    switch (ms_eScreenshotFormat) {
    case 0:
        extension = ".tga";
        break;
    case 1:
        extension = ".png";
        break;
    case 2:
        extension = ".jpg";
        break;
    default:
        break;
    }
    char filename[0x104];
    Virtual<void>(self, 42, filename, 0x104u, extension);
    if (Virtual<uint8_t>(self->renderer, 57, static_cast<const char*>(filename), ms_eScreenshotFormat)) {
        self->screenshotCount++;
    }
}

// 0x829200: Ni3DRenderView::SetCamera(camera)
void __fastcall View_SetCamera(void* self, re::Edx, NiObject* camera) {
    Assign(At<NiObject*>(self, 0x20), camera);
}

// ---- NiAccumulatorProcessor

// 0x829250: NiAccumulatorProcessor(accumulator, flag): its own camera made
NiAccumulatorProcessor* __fastcall Accum_Construct(NiAccumulatorProcessor* self, re::Edx, NiObject* accumulator,
                                                   int unknown) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiAccumulatorProcessor_vtable);
    self->accumulator = accumulator;
    Ref(accumulator);
    self->camera = nullptr;
    self->unknown10 = static_cast<uint8_t>(unknown);
    void* memory = NiNew(0x13C);
    NiObject* camera = memory != nullptr ? Camera_Construct(memory) : nullptr;
    Assign(self->camera, camera);
    return self;
}

// 0x8292F0
const RTTI* __fastcall Accum_GetRTTI(NiAccumulatorProcessor*, re::Edx) {
    return &NiAccumulatorProcessor_RTTI;
}

// 0x829300: ~NiAccumulatorProcessor
void __fastcall Accum_Destruct(NiAccumulatorProcessor* self, re::Edx) {
    Unref(self->camera);
    Unref(self->accumulator);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    sys::Decrement(&NiRefObject_ms_uiObjects);
}

// 0x829350: SetAccumulator(accumulator) (the same code as NiTexturingProperty::Map::SetTexture:
// the reference at +8 assigned; the game calls this address for both)
void __fastcall Accum_SetAccumulator(void* self, re::Edx, NiObject* accumulator) {
    Assign(At<NiObject*>(self, 8), accumulator);
}

// 0x8296F0: the deleting destructor
NiAccumulatorProcessor* __fastcall Accum_DeletingDestructor(NiAccumulatorProcessor* self, re::Edx, uint32_t flags) {
    AccumulatorProcessor_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x14);
    }
    return self;
}

// ---- NiApplication's virtuals (continued)

// 0x8293A0: SetShowAllTrackers(show): every visual tracker shown or hidden
void __fastcall SetShowAllTrackers(NiApplication* self, re::Edx, int show) {
    ForEach(self->visualTrackerClick, 0x50, [&](void* tracker) { VisualTracker_SetShow(tracker, show); });
    self->showAllTrackers = static_cast<uint8_t>(show);
}

// 0x8293E0: ProcessVisualTrackerInput (slot 7): key 0x55 or a pad's button 13 toggles the trackers
void __fastcall ProcessVisualTrackerInput(NiApplication* self, re::Edx) {
    const uint8_t* keyboard = At<const uint8_t*>(self->inputSystem, 0x40);
    if (keyboard != nullptr && (keyboard[0x2B] & 0x20) != 0 && (keyboard[0x4B] & 0x20) == 0) {
        NiApplication_SetShowAllTrackers(self, self->showAllTrackers == 0);
    }
    for (uint32_t i = 0, offset = 0x48; offset < 0x68; offset += 4, i++) {
        if (i >= 8) {
            continue;
        }
        void* pad = At<void*>(self->inputSystem, int(offset));
        if (pad != nullptr && GamePad_ButtonWasPressed(pad, 0xD)) {
            NiApplication_SetShowAllTrackers(self, self->showAllTrackers == 0);
        }
    }
}

// 0x829450: UpdateVisualTrackers (slot 25)
void __fastcall UpdateVisualTrackers(NiApplication* self, re::Edx) {
    ForEach(self->visualTrackerClick, 0x50, [](void* tracker) { VisualTracker_Update(tracker); });
}

// 0x829480: BeginFrame (slot 10): the renderer's frame begun (outside the frame system)
void __fastcall BeginFrame(NiApplication* self, re::Edx) {
    if (self->useFrameSystem) {
        return;
    }
    NiObject* renderer = self->renderer;
    if (FrameState(renderer) == 0 && Virtual<uint8_t>(renderer, 82)) {   // Do_BeginFrame
        FrameState(renderer) = 1;
    }
}

// 0x8294C0: RenderFrame (slot 18): the render frame drawn (with the accumulator, shadow and view
// cameras set) and its statistics gathered; or, without the frame system, the default target,
// cull, the visible array drawn, the screen items and trackers
void __fastcall RenderFrame(NiApplication* self, re::Edx) {
    if (self->useFrameSystem) {
        if (self->accumulatorProcessor != nullptr) {
            AccumulatorProcessor_SetAccumulator(self->accumulatorProcessor, At<NiObject*>(self->renderer, 8));
        }
        if (self->camera != nullptr) {
            if (NiShadowManager_ms_pkShadowManager != nullptr) {
                ShadowManager_SetSceneCamera(self->camera);
            }
            if (self->mainSceneView != nullptr) {
                Ni3DRenderView_SetCamera(self->mainSceneView, self->camera);
            }
        }
        RenderFrame_Draw(self->frame);
        RenderFrame_GatherStatistics(self->frame, &self->numObjectsDrawn, &self->cullTime, &self->renderTime);
        return;
    }
    NiObject* renderer = self->renderer;
    uint32_t state = FrameState(renderer);
    if ((state == 1 || state == 2 || state == 4) && (At<uint8_t>(renderer, 0x90) == 0 || state == 4)) {
        // (the 7 is pushed before the GetDefaultRenderTargetGroup call: BeginUsing's argument)
        void* group = Virtual<void*>(renderer, 29);
        Renderer_BeginUsingRenderTargetGroup(renderer, group, 7);
    }
    Virtual<void>(self, 15);
    Virtual<void>(self, 17);
    NiDrawVisibleArray(self->camera, &self->visible);
    Virtual<void>(self, 20);
    Virtual<void>(self, 19);
    Virtual<void>(self, 21);
    Renderer_EndUsingRenderTargetGroup(self->renderer);
}

// 0x8295B0: RenderScreenItems (slot 20): screen-space camera data reset (while drawing to a
// target), the screen elements' and screen textures' draws
void __fastcall RenderScreenItems(NiApplication* self, re::Edx) {
    NiObject* renderer = self->renderer;
    uint32_t state = FrameState(renderer);
    if ((state == 1 || state == 2 || state == 4) && (At<uint8_t>(renderer, 0x90) == 1 || state == 4)) {
        Virtual<void>(renderer, 87, static_cast<void*>(nullptr));   // SetScreenSpaceCameraData
    }
    ForEach(self->screenElementsView, 0x24,
            [&](void* element) { ScreenElements_Draw(element, self->renderer); });
    ForEach(self->screenTextureClick, 0x50,
            [&](void* texture) { ScreenTexture_Draw(texture, self->renderer); });
}

// 0x829630: RenderVisualTrackers (slot 21)
void __fastcall RenderVisualTrackers(NiApplication* self, re::Edx) {
    ForEach(self->visualTrackerClick, 0x50, [](void* tracker) { VisualTracker_Draw(tracker); });
}

// 0x829660: EndFrame (slot 22): the renderer's frame ended (3 when it did, else 0)
void __fastcall EndFrame(NiApplication* self, re::Edx) {
    if (self->useFrameSystem) {
        return;
    }
    NiObject* renderer = self->renderer;
    if (FrameState(renderer) == 1) {
        FrameState(renderer) = Virtual<uint8_t>(renderer, 83) != 0 ? 3 : 0;   // Do_EndFrame
    }
}

// 0x8296A0: DisplayFrame (slot 24): the screenshot check, then the render frame's display or the
// renderer's (its frame count moved on when shown)
void __fastcall DisplayFrame(NiApplication* self, re::Edx) {
    Virtual<void>(self, 23);
    if (self->useFrameSystem) {
        RenderFrame_Display(self->frame);
        return;
    }
    NiObject* renderer = self->renderer;
    if (FrameState(renderer) == 3) {
        uint8_t shown = Virtual<uint8_t>(renderer, 84);   // Do_DisplayFrame
        FrameState(renderer) = 0;
        if (shown) {
            At<uint32_t>(renderer, 0x9C)++;
        }
    }
}

// 0x829780: UpdateFrameRateText (slot 40, a guess): every maxTimer frames, the frame rate since
// the last time put in the status bar
void __fastcall UpdateFrameRateText(NiApplication* self, re::Edx) {
    if (--self->timer != 0) {
        return;
    }
    if (0.0f < self->accumTime) {
        if (self->lastFrameRateTime < self->accumTime) {
            int32_t frames = self->clicks - self->lastFrameRateClicks;
            self->frameRateValue = float(frames) / (self->accumTime - self->lastFrameRateTime);
        }
    } else {
        self->frameRateValue = 0.0f;
    }
    char text[0x20];
    NiSprintf(text, 0x20, "fps: %.1f", double(self->frameRateValue));
    AppWindow_SetStatusPaneText(self->appWindow, 0, text);
    self->lastFrameRateTime = self->accumTime;
    self->timer = self->maxTimer;
    self->lastFrameRateClicks = self->clicks;
}

// 0x829940: GetScreenshotFilename(buffer, size, extension) (slot 42)
void __fastcall GetScreenshotFilename(NiApplication* self, re::Edx, char* buffer, uint32_t size,
                                      const char* extension) {
    NiSprintf(buffer, size, "Screenshots/%s_%d%s", self->appWindow->caption, self->screenshotCount, extension);
}

// 0x829970: CreateFrame (slot 33): the default shadow click generator, the shadow render step
// (its clicks the shadow manager's), the main scene's view and click (the accumulator processor
// sorting it), the screen elements' click, the main and screen-space steps, and the render frame
// holding the three steps
uint8_t __fastcall CreateFrame(NiApplication* self, re::Edx) {
    NiFixedString generator = StringTable_AddString("NiDefaultShadowClickGenerator");
    ShadowManager_SetActiveShadowClickGenerator(&generator);
    scene::FixedStringRelease(&generator);
    ShadowManager_SetSceneCamera(self->camera);
    ShadowManager_SetCullingProcess(self->culler);

    NiObject* shadowStep = NewClickRenderStep();
    SetName(shadowStep, 0x18, &self->names[1]);
    At<void*>(shadowStep, 8) = reinterpret_cast<void*>(0x00829710);   // pre-processing callback
    At<uint32_t>(shadowStep, 0x10) = 0;

    void* memory = NiNew(0x3C);
    NiObject* view = memory != nullptr ? Ni3DRenderView_Construct(memory, self->camera, self->culler, 1) : nullptr;
    Assign(self->mainSceneView, view);
    SetName(self->mainSceneView, 0x1C, &self->names[4]);
    AddTailHeld(static_cast<uint8_t*>(static_cast<void*>(self->mainSceneView)) + 0x28, self->scene);

    memory = NiNew(0x80);
    NiObject* mainClick = memory != nullptr ? ViewRenderClick_Construct(memory) : nullptr;
    SetName(mainClick, 0x48, &self->names[3]);
    AddTailHeld(static_cast<uint8_t*>(static_cast<void*>(mainClick)) + 0x4C, self->mainSceneView);
    const NiCamera* camera = re::PtrCast<const NiCamera*>(self->camera);
    At<NiRectFloat>(mainClick, 0x14) = camera->port;
    At<uint32_t>(mainClick, 0x34) = 7;

    memory = NiNew(0x14);
    NiAccumulatorProcessor* processor = memory != nullptr ? AccumulatorProcessor_Construct(memory, nullptr, 1) : nullptr;
    Assign(self->accumulatorProcessor, processor);
    ViewRenderClick_SetProcessor(mainClick, self->accumulatorProcessor, nullptr);

    memory = NiNew(0x80);
    NiObject* screenClick = memory != nullptr ? ViewRenderClick_Construct(memory) : nullptr;
    SetName(screenClick, 0x48, &self->names[6]);
    AddTailHeld(static_cast<uint8_t*>(static_cast<void*>(screenClick)) + 0x4C, self->screenElementsView);

    NiObject* mainStep = NewClickRenderStep();
    SetName(mainStep, 0x18, &self->names[2]);
    AddTailHeld(static_cast<uint8_t*>(static_cast<void*>(mainStep)) + 0x2C, mainClick, false);

    NiObject* screenStep = NewClickRenderStep();
    SetName(screenStep, 0x18, &self->names[5]);
    void* screenClicks = static_cast<uint8_t*>(static_cast<void*>(screenStep)) + 0x2C;
    AddTailHeld(screenClicks, screenClick);
    AddTailHeld(screenClicks, self->screenTextureClick);
    AddTailHeld(screenClicks, self->visualTrackerClick);

    memory = NiNew(0x2C);
    NiObject* frame = memory != nullptr ? RenderFrame_Construct(memory) : nullptr;
    Assign(self->frame, frame);
    SetName(self->frame, 0x28, &self->names[0]);
    AddTailHeld(static_cast<uint8_t*>(static_cast<void*>(self->frame)) + 8, shadowStep, false);
    AddTailHeld(static_cast<uint8_t*>(static_cast<void*>(self->frame)) + 8, mainStep);
    AddTailHeld(static_cast<uint8_t*>(static_cast<void*>(self->frame)) + 8, screenStep);
    return 1;
}

// 0x82A0C0: CreateCamera (slot 32): a camera with a 20-degree vertical field of view at the back
// buffer's (or the window's) aspect, near 1, far 1000, the full viewport
uint8_t __fastcall CreateCamera(NiApplication* self, re::Edx) {
    void* memory = NiNew(0x13C);
    NiObject* camera = memory != nullptr ? Camera_Construct(memory) : nullptr;
    Assign(self->camera, camera);
    uint32_t width;
    uint32_t height;
    if (self->renderer != nullptr) {
        void* buffer = Virtual<void*>(self->renderer, 32);   // the default back buffer
        width = At<uint32_t>(buffer, 8);
        height = At<uint32_t>(buffer, 0xC);
    } else {
        width = self->appWindow->parentWidth;
        height = self->appWindow->parentHeight;
    }
    float aspect = float(double(width)) / float(double(height));
    float tanHalf = float(crt::Tan(kHalfFieldOfView));
    float right = tanHalf * aspect;
    NiFrustum frustum;
    Frustum_Construct(&frustum, -right, right, tanHalf, -tanHalf, 1.0f, kFarPlane, 0);
    NiRectFloat port = {0.0f, 1.0f, 1.0f, 0.0f};
    Camera_SetViewFrustum(self->camera, &frustum);
    re::PtrCast<NiCamera*>(self->camera)->port = port;
    return 1;
}

// 0x82A220: CreateRenderer (slot 34): the shader directory set, the D3D10 or DX9 renderer made
// (the selection dialogs when asked); an error box when it fails, else a grey background
uint8_t __fastcall CreateRenderer(NiApplication* self, re::Edx) {
    SetShaderDirectory(NiApplication_ConvertMediaFilename("Shaders\\Generated"));
    NiAppWindow* window = self->appWindow;
    uint32_t height = window->parentHeight;
    uint32_t width = window->parentWidth;
    re::Ptr32<NiObject> created = nullptr;
    re::Ptr32<NiObject>* result;
    if (self->d3d10) {
        result = CreateD3D10Renderer(&created, window->window, self->rendererDialog, width, height, self->multiThread,
                                     self->refRast, &self->fullscreen);
    } else {
        result = CreateDX9Renderer(&created, window->window, window->renderWindow, self->rendererDialog,
                                   self->bitDepth, width, height, self->stencil, self->multiThread, self->refRast,
                                   self->swVertex, &self->nvPerfHUD, &self->fullscreen);
    }
    Assign(self->renderer, *result);
    Unref(created);
    if (self->renderer == nullptr) {
        char caption[0x200];   // (filled, not used)
        if (self->d3d10) {
            crt::StrcpyS(caption, 0x200, "D3D10 Renderer Creation Failed");
            if (NiMessageBoxFunction != nullptr) {
                NiMessageBoxFunction("Unable to create a D3D10 Renderer.\nThis symptom may indicate you are not "
                                     "running on a D3D10-capable graphics card.\n",
                                     "D3D10 Renderer Creation Failed", 0);
            }
        } else {
            crt::StrcpyS(caption, 0x200, "DX9 Renderer Creation Failed");
            if (NiMessageBoxFunction != nullptr) {
                NiMessageBoxFunction("Unable to create a DX9 Renderer.\n", "DX9 Renderer Creation Failed", 0);
            }
        }
        return 0;
    }
    float grey[3] = {kBackgroundGrey, kBackgroundGrey, kBackgroundGrey};
    Virtual<void>(self->renderer, 23, grey);   // SetBackgroundColor
    return 1;
}

// 0x82A400: CreateInputSystem (slot 35): made from the create parameters (slot 37; deleted
// after), enumerated, the mouse, keyboard and eight pads opened
uint8_t __fastcall CreateInputSystem(NiApplication* self, re::Edx) {
    void* params = Virtual<void*>(self, 37);
    NiObject* input = InputSystem_Create(params);
    Assign(self->inputSystem, input);
    if (params != nullptr) {
        Virtual<void*>(params, 1, 1u);
    }
    if (self->inputSystem == nullptr) {
        if (NiMessageBoxFunction != nullptr) {
            NiMessageBoxFunction("CreateInputSystem: Creation failed.", "NiApplication Error", 0);
        }
        return 0;
    }
    uint32_t status = Virtual<uint32_t>(self->inputSystem, 21);   // CheckEnumerationStatus
    if (status == 0x80000030 || status == 0x80000032) {
        return 0;
    }
    if (Virtual<void*>(self->inputSystem, 7) != nullptr && Virtual<void*>(self->inputSystem, 8) != nullptr) {
        for (uint32_t i = 0; i < 8; i++) {
            Virtual<void*>(self->inputSystem, 6, i, 0u);   // OpenGamePad(index, port)
        }
        return 1;
    }
    if (NiMessageBoxFunction != nullptr) {
        NiMessageBoxFunction("CreateInputSystem: Mouse or keyboard failed to open.", "NiApplication Error", 0);
    }
    return 0;
}

// 0x82A500: Initialize (slot 2): renderer, camera, input, scene and (frame system) frame made,
// the scene and camera updated, the visual trackers made
uint8_t __fastcall Initialize(NiApplication* self, re::Edx) {
    if (self->useFrameSystem) {
        ShadowManager_Initialize();
    }
    if (!Virtual<uint8_t>(self, 34) || !Virtual<uint8_t>(self, 32) || !Virtual<uint8_t>(self, 35) ||
        !Virtual<uint8_t>(self, 31)) {
        return 0;
    }
    if (self->useFrameSystem && !Virtual<uint8_t>(self, 33)) {
        return 0;
    }
    if (self->scene != nullptr) {
        AVObject_Update(self->scene, 0.0f, 1);
        AVObject_UpdateProperties(self->scene);
        AVObject_UpdateEffects(self->scene);
    }
    if (self->camera != nullptr) {
        AVObject_Update(self->camera, 0.0f, 1);
    }
    return Virtual<uint8_t>(self, 36) != 0 ? 1 : 0;
}

// 0x82A5E0: QuitApplication: WM_DESTROY posted to the window
void __fastcall QuitApplication(NiApplication* self, re::Edx) {
    Import<int>(kPostMessageA, self->appWindow->window, 2u, 0u, 0);
}

// 0x82A600: Process (slot 43; the application static, not `this`): one message dispatched (false
// on WM_QUIT), or the idle frame when there's none
uint8_t __fastcall Process(NiApplication*, re::Edx) {
    MSG msg;
    if (Import<int>(kPeekMessageA, &msg, static_cast<void*>(nullptr), 0u, 0u, 1u) == 0) {
        Virtual<void>(ms_pkApplication, 8);
        return 1;
    }
    if (msg.message == 0x12) {
        return 0;
    }
    if (Import<int>(kTranslateAcceleratorA, ms_pkApplication->appWindow->window, ms_pAccel, &msg) == 0) {
        Import<int>(kTranslateMessage, &msg);
        Import<intptr_t>(kDispatchMessageA, &msg);
    }
    return 1;
}

}   // namespace
}   // namespace ni::app

// ---- registration ---------------------------------------------------------------------

#define C33_APP_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_APP_REVERSED("NiApplication::SetCommandLine", 0x00828B00, &ni::app::SetCommandLine);
C33_APP_REVERSED("NiApplication::MeasureTime", 0x00828B50, &ni::app::MeasureTime);
C33_APP_REVERSED("NiApplication::OnIdle", 0x00828C60, &ni::app::OnIdle);
C33_APP_REVERSED("NiApplication::ResetFrameTimings", 0x00828D10, &ni::app::ResetFrameTimings);
C33_APP_REVERSED("NiApplication::BeginUpdate", 0x00828D20, &ni::app::BeginUpdate);
C33_APP_REVERSED("NiApplication::EndUpdate", 0x00828D30, &ni::app::EndUpdate);
C33_APP_REVERSED("NiApplication::BeginCull", 0x00828D60, &ni::app::BeginCull);
C33_APP_REVERSED("NiApplication::EndCull", 0x00828D70, &ni::app::EndCull);
C33_APP_REVERSED("NiApplication::BeginRender", 0x00828DA0, &ni::app::BeginRender);
C33_APP_REVERSED("NiApplication::EndRender", 0x00828DB0, &ni::app::EndRender);
C33_APP_REVERSED("NiApplication::CheckScreenshot", 0x00828DE0, &ni::app::CheckScreenshot);
C33_APP_REVERSED("NiInputKeyboard::KeyWasPressed", 0x00828E50, &ni::app::Keyboard_KeyWasPressed);
C33_APP_REVERSED("NiApplication::CreateScene", 0x00828E90, &ni::app::CreateScene);
C33_APP_REVERSED("NiApplication::ConfigureInputDevices", 0x00828EF0, &ni::app::ConfigureInputDevices);
C33_APP_REVERSED("NiApplication::Terminate", 0x00828F00, &ni::app::Terminate);
C33_APP_REVERSED("NiApplication::UpdateInput", 0x00829090, &ni::app::UpdateInput);
C33_APP_REVERSED("NiApplication::UpdateFrame", 0x008290E0, &ni::app::UpdateFrame);
C33_APP_REVERSED("NiApplication::CullFrame", 0x00829120, &ni::app::CullFrame);
C33_APP_REVERSED("NiApplication::SaveScreenshot", 0x00829160, &ni::app::SaveScreenshot);
C33_APP_REVERSED("Ni3DRenderView::SetCamera", 0x00829200, &ni::app::View_SetCamera);
C33_APP_REVERSED("NiAccumulatorProcessor::NiAccumulatorProcessor", 0x00829250, &ni::app::Accum_Construct);
C33_APP_REVERSED("NiAccumulatorProcessor::GetRTTI", 0x008292F0, &ni::app::Accum_GetRTTI);
C33_APP_REVERSED("NiAccumulatorProcessor::~NiAccumulatorProcessor", 0x00829300, &ni::app::Accum_Destruct);
C33_APP_REVERSED("NiAccumulatorProcessor::SetAccumulator", 0x00829350, &ni::app::Accum_SetAccumulator);
C33_APP_REVERSED("NiApplication::SetShowAllTrackers", 0x008293A0, &ni::app::SetShowAllTrackers);
C33_APP_REVERSED("NiApplication::ProcessVisualTrackerInput", 0x008293E0, &ni::app::ProcessVisualTrackerInput);
C33_APP_REVERSED("NiApplication::UpdateVisualTrackers", 0x00829450, &ni::app::UpdateVisualTrackers);
C33_APP_REVERSED("NiApplication::BeginFrame", 0x00829480, &ni::app::BeginFrame);
C33_APP_REVERSED("NiApplication::RenderFrame", 0x008294C0, &ni::app::RenderFrame);
C33_APP_REVERSED("NiApplication::RenderScreenItems", 0x008295B0, &ni::app::RenderScreenItems);
C33_APP_REVERSED("NiApplication::RenderVisualTrackers", 0x00829630, &ni::app::RenderVisualTrackers);
C33_APP_REVERSED("NiApplication::EndFrame", 0x00829660, &ni::app::EndFrame);
C33_APP_REVERSED("NiApplication::DisplayFrame", 0x008296A0, &ni::app::DisplayFrame);
C33_APP_REVERSED("NiAccumulatorProcessor::DeletingDestructor", 0x008296F0, &ni::app::Accum_DeletingDestructor);
C33_APP_REVERSED("NiApplication::ShadowRenderStepPre", 0x00829710, &ni::app::ShadowRenderStepPreImpl);
C33_APP_REVERSED("NiApplication::UpdateFrameRateText", 0x00829780, &ni::app::UpdateFrameRateText);
C33_APP_REVERSED("NiApplication::EnableFrameRate", 0x00829820, &ni::app::EnableFrameRate);
C33_APP_REVERSED("NiApplication::ConvertMediaFilename", 0x00829900, &ni::app::ConvertMediaFilename);
C33_APP_REVERSED("NiApplication::GetScreenshotFilename", 0x00829940, &ni::app::GetScreenshotFilename);
C33_APP_REVERSED("NiApplication::CreateFrame", 0x00829970, &ni::app::CreateFrame);
C33_APP_REVERSED("NiApplication::ParseCommandLine", 0x00829EE0, &ni::app::ParseCommandLine);
C33_APP_REVERSED("NiApplication::ShowPointer", 0x0082A000, &ni::app::ShowPointer);
C33_APP_REVERSED("NiApplication::WndProc", 0x0082A010, &ni::app::WndProc);
C33_APP_REVERSED("NiApplication::CreateCamera", 0x0082A0C0, &ni::app::CreateCamera);
C33_APP_REVERSED("NiApplication::CreateRenderer", 0x0082A220, &ni::app::CreateRenderer);
C33_APP_REVERSED("NiApplication::CreateInputSystem", 0x0082A400, &ni::app::CreateInputSystem);
C33_APP_REVERSED("NiApplication::Initialize", 0x0082A500, &ni::app::Initialize);
C33_APP_REVERSED("NiApplication::SetMediaPath", 0x0082A5C0, &ni::app::SetMediaPath);
C33_APP_REVERSED("NiApplication::QuitApplication", 0x0082A5E0, &ni::app::QuitApplication);
C33_APP_REVERSED("NiApplication::Process", 0x0082A600, &ni::app::Process);
