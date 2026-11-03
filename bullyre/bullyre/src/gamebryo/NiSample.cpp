// NiSample, Gamebryo's sample application class Bully's main frame derives from (vtable
// 0x9890F8 over NiApplication's 0x968988, 0x230 bytes): its Initialize and Terminate, the frame
// update and render around NiApplication's, the cursor, the UI system with its output log and
// frame-rate labels, the two navigation controllers ("Flying" and "Orbiting") and the shader
// library and parser loading through the renderer's shader directories. NiApplication's own
// virtuals (the other pass's range) are called at their addresses.
//
// The virtuals' names are Gamebryo 2.x's NiSample's where the behaviour matches (a guess: no
// symbols). BullyMain (src/game/BullyMainFrame.cpp) overrides 0 (destructor), 2 (Initialize),
// 3 (Terminate), 12, 18, 29, 31, 33 (CreateFrame), 50 (LoadShaders), 54 and adds 60, 61.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSampleFramework.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::sample {
namespace {

// NiApplication's (the other pass's range)
GAME_FN(void, __thiscall, Application_Construct, 0x0082B280, NiSample* self, const char* caption, uint32_t width,
        uint32_t height, int useFrameSystem, uint32_t menuID, uint32_t numStatusPanes, uint32_t bitDepth);
GAME_FN(uint8_t, __thiscall, Application_CreateInputSystem, 0x0082A400, NiSample* self);
GAME_FN(void, __thiscall, Application_UpdateInput, 0x008290E0, NiSample* self);
GAME_FN(void, __thiscall, Application_UpdateFrame, 0x00828D30, NiSample* self);
GAME_FN(void, __thiscall, Application_RenderFrame, 0x008294C0, NiSample* self);
GAME_FN(void, __thiscall, Application_Terminate, 0x00828F00, NiSample* self);
GAME_FN(uint8_t, __thiscall, Application_CreateFrame, 0x00829970, NiSample* self);
GAME_FN(const char*, __cdecl, ConvertMediaFilename, 0x00829900, const char* filename);
GAME_FN(void, __thiscall, Sample_DestructBody, 0x004045F0, NiSample* self);   // (game code's region)
// the message box wrapper NiSample installs while the input system's made (shows the cursor)
constexpr uintptr_t kSampleMessageBox = 0x00830D40;
GAME_VAR(MessageBoxFn, NiSample_ms_pfnPreviousMessageBox, 0x020E5A00);
// the member slots' functions (NiApplication's / the other pass's)
constexpr uintptr_t kToggleUIFunction = 0x008A20A0;      // (an empty one: Bully's)
constexpr uintptr_t kToggleNavHelpFunction = 0x008313A0;
constexpr uintptr_t kCycleNavFunction = 0x00830DA0;      // (calls vf58)
constexpr uintptr_t kSlot168Function = 0x0082A5E0;

GAME_FN(uint32_t, __cdecl, NiShadowManager_Initialize, 0x00759570);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, void* self);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E240, void* self);
GAME_FN(float, __cdecl, NiGetCurrentTimeInSec, 0x007537B0);
GAME_FN(void, __thiscall, Renderer_ConvertFromNDCToPixels, 0x00755810, void* self, float x, float y, uint32_t* px,
        uint32_t* py);
GAME_FN(uint8_t, __thiscall, Renderer_BeginUsingRenderTargetGroup, 0x00407890, void* self, void* group,
        uint32_t clearMode);
GAME_FN(uint8_t, __thiscall, Renderer_EndUsingRenderTargetGroup, 0x00407990, void* self);
GAME_FN(void, __cdecl, NiDrawVisibleArray, 0x008C5850, void* camera, void* visible);
GAME_FN(NiRefObject*, __cdecl, NiCursor_Create, 0x008C4370, void* renderer, const int32_t* rect, uint32_t flags,
        uint32_t cursorCount, const char* filename, void* renderTargetGroup);
GAME_FN(void, __cdecl, NiCursor_Shutdown, 0x008C4440);

GAME_FN(char*, __thiscall, NiString_Construct, 0x007741A0, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, re::Ptr32<char>* self);
GAME_FN(uint32_t, __thiscall, NiString_Length, 0x00773D10, const re::Ptr32<char>* self);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_AssignText, 0x007745E0, re::Ptr32<char>* self, const char* text);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);

GAME_FN(uint8_t, __cdecl, ShaderFactory_SetClassCreationCallback, 0x00756670, void* callback);
GAME_FN(uint8_t, __cdecl, ShaderFactory_SetRunParserCallback, 0x00756690, void* callback);
GAME_FN(uint8_t, __cdecl, ShaderFactory_SetErrorCallback, 0x007566B0, void* callback);
GAME_FN(void, __cdecl, ShaderFactory_AddShaderProgramFileDirectory, 0x007566E0, const char* directory);
GAME_FN(uint8_t, __cdecl, ShaderFactory_LoadAndRegisterShaderLibrary, 0x00756990, const char* name, int count,
        re::Ptr32<char>* directories, int recurse);
GAME_FN(uint32_t, __cdecl, ShaderFactory_LoadAndRunParserLibrary, 0x00756A10, const char* name,
        const char* directory, int recurse);
constexpr uintptr_t kReturnZero = 0x00824EC0;   // (folded "return 0": the error callback)
// the renderers' class indices (Bully's NiRTTI keeps a bit set of its bases: +0xC)
GAME_VAR(uint32_t, NiDX9Renderer_ms_uiClassIndex, 0x020E6D20);
GAME_VAR(uint32_t, NiD3D10Renderer_ms_uiClassIndex, 0x020E8F38);

