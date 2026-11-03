// English class (ClassEnglish): the word game's update and draw virtuals (0x6ECF80,
// 0x6ED500) and two of its script natives; the rest of the class (the letter tiles, the
// word checks, set-up) is another pass's (0x6EB8B0..0x6EC9C0).
//
// The player builds words from the scrambled letters (the tiles); every word of the list
// found counts its letters. The game ends when all are found or the time runs out, a
// success when the letters found reach the minimum share (+0x4E8).

#include <cstddef>
#include <cstdint>

#include "game/SchoolClasses.h"
#include "game/ScriptApi.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using namespace school;

// a score message (ClassEnglishSetScoreMsg / ClassMathSetScoreMsg): shown from `score` up
struct EnglishScoreMsg {
    float score;
    char key[0x40];
};
static_assert(sizeof(EnglishScoreMsg) == 0x44);

struct ClassEnglish {
    re::Ptr32<re::Ptr32<void>> vtable;                     // +0x0
    uint8_t active;                    // +0x4
    uint8_t hud;                       // +0x5
    uint8_t success;                   // +0x6
    uint8_t unknown7[0x15];
    re::Ptr32<re::Ptr32<void>> textures;                   // +0x1C ([0] the title, [1..16] the background, [17] the side bar)
    uint8_t unknown20[0x2C];
    re::Ptr32<uint32_t> sounds;                  // +0x4C
    int32_t popupKind;                 // +0x50 the popup (1 the exit question, 2 the info)
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;              // +0xA0
    uint8_t unknownA1[0x23];
    int32_t level;                     // +0xC4
    int32_t scoreMsgCount;             // +0xC8
    EnglishScoreMsg scoreMsgs[10];     // +0xCC
    uint8_t warned;                    // +0x374 the time warning played
    uint8_t unknown375[0x133];
    int32_t unknown4A8;                // +0x4A8 (the submit's argument)
    int32_t tileCount;                 // +0x4AC the letter tiles (0x1C bytes each; +0x3A2 a byte)
    int32_t unknown4B0;                // +0x4B0 (non-zero: the delete makes its sound)
    float warnTime;                    // +0x4B4
    float triggerTime;                 // +0x4B8 the script's function runs below this
    uint32_t triggerFunction;          // +0x4BC
    float penaltyTime;                 // +0x4C0
    float unknown4C4;                  // +0x4C4 (SetTimer's second)
    float timeLeft;                    // +0x4C8 seconds
    uint8_t unknown4CC[0x1C];
    float minPercentage;               // +0x4E8 the share of letters to find (0..1)
    int32_t padRepeat;                 // +0x4EC ms before the stick moves the cursor again
    uint8_t entering;                  // +0x4F0 the letters row is active (else the words)
    uint8_t unknown4F1;
    uint16_t words[70][9];             // +0x4F2 the list
    uint8_t found[70];                 // +0x9DE
    int32_t wordCount;                 // +0xA24
    uint8_t unknownA28[0x338];
};
static_assert(offsetof(ClassEnglish, popupKind) == 0x50);
static_assert(offsetof(ClassEnglish, popupShowing) == 0xA0);
static_assert(offsetof(ClassEnglish, scoreMsgs) == 0xCC);
static_assert(offsetof(ClassEnglish, warned) == 0x374);
static_assert(offsetof(ClassEnglish, unknown4A8) == 0x4A8);
static_assert(offsetof(ClassEnglish, timeLeft) == 0x4C8);
static_assert(offsetof(ClassEnglish, minPercentage) == 0x4E8);
static_assert(offsetof(ClassEnglish, words) == 0x4F2);
static_assert(offsetof(ClassEnglish, found) == 0x9DE);
static_assert(offsetof(ClassEnglish, wordCount) == 0xA24);
static_assert(sizeof(ClassEnglish) == 0xD60);   // (the minigame factory 0x704890 allocates it)

// the English sounds (+0x4C)
enum EnglishSound : int {
    ENG_SOUND_MOVE = 0, ENG_SOUND_SUBMIT = 1, ENG_SOUND_DELETE = 2, ENG_SOUND_SWITCH = 5,
    ENG_SOUND_TIME_WARNING = 7, ENG_SOUND_SCRAMBLE = 8,
};

