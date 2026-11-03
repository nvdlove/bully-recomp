// The population's model sets (CObjectModelInfo, vtable 0x90BCD4, and its CPedModelsInfo /
// CVehicleModelsInfo): every model of a kind with a count of how many are out, and the
// choice of the next one (the least used, from a random start). CObjectNumbersInfo
// (vtable 0x90BCC4): a total and up to 12 weights, for a weighted random pick.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct ModelSetEntry {
    int32_t model;  // +0
    int32_t count;  // +4 how many are out
};
static_assert(sizeof(ModelSetEntry) == 8);

// the game's growing array of entries
struct ModelSetArray {
    re::Ptr32<ModelSetEntry> data;  // +0
    uint32_t capacity;    // +4
    uint32_t size;        // +8

    void Reserve(uint32_t n, int setSize);
};
static_assert(sizeof(ModelSetArray) == 0xC);

struct CObjectModelInfo {
    re::Ptr32<re::Ptr32<void>> vtable;         // +0 (vf0: a model of the set's kind)
    int32_t total;         // +4 the counts' sum
    ModelSetArray models;  // +8

    CObjectModelInfo* Construct();
    void Destruct();
    void Build(int kind);
    void Rebuild(int kind);
    int ChoosePedModel(int gender, int type, int ignoreUnique);
    int ChoosePedModelByGender(int male, int female, int ignoreUnique);
    int ChoosePedModelForGroup(int group);
    int ChooseVehicleModel(int except, int except2);
    void AddModel(int model);
    void AddRef(int model);
    void RemoveRef(int model);
    uint8_t IsVehicleOfClass(int model, int vehicleClass);
    uint8_t IsPedOfType(int model, int pedType);
};
static_assert(offsetof(CObjectModelInfo, models) == 8);
static_assert(sizeof(CObjectModelInfo) == 0x14);

struct CObjectNumbersInfo {
    re::Ptr32<re::Ptr32<void>> vtable;       // +0 (vf1: how many weights it takes)
    int32_t total;       // +4 the weights' sum
    uint8_t weights[12]; // +8
    uint32_t count;      // +0x14

    CObjectNumbersInfo* Construct(int unused);
    void Destruct();
    void Parse(const char* line);
    void Set(const int* values);
    void CopyFrom(const CObjectNumbersInfo* other);
    int PickRandom();
    CObjectNumbersInfo* Destroy(uint8_t flags);
};
static_assert(offsetof(CObjectNumbersInfo, count) == 0x14);
static_assert(sizeof(CObjectNumbersInfo) == 0x18);

inline re::Ptr32<void>* const s_modelSetVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090BCD4);
inline re::Ptr32<void>* const s_numbersVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090BCC4);
inline const char* const s_formatInt = reinterpret_cast<const char*>(0x008FFF78);       // "%d"
inline const char* const s_formatSpaceInt = reinterpret_cast<const char*>(0x0090BCCC);  // " %d"
GAME_ARRAY(uint8_t*, s_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, s_timer, 0x00C1A9B4);            // CTimer::m_snTimeInMilliseconds
GAME_VAR(int32_t, s_longCooldownModel, 0x00BF3EC4); // (its cooldown is 100 s, the others' 50)
GAME_VAR(int32_t, s_excludedVehicle, 0x00BF4454);
GAME_VAR(void*, s_population, 0x00C2C108);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, size_t size, int count, void* constructor);
inline void* const s_entryConstructor = reinterpret_cast<void*>(0x005F4CB0);
inline int __cdecl Rand() { return crt::Rand(); }
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(int16_t, __thiscall, PedModelInfo_GetUniqueStatus, 0x005228D0, void* info);  // -1: not now, 0: any number, n: at most n
GAME_FN(int32_t*, __thiscall, PedModelInfo_GetType, 0x00522380, void* info, int32_t* out);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(int, __thiscall, Population_CountPedsWithName, 0x0049E960, void* population, uint32_t hash, int max);
GAME_FN(uint8_t, __cdecl, IsVehicleModel, 0x0051B420, int model);
GAME_FN(uint8_t, __cdecl, IsPedModel, 0x0051B3F0, int model);
// (its own, called where they are)
GAME_FN(void, __thiscall, ModelSetArray_Reserve, 0x00464530, ModelSetArray* array, uint32_t n, int setSize);
GAME_FN(void, __thiscall, ModelSet_Build, 0x00464800, CObjectModelInfo* set, int kind);
GAME_FN(int, __thiscall, ModelSet_ChoosePedModel, 0x004648C0, CObjectModelInfo* set, int gender, int type, int ignoreUnique);

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