GAME_FN(void, __thiscall, ClickRenderStep_Construct, 0x007650D0, void* self);
GAME_FN(void, __thiscall, ClickRenderStep_AppendRenderClick, 0x00402960, void* self, void* click);
GAME_FN(void, __thiscall, RenderFrame_AppendRenderStep, 0x00402B40, void* self, void* step);
GAME_FN(void, __thiscall, RenderClick_Construct, 0x00515C00, void* self);
GAME_FN(void, __thiscall, UIManagerRenderClick_Construct, 0x00839010, void* self);
GAME_FN(void, __thiscall, CursorRenderClick_AppendCursor, 0x008316F0, void* self, NiRefObject* cursor);
constexpr uint32_t kNiDefaultClickRenderStepVtable = 0x008FF5B4;
constexpr uint32_t kNiTPointerListRenderClickVtable = 0x008FF508;
constexpr uint32_t kNiCursorRenderClickVtable = 0x00988FD4;
constexpr uint32_t kNiTPointerListCursorVtable = 0x0095127C;

constexpr uint32_t kNiSampleVtable = 0x009890F8;
constexpr uint32_t kStringArrayVtable = 0x00988FB0;     // NiTPrimitiveArray<NiString*>
constexpr uint32_t kGroupArrayVtable = 0x00988FD0;      // NiTArray<NiPointer<NiUIGroup>>
constexpr uint32_t kCreatorArrayVtable = 0x00988FB4;    // NiTPrimitiveArray<library creator>
constexpr uint32_t kParserArrayVtable = 0x00988FB8;     // NiTPrimitiveArray<parser runner>

// the label colours (NiColorA background, NiColor text)
GAME_ARRAY(float, kLabelBackground, 0x00BC75D4);
GAME_ARRAY(float, kLabelText, 0x00BC75C8);

const char* const kEmpty = reinterpret_cast<const char*>(0x008FEFBB);   // ""

// a renderer's NiRTTI bit for a class index (Bully's IsKindOf)
bool RendererIsKindOf(void* renderer, uint32_t classIndex) {
    void* rtti = Virtual<void*>(renderer, 2);   // GetRTTI
    uint32_t bits = At<uint32_t>(rtti, 0xC + int(classIndex >> 5) * 4);
    return (bits & (1u << (classIndex & 0x1F))) != 0;
}

// the clamp the navigation scale gets (an x87 fcom: NaN passes)
inline float AtLeast(float value, float minimum) {
    return minimum > value ? minimum : value;
}

// ---- the virtuals

// vf2 Initialize: the renderer, camera, shaders, input, cursor, scene (and frame) made, the
// scene and camera updated once, then the UI system, navigation, UI elements, the visual trackers
uint8_t __fastcall Initialize(NiSample* self, re::Edx) {
    if (At<uint8_t>(self, app::kUseFrameSystem) != 0) {
        NiShadowManager_Initialize();
    }
    if (!Virtual<uint8_t>(self, 34)) {   // CreateRenderer
        return 0;
    }
    if (!Virtual<uint8_t>(self, 32)) {   // CreateCamera
        return 0;
    }
    if (!Virtual<uint8_t>(self, 50)) {   // LoadShaders
        return 0;
    }
    if (!Virtual<uint8_t>(self, 35)) {   // CreateInputSystem
        return 0;
    }
    if (!Virtual<uint8_t>(self, 44)) {   // CreateCursor
        return 0;
    }
    if (!Virtual<uint8_t>(self, 31)) {   // CreateScene
        return 0;
    }
    if (At<uint8_t>(self, app::kUseFrameSystem) != 0 && !Virtual<uint8_t>(self, 33)) {   // CreateFrame
        return 0;
    }
    void* scene = At<void*>(self, app::kScene);
    if (scene != nullptr) {
        AVObject_Update(scene, 0.0f, 1);
        AVObject_UpdateProperties(scene);
        AVObject_UpdateEffects(scene);
    }
    void* camera = At<void*>(self, app::kCamera);
    if (camera != nullptr) {
        AVObject_Update(camera, 0.0f, 1);
    }
    if (!Virtual<uint8_t>(self, 45)) {   // CreateUISystem
        return 0;
    }
    if (!Virtual<uint8_t>(self, 46)) {   // CreateNavigationControllers
        return 0;
    }
    if (!Virtual<uint8_t>(self, 48)) {   // CreateUIElements
        return 0;
    }
    if (!Virtual<uint8_t>(self, 49)) {   // CompleteUISystem
        return 0;
    }
    return Virtual<uint8_t>(self, 36) != 0;   // CreateVisualTrackers
}

// vf35 CreateInputSystem: NiApplication's, with the message box function swapped for the one
// showing the cursor (and never put back: the game's)
uint8_t __fastcall CreateInputSystem(NiSample* self, re::Edx) {
    NiSample_ms_pfnPreviousMessageBox = NiMessageBoxFunction;
    NiMessageBoxFunction = reinterpret_cast<MessageBoxFn>(kSampleMessageBox);
    return Application_CreateInputSystem(self);
}

// the shader factory's class creation callback: the application's creator at the index being
// loaded (none: success)
using LibraryCreator = uint8_t(__cdecl*)(void* renderer, int count, re::Ptr32<char>* directories, int recurse,
                                         re::Ptr32<void>* library);
uint8_t __cdecl LibraryClassCreate(const char*, void* renderer, int count, re::Ptr32<char>* directories, int recurse,
                                   re::Ptr32<void>* library) {
    *library = nullptr;
    NiSample* app = NiApplication_ms_pkApplication;
    uint32_t index = app->loadIndex;
    if (index >= app->libraryCreators.size) {
        return 0;
    }
    auto creator = re::PtrCast<LibraryCreator>(app->libraryCreators.base[index]);
    if (creator != nullptr && !creator(renderer, count, directories, recurse, library)) {
        return 0;
    }
    return 1;
}

