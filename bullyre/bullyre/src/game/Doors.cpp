// The door manager (0xC3B808, 0x11CC bytes; its save block is "DoorSaveData"): peds
// pushing doors this frame, the per-door state bits saved with the game, the doors held
// open or shut for a while, and the areas cached around use-points. Also the door's
// side tests for a ped and its small open/close state machine.
// Doors are CPropAnims; the peds' door fields: +0x1DD4 the door it's at (a registered
// reference), +0x1538 still pushing, +0x157C the push amount.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVec {
    float x, y, z;
};

// a ped pushing a door (both registered references)
struct DoorPush {
    re::Ptr32<void> ped;         // +0
    re::Ptr32<void> door;        // +4
    float amount;      // +8
    int32_t unknownC;  // +0xC
};
static_assert(sizeof(DoorPush) == 0x10);

// a door's saved state, by its id
struct DoorState {
    int32_t id;      // +0
    uint32_t flags;  // +4 bits 0..2
};
static_assert(sizeof(DoorState) == 8);

// the floor area around one of a model's use-points (model info +0x80)
struct DoorArea {
    re::Ptr32<void> entity;          // +0 (a registered reference)
    uint8_t point;         // +4 the use-point
    uint8_t unknown5[3];   // +5
    float centreX;         // +8
    float centreY;         // +0xC
    float radius;          // +0x10
    float corners[4][2];   // +0x14 (CVector2Ds)
    uint32_t lastUsed;     // +0x34 the frame counter
};
static_assert(sizeof(DoorArea) == 0x38);

struct CDoorManager {
    DoorPush pushes[30];         // +0
    int32_t numPushes;           // +0x1E0
    uint8_t heldState[30][2];    // +0x1E4 the held doors' +0x184/+0x20 while suspended
    DoorState states[400];       // +0x220
    int32_t numStates;           // +0xEA0
    re::Ptr32<void> held[30];              // +0xEA4 (registered references)
    uint32_t numHeld;            // +0xF1C
    re::Ptr32<void> shut[30];              // +0xF20 (registered references)
    uint32_t numShut;            // +0xF98
    DoorArea areas[10];          // +0xF9C
};
static_assert(offsetof(CDoorManager, numPushes) == 0x1E0);
static_assert(offsetof(CDoorManager, heldState) == 0x1E4);
static_assert(offsetof(CDoorManager, states) == 0x220);
static_assert(offsetof(CDoorManager, numStates) == 0xEA0);
static_assert(offsetof(CDoorManager, held) == 0xEA4);
static_assert(offsetof(CDoorManager, numHeld) == 0xF1C);
static_assert(offsetof(CDoorManager, shut) == 0xF20);
static_assert(offsetof(CDoorManager, numShut) == 0xF98);
static_assert(offsetof(CDoorManager, areas) == 0xF9C);
static_assert(sizeof(CDoorManager) == 0x11CC);

// the save block (0xCB0 bytes used)
struct DoorSaveData {
    int32_t version;         // +0 (2)
    int32_t size;            // +4 (0xD10)
    int32_t capacity;        // +8 (400)
    DoorState states[400];   // +0xC
    int32_t numStates;       // +0xC8C
    char name[0x20];         // +0xC90 "DoorSaveData"
};
static_assert(offsetof(DoorSaveData, numStates) == 0xC8C);
static_assert(offsetof(DoorSaveData, name) == 0xC90);

GAME_VAR(uint8_t*, dm_playerPed, 0x00C1AEA8);
GAME_VAR(uint32_t, dm_frameCounter, 0x00C1A99C);
GAME_VAR(float, dm_timeStep, 0x00C1A9A4);
GAME_VAR(float, dm_animSpeed, 0x00A453D8);   // 0.02
GAME_VAR(int32_t, dm_doorAnim, 0x00C8E910);
GAME_VAR(int32_t, dm_poiModel, 0x00BF42DC);
GAME_VAR(uint8_t, dm_chapter, 0x00A147E0);
GAME_ARRAY(uint8_t*, dm_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, dm_currentUsePointNode, 0x01B649DC);
inline void* const dm_pushInput = reinterpret_cast<void*>(0x01B654E4);
inline void* const dm_renderer = reinterpret_cast<void*>(0x020C4910);
inline const char* const dm_saveName = reinterpret_cast<const char*>(0x00913D0C);   // "DoorSaveData"

GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<void>* ref, void* entity);
GAME_FN(uint8_t*, __thiscall, RefGet, 0x00773DB0, re::Ptr32<void>* ref);
GAME_FN(void, __thiscall, RefAssign, 0x00472CA0, re::Ptr32<void>* ref, void* entity);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void* ctor);
inline void* const dm_vector2DConstruct = reinterpret_cast<void*>(0x0066BEA0);
GAME_FN(uint8_t, __cdecl, Save_CheckValue, 0x00733320, int, int, int value, int expected);
GAME_FN(uint8_t, __cdecl, Save_CheckBlock, 0x00733620, const char* name, void* data, int size);
GAME_FN(void, __thiscall, DoorState_Save, 0x00733470, DoorState* out, DoorState* from);
GAME_FN(uint8_t, __cdecl, IsTimePaused, 0x00705500);
GAME_FN(uint8_t*, __thiscall, ObjectiveQueue_GetCurrent, 0x00470340, void* queue);
GAME_FN(uint8_t*, __thiscall, Task_GetTarget, 0x008B6120, void* task);
GAME_FN(uint8_t, __thiscall, Ped_IsDog, 0x004473E0, void* ped);
GAME_FN(CVec*, __thiscall, Entity_GetBoundCentre, 0x00466C70, void* entity, CVec* out);
GAME_FN(CVec*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVec* out);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVec* v);
GAME_FN(CVec*, __cdecl, VectorSub, 0x00414B70, CVec* out, const CVec* a, const CVec* b);
GAME_FN(void, __thiscall, UsePoint_GetArea, 0x00564700, void* point, void* entity, CVec* corners);
GAME_FN(uint8_t, __thiscall, Controller_IsNodePlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(uint8_t, __thiscall, Controller_HasNode, 0x005F48D0, void* controller, void* node, int, int);
GAME_FN(void, __thiscall, Controller_Stop, 0x005F40A0, void* controller);
GAME_FN(void, __thiscall, Controller_Update, 0x005F4B00, void* controller, float dt);
GAME_FN(void, __thiscall, Renderer_UpdateNode, 0x006C1910, void* renderer, void* node, float, int, uintptr_t);
// the door (CPropAnim) side
GAME_FN(uint8_t, __thiscall, Door_IsLocked, 0x004EA3A0, void* door);   // +0x17C
GAME_FN(void, __thiscall, Door_PlayAnim, 0x004EA450, void* door, int anim, int);
GAME_FN(void, __thiscall, Door_SetCurrent, 0x004ED940, void* door);
GAME_FN(void, __thiscall, Door_SetAnim, 0x004EE010, void* door, int anim);
GAME_FN(void, __thiscall, Door_GetPush, 0x004EE0D0, void* door, void* ped, float* amount);
GAME_FN(uint8_t, __thiscall, DoorLock_IsOpening, 0x004EA3D0, void* lock);
GAME_FN(uint8_t, __thiscall, DoorLock_IsClosing, 0x004EA400, void* lock);
// (ours, through their addresses)
GAME_FN(uint8_t, __thiscall, DM_HasPush, 0x004E9000, CDoorManager* self, void* ped, void* door);
GAME_FN(void, __thiscall, DM_InitStates, 0x004E9060, void* states);
GAME_FN(void, __thiscall, DM_InitList, 0x004E90A0, void* list);
GAME_FN(void, __thiscall, DM_Hold, 0x004E9600, CDoorManager* self, void* door);
GAME_FN(void, __thiscall, DM_Shut, 0x004E96A0, CDoorManager* self, void* door);
GAME_FN(uint8_t, __thiscall, DM_SetArea, 0x004E9FD0, CDoorManager* self, void* entity, int point, int slot);
GAME_FN(uint8_t, __thiscall, Door_CanPedPass, 0x004EAA10, void* door, void* ped);

const CVec& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVec*>(matrix + 0x30) : At<CVec>(entity, 4);
}

// the door pushed and the ped pushing it (a list entry)
uint8_t PushDoor(DoorPush& push) {
    return push.door != nullptr && push.ped != nullptr;
}

// 0x4E9000 (thiscall): does a ped push this door already
uint8_t __fastcall HasPush(CDoorManager* self, re::Edx, void* ped, void* door) {
    for (int i = 0; i < self->numPushes; i++) {
        DoorPush& push = self->pushes[i];
        if (PushDoor(push) && RefGet(&push.door) == door && RefGet(&push.ped) == ped) {
            return 1;
        }
    }
    return 0;
}

// 0x4E9060 (thiscall): the state block's constructor
void __fastcall InitStates(DoorState* states) {
    for (int i = 0; i < 400; i++) {
        states[i].id = 0;
        states[i].flags = 0;
    }
    *reinterpret_cast<int32_t*>(states + 400) = 0;
}

// 0x4E90A0 (thiscall): a door list's constructor (30 references and the count)
void __fastcall InitList(re::Ptr32<void>* list) {
    for (int i = 0; i < 31; i++) {
        list[i] = nullptr;
    }
}

// one of the door lists, pruned of doors gone or let go (a removal skips the entry that
// moves into its place until the next call)
void PruneList(re::Ptr32<void>* list, uint32_t& count) {
    for (uint32_t i = 0; i < count; i++) {
        if (list[i] != nullptr) {
            uint8_t* door = RefGet(&list[i]);
            if (door != nullptr && At<int32_t>(door, 0x184) != 0) {
                continue;
            }
        }
        for (uint32_t j = i; j < count - 1; j++) {
            SetRef(&list[j], list[j + 1]);
        }
        count--;
    }
}

