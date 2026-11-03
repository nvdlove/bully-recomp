// 0x40C970 to 0x40D920: the light scene loader, a TiXmlDocument (".gsa" XML: GSA >
// OBJECTS > COMPONENTs with LinkIDs, ENTITYs referring to them by RefLinkID) that
// indexes its components in a hash_map<int, TiXmlElement*> and builds lights from the
// NiGeneralEntity elements that have a light (or shadow generator) component; and the
// hash_map's insert, operator[] and clear.
//
// The loader: the TiXmlDocument (+0x34 its error string), +0x48 the file name (a
// std::string), +0x64 loaded, +0x68/+0x6C/+0x70 the light entities (items, capacity,
// count), +0x74.. the shadow entities, +0x80 the hash_map (+0x84 its list: +0x88 the head,
// +0x8C the size; +0x90 the bucket vector: +0x94 first, +0x98 last; +0xA0 the mask, +0xA4
// the bucket count).

#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_FN(void*, __cdecl, CMemoryMgr_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, CMemoryMgr_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
inline void* __cdecl memset_(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
inline long __cdecl atol_(const char* s) { return crt::Atol(s); }
template <typename... A>
inline int __cdecl sscanf_(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline void __cdecl InvalidParameter() { crt::InvalidParameter(); }
GAME_VAR(char*, ms_nullString, 0x00951028);

GAME_FN(void*, __thiscall, TiXmlNode_FirstChildElement, 0x008574C0, const void* node, const char* name);
GAME_FN(void*, __thiscall, TiXmlNode_NextSiblingElement, 0x00857530, const void* node, const char* name);
GAME_FN(const char*, __thiscall, TiXmlElement_Attribute, 0x00857A90, const void* element, const char* name);
GAME_FN(const char*, __thiscall, TiXmlElement_GetText, 0x00857590, const void* element);
GAME_FN(void, __thiscall, TiXmlDocument_Construct, 0x00857E50, void* self);
GAME_FN(uint8_t, __thiscall, TiXmlDocument_LoadFile, 0x00857FC0, void* self, const char* path, int encoding);
GAME_FN(void, __thiscall, TiXmlNode_Destruct, 0x008579C0, void* self);

GAME_FN(void, __thiscall, List_Destruct, 0x0040BAD0, void* list);
GAME_FN(void*, __thiscall, List_BuyHead, 0x0040BEB0, void* list);
GAME_FN(uint32_t*, __thiscall, List_BuyNode, 0x0040BF30, void* list, void* next, void* previous, const void* value);
GAME_FN(void, __thiscall, List_IncSize, 0x0040C430, void* list, uint32_t count);
GAME_FN(uint32_t*, __thiscall, HashMap_Find, 0x0040BD60, void* self, uint32_t* result, const int32_t* key);
GAME_FN(uint32_t*, __thiscall, HashMap_Insert, 0x0040C970, void* self, uint32_t* result, const int32_t* value);
GAME_FN(int32_t*, __thiscall, HashMap_Index, 0x0040CE50, void* self, const int32_t* key);
GAME_FN(void, __thiscall, HashMap_Clear, 0x0040CEE0, void* self);
GAME_FN(void, __thiscall, Vector_Resize, 0x0040C7B0, void* self, uint32_t n, uint32_t value0, uint32_t value1);
GAME_FN(void, __thiscall, Vector_Construct, 0x0040C860, void* self, uint32_t n, const uint32_t* value);
GAME_FN(void, __thiscall, Vector_Assign, 0x0040C900, void* self, uint32_t n, const uint32_t* value);
GAME_FN(void, __thiscall, PointerVector_PushBack, 0x0040BA00, void* self, const void* item);
GAME_FN(void*, __thiscall, String_AssignChars, 0x0040C1B0, void* self, const char* s, uint32_t count);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignCString, 0x00406730, re::Ptr32<char>* self, const char* s);
GAME_FN(void, __thiscall, Loader_Reset, 0x0040CF50, void* self);
GAME_FN(uint8_t, __thiscall, Loader_Load, 0x0040D6A0, void* self, const char* path);
GAME_FN(void, __stdcall, ParseTransform, 0x0040B860, const void* element, void* transform);
GAME_FN(uint32_t, __stdcall, GetLightType, 0x0040B380, const void* element);
GAME_FN(void, __stdcall, ParseLight, 0x0040B700, const void* element, const void* templateElement, void* light);

// the hash of an int key: Park-Miller of key ^ 0xDEADBEEF
uint32_t Hash(int32_t key) {
    int32_t x = int32_t(uint32_t(key) ^ 0xDEADBEEFu);
    int32_t h = (x % 127773) * 16807 - (x / 127773) * 2836;
    if (h < 0) {
        h += 0x7FFFFFFF;
    }
    return uint32_t(h);
}

// the hash_map's parts (relative to itself)
struct HashMap {
    uint8_t* self;
    uint8_t* List() const {
        return self + 4;
    }
    uint32_t* Head() const {
        return *reinterpret_cast<re::Ptr32<uint32_t>*>(self + 8);
    }
    uint32_t& Mask() const {
        return *reinterpret_cast<uint32_t*>(self + 0x20);
    }
    uint32_t& MaxIndex() const {
        return *reinterpret_cast<uint32_t*>(self + 0x24);
    }
    uint32_t* Buckets() const {
        return *reinterpret_cast<re::Ptr32<uint32_t>*>(self + 0x14);
    }
    uint32_t BucketCount() const {
        uint8_t* first = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x14);
        return first ? uint32_t((*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x18) - first) >> 3) : 0;
    }
    void Check(uint32_t i) const {
        if (!Buckets() || i >= BucketCount()) {
            InvalidParameter();
        }
    }
    uint32_t* BucketNode(uint32_t i) const {
        Check(i);
        return reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(Buckets()[i * 2 + 1]));
    }
    void SetBucket(uint32_t i, void* cont, uint32_t* node) const {
        Check(i);
        Buckets()[i * 2] = re::Addr32(cont);
        Buckets()[i * 2 + 1] = re::Addr32(node);
    }
};

