#pragma once

// Gamebryo's sample framework as Bully's exe has it (the NiSample application class Bully's
// main frame derives from, 0x830F80..0x83BBB0): the UI (NiUIManager and its render click, the
// signals and slots, NiUIBaseElement, NiUILabel, NiUIRenderGroup, NiUIGroup), the cursor's
// render click, the navigation controllers (NiNavBaseController, the "Flying" and "Orbiting"
// ones, NiNavManager) and the shader library and parser loading. Layouts read from the
// constructors (NiSample 0x832A20, NiUIManager 0x837B40, NiUIBaseElement 0x83A260, NiUILabel
// 0x832F70, NiUIRenderGroup 0x83B830, NiUIGroup 0x83B0F0, NiNavBaseController 0x838DC0, the fly
// controller 0x834DA0, the orbit controller 0x836130, NiNavManager 0x8339D0) and checked with
// static_asserts. Classes with an NiRTTI (src/game/StaticInit4.cpp) carry its name; the
// navigation ones and NiUIManager have none and are named after Gamebryo 2.x's sample framework
// (NiNavFlyController, NiNavOrbitController...) and their strings: a guess.
//
// NiApplication (the other half of NiSample, 0x825F20..0x830F20) is another pass's: its fields
// are reached through named offsets here (NiSample's own start at +0xFC).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::sample {

using re::At;

// a virtual call by slot (thiscall), as the library's code makes them
template <typename R = void, typename... A>
inline R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

// NiPointer's reference counting as the library inlines it (through the game's import slots)
inline void AddRef(void* object) {
    if (object != nullptr) {
        ni::sys::Increment(static_cast<uint8_t*>(object) + 4);
    }
}
inline void Release(void* object) {
    if (object != nullptr && ni::sys::Decrement(static_cast<uint8_t*>(object) + 4) == 0) {
        Virtual<void>(object, 1);   // DeleteThis
    }
}
// NiPointer::operator= inlined: the old one released first, then the new one held
template <typename T>
inline void AssignRef(re::Ptr32<T>& slot, std::type_identity_t<T>* value) {
    T* old = slot;
    if (old != value) {
        Release(old);
        slot = value;
        AddRef(value);
    }
}
// NiPointer's destructor / = nullptr, inlined
template <typename T>
inline void ClearRef(re::Ptr32<T>& slot) {
    if (slot != nullptr) {
        Release(slot);
        slot = nullptr;
    }
}

// the vtables and statics the constructors and destructors set
constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);

// NiTPrimitiveSet<T> (NiTSet's layout): the array, how many there's room for, how many used
template <typename T>
struct NiTSet {
    re::Ptr32<re::Field<T>> base;          // +0x0
    uint32_t alloced; // +0x4
    uint32_t used;    // +0x8
};
static_assert(sizeof(NiTSet<void*>) == 0xC);

// ---- signals and slots (NiUISignal / NiUISlot: each keeps the set of the other kind it's
// connected to; Subscribe adds both ways, the destructors take themselves off every other)

// NiUISignal0, NiUISignal1<T>, NiUIBaseSlot0, NiUIBaseSlot1<T> (0x10): the vtable, the set
struct NiUISignal {
    re::Ptr32<void> vtable;              // +0x0
    NiTSet<NiUISignal*> set;   // +0x4 (the slots of a signal, the signals of a slot)
};
static_assert(sizeof(NiUISignal) == 0x10);

// NiUIMemberSlot0<T> / NiUIMemberSlot1<T, A> (0x20): the instance and its member function
// (a pointer to member: the function and the this adjustment), ReceiveSignal calling it
struct NiUIMemberSlot : NiUISignal {
    re::Ptr32<void> instance;      // +0x10
    uint32_t unknown14;  // +0x14
    re::Ptr32<void> function;      // +0x18
    int32_t adjust;      // +0x1C
};
static_assert(sizeof(NiUIMemberSlot) == 0x20);

constexpr uint32_t kNiUISignal1UcharVtable = 0x00988FAC;     // NiUISignal1<unsigned char>
constexpr uint32_t kNiUISignalVtable = 0x00989430;           // NiUISignal0 / NiUISignal1<T*> (folded)
constexpr uint32_t kNiUIBaseSlot1BoolVtable = 0x00989498;    // NiUIBaseSlot1<bool>
constexpr uint32_t kNiUIMemberSlot1GroupVtable = 0x009894A0; // NiUIMemberSlot1<NiUIGroup, bool>
constexpr uint32_t kNiUIMemberSlot1UcharVtable = 0x009893AC; // NiUIMemberSlot1<T, unsigned char>
constexpr uint32_t kNiUIMemberSlot1SampleVtable = 0x008FF5A0;   // NiUIMemberSlot1<NiSample, ...>
constexpr uint32_t kNiUIMemberSlot0SampleVtable = 0x008FF38C;   // NiUIMemberSlot0<NiSample>

