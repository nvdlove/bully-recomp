#pragma once

// Gamebryo's application framework (NiApplication, the sample code linked into the exe) as Bully's
// exe has it: the application (vtable 0x968988, 0xFC bytes, constructor 0x82B280; NiSample,
// vtable 0x9890F8, builds on it and BullyMain, vtable 0x8FF62C, on that), its window, the command
// line, the frame-rate counter, the visual trackers (the performance graphs) and the input
// system's create parameters. These classes have no NiRTTI of their own (but the render views and
// clicks); their names are Gamebryo 2.x's, guessed from the strings and the roles (said so where
// it's a guess). Layouts read from the constructors and checked with static_asserts.
//
// NiApplication's virtual slots (the game's code calls them by slot: src/game/App2.cpp,
// BullyMainFrame.cpp): 0 the deleting destructor, 1 EnableFrameRate(bool), 2 Initialize,
// 3 Terminate, 5 UpdateInput, 6 ProcessInput, 7 ProcessVisualTrackerInput, 8 OnIdle,
// 9 ResetFrameTimings, 10 BeginFrame, 11 BeginUpdate, 12 UpdateFrame, 13 EndUpdate, 14 BeginCull,
// 15 CullFrame, 16 EndCull, 17 BeginRender, 18 RenderFrame, 19 EndRender, 20 RenderScreenItems,
// 21 RenderVisualTrackers, 22 EndFrame, 23 CheckScreenshot, 24 DisplayFrame,
// 25 UpdateVisualTrackers, 26 UpdateMetrics (empty), 27 OnWindowResize(w, h, type, wnd),
// 28 OnWindowDestroy(wnd, option), 29 OnDefault(event record), 30 IsExclusiveMouse,
// 31 CreateScene, 32 CreateCamera, 33 CreateFrame, 34 CreateRenderer, 35 CreateInputSystem,
// 36 CreateVisualTrackers, 37 GetInputSystemCreateParams, 38 ConfigureInputDevices,
// 39 MeasureTime, 40 UpdateFrameRateText, 41 SaveScreenshot, 42 GetScreenshotFilename(buffer,
// size, extension), 43 Process (the message pump).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni::app {

// NiAppWindow (vtable 0x968C60, 0x30 bytes, constructor 0x82B7B0): the main window, the render
// window inside it and the status bar's panes
struct NiAppWindow {
    re::Ptr32<void> vtable;              // +0x0 (slot 0 CreateMainWindow(show, parent), 0x82BA80)
    re::Ptr32<void> window;              // +0x4 (HWND)
    re::Ptr32<void> renderWindow;        // +0x8 (HWND)
    re::Ptr32<re::Ptr32<void>> statusWindow;       // +0xC (NiMalloc'd: the status bar's HWND at [0])
    re::Ptr32<char> caption;             // +0x10 (NiMalloc'd copy; "Gamebryo Application" when none)
    uint32_t parentWidth;      // +0x14
    uint32_t width;            // +0x18
    uint32_t parentHeight;     // +0x1C
    uint32_t height;           // +0x20
    uint32_t numStatusPanes;   // +0x24
    uint8_t statusPanes;       // +0x28 (the status bar made)
    uint8_t pad29[3];
    uint32_t windowStyle;      // +0x2C (WS_ flags: 0xCA0000)
};
static_assert(offsetof(NiAppWindow, caption) == 0x10);
static_assert(offsetof(NiAppWindow, statusPanes) == 0x28);
static_assert(sizeof(NiAppWindow) == 0x30);

// NiCommand (0x20 bytes, constructor 0x82BCD0): the command line split into arguments (a copy
// cut up in place; quoted ones kept whole), which have been used, and the last error
struct NiCommand {
    int32_t argc;              // +0x0 (the arguments and the program's slot 0)
    re::Ptr32<re::Ptr32<char>> argv;               // +0x4 (NiMalloc'd; argv[0] the whole copy)
    re::Ptr32<char> commandLine;         // +0x8 (NiMalloc'd copy, cut up)
    re::Ptr32<uint8_t> used;             // +0xC (NiMalloc'd: one per argument)
    uint32_t unknown10;        // +0x10 (0)
    uint32_t unknown14;        // +0x14 (0)
    uint8_t unknown18[4];      // +0x18 (0)
    re::Ptr32<const char> error;         // +0x1C ("option not found")
};
static_assert(sizeof(NiCommand) == 0x20);

