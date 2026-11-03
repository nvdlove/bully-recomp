// MdMoviePlayer: the logo movies, played with DirectShow (a filter graph rendering the
// file into a windowless Video Mixing Renderer on the game's window).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace c01movie {

// The imports, read at call time (the mod loader may hook them)
using CloseHandle_t = int(__stdcall*)(void*);
using CoInitialize_t = long(__stdcall*)(void*);
using CoUninitialize_t = void(__stdcall*)();
using CoCreateInstance_t = long(__stdcall*)(const void* clsid, void* outer, uint32_t context, const void* iid, void** out);
using GetClientRect_t = int(__stdcall*)(void* window, void* rect);
GAME_VAR(CloseHandle_t, Import_CloseHandle, 0x008FC050);
GAME_VAR(CoInitialize_t, Import_CoInitialize, 0x008FC338);
GAME_VAR(CoUninitialize_t, Import_CoUninitialize, 0x008FC33C);
GAME_VAR(CoCreateInstance_t, Import_CoCreateInstance, 0x008FC32C);
GAME_VAR(GetClientRect_t, Import_GetClientRect, 0x008FC22C);

// The GUIDs, the game's own
inline const void* const CLSID_FilterGraph = reinterpret_cast<const void*>(0x0098CB8C);
inline const void* const CLSID_VideoMixingRenderer = reinterpret_cast<const void*>(0x0098CB7C);
inline const void* const IID_IGraphBuilder = reinterpret_cast<const void*>(0x0098CB3C);
inline const void* const IID_IBaseFilter = reinterpret_cast<const void*>(0x0098CB4C);
inline const void* const IID_IVMRFilterConfig = reinterpret_cast<const void*>(0x0098CB1C);
inline const void* const IID_IVMRWindowlessControl = reinterpret_cast<const void*>(0x0098CB2C);
inline const void* const IID_IMediaControl = reinterpret_cast<const void*>(0x0098CB6C);
inline const void* const IID_IMediaEvent = reinterpret_cast<const void*>(0x0098CB5C);
inline const wchar_t* const vmrFilterName = reinterpret_cast<const wchar_t*>(0x00900590);   // L"Video Mixing Renderer"

GAME_VAR(void*, RsGlobal_window, 0x00BD77FC);
GAME_VAR(uint8_t, ms_bLogoMoviesPlaying, 0x00BCBB50);
GAME_VAR(uint8_t*, ms_pVideoSettings_20E2380, 0x020E2380);   // (+0xA4: movies enabled)

GAME_FN(void, __cdecl, SetLoadingScreenPaused, 0x0043ACD0, int paused);
GAME_FN(void, __cdecl, Pads_Update, 0x00738610);
GAME_FN(void, __cdecl, Pads_Clear, 0x00738630);
GAME_FN(uint8_t, __cdecl, Pad_IsBeingPressed, 0x00738900, int button, int pad);
GAME_FN(const char*, __cdecl, FixPath, 0x00829900, const char* path);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline int __cdecl MbsToWcs(wchar_t* out, const char* in, size_t count) { return (int)crt::Mbstowcs(out, in, count); }   // the CRT's mbstowcs
inline int __cdecl AtExit(void(__cdecl* fn)()) { return crt::Atexit((void(__cdecl*)())fn); }
GAME_FN(void, __cdecl, MoviePlayer_DestroyStatic, 0x008F1310);   // (atexit: the static's destructor)

// A COM method: vtable slot `offset` of `object` (stdcall, the object first)
template <typename R = long, typename... A>
R Com(void* object, int offset, A... args) {
    return re::ComCall<R>(object, offset / 4, args...);
}
void Release(void* object) {
    Com<unsigned long>(object, 8);
}

struct MdMoviePlayer {
    re::Ptr32<re::Ptr32<void>> vtable;   // +0 (0x90055C; the Imp's 0x900578)
};
inline re::Ptr32<void>* const MdMoviePlayer_vftable = reinterpret_cast<re::Ptr32<void>*>(0x0090055C);
inline re::Ptr32<void>* const MdMoviePlayerImp_vftable = reinterpret_cast<re::Ptr32<void>*>(0x00900578);

struct MdMoviePlayerImp : MdMoviePlayer {
    int32_t state;          // +4 1 while playing
    uint8_t comInitialised; // +8 (CoInitialize succeeded: CoUninitialize at the end)
    uint8_t unknown9[3];
    re::Ptr32<void> graph;            // +0xC IGraphBuilder
    re::Ptr32<void> control;          // +0x10 IMediaControl
    re::Ptr32<void> event;            // +0x14 IMediaEvent
    re::Ptr32<void> windowless;       // +0x18 IVMRWindowlessControl
    re::Ptr32<void> file;             // +0x1C a HANDLE, -1 for none
    uint32_t flags;         // +0x20 (2: a button skips it)
};
static_assert(sizeof(MdMoviePlayerImp) == 0x24);
static_assert(offsetof(MdMoviePlayerImp, graph) == 0xC);
static_assert(offsetof(MdMoviePlayerImp, flags) == 0x20);

