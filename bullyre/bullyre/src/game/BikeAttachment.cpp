// CBikeAttachment (vtable 0x90BD8C): an entity attached to a bike, with its own animation
// data; and small helpers from around it (a quaternion, a two-channel oscillator, an
// emitter's settings).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "game/Pool.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

struct CBikeAttachment : CEntity {
    int32_t anim;            // +0x120 a running animation's handle (0: none)
    uint8_t animData[0x104]; // +0x124 (0x6BA240 makes it; +0x114 points at it)
    re::Ptr32<uint8_t> animHolder;     // +0x228

    CBikeAttachment* Construct();
    void PreRender();
    void Render();
    void DeleteRwObject();
    void Shutdown();
    void SetModel(int16_t model);
    CBikeAttachment* Destroy(uint8_t flags);
};
static_assert(offsetof(CBikeAttachment, anim) == 0x120);
static_assert(offsetof(CBikeAttachment, animHolder) == 0x228);

inline re::Ptr32<void>* const s_bikeAttachmentVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090BD8C);
inline void* const s_animManager = reinterpret_cast<void*>(0x020C4910);
GAME_ARRAY(uint8_t*, s_modelInfoPtrs, 0x00C67738);
GAME_VAR(CPool<uint8_t>*, s_dummyPool, 0x00C0F600);

GAME_FN(void, __thiscall, Entity_Construct, 0x004656A0, CEntity* entity);
GAME_FN(void, __thiscall, Entity_Destruct, 0x00467E60, CEntity* entity);
GAME_FN(void, __thiscall, Entity_Render, 0x00466D40, CEntity* entity);  // CEntity vf18
GAME_FN(void, __thiscall, Entity_PreRender, 0x00466CC0, CEntity* entity);
GAME_FN(void, __thiscall, AnimData_Construct, 0x006BA240, void* data);
GAME_FN(void, __thiscall, AnimData_Destruct, 0x006B5A80, void* data);
GAME_FN(void, __thiscall, AnimManager_Stop, 0x006C2200, void* manager, void* holder, uintptr_t anim);
GAME_FN(int, __thiscall, ModelInfo_RemoveRef, 0x0050EBC0, void* modelInfo);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, BikeAttachment_DeleteRwObject, 0x00464F50, CBikeAttachment* self);
GAME_FN(void, __thiscall, BikeAttachment_Shutdown, 0x00464FA0, CBikeAttachment* self);
GAME_FN(void, __thiscall, NiQuaternion_Construct, 0x00775B90, float* q, float w, float x, float y, float z);
GAME_FN(void, __thiscall, NiQuaternion_FromRotation, 0x00776040, float* q, const void* rotation);
GAME_FN(int, __cdecl, Fn824EC0, 0x00824EC0, void* p);
GAME_FN(CColModel*, __cdecl, GetColModel, 0x0051AF50, CEntity* entity);
GAME_FN(col::CRect*, __cdecl, ColStore_GetBoundingBox, 0x0056F0F0, int slot);
GAME_FN(void, __thiscall, Rect_Extend, 0x00414410, col::CRect* rect, const col::CRect* other);
GAME_FN(void, __thiscall, ColModel_SetOwner, 0x0056E770, CColModel* col, void* owner);
GAME_ARRAY(CColModel*, s_tempColModels, 0x00CF3120);   // [0]: the one for models without collision

// the entity's vf8: a new model (the object made)
void SetModelIndexVirtual(CEntity* e, int model, int arg) {
    reinterpret_cast<void(__thiscall*)(CEntity*, int, int)>(static_cast<void*>(e->vtable[8]))(e, model, arg);
}

// 0x464EF0 (thiscall)
CBikeAttachment* CBikeAttachment::Construct() {
    Entity_Construct(this);
    vtable = s_bikeAttachmentVtable;
    AnimData_Construct(animData);
    animObject = animData;
    anim = 0;
    return this;
}

// 0x464F30 vf18 (thiscall): rendered only with an object
void CBikeAttachment::Render() {
    if (rwObject) {
        Entity_Render(this);
    }
}

// 0x464F40 vf17 (thiscall)
void CBikeAttachment::PreRender() {
    if (rwObject) {
        Entity_PreRender(this);
    }
}