// 0x4E9110 (thiscall): drops the held and shut doors that are gone or no longer held
void __fastcall PruneLists(CDoorManager* self) {
    PruneList(self->held, self->numHeld);
    PruneList(self->shut, self->numShut);
}

// 0x4E9290 (thiscall)
void __fastcall Clear(CDoorManager* self) {
    self->numStates = 0;
    self->numHeld = 0;
    self->numShut = 0;
    self->numPushes = 0;
}

// 0x4E92B0 (thiscall): this frame's pushes: per door the strongest pusher wins, the door
// takes its amount and animates; the list is emptied
void __fastcall ProcessPushes(CDoorManager* self) {
    if (self->numPushes == 0) {
        return;
    }
    uint8_t visited[30] = {};
    uint8_t* player = dm_playerPed;
    for (int i = 0; i < self->numPushes; i++) {
        DoorPush& push = self->pushes[i];
        if (!visited[i] && push.door != nullptr && push.ped != nullptr) {
            uint8_t* door = RefGet(&push.door);
            uint8_t* pusher = RefGet(&push.ped);
            Door_GetPush(door, pusher, &push.amount);
            for (int j = i + 1; j < self->numPushes; j++) {
                DoorPush& other = self->pushes[j];
                if (other.door == nullptr || other.ped == nullptr) {
                    continue;
                }
                uint8_t* otherDoor = RefGet(&other.door);
                uint8_t* otherPed = RefGet(&other.ped);
                if (!visited[j] && door == otherDoor) {
                    Door_GetPush(door, otherPed, &other.amount);
                    if (push.amount < other.amount) {
                        push.amount = other.amount;
                        At<float>(pusher, 0x157C) = 0.0f;
                        SetRef(&At<void*>(otherPed, 0x1DD4), door);
                        pusher = otherPed;
                        At<int32_t>(otherPed, 0x1538) = 1;
                    }
                    visited[j] = 1;
                }
            }
            At<float>(pusher, 0x157C) = push.amount;
            // the player left at the door without pushing it any more
            if (At<int32_t>(door, 0x194) != 0 && !Door_IsLocked(door) && 0.0f < At<float>(door, 0x3B0) &&
                !DM_HasPush(self, player, door)) {
                float amount;
                Door_GetPush(door, player, &amount);
                if (amount <= 0.0f) {
                    SetRef(&At<void*>(player, 0x1DD4), nullptr);
                    At<int32_t>(player, 0x1538) = 0;
                }
            }
            if (0.0f < push.amount) {
                SetRef(&At<void*>(pusher, 0x1DD4), door);
                // (0.85f read as a double)
                At<int32_t>(pusher, 0x1538) = push.amount < 0.85f ? 1 : 0;
                Door_SetCurrent(door);
                if (!Controller_IsNodePlaying(door + 0x128, dm_pushInput, 1, 1)) {
                    if (At<int32_t>(door, 0x184) == 0) {
                        Door_PlayAnim(door, 0xE, 1);
                    }
                    Door_SetAnim(door, dm_doorAnim);
                }
            } else {
                SetRef(&At<void*>(pusher, 0x1DD4), nullptr);
                At<int32_t>(pusher, 0x1538) = 0;
            }
            if (At<float>(door, 0x3B0) <= push.amount) {
                At<float>(door, 0x3B0) = push.amount;
                At<float>(door, 0x254) = 0.0f;
            }
            float dt = dm_animSpeed * dm_timeStep;
            Door_SetCurrent(door);
            Controller_Update(door + 0x128, dt);
            Renderer_UpdateNode(dm_renderer, At<void*>(door, 0x114), 1.0f, 1, 0);
        }
        visited[i] = 1;
    }
    for (int i = 0; i < self->numPushes; i++) {
        SetRef(&self->pushes[i].door, nullptr);
        SetRef(&self->pushes[i].ped, nullptr);
    }
    self->numPushes = 0;
}

// 0x4E9600 (thiscall): holds a door (added once, while there's room)
void __fastcall Hold(CDoorManager* self, re::Edx, void* door) {
    re::Ptr32<void> ref = nullptr;
    SetRef(&ref, door);
    uint32_t found = 0xFFFFFFFF;
    for (uint32_t i = 0; i < self->numHeld; i++) {
        if (self->held[i] == ref) {
            found = i;
            break;
        }
    }
    SetRef(&ref, nullptr);
    if (found == 0xFFFFFFFF && self->numHeld != 30) {
        // (the game builds the temporary reference in the argument's slot)
        re::Ptr32<void> temp = nullptr;
        SetRef(&temp, door);
        SetRef(&self->held[self->numHeld], temp);
        self->numHeld++;
        SetRef(&temp, nullptr);
    }
}

