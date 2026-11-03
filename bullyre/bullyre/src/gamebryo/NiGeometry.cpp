// NiGeometry (src/gamebryo/NiGeometry.h): the materials (applied, the active one, the shader
// they give), the property/effect states, the geometry data and skin instance, streaming and
// cloning, ApplyTransform (into the vertices and normals).
//
// Its own virtual slots: 34 RenderImmediate, 35 SetModelData, 36 CalculateNormals (the data's
// slot 20), 37 CalculateConsistency.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometry.h"
#include "gamebryo/NiSceneCommon.h"
#include "game/HeapCheck.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiGeometry_RTTI, 0x020E25B4);
constexpr uintptr_t NiGeometry_vtable = 0x00951CC8;

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
GAME_VAR(uint8_t, NiGeometry_ms_bStreamingAppliesDefaultMaterial, 0x00BC75C4);
GAME_VAR(void*, NiMaterial_ms_pkMaterials, 0x020E2704);   // NiTFixedStringMap<NiMaterial*>

using Instance = NiMaterialInstance;

// the materials' array (data, allocated, count)
struct MaterialArray {
    re::Ptr32<Instance> data;
    uint32_t allocated;
    uint32_t count;
};

inline MaterialArray* Materials(NiGeometry* geometry) {
    return reinterpret_cast<MaterialArray*>(&geometry->materials);
}

GAME_FN(void, __thiscall, Bound_Update, 0x007D2FD0, NiBound* self, const NiBound* bound, const NiTransform* xform);
GAME_FN(void, __thiscall, Bound_ComputeFromData, 0x007D2890, NiBound* self, uint32_t count, const NiPoint3* points);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Multiply, 0x00768920, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_TransposeTimes, 0x00768F60, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(uint8_t, __thiscall, Matrix3_Inverse, 0x00768B30, const NiMatrix3* self, NiMatrix3* result);
GAME_FN(NiPoint3*, __thiscall, Matrix3_MultiplyPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(NiPoint3*, __cdecl, Point3_TimesMatrix, 0x00768AD0, NiPoint3* result, const NiPoint3* point,
        const NiMatrix3* matrix);
GAME_FN(void, __thiscall, VisibleArray_SetAllocatedSize, 0x00776B50, void* array, uint32_t size);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count, void* constructor);
GAME_FN(void*, __thiscall, Instances_VectorDelete, 0x00814250, Instance* self, uint32_t flags);
GAME_FN(uint32_t, __thiscall, Instance_GetExtraData, 0x00856850, const Instance* self);
GAME_FN(NiObject*, __thiscall, Instance_GetCachedShader, 0x007D3280, Instance* self, NiGeometry* geometry,
        NiObject* skin, NiObject* propertyState, NiObject* effectState);
GAME_FN(NiObject*, __thiscall, Instance_GetCurrentShader, 0x007D32E0, Instance* self, NiGeometry* geometry,
        NiObject* skin, NiObject* propertyState, NiObject* effectState);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
GAME_FN(NiObject*, __cdecl, Material_GetMaterial, 0x00516AC0, const re::Ptr32<const char>* name);
GAME_FN(NiObject*, __cdecl, MaterialLibrary_CreateMaterial, 0x00762980, const re::Ptr32<const char>* name);
GAME_FN(NiObject*, __cdecl, SingleShaderMaterial_Create, 0x007D37C0, const char* name, int applyDefault);   // (the fixed string as its pointer)
constexpr uintptr_t Instance_Construct = 0x007614E0;   // NiMaterialInstance's constructor
// (this chunk's, called where modes apply)
GAME_FN(Instance*, __thiscall, Instance_Assign, 0x00761450, Instance* self, const Instance* other);
GAME_FN(Instance*, __cdecl, Instances_New, 0x00761BA0, uint32_t count);
GAME_FN(void, __thiscall, Materials_SetAllocated, 0x00761BF0, MaterialArray* self, uint32_t count);
GAME_FN(uint8_t, __thiscall, Geometry_ApplyMaterial, 0x00761E80, NiGeometry* self, NiObject* material,
        uint32_t extraData);
GAME_FN(uint8_t, __thiscall, Geometry_ApplyAndSetActiveMaterial, 0x00761F70, NiGeometry* self,
        NiObject* material, uint32_t extraData);
GAME_FN(void, __thiscall, Geometry_Destructor, 0x00761D10, NiGeometry* self);

// ---- the materials