// (the check the choices end with) a unique ped model already out as often as allowed
bool TooManyOut(uint8_t* mi) {
    int unique = PedModelInfo_GetUniqueStatus(mi);
    if (unique < 0) {
        return true;
    }
    if (unique >= 1) {
        uint32_t hash = HashName(reinterpret_cast<const char*>(mi + 0x98));
        if (Population_CountPedsWithName(s_population, hash, unique) >= unique) {
            return true;
        }
    }
    return false;
}

// 0x464490: a line split in place into words (quoted ones whole) into `words` (a null
// after the last); `count` counts them
void __cdecl SplitWords(char* line, int* count, re::Ptr32<char>* words) {
    if (!line) {
        return;
    }
    char c = *line;
    while (c != '\0') {
        while (*line == ' ' || *line == '\t') {
            line++;
        }
        c = *line;
        char* end = line;
        if (c == '"') {
            line++;
            end = line;
            c = *end;
            while (c != '\0' && c != '"') {
                c = *++end;
            }
        } else {
            while (c != '\0' && c != ' ' && c != '\t') {
                c = *++end;
            }
        }
        words[*count] = line;
        (*count)++;
        if (*end == '\0') {
            break;
        }
        *end = '\0';
        line = end + 1;
        c = *line;
    }
    words[*count] = nullptr;
}

// 0x464530 (thiscall): room for n (the old entries copied); setSize: the size becomes n
void ModelSetArray::Reserve(uint32_t n, int setSize) {
    capacity = n;
    ModelSetEntry* entries = nullptr;
    if (n) {
        uint64_t bytes = uint64_t(n) * 8;
        entries = static_cast<ModelSetEntry*>(Malloc(bytes > 0xFFFFFFFFu ? size_t(-1) : size_t(bytes)));
        if (entries) {
            ConstructArray(entries, 8, int(n), s_entryConstructor);
        } else {
            entries = nullptr;
        }
    }
    if (data) {
        for (uint32_t i = 0; i < size; i++) {
            entries[i] = data[i];
        }
        Free(data);
    }
    data = entries;
    if (re::Flag(setSize)) {
        size = capacity;
    }
}

// ---- CObjectNumbersInfo -------------------------------------------------------------------

// 0x4645C0 (thiscall; its argument unused)
CObjectNumbersInfo* CObjectNumbersInfo::Construct(int) {
    vtable = s_numbersVtable;
    total = 0;
    count = 0;
    return this;
}

// 0x4645E0 (thiscall)
void CObjectNumbersInfo::Destruct() {
    vtable = s_numbersVtable;
}

// 0x4645F0 (thiscall): "total w1 w2 ..." (as many weights as vf1 says)
void CObjectNumbersInfo::Parse(const char* line) {
    Sscanf(line, s_formatInt, &total);
    int n = reinterpret_cast<int(__thiscall*)(CObjectNumbersInfo*)>(static_cast<void*>(vtable[1]))(this);
    const char* p = line;
    for (int i = 0; i < n; i++) {
        // the next word: past this one and the spaces after it
        while (*p != '\0' && *p != ' ') {
            p++;
        }
        while (*p == ' ') {
            p++;
        }
        int value;
        Sscanf(p, s_formatSpaceInt, &value);
        weights[count] = uint8_t(value);
        count++;
    }
}

// 0x464680 (thiscall): from ints (the total, then the weights)
void CObjectNumbersInfo::Set(const int* values) {
    total = values[0];
    uint32_t n = uint32_t(reinterpret_cast<int(__thiscall*)(CObjectNumbersInfo*)>(static_cast<void*>(vtable[1]))(this));
    for (uint32_t i = 0; i < n; i++) {
        if (i < count) {
            weights[i] = uint8_t(values[1 + i]);
        } else {
            weights[count] = uint8_t(values[1 + i]);
            count++;
        }
    }
}

