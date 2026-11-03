// The font's setup (0x688780..0x68A330; the rest of CFont in Font.cpp, its layout and
// drawing in FontLayout.cpp): the HUD's font setups (config\hdt\font*.HDT: 8 fixed at
// 0x208B800 and up to 5 "Custom" ones), the button glyphs (a texture per controller
// button, their sizes into every style's glyph table), the text formatter that expands
// "~token~"s, the start-up (the font textures, config\fonts.bin's token names and glyph
// tables, the default details) and the per-frame reset.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

GAME_VAR(uint8_t, fsLoaded, 0x0208B264);
GAME_VAR(int32_t, fsLanguage, 0x00C18724);
GAME_VAR(int32_t, fsWidescreen, 0x00C3CD0C);
GAME_VAR(float, fsScreenScaleX, 0x00A957A8);
GAME_VAR(float, fsScreenScaleY, 0x00A957AC);
GAME_VAR(float, fsScaleX, 0x0208B76C);
GAME_VAR(float, fsScaleY, 0x0208B770);
GAME_VAR(float, fsWrapLeft, 0x0208B774);
GAME_VAR(float, fsWrapCentre, 0x0208B778);
GAME_VAR(float, fsWrapRight, 0x0208B77C);
GAME_VAR(int16_t, fsShadow, 0x0208B784);
GAME_VAR(int16_t, fsJustify, 0x0208B786);
GAME_VAR(uint8_t, fsFlags, 0x0208B78A);
GAME_VAR(int16_t, fsLastGlyph, 0x0208B7F8);
GAME_VAR(uint8_t*, fsRenderCursor, 0x01B8A184);
GAME_ARRAY(uint8_t, fsColour, 0x0208B764);
GAME_ARRAY(uint8_t, fsColour2, 0x0208B768);
GAME_ARRAY(uint8_t, fsShadowColour, 0x0208B780);
GAME_ARRAY(float, fsHeights, 0x0208B240);
GAME_ARRAY(void*, fsTextures, 0x0208B218);
GAME_ARRAY(float, fsInvSize, 0x0208B790);
GAME_ARRAY(float, fsLineHeight, 0x00BA4E8C);
inline uint8_t* const fsDefaults = reinterpret_cast<uint8_t*>(0x0208B9C0);   // the details' defaults (11 words)
inline uint8_t* const fsDetails = reinterpret_cast<uint8_t*>(0x0208B764);
inline uint8_t* const fsSetups = reinterpret_cast<uint8_t*>(0x0208B800);     // [8] CFontSetup (0x38)
GAME_ARRAY(uint8_t*, fsCustomSetups, 0x01B89230);   // [5]
GAME_VAR(int32_t, fsCustomCount, 0x0208B280);
GAME_VAR(int32_t, fsButtonGlyphCount, 0x0208B284);
GAME_ARRAY(int32_t, fsButtonGlyphs, 0x01B89258);
GAME_ARRAY(void*, fsButtons, 0x01B89244);   // [5]
GAME_ARRAY(void*, fsHudTextures, 0x00C66DD0);
GAME_VAR(int32_t, fsTokenCount, 0x0208A524);
inline uint8_t* const fsTokens = reinterpret_cast<uint8_t*>(0x0208A528);
inline uint8_t* const fsGlyphs = reinterpret_cast<uint8_t*>(0x01B8A9C4);
constexpr int kFsGlyphStride = 0x7FF8A;
GAME_ARRAY(float, fsNumbers, 0x01B89560);
GAME_VAR(int32_t, fsNumberCount, 0x01B89578);
GAME_ARRAY(const uint16_t*, fsStrings, 0x01B89550);
GAME_VAR(int32_t, fsStringCount, 0x01B8957C);
GAME_VAR(uint8_t, fsBoldNext, 0x01B8A9B9);
inline uint16_t* const fsHashTextBuffer = reinterpret_cast<uint16_t*>(0x01B89980);
inline uint16_t* const fsHashStringBuffer = reinterpret_cast<uint16_t*>(0x01B89488);
inline void* const fsText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const fsButtonMap = reinterpret_cast<void*>(0x00A14810);
constexpr uintptr_t kFontSetupVtable = 0x0093EDF0;