// vf22 SetDefaultMaterialNeedsUpdateFlag
void __fastcall SetDefaultMaterialNeedsUpdateFlag(NiGeometry* self, re::Edx, int flag) {
    self->defaultMaterialNeedsUpdate = uint8_t(flag);
}

// vf21 SetMaterialNeedsUpdate
void __fastcall SetMaterialNeedsUpdate(NiGeometry* self, re::Edx, int needsUpdate) {
    uint32_t count = self->materialCount;
    uint32_t value = uint8_t(needsUpdate) != 0 ? 1 : 0;
    for (uint32_t i = 0; i < count; i++) {
        self->materials[i].needsUpdate = value;
    }
}

uint32_t __fastcall GetMaterialCount(NiGeometry* self, re::Edx) {
    return self->materialCount;
}

Instance* __fastcall GetMaterialInstance(NiGeometry* self, re::Edx, uint32_t index) {
    return &self->materials[index];
}

// NiMaterialInstance::operator=
Instance* __fastcall InstanceAssign(Instance* self, re::Edx, const Instance* other) {
    if (self->cachedShader != other->cachedShader) {
        Release(self->cachedShader);
        self->cachedShader = other->cachedShader;
        AddRef(self->cachedShader);
    }
    if (self->material != other->material) {
        Release(self->material);
        self->material = other->material;
        AddRef(self->material);
    }
    self->extraData = other->extraData;
    self->needsUpdate = other->needsUpdate;
    return self;
}

// ~NiMaterialInstance
void __fastcall InstanceDestruct(Instance* self, re::Edx) {
    Release(self->material);
    Release(self->cachedShader);
}

// new NiMaterialInstance[count] (the count before the array; sizes that overflow: -1)
Instance* __cdecl InstancesNew(uint32_t count) {
    uint64_t product = uint64_t(count) * 0x10;
    uint32_t size = (product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product);
    size = size > 0xFFFFFFFB ? 0xFFFFFFFF : size + 4;
    uint32_t* p = static_cast<uint32_t*>(NiNewArray(size));
    if (p == nullptr) {
        return nullptr;
    }
    *p = count;
    VectorConstructor(p + 1, 0x10, count, reinterpret_cast<void*>(Instance_Construct));
    return reinterpret_cast<Instance*>(p + 1);
}

// the array reallocated (the instances there copied over; none copied for 0)
void __fastcall MaterialsSetAllocated(MaterialArray* self, re::Edx, uint32_t allocated) {
    if (allocated == self->allocated) {
        return;
    }
    Instance* data = nullptr;
    if (allocated != 0) {
        data = Instances_New(allocated);
        for (uint32_t i = 0; i < self->count; i++) {
            Instance_Assign(&data[i], &self->data[i]);
        }
    }
    if (self->data != nullptr) {
        Instances_VectorDelete(self->data, 3);
    }
    self->data = data;
    self->allocated = allocated;
}

// the instance added at the end (the array doubled when full)
void AppendMaterial(NiGeometry* self, NiObject* material, uint32_t extraData) {
    Instance added;
    added.material = material;
    added.cachedShader = nullptr;
    AddRef(material);
    added.extraData = extraData;
    MaterialArray* array = Materials(self);
    added.needsUpdate = 1;
    if (array->count == array->allocated) {
        Materials_SetAllocated(array, array->allocated != 0 ? array->allocated * 2 : 1);
    }
    uint32_t index = array->count;
    array->count = index + 1;
    Instance_Assign(&array->data[index], &added);
    Release(added.material);
    Release(added.cachedShader);
}

// ApplyMaterial(pkMaterial, uiMaterialExtraData): false when it's there already or null
uint8_t __fastcall ApplyMaterial(NiGeometry* self, re::Edx, NiObject* material, uint32_t extraData) {
    for (uint32_t i = 0; i < self->materialCount; i++) {
        if (self->materials[i].material == material) {
            return 0;
        }
    }
    if (material == nullptr) {
        return 0;
    }
    AppendMaterial(self, material, extraData);
    return 1;
}

// ApplyAndSetActiveMaterial(pkMaterial, uiMaterialExtraData)
uint8_t __fastcall ApplyAndSetActiveMaterial(NiGeometry* self, re::Edx, NiObject* material, uint32_t extraData) {
    for (uint32_t i = 0; i < self->materialCount; i++) {
        if (self->materials[i].material == material) {
            self->activeMaterial = i;
            return 1;
        }
    }
    if (material == nullptr) {
        self->activeMaterial = 0xFFFFFFFF;
        return 1;
    }
    self->activeMaterial = self->materialCount;
    AppendMaterial(self, material, extraData);
    return 1;
}