// 0x4646D0 (thiscall): another's (missing weights 0; gaps filled with 0)
void CObjectNumbersInfo::CopyFrom(const CObjectNumbersInfo* other) {
    total = other->total;
    int n = reinterpret_cast<int(__thiscall*)(CObjectNumbersInfo*)>(static_cast<void*>(vtable[1]))(this);
    for (uint32_t i = 0; int(i) < n; i++) {
        uint8_t w = i < other->count ? other->weights[i] : 0;
        if (i < count) {
            weights[i] = w;
            continue;
        }
        while (i > count) {
            weights[count] = 0;
            count++;
        }
        weights[count] = w;
        count++;
    }
}

// 0x464740 (thiscall): an index by weight (-1: none, or the total is 0)
int CObjectNumbersInfo::PickRandom() {
    if (total == 0) {
        return -1;
    }
    int threshold = Rand() * (total - 1) / 0x7FFF;
    int sum = 0;
    for (uint32_t i = 0; i < count; i++) {
        sum += weights[i];
        if (sum > threshold) {
            return int(i);
        }
    }
    return -1;
}

// 0x4647E0 vf0 (thiscall): the destructor (flags 1: and freed)
CObjectNumbersInfo* CObjectNumbersInfo::Destroy(uint8_t flags) {
    vtable = s_numbersVtable;
    if (flags & 1) {
        Free(this);
    }
    return this;
}

// ---- CObjectModelInfo ---------------------------------------------------------------------

// 0x464E00 (thiscall)
CObjectModelInfo* CObjectModelInfo::Construct() {
    vtable = s_modelSetVtable;
    total = 0;
    models.data = nullptr;
    models.capacity = 0;
    models.size = 0;
    return this;
}

// 0x4647A0 (thiscall): the destructor's body (the array freed; the game checks twice)
void CObjectModelInfo::Destruct() {
    vtable = s_modelSetVtable;
    if (models.data) {
        Free(models.data);
        models.data = nullptr;
    }
    models.capacity = 0;
    models.size = 0;
    if (models.data) {
        Free(models.data);
        models.data = nullptr;
    }
}

// 0x464800 (thiscall): every model (of 12200) vf0 says is of the kind
void CObjectModelInfo::Build(int kind) {
    using IsKind = uint8_t(__thiscall*)(CObjectModelInfo*, int, int);
    int found = 0;
    for (int model = 0; model < 0x2FA8; model++) {
        if (re::PtrCast<IsKind>(vtable[0])(this, model, kind)) {
            found++;
        }
    }
    if (!found) {
        return;
    }
    if (models.capacity < models.size + found) {
        ModelSetArray_Reserve(&models, models.size + found, 0);
    }
    int added = 0;
    for (int model = 0; model < 0x2FA8; model++) {
        if (!re::PtrCast<IsKind>(vtable[0])(this, model, kind)) {
            continue;
        }
        if (models.size == models.capacity) {
            ModelSetArray_Reserve(&models, models.capacity ? models.capacity * 2 : 1, 0);
        }
        models.data[models.size].model = model;
        models.data[models.size].count = 0;
        models.size++;
        if (++added >= found) {
            return;
        }
    }
}