GAME_FN(void*, __cdecl, DataReader_Open, 0x005B07E0, const char* path, int32_t* out, int);
GAME_FN(uint8_t, __thiscall, DataReader_Next, 0x005B0720, void* reader, void* archivable, const char* key);
GAME_FN(void, __cdecl, DataReader_Close, 0x005B0860, void* reader);
GAME_FN(const char*, __cdecl, FontSetup_Name, 0x005DAE80, int index);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(int32_t, __cdecl, Pad_GetActive, 0x00738070);
GAME_FN(int32_t, __cdecl, Pad_Kind, 0x00738500, int32_t pad);
GAME_FN(uint8_t, __cdecl, Pad_IsUsed, 0x00738520, int32_t pad, int);
GAME_FN(int32_t, __cdecl, Pad_BindingKind, 0x007384D0, int32_t pad, int32_t key, int32_t* a, int32_t* button);
GAME_FN(void*, __cdecl, Hud_GetButtonTexture, 0x005145B0, int action, uint8_t* isKey, int32_t kind, int player);
GAME_FN(const int32_t*, __thiscall, ButtonMap_GetKeys, 0x00454CD0, void* map, int player);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
template <typename... A>
inline int __cdecl SprintfS(char* buffer, size_t size, const char* format, A... rest) { return crt::SprintfS(buffer, size, format, rest...); }
inline void* __cdecl Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, AsciiToText, 0x00690090, const char* ascii, uint16_t* text);
GAME_FN(void, __cdecl, HashToText, 0x006900C0, const void* hashed, uint16_t* text);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, Txd_LoadTxdFile, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, FileMgr_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, FileMgr_Read, 0x0042D2A0, int handle, void* to, int size);
GAME_FN(void, __cdecl, FileMgr_CloseFile, 0x0042D450, int handle);
// ours, through the game's addresses
GAME_FN(void, __thiscall, FontSetup_CopyFrom, 0x00688780, uint8_t* self, uint8_t* other);
GAME_FN(void, __cdecl, FontSetups_Load, 0x00688810);
GAME_FN(void*, __cdecl, Font_GetButtonTexture, 0x00688980, int glyph, re::Ptr32<const uint16_t>* text);
GAME_FN(void, __thiscall, FontBox_AdjustForScreen, 0x00687B10, uint8_t* self, int widescreen);
GAME_FN(void, __cdecl, FontLayout_GetBox, 0x00688370, uint8_t* layout, float* rect, float x, float y);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(int, __cdecl, Font_IsNone, 0x00687A30, const uint16_t* text);
GAME_FN(void, __cdecl, Font_FormatClock, 0x00688460, char* out);
GAME_FN(void, __cdecl, Font_FormatMoney, 0x006884C0, char* out, int32_t cents);
GAME_FN(void, __cdecl, FontStyle_Load, 0x00687BC0, int style);

inline float& F(uint8_t* p, int offset) {
    return At<float>(p, offset);
}
inline void CopyRgba(uint8_t* to, const uint8_t* from) {
    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2];
    to[3] = from[3];
}
inline void SetRgba(uint8_t* to, int r, int g, int b, int a) {
    uint8_t colour[4];
    CopyRgba(to, RGBA_Construct(colour, r, g, b, a));
}
// the details from their defaults, the scale from style 3's height
inline void DetailsToDefaults() {
    float height = fsHeights[3];
    float x = height * fsScreenScaleX;
    for (int i = 0; i < 11; i++) {
        At<uint32_t>(fsDetails, i * 4) = At<uint32_t>(fsDefaults, i * 4);
    }
    fsScaleX = x;
    fsScaleY = height * fsScreenScaleY;
}

// 0x688780 (thiscall, ret 4): CFontSetup copied (its fields, not the vtable or index)
void __fastcall FontSetup_CopyFromImpl(uint8_t* self, re::Edx, uint8_t* other) {
    At<int32_t>(self, 4) = At<int32_t>(other, 4);
    At<int32_t>(self, 8) = At<int32_t>(other, 8);
    At<float>(self, 0xC) = At<float>(other, 0xC);
    At<float>(self, 0x10) = At<float>(other, 0x10);
    At<int32_t>(self, 0x14) = At<int32_t>(other, 0x14);
    for (int i = 0x18; i < 0x20; i++) {
        self[i] = other[i];
    }
    At<float>(self, 0x20) = At<float>(other, 0x20);
    At<int32_t>(self, 0x24) = At<int32_t>(other, 0x24);
    At<float>(self, 0x28) = At<float>(other, 0x28);
    At<float>(self, 0x2C) = At<float>(other, 0x2C);
    self[0x30] = other[0x30];
    self[0x31] = other[0x31];
    self[0x32] = other[0x32];
}

// 0x688810 (cdecl): the font setups read (the language's HDT file): the 8 fixed ones by
// name (adjusted for the screen), the custom ones (up to 5, the old ones deleted)
void __cdecl FontSetups_LoadImpl() {
    int32_t unused = 0;
    const char* path;
    if (fsLanguage == 7) {
        path = reinterpret_cast<const char*>(0x0093EE88);   // "Config\hdt\fontj.HDT"
    } else if (fsLanguage == 6) {
        path = reinterpret_cast<const char*>(0x0093EE70);   // "Config\hdt\fontr.HDT"
    } else {
        path = reinterpret_cast<const char*>(0x0093EE5C);   // "Config\hdt\font.HDT"
    }
    void* reader = DataReader_Open(path, &unused, 0);
    if (!reader) {
        return;
    }
    int32_t index = 0;
    for (uint8_t* setup = fsSetups; setup < fsSetups + 8 * 0x38; setup += 0x38, index++) {
        At<int32_t>(setup, 0x34) = index;
        DataReader_Next(reader, setup, FontSetup_Name(index));
        FontBox_AdjustForScreen(setup, fsLanguage);
    }
    alignas(4) uint8_t custom[0x38];
    At<uint32_t>(custom, 0) = uint32_t(kFontSetupVtable);
    At<int32_t>(custom, 0x34) = -1;
    for (int32_t i = 0; i < fsCustomCount; i++) {
        if (fsCustomSetups[i]) {
            vc::Call<void>(fsCustomSetups[i], 0, 1);
        }
    }
    fsCustomCount = 0;
    const char* customKey = reinterpret_cast<const char*>(0x0093EE54);   // "Custom"
    while (DataReader_Next(reader, custom, customKey)) {
        if (fsCustomCount < 5) {
            uint8_t* setup = static_cast<uint8_t*>(Malloc(0x38));
            if (setup) {
                At<uint32_t>(setup, 0) = uint32_t(kFontSetupVtable);
                At<int32_t>(setup, 0x34) = -1;
            }
            FontSetup_CopyFrom(setup, custom);
            fsCustomSetups[fsCustomCount] = setup;
            fsCustomCount++;
        }
    }
    DataReader_Close(reader);
}