// the shader factory's run-parser callback: the application's runner at the index being loaded
using ParserRunner = uint32_t(__cdecl*)(const char* directory, int recurse);
uint32_t __cdecl RunParser(const char*, void*, const char* directory, int recurse) {
    NiSample* app = NiApplication_ms_pkApplication;
    uint32_t index = app->loadIndex;
    if (index >= app->parserRunners.size) {
        return 0;
    }
    auto runner = re::PtrCast<ParserRunner>(app->parserRunners.base[index]);
    if (runner == nullptr) {
        return 0;
    }
    return runner(directory, recurse);
}

// vf46 CreateNavigationControllers: the "Flying" and "Orbiting" controllers on the camera,
// handed to the navigation manager (none: failure)
uint8_t __fastcall CreateNavigationControllers(NiSample* self, re::Edx) {
    if (self->useNavigation == 0) {
        return 1;
    }
    if (NiNavManager_ms_pkNavManager == nullptr) {
        return 0;
    }
    auto camera = At<NiAVObject*>(self, app::kCamera);
    auto* fly = static_cast<NiNavFlyController*>(NiNew(0xC8));
    if (fly != nullptr) {
        fly = FlyController_Construct(fly, camera, camera, "\"Flying\" Mode", self->navScale, self->navUpDir.x,
                                      self->navUpDir.y, self->navUpDir.z, 0, 0);
    }
    Pointer_Assign(reinterpret_cast<re::Ptr32<void>*>(&self->fly), fly);
    auto* orbit = static_cast<NiNavOrbitController*>(NiNew(0xB8));
    if (orbit != nullptr) {
        orbit = OrbitController_Construct(orbit, camera, "\"Orbiting\" Mode", self->navScale, self->navUpDir.x,
                                          self->navUpDir.y, self->navUpDir.z);
    }
    Pointer_Assign(reinterpret_cast<re::Ptr32<void>*>(&self->orbit), orbit);
    NavManager_AddNavigationController(NiNavManager_ms_pkNavManager, self->fly);
    NavManager_AddNavigationController(NiNavManager_ms_pkNavManager, self->orbit);
    return 1;
}

// vf49 CompleteUISystem: the render groups prepared; with more than one, the second focused
// and its focus moved to its first element
uint8_t __fastcall CompleteUISystem(NiSample*, re::Edx) {
    NiUIManager* manager = UIManager_Get();
    UIManager_PrepareRenderGroups(manager);
    if (manager->groups.size > 1) {
        NiUIRenderGroup* group = UIManager_SetFocusedGroup(manager, 1);
        if (group != nullptr) {
            Virtual<uint8_t>(group, 21);   // NiUIGroup::AdvanceFocus
        }
    }
    return 1;
}

// vf55 RenderUIElements: the UI manager's groups, then the cursor
void __fastcall RenderUIElements(NiSample* self, re::Edx) {
    void* renderer = At<void*>(self, app::kRenderer);
    UIManager_Draw(UIManager_Get(), renderer);
    Virtual<void>(self->cursor, 7);   // NiCursor::Draw (a tail call)
}

// UpdateLog: every log line joined with line feeds into a 1 KB buffer (not bounds-checked: the
// game's) as the log label's text, then the label reformatted
void __fastcall UpdateLog(NiSample* self, re::Edx) {
    char buffer[0x400];
    uint32_t count = self->logLines.size;
    uint32_t length = 0;
    for (uint32_t i = 0; i < count; i++) {
        re::Ptr32<char>* line = self->logLines.base[i];
        if (line != nullptr) {
            uint32_t lineLength = NiString_Length(line);
            crt::Memcpy(buffer + length, *line, lineLength);
            length += lineLength;
            buffer[length] = '\n';
            length++;
        }
    }
    if (length == 0) {
        buffer[0] = '\0';
    } else {
        buffer[length - 1] = '\0';
    }
    NiUILabel* label = self->logLabel;
    NiString_AssignText(&label->text, buffer);
    UILabel_ReformatText(label);
}

// the toggle-slot's function: the navigation help shown or hidden (the current controller's group)
void __fastcall ToggleNavHelp(NiSample* self, re::Edx) {
    if (self->useNavigation == 0) {
        return;
    }
    self->showNavHelp = self->showNavHelp == 0 ? 1 : 0;
    NiNavManager* manager = NiNavManager_ms_pkNavManager;
    if (manager == nullptr) {
        return;
    }
    NiUIGroup* group = self->navHelpGroups.base[GetUint30(manager)];   // the current controller's
    if (group != nullptr) {
        Virtual<void>(group, 7, uint32_t(self->showNavHelp));   // SetVisible
    }
}

// vf58 CycleNavigationController: the next controller made current, its help group shown in
// the old one's place, the log line updated
void __fastcall CycleNavigationController(NiSample* self, re::Edx) {
    if (self->useNavigation == 0) {
        return;
    }
    NiNavManager* manager = NiNavManager_ms_pkNavManager;
    uint32_t oldIndex = GetUint30(manager);
    NavManager_CycleController(manager);
    NiNavBaseController* controller = NavManager_GetCurrentController(manager);
    uint32_t newIndex = GetUint30(manager);
    NiUIGroup* oldGroup = self->navHelpGroups.base[oldIndex];
    if (oldGroup != nullptr) {
        Virtual<void>(oldGroup, 7, 0u);
    }
    NiUIGroup* newGroup = self->navHelpGroups.base[newIndex];
    if (newGroup != nullptr) {
        Virtual<void>(newGroup, 7, uint32_t(self->showNavHelp));
    }
    Virtual<void>(self, 59, controller);   // SetNavigationLogLine
}

