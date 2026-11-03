// The camera: HUDPhoto's viewfinder (the frame, the zoom ladder, the brackets and
// cross-hair, the iris closing), the menu after a shot (save it to the album, the
// memory card's messages), and the photo album (0xAB4688: the photos as JPEGs on the
// save device, a big one and 5 thumbnails loaded as textures).
// 0x543D30..0x546490 (HUDPhoto's constructor is Hud.cpp's).

#include <windows.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "reversed.h"

namespace {

struct CVector2D {
    float x, y;
};
struct CVector {
    float x, y, z;
};

// HUDPhoto, with what's understood here (Hud.h's)
struct Photo : HUDComponent {
    uint8_t showExit;         // +0x20 ("PHO_EXIT" shown)
    uint8_t previewShown;     // +0x21 the last shot shown in the frame
    uint8_t unknown22[2];
    uint32_t writeUntil;      // +0x24 a time (the "writing" message's)
    uint8_t active;           // +0x28
    uint8_t unknown29[3];
    int32_t state;            // +0x2C 0 aiming, 1 shot taken, 2 save menu, 3..8 saving
    uint32_t shotTime;        // +0x30
    uint8_t good;             // +0x34 the shot was of the target
    uint8_t judged;           // +0x35 (a target was checked)
    uint8_t take;             // +0x36 a shot asked for
    uint8_t canSave;          // +0x37
    uint8_t unknown38;
    uint8_t onTarget;         // +0x39
    uint8_t unknown3A[2];
    float zoom;               // +0x3C 0..1
    uint8_t unknown40;        // +0x40
    uint8_t unknown41[3];
    uint32_t targetTime;      // +0x44 when the target was last in the frame
    int32_t unknown48;        // +0x48
    uint32_t menuTime;        // +0x4C
    uint8_t autoClose;        // +0x50
    uint8_t unknown51[3];
    uint32_t closeTime;       // +0x54 (the other timer, 0xA14A50)
    uint32_t saveText;        // +0x58 text hashes: the menu's two lines
    uint32_t cancelText;      // +0x5C
    int32_t menuItem;         // +0x60 0 save, 1 cancel
    int32_t unknown64;        // +0x64
    int32_t formatFrames;     // +0x68
    uint8_t unknown6C[0x10];
    uint32_t resultText;      // +0x7C a hash (0: "good"/"bad")
    uint32_t resultParam;     // +0x80
    uint8_t frontendText;     // +0x84 the Frontend texts loaded
    uint8_t unknown85[3];
};
static_assert(offsetof(Photo, state) == 0x2C);
static_assert(offsetof(Photo, zoom) == 0x3C);
static_assert(offsetof(Photo, menuItem) == 0x60);
static_assert(offsetof(Photo, frontendText) == 0x84);
static_assert(sizeof(Photo) == sizeof(HUDPhoto));

// A photo loaded as a texture
struct PhotoSlot {
    re::Ptr32<void> raster;             // +0 (the RW emulation's)
    re::Ptr32<void> texture;            // +4 its texture
};
struct PhotoThumb {           // (0xC)
    uint8_t loaded;           // +0
    uint8_t unknown01[3];
    re::Ptr32<void> raster;             // +4
    re::Ptr32<void> texture;            // +8
};
static_assert(sizeof(PhotoThumb) == 0xC);

struct PhotoAlbum {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x00 0x919BDC
    int32_t count;            // +0x04 photos on the device
    int32_t selected;         // +0x08
    PhotoSlot big;            // +0x0C the selected one
    int32_t firstThumb;       // +0x14
    uint8_t thumbsLoaded;     // +0x18
    uint8_t unknown19[3];
    int32_t numThumbs;        // +0x1C
    PhotoThumb thumbs[5];     // +0x20
    uint8_t unknown5C[4];
    int32_t newPhotos;        // +0x60 taken since the album was last looked at (at most 15)
};
static_assert(offsetof(PhotoAlbum, thumbs) == 0x20);
static_assert(offsetof(PhotoAlbum, newPhotos) == 0x60);
int32_t& NewPhotos(PhotoAlbum* album) {
    return album->newPhotos;
}

// COM (D3D9) through the vtable, stdcall with `this` first
template <typename R, typename... A>
R Com(void* object, int slot, A... args) {
    return re::ComCall<R>(object, slot, args...);
}
enum {
    COM_RELEASE = 2,
    TEX_GET_SURFACE_LEVEL = 18,
    TEX_LOCK_RECT = 19,
    TEX_UNLOCK_RECT = 20,
};
struct LockedRect {
    int32_t pitch;
    uint8_t* bits;
};

GAME_FN(long, __stdcall, D3DXCreateTexture, 0x0085AC78, void* device, unsigned width, unsigned height,
        unsigned levels, uint32_t usage, int format, int pool, void** texture);
GAME_FN(long, __stdcall, D3DXLoadSurfaceFromSurface, 0x0085AC7E, void* dest, const void* destPalette,
        const void* destRect, void* src, const void* srcPalette, const void* srcRect, uint32_t filter,
        uint32_t colorKey);
GAME_FN(void, __cdecl, JpegEncode, 0x005865A0, const uint8_t* pixels, int width, int height, int pitch,
        uint8_t* out, uint32_t* size);
GAME_FN(uint8_t, __cdecl, JpegDecode, 0x00586710, uint8_t* pixels, int width, int height, int pitch,
        const uint8_t* jpeg);
// the save device's photos
GAME_FN(uint8_t, __cdecl, Photos_GetCount, 0x00736FF0, int32_t* count, int flag);
GAME_FN(uint8_t, __cdecl, Photos_Delete, 0x00736000, int index);
GAME_FN(uint8_t, __cdecl, Photos_Write, 0x00735C60, const uint8_t* jpeg, uint32_t size);
GAME_FN(uint8_t, __cdecl, Photos_Read, 0x00735EB0, int index, uint8_t* jpeg);
GAME_FN(void, __cdecl, Save_Fn7351C0, 0x007351C0);
GAME_FN(int, __cdecl, PhotoAlbum_GetState, 0x00735370);
GAME_FN(const uint16_t*, __fastcall, Save_GetMessage, 0x007354E0, const char* fallback);   // (fallback in ecx: the key when the state is past 3)
GAME_FN(uint8_t, __cdecl, Save_Fn7368C0, 0x007368C0, int);
GAME_FN(uint8_t, __cdecl, Save_Fn736E70, 0x00736E70, int);
GAME_FN(uint32_t, __cdecl, Save_GetPhotoSpace, 0x007353D0);
GAME_FN(uint8_t, __thiscall, Save_HasSpace, 0x008A0200, void* save, uint32_t size, uint32_t* space, int);
GAME_FN(uint8_t, __thiscall, Save_Fn736480, 0x00736480, void* save, int);
GAME_FN(void, __thiscall, Save_Fn7364A0, 0x007364A0, void* save, int);
GAME_FN(void, __thiscall, Save_Format, 0x00735BA0, void* save);
// the rest
GAME_FN(void, __cdecl, PhotoFilter_Set, 0x00560A30, const CRGBA* colour);
GAME_FN(void, __cdecl, PhotoFilter_SetRect, 0x0055FED0, const float* uvRect);
GAME_FN(void, __thiscall, Text_UnloadMission, 0x0068F550, void* text);
GAME_FN(void, __thiscall, Text_LoadMission, 0x0068FDA0, void* text, const char* name);
GAME_FN(void, __cdecl, PF_SetPrompt, 0x005143D0, const char* text);
GAME_FN(void, __thiscall, HUDText_ClearAll, 0x0055B180, void* text);
GAME_FN(void, __cdecl, PushStreamingMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopMemoryTag, 0x0052E710);
GAME_FN(void, __cdecl, Fn_45B800, 0x0045B800);
GAME_FN(void, __cdecl, CTimer_Update, 0x0045B960, int);
GAME_FN(void, __cdecl, CTimer_StartUserPause, 0x0045B810);
GAME_FN(void, __cdecl, CTimer_EndUserPause, 0x0045B820);
GAME_FN(uint32_t, __cdecl, Timer_GetCurrentTime, 0x0045BCA0);
GAME_FN(void*, __cdecl, RwRasterCreate, 0x00519F60, int width, int height, int depth, int flags);
GAME_FN(void, __cdecl, Texture_Fn6CA7D0, 0x006CA7D0, void* texture, int);
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, void* object);
GAME_FN(void, __cdecl, CStats_IncrementStat, 0x00458480, int stat, int amount);
GAME_FN(void*, __thiscall, CTargetingSystem_ClassifyTarget, 0x004D5AE0, void* system, uint8_t* entity);
GAME_FN(float, __thiscall, CTargetingSystem_ScoreEntity, 0x004D7AF0, void* system, void* kind, uint8_t* entity, int);
GAME_FN(uint8_t, __thiscall, CTargetingSystem_IsEntityVisible, 0x004D6E20, void* system, uint8_t* entity, int);
GAME_FN(CVector*, __cdecl, CVector_Add, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(uint8_t, __cdecl, CWorld_GetIsLineOfSightClear, 0x004617D0, const CVector* from, const CVector* to, int flags);
GAME_FN(uint8_t, __cdecl, Pad_ButtonJustUp, 0x00738940, int button, int pad);
GAME_FN(uint8_t, __thiscall, MenuInput_AcceptDown, 0x0050E180, void* input, int);
GAME_FN(void, __cdecl, Fn_7379A0, 0x007379A0);
GAME_FN(void, __cdecl, HelpText_Show, 0x00514420, const char* key, float time);
GAME_FN(void, __cdecl, HelpText_Show2, 0x00514470, const char* key, float time);
GAME_FN(void, __cdecl, ShowPrompt, 0x0068F580, const char* key, float scale);
GAME_FN(void, __cdecl, DrawPrompt2, 0x0068F5C0, const char* key, float scale);
GAME_FN(void, __cdecl, DrawPrompt3, 0x0068F600, const char* key, float scale);
GAME_FN(void, __cdecl, Frontend_Fn6A2360, 0x006A2360);
GAME_FN(void, __cdecl, PrintText, 0x006A5450, int x, int y, const uint16_t* text, int, int, int, int);
GAME_FN(void, __cdecl, PrintTextKey, 0x006A6380, int x, int y, const char* key, int, int, int, int);
GAME_FN(uint8_t, __cdecl, FE_PadPressed, 0x006A3170, int);
GAME_FN(void, __cdecl, FE_PadClear, 0x006A3120, int);
GAME_FN(void, __cdecl, Camera_Fn6C86C0, 0x006C86C0, void* camera);
GAME_FN(void, __cdecl, Camera_Fn6C8350, 0x006C8350, void* camera, int, int);
GAME_FN(void, __cdecl, Camera_BeginUpdate, 0x006C86A0, void* camera);
GAME_FN(void, __cdecl, App_WaitForChange, 0x0043A990);
GAME_FN(void, __thiscall, CCamera_Fn4F2390, 0x004F2390, void* camera, int);
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(col::CRect*, __cdecl, ScreenRect, 0x0052D650, col::CRect* out, float x, float y, float width, float height,
        int hAlign, int vAlign);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const col::CRect* rect, const CRGBA* colour);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, Draw2DPolygon, 0x0052C270, float x1, float y1, float x2, float y2, float x3, float y3, float x4,
        float y4, const CRGBA* colour);
