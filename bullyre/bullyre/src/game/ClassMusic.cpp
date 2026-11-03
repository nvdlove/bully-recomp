// Music class (ClassMusic, 0x105A8 bytes, vtable 0x947E80): the rhythm game: notes scroll
// to a hit line; each is one of the two buttons (10, 12) or both; a player's song is read
// from "Config/MusicClassData/". Up to two players (a block of 0x8254 bytes each); the
// "3" mode is the 3_01C mission's (its own texts; player 1 shadows player 0). Here: the
// set-up, the song loading, the judging, the HUD and the notes' drawing (0x6FAAE0..
// 0x6FC790; the class's virtuals 0..2 are another pass's).
//
// The x87 runs at 24-bit precision here (the main thread): float, with the game's double
// constants (0.47) kept double.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/SchoolClasses.h"
#include "game/ScriptApi.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using namespace school;

// a note on its way (its state: 0 waiting, 1 not reached, 2 good, 3 perfect, 4 / 5 hit
// (fading), 6 / 7 missed (fading), 8 gone)
struct MusicNote {
    int32_t state;      // +0x0
    int32_t button;     // +0x4 1 the first, 2 the second, 3 both
    float y;            // +0x8 from the start, scrolling
    float alpha;        // +0xC 255 down
    uint8_t unknown10[4];
    uint8_t active;     // +0x14
    uint8_t flash;      // +0x15
    uint8_t unknown16[2];
    float size;         // +0x18 (0.25 - y / 1000, at most 0.45)
};
static_assert(sizeof(MusicNote) == 0x1C);

// a note as the song file gives it
struct MusicNoteInfo {
    int32_t kind;       // +0x0 0 the first button, 1 the second, 2 both
    char name[16];      // +0x4
    float beat;         // +0x14 when it appears (the song time)
};
static_assert(sizeof(MusicNoteInfo) == 0x18);

struct MusicPlayer {
    MusicNote notes[640];          // +0x0
    MusicNoteInfo infos[640];      // +0x4600
    int32_t current;               // +0x8200 the next note
    int32_t count;                 // +0x8204
    int32_t combo;                 // +0x8208 (10 on: the gold notes)
    int32_t perfects;              // +0x820C
    int32_t difficulty;            // +0x8210 the pass table's index
    char feedbackKey[16];          // +0x8214 "MC_GOOD", "MC_PERFECT"
    char bonusKey[16];             // +0x8224 "MC_BONUS"
    int32_t feedbackKind;          // +0x8234 (3, 4: the bonus shown too)
    uint8_t feedbackColour[4];     // +0x8238 (its alpha fades by 5 a frame)
    uint8_t unknown823C[2];
    uint8_t blocked;               // +0x823E no new hold
    uint8_t judged;                // +0x823F a note judged this frame
    int32_t held;                  // +0x8240 the buttons held (1, 2, 3)
    float posX;                    // +0x8244 the player's hit area
    float posY;                    // +0x8248
    float score;                   // +0x824C
    int16_t length;                // +0x8250 the notes in the song, less one
    uint8_t unknown8252[2];
};
static_assert(offsetof(MusicPlayer, infos) == 0x4600);
static_assert(offsetof(MusicPlayer, current) == 0x8200);
static_assert(offsetof(MusicPlayer, feedbackKey) == 0x8214);
static_assert(offsetof(MusicPlayer, held) == 0x8240);
static_assert(offsetof(MusicPlayer, score) == 0x824C);
static_assert(sizeof(MusicPlayer) == 0x8254);

