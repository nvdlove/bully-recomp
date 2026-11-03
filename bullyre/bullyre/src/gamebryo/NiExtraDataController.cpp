// NiExtraDataController's own slots (src/gamebryo/NiAnimControllers.h: vtable 0x9685E0, 0x40 bytes):
// a NiSingleInterpController driving one of its target's extra data, named by +0x38 and found
// again (+0x3C, a reference) whenever the target changes, links up or is cloned. Its kinds
// (the float, floats, point and colour ones) are src/gamebryo/NiExtraDataControllers.cpp.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiAnimControllers.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::GetViewerStringString;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiExtraDataController_RTTI, 0x020E580C);
GAME_VAR(uint32_t, ms_NiObjectNETClassIndex, 0x020E277C);   // (the RTTI's +8)

constexpr uintptr_t NiExtraDataController_vtable = 0x009685E0;

GAME_FN(NiObject*, __thiscall, ObjectNET_GetExtraData, 0x00764250, NiObjectNET* self, const re::Ptr32<const char>* name);
GAME_FN(uint8_t, __thiscall, Map_GetAt, 0x005AC5F0, void* map, void* key, re::Ptr32<void>* value);   // NiTPointerMap::GetAt
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* self, NiRefObject* object);
GAME_FN(void, __thiscall, Object_PostLinkObject, 0x008A20A0, void* self, void* stream);   // (empty)
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
GAME_FN(void, __thiscall, TimeController_SetTarget, 0x0077E050, void* self, NiObjectNET* target);
// NiSingleInterpController's
GAME_FN(void, __thiscall, SingleInterp_Construct, 0x008250C0, void* self);
GAME_FN(void, __thiscall, SingleInterp_CopyMembers, 0x008251B0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, SingleInterp_ProcessClone, 0x00825210, void* self, void* cloning);
GAME_FN(void, __thiscall, SingleInterp_LoadBinary, 0x00825240, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, SingleInterp_RegisterStreamables, 0x008252B0, void* self, void* stream);
GAME_FN(void, __thiscall, SingleInterp_SaveBinary, 0x008252E0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, SingleInterp_IsEqual, 0x00825300, void* self, void* other);
GAME_FN(void, __thiscall, SingleInterp_GetViewerStrings, 0x00825350, void* self, NiTArray<char*>* strings);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, ExtraDataController_GetExtraDataPointerFromName, 0x00828030, void* self);
GAME_FN(void, __thiscall, ExtraDataController_Destruct, 0x008280C0, void* self);

// 0x827F40: RegisterStreamables (the name registered with the stream: its slot 8)
uint8_t __fastcall ExtraCtlr_RegisterStreamables(NiExtraDataController* self, re::Edx, void* stream) {
    if (!SingleInterp_RegisterStreamables(self, stream)) {
        return 0;
    }
    Virtual<uint8_t>(stream, 8, &self->extraDataName);
    return 1;
}

// 0x827F70: SaveBinary (the name)
void __fastcall ExtraCtlr_SaveBinary(NiExtraDataController* self, re::Edx, void* stream) {
    SingleInterp_SaveBinary(self, stream);
    scene::Stream_SaveFixedString(stream, &self->extraDataName);
}

// 0x827F90: LoadBinary (the name: inline before 20.1.0.1, by index from it)
void __fastcall ExtraCtlr_LoadBinary(NiExtraDataController* self, re::Edx, void* stream) {
    SingleInterp_LoadBinary(self, stream);
    if (scene::StreamVersion(stream) < 0x14010001) {
        scene::Stream_LoadInlineFixedString(stream, &self->extraDataName);
        return;
    }
    scene::Stream_LoadFixedString(stream, &self->extraDataName);
}

// 0x827FD0: IsEqual (the extra data both there and equal, or both missing; the same name)
uint8_t __fastcall ExtraCtlr_IsEqual(NiExtraDataController* self, re::Edx, NiExtraDataController* other) {
    if (!SingleInterp_IsEqual(self, other)) {
        return 0;
    }
    NiObject* mine = self->extraData;
    NiObject* theirs = other->extraData;
    if (mine != nullptr) {
        if (theirs == nullptr) {
            return 0;
        }
        if (!Virtual<uint8_t>(mine, 8, theirs)) {
            return 0;
        }
    } else if (theirs != nullptr) {
        return 0;
    }
    return other->extraDataName == self->extraDataName ? 1 : 0;
}