uint32_t* Next(uint32_t* node) {
    return reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(node[0]));
}
uint32_t* Prev(uint32_t* node) {
    return reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(node[1]));
}
uint32_t Ptr(void* p) {
    return re::Addr32(p);
}

int Compare(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return static_cast<uint8_t>(*a) - static_cast<uint8_t>(*b);
}

} // namespace

namespace LightXmlLoader {

// 0x40C970 (thiscall): hash_map::insert(value) (8 bytes: key, value): first the table
// grown by a bucket when the list has four items a bucket (the split bucket's items that
// hash to the new one moved to the list's end), then the value inserted in its bucket
// in key order unless the key is there; a pair (iterator, inserted) out
static uint32_t* __fastcall HashMap_Insert_(uint8_t* self, re::Edx, uint32_t* result, const int32_t* value) {
    HashMap map{self};
    uint32_t size = *reinterpret_cast<uint32_t*>(self + 0xC);
    if (map.MaxIndex() <= size >> 2) {
        uint32_t count = map.BucketCount();
        if (count - 1 <= map.MaxIndex()) {
            map.Mask() = count * 2 - 3;
            Vector_Resize(self + 0x10, map.Mask() + 2, Ptr(self + 4), Ptr(map.Head()));
        } else if (map.Mask() < map.MaxIndex()) {
            map.Mask() = map.Mask() * 2 + 1;
        }
        uint32_t bucket = map.MaxIndex() - (map.Mask() >> 1) - 1;
        uint32_t* node = map.BucketNode(bucket);
        uint8_t* list = self + 4;
        uint8_t* cont = list;
        while (true) {
            if (map.BucketNode(bucket + 1) == node) {
                break;
            }
            if (!cont) {
                InvalidParameter();
            }
            if (node == *reinterpret_cast<re::Ptr32<uint32_t>*>(cont + 4)) {
                InvalidParameter();
            }
            if ((Hash(int32_t(node[2])) & map.Mask()) == bucket) {
                if (node == *reinterpret_cast<re::Ptr32<uint32_t>*>(cont + 4)) {
                    InvalidParameter();
                }
                node = Next(node);
                continue;
            }
            // the item moves to the new bucket
            uint8_t* nodeCont = cont;
            uint32_t* head = map.Head();
            if (node == *reinterpret_cast<re::Ptr32<uint32_t>*>(nodeCont + 4)) {
                InvalidParameter();
            }
            if (nodeCont != list) {
                InvalidParameter();
            }
            uint32_t* next = Next(node);
            if (next != head) {
                for (uint32_t b = bucket;; b--) {
                    if (map.BucketNode(b) != node) {
                        break;
                    }
                    map.SetBucket(b, nodeCont, next);
                    if (b == 0) {
                        break;
                    }
                }
                // splice [node, next) to the end
                uint32_t* end = map.Head();
                uint32_t* previous = Prev(node);
                previous[0] = Ptr(next);
                Prev(next)[0] = Ptr(end);
                Prev(end)[0] = Ptr(node);
                uint32_t* last = Prev(end);
                end[1] = Ptr(Prev(next));
                next[1] = Ptr(Prev(node));
                node[1] = Ptr(last);
                end = map.Head();
                node = Prev(end);
                if (node == end) {
                    InvalidParameter();
                }
                map.SetBucket(map.MaxIndex() + 1, list, map.Head());
                cont = list;
            }
            for (uint32_t b = map.MaxIndex(); bucket < b; b--) {
                if (map.BucketNode(b) != map.Head()) {
                    break;
                }
                map.SetBucket(b, cont, node);
            }
            if (nodeCont != list) {
                InvalidParameter();
            }
            if (next == map.Head()) {
                break;
            }
            cont = nodeCont;
            node = next;
        }
        map.MaxIndex()++;
    }
    // the insert
    uint32_t bucket = Hash(value[0]) & map.Mask();
    if (map.MaxIndex() <= bucket) {
        bucket += 0xFFFFFFFFu - (map.Mask() >> 1);
    }
    uint32_t* at = map.BucketNode(bucket + 1);
    uint8_t* list = self + 4;
    while (true) {
        if (map.BucketNode(bucket) == at) {
            break;   // insert before `at`
        }
        at = Prev(at);
        if (at == map.Head()) {
            InvalidParameter();
            if (at == map.Head()) {
                InvalidParameter();
            }
        }
        if (value[0] < int32_t(at[2])) {
            continue;
        }
        if (at == map.Head()) {
            InvalidParameter();
        }
        if (int32_t(at[2]) >= value[0]) {
            result[0] = Ptr(list);
            result[1] = Ptr(at);
            reinterpret_cast<uint8_t*>(result)[8] = 0;
            return result;
        }
        if (at == map.Head()) {
            InvalidParameter();
        }
        at = Next(at);
        break;
    }
    uint32_t* node = List_BuyNode(list, at, Prev(at), value);
    List_IncSize(list, 1);
    at[1] = Ptr(node);
    Prev(node)[0] = Ptr(node);
    uint32_t* inserted = Prev(at);
    if (inserted == map.Head()) {
        InvalidParameter();
    }
    for (;; bucket--) {
        if (map.BucketNode(bucket) != at) {
            break;
        }
        map.SetBucket(bucket, list, inserted);
        if (bucket == 0) {
            break;
        }
    }
    result[0] = Ptr(list);
    result[1] = Ptr(inserted);
    reinterpret_cast<uint8_t*>(result)[8] = 1;
    return result;
}

// 0x40CE50 (thiscall): hash_map::operator[] (inserting (key, 0) when missing); returns
// the value's address
static int32_t* __fastcall HashMap_Index_(uint8_t* self, re::Edx, const int32_t* key) {
    uint32_t found[2];
    HashMap_Find(self, found, key);
    uint32_t end = Ptr(*reinterpret_cast<re::Ptr32<void>*>(self + 8));
    if (!found[0] || found[0] != Ptr(self + 4)) {
        InvalidParameter();
    }
    uint32_t cont = found[0];
    uint32_t node = found[1];
    if (node == end) {
        int32_t pair[2] = {*key, 0};
        uint32_t result[3];
        uint32_t* r = HashMap_Insert(self, result, pair);
        cont = r[0];
        node = r[1];
    }
    if (!cont) {
        InvalidParameter();
    }
    if (node == *reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(cont) + 4)) {
        InvalidParameter();
    }
    return reinterpret_cast<int32_t*>(static_cast<uintptr_t>(node) + 0xC);
}