// ApplyAndSetActiveMaterial(kName, uiMaterialExtraData): the material by name (from the
// libraries when it doesn't exist yet)
uint8_t __fastcall ApplyAndSetActiveMaterialByName(NiGeometry* self, re::Edx, const re::Ptr32<const char>* name,
                                                   uint32_t extraData) {
    NiObject* material = Material_GetMaterial(name);
    if (material == nullptr) {
        material = MaterialLibrary_CreateMaterial(name);
    }
    return Geometry_ApplyAndSetActiveMaterial(self, material, extraData);
}

// the active material's shader, as cached / as current
NiObject* __fastcall GetCachedShader(NiGeometry* self, re::Edx) {
    if (self->activeMaterial >= self->materialCount) {
        return nullptr;
    }
    Instance* active = &self->materials[self->activeMaterial];
    NiObject* effects = self->effectState;
    AddRef(effects);
    NiObject* properties = self->propertyState;
    AddRef(properties);
    NiObject* shader = Instance_GetCachedShader(active, self, self->skinInstance, properties, effects);
    Release(properties);
    Release(effects);
    return shader;
}

NiObject* __fastcall GetShader(NiGeometry* self, re::Edx) {
    if (self->activeMaterial >= self->materialCount) {
        return nullptr;
    }
    Instance* active = &self->materials[self->activeMaterial];
    NiObject* effects = self->effectState;
    AddRef(effects);
    NiObject* properties = self->propertyState;
    AddRef(properties);
    NiObject* shader = Instance_GetCurrentShader(active, self, self->skinInstance, properties, effects);
    Release(properties);
    Release(effects);
    return shader;
}

// ---- the scene graph's

// vf32 OnVisible: appended to the culler's visible array (or handed to its Append, slot 4)
void __fastcall OnVisible(NiGeometry* self, re::Edx, void* culler) {
    if (At<uint8_t>(culler, 8) != 0) {
        Virtual<void>(culler, 4, self);
        return;
    }
    // NiVisibleArray: base, count, allocated, growth
    void* visible = At<void*>(culler, 0xC);
    uint32_t count = At<uint32_t>(visible, 4);
    if (count == At<uint32_t>(visible, 8)) {
        VisibleArray_SetAllocatedSize(visible, At<uint32_t>(visible, 0xC) + count);
    }
    At<re::Ptr32<NiGeometry>*>(visible, 0)[At<uint32_t>(visible, 4)] = self;
    At<uint32_t>(visible, 4) += 1;
}

// vf36 CalculateNormals: the data's
void __fastcall CalculateNormals(NiGeometry* self, re::Edx) {
    Virtual<void>(self->modelData, 20);
}

// vf31 UpdateWorldBound: the data's bound in the world
void __fastcall UpdateWorldBound(NiGeometry* self, re::Edx) {
    Bound_Update(&self->worldBound, &At<NiBound>(self->modelData, 0x10), &self->world);
}

// vf29 UpdateEffectsDownward: the state kept, the materials to update
void __fastcall UpdateEffectsDownward(NiGeometry* self, re::Edx, NiObject* state) {
    heapcheck::Enter(self, "NiGeometry::UpdateEffectsDownward");   // (testing: see HeapCheck.h)
    NiObject* old = self->effectState;
    if (state == nullptr) {
        if (old != nullptr) {
            heapcheck::Trace(old, heapcheck::kRelease);
            Release(old);
            self->effectState = nullptr;
        }
    } else if (old != state) {
        heapcheck::Trace(old, heapcheck::kRelease);
        Release(old);
        self->effectState = state;
        AddRef(state);
        heapcheck::Trace(state, heapcheck::kAddRef);
    }
    Virtual<void>(self, 21, 1);   // SetMaterialNeedsUpdate
    heapcheck::Leave(self);
}

// vf28 UpdatePropertiesDownward
void __fastcall UpdatePropertiesDownward(NiGeometry* self, re::Edx, NiObject* parentState) {
    re::Ptr32<NiObject> state;
    re::Ptr32<NiObject>* pushed = AVObject_PushLocalProperties(self, &state, parentState, 1);
    if (self->propertyState != *pushed) {
        Release(self->propertyState);
        self->propertyState = *pushed;
        AddRef(self->propertyState);
    }
    Release(state);
    Virtual<void>(self, 21, 1);
}

