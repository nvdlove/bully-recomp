// NiDX9SystemDesc and what it holds (Gamebryo's description of the Direct3D 9 system, made once
// by NiDX9Renderer::GetSystemDesc): the adapters (NiDX9AdapterDesc, 0x468: +0x0 the ordinal,
// +0x4 D3DADAPTER_IDENTIFIER9, +0x450 the display modes (NiTPrimitiveArray<ModeDesc*>), +0x460 the
// HAL device, +0x464 the reference one), each device (NiDX9DeviceDesc, 0x148: +0x0 the type, +0x4
// D3DCAPS9, +0x134 its display formats (NiTPointerList<DisplayFormatInfo*>), +0x144 whether it
// renders windowed on the desktop's format), each display format (DisplayFormatInfo, 0x18: +0x0
// the format, +0x4 windowed, +0x5 fullscreen, +0x8 the render target formats it takes: an
// NiTPointerMap<D3DFORMAT, uint8_t[2][9][17]>, the multisample quality levels by depth format and
// sample count, windowed then fullscreen), each mode (ModeDesc, 0x20: +0x0 width, +0x4 height,
// +0x8 bits per pixel, +0xC the format, +0x10 its refresh rates (NiTPointerList)). Plus the
// D3DFORMAT helpers (name, bits per pixel, depth / stencil bits) and NiTPointerList's base
// deleting destructor (vtable 0x99C748).

#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kListBaseVtable = 0x0099C748;     // NiTListBase (the abstract allocator holder)
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;
constexpr uintptr_t kRefreshListVtable = 0x0095145C;  // NiTPointerList<uint32_t>
constexpr uintptr_t kFormatListVtable = 0x00952E10;   // NiTPointerList<DisplayFormatInfo*>
constexpr uintptr_t kMapBaseVtable = 0x00950760;      // NiTMapBase
constexpr uintptr_t kPointerMapVtable = 0x00965A9C;   // NiTPointerMap

// IDirect3D9's slots
enum : int {
    kGetAdapterCount = 4,
    kGetAdapterIdentifier = 5,
    kGetAdapterModeCount = 6,
    kEnumAdapterModes = 7,
    kGetAdapterDisplayMode = 8,
    kCheckDeviceType = 9,
    kCheckDeviceFormat = 10,
    kCheckDeviceMultiSampleType = 11,
    kCheckDepthStencilMatch = 12,
    kGetDeviceCaps = 14,
};

// the depth / stencil formats a format info tries, in its table's order
GAME_ARRAY(const uint32_t, ms_aeDepthStencilFormats, 0x00BCB90C);   // [9]

GAME_FN(uint32_t, __cdecl, FrameBufferFormatToD3D, 0x0082C6B0, uint32_t format);   // NiDX9Renderer::GetD3DFormat
GAME_FN(uint32_t, __cdecl, GetNiFBFormat, 0x0082C780, uint32_t d3dFormat);
GAME_FN(void, __thiscall, PrimitiveArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, PointerMap_SetAt, 0x008A5140, void* map, uint32_t key, void* value);
// this file's (called where modes apply)
GAME_FN(void, __cdecl, GetDepthStencilBits, 0x008903B0, uint32_t format, uint32_t* depth, uint32_t* stencil);
GAME_FN(uint32_t, __cdecl, GetDepthStencilIndex, 0x008904A0, uint32_t format);
GAME_FN(uint32_t, __cdecl, GetFormatBitsPerPixel, 0x00890840, uint32_t format);
GAME_FN(uint32_t, __thiscall, ModeDesc_FindClosestRefreshRate, 0x00890A30, void* self, uint32_t rate);
GAME_FN(void*, __thiscall, FormatInfo_Construct, 0x00890A90, void* self, void* d3d, uint32_t adapter,
        uint32_t deviceType, uint32_t format, int fullscreen, int windowed);
GAME_FN(uint32_t, __thiscall, FormatInfo_FindClosestDepthStencil, 0x00890F40, void* self, uint32_t renderTarget,
        uint32_t depthBits, uint32_t stencilBits);
GAME_FN(void*, __thiscall, DeviceDesc_Construct, 0x00891170, void* self, void* d3d, uint32_t adapter,
        uint32_t deviceType, uint32_t desktopFormat, void* formats);
GAME_FN(void*, __thiscall, ModeDesc_Construct, 0x008913E0, void* self, const uint32_t* mode);
GAME_FN(void*, __thiscall, AdapterDesc_Construct, 0x00891460, void* self, void* d3d, uint32_t adapter, void* formats);
GAME_FN(void, __thiscall, SystemDesc_Enumerate, 0x008916F0, void* self, void* d3d, void* formats);
GAME_FN(void, __thiscall, FormatInfo_Destruct, 0x00890DA0, void* self);
GAME_FN(void, __thiscall, DeviceDesc_Destruct, 0x00891390, void* self);
GAME_FN(void, __thiscall, AdapterDesc_Destruct, 0x00891790, void* self);
GAME_FN(void, __thiscall, TMapBase_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);

// an NiTPointerList's node (next, prev, element) and its allocator (the list's own vtable,
// slot 1 a new node)
struct ListNode {
    re::Ptr32<ListNode> next;
    re::Ptr32<ListNode> prev;
    uint32_t element;
};
struct List {
    re::Ptr32<void> vtable;
    re::Ptr32<ListNode> head;
    re::Ptr32<ListNode> tail;
    uint32_t count;
};
static_assert(sizeof(List) == 0x10);

