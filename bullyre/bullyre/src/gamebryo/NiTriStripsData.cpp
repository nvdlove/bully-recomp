// NiTriStripsData::Replace (the strip count and the two arrays swapped in, the old ones freed or,
// for data in an object group, the group's count dropped), and the lone jumps the linker made
// for the geometry, extra data and node classes' shared slots (marked here, no code: each only
// jumps to the base class's function).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

// NiObjectGroup: size, buffer, free, reference count
struct ObjectGroup {
    uint32_t size;
    re::Ptr32<uint8_t> buffer;
    re::Ptr32<uint8_t> free;
    uint32_t refCount;   // +0xC
};

GAME_FN(void, __thiscall, ObjectGroup_Destruct, 0x007D1CE0, ObjectGroup* self);

// the group the data's arrays came from (slot 16 GetGroup) let go of one array
inline void DropGroupReference(void* self) {
    ObjectGroup* group = Virtual<ObjectGroup*>(self, 16);
    if (--group->refCount == 0) {
        ObjectGroup_Destruct(group);
        NiDelete(group, 0x10);
    }
}

// 0x7D85E0 NiTriStripsData::Replace(strips, stripLengths, stripLists)
void __fastcall TriStripsData_ReplaceImpl(uint8_t* self, re::Edx, uint32_t strips, uint16_t* lengths,
                                          uint16_t* lists) {
    if (Virtual<ObjectGroup*>(self, 16) != nullptr) {
        if (lengths != At<uint16_t*>(self, 0x48)) {
            DropGroupReference(self);
        }
        if (lists != At<uint16_t*>(self, 0x4C)) {
            DropGroupReference(self);
        }
    } else {
        if (lengths != At<uint16_t*>(self, 0x48)) {
            NiFree(At<uint16_t*>(self, 0x48));
        }
        if (lists != At<uint16_t*>(self, 0x4C)) {
            NiFree(At<uint16_t*>(self, 0x4C));
        }
    }
    At<uint16_t*>(self, 0x48) = lengths;
    At<uint16_t*>(self, 0x4C) = lists;
    At<uint16_t>(self, 0x44) = uint16_t(strips);
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NITRISTRIPS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

// lone jumps (the target in the comment)
RE_UNHOOKED("NiGeometryData::RegisterStreamables (jump)", 0x007D6210);          // -> 0x77A3F0
RE_UNHOOKED("NiScreenElements::LoadBinary (jump)", 0x007D6260);                 // -> 0x77B8E0
RE_UNHOOKED("NiScreenElements::LinkObject (jump)", 0x007D6270);                 // -> 0x77B8F0
RE_UNHOOKED("NiScreenElements::IsEqual (jump)", 0x007D6280);                    // -> 0x7D8A50
RE_UNHOOKED("NiExtraData::RegisterStreamables (jump)", 0x007D8050);             // -> 0x765580
RE_UNHOOKED("NiTriShapeData::LinkObject (jump)", 0x007D83A0);                   // -> 0x779CB0
RE_UNHOOKED("NiTriShape::RegisterStreamables (jump)", 0x007D8A30);              // -> 0x7E0860
RE_UNHOOKED("NiTriShape::SaveBinary (jump)", 0x007D8A40);                       // -> 0x7799E0
RE_UNHOOKED("NiTriShape::IsEqual (jump)", 0x007D8A50);                          // -> 0x779400
RE_UNHOOKED("NiTriShapeDynamicData::LinkObject (jump)", 0x007D8C10);            // -> 0x7D83A0
RE_UNHOOKED("NiTriShapeDynamicData::RegisterStreamables (jump)", 0x007D8C20);   // -> 0x77BB50
RE_UNHOOKED("NiSwitchNode::LinkObject (jump)", 0x007D93E0);                     // -> 0x760590
RE_UNHOOKED("NiSwitchNode::RegisterStreamables (jump)", 0x007D93F0);            // -> 0x760150
NITRISTRIPS_REVERSED("NiTriStripsData::Replace", 0x007D85E0, ni::TriStripsData_ReplaceImpl);