// ---- NiUIBaseElement (vtable 0x989438, 0x98, NiRTTI 0x20E5A74 "NiUIBaseElement"). Slots:
// 0 the deleting destructor, 1 DeleteThis, 2 GetRTTI, 3 Draw(renderer), 4 EnterPressed(state),
// 5 LeftMouseButton(state), 6 SetFocus(focus), 7 SetVisible(visible), 8 MouseLocation(x, y),
// 9 AttachResources(screen elements), 10 GetChildElementCount, 11 GetChildElement(i),
// 12 ReinitializeDisplayElements, 13 NumQuadsRequired, 14 IsInside(x, y), 15 UpdateRect,
// 16 ReinitializeChildren, 17 UpdateLayout, 18 UpdateChildLayout (names: Gamebryo 2.x's)
struct NiUIBaseElement : NiRefObject {
    float nscRect[4];             // +0x8 (left, right, top, bottom: normalized screen space)
    uint32_t pixelRect[4];        // +0x18 (left, right, top, bottom: pixels)
    float offset[2];              // +0x28 (from the parent's corner)
    float dimensions[2];          // +0x30
    re::Ptr32<NiUIBaseElement> parent;      // +0x38
    NiUISignal shown;             // +0x3C (NiUISignal0s: SetVisible's)
    NiUISignal hidden;            // +0x4C
    NiUISignal focusGained;       // +0x5C (SetFocus's, and the mouse coming over it)
    NiUISignal focusLost;         // +0x6C
    NiUISignal signal7C;          // +0x7C (the buttons')
    uint32_t baseQuadIndex;       // +0x8C (its first quad in the screen elements)
    re::Ptr32<NiRefObject> screenElements;  // +0x90 (a reference: NiScreenElements)
    uint8_t focused;              // +0x94
    uint8_t visible;              // +0x95
    uint8_t mouseInside;          // +0x96
    uint8_t canTakeFocus;         // +0x97
};
static_assert(offsetof(NiUIBaseElement, offset) == 0x28);
static_assert(offsetof(NiUIBaseElement, shown) == 0x3C);
static_assert(offsetof(NiUIBaseElement, baseQuadIndex) == 0x8C);
static_assert(offsetof(NiUIBaseElement, focused) == 0x94);
static_assert(sizeof(NiUIBaseElement) == 0x98);

constexpr uint32_t kNiUIBaseElementVtable = 0x00989438;

// NiUILabel (vtable 0x989218, 0xDC, NiRTTI 0x20E5A04): a text on a background quad
struct NiUILabel : NiUIBaseElement {
    uint32_t unknown98[4];        // +0x98
    float backgroundColor[4];     // +0xA8 (NiColorA)
    float textColor[4];           // +0xB8 (NiColorA; the alpha 1)
    re::Ptr32<char> text;                   // +0xC8 (NiString: as given)
    re::Ptr32<char> wrappedText;            // +0xCC (NiString: broken into lines that fit)
    re::Ptr32<NiRefObject> string2D;        // +0xD0 (a reference: NiString2D)
    uint32_t verticalAlign;       // +0xD4 (0 top, 1 bottom, 2 centre)
    uint32_t horizontalAlign;     // +0xD8 (0 left, 1 right, 2 centre)
};
static_assert(offsetof(NiUILabel, backgroundColor) == 0xA8);
static_assert(offsetof(NiUILabel, text) == 0xC8);
static_assert(sizeof(NiUILabel) == 0xDC);

constexpr uint32_t kNiUILabelVtable = 0x00989218;

// NiUIRenderGroup (vtable 0x989518, 0xA8, NiRTTI 0x20E5ADC): a group of elements drawn in one
// screen-elements object, in its render order
struct NiUIRenderGroup : NiUIBaseElement {
    NiTSet<NiUIBaseElement*> children;   // +0x98 (NiTObjectSet: references held)
    uint8_t renderOrder;                 // +0xA4
};
static_assert(offsetof(NiUIRenderGroup, children) == 0x98);
static_assert(sizeof(NiUIRenderGroup) == 0xA8);

constexpr uint32_t kNiUIRenderGroupVtable = 0x00989518;

// NiUIGroup (vtable 0x9894A8, 0x110, NiRTTI 0x20E5AA8): a render group with a title bar
// (a label and an expand button) and the keyboard / game pad focus moving through its children
struct NiUIGroup : NiUIRenderGroup {
    float headerHeight;           // +0xA8
    re::Ptr32<NiUILabel> titleLabel;        // +0xAC (a reference)
    uint8_t expanded;             // +0xB0
    float outerRect[4];           // +0xB4 (NiRect<float>)
    float innerRect[4];           // +0xC4
    re::Ptr32<NiUIBaseElement> expandButton;   // +0xD4 (a reference: NiUIImageButton, 0xC0; its state +0xA8)
    NiUIMemberSlot expandSlot;    // +0xD8 (NiUIMemberSlot1<NiUIGroup, bool>: SetExpanded)
    uint32_t mouseRect[4];        // +0xF8 (its own pixel rect, as UpdateRect last saw it)
    int32_t focusedChild;         // +0x108 (-1 the title, -2 none)
    uint32_t unknown10C;
};
static_assert(offsetof(NiUIGroup, titleLabel) == 0xAC);
static_assert(offsetof(NiUIGroup, expandButton) == 0xD4);
static_assert(offsetof(NiUIGroup, expandSlot) == 0xD8);
static_assert(offsetof(NiUIGroup, focusedChild) == 0x108);
static_assert(sizeof(NiUIGroup) == 0x110);

