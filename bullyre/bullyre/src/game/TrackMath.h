#pragma once

// The C runtime maths the action tree's tracks of 0x611E50..0x621660 call (the game's code
// reaches VC8's on the x87 stack: cos 0x85AD80, sin 0x85AEF0, asin 0x85D460, pow 0x85CB70,
// atan2 0x85CF1A): Crt.h's, the result rounded to float as the game stores it.
// Tracks*.cpp.

#include "game/Crt.h"

namespace trk {

inline float CrtCos(float x) {
    return crt::Cosf(x);
}
inline float CrtSin(float x) {
    return crt::Sinf(x);
}
inline float CrtAsin(float x) {
    return crt::Asinf(x);
}
inline float CrtPow(float x, float y) {
    return crt::Powf(x, y);
}
// atan2(y, x)
inline float CrtAtan2(float y, float x) {
    return crt::Atan2f(y, x);
}

} // namespace trk