inline ListNode* NewNode(List* list) {
    return Virtual<ListNode*>(list, 1);
}
inline void AddTail(List* list, uintptr_t element) {
    ListNode* node = NewNode(list);
    node->element = element;
    node->next = nullptr;
    node->prev = list->tail;
    if (list->tail != nullptr) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->count++;
    list->tail = node;
}

// NiTPrimitiveArray's SetAt as inlined (the slot counts kept)
inline void ArraySetAt(NiTArray<void*>* array, uint32_t index, void* value) {
    if (index >= array->size) {
        array->size = uint16_t(index + 1);
        if (value != nullptr) {
            array->esize++;
        }
    } else if (value != nullptr) {
        if (array->base[index] == nullptr) {
            array->esize++;
        }
    } else if (array->base[index] != nullptr) {
        array->esize--;
    }
    array->base[index] = value;
}

// the NiTPointerMap<D3DFORMAT, ...> lookup the format info inlines (+0xC the size, +0x10 the
// buckets; a bucket's node: next, key, value)
inline uint8_t* FindTable(const uint8_t* info, uint32_t format) {
    re::Ptr32<void>* node = At<re::Ptr32<re::Ptr32<void>>*>(info, 0x10)[format % At<uint32_t>(info, 0xC)];
    while (node != nullptr) {
        if (re::PtrCast<uintptr_t>(node[1]) == format) {
            return static_cast<uint8_t*>(node[2]);
        }
        node = static_cast<re::Ptr32<void>*>(node[0]);
    }
    return nullptr;
}

inline int Distance(int a, int b) {
    int d = a - b;
    return d < 0 ? -d : d;
}

} // namespace

// 0x8903B0 (cdecl): the depth and stencil bits of a depth / stencil format (0 for the rest)
void __cdecl NiDX9_GetDepthStencilBits(uint32_t format, uint32_t* depth, uint32_t* stencil) {
    switch (format) {
    case 0x46:   // D3DFMT_D16_LOCKABLE
        *depth = 16;
        *stencil = 0;
        return;
    case 0x47:   // D3DFMT_D32
        *depth = 32;
        *stencil = 0;
        return;
    case 0x49:   // D3DFMT_D15S1
        *depth = 15;
        *stencil = 1;
        return;
    case 0x4B:   // D3DFMT_D24S8
        *depth = 24;
        *stencil = 8;
        return;
    case 0x4D:   // D3DFMT_D24X8
        *depth = 24;
        *stencil = 0;
        return;
    case 0x4F:   // D3DFMT_D24X4S4
        *depth = 24;
        *stencil = 4;
        return;
    case 0x50:   // D3DFMT_D16
        *depth = 16;
        *stencil = 0;
        return;
    default:
        *depth = 0;
        *stencil = 0;
        return;
    }
}

// 0x8904A0 (cdecl): a depth / stencil format's row in the multisample tables (9 for the rest)
uint32_t __cdecl NiDX9_GetDepthStencilIndex(uint32_t format) {
    switch (format) {
    case 0x46:
        return 0;
    case 0x47:
        return 1;
    case 0x49:
        return 2;
    case 0x4B:
        return 3;
    case 0x4D:
        return 4;
    case 0x4F:
        return 5;
    case 0x50:
        return 8;
    case 0x52:
        return 6;
    case 0x53:
        return 7;
    default:
        return 9;
    }
}

// 0x890530 (cdecl): a D3DFORMAT's name
const char* __cdecl NiDX9_GetD3DFormatString(uint32_t format) {
    switch (format) {
    case 0x3154454D:
        return "D3DFMT_MULTI2_ARGB8";
    case 0x14:
        return "D3DFMT_R8G8B8";
    case 0x15:
        return "D3DFMT_A8R8G8B8";
    case 0x16:
        return "D3DFMT_X8R8G8B8";
    case 0x17:
        return "D3DFMT_R5G6B5";
    case 0x18:
        return "D3DFMT_X1R5G5B5";
    case 0x19:
        return "D3DFMT_A1R5G5B5";
    case 0x1A:
        return "D3DFMT_A4R4G4B4";
    case 0x1B:
        return "D3DFMT_R3G3B2";
    case 0x1C:
        return "D3DFMT_A8";
    case 0x1D:
        return "D3DFMT_A8R3G3B2";
    case 0x1E:
        return "D3DFMT_X4R4G4B4";
    case 0x1F:
        return "D3DFMT_A2B10G10R10";
    case 0x20:
        return "D3DFMT_A8B8G8R8";
    case 0x21:
        return "D3DFMT_X8B8G8R8";
    case 0x22:
        return "D3DFMT_G16R16";
    case 0x23:
        return "D3DFMT_A2R10G10B10";
    case 0x24:
        return "D3DFMT_A16B16G16R16";
    case 0x28:
        return "D3DFMT_A8P8";
    case 0x29:
        return "D3DFMT_P8";
    case 0x32:
        return "D3DFMT_L8";
    case 0x33:
        return "D3DFMT_A8L8";
    case 0x34:
        return "D3DFMT_A4L4";
    case 0x3C:
        return "D3DFMT_V8U8";
    case 0x3D:
        return "D3DFMT_L6V5U5";
    case 0x3E:
        return "D3DFMT_X8L8V8U8";
    case 0x3F:
        return "D3DFMT_Q8W8V8U8";
    case 0x40:
        return "D3DFMT_V16U16";
    case 0x43:
        return "D3DFMT_A2W10V10U10";
    case 0x46:
        return "D3DFMT_D16_LOCKABLE";
    case 0x47:
        return "D3DFMT_D32";
    case 0x49:
        return "D3DFMT_D15S1";
    case 0x4B:
        return "D3DFMT_D24S8";
    case 0x4D:
        return "D3DFMT_D24X8";
    case 0x4F:
        return "D3DFMT_D24X4S4";
    case 0x50:
        return "D3DFMT_D16";
    case 0x51:
        return "D3DFMT_L16";
    case 0x52:
        return "D3DFMT_D32F_LOCKABLE";
    case 0x53:
        return "D3DFMT_D24FS8";
    case 100:
        return "D3DFMT_VERTEXDATA";
    case 0x65:
        return "D3DFMT_INDEX16";
    case 0x66:
        return "D3DFMT_INDEX32";
    case 0x6E:
        return "D3DFMT_Q16W16V16U16";
    case 0x6F:
        return "D3DFMT_R16F";
    case 0x70:
        return "D3DFMT_G16R16F";
    case 0x71:
        return "D3DFMT_A16B16G16R16F";
    case 0x72:
        return "D3DFMT_R32F";
    case 0x73:
        return "D3DFMT_G32R32F";
    case 0x74:
        return "D3DFMT_A32B32G32R32F";
    case 0x75:
        return "D3DFMT_CxV8U8";
    case 0x34545844:
        return "D3DFMT_DXT4";
    case 0x32595559:
        return "D3DFMT_YUY2";
    case 0x31545844:
        return "D3DFMT_DXT1";
    case 0x32545844:
        return "D3DFMT_DXT2";
    case 0x33545844:
        return "D3DFMT_DXT3";
    case 0x47424752:
        return "D3DFMT_R8G8_B8G8";
    case 0x35545844:
        return "D3DFMT_DXT5";
    case 0x42475247:
        return "D3DFMT_G8R8_G8B8";
    case 0x59565955:
        return "D3DFMT_UYVY";
    default:
        return "UNKNOWN";
    }
}