// the controller action a button glyph stands for (-1: the HUD's own texture)
inline int ButtonAction(int glyph) {
    switch (glyph) {
    case 4: return 7;
    case 5: return 8;
    case 6: return 6;
    case 7: return 9;
    case 8: return 0xB;
    case 9: return 0xA;
    case 0xA: return 0xE;
    case 0xB: return 0xD;
    case 0xC: return 0xC;
    case 0xD: return 0xF;
    case 0x13: return 2;
    case 0x14: return 3;
    case 0x15: return 0;
    case 0x16: return 1;
    case 0x17: return 5;
    case 0x18: return 4;
    case 0x1A: return 0x19;
    case 0x1B: return 0x1A;
    default:
        if (glyph >= 0x1C && glyph <= 0x24) {
            return glyph - 0x1C + 0x10;
        }
        return -1;
    }
}

// 0x688980 (cdecl): a button glyph's texture for the current controller (glyphs 0x98..0x9A
// the three button textures, others not buttons the HUD's textures); without a texture
// (or for a key) *text gets the binding's name ("DIBUTTON_..." text, the button number
// as the next number)
void* __cdecl Font_GetButtonTextureImpl(int glyph, re::Ptr32<const uint16_t>* text) {
    *text = nullptr;
    int action;
    switch (glyph) {
    case 0x98:
        return fsButtons[0];
    case 0x99:
        return fsButtons[1];
    case 0x9A:
        return fsButtons[2];
    case 0x9B:
        action = Pad_IsUsed(Pad_GetActive(), 1) ? 0x11 : 7;
        break;
    case 0x9C:
        action = Pad_IsUsed(Pad_GetActive(), 1) ? 0x15 : 6;
        break;
    default:
        action = ButtonAction(glyph);
        if (action == -1) {
            return fsHudTextures[glyph];
        }
        break;
    }
    uint8_t isKey = 0;
    void* texture = Hud_GetButtonTexture(action, &isKey, Pad_Kind(Pad_GetActive()), 0);
    if (!isKey && texture) {
        return texture;
    }
    char name[64];
    if (action == 0x19) {
        SprintfS(name, 0x40, reinterpret_cast<const char*>(0x0093EF5C));   // "DIBUTTON_MOVEMENT"
    } else if (action == 0x1A) {
        SprintfS(name, 0x40, reinterpret_cast<const char*>(0x0093EF4C));   // "DIBUTTON_CAMERA"
    } else if (ButtonMap_GetKeys(fsButtonMap, 0)[action] < 0x10A) {
        SprintfS(name, 0x40, reinterpret_cast<const char*>(0x0093EEA0), ButtonMap_GetKeys(fsButtonMap, 0)[action]);   // "DIBUTTON_%d"
    } else {
        int32_t index = 0;
        int32_t button = 0;
        int32_t key = ButtonMap_GetKeys(fsButtonMap, 0)[action];
        static const uintptr_t kKinds[7] = {
            0x0093EF34,   // "DIBUTTON_JOYAXIS_NEG"
            0x0093EF1C,   // "DIBUTTON_JOYAXIS_POS"
            0x0093EED8,   // "DIBUTTON_JOYPOV_UP"
            0x0093EEC0,   // "DIBUTTON_JOYPOV_DOWN"
            0x0093EF04,   // "DIBUTTON_JOYPOV_LEFT"
            0x0093EEEC,   // "DIBUTTON_JOYPOV_RIGHT"
            0x0093EEAC,   // "DIBUTTON_JOYBUTTON"
        };
        uint32_t kind = uint32_t(Pad_BindingKind(Pad_GetActive(), key, &index, &button));
        if (kind <= 6) {
            SprintfS(name, 0x40, reinterpret_cast<const char*>(kKinds[kind]));
        }
        button = button + 1;
        Font_AddNumber(float(button));
    }
    *text = Text_Get(fsText, name);
    return texture;
}

