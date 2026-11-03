// The Biology class's animals (0x716040..0x71C46B): the alien's (7), the bat's (5) and the
// frog's (0) set-up (their textures), start (the stage machine: each tool's check moving the
// dissection on a stage) and reset (each stage's pins, marks, incisions, organs and the
// markers showing the animal's parts). ClassBiology.cpp is the class (its +0x110 is the
// stage here); the other animals are another pass's.
//
// The fields (ClassBiology.cpp's layout): +0x128 the background, +0x14C pins (+0x150 their
// positions, +0x170 pinned), +0x174 marks (+0x178, +0x198), +0x1CC incisions (+0x1D0 loop,
// +0x1DC the marker they are relative to, +0x1E8 started, +0x1EB done, +0x1FC point counts,
// +0x214 / +0x2D4 segment flags, +0x394 points: 0x40 each), +0x1338 organs (+0x133C
// positions, +0x135C removed, +0x1364 the turning direction, +0x1394 the tray image, +0x13A4
// frame counts, +0x13B4 their markers), +0x1404 markers (+0x1408 positions, +0x1488 scales,
// +0x14C8 images, +0x1508 visible), +0x1518 the tray, +0x152C its item's scale.
// The resets are long tables of constants (from the game's code; checked against it in
// emulation, every stage); the incisions' points of some stages are scaled to the screen and
// placed relative to their marker. The x87 runs at 24-bit precision here: float.

#include <bit>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

inline float& F32(uint8_t* bio, int offset) {
    return *reinterpret_cast<float*>(bio + offset);
}
inline int32_t& I32(uint8_t* bio, int offset) {
    return *reinterpret_cast<int32_t*>(bio + offset);
}
inline uint8_t& U8(uint8_t* bio, int offset) {
    return bio[offset];
}
inline float Fb(uint32_t bits) {
    return std::bit_cast<float>(bits);
}

GAME_VAR(float, screenScaleXBio, 0x00A957A8);
GAME_VAR(float, screenScaleYBio, 0x00A957AC);
inline void* const soundsBio = reinterpret_cast<void*>(0x00CF3DA0);

// a Minigame texture table entry
struct BioAnimalTexture {
    int32_t id;
    re::Ptr32<const char> name;
    re::Ptr32<const char> mask;
};
static_assert(sizeof(BioAnimalTexture) == 0xC);

// ClassBiology.cpp's (called at the game's addresses, so modes apply)
GAME_FN(uint8_t, __thiscall, Bio_LoadTextureSets, 0x006E38B0, uint8_t* bio, int count, const BioAnimalTexture* entries,
        const char* txd1, const char* txd2, const char* txd3, int n1, int n2, int n3);
