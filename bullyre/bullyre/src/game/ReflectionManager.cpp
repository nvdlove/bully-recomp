// 0x523D90 to 0x523FB0: the reflection manager (Bully's mirrors): the mirror entries
// from Config\Dat\mirrors.dat (0x60 bytes each from +0x24, the count at +0x984), its
// state reset, and the RenderWare camera it renders the reflection with (+0x98C).

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "reversed.h"

namespace {

GAME_ARRAY(char, ms_fileBuffer, 0x00BD7800);   // 0x1C000 bytes
GAME_VAR(void*, ms_pCamera_C3CCF4, 0x00C3CCF4);

GAME_FN(int, __cdecl, LoadFile, 0x0042D180, const char* path, char* buffer, int size, const char* mode);
GAME_FN(uint32_t, __cdecl, HashName_4305D0, 0x004305D0, const char* name);
GAME_FN(void, __thiscall, MirrorEntry_Copy, 0x00523D90, void* self, const void* other);
GAME_FN(void*, __cdecl, RwCameraGetFrame, 0x006C82F0, void* camera);
GAME_FN(void*, __cdecl, RwCameraGetRaster, 0x006C8740, void* camera);
GAME_FN(void*, __cdecl, RwCameraGetZRaster, 0x006C8310, void* camera);
GAME_FN(void, __cdecl, RwCameraSetFrame, 0x006C8720, void* camera, void* frame);
GAME_FN(void, __cdecl, RwCameraSetRaster, 0x006C8750, void* camera, void* raster);
GAME_FN(void, __cdecl, RwCameraSetZRaster, 0x006C8920, void* camera, void* raster);
GAME_FN(void, __cdecl, RwCameraSetProjection, 0x006C8320, void* camera, int projection);
GAME_FN(void, __cdecl, RwCameraDestroy, 0x006C86E0, void* camera);
GAME_FN(void*, __cdecl, RwCameraCreate, 0x006C99E0);
GAME_FN(void*, __cdecl, RwFrameCreate, 0x006CA210);
GAME_FN(void, __cdecl, RwFrameDestroy, 0x006CA260, void* frame);

using re::At;

} // namespace

namespace ReflectionManager {

// 0x523D90 (thiscall): a mirror entry copied (0x60 bytes)
static void* __fastcall CopyEntry(uint32_t* self, re::Edx, const uint32_t* other) {
    for (int i = 0; i < 24; i++) {
        self[i] = other[i];
    }
    return self;
}

// 0x523E30 (thiscall): the camera destroyed (its frame and raster detached first)
static void __fastcall DestroyCamera(uint8_t* self, re::Edx) {
    void* camera = At<void*>(self, 0x98C);
    if (!camera) {
        return;
    }
    void* frame = RwCameraGetFrame(camera);
    void* raster = RwCameraGetRaster(At<void*>(self, 0x98C));
    if (frame) {
        RwCameraSetFrame(At<void*>(self, 0x98C), nullptr);
        RwFrameDestroy(frame);
    }
    if (raster) {
        RwCameraSetRaster(At<void*>(self, 0x98C), nullptr);
    }
    RwCameraDestroy(At<void*>(self, 0x98C));
    At<void*>(self, 0x98C) = nullptr;
}

// 0x523EB0 (thiscall): the state reset: no entries, none current (0xFF), no camera,
// the box at +0 and +0x20 cleared
static void __fastcall Reset(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x984) = 0;
    self[0x98A] = 0;
    At<void*>(self, 0x98C) = nullptr;
    self[0x988] = 0xFF;
    self[0x989] = 0xFF;
    for (int i = 0; i < 8; i++) {
        At<float>(self, i * 4) = 0.0f;
    }
    At<float>(self, 0x20) = 0.0f;
}

