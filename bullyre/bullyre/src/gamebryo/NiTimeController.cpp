// NiTimeController, NiShaderTimeController and NiTimeSyncController (src/gamebryo/NiTimeController.h):
// construction, the target (the controller put on / taken off its chain), cloning, streaming
// (the chain saved from its first streamable controller), comparison, viewer strings.

#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "gamebryo/NiTimeController.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiShaderTimeController_RTTI, 0x020E3244);
GAME_VAR(RTTI, NiTimeSyncController_RTTI, 0x020E3278);
GAME_VAR(RTTI, NiTimeController_RTTI, 0x020E32BC);

constexpr uintptr_t NiTimeController_vtable = 0x00953E98;
constexpr uintptr_t NiShaderTimeController_vtable = 0x00953DE8;
constexpr uintptr_t NiTimeSyncController_vtable = 0x009503E8;

constexpr float kMaxFloat = 3.40282347e+38f;   // FLT_MAX (0x953E88)

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(NiTimeController*, __thiscall, TimeSyncController_Construct, 0x00746A20, void* self);
GAME_FN(void, __thiscall, ObjectNET_PrependController, 0x00764660, NiObjectNET* self, NiTimeController* controller);
GAME_FN(void, __thiscall, ObjectNET_RemoveController, 0x007646E0, NiObjectNET* self, NiTimeController* controller);
GAME_FN(void, __thiscall, Object_SaveBinaryNone, 0x008A20A0, void* self, void* stream);   // NiObject's: nothing
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, ShaderTimeController_Destruct, 0x0077D7A0, NiTimeController* self);
GAME_FN(uint8_t, __thiscall, TimeController_IsEqual, 0x0077DB90, NiTimeController* self, NiTimeController* other);
GAME_FN(NiTimeController*, __thiscall, TimeController_Construct, 0x0077DFA0, NiTimeController* self);
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, NiTimeController* self);
GAME_FN(void, __thiscall, TimeController_CopyMembers, 0x0077E100, NiTimeController* self, NiTimeController* dest,
        void* cloning);
GAME_FN(char*, __cdecl, AnimTypeViewerString, 0x0077E440, const char* prefix, uint32_t type);
GAME_FN(char*, __cdecl, CycleTypeViewerString, 0x0077E4B0, const char* prefix, uint32_t type);

// a reference let go as the library's code does it here (the import's decrement)
inline void ReleaseImport(NiRefObject* object) {
    if (object != nullptr && ni::sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);   // DeleteThis
    }
}

// a reference slot assigned, the old one let go first
inline void AssignReleasingFirst(re::Ptr32<NiTimeController>& slot, NiTimeController* value) {
    NiTimeController* old = slot;
    if (old != value) {
        ReleaseImport(old);
        slot = value;
        if (value != nullptr) {
            ni::sys::Increment(&value->refCount);
        }
    }
}

// ---- NiShaderTimeController

NiTimeController* __fastcall ShaderConstruct(NiTimeController* self, re::Edx) {
    TimeController_Construct(self);
    self->loKeyTime = -kMaxFloat;
    self->vtable = reinterpret_cast<void*>(NiShaderTimeController_vtable);
    self->hiKeyTime = kMaxFloat;
    return self;
}

void __fastcall ShaderDestruct(NiTimeController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShaderTimeController_vtable);
    TimeController_Destruct(self);
}

const RTTI* __fastcall ShaderGetRTTI(NiTimeController*, re::Edx) {
    return &NiShaderTimeController_RTTI;
}

NiTimeController* __fastcall ShaderDeletingDestructor(NiTimeController* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiShaderTimeController_vtable);
    TimeController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x34);
    }
    return self;
}

// ---- NiTimeSyncController

uint8_t __fastcall SyncIsEqual(NiTimeController* self, re::Edx, NiTimeController* other) {
    return TimeController_IsEqual(self, other) != 0 ? 1 : 0;
}

// the destructor (its reference let go twice over: the second finds it null)
void __fastcall SyncDestruct(NiTimeSyncController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiTimeSyncController_vtable);
    if (self->unknown34 != nullptr) {
        ReleaseImport(self->unknown34);
        self->unknown34 = nullptr;
    }
    ReleaseImport(self->unknown34);
    ShaderTimeController_Destruct(self);
}

// vf6 RegisterStreamables: the next controller's only
uint8_t __fastcall SyncRegisterStreamables(NiTimeController* self, re::Edx, void* stream) {
    if (self->next != nullptr) {
        Virtual<uint8_t>(self->next, 6, stream);
    }
    return 1;
}

