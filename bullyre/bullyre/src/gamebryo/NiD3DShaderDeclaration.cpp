// NiD3DShaderDeclaration (vtable 0x99BEB8, 29 slots; NiShaderDeclaration's RTTI; 0x38 bytes): the
// DX9 renderer's NiShaderDeclaration. The D3D vertex declaration made from the entries when they
// changed (slot 28), the strides per stream, and the packing of a geometry's data into its vertex
// buffers by the entries (slot 26, through the packing functions by D3D type and source type).
//
// Layout past NiShaderDeclaration's 0x18 (+0x8 entries a stream, +0xC streams, +0x10 the
// streams (0x10 each: +0x4 their entries (0x18 each: +0x0 the packing offset, +0x4 the input
// register (-1 none), +0x8 the type (0x11 unused), +0xC the usage name, +0x10 the usage index,
// +0x14 the method), +0x8 the stride, +0xC used), +0x14 modified): +0x18 the renderer, +0x1C the
// vertex buffer manager, +0x20 the device, +0x24 the element capacity, +0x28 the elements, +0x2C
// the D3DVERTEXELEMENT9 array (the game's operator new), +0x30 the IDirect3DVertexDeclaration9,
// +0x34 software vertex processing.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kDeclarationVtable = 0x0099BEB8;
constexpr uint32_t kDeclarationSize = 0x38;

GAME_ARRAY(const uint32_t, ms_akD3DTypes, 0x00BCB828);     // D3DDECLTYPE by type (4 bytes each)
GAME_ARRAY(const uint32_t, ms_akD3DMethods, 0x00BCB870);   // D3DDECLMETHOD by method
GAME_ARRAY(const uint32_t, ms_akD3DUsages, 0x00BCB88C);    // D3DDECLUSAGE by usage name
GAME_ARRAY(const uint32_t, ms_auiTypeSizes, 0x020E7438);   // bytes by type (0x12)
GAME_ARRAY(NiFixedString, ms_akUsageNames, 0x020E31CC);    // POSITION, BLENDWEIGHT, ... (14)
GAME_VAR(re::Ptr32<uint8_t>*, ms_ppkPackingFunctions, 0x020E7480);   // (+4: the arrays by D3D type)

GAME_FN(void*, __thiscall, ShaderDeclaration_ConstructBase, 0x008A01D0, void* self);
GAME_FN(void, __thiscall, ShaderDeclaration_DestructBase, 0x008A1220, void* self);
GAME_FN(uint8_t, __thiscall, ShaderDeclaration_Initialize, 0x008A1250, void* self, void* renderer, uint32_t entries,
        uint32_t streams);
GAME_FN(void, __thiscall, ShaderDeclaration_PackEntry, 0x008A1910, void* self, uint32_t vertices, void* data,
        void* skin, void* partition, void* entry, uint32_t stride, void* dest);
// the binormals and tangents packed with the positions and normals (stdcall)
GAME_FN(void, __thiscall, CalculateBinormalsAndTangents, 0x007D48C0, void* skin, void* vertices, void* normals, uint32_t count,
        void* positionDest, void* normalDest, void* binormalDest, void* tangentDest, uint32_t stride);
GAME_FN(void, __thiscall, BufferData_SetVertexDeclaration, 0x00897040, void* self, void* declaration);
GAME_FN(uint8_t*, __thiscall, BufferData_GetVBChip, 0x00878550, void* self, uint32_t stream);
GAME_FN(uint8_t, __thiscall, VBManager_CreateVertexBuffer, 0x00887230, void* self, void* bufferData,
        uint32_t stream);
GAME_FN(uint8_t*, __thiscall, VBManager_LockVB, 0x00887280, void* self, void* vb, uint32_t offset, uint32_t size,
        uint32_t flags);
GAME_FN(uint8_t, __thiscall, VBManager_UnlockVB, 0x00887360, void* self, void* vb);
GAME_FN(void, __thiscall, FunctionArray_SetAt, 0x00833C70, void* array, uint32_t index, const uintptr_t* value);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);   // the game's operator new
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);           // and delete

// this file's (called where modes apply)
GAME_FN(uint32_t, __thiscall, Declaration_AddVertexElement, 0x00886240, void* self, void* entry, uint32_t stream);

