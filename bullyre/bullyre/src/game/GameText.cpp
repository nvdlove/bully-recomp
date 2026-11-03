// The game's text: CText (one object at 0x20C2FA4: its blocks of keyed UTF-16 strings,
// read from Config\Text\<language>.img through the .dir directory), the blocks, the
// UTF-16 string helpers, and the subtitle line (CSubtitles). 0x68EFF0..0x6905EB.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

// One block of text (0x38 bytes): a sorted table of (key hash, string) and the strings.
// Its first 0x28 bytes are read from the file as they are (the header).
struct TextEntry {
    int32_t hash;             // +0x00
    re::Ptr32<uint16_t> text;           // +0x04 (in the file: an offset + 1 into the data)
};
static_assert(sizeof(TextEntry) == 8);

struct CTextBlock {
    uint32_t magic;           // +0x00 0xABCD1234
    int32_t version;          // +0x04 1
    int32_t headerSize;       // +0x08 0x28
    int32_t dataSize;         // +0x0C
    int32_t tableSize;        // +0x10 count * 8
    int32_t count;            // +0x14 -1: not loaded
    int32_t unknown18;        // +0x18
    int32_t unknown1C;        // +0x1C
    char tag[4];              // +0x20 "BD" or "BE" (0x454545 until read)
    int32_t bigEndian;        // +0x24 which tag
    re::Ptr32<TextEntry> entries;       // +0x28 (new[]: the count before it)
    re::Ptr32<uint8_t> data;            // +0x2C
    int32_t id;               // +0x30 the block's name hash
    uint8_t dataNotOwned;     // +0x34 in the caller's buffer
    uint8_t pad35[3];
};
static_assert(offsetof(CTextBlock, entries) == 0x28);
static_assert(sizeof(CTextBlock) == 0x38);

// CText: 0 general (always), 1 / 2 two missions', 3 one preloaded, 4 another, 5 the
// credits, 6 (only searched)
struct CText {
    re::Ptr32<CTextBlock> blocks[7];    // +0x00
};

// A subtitle line (0xA0 bytes)
struct SubtitleLine {
    int32_t rect[4];          // +0x00
    uint8_t unknown10;        // +0x10
    uint8_t colour[4];        // +0x11
    uint8_t pad15[3];
    int32_t duration;         // +0x18 ms, 0: none
    uint32_t startTime;       // +0x1C
    char text[0x80];          // +0x20
};
static_assert(offsetof(SubtitleLine, colour) == 0x11);
static_assert(sizeof(SubtitleLine) == 0xA0);

// CSubtitles: its line(s) (one: the count at +0xA4 is 1), then its own settings
struct CSubtitles {
    SubtitleLine line;        // +0x00
    uint8_t initialised;      // +0xA0
    uint8_t showing;          // +0xA1
    uint8_t padA2[2];
    int32_t count;            // +0xA4
    float scale[3];           // +0xA8
    uint8_t colour[4];        // +0xB4
};
static_assert(offsetof(CSubtitles, count) == 0xA4);
static_assert(sizeof(CSubtitles) == 0xB8);

GAME_VAR(uint32_t, g_timeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_ARRAY(const char*, g_languageNames, 0x00BA5C7C);  // [8]
GAME_VAR(void*, g_textDirectory, 0x020C2FA0);          // a CDirectory
GAME_ARRAY(uint8_t, g_generalTextBuffer, 0x0208D440);  // 220000 bytes
GAME_ARRAY(uint8_t, g_subtitleColour, 0x0208D43C);
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_VAR(uint8_t, g_subtitlesOn, 0x00C1870C);
GAME_VAR(uint8_t, g_unknownC18720, 0x00C18720);
GAME_VAR(void*, g_screenCamera, 0x00C1AE7C);
GAME_VAR(uint8_t, g_userStart, 0x020C32E9);
GAME_VAR(uint8_t, g_menuFlag20C32EA, 0x020C32EA);
GAME_VAR(uint8_t, g_menuFlag20C32EB, 0x020C32EB);
GAME_VAR(uint8_t, g_menuFlag20C32F2, 0x020C32F2);
GAME_VAR(void*, g_hudText, 0x00C67548);
uint16_t* const kMissingText = reinterpret_cast<uint16_t*>(0x0093F0E8);  // L"  "
constexpr uintptr_t VT_CDirectory = 0x009186D0;

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, int size, int count, void* ctor);
void* const kTextEntryConstruct = reinterpret_cast<void*>(0x006CC2D0);
inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(int, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, int file, void* buffer, int size);
GAME_FN(int, __cdecl, File_Seek, 0x0042D300, int file, int offset, int origin);
GAME_FN(int, __cdecl, File_Close, 0x0042D450, int file);
GAME_FN(void, __thiscall, Directory_Construct, 0x004291B0, void* directory, int capacity);
GAME_FN(void, __thiscall, Directory_ReadDirFile, 0x004291F0, void* directory, const char* path);
GAME_FN(uint8_t, __thiscall, Directory_FindItemByHash, 0x004290A0, void* directory, int32_t hash, int32_t* offset,
        int32_t* size);
