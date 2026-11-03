// The action tree's paths and files: looking a node up by path ("/Global/...", "..",
// "."), resolving a hashed path, the act files (Act\Act.DIR / Act.IMG, .cat trees) loaded
// under the root or under a file reference (owned, or shared by a multi-parent
// reference), removing a subtree, the file reference's two names, and the tree's
// initialisation. 0x5F4CC0..0x5F7080.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/ActionTree.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace acttree;

namespace {

// a path's tokens (0x61A470): the rest of the path, the separators
struct PathTokenizer {
    re::Ptr32<const char> ptr;  // +0
    char seps[8];     // +4
    re::Ptr32<const char> start;  // +0xC
    int32_t unknown10;  // +0x10
    uint8_t pad[0x28];  // (the game's frame keeps 0x3C bytes for it)
};
static_assert(offsetof(PathTokenizer, start) == 0xC);

// a file reference's names (at +0x10 in the node): +8 the file, +0xC its folder; the
// flags' bits 1 / 2: owned copies (the game always clears them again)
struct FileNames {
    uint32_t unknown0;
    uint32_t unknown4;
    re::Ptr32<const char> file;    // +8
    re::Ptr32<const char> folder;  // +0xC
    uint32_t unknown10;
    uint32_t flags;      // +0x14
};
static_assert(sizeof(FileNames) == 0x18);

GAME_VAR(uint32_t, dotDotHash, 0x01B64A7C);   // ".."
GAME_VAR(uint32_t, dotHash, 0x01B64A80);      // "."
GAME_VAR(uint32_t, actionNodeHash, 0x01B649E8);
GAME_VAR(void*, actDirectory, 0x01B649D8);
GAME_VAR(void*, actImgStream, 0x01B649E0);
GAME_VAR(void*, actImgBuffer, 0x01B649E4);
inline void* const fileCache_D02850 = reinterpret_cast<void*>(0x00D02850);
inline void* const heapA_D14240 = reinterpret_cast<void*>(0x00D14240);
inline void* const heapB_D14170 = reinterpret_cast<void*>(0x00D14170);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int32_t, __thiscall, Heap_GetMemoryUsed, 0x005EEDD0, void* heap, int tag);
GAME_FN(uint8_t, __cdecl, Stub_8283D0, 0x008283D0, void*);   // (returns 0 in al)
GAME_FN(void, __thiscall, StringRef_Assign, 0x005772F0, re::Ptr32<const char>* field, const char* value);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, NameKey_FromString, 0x005FA890, uint32_t* key, const char* name);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, Tokenizer_Construct, 0x0061A470, PathTokenizer* tok, const char* path, const char* seps);
GAME_FN(uint8_t, __thiscall, Tokenizer_Is, 0x0061A310, PathTokenizer* tok, const char* token);
GAME_FN(void, __thiscall, Tokenizer_Next, 0x0061A4D0, PathTokenizer* tok);
GAME_FN(void, __thiscall, Tokenizer_Copy, 0x0061A3B0, PathTokenizer* tok, const char* from, char* out, int size, int, int);
GAME_FN(void, __thiscall, Tokenizer_SkipTo, 0x0061A560, PathTokenizer* tok, const char* token);
GAME_FN(int32_t, __thiscall, Node_GetRootKey, 0x005F4FA0, uint8_t* node);
GAME_FN(uint8_t*, __cdecl, ActionTree_Find, 0x005F5B10, const char* path, uint8_t* from);
GAME_FN(void, __cdecl, ActionTree_RemoveNode, 0x005F59D0, uint8_t* node, int);
GAME_FN(uint8_t*, __thiscall, MultiParent_Construct, 0x005F6810, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Bank_ConstructNamed, 0x005F5F70, uint8_t* node, const char* name);
GAME_FN(void, __thiscall, MultiParent_AddParent, 0x005F6960, uint8_t* node, uint8_t* parent, int32_t key);
GAME_FN(void, __thiscall, FileNames_SetFile, 0x005F4CC0, FileNames* names, const char* file, int copy);
GAME_FN(void, __thiscall, FileNames_SetFolder, 0x005F4D50, FileNames* names, const char* folder, int copy);
GAME_FN(uint8_t*, __thiscall, FileReference_LoadShared, 0x005F6AC0, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, FileReference_LoadOwned, 0x005F6EF0, uint8_t* node);
GAME_FN(void, __thiscall, Node_ResolveFileReferences, 0x005F6D10, uint8_t* node);
GAME_FN(uint8_t*, __cdecl, ActFile_Load, 0x005F6DB0, const char* path, uint8_t* parent, FileNames* names);
inline char* __cdecl Strncpy(char* to, const char* from, size_t size) { return crt::Strncpy(to, from, size); }
GAME_FN(void, __cdecl, NormalisePath, 0x00429D70, char* path);
GAME_FN(uint8_t*, __cdecl, CatFile_Load, 0x005FB550, const char* path);
GAME_FN(uint8_t*, __cdecl, CatData_Load, 0x005FB7A0, void* data, void* size);
GAME_FN(void, __thiscall, FileCache_Add, 0x005DBEB0, void* cache, const char* name, uint8_t* tree, int);
GAME_FN(void, __thiscall, Directory_Construct, 0x004291B0, void* dir, int capacity);
GAME_FN(void, __thiscall, Directory_Read, 0x004291F0, void* dir, const char* path);
GAME_FN(int, __cdecl, FileMgr_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(void, __cdecl, FileMgr_CloseFile, 0x0042D450, int handle);
GAME_FN(int, __cdecl, FileMgr_Read, 0x0042D2A0, int handle, void* to, int size);
GAME_FN(void*, __cdecl, OpenStream, 0x0042D4C0, int kind, int mode, void* arg);
GAME_FN(int, __cdecl, CloseStream, 0x0042D570, void* stream, void* out);
GAME_FN(void, __cdecl, PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopTxdMemoryTag, 0x0052E710);
GAME_FN(void, __cdecl, Tracks_Initialise_6502B0, 0x006502B0);
GAME_FN(void, __cdecl, Conditions_Initialise_64B1F0, 0x0064B1F0);
GAME_FN(void, __cdecl, ActionTree_InitHashes, 0x005FA8E0);
GAME_FN(void, __cdecl, ActionTree_LoadDirectory, 0x005F58C0);
GAME_FN(void, __cdecl, ActionTree_LoadGlobals, 0x005F6F90);

constexpr uint32_t kVtImplementation = 0x0092DA44;
constexpr uint32_t kVtBank = 0x0092DB34;

// a name set (copied when asked: the copy's flag bit then cleared again by the game)
void SetName(FileNames* names, re::Ptr32<const char>& field, uint32_t bit, const char* value, int copy) {
    if (names->flags & bit) {
        Free(re::PtrCast<char*>(field));
    }
    const char* p = value;
    if (re::Flag(copy)) {
        names->flags |= bit;
        size_t size = std::strlen(value) + 1;
        char* dup = static_cast<char*>(Malloc(size));
        std::memcpy(dup, value, size);
        p = dup;
    }
    uint32_t got = Stub_8283D0(&p);
    names->flags ^= ((bit == 1 ? got : (got & 0xFF) * 2) ^ names->flags) & bit;
    StringRef_Assign(&field, p);
}

// 0x5F4CC0 (thiscall on the names)
void __fastcall FileNames_SetFileImpl(FileNames* self, re::Edx, const char* file, int copy) {
    SetName(self, self->file, 1, file, copy);
}

// 0x5F4D50 (thiscall on the names)
void __fastcall FileNames_SetFolderImpl(FileNames* self, re::Edx, const char* folder, int copy) {
    SetName(self, self->folder, 2, folder, copy);
}

// 0x5F4E60 (cdecl): a node by a hashed path ({count, hashes}): "." from `from`, ".."
// up from it, else from the root (the first part skipped)
uint8_t* __cdecl ResolveHashedPath(const int32_t* path, uint8_t* from) {
    int count = path[0];
    const uint32_t* hashes = reinterpret_cast<const uint32_t*>(path[1]);
    if (count <= 0) {
        return nullptr;
    }
    int i = 0;
    uint8_t* node;
    if (dotHash == hashes[0]) {
        i = 1;
        node = from;
    } else if (dotDotHash == hashes[0]) {
        node = from;
    } else {
        node = root;
        i = 1;
    }
    for (; i < count; i++) {
        if (!node) {
            return nullptr;
        }
        if (dotDotHash == hashes[i]) {
            node = vc::Call<uint8_t*>(node, kNodeGetParent, -1);
            continue;
        }
        int n = vc::Call<int>(node, kNodeNumChildren);
        uint8_t* found = nullptr;
        for (int j = 0; j < n; j++) {
            uint8_t* child = vc::Call<uint8_t*>(node, kNodeGetChild, j);
            if (hashes[i] == *vc::Call<uint32_t*>(child, kNodeGetName)) {
                found = child;
                break;
            }
        }
        node = found;
    }
    return node;
}

// 0x5F56B0 (thiscall on a file reference): its file name (not copied)
void __fastcall FileReference_SetFile(uint8_t* self, re::Edx, const char* file) {
    uint32_t& flags = vc::At<uint32_t>(self, 0x24);
    if (flags & 1) {
        Free(vc::At<void*>(self, 0x18));
    }
    const char* p = file;
    uint32_t got = Stub_8283D0(&p);
    flags ^= (got ^ flags) & 1;
    StringRef_Assign(&vc::At<const char*>(self, 0x18), p);
}

// 0x5F5700 (thiscall on a file reference): its folder: the path from its "Act" part on
void __fastcall FileReference_SetFolderFromPath(uint8_t* self, re::Edx, const char* path) {
    char seps[3] = {'\\', '/', 0};
    PathTokenizer tok;
    Tokenizer_Construct(&tok, path, seps);
    Tokenizer_SkipTo(&tok, "Act");
    const char* p = tok.ptr;
    uint32_t& flags = vc::At<uint32_t>(self, 0x24);
    if (flags & 2) {
        Free(vc::At<void*>(self, 0x1C));
    }
    uint32_t got = Stub_8283D0(&p);
    flags ^= ((got & 0xFF) * 2 ^ flags) & 2;
    StringRef_Assign(&vc::At<const char*>(self, 0x1C), p);
}

// 0x5F58C0 (cdecl): the act files' directory (Act\Act.DIR, 800 entries)
void __cdecl LoadDirectory() {
    void* p = Malloc(0x10);
    uint8_t* dir = nullptr;
    if (p) {
        Directory_Construct(p, 800);
        vc::At<uint32_t>(p, 0) = 0x009186D0;   // (CDirectory's vtable)
        dir = static_cast<uint8_t*>(p);
    }
    actDirectory = dir;
    vc::At<uint32_t>(dir, 0xC) = 0;   // (through nullptr when out of memory, as the game)
    int handle = FileMgr_OpenFile("Act\\Act.DIR", "rb", 1);
    Directory_Read(actDirectory, "Act\\Act.DIR");
    FileMgr_CloseFile(handle);
}

// 0x5F5930 (thiscall): the names' destructor
void __fastcall FileNames_Destruct(FileNames* self, re::Edx) {
    const char* p = nullptr;
    if (self->flags & 1) {
        Free(re::PtrCast<char*>(self->file));
    }
    uint32_t got = Stub_8283D0(&p);
    self->flags ^= (got ^ self->flags) & 1;
    StringRef_Assign(&self->file, p);
    p = nullptr;
    if (self->flags & 2) {
        Free(re::PtrCast<char*>(self->folder));
    }
    got = Stub_8283D0(&p);
    self->flags ^= ((got & 0xFF) * 2 ^ self->flags) & 2;
    StringRef_Assign(&self->folder, p);
    NameKey_Destruct(&self->folder);
    NameKey_Destruct(&self->file);
}

void QueryMemory() {
    Heap_GetMemoryUsed(heapA_D14240, 0x25);
    Heap_GetMemoryUsed(heapB_D14170, 0x25);
}

// 0x5F59D0 (cdecl): a node deleted: out of the root, or its reference parent let go of it
void __cdecl RemoveNode(uint8_t* node, int) {
    if (!node) {
        return;
    }
    QueryMemory();
    if (!vc::Call<uint8_t>(root, kNodeRemoveChild, node) && vc::At<uint8_t*>(node, 4)) {
        uint8_t* parent = vc::At<uint8_t*>(node, 4);
        if (vc::Call<uint8_t>(parent, kNodeIsA, int(uint16_t(ReferenceId()))) && vc::At<uint8_t*>(node, 4)) {
            vc::Call<void>(vc::At<uint8_t*>(node, 4), kNodeSetTarget, nullptr, -1, 1);
        }
    }
    vc::Call<void>(node, kNodeDestruct, 1);
    QueryMemory();
}

// 0x5F5A60 (cdecl): the top-level tree holding `node` deleted, unless `keepA` or `keepB`
// is on the way up
void __cdecl RemoveTopLevel(uint8_t* node, uint8_t* keepA, uint8_t* keepB) {
    QueryMemory();
    uint8_t* top = node;
    uint8_t* at = node;
    if (at != root) {
        do {
            if (at == keepA || at == keepB) {
                return;
            }
            top = at;
            at = vc::Call<uint8_t*>(at, kNodeGetParent, -1);
        } while (at != root);
    }
    if (top && vc::Call<uint8_t>(root, kNodeRemoveChild, top)) {
        vc::Call<void>(top, kNodeDestruct, 1);
    }
    QueryMemory();
}

// 0x5F5B10 (cdecl): a node by path: relative names are looked for anywhere under the
// root; "/..." from the root, "." / ".." from `from` (keys: the current key with a
// `from`, else the root child's index)
uint8_t* __cdecl FindImpl(const char* path, uint8_t* from) {
    int32_t key = from ? currentKey : -1;
    char seps[3] = {'\\', '/', 0};
    PathTokenizer tok;
    Tokenizer_Construct(&tok, path, seps);
    uint8_t isDot = Tokenizer_Is(&tok, ".");
    uint8_t isDotDot = Tokenizer_Is(&tok, "..");
    uint8_t* node;
    if (isDot) {
        if (*tok.ptr) {
            Tokenizer_Next(&tok);
        }
        node = from;
    } else if (isDotDot) {
        node = from;
    } else if (path[0] != '\\' && path[0] != '/') {
        uint32_t* name = reinterpret_cast<uint32_t*>(&key);   // (the key's slot reused)
        NameKey_Construct(name, path);
        uint8_t* found = vc::Call<uint8_t*>(root, kNodeFindChild, name);
        NameKey_Destruct(name);
        return found;
    } else {
        node = root;
        if (!*tok.ptr) {
            return node;
        }
        Tokenizer_Next(&tok);
    }
    if (!*tok.ptr) {
        return node;
    }
    char part[0x100];
    do {
        if (!node) {
            return nullptr;
        }
        Tokenizer_Copy(&tok, tok.ptr, part, 0x100, 0, 1);
        if (std::memcmp(part, "..", 3) == 0) {
            node = vc::Call<uint8_t*>(node, kNodeGetParent, key);
        } else {
            int n = vc::Call<int>(node, kNodeNumChildren);
            uint8_t* found = nullptr;
            int j = 0;
            for (; j < n; j++) {
                uint8_t* child = vc::Call<uint8_t*>(node, kNodeGetChild, j);
                uint32_t* childName = vc::Call<uint32_t*>(child, kNodeGetName);
                if (*childName != 0 && HashName(part) == *childName) {
                    found = child;
                    break;
                }
            }
            if (node == root) {
                key = j;
            }
            node = found;
        }
        Tokenizer_Next(&tok);
    } while (*tok.ptr);
    return node;
}

// 0x5F6500 (cdecl): the node at the path deleted
void __cdecl RemovePath(const char* path) {
    uint8_t* node = ActionTree_Find(path, root);
    if (node) {
        ActionTree_RemoveNode(node, 1);
    }
}

// 0x5F6AC0 (thiscall on a file reference): its file shared: one multi-parent reference
// under /Global/<file> holds the loaded tree for every file reference to it
uint8_t* __fastcall FileReference_LoadSharedImpl(uint8_t* self, re::Edx) {
    if (!vc::Call<uint8_t*>(self, kNodeGetParent, -1)) {
        return nullptr;
    }
    PushMemoryTag(0x26);
    currentKey = Node_GetRootKey(self);
    char path[0x104];
    std::sprintf(path, "/Global/%s", static_cast<const char*>(vc::At<const char*>(self, 0x18)));
    uint8_t* tree = nullptr;
    path[0x103] = 0;
    uint8_t* holder = ActionTree_Find(path, nullptr);
    uint8_t* shared = nullptr;
    bool found = false;
    if (holder) {
        shared = vc::Call<uint8_t*>(holder, kNodeGetChild, 0);
        if (shared && vc::Call<uint8_t>(shared, kNodeIsA, int(uint16_t(MultiParentId())))) {
            found = true;
        }
    }
    if (!found) {
        void* p = Malloc(0x40);
        shared = p ? MultiParent_Construct(static_cast<uint8_t*>(p)) : nullptr;
        FileNames* names = reinterpret_cast<FileNames*>(shared + 0x10);
        FileNames_SetFile(names, vc::At<const char*>(self, 0x18), 1);
        FileNames_SetFolder(names, vc::At<const char*>(self, 0x1C), 1);
        if (!holder) {
            void* q = Malloc(0x20);
            holder = q ? Bank_ConstructNamed(static_cast<uint8_t*>(q), vc::At<const char*>(shared, 0x18)) : nullptr;
            vc::Call<void>(root, kNodeAddChild, holder);
        }
        int32_t holderName = int32_t(*vc::Call<uint32_t*>(holder, kNodeGetName));
        vc::Call<void>(holder, kNodeAddChild, shared);
        MultiParent_AddParent(shared, holder, holderName);
        std::sprintf(path, "%s%s", static_cast<const char*>(vc::At<const char*>(self, 0x1C)), static_cast<const char*>(vc::At<const char*>(self, 0x18)));
        tree = ActFile_Load(path, shared, names);
        FileCache_Add(fileCache_D02850, vc::At<const char*>(self, 0x18), tree, 1);
        if (tree) {
            int n = vc::Call<int>(tree, kNodeNumChildren);
            for (int i = 0; i < n; i++) {
                vc::Call<void>(vc::Call<uint8_t*>(tree, kNodeGetChild, i), kNodeSetParent, shared);
            }
            vc::Call<void>(shared, kNodeSetTarget, tree, -1, 1);
        }
    }
    int32_t key = currentKey;
    MultiParent_AddParent(shared, vc::Call<uint8_t*>(self, kNodeGetParent, -1), key);
    vc::Call<void>(self, kNodeSetTarget, vc::At<uint8_t*>(shared, 0xC), -1, 1);
    vc::Call<void>(shared, kNodeSetTarget, vc::At<uint8_t*>(shared, 0xC), -1, 1);
    PopMemoryTag();
    return tree;
}

// 0x5F6CE0 (thiscall on a file reference): its file loaded (owned or shared)
uint8_t* __fastcall FileReference_Load(uint8_t* self, re::Edx) {
    uint8_t* parent = vc::Call<uint8_t*>(self, kNodeGetParent, -1);
    if (!parent) {
        return parent;
    }
    if (self[0x28]) {
        return FileReference_LoadOwned(self);
    }
    return FileReference_LoadShared(self);
}

// 0x5F6D10 (thiscall): the file references in the subtree without a tree loaded
void __fastcall ResolveFileReferences(uint8_t* self, re::Edx) {
    if (self && vc::Call<uint8_t>(self, kNodeIsA, int(uint16_t(TypeIdAt<0x00CFB378, 0x00CFB374>()))) &&
        vc::At<uint8_t*>(self, 0xC) == nullptr && vc::Call<uint8_t*>(self, kNodeGetParent, -1)) {
        if (self[0x28]) {
            FileReference_LoadOwned(self);
        } else {
            FileReference_LoadShared(self);
        }
    }
    int count = vc::Call<int>(self, kNodeNumChildren);
    for (int i = 0; i < count; i++) {
        Node_ResolveFileReferences(vc::Call<uint8_t*>(self, kNodeGetChild, i));
    }
}

// 0x5F6DB0 (cdecl): a .cat file's tree loaded (without a parent: under the root, its
// key current), its file references loaded
uint8_t* __cdecl LoadActFile(const char* path, uint8_t* parent, FileNames*) {
    PushMemoryTag(0x25);
    if (*path == '/') {
        path++;
    }
    char name[0x100];
    Strncpy(name, path, 0x100);
    size_t length = std::strlen(path);
    // the 4-character extension replaced: "Act/Globals.act" -> "Act/Globals.cat" (the
    // game writes at [esp + len + 4] right after a push: name + len - 4)
    std::memcpy(name + length - 4, ".cat", 4);
    NormalisePath(name);
    uint8_t* tree = CatFile_Load(name);
    if (tree) {
        if (!parent) {
            vc::Call<void>(root, kNodeAddChild, tree);
            currentKey = Node_GetRootKey(tree);
        }
        Node_ResolveFileReferences(tree);
    }
    PopMemoryTag();
    return tree;
}

// 0x5F6E60 (cdecl): CCatStore::LoadData: a tree from memory, under `parent` or the root
uint8_t* __cdecl CatStore_LoadData(void* data, void* size, uint8_t* parent) {
    PushMemoryTag(0x20);
    QueryMemory();
    uint8_t* tree = CatData_Load(data, size);
    if (tree) {
        if (parent) {
            vc::Call<void>(tree, kNodeSetParent, parent);
        } else {
            vc::Call<void>(root, kNodeAddChild, tree);
        }
        Node_ResolveFileReferences(tree);
    }
    QueryMemory();
    PopMemoryTag();
    return tree;
}

// 0x5F6EF0 (thiscall on a file reference): its own copy of the file's tree
uint8_t* __fastcall FileReference_LoadOwnedImpl(uint8_t* self, re::Edx) {
    char path[0x104];
    std::sprintf(path, "%s%s", static_cast<const char*>(vc::At<const char*>(self, 0x1C)), static_cast<const char*>(vc::At<const char*>(self, 0x18)));
    uint8_t* tree = ActFile_Load(path, self, reinterpret_cast<FileNames*>(self + 0x10));
    if (tree) {
        FileCache_Add(fileCache_D02850, vc::At<const char*>(self, 0x18), tree, 1);
        int n = vc::Call<int>(tree, kNodeNumChildren);
        for (int i = 0; i < n; i++) {
            vc::Call<void>(vc::Call<uint8_t*>(tree, kNodeGetChild, i), kNodeSetParent, self);
        }
        vc::Call<void>(self, kNodeSetTarget, tree, -1, 1);
    }
    return tree;
}

// 0x5F6F90 (cdecl): the global act files: Act\Act.IMG read whole (0x311000 bytes) into a
// memory stream, each name of the null-ended table at 0xB0A4F0 loaded from it and cached
void __cdecl LoadGlobals() {
    int handle = FileMgr_OpenFile("Act\\Act.IMG", "r", 1);
    PushTxdMemoryTag();
    actImgBuffer = Malloc(0x311000);
    PopTxdMemoryTag();
    FileMgr_Read(handle, actImgBuffer, 0x311000);
    struct {
        re::Ptr32<void> data;
        uint32_t size;
    } memory{actImgBuffer, 0x311000};
    actImgStream = OpenStream(3, 1, &memory);
    FileMgr_CloseFile(handle);
    const re::Ptr32<const char>* names = reinterpret_cast<const re::Ptr32<const char>*>(0x00B0A4F0);
    for (int i = 0; names[i]; i++) {
        uint8_t* tree = ActFile_Load(names[i], nullptr, nullptr);
        if (tree) {
            FileCache_Add(fileCache_D02850, names[i], tree, 0);
        }
    }
    CloseStream(actImgStream, nullptr);
    Free(actImgBuffer);
    actImgStream = nullptr;
    actImgBuffer = nullptr;
}

// 0x5F7080 (cdecl): the tree's start: the name hashes, the track and condition
// registries, the root ("Global"), the directory, then the global act files (0x5F6F90)
void __cdecl Initialise() {
    PushMemoryTag(0x26);
    NameKey_FromString(&actionNodeHash, "ActionNode");
    Tracks_Initialise_6502B0();
    Conditions_Initialise_64B1F0();
    if (!root) {
        uint8_t* bank = static_cast<uint8_t*>(Malloc(0x20));
        if (bank) {
            vc::At<uint8_t*>(bank, 4) = nullptr;
            vc::At<uint32_t>(bank, 0) = kVtImplementation;
            NameKey_Construct(&vc::At<uint32_t>(bank, 8), "Global");
            vc::At<uint32_t>(bank, 0xC) = 0;
            vc::At<uint32_t>(bank, 0) = kVtBank;
            vc::At<uint32_t>(bank, 0x14) = 0;
            vc::At<uint32_t>(bank, 0x18) = 0;
            vc::At<uint32_t>(bank, 0x1C) = 0;
            root = bank;
        } else {
            root = nullptr;
        }
    }
    ActionTree_LoadDirectory();
    ActionTree_InitHashes();
    PopMemoryTag();
    ActionTree_LoadGlobals();
}

} // namespace

