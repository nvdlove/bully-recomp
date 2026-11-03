// The sample framework's camera navigation (no NiRTTI: Gamebryo 2.x's NiNavBaseController,
// NiNavFlyController, NiNavOrbitController and NiNavManager, named after their strings): each
// controller has six controls, each driven both ways by a keyboard key, a mouse button or axis,
// a game pad button or stick, reserved with the UI manager while it's active. Every frame the
// manager polls the current controller's analogue inputs (mouse movement while its button is
// held, the sticks) into its control amounts, then the controller moves its object: the "Flying"
// one moves and turns it about its own axes (yaw about the up direction when there is one),
// the "Orbiting" one turns it about a centre point and pans the centre. Float maths in float,
// in the game's order (the main thread's x87 runs at 24 bits).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSampleFramework.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::sample {
namespace {

GAME_FN(float, __cdecl, NiGetCurrentTimeInSec, 0x007537B0);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_ConstructSized, 0x007740D0, re::Ptr32<char>* self, uint32_t size);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, re::Ptr32<char>* self);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_AssignText, 0x007745E0, re::Ptr32<char>* self, const char* text);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_AssignChar, 0x00774600, re::Ptr32<char>* self, char c);
GAME_FN(float, __thiscall, Point3_Unitize, 0x00415130, NiPoint3* self);
GAME_FN(NiPoint3*, __thiscall, Point3_UnitCross, 0x00762F30, const NiPoint3* self, NiPoint3* result,
        const NiPoint3* other);
GAME_FN(void, __thiscall, Matrix3_MakeRotation, 0x00768820, NiMatrix3* self, float angle, float x, float y, float z);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Multiply, 0x00768920, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(uint8_t, __thiscall, Matrix3_Reorthogonalize, 0x00768CD0, NiMatrix3* self);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_SetCols, 0x00768780, NiMatrix3* self, const NiPoint3* col0,
        const NiPoint3* col1, const NiPoint3* col2);
GAME_FN(NiPoint3*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(void, __thiscall, Transform_Invert, 0x00770950, const NiTransform* self, NiTransform* result);
GAME_FN(NiTransform*, __thiscall, Transform_Multiply, 0x00406830, const NiTransform* self, NiTransform* result,
        const NiTransform* other);
GAME_FN(int32_t, __thiscall, GamePad_GetAxisValue, 0x008D0AA0, void* self, uint32_t axis);
GAME_FN(float, __cdecl, NiSelect, 0x008382D0, int flag, float a, float b);
GAME_FN(void, __thiscall, ControlInputArray_SetSize, 0x008349E0, NiTArray<NiNavControlInput>* self, uint32_t size);
GAME_FN(void, __thiscall, ControlInputArray_SetAt, 0x00834B70, NiTArray<NiNavControlInput>* self, uint32_t index,
        const NiNavControlInput* input);
GAME_FN(void, __thiscall, BoolArray_SetSize, 0x00834CA0, NiTArray<uint8_t>* self, uint32_t size);
GAME_FN(void, __thiscall, FlyController_Destruct, 0x00834060, NiNavFlyController* self);
GAME_FN(void, __thiscall, OrbitController_Destruct, 0x008357E0, NiNavOrbitController* self);
GAME_FN(void, __thiscall, NavManager_Construct, 0x008339D0, NiNavManager* self);

GAME_VAR(NiPoint3, NiPoint3_ZERO, 0x020E2D44);
GAME_VAR(NiPoint3, NiPoint3_UNIT_X, 0x00BC8A90);
GAME_ARRAY(const char*, kFlyControlNames, 0x00BC8F8C);
GAME_ARRAY(const char*, kOrbitControlNames, 0x00BC8FAC);
const float& kFlyMinScale = *reinterpret_cast<const float*>(0x009892F4);     // 1e-5
const float& kOrbitMinScale = *reinterpret_cast<const float*>(0x00989388);   // 1e-5

inline float FrameTime() {
    return NiNavManager_ms_pkNavManager->frameTime;
}

inline bool IsZero(const NiPoint3& p) {
    const NiPoint3& z = NiPoint3_ZERO;
    return p.x == z.x && p.y == z.y && p.z == z.z;
}

// the "empty" control input (what SetAt and SetSize compare with)
inline NiNavControlInput EmptyInput() {
    NiNavControlInput input{};
    input.mouseButton = -1;
    input.gamePadButton = -1;
    return input;
}

// a column of a rotation as a point
inline NiPoint3 Column(const NiMatrix3& m, int c) {
    return NiPoint3{m.m[0][c], m.m[1][c], m.m[2][c]};
}

// position + scale * the rotation's first column (the products first)
inline NiPoint3 PointInFront(const NiAVObject* object, float scale) {
    float x = object->world.rotate.m[0][0] * scale;
    float y = object->world.rotate.m[1][0] * scale;
    float z = scale * object->world.rotate.m[2][0];
    return NiPoint3{object->world.translate.x + x, object->world.translate.y + y, object->world.translate.z + z};
}

// ---- NiNavManager

// Update: the frame time, then the current controller's update
void __fastcall Manager_Update(NiNavManager* self, re::Edx) {
    float now = NiGetCurrentTimeInSec();
    self->frameTime = now - self->lastTime;
    self->lastTime = now;
    if (self->controllers.size != 0) {
        NavController_Update(self->controllers.base[self->current]);
    }
}

// FlyController::SetReference(object): the object whose rotation the moves follow (none while
// active: deactivated)
void __fastcall Fly_SetReference(NiNavFlyController* self, re::Edx, NiAVObject* object) {
    AssignRef(self->reference, object);
    if (self->reference == nullptr) {
        NiNavState state;   // (object and scale written by the deactivation)
        state.keepScale = 1;
        if (self->active != 0) {
            Virtual<uint8_t>(self, 2, 0, &state);   // SetActive
        }
    }
}

// FlyController::UpdateLookAt: the point scale in front of the controlled object
void __fastcall Fly_UpdateLookAt(NiNavFlyController* self, re::Edx) {
    self->lookAt = PointInFront(self->controlled, self->scale);
}

// OrbitController::SetControlled(object): (none while active: deactivated) the centre put scale
// in front of it
void __fastcall Orbit_SetControlled(NiNavOrbitController* self, re::Edx, NiAVObject* object) {
    AssignRef(self->controlled, object);
    if (self->controlled == nullptr) {
        NiNavState state;
        state.keepScale = 1;
        if (self->active == 0) {
            return;
        }
        Virtual<uint8_t>(self, 2, 0, &state);
    }
    if (self->active != 0) {
        self->center = PointInFront(self->controlled, self->scale);
    }
}

// FlyController::SetControlled(object): (none while active: deactivated; with the UI manager
// gone that leaves it active and the look-at reads null: the game's) the look-at updated
void __fastcall Fly_SetControlled(NiNavFlyController* self, re::Edx, NiAVObject* object) {
    AssignRef(self->controlled, object);
    if (self->controlled == nullptr) {
        NiNavState state;
        state.keepScale = 1;
        if (self->active == 0) {
            return;
        }
        Virtual<uint8_t>(self, 2, 0, &state);
    }
    if (self->active != 0) {
        FlyController_UpdateLookAt(self);
    }
}

// NiNavManager::GetCurrentController
NiNavBaseController* __fastcall Manager_GetCurrentController(NiNavManager* self, re::Edx) {
    if (self->current < self->controllers.size) {
        return self->controllers.base[self->current];
    }
    return nullptr;
}

// NiNavManager::CycleController: the current one deactivated (handing its object and scale
// over), the next activated with them, the change signalled
void __fastcall Manager_CycleController(NiNavManager* self, re::Edx) {
    uint32_t count = self->controllers.size;
    if (count == 0) {
        return;
    }
    NiNavState state;
    state.keepScale = 0;
    Virtual<uint8_t>(self->controllers.base[self->current], 2, 0, &state);
    self->current = (self->current + 1) % count;
    NiNavBaseController* next = self->controllers.base[self->current];
    Virtual<uint8_t>(next, 2, 1, &state);
    Signal1_Emit(&self->controllerChanged, uint32_t(reinterpret_cast<uintptr_t>(next)));
}

// NiNavManager's constructor
NiNavManager* __fastcall Manager_Construct(NiNavManager* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiNavManagerVtable);
    self->controllerChanged.vtable = reinterpret_cast<void*>(kNiUISignalVtable);
    self->controllerChanged.set.base = nullptr;
    self->controllerChanged.set.alloced = 0;
    self->controllerChanged.set.used = 0;
    self->lastTime = 0.0f;
    self->frameTime = 0.0f;
    self->controllers.maxSize = 0;
    self->controllers.size = 0;
    self->controllers.esize = 0;
    self->controllers.base = nullptr;
    self->controllers.growBy = 1;
    self->controllers.vtable = reinterpret_cast<void*>(kNiTObjectArrayVtable);
    self->current = 0;
    return self;
}