// 0x4E96A0 (thiscall): keeps a door shut (not one with +0x180 set)
void __fastcall Shut(CDoorManager* self, re::Edx, uint8_t* door) {
    bool found;
    bool madeRef = false;
    re::Ptr32<void> ref = nullptr;
    if (At<int32_t>(door, 0x180) != 0) {
        found = true;
    } else {
        SetRef(&ref, door);
        madeRef = true;
        found = false;
        for (uint32_t i = 0; i < self->numShut; i++) {
            if (self->shut[i] == ref) {
                found = true;
                break;
            }
        }
    }
    if (madeRef) {
        SetRef(&ref, nullptr);
    }
    if (!found && self->numShut != 30) {
        re::Ptr32<void> temp = nullptr;
        SetRef(&temp, door);
        SetRef(&self->shut[self->numShut], temp);
        self->numShut++;
        SetRef(&temp, nullptr);
    }
}

// 0x4E9760 (thiscall): the door states into the save block
void __fastcall Save(CDoorManager* self, re::Edx, DoorSaveData* out) {
    out->version = 2;
    out->size = 0xD10;
    out->capacity = 400;
    out->numStates = self->numStates;
    for (int i = 0; i < out->numStates; i++) {
        DoorState_Save(&out->states[i], &self->states[i]);
    }
}

// 0x4E97C0 (thiscall): and back (nothing when the block doesn't check out)
void __fastcall Load(CDoorManager* self, re::Edx, DoorSaveData* from) {
    self->numStates = 0;
    if (!Save_CheckValue(0, 0, from->version, 2) || !Save_CheckValue(0, 0, from->size, 0xD10) ||
        !Save_CheckValue(0, 0, from->capacity, 400) || !Save_CheckBlock(dm_saveName, from->name, 0x20)) {
        return;
    }
    for (int i = 0; i < from->numStates; i++) {
        self->states[self->numStates] = from->states[i];
        self->numStates++;
    }
}

// 0x4E9890 (thiscall): the constructor
CDoorManager* __fastcall Construct(CDoorManager* self) {
    for (int i = 0; i < 30; i++) {
        self->pushes[i].ped = nullptr;
        self->pushes[i].door = nullptr;
    }
    DM_InitStates(self->states);
    DM_InitList(self->held);
    DM_InitList(self->shut);
    for (int i = 0; i < 10; i++) {
        DoorArea& area = self->areas[i];
        area.entity = nullptr;
        VectorConstruct(area.corners, 8, 4, dm_vector2DConstruct);
        area.lastUsed = 0;
    }
    return self;
}

// 0x4E9910 (thiscall): the destructor (the references let go, last first)
void __fastcall Destruct(CDoorManager* self) {
    for (int i = 9; i >= 0; i--) {
        DoorArea& area = self->areas[i];
        if (area.entity != nullptr) {
            CleanUpOldReference(area.entity, &area.entity);
        }
    }
    for (int i = 29; i >= 0; i--) {
        SetRef(&self->shut[i], nullptr);
    }
    for (int i = 29; i >= 0; i--) {
        SetRef(&self->held[i], nullptr);
    }
    for (int i = 29; i >= 0; i--) {
        SetRef(&self->pushes[i].door, nullptr);
        SetRef(&self->pushes[i].ped, nullptr);
    }
}

int FindState(CDoorManager* self, int id) {
    for (uint32_t i = 0; i < uint32_t(self->numStates); i++) {
        if (self->states[i].id == id) {
            return int(i);
        }
    }
    return -1;
}

uint8_t GetStateBit(CDoorManager* self, int id, int bit) {
    int i = FindState(self, id);
    if (i == -1) {
        return 0;
    }
    return uint8_t((self->states[i].flags >> bit) & 1);
}

// a state bit set or cleared; an unknown door gets an entry with just that bit
void SetStateBit(CDoorManager* self, int id, int value, int bit) {
    uint32_t count = uint32_t(self->numStates);
    int i = FindState(self, id);
    if (i != -1) {
        if (re::Flag(value)) {
            self->states[i].flags |= 1u << bit;
        } else {
            self->states[i].flags &= ~(1u << bit);
        }
        return;
    }
    if (count < 400) {
        self->states[count].id = id;
        self->states[count].flags = re::Flag(value) ? 1u << bit : 0;
        self->numStates++;
    }
}

// 0x4E99A0 (thiscall)
uint8_t __fastcall GetState0(CDoorManager* self, re::Edx, int id) {
    return GetStateBit(self, id, 0);
}

// 0x4E99F0 (thiscall)
void __fastcall SetState0(CDoorManager* self, re::Edx, int id, int value) {
    SetStateBit(self, id, value, 0);
}