GAME_VAR(float, engTimerRate, 0x00BC656C);    // (0.02)
GAME_VAR(float, engMessageY, 0x00BC6574);
GAME_VAR(float, engScoreY, 0x00BC6570);

// the class's own (another pass's)
GAME_FN(int, __thiscall, English_LettersFound, 0x006EC2D0, ClassEnglish* self);
GAME_FN(int, __thiscall, English_LettersTotal, 0x006EC320, ClassEnglish* self);
GAME_FN(void, __thiscall, English_CursorRight, 0x006EB8E0, ClassEnglish* self);
GAME_FN(void, __thiscall, English_CursorLeft, 0x006EB930, ClassEnglish* self);
GAME_FN(void, __thiscall, English_Delete, 0x006EB9A0, ClassEnglish* self);
GAME_FN(void, __thiscall, English_Scramble, 0x006EBA60, ClassEnglish* self);
GAME_FN(void, __thiscall, English_Submit, 0x006EBB80, ClassEnglish* self);
GAME_FN(void, __thiscall, English_PickLetter, 0x006EC8F0, ClassEnglish* self, int);
GAME_FN(void, __thiscall, English_DrawWords, 0x006EBE90, ClassEnglish* self, int);
GAME_FN(void, __thiscall, English_DrawLetters, 0x006EC690, ClassEnglish* self);
GAME_FN(uint8_t*, __thiscall, English_PulseColour, 0x006EC3A0, ClassEnglish* self, uint8_t* out, const uint8_t* a,
        const uint8_t* b);
GAME_FN(void, __thiscall, English_SetTimer, 0x006EC360, void* self, float time, float second);

// a texture of the set (nullptr without one)
inline void* EnglishTexture(const ClassEnglish* e, int index) {
    return e->textures != nullptr ? e->textures[index] : nullptr;
}

// the share of letters found
inline float EnglishRatio(ClassEnglish* e) {
    float found = float(English_LettersFound(e));
    float total = float(English_LettersTotal(e));
    return found / total;
}

// the game over: a success with enough of the letters
inline void EnglishFinish(ClassEnglish* e, float ratio) {
    if (e->minPercentage <= ratio) {
        e->success = 1;
    }
    e->active = 0;
}

// a pad direction pressed: the stick past 0.05 (only when the repeat delay allows)
inline bool EnglishStickBelow(int control, bool idle) {
    return Pad_GetValue(control, 0) < -0.05 && idle;
}
inline bool EnglishStickAbove(int control, bool idle) {
    return Pad_GetValue(control, 0) > 0.05 && idle;
}

