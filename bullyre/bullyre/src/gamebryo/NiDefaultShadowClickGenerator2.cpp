// NiDefaultShadowClickGenerator's directional light (slot 10 HandleDirectionalLight: an
// orthographic camera over the casters' bound, along the light's direction), the directional
// shadow parameters it fits to that bound, and its map of the lights' last directions
// (NiTPointerMap<void*, float>::GetAt). NiDefaultShadowClickGenerator.cpp has the class and the
// spot and point lights.
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in its order.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiShadowGenerator.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

GAME_VAR(uint32_t, kNi3DRenderViewClassIndex, 0x020E35D0);
GAME_VAR(uint8_t*, ms_pkShadowManager, 0x020E2410);
GAME_VAR(double, kHalf, 0x008FF1F8);       // 0.5
GAME_VAR(double, kThree, 0x00900988);      // 3
GAME_VAR(double, kSixteen, 0x00919BD0);    // 16
GAME_VAR(float, kDirectionEpsilon, 0x00900A88);   // 1e-6

constexpr int kDirectionalSwapProcessor = 0xC8;

// NiTPointerMap<void*, float>: an item is next, key, value
struct MapItem {   // (the game's layout: 0xC)
    re::Ptr32<MapItem> next;
    re::Ptr32<void> key;
    float value;
};
static_assert(sizeof(MapItem) == 0xC);
struct Map {
    re::Ptr32<void> vtable;
    uint32_t hashSize;   // +0x4
    re::Ptr32<re::Ptr32<MapItem>> buckets;   // +0x8
    uint32_t count;      // +0xC
};

using ObjectList = NiTPointerList<NiObject*>;

// a directional light's world direction (NiDirectionalLight +0x124)
inline const NiPoint3& Direction(NiAVObject* light) {
    return At<NiPoint3>(light, 0x124);
}

struct NiFrustum {
    float left, right, top, bottom, nearPlane, farPlane;
    uint8_t ortho;
    uint8_t pad[3];
};
static_assert(sizeof(NiFrustum) == 0x1C);

GAME_FN(void, __thiscall, ShadowGenerator_GetCasterGeometryList, 0x00773360, NiShadowGenerator* self,
        ObjectList* list);
GAME_FN(void, __thiscall, ObjectList_AddTail, 0x007843D0, ObjectList* self, const re::Ptr32<NiObject>* element);
GAME_FN(void, __thiscall, ObjectList_RemoveAll, 0x0075DB60, ObjectList* self);
GAME_FN(void, __thiscall, RenderView_AppendScene, 0x007CFB00, NiObject* self, NiObject* scene);
GAME_FN(void, __thiscall, Bound_Merge, 0x007D2E60, NiBound* self, const NiBound* other);
GAME_FN(NiFrustum*, __thiscall, Frustum_Construct, 0x007718A0, NiFrustum* self, float left, float right, float top,
        float bottom, float nearPlane, float farPlane, int ortho);
GAME_FN(void, __thiscall, Camera_SetViewFrustum, 0x00762D60, NiAVObject* self, const NiFrustum* frustum);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, NiAVObject* self, float time, int updateControllers);
GAME_FN(void, __thiscall, ViewRenderClick_SetProcessor, 0x007CF610, NiObject* self, NiRefObject* processor,
        void* data);
GAME_FN(void, __thiscall, ClickGenerator_AddRenderClick, 0x007CF680, void* self, NiObject* click);
GAME_FN(void, __thiscall, Map_SetAt, 0x0076E930, Map* self, void* key, float value);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Pointer_Construct, 0x00401B10, re::Ptr32<NiObject>* self, NiObject* object);   // NiPointer(T*)
GAME_FN(void, __thiscall, Pointer_Destruct, 0x007549D0, re::Ptr32<NiObject>* self);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, ClickGenerator_ComputeDirectionalShadow, 0x007CF3D0, void* self, const NiBound* bound,
        NiShadowGenerator* generator, NiAVObject* light, float* nearPlane, float* farPlane, float* size,
        NiPoint3* position);
GAME_FN(uint8_t, __thiscall, Map_GetAt, 0x007CF5D0, Map* self, void* key, float* value);

