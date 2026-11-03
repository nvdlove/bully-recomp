// The placement file reader's record readers: each reads one record of a section from
// the IplStream (fields in file order: strings as a length byte and the characters,
// 4-byte values) and hands the fields to the section's loader object through its
// virtual functions; and the section reader (0x435290) that picks the reader by the
// section's tag. All thiscall on the loader object, the stream the argument.
//
// The values are passed on as the raw 4 bytes read (the game copies the float ones
// through the x87 stack, which would only change a signalling NaN). The game's string
// buffers are 0x44 bytes for some fields, with a length byte up to 255 (ours are 0x100).

#include <cstdint>
#include <cstring>

#include "game/Placement.h"
#include "reversed.h"

namespace {

GAME_FN(uint32_t, __cdecl, PR_File_Read, 0x0042D2A0, void* file, void* buffer, uint32_t size);
GAME_FN(uint32_t, __cdecl, PR_GetTimeMs, 0x0045BCA0);   // (from the performance counter)
// (the stream's read, thiscall on the stream: the same as Read below)
GAME_FN(void, __thiscall, PR_Stream_Read, 0x00433510, IplStream* stream, void* buffer, uint32_t size);
// the record readers (ours below, and the ones of other files)
GAME_FN(void, __thiscall, PR_ReadInstance, 0x00433560, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadSpecial, 0x004335C0, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadProjectile, 0x00433630, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadOcclusion, 0x004336A0, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadPerimeter, 0x00433840, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadTrigger, 0x00433A20, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadPoint, 0x00433ED0, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadPathPoint, 0x004344C0, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadPath, 0x004346C0, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadProp, 0x00434850, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadPoiPoint, 0x00434900, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadPoi, 0x00434F40, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PR_ReadRail, 0x004350A0, void* loader, IplStream* stream);

// (inlined at every field)
void Read(IplStream* stream, void* buffer, uint32_t size) {
    if (!stream->data) {
        PR_File_Read(stream->file, buffer, size);
    } else {
        std::memcpy(buffer, stream->data + stream->position, size);
        stream->position += size;
    }
}

uint32_t Read4(IplStream* stream) {
    uint32_t value = 0;
    Read(stream, &value, 4);
    return value;
}

// a length byte, the characters, a terminating 0 added; the length
int ReadString(IplStream* stream, char* buffer) {
    uint8_t length = 0;
    Read(stream, &length, 1);
    Read(stream, buffer, length);
    buffer[length] = 0;
    return length;
}

// a virtual function of the loader object, by its offset in the vtable
template <typename R = void, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    re::Ptr32<void>* vtable = *static_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return re::PtrCast<R(__thiscall*)(void*, re::Arg<A>...)>(vtable[offset / 4])(object, args...);
}

} // namespace

