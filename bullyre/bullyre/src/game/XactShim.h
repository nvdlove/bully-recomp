#pragma once
#include "reversed.h"

// XACT 3 over FAudio's FACT (third_party/FAudio): an engine with IXACT3Engine's interface (and
// its sound banks, wave banks and cues with theirs), for the game's audio (MDXact*.cpp, the
// game's own code too) to use as it used XACT's, which is 32-bit only. XactShim.cpp.

namespace xact {

// a new engine (as CoCreateInstance(CLSID_XACTEngine) gave one: IXACT3Engine*, refcount 1)
long __stdcall CreateEngine(re::Ptr32<void>* engine);

} // namespace xact
