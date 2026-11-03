// The lip-sync store (CLipStore, the one at 0x208B9F0; 0x68DC40..0x68E3B1): lip files
// loaded per speech bank (0x1ED of them), two banks in use (the speech being played,
// swapped between), and two playing channels addressed by handles (the channel index in
// the low 16 bits, a serial in the high). A lip entry holds a speech id and a mouth
// openness track of 2 bit samples (0..3, four to a byte, high bits first) over its
// duration; a channel samples its track at the time since it started (plus a lead of 150
// ms and minus the lip offset at 0xBA4FF0) and eases its value towards it.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

// a lip file's entry (0x18 bytes)
struct LipEntry {
    uint16_t id;            // +0x00 the speech id
    uint16_t unknown02;     // +0x02
    float duration;         // +0x04 seconds
    uint16_t samples;       // +0x08 the sample count
    uint16_t dataOffset;    // +0x0A into the bank's samples
    uint32_t info[2];       // +0x0C, +0x10 (handed out by GetInfo)
    uint32_t unknown14;     // +0x14
};
static_assert(sizeof(LipEntry) == 0x18);

// a bank: a lip file's header (entry count, sample bytes), entries and samples
struct LipBank {
    uint16_t count;         // +0x00
    uint16_t dataSize;      // +0x02
    re::Ptr32<LipEntry> entries;      // +0x04
    re::Ptr32<int8_t> data;           // +0x08
};
static_assert(sizeof(LipBank) == 0xC);

// a playing channel (0x14 bytes)
struct LipChannel {
    re::Ptr32<LipEntry> entry;        // +0x00
    int32_t start;          // +0x04 the start time (ms)
    re::Ptr32<int8_t> data;           // +0x08 the entry's samples
    float value;            // +0x0C the sampled openness
    float eased;            // +0x10 eased towards it
};
static_assert(sizeof(LipChannel) == 0x14);

constexpr int kLipFiles = 0x1ED;

struct LipStore {
    LipBank banks[2];               // +0x0000 the banks in use
    LipBank files[kLipFiles];       // +0x0018 the loaded lip files
    uint8_t loaded[kLipFiles];      // +0x1734
    uint8_t pad1921[3];             // +0x1921
    LipChannel channels[2];         // +0x1924
    uint32_t serials[2];            // +0x194C 0: the channel's free
    uint32_t nextSerial;            // +0x1954
};
static_assert(offsetof(LipStore, files) == 0x18);
static_assert(offsetof(LipStore, loaded) == 0x1734);
static_assert(offsetof(LipStore, channels) == 0x1924);
static_assert(offsetof(LipStore, serials) == 0x194C);
static_assert(offsetof(LipStore, nextSerial) == 0x1954);

GAME_VAR(int32_t, lsTimeMs, 0x00C1A9B4);          // CTimer::m_snTimeInMilliseconds
GAME_VAR(int32_t, lsLipOffset, 0x00BA4FF0);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(int32_t, __cdecl, PopMemoryTag, 0x005EF100);
inline void* __cdecl Memcpy(void* dst, const void* src, uint32_t size) { return crt::Memcpy(dst, src, (size_t)size); }
GAME_FN(void, __cdecl, Streaming_52F640, 0x0052F640, int id);   // (Ghidra: CStreaming::RemoveModel)
GAME_FN(uint8_t, __cdecl, ApproachF, 0x0059CFC0, float* value, float target, float step);
GAME_FN(void, __thiscall, Mouth_SetLip, 0x0068E660, void* mouth, uint32_t handle);
// ours, through the game's addresses
GAME_FN(LipEntry*, __thiscall, LipStore_Find, 0x0068DD30, LipStore* self, int bank, int id, int first, int last);
GAME_FN(uint32_t, __thiscall, LipStore_NewHandle, 0x0068DDF0, LipStore* self);
GAME_FN(float, __thiscall, LipChannel_Sample, 0x0068DE90, LipChannel* self, int32_t time);