// 0x890840 (cdecl): a D3DFORMAT's bits per pixel (0 for the compressed, video and multi-element
// ones, 0xFFFFFFFF for an unknown one)
uint32_t __cdecl NiDX9_GetFormatBitsPerPixel(uint32_t format) {
    switch (format) {
    case 0x14:
        return 24;
    case 0x15:
    case 0x16:
    case 0x1F:
    case 0x20:
    case 0x21:
    case 0x23:
    case 0x3E:
    case 0x3F:
    case 0x40:
    case 0x43:
    case 0x47:
    case 0x4B:
    case 0x4D:
    case 0x4F:
    case 0x52:
    case 0x53:
    case 0x66:
    case 0x70:
    case 0x72:
        return 32;
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1D:
    case 0x1E:
    case 0x22:
    case 0x28:
    case 0x33:
    case 0x3C:
    case 0x3D:
    case 0x46:
    case 0x49:
    case 0x50:
    case 0x51:
    case 0x65:
    case 0x6F:
    case 0x75:
        return 16;
    case 0x1B:
    case 0x1C:
    case 0x29:
    case 0x32:
    case 0x34:
        return 8;
    case 0x24:
    case 0x6E:
    case 0x71:
    case 0x73:
        return 64;
    case 0x74:
        return 128;
    case 0:
    case 100:
    case 0x3154454D:
    case 0x34545844:
    case 0x32595559:
    case 0x31545844:
    case 0x32545844:
    case 0x33545844:
    case 0x47424752:
    case 0x35545844:
    case 0x42475247:
    case 0x59565955:
        return 0;
    default:
        return 0xFFFFFFFF;
    }
}