// 0x688DD0 (thiscall, ret 4): CFont's details copied (the flags' low 4 bits merged)
uint8_t* __fastcall FontDetails_CopyFrom(uint8_t* self, re::Edx, const uint8_t* other) {
    F(self, 0) = At<float>(const_cast<uint8_t*>(other), 0);
    F(self, 4) = At<float>(const_cast<uint8_t*>(other), 4);
    F(self, 8) = At<float>(const_cast<uint8_t*>(other), 8);
    F(self, 0xC) = At<float>(const_cast<uint8_t*>(other), 0xC);
    self[0x10] = other[0x10];
    self[0x11] = other[0x11];
    self[0x12] = other[0x12];
    self[0x13] = other[0x13];
    F(self, 0x14) = At<float>(const_cast<uint8_t*>(other), 0x14);
    At<uint16_t>(self, 0x18) = At<uint16_t>(const_cast<uint8_t*>(other), 0x18);
    for (int bit = 1; bit <= 8; bit <<= 1) {
        self[0x1A] = uint8_t(self[0x1A] ^ ((self[0x1A] ^ other[0x1A]) & bit));
    }
    return self;
}

// 0x688E60 (cdecl): the details back to their defaults (CFont::InitPerFrame's start)
void __cdecl Font_BeginImpl() {
    DetailsToDefaults();
}

// 0x688EA0 (cdecl): a layout's box at its own start (null: the last printed one's)
void __cdecl FontLayout_GetBoxAt(float* rect, uint8_t* layout) {
    if (!layout) {
        layout = reinterpret_cast<uint8_t*>(0x0208B288);
    }
    FontLayout_GetBox(layout, rect, F(layout, 0), F(layout, 4));
}

// the converted text appended to out
inline uint16_t* Append(uint16_t* out, const uint16_t* text) {
    while (*text) {
        *out++ = *text++;
    }
    return out;
}

// 0x688ED0 (cdecl): a text formatted: "~z<n>~" / "~Z<n>~" a number zero / space padded to n
// (2 by default), "~tod~" the clock, "~$~" money, "~f~" a float, "~i~" an int, "~a~" a
// string (a hashed one decoded), "~n~" a new line, "~B~" bold next; other tokens copied
// as they are. The numbers and strings added before are used up. (A hashed text, 0xFFFF
// first, is decoded first.)
void __cdecl Font_FormatText(uint16_t* out, const uint16_t* text) {
    if (text && *text == 0xFFFF) {
        HashToText(text + 1, fsHashTextBuffer);
        text = fsHashTextBuffer;
    }
    if (*text != 0) {
        float* number = &fsNumbers[1];
        re::Ptr32<const uint16_t>* string = &fsStrings[0];
        char ascii[256];
        uint16_t wide[256];
        do {
            uint16_t c = *text;
            if (c != '~') {
                if (c == 0) {
                    break;
                }
                *out++ = c;
                text++;
                continue;
            }
            text++;
            c = *text;
            bool zeros = c == 'z';
            bool spaces = c == 'Z';
            bool handled = true;
            if (zeros || spaces) {
                if (fsNumberCount != 0) {
                    text++;
                    int32_t width = 0;
                    if (*text == '~') {
                        width = 2;
                    } else {
                        while (*text != 0) {
                            width = int32_t(int8_t(*text)) + width * 10 - '0';
                            text++;
                            if (*text == '~') {
                                break;
                            }
                        }
                    }
                    int32_t value = int32_t(*number);
                    number++;
                    if (zeros) {
                        Sprintf(ascii, reinterpret_cast<const char*>(0x0093EF74), width, value);   // "%0*d"
                    }
                    if (spaces) {
                        Sprintf(ascii, reinterpret_cast<const char*>(0x0093EF70), width, value);   // "%*d"
                    }
                    AsciiToText(ascii, wide);
                    out = Append(out, wide);
                }
            } else if (c == 't' && text[1] == 'o' && text[2] == 'd') {
                Font_FormatClock(ascii);
                AsciiToText(ascii, wide);
                out = Append(out, wide);
            } else if (text[1] != '~') {
                handled = false;
            } else {
                switch (c) {
                case '$':
                    if (fsNumberCount != 0) {
                        int32_t cents = int32_t(*number);
                        number++;
                        Font_FormatMoney(ascii, cents);
                        AsciiToText(ascii, wide);
                        out = Append(out, wide);
                    }
                    break;
                case 'B':
                    fsBoldNext = 1;
                    break;
                case 'a':
                    if (fsStringCount != 0) {
                        const uint16_t* s = *string++;
                        if (s) {
                            if (Font_IsNone(s)) {
                                HashToText(s + 1, fsHashStringBuffer);
                                s = fsHashStringBuffer;
                            }
                            out = Append(out, s);
                        }
                    }
                    break;
                case 'f':
                    if (fsNumberCount != 0) {
                        Sprintf(ascii, reinterpret_cast<const char*>(0x0093EF7C), double(*number));   // "%0.2f"
                        number++;
                        AsciiToText(ascii, wide);
                        out = Append(out, wide);
                    }
                    break;
                case 'i':
                    if (fsNumberCount != 0) {
                        int32_t value = int32_t(*number);
                        number++;
                        Sprintf(ascii, reinterpret_cast<const char*>(0x008FFF78), value);   // "%d"
                        AsciiToText(ascii, wide);
                        out = Append(out, wide);
                    }
                    break;
                case 'n':
                    Sprintf(ascii, reinterpret_cast<const char*>(0x00917810));   // "\n"
                    AsciiToText(ascii, wide);
                    out = Append(out, wide);
                    break;
                default:
                    handled = false;
                    break;
                }
            }
            if (!handled) {
                // (copied as it is, both "~"s)
                *out++ = '~';
                c = *text;
                while (c != '~') {
                    if (c == 0) {
                        goto done;
                    }
                    text++;
                    *out++ = c;
                    c = *text;
                }
                *out++ = c;
                text++;
                continue;
            }
            // past the token's closing "~"
            while (*text != '~') {
                if (*text == 0) {
                    goto done;
                }
                text++;
            }
            if (*text == 0) {
                break;
            }
            text++;
        } while (*text != 0);
    }
done:
    *out = 0;
    fsNumberCount = 0;
    fsStringCount = 0;
}