GAME_FN(uint8_t*, __cdecl, Camera_GetRaster, 0x006C8740, void* camera);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __thiscall, HUDText_SetText, 0x0055B130, void* hudText, int slot, const char* text);

GAME_FN(void, __thiscall, Line_Setup, 0x0068F000, SubtitleLine* line, const int32_t* rect, int unknown);
GAME_FN(void, __thiscall, Line_Clear, 0x0068F060, SubtitleLine* line);
GAME_FN(void, __thiscall, Subtitles_ClearLines, 0x0068F0D0, CSubtitles* self);
GAME_FN(void, __thiscall, Subtitles_Update, 0x0068F2E0, CSubtitles* self);
GAME_FN(uint8_t, __cdecl, Text_ReadDirectory, 0x0068F640);
GAME_FN(uint8_t, __thiscall, Text_LoadMissionA, 0x0068F6C0, CText* self, int32_t id);
GAME_FN(uint8_t, __thiscall, Text_LoadMissionB, 0x0068F7A0, CText* self, int32_t id);
GAME_FN(uint8_t, __thiscall, Text_Preload, 0x0068F880, CText* self, int32_t id);
GAME_FN(uint8_t, __thiscall, Text_LoadExtra, 0x0068F970, CText* self, int32_t id);
GAME_FN(void, __thiscall, Text_Unload, 0x0068FB40, CText* self);
GAME_FN(uint8_t, __thiscall, Text_Load, 0x0068FC00, CText* self);
GAME_FN(CTextBlock*, __thiscall, Block_Construct, 0x006901E0, CTextBlock* self, int32_t id);
GAME_FN(TextEntry*, __thiscall, Block_Search, 0x00690230, CTextBlock* self, int32_t hash, int lo, int hi);
GAME_FN(TextEntry*, __thiscall, Block_Find, 0x006902A0, CTextBlock* self, const char* key);
GAME_FN(TextEntry*, __thiscall, Block_FindByHash, 0x006902D0, CTextBlock* self, int32_t hash);
GAME_FN(void, __thiscall, Block_Unload, 0x006902F0, CTextBlock* self);
GAME_FN(uint8_t, __thiscall, Block_Read, 0x006903A0, CTextBlock* self, int file, int32_t sector, int32_t size,
        uint8_t* buffer, int32_t bufferSize);
GAME_FN(void, __thiscall, Block_Destruct, 0x00690550, CTextBlock* self);
GAME_FN(uint8_t, __thiscall, Block_Load, 0x00690560, CTextBlock* self, void* directory, const char* path, int32_t id,
        uint8_t* buffer, int32_t bufferSize);

void DeleteBlock(CTextBlock* block) {
    if (block != nullptr) {
        Block_Destruct(block);
        Free(block);
    }
}

CTextBlock* NewBlock(int32_t id) {
    void* memory = Malloc(sizeof(CTextBlock));
    return memory != nullptr ? Block_Construct(static_cast<CTextBlock*>(memory), id) : nullptr;
}

void ImagePath(char* path) {
    Sprintf(path, "%s%s.img", "Config\\Text\\", g_languageNames[g_language]);
}

} // namespace

namespace gametext {

// ---- the subtitles -----------------------------------------------------------------------

// 0x68EFF0
void __fastcall Subtitles_SetShowing(CSubtitles* self, re::Edx, int showing) {
    self->showing = uint8_t(showing);
}

// 0x68F000: its rectangle, white
void __fastcall Line_SetupImpl(SubtitleLine* self, re::Edx, const int32_t* rect, int unknown) {
    self->rect[0] = rect[0];
    self->rect[1] = rect[1];
    self->rect[2] = rect[2];
    self->rect[3] = rect[3];
    uint8_t white[4];
    uint8_t* colour = RGBA_Construct(white, 255, 255, 255, 255);
    self->colour[0] = colour[0];
    self->colour[1] = colour[1];
    self->colour[2] = colour[2];
    self->colour[3] = colour[3];
    self->unknown10 = uint8_t(unknown);
}

// the line emptied (0x68F060, and inlined in 0x68F0D0 / 0x68F250)
static void ClearLine(SubtitleLine* line) {
    Strncpy(line->text, "", 0x7F);
    line->duration = 0;
    line->startTime = g_timeInMs;
    line->colour[0] = g_subtitleColour[0];
    line->colour[1] = g_subtitleColour[1];
    line->colour[2] = g_subtitleColour[2];
    line->colour[3] = g_subtitleColour[3];
    line->rect[0] = 0;
    line->rect[1] = 0;
    line->rect[2] = 0;
    line->rect[3] = 0;
    line->startTime = 0;
    line->duration = 0;
    line->unknown10 = 0;
}

void __fastcall Line_ClearImpl(SubtitleLine* self, re::Edx) {
    ClearLine(self);
}

// 0x68F0D0
void __fastcall Subtitles_ClearLinesImpl(CSubtitles* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        ClearLine(&self->line + i);
    }
}