struct ClassMusic {
    re::Ptr32<re::Ptr32<void>> vtable;                 // +0x0
    uint8_t active;                // +0x4
    uint8_t hud;                   // +0x5
    uint8_t success;               // +0x6
    uint8_t unknown7[0x15];
    re::Ptr32<re::Ptr32<void>> textures;               // +0x1C ("MG_Music": see MusicTexture)
    uint8_t unknown20[0x18];
    int32_t unknown38;             // +0x38 (1)
    int32_t unknown3C;             // +0x3C (0x12)
    uint8_t unknown40[0xC];
    re::Ptr32<uint32_t> sounds;              // +0x4C
    int32_t popupKind;             // +0x50
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;          // +0xA0
    uint8_t unknownA1[0x23];
    uint32_t feedbackFunction;     // +0xC4 the script's (ClassMusicFeedbackCallback)
    MusicPlayer players[2];        // +0xC8
    uint8_t unknown10570[2];
    uint8_t running;               // +0x10572
    uint8_t unknown10573;          // +0x10573
    uint8_t unknown10574;          // +0x10574
    uint8_t unknown10575[3];
    float songTime;                // +0x10578
    float speed;                   // +0x1057C
    float unknown10580;            // +0x10580
    int32_t playerMode;            // +0x10584 1, 2 players; 3 the mission's
    int32_t winner;                // +0x10588 0 won, 2 lost
    uint8_t ended;                 // +0x1058C
    uint8_t unknown1058D[3];
    re::Ptr32<void> noteTexture;             // +0x10590
    uint32_t noteWidth;            // +0x10594
    uint32_t noteHeight;           // +0x10598
    uint8_t colour[4];             // +0x1059C
    int16_t unknown105A0;          // +0x105A0
    int16_t flashTime;             // +0x105A2
    float unknown105A4;            // +0x105A4 (26)
};
static_assert(offsetof(ClassMusic, feedbackFunction) == 0xC4);
static_assert(offsetof(ClassMusic, players) == 0xC8);
static_assert(offsetof(ClassMusic, running) == 0x10572);
static_assert(offsetof(ClassMusic, songTime) == 0x10578);
static_assert(offsetof(ClassMusic, playerMode) == 0x10584);
static_assert(offsetof(ClassMusic, noteTexture) == 0x10590);
static_assert(offsetof(ClassMusic, colour) == 0x1059C);
static_assert(sizeof(ClassMusic) == 0x105A8);

// the "MG_Music" textures (+0x1C)
enum MusicTexture : int {
    MUSIC_TEX_ARROW, MUSIC_TEX_FLASH, MUSIC_TEX_GLOW, MUSIC_TEX_STAR, MUSIC_TEX_GOLD_STAR, MUSIC_TEX_GARY,
    MUSIC_TEX_JIMMY,
};

GAME_VAR(float, musicHitLineY, 0x00BC6860);   // (-360)
GAME_VAR(float, musicFadeStep, 0x00BC6864);   // (25.5)
GAME_VAR(float, musicShrink, 0x00BC686C);     // (0.001)
GAME_VAR(int32_t, musicCameraState, 0x00BCBC48);
GAME_VAR(uint8_t, musicPauseFlag, 0x020C330C);
inline void* const musicCameraSettings = reinterpret_cast<void*>(0x00BC67F8);
GAME_FN(void, __thiscall, Camera_PushSettings, 0x00454D10, void* camera, void* settings);
GAME_FN(void, __thiscall, Camera_PopSettings, 0x00454D60, void* camera);
GAME_FN(void, __thiscall, Sounds_PauseAll, 0x00595100, void* sounds);
GAME_FN(int, __cdecl, GetActivePad, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_IsXInput, 0x00738500, int pad);
GAME_FN(uint8_t*, __thiscall, RGBA_Copy, 0x00424450, uint8_t* rgba, const uint8_t* from);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
inline void* __cdecl Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(int, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, File_Seek, 0x0042D300, int file, int offset, int origin);
GAME_FN(uint32_t, __cdecl, File_Tell, 0x0042D430, int file);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, int file, void* buffer, int size);
GAME_FN(int, __cdecl, File_Close, 0x0042D450, int file);
// a song file's note: its kind from the two names (and the beat)
GAME_FN(int, __cdecl, Music_NoteKind, 0x005DADE0, const char* name);

// the class's own, through the game's addresses
GAME_FN(void, __thiscall, Music_AddNote, 0x006FAAE0, ClassMusic* self, int player, int kind, const char* name, float beat);
GAME_FN(void, __thiscall, Music_Finish, 0x006FACD0, ClassMusic* self);
GAME_FN(void, __thiscall, Music_Feedback, 0x006FACF0, ClassMusic* self, int player, int button, const char* name,
        int good, int extra);

const float kUv0[2] = {0.0f, 0.0f};
const float kUv1[2] = {1.0f, 1.0f};

inline float MulD(float value, double factor) {
    return float(double(value) * factor);
}
inline uint32_t ByteSwap(uint32_t v) {
    return (v << 24) | ((v << 8) & 0x00FF0000u) | ((v >> 8) & 0x0000FF00u) | (v >> 24);
}
inline bool MissionMode(const ClassMusic* m) {
    return m->playerMode == 3;
}

