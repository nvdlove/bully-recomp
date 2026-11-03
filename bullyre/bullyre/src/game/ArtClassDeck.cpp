// Art class (MGClassArt): the deck, the grid the player draws lines across to fill
// (0x6DA010..0x6DADBA), and MGClassArt's defaults. The game itself (the objects, the
// drawing, the rules) is ArtClassArcade.cpp's and ArtClassObjects.cpp's.
//
// The deck (by offset): +0xC the cells across (and down), +0x10 the cells (uint16: bit
// 0 filled, bits 1..3 the lines out of the vertex (1 left, 2 up, 4 right, 8 down), the
// same << 4 the line being drawn, 0x100 the flood fill's mark), +0x14 / +0x18 the
// cursor, +0x1C / +0x1D its half steps, +0x20 a direction (5 none), +0x24 drawing, +0x25
// a line closed, +0x28 / +0x2C where the line started, +0x30 / +0x34 the cursor then,
// +0x38 its first direction, +0x48 / +0x4C the flood fill's stack (pairs) and depth.

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_FN(const uint16_t*, __thiscall, ACD_Text_Get, 0x0068FE80, void* text, const char* key);
inline int __cdecl ACD_Rand() { return crt::Rand(); }

uint32_t Size(uint8_t* deck) {
    return At<uint32_t>(deck, 0xC);
}
uint16_t* Cells(uint8_t* deck) {
    return At<uint16_t*>(deck, 0x10);
}
uint16_t& Cell(uint8_t* deck, uint32_t col, uint32_t row) {
    return Cells(deck)[Size(deck) * row + col];
}

// the deck's size as a float (an unsigned value: fild, +2^32 when the sign bit's set)
float SizeF(uint8_t* deck) {
    int32_t size = At<int32_t>(deck, 0xC);
    double value = double(size);
    if (size < 0) {
        value = double(float(value + 4294967296.0));
    }
    return float(value);
}

// the x87's (chopped) conversion to a 64-bit integer, its low dword
int32_t Chop(float value) {
    return int32_t(int64_t(value));
}

} // namespace

#define ARTDECK_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// 0x6DA010 (thiscall): MGClassArt's two power-up icons (+0x5F8, +0x608) told to go
void __fastcall MGClassArt_DropIcons(uint8_t* self, re::Edx) {
    if (At<uint8_t*>(self, 0x5F8) != nullptr) {
        At<uint8_t>(At<uint8_t*>(self, 0x5F8), 0x5C) = 1;
        At<uint8_t*>(self, 0x5F8) = nullptr;
    }
    if (At<uint8_t*>(self, 0x608) != nullptr) {
        At<uint8_t>(At<uint8_t*>(self, 0x608), 0x5C) = 1;
        At<uint8_t*>(self, 0x608) = nullptr;
    }
}
ARTDECK_REVERSED("MGClassArt::DropIcons", 0x006DA010, &MGClassArt_DropIcons);

