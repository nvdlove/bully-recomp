#pragma once

// Math class (ClassMath, 0xACC bytes, vtable 0x947A70): a question (an equation, or one of
// the picture questions: highest, lowest, tallest... count the triangles, squares, circles)
// with up to nine answers in columns; the player moves a cursor onto one and submits. The
// script sets each question (the ClassMathSet* natives). ClassMathSetup.cpp and
// ClassMathPlay.cpp share this.

#include <cstddef>
#include <cstdint>

#include "game/SchoolClasses.h"
#include "reversed.h"

namespace cmath {

struct MathScoreMsg {
    float score;
    char key[0x40];
};
static_assert(sizeof(MathScoreMsg) == 0x44);

struct ClassMath {
    re::Ptr32<re::Ptr32<void>> vtable;                     // +0x0
    uint8_t active;                    // +0x4
    uint8_t hud;                       // +0x5
    uint8_t success;                   // +0x6
    uint8_t unknown7[0x15];
    re::Ptr32<re::Ptr32<void>> textures;                   // +0x1C ("MG_Math": see MathTexture)
    uint8_t unknown20[4];
    float warnTime;                    // +0x24
    uint8_t unknown28;
    uint8_t triggered;                 // +0x29
    uint8_t unknown2A[2];
    float triggerTime;                 // +0x2C
    uint8_t unknown30[4];
    float timeLeft;                    // +0x34
    int32_t unknown38;                 // +0x38 (1)
    int32_t unknown3C;                 // +0x3C (2)
    uint8_t unknown40[0xC];
    re::Ptr32<uint32_t> sounds;                  // +0x4C
    int32_t popupKind;                 // +0x50
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;              // +0xA0
    uint8_t unknownA1[0x23];
    int32_t level;                     // +0xC4
    int32_t scoreMsgCount;             // +0xC8
    MathScoreMsg scoreMsgs[10];        // +0xCC
    int32_t questionCount;             // +0x374 (ClassMathSetNumQuestions)
    int32_t score;                     // +0x378 questions answered right
    uint8_t unknown37C;                // +0x37C
    uint8_t unknown37D[3];
    uint32_t triggerFunction;          // +0x380
    uint8_t unknown384[4];
    float unknown388;                  // +0x388 (SetTimer's second)
    int32_t passPercentage;            // +0x38C (100)
    uint8_t textQuestion;              // +0x390 the question is the script's text (an equation)
    uint8_t unknown391[3];
    int32_t questionType;              // +0x394 the title shown (-1 for a text question)
    int32_t titleCount;                // +0x398
    uint16_t titles[14][0x21];         // +0x39C "MGMA_QUES_TIT01"..
    uint16_t question[0x21];           // +0x738
    uint8_t unknown77A[2];
    int32_t shapeTexture;              // +0x77C the picture (2 circle, 3 square, or the script's)
    int32_t shapeCount;                // +0x780 the shapes to count
    float shapeX[9];                   // +0x784
    float shapeY[9];                   // +0x7A8
    int32_t answerCount;               // +0x7CC
    int32_t cursorAnswer;              // +0x7D0 the answer under the cursor (-1 none)
    int32_t correctAnswer;             // +0x7D4
    float answerX[9];                  // +0x7D8
    float answerY[9];                  // +0x7FC
    int32_t answerValue[9];            // +0x820 a picture answer's texture
    uint16_t answerText[9][0x21];      // +0x844
    uint8_t answerState[9];            // +0xA96 (1: highlighted)
    uint8_t unknownA9F;
    int32_t submitDelay;               // +0xAA0 ms
    uint8_t answered;                  // +0xAA4 an answer given (the cursor dims for 3 seconds)
    uint8_t unknownAA5[3];
    float answeredTime;                // +0xAA8
    uint8_t answerGiven;               // +0xAAC
    uint8_t validAnswer;               // +0xAAD
    uint8_t invalidAnswer;             // +0xAAE
    uint8_t unknownAAF;
    float cursorWidth;                 // +0xAB0
    float cursorHeight;                // +0xAB4
    float cursorX;                     // +0xAB8
    float cursorY;                     // +0xABC
    float unknownAC0;                  // +0xAC0 (4)
    int32_t cursorTexture;             // +0xAC4 (1)
    uint8_t unknownAC8[4];
};
static_assert(offsetof(ClassMath, scoreMsgs) == 0xCC);
static_assert(offsetof(ClassMath, questionCount) == 0x374);
static_assert(offsetof(ClassMath, triggerFunction) == 0x380);
static_assert(offsetof(ClassMath, textQuestion) == 0x390);
static_assert(offsetof(ClassMath, titles) == 0x39C);
static_assert(offsetof(ClassMath, question) == 0x738);
static_assert(offsetof(ClassMath, shapeTexture) == 0x77C);
static_assert(offsetof(ClassMath, answerCount) == 0x7CC);
static_assert(offsetof(ClassMath, answerValue) == 0x820);
static_assert(offsetof(ClassMath, answerText) == 0x844);
static_assert(offsetof(ClassMath, answerState) == 0xA96);
static_assert(offsetof(ClassMath, submitDelay) == 0xAA0);
static_assert(offsetof(ClassMath, cursorX) == 0xAB8);
static_assert(offsetof(ClassMath, cursorTexture) == 0xAC4);
static_assert(sizeof(ClassMath) == 0xACC);

// the "MG_Math" textures (+0x1C), the first ones
enum MathTexture : int {
    MATH_TEX_BACK, MATH_TEX_CURSOR, MATH_TEX_CIRCLE, MATH_TEX_SQUARE,
};
// the sounds (+0x4C: "MthClass")
enum MathSound : int {
    MATH_SOUND_NEW_EQUATION, MATH_SOUND_CORRECT, MATH_SOUND_INCORRECT, MATH_SOUND_TIME_WARN, MATH_SOUND_TIME_WARN_LOOP,
};

// the class's own functions, through the game's addresses
GAME_FN(void, __thiscall, Math_Jitter, 0x006F7CB0, ClassMath* self);
GAME_FN(void, __thiscall, Math_SetAnswers, 0x006F8050, ClassMath* self, int count, int correct);
GAME_FN(void, __thiscall, Math_SetTextQuestion, 0x006F8280, ClassMath* self, const char* text, int count, int correct,
        const re::Ptr32<const char>* answers);
GAME_FN(void, __thiscall, Math_SetHighest, 0x006F8300, ClassMath* self, int count, int correct, const re::Ptr32<const char>* answers);
GAME_FN(void, __thiscall, Math_SetLowest, 0x006F83D0, ClassMath* self, int count, int correct, const re::Ptr32<const char>* answers);
GAME_FN(void, __thiscall, Math_SetDifference, 0x006F84A0, ClassMath* self, int count, int correct, int other, int right);
GAME_FN(void, __thiscall, Math_SetTriangles, 0x006F8570, ClassMath* self, int shape, int count, int correct,
        const re::Ptr32<const char>* answers);
GAME_FN(void, __thiscall, Math_SetTallest, 0x006F8650, ClassMath* self, int count, int correct, const int* answers);
GAME_FN(void, __thiscall, Math_SetShortest, 0x006F8710, ClassMath* self, int count, int correct, const int* answers);
GAME_FN(void, __thiscall, Math_SetFastest, 0x006F87D0, ClassMath* self, int count, int correct, const int* answers);
GAME_FN(void, __thiscall, Math_SetSlowest, 0x006F8890, ClassMath* self, int count, int correct, const int* answers);
GAME_FN(void, __thiscall, Math_SetFattest, 0x006F8950, ClassMath* self, int count, int correct, const int* answers);
GAME_FN(void, __thiscall, Math_SetThinnest, 0x006F8A10, ClassMath* self, int count, int correct, const int* answers);
GAME_FN(void, __thiscall, Math_SetSquares, 0x006F8AD0, ClassMath* self, int shapes, int count, int correct,
        const re::Ptr32<const char>* answers);
GAME_FN(void, __thiscall, Math_SetCircles, 0x006F8BC0, ClassMath* self, int shapes, int count, int correct,
        const re::Ptr32<const char>* answers);
GAME_FN(void, __thiscall, Math_SetTimer, 0x006F8D50, ClassMath* self, float time, float second);
GAME_FN(void, __thiscall, Math_UpdateTimer, 0x006F8DC0, ClassMath* self);
GAME_FN(void, __thiscall, Math_DrawCursor, 0x006F8EC0, ClassMath* self);
GAME_FN(void, __thiscall, Math_DrawBackground, 0x006F9010, ClassMath* self);
GAME_FN(void, __thiscall, Math_DrawShape, 0x006F90E0, ClassMath* self);
GAME_FN(void, __thiscall, Math_DrawShapes, 0x006F91F0, ClassMath* self);
GAME_FN(void, __thiscall, Math_DrawAnswers, 0x006F9380, ClassMath* self);
GAME_FN(void, __thiscall, Math_DrawTimer, 0x006F96A0, ClassMath* self);
GAME_FN(void, __thiscall, Math_DrawScore, 0x006F97B0, ClassMath* self);
GAME_FN(void, __thiscall, Math_DrawHelp, 0x006F99C0, ClassMath* self);
GAME_FN(void, __thiscall, Math_Submit, 0x006F9B40, ClassMath* self);
GAME_FN(void, __thiscall, Math_MoveCursor, 0x006F9CC0, ClassMath* self);
GAME_FN(void, __thiscall, Math_DrawQuestion, 0x006F9F40, ClassMath* self);

} // namespace cmath
