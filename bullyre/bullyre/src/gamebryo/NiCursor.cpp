// NiCursor (vtable 0x9A17A8, 11 slots; NiRTTI 0x20E9378; 0x5C bytes) and NiSystemCursor (vtable
// 0x9A1734, 12 slots; NiRTTI 0x20E930C; 0x68 bytes): the sample framework's software cursors (an
// NiScreenElements quad with the cursor image, drawn by NiCursorRenderClick after the scene). Bully
// stubs the framework's CreateCursor, so none is made in the game: cold code, written exactly.
//
// NiCursor: +0x8 the renderer (held), +0xC its flags, +0x10 / +0x14 the hot spot, +0x18 / +0x1C the
// image's size, +0x20 its alpha property, +0x24 its texture (NiSourceTexture), +0x28 its texturing
// property, +0x2C its z buffer property, +0x30 its screen elements (all held), +0x38 / +0x3C the
// position (pixels), +0x40 the show count, +0x44 a byte, +0x48 the render target group whose size
// it uses (none: the renderer's default, slot 29), +0x4C..+0x58 the rectangle it moves in (left,
// right, top, bottom). NiSystemCursor: +0x5C its kind (an index into the table at 0xBCB9AC: 20
// bytes each, the hot spot and the image's corner in SystemCursors.tga), +0x60 / +0x64 that corner.
//
// Slots: 0 the deleting destructor, 2 GetRTTI, 3 GetViewerStrings, 4 Show, 5 SetPosition, 6 Move,
// 7 Draw, 8 SetFlags (a folded setter of +0xC the game's code shares), 9 SetImage, 10
// UpdateScreenGeometry; NiSystemCursor's 11 its texture coordinates. Also
// NiCursorRenderClick::PerformRendering (slot 6 of its vtable 0x988FD4).

#include "gamebryo/NiD3D10C44.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::c44 {
namespace {

constexpr uintptr_t kCursorVtable = 0x009A17A8;
constexpr uintptr_t kSystemCursorVtable = 0x009A1734;
constexpr uint32_t kCursorSize = 0x5C;
constexpr uint32_t kSystemCursorSize = 0x68;

GAME_VAR(uint8_t, NiRTTI_NiSystemCursor, 0x020E930C);
GAME_VAR(uint8_t, NiRTTI_NiCursor, 0x020E9378);
GAME_VAR(void*, ms_spSystemCursor, 0x020E9340);    // (NiPointer: the one Create made)
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);        // NiRenderer::ms_pkRenderer
GAME_VAR(const double, kCursorPixels, 0x00903910);  // 32 (the image's size)
GAME_VAR(const float, kCursorDepth, 0x009055A0);    // the quad's z (0.5)
GAME_ARRAY(const uint32_t, ms_akSystemCursors, 0x00BCB9AC);   // 5 words a kind

// the screen elements and their data, the properties, the texture (the library's)
GAME_FN(void*, __cdecl, SourceTexture_Create, 0x00769910, const re::Ptr32<char>* name, const void* prefs, int loadDirect,
        int srgb);
GAME_FN(void*, __thiscall, NiPointer_AssignPointer, 0x00800420, void* slot, void* object);
GAME_FN(void*, __thiscall, TexturingProperty_Construct, 0x00765DD0, void* self);
GAME_FN(void, __thiscall, TexturingProperty_SetBaseTexture, 0x00407A60, void* self, void* texture);
GAME_FN(void, __thiscall, TexturingProperty_SetApplyMode, 0x0082BFE0, void* self, uint32_t mode);
GAME_FN(void*, __thiscall, AlphaProperty_Construct, 0x00407800, void* self);
GAME_FN(void*, __thiscall, ZBufferProperty_Construct, 0x00407B40, void* self);
GAME_FN(void*, __thiscall, ScreenElementsData_Construct, 0x007893A0, void* self, int colors, int normals,
        uint32_t textureSets, uint32_t polygons, uint32_t polygonGrowBy, uint32_t vertices, uint32_t vertexGrowBy,
        uint32_t triangles, uint32_t triangleGrowBy);