NiTimeController* __fastcall SyncCreateClone(NiTimeController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x38);
    NiTimeController* clone = memory != nullptr ? TimeSyncController_Construct(memory) : nullptr;
    TimeController_CopyMembers(self, clone, cloning);
    return clone;
}

void __fastcall SyncGetViewerStrings(NiTimeController*, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, GetViewerString(NiTimeSyncController_RTTI.name));
}

// ---- NiTimeController

// vf8 IsEqual: flags, frequency, phase, key times, whether there's a target
uint8_t __fastcall IsEqual(NiTimeController* self, re::Edx, NiTimeController* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    if (self->flags != other->flags || self->frequency != other->frequency || self->phase != other->phase ||
        self->loKeyTime != other->loKeyTime || self->hiKeyTime != other->hiKeyTime) {
        return 0;
    }
    if ((self->target != nullptr) != (other->target != nullptr)) {
        return 0;
    }
    return 1;
}

// the controllers on the chain from this one
uint32_t __fastcall CountChain(NiTimeController* self, re::Edx) {
    uint32_t count = 0;
    for (NiTimeController* controller = self; controller != nullptr; controller = controller->next) {
        count++;
    }
    return count;
}

// vf18 Start(time): active, the last time reset (the start time set for APP_INIT)
void __fastcall Start(NiTimeController* self, re::Edx, float time) {
    self->flags |= 8;
    self->lastTime = -kMaxFloat;
    if ((self->flags & 1) != 0) {
        self->startTime = time;
    }
}

// vf11 ProcessClone: the chain's too
void __fastcall ProcessClone(NiTimeController* self, re::Edx, void* cloning) {
    Object_ProcessClone(self, cloning);
    if (self->next != nullptr) {
        Virtual<void>(self->next, 11, cloning);
    }
}

// vf5 LinkObject: the next controller (a reference) and the target (not one)
void __fastcall LinkObject(NiTimeController* self, re::Edx, void* stream) {
    NiTimeController* next = static_cast<NiTimeController*>(Stream_ResolveLinkID(stream));
    AssignReleasingFirst(self->next, next);
    self->target = static_cast<NiObjectNET*>(Stream_ResolveLinkID(stream));
}

// vf6 RegisterStreamables
uint8_t __fastcall RegisterStreamables(NiTimeController* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->next != nullptr) {
        Virtual<uint8_t>(self->next, 6, stream);
    }
    return 1;
}

NiTimeController* __fastcall Construct(NiTimeController* self, re::Edx) {
    RefObject_Construct(self);
    self->frequency = 1.0f;
    self->flags = 0;
    self->phase = 0.0f;
    self->target = nullptr;
    self->vtable = reinterpret_cast<void*>(NiTimeController_vtable);
    self->loKeyTime = kMaxFloat;
    self->hiKeyTime = -kMaxFloat;
    self->startTime = -kMaxFloat;
    self->lastTime = -kMaxFloat;
    self->scaledTime = -kMaxFloat;
    self->weightedLastTime = 0.0f;
    self->next = nullptr;
    self->flags = uint16_t((self->flags & 0xFF6C) | 0x4C);
    return self;
}

const RTTI* __fastcall GetRTTI(NiTimeController*, re::Edx) {
    return &NiTimeController_RTTI;
}

void __fastcall Destruct(NiTimeController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiTimeController_vtable);
    ReleaseImport(self->next);
    Object_Destruct(self);
}

// SetTarget(target): off the old target's chain, onto the new one's front (unless it's there);
// a vertex controller makes the target geometry's data volatile. Kept alive meanwhile
void __fastcall SetTarget(NiTimeController* self, re::Edx, NiObjectNET* target) {
    if (self->target == target) {
        return;
    }
    ni::sys::Increment(&self->refCount);
    if (self->target != nullptr && self->target->controller != nullptr) {
        ObjectNET_RemoveController(self->target, self);
    }
    self->target = target;
    if (target != nullptr) {
        for (NiTimeController* c = target->controller; c != nullptr; c = c->next) {
            if (c == self) {
                ReleaseImport(self);
                return;
            }
        }
        ObjectNET_PrependController(target, self);
        if (Virtual<uint8_t>(self, 23)) {   // IsVertexController
            uint16_t& flags = At<uint16_t>(At<void*>(self->target, 0xCC), 0x34);
            flags = uint16_t((flags & 0x0FFF) | 0x8000);
        }
    }
    ReleaseImport(self);
}

