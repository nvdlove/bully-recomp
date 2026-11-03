// Math class (ClassMath): its set-up and end, the questions the script sets (one setter per
// kind of question, written once per shape as templates), the small accessors and the
// script natives (0x6F7420..0x6F8D70, 0x6F9C60, 0x6FA280..0x6FAA70).
//
// A question: its answers' count and the right one (0x6F8050 places them in columns), the
// title (the type's, from "MGMA_QUES_TITnn", or the script's text) and the answers: texts,
// or picture textures. Register hazard: the game's setters 0x6F84A0..0x6F8BC0 use ecx
// after calling 0x6F8050 (and 0x6F8AD0 / 0x6F8BC0 call 0x6F7CB0 with it), relying on it
// still being the class: switch 0x6F8050 to ours only with them.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/ClassMath.h"
#include "game/ScriptApi.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using namespace school;
using namespace cmath;

// the answers' columns (language 7 moves them)
GAME_VAR(float, mathColumn1, 0x00BC66E0);
GAME_VAR(float, mathColumn2, 0x00BC66DC);
GAME_VAR(float, mathColumn3, 0x00BC66D8);
GAME_FN(void, __cdecl, FontStyle_Load, 0x00687BC0, int style);
GAME_FN(void, __cdecl, FontStyle_Unload, 0x00687D10, int style);
GAME_FN(void, __cdecl, PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopMemoryTag, 0x0052E710);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(int, __cdecl, WideToInt, 0x0043A5C0, const uint16_t* text);
GAME_FN(void, __cdecl, AsciiToWide, 0x00690090, const char* src, uint16_t* dst);