GAME_FN(void*, __thiscall, ScreenElements_Construct, 0x007AB380, void* self, void* data);
GAME_FN(uint32_t, __thiscall, ScreenElementsData_Insert, 0x00788AA0, void* self, uint32_t vertices,
        uint32_t triangles, uintptr_t index);
GAME_FN(uint8_t, __thiscall, ScreenElementsData_SetRectangle, 0x00789060, void* self, uint32_t polygon, float left,
        float top, float width, float height);
GAME_FN(void, __thiscall, ScreenElementsData_UpdateBound, 0x00788830, void* self);
GAME_FN(uint8_t, __thiscall, ScreenElementsData_SetTextures, 0x00789300, void* self, uint32_t polygon,
        uint32_t textureSet, float left, float top, float right, float bottom);
GAME_FN(void, __thiscall, ScreenElements_RenderImmediate, 0x007AB310, void* self, void* renderer);
GAME_FN(void, __thiscall, AVObject_AttachProperty, 0x004099F0, void* self, void* property);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, void* self);
GAME_FN(float, __cdecl, NiGetCurrentTimeInSec, 0x007537B0);
// this file's (called where modes apply)
GAME_FN(uint32_t, __thiscall, SystemCursor_SetKind, 0x008C42C0, void* self, uint32_t kind);
GAME_FN(void*, __thiscall, SystemCursor_Construct, 0x008C4310, void* self, void* renderer);
GAME_FN(uint8_t, __thiscall, SystemCursor_UpdateTextureCoords, 0x008C4470, void* self);
GAME_FN(void*, __thiscall, Cursor_Construct, 0x008C4940, void* self, void* renderer);
GAME_FN(void, __thiscall, Cursor_Destruct, 0x008C49C0, void* self);
GAME_FN(void, __thiscall, Cursor_GetViewerStrings, 0x008C4F10, void* self, NiTArray<char*>* strings);

inline re::Ptr32<void>& Slot(uint8_t* self, int offset) {
    return At<void*>(self, offset);
}

// the render target group's width and height reciprocals (its own, or the renderer's default)
void TargetReciprocals(uint8_t* self, float& invW, float& invH) {
    void* target = At<void*>(self, 0x48);
    if (target == nullptr) {
        target = Virtual<void*>(At<void*>(self, 8), 29);   // GetDefaultRenderTargetGroup
    }
    uint32_t width = Virtual<uint32_t>(target, 18, 0u);
    invW = 1.0f / float(double(width));
    uint32_t height = Virtual<uint32_t>(target, 19, 0u);
    invH = 1.0f / float(double(height));
}

// the screen-space camera reset for drawing when a frame is open (states 1, 2, 4: the renderer's
// +0x94; with its +0x90 set, or state 4)
void SetScreenSpaceCamera(void* renderer, const void* port) {
    uint32_t state = At<uint32_t>(renderer, 0x94);
    if ((state == 1 || state == 2 || state == 4) && (At<uint8_t>(renderer, 0x90) == 1 || state == 4)) {
        Virtual<void>(renderer, 87, port);   // SetScreenSpaceCameraData
    }
}

// the image's quad at the hot spot, its size in the target's units
void SetQuad(uint8_t* self, void* data, float invW, float invH) {
    float height = invH * float(kCursorPixels);
    float width = float(kCursorPixels) * invW;
    float top = -invH * float(double(At<uint32_t>(self, 0x14)));
    float left = -invW * float(double(At<uint32_t>(self, 0x10)));
    ScreenElementsData_SetRectangle(data, 0, left, top, width, height);
}

} // namespace

// 0x8C00F0 (thiscall, slot 8): SetFlags (+0xC; the same code serves the game's classes)
uint32_t __fastcall Cursor_SetFlags(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0xC) = flags;
    return flags;   // (what the original leaves in eax)
}

// 0x8C4280 (slot 2): NiSystemCursor::GetRTTI
void* __fastcall SystemCursor_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiSystemCursor;
}

// 0x8C4290 (thiscall, slot 0): NiSystemCursor's deleting destructor
void* __fastcall SystemCursor_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kSystemCursorVtable;
    Cursor_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, kSystemCursorSize);
    }
    return self;
}