// 0x6ECF80 (virtual +4): the popup, the timer and the controls
void __fastcall EnglishUpdate(ClassEnglish* e, re::Edx) {
    if (e->popupShowing != 0) {
        if (e->popupKind == 1) {
            if (Channel_IsPlaying(musicChannel) != 0) {
                Channel_Pause(musicChannel);
            }
            if (Popup_Result(&e->popupKind) != 2) {
                return;
            }
            EnglishFinish(e, EnglishRatio(e));
            return;
        }
        if (e->popupShowing != 0 && e->popupKind == 2) {
            Popup_Result(&e->popupKind);
            return;
        }
    }
    if (userStartResponse != 0) {
        return;
    }
    if (English_LettersFound(e) == English_LettersTotal(e)) {
        e->success = 1;
        e->active = 0;
        return;
    }
    float step = engTimerRate * timeStep;
    float left = e->timeLeft - step;
    e->timeLeft = left;
    if (!(0.0f < left)) {
        e->timeLeft = 0.0f;
        EnglishFinish(e, EnglishRatio(e));
        return;
    }
    if (e->active == 0) {
        return;
    }
    if (Channel_IsPaused(musicChannel) != 0) {
        Channel_Resume(musicChannel);
    }

    // left / right along the row (the d-pad buttons, or the sticks with the repeat delay)
    if ((Pad_GetValue(1, 0) != 0.0f && e->padRepeat == 0) || Pad_JustDown(1, 0) != 0 ||
        EnglishStickBelow(0x10, e->padRepeat == 0) || EnglishStickBelow(0x12, e->padRepeat == 0)) {
        if (e->entering == 0) {
            PlayClassSound(e, ENG_SOUND_MOVE);
            English_CursorLeft(e);
            e->padRepeat = 300;
        }
    } else if ((Pad_GetValue(0, 0) != 0.0f && e->padRepeat == 0) || Pad_JustDown(0, 0) != 0 ||
               EnglishStickAbove(0x10, e->padRepeat == 0) || EnglishStickAbove(0x12, e->padRepeat == 0)) {
        if (e->entering == 0) {
            PlayClassSound(e, ENG_SOUND_MOVE);
            English_CursorRight(e);
            e->padRepeat = 300;
        }
    } else if (Pad_JustDown(2, 0) != 0 || Pad_GetValue(0x11, 0) > 0.05 || Pad_GetValue(0x13, 0) > 0.05) {
        // up: back to the words (when a tile is chosen)
        if (e->entering != 0) {
            for (int i = 0; i < e->tileCount; ++i) {
                if (At<uint8_t>(e, 0x3A2 + i * 0x1C) != 0) {
                    PlayClassSound(e, ENG_SOUND_SWITCH);
                    e->entering = 0;
                    break;
                }
            }
        }
    } else if (Pad_JustDown(3, 0) != 0 || Pad_GetValue(0x11, 0) < -0.05 || Pad_GetValue(0x13, 0) < -0.05) {
        // down: to the letters
        if (e->entering == 0) {
            PlayClassSound(e, ENG_SOUND_SWITCH);
            e->entering = 1;
        }
    }

    if (Pad_JustDown(7, 0) != 0) {
        if (e->entering != 0) {
            English_Submit(e);
        } else {
            PlayClassSound(e, ENG_SOUND_SUBMIT);
            English_PickLetter(e, e->unknown4A8);
        }
    }
    if (Pad_JustDown(0xC, 0) != 0) {
        PlayClassSound(e, ENG_SOUND_SCRAMBLE);
        English_Scramble(e);
    }
    if (Pad_JustDown(6, 0) != 0) {
        if (e->unknown4B0 != 0) {
            PlayClassSound(e, ENG_SOUND_DELETE);
        }
        English_Delete(e);
    }
    if (Pad_JustDown(8, 0) != 0) {
        Popup_Show(&e->popupKind, 1, GameString(0x00944690), GameString(0x00941D14), 7, 8);   // EXIT_QUESTION
        return;
    }
    if (Pad_JustDown(9, 0) != 0) {
        Popup_Show(&e->popupKind, 2, GameString(0x0094552C), GameString(0x00945538), 7, -1);  // MGCE_INFO
        return;
    }
    if (e->padRepeat != 0) {
        float stepMs = engTimerRate * timeStep;
        int repeat = int(float(e->padRepeat) - stepMs * 1000.0f);
        e->padRepeat = repeat;
        if (repeat < 0) {
            e->padRepeat = 0;
        }
    }
}

// the style-3 font with a outline (not for languages 6 and 7)
inline bool EnglishOutlineFont() {
    return language != 7 && language != 6 && fontStyleTexture[3] != nullptr;
}