// 0x40CEE0 (thiscall): hash_map::clear: the items freed, 9 empty buckets, mask 1
static void __fastcall HashMap_Clear_(uint8_t* self, re::Edx) {
    uint32_t* head = *reinterpret_cast<re::Ptr32<uint32_t>*>(self + 8);
    uint32_t* node = Next(head);
    head[0] = Ptr(head);
    head = *reinterpret_cast<re::Ptr32<uint32_t>*>(self + 8);
    head[1] = Ptr(head);
    *reinterpret_cast<uint32_t*>(self + 0xC) = 0;
    while (node != *reinterpret_cast<re::Ptr32<uint32_t>*>(self + 8)) {
        uint32_t* next = Next(node);
        CMemoryMgr_Free(node);
        node = next;
    }
    uint32_t empty[2] = {Ptr(self + 4), Ptr(*reinterpret_cast<re::Ptr32<void>*>(self + 8))};
    Vector_Assign(self + 0x10, 9, empty);
    *reinterpret_cast<uint32_t*>(self + 0x20) = 1;
    *reinterpret_cast<uint32_t*>(self + 0x24) = 1;
}

// 0x40CF50 (thiscall): the loader emptied
static void __fastcall Reset(uint8_t* self, re::Edx) {
    HashMap_Clear(self + 0x80);
    re::Ptr32<void>& lights = *reinterpret_cast<re::Ptr32<void>*>(self + 0x68);
    if (lights) {
        CMemoryMgr_Free(lights);
        lights = nullptr;
    }
    *reinterpret_cast<uint32_t*>(self + 0x6C) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x70) = 0;
    re::Ptr32<void>& shadows = *reinterpret_cast<re::Ptr32<void>*>(self + 0x74);
    if (shadows) {
        CMemoryMgr_Free(shadows);
        shadows = nullptr;
    }
    *reinterpret_cast<uint32_t*>(self + 0x78) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x7C) = 0;
    String_AssignChars(self + 0x48, reinterpret_cast<const char*>(0x008FEFBB), 0);
    self[0x64] = 0;
}

