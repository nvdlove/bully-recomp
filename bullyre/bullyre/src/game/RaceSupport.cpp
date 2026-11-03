// CRace's small helpers (0x70B5C0..0x70B9C0: the racers' progress ordering, the next /
// previous path node, the node and highlight lists, the transition screen's teardown, the
// racer's default values) and its vf2, the arcade race's transition screens (0x70BA10:
// the end / loading / licence / game over / instructions / continue texts on a black
// screen with the transition sprite). The rest of CRace is in MinigameRace.cpp. The x87
// runs at 24-bit precision here (the main thread): float throughout.

#include <cstdint>

#include "game/SchoolClasses.h"
#include "reversed.h"

namespace {

using school::At;

// the font's state the transition screen sets directly
GAME_ARRAY(uint8_t, rsFontShadowColour, 0x0208B780);
GAME_VAR(float, rsFontWrap, 0x0208B778);
GAME_VAR(int16_t, rsFontShadow, 0x0208B784);
GAME_VAR(int16_t, rsFontEdge, 0x0208B786);
GAME_VAR(int16_t, rsFontStyle, 0x0208B788);
GAME_VAR(int32_t, rsFontStyle4Texture, 0x0208B228);   // font style 4's texture
GAME_VAR(int32_t, rsLanguage, 0x00C18724);
GAME_VAR(float, rsScreenScaleX, 0x00A957A8);
GAME_VAR(float, rsBA4E10, 0x00BA4E10);
// the transition texts' scales (the large title, the second title, the small texts...),
// smaller in language 7
GAME_VAR(float, rsScaleTitle, 0x00BC7050);
GAME_VAR(float, rsScaleSubtitle, 0x00BC704C);
GAME_VAR(float, rsScaleBody, 0x00BC7048);
GAME_VAR(float, rsScaleSmall, 0x00BC7044);
GAME_VAR(float, rsScaleTiny, 0x00BC7040);
GAME_VAR(uint8_t, rsTransitionShown, 0x020C8824);
GAME_VAR(uint8_t*, rsCurrentMinigame, 0x020C8704);
GAME_VAR(void*, rsPathStore, 0x00D13CB4);
GAME_VAR(void*, rsRadar, 0x00C674CC);
inline void* const rsTheText = reinterpret_cast<void*>(0x020C2FA4);

GAME_FN(void, __cdecl, RS_FontStyle_Load, 0x00687BC0, int style);
GAME_FN(void, __thiscall, RS_Radar_RemoveBlip, 0x0054A940, void* radar, int32_t blip);
GAME_FN(void, __thiscall, RS_Proxy_Kill, 0x00668BA0, void* proxy);
GAME_FN(uint8_t*, __thiscall, RS_PathStore_Get, 0x005ED3A0, void* store, int32_t id);
GAME_FN(uint32_t, __thiscall, RS_Path_GetCount, 0x005EC670, uint8_t* path);
GAME_FN(const int32_t*, __thiscall, RS_Path_GetPoint, 0x005EC8C0, uint8_t* path, int32_t node);
GAME_FN(void, __thiscall, RS_Sprite_Delete, 0x0052C6D0, void* sprite);
GAME_FN(void, __thiscall, RS_Sprite_Draw, 0x0052C770, void* sprite, const float* rect, const uint8_t* colour,
        int flags);
GAME_FN(void, __cdecl, RS_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(void*, __cdecl, RS_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, RS_Free, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, RS_VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(uint8_t*, __thiscall, RS_RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, RS_DrawRect, 0x0052C470, const float* rect, const uint8_t* colour);
GAME_FN(void*, __cdecl, RS_Texture_GetRaster, 0x0051A120, void* texture);
GAME_FN(void, __cdecl, RS_SetTextColour, 0x00424480, const uint8_t* rgba);
GAME_FN(void, __cdecl, RS_Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, RS_Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, RS_Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(void, __cdecl, RS_Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, RS_Font_Render, 0x0068CDF0);
GAME_FN(const uint16_t*, __thiscall, RS_Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __thiscall, RS_Popup_Draw, 0x00706000, void* popup);

// 0x70B5C0: the race order of two progress records {node, path, link}: on the same path
// by node; on different shortcut links by the links' rejoin nodes (a record off a link
// by its own node)
uint8_t __fastcall ProgressLess(const int32_t* self, re::Edx, const int32_t* other) {
    if (self[1] != other[1]) {
        return self[1] < other[1];
    }
    int32_t a = self[2];
    int32_t b = other[2];
    if (a == b) {
        return self[0] < other[0];
    }
    a = a != 0 ? reinterpret_cast<const int32_t*>(a)[2] : self[0];
    if (b != 0) {
        return a < reinterpret_cast<const int32_t*>(b)[2];
    }
    return a < other[0];
}

// 0x70B620: the {-1, -1} pair's constructor (0x70B870's element constructor; Ghidra
// lacked it)
int32_t* __fastcall PairConstruct(int32_t* self, re::Edx) {
    self[0] = -1;
    self[1] = -1;
    return self;
}

// 0x70B630: the arcade race: flag it and load font style 4
void __fastcall RaceSetArcade(uint8_t* mg, re::Edx) {
    mg[0x50C] = 1;
    RS_FontStyle_Load(4);
}

// 0x70B640: remove the race's blip and kill its two effects
void __fastcall RaceCleanUp(uint8_t* mg, re::Edx) {
    const int32_t blip = At<int32_t>(mg, 0x140);
    if (blip != -1) {
        RS_Radar_RemoveBlip(rsRadar, blip);
    }
    RS_Proxy_Kill(mg + 0x144);
    RS_Proxy_Kill(mg + 0x14C);
}

// the path a racer is on: its shortcut link's, else the race's
inline int32_t RacerPathId(uint8_t* racer) {
    const int32_t* link = At<int32_t*>(racer, 0xC);
    return link != nullptr ? link[0] : At<int32_t>(At<uint8_t*>(racer, 0x10), 0x118);
}
inline int32_t* CopyPoint(int32_t* out, const int32_t* point) {
    out[0] = point[0];
    out[1] = point[1];
    out[2] = point[2];
    return out;
}

// 0x70B680: the position of the racer's next node (past a shortcut's end, the node it
// rejoins the race's path at; past the path's end, its first)
int32_t* __fastcall RacerNextNodePos(uint8_t* racer, re::Edx, int32_t* out) {
    uint8_t* path = RS_PathStore_Get(rsPathStore, RacerPathId(racer));
    uint32_t node = uint32_t(At<int32_t>(racer, 4) + 1);
    if (node >= RS_Path_GetCount(path)) {
        const int32_t* link = At<int32_t*>(racer, 0xC);
        if (link != nullptr) {
            uint8_t* main = RS_PathStore_Get(rsPathStore, At<int32_t>(At<uint8_t*>(racer, 0x10), 0x118));
            return CopyPoint(out, RS_Path_GetPoint(main, link[2]));
        }
        node = 0;
    }
    return CopyPoint(out, RS_Path_GetPoint(path, int32_t(node)));
}

// 0x70B710: the position of the racer's previous node (before a shortcut's start, the
// node it left the race's path at; before the path's start, its last)
int32_t* __fastcall RacerPrevNodePos(uint8_t* racer, re::Edx, int32_t* out) {
    uint8_t* path = RS_PathStore_Get(rsPathStore, RacerPathId(racer));
    int32_t node = At<int32_t>(racer, 4) - 1;
    if (node < 0) {
        const int32_t* link = At<int32_t*>(racer, 0xC);
        if (link != nullptr) {
            uint8_t* main = RS_PathStore_Get(rsPathStore, At<int32_t>(At<uint8_t*>(racer, 0x10), 0x118));
            return CopyPoint(out, RS_Path_GetPoint(main, link[1]));
        }
        node = int32_t(RS_Path_GetCount(path) - 1);
    }
    return CopyPoint(out, RS_Path_GetPoint(path, node));
}

// 0x70B7A0: append a 6-dword record (two vectors) to the race's list at +0x158
void __fastcall RaceAddPair(uint8_t* mg, re::Edx, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e,
                            uint32_t f) {
    uint32_t* entry = &At<uint32_t>(mg, 0x158 + At<int32_t>(mg, 0x3B0) * 0x18);
    entry[0] = a;
    entry[1] = b;
    entry[2] = c;
    entry = &At<uint32_t>(mg, 0x164 + At<int32_t>(mg, 0x3B0) * 0x18);
    entry[0] = d;
    entry[1] = e;
    entry[2] = f;
    ++At<int32_t>(mg, 0x3B0);
}

// 0x70B800: append a 3-dword record (a vector) to the race's list at +0x3BC
void __fastcall RaceAddPoint(uint8_t* mg, re::Edx, uint32_t a, uint32_t b, uint32_t c) {
    uint32_t* entry = &At<uint32_t>(mg, 0x3BC + At<int32_t>(mg, 0x4FC) * 0xC);
    entry[0] = a;
    entry[1] = b;
    entry[2] = c;
    ++At<int32_t>(mg, 0x4FC);
}

// 0x70B830: drop the transition screen: its sprite, its texture dictionary
void __fastcall RaceDropTransition(uint8_t* mg, re::Edx) {
    RS_Sprite_Delete(mg + 0x504);
    if (At<int32_t>(mg, 0x508) != -1) {
        RS_RemoveTxdSlot(At<int32_t>(mg, 0x508));
    }
    At<int32_t>(mg, 0x508) = -1;
    rsTransitionShown = 0;
    At<int32_t>(mg, 0x500) = -1;
}

// 0x70B870: an array {data, capacity, count} of {-1, -1} pairs: reallocate to `capacity`
// (copying the old count), the count set to it when asked
struct PairArray {
    re::Ptr32<uint32_t> data;
    uint32_t capacity;
    uint32_t count;
};
void __fastcall PairArrayReserve(PairArray* self, re::Edx, uint32_t capacity, int setCount) {
    uint32_t* fresh = nullptr;
    self->capacity = capacity;
    if (capacity != 0) {
        const uint64_t bytes = uint64_t(capacity) * 8;
        fresh = static_cast<uint32_t*>(RS_Malloc(bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : uint32_t(bytes)));
        if (fresh != nullptr) {
            RS_VectorConstruct(fresh, 8, capacity, reinterpret_cast<void*>(0x0070B620));
        }
    }
    if (self->data != nullptr) {
        for (uint32_t k = 0; k < self->count; ++k) {
            fresh[k * 2] = self->data[k * 2];
            fresh[k * 2 + 1] = self->data[k * 2 + 1];
        }
        RS_Free(self->data);
    }
    self->data = fresh;
    if (re::Flag(setCount)) {
        self->count = self->capacity;
    }
}

// 0x70B900: a racer's default values
uint8_t* __fastcall RacerDefaults(uint8_t* racer, re::Edx) {
    for (int off = 0; off <= 0x18; off += 4) {
        At<int32_t>(racer, off) = 0;
    }
    At<float>(racer, 0x1C) = -1.0f;
    racer[0x20] = 0;
    At<float>(racer, 0x24) = 1.3f;
    At<float>(racer, 0x28) = 1.0f;
    At<float>(racer, 0x2C) = 10.0f;
    At<float>(racer, 0x34) = 10.0f;
    At<float>(racer, 0x30) = 1.3f;
    At<float>(racer, 0x38) = 0.7f;
    for (int off = 0x3C; off <= 0x4C; off += 4) {
        At<int32_t>(racer, off) = 0;
    }
    At<float>(racer, 0x54) = 0.5f;
    for (int off = 0x58; off <= 0x68; off += 4) {
        At<int32_t>(racer, off) = 0;
    }
    return racer;
}

// 0x70B980: is `node` in the race's bottleneck list
uint8_t __fastcall RaceHasBottleneck(uint8_t* mg, re::Edx, int32_t node) {
    const uint32_t count = At<uint32_t>(mg, 0x114);
    const int32_t* nodes = At<int32_t*>(mg, 0x10C);
    for (uint32_t k = 0; k < count; ++k) {
        if (nodes[k] == node) {
            return 1;
        }
    }
    return 0;
}

// 0x70B9C0: a progress record's position: node [0] of its link's path, else of the
// current race's
int32_t* __fastcall ProgressPos(const int32_t* self, re::Edx, int32_t* out) {
    const int32_t* link = reinterpret_cast<const int32_t*>(self[2]);
    const int32_t id = link != nullptr ? link[0] : At<int32_t>(rsCurrentMinigame, 0x118);
    return CopyPoint(out, RS_Path_GetPoint(RS_PathStore_Get(rsPathStore, id), self[0]));
}

// the transition screen's texts
inline const uint16_t* RaceText(const char* key) {
    return RS_Text_Get(rsTheText, key);
}
inline void PrintCentred(const char* key, float y) {
    RS_Font_PrintString(320.0f, y, RaceText(key), 1);
}
// a title outlined in `outline` (printed at the 8 offsets of 3x its scale round (320, y))
// and filled in `fill`; its scale is read from `scale` (the game reads it twice)
void OutlinedTitle(uint16_t* buf, const char* key, const float* scale, float y, const uint8_t* outline,
                   const uint8_t* fill) {
    RS_Font_SetScale(*scale);
    RS_Font_Format(buf, RaceText(key), 0x80);
    const float s = *scale * 3.0f;
    RS_SetTextColour(outline);
    const float up = y - s;
    const float left = 320.0f - s;
    RS_Font_PrintString(left, up, buf, 1);
    RS_Font_PrintString(left, y, buf, 1);
    RS_Font_PrintString(320.0f, up, buf, 1);
    const float down = s + y;
    RS_Font_PrintString(left, down, buf, 1);
    const float right = s + 320.0f;
    RS_Font_PrintString(right, up, buf, 1);
    RS_Font_PrintString(right, y, buf, 1);
    RS_Font_PrintString(320.0f, down, buf, 1);
    RS_Font_PrintString(right, down, buf, 1);
    RS_SetTextColour(fill);
    RS_Font_PrintString(320.0f, y, buf, 1);
}

// 0x70BA10 (CRace::vf2): the arcade race's transition screen (its popup instead while
// that's up): the transition sprite over black, then screen mg+0x500's texts
void __fastcall RaceDrawTransition(uint8_t* mg, re::Edx) {
    if (mg[0xA0] != 0) {
        RS_Popup_Draw(mg + 0x50);
        return;
    }
    if (rsTransitionShown == 0) {
        return;
    }
    const float sx = rsScreenScaleX;
    const float width = sx * 640.0f;
    float rect[4];
    const float half = width * 0.5f;
    rect[0] = 320.0f - half;
    rect[3] = 0.0f;
    rect[2] = 320.0f + half;
    rect[1] = 480.0f;
    uint8_t white[4];
    RS_RGBA_Set(white, 0xFF, 0xFF, 0xFF, 0xFF);
    float screen[4];
    uint8_t black[4];
    screen[0] = 0.0f;
    screen[3] = 0.0f;
    screen[2] = 640.0f;
    screen[1] = 480.0f;
    RS_DrawRect(screen, RS_RGBA_Set(black, 0, 0, 0, 0xFF));
    RS_Texture_GetRaster(At<void*>(mg, 0x504));
    RS_Sprite_Draw(mg + 0x504, rect, white, 0x1800);
    uint8_t yellow[4];
    uint8_t light[4];
    uint8_t blue[4];
    uint8_t dark[4];
    RS_RGBA_Set(yellow, 0xEB, 0xEB, 0x10, 0xFF);
    RS_RGBA_Set(light, 0xEB, 0xEB, 0xEB, 0xFF);
    RS_RGBA_Set(blue, 0x64, 0x64, 0xEB, 0xFF);   // (unused)
    RS_RGBA_Set(dark, 0x10, 0x10, 0x10, 0xFF);
    RS_Font_Reset();
    if (rsFontStyle4Texture != 0) {
        rsFontStyle = 4;
        rsFontShadow = 0;
    }
    rsFontEdge = 3;
    RS_Font_SetScale(1.0f);
    rsFontWrap = sx * 520.0f;
    rsFontShadowColour[0] = dark[0];
    rsFontShadowColour[1] = dark[1];
    rsFontShadowColour[2] = dark[2];
    rsFontShadowColour[3] = dark[3];
    rsFontShadow = 2;
    const bool lang7 = rsLanguage == 7;
    if (lang7) {
        rsScaleTitle = 1.3f;
        rsScaleSubtitle = 0.9f;
        rsScaleBody = 0.45f;
        rsScaleSmall = 0.4f;
        rsScaleTiny = 0.3f;
    }
    uint16_t buf[0x80];
    switch (At<uint32_t>(mg, 0x500)) {
    case 0:   // the race is over
        rsFontEdge = 3;
        rsFontShadow = 0;
        OutlinedTitle(buf, "RARC_END0", lang7 ? &rsScaleSubtitle : &rsScaleTitle, 50.0f, dark, yellow);
        OutlinedTitle(buf, "RARC_END1", lang7 ? &rsScaleBody : &rsScaleSubtitle, 200.0f, dark, light);
        break;
    case 1:
        rsFontEdge = 3;
        RS_SetTextColour(light);
        RS_Font_SetScale(rsScaleTiny);
        PrintCentred("RARC_NOTE", 400.0f);
        break;
    case 2:
    case 3:
    case 4:   // loading
        rsFontShadow = 2;
        rsFontEdge = 3;
        RS_SetTextColour(yellow);
        RS_Font_SetScale(rsScaleSmall);
        PrintCentred("RARC_LOADING", 380.0f);
        break;
    case 5: {   // the licence, the start / exit instructions
        rsFontEdge = 3;
        RS_SetTextColour(light);
        RS_Font_SetScale(rsScaleTiny);
        PrintCentred("RARC_LIC", 415.0f);
        RS_Font_SetScale(rsScaleBody);
        const float x = sx * -100.0f + 320.0f;
        rsFontEdge = 0;
        RS_Font_PrintString(x, 325.0f, RaceText("RARC_SINST"), 1);
        RS_Font_PrintString(x, 350.0f, RaceText("RARC_STOSTART"), 1);
        RS_Font_PrintString(x, 375.0f, RaceText("RARC_STOEXIT"), 1);
        if (rsLanguage == 7 || rsLanguage == 6) {
            const float saved = rsBA4E10;
            rsBA4E10 = 1.2f;
            RS_Font_Render();
            rsBA4E10 = saved;
            RS_Font_Render();
            return;
        }
        break;
    }
    case 6:   // game over
        rsFontEdge = 3;
        rsFontShadow = 0;
        OutlinedTitle(buf, "RARC_OVER", lang7 ? &rsScaleBody : &rsScaleTitle, 125.0f, dark, light);
        rsFontShadow = 2;
        RS_SetTextColour(light);
        RS_Font_SetScale(rsScaleTiny);
        PrintCentred("RARC_COPR", 390.0f);
        break;
    case 7: {   // the objectives and the controls
        rsFontEdge = 3;
        RS_SetTextColour(yellow);
        RS_Font_SetScale(rsScaleBody);
        PrintCentred("RARC_OBJ0", 60.0f);
        RS_SetTextColour(light);
        PrintCentred("RARC_OBJ1", 100.0f);
        RS_SetTextColour(yellow);
        PrintCentred("RARC_CTRL0", 180.0f);
        RS_Font_Render();
        RS_SetTextColour(light);
        RS_Font_SetScale(rsScaleSmall);
        RS_Font_PrintString(sx * -150.0f + 320.0f, 380.0f, RaceText("RARC_ToCONT"), 1);
        RS_Font_Render();
        RS_Font_PrintString(sx * 150.0f + 320.0f, 380.0f, RaceText("RARC_ToEXIT"), 1);
        RS_Font_Render();
        RS_Font_SetScale(rsScaleBody);
        rsFontEdge = 0;
        const float x = sx * -250.0f + 320.0f;
        RS_Font_PrintString(x, 220.0f, RaceText("RARC_CTRL1"), 1);
        RS_Font_Render();
        RS_Font_PrintString(x, 250.0f, RaceText("RARC_CTRL2"), 1);
        RS_Font_Render();
        RS_Font_PrintString(x, 280.0f, RaceText("RARC_CTRL3"), 1);
        RS_Font_Render();
        RS_Font_PrintString(x, 310.0f, RaceText("RARC_CTRL4"), 1);
        break;
    }
    case 8: {   // lost: pay to continue?
        rsFontEdge = 3;
        rsFontShadow = 0;
        OutlinedTitle(buf, "RARC_LOSE", lang7 ? &rsScaleBody : &rsScaleTitle, 125.0f, dark, light);
        OutlinedTitle(buf, "RARC_PAY", lang7 ? &rsScaleBody : &rsScaleSubtitle, 205.0f, dark, yellow);
        rsFontShadow = 2;
        RS_SetTextColour(light);
        RS_Font_SetScale(rsScaleBody);
        const float x = sx * -40.0f + 320.0f;
        rsFontEdge = 0;
        RS_Font_PrintString(x, 315.0f, RaceText("RARC_NO"), 1);
        RS_Font_PrintString(x, 345.0f, RaceText("RARC_YES"), 1);
        break;
    }
    default:
        break;
    }
    RS_Font_Render();
}

} // namespace

#define RACESUP44_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

RACESUP44_REVERSED("RaceProgress::Less", 0x0070B5C0, &ProgressLess);
RACESUP44_REVERSED("RacePair::RacePair", 0x0070B620, &PairConstruct);
RACESUP44_REVERSED("CRace::SetArcade", 0x0070B630, &RaceSetArcade);
RACESUP44_REVERSED("CRace::CleanUp", 0x0070B640, &RaceCleanUp);
RACESUP44_REVERSED("RaceRacer::GetNextNodePos", 0x0070B680, &RacerNextNodePos);
RACESUP44_REVERSED("RaceRacer::GetPrevNodePos", 0x0070B710, &RacerPrevNodePos);
RACESUP44_REVERSED("CRace::AddNodePair", 0x0070B7A0, &RaceAddPair);
RACESUP44_REVERSED("CRace::AddNodePoint", 0x0070B800, &RaceAddPoint);
RACESUP44_REVERSED("CRace::DestroyTransition", 0x0070B830, &RaceDropTransition);
RACESUP44_REVERSED("RacePairArray::Reserve", 0x0070B870, &PairArrayReserve);
RACESUP44_REVERSED("RaceRacer::RaceRacer", 0x0070B900, &RacerDefaults);
RACESUP44_REVERSED("CRace::IsBottleneck", 0x0070B980, &RaceHasBottleneck);
RACESUP44_REVERSED("RaceProgress::GetPos", 0x0070B9C0, &ProgressPos);
RACESUP44_REVERSED("CRace::DrawTransition", 0x0070BA10, &RaceDrawTransition);