inline bool HasClass(const void* rtti, uint32_t index) {
    return (At<uint32_t>(rtti, 0xC + (index >> 5) * 4) & (1u << (index & 0x1F))) != 0;
}

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// NiTPointerList<NiAVObjectPtr>::AddTail with a reference made for it
inline void AddScene(ObjectList* scenes, NiObject* scene) {
    re::Ptr32<NiObject> held = scene;
    if (held != nullptr) {
        Increment(&held->refCount);
    }
    ObjectList_AddTail(scenes, &held);
    ReleaseRef(held);
}

// the camera's orthographic frustum (the size each way), its position
inline void SetOrthographic(NiAVObject* camera, float size, float nearPlane, float farPlane,
                            const NiPoint3& position) {
    NiFrustum frustum;
    Frustum_Construct(&frustum, -size, size, size, -size, nearPlane, farPlane, 1);
    Camera_SetViewFrustum(camera, &frustum);
    camera->local.translate = position;
}

} // namespace

// 0x7CF3D0: the directional shadow's parameters from the casters' bound, as the generator's flags
// ask: 0x40 the near plane (half the radius), 0x80 the far plane (48 radii), 0x100 the size (the
// radius), 0x200 the camera two radii back from the center along the light (else the light's
// position)
void __fastcall NiDefaultShadowClickGenerator_ComputeDirectionalShadow(void*, re::Edx, const NiBound* bound,
                                                                       NiShadowGenerator* generator,
                                                                       NiAVObject* light, float* nearPlane,
                                                                       float* farPlane, float* size,
                                                                       NiPoint3* position) {
    if (((uint8_t(generator->flags) >> 6) & 1) != 0) {
        float value = float(double(bound->radius) * kHalf);
        *nearPlane = value;
        generator->unknown58 = value;
    }
    if (((uint8_t(generator->flags) >> 7) & 1) != 0) {
        float value = float(double(bound->radius) * kThree);
        value = float(double(value) * kSixteen);
        *farPlane = value;
        generator->unknown5C = value;
    }
    if ((At<uint8_t>(generator, 9) & 1) != 0) {
        float value = bound->radius;
        *size = value;
        generator->unknown60 = value;
    }
    if (((generator->flags >> 9) & 1) == 0) {
        *position = light->world.translate;
        return;
    }
    float distance = bound->radius + bound->radius;
    const NiPoint3& direction = Direction(light);
    float x = direction.x * distance;
    float y = direction.y * distance;
    float z = distance * direction.z;
    NiPoint3 result;
    result.x = bound->center.x - x;
    result.y = bound->center.y - y;
    result.z = bound->center.z - z;
    *position = result;
}

// 0x7CF5D0: NiTPointerMap<void*, float>::GetAt(key, value&)
uint8_t __fastcall NiTPointerMap_VoidFloat_GetAt(Map* self, re::Edx, void* key, float* value) {
    for (MapItem* item = self->buckets[reinterpret_cast<uintptr_t>(key) % self->hashSize]; item != nullptr;
         item = item->next) {
        if (item->key == key) {
            *value = item->value;
            return 1;
        }
    }
    return 0;
}