GAME_FN(void, __thiscall, Bio_ResetAnimal, 0x006E3850, uint8_t* bio);
GAME_FN(void, __thiscall, Bio_ResetCuts_x64, 0x006E3DE0, uint8_t* bio, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Bio_ResetCuts(uint8_t* a0, float a1) {
    return Bio_ResetCuts_x64(a0, re::BitsOf(a1));
}
GAME_FN(uint8_t, __thiscall, Bio_CheckPins, 0x006E48F0, uint8_t* bio);
GAME_FN(uint8_t, __thiscall, Bio_CheckMarks, 0x006E49C0, uint8_t* bio);
GAME_FN(uint8_t, __thiscall, Bio_UpdateOrgans, 0x006E4BB0, uint8_t* bio);
GAME_FN(uint8_t, __thiscall, Bio_UpdateCut, 0x006E72F0, uint8_t* bio);
GAME_FN(void, __thiscall, Minigame_Success, 0x00705790, uint8_t* bio);
GAME_FN(int32_t, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, int32_t id, const float* pos, int, uintptr_t, int, int,
        int, int, uintptr_t, uintptr_t, int, int, float, int);
// the markers all shown: this file's and the other animals' copy (no argument)
GAME_FN(void, __thiscall, Bio_ShowMarkers, 0x00716210, uint8_t* bio, float);
GAME_FN(void, __thiscall, Bio_ShowMarkersB, 0x00725A20, uint8_t* bio);

// flags[0..count) cleared (at least the first: the game's do-while)
inline void ClearFlags(uint8_t* bio, int flags, int count) {
    int i = 0;
    do {
        U8(bio, flags + i) = 0;
        i++;
    } while (i < I32(bio, count));
}
// the incisions' flags cleared (at least the first's)
inline void ResetCutFlags(uint8_t* bio) {
    int c = 0;
    do {
        U8(bio, 0x1EB + c) = 0;
        U8(bio, 0x1E8 + c) = 0;
        for (int j = 0; j < I32(bio, 0x1FC + c * 4); j++) {
            U8(bio, 0x214 + c * 0x40 + j) = 0;
            U8(bio, 0x2D4 + c * 0x40 + j) = 0;
        }
        c++;
    } while (c < I32(bio, 0x1CC));
}
// the incisions' flags cleared and their points scaled to the screen (x by sx, y by sy,
// both by k) and placed relative to their markers
inline void ScaleCuts(uint8_t* bio, float sx, float sy, float k) {
    for (int c = 0; c < I32(bio, 0x1CC); c++) {
        U8(bio, 0x1EB + c) = 0;
        U8(bio, 0x1E8 + c) = 0;
        float* p = &F32(bio, 0x394 + c * 0x200);
        for (int j = 0; j < I32(bio, 0x1FC + c * 4); j++, p += 2) {
            U8(bio, 0x214 + c * 0x40 + j) = 0;
            U8(bio, 0x2D4 + c * 0x40 + j) = 0;
            p[0] = p[0] * sx;
            p[1] = sy * p[1];
            p[0] = p[0] * k;
            p[1] = p[1] * k;
            const float* marker = &F32(bio, 0x1408 + I32(bio, 0x1DC + c * 4) * 8);
            p[0] = p[0] + marker[0];
            p[1] = marker[1] + p[1];
        }
    }
}

#define BIOANIMALS45_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// ---- the set-ups: each animal's textures from its three txds ---------------------------------------

// (thiscall) the alien's
void __fastcall AlienSetup(uint8_t* bio, re::Edx) {
    const BioAnimalTexture entries[15] = {
        {0, "alien_long", nullptr},
        {1, "alien_close", nullptr},
        {2, "alien_guts", nullptr},
        {3, "alien_flap_closed_r", "alien_flap_closed_r_x"},
        {4, "alien_flap_closed_l", "alien_flap_closed_l_x"},
        {5, "alien_flap_pinch_r", "alien_flap_pinch_r_x"},
        {6, "alien_flap_pinch_l", "alien_flap_pinch_l_x"},
        {7, "alien_flap_pull_r", "alien_flap_pull_r_x"},
        {8, "alien_flap_pull_l", "alien_flap_pull_l_x"},
        {9, "alien_flap_open_r", "alien_flap_open_r_x"},
        {10, "alien_flap_open_l", "alien_flap_open_l_x"},
        {11, "alien_heart", "alien_heart_x"},
        {12, "alien_kidney", "alien_kidney_x"},
        {13, "alien_spleen", "alien_spleen_x"},
        {14, "alien_stomach", "alien_stomach_x"},
    };
    Bio_LoadTextureSets(bio, 15, entries, "MG_BioAlien1", "MG_BioAlien2", "MG_BioAlien3", 4, 6, 5);
}
BIOANIMALS45_REVERSED("ClassBiology::AlienSetup", 0x00716040, &AlienSetup);

// (thiscall) the bat's
void __fastcall BatSetup(uint8_t* bio, re::Edx) {
    const BioAnimalTexture entries[14] = {
        {0, "bat_long", nullptr},
        {1, "bat_close", nullptr},
        {2, "bat_guts", nullptr},
        {3, "bat_flap_closed_r", "bat_flap_closed_r_x"},
        {4, "bat_flap_closed_l", "bat_flap_closed_l_x"},
        {5, "bat_flap_pinch_r", "bat_flap_pinch_r_x"},
        {6, "bat_flap_pinch_l", "bat_flap_pinch_l_x"},
        {7, "bat_flap_pull_r", "bat_flap_pull_r_x"},
        {8, "bat_flap_pull_l", "bat_flap_pull_l_x"},
        {9, "bat_flap_open_r", "bat_flap_open_r_x"},
        {10, "bat_flap_open_l", "bat_flap_open_l_x"},
        {11, "bat_heart", "bat_heart_x"},
        {12, "bat_LUNGS", "bat_LUNGS_x"},
        {13, "bat_liver", "bat_liver_x"},
    };
    Bio_LoadTextureSets(bio, 14, entries, "MG_BioBat1", "MG_BioBat2", "MG_BioBat3", 4, 4, 6);
}
BIOANIMALS45_REVERSED("ClassBiology::BatSetup", 0x007187D0, &BatSetup);

// (thiscall) the frog's
void __fastcall FrogSetup(uint8_t* bio, re::Edx) {
    const BioAnimalTexture entries[14] = {
        {0, "frog_long", nullptr},
        {1, "frog_close", nullptr},
        {2, "frog_guts", nullptr},
        {3, "frog_heart", "frog_heart_x"},
        {4, "frog_stoma", "frog_stoma_x"},
        {5, "frog_intes", "frog_intes_x"},
        {6, "frog_flap_r_closed", "frog_flap_r_closed_x"},
        {7, "frog_flap_l_closed", "frog_flap_l_closed_x"},
        {8, "frog_flap_r_pinch", "frog_flap_r_pinch_x"},
        {9, "frog_flap_l_pinch", "frog_flap_l_pinch_x"},
        {10, "frog_flap_r_pull", "frog_flap_r_pull_x"},
        {11, "frog_flap_l_pull", "frog_flap_l_pull_x"},
        {12, "frog_flap_r_open", "frog_flap_r_open_x"},
        {13, "frog_flap_l_open", "frog_flap_l_open_x"},
    };
    Bio_LoadTextureSets(bio, 14, entries, "MG_BioFrog1", "MG_BioFrog2", "MG_BioFrog3", 4, 5, 5);
}
BIOANIMALS45_REVERSED("ClassBiology::FrogSetup", 0x0071A4D0, &FrogSetup);

// (thiscall) every marker shown (the argument unused)
void __fastcall ShowMarkers(uint8_t* bio, re::Edx, float) {
    for (int i = 0; i < I32(bio, 0x1404); i++) {
        U8(bio, 0x1508 + i) = 1;
    }
}
// (callers rely on ecx and the x87 stack surviving it: switch it on together with the
// resets that call it, 0x716560)
BIOANIMALS45_REVERSED("ClassBiology::ShowMarkers", 0x00716210, &ShowMarkers);

// ---- the starts: the stage machine, by the tool in use ----------------------------------------------

// a stage moved on when its check passes (the animal then reset for it)
inline bool Advance(uint8_t* bio, uint8_t (*check)(uint8_t*), int to) {
    if (check(bio) == 1) {
        I32(bio, 0x110) = to;
        Bio_ResetAnimal(bio);
        return true;
    }
    return false;
}
uint8_t CheckPins(uint8_t* bio) {
    return Bio_CheckPins(bio);
}
uint8_t CheckMarks(uint8_t* bio) {
    return Bio_CheckMarks(bio);
}
uint8_t UpdateCut(uint8_t* bio) {
    return Bio_UpdateCut(bio);
}
uint8_t UpdateOrgans(uint8_t* bio) {
    return Bio_UpdateOrgans(bio);
}

// (thiscall) the alien's: pins (stages 5, 8, 11, 14), marks (0, 4), cuts (1, 2, 6, 9, 12,
// 15), organs (3, 7, 10, 13; at 16 the class passed)
void __fastcall AlienStart(uint8_t* bio, re::Edx) {
    int stage = I32(bio, 0x110);
    switch (I32(bio, 0x144)) {
    case 1:
        if (stage == 5) {
            Advance(bio, CheckPins, 6);
        } else if (stage == 8) {
            Advance(bio, CheckPins, 9);
        } else if (stage == 11) {
            Advance(bio, CheckPins, 12);
        } else if (stage == 14) {
            Advance(bio, CheckPins, 15);
        }
        break;
    case 2:
        if (stage == 0) {
            if (Bio_CheckMarks(bio) == 1) {
                I32(bio, 0x174) = 0;
                I32(bio, 0x110) = 1;
                Bio_ResetAnimal(bio);
            }
        } else if (stage == 4) {
            Advance(bio, CheckMarks, 5);
        }
        break;
    case 3:
        if (stage == 1 || stage == 2 || stage == 6 || stage == 9 || stage == 12 || stage == 15) {
            if (Bio_UpdateCut(bio) == 1) {
                I32(bio, 0x1CC) = 0;
                I32(bio, 0x110) = stage + 1;
                Bio_ResetAnimal(bio);
            }
        }
        break;
    case 4:
        if (stage == 3 || stage == 7 || stage == 10 || stage == 13) {
            if (Bio_UpdateOrgans(bio) == 1) {
                I32(bio, 0x1338) = 0;
                I32(bio, 0x110) = stage + 1;
                Bio_ResetAnimal(bio);
            }
        } else if (stage == 16) {
            if (Bio_UpdateOrgans(bio) == 1) {
                I32(bio, 0x1338) = 0;
                Minigame_Success(bio);
            }
        }
        break;
    }
}
BIOANIMALS45_REVERSED("ClassBiology::AlienStart", 0x00716240, &AlienStart);

// (thiscall) the frog's and the bat's: pins (stages 6, 9, 12; the identify sound), marks (0,
// 5), cuts (1, 2, 3, 7, 10, 13), organs (4, 8, 11 grossing out after the first; at 14 the
// class passed)
void __fastcall FrogStart(uint8_t* bio, re::Edx) {
    int stage = I32(bio, 0x110);
    switch (I32(bio, 0x144)) {
    case 1:
        if (stage == 6 || stage == 9 || stage == 12) {
            if (Bio_CheckPins(bio) == 1) {
                Sounds_Play(soundsBio, re::PtrCast<int32_t*>(*reinterpret_cast<re::Ptr32<uint32_t>*>(bio + 0x4C))[8], nullptr,
                            1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
                I32(bio, 0x110) = stage + 1;
                Bio_ResetAnimal(bio);
            }
        }
        break;
    case 2:
        if (stage == 0) {
            if (Bio_CheckMarks(bio) == 1) {
                I32(bio, 0x174) = 0;
                I32(bio, 0x110) = 1;
                Bio_ResetAnimal(bio);
            }
        } else if (stage == 5) {
            Advance(bio, CheckMarks, 6);
        }
        break;
    case 3:
        if (stage == 1 || stage == 2 || stage == 3 || stage == 7 || stage == 10 || stage == 13) {
            if (Bio_UpdateCut(bio) == 1) {
                I32(bio, 0x1CC) = 0;
                I32(bio, 0x110) = stage + 1;
                Bio_ResetAnimal(bio);
            }
        }
        break;
    case 4:
        if (stage == 4) {
            if (Bio_UpdateOrgans(bio) == 1) {
                I32(bio, 0x1338) = 0;
                I32(bio, 0x110) = 5;
                Bio_ResetAnimal(bio);
            }
        } else if (stage == 8 || stage == 11) {
            if (Bio_UpdateOrgans(bio) == 1) {
                U8(bio, 0x11B) = 1;
                I32(bio, 0x1338) = 0;
                I32(bio, 0x110) = stage + 1;
                Bio_ResetAnimal(bio);
            }
        } else if (stage == 14) {
            if (Bio_UpdateOrgans(bio) == 1) {
                I32(bio, 0x1338) = 0;
                Minigame_Success(bio);
            }
        }
        break;
    }
}
BIOANIMALS45_REVERSED("ClassBiology::FrogStart", 0x0071A680, &FrogStart);

// ---- the resets: each stage's parts (from the game's tables) ------------------------------------------

// (thiscall) the alien's (stages 0..0x11)
void __fastcall AlienReset(uint8_t* bio, re::Edx) {
    float sx = screenScaleXBio;
    float sy = screenScaleYBio;
    (void)sx;
    (void)sy;
    switch (I32(bio, 0x110)) {
    case 0:
        I32(bio, 0x128) = 0;
        I32(bio, 0x174) = 4;
        ClearFlags(bio, 0x198, 0x174);
        F32(bio, 0x178) = -64.0f;
        F32(bio, 0x17C) = -116.0f;
        F32(bio, 0x180) = 70.0f;
        F32(bio, 0x184) = -115.0f;
        F32(bio, 0x188) = -75.0f;
        F32(bio, 0x18C) = 115.0f;
        F32(bio, 0x190) = 75.0f;
        F32(bio, 0x194) = 120.0f;
        return;
    case 1:
        F32(bio, 0x1408) = -21.5f;
        I32(bio, 0x128) = 1;
        F32(bio, 0x140C) = -5.5f;
        I32(bio, 0x1404) = 2;
        I32(bio, 0x14C8) = 3;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14CC) = 4;
        F32(bio, 0x1410) = 33.0f;
        F32(bio, 0x1414) = -5.5f;
        F32(bio, 0x148C) = 1.0f;
        Bio_ShowMarkers(bio, 0.0f);
        F32(bio, 0x394) = 8.0f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x398) = -168.0f;
        I32(bio, 0x1D0) = 0;
        F32(bio, 0x39C) = 8.0f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 14;
        F32(bio, 0x3A0) = -143.0f;
        F32(bio, 0x3A4) = 8.0f;
        F32(bio, 0x3A8) = -118.0f;
        F32(bio, 0x3AC) = 8.0f;
        F32(bio, 0x3B0) = -93.0f;
        F32(bio, 0x3B4) = 8.0f;
        F32(bio, 0x3B8) = -68.0f;
        F32(bio, 0x3BC) = 8.0f;
        F32(bio, 0x3C0) = -43.0f;
        F32(bio, 0x3C4) = 8.0f;
        F32(bio, 0x3C8) = -18.0f;
        F32(bio, 0x3CC) = 8.0f;
        F32(bio, 0x3D0) = 7.0f;
        F32(bio, 0x3D4) = 8.0f;
        F32(bio, 0x3D8) = 32.0f;
        F32(bio, 0x3DC) = 8.0f;
        F32(bio, 0x3E0) = 57.0f;
        F32(bio, 0x3E4) = 8.0f;
        F32(bio, 0x3E8) = 82.0f;
        F32(bio, 0x3EC) = 8.0f;
        F32(bio, 0x3F0) = 107.0f;
        F32(bio, 0x3F4) = 8.0f;
        F32(bio, 0x3F8) = 132.0f;
        F32(bio, 0x3FC) = 8.0f;
        F32(bio, 0x400) = 157.0f;
        ResetCutFlags(bio);
        return;
    case 2:
        F32(bio, 0x1408) = -21.5f;
        I32(bio, 0x128) = 1;
        F32(bio, 0x140C) = -5.5f;
        I32(bio, 0x1404) = 2;
        I32(bio, 0x14C8) = 3;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14CC) = 4;
        F32(bio, 0x1410) = 33.0f;
        F32(bio, 0x1414) = -5.5f;
        F32(bio, 0x148C) = 1.0f;
        Bio_ShowMarkers(bio, 0.0f);
        F32(bio, 0x394) = -44.5f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x398) = -143.5f;
        F32(bio, 0x39C) = -34.5f;
        I32(bio, 0x1D0) = 0;
        I32(bio, 0x1DC) = 0;
        F32(bio, 0x3A0) = -159.5f;
        I32(bio, 0x1FC) = 7;
        F32(bio, 0x3A4) = -15.5f;
        F32(bio, 0x3A8) = -164.5f;
        F32(bio, 0x3AC) = 4.0f;
        F32(bio, 0x3B0) = -167.5f;
        F32(bio, 0x3B4) = 24.5f;
        F32(bio, 0x3B8) = -165.5f;
        F32(bio, 0x3BC) = 45.5f;
        F32(bio, 0x3C0) = -159.5f;
        F32(bio, 0x3C4) = 57.5f;
        F32(bio, 0x3C8) = -144.5f;
        ResetCutFlags(bio);
        return;
    case 3:
        F32(bio, 0x1408) = 8.5f;
        I32(bio, 0x1404) = 12;
        I32(bio, 0x14D0) = 12;
        F32(bio, 0x140C) = 84.0f;
        F32(bio, 0x1488) = 0.5f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x14C8) = 14;
        F32(bio, 0x1410) = 4.5f;
        I32(bio, 0x14CC) = 13;
        I32(bio, 0x14D4) = 11;
        F32(bio, 0x1414) = 11.5f;
        I32(bio, 0x14D8) = 3;
        I32(bio, 0x14DC) = 4;
        F32(bio, 0x148C) = 0.49f;
        I32(bio, 0x14E0) = 5;
        I32(bio, 0x14E4) = 6;
        F32(bio, 0x1418) = 4.0f;
        I32(bio, 0x14E8) = 7;
        I32(bio, 0x14EC) = 8;
        F32(bio, 0x141C) = -43.5f;
        I32(bio, 0x14F0) = 9;
        F32(bio, 0x1490) = 0.48f;
        F32(bio, 0x1420) = 6.0f;
        F32(bio, 0x1424) = -106.5f;
        F32(bio, 0x1494) = 0.6f;
        F32(bio, 0x1428) = -21.5f;
        F32(bio, 0x142C) = -5.5f;
        F32(bio, 0x1498) = 1.0f;
        F32(bio, 0x1430) = 33.0f;
        F32(bio, 0x1434) = -5.5f;
        F32(bio, 0x149C) = 1.0f;
        F32(bio, 0x1438) = -21.5f;
        F32(bio, 0x143C) = -5.5f;
        F32(bio, 0x14A0) = 1.0f;
        F32(bio, 0x1440) = 33.0f;
        F32(bio, 0x1444) = -5.5f;
        F32(bio, 0x14A4) = 1.0f;
        F32(bio, 0x1448) = -21.5f;
        F32(bio, 0x144C) = -5.5f;
        F32(bio, 0x14A8) = 1.0f;
        F32(bio, 0x1450) = 33.0f;
        F32(bio, 0x1454) = -5.5f;
        F32(bio, 0x14AC) = 1.0f;
        F32(bio, 0x1458) = -69.5f;
        F32(bio, 0x145C) = 9.0f;
        F32(bio, 0x14B0) = 1.0f;
        F32(bio, 0x1460) = 79.0f;
        I32(bio, 0x14F4) = 10;
        F32(bio, 0x1464) = 9.0f;
        F32(bio, 0x14B4) = 1.0f;
        Bio_ShowMarkers(bio, 0.0f);
        U8(bio, 0x150E) = 0;
        U8(bio, 0x150F) = 0;
        U8(bio, 0x1510) = 0;
        U8(bio, 0x1511) = 0;
        U8(bio, 0x1512) = 0;
        U8(bio, 0x1513) = 0;
        I32(bio, 0x1338) = 2;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = -14.0f;
        F32(bio, 0x1340) = -20.0f;
        I32(bio, 0x1364) = 2;
        I32(bio, 0x13A4) = 4;
        I32(bio, 0x13B4) = 4;
        I32(bio, 0x13B8) = 6;
        I32(bio, 0x13BC) = 8;
        I32(bio, 0x13C0) = 10;
        F32(bio, 0x1344) = 29.0f;
        F32(bio, 0x1348) = -20.0f;
        I32(bio, 0x1368) = 3;
        I32(bio, 0x13A8) = 4;
        I32(bio, 0x13C4) = 5;
        I32(bio, 0x13C8) = 7;
        I32(bio, 0x13CC) = 9;
        I32(bio, 0x13D0) = 11;
        return;
    case 4:
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x1404) = 6;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x14C8) = 9;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14D0) = 14;
        I32(bio, 0x14D4) = 13;
        F32(bio, 0x1410) = 79.0f;
        I32(bio, 0x14D8) = 12;
        I32(bio, 0x14DC) = 11;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 8.5f;
        F32(bio, 0x141C) = 84.0f;
        F32(bio, 0x1490) = 0.5f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        F32(bio, 0x1428) = 4.0f;
        F32(bio, 0x142C) = -43.5f;
        F32(bio, 0x1498) = 0.48f;
        F32(bio, 0x1430) = 6.0f;
        F32(bio, 0x1434) = -106.5f;
        F32(bio, 0x149C) = 0.6f;
        Bio_ShowMarkers(bio, 0.0f);
        I32(bio, 0x174) = 2;
        ClearFlags(bio, 0x198, 0x174);
        F32(bio, 0x178) = -121.0f;
        F32(bio, 0x17C) = 26.0f;
        F32(bio, 0x180) = 126.0f;
        F32(bio, 0x184) = 33.0f;
        return;
    case 5:
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x128) = 1;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x1404) = 6;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14CC) = 10;
        I32(bio, 0x14D0) = 14;
        F32(bio, 0x1410) = 79.0f;
        I32(bio, 0x14D4) = 13;
        I32(bio, 0x14D8) = 12;
        F32(bio, 0x1414) = 9.0f;
        I32(bio, 0x14DC) = 11;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 8.5f;
        F32(bio, 0x141C) = 84.0f;
        F32(bio, 0x1490) = 0.5f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        F32(bio, 0x1428) = 4.0f;
        F32(bio, 0x142C) = -43.5f;
        F32(bio, 0x1498) = 0.48f;
        F32(bio, 0x1430) = 6.0f;
        F32(bio, 0x1434) = -106.5f;
        F32(bio, 0x149C) = 0.6f;
        Bio_ShowMarkers(bio, 0.0f);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 6.0f;
        F32(bio, 0x154) = -106.5f;
        return;
    case 6:
        F32(bio, 0x1408) = 3.5f;
        F32(bio, 0x140C) = -14.0f;
        I32(bio, 0x128) = 2;
        I32(bio, 0x174) = 0;
        F32(bio, 0x1488) = 2.5f;
        I32(bio, 0x1404) = 1;
        I32(bio, 0x14C8) = 11;
        Bio_ShowMarkers(bio, 0.0f);
        F32(bio, 0x394) = 0.0f;
        F32(bio, 0x398) = -89.0f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x39C) = 30.0f;
        I32(bio, 0x1D0) = 1;
        I32(bio, 0x1DC) = 0;
        F32(bio, 0x3A0) = -79.0f;
        I32(bio, 0x1FC) = 18;
        F32(bio, 0x3A4) = 56.0f;
        F32(bio, 0x3A8) = -57.0f;
        F32(bio, 0x3AC) = 65.0f;
        F32(bio, 0x3B0) = -23.0f;
        F32(bio, 0x3B4) = 71.0f;
        F32(bio, 0x3B8) = 11.0f;
        F32(bio, 0x3BC) = 82.0f;
        F32(bio, 0x3C0) = 41.0f;
        F32(bio, 0x3C4) = 80.0f;
        F32(bio, 0x3C8) = 75.0f;
        F32(bio, 0x3CC) = 62.0f;
        F32(bio, 0x3D0) = 102.0f;
        F32(bio, 0x3D4) = 30.0f;
        F32(bio, 0x3D8) = 106.0f;
        F32(bio, 0x3DC) = 0.0f;
        F32(bio, 0x3E0) = 102.0f;
        F32(bio, 0x3E4) = -31.0f;
        F32(bio, 0x3E8) = 89.0f;
        F32(bio, 0x3EC) = -57.0f;
        F32(bio, 0x3F0) = 68.0f;
        F32(bio, 0x3F4) = -79.0f;
        F32(bio, 0x3F8) = 41.0f;
        F32(bio, 0x3FC) = -88.0f;
        F32(bio, 0x400) = 8.0f;
        F32(bio, 0x404) = -92.0f;
        F32(bio, 0x408) = -26.0f;
        F32(bio, 0x40C) = -79.0f;
        F32(bio, 0x410) = -57.0f;
        F32(bio, 0x414) = -58.0f;
        F32(bio, 0x418) = -80.0f;
        F32(bio, 0x41C) = -29.0f;
        F32(bio, 0x420) = -93.0f;
        ScaleCuts(bio, sx, sy, 1.21f);
        return;
    case 7:
        I32(bio, 0x128) = 1;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.6f;
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x174) = 2;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x1404) = 6;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14CC) = 10;
        I32(bio, 0x14D0) = 14;
        F32(bio, 0x1410) = 79.0f;
        I32(bio, 0x14D4) = 13;
        I32(bio, 0x14D8) = 12;
        F32(bio, 0x1414) = 9.0f;
        I32(bio, 0x14DC) = 11;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 8.5f;
        F32(bio, 0x141C) = 84.0f;
        F32(bio, 0x1490) = 0.5f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        F32(bio, 0x1428) = 4.0f;
        F32(bio, 0x142C) = -43.5f;
        F32(bio, 0x1498) = 0.48f;
        F32(bio, 0x1430) = 6.0f;
        F32(bio, 0x1434) = -106.5f;
        F32(bio, 0x149C) = 0.6f;
        Bio_ShowMarkers(bio, 0.0f);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 6.0f;
        F32(bio, 0x1340) = -106.5f;
        I32(bio, 0x1394) = 11;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13A4) = 1;
        I32(bio, 0x13B4) = 5;
        return;
    case 8:
        I32(bio, 0x128) = 1;
        I32(bio, 0x144) = 0;
        I32(bio, 0x138) = 0;
        U8(bio, 0x1518) = 0;
        F32(bio, 0x1408) = -69.5f;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x1404) = 5;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14CC) = 10;
        I32(bio, 0x14D0) = 14;
        F32(bio, 0x1410) = 79.0f;
        I32(bio, 0x14D4) = 13;
        I32(bio, 0x14D8) = 12;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 8.5f;
        F32(bio, 0x141C) = 84.0f;
        F32(bio, 0x1490) = 0.5f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        F32(bio, 0x1428) = 4.0f;
        F32(bio, 0x142C) = -43.5f;
        F32(bio, 0x1498) = 0.48f;
        Bio_ShowMarkers(bio, 0.0f);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 4.0f;
        F32(bio, 0x154) = -43.5f;
        return;
    case 9:
        F32(bio, 0x1408) = 20.0f;
        F32(bio, 0x140C) = 1.5f;
        I32(bio, 0x128) = 2;
        I32(bio, 0x174) = 0;
        F32(bio, 0x1488) = 1.42f;
        I32(bio, 0x1404) = 1;
        I32(bio, 0x14C8) = 12;
        Bio_ShowMarkers(bio, 0.0f);
        F32(bio, 0x394) = 1.0f;
        F32(bio, 0x398) = -170.0f;
        I32(bio, 0x1CC) = 1;
        I32(bio, 0x1D0) = 1;
        F32(bio, 0x39C) = 34.0f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 34;
        F32(bio, 0x3A0) = -169.0f;
        F32(bio, 0x3A4) = 69.0f;
        F32(bio, 0x3A8) = -162.0f;
        F32(bio, 0x3AC) = 105.0f;
        F32(bio, 0x3B0) = -152.0f;
        F32(bio, 0x3B4) = 141.0f;
        F32(bio, 0x3B8) = -138.0f;
        F32(bio, 0x3BC) = 168.0f;
        F32(bio, 0x3C0) = -116.0f;
        F32(bio, 0x3C4) = 188.0f;
        F32(bio, 0x3C8) = -85.0f;
        F32(bio, 0x3CC) = 206.0f;
        F32(bio, 0x3D0) = -55.0f;
        F32(bio, 0x3D4) = 219.0f;
        F32(bio, 0x3D8) = -19.0f;
        F32(bio, 0x3DC) = 219.0f;
        F32(bio, 0x3E0) = 17.0f;
        F32(bio, 0x3E4) = 206.0f;
        F32(bio, 0x3E8) = 51.0f;
        F32(bio, 0x3EC) = 173.0f;
        F32(bio, 0x3F0) = 64.0f;
        F32(bio, 0x3F4) = 136.0f;
        F32(bio, 0x3F8) = 78.0f;
        F32(bio, 0x3FC) = 108.0f;
        F32(bio, 0x400) = 98.0f;
        F32(bio, 0x404) = 76.0f;
        F32(bio, 0x408) = 119.0f;
        F32(bio, 0x40C) = 41.0f;
        F32(bio, 0x410) = 131.0f;
        F32(bio, 0x414) = 7.0f;
        F32(bio, 0x418) = 139.0f;
        F32(bio, 0x41C) = -25.0f;
        F32(bio, 0x420) = 146.0f;
        F32(bio, 0x424) = -60.0f;
        F32(bio, 0x428) = 147.0f;
        F32(bio, 0x42C) = -96.0f;
        F32(bio, 0x430) = 143.0f;
        F32(bio, 0x434) = -129.0f;
        F32(bio, 0x438) = 128.0f;
        F32(bio, 0x43C) = -162.0f;
        F32(bio, 0x440) = 113.0f;
        F32(bio, 0x444) = -192.0f;
        F32(bio, 0x448) = 98.0f;
        F32(bio, 0x44C) = -214.0f;
        F32(bio, 0x450) = 71.0f;
        F32(bio, 0x454) = -212.0f;
        F32(bio, 0x458) = 37.0f;
        F32(bio, 0x45C) = -205.0f;
        F32(bio, 0x460) = 4.0f;
        F32(bio, 0x464) = -198.0f;
        F32(bio, 0x468) = -29.0f;
        F32(bio, 0x46C) = -186.0f;
        F32(bio, 0x470) = -59.0f;
        F32(bio, 0x474) = -167.0f;
        F32(bio, 0x478) = -84.0f;
        F32(bio, 0x47C) = -141.0f;
        F32(bio, 0x480) = -109.0f;
        F32(bio, 0x484) = -116.0f;
        F32(bio, 0x488) = -129.0f;
        F32(bio, 0x48C) = -87.0f;
        F32(bio, 0x490) = -144.0f;
        F32(bio, 0x494) = -59.0f;
        F32(bio, 0x498) = -156.0f;
        F32(bio, 0x49C) = -30.0f;
        F32(bio, 0x4A0) = -164.0f;
        ScaleCuts(bio, sx, sy, 0.71f);
        return;
    case 10:
        I32(bio, 0x174) = 2;
        I32(bio, 0x128) = 1;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.48f;
        F32(bio, 0x1408) = -69.5f;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x1404) = 5;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14CC) = 10;
        I32(bio, 0x14D0) = 14;
        F32(bio, 0x1410) = 79.0f;
        I32(bio, 0x14D4) = 13;
        I32(bio, 0x14D8) = 12;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 8.5f;
        F32(bio, 0x141C) = 84.0f;
        F32(bio, 0x1490) = 0.5f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        F32(bio, 0x1428) = 4.0f;
        F32(bio, 0x142C) = -43.5f;
        F32(bio, 0x1498) = 0.48f;
        Bio_ShowMarkers(bio, 0.0f);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 4.0f;
        F32(bio, 0x1340) = -43.5f;
        I32(bio, 0x1394) = 12;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13A4) = 1;
        I32(bio, 0x13B4) = 4;
        return;
    case 0xb:
        I32(bio, 0x128) = 1;
        I32(bio, 0x144) = 0;
        I32(bio, 0x138) = 0;
        U8(bio, 0x1518) = 0;
        F32(bio, 0x1408) = -69.5f;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x1404) = 4;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14CC) = 10;
        I32(bio, 0x14D0) = 14;
        F32(bio, 0x1410) = 79.0f;
        I32(bio, 0x14D4) = 13;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 8.5f;
        F32(bio, 0x141C) = 84.0f;
        F32(bio, 0x1490) = 0.5f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        Bio_ShowMarkers(bio, 0.0f);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 4.5f;
        F32(bio, 0x154) = 11.5f;
        return;
    case 0xc:
        F32(bio, 0x1408) = 2.0f;
        F32(bio, 0x140C) = -5.0f;
        I32(bio, 0x128) = 2;
        I32(bio, 0x174) = 0;
        F32(bio, 0x1488) = 1.15f;
        I32(bio, 0x1404) = 1;
        I32(bio, 0x14C8) = 13;
        Bio_ShowMarkers(bio, 0.0f);
        F32(bio, 0x394) = 33.0f;
        F32(bio, 0x398) = -188.0f;
        I32(bio, 0x1CC) = 1;
        I32(bio, 0x1D0) = 1;
        F32(bio, 0x39C) = 68.0f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 34;
        F32(bio, 0x3A0) = -184.0f;
        F32(bio, 0x3A4) = 102.0f;
        F32(bio, 0x3A8) = -187.0f;
        F32(bio, 0x3AC) = 133.0f;
        F32(bio, 0x3B0) = -174.0f;
        F32(bio, 0x3B4) = 146.0f;
        F32(bio, 0x3B8) = -144.0f;
        F32(bio, 0x3BC) = 146.0f;
        F32(bio, 0x3C0) = -109.0f;
        F32(bio, 0x3C4) = 135.0f;
        F32(bio, 0x3C8) = -78.0f;
        F32(bio, 0x3CC) = 115.0f;
        F32(bio, 0x3D0) = -51.0f;
        F32(bio, 0x3D4) = 112.0f;
        F32(bio, 0x3D8) = -14.0f;
        F32(bio, 0x3DC) = 113.0f;
        F32(bio, 0x3E0) = 20.0f;
        F32(bio, 0x3E4) = 129.0f;
        F32(bio, 0x3E8) = 48.0f;
        F32(bio, 0x3EC) = 125.0f;
        F32(bio, 0x3F0) = 83.0f;
        F32(bio, 0x3F4) = 115.0f;
        F32(bio, 0x3F8) = 116.0f;
        F32(bio, 0x3FC) = 98.0f;
        F32(bio, 0x400) = 146.0f;
        F32(bio, 0x404) = 74.0f;
        F32(bio, 0x408) = 168.0f;
        F32(bio, 0x40C) = 44.0f;
        F32(bio, 0x410) = 184.0f;
        F32(bio, 0x414) = 12.0f;
        F32(bio, 0x418) = 191.0f;
        F32(bio, 0x41C) = -21.0f;
        F32(bio, 0x420) = 189.0f;
        F32(bio, 0x424) = -51.0f;
        F32(bio, 0x428) = 183.0f;
        F32(bio, 0x42C) = -82.0f;
        F32(bio, 0x430) = 168.0f;
        F32(bio, 0x434) = -104.0f;
        F32(bio, 0x438) = 143.0f;
        F32(bio, 0x43C) = -113.0f;
        F32(bio, 0x440) = 110.0f;
        F32(bio, 0x444) = -119.0f;
        F32(bio, 0x448) = 80.0f;
        F32(bio, 0x44C) = -128.0f;
        F32(bio, 0x450) = 47.0f;
        F32(bio, 0x454) = -138.0f;
        F32(bio, 0x458) = 16.0f;
        F32(bio, 0x45C) = -143.0f;
        F32(bio, 0x460) = -17.0f;
        F32(bio, 0x464) = -147.0f;
        F32(bio, 0x468) = -48.0f;
        F32(bio, 0x46C) = -150.0f;
        F32(bio, 0x470) = -83.0f;
        F32(bio, 0x474) = -142.0f;
        F32(bio, 0x478) = -115.0f;
        F32(bio, 0x47C) = -124.0f;
        F32(bio, 0x480) = -141.0f;
        F32(bio, 0x484) = -99.0f;
        F32(bio, 0x488) = -165.0f;
        F32(bio, 0x48C) = -69.0f;
        F32(bio, 0x490) = -180.0f;
        F32(bio, 0x494) = -34.0f;
        F32(bio, 0x498) = -187.0f;
        F32(bio, 0x49C) = -1.0f;
        F32(bio, 0x4A0) = -189.0f;
        ScaleCuts(bio, sx, sy, 0.755f);
        return;
    case 0xd:
        I32(bio, 0x174) = 2;
        I32(bio, 0x128) = 1;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.49f;
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x1404) = 4;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x14C8) = 9;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14D0) = 14;
        I32(bio, 0x14D4) = 13;
        F32(bio, 0x1410) = 79.0f;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 8.5f;
        F32(bio, 0x141C) = 84.0f;
        F32(bio, 0x1490) = 0.5f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        Bio_ShowMarkers(bio, 0.0f);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 4.5f;
        F32(bio, 0x1340) = 11.5f;
        I32(bio, 0x1394) = 13;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13A4) = 1;
        I32(bio, 0x13B4) = 3;
        return;
    case 0xe:
        I32(bio, 0x128) = 1;
        I32(bio, 0x144) = 0;
        I32(bio, 0x138) = 0;
        U8(bio, 0x1518) = 0;
        F32(bio, 0x1408) = -69.5f;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x1404) = 3;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14CC) = 10;
        I32(bio, 0x14D0) = 14;
        F32(bio, 0x1410) = 79.0f;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 8.5f;
        F32(bio, 0x141C) = 84.0f;
        F32(bio, 0x1490) = 0.5f;
        Bio_ShowMarkers(bio, 0.0f);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 8.5f;
        F32(bio, 0x154) = 84.0f;
        return;
    case 0xf:
        F32(bio, 0x1408) = 6.0f;
        F32(bio, 0x140C) = -75.0f;
        I32(bio, 0x128) = 2;
        I32(bio, 0x174) = 0;
        F32(bio, 0x1488) = 1.65f;
        I32(bio, 0x1404) = 1;
        I32(bio, 0x14C8) = 14;
        Bio_ShowMarkers(bio, 0.0f);
        F32(bio, 0x394) = 0.0f;
        F32(bio, 0x398) = -45.0f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x39C) = 27.0f;
        I32(bio, 0x1D0) = 1;
        I32(bio, 0x1DC) = 0;
        F32(bio, 0x3A0) = -55.0f;
        I32(bio, 0x1FC) = 28;
        F32(bio, 0x3A4) = 58.0f;
        F32(bio, 0x3A8) = -56.0f;
        F32(bio, 0x3AC) = 81.0f;
        F32(bio, 0x3B0) = -37.0f;
        F32(bio, 0x3B4) = 95.0f;
        F32(bio, 0x3B8) = -9.0f;
        F32(bio, 0x3BC) = 102.0f;
        F32(bio, 0x3C0) = 23.0f;
        F32(bio, 0x3C4) = 104.0f;
        F32(bio, 0x3C8) = 55.0f;
        F32(bio, 0x3CC) = 113.0f;
        F32(bio, 0x3D0) = 86.0f;
        F32(bio, 0x3D4) = 118.0f;
        F32(bio, 0x3D8) = 118.0f;
        F32(bio, 0x3DC) = 109.0f;
        F32(bio, 0x3E0) = 150.0f;
        F32(bio, 0x3E4) = 88.0f;
        F32(bio, 0x3E8) = 173.0f;
        F32(bio, 0x3EC) = 54.0f;
        F32(bio, 0x3F0) = 177.0f;
        F32(bio, 0x3F4) = 29.0f;
        F32(bio, 0x3F8) = 154.0f;
        F32(bio, 0x3FC) = 15.0f;
        F32(bio, 0x400) = 125.0f;
        F32(bio, 0x404) = 0.0f;
        F32(bio, 0x408) = 96.0f;
        F32(bio, 0x40C) = -10.0f;
        F32(bio, 0x410) = 125.0f;
        F32(bio, 0x414) = -22.0f;
        F32(bio, 0x418) = 157.0f;
        F32(bio, 0x41C) = -44.0f;
        F32(bio, 0x420) = 181.0f;
        F32(bio, 0x424) = -76.0f;
        F32(bio, 0x428) = 186.0f;
        F32(bio, 0x42C) = -103.0f;
        F32(bio, 0x430) = 170.0f;
        F32(bio, 0x434) = -117.0f;
        F32(bio, 0x438) = 139.0f;
        F32(bio, 0x43C) = -112.0f;
        F32(bio, 0x440) = 106.0f;
        F32(bio, 0x444) = -106.0f;
        F32(bio, 0x448) = 74.0f;
        F32(bio, 0x44C) = -102.0f;
        F32(bio, 0x450) = 45.0f;
        F32(bio, 0x454) = -97.0f;
        F32(bio, 0x458) = 9.0f;
        F32(bio, 0x45C) = -85.0f;
        F32(bio, 0x460) = -21.0f;
        F32(bio, 0x464) = -62.0f;
        F32(bio, 0x468) = -42.0f;
        F32(bio, 0x46C) = -29.0f;
        F32(bio, 0x470) = -46.0f;
        ScaleCuts(bio, sx, sy, 1.06f);
        return;
    case 0x10:
        I32(bio, 0x128) = 1;
        I32(bio, 0x174) = 2;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.5f;
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x1404) = 3;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x14C8) = 9;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14D0) = 14;
        F32(bio, 0x1410) = 79.0f;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 8.5f;
        F32(bio, 0x141C) = 84.0f;
        F32(bio, 0x1490) = 0.5f;
        Bio_ShowMarkers(bio, 0.0f);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 8.5f;
        F32(bio, 0x1340) = 84.0f;
        I32(bio, 0x1394) = 14;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13A4) = 1;
        I32(bio, 0x13B4) = 2;
        return;
    case 0x11:
        I32(bio, 0x128) = 1;
        F32(bio, 0x1408) = -139.5f;
        I32(bio, 0x1404) = 2;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x140C) = -28.0f;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x1488) = 1.0f;
        F32(bio, 0x1410) = 110.5f;
        F32(bio, 0x1414) = -32.0f;
        F32(bio, 0x148C) = 1.0f;
    }
  return;
}
BIOANIMALS45_REVERSED("ClassBiology::AlienReset", 0x00716560, &AlienReset);