// 0x8C42C0 (thiscall): NiSystemCursor::SetKind (its hot spot and image corner from the table, its
// coordinates updated; the kind it had)
uint32_t __fastcall SystemCursor_SetKindImpl(uint8_t* self, re::Edx, uint32_t kind) {
    uint32_t old = At<uint32_t>(self, 0x5C);
    if (old != kind) {
        At<uint32_t>(self, 0x5C) = kind;
        const uint32_t* entry = ms_akSystemCursors + kind * 5;
        At<uint32_t>(self, 0x64) = entry[3];
        At<uint32_t>(self, 0x60) = entry[2];
        At<uint32_t>(self, 0x14) = entry[1];
        At<uint32_t>(self, 0x10) = entry[0];
        Virtual<uint8_t>(self, 11);
    }
    return old;
}

// 0x8C4310 (thiscall): NiSystemCursor's constructor (kind 13 at first)
void* __fastcall SystemCursor_ConstructImpl(uint8_t* self, re::Edx, void* renderer) {
    Cursor_Construct(self, renderer);
    At<uint32_t>(self, 0x60) = 0;
    At<uint32_t>(self, 0x64) = 0;
    At<uint32_t>(self, 0) = kSystemCursorVtable;
    At<uint32_t>(self, 0x5C) = 0xD;
    const uint32_t* entry = ms_akSystemCursors + 0xD * 5;
    At<uint32_t>(self, 0x64) = entry[3];
    At<uint32_t>(self, 0x60) = entry[2];
    At<uint32_t>(self, 0x10) = entry[0];
    At<uint32_t>(self, 0x14) = entry[1];
    SystemCursor_UpdateTextureCoords(self);
    return self;
}

// 0x8C4370 (cdecl): NiSystemCursor::Create (the one already made, or a new one with its flags,
// rectangle, image (SystemCursors.tga by default), 32x32, its kind; kept in the static)
void* __cdecl SystemCursor_Create(void* renderer, const int32_t* rect, uint32_t kind, uint32_t flags,
                                  const char* fileName, void* target) {
    if (ms_spSystemCursor != nullptr) {
        return ms_spSystemCursor;
    }
    void* memory = NiNew(kSystemCursorSize);
    if (memory == nullptr) {
        return nullptr;
    }
    uint8_t* cursor = static_cast<uint8_t*>(SystemCursor_Construct(memory, renderer));
    if (cursor == nullptr) {
        return nullptr;
    }
    Virtual<void>(cursor, 8, flags);   // SetFlags
    At<int32_t>(cursor, 0x4C) = rect[0];
    At<int32_t>(cursor, 0x54) = rect[2];
    At<int32_t>(cursor, 0x50) = rect[1];
    At<void*>(cursor, 0x48) = target;
    At<int32_t>(cursor, 0x58) = rect[3];
    if (fileName == nullptr) {
        fileName = Text(0x009A1764);   // "../../Data/SystemCursors.tga"
    }
    if (!Virtual<uint8_t>(cursor, 9, fileName)) {   // SetImage
        Virtual<void>(cursor, 0, 1);
        return nullptr;
    }
    At<uint32_t>(cursor, 0x18) = 0x20;
    At<uint32_t>(cursor, 0x1C) = 0x20;
    SystemCursor_SetKind(cursor, kind);
    Virtual<uint8_t>(cursor, 11);
    NiPointer_AssignPointer(&ms_spSystemCursor, cursor);
    return cursor;
}

// 0x8C4440 (cdecl): NiSystemCursor::Shutdown (the static let go)
void __cdecl SystemCursor_Shutdown() {
    void* cursor = ms_spSystemCursor;
    if (cursor != nullptr) {
        ReleaseRef(cursor);
        ms_spSystemCursor = nullptr;
    }
}

