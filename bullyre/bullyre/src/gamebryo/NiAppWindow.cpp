// NiAppWindow (the application's main window, its render window and status bar) and NiCommand
// (the command line split into arguments, looked up as "-name" options), src/gamebryo/
// NiApplication.h. (0x82B850 and 0x82BA80, the status bar and the windows' creation, aren't on
// the reach list; written with the rest of the class.)

#include <windows.h>

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiApplication.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"
#include "runflags.h"

namespace ni::app {
namespace {

using sys::Import;

constexpr uintptr_t NiAppWindow_vtable = 0x00968C60;
inline const char* const kDefaultCaption = reinterpret_cast<const char*>(0x00BC8D70);   // "Gamebryo Application"
inline const char* const kWindowClass = reinterpret_cast<const char*>(0x00BC8D70);      // (the same string)
inline const char* const kOptionNotFound = reinterpret_cast<const char*>(0x00BC8D88);   // "option not found"

// Win32 through the game's import slots
constexpr uintptr_t kInitCommonControlsEx = 0x008FC01C;
constexpr uintptr_t kMoveWindow = 0x008FC220;
constexpr uintptr_t kShowWindow = 0x008FC224;
constexpr uintptr_t kGetWindowRect = 0x008FC264;
constexpr uintptr_t kIsWindow = 0x008FC268;
constexpr uintptr_t kSendMessageA = 0x008FC26C;
constexpr uintptr_t kCreateWindowExA = 0x008FC274;
constexpr uintptr_t kAdjustWindowRect = 0x008FC278;
constexpr uintptr_t kDestroyWindow = 0x008FC280;
constexpr uintptr_t kUpdateWindow = 0x008FC29C;

struct RECT {
    int32_t left, top, right, bottom;
};

GAME_FN(void*, __cdecl, NiMalloc_, 0x00752300, uint32_t size);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, AppWindow_AddStatusPanes, 0x0082B850, NiAppWindow* self);
GAME_FN(void, __thiscall, AppWindow_RemoveStatusPanes, 0x0082B9C0, NiAppWindow* self);
GAME_FN(int32_t, __thiscall, Command_Index, 0x0082BC10, NiCommand* self, const char* name, uint8_t* found);

inline size_t Length(const char* s) {
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

// the status bar's window (the panes asked for and the array made)
inline void* StatusWindow(const NiAppWindow* self) {
    if (self->numStatusPanes == 0 || self->statusWindow == nullptr) {
        return nullptr;
    }
    return self->statusWindow[0];
}

// ---- NiAppWindow

// 0x82B7B0: NiAppWindow(caption, width, height, status panes, window style): the caption copied
// ("Gamebryo Application" when none), the sizes as both the parent's and the window's
NiAppWindow* __fastcall AppWindow_Construct(NiAppWindow* self, re::Edx, const char* caption, uint32_t width,
                                            uint32_t height, uint32_t numStatusPanes, uint32_t windowStyle) {
    self->vtable = reinterpret_cast<void*>(NiAppWindow_vtable);
    if (caption == nullptr || *caption == '\0') {
        caption = kDefaultCaption;
    }
    size_t size = Length(caption) + 1;
    auto* copy = static_cast<char*>(NiMalloc_(uint32_t(size)));
    self->caption = copy;
    crt::StrcpyS(copy, size, caption);
    self->width = width;
    self->parentWidth = width;
    self->height = height;
    self->parentHeight = height;
    self->windowStyle = windowStyle;
    self->window = nullptr;
    self->renderWindow = nullptr;
    self->statusWindow = nullptr;
    self->statusPanes = 0;
    self->numStatusPanes = numStatusPanes;
    return self;
}

// 0x82B850: AddStatusPanes (a guess): none when fullscreen; else the status bar made under the
// window and the window grown by its height
void __fastcall AppWindow_AddStatusPanesImpl(NiAppWindow* self, re::Edx) {
    if (ms_pkApplication->fullscreen) {
        self->numStatusPanes = 0;
    }
    if (self->numStatusPanes == 0 || self->statusPanes) {
        return;
    }
    struct INITCOMMONCONTROLSEX {
        uint32_t size;
        uint32_t classes;
    } controls = {8, 4};   // ICC_BAR_CLASSES
    Import<int>(kInitCommonControlsEx, &controls);
    void* status = Import<void*>(kCreateWindowExA, 0u, "msctls_statusbar32", "", 0x50000000u, 0, 0, 0, 0,
                                 self->window, static_cast<void*>(nullptr), ms_pInstance, static_cast<void*>(nullptr));
    if (self->numStatusPanes != 0) {
        if (self->statusWindow == nullptr) {
            self->statusWindow = static_cast<re::Ptr32<void>*>(NiMalloc_(self->numStatusPanes * 4));
        }
        self->statusWindow[0] = status;
    }
    RECT statusRect;
    Import<int>(kGetWindowRect, StatusWindow(self), &statusRect);
    RECT rect;
    Import<int>(kGetWindowRect, self->window, &rect);
    int32_t height = statusRect.bottom - statusRect.top - rect.top + rect.bottom;
    self->statusPanes = 1;
    Import<int>(kMoveWindow, self->window, rect.left, rect.top, rect.right - rect.left, height, 1);
}

// 0x82B950: SetStatusPaneText(pane, text)
void __fastcall AppWindow_SetStatusPaneText(NiAppWindow* self, re::Edx, uint32_t pane, const char* text) {
    if (pane >= self->numStatusPanes || !self->statusPanes) {
        return;
    }
    Import<intptr_t>(kSendMessageA, StatusWindow(self), 0x401u, uintptr_t(pane), reinterpret_cast<intptr_t>(text));
    Import<intptr_t>(kSendMessageA, StatusWindow(self), 0xFu, uintptr_t(0), intptr_t(0));
}

// 0x82B9C0: RemoveStatusPanes (a guess): the status bar destroyed and the window shrunk by its
// height
void __fastcall AppWindow_RemoveStatusPanesImpl(NiAppWindow* self, re::Edx) {
    if (self->numStatusPanes == 0 || !self->statusPanes) {
        return;
    }
    RECT statusRect;
    Import<int>(kGetWindowRect, StatusWindow(self), &statusRect);
    RECT rect;
    Import<int>(kGetWindowRect, self->window, &rect);
    Import<int>(kDestroyWindow, StatusWindow(self));
    if (self->numStatusPanes != 0) {
        if (self->statusWindow == nullptr) {
            self->statusWindow = static_cast<re::Ptr32<void>*>(NiMalloc_(self->numStatusPanes * 4));
        }
        self->statusWindow[0] = nullptr;
    }
    int32_t height = statusRect.top - statusRect.bottom - rect.top + rect.bottom;
    self->statusPanes = 0;
    Import<int>(kMoveWindow, self->window, rect.left, rect.top, rect.right - rect.left, height, 1);
}

// 0x82BA80: CreateMainWindow(show, parent) (slot 0): the main window (sized for the client area
// unless fullscreen: then a popup), its status bar, the render window filling it; shown
void* __fastcall AppWindow_CreateMainWindow(NiAppWindow* self, re::Edx, int show, void* parent) {
    NiApplication* app = ms_pkApplication;
    RECT rect = {0, 0, int32_t(self->parentWidth), int32_t(self->parentHeight)};
    if (!app->fullscreen) {
        Import<int>(kAdjustWindowRect, &rect, self->windowStyle, app->menuID);
        app = ms_pkApplication;
    }
    uint32_t renderStyle = 0x40000000;   // WS_CHILD
    void* window;
    if (app->fullscreen) {
        window = Import<void*>(kCreateWindowExA, 0u, kWindowClass, self->caption, 0x80000000u, 0, 0,
                               rect.right - rect.left, rect.bottom - rect.top, parent, static_cast<void*>(nullptr),
                               ms_pInstance, static_cast<void*>(nullptr));
    } else if (re::PlayWindow()) {
        // (bullyre_x64 played, runflags.h: an ordinary window, sized at its edges and maximized like
        // any other; maximized from the start when the game's mode doesn't fit the work area)
        uint32_t style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
        ::RECT outer = {0, 0, int32_t(self->parentWidth), int32_t(self->parentHeight)};
        AdjustWindowRect(&outer, style, FALSE);
        ::RECT work{};
        if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0) &&
            (outer.right - outer.left > work.right - work.left || outer.bottom - outer.top > work.bottom - work.top)) {
            show = SW_SHOWMAXIMIZED;
        }
        renderStyle = 0x50000000;   // WS_CHILD | WS_VISIBLE
        window = Import<void*>(kCreateWindowExA, 0u, kWindowClass, self->caption, style, 0, 0, outer.right - outer.left,
                               outer.bottom - outer.top, parent, static_cast<void*>(nullptr), ms_pInstance, static_cast<void*>(nullptr));
        re::PlayWindowCreated(window);
    } else {
        renderStyle = 0x50000000;   // WS_CHILD | WS_VISIBLE
        window = Import<void*>(kCreateWindowExA, 0u, kWindowClass, self->caption, self->windowStyle, 0, 0,
                               rect.right - rect.left, rect.bottom - rect.top, parent, static_cast<void*>(nullptr),
                               ms_pInstance, static_cast<void*>(nullptr));
    }
    self->window = window;
    if (self->numStatusPanes != 0) {
        AppWindow_AddStatusPanes(self);
    }
    self->renderWindow = Import<void*>(kCreateWindowExA, 0u, kWindowClass, static_cast<const char*>(nullptr),
                                       renderStyle, 0, 0, self->parentWidth, self->parentHeight, window,
                                       static_cast<void*>(nullptr), ms_pInstance, static_cast<void*>(nullptr));
    if (re::PlayWindow()) {
        re::PlayWindowCreated(self->renderWindow);   // (the picture's child: the keys can be its)
    }
    Import<int>(kShowWindow, self->window, show);
    Import<int>(kUpdateWindow, self->window);
    return window;
}

