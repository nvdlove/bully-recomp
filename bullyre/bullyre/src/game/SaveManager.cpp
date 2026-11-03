// The PC save manager (0x20CE75C, 0x288 bytes): the save folder "My Documents\Bully
// Scholarship Edition\", the slot table file "FileTableBully" (the 6 slots' summaries and the
// last slot saved, 0x154 bytes), the six save files BullyFile1..6 (the 0x56F4-byte block),
// the photo gallery "Gallery" (a 0x40-byte header and 15 photos of 0x14000 bytes), the state
// machine the frontend drives (Request, Dispatch) and its messages. 0x7351C0..0x737C90.
//
// States: 1 idle/format, 2 load, 3 save, 4 delete, 5 error, 6 done, 7/8/9 read the table
// then format (8 = for a save), 0x10/0x11 write the table after a save/delete, 0x14/0x15
// wait (0x15 = loaded), 0x16/0x17 finished, 0x18 reset, 0x19 no table, 0x1A start without
// saving. All file I/O goes through the game's file wrappers (0x42D040 etc.); the shell
// calls (SHGetFolderPathA, DeleteFileA, GetLastError) through their import slots.

#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// a slot's summary (SaveSlotInfo, set by 0x7350E0)
struct SmSlot {
    uint8_t used;        // +0x0
    uint8_t pad1[3];
    float playTime;      // +0x4
    int32_t info[6];     // +0x8 chapter, area, yearbook, grades
    int32_t year;        // +0x20
    int32_t month;       // +0x24
    int32_t day;         // +0x28
    int32_t hour;        // +0x2C
    int32_t minute;      // +0x30
    int32_t second;      // +0x34
};
static_assert(sizeof(SmSlot) == 0x38);

struct SaveManager {
    char dir[0x104];         // +0x0 the save folder, ending in '\'
    SmSlot slots[6];         // +0x104 (the table file: slots and lastSlot)
    int32_t lastSlot;        // +0x254 the slot saved last
    int32_t error;           // +0x258 the Win32 error (0x16 if none)
    int32_t spaceNeededKb;   // +0x25C
    int32_t unk260;          // +0x260
    int32_t state;           // +0x264
    uint8_t confirmed;       // +0x268 (the first request of a save/load/delete only asks)
    uint8_t needTable;       // +0x269
    uint8_t pad26A[2];
    int32_t slot;            // +0x26C
    uint8_t loaded;          // +0x270
    uint8_t pad271[3];
    int32_t failKind;        // +0x274 0 format, 1 load, 2 save, 3 delete
    uint8_t overwrote;       // +0x278
    uint8_t started;         // +0x279
    uint8_t pad27A[2];
    uint32_t deadline;       // +0x27C
    int32_t checkSlot;       // +0x280 (the slot the check walks, -1 when done)
    int32_t retryError;      // +0x284
};
static_assert(offsetof(SaveManager, slots) == 0x104);
static_assert(offsetof(SaveManager, lastSlot) == 0x254);
static_assert(offsetof(SaveManager, state) == 0x264);
static_assert(offsetof(SaveManager, slot) == 0x26C);
static_assert(offsetof(SaveManager, failKind) == 0x274);
static_assert(offsetof(SaveManager, deadline) == 0x27C);
static_assert(sizeof(SaveManager) == 0x288);

// the photo gallery's header
struct SmGalleryHeader {
    int32_t count;       // +0x0
    uint8_t used[15];    // +0x4
    int8_t order[15];    // +0x13
    uint8_t rest[0x1E];  // +0x22
};
static_assert(sizeof(SmGalleryHeader) == 0x40);

constexpr int kBlockSize = 0x56F4;
constexpr int kTableSize = 0x154;
constexpr int kPhotoSize = 0x14000;
constexpr int kGallerySize = 0x12C040;
constexpr int kPathSize = 0x204;

constexpr uintptr_t kGetLastError = 0x008FC060;
constexpr uintptr_t kDeleteFileA = 0x008FC094;
constexpr uintptr_t kSHGetFolderPathA = 0x008FC204;

GAME_VAR(SaveManager*, smManager, 0x020CE75C);
GAME_VAR(int32_t, smSaveMode, 0x020CE760);          // 2 saving, 1 loading
GAME_ARRAY(uint16_t, smMessage, 0x020CE768);       // 0x400
GAME_ARRAY(const char*, smSlotNames, 0x00BC7430);  // "BullyFile1".."BullyFile6"
GAME_VAR(const char*, smTableName, 0x00BC742C);    // "FileTableBully"
GAME_VAR(const char*, smGalleryName, 0x00BC73F4);  // "Gallery"
GAME_ARRAY(uint8_t, smSaveBlock, 0x020C9068);
GAME_VAR(int32_t, smPhotoCount, 0x00AB46E8);
GAME_ARRAY(uint8_t, smDefaultPhoto, 0x00BD7800);
GAME_VAR(void*, smMainCamera, 0x00C1AE7C);
inline void* const smText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const smMenuInput = reinterpret_cast<void*>(0x00A45A30);
GAME_VAR(float, smTextWrap, 0x0208B778);
GAME_ARRAY(uint8_t, smShadowColour, 0x0208B780);
GAME_VAR(uint16_t, smShadowOn, 0x0208B784);
GAME_ARRAY(uint8_t, smHiliteColour, 0x020C331C);
GAME_VAR(float, smDefaultWrap, 0x00918EF8);
GAME_VAR(int32_t, smLanguage, 0x00C18724);
GAME_VAR(int32_t, smRegion, 0x00C18728);
GAME_VAR(float, smHiliteOffset, 0x020CEB68);
GAME_VAR(float, smRectTop, 0x00BA689C);
GAME_ARRAY(float, smEmptyRect, 0x00BA6898);
GAME_VAR(uint8_t, smRetryChosen, 0x020CEB70);
GAME_VAR(int32_t, smSelection, 0x020CEB6C);
GAME_VAR(int32_t, smPromptState, 0x00BC7484);
GAME_VAR(int32_t, smPromptColour, 0x00BC7480);
GAME_VAR(uint8_t, smFontReset, 0x0208B264);
GAME_VAR(uint8_t, smLoadedA, 0x020C32EE);
GAME_VAR(uint8_t, smLoadedB, 0x020C32EC);

