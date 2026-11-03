// The AM_ animation system's players: an anim player (0x14 bytes: the anim, its time,
// last frame's time, the rate, flags 1 playing / 2 looping / 4 started) stepping through
// an anim and collecting the events it passes, and AM_AnimTrack's Update (two players
// blended, the track faded in or out). 0x6B1FC0..0x6B29DE.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

struct AnimEvent {
    float time;  // +0 (a fraction of the anim)
    int32_t unknown04;
    int32_t kind;  // +8
    re::Ptr32<void> data;    // +0xC
};
static_assert(sizeof(AnimEvent) == 0x10);

struct Anim {
    uint8_t unknown00[0xC];
    float duration;  // +0xC
    uint8_t unknown10[8];
    uint32_t flags;  // +0x18 (4: wrapping clears the player's "started")
    uint8_t unknown1C[8];
    re::Ptr32<AnimEvent> events;    // +0x24
    uint16_t eventCount;  // +0x28
};

struct AnimPlayer {
    re::Ptr32<Anim> anim;      // +0
    float time;      // +4
    float lastTime;  // +8
    float rate;      // +0xC
    uint32_t flags;  // +0x10
};
static_assert(sizeof(AnimPlayer) == 0x14);

struct AnimTrack {
    re::Ptr32<void> vtable;
    uint8_t unknown04[8];
    float weight;        // +0xC
    float fadeTime;      // +0x10 (> 0: fading out, < 0: fading in)
    float fadeDuration;  // +0x14
    AnimPlayer players[2];  // +0x18
    float blend;         // +0x40 the second player's share
};
static_assert(sizeof(AnimTrack) == 0x44);

// the events passed this frame (the list isn't bounds-checked: room for 20)
GAME_VAR(int32_t, am_eventCount, 0x020C4A68);
GAME_ARRAY(AnimEvent*, am_events, 0x020C4A6C);
GAME_ARRAY(Anim*, am_eventAnims, 0x020C4ABC);

GAME_FN(void, __thiscall, Player_CollectEvents, 0x006B2220, AnimPlayer* self, float dt);
GAME_FN(void, __thiscall, Player_Update, 0x006B26C0, AnimPlayer* self, float dt, int collectEvents);

// 0x6B1FC0: winds +0x28 towards 0 by dt (flag 1 of +0x38 on; flag 2 set by each step,
// flag 1 cleared only when a negative value reaches 0)
void __fastcall Fader_Step(uint8_t* self, re::Edx, float dt) {
    uint32_t& flags = *reinterpret_cast<uint32_t*>(self + 0x38);
    float& value = *reinterpret_cast<float*>(self + 0x28);
    float& other = *reinterpret_cast<float*>(self + 0x2C);
    uint32_t f = flags;
    if (!(f & 1)) {
        return;
    }
    f |= 2;
    flags = f;
    float magnitude = std::fabs(value);
    if (magnitude < 0.001f) {
        return;
    }
    if (0.0f < value) {
        float v = value - dt;
        value = v;
        if (v < 0.0f) {
            value = 0.0f;
            other = 0.0f;
        }
    } else {
        float v = value + dt;
        value = v;
        if (v > 0.0f) {
            value = 0.0f;
            other = 0.0f;
            flags = f & ~1u;
        }
    }
}

inline void Record(Anim* anim, AnimEvent* event) {
    am_eventAnims[am_eventCount] = anim;
    am_events[am_eventCount] = event;
    am_eventCount++;
}

// 0x6B2220: collects the events between last frame's time and this one's (backwards
// when the rate is negative; stops at the first past the time)
void __fastcall CollectEvents(AnimPlayer* self, re::Edx, float) {
    uint32_t count = self->anim->eventCount;
    if (count == 0 && (self->flags & 1)) {
        return;
    }
    if (0.0f <= self->rate) {
        for (uint32_t i = 0; int(i) < int(count); i++) {
            Anim* anim = self->anim;
            AnimEvent* event = i < anim->eventCount ? &anim->events[i] : nullptr;
            float t = anim->duration * event->time;
            if (self->lastTime < t) {
                if (!(self->time >= t)) {
                    return;
                }
                Record(anim, event);
            }
        }
    } else {
        for (int i = int(count) - 1; i >= 0; i--) {
            Anim* anim = self->anim;
            AnimEvent* event = uint32_t(i) < anim->eventCount ? &anim->events[i] : nullptr;
            float t = anim->duration * event->time;
            if (self->lastTime > t) {
                if (self->time > t || std::isnan(self->time)) {
                    return;
                }
                Record(anim, event);
            }
        }
    }
}