// NiNavManager's deleting destructor
NiNavManager* __fastcall Manager_DeletingDestructor(NiNavManager* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kNiNavManagerVtable);
    self->controllers.vtable = reinterpret_cast<void*>(kNiTObjectArrayVtable);
    PointerArray_Delete(self->controllers.base);
    Signal_Destruct(&self->controllerChanged);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x34);
    }
    return self;
}

// NiNavManager::Create: the one manager, once the UI manager's there
uint8_t __cdecl Manager_Create() {
    if (UIManager_Get() == nullptr) {
        return 0;
    }
    if (NiNavManager_ms_pkNavManager == nullptr) {
        auto* manager = static_cast<NiNavManager*>(NiNew(0x34));
        if (manager != nullptr) {
            NavManager_Construct(manager);
            NiNavManager_ms_pkNavManager = manager;
            return 1;
        }
        NiNavManager_ms_pkNavManager = nullptr;
    }
    return 1;
}

// NiNavManager::AddNavigationController(controller): appended (the first one activated and
// signalled, the others deactivated); its index (-1 none or there already)
uint32_t __fastcall Manager_AddNavigationController(NiNavManager* self, re::Edx, NiNavBaseController* controller) {
    if (controller == nullptr) {
        return 0xFFFFFFFF;
    }
    uint16_t count = self->controllers.size;
    for (uint32_t i = 0; i < count; i++) {
        if (self->controllers.base[i] == controller) {
            return 0xFFFFFFFF;
        }
    }
    NiNavState state;   // (uninitialized: the activation only reads keepScale)
    if (count == 0) {
        self->current = 0;
        state.keepScale = 1;
        Virtual<uint8_t>(controller, 2, 1, &state);
        Signal1_Emit(&self->controllerChanged, uint32_t(reinterpret_cast<uintptr_t>(controller)));
    } else {
        Virtual<uint8_t>(controller, 2, 0, &state);
    }
    NiNavBaseController* held = controller;
    ni::sys::Increment(&controller->refCount);
    uint32_t index = self->controllers.size;
    if (index >= self->controllers.maxSize) {
        ObjectArray_SetSize(&self->controllers, self->controllers.growBy + index);
    }
    ObjectArray_SetAt(&self->controllers, index, re::PtrCast<const re::Ptr32<void>*>(&held));
    if (ni::sys::Decrement(&controller->refCount) == 0) {
        Virtual<void>(controller, 1);
    }
    return index;
}

// NiAVObject::SetWorldTransform(transform): the local transform that gives it (through the
// parent's inverse), the scale made positive
void __fastcall SetWorldTransform(NiAVObject* self, re::Edx, const NiTransform* transform) {
    if (self->parent != nullptr) {
        NiTransform inverse;
        Transform_Invert(&self->parent->world, &inverse);
        NiTransform local;
        Transform_Multiply(&inverse, &local, transform);
        self->local = local;
    } else {
        self->local = *transform;
    }
    self->local.scale = std::fabs(self->local.scale);
}

// ---- the controls' slot functions (each takes the button state, unused). The amount: the
// scale * 1.5 (or 2) * the frame time, added or taken off

template <int Control, int Field, int ScaleField, bool Double, bool Add>
void __fastcall ControlSlot(NiNavBaseController* self, re::Edx, uint32_t) {
    if (self->controlActive.base[Control] == 0 || self->active == 0) {
        return;
    }
    float scale = At<float>(self, ScaleField);
    float amount = Double ? (scale + scale) * FrameTime() : scale * 1.5f * FrameTime();
    float& field = At<float>(self, Field);
    field = Add ? amount + field : field - amount;
}

// the zoom ones: the motion / radius factor pow(base, frame time) or its inverse
template <uintptr_t Base, bool Inverse>
void __fastcall ZoomSlot(NiNavBaseController* self, re::Edx, uint32_t) {
    if (self->controlActive.base[0] == 0 || self->active == 0) {
        return;
    }
    double base = *reinterpret_cast<const double*>(Base);
    float power = float(crt::Pow(base, double(FrameTime())));
    At<float>(self, 0x80) = Inverse ? 1.0f / power : power;
}

constexpr uintptr_t kFlyZoomBase = 0x00900E58;     // 0.7
constexpr uintptr_t kOrbitZoomBase = 0x00900D50;   // 0.6

// ---- the "Flying" controller

void __fastcall Fly_Destruct(NiNavFlyController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiNavFlyControllerVtable);
    ClearRef(self->reference);
    ClearRef(self->controlled);
    Release(self->controlled);
    Release(self->reference);
    NavController_Destruct(self);
}

const char* __fastcall Fly_GetControlName(NiNavFlyController*, re::Edx, uint32_t control) {
    return kFlyControlNames[control];
}

// IsControlActive(i) (shared by every controller)
uint8_t __fastcall IsControlActive(NiNavBaseController* self, re::Edx, uint32_t control) {
    return self->controlActive.base[control];
}

// the zoom from a signed input: up zooms by pow(base, v * dt), down by its inverse
inline void ZoomFromInput(NiNavBaseController* self, double base, float value, bool invert) {
    if (value > 0.0f) {
        float t = value * FrameTime();
        float power = float(crt::Pow(base, double(t)));
        At<float>(self, 0x80) = invert ? 1.0f / power : power;
    } else if (value < 0.0f) {
        float t = FrameTime() * -value;
        float power = float(crt::Pow(base, double(t)));
        At<float>(self, 0x80) = invert ? power : 1.0f / power;
    }
}

// MouseInput(control, value): the mouse's amount added to the control (4: none)
void __fastcall Fly_MouseInput(NiNavFlyController* self, re::Edx, uint32_t control, float value) {
    if (self->controlActive.base[control] == 0 || self->active == 0 || control > 5) {
        return;
    }
    switch (control) {
    case 0:
        ZoomFromInput(self, *reinterpret_cast<const double*>(kFlyZoomBase), value, false);
        break;
    case 1:
        self->rotateHorizontal = FrameTime() * (value + value) + self->rotateHorizontal;
        break;
    case 2:
        self->rotateVertical = FrameTime() * (value + value) + self->rotateVertical;
        break;
    case 3:
        self->flyHorizontal = self->scale * 1.5f * value * FrameTime() + self->flyHorizontal;
        break;
    case 5:
        self->flyForward = self->scale * 1.5f * value * FrameTime() + self->flyForward;
        break;
    default:
        break;
    }
}