GAME_FN(void, __cdecl, DrawLineStrip, 0x0073A410, int count, const CVector2D* points);
GAME_FN(void, __cdecl, DrawTexturedRect, 0x00739B90, int, int, const float* uvRect, void* d3dTexture);
GAME_FN(float, __cdecl, AlignX, 0x0052C690, int which);
GAME_FN(float, __cdecl, AlignY, 0x0052C6B0, int which);
GAME_FN(void*, __cdecl, GetButtonTexture, 0x00514EC0, int button);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(const uint16_t*, __thiscall, Text_GetByHash, 0x0068FF10, void* text, uint32_t hash);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_SetColour, 0x00424480, const CRGBA* colour);
GAME_FN(void, __cdecl, Font_SetDropColour, 0x0043B110, const CRGBA* colour);
GAME_FN(void, __cdecl, Font_AddString, 0x006880F0, const uint16_t* string);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
// its own, where modes apply
GAME_FN(void, __cdecl, Photo_CreateTexture, 0x00543F60);
GAME_FN(uint8_t, __cdecl, Photo_Save, 0x00543FC0, void* texture);
GAME_FN(uint8_t, __cdecl, Photo_Load, 0x00544180, int index, void* texture);
GAME_FN(uint8_t, __thiscall, Photo_IsInShot, 0x005442E0, Photo* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, PhotoSlot_Load, 0x00544400, PhotoSlot* self, int index);
GAME_FN(uint8_t, __thiscall, PhotoAlbum_Add, 0x005444B0, PhotoAlbum* self, void* texture);
GAME_FN(void, __thiscall, Photo_DrawIris, 0x00544550, Photo* self, float t);
GAME_FN(void, __thiscall, Photo_DrawMenu, 0x00544760, Photo* self);
GAME_FN(CRGBA*, __thiscall, Photo_FrameColour, 0x00545110, Photo* self, CRGBA* out, int flash, int good);
GAME_FN(void, __thiscall, PhotoThumb_Clear, 0x005452B0, PhotoThumb* self);
GAME_FN(void, __thiscall, Photo_CountSubjects, 0x005452F0, Photo* self);
GAME_FN(void, __thiscall, PhotoAlbum_ClearThumbs, 0x00546200, PhotoAlbum* self);
GAME_FN(void, __thiscall, PhotoAlbum_Destruct, 0x005462A0, PhotoAlbum* self);
GAME_FN(void, __cdecl, Photo_SetFilter, 0x00543EB0);

GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint32_t, g_otherTimer, 0x00A14A50);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(uint16_t, g_scanCode, 0x00C1AE88);
GAME_VAR(void*, g_camera, 0x00C3CCF4);
GAME_VAR(uint8_t*, g_viewport, 0x020C8904);            // (+0x1C..0x28 its rect, +0x44/+0x48 a text position)
GAME_VAR(void*, g_photoTexture, 0x00CF01B8);           // IDirect3DTexture9, 256x256 (system memory)
GAME_VAR(uint8_t, g_photoTestFile, 0x00CF01B4);        // (debug: the JPEG written to PhotoTest.jpg too)
GAME_VAR(uint8_t*, g_renderer, 0x020E2380);            // (+0xA4 the D3D device)
GAME_VAR(uint32_t, g_dx9TextureDataBit, 0x020E2250);   // NiDX9TextureData's RTTI bit
GAME_VAR(uint32_t, g_renderedTextureBit, 0x020E24E8);
GAME_VAR(uint8_t*, g_save, 0x020CE75C);                // (+0x25C, +0x264, +0x274)
GAME_ARRAY(uint8_t, g_jpeg, 0x00BD7800);               // [0x1C000]
GAME_VAR(int32_t, g_filterSet, 0x00CF108C);
GAME_VAR(uint32_t, g_filterColour, 0x00AC7638);
GAME_VAR(uint8_t, g_filterOff, 0x00C1B171);
GAME_VAR(uint8_t, g_canSave, 0x00C1B172);
GAME_VAR(uint8_t, g_canSaveMore, 0x00C1B173);
GAME_VAR(int32_t, g_photoLimit, 0x00AB46E4);
GAME_VAR(int32_t, g_photoLimit2, 0x00AB46E8);
GAME_VAR(int32_t, g_albumState, 0x00AB4678);
GAME_VAR(void*, g_shotTexture, 0x00CF1308);            // what the camera rendered
GAME_VAR(int32_t, g_numPeds, 0x00C13F00);
GAME_ARRAY(uint8_t*, g_peds, 0x00C11FC0);
GAME_VAR(int32_t, g_irisOffset, 0x00AB4704);           // (23: degrees)
GAME_VAR(int32_t, g_irisMax, 0x00AB4708);
GAME_VAR(int32_t, g_irisBlades, 0x00AB470C);           // (6)
GAME_VAR(void*, g_hudText, 0x00C67548);
GAME_VAR(uint8_t, g_frontendBusy1, 0x020C32E9);
GAME_VAR(uint8_t, g_frontendBusy2, 0x020C32EA);
GAME_VAR(uint8_t, g_padUp, 0x020C3278);
GAME_VAR(uint8_t, g_padDown, 0x020C3274);
GAME_VAR(void*, g_cameraObject, 0x00C1AE7C);
GAME_VAR(uint32_t, g_lineColour, 0x020DCCCC);          // DrawLineStrip's
GAME_VAR(void*, g_tex_C66FDC, 0x00C66FDC);             // the cross-hair
GAME_VAR(void*, g_tex_C66FE0, 0x00C66FE0);
GAME_VAR(void*, g_tex_C66FE4, 0x00C66FE4);
GAME_VAR(void*, g_tex_C66FE8, 0x00C66FE8);             // the zoom ladder's step, lit
GAME_VAR(void*, g_tex_C66FEC, 0x00C66FEC);
GAME_VAR(float, g_fontWrapCentre, 0x0208B778);
GAME_VAR(int16_t, g_fontEdge, 0x0208B784);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const kMenuInput = reinterpret_cast<void*>(0x00A45A30);
inline void* const kTheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline CRGBA* const kMenuDropColour = reinterpret_cast<CRGBA*>(0x020C331C);
inline PhotoAlbum* const kAlbum = reinterpret_cast<PhotoAlbum*>(0x00AB4688);
GAME_VAR(int32_t, g_cameraMode, 0x00C3CCF8);

// the file imports (read at call time)
using CreateFileA_t = HANDLE(WINAPI*)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
using WriteFile_t = BOOL(WINAPI*)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
using CloseHandle_t = BOOL(WINAPI*)(HANDLE);
GAME_VAR(CreateFileA_t, Import_CreateFileA, 0x008FC064);
GAME_VAR(WriteFile_t, Import_WriteFile, 0x008FC070);
GAME_VAR(CloseHandle_t, Import_CloseHandle, 0x008FC050);

// NiObject::IsKindOf by the class's bit (vtable[2]: GetRTTI)
uint8_t* IsKindOfBit(void* object, uint32_t bit) {
    re::Ptr32<void>* vt = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    uint8_t* rtti = re::PtrCast<uint8_t*(__thiscall*)(void*)>(vt[2])(object);
    uint32_t bits = *reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4);
    return (bits & (1u << (bit & 0x1F))) ? static_cast<uint8_t*>(object) : nullptr;
}

// a texture's D3D texture (its renderer data, +0x24, when it's NiDX9TextureData's;
// nullptr's +0x5C read otherwise, as the game does)
void* D3DTexture(void* texture) {
    void* data = *reinterpret_cast<re::Ptr32<void>*>(static_cast<uint8_t*>(texture) + 0x24);
    uint8_t* d = data ? IsKindOfBit(data, g_dx9TextureDataBit) : nullptr;
    return *reinterpret_cast<re::Ptr32<void>*>(d + 0x5C);
}

// the viewport's size in whole pixels
float ViewWidth() {
    return float(*reinterpret_cast<int32_t*>(g_viewport + 0x24) - *reinterpret_cast<int32_t*>(g_viewport + 0x1C));
}
float ViewHeight() {
    return float(*reinterpret_cast<int32_t*>(g_viewport + 0x28) - *reinterpret_cast<int32_t*>(g_viewport + 0x20));
}

// ---- the photo itself --------------------------------------------------------------------

// 0x543D30: the viewport over 640x480
void __cdecl Photo_GetViewScale(float* x, float* y) {
    *x = ViewWidth() / 640.0f;
    *y = ViewHeight() / 480.0f;
}

// 0x543DE0: the album's count from the device
uint8_t __fastcall PhotoAlbum_RefreshCount(PhotoAlbum* self, re::Edx, int flag) {
    int32_t count = flag;
    if (!Photos_GetCount(&count, flag)) {
        return 0;
    }
    self->count = count;
    return 1;
}

// 0x543E10: a photo deleted from the device (the selection kept in range)
uint8_t __fastcall PhotoAlbum_Delete(PhotoAlbum* self, re::Edx, int index) {
    if (!Photos_Delete(index)) {
        return 0;
    }
    self->count--;
    float fresh = float(NewPhotos(self) - 1);
    if (fresh < 0.0f) {
        fresh = 0.0f;
    }
    NewPhotos(self) = int(fresh);
    int count = self->count;
    if (index > self->selected) {
        return 1;
    }
    if (index < self->selected) {
        self->selected--;
        return 1;
    }
    if (self->selected >= count) {
        self->selected = count > 0 ? count - 1 : 0;
    }
    return 1;
}

// 0x543EB0: the camera's colour filter on (white), once
void __cdecl Photo_SetFilter_() {
    if (g_filterOff) {
        g_filterSet = 0;
        return;
    }
    if (g_filterSet == 0) {
        CRGBA white = {0xFF, 0xFF, 0xFF, 0xFF};
        g_filterSet = 1;
        std::memcpy(&g_filterColour, &white, 4);
        PhotoFilter_Set(&white);
    }
}

// 0x543F00: the camera put away
void __fastcall Photo_Close(Photo* self, re::Edx) {
    if (self->active) {
        if (self->frontendText) {
            Text_UnloadMission(kTheText);
            self->frontendText = 0;
        }
        PF_SetPrompt(nullptr);
        HUDText_ClearAll(g_hudText);
        self->state = 0;
        self->active = 0;
    }
    if (self->previewShown) {
        self->previewShown = 0;
        self->shotTime = 0;
    }
    g_filterSet = 0;
}

// 0x543F60: the 256x256 system-memory texture photos go through
void __cdecl Photo_CreateTexture_() {
    if (!g_photoTexture) {
        void* device = *reinterpret_cast<re::Ptr32<void>*>(g_renderer + 0xA4);
        D3DXCreateTexture(device, 0x100, 0x100, 1, 0x200, 0x15, 2, re::OutPtr(&g_photoTexture));
    }
}

// 0x543FA0
void __cdecl Photo_ReleaseTexture() {
    if (g_photoTexture) {
        Com<unsigned long>(g_photoTexture, COM_RELEASE);
        g_photoTexture = nullptr;
    }
}

// 0x543FC0: a rendered texture saved to the device as a JPEG
uint8_t __cdecl Photo_Save_(void* texture) {
    if (!texture) {
        return 0;
    }
    reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture))[0x50 / 4]))(texture);
    uint8_t* save = g_save;
    void* d3dTexture = D3DTexture(texture);
    void* photo = g_photoTexture;
    if (!photo) {
        return 0;
    }
    void* src = nullptr;
    void* dst = nullptr;
    Com<long>(d3dTexture, TEX_GET_SURFACE_LEVEL, 0u, &src);
    Com<long>(photo, TEX_GET_SURFACE_LEVEL, 0u, &dst);
    long hr = D3DXLoadSurfaceFromSurface(dst, nullptr, nullptr, src, nullptr, nullptr, 3, 0);
    Com<unsigned long>(src, COM_RELEASE);
    Com<unsigned long>(dst, COM_RELEASE);
    if (hr < 0) {
        return 0;
    }
    LockedRect locked;
    Com<long>(photo, TEX_LOCK_RECT, 0u, &locked, nullptr, 0u);
    // BGRA to RGBA
    int n = (locked.pitch * 0x100) / 4;
    for (uint8_t* p = locked.bits; n > 0; n--, p += 4) {
        uint8_t b = p[0];
        p[0] = p[2];
        p[2] = b;
    }
    uint32_t size = 0x1C000;
    JpegEncode(locked.bits, 0x100, 0x100, locked.pitch, g_jpeg, &size);
    Com<long>(photo, TEX_UNLOCK_RECT, 0u);
    *reinterpret_cast<int32_t*>(save + 0x264) = 1;
    uint8_t ok = Photos_Write(g_jpeg, size);
    if (g_photoTestFile) {
        HANDLE file = Import_CreateFileA("PhotoTest.jpg", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            DWORD written;
            Import_WriteFile(file, g_jpeg, size, &written, nullptr);
            Import_CloseHandle(file);
        }
    }
    return ok;
}