// 0x6F7420 (virtual +0x10): the game starts: the cursor, the question titles, the
// textures (the shapes, the height / size / speed pictures), the font, the sounds, the info
const ClassTextureEntry kMathTextures[51] = {
        {0, GameString(0x00947A1C), nullptr},   // math_back
        {1, GameString(0x00947A10), GameString(0x00947A00)},   // math_cursor / math_cursor_x
        {2, GameString(0x009479F4), GameString(0x009479E4)},   // math_circle / math_circle_x
        {3, GameString(0x009479D8), GameString(0x009479C8)},   // math_square / math_square_x
        {4, GameString(0x009479C0), GameString(0x009479B8)},   // tri01 / tri01_x
        {5, GameString(0x009479B0), GameString(0x009479A8)},   // tri02 / tri02_x
        {6, GameString(0x009479A0), GameString(0x00947998)},   // tri03 / tri03_x
        {7, GameString(0x00947990), GameString(0x00947988)},   // tri04 / tri04_x
        {8, GameString(0x00947980), GameString(0x00947978)},   // tri05 / tri05_x
        {9, GameString(0x00947970), GameString(0x00947968)},   // tri06 / tri06_x
        {10, GameString(0x0094795C), GameString(0x0094794C)},   // height_barn / height_barn_x
        {11, GameString(0x00947938), GameString(0x00947924)},   // height_birdhouse / height_birdhouse_x
        {12, GameString(0x00947910), GameString(0x009478FC)},   // height_cathedral / height_cathedral_x
        {13, GameString(0x009478EC), GameString(0x009478D8)},   // height_doghouse / height_doghouse_x
        {14, GameString(0x009478C4), GameString(0x009478B0)},   // height_eiffltower / height_eiffltower_x
        {15, GameString(0x009478A0), GameString(0x00947890)},   // height_house / height_house_x
        {16, GameString(0x00947880), GameString(0x0094786C)},   // height_mailbox / height_mailbox_x
        {17, GameString(0x00947858), GameString(0x00947844)},   // height_phonebooth / height_phonebooth_x
        {18, GameString(0x00947834), GameString(0x00947820)},   // height_pyramid / height_pyramid_x
        {19, GameString(0x0094780C), GameString(0x009477F8)},   // height_skyscraper / height_skyscraper_x
        {20, GameString(0x009477EC), GameString(0x009477DC)},   // height_tent / height_tent_x
        {21, GameString(0x009477C8), GameString(0x009477B4)},   // height_tombstone / height_tombstone_x
        {22, GameString(0x009477A8), GameString(0x00947798)},   // size_amoeba / size_amoeba_x
        {23, GameString(0x0094778C), GameString(0x00947780)},   // size_ant / size_ant_x
        {24, GameString(0x00947770), GameString(0x0094775C)},   // size_basktball / size_basktball_x
        {25, GameString(0x00947750), GameString(0x00947740)},   // size_bridge / size_bridge_x
        {26, GameString(0x00947734), GameString(0x0094771C)},   // size_chair / size_chair_x
        {27, GameString(0x0094770C), GameString(0x009476F8)},   // size_dumptruck / size_dumptruck_x
        {28, GameString(0x009476E4), GameString(0x009476D0)},   // size_fruitbasket / size_fruitbasket_x
        {29, GameString(0x009476C4), GameString(0x009476B8)},   // size_kcar / size_kcar_x
        {30, GameString(0x009476AC), GameString(0x009476A0)},   // size_key / size_key_x
        {31, GameString(0x00947690), GameString(0x00947680)},   // size_mosquito / size_mosquito_x
        {32, GameString(0x00947674), GameString(0x00947664)},   // size_saturn / size_saturn_x
        {33, GameString(0x00947658), GameString(0x0094764C)},   // size_ship / size_ship_x
        {34, GameString(0x0094763C), GameString(0x00947628)},   // size_slicepizza / size_slicepizza_x
        {35, GameString(0x00947618), GameString(0x00947604)},   // size_thumbtack / size_thumbtack_x
        {36, GameString(0x009475F4), GameString(0x009475E0)},   // size_watermelon / size_watermelon_x
        {37, GameString(0x009475D4), GameString(0x009475C4)},   // size_whale / size_whale_x
        {38, GameString(0x009475B8), GameString(0x009475A8)},   // speed_bike / speed_bike_x
        {39, GameString(0x00947598), GameString(0x00947584)},   // speed_golfcart / speed_golfcart_x
        {40, GameString(0x00947574), GameString(0x00947564)},   // speed_lawnmow / speed_lawnmow_x
        {41, GameString(0x00947554), GameString(0x00947544)},   // speed_person / speed_person_x
        {42, GameString(0x00947538), GameString(0x00947528)},   // speed_plane / speed_plane_x
        {43, GameString(0x0094751C), GameString(0x0094750C)},   // speed_puppy / speed_puppy_x
        {44, GameString(0x009474FC), GameString(0x009474EC)},   // speed_racecar / speed_racecar_x
        {45, GameString(0x009474DC), GameString(0x009474CC)},   // speed_rocket / speed_rocket_x
        {46, GameString(0x009474C0), GameString(0x009474B0)},   // speed_sloth / speed_sloth_x
        {47, GameString(0x009474A4), GameString(0x00947494)},   // speed_snail / speed_snail_x
        {48, GameString(0x00947488), GameString(0x00947478)},   // speed_train / speed_train_x
        {49, GameString(0x00947468), GameString(0x00947458)},   // speed_turtle / speed_turtle_x
        {50, GameString(0x0094744C), GameString(0x0094743C)},   // speed_worm / speed_worm_x
};

void __fastcall MathStart(ClassMath* m, re::Edx) {
    m->warnTime = 30.0f;
    m->triggerTime = 15.0f;
    m->active = 1;
    m->cursorWidth = 1.0f;
    m->hud = 0;
    m->cursorHeight = 1.0f;
    m->unknown37C = 0;
    m->passPercentage = 100;
    m->cursorX = 20.0f;
    m->cursorAnswer = -1;
    m->submitDelay = 0;
    m->cursorY = -20.0f;
    m->answered = 0;
    m->unknownAC0 = 4.0f;
    m->cursorTexture = 1;
    m->answeredTime = 0.0f;
    std::memset(m->answerState, 0, sizeof(m->answerState));
    m->score = 0;
    char key[32];
    Sprintf(key, GameString(0x00947A3C));   // MGMA_NUM_QTITLES
    int titles = WideToInt(GetText(key));
    m->titleCount = titles;
    for (int i = 0; i < m->titleCount; ++i) {
        Sprintf(key, GameString(0x00947A28), i + 1);   // MGMA_QUES_TIT%02d
        Font_Format(m->titles[i], GetText(key), 0x20);
    }
    Minigame_LoadTextures(m, 51, kMathTextures, GameString(0x0094772C));   // MG_Math
    PushTxdMemoryTag();
    FontStyle_Load(3);
    PopMemoryTag();
    const ClassSoundEntry soundTable[5] = {
        {0, GameString(0x00947424)},   // NewEquation
        {1, GameString(0x00947418)},   // MathCorrect
        {2, GameString(0x00947408)},   // MathIncorrect
        {3, GameString(0x009473F8)},   // MathTimeWarn
        {4, GameString(0x009473E8)},   // MathTimeWarnLOO
    };
    Minigame_LoadSounds(m, 5, soundTable, GameString(0x00947430));   // MthClass
    Popup_Show(&m->popupKind, 2, GameString(0x009473D0), GameString(0x009473DC), 7, -1);   // MGMA_INFO
    radarHidden = 0;
    Camera_SetFov(camera, 1.6f);
}