// vf24 SetSelectiveUpdateFlags: skinned geometry always updates its transforms, never rigid
void __fastcall SetSelectiveUpdateFlags(NiGeometry* self, re::Edx, uint8_t* selectiveUpdate, int transforms,
                                        uint8_t* rigid) {
    AVObject_SetSelectiveUpdateFlags(self, selectiveUpdate, transforms, rigid);
    if (self->skinInstance != nullptr) {
        self->flags |= 4;
        *selectiveUpdate = 1;
        self->flags |= 2;
        *rigid = 0;
    }
}

// vf37 CalculateConsistency(bTool): volatile with vertex controllers or (on the hardware) a skin
// without a partition / a renderer that doesn't skin; static otherwise
void __fastcall CalculateConsistency(NiGeometry* self, re::Edx, int tool) {
    uint16_t consistency = 0x4000;
    for (NiObject* controller = re::PtrCast<NiObject*>(self->controller); controller != nullptr;
         controller = At<NiObject*>(controller, 0x30)) {
        if (Virtual<uint8_t>(controller, 23)) {   // IsVertexController
            consistency = 0x8000;
            break;
        }
    }
    NiObject* skin = self->skinInstance;
    if (skin != nullptr) {
        if (uint8_t(tool) != 0) {
            if (At<void*>(skin, 0xC) == nullptr) {
                consistency = 0x8000;
            }
        } else if (At<void*>(skin, 0xC) == nullptr) {
            consistency = 0x8000;
        } else if (NiRenderer_ms_pkRenderer != nullptr) {
            if ((Virtual<uint8_t>(NiRenderer_ms_pkRenderer, 19) & 2) == 0) {
                consistency = 0x8000;
            }
        }
    }
    uint16_t& flags = At<uint16_t>(self->modelData, 0x34);
    flags = uint16_t((flags & 0x0FFF) | consistency);
}

// vf33 PurgeRendererData: the data's, the cached shaders, the skin's
void __fastcall PurgeRendererData(NiGeometry* self, re::Edx, void* renderer) {
    AVObject_PurgeRendererData(self, renderer);
    Virtual<void>(renderer, 48, self->modelData);   // PurgeGeometryData
    uint32_t count = self->materialCount;
    for (uint32_t i = 0; i < count; i++) {
        Instance* instance = &self->materials[i];
        if (instance->cachedShader != nullptr) {
            Release(instance->cachedShader);
            instance->cachedShader = nullptr;
        }
    }
    if (self->skinInstance != nullptr) {
        Virtual<void>(renderer, 53, self->skinInstance);                     // PurgeSkinInstance
        Virtual<void>(renderer, 52, At<void*>(self->skinInstance, 0xC));     // the partition's
    }
}

// vf34 RenderImmediate: the renderer's current states set, the controllers told (OnPreDisplay)
void __fastcall RenderImmediate(NiGeometry* self, re::Edx, void* renderer) {
    At<NiObject*>(renderer, 0xC) = self->propertyState;
    At<NiObject*>(renderer, 0x10) = self->effectState;
    for (NiObject* controller = re::PtrCast<NiObject*>(self->controller); controller != nullptr;
         controller = At<NiObject*>(controller, 0x30)) {
        Virtual<void>(controller, 25);
    }
}

// vf35 SetModelData (the collision object's slots 20, 19 first)
void __fastcall SetModelData(NiGeometry* self, re::Edx, NiObject* data) {
    if (data == nullptr) {
        return;
    }
    if (self->collisionObject != nullptr) {
        Virtual<void>(self->collisionObject, 20);
        Virtual<void>(self->collisionObject, 19);
    }
    if (self->modelData != data) {
        Release(self->modelData);
        self->modelData = data;
        AddRef(data);
    }
}