// 0x890970 (thiscall): NiTListBase's deleting destructor (0x10)
void* __fastcall NiTListBase_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kListBaseVtable;
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x8909A0 (thiscall): NiDX9DeviceDesc::GetDefaultFrameBufferFormat (the first display format of
// that bit depth the device renders to windowed / fullscreen, as Gamebryo's format; 0 for none)
uint32_t __fastcall NiDX9DeviceDesc_GetDefaultFrameBufferFormat(uint8_t* self, re::Edx, int windowed, uint32_t bits) {
    for (ListNode* node = At<ListNode*>(self, 0x138); node != nullptr;) {
        uint8_t* info = reinterpret_cast<uint8_t*>(node->element);
        node = node->next;
        if (info != nullptr && At<uint8_t>(info, re::Flag(windowed) ? 4 : 5) != 0 &&
            GetFormatBitsPerPixel(At<uint32_t>(info, 0)) == bits) {
            return GetNiFBFormat(At<uint32_t>(info, 0));
        }
    }
    return 0;
}

// 0x890A30 (thiscall): NiDX9AdapterDesc::ModeDesc::FindClosestRefreshRate (0 for 0. Quirk kept:
// the distance starts at the first rate's but only the later ones can win, so the first one is
// never returned: 0 when it's the closest)
uint32_t __fastcall NiDX9ModeDesc_FindClosestRefreshRate(uint8_t* self, re::Edx, uint32_t rate) {
    if (rate == 0) {
        return 0;
    }
    ListNode* head = At<ListNode*>(self, 0x14);
    uint32_t best = uint32_t(Distance(int(head->element), int(rate)));
    uint32_t result = 0;
    for (ListNode* node = head->next; node != nullptr;) {
        uint32_t candidate = uint32_t(node->element);
        node = node->next;
        uint32_t distance = uint32_t(Distance(int(candidate), int(rate)));
        if (distance < best) {
            best = distance;
            result = candidate;
        }
    }
    return result;
}

// 0x890A90 (thiscall): NiDX9DeviceDesc::DisplayFormatInfo's constructor (each Gamebryo frame
// buffer format that renders on this display format, with the multisample quality levels each
// matching depth / stencil format allows, windowed and fullscreen. Quirk kept: which sample counts
// to try comes from the display format, not the render target's)
void* __fastcall NiDX9DisplayFormatInfo_Construct(uint8_t* self, re::Edx, void* d3d, uint32_t adapter,
                                                  uint32_t deviceType, uint32_t format, int fullscreen,
                                                  int windowed) {
    At<uint32_t>(self, 8) = kMapBaseVtable;
    At<uint32_t>(self, 0xC) = 37;
    At<uint32_t>(self, 0x14) = 0;
    void* buckets = NiMalloc(0x94);
    At<void*>(self, 0x10) = buckets;
    crt::Memset(buckets, 0, At<uint32_t>(self, 0xC) * 4);
    At<uint32_t>(self, 8) = kPointerMapVtable;
    At<uint32_t>(self, 0) = format;
    At<uint8_t>(self, 5) = uint8_t(fullscreen);
    At<uint8_t>(self, 4) = uint8_t(windowed);
    for (uint32_t i = 0; i < 18; i++) {
        if (i == 0) {
            continue;
        }
        uint32_t target = FrameBufferFormatToD3D(i);
        if (Com<long>(d3d, kCheckDeviceFormat, adapter, deviceType, format, 1u, 1u, target) < 0) {
            continue;   // (not a render target here)
        }
        uint8_t* table = static_cast<uint8_t*>(NiNew(0x132));
        if (table != nullptr) {
            crt::Memset(table, 0, 0x99);
            crt::Memset(table + 0x99, 0, 0x99);
        }
        uint32_t windowedMask = 0;
        uint32_t fullscreenMask = 0;
        uint32_t quality;
        for (uint32_t j = 0; j < 16; j++) {
            if (uint8_t(windowed) != 0 &&
                Com<long>(d3d, kCheckDeviceMultiSampleType, adapter, deviceType, format, 1, j + 1, &quality) >= 0) {
                windowedMask |= 1u << j;
            }
            if (uint8_t(fullscreen) != 0 &&
                Com<long>(d3d, kCheckDeviceMultiSampleType, adapter, deviceType, format, 0, j + 1, &quality) >= 0) {
                fullscreenMask |= 1u << j;
            }
        }
        bool any = false;
        uint8_t* fullRow = table + 0x99;
        for (uint32_t k = 0; k < 9; k++, fullRow += 0x11) {
            uint32_t depth = ms_aeDepthStencilFormats[k];
            if (Com<long>(d3d, kCheckDeviceFormat, adapter, deviceType, format, 2u, 1u, depth) < 0 ||
                Com<long>(d3d, kCheckDepthStencilMatch, adapter, deviceType, format, target, depth) < 0) {
                continue;
            }
            uint8_t* windowedRow = fullRow - 0x99;
            windowedRow[0] = uint8_t(windowed);
            fullRow[0] = uint8_t(fullscreen);
            any = true;
            for (uint32_t j = 0; j < 16; j++) {
                uint32_t bit = 1u << j;
                if (windowedMask & bit) {
                    uint32_t levels;
                    long hr = Com<long>(d3d, kCheckDeviceMultiSampleType, adapter, deviceType, depth, 1, j + 1, &levels);
                    windowedRow[j + 1] = hr < 0 ? 0 : uint8_t(levels);
                } else {
                    windowedRow[j + 1] = 0;
                }
                if (fullscreenMask & bit) {
                    uint32_t levels;
                    long hr = Com<long>(d3d, kCheckDeviceMultiSampleType, adapter, deviceType, depth, 0, j + 1, &levels);
                    fullRow[j + 1] = hr < 0 ? 0 : uint8_t(levels);
                } else {
                    fullRow[j + 1] = 0;
                }
            }
        }
        if (any) {
            PointerMap_SetAt(self + 8, target, table);
        } else {
            NiDelete(table, 0x132);
        }
    }
    return self;
}

// 0x890E50 (thiscall): DisplayFormatInfo::GetMultiSampleQualityLevels (0 for no multisampling,
// an unknown render target format; by the depth format's row)
uint8_t __fastcall NiDX9DisplayFormatInfo_GetMultiSampleQualityLevels(uint8_t* self, re::Edx, uint32_t samples,
                                                                      int windowed, uint32_t renderTarget,
                                                                      uint32_t depth) {
    if (samples == 0) {
        return 0;
    }
    uint8_t* table = FindTable(self, renderTarget);
    if (table == nullptr) {
        return 0;
    }
    uint8_t* row = table + GetDepthStencilIndex(depth) * 0x11;
    return re::Flag(windowed) ? row[samples] : row[samples + 0x99];
}

// 0x890ED0 (thiscall): DisplayFormatInfo::IsMultiSampleValid (any render target format: no
// multisampling always is)
uint8_t __fastcall NiDX9DisplayFormatInfo_IsMultiSampleValid(uint8_t* self, re::Edx, int windowed, uint32_t samples,
                                                             uint32_t renderTarget, uint32_t depth) {
    uint8_t* table = FindTable(self, renderTarget);
    if (table == nullptr) {
        return 0;
    }
    if (samples == 0) {
        return 1;
    }
    uint32_t index = GetDepthStencilIndex(depth);
    if (!re::Flag(windowed)) {
        index += 9;
    }
    return table[index * 0x11 + samples] != 0 ? 1 : 0;
}

// 0x890F40 (thiscall): DisplayFormatInfo::FindClosestDepthStencil (of the depth / stencil formats
// valid fullscreen with the render target format, the one closest in depth bits, then in stencil
// bits; ties go to the later one)
uint32_t __fastcall NiDX9DisplayFormatInfo_FindClosestDepthStencil(uint8_t* self, re::Edx, uint32_t renderTarget,
                                                                   uint32_t depthBits, uint32_t stencilBits) {
    uint8_t* table = FindTable(self, renderTarget);
    if (table == nullptr) {
        return 0;
    }
    int bestDepth = 0x7FFFFFFF;
    int bestStencil = 0x7FFFFFFF;
    uint32_t result = 0;
    const uint8_t* row = table + 0x99;
    for (uint32_t k = 0; k < 9; k++, row += 0x11) {
        if (row[0] == 0) {
            continue;
        }
        uint32_t format = ms_aeDepthStencilFormats[k];
        uint32_t depth = 0;
        uint32_t stencil = 0;
        GetDepthStencilBits(format, &depth, &stencil);
        if (Distance(bestDepth, int(depthBits)) < Distance(int(depth), int(depthBits))) {
            continue;
        }
        if (Distance(bestStencil, int(stencilBits)) < Distance(int(stencil), int(stencilBits))) {
            continue;
        }
        bestStencil = int(stencil);
        bestDepth = int(depth);
        result = format;
    }
    return result;
}

// 0x891040 (thiscall): NiDX9DeviceDesc::FindClosestDepthStencilFormat (on the display format's
// info: with a stencil wanted, 15/1 or 24/8 by the depth bits and stencil asked for)
uint32_t __fastcall NiDX9DeviceDesc_FindClosestDepthStencilFormat(uint8_t* self, re::Edx, uint32_t adapterFormat,
                                                                  uint32_t renderTarget, uint32_t depthBits,
                                                                  uint32_t stencilBits) {
    uint8_t* info = nullptr;
    for (ListNode* node = At<ListNode*>(self, 0x138); node != nullptr;) {
        uint8_t* candidate = reinterpret_cast<uint8_t*>(node->element);
        node = node->next;
        if (At<uint32_t>(candidate, 0) == adapterFormat) {
            info = candidate;
            break;
        }
    }
    if (info == nullptr) {
        return 0;
    }
    if (stencilBits != 0) {
        if (depthBits > 16) {
            depthBits = 24;
            stencilBits = stencilBits == 1 ? 1 : 8;
        } else if (stencilBits == 1) {
            depthBits = 15;
        } else {
            depthBits = 24;
            stencilBits = 8;
        }
    }
    return FormatInfo_FindClosestDepthStencil(info, renderTarget, depthBits, stencilBits);
}

// 0x8910E0 (thiscall): NiDX9AdapterDesc::ValidateFullscreenMode (the last mode of that format and
// size; the refresh rate made the closest each matching mode has)
void* __fastcall NiDX9AdapterDesc_ValidateFullscreenMode(uint8_t* self, re::Edx, uint32_t format, uint32_t width,
                                                         uint32_t height, uint32_t* refreshRate) {
    void* result = nullptr;
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x45A); i++) {
        uint8_t* mode = At<re::Ptr32<uint8_t>*>(self, 0x454)[i];
        if (mode != nullptr && At<uint32_t>(mode, 0xC) == format && At<uint32_t>(mode, 0) == width &&
            At<uint32_t>(mode, 4) == height) {
            result = mode;
            *refreshRate = ModeDesc_FindClosestRefreshRate(mode, *refreshRate);
        }
    }
    return result;
}