// (thiscall) the bat's (stages 0..0xF)
void __fastcall BatReset(uint8_t* bio, re::Edx) {
    float sx = screenScaleXBio;
    float sy = screenScaleYBio;
    (void)sx;
    (void)sy;
    switch (I32(bio, 0x110)) {
    case 0:
        I32(bio, 0x128) = 0;
        I32(bio, 0x174) = 4;
        ClearFlags(bio, 0x198, 0x174);
        F32(bio, 0x178) = -64.0f;
        F32(bio, 0x17C) = -116.0f;
        F32(bio, 0x180) = 70.0f;
        F32(bio, 0x184) = -115.0f;
        F32(bio, 0x188) = -75.0f;
        F32(bio, 0x18C) = 115.0f;
        F32(bio, 0x190) = 75.0f;
        F32(bio, 0x194) = 120.0f;
        return;
    case 1:
        F32(bio, 0x1408) = -21.5f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x1404) = 2;
        F32(bio, 0x140C) = -5.5f;
        I32(bio, 0x14C8) = 3;
        I32(bio, 0x14CC) = 4;
        F32(bio, 0x1488) = 1.0f;
        F32(bio, 0x1410) = 33.0f;
        F32(bio, 0x1414) = -5.5f;
        F32(bio, 0x148C) = 1.0f;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = 8.0f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x398) = -168.0f;
        I32(bio, 0x1D0) = 0;
        F32(bio, 0x39C) = 8.0f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 14;
        F32(bio, 0x3A0) = -143.0f;
        F32(bio, 0x3A4) = 8.0f;
        F32(bio, 0x3A8) = -118.0f;
        F32(bio, 0x3AC) = 8.0f;
        F32(bio, 0x3B0) = -93.0f;
        F32(bio, 0x3B4) = 8.0f;
        F32(bio, 0x3B8) = -68.0f;
        F32(bio, 0x3BC) = 8.0f;
        F32(bio, 0x3C0) = -43.0f;
        F32(bio, 0x3C4) = 8.0f;
        F32(bio, 0x3C8) = -18.0f;
        F32(bio, 0x3CC) = 8.0f;
        F32(bio, 0x3D0) = 7.0f;
        F32(bio, 0x3D4) = 8.0f;
        F32(bio, 0x3D8) = 32.0f;
        F32(bio, 0x3DC) = 8.0f;
        F32(bio, 0x3E0) = 57.0f;
        F32(bio, 0x3E4) = 8.0f;
        F32(bio, 0x3E8) = 82.0f;
        F32(bio, 0x3EC) = 8.0f;
        F32(bio, 0x3F0) = 107.0f;
        F32(bio, 0x3F4) = 8.0f;
        F32(bio, 0x3F8) = 132.0f;
        F32(bio, 0x3FC) = 8.0f;
        F32(bio, 0x400) = 157.0f;
        ResetCutFlags(bio);
        return;
    case 2:
        F32(bio, 0x1408) = -21.5f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x1404) = 2;
        F32(bio, 0x140C) = -5.5f;
        I32(bio, 0x14C8) = 3;
        I32(bio, 0x14CC) = 4;
        F32(bio, 0x1488) = 1.0f;
        F32(bio, 0x1410) = 33.0f;
        F32(bio, 0x1414) = -5.5f;
        F32(bio, 0x148C) = 1.0f;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = -44.5f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x398) = -143.5f;
        I32(bio, 0x1D0) = 0;
        F32(bio, 0x39C) = -34.5f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 7;
        F32(bio, 0x3A0) = -159.5f;
        F32(bio, 0x3A4) = -15.5f;
        F32(bio, 0x3A8) = -164.5f;
        F32(bio, 0x3AC) = 4.0f;
        F32(bio, 0x3B0) = -167.5f;
        F32(bio, 0x3B4) = 24.5f;
        F32(bio, 0x3B8) = -165.5f;
        F32(bio, 0x3BC) = 45.5f;
        F32(bio, 0x3C0) = -159.5f;
        F32(bio, 0x3C4) = 57.5f;
        F32(bio, 0x3C8) = -144.5f;
        ResetCutFlags(bio);
        return;
    case 3:
        F32(bio, 0x1408) = -21.5f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x1404) = 2;
        F32(bio, 0x140C) = -5.5f;
        I32(bio, 0x14C8) = 3;
        I32(bio, 0x14CC) = 4;
        F32(bio, 0x1488) = 1.0f;
        F32(bio, 0x1410) = 33.0f;
        F32(bio, 0x1414) = -5.5f;
        F32(bio, 0x148C) = 1.0f;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = -31.5f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x398) = 134.0f;
        I32(bio, 0x1D0) = 0;
        F32(bio, 0x39C) = -18.5f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 6;
        F32(bio, 0x3A0) = 145.0f;
        F32(bio, 0x3A4) = -4.5f;
        F32(bio, 0x3A8) = 157.0f;
        F32(bio, 0x3AC) = 19.5f;
        F32(bio, 0x3B0) = 157.0f;
        F32(bio, 0x3B4) = 33.5f;
        F32(bio, 0x3B8) = 145.0f;
        F32(bio, 0x3BC) = 47.5f;
        F32(bio, 0x3C0) = 134.0f;
        ResetCutFlags(bio);
        return;
    case 4:
        F32(bio, 0x1408) = 4.0f;
        F32(bio, 0x140C) = -43.5f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x1404) = 11;
        F32(bio, 0x1488) = 0.48f;
        I32(bio, 0x14C8) = 13;
        I32(bio, 0x14CC) = 12;
        F32(bio, 0x1410) = 4.5f;
        I32(bio, 0x14D0) = 11;
        I32(bio, 0x14D4) = 3;
        F32(bio, 0x1414) = 11.5f;
        I32(bio, 0x14D8) = 4;
        I32(bio, 0x14DC) = 5;
        F32(bio, 0x148C) = 0.49f;
        I32(bio, 0x14E0) = 6;
        I32(bio, 0x14E4) = 7;
        F32(bio, 0x1418) = 6.0f;
        I32(bio, 0x14E8) = 8;
        I32(bio, 0x14EC) = 9;
        F32(bio, 0x141C) = -106.5f;
        I32(bio, 0x14F0) = 10;
        F32(bio, 0x1490) = 0.6f;
        F32(bio, 0x1420) = -21.5f;
        F32(bio, 0x1424) = -5.5f;
        F32(bio, 0x1494) = 1.0f;
        F32(bio, 0x1428) = 33.0f;
        F32(bio, 0x142C) = -5.5f;
        F32(bio, 0x1498) = 1.0f;
        F32(bio, 0x1430) = -21.5f;
        F32(bio, 0x1434) = -5.5f;
        F32(bio, 0x149C) = 1.0f;
        F32(bio, 0x1438) = 33.0f;
        F32(bio, 0x143C) = -5.5f;
        F32(bio, 0x14A0) = 1.0f;
        F32(bio, 0x1440) = -21.5f;
        F32(bio, 0x1444) = -5.5f;
        F32(bio, 0x14A4) = 1.0f;
        F32(bio, 0x1448) = 33.0f;
        F32(bio, 0x144C) = -5.5f;
        F32(bio, 0x14A8) = 1.0f;
        F32(bio, 0x1450) = -69.5f;
        F32(bio, 0x1454) = 9.0f;
        F32(bio, 0x14AC) = 1.0f;
        F32(bio, 0x1458) = 79.0f;
        F32(bio, 0x145C) = 9.0f;
        F32(bio, 0x14B0) = 1.0f;
        Bio_ShowMarkersB(bio);
        U8(bio, 0x150E) = 0;
        U8(bio, 0x150F) = 0;
        U8(bio, 0x1510) = 0;
        U8(bio, 0x1511) = 0;
        U8(bio, 0x1512) = 0;
        I32(bio, 0x1338) = 2;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = -14.0f;
        F32(bio, 0x1340) = -20.0f;
        I32(bio, 0x1364) = 2;
        I32(bio, 0x13A4) = 4;
        I32(bio, 0x13B4) = 4;
        I32(bio, 0x13B8) = 6;
        I32(bio, 0x13BC) = 8;
        I32(bio, 0x13C0) = 10;
        F32(bio, 0x1344) = 29.0f;
        F32(bio, 0x1348) = -20.0f;
        I32(bio, 0x1368) = 3;
        I32(bio, 0x13A8) = 4;
        I32(bio, 0x13C4) = 5;
        I32(bio, 0x13C8) = 7;
        I32(bio, 0x13CC) = 9;
        I32(bio, 0x13D0) = 11;
        return;
    case 5:
        I32(bio, 0x128) = 1;
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x1404) = 5;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x14CC) = 10;
        I32(bio, 0x14D0) = 13;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14D4) = 12;
        I32(bio, 0x14D8) = 11;
        F32(bio, 0x1410) = 79.0f;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 4.0f;
        F32(bio, 0x141C) = -43.5f;
        F32(bio, 0x1490) = 0.48f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        F32(bio, 0x1428) = 6.0f;
        F32(bio, 0x142C) = -106.5f;
        F32(bio, 0x1498) = 0.6f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x174) = 2;
        ClearFlags(bio, 0x198, 0x174);
        F32(bio, 0x178) = -121.0f;
        F32(bio, 0x17C) = 26.0f;
        F32(bio, 0x180) = 126.0f;
        F32(bio, 0x184) = 33.0f;
        return;
    case 6:
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x128) = 1;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x1404) = 5;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14CC) = 10;
        I32(bio, 0x14D0) = 13;
        F32(bio, 0x1410) = 79.0f;
        I32(bio, 0x14D4) = 12;
        I32(bio, 0x14D8) = 11;
        F32(bio, 0x1414) = 9.0f;
        I32(bio, 0x14DC) = 11;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 4.0f;
        F32(bio, 0x141C) = -43.5f;
        F32(bio, 0x1490) = 0.48f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        F32(bio, 0x1428) = 6.0f;
        F32(bio, 0x142C) = -106.5f;
        F32(bio, 0x1498) = 0.6f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 6.0f;
        F32(bio, 0x154) = -106.5f;
        return;
    case 7:
        F32(bio, 0x1408) = 3.5f;
        I32(bio, 0x128) = 2;
        F32(bio, 0x140C) = -14.0f;
        I32(bio, 0x174) = 0;
        I32(bio, 0x1404) = 1;
        F32(bio, 0x1488) = 2.5f;
        I32(bio, 0x14C8) = 11;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = 26.0f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x398) = -81.0f;
        I32(bio, 0x1D0) = 1;
        I32(bio, 0x1DC) = 0;
        F32(bio, 0x39C) = 54.0f;
        I32(bio, 0x1FC) = 21;
        F32(bio, 0x3A0) = -61.0f;
        F32(bio, 0x3A4) = 69.0f;
        F32(bio, 0x3A8) = -29.0f;
        F32(bio, 0x3AC) = 83.0f;
        F32(bio, 0x3B0) = 3.0f;
        F32(bio, 0x3B4) = 87.0f;
        F32(bio, 0x3B8) = 35.0f;
        F32(bio, 0x3BC) = 87.0f;
        F32(bio, 0x3C0) = 68.0f;
        F32(bio, 0x3C4) = 76.0f;
        F32(bio, 0x3C8) = 99.0f;
        F32(bio, 0x3CC) = 54.0f;
        F32(bio, 0x3D0) = 125.0f;
        F32(bio, 0x3D4) = 21.0f;
        F32(bio, 0x3D8) = 129.0f;
        F32(bio, 0x3DC) = -12.0f;
        F32(bio, 0x3E0) = 122.0f;
        F32(bio, 0x3E4) = -40.0f;
        F32(bio, 0x3E8) = 102.0f;
        F32(bio, 0x3EC) = -58.0f;
        F32(bio, 0x3F0) = 75.0f;
        F32(bio, 0x3F4) = -78.0f;
        F32(bio, 0x3F8) = 47.0f;
        F32(bio, 0x3FC) = -82.0f;
        F32(bio, 0x400) = 16.0f;
        F32(bio, 0x404) = -75.0f;
        F32(bio, 0x408) = -16.0f;
        F32(bio, 0x40C) = -58.0f;
        F32(bio, 0x410) = -44.0f;
        F32(bio, 0x414) = -50.0f;
        F32(bio, 0x418) = -72.0f;
        F32(bio, 0x41C) = -65.0f;
        F32(bio, 0x420) = -99.0f;
        F32(bio, 0x424) = -52.0f;
        F32(bio, 0x428) = -123.0f;
        F32(bio, 0x42C) = -19.0f;
        F32(bio, 0x430) = -122.0f;
        F32(bio, 0x434) = 0.0f;
        F32(bio, 0x438) = -99.0f;
        ScaleCuts(bio, sx, sy, 1.21f);
        return;
    case 8:
        I32(bio, 0x128) = 1;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.6f;
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x174) = 2;
        I32(bio, 0x1404) = 6;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x14C8) = 9;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14D0) = 13;
        I32(bio, 0x14D4) = 12;
        F32(bio, 0x1410) = 79.0f;
        I32(bio, 0x14D8) = 11;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 4.0f;
        F32(bio, 0x141C) = -43.5f;
        F32(bio, 0x1490) = 0.48f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        F32(bio, 0x1428) = 6.0f;
        F32(bio, 0x142C) = -106.5f;
        F32(bio, 0x1498) = 0.6f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 6.0f;
        F32(bio, 0x1340) = -106.5f;
        I32(bio, 0x1394) = 11;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13A4) = 1;
        I32(bio, 0x13B4) = 5;
        return;
    case 9:
        I32(bio, 0x128) = 1;
        I32(bio, 0x144) = 0;
        I32(bio, 0x138) = 0;
        U8(bio, 0x1518) = 0;
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x1404) = 4;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x14C8) = 9;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14D0) = 13;
        I32(bio, 0x14D4) = 12;
        F32(bio, 0x1410) = 79.0f;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 4.0f;
        F32(bio, 0x141C) = -43.5f;
        F32(bio, 0x1490) = 0.48f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 4.0f;
        F32(bio, 0x154) = -43.5f;
        return;
    case 10:
        F32(bio, 0x1408) = 20.0f;
        I32(bio, 0x128) = 2;
        F32(bio, 0x140C) = 1.5f;
        I32(bio, 0x174) = 0;
        I32(bio, 0x1404) = 1;
        F32(bio, 0x1488) = 1.42f;
        I32(bio, 0x14C8) = 13;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = 7.0f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x398) = -72.0f;
        I32(bio, 0x1D0) = 1;
        I32(bio, 0x1DC) = 0;
        F32(bio, 0x39C) = 33.0f;
        I32(bio, 0x1FC) = 30;
        F32(bio, 0x3A0) = -91.0f;
        F32(bio, 0x3A4) = 67.0f;
        F32(bio, 0x3A8) = -95.0f;
        F32(bio, 0x3AC) = 99.0f;
        F32(bio, 0x3B0) = -84.0f;
        F32(bio, 0x3B4) = 120.0f;
        F32(bio, 0x3B8) = -60.0f;
        F32(bio, 0x3BC) = 133.0f;
        F32(bio, 0x3C0) = -31.0f;
        F32(bio, 0x3C4) = 145.0f;
        F32(bio, 0x3C8) = 2.0f;
        F32(bio, 0x3CC) = 152.0f;
        F32(bio, 0x3D0) = 35.0f;
        F32(bio, 0x3D4) = 148.0f;
        F32(bio, 0x3D8) = 70.0f;
        F32(bio, 0x3DC) = 140.0f;
        F32(bio, 0x3E0) = 103.0f;
        F32(bio, 0x3E4) = 124.0f;
        F32(bio, 0x3E8) = 134.0f;
        F32(bio, 0x3EC) = 103.0f;
        F32(bio, 0x3F0) = 160.0f;
        F32(bio, 0x3F4) = 78.0f;
        F32(bio, 0x3F8) = 182.0f;
        F32(bio, 0x3FC) = 51.0f;
        F32(bio, 0x400) = 200.0f;
        F32(bio, 0x404) = 19.0f;
        F32(bio, 0x408) = 191.0f;
        F32(bio, 0x40C) = 2.0f;
        F32(bio, 0x410) = 167.0f;
        F32(bio, 0x414) = -22.0f;
        F32(bio, 0x418) = 179.0f;
        F32(bio, 0x41C) = -48.0f;
        F32(bio, 0x420) = 198.0f;
        F32(bio, 0x424) = -77.0f;
        F32(bio, 0x428) = 179.0f;
        F32(bio, 0x42C) = -100.0f;
        F32(bio, 0x430) = 167.0f;
        F32(bio, 0x434) = -118.0f;
        F32(bio, 0x438) = 134.0f;
        F32(bio, 0x43C) = -136.0f;
        F32(bio, 0x440) = 103.0f;
        F32(bio, 0x444) = -143.0f;
        F32(bio, 0x448) = 69.0f;
        F32(bio, 0x44C) = -147.0f;
        F32(bio, 0x450) = 35.0f;
        F32(bio, 0x454) = -142.0f;
        F32(bio, 0x458) = -1.0f;
        F32(bio, 0x45C) = -128.0f;
        F32(bio, 0x460) = -34.0f;
        F32(bio, 0x464) = -111.0f;
        F32(bio, 0x468) = -67.0f;
        F32(bio, 0x46C) = -87.0f;
        F32(bio, 0x470) = -92.0f;
        F32(bio, 0x474) = -52.0f;
        F32(bio, 0x478) = -101.0f;
        F32(bio, 0x47C) = -18.0f;
        F32(bio, 0x480) = -93.0f;
        ScaleCuts(bio, sx, sy, 0.71f);
        return;
    case 0xb:
        I32(bio, 0x174) = 2;
        I32(bio, 0x128) = 1;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.49f;
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x1404) = 4;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x14C8) = 9;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14D0) = 13;
        I32(bio, 0x14D8) = 13;
        F32(bio, 0x1410) = 79.0f;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 4.0f;
        F32(bio, 0x141C) = -43.5f;
        F32(bio, 0x1490) = 0.48f;
        F32(bio, 0x1420) = 4.5f;
        F32(bio, 0x1424) = 11.5f;
        F32(bio, 0x1494) = 0.49f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 4.0f;
        F32(bio, 0x1340) = -43.5f;
        I32(bio, 0x1394) = 13;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13A4) = 1;
        I32(bio, 0x13B4) = 4;
        return;
    case 0xc:
        I32(bio, 0x128) = 1;
        I32(bio, 0x144) = 0;
        I32(bio, 0x138) = 0;
        U8(bio, 0x1518) = 0;
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x1404) = 3;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x14C8) = 9;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x1488) = 1.0f;
        I32(bio, 0x14D4) = 12;
        F32(bio, 0x1410) = 79.0f;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 4.0f;
        F32(bio, 0x141C) = -43.5f;
        F32(bio, 0x1490) = 0.48f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 4.5f;
        F32(bio, 0x154) = 11.5f;
        return;
    case 0xd:
        F32(bio, 0x1408) = 2.0f;
        I32(bio, 0x128) = 2;
        F32(bio, 0x140C) = -5.0f;
        I32(bio, 0x174) = 0;
        I32(bio, 0x1404) = 1;
        F32(bio, 0x1488) = 1.15f;
        I32(bio, 0x14C8) = 12;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = 31.0f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x398) = -85.0f;
        I32(bio, 0x1D0) = 1;
        I32(bio, 0x1DC) = 0;
        F32(bio, 0x39C) = 61.0f;
        I32(bio, 0x1FC) = 24;
        F32(bio, 0x3A0) = -77.0f;
        F32(bio, 0x3A4) = 91.0f;
        F32(bio, 0x3A8) = -64.0f;
        F32(bio, 0x3AC) = 115.0f;
        F32(bio, 0x3B0) = -44.0f;
        F32(bio, 0x3B4) = 132.0f;
        F32(bio, 0x3B8) = -18.0f;
        F32(bio, 0x3BC) = 145.0f;
        F32(bio, 0x3C0) = 10.0f;
        F32(bio, 0x3C4) = 141.0f;
        F32(bio, 0x3C8) = 43.0f;
        F32(bio, 0x3CC) = 122.0f;
        F32(bio, 0x3D0) = 68.0f;
        F32(bio, 0x3D4) = 102.0f;
        F32(bio, 0x3D8) = 91.0f;
        F32(bio, 0x3DC) = 74.0f;
        F32(bio, 0x3E0) = 109.0f;
        F32(bio, 0x3E4) = 43.0f;
        F32(bio, 0x3E8) = 119.0f;
        F32(bio, 0x3EC) = 11.0f;
        F32(bio, 0x3F0) = 123.0f;
        F32(bio, 0x3F4) = -22.0f;
        F32(bio, 0x3F8) = 122.0f;
        F32(bio, 0x3FC) = -52.0f;
        F32(bio, 0x400) = 109.0f;
        F32(bio, 0x404) = -84.0f;
        F32(bio, 0x408) = 99.0f;
        F32(bio, 0x40C) = -117.0f;
        F32(bio, 0x410) = 87.0f;
        F32(bio, 0x414) = -137.0f;
        F32(bio, 0x418) = 64.0f;
        F32(bio, 0x41C) = -142.0f;
        F32(bio, 0x420) = 30.0f;
        F32(bio, 0x424) = -135.0f;
        F32(bio, 0x428) = -2.0f;
        F32(bio, 0x42C) = -119.0f;
        F32(bio, 0x430) = -31.0f;
        F32(bio, 0x434) = -96.0f;
        F32(bio, 0x438) = -53.0f;
        F32(bio, 0x43C) = -69.0f;
        F32(bio, 0x440) = -71.0f;
        F32(bio, 0x444) = -36.0f;
        F32(bio, 0x448) = -83.0f;
        F32(bio, 0x44C) = -1.0f;
        F32(bio, 0x450) = -88.0f;
        ScaleCuts(bio, sx, sy, 0.71f);
        return;
    case 0xe:
        I32(bio, 0x128) = 1;
        I32(bio, 0x174) = 2;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.48f;
        I32(bio, 0x1404) = 3;
        F32(bio, 0x1408) = -69.5f;
        I32(bio, 0x14C8) = 9;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x140C) = 9.0f;
        I32(bio, 0x14D4) = 12;
        F32(bio, 0x1488) = 1.0f;
        F32(bio, 0x1410) = 79.0f;
        F32(bio, 0x1414) = 9.0f;
        F32(bio, 0x148C) = 1.0f;
        F32(bio, 0x1418) = 4.0f;
        F32(bio, 0x141C) = -43.5f;
        F32(bio, 0x1490) = 0.48f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 4.0f;
        F32(bio, 0x1340) = -43.5f;
        I32(bio, 0x1394) = 13;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13A4) = 1;
        I32(bio, 0x13B4) = 2;
        return;
    case 0xf:
        I32(bio, 0x128) = 1;
        F32(bio, 0x1408) = -139.5f;
        I32(bio, 0x1404) = 2;
        I32(bio, 0x14C8) = 9;
        F32(bio, 0x140C) = -28.0f;
        I32(bio, 0x14CC) = 10;
        F32(bio, 0x1488) = 1.0f;
        F32(bio, 0x1410) = 110.5f;
        F32(bio, 0x1414) = -32.0f;
        F32(bio, 0x148C) = 1.0f;
    }
  return;
}
BIOANIMALS45_REVERSED("ClassBiology::BatReset", 0x00718980, &BatReset);