// 0x464F50 (thiscall): its animation stopped, its model (and object) dropped
void CBikeAttachment::DeleteRwObject() {
    if (!rwObject) {
        return;
    }
    if (animHolder && anim) {
        AnimManager_Stop(s_animManager, animHolder, anim);
        anim = 0;
    }
    if (modelIndex != -1) {
        SetModelIndexVirtual(this, -1, 1);
    }
}

// 0x464FA0 (thiscall): its model's reference given back, then DeleteRwObject (inlined)
void CBikeAttachment::Shutdown() {
    if (modelIndex != -1) {
        ModelInfo_RemoveRef(s_modelInfoPtrs[modelIndex]);
    }
    if (!rwObject) {
        return;
    }
    if (animHolder && anim) {
        AnimManager_Stop(s_animManager, animHolder, anim);
        anim = 0;
    }
    if (modelIndex != -1) {
        SetModelIndexVirtual(this, -1, 1);
    }
}

// 0x465010 (thiscall): another model (-1: none)
void CBikeAttachment::SetModel(int16_t model) {
    if (rwObject) {
        BikeAttachment_DeleteRwObject(this);
    }
    if (model != -1) {
        SetModelIndexVirtual(this, model, 1);
    }
}

// 0x465040 vf0 (thiscall): the destructor (flags 1: and freed)
CBikeAttachment* CBikeAttachment::Destroy(uint8_t flags) {
    vtable = s_bikeAttachmentVtable;
    BikeAttachment_Shutdown(this);
    animObject = nullptr;
    AnimData_Destruct(animData);
    Entity_Destruct(this);
    if (flags & 1) {
        Free(this);
    }
    return this;
}

// ---- an emitter's settings (0x34) ---------------------------------------------------------

struct EmitterSettings {
    int32_t unknown00;     // +0
    int32_t unknown04;     // +4
    int32_t maxCount;      // +8
    int32_t unknown0C;     // +0xC
    int32_t count;         // +0x10 (1..maxCount)
    int32_t state;         // +0x14 (1 set, -1 stopped)
    re::Ptr32<int32_t> values;       // +0x18
    int32_t maxValues;     // +0x1C
    int32_t numValues;     // +0x20
    float a;               // +0x24
    float b;               // +0x28
    float c;               // +0x2C (hundredths)
    float d;               // +0x30 (hundredths)

    EmitterSettings* Construct();
    void Set(float a_, float b_, int count_, float c_, float d_, int numValues_, const int32_t* values_);
    void Stop();
};
static_assert(sizeof(EmitterSettings) == 0x34);

// 0x465080 (thiscall)
EmitterSettings* EmitterSettings::Construct() {
    a = 0.0f;
    b = 0.0f;
    unknown00 = 0;
    c = 0.0f;
    unknown04 = 0;
    d = 0.0f;
    maxCount = 0;
    unknown0C = 0;
    count = 0;
    state = 0;
    values = nullptr;
    maxValues = 0;
    numValues = 0;
    return this;
}

// 0x4650B0 (thiscall)
void EmitterSettings::Set(float a_, float b_, int count_, float c_, float d_, int numValues_, const int32_t* values_) {
    a = a_;
    b = b_;
    state = 1;
    const double hundredth = 0.009999999776482582;
    c = float(c_ * hundredth);
    d = float(hundredth * d_);
    if (count_ > maxCount) {
        count_ = maxCount;
    } else if (count_ <= 0) {
        count_ = 1;
    }
    count = count_;
    unknown0C = 0;
    numValues = numValues_ > maxValues ? maxValues : numValues_;
    for (int i = 0; i < numValues; i++) {
        values[i] = values_[i];
    }
}

// 0x465140 (thiscall)
void EmitterSettings::Stop() {
    state = -1;
}

// ---- helpers ------------------------------------------------------------------------------

// 0x465450: a dummy of the pool (index up to its size, inclusive) with +0x120 set
uint8_t __cdecl IsDummyWith120(uint8_t* dummy) {
    if (!dummy) {
        return 0;
    }
    CPool<uint8_t>* pool = s_dummyPool;
    int index = int(dummy - pool->objects) / pool->elemSize;
    if (index < 0 || index > pool->size) {
        return 0;
    }
    return *reinterpret_cast<int32_t*>(dummy + 0x120) != 0;
}

