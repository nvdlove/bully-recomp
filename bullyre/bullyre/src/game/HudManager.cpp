// 0x513FE0 to 0x514EF0: the HUD as a whole (Bully's own): the 48 components (one of
// each kind, g_hudComponents), made, reset, shown and hidden together, drawn in the
// render list's order; the HUD's textures (the button icons by language); the script
// text wrappers; and the three prompt lines drawn over it (with a fade).

#include <cstddef>
#include <cstdint>

#include "game/Hud.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

constexpr int kNumComponents = HUD_NUM_COMPONENT_TYPES;   // 48
constexpr int kNumTextures = 0x1A7;

GAME_ARRAY(HUDComponent*, g_hudComponents, 0x00C674A0);
GAME_ARRAY(void*, g_hudTextures, 0x00C66DD0);             // 0x1A7, by the texture tables' indices
GAME_ARRAY(uint8_t, g_savedVisible, 0x00C66D98);          // by component
GAME_ARRAY(uint8_t, g_savedSuppressed, 0x00C66D68);
GAME_VAR(uint8_t, g_hudInitialised, 0x00C67560);
GAME_VAR(uint8_t, g_hudHideMost, 0x00C67561);             // (only the tutorial and component 12 drawn)
GAME_VAR(float, g_hudFloatC67564, 0x00C67564);
GAME_VAR(int32_t, g_hudIntC67568, 0x00C67568);
GAME_VAR(float, g_hudFloatA64880, 0x00A64880);
GAME_VAR(float, g_hudFloatA64884, 0x00A64884);
GAME_VAR(float, g_hudFloatA64888, 0x00A64888);

// the order components are drawn in (0x514EF0), ended by component -1
struct HUDRenderListEntry {
    int32_t component;     // +0
    uint8_t drawnHiding;   // +4 drawn as usual while most of the HUD is hidden
    uint8_t drawnAlways;   // +5 drawn in the widescreen and weapon-select modes
    uint8_t stopsClock;    // +6 the clock stops while it's shown (0x513FE0)
    uint8_t unknown7;
};
static_assert(sizeof(HUDRenderListEntry) == 8);
GAME_ARRAY(HUDRenderListEntry, g_hudRenderList, 0x00A66E50);

// a texture table: an index into g_hudTextures, the texture's and its mask's names;
// ended by index -1
struct HUDTextureEntry {
    int32_t index;
    re::Ptr32<const char> name;
    re::Ptr32<const char> mask;
};
static_assert(sizeof(HUDTextureEntry) == 0xC);
GAME_ARRAY(HUDTextureEntry, g_hudTexturesCommon, 0x00A64890);
GAME_ARRAY(HUDTextureEntry, g_hudTexturesLanguage1, 0x00A65488);   // the buttons, by language
GAME_ARRAY(HUDTextureEntry, g_hudTexturesLanguage2, 0x00A65ED8);
GAME_ARRAY(HUDTextureEntry, g_hudTexturesLanguage3, 0x00A659B0);
GAME_ARRAY(HUDTextureEntry, g_hudTexturesLanguage4, 0x00A66400);
GAME_ARRAY(HUDTextureEntry, g_hudTexturesLanguage6, 0x00A64F60);
GAME_ARRAY(HUDTextureEntry, g_hudTexturesDefault, 0x00A66928);

// the prompt lines
GAME_ARRAY(char, g_prompt1, 0x00C6748C);   // 16 characters each
GAME_ARRAY(char, g_prompt2, 0x00C6747C);
GAME_ARRAY(char, g_prompt3, 0x00C6746C);
GAME_VAR(float, g_promptTime, 0x00C66DC8);   // left to show them
GAME_VAR(float, g_promptFade, 0x00C66DCC);   // 0..1
GAME_VAR(float, g_promptTimeScale, 0x00A66F50);
GAME_VAR(float, ms_fTimeStep, 0x00C1A9A4);

GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(int32_t, g_cameraMode, 0x00C3CCF8);    // 2: the photo camera
GAME_VAR(int32_t, g_widescreen, 0x00C3CD0C);
GAME_VAR(uint8_t, g_hudEnabled, 0x00C18701);
GAME_VAR(int32_t, g_intCF108C, 0x00CF108C);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, ms_channelError, 0x00CA3920);   // CStreaming's failed channel (-1: none)
inline int32_t& ChannelTries(int channel) {
    return *reinterpret_cast<int32_t*>(0x00CA38E0 + channel * 0x38);   // ms_channel[].tries
}
GAME_ARRAY(int32_t, g_padButtonBits, 0x00A14810);   // the pad's bit for each button
inline void* const TheButtonMap = reinterpret_cast<void*>(0x00A14810);
inline void* const g_objBA4FF8 = reinterpret_cast<void*>(0x00BA4FF8);
inline void* const g_obj208D388 = reinterpret_cast<void*>(0x0208D388);
inline void* const g_objBCD098 = reinterpret_cast<void*>(0x00BCD098);

GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);   // (just a ret)
GAME_FN(void, __thiscall, Fn_68F230, 0x0068F230, void* self);
GAME_FN(void, __thiscall, Fn_68F340, 0x0068F340, void* self);
GAME_FN(void, __thiscall, Fn_68F440, 0x0068F440, void* self);
GAME_FN(void, __thiscall, Fn_424340, 0x00424340, void* self);
GAME_FN(void, __thiscall, Radar_Fn54B430, 0x0054B430, void* radar);
GAME_FN(void, __thiscall, Radar_Fn550EA0, 0x00550EA0, void* radar);
GAME_FN(void, __thiscall, Weapons_Fn55C5D0, 0x0055C5D0, void* weapons);
GAME_FN(void, __thiscall, Weapons_Fn55D8E0, 0x0055D8E0, void* weapons);
GAME_FN(void, __thiscall, Text_Fn55B180, 0x0055B180, void* text);
GAME_FN(void, __cdecl, Fn_5B0900, 0x005B0900, void* component);
GAME_FN(void, __cdecl, NiRelease, 0x005F0F90, void* object);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int);
GAME_FN(void, __cdecl, Txd_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, Txd_LoadTxdFile, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(HUDComponent*, __cdecl, CreateHUDComponent, 0x0053D030, int type);
GAME_FN(const char*, __cdecl, GetHUDComponentTypeName, 0x0053D010, int type);
GAME_FN(void, __thiscall, HUDComponent_Render, 0x0053CF50, HUDComponent* self);
GAME_FN(void*, __cdecl, Layout_Open, 0x005B08A0, const char* path, uint32_t hash);
GAME_FN(void, __thiscall, Layout_Apply, 0x005B0760, void* layout, HUDComponent* component, const char* name);
GAME_FN(void, __cdecl, Layout_Close, 0x005B08E0, void* layout);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
inline char* __cdecl Strncpy(char* dest, const char* source, size_t count) { return crt::Strncpy(dest, source, count); }
GAME_FN(void, __cdecl, DrawPrompt1, 0x0068F580, const char* text, float fade);
GAME_FN(void, __cdecl, DrawPrompt2, 0x0068F5C0, const char* text, float fade);
GAME_FN(void, __cdecl, DrawPrompt3, 0x0068F600, const char* text, float fade);
GAME_FN(void, __cdecl, FadePromptsIn, 0x00514520);
GAME_FN(void, __cdecl, UpdatePrompts_, 0x00514DC0);
GAME_FN(void, __cdecl, LoadLayout_, 0x00514360);
GAME_FN(void, __cdecl, Reset_, 0x005140C0);
GAME_FN(void*, __cdecl, GetButtonTexture_, 0x005145B0, int button, uint8_t* isKey, int pad, int player);
GAME_FN(int, __cdecl, GetActivePad, 0x00738070, int);
GAME_FN(uint8_t, __cdecl, Pad_Fn738500, 0x00738500, int pad);
GAME_FN(const int32_t*, __thiscall, ButtonMap_GetKeys, 0x00454CD0, void* map, int player);

// the script text components (the big and the small)
GAME_FN(void, __thiscall, ScriptText_AddFloat, 0x00553BA0, HUDComponent* self, float value);
GAME_FN(void, __thiscall, ScriptText_AddString, 0x0053A7C0, HUDComponent* self, const char* text);
GAME_FN(void, __thiscall, ScriptText_AddText, 0x0053A820, HUDComponent* self, const uint16_t* text);
GAME_FN(void, __thiscall, ScriptText_Show, 0x0053A880, HUDComponent* self, const char* text, float seconds);
GAME_FN(void, __thiscall, ScriptText_Clear, 0x005539D0, HUDComponent* self);
GAME_FN(uint8_t, __thiscall, ScriptText_IsShowing, 0x0053A5C0, HUDComponent* self);

uint8_t IsVisible(HUDComponent* c) {
    return vc::Call<uint8_t>(c, 0x10);
}
void SetVisible(HUDComponent* c, int visible) {
    vc::Call<void>(c, 0x14, visible);
}

void LoadTextures(const HUDTextureEntry* table) {
    for (int i = 0; table[i].index != -1; i++) {
        g_hudTextures[table[i].index] = Txd_FindTexture(table[i].name, table[i].mask);
    }
}

void ClearPrompts() {
    g_prompt1[0] = 0;
    g_promptTime = 0.0f;
    g_prompt2[0] = 0;
    g_promptFade = 0.0f;
    g_prompt3[0] = 0;
}

} // namespace