// NiFrameRate (0xB0 bytes): the frame-rate digits drawn over the scene (a screen elements object
// of three textured quads) and the frame time statistics. (The constructor isn't the
// application's: NiSample's)
struct NiFrameRate {
    re::Ptr32<NiObject> element;         // +0x0 (NiScreenElements: the digits; a reference)
    int32_t digits[3];         // +0x4 (its polygons, hundreds first)
    uint8_t enabled;           // +0x10 (the digits made: drawn and released)
    uint8_t pad11[3];
    uint32_t frameRate;        // +0x14 (frames over the last 20 samples' time, rounded)
    uint32_t lastFrameRate;    // +0x18 (the one the digits show)
    uint32_t numSamples;       // +0x1C
    float sumOfTimes;          // +0x20
    float sumOfSquares;        // +0x24
    uint8_t first;             // +0x28 (no sample yet)
    uint8_t pad29[3];
    float startTime;           // +0x2C
    float lastTime;            // +0x30
    float sampleTimes[20];     // +0x34 (a ring)
    uint32_t sampleIndex;      // +0x84
    float minTime;             // +0x88
    float maxTime;             // +0x8C
    uint32_t histogramSize;    // +0x90
    re::Ptr32<uint32_t> histogram;       // +0x94 (NiMalloc'd: frames by 1 / time)
    float x;                   // +0x98 (the digits' place on the target, in pixels)
    float y;                   // +0x9C
    float digitStep;           // +0xA0 (one glyph's share of the texture's v: 11 glyphs, the
                               //  eleventh blank)
    re::Ptr32<NiObject> target;          // +0xA4 (the render target measured: none, the renderer's default)
    uint32_t unknownA8;        // +0xA8
    re::Ptr32<NiObject> texture;         // +0xAC (a reference)
};
static_assert(offsetof(NiFrameRate, sampleTimes) == 0x34);
static_assert(offsetof(NiFrameRate, histogram) == 0x94);
static_assert(sizeof(NiFrameRate) == 0xB0);

// NiInputSystem::CreateParams (vtable 0x968974, 0x20) and NiDI8InputSystem::DI8CreateParams
// (vtable 0x96897C, 0x7C): slot 0 GetRTTI, slot 1 the deleting destructor
struct NiInputCreateParams {
    re::Ptr32<void> vtable;              // +0x0
    re::Ptr32<void> actionMap;           // +0x4 (0)
    uint32_t keyboardFlags;    // +0x8 (8)
    uint32_t mouseFlags;       // +0xC (8)
    uint32_t gamePadFlags;     // +0x10 (8)
    int32_t axisLow;           // +0x14 (-100)
    int32_t axisHigh;          // +0x18 (100)
    re::Ptr32<NiRefObject> renderer;     // +0x1C (a reference)
};
static_assert(sizeof(NiInputCreateParams) == 0x20);

struct NiDI8CreateParams : NiInputCreateParams {
    re::Ptr32<void> instance;            // +0x20 (HINSTANCE)
    re::Ptr32<void> window;              // +0x24 (HWND)
    re::Ptr32<char> actionNames[21];     // +0x28 (NiFree'd)
};
static_assert(sizeof(NiDI8CreateParams) == 0x7C);

// NiAccumulatorProcessor (vtable 0x9687A8, 0x14, RTTI "NiAccumulatorProcessor" under
// NiRenderListProcessor): the visible geometry handed to an accumulator (sorter) instead of drawn
struct NiAccumulatorProcessor : NiRefObject {
    re::Ptr32<NiObject> accumulator;     // +0x8 (a reference)
    re::Ptr32<NiObject> camera;          // +0xC (a NiCamera of its own, a reference)
    uint8_t unknown10;         // +0x10
    uint8_t pad11[3];
};
static_assert(sizeof(NiAccumulatorProcessor) == 0x14);