constexpr uintptr_t kPackNothing = 0x00824EC0;   // (returns 0)

} // namespace

// 0x884C20 (thiscall, slot 21): NiD3DShaderDeclaration::MarkAsModified (the D3D declaration let
// go)
void __fastcall Declaration_MarkAsModified(uint8_t* self, re::Edx) {
    void* declaration = At<void*>(self, 0x30);
    At<uint8_t>(self, 0x14) = 1;
    if (declaration != nullptr) {
        Com<long>(declaration, kRelease);
        At<void*>(self, 0x30) = nullptr;
    }
}

// 0x884C40 (thiscall, slot 23): NiD3DShaderDeclaration::GetVertexStride (the declaration made
// first)
uint32_t __fastcall Declaration_GetVertexStride(uint8_t* self, re::Edx, uint32_t stream) {
    if (stream >= At<uint32_t>(self, 0xC)) {
        return 0;
    }
    Virtual<void*>(self, 28);
    return At<uint32_t>(At<uint8_t*>(self, 0x10), int(stream) * 0x10 + 8);
}

// 0x884C70 (thiscall, slot 24): NiD3DShaderDeclaration::SetSoftwareVertexProcessing (the same
// code serves other classes' one-byte setters at +0x34)
void __fastcall Declaration_SetSoftwareVertexProcessing(uint8_t* self, re::Edx, int software) {
    At<uint8_t>(self, 0x34) = uint8_t(software);
}

// 0x884C80 (thiscall, slot 25): NiD3DShaderDeclaration::GetSoftwareVertexProcessing (4 bytes: not
// hooked)
uint8_t __fastcall Declaration_GetSoftwareVertexProcessing(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, 0x34);
}
RE_UNHOOKED_FN("NiD3DShaderDeclaration::GetSoftwareVertexProcessing", 0x00884C80, &Declaration_GetSoftwareVertexProcessing);

// 0x884C90 (thiscall, slot 27): the D3D declaration (made when modified) and the stream count;
// whether there's a declaration
uint8_t __fastcall Declaration_GetD3DDeclarationAndStreams(uint8_t* self, re::Edx, re::Ptr32<void>* declaration,
                                                           uint32_t* streams) {
    *declaration = Virtual<void*>(self, 28);
    *streams = At<uint32_t>(self, 0xC);
    return *declaration != nullptr ? 1 : 0;
}

// 0x884CC0: a D3D usage's register, type, usage name and method (POSITION, BLENDWEIGHT,
// BLENDINDICES, NORMAL, TEXCOORD 0..7, TANGENT, BINORMAL, COLOR); false for the others
uint8_t __cdecl Declaration_FromD3DUsage(uint32_t usage, uint32_t usageIndex, uint32_t* registerIndex,
                                         uint32_t* type, uint32_t* usageName, uint32_t* method) {
    uint32_t r, t;
    switch (usage) {
    case 0: r = 0; t = 2; break;
    case 1: r = 1; t = 2; break;
    case 2: r = 2; t = 4; break;
    case 3: r = 3; t = 2; break;
    case 5:
        if (usageIndex >= 8) {
            return 0;
        }
        r = usageIndex + 5;
        t = 1;
        break;
    case 6: r = 0xD; t = 2; break;
    case 7: r = 0xE; t = 2; break;
    case 10: r = 4; t = 4; break;
    default:
        return 0;
    }
    *registerIndex = r;
    *type = t;
    *usageName = usage;
    *method = 0;
    return 1;
}

// 0x885E40 (stdcall, ecx passed on): the binormals and tangents of a geometry data's vertices packed
// with its positions and normals (NiSkinInstance::Deform on the skin instance its caller has in ecx)
void __fastcall Declaration_PackBinormalsAndTangents(void* skin, re::Edx, uint8_t* data, void* positionDest, void* normalDest,
                                                     void* binormalDest, void* tangentDest, uint32_t stride) {
    CalculateBinormalsAndTangents(skin, At<void*>(data, 0x20), At<void*>(data, 0x24), At<uint16_t>(data, 0xC), positionDest,
                                  normalDest, binormalDest, tangentDest, stride);
}