// 0x689400 (thiscall): a font setup made current: the details from their defaults, its
// justification and line width (the widescreen one when set) as the wrap bound, colour,
// scale, italic / outline flags, drop shadow (or outline) at its distance in its shadow
// colour
void __fastcall FontSetup_Apply(uint8_t* self, re::Edx) {
    float width = fsWidescreen != 0 ? F(self, 0x2C) : F(self, 0x28);
    int32_t wrap = int32_t(width);
    float height = fsHeights[3];
    float sx = fsScreenScaleX;
    float x = height * sx;
    for (int i = 0; i < 11; i++) {
        At<uint32_t>(fsDetails, i * 4) = At<uint32_t>(fsDefaults, i * 4);
    }
    fsScaleX = x;
    float sy = fsScreenScaleY;
    fsScaleY = height * sy;
    switch (At<int32_t>(self, 0x24)) {
    case 0:
        fsJustify = 0;
        break;
    case 1:
        fsJustify = 1;
        break;
    case 2:
        fsJustify = 3;
        break;
    case 3:
        fsJustify = 2;
        break;
    default:
        break;
    }
    switch (At<int32_t>(self, 0x24)) {
    case 0:
        fsWrapLeft = float(wrap);
        break;
    case 1:
        fsWrapRight = float(wrap);
        break;
    case 2:
        fsWrapCentre = float(wrap);
        break;
    default:
        break;
    }
    CopyRgba(fsColour, self + 0x18);
    int32_t scale = At<int32_t>(self, 0x14);
    fsScaleX = sx * fsHeights[scale];
    fsScaleY = sy * fsHeights[scale];
    uint8_t flags = fsFlags;
    flags = uint8_t(flags ^ (((self[0x30] << 4) ^ flags) & 0x10));
    fsFlags = flags;
    flags = uint8_t(flags ^ (((self[0x32] << 5) ^ flags) & 0x20));
    fsFlags = flags;
    if (!self[0x31] && !self[0x32]) {
        fsShadow = 0;
        CopyRgba(fsShadowColour, self + 0x18);
        return;
    }
    fsShadow = int16_t(int32_t(F(self, 0x20)));
    CopyRgba(fsShadowColour, self + 0x1C);
}

// the details' defaults and the 9 styles' heights; the details reset from them
inline void SetDefaults() {
    SetRgba(fsDefaults, 0xFF, 0xFF, 0xFF, 0xFF);
    SetRgba(fsDefaults + 4, 0, 0, 0, 0xFF);
    F(fsDefaults, 8) = 0.0f;
    F(fsDefaults, 0xC) = 0.0f;
    F(fsDefaults, 0x10) = 592.0f;
    F(fsDefaults, 0x14) = 544.0f;
    F(fsDefaults, 0x18) = 48.0f;
    SetRgba(fsDefaults + 0x1C, 0, 0, 0, 0xFF);
}