// 0x8C4470 (thiscall, slot 11): the quad at the hot spot and its texture coordinates at the
// kind's corner of the image (32 pixels); false without screen elements
uint8_t __fastcall SystemCursor_UpdateTextureCoordsImpl(uint8_t* self, re::Edx) {
    void* elements = At<void*>(self, 0x30);
    if (elements == nullptr) {
        return 0;
    }
    float invW;
    float invH;
    TargetReciprocals(self, invW, invH);
    SetQuad(self, At<void*>(elements, 0xCC), invW, invH);
    ScreenElementsData_UpdateBound(At<void*>(At<void*>(self, 0x30), 0xCC));
    AVObject_Update(At<void*>(self, 0x30), 0.0f, 1);
    void* texture = At<void*>(self, 0x24);
    uint32_t textureWidth = Virtual<uint32_t>(texture, 18);
    uint32_t textureHeight = Virtual<uint32_t>(At<void*>(self, 0x24), 19);
    float w = float(double(textureWidth));
    float u0 = float(double(At<uint32_t>(self, 0x60))) / w;
    float h = float(double(textureHeight));
    float v0 = float(double(At<uint32_t>(self, 0x64))) / h;
    float v1 = v0 + float(kCursorPixels) / h;
    float u1 = u0 + float(kCursorPixels) / w;
    ScreenElementsData_SetTextures(At<void*>(At<void*>(self, 0x30), 0xCC), 0, 0, u0, v0, u1, v1);
    At<uint16_t>(At<void*>(At<void*>(self, 0x30), 0xCC), 0x34) |= 8;
    return 1;
}

// 0x8C4680 (thiscall, slot 3): NiSystemCursor::GetViewerStrings (its class name, then NiCursor's)
void __fastcall SystemCursor_GetViewerStrings(uint8_t* self, re::Edx, NiTArray<char*>* strings) {
    ni::scene::AddViewerString(strings, ni::scene::GetViewerString(*reinterpret_cast<const re::Ptr32<const char>*>(
                                            &NiRTTI_NiSystemCursor)));
    Cursor_GetViewerStrings(self, strings);
}

// 0x8C4700 (thiscall): NiCursorRenderClick::PerformRendering (the screen-space camera set to its
// viewport; each cursor drawn, slot 7; the count drawn and the time taken kept)
void __fastcall CursorRenderClick_PerformRendering(uint8_t* self, re::Edx, uint32_t) {
    SetScreenSpaceCamera(ms_pkRenderer, self + 0x14);
    float start = NiGetCurrentTimeInSec();
    for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(self, 0x50); node != nullptr;) {
        void* cursor = node[2];
        node = static_cast<re::Ptr32<void>*>(node[0]);
        Virtual<void>(cursor, 7);
    }
    float end = NiGetCurrentTimeInSec();
    At<uint32_t>(self, 0x5C) = At<uint32_t>(self, 0x58);
    At<float>(self, 0x60) = end - start;
}

// 0x8C4780 (thiscall, slot 4): Show (the show count up or down; the count)
int32_t __fastcall Cursor_Show(uint8_t* self, re::Edx, int show) {
    if (re::Flag(show)) {
        At<int32_t>(self, 0x40) += 1;
    } else {
        At<int32_t>(self, 0x40) -= 1;
    }
    return At<int32_t>(self, 0x40);
}

// 0x8C47A0 (thiscall, slot 5): SetPosition (each coordinate inside the rectangle taken; the
// geometry updated, slot 10, when either changed)
uint8_t __fastcall Cursor_SetPosition(uint8_t* self, re::Edx, float, int32_t x, int32_t y) {
    uint8_t changed = 0;
    if (At<int32_t>(self, 0x38) != x && x >= At<int32_t>(self, 0x4C) && x < At<int32_t>(self, 0x50)) {
        At<int32_t>(self, 0x38) = x;
        changed = 1;
    }
    if (At<int32_t>(self, 0x3C) != y && y >= At<int32_t>(self, 0x54) && y < At<int32_t>(self, 0x58)) {
        At<int32_t>(self, 0x3C) = y;
        Virtual<uint8_t>(self, 10);
        return 1;
    }
    if (changed != 0) {
        Virtual<uint8_t>(self, 10);
    }
    return changed;
}