// 0x6F7C70 (virtual +0x14): the game ends
void __fastcall MathEnd(ClassMath* m, re::Edx) {
    m->active = 0;
    m->hud = 0;
    Minigame_EndTextures(m);
    FontStyle_Unload(3);
    Minigame_EndSounds(m);
    radarHidden = 1;
    Camera_RestoreFov(camera);
}

// 0x6F7CB0: the counted shapes' positions, each jittered by -8..8 (two columns of rows)
void __fastcall MathJitter(ClassMath* m, re::Edx) {
    struct Place {
        float* field;
        float base;
    };
    const Place places[18] = {
        {&m->shapeX[0], -55.0f},  {&m->shapeY[0], 70.0f},  {&m->shapeX[1], -130.0f}, {&m->shapeY[1], 130.0f},
        {&m->shapeX[2], -205.0f}, {&m->shapeY[2], 70.0f},  {&m->shapeX[3], -155.0f}, {&m->shapeY[3], 70.0f},
        {&m->shapeX[4], -255.0f}, {&m->shapeY[4], 70.0f},  {&m->shapeX[5], -105.0f}, {&m->shapeY[5], 70.0f},
        {&m->shapeX[6], -180.0f}, {&m->shapeY[6], 130.0f}, {&m->shapeX[7], -80.0f},  {&m->shapeY[7], 130.0f},
        {&m->shapeX[8], -230.0f}, {&m->shapeY[8], 130.0f},
    };
    for (const Place& p : places) {
        int r = Rand();
        float jitter = float(double(r) / 32767.0) * 16.0f - 8.0f;
        *p.field = jitter + p.base;
    }
}

// 0x6F8050: a question's answers: their count and the right one; the columns they sit in
// (3 to 6 answers; other counts keep the old places)
void __fastcall MathSetAnswers(ClassMath* m, re::Edx, int count, int correct) {
    m->answeredTime = 0.0f;
    m->answerCount = count;
    m->correctAnswer = correct;
    m->cursorAnswer = -1;
    m->answered = 0;
    std::memset(m->answerState, 0, sizeof(m->answerState));
    if (language == 7) {
        mathColumn1 = 95.0f;
        mathColumn2 = 200.0f;
        mathColumn3 = 115.0f;
    }
    switch (m->answerCount) {
    case 3: {
        const float y[3] = {-120.0f, -40.0f, 40.0f};
        const float x[3] = {mathColumn1, mathColumn2, mathColumn3};
        for (int i = 0; i < 3; ++i) {
            m->answerX[i] = x[i];
            m->answerY[i] = y[i];
        }
        break;
    }
    case 4: {
        const float y[4] = {-140.0f, -70.0f, 0.0f, 70.0f};
        for (int i = 0; i < 4; ++i) {
            m->answerX[i] = (i & 1) == 0 ? mathColumn1 : mathColumn2;
            m->answerY[i] = y[i];
        }
        break;
    }
    case 5: {
        const float y[5] = {-150.0f, -80.0f, -30.0f, 30.0f, 90.0f};
        for (int i = 0; i < 5; ++i) {
            m->answerX[i] = (i & 1) == 0 ? mathColumn1 : mathColumn2;
            m->answerY[i] = y[i];
        }
        break;
    }
    case 6: {
        const float y[6] = {-150.0f, -100.0f, -50.0f, 0.0f, 50.0f, 100.0f};
        for (int i = 0; i < 6; ++i) {
            m->answerX[i] = (i & 1) == 0 ? mathColumn1 : mathColumn2;
            m->answerY[i] = y[i];
        }
        break;
    }
    default:
        break;
    }
}