// 0x4648C0 (thiscall): a ped model of the gender (1 or 2 excludes; its +0x50 says
// female) and type (5: any), the least used from a random start, not used in the last
// 50 s (100 for one model). (ignoreUnique: the unique limits aren't looked at)
int CObjectModelInfo::ChoosePedModel(int gender, int type, int ignoreUnique) {
    if (models.size == 0) {
        return -1;
    }
    int best = -1;
    int bestCount = 100;
    uint32_t start = uint32_t(Rand()) % models.size;
    uint32_t i = start;
    do {
        ModelSetEntry* entry = &models.data[i];
        uint8_t* mi = s_modelInfoPtrs[entry->model];
        bool cooling = false;
        if (mi) {
            int lastUsed = Field<int32_t>(mi, 0x60);
            if (lastUsed > 0 && s_timer >= lastUsed) {
                int cooldown = entry->model != s_longCooldownModel ? 50000 : 100000;
                cooling = s_timer - lastUsed < cooldown;
            }
        }
        // (a missing model info is read through when unique limits apply)
        if (!cooling && (re::Flag(ignoreUnique) || PedModelInfo_GetUniqueStatus(mi) >= 0)) {
            bool excluded = Field<uint8_t>(mi, 0x50) ? gender == 1 : gender == 2;
            int32_t pedType;
            if (!excluded && (type == 5 || *PedModelInfo_GetType(mi, &pedType) == type) && entry->count < bestCount) {
                if (re::Flag(ignoreUnique)) {
                    best = int(i);
                    bestCount = entry->count;
                } else if (PedModelInfo_GetUniqueStatus(mi) == 0 || entry->count < PedModelInfo_GetUniqueStatus(mi)) {
                    best = int(i);
                    bestCount = entry->count;
                }
            }
        }
        if (++i >= models.size) {
            i = 0;
        }
    } while (i != start);
    if (best == -1) {
        return -1;
    }
    uint8_t* mi = s_modelInfoPtrs[models.data[best].model];
    if (!re::Flag(ignoreUnique) && mi && TooManyOut(mi)) {
        return -1;
    }
    return models.data[best].model;
}

// 0x464A50 (thiscall): the model, if not there yet
void CObjectModelInfo::AddModel(int model) {
    for (uint32_t i = 0; i < models.size; i++) {
        if (models.data[i].model == model) {
            return;
        }
    }
    if (models.size == models.capacity) {
        ModelSetArray_Reserve(&models, models.capacity ? models.capacity * 2 : 1, 0);
    }
    models.data[models.size].model = model;
    models.data[models.size].count = 0;
    models.size++;
}

// 0x464AB0 (thiscall): one more of the model out
void CObjectModelInfo::AddRef(int model) {
    for (uint32_t i = 0; i < models.size; i++) {
        if (models.data[i].model == model) {
            ModelSetEntry* entry = &models.data[i];
            if (entry) {
                total++;
                entry->count++;
            }
            return;
        }
    }
}

// 0x464AF0 (thiscall): one fewer
void CObjectModelInfo::RemoveRef(int model) {
    for (uint32_t i = 0; i < models.size; i++) {
        if (models.data[i].model == model) {
            ModelSetEntry* entry = &models.data[i];
            if (entry) {
                total--;
                entry->count--;
            }
            return;
        }
    }
}

// 0x464B30 CVehicleModelsInfo vf0 (thiscall): a vehicle model (not the excluded one) of
// the class (its model info's +0x54)
uint8_t CObjectModelInfo::IsVehicleOfClass(int model, int vehicleClass) {
    if (IsVehicleModel(model) && model != s_excludedVehicle && Field<int32_t>(s_modelInfoPtrs[model], 0x54) == vehicleClass) {
        return 1;
    }
    return 0;
}

// 0x464B70 (thiscall): a vehicle model within its limit (+0x62), the least used from a
// random start; `except` only when none is out, never `except2`
int CObjectModelInfo::ChooseVehicleModel(int except, int except2) {
    if (models.size == 0) {
        return -1;
    }
    int best = -1;
    int bestCount = 100;
    uint32_t start = uint32_t(Rand()) % models.size;
    uint32_t i = start;
    do {
        int model = models.data[i].model;
        int16_t limit = Field<int16_t>(s_modelInfoPtrs[model], 0x62);
        int count = models.data[i].count;
        if (limit > 0 && count <= limit && count < bestCount && (model != except || count < 1) && model != except2) {
            bestCount = models.data[i].count;
            best = int(i);
        }
        if (++i >= models.size) {
            i = 0;
        }
    } while (i != start);
    if (best == -1) {
        return -1;
    }
    return models.data[best].model;
}

// 0x464C20 CPedModelsInfo vf0 (thiscall): a ped model of the type (its model info's +0x4C)
uint8_t CObjectModelInfo::IsPedOfType(int model, int pedType) {
    if (IsPedModel(model) && Field<int32_t>(s_modelInfoPtrs[model], 0x4C) == pedType) {
        return 1;
    }
    return 0;
}