// vf20 ApplyTransform(kMat, kTrn, bOnLeft): into the vertices and normals, then the data's bound
void __fastcall ApplyTransform(NiGeometry* self, re::Edx, const NiMatrix3* matrix, const NiPoint3* translate,
                               int onLeft) {
    NiObject* data = self->modelData;
    uint16_t count = At<uint16_t>(data, 0x30);
    NiPoint3* vertices = At<NiPoint3*>(data, 0x20);
    NiPoint3* normals = At<NiPoint3*>(data, 0x24);
    if (uint8_t(onLeft) != 0) {
        NiMatrix3 product;
        NiMatrix3 rotate;
        Matrix3_TransposeTimes(&self->local.rotate, &rotate,
                               Matrix3_Multiply(matrix, &product, &self->local.rotate));
        NiPoint3 moved;
        const NiPoint3* m = Matrix3_MultiplyPoint(matrix, &moved, &self->local.translate);
        NiPoint3 sum;
        sum.x = m->x + translate->x;
        sum.y = m->y + translate->y;
        sum.z = m->z + translate->z;
        NiPoint3 delta;
        delta.x = sum.x - self->local.translate.x;
        delta.y = sum.y - self->local.translate.y;
        delta.z = sum.z - self->local.translate.z;
        float scale = self->local.scale;
        NiPoint3 local;
        const NiPoint3* r = Point3_TimesMatrix(&local, &delta, &self->local.rotate);
        float inverse = 1.0f / scale;
        NiPoint3 t;
        t.x = r->x * inverse;
        t.y = r->y * inverse;
        t.z = inverse * r->z;
        if (vertices != nullptr && count != 0) {
            NiPoint3* p = vertices;
            for (uint32_t n = count; n != 0; n--, p++) {
                NiPoint3 w;
                const NiPoint3* q = Matrix3_MultiplyPoint(&rotate, &w, p);
                float x = t.x + q->x;
                float y = q->y + t.y;
                float z = q->z + t.z;
                p->x = x;
                p->y = y;
                p->z = z;
            }
        }
        if (normals != nullptr) {
            NiMatrix3 inverseRotate;
            Matrix3_Inverse(&rotate, &inverseRotate);
            if (count != 0) {
                NiPoint3* p = normals;
                for (uint32_t n = count; n != 0; n--, p++) {
                    NiPoint3 w;
                    *p = *Point3_TimesMatrix(&w, p, &inverseRotate);
                }
            }
        }
    } else {
        if (vertices != nullptr && count != 0) {
            NiPoint3* p = vertices;
            for (uint32_t n = count; n != 0; n--, p++) {
                NiPoint3 w;
                const NiPoint3* q = Matrix3_MultiplyPoint(matrix, &w, p);
                float x = translate->x + q->x;
                float y = q->y + translate->y;
                float z = q->z + translate->z;
                p->x = x;
                p->y = y;
                p->z = z;
            }
        }
        if (normals != nullptr) {
            NiMatrix3 inverseRotate;
            Matrix3_Inverse(matrix, &inverseRotate);
            if (count != 0) {
                NiPoint3* p = normals;
                for (uint32_t n = count; n != 0; n--, p++) {
                    NiPoint3 w;
                    *p = *Point3_TimesMatrix(&w, p, &inverseRotate);
                }
            }
        }
    }
    Bound_ComputeFromData(&At<NiBound>(self->modelData, 0x10), count, vertices);
}

// ---- construction

NiGeometry* __fastcall Construct(NiGeometry* self, re::Edx) {
    AVObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiGeometry_vtable);
    self->propertyState = nullptr;
    self->effectState = nullptr;
    self->modelData = nullptr;
    self->skinInstance = nullptr;
    self->materials = nullptr;
    self->materialsAllocated = 0;
    self->materialCount = 0;
    self->activeMaterial = 0xFFFFFFFF;
    self->unknownE4 = nullptr;
    self->defaultMaterialNeedsUpdate = 1;
    return self;
}

NiGeometry* __fastcall ConstructWithData(NiGeometry* self, re::Edx, NiObject* data) {
    AVObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiGeometry_vtable);
    self->propertyState = nullptr;
    self->effectState = nullptr;
    self->modelData = data;
    AddRef(data);
    self->skinInstance = nullptr;
    self->materials = nullptr;
    self->materialsAllocated = 0;
    self->materialCount = 0;
    self->activeMaterial = 0xFFFFFFFF;
    self->unknownE4 = nullptr;
    self->defaultMaterialNeedsUpdate = 1;
    return self;
}

void __fastcall Destruct(NiGeometry* self, re::Edx) {
    uint32_t count = self->materialCount;
    self->vtable = reinterpret_cast<void*>(NiGeometry_vtable);
    for (uint32_t i = 0; i < count; i++) {
        Instance* instance = &self->materials[i];
        if (instance->cachedShader != nullptr) {
            Release(instance->cachedShader);
            instance->cachedShader = nullptr;
        }
        if (instance->material != nullptr) {
            Release(instance->material);
            instance->material = nullptr;
        }
        instance->needsUpdate = 1;
    }
    self->materialCount = 0;
    if (self->modelData != nullptr) {
        Release(self->modelData);
        self->modelData = nullptr;
    }
    // the members' destructors
    Release(self->unknownE4);
    if (self->materials != nullptr) {
        Instances_VectorDelete(self->materials, 3);
    }
    Release(self->skinInstance);
    Release(self->modelData);
    Release(self->effectState);
    Release(self->propertyState);
    AVObject_Destruct(self);
}