// 0x544180: a photo from the device into a texture (a failed decode leaves the photo
// texture locked, as the game does)
uint8_t __cdecl Photo_Load_(int index, void* texture) {
    void* photo = g_photoTexture;
    if (!photo) {
        return 0;
    }
    if (!g_save) {
        Save_Fn7351C0();
    }
    if (!Photos_Read(index, g_jpeg)) {
        return 0;
    }
    LockedRect locked;
    Com<long>(photo, TEX_LOCK_RECT, 0u, &locked, nullptr, 0u);
    if (!JpegDecode(locked.bits, 0x100, 0x100, locked.pitch, g_jpeg)) {
        return 0;
    }
    // RGB to BGRA, opaque
    int n = (locked.pitch * 0x100) / 4;
    for (uint8_t* p = locked.bits; n > 0; n--, p += 4) {
        uint8_t r = p[2];
        uint8_t b = p[0];
        p[3] = 0xFF;
        p[2] = b;
        p[0] = r;
    }
    Com<long>(photo, TEX_UNLOCK_RECT, 0u);
    void* d3dTexture = D3DTexture(texture);
    void* src = nullptr;
    void* dst = nullptr;
    Com<long>(photo, TEX_GET_SURFACE_LEVEL, 0u, &src);
    Com<long>(d3dTexture, TEX_GET_SURFACE_LEVEL, 0u, &dst);
    long hr = D3DXLoadSurfaceFromSurface(dst, nullptr, nullptr, src, nullptr, nullptr, 3, 0);
    Com<unsigned long>(src, COM_RELEASE);
    Com<unsigned long>(dst, COM_RELEASE);
    return hr >= 0;
}

// 0x5442E0: a ped in the shot: targetable, on screen and in sight of the camera
// (ecx unused)
uint8_t __fastcall Photo_IsInShot_(Photo*, re::Edx, uint8_t* ped) {
    void* targeting = *reinterpret_cast<re::Ptr32<void>*>(g_playerPed + 0x1C8);
    void* kind = CTargetingSystem_ClassifyTarget(targeting, ped);
    float score = CTargetingSystem_ScoreEntity(*reinterpret_cast<re::Ptr32<void>*>(g_playerPed + 0x1C8), kind, ped, 1);
    if (!CTargetingSystem_IsEntityVisible(*reinterpret_cast<re::Ptr32<void>*>(g_playerPed + 0x1C8), ped, 0)) {
        return 0;
    }
    if (-99999997952.0f == score || !kind) {
        return 0;
    }
    *reinterpret_cast<uint16_t*>(g_playerPed + 0x10A) = uint16_t(g_scanCode + 1);
    *reinterpret_cast<uint16_t*>(ped + 0x10A) = uint16_t(g_scanCode + 1);
    float height = reinterpret_cast<float(__thiscall*)(uint8_t*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0x6C / 4]))(ped);
    CVector up = {0.0f, 0.0f, float(double(height) * 0.5)};
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x14);
    const CVector* pos = reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : ped + 4);
    CVector point;
    CVector_Add(&point, pos, &up);
    uint8_t* cam = GetLocalTransform(GetCameraNode(g_camera));
    if (!CWorld_GetIsLineOfSightClear(reinterpret_cast<const CVector*>(cam + 0x24), &point, 0x4B)) {
        return 0;
    }
    return 1;
}

// 0x544400: a photo loaded into a slot (a new 256x256 raster)
uint8_t __fastcall PhotoSlot_Load_(PhotoSlot* self, re::Edx, int index) {
    PushStreamingMemoryTag();
    Fn_45B800();
    if (self->raster) {
        self->raster = nullptr;
    }
    if (self->texture) {
        Texture_Fn6CA7D0(self->texture, 0);
        NiRelease(self->texture);
        self->texture = nullptr;
    }
    self->raster = RwRasterCreate(0x100, 0x100, 0, 0x504);
    self->texture = *reinterpret_cast<re::Ptr32<void>*>(static_cast<uint8_t*>(self->raster) + 0x34);
    uint8_t ok = Photo_Load(index, self->texture);
    CTimer_Update(0);
    PopMemoryTag();
    return ok;
}

// 0x5444B0: a rendered texture added to the album
uint8_t __fastcall PhotoAlbum_Add_(PhotoAlbum* self, re::Edx, void* texture) {
    if (!texture) {
        return 0;
    }
    uint8_t* t = IsKindOfBit(texture, g_renderedTextureBit);
    if (!t) {
        return 0;
    }
    uint8_t ok = Photo_Save(t);
    if (ok) {
        self->count++;
        float fresh = float(NewPhotos(self) + 1);
        if (!(fresh <= 15.0f)) {
            fresh = 15.0f;
        }
        NewPhotos(self) = int(fresh);
    }
    return ok;
}

// 0x544550: the iris closing (t: 1 open), a blade per sixth of the turn. (ecx unused)
void __fastcall Photo_DrawIris_(Photo*, re::Edx, float t) {
    if (t == 1.0f) {
        return;
    }
    float radius = t * 560.0f + (1.0f - t) * 0.0f;
    CRGBA black;
    RGBA_Construct(&black, 0, 0, 0, 0xFF);
    constexpr float kPi = 3.14159274f;
    for (int i = 0; i < g_irisBlades; i++) {
        if (i > g_irisMax) {
            continue;
        }
        int n = g_irisBlades;
        float a0 = float(i * 360 / n + g_irisOffset) * kPi / 180.0f;
        float a1 = float((i * 360 + 360) / n + g_irisOffset) * kPi / 180.0f;
        float s0 = float(std::sin(double(a0)));
        float c0 = float(std::cos(double(a0)));
        float s1 = float(std::sin(double(a1)));
        float c1 = float(std::cos(double(a1)));
        Draw2DPolygon(s0 * radius + 320.0f, c0 * radius + 240.0f, s0 * 560.0f + 320.0f, c0 * 560.0f + 240.0f,
                      s1 * radius + 320.0f, c1 * radius + 240.0f, s1 * 560.0f + 320.0f, c1 * 560.0f + 240.0f, &black);
    }
}

// what DrawMenu's exits share: the camera closed, the save state reset
void MenuExit(Photo* self) {
    self->autoClose = 0;
    self->state = 0;
    CTimer_EndUserPause();
    *reinterpret_cast<int32_t*>(g_save + 0x264) = 6;
    CCamera_Fn4F2390(kTheCamera, 1);
    if (self->previewShown) {
        self->shotTime = 0;
        self->previewShown = 0;
    }
    Frontend_Fn6A2360();
}

void WritingMessage(const char* key) {
    g_fontWrapCentre = 515.0f;
    PrintTextKey(0x140, 0x6E, key, 3, 2, 3, 0);
}