// 0x891150 (thiscall): DisplayFormatInfo::FindClosestDepthStencil for its own format
uint32_t __fastcall NiDX9DisplayFormatInfo_FindClosestDepthStencilOwn(uint8_t* self, re::Edx, uint32_t depthBits,
                                                                      uint32_t stencilBits) {
    return FormatInfo_FindClosestDepthStencil(self, At<uint32_t>(self, 0), depthBits, stencilBits);
}

// 0x891170 (thiscall): NiDX9DeviceDesc's constructor (its caps; the display formats of every
// mode it can render fullscreen or windowed on the desktop. Quirks kept: the fullscreen check's
// result isn't looked at, so every enumerated mode counts as fullscreen; the windowed check runs
// even when the mode wasn't enumerated)
void* __fastcall NiDX9DeviceDesc_Construct(uint8_t* self, re::Edx, void* d3d, uint32_t adapter, uint32_t deviceType,
                                           uint32_t desktopFormat, List* formats) {
    At<uint32_t>(self, 0) = deviceType;
    List* infos = reinterpret_cast<List*>(self + 0x134);
    infos->count = 0;
    infos->head = nullptr;
    infos->tail = nullptr;
    infos->vtable = reinterpret_cast<void*>(kFormatListVtable);
    if (Com<long>(d3d, kGetDeviceCaps, adapter, At<uint32_t>(self, 0), self + 4) < 0) {
        At<uint32_t>(self, 4) = 0;
    } else {
        uint32_t mode[4] = {};   // D3DDISPLAYMODE (the original's is uninitialized stack)
        for (ListNode* node = formats->head; node != nullptr;) {
            uint32_t format = uint32_t(node->element);
            node = node->next;
            if (format == 0) {
                continue;
            }
            uint32_t count = Com<uint32_t>(d3d, kGetAdapterModeCount, adapter, format);
            for (uint32_t k = 0; k < count; k++) {
                uint8_t windowed = 0;
                uint8_t fullscreen = 0;
                if (Com<long>(d3d, kEnumAdapterModes, adapter, format, k, mode) >= 0) {
                    Com<long>(d3d, kCheckDeviceType, adapter, At<uint32_t>(self, 0), format, mode[3], 0);
                    fullscreen = 1;
                }
                if (Com<long>(d3d, kCheckDeviceType, adapter, At<uint32_t>(self, 0), desktopFormat, mode[3], 1) >= 0) {
                    windowed = 1;
                }
                if (fullscreen == 0 && windowed == 0) {
                    continue;
                }
                bool known = false;
                for (ListNode* info = infos->head; info != nullptr; info = info->next) {
                    if (*reinterpret_cast<uint32_t*>(info->element) == mode[3]) {
                        known = true;
                        break;
                    }
                }
                if (known) {
                    continue;
                }
                void* info = NiNew(0x18);
                if (info != nullptr) {
                    info = FormatInfo_Construct(info, d3d, adapter, At<uint32_t>(self, 0), mode[3], fullscreen,
                                                windowed);
                }
                AddTail(infos, reinterpret_cast<uintptr_t>(info));
            }
        }
    }
    At<uint8_t>(self, 0x144) = 0;
    for (ListNode* node = infos->head; node != nullptr; node = node->next) {
        uint8_t* info = reinterpret_cast<uint8_t*>(node->element);
        if (At<uint32_t>(info, 0) == desktopFormat) {
            if (At<uint8_t>(info, 4) != 0) {
                At<uint8_t>(self, 0x144) = 1;
            }
            break;
        }
    }
    return self;
}