// the question's title: the type's (33 characters)
void MathCopyTitle(ClassMath* m) {
    for (int j = 0; j < 0x21; ++j) {
        m->question[j] = m->titles[m->questionType][j];
    }
}
void MathWidenAnswers(ClassMath* m, const re::Ptr32<const char>* answers) {
    for (int i = 0; i < m->answerCount; ++i) {
        AsciiToWide(answers[i], m->answerText[i]);
    }
}
void MathCopyValues(ClassMath* m, const int* answers) {
    for (int i = 0; i < m->answerCount; ++i) {
        m->answerValue[i] = answers[i];
    }
}

// 0x6F8280: ClassMathSetEquation's: the script's question and answers
void __fastcall MathSetTextQuestion(ClassMath* m, re::Edx, const char* text, int count, int correct,
                                    const re::Ptr32<const char>* answers) {
    m->textQuestion = 1;
    m->questionType = -1;
    Math_SetAnswers(m, count, correct);
    AsciiToWide(text, m->question);
    MathWidenAnswers(m, answers);
}

// 0x6F8300 (highest, type 0), 0x6F83D0 (lowest, 1): text answers under a title
template <int Type>
void __fastcall MathSetTextAnswers(ClassMath* m, re::Edx, int count, int correct, const re::Ptr32<const char>* answers) {
    m->textQuestion = 0;
    m->questionType = Type;
    Math_SetAnswers(m, count, correct);
    MathCopyTitle(m);
    MathWidenAnswers(m, answers);
}

// 0x6F84A0: ClassMathSetDifference's (type 8): the picture `right` for the right answer,
// `other` for the rest
void __fastcall MathSetDifference(ClassMath* m, re::Edx, int count, int correct, int other, int right) {
    m->textQuestion = 0;
    m->questionType = 8;
    Math_SetAnswers(m, count, correct);
    MathCopyTitle(m);
    for (int i = 0; i < m->answerCount; ++i) {
        m->answerValue[i] = i == m->correctAnswer ? right : other;
    }
}

// 0x6F8570: ClassMathSetTriangles's (type 9): the picture, text answers
void __fastcall MathSetTriangles(ClassMath* m, re::Edx, int shape, int count, int correct, const re::Ptr32<const char>* answers) {
    m->textQuestion = 0;
    m->questionType = 9;
    Math_SetAnswers(m, count, correct);
    MathCopyTitle(m);
    m->shapeTexture = shape;
    MathWidenAnswers(m, answers);
}

// 0x6F8650 (tallest, 4), 0x6F8710 (shortest, 5), 0x6F87D0 (fastest, 2), 0x6F8890 (slowest, 3),
// 0x6F8950 (fattest, 6), 0x6F8A10 (thinnest, 7): picture answers
template <int Type>
void __fastcall MathSetPictureAnswers(ClassMath* m, re::Edx, int count, int correct, const int* answers) {
    m->textQuestion = 0;
    m->questionType = Type;
    Math_SetAnswers(m, count, correct);
    MathCopyTitle(m);
    MathCopyValues(m, answers);
}

// 0x6F8AD0 (squares: type 11, texture 3), 0x6F8BC0 (circles: type 10, texture 2): count
// the shapes (placed with jitter), text answers
template <int Type, int Texture>
void __fastcall MathSetCountShapes(ClassMath* m, re::Edx, int shapes, int count, int correct, const re::Ptr32<const char>* answers) {
    m->textQuestion = 0;
    m->questionType = Type;
    m->shapeCount = shapes;
    m->shapeTexture = Texture;
    Math_SetAnswers(m, count, correct);
    Math_Jitter(m);
    MathCopyTitle(m);
    MathWidenAnswers(m, answers);
}

