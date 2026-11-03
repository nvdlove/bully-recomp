// HUDTimeCycle (HUD kind 6, 0x30 bytes, vtable 0x941EB0): the clock in the corner, a
// face with an hour and a minute hand (textured quads turned by the clock's time), lit
// in the evening. 0x6B1030..0x6B179A.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Hud.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

struct CVector2D {
    float x, y;
};

struct HUDTimeCycle : HUDComponent {
    col::CRect rect;  // +0x20 the face on the screen
};
static_assert(offsetof(HUDTimeCycle, rect) == 0x20);
static_assert(sizeof(HUDTimeCycle) == 0x30);

inline re::Ptr32<void>* const kVtable = reinterpret_cast<re::Ptr32<void>*>(0x00941EB0);
inline void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);

GAME_VAR(uint8_t, htc_clockPaused, 0x00BCEB42);    // CClock::ms_bPaused
GAME_VAR(uint8_t, htc_clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, htc_clockMinutes, 0x00BCEB4A);
GAME_VAR(HUDComponent*, htc_hudKeys, 0x00C67544);
GAME_VAR(HUDComponent*, htc_raceHud, 0x00C674C8);
GAME_VAR(float, htc_lightsOnFactor, 0x00C66B20);
GAME_VAR(float, htc_screenScaleX, 0x00A957A8);
GAME_VAR(float, htc_screenScaleY, 0x00A957AC);
GAME_VAR(void*, htc_texOverlay, 0x00C66F38);      // (drawn over the hands)
GAME_VAR(void*, htc_texFace, 0x00C66F3C);
GAME_VAR(void*, htc_texMinuteHand, 0x00C66F40);
GAME_VAR(void*, htc_texHourHand, 0x00C66F44);

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(HUDComponent*, __thiscall, HUDComponent_Construct, 0x0053CDC0, HUDComponent* self);
GAME_FN(void, __thiscall, HUDComponent_Destruct, 0x0053CDF0, HUDComponent* self);
GAME_FN(void, __thiscall, HUDComponent_SetAlignment, 0x0053CE00, HUDComponent* self, const char* h, const char* v);
GAME_FN(col::CRect*, __cdecl, ScreenRect, 0x0052D650, col::CRect* out, float x, float y, float width, float height,
        int hAlign, int vAlign);
GAME_FN(CRGBA*, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const CRGBA* colour, const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, DrawQuadUV, 0x0052CCC0, float x1, float y1, float x2, float y2, float x3, float y3, float x4,
        float y4, void* texture, const CRGBA* colour, const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(uint8_t, __thiscall, MissionMgr_Fn6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(int32_t*, __thiscall, MissionMgr_Current, 0x006AA790, void* mgr);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetMission, 0x006AA660, void* mgr, int32_t index);

// 0x6B1030: IsVisible (not while the clock's stopped, the keys or race HUD show, or a
// mission of kind 5 runs)
uint8_t __fastcall IsVisible(HUDTimeCycle* self, re::Edx) {
    if (self->suppressed) {
        return self->visible;
    }
    if (htc_clockPaused || vc::Call<uint8_t>(htc_hudKeys, 0x10) || vc::Call<uint8_t>(htc_raceHud, 0x10)) {
        return 0;
    }
    if (MissionMgr_Fn6AA7E0(kMissionMgr)) {
        int32_t mission = *MissionMgr_Current(kMissionMgr);
        if (mission >= 0 && *reinterpret_cast<int32_t*>(MissionMgr_GetMission(kMissionMgr, mission) + 0x28) == 5) {
            return 0;
        }
    }
    return 1;
}

// 0x6B10A0: Setup (the record: x, y, width, height, then the alignment names at +0xA, +0x12)
void __fastcall Setup(HUDTimeCycle* self, re::Edx, const HUDLayoutRecord* record) {
    const int16_t* data = re::PtrCast<const int16_t*>(record->data);
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(data) + 0xA,
                              reinterpret_cast<const char*>(data) + 0x12);
    col::CRect out;
    col::CRect* r = ScreenRect(&out, float(data[0]), float(data[1]), float(data[2]), float(data[3]), self->hAlign,
                               self->vAlign);
    self->rect = *r;
}

