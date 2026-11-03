// The population's model list and spawn ring (PedPopulation.h's PedModelList and
// PedSpawnRing): making, adding round the ring, the lookup, the clear (0x49BFD0 ..
// 0x49C0B7).

#include <cstdint>

#include "game/PedPopulation.h"

namespace {
GAME_FN(void*, __cdecl, PML_Malloc, 0x005EEAA0, size_t size);

// 0x49BFD0 (thiscall): `size` + 1 slots, all -1
PedModelList* __fastcall ModelList_Construct(PedModelList* self, re::Edx, int size) {
    int count = size + 1;
    uint64_t bytes = uint64_t(uint32_t(count)) * 4;
    self->models = static_cast<int32_t*>(PML_Malloc(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : size_t(bytes)));
    self->count = count;
    self->unknown8 = count;
    self->cursor = 0;
    for (int i = 0; i < self->count; i++) {
        self->models[i] = -1;
    }
    return self;
}

// 0x49C030 (thiscall): at the cursor, which moves on round the list (returns whether
// it wrapped: (cursor + 1) / count)
int __fastcall ModelList_Add(PedModelList* self, re::Edx, int model) {
    self->models[self->cursor] = model;
    int next = self->cursor + 1;
    self->cursor = next % self->count;
    return next / self->count;
}

// 0x49C050 (thiscall)
uint8_t __fastcall ModelList_Contains(PedModelList* self, re::Edx, int model) {
    for (int i = 0; i < self->count; i++) {
        if (self->models[i] == model) {
            return 1;
        }
    }
    return 0;
}

// 0x49C080 (thiscall): every record free (model -1)
void __fastcall SpawnRing_Clear(PedSpawnRing* self, re::Edx) {
    self->unknown8 = self->size;
    self->cursor = 0;
    for (int i = 0; i < self->size; i++) {
        self->entries[i].model = -1;
        self->entries[i].entity = nullptr;
        self->entries[i].time = 0;
    }
}
} // namespace

#define C08P4_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P4_REVERSED("PedModelList::Construct", 0x0049BFD0, &ModelList_Construct);
C08P4_REVERSED("PedModelList::Add", 0x0049C030, &ModelList_Add);
C08P4_REVERSED("PedModelList::Contains", 0x0049C050, &ModelList_Contains);
C08P4_REVERSED("PedSpawnRing::Clear", 0x0049C080, &SpawnRing_Clear);