// (the game's own functions: called through their addresses)
GAME_FN(int, __cdecl, Sm_GetState, 0x00735370);
GAME_FN(void, __cdecl, Sm_SetError, 0x00735390, uint32_t error);
GAME_FN(const uint16_t*, __fastcall, Sm_GetMessage, 0x007354E0, const void* fallback);
GAME_FN(int, __thiscall, Sm_Open, 0x00735750, const char* dir, const char* name, const char* mode);
GAME_FN(uint8_t, __cdecl, Sm_Exists, 0x007357A0, const char* name);
GAME_FN(uint8_t, __thiscall, Sm_WriteTable, 0x00735810, SaveManager* self);
GAME_FN(uint8_t, __cdecl, Sm_ReadSlot, 0x007358F0, int slot);
GAME_FN(void, __thiscall, Sm_CheckSlots, 0x00735BA0, SaveManager* self);
GAME_FN(uint8_t, __cdecl, Sm_AddPhoto, 0x00735C60, const void* data, int size);
GAME_FN(void, __cdecl, Sm_DrawRetry, 0x007362B0, const char* key, int choice);
GAME_FN(void, __thiscall, Sm_UpdateSpace, 0x007364F0, SaveManager* self, uint8_t checkTable, uint8_t checkGallery);
GAME_FN(uint8_t, __thiscall, Sm_CreateGallery, 0x00736A00, SaveManager* self, uint8_t addDefault);
GAME_FN(void, __thiscall, Sm_ReadTable, 0x00736C60, SaveManager* self, uint8_t read, uint8_t create);
GAME_FN(uint8_t, __cdecl, Sm_GetPhotoCount, 0x00736FF0, int32_t* count, uint8_t create);
GAME_FN(int, __cdecl, Sm_NewestSlot, 0x007373D0, uint8_t create);
GAME_FN(uint8_t, __thiscall, Sm_AnyUsed, 0x00735490, SaveManager* self);
GAME_FN(uint8_t, __thiscall, Sm_LoadPrompt, 0x00737520, SaveManager* self);
GAME_FN(void, __thiscall, Sm_Load, 0x00737200, SaveManager* self);
GAME_FN(void, __thiscall, Sm_Save, 0x00736690, SaveManager* self);
GAME_FN(void, __thiscall, Sm_Delete, 0x00735A20, SaveManager* self);
GAME_FN(void, __thiscall, Sm_RefreshTable, 0x007365C0, SaveManager* self);
GAME_FN(void, __cdecl, Sm_SaveBlock, 0x00734D60, uint8_t* block);
GAME_FN(void, __cdecl, Sm_CheckBlock, 0x007344D0, uint8_t* block, uint8_t* valid, uint8_t* corrupt);
GAME_FN(void, __thiscall, Sm_SlotSet, 0x007350E0, SmSlot* self, uint8_t* block, float playTime);
GAME_FN(void, __thiscall, Sm_SlotClear, 0x00735190, SmSlot* self);

// files and memory
GAME_FN(int, __cdecl, Sm_FileOpen, 0x0042D040, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, Sm_FileRead, 0x0042D2A0, int file, void* to, int size);
GAME_FN(int, __cdecl, Sm_FileWrite, 0x0042D2D0, int file, const void* from, int size);
GAME_FN(void, __cdecl, Sm_FileClose, 0x0042D450, int file);
GAME_FN(void*, __cdecl, Sm_StreamOpen, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, Sm_StreamRead, 0x0042D0E0, void* to, int size, int count, void* file);
GAME_FN(int, __cdecl, Sm_StreamClose, 0x0042D140, void* file);
inline int __cdecl Sm_StrcpyS(char* to, uint32_t size, const char* from) { return crt::StrcpyS(to, (size_t)size, from); }
inline int __cdecl Sm_StrcatS(char* to, uint32_t size, const char* from) { return crt::StrcatS(to, (size_t)size, from); }
inline int __cdecl Sm_Mkdir(const char* path) { return crt::Mkdir(path); }
inline void* __cdecl Sm_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
inline void* __cdecl Sm_Memcpy(void* to, const void* from, uint32_t size) { return crt::Memcpy(to, from, (size_t)size); }
inline void* __cdecl Sm_CrtMalloc(uint32_t size) { return crt::Malloc((size_t)size); }
inline void __cdecl Sm_CrtFree(void* p) { crt::Free(p); }
GAME_FN(void*, __cdecl, Sm_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Sm_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, Sm_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, Sm_PopMemoryTag, 0x005EF100);
GAME_FN(uint32_t, __cdecl, Sm_GetTime, 0x0045BCA0);
GAME_FN(float, __cdecl, Sm_GetCompletion, 0x006A98C0);

// the camera (the save shuts the frame around the file access)
GAME_FN(void*, __cdecl, Sm_CameraGetCurrent, 0x006C8370);
GAME_FN(void, __cdecl, Sm_CameraEndUpdate, 0x006C86C0, void* camera);
GAME_FN(void, __cdecl, Sm_CameraShowRaster, 0x006C8350, void* camera, int, int);
GAME_FN(void, __cdecl, Sm_CameraBeginUpdate, 0x006C86A0, void* camera);

// text and the frontend
GAME_FN(const uint16_t*, __thiscall, Sm_TextGet, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Sm_FontAddString, 0x006880F0, const uint16_t* text);
GAME_FN(void, __cdecl, Sm_FontAddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Sm_FontFormat, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(float, __cdecl, Sm_FontLineHeight, 0x00688220);
GAME_FN(void, __cdecl, Sm_FontReset, 0x0068A250);
GAME_FN(void, __cdecl, Sm_DrawKeyAtCentred, 0x006A6630, const void* position, const char* key, const void* style);
GAME_FN(void, __cdecl, Sm_DrawTextAtCentred, 0x006A65F0, const void* position, const uint16_t* text, const void* style);
GAME_FN(void, __cdecl, Sm_DrawTextKey, 0x006A6380, int x, int y, const char* key, int font, int justify, int, int);
GAME_FN(void, __cdecl, Sm_MeasureTextKey, 0x006A63F0, float* rect, int x, int y, const char* key, int font, int justify,
        int, int);
GAME_FN(void, __cdecl, Sm_MeasureText, 0x006A54D0, float* rect, int x, int y, const uint16_t* text, int font, int justify,
        int, int);
GAME_FN(void, __cdecl, Sm_DrawText, 0x006A5450, int x, int y, const uint16_t* text, int font, int justify, int, int);
GAME_FN(void, __cdecl, Sm_SetTextStyle, 0x006A3FC0, int, int, int, int);
GAME_FN(void, __cdecl, Sm_DrawControls, 0x006A6510, const char* key);
GAME_FN(void, __cdecl, Sm_DrawBackdrop, 0x006A2E00);
GAME_FN(void, __cdecl, Sm_LoadTextures, 0x006A2B00);
GAME_FN(void, __cdecl, Sm_UnloadTextures, 0x006A2D50);
GAME_FN(uint8_t, __cdecl, Sm_BeginFrame, 0x0043AB30, int, int, int, int, int, int, int, int, int, int, int, int, int);
GAME_FN(void, __cdecl, Sm_EndFrame, 0x0043ACE0, int);
GAME_FN(void, __cdecl, Sm_SetRenderState, 0x00409520, int, int);
GAME_FN(void, __cdecl, Sm_BeginMenuFrame, 0x00409860);
GAME_FN(void, __cdecl, Sm_EndMenuFrame, 0x004098D0);
GAME_FN(void, __cdecl, Sm_PadsUpdate, 0x00738610);
GAME_FN(uint8_t, __cdecl, Sm_PadsReady, 0x00738820);
GAME_FN(uint8_t, __cdecl, Sm_PadJustDown, 0x00738900, int control, int pad);
GAME_FN(uint8_t, __thiscall, Sm_InputUp, 0x0050E0A0, void* input, int pad);
GAME_FN(uint8_t, __thiscall, Sm_InputDown, 0x0050E110, void* input, int pad);

SmSlot& SlotAt(SaveManager* m, int i) {
    return m->slots[i];
}

// path = dir + name
void MakePath(char* path, const char* dir, const char* name) {
    Sm_StrcpyS(path, kPathSize, dir);
    Sm_StrcatS(path, kPathSize, name);
}

// 0x735390 inlined: the last Win32 error (0x16 when there is none) into the manager
void FailWithLastError() {
    uint32_t error = vc::Import<uint32_t>(kGetLastError);
    if (error == 0) {
        error = 0x16;
    }
    if (smManager->state != 5) {
        smManager->state = 5;
        smManager->error = static_cast<int32_t>(error);
    }
}

// bytes to kilobytes as the game does (through a 64-bit integer and a double)
int Kilobytes(long long bytes) {
    return static_cast<int>(static_cast<double>(bytes) * 0.0009765625);
}

bool TableStateWritable(int state) {
    return state == 1 || state == 3 || state == 0x19 || state == 5;
}

// check the slots, then (re)write an empty table if the state allows it
uint8_t CheckAndWriteTable(SaveManager* m) {
    Sm_CheckSlots(m);
    if (TableStateWritable(m->state)) {
        return Sm_WriteTable(m);
    }
    return 0;
}

void EndCamera(void* camera) {
    Sm_CameraEndUpdate(camera);
    Sm_CameraShowRaster(camera, 0, 0);
}

void BeginFullscreenFrame() {
    Sm_BeginFrame(0, 0, 0, 0, 0, 0, 0xFF, 0, 0, 0, 0x280, 0x1E0, 1);
    Sm_SetRenderState(7, 1);
}

// "creating the table" in the middle of the screen
void DrawWritingNotice() {
    smTextWrap = smDefaultWrap;
    Sm_SetTextStyle(3, 2, 2, 1);
    Sm_FontLineHeight();
    float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    Sm_MeasureTextKey(rect, 0x140, 0x78, "FESZ_WR", 3, 2, 3, 1);
    Sm_DrawTextKey(0x140, 0x78, "FESZ_WR", 3, 2, 2, 1);
    Sm_DrawControls("MC_CONFIRM");
}

void WaitUntil(uint32_t deadline) {
    while (Sm_GetTime() < deadline) {
    }
}

} // namespace