// 0x6DA040 (thiscall): MGClassArt's defaults: its texts and settings
void __fastcall MGClassArt_SetDefaults(uint8_t* self, re::Edx) {
    At<const uint16_t*>(self, 0xC4) = ACD_Text_Get(TheText, "MGCA_initial_text");
    At<const uint16_t*>(self, 0xCC) = ACD_Text_Get(TheText, "MGCA_exit_text");
    At<const uint16_t*>(self, 0xC8) = ACD_Text_Get(TheText, "MGCA_killed_text");
    At<const uint16_t*>(self, 0xD0) = ACD_Text_Get(TheText, "MGCA_speedboost_name");
    At<const uint16_t*>(self, 0xD4) = ACD_Text_Get(TheText, "MGCA_freeze_name");
    At<const uint16_t*>(self, 0xD8) = ACD_Text_Get(TheText, "MGCA_reversal_name");
    At<const uint16_t*>(self, 0xDC) = ACD_Text_Get(TheText, "MGCA_instructions_heading");
    At<const uint16_t*>(self, 0xE0) = ACD_Text_Get(TheText, "MGCA_instructions_body");
    At<const uint16_t*>(self, 0xE4) = ACD_Text_Get(TheText, "MGCA_instructions_powerups_heading");
    At<const uint16_t*>(self, 0xE8) = ACD_Text_Get(TheText, "MGCA_instructions_speedboost_desc");
    At<const uint16_t*>(self, 0xEC) = ACD_Text_Get(TheText, "MGCA_instructions_reversal_desc");
    At<const uint16_t*>(self, 0xF0) = ACD_Text_Get(TheText, "MGCA_instructions_clear_desc");
    At<const uint16_t*>(self, 0xF4) = ACD_Text_Get(TheText, "MGCA_instructions_freeze_desc");
    At<const uint16_t*>(self, 0xF8) = ACD_Text_Get(TheText, "MGCA_instructions_enemies_heading");
    At<const uint16_t*>(self, 0xFC) = ACD_Text_Get(TheText, "MGCA_instructions_floater_bouncer_desc");
    At<const uint16_t*>(self, 0x100) = ACD_Text_Get(TheText, "MGCA_instructions_crawler_fastcrawler_desc");
    At<const uint16_t*>(self, 0x104) = ACD_Text_Get(TheText, "MGCA_instructions_exploder_desc");
    At<const uint16_t*>(self, 0x108) = ACD_Text_Get(TheText, "MGCA_instructions_return_to_game_text");
    At<float>(self, 0x110) = 90.0f;
    At<float>(self, 0x114) = 0.1666f;
    At<int32_t>(self, 0x10C) = 0;
    At<float>(self, 0x118) = 1.5f;
    At<float>(self, 0x11C) = 0.15f;
    At<float>(self, 0x120) = 0.2f;
    At<float>(self, 0x124) = 0.2f;
    At<float>(self, 0x128) = 0.25f;
    At<float>(self, 0x12C) = 0.2f;
    At<float>(self, 0x130) = 0.2f;
    At<float>(self, 0x134) = 0.2f;
    At<float>(self, 0x138) = 4.0f;
    At<float>(self, 0x13C) = 0.0f;
    At<float>(self, 0x140) = 0.0f;
    At<float>(self, 0x144) = 10.0f;
    At<float>(self, 0x148) = 0.0f;
    At<float>(self, 0x14C) = 0.0f;
    At<float>(self, 0x150) = 5.0f;
    At<float>(self, 0x154) = 45.0f;
    At<float>(self, 0x158) = 0.0f;
    At<float>(self, 0x15C) = 0.0f;
    At<float>(self, 0x160) = 60.0f;
    At<float>(self, 0x164) = 0.0f;
    At<float>(self, 0x168) = 0.0f;
    At<float>(self, 0x16C) = 40.0f;
    for (int offset = 0x170; offset <= 0x188; offset += 4) {
        At<int32_t>(self, offset) = 0;
    }
    for (int offset = 0x18C; offset <= 0x1A8; offset += 4) {
        At<int32_t>(self, offset) = -1;
    }
    At<float>(self, 0x1AC) = 30.0f;
    At<float>(self, 0x1B0) = 0.3f;
    At<int32_t>(self, 0x1B4) = 0x10;
    for (int offset = 0x1B8; offset <= 0x1C4; offset += 4) {
        At<float>(self, offset) = 0.05f;
    }
    At<float>(self, 0x1C8) = 10.0f;
    At<float>(self, 0x1CC) = 20.0f;
    At<float>(self, 0x1D0) = 30.0f;
    At<float>(self, 0x1D4) = 40.0f;
    for (int offset = 0x1D8; offset <= 0x1E4; offset += 4) {
        At<float>(self, offset) = 30.0f;
    }
    for (int offset = 0x1E8; offset <= 0x1F8; offset += 4) {
        At<int32_t>(self, offset) = -1;
    }
    At<int32_t>(self, 0x1FC) = 0;
    At<int32_t>(self, 0x200) = 0;
    At<int32_t>(self, 0x204) = 0;
    At<int32_t>(self, 0x208) = 0;
    for (int offset = 0x20C; offset <= 0x218; offset += 4) {
        At<float>(self, offset) = 10.0f;
    }
    At<float>(self, 0x21C) = 1.5f;
    At<float>(self, 0x220) = 0.8f;
    At<float>(self, 0x224) = 0.0625f;
    At<float>(self, 0x228) = 60.0f;
    At<int32_t>(self, 0x22C) = 3;
    At<int32_t>(self, 0x230) = 0x32;
    At<int32_t>(self, 0x244) = 0;
    // the colours: +0x234, +0x238, +0x23C, +0x240
    const uint8_t colours[16] = {0xEA, 0xAB, 0x5D, 0xFF, 0xA3, 0xD3, 0x9C, 0xFF,
                                 0x6D, 0xCF, 0xF6, 0xFF, 0xF4, 0x9A, 0xC1, 0xFF};
    for (int i = 0; i < 16; i++) {
        At<uint8_t>(self, 0x234 + i) = colours[i];
    }
}
ARTDECK_REVERSED("MGClassArt::SetDefaults", 0x006DA040, &MGClassArt_SetDefaults);