// 0x68F160: a line's text for `duration` ms
void __fastcall Subtitles_Add(CSubtitles* self, re::Edx, const char* text, int duration, int index) {
    Font_Reset();
    Font_SetJustify(3);
    SubtitleLine* line = &self->line + index;
    Strncpy(line->text, text, 0x7F);
    line->duration = duration;
    line->startTime = g_timeInMs;
    line->colour[0] = g_subtitleColour[0];
    line->colour[1] = g_subtitleColour[1];
    line->colour[2] = g_subtitleColour[2];
    line->colour[3] = g_subtitleColour[3];
    self->showing = 1;
}

// 0x68F1D0
void __fastcall Subtitles_Setup(CSubtitles* self, re::Edx, const int32_t* rect, int unknown, const float* scale,
                                const uint8_t* colour) {
    self->colour[0] = colour[0];
    self->colour[1] = colour[1];
    self->colour[2] = colour[2];
    self->colour[3] = colour[3];
    self->scale[0] = scale[0];
    self->scale[1] = scale[1];
    self->scale[2] = scale[2];
    Line_Setup(&self->line, rect, unknown);
}

// 0x68F230
void __fastcall Subtitles_Shutdown(CSubtitles* self, re::Edx) {
    if (self->initialised != 0) {
        Subtitles_ClearLines(self);
    }
    self->initialised = 0;
}

// 0x68F250
CSubtitles* __fastcall Subtitles_Construct(CSubtitles* self, re::Edx) {
    ClearLine(&self->line);
    self->scale[0] = 0.0f;
    self->scale[1] = 0.0f;
    self->scale[2] = 0.0f;
    self->count = 1;
    self->showing = 0;
    Subtitles_ClearLines(self);
    return self;
}

// 0x68F2E0: lines past their time emptied; none left: not showing
void __fastcall Subtitles_UpdateImpl(CSubtitles* self, re::Edx) {
    int live = 0;
    for (int i = 0; i < self->count; i++) {
        SubtitleLine* line = &self->line + i;
        if (line->duration != 0 && line->startTime != 0) {
            if (line->startTime + uint32_t(line->duration) < g_timeInMs) {
                Line_Clear(line);
            } else {
                live++;
            }
        }
    }
    if (live == 0) {
        self->showing = 0;
    }
}

// 0x68F340: one line along the screen's bottom (50 in from each side, 55 up)
void __fastcall Subtitles_Init(CSubtitles* self, re::Edx) {
    if (self->initialised != 0) {
        Subtitles_ClearLines(self);
        self->initialised = 0;
    }
    self->showing = 0;
    self->count = 1;
    uint8_t* raster = Camera_GetRaster(g_screenCamera);
    int32_t width = *reinterpret_cast<int32_t*>(raster + 0xC);
    int32_t height = *reinterpret_cast<int32_t*>(raster + 0x10);
    int32_t rect[4];
    rect[2] = width - 100;
    rect[1] = height - 55;
    rect[0] = (width - rect[2]) / 2;
    rect[3] = 50;
    uint8_t white[4];
    uint8_t* colour = RGBA_Construct(white, 255, 255, 255, 255);
    self->colour[0] = colour[0];
    self->colour[1] = colour[1];
    self->colour[2] = colour[2];
    self->colour[3] = colour[3];
    self->scale[0] = 0.8f;
    self->scale[1] = 1.0f;
    self->scale[2] = 0.0f;
    Line_Setup(&self->line, rect, 0);
    Subtitles_ClearLines(self);
    self->initialised = 1;
}

// 0x68F440: the lines through the HUD's text (subtitles on, or the other setting off)
void __fastcall Subtitles_Render(CSubtitles* self, re::Edx) {
    if (self->showing == 0 || self->initialised == 0) {
        return;
    }
    Subtitles_Update(self);
    if (g_subtitlesOn == 0 && g_unknownC18720 != 0) {
        return;
    }
    Font_Reset();
    Font_SetJustify(3);
    for (int i = 0; i < self->count; i++) {
        SubtitleLine* line = &self->line + i;
        HUDText_SetText(g_hudText, 0, line->text);
        line->colour[0] = g_fontColour[0];
        line->colour[1] = g_fontColour[1];
        line->colour[2] = g_fontColour[2];
        line->colour[3] = g_fontColour[3];
    }
}

// ---- CText -------------------------------------------------------------------------------

// 0x68F4E0 (cdecl): the text directory (0x120 entries)
void __cdecl Text_CreateDirectory() {
    void* directory = Malloc(0x10);
    if (directory == nullptr) {
        g_textDirectory = nullptr;
        return;
    }
    Directory_Construct(directory, 0x120);
    *static_cast<uint32_t*>(directory) = VT_CDirectory;
    g_textDirectory = directory;
}