// vf50 LoadShaders: the factory's errors swallowed, the program directories for the renderer
// (Shaders/Common, then Shaders/DX9 or Shaders/D3D10), the parsers run over "Shaders", then
// the libraries registered from it
uint8_t __fastcall LoadShaders(NiSample* self, re::Edx) {
    ShaderFactory_SetErrorCallback(reinterpret_cast<void*>(kReturnZero));
    void* renderer = *reinterpret_cast<re::Ptr32<void>*>(0x020E2380);   // NiRenderer::ms_pkRenderer
    const char* rendererDirectory = nullptr;
    if (renderer != nullptr) {
        if (RendererIsKindOf(renderer, NiDX9Renderer_ms_uiClassIndex)) {
            rendererDirectory = "Shaders/DX9";
        } else if (RendererIsKindOf(renderer, NiD3D10Renderer_ms_uiClassIndex)) {
            rendererDirectory = "Shaders/D3D10";
        }
    }
    const char* directory = "Shaders";
    ShaderFactory_AddShaderProgramFileDirectory(ConvertMediaFilename("Shaders/Common"));
    ShaderFactory_AddShaderProgramFileDirectory(ConvertMediaFilename(rendererDirectory));
    if (!Virtual<uint8_t>(self, 51, directory)) {   // RunShaderParsers
        if (NiMessageBoxFunction != nullptr) {
            NiMessageBoxFunction("Failed to run shader parsers!", "ERROR", 0);
        }
        return 0;
    }
    if (!Virtual<uint8_t>(self, 52, 1, &directory)) {   // RegisterShaderLibraries
        if (NiMessageBoxFunction != nullptr) {
            NiMessageBoxFunction("Failed to load shader libraries!", "ERROR", 0);
        }
        return 0;
    }
    return 1;
}

// vf51 RunShaderParsers(directory): each parser runner the application registered (vf53)
uint8_t __fastcall RunShaderParsers(NiSample* self, re::Edx, const char* directory) {
    ShaderFactory_SetRunParserCallback(reinterpret_cast<void*>(0x00831130));   // RunParser
    if (!Virtual<uint8_t>(self, 53)) {   // RegisterShaderParsers
        return 0;
    }
    self->loadIndex = 0;
    while (self->loadIndex < self->parserRunners.size) {
        ShaderFactory_LoadAndRunParserLibrary(nullptr, ConvertMediaFilename(directory), 1);
        self->loadIndex++;
    }
    return 1;
}

// vf52 RegisterShaderLibraries(count, directories): each library creator the application
// registered (vf54), over copies of the directories as the media paths
uint8_t __fastcall RegisterShaderLibraries(NiSample* self, re::Edx, int count, const re::Ptr32<const char>* directories) {
    ShaderFactory_SetClassCreationCallback(reinterpret_cast<void*>(0x008310D0));
    if (!Virtual<uint8_t>(self, 54)) {   // RegisterShaderLibraryCreators
        return 0;
    }
    auto* copies = static_cast<re::Ptr32<char>*>(NiMalloc(uint32_t(count) * 4));
    for (int i = 0; i < count; i++) {
        const char* path = ConvertMediaFilename(directories[i]);
        size_t size = std::strlen(path) + 1;
        copies[i] = static_cast<char*>(NiMalloc(uint32_t(size)));
        crt::StrcpyS(copies[i], size, path);
    }
    self->loadIndex = 0;
    while (self->loadIndex < self->libraryCreators.size) {
        if (!ShaderFactory_LoadAndRegisterShaderLibrary(nullptr, count, copies, 1)) {
            if (NiMessageBoxFunction != nullptr) {
                NiMessageBoxFunction("Failed to load shader library!", "ERROR", 0);
            }
            for (int i = 0; i < count; i++) {
                NiFree(copies[i]);
                copies[i] = nullptr;
            }
            NiFree(copies);
            return 0;
        }
        self->loadIndex++;
    }
    for (int i = 0; i < count; i++) {
        NiFree(copies[i]);
        copies[i] = nullptr;
    }
    NiFree(copies);
    return 1;
}

// vf44 CreateCursor: the system cursors over the default render target, put where the Windows
// cursor is (320, 240 when that fails), shown, Windows' hidden, added to the cursor click
uint8_t __fastcall CreateCursor(NiSample* self, re::Edx) {
    void* renderer = At<void*>(self, app::kRenderer);
    void* group = Virtual<void*>(renderer, 29);   // GetDefaultRenderTargetGroup
    int32_t height = Virtual<int32_t>(group, 19, 0u);
    int32_t width = Virtual<int32_t>(group, 18, 0u);
    int32_t rect[4] = {0, width, 0, height};   // NiRect<int>: left, right, top, bottom
    NiRefObject* cursor =
        NiCursor_Create(renderer, rect, 4, 0xD, "../../../../Data/SystemCursors.tga", group);
    AssignRef(self->cursor, cursor);
    if (self->cursor == nullptr) {
        return 0;
    }
    int32_t point[2];
    int32_t x = 0x140;
    int32_t y = 0xF0;
    if (ni::sys::Import<int>(0x008FC254, point) == 1) {   // GetCursorPos
        void* window = At<void*>(At<void*>(self, app::kAppWindow), 8);
        if (ni::sys::Import<int>(0x008FC28C, window, point) == 1) {   // ScreenToClient
            x = point[0];
            y = point[1];
        }
    }
    Virtual<void>(self->cursor, 5, 0.0f, x, y);   // SetPosition(time, x, y)
    Virtual<void>(self->cursor, 4, 1);            // Show
    ni::sys::Import<int>(0x008FC244, 0);           // ShowCursor(FALSE)
    CursorRenderClick_AppendCursor(self->cursorClick, self->cursor);
    return 1;
}