GAME_FN(void, __thiscall, MoviePlayerImp_Cleanup, 0x00410C90, MdMoviePlayerImp* self);

// the virtuals, through the vtable as the game calls them
uint8_t VOpen(MdMoviePlayerImp* self, const char* path, uint32_t flags) {
    return reinterpret_cast<uint8_t(__thiscall*)(MdMoviePlayerImp*, const char*, uint32_t)>(static_cast<void*>(self->vtable[2]))(self, path, flags);
}
void VStop(MdMoviePlayerImp* self) {
    reinterpret_cast<void(__thiscall*)(MdMoviePlayerImp*)>(static_cast<void*>(self->vtable[3]))(self);
}
void VUpdate(MdMoviePlayerImp* self) {
    reinterpret_cast<void(__thiscall*)(MdMoviePlayerImp*)>(static_cast<void*>(self->vtable[5]))(self);
}

// 0x410C70 (thiscall, vf0): the base's scalar deleting destructor
MdMoviePlayer* __fastcall MoviePlayer_Destroy(MdMoviePlayer* self, re::Edx, uint8_t flags) {
    self->vtable = MdMoviePlayer_vftable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x410C90 (thiscall): everything released, the state back to stopped
void __fastcall Cleanup(MdMoviePlayerImp* self, re::Edx) {
    if (self->file != reinterpret_cast<void*>(-1)) {
        Import_CloseHandle(self->file);
        self->file = reinterpret_cast<void*>(-1);
    }
    self->flags = 0;
    self->state = 0;
    if (self->graph) {
        Release(self->graph);
        self->graph = nullptr;
    }
    if (self->control) {
        Release(self->control);
        self->control = nullptr;
    }
    if (self->event) {
        Release(self->event);
        self->event = nullptr;
    }
    if (self->windowless) {
        Release(self->windowless);
        self->windowless = nullptr;
    }
    if (self->comInitialised) {
        Import_CoUninitialize();
        self->comInitialised = 0;
    }
    SetLoadingScreenPaused(0);
}

// 0x410D20 (thiscall, vf1): plays a movie to its end (or until a button skips it,
// when flags has 2)
uint8_t __fastcall PlayAndWait(MdMoviePlayerImp* self, re::Edx, const char* path, uint32_t flags) {
    if (!VOpen(self, path, flags)) {
        return 0;
    }
    while (self->state != 0) {
        if (self->flags & 2) {
            Pads_Update();
            bool pressed = false;
            // buttons 4..13 on pads 0..3, in that order
            for (int pad = 0; pad < 4 && !pressed; pad++) {
                for (int button = 4; button <= 13 && !pressed; button++) {
                    pressed = Pad_IsBeingPressed(button, pad) != 0;
                }
            }
            if (pressed || ms_bLogoMoviesPlaying) {
                Pads_Clear();
                VStop(self);
            }
        }
        if (self->state == 1) {
            VUpdate(self);
        }
    }
    return 1;
}

// 0x4110A0 (thiscall, vf2): builds the graph and starts the movie
uint8_t __fastcall Open(MdMoviePlayerImp* self, re::Edx, const char* path, uint32_t flags) {
    if (!path || self->state != 0) {
        return 0;
    }
    long hr = Import_CoInitialize(nullptr);
    if (hr >= 0) {
        self->comInitialised = 1;
    } else if (hr != static_cast<long>(0x80010106)) {   // RPC_E_CHANGED_MODE: initialised already
        return 0;
    }
    if (*reinterpret_cast<int32_t*>(ms_pVideoSettings_20E2380 + 0xA4) == 0) {
        MoviePlayerImp_Cleanup(self);
        return 0;
    }
    SetLoadingScreenPaused(1);
    // (re::OutPtr: the system writes a native pointer, the slot is 4 bytes in the 64-bit build)
    if (Import_CoCreateInstance(CLSID_FilterGraph, nullptr, 1, IID_IGraphBuilder, re::OutPtr(&self->graph)) < 0) {
        MoviePlayerImp_Cleanup(self);
        return 0;
    }
    re::Ptr32<void> vmr = nullptr;
    if (Import_CoCreateInstance(CLSID_VideoMixingRenderer, nullptr, 1, IID_IBaseFilter, re::OutPtr(&vmr)) < 0) {
        MoviePlayerImp_Cleanup(self);
        return 0;
    }
    if (Com(self->graph, 0xC, vmr, vmrFilterName) < 0) {   // AddFilter
        Release(vmr);
        MoviePlayerImp_Cleanup(self);
        return 0;
    }
    void* config = nullptr;
    if (Com(vmr, 0, IID_IVMRFilterConfig, &config) >= 0) {
        long mode = Com(config, 0x20, 2);   // SetRenderingMode(VMRMode_Windowless)
        Release(config);
        if (mode >= 0 && Com(vmr, 0, IID_IVMRWindowlessControl, &self->windowless) >= 0) {
            Com(self->windowless, 0x28, RsGlobal_window);   // SetVideoClippingWindow
            int32_t rect[4];
            Import_GetClientRect(RsGlobal_window, rect);
            if (Com(self->windowless, 0x18, static_cast<void*>(nullptr), static_cast<void*>(rect)) < 0) {   // SetVideoPosition
                // (released but kept: Cleanup releases it again)
                Release(self->windowless);
            }
        }
    }
    Release(vmr);
    if (Com(self->graph, 0, IID_IMediaControl, &self->control) < 0) {
        MoviePlayerImp_Cleanup(self);
        return 0;
    }
    Com(self->graph, 0, IID_IMediaEvent, &self->event);
    const char* fullPath = FixPath(path);
    size_t count = std::strlen(fullPath) + 1;
    // (the size overflow-checked: all ones when count * 2 overflows)
    uint64_t bytes = static_cast<uint64_t>(count) * 2;
    wchar_t* wide = static_cast<wchar_t*>(Malloc((bytes >> 32) ? SIZE_MAX : static_cast<size_t>(bytes)));
    if (MbsToWcs(wide, fullPath, count) < 1) {
        Free(wide);
        return 0;
    }
    Com(self->graph, 0x34, static_cast<const wchar_t*>(wide), static_cast<void*>(nullptr));   // RenderFile
    Com(self->control, 0x1C);   // Run
    self->flags = flags;
    self->state = 1;
    Free(wide);
    return 1;
}

// 0x4112E0 (thiscall, vf3): stops the movie
void __fastcall Stop(MdMoviePlayerImp* self, re::Edx) {
    if (self->control) {
        Com(self->control, 0x24);   // Stop
        MoviePlayerImp_Cleanup(self);
    }
}

// 0x411300 (thiscall, vf5): ends it when the graph reports EC_COMPLETE
void __fastcall Update(MdMoviePlayerImp* self, re::Edx) {
    if (self->control && self->state == 1) {
        // (the game's slot for it is where `this` was pushed: what it holds when
        // WaitForCompletion leaves it)
        int32_t code = static_cast<int32_t>(reinterpret_cast<uintptr_t>(self));
        Com(self->event, 0x24, 0, &code);   // WaitForCompletion(0, &code)
        if (code == 1) {
            MoviePlayerImp_Cleanup(self);
        }
    }
}

// 0x411340 (thiscall, vf0): the scalar deleting destructor
MdMoviePlayerImp* __fastcall Destroy(MdMoviePlayerImp* self, re::Edx, uint8_t flags) {
    self->vtable = MdMoviePlayerImp_vftable;
    MoviePlayerImp_Cleanup(self);
    self->vtable = MdMoviePlayer_vftable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x411370: the player (a function-local static)
GAME_VAR(uint32_t, playerGuard, 0x00BCC160);
GAME_VAR(MdMoviePlayerImp, thePlayer, 0x00BCC13C);

MdMoviePlayerImp* __cdecl Get() {
    if (!(playerGuard & 1)) {
        playerGuard |= 1;
        thePlayer.vtable = MdMoviePlayerImp_vftable;
        thePlayer.state = 0;
        thePlayer.graph = nullptr;
        thePlayer.control = nullptr;
        thePlayer.event = nullptr;
        thePlayer.windowless = nullptr;
        thePlayer.file = reinterpret_cast<void*>(-1);
        thePlayer.flags = 0;
        thePlayer.comInitialised = 0;
        AtExit(MoviePlayer_DestroyStatic);
    }
    return &thePlayer;
}

// 0x8F1310 (atexit): the player's destructor (the Imp's vtable, cleaned up, the base's vtable)
void __cdecl DestroyStatic() {
    thePlayer.vtable = MdMoviePlayerImp_vftable;
    MoviePlayerImp_Cleanup(&thePlayer);
    thePlayer.vtable = MdMoviePlayer_vftable;
}

} // namespace c01movie

// ---- registration ----------------------------------------------------------------------

#define MOVIE_REVERSED_C01(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01movie {
MOVIE_REVERSED_C01("MdMoviePlayer::~MdMoviePlayer", 0x00410C70, &MoviePlayer_Destroy);
MOVIE_REVERSED_C01("MdMoviePlayerImp::Cleanup", 0x00410C90, &Cleanup);
MOVIE_REVERSED_C01("MdMoviePlayerImp::PlayAndWait", 0x00410D20, &PlayAndWait);
MOVIE_REVERSED_C01("MdMoviePlayerImp::Open", 0x004110A0, &Open);
MOVIE_REVERSED_C01("MdMoviePlayerImp::Stop", 0x004112E0, &Stop);
MOVIE_REVERSED_C01("MdMoviePlayerImp::Update", 0x00411300, &Update);
MOVIE_REVERSED_C01("MdMoviePlayerImp::~MdMoviePlayerImp", 0x00411340, &Destroy);
MOVIE_REVERSED_C01("MdMoviePlayer::Get", 0x00411370, &Get);
MOVIE_REVERSED_C01("MdMoviePlayer::DestroyStatic", 0x008F1310, &DestroyStatic);
} // namespace c01movie
