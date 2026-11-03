// CTimeCycle's update: the current values (0x20C47A0, floats and bytes) mixed from the
// timecyc.dat boxes (0xBA6C70: 24 hours x 21 weathers of 0x7C bytes; the chapter's
// season 2 moves the weathers up by 6, season 3 by 12; interiors use weathers 18..20 with
// the area as the "hour"), smoothed towards the last frame's (0xBB60B0), and a counter
// easing the fog in while the player is under cover (no rain) or looks straight down.
// 0x6AEBB0..0x6B1030.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/TrackMath.h"
#include "reversed.h"

namespace {

// a box's layout (0x7C bytes) as the mixers read it
//  +0x00..0x0E bytes (0x0C..0x0E scaled by the float at +0x10 once mixed)
//  +0x10..0x28 floats
//  +0x2C..0x31, 0x35..0x3A bytes (colours), +0x3B..0x3D signed bytes, +0x3E byte
//  +0x40, 0x44 floats, +0x48, 0x4A shorts, +0x4C, 0x4D bytes, +0x50 float,
//  +0x54..0x5E bytes, +0x60..0x74 floats, +0x78 short
// The current values (0xB4 bytes) keep them in the same order, as floats from the bytes
// at +0x00..0x0E, +0x3B..0x3D.

using re::At;
template <typename T>
inline T Get(const void* p, int offset) {
    T v;
    std::memcpy(&v, static_cast<const uint8_t*>(p) + offset, sizeof(T));
    return v;
}

GAME_ARRAY(uint8_t, tc_boxes, 0x00BA6C70);
GAME_ARRAY(uint8_t, tc_current, 0x020C47A0);    // CTimeCycle's current values
GAME_ARRAY(uint8_t, tc_last, 0x00BB60B0);       // last frame's (for the smoothing)
GAME_VAR(float, tc_brightness, 0x020C4798);
GAME_VAR(int32_t, tc_coverCounter, 0x020C479C);  // 0..64 (eases the fog in under cover)
GAME_VAR(int16_t, tc_fogStart, 0x020C4820);
GAME_VAR(int32_t, tc_counterState, 0x020C4854);
GAME_ARRAY(float, tc_smoothState, 0x020C4858);  // the step states of +0x3C, +0x40.. (below)
GAME_ARRAY(float, tc_sunDirection, 0x020C4884); // x, y, z, w
GAME_VAR(int32_t, tc_lastArea, 0x00BB6100);
GAME_VAR(uint8_t, tc_bigJump, 0x00BCEB3C);      // CClock::ms_bBigJump
GAME_VAR(uint8_t, tc_clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, tc_clockMinutes, 0x00BCEB4A);
GAME_VAR(uint16_t, tc_clockSeconds, 0x00BCEB48);
GAME_VAR(float, tc_weatherInterpolation, 0x00CE7568);
GAME_VAR(int16_t, tc_newWeather, 0x00CE7570);
GAME_VAR(int16_t, tc_oldWeather, 0x00CE7574);
GAME_VAR(uint8_t, tc_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, tc_chapterSeason, 0x00A147EC);
GAME_VAR(int32_t, tc_currArea, 0x00BD1008);
GAME_VAR(float, tc_lightsOnFactor, 0x00C66B20);
GAME_VAR(void*, tc_niCamera, 0x00C3CCF4);
GAME_VAR(uint8_t, tc_cutsceneRunning, 0x020C5BE4);

GAME_FN(void, __cdecl, StepIntTowards, 0x00432A90, int32_t* out, int32_t* state, int32_t target);
GAME_FN(void, __cdecl, StepFloatTowards, 0x00432AD0, float* out, float* state, float target, float maxStep);
GAME_FN(uint8_t, __cdecl, IsAreaExterior, 0x00430320, int32_t area);  // (the table at 0x901DD0: 1 for areas 0, 1, 22, 31, 42, 43..)
GAME_FN(float, __thiscall, Vec_Normalise, 0x004294A0, float* v);
// (reads the first two; the game passes four floats, the last uninitialised)
GAME_FN(void, __cdecl, CalcPedShadowValues, 0x00526770, float x, float y, float z, float w);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(uint8_t, __cdecl, PlayerNoRain, 0x00538C80);
GAME_FN(uint8_t, __cdecl, CamNoRain, 0x00538C70);
// this file's
GAME_FN(void, __cdecl, TC_Mix, 0x006AEF80, void* out, const uint8_t* a, const uint8_t* b, const uint8_t* c,
        const uint8_t* d, float t, float weather, int interior);
GAME_FN(void, __cdecl, TC_Copy, 0x006AEBB0, void* out, const uint8_t* box);
GAME_FN(void, __cdecl, TC_StepVector, 0x006AEE50, float* out, float* state, const float* target, float maxStep);
GAME_FN(void, __cdecl, TC_StepColour, 0x006AEEC0, uint8_t* out, uint8_t* state, const uint8_t* target);

const uint8_t* Box(int hour, int weather) {
    return tc_boxes + (hour * 21 + weather) * 0x7C;
}

// 0x6AEBB0: the current values straight from one box (the colours only when set)
void __cdecl CopyBox(void* out, const uint8_t* box) {
    if (box[0x2C] || box[0x2D] || box[0x2E]) {
        for (int i = 0; i < 6; i++) {
            At<uint8_t>(out, 0x58 + i) = box[0x2C + i];
        }
        for (int i = 0; i < 6; i++) {
            At<uint8_t>(out, 0x61 + i) = box[0x35 + i];
        }
        At<float>(out, 0x68) = float(int8_t(box[0x3B]));
        At<float>(out, 0x88) = Get<float>(box, 0x50);
        for (int i = 0; i < 6; i++) {
            At<uint8_t>(out, 0x8C + i) = box[0x54 + i];
        }
    }
    for (int i = 0; i < 15; i++) {
        At<float>(out, i * 4) = float(box[i]);
    }
    for (int i = 0; i < 7; i++) {
        At<float>(out, 0x3C + i * 4) = Get<float>(box, 0x10 + i * 4);
    }
    At<float>(out, 0x6C) = float(int8_t(box[0x3C]));
    At<float>(out, 0x70) = float(int8_t(box[0x3D]));
    At<uint8_t>(out, 0x74) = box[0x3E];
    At<float>(out, 0x78) = Get<float>(box, 0x40);
    At<float>(out, 0x7C) = Get<float>(box, 0x44);
    At<int16_t>(out, 0x80) = Get<int16_t>(box, 0x48);
    At<int16_t>(out, 0xB0) = Get<int16_t>(box, 0x78);
    At<int16_t>(out, 0x82) = Get<int16_t>(box, 0x4A);
    At<uint8_t>(out, 0x84) = box[0x4C];
    At<uint8_t>(out, 0x85) = box[0x4D];
    for (int i = 0; i < 5; i++) {
        At<uint8_t>(out, 0x92 + i) = box[0x5A + i];
    }
    for (int i = 0; i < 6; i++) {
        At<float>(out, 0x98 + i * 4) = Get<float>(box, 0x60 + i * 4);
    }
}

// 0x6AEE50: three floats stepped towards a target by at most maxStep
void __cdecl StepVector(float* out, float* state, const float* target, float maxStep) {
    StepFloatTowards(&out[0], &state[0], target[0], maxStep);
    StepFloatTowards(&out[1], &state[1], target[1], maxStep);
    StepFloatTowards(&out[2], &state[2], target[2], maxStep);
}

// 0x6AEEC0: an RGB colour stepped towards a target (a step of one per channel)
void __cdecl StepColour(uint8_t* out, uint8_t* state, const uint8_t* target) {
    for (int i = 0; i < 3; i++) {
        int32_t o = out[i];
        int32_t s = state[i];
        StepIntTowards(&o, &s, target[i]);
        out[i] = uint8_t(o);
        state[i] = uint8_t(s);
    }
}

// 0x6AEF70
void __cdecl ResetCoverCounter() {
    tc_coverCounter = 0;
}

// the four boxes weighted (the game's order of the sums: d, a, b, c)
struct Weights {
    const uint8_t *a, *b, *c, *d;  // (hour, weather 1), (hour, 2), (hour + 1, 1), (hour + 1, 2)
    float wa, wb, wc, wd;