// 0x40CFB0 (thiscall): light `index` (0x88 bytes: +0 its type, colours at +4, +0x10,
// +0x1C, its transform's rotation at +0x28, translation +0x4C, scale +0x58, attenuation
// +0x5C.., dimmer +0x68, spot angles and exponent +0x6C.., the flags +0x78.., its name
// +0x84), from the entity's components; point lights are type 2, spots 0, and a
// light with a shadow generator is type 1 and takes the master's properties too.
// Ambient and directional lights give null (their light is leaked)
static uint8_t* __fastcall BuildLight(uint8_t* self, re::Edx, int32_t index) {
    if (index < 0 || uint32_t(index) >= *reinterpret_cast<uint32_t*>(self + 0x70)) {
        return nullptr;
    }
    const void* entity = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 0x68))[index];
    const void* component = TiXmlNode_FirstChildElement(entity, "COMPONENT");
    const void* shadow = nullptr;
    PushMemoryTag(6);
    uint8_t* light = static_cast<uint8_t*>(CMemoryMgr_Malloc(0x88));
    if (light) {
        for (int i = 1; i <= 9; i++) {
            reinterpret_cast<float*>(light)[i] = 0.0f;
        }
        *reinterpret_cast<re::Ptr32<char>*>(light + 0x84) = ms_nullString;
    }
    // (and then all of it zeroed: the name handle becomes 0, not the null string)
    memset_(light, 0, 0x88);
    PopMemoryTag();
    float transform[13];   // (uninitialised without a transformation component: the game's)
    memset(transform, 0, sizeof(transform));
    uint32_t type = 4;
    float* f = reinterpret_cast<float*>(light);
    for (; component; component = TiXmlNode_NextSiblingElement(component, "COMPONENT")) {
        const char* ref = TiXmlElement_Attribute(component, "RefLinkID");
        if (!ref) {
            continue;
        }
        int32_t id = atol_(ref);
        const void* element = reinterpret_cast<const void*>(static_cast<uintptr_t>(*HashMap_Index(self + 0x80, &id)));
        if (!element || !TiXmlElement_Attribute(element, "Class")) {
            continue;
        }
        const char* cls = TiXmlElement_Attribute(element, "Class");
        if (!cls) {
            continue;
        }
        if (Compare(cls, "NiTransformationComponent") == 0) {
            ParseTransform(element, transform);
            continue;
        }
        if (Compare(cls, "NiShadowGeneratorComponent") == 0) {
            shadow = element;
            continue;
        }
        if (Compare(cls, "NiLightComponent") != 0) {
            continue;
        }
        type = GetLightType(element);
        switch (type) {
        case 0:
        case 3:
            return nullptr;
        case 1:
            *reinterpret_cast<uint32_t*>(light) = 2;
            break;
        case 2:
            *reinterpret_cast<uint32_t*>(light) = 0;
            break;
        default:
            break;
        }
        FixedString_AssignCString(reinterpret_cast<re::Ptr32<char>*>(light + 0x84), TiXmlElement_Attribute(entity, "Name"));
        for (const void* p = TiXmlNode_FirstChildElement(element, "PROPERTY"); p;
             p = TiXmlNode_NextSiblingElement(p, "PROPERTY")) {
            const char* name = TiXmlElement_Attribute(p, "Name");
            if (!name || Compare(name, "Affected Entities") == 0) {
                continue;
            }
            const char* text = TiXmlElement_GetText(p);
            if (!text) {
                continue;
            }
            float v[3] = {0.0f, 0.0f, 0.0f};
            if (Compare(name, "Dimmer") == 0) {
                sscanf_(text, "%f", &v[0]);   // (uninitialised when it fails: the game's)
                f[0x68 / 4] = v[0];
            } else if (Compare(name, "Color (Ambient)") == 0) {
                sscanf_(text, "%f, %f, %f", &v[0], &v[1], &v[2]);
                f[1] = v[0];
                f[2] = v[1];
                f[3] = v[2];
            } else if (Compare(name, "Color (Diffuse)") == 0) {
                sscanf_(text, "%f, %f, %f", &v[0], &v[1], &v[2]);
                f[4] = v[0];
                f[5] = v[1];
                f[6] = v[2];
            } else if (Compare(name, "Color (Specular)") == 0) {
                sscanf_(text, "%f, %f, %f", &v[0], &v[1], &v[2]);
                f[7] = v[0];
                f[8] = v[1];
                f[9] = v[2];
            } else if (Compare(name, "Attenuation (Constant)") == 0) {
                if (type == 1 || type == 2) {
                    sscanf_(text, "%f", &v[0]);
                    f[0x5C / 4] = v[0];
                }
            } else if (Compare(name, "Attenuation (Linear)") == 0) {
                if (type == 1 || type == 2) {
                    sscanf_(text, "%f", &v[0]);
                    f[0x60 / 4] = v[0];
                }
            } else if (Compare(name, "Attenuation (Quadratic)") == 0) {
                if (type == 1 || type == 2) {
                    sscanf_(text, "%f", &v[0]);
                    f[0x64 / 4] = v[0];
                }
            } else if (Compare(name, "Spot Angle (Outer)") == 0) {
                if (type == 2) {
                    sscanf_(text, "%f", &v[0]);
                    f[0x6C / 4] = v[0];
                }
            } else if (Compare(name, "Spot Angle (Inner)") == 0) {
                if (type == 2) {
                    sscanf_(text, "%f", &v[0]);
                    f[0x70 / 4] = v[0];
                }
            } else if (Compare(name, "Spot Exponent") == 0) {
                if (type == 2) {
                    sscanf_(text, "%f", &v[0]);
                    f[0x74 / 4] = v[0];
                }
            }
        }
    }
    for (int i = 0; i < 9; i++) {
        f[0x28 / 4 + i] = transform[i];
    }
    f[0x58 / 4] = transform[12];
    f[0x4C / 4] = transform[9];
    f[0x50 / 4] = transform[10];
    f[0x54 / 4] = transform[11];
    if (shadow && *reinterpret_cast<uint32_t*>(light) == 0) {
        *reinterpret_cast<uint32_t*>(light) = 1;
        int32_t master = atol_(TiXmlElement_Attribute(shadow, "MasterLinkID"));
        const void* element = reinterpret_cast<const void*>(static_cast<uintptr_t>(*HashMap_Index(self + 0x80, &master)));
        ParseLight(shadow, element, light);
    }
    return light;
}