// 0x68F520 / 0x68F550 / 0x68FA40: a block deleted
template <int Index>
void __fastcall Text_UnloadBlock(CText* self, re::Edx) {
    DeleteBlock(self->blocks[Index]);
    self->blocks[Index] = nullptr;
}

// 0x68F580 / 0x68F5C0 / 0x68F600 (cdecl): a HUD text slot's text, unless a menu is up
template <int Slot>
void __cdecl ShowHudText(const char* text) {
    if (g_userStart == 0 && g_menuFlag20C32EA == 0 && g_menuFlag20C32EB == 0 && g_menuFlag20C32F2 == 0) {
        HUDText_SetText(g_hudText, Slot, text);
    }
}

// 0x68F640 (cdecl): the language's .dir read into the directory
uint8_t __cdecl Text_ReadDirectoryImpl() {
    *reinterpret_cast<int32_t*>(static_cast<uint8_t*>(g_textDirectory) + 0xC) = 0;
    char path[260];
    Sprintf(path, "%s%s.dir", "Config\\Text\\", g_languageNames[g_language]);
    int file = File_Open(path, "r", 1);
    if (file == 0) {
        return 0;
    }
    File_Close(file);
    Directory_ReadDirFile(g_textDirectory, path);
    return 1;
}

// 0x68F6C0 / 0x68F7A0: a mission's block (taken from the preloaded one when that's it)
template <int Index>
uint8_t __fastcall Text_LoadMission(CText* self, re::Edx, int32_t id) {
    DeleteBlock(self->blocks[Index]);
    self->blocks[Index] = nullptr;
    CTextBlock* preloaded = self->blocks[3];
    if (preloaded != nullptr && preloaded->id == id) {
        self->blocks[Index] = preloaded;
        self->blocks[3] = nullptr;
        return 1;
    }
    self->blocks[Index] = NewBlock(id);
    char path[260];
    ImagePath(path);
    if (Block_Load(self->blocks[Index], g_textDirectory, path, id, nullptr, 0) == 0) {
        DeleteBlock(self->blocks[Index]);
        self->blocks[Index] = nullptr;
        return 0;
    }
    return 1;
}

// 0x68F880: a block loaded ahead (not when a mission block has it already)
uint8_t __fastcall Text_PreloadImpl(CText* self, re::Edx, int32_t id) {
    if ((self->blocks[1] != nullptr && self->blocks[1]->id == id) ||
        (self->blocks[2] != nullptr && self->blocks[2]->id == id)) {
        return 1;
    }
    DeleteBlock(self->blocks[3]);
    self->blocks[3] = nullptr;
    self->blocks[3] = NewBlock(id);
    char path[260];
    ImagePath(path);
    if (Block_Load(self->blocks[3], g_textDirectory, path, id, nullptr, 0) == 0) {
        DeleteBlock(self->blocks[3]);
        self->blocks[3] = nullptr;
        return 0;
    }
    return 1;
}

// 0x68F970
uint8_t __fastcall Text_LoadExtraImpl(CText* self, re::Edx, int32_t id) {
    DeleteBlock(self->blocks[4]);
    self->blocks[4] = nullptr;
    self->blocks[4] = NewBlock(id);
    char path[260];
    ImagePath(path);
    if (Block_Load(self->blocks[4], g_textDirectory, path, id, nullptr, 0) == 0) {
        DeleteBlock(self->blocks[4]);
        self->blocks[4] = nullptr;
        return 0;
    }
    return 1;
}

// 0x68FA70: CREDITS.bin
uint8_t __fastcall Text_LoadCredits(CText* self, re::Edx) {
    DeleteBlock(self->blocks[5]);
    self->blocks[5] = nullptr;
    int32_t id = int32_t(HashName("CREDITS.bin"));
    char path[260];
    ImagePath(path);
    self->blocks[5] = NewBlock(id);
    if (Block_Load(self->blocks[5], g_textDirectory, path, id, nullptr, 0) == 0) {
        DeleteBlock(self->blocks[5]);
        self->blocks[5] = nullptr;
        return 0;
    }
    return 1;
}

// 0x68FB40: blocks 0 .. 5 deleted
void __fastcall Text_UnloadImpl(CText* self, re::Edx) {
    for (int i = 0; i < 6; i++) {
        DeleteBlock(self->blocks[i]);
        self->blocks[i] = nullptr;
    }
}

// 0x68FC00: the directory, then general.bin into its fixed buffer
uint8_t __fastcall Text_LoadImpl(CText* self, re::Edx) {
    if (Text_ReadDirectory() == 0) {
        return 0;
    }
    int32_t id = int32_t(HashName("general.bin"));
    if (self->blocks[0] == nullptr) {
        self->blocks[0] = NewBlock(id);
        char path[260];
        ImagePath(path);
        if (Block_Load(self->blocks[0], g_textDirectory, path, id, g_generalTextBuffer, 220000) == 0) {
            Text_Unload(self);
            return 0;
        }
    }
    return 1;
}