namespace savemgr {

// 0x736930: a prompt (key in esi) with up to two numbers (the first when bl is 1) and the
// controls line. Register arguments: written here, not hooked.
void DrawConfirm(uint8_t withNumber, const char* key, const char* controls, int number, uint8_t withNumber2,
                 int number2) {
    smTextWrap = smDefaultWrap;
    Sm_SetTextStyle(3, 2, 2, 1);
    Sm_FontLineHeight();
    if (withNumber == 1) {
        Sm_FontAddNumber(static_cast<float>(number));
    }
    if (withNumber2 == 1) {
        Sm_FontAddNumber(static_cast<float>(number2));
    }
    float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    Sm_MeasureTextKey(rect, 0x140, 0x78, key, 3, 2, 3, 1);
    if (withNumber == 1) {
        Sm_FontAddNumber(static_cast<float>(number));
    }
    Sm_DrawTextKey(0x140, 0x78, key, 3, 2, 2, 1);
    Sm_DrawControls(controls);
}

// 0x7351C0: create the manager and find the save folder (creating it)
void __cdecl Init() {
    SaveManager* m = smManager;
    if (m == nullptr) {
        m = static_cast<SaveManager*>(Sm_Malloc(sizeof(SaveManager)));
        if (m != nullptr) {
            m->state = 0x18;
            m->needTable = 1;
            m->loaded = 0;
            m->overwrote = 0;
        } else {
            m = nullptr;
        }
        smManager = m;
    }
    Sm_Memset(&m->slots, 0, kTableSize);
    smManager->error = 0;
    smManager->spaceNeededKb = Kilobytes(static_cast<long long>(1251464.0f));
    smManager->state = 0x18;
    smManager->confirmed = 0;
    smManager->needTable = 1;
    smManager->slot = 0;
    smManager->loaded = 0;
    smManager->failKind = 1;
    smManager->overwrote = 0;
    smManager->started = 0;
    smManager->deadline = 0;
    smManager->checkSlot = -1;
    smManager->retryError = -1;
    vc::Import<long>(kSHGetFolderPathA, static_cast<void*>(nullptr), 5, static_cast<void*>(nullptr), 0u,
                     smManager->dir);   // CSIDL_PERSONAL
    Sm_StrcatS(smManager->dir, 0x104, "\\");
    Sm_StrcatS(smManager->dir, 0x104, "Bully Scholarship Edition");
    Sm_StrcatS(smManager->dir, 0x104, "\\");
    Sm_Mkdir(smManager->dir);
}

// 0x735370
int __cdecl GetState() {
    SaveManager* m = smManager;
    return m != nullptr ? m->state : 0;
}

// 0x735390
void __cdecl SetError(uint32_t error) {
    if (error == 0) {
        error = 0x16;
    }
    if (smManager->state != 5) {
        smManager->state = 5;
        smManager->error = static_cast<int32_t>(error);
    }
}

// 0x7353D0
int __cdecl GetSpaceNeeded() {
    return smManager->spaceNeededKb;
}

// 0x7353E0: the frontend asks for the slot's operation
void __cdecl Request(int slot) {
    smManager->confirmed = 0;
    smManager->slot = slot;
    SaveManager* m = smManager;
    int state = m->state;
    if (state == 8) {
        m->state = 3;
    } else if (state == 7) {
        uint32_t time = Sm_GetTime();
        smManager->deadline = time + 3000;
        smManager->state = 2;
    } else if (state == 9) {
        m->state = 4;
    }
}

// 0x735460
void __cdecl BeginSaveMode() {
    SaveManager* m = smManager;
    smSaveMode = 2;
    if (m->state != 5) {
        m->state = 8;
    }
}

// 0x735490
uint8_t __fastcall AnyUsed(SaveManager* self, re::Edx) {
    for (int i = 0; i < 6; i++) {
        if (self->slots[i].used == 1) {
            return 1;
        }
    }
    return 0;
}

// 0x7354B0
SmSlot* __cdecl SlotInfo(int i) {
    if (i >= 6) {
        return nullptr;
    }
    return &smManager->slots[i];
}

// 0x7354E0: the error message (the failed operation and the reason) into 0x20CE768.
// Out-of-range operations use ecx as the text key (the callers leave anything there).
const uint16_t* __fastcall GetMessage(const char* fallback) {
    const char* kind;
    switch (smManager->failKind) {
    case 0: kind = "FR_FAIL"; break;
    case 1: kind = "LD_FAIL"; break;
    case 2: kind = "SV_FAIL"; break;
    case 3: kind = "DL_FAIL"; break;
    default: kind = fallback; break;
    }
    Sm_FontAddString(Sm_TextGet(smText, kind));
    SaveManager* m = smManager;
    const char* reason;
    switch (m->error) {
    case 2:
    case 3: reason = "ERR_NOT_FOUND"; break;
    case 8:
    case 0x70:
        reason = "ERR_FULL";
        Sm_FontAddNumber(static_cast<float>(m->spaceNeededKb));
        break;
    case 0xF: reason = "FES_INVC"; break;
    case 0x1D: reason = "ERR_WRITE"; break;
    case 0x1E:
    case 0x26:
    case 0x570:
    case 0x65B: reason = "ERR_OVERWRITE"; break;
    case 0x48F:
    case 0x651: reason = "MD_DISCONNECTED"; break;
    case 0x8CA:
        if (smSaveMode == 2) {
            reason = "MD_SAVE_SIGNIN";
        } else if (smSaveMode == 1) {
            reason = "MD_LOAD_SIGNIN";
        } else {
            reason = "MD_SIGNIN";
        }
        break;
    default: reason = "ERR_CHANGE"; break;
    }
    Sm_FontFormat(smMessage, Sm_TextGet(smText, reason), 0x400);
    return smMessage;
}

// 0x735660: the state's message box; false when there is none
uint8_t __cdecl DrawMessage(const void* position, const void* style) {
    int state = smManager != nullptr ? smManager->state : 0;
    switch (state) {
    case 2:
    case 0x15: Sm_DrawKeyAtCentred(position, "FELD_WR", style); return 1;
    case 4:
    case 0x11: Sm_DrawKeyAtCentred(position, "FEDL_WR", style); return 1;
    case 3:
    case 0x10:
    case 0x14: Sm_DrawKeyAtCentred(position, "FESZ_WR", style); return 1;
    case 5: Sm_DrawTextAtCentred(position, Sm_GetMessage(style), style); return 1;
    case 6: return 1;
    case 0x19: Sm_DrawKeyAtCentred(position, "FELD_ND", style); return 1;
    default: return 0;
    }
}

// 0x735750: open a file in the folder (this = the folder)
int __fastcall Open(const char* dir, re::Edx, const char* name, const char* mode) {
    char path[kPathSize];
    MakePath(path, dir, name);
    return Sm_FileOpen(path, mode, 0);
}

// 0x7357A0
uint8_t __cdecl Exists(const char* name) {
    char path[kPathSize];
    MakePath(path, smManager->dir, name);
    int file = Sm_FileOpen(path, "r", 0);
    if (file > 0) {
        Sm_FileClose(file);
        return 1;
    }
    return 0;
}

// 0x735810: write a new (empty) table
uint8_t __fastcall WriteTable(SaveManager* self, re::Edx) {
    char path[kPathSize];
    Sm_Memset(&self->slots, 0, kTableSize);
    self->failKind = 2;
    const char* name = smTableName;
    uint8_t ok = 0;
    MakePath(path, self->dir, name);
    int file = Sm_FileOpen(path, "wb", 0);
    if (file > 0) {
        if (Sm_FileWrite(file, &self->slots, kTableSize) == kTableSize) {
            ok = 1;
        }
        Sm_FileClose(file);
        if (ok) {
            return ok;
        }
    }
    FailWithLastError();
    return ok;
}

// the read block's check: valid and not corrupt
bool BlockIsGood() {
    uint8_t valid = 1;
    uint8_t corrupt = 0;
    Sm_CheckBlock(smSaveBlock, &valid, &corrupt);
    return valid != 0 && corrupt == 0;
}

// 0x7358F0: read a slot's file into the save block and check it
uint8_t __cdecl ReadSlot(int slot) {
    char path[kPathSize];
    smManager->failKind = 1;
    SaveManager* m = smManager;
    uint8_t ok = 0;
    if (m->slots[slot].used == 0) {
        return ok;
    }
    MakePath(path, m->dir, smSlotNames[slot]);
    int file = Sm_FileOpen(path, "r", 0);
    bool read = false;
    if (file > 0) {
        if (Sm_FileRead(file, smSaveBlock, kBlockSize) == kBlockSize) {
            ok = 1;
        }
        Sm_FileClose(file);
        read = ok != 0;
    }
    if (!read) {
        FailWithLastError();
    }
    if (!BlockIsGood()) {
        smManager->state = 5;
        return 0;
    }
    return ok;
}

// 0x735A20: delete the slot's file (the first request only asks)
void __fastcall Delete(SaveManager* self, re::Edx) {
    char path[kPathSize];
    if (self->slots[self->slot].used != 0) {
        if (self->confirmed == 0) {
            self->confirmed = 1;
            return;
        }
        void* camera = Sm_CameraGetCurrent();
        if (camera != nullptr) {
            EndCamera(camera);
        }
        self->failKind = 3;
        smManager->state = 4;
        MakePath(path, self->dir, smSlotNames[self->slot]);
        if (vc::Import<int>(kDeleteFileA, static_cast<const char*>(path)) != 0) {
            SmSlot& s = self->slots[self->slot];
            s.playTime = 0.0f;
            s.used = 0;
            for (int i = 0; i < 6; i++) {
                s.info[i] = 0;
            }
            s.year = 0;
            s.month = 0;
            s.day = 0;
            s.hour = 0;
            s.minute = 0;
            s.second = 0;
            if (self->slot == self->lastSlot) {
                self->lastSlot = 0;
                for (int i = 0; i < 6; i++) {
                    if (self->slots[i].used != 0) {
                        self->lastSlot = i;
                        break;
                    }
                }
            }
        } else {
            smManager->state = 5;
        }
        if (camera != nullptr) {
            Sm_CameraBeginUpdate(smMainCamera);
        }
    }
    SaveManager* m = smManager;
    if (m->state != 5) {
        m->state = 0x11;
    }
}

// 0x735BA0: check one used slot's file a call (clearing slots whose file is gone)
void __fastcall CheckSlots(SaveManager* self, re::Edx) {
    int state = self->state;
    if (state == 5 || self->started == 0) {
        return;
    }
    if (state == 6) {
        self->state = 1;
        self->needTable = 1;
    }
    int i = self->checkSlot;
    if (i == -1) {
        return;
    }
    if (self->slots[i].used != 0 && !Sm_Exists(smSlotNames[i])) {
        Sm_SlotClear(&self->slots[self->checkSlot]);
        if (self->lastSlot == self->checkSlot) {
            self->lastSlot = 0;
        }
    }
    self->checkSlot++;
    if (self->checkSlot == 6) {
        self->checkSlot = -1;
    }
}

// read the gallery file into a new buffer (null when it can't be opened, the error set
// when it can't be read)
uint8_t* ReadGallery(char* path, bool& opened) {
    MakePath(path, smManager->dir, smGalleryName);
    int file = Sm_FileOpen(path, "r", 0);
    opened = file > 0;
    if (!opened) {
        return nullptr;
    }
    uint8_t* data = static_cast<uint8_t*>(Sm_CrtMalloc(kGallerySize));
    bool ok = Sm_FileRead(file, data, kGallerySize) == kGallerySize;
    Sm_FileClose(file);
    if (!ok) {
        FailWithLastError();
        Sm_CrtFree(data);
        return nullptr;
    }
    return data;
}

// 0x735C60: add a photo (up to 0x14000 bytes) to the gallery
uint8_t __cdecl AddPhoto(const void* photo, int size) {
    char path[kPathSize];
    if (size > kPhotoSize) {
        size = kPhotoSize;
    }
    smManager->failKind = 2;
    bool opened;
    uint8_t* data = ReadGallery(path, opened);
    if (data == nullptr) {
        return 0;
    }
    SmGalleryHeader header;
    Sm_Memcpy(&header, data, sizeof(header));   // (rep movsd)
    int count = header.count;
    int i = 0;
    if (count != 0) {
        while (header.used[i] != 0) {
            i++;
            if (i >= 15) {
                Sm_CrtFree(data);
                return 0;
            }
        }
    }
    uint8_t* to = data + 0x40 + i * kPhotoSize;
    Sm_Memcpy(to, photo, size);
    if (size < kPhotoSize) {
        Sm_Memset(to + size, 0, kPhotoSize - size);
    }
    header.count++;
    header.used[i] = 1;
    header.order[i] = static_cast<int8_t>(count);
    Sm_Memcpy(data, &header, sizeof(header));
    MakePath(path, smManager->dir, smGalleryName);
    int file = Sm_FileOpen(path, "w", 0);
    if (file <= 0) {
        return 0;   // (the game leaks the buffer here)
    }
    uint8_t ok = Sm_FileWrite(file, data, kGallerySize) == kGallerySize;
    if (!ok) {
        Sm_SetError(vc::Import<uint32_t>(kGetLastError));
    }
    Sm_CrtFree(data);
    Sm_FileClose(file);
    return ok;
}

// 0x735EB0: copy the photo shown n-th into out (0x14000 bytes)
uint8_t __cdecl GetPhoto(int order, void* out) {
    char path[kPathSize];
    smManager->failKind = 1;
    MakePath(path, smManager->dir, smGalleryName);
    int file = Sm_FileOpen(path, "r", 0);
    if (file <= 0) {
        return 0;
    }
    uint8_t* data = static_cast<uint8_t*>(Sm_CrtMalloc(kGallerySize));
    if (Sm_FileRead(file, data, kGallerySize) == kGallerySize) {
        SmGalleryHeader header;
        Sm_Memcpy(&header, data, sizeof(header));
        for (int i = 0; i < 15; i++) {
            if (header.used[i] == 1 && order == header.order[i]) {
                Sm_Memcpy(out, data + 0x40 + i * kPhotoSize, kPhotoSize);
                Sm_CrtFree(data);
                Sm_FileClose(file);
                return 1;
            }
        }
    } else {
        FailWithLastError();
    }
    Sm_CrtFree(data);
    Sm_FileClose(file);
    return 0;
}

// 0x736000: remove the photo shown n-th (the later ones move up)
uint8_t __cdecl DeletePhoto(int order) {
    char path[kPathSize];
    smManager->failKind = 3;
    bool opened;
    uint8_t* data = ReadGallery(path, opened);
    if (data == nullptr) {
        return 0;
    }
    SmGalleryHeader header;
    Sm_Memcpy(&header, data, sizeof(header));
    int i = 0;
    while (!(header.used[i] == 1 && order == header.order[i])) {
        i++;
        if (i >= 15) {
            Sm_CrtFree(data);
            return 0;
        }
    }
    for (int j = 0; j < 15; j++) {
        if (header.used[j] == 1 && header.order[j] > order) {
            header.order[j]--;
        }
    }
    header.count--;
    header.used[i] = 0;
    header.order[i] = -1;
    Sm_Memcpy(data, &header, sizeof(header));
    Sm_Memset(data + 0x40 + i * kPhotoSize, 0, kPhotoSize);
    MakePath(path, smManager->dir, smGalleryName);
    int file = Sm_FileOpen(path, "w", 0);   // (not checked)
    uint8_t ok = Sm_FileWrite(file, data, kGallerySize) == kGallerySize;
    if (!ok) {
        FailWithLastError();
    }
    Sm_CrtFree(data);
    Sm_FileClose(file);
    return ok;
}

// 0x7362B0: the key over a YES/NO choice (1 = YES)
void __cdecl DrawRetry(const char* key, int choice) {
    for (int i = 0; i < 4; i++) {
        smShadowColour[i] = smHiliteColour[i];
    }
    smTextWrap = smDefaultWrap;
    smShadowOn = 1;
    if (smLanguage == 7) {
        smHiliteOffset = -7.0f;
    }
    int y = choice == 1 ? 320 : 341;
    int x = static_cast<int>(320.0f + smHiliteOffset);
    Sm_DrawTextKey(x, y, "FESA_HILITE", 3, 2, 2, 1);
    Sm_DrawTextKey(0x140, 0x155, "NO", choice != 0 ? 11 : 3, 2, 2, choice == 0);
    Sm_DrawTextKey(0x140, 0x140, "YES", choice != 1 ? 11 : 3, 2, 2, choice == 1);
    float top = smRectTop;
    float rect[4] = {smEmptyRect[0], smEmptyRect[1], smEmptyRect[2], smEmptyRect[3]};
    Sm_MeasureTextKey(rect, 0x140, 0x10E, key, 3, 2, 2, 1);
    float height = rect[3] - rect[1];
    Sm_SetTextStyle(3, 2, 2, 1);
    float lineHeight = Sm_FontLineHeight();
    float textY = top - lineHeight * 0.5f - height;
    if (smRegion == 2) {
        textY = textY + 16.0f;
    }
    Sm_DrawTextKey(0x140, static_cast<int>(textY), key, 3, 2, 2, 1);
}

// 0x736480
uint8_t __fastcall TableExists(void*, re::Edx, int) {
    return Sm_Exists(smTableName);
}

// 0x7364A0
uint8_t __fastcall FormatTable(SaveManager* self, re::Edx, uint8_t toSave) {
    Sm_CheckSlots(self);
    if (!TableStateWritable(self->state) || !Sm_WriteTable(self)) {
        return 0;
    }
    if (toSave) {
        smManager->state = 3;
    }
    return 1;
}

// 0x7364F0: the space a save would need (the table, a slot's file, the gallery)
void __fastcall UpdateSpace(SaveManager* self, re::Edx, uint8_t checkTable, uint8_t checkGallery) {
    int need = 0;
    if (checkTable && !Sm_Exists(smTableName)) {
        need = 0x5848;
    } else {
        for (int i = 0; i < 6; i++) {
            if (self->slots[i].used == 0) {
                need = kBlockSize;
                break;
            }
        }
    }
    if (checkGallery && !Sm_Exists(smGalleryName)) {
        need += kGallerySize;
    }
    if (need != 0) {
        self->spaceNeededKb = Kilobytes(need);
    } else {
        self->spaceNeededKb = 0;
    }
}

// 0x7365C0: write the table after a save or delete
void __fastcall RefreshTable(SaveManager* self, re::Edx) {
    char path[kPathSize];
    const char* name = smTableName;
    uint8_t ok = 0;
    MakePath(path, self->dir, name);
    int file = Sm_FileOpen(path, "wb", 0);
    if (file > 0) {
        ok = Sm_FileWrite(file, &self->slots, kTableSize) == kTableSize;
        Sm_FileClose(file);
    }
    if (ok) {
        SaveManager* m = smManager;
        if (m->state == 0x10) {
            m->state = 0x14;
        } else {
            m->state = m->state == 0x11 ? 0x17 : 6;
        }
    } else {
        self->state = 5;
    }
    Sm_UpdateSpace(self, ok == 0, 1);
}

// 0x736690: save into the slot (the first request only asks)
void __fastcall Save(SaveManager* self, re::Edx) {
    if (self->confirmed == 0) {
        self->confirmed = 1;
        return;
    }
    self->deadline = Sm_GetTime() + 3000;
    void* camera = Sm_CameraGetCurrent();
    if (camera != nullptr) {
        EndCamera(camera);
    }
    self->failKind = 2;
    if (!Sm_Exists(smTableName)) {
        SaveManager* m = smManager;
        if (!CheckAndWriteTable(m)) {
            if (camera != nullptr) {
                Sm_CameraBeginUpdate(smMainCamera);
            }
            return;
        }
        smManager->state = 3;
    }
    smManager->state = 3;
    smManager->overwrote = 0;
    uint8_t ok = 0;
    Sm_PushMemoryTag(0x2A);
    uint8_t* block = static_cast<uint8_t*>(Sm_Malloc(kBlockSize));
    if (block != nullptr) {
        // (the block's constructor)
        float* f = reinterpret_cast<float*>(block);
        f[0x3C / 4] = f[0x40 / 4] = f[0x44 / 4] = 0.0f;
        f[0x4C / 4] = f[0x50 / 4] = f[0x54 / 4] = 0.0f;
    }
    Sm_PopMemoryTag();
    Sm_Memset(block, 0, kBlockSize);
    Sm_SaveBlock(block);
    int file = Sm_Open(self->dir, smSlotNames[self->slot], "wb");
    if (file > 0) {
        if (Sm_FileWrite(file, block, kBlockSize) == kBlockSize) {
            ok = 1;
        }
        Sm_FileClose(file);
    }
    if (ok) {
        smManager->overwrote = self->slots[self->slot].used;
        float completion = Sm_GetCompletion();
        Sm_SlotSet(&self->slots[self->slot], block, completion);
        self->lastSlot = self->slot;
        smManager->state = 0x10;
    } else {
        Sm_SetError(vc::Import<uint32_t>(kGetLastError));
    }
    Sm_Free(block);
    if (camera != nullptr) {
        Sm_CameraBeginUpdate(smMainCamera);
    }
    Sm_UpdateSpace(self, 0, 1);
}

// 0x7368C0: after an overwrite error, whether the file is back (then retry)
uint8_t __cdecl RetryAfterError(uint8_t gallery) {
    int error = smManager->retryError;
    if (error != 2 && error != 3 && error != 0x1E && error != 0x26 && error != 0x570 && error != 0x65B) {
        return 0;
    }
    if (!Sm_Exists(gallery ? smGalleryName : smTableName)) {
        return 0;
    }
    smManager->retryError = -1;
    return 1;
}

// 0x736A00: write an empty gallery (and the default photo, config/dat/kewl.dat)
uint8_t __fastcall CreateGallery(SaveManager* self, re::Edx, uint8_t addDefault) {
    char path[kPathSize];
    SmGalleryHeader header;
    header.count = 0;
    self->failKind = 2;
    for (int i = 0; i < 15; i++) {
        header.used[i] = 0;
        header.order[i] = -1;
    }
    for (uint8_t& b : header.rest) {
        b = 0;   // (left uninitialised by the game)
    }
    uint8_t* data = static_cast<uint8_t*>(Sm_CrtMalloc(kGallerySize));
    if (data == nullptr) {
        return 0;
    }
    Sm_Memset(data, 0, kGallerySize);
    Sm_Memcpy(data, &header, sizeof(header));
    uint8_t ok = 0;
    MakePath(path, self->dir, smGalleryName);
    int file = Sm_FileOpen(path, "wb", 0);
    if (file > 0) {
        if (Sm_FileWrite(file, data, kGallerySize) == kGallerySize) {
            ok = 1;
        }
        Sm_FileClose(file);
    }
    // (the buffer is never freed)
    if (!ok) {
        FailWithLastError();
        return 0;
    }
    if (addDefault) {
        char name[0x14] = "config/dat/kewl.dat";
        void* stream = Sm_StreamOpen(name, "rt", 1);
        if (stream == nullptr) {
            return 0;
        }
        int read = Sm_StreamRead(smDefaultPhoto, 1, kPhotoSize, stream);
        Sm_StreamClose(stream);
        if (read <= 0 || !Sm_AddPhoto(smDefaultPhoto, kPhotoSize)) {
            return 0;
        }
    }
    int need;
    if (!Sm_Exists(smTableName)) {
        need = 0x5848;
    } else {
        int i = 0;
        while (self->slots[i].used != 0) {
            i++;
            if (i >= 6) {
                self->spaceNeededKb = 0;
                return 1;
            }
        }
        need = kBlockSize;
    }
    self->spaceNeededKb = Kilobytes(need);
    return 1;
}

// 0x736C60: read the table (creating it, with a notice, when it is missing)
void __fastcall ReadTable(SaveManager* self, re::Edx, uint8_t read, uint8_t create) {
    if (!Sm_Exists(smTableName) && create) {
        Sm_BeginMenuFrame();
        Sm_DrawBackdrop();
        DrawWritingNotice();
        Sm_EndMenuFrame();
        uint32_t deadline = Sm_GetTime() + 3000;
        int state = self->state;
        self->state = 0x19;
        uint8_t written = CheckAndWriteTable(self) != 0;
        WaitUntil(deadline);
        if (!written) {
            return;
        }
        self->state = state;
    }
    self->needTable = 0;
    self->failKind = 1;
    if (read) {
        int file = Sm_Open(self->dir, smTableName, "r");
        if (file > 0) {
            uint8_t ok = Sm_FileRead(file, &smManager->slots, kTableSize) == kTableSize;
            Sm_FileClose(file);
            if (ok) {
                self->checkSlot = 0;
            } else {
                self->state = 5;
            }
            return;
        }
    }
    for (int i = 0; i < 6; i++) {
        SmSlot& s = self->slots[i];
        s.playTime = 0.0f;
        s.used = 0;
        for (int k = 0; k < 6; k++) {
            s.info[k] = 0;
        }
        s.year = 0;
        s.month = 0;
        s.day = 0;
        s.hour = 0;
        s.minute = 0;
        s.second = 0;
        self->lastSlot = 0;
    }
}

// 0x736E70: after an overwrite error, recreate the file (with a notice)
uint8_t __cdecl RecoverFromError(uint8_t gallery) {
    SaveManager* m = smManager;
    if (m == nullptr || m->state != 5) {
        return 0;
    }
    int error = m->error;
    if (error != 0x1E && error != 0x26 && error != 0x570 && error != 0x65B) {
        return 0;
    }
    BeginFullscreenFrame();
    Sm_DrawBackdrop();
    DrawWritingNotice();
    Sm_EndFrame(1);
    uint32_t deadline = Sm_GetTime() + 3000;
    m = smManager;
    if (gallery) {
        Sm_CreateGallery(m, 1);
    } else {
        CheckAndWriteTable(m);
    }
    WaitUntil(deadline);
    smManager->retryError = smManager->error;
    return 1;
}

// 0x736FF0: the gallery's photo count (creating the gallery when asked)
uint8_t __cdecl GetPhotoCount(int32_t* count, uint8_t create) {
    char path[kPathSize];
    smManager->failKind = 1;
    uint8_t ok = 0;
    MakePath(path, smManager->dir, smGalleryName);
    int file = Sm_FileOpen(path, "r", 0);
    if (file > 0) {
        if (Sm_FileRead(file, count, 4) == 4) {
            ok = 1;
        }
        Sm_FileClose(file);
        if (!ok) {
            *count = 0;
            FailWithLastError();
        }
        return ok;
    }
    if (!create) {
        *count = 0;
        return 1;
    }
    BeginFullscreenFrame();
    Sm_DrawBackdrop();
    DrawWritingNotice();
    Sm_EndFrame(1);
    uint32_t deadline = Sm_GetTime() + 3000;
    Sm_CreateGallery(smManager, 1);
    WaitUntil(deadline);
    if (smManager->state == 5) {
        return 0;
    }
    *count = 1;
    return 1;
}

// 0x737200: load the slot into the save block (the first request only asks)
void __fastcall Load(SaveManager* self, re::Edx) {
    char path[kPathSize];
    self->failKind = 1;
    if (self->slots[self->slot].used != 0) {
        if (self->confirmed == 0) {
            self->confirmed = 1;
            return;
        }
        void* camera = Sm_CameraGetCurrent();
        if (camera != nullptr) {
            EndCamera(camera);
        }
    }
    uint8_t ok = 0;
    MakePath(path, self->dir, smSlotNames[self->slot]);
    int file = Sm_FileOpen(path, "r", 0);
    if (file > 0) {
        if (Sm_FileRead(file, smSaveBlock, kBlockSize) == kBlockSize) {
            ok = 1;
        }
        Sm_FileClose(file);
    }
    if (Sm_CameraGetCurrent() != nullptr) {   // (asks again)
        Sm_CameraBeginUpdate(smMainCamera);
    }
    if (ok) {
        if (BlockIsGood()) {
            SaveManager* m = smManager;
            if (m->state == 2) {
                int32_t photos = 0;
                if (Sm_GetPhotoCount(&photos, 0)) {
                    smPhotoCount = photos;
                }
                smManager->state = 0x15;
            } else {
                m->state = 6;
            }
            return;
        }
        smManager->state = 5;
    }
    FailWithLastError();
}

// 0x7373D0: the used slot saved last (by its date), reading the table first
int __cdecl NewestSlot(uint8_t create) {
    Sm_ReadTable(smManager, 1, create);
    SaveManager* m = smManager;
    int newest = m->lastSlot;
    uint32_t best = 0;
    for (int i = 0; i < 6; i++) {
        SmSlot* s = &m->slots[i];
        if (s == nullptr || s->used == 0) {
            continue;
        }
        uint32_t stamp = static_cast<uint32_t>(s->year) * 12 + s->month;
        stamp = ((((stamp * 31 + s->day) * 24 + s->hour) * 60 + s->minute) * 60) + s->second;
        if (stamp > best) {
            newest = i;
            best = stamp;
        }
    }
    return newest;
}

// the RETRY/start-without-saving selection (up = YES)
void UpdateSelection(uint8_t up, uint8_t down) {
    if (down) {
        smSelection = 0;
    }
    if (up) {
        smSelection = 1;
    }
}

// 0x737520: the start-up load prompt, a frame; true when it is done
uint8_t __fastcall LoadPrompt(SaveManager* self, re::Edx) {
    uint8_t accept = Sm_PadJustDown(7, 0);
    self->started = 1;
    Sm_CheckSlots(self);
    if (smRetryChosen == 1) {
        smRetryChosen = 0;
        return 0;
    }
    int state = self->state;
    if (smPromptState != state) {
        smSelection = 0;
        smPromptState = state;
    }
    uint8_t up = Sm_InputUp(smMenuInput, 0);
    uint8_t down = Sm_InputDown(smMenuInput, 0);
    switch (self->state) {
    case 1:
    case 0x19: {
        smPromptColour = 0xCC07C9;
        uint8_t done = 1;
        uint8_t tableFound = 0;
        if (Sm_Exists(smTableName)) {
            Sm_ReadTable(self, 1, 1);
            tableFound = 1;
            if (Sm_AnyUsed(self) == 1) {
                Sm_DrawBackdrop();
                done = 0;
                DrawConfirm(0, "FELD_WR", nullptr, -1, 0, -1);
                self->state = 2;
            }
        }
        Sm_UpdateSpace(self, tableFound == 0, 1);
        return done == 1 ? 1 : 0;
    }
    case 2: {
        void* camera = Sm_CameraGetCurrent();
        uint8_t exists = Sm_Exists(smTableName);
        if (self->state == 5) {
            if (camera != nullptr) {
                Sm_CameraBeginUpdate(smMainCamera);
            }
        } else if (!exists || (Sm_ReadTable(self, 1, 0), !Sm_AnyUsed(self))) {
            if (camera != nullptr) {
                Sm_CameraBeginUpdate(smMainCamera);
            }
            if (self->state != 5) {
                self->state = 0x19;
            }
        } else {
            int slot = Sm_NewestSlot(0);
            int32_t photos;
            uint8_t ok = Sm_GetPhotoCount(&photos, 0);
            if (ok == 1) {
                ok = Sm_ReadSlot(slot);
            }
            if (camera != nullptr) {
                Sm_CameraBeginUpdate(smMainCamera);
            }
            if (ok == 1) {
                Sm_DrawBackdrop();
                DrawConfirm(0, "FELD_WR", nullptr, -1, 0, -1);
                if (camera != nullptr) {
                    EndCamera(camera);
                }
                self->loaded = 1;
                smPhotoCount = photos;
                return 1;
            }
            self->state = 5;
        }
        Sm_DrawBackdrop();
        DrawConfirm(0, "FELD_WR", nullptr, -1, 0, -1);
        if (camera != nullptr) {
            EndCamera(camera);
        }
        return 0;
    }
    case 6:
        Sm_DrawBackdrop();
        DrawConfirm(0, "FELD_WR", nullptr, -1, 0, -1);
        return 0;
    case 0x1A:
        UpdateSelection(up, down);
        Sm_DrawBackdrop();
        Sm_DrawRetry("START_NOSAV", smSelection);
        DrawConfirm(1, "START_NOM_P", "MC_CONFIRM", smManager->spaceNeededKb, 0, -1);
        if (accept && smSelection != 0) {
            return 1;
        }
        return 0;
    default: {
        UpdateSelection(up, down);
        Sm_DrawBackdrop();
        Sm_DrawRetry("RETRY_Q", smSelection);
        float top = smRectTop;
        smTextWrap = smDefaultWrap;
        Sm_SetTextStyle(3, 2, 2, 1);
        float lineHeight = Sm_FontLineHeight();
        float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
        Sm_MeasureText(rect, 0, 0, Sm_GetMessage(nullptr), 3, 2, 2, 1);
        float height = rect[3] - rect[1];
        float y = top - lineHeight * 0.5f - height;
        if (!(y < 120.0f)) {
            y = 120.0f;
        }
        const uint16_t* message = Sm_GetMessage(nullptr);
        Sm_DrawText(0x140, static_cast<int>(y), message, 3, 2, 3, 1);
        Sm_DrawControls("MC_CONFIRM");
        if (!accept) {
            return 0;
        }
        if (smSelection == 1) {
            self->state = 6;
            smRetryChosen = 1;
            return 0;
        }
        return smSelection == 0 ? 1 : 0;
    }
    }
}

// 0x7379A0: run the state's operation (a frame)
void __cdecl Dispatch() {
    SaveManager* m = smManager;
    if (m == nullptr) {
        return;
    }
    int state = m->state;
    switch (state) {
    case 0:
    case 1:
    case 6:
    case 0x12:
    case 0x13:
    case 0x19: Sm_CheckSlots(m); return;
    case 2: Sm_Load(m); return;
    case 3: Sm_Save(m); return;
    case 4: Sm_Delete(m); return;
    case 7:
    case 8:
    case 9:
        if (m->needTable) {
            Sm_ReadTable(m, 1, state == 8);
            m = smManager;
        }
        Sm_CheckSlots(m);
        return;
    case 0x10:
    case 0x11: Sm_RefreshTable(m); return;
    case 0x14:
        if (Sm_GetTime() > smManager->deadline) {
            smManager->state = 0x16;
        }
        return;
    case 0x15:
        if (Sm_GetTime() > smManager->deadline) {
            smLoadedA = 1;
            smLoadedB = 1;
            smManager->loaded = 1;
            smManager->state = 6;
        }
        return;
    case 0x18:
        m->state = 6;
        Sm_CheckSlots(smManager);
        return;
    default: return;
    }
}

// 0x737AB0: the start-up load prompt's loop (until it is done), then the table's creation
void __cdecl RunLoadPrompt() {
    smManager->loaded = 0;
    Sm_LoadTextures();
    while (!Sm_PadsReady()) {
        Sm_PadsUpdate();
    }
    uint8_t done;
    do {
        Sm_PadsUpdate();
        BeginFullscreenFrame();
        if (smFontReset) {
            Sm_FontReset();
        }
        done = Sm_LoadPrompt(smManager);
        Sm_EndFrame(1);
    } while (!done);
    if (!Sm_Exists(smTableName)) {
        BeginFullscreenFrame();
        Sm_DrawBackdrop();
        DrawWritingNotice();
        Sm_EndFrame(1);
        uint32_t deadline = Sm_GetTime() + 3000;
        CheckAndWriteTable(smManager);
        WaitUntil(deadline);
    }
    Sm_UnloadTextures();
    smManager->state = 6;
}

} // namespace savemgr