// a bank emptied; both when it shared its entries or samples with the other one
void ClearBank(LipStore* self, LipBank& bank) {
    bool shared = false;
    if (bank.entries) {
        if (self->banks[0].entries == self->banks[1].entries) {
            shared = true;
        }
        bank.entries = nullptr;
    }
    if (bank.data) {
        if (self->banks[0].data == self->banks[1].data) {
            shared = true;
        }
        bank.data = nullptr;
    }
    bank.count = 0;
    if (shared) {
        self->banks[0].count = 0;
        self->banks[1].count = 0;
        self->banks[0].entries = nullptr;
        self->banks[1].entries = nullptr;
        self->banks[0].data = nullptr;
        self->banks[1].data = nullptr;
    }
}

// 0x68DC40 (thiscall): an empty store
void __fastcall LipStore_Construct(LipStore* self, re::Edx) {
    for (int i = 0; i < 2; i++) {
        self->banks[i].count = 0;
        self->banks[i].entries = nullptr;
        self->banks[i].data = nullptr;
    }
    self->serials[0] = 0;
    self->serials[1] = 0;
    self->nextSerial = 1;
    for (int i = 0; i < kLipFiles; i++) {
        self->files[i].count = 0;
        self->files[i].entries = nullptr;
        self->files[i].data = nullptr;
        self->loaded[i] = 0;
    }
}

// 0x68DCA0 (thiscall): the banks swapped
void __fastcall LipStore_SwapBanks(LipStore* self, re::Edx) {
    LipBank bank = self->banks[0];
    self->banks[0] = self->banks[1];
    self->banks[1] = bank;
}

// 0x68DCD0 (thiscall): a bank emptied
void __fastcall LipStore_ClearBank(LipStore* self, re::Edx, int bank) {
    ClearBank(self, self->banks[bank]);
}

// 0x68DD30 (thiscall): a bank's entry for a speech id among [first, last] (all for a
// negative bound), null if none
LipEntry* __fastcall LipStore_FindImpl(LipStore* self, re::Edx, int bank, int id, int first, int last) {
    int16_t i = int16_t(first);
    int16_t end = int16_t(last);
    if (i < 0 || end < 0) {
        end = int16_t(uint16_t(self->banks[bank].count - 1));
        i = 0;
    }
    for (; i <= end; i++) {
        LipEntry* entry = &self->banks[bank].entries[i];
        if (id == entry->id) {
            return entry;
        }
    }
    return nullptr;
}

// 0x68DDA0 (thiscall): a bank set to the other one
void __fastcall LipStore_CopyOtherBank(LipStore* self, re::Edx, int bank) {
    self->banks[bank] = self->banks[bank == 0 ? 1 : 0];
}

// 0x68DDD0 (thiscall): a bank forgotten (not freed)
void __fastcall LipStore_ZeroBank(LipStore* self, re::Edx, int bank) {
    self->banks[bank] = LipBank{};
}

// 0x68DDF0 (thiscall): a free channel's handle (0: none)
uint32_t __fastcall LipStore_NewHandleImpl(LipStore* self, re::Edx) {
    int i = 0;
    for (; i < 2; i++) {
        if (self->serials[i] == 0) {
            break;
        }
    }
    if (i >= 2) {
        return 0;
    }
    self->serials[i] = self->nextSerial;
    uint32_t serial = self->nextSerial;
    uint32_t handle = (serial << 16) | uint32_t(i & 0xFFFF);
    self->nextSerial = serial + 1;
    if (int32_t(serial + 1) > 0xFFFF) {
        self->nextSerial = 1;
    }
    return handle;
}

// 0x68DE60 (thiscall): a handle's channel freed (if it's still its)
void __fastcall LipStore_FreeHandle(LipStore* self, re::Edx, uint32_t handle) {
    uint32_t& serial = self->serials[handle & 0xFFFF];
    if (serial == handle >> 16) {
        serial = 0;
    }
}

// the 2 bit sample at an index
inline int32_t Sample(const int8_t* data, int32_t index) {
    int32_t bits = data[index / 4];
    int32_t slot = index % 4;
    if (slot != 3) {
        bits >>= (3 - slot) * 2;
    }
    return bits & 3;
}