// 0x68FCB0 / 0x68FD00 / 0x68FD50 / 0x68FDA0: the same by name ("<name>.bin")
template <uintptr_t Loader>
uint8_t __fastcall Text_LoadByName(CText* self, re::Edx, const char* name) {
    if (name == nullptr) {
        return 0;
    }
    char file[260];
    Sprintf(file, "%s.bin", name);
    int32_t id = int32_t(HashName(file));
    return reinterpret_cast<uint8_t(__thiscall*)(CText*, int32_t)>(Loader)(self, id);
}

// 0x68FDF0: everything again (the language changed): the same blocks reloaded
void __fastcall Text_ReloadImpl(CText* self, re::Edx) {
    int32_t missionA = self->blocks[1] != nullptr ? self->blocks[1]->id : -1;
    int32_t missionB = self->blocks[2] != nullptr ? self->blocks[2]->id : -1;
    int32_t preload = self->blocks[3] != nullptr ? self->blocks[3]->id : -1;
    int32_t extra = self->blocks[4] != nullptr ? self->blocks[4]->id : -1;
    Text_Unload(self);
    Text_Load(self);
    if (missionA > -1) {
        Text_LoadMissionA(self, missionA);
    }
    if (missionB > -1) {
        Text_LoadMissionB(self, missionB);
    }
    if (preload > -1) {
        Text_Preload(self, preload);
    }
    if (extra > -1) {
        Text_LoadExtra(self, extra);
    }
}

// 0x68FE80: a key's text from the first block that has it; L"  " when none
const uint16_t* __fastcall Text_Get(CText* self, re::Edx, const char* key) {
    if (key == nullptr || key[0] == '\0') {
        return kMissingText;
    }
    if (self->blocks[0] == nullptr && Text_Load(self) == 0) {
        return kMissingText;
    }
    CTextBlock* blocks[7];
    for (int i = 0; i < 7; i++) {
        blocks[i] = self->blocks[i];
    }
    for (int i = 0; i < 7; i++) {
        if (blocks[i] == nullptr) {
            continue;
        }
        TextEntry* entry = Block_Find(blocks[i], key);
        if (entry != nullptr) {
            return entry->text != nullptr ? static_cast<uint16_t*>(entry->text) : kMissingText;
        }
    }
    return kMissingText;
}

// 0x68FF10: by the key's hash (the credits' block not searched)
const uint16_t* __fastcall Text_GetByHash(CText* self, re::Edx, int32_t hash) {
    if (hash == 0) {
        return kMissingText;
    }
    if (self->blocks[0] == nullptr && Text_Load(self) == 0) {
        return kMissingText;
    }
    TextEntry* entry = Block_FindByHash(self->blocks[0], hash);
    static const int kOthers[] = {1, 2, 3, 4, 6};
    for (int i = 0; entry == nullptr && i < 5; i++) {
        CTextBlock* block = self->blocks[kOthers[i]];
        if (block != nullptr) {
            entry = Block_FindByHash(block, hash);
        }
    }
    if (entry == nullptr || entry->text == nullptr) {
        return kMissingText;
    }
    return entry->text;
}

// ---- UTF-16 strings ------------------------------------------------------------------------

// 0x68FFA0
void __cdecl WStrCpy(uint16_t* dst, const uint16_t* src) {
    if (dst == nullptr) {
        return;
    }
    for (uint16_t c = *src; c != 0; c = *src) {
        src++;
        *dst++ = c;
    }
    *dst = 0;
}

// 0x68FFD0: at most n - 1 characters
void __cdecl WStrNCpy(uint16_t* dst, const uint16_t* src, int n) {
    if (dst == nullptr) {
        return;
    }
    uint16_t left = uint16_t(n);
    for (uint16_t c = *src; c != 0; c = *src) {
        left = uint16_t(left - 1);
        if (left == 0) {
            break;
        }
        src++;
        *dst++ = c;
    }
    *dst = 0;
}

// 0x690020
int __cdecl WStrLen(const uint16_t* s) {
    int length = 0;
    while (*s != 0) {
        s++;
        length++;
    }
    return length;
}

// 0x690040: the first difference (as unsigned characters) in the first n
int __cdecl WStrNCmp(const uint16_t* a, const uint16_t* b, int n) {
    uint16_t left = uint16_t(n);
    if (left == 0) {
        return 0;
    }
    for (;;) {
        uint16_t ca = *a;
        uint16_t cb = *b++;
        if (ca != cb) {
            return int(*a) - int(b[-1]);
        }
        a++;
        if (ca == 0) {
            return 0;
        }
        left = uint16_t(left - 1);
        if (left == 0) {
            return 0;
        }
    }
}

// 0x690090: ASCII widened (bytes sign-extended)
void __cdecl AsciiToWide(const char* src, uint16_t* dst) {
    if (dst == nullptr) {
        return;
    }
    for (char c = *src; c != 0; c = *src) {
        *dst++ = uint16_t(int16_t(int8_t(c)));
        src++;
    }
    *dst = 0;
}

