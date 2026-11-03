// bullyre_x64's play window (runflags.h re::PlayWindow): the game in an ordinary Windows window.
//
// - It sizes like any window (its edges, corners, maximize): bullygfx's resizer gives the game the
//   new client size as its video mode once the edge is let go (the game resets its device to it
//   and saves it, as its options menu does). A mode that doesn't fit the work area starts maximized.
// - The keyboard and mouse are shared with Windows (DirectInput non-exclusive: the Windows key
//   works, the cursor isn't the game's to hide and trap), and the game keeps running when another
//   window is in front (re::KeepActive): Alt+Tab or the Windows key frees the mouse without
//   pausing. While another window is in front the game reads no input (nothing typed elsewhere
//   reaches it).
// - The cursor is held in the window only while the game is being played (its window in front,
//   not paused, not minimized, the cursor already in the picture: clipping it on the title bar
//   would yank a window being dragged), and isn't drawn over the picture while the window is in
//   front.
// - Its window menu isn't reached from the keyboard (the game's keys are DirectInput's): Alt, the
//   run key, held with another key doesn't ding.

#include <windows.h>

#include <cstdint>

#include "reversed.h"
#include "runflags.h"

namespace {

HWND GameWindow() {
    return static_cast<HWND>(static_cast<void*>(*reinterpret_cast<re::Ptr32<void>*>(0x00BD77FC)));   // (RsGlobal's window)
}

// the simulation going: a player, a time step (0 paused: the pause menu, the front end, loading)
bool Playing() {
    uint32_t player = *reinterpret_cast<const uint32_t*>(0x00C1AEA8);
    float step = *reinterpret_cast<const float*>(0x00C1A9A0);
    return player != 0 && step > 0.0f;
}

bool InFront(HWND w) {
    HWND front = GetForegroundWindow();
    HWND root = w ? GetAncestor(w, GA_ROOT) : nullptr;
    return root && front && (front == root || IsChild(root, front));
}

bool g_clipped = false;
RECT g_clip{};

// the windows subclassed (the frame and the picture's child) and their own procedures
struct Subclassed {
    HWND window;
    WNDPROC proc;
};
Subclassed g_subclassed[4] = {};

WNDPROC GameProc(HWND w) {
    for (const Subclassed& s : g_subclassed) {
        if (s.window == w) {
            return s.proc;
        }
    }
    return g_subclassed[0].proc;
}

LRESULT CALLBACK PlayWindowProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_SETCURSOR && LOWORD(lp) == HTCLIENT && InFront(w)) {
        SetCursor(nullptr);   // (the picture is the game's: its own pointer where it has one)
        return TRUE;
    }
    if (msg == WM_ACTIVATEAPP && wp == FALSE && g_clipped) {
        ClipCursor(nullptr);   // (at once: the next frame's would be late for a click elsewhere)
        g_clipped = false;
    }
    // Alt held (the game's run key) with another key: Windows looks for the window menu's
    // shortcut (WM_SYSCHAR -> SC_KEYMENU), finds no menu and dings at every key. Its keyboard is
    // DirectInput's, so the window's menu keys go nowhere: F10 and Alt+Space too (Alt+Tab and
    // Alt+F4 are other paths)
    if (msg == WM_SYSCHAR) {
        return 0;
    }
    if (msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_KEYMENU) {
        return 0;
    }
    if (msg == WM_MENUCHAR) {
        return MAKELRESULT(0, MNC_CLOSE);
    }
    return CallWindowProcA(GameProc(w), w, msg, wp, lp);
}

} // namespace

namespace re {

bool GameInputAllowed() {
    return !PlayWindow() || ScriptedKeysActive() || InFront(GameWindow());   // (a test run's keys: its own)
}

void PlayWindowCreated(void* window) {
    HWND w = static_cast<HWND>(window);
    if (!PlayWindow() || !w) {
        return;
    }
    for (Subclassed& s : g_subclassed) {
        if (s.window == w) {
            return;
        }
        if (!s.window) {
            s.proc = reinterpret_cast<WNDPROC>(GetWindowLongPtrA(w, GWLP_WNDPROC));
            s.window = w;
            SetWindowLongPtrA(w, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PlayWindowProc));
            return;
        }
    }
}

void PlayWindowFrame() {
    if (!PlayWindow()) {
        return;
    }
    HWND w = GameWindow();
    bool hold = false;
    RECT r{};
    if (w && InFront(w) && !IsIconic(w) && Playing()) {
        RECT c;
        GetClientRect(w, &c);
        POINT o{0, 0};
        ClientToScreen(w, &o);
        r = {o.x, o.y, o.x + c.right, o.y + c.bottom};
        POINT p;
        GetCursorPos(&p);
        bool inside = p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom;
        hold = inside || g_clipped;
    }
    if (hold && (!g_clipped || !EqualRect(&r, &g_clip))) {
        ClipCursor(&r);
        g_clip = r;
        g_clipped = true;
    } else if (!hold && g_clipped) {
        ClipCursor(nullptr);
        g_clipped = false;
    }
}

} // namespace re