// 0x544760: the prompts and menus by the state (aiming, shot taken, the save menu and
// the memory card's messages)
void __fastcall Photo_DrawMenu_(Photo* self, re::Edx) {
    if (g_frontendBusy1 == 1 || g_frontendBusy2 == 1) {
        return;
    }
    uint8_t cancel = Pad_ButtonJustUp(8, 0);
    uint8_t accept = MenuInput_AcceptDown(kMenuInput, 0);
    uint8_t timeout = (self->autoClose == 1 && self->closeTime <= g_otherTimer) ? 1 : 0;
    uint8_t* player = g_playerPed;
    if (g_canSave) {
        self->canSave = 1;
        g_photoLimit = g_canSaveMore ? 15 : 5;
    } else {
        self->canSave = 0;
        g_photoLimit = 0;
    }
    if (self->state != 0) {
        CTimer_StartUserPause();
        PF_SetPrompt(nullptr);
    }
    if (self->state > 1) {
        col::CRect r;
        ScreenRect(&r, 0.0f, 0.0f, ViewWidth(), ViewHeight(), 1, 1);
        CRGBA shade;
        RGBA_Construct(&shade, 0, 0, 0, 0x80);
        DrawRect(&r, &shade);
    }
    Fn_7379A0();
    uint8_t* save;
    switch (self->state) {
    case 0:
        if (*reinterpret_cast<int32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(player + 0x1D0) + 0x26C) == 0) {
            HelpText_Show("PHO_PICT", 0.001f);
            if (self->showExit) {
                HelpText_Show2("PHO_EXIT", 0.001f);
            }
        }
        return;
    case 1: {
        Font_Begin();
        CRGBA grey;
        RGBA_Construct(&grey, 0xD7, 0xD7, 0xD7, 0xFF);
        Font_SetColour(&grey);
        Font_SetSizePreset(1);
        if (self->canSave) {
            ShowPrompt("PHO_SAVE", 1.0f);
        }
        DrawPrompt2("PHO_BACK", 1.0f);
        Font_Render();
        if (self->judged) {
            Font_SetJustify(2);
            Font_SetSizePreset(1);
            if (self->resultText) {
                Font_AddString(Text_GetByHash(kTheText, self->resultParam));
                uint16_t text[0x80];
                Font_FormatText(text, Text_GetByHash(kTheText, self->resultText), 0x80);
                Font_PrintString(float(*reinterpret_cast<int32_t*>(g_viewport + 0x44)),
                                 float(*reinterpret_cast<int32_t*>(g_viewport + 0x48)), text, 1);
            } else {
                int y = *reinterpret_cast<int32_t*>(g_viewport + 0x48);
                int x = *reinterpret_cast<int32_t*>(g_viewport + 0x44);
                const uint16_t* text = Text_Get(kTheText, self->good ? "PHO_GOOD" : "PHO_BAD");
                Font_PrintString(float(x), float(y), text, 1);
            }
        }
        if (cancel == 1 || timeout == 1) {
            MenuExit(self);
            return;
        }
        if (accept != 1 || self->canSave != 1) {
            return;
        }
        if (!self->frontendText) {
            Text_LoadMission(kTheText, "Frontend");
            self->frontendText = 1;
        }
        self->autoClose = 0;
        self->menuTime = g_timeInMilliseconds;
        self->state = 2;
        self->menuItem = 1;
        Frontend_Fn6A2360();
        return;
    }
    case 2: {
        g_fontWrapCentre = 515.0f;
        save = g_save;
        int albumState = PhotoAlbum_GetState();
        if (g_albumState != albumState) {
            self->menuItem = 1;
            g_albumState = albumState;
        }
        bool retry;
        if (albumState == 1) {
            uint32_t space;
            uint8_t ok = Save_HasSpace(save, 0x14000, &space, 0);
            if (g_photoLimit2 == g_photoLimit) {
                Font_AddNumber(float(space));
                PrintTextKey(0x140, 0x6E, "ALBUM_FULL", 3, 2, 3, 0);
                DrawPrompt3("FE_CONTINUE", 1.0f);
                if (accept != 1) {
                    return;
                }
                MenuExit(self);
                return;
            }
            if (ok) {
                Font_SetDropColour(kMenuDropColour);
                g_fontEdge = 1;
                PrintTextKey(0x140, 0x32, "PHOTO_SAV", 3, 2, 1, 1);
                if (accept == 1 && self->menuItem == 0) {
                    self->state = 3;
                } else if (cancel == 1 || (self->menuItem == 1 && accept == 1)) {
                    self->state = 1;
                    self->autoClose = 0;
                }
                goto menu;
            }
            if (space < Save_GetPhotoSpace()) {
                Font_AddNumber(float(*reinterpret_cast<int32_t*>(save + 0x25C)));
                PrintTextKey(0x140, 0x6E, "PHOTO_FULL_P", 3, 2, 1, 1);
            } else {
                Font_AddNumber(float(space));
                PrintTextKey(0x140, 0x6E, "PHOTO_FULL", 3, 2, 1, 1);
            }
            PrintTextKey(0x140, 0xBE, "RETRY_Q", 3, 2, 1, 1);
        } else if (albumState == 5) {
            PrintText(0x140, 0x6E, Save_GetMessage(nullptr), 3, 2, 1, 1);   // (the game's ecx is left over)
            PrintTextKey(0x140, 0xBE, "RETRY_Q", 3, 2, 1, 1);
            if (Save_Fn7368C0(0)) {
                *reinterpret_cast<int32_t*>(g_save + 0x264) = 6;
                return;
            }
        } else {
            return;
        }
        retry = self->menuItem == 0 && accept == 1;
        if (cancel != 0 || (self->menuItem == 1 && accept == 1)) {
            self->state = 1;
            self->autoClose = 0;
            if (albumState == 5) {
                *reinterpret_cast<int32_t*>(g_save + 0x264) = 6;
            }
        } else if (retry) {
            if (albumState == 5) {
                if (!Save_Fn736E70(0)) {
                    *reinterpret_cast<int32_t*>(g_save + 0x264) = 6;
                }
            } else if (albumState == 1) {
                self->menuItem = 1;
            }
        }
    menu: {
        // the two lines, the chosen one lit (the last argument: the game passes the
        // dword the flag byte is in, its other bytes stale)
        float h = g_screenScaleY * 13.0f;
        int styleSave = 0xB, styleCancel = 0xB;
        int litSave = 0, litCancel = 0;
        float top = 240.0f - h;
        float bottom = h + 240.0f;
        if (self->menuItem == 0) {
            styleSave = 3;
            litSave = 1;
        } else if (self->menuItem == 1) {
            styleCancel = 3;
            litCancel = 1;
        }
        Font_SetDropColour(kMenuDropColour);
        g_fontEdge = 1;
        const uint16_t* text = Text_GetByHash(kTheText, self->saveText);
        PrintText(0x140, int(top), text, styleSave, 2, 3, litSave);
        text = Text_GetByHash(kTheText, self->cancelText);
        PrintText(0x140, int(bottom), text, styleCancel, 2, 3, litCancel);
        ShowPrompt("FEPA_CONFIRM", 1.0f);
        DrawPrompt2("FEPA_CANCEL", 1.0f);
        if (g_padUp && FE_PadPressed(0)) {
            self->menuItem--;
            if (self->menuItem < 0) {
                self->menuItem = 0;
            }
            FE_PadClear(0);
            return;
        }
        if (!g_padDown || !FE_PadPressed(0)) {
            return;
        }
        self->menuItem++;
        if (self->menuItem > 1) {
            self->menuItem = 1;
        }
        FE_PadClear(0);
        return;
    }
    }
    case 3:
        self->writeUntil = Timer_GetCurrentTime() + 3000;
        WritingMessage("FESZ_WR");
        self->state = 4;
        return;
    case 4:
        save = g_save;
        WritingMessage("FESZ_WR");
        Camera_Fn6C86C0(g_cameraObject);
        Camera_Fn6C8350(g_cameraObject, 0, 0);
        if (!Save_Fn736480(save, 0)) {
            Save_Fn7364A0(save, 0);
            App_WaitForChange();
            Fn_7379A0();
            Save_Fn7364A0(save, 0);
        }
        Camera_BeginUpdate(g_cameraObject);
        return;
    case 5:
        WritingMessage("FESZ_WR");
        if (Timer_GetCurrentTime() > self->writeUntil) {
            self->state = 6;
        }
        return;
    case 6:
        WritingMessage("SUCCES_SAVE");
        DrawPrompt3("FE_CONTINUE", 1.0f);
        if (accept != 1) {
            return;
        }
        MenuExit(self);
        return;
    case 7:
        WritingMessage("SUCCES_FORMAT");
        self->formatFrames--;
        if (self->formatFrames < 1) {
            Save_Format(g_save);
            self->state = 3;
            *reinterpret_cast<int32_t*>(g_save + 0x264) = 6;
            Frontend_Fn6A2360();
        }
        return;
    case 8:
        WritingMessage("ALBUM_FULL");
        DrawPrompt3("FE_CONTINUE", 1.0f);
        if (accept != 1) {
            return;
        }
        MenuExit(self);
        return;
    }
}

