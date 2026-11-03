// SpeechLib (0xCF4E10, vtable 0x9211A8): the lines' categories (loaded from its blob, +0x498)
// with their used bits (+0xC84), picking a line, and the queue of 16 requests (+4) with its
// linked list (+0x48C head, +0x490 tail, +0x494 count). 0x59EF60..0x59F9D0.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

// a line of a category (4 bytes)
struct SpeechLine {
    uint8_t model;     // +0 (compared through 0x59E8E0's groups)
    uint8_t variant;   // +1
    uint16_t index;    // +2 the line's number (its used bit)
};
static_assert(sizeof(SpeechLine) == 4);

// a category (0xC; +0x4D0 points at them by id)
struct SpeechCategory {
    uint16_t id;          // +0
    uint16_t count;       // +2
    uint32_t flags;       // +4 (its low byte: the last model; 0x100 in order; 0x400 used ones again;
                          //     +6 the last line)
    re::Ptr32<SpeechLine> lines;    // +8
};
static_assert(sizeof(SpeechCategory) == 0xC);

// a queued request (0x48)
struct SpeechRequest {
    uint8_t active;        // +0
    uint8_t pad1[3];
    uint32_t event[12];    // +4 the event (0x30): id 0..8, model 9..16, type 17..24; +4 variant
                           //    (0..10); +8 flags; +0xC channel (&3); +0x10 time out
    re::Ptr32<void> item;            // +0x34
    uint32_t start;        // +0x38
    int32_t arg;           // +0x3C
    re::Ptr32<SpeechRequest> prev;   // +0x40
    re::Ptr32<SpeechRequest> next;   // +0x44
};
static_assert(sizeof(SpeechRequest) == 0x48);

// the list the blob is copied into (+0xC90)
struct SpeechCategoryList {
    uint32_t count;              // +0
    re::Ptr32<SpeechCategory> categories;  // +4
};

inline uint16_t* const scratchLines = reinterpret_cast<uint16_t*>(0x00BD7800);
inline void* const theSpeech = reinterpret_cast<void*>(0x00CF4378);
GAME_VAR(int32_t, speechQueueLengthStat, 0x00AE7CBC);
GAME_VAR(uint8_t, streamsBlocked, 0x00CF4286);

GAME_FN(uint8_t, __cdecl, SL_ModelsMatch, 0x0059E8E0, int model, int other);
GAME_FN(void*, __cdecl, SL_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void*, __cdecl, SL_MallocArray, 0x005EEA50, uint32_t size, int arg);
GAME_FN(void, __cdecl, SL_FreeArray, 0x005EEA60, void* p);
GAME_FN(void, __thiscall, SL_BitMatrixInit, 0x00598920, void* bits, int32_t width, int32_t height);
GAME_FN(void, __thiscall, SL_RunDoneCallback, 0x005A28A0, void* speech, void* event, uintptr_t arg);
GAME_FN(int32_t, __thiscall, SL_CheckStart, 0x005A25B0, void* speech, void* event, uintptr_t arg);
GAME_FN(int32_t, __stdcall, SL_CompareDifferences, 0x005A0780, int32_t a, int32_t b, int32_t c, int32_t d);
// its own
GAME_FN(uint16_t, __thiscall, SL_FindLines, 0x0059EF60, void* lib, uint16_t* out, SpeechCategory* category,
        uint32_t* event, uint16_t* usedCount);
GAME_FN(uint8_t, __thiscall, SL_ClearUsed, 0x0059F060, void* lib, SpeechCategory* category, uint32_t* event);
GAME_FN(void, __thiscall, SL_Remove, 0x0059F310, void* lib, SpeechRequest* request);