// 0x68DE90 (thiscall): the openness (0..1) at a time (ms) since the start, interpolated
// between samples
float __fastcall LipChannel_SampleImpl(LipChannel* self, re::Edx, int32_t time) {
    LipEntry* entry = self->entry;
    float t = (float(time) + 150.0f) / (entry->duration * 1000.0f);
    if (t < 0.0f) {
        t = 0.0f;
    } else if (1.0f < t) {
        t = 1.0f;
    }
    int32_t samples = entry->samples;
    int32_t index = int32_t(float(samples - 1) * t);
    int32_t next = index + 1;
    if (!(next < samples)) {
        next = index;
    }
    int8_t* data = self->data;
    float position = t * float(int32_t(entry->samples) - 1);
    float frac = position - float(index);
    float first = float(Sample(data, index));
    float second = float(Sample(data, next));
    return (second * frac + (1.0f - frac) * first) * 0.3333333432674408f;
}

// 0x68DFD0 (thiscall): a handle's channel's openness (0 for a stale handle)
float __fastcall LipStore_GetValue(LipStore* self, re::Edx, uint32_t handle) {
    if (handle == 0) {
        return 0.0f;
    }
    uint32_t index = handle & 0xFFFF;
    if (self->serials[index] != handle >> 16 || index == 0xFFFFFFFFu) {
        return 0.0f;
    }
    return self->channels[index].value;
}

// 0x68E010 (thiscall): the banks emptied and the loaded lip files freed (still marked
// loaded, as the game leaves them)
void __fastcall LipStore_Shutdown(LipStore* self, re::Edx) {
    for (int i = 0; i < 2; i++) {
        ClearBank(self, self->banks[i]);
    }
    for (int i = 0; i < kLipFiles; i++) {
        if (!self->loaded[i]) {
            continue;
        }
        if (self->files[i].entries) {
            Free(self->files[i].entries);
            self->files[i].entries = nullptr;
        }
        if (self->files[i].data) {
            Free(self->files[i].data);
            self->files[i].data = nullptr;
        }
    }
}

// 0x68E0B0 (thiscall): a lip file read from memory (header, entries, samples) and the
// streaming told; false if out of range or loaded
uint8_t __fastcall LipStore_LoadLip(LipStore* self, re::Edx, uint32_t index, const uint8_t* file) {
    if (index > kLipFiles - 1 || self->loaded[index]) {
        return 0;
    }
    LipBank& bank = self->files[index];
    At<uint32_t>(&bank, 0) = At<uint32_t>(const_cast<uint8_t*>(file), 0);
    PushMemoryTag(0x3E);
    bank.entries = static_cast<LipEntry*>(Malloc(uint32_t(bank.count) * 0x18));
    bank.data = static_cast<int8_t*>(Malloc(bank.dataSize));
    PopMemoryTag();
    const uint8_t* entries = file + 4;
    Memcpy(bank.entries, entries, uint32_t(bank.count) * 0x18);
    Memcpy(bank.data, entries + bank.count * 0x18, bank.dataSize);
    self->loaded[index] = 1;
    Streaming_52F640(int(index) + 0x5B54);
    return 1;
}

// 0x68E190 (thiscall): a bank set to a lip file (and the other bank too with both)
uint8_t __fastcall LipStore_SetBank(LipStore* self, re::Edx, int bank, uint32_t index, int both) {
    if (index > kLipFiles - 1) {
        return 0;
    }
    self->banks[bank] = self->files[index];
    if (uint8_t(both)) {
        int other = bank != 1 ? 1 : 0;
        self->banks[other] = self->banks[other == 0 ? 1 : 0];
    }
    return 1;
}

// 0x68E200 (thiscall): a speech's lip info (false if the bank lacks it)
uint8_t __fastcall LipStore_GetInfo(LipStore* self, re::Edx, int bank, int id, uint32_t* a, uint32_t* b, uint32_t* c) {
    LipEntry* entry = LipStore_Find(self, bank, id, 0, uint16_t(self->banks[bank].count - 1));
    if (!entry) {
        return 0;
    }
    *a = 0;
    *b = entry->info[0];
    *c = entry->info[1];
    return 1;
}