// 0x82BB90: ~NiAppWindow (the status bar removed while the window lives; the caption freed)
void __fastcall AppWindow_Destruct(NiAppWindow* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiAppWindow_vtable);
    if (self->statusPanes) {
        if (Import<int>(kIsWindow, self->window) != 0) {
            AppWindow_RemoveStatusPanes(self);
        }
        if (self->statusWindow != nullptr) {
            NiFree(self->statusWindow);
        }
    }
    NiFree(self->caption);
    self->window = nullptr;
    self->renderWindow = nullptr;
}

// ---- NiCommand

// 0x82BBE0: ~NiCommand
void __fastcall Command_Destruct(NiCommand* self, re::Edx) {
    NiFree(self->used);
    if (self->commandLine != nullptr) {
        NiFree(self->argv);
        NiFree(self->commandLine);
    }
}

// 0x82BC10: Index(name, found&) (a guess): the first unused "-name" argument, marked used (0 and
// "option not found" when there's none)
int32_t __fastcall Command_IndexImpl(NiCommand* self, re::Edx, const char* name, uint8_t* found) {
    if (self == nullptr) {
        return 0;
    }
    *found = 0;
    for (int32_t i = 1; i < self->argc; i++) {
        const char* argument = self->argv[i];
        if (self->used[i] == 0 && argument[0] == '-' && crt::Strcmp(name, argument + 1) == 0) {
            self->used[i] = 1;
            *found = 1;
            return i;
        }
    }
    self->error = kOptionNotFound;
    return 0;
}