struct Vector3Ops {
    float x, y, z;

    CVector* SubtractInto(CVector* out, const CVector* v);
};

// 0x465480 (thiscall): this -= v, and a copy of the result
CVector* Vector3Ops::SubtractInto(CVector* out, const CVector* v) {
    x -= v->x;
    y -= v->y;
    z -= v->z;
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}

struct Quaternion {
    float x, y, z, w;

    void Multiply(const Quaternion* a, const Quaternion* b);
    void Set(const CMatrix* m);
};

// 0x4654C0 (thiscall): this = a * b
void Quaternion::Multiply(const Quaternion* a, const Quaternion* b) {
    float nx = float(double(b->x) * a->w + double(a->x) * b->w + double(a->y) * b->z - double(a->z) * b->y);
    float ny = float(double(a->w) * b->y + double(a->y) * b->w + double(b->x) * a->z - double(b->z) * a->x);
    float nz = float(double(b->z) * a->w + double(b->w) * a->z + double(a->x) * b->y - double(a->y) * b->x);
    w = float(double(b->w) * a->w - double(a->x) * b->x - double(a->y) * b->y - double(b->z) * a->z);
    x = nx;
    y = ny;
    z = nz;
}

// 0x465620 (thiscall): from a rotation (through NiQuaternion::FromRotation)
void Quaternion::Set(const CMatrix* m) {
    float q[4];  // NiQuaternion: w, x, y, z
    NiQuaternion_Construct(q, w, x, y, z);
    NiQuaternion_FromRotation(q, m);
    x = q[1];
    y = q[2];
    z = q[3];
    w = q[0];
}

// a growing array of 64-byte elements
struct Array64 {   // (the game's layout)
    re::Ptr32<uint8_t> data;   // +0
    uint32_t count;  // +4

    void* Get(uint32_t index);
};

// 0x465560 (thiscall): element i (nullptr past the end)
void* Array64::Get(uint32_t index) {
    if (index >= count) {
        return nullptr;
    }
    return data + index * 64;
}

// 0x466140: 0x824EC0's (it does nothing), then +0x1C set
void* __cdecl SetField1C(uint8_t* p, int32_t value) {
    Fn824EC0(p);
    *reinterpret_cast<int32_t*>(p + 0x1C) = value;
    return p;
}

// the two oscillators (0xA24008: speeds, 0xA2401C: positions 0..1, bouncing; 0xC2A8F8:
// phases, 0xC2A900: the last ones, 0xC2A908: the phases' sums)
GAME_ARRAY(float, s_oscSpeed, 0x00A24008);
GAME_ARRAY(float, s_oscPos, 0x00A2401C);
GAME_ARRAY(float, s_oscPhase, 0x00C2A8F8);
GAME_ARRAY(float, s_oscLastPhase, 0x00C2A900);
GAME_ARRAY(float, s_oscPhaseSum, 0x00C2A908);

// 0x465890: every frame: each speeds up towards the middle, slows past it, turns at the ends
void __cdecl UpdateOscillators() {
    for (int8_t i = 0; i < 2; i++) {
        float pos = s_oscSpeed[i] + s_oscPos[i];
        s_oscPos[i] = pos;
        bool lowHalf = pos < 0.5f;
        if (0.0f < s_oscSpeed[i]) {
            float factor = lowHalf ? 1.02f : 0.98f;
            float speed = s_oscSpeed[i] * factor;
            s_oscSpeed[i] = speed;
            if (speed < 0.003f) {
                s_oscSpeed[i] = 0.003f;
            }
        } else {
            float factor = lowHalf ? 0.98f : 1.02f;
            float speed = s_oscSpeed[i] * factor;
            s_oscSpeed[i] = speed;
            if (speed > -0.003f) {
                s_oscSpeed[i] = -0.003f;
            }
        }
        if (0.0f > s_oscPos[i]) {
            s_oscSpeed[i] = -s_oscSpeed[i];
            s_oscPos[i] = 0.0f;
        } else if (1.0f < s_oscPos[i]) {
            s_oscSpeed[i] = -s_oscSpeed[i];
            s_oscPos[i] = 1.0f;
        }
        s_oscLastPhase[i] = s_oscPhase[i];
        if (i == 0) {
            s_oscPhase[0] = float(s_oscPhase[0] + 0.02);
        } else {
            s_oscPhase[i] = float(s_oscPhase[i] + 0.04);
        }
        if (1.0f < s_oscPhase[i]) {
            s_oscPhase[i] = float(s_oscPhase[i] - 1.0);
        }
        s_oscPhaseSum[i] = float(double(s_oscPhase[i]) - s_oscLastPhase[i] + s_oscPhaseSum[i]);
    }
}