// vf18 RenderFrame: with the frame system, NiApplication's; else the default target begun
// (cleared when the renderer needs it), the scene and screen items, the visible set drawn, the
// visual trackers, UI and cursor, the target ended
void __fastcall RenderFrame(NiSample* self, re::Edx) {
    if (At<uint8_t>(self, app::kUseFrameSystem) != 0) {
        Application_RenderFrame(self);
        return;
    }
    void* renderer = At<void*>(self, app::kRenderer);
    int32_t mode = At<int32_t>(renderer, 0x94);
    if ((mode == 1 || mode == 2 || mode == 4) && (At<uint8_t>(renderer, 0x90) == 0 || mode == 4)) {
        Renderer_BeginUsingRenderTargetGroup(renderer, Virtual<void*>(renderer, 29), 7);
    }
    Virtual<void>(self, 15);   // RenderScene
    Virtual<void>(self, 17);   // CullFrame
    NiDrawVisibleArray(At<void*>(self, app::kCamera), reinterpret_cast<uint8_t*>(self) + app::kVisible);
    Virtual<void>(self, 20);   // RenderScreenItems
    Virtual<void>(self, 55);   // RenderUIElements
    Virtual<void>(self, 19);   // RenderVisualTrackers
    Virtual<void>(self, 21);   // EndRender
    Renderer_EndUsingRenderTargetGroup(At<void*>(self, app::kRenderer));
}

// vf3 Terminate: the log lines deleted, every reference dropped, the cursor and the UI and
// navigation managers shut down, then NiApplication's
void __fastcall Terminate(NiSample* self, re::Edx) {
    for (uint32_t i = 0; i < self->logLines.size; i++) {
        re::Ptr32<char>* line = self->logLines.base[i];
        if (line != nullptr) {
            NiString_Destruct(line);
            NiDelete(line, 4);
        }
    }
    for (uint16_t i = 0; i < self->logLines.size; i++) {
        self->logLines.base[i] = nullptr;
    }
    self->logLines.size = 0;
    self->logLines.esize = 0;
    ClearRef(self->orbit);
    ClearRef(self->fly);
    ClearRef(self->logLabel);
    ClearRef(self->frameRateLabel);
    ObjectArray_RemoveAll(&self->navHelpGroups);
    ClearRef(self->cursorClick);
    ClearRef(self->cursor);
    NiCursor_Shutdown();
    if (NiNavManager_ms_pkNavManager != nullptr) {
        Virtual<void>(NiNavManager_ms_pkNavManager, 0, 1);   // the deleting destructor
    }
    NiNavManager_ms_pkNavManager = nullptr;
    UIManager_Shutdown();
    Application_Terminate(self);
}

// vf12 UpdateInput: NiApplication's, then the mouse's movement moves the cursor
void __fastcall UpdateInput(NiSample* self, re::Edx) {
    Application_UpdateInput(self);
    void* mouse = At<void*>(At<void*>(self, app::kInputSystem), 0x44);
    if (mouse == nullptr || self->cursor == nullptr) {
        return;
    }
    int32_t dx = At<int32_t>(mouse, 0x24);
    int32_t dy = At<int32_t>(mouse, 0x44);
    if (dx != 0 || dy != 0) {
        Virtual<void>(self->cursor, 6, 0.0f, dx, dy);   // Move(time, dx, dy)
    }
}

// SetLogLine(index, text): one line replaced (the total length kept), the log updated
uint8_t __fastcall SetLogLine(NiSample* self, re::Edx, uint32_t index, const char* text) {
    if (index >= self->logLines.size) {
        return 0;
    }
    uint32_t length = uint32_t(std::strlen(text));
    re::Ptr32<char>* line = self->logLines.base[index];
    uint32_t oldLength = 0;
    if (line != nullptr) {
        oldLength = NiString_Length(line);
        NiString_AssignText(line, text);
    }
    self->logLength += length - oldLength;
    Sample_UpdateLog(self);
    return 1;
}

// vf59 SetNavigationLogLine(controller): "Navigation Controller Type: <name>" on its line
void __fastcall SetNavigationLogLine(NiSample* self, re::Edx, NiNavBaseController* controller) {
    if (controller == nullptr || self->navLogLine == -1) {
        return;
    }
    char buffer[0x100];
    NiSprintf(buffer, 0x100, "Navigation Controller Type: %s", controller->name);
    Sample_SetLogLine(self, uint32_t(self->navLogLine), buffer);
}

// vf47 SetNavigationParameters(controlled, reference, up, scale): both controllers given the
// objects, the up direction and the scale (at least 1e-5)
uint8_t __fastcall SetNavigationParameters(NiSample* self, re::Edx, NiAVObject* controlled, NiAVObject* reference,
                                           const NiPoint3* upDir, float scale) {
    if (self->useNavigation == 0) {
        return 1;
    }
    if (self->fly != nullptr) {
        FlyController_SetUpDir(self->fly, upDir);
        self->fly->scale = AtLeast(scale, *reinterpret_cast<const float*>(0x009892F4));   // 1e-5
        FlyController_SetControlled(self->fly, controlled);
        FlyController_SetReference(self->fly, reference);
    }
    if (self->orbit != nullptr) {
        self->orbit->scale = AtLeast(scale, *reinterpret_cast<const float*>(0x00989388));   // 1e-5
        OrbitController_SetControlled(self->orbit, controlled);
        OrbitController_SetUpDir(self->orbit, upDir);
    }
    return 1;
}