// 0x6ED500 (virtual +8): the board, the timer, the score, the controls' help
void __fastcall EnglishDraw(ClassEnglish* e, re::Edx) {
    if (e->hud == 0) {
        return;
    }
    const float uv0[2] = {0.0f, 0.0f};
    const float uv1[2] = {1.0f, 1.0f};
    float sx = screenScaleX;
    float sy = screenScaleY;
    float left = 320.0f - sx * 320.0f;
    float width = sx * 160.0f;
    // the background: 4x4 textures over the 4:3 area
    for (int row = 0; row < 4; ++row) {
        float y1 = float(row) * 120.0f;
        float y2 = y1 + 120.0f;
        for (int col = 0; col < 4; ++col) {
            void* texture = EnglishTexture(e, 1 + row * 4 + col);
            float x1 = float(col) * width + left;
            float x2 = x1 + width;
            DrawRectUV(x1, y1, x2, y2, texture, GameColour(0x020C82B4), uv0, uv1);
        }
    }
    if (widescreen != 0) {
        uint8_t white[4];
        float rect[4] = {0.0f, 480.0f, left, 0.0f};
        void* bar = EnglishTexture(e, 17);
        DrawSprite(rect, bar, RGBA_Set(white, 0xFF, 0xFF, 0xFF, 0xFF), uv0, uv1);
        bar = EnglishTexture(e, 17);
        float rect2[4] = {640.0f - left, 480.0f, 640.0f, 0.0f};
        DrawSprite(rect2, bar, RGBA_Set(white, 0xFF, 0xFF, 0xFF, 0xFF), uv0, uv1);
    }
    // the title
    float title[2];
    ScreenPosition(title, 110.0f, 46.0f, 1, 1);
    void* titleTexture = EnglishTexture(e, 0);
    float halfWidth = sx * 70.0f;
    float height = sy * 43.0f;
    DrawRectUV(title[0] - halfWidth, title[1], title[0] + halfWidth, title[1] + height, titleTexture,
               GameColour(0x020C82B8), uv0, uv1);

    Font_Reset();
    if (EnglishOutlineFont()) {
        fontStyle = 3;
    }
    fontShadow = 0;
    uint8_t rgba[4];
    SetFontShadowColour(RGBA_Set(rgba, 0x10, 0x10, 0x10, 0xFF));
    Font_SetJustify(2);
    SetFontColour(GameColour(0x020C82C0));
    Font_SetScale(1.0f);
    English_DrawLetters(e);
    Font_SetJustify(0);
    SetFontColour(GameColour(0x020C82BC));
    Font_SetScale(0.46f);
    English_DrawWords(e, 0);
    if (fontStyleTexture[0] != nullptr) {
        fontStyle = 0;
    }
    Font_SetSizePreset(3);
    fontShadow = 2;

    // the timer (the warning colour and sound below the warning time)
    if (e->warnTime < e->timeLeft) {
        SetFontColour(GameColour(0x020C82EC));
    } else {
        if (e->warned == 0) {
            PlayClassSound(e, ENG_SOUND_TIME_WARNING);
            e->warned = 1;
        }
        SetFontColour(GameColour(0x020C82F0));
    }
    if (!(0.0f == e->triggerTime) && !(e->triggerTime < e->timeLeft)) {
        e->triggerTime = 0.0f;
        RunScriptRef(&e->triggerFunction);
    }
    uint16_t text[0x40];
    Font_AddNumber(float(int(e->timeLeft / 60.0f)));
    Font_AddNumber(float(int(e->timeLeft) % 60));
    Font_Format(text, GetText(GameString(0x00945778)), 0x40);   // MGCE_TIME
    float pos[2];
    ScreenPosition(pos, -270.0f, -199.0f, 1, 1);
    Font_PrintString(pos[0], pos[1], text, 1);

    // the score: the share of the letters found
    float found = float(English_LettersFound(e));
    int total = 0;
    for (int i = 0; i < e->wordCount; ++i) {
        total += WStrLen(e->words[i]);
    }
    float ratio = found / float(total);
    int percent = int(ratio * 100.0f);
    if (e->minPercentage <= ratio) {
        SetFontColour(GameColour(0x020C82F4));
    } else {
        SetFontColour(GameColour(0x020C82F8));
    }
    const char* message = nullptr;
    if (e->scoreMsgCount > 0) {
        float score = float(percent);
        for (int i = 0; i < e->scoreMsgCount; ++i) {
            if (e->scoreMsgs[i].score <= score) {
                message = e->scoreMsgs[i].key;
            }
        }
    }
    float messagePos[2];
    ScreenPosition(messagePos, 60.0f, engMessageY, 1, 1);
    if (message != nullptr) {
        Font_PrintString(messagePos[0], messagePos[1], GetText(message), 1);
    }
    Font_AddNumber(float(percent));
    Font_Format(text, GetText(GameString(0x0094576C)), 0x40);   // MGCE_SCORE
    float scorePos[2];
    ScreenPosition(scorePos, -20.0f, engScoreY, 1, 1);
    Font_PrintString(scorePos[0], scorePos[1], text, 1);

    // the controls' help (two columns)
    Font_SetJustify(2);
    SetFontColour(GameColour(0x020C82FC));
    Font_SetSizePreset(1);
    float help[2];
    ScreenPosition(help, -100.0f, 176.0f, 1, 1);
    float column = sx * 200.0f;
    float zero = column * 0.0f;
    float x = zero + help[0];
    Font_PrintString(x, help[1], GetText(GameString(0x00945760)), 1);   // MGCE_SEL
    x = column + help[0];
    Font_PrintString(x, help[1], GetText(GameString(0x00945754)), 1);   // MGCE_DEL
    x = help[0] + zero;
    Font_PrintString(x, help[1] + 25.0f, GetText(GameString(0x00945748)), 1);   // MGCE_SCRAM
    x = help[0] + column;
    Font_PrintString(x, help[1] + 25.0f, GetText(GameString(0x0094573C)), 1);   // MGCE_EXIT
    Font_Render();

    // the "enter" button and its label by the title (pulsing while entering)
    Font_SetScale(0.72f);
    if (EnglishOutlineFont()) {
        fontStyle = 3;
        fontShadow = 0;
    }
    void* button = GetButtonTexture(7);
    if (button != nullptr) {
        float buttonWidth = float(vc::Call<uint32_t>(button, 0x48)) * 32.0f;
        float buttonHeight = float(vc::Call<uint32_t>(button, 0x4C));
        float scale = buttonWidth / buttonHeight;
        float w = sx * scale;
        float right = sx * -20.0f + title[0] - 5.0f;
        uint8_t pulse[4];
        const uint8_t* colour = e->entering != 0
                                    ? English_PulseColour(e, pulse, GameColour(0x020C82E8), GameColour(0x020C82D8))
                                    : GameColour(0x020C82D0);
        float top = title[1] + 4.5f;
        DrawRectUV(right - w, top, right, top + 32.0f, button, colour, uv0, uv1);
    }
    const uint16_t* enter = GetText(GameString(0x00945730));   // MGCE_ENTER
    Font_SetJustify(0);
    if (e->entering != 0) {
        fontFlags |= 0x20;
        fontShadow = 2;
        uint8_t pulse[4];
        SetFontShadowColour(English_PulseColour(e, pulse, GameColour(0x020C82E8), GameColour(0x020C82D8)));
        SetFontColour(English_PulseColour(e, pulse, GameColour(0x020C82E4), GameColour(0x020C82D4)));
    } else {
        SetFontColour(GameColour(0x020C82D0));
        fontShadow = 0;
    }
    Font_PrintString(sx * -20.0f + title[0], title[1] + 4.5f, enter, 1);
    Font_Render();
    if (e->popupShowing != 0) {
        Popup_Draw(&e->popupKind);
    }
}