// 0x6FAAE0: a note added to a player's song (kind 0, 1, 2: buttons 1, 2, both)
void __fastcall MusicAddNote(ClassMusic* m, re::Edx, int player, int kind, const char* name, float beat) {
    MusicPlayer& p = m->players[player];
    p.infos[p.count].kind = kind;
    std::strcpy(p.infos[p.count].name, name);
    p.infos[p.count].beat = (beat - 2.0f) + 1.0f;
    MusicNote& note = p.notes[p.count];
    note.alpha = 255.0f;
    note.state = 0;
    note.active = 1;
    note.y = 0.0f;
    note.flash = 0;
    if (kind == 0) {
        note.button = 1;
    } else if (kind == 1) {
        note.button = 2;
    } else if (kind == 2) {
        note.button = 3;
    } else {
        note.button = 0;
    }
    p.count += 1;
}

// 0x6FACA0 (virtual +0x14): the game ends
void __fastcall MusicEnd(ClassMusic* m, re::Edx) {
    ScriptRef_Clear(&m->feedbackFunction);
    Minigame_EndSounds(m);
    Camera_PopSettings(camera);
    musicCameraState = -1;
}

// 0x6FACD0: the song over: a success (the winner set before)
void __fastcall MusicFinish(ClassMusic* m, re::Edx) {
    m->active = 0;
    m->hud = 0;
    m->success = 1;
    m->running = 0;
    Camera_PopSettings(camera);
}

// 0x6FACF0: the script's feedback function (player, button, the note's name, good, extra)
void __fastcall MusicFeedback(ClassMusic* m, re::Edx, int player, int button, const char* name, int good, int extra) {
    uint32_t* ref = &m->feedbackFunction;
    if (ScriptRef_IsSet(ref) == 0) {
        return;
    }
    lua_State* state = static_cast<lua_State*>(ScriptRef_GetState(ref));
    Event_Begin(state);
    Lua_GetRef(state, ScriptRef_GetId(ref));
    Script::PushInt(state, player);
    Script::PushInt(state, button);
    Script::PushString(state, name);
    Script::PushBool(state, uint8_t(good));
    Script::PushInt(state, extra);
    Lua_Call(state, 5, 0);
}

// 0x6FAD70: ClassMusicGetWinner's
int __fastcall MusicGetWinner(ClassMusic* m, re::Edx) {
    return m->winner;
}

// 0x6FAD80: a player's score, truncated
int __fastcall MusicGetScore(ClassMusic* m, re::Edx, int player) {
    return int(m->players[player].score);
}

// the pass marks by difficulty
inline bool MusicPassed(const MusicPlayer& p) {
    const float pass[6] = {0.65f, 0.75f, 0.65f, 0.85f, 0.9f, 0.9f};
    float ratio = p.score / float(int(p.length));
    return pass[p.difficulty] <= ratio;
}

// 0x6FADA0: a note judged: the script told, the score (points; -0.25 and -0.5 are misses),
// the next note; at the song's end the winner (the mission mode: player 0's pass mark)
void __fastcall MusicJudge(ClassMusic* m, re::Edx, int player, float points, int hold, int extra) {
    bool good = !(points == -0.25f || points == -0.5f);
    MusicPlayer& p = m->players[player];
    Music_Feedback(m, player, p.notes[p.current].button, p.infos[p.current].name, good ? 1 : 0, extra);
    m->flashTime = 0;
    p.combo = 0;
    if (points == 1.0f) {
        p.perfects += 1;
    }
    float score = points + p.score;
    p.score = score;
    if (0.0f > score) {
        p.score = 0.0f;
    }
    if (uint8_t(hold) == 0) {
        p.notes[p.current].active = 1;
        p.current += 1;
        p.notes[p.current].active = 0;
    }
    if (m->ended != 0 || p.current < p.count - 1) {
        return;
    }
    if (m->playerMode == 3 && player == 0) {
        m->winner = MusicPassed(m->players[0]) ? 0 : 2;
        Music_Finish(m);
        return;
    }
    if (m->playerMode == 1) {
        m->winner = MusicPassed(p) ? 0 : 2;
        m->active = 0;
        m->hud = 0;
        m->success = 1;
        m->running = 0;
        Camera_PopSettings(camera);
    }
}

