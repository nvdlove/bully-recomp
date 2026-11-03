// The speaker layout and panning (0x5A7E20..0x5A8FF8): the speaker mode (0xAE80E4 under
// the critical section 0xCF7148: 0/1 stereo, 2 5.1), the speaker positions (16 bytes
// each: stereo 0xAE8148, 5.1 0xAE80E8) rotated by the camera into 0xCF70E8, volume in
// millibels, and the per-speaker gains of a sound by angle or position (the sixth, at 3,
// is the centre/LFE one). A sound's gains go to its voice with 0x5AEAE0 (thiscall, six
// floats).
//
// (x87 notes: the CRT's log 0x85F670, cos 0x85AD80, sin 0x85AEF0, sqrt 0x85C500, pow
// 0x85CB70 are std:: here; the intermediate sums are double where the game used the
// x87's wider registers.)

#include <cmath>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct CVector {
    float x, y, z;
};

using re::At;

// a speaker (16 bytes)
struct Speaker {
    float x, y, z;
    float unknownC;
};
static_assert(sizeof(Speaker) == 0x10);

inline void* const speakerSection = reinterpret_cast<void*>(0x00CF7148);
inline void* const speakerRotation = reinterpret_cast<void*>(0x00AE8168);   // a 3x3 matrix
GAME_VAR(int32_t, speakerMode, 0x00AE80E4);
GAME_ARRAY(Speaker, speakersStereo, 0x00AE8148);
GAME_ARRAY(Speaker, speakers51, 0x00AE80E8);
GAME_ARRAY(Speaker, speakersRotated, 0x00CF70E8);
GAME_VAR(void*, nearPlane, 0x00C3CCF4);

using SectionFn = void(__stdcall*)(void*);
inline void Lock() {
    re::GameFnAt<SectionFn>(uintptr_t(0x008FC17C))(speakerSection);   // EnterCriticalSection
}
inline void Unlock() {
    re::GameFnAt<SectionFn>(uintptr_t(0x008FC178))(speakerSection);   // LeaveCriticalSection
}

inline void* __cdecl Memcpy(void* dst, const void* src, uint32_t size) { return crt::Memcpy(dst, src, (size_t)size); }
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const CVector* v);
GAME_FN(void*, __cdecl, Transform_SetRotation, 0x00415EB0, void* matrix, const CVector* axis, float angle, int);
GAME_FN(CVector*, __thiscall, Matrix_Multiply, 0x00768A70, const void* matrix, CVector* out, const CVector* v);
GAME_FN(void, __thiscall, Voice_SetGains, 0x005AEAE0, void* voice, float, float, float, float, float, float);
// this file's
GAME_FN(int, __cdecl, VolumeToMillibels, 0x005A7E50, float volume);
GAME_FN(int32_t, __cdecl, Speakers_Get, 0x005A7FB0, re::Ptr32<Speaker>* out);
GAME_FN(int32_t, __cdecl, Speakers_Count, 0x005A8010);
GAME_FN(void, __cdecl, Speakers_PanByAngle, 0x005A8460, void* voice, int level, int angle, float volume);
GAME_FN(void, __cdecl, Speakers_GainsByAngleStereo, 0x005A8390, re::Ptr32<float>* out, int level, int angle);
GAME_FN(void, __cdecl, Speakers_Rotate, 0x005A8850);

inline int32_t SpeakerCountLocked() {
    Lock();
    int32_t mode = speakerMode;
    Unlock();
    if (mode < 0) {
        return 0;
    }
    if (mode <= 1) {
        return 2;
    }
    return mode == 2 ? 6 : 0;
}