// ---- its script natives ----------------------------------------------------------------------

// 0x6EE080: ClassEnglishSetTimer(time [, second])
int __cdecl Native_ClassEnglishSetTimer(lua_State* L) {
    Script::GetArgCount(L);
    float time = Script::GetFloat(L, 0);
    float second = 0.0f;
    if (Script::GetArgCount(L) >= 2) {
        second = Script::GetFloat(L, 1);
    }
    English_SetTimer(currentMinigame, time, second);
    return 0;
}

// 0x6EE1A0: ClassEnglishGetScorePercentage: the letters found, percent
int __cdecl Native_ClassEnglishGetScorePercentage(lua_State* L) {
    ClassEnglish* e = re::PtrCast<ClassEnglish*>(currentMinigame);
    float found = float(English_LettersFound(e));
    e = re::PtrCast<ClassEnglish*>(currentMinigame);
    float total = float(English_LettersTotal(e));
    Script::PushFloat(L, found / total * 100.0f);
    return 1;
}

} // namespace

#define CENG43_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CENG43_REVERSED("ClassEnglish::Update_6ECF80", 0x006ECF80, &EnglishUpdate);
CENG43_REVERSED("ClassEnglish::Draw_6ED500", 0x006ED500, &EnglishDraw);
CENG43_REVERSED("Native_ClassEnglishSetTimer", 0x006EE080, &Native_ClassEnglishSetTimer);
CENG43_REVERSED("Native_ClassEnglishGetScorePercentage", 0x006EE1A0, &Native_ClassEnglishGetScorePercentage);