// 0x6FB020: a note's buttons pressed (kind 0 the first, 1 the second, 2 both; 3 always)
uint8_t __fastcall MusicCheckPress(ClassMusic* m, re::Edx, int kind, int player) {
    uint8_t first = Pad_JustDown(10, player);
    uint8_t second = Pad_JustDown(12, player);
    switch (kind) {
    case 0:
        if (first != 0) {
            m->players[player].held = 1;
            return 1;
        }
        break;
    case 1:
        if (second != 0) {
            m->players[player].held = 2;
            return 1;
        }
        break;
    case 2:
        if (first != 0 && second != 0) {
            m->players[player].held = 3;
            return 1;
        }
        break;
    case 3:
        return 1;
    default:
        break;
    }
    return 0;
}

// 0x6FB0C0: a hold started on the current note's buttons; a hold let go (both buttons
// for a double)
uint8_t __fastcall MusicCheckHold(ClassMusic* m, re::Edx, int player) {
    uint8_t first = Pad_JustDown(10, player);
    uint8_t second = Pad_JustDown(12, player);
    MusicPlayer& p = m->players[player];
    int32_t& held = p.held;
    if (held == 0) {
        int32_t button = p.notes[p.current].button;
        if (button == 1 && first != 0 && p.blocked == 0) {
            held = button;
            return 1;
        }
        if (button == 2 && second != 0 && p.blocked == 0) {
            held = button;
            return 1;
        }
        if (button == 3 && second != 0 && first != 0 && p.blocked == 0) {
            held = button;
            return 1;
        }
        return 0;
    }
    if (held == 1) {
        if (first == 0) {
            held = 0;
        }
    } else if (held == 2) {
        if (second == 0) {
            held = 0;
        }
    } else if (held == 3) {
        if (first == 0 && second == 0) {
            held = 0;
        }
    }
    return 0;
}

// 0x6FB1A0: ClassMusicSetPlayers's
void __fastcall MusicSetPlayerMode(ClassMusic* m, re::Edx, int mode) {
    m->playerMode = mode;
}

// 0x6FB1B0 (virtual +0x10): the game starts: the textures, the camera, the info popup (not
// in the mission mode), the sounds paused
void __fastcall MusicStart(ClassMusic* m, re::Edx) {
    m->hud = 0;
    m->unknown10580 = 0.0f;
    m->running = 0;
    m->active = 1;
    const ClassTextureEntry textures[7] = {
        {0, GameString(0x00947F40), GameString(0x00947F2C)},   // music_arrow_32
        {1, GameString(0x00944618), GameString(0x00944604)},   // Explosion_Flash
        {2, GameString(0x009445B0), GameString(0x009445A8)},   // Glow
        {3, GameString(0x00947F20), nullptr},                  // music_star
        {4, GameString(0x00947F14), GameString(0x00947F08)},   // goldstar
        {5, GameString(0x00947EF8), GameString(0x00947EE4)},   // music_hud_gary
        {6, GameString(0x00947ED4), GameString(0x00947EC0)},   // music_hud_jimmy
    };
    Minigame_LoadTextures(m, 7, textures, GameString(0x00947F50));   // MG_Music
    Camera_PushSettings(camera, musicCameraSettings);
    musicCameraState = 0x23;
    if (m->playerMode == 3) {
        return;
    }
    if (Pad_IsXInput(GetActivePad()) != 0) {
        Popup_Show(&m->popupKind, 2, GameString(0x00947E9C), GameString(0x00947EB4), 7, -1);   // MC_INFO, MC_FOOTER
    } else {
        Popup_Show(&m->popupKind, 2, GameString(0x00947E9C), GameString(0x00947EA4), 10, -1);  // MC_DI_FOOTER
    }
    musicPauseFlag = 1;
    Sounds_PauseAll(sounds);
}

// a player's block by raw address: the mission mode (3) resets a third one, past the object
// (the game's)
inline MusicPlayer& PlayerBlock(ClassMusic* m, int i) {
    return *reinterpret_cast<MusicPlayer*>(reinterpret_cast<uint8_t*>(m) + offsetof(ClassMusic, players) +
                                           size_t(i) * sizeof(MusicPlayer));
}