using SpeechTimeFn = uint32_t(__cdecl*)();
using SpeechRandFn = uint32_t(__cdecl*)(int low, int high);
using SpeechLoadFn = int32_t(__cdecl*)(const char* path, void* buffer, int size, int);
using SpeechPickedFn = void(__cdecl*)(uint32_t* event, SpeechCategory* category, SpeechLine* line, uint32_t* out);
using SpeechDoneFn = void(__cdecl*)(uint32_t* event, void* item, uint32_t reason);
using SpeechVf0Fn = int32_t(__cdecl*)(void* a1, void* a2, const char* a3, int32_t a4);
using SpeechStartFn = int32_t(__thiscall*)(void* lib, uint32_t* event, void* item, const char* name, int32_t arg);

SpeechRequest* RequestAt(uint8_t* lib, int i) {
    return reinterpret_cast<SpeechRequest*>(lib + 4 + i * 0x48);
}

uint8_t* UsedBits(uint8_t* lib) {
    return At<uint8_t*>(lib, 0xC8C);
}

// a model byte: bits 9..16 of the event's first word
inline uint32_t ModelOf(uint32_t word0) {
    return word0 >> 9 & 0xFF;
}

// 0x59EF60 (thiscall): the category's lines for the event's model (any with its flag 4) and
// variant not yet used into out; the used ones counted, and when there are none unused, out[0]
// is the first used one (-1 none)
uint16_t __fastcall SpeechLib_FindLines(uint8_t* self, re::Edx, uint16_t* out, SpeechCategory* category,
                                        uint32_t* event, uint16_t* usedCount) {
    int count = category->count;
    int found = 0;
    int firstUsed = -1;
    *usedCount = 0;
    for (int i = 0; i < count; ++i) {
        SpeechLine* line = &category->lines[i];
        if ((event[2] & 4) == 0 && SL_ModelsMatch(int(event[0] >> 9), line->model) == 0) {
            continue;
        }
        if ((event[1] & 0x7FF) != line->variant) {
            continue;
        }
        int index = line->index;
        if ((UsedBits(self)[index >> 3] & uint8_t(1 << (index & 7))) != 0) {
            *usedCount = uint16_t(*usedCount + 1);
            if (firstUsed < 0) {
                firstUsed = i;
            }
        } else {
            out[found] = uint16_t(i);
            ++found;
        }
    }
    if (found == 0) {
        out[0] = uint16_t(firstUsed);
    }
    return uint16_t(found);
}

// 0x59F060 (thiscall): the used bits of the category's lines for the event's model (any with
// its flag 4) cleared; were there any
uint8_t __fastcall SpeechLib_ClearUsed(uint8_t* self, re::Edx, SpeechCategory* category, uint32_t* event) {
    uint8_t cleared = 0;
    if (category == nullptr) {
        return 0;
    }
    int count = category->count;
    for (int i = 0; i < count; ++i) {
        SpeechLine* line = &category->lines[i];
        if ((event[2] & 4) == 0 && SL_ModelsMatch(int(event[0] >> 9), line->model) == 0) {
            continue;
        }
        int index = line->index;
        UsedBits(self)[index >> 3] &= uint8_t(~(1 << (index & 7)));
        cleared = 1;
    }
    return cleared;
}

// 0x59F110 (thiscall): the requests of a model lose +0x18
void __fastcall SpeechLib_ForgetModel(uint8_t* self, re::Edx, uint32_t model) {
    for (int i = 0; i < 16; ++i) {
        SpeechRequest* r = RequestAt(self, i);
        if (r->active != 0 && ModelOf(r->event[0]) == model) {
            r->event[5] = 0;
        }
    }
}

// 0x59F1A0 (thiscall): is a request queued for the ped's model (null: any), id (-1 any) and
// variant (-1 any)
uint8_t __fastcall SpeechLib_IsQueued(uint8_t* self, re::Edx, uint8_t* ped, int id, int variant) {
    for (int i = 0; i < 16; ++i) {
        SpeechRequest* r = RequestAt(self, i);
        if (r->active == 0) {
            continue;
        }
        if (ped != nullptr && At<uint16_t>(ped, 0x10E) != uint16_t(ModelOf(r->event[0]))) {
            continue;
        }
        if (id != -1 && int(r->event[0] & 0x1FF) != id) {
            continue;
        }
        if (variant == -1 || int(r->event[1] & 0x7FF) == variant) {
            return 1;
        }
    }
    return 0;
}

