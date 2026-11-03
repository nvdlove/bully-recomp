// NiFragmentMaterial::InsertIntoHash (0x787C60), the tail of GenerateShader (0x787D50): a made
// shader kept in the material's shader hash. Its descriptor (slot 25, cast by
// NiFragmentShaderInstanceDescriptor's class bit, 0x20E40EC) hashed by the material (slot 15, on
// the descriptor's key at +0x4) picks the bucket (the object sets at +0x24, 0xC each), which holds a
// reference to the shader.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c39 {
namespace {

using ni::sys::At;
using ni::sys::Virtual;

GAME_VAR(uint32_t, NiFragmentShaderInstanceDescriptor_ClassBit, 0x020E40EC);
GAME_FN(void, __thiscall, ShaderSet_Add, 0x00787BE0, void* set, const re::Ptr32<NiRefObject>* shader);

inline bool HasClassBit(const void* object, uint32_t bit) {
    const uint8_t* rtti = Virtual<const uint8_t*>(object, 2);
    return (At<uint32_t>(rtti, 0xC + int(bit >> 5) * 4) & (1u << (bit & 0x1F))) != 0;
}

} // namespace

// 0x787C60 (thiscall): NiFragmentMaterial::InsertIntoHash (the shader held while it's added)
void __fastcall FragmentMaterial_InsertIntoHash(uint8_t* self, re::Edx, NiRefObject* shader) {
    void* descriptor = Virtual<void*>(shader, 25);   // GetShaderDesc
    uint8_t* instance = nullptr;
    if (descriptor != nullptr && HasClassBit(descriptor, NiFragmentShaderInstanceDescriptor_ClassBit)) {
        instance = static_cast<uint8_t*>(descriptor);
    }
    uint32_t bucket = Virtual<uint32_t>(self, 15, reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(instance) + 4));   // the descriptor's hash
    re::Ptr32<NiRefObject> held = shader;
    ni::sys::Increment(&shader->refCount);
    ShaderSet_Add(self + 0x24 + bucket * 0xC, &held);
    if (ni::sys::Decrement(&shader->refCount) == 0) {
        Virtual<void>(shader, 1);   // DeleteThis
    }
}

#define C49_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C49_REVERSED("NiFragmentMaterial::InsertIntoHash", 0x00787C60, FragmentMaterial_InsertIntoHash);

} // namespace ni::c39
