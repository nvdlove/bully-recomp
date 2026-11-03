#pragma once

// The loading screen's helpers that take their arguments in registers (not hooked, so
// called directly), and the splash frame the game's main function has inlined.

#include <cstdint>

namespace LoadingScreen {

// 0x43AD80: the loading screen's texture name (`name` in esi)
char* LoadingScreenName(const char* name);
// 0x43BA00: the fading pictures and the sprite colour (the colour in edi)
void DrawFade(uint8_t* colour);
// RenderSplash(1, 0) (0x43C060) as inlined in 0x43CF20; `calls404F40`: the copies that
// call 0x404F40 rather than test its two flags themselves
void RenderSplashInlined(bool calls404F40);

} // namespace LoadingScreen
