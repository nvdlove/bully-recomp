// The binary placement (IPB) sections' records: read from an IplStream (its data, or
// its file when it has none) and handed to the section's loader through its virtuals
// (0x435290 dispatches on the section's tag: 'inst', 'spec', 'proj', 'occl', 'rail',
// 'trig'). And a second stream kind's read (0x42E730). 0x433510..0x433ED0.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Placement.h"
#include "reversed.h"

#define IPB_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

GAME_FN(int, __cdecl, IPB_File_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(uint32_t, __cdecl, IPB_File_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, void* file);

// (IplStream::Read, inlined in the section readers)
void ReadBytes(IplStream* stream, void* out, uint32_t size) {
    if (!stream->data) {
        IPB_File_Read(stream->file, out, int(size));
        return;
    }
    std::memcpy(out, stream->data + stream->position, size);
    stream->position += size;
}
template <typename T>
T Read(IplStream* stream) {
    T value;
    ReadBytes(stream, &value, sizeof(T));
    return value;
}

// A section's loader: only its virtuals are used here
struct IplSectionLoader {
    re::Ptr32<re::Ptr32<void>> vtable;

    template <typename R = void, typename... A>
    R Call(int slot, A... args) {
        return re::PtrCast<R(__thiscall*)(IplSectionLoader*, re::Arg<A>...)>(vtable[slot])(this, args...);
    }
};

// The other stream (0x10: its file, or data with the read position at +0xC)
struct DataStream {
    re::Ptr32<void> file;         // +0
    re::Ptr32<uint8_t> data;      // +4
    uint32_t unknown8;  // +8
    uint32_t position;  // +0xC
};
static_assert(offsetof(DataStream, position) == 0xC);

// A name: its length byte, then that many characters
void ReadName(IplStream* stream, char (&name)[256], uint8_t& length) {
    length = Read<uint8_t>(stream);
    ReadBytes(stream, name, length);
    name[length] = 0;
}

} // namespace

namespace IplBinary {

// 0x42E730: from the file (fread-like: 1 item of `size`) or the data; the items or the
// bytes read (0: no stream)
uint32_t __cdecl DataStream_Read(DataStream* stream, void* out, uint32_t size) {
    if (!stream) {
        return 0;
    }
    if (stream->file) {
        return IPB_File_ReadItems(out, size, 1, stream->file);
    }
    if (!stream->data) {
        return 0;
    }
    std::memcpy(out, stream->data + stream->position, size);
    stream->position += size;
    return size;
}

// 0x433510 (thiscall): IplStream::Read
void __fastcall IplStream_Read(IplStream* stream, re::Edx, void* out, uint32_t size) {
    ReadBytes(stream, out, size);
}

// 0x433560 (thiscall, on the 'inst' loader): an instance's 0x78 bytes to its vf5
void __fastcall ReadInstance(IplSectionLoader* loader, re::Edx, IplStream* stream) {
    uint8_t record[0x78];
    ReadBytes(stream, record, sizeof(record));
    loader->Call(5, record);
}

// 0x4335C0 (thiscall, on the 'spec' loader): 0x80 bytes to its vf6
void __fastcall ReadSpecial(IplSectionLoader* loader, re::Edx, IplStream* stream) {
    uint8_t record[0x80];
    ReadBytes(stream, record, sizeof(record));
    loader->Call(6, record);
}

// 0x433630 (thiscall, on the 'proj' loader): 0x6C bytes to its vf0
void __fastcall ReadProjectile(IplSectionLoader* loader, re::Edx, IplStream* stream) {
    uint8_t record[0x6C];
    ReadBytes(stream, record, sizeof(record));
    loader->Call(0, record);
}

// 0x4336A0 (thiscall, on the 'occl' loader): vf0 (begin), 3 floats to vf1 (the centre),
// 4 to vf2
void __fastcall ReadOcclusionVolume(IplSectionLoader* loader, re::Edx, IplStream* stream) {
    loader->Call(0);
    float a = Read<float>(stream);
    float b = Read<float>(stream);
    float c = Read<float>(stream);
    loader->Call(1, a, b, c);
    float d = Read<float>(stream);
    float e = Read<float>(stream);
    float f = Read<float>(stream);
    float g = Read<float>(stream);
    loader->Call(2, d, e, f, g);
}

// 0x433840 (thiscall, on the 'rail' loader): its name, an int, two floats and the
// point count to vf0, the count to vf1, each point's float to vf2 (index, value), then
// vf3 and vf4
void __fastcall ReadRail(IplSectionLoader* loader, re::Edx, IplStream* stream) {
    char name[256];
    uint8_t length;
    ReadName(stream, name, length);
    int32_t a = Read<int32_t>(stream);
    float b = Read<float>(stream);
    float c = Read<float>(stream);
    int32_t count = Read<int32_t>(stream);
    loader->Call(0, static_cast<char*>(name), uint32_t(length), a, b, c, count);
    loader->Call(1, count);
    for (int32_t i = 0; i < count; i++) {
        float value = Read<float>(stream);
        loader->Call(2, i, value);
    }
    loader->Call(3);
    loader->Call(4);
}

// 0x433A20 (thiscall, on the 'trig' loader): its name and an int to vf0, then the
// position (vf1), size (vf2), four single floats (vf3..vf6), a vector (vf7) and five
// ints (vf8)
void __fastcall ReadTrigger(IplSectionLoader* loader, re::Edx, IplStream* stream) {
    char name[256];
    uint8_t length;
    ReadName(stream, name, length);
    int32_t id = Read<int32_t>(stream);
    loader->Call(0, static_cast<char*>(name), uint32_t(length), id);
    float x = Read<float>(stream);
    float y = Read<float>(stream);
    float z = Read<float>(stream);
    loader->Call(1, x, y, z);
    x = Read<float>(stream);
    y = Read<float>(stream);
    z = Read<float>(stream);
    loader->Call(2, x, y, z);
    loader->Call(3, Read<float>(stream));
    loader->Call(4, Read<float>(stream));
    loader->Call(5, Read<float>(stream));
    loader->Call(6, Read<float>(stream));
    x = Read<float>(stream);
    y = Read<float>(stream);
    z = Read<float>(stream);
    loader->Call(7, x, y, z);
    int32_t v0 = Read<int32_t>(stream);
    int32_t v1 = Read<int32_t>(stream);
    int32_t v2 = Read<int32_t>(stream);
    int32_t v3 = Read<int32_t>(stream);
    int32_t v4 = Read<int32_t>(stream);
    loader->Call(8, v0, v1, v2, v3, v4);
}

IPB_REVERSED("DataStream::Read", 0x0042E730, &DataStream_Read);
IPB_REVERSED("IplStream::Read", 0x00433510, &IplStream_Read);
IPB_REVERSED("IplBinary::ReadInstance", 0x00433560, &ReadInstance);
IPB_REVERSED("IplBinary::ReadSpecial", 0x004335C0, &ReadSpecial);
IPB_REVERSED("IplBinary::ReadProjectile", 0x00433630, &ReadProjectile);
IPB_REVERSED("IplBinary::ReadOcclusionVolume", 0x004336A0, &ReadOcclusionVolume);
IPB_REVERSED("IplBinary::ReadRail", 0x00433840, &ReadRail);
IPB_REVERSED("IplBinary::ReadTrigger", 0x00433A20, &ReadTrigger);

} // namespace IplBinary