// 0x885E80: NiD3DShaderDeclaration::Create (not past the device's streams; deleted when it can't
// be initialized)
void* __cdecl Declaration_Create(uint8_t* renderer, uint32_t entries, uint32_t streams) {
    if (streams > At<uint32_t>(renderer, kMaxStreams)) {
        return nullptr;
    }
    uint8_t* declaration = static_cast<uint8_t*>(NiNew(kDeclarationSize));
    if (declaration == nullptr) {
        return nullptr;
    }
    ShaderDeclaration_ConstructBase(declaration);
    At<uint32_t>(declaration, 0) = kDeclarationVtable;
    At<void*>(declaration, 0x2C) = nullptr;
    At<void*>(declaration, 0x30) = nullptr;
    At<uint8_t>(declaration, 0x34) = 0;
    if (ShaderDeclaration_Initialize(declaration, renderer, entries, streams)) {
        return declaration;
    }
    Virtual<void>(declaration, 0, 1);
    return nullptr;
}

// 0x885EF0 (thiscall, slot 0): NiD3DShaderDeclaration's deleting destructor
uint8_t* __fastcall Declaration_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    void* elements = At<void*>(self, 0x2C);
    At<uint32_t>(self, 0) = kDeclarationVtable;
    Free(elements);
    if (At<void*>(self, 0x30) != nullptr) {
        Com<long>(At<void*>(self, 0x30), kRelease);
        At<void*>(self, 0x30) = nullptr;
    }
    ShaderDeclaration_DestructBase(self);
    if (flags & 1) {
        NiDelete(self, kDeclarationSize);
    }
    return self;
}

// 0x885F40 (thiscall, slot 26): NiD3DShaderDeclaration::PackUntransformedVB (a stream of the data
// (or a partition's) packed by the entries into its vertex buffer: the declaration and stride set
// on the buffer data; the given chip reused when it's a big enough vertex-data buffer of static
// data, else a new vertex buffer and every part packed; locked unless given the locked bits;
// binormals and tangents packed together when skinned without a partition). Returns the chip, or
// null. (`parts`: 1 positions, 2 normals and the basis, 4 colors, 8 texture coordinates.)
uint8_t* __fastcall Declaration_PackUntransformedVB(uint8_t* self, re::Edx, uint8_t* data, void* skin, uint8_t* partition,
                                                    uint32_t parts, uint8_t* chip, uint32_t stream, uint8_t* locked) {
    uint32_t vertices = partition != nullptr ? At<uint16_t>(partition, 0x1C) : At<uint16_t>(data, 0x30);
    if (uint16_t(vertices) == 0) {
        return chip;
    }
    void* declaration = nullptr;
    uint32_t streams = 0;
    if (!Virtual<uint8_t>(self, 27, &declaration, &streams)) {
        return nullptr;
    }
    uint8_t* bufferData = partition != nullptr ? At<uint8_t*>(partition, 0x28) : At<uint8_t*>(data, 0x3C);
    BufferData_SetVertexDeclaration(bufferData, declaration);
    if (stream < At<uint32_t>(bufferData, 0x20)) {
        At<uint32_t*>(bufferData, 0x24)[stream] = At<uint32_t>(At<uint8_t*>(self, 0x10), int(stream) * 0x10 + 8);
    }
    if (At<uint8_t>(At<uint8_t*>(self, 0x18), kSWVertexCapable) != 0) {
        At<uint8_t>(bufferData, 0x14) = Virtual<uint8_t>(self, 25);
    }
    bool unlock = true;
    if (locked != nullptr) {
        unlock = false;
    } else {
        bool reuse = false;
        if (chip != nullptr && At<void*>(chip, 8) != nullptr && (At<uint16_t>(data, 0x34) & 0xF000) != 0x8000) {
            uint32_t desc[6];   // D3DVERTEXBUFFER_DESC: Format, Type, Usage, Pool, Size, FVF
            Com<long>(At<void*>(chip, 8), 13, desc);   // GetDesc
            if (desc[0] == 100 &&   // D3DFMT_VERTEXDATA
                desc[4] >= Virtual<uint32_t>(self, 23, stream) * uint16_t(vertices)) {
                reuse = true;
            }
        }
        if (!reuse) {
            if (!VBManager_CreateVertexBuffer(At<void*>(self, 0x1C), bufferData, stream)) {
                return nullptr;
            }
            chip = BufferData_GetVBChip(bufferData, stream);
            parts = 0xFFF;
        }
        locked = VBManager_LockVB(At<void*>(self, 0x1C), At<void*>(chip, 8), At<uint32_t>(chip, 0xC),
                                  At<uint32_t>(chip, 0x14), At<uint32_t>(chip, 0x10));
    }
    bool basis = skin != nullptr && partition == nullptr;
    uint8_t* streamEntry = At<uint8_t*>(self, 0x10) + stream * 0x10;
    void* positions = nullptr;
    void* normals = nullptr;
    void* tangents = nullptr;
    void* binormals = nullptr;
    if (streamEntry != nullptr) {
        for (uint32_t i = 0; i < At<uint32_t>(self, 8); i++) {
            uint8_t* entry = At<uint8_t*>(streamEntry, 4) + i * 0x18;
            uint8_t* dest = locked + At<uint32_t>(entry, 0);
            bool pack;
            switch (At<uint32_t>(entry, 4)) {   // the input register
            case 0:   // position
                if (!(parts & 1)) {
                    continue;
                }
                pack = !basis;
                if (basis) {
                    positions = dest;
                }
                break;
            case 3:   // normal
                if (!(parts & 2)) {
                    continue;
                }
                pack = !basis;
                if (basis) {
                    normals = dest;
                }
                break;
            case 14:   // tangent
                if (!(parts & 2)) {
                    continue;
                }
                pack = !basis;
                if (basis) {
                    tangents = dest;
                }
                break;
            case 13:   // binormal
                if (!(parts & 2)) {
                    continue;
                }
                pack = !basis;
                if (basis) {
                    binormals = dest;
                }
                break;
            case 4:   // color
                pack = (parts & 4) != 0;
                break;
            case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12:   // texture coordinates
                pack = (parts & 8) != 0;
                break;
            default:
                pack = true;
                break;
            }
            if (pack) {
                ShaderDeclaration_PackEntry(self, vertices, data, skin, partition, entry, At<uint32_t>(streamEntry, 8),
                                            dest);
            }
        }
    }
    if (basis) {
        CalculateBinormalsAndTangents(skin, At<void*>(data, 0x20), At<void*>(data, 0x24), At<uint16_t>(data, 0xC), positions,
                                      normals, tangents, binormals, At<uint32_t>(streamEntry, 8));
    }
    if (unlock && !VBManager_UnlockVB(At<void*>(self, 0x1C), At<void*>(chip, 8))) {
        return nullptr;
    }
    return chip;
}