namespace HUDManager {

// 0x513FE0: whether a shown component stops the clock
static uint8_t __cdecl IsClockStopped() {
    for (int i = 0; g_hudRenderList[i].component != -1; i++) {
        if (IsVisible(g_hudComponents[g_hudRenderList[i].component]) && g_hudRenderList[i].stopsClock) {
            return 1;
        }
    }
    return 0;
}

// 0x514040: the components released, the textures and the HUD's dictionary too
static void __cdecl Shutdown() {
    EmptyThiscall(g_objBA4FF8);
    Fn_68F230(g_obj208D388);
    Radar_Fn54B430(g_hudComponents[HUD_RADAR]);
    for (int i = 0; i < kNumComponents; i++) {
        Fn_5B0900(g_hudComponents[i]);
    }
    for (int i = 0; i < kNumTextures; i++) {
        if (g_hudTextures[i]) {
            NiRelease(g_hudTextures[i]);
            g_hudTextures[i] = nullptr;
        }
    }
    Txd_RemoveTxdSlot(Txd_FindTxdSlot("HUDElems", 0));
    g_hudInitialised = 0;
}

// 0x5140C0: everything back to the start (each component's Reset)
static void __cdecl Reset() {
    g_hudFloatA64880 = 1.0f;
    g_hudFloatA64884 = 1.0f;
    g_hudIntC67568 = 0;
    g_prompt1[0] = 0;
    g_hudFloatC67564 = 0.0f;
    g_hudHideMost = 0;
    g_hudFloatA64888 = 1.0f;
    g_promptFade = 0.0f;
    g_promptTime = 0.0f;
    for (int i = 0; i < kNumComponents; i++) {
        vc::Call<void>(g_hudComponents[i], 0x1C);
    }
    g_hudInitialised = 1;
}

// 0x514120
static uint8_t __cdecl IsInitialised() {
    return g_hudInitialised;
}

// 0x514130: each component's visibility and suppression saved
static void __cdecl SaveVisibility() {
    for (int i = 0; i < kNumComponents; i++) {
        HUDComponent* c = g_hudComponents[i];
        g_savedVisible[i] = IsVisible(c);
        g_savedSuppressed[i] = c->suppressed;
    }
}

// 0x514160: all hidden and suppressed but the script texts (1 and 2)
static void __cdecl HideAllButScriptText() {
    for (int i = 0; i < kNumComponents; i++) {
        if (i != HUD_BIG_TEXT_FOR_SCRIPT && i != HUD_SMALL_TEXT_FOR_SCRIPT) {
            HUDComponent* c = g_hudComponents[i];
            SetVisible(c, 0);
            c->suppressed = 1;
        }
    }
}

// 0x514190: what 0x514130 saved put back
static void __cdecl RestoreVisibility() {
    for (int i = 0; i < kNumComponents; i++) {
        HUDComponent* c = g_hudComponents[i];
        SetVisible(c, g_savedVisible[i]);
        c->suppressed = g_savedSuppressed[i];
    }
}

// 0x5141D0: all but one hidden (their visibility saved)
static void __cdecl HideAllExcept(int except) {
    for (int i = 0; i < kNumComponents; i++) {
        if (i != except) {
            g_savedVisible[i] = IsVisible(g_hudComponents[i]);
            SetVisible(g_hudComponents[i], 0);
        }
    }
}

// 0x514210: their visibility back
static void __cdecl RestoreAllExcept(int except) {
    for (int i = 0; i < kNumComponents; i++) {
        if (i != except) {
            SetVisible(g_hudComponents[i], g_savedVisible[i]);
        }
    }
}

// 0x514240 .. 0x5142D0: the big script text's
static void __cdecl BigText_AddFloat(float value) {
    ScriptText_AddFloat(g_hudComponents[HUD_BIG_TEXT_FOR_SCRIPT], value);
}
static void __cdecl BigText_AddString(const char* text) {
    ScriptText_AddString(g_hudComponents[HUD_BIG_TEXT_FOR_SCRIPT], text);
}
static void __cdecl BigText_AddText(const uint16_t* text) {
    ScriptText_AddText(g_hudComponents[HUD_BIG_TEXT_FOR_SCRIPT], text);
}
static void __cdecl BigText_Show(const char* text, float seconds) {
    ScriptText_Show(g_hudComponents[HUD_BIG_TEXT_FOR_SCRIPT], text, seconds);
}
static void __cdecl BigText_Clear() {
    ScriptText_Clear(g_hudComponents[HUD_BIG_TEXT_FOR_SCRIPT]);
}
static uint8_t __cdecl BigText_IsShowing() {
    return ScriptText_IsShowing(g_hudComponents[HUD_BIG_TEXT_FOR_SCRIPT]);
}

// 0x5142E0 .. 0x514340: the small one's
static void __cdecl SmallText_AddFloat(float value) {
    ScriptText_AddFloat(g_hudComponents[HUD_SMALL_TEXT_FOR_SCRIPT], value);
}
static void __cdecl SmallText_AddString(const char* text) {
    ScriptText_AddString(g_hudComponents[HUD_SMALL_TEXT_FOR_SCRIPT], text);
}
static void __cdecl SmallText_AddText(const uint16_t* text) {
    ScriptText_AddText(g_hudComponents[HUD_SMALL_TEXT_FOR_SCRIPT], text);
}
static void __cdecl SmallText_Show(const char* text, float seconds) {
    ScriptText_Show(g_hudComponents[HUD_SMALL_TEXT_FOR_SCRIPT], text, seconds);
}

// 0x514360: the layout file (by language) applied to every component
static void __cdecl LoadLayout() {
    const char* path;
    if (g_language == 7) {
        path = "Config\\hdt\\hud_datj.htb";
    } else if (g_language == 6) {
        path = "Config\\hdt\\hud_datr.htb";
    } else {
        path = "Config\\hdt\\hud_data.htb";
    }
    void* layout = Layout_Open(path, 0x3AD708B8);
    if (!layout) {
        return;
    }
    for (int i = 0; i < kNumComponents; i++) {
        Layout_Apply(layout, g_hudComponents[i], GetHUDComponentTypeName(i));
    }
    Layout_Close(layout);
}

// 0x5143D0: the first prompt line, shown for a moment (nullptr: all cleared)
static void __cdecl SetPrompt(const char* text) {
    if (text) {
        Strncpy(g_prompt1, text, 0x10);
        g_promptTime = 0.001f;
        return;
    }
    ClearPrompts();
}

// 0x514420: the first line, for a while
static void __cdecl SetPromptTimed(const char* text, float time) {
    if (text) {
        Strncpy(g_prompt1, text, 0x10);
        g_promptTime = time;
        return;
    }
    ClearPrompts();
}

// 0x514470: the second line
static void __cdecl SetPrompt2(const char* text, float time) {
    if (text) {
        Strncpy(g_prompt2, text, 0x10);
        g_promptTime = time;
        return;
    }
    ClearPrompts();
}

// 0x5144C0: the third
static void __cdecl SetPrompt3(const char* text, float time) {
    if (text) {
        Strncpy(g_prompt3, text, 0x10);
        g_promptTime = time;
        return;
    }
    ClearPrompts();
}

// 0x514510
static char* __cdecl GetPrompt() {
    return g_prompt1;
}

// 0x514520: a step of the prompts' fade in (to at most 1)
static void __cdecl FadeIn() {
    double step = double(g_promptFade) + 0.012500000186264515;
    g_promptFade = float(g_promptFade + (step + step));
    if (!(g_promptFade < 1.0f)) {
        g_promptFade = 1.0f;
    }
}

// 0x514570: whether the clothing, barber or tattoo component is shown
static uint8_t __cdecl IsShopShown() {
    if (IsVisible(g_hudComponents[HUD_CLOTHING]) || IsVisible(g_hudComponents[HUD_BARBER]) ||
        IsVisible(g_hudComponents[HUD_TATTOO])) {
        return 1;
    }
    return 0;
}

// 0x5145B0: a button's icon: the pad's (by the button's bit; 0x10.. the sticks'
// directions) or, for keys, the key the player has on it (isKey set when it's not a
// plain key's letter; 0x19 and 0x1A the two movement sets)
static void* __cdecl GetButtonTexture(int button, uint8_t* isKey, int pad, int player) {
    if (re::Flag(pad)) {
        if (button < 0x10) {
            switch (g_padButtonBits[button]) {
            case 0x1: return g_hudTextures[0x13];
            case 0x2: return g_hudTextures[0x14];
            case 0x4: return g_hudTextures[0x15];
            case 0x8: return g_hudTextures[0x16];
            case 0x10: return g_hudTextures[0x17];
            case 0x20: return g_hudTextures[0x18];
            case 0x40: return g_hudTextures[0xA];
            case 0x80: return g_hudTextures[0xD];
            case 0x100: return g_hudTextures[0x8];
            case 0x200: return g_hudTextures[0xB];
            case 0x400: return g_hudTextures[0x9];
            case 0x800: return g_hudTextures[0xC];
            case 0x1000: return g_hudTextures[0x4];
            case 0x2000: return g_hudTextures[0x5];
            case 0x4000: return g_hudTextures[0x6];
            case 0x8000: return g_hudTextures[0x7];
            default: return nullptr;
            }
        }
        switch (button) {
        case 0x10: return g_hudTextures[0x1C];
        case 0x11: return g_hudTextures[0x1D];
        case 0x12: return g_hudTextures[0x1E];
        case 0x13: return g_hudTextures[0x1F];
        case 0x14: return g_hudTextures[0x20];
        case 0x15: return g_hudTextures[0x21];
        case 0x16: return g_hudTextures[0x22];
        case 0x17: return g_hudTextures[0x23];
        case 0x19: return g_hudTextures[0x1A];
        case 0x1A: return g_hudTextures[0x1B];
        default: return nullptr;
        }
    }
    const int32_t* keys = ButtonMap_GetKeys(TheButtonMap, player);
    if (button == 0x19) {
        if (keys[0x10] == 0x108 && keys[0x11] == 0x106 && keys[0x14] == 0x109 && keys[0x15] == 0x107) {
            return g_hudTextures[0x97];   // (the arrow keys)
        }
        *isKey = 1;
        return g_hudTextures[0x93];
    }
    if (button == 0x1A) {
        if (keys[0x12] == 0x108 && keys[0x13] == 0x106 && keys[0x16] == 0x109 && keys[0x17] == 0x107) {
            return g_hudTextures[0x97];
        }
        *isKey = 1;
        return g_hudTextures[0x94];
    }
    int key = keys[button];
    if (key < 0x10A) {
        key += 0x9D;
        if (key == -1) {
            return nullptr;
        }
        return g_hudTextures[key];
    }
    *isKey = 1;
    static const uint16_t kKeyTextures[0x18] = {
        0x0F, 0x0E, 0x10, 0x11, 0x8E, 0x8D, 0x8A, 0x89, 0x8B, 0x8C, 0x90, 0x91,
        0x8F, 0x92, 0x96, 0x95, 0x93, 0x93, 0x94, 0x94, 0x93, 0x93, 0x94, 0x94,
    };
    if (uint32_t(button) > 0x17) {
        return nullptr;
    }
    return g_hudTextures[kKeyTextures[button]];
}

// 0x514A70: the HUD's dictionary loaded, the components made and laid out, the
// textures found (the buttons' by language)
static void __cdecl Init() {
    PushMemoryTag(0x17);
    int txd = Txd_AddTxdSlot("HUDElems");
    Txd_LoadTxdFile(txd, "TXD/HUDElems.NFT");
    Txd_AddRef(txd);
    PopMemoryTag();
    PushMemoryTag(0x30);
    for (int i = 0; i < kNumComponents; i++) {
        g_hudComponents[i] = CreateHUDComponent(i);
    }
    LoadLayout_();
    Reset_();
    PopMemoryTag();
    for (int i = 0; i < kNumTextures; i++) {
        g_hudTextures[i] = nullptr;
    }
    PushMemoryTag(0x17);
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(txd);
    switch (g_language) {
    case 1: LoadTextures(g_hudTexturesLanguage1); break;
    case 2: LoadTextures(g_hudTexturesLanguage2); break;
    case 3: LoadTextures(g_hudTexturesLanguage3); break;
    case 4: LoadTextures(g_hudTexturesLanguage4); break;
    case 6: LoadTextures(g_hudTexturesLanguage6); break;
    default: LoadTextures(g_hudTexturesDefault); break;
    }
    LoadTextures(g_hudTexturesCommon);
    // the sticks' directions: the d-pad's icons
    g_hudTextures[0x1C] = g_hudTextures[0xF];
    g_hudTextures[0x1D] = g_hudTextures[0x10];
    g_hudTextures[0x1E] = g_hudTextures[0xF];
    g_hudTextures[0x1F] = g_hudTextures[0x10];
    g_hudTextures[0x20] = g_hudTextures[0xE];
    g_hudTextures[0x21] = g_hudTextures[0x11];
    g_hudTextures[0x22] = g_hudTextures[0xE];
    g_hudTextures[0x23] = g_hudTextures[0x11];
    Txd_PopCurrentTxd();
    PopMemoryTag();
    PushMemoryTag(0x30);
    Fn_68F340(g_obj208D388);
    EmptyThiscall(g_objBA4FF8);
    PopMemoryTag();
}

// 0x514DC0: the prompts' time and fade moved on, and the lines drawn while faded in
static void __cdecl UpdatePrompts() {
    if (0.0f < g_promptTime) {
        if (1.0f > g_promptFade) {
            FadePromptsIn();
        }
        g_promptTime = g_promptTime - float(g_promptTimeScale * ms_fTimeStep);
        if (!(g_promptTime > 0.0f)) {
            g_promptTime = 0.0f;
        }
    } else {
        g_promptFade = float(g_promptFade - 0.14000000059604645);
        if (!(g_promptFade > 0.0f)) {
            g_promptFade = 0.0f;
        }
    }
    if (g_promptFade > 0.0f) {
        DrawPrompt1(g_prompt1, g_promptFade);
        DrawPrompt2(g_prompt2, g_promptFade);
        DrawPrompt3(g_prompt3, g_promptFade);
    }
}

// 0x514EC0: a button's icon for the player's pad (or keys)
static void* __cdecl GetButtonTextureForPlayer(int button) {
    uint8_t isKey;   // (not initialised: only written)
    return GetButtonTexture_(button, &isKey, Pad_Fn738500(GetActivePad(0)), 0);
}

// 0x514EF0: the HUD drawn, the components in the render list's order (only the
// text while a streaming channel keeps failing)
static void __cdecl Render() {
    if (ms_channelError != -1 && ChannelTries(ms_channelError) >= 3000) {
        Text_Fn55B180(g_hudComponents[HUD_TEXT]);
        return;
    }
    uint8_t hideMost = g_cameraMode == 2;
    if (g_hudHideMost) {
        hideMost = 1;
    }
    if (hideMost) {
        if (g_widescreen == 0) {
            Weapons_Fn55C5D0(g_hudComponents[HUD_WEAPONS]);
        }
    } else {
        g_intCF108C = 0;
        *reinterpret_cast<int32_t*>(g_playerPed + 0x1458) = 1;
    }
    UpdatePrompts_();
    SetRenderState(9, 2);
    SetRenderState(2, 3);
    SetRenderState(0xC, 1);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    if (g_hudEnabled) {
        Fn_424340(g_objBCD098);
        Fn_68F440(g_obj208D388);
        for (int i = 0; g_hudRenderList[i].component != -1; i++) {
            const HUDRenderListEntry& e = g_hudRenderList[i];
            HUDComponent* weapons = g_hudComponents[HUD_WEAPONS];
            if (hideMost && !e.drawnHiding) {
                if (e.component == HUD_TUTORIAL) {
                    if (vc::At<int32_t>(weapons, 0x138) == 1) {
                        HUDComponent_Render(g_hudComponents[HUD_TUTORIAL]);
                    }
                } else if (e.component == HUD_SECONDARY_BAR) {
                    if (vc::At<uint8_t>(g_hudComponents[HUD_SECONDARY_BAR], 0xBC)) {
                        HUDComponent_Render(g_hudComponents[HUD_SECONDARY_BAR]);
                    }
                }
            } else if (vc::At<int32_t>(weapons, 0x138) == 2) {
                if (i == 0x14) {
                    Weapons_Fn55D8E0(weapons);
                }
                if (e.drawnAlways) {
                    HUDComponent_Render(g_hudComponents[e.component]);
                }
            } else if (g_widescreen != 0 && !e.drawnAlways) {
                if (e.component == HUD_TUTORIAL && vc::At<uint8_t>(g_hudComponents[HUD_TUTORIAL], 0x3C)) {
                    HUDComponent_Render(g_hudComponents[HUD_TUTORIAL]);
                }
            } else {
                HUDComponent_Render(g_hudComponents[e.component]);
            }
        }
        vc::At<uint8_t>(g_hudComponents[HUD_SOCIAL_ERRANDS], 0x4A) = 0;
        HUDComponent* radar = g_hudComponents[HUD_RADAR];
        if (!IsVisible(radar) || g_hudComponents[HUD_RADAR]->suppressed) {
            Radar_Fn550EA0(g_hudComponents[HUD_RADAR]);
        }
        EmptyThiscall(g_objBA4FF8);
    }
    Text_Fn55B180(g_hudComponents[HUD_TEXT]);
}

} // namespace HUDManager