#define ACTFILES_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTFILES_REVERSED("ActionFileNames::SetFile", 0x005F4CC0, &FileNames_SetFileImpl);
ACTFILES_REVERSED("ActionFileNames::SetFolder", 0x005F4D50, &FileNames_SetFolderImpl);
ACTFILES_REVERSED("ActionTree::ResolveHashedPath", 0x005F4E60, &ResolveHashedPath);
ACTFILES_REVERSED("ActionNodeFileReference::SetFile", 0x005F56B0, &FileReference_SetFile);
ACTFILES_REVERSED("ActionNodeFileReference::SetFolderFromPath", 0x005F5700, &FileReference_SetFolderFromPath);
ACTFILES_REVERSED("ActionTree::LoadDirectory", 0x005F58C0, &LoadDirectory);
ACTFILES_REVERSED("ActionFileNames::Destruct", 0x005F5930, &FileNames_Destruct);
ACTFILES_REVERSED("ActionTree::RemoveNode", 0x005F59D0, &RemoveNode);
ACTFILES_REVERSED("ActionTree::RemoveTopLevel", 0x005F5A60, &RemoveTopLevel);
ACTFILES_REVERSED("ActionTree::Find", 0x005F5B10, &FindImpl);
ACTFILES_REVERSED("ActionTree::RemovePath", 0x005F6500, &RemovePath);
ACTFILES_REVERSED("ActionNodeFileReference::LoadShared", 0x005F6AC0, &FileReference_LoadSharedImpl);
ACTFILES_REVERSED("ActionNodeFileReference::Load", 0x005F6CE0, &FileReference_Load);
ACTFILES_REVERSED("ActionTree::ResolveFileReferences", 0x005F6D10, &ResolveFileReferences);
ACTFILES_REVERSED("ActionTree::LoadActFile", 0x005F6DB0, &LoadActFile);
ACTFILES_REVERSED("CCatStore::LoadData", 0x005F6E60, &CatStore_LoadData);
ACTFILES_REVERSED("ActionNodeFileReference::LoadOwned", 0x005F6EF0, &FileReference_LoadOwnedImpl);
ACTFILES_REVERSED("ActionTree::LoadGlobals", 0x005F6F90, &LoadGlobals);
ACTFILES_REVERSED("ActionTree::Initialise", 0x005F7080, &Initialise);