    template <typename T>
    float Sum(int offset) const {
        return ((float(Get<T>(c, offset)) * wc + float(Get<T>(a, offset)) * wa) + float(Get<T>(b, offset)) * wb) +
               float(Get<T>(d, offset)) * wd;
    }
    // rounded to the nearest (+ 0.5, floored)
    float Rounded(int offset) const {
        float r = Sum<uint8_t>(offset) + 0.5f;
        return float(std::floor(double(r)));
    }
    uint8_t RoundedByte(int offset) const {
        return uint8_t(int(Rounded(offset)));
    }
    uint8_t TruncatedByte(int offset) const {
        return uint8_t(int(Sum<uint8_t>(offset)));
    }
};

// 0x6AEF80: the current values mixed from four boxes: the hour's and the next's, each
// in the old and the new weather (hourT: into the hour, weatherT: into the weather).
// Interiors only take the colours
void __cdecl MixBoxes(void* out, const uint8_t* a, const uint8_t* b, const uint8_t* c, const uint8_t* d, float hourT,
                      float weatherT, int interior) {
    Weights w;
    w.a = a;
    w.b = b;
    w.c = c;
    w.d = d;
    w.wa = (1.0f - weatherT) * (1.0f - hourT);
    w.wb = (1.0f - hourT) * weatherT;
    w.wc = (1.0f - weatherT) * hourT;
    w.wd = hourT * weatherT;
    for (int i = 0; i < 6; i++) {
        At<uint8_t>(out, 0x58 + i) = w.RoundedByte(0x2C + i);
    }
    for (int i = 0; i < 6; i++) {
        At<uint8_t>(out, 0x61 + i) = w.RoundedByte(0x35 + i);
    }
    At<float>(out, 0x68) = w.Sum<int8_t>(0x3B);
    At<float>(out, 0x88) = w.Sum<float>(0x50);
    for (int i = 0; i < 6; i++) {
        At<uint8_t>(out, 0x8C + i) = w.RoundedByte(0x54 + i);
    }
    if (re::Flag(interior)) {
        return;
    }
    for (int i = 0; i < 12; i++) {
        At<float>(out, i * 4) = w.Rounded(i);
    }
    for (int i = 0; i < 7; i++) {
        At<float>(out, 0x3C + i * 4) = w.Sum<float>(0x10 + i * 4);
    }
    for (int i = 12; i < 15; i++) {
        At<float>(out, i * 4) = w.Rounded(i);
    }
    // (+0x0C..0x14 scaled by +0x3C)
    for (int i = 12; i < 15; i++) {
        At<float>(out, i * 4) = At<float>(out, i * 4) * At<float>(out, 0x3C);
    }
    At<float>(out, 0x6C) = w.Sum<int8_t>(0x3C);
    At<float>(out, 0x70) = w.Sum<int8_t>(0x3D);
    At<uint8_t>(out, 0x74) = w.TruncatedByte(0x3E);
    At<float>(out, 0x78) = w.Sum<float>(0x40);
    At<float>(out, 0x7C) = w.Sum<float>(0x44);
    At<int16_t>(out, 0x80) = int16_t(int(w.Sum<int16_t>(0x48)));
    At<int16_t>(out, 0xB0) = int16_t(int(w.Sum<int16_t>(0x78)));
    At<int16_t>(out, 0x82) = int16_t(int(w.Sum<int16_t>(0x4A)));
    At<uint8_t>(out, 0x84) = w.TruncatedByte(0x4C);
    At<uint8_t>(out, 0x85) = w.TruncatedByte(0x4D);
    At<uint8_t>(out, 0x92) = d[0x5A];
    for (int i = 0; i < 4; i++) {
        At<uint8_t>(out, 0x93 + i) = w.RoundedByte(0x5B + i);
    }
    for (int i = 0; i < 6; i++) {
        At<float>(out, 0x98 + i * 4) = w.Sum<float>(0x60 + i * 4);
    }
}

// 0x6B0780: CTimeCycle::Update
void __cdecl Update() {
    uint8_t interior = IsAreaExterior(tc_currArea) == 0 ? 1 : 0;
    int hour = tc_clockHours;
    int nextHour = uint8_t((hour + 1) % 24);
    int seasonOffset = 0;
    switch (tc_chapterSeason[tc_chapter]) {
    case 2:
        seasonOffset = 6;
        break;
    case 3:
        seasonOffset = 12;
        break;
    }
    int oldWeather = int16_t(tc_oldWeather + seasonOffset);
    int newWeather = int16_t(tc_newWeather + seasonOffset);
    // (the seconds' low byte only)
    float hourT = (float(uint8_t(tc_clockSeconds)) / 60.0f + float(tc_clockMinutes)) / 60.0f;
    TC_Mix(tc_current, Box(hour, oldWeather), Box(hour, newWeather), Box(nextHour, oldWeather),
           Box(nextHour, newWeather), hourT, tc_weatherInterpolation, interior);
    if (tc_coverCounter != 0) {
        float fog = float(tc_fogStart);
        float target = float(double(float(tc_coverCounter)) * 10.15625);
        tc_fogStart = int16_t(int(!(target < fog) ? target : fog));
    }
    float scale = 1.0f;
    float ambientBlend = At<float>(tc_current, 0x40);
    tc_brightness = ambientBlend + (1.0f - ambientBlend) * (1.0f - tc_lightsOnFactor);
    float oldW = tc_sunDirection[3];  // (the game writes an uninitialised local here)
    if (interior) {
        tc_sunDirection[0] = 1.0f;
        tc_sunDirection[2] = 1.0f;
        tc_sunDirection[1] = 1.0f;
        tc_sunDirection[3] = oldW;
        Vec_Normalise(tc_sunDirection);
        int area = tc_currArea;
        int16_t weather = area < 50 ? int16_t((area >= 26 ? 1 : 0) + 18) : int16_t(20);
        TC_Copy(tc_current, tc_boxes + (uint8_t((area - 2) % 24) * 21 + weather) * 0x7C);
        StepFloatTowards(&At<float>(tc_current, 0x40), &tc_smoothState[1], At<float>(tc_current, 0x40), 0.01f);
        scale = tc_brightness;
    } else {
        float minutes = float(int(tc_clockHours) * 60 + int(tc_clockMinutes)) + float(tc_clockSeconds) / 60.0f;
        float degrees = minutes / 1440.0f * 360.0f;
        float angle = float(double(degrees / 180.0f) * 3.1415927410125732);
        // (the CRT's x87 sin 0x85AEF0 and cos 0x85AD80, called the game's way)
        float s = trk::CrtSin(angle);
        float c = trk::CrtCos(angle);
        tc_sunDirection[0] = -0.8f;
        tc_sunDirection[3] = oldW;
        tc_sunDirection[1] = -s;
        tc_sunDirection[2] = -c;
        Vec_Normalise(tc_sunDirection);
    }
    uint8_t* colours = tc_current + 0x58;  // three RGB colours, the third their mix
    float s255 = scale * float(1.0 / 255.0);
    constexpr float k255 = float(1.0 / 255.0);
    At<float>(tc_current, 0x00) = At<float>(tc_current, 0x00) * s255;
    At<float>(tc_current, 0x04) = At<float>(tc_current, 0x04) * s255;
    At<float>(tc_current, 0x08) = At<float>(tc_current, 0x08) * s255;
    colours[6] = uint8_t(int(colours[0] + colours[3] * 2) / 3);
    At<float>(tc_current, 0x0C) = At<float>(tc_current, 0x0C) * k255;
    At<float>(tc_current, 0x10) = At<float>(tc_current, 0x10) * k255;
    At<float>(tc_current, 0x14) = k255 * At<float>(tc_current, 0x14);
    colours[7] = uint8_t(int(colours[1] + colours[4] * 2) / 3);
    for (int off = 0x18; off <= 0x38; off += 4) {
        At<float>(tc_current, off) = At<float>(tc_current, off) * s255;
    }
    constexpr float kTenth = 0.1f;
    At<float>(tc_current, 0x68) = At<float>(tc_current, 0x68) * kTenth;
    At<float>(tc_current, 0x6C) = At<float>(tc_current, 0x6C) * kTenth;
    At<float>(tc_current, 0x70) = kTenth * At<float>(tc_current, 0x70);
    uint8_t blue2 = colours[5];
    colours[8] = uint8_t(int(colours[2] + blue2 * 2) / 3);
    if (tc_lastArea == tc_currArea && tc_bigJump == 0) {
        TC_StepVector(&At<float>(tc_current, 0x00), reinterpret_cast<float*>(tc_last + 0x00),
                      &At<float>(tc_current, 0x00), 0.01f);
        TC_StepVector(&At<float>(tc_current, 0x0C), reinterpret_cast<float*>(tc_last + 0x0C),
                      &At<float>(tc_current, 0x0C), 0.01f);
        TC_StepVector(&At<float>(tc_current, 0x18), reinterpret_cast<float*>(tc_last + 0x18),
                      &At<float>(tc_current, 0x18), 0.01f);
        TC_StepVector(&At<float>(tc_current, 0x24), reinterpret_cast<float*>(tc_last + 0x24),
                      &At<float>(tc_current, 0x24), 0.01f);
        TC_StepVector(&At<float>(tc_current, 0x30), reinterpret_cast<float*>(tc_last + 0x30),
                      &At<float>(tc_current, 0x30), 0.01f);
        StepFloatTowards(&At<float>(tc_current, 0x3C), &tc_smoothState[0], At<float>(tc_current, 0x3C), 0.01f);
        TC_StepColour(colours + 0, tc_last + 0x3C, colours + 0);
        TC_StepColour(colours + 3, tc_last + 0x40, colours + 3);
        TC_StepColour(colours + 6, tc_last + 0x44, colours + 6);
        for (int i = 0; i < 5; i++) {
            StepFloatTowards(&At<float>(tc_current, 0x44 + i * 4), &tc_smoothState[2 + i],
                             At<float>(tc_current, 0x44 + i * 4), 0.01f);
        }
    } else {
        tc_smoothState[0] = At<float>(tc_current, 0x3C);
        for (int i = 0; i < 5; i++) {
            tc_smoothState[2 + i] = At<float>(tc_current, 0x44 + i * 4);
        }
        std::memcpy(tc_last, tc_current, 0x3C);
        std::memcpy(tc_last + 0x3C, colours, 3);
        std::memcpy(tc_last + 0x40, colours + 3, 3);
        std::memcpy(tc_last + 0x44, colours + 6, 2);
        tc_last[0x46] = colours[8];
    }
    tc_lastArea = tc_currArea;
    tc_bigJump = 0;
    CalcPedShadowValues(tc_sunDirection[0], tc_sunDirection[1], tc_sunDirection[2], 0.0f);
    float* view = GetLocalTransform(GetCameraNode(tc_niCamera));
    float counter;
    if (view[8] < -0.9f || PlayerNoRain() || CamNoRain() || tc_cutsceneRunning) {
        // (looking straight down, under cover or in a cutscene: up to 64)
        counter = float(tc_coverCounter + 1);
        if (!(counter < 64.0f)) {
            counter = 64.0f;
        }
    } else {
        counter = float(tc_coverCounter - 1);
        if (!(counter > 0.0f)) {
            counter = 0.0f;
        }
    }
    tc_coverCounter = int(counter);
    StepIntTowards(&tc_coverCounter, &tc_counterState, tc_coverCounter);
}

} // namespace

#define TCUPDATE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TCUPDATE_REVERSED("CTimeCycle::CopyBox", 0x006AEBB0, &CopyBox);
TCUPDATE_REVERSED("CTimeCycle::StepVector", 0x006AEE50, &StepVector);
TCUPDATE_REVERSED("CTimeCycle::StepColour", 0x006AEEC0, &StepColour);
TCUPDATE_REVERSED("CTimeCycle::ResetCoverCounter", 0x006AEF70, &ResetCoverCounter);
TCUPDATE_REVERSED("CTimeCycle::MixBoxes", 0x006AEF80, &MixBoxes);
TCUPDATE_REVERSED("CTimeCycle::Update", 0x006B0780, &Update);