// vf13 UpdateFrame: the UI updated, the navigation (and the camera with it), NiApplication's,
// then every 30 frames the frame rate shown
void __fastcall UpdateFrame(NiSample* self, re::Edx) {
    if (UIManager_Get() != nullptr) {
        UIManager_UpdateUI(UIManager_Get());
        if (self->useNavigation != 0) {
            NavManager_Update(NiNavManager_ms_pkNavManager);
            void* camera = At<void*>(self, app::kCamera);
            if (camera != nullptr) {
                AVObject_Update(camera, At<float>(self, app::kAccumTime), 1);
            }
        }
    }
    Application_UpdateFrame(self);
    if (self->frameRateLabel != nullptr && At<int32_t>(self, app::kFrameCount) % 30 == 0) {
        char buffer[0x20];
        NiSprintf(buffer, 0x20, "Frame Rate: %.1f", double(At<float>(self, app::kFrameRate)));
        NiUILabel* label = self->frameRateLabel;
        NiString_AssignText(&label->text, buffer);
        UILabel_ReformatText(label);
    }
}

// AddLogLine(text): a new line at the end (the total length counting it plus 2), the log
// updated; its index
uint32_t __fastcall AddLogLine(NiSample* self, re::Edx, const char* text) {
    uint32_t length = uint32_t(std::strlen(text));
    auto* line = static_cast<re::Ptr32<char>*>(NiNew(4));
    if (line != nullptr) {
        NiString_Construct(line, text);
    }
    uint32_t index = self->logLines.size;
    if (index >= self->logLines.maxSize) {
        PrimitiveArray_SetSize(&self->logLines, self->logLines.growBy + index);
    }
    PrimitiveArray_SetAt(&self->logLines, index, re::PtrCast<const re::Ptr32<void>*>(&line));
    self->logLength += length + 2;
    Sample_UpdateLog(self);
    return index;
}

// a NiDefaultClickRenderStep (0x3C) as the code inlines its constructor
void* NewClickRenderStep() {
    auto* step = static_cast<uint8_t*>(NiNew(0x3C));
    if (step == nullptr) {
        return nullptr;
    }
    ClickRenderStep_Construct(step);
    At<uint32_t>(step, 0) = kNiDefaultClickRenderStepVtable;
    At<uint32_t>(step, 0x38) = 0;
    At<uint32_t>(step, 0x30) = 0;
    At<uint32_t>(step, 0x34) = 0;
    At<uint32_t>(step, 0x2C) = kNiTPointerListRenderClickVtable;
    return step;
}

// vf33 CreateFrame: NiApplication's, then the UI manager's step and click and the cursor's step
// with the cursor click appended to the frame (a failed allocation isn't checked: the game's)
uint8_t __fastcall CreateFrame(NiSample* self, re::Edx) {
    if (!Application_CreateFrame(self)) {
        return 0;
    }
    void* click = NiNew(0x50);
    if (click != nullptr) {
        UIManagerRenderClick_Construct(click);
    }
    void* step = NewClickRenderStep();
    FixedString_Assign(&At<NiFixedString>(step, 0x18), &self->uiStepName);
    ClickRenderStep_AppendRenderClick(step, click);
    void* frame = At<void*>(self, 0xC);
    RenderFrame_AppendRenderStep(frame, step);
    step = NewClickRenderStep();
    FixedString_Assign(&At<NiFixedString>(step, 0x18), &self->cursorStepName);
    ClickRenderStep_AppendRenderClick(step, self->cursorClick);
    RenderFrame_AppendRenderStep(At<void*>(self, 0xC), step);
    return 1;
}

// vf48 CreateUIElements: the "Output Log" group with the frame-rate label, the log label (as
// many lines as +0x188 says) and the hint line, sized and handed to the UI manager; then the
// navigation line
uint8_t __fastcall CreateUIElements(NiSample* self, re::Edx) {
    uint32_t pixels[2];
    Renderer_ConvertFromNDCToPixels(*reinterpret_cast<re::Ptr32<void>*>(0x020E2380), 1.0f, 1.0f, &pixels[0], &pixels[1]);
    auto* group = static_cast<NiUIGroup*>(NiNew(0x110));
    if (group != nullptr) {
        UIGroup_Construct(group, "Output Log", self->uiHeaderHeight, 1);
    }
    // (NiNew's result is the constructed object: the constructor returns it)
    float width = UIManager_Get()->maxCharWidth * 35.0f;
    if (double(width) > 0.45) {
        width = 0.45f;
    }
    float charHeight = UIManager_Get()->maxCharHeight;
    // (the count loaded exact by fild, one rounding of the product)
    float logHeight = float(double(self->logLineCount) * double(charHeight));
    float groupWidth = ((self->uiBorderWidth + self->uiBorderWidth) + width) + 0.0f;
    float y = 0.0f + self->uiBorderHeight;

    auto* label = static_cast<NiUILabel*>(NiNew(0xDC));
    if (label != nullptr) {
        UILabel_Construct(label, kEmpty, kLabelBackground, kLabelText);
    }
    AssignRef(self->frameRateLabel, label);
    UIElement_SetOffset(self->frameRateLabel, self->uiBorderWidth, y);
    UIElement_SetDimensions(self->frameRateLabel, width, charHeight);
    self->frameRateLabel->verticalAlign = 0;
    self->frameRateLabel->horizontalAlign = 0;
    UILabel_UpdateTextPosition(self->frameRateLabel);
    Virtual<void>(group, 19, self->frameRateLabel);   // AddChild
    y = (charHeight + charHeight * 0.5f) + y;

    label = static_cast<NiUILabel*>(NiNew(0xDC));
    if (label != nullptr) {
        UILabel_Construct(label, kEmpty, kLabelBackground, kLabelText);
    }
    AssignRef(self->logLabel, label);
    UIElement_SetOffset(self->logLabel, self->uiBorderWidth, y);
    UIElement_SetDimensions(self->logLabel, width, logHeight);
    self->logLabel->verticalAlign = 0;
    self->logLabel->horizontalAlign = 0;
    UILabel_UpdateTextPosition(self->logLabel);
    Virtual<void>(group, 19, self->logLabel);
    y = (logHeight + charHeight) + y;

    Sample_AddLogLine(self,
                      "To hide the log and all UI elements, press 'Z' on a keyboard or left analog stick press on a "
                      "Gamepad.");
    UIElement_SetOffset(group, 0.0f, 0.0f);
    UIElement_SetDimensions(group, groupWidth, y);
    Virtual<void>(group, 12);   // ReinitializeDisplayElements
    UIManager_AddUIGroup(UIManager_Get(), group);
    self->uiNextX = 0.0f;
    self->uiNextY = y + charHeight;
    if (self->useNavigation != 0) {
        self->navLogLine = int32_t(Sample_AddLogLine(self, kEmpty));
        Virtual<void>(self, 59, NavManager_GetCurrentController(NiNavManager_ms_pkNavManager));
    }
    return 1;
}