// 0x6F8CB0, 0x6F8CD0, 0x6F8CF0: an answer given / right / wrong since the last ask (cleared)
template <size_t Offset>
uint8_t __fastcall MathTakeFlag(ClassMath* m, re::Edx) {
    uint8_t& flag = At<uint8_t>(m, int(Offset));
    uint8_t was = flag;
    flag = 0;
    return was == 1 ? 1 : 0;
}

// 0x6F8D10: ClassMathSetNumQuestions's
void __fastcall MathSetQuestionCount(ClassMath* m, re::Edx, int count) {
    m->questionCount = count;
}

// 0x6F8D20
int __fastcall MathGetScore(ClassMath* m, re::Edx) {
    return m->score;
}

// 0x6F8D30: the share answered right, percent
int __fastcall MathGetScorePercentage(ClassMath* m, re::Edx) {
    return int(float(m->score) / float(m->questionCount) * 100.0f);
}

// 0x6F8D50: ClassMathSetTimer's
void __fastcall MathSetTimer(ClassMath* m, re::Edx, float time, float second) {
    m->timeLeft = time;
    m->unknown388 = second;
}

// 0x6F8D70
ClassMath* __fastcall MathConstruct(ClassMath* m, re::Edx) {
    Minigame_Construct(m);
    m->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00947A70);
    m->triggerFunction = 0;
    m->timeLeft = 10.0f;
    m->unknown38 = 1;
    m->unknown388 = 0.0f;
    m->questionCount = 1;
    m->unknown3C = 2;
    m->level = 0;
    return m;
}

// 0x6F9C60 (virtual +0): the scalar deleting destructor (the end inlined)
ClassMath* __fastcall MathDestruct(ClassMath* m, re::Edx, int flags) {
    m->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00947A70);
    m->active = 0;
    m->hud = 0;
    Minigame_EndTextures(m);
    FontStyle_Unload(3);
    Minigame_EndSounds(m);
    radarHidden = 1;
    Camera_RestoreFov(camera);
    ScriptRef_Clear(&m->triggerFunction);
    Minigame_Destruct(m);
    if ((flags & 1) != 0) {
        Free(m);
    }
    return m;
}

// 0x6FA280: ClassEnglishSetScoreMsg's and ClassMathSetScoreMsg's (count +0xC8, ten at most)
void __fastcall ClassSetScoreMsg(ClassMath* m, re::Edx, float score, const char* key) {
    if (m->scoreMsgCount >= 10) {
        return;
    }
    m->scoreMsgs[m->scoreMsgCount].score = score;
    std::strcpy(m->scoreMsgs[m->scoreMsgCount].key, key);
    m->scoreMsgCount += 1;
}

// ---- the script natives ----------------------------------------------------------------------

inline ClassMath* CurrentMath() {
    return re::PtrCast<ClassMath*>(currentMinigame);
}

// 0x6FA360: ClassMathSetTimer(time [, second])
int __cdecl Native_ClassMathSetTimer(lua_State* L) {
    Script::GetArgCount(L);
    float time = Script::GetFloat(L, 0);
    float second = 0.0f;
    if (Script::GetArgCount(L) >= 2) {
        second = Script::GetFloat(L, 1);
    }
    Math_SetTimer(CurrentMath(), time, second);
    return 0;
}

// (the game's answer lists are five-entry stack arrays: more answers overrun them)
constexpr int kMaxAnswers = 16;

// 0x6FA550: ClassMathSetEquation(text, count, right, answers...)
int __cdecl Native_ClassMathSetEquation(lua_State* L) {
    int count = Script::GetInt(L, 1);
    const char* text = Script::GetString(L, 0);
    int correct = Script::GetInt(L, 2) - 1;
    re::Ptr32<const char> answers[kMaxAnswers];
    for (int i = 0; i < count; ++i) {
        answers[i] = Script::GetString(L, i + 3);
    }
    Math_SetTextQuestion(CurrentMath(), text, count, correct, answers);
    return 0;
}