// 0x4E9A70 (thiscall)
uint8_t __fastcall GetState1(CDoorManager* self, re::Edx, int id) {
    return GetStateBit(self, id, 1);
}

// 0x4E9AC0 (thiscall)
void __fastcall SetState1(CDoorManager* self, re::Edx, int id, int value) {
    SetStateBit(self, id, value, 1);
}

// 0x4E9B40 (thiscall)
uint8_t __fastcall GetState2(CDoorManager* self, re::Edx, int id) {
    return GetStateBit(self, id, 2);
}

// 0x4E9B90 (thiscall)
void __fastcall SetState2(CDoorManager* self, re::Edx, int id, int value) {
    SetStateBit(self, id, value, 2);
}

// 0x4E9C10 (thiscall): all three bits (an unknown door is added with bit 0 = `value`;
// the outputs stay untouched when the table is full)
void __fastcall GetStates(CDoorManager* self, re::Edx, int id, int value, uint8_t* bit0, uint8_t* bit1,
                          uint8_t* bit2) {
    uint32_t count = uint32_t(self->numStates);
    int i = FindState(self, id);
    if (i == -1) {
        if (count >= 400) {
            return;
        }
        self->states[count].id = id;
        self->states[count].flags = re::Flag(value) ? 1 : 0;
        self->numStates++;
        i = self->numStates - 1;
    }
    *bit0 = uint8_t(self->states[i].flags & 1);
    *bit1 = uint8_t((self->states[i].flags >> 1) & 1);
    *bit2 = uint8_t((self->states[i].flags >> 2) & 1);
}

// 0x4E9CB0 (thiscall): a ped pushing a door this frame (0 when it may not; a door with
// +0x1A0 set is let through without being listed)
uint8_t __fastcall AddPush(CDoorManager* self, re::Edx, uint8_t* ped, uint8_t* door) {
    if (IsTimePaused()) {
        return 0;
    }
    if (ped != nullptr) {
        // (door can be null here: the game reads through it all the same)
        if (At<int32_t>(ped, 0x1310) == 0xD && Door_IsLocked(door)) {
            return 0;
        }
        if (At<int32_t>(ped, 0x1310) != 0xD) {
            if (At<int32_t>(door, 0x180) != 0) {
                return 0;
            }
            if (At<int32_t>(ped, 0x1310) != 0xD) {
                uint8_t* info = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0x40 / 4]))(ped);
                if (info[0x1D] != 0) {
                    return 0;
                }
            }
        }
        if (At<int16_t>(ped, 0x10E) == dm_poiModel) {
            return 0;
        }
        if (Ped_IsDog(ped)) {
            return 0;
        }
        if (ObjectiveQueue_GetCurrent(ped + 0x5F0) != nullptr && !Door_CanPedPass(door, ped)) {
            return 0;
        }
        if (door == nullptr) {
            goto add;
        }
        if (At<int32_t>(ped, 0x1310) != 0xD) {
            if (At<int32_t>(ped, 0x155C) != 0) {
                return 0;
            }
            if (At<int32_t>(ped, 0x1310) != 0xD) {
                goto through;
            }
        }
        if (At<uint8_t*>(ped, 0x155C) != nullptr && At<int32_t>(At<uint8_t*>(ped, 0x155C), 0x1DD4) == 0) {
            return 0;
        }
    }
through:
    if (door != nullptr && At<int32_t>(door, 0x1A0) != 0) {
        return 1;
    }
add:
    if (Door_IsLocked(door) || IsTimePaused()) {
        DM_Hold(self, door);
    }
    DM_Shut(self, door);
    if (self->numPushes < 30) {
        SetRef(&self->pushes[self->numPushes].ped, ped);
        SetRef(&self->pushes[self->numPushes].door, door);
        self->numPushes++;
    }
    return 1;
}

// 0x4E9DF0 (thiscall): suspends the held doors (their +0x184/+0x20 kept, made visible
// and let go) or restores them
void __fastcall SuspendHeld(CDoorManager* self, re::Edx, int suspend) {
    int count = int(self->numHeld);
    if (count == 0) {
        return;
    }
    if (re::Flag(suspend)) {
        for (int i = 0; i < count; i++) {
            if (self->held[i] == nullptr) {
                continue;
            }
            uint8_t* door = RefGet(&self->held[i]);
            self->heldState[i][0] = At<int32_t>(door, 0x184) != 0;
            bool visible = At<int32_t>(door, 0x20) != 0 && (At<int32_t>(door, 0x1C) != 0 || At<int32_t>(door, 0x60) != 0) &&
                           (dm_modelInfoPtrs[At<int16_t>(door, 0x10E)][0xB] & uint8_t(1 << (dm_chapter & 0x1F))) != 0 &&
                           At<int32_t>(door, 0xF4) == 0;
            self->heldState[i][1] = visible ? 1 : 0;
            At<int32_t>(door, 0x20) = 1;
            At<int32_t>(door, 0x184) = 0;
        }
    } else {
        for (int i = 0; i < count; i++) {
            if (self->held[i] == nullptr) {
                continue;
            }
            uint8_t* door = RefGet(&self->held[i]);
            At<int32_t>(door, 0x184) = self->heldState[i][0];
            At<int32_t>(door, 0x20) = self->heldState[i][1];
        }
    }
}