// 0x886240 (thiscall): an entry's D3DVERTEXELEMENT9 added (the stream, its packing offset, the D3D
// type, method and usage, the usage index); the type's size
uint32_t __fastcall Declaration_AddVertexElementImpl(uint8_t* self, re::Edx, uint8_t* entry, uint32_t stream) {
    uint8_t* elements = At<uint8_t*>(self, 0x2C);
    At<uint16_t>(elements, At<int32_t>(self, 0x28) * 8) = uint16_t(stream);
    At<uint16_t>(At<uint8_t*>(self, 0x2C), At<int32_t>(self, 0x28) * 8 + 2) = At<uint16_t>(entry, 0);
    At<uint8_t>(At<uint8_t*>(self, 0x2C), At<int32_t>(self, 0x28) * 8 + 4) =
        uint8_t(ms_akD3DTypes[At<uint32_t>(entry, 8)]);
    At<uint8_t>(At<uint8_t*>(self, 0x2C), At<int32_t>(self, 0x28) * 8 + 5) =
        uint8_t(ms_akD3DMethods[At<uint32_t>(entry, 0x14)]);
    uint32_t usage = 0;
    for (uint32_t i = 0; i < 0xE; i++) {
        if (ms_akUsageNames[i] == At<NiFixedString>(entry, 0xC)) {
            usage = i;
            break;
        }
    }
    At<uint8_t>(At<uint8_t*>(self, 0x2C), At<int32_t>(self, 0x28) * 8 + 6) = uint8_t(ms_akD3DUsages[usage]);
    At<uint8_t>(At<uint8_t*>(self, 0x2C), At<int32_t>(self, 0x28) * 8 + 7) = At<uint8_t>(entry, 0x10);
    At<int32_t>(self, 0x28)++;
    if (At<int32_t>(entry, 8) < 0x12) {
        return ms_auiTypeSizes[At<int32_t>(entry, 8)];
    }
    return 0;
}