// 0x6895D0 (cdecl): CFont::Initialise (once): the western fonts' textures (style 0 / 1
// from TXD/FONTS.NFT), the token names and 8 styles' glyph tables from config/fonts.bin
// (other languages: empty tables), the defaults, the setups, the first two styles' line
// heights; the Japanese / Russian style (8 / 9) standing in for styles 0 and 1
void __cdecl Font_Initialise() {
    if (fsLoaded) {
        return;
    }
    if (fsLanguage != 7 && fsLanguage != 6) {
        int slot = Txd_AddTxdSlot(reinterpret_cast<const char*>(0x0093EFC0));        // "fonts"
        Txd_LoadTxdFile(slot, reinterpret_cast<const char*>(0x0093EFB0));            // "TXD/FONTS.NFT"
        Txd_AddRef(slot);
        Txd_PushCurrentTxd();
        Txd_SetCurrentTxd(slot);
        const char* names[4] = {reinterpret_cast<const char*>(0x0093EDA4), reinterpret_cast<const char*>(0x0093EFA8),
                                reinterpret_cast<const char*>(0x0093ED9C), reinterpret_cast<const char*>(0x0093EFA0)};
        char cleared[64];
        Memset(cleared, 0, 0x40);
        for (int i = 0; i < 2; i++) {
            void* texture = Txd_FindTexture(names[i * 2], names[i * 2 + 1]);
            fsTextures[i] = texture;
            uint32_t width = vc::Call<uint32_t>(texture, 0x48);
            fsInvSize[i * 2] = 1.0f / float(width);
            uint32_t height = vc::Call<uint32_t>(fsTextures[i], 0x4C);
            fsInvSize[i * 2 + 1] = 1.0f / float(height);
        }
        fsFlags |= 2;
        Txd_PopCurrentTxd();
    }
    int file = FileMgr_OpenFile(reinterpret_cast<const char*>(0x0093EF8C), reinterpret_cast<const char*>(0x009010BC), 1);   // "config/fonts.bin", "rb"
    FileMgr_Read(file, &fsTokenCount, 4);
    FileMgr_Read(file, fsTokens, fsTokenCount * 0x15);
    int32_t language = fsLanguage;
    int style = 0;
    for (uint8_t* p = reinterpret_cast<uint8_t*>(0x01C0A948); p < reinterpret_cast<uint8_t*>(0x0210A4AC); p += kFsGlyphStride, style++) {
        if (language == 7 || language == 6) {
            At<int32_t>(p, -0x7FB28) = 0;
            At<int32_t>(p, -0x7FB24) = 0;
            At<int16_t>(p, 0) = 0x10;
        } else if (style < 8) {
            FileMgr_Read(file, p - 0x7FB28, 0x800);
            FileMgr_Read(file, p, 2);
            language = fsLanguage;
        }
    }
    FileMgr_CloseFile(file);
    fsButtonGlyphCount = 0;
    SetDefaults();
    for (int i = 0; i < 9; i++) {
        fsHeights[i] = 0.5f;
    }
    At<int16_t>(fsDefaults, 0x20) = 2;
    At<int16_t>(fsDefaults, 0x22) = 0;
    At<int16_t>(fsDefaults, 0x24) = 0;
    fsDefaults[0x26] = uint8_t((fsDefaults[0x26] & 0xC2) | 2);
    float x = fsScreenScaleX * 0.5f;
    for (int i = 0; i < 11; i++) {
        At<uint32_t>(fsDetails, i * 4) = At<uint32_t>(fsDefaults, i * 4);
    }
    fsScaleX = x;
    fsScaleY = 0.5f * fsScreenScaleY;
    SetRgba(fsColour, 0xFF, 0xFF, 0xFF, 0);
    SetRgba(fsColour2, 0x80, 0x80, 0x80, 0x80);
    fsShadow = 0;
    FontSetups_Load();
    if (0.0f == fsLineHeight[0]) {
        fsLineHeight[0] = float(int32_t(At<int16_t>(fsGlyphs, 0x212)) - int32_t(At<int16_t>(fsGlyphs, 0x20E)));
    }
    if (0.0f == fsLineHeight[1]) {
        fsLineHeight[1] = float(int32_t(At<int16_t>(fsGlyphs, kFsGlyphStride + 0x212)) -
                                int32_t(At<int16_t>(fsGlyphs, kFsGlyphStride + 0x20E)));
    }
    fsLoaded = 1;
    int alternative = fsLanguage == 7 ? 8 : fsLanguage == 6 ? 9 : -1;
    if (alternative != -1) {
        FontStyle_Load(alternative);
        float invWidth = fsInvSize[alternative * 2];
        void* texture = fsTextures[alternative];
        fsInvSize[0] = invWidth;
        float invHeight = fsInvSize[alternative * 2 + 1];
        fsInvSize[1] = invHeight;
        fsTextures[1] = texture;
        fsInvSize[3] = invHeight;
        fsTextures[0] = texture;
        fsInvSize[2] = invWidth;
    }
}

