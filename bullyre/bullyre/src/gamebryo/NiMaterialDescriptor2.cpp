// NiMaterialDescriptor's constructor, operator== and operator= (0x14 bytes: its identifier, an
// NiFixedString, then four 32-bit bit fields; NiMaterialDescriptor.cpp has the bit fields).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {
namespace {

struct Descriptor {
    re::Ptr32<char> identifier;   // +0x0 (NiFixedString)
    uint32_t bits[4];   // +0x4
};
static_assert(sizeof(Descriptor) == 0x14);

GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);

// 0x7D5440 operator==: the identifier (by handle) and the four words
uint8_t __fastcall Descriptor_EqualsImpl(const Descriptor* self, re::Edx, const Descriptor* other) {
    if (self->identifier != other->identifier) {
        return 0;
    }
    for (int i = 0; i < 4; i++) {
        if (other->bits[i] != self->bits[i]) {
            return 0;
        }
    }
    return 1;
}

// 0x7D5470 NiMaterialDescriptor: all zero
Descriptor* __fastcall Descriptor_ConstructImpl(Descriptor* self, re::Edx) {
    self->identifier = nullptr;
    self->bits[0] = 0;
    self->bits[1] = 0;
    self->bits[2] = 0;
    self->bits[3] = 0;
    return self;
}

// 0x7D5490 operator=
Descriptor* __fastcall Descriptor_AssignImpl(Descriptor* self, re::Edx, const Descriptor* other) {
    FixedString_Assign(&self->identifier, &other->identifier);
    self->bits[0] = other->bits[0];
    self->bits[1] = other->bits[1];
    self->bits[2] = other->bits[2];
    self->bits[3] = other->bits[3];
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIMATDESC2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIMATDESC2_REVERSED("NiMaterialDescriptor::operator==", 0x007D5440, ni::Descriptor_EqualsImpl);
NIMATDESC2_REVERSED("NiMaterialDescriptor::NiMaterialDescriptor", 0x007D5470, ni::Descriptor_ConstructImpl);
NIMATDESC2_REVERSED("NiMaterialDescriptor::operator=", 0x007D5490, ni::Descriptor_AssignImpl);