// 0x6DA480 (thiscall): the cursor back to the middle of the top edge
void __fastcall ArtDeck_ResetCursor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x14) = Size(self) >> 1;
    At<uint32_t>(self, 0x18) = 0;
    At<uint8_t>(self, 0x1C) = 0;
    At<uint8_t>(self, 0x1D) = 0;
    At<int32_t>(self, 0x20) = 5;
}
ARTDECK_REVERSED("ArtDeck::ResetCursor", 0x006DA480, &ArtDeck_ResetCursor);

// 0x6DA4A0 (thiscall): from a vertex, how far the drawn lines run left, up, right and
// down (stopping at the filled cells beside them); with `exclusive`, a run along the
// line being drawn is dropped (the first such of left / right / up, else down)
void __fastcall ArtDeck_GetRuns(uint8_t* self, re::Edx, uint32_t col, uint32_t row, int exclusive, int32_t* left,
                                int32_t* up, int32_t* right, int32_t* down) {
    uint32_t size = Size(self);
    uint32_t cellCol;
    uint16_t vertical;
    if (col == size) {
        cellCol = size - 1;
        vertical = 4;
    } else {
        vertical = 1;
        cellCol = col;
    }
    uint32_t cellRow;
    uint16_t horizontal;
    if (row == size) {
        horizontal = 8;
        cellRow = size - 1;
    } else {
        horizontal = 2;
        cellRow = row;
    }
    *left = 0;
    if (col != 0) {
        uint32_t c = col;
        while ((Cells(self)[Size(self) * cellRow + c - 1] & horizontal) != 0) {
            *left += 1;
            uint32_t s = Size(self);
            if ((Cells(self)[s * cellRow + c - 1] & 1) != 0) {
                break;
            }
            if (row != 0 && row != s && (Cells(self)[(cellRow - 1) * s + c - 1] & 1) != 0) {
                break;
            }
            c--;
            if (c == 0) {
                break;
            }
        }
    }
    *up = 0;
    if (row != 0) {
        uint32_t r = row;
        for (;;) {
            r--;
            if ((Cells(self)[r * Size(self) + cellCol] & vertical) == 0) {
                break;
            }
            *up += 1;
            uint32_t s = Size(self);
            uint32_t index = r * s + cellCol;
            if ((Cells(self)[index] & 2) != 0) {
                break;
            }
            if (col != 0 && col != s && (Cells(self)[index - 1] & 2) != 0) {
                break;
            }
            if (r == 0) {
                break;
            }
        }
    }
    *right = 0;
    if (col < Size(self)) {
        uint32_t c = col;
        while ((Cells(self)[Size(self) * cellRow + c] & horizontal) != 0) {
            *right += 1;
            uint32_t s = Size(self);
            if ((Cells(self)[s * cellRow + c] & 4) != 0) {
                break;
            }
            if (row != 0 && row != s && (Cells(self)[(cellRow - 1) * s + c] & 4) != 0) {
                break;
            }
            c++;
            if (s <= c) {
                break;
            }
        }
    }
    *down = 0;
    uint32_t s = Size(self);
    if (row < s) {
        uint32_t r = row;
        while ((Cells(self)[s * r + cellCol] & vertical) != 0) {
            *down += 1;
            s = Size(self);
            uint32_t index = s * r + cellCol;
            if ((Cells(self)[index] & 8) != 0) {
                break;
            }
            if (col != 0 && col != s && (Cells(self)[index - 1] & 8) != 0) {
                break;
            }
            r++;
            if (s <= r) {
                break;
            }
        }
    }
    if (!re::Flag(exclusive)) {
        return;
    }
    uint16_t drawnH = uint16_t(horizontal << 4);
    uint16_t drawnV = uint16_t(vertical << 4);
    if (*left != 0 && (drawnH & Cells(self)[Size(self) * cellRow + col - 1]) != 0) {
        *left = 0;
        return;
    }
    if (*right != 0 && (drawnH & Cells(self)[Size(self) * cellRow + col]) != 0) {
        *right = 0;
        return;
    }
    if (*up != 0 && (drawnV & Cells(self)[(row - 1) * Size(self) + cellCol]) != 0) {
        *up = 0;
        return;
    }
    if (*down != 0 && (drawnV & Cells(self)[Size(self) * row + cellCol]) != 0) {
        *down = 0;
    }
}
ARTDECK_REVERSED("ArtDeck::GetRuns", 0x006DA4A0, &ArtDeck_GetRuns);