// 0x689A80's constant stores (address, value), in its order: the button glyphs' list and
// the glyph remapping tables
struct FontStore {
    uint32_t address;
    uint32_t value;
};
const FontStore kButtonTableStores[] = {
    {0x01B892D0, 0x36}, {0x0208B274, 0x36}, {0x0208B278, 0x36}, {0x01B89258, 0x7},
    {0x01B8925C, 0x6}, {0x01B89260, 0x4}, {0x01B89264, 0x5}, {0x01B89268, 0xF},
    {0x01B8926C, 0xE}, {0x01B89270, 0x8}, {0x01B89274, 0x9}, {0x01B89278, 0xA},
    {0x01B8927C, 0xB}, {0x01B89280, 0xC}, {0x01B89284, 0xD}, {0x01B89288, 0x17},
    {0x01B8928C, 0x18}, {0x01B89290, 0x13}, {0x01B89294, 0x15}, {0x01B89298, 0x14},
    {0x01B8929C, 0x16}, {0x01B892A0, 0x1A}, {0x01B892A4, 0x1B}, {0x01B892A8, 0x6A},
    {0x01B892AC, 0x82}, {0x01B892B0, 0x2F}, {0x01B892B4, 0x4A}, {0x01B892B8, 0x2B},
    {0x01B892BC, 0x2C}, {0x01B892C0, 0x2A}, {0x01B892C4, 0x4B}, {0x01B892C8, 0x32},
    {0x01B892CC, 0x46}, {0x01B892D4, 0x34}, {0x01B892D8, 0x7D}, {0x01B892DC, 0x73},
    {0x01B892E0, 0x7E}, {0x01B892E4, 0x76}, {0x01B892E8, 0x72}, {0x01B892EC, 0x7F},
    {0x01B892F0, 0x75}, {0x01B892F4, 0x6F}, {0x01B892F8, 0x80}, {0x01B892FC, 0x77},
    {0x01B89300, 0x6C}, {0x01B89304, 0x71}, {0x01B89308, 0x78}, {0x01B8930C, 0x79},
    {0x01B89310, 0x74}, {0x01B89314, 0x70}, {0x01B89318, 0x6E}, {0x01B8931C, 0x6B},
    {0x01B89320, 0x6D}, {0x01B89324, 0x7A}, {0x0208B270, 0x34}, {0x01B89328, 0x7B},
    {0x01B8932C, 0x7C}, {0x01B89330, 0x98}, {0x01B89334, 0x99}, {0x01B89338, 0x9A},
    {0x0208B27C, 0x39}, {0x01B8933C, 0x7}, {0x01B89340, 0x5}, {0x01B89344, 0x4},
    {0x01B89348, 0x6}, {0x01B8934C, 0x8}, {0x01B89350, 0xB}, {0x01B89354, 0x7},
    {0x01B89358, 0xC}, {0x01B89360, 0x8}, {0x01B89370, 0x16}, {0x01B89388, 0x4},
    {0x01B8938C, 0x5}, {0x01B89390, 0x4}, {0x01B89398, 0x5}, {0x01B893A8, 0x5},
    {0x01B893B0, 0x5}, {0x01B893BC, 0x4}, {0x01B893C0, 0x5}, {0x01B893C4, 0x4},
    {0x01B893CC, 0x4}, {0x01B893D0, 0x4}, {0x01B893D8, 0x16}, {0x01B893E0, 0x4},
    {0x01B893E8, 0x4}, {0x01B893EC, 0x5}, {0x01B893F0, 0x5}, {0x01B89400, 0x4},
    {0x01B89404, 0x4}, {0x01B89408, 0x5}, {0x01B89414, 0x4}, {0x01B8941C, 0x5},
    {0x01B89424, 0x4}, {0x01B8942C, 0x5}, {0x01B89430, 0x5}, {0x01B89450, 0x5},
    {0x01B89454, 0x5}, {0x01B89458, 0x5}, {0x01B8945C, 0x5}, {0x01B89464, 0x5},
    {0x01B89468, 0x4}, {0x01B89474, 0x5}, {0x01B89480, 0x5}, {0x01B8935C, 0x9},
    {0x01B89364, 0xB}, {0x01B89368, 0x17}, {0x01B8936C, 0x18}, {0x01B89374, 0x15},
    {0x01B89378, 0x13}, {0x01B8937C, 0x14}, {0x01B89380, 0xA}, {0x01B89384, 0xD},
    {0x01B89394, 0x6}, {0x01B8939C, 0x9B}, {0x01B893A0, 0x9C}, {0x01B893A4, 0xB},
    {0x01B893AC, 0x7}, {0x01B893B4, 0x9}, {0x01B893B8, 0xC}, {0x01B893C8, 0x6},
    {0x01B893D4, 0x6}, {0x01B893DC, 0xC}, {0x01B893E4, 0x6}, {0x01B893F4, 0x6},
    {0x01B893F8, 0xC}, {0x01B893FC, 0x6}, {0x01B8940C, 0x7}, {0x01B89410, 0x6},
    {0x01B89418, 0x7}, {0x01B89420, 0x6}, {0x01B89428, 0x7}, {0x01B89434, 0x6},
    {0x01B89438, 0x6}, {0x01B8943C, 0xC}, {0x01B89440, 0x13}, {0x01B89444, 0x14},
    {0x01B89448, 0xB}, {0x01B8944C, 0xB}, {0x01B89460, 0x19}, {0x01B8946C, 0x6},
    {0x01B89470, 0x7}, {0x01B89478, 0x1B}, {0x01B8947C, 0xC}, {0x0208B284, 0x8B},
};

