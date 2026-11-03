#pragma once

// The frontend (the pause menu's screens): FrontEndScreen and its kinds (FEOptions,
// FEControls, FEStats, FEStatus, FETasks, FEUpgrades, FEYearBook, ...), their
// selection lists, and the options' COption kinds. Only what's reversed so far.
//
// FrontEndScreen's virtual functions (IArchivable's first):
//   0 deleting destructor   3 OnEnter          5 Process (input)
//   1 Load(archive)         4 OnLeave          6 Draw
//   2 (empty, one argument)
// COption's: 0 Init (reads the setting), 1 Draw, 2 OnDeselect, 3 Apply, 4 Save (keeps
// the setting to go back to), 5 Restore, 6 Change(up) / HandleInput, 7 Step(up).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

// ---- the options ----------------------------------------------------------------------

struct COption {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x00
    uint32_t unknown04;       // +0x04
    uint32_t nameKey;         // +0x08 its text
};
static_assert(sizeof(COption) == 0xC);

// On/off and cycling options: the value and what Save kept
struct COptionValue : COption {
    int8_t value;             // +0x0C
    int8_t saved;             // +0x0D
    uint8_t unknown0E[2];
    uint32_t unknown10;       // +0x10 (vibration: a time)
};
static_assert(offsetof(COptionValue, value) == 0xC);
static_assert(offsetof(COptionValue, unknown10) == 0x10);

// Music/SFX/speech volume: the volume Save kept
struct CVolumeOption : COption {
    int32_t savedVolume;      // +0x0C
    uint8_t previewing;       // +0x10 (music: its sample playing)
};
static_assert(offsetof(CVolumeOption, previewing) == 0x10);

struct CBrightnessOption : COption {
    uint8_t changed;          // +0x0C
    uint8_t unknown0D;
    uint16_t level;           // +0x0E 0 .. 10
    uint16_t savedLevel;      // +0x10
};
static_assert(offsetof(CBrightnessOption, savedLevel) == 0x10);

struct CAdjustScreenPosOption : COption {
    uint8_t adjusting;        // +0x0C
    uint8_t unknown0D[3];
    float x, y;               // +0x10 the offsets being tried
    float savedX, savedY;     // +0x18
};
static_assert(offsetof(CAdjustScreenPosOption, savedY) == 0x1C);

struct CResetToDefaultsOption : COption {
    int32_t unknown0C;        // +0x0C (0: resets)
};

// ---- selection lists ---------------------------------------------------------------------

// FESelectionListWindow: a list of text keys, some visible at a time
struct FESelectionListWindow {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x00
    uint8_t unknown04[0x54];
    int8_t visibleCount;      // +0x58
    uint8_t unknown59[0xB];
    re::Ptr32<uint32_t> entries;        // +0x64
    int8_t selected;          // +0x68
    uint8_t unknown69;
    uint8_t count;            // +0x6A
    uint8_t firstVisible;     // +0x6B
    uint8_t unknown6C;
    uint8_t unknown6D;        // +0x6D
};
static_assert(offsetof(FESelectionListWindow, entries) == 0x64);
static_assert(offsetof(FESelectionListWindow, unknown6D) == 0x6D);

// FEScrollingSelectionListWindow: items from a virtual GetItem(index) (slot 1)
struct FEScrollingSelectionListWindow {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x00
    int32_t count;            // +0x04
    int32_t unknown08;        // +0x08
    int32_t selected;         // +0x0C -1: none selectable
    float x, y;               // +0x10
    float width, height;      // +0x18
    int32_t unknown20;        // +0x20
    float unknown24;          // +0x24
    float unknown28;          // +0x28
    int32_t unknown2C;        // +0x2C
    float unknown30;          // +0x30
    float unknown34;          // +0x34
    float arrowWidth;         // +0x38
    float arrowHeight;        // +0x3C
    float totalHeight;        // +0x40 (-1: to measure)
    uint8_t unknown44[4];
    uint8_t unknown48;        // +0x48
    uint8_t unknown49[3];
    float unknown4C[4];       // +0x4C
    float scrollThreshold;    // +0x5C
    float unknown60;          // +0x60
    uint8_t unknown64;        // +0x64
};
static_assert(offsetof(FEScrollingSelectionListWindow, totalHeight) == 0x40);
static_assert(offsetof(FEScrollingSelectionListWindow, unknown64) == 0x64);

// An item of one (its vtable: 0 GetExtent(rect, x, y), 1 Draw, ...)
struct FEListItem {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x00
    uint8_t selectable;       // +0x04
    uint8_t unknown05[3];
    int32_t id;               // +0x08 (a task, a stat, ...)
};