// 0x6DA770 (thiscall): a random corner, not the quadrant the cursor's in (when the
// pick is it: the one before, or 1)
void __fastcall ArtDeck_RandomCorner(uint8_t* self, re::Edx, uint32_t* col, uint32_t* row) {
    uint32_t half = Size(self) >> 1;
    int32_t quadrant = 0;
    if (At<uint32_t>(self, 0x14) > half) {
        quadrant = 1;
    }
    if (At<uint32_t>(self, 0x18) > half) {
        quadrant += 2;
    }
    int32_t pick = ACD_Rand() % 4;
    if (pick == quadrant) {
        pick = quadrant > 0 ? quadrant - 1 : 1;
    }
    *col = (pick & 1) != 0 ? 0 : Size(self);
    if ((pick & 2) != 0) {
        *row = 0;
        return;
    }
    *row = Size(self);
}
ARTDECK_REVERSED("ArtDeck::RandomCorner", 0x006DA770, &ArtDeck_RandomCorner);

// 0x6DA7E0 (thiscall): the cell under a point is filled (the cell's bit 8)
uint8_t __fastcall ArtDeck_IsFilledAt(uint8_t* self, re::Edx, const float* pos) {
    float size = SizeF(self);
    int32_t row = Chop(size * pos[1]);
    int32_t col = Chop(size * pos[0]);
    uint8_t* cell = reinterpret_cast<uint8_t*>(&Cells(self)[row * At<int32_t>(self, 0xC) + col]);
    return cell[1] & 1;
}
// callers 0x6DE2F0 / 0x6E07F0 (Exploder / FloaterObject Update, ArtClassArcade.cpp) rely on ecx: turn on together with them
ARTDECK_REVERSED("ArtDeck::IsFilledAt", 0x006DA7E0, &ArtDeck_IsFilledAt);

// 0x6DA870 (thiscall): the line being drawn rubbed out (its bits and the lines' marks)
void __fastcall ArtDeck_ClearDrawing(uint8_t* self, re::Edx) {
    for (uint32_t row = 0; row < Size(self); row++) {
        for (uint32_t col = 0; col < Size(self); col++) {
            uint16_t* cell = &Cells(self)[Size(self) * row + col];
            uint16_t drawn = uint16_t(*reinterpret_cast<uint8_t*>(cell) & 0xF0);
            if (drawn != 0) {
                *cell &= uint16_t(~uint16_t((drawn >> 4) | drawn));
            }
        }
    }
    At<uint8_t>(self, 0x24) = 0;
    At<uint8_t>(self, 0x25) = 0;
}
ARTDECK_REVERSED("ArtDeck::ClearDrawing", 0x006DA870, &ArtDeck_ClearDrawing);

// 0x6DA8D0 (thiscall): a point to the nearest vertex
void __fastcall ArtDeck_ToGrid(uint8_t* self, re::Edx, const float* pos, int32_t* col, int32_t* row) {
    float size = SizeF(self);
    float x = size * pos[0];
    float y = size * pos[1];
    int32_t c = Chop(x);
    int32_t r = Chop(y);
    if (float(std::fmod(double(x), 1.0)) >= 0.5f) {
        c++;
    }
    if (float(std::fmod(double(y), 1.0)) >= 0.5f) {
        *col = c;
        *row = r + 1;
        return;
    }
    *col = c;
    *row = r;
}
ARTDECK_REVERSED("ArtDeck::ToGrid", 0x006DA8D0, &ArtDeck_ToGrid);

