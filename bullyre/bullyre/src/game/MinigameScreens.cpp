// The minigames' screens: the popup (the exit question / info box, 0x706000), the class
// grades screen (0x706600) and the completion screen (0x707510); their state is set in
// MinigameBase.cpp. All in the 640x480 virtual screen; the font's globals (0x208B7xx) set
// directly the way the game does. The x87 runs at 24-bit precision here (the main
// thread): float throughout.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/SchoolClasses.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using school::At;
using school::GameString;

GAME_ARRAY(uint8_t, msFontColour, 0x0208B764);
GAME_ARRAY(uint8_t, msFontShadowColour, 0x0208B780);
GAME_VAR(float, msFontWrap, 0x0208B778);
GAME_VAR(int16_t, msFontShadow, 0x0208B784);
GAME_VAR(int16_t, msFontEdge, 0x0208B786);
GAME_VAR(uint8_t, msFontFlags, 0x0208B78A);
GAME_VAR(int32_t, msLanguage, 0x00C18724);
GAME_VAR(uint8_t, msWidescreen, 0x00C1870E);
GAME_VAR(float, msBA4E10, 0x00BA4E10);
GAME_VAR(float, msHiliteOffset, 0x020C8804);
GAME_VAR(float, msScreenScaleX, 0x00A957A8);
GAME_VAR(float, msScreenScaleY, 0x00A957AC);
GAME_VAR(float, msTimeStep, 0x00C1A9A4);
inline void* const msTheText = reinterpret_cast<void*>(0x020C2FA4);
// the grades screen
GAME_VAR(uint8_t, msShowingGrades, 0x020C8708);
GAME_VAR(float, msGradesTime, 0x020C870C);
GAME_VAR(float, msGradesRate, 0x00BC6ECC);
GAME_VAR(float, msGradesExitDrop, 0x00BC6ED0);
GAME_VAR(int32_t, msGradesClass, 0x020C86F8);
GAME_VAR(int32_t, msGradesGrade, 0x020C8654);
GAME_ARRAY(char, msGradesText, 0x020C8710);
GAME_ARRAY(int32_t, msGradeTable, 0x00C1B02C);
GAME_VAR(void*, msGradeIconA, 0x00C66F54);
GAME_VAR(void*, msGradeIconB, 0x00C66F58);