// 0x8C4800 (thiscall, slot 6): Move (by a delta, each coordinate kept inside the rectangle)
uint8_t __fastcall Cursor_Move(uint8_t* self, re::Edx, float, int32_t dx, int32_t dy) {
    uint8_t changed = 0;
    if (dx < 0) {
        int32_t x = dx + At<int32_t>(self, 0x38);
        if (x >= At<int32_t>(self, 0x4C)) {
            changed = 1;
            At<int32_t>(self, 0x38) = x;
        }
    } else if (dx > 0) {
        int32_t x = dx + At<int32_t>(self, 0x38);
        if (x < At<int32_t>(self, 0x50)) {
            changed = 1;
            At<int32_t>(self, 0x38) = x;
        }
    }
    bool moveY = false;
    int32_t y = 0;
    if (dy < 0) {
        y = dy + At<int32_t>(self, 0x3C);
        moveY = y >= At<int32_t>(self, 0x54);
    } else if (dy > 0) {
        y = dy + At<int32_t>(self, 0x3C);
        moveY = y < At<int32_t>(self, 0x58);
    }
    if (moveY) {
        At<int32_t>(self, 0x3C) = y;
        Virtual<uint8_t>(self, 10);
        return 1;
    }
    if (changed != 0) {
        Virtual<uint8_t>(self, 10);
    }
    return changed;
}

// 0x8C4870 (thiscall, slot 10): UpdateScreenGeometry (the quad's translation from the position in
// the target's units; false without screen elements)
uint8_t __fastcall Cursor_UpdateScreenGeometry(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x30) == nullptr) {
        return 0;
    }
    float invW;
    float invH;
    TargetReciprocals(self, invW, invH);
    float* translate = reinterpret_cast<float*>(At<uint8_t*>(self, 0x30) + 0x50);
    translate[0] = float(At<int32_t>(self, 0x38)) * invW;
    translate[1] = float(At<int32_t>(self, 0x3C)) * invH;
    translate[2] = kCursorDepth;
    AVObject_Update(At<void*>(self, 0x30), 0.0f, 1);
    return 1;
}

// 0x8C4940 (thiscall): NiCursor's constructor (the renderer held)
void* __fastcall Cursor_ConstructImpl(uint8_t* self, re::Edx, void* renderer) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    At<uint32_t>(self, 0) = kCursorVtable;
    At<void*>(self, 8) = renderer;
    AddRefRef(renderer);
    static const int kZeroed[11] = {0xC, 0x10, 0x14, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x38, 0x3C, 0x40};
    for (int offset : kZeroed) {
        At<uint32_t>(self, offset) = 0;
    }
    At<uint8_t>(self, 0x44) = 0;
    At<uint32_t>(self, 0x48) = 0;
    At<uint32_t>(self, 0x4C) = 0;
    At<uint32_t>(self, 0x54) = 0;
    At<uint32_t>(self, 0x50) = 0;
    At<uint32_t>(self, 0x58) = 0;
    return self;
}

// 0x8C49B0 (slot 2): NiCursor::GetRTTI
void* __fastcall Cursor_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiCursor;
}

// 0x8C49C0 (thiscall): NiCursor's destructor (the texture, screen elements and renderer let go
// and cleared; then the members' own destructors)
void __fastcall Cursor_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kCursorVtable;
    if (Slot(self, 0x24) != nullptr) {
        ReleaseRef(Slot(self, 0x24));
        Slot(self, 0x24) = nullptr;
    }
    if (Slot(self, 0x30) != nullptr) {
        ReleaseRef(Slot(self, 0x30));
        Slot(self, 0x30) = nullptr;
    }
    if (Slot(self, 8) != nullptr) {
        ReleaseRef(Slot(self, 8));
        Slot(self, 8) = nullptr;
    }
    ReleaseRef(Slot(self, 0x30));
    ReleaseRef(Slot(self, 0x2C));
    ReleaseRef(Slot(self, 0x28));
    ReleaseRef(Slot(self, 0x24));
    ReleaseRef(Slot(self, 0x20));
    ReleaseRef(Slot(self, 8));
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
}

// 0x8C4AF0 (thiscall, slot 7): Draw (when shown: the screen-space camera reset, the screen
// elements drawn)
void __fastcall Cursor_Draw(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x40) <= 0) {
        return;
    }
    SetScreenSpaceCamera(At<void*>(self, 8), nullptr);
    ScreenElements_RenderImmediate(At<void*>(self, 0x30), At<void*>(self, 8));
}