// 0x6DA9C0 (thiscall): a step of the line being drawn from a vertex in a direction:
// 1 back onto a drawn line (not drawing), 0 into the filled area, 2 a step, 3 / 4 the
// line met a line (`stop` bits; 4 its own)
int __fastcall ArtDeck_DrawStep(uint8_t* self, re::Edx, uint32_t col, uint32_t row, uint32_t direction, uint32_t stop) {
    uint16_t bits = uint16_t(direction);
    if (At<uint8_t>(self, 0x24) == 0) {
        uint16_t cell = Cell(self, col, row);
        if ((cell & bits) != 0) {
            return 1;
        }
        if ((cell & 0x100) != 0) {
            return 0;
        }
        At<uint32_t>(self, 0x30) = At<uint32_t>(self, 0x14);
        At<uint32_t>(self, 0x28) = col;
        At<uint32_t>(self, 0x2C) = row;
        At<uint16_t>(self, 0x38) = bits;
        At<uint32_t>(self, 0x34) = At<uint32_t>(self, 0x18);
        At<uint8_t>(self, 0x24) = 1;
    }
    uint16_t marks = uint16_t(uint16_t(direction << 4) | bits);
    uint16_t* cell = &Cell(self, col, row);
    if ((*cell & marks) == marks) {
        At<uint8_t>(self, 0x24) = 0;
        At<uint8_t>(self, 0x25) = 0;
        return 4;
    }
    *cell |= marks;
    bool other = false;
    uint32_t otherCol = 0;
    uint32_t otherRow = 0;
    if ((direction & 1) != 0 && col != 0) {
        Cells(self)[Size(self) * row + col - 1] |= 0x44;
        otherCol = col - 1;
        otherRow = row;
        other = true;
    } else if ((direction & 2) != 0 && row != 0) {
        Cells(self)[(row - 1) * Size(self) + col] |= 0x88;
        otherCol = col;
        otherRow = row - 1;
        other = true;
    }
    uint16_t here = Cell(self, col, row);
    int resultHere = 2;
    int resultOther = 2;
    if ((here & uint16_t(stop)) != 0) {
        resultHere = (((stop & 0xFFFF) << 4 & uint32_t(here)) != 0 ? 1 : 0) + 3;
    }
    if (other) {
        uint16_t there = Cells(self)[Size(self) * otherRow + otherCol];
        if ((there & uint16_t(stop)) != 0) {
            resultOther = (((stop & 0xFFFF) << 4 & uint32_t(there)) != 0 ? 1 : 0) + 3;
        }
    }
    if (resultHere == 2 && resultOther == 2) {
        return 2;
    }
    At<uint8_t>(self, 0x25) = 1;
    At<uint8_t>(self, 0x24) = 0;
    if (resultHere == 4 || resultOther == 4) {
        return 4;
    }
    return 3;
}
ARTDECK_REVERSED("ArtDeck::DrawStep", 0x006DA9C0, &ArtDeck_DrawStep);

// 0x6DAB60 (thiscall): the cell on one side of the line's start (the first direction's)
void __fastcall ArtDeck_GetStartCell(uint8_t* self, re::Edx, int otherSide, int32_t* col, int32_t* row) {
    if ((At<uint8_t>(self, 0x38) & 1) == 0) {
        if (!re::Flag(otherSide)) {
            *col = At<int32_t>(self, 0x28);
            *row = At<int32_t>(self, 0x2C) - 1;
            return;
        }
    } else if (!re::Flag(otherSide)) {
        *col = At<int32_t>(self, 0x28) - 1;
        *row = At<int32_t>(self, 0x2C);
        return;
    }
    *col = At<int32_t>(self, 0x28);
    *row = At<int32_t>(self, 0x2C);
}
ARTDECK_REVERSED("ArtDeck::GetStartCell", 0x006DAB60, &ArtDeck_GetStartCell);