constexpr uint32_t kNiUIGroupVtable = 0x009894A8;

// ---- NiUIManager (vtable 0x9893C4, 0x6F0, no NiRTTI; the one at 0x20E5A3C)

// a reserved game pad button or axis (vtable 0x988FF0, 0x28)
struct NiUIGamePadBinding : NiRefObject {
    uint32_t button;          // +0x8 (-1: an axis)
    uint32_t modifiers;       // +0xC
    uint32_t axis;            // +0x10
    NiUISignal signal;        // +0x14 (NiUISignal1<unsigned char>)
    uint8_t stateMask;        // +0x24 (1 down, 2 pressed, 4 released)
    uint8_t isButton;         // +0x25
};
static_assert(offsetof(NiUIGamePadBinding, signal) == 0x14);
static_assert(sizeof(NiUIGamePadBinding) == 0x28);

// a reserved keyboard key or mouse button (vtable 0x988FF8, 0x24)
struct NiUIKeyBinding : NiRefObject {
    uint32_t key;             // +0x8
    uint32_t modifiers;       // +0xC
    NiUISignal signal;        // +0x10 (NiUISignal1<unsigned char>)
    uint8_t stateMask;        // +0x20
};
static_assert(offsetof(NiUIKeyBinding, signal) == 0x10);
static_assert(sizeof(NiUIKeyBinding) == 0x24);

constexpr uint32_t kNiUIGamePadBindingVtable = 0x00988FF0;
constexpr uint32_t kNiUIKeyBindingVtable = 0x00988FF8;

struct NiUIManager : NiRefObject {
    NiUIMemberSlot selectNextGroup;     // +0x8 (NiUIMemberSlot1<NiUIManager, unsigned char>)
    NiUIMemberSlot selectPrevGroup;     // +0x28
    NiUIMemberSlot enterPressed;        // +0x48
    NiUIMemberSlot leftMouseButton;     // +0x68
    NiUISignal mouseSignal;             // +0x88 (NiUISignal1<unsigned char>: the reserved mouse)
    int32_t focusedGroup;               // +0x98 (-1 none)
    uint8_t atlasMap[0x678 - 0x9C];     // +0x9C (NiUIAtlasMap, vtable 0x989C68: the skin's rects)
    re::Ptr32<NiRefObject> font;                  // +0x678 (a reference: NiFont)
    re::Ptr32<NiRefObject> inputSystem;           // +0x67C (a reference)
    re::Ptr32<NiRefObject> cursor;                // +0x680 (a reference: NiCursor)
    NiTSet<NiUIKeyBinding*> mouseBindings;          // +0x684 (NiTObjectSet)
    NiTSet<NiUIGamePadBinding*> gamePadBindings;    // +0x690
    NiTSet<NiUIKeyBinding*> keyboardBindings;       // +0x69C
    NiTArray<NiUIRenderGroup*> groups;  // +0x6A8 (NiTObjectArray, vtable 0x9893C0: focus order)
    NiTArray<NiUIRenderGroup*> renderGroups;   // +0x6B8 (vtable 0x988FD0: in render order)
    float safeRect[4];                  // +0x6C8 (the renderer's +0x20: NiRect<float>)
    float screenWidth;                  // +0x6D8 (pixels)
    float screenHeight;                 // +0x6DC
    float maxCharWidth;                 // +0x6E0 (normalized)
    float maxCharHeight;                // +0x6E4
    uint8_t hasMouse;                   // +0x6E8
    uint8_t hasKeyboard;                // +0x6E9
    uint8_t hasGamePad;                 // +0x6EA
    uint8_t mouseReserved;              // +0x6EB
    uint8_t groupFocusActive;           // +0x6EC
    uint8_t mouseOverGroup;             // +0x6ED
    uint8_t mouseOverNothing;           // +0x6EE
    uint8_t visible;                    // +0x6EF
};
static_assert(offsetof(NiUIManager, mouseSignal) == 0x88);
static_assert(offsetof(NiUIManager, focusedGroup) == 0x98);
static_assert(offsetof(NiUIManager, font) == 0x678);
static_assert(offsetof(NiUIManager, mouseBindings) == 0x684);
static_assert(offsetof(NiUIManager, groups) == 0x6A8);
static_assert(offsetof(NiUIManager, renderGroups) == 0x6B8);
static_assert(offsetof(NiUIManager, safeRect) == 0x6C8);
static_assert(offsetof(NiUIManager, maxCharWidth) == 0x6E0);
static_assert(offsetof(NiUIManager, hasMouse) == 0x6E8);
static_assert(sizeof(NiUIManager) == 0x6F0);