// 0x59F210 (thiscall): the blob (+0x498, 0x50000 bytes) read through the loader (+0x4A8); no
// loader: whether a path was given
int32_t __fastcall SpeechLib_Load(uint8_t* self, re::Edx, const char* path) {
    SpeechLoadFn load = At<SpeechLoadFn>(self, 0x4A8);
    if (load == nullptr) {
        return path != nullptr ? 1 : 0;
    }
    if (At<void*>(self, 0x498) != nullptr) {
        SL_FreeArray(At<void*>(self, 0x498));
    }
    At<void*>(self, 0x498) = nullptr;
    void* buffer = SL_MallocArray(0x50000, 0x7FFFFFFF);
    At<void*>(self, 0x498) = buffer;
    int32_t read = load(path, buffer, 0x50000, 0);
    return read != 0 ? 1 : 0;
}

// 0x59F290 (thiscall): the queue emptied
void __fastcall SpeechLib_Reset(uint8_t* self) {
    for (int i = 0; i < 16; ++i) {
        SpeechRequest* r = RequestAt(self, i);
        uint32_t word0 = r->event[0];
        r->event[1] &= 0xFFFFF800u;
        reinterpret_cast<uint8_t*>(&r->event[2])[0] &= 0xFE;
        r->event[4] = 0;
        r->event[5] = 0;
        r->event[6] = 0;
        r->event[0] = (word0 & 0xFE0200FFu) | 0x200FFu;
        r->active = 0;
        r->item = nullptr;
        r->start = 0;
        r->prev = nullptr;
        r->next = nullptr;
        r->arg = 0;
    }
    At<uint32_t>(self, 0x484) &= 0x1E0000u;
    At<void*>(self, 0x48C) = nullptr;
    At<void*>(self, 0x490) = nullptr;
    At<int32_t>(self, 0x488) = 0;
    At<int32_t>(self, 0x494) = 0;
}

// 0x59F310 (thiscall): a request's done callback run, then freed and unlinked
void __fastcall SpeechLib_RemoveImpl(uint8_t* self, re::Edx, SpeechRequest* r) {
    SL_RunDoneCallback(theSpeech, r->event, r->arg);
    r->event[0] = (r->event[0] & 0xFFFFFEFFu) | 0xFF;
    r->active = 0;
    r->item = nullptr;
    r->arg = 0;
    if (r == At<SpeechRequest*>(self, 0x48C)) {
        At<SpeechRequest*>(self, 0x48C) = r->next;
    } else if (r->prev != nullptr) {
        r->prev->next = r->next;
    }
    if (r == At<SpeechRequest*>(self, 0x490)) {
        At<int32_t>(self, 0x494) -= 1;
        At<SpeechRequest*>(self, 0x490) = r->prev;
    } else {
        if (r->next != nullptr) {
            r->next->prev = r->prev;
        }
        At<int32_t>(self, 0x494) -= 1;
    }
    r->prev = nullptr;
    r->next = nullptr;
}

// 0x59F3B0 (thiscall, vf0): through the function at +0x4B4 (none: -7)
int32_t __fastcall SpeechLib_CallStart(uint8_t* self, re::Edx, void* a1, void* a2, const char* a3, int32_t a4) {
    SpeechVf0Fn fn = At<SpeechVf0Fn>(self, 0x4B4);
    if (fn == nullptr) {
        return -7;
    }
    return fn(a1, a2, a3, a4);
}

