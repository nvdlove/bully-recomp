// Music class (ClassMusic, vtable 0x947E80): its per-frame virtuals (update, draw, the
// deleting destructor), the judging of each player's current note and the song native
// (0x6FC7D0..0x6FCF9C; the rest of the class, 0x6FAD70..0x6FC790, is another pass's).
//
// The class: +0xC4 the feedback callback (a script function's reference), then one block
// of 0x8254 bytes per player from +0x0: +0xC8 its notes (0x1C each: +0xC8 the state, +0xCC
// the button, +0xD0 the time, +0xD8 the error, +0xDD hit), +0x46C8 the notes' kinds (0x18
// each), +0x82C8 the current note, +0x82D0 the streak, +0x82FC the last grade, +0x8306 /
// +0x8307 flags. After them: +0x1051C the second player's note, +0x10572 running,
// +0x10573 / +0x10574 the song loaded / playing, +0x10578 the song time, +0x1057C its last
// step, +0x10584 the players (3: one player against the game), +0x105A0 / +0x105A2 tick
// counters. Note states: 1 waiting, 2 / 3 due, 4 hit, 5 missed, 6 / 7 a hold.

#include <cstdint>

#include "game/SchoolClasses.h"
#include "game/ScriptApi.h"
#include "reversed.h"