// GamePadInput(control, value): the stick's amount (forward the other way)
void __fastcall Fly_GamePadInput(NiNavFlyController* self, re::Edx, uint32_t control, float value) {
    if (self->controlActive.base[control] == 0 || self->active == 0 || control > 5) {
        return;
    }
    switch (control) {
    case 0:
        ZoomFromInput(self, *reinterpret_cast<const double*>(kFlyZoomBase), value, false);
        break;
    case 1:
        self->rotateHorizontal = FrameTime() * (value + value) + self->rotateHorizontal;
        break;
    case 2:
        self->rotateVertical = FrameTime() * (value + value) + self->rotateVertical;
        break;
    case 3:
        self->flyHorizontal = self->scale * 1.5f * value * FrameTime() + self->flyHorizontal;
        break;
    case 4:
        self->flyVertical = self->scale * 1.5f * value * FrameTime() + self->flyVertical;
        break;
    case 5:
        self->flyForward = self->flyForward - self->scale * 1.5f * value * FrameTime();
        break;
    }
}

// UpdatePose: the look-at kept at the scaled distance, the object moved along the reference's
// axes (forward, sideways, up), turned by the pitch about its right axis and the yaw about the
// up direction (or its own up), kept upright against the up direction, the amounts reset
void __fastcall Fly_UpdatePose(NiNavFlyController* self, re::Edx) {
    NiMatrix3 rotation = self->reference->world.rotate;
    NiPoint3 forward = Column(rotation, 0);
    NiPoint3 up = Column(rotation, 1);
    NiPoint3 right = Column(rotation, 2);
    NiTransform transform = self->controlled->world;
    float scale = self->scale * self->motionScale;
    NiPoint3 position = transform.translate;
    if (!(scale > kFlyMinScale)) {
        scale = kFlyMinScale;
    }
    self->scale = scale;
    NiPoint3* lookAt = &self->lookAt;
    NiPoint3 toLookAt{lookAt->x - transform.translate.x, lookAt->y - transform.translate.y,
                      lookAt->z - transform.translate.z};
    Point3_Unitize(&toLookAt);
    PointAlongAxis(self, &toLookAt, scale, &position, lookAt);

    float amount = self->flyForward;
    NiPoint3 step{forward.x * amount, forward.y * amount, amount * forward.z};
    position = NiPoint3{transform.translate.x + step.x, transform.translate.y + step.y,
                        transform.translate.z + step.z};
    PointAlongAxis(self, &forward, self->flyForward, lookAt, lookAt);

    amount = self->flyHorizontal;
    step = NiPoint3{right.x * amount, right.y * amount, amount * right.z};
    NiPoint3 moved{step.x + position.x, step.y + position.y, step.z + position.z};
    PointAlongAxis(self, &right, self->flyHorizontal, lookAt, lookAt);

    amount = self->flyVertical;
    step = NiPoint3{up.x * amount, up.y * amount, amount * up.z};
    NiPoint3 newPosition{step.x + moved.x, step.y + moved.y, step.z + moved.z};
    PointAlongAxis(self, &up, self->flyVertical, lookAt, lookAt);

    NiMatrix3 original = transform.rotate;
    NiMatrix3 pitch;
    Matrix3_MakeRotation(&pitch, self->pitchSign * self->rotateVertical, right.x, right.y, right.z);
    NiMatrix3 yaw;
    const NiPoint3& upDir = self->upDir;
    float yawAngle = self->yawSign * self->rotateHorizontal;
    if (!IsZero(upDir)) {
        Matrix3_MakeRotation(&yaw, yawAngle, upDir.x, upDir.y, upDir.z);
    } else {
        Matrix3_MakeRotation(&yaw, yawAngle, up.x, up.y, up.z);
    }
    NiMatrix3 turned;
    NiMatrix3 result;
    Matrix3_Multiply(Matrix3_Multiply(&yaw, &turned, &pitch), &result, &original);
    Matrix3_Reorthogonalize(&result);
    if (!IsZero(upDir)) {
        float dot = (upDir.y * result.m[1][1] + upDir.x * result.m[0][1]) + upDir.z * result.m[2][1];
        if (dot <= 0.0f) {
            // pitched over the top: the yaw alone
            NiMatrix3 yawOnly;
            result = *Matrix3_Multiply(&yaw, &yawOnly, &original);
            Matrix3_Reorthogonalize(&result);
        }
        NiPoint3 direction = Column(result, 0);
        NiPoint3 side;
        Point3_UnitCross(&direction, &side, &upDir);
        NiPoint3 newUp;
        Point3_UnitCross(&side, &newUp, &direction);
        NiMatrix3 upright;
        result = *Matrix3_SetCols(&upright, &direction, &newUp, &side);
    }
    transform.rotate = result;
    transform.translate = newPosition;
    AVObject_SetWorldTransform(self->controlled, &transform);
    self->motionScale = 0.0f;
    self->rotateHorizontal = 0.0f;
    self->rotateVertical = 0.0f;
    self->flyHorizontal = 0.0f;
    self->flyVertical = 0.0f;
    self->flyForward = 0.0f;
    self->motionScale = 1.0f;
}

// SetActive(active, state): the base's (the inputs reserved or freed); activating takes the
// scale handed over (unless kept) and places the look-at, deactivating hands its scale and
// reference over
uint8_t __fastcall Fly_SetActive(NiNavFlyController* self, re::Edx, int active, NiNavState* state) {
    uint8_t result = NavController_SetActive(self, active, state);
    if (uint8_t(active) != 0) {
        if (state->keepScale == 0) {
            self->scale = state->scale;
        }
        if (self->controlled != nullptr) {
            FlyController_UpdateLookAt(self);
        }
    } else {
        state->scale = self->scale;
        state->object = self->reference;
    }
    return result;
}

// the deleting destructors
NiNavFlyController* __fastcall Fly_DeletingDestructor(NiNavFlyController* self, re::Edx, int flags) {
    FlyController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xC8);
    }
    return self;
}

// a controller's control: its input (into the inputs array) and its slot (a member slot on the
// controller's function, into the slots array)
struct ControlEntry {
    uint32_t index;
    uint8_t devices;
    uint32_t key;
    uint32_t mouseAxis;
    int32_t gamePadButton;
    uint32_t gamePadAxis;
    uintptr_t function;
    bool slotFirst;   // (the fly's last: the slot set before the input)
};

void AddControl(NiNavBaseController* self, NiNavControlInput& input, const ControlEntry& entry) {
    input.devices = entry.devices;
    input.key = entry.key;
    input.keyModifiers = 0;
    input.mouseButton = -1;
    input.mouseModifiers = 0;
    input.mouseAxis = entry.mouseAxis;
    input.gamePadButton = entry.gamePadButton;
    input.gamePadModifiers = 0;
    input.gamePadAxis = entry.gamePadAxis;
    if (!entry.slotFirst) {
        ControlInputArray_SetAt(&self->inputs, entry.index, &input);
    }
    auto* slot = static_cast<NiUIMemberSlot*>(NiNew(0x20));
    if (slot != nullptr) {
        slot->set.base = nullptr;
        slot->set.alloced = 0;
        slot->set.used = 0;
        slot->vtable = reinterpret_cast<void*>(kNiUIMemberSlot1UcharVtable);
        slot->instance = self;
        slot->function = reinterpret_cast<void*>(entry.function);
        slot->adjust = 0;
    }
    PrimitiveArray_SetAt(&self->controlSlots, entry.index, re::PtrCast<const re::Ptr32<void>*>(&slot));
    if (entry.slotFirst) {
        ControlInputArray_SetAt(&self->inputs, entry.index, &input);
    }
}