constexpr uint32_t kNiUIManagerVtable = 0x009893C4;

GAME_VAR(NiUIManager*, NiUIManager_ms_pkUIManager, 0x020E5A3C);

// ---- navigation (no NiRTTI)

// a control's input description (0x24; two per control, the positive and the negative way):
// the devices it's on (1 keyboard key, 2 mouse axis, 4 mouse button, 8 game pad axis, 0x10 game
// pad button), the key and its modifiers, the mouse button and its modifiers, the mouse axis,
// the game pad button and its modifiers, the game pad axis. Unused: the button fields -1
struct NiNavControlInput {
    uint8_t devices;          // +0x0
    uint32_t key;             // +0x4
    uint32_t keyModifiers;    // +0x8
    int32_t mouseButton;      // +0xC
    uint32_t mouseModifiers;  // +0x10
    uint32_t mouseAxis;       // +0x14 (2 the wheel)
    int32_t gamePadButton;    // +0x18
    uint32_t gamePadModifiers;   // +0x1C
    uint32_t gamePadAxis;     // +0x20
};
static_assert(sizeof(NiNavControlInput) == 0x24);

// NiNavBaseController (vtable 0x9893D4, 0x80): slots 0 the deleting destructor, 1 DeleteThis,
// 2 SetActive(active, state), 3 GetControlName(i), 4 IsControlActive(i), 5 UpdatePose,
// 6 MouseInput(i, value), 7 GamePadInput(i, value)
struct NiNavBaseController : NiRefObject {
    NiUIMemberSlot mouseSlot;                 // +0x8 (NiUIMemberSlot1<..., unsigned char>)
    NiTArray<NiUIMemberSlot*> controlSlots;   // +0x28 (vtable 0x961988: two per control)
    NiTArray<NiNavControlInput> inputs;       // +0x38 (two per control)
    NiTArray<uint8_t> controlActive;          // +0x48
    re::Ptr32<char> name;                               // +0x58 (NiString)
    uint8_t active;                           // +0x5C
    uint8_t axisReserved[8];                  // +0x5D (per game pad axis)
    uint8_t mouseDown;                        // +0x65
    uint8_t mouseReserved;                    // +0x66
    uint8_t pad67;
    float mouseSensitivity;                   // +0x68 (0.0008)
    float mouseExponent;                      // +0x6C (2)
    float mouseScale;                         // +0x70 (0.5)
    float stickExponent;                      // +0x74 (2)
    float stickScale;                         // +0x78 (1)
    float unknown7C;                          // +0x7C (not constructed)
};
static_assert(offsetof(NiNavBaseController, controlSlots) == 0x28);
static_assert(offsetof(NiNavBaseController, inputs) == 0x38);
static_assert(offsetof(NiNavBaseController, controlActive) == 0x48);
static_assert(offsetof(NiNavBaseController, name) == 0x58);
static_assert(offsetof(NiNavBaseController, axisReserved) == 0x5D);
static_assert(offsetof(NiNavBaseController, mouseSensitivity) == 0x68);
static_assert(sizeof(NiNavBaseController) == 0x80);

constexpr uint32_t kNiNavBaseControllerVtable = 0x009893D4;
constexpr uint32_t kNiTPrimitiveArrayVtable = 0x00961988;

// the state SetActive hands from one controller to the next
struct NiNavState {
    re::Ptr32<NiAVObject> object;   // +0x0
    float scale;          // +0x4
    uint8_t keepScale;    // +0x8 (set: the new one keeps its own)
};

// the "Flying" controller (vtable 0x9892FC, 0xC8): NiNavFlyController. Controls (0x834110's
// names): 0 Change Motion Scale, 1 Rotate Horizontally, 2 Rotate Vertically, 3 Fly
// Horizontally, 4 Fly Vertically, 5 Fly Forward; their per-frame amounts below
struct NiNavFlyController : NiNavBaseController {
    float motionScale;              // +0x80 (a factor: 1 none)
    float rotateHorizontal;         // +0x84
    float rotateVertical;           // +0x88
    float flyHorizontal;            // +0x8C
    float flyVertical;              // +0x90
    float flyForward;               // +0x94
    NiPoint3 upDir;                 // +0x98
    NiPoint3 lookAt;                // +0xA4
    re::Ptr32<NiAVObject> reference;          // +0xB0 (a reference: whose rotation the moves follow)
    re::Ptr32<NiAVObject> controlled;         // +0xB4 (a reference: the object moved)
    float scale;                    // +0xB8
    float pitchSign;                // +0xBC (-1 inverted)
    float yawSign;                  // +0xC0
    uint32_t unknownC4;
};
static_assert(offsetof(NiNavFlyController, motionScale) == 0x80);
static_assert(offsetof(NiNavFlyController, upDir) == 0x98);
static_assert(offsetof(NiNavFlyController, reference) == 0xB0);
static_assert(sizeof(NiNavFlyController) == 0xC8);

constexpr uint32_t kNiNavFlyControllerVtable = 0x009892FC;

