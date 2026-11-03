// NiNIFImageReader (vtable 0x965318, 0x430 bytes) on NiImageReader (vtable 0x965308, 0x24
// bytes: a critical section): images read from NIF files (an NiStream at +0x24 whose one
// top-level object is an NiPixelData). The image converter (NiImageConverter.cpp) owns one.
//
// Slots: 0 the deleting destructor, 1 CanReadFile(extension), 2 ReadFile(file, optional
// destination), 3 ReadHeader(file, width&, height&, format&, mipmaps&, faces&).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiStream.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t kNiImageReaderVtable = 0x00965308;
constexpr uintptr_t kNiNIFImageReaderVtable = 0x00965318;
// NiPixelData's class bit (its NiRTTI's word at +8: NiRTTI 0x20E2A44); an object's NiRTTI has
// the bits of its classes from +0xC
GAME_VAR(uint32_t, NiPixelData_ClassBit, 0x020E2A4C);

struct NiNIFImageReader {
    re::Ptr32<void> vtable;                  // +0x0
    NiCriticalSection section;     // +0x4 (NiImageReader's)
    NiStream stream;               // +0x24
};
static_assert(offsetof(NiNIFImageReader, stream) == 0x24);
static_assert(sizeof(NiNIFImageReader) == 0x430);

GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(NiStream*, __thiscall, Stream_Construct, 0x0075CE40, NiStream* self);
GAME_FN(void, __thiscall, Stream_Destruct, 0x0075C020, NiStream* self);
GAME_FN(void, __thiscall, Stream_RemoveAllObjects, 0x0075B4D0, NiStream* self);

inline void LeaveSection(NiCriticalSection& cs) {
    if (--cs.lockCount == 0) {
        cs.owner = 0;
    }
    Import<void>(kLeaveCriticalSection, &cs.section);
}

// vf1 CanReadFile(extension)
uint8_t __fastcall NIFReaderCanReadFile(NiNIFImageReader*, re::Edx, const char* extension) {
    return NiStricmp(extension, ".nif") == 0 ? 1 : 0;
}

// NiImageReader's deleting destructor
NiNIFImageReader* __fastcall ImageReaderDeletingDestructor(NiNIFImageReader* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kNiImageReaderVtable);
    Import<void>(kDeleteCriticalSection, &self->section.section);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x24);
    }
    return self;
}

// ~NiNIFImageReader
void __fastcall NIFReaderDestruct(NiNIFImageReader* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiNIFImageReaderVtable);
    Stream_Destruct(&self->stream);
    self->vtable = reinterpret_cast<void*>(kNiImageReaderVtable);
    Import<void>(kDeleteCriticalSection, &self->section.section);
}

// vf3 ReadHeader: the file read whole (ReadFile, slot 2) and the image's first level described
uint8_t __fastcall NIFReaderReadHeader(NiNIFImageReader* self, re::Edx, void* file, uint32_t* width, uint32_t* height,
                                       uint32_t* format, uint8_t* mipmaps, uint32_t* faces) {
    NiRefObject* pixels = Virtual<NiRefObject*>(self, 2, file, static_cast<void*>(nullptr));
    if (pixels == nullptr) {
        return 0;
    }
    Increment(&pixels->refCount);
    *width = *At<uint32_t*>(pixels, 0x54);
    *height = *At<uint32_t*>(pixels, 0x58);
    const uint32_t* from = &At<uint32_t>(pixels, 0x8);   // the NiPixelFormat (0x44 bytes)
    for (int i = 0; i < 0x11; i++) {
        format[i] = from[i];
    }
    *mipmaps = At<uint32_t>(pixels, 0x60) > 1 ? 1 : 0;
    *faces = At<uint32_t>(pixels, 0x6C);
    if (Decrement(&pixels->refCount) == 0) {
        Virtual<void>(pixels, 1);
    }
    return 1;
}

// 0x7F75B0 NiNIFImageReader()
NiNIFImageReader* __fastcall NIFReaderConstruct(NiNIFImageReader* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiImageReaderVtable);
    self->section.owner = 0;
    self->section.lockCount = 0;
    Import<void>(kInitializeCriticalSection, &self->section.section);
    self->vtable = reinterpret_cast<void*>(kNiNIFImageReaderVtable);
    Stream_Construct(&self->stream);
    return self;
}

// vf0 NiNIFImageReader's deleting destructor
NiNIFImageReader* __fastcall NIFReaderDeletingDestructor(NiNIFImageReader* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kNiNIFImageReaderVtable);
    Stream_Destruct(&self->stream);
    self->vtable = reinterpret_cast<void*>(kNiImageReaderVtable);
    Import<void>(kDeleteCriticalSection, &self->section.section);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x430);
    }
    return self;
}

// vf2 ReadFile(file, destination): under the lock, the stream emptied and loaded from the file
// (its slot 1); its one top-level object, when an NiPixelData (the stream keeps the reference
// until its next load). The destination is ignored
NiRefObject* __fastcall NIFReaderReadFile(NiNIFImageReader* self, re::Edx, void* file, void*) {
    NiCriticalSection& cs = self->section;
    Import<void>(kEnterCriticalSection, &cs.section);
    cs.owner = Import<uint32_t>(kGetCurrentThreadId);
    cs.lockCount++;
    Stream_RemoveAllObjects(&self->stream);
    Virtual<void>(&self->stream, 1, file);
    if (self->stream.topObjects.size != 1) {
        LeaveSection(cs);
        return nullptr;
    }
    NiRefObject* object = self->stream.topObjects.base[0];
    if (object != nullptr) {
        const uint8_t* rtti = Virtual<const uint8_t*>(object, 2);
        uint32_t bit = NiPixelData_ClassBit;
        if ((At<uint32_t>(rtti, 0xC + int(bit >> 5) * 4) & (1u << (bit & 31))) != 0) {
            LeaveSection(cs);
            return object;
        }
    }
    LeaveSection(cs);
    return nullptr;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NINIFREADER_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NINIFREADER_REVERSED("NiNIFImageReader::CanReadFile", 0x007F74B0, ni::NIFReaderCanReadFile);
NINIFREADER_REVERSED("NiImageReader::DeletingDestructor", 0x007F74D0, ni::ImageReaderDeletingDestructor);
NINIFREADER_REVERSED("NiNIFImageReader::~NiNIFImageReader", 0x007F7500, ni::NIFReaderDestruct);
NINIFREADER_REVERSED("NiNIFImageReader::ReadHeader", 0x007F7530, ni::NIFReaderReadHeader);
NINIFREADER_REVERSED("NiNIFImageReader::NiNIFImageReader", 0x007F75B0, ni::NIFReaderConstruct);
NINIFREADER_REVERSED("NiNIFImageReader::DeletingDestructor", 0x007F75E0, ni::NIFReaderDeletingDestructor);
NINIFREADER_REVERSED("NiNIFImageReader::ReadFile", 0x007F7620, ni::NIFReaderReadFile);