// every control's amount cleared and the control turned on, the zoom factor 1
void ResetControls(NiNavBaseController* self) {
    float* amounts = &At<float>(self, 0x80);
    for (uint32_t i = 0; i < 6; i++) {
        amounts[i] = 0.0f;
        if (i >= self->controlActive.size) {
            self->controlActive.size = uint16_t(i + 1);
            self->controlActive.esize++;
        } else if (self->controlActive.base[i] == 0) {
            self->controlActive.esize++;
        }
        self->controlActive.base[i] = 1;
    }
    amounts[0] = 1.0f;
}

// the constructor (reference, controlled, name, scale, up, invert pitch, invert yaw): W/S
// forward (the left stick), A/D sideways, R/F up and down, the right stick turning, +/- (the
// wheel, the shoulder buttons) the motion scale
NiNavFlyController* __fastcall Fly_Construct(NiNavFlyController* self, re::Edx, NiAVObject* reference,
                                             NiAVObject* controlled, const char* name, float scale, NiPoint3 upDir,
                                             int invertPitch, int invertYaw) {
    NavController_Construct(self, name);
    self->upDir = upDir;
    self->vtable = reinterpret_cast<void*>(kNiNavFlyControllerVtable);
    self->lookAt = NiPoint3_ZERO;
    self->reference = reference;
    AddRef(reference);
    self->controlled = controlled;
    AddRef(controlled);
    self->scale = scale;
    PrimitiveArray_SetSize(&self->controlSlots, 0xC);
    ControlInputArray_SetSize(&self->inputs, 0xC);
    BoolArray_SetSize(&self->controlActive, 6);
    static const ControlEntry kEntries[] = {
        {11, 9, 0x11, 0, -1, 1, 0x00833D60, false},    // W / left stick: forward
        {10, 9, 0x1F, 0, -1, 1, 0x00833DA0, false},    // S: back
        {7, 9, 0x20, 0, -1, 0, 0x00833DE0, false},     // D: right
        {6, 9, 0x1E, 0, -1, 0, 0x00833E20, false},     // A: left
        {9, 1, 0x13, 0, -1, 0, 0x00833E60, false},     // R: up
        {8, 1, 0x21, 0, -1, 0, 0x00833EA0, false},     // F: down
        {3, 10, 0, 0, -1, 2, 0x00833EE0, false},       // mouse x / right stick: yaw
        {2, 10, 0, 0, -1, 2, 0x00833F20, false},
        {5, 10, 0, 1, -1, 3, 0x00833F60, false},       // mouse y / right stick: pitch
        {4, 10, 0, 1, -1, 3, 0x00833FA0, false},
        {1, 0x13, 0xD, 2, 6, 0, 0x00833FE0, false},    // =, the wheel, a shoulder: motion scale
        {0, 0x13, 0xC, 2, 4, 0, 0x00834020, true},     // -
    };
    NiNavControlInput input;
    for (const ControlEntry& entry : kEntries) {
        AddControl(self, input, entry);
    }
    ResetControls(self);
    if (self->scale <= 0.0f) {
        self->scale = 0.25f;
    }
    self->pitchSign = uint8_t(invertPitch) != 0 ? -1.0f : 1.0f;
    self->yawSign = uint8_t(invertYaw) != 0 ? -1.0f : 1.0f;
    return self;
}

// ---- the "Orbiting" controller

void __fastcall Orbit_Destruct(NiNavOrbitController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiNavOrbitControllerVtable);
    ClearRef(self->controlled);
    Release(self->controlled);
    NavController_Destruct(self);
}

const char* __fastcall Orbit_GetControlName(NiNavOrbitController*, re::Edx, uint32_t control) {
    return kOrbitControlNames[control];
}

// MouseInput(control, value): the amounts (the radius zooming out on up, the horizontal orbit
// against the mouse)
void __fastcall Orbit_MouseInput(NiNavOrbitController* self, re::Edx, uint32_t control, float value) {
    if (self->controlActive.base[control] == 0 || self->active == 0 || control > 5) {
        return;
    }
    switch (control) {
    case 0:
        ZoomFromInput(self, *reinterpret_cast<const double*>(kOrbitZoomBase), value, false);
        break;
    case 1:
        self->orbitHorizontal = self->orbitHorizontal - FrameTime() * (value + value);
        break;
    case 2:
        self->orbitVertical = FrameTime() * (value + value) + self->orbitVertical;
        break;
    case 3:
        self->panHorizontal = value * 1.5f * self->scale * FrameTime() + self->panHorizontal;
        break;
    case 4:
        self->panVertical = value * 1.5f * self->scale * FrameTime() + self->panVertical;
        break;
    case 5:
        self->flyForward = value * 1.5f * self->scale * FrameTime() + self->flyForward;
        break;
    }
}

// GamePadInput(control, value): the stick's amounts (the zoom the other way, vertical moves
// inverted)
void __fastcall Orbit_GamePadInput(NiNavOrbitController* self, re::Edx, uint32_t control, float value) {
    if (self->controlActive.base[control] == 0 || self->active == 0 || control > 5) {
        return;
    }
    switch (control) {
    case 0:
        ZoomFromInput(self, *reinterpret_cast<const double*>(kOrbitZoomBase), value, true);
        break;
    case 1:
        self->orbitHorizontal = FrameTime() * (value + value) + self->orbitHorizontal;
        break;
    case 2:
        self->orbitVertical = self->orbitVertical - FrameTime() * (value + value);
        break;
    case 3:
        self->panHorizontal = value * 1.5f * self->scale * FrameTime() + self->panHorizontal;
        break;
    case 4:
        self->panVertical = self->panVertical - value * 1.5f * self->scale * FrameTime();
        break;
    case 5:
        self->flyForward = self->flyForward - value * 1.5f * self->scale * FrameTime();
        break;
    }
}