// 0x545110: the frame's colour: grey, or green/red while a target's just been judged.
// (ecx unused)
CRGBA* __fastcall Photo_FrameColour_(Photo*, re::Edx, CRGBA* out, int flash, int good) {
    if (!uint8_t(flash)) {
        RGBA_Construct(out, 0xD7, 0xD7, 0xD7, 0xFF);
        return out;
    }
    CRGBA c;
    if (uint8_t(good)) {
        RGBA_Construct(&c, 0x22, 0x5D, 0x42, 0xFF);
    } else {
        RGBA_Construct(&c, 0x73, 0x20, 0x20, 0xFF);
    }
    *out = c;
    return out;
}

// 0x545190: the shot saved to the album (state 4), then state 5 (saved), 8 (full) or 2
void __fastcall Photo_SaveToAlbum(Photo* self, re::Edx) {
    if (self->state != 4) {
        return;
    }
    uint8_t* save = g_save;
    bool full = true;
    int32_t count;
    uint8_t ok = Photos_GetCount(&count, 1) ? 1 : 0;
    if (ok) {
        kAlbum->count = count;
    }
    *reinterpret_cast<int32_t*>(save + 0x274) = 2;
    if (ok && kAlbum->count < 15) {
        full = false;
        Photo_CreateTexture();
        ok = PhotoAlbum_Add(kAlbum, g_shotTexture);
        if (g_photoTexture) {
            Com<unsigned long>(g_photoTexture, COM_RELEASE);
            g_photoTexture = nullptr;
        }
    }
    if (ok == 1) {
        self->state = full ? 8 : 5;
        return;
    }
    self->state = 2;
}

// 0x5452B0
void __fastcall PhotoThumb_Clear_(PhotoThumb* self, re::Edx) {
    if (!self->loaded) {
        return;
    }
    if (self->raster) {
        self->raster = nullptr;
    }
    if (self->texture) {
        Texture_Fn6CA7D0(self->texture, 0);
        NiRelease(self->texture);
        self->texture = nullptr;
    }
    self->loaded = 0;
}

// 0x5452F0: the photo's stats: one more photo, and one per kind of ped in it
void __fastcall Photo_CountSubjects_(Photo* self, re::Edx) {
    CStats_IncrementStat(0xB7, 1);
    for (int i = 0; i < g_numPeds; i++) {
        uint8_t* ped = g_peds[i];
        if (!ped || !Photo_IsInShot(self, ped) || (ped[0x108] & 7) != 3) {
            continue;
        }
        int stat;
        switch (*reinterpret_cast<int32_t*>(ped + 0x1310)) {
        case 8: stat = 0xBE; break;
        case 9: stat = 0xC0; break;
        case 1: stat = 0xB9; break;
        case 5: stat = 0xBA; break;
        case 4: stat = 0xBB; break;
        case 2: stat = 0xBC; break;
        case 3: stat = 0xBD; break;
        case 0: stat = 0xBF; break;
        case 0xB: stat = 0xC1; break;
        default: continue;
        }
        CStats_IncrementStat(stat, 1);
    }
}

// 0x5453F0: a photo selected, loaded big
uint8_t __fastcall PhotoAlbum_Select(PhotoAlbum* self, re::Edx, int index) {
    if (index >= self->count || index < 0) {
        return 0;
    }
    PhotoSlot* big = &self->big;
    if (big->raster) {
        big->raster = nullptr;
    }
    if (big->texture) {
        Texture_Fn6CA7D0(big->texture, 0);
        NiRelease(big->texture);
        big->texture = nullptr;
    }
    self->selected = index;
    return PhotoSlot_Load(big, index);
}

