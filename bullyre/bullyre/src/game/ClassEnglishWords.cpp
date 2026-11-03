// English class (ClassEnglish): the word game's letter tiles and word list
// (0x6EB880..0x6EC9C0): the set-up, the tiles moved between the row and the word being
// built, the word submitted against the lists, the words and letters drawn. Its update
// and draw virtuals are ClassEnglishPlay.cpp's, its script natives NativesC2.cpp's.
//
// The letters (the longest word of the list) are shuffled onto tiles in a row; picking
// one moves it up into the word, delete returns the last; a submitted word is checked
// against the bad words (a time penalty), the short words (rejected) and the list (a
// time bonus the first time each is found).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/SchoolClasses.h"
#include "game/ScriptApi.h"
#include "game/TrackMath.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using namespace school;

// a letter tile: drawn at (x, y) easing to its target; its home in the row
struct EnglishTile {
    float x;                  // +0x0
    float y;                  // +0x4
    float targetX;            // +0x8
    float targetY;            // +0xC
    float homeX;              // +0x10
    float homeY;              // +0x14
    uint16_t letter;          // +0x18
    uint8_t available;        // +0x1A in the row (not in the word)
    uint8_t unknown1B;
};
static_assert(sizeof(EnglishTile) == 0x1C);

struct ClassEnglish {
    re::Ptr32<re::Ptr32<void>> vtable;                     // +0x0
    uint8_t active;                    // +0x4
    uint8_t hud;                       // +0x5
    uint8_t success;                   // +0x6
    uint8_t unknown7[0x15];
    re::Ptr32<re::Ptr32<void>> textures;                   // +0x1C
    uint8_t unknown20[0x18];
    int32_t unknown38;                 // +0x38 (1)
    int32_t unknown3C;                 // +0x3C (1)
    uint8_t unknown40[0xC];
    re::Ptr32<uint32_t> sounds;                  // +0x4C
    int32_t popupKind;                 // +0x50
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;              // +0xA0
    uint8_t unknownA1[0x23];
    int32_t level;                     // +0xC4 (0..4: the word lists MGCE_T<level+1>...)
    int32_t scoreMsgCount;             // +0xC8
    uint8_t scoreMsgs[10][0x44];       // +0xCC
    uint8_t warned;                    // +0x374
    uint8_t unknown375;
    uint16_t letters[9];               // +0x376 the longest word's letters
    EnglishTile tiles[9];              // +0x388
    int32_t chosen[9];                 // +0x484 the word being built: tile indices
    int32_t cursor;                    // +0x4A8 the tile under the cursor
    int32_t tileCount;                 // +0x4AC
    int32_t chosenCount;               // +0x4B0
    float warnTime;                    // +0x4B4
    float triggerTime;                 // +0x4B8
    uint32_t triggerFunction;          // +0x4BC
    float penaltyTime;                 // +0x4C0 a bad word's
    float bonusTime;                   // +0x4C4 a new word's
    float timeLeft;                    // +0x4C8
    int32_t unknown4CC;                // +0x4CC (2)
    int32_t unknown4D0;                // +0x4D0 (3)
    int32_t minScore;                  // +0x4D4 (ClassEnglishSetMinScore)
    uint8_t minScoreReached;           // +0x4D8
    uint8_t wordValid;                 // +0x4D9 the last submission's result
    uint8_t wordNotValid;              // +0x4DA
    uint8_t wordDuplicate;             // +0x4DB
    uint8_t wordNaughty;               // +0x4DC
    uint8_t wordTooShort;              // +0x4DD
    char lastWord[10];                 // +0x4DE
    float minPercentage;               // +0x4E8
    int32_t padRepeat;                 // +0x4EC
    uint8_t entering;                  // +0x4F0
    uint8_t unknown4F1;
    uint16_t words[70][9];             // +0x4F2 the list
    uint8_t found[70];                 // +0x9DE
    int32_t wordCount;                 // +0xA24
    uint16_t badWords[15][9];          // +0xA28
    uint8_t unknownB36[2];
    int32_t badWordCount;              // +0xB38
    uint16_t shortWords[30][9];        // +0xB3C
    int32_t shortWordCount;            // +0xD58
    float pulseStart;                  // +0xD5C seconds: the cursor's pulse
};
static_assert(offsetof(ClassEnglish, level) == 0xC4);
static_assert(offsetof(ClassEnglish, warned) == 0x374);
static_assert(offsetof(ClassEnglish, letters) == 0x376);
static_assert(offsetof(ClassEnglish, tiles) == 0x388);
static_assert(offsetof(ClassEnglish, chosen) == 0x484);
static_assert(offsetof(ClassEnglish, cursor) == 0x4A8);
static_assert(offsetof(ClassEnglish, timeLeft) == 0x4C8);
static_assert(offsetof(ClassEnglish, lastWord) == 0x4DE);
static_assert(offsetof(ClassEnglish, minPercentage) == 0x4E8);
static_assert(offsetof(ClassEnglish, words) == 0x4F2);
static_assert(offsetof(ClassEnglish, found) == 0x9DE);
static_assert(offsetof(ClassEnglish, wordCount) == 0xA24);
static_assert(offsetof(ClassEnglish, badWordCount) == 0xB38);
static_assert(offsetof(ClassEnglish, shortWords) == 0xB3C);
static_assert(offsetof(ClassEnglish, shortWordCount) == 0xD58);
static_assert(sizeof(ClassEnglish) == 0xD60);