// UpdatePose: the centre panned along the object's axes, the object turned about it (the
// vertical orbit about its right axis, the horizontal about the up direction or its own up),
// kept upright, put the radius behind the centre, the amounts reset
void __fastcall Orbit_UpdatePose(NiNavOrbitController* self, re::Edx) {
    NiMatrix3 rotation = self->controlled->world.rotate;
    NiPoint3 forward = Column(rotation, 0);
    NiPoint3 up = Column(rotation, 1);
    NiPoint3 right = Column(rotation, 2);
    NiTransform transform = self->controlled->world;
    float scale = self->scale * self->radiusScale;
    if (!(scale > kOrbitMinScale)) {
        scale = kOrbitMinScale;
    }
    self->scale = scale;
    NiPoint3* center = &self->center;
    PointAlongAxis(self, &right, self->panHorizontal, center, center);
    PointAlongAxis(self, &up, self->panVertical, center, center);
    PointAlongAxis(self, &forward, self->flyForward, center, center);

    NiMatrix3 original = transform.rotate;
    NiMatrix3 vertical;
    Matrix3_MakeRotation(&vertical, self->orbitVertical, right.x, right.y, right.z);
    NiMatrix3 horizontal;
    const NiPoint3& upDir = self->upDir;
    if (!IsZero(upDir)) {
        Matrix3_MakeRotation(&horizontal, -self->orbitHorizontal, upDir.x, upDir.y, upDir.z);
    } else {
        Matrix3_MakeRotation(&horizontal, -self->orbitHorizontal, up.x, up.y, up.z);
    }
    NiMatrix3 turned;
    NiMatrix3 result;
    Matrix3_Multiply(Matrix3_Multiply(&horizontal, &turned, &vertical), &result, &original);
    Matrix3_Reorthogonalize(&result);
    if (!IsZero(upDir)) {
        float dot = (upDir.y * result.m[1][1] + result.m[0][1] * upDir.x) + upDir.z * result.m[2][1];
        if (dot <= 0.0f) {
            NiMatrix3 horizontalOnly;
            result = *Matrix3_Multiply(&horizontal, &horizontalOnly, &original);
            Matrix3_Reorthogonalize(&result);
        }
        NiPoint3 direction = Column(result, 0);
        NiPoint3 side;
        Point3_UnitCross(&direction, &side, &upDir);
        NiPoint3 newUp;
        Point3_UnitCross(&side, &newUp, &direction);
        NiMatrix3 upright;
        result = *Matrix3_SetCols(&upright, &direction, &newUp, &side);
    }
    float back = -self->scale;
    NiPoint3 offset{back * NiPoint3_UNIT_X.x, back * NiPoint3_UNIT_X.y, back * NiPoint3_UNIT_X.z};
    NiPoint3 rotated;
    Matrix3_TimesPoint(&result, &rotated, &offset);
    transform.translate = NiPoint3{center->x + rotated.x, rotated.y + center->y, rotated.z + center->z};
    transform.rotate = result;
    AVObject_SetWorldTransform(self->controlled, &transform);
    self->radiusScale = 0.0f;
    self->orbitHorizontal = 0.0f;
    self->orbitVertical = 0.0f;
    self->panHorizontal = 0.0f;
    self->panVertical = 0.0f;
    self->flyForward = 0.0f;
    self->radiusScale = 1.0f;
}

// SetActive(active, state): the base's; with no object, false. Activating takes the scale
// handed over (unless kept) and puts the centre that far in front of the object
uint8_t __fastcall Orbit_SetActive(NiNavOrbitController* self, re::Edx, int active, NiNavState* state) {
    uint8_t result = NavController_SetActive(self, active, state);
    NiAVObject* controlled = self->controlled;
    if (controlled == nullptr) {
        return 0;
    }
    if (uint8_t(active) != 0) {
        if (state->keepScale == 0) {
            self->scale = state->scale;
        }
        self->center = PointInFront(controlled, self->scale);
    } else {
        state->scale = self->scale;
        state->object = self->controlled;
    }
    return result;
}

NiNavOrbitController* __fastcall Orbit_DeletingDestructor(NiNavOrbitController* self, re::Edx, int flags) {
    OrbitController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xB8);
    }
    return self;
}

// NiUIMemberSlot1<T, unsigned char>'s deleting destructor
NiUIMemberSlot* __fastcall MemberSlot1_DeletingDestructor(NiUIMemberSlot* self, re::Edx, int flags) {
    self->instance = nullptr;
    self->adjust = 0;
    self->vtable = reinterpret_cast<void*>(kNiUIMemberSlot1UcharVtable);
    self->function = nullptr;
    BaseSlot1_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x20);
    }
    return self;
}

// the constructor (controlled, name, scale, up): the same keys, the mouse orbiting and the
// right stick, the up direction unitized (when not zero), a scale not above 0 made 0.25
NiNavOrbitController* __fastcall Orbit_Construct(NiNavOrbitController* self, re::Edx, NiAVObject* controlled,
                                                 const char* name, float scale, NiPoint3 upDir) {
    NavController_Construct(self, name);
    self->vtable = reinterpret_cast<void*>(kNiNavOrbitControllerVtable);
    self->controlled = controlled;
    AddRef(controlled);
    ControlInputArray_SetSize(&self->inputs, 0xC);
    BoolArray_SetSize(&self->controlActive, 6);
    PrimitiveArray_SetSize(&self->controlSlots, 0xC);
    static const ControlEntry kEntries[] = {
        {1, 0x13, 0xD, 2, 6, 0, 0x00835500, false},    // =: radius
        {0, 0x13, 0xC, 2, 4, 0, 0x00835540, false},    // -
        {3, 10, 0, 0, -1, 2, 0x00835580, false},       // mouse x / right stick: orbit horizontally
        {2, 10, 0, 0, -1, 2, 0x008355B0, false},
        {5, 10, 0, 1, -1, 3, 0x008355F0, false},       // mouse y / right stick: orbit vertically
        {4, 10, 0, 1, -1, 3, 0x00835620, false},
        {7, 9, 0x20, 0, -1, 0, 0x00835660, false},     // D: pan
        {6, 9, 0x1E, 0, -1, 0, 0x008356A0, false},     // A
        {9, 1, 0x13, 0, -1, 0, 0x008356E0, false},     // R: pan up
        {8, 1, 0x21, 0, -1, 0, 0x00835720, false},     // F
        {11, 9, 0x11, 0, -1, 1, 0x00835760, false},    // W / left stick: forward
        {10, 9, 0x1F, 0, -1, 1, 0x008357A0, false},    // S
    };
    NiNavControlInput input;
    for (const ControlEntry& entry : kEntries) {
        AddControl(self, input, entry);
    }
    ResetControls(self);
    self->center = NiPoint3_ZERO;
    NiPoint3& up = self->upDir;
    up = upDir;
    float lengthSquared = (up.y * up.y + up.x * up.x) + up.z * up.z;
    float length = float(crt::Sqrt(lengthSquared));
    if (length != 0.0f) {
        Point3_Unitize(&up);
    }
    self->scale = scale <= 0.0f ? 0.25f : scale;
    return self;
}

// ---- NiNavBaseController

// the mouse slot's function: the mouse button down / up
void __fastcall MouseButtonSlot(NiNavBaseController* self, re::Edx, uint32_t state) {
    uint8_t bits = uint8_t(state);
    if ((bits & 2) != 0) {
        self->mouseDown = 1;
    } else if ((bits & 4) != 0) {
        self->mouseDown = 0;
    }
}