// 0x7D0820: HandleDirectionalLight (slot 10)(generator, light, regenerateViews, updateShadowMaps):
// the map's render view gets the casters, the camera the light's rotation and an orthographic
// frustum fitted to the casters' bound (with the generator's auto flags 0x3C0; else the stored
// near / far / size and the light's position). Without new views the frustum is fitted again
// only when the light's direction changed (a hash of it kept per light)
uint8_t __fastcall NiDefaultShadowClickGenerator_HandleDirectionalLight(void* self, re::Edx,
                                                                         NiShadowGenerator* generator,
                                                                         NiAVObject* light, int regenerateViews,
                                                                         int updateShadowMaps) {
    void* map = Virtual<void*>(self, 11, generator, 0u, static_cast<void*>(nullptr));
    if (map == nullptr) {
        return 0;
    }
    NiObject* click = At<NiObject*>(map, 0xC);
    NiObject* firstView = At<NiTListNode<NiObject*>*>(click, 0x50)->element;
    NiObject* view = firstView != nullptr && HasClass(Virtual<void*>(firstView, 2), kNi3DRenderViewClassIndex)
                         ? firstView
                         : nullptr;
    NiAVObject* camera = At<NiAVObject*>(view, 0x20);
    ObjectList* casters = &At<ObjectList>(self, 0x3C);
    if (re::Flag(regenerateViews)) {
        ObjectList* scenes = &At<ObjectList>(view, 0x28);
        ObjectList_RemoveAll(scenes);
        ShadowGenerator_GetCasterGeometryList(generator, casters);
        float nearPlane = generator->unknown58;
        float farPlane = generator->unknown5C;
        float size = generator->unknown60;
        NiTListNode<NiObject*>* node = casters->head;
        NiPoint3 position;
        if ((generator->flags & 0x3C0) != 0) {
            NiBound bound;
            if (node != nullptr) {
                NiObject* geometry = node->element;
                node = node->next;
                RenderView_AppendScene(view, geometry);
                bound = At<NiBound>(geometry, 0x1C);
                for (; node != nullptr; node = node->next) {
                    geometry = node->element;
                    Bound_Merge(&bound, &At<NiBound>(geometry, 0x1C));
                    AddScene(scenes, geometry);
                }
            }
            // (with no casters the bound is the stack's leftovers, as the game has it)
            ClickGenerator_ComputeDirectionalShadow(self, &bound, generator, light, &nearPlane, &farPlane, &size,
                                                    &position);
        } else {
            for (; node != nullptr; node = node->next) {
                AddScene(scenes, node->element);
            }
            position = light->world.translate;
        }
        SetOrthographic(camera, size, nearPlane, farPlane, position);
    } else if ((generator->flags & 0x3C0) == 0) {
        camera->local.translate = light->world.translate;
    } else {
        Map* directions = &At<Map>(self, 0x4C);
        float last = 0.0f;
        const NiPoint3& direction = Direction(light);
        float hash = direction.y + direction.y;
        hash = hash + float(double(direction.x) * kThree);
        hash = hash + float(double(direction.z) * kHalf);
        bool changed = !Map_GetAt(directions, light, &last);
        if (!changed) {
            float difference = std::fabs(last - hash);
            changed = !(difference <= kDirectionEpsilon) && !std::isnan(difference);
        }
        if (changed) {
            Map_SetAt(directions, light, hash);
            float nearPlane = generator->unknown58;
            float farPlane = generator->unknown5C;
            float size = generator->unknown60;
            NiTListNode<NiObject*>* node = At<ObjectList>(view, 0x28).head;
            NiBound bound;
            if (node != nullptr) {
                re::Ptr32<NiObject> scene = nullptr;
                NiObject* first = node->element;
                node = node->next;
                Pointer_Construct(&scene, first);
                bound = At<NiBound>(scene, 0x1C);
                for (; node != nullptr; node = node->next) {
                    NiObject* next = node->element;
                    if (scene != next) {
                        ReleaseRef(scene);
                        scene = next;
                        if (next != nullptr) {
                            Increment(&next->refCount);
                        }
                    }
                    Bound_Merge(&bound, &At<NiBound>(scene, 0x1C));
                }
                Pointer_Destruct(&scene);
            }
            NiPoint3 position;
            ClickGenerator_ComputeDirectionalShadow(self, &bound, generator, light, &nearPlane, &farPlane, &size,
                                                    &position);
            SetOrthographic(camera, size, nearPlane, farPlane, position);
        }
    }
    camera->local.rotate = light->world.rotate;
    camera->local.scale = std::fabs(light->world.scale);
    AVObject_Update(camera, 0.0f, 1);
    NiRefObject* processor = At<NiRefObject*>(ms_pkShadowManager, kDirectionalSwapProcessor);
    if (processor != nullptr) {
        Increment(&processor->refCount);
    }
    ViewRenderClick_SetProcessor(click, processor, nullptr);
    ReleaseRef(processor);
    if (re::Flag(updateShadowMaps)) {
        ClickGenerator_AddRenderClick(self, click);
        void* technique = generator->technique;
        Virtual<void>(technique, 3, map);
        Virtual<void>(technique, 2, map, self);
    }
    generator->flags &= 0xFFFD;
    return 1;
}

#define NIDEFSHADOWCLICK2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIDEFSHADOWCLICK2_REVERSED("NiDefaultShadowClickGenerator::ComputeDirectionalShadow", 0x007CF3D0,
                           NiDefaultShadowClickGenerator_ComputeDirectionalShadow);
NIDEFSHADOWCLICK2_REVERSED("NiTPointerMap<void*,float>::GetAt", 0x007CF5D0, NiTPointerMap_VoidFloat_GetAt);
NIDEFSHADOWCLICK2_REVERSED("NiDefaultShadowClickGenerator::HandleDirectionalLight", 0x007D0820,
                           NiDefaultShadowClickGenerator_HandleDirectionalLight);

} // namespace ni