// the "Orbiting" controller (vtable 0x98938C, 0xB8): NiNavOrbitController. Controls (0x835840's
// names): 0 Orbit Radius, 1 Orbit Horizontally, 2 Orbit Vertically, 3 Pan Horizontally, 4 Pan
// Vertically, 5 Fly Forward
struct NiNavOrbitController : NiNavBaseController {
    float radiusScale;              // +0x80 (a factor: 1 none)
    float orbitHorizontal;          // +0x84
    float orbitVertical;            // +0x88
    float panHorizontal;            // +0x8C
    float panVertical;              // +0x90
    float flyForward;               // +0x94
    NiPoint3 center;                // +0x98
    NiPoint3 upDir;                 // +0xA4
    re::Ptr32<NiAVObject> controlled;         // +0xB0 (a reference)
    float scale;                    // +0xB4 (the radius)
};
static_assert(offsetof(NiNavOrbitController, center) == 0x98);
static_assert(offsetof(NiNavOrbitController, controlled) == 0xB0);
static_assert(sizeof(NiNavOrbitController) == 0xB8);

constexpr uint32_t kNiNavOrbitControllerVtable = 0x0098938C;

// NiNavManager (vtable 0x989274, 0x34; the one at 0x20E5A38)
struct NiNavManager : NiRefObject {
    NiUISignal controllerChanged;             // +0x8 (NiUISignal1<NiNavBaseController*>)
    float lastTime;                           // +0x18
    float frameTime;                          // +0x1C
    NiTArray<NiNavBaseController*> controllers;   // +0x20 (NiTObjectArray, vtable 0x9893C0)
    uint32_t current;                         // +0x30
};
static_assert(offsetof(NiNavManager, controllers) == 0x20);
static_assert(sizeof(NiNavManager) == 0x34);

constexpr uint32_t kNiNavManagerVtable = 0x00989274;
constexpr uint32_t kNiTObjectArrayVtable = 0x009893C0;   // NiTObjectArray<NiPointer<T>> (folded)

GAME_VAR(NiNavManager*, NiNavManager_ms_pkNavManager, 0x020E5A38);

// ---- NiSample (vtable 0x9890F8, 0x230): NiApplication's 0xFC bytes, then these
struct NiSample {
    uint8_t application[0xFC];       // +0x0 (NiApplication's; the fields used below)
    re::Ptr32<NiNavOrbitController> orbit;     // +0xFC (a reference)
    re::Ptr32<NiNavFlyController> fly;         // +0x100 (a reference)
    uint32_t unknown104;
    NiUIMemberSlot toggleUISlot;     // +0x108 (NiUIMemberSlot1<NiSample, unsigned char>)
    NiUIMemberSlot toggleNavHelpSlot;   // +0x128 (NiUIMemberSlot0<NiSample>)
    NiUIMemberSlot slot148;          // +0x148 (NiUIMemberSlot0<NiSample>: vf58)
    NiUIMemberSlot slot168;          // +0x168 (NiUIMemberSlot0<NiSample>: 0x82A5E0)
    uint32_t logLineCount;           // +0x188 (3)
    float uiTitleHeight;             // +0x18C
    float uiLabelWidth;              // +0x190
    float uiHeaderHeight;            // +0x194 (not constructed)
    float uiNextX;                   // +0x198
    float uiNextY;                   // +0x19C
    float uiBorderWidth;             // +0x1A0
    float uiBorderHeight;            // +0x1A4
    re::Ptr32<NiUILabel> logLabel;             // +0x1A8 (a reference)
    NiTArray<re::Ptr32<char>*> logLines;       // +0x1AC (vtable 0x988FB0: NiString*)
    uint32_t logLength;              // +0x1BC
    int32_t navLogLine;              // +0x1C0 (-1 none)
    re::Ptr32<NiUILabel> frameRateLabel;       // +0x1C4 (a reference)
    uint8_t showNavHelp;             // +0x1C8
    uint8_t pad1C9[3];
    NiTArray<NiUIGroup*> navHelpGroups;   // +0x1CC (vtable 0x988FD0: one per controller)
    re::Ptr32<NiRefObject> cursor;             // +0x1DC (a reference: NiCursor)
    re::Ptr32<NiRefObject> cursorClick;        // +0x1E0 (a reference: NiCursorRenderClick)
    NiPoint3 navUpDir;               // +0x1E4 (0, 0, 1)
    float navScale;                  // +0x1F0 (300)
    NiTArray<void*> libraryCreators; // +0x1F4 (vtable 0x988FB4)
    NiTArray<void*> parserRunners;   // +0x204 (vtable 0x988FB8)
    uint32_t loadIndex;              // +0x214 (the one being loaded, for the callbacks)
    re::Ptr32<const char> uiStepName;        // +0x218
    re::Ptr32<const char> uiClickName;       // +0x21C
    re::Ptr32<const char> cursorStepName;    // +0x220
    re::Ptr32<const char> cursorClickName;   // +0x224
    uint8_t useNavigation;           // +0x228
    uint8_t pad229[7];
};
static_assert(offsetof(NiSample, toggleUISlot) == 0x108);
static_assert(offsetof(NiSample, logLineCount) == 0x188);
static_assert(offsetof(NiSample, logLabel) == 0x1A8);
static_assert(offsetof(NiSample, logLines) == 0x1AC);
static_assert(offsetof(NiSample, navHelpGroups) == 0x1CC);
static_assert(offsetof(NiSample, cursor) == 0x1DC);
static_assert(offsetof(NiSample, libraryCreators) == 0x1F4);
static_assert(offsetof(NiSample, loadIndex) == 0x214);
static_assert(offsetof(NiSample, useNavigation) == 0x228);
static_assert(sizeof(NiSample) == 0x230);