// 0x6900C0: UTF-8 (up to 3 bytes) to UTF-16; a bad lead byte is skipped
void __cdecl Utf8ToWide(const uint8_t* src, uint16_t* dst) {
    uint8_t c = *src;
    while (c != 0) {
        if ((c & 0x80) == 0) {
            *dst++ = uint16_t(int16_t(int8_t(c)));
            src++;
        } else if ((c & 0xE0) == 0xC0) {
            *dst = uint16_t(int16_t(int8_t(c & 0x1F)) << 6);
            src++;
            *dst = uint16_t(*dst | uint16_t(int16_t(int8_t(*src & 0x3F))));
            src++;
            dst++;
        } else {
            src++;
            if ((c & 0xF0) == 0xE0) {
                *dst = uint16_t(int16_t(int8_t(c)) << 12);
                *dst = uint16_t(*dst | uint16_t(int16_t(int8_t(*src & 0x3F)) << 6));
                src++;
                *dst = uint16_t(*dst | uint16_t(int16_t(int8_t(*src & 0x3F))));
                src++;
                dst++;
            }
        }
        c = *src;
    }
    *dst = 0;
}

// 0x690150: one UTF-8 character's code (0: not a lead byte)
uint16_t __cdecl Utf8CharToWide(const uint8_t* src) {
    uint8_t c = src[0];
    if ((c & 0x80) == 0) {
        return uint16_t(int16_t(int8_t(c)));
    }
    if ((c & 0xE0) == 0xC0) {
        return uint16_t((int16_t(int8_t(c & 0x1F)) << 6) | int16_t(int8_t(src[1] & 0x3F)));
    }
    if ((c & 0xF0) == 0xE0) {
        return uint16_t((int16_t(int8_t(c)) << 12) | (int16_t(int8_t(src[1] & 0x3F)) << 6) |
                        int16_t(int8_t(src[2] & 0x3F)));
    }
    return 0;
}

// ---- the blocks ---------------------------------------------------------------------------

// 0x6901E0
CTextBlock* __fastcall Block_ConstructImpl(CTextBlock* self, re::Edx, int32_t id) {
    self->version = 1;
    self->count = -1;
    self->unknown18 = -1;
    self->bigEndian = 1;
    self->magic = 0xABCD1234;
    self->headerSize = 0x28;
    *reinterpret_cast<uint32_t*>(self->tag) = 0x454545;
    self->unknown1C = 0;
    self->entries = nullptr;
    self->data = nullptr;
    self->id = id;
    self->dataNotOwned = 0;
    return self;
}

// 0x690230: binary search of the table between lo and hi (16-bit indices)
TextEntry* __fastcall Block_SearchImpl(CTextBlock* self, re::Edx, int32_t hash, int lo, int hi) {
    int16_t low = int16_t(lo);
    int16_t high = int16_t(hi);
    while (low <= high) {
        uint16_t middle = uint16_t((int(low) + int(high)) / 2);
        int32_t key = self->entries[middle].hash;
        if (hash == key) {
            return &self->entries[middle];
        }
        int order = hash > key ? 1 : -1;
        if (order < 0) {
            high = int16_t(middle - 1);
        } else if (order > 0) {
            low = int16_t(middle + 1);
        } else {
            return &self->entries[middle];
        }
    }
    return nullptr;
}

// 0x6902A0
TextEntry* __fastcall Block_FindImpl(CTextBlock* self, re::Edx, const char* key) {
    uint16_t last = uint16_t(uint16_t(self->count) - 1);
    return Block_Search(self, int32_t(HashName(key)), 0, last);
}

// 0x6902D0
TextEntry* __fastcall Block_FindByHashImpl(CTextBlock* self, re::Edx, int32_t hash) {
    return Block_Search(self, hash, 0, int16_t(self->count) - 1);
}

// 0x6902F0: its data (unless the caller's) and table freed
void __fastcall Block_UnloadImpl(CTextBlock* self, re::Edx) {
    PushMemoryTag(0x35);
    if (self->data != nullptr) {
        if (self->dataNotOwned == 0) {
            Free(self->data);
        }
        self->data = nullptr;
        self->dataNotOwned = 0;
    }
    for (int i = 0; i < self->count; i++) {
        self->entries[i].text = nullptr;
    }
    if (self->entries != nullptr) {
        // (delete[]: each entry's destructor zeroes it, last first)
        int32_t* base = re::PtrCast<int32_t*>(self->entries) - 1;
        for (int i = *base - 1; i >= 0; i--) {
            self->entries[i].text = nullptr;
            self->entries[i].hash = 0;
        }
        Free(base);
    }
    PopMemoryTag();
    self->unknown1C = 0;
    self->entries = nullptr;
    self->count = -1;
    self->unknown18 = -1;
}

