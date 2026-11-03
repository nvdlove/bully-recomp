// Math class (ClassMath): the play: the timer, the cursor (moved by the stick, over an
// answer within 40 units), submitting, and the drawing of the board, question, answers,
// timer, score and help (0x6F8DC0..0x6F9F40, 0x6FA090, 0x6FA220).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/ClassMath.h"
#include "reversed.h"

namespace {

using namespace school;
using namespace cmath;

GAME_VAR(float, mathTimerRate, 0x00BC66CC);   // (0.02)
GAME_VAR(float, mathMessageY, 0x00BC66D4);
GAME_VAR(float, mathScoreY, 0x00BC66D0);
// a font size preset's value (per language: 7 has its own table)
GAME_FN(void, __cdecl, Font_SetPresetSize, 0x006881F0, int preset, float size);
GAME_FN(float*, __cdecl, ScreenPositionScaled, 0x0052D590, float* out, float x, float y, int hAlign, int vAlign);
GAME_FN(float*, __thiscall, Bio_ReadPad, 0x006E4540, void* self, float* out);

const float kUv0[2] = {0.0f, 0.0f};
const float kUv1[2] = {1.0f, 1.0f};

inline void DrawCentred(const float* centre, float halfWidth, float halfHeight, void* texture, const uint8_t* colour) {
    DrawRectUV(centre[0] - halfWidth, centre[1] - halfHeight, centre[0] + halfWidth, centre[1] + halfHeight, texture,
               colour, kUv0, kUv1);
}
inline void* MathTexture(const ClassMath* m, int index) {
    return m->textures != nullptr ? m->textures[index] : nullptr;
}
// the style-3 font without a shadow (not for languages 6 and 7)
inline void MathOutlineFont() {
    if (language != 7 && language != 6 && fontStyleTexture[3] != nullptr) {
        fontStyle = 3;
        fontShadow = 0;
    }
}

// 0x6F8DC0: the timer counts down (the game ends at zero); the warning colour; the trigger
// function
void __fastcall MathUpdateTimer(ClassMath* m, re::Edx) {
    float step = mathTimerRate * timeStep;
    float left = m->timeLeft - step;
    m->timeLeft = left;
    Minigame_SetTimerShown(m, left);
    if (!(0.0f < m->timeLeft)) {
        m->timeLeft = 0.0f;
        m->active = 0;
        return;
    }
    if (m->warnTime < m->timeLeft) {
        SetFontColour(GameColour(0x020C835C));
    } else {
        SetFontColour(GameColour(0x020C8360));
    }
    if (m->triggered == 0 && !(m->triggerTime < m->timeLeft)) {
        m->triggered = 1;
        RunScriptRef(&m->triggerFunction);
    }
}

// 0x6F8EC0: the cursor (dimmed after an answer); a quarter of its texture across, its
// height not scaled by the screen (the game's)
void __fastcall MathDrawCursor(ClassMath* m, re::Edx) {
    float sx = screenScaleX;
    uint8_t colour[4];
    std::memcpy(colour, GameColour(0x020C8354), 4);
    if (m->answered != 0) {
        colour[3] = 0x5A;
    }
    float pos[2];
    ScreenPosition(pos, m->cursorX, m->cursorY, 1, 1);
    void* cursor = MathTexture(m, m->cursorTexture);
    if (m->textures == nullptr || cursor == nullptr) {
        return;
    }
    int halfW = int(float(TexWidth(cursor)) * 0.5f);
    int halfH = int(float(TexHeight(cursor)) * 0.5f);
    float h = float(halfH) * 0.5f;
    float w = 0.5f * float(halfW) * sx;
    DrawCentred(pos, w, h, cursor, colour);
}

// 0x6F9010: the board (852 wide at the screen's scale, 480 high)
void __fastcall MathDrawBackground(ClassMath* m, re::Edx) {
    float halfWidth = screenScaleX * 426.0f;
    float pos[2];
    ScreenPositionScaled(pos, 0.0f, 0.0f, 1, 1);
    void* back = MathTexture(m, MATH_TEX_BACK);
    DrawRectUV(pos[0] - halfWidth, pos[1] - 240.0f, pos[0] + halfWidth, pos[1] + 240.0f, back, GameColour(0x020C8350),
               kUv0, kUv1);
}

// 0x6F90E0: the triangles question's picture
void __fastcall MathDrawShape(ClassMath* m, re::Edx) {
    float sx = screenScaleX;
    float sy = screenScaleY;
    float pos[2];
    ScreenPosition(pos, -155.0f, 100.0f, 1, 1);
    if (m->textures == nullptr || m->textures[m->shapeTexture] == nullptr) {
        return;
    }
    void* shape = m->textures[m->shapeTexture];
    int w = int(TexWidth(shape));
    int h = int(TexHeight(shape));
    DrawCentred(pos, 0.5f * float(w) * sx, float(h) * 0.5f * sy, shape, GameColour(0x020C8374));
}

// 0x6F91F0: the shapes to count (a quarter of their texture's size)
void __fastcall MathDrawShapes(ClassMath* m, re::Edx) {
    float sx = screenScaleX;
    float sy = screenScaleY;
    if (m->textures == nullptr || m->textures[m->shapeTexture] == nullptr) {
        return;
    }
    void* shape = m->textures[m->shapeTexture];
    int quarterW = int(float(TexWidth(shape)) * 0.25f);
    int quarterH = int(float(TexHeight(shape)) * 0.25f);
    if (m->shapeCount <= 0) {
        return;
    }
    float halfHeight = float(quarterH) * 0.5f * sy;
    float halfWidth = 0.5f * float(quarterW) * sx;
    for (int i = 0; i < m->shapeCount; ++i) {
        float pos[2];
        ScreenPosition(pos, m->shapeX[i], m->shapeY[i], 1, 1);
        DrawCentred(pos, halfWidth, halfHeight, shape, GameColour(0x020C8374));
    }
}

// 0x6F9380: the answers: texts (equations and the text questions) or pictures (half size
// for the difference question); highlighted, under the cursor, or plain
void __fastcall MathDrawAnswers(ClassMath* m, re::Edx) {
    float sx = screenScaleX;
    float sy = screenScaleY;
    for (int i = 0; i < m->answerCount; ++i) {
        uint8_t colour[4];
        if (m->answerState[i] == 1) {
            std::memcpy(colour, GameColour(0x020C8378), 4);
        } else if (m->answered == 0 && i == m->cursorAnswer) {
            std::memcpy(colour, GameColour(0x020C837C), 4);
        } else {
            std::memcpy(colour, GameColour(0x020C8374), 4);
        }
        float pos[2];
        ScreenPosition(pos, m->answerX[i], m->answerY[i], 1, 1);
        int type = m->questionType;
        if (m->textQuestion == 1 || type == 0 || type == 1 || type == 9 || type == 0xB || type == 0xA) {
            Font_Reset();
            Font_SetJustify(2);
            Font_SetPresetSize(3, -20.0f);
            Font_SetScale(language == 7 ? 1.25f : 1.5f);
            MathOutlineFont();
            SetFontColour(colour);
            uint16_t text[0x80];
            Font_Format(text, m->answerText[i], 0x80);
            Font_PrintString(pos[0], pos[1], text, 1);
            continue;
        }
        if (m->textures == nullptr || m->textures[m->answerValue[i]] == nullptr) {
            continue;
        }
        void* picture = m->textures[m->answerValue[i]];
        int w = int(TexWidth(picture));
        int h = int(TexHeight(picture));
        if (m->questionType == 8) {
            w = int(float(w) * 0.5f);
            h = int(float(h) * 0.5f);
        }
        float halfHeight = float(h) * 0.5f * sy;
        float halfWidth = 0.5f * float(w) * sx;
        DrawRectUV(pos[0] - halfWidth - 5.0f, pos[1] - halfHeight + 40.0f, pos[0] + halfWidth - 5.0f,
                   pos[1] + halfHeight + 40.0f, picture, colour, kUv0, kUv1);
    }
}

// 0x6F96A0: the timer (minutes, seconds)
void __fastcall MathDrawTimer(ClassMath* m, re::Edx) {
    if (fontStyleTexture[0] != nullptr) {
        fontStyle = 0;
    }
    Font_SetSizePreset(3);
    SetFontColour(GameColour(0x020C8350));
    fontShadow = 2;
    Font_AddNumber(float(int(m->timeLeft / 60.0f)));
    Font_AddNumber(float(int(m->timeLeft) % 60));
    float pos[2];
    ScreenPosition(pos, -150.0f, -190.0f, 1, 1);
    uint16_t text[0x40];
    Font_Format(text, GetText(GameString(0x00947A98)), 0x40);   // MGMA_TIME
    Font_PrintString(pos[0], pos[1], text, 1);
}

// 0x6F97B0: the score (the share right, percent) and the score message reached
void __fastcall MathDrawScore(ClassMath* m, re::Edx) {
    int percent = int(float(m->score) / float(m->questionCount) * 100.0f);
    if (percent < m->passPercentage) {
        SetFontColour(GameColour(0x020C8368));
    } else {
        SetFontColour(GameColour(0x020C8364));
    }
    const char* message = nullptr;
    int count = m->scoreMsgCount;
    if (count > 0) {
        float score = float(percent);
        for (int i = 0; i < count; ++i) {
            if (m->scoreMsgs[i].score <= score) {
                message = m->scoreMsgs[i].key;
            }
        }
    }
    float messagePos[2];
    ScreenPosition(messagePos, 60.0f, mathMessageY, 1, 1);
    if (message != nullptr) {
        Font_PrintString(messagePos[0], messagePos[1], GetText(message), 1);
    }
    Font_AddNumber(float(percent));
    uint16_t text[0x40];
    Font_Format(text, GetText(GameString(0x00947AA4)), 0x40);   // MGMA_SCORE
    float pos[2];
    ScreenPosition(pos, 240.0f, mathScoreY, 1, 1);
    Font_PrintString(pos[0], pos[1], text, 1);
}

// 0x6F99C0: the controls' help (three columns)
void __fastcall MathDrawHelp(ClassMath* m, re::Edx) {
    (void)m;
    float sx = screenScaleX;
    Font_SetJustify(2);
    SetFontColour(GameColour(0x020C836C));
    Font_SetSizePreset(1);
    if (language == 7) {
        Font_SetSizePreset(2);
        if (fontStyleTexture[8] != nullptr) {
            fontStyle = 8;
            fontShadow = 0;
        }
    } else if (language == 6) {
        Font_SetSizePreset(2);
        if (fontStyleTexture[9] != nullptr) {
            fontStyle = 9;
            fontShadow = 0;
        }
    }
    float pos[2];
    ScreenPosition(pos, -220.0f, 176.0f, 1, 1);
    float column = sx * 150.0f;
    float x = column * 0.0f + pos[0];
    Font_PrintString(x, pos[1], GetText(GameString(0x00947ACC)), 1);   // MGMA_SEL
    x = column + pos[0];
    Font_PrintString(x, pos[1], GetText(GameString(0x00947ABC)), 1);   // MGMA_GETINFO
    x = (column + column) + pos[0];
    Font_PrintString(x, pos[1], GetText(GameString(0x00947AB0)), 1);   // MGMA_EXIT
}

// the cursor's hot spot (5 right, 40 up) to an answer's distance
inline float MathAnswerDistance(const ClassMath* m, int i) {
    float hotX = m->cursorX - -5.0f;
    float hotY = m->cursorY - 40.0f;
    return Distance2D(hotX - m->answerX[i], hotY - m->answerY[i]);
}

// 0x6F9B40: submitting: the answer under the cursor (40 units) scores when right
void __fastcall MathSubmit(ClassMath* m, re::Edx) {
    for (int i = 0; i < m->answerCount; ++i) {
        if (!(MathAnswerDistance(m, i) <= 40.0f)) {
            continue;
        }
        m->answerGiven = 1;
        if (i == m->correctAnswer) {
            m->score += 1;
            m->validAnswer = 1;
            PlayClassSound(m, MATH_SOUND_CORRECT);
            return;
        }
        m->invalidAnswer = 1;
        PlayClassSound(m, MATH_SOUND_INCORRECT);
        return;
    }
}

// 0x6F9CC0: the cursor moved by the pad (speed 4 with the trigger, else 9), kept on the
// board; the answer under it; the dimming after an answer ends after 3 seconds
void __fastcall MathMoveCursor(ClassMath* m, re::Edx) {
    if (m->textures != nullptr && m->textures[m->cursorTexture] != nullptr) {
        void* cursor = m->textures[m->cursorTexture];
        float sx = screenScaleX;
        m->cursorWidth = float(TexWidth(cursor)) * sx;
        float sy = screenScaleY;
        m->cursorHeight = float(TexHeight(cursor)) * sy;
    }
    float move[2];
    Bio_ReadPad(m, move);
    float speed = Pad_GetValue(0xD, 0) == 0.0f ? 9.0f : 4.0f;
    float moveX = move[0] * speed;
    float moveY = speed * move[1];
    m->cursorX = m->cursorX + moveX;
    m->cursorY = m->cursorY + moveY;
    float limit = 320.0f / screenScaleX - 50.0f;
    float low = -limit;
    if (!(low < m->cursorX)) {
        m->cursorX = low;
    } else if (!(m->cursorX < limit)) {
        m->cursorX = limit;
    }
    if (!(-190.0f < m->cursorY)) {
        m->cursorY = -190.0f;
    } else if (190.0f <= m->cursorY) {
        m->cursorY = 190.0f;
    }
    m->cursorAnswer = -1;
    for (int i = 0; i < m->answerCount; ++i) {
        if (MathAnswerDistance(m, i) <= 40.0f) {
            m->cursorAnswer = i;
            break;
        }
    }
    if (m->answered == 1) {
        float step = mathTimerRate * timeStep;
        float time = step + m->answeredTime;
        m->answeredTime = time;
        if (!(time < 3.0f)) {
            m->answered = 0;
            m->answeredTime = 0.0f;
        }
    }
}

// 0x6F9F40: the question (the title or the equation) and its picture
void __fastcall MathDrawQuestion(ClassMath* m, re::Edx) {
    Font_Reset();
    Font_SetJustify(2);
    MathOutlineFont();
    float scale = 1.0f;
    if (language == 7) {
        scale = 0.5f;
    }
    if (m->textQuestion == 1) {
        Font_SetPresetSize(3, -20.0f);
        scale = 1.05f;
    } else {
        Font_SetPresetSize(3, -20.0f);
    }
    Font_SetScale(scale);
    SetFontColour(GameColour(0x020C8370));
    float pos[2];
    ScreenPosition(pos, -150.0f, -60.0f, 1, 1);
    uint16_t text[0x80];
    Font_Format(text, m->question, 0x80);
    Font_PrintString(pos[0], pos[1], text, 1);
    if (m->questionType == 9) {
        Math_DrawShape(m);
        return;
    }
    if (m->questionType == 0xB || m->questionType == 0xA) {
        Math_DrawShapes(m);
    }
}

// 0x6FA090 (virtual +4): the popup, the timer, the cursor, submitting (button 7 or 12, then
// 300 ms before another)
void __fastcall MathUpdate(ClassMath* m, re::Edx) {
    if (m->popupShowing != 0) {
        if (m->popupKind == 1) {
            if (musicChannel != nullptr && Channel_IsPlaying(musicChannel) != 0) {
                Channel_Pause(musicChannel);
            }
            if (Popup_Result(&m->popupKind) == 2) {
                m->active = 0;
            }
            return;
        }
        if (m->popupShowing != 0 && m->popupKind == 2) {
            Popup_Result(&m->popupKind);
            return;
        }
    }
    if (userStartResponse != 0) {
        return;
    }
    Math_UpdateTimer(m);
    if (m->active == 0) {
        return;
    }
    if (musicChannel != nullptr && Channel_IsPaused(musicChannel) != 0) {
        Channel_Resume(musicChannel);
    }
    Math_MoveCursor(m);
    if ((Pad_JustDown(7, 0) != 0 || Pad_JustDown(0xC, 0) != 0) && m->submitDelay == 0 && m->answered == 0) {
        Math_Submit(m);
        m->submitDelay = 300;
    }
    if (Pad_JustDown(8, 0) != 0) {
        Popup_Show(&m->popupKind, 1, GameString(0x00944690), GameString(0x00941D14), 7, 8);   // EXIT_QUESTION
        return;
    }
    if (Pad_JustDown(6, 0) != 0) {
        Popup_Show(&m->popupKind, 2, GameString(0x009473D0), GameString(0x009473DC), 7, -1);   // MGMA_INFO
        return;
    }
    if (m->submitDelay != 0) {
        float step = mathTimerRate * timeStep;
        int delay = int(float(m->submitDelay) - step * 1000.0f);
        m->submitDelay = delay;
        if (delay < 0) {
            m->submitDelay = 0;
        }
    }
}

// 0x6FA220 (virtual +8)
void __fastcall MathDraw(ClassMath* m, re::Edx) {
    if (m->hud == 0) {
        return;
    }
    Math_DrawBackground(m);
    Math_DrawQuestion(m);
    Math_DrawAnswers(m);
    Math_DrawTimer(m);
    Math_DrawScore(m);
    Math_DrawHelp(m);
    Math_DrawCursor(m);
    Font_Render();
    if (m->popupShowing != 0) {
        Popup_Draw(&m->popupKind);
    }
}

} // namespace