// the English sounds (+0x4C)
enum EnglishWordSound : int {
    ENGW_SOUND_VALID = 3, ENGW_SOUND_INVALID = 4,
};

GAME_FN(void, __cdecl, Eng_AsciiToWide, 0x00690090, const char* src, uint16_t* dst);
GAME_FN(void, __cdecl, Eng_FontStyleLoad, 0x00687BC0, int style);
GAME_FN(void, __cdecl, Eng_FontStyleUnload, 0x00687D10, int style);
GAME_FN(void, __cdecl, Eng_PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, Eng_PopMemoryTag, 0x0052E710);
GAME_FN(int, __cdecl, Eng_ParseTrailingNumber, 0x0043A5C0, const uint16_t* s);
template <typename... A>
inline int __cdecl Eng_Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }

// the class's own (through the game's addresses)
GAME_FN(void, __thiscall, Eng_ReturnAll, 0x006EBA00, ClassEnglish* self);
GAME_FN(void, __thiscall, Eng_Scramble, 0x006EBA60, ClassEnglish* self);
GAME_FN(void, __thiscall, Eng_NextTile, 0x006EB930, ClassEnglish* self);
GAME_FN(void, __thiscall, Eng_Reset, 0x006EC5D0, ClassEnglish* self);
GAME_FN(uint8_t*, __thiscall, Eng_PulseColour, 0x006EC3A0, ClassEnglish* self, uint8_t* out, const uint8_t* a,
        const uint8_t* b);

inline void* const kEnglishVtable = reinterpret_cast<void*>(0x00945510);

// the game's clock in seconds (a uint32 of ms, as float)
inline float ClockSeconds() {
    return float(double(timeInMs) * 0.0010000000474974513);
}

// ---- the tiles -------------------------------------------------------------------------------

// 0x6EB880: the constructor
ClassEnglish* __fastcall EnglishConstruct(ClassEnglish* e, re::Edx) {
    Minigame_Construct(e);
    e->vtable = reinterpret_cast<re::Ptr32<void>*>(kEnglishVtable);
    e->triggerFunction = 0;
    e->unknown38 = 1;
    e->unknown3C = 1;
    e->level = 0;
    return e;
}

// the textures, the font style 3 (but in language 6) and the sounds released
inline void EnglishRelease(ClassEnglish* e) {
    e->active = 0;
    e->hud = 0;
    Minigame_EndTextures(e);
    if (language != 6) {
        Eng_FontStyleUnload(3);
    }
}