// NiApplication's fields NiSample's code reads
namespace app {
constexpr int kScene = 0x4;              // NiNode*
constexpr int kCamera = 0x8;             // NiCamera*
constexpr int kFrameTime = 0x1C;         // (NiNavManager's at the same place: not this)
constexpr int kRenderer = 0x24;          // NiRenderer*
constexpr int kInputSystem = 0x28;       // NiInputSystem*
constexpr int kAppWindow = 0x2C;         // NiAppWindow* (its HWND at +8)
constexpr int kAccumTime = 0x54;         // float
constexpr int kFrameCount = 0x5C;        // int
constexpr int kFrameRate = 0x60;         // float
constexpr int kUnknownA0 = 0xA0;         // byte (NiSample's constructor sets it)
constexpr int kVisible = 0xA4;           // NiVisibleArray
constexpr int kUseFrameSystem = 0xE1;    // byte
} // namespace app

GAME_VAR(NiSample*, NiApplication_ms_pkApplication, 0x020E5874);
// the message box function the library's errors go through (text, caption, flags)
using MessageBoxFn = void(__cdecl*)(const char* text, const char* caption, uint32_t flags);
GAME_VAR(MessageBoxFn, NiMessageBoxFunction, 0x00BC74FC);

// ---- this chunk's functions the others call (through the game's addresses)

GAME_FN(NiUIManager*, __cdecl, UIManager_Get, 0x008368E0);
GAME_FN(void, __thiscall, Signal1Uchar_Subscribe, 0x008322D0, NiUISignal* self, NiUISignal* other);
GAME_FN(void, __thiscall, Signal1Bool_Subscribe, 0x0083AF70, NiUISignal* self, NiUISignal* other);
GAME_FN(void, __thiscall, Signal0_Emit, 0x00833080, NiUISignal* self);
GAME_FN(void, __thiscall, Signal1_Emit, 0x00836F50, NiUISignal* self, uint32_t arg);
GAME_FN(void, __thiscall, Signal1Uchar_Destruct, 0x00831EF0, NiUISignal* self);
GAME_FN(void, __thiscall, Signal_Destruct, 0x0083A110, NiUISignal* self);
GAME_FN(void, __thiscall, BaseSlot1Bool_Destruct, 0x0083AE70, NiUISignal* self);
GAME_FN(void, __thiscall, BaseSlot1_Destruct, 0x00404290, NiUISignal* self);   // (game code's region)
GAME_FN(void, __thiscall, BaseSlot0_Unsubscribe, 0x00404130, NiUISignal* self, NiUISignal* other);
GAME_FN(uint8_t, __thiscall, Signal_IsSubscribed, 0x00838650, const NiUISignal* self, const NiUISignal* other);

GAME_FN(void, __thiscall, UIElement_SetOffset, 0x00839120, NiUIBaseElement* self, float x, float y);
GAME_FN(void, __thiscall, UIElement_SetDimensions, 0x008390F0, NiUIBaseElement* self, float w, float h);
GAME_FN(void, __thiscall, UIElement_SetParent, 0x008390E0, NiUIBaseElement* self, NiUIBaseElement* parent);
GAME_FN(float*, __thiscall, UIElement_GetOffset, 0x00839150, NiUIBaseElement* self, float* result);
GAME_FN(void, __thiscall, UIElement_Construct, 0x0083A260, NiUIBaseElement* self, int canTakeFocus);
GAME_FN(void, __thiscall, UIElement_Destruct, 0x0083A340, NiUIBaseElement* self);
GAME_FN(void, __thiscall, UILabel_Construct, 0x00832F70, NiUILabel* self, const char* text,
        const float* background, const float* textColor);
GAME_FN(void, __thiscall, UILabel_ReformatText, 0x00833550, NiUILabel* self);
GAME_FN(void, __thiscall, UILabel_UpdateTextPosition, 0x00833350, NiUILabel* self);
GAME_FN(void, __thiscall, UIRenderGroup_Construct, 0x0083B830, NiUIRenderGroup* self, int renderOrder);
GAME_FN(void, __thiscall, UIRenderGroup_Destruct, 0x0083B710, NiUIRenderGroup* self);
GAME_FN(void, __thiscall, UIRenderGroup_InitializeScreenElements, 0x0083B480, NiUIRenderGroup* self);
GAME_FN(void, __thiscall, UIGroup_Construct, 0x0083B0F0, NiUIGroup* self, const char* name, float headerHeight,
        int expanded);