// 0x828030: GetExtraDataPointerFromName (Gamebryo's name, a guess): the target's extra data of
// that name held (none when it has none)
void __fastcall ExtraCtlr_GetExtraDataPointerFromName(NiExtraDataController* self, re::Edx) {
    NiObject* data = ObjectNET_GetExtraData(self->target, &self->extraDataName);
    AssignReleasingFirst(self->extraData, data);
}

// 0x828080
const RTTI* __fastcall ExtraCtlr_GetRTTI(NiExtraDataController*, re::Edx) {
    return &NiExtraDataController_RTTI;
}

// 0x828090: NiExtraDataController() (no name: the empty handle, not counted)
NiExtraDataController* __fastcall ExtraCtlr_Construct(NiExtraDataController* self, re::Edx) {
    SingleInterp_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiExtraDataController_vtable);
    self->extraDataName = scene::NullFixedString;
    self->extraData = nullptr;
    return self;
}

// 0x8280C0: ~NiExtraDataController (the extra data and the name let go, then
// ~NiSingleInterpController inlined: the interpolator)
void __fastcall ExtraCtlr_Destruct(NiExtraDataController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiExtraDataController_vtable);
    ReleaseRef(self->extraData);
    scene::FixedStringRelease(&self->extraDataName);
    ReleaseRef(self->interpolator);
    TimeController_Destruct(self);
}

// 0x828140: SetTarget(target) (slot 21): the extra data let go, the target set, the extra data
// found on it
void __fastcall ExtraCtlr_SetTarget(NiExtraDataController* self, re::Edx, NiObjectNET* target) {
    if (self->extraData != nullptr) {
        ReleaseRef(self->extraData);
        self->extraData = nullptr;
    }
    TimeController_SetTarget(self, target);
    if (self->target != nullptr) {
        ExtraDataController_GetExtraDataPointerFromName(self);
    }
}

// 0x828190: InterpTargetIsCorrectType(target) (slot 40): a NiObjectNET with an extra data of the
// name (its list read up to the first empty slot)
uint8_t __fastcall ExtraCtlr_InterpTargetIsCorrectType(NiExtraDataController* self, re::Edx, NiObjectNET* target) {
    if (target == nullptr || !IsKindOf(target, ms_NiObjectNETClassIndex)) {
        return 0;
    }
    NiFixedString name = self->extraDataName;
    if (name == scene::NullFixedString) {
        return 0;
    }
    uint16_t count = target->extraDataSize;
    if (count == 0) {
        return 0;
    }
    re::Ptr32<NiExtraData>* entries = target->extraData;
    NiExtraData* entry = entries[0];
    uint32_t i = 1;
    while (entry != nullptr) {
        if (scene::At<NiFixedString>(entry, 8) == name) {
            return 1;
        }
        if (i >= count) {
            break;
        }
        entry = entries[i];
        i++;
    }
    return 0;
}

// 0x828220: CopyMembers (the name)
void __fastcall ExtraCtlr_CopyMembers(NiExtraDataController* self, re::Edx, NiExtraDataController* dest,
                                      void* cloning) {
    SingleInterp_CopyMembers(self, dest, cloning);
    scene::FixedString_Assign(&dest->extraDataName, &self->extraDataName);
}

// 0x828250: ProcessClone (slot 11): when both have targets, the clone's extra data made the
// cloning's clone of this one's (none when it isn't in the map; the clone, found in the map, is
// written through even when missing)
void __fastcall ExtraCtlr_ProcessClone(NiExtraDataController* self, re::Edx, re::Ptr32<void>* cloning) {
    SingleInterp_ProcessClone(self, cloning);
    struct MapNode {   // (the game's layouts)
        re::Ptr32<MapNode> next;
        re::Ptr32<void> key;
        re::Ptr32<void> value;
    };
    struct CloneMap {
        re::Ptr32<void> vtable;
        uint32_t hashSize;
        re::Ptr32<re::Ptr32<MapNode>> buckets;
    };
    auto* map = static_cast<CloneMap*>(*cloning);
    NiExtraDataController* clone = nullptr;
    for (MapNode* node = map->buckets[reinterpret_cast<uintptr_t>(self) % map->hashSize]; node != nullptr;
         node = node->next) {
        if (node->key == self) {
            clone = static_cast<NiExtraDataController*>(node->value);
            break;
        }
    }
    if (self->target != nullptr && clone->target != nullptr) {
        re::Ptr32<void> data = nullptr;
        Map_GetAt(*cloning, self->extraData, &data);
        Pointer_Assign(reinterpret_cast<re::Ptr32<NiRefObject>*>(&clone->extraData), static_cast<NiRefObject*>(data));
    }
}

