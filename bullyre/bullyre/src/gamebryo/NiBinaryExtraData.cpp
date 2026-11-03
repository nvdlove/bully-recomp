// NiBinaryExtraData (vtable 0x9179A4, 0x14 bytes): extra data holding a NiMalloc'd block of bytes
// (+0xC) and its size (+0x10). Cloning copies the block, streaming writes it as single bytes,
// comparison is by class and contents (not through NiExtraData's).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiExtraData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiBinaryExtraData : NiExtraData {
    re::Ptr32<uint8_t> data;     // +0xC
    uint32_t size;     // +0x10
};
static_assert(offsetof(NiBinaryExtraData, data) == 0xC);
static_assert(sizeof(NiBinaryExtraData) == 0x14);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBinaryExtraData_RTTI, 0x020E2EC4);
constexpr uintptr_t NiBinaryExtraData_vtable = 0x009179A4;

// NiExtraData's (another chunk's)
GAME_FN(void, __thiscall, ExtraData_SaveBinary, 0x007655B0, void* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_LoadBinary, 0x007655D0, void* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, ExtraData_CopyMembers, 0x007656C0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, ExtraData_GetViewerStrings, 0x00765780, void* self, NiTArray<char*>* strings);

// the block through the binary streams as bytes (one component of size 1: never swapped)
inline void LoadByteBlock(void* stream, void* buffer, uint32_t size) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t componentSize = 1;
    in->position += in->read(in, buffer, size, &componentSize, 1);
}
inline void SaveByteBlock(void* stream, const void* buffer, uint32_t size) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t componentSize = 1;
    out->position += out->write(out, buffer, size, &componentSize, 1);
}

// vf3 CreateClone: the constructor inlined, the block copied (a failed allocation isn't checked)
NiBinaryExtraData* __fastcall CreateClone(NiBinaryExtraData* self, re::Edx, void* cloning) {
    NiBinaryExtraData* clone = static_cast<NiBinaryExtraData*>(NiNew(0x14));
    if (clone != nullptr) {
        ExtraData_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiBinaryExtraData_vtable);
        clone->size = 0;
        clone->data = nullptr;
    } else {
        clone = nullptr;
    }
    ExtraData_CopyMembers(self, clone, cloning);
    if (self->size != 0) {
        clone->data = static_cast<uint8_t*>(NiMalloc(self->size));
        crt::MemcpyS(clone->data, self->size, self->data, self->size);
        clone->size = self->size;
        return clone;
    }
    clone->size = 0;
    clone->data = nullptr;
    return clone;
}

void __fastcall LoadBinary(NiBinaryExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    LoadBytes(stream, &self->size, 4);
    if (self->size != 0) {
        self->data = static_cast<uint8_t*>(NiMalloc(self->size));
        LoadByteBlock(stream, self->data, self->size);
    }
}

void __fastcall SaveBinary(NiBinaryExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    SaveBytes(stream, &self->size, 4);
    uint32_t size = self->size;
    if (size != 0) {
        SaveByteBlock(stream, self->data, size);
    }
}

// vf8: another binary extra data (its RTTI exactly) with the same bytes
uint8_t __fastcall IsEqual(NiBinaryExtraData* self, re::Edx, NiBinaryExtraData* other) {
    if (other == nullptr || Virtual<const RTTI*>(other, 2) != &NiBinaryExtraData_RTTI) {
        return 0;
    }
    uint32_t size = self->size;
    if (size != other->size) {
        return 0;
    }
    return std::memcmp(self->data, other->data, size) == 0 ? 1 : 0;
}

void __fastcall GetViewerStrings(NiBinaryExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBinaryExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiSize", self->size));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIBINEXTRA_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIBINEXTRA_REVERSED("NiBinaryExtraData::CreateClone", 0x00777EC0, &ni::CreateClone);
NIBINEXTRA_REVERSED("NiBinaryExtraData::LoadBinary", 0x00777F40, &ni::LoadBinary);
NIBINEXTRA_REVERSED("NiBinaryExtraData::SaveBinary", 0x00777FC0, &ni::SaveBinary);
NIBINEXTRA_REVERSED("NiBinaryExtraData::IsEqual", 0x00778030, &ni::IsEqual);
NIBINEXTRA_REVERSED("NiBinaryExtraData::GetViewerStrings", 0x00778100, &ni::GetViewerStrings);