// 0x6FA5C0 (highest), 0x6FA630 (lowest): (count, right, answers...)
template <uintptr_t Setter>
int __cdecl Native_MathTextAnswers(lua_State* L) {
    int count = Script::GetInt(L, 0);
    int correct = Script::GetInt(L, 1) - 1;
    const char* answers[kMaxAnswers];
    for (int i = 0; i < count; ++i) {
        answers[i] = Script::GetString(L, i + 2);
    }
    using Fn = void(__thiscall*)(ClassMath*, int, int, const char* const*);
    reinterpret_cast<Fn>(Setter)(CurrentMath(), count, correct, answers);
    return 0;
}

// 0x6FA6A0: ClassMathSetDifference(count, right, other, rightPicture)
int __cdecl Native_ClassMathSetDifference(lua_State* L) {
    int count = Script::GetInt(L, 0);
    int correct = Script::GetInt(L, 1) - 1;
    int other = Script::GetInt(L, 2);
    int right = Script::GetInt(L, 3);
    Math_SetDifference(CurrentMath(), count, correct, other, right);
    return 0;
}

// 0x6FA6F0 (triangles), 0x6FAA00 (squares), 0x6FAA70 (circles): (shape, count, right, answers...)
template <uintptr_t Setter>
int __cdecl Native_MathShapeAnswers(lua_State* L) {
    int shape = Script::GetInt(L, 0);
    int count = Script::GetInt(L, 1);
    int correct = Script::GetInt(L, 2) - 1;
    const char* answers[kMaxAnswers];
    for (int i = 0; i < count; ++i) {
        answers[i] = Script::GetString(L, i + 3);
    }
    using Fn = void(__thiscall*)(ClassMath*, int, int, int, const char* const*);
    reinterpret_cast<Fn>(Setter)(CurrentMath(), shape, count, correct, answers);
    return 0;
}

// 0x6FA760 (tallest), 0x6FA7D0 (shortest), 0x6FA840 (fastest), 0x6FA8B0 (slowest), 0x6FA920
// (fattest), 0x6FA990 (thinnest): (count, right, pictures...)
template <uintptr_t Setter>
int __cdecl Native_MathPictureAnswers(lua_State* L) {
    int count = Script::GetInt(L, 0);
    int correct = Script::GetInt(L, 1) - 1;
    int answers[kMaxAnswers];
    for (int i = 0; i < count; ++i) {
        answers[i] = Script::GetInt(L, i + 2);
    }
    using Fn = void(__thiscall*)(ClassMath*, int, int, const int*);
    reinterpret_cast<Fn>(Setter)(CurrentMath(), count, correct, answers);
    return 0;
}

} // namespace