// 0x40D5F0 (thiscall): the loader's destructor
static void __fastcall Destruct(uint8_t* self, re::Edx) {
    Loader_Reset(self);
    re::Ptr32<void>& buckets = *reinterpret_cast<re::Ptr32<void>*>(self + 0x94);
    if (buckets) {
        CMemoryMgr_Free(buckets);
    }
    buckets = nullptr;
    *reinterpret_cast<uint32_t*>(self + 0x98) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x9C) = 0;
    List_Destruct(self + 0x84);
    static const int kVectors[2] = {0x74, 0x68};
    for (int offset : kVectors) {
        re::Ptr32<void>& p = *reinterpret_cast<re::Ptr32<void>*>(self + offset);
        if (p) {
            CMemoryMgr_Free(p);
            p = nullptr;
        }
    }
    if (*reinterpret_cast<uint32_t*>(self + 0x60) >= 0x10) {
        CMemoryMgr_Free(*reinterpret_cast<re::Ptr32<void>*>(self + 0x4C));
    }
    *reinterpret_cast<uint32_t*>(self + 0x60) = 0xF;
    *reinterpret_cast<uint32_t*>(self + 0x5C) = 0;
    self[0x4C] = 0;
    *reinterpret_cast<uint32_t*>(self) = 0x008FFF84;   // TiXmlDocument's
    void* error = *reinterpret_cast<re::Ptr32<void>*>(self + 0x34);
    if (error != reinterpret_cast<void*>(0x020E6004)) {
        CMemoryMgr_Free(error);
    }
    TiXmlNode_Destruct(self);
}