// 0x464C50 (thiscall): a ped model whose stats (+0x54) list the group (+0xC0, +0xC4,
// +0xC8), among the models up to the last one with any group
int CObjectModelInfo::ChoosePedModelForGroup(int group) {
    if (models.size == 0) {
        return -1;
    }
    int limit = int(models.size) - 1;
    for (int i = int(models.size) - 1; i >= 0; i--) {
        uint8_t* mi = s_modelInfoPtrs[models.data[i].model];
        if (PedModelInfo_GetUniqueStatus(mi) == -1) {
            continue;
        }
        uint8_t* stats = Field<uint8_t*>(mi, 0x54);
        if (Field<int32_t>(stats, 0xC0) || Field<int32_t>(stats, 0xC4) || Field<int32_t>(stats, 0xC8)) {
            limit = i;
            break;
        }
    }
    if (limit == 0) {
        limit = 1;
    }
    int best = -1;
    int best2 = -1;
    int bestCount = 100;
    int start = Rand() % limit;
    int i = start;
    do {
        ModelSetEntry* entry = &models.data[i];
        uint8_t* mi = s_modelInfoPtrs[entry->model];
        if (entry->count < bestCount &&
            (PedModelInfo_GetUniqueStatus(mi) == 0 || entry->count < PedModelInfo_GetUniqueStatus(mi))) {
            uint8_t* stats = Field<uint8_t*>(mi, 0x54);
            if (Field<int32_t>(stats, 0xC0) == group || Field<int32_t>(stats, 0xC4) == group || Field<int32_t>(stats, 0xC8) == group) {
                bestCount = models.data[i].count;
                best2 = i;
                best = i;
            }
        }
        if (++i >= limit) {
            i = 0;
        }
    } while (i != start);
    if (best == -1) {
        if (best2 == -1) {
            return -1;
        }
        best = best2;
    }
    uint8_t* mi = s_modelInfoPtrs[models.data[best].model];
    if (mi && TooManyOut(mi)) {
        return -1;
    }
    return models.data[best].model;
}

// 0x464E30 (thiscall): emptied, then built again
void CObjectModelInfo::Rebuild(int kind) {
    total = 0;
    if (models.data) {
        Free(models.data);
        models.data = nullptr;
    }
    models.capacity = 0;
    models.size = 0;
    ModelSet_Build(this, kind);
}

// 0x464E70 (thiscall): ChoosePedModel of any type: not male: gender 1; not female: 2
int CObjectModelInfo::ChoosePedModelByGender(int male, int female, int ignoreUnique) {
    int gender = 0;
    if (!re::Flag(male)) {
        gender = 1;
    } else if (!re::Flag(female)) {
        gender = 2;
    }
    return ModelSet_ChoosePedModel(this, gender, 5, ignoreUnique);
}

// An object with its own table of 0x28-byte elements (+0x9C of them at +0xA0) that
// continues into another's 0xC-byte ones (+0x10: +0x5C of them at +0x60); the elements
// have an id at +4. (Which class this is isn't known yet)
struct ElementTable {
    void* GetElement(uint32_t index);
    void* FindById(int id, int* outIndex);
};

// 0x464EA0 (thiscall): element i, counting on into the other's; nullptr past both
void* ElementTable::GetElement(uint32_t index) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this);
    uint32_t own = Field<uint32_t>(self, 0x9C);
    if (index < own) {
        return Field<uint8_t*>(self, 0xA0) + index * 0x28;
    }
    uint8_t* next = Field<uint8_t*>(self, 0x10);
    if (next) {
        index -= own;
        if (index < Field<uint32_t>(next, 0x5C)) {
            uint8_t* element = Field<uint8_t*>(next, 0x60) + index * 0xC;
            // (the game tests the element and its +4 for null)
            if (element && element + 4) {
                return element;
            }
        }
    }
    return nullptr;
}