// 0x6FB2F0: the players reset for a song at a difficulty (1-based); their hit areas
void __fastcall MusicReset(ClassMusic* m, re::Edx, int difficulty) {
    for (int i = 0; i < m->playerMode; ++i) {
        MusicPlayer& p = PlayerBlock(m, i);
        p.score = 0.0f;
        p.current = 0;
        p.difficulty = difficulty - 1;
        std::memset(p.unknown823C, 0, 2);
        p.blocked = 0;
        p.judged = 0;
        p.combo = 0;
        p.feedbackKind = 0;
        std::memset(p.feedbackColour, 0, 4);
        p.perfects = 0;
        if (MissionMode(m)) {
            Sprintf(p.feedbackKey, GameString(0x00947F80));   // 3_01C_PERFECT
            Sprintf(p.bonusKey, GameString(0x00947F74));      // 3_01C_BONUS
        } else {
            Sprintf(p.feedbackKey, GameString(0x00947F68));   // MC_PERFECT
            Sprintf(p.bonusKey, GameString(0x00947F5C));      // MC_BONUS
        }
        float x = widescreen != 0 ? 260.0f : 160.0f;
        float pos[2];
        float* at = ScreenPosition(pos, -x, 120.0f, 1, 1);
        p.posX = at[0];
        p.posY = at[1];
    }
    m->unknown10573 = 0;
    m->songTime = 0.0f;
    m->ended = 0;
    m->unknown10574 = 0;
    m->unknown105A0 = 0;
    m->unknown105A4 = 26.0f;
    m->flashTime = 0;
    m->running = 1;
}

// 0x6FB440: a player's song from "Config/MusicClassData/<file>": a big-endian count, then
// 0x24-byte notes (two space-padded names, a big-endian beat). (Without a size the file is
// left open: the game's.)
void __fastcall MusicLoadSong(ClassMusic* m, re::Edx, int player, const char* file) {
    char path[0x100];
    std::memcpy(path, GameString(0x00947F90), 23);   // "Config/MusicClassData/"
    char name[16] = {};
    char kindName[16] = {};
    Memset(path + 23, 0, 0xE9);
    std::strcat(path, file);
    int handle = File_Open(path, GameString(0x009010BC), 1);   // "rb"
    if (handle == 0) {
        return;
    }
    File_Seek(handle, 0, 2);
    uint32_t size = File_Tell(handle);
    if (size == 0) {
        return;
    }
    File_Seek(handle, 0, 0);
    uint8_t* data = static_cast<uint8_t*>(Malloc(size));
    File_Read(handle, data, int(size));
    File_Close(handle);
    uint32_t raw;
    std::memcpy(&raw, data, 4);
    int count = int(ByteSwap(raw));
    m->players[player].length = int16_t(count - 1);
    if (count >= 0) {
        const uint8_t* record = data + 4;
        for (int n = count + 1; n != 0; --n) {
            for (int k = 0; k < 16; ++k) {
                if (record[k] != ' ') {
                    name[k] = char(record[k]);
                }
            }
            for (int k = 0; k < 16; ++k) {
                if (record[0x10 + k] != ' ') {
                    kindName[k] = char(record[0x10 + k]);
                }
            }
            // (the game copies the beat through the FPU: a signalling NaN pattern would be quieted)
            uint32_t bits;
            std::memcpy(&bits, record + 0x20, 4);
            bits = ByteSwap(bits);
            float beat;
            std::memcpy(&beat, &bits, 4);
            int kind = Music_NoteKind(name);
            Music_AddNote(m, player, kind, kindName, beat);
            record += 0x24;
            std::memset(name, 0, sizeof(name));
            std::memset(kindName, 0, sizeof(kindName));
        }
    }
    Free(data);
}

// a button's icon over the HUD
void MusicDrawButtonIcon(ClassMusic* m, int button, float hudX, float hudY, float across) {
    void* icon = GetButtonTexture(button);
    if (icon == nullptr) {
        return;
    }
    float width;
    if (!(float(TexWidth(icon)) <= 48.0f)) {
        width = 48.0f;
    } else {
        width = float(TexWidth(icon));
    }
    float scaled = float(TexHeight(icon)) * width;
    float height = scaled / float(TexWidth(icon));
    float sx = screenScaleX;
    float iconWidth = sx * width;
    float x = (sx * across + hudX) - iconWidth * 0.5f;
    float y = (hudY + 85.0f) - 0.5f * height;
    DrawRectUV(x, y, x + iconWidth, y + height, icon, m->colour, kUv0, kUv1);
}