// (thiscall) the frog's (stages 0..0xF)
void __fastcall FrogReset(uint8_t* bio, re::Edx) {
    float sx = screenScaleXBio;
    float sy = screenScaleYBio;
    (void)sx;
    (void)sy;
    switch (I32(bio, 0x110)) {
    case 0:
        I32(bio, 0x128) = 0;
        I32(bio, 0x174) = 4;
        ClearFlags(bio, 0x198, 0x174);
        F32(bio, 0x178) = -54.0f;
        F32(bio, 0x17C) = -120.0f;
        F32(bio, 0x180) = 61.0f;
        F32(bio, 0x184) = -124.0f;
        F32(bio, 0x188) = -32.0f;
        F32(bio, 0x18C) = 135.0f;
        F32(bio, 0x190) = 29.0f;
        F32(bio, 0x194) = 122.5f;
        return;
    case 1:
        F32(bio, 0x1408) = -32.0f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x1404) = 2;
        F32(bio, 0x140C) = -6.0f;
        I32(bio, 0x14C8) = 6;
        I32(bio, 0x14CC) = 7;
        F32(bio, 0x1488) = 0.63f;
        F32(bio, 0x1410) = 39.5f;
        F32(bio, 0x1414) = -8.0f;
        F32(bio, 0x148C) = 0.63f;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = -0.313709f;
        I32(bio, 0x1CC) = 1;
        I32(bio, 0x1D0) = 0;
        F32(bio, 0x398) = -160.3879f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 10;
        F32(bio, 0x39C) = -0.313709f;
        F32(bio, 0x3A0) = -126.22977f;
        F32(bio, 0x3A4) = -0.313709f;
        F32(bio, 0x3A8) = -92.07182f;
        F32(bio, 0x3AC) = -0.313709f;
        F32(bio, 0x3B0) = -57.913765f;
        F32(bio, 0x3B4) = -0.313709f;
        F32(bio, 0x3B8) = -23.75572f;
        F32(bio, 0x3BC) = -0.313709f;
        F32(bio, 0x3C0) = 10.402324f;
        F32(bio, 0x3C4) = -0.313709f;
        F32(bio, 0x3C8) = 44.56044f;
        F32(bio, 0x3CC) = -0.313709f;
        F32(bio, 0x3D0) = 78.71858f;
        F32(bio, 0x3D4) = -0.313709f;
        F32(bio, 0x3D8) = 112.87647f;
        F32(bio, 0x3DC) = -0.313709f;
        F32(bio, 0x3E0) = 147.03441f;   // (the shared tail)
        Bio_ResetCuts(bio, 320.0f);
        return;
    case 2:
        F32(bio, 0x1408) = -32.0f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x1404) = 2;
        F32(bio, 0x140C) = -6.0f;
        I32(bio, 0x14C8) = 6;
        I32(bio, 0x14CC) = 7;
        F32(bio, 0x1488) = 0.63f;
        F32(bio, 0x1410) = 39.5f;
        F32(bio, 0x1414) = -8.0f;
        F32(bio, 0x148C) = 0.63f;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = -82.11231f;
        I32(bio, 0x1CC) = 1;
        I32(bio, 0x1D0) = 0;
        F32(bio, 0x398) = -93.81425f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 10;
        F32(bio, 0x39C) = -71.758446f;
        F32(bio, 0x3A0) = -117.62815f;
        F32(bio, 0x3A4) = -57.263058f;
        F32(bio, 0x3A8) = -138.33588f;
        F32(bio, 0x3AC) = -36.555264f;
        F32(bio, 0x3B0) = -151.79591f;
        F32(bio, 0x3B4) = -10.670597f;
        F32(bio, 0x3B8) = -156.97276f;
        F32(bio, 0x3BC) = 15.21407f;
        F32(bio, 0x3C0) = -155.93748f;
        F32(bio, 0x3C4) = 41.098732f;
        F32(bio, 0x3C8) = -147.6544f;
        F32(bio, 0x3CC) = 61.80648f;
        F32(bio, 0x3D0) = -132.12358f;
        F32(bio, 0x3D4) = 76.30185f;
        F32(bio, 0x3D8) = -115.55736f;
        F32(bio, 0x3DC) = 88.726555f;
        F32(bio, 0x3E0) = -94.849655f;   // (the shared tail)
        Bio_ResetCuts(bio, 320.0f);
        return;
    case 3:
        F32(bio, 0x1408) = -32.0f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x1404) = 2;
        F32(bio, 0x140C) = -6.0f;
        I32(bio, 0x14C8) = 6;
        I32(bio, 0x14CC) = 7;
        F32(bio, 0x1488) = 0.63f;
        F32(bio, 0x1410) = 39.5f;
        F32(bio, 0x1414) = -8.0f;
        F32(bio, 0x148C) = 0.63f;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = -80.32057f;
        I32(bio, 0x1CC) = 1;
        F32(bio, 0x398) = 107.13288f;
        I32(bio, 0x1D0) = 0;
        I32(bio, 0x1DC) = 0;
        F32(bio, 0x39C) = -61.301388f;
        I32(bio, 0x1FC) = 8;
        F32(bio, 0x3A0) = 128.26529f;
        F32(bio, 0x3A4) = -35.942566f;
        F32(bio, 0x3A8) = 136.71825f;
        F32(bio, 0x3AC) = -7.41381f;
        F32(bio, 0x3B0) = 143.05797f;
        F32(bio, 0x3B4) = 22.171547f;
        F32(bio, 0x3B8) = 142.0014f;
        F32(bio, 0x3BC) = 50.70029f;
        F32(bio, 0x3C0) = 134.60501f;
        F32(bio, 0x3C4) = 76.05918f;
        F32(bio, 0x3C8) = 121.925545f;
        F32(bio, 0x3CC) = 82.398895f;
        F32(bio, 0x3D0) = 99.73654f;
        Bio_ResetCuts(bio, 320.0f);
        return;
    case 4:
        F32(bio, 0x1408) = 3.0f;
        F32(bio, 0x140C) = 60.0f;
        I32(bio, 0x128) = 1;
        F32(bio, 0x1488) = 0.715f;
        I32(bio, 0x1404) = 11;
        I32(bio, 0x14C8) = 5;
        F32(bio, 0x1410) = 12.0f;
        I32(bio, 0x14CC) = 4;
        I32(bio, 0x14D0) = 3;
        F32(bio, 0x1414) = 4.0f;
        I32(bio, 0x14D4) = 6;
        I32(bio, 0x14D8) = 7;
        F32(bio, 0x148C) = 0.705f;
        I32(bio, 0x14DC) = 8;
        I32(bio, 0x14E0) = 9;
        F32(bio, 0x1418) = 1.5f;
        I32(bio, 0x14E4) = 10;
        I32(bio, 0x14E8) = 11;
        F32(bio, 0x141C) = -87.0f;
        I32(bio, 0x14EC) = 12;
        I32(bio, 0x14F0) = 13;
        F32(bio, 0x1490) = 0.505f;
        F32(bio, 0x1420) = -32.0f;
        F32(bio, 0x1424) = -6.0f;
        F32(bio, 0x1494) = 0.63f;
        F32(bio, 0x1428) = 39.5f;
        F32(bio, 0x142C) = -8.0f;
        F32(bio, 0x1498) = 0.63f;
        F32(bio, 0x1430) = -32.0f;
        F32(bio, 0x1434) = -6.0f;
        F32(bio, 0x149C) = 0.63f;
        F32(bio, 0x1438) = 39.5f;
        F32(bio, 0x143C) = -8.0f;
        F32(bio, 0x14A0) = 0.63f;
        F32(bio, 0x1440) = -32.0f;
        F32(bio, 0x1444) = -6.0f;
        F32(bio, 0x14A4) = 0.63f;
        F32(bio, 0x1448) = 39.5f;
        F32(bio, 0x144C) = -8.0f;
        F32(bio, 0x14A8) = 0.63f;
        F32(bio, 0x1450) = -130.5f;
        F32(bio, 0x1454) = -11.5f;
        F32(bio, 0x14AC) = 0.63f;
        F32(bio, 0x1458) = 123.5f;
        F32(bio, 0x145C) = -13.5f;
        F32(bio, 0x14B0) = 0.63f;
        Bio_ShowMarkersB(bio);
        U8(bio, 0x150D) = 0;
        U8(bio, 0x150E) = 0;
        U8(bio, 0x150F) = 0;
        U8(bio, 0x1510) = 0;
        U8(bio, 0x1511) = 0;
        U8(bio, 0x1512) = 0;
        I32(bio, 0x1338) = 2;
        ClearFlags(bio, 0x135C, 0x1338);
        I32(bio, 0x13A4) = 4;
        F32(bio, 0x133C) = -25.5f;
        I32(bio, 0x13B4) = 3;
        I32(bio, 0x13B8) = 5;
        F32(bio, 0x1340) = 2.5f;
        I32(bio, 0x1364) = 2;
        I32(bio, 0x13BC) = 7;
        F32(bio, 0x1344) = 27.0f;
        I32(bio, 0x13C0) = 9;
        F32(bio, 0x1348) = 2.5f;
        I32(bio, 0x1368) = 3;
        I32(bio, 0x13A8) = 4;
        I32(bio, 0x13C4) = 4;
        I32(bio, 0x13C8) = 6;
        I32(bio, 0x13CC) = 8;
        I32(bio, 0x13D0) = 10;
        return;
    case 5:
        F32(bio, 0x1408) = -130.5f;
        I32(bio, 0x128) = 1;
        I32(bio, 0x1404) = 5;
        F32(bio, 0x140C) = -11.5f;
        I32(bio, 0x14C8) = 12;
        I32(bio, 0x14CC) = 13;
        F32(bio, 0x1488) = 0.63f;
        I32(bio, 0x14D0) = 5;
        I32(bio, 0x14D4) = 4;
        F32(bio, 0x1410) = 123.5f;
        I32(bio, 0x14D8) = 3;
        F32(bio, 0x1414) = -13.5f;
        F32(bio, 0x148C) = 0.63f;
        F32(bio, 0x1418) = 3.0f;
        F32(bio, 0x141C) = 60.0f;
        F32(bio, 0x1490) = 0.715f;
        F32(bio, 0x1420) = 12.0f;
        F32(bio, 0x1424) = 4.0f;
        F32(bio, 0x1494) = 0.705f;
        F32(bio, 0x1428) = 1.5f;
        F32(bio, 0x142C) = -87.0f;
        F32(bio, 0x1498) = 0.505f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x174) = 2;
        ClearFlags(bio, 0x198, 0x174);
        F32(bio, 0x178) = -134.0f;
        F32(bio, 0x17C) = -20.0f;
        F32(bio, 0x180) = 122.0f;
        F32(bio, 0x184) = -22.0f;
        return;
    case 6:
        F32(bio, 0x1408) = -130.5f;
        I32(bio, 0x128) = 1;
        F32(bio, 0x140C) = -11.5f;
        I32(bio, 0x1404) = 5;
        I32(bio, 0x14C8) = 12;
        F32(bio, 0x1488) = 0.63f;
        I32(bio, 0x14CC) = 13;
        I32(bio, 0x14D0) = 5;
        F32(bio, 0x1410) = 123.5f;
        I32(bio, 0x14D4) = 4;
        I32(bio, 0x14D8) = 3;
        F32(bio, 0x1414) = -13.5f;
        F32(bio, 0x148C) = 0.63f;
        F32(bio, 0x1418) = 3.0f;
        F32(bio, 0x141C) = 60.0f;
        F32(bio, 0x1490) = 0.715f;
        F32(bio, 0x1420) = 12.0f;
        F32(bio, 0x1424) = 4.0f;
        F32(bio, 0x1494) = 0.705f;
        F32(bio, 0x1428) = 1.5f;
        F32(bio, 0x142C) = -87.0f;
        F32(bio, 0x1498) = 0.505f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 1.5f;
        F32(bio, 0x154) = -87.0f;
        return;
    case 7:
        F32(bio, 0x1408) = 5.5f;
        I32(bio, 0x128) = 2;
        F32(bio, 0x140C) = -17.5f;
        I32(bio, 0x174) = 0;
        I32(bio, 0x1404) = 1;
        F32(bio, 0x1488) = 1.16f;
        I32(bio, 0x14C8) = 3;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = 19.844976f;
        I32(bio, 0x1CC) = 1;
        I32(bio, 0x1D0) = 1;
        F32(bio, 0x398) = -100.29941f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 22;
        F32(bio, 0x39C) = 48.24021f;
        F32(bio, 0x3A0) = -90.834335f;
        F32(bio, 0x3A4) = 71.902954f;
        F32(bio, 0x3A8) = -73.79711f;
        F32(bio, 0x3AC) = 83.26102f;
        F32(bio, 0x3B0) = -49.187923f;
        F32(bio, 0x3B4) = 82.31458f;
        F32(bio, 0x3B8) = -20.79266f;
        F32(bio, 0x3BC) = 73.79589f;
        F32(bio, 0x3C0) = 5.709547f;
        F32(bio, 0x3C4) = 77.58197f;
        F32(bio, 0x3C8) = 35.99783f;
        F32(bio, 0x3CC) = 74.74251f;
        F32(bio, 0x3D0) = 64.39303f;
        F32(bio, 0x3D4) = 64.33087f;
        F32(bio, 0x3D8) = 94.68135f;
        F32(bio, 0x3DC) = 49.186745f;
        F32(bio, 0x3E0) = 120.23698f;
        F32(bio, 0x3E4) = 19.844976f;
        F32(bio, 0x3E8) = 124.02326f;
        F32(bio, 0x3EC) = -6.657248f;
        F32(bio, 0x3F0) = 112.66491f;
        F32(bio, 0x3F4) = -29.373432f;
        F32(bio, 0x3F8) = 93.73482f;
        F32(bio, 0x3FC) = -49.250114f;
        F32(bio, 0x400) = 71.965126f;
        F32(bio, 0x404) = -63.447685f;
        F32(bio, 0x408) = 46.409405f;
        F32(bio, 0x40C) = -66.28727f;
        F32(bio, 0x410) = 18.96067f;
        F32(bio, 0x414) = -85.21743f;
        F32(bio, 0x418) = -1.862518f;
        F32(bio, 0x41C) = -83.32445f;
        F32(bio, 0x420) = -31.204254f;
        F32(bio, 0x424) = -71.96631f;
        F32(bio, 0x428) = -58.65303f;
        F32(bio, 0x42C) = -56.82218f;
        F32(bio, 0x430) = -80.42271f;
        F32(bio, 0x434) = -35.052498f;
        F32(bio, 0x438) = -98.40638f;
        F32(bio, 0x43C) = -9.496765f;
        F32(bio, 0x440) = -102.19248f;
        Bio_ResetCuts(bio, 320.0f);
        return;
    case 8:
        I32(bio, 0x128) = 1;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.505f;
        F32(bio, 0x1408) = -130.5f;
        F32(bio, 0x140C) = -11.5f;
        I32(bio, 0x174) = 2;
        I32(bio, 0x1404) = 5;
        F32(bio, 0x1488) = 0.63f;
        I32(bio, 0x14C8) = 12;
        I32(bio, 0x14CC) = 13;
        F32(bio, 0x1410) = 123.5f;
        I32(bio, 0x14D0) = 5;
        I32(bio, 0x14D4) = 4;
        F32(bio, 0x1414) = -13.5f;
        I32(bio, 0x14D8) = 3;
        F32(bio, 0x148C) = 0.63f;
        F32(bio, 0x1418) = 3.0f;
        F32(bio, 0x141C) = 60.0f;
        F32(bio, 0x1490) = 0.715f;
        F32(bio, 0x1420) = 12.0f;
        F32(bio, 0x1424) = 4.0f;
        F32(bio, 0x1494) = 0.705f;
        F32(bio, 0x1428) = 1.5f;
        F32(bio, 0x142C) = -87.0f;
        F32(bio, 0x1498) = 0.505f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 1.5f;
        F32(bio, 0x1340) = -87.0f;
        I32(bio, 0x1394) = 3;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13B4) = 4;
        I32(bio, 0x13A4) = 1;
        return;
    case 9:
        I32(bio, 0x128) = 1;
        I32(bio, 0x144) = 0;
        I32(bio, 0x138) = 0;
        U8(bio, 0x1518) = 0;
        F32(bio, 0x1408) = -130.5f;
        F32(bio, 0x140C) = -11.5f;
        I32(bio, 0x1404) = 4;
        I32(bio, 0x14C8) = 12;
        F32(bio, 0x1488) = 0.63f;
        I32(bio, 0x14CC) = 13;
        I32(bio, 0x14D0) = 5;
        F32(bio, 0x1410) = 123.5f;
        I32(bio, 0x14D4) = 4;
        F32(bio, 0x1414) = -13.5f;
        F32(bio, 0x148C) = 0.63f;
        F32(bio, 0x1418) = 3.0f;
        F32(bio, 0x141C) = 60.0f;
        F32(bio, 0x1490) = 0.715f;
        F32(bio, 0x1420) = 12.0f;
        F32(bio, 0x1424) = 4.0f;
        F32(bio, 0x1494) = 0.705f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 12.0f;
        F32(bio, 0x154) = 4.0f;
        return;
    case 10:
        F32(bio, 0x1408) = 20.0f;
        I32(bio, 0x128) = 2;
        F32(bio, 0x140C) = 32.5f;
        I32(bio, 0x174) = 0;
        I32(bio, 0x1404) = 1;
        F32(bio, 0x1488) = 1.48f;
        I32(bio, 0x14C8) = 4;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = 41.80487f;
        I32(bio, 0x1CC) = 1;
        I32(bio, 0x1D0) = 1;
        F32(bio, 0x398) = -151.01402f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 42;
        F32(bio, 0x39C) = 63.4124f;
        F32(bio, 0x3A0) = -132.73064f;
        F32(bio, 0x3A4) = 81.69578f;
        F32(bio, 0x3A8) = -114.4472f;
        F32(bio, 0x3AC) = 106.627686f;
        F32(bio, 0x3B0) = -106.13647f;
        F32(bio, 0x3B4) = 130.72838f;
        F32(bio, 0x3B8) = -97.82598f;
        F32(bio, 0x3BC) = 156.4914f;
        F32(bio, 0x3C0) = -89.51528f;
        F32(bio, 0x3C4) = 175.60571f;
        F32(bio, 0x3C8) = -71.23202f;
        F32(bio, 0x3CC) = 188.90263f;
        F32(bio, 0x3D0) = -47.131252f;
        F32(bio, 0x3D4) = 193.88905f;
        F32(bio, 0x3D8) = -22.19936f;
        F32(bio, 0x3DC) = 200.5376f;
        F32(bio, 0x3E0) = 2.732469f;
        F32(bio, 0x3E4) = 190.56473f;
        F32(bio, 0x3E8) = 26.00219f;
        F32(bio, 0x3EC) = 176.4367f;
        F32(bio, 0x3F0) = 49.271935f;
        F32(bio, 0x3F4) = 159.81554f;
        F32(bio, 0x3F8) = 70.87943f;
        F32(bio, 0x3FC) = 139.87009f;
        F32(bio, 0x400) = 87.50076f;
        F32(bio, 0x404) = 116.600334f;
        F32(bio, 0x408) = 100.79776f;
        F32(bio, 0x40C) = 90.837425f;
        F32(bio, 0x410) = 109.108406f;
        F32(bio, 0x414) = 65.074615f;
        F32(bio, 0x418) = 117.418976f;
        F32(bio, 0x41C) = 37.64951f;
        F32(bio, 0x420) = 122.40536f;
        F32(bio, 0x424) = 13.54874f;
        F32(bio, 0x428) = 127.39163f;
        F32(bio, 0x42C) = -13.045218f;
        F32(bio, 0x430) = 128.22273f;
        F32(bio, 0x434) = -38.808132f;
        F32(bio, 0x438) = 128.22273f;
        F32(bio, 0x43C) = -64.57113f;
        F32(bio, 0x440) = 124.06742f;
        F32(bio, 0x444) = -91.16504f;
        F32(bio, 0x448) = 117.418976f;
        F32(bio, 0x44C) = -113.60363f;
        F32(bio, 0x450) = 102.459785f;
        F32(bio, 0x454) = -135.21138f;
        F32(bio, 0x458) = 84.176605f;
        F32(bio, 0x45C) = -156.81888f;
        F32(bio, 0x460) = 64.23107f;
        F32(bio, 0x464) = -156.81888f;
        F32(bio, 0x468) = 37.637074f;
        F32(bio, 0x46C) = -136.04236f;
        F32(bio, 0x470) = 21.846878f;
        F32(bio, 0x474) = -109.44834f;
        F32(bio, 0x478) = 17.691586f;
        F32(bio, 0x47C) = -82.85446f;
        F32(bio, 0x480) = 18.522642f;
        F32(bio, 0x484) = -57.922493f;
        F32(bio, 0x488) = 16.029451f;
        F32(bio, 0x48C) = -31.328588f;
        F32(bio, 0x490) = 11.043082f;
        F32(bio, 0x494) = -4.734621f;
        F32(bio, 0x498) = 2.732469f;
        F32(bio, 0x49C) = 23.521502f;
        F32(bio, 0x4A0) = -3.916022f;
        F32(bio, 0x4A4) = 49.28437f;
        F32(bio, 0x4A8) = -13.888764f;
        F32(bio, 0x4AC) = 73.38515f;
        F32(bio, 0x4B0) = -22.19936f;
        F32(bio, 0x4B4) = 96.654884f;
        F32(bio, 0x4B8) = -30.510004f;
        F32(bio, 0x4BC) = 85.85108f;
        F32(bio, 0x4C0) = -53.779713f;
        F32(bio, 0x4C4) = 61.750313f;
        F32(bio, 0x4C8) = -65.41457f;
        F32(bio, 0x4CC) = 41.80487f;
        F32(bio, 0x4D0) = -82.86689f;
        F32(bio, 0x4D4) = 25.18357f;
        F32(bio, 0x4D8) = -107.79865f;
        F32(bio, 0x4DC) = 20.197235f;
        F32(bio, 0x4E0) = -136.05484f;
        Bio_ResetCuts(bio, 320.0f);
        return;
    case 0xb:
        I32(bio, 0x174) = 2;
        I32(bio, 0x128) = 1;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.705f;
        F32(bio, 0x1408) = -130.5f;
        I32(bio, 0x1404) = 4;
        F32(bio, 0x140C) = -11.5f;
        I32(bio, 0x14C8) = 12;
        I32(bio, 0x14CC) = 13;
        F32(bio, 0x1488) = 0.63f;
        I32(bio, 0x14D0) = 5;
        I32(bio, 0x14D4) = 4;
        F32(bio, 0x1410) = 123.5f;
        F32(bio, 0x1414) = -13.5f;
        F32(bio, 0x148C) = 0.63f;
        F32(bio, 0x1418) = 3.0f;
        F32(bio, 0x141C) = 60.0f;
        F32(bio, 0x1490) = 0.715f;
        F32(bio, 0x1420) = 12.0f;
        F32(bio, 0x1424) = 4.0f;
        F32(bio, 0x1494) = 0.705f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 12.0f;
        F32(bio, 0x1340) = 4.0f;
        I32(bio, 0x1394) = 4;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13A4) = 1;
        I32(bio, 0x13B4) = 3;
        return;
    case 0xc:
        I32(bio, 0x128) = 1;
        I32(bio, 0x144) = 0;
        I32(bio, 0x138) = 0;
        U8(bio, 0x1518) = 0;
        F32(bio, 0x1408) = -130.5f;
        I32(bio, 0x1404) = 3;
        F32(bio, 0x140C) = -11.5f;
        I32(bio, 0x14C8) = 12;
        I32(bio, 0x14CC) = 13;
        F32(bio, 0x1488) = 0.63f;
        I32(bio, 0x14D0) = 5;
        F32(bio, 0x1410) = 123.5f;
        F32(bio, 0x1414) = -13.5f;
        F32(bio, 0x148C) = 0.63f;
        F32(bio, 0x1418) = 3.0f;
        F32(bio, 0x141C) = 60.0f;
        F32(bio, 0x1490) = 0.715f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x14C) = 1;
        ClearFlags(bio, 0x170, 0x14C);
        F32(bio, 0x150) = 3.0f;
        F32(bio, 0x154) = 60.0f;
        return;
    case 0xd:
        F32(bio, 0x1408) = 20.0f;
        I32(bio, 0x128) = 2;
        F32(bio, 0x140C) = -14.0f;
        I32(bio, 0x174) = 0;
        I32(bio, 0x1404) = 1;
        F32(bio, 0x1488) = 1.28f;
        I32(bio, 0x14C8) = 5;
        Bio_ShowMarkersB(bio);
        F32(bio, 0x394) = 33.16276f;
        I32(bio, 0x1CC) = 1;
        I32(bio, 0x1D0) = 1;
        F32(bio, 0x398) = -101.563866f;
        I32(bio, 0x1DC) = 0;
        I32(bio, 0x1FC) = 36;
        F32(bio, 0x39C) = 54.659573f;
        F32(bio, 0x3A0) = -97.857506f;
        F32(bio, 0x3A4) = 79.12154f;
        F32(bio, 0x3A8) = -98.598724f;
        F32(bio, 0x3AC) = 104.32471f;
        F32(bio, 0x3B0) = -93.40979f;
        F32(bio, 0x3B4) = 127.30406f;
        F32(bio, 0x3B8) = -90.44483f;
        F32(bio, 0x3BC) = 153.98994f;
        F32(bio, 0x3C0) = -88.22097f;
        F32(bio, 0x3C4) = 173.26295f;
        F32(bio, 0x3C8) = -72.654335f;
        F32(bio, 0x3CC) = 173.26295f;
        F32(bio, 0x3D0) = -48.93371f;
        F32(bio, 0x3D4) = 166.59142f;
        F32(bio, 0x3D8) = -24.471756f;
        F32(bio, 0x3DC) = 151.76596f;
        F32(bio, 0x3E0) = -4.457456f;
        F32(bio, 0x3E4) = 136.94054f;
        F32(bio, 0x3E8) = 15.55687f;
        F32(bio, 0x3EC) = 113.219986f;
        F32(bio, 0x3F0) = 23.710815f;
        F32(bio, 0x3F4) = 91.72314f;
        F32(bio, 0x3F8) = 33.347343f;
        F32(bio, 0x3FC) = 78.3803f;
        F32(bio, 0x400) = 51.879128f;
        F32(bio, 0x404) = 77.638985f;
        F32(bio, 0x408) = 76.34111f;
        F32(bio, 0x40C) = 73.191284f;
        F32(bio, 0x410) = 102.28541f;
        F32(bio, 0x414) = 61.331024f;
        F32(bio, 0x418) = 123.04105f;
        F32(bio, 0x41C) = 37.610382f;
        F32(bio, 0x420) = 120.07597f;
        F32(bio, 0x424) = 32.421497f;
        F32(bio, 0x428) = 97.837906f;
        F32(bio, 0x42C) = 16.113523f;
        F32(bio, 0x430) = 80.78859f;
        F32(bio, 0x434) = -6.124584f;
        F32(bio, 0x438) = 68.92833f;
        F32(bio, 0x43C) = -29.845255f;
        F32(bio, 0x440) = 60.77433f;
        F32(bio, 0x444) = -55.7897f;
        F32(bio, 0x448) = 54.844208f;
        F32(bio, 0x44C) = -80.25159f;
        F32(bio, 0x450) = 45.948933f;
        F32(bio, 0x454) = -95.8183f;
        F32(bio, 0x458) = 28.158443f;
        F32(bio, 0x45C) = -116.573906f;
        F32(bio, 0x460) = 11.850496f;
        F32(bio, 0x464) = -130.65807f;
        F32(bio, 0x468) = -10.387623f;
        F32(bio, 0x46C) = -135.1056f;
        F32(bio, 0x470) = -36.3321f;
        F32(bio, 0x474) = -136.5882f;
        F32(bio, 0x478) = -60.05274f;
        F32(bio, 0x47C) = -132.88174f;
        F32(bio, 0x480) = -88.96232f;
        F32(bio, 0x484) = -116.573906f;
        F32(bio, 0x488) = -111.94166f;
        F32(bio, 0x48C) = -92.11195f;
        F32(bio, 0x490) = -122.31955f;
        F32(bio, 0x494) = -64.68494f;
        F32(bio, 0x498) = -121.57825f;
        F32(bio, 0x49C) = -38.740463f;
        F32(bio, 0x4A0) = -119.35433f;
        F32(bio, 0x4A4) = -15.019814f;
        F32(bio, 0x4A8) = -114.16551f;
        F32(bio, 0x4AC) = 10.183367f;
        F32(bio, 0x4B0) = -108.235214f;
        Bio_ResetCuts(bio, 320.0f);
        return;
    case 0xe:
        I32(bio, 0x128) = 1;
        I32(bio, 0x174) = 2;
        U8(bio, 0x1518) = 1;
        U8(bio, 0x1524) = 0;
        F32(bio, 0x152C) = 0.715f;
        F32(bio, 0x1408) = -130.5f;
        I32(bio, 0x1404) = 3;
        I32(bio, 0x14C8) = 12;
        F32(bio, 0x140C) = -11.5f;
        I32(bio, 0x14CC) = 13;
        I32(bio, 0x14D0) = 5;
        F32(bio, 0x1488) = 0.63f;
        F32(bio, 0x1410) = 123.5f;
        F32(bio, 0x1414) = -13.5f;
        F32(bio, 0x148C) = 0.63f;
        F32(bio, 0x1418) = 3.0f;
        F32(bio, 0x141C) = 60.0f;
        F32(bio, 0x1490) = 0.715f;
        Bio_ShowMarkersB(bio);
        I32(bio, 0x1338) = 1;
        ClearFlags(bio, 0x135C, 0x1338);
        F32(bio, 0x133C) = 3.0f;
        F32(bio, 0x1340) = 60.0f;
        I32(bio, 0x1394) = 5;
        I32(bio, 0x1364) = 5;
        I32(bio, 0x13A4) = 1;
        I32(bio, 0x13B4) = 2;
        return;
    case 0xf:
        I32(bio, 0x128) = 1;
        F32(bio, 0x1408) = -139.5f;
        I32(bio, 0x1404) = 2;
        I32(bio, 0x14C8) = 12;
        F32(bio, 0x140C) = -28.0f;
        I32(bio, 0x14CC) = 13;
        F32(bio, 0x1488) = 0.63f;
        F32(bio, 0x1410) = 110.5f;
        F32(bio, 0x1414) = -32.0f;
        F32(bio, 0x148C) = 0.63f;
        return;
    default:
        return;
    }
}
BIOANIMALS45_REVERSED("ClassBiology::FrogReset", 0x0071A9E0, &FrogReset);

} // namespace