GAME_FN(void, __cdecl, MS_SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, MS_GetRenderState, 0x00519290, int state, int32_t* out);
GAME_FN(uint8_t*, __thiscall, MS_RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, MS_DrawRect, 0x0052C470, const float* rect, const uint8_t* colour);
GAME_FN(void, __cdecl, MS_DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, MS_DrawSprite, 0x0052D610, const float* rect, void* texture, const uint8_t* colour,
        const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, MS_Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, MS_Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, MS_Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, MS_Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, MS_Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(void, __cdecl, MS_Font_GetStringBox, 0x0068B090, float* box, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, MS_Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, MS_Font_Render, 0x0068CDF0);
GAME_FN(const uint16_t*, __thiscall, MS_Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(uint8_t, __cdecl, MS_Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(void, __cdecl, MS_Pause_45B810, 0x0045B810);
GAME_FN(int, __thiscall, MS_Popup_Result, 0x00705080, void* popup);

inline void SetFontColour(const uint8_t* c) {
    msFontColour[0] = c[0];
    msFontColour[1] = c[1];
    msFontColour[2] = c[2];
    msFontColour[3] = c[3];
}
inline const uint16_t* TextOf(const char* key) {
    return MS_Text_Get(msTheText, key);
}

// 0x706000: the popup: its texture (a plain box with a border without one), the title,
// the yes / no choice (the chosen one highlighted) and the footer
void __fastcall PopupDraw(uint8_t* popup, re::Edx) {
    if (popup[0x50] == 0) {
        return;
    }
    float savedBA4E10 = msBA4E10;
    if (msLanguage == 7) {
        msBA4E10 = 1.0f;
    }
    MS_Pause_45B810();
    if (popup[0x51] != 0) {
        MS_Popup_Result(popup);
    }
    uint8_t white[4];
    uint8_t shadow[4];
    uint8_t title[4];
    uint8_t footer[4];
    uint8_t dim[4];
    uint8_t lit[4];
    uint8_t border[4];
    MS_RGBA_Set(white, 0xFF, 0xFF, 0xFF, 0xFF);
    MS_RGBA_Set(shadow, 0x10, 0x10, 0x10, 0x80);
    MS_RGBA_Set(title, 0xEB, 0xEB, 0x80, 0xFF);
    MS_RGBA_Set(footer, 0x60, 0x60, 0x80, 0xFF);
    MS_RGBA_Set(dim, 0x64, 0x64, 0x64, 0xFF);
    MS_RGBA_Set(lit, 0xD7, 0xD7, 0xD7, 0xFF);
    MS_RGBA_Set(border, 0x80, 0x80, 0xEB, 0x80);
    float rect[4] = {0.0f, 0.0f, 640.0f, 480.0f};
    void* texture = msWidescreen != 0 ? At<void*>(popup, 0x5C) : At<void*>(popup, 0x58);
    float uv2[2] = {1.0f, 1.0f};
    float uv1[2] = {0.0f, 0.0f};
    MS_DrawSprite(rect, texture, white, uv1, uv2);
    if (texture == nullptr) {
        uint8_t back[4];
        MS_DrawRect(rect, MS_RGBA_Set(back, 0x10, 0x10, 0x10, 0xC0));
        rect[0] = 160.0f;
        rect[1] = 120.0f;
        rect[2] = 480.0f;
        rect[3] = 360.0f;
        MS_DrawRect(rect, shadow);
        float edge[4];
        edge[0] = rect[0] - 4.0f;
        edge[3] = rect[3];
        edge[2] = rect[0];
        edge[1] = rect[1];
        MS_DrawRect(edge, border);
        edge[0] = rect[2];
        edge[3] = rect[3];
        edge[2] = rect[2] + 4.0f;
        edge[1] = rect[1];
        MS_DrawRect(edge, border);
        edge[0] = rect[0] - 4.0f;
        edge[3] = rect[1] - 4.0f;
        edge[2] = 4.0f + rect[2];
        edge[1] = rect[1];
        MS_DrawRect(edge, border);
        edge[0] = rect[0] - 4.0f;
        edge[3] = rect[3];
        edge[2] = rect[2] + 4.0f;
        edge[1] = 4.0f + rect[3];
        MS_DrawRect(edge, border);
    }
    MS_Font_Reset();
    MS_Font_SetJustify(2);
    MS_Font_SetSizePreset(3);
    msFontWrap = 448.0f;
    SetFontColour(title);
    uint16_t text[0x200];
    MS_Font_Format(text, MS_Text_Get(msTheText, reinterpret_cast<const char*>(popup + 0x10)), 0x200);
    float box[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    MS_Font_GetStringBox(box, 320.0f, 170.0f, text);
    float height = std::fabs(box[3] - box[1]);
    float y = 180.0f - height * 0.5f;
    if (At<int32_t>(popup, 8) == -1) {
        y = y + 28.7999992f;
    }
    MS_Font_PrintString(320.0f, y, text, 1);
    MS_Font_Render();
    if (msLanguage == 7) {
        msHiliteOffset = -8.0f;
    }
    if (At<int32_t>(popup, 8) != -1) {
        if (At<int32_t>(popup, 0xC) == 2) {
            SetFontColour(lit);
            MS_Font_PrintString(msHiliteOffset + 320.0f, 240.0f, TextOf(GameString(0x009412FC)), 1);   // EXIT_HILITE
        } else {
            SetFontColour(dim);
        }
        MS_Font_PrintString(320.0f, 240.0f, TextOf(GameString(0x00906974)), 1);   // YES
        if (At<int32_t>(popup, 0xC) == 1) {
            SetFontColour(lit);
            MS_Font_PrintString(msHiliteOffset + 320.0f, 264.0f, TextOf(GameString(0x009412FC)), 1);
        } else {
            SetFontColour(dim);
        }
        MS_Font_PrintString(320.0f, 264.0f, TextOf(GameString(0x00906970)), 1);   // NO
    }
    SetFontColour(footer);
    MS_Font_SetSizePreset(1);
    msFontFlags |= 0x10;
    MS_Font_SetJustify(2);
    MS_Font_PrintString(320.0f, 400.0f, TextOf(reinterpret_cast<const char*>(popup + 0x30)), 1);
    msFontFlags &= 0xEF;
    if (msLanguage == 7) {
        msBA4E10 = savedBA4E10;
    }
}

// 0x706600 (cdecl): the class grades screen: the class's name (outlined in gold), five
// numbered boxes with the grade's ticks, pass or fail; button 7 closes it once its two
// seconds are up
void __cdecl GradesDraw() {
    if (msShowingGrades == 0) {
        return;
    }
    if (msGradesTime == 0.0f) {
        if (MS_Pad_JustDown(7, 0) != 0) {
            msShowingGrades = 0;
            return;
        }
    } else {
        float step = msGradesRate * msTimeStep;
        msGradesTime = msGradesTime - step;
        if (msGradesTime <= 0.0f) {
            msGradesTime = 0.0f;
        }
    }
    MS_SetRenderState(6, 0);
    MS_SetRenderState(8, 0);
    MS_SetRenderState(10, 5);
    MS_SetRenderState(11, 6);
    MS_SetRenderState(0xC, 1);
    MS_SetRenderState(9, 2);
    MS_SetRenderState(2, 3);
    MS_SetRenderState(7, 2);
    // (this flag shows "pass": the table's value above the grade)
    bool pass = msGradeTable[msGradesClass] > msGradesGrade;
    uint8_t backdrop[4];
    uint8_t exitColour[4];
    uint8_t titleColour[4];
    uint8_t labelAbove[4];
    uint8_t nameGold[4];
    uint8_t outline10[4];
    uint8_t outline40[4];
    uint8_t outline80[4];
    uint8_t outlineC0[4];
    uint8_t outlineEB[4];
    uint8_t labelAtOrBelow[4];
    uint8_t passGreen[4];
    uint8_t failRed[4];
    uint8_t passShadow[4];
    uint8_t iconA[4];
    uint8_t iconB[4];
    MS_RGBA_Set(backdrop, 0x10, 0x10, 0x10, 0x64);
    MS_RGBA_Set(exitColour, 0x80, 0x80, 0x80, 0xFF);
    MS_RGBA_Set(titleColour, 0x80, 0x80, 0xC8, 0xFF);
    MS_RGBA_Set(labelAtOrBelow, 0xB4, 0xB4, 0xB4, 0xFF);
    MS_RGBA_Set(nameGold, 0xA6, 0x93, 0x3F, 0xD2);
    MS_RGBA_Set(outline10, 0x10, 0x10, 0x10, 0xC0);
    MS_RGBA_Set(outline40, 0x40, 0x40, 0x40, 0xC0);
    MS_RGBA_Set(outline80, 0x80, 0x80, 0x80, 0xC0);
    MS_RGBA_Set(outlineC0, 0xC0, 0xC0, 0xC0, 0x80);
    MS_RGBA_Set(outlineEB, 0xEB, 0xEB, 0xEB, 0x80);
    MS_RGBA_Set(labelAbove, 0xB4, 0xB4, 0xB4, 0xFF);
    MS_RGBA_Set(passShadow, 0x10, 0x10, 0x10, 0xFF);
    MS_RGBA_Set(passGreen, 0x22, 0x5D, 0x42, 0xFF);
    MS_RGBA_Set(failRed, 0x73, 0x20, 0x20, 0xFF);
    MS_RGBA_Set(iconA, 0xEB, 0xEB, 0xEB, 0xFF);
    MS_RGBA_Set(iconB, 0xEB, 0xEB, 0xEB, 0xFF);
    int32_t saved;
    MS_GetRenderState(9, &saved);
    MS_SetRenderState(9, 2);
    MS_Font_Reset();
    msFontWrap = 310.0f;
    msFontEdge = 3;
    msFontShadow = 2;
    MS_Font_SetSizePreset(3);
    float exitY = 365.0f;
    float box[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    uint16_t text[0x100];
    if (msGradesText[0] != 0) {
        MS_Font_Format(text, TextOf(msGradesText), 0x100);
        MS_Font_GetStringBox(box, 320.0f, 0.0f, text);
        exitY = float(double(box[3]) + 0.0) + 365.0f;
    } else {
        text[0] = 0;
    }
    float screen[4] = {0.0f, 480.0f, 640.0f, 0.0f};
    MS_DrawRect(screen, backdrop);
    msFontFlags &= 0xEF;
    msFontShadow = 2;
    MS_Font_SetSizePreset(3);
    if (text[0] != 0) {
        msFontFlags |= 0x10;
        SetFontColour(titleColour);
        MS_Font_PrintString(320.0f, 335.0f, text, 1);
        msFontFlags &= 0xEF;
    }
    SetFontColour(exitColour);
    if (msGradesTime == 0.0f) {
        MS_Font_PrintString(320.0f, exitY - msGradesExitDrop, TextOf(GameString(0x00948FD8)), 1);   // CLASS_EXIT
    }
    static const uintptr_t kClassNames[10] = {
            0x0090A25C, 0x0090A254, 0x0090A24C, 0x0090A244, 0x0090A23C,   // CN_ART, CHEM, ENG, MAT, BIO
            0x0090A234, 0x0090A22C, 0x0090A220, 0x0090A218, 0x0090A210,   // CN_GEO, GYM, PHOTO, SHOP, MUS
    };
    char names[10][0x10];
    for (int i = 0; i < 10; i++) {
        std::strncpy(names[i], GameString(kClassNames[i]), 0x10);
    }
    char gradeF[0x10];
    std::strncpy(gradeF, GameString(0x00948FD0), 0x10);   // GRADE_F (the others unused)
    msFontEdge = 3;
    MS_Font_SetScale(1.0f);
    const uint16_t* name = TextOf(names[msGradesClass]);
    // the class's name, outlined
    SetFontColour(outlineEB);
    MS_Font_PrintString(318.0f, 233.0f, name, 1);
    SetFontColour(outlineC0);
    MS_Font_PrintString(318.0f, 235.0f, name, 1);
    MS_Font_PrintString(320.0f, 233.0f, name, 1);
    SetFontColour(outline80);
    MS_Font_PrintString(322.0f, 233.0f, name, 1);
    MS_Font_PrintString(318.0f, 237.0f, name, 1);
    SetFontColour(outline40);
    MS_Font_PrintString(322.0f, 235.0f, name, 1);
    MS_Font_PrintString(320.0f, 237.0f, name, 1);
    SetFontColour(outline10);
    MS_Font_PrintString(322.0f, 237.0f, name, 1);
    SetFontColour(nameGold);
    MS_Font_PrintString(320.0f, 235.0f, name, 1);
    MS_Font_SetSizePreset(3);
    // the grade boxes
    uint8_t line[4];
    MS_RGBA_Set(line, 0x40, 0x40, 0x40, 0xC8);
    float x = 207.5f;
    float frame[4] = {206.5f, 349.0f, 433.5f, 279.0f};
    MS_DrawRect(frame, backdrop);
    for (int i = 0; i < 6; i++) {
        frame[0] = x - 1.0f;
        frame[3] = 279.0f;
        frame[2] = x + 1.0f;
        frame[1] = 349.0f;
        MS_DrawRect(frame, line);
        x = x + 45.0f;
    }
    frame[0] = 206.5f;
    frame[3] = 277.0f;
    frame[2] = 433.5f;
    frame[1] = 279.0f;
    MS_DrawRect(frame, line);
    frame[0] = 206.5f;
    frame[3] = 349.0f;
    frame[2] = 433.5f;
    frame[1] = 351.0f;
    MS_DrawRect(frame, line);
    frame[0] = 208.5f;
    frame[3] = 313.0f;
    frame[2] = 432.5f;
    frame[1] = 315.0f;
    MS_DrawRect(frame, line);
    float sx = msScreenScaleX;
    float sy = msScreenScaleY;
    x = 230.0f;
    for (int i = 0; i < 5; i++) {
        uint16_t label[2] = {uint16_t(i + 0x31), 0};
        SetFontColour(i > msGradesGrade ? labelAbove : labelAtOrBelow);
        MS_Font_PrintString(x, 280.0f, label, 1);
        msFontFlags |= 0x10;
        if (i == msGradesGrade || i < msGradesGrade) {
            float uv2[2] = {1.0f, 1.0f};
            float uv1[2] = {0.0f, 0.0f};
            void* icon;
            const uint8_t* colour;
            if (i == msGradesGrade) {
                icon = pass ? msGradeIconA : msGradeIconB;
                colour = pass ? iconA : iconB;
            } else {
                icon = msGradeIconA;
                colour = iconA;
            }
            float half = sx * 12.0f;
            float bottom = (sy * 24.0f + 320.0f) + 3.0f;
            MS_DrawRectUV(x - half, 323.0f, x + half, bottom, icon, colour, uv1, uv2);
        } else {
            SetFontColour(labelAbove);
            MS_Font_PrintString(x, 320.0f, TextOf(gradeF), 1);
        }
        msFontFlags &= 0xEF;
        x = x + 45.0f;
    }
    if (msGradesTime == 0.0f) {
        msFontWrap = 310.0f;
        msFontEdge = 3;
        MS_Font_SetSizePreset(4);
        msFontFlags |= 0x10;
        SetFontColour(passShadow);
        msFontShadow = 0;
        const char* key = pass ? GameString(0x00948F5C) : GameString(0x00948F50);   // CLASS_PASS / CLASS_FAIL
        static const float kOutline[8][2] = {{318.0f, 295.0f}, {322.0f, 295.0f}, {320.0f, 293.0f}, {320.0f, 297.0f},
                                             {318.0f, 293.0f}, {322.0f, 297.0f}, {322.0f, 293.0f}, {318.0f, 297.0f}};
        for (int k = 0; k < 8; k++) {
            MS_Font_PrintString(kOutline[k][0], kOutline[k][1], TextOf(key), 1);
        }
        SetFontColour(pass ? passGreen : failRed);
        MS_Font_PrintString(320.0f, 295.0f, TextOf(key), 1);
        msFontFlags &= 0xEF;
    }
    MS_SetRenderState(9, saved);
    MS_Font_Render();
}

// ---- the completion screen (0x707510) ----------------------------------------------------------
GAME_VAR(float, mscTime, 0x020C87A0);           // its time left (2 s; 0 when off)
GAME_VAR(uint8_t, mscFading, 0x020C8709);       // a fading (mission passed) one
GAME_VAR(uint8_t, mscColoured, 0x020C870A);     // given its colour
GAME_VAR(uint8_t, mscHold, 0x020C870B);
GAME_VAR(uint8_t, mscUber, 0x020C87A8);
GAME_VAR(uint8_t, mscUberFaded, 0x020C87A9);
GAME_VAR(uint8_t, mscErrandDone, 0x020C87AA);
GAME_ARRAY(char, mscTitle, 0x020C8738);
GAME_ARRAY(char, mscSubtitle, 0x020C8758);
GAME_ARRAY(char, mscExtra, 0x020C8778);
GAME_VAR(int32_t, mscExtraNumber, 0x020C87A4);
GAME_ARRAY(uint8_t, mscColour, 0x020C87F0);
GAME_ARRAY(uint8_t, mscMsgColours, 0x020C87F4);   // [n][4] (the money's is [0])
GAME_VAR(float, mscMoney, 0x020C8734);
GAME_VAR(int32_t, mscMsgCount, 0x020C8798);
GAME_VAR(float, mscMsgTime, 0x020C879C);
GAME_ARRAY(char, mscMsgs, 0x020C8658);            // [5][0x20]
GAME_ARRAY(int32_t, mscMsgColourIds, 0x020C8640);
GAME_VAR(void*, mscCrest, 0x020C8730);
GAME_VAR(int32_t, mscCrestTxd, 0x00BC6C68);
GAME_VAR(float, mscMoneyDrop, 0x00BC6ED4);       // the money's offset when fading (180 in language 7)
GAME_VAR(int32_t, mscTimeMs, 0x00C1A9B4);
GAME_VAR(uint8_t*, mscPlayerPed, 0x00C1AEA8);
GAME_VAR(int32_t, mscWidescreen, 0x00C3CD0C);
GAME_VAR(float, mscC3CD14, 0x00C3CD14);
GAME_VAR(float, mscCameraFade, 0x00C3CD28);
GAME_ARRAY(uint8_t, mscCameraFadeColour, 0x00C3CD2C);
GAME_VAR(uint8_t, mscCF3B12, 0x00CF3B12);
GAME_VAR(int32_t, mscCF3AE8, 0x00CF3AE8);
GAME_VAR(int16_t, mscFontStyle, 0x0208B788);
GAME_VAR(float, mscFontScaleY, 0x0208B770);
GAME_ARRAY(float, mscFontLineHeight, 0x00BA4E8C);   // per style
inline void* const mscMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const mscStream = reinterpret_cast<void*>(0x00CF42D4);
inline void* const mscCamera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(void, __cdecl, MSC_NiRelease, 0x005F0F90, void* object);
GAME_FN(void, __cdecl, MSC_RemoveModel, 0x0052F640, int id);
GAME_FN(uint8_t, __thiscall, MSC_MissionMgr_6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(int, __thiscall, MSC_Stream_GetState, 0x0058EDC0, void* stream);
GAME_FN(uint32_t, __thiscall, MSC_Stream_Get14, 0x0058EDE0, void* stream);
GAME_FN(void, __thiscall, MSC_Camera_Fade, 0x004F1AD0, void* camera, float seconds, int direction, int);
GAME_FN(float, __cdecl, MSC_MaxF, 0x00414490, float a, float b);
GAME_FN(void, __cdecl, MSC_Font_SetScaleXY, 0x006880B0, float x, float y);
GAME_FN(void, __cdecl, MSC_Font_AddNumber, 0x006880D0, float number);
GAME_FN(float, __cdecl, MSC_Font_GetLineHeight, 0x00688220);
GAME_FN(float*, __cdecl, MSC_ScreenRect, 0x0052D650, float* out, float x, float y, float width, float height,
        int hAlign, int vAlign);

inline uint8_t AlphaByte(float a) {
    return uint8_t(int(a));   // (fistp, truncating)
}
inline void SetFontColourA(const uint8_t* c, uint8_t a) {
    msFontColour[0] = c[0];
    msFontColour[1] = c[1];
    msFontColour[2] = c[2];
    msFontColour[3] = a;
}
inline void SetShadowColourA(const uint8_t* c, uint8_t a) {
    msFontShadowColour[0] = c[0];
    msFontShadowColour[1] = c[1];
    msFontShadowColour[2] = c[2];
    msFontShadowColour[3] = a;
}

// 0x707510 (cdecl): the completion screen ("mission passed", errands, class results):
// the title embossed (four shades round it), the subtitle (with the crest when one's
// loaded), the extra line, the queued messages (2 s each) and the money; it fades in its
// last `limit` seconds. Its time running out ends it (an uncoloured one fading the camera
// out); with no time left it only frees the crest.
void __cdecl CompletionDraw() {
    if (mscTime == 0.0f) {
        if (mscCrest != nullptr) {
            MSC_NiRelease(mscCrest);
            MSC_RemoveModel(mscCrestTxd + 0x2FA8);
            mscCrest = nullptr;
            mscCrestTxd = -1;
        }
        return;
    }
    if (MSC_MissionMgr_6AA7E0(mscMissionMgr) != 0 && mscErrandDone != 0) {
        mscTime = 0.0f;
        return;
    }
    const uint8_t fading = mscFading;
    const float limit = fading != 0 ? 0.5f : 2.0f;
    // the time runs unless a stream is in its first 10 s or messages are up, until it's in
    // its last `limit` seconds; held there while held or messages show
    bool runs;
    if (MSC_Stream_GetState(mscStream) == 7 && uint32_t(mscTimeMs - MSC_Stream_Get14(mscStream)) <= 10000u) {
        runs = limit < mscTime;
    } else if (0.0f < mscMsgTime) {
        runs = limit < mscTime;
    } else {
        runs = true;
    }
    if (runs) {
        const float step = msGradesRate * msTimeStep;
        mscTime = mscTime - step;
        const float t = mscTime;
        if ((mscHold != 0 || mscMsgTime != 0.0f) && t <= limit) {
            mscTime = limit;
        } else if (t <= 0.0f) {
            const bool playerFlag = mscPlayerPed != nullptr && mscPlayerPed[0x1C98] != 0;
            if (mscColoured == 0 && mscFading == 0) {
                mscC3CD14 = 0.0f;
                mscWidescreen = 0;
                if (mscUber == 0 && playerFlag) {
                    mscCameraFadeColour[0] = 0;
                    mscCameraFadeColour[1] = 0;
                    mscCameraFadeColour[2] = 0;
                    mscCameraFadeColour[3] = 0xFF;
                    MSC_Camera_Fade(mscCamera, 1.0f, 1, 0);
                }
            }
            mscTime = 0.0f;
            mscHold = 0;
            mscUber = 0;
            if (mscCF3B12 != 0) {
                const uint32_t when = uint32_t(mscTimeMs + 800);
                const float last = float(mscCF3AE8);
                mscCF3B12 = 0;
                mscCF3AE8 = int(MSC_MaxF(float(when), last));   // (__ftol2)
            }
        }
    }
    float alpha = 1.0f;
    {
        const float t = mscTime;
        if (t < limit) {
            const float x = 1.0f - t / limit;
            alpha = 1.0f - x * x;
        }
    }
    if (mscUber != 0) {
        const float fade = mscCameraFade;
        if (mscUberFaded != 0 && fade <= 254.0f) {
            alpha = fade / 255.0f * alpha;
            if (alpha >= 0.997f) {
                mscTime = 0.0f;
            }
        }
        if (fade == 255.0f) {
            mscUberFaded = 1;
        }
    }
    uint8_t grey[4], dark100[4], green100[4], purple100[4], faint[4], black[4], pale1[4], pale2[4], red[4];
    uint8_t edge0[4], edge1[4], edge2[4], edge3[4], edge4[4];
    MS_RGBA_Set(grey, 200, 200, 200, 0xFF);
    MS_RGBA_Set(dark100, 0x10, 0x10, 0x10, 100);     // (unused)
    MS_RGBA_Set(green100, 0x20, 0x40, 0x20, 100);    // (unused)
    MS_RGBA_Set(purple100, 0x40, 0x20, 0x40, 100);   // (unused)
    if (msLanguage == 7) {
        mscMoneyDrop = 180.0f;
    }
    MS_RGBA_Set(faint, 0xEB, 0xEB, 0xEB, 0x20);
    MS_RGBA_Set(black, 0x10, 0x10, 0x10, 0xFF);
    MS_RGBA_Set(pale1, 0xDC, 0xDC, 0xDC, 0xFF);
    MS_RGBA_Set(pale2, 0xDC, 0xDC, 0xDC, 0xFF);
    MS_RGBA_Set(red, 0xEB, 0x80, 0x80, 0xFF);
    MS_RGBA_Set(edge0, 0x10, 0x10, 0x10, 0xC0);
    MS_RGBA_Set(edge1, 0x40, 0x40, 0x40, 0xC0);
    MS_RGBA_Set(edge2, 0x80, 0x80, 0x80, 0xC0);
    MS_RGBA_Set(edge3, 0xC0, 0xC0, 0xC0, 0x80);
    MS_RGBA_Set(edge4, 0xEB, 0xEB, 0xEB, 0x80);
    const float sx = msScreenScaleX;
    const float offX = sx * 2.0f;
    const float offY = 2.0f * msScreenScaleY;
    MS_Font_Reset();
    msFontWrap = 600.0f;
    msFontEdge = 3;
    msFontShadow = 2;
    const uint8_t shadowA = AlphaByte(float(black[3]) * alpha);
    SetShadowColourA(black, shadowA);
    const uint8_t moneyBack[4] = {faint[0], faint[1], faint[2], AlphaByte(alpha * float(faint[3]))};
    MS_Font_SetSizePreset(3);
    // the box: 160..480 across; the title's top 120 (78 with a subtitle), the subtitle's
    // line 360 (318), the extra line's 332
    const float left = 160.0f;
    const float right = 480.0f;
    float top = 120.0f;
    float subtitleY = 360.0f;
    float extraY = 332.0f;
    if (mscSubtitle[0] != 0) {
        top = 78.0f;
        subtitleY = 318.0f;
    }
    float box[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    uint16_t line[256];
    if (mscExtra[0] != 0) {
        MSC_Font_AddNumber(float(mscExtraNumber));
        MS_Font_Format(line, TextOf(mscExtra), 0x100);
        MS_Font_GetStringBox(box, 320.0f, 0.0f, line);
        subtitleY = box[3] + 0.0f + subtitleY;
    } else {
        line[0] = 0;
    }
    if (fading != 0) {
        if (mscColoured != 0) {
            MS_Font_SetScale((1.0f - alpha) + 1.0f);
        } else {
            // it shrinks from 1.4 to 0.001 as it fades
            const float s = float(double(1.0f - alpha) * -1.3989999761106446) + 1.4f;
            const float scaleY = msScreenScaleY * s;
            MSC_Font_SetScaleXY(s * 1.1f * sx, scaleY);
        }
    } else if (mscColoured != 0) {
        MS_Font_SetScale(1.0f);
    } else {
        const float scaleY = msScreenScaleY * 1.4f;
        MSC_Font_SetScaleXY(float(double(sx) * 1.5400000071525568), scaleY);
    }
    msFontFlags &= 0xEF;
    msFontShadow = 0;
    if (fading != 0) {
        msFontWrap = 5000.0f;
    }
    // the title: printed 9 times, a shade per offset (all the darkest with a crest), then
    // in its colour
    uint16_t title[256];
    MS_Font_Format(title, TextOf(mscTitle), 0x100);
    const uint8_t a = AlphaByte(float(shadowA) * alpha);
    uint8_t shade0[4] = {edge0[0], edge0[1], edge0[2], a};
    uint8_t shade1[4] = {edge1[0], edge1[1], edge1[2], a};
    uint8_t shade2[4] = {edge2[0], edge2[1], edge2[2], a};
    uint8_t shade3[4] = {edge3[0], edge3[1], edge3[2], a};
    uint8_t shade4[4] = {edge4[0], edge4[1], edge4[2], a};
    if (mscCrest != nullptr) {
        uint8_t* shades[5] = {shade0, shade1, shade2, shade3, shade4};
        for (uint8_t* shade : shades) {
            shade[0] = edge0[0];
            shade[1] = edge0[1];
            shade[2] = edge0[2];
        }
    }
    float raise = fading != 0 && mscCrest != nullptr ? -175.0f : -155.0f;
    if (mscColoured != 0) {
        raise = -162.0f;
    }
    const float height = mscFontLineHeight[mscFontStyle] * mscFontScaleY;
    const float y = top - (height * 0.5f + raise);
    const float above = y - 2.0f;
    SetFontColourA(shade4, a);
    MS_Font_PrintString(318.0f, above, title, 1);
    SetFontColourA(shade3, a);
    MS_Font_PrintString(318.0f, y, title, 1);
    MS_Font_PrintString(320.0f, above, title, 1);
    SetFontColourA(shade2, a);
    MS_Font_PrintString(322.0f, above, title, 1);
    const float below = y + 2.0f;
    MS_Font_PrintString(318.0f, below, title, 1);
    SetFontColourA(shade1, a);
    MS_Font_PrintString(322.0f, y, title, 1);
    MS_Font_PrintString(320.0f, below, title, 1);
    SetFontColourA(shade0, a);
    MS_Font_PrintString(322.0f, below, title, 1);
    SetFontColourA(mscColour, AlphaByte(float(mscColour[3]) * alpha));
    MS_Font_PrintString(320.0f, y, title, 1);
    alpha = alpha * (alpha * alpha);
    if (mscSubtitle[0] != 0) {
        uint8_t white[4];
        MS_RGBA_Set(white, 0xFF, 0xFF, 0xFF, 0xFF);
        uint16_t subtitle[256];
        MS_Font_Format(subtitle, TextOf(mscSubtitle), 0x100);
        SetShadowColourA(black, AlphaByte(float(black[3]) * alpha));
        SetFontColourA(white, AlphaByte(alpha * float(white[3])));
        msFontShadow = 2;
        if (mscCrest != nullptr) {
            MS_Font_SetScale(0.8f);
        } else {
            MS_Font_SetSizePreset(3);
        }
        const float x = (right - left) * 0.5f + left + 0.0f;
        const float drop = mscCrest != nullptr ? 15.0f : -20.0f;
        const float sy = subtitleY - 28.0f + drop;
        if (mscCrest != nullptr) {
            msFontFlags |= 0x30;
        }
        MS_Font_PrintString(x, sy, subtitle, 1);
        if (mscCrest != nullptr) {
            const uint8_t crestColour[4] = {grey[0], grey[1], grey[2], AlphaByte(float(grey[3]) * alpha)};
            msFontFlags &= 0xCF;
            float rect[4];
            MSC_ScreenRect(rect, 0.0f, -75.0f, 128.0f, -128.0f, 1, 1);
            const float uv1[2] = {1.0f, 1.0f};
            const float uv0[2] = {0.0f, 0.0f};
            MS_DrawSprite(rect, mscCrest, crestColour, uv0, uv1);
        }
    }
    msFontFlags &= 0xEF;
    msFontShadow = 2;
    MS_Font_SetSizePreset(3);
    msFontWrap = 600.0f;
    if (line[0] != 0) {
        if (fading != 0) {
            msFontFlags &= 0xEF;
        } else {
            msFontFlags |= 0x10;
        }
        MS_Font_SetScale(0.66f);
        if (mscMoney == 0.0f) {
            extraY = 332.0f - MSC_Font_GetLineHeight();
        }
        SetShadowColourA(black, AlphaByte(float(black[3]) * alpha));
        const uint8_t* colour = fading != 0 ? pale1 : pale2;
        SetFontColourA(colour, AlphaByte(alpha * float(colour[3])));
        MS_Font_PrintString(320.0f, extraY, line, 1);
        msFontFlags &= 0xEF;
    }
    if (mscMsgTime != 0.0f) {
        // the newest message, fading in its first and out after 0.15 of its 2 s
        MS_Font_SetScale(0.66f);
        const float p = 1.0f - mscMsgTime * 0.5f;
        float show;
        if (p < 0.15f) {
            const float q = p / 0.15f;
            show = q * q;
        } else {
            const float q = float(double(p - 0.15f) / 0.8499999940395355);
            show = 1.0f - q * q;
        }
        if (line[0] == 0) {
            if (mscMoney == 0.0f) {
                extraY = extraY - MSC_Font_GetLineHeight();
            }
        } else {
            extraY = MSC_Font_GetLineHeight() + extraY;
        }
        MS_Font_Format(line, TextOf(mscMsgs + (mscMsgCount - 1) * 0x20), 0x100);
        const uint8_t* colour = mscMsgColours + mscMsgColourIds[mscMsgCount - 1] * 4;
        SetFontColourA(colour, AlphaByte(float(colour[3]) * show * alpha));
        msFontShadow = 2;
        SetShadowColourA(black, AlphaByte(show * float(black[3]) * alpha));
        MS_Font_PrintString(320.0f, extraY, line, 1);
        mscMsgTime = mscMsgTime - msGradesRate * msTimeStep;
        if (mscMsgTime <= 0.0f) {
            mscMsgCount -= 1;
            mscMsgTime = mscMsgCount != 0 ? 2.0f : 0.0f;
        }
    }
    if (mscMoney != 0.0f) {
        // the money: its shadow offset by 2 screen pixels, then in green (a gain) or red
        SetShadowColourA(black, AlphaByte(float(black[3]) * alpha));
        if (fading != 0) {
            MS_Font_SetScale(0.9f);
        } else {
            MS_Font_SetSizePreset(3);
        }
        msFontEdge = 3;
        const float money = mscMoney;
        const bool gain = money > 0.0f;
        MSC_Font_AddNumber(gain ? money : -money);
        MS_Font_Format(line, TextOf(gain ? "M_MONEY_POS" : "M_MONEY_NEG"), 0x100);
        SetFontColourA(moneyBack, moneyBack[3]);
        msFontShadow = 0;
        float drop = fading != 0 ? mscMoneyDrop : 30.0f;
        const float x = (right - left) * 0.5f + left;
        MS_Font_PrintString(x - offX, top + drop - offY, line, 1);
        const uint8_t* colour = gain ? mscMsgColours : red;
        SetFontColourA(colour, AlphaByte(float(colour[3]) * alpha));
        msFontShadow = 2;
        drop = fading != 0 ? mscMoneyDrop : 30.0f;
        MS_Font_PrintString(x, top + drop, line, 1);
    }
    MS_Font_Render();
}

} // namespace

#define MGSCREENS44_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

MGSCREENS44_REVERSED("MinigamePopup::Draw_706000", 0x00706000, &PopupDraw);
MGSCREENS44_REVERSED("Minigame::DrawGrades_706600", 0x00706600, &GradesDraw);
MGSCREENS44_REVERSED("Minigame::DrawCompletion_707510", 0x00707510, &CompletionDraw);