// 0x6FB6A0: a player's HUD (Jimmy's, or Gary's for player 1) and the two buttons' icons
void __fastcall MusicDrawHud(ClassMusic* m, re::Edx, int player, float scaleX, float scaleY) {
    void* face = nullptr;
    if (m->textures != nullptr) {
        face = player == 0 ? m->textures[MUSIC_TEX_JIMMY] : m->textures[MUSIC_TEX_GARY];
    }
    m->noteTexture = face;
    if (face != nullptr) {
        m->noteWidth = TexWidth(face);
        m->noteHeight = TexHeight(m->noteTexture);
    }
    std::memcpy(m->colour, GameColour(0x020C8380), 4);
    float offset = widescreen != 0 ? 15.0f : 20.0f;
    int side = player == 0 ? 1 : -1;
    offset = float(side) * offset;
    const MusicPlayer& p = m->players[player];
    float halfWidth = MulD(float(m->noteWidth), 0.47) * scaleX;
    float x1 = (p.posX - halfWidth) - offset;
    float halfHeight = MulD(float(m->noteHeight), 0.47) * scaleY;
    float y1 = (p.posY - halfHeight) + 23.0f;
    float y2 = 23.0f + (halfHeight + p.posY);
    float x2 = (halfWidth + p.posX) - offset;
    DrawRectUV(x1, y1, x2, y2, m->noteTexture, m->colour, kUv0, kUv1);
    MusicDrawButtonIcon(m, 10, x1, y1, 110.0f);
    MusicDrawButtonIcon(m, 12, x1, y1, 170.0f);
}

// 0x6FBAF0: a player's notes (the 16 from the last judged): judged by their distance to the
// hit line (windows of the scroll step: 7 early, 4 good, perfect in between), faded out
// when hit or missed, and drawn on the buttons' sides
void __fastcall MusicDrawNotes(ClassMusic* m, re::Edx, int player, float scaleX, float scaleY) {
    float hitY = musicHitLineY;
    float gap = float(widescreen == 0 ? 0x1E : 0x16);
    MusicPlayer& p = m->players[player];
    p.judged = 0;
    float step = hitY * m->speed * 0.5f;
    int i = p.current;
    if (i >= 1) {
        i -= 1;
    }
    for (; i < p.current + 0x10; ++i) {
        if (m->songTime < p.infos[i].beat) {
            continue;
        }
        MusicNote& note = p.notes[i];
        if (note.state == 8) {
            continue;
        }
        m->noteTexture = Minigame_GetTexture(m, p.combo >= 10 ? MUSIC_TEX_STAR : MUSIC_TEX_ARROW);
        if (m->noteTexture != nullptr) {
            m->noteWidth = TexWidth(m->noteTexture);
            m->noteHeight = TexHeight(m->noteTexture);
        }
        float scale = 1.0f;
        std::memcpy(m->colour, GameColour(0x020C8380), 4);
        if (note.flash != 0) {
            m->flashTime += 1;
            std::memcpy(m->colour, GameColour(0x020C838C), 4);
            if (m->flashTime > 10) {
                note.flash = 0;
                m->flashTime = 0;
            }
        }
        float y = note.y;
        float drop = hitY - y;
        if (note.state != 5 && note.state != 4) {
            float window = step * 2.0f;
            float early = window * 3.5f;
            float late = window * 2.0f;
            if (hitY - early < y) {
                note.state = 1;
                if (i == 0) {
                    p.notes[0].active = 0;
                }
            } else {
                bool good = !(hitY - early < y) && hitY - late < y;
                if (good) {
                    note.state = 2;
                    p.judged = 1;
                    m->unknown105A0 = 0;
                } else if (!(hitY - late < y) && hitY + late <= y) {
                    note.state = 3;
                    if (m->playerMode == 3 && player == 1) {
                        m->players[1].notes[i].state = 5;
                    }
                    p.judged = 1;
                    m->unknown105A0 = 0;
                } else if (!(late + hitY <= y) && hitY + early <= y) {
                    note.state = 2;
                    p.judged = 1;
                    m->unknown105A0 = 0;
                } else if (!(early + hitY <= y)) {
                    note.state = 6;
                }
            }
        }
        switch (note.state) {
        case 4:
        case 5: {
            p.judged = 1;
            m->unknown105A0 = 0;
            drop = 0.0f;
            if (!(1.0f > note.alpha)) {
                scale = 2.0f * note.alpha / 255.0f;
            }
            int32_t kind = p.feedbackKind;
            if (note.state == 4) {
                if (kind == 3 || kind == 4) {
                    m->noteTexture = Minigame_GetTexture(m, MUSIC_TEX_STAR);
                    std::memcpy(m->colour, GameColour(0x020C8388), 4);
                } else {
                    m->noteTexture = Minigame_GetTexture(m, MUSIC_TEX_ARROW);
                    RGBA_Copy(m->colour, GameColour(0x020C8388));
                }
            } else if (kind == 3 || kind == 4) {
                std::memcpy(m->colour, GameColour(0x020C8384), 4);
                m->noteTexture = Minigame_GetTexture(m, MUSIC_TEX_STAR);
            } else {
                m->noteTexture = Minigame_GetTexture(m, MUSIC_TEX_ARROW);
                RGBA_Copy(m->colour, GameColour(0x020C8384));
            }
            m->colour[3] = uint8_t(int(note.alpha));
            float alpha = note.alpha - musicFadeStep;
            note.alpha = alpha;
            if (alpha + alpha <= 255.0f) {
                note.state = 8;
            }
            break;
        }
        case 6:
        case 7: {
            std::memcpy(m->colour, GameColour(0x020C838C), 4);
            m->colour[3] = uint8_t(int(note.alpha));
            float alpha = note.alpha - musicFadeStep;
            note.alpha = alpha;
            if (!(0.0f < alpha)) {
                note.state = 8;
            }
            break;
        }
        default:
            break;
        }
        float size = 0.25f - y * musicShrink;
        note.size = size;
        if (!(size < 0.45f)) {
            note.size = 0.45f;
        }
        if (m->noteTexture != nullptr) {
            m->noteWidth = TexWidth(m->noteTexture);
            m->noteHeight = TexHeight(m->noteTexture);
        }
        if (m->playerMode < 3 || player == 0) {
            if (note.button == 1 || note.button == 3) {
                float halfHeight = float(m->noteHeight) * note.size * scaleY * scale;
                float halfWidth = scale * (float(m->noteWidth) * note.size * scaleX);
                DrawRectUV((p.posX - halfWidth) - gap, drop + (p.posY - halfHeight), (p.posX + halfWidth) - gap,
                           (p.posY + halfHeight) + drop, m->noteTexture, m->colour, kUv0, kUv1);
            }
            if (note.button == 2 || note.button == 3) {
                float halfHeight = float(m->noteHeight) * note.size * scaleY * scale;
                float halfWidth = scale * (float(m->noteWidth) * note.size * scaleX);
                DrawRectUV(gap + (p.posX - halfWidth), drop + (p.posY - halfHeight), (p.posX + halfWidth) + gap,
                           (p.posY + halfHeight) + drop, m->noteTexture, m->colour, kUv0, kUv1);
            }
        }
        note.y = step + note.y;
    }
}