// 0x8862F0: the packing function for a D3D type from a source type set in the type's array (the
// float, color, UBYTE4 and SHORT2/4 converters; 0 a null packer)
void __cdecl Declaration_SetPackingFunction(uint32_t d3dType, uint32_t sourceType) {
    uint8_t* array = At<re::Ptr32<uint8_t>*>(ms_ppkPackingFunctions, 4)[d3dType] + 4;
    if (sourceType == 0) {
        uintptr_t fn = kPackNothing;
        FunctionArray_SetAt(array, 0, &fn);
        return;
    }
    if (sourceType - 1 > 0x1F) {
        return;
    }
    uint32_t group = (sourceType - 1) / 4;   // 0 float, 1/2 int, 3 short, 4/5 ..., 6 ..., 7 none
    uintptr_t fn = 0;
    if (d3dType <= 3) {   // FLOAT1..FLOAT4
        static const uintptr_t kFloat[8] = {0, 0x008A0210, 0x008A0420, 0x008A05F0, 0x008A0820, 0x008A09F0, 0x008B1580,
                                            0x008A1430};
        if (group == 0) {
            uint32_t components = sourceType - 1;
            fn = components == d3dType ? 0x008A1580 : components < d3dType ? 0x008A1430 : 0x008A1350;
        } else {
            fn = kFloat[group];
        }
    } else if (d3dType == 4) {   // D3DCOLOR
        static const uintptr_t kColor[32] = {
            kPackNothing, kPackNothing, 0x008A1670, 0x008A1670, 0x008A0C20, kPackNothing, kPackNothing, kPackNothing,
            0x008A0C20, kPackNothing, kPackNothing, kPackNothing, kPackNothing, kPackNothing, kPackNothing, 0x008A0CC0,
            kPackNothing, kPackNothing, kPackNothing, 0x008A0E10, 0x008A0F70, 0x008A0F70, 0x008A0F70, 0x008A0F70,
            0x008A10C0, 0x008A10C0, 0x008A10C0, 0x008A10C0, 0x008A1670, 0x008A1670, 0x008A1670, 0x008A1670};
        fn = kColor[sourceType - 1];
    } else if (d3dType == 6 || d3dType == 7) {   // SHORT2, SHORT4
        static const uintptr_t kShort[8] = {0x00884E70, 0x00885050, 0x00885210, 0x008853D0, 0x008B24A0, 0x00885570,
                                            0x008B2660, kPackNothing};
        fn = kShort[group];
    } else if (d3dType == 5) {   // UBYTE4
        static const uintptr_t kUByte[8] = {0x00885740, 0x00885930, 0x00885AE0, 0x008B2180, 0x00885C90, 0x008B2830,
                                            0x008B29E0, kPackNothing};
        fn = kUByte[group];
    } else {
        return;
    }
    if (fn != 0) {
        FunctionArray_SetAt(array, sourceType, &fn);
    }
}