namespace IplRecords {

// 0x433ED0 ("pont" records, PointLoaderImpl): vf0 the name and a value, vf1 and vf2 two
// vectors, vf3 a float, three strings, six values, two strings
static void __fastcall ReadPoint(void* loader, re::Edx, IplStream* stream) {
    char name[0x100], s1[0x100], s2[0x100], s3[0x100], s4[0x100], s5[0x100];
    int nameLength = ReadString(stream, name);
    uint32_t value = Read4(stream);
    VCall(loader, 0, name, nameLength, value);
    uint32_t a = Read4(stream), b = Read4(stream), c = Read4(stream);
    VCall(loader, 4, a, b, c);
    a = Read4(stream), b = Read4(stream), c = Read4(stream);
    VCall(loader, 8, a, b, c);
    uint32_t f = Read4(stream);
    int l1 = ReadString(stream, s1);
    int l2 = ReadString(stream, s2);
    int l3 = ReadString(stream, s3);
    uint32_t v[6];
    for (uint32_t& x : v) {
        x = Read4(stream);
    }
    int l4 = ReadString(stream, s4);
    int l5 = ReadString(stream, s5);
    VCall(loader, 0xC, f, s1, l1, s2, l2, s3, l3, v[0], v[1], v[2], v[3], v[4], v[5], s4, l4, s5, l5);
}

// 0x4344C0 (a path's point, PathPointsLoaderImpl): vf0, vf1 and vf2 two vectors, vf3 a
// float and two values
static void __fastcall ReadPathPoint(void* loader, re::Edx, IplStream* stream) {
    VCall(loader, 0);
    uint32_t a = Read4(stream), b = Read4(stream), c = Read4(stream);
    VCall(loader, 4, a, b, c);
    a = Read4(stream), b = Read4(stream), c = Read4(stream);
    VCall(loader, 8, a, b, c);
    a = Read4(stream), b = Read4(stream), c = Read4(stream);
    VCall(loader, 0xC, a, b, c);
}

// 0x4346C0 ("ptpx" records, PathLoaderImpl): the name, two values and the point count
// (vf0, vf1), each point through the loader vf2 gives (vf3 told), vf4, vf5
static void __fastcall ReadPath(void* loader, re::Edx, IplStream* stream) {
    char name[0x100];
    int nameLength = ReadString(stream, name);
    uint32_t a = Read4(stream);
    uint32_t b = Read4(stream);
    int32_t count = int32_t(Read4(stream));
    VCall(loader, 0, name, nameLength, a, b, count);
    VCall(loader, 4, count);
    for (int32_t i = 0; i < count; i++) {
        void* point = VCall<void*>(loader, 8, i);
        PR_ReadPathPoint(point, stream);
        VCall(loader, 0xC, i, point);
    }
    VCall(loader, 0x10);
    VCall(loader, 0x14);
}

// 0x434850 ("prop" records, PropLoaderImpl): the name (vf0), vf1
static void __fastcall ReadProp(void* loader, re::Edx, IplStream* stream) {
    char name[0x100];
    int nameLength = ReadString(stream, name);
    VCall(loader, 0, name, nameLength);
    VCall(loader, 4);
}

// 0x434900 (a POI's point, PoiPointsLoaderImpl): vf0 four strings and a value, vf1 and
// vf2 two vectors, vf3 fourteen values
static void __fastcall ReadPoiPoint(void* loader, re::Edx, IplStream* stream) {
    char s1[0x100], s2[0x100], s3[0x100], s4[0x100];
    int l1 = ReadString(stream, s1);
    int l2 = ReadString(stream, s2);
    int l3 = ReadString(stream, s3);
    int l4 = ReadString(stream, s4);
    uint32_t value = Read4(stream);
    VCall(loader, 0, s1, l1, s2, l2, s3, l3, s4, l4, value);
    uint32_t a = Read4(stream), b = Read4(stream), c = Read4(stream);
    VCall(loader, 4, a, b, c);
    a = Read4(stream), b = Read4(stream), c = Read4(stream);
    VCall(loader, 8, a, b, c);
    uint32_t v[14];
    for (uint32_t& x : v) {
        x = Read4(stream);
    }
    VCall(loader, 0xC, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13]);
}

// 0x434F40 ("pois" records, PoiLoaderImpl): the name, a value and the point count (vf0,
// vf1), each point through the loader vf2 gives (vf3 told), vf4, vf5
static void __fastcall ReadPoi(void* loader, re::Edx, IplStream* stream) {
    char name[0x100];
    int nameLength = ReadString(stream, name);
    uint32_t a = Read4(stream);
    int32_t count = int32_t(Read4(stream));
    VCall(loader, 0, name, nameLength, a, count);
    VCall(loader, 4, count);
    for (int32_t i = 0; i < count; i++) {
        void* point = VCall<void*>(loader, 8, i);
        PR_ReadPoiPoint(point, stream);
        VCall(loader, 0xC, i, point);
    }
    VCall(loader, 0x10);
    VCall(loader, 0x14);
}

// 0x4350A0 ("rail" records, RailLoaderImpl): the name, a value and the point count (vf0,
// vf1), each point's position (vf2 with its index), vf3, vf4
static void __fastcall ReadRail(void* loader, re::Edx, IplStream* stream) {
    char name[0x100];
    int nameLength = ReadString(stream, name);
    uint32_t a = Read4(stream);
    int32_t count = int32_t(Read4(stream));
    VCall(loader, 0, name, nameLength, a, count);
    VCall(loader, 4, count);
    for (int32_t i = 0; i < count; i++) {
        uint32_t x = Read4(stream), y = Read4(stream), z = Read4(stream);
        VCall(loader, 8, i, x, y, z);
    }
    VCall(loader, 0xC);
    VCall(loader, 0x10);
}