// 0x6FC2B0: a player's score (the share of the song, at most 100%: green from the pass mark)
void __fastcall MusicDrawScore(ClassMusic* m, re::Edx, int player) {
    uint8_t passed[4];
    uint8_t failed[4];
    RGBA_Set(passed, 0x22, 0x5D, 0x42, 0xFF);
    RGBA_Set(failed, 0x73, 0x20, 0x20, 0xFF);
    const float pass[6] = {0.65f, 0.75f, 0.65f, 0.85f, 0.9f, 0.9f};
    const MusicPlayer& p = m->players[player];
    float ratio;
    if (!(p.score <= 0.0f)) {
        ratio = p.score / float(int(p.length));
        if (1.0f <= ratio) {
            ratio = 1.0f;
        }
    } else {
        ratio = 0.0f;
    }
    if (pass[p.difficulty] <= ratio) {
        SetFontColour(passed);
    } else {
        SetFontColour(failed);
    }
    Font_AddNumber(float(int(ratio * 100.0f)));
    uint16_t text[0x40];
    if (MissionMode(m)) {
        Font_Format(text, GetText(GameString(0x00947FD8)), 0x40);   // 3_01C_SCORE
    } else {
        Font_Format(text, GetText(GameString(0x00947FCC)), 0x40);   // MC_SCORE
    }
    float pos[2];
    ScreenPosition(pos, -20.0f, -199.0f, 1, 1);
    Font_PrintString(pos[0], pos[1], text, 1);
}

// 0x6FC460: a judged note's feedback (1, 3 good; 2, 4 perfect) and the bonus text
void __fastcall MusicSetFeedback(ClassMusic* m, re::Edx, int player, int kind, int bonus) {
    MusicPlayer& p = m->players[player];
    switch (kind) {
    case 1:
    case 3:
        std::memcpy(p.feedbackColour, GameColour(0x020C8388), 4);
        Sprintf(p.feedbackKey, MissionMode(m) ? GameString(0x00947FEC) : GameString(0x00947FE4));   // GOOD
        break;
    case 2:
    case 4:
        std::memcpy(p.feedbackColour, GameColour(0x020C8384), 4);
        Sprintf(p.feedbackKey, MissionMode(m) ? GameString(0x00947F80) : GameString(0x00947F68));   // PERFECT
        break;
    default:
        break;
    }
    if (uint8_t(bonus) != 0) {
        Sprintf(p.bonusKey, MissionMode(m) ? GameString(0x00947F74) : GameString(0x00947F5C));   // BONUS
    }
}