// 0x6B1140
HUDTimeCycle* __fastcall DeletingDestruct(HUDTimeCycle* self, re::Edx, int flags) {
    self->vtable = kVtable;
    HUDComponent_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// a hand's quad: 128 long, 8 wide, through the centre, turned by the angle's cos, sin
struct HandQuad {
    float x[4], y[4];
};

HandQuad MakeHand(float co, float si, float cx, float cy, float sx, float sy) {
    HandQuad q;
    q.x[0] = (co * -4.0f - si * -64.0f) * sx + cx;
    q.y[0] = (si * -4.0f + co * -64.0f) * sy + cy;
    q.x[1] = (co * 4.0f - si * -64.0f) * sx + cx;
    q.y[1] = (co * -64.0f + si * 4.0f) * sy + cy;
    q.x[2] = (co * -4.0f - si * 64.0f) * sx + cx;
    q.y[2] = (si * -4.0f + co * 64.0f) * sy + cy;
    q.x[3] = (co * 4.0f - si * 64.0f) * sx + cx;
    q.y[3] = (si * 4.0f + co * 64.0f) * sy + cy;
    return q;
}

// 0x6B1170: Draw
void __fastcall Draw(HUDTimeCycle* self, re::Edx) {
    constexpr float kPi = 3.14159274f;
    float minuteAngle = float(htc_clockMinutes * 6) * kPi / 180.0f;
    float halfMinutes = float(int(htc_clockMinutes) + (htc_clockHours % 12) * 60) * 0.5f;
    float hourAngle = kPi * halfMinutes / 180.0f;
    // (the CRT's x87 sin 0x85AEF0 and cos 0x85AD80, called the game's way)
    float minuteSin = trk::CrtSin(minuteAngle);
    float minuteCos = trk::CrtCos(minuteAngle);
    float hourSin = trk::CrtSin(hourAngle);
    float hourCos = trk::CrtCos(hourAngle);
    void* texOverlay = htc_texOverlay;
    void* texFace = htc_texFace;
    void* texMinute = htc_texMinuteHand;
    void* texHour = htc_texHourHand;
    SetRenderState(10, 5);
    SetRenderState(11, 6);
    CRGBA faceLit, face;
    RGBA_Construct(&faceLit, 0x50, 0x64, 0x64, 0xFF);
    RGBA_Construct(&face, 0x80, 0x80, 0x80, 0xFF);
    CRGBA colour = face;
    if (htc_lightsOnFactor > 0.99f) {
        colour = faceLit;
    }
    CVector2D uv1{0.0f, 0.0f};
    CVector2D uv2{1.0f, 1.0f};
    DrawRectUV(self->rect.left, self->rect.top, self->rect.right, self->rect.bottom, texFace, &colour, &uv1, &uv2);
    float cx = std::fabs(self->rect.right - self->rect.left) * 0.5f + self->rect.left;
    float cy = 0.5f * std::fabs(self->rect.bottom - self->rect.top) + self->rect.top;
    float sx = htc_screenScaleX;
    float sy = htc_screenScaleY;
    HandQuad minute = MakeHand(minuteCos, minuteSin, cx, cy, sx, sy);
    HandQuad hour = MakeHand(hourCos, hourSin, cx, cy, sx, sy);
    CRGBA hands;
    RGBA_Construct(&hands, 0x80, 0x80, 0x80, 200);
    CRGBA handColour = hands;
    uv1 = CVector2D{0.0f, 0.0f};
    uv2 = CVector2D{1.0f, 1.0f};
    DrawQuadUV(minute.x[0], minute.y[0], minute.x[1], minute.y[1], minute.x[2], minute.y[2], minute.x[3], minute.y[3],
               texMinute, &handColour, &uv1, &uv2);
    uv1 = CVector2D{0.0f, 0.0f};
    uv2 = CVector2D{1.0f, 1.0f};
    DrawQuadUV(hour.x[0], hour.y[0], hour.x[1], hour.y[1], hour.x[2], hour.y[2], hour.x[3], hour.y[3], texHour,
               &handColour, &uv1, &uv2);
    CRGBA overlayLit, overlay;
    RGBA_Construct(&overlayLit, 0x99, 0xF6, 0xED, 0x80);
    RGBA_Construct(&overlay, 0x10, 0x10, 0x10, 0xFF);
    colour = overlay;
    if (htc_lightsOnFactor > 0.99f) {
        colour = overlayLit;
    }
    uv1 = CVector2D{0.0f, 0.0f};
    uv2 = CVector2D{1.0f, 1.0f};
    DrawRectUV(self->rect.left, self->rect.top, self->rect.right, self->rect.bottom, texOverlay, &colour, &uv1, &uv2);
}

// 0x6B1770
HUDTimeCycle* __fastcall Construct(HUDTimeCycle* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = kVtable;
    self->rect.left = 1000000.0f;
    self->rect.bottom = 1000000.0f;
    self->rect.right = -1000000.0f;
    self->rect.top = -1000000.0f;
    return self;
}

} // namespace

#define HUDTC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

HUDTC_REVERSED("HUDTimeCycle::IsVisible", 0x006B1030, &IsVisible);
HUDTC_REVERSED("HUDTimeCycle::Setup", 0x006B10A0, &Setup);
HUDTC_REVERSED("HUDTimeCycle::DeletingDestructor", 0x006B1140, &DeletingDestruct);
HUDTC_REVERSED("HUDTimeCycle::Draw", 0x006B1170, &Draw);
HUDTC_REVERSED("HUDTimeCycle::HUDTimeCycle", 0x006B1770, &Construct);