// the constructor (caption, width, height, useFrameSystem, menu, status panes, bit depth)
NiSample* __fastcall Construct(NiSample* self, re::Edx, const char* caption, uint32_t width, uint32_t height,
                               int useFrameSystem, uint32_t menuID, uint32_t numStatusPanes, uint32_t bitDepth) {
    Application_Construct(self, caption, width, height, useFrameSystem, menuID, numStatusPanes, bitDepth);
    *reinterpret_cast<uint32_t*>(self) = kNiSampleVtable;
    self->orbit = nullptr;
    self->fly = nullptr;
    auto initSlot = [](NiUIMemberSlot& slot, uint32_t vtable) {
        slot.set.base = nullptr;
        slot.set.alloced = 0;
        slot.set.used = 0;
        slot.vtable = reinterpret_cast<void*>(vtable);
        slot.instance = nullptr;
        slot.adjust = 0;
        slot.function = nullptr;
    };
    initSlot(self->toggleUISlot, kNiUIMemberSlot1SampleVtable);
    initSlot(self->toggleNavHelpSlot, kNiUIMemberSlot0SampleVtable);
    initSlot(self->slot148, kNiUIMemberSlot0SampleVtable);
    initSlot(self->slot168, kNiUIMemberSlot0SampleVtable);
    self->uiTitleHeight = 0.0f;
    self->uiLabelWidth = 0.0f;
    self->logLineCount = 3;
    self->uiNextX = 0.0f;
    self->uiNextY = 0.0f;
    self->uiBorderWidth = 0.0f;
    self->uiBorderHeight = 0.0f;
    self->logLabel = nullptr;
    self->logLines.maxSize = 0;
    self->logLines.growBy = 1;
    self->logLines.size = 0;
    self->logLines.esize = 0;
    self->logLines.base = nullptr;
    self->logLines.vtable = reinterpret_cast<void*>(kStringArrayVtable);
    self->logLength = 0;
    self->navLogLine = -1;
    self->frameRateLabel = nullptr;
    self->showNavHelp = 0;
    self->navHelpGroups.maxSize = 0;
    self->navHelpGroups.growBy = 1;
    self->navHelpGroups.size = 0;
    self->navHelpGroups.esize = 0;
    self->navHelpGroups.base = nullptr;
    self->navHelpGroups.vtable = reinterpret_cast<void*>(kGroupArrayVtable);
    self->cursor = nullptr;
    self->cursorClick = nullptr;
    self->navUpDir.x = 0.0f;
    self->navUpDir.y = 0.0f;
    self->navUpDir.z = 1.0f;
    self->navScale = 300.0f;
    self->libraryCreators.growBy = 1;
    self->libraryCreators.maxSize = 0;
    self->libraryCreators.size = 0;
    self->libraryCreators.esize = 0;
    self->libraryCreators.base = nullptr;
    self->libraryCreators.vtable = reinterpret_cast<void*>(kCreatorArrayVtable);
    self->parserRunners.growBy = 1;
    self->parserRunners.maxSize = 0;
    self->parserRunners.size = 0;
    self->parserRunners.esize = 0;
    self->parserRunners.base = nullptr;
    self->parserRunners.vtable = reinterpret_cast<void*>(kParserArrayVtable);
    self->uiStepName = ni::sys::kNullFixedString;
    self->uiClickName = ni::sys::kNullFixedString;
    self->cursorStepName = ni::sys::kNullFixedString;
    self->cursorClickName = ni::sys::kNullFixedString;
    self->useNavigation = 1;
    self->toggleUISlot.instance = self;
    self->toggleUISlot.adjust = 0;
    self->toggleUISlot.function = reinterpret_cast<void*>(kToggleUIFunction);
    self->toggleNavHelpSlot.instance = self;
    self->toggleNavHelpSlot.adjust = 0;
    self->toggleNavHelpSlot.function = reinterpret_cast<void*>(kToggleNavHelpFunction);
    self->slot148.function = reinterpret_cast<void*>(kCycleNavFunction);
    self->slot148.instance = self;
    self->slot148.adjust = 0;
    self->slot168.instance = self;
    self->slot168.function = reinterpret_cast<void*>(kSlot168Function);
    self->slot168.adjust = 0;
    FixedString_AssignText(&self->uiStepName, "NiSample UI Manager Render Step");
    FixedString_AssignText(&self->uiClickName, "NiSample UI Manager Render Click");
    FixedString_AssignText(&self->cursorStepName, "NiSample Cursor Render Step");
    FixedString_AssignText(&self->cursorClickName, "NiSample Cursor Render Click");
    At<uint8_t>(self, app::kUnknownA0) = 1;
    // the cursor click (NiCursorRenderClick, 0x64): a render click with a list of cursors
    auto* click = static_cast<uint8_t*>(NiNew(0x64));
    if (click != nullptr) {
        RenderClick_Construct(click);
        At<uint32_t>(click, 0) = kNiCursorRenderClickVtable;
        At<uint32_t>(click, 0x58) = 0;
        At<uint32_t>(click, 0x50) = 0;
        At<uint32_t>(click, 0x54) = 0;
        At<uint32_t>(click, 0x4C) = kNiTPointerListCursorVtable;
        At<float>(click, 0x60) = 0.0f;
        At<uint32_t>(click, 0x5C) = 0;
    }
    AssignRef(self->cursorClick, reinterpret_cast<NiRefObject*>(click));
    FixedString_Assign(&At<NiFixedString>(self->cursorClick, 0x48), &self->cursorClickName);
    return self;
}