namespace {

using namespace school;

GAME_VAR(uint8_t, musicPausedC1A998, 0x00C1A998);
GAME_VAR(uint8_t, musicPausedC1A999, 0x00C1A999);
GAME_VAR(uint8_t, musicPausedC1A99A, 0x00C1A99A);
GAME_VAR(uint8_t, musicPopupShowing, 0x020C330C);   // a minigame popup is up
GAME_VAR(int32_t, musicFpsLimit, 0x00BCBC48);
GAME_VAR(uint8_t, musicSoundsPausedCF3DD8, 0x00CF3DD8);
GAME_VAR(float, musicSongTime, 0x00BC6860);          // the song's position (the notes' clock)
GAME_VAR(float, musicSongSpeed, 0x00BC685C);
inline void* const musicStreams = reinterpret_cast<void*>(0x00CF42D4);
inline void* const musicButtonMap = reinterpret_cast<void*>(0x00A14810);

GAME_FN(void, __thiscall, MusicStream_StopAny, 0x00591480, void* stream, int priority);
GAME_FN(void, __thiscall, MusicStream_PlayPreloaded, 0x005905E0, void* stream, int priority);
GAME_FN(uint8_t, __thiscall, MusicStream_IsLoaded, 0x0058EE40, void* stream);
GAME_FN(void, __thiscall, MusicStream_PreloadByName, 0x00591380, void* stream, const char* name, float volume,
        int flag, int priority, int fadeIn, int fadeOut);
GAME_FN(void, __thiscall, ButtonMap_RestoreKeys, 0x00454D60, void* map);
// the rest of the class (another pass's)
GAME_FN(void, __thiscall, Music_Grade, 0x006FADA0, void* mg, int player, float grade, int held, int state);
GAME_FN(uint8_t, __thiscall, Music_ButtonPressed, 0x006FB020, void* mg, int kind, int player);
GAME_FN(uint8_t, __thiscall, Music_HoldReleased, 0x006FB0C0, void* mg, int player);
GAME_FN(void, __thiscall, Music_DrawTrack, 0x006FB6A0, void* mg, int player, float sx, float sy);
GAME_FN(void, __thiscall, Music_DrawNotes, 0x006FBAF0, void* mg, int player, float sx, float sy);
GAME_FN(void, __thiscall, Music_DrawScore, 0x006FC2B0, void* mg, int player);
GAME_FN(void, __thiscall, Music_ShowGrade, 0x006FC460, void* mg, int player, int grade, int late);
GAME_FN(void, __thiscall, Music_DrawFeedback, 0x006FC5A0, void* mg, int player);
// (this file's)
GAME_FN(void, __thiscall, Music_Judge, 0x006FC9A0, void* mg, int player);

constexpr int kMusicPlayerSize = 0x8254;

inline uint8_t* MusicPlayer(uint8_t* mg, int player) {
    return mg + player * kMusicPlayerSize;
}
inline int32_t& MusicNoteState(uint8_t* p, int note) {
    return At<int32_t>(p, note * 0x1C + 0xC8);
}
inline bool MusicSoloMode(int32_t players) {
    return players == 1 || players == 3;
}

// 0x6FC7D0 (virtual 0): the deleting destructor (the callback cleared twice)
uint8_t* __fastcall MusicDestruct(uint8_t* mg, re::Edx, int flags) {
    At<uint32_t>(mg, 0) = 0x00947E80;
    ScriptRef_Clear(&At<uint32_t>(mg, 0xC4));
    Minigame_EndSounds(mg);
    ButtonMap_RestoreKeys(musicButtonMap);
    musicFpsLimit = -1;
    ScriptRef_Clear(&At<uint32_t>(mg, 0xC4));
    Minigame_Destruct(mg);
    if ((flags & 1) != 0) {
        Free(mg);
    }
    return mg;
}

// 0x6FC830 (virtual 2): draws the players' tracks, notes and feedback (one player against
// the game: the second's notes only, after the first's score; no Font_Render then)
void __fastcall MusicDraw(uint8_t* mg, re::Edx) {
    if (At<uint8_t>(mg, 0xA0) != 0) {
        Popup_Draw(mg + 0x50);
        return;
    }
    if (userStartResponse != 0 || musicPausedC1A99A != 0 || musicPausedC1A999 != 0 || musicPausedC1A998 != 0 ||
        At<uint8_t>(mg, 5) == 0 || At<uint8_t>(mg, 0x10572) == 0) {
        return;
    }
    float sx = screenScaleX;
    float sy = screenScaleY;
    int32_t players = At<int32_t>(mg, 0x10584);
    if (players == 3) {
        Music_DrawTrack(mg, 0, sx, sy);
        Music_DrawNotes(mg, 0, sx, sy);
        Music_DrawFeedback(mg, 0);
        Music_DrawScore(mg, 0);
        Music_DrawNotes(mg, 1, sx, sy);
        return;
    }
    for (int i = 0; i < At<int32_t>(mg, 0x10584); i++) {
        Music_DrawTrack(mg, i, sx, sy);
    }
    for (int i = 0; i < At<int32_t>(mg, 0x10584); i++) {
        Music_DrawNotes(mg, i, sx, sy);
        Music_DrawFeedback(mg, i);
    }
    if (At<int32_t>(mg, 0x10584) == 1) {
        Music_DrawScore(mg, 0);
    }
    Font_Render();
}

// 0x6FC9A0: judges a player's current note against the pad (the computer's notes, player
// 1 against the game, are always hit)
void __fastcall MusicJudge(uint8_t* mg, re::Edx, int player) {
    int32_t players = At<int32_t>(mg, 0x10584);
    if (players == 3 && player == 1) {
        // (the second player's note: +0x831C is its block's +0xC8)
        int32_t& second = At<int32_t>(mg, At<int32_t>(mg, 0x1051C) * 0x1C + 0x831C);
        if (second == 5) {
            second = 5;
            At<float>(mg, At<int32_t>(mg, 0x1051C) * 0x1C + 0x832C) = 0.0f;
            Music_Grade(mg, 1, 1.0f, 0, 0);
            return;
        }
    }
    uint8_t* p = MusicPlayer(mg, player);
    int32_t note = At<int32_t>(p, 0x82C8);
    int32_t state = MusicNoteState(p, note);
    switch (state) {
    case 1: {
        if (At<uint8_t>(p, 0x8307) != 0) {
            return;
        }
        int16_t ticks = At<int16_t>(mg, 0x105A0);
        if (ticks <= 2) {
            At<int16_t>(mg, 0x105A0) = int16_t(ticks + 1);
            return;
        }
        uint8_t released = Music_HoldReleased(mg, player);
        At<uint8_t>(p, 0x8306) = released;
        if (released == 0) {
            return;
        }
        At<int16_t>(mg, 0x105A0) = 0;
        At<int16_t>(mg, 0x105A2) = 0;
        At<uint8_t>(p, At<int32_t>(p, 0x82C8) * 0x1C + 0xDD) = 1;
        int32_t now = At<int32_t>(mg, 0x10584);
        Music_Grade(mg, player, MusicSoloMode(now) ? -0.5f : -0.25f, 1, 0);
        return;
    }
    case 2:
    case 3: {
        if (Music_ButtonPressed(mg, At<int32_t>(p, note * 0x18 + 0x46C8), player) == 0) {
            return;
        }
        note = At<int32_t>(p, 0x82C8);
        if (At<int32_t>(p, note * 0x18 + 0x46C8) == 3) {
            Music_Grade(mg, player, 0.0f, 0, 0);
            return;
        }
        uint8_t late = At<int32_t>(p, 0x82D0) >= 10 ? 1 : 0;
        // (the grade's flag argument is the player argument's slot with its low byte
        // replaced: the same value for players 0 and 1)
        int lateArg = (player & ~0xFF) | late;
        int32_t& noteState = MusicNoteState(p, note);
        if (noteState == 2) {
            noteState = 4;
            int32_t now = At<int32_t>(p, 0x82C8);
            At<float>(p, now * 0x1C + 0xD8) = musicSongTime - At<float>(p, now * 0x1C + 0xD0);
            int grade = late * 2 + 1;
            At<int32_t>(p, 0x82FC) = grade;
            Music_ShowGrade(mg, player, grade, lateArg);
            Music_Grade(mg, player, MusicSoloMode(At<int32_t>(mg, 0x10584)) ? 0.9f : 0.5f, 0, 0);
            return;
        }
        noteState = 5;
        At<float>(p, At<int32_t>(p, 0x82C8) * 0x1C + 0xD8) = 0.0f;
        int grade = late * 2 + 2;
        At<int32_t>(p, 0x82FC) = grade;
        Music_ShowGrade(mg, player, grade, lateArg);
        if (MusicSoloMode(At<int32_t>(mg, 0x10584))) {
            Music_Grade(mg, player, 1.0f, 0, 0);
        } else {
            Music_Grade(mg, player, float(int32_t(late) + 1), 0, 0);
        }
        return;
    }
    case 6:
    case 7:
        // (players as read on entry)
        Music_Grade(mg, player, MusicSoloMode(players) ? -0.5f : -0.25f, 0, state);
        return;
    default:
        return;
    }
}

// 0x6FCCF0 (virtual 1): the popups (the exit question quits the class), the song (started
// once loaded), the clock and the judging, then the pause buttons
void __fastcall MusicUpdate(uint8_t* mg, re::Edx) {
    void* popup = mg + 0x50;
    if (At<uint8_t>(mg, 0xA0) != 0) {
        if (At<int32_t>(mg, 0x50) == 1) {
            if (Popup_Result(popup) == 2) {
                MusicStream_StopAny(musicStreams, 0);
                musicPopupShowing = 0;
                At<uint8_t>(mg, 4) = 0;
                At<uint8_t>(mg, 5) = 0;
                At<uint8_t>(mg, 6) = 0;
                At<uint8_t>(mg, 0x10572) = 0;
                ScriptRef_Clear(&At<uint32_t>(mg, 0xC4));
                Minigame_EndSounds(mg);
                return;
            }
            if (At<uint8_t>(mg, 0xA0) == 0) {
                musicPopupShowing = 0;
                return;
            }
        } else if (At<uint8_t>(mg, 0xA0) != 0) {
            if (At<int32_t>(mg, 0x50) == 2) {
                Popup_Result(popup);
                if (At<uint8_t>(mg, 0xA0) != 0) {
                    return;
                }
                musicPopupShowing = 0;
                return;
            }
            if (At<uint8_t>(mg, 0xA0) != 0 && At<int32_t>(mg, 0x50) == 3) {
                if (Popup_Result(popup) != 2) {
                    return;
                }
                At<uint8_t>(mg, 4) = 0;
                return;
            }
        }
    }
    if (userStartResponse != 0 || musicPausedC1A99A != 0 || musicPausedC1A999 != 0 || musicPausedC1A998 != 0 ||
        At<uint8_t>(mg, 0x10572) == 0) {
        return;
    }
    if (At<uint8_t>(mg, 0x10573) != 0 && At<uint8_t>(mg, 0x10574) == 0) {
        MusicStream_PlayPreloaded(musicStreams, 6);
        At<uint8_t>(mg, 0x10574) = 1;
    }
    if (MusicStream_IsLoaded(musicStreams) != 0 && At<uint8_t>(mg, 0x10573) == 0) {
        At<uint8_t>(mg, 0x10573) = 1;
    }
    if (At<uint8_t>(mg, 0x10573) != 0) {
        float step = musicSongSpeed * timeStep;
        At<float>(mg, 0x1057C) = step;
        At<float>(mg, 0x10578) = step + At<float>(mg, 0x10578);
        if (At<int32_t>(mg, 0x10584) == 3) {
            Music_Judge(mg, 0);
            Music_Judge(mg, 1);
        } else {
            for (int i = 0; i < At<int32_t>(mg, 0x10584); i++) {
                Music_Judge(mg, i);
            }
        }
    }
    if (Pad_JustDown(8, 0) != 0) {
        Popup_Show(popup, 1, GameString(0x00944690), GameString(0x00941D14), 7, 8);   // EXIT_QUESTION
        musicPopupShowing = 1;
        return;
    }
    if (At<int32_t>(mg, 0x10584) != 3 && Pad_JustDown(6, 0) != 0) {
        Popup_Show(popup, 2, GameString(0x00947E9C), GameString(0x00947EB4), 7, -1);   // MC_INFO, MC_FOOTER
        musicSoundsPausedCF3DD8 = 1;
        musicPopupShowing = 1;
    }
}

// 0x6FCF40: ClassMusicSetSong(name, volume [, fadeIn = 500, fadeOut = 1000]): preloads it
int __cdecl Native_ClassMusicSetSong(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    float volume = Script::GetFloat(L, 1);
    int fadeIn = Script::GetIntOpt(L, 2, 500);
    int fadeOut = Script::GetIntOpt(L, 3, 1000);
    MusicStream_PreloadByName(musicStreams, name, volume, 0, 6, fadeIn, fadeOut);
    return 0;
}

} // namespace

#define CMUS44_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CMUS44_REVERSED("ClassMusic::DeletingDestructor_6FC7D0", 0x006FC7D0, &MusicDestruct);
CMUS44_REVERSED("ClassMusic::Draw_6FC830", 0x006FC830, &MusicDraw);
CMUS44_REVERSED("ClassMusic::Judge_6FC9A0", 0x006FC9A0, &MusicJudge);
CMUS44_REVERSED("ClassMusic::Update_6FCCF0", 0x006FCCF0, &MusicUpdate);
CMUS44_REVERSED("Native_ClassMusicSetSong", 0x006FCF40, &Native_ClassMusicSetSong);