// 0x82BCD0: NiCommand(command line): a copy (line breaks made spaces) cut into arguments at
// spaces and tabs, quoted ones whole; argv[0] the copy itself (nothing when the line is empty:
// the rest not even cleared)
NiCommand* __fastcall Command_Construct(NiCommand* self, re::Edx, const char* commandLine) {
    self->argc = 0;
    self->argv = nullptr;
    self->used = nullptr;
    self->commandLine = nullptr;
    if (commandLine == nullptr || Length(commandLine) == 0) {
        return self;
    }
    size_t size = Length(commandLine) + 1;
    auto* copy = static_cast<char*>(NiMalloc_(uint32_t(size)));
    self->commandLine = copy;
    crt::StrcpyS(copy, size, commandLine);
    for (size_t i = 0; i < Length(commandLine) + 1; i++) {
        char& c = self->commandLine[i];
        if (c == '\r' || c == '\n') {
            c = ' ';
        }
    }
    struct Node {
        re::Ptr32<char> argument;
        re::Ptr32<Node> next;
    };
    Node* list = nullptr;
    char* p = self->commandLine;
    if (p != nullptr) {
        while (*p != '\0') {
            self->argc++;
            auto* node = static_cast<Node*>(NiMalloc_(8));
            node->next = list;
            if (*p == '"') {
                p++;
                node->argument = p;
                while (*p != '\0' && *p != '"') {
                    p++;
                }
            } else {
                node->argument = p;
                while (*p != '\0' && *p != ' ' && *p != '\t') {
                    p++;
                }
            }
            if (*p != '\0') {
                *p = '\0';
                p++;
            }
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            list = node;
        }
    }
    self->argc++;
    self->argv = static_cast<re::Ptr32<char>*>(NiMalloc_(uint32_t(self->argc) * 4));
    self->argv[0] = self->commandLine;
    int32_t index = self->argc - 1;
    while (list != nullptr) {
        self->argv[index] = list->argument;
        Node* next = list->next;
        NiFree(list);
        index--;
        list = next;
    }
    self->used = static_cast<uint8_t*>(NiMalloc_(uint32_t(self->argc)));
    crt::Memset(self->used, 0, uint32_t(self->argc));
    self->unknown10 = 0;   // (0.0f)
    self->unknown14 = 0;
    self->unknown18[0] = 0;
    self->unknown18[1] = 0;
    self->unknown18[2] = 0;
    self->unknown18[3] = 0;
    return self;
}