// 0x466260 (thiscall): its collision flags from its model's collision (none: no collision;
// the empty model: skipped by the line tests (+0xF4); +0x34 clear: no collision), its
// collision file's box grown by its rectangle, the model's owner set
void __fastcall SetupCollision(CEntity* self, re::Edx) {
    uint8_t* mi = s_modelInfoPtrs[self->modelIndex];
    if (!*reinterpret_cast<re::Ptr32<void>*>(mi + 0xC)) {
        self->bUsesCollision = 0;
        return;
    }
    self->flagF4 = 0;
    CColModel* col = GetColModel(self);
    if (col == s_tempColModels[0]) {
        self->flagF4 = 1;
    } else if (col->unknown34 == 0) {
        self->bUsesCollision = 0;
    }
    uint16_t slot = col->colSlot;
    if (slot) {
        col::CRect rect;
        col::CRect* bounds = self->CallGetBoundRect(&rect);
        Rect_Extend(ColStore_GetBoundingBox(slot), bounds);
    }
    ColModel_SetOwner(col, self);
}

} // namespace

#define BIKEATT_C05_REVERSED(name, address, fn) REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)
#define BIKEATT_C05_REVERSED_FN(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

BIKEATT_C05_REVERSED("CBikeAttachment::CBikeAttachment", 0x00464EF0, &CBikeAttachment::Construct);
BIKEATT_C05_REVERSED("CBikeAttachment::Render", 0x00464F30, &CBikeAttachment::Render);
BIKEATT_C05_REVERSED("CBikeAttachment::PreRender", 0x00464F40, &CBikeAttachment::PreRender);
BIKEATT_C05_REVERSED("CBikeAttachment::DeleteRwObject", 0x00464F50, &CBikeAttachment::DeleteRwObject);
BIKEATT_C05_REVERSED("CBikeAttachment::Shutdown", 0x00464FA0, &CBikeAttachment::Shutdown);
BIKEATT_C05_REVERSED("CBikeAttachment::SetModel", 0x00465010, &CBikeAttachment::SetModel);
BIKEATT_C05_REVERSED("CBikeAttachment::~CBikeAttachment", 0x00465040, &CBikeAttachment::Destroy);
BIKEATT_C05_REVERSED("EmitterSettings::EmitterSettings", 0x00465080, &EmitterSettings::Construct);
BIKEATT_C05_REVERSED("EmitterSettings::Set", 0x004650B0, &EmitterSettings::Set);
BIKEATT_C05_REVERSED("EmitterSettings::Stop", 0x00465140, &EmitterSettings::Stop);
BIKEATT_C05_REVERSED_FN("IsDummyWith120", 0x00465450, &IsDummyWith120);
BIKEATT_C05_REVERSED("CVector::SubtractInto", 0x00465480, &Vector3Ops::SubtractInto);
BIKEATT_C05_REVERSED("CQuaternion::Multiply", 0x004654C0, &Quaternion::Multiply);
BIKEATT_C05_REVERSED("Array64::Get", 0x00465560, &Array64::Get);
BIKEATT_C05_REVERSED("CQuaternion::Set", 0x00465620, &Quaternion::Set);
BIKEATT_C05_REVERSED_FN("UpdateOscillators", 0x00465890, &UpdateOscillators);
BIKEATT_C05_REVERSED_FN("SetField1C", 0x00466140, &SetField1C);
BIKEATT_C05_REVERSED_FN("CEntity::SetupCollision", 0x00466260, &SetupCollision);