// 0x465580 (thiscall): the element with the id, and its index (counting on into the
// other's)
void* ElementTable::FindById(int id, int* outIndex) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this);
    uint32_t own = Field<uint32_t>(self, 0x9C);
    for (uint32_t i = 0; i < own; i++) {
        if (Field<int32_t>(Field<uint8_t*>(self, 0xA0) + i * 0x28, 4) == id) {
            if (outIndex) {
                *outIndex = int(i);
            }
            return Field<uint8_t*>(self, 0xA0) + i * 0x28;
        }
    }
    uint8_t* next = Field<uint8_t*>(self, 0x10);
    if (next) {
        uint32_t n = Field<uint32_t>(next, 0x5C);
        uint8_t* elements = Field<uint8_t*>(next, 0x60);
        for (uint32_t i = 0; i < n; i++) {
            if (Field<int32_t>(elements + i * 0xC, 4) == id) {
                uint8_t* element = elements + i * 0xC;
                if (!element || !(element + 4)) {
                    return nullptr;
                }
                if (outIndex) {
                    *outIndex = int(own + i);
                }
                return element;
            }
        }
    }
    return nullptr;
}

} // namespace

#define MSET_C05_REVERSED(name, address, fn) REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)

REVERSED("SplitWords", 0x00464490, RE_BODY(&SplitWords), nullptr, nullptr, re::Mode::Original);
MSET_C05_REVERSED("ModelSetArray::Reserve", 0x00464530, &ModelSetArray::Reserve);
MSET_C05_REVERSED("CObjectNumbersInfo::CObjectNumbersInfo", 0x004645C0, &CObjectNumbersInfo::Construct);
MSET_C05_REVERSED("CObjectNumbersInfo::~CObjectNumbersInfo", 0x004645E0, &CObjectNumbersInfo::Destruct);
MSET_C05_REVERSED("CObjectNumbersInfo::Parse", 0x004645F0, &CObjectNumbersInfo::Parse);
MSET_C05_REVERSED("CObjectNumbersInfo::Set", 0x00464680, &CObjectNumbersInfo::Set);
MSET_C05_REVERSED("CObjectNumbersInfo::CopyFrom", 0x004646D0, &CObjectNumbersInfo::CopyFrom);
MSET_C05_REVERSED("CObjectNumbersInfo::PickRandom", 0x00464740, &CObjectNumbersInfo::PickRandom);
MSET_C05_REVERSED("CObjectModelInfo::~CObjectModelInfo", 0x004647A0, &CObjectModelInfo::Destruct);
MSET_C05_REVERSED("CObjectNumbersInfo::Destroy", 0x004647E0, &CObjectNumbersInfo::Destroy);
MSET_C05_REVERSED("CObjectModelInfo::Build", 0x00464800, &CObjectModelInfo::Build);
MSET_C05_REVERSED("CPedModelsInfo::ChooseModel", 0x004648C0, &CObjectModelInfo::ChoosePedModel);
MSET_C05_REVERSED("CObjectModelInfo::AddModel", 0x00464A50, &CObjectModelInfo::AddModel);
MSET_C05_REVERSED("CObjectModelInfo::AddRef", 0x00464AB0, &CObjectModelInfo::AddRef);
MSET_C05_REVERSED("CObjectModelInfo::RemoveRef", 0x00464AF0, &CObjectModelInfo::RemoveRef);
MSET_C05_REVERSED("CVehicleModelsInfo::IsOfKind", 0x00464B30, &CObjectModelInfo::IsVehicleOfClass);
MSET_C05_REVERSED("CVehicleModelsInfo::ChooseModel", 0x00464B70, &CObjectModelInfo::ChooseVehicleModel);
MSET_C05_REVERSED("CPedModelsInfo::IsOfKind", 0x00464C20, &CObjectModelInfo::IsPedOfType);
MSET_C05_REVERSED("CPedModelsInfo::ChooseModelForGroup", 0x00464C50, &CObjectModelInfo::ChoosePedModelForGroup);
MSET_C05_REVERSED("CObjectModelInfo::CObjectModelInfo", 0x00464E00, &CObjectModelInfo::Construct);
MSET_C05_REVERSED("CObjectModelInfo::Rebuild", 0x00464E30, &CObjectModelInfo::Rebuild);
MSET_C05_REVERSED("CPedModelsInfo::ChooseModelByGender", 0x00464E70, &CObjectModelInfo::ChoosePedModelByGender);
MSET_C05_REVERSED("ElementTable::GetElement", 0x00464EA0, &ElementTable::GetElement);
MSET_C05_REVERSED("ElementTable::FindById", 0x00465580, &ElementTable::FindById);