// 0x59F3E0 (thiscall): has the category (id) a line for the model
uint8_t __fastcall SpeechLib_HasLine(uint8_t* self, re::Edx, int id, int model) {
    SpeechCategory* category = At<SpeechCategory*>(self, 0x4D0 + id * 4);
    if (category == nullptr || category->count == 0) {
        return 0;
    }
    int count = category->count;
    for (int i = 0; i < count; ++i) {
        if (SL_ModelsMatch(model, category->lines[i].model) != 0) {
            return 1;
        }
    }
    return 0;
}

// the inlined size of an n-element array (overflow: all ones)
size_t ArraySize(uint32_t n, uint32_t size) {
    uint64_t bytes = uint64_t(n) * size;
    return bytes > 0xFFFFFFFFull ? size_t(0xFFFFFFFFu) : size_t(bytes);
}

// 0x59F440 (thiscall): the blob into the category list (+0xC90), each id's category (+0x4D0,
// 0x1ED of them; missing: one past the list's end) and the used bits sized for the highest line
uint32_t __fastcall SpeechLib_BuildCategories(uint8_t* self) {
    uint32_t highest = 0;
    SpeechCategoryList* list = static_cast<SpeechCategoryList*>(SL_Malloc(8));
    const uint32_t* blob = At<uint32_t*>(self, 0x498);
    At<SpeechCategoryList*>(self, 0xC90) = list;
    list->count = blob[0];
    list->categories = static_cast<SpeechCategory*>(SL_Malloc(ArraySize(blob[0], 0xC)));
    const uint32_t* p = At<uint32_t*>(self, 0x498) + 1;
    for (uint32_t i = 0; i < At<SpeechCategoryList*>(self, 0xC90)->count; ++i) {
        uint32_t* c = reinterpret_cast<uint32_t*>(&At<SpeechCategoryList*>(self, 0xC90)->categories[i]);
        c[0] = p[0];
        c[1] = p[1];
        c[2] = p[2];
        p += 2;
        SpeechCategory* category = &At<SpeechCategoryList*>(self, 0xC90)->categories[i];
        void* lines = SL_Malloc(ArraySize(category->count, 4));
        At<SpeechCategoryList*>(self, 0xC90)->categories[i].lines = static_cast<SpeechLine*>(lines);
        for (int j = 0; j < At<SpeechCategoryList*>(self, 0xC90)->categories[i].count; ++j) {
            re::PtrCast<uint32_t*>(At<SpeechCategoryList*>(self, 0xC90)->categories[i].lines)[j] = *p;
            ++p;
        }
    }
    int id = 0;
    for (; id < 0x1ED; ++id) {
        SpeechCategoryList* l = At<SpeechCategoryList*>(self, 0xC90);
        SpeechCategory* found = nullptr;
        if (l->count != 0) {
            found = l->categories;
            for (uint32_t c = 0; c < At<SpeechCategoryList*>(self, 0xC90)->count; ++c, ++found) {
                if (uint32_t(id & 0x1FF) == found->id) {
                    break;
                }
            }
        }
        At<SpeechCategory*>(self, 0x4D0 + id * 4) = found;
        if (found->count != 0) {   // (the game reads a null or past-the-end one too)
            const SpeechLine* line = found->lines;
            for (uint32_t n = At<SpeechCategory*>(self, 0x4D0 + id * 4)->count; n != 0; --n, ++line) {
                if (int(line->index) > int(highest)) {
                    highest = line->index;
                }
            }
        }
    }
    SL_BitMatrixInit(self + 0xC84, int32_t(highest + 1), 1);
    return id == 0x1ED ? 1 : 0;
}