const RTTI* __fastcall GetRTTI(NiGeometry*, re::Edx) {
    return &NiGeometry_RTTI;
}

// ---- cloning and streaming

// CopyMembers: the data shared, the materials' flag
void __fastcall CopyMembers(NiGeometry* self, re::Edx, NiGeometry* dest, void* cloning) {
    Virtual<void>(dest, 35, self->modelData);
    Virtual<void>(dest, 22, int(self->defaultMaterialNeedsUpdate));
    AVObject_CopyMembers(self, dest, cloning);
}

// vf11 ProcessClone: the clone gets the skin instance's clone and the materials
void __fastcall ProcessClone(NiGeometry* self, re::Edx, void* cloning) {
    AVObject_ProcessClone(self, cloning);
    void* map = At<void*>(cloning, 0);
    auto lookup = [map](const void* key, re::Ptr32<NiObject>* value) {
        re::Ptr32<void>* table = At<re::Ptr32<void>*>(map, 8);
        for (void* item = table[reinterpret_cast<uintptr_t>(key) % At<uint32_t>(map, 4)]; item != nullptr;
             item = At<void*>(item, 0)) {
            if (At<const void*>(item, 4) == key) {
                *value = At<NiObject*>(item, 8);
                return true;
            }
        }
        return false;
    };
    re::Ptr32<NiObject> found = nullptr;
    NiGeometry* clone = lookup(self, &found) ? re::PtrCast<NiGeometry*>(found) : nullptr;
    NiObject* skin = self->skinInstance;
    if (skin != nullptr) {
        re::Ptr32<NiObject> skinClone;
        if (lookup(skin, &skinClone)) {
            Pointer_Assign(&clone->skinInstance, skinClone);
        } else {
            Pointer_Assign(&clone->skinInstance, Virtual<NiObject*>(skin, 3, cloning));
            Virtual<void>(self->skinInstance, 11, cloning);
        }
    }
    for (uint32_t i = 0; i < self->materialCount; i++) {
        Instance* instance = &self->materials[i];
        NiObject* material = instance->material;
        Geometry_ApplyMaterial(clone, material, Instance_GetExtraData(instance));
    }
    NiObject* active = self->activeMaterial < self->materialCount
                           ? self->materials[self->activeMaterial].material
                           : nullptr;
    for (uint32_t i = 0; i < clone->materialCount; i++) {
        if (clone->materials[i].material == active) {
            clone->activeMaterial = i;
            return;
        }
    }
    clone->activeMaterial = 0xFFFFFFFF;
}

// vf4: the data and skin as link IDs, the materials by name
void __fastcall LoadBinary(NiGeometry* self, re::Edx, void* stream) {
    AVObject_LoadBinary(self, stream);
    Stream_ReadLinkID(stream);   // the data
    Stream_ReadLinkID(stream);   // the skin instance
    if (StreamVersion(stream) >= 0x14020005) {
        uint32_t count = 0;
        LoadBytes(stream, &count, 4);
        for (uint32_t i = 0; i < count; i++) {
            NiFixedString name = NullFixedString;
            Stream_LoadFixedString(stream, &name);
            uint32_t extraData;
            LoadBytes(stream, &extraData, 4);
            // NiMaterial::GetMaterial, inlined
            NiObject* material = nullptr;
            if (name != NullFixedString && *reinterpret_cast<const uint16_t*>(name - 4) != 0) {
                void* map = NiMaterial_ms_pkMaterials;
                re::Ptr32<void>* table = At<re::Ptr32<void>*>(map, 8);
                for (void* item = table[re::PtrCast<uintptr_t>(name) % At<uint32_t>(map, 4)]; item != nullptr;
                     item = At<void*>(item, 0)) {
                    if (At<NiFixedString>(item, 4) == name) {
                        material = At<NiObject*>(item, 8);
                        break;
                    }
                }
            }
            if (material == nullptr) {
                material = MaterialLibrary_CreateMaterial(&name);
            }
            Geometry_ApplyMaterial(self, material, extraData);
            FixedStringRelease(&name);
        }
        LoadBytes(stream, &self->activeMaterial, 4);
        if (count == 0) {
            self->activeMaterial = 0xFFFFFFFF;
        }
    } else {
        uint8_t hasShader = 0;
        LoadBytes(stream, &hasShader, 1);
        if (hasShader != 0) {
            NiFixedString name = NullFixedString;
            if (StreamVersion(stream) < 0x14010001) {
                Stream_LoadInlineFixedString(stream, &name);
            } else {
                Stream_LoadFixedString(stream, &name);
            }
            uint32_t extraData;
            LoadBytes(stream, &extraData, 4);
            NiObject* material = SingleShaderMaterial_Create(name, 1);
            Geometry_ApplyAndSetActiveMaterial(self, material, extraData);
            FixedStringRelease(&name);
        }
    }
    if (StreamVersion(stream) >= 0x14020007) {
        uint8_t flag = 0;
        LoadBytes(stream, &flag, 1);
        self->defaultMaterialNeedsUpdate = flag != 0 ? 1 : 0;
    }
    if (NiGeometry_ms_bStreamingAppliesDefaultMaterial != 0 && self->materialCount == 0 &&
        NiRenderer_ms_pkRenderer != nullptr) {
        Virtual<void>(NiRenderer_ms_pkRenderer, 42, self);   // ApplyDefaultMaterial
        self->defaultMaterialNeedsUpdate = 1;
    }
}