GAME_FN(void, __thiscall, UIGroup_SetExpanded, 0x0083A4F0, NiUIGroup* self, int expanded);
GAME_FN(uint8_t, __thiscall, UIElement_SetFocus, 0x008331C0, NiUIBaseElement* self, int focus);
GAME_FN(void, __thiscall, UIElement_SetVisible, 0x00833120, NiUIBaseElement* self, int visible);

GAME_FN(NiUIManager*, __thiscall, UIManager_Construct, 0x00837B40, NiUIManager* self);
GAME_FN(void, __thiscall, UIManager_Destruct, 0x00837750, NiUIManager* self);
GAME_FN(void, __cdecl, UIManager_Create, 0x00837DE0);
GAME_FN(void, __cdecl, UIManager_Shutdown, 0x008368F0);
GAME_FN(uint8_t, __thiscall, UIManager_Initialize, 0x00837E20, NiUIManager* self, NiRefObject* inputSystem,
        const char* filename, NiRefObject* cursor);
GAME_FN(void, __thiscall, UIManager_UpdateUI, 0x008370F0, NiUIManager* self);
GAME_FN(void, __thiscall, UIManager_Draw, 0x008369E0, NiUIManager* self, void* renderer);
GAME_FN(void, __thiscall, UIManager_PrepareRenderGroups, 0x00836FE0, NiUIManager* self);
GAME_FN(void, __thiscall, UIManager_AddUIGroup, 0x00838210, NiUIManager* self, NiUIRenderGroup* group);
GAME_FN(void, __thiscall, UIManager_SortGroups, 0x00837540, NiUIManager* self);
GAME_FN(uint8_t, __thiscall, UIManager_ReserveGamePadButton, 0x008328B0, NiUIManager* self, uint32_t button,
        NiUISignal* slot, int state, uint32_t modifiers);
GAME_FN(uint8_t, __thiscall, UIManager_ReserveKeyboardButton, 0x00832970, NiUIManager* self, uint32_t key,
        NiUISignal* slot, int state, uint32_t modifiers);
GAME_FN(uint8_t, __thiscall, UIManager_ReserveMouseButton, 0x00837A90, NiUIManager* self, uint32_t button,
        NiUISignal* slot, int state, uint32_t modifiers);
GAME_FN(uint8_t, __thiscall, UIManager_ReserveGamePadAxis, 0x00838900, NiUIManager* self, uint32_t axis,
        NiUISignal* slot);
GAME_FN(uint8_t, __thiscall, UIManager_ReserveMouse, 0x008388B0, NiUIManager* self, NiUISignal* slot);
GAME_FN(void, __thiscall, UIManager_UnreserveMouse, 0x00838710, NiUIManager* self, NiUISignal* slot);
GAME_FN(void, __thiscall, UIManager_UnreserveMouseButton, 0x00838740, NiUIManager* self, uint32_t button,
        NiUISignal* slot, uint32_t modifiers);
GAME_FN(void, __thiscall, UIManager_UnreserveGamePadButton, 0x008387A0, NiUIManager* self, uint32_t button,
        NiUISignal* slot, uint32_t modifiers);
GAME_FN(void, __thiscall, UIManager_UnreserveGamePadAxis, 0x00838800, NiUIManager* self, uint32_t axis,
        NiUISignal* slot);
GAME_FN(void, __thiscall, UIManager_UnreserveKeyboardButton, 0x00838850, NiUIManager* self, uint32_t key,
        NiUISignal* slot, uint32_t modifiers);
GAME_FN(int, __thiscall, UIManager_FindMouseBinding, 0x008369A0, NiUIManager* self, uint32_t button,
        uint32_t modifiers);
GAME_FN(int, __thiscall, UIManager_FindGamePadAxisBinding, 0x00838320, NiUIManager* self, uint32_t axis);
// (the other pass's range, called where modes apply)
GAME_FN(int, __thiscall, UIManager_FindGamePadBinding, 0x00830E90, NiUIManager* self, uint32_t button,
        uint32_t modifiers);
GAME_FN(int, __thiscall, UIManager_FindKeyboardBinding, 0x00830EE0, NiUIManager* self, uint32_t key,
        uint32_t modifiers);
GAME_FN(NiUIRenderGroup*, __thiscall, UIManager_SetFocusedGroup, 0x00830F20, NiUIManager* self, uint32_t index);
GAME_FN(float*, __thiscall, UIManager_ConvertNSCToSafeArea, 0x00836BC0, NiUIManager* self, float* result,
        const float* rect);
GAME_FN(uint32_t*, __thiscall, UIManager_ConvertNSCToPixels, 0x00836D00, NiUIManager* self, uint32_t* result,
        const float* rect);
GAME_FN(float*, __thiscall, UIManager_ConvertPixelsToNSC, 0x00836DD0, NiUIManager* self, float* result,
        uint32_t x, uint32_t y);