// 0x6EB8B0 (virtual +0x14): the game ends
void __fastcall EnglishEnd(ClassEnglish* e, re::Edx) {
    EnglishRelease(e);
    Minigame_EndSounds(e);
}

// the cursor stepped (by `step`, wrapping) to the next tile still in the row (or back where
// it started); with the row empty stepping forward, the word is entered
template <int Step>
void StepCursor(ClassEnglish* e) {
    if (e->entering != 0) {
        return;
    }
    int start = e->cursor;
    int at;
    do {
        at = e->cursor + Step;
        e->cursor = at;
        if (Step < 0) {
            if (at < 0) {
                e->cursor = e->tileCount - 1;
            }
        } else if (at >= e->tileCount) {
            e->cursor = 0;
        }
        at = e->cursor;
    } while (e->tiles[at].available == 0 && at != start);
    if (Step > 0 && at == start) {
        e->entering = 1;
    }
}

// 0x6EB8E0: the cursor to the tile before
void __fastcall EnglishPreviousTile(ClassEnglish* e, re::Edx) {
    StepCursor<-1>(e);
}

// 0x6EB930: the cursor to the tile after (the word entered once the row is empty)
void __fastcall EnglishNextTile(ClassEnglish* e, re::Edx) {
    StepCursor<1>(e);
}

// the word's last tile back to its home in the row
inline void ReturnLast(ClassEnglish* e) {
    int count = e->chosenCount - 1;
    e->chosenCount = count;
    EnglishTile& tile = e->tiles[e->chosen[count]];
    tile.targetX = tile.homeX;
    tile.available = 1;
    tile.targetY = tile.homeY;
}

// 0x6EB9A0: delete: the word's last letter back
void __fastcall EnglishDelete(ClassEnglish* e, re::Edx) {
    if (e->chosenCount != 0) {
        ReturnLast(e);
    }
}

// 0x6EBA00: the word's letters all back
void __fastcall EnglishReturnAll(ClassEnglish* e, re::Edx) {
    while (e->chosenCount != 0) {
        ReturnLast(e);
    }
}

// 0x6EBA60: the letters back, then shuffled over the row (each tile taking a random one's
// letter and drawn position, its home the slot's). The shuffle's order array is 8 long
// in the game's frame, the tiles' copy after it: a ninth tile's index lands on the copy's
// first x (kept here); on the last pass the pick's range is -1 (a rand() of 32767 picks
// index -1: the float before the array).
void __fastcall EnglishScramble(ClassEnglish* e, re::Edx) {
    Eng_ReturnAll(e);
    int32_t frame[1 + 8 + 9 * 7];   // the slot before, the order, the tiles' copy
    frame[0] = 0;
    int32_t* order = frame + 1;
    EnglishTile* copy = reinterpret_cast<EnglishTile*>(frame + 9);
    int count = e->tileCount;
    if (count > 0) {
        std::memcpy(copy, e->tiles, count * sizeof(EnglishTile));
        for (int i = 0; i < count; ++i) {
            order[i] = i;
        }
    }
    int last = count - 1;
    int range = last - 1;
    for (int k = 0; k < e->tileCount; ++k) {
        int pick = (Rand() * range) / 0x7FFF;
        const EnglishTile& from = copy[order[pick]];
        EnglishTile& tile = e->tiles[k];
        tile.letter = from.letter;
        tile.x = from.x;
        tile.y = from.y;
        if (pick < last) {
            order[pick] = order[last];
        }
        tile.available = 1;
        --last;
        --range;
        float x = float(k) * 45.0f + 0.0f;
        std::memcpy(&frame[0], &x, 4);
        tile.targetX = x;
        tile.homeX = x;
        tile.targetY = -24.0f;
        tile.homeY = -24.0f;
    }
}