// 0x8865D0 (thiscall, slot 28): NiD3DShaderDeclaration::GetD3DDeclaration (made again when
// modified: the streams that use an entry, an element per used entry and the end, offsets packed
// in order, the strides; CreateVertexDeclaration)
void* __fastcall Declaration_GetD3DDeclaration(uint8_t* self, re::Edx) {
    void* declaration = At<void*>(self, 0x30);
    if (At<uint8_t>(self, 0x14) == 0) {
        return declaration;
    }
    if (declaration != nullptr) {
        Com<long>(declaration, kRelease);
        At<void*>(self, 0x30) = nullptr;
    }
    uint32_t used = 0;
    for (uint32_t s = 0; s < At<uint32_t>(self, 0xC); s++) {
        uint8_t* stream = At<uint8_t*>(self, 0x10) + s * 0x10;
        if (stream == nullptr) {
            continue;
        }
        At<uint8_t>(stream, 0xC) = 0;
        for (uint32_t i = 0; i < At<uint32_t>(self, 8); i++) {
            uint8_t* entry = At<uint8_t*>(stream, 4) + i * 0x18;
            if (At<int32_t>(entry, 4) != -1 && At<uint32_t>(entry, 8) != 0x11) {
                used++;
                At<uint8_t>(stream, 0xC) = 1;
            }
        }
    }
    if (At<uint32_t>(self, 0xC) != 0 && used != 0) {
        used++;   // (the end element)
    }
    if (At<uint32_t>(self, 0x24) < used) {
        Free(At<void*>(self, 0x2C));
        At<void*>(self, 0x2C) = nullptr;
    }
    if (used == 0) {
        At<uint32_t>(self, 0x24) = 0;
        return nullptr;
    }
    if (At<void*>(self, 0x2C) == nullptr) {
        At<uint32_t>(self, 0x24) = used;
        uint64_t bytes = uint64_t(used) * 8;
        At<void*>(self, 0x2C) = Malloc((bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes));
    }
    At<uint32_t>(self, 0x28) = 0;
    for (uint32_t s = 0; s < At<uint32_t>(self, 0xC); s++) {
        uint8_t* stream = At<uint8_t*>(self, 0x10) + s * 0x10;
        if (At<uint8_t>(stream, 0xC) == 0) {
            continue;
        }
        uint32_t offset = 0;
        for (uint32_t i = 0; i < At<uint32_t>(self, 8); i++) {
            uint8_t* entry = At<uint8_t*>(stream, 4) + i * 0x18;
            if (At<int32_t>(entry, 4) != -1 && At<uint32_t>(entry, 8) != 0x11) {
                At<uint32_t>(entry, 0) = offset;
                offset += Declaration_AddVertexElement(self, entry, s);
            }
        }
        At<uint32_t>(stream, 8) = offset;
    }
    uint8_t* end = At<uint8_t*>(self, 0x2C) + At<int32_t>(self, 0x28) * 8;   // D3DDECL_END
    At<uint16_t>(end, 0) = 0xFF;
    At<uint16_t>(end, 2) = 0;
    At<uint8_t>(end, 4) = 0x11;
    At<uint8_t>(end, 5) = 0;
    At<uint8_t>(end, 6) = 0;
    At<uint8_t>(end, 7) = 0;
    if (Com<long>(At<void*>(self, 0x20), 86, At<void*>(self, 0x2C), &At<void*>(self, 0x30)) >= 0) {   // CreateVertexDeclaration
        At<uint8_t>(self, 0x14) = 0;
        return At<void*>(self, 0x30);
    }
    return nullptr;
}

#define C39F_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39F_REVERSED("NiD3DShaderDeclaration::MarkAsModified", 0x00884C20, Declaration_MarkAsModified);
C39F_REVERSED("NiD3DShaderDeclaration::GetVertexStride", 0x00884C40, Declaration_GetVertexStride);
C39F_REVERSED("NiD3DShaderDeclaration::SetSoftwareVertexProcessing", 0x00884C70,
              Declaration_SetSoftwareVertexProcessing);
C39F_REVERSED("NiD3DShaderDeclaration::GetD3DDeclarationAndStreams", 0x00884C90,
              Declaration_GetD3DDeclarationAndStreams);
C39F_REVERSED("NiD3DShaderDeclaration::FromD3DUsage", 0x00884CC0, Declaration_FromD3DUsage);
C39F_REVERSED("NiD3DShaderDeclaration::PackBinormalsAndTangents", 0x00885E40, Declaration_PackBinormalsAndTangents);
C39F_REVERSED("NiD3DShaderDeclaration::Create", 0x00885E80, Declaration_Create);
C39F_REVERSED("NiD3DShaderDeclaration::DeletingDestructor", 0x00885EF0, Declaration_DeletingDestructor);
C39F_REVERSED("NiD3DShaderDeclaration::PackUntransformedVB", 0x00885F40, Declaration_PackUntransformedVB);
C39F_REVERSED("NiD3DShaderDeclaration::AddVertexElement", 0x00886240, Declaration_AddVertexElementImpl);
C39F_REVERSED("NiD3DShaderDeclaration::SetPackingFunction", 0x008862F0, Declaration_SetPackingFunction);
C39F_REVERSED("NiD3DShaderDeclaration::GetD3DDeclaration", 0x008865D0, Declaration_GetD3DDeclaration);

} // namespace ni::c39
