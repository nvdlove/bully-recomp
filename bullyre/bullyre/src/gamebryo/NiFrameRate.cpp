// NiFrameRate (src/gamebryo/NiApplication.h): the frame-rate counter NiApplication's UpdateFrame
// drives: a sample per frame (the rate over the last 20 frames, the frame time sums, extremes and
// histogram), the three digit quads' texture coordinates set when the number changes and the
// digits placed on the render target; its shutdown and destructor. Plus
// NiTexturingProperty::SetBaseFilterMode, which sits among them.
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's order.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiApplication.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::app {
namespace {

using sys::At;
using sys::Virtual;

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);

GAME_FN(float, __cdecl, NiGetCurrentTimeInSec, 0x007537B0);
GAME_FN(void, __thiscall, ScreenElementsData_SetTexture, 0x007891F0, void* self, int32_t polygon, uint32_t vertex,
        uint32_t textureSet, const float* uv);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void*, __thiscall, TexturingMap_Construct, 0x00765BA0, void* self);
GAME_FN(void, __thiscall, MapArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);

inline void Unref(NiRefObject* object) {
    if (object != nullptr && sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// 0x82BEB0: TakeSample: the time now (the first only starts the clock); the rate over the last
// 20 samples (19 intervals, rounded); the frame time's sums, extremes and histogram (by 1 / time,
// the last bucket catching the rest)
void __fastcall FrameRate_TakeSample(NiFrameRate* self, re::Edx) {
    float now = NiGetCurrentTimeInSec();
    if (self->first) {
        self->startTime = now;
        self->first = 0;
        self->lastTime = now;
        return;
    }
    self->numSamples++;
    self->sampleTimes[self->sampleIndex] = now;
    self->sampleIndex = (self->sampleIndex + 1) % 20;
    if (self->numSamples >= 20) {
        float rate = 19.0f / (now - self->sampleTimes[self->sampleIndex]);
        self->frameRate = uint32_t(crt::FloatToInt(double(rate + 0.5f)));
    }
    float time = now - self->lastTime;
    self->sumOfTimes = self->sumOfTimes + time;
    self->sumOfSquares = time * time + self->sumOfSquares;
    self->lastTime = now;
    if (self->minTime > time) {
        self->minTime = time;
    }
    if (self->maxTime < time) {
        self->maxTime = time;
    }
    if (self->histogram != nullptr) {
        uint32_t bucket = uint32_t(crt::FloatToInt(double(1.0f / time)));
        if (bucket >= self->histogramSize) {
            bucket = self->histogramSize - 1;
        }
        self->histogram[bucket]++;
    }
}

// 0x82BFE0: NiTexturingProperty::SetBaseFilterMode(mode): the base map's filter (the map made
// when there's none)
void __fastcall TexturingProperty_SetBaseFilterMode(void* self, re::Edx, int mode) {
    void* map = *At<re::Ptr32<void>*>(self, 0x20);
    if (map == nullptr) {
        void* memory = NiNew(0x10);
        map = memory != nullptr ? TexturingMap_Construct(memory) : nullptr;
        re::Ptr32<void> value = map;
        MapArray_SetAt(static_cast<uint8_t*>(self) + 0x1C, 0, &value);
    }
    uint16_t& flags = At<uint16_t>(map, 4);
    flags = uint16_t((flags & 0xF0FF) | (uint16_t(uint8_t(mode)) << 8));
}

// 0x82C040: Update: when the rate changed, each digit quad's texture coordinates moved to its
// glyph (leading zeros blank: glyph 10), and the digits placed at their pixel position on the
// target (the renderer's default when none)
void __fastcall FrameRate_Update(NiFrameRate* self, re::Edx) {
    if (!self->enabled) {
        return;
    }
    uint32_t value = self->frameRate;
    if (self->lastFrameRate == value) {
        return;
    }
    self->lastFrameRate = value;
    uint32_t digits[3];
    for (int i = 3; i != 0;) {
        i--;
        digits[i] = value % 10;
        value /= 10;
    }
    void* data = At<void*>(self->element, 0xCC);   // (re-read for each call)
    bool seen = false;
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t glyph = digits[i];
        if (glyph == 0 && !seen) {
            glyph = 10;
        } else {
            seen = true;
        }
        float v = float(double(glyph)) * self->digitStep;
        float v2 = self->digitStep + v;
        int32_t polygon = self->digits[i];
        float uv[2] = {0.0f, v};
        data = At<void*>(self->element, 0xCC);
        ScreenElementsData_SetTexture(data, polygon, 0, 0, uv);
        float uv1[2] = {0.0f, v2};
        ScreenElementsData_SetTexture(At<void*>(self->element, 0xCC), self->digits[i], 1, 0, uv1);
        float uv2[2] = {1.0f, v2};
        ScreenElementsData_SetTexture(At<void*>(self->element, 0xCC), self->digits[i], 2, 0, uv2);
        float uv3[2] = {1.0f, v};
        ScreenElementsData_SetTexture(At<void*>(self->element, 0xCC), self->digits[i], 3, 0, uv3);
    }
    void* target = self->target;
    if (target == nullptr) {
        target = Virtual<void*>(NiRenderer_ms_pkRenderer, 29);   // GetDefaultRenderTargetGroup
    }
    float invWidth = 1.0f / float(double(Virtual<uint32_t>(target, 18, 0u)));    // GetWidth(0)
    float invHeight = 1.0f / float(double(Virtual<uint32_t>(target, 19, 0u)));   // GetHeight(0)
    NiPoint3& translate = At<NiPoint3>(self->element, 0x50);   // the local translation
    translate.x = self->x * invWidth;
    translate.y = self->y * invHeight;
    translate.z = 0.0f;
    AVObject_Update(self->element, 0.0f, 1);
}

// 0x82C240: Shutdown (a guess): the digits let go (when made), the histogram freed
void __fastcall FrameRate_Shutdown(NiFrameRate* self, re::Edx) {
    if (self->enabled && self->element != nullptr) {
        Unref(self->element);
        self->element = nullptr;
    }
    NiFree(self->histogram);
}

// 0x82C280: ~NiFrameRate (the same, then the texture and what's left of the element let go)
void __fastcall FrameRate_Destruct(NiFrameRate* self, re::Edx) {
    if (self->enabled && self->element != nullptr) {
        Unref(self->element);
        self->element = nullptr;
    }
    NiFree(self->histogram);
    Unref(self->texture);
    Unref(self->element);
}

}   // namespace
}   // namespace ni::app

// ---- registration ---------------------------------------------------------------------

#define C33_FRAMERATE_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_FRAMERATE_REVERSED("NiFrameRate::TakeSample", 0x0082BEB0, &ni::app::FrameRate_TakeSample);
C33_FRAMERATE_REVERSED("NiTexturingProperty::SetBaseFilterMode", 0x0082BFE0,
                       &ni::app::TexturingProperty_SetBaseFilterMode);
C33_FRAMERATE_REVERSED("NiFrameRate::Update", 0x0082C040, &ni::app::FrameRate_Update);
C33_FRAMERATE_REVERSED("NiFrameRate::Shutdown", 0x0082C240, &ni::app::FrameRate_Shutdown);
C33_FRAMERATE_REVERSED("NiFrameRate::~NiFrameRate", 0x0082C280, &ni::app::FrameRate_Destruct);