// vf5
void __fastcall LinkObject(NiGeometry* self, re::Edx, void* stream) {
    AVObject_LinkObject(self, stream);
    NiObject* data = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
    if (self->modelData != data) {
        Release(self->modelData);
        self->modelData = data;
        AddRef(data);
    }
    NiObject* skin = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
    if (self->skinInstance != skin) {
        Release(self->skinInstance);
        self->skinInstance = skin;
        AddRef(skin);
    }
}

// vf6: the data, the skin, the materials' names
uint8_t __fastcall RegisterStreamables(NiGeometry* self, re::Edx, void* stream) {
    if (!AVObject_RegisterStreamables(self, stream)) {
        return 0;
    }
    Virtual<uint8_t>(self->modelData, 6, stream);
    if (self->skinInstance != nullptr) {
        Virtual<uint8_t>(self->skinInstance, 6, stream);
    }
    uint32_t count = self->materialCount;
    for (uint32_t i = 0; i < count; i++) {
        NiObject* material = self->materials[i].material;
        if (material != nullptr) {
            Virtual<uint8_t>(stream, 8, &At<NiFixedString>(material, 8));   // RegisterFixedString
        }
    }
    return 1;
}

// vf7
void __fastcall SaveBinary(NiGeometry* self, re::Edx, void* stream) {
    AVObject_SaveBinary(self, stream);
    Virtual<void>(stream, 12, self->modelData);
    Virtual<void>(stream, 12, self->skinInstance);
    uint32_t count = self->materialCount;
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        Stream_SaveFixedString(stream, &At<NiFixedString>(self->materials[i].material, 8));
        uint32_t extraData = Instance_GetExtraData(&self->materials[i]);
        SaveBytes(stream, &extraData, 4);
    }
    SaveBytes(stream, &self->activeMaterial, 4);
    uint8_t flag = self->defaultMaterialNeedsUpdate;
    SaveBytes(stream, &flag, 1);
}

// vf8
uint8_t __fastcall IsEqual(NiGeometry* self, re::Edx, NiGeometry* other) {
    if (!AVObject_IsEqual(self, other)) {
        return 0;
    }
    NiObject* skin = self->skinInstance;
    NiObject* otherSkin = other->skinInstance;
    if ((skin == nullptr) != (otherSkin == nullptr)) {
        return 0;
    }
    if (skin != nullptr && !Virtual<uint8_t>(skin, 8, otherSkin)) {
        return 0;
    }
    if (self->defaultMaterialNeedsUpdate != other->defaultMaterialNeedsUpdate) {
        return 0;
    }
    return Virtual<uint8_t>(self->modelData, 8, other->modelData);
}

// vf9
void __fastcall GetViewerStrings(NiGeometry* self, re::Edx, NiTArray<char*>* strings) {
    AVObject_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiGeometry_RTTI.name));
}

} // namespace