// 0x59F620 (thiscall): a line of the category for the event into *out (its used bit set, the
// picked callback +0x4A4 told): in order (0x100) or at random (+0x4AC, not the last one again);
// when all are used, again from the start (0x400) or the first used one. -4 empty category,
// -2 nothing for the model, -5 no random, -9 only the last one
int32_t __fastcall SpeechLib_PickLine(uint8_t* self, re::Edx, SpeechCategory* category, uint32_t* event, uint32_t* out) {
    uint16_t count = category->count;
    uint16_t used = 0;
    *out = 0;
    if (count == 0) {
        return -4;
    }
    uint16_t n = SL_FindLines(self, scratchLines, category, event, &used);
    if (n == 0) {
        if (used == 0) {
            return -2;
        }
        if ((category->flags & 0x400) == 0) {
            *out = category->lines[scratchLines[0]].index;
            return 0;
        }
        if (SL_ClearUsed(self, category, event) == 0) {
            return -2;
        }
        n = SL_FindLines(self, scratchLines, category, event, &used);
        if (n == 0) {
            return -2;
        }
    }
    SpeechRandFn random = At<SpeechRandFn>(self, 0x4AC);
    if (random == nullptr) {
        return -5;
    }
    uint16_t& last = *reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(category) + 6);
    uint16_t pick;
    if ((category->flags & 0x100) != 0) {
        pick = uint16_t(last + 1);
        if (pick >= count) {
            if ((category->flags & 0x400) == 0) {
                return 0;
            }
            pick = scratchLines[0];
        }
    } else if (count <= 1 || n <= 1) {
        pick = scratchLines[0];
    } else {
        int tries = 0;
        do {
            uint16_t r = uint16_t(random(0, n - 1));
            pick = scratchLines[r];
            if (pick != last) {
                break;
            }
            ++tries;
        } while (tries < 0x14);
        if (pick == last) {
            return -9;
        }
    }
    last = pick;
    SpeechLine* line = &category->lines[pick];
    *reinterpret_cast<uint8_t*>(&category->flags) = uint8_t(event[0] >> 9);
    *out = 0;
    *out = line->index;
    int index = line->index;
    UsedBits(self)[index >> 3] |= uint8_t(1 << (index & 7));
    SpeechPickedFn picked = At<SpeechPickedFn>(self, 0x4A4);
    if (picked != nullptr) {
        picked(event, category, line, out);
    }
    return 0;
}

// 0x59F800 (thiscall): a request into the first free slot, stamped (+0x4B0; none: -6) and
// appended; 0 when full
uint8_t __fastcall SpeechLib_AddImpl(uint8_t* self, re::Edx, const uint32_t* event, void* item, int32_t arg) {
    SpeechRequest* r = nullptr;
    for (int i = 0; i < 16; ++i) {
        if (RequestAt(self, i)->active == 0) {
            r = RequestAt(self, i);
            break;
        }
    }
    if (r == nullptr) {
        return 0;
    }
    for (int i = 0; i < 12; ++i) {
        r->event[i] = event[i];
    }
    r->item = item;
    r->active = 1;
    r->arg = arg;
    SpeechTimeFn now = At<SpeechTimeFn>(self, 0x4B0);
    r->start = now != nullptr ? now() : uint32_t(-6);
    SpeechRequest* tail = At<SpeechRequest*>(self, 0x490);
    if (At<SpeechRequest*>(self, 0x48C) == tail && tail == nullptr) {
        At<SpeechRequest*>(self, 0x48C) = r;
    } else {
        tail->next = r;
    }
    r->prev = At<SpeechRequest*>(self, 0x490);
    At<int32_t>(self, 0x494) += 1;
    At<SpeechRequest*>(self, 0x490) = r;
    return 1;
}

// 0x59F8A0 (thiscall): every request removed
void __fastcall SpeechLib_RemoveAll(uint8_t* self) {
    for (int i = 0; i < 16; ++i) {
        SpeechRequest* r = RequestAt(self, i);
        if (r->active != 0) {
            SL_Remove(self, r);
        }
    }
}