// two words equal (the game's inline compare)
inline bool SameWord(const uint16_t* a, const uint16_t* b) {
    while (*a != 0) {
        if (*a != *b) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == *b;
}
// the word in a list, or -1
inline int FindWord(const uint16_t* word, const uint16_t (*list)[9], int count) {
    for (int i = 0; i < count; ++i) {
        if (SameWord(word, list[i])) {
            return i;
        }
    }
    return -1;
}

// 0x6EBB80: the word submitted (its text kept for the scripts): a bad word costs the
// penalty time; a short word is refused; a word of the list scores (and the bonus time)
// the first time; anything else is not valid (the row stays entered). The game builds the
// word in an 8-letter buffer: a ninth letter's terminator lands past it.
void __fastcall EnglishSubmit(ClassEnglish* e, re::Edx) {
    uint16_t word[10];
    int n = 0;
    for (; n < e->chosenCount; ++n) {
        uint16_t letter = e->tiles[e->chosen[n]].letter;
        e->lastWord[n] = char(letter);
        word[n] = letter;
    }
    e->lastWord[n] = 0;
    word[n] = 0;
    e->wordValid = 0;
    e->wordNotValid = 0;
    e->wordDuplicate = 0;
    e->wordNaughty = 0;
    e->wordTooShort = 0;
    if (FindWord(word, e->badWords, e->badWordCount) >= 0) {
        PlayClassSound(e, ENGW_SOUND_INVALID);
        e->wordNaughty = 1;
        float left = e->timeLeft - e->penaltyTime;
        e->timeLeft = left;
        if (left <= 0.0f) {
            e->timeLeft = 0.0f;
        }
        Eng_ReturnAll(e);
        e->entering = 0;
        return;
    }
    if (FindWord(word, e->shortWords, e->shortWordCount) >= 0) {
        PlayClassSound(e, ENGW_SOUND_INVALID);
        e->wordTooShort = 1;
        Eng_ReturnAll(e);
        e->entering = 0;
        return;
    }
    int i = FindWord(word, e->words, e->wordCount);
    if (i >= 0) {
        if (e->found[i] != 0) {
            e->wordDuplicate = 1;
        } else {
            PlayClassSound(e, ENGW_SOUND_VALID);
            e->wordValid = 1;
            e->timeLeft = e->bonusTime + e->timeLeft;
        }
        Eng_ReturnAll(e);
        e->found[i] = 1;
        e->entering = 0;
        return;
    }
    PlayClassSound(e, ENGW_SOUND_INVALID);
    Eng_ReturnAll(e);
    e->wordNotValid = 1;
}

// ---- drawing ---------------------------------------------------------------------------------

// a word not found yet: its length in underscores
inline const uint16_t* Hidden(const uint16_t* word, uint16_t* out) {
    char blanks[32];
    int n = WStrLen(word);
    blanks[n] = 0;
    for (int i = n - 1; i >= 0; --i) {
        blanks[i] = '_';
    }
    Eng_AsciiToWide(blanks, out);
    return out;
}
// a column's x moved past a word: its 10 pixels a letter and 15 more
inline float PastWord(float x, const uint16_t* word, float scaleX) {
    float letters = float(uint32_t(WStrLen(word)));
    float moved = x + letters * (10.0f * scaleX);
    return moved + scaleX * 15.0f;
}

// 0x6EBE90: the list (the words not found as blanks, unless `revealAll`): up to 12 in
// one column; more by length (3 to 7 letters), each length's words in its block below the
// last, the 3-letter ones split into three columns, 4 and 5 letters into two (the columns
// moved right by the word's own width, the 3-letter third twice), each block 5 pixels
// further left. (Lengths outside 3..7 index outside the blocks, in the game too.)
void __fastcall EnglishDrawWords(ClassEnglish* e, re::Edx, int revealAll) {
    float scaleX = screenScaleX;
    float pos[2];
    ScreenPosition(pos, -255.0f, -144.0f, 1, 1);
    uint16_t hidden[32];
    if (e->wordCount < 13) {
        for (int i = 0; i < e->wordCount; ++i) {
            const uint16_t* text = e->words[i];
            if (e->found[i] == 0 && !re::Flag(revealAll)) {
                text = Hidden(e->words[i], hidden);
            }
            float y = float(i) * 20.0f + pos[1];
            Font_PrintString(pos[0], y, text, 1);
        }
        return;
    }
    int counts[5] = {0, 0, 0, 0, 0};
    const uint16_t* blocks[5 * 70 + 1];   // each length's words (and its end marker)
    uint8_t blockFound[5 * 70];
    for (int i = 0; i < e->wordCount; ++i) {
        int length = WStrLen(e->words[i]) - 3;
        int slot = length * 70 + counts[length];
        blocks[slot] = e->words[i];
        blockFound[slot] = e->found[i];
        counts[length] += 1;
    }
    float maxY = 0.0f;
    float baseX = pos[0];
    blocks[counts[0]] = nullptr;
    float blockY = pos[1];
    blocks[70 + counts[1]] = nullptr;
    blocks[140 + counts[2]] = nullptr;
    blocks[210 + counts[3]] = nullptr;
    blocks[280 + counts[4]] = nullptr;
    for (int block = 0; block < 5; ++block) {
        int rows;
        float exact;
        if (block == 0) {
            rows = counts[0] / 3;
            exact = float(counts[0]) / 3.0f;
        } else {
            rows = counts[block] >> 1;
            exact = float(counts[block]) * 0.5f;
        }
        if (float(rows) < exact) {
            rows += 1;
        }
        if (counts[block] > 0) {
            float shift = scaleX * -5.0f * float(block);
            const uint16_t* const* words = &blocks[block * 70];
            const uint8_t* wordFound = &blockFound[block * 70];
            for (int j = 0; j < counts[block]; ++j) {
                float x = baseX;
                float y = float(j) * 20.0f + blockY;
                if (j >= rows && block < 3) {
                    if (block == 0 && j >= rows * 2) {
                        x = PastWord(x, blocks[j], scaleX);
                        y = y - float(rows) * 20.0f;
                    }
                    x = PastWord(x, words[j], scaleX);
                    y = y - float(rows) * 20.0f;
                }
                x = x + shift;
                if (maxY < y) {
                    maxY = y;
                }
                const uint16_t* text = words[j];
                if (wordFound[j] == 0 && !re::Flag(revealAll)) {
                    text = Hidden(words[j], hidden);
                }
                Font_PrintString(x, y, text, 1);
            }
        }
        blockY = maxY + 20.0f;
    }
}

// 0x6EC2D0: the letters of the words found
int __fastcall EnglishLettersFound(ClassEnglish* e, re::Edx) {
    int letters = 0;
    for (int i = 0; i < e->wordCount; ++i) {
        if (e->found[i] != 0) {
            letters += WStrLen(e->words[i]);
        }
    }
    return letters;
}

// 0x6EC320: the letters of all the words
int __fastcall EnglishLettersTotal(ClassEnglish* e, re::Edx) {
    int letters = 0;
    for (int i = 0; i < e->wordCount; ++i) {
        letters += WStrLen(e->words[i]);
    }
    return letters;
}

// 0x6EC360: ClassEnglishSetTimer's: the time left and the new word's bonus
void __fastcall EnglishSetTimer(ClassEnglish* e, re::Edx, float time, float bonus) {
    e->timeLeft = time;
    e->bonusTime = bonus;
}

// 0x6EC380: ClassEnglishSetMinScore's
void __fastcall EnglishSetMinScore(ClassEnglish* e, re::Edx, int score) {
    e->minScore = score;
    e->minScoreReached = 0;
}

// 0x6EC3A0: a colour between `a` and `b` by a sine over a second (restarted each second,
// 0.125 of the way while the popup shows)
uint8_t* __fastcall EnglishPulseColour(ClassEnglish* e, re::Edx, uint8_t* out, const uint8_t* a, const uint8_t* b) {
    float now = ClockSeconds();
    float phase = now - e->pulseStart;
    if (phase >= 0.5f) {
        phase = phase - 0.5f;
        float start = now - phase;
        e->pulseStart = start;
        float later = ClockSeconds();
        if (start <= later - 0.5f) {
            e->pulseStart = later;
        }
    } else if (0.0f > phase) {
        phase = 0.0f;
    }
    if (e->popupShowing != 0) {
        phase = 0.125f;
    }
    float twice = phase + phase;
    float angle = twice * 3.1415927f;
    float s = trk::CrtSin(angle);
    int a0 = a[0];
    int a1 = a[1];
    int a2 = a[2];
    int a3 = a[3];
    int alpha = int(s * float(int(b[3]) - a3) + float(a3));
    int blue = int(float(int(b[2]) - a2) * s + float(a2));
    int green = int(float(int(b[1]) - a1) * s + float(a1));
    int red = int(s * float(int(b[0]) - a0) + float(a0));
    RGBA_Set(out, red & 0xFF, green & 0xFF, blue & 0xFF, alpha & 0xFF);
    return out;
}

// 0x6EC5D0: the row dealt again from the letters: the word emptied, the tiles shuffled
// and set out in the row
void __fastcall EnglishReset(ClassEnglish* e, re::Edx) {
    e->cursor = 0;
    e->chosenCount = 0;
    for (int i = 0; i < e->tileCount; ++i) {
        e->tiles[i].letter = e->letters[i];
    }
    Eng_Scramble(e);
    for (int i = 0; i < e->tileCount; ++i) {
        EnglishTile& tile = e->tiles[i];
        tile.available = 1;
        float x = float(i) * 45.0f + 0.0f;
        tile.targetX = x;
        tile.x = x;
        tile.homeX = x;
        tile.targetY = -24.0f;
        tile.y = -24.0f;
        tile.homeY = -24.0f;
    }
}

// 0x6EC690: the tiles' letters, each easing toward its target (0.4 to the power of the
// time step a frame); the one under the cursor (in the row, while choosing) pulsing with
// a pulsing outline
void __fastcall EnglishDrawLetters(ClassEnglish* e, re::Edx) {
    uint16_t text[2];
    text[1] = 0;
    for (int i = 0; i < e->tileCount; ++i) {
        EnglishTile& tile = e->tiles[i];
        float k = trk::CrtPow(0.4f, timeStep);
        tile.x = (tile.targetX - tile.x) * k + tile.x;
        float k2 = trk::CrtPow(0.4f, timeStep);
        text[0] = tile.letter;
        float y = (tile.targetY - tile.y) * k2 + tile.y;
        tile.y = y;
        float pos[2];
        ScreenPosition(pos, tile.x, y, 1, 1);
        if (i == e->cursor && tile.available != 0 && e->entering == 0) {
            uint8_t outline[4];
            SetFontColour(Eng_PulseColour(e, outline, GameColour(0x020C82E0), GameColour(0x020C82CC)));
            Font_PrintString(pos[0] - 2.0f, pos[1], text, 1);
            Font_PrintString(pos[0] + 2.0f, pos[1], text, 1);
            Font_PrintString(pos[0], pos[1] - 2.0f, text, 1);
            Font_PrintString(pos[0], pos[1] + 2.0f, text, 1);
            uint8_t letter[4];
            SetFontColour(Eng_PulseColour(e, letter, GameColour(0x020C82DC), GameColour(0x020C82C4)));
        } else {
            SetFontColour(GameColour(0x020C82C0));
        }
        Font_PrintString(pos[0], pos[1], text, 1);
    }
}

// 0x6EC8F0: a tile picked from the row: up into the word (45 pixels a letter), the cursor
// on to the next
void __fastcall EnglishPickLetter(ClassEnglish* e, re::Edx, int index) {
    EnglishTile& tile = e->tiles[index];
    if (tile.available == 0) {
        return;
    }
    tile.available = 0;
    tile.targetX = float(e->chosenCount) * 45.0f + 0.0f;
    tile.targetY = -94.0f;
    e->chosen[e->chosenCount] = index;
    e->chosenCount += 1;
    Eng_NextTile(e);
}

// 0x6EC960 (virtual +0): the deleting destructor
ClassEnglish* __fastcall EnglishDestroy(ClassEnglish* e, re::Edx, int flags) {
    e->vtable = reinterpret_cast<re::Ptr32<void>*>(kEnglishVtable);
    EnglishRelease(e);
    Minigame_EndSounds(e);
    ScriptRef_Clear(&e->triggerFunction);
    Minigame_Destruct(e);
    if ((flags & 1) != 0) {
        Free(e);
    }
    return e;
}

// 0x6EC9C0 (virtual +0x10): the game starts: three minutes, a warning at 30 seconds, a
// success at 65% of the letters; the bad and short words and the level's list read from
// the text (the letters the list's longest word's), the row dealt; the textures
// ("MG_English", the font style 3 but in language 6), the sounds ("EngClass") and the
// info popup
void __fastcall EnglishStart(ClassEnglish* e, re::Edx) {
    e->timeLeft = 180.0f;
    e->active = 1;
    e->bonusTime = 0.0f;
    e->hud = 0;
    e->unknown4CC = 2;
    e->warnTime = 30.0f;
    e->unknown4D0 = 3;
    e->minScore = 6;
    e->minScoreReached = 0;
    e->triggerTime = 0.0f;
    e->padRepeat = 0;
    e->penaltyTime = 0.0f;
    e->warned = 0;
    e->pulseStart = ClockSeconds();
    char key[32];
    int count = Eng_ParseTrailingNumber(GetText(GameString(0x00945720)));   // "MGCE_BADWORDS"
    e->badWordCount = count;
    for (int i = 0; i < e->badWordCount; ++i) {
        Eng_Sprintf(key, GameString(0x00945710), i + 1);   // "MGCE_BADW%02d"
        Font_Format(e->badWords[i], GetText(key), 8);
    }
    count = Eng_ParseTrailingNumber(GetText(GameString(0x00945700)));   // "MGCE_SHORTWORDS"
    e->shortWordCount = count;
    for (int i = 0; i < e->shortWordCount; ++i) {
        Eng_Sprintf(key, GameString(0x009456F0), i + 1);   // "MGCE_SHORTW%02d"
        Font_Format(e->shortWords[i], GetText(key), 8);
    }
    Eng_Sprintf(key, GameString(0x009456DC), e->level + 1, 1);   // "MGCE_T%dG%02dWORDS"
    e->wordCount = Eng_ParseTrailingNumber(GetText(key));
    for (int i = 0; i < e->wordCount; ++i) {
        Eng_Sprintf(key, GameString(0x009456C8), e->level + 1, 1, i + 1);   // "MGCE_T%dG%02dW%02d"
        Font_Format(e->words[i], GetText(key), 8);
        e->found[i] = 0;
        uint32_t length = uint32_t(WStrLen(e->words[i]));
        if (length > uint32_t(WStrLen(e->letters))) {
            Font_Format(e->letters, e->words[i], 8);
        }
    }
    e->minPercentage = 0.65f;
    e->scoreMsgCount = 0;
    e->tileCount = WStrLen(e->letters);
    Eng_Reset(e);
    e->entering = 0;
    const ClassTextureEntry textures[19] = {
        {0, GameString(0x009456B4), GameString(0x009456A8)},   // enterbg, enterbg_x
        {1, GameString(0x0094569C), nullptr},                  // wrdpuzz00
        {2, GameString(0x00945690), nullptr},
        {3, GameString(0x00945684), nullptr},
        {4, GameString(0x00945678), nullptr},
        {5, GameString(0x0094566C), nullptr},
        {6, GameString(0x00945660), nullptr},
        {7, GameString(0x00945654), nullptr},
        {8, GameString(0x00945648), nullptr},
        {9, GameString(0x0094563C), nullptr},
        {0xA, GameString(0x00945630), nullptr},
        {0xB, GameString(0x00945624), nullptr},
        {0xC, GameString(0x00945618), nullptr},
        {0xD, GameString(0x0094560C), nullptr},
        {0xE, GameString(0x00945600), nullptr},
        {0xF, GameString(0x009455F4), nullptr},
        {0x10, GameString(0x009455E8), nullptr},                // wrdpuzz15
        {0x11, GameString(0x009455D0), nullptr},                // English_SideScreen_left
        {0x12, GameString(0x009455B4), nullptr},                // English_SideScreen_right
    };
    Minigame_LoadTextures(e, 19, textures, GameString(0x009456BC));   // "MG_English"
    Eng_PushTxdMemoryTag();
    if (language != 6) {
        Eng_FontStyleLoad(3);
    }
    Eng_PopMemoryTag();
    const char* chalkLong = GameString(0x0094559C);   // "ChalkLong"
    const ClassSoundEntry soundTable[9] = {
        {0, GameString(0x00945590)},   // ChalkShort
        {1, chalkLong},
        {2, chalkLong},
        {3, GameString(0x00945580)},   // ChalkMedWrite
        {4, GameString(0x00945570)},   // ChalkBoardWipe
        {5, GameString(0x00945560)},   // ChalkSelectDWN
        {6, GameString(0x00945554)},   // ChalkNails
        {7, GameString(0x0094448C)},   // TimeWarning
        {8, GameString(0x00945548)},   // Scramble
    };
    Minigame_LoadSounds(e, 9, soundTable, GameString(0x009455A8));   // "EngClass"
    Popup_Show(&e->popupKind, 2, GameString(0x0094552C), GameString(0x00945538), 7, -1);   // MGCE_INFO
}

} // namespace