RE_UNHOOKED("NiGeometry::PostLinkObject", 0x00760F70);   // a jmp to NiObjectNET's

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIGEOM_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIGEOM_REVERSED("NiGeometry::SetDefaultMaterialNeedsUpdateFlag", 0x00760F60, &ni::SetDefaultMaterialNeedsUpdateFlag);
NIGEOM_REVERSED("NiGeometry::OnVisible", 0x00760F80, &ni::OnVisible);
NIGEOM_REVERSED("NiGeometry::GetMaterialCount", 0x00760FD0, &ni::GetMaterialCount);
NIGEOM_REVERSED("NiGeometry::GetMaterialInstance", 0x00760FE0, &ni::GetMaterialInstance);
NIGEOM_REVERSED("NiGeometry::SetMaterialNeedsUpdate", 0x00760FF0, &ni::SetMaterialNeedsUpdate);
NIGEOM_REVERSED("NiGeometry::CalculateNormals", 0x00761030, &ni::CalculateNormals);
NIGEOM_REVERSED("NiGeometry::UpdateWorldBound", 0x00761040, &ni::UpdateWorldBound);
NIGEOM_REVERSED("NiGeometry::UpdateEffectsDownward", 0x00761060, &ni::UpdateEffectsDownward);
NIGEOM_REVERSED("NiGeometry::ApplyTransform", 0x007610F0, &ni::ApplyTransform);
NIGEOM_REVERSED("NiGeometry::CopyMembers", 0x00761390, &ni::CopyMembers);
NIGEOM_REVERSED("NiGeometry::IsEqual", 0x007613D0, &ni::IsEqual);
NIGEOM_REVERSED("NiMaterialInstance::operator=", 0x00761450, &ni::InstanceAssign);
NIGEOM_REVERSED("NiMaterialInstance::~NiMaterialInstance", 0x00761500, &ni::InstanceDestruct);
NIGEOM_REVERSED("NiGeometry::RenderImmediate", 0x00761550, &ni::RenderImmediate);
NIGEOM_REVERSED("NiGeometry::GetCachedShader", 0x00761590, &ni::GetCachedShader);
NIGEOM_REVERSED("NiGeometry::GetShader", 0x00761630, &ni::GetShader);
NIGEOM_REVERSED("NiGeometry::SetModelData", 0x007616D0, &ni::SetModelData);
NIGEOM_REVERSED("NiGeometry::UpdatePropertiesDownward", 0x00761740, &ni::UpdatePropertiesDownward);
NIGEOM_REVERSED("NiGeometry::SetSelectiveUpdateFlags", 0x007617D0, &ni::SetSelectiveUpdateFlags);
NIGEOM_REVERSED("NiGeometry::CalculateConsistency", 0x00761810, &ni::CalculateConsistency);
NIGEOM_REVERSED("NiGeometry::PurgeRendererData", 0x00761890, &ni::PurgeRendererData);
NIGEOM_REVERSED("NiGeometry::LinkObject", 0x00761940, &ni::LinkObject);
NIGEOM_REVERSED("NiGeometry::RegisterStreamables", 0x007619E0, &ni::RegisterStreamables);
NIGEOM_REVERSED("NiGeometry::SaveBinary", 0x00761A60, &ni::SaveBinary);
NIGEOM_REVERSED("NiMaterialInstance::NewArray", 0x00761BA0, &ni::InstancesNew);
NIGEOM_REVERSED("NiGeometry::SetMaterialsAllocated", 0x00761BF0, &ni::MaterialsSetAllocated);
NIGEOM_REVERSED("NiGeometry::GetViewerStrings", 0x00761C50, &ni::GetViewerStrings);
NIGEOM_REVERSED("NiGeometry::NiGeometry", 0x00761CA0, &ni::Construct);
NIGEOM_REVERSED("NiGeometry::GetRTTI", 0x00761D00, &ni::GetRTTI);
NIGEOM_REVERSED("NiGeometry::~NiGeometry", 0x00761D10, &ni::Destruct);
NIGEOM_REVERSED("NiGeometry::ApplyMaterial", 0x00761E80, &ni::ApplyMaterial);
NIGEOM_REVERSED("NiGeometry::ApplyAndSetActiveMaterial", 0x00761F70, &ni::ApplyAndSetActiveMaterial);
NIGEOM_REVERSED("NiGeometry::ApplyAndSetActiveMaterialByName", 0x00762080, &ni::ApplyAndSetActiveMaterialByName);
NIGEOM_REVERSED("NiGeometry::ProcessClone", 0x007620B0, &ni::ProcessClone);
NIGEOM_REVERSED("NiGeometry::LoadBinary", 0x007621F0, &ni::LoadBinary);
NIGEOM_REVERSED("NiGeometry::NiGeometryWithData", 0x007624E0, &ni::ConstructWithData);