// Update: each active control's analogue inputs: the mouse's movement (the wheel, or while the
// button's down) over the frame time, curved by pow and scaled; each game pad's stick within
// its range, curved and scaled (when past 0.0001); then UpdatePose
void __fastcall Update(NiNavBaseController* self, re::Edx) {
    if (self->active == 0 || UIManager_Get() == nullptr) {
        return;
    }
    uint32_t count = uint32_t(self->inputs.maxSize >> 1);
    int32_t mouseDelta = 0;   // (kept from one control to the next: the game's)
    for (uint32_t control = 0; control < count; control++) {
        if (self->controlActive.base[control] == 0) {
            continue;
        }
        const NiNavControlInput& input = self->inputs.base[control * 2 + 1];
        uint8_t devices = input.devices;
        uint32_t mouseAxis = input.mouseAxis;
        uint32_t padAxis = input.gamePadAxis;
        if (UIManager_Get()->hasMouse != 0 && (devices & 2) != 0) {
            uint8_t* mouse =
                UIManager_Get()->hasMouse != 0 ? At<uint8_t*>(UIManager_Get()->inputSystem, 0x44) : nullptr;
            bool read = true;
            if (!(mouseAxis == 2 && self->mouseReserved != 0)) {
                if (self->mouseDown == 0) {
                    goto gamePad;
                }
                read = mouseAxis < 3;
            }
            if (read) {
                mouseDelta = At<int32_t>(mouse, int(mouseAxis) * 0x20 + 0x24);
            }
            float frameTime = FrameTime();
            if (mouseDelta != 0 && frameTime > 0.0f) {
                // (the delta's exact in double: one rounding of the product, as the x87's)
                float speed = float(double(self->mouseSensitivity) * double(mouseDelta)) / frameTime;
                uint8_t negative = speed < 0.0f ? 1 : 0;
                float magnitude = NiSelect(negative, -speed, speed);
                float curved = float(crt::Pow(double(magnitude), double(self->mouseExponent)));
                float amount = (curved + magnitude) * self->mouseScale;
                amount = NiSelect(negative, -amount, amount);
                Virtual<void>(self, 6, control, amount);   // MouseInput
            }
        }
    gamePad:
        if (UIManager_Get()->hasGamePad != 0 && (devices & 8) != 0 && self->axisReserved[padAxis] != 0) {
            for (uint32_t pad = 0; pad < 8; pad++) {
                NiUIManager* manager = UIManager_Get();
                void* gamePad = At<void*>(manager->inputSystem, 0x48 + int(pad) * 4);
                if (gamePad == nullptr) {
                    continue;
                }
                int32_t value = GamePad_GetAxisValue(gamePad, padAxis);
                int32_t low = 0;
                int32_t high = 0;
                if (At<uint32_t>(gamePad, 0x34) != 0) {
                    uint8_t* axes = At<uint8_t*>(gamePad, 0x38);
                    low = At<int32_t>(axes, 0x18);
                    high = At<int32_t>(axes, 0x1C);
                }
                // (small integers: exact as floats, one rounding of the quotient)
                float unit = float(value - low) / float(high - low);
                float centered = unit - 0.5f;
                centered = centered + centered;
                bool negative = centered < 0.0f;
                float curved = float(crt::Pow(double(std::fabs(centered)), double(self->stickExponent)));
                float amount = curved * self->stickScale;
                if (negative) {
                    amount = -amount;
                }
                if (std::fabs(amount) > 0.0001) {
                    Virtual<void>(self, 7, control, amount);   // GamePadInput
                }
            }
        }
    }
    Virtual<void>(self, 5);   // UpdatePose
}

// SetActive(active, state): the inputs of each active control reserved with the UI manager
// (stopping at the first failure) or freed; whether all went through (no UI manager: false)
uint8_t __fastcall SetActive(NiNavBaseController* self, re::Edx, int active, NiNavState*) {
    NiUIManager* manager = UIManager_Get();
    if (manager == nullptr) {
        return 0;
    }
    bool activate = uint8_t(active) != 0;
    self->active = uint8_t(active);
    uint8_t hasGamePad = manager->hasGamePad;
    uint8_t hasKeyboard = manager->hasKeyboard;
    uint8_t hasMouse = manager->hasMouse;
    uint8_t mouseResult = 0;
    uint8_t ok = 1;
    uint32_t count = uint32_t(self->inputs.maxSize >> 1);
    for (uint32_t control = 0; control < count; control++) {
        if (self->controlActive.base[control] == 0) {
            continue;
        }
        const NiNavControlInput second = self->inputs.base[control * 2 + 1];
        const NiNavControlInput first = self->inputs.base[control * 2];
        auto reserved = [&](uint8_t result) { ok = (ok != 0 && result != 0) ? 1 : 0; };
        if (hasKeyboard != 0) {
            if ((second.devices & 1) != 0) {
                if (activate) {
                    reserved(ok != 0 ? UIManager_ReserveKeyboardButton(manager, second.key,
                                                                       self->controlSlots.base[control * 2 + 1], 7,
                                                                       second.keyModifiers)
                                     : 0);
                } else {
                    UIManager_UnreserveKeyboardButton(manager, second.key, self->controlSlots.base[control * 2 + 1],
                                                      second.keyModifiers);
                }
            }
            if ((first.devices & 1) != 0) {
                if (activate) {
                    reserved(ok != 0 ? UIManager_ReserveKeyboardButton(manager, first.key,
                                                                       self->controlSlots.base[control * 2], 7,
                                                                       first.keyModifiers)
                                     : 0);
                } else {
                    UIManager_UnreserveKeyboardButton(manager, first.key, self->controlSlots.base[control * 2],
                                                      first.keyModifiers);
                }
            }
        }
        if (hasMouse != 0) {
            if ((second.devices & 2) != 0) {
                // the mouse's movement: the whole mouse reserved (once), the wheel's flag set
                uint8_t value = mouseResult;
                if (value == 0) {
                    if (activate) {
                        value = UIManager_ReserveMouse(manager, &self->mouseSlot);
                        ok = (ok != 0 && value != 0) ? 1 : 0;
                    } else {
                        UIManager_UnreserveMouse(manager, &self->mouseSlot);
                        value = 1;   // (deactivating leaves the wheel's flag set: the game's)
                    }
                    mouseResult = value;
                }
                if (second.mouseAxis == 2) {
                    self->mouseReserved = value;
                }
            } else {
                if ((second.devices & 4) != 0) {
                    if (activate) {
                        reserved(ok != 0 ? UIManager_ReserveMouseButton(manager, uint32_t(second.mouseButton),
                                                                        self->controlSlots.base[control * 2 + 1], 7,
                                                                        second.mouseModifiers)
                                         : 0);
                    } else {
                        UIManager_UnreserveMouseButton(manager, uint32_t(second.mouseButton),
                                                       self->controlSlots.base[control * 2 + 1],
                                                       second.mouseModifiers);
                    }
                }
                if ((first.devices & 4) != 0) {
                    if (activate) {
                        reserved(ok != 0 ? UIManager_ReserveMouseButton(manager, uint32_t(first.mouseButton),
                                                                        self->controlSlots.base[control * 2], 7,
                                                                        first.mouseModifiers)
                                         : 0);
                    } else {
                        UIManager_UnreserveMouseButton(manager, uint32_t(first.mouseButton),
                                                       self->controlSlots.base[control * 2], first.mouseModifiers);
                    }
                }
            }
        }
        if (hasGamePad != 0) {
            if ((second.devices & 8) != 0) {
                if (activate) {
                    uint8_t result =
                        UIManager_ReserveGamePadAxis(manager, second.gamePadAxis, self->controlSlots.base[control * 2 + 1]);
                    self->axisReserved[second.gamePadAxis] = result;
                    reserved(result);
                } else {
                    UIManager_UnreserveGamePadAxis(manager, second.gamePadAxis,
                                                   self->controlSlots.base[control * 2 + 1]);
                }
                continue;
            }
            if ((second.devices & 0x10) != 0) {
                if (activate) {
                    reserved(ok != 0 ? UIManager_ReserveGamePadButton(manager, uint32_t(second.gamePadButton),
                                                                      self->controlSlots.base[control * 2 + 1], 7,
                                                                      second.gamePadModifiers)
                                     : 0);
                } else {
                    UIManager_UnreserveGamePadButton(manager, uint32_t(second.gamePadButton),
                                                     self->controlSlots.base[control * 2 + 1],
                                                     second.gamePadModifiers);
                }
            }
            if ((first.devices & 0x10) != 0) {
                if (activate) {
                    reserved(ok != 0 ? UIManager_ReserveGamePadButton(manager, uint32_t(first.gamePadButton),
                                                                      self->controlSlots.base[control * 2], 7,
                                                                      first.gamePadModifiers)
                                     : 0);
                } else {
                    UIManager_UnreserveGamePadButton(manager, uint32_t(first.gamePadButton),
                                                     self->controlSlots.base[control * 2], first.gamePadModifiers);
                }
            }
        }
    }
    return ok;
}