// 0x6B26C0: steps the player by dt (wrapping when looping, stopping at the ends)
void __fastcall PlayerUpdate(AnimPlayer* self, re::Edx, float dt, int collectEvents) {
    Anim* anim = self->anim;
    if (anim == nullptr) {
        return;
    }
    uint32_t flags = self->flags;
    float delta = self->rate * dt;
    bool notStarted = !((flags >> 2) & 1);
    bool playing = (flags & 1) != 0;
    if (playing) {
        self->lastTime = self->time;
    }
    if (self->lastTime < 0.0f) {
        self->time = 0.0f;
        self->lastTime = 0.0f;
    }
    float magnitude = std::fabs(anim->duration);
    if (magnitude < 0.001f) {
        self->time = 0.0f;
        self->flags = flags & ~1u;
        return;
    }
    if (notStarted) {
        if (playing) {
            self->flags = flags | 4;
        }
        return;
    }
    if (!playing) {
        return;
    }
    self->time = self->time + delta;
    if (re::Flag(collectEvents)) {
        Player_CollectEvents(self, dt);
    }
    if (self->time < 0.0f) {
        uint32_t f = self->flags;
        if (f & 2) {
            float t = self->time + self->anim->duration;
            self->time = t;
            if (t < 0.0f) {
                self->time = self->anim->duration;
            }
            if ((self->anim->flags >> 2) & 1) {
                self->flags = f & ~4u;
            }
        } else {
            self->time = 0.0f;
            self->flags = f & ~1u;
        }
    }
    anim = self->anim;
    if (!(anim->duration < self->time)) {
        return;
    }
    uint32_t f = self->flags;
    if (f & 2) {
        float t = self->time - anim->duration;
        self->time = t;
        if (anim->duration < t) {
            self->time = 0.0f;
        }
        if ((anim->flags >> 2) & 1) {
            self->flags = f & ~4u;
        }
        return;
    }
    self->time = anim->duration;
    self->flags = f & ~1u;
}

// 0x6B2850: AM_AnimTrack::Update: the fade, then both players (their events while the
// track and the player have weight); false when the track faded out
uint8_t __fastcall Track_Update(AnimTrack* self, re::Edx, float dt) {
    if (self->fadeTime < 0.0f) {
        // fading in
        if (self->weight == 1.0f) {
            self->fadeTime = 0.0f;
            self->fadeDuration = 0.0f;
        } else {
            float w = self->weight - dt / self->fadeDuration;
            self->weight = w;
            float fade = dt + self->fadeTime;
            self->fadeTime = fade;
            if (w > 1.0f) {
                self->weight = 1.0f;
                self->fadeTime = 0.0f;
                self->fadeDuration = 0.0f;
            } else if (fade > 0.0f) {
                self->weight = 1.0f;
                self->fadeTime = 0.0f;
                self->fadeDuration = 0.0f;
            }
        }
    } else if (0.0f < self->fadeTime) {
        // fading out
        if (self->weight < 0.001f) {
            self->fadeTime = 0.0f;
            self->fadeDuration = 0.0f;
            return 0;
        }
        float w = self->weight - dt / self->fadeDuration;
        self->weight = w;
        float fade = self->fadeTime - dt;
        self->fadeTime = fade;
        if (w < 0.0f) {
            self->fadeTime = 0.0f;
            self->weight = 0.0f;
            self->fadeDuration = 0.0f;
            return 0;
        }
        if (fade < 0.0f) {
            self->fadeTime = 0.0f;
            self->weight = 0.0f;
            self->fadeDuration = 0.0f;
            return 0;
        }
    }
    for (uint32_t i = 0; i < 2; i++) {
        float share = i == 0 ? 1.0f - self->blend : self->blend;
        int events = (0.0f < share && 0.0f < self->weight && 0.0f < self->weight) ? 1 : 0;
        Player_Update(&self->players[i], dt, events);
    }
    return 1;
}

} // namespace

#define AMPLAYERS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMPLAYERS_REVERSED("AM_Fader::Step", 0x006B1FC0, &Fader_Step);
AMPLAYERS_REVERSED("AM_AnimPlayer::CollectEvents", 0x006B2220, &CollectEvents);
AMPLAYERS_REVERSED("AM_AnimPlayer::Update", 0x006B26C0, &PlayerUpdate);
AMPLAYERS_REVERSED("AM_AnimTrack::Update", 0x006B2850, &Track_Update);