// 0x8913E0 (thiscall): NiDX9AdapterDesc::ModeDesc's constructor (from a D3DDISPLAYMODE: its
// refresh rate the first of its list)
void* __fastcall NiDX9ModeDesc_Construct(uint8_t* self, re::Edx, const uint32_t* mode) {
    At<uint32_t>(self, 0) = mode[0];
    At<uint32_t>(self, 4) = mode[1];
    At<uint32_t>(self, 0xC) = mode[3];
    List* rates = reinterpret_cast<List*>(self + 0x10);
    rates->count = 0;
    rates->head = nullptr;
    rates->tail = nullptr;
    rates->vtable = reinterpret_cast<void*>(kRefreshListVtable);
    ListNode* node = NewNode(rates);
    node->element = mode[2];
    node->prev = nullptr;
    node->next = rates->head;
    if (rates->head != nullptr) {
        rates->head->prev = node;
    } else {
        rates->tail = node;
    }
    rates->count++;
    rates->head = node;
    At<uint32_t>(self, 8) = GetFormatBitsPerPixel(mode[3]);
    return self;
}

// 0x891460 (thiscall): NiDX9AdapterDesc's constructor (its identifier; its modes of each display
// format, a mode's refresh rates gathered; its HAL and reference devices)
void* __fastcall NiDX9AdapterDesc_Construct(uint8_t* self, re::Edx, void* d3d, uint32_t adapter, List* formats) {
    At<uint32_t>(self, 0) = adapter;
    NiTArray<void*>* modes = reinterpret_cast<NiTArray<void*>*>(self + 0x450);
    modes->vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    modes->maxSize = 0x20;
    modes->growBy = 0x10;
    modes->size = 0;
    modes->esize = 0;
    modes->base = static_cast<re::Ptr32<void>*>(NiMalloc(0x80));
    modes->vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    Com<long>(d3d, kGetAdapterIdentifier, At<uint32_t>(self, 0), 0u, self + 4);
    for (ListNode* node = formats->head; node != nullptr;) {
        uint32_t format = uint32_t(node->element);
        node = node->next;
        if (format == 0) {
            continue;
        }
        uint32_t count = Com<uint32_t>(d3d, kGetAdapterModeCount, At<uint32_t>(self, 0), format);
        for (uint32_t k = 0; k < count; k++) {
            uint32_t mode[4];   // D3DDISPLAYMODE
            if (Com<long>(d3d, kEnumAdapterModes, At<uint32_t>(self, 0), format, k, mode) < 0) {
                continue;
            }
            uint8_t* found = nullptr;
            for (uint32_t i = 0; i < modes->size; i++) {
                uint8_t* desc = static_cast<uint8_t*>(modes->base[i]);
                if (desc != nullptr && At<uint32_t>(desc, 0xC) == format && At<uint32_t>(desc, 0) == mode[0] &&
                    At<uint32_t>(desc, 4) == mode[1]) {
                    found = desc;
                    AddTail(reinterpret_cast<List*>(desc + 0x10), mode[2]);
                }
            }
            if (found != nullptr) {
                continue;
            }
            void* desc = NiNew(0x20);
            if (desc != nullptr) {
                desc = ModeDesc_Construct(desc, mode);
            }
            uint32_t index = modes->size;
            if (index >= modes->maxSize) {
                PrimitiveArray_SetSize(modes, modes->growBy + index);
            }
            ArraySetAt(modes, index, desc);
        }
    }
    uint32_t desktop[4];   // D3DDISPLAYMODE
    Com<long>(d3d, kGetAdapterDisplayMode, At<uint32_t>(self, 0), desktop);
    void* hal = NiNew(0x148);
    if (hal != nullptr) {
        hal = DeviceDesc_Construct(hal, d3d, adapter, 1, desktop[3], formats);
    }
    At<void*>(self, 0x460) = hal;
    void* reference = NiNew(0x148);
    if (reference != nullptr) {
        reference = DeviceDesc_Construct(reference, d3d, adapter, 2, desktop[3], formats);
    }
    At<void*>(self, 0x464) = reference;
    return self;
}