// 0x4E9EF0 (thiscall): the same for the shut doors (hidden while suspended; these share
// the held doors' saved bytes)
void __fastcall SuspendShut(CDoorManager* self, re::Edx, int suspend) {
    int count = int(self->numShut);
    if (count == 0) {
        return;
    }
    if (re::Flag(suspend)) {
        for (int i = 0; i < count; i++) {
            if (self->shut[i] == nullptr) {
                continue;
            }
            uint8_t* door = RefGet(&self->shut[i]);
            bool visible = At<int32_t>(door, 0x20) != 0 && (At<int32_t>(door, 0x1C) != 0 || At<int32_t>(door, 0x60) != 0) &&
                           (dm_modelInfoPtrs[At<int16_t>(door, 0x10E)][0xB] & uint8_t(1 << (dm_chapter & 0x1F))) != 0 &&
                           At<int32_t>(door, 0xF4) == 0;
            self->heldState[i][1] = visible ? 1 : 0;
            At<int32_t>(RefGet(&self->shut[i]), 0x20) = 0;
        }
    } else {
        for (int i = 0; i < count; i++) {
            if (self->shut[i] == nullptr) {
                continue;
            }
            uint8_t visible = self->heldState[i][1];
            At<int32_t>(RefGet(&self->shut[i]), 0x20) = visible;
        }
    }
}

// 0x4E9FD0 (thiscall): caches the area around an entity's use-point in a slot (0 when
// its model has no use-points)
uint8_t __fastcall SetArea(CDoorManager* self, re::Edx, uint8_t* entity, int point, int slot) {
    uint8_t* points = At<uint8_t*>(dm_modelInfoPtrs[At<int16_t>(entity, 0x10E)], 0x80);
    if (points == nullptr) {
        return 0;
    }
    DoorArea& area = self->areas[slot];
    if (entity != area.entity) {
        if (area.entity != nullptr) {
            CleanUpOldReference(area.entity, &area.entity);
        }
        area.entity = entity;
        RegisterReference(entity, &area.entity);
    }
    area.point = uint8_t(point);
    uint8_t* usePoint = At<uint8_t*>(At<uint8_t*>(points, 0x38), 8) + point * 0x24;
    CVec corners[4] = {};
    UsePoint_GetArea(usePoint, entity, corners);
    for (int i = 0; i < 4; i++) {
        area.corners[i][0] = corners[i].x;
        area.corners[i][1] = corners[i].y;
    }
    float sumX = area.corners[0][0] + area.corners[1][0];
    float sumY = area.corners[0][1] + area.corners[1][1];
    sumX = area.corners[2][0] + sumX;
    sumY = area.corners[2][1] + sumY;
    sumX = sumX + area.corners[3][0];
    sumY = area.corners[3][1] + sumY;
    area.centreX = float(sumX * 0.25);
    area.centreY = float(sumY * 0.25);
    // half the use-point's width (its first two corners)
    float dx = At<float>(usePoint, 0x10) - At<float>(usePoint, 0);
    float dy = At<float>(usePoint, 0x14) - At<float>(usePoint, 4);
    float dx2 = dx * dx;
    float dy2 = dy * dy;
    float length = std::sqrt(std::fabs(float(dx2 + dy2)));
    area.radius = float(length * 0.5);
    area.lastUsed = dm_frameCounter;
    return 1;
}

// 0x4EA210 (thiscall): the cached area for an entity's use-point, made in the least
// recently used slot when it's not there (null when it can't be)
DoorArea* __fastcall GetArea(CDoorManager* self, re::Edx, void* entity, int point) {
    int oldest = -1;
    uint32_t oldestTime = 0x7FFFFFFF;
    for (int i = 0; i < 10; i++) {
        DoorArea& area = self->areas[i];
        if (area.entity == entity && uint32_t(area.point) == uint32_t(point)) {
            area.lastUsed = dm_frameCounter;
            return &area;
        }
        if (area.lastUsed < oldestTime) {
            oldestTime = area.lastUsed;
            oldest = i;
        }
    }
    if (DM_SetArea(self, entity, point, oldest)) {
        return &self->areas[oldest];
    }
    return nullptr;
}