// 0x59F8D0 (thiscall): a model's requests of a type up to maxType (negative: any) removed
// (reason 1: those with flag 0x2000, 2: 0x8000, else all)
void __fastcall SpeechLib_RemoveModel(uint8_t* self, re::Edx, uint32_t model, int reason, int maxType) {
    for (int i = 0; i < 16; ++i) {
        SpeechRequest* r = RequestAt(self, i);
        if (r->active == 0 || ModelOf(r->event[0]) != model) {
            continue;
        }
        uint32_t type = r->event[0] >> 0x11 & 0xFF;
        if (type > uint32_t(maxType) && maxType >= 0) {
            continue;
        }
        uint8_t flags = reinterpret_cast<uint8_t*>(&r->event[2])[1];   // (+0xD)
        if (reason == 1) {
            if ((flags >> 5 & 1) == 0) {
                continue;
            }
        } else if (reason == 2) {
            if ((flags >> 7) == 0) {
                continue;
            }
        }
        SL_Remove(self, r);
    }
}

// 0x59F960 (thiscall): a model's requests of an id and variant (-1 any) removed
void __fastcall SpeechLib_RemoveLine(uint8_t* self, re::Edx, uint32_t model, uint32_t id, int variant) {
    for (int i = 0; i < 16; ++i) {
        SpeechRequest* r = RequestAt(self, i);
        if (r->active == 0 || ModelOf(r->event[0]) != model) {
            continue;
        }
        if ((r->event[0] & 0x1FF) != id) {
            continue;
        }
        if (int(r->event[1] & 0x7FF) == variant || variant == -1) {
            SL_Remove(self, r);
        }
    }
}

uint32_t RequestTime(uint8_t* lib) {
    SpeechTimeFn now = At<SpeechTimeFn>(lib, 0x4B0);
    return now != nullptr ? now() : uint32_t(-6);
}

void DoneAndRemove(uint8_t* lib, SpeechRequest* r, uint32_t reason) {
    SpeechDoneFn done = At<SpeechDoneFn>(lib, 0x4C4);
    if (done != nullptr) {
        done(r->event, r->item, reason);
    }
    SL_Remove(lib, r);
}

// 0x59F9D0 (thiscall): the frame: timed-out requests dropped, the best (0x5A0780's order) one
// started when it can be (the others it replaces dropped), or dropped by the reason it can't
void __fastcall SpeechLib_Update(uint8_t* self) {
    int length = 0;
    for (SpeechRequest* r = At<SpeechRequest*>(self, 0x48C); r != nullptr; r = r->next) {
        ++length;
    }
    if (length != speechQueueLengthStat) {
        speechQueueLengthStat += 1;
    }
    SpeechRequest* r = At<SpeechRequest*>(self, 0x48C);
    while (r != nullptr) {
        uint32_t now = RequestTime(self);
        if (now >= r->event[4] + r->start) {
            SpeechRequest* next = r->next;
            DoneAndRemove(self, r, r->start);   // (the start time as the reason: what's in ecx)
            r = next;
        } else {
            r = r->next;
        }
    }
    SpeechRequest* best = At<SpeechRequest*>(self, 0x48C);
    bool changed = false;
    if (best == nullptr) {
        return;
    }
    for (SpeechRequest* o = best->next; o != nullptr; o = o->next) {
        if (SL_CompareDifferences(int32_t(o->event[0] >> 0x11 & 0xFF), int32_t(o->event[3] & 3),
                                  int32_t(best->event[0] >> 0x11 & 0xFF), int32_t(best->event[3] & 3)) > 0) {
            best = o;
            changed = true;
        }
    }
    int32_t verdict = SL_CheckStart(theSpeech, best->event, best->arg);
    if (verdict == 0 && streamsBlocked == 0) {
        SpeechRequest* o = At<SpeechRequest*>(self, 0x48C);
        if (changed) {
            while (o != nullptr) {
                if (o == best) {
                    o = o->next;
                    continue;
                }
                SpeechRequest* next = o->next;
                uint8_t flagsC = reinterpret_cast<uint8_t*>(&o->event[2])[0];
                uint8_t flagsD = reinterpret_cast<uint8_t*>(&o->event[2])[1];
                bool drop = false;
                if ((flagsC & 0x40) != 0) {
                    drop = true;
                } else if ((flagsD & 1) != 0 && ((o->event[0] ^ best->event[0]) & 0x1FE00) == 0) {
                    drop = true;
                } else if ((flagsD & 2) != 0 && ((o->event[0] ^ best->event[0]) & 0x1FE00) == 0 &&
                           ((o->event[0] ^ best->event[0]) & 0x1FF) == 0) {
                    drop = true;
                } else if (streamsBlocked != 0) {
                    drop = true;
                }
                if (drop) {
                    DoneAndRemove(self, o, 0);
                }
                o = next;
            }
        }
        SpeechStartFn start = re::GameFnAt<SpeechStartFn>(uintptr_t(At<uint32_t>(self, 0)));
        int32_t result = start(self, best->event, best->item, reinterpret_cast<const char*>(0x008FEFBB),
                               best->arg);
        if (result == 0) {
            best->event[5] = 0;
            SL_Remove(self, best);
        }
        return;
    }
    uint8_t flagsC = reinterpret_cast<uint8_t*>(&best->event[2])[0];
    uint8_t flagsD = reinterpret_cast<uint8_t*>(&best->event[2])[1];
    switch (verdict) {
    case 1:
        if ((flagsD & 1) == 0) {
            return;
        }
        break;
    case 2:
        if ((flagsC & 0x40) == 0) {
            return;
        }
        break;
    case 3:
        if ((flagsD & 2) == 0) {
            return;
        }
        break;
    default:
        return;
    }
    DoneAndRemove(self, best, 0);
}