// 0x68E250 (thiscall): a speech's lips played on a free channel; its handle (0: none),
// handed to the ped's mouth
uint32_t __fastcall LipStore_Play(LipStore* self, re::Edx, int bank, int id, uint8_t* ped) {
    LipBank& from = self->banks[bank];
    LipEntry* entry = LipStore_Find(self, bank, id, 0, uint16_t(from.count - 1));
    if (!entry) {
        return 0;
    }
    uint32_t handle = LipStore_NewHandle(self);
    if (!handle) {
        return 0;
    }
    int32_t index = int32_t(handle & 0xFFFF);
    if (self->serials[index] != handle >> 16) {
        index = -1;
    }
    LipChannel& channel = self->channels[index];
    channel.entry = entry;
    channel.start = lsTimeMs;
    channel.data = from.data + entry->dataOffset;
    if (ped && (ped[0x108] & 7) == 3) {
        uint8_t* facial = At<uint8_t*>(ped, 0x1CC);
        if (facial) {
            Mouth_SetLip(facial + 0x4C, handle);
        }
    }
    return handle;
}

// 0x68E300 (thiscall): a channel sampled now and its eased value moved towards it (by 0.2,
// or 0.4 of the gap under 0.5, at least 0.01; snapped once reached)
void __fastcall LipChannel_Update(LipChannel* self, re::Edx) {
    float value = LipChannel_Sample(self, lsTimeMs - self->start - lsLipOffset);
    self->value = value;
    float gap = std::fabs(self->eased - value);
    float step = 0.2f;
    if (gap < 0.5f) {
        step = (gap + gap) * 0.20000000298023224f;
        if (0.009999999776482582f > step) {
            step = 0.009999999776482582f;
        }
    }
    if (ApproachF(&self->eased, value, step)) {
        self->value = self->eased;
    }
    self->eased = self->value;
}

#define LIPSTORE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

LIPSTORE_REVERSED("CLipStore::CLipStore_68DC40", 0x0068DC40, &LipStore_Construct);
LIPSTORE_REVERSED("CLipStore::SwapBanks_68DCA0", 0x0068DCA0, &LipStore_SwapBanks);
LIPSTORE_REVERSED("CLipStore::ClearBank_68DCD0", 0x0068DCD0, &LipStore_ClearBank);
LIPSTORE_REVERSED("CLipStore::FindEntry_68DD30", 0x0068DD30, &LipStore_FindImpl);
LIPSTORE_REVERSED("CLipStore::CopyOtherBank_68DDA0", 0x0068DDA0, &LipStore_CopyOtherBank);
LIPSTORE_REVERSED("CLipStore::ZeroBank_68DDD0", 0x0068DDD0, &LipStore_ZeroBank);
LIPSTORE_REVERSED("CLipStore::NewHandle_68DDF0", 0x0068DDF0, &LipStore_NewHandleImpl);
LIPSTORE_REVERSED("CLipStore::FreeHandle_68DE60", 0x0068DE60, &LipStore_FreeHandle);
LIPSTORE_REVERSED("CLipChannel::Sample_68DE90", 0x0068DE90, &LipChannel_SampleImpl);
LIPSTORE_REVERSED("CLipStore::GetValue_68DFD0", 0x0068DFD0, &LipStore_GetValue);
LIPSTORE_REVERSED("CLipStore::Shutdown_68E010", 0x0068E010, &LipStore_Shutdown);
LIPSTORE_REVERSED("CLipStore::LoadLip", 0x0068E0B0, &LipStore_LoadLip);
LIPSTORE_REVERSED("CLipStore::SetBank_68E190", 0x0068E190, &LipStore_SetBank);
LIPSTORE_REVERSED("CLipStore::GetInfo_68E200", 0x0068E200, &LipStore_GetInfo);
LIPSTORE_REVERSED("CLipStore::Play_68E250", 0x0068E250, &LipStore_Play);
LIPSTORE_REVERSED("CLipChannel::Update_68E300", 0x0068E300, &LipChannel_Update);

} // namespace