// 0x6FC5A0: the feedback text by the player's hit area, fading (and the bonus line)
void __fastcall MusicDrawFeedback(ClassMusic* m, re::Edx, int player) {
    (void)m;
    Font_SetJustify(2);
    if (language == 7) {
        Font_SetScale(0.6f);
    } else {
        Font_SetSizePreset(3);
    }
    MusicPlayer& p = m->players[player];
    SetFontColour(p.feedbackColour);
    fontShadow = 2;
    uint8_t black[4];
    SetFontShadowColour(RGBA_Set(black, 0, 0, 0, 0xFF));
    float across = widescreen != 0 ? 100.0f : 110.0f;
    int alpha = p.feedbackColour[3];
    if (!(float(alpha) > 5.0f)) {
        return;
    }
    p.feedbackColour[3] = uint8_t(int(float(alpha) - 5.0f));
    int side = player == 0 ? 1 : -1;
    float offset = float(side) * across;
    const uint16_t* text = GetText(p.feedbackKey);
    Font_PrintString(p.posX + offset, p.posY - 20.0f, text, 1);
    if (p.feedbackKind == 3 || p.feedbackKind == 4) {
        Font_PrintString(p.posX + offset, p.posY, GetText(p.bonusKey), 1);
    }
}

// 0x6FC790
ClassMusic* __fastcall MusicConstruct(ClassMusic* m, re::Edx) {
    Minigame_Construct(m);
    m->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00947E80);
    m->feedbackFunction = 0;
    m->unknown105A4 = 26.0f;
    m->unknown38 = 1;
    m->unknown3C = 0x12;
    return m;
}

} // namespace

#define CMUS43_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CMUS43_REVERSED("ClassMusic::AddNote_6FAAE0", 0x006FAAE0, &MusicAddNote);
CMUS43_REVERSED("ClassMusic::End_6FACA0", 0x006FACA0, &MusicEnd);
CMUS43_REVERSED("ClassMusic::Finish_6FACD0", 0x006FACD0, &MusicFinish);
CMUS43_REVERSED("ClassMusic::Feedback_6FACF0", 0x006FACF0, &MusicFeedback);
CMUS43_REVERSED("ClassMusic::GetWinner_6FAD70", 0x006FAD70, &MusicGetWinner);
CMUS43_REVERSED("ClassMusic::GetScore_6FAD80", 0x006FAD80, &MusicGetScore);
CMUS43_REVERSED("ClassMusic::Judge_6FADA0", 0x006FADA0, &MusicJudge);
CMUS43_REVERSED("ClassMusic::CheckPress_6FB020", 0x006FB020, &MusicCheckPress);
CMUS43_REVERSED("ClassMusic::CheckHold_6FB0C0", 0x006FB0C0, &MusicCheckHold);
CMUS43_REVERSED("ClassMusic::SetPlayerMode_6FB1A0", 0x006FB1A0, &MusicSetPlayerMode);
CMUS43_REVERSED("ClassMusic::Start_6FB1B0", 0x006FB1B0, &MusicStart);
CMUS43_REVERSED("ClassMusic::Reset_6FB2F0", 0x006FB2F0, &MusicReset);
CMUS43_REVERSED("ClassMusic::LoadSong_6FB440", 0x006FB440, &MusicLoadSong);
CMUS43_REVERSED("ClassMusic::DrawHud_6FB6A0", 0x006FB6A0, &MusicDrawHud);
CMUS43_REVERSED("ClassMusic::DrawNotes_6FBAF0", 0x006FBAF0, &MusicDrawNotes);
CMUS43_REVERSED("ClassMusic::DrawScore_6FC2B0", 0x006FC2B0, &MusicDrawScore);
CMUS43_REVERSED("ClassMusic::SetFeedback_6FC460", 0x006FC460, &MusicSetFeedback);
CMUS43_REVERSED("ClassMusic::DrawFeedback_6FC5A0", 0x006FC5A0, &MusicDrawFeedback);
CMUS43_REVERSED("ClassMusic::ClassMusic_6FC790", 0x006FC790, &MusicConstruct);