// CopyMembers: the target is the clone of this one's (when the clone's class matches; none
// otherwise), the chain cloned
void __fastcall CopyMembers(NiTimeController* self, re::Edx, NiTimeController* dest, void* cloning) {
    Object_CopyMembers(self, dest, cloning);
    dest->flags = self->flags;
    dest->frequency = self->frequency;
    dest->phase = self->phase;
    dest->loKeyTime = self->loKeyTime;
    dest->hiKeyTime = self->hiKeyTime;
    dest->startTime = self->startTime;
    dest->lastTime = self->lastTime;
    dest->weightedLastTime = self->weightedLastTime;
    NiObjectNET* target = self->target;
    if (target != nullptr) {
        // the cloning's map (NiTPointerMap<NiObject*, NiObject*>: buckets +0x8, size +0x4)
        void* map = At<void*>(cloning, 0);
        re::Ptr32<void>* buckets = At<re::Ptr32<void>*>(map, 8);
        re::Ptr32<void>* item = static_cast<re::Ptr32<void>*>(buckets[reinterpret_cast<uintptr_t>(target) % At<uint32_t>(map, 4)]);
        for (; item != nullptr; item = static_cast<re::Ptr32<void>*>(item[0])) {
            if (item[1] == target) {
                break;
            }
        }
        NiObjectNET* clone = nullptr;
        if (item != nullptr) {
            NiObjectNET* found = static_cast<NiObjectNET*>(item[2]);
            const void* foundRTTI = Virtual<const void*>(found, 2);
            if (Virtual<const void*>(target, 2) == foundRTTI) {
                clone = found;
            }
        }
        dest->target = clone;
    }
    if (self->next != nullptr) {
        NiTimeController* next = Virtual<NiTimeController*>(self->next, 3, cloning);   // CreateClone
        AssignReleasingFirst(dest->next, next);
    }
}

// vf4 LoadBinary (before 10.1.0.109 the manager-controlled bit is cleared; scaled time always on)
void __fastcall LoadBinary(NiTimeController* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    Stream_ReadLinkID(stream);   // the next controller
    LoadBytes(stream, &self->flags, 2);
    LoadBytes(stream, &self->frequency, 4);
    LoadBytes(stream, &self->phase, 4);
    LoadBytes(stream, &self->loKeyTime, 4);
    LoadBytes(stream, &self->hiKeyTime, 4);
    Stream_ReadLinkID(stream);   // the target
    if (StreamVersion(stream) < 0x0A01006D) {
        self->flags &= 0xFFDF;
    }
    self->flags = uint16_t((self->flags & 0xFF7F) | 0x40);
}

// vf7 SaveBinary: the next streamable controller (slot 26) as the link
void __fastcall SaveBinary(NiTimeController* self, re::Edx, void* stream) {
    Object_SaveBinaryNone(self, stream);
    NiTimeController* next = self->next;
    while (next != nullptr && !Virtual<uint8_t>(next, 26)) {
        next = next->next;
    }
    Virtual<void>(stream, 12, next);   // SaveLinkID
    SaveBytes(stream, &self->flags, 2);
    SaveBytes(stream, &self->frequency, 4);
    SaveBytes(stream, &self->phase, 4);
    SaveBytes(stream, &self->loKeyTime, 4);
    SaveBytes(stream, &self->hiKeyTime, 4);
    Virtual<void>(stream, 12, self->target);
}

NiTimeController* __fastcall DeletingDestructor(NiTimeController* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiTimeController_vtable);
    ReleaseImport(self->next);
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x34);
    }
    return self;
}

// the anim type's name (an unknown one: the buffer as allocated)
char* __cdecl AnimTypeString(const char* prefix, uint32_t type) {
    size_t size = std::strlen(prefix) + 0xC;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    if (type == 0) {
        NiSprintf(text, size, "%s = APP_TIME", prefix);
    } else if (type == 1) {
        NiSprintf(text, size, "%s = APP_INIT", prefix);
    }
    return text;
}

char* __cdecl CycleTypeString(const char* prefix, uint32_t type) {
    size_t size = std::strlen(prefix) + 0xB;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    if (type == 0) {
        NiSprintf(text, size, "%s = LOOP", prefix);
    } else if (type == 1) {
        NiSprintf(text, size, "%s = REVERSE", prefix);
    } else if (type != 2) {
        NiSprintf(text, size, "%s = UNKNOWN!!!", prefix);
    } else {
        NiSprintf(text, size, "%s = CLAMP", prefix);
    }
    return text;
}

