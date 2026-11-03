// IArchivable methods Ghidra's function list missed (found by tools/sweep in a vtable).
// CAmbientVehicleInfo: see AmbientVehicleInfo.cpp (0x1C bytes, its IArchivable at +0x18,
// vtable 0x912614: 0x4A8160 destroy, 0x4A81B0 read, 0x4A8220 write).

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

GAME_VAR(const char*, keyUnknown4, 0x00A356A8);
GAME_FN(const char*, __cdecl, SeatKindName, 0x004CE3C0, int kind);

} // namespace

namespace sweeparchivable {

// 0x4A8220 (thiscall, this is the IArchivable): CAmbientVehicleInfo's write: +0x4, then
// the byte of each of the base's (vf1) kinds under the seat kinds' names, through the
// writer's int write (vf24)
void __fastcall AmbientVehicleInfo_ArchivableWrite(uint8_t* self, re::Edx, void* writer) {
    using WriteInt = void(__thiscall*)(void*, int, const char*);
    uint8_t* base = self - 0x18;
    re::PtrCast<WriteInt>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(writer))[24])(writer, At<int32_t>(base, 4), keyUnknown4);
    using Count = int(__thiscall*)(void*);
    int count = re::PtrCast<Count>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(base))[1])(base);
    for (int i = 0; i < count; i++) {
        const char* name = SeatKindName(i);
        re::PtrCast<WriteInt>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(writer))[24])(writer, base[8 + i], name);
    }
}

} // namespace sweeparchivable

// ---- registration ---------------------------------------------------------------------

#define SWEEPARCH_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

SWEEPARCH_REVERSED("CAmbientVehicleInfo::ArchivableWrite", 0x004A8220, &sweeparchivable::AmbientVehicleInfo_ArchivableWrite);