// the constructor (name): the mouse slot on the mouse-button function, the three arrays empty,
// the name (none: an empty one), the curves' constants
NiNavBaseController* __fastcall Construct(NiNavBaseController* self, re::Edx, const char* name) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiNavBaseControllerVtable);
    self->mouseSlot.set.base = nullptr;
    self->mouseSlot.set.alloced = 0;
    self->mouseSlot.set.used = 0;
    self->mouseSlot.vtable = reinterpret_cast<void*>(kNiUIMemberSlot1UcharVtable);
    self->mouseSlot.instance = nullptr;
    self->mouseSlot.function = nullptr;
    self->mouseSlot.adjust = 0;
    auto initArray = [](auto& array) {
        array.maxSize = 0;
        array.size = 0;
        array.esize = 0;
        array.base = nullptr;
        array.vtable = reinterpret_cast<void*>(kNiTPrimitiveArrayVtable);
        array.growBy = 1;
    };
    initArray(self->controlSlots);
    initArray(self->inputs);
    initArray(self->controlActive);
    NiString_ConstructSized(&self->name, 0x100);
    self->mouseSensitivity = *reinterpret_cast<const float*>(0x009893D0);   // 0.0008
    self->active = 0;
    self->mouseExponent = 2.0f;
    self->mouseDown = 0;
    self->mouseReserved = 0;
    self->mouseScale = 0.5f;
    self->stickExponent = 2.0f;
    self->stickScale = 1.0f;
    self->mouseSlot.function = reinterpret_cast<void*>(0x00838300);
    self->mouseSlot.adjust = 0;
    self->mouseSlot.instance = self;
    for (uint8_t& reservedAxis : self->axisReserved) {
        reservedAxis = 0;
    }
    if (name != nullptr) {
        NiString_AssignText(&self->name, name);
    } else {
        NiString_AssignChar(&self->name, 0);
    }
    return self;
}

// the destructor: deactivated (when active), the control slots deleted, the arrays freed
void __fastcall Destruct(NiNavBaseController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiNavBaseControllerVtable);
    if (self->active != 0) {
        NiNavState state;
        NavController_SetActive(self, 0, &state);
    }
    uint32_t count = self->controlSlots.size;
    for (uint32_t i = 0; i < count; i++) {
        NiUIMemberSlot* slot = self->controlSlots.base[i];
        if (slot != nullptr) {
            Virtual<void>(slot, 0, 1);
        }
    }
    NiString_Destruct(&self->name);
    self->controlActive.vtable = reinterpret_cast<void*>(kNiTPrimitiveArrayVtable);
    NiFree(self->controlActive.base);
    self->inputs.vtable = reinterpret_cast<void*>(kNiTPrimitiveArrayVtable);
    NiFree(self->inputs.base);
    self->controlSlots.vtable = reinterpret_cast<void*>(kNiTPrimitiveArrayVtable);
    NiFree(self->controlSlots.base);
    self->mouseSlot.vtable = reinterpret_cast<void*>(kNiUIMemberSlot1UcharVtable);
    self->mouseSlot.instance = nullptr;
    self->mouseSlot.function = nullptr;
    self->mouseSlot.adjust = 0;
    BaseSlot1_Destruct(&self->mouseSlot);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
}

NiNavBaseController* __fastcall DeletingDestructor(NiNavBaseController* self, re::Edx, int flags) {
    NavController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x80);
    }
    return self;
}

// NiTArray<ControlInput>::SetSize(size): the elements past it cleared (the used count kept), a
// new array of that size with the rest empty
void __fastcall ControlInputs_SetSize(NiTArray<NiNavControlInput>* self, re::Edx, uint32_t size) {
    if (size == self->maxSize) {
        return;
    }
    const NiNavControlInput empty = EmptyInput();
    uint16_t newSize = uint16_t(size);
    if (size < self->size) {
        for (uint16_t i = newSize; i < self->size; i++) {
            NiNavControlInput& element = self->base[i];
            if (element.devices != 0 || element.key != 0 || element.keyModifiers != 0 || element.mouseButton != -1 ||
                element.mouseModifiers != 0 || element.mouseAxis != 0 || element.gamePadButton != -1 ||
                element.gamePadModifiers != 0 || element.gamePadAxis != 0) {
                element = empty;
                self->esize--;
            }
        }
        self->size = newSize;
    }
    NiNavControlInput* old = self->base;
    self->maxSize = newSize;
    if (size == 0) {
        self->base = nullptr;
    } else {
        self->base = static_cast<NiNavControlInput*>(NiMalloc(uint32_t(newSize) * sizeof(NiNavControlInput)));
        for (uint16_t i = 0; i < self->size; i++) {
            self->base[i] = old[i];
        }
        for (uint16_t i = self->size; i < self->maxSize; i++) {
            self->base[i] = empty;
        }
    }
    NiFree(old);
}

// NiTArray<ControlInput>::SetAt(index, input): the used count kept against the empty input
void __fastcall ControlInputs_SetAt(NiTArray<NiNavControlInput>* self, re::Edx, uint32_t index,
                                    const NiNavControlInput* input) {
    NiNavControlInput empty = EmptyInput();
    if (index >= self->size) {
        self->size = uint16_t(index + 1);
        if (!ControlInput_Equals(input, &empty)) {
            self->esize++;
        }
    } else if (ControlInput_Equals(input, &empty)) {
        if (!ControlInput_Equals(&self->base[index], &empty)) {
            self->esize--;
        }
    } else if (ControlInput_Equals(&self->base[index], &empty)) {
        self->esize++;
    }
    self->base[index] = *input;
}

// NiTArray<bool>::SetSize(size)
void __fastcall Bools_SetSize(NiTArray<uint8_t>* self, re::Edx, uint32_t size) {
    if (size == self->maxSize) {
        return;
    }
    uint16_t newSize = uint16_t(size);
    if (size < self->size) {
        for (uint16_t i = newSize; i < self->size; i++) {
            if (self->base[i] != 0) {
                self->base[i] = 0;
                self->esize--;
            }
        }
        self->size = newSize;
    }
    uint8_t* old = self->base;
    self->maxSize = newSize;
    if (size == 0) {
        self->base = nullptr;
    } else {
        self->base = static_cast<uint8_t*>(NiMalloc(newSize));
        for (uint16_t i = 0; i < self->size; i++) {
            self->base[i] = old[i];
        }
        for (uint16_t i = self->size; i < self->maxSize; i++) {
            self->base[i] = 0;
        }
    }
    NiFree(old);
}

} // namespace