// Untested: the game's code runs until each is checked.
#define CLASSENG42_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CLASSENG42_REVERSED("ClassEnglish::ClassEnglish", 0x006EB880, &EnglishConstruct);
CLASSENG42_REVERSED("ClassEnglish::End", 0x006EB8B0, &EnglishEnd);
CLASSENG42_REVERSED("ClassEnglish::PreviousTile", 0x006EB8E0, &EnglishPreviousTile);
CLASSENG42_REVERSED("ClassEnglish::NextTile", 0x006EB930, &EnglishNextTile);
CLASSENG42_REVERSED("ClassEnglish::Delete", 0x006EB9A0, &EnglishDelete);
CLASSENG42_REVERSED("ClassEnglish::ReturnAll", 0x006EBA00, &EnglishReturnAll);
CLASSENG42_REVERSED("ClassEnglish::Scramble", 0x006EBA60, &EnglishScramble);
CLASSENG42_REVERSED("ClassEnglish::Submit", 0x006EBB80, &EnglishSubmit);
CLASSENG42_REVERSED("ClassEnglish::DrawWords", 0x006EBE90, &EnglishDrawWords);
CLASSENG42_REVERSED("ClassEnglish::LettersFound", 0x006EC2D0, &EnglishLettersFound);
CLASSENG42_REVERSED("ClassEnglish::LettersTotal", 0x006EC320, &EnglishLettersTotal);
CLASSENG42_REVERSED("ClassEnglish::SetTimer", 0x006EC360, &EnglishSetTimer);
CLASSENG42_REVERSED("ClassEnglish::SetMinScore", 0x006EC380, &EnglishSetMinScore);
CLASSENG42_REVERSED("ClassEnglish::PulseColour", 0x006EC3A0, &EnglishPulseColour);
CLASSENG42_REVERSED("ClassEnglish::Reset", 0x006EC5D0, &EnglishReset);
CLASSENG42_REVERSED("ClassEnglish::DrawLetters", 0x006EC690, &EnglishDrawLetters);
CLASSENG42_REVERSED("ClassEnglish::PickLetter", 0x006EC8F0, &EnglishPickLetter);
CLASSENG42_REVERSED("ClassEnglish::~ClassEnglish", 0x006EC960, &EnglishDestroy);
CLASSENG42_REVERSED("ClassEnglish::Start", 0x006EC9C0, &EnglishStart);