// 0x6903A0: the block at a sector of an open file: the header over its first 0x28
// bytes, checked, then the strings (into `buffer` when it's big enough; with a buffer
// too small: none, and false) and the table, its offsets made pointers
uint8_t __fastcall Block_ReadImpl(CTextBlock* self, re::Edx, int file, int32_t sector, int32_t, uint8_t* buffer,
                                  int32_t bufferSize) {
    Block_Unload(self);
    File_Seek(file, sector << 11, 0);
    if (File_Read(file, self, 0x28) == 0) {
        return 0;
    }
    if (self->magic != 0xABCD1234 || self->version != 1) {
        return 0;
    }
    const char* tag = self->bigEndian != 0 ? "BE" : "BD";
    for (int i = 0; i < 3; i++) {
        if (self->tag[i] != tag[i]) {
            return 0;
        }
    }
    if (self->tableSize != self->count * 8) {
        return 0;
    }
    self->dataNotOwned = 0;
    if (buffer != nullptr) {
        if (bufferSize < self->dataSize) {
            self->data = nullptr;
        } else {
            self->data = buffer;
            self->dataNotOwned = 1;
        }
    } else {
        PushMemoryTag(0x35);
        self->data = static_cast<uint8_t*>(Malloc(uint32_t(self->dataSize)));
        PopMemoryTag();
    }
    if (self->data == nullptr) {
        return 0;
    }
    PushMemoryTag(0x35);
    uint32_t count = uint32_t(self->count);
    uint64_t bytes64 = uint64_t(count) * 8;
    uint32_t bytes = (bytes64 >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes64);
    uint32_t total = bytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : bytes + 4;
    uint32_t* memory = static_cast<uint32_t*>(Malloc(total));
    TextEntry* entries = nullptr;
    if (memory != nullptr) {
        entries = reinterpret_cast<TextEntry*>(memory + 1);
        *memory = count;
        VectorConstruct(entries, 8, int(count), kTextEntryConstruct);
    }
    self->entries = entries;
    PopMemoryTag();
    if (self->entries == nullptr) {
        PushMemoryTag(0x35);
        Free(self->data);
        PopMemoryTag();
        self->data = nullptr;
        return 0;
    }
    if (File_Read(file, self->data, self->dataSize) == 0) {
        return 0;
    }
    if (File_Read(file, self->entries, self->tableSize) == 0) {
        return 0;
    }
    for (int i = 0; i < self->count; i++) {
        self->entries[i].text = reinterpret_cast<uint16_t*>(re::PtrCast<uintptr_t>(self->entries[i].text) +
                                                            re::PtrCast<uintptr_t>(self->data) - 1);
    }
    return 1;
}

// 0x690560: the block found in the directory, read from the image
uint8_t __fastcall Block_LoadImpl(CTextBlock* self, re::Edx, void* directory, const char* path, int32_t id,
                                  uint8_t* buffer, int32_t bufferSize) {
    int32_t size = 0;
    int32_t offset = 0;
    int file = File_Open(path, "r", 1);
    if (file == 0) {
        return 0;
    }
    if (Directory_FindItemByHash(directory, id, &offset, &size) == 0) {
        File_Close(file);
        return 0;
    }
    uint8_t result = Block_Read(self, file, offset, size, buffer, bufferSize);
    File_Close(file);
    return result;
}

} // namespace gametext

// Thunks (a lone jmp): 0x68FBF0 CText's destructor -> Unload, 0x690550 the block's -> Unload
RE_UNHOOKED("CText::~CText (jmp 0x68FB40)", 0x0068FBF0);
RE_UNHOOKED("CTextBlock::~CTextBlock (jmp 0x6902F0)", 0x00690550);
// 0x690056 is inside WStrNCmp (0x690040: its loop), not a function (its "vtable" refs are
// UTF-16 strings)
RE_UNHOOKED("WStrNCmp loop (inside 0x690040)", 0x00690056);

#define GAMETEXT_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(gametext_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(gametext_original_, __LINE__), \
             re::Mode::Original)