// 0x4EA8C0 (thiscall): which way a ped faces the door: towards its centre, and towards
// the entity at the door's +0x23C (each a dot of at least 0.75, flat)
void __fastcall Door_GetFacing(uint8_t* door, re::Edx, uint8_t* ped, uint8_t* towardsCentre, uint8_t* towardsOther) {
    CVec centre;
    Entity_GetBoundCentre(door, &centre);
    uint8_t* other = At<uint8_t*>(door, 0x23C);
    centre.z = Position(door).z;
    CVec forward;
    Entity_GetForward(ped, &forward);
    forward.z = 0.0f;
    Vector_Normalise(&forward);
    CVec toCentre;
    VectorSub(&toCentre, &centre, &Position(ped));
    toCentre.z = 0.0f;
    Vector_Normalise(&toCentre);
    CVec toOther;
    VectorSub(&toOther, reinterpret_cast<CVec*>(other + 4), &Position(ped));
    toOther.z = 0.0f;
    Vector_Normalise(&toOther);
    float dot = float(double(toCentre.x) * forward.x + double(forward.y) * toCentre.y + double(toCentre.z) * forward.z);
    *towardsCentre = dot >= 0.75f ? 1 : 0;
    dot = float(double(forward.x) * toOther.x + double(forward.y) * toOther.y + double(forward.z) * toOther.z);
    *towardsOther = dot >= 0.75f ? 1 : 0;
}

// 0x4EAA10 (thiscall): may a ped pass the door: 0 when its current objective (type 7)
// targets something on the same side of the door as the ped is (or for no ped)
uint8_t __fastcall Door_CanPass(uint8_t* door, re::Edx, uint8_t* ped) {
    if (ped == nullptr) {
        return 0;
    }
    uint8_t* objective = ObjectiveQueue_GetCurrent(ped + 0x5F0);
    if (objective == nullptr || At<int32_t>(objective, 8) != 7) {
        return 1;
    }
    uint8_t* target = Task_GetTarget(objective);
    if (target == nullptr) {
        return 1;
    }
    CVec targetPos = Position(target);
    CVec doorPos = Position(door);
    CVec toTarget;
    VectorSub(&toTarget, &targetPos, &doorPos);
    toTarget.z = 0.0f;
    Vector_Normalise(&toTarget);
    CVec forward;
    Entity_GetForward(door, &forward);
    forward.z = 0.0f;
    Vector_Normalise(&forward);
    CVec toPed;
    VectorSub(&toPed, &Position(ped), &doorPos);
    toPed.z = 0.0f;
    Vector_Normalise(&toPed);
    float targetSide = float(double(forward.x) * toTarget.x + double(forward.y) * toTarget.y + double(forward.z) * toTarget.z);
    float pedSide = float(double(forward.x) * toPed.x + double(forward.y) * toPed.y + double(forward.z) * toPed.z);
    if (targetSide > 0.0f && pedSide > 0.0f) {
        return 0;
    }
    if (pedSide < 0.0f && targetSide < 0.0f) {
        return 0;
    }
    return 1;
}

// 0x4EAC30 (thiscall): stops the peds using this prop (with `own`: the peds whose
// use-point entity it is; else those on the same use-point id, or whose action
// controller runs the prop's top action node)
void __fastcall Prop_StopUsers(uint8_t* prop, re::Edx, int own) {
    auto pool = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(0x00C0F5F0);   // CPools::ms_pPedPool
    uint8_t* objects = re::PtrCast<uint8_t*>(pool[0]);
    uint8_t* flags = re::PtrCast<uint8_t*>(pool[1]);
    int elemSize = re::PtrCast<int32_t*>(pool)[3];
    for (int i = 0; i < re::PtrCast<int32_t*>(pool)[2]; i++) {
        if (flags[i] & 0x80) {
            continue;
        }
        uint8_t* ped = objects + elemSize * i;
        if (ped == nullptr) {
            continue;
        }
        uint8_t* used = At<uint8_t*>(ped, 0x548);
        bool stop;
        if (re::Flag(own)) {
            stop = used == prop;
        } else if (used != nullptr && At<int16_t>(used, 0x22C) == At<int16_t>(prop, 0x22C)) {
            stop = true;
        } else {
            void* node = At<void*>(prop, 0x228);
            if (node == nullptr || At<int16_t>(prop, 0x22C) == -1) {
                continue;
            }
            auto parent = [](void* n) {
                return reinterpret_cast<void*(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(n))[0x34 / 4]))(n, -1);
            };
            int top = dm_currentUsePointNode;
            if (reinterpret_cast<intptr_t>(parent(node)) != top) {
                do {
                    node = parent(node);
                    top = dm_currentUsePointNode;
                } while (reinterpret_cast<intptr_t>(parent(node)) != top);
            }
            stop = Controller_HasNode(At<uint8_t*>(ped, 0x2E0) + 0x258, node, 1, 0) != 0;
        }
        if (stop) {
            Controller_Stop(At<uint8_t*>(ped, 0x2E0) + 0x258);
        }
    }
}

