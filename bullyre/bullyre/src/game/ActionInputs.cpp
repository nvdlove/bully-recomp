// A ped's action requests (47 buttons, each a state byte and how long it has been held,
// with the previous frame's time) and its motion intention's two event flags (set at
// most every 0.1 s). 0x5F3700..0x5F3BC0.

#include <cfloat>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

// state bits: 1 pressed this frame (cleared to its 2 on the next), 2 held, 8 released
// (dropped the frame after)
struct ActionRequest {
    uint8_t state;   // +0
    uint8_t pad[3];
    float time;      // +4
    float prevTime;  // +8
};
static_assert(sizeof(ActionRequest) == 0xC);

constexpr int kNumRequests = 0x2F;

struct ActionInputs {
    float unknown0[6];                   // +0x0
    int32_t unknown18;                   // +0x18
    float unknown1C;                     // +0x1C
    ActionRequest requests[kNumRequests];  // +0x20
    uint32_t unknown254;                 // +0x254 (a member with its own constructor, 0x66BEA0)
};
static_assert(offsetof(ActionInputs, requests) == 0x20);
static_assert(offsetof(ActionInputs, unknown254) == 0x254);

struct MotionIntention {
    float unknown0[3];     // +0x0
    float unknownC[4];     // +0xC
    float unknown1C;       // +0x1C
    float unknown20;       // +0x20
    float unknown24;       // +0x24
    float unknown28;       // +0x28
    float unknown2C;       // +0x2C
    uint8_t eventA;        // +0x30
    uint8_t eventB;        // +0x31
    uint8_t pad32[2];
    float timeSinceA;      // +0x34
    float timeSinceB;      // +0x38
};
static_assert(offsetof(MotionIntention, timeSinceB) == 0x38);

GAME_VAR(float, kEventInterval, 0x00B0A4EC);   // 0.1
GAME_FN(void, __thiscall, Member254_Construct, 0x0066BEA0, void* member);
GAME_FN(void, __thiscall, ActionInputs_Clear, 0x005F3A40, ActionInputs* self);

// 0x5F3700 (thiscall)
uint32_t __fastcall IsPressed(ActionInputs* self, re::Edx, int button) {
    return self->requests[button].state & 1;
}

// 0x5F3720 (thiscall): nullptr past the last
ActionRequest* __fastcall GetRequest(ActionInputs* self, re::Edx, int button) {
    if (button < kNumRequests) {
        return &self->requests[button];
    }
    return nullptr;
}

// 0x5F3740 (thiscall)
void __fastcall Set254(ActionInputs* self, re::Edx, const uint32_t* value) {
    self->unknown254 = *value;
}

// 0x5F3750 (thiscall)
void __fastcall MotionIntention_AddTime(MotionIntention* self, re::Edx, float dt) {
    self->timeSinceA = float(double(dt) + double(self->timeSinceA));
    self->timeSinceB = float(double(dt) + double(self->timeSinceB));
}

// 0x5F3770 (thiscall): set (at most every 0.1 s) or cleared
void __fastcall MotionIntention_SetEventA(MotionIntention* self, re::Edx, int set) {
    if (!re::Flag(set)) {
        self->eventA = 0;
        return;
    }
    if (kEventInterval < self->timeSinceA) {
        self->eventA = 1;
        self->timeSinceA = 0.0f;
    }
}

// 0x5F37A0 (thiscall)
void __fastcall MotionIntention_SetEventB(MotionIntention* self, re::Edx, int set) {
    if (!re::Flag(set)) {
        self->eventB = 0;
        return;
    }
    if (kEventInterval < self->timeSinceB) {
        self->eventB = 1;
        self->timeSinceB = 0.0f;
    }
}

void Age(ActionRequest& r, float dt) {
    r.time = float(double(dt) + double(r.time));
    if (r.state & 1) {
        r.state &= 2;
    }
    if (((r.state >> 3) & 1) && r.state != 0) {
        r.state = 0;
    }
}

// 0x5F3880 (thiscall): a frame on: the times grow, pressed becomes held, released goes
void __fastcall Update(ActionInputs* self, re::Edx, float dt) {
    for (int i = 0; i < kNumRequests; i++) {
        Age(self->requests[i], dt);
    }
    self->unknown254 = 0;
}