GAMETEXT_REVERSED("CSubtitles::SetShowing", 0x0068EFF0, &gametext::Subtitles_SetShowing);
GAMETEXT_REVERSED("SubtitleLine::Setup", 0x0068F000, &gametext::Line_SetupImpl);
GAMETEXT_REVERSED("SubtitleLine::Clear", 0x0068F060, &gametext::Line_ClearImpl);
GAMETEXT_REVERSED("CSubtitles::ClearLines", 0x0068F0D0, &gametext::Subtitles_ClearLinesImpl);
GAMETEXT_REVERSED("CSubtitles::Add", 0x0068F160, &gametext::Subtitles_Add);
GAMETEXT_REVERSED("CSubtitles::Setup", 0x0068F1D0, &gametext::Subtitles_Setup);
GAMETEXT_REVERSED("CSubtitles::Shutdown", 0x0068F230, &gametext::Subtitles_Shutdown);
GAMETEXT_REVERSED("CSubtitles::CSubtitles", 0x0068F250, &gametext::Subtitles_Construct);
GAMETEXT_REVERSED("CSubtitles::Update", 0x0068F2E0, &gametext::Subtitles_UpdateImpl);
GAMETEXT_REVERSED("CSubtitles::Init", 0x0068F340, &gametext::Subtitles_Init);
GAMETEXT_REVERSED("CSubtitles::Render", 0x0068F440, &gametext::Subtitles_Render);
GAMETEXT_REVERSED("CText::CreateDirectory", 0x0068F4E0, &gametext::Text_CreateDirectory);
GAMETEXT_REVERSED("CText::UnloadPreload", 0x0068F520, &gametext::Text_UnloadBlock<3>);
GAMETEXT_REVERSED("CText::UnloadExtra", 0x0068F550, &gametext::Text_UnloadBlock<4>);
GAMETEXT_REVERSED("CText::UnloadCredits", 0x0068FA40, &gametext::Text_UnloadBlock<5>);
GAMETEXT_REVERSED("ShowHudText3", 0x0068F580, &gametext::ShowHudText<3>);
GAMETEXT_REVERSED("ShowHudText6", 0x0068F5C0, &gametext::ShowHudText<6>);
GAMETEXT_REVERSED("ShowHudText7", 0x0068F600, &gametext::ShowHudText<7>);
GAMETEXT_REVERSED("CText::ReadDirectory", 0x0068F640, &gametext::Text_ReadDirectoryImpl);
GAMETEXT_REVERSED("CText::LoadMissionA", 0x0068F6C0, &gametext::Text_LoadMission<1>);
GAMETEXT_REVERSED("CText::LoadMissionB", 0x0068F7A0, &gametext::Text_LoadMission<2>);
GAMETEXT_REVERSED("CText::Preload", 0x0068F880, &gametext::Text_PreloadImpl);
GAMETEXT_REVERSED("CText::LoadExtra", 0x0068F970, &gametext::Text_LoadExtraImpl);
GAMETEXT_REVERSED("CText::LoadCredits", 0x0068FA70, &gametext::Text_LoadCredits);
GAMETEXT_REVERSED("CText::Unload", 0x0068FB40, &gametext::Text_UnloadImpl);
GAMETEXT_REVERSED("CText::Load", 0x0068FC00, &gametext::Text_LoadImpl);
GAMETEXT_REVERSED("CText::LoadMissionAByName", 0x0068FCB0, &gametext::Text_LoadByName<0x0068F6C0>);
GAMETEXT_REVERSED("CText::LoadMissionBByName", 0x0068FD00, &gametext::Text_LoadByName<0x0068F7A0>);
GAMETEXT_REVERSED("CText::PreloadByName", 0x0068FD50, &gametext::Text_LoadByName<0x0068F880>);
GAMETEXT_REVERSED("CText::LoadExtraByName", 0x0068FDA0, &gametext::Text_LoadByName<0x0068F970>);
GAMETEXT_REVERSED("CText::Reload", 0x0068FDF0, &gametext::Text_ReloadImpl);
GAMETEXT_REVERSED("CText::Get", 0x0068FE80, &gametext::Text_Get);
GAMETEXT_REVERSED("CText::GetByHash", 0x0068FF10, &gametext::Text_GetByHash);
GAMETEXT_REVERSED("WStrCpy", 0x0068FFA0, &gametext::WStrCpy);
GAMETEXT_REVERSED("WStrNCpy", 0x0068FFD0, &gametext::WStrNCpy);
GAMETEXT_REVERSED("WStrLen", 0x00690020, &gametext::WStrLen);
GAMETEXT_REVERSED("WStrNCmp", 0x00690040, &gametext::WStrNCmp);
GAMETEXT_REVERSED("AsciiToWide", 0x00690090, &gametext::AsciiToWide);
GAMETEXT_REVERSED("Utf8ToWide", 0x006900C0, &gametext::Utf8ToWide);
GAMETEXT_REVERSED("Utf8CharToWide", 0x00690150, &gametext::Utf8CharToWide);
GAMETEXT_REVERSED("CTextBlock::CTextBlock", 0x006901E0, &gametext::Block_ConstructImpl);
GAMETEXT_REVERSED("CTextBlock::Search", 0x00690230, &gametext::Block_SearchImpl);
GAMETEXT_REVERSED("CTextBlock::Find", 0x006902A0, &gametext::Block_FindImpl);
GAMETEXT_REVERSED("CTextBlock::FindByHash", 0x006902D0, &gametext::Block_FindByHashImpl);
GAMETEXT_REVERSED("CTextBlock::Unload", 0x006902F0, &gametext::Block_UnloadImpl);
GAMETEXT_REVERSED("CTextBlock::Read", 0x006903A0, &gametext::Block_ReadImpl);
GAMETEXT_REVERSED("CTextBlock::Load", 0x00690560, &gametext::Block_LoadImpl);