// 0x8916F0 (thiscall): NiDX9SystemDesc's enumeration (an adapter description for each)
void __fastcall NiDX9SystemDesc_Enumerate(uint8_t* self, re::Edx, void* d3d, List* formats) {
    uint32_t count = Com<uint32_t>(d3d, kGetAdapterCount);
    At<uint32_t>(self, 0) = count;
    NiTArray<void*>* adapters = reinterpret_cast<NiTArray<void*>*>(self + 4);
    PrimitiveArray_SetSize(adapters, count);
    for (uint32_t i = 0; i < At<uint32_t>(self, 0); i++) {
        void* desc = NiNew(0x468);
        if (desc != nullptr) {
            desc = AdapterDesc_Construct(desc, d3d, i, formats);
        }
        ArraySetAt(adapters, i, desc);
    }
}

// 0x891880 (thiscall): NiDX9SystemDesc's constructor
// 0x890DA0 (thiscall): DisplayFormatInfo's destructor: each render target's quality table
// (0x132 bytes) deleted, walking the map's items (the next bucket after an item's key), then the
// map emptied (as NiTPointerMap, then NiTMapBase) and its buckets freed
void __fastcall NiDX9DisplayFormatInfo_Destruct(uint8_t* self, re::Edx) {
    uint8_t* map = self + 8;
    uint32_t hashSize = At<uint32_t>(map, 4);
    re::Ptr32<uint8_t>* buckets = At<re::Ptr32<uint8_t>*>(map, 8);
    uint8_t* item = nullptr;
    for (uint32_t i = 0; i < hashSize; i++) {
        if (buckets[i] != nullptr) {
            item = buckets[i];
            break;
        }
    }
    while (item != nullptr) {
        uint8_t* table = At<uint8_t*>(item, 8);
        uint8_t* next = At<uint8_t*>(item, 0);
        if (next == nullptr) {
            uint32_t size = At<uint32_t>(map, 4);
            for (uint32_t i = At<uint32_t>(item, 4) % size + 1; i < size; i++) {
                if (At<re::Ptr32<uint8_t>*>(map, 8)[i] != nullptr) {
                    next = At<re::Ptr32<uint8_t>*>(map, 8)[i];
                    break;
                }
            }
        }
        NiDelete(table, 0x132);
        item = next;
    }
    At<uint32_t>(map, 0) = kPointerMapVtable;
    TMapBase_RemoveAll(map);
    At<uint32_t>(map, 0) = kMapBaseVtable;
    TMapBase_RemoveAll(map);
    NiFree(At<void*>(map, 8));
}

// 0x891390 (thiscall): NiDX9DeviceDesc's destructor: its display formats deleted, the list
// emptied
void __fastcall NiDX9DeviceDesc_Destruct(uint8_t* self, re::Edx) {
    for (ListNode* node = At<ListNode*>(self, 0x138); node != nullptr;) {
        uint8_t* info = reinterpret_cast<uint8_t*>(uintptr_t(node->element));
        node = node->next;
        if (info != nullptr) {
            FormatInfo_Destruct(info);
            NiDelete(info, 0x18);
        }
    }
    At<uint32_t>(self, 0x134) = kFormatListVtable;
    PointerList_RemoveAll(self + 0x134);
    At<uint32_t>(self, 0x134) = kListBaseVtable;
}

// 0x891790 (thiscall): NiDX9AdapterDesc's destructor: each mode's refresh rates freed (the
// list's slot 2 a node; the list's vtable the compiler's shared NiTPointerList one) and the mode
// deleted, the two devices deleted, the modes' array freed
void __fastcall NiDX9AdapterDesc_Destruct(uint8_t* self, re::Edx) {
    NiTArray<void*>* modes = reinterpret_cast<NiTArray<void*>*>(self + 0x450);
    for (uint32_t i = 0; i < modes->size; i++) {
        uint8_t* mode = static_cast<uint8_t*>(static_cast<void*>(modes->base[i]));
        if (mode == nullptr) {
            continue;
        }
        List* rates = reinterpret_cast<List*>(mode + 0x10);
        rates->vtable = reinterpret_cast<void*>(kFormatListVtable);
        for (ListNode* node = rates->head; node != nullptr;) {
            ListNode* item = node;
            node = node->next;
            Virtual<void>(rates, 2, item);
        }
        rates->count = 0;
        rates->head = nullptr;
        rates->tail = nullptr;
        rates->vtable = reinterpret_cast<void*>(kListBaseVtable);
        NiDelete(mode, 0x20);
    }
    const int devices[2] = {0x460, 0x464};   // the HAL device, the reference one
    for (int offset : devices) {
        if (uint8_t* device = At<uint8_t*>(self, offset)) {
            DeviceDesc_Destruct(device);
            NiDelete(device, 0x148);
        }
    }
    modes->vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    NiFree(modes->base);
}