// 0x40D6A0 (thiscall): a .gsa file loaded: the components indexed by LinkID, then the
// NiGeneralEntity entities with a light component (and a MasterLinkID) listed as lights,
// those with a shadow generator component as shadows
static uint8_t __fastcall Load(uint8_t* self, re::Edx, const char* path) {
    Loader_Reset(self);
    if (!path || !*path || !TiXmlDocument_LoadFile(self, path, 0)) {
        return 0;
    }
    String_AssignChars(self + 0x48, path, uint32_t(strlen(path)));
    const void* gsa = TiXmlNode_FirstChildElement(self, "GSA");
    if (!gsa) {
        return 0;
    }
    const void* objects = TiXmlNode_FirstChildElement(gsa, "OBJECTS");
    if (!objects) {
        return 0;
    }
    for (const void* c = TiXmlNode_FirstChildElement(objects, "COMPONENT"); c;
         c = TiXmlNode_NextSiblingElement(c, "COMPONENT")) {
        const char* id = TiXmlElement_Attribute(c, "LinkID");
        if (id) {
            int32_t key = atol_(id);
            *HashMap_Index(self + 0x80, &key) = static_cast<int32_t>(reinterpret_cast<uintptr_t>(c));
        }
    }
    for (const void* entity = TiXmlNode_FirstChildElement(objects, "ENTITY"); entity;
         entity = TiXmlNode_NextSiblingElement(entity, "ENTITY")) {
        const char* cls = TiXmlElement_Attribute(entity, "Class");
        if (!cls || Compare(cls, "NiGeneralEntity") != 0) {
            continue;
        }
        bool isLight = false;
        bool isShadow = false;
        for (const void* c = TiXmlNode_FirstChildElement(entity, "COMPONENT"); c;
             c = TiXmlNode_NextSiblingElement(c, "COMPONENT")) {
            const char* ref = TiXmlElement_Attribute(c, "RefLinkID");
            if (!ref) {
                continue;
            }
            int32_t key = atol_(ref);
            const void* element = reinterpret_cast<const void*>(static_cast<uintptr_t>(*HashMap_Index(self + 0x80, &key)));
            if (!element) {
                continue;
            }
            const char* componentClass = TiXmlElement_Attribute(element, "Class");
            if (!componentClass) {
                continue;
            }
            if (Compare(componentClass, "NiLightComponent") == 0) {
                if (TiXmlElement_Attribute(entity, "MasterLinkID")) {
                    isLight = true;
                }
            } else if (Compare(componentClass, "NiShadowGeneratorComponent") == 0) {
                if (TiXmlElement_Attribute(entity, "MasterLinkID")) {
                    isShadow = true;
                }
            }
        }
        if (isLight) {
            PointerVector_PushBack(self + 0x68, &entity);
        }
        if (isShadow) {
            PointerVector_PushBack(self + 0x74, &entity);
        }
    }
    self[0x64] = 1;
    return 1;
}