// 0x523F40 (thiscall): the camera: a new one with a frame, projection 1, sharing the
// main camera's raster and z raster
static void __fastcall CreateCamera(uint8_t* self, re::Edx) {
    At<void*>(self, 0x98C) = RwCameraCreate();
    RwCameraSetFrame(At<void*>(self, 0x98C), RwFrameCreate());
    RwCameraSetProjection(At<void*>(self, 0x98C), 1);
    void* mainCamera = ms_pCamera_C3CCF4;
    RwCameraSetRaster(At<void*>(self, 0x98C), RwCameraGetRaster(mainCamera));
    RwCameraSetZRaster(At<void*>(self, 0x98C), RwCameraGetZRaster(mainCamera));
}

// 0x523FB0 (thiscall): Config\Dat\mirrors.dat: per line (carriage returns, commas and
// tabs as spaces; blank lines and "#" comments skipped) 12 floats, a name (its hash),
// 12 more floats, into the next entry. (The blank test reads on past the line's end
// until a printable character; the 20th float is read and unused)
static void __fastcall LoadData(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x984) = 0;
    int size = LoadFile("Config\\Dat\\mirrors.dat", ms_fileBuffer, 0x1C000, "r");
    char line[256];
    int i = 0;
    while (i < size) {
        char c = ms_fileBuffer[i];
        int n = 0;
        while (c != '\n' && i < size) {
            line[n] = (c == '\r' || c == ',' || c == '\t') ? ' ' : c;
            c = ms_fileBuffer[i + 1];
            i++;
            n++;
        }
        i++;
        line[n] = '\0';
        int j = 0;
        while (static_cast<signed char>(line[j]) <= ' ') {
            j++;
        }
        if (j == n || line[j] == '#') {
            continue;
        }
        float f[26];
        char name[65];
        std::sscanf(line, "%f %f %f %f %f %f %f %f %f %f %f %f %64s %f %f %f %f %f %f %f %f %f %f %f %f", &f[1],
                    &f[2], &f[3], &f[4], &f[5], &f[6], &f[7], &f[8], &f[9], &f[10], &f[11], &f[12], name, &f[14],
                    &f[15], &f[16], &f[17], &f[18], &f[19], &f[20], &f[21], &f[22], &f[23], &f[24], &f[25]);
        float entry[24];
        entry[0] = f[1];
        entry[1] = f[2];
        entry[2] = f[3];
        uint32_t hash = HashName_4305D0(name);
        std::memcpy(&entry[3], &hash, 4);
        entry[4] = f[4];
        entry[5] = f[5];
        entry[6] = f[6];
        entry[7] = f[21];
        entry[8] = f[7];
        entry[9] = f[8];
        entry[10] = f[9];
        entry[11] = f[24];
        entry[12] = f[10];
        entry[13] = f[11];
        entry[14] = f[12];
        entry[15] = f[25];
        entry[16] = f[14];
        entry[17] = f[15];
        entry[18] = f[18];
        entry[19] = f[23];
        entry[20] = f[16];
        entry[21] = f[17];
        entry[22] = f[19];
        entry[23] = f[22];
        uint32_t count = At<uint32_t>(self, 0x984);
        MirrorEntry_Copy(self + 0x24 + count * 0x60, entry);
        At<uint32_t>(self, 0x984) = count + 1;
    }
}

} // namespace ReflectionManager

// ---- registration ----------------------------------------------------------------------

#define C16_REFLECT_REVERSED(name, fn, address)                                                      \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace ReflectionManager {

C16_REFLECT_REVERSED("CReflectionManager::CopyEntry", CopyEntry, 0x00523D90);
C16_REFLECT_REVERSED("CReflectionManager::DestroyCamera", DestroyCamera, 0x00523E30);
C16_REFLECT_REVERSED("CReflectionManager::Reset", Reset, 0x00523EB0);
C16_REFLECT_REVERSED("CReflectionManager::CreateCamera", CreateCamera, 0x00523F40);
C16_REFLECT_REVERSED("CReflectionManager::LoadData", LoadData, 0x00523FB0);

} // namespace ReflectionManager