// (bl and esi carry arguments)
RE_UNHOOKED("SaveManager::DrawConfirm", 0x00736930);

#define SAVEMGR_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

SAVEMGR_REVERSED("SaveManager::Init", 0x007351C0, &savemgr::Init);
SAVEMGR_REVERSED("SaveManager::GetState", 0x00735370, &savemgr::GetState);
SAVEMGR_REVERSED("SaveManager::SetError", 0x00735390, &savemgr::SetError);
SAVEMGR_REVERSED("SaveManager::GetSpaceNeeded", 0x007353D0, &savemgr::GetSpaceNeeded);
SAVEMGR_REVERSED("SaveManager::Request", 0x007353E0, &savemgr::Request);
SAVEMGR_REVERSED("SaveManager::BeginSaveMode", 0x00735460, &savemgr::BeginSaveMode);
SAVEMGR_REVERSED("SaveManager::AnyUsed", 0x00735490, &savemgr::AnyUsed);
SAVEMGR_REVERSED("SaveManager::SlotInfo", 0x007354B0, &savemgr::SlotInfo);
SAVEMGR_REVERSED("SaveManager::GetMessage", 0x007354E0, &savemgr::GetMessage);
SAVEMGR_REVERSED("SaveManager::DrawMessage", 0x00735660, &savemgr::DrawMessage);
SAVEMGR_REVERSED("SaveManager::Open", 0x00735750, &savemgr::Open);
SAVEMGR_REVERSED("SaveManager::Exists", 0x007357A0, &savemgr::Exists);
SAVEMGR_REVERSED("SaveManager::WriteTable", 0x00735810, &savemgr::WriteTable);
SAVEMGR_REVERSED("SaveManager::ReadSlot", 0x007358F0, &savemgr::ReadSlot);
SAVEMGR_REVERSED("SaveManager::Delete", 0x00735A20, &savemgr::Delete);
SAVEMGR_REVERSED("SaveManager::CheckSlots", 0x00735BA0, &savemgr::CheckSlots);
SAVEMGR_REVERSED("SaveManager::AddPhoto", 0x00735C60, &savemgr::AddPhoto);
SAVEMGR_REVERSED("SaveManager::GetPhoto", 0x00735EB0, &savemgr::GetPhoto);
SAVEMGR_REVERSED("SaveManager::DeletePhoto", 0x00736000, &savemgr::DeletePhoto);
SAVEMGR_REVERSED("SaveManager::DrawRetry", 0x007362B0, &savemgr::DrawRetry);
SAVEMGR_REVERSED("SaveManager::TableExists", 0x00736480, &savemgr::TableExists);
SAVEMGR_REVERSED("SaveManager::FormatTable", 0x007364A0, &savemgr::FormatTable);
SAVEMGR_REVERSED("SaveManager::UpdateSpace", 0x007364F0, &savemgr::UpdateSpace);
SAVEMGR_REVERSED("SaveManager::RefreshTable", 0x007365C0, &savemgr::RefreshTable);
SAVEMGR_REVERSED("SaveManager::Save", 0x00736690, &savemgr::Save);
SAVEMGR_REVERSED("SaveManager::RetryAfterError", 0x007368C0, &savemgr::RetryAfterError);
SAVEMGR_REVERSED("SaveManager::CreateGallery", 0x00736A00, &savemgr::CreateGallery);
SAVEMGR_REVERSED("SaveManager::ReadTable", 0x00736C60, &savemgr::ReadTable);
SAVEMGR_REVERSED("SaveManager::RecoverFromError", 0x00736E70, &savemgr::RecoverFromError);
SAVEMGR_REVERSED("SaveManager::GetPhotoCount", 0x00736FF0, &savemgr::GetPhotoCount);
SAVEMGR_REVERSED("SaveManager::Load", 0x00737200, &savemgr::Load);
SAVEMGR_REVERSED("SaveManager::NewestSlot", 0x007373D0, &savemgr::NewestSlot);
SAVEMGR_REVERSED("SaveManager::LoadPrompt", 0x00737520, &savemgr::LoadPrompt);
SAVEMGR_REVERSED("SaveManager::Dispatch", 0x007379A0, &savemgr::Dispatch);
SAVEMGR_REVERSED("SaveManager::RunLoadPrompt", 0x00737AB0, &savemgr::RunLoadPrompt);