// 0x545450: the viewfinder (while aiming), the frame, the preview, the iris and the menu
void __fastcall Photo_DrawViewfinder(Photo* self, re::Edx, float iris) {
    int state12 = 0;
    *reinterpret_cast<uint32_t*>(g_playerPed + 0x1458) = (g_timeInMilliseconds - self->menuTime) > 1000;
    if (!self->active) {
        self->active = 1;
        self->unknown64 = 0;
    }
    int state8, state6;
    GetRenderState(8, &state8);
    GetRenderState(6, &state6);
    GetRenderState(0xC, &state12);
    if (self->state == 0) {
        Photo_SetFilter();
    }
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    col::CRect view;
    ScreenRect(&view, 0.0f, 0.0f, ViewWidth(), ViewHeight(), 1, 1);
    // the bars round it
    CRGBA bars;
    RGBA_Construct(&bars, 0x20, 0x20, 0x20, 0xFF);
    col::CRect bar = {0.0f, 480.0f, view.left, 0.0f};
    DrawRect(&bar, &bars);
    bar = {view.right, 480.0f, 640.0f, 0.0f};
    DrawRect(&bar, &bars);
    bar = {view.left, view.top, view.right, 0.0f};
    DrawRect(&bar, &bars);
    bar = {view.left, 480.0f, view.right, view.bottom};
    DrawRect(&bar, &bars);
    // the frame
    uint32_t since = g_timeInMilliseconds - self->targetTime;
    uint8_t good = since < 300 ? self->onTarget : 0;
    CRGBA frame;
    Photo_FrameColour(self, &frame, since < 300, good);
    frame.a = 0xFF;
    CVector2D outline[5] = {
        {view.left, view.top}, {view.left, view.bottom}, {view.right, view.bottom}, {view.right, view.top},
        {view.left, view.top},
    };
    std::memcpy(&g_lineColour, &frame, 4);
    DrawLineStrip(5, outline);
    if (self->state == 0) {
        SetRenderState(6, 0);
        SetRenderState(8, 0);
        SetRenderState(0xC, 1);
        SetRenderState(9, 1);
        SetRenderState(2, 3);
        float w64 = 64.0f * sx;
        float h32 = 32.0f * sy;
        float x0 = sx * 4.0f + view.left;
        float y0 = 4.0f * sy + view.top;
        CVector2D uv2 = {1.0f, 1.0f};
        CVector2D uv1 = {0.0f, 0.0f};
        float y1 = y0 + h32;
        float x1 = w64 * 0.5f + x0;
        CRGBA white;
        RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
        DrawRectUV(x0, y0, x1, y1, g_tex_C66FE0, &white, &uv1, &uv2);
        // the buttons at the ends of the zoom ladder, at their aspect
        auto buttonWidth = [&](void* texture) {
            re::Ptr32<void>* vt = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture);
            uint32_t w = re::PtrCast<uint32_t(__thiscall*)(void*)>(vt[0x48 / 4])(texture);
            float wide = float(w) * h32;
            uint32_t h = re::PtrCast<uint32_t(__thiscall*)(void*)>(vt[0x4C / 4])(texture);
            return float(double(wide) / double(float(h)));
        };
        void* button = GetButtonTexture(2);
        float bw = buttonWidth(button);
        RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
        DrawRectUV(x1, y0, float(double(bw) + double(x1)), y1, button, &white, &uv1, &uv2);
        float xa = view.left + sx * 4.0f;
        float yb = (view.bottom - h32) - (sy + sy);
        float yb2 = h32 + yb;
        float xb = xa + w64 * 0.5f;
        RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
        DrawRectUV(xa, yb, xb, yb2, g_tex_C66FE4, &white, &uv1, &uv2);
        button = GetButtonTexture(3);
        bw = buttonWidth(button);
        RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
        DrawRectUV(xb, yb, float(double(bw) + double(xb)), yb2, button, &white, &uv1, &uv2);
        // the ladder: 11 steps between them, the zoom's lit
        float w32 = 32.0f * sx;
        float h16 = 16.0f * sy;
        float xz = 16.0f * sx + view.left;
        float span = yb - y1;
        float step = span / 11.0f;
        float offset = ((step - h16) * 0.5f) * sy;
        float margin = (span - 11.0f * step) * 0.5f;
        float z = 1.0f - self->zoom;
        float sel = float(-1 - int(z * -11.0f));
        if (sel < 0.0f) {
            sel = 0.0f;
        } else if (11.0f < sel) {
            sel = 11.0f;
        }
        int selected = int(sel);
        float y = (y1 + margin) - sy * 3.0f;
        float xz2 = xz + w32;
        for (int i = 0; i < 0xB; i++) {
            void* texture = selected != i ? g_tex_C66FEC : g_tex_C66FE8;
            CVector2D u2 = {1.0f, 1.0f};
            CVector2D u1 = {0.0f, 0.0f};
            float top = y + offset;
            RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
            DrawRectUV(xz, top, xz2, h16 + top, texture, &white, &u1, &u2);
            y = y + step;
        }
        // the brackets either side of the centre
        CRGBA black;
        RGBA_Construct(&black, 0x10, 0x10, 0x10, 0xFF);
        float a115 = 115.0f * sx;
        float a130 = 130.0f * sx;
        float b65 = sy * 65.0f;
        float b63 = b65 - 3.0f;
        float a126 = a130 - 4.0f;
        float yA = b65 + 240.0f;
        float yB = -b65 + 240.0f;
        float yC = -b63 + 240.0f;
        float yD = b63 + 240.0f;
        float xA = a115 + 320.0f;
        float xB = a130 + 320.0f;
        float xC = a126 + 320.0f;
        CVector2D right[9] = {{xA, yA}, {xB, yA}, {xB, yB}, {xA, yB}, {xA, yC},
                              {xC, yC}, {xC, yD}, {xA, yD}, {xA, yA}};
        std::memcpy(&g_lineColour, &black, 4);
        DrawLineStrip(9, right);
        float xL = 320.0f - a115;
        float xL2 = 320.0f - a130;
        float xL5 = 320.0f - a126;
        CVector2D left[9] = {{xL, yA}, {xL2, yA}, {xL2, yB}, {xL, yB}, {xL, yC},
                             {xL5, yC}, {xL5, yD}, {xL, yD}, {xL, yA}};
        std::memcpy(&g_lineColour, &black, 4);
        DrawLineStrip(9, left);
        // the cross-hair
        float cx = AlignX(1);
        float cy = AlignY(1);
        float halfW = w64 * 0.5f;
        float halfH = sy * 64.0f * 0.5f;
        CVector2D c2 = {1.0f, 1.0f};
        CVector2D c1 = {0.0f, 0.0f};
        RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
        DrawRectUV(cx - halfW, cy - halfH, cx + halfW, cy + halfH, g_tex_C66FDC, &white, &c1, &c2);
    } else {
        self->shotTime = g_timeInMilliseconds;
    }
    SetRenderState(8, state8);
    SetRenderState(6, state6);
    SetRenderState(0xC, state12);
    if (self->previewShown) {
        if (self->state == 0) {
            self->previewShown = 0;
            self->shotTime = 0;
        }
        if (self->previewShown) {
            // the shot in the frame, bordered by the verdict
            float s5x = g_screenScaleX * 5.0f;
            float s5y = 5.0f * g_screenScaleY;
            col::CRect r;
            ScreenRect(&r, 0.0f, 0.0f, ViewWidth(), ViewHeight(), 1, 1);
            CRGBA colour;
            if (!self->judged) {
                RGBA_Construct(&colour, 0xD7, 0xD7, 0xD7, 0xFF);
            } else {
                CRGBA c;
                if (self->good) {
                    RGBA_Construct(&c, 0x22, 0x5D, 0x42, 0xFF);
                } else {
                    RGBA_Construct(&c, 0x73, 0x20, 0x20, 0xFF);
                }
                colour = c;
            }
            col::CRect border = {r.left - s5x, s5y + r.bottom, s5x + r.right, r.top - s5y};
            DrawRect(&border, &colour);
            void* texture = g_shotTexture;
            reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture))[0x50 / 4]))(texture);
            void* d3dTexture = D3DTexture(texture);
            float uv[4] = {r.left / 640.0f, r.top / 480.0f, r.right / 640.0f, r.bottom / 480.0f};
            DrawTexturedRect(0, 0, uv, d3dTexture);
        }
    }
    Font_Render();
    Photo_DrawIris(self, iris);
    Photo_DrawMenu(self);
}

// 0x546200: the thumbnails let go
void __fastcall PhotoAlbum_ClearThumbs_(PhotoAlbum* self, re::Edx) {
    for (int i = 0; i < self->numThumbs; i++) {
        PhotoThumb& t = self->thumbs[i];
        if (!t.loaded) {
            continue;
        }
        if (t.raster) {
            t.raster = nullptr;
        }
        if (t.texture) {
            Texture_Fn6CA7D0(t.texture, 0);
            NiRelease(t.texture);
            t.texture = nullptr;
        }
        t.loaded = 0;
    }
    self->numThumbs = 0;
    self->thumbsLoaded = 0;
}

// 0x546260: the big photo and the thumbnails let go
void __fastcall PhotoAlbum_Clear(PhotoAlbum* self, re::Edx) {
    if (self->big.raster) {
        self->big.raster = nullptr;
    }
    if (self->big.texture) {
        Texture_Fn6CA7D0(self->big.texture, 0);
        NiRelease(self->big.texture);
        self->big.texture = nullptr;
    }
    PhotoAlbum_ClearThumbs(self);
}

// 0x5462A0: the destructor (the thumbnails from the last)
void __fastcall PhotoAlbum_Destruct_(PhotoAlbum* self, re::Edx) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00919BDC);
    for (int i = 4; i >= 0; i--) {
        PhotoThumb& t = self->thumbs[i];
        if (!t.loaded) {
            continue;
        }
        if (t.raster) {
            t.raster = nullptr;
        }
        if (t.texture) {
            Texture_Fn6CA7D0(t.texture, 0);
            NiRelease(t.texture);
            t.texture = nullptr;
        }
        t.loaded = 0;
    }
    if (self->big.raster) {
        self->big.raster = nullptr;
    }
    if (self->big.texture) {
        Texture_Fn6CA7D0(self->big.texture, 0);
        NiRelease(self->big.texture);
        self->big.texture = nullptr;
    }
}

// 0x546320
PhotoAlbum* __fastcall PhotoAlbum_DeletingDestructor(PhotoAlbum* self, re::Edx, int flags) {
    PhotoAlbum_Destruct(self);
    if (flags & 1) {
        reinterpret_cast<void(__cdecl*)(void*)>(0x005EEAB0)(self);
    }
    return self;
}