// NiApplication (vtable 0x968988, 0xFC bytes)
struct NiApplication {
    re::Ptr32<void> vtable;                     // +0x0
    re::Ptr32<NiNode> scene;                    // +0x4 (references, as are the next ten)
    re::Ptr32<NiObject> camera;                 // +0x8 (NiCamera)
    re::Ptr32<NiObject> frame;                  // +0xC (NiRenderFrame)
    re::Ptr32<NiObject> mainSceneView;          // +0x10 (Ni3DRenderView)
    re::Ptr32<NiObject> screenElementsView;     // +0x14 (Ni2DRenderView)
    re::Ptr32<NiObject> screenTextureClick;     // +0x18 (NiScreenTextureRenderClick)
    re::Ptr32<NiObject> visualTrackerClick;     // +0x1C (NiVisualTrackerRenderClick)
    re::Ptr32<NiAccumulatorProcessor> accumulatorProcessor;   // +0x20
    re::Ptr32<NiObject> renderer;               // +0x24 (NiRenderer)
    re::Ptr32<NiObject> inputSystem;            // +0x28 (NiInputSystem)
    re::Ptr32<NiAppWindow> appWindow;           // +0x2C
    uint32_t menuID;                  // +0x30
    uint8_t fullscreen;               // +0x34
    uint8_t stencil;                  // +0x35
    uint8_t rendererDialog;           // +0x36
    uint8_t multiThread;              // +0x37
    uint32_t bitDepth;                // +0x38
    re::Ptr32<NiFrameRate> frameRate;           // +0x3C
    uint8_t frameRateEnabled;         // +0x40
    uint8_t pad41[3];
    int32_t maxTimer;                 // +0x44 (30: frames between the status bar's fps)
    int32_t timer;                    // +0x48
    float currentTime;                // +0x4C
    float lastTime;                   // +0x50 (-1: none yet)
    float accumTime;                  // +0x54
    float frameTime;                  // +0x58
    int32_t clicks;                   // +0x5C (frames)
    float frameRateValue;             // +0x60
    float lastFrameRateTime;          // +0x64
    int32_t lastFrameRateClicks;      // +0x68
    uint32_t numObjectsDrawn;         // +0x6C
    float cullTime;                   // +0x70
    float renderTime;                 // +0x74
    float updateTime;                 // +0x78
    float beginUpdate;                // +0x7C
    float beginCull;                  // +0x80
    float beginRender;                // +0x84
    float minFramePeriod;             // +0x88 (0.01)
    float lastFrame;                  // +0x8C
    float vtPerformanceMax;           // +0x90 (100: the performance tracker's range)
    float vtTimeMax;                  // +0x94 (10000)
    float vtFrameRateMax;             // +0x98 (100: the time tracker's range)
    uint8_t d3d10;                    // +0x9C ("-d3d10")
    uint8_t refRast;                  // +0x9D ("-refrast")
    uint8_t swVertex;                 // +0x9E ("-swvertex")
    uint8_t nvPerfHUD;                // +0x9F ("-nvperf")
    uint8_t exclusiveMouse;           // +0xA0
    uint8_t padA1[3];
    re::Ptr32<re::Ptr32<void>> visible;                   // +0xA4 (NiVisibleArray: the array, count, allocated, grow by)
    uint32_t visibleCount;            // +0xA8
    uint32_t visibleAllocated;        // +0xAC
    uint32_t visibleGrowBy;           // +0xB0 (0x400)
    re::Ptr32<NiObject> culler;                 // +0xB4 (NiCullingProcess, a reference)
    re::Ptr32<const char> names[10];          // +0xB8 (the render frame's, steps', clicks' and views')
    uint8_t showAllTrackers;          // +0xE0
    uint8_t useFrameSystem;           // +0xE1
    uint8_t useFixedTime;             // +0xE2 ("-test")
    uint8_t padE3;
    float fixedTimeIncrement;         // +0xE4 (1/60)
    float appEndTime;                 // +0xE8 (FLT_MAX: never)
    uint8_t screenshots;              // +0xEC (taken every screenshotPeriod)
    uint8_t padED[3];
    float screenshotPeriod;           // +0xF0 (-FLT_MAX: none)
    float lastScreenshotTime;         // +0xF4 (-FLT_MAX)
    uint32_t screenshotCount;         // +0xF8
};
static_assert(offsetof(NiApplication, appWindow) == 0x2C);
static_assert(offsetof(NiApplication, frameRate) == 0x3C);
static_assert(offsetof(NiApplication, minFramePeriod) == 0x88);
static_assert(offsetof(NiApplication, d3d10) == 0x9C);
static_assert(offsetof(NiApplication, visible) == 0xA4);
static_assert(offsetof(NiApplication, names) == 0xB8);
static_assert(offsetof(NiApplication, showAllTrackers) == 0xE0);
static_assert(offsetof(NiApplication, screenshotCount) == 0xF8);
static_assert(sizeof(NiApplication) == 0xFC);

// the application's statics
GAME_VAR(NiApplication*, ms_pkApplication, 0x020E5874);
GAME_VAR(void*, ms_pInstance, 0x020E5878);         // HINSTANCE (the game sets 0)
GAME_VAR(void*, ms_pAccel, 0x020E587C);            // HACCEL
GAME_VAR(NiCommand*, ms_pkCommand, 0x020E5880);
GAME_ARRAY(char, ms_acMediaPath, 0x00BC8C68);      // "./../../Data/" (0x104)
GAME_ARRAY(char, ms_acTempMediaFilename, 0x020E5888);   // ConvertMediaFilename's (0x104)
GAME_VAR(uint32_t, ms_eScreenshotFormat, 0x00BC8D6C);   // 0 tga, 1 png, 2 jpg

// the NiTPointerList node the list walks read (next, previous, element)
struct ListNode {
    re::Ptr32<ListNode> next;
    re::Ptr32<ListNode> prev;
    re::Ptr32<void> element;
};

}   // namespace ni::app