// 0x40D920 (thiscall): the loader's constructor (loading `path` when given). (The hash's
// comparator byte takes the path pointer's low byte: the game's)
static uint8_t* __fastcall Construct(uint8_t* self, re::Edx, const char* path) {
    TiXmlDocument_Construct(self);
    *reinterpret_cast<uint32_t*>(self + 0x60) = 0xF;
    *reinterpret_cast<uint32_t*>(self + 0x5C) = 0;
    self[0x4C] = 0;
    self[0x64] = 0;
    for (int offset = 0x68; offset <= 0x7C; offset += 4) {
        *reinterpret_cast<uint32_t*>(self + offset) = 0;
    }
    self[0x80] = uint8_t(reinterpret_cast<uintptr_t>(path));
    void* head = List_BuyHead(self + 0x84);
    *reinterpret_cast<re::Ptr32<void>*>(self + 0x88) = head;
    *reinterpret_cast<uint32_t*>(self + 0x8C) = 0;
    uint32_t empty[2] = {Ptr(self + 0x84), Ptr(head)};
    Vector_Construct(self + 0x90, 9, empty);
    *reinterpret_cast<uint32_t*>(self + 0xA0) = 1;
    *reinterpret_cast<uint32_t*>(self + 0xA4) = 1;
    if (path) {
        Loader_Load(self, path);
    }
    return self;
}

} // namespace LightXmlLoader

// ---- registration ----------------------------------------------------------------------

#define C00_GSA_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace LightXmlLoader {
C00_GSA_REVERSED("stdext::hash_map::insert", HashMap_Insert_, 0x0040C970);
C00_GSA_REVERSED("stdext::hash_map::operator[]", HashMap_Index_, 0x0040CE50);
C00_GSA_REVERSED("stdext::hash_map::clear", HashMap_Clear_, 0x0040CEE0);
C00_GSA_REVERSED("LightXmlLoader::Reset", Reset, 0x0040CF50);
C00_GSA_REVERSED("LightXmlLoader::BuildLight", BuildLight, 0x0040CFB0);
C00_GSA_REVERSED("LightXmlLoader::Destruct", Destruct, 0x0040D5F0);
C00_GSA_REVERSED("LightXmlLoader::Load", Load, 0x0040D6A0);
C00_GSA_REVERSED("LightXmlLoader::Construct", Construct, 0x0040D920);
} // namespace LightXmlLoader
