#pragma once
// The 64-bit build's C runtime state of the game's (src/x64/crt64.cpp).
#include <cstddef>

namespace x64crt {
// the game's exit list (atexit), run last-first
void RunExitList();
size_t ExitListSize();
} // namespace x64crt