// 0x82BE90: Boolean(name): the option's index (0: not given)
int32_t __fastcall Command_Boolean(NiCommand* self, re::Edx, const char* name) {
    if (self == nullptr) {
        return 0;
    }
    uint8_t found;
    return Command_Index(self, name, &found);
}

}   // namespace
}   // namespace ni::app

// ---- registration ---------------------------------------------------------------------

#define C33_APPWINDOW_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_APPWINDOW_REVERSED("NiAppWindow::NiAppWindow", 0x0082B7B0, &ni::app::AppWindow_Construct);
C33_APPWINDOW_REVERSED("NiAppWindow::AddStatusPanes", 0x0082B850, &ni::app::AppWindow_AddStatusPanesImpl);
C33_APPWINDOW_REVERSED("NiAppWindow::SetStatusPaneText", 0x0082B950, &ni::app::AppWindow_SetStatusPaneText);
C33_APPWINDOW_REVERSED("NiAppWindow::RemoveStatusPanes", 0x0082B9C0, &ni::app::AppWindow_RemoveStatusPanesImpl);
C33_APPWINDOW_REVERSED("NiAppWindow::CreateMainWindow", 0x0082BA80, &ni::app::AppWindow_CreateMainWindow);
C33_APPWINDOW_REVERSED("NiAppWindow::~NiAppWindow", 0x0082BB90, &ni::app::AppWindow_Destruct);
C33_APPWINDOW_REVERSED("NiCommand::~NiCommand", 0x0082BBE0, &ni::app::Command_Destruct);
C33_APPWINDOW_REVERSED("NiCommand::Index", 0x0082BC10, &ni::app::Command_IndexImpl);
C33_APPWINDOW_REVERSED("NiCommand::NiCommand", 0x0082BCD0, &ni::app::Command_Construct);
C33_APPWINDOW_REVERSED("NiCommand::Boolean", 0x0082BE90, &ni::app::Command_Boolean);