void __fastcall GetViewerStrings(NiTimeController* self, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, GetViewerString(NiTimeController_RTTI.name));
    AddViewerString(strings, GetViewerStringFloat("m_fFrequency", self->frequency));
    AddViewerString(strings, GetViewerStringFloat("m_fPhase", self->phase));
    AddViewerString(strings, GetViewerStringFloat("m_fLoKeyTime", self->loKeyTime));
    AddViewerString(strings, GetViewerStringFloat("m_fHiKeyTime", self->hiKeyTime));
    AddViewerString(strings, GetViewerStringFloat("m_fStartTime", self->startTime));
    AddViewerString(strings, GetViewerStringFloat("m_fLastTime", self->lastTime));
    AddViewerString(strings, GetViewerStringPointer("m_pkTarget", self->target));
    uint8_t flags = uint8_t(self->flags);
    AddViewerString(strings, AnimTypeViewerString("anim type", flags & 1));
    AddViewerString(strings, CycleTypeViewerString("cycle type", (flags >> 1) & 3));
    AddViewerString(strings, GetViewerStringBool("Active", (flags >> 3) & 1));
    AddViewerString(strings, GetViewerStringBool("Play Backwards", (uint8_t(self->flags) >> 4) & 1));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NITIMECTRL_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NITIMECTRL_REVERSED("NiShaderTimeController::NiShaderTimeController", 0x0077D770, &ni::ShaderConstruct);
NITIMECTRL_REVERSED("NiShaderTimeController::~NiShaderTimeController", 0x0077D7A0, &ni::ShaderDestruct);
NITIMECTRL_REVERSED("NiShaderTimeController::GetRTTI", 0x0077D7B0, &ni::ShaderGetRTTI);
NITIMECTRL_REVERSED("NiShaderTimeController::DeletingDestructor", 0x0077D7C0, &ni::ShaderDeletingDestructor);
NITIMECTRL_REVERSED("NiTimeSyncController::IsEqual", 0x0077D7F0, &ni::SyncIsEqual);
NITIMECTRL_REVERSED("NiTimeSyncController::~NiTimeSyncController", 0x0077D810, &ni::SyncDestruct);
NITIMECTRL_REVERSED("NiTimeSyncController::RegisterStreamables", 0x0077D870, &ni::SyncRegisterStreamables);
NITIMECTRL_REVERSED("NiTimeSyncController::CreateClone", 0x0077D950, &ni::SyncCreateClone);
NITIMECTRL_REVERSED("NiTimeSyncController::GetViewerStrings", 0x0077DB40, &ni::SyncGetViewerStrings);
NITIMECTRL_REVERSED("NiTimeController::IsEqual", 0x0077DB90, &ni::IsEqual);
NITIMECTRL_REVERSED("NiTimeController::CountChain", 0x0077DC10, &ni::CountChain);
NITIMECTRL_REVERSED("NiTimeController::Start", 0x0077DC30, &ni::Start);
NITIMECTRL_REVERSED("NiTimeController::ProcessClone", 0x0077DEE0, &ni::ProcessClone);
NITIMECTRL_REVERSED("NiTimeController::LinkObject", 0x0077DF10, &ni::LinkObject);
NITIMECTRL_REVERSED("NiTimeController::RegisterStreamables", 0x0077DF70, &ni::RegisterStreamables);
NITIMECTRL_REVERSED("NiTimeController::NiTimeController", 0x0077DFA0, &ni::Construct);
NITIMECTRL_REVERSED("NiTimeController::GetRTTI", 0x0077E000, &ni::GetRTTI);
NITIMECTRL_REVERSED("NiTimeController::~NiTimeController", 0x0077E010, &ni::Destruct);
NITIMECTRL_REVERSED("NiTimeController::SetTarget", 0x0077E050, &ni::SetTarget);
NITIMECTRL_REVERSED("NiTimeController::CopyMembers", 0x0077E100, &ni::CopyMembers);
NITIMECTRL_REVERSED("NiTimeController::LoadBinary", 0x0077E1F0, &ni::LoadBinary);
NITIMECTRL_REVERSED("NiTimeController::SaveBinary", 0x0077E2F0, &ni::SaveBinary);
NITIMECTRL_REVERSED("NiTimeController::DeletingDestructor", 0x0077E3F0, &ni::DeletingDestructor);
NITIMECTRL_REVERSED("NiTimeController::GetAnimTypeViewerString", 0x0077E440, &ni::AnimTypeString);
NITIMECTRL_REVERSED("NiTimeController::GetCycleTypeViewerString", 0x0077E4B0, &ni::CycleTypeString);
NITIMECTRL_REVERSED("NiTimeController::GetViewerStrings", 0x0077E550, &ni::GetViewerStrings);