// 0x5F3A40 (thiscall): all released, their times kept as the previous ones
void __fastcall Clear(ActionInputs* self, re::Edx) {
    for (int i = 0; i < kNumRequests; i++) {
        ActionRequest& r = self->requests[i];
        if (r.prevTime != r.time) {
            r.prevTime = r.time;
        }
        if (0.0f != r.time) {
            r.time = 0.0f;
        }
        if (r.state != 0) {
            r.state = 0;
        }
    }
    self->unknown254 = 0;
    self->unknown1C = 0.0f;
}

// 0x5F3A90 (thiscall): a button set to 8 (kind 3), 3 (kind 2) or 1 (else), its time
// restarted; a non-zero `time` then stands as its time
void __fastcall SetRequest(ActionInputs* self, re::Edx, int button, int kind, float time) {
    ActionRequest& r = self->requests[button];
    if (kind == 3) {
        if (r.state != 8) {
            r.state = 8;
        }
        if (r.time != r.prevTime) {
            r.prevTime = r.time;
        }
        if (0.0f != r.time) {
            r.time = 0.0f;
        }
    } else {
        uint8_t state = kind == 2 ? 3 : 1;
        if (r.time != r.prevTime) {
            r.prevTime = r.time;
        }
        if (0.0f != r.time) {
            r.time = 0.0f;
        }
        if (r.state != state) {
            r.state = state;
        }
    }
    if (time != 0.0f) {
        r.time = time;
    }
}

// 0x5F3B70 (thiscall): the motion intention's constructor
MotionIntention* __fastcall MotionIntention_Construct(MotionIntention* self, re::Edx) {
    self->unknown24 = 0.0f;
    self->unknown28 = 0.0f;
    self->unknown2C = 0.0f;
    self->eventA = 0;
    self->unknown0[0] = -1.0f;
    self->eventB = 0;
    self->unknown0[1] = -1.0f;
    self->unknown0[2] = -1.0f;
    self->unknown1C = -1.0f;
    self->unknown20 = 0.0f;
    self->unknown24 = 1.0f;
    self->unknown28 = 0.0f;
    self->unknown2C = 0.0f;
    self->timeSinceA = kEventInterval;
    self->timeSinceB = kEventInterval;
    return self;
}

// 0x5F3BC0 (thiscall): the constructor (the times at FLT_MAX, then cleared)
ActionInputs* __fastcall Construct(ActionInputs* self, re::Edx) {
    for (int i = 0; i < 6; i++) {
        self->unknown0[i] = 0.0f;
    }
    for (int i = 0; i < kNumRequests; i++) {
        self->requests[i].time = FLT_MAX;
        self->requests[i].prevTime = FLT_MAX;
    }
    Member254_Construct(&self->unknown254);
    ActionInputs_Clear(self);
    self->unknown18 = 0;
    return self;
}

} // namespace

#define ACTINPUTS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTINPUTS_REVERSED("ActionInputs::IsPressed", 0x005F3700, &IsPressed);
ACTINPUTS_REVERSED("ActionInputs::GetRequest", 0x005F3720, &GetRequest);
ACTINPUTS_REVERSED("ActionInputs::Set254", 0x005F3740, &Set254);
ACTINPUTS_REVERSED("MotionIntention::AddTime", 0x005F3750, &MotionIntention_AddTime);
ACTINPUTS_REVERSED("MotionIntention::SetEventA", 0x005F3770, &MotionIntention_SetEventA);
ACTINPUTS_REVERSED("MotionIntention::SetEventB", 0x005F37A0, &MotionIntention_SetEventB);
ACTINPUTS_REVERSED("ActionInputs::Update", 0x005F3880, &Update);
ACTINPUTS_REVERSED("ActionInputs::Clear", 0x005F3A40, &Clear);
ACTINPUTS_REVERSED("ActionInputs::SetRequest", 0x005F3A90, &SetRequest);
ACTINPUTS_REVERSED("MotionIntention::Construct", 0x005F3B70, &MotionIntention_Construct);
ACTINPUTS_REVERSED("ActionInputs::Construct", 0x005F3BC0, &Construct);