// 0x5A7E20: silences a voice
void __cdecl Voice_Silence(void* voice) {
    if (voice != nullptr) {
        Voice_SetGains(voice, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
}

// 0x5A7E50: a linear volume in millibels (600 per doubling; -8500 below 1e-7)
int __cdecl VolumeToMillibelsImpl(float volume) {
    if (!(double(volume) >= 1.0000000116860974e-07 || volume != volume)) {
        return -0x2134;
    }
    float logVolume = float(std::log(double(volume)));
    float logTwo = float(std::log(2.0));
    return int(double(logVolume) / double(logTwo) * 600.0);
}

// 0x5A7EA0: a pan value (-1536..1536 scaled by 8/3 and clamped to +-4096) as a gain offset
float __cdecl PanToGain(int pan) {
    float value = float(double(pan) * 2.6666667461395264);
    if (!(value <= 4096.0f || value != value)) {
        value = 4096.0f;
    } else if (-4096.0f > value) {
        value = -4096.0f;
    }
    return float(double(value) * 0.2919999957084656 * 0.009999999776482582);
}

// 0x5A7F00: a stereo balance: 0 centred, +-12 at the sides (2a/b over +-342)
float __cdecl BalanceToPan(float a, float b) {
    float ratio = 0.0f;
    if (!(b <= 0.0f || b != b)) {
        ratio = a / b;
    }
    ratio = ratio + ratio;
    if (ratio == 0.0f) {
        return 0.0f;
    }
    if (ratio >= 342.0f) {
        return -12.0f;
    }
    if (!(-342.0f < ratio || ratio != ratio)) {
        return 12.0f;
    }
    float t = float(1.0 - double(ratio) / 342.0);
    return float(double(t) * 12.0 - 12.0);
}

// 0x5A7FB0: the speakers and their count
int32_t __cdecl Speakers_GetImpl(re::Ptr32<Speaker>* out) {
    Lock();
    int32_t mode = speakerMode;
    Unlock();
    if (mode >= 0) {
        if (mode <= 1) {
            *out = speakersStereo;
            return 2;
        }
        if (mode == 2) {
            *out = speakers51;
            return 6;
        }
    }
    *out = nullptr;
    return 0;
}

// 0x5A8010: the speaker count
int32_t __cdecl Speakers_CountImpl() {
    return SpeakerCountLocked();
}

// the gains of a level in millibels: 10^((dB - 4 - trim) / 20), the trim 0 for the fronts
// and the centre, 3 for speaker 2, 6.2 for the rears
float GainOf(float decibels, int speaker) {
    double trim;
    switch (speaker) {
    case 2: trim = 3.0; break;
    case 4:
    case 5: trim = 6.199999809265137; break;
    default: trim = 0.0; break;
    }
    float exponent = float(double(decibels) - 4.0 - trim);
    return float(std::pow(10.0, double(exponent) / 20.0));
}

// 0x5A8050: sets a voice's gains for a volume (1/1024 units)
void __cdecl Voice_SetLevel(void* voice, int level) {
    Lock();
    if (voice != nullptr) {
        float scaled = float(double(level) * 0.0009765625);
        int millibels = VolumeToMillibels(scaled);
        float decibels = float(double(millibels) * 0.009999999776482582);
        int32_t count = SpeakerCountLocked();
        float gains[6] = {};
        for (int i = 0; i < 6; ++i) {
            gains[i] = i < count ? GainOf(decibels, i) : 0.0f;
        }
        Voice_SetGains(voice, gains[0], gains[1], gains[2], gains[3], gains[4], gains[5]);
    }
    Unlock();
}

// 0x5A8230: the same gains into an array (*out)
void __cdecl Speakers_GainsForLevel(re::Ptr32<float>* out, int level) {
    float scaled = float(double(level) * 0.0009765625);
    int millibels = VolumeToMillibels(scaled);
    float decibels = float(double(millibels) * 0.009999999776482582);
    int32_t count = SpeakerCountLocked();
    float* gains = *out;
    for (int i = 0; i < 6; ++i) {
        gains[i] = i < count ? GainOf(decibels, i) : 0.0f;
    }
}

// |1 - |angle - centre| / 180| (as floats like the game)
float SideWeight(int32_t offset) {
    float t = float(1.0 - double(offset) / 180.0);
    return std::fabs(t);
}

// 0x5A8390: stereo gains by angle (90 right, 270 left) into *out
void __cdecl Speakers_GainsByAngleStereoImpl(re::Ptr32<float>* out, int level, int angle) {
    float volume = float(double(level) * 0.0009765625);
    Lock();
    Unlock();
    float* gains = *out;
    int32_t right = 0x5A - angle;
    right = right < 0 ? -right : right;
    float rightWeight = SideWeight(right);
    gains[0] = float(double(rightWeight) * double(volume) * 0.75);
    float left = std::fabs(float(270.0 - double(angle)));
    float leftWeight = std::fabs(float(1.0 - double(left) / 180.0));
    gains[1] = float(double(leftWeight) * double(volume) * 0.75);
    gains[2] = 0.0f;
    gains[4] = 0.0f;
    gains[5] = 0.0f;
    gains[3] = float(double(volume) * 0.5 * 0.75);
}

// 0x5A8460: a voice's gains by angle (the volume, or level/1024 when it's 0): stereo
// halves the louder side into both in mode 0
void __cdecl Speakers_PanByAngleImpl(void* voice, int level, int angle, float volume) {
    Lock();
    if (voice != nullptr) {
        if (volume == 0.0f) {
            volume = float(double(level) * 0.0009765625);
        }
        Lock();
        Unlock();
        int32_t right = 0x5A - angle;
        right = right < 0 ? -right : right;
        float rightWeight = SideWeight(right);
        float left = std::fabs(float(270.0 - double(angle)));
        float leftWeight = std::fabs(float(1.0 - double(left) / 180.0));
        if (uint32_t(angle - 0x5B) <= 0xB2) {   // (behind)
            leftWeight = float(double(leftWeight) * 0.75);
            rightWeight = float(double(0.75) * rightWeight);
        }
        Lock();
        int32_t mode = speakerMode;
        Unlock();
        float first;
        float second;
        if (mode == 0) {
            float louder = (leftWeight >= rightWeight || leftWeight != leftWeight) ? leftWeight : rightWeight;
            first = float(double(louder) * double(volume) * 0.75 * 0.5);
            second = first;
        } else {
            first = float(double(rightWeight) * double(volume) * 0.75);
            second = float(double(leftWeight) * double(volume) * 0.75);
        }
        float centre = float(double(volume) * 0.5 * 0.75);
        Voice_SetGains(voice, first, second, 0.0f, centre, 0.0f, 0.0f);
    }
    Unlock();
}

// a speaker's gain from a unit direction's point: 1 - 0.65 * distance
float DistanceGain(const CVector& point, const Speaker& speaker) {
    float dx = float(point.x - speaker.x);
    float dy = float(point.y - speaker.y);
    float dz = float(point.z - speaker.z);
    float length2 = float(double(dy) * dy + double(dx) * dx + double(dz) * dz);
    float distance = float(std::sqrt(double(std::fabs(length2))));
    return float(1.0 - double(distance) * 0.6499999761581421);
}

// 0x5A8620: a voice's gains by angle on 5.1 (by distance to each speaker); stereo modes
// use 0x5A8460
void __cdecl Speakers_Pan51(void* voice, int level, int angle, float volume) {
    Lock();
    Lock();
    int32_t mode = speakerMode;
    Unlock();
    if (mode != 2) {
        Speakers_PanByAngle(voice, level, angle, volume);
        Unlock();
        return;
    }
    if (voice == nullptr) {
        Unlock();
        return;
    }
    float radians = float(double(angle) * 3.1415927410125732 / 180.0);
    float c = float(std::cos(double(radians)));
    float s = float(std::sin(double(radians)));
    CVector point = {-s, c, 0.0f};
    if (volume == 0.0f) {
        volume = float(double(level) * 0.0009765625);
    }
    int32_t count = Speakers_Count();
    float gains[6] = {};
    for (int i = 0; i < 6; ++i) {
        if (i == 3) {
            continue;
        }
        if (i >= count) {
            gains[i] = 0.0f;
            continue;
        }
        float gain = DistanceGain(point, speakers51[i]);
        if (!(0.0f <= gain || gain != gain)) {
            gain = 0.0f;
        }
        gains[i] = float(double(volume) * gain * 0.5);
    }
    gains[3] = float(double(volume) * 0.5 * 0.5);
    Voice_SetGains(voice, gains[0], gains[1], gains[2], gains[3], gains[4], gains[5]);
    Unlock();
}

// 0x5A8850: rotates the speaker positions by the camera's heading
void __cdecl Speakers_RotateImpl() {
    Lock();
    float* t = GetLocalTransform(GetCameraNode(nearPlane));
    CVector direction;
    direction.x = t[1];
    direction.y = GetLocalTransform(GetCameraNode(nearPlane))[4];
    direction.z = GetLocalTransform(GetCameraNode(nearPlane))[7];
    float* last = GetLocalTransform(GetCameraNode(nearPlane));
    CVector up = {last[2], last[5], last[8]};
    float heading = VecHeading(&up);
    double length2 = double(direction.y) * direction.y + double(direction.x) * direction.x +
                     double(direction.z) * direction.z;
    if (length2 > 0.0001) {
        Transform_SetRotation(speakerRotation, &direction, heading, 0);
        re::Ptr32<Speaker> speakers;
        int32_t count = Speakers_Get(&speakers);
        for (int32_t i = 0; i < count; ++i) {
            CVector in = {speakers[i].x, speakers[i].y, speakers[i].z};
            CVector result;
            CVector* rotated = Matrix_Multiply(speakerRotation, &result, &in);
            CVector copy = *rotated;
            speakersRotated[i].x = copy.x;
            speakersRotated[i].y = copy.y;
            speakersRotated[i].z = copy.z;
        }
    }
    Unlock();
}

// the direction of an angle, rotated with the speakers: (-r.y, r.x, r.z)
CVector RotatedDirection(int angle) {
    float radians = float(double(angle) * 3.1415927410125732 / 180.0);
    float c = float(std::cos(double(radians)));
    float s = float(std::sin(double(radians)));
    CVector in = {c, s, 0.0f};
    CVector result;
    CVector* r = Matrix_Multiply(speakerRotation, &result, &in);
    CVector rotated = *r;
    return {-rotated.y, rotated.x, rotated.z};
}

// the 5.1 gain of the rotated direction for one speaker: ((1 - v) * 0.3 + 0.6) *
// (g - 0.5) + 0.5 for g > 0
float BlendGain(float gain, float volume) {
    return float(((1.0 - double(volume)) * 0.30000001192092896 + 0.6000000238418579) * (double(gain) - 0.5) + 0.5);
}

// 0x5A89F0: 5.1 gains by angle into *out (speakers with no gain keep their old value);
// stereo modes use 0x5A8390
void __cdecl Speakers_GainsByAngle(re::Ptr32<float>* out, int level, int angle) {
    Lock();
    Lock();
    int32_t mode = speakerMode;
    Unlock();
    if (mode != 2) {
        Speakers_GainsByAngleStereo(out, level, angle);
        Unlock();
        return;
    }
    CVector point = RotatedDirection(angle);
    float volume = float(double(level) * 0.0009765625);
    int32_t count = SpeakerCountLocked();
    for (int i = 0; i < 6; ++i) {
        if (i == 3) {
            continue;
        }
        if (i >= count) {
            (*out)[i] = 0.0f;
            continue;
        }
        float gain = DistanceGain(point, speakersRotated[i]);
        if (gain >= 0.0f && gain > 0.0f) {
            float blended = BlendGain(gain, volume);
            (*out)[i] = float(double(blended) * volume * 0.75);
        }
    }
    (*out)[3] = float(double(volume) * 0.5 * 0.75);
    Unlock();
}

// 0x5A89F0's voice form (0x5A8C60): no gain is 0 here; stereo modes use 0x5A8460 (debug
// strings the game builds and drops are left out)
void __cdecl Speakers_PanRotated(void* voice, int level, int angle) {
    Lock();
    Lock();
    int32_t mode = speakerMode;
    Unlock();
    if (mode != 2) {
        Speakers_PanByAngle(voice, level, angle, 0.0f);
        Unlock();
        return;
    }
    if (voice == nullptr) {
        Unlock();
        return;
    }
    CVector point = RotatedDirection(angle);
    float volume = float(double(level) * 0.0009765625);
    int32_t count = Speakers_Count();
    float gains[6] = {};
    for (int i = 0; i < 6; ++i) {
        if (i == 3) {
            continue;
        }
        if (i >= count) {
            gains[i] = 0.0f;
            continue;
        }
        float gain = DistanceGain(point, speakersRotated[i]);
        if (!(gain >= 0.0f || gain != gain)) {
            gain = 0.0f;
        } else if (gain > 0.0f) {
            gain = BlendGain(gain, volume);
        }
        gains[i] = float(double(gain) * volume * 0.75);
    }
    gains[3] = float(double(volume) * 0.5 * 0.75);
    Voice_SetGains(voice, gains[0], gains[1], gains[2], gains[3], gains[4], gains[5]);
    Unlock();
}

// 0x5A8FA0: sets the speaker mode and rotates the new layout
void __cdecl Speakers_SetMode(int32_t mode) {
    Lock();
    speakerMode = mode;
    Lock();
    re::Ptr32<Speaker> speakers;
    int32_t count = Speakers_Get(&speakers);
    Memcpy(speakersRotated, speakers, uint32_t(count) << 4);
    Speakers_Rotate();
    Unlock();
    Unlock();
}

// ---- registration ---------------------------------------------------------------------------

#define SURROUND_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(surround_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(surround_original_, __LINE__), \
             re::Mode::Original)

SURROUND_REVERSED("Voice::Silence", 0x005A7E20, &Voice_Silence);
SURROUND_REVERSED("Speakers::VolumeToMillibels", 0x005A7E50, &VolumeToMillibelsImpl);
SURROUND_REVERSED("Speakers::PanToGain", 0x005A7EA0, &PanToGain);
SURROUND_REVERSED("Speakers::BalanceToPan", 0x005A7F00, &BalanceToPan);
SURROUND_REVERSED("Speakers::Get", 0x005A7FB0, &Speakers_GetImpl);
SURROUND_REVERSED("Speakers::Count", 0x005A8010, &Speakers_CountImpl);
SURROUND_REVERSED("Voice::SetLevel", 0x005A8050, &Voice_SetLevel);
SURROUND_REVERSED("Speakers::GainsForLevel", 0x005A8230, &Speakers_GainsForLevel);
SURROUND_REVERSED("Speakers::GainsByAngleStereo", 0x005A8390, &Speakers_GainsByAngleStereoImpl);
SURROUND_REVERSED("Speakers::PanByAngle", 0x005A8460, &Speakers_PanByAngleImpl);
SURROUND_REVERSED("Speakers::Pan51", 0x005A8620, &Speakers_Pan51);
SURROUND_REVERSED("Speakers::Rotate", 0x005A8850, &Speakers_RotateImpl);
SURROUND_REVERSED("Speakers::GainsByAngle", 0x005A89F0, &Speakers_GainsByAngle);
SURROUND_REVERSED("Speakers::PanRotated", 0x005A8C60, &Speakers_PanRotated);
SURROUND_REVERSED("Speakers::SetMode", 0x005A8FA0, &Speakers_SetMode);

} // namespace