// 0x546340: a shot taken (in first-person camera mode): judged, counted, previewed
void __fastcall Photo_TakePicture(Photo* self, re::Edx) {
    if (!self->take) {
        return;
    }
    if (g_cameraMode != 2) {
        self->state = 0;
        self->good = 0;
        self->menuItem = 1;
        self->take = 0;
        return;
    }
    uint32_t since = g_timeInMilliseconds - self->targetTime;
    self->good = since < 300 ? self->onTarget : 0;
    self->judged = (g_timeInMilliseconds - self->targetTime) < 300;
    self->shotTime = g_timeInMilliseconds;
    CTimer_StartUserPause();
    Photo_CountSubjects(self);
    Photo_SetFilter();
    if (self->previewShown) {
        self->previewShown = 0;
        self->shotTime = 0;
    }
    col::CRect r;
    ScreenRect(&r, 0.0f, 0.0f, ViewWidth(), ViewHeight(), 1, 1);
    float uv[4] = {r.left / 640.0f, r.top / 480.0f, r.right / 640.0f, r.bottom / 480.0f};
    PhotoFilter_SetRect(uv);
    self->previewShown = 1;
    self->menuItem = 1;
    self->state = 1;
    CTimer_StartUserPause();
    self->autoClose = 1;
    self->unknown40 = 1;
    self->closeTime = g_otherTimer + 4000;
    self->take = 0;
}

// a thumbnail loaded (a new 64x64 raster)
uint8_t LoadThumb(PhotoThumb& t, int index) {
    PushStreamingMemoryTag();
    Fn_45B800();
    t.raster = RwRasterCreate(0x40, 0x40, 0, 0x504);
    t.texture = *reinterpret_cast<re::Ptr32<void>*>(static_cast<uint8_t*>(t.raster) + 0x34);
    uint8_t ok = Photo_Load(index, t.texture);
    CTimer_Update(0);
    t.loaded = 1;
    PopMemoryTag();
    return ok;
}

// 0x546490: the thumbnails from `first`: the loaded ones scrolled, the new ones loaded
uint8_t __fastcall PhotoAlbum_LoadThumbs(PhotoAlbum* self, re::Edx, int first) {
    while (self->thumbsLoaded) {
        int current = self->firstThumb;
        if (first < current) {
            int d = current - first;
            if (d < 5) {
                for (int k = 0; k < d; k++) {
                    PhotoThumb& t = self->thumbs[4 - k];
                    if (t.loaded) {
                        if (t.raster) {
                            t.raster = nullptr;
                        }
                        if (t.texture) {
                            Texture_Fn6CA7D0(t.texture, 0);
                            NiRelease(t.texture);
                            t.texture = nullptr;
                        }
                        t.loaded = 0;
                    }
                    self->numThumbs--;
                }
                for (int j = 0; j < 5 - d; j++) {
                    PhotoThumb& dst = self->thumbs[4 - j];
                    PhotoThumb& src = self->thumbs[4 - j - d];
                    dst = src;
                    src.loaded = 0;
                    src.raster = nullptr;
                    src.texture = nullptr;
                }
                for (int k = 0; k < d; k++) {
                    if (!LoadThumb(self->thumbs[k], first + k)) {
                        PhotoAlbum_ClearThumbs(self);
                        return 0;
                    }
                    self->numThumbs++;
                }
                self->firstThumb = first;
                return self->numThumbs > 0;
            }
        } else {
            if (first <= current) {
                self->firstThumb = first;
                return self->numThumbs > 0;
            }
            int d = first - current;
            if (d < 5) {
                for (int k = 0; k < d; k++) {
                    PhotoThumb_Clear(&self->thumbs[k]);
                    self->numThumbs--;
                }
                int keep = 5 - d;
                for (int j = 0; j < keep; j++) {
                    PhotoThumb& dst = self->thumbs[j];
                    PhotoThumb& src = self->thumbs[j + d];
                    dst = src;
                    src.loaded = 0;
                    src.raster = nullptr;
                    src.texture = nullptr;
                }
                for (int k = keep; k < 5; k++) {
                    if (!LoadThumb(self->thumbs[k], first + k)) {
                        PhotoAlbum_ClearThumbs(self);
                        return 0;
                    }
                    self->numThumbs++;
                }
                self->firstThumb = first;
                return self->numThumbs > 0;
            }
        }
        PhotoAlbum_ClearThumbs(self);
    }
    float count = float(self->count);
    if (!(count <= 5.0f)) {
        count = 5.0f;
    }
    int n = int(count);
    for (int k = 0; k < n; k++) {
        if (!LoadThumb(self->thumbs[k], first + k)) {
            PhotoAlbum_ClearThumbs(self);
            return 0;
        }
        self->numThumbs++;
    }
    self->thumbsLoaded = 1;
    self->firstThumb = first;
    return self->numThumbs > 0;
}

} // namespace

#define HUDPHOTO_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(hudphoto_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudphoto_original_, __LINE__), \
             re::Mode::Original)

HUDPHOTO_REVERSED("HUDPhoto::GetViewScale", 0x00543D30, &Photo_GetViewScale);
HUDPHOTO_REVERSED("PhotoAlbum::RefreshCount", 0x00543DE0, &PhotoAlbum_RefreshCount);
HUDPHOTO_REVERSED("PhotoAlbum::Delete", 0x00543E10, &PhotoAlbum_Delete);
HUDPHOTO_REVERSED("HUDPhoto::SetFilter", 0x00543EB0, &Photo_SetFilter_);
HUDPHOTO_REVERSED("HUDPhoto::Close", 0x00543F00, &Photo_Close);
HUDPHOTO_REVERSED("HUDPhoto::CreateTexture", 0x00543F60, &Photo_CreateTexture_);
HUDPHOTO_REVERSED("HUDPhoto::ReleaseTexture", 0x00543FA0, &Photo_ReleaseTexture);
HUDPHOTO_REVERSED("HUDPhoto::SavePhoto", 0x00543FC0, &Photo_Save_);
HUDPHOTO_REVERSED("HUDPhoto::LoadPhoto", 0x00544180, &Photo_Load_);
HUDPHOTO_REVERSED("HUDPhoto::IsInShot", 0x005442E0, &Photo_IsInShot_);
HUDPHOTO_REVERSED("PhotoSlot::Load", 0x00544400, &PhotoSlot_Load_);
HUDPHOTO_REVERSED("PhotoAlbum::Add", 0x005444B0, &PhotoAlbum_Add_);
HUDPHOTO_REVERSED("HUDPhoto::DrawIris", 0x00544550, &Photo_DrawIris_);
HUDPHOTO_REVERSED("HUDPhoto::DrawMenu", 0x00544760, &Photo_DrawMenu_);
HUDPHOTO_REVERSED("HUDPhoto::FrameColour", 0x00545110, &Photo_FrameColour_);
HUDPHOTO_REVERSED("HUDPhoto::SaveToAlbum", 0x00545190, &Photo_SaveToAlbum);
HUDPHOTO_REVERSED("PhotoThumb::Clear", 0x005452B0, &PhotoThumb_Clear_);
HUDPHOTO_REVERSED("HUDPhoto::CountSubjects", 0x005452F0, &Photo_CountSubjects_);
HUDPHOTO_REVERSED("PhotoAlbum::Select", 0x005453F0, &PhotoAlbum_Select);
HUDPHOTO_REVERSED("HUDPhoto::DrawViewfinder", 0x00545450, &Photo_DrawViewfinder);
HUDPHOTO_REVERSED("PhotoAlbum::ClearThumbs", 0x00546200, &PhotoAlbum_ClearThumbs_);
HUDPHOTO_REVERSED("PhotoAlbum::Clear", 0x00546260, &PhotoAlbum_Clear);
HUDPHOTO_REVERSED("PhotoAlbum::~PhotoAlbum", 0x005462A0, &PhotoAlbum_Destruct_);
HUDPHOTO_REVERSED("PhotoAlbum::DeletingDestructor", 0x00546320, &PhotoAlbum_DeletingDestructor);
HUDPHOTO_REVERSED("HUDPhoto::TakePicture", 0x00546340, &Photo_TakePicture);
HUDPHOTO_REVERSED("PhotoAlbum::LoadThumbs", 0x00546490, &PhotoAlbum_LoadThumbs);