// 0x8918C0 (thiscall): NiDX9SystemDesc's destructor: its adapters deleted, their array freed
void __fastcall NiDX9SystemDesc_Destruct(uint8_t* self, re::Edx) {
    NiTArray<void*>* adapters = reinterpret_cast<NiTArray<void*>*>(self + 4);
    for (uint32_t i = 0; i < adapters->size; i++) {
        uint8_t* adapter = static_cast<uint8_t*>(static_cast<void*>(adapters->base[i]));
        if (adapter != nullptr) {
            AdapterDesc_Destruct(adapter);
            NiDelete(adapter, 0x468);
        }
    }
    adapters->vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    NiFree(adapters->base);
}

void* __fastcall NiDX9SystemDesc_Construct(uint8_t* self, re::Edx, void* d3d, List* formats) {
    NiTArray<void*>* adapters = reinterpret_cast<NiTArray<void*>*>(self + 4);
    adapters->maxSize = 0;
    adapters->size = 0;
    adapters->esize = 0;
    adapters->base = nullptr;
    adapters->growBy = 1;
    adapters->vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    At<uint32_t>(self, 0) = 0;
    SystemDesc_Enumerate(self, d3d, formats);
    return self;
}

#define C40B_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40B_REVERSED("NiDX9Renderer::GetDepthStencilBits", 0x008903B0, NiDX9_GetDepthStencilBits);
// (regcheck2: the game's 0x890E50 and 0x890ED0 keep edx across their calls to 0x8904A0: turn it on
// together with those two)
C40B_REVERSED("NiDX9DeviceDesc::GetDepthStencilIndex", 0x008904A0, NiDX9_GetDepthStencilIndex);
C40B_REVERSED("NiDX9Renderer::GetD3DFormatString", 0x00890530, NiDX9_GetD3DFormatString);
// (regcheck2: the game's 0x8909A0 keeps ecx and edx across its calls to 0x890840: turn it on
// together with 0x8909A0)
C40B_REVERSED("NiDX9Renderer::GetFormatBitsPerPixel", 0x00890840, NiDX9_GetFormatBitsPerPixel);
C40B_REVERSED("NiTListBase::DeletingDestructor_890970", 0x00890970, NiTListBase_DeletingDestructor);
C40B_REVERSED("NiDX9DeviceDesc::GetDefaultFrameBufferFormat", 0x008909A0, NiDX9DeviceDesc_GetDefaultFrameBufferFormat);
C40B_REVERSED("NiDX9AdapterDesc::ModeDesc::FindClosestRefreshRate", 0x00890A30, NiDX9ModeDesc_FindClosestRefreshRate);
C40B_REVERSED("NiDX9DeviceDesc::DisplayFormatInfo::DisplayFormatInfo", 0x00890A90, NiDX9DisplayFormatInfo_Construct);
C40B_REVERSED("NiDX9DeviceDesc::DisplayFormatInfo::GetMultiSampleQualityLevels", 0x00890E50,
              NiDX9DisplayFormatInfo_GetMultiSampleQualityLevels);
C40B_REVERSED("NiDX9DeviceDesc::DisplayFormatInfo::IsMultiSampleValid", 0x00890ED0,
              NiDX9DisplayFormatInfo_IsMultiSampleValid);
C40B_REVERSED("NiDX9DeviceDesc::DisplayFormatInfo::FindClosestDepthStencil", 0x00890F40,
              NiDX9DisplayFormatInfo_FindClosestDepthStencil);
C40B_REVERSED("NiDX9DeviceDesc::FindClosestDepthStencilFormat", 0x00891040,
              NiDX9DeviceDesc_FindClosestDepthStencilFormat);
C40B_REVERSED("NiDX9AdapterDesc::ValidateFullscreenMode", 0x008910E0, NiDX9AdapterDesc_ValidateFullscreenMode);
C40B_REVERSED("NiDX9DeviceDesc::DisplayFormatInfo::FindClosestDepthStencilOwn", 0x00891150,
              NiDX9DisplayFormatInfo_FindClosestDepthStencilOwn);
C40B_REVERSED("NiDX9DeviceDesc::NiDX9DeviceDesc", 0x00891170, NiDX9DeviceDesc_Construct);
C40B_REVERSED("NiDX9AdapterDesc::ModeDesc::ModeDesc", 0x008913E0, NiDX9ModeDesc_Construct);
C40B_REVERSED("NiDX9AdapterDesc::NiDX9AdapterDesc", 0x00891460, NiDX9AdapterDesc_Construct);
C40B_REVERSED("NiDX9SystemDesc::Enumerate", 0x008916F0, NiDX9SystemDesc_Enumerate);
C40B_REVERSED("NiDX9SystemDesc::NiDX9SystemDesc", 0x00891880, NiDX9SystemDesc_Construct);
C40B_REVERSED("NiDX9DeviceDesc::DisplayFormatInfo::~DisplayFormatInfo", 0x00890DA0, NiDX9DisplayFormatInfo_Destruct);
C40B_REVERSED("NiDX9DeviceDesc::~NiDX9DeviceDesc", 0x00891390, NiDX9DeviceDesc_Destruct);
C40B_REVERSED("NiDX9AdapterDesc::~NiDX9AdapterDesc", 0x00891790, NiDX9AdapterDesc_Destruct);
C40B_REVERSED("NiDX9SystemDesc::~NiDX9SystemDesc", 0x008918C0, NiDX9SystemDesc_Destruct);

} // namespace ni::c40