// 0x8282C0: PostLinkObject (slot 12): the extra data found on the target
void __fastcall ExtraCtlr_PostLinkObject(NiExtraDataController* self, re::Edx, void* stream) {
    Object_PostLinkObject(self, stream);
    if (self->target != nullptr) {
        ExtraDataController_GetExtraDataPointerFromName(self);
    }
}

// 0x8282E0: the deleting destructor
NiExtraDataController* __fastcall ExtraCtlr_DeletingDestructor(NiExtraDataController* self, re::Edx, uint32_t flags) {
    ExtraDataController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x40);
    }
    return self;
}

// 0x828300: GetViewerStrings
void __fastcall ExtraCtlr_GetViewerStrings(NiExtraDataController* self, re::Edx, NiTArray<char*>* strings) {
    SingleInterp_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiExtraDataController_RTTI.name));
    AddViewerString(strings, GetViewerStringString("Extra Data Name", self->extraDataName));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define C33_EXTRACTLR_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_EXTRACTLR_REVERSED("NiExtraDataController::RegisterStreamables", 0x00827F40,
                       &ni::anim::ExtraCtlr_RegisterStreamables);
C33_EXTRACTLR_REVERSED("NiExtraDataController::SaveBinary", 0x00827F70, &ni::anim::ExtraCtlr_SaveBinary);
C33_EXTRACTLR_REVERSED("NiExtraDataController::LoadBinary", 0x00827F90, &ni::anim::ExtraCtlr_LoadBinary);
C33_EXTRACTLR_REVERSED("NiExtraDataController::IsEqual", 0x00827FD0, &ni::anim::ExtraCtlr_IsEqual);
C33_EXTRACTLR_REVERSED("NiExtraDataController::GetExtraDataPointerFromName", 0x00828030,
                       &ni::anim::ExtraCtlr_GetExtraDataPointerFromName);
C33_EXTRACTLR_REVERSED("NiExtraDataController::GetRTTI", 0x00828080, &ni::anim::ExtraCtlr_GetRTTI);
C33_EXTRACTLR_REVERSED("NiExtraDataController::NiExtraDataController", 0x00828090,
                       &ni::anim::ExtraCtlr_Construct);
C33_EXTRACTLR_REVERSED("NiExtraDataController::~NiExtraDataController", 0x008280C0,
                       &ni::anim::ExtraCtlr_Destruct);
C33_EXTRACTLR_REVERSED("NiExtraDataController::SetTarget", 0x00828140, &ni::anim::ExtraCtlr_SetTarget);
C33_EXTRACTLR_REVERSED("NiExtraDataController::InterpTargetIsCorrectType", 0x00828190,
                       &ni::anim::ExtraCtlr_InterpTargetIsCorrectType);
C33_EXTRACTLR_REVERSED("NiExtraDataController::CopyMembers", 0x00828220, &ni::anim::ExtraCtlr_CopyMembers);
C33_EXTRACTLR_REVERSED("NiExtraDataController::ProcessClone", 0x00828250, &ni::anim::ExtraCtlr_ProcessClone);
C33_EXTRACTLR_REVERSED("NiExtraDataController::PostLinkObject", 0x008282C0, &ni::anim::ExtraCtlr_PostLinkObject);
C33_EXTRACTLR_REVERSED("NiExtraDataController::DeletingDestructor", 0x008282E0,
                       &ni::anim::ExtraCtlr_DeletingDestructor);
C33_EXTRACTLR_REVERSED("NiExtraDataController::GetViewerStrings", 0x00828300,
                       &ni::anim::ExtraCtlr_GetViewerStrings);