// a door's lock: 0 open, 1 locking, 2 locked, 3 forced (by a ped of type 0xD); the ped
// it's for (a registered reference); +8 and +0xC a flag and a timer
struct DoorLock {
    int32_t state;   // +0
    re::Ptr32<void> ped;       // +4
    uint8_t active;  // +8
    uint8_t unknown9[3];
    float timer;     // +0xC
};
static_assert(sizeof(DoorLock) == 0x10);

// 0x4EAD40 (thiscall): is the lock done with a request
uint8_t __fastcall DoorLock_IsDone(DoorLock* self, re::Edx, uint8_t* ped, int request) {
    switch (request) {
    case 0:
        if (ped == self->ped) {
            return 1;
        }
        break;
    case 1:
        if (self->state == 0 || (self->active != 0 && 20.0f < self->timer)) {
            return 1;
        }
        break;
    case 2:
        if (ped == self->ped && DoorLock_IsOpening(self)) {
            return 1;
        }
        break;
    case 3:
        if (At<int32_t>(ped, 0x1310) == 0xD && !DoorLock_IsClosing(self)) {
            return 1;
        }
        break;
    }
    return 0;
}

// 0x4EADC0 (thiscall): a request to the lock (1 when taken)
uint8_t __fastcall DoorLock_Request(DoorLock* self, re::Edx, uint8_t* ped, int request, int) {
    switch (request) {
    case 0:
        if (self->ped == ped) {
            SetRef(&self->ped, nullptr);
            self->timer = 0.0f;
            self->state = 0;
            return 1;
        }
        break;
    case 1:
        if (self->state == 0 && self->ped == nullptr) {
            RefAssign(&self->ped, ped);
            self->timer = 0.0f;
            self->state = 1;
            return 1;
        }
        break;
    case 2:
        if (self->state == 1 && self->ped == ped) {
            self->state = 2;
            self->timer = 0.0f;
            return 1;
        }
        break;
    case 3:
        if (At<int32_t>(ped, 0x1310) == 0xD && self->state != 2) {
            RefAssign(&self->ped, ped);
            self->timer = 0.0f;
            self->state = 3;
            return 1;
        }
        break;
    }
    return 0;
}

#define C13B_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C13B_REVERSED("CDoorManager::HasPush", 0x004E9000, &HasPush);
C13B_REVERSED("CDoorManager::InitStates", 0x004E9060, &InitStates);
C13B_REVERSED("CDoorManager::InitList", 0x004E90A0, &InitList);
C13B_REVERSED("CDoorManager::PruneLists", 0x004E9110, &PruneLists);
C13B_REVERSED("CDoorManager::Clear", 0x004E9290, &Clear);
C13B_REVERSED("CDoorManager::ProcessPushes", 0x004E92B0, &ProcessPushes);
C13B_REVERSED("CDoorManager::Hold", 0x004E9600, &Hold);
C13B_REVERSED("CDoorManager::Shut", 0x004E96A0, &Shut);
C13B_REVERSED("CDoorManager::Save", 0x004E9760, &Save);
C13B_REVERSED("CDoorManager::Load", 0x004E97C0, &Load);
C13B_REVERSED("CDoorManager::Construct", 0x004E9890, &Construct);
C13B_REVERSED("CDoorManager::Destruct", 0x004E9910, &Destruct);
C13B_REVERSED("CDoorManager::GetState0", 0x004E99A0, &GetState0);
C13B_REVERSED("CDoorManager::SetState0", 0x004E99F0, &SetState0);
C13B_REVERSED("CDoorManager::GetState1", 0x004E9A70, &GetState1);
C13B_REVERSED("CDoorManager::SetState1", 0x004E9AC0, &SetState1);
C13B_REVERSED("CDoorManager::GetState2", 0x004E9B40, &GetState2);
C13B_REVERSED("CDoorManager::SetState2", 0x004E9B90, &SetState2);
C13B_REVERSED("CDoorManager::GetStates", 0x004E9C10, &GetStates);
C13B_REVERSED("CDoorManager::AddPush", 0x004E9CB0, &AddPush);
C13B_REVERSED("CDoorManager::SuspendHeld", 0x004E9DF0, &SuspendHeld);
C13B_REVERSED("CDoorManager::SuspendShut", 0x004E9EF0, &SuspendShut);
C13B_REVERSED("CDoorManager::SetArea", 0x004E9FD0, &SetArea);
C13B_REVERSED("CDoorManager::GetArea", 0x004EA210, &GetArea);
C13B_REVERSED("CPropAnim::GetFacing", 0x004EA8C0, &Door_GetFacing);
C13B_REVERSED("CPropAnim::CanPedPass", 0x004EAA10, &Door_CanPass);
C13B_REVERSED("CPropAnim::StopUsers", 0x004EAC30, &Prop_StopUsers);
C13B_REVERSED("DoorLock::IsDone", 0x004EAD40, &DoorLock_IsDone);
C13B_REVERSED("DoorLock::Request", 0x004EADC0, &DoorLock_Request);

} // namespace