#define C16A_HUD_REVERSED(name, fn, address)                                                       \
    static void* Original_##fn = nullptr;                                                        \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

using namespace HUDManager;
C16A_HUD_REVERSED("HUDManager::IsClockStopped", IsClockStopped, 0x00513FE0);
C16A_HUD_REVERSED("HUDManager::Shutdown", Shutdown, 0x00514040);
C16A_HUD_REVERSED("HUDManager::Reset", Reset, 0x005140C0);
C16A_HUD_REVERSED("HUDManager::IsInitialised", IsInitialised, 0x00514120);
C16A_HUD_REVERSED("HUDManager::SaveVisibility", SaveVisibility, 0x00514130);
C16A_HUD_REVERSED("HUDManager::HideAllButScriptText", HideAllButScriptText, 0x00514160);
C16A_HUD_REVERSED("HUDManager::RestoreVisibility", RestoreVisibility, 0x00514190);
C16A_HUD_REVERSED("HUDManager::HideAllExcept", HideAllExcept, 0x005141D0);
C16A_HUD_REVERSED("HUDManager::RestoreAllExcept", RestoreAllExcept, 0x00514210);
C16A_HUD_REVERSED("HUDManager::BigText_AddFloat", BigText_AddFloat, 0x00514240);
C16A_HUD_REVERSED("HUDManager::BigText_AddString", BigText_AddString, 0x00514260);
C16A_HUD_REVERSED("HUDManager::BigText_AddText", BigText_AddText, 0x00514280);
C16A_HUD_REVERSED("HUDManager::BigText_Show", BigText_Show, 0x005142A0);
C16A_HUD_REVERSED("HUDManager::BigText_Clear", BigText_Clear, 0x005142C0);
C16A_HUD_REVERSED("HUDManager::BigText_IsShowing", BigText_IsShowing, 0x005142D0);
C16A_HUD_REVERSED("HUDManager::SmallText_AddFloat", SmallText_AddFloat, 0x005142E0);
C16A_HUD_REVERSED("HUDManager::SmallText_AddString", SmallText_AddString, 0x00514300);
C16A_HUD_REVERSED("HUDManager::SmallText_AddText", SmallText_AddText, 0x00514320);
C16A_HUD_REVERSED("HUDManager::SmallText_Show", SmallText_Show, 0x00514340);
C16A_HUD_REVERSED("HUDManager::LoadLayout", LoadLayout, 0x00514360);
C16A_HUD_REVERSED("HUDManager::SetPrompt", SetPrompt, 0x005143D0);
C16A_HUD_REVERSED("HUDManager::SetPromptTimed", SetPromptTimed, 0x00514420);
C16A_HUD_REVERSED("HUDManager::SetPrompt2", SetPrompt2, 0x00514470);
C16A_HUD_REVERSED("HUDManager::SetPrompt3", SetPrompt3, 0x005144C0);
C16A_HUD_REVERSED("HUDManager::GetPrompt", GetPrompt, 0x00514510);
C16A_HUD_REVERSED("HUDManager::FadeIn", FadeIn, 0x00514520);
C16A_HUD_REVERSED("HUDManager::IsShopShown", IsShopShown, 0x00514570);
C16A_HUD_REVERSED("HUDManager::GetButtonTexture", GetButtonTexture, 0x005145B0);
C16A_HUD_REVERSED("HUDManager::Init", Init, 0x00514A70);
C16A_HUD_REVERSED("HUDManager::UpdatePrompts", UpdatePrompts, 0x00514DC0);
C16A_HUD_REVERSED("HUDManager::GetButtonTextureForPlayer", GetButtonTextureForPlayer, 0x00514EC0);
C16A_HUD_REVERSED("HUDManager::Render", Render, 0x00514EF0);
