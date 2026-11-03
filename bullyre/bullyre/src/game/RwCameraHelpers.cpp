// A render camera made with its frame, raster and z raster, sized and destroyed.
// 0x5EF8A0..0x5EF9D0.

#include <cstdint>

#include "reversed.h"
#include "runflags.h"

namespace {

struct CamRect {
    int32_t x, y, w, h;
};

GAME_FN(uint8_t*, __cdecl, CH_GetRaster, 0x006C8740, void* camera);
GAME_FN(uint8_t*, __cdecl, CH_GetZRaster, 0x006C8770, void* camera);
GAME_FN(void, __cdecl, CH_SetViewWindow, 0x006C8920, void* camera, const float* window);
GAME_FN(void*, __cdecl, CH_GetFrame, 0x006C82F0, void* camera);
GAME_FN(void, __cdecl, CH_FrameDestroy, 0x006CA260, void* frame);
GAME_FN(void, __cdecl, CH_RasterDestroy, 0x00519090, void* raster);
GAME_FN(void, __cdecl, CH_CameraDestroy, 0x006C86E0, void* camera);
GAME_FN(void*, __cdecl, CH_CameraCreate, 0x006C99E0);
GAME_FN(void*, __cdecl, CH_FrameCreate, 0x006CA210);
GAME_FN(void, __cdecl, CH_SetFrame, 0x006C8720, void* camera, void* frame);
GAME_FN(void*, __cdecl, CH_RasterCreate, 0x00519F60, int width, int height, int depth, int flags);
GAME_FN(void, __cdecl, CH_SetRaster, 0x006C8750, void* camera, void* raster);
GAME_FN(void, __cdecl, CH_SetZRaster, 0x006C8760, void* camera, void* raster);
GAME_FN(void, __cdecl, CH_Destroy, 0x005EF940, void* camera);

// 0x5EF8A0: the rasters resized to a rect's size (640x480 without one) and the view
// window set (viewWindow, viewWindow / aspect)
void __cdecl SetView(void* camera, const CamRect* rect, float viewWindow, float aspect) {
    if (!camera) {
        return;
    }
    int32_t width = *reinterpret_cast<int32_t*>(CH_GetRaster(camera) + 0xC);
    int32_t height = *reinterpret_cast<int32_t*>(CH_GetRaster(camera) + 0x10);
    CamRect standard;
    if (!rect) {
        standard.y = 0;
        standard.x = 0;
        standard.w = 0x280;
        standard.h = 0x1E0;
        rect = &standard;
    }
    if (width != rect->w || height != rect->h) {
        uint8_t* raster = CH_GetRaster(camera);
        uint8_t* zRaster = CH_GetZRaster(camera);
        int32_t w = rect->w;
        *reinterpret_cast<int32_t*>(zRaster + 0xC) = w;
        *reinterpret_cast<int32_t*>(raster + 0xC) = w;
        int32_t h = rect->h;
        *reinterpret_cast<int32_t*>(zRaster + 0x10) = h;
        *reinterpret_cast<int32_t*>(raster + 0x10) = h;
    }
    float window[2] = {viewWindow, viewWindow / aspect};
    if (aspect > 1.5f) {
        window[0] = float(window[0] * re::WiderThan169());   // (a screen wider than 16:9: runflags.h)
    }
    CH_SetViewWindow(camera, window);
}

// the camera's raster destroyed (and the one it points at, when another)
void DestroyRaster(uint8_t* raster) {
    if (!raster) {
        return;
    }
    uint8_t* parent = *reinterpret_cast<re::Ptr32<uint8_t>*>(raster);
    CH_RasterDestroy(raster);
    if (parent && parent != raster) {
        CH_RasterDestroy(parent);
    }
}

// 0x5EF940: the camera destroyed with its frame and rasters
void __cdecl Destroy(void* camera) {
    if (!camera) {
        return;
    }
    if (CH_GetFrame(camera)) {
        CH_FrameDestroy(CH_GetFrame(camera));
    }
    DestroyRaster(CH_GetRaster(camera));
    DestroyRaster(CH_GetZRaster(camera));
    CH_CameraDestroy(camera);
}

// 0x5EF9D0: a camera made with a frame, a raster and (when asked) a z raster; destroyed
// and null when any is missing
void* __cdecl Create(int width, int height, int zBuffer) {
    void* camera = CH_CameraCreate();
    if (camera) {
        CH_SetFrame(camera, CH_FrameCreate());
        CH_SetRaster(camera, CH_RasterCreate(width, height, 0, 2));
        if (zBuffer) {
            CH_SetZRaster(camera, CH_RasterCreate(width, height, 0, 1));
        }
        if (CH_GetFrame(camera) && CH_GetRaster(camera) && (!zBuffer || CH_GetZRaster(camera))) {
            return camera;
        }
    }
    CH_Destroy(camera);
    return nullptr;
}

#define CAMHELP_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(CamHelpOriginal_, __LINE__);                                           \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(CamHelpOriginal_, __LINE__), \
             re::Mode::Original)

CAMHELP_REVERSED("RwCameraHelpers::SetView", 0x005EF8A0, &SetView);
CAMHELP_REVERSED("RwCameraHelpers::Destroy", 0x005EF940, &Destroy);
CAMHELP_REVERSED("RwCameraHelpers::Create", 0x005EF9D0, &Create);

} // namespace