// 0x6DABC0 (thiscall): a scanline flood fill from a cell over `marks` (a copy of the
// cells: 0x100 the cells reached), bounded by the lines; returns how many it reached
int __fastcall ArtDeck_FloodFill(uint8_t* self, re::Edx, int32_t col, int32_t row, uint8_t* marks) {
    re::Ptr32<int32_t>& stack = At<int32_t*>(self, 0x48);
    int32_t& depth = At<int32_t>(self, 0x4C);
    depth = 0;
    int reached = 0;
    stack[0] = col;
    depth++;
    stack[depth] = row;
    depth++;
    auto Mark = [&](int32_t c, int32_t r) -> uint16_t& {
        return *reinterpret_cast<uint16_t*>(marks + (At<int32_t>(self, 0xC) * r + c) * 2);
    };
    for (;;) {
        if (depth == 0) {
            return reached;
        }
        int32_t size = At<int32_t>(self, 0xC);
        int32_t r = stack[depth - 1];
        int32_t c = stack[depth - 2];
        depth -= 2;
        // up to the top of the run
        if ((*reinterpret_cast<uint16_t*>(marks + (size * r + c) * 2) & 0x100) == 0) {
            uint8_t* cell = reinterpret_cast<uint8_t*>(Cells(self)) + (size * r + c) * 2;
            do {
                if ((*cell & 2) != 0) {
                    break;
                }
                cell -= size * 2;
                r--;
            } while ((*reinterpret_cast<uint16_t*>(marks + (size * r + c) * 2) & 0x100) == 0);
        }
        bool left = false;
        bool right = false;
        int32_t s = size;
        uint16_t mark = *reinterpret_cast<uint16_t*>(marks + (s * r + c) * 2);
        while ((mark & 0x100) == 0) {
            reached++;
            Mark(c, r) |= 0x100;
            int32_t index = (At<int32_t>(self, 0xC) * r + c) * 2;
            uint8_t cellByte = *(reinterpret_cast<uint8_t*>(Cells(self)) + index);
            if (left) {
                if ((cellByte & 1) != 0 || (*reinterpret_cast<uint16_t*>(marks + index - 2) & 0x100) != 0) {
                    left = false;
                }
            } else if ((cellByte & 1) == 0 && (*reinterpret_cast<uint16_t*>(marks + index - 2) & 0x100) == 0) {
                stack[depth] = c - 1;
                depth++;
                stack[depth] = r;
                depth++;
                left = true;
            }
            index = (At<int32_t>(self, 0xC) * r + c) * 2;
            cellByte = *(reinterpret_cast<uint8_t*>(Cells(self)) + index);
            if (right) {
                if ((cellByte & 4) != 0 || (*reinterpret_cast<uint16_t*>(marks + index + 2) & 0x100) != 0) {
                    right = false;
                }
            } else if ((cellByte & 4) == 0 && (*reinterpret_cast<uint16_t*>(marks + index + 2) & 0x100) == 0) {
                stack[depth] = c + 1;
                depth++;
                stack[depth] = r;
                depth++;
                right = true;
            }
            s = At<int32_t>(self, 0xC);
            if ((*(reinterpret_cast<uint8_t*>(Cells(self)) + (s * r + c) * 2) & 8) != 0) {
                break;
            }
            r++;
            mark = *reinterpret_cast<uint16_t*>(marks + (s * r + c) * 2);
        }
    }
}
ARTDECK_REVERSED("ArtDeck::FloodFill", 0x006DABC0, &ArtDeck_FloodFill);

// 0x6DAD80 (thiscall): the drawing marks cleared (bits 4..7)
void __fastcall ArtDeck_ClearMarks(uint8_t* self, re::Edx) {
    uint32_t size = Size(self);
    for (uint32_t row = 0; row < size; row++) {
        for (uint32_t col = 0; col < size; col++) {
            uint16_t* cell = &Cells(self)[size * row + col];
            *cell &= 0xFF0F;
            size = Size(self);
        }
        size = Size(self);
    }
}
ARTDECK_REVERSED("ArtDeck::ClearMarks", 0x006DAD80, &ArtDeck_ClearMarks);