// 0x8C4B40 (thiscall, slot 0): NiCursor's deleting destructor
void* __fastcall Cursor_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    Cursor_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, kCursorSize);
    }
    return self;
}

// 0x8C4B60 (thiscall, slot 9): SetImage (the texture loaded and set on the texturing property;
// the first time, the screen elements made: alpha and z buffer properties, one quad at the hot
// spot with the whole image, at the position as the original has it: its x for both coordinates);
// the image's size taken; false when it can't be loaded
uint8_t __fastcall Cursor_SetImage(uint8_t* self, re::Edx, const char* fileName) {
    uint32_t prefs[3] = {2, 1, 0};   // FormatPrefs: pixel layout, alpha, mipmaps
    re::Ptr32<char> name = fileName != nullptr ? FixedString_Add(fileName) : NullString();
    void* texture = SourceTexture_Create(&name, prefs, 0, 0);
    StringRelease(&name);
    if (texture == nullptr) {
        return 0;
    }
    if (Slot(self, 0x24) != texture) {
        ReleaseRef(Slot(self, 0x24));
        Slot(self, 0x24) = texture;
        Increment(&At<uint32_t>(texture, 4));
    }
    if (Slot(self, 0x28) == nullptr) {
        void* memory = NiNew(0x34);
        NiPointer_AssignPointer(&Slot(self, 0x28), memory != nullptr ? TexturingProperty_Construct(memory) : nullptr);
        if (Slot(self, 0x28) == nullptr) {
            return 0;
        }
    }
    TexturingProperty_SetBaseTexture(Slot(self, 0x28), texture);
    TexturingProperty_SetApplyMode(Slot(self, 0x28), 0);
    At<uint16_t>(Slot(self, 0x28), 0x18) &= 0xFFF1;
    if (Slot(self, 0x30) == nullptr) {
        if (Slot(self, 0x20) == nullptr) {
            void* memory = NiNew(0x1C);
            NiPointer_AssignPointer(&Slot(self, 0x20), memory != nullptr ? AlphaProperty_Construct(memory) : nullptr);
            At<uint16_t>(Slot(self, 0x20), 0x18) |= 1;
        }
        if (Slot(self, 0x2C) == nullptr) {
            void* memory = NiNew(0x30);
            NiPointer_AssignPointer(&Slot(self, 0x2C), memory != nullptr ? ZBufferProperty_Construct(memory) : nullptr);
            At<uint16_t>(Slot(self, 0x2C), 0x18) &= 0xFFFE;
            At<uint16_t>(Slot(self, 0x2C), 0x18) |= 2;
        }
        void* elements = NiNew(0xEC);
        if (elements != nullptr) {
            void* dataMemory = NiNew(0x70);
            void* data = dataMemory != nullptr
                             ? ScreenElementsData_Construct(dataMemory, 0, 0, 1, 1, 1, 4, 1, 2, 1)
                             : nullptr;
            elements = ScreenElements_Construct(elements, data);
        }
        NiPointer_AssignPointer(&Slot(self, 0x30), elements);
        ScreenElementsData_Insert(At<void*>(Slot(self, 0x30), 0xCC), 4, 0, 0);
        float invW;
        float invH;
        TargetReciprocals(self, invW, invH);
        SetQuad(self, At<void*>(Slot(self, 0x30), 0xCC), invW, invH);
        ScreenElementsData_UpdateBound(At<void*>(Slot(self, 0x30), 0xCC));
        ScreenElementsData_SetTextures(At<void*>(Slot(self, 0x30), 0xCC), 0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
        AVObject_AttachProperty(Slot(self, 0x30), Slot(self, 0x28));
        AVObject_AttachProperty(Slot(self, 0x30), Slot(self, 0x20));
        AVObject_AttachProperty(Slot(self, 0x30), Slot(self, 0x2C));
        float x = float(At<int32_t>(self, 0x38));
        float* translate = reinterpret_cast<float*>(At<uint8_t*>(self, 0x30) + 0x50);
        translate[0] = x * invW;
        translate[1] = x * invH;
        translate[2] = kCursorDepth;
    }
    At<uint32_t>(self, 0x18) = Virtual<uint32_t>(texture, 18);   // GetWidth
    At<uint32_t>(self, 0x1C) = Virtual<uint32_t>(texture, 19);   // GetHeight
    AVObject_Update(Slot(self, 0x30), 0.0f, 1);
    AVObject_UpdateProperties(Slot(self, 0x30));
    return 1;
}

// 0x8C4F10 (thiscall, slot 3): NiCursor::GetViewerStrings (its class name, flags, hot spot; the
// texture's and screen elements' own)
void __fastcall Cursor_GetViewerStringsImpl(uint8_t* self, re::Edx, NiTArray<char*>* strings) {
    ni::scene::AddViewerString(strings,
                               ni::scene::GetViewerString(*reinterpret_cast<const re::Ptr32<const char>*>(&NiRTTI_NiCursor)));
    ni::scene::AddViewerString(strings, ni::scene::GetViewerStringUint(Text(0x009A17F4), At<uint32_t>(self, 0xC)));
    ni::scene::AddViewerString(strings, ni::scene::GetViewerStringUint(Text(0x009A17E4), At<uint32_t>(self, 0x10)));
    ni::scene::AddViewerString(strings, ni::scene::GetViewerStringUint(Text(0x009A17D4), At<uint32_t>(self, 0x14)));
    Virtual<void>(At<void*>(self, 0x24), 9, strings);
    Virtual<void>(At<void*>(self, 0x30), 9, strings);
}

#define GB44C_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44C_REVERSED("NiCursor::SetFlags", 0x008C00F0, Cursor_SetFlags);
GB44C_REVERSED("NiSystemCursor::GetRTTI", 0x008C4280, SystemCursor_GetRTTI);
GB44C_REVERSED("NiSystemCursor::DeletingDestructor", 0x008C4290, SystemCursor_DeletingDestructor);
GB44C_REVERSED("NiSystemCursor::SetKind", 0x008C42C0, SystemCursor_SetKindImpl);
GB44C_REVERSED("NiSystemCursor::NiSystemCursor", 0x008C4310, SystemCursor_ConstructImpl);
GB44C_REVERSED("NiSystemCursor::Create", 0x008C4370, SystemCursor_Create);
GB44C_REVERSED("NiSystemCursor::Shutdown", 0x008C4440, SystemCursor_Shutdown);
GB44C_REVERSED("NiSystemCursor::UpdateTextureCoords", 0x008C4470, SystemCursor_UpdateTextureCoordsImpl);
GB44C_REVERSED("NiSystemCursor::GetViewerStrings", 0x008C4680, SystemCursor_GetViewerStrings);
GB44C_REVERSED("NiCursorRenderClick::PerformRendering", 0x008C4700, CursorRenderClick_PerformRendering);
GB44C_REVERSED("NiCursor::Show", 0x008C4780, Cursor_Show);
GB44C_REVERSED("NiCursor::SetPosition", 0x008C47A0, Cursor_SetPosition);
GB44C_REVERSED("NiCursor::Move", 0x008C4800, Cursor_Move);
GB44C_REVERSED("NiCursor::UpdateScreenGeometry", 0x008C4870, Cursor_UpdateScreenGeometry);
GB44C_REVERSED("NiCursor::NiCursor", 0x008C4940, Cursor_ConstructImpl);
GB44C_REVERSED("NiCursor::GetRTTI", 0x008C49B0, Cursor_GetRTTI);
GB44C_REVERSED("NiCursor::~NiCursor", 0x008C49C0, Cursor_DestructImpl);
GB44C_REVERSED("NiCursor::Draw", 0x008C4AF0, Cursor_Draw);
GB44C_REVERSED("NiCursor::DeletingDestructor", 0x008C4B40, Cursor_DeletingDestructor);
GB44C_REVERSED("NiCursor::SetImage", 0x008C4B60, Cursor_SetImage);
GB44C_REVERSED("NiCursor::GetViewerStrings", 0x008C4F10, Cursor_GetViewerStringsImpl);

} // namespace ni::c44
