#pragma once

// The IJG JPEG library, release 6b, built from its source (third_party/jpeg-6b) as the game's
// copy was: `boolean` is a byte, RGB pixels are 4 bytes (RGBx), the float DCT only.
// JpegLibrary.cpp puts it in place of the game's copy.

#include <cstddef>
#include <cstdio>

extern "C" {
#include "jpeglib.h"
}