GAME_FN(float*, __thiscall, UIManager_ConvertSizeToNSC, 0x00836EB0, NiUIManager* self, float* result,
        float width, float height);

GAME_FN(void, __thiscall, NavManager_Update, 0x00831470, NiNavManager* self);
GAME_FN(uint32_t, __thiscall, NavManager_AddNavigationController, 0x00833AC0, NiNavManager* self,
        NiNavBaseController* controller);
GAME_FN(NiNavBaseController*, __thiscall, NavManager_GetCurrentController, 0x00833950, NiNavManager* self);
GAME_FN(void, __thiscall, NavManager_CycleController, 0x00833970, NiNavManager* self);
GAME_FN(uint8_t, __cdecl, NavManager_Create, 0x00833A80);
GAME_FN(uint32_t, __thiscall, GetUint30, 0x007E8330, const void* self);   // (folded: returns +0x30)

GAME_FN(void, __thiscall, NavController_Construct, 0x00838DC0, NiNavBaseController* self, const char* name);
GAME_FN(void, __thiscall, NavController_Destruct, 0x00838ED0, NiNavBaseController* self);
GAME_FN(uint8_t, __thiscall, NavController_SetActive, 0x008389B0, NiNavBaseController* self, int active,
        NiNavState* state);
GAME_FN(void, __thiscall, NavController_Update, 0x00838360, NiNavBaseController* self);
GAME_FN(void, __thiscall, FlyController_UpdateLookAt, 0x00831530, NiNavFlyController* self);
GAME_FN(void, __thiscall, FlyController_SetControlled, 0x00832010, NiNavFlyController* self, NiAVObject* object);
GAME_FN(void, __thiscall, FlyController_SetReference, 0x008314B0, NiNavFlyController* self, NiAVObject* object);
GAME_FN(void, __thiscall, OrbitController_SetControlled, 0x008315D0, NiNavOrbitController* self,
        NiAVObject* object);
GAME_FN(void, __thiscall, FlyController_SetUpDir, 0x00830DB0, NiNavFlyController* self, const NiPoint3* dir);
GAME_FN(void, __thiscall, OrbitController_SetUpDir, 0x00830E20, NiNavOrbitController* self, const NiPoint3* dir);
// (the up direction is a NiPoint3 by value: its three floats on the stack)
GAME_FN(NiNavFlyController*, __thiscall, FlyController_Construct_x64, 0x00834DA0, NiNavFlyController* self, NiAVObject* reference, NiAVObject* controlled, const char* name, float scale, re::Float3, int invertPitch, int invertYaw);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline NiNavFlyController* FlyController_Construct(NiNavFlyController* a0, NiAVObject* a1, NiAVObject* a2, const char* a3, float a4, float a5, float a6, float a7, int a8, int a9) {
    return FlyController_Construct_x64(a0, a1, a2, a3, a4, re::Float3{a5, a6, a7}, a8, a9);
}
GAME_FN(NiNavOrbitController*, __thiscall, OrbitController_Construct_x64, 0x00836130, NiNavOrbitController* self, NiAVObject* controlled, const char* name, float scale, re::Float3);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline NiNavOrbitController* OrbitController_Construct(NiNavOrbitController* a0, NiAVObject* a1, const char* a2, float a3, float a4, float a5, float a6) {
    return OrbitController_Construct_x64(a0, a1, a2, a3, re::Float3{a4, a5, a6});
}
GAME_FN(void, __thiscall, AVObject_SetWorldTransform, 0x00833CE0, NiAVObject* self, const NiTransform* t);
GAME_FN(void, __thiscall, PointAlongAxis, 0x00833C00, const void* self, const NiPoint3* dir, float t,
        const NiPoint3* origin, NiPoint3* out);
GAME_FN(uint8_t, __thiscall, ControlInput_Equals, 0x00833BA0, const NiNavControlInput* self,
        const NiNavControlInput* other);

// the sample's own
GAME_FN(void, __thiscall, Sample_UpdateLog, 0x008312E0, NiSample* self);
GAME_FN(uint8_t, __thiscall, Sample_SetLogLine, 0x00831E00, NiSample* self, uint32_t index, const char* text);
GAME_FN(uint32_t, __thiscall, Sample_AddLogLine, 0x00832210, NiSample* self, const char* text);

// the templates' (folded) helpers
GAME_FN(void, __thiscall, ObjectArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, ObjectArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, ObjectArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __thiscall, PrimitiveArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, PrimitiveArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, PrimitiveSet_Realloc, 0x0083AD20, void* set, uint32_t alloced);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, void* set, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, ObjectSet_RemoveAt, 0x00838680, void* set, uint32_t index);
GAME_FN(void, __thiscall, ObjectSet_RemoveAll, 0x00836940, void* set);
GAME_FN(void, __cdecl, PointerArray_Delete, 0x00836F80, void* base);   // delete[] of NiPointers
GAME_FN(re::Ptr32<void>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<void>* self, void* object);

} // namespace ni::sample