#define CMATH43_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CMATH43_REVERSED("ClassMath::Start_6F7420", 0x006F7420, &MathStart);
CMATH43_REVERSED("ClassMath::End_6F7C70", 0x006F7C70, &MathEnd);
CMATH43_REVERSED("ClassMath::Jitter_6F7CB0", 0x006F7CB0, &MathJitter);
// callers rely on ecx surviving it (0x6F84A0, 0x6F8650, 0x6F8710, 0x6F87D0, 0x6F8890, 0x6F8950,
// 0x6F8A10; 0x6F8AD0 / 0x6F8BC0 then call 0x6F7CB0 with it): turn on together with them
CMATH43_REVERSED("ClassMath::SetAnswers_6F8050", 0x006F8050, &MathSetAnswers);
CMATH43_REVERSED("ClassMath::SetTextQuestion_6F8280", 0x006F8280, &MathSetTextQuestion);
CMATH43_REVERSED("ClassMath::SetHighest_6F8300", 0x006F8300, &MathSetTextAnswers<0>);
CMATH43_REVERSED("ClassMath::SetLowest_6F83D0", 0x006F83D0, &MathSetTextAnswers<1>);
CMATH43_REVERSED("ClassMath::SetDifference_6F84A0", 0x006F84A0, &MathSetDifference);
CMATH43_REVERSED("ClassMath::SetTriangles_6F8570", 0x006F8570, &MathSetTriangles);
CMATH43_REVERSED("ClassMath::SetTallest_6F8650", 0x006F8650, &MathSetPictureAnswers<4>);
CMATH43_REVERSED("ClassMath::SetShortest_6F8710", 0x006F8710, &MathSetPictureAnswers<5>);
CMATH43_REVERSED("ClassMath::SetFastest_6F87D0", 0x006F87D0, &MathSetPictureAnswers<2>);
CMATH43_REVERSED("ClassMath::SetSlowest_6F8890", 0x006F8890, &MathSetPictureAnswers<3>);
CMATH43_REVERSED("ClassMath::SetFattest_6F8950", 0x006F8950, &MathSetPictureAnswers<6>);
CMATH43_REVERSED("ClassMath::SetThinnest_6F8A10", 0x006F8A10, &MathSetPictureAnswers<7>);
CMATH43_REVERSED("ClassMath::SetSquares_6F8AD0", 0x006F8AD0, (&MathSetCountShapes<11, 3>));
CMATH43_REVERSED("ClassMath::SetCircles_6F8BC0", 0x006F8BC0, (&MathSetCountShapes<10, 2>));
CMATH43_REVERSED("ClassMath::TakeAnswerGiven_6F8CB0", 0x006F8CB0, &MathTakeFlag<0xAAC>);
CMATH43_REVERSED("ClassMath::TakeValidAnswer_6F8CD0", 0x006F8CD0, &MathTakeFlag<0xAAD>);
CMATH43_REVERSED("ClassMath::TakeInvalidAnswer_6F8CF0", 0x006F8CF0, &MathTakeFlag<0xAAE>);
CMATH43_REVERSED("ClassMath::SetQuestionCount_6F8D10", 0x006F8D10, &MathSetQuestionCount);
CMATH43_REVERSED("ClassMath::GetScore_6F8D20", 0x006F8D20, &MathGetScore);
CMATH43_REVERSED("ClassMath::GetScorePercentage_6F8D30", 0x006F8D30, &MathGetScorePercentage);
CMATH43_REVERSED("ClassMath::SetTimer_6F8D50", 0x006F8D50, &MathSetTimer);
CMATH43_REVERSED("ClassMath::ClassMath_6F8D70", 0x006F8D70, &MathConstruct);
CMATH43_REVERSED("ClassMath::~ClassMath_6F9C60", 0x006F9C60, &MathDestruct);
CMATH43_REVERSED("Minigame::SetScoreMsg_6FA280", 0x006FA280, &ClassSetScoreMsg);
CMATH43_REVERSED("Native_ClassMathSetTimer", 0x006FA360, &Native_ClassMathSetTimer);
CMATH43_REVERSED("Native_ClassMathSetEquation", 0x006FA550, &Native_ClassMathSetEquation);
CMATH43_REVERSED("Native_ClassMathSetHighest", 0x006FA5C0, &Native_MathTextAnswers<0x006F8300>);
CMATH43_REVERSED("Native_ClassMathSetLowest", 0x006FA630, &Native_MathTextAnswers<0x006F83D0>);
CMATH43_REVERSED("Native_ClassMathSetDifference", 0x006FA6A0, &Native_ClassMathSetDifference);
CMATH43_REVERSED("Native_ClassMathSetTriangles", 0x006FA6F0, &Native_MathShapeAnswers<0x006F8570>);
CMATH43_REVERSED("Native_ClassMathSetTallest", 0x006FA760, &Native_MathPictureAnswers<0x006F8650>);
CMATH43_REVERSED("Native_ClassMathSetShortest", 0x006FA7D0, &Native_MathPictureAnswers<0x006F8710>);
CMATH43_REVERSED("Native_ClassMathSetFastest", 0x006FA840, &Native_MathPictureAnswers<0x006F87D0>);
CMATH43_REVERSED("Native_ClassMathSetSlowest", 0x006FA8B0, &Native_MathPictureAnswers<0x006F8890>);
CMATH43_REVERSED("Native_ClassMathSetFattest", 0x006FA920, &Native_MathPictureAnswers<0x006F8950>);
CMATH43_REVERSED("Native_ClassMathSetThinnest", 0x006FA990, &Native_MathPictureAnswers<0x006F8A10>);
CMATH43_REVERSED("Native_ClassMathSetSquares", 0x006FAA00, &Native_MathShapeAnswers<0x006F8AD0>);
CMATH43_REVERSED("Native_ClassMathSetCircles", 0x006FAA70, &Native_MathShapeAnswers<0x006F8BC0>);