// vf0: the deleting destructor (the destructor itself sits in the game code's range, 0x4045F0)
NiSample* __fastcall DeletingDestructor(NiSample* self, re::Edx, int flags) {
    Sample_DestructBody(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x230);
    }
    return self;
}

// vf45 CreateUISystem: the UI manager made and initialized with the full skin, the sizes the
// labels use from its character size, the navigation manager, 'Z' and the left stick press
// reserved for the UI toggle, the UI hidden
uint8_t __fastcall CreateUISystem(NiSample* self, re::Edx) {
    UIManager_Create();
    NiUIManager* manager = UIManager_Get();
    if (manager == nullptr) {
        return 0;
    }
    if (!UIManager_Initialize(manager, At<NiRefObject*>(self, app::kInputSystem), "../../../../Data/UISkinFull.dds",
                              self->cursor)) {
        return 0;
    }
    self->uiTitleHeight = manager->maxCharHeight * 3.0f;
    float width = manager->maxCharWidth * 25.0f;
    if (double(width) > 0.4) {
        width = 0.4f;
    }
    self->uiLabelWidth = width;
    self->uiHeaderHeight = float(double(manager->maxCharHeight) * 2.75);
    self->uiBorderWidth = manager->maxCharWidth * 1.5f;
    self->uiBorderHeight = manager->maxCharHeight * 0.5f + self->uiHeaderHeight;
    if (self->useNavigation != 0 && !NavManager_Create()) {
        return 0;
    }
    UIManager_ReserveGamePadButton(UIManager_Get(), 0xC, &self->toggleUISlot, 2, 0);
    UIManager_ReserveKeyboardButton(UIManager_Get(), 0x2C, &self->toggleUISlot, 2, 0);
    UIManager_Get()->visible = 0;
    return 1;
}

} // namespace

#define NISAMPLE_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISAMPLE_REVERSED("NiSample::Initialize", 0x00830F80, &Initialize);
NISAMPLE_REVERSED("NiSample::CreateInputSystem", 0x008310B0, &CreateInputSystem);
NISAMPLE_REVERSED("NiSample::LibraryClassCreate", 0x008310D0, &LibraryClassCreate);
NISAMPLE_REVERSED("NiSample::RunParser", 0x00831130, &RunParser);
NISAMPLE_REVERSED("NiSample::CreateNavigationControllers", 0x00831170, &CreateNavigationControllers);
NISAMPLE_REVERSED("NiSample::CompleteUISystem", 0x00831270, &CompleteUISystem);
NISAMPLE_REVERSED("NiSample::RenderUIElements", 0x008312B0, &RenderUIElements);
NISAMPLE_REVERSED("NiSample::UpdateLog", 0x008312E0, &UpdateLog);
NISAMPLE_REVERSED("NiSample::ToggleNavHelp", 0x008313A0, &ToggleNavHelp);
NISAMPLE_REVERSED("NiSample::CycleNavigationController", 0x008313F0, &CycleNavigationController);
NISAMPLE_REVERSED("NiSample::LoadShaders", 0x00831740, &LoadShaders);
NISAMPLE_REVERSED("NiSample::RunShaderParsers", 0x00831860, &RunShaderParsers);
NISAMPLE_REVERSED("NiSample::RegisterShaderLibraries", 0x008318E0, &RegisterShaderLibraries);
NISAMPLE_REVERSED("NiSample::CreateCursor", 0x00831A50, &CreateCursor);
NISAMPLE_REVERSED("NiSample::RenderFrame", 0x00831B80, &RenderFrame);
NISAMPLE_REVERSED("NiSample::Terminate", 0x00831C30, &Terminate);
NISAMPLE_REVERSED("NiSample::UpdateInput", 0x00831DC0, &UpdateInput);
NISAMPLE_REVERSED("NiSample::SetLogLine", 0x00831E00, &SetLogLine);
NISAMPLE_REVERSED("NiSample::SetNavigationLogLine", 0x00831E70, &SetNavigationLogLine);
NISAMPLE_REVERSED("NiSample::SetNavigationParameters", 0x00832090, &SetNavigationParameters);
NISAMPLE_REVERSED("NiSample::UpdateFrame", 0x00832150, &UpdateFrame);
NISAMPLE_REVERSED("NiSample::AddLogLine", 0x00832210, &AddLogLine);
NISAMPLE_REVERSED("NiSample::CreateFrame", 0x00832490, &CreateFrame);
NISAMPLE_REVERSED("NiSample::CreateUIElements", 0x00832570, &CreateUIElements);
NISAMPLE_REVERSED("NiSample::NiSample", 0x00832A20, &Construct);
NISAMPLE_REVERSED("NiSample::DeletingDestructor", 0x00832DB0, &DeletingDestructor);
NISAMPLE_REVERSED("NiSample::CreateUISystem", 0x00832DE0, &CreateUISystem);

} // namespace ni::sample