// 0x435290 (SectionLoaderImpl's section read): the tag (told to vf0; or kept in the def's
// +0x64 from a read that stopped), then the section's records through the loader its
// getter gives. Sections other than "inst", "proj", "spec", "occl" and "prop" only on
// the file's first read (def +0x5C). "inst" and "proj" stop after 8 ms (unless +0xC)
// and go on next time from the def's +0x68 of +0x6C: false then. vf44 at the end.
static uint8_t __fastcall ReadSection(void* section, re::Edx, IplStream* stream) {
    uint8_t* def = *reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<uint8_t*>(section) + 8);
    uint8_t noTimeLimit = *(static_cast<uint8_t*>(section) + 0xC);
    uint32_t tag = *reinterpret_cast<uint32_t*>(def + 0x64);
    if (tag == 0) {
        tag = Read4(stream);
        VCall(section, 0, tag);
    }
    // the counted sections: (count vf, getter vf, done vf, end vf, reader, first read only)
    struct Kind {
        uint32_t tag;
        uint32_t count, get, done, end;
        void(__thiscall* const* reader)(void*, IplStream*);
        bool firstReadOnly;
    };
    static const Kind kinds[] = {
        {0x67697274, 0x50, 0x54, 0x58, 0x5C, &PR_ReadTrigger, true},      // "trig"
        {0x63657073, 0x14, 0x08, 0x28, 0x2C, &PR_ReadSpecial, false},     // "spec"
        {0x6C63636F, 0x30, 0x34, 0x38, 0x3C, &PR_ReadOcclusion, false},   // "occl"
        {0x6C696172, 0xA0, 0xA4, 0xA8, 0xAC, &PR_ReadRail, true},         // "rail"
        {0x6D726570, 0x40, 0x44, 0x48, 0x4C, &PR_ReadPerimeter, true},    // "perm"
        {0x746E6F70, 0x60, 0x64, 0x68, 0x6C, &PR_ReadPoint, true},        // "pont"
        {0x706F7270, 0x80, 0x84, 0x88, 0x8C, &PR_ReadProp, false},        // "prop"
        {0x73696F70, 0x90, 0x94, 0x98, 0x9C, &PR_ReadPoi, true},          // "pois"
        {0x78687470, 0x70, 0x74, 0x78, 0x7C, &PR_ReadPath, true},         // "ptpx"
    };
    if (tag == 0x74736E69 || tag == 0x6A6F7270) {
        // "inst" / "proj": in parts
        bool inst = tag == 0x74736E69;
        uint32_t countVf = inst ? 0x04 : 0x18, getVf = inst ? 0x08 : 0x1C, doneVf = inst ? 0x0C : 0x20, endVf = inst ? 0x10 : 0x24;
        uint32_t i = 0;
        uint32_t count;
        if (*reinterpret_cast<uint32_t*>(def + 0x6C) == 0) {
            PR_Stream_Read(stream, &count, 4);
            VCall(section, countVf, count);
        } else {
            count = *reinterpret_cast<uint32_t*>(def + 0x6C);
            i = *reinterpret_cast<uint32_t*>(def + 0x68);
        }
        uint32_t start = 0;  // (the game's is uninitialised when +0xC is set, and unused)
        if (!noTimeLimit) {
            start = PR_GetTimeMs();
        }
        for (; i < count; i++) {
            void* loader = VCall<void*>(section, getVf, i);
            if (inst) {
                PR_ReadInstance(loader, stream);
            } else {
                PR_ReadProjectile(loader, stream);
            }
            VCall(section, doneVf, i, loader);
            if (!*(static_cast<uint8_t*>(section) + 0xC) && PR_GetTimeMs() > start + 8) {
                uint8_t* current = *reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<uint8_t*>(section) + 8);
                *reinterpret_cast<uint32_t*>(current + 0x64) = tag;
                *reinterpret_cast<uint32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<uint8_t*>(section) + 8) + 0x68) = i + 1;
                *reinterpret_cast<uint32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<uint8_t*>(section) + 8) + 0x6C) = count;
                return 0;
            }
        }
        static const uint32_t cleared[] = {0x64, 0x68, 0x6C, 0x60};
        for (uint32_t offset : cleared) {
            *reinterpret_cast<uint32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<uint8_t*>(section) + 8) + offset) = 0;
        }
        VCall(section, endVf);
    } else {
        for (const Kind& kind : kinds) {
            if (kind.tag != tag) {
                continue;
            }
            if (kind.firstReadOnly && !(*reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<uint8_t*>(section) + 8))[0x5C]) {
                break;
            }
            uint32_t count;
            PR_Stream_Read(stream, &count, 4);
            VCall(section, kind.count, count);
            for (uint32_t i = 0; i < count; i++) {
                void* loader = VCall<void*>(section, kind.get, i);
                (*kind.reader)(loader, stream);
                VCall(section, kind.done, i, loader);
            }
            VCall(section, kind.end);
            break;
        }
    }
    VCall(section, 0xB0);
    return 1;
}

#define PLACEREC_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

PLACEREC_REVERSED("IplLoaderImpl::ReadPoint", 0x00433ED0, &ReadPoint);
PLACEREC_REVERSED("IplLoaderImpl::ReadPathPoint", 0x004344C0, &ReadPathPoint);
PLACEREC_REVERSED("IplLoaderImpl::ReadPath", 0x004346C0, &ReadPath);
PLACEREC_REVERSED("IplLoaderImpl::ReadProp", 0x00434850, &ReadProp);
PLACEREC_REVERSED("IplLoaderImpl::ReadPoiPoint", 0x00434900, &ReadPoiPoint);
PLACEREC_REVERSED("IplLoaderImpl::ReadPoi", 0x00434F40, &ReadPoi);
PLACEREC_REVERSED("IplLoaderImpl::ReadRail", 0x004350A0, &ReadRail);
PLACEREC_REVERSED("SectionLoaderImpl::ReadSection", 0x00435290, &ReadSection);

} // namespace IplRecords