// ---- registration -----------------------------------------------------------------------------

#define SPEECHLIBQ_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(speechlibq_original_, __LINE__);                                 \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(speechlibq_original_, __LINE__), \
             re::Mode::Original)

SPEECHLIBQ_REVERSED("SpeechLib::FindLines", 0x0059EF60, &SpeechLib_FindLines);
SPEECHLIBQ_REVERSED("SpeechLib::ClearUsed", 0x0059F060, &SpeechLib_ClearUsed);
SPEECHLIBQ_REVERSED("SpeechLib::ForgetModel", 0x0059F110, &SpeechLib_ForgetModel);
SPEECHLIBQ_REVERSED("SpeechLib::IsQueued", 0x0059F1A0, &SpeechLib_IsQueued);
SPEECHLIBQ_REVERSED("SpeechLib::Load", 0x0059F210, &SpeechLib_Load);
SPEECHLIBQ_REVERSED("SpeechLib::Reset", 0x0059F290, &SpeechLib_Reset);
SPEECHLIBQ_REVERSED("SpeechLib::Remove", 0x0059F310, &SpeechLib_RemoveImpl);
SPEECHLIBQ_REVERSED("SpeechLib::CallStart", 0x0059F3B0, &SpeechLib_CallStart);
SPEECHLIBQ_REVERSED("SpeechLib::HasLine", 0x0059F3E0, &SpeechLib_HasLine);
SPEECHLIBQ_REVERSED("SpeechLib::BuildCategories", 0x0059F440, &SpeechLib_BuildCategories);
SPEECHLIBQ_REVERSED("SpeechLib::PickLine", 0x0059F620, &SpeechLib_PickLine);
SPEECHLIBQ_REVERSED("SpeechLib::Add", 0x0059F800, &SpeechLib_AddImpl);
SPEECHLIBQ_REVERSED("SpeechLib::RemoveAll", 0x0059F8A0, &SpeechLib_RemoveAll);
SPEECHLIBQ_REVERSED("SpeechLib::RemoveModel", 0x0059F8D0, &SpeechLib_RemoveModel);
SPEECHLIBQ_REVERSED("SpeechLib::RemoveLine", 0x0059F960, &SpeechLib_RemoveLine);
SPEECHLIBQ_REVERSED("SpeechLib::Update", 0x0059F9D0, &SpeechLib_Update);

} // namespace