#define NINAV_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NINAV_REVERSED("NiNavManager::Update", 0x00831470, &Manager_Update);
NINAV_REVERSED("NiNavFlyController::SetReference", 0x008314B0, &Fly_SetReference);
NINAV_REVERSED("NiNavFlyController::UpdateLookAt", 0x00831530, &Fly_UpdateLookAt);
NINAV_REVERSED("NiNavOrbitController::SetControlled", 0x008315D0, &Orbit_SetControlled);
NINAV_REVERSED("NiNavFlyController::SetControlled", 0x00832010, &Fly_SetControlled);
NINAV_REVERSED("NiNavManager::GetCurrentController", 0x00833950, &Manager_GetCurrentController);
NINAV_REVERSED("NiNavManager::CycleController", 0x00833970, &Manager_CycleController);
NINAV_REVERSED("NiNavManager::NiNavManager", 0x008339D0, &Manager_Construct);
NINAV_REVERSED("NiNavManager::DeletingDestructor", 0x00833A30, &Manager_DeletingDestructor);
NINAV_REVERSED("NiNavManager::Create", 0x00833A80, &Manager_Create);
NINAV_REVERSED("NiNavManager::AddNavigationController", 0x00833AC0, &Manager_AddNavigationController);
NINAV_REVERSED("NiAVObject::SetWorldTransform", 0x00833CE0, &SetWorldTransform);
NINAV_REVERSED("NiNavFlyController::FlyForward", 0x00833D60, (&ControlSlot<5, 0x94, 0xB8, false, true>));
NINAV_REVERSED("NiNavFlyController::FlyBackward", 0x00833DA0, (&ControlSlot<5, 0x94, 0xB8, false, false>));
NINAV_REVERSED("NiNavFlyController::FlyRight", 0x00833DE0, (&ControlSlot<3, 0x8C, 0xB8, false, true>));
NINAV_REVERSED("NiNavFlyController::FlyLeft", 0x00833E20, (&ControlSlot<3, 0x8C, 0xB8, false, false>));
NINAV_REVERSED("NiNavFlyController::FlyUp", 0x00833E60, (&ControlSlot<4, 0x90, 0xB8, false, true>));
NINAV_REVERSED("NiNavFlyController::FlyDown", 0x00833EA0, (&ControlSlot<4, 0x90, 0xB8, false, false>));
NINAV_REVERSED("NiNavFlyController::RotateRight", 0x00833EE0, (&ControlSlot<1, 0x84, 0xB8, false, true>));
NINAV_REVERSED("NiNavFlyController::RotateLeft", 0x00833F20, (&ControlSlot<1, 0x84, 0xB8, false, false>));
NINAV_REVERSED("NiNavFlyController::RotateUp", 0x00833F60, (&ControlSlot<2, 0x88, 0xB8, false, true>));
NINAV_REVERSED("NiNavFlyController::RotateDown", 0x00833FA0, (&ControlSlot<2, 0x88, 0xB8, false, false>));
NINAV_REVERSED("NiNavFlyController::IncreaseMotionScale", 0x00833FE0, (&ZoomSlot<kFlyZoomBase, true>));
NINAV_REVERSED("NiNavFlyController::DecreaseMotionScale", 0x00834020, (&ZoomSlot<kFlyZoomBase, false>));
NINAV_REVERSED("NiNavFlyController::~NiNavFlyController", 0x00834060, &Fly_Destruct);
NINAV_REVERSED("NiNavFlyController::GetControlName", 0x00834110, &Fly_GetControlName);
NINAV_REVERSED("NiNavBaseController::IsControlActive", 0x00834120, &IsControlActive);
NINAV_REVERSED("NiNavFlyController::MouseInput", 0x00834130, &Fly_MouseInput);
NINAV_REVERSED("NiNavFlyController::GamePadInput", 0x00834290, &Fly_GamePadInput);
NINAV_REVERSED("NiNavFlyController::UpdatePose", 0x00834420, &Fly_UpdatePose);
NINAV_REVERSED("NiNavFlyController::SetActive", 0x00834980, &Fly_SetActive);
NINAV_REVERSED("NiTArray<NiNavBaseController::ControlInput>::SetSize", 0x008349E0, &ControlInputs_SetSize);
NINAV_REVERSED("NiTArray<NiNavBaseController::ControlInput>::SetAt", 0x00834B70, &ControlInputs_SetAt);
NINAV_REVERSED("NiTArray<bool>::SetSize", 0x00834CA0, &Bools_SetSize);
NINAV_REVERSED("NiNavFlyController::DeletingDestructor", 0x00834D70, &Fly_DeletingDestructor);
NINAV_REVERSED("NiNavFlyController::NiNavFlyController", 0x00834DA0, &Fly_Construct);
NINAV_REVERSED("NiNavOrbitController::IncreaseRadius", 0x00835500, (&ZoomSlot<kOrbitZoomBase, true>));
NINAV_REVERSED("NiNavOrbitController::DecreaseRadius", 0x00835540, (&ZoomSlot<kOrbitZoomBase, false>));
NINAV_REVERSED("NiNavOrbitController::OrbitRight", 0x00835580, (&ControlSlot<1, 0x84, 0xB4, true, true>));
NINAV_REVERSED("NiNavOrbitController::OrbitLeft", 0x008355B0, (&ControlSlot<1, 0x84, 0xB4, true, false>));
NINAV_REVERSED("NiNavOrbitController::OrbitUp", 0x008355F0, (&ControlSlot<2, 0x88, 0xB4, true, true>));
NINAV_REVERSED("NiNavOrbitController::OrbitDown", 0x00835620, (&ControlSlot<2, 0x88, 0xB4, true, false>));
NINAV_REVERSED("NiNavOrbitController::PanRight", 0x00835660, (&ControlSlot<3, 0x8C, 0xB4, false, true>));
NINAV_REVERSED("NiNavOrbitController::PanLeft", 0x008356A0, (&ControlSlot<3, 0x8C, 0xB4, false, false>));
NINAV_REVERSED("NiNavOrbitController::PanUp", 0x008356E0, (&ControlSlot<4, 0x90, 0xB4, false, true>));
NINAV_REVERSED("NiNavOrbitController::PanDown", 0x00835720, (&ControlSlot<4, 0x90, 0xB4, false, false>));
NINAV_REVERSED("NiNavOrbitController::FlyForward", 0x00835760, (&ControlSlot<5, 0x94, 0xB4, false, true>));
NINAV_REVERSED("NiNavOrbitController::FlyBackward", 0x008357A0, (&ControlSlot<5, 0x94, 0xB4, false, false>));
NINAV_REVERSED("NiNavOrbitController::~NiNavOrbitController", 0x008357E0, &Orbit_Destruct);
NINAV_REVERSED("NiNavOrbitController::GetControlName", 0x00835840, &Orbit_GetControlName);
NINAV_REVERSED("NiNavOrbitController::MouseInput", 0x00835850, &Orbit_MouseInput);
NINAV_REVERSED("NiNavOrbitController::GamePadInput", 0x008359F0, &Orbit_GamePadInput);
NINAV_REVERSED("NiNavOrbitController::UpdatePose", 0x00835B90, &Orbit_UpdatePose);
NINAV_REVERSED("NiNavOrbitController::SetActive", 0x00835FC0, &Orbit_SetActive);
NINAV_REVERSED("NiNavOrbitController::DeletingDestructor", 0x008360C0, &Orbit_DeletingDestructor);
NINAV_REVERSED("NiUIMemberSlot1<unsigned char>::DeletingDestructor", 0x008360F0, &MemberSlot1_DeletingDestructor);
NINAV_REVERSED("NiNavOrbitController::NiNavOrbitController", 0x00836130, &Orbit_Construct);
NINAV_REVERSED("NiNavBaseController::MouseButton", 0x00838300, &MouseButtonSlot);
NINAV_REVERSED("NiNavBaseController::Update", 0x00838360, &Update);
NINAV_REVERSED("NiNavBaseController::SetActive", 0x008389B0, &SetActive);
NINAV_REVERSED("NiNavBaseController::NiNavBaseController", 0x00838DC0, &Construct);
NINAV_REVERSED("NiNavBaseController::~NiNavBaseController", 0x00838ED0, &Destruct);
NINAV_REVERSED("NiNavBaseController::DeletingDestructor", 0x00838F90, &DeletingDestructor);

} // namespace ni::sample