// 0x689A80 (cdecl): the button glyphs set up: the list of button glyphs (0x8B of them) and
// their tables, the button textures from the HUD's; every style's button glyph sizes (by
// its texture, up to 28 high, width scaled; 28 x 28 without one); the defaults, setups
// and first two line heights as at start-up
void __cdecl Font_InitialiseButtons() {
    for (const FontStore& store : kButtonTableStores) {
        *reinterpret_cast<uint32_t*>(uintptr_t(store.address)) = store.value;
    }
    void* texture0 = At<void*>(reinterpret_cast<uint8_t*>(0x00C66F58), 0);
    for (int i = 0; i < 5; i++) {
        fsButtons[i] = texture0;
    }
    for (uint8_t* table = fsGlyphs; table < reinterpret_cast<uint8_t*>(0x0208A528); table += kFsGlyphStride) {
        uint8_t* entry = table;
        for (int32_t i = 0; i < fsButtonGlyphCount; i++, entry += 8) {
            int32_t glyph = fsButtonGlyphs[i];
            At<int16_t>(entry, -4) = 0;
            At<int16_t>(entry, -2) = 0;
            void* texture;
            if (glyph == 0x9B || glyph == 0x9C) {
                texture = fsHudTextures[4];
            } else {
                re::Ptr32<const uint16_t> text;
                texture = Font_GetButtonTexture(glyph, &text);
            }
            if (!texture) {
                At<int16_t>(entry, 0) = 0x1C;
                At<int16_t>(entry, 2) = 0x1C;
                continue;
            }
            uint32_t h = vc::Call<uint32_t>(texture, 0x4C);
            At<int16_t>(entry, 2) = int16_t(h < 0x1C ? vc::Call<uint32_t>(texture, 0x4C) : 0x1C);
            int32_t w = vc::Call<int32_t>(texture, 0x48);
            int32_t scaled = w * int32_t(At<int16_t>(entry, 2));
            uint32_t full = vc::Call<uint32_t>(texture, 0x4C);
            At<int16_t>(entry, 0) = int16_t(uint32_t(scaled) / full);
        }
    }
    SetDefaults();
    At<int16_t>(fsDefaults, 0x20) = 2;
    At<int16_t>(fsDefaults, 0x22) = 0;
    At<int16_t>(fsDefaults, 0x24) = 0;
    fsDefaults[0x26] = uint8_t((fsDefaults[0x26] & 0xC2) | 2);
    DetailsToDefaults();
    SetRgba(fsColour, 0xFF, 0xFF, 0xFF, 0);
    SetRgba(fsColour2, 0x80, 0x80, 0x80, 0x80);
    fsShadow = 0;
    FontSetups_Load();
    if (0.0f == fsLineHeight[0]) {
        fsLineHeight[0] = float(int32_t(At<int16_t>(fsGlyphs, 0x672)) - int32_t(At<int16_t>(fsGlyphs, 0x66E)));
    }
    fsLoaded = 1;
    if (0.0f == fsLineHeight[1]) {
        fsLineHeight[1] = float(int32_t(At<int16_t>(fsGlyphs, kFsGlyphStride + 0x672)) -
                                int32_t(At<int16_t>(fsGlyphs, kFsGlyphStride + 0x66E)));
    }
}

// 0x68A250 (cdecl): the details reset for a new frame (grey), no drop shadow, no last
// glyph, the render buffer emptied
void __cdecl Font_Reset() {
    DetailsToDefaults();
    SetRgba(fsColour, 0x80, 0x80, 0x80, 0xFF);
    SetRgba(fsColour2, 0x80, 0x80, 0x80, 0x80);
    fsShadow = 0;
    fsLastGlyph = -1;
    fsRenderCursor = reinterpret_cast<uint8_t*>(0x01B8A588);
}

#define FONTSYS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// Register hazard (regcheck2): the game's 0x688810 (LoadSetups) stores the ecx 0x688780
// leaves (the setup) after calling it: turn 0x688780 on only with 0x688810.
FONTSYS_REVERSED("CFontSetup::CopyFrom_688780", 0x00688780, &FontSetup_CopyFromImpl);
FONTSYS_REVERSED("CFont::LoadSetups_688810", 0x00688810, &FontSetups_LoadImpl);
FONTSYS_REVERSED("CFont::GetButtonTexture_688980", 0x00688980, &Font_GetButtonTextureImpl);
FONTSYS_REVERSED("CFontDetails::CopyFrom_688DD0", 0x00688DD0, &FontDetails_CopyFrom);
FONTSYS_REVERSED("CFont::Begin_688E60", 0x00688E60, &Font_BeginImpl);
FONTSYS_REVERSED("CFont::GetBoxAt_688EA0", 0x00688EA0, &FontLayout_GetBoxAt);
FONTSYS_REVERSED("CFont::FormatText_688ED0", 0x00688ED0, &Font_FormatText);
FONTSYS_REVERSED("CFontSetup::Apply_689400", 0x00689400, &FontSetup_Apply);
FONTSYS_REVERSED("CFont::Initialise_6895D0", 0x006895D0, &Font_Initialise);
FONTSYS_REVERSED("CFont::InitialiseButtons_689A80", 0x00689A80, &Font_InitialiseButtons);
FONTSYS_REVERSED("CFont::Reset_68A250", 0x0068A250, &Font_Reset);

} // namespace