#define CMATHP43_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CMATHP43_REVERSED("ClassMath::UpdateTimer_6F8DC0", 0x006F8DC0, &MathUpdateTimer);
CMATHP43_REVERSED("ClassMath::DrawCursor_6F8EC0", 0x006F8EC0, &MathDrawCursor);
CMATHP43_REVERSED("ClassMath::DrawBackground_6F9010", 0x006F9010, &MathDrawBackground);
CMATHP43_REVERSED("ClassMath::DrawShape_6F90E0", 0x006F90E0, &MathDrawShape);
CMATHP43_REVERSED("ClassMath::DrawShapes_6F91F0", 0x006F91F0, &MathDrawShapes);
CMATHP43_REVERSED("ClassMath::DrawAnswers_6F9380", 0x006F9380, &MathDrawAnswers);
CMATHP43_REVERSED("ClassMath::DrawTimer_6F96A0", 0x006F96A0, &MathDrawTimer);
CMATHP43_REVERSED("ClassMath::DrawScore_6F97B0", 0x006F97B0, &MathDrawScore);
CMATHP43_REVERSED("ClassMath::DrawHelp_6F99C0", 0x006F99C0, &MathDrawHelp);
CMATHP43_REVERSED("ClassMath::Submit_6F9B40", 0x006F9B40, &MathSubmit);
CMATHP43_REVERSED("ClassMath::MoveCursor_6F9CC0", 0x006F9CC0, &MathMoveCursor);
CMATHP43_REVERSED("ClassMath::DrawQuestion_6F9F40", 0x006F9F40, &MathDrawQuestion);
CMATHP43_REVERSED("ClassMath::Update_6FA090", 0x006FA090, &MathUpdate);
CMATHP43_REVERSED("ClassMath::Draw_6FA220", 0x006FA220, &MathDraw);
