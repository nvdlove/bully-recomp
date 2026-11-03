// XACT 3's interfaces (xact3.h: IXACT3Engine, IXACT3SoundBank, IXACT3WaveBank, IXACT3Cue) over
// FAudio's FACT, whose functions are the same methods one for one and whose structures have
// XACT's layouts. What the shim adds:
// - objects with XACT's vtables around FACT's (the game's code calls the vtable slots);
// - the notifications' object pointers turned back into ours (FACT reports its own);
// - XACT's default file reading for streaming wave banks (Win32 ReadFile on the handle the
//   game opened), which FACT does on its own stream type unless given these callbacks;
// - cue properties copied into CoTaskMemAlloc's memory (the game frees them with
//   CoTaskMemFree);
// - on x64, the structures with pointers (the runtime and streaming parameters, notification
//   descriptions and notifications) turned between the game's 32-bit layouts and FACT's.
// The waves (PrepareWave and the IXACT3Wave interface) aren't used by the game: E_NOTIMPL.

#include "game/XactShim.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <windows.h>
#include <objbase.h>

#if defined(_WIN64)
#include "x64/lowheap.h"
#endif

// (FACT.h's nameless unions: a Microsoft extension)
#pragma warning(push)
#pragma warning(disable : 4201)
extern "C" {
#include "FACT.h"
#include "reversed.h"
}
#pragma warning(pop)

namespace xact {

namespace {

constexpr long kOk = 0;
constexpr long kNotImplemented = long(0x80004001);
constexpr long kNoInterface = long(0x80004002);
constexpr long kInvalidArgument = long(0x80070057);
constexpr long kOutOfMemory = long(0x8007000E);

class Engine;
class SoundBank;
class WaveBank;
class Cue;

// FACT's objects to ours (the notifications name FACT's)
std::mutex g_lock;
std::unordered_map<FACTCue*, Cue*> g_cues;
std::unordered_map<FACTSoundBank*, SoundBank*> g_soundBanks;
std::unordered_map<FACTWaveBank*, WaveBank*> g_waveBanks;

// ---- the game's 32-bit layouts of the structures with pointers (the same as FACT's on Win32)

#if defined(_M_X64)
void* Wide(const void* at) {
    int32_t v;
    std::memcpy(&v, at, 4);
    return reinterpret_cast<void*>(intptr_t(v));
}
void Narrow(void* at, const void* p) {
    uint32_t v = re::Addr32(p);
    std::memcpy(at, &v, 4);
}
template <typename T>
T Get(const uint8_t* at) {
    T v;
    std::memcpy(&v, at, sizeof(T));
    return v;
}
template <typename T>
void Put(uint8_t* at, T v) {
    std::memcpy(at, &v, sizeof(T));
}
#endif

// XACT_RUNTIME_PARAMETERS
FACTRuntimeParameters RuntimeParameters(const void* game) {
    FACTRuntimeParameters p;
#if defined(_M_X64)
    const auto* g = static_cast<const uint8_t*>(game);
    p.lookAheadTime = Get<uint32_t>(g);
    p.pGlobalSettingsBuffer = Wide(g + 4);
    p.globalSettingsBufferSize = Get<uint32_t>(g + 8);
    p.globalSettingsFlags = Get<uint32_t>(g + 0xC);
    p.globalSettingsAllocAttributes = Get<uint32_t>(g + 0x10);
    p.fileIOCallbacks.readFileCallback = reinterpret_cast<FACTReadFileCallback>(Wide(g + 0x14));
    p.fileIOCallbacks.getOverlappedResultCallback = reinterpret_cast<FACTGetOverlappedResultCallback>(Wide(g + 0x18));
    p.fnNotificationCallback = reinterpret_cast<FACTNotificationCallback>(Wide(g + 0x1C));
    p.pRendererID = static_cast<int16_t*>(Wide(g + 0x20));
    p.pXAudio2 = static_cast<FAudio*>(Wide(g + 0x24));
    p.pMasteringVoice = static_cast<FAudioMasteringVoice*>(Wide(g + 0x28));
#else
    std::memcpy(&p, game, sizeof(p));
#endif
    return p;
}

// XACT_STREAMING_PARAMETERS
FACTStreamingParameters StreamingParameters(const void* game) {
    FACTStreamingParameters p;
#if defined(_M_X64)
    const auto* g = static_cast<const uint8_t*>(game);
    p.file = Wide(g);
    p.offset = Get<uint32_t>(g + 4);
    p.flags = Get<uint32_t>(g + 8);
    p.packetSize = Get<uint16_t>(g + 0xC);
#else
    std::memcpy(&p, game, sizeof(p));
#endif
    return p;
}

// XACT_NOTIFICATION_DESCRIPTION (packed)
FACTNotificationDescription NotificationDescription(const void* game) {
    FACTNotificationDescription d;
#if defined(_M_X64)
    const auto* g = static_cast<const uint8_t*>(game);
    d.type = g[0];
    d.flags = g[1];
    d.pSoundBank = static_cast<FACTSoundBank*>(Wide(g + 2));
    d.pWaveBank = static_cast<FACTWaveBank*>(Wide(g + 6));
    d.pCue = static_cast<FACTCue*>(Wide(g + 0xA));
    d.pWave = static_cast<FACTWave*>(Wide(g + 0xE));
    d.cueIndex = Get<uint16_t>(g + 0x12);
    d.waveIndex = Get<uint16_t>(g + 0x14);
    d.pvContext = Wide(g + 0x16);
#else
    std::memcpy(&d, game, sizeof(d));
#endif
    return d;
}

// XACT_NOTIFICATION (packed) as the game's callback reads it: the type, +1 the time stamp, +5
// the context, the union from +9
#if defined(_M_X64)
void GameNotification(const FACTNotification& n, uint8_t* g) {
    g[0] = n.type;
    Put<int32_t>(g + 1, n.timeStamp);
    Narrow(g + 5, n.pvContext);
    uint8_t* u = g + 9;
    switch (n.type) {
    case FACTNOTIFICATIONTYPE_CUEPREPARED:
    case FACTNOTIFICATIONTYPE_CUEPLAY:
    case FACTNOTIFICATIONTYPE_CUESTOP:
    case FACTNOTIFICATIONTYPE_CUEDESTROYED:
        Put<uint16_t>(u, n.cue.cueIndex);
        Narrow(u + 2, n.cue.pSoundBank);
        Narrow(u + 6, n.cue.pCue);
        break;
    case FACTNOTIFICATIONTYPE_MARKER:
        Put<uint16_t>(u, n.marker.cueIndex);
        Narrow(u + 2, n.marker.pSoundBank);
        Narrow(u + 6, n.marker.pCue);
        Put<uint32_t>(u + 0xA, n.marker.marker);
        break;
    case FACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED:
        Narrow(u, n.soundBank.pSoundBank);
        break;
    case FACTNOTIFICATIONTYPE_WAVEBANKDESTROYED:
    case FACTNOTIFICATIONTYPE_WAVEBANKPREPARED:
    case FACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT:
        Narrow(u, n.waveBank.pWaveBank);
        break;
    case FACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED:
    case FACTNOTIFICATIONTYPE_GLOBALVARIABLECHANGED:
        Put<uint16_t>(u, n.variable.cueIndex);
        Narrow(u + 2, n.variable.pSoundBank);
        Narrow(u + 6, n.variable.pCue);
        Put<uint16_t>(u + 0xA, n.variable.variableIndex);
        Put<float>(u + 0xC, n.variable.variableValue);
        Put<int32_t>(u + 0x10, n.variable.local);
        break;
    case FACTNOTIFICATIONTYPE_GUICONNECTED:
    case FACTNOTIFICATIONTYPE_GUIDISCONNECTED:
        Put<uint32_t>(u, n.gui.reserved);
        break;
    default:   // the waves'
        Narrow(u, n.wave.pWaveBank);
        Put<uint16_t>(u + 4, n.wave.waveIndex);
        Put<uint16_t>(u + 6, n.wave.cueIndex);
        Narrow(u + 8, n.wave.pSoundBank);
        Narrow(u + 0xC, n.wave.pCue);
        Narrow(u + 0x10, n.wave.pWave);
        break;
    }
}
#endif

template <typename F, typename O>
O* Find(std::unordered_map<F*, O*>& map, F* key) {
    std::lock_guard<std::mutex> guard(g_lock);
    auto it = map.find(key);
    return it != map.end() ? it->second : nullptr;
}

// ---- IXACT3Cue

class Cue {
public:
    Cue(FACTCue* cue, SoundBank* bank) : cue_(cue), bank_(bank) {}
    FACTCue* fact() const { return cue_; }
    SoundBank* bank() const { return bank_; }

    virtual long __stdcall Play() { return long(FACTCue_Play(cue_)); }
    virtual long __stdcall Stop(uint32_t flags) { return long(FACTCue_Stop(cue_, flags)); }
    virtual long __stdcall GetState(uint32_t* state) { return long(FACTCue_GetState(cue_, state)); }
    virtual long __stdcall Destroy();
    virtual long __stdcall SetMatrixCoefficients(uint32_t sourceChannels, uint32_t destinationChannels, float* coefficients) {
        return long(FACTCue_SetMatrixCoefficients(cue_, sourceChannels, destinationChannels, coefficients));
    }
    virtual uint16_t __stdcall GetVariableIndex(const char* name) { return FACTCue_GetVariableIndex(cue_, name); }
    virtual long __stdcall SetVariable(uint16_t index, float value) { return long(FACTCue_SetVariable(cue_, index, value)); }
    virtual long __stdcall GetVariable(uint16_t index, float* value) { return long(FACTCue_GetVariable(cue_, index, value)); }
    virtual long __stdcall Pause(int32_t pause) { return long(FACTCue_Pause(cue_, pause)); }
    virtual long __stdcall GetProperties(re::Ptr32<void>* properties);
    virtual long __stdcall SetOutputVoices(const void*) { return kNotImplemented; }
    virtual long __stdcall SetOutputVoiceMatrix(void*, uint32_t, uint32_t, const float*) { return kNotImplemented; }

private:
    FACTCue* cue_;
    SoundBank* bank_;
};

Cue* AdoptCue(FACTCue* fact, SoundBank* bank) {
    if (fact == nullptr) {
        return nullptr;
    }
    Cue* cue = new Cue(fact, bank);
    std::lock_guard<std::mutex> guard(g_lock);
    g_cues[fact] = cue;
    return cue;
}

// a cue FACT destroyed (its own Destroy, or its bank's): ours goes once the notification has
// gone out
void Drop(FACTCue* fact) {
    Cue* cue = nullptr;
    {
        std::lock_guard<std::mutex> guard(g_lock);
        auto it = g_cues.find(fact);
        if (it == g_cues.end()) {
            return;
        }
        cue = it->second;
        g_cues.erase(it);
    }
    delete cue;
}

long __stdcall Cue::Destroy() {
    FACTCue* fact = cue_;
    long result = long(FACTCue_Destroy(fact));   // (the cue-destroyed notification from in here)
    Drop(fact);
    return result;
}

long __stdcall Cue::GetProperties(re::Ptr32<void>* properties) {
    if (properties == nullptr) {
        return kInvalidArgument;
    }
    FACTCueInstanceProperties* fact = nullptr;
    uint32_t result = FACTCue_GetProperties(cue_, &fact);
    if (result != 0 || fact == nullptr) {
        return long(result);
    }
    // (FACT's size: the structure with as many track entries as the sound has, at least one)
    uint16_t tracks = fact->activeVariationProperties.soundProperties.numTracks;
    size_t size = sizeof(FACTCueInstanceProperties) + sizeof(FACTTrackProperties) * (tracks > 1 ? tracks - 1 : 0);
    // (the game frees it with CoTaskMemFree; in the 64-bit build it's the low heap's, as what
    // the game's fields point at must be, and the dispatch's CoTaskMemFree knows those)
#if defined(_WIN64)
    void* copy = re::low::Alloc(size);
#else
    void* copy = CoTaskMemAlloc(size);
#endif
    if (copy != nullptr) {
        std::memcpy(copy, fact, size);
    }
    std::free(fact);   // (the engine's allocator: CreateEngine gives it the C runtime's)
    *properties = copy;
    return copy != nullptr ? kOk : kOutOfMemory;
}

// ---- IXACT3SoundBank

class SoundBank {
public:
    explicit SoundBank(FACTSoundBank* bank) : bank_(bank) {}
    FACTSoundBank* fact() const { return bank_; }

    virtual uint16_t __stdcall GetCueIndex(const char* name) { return FACTSoundBank_GetCueIndex(bank_, name); }
    virtual long __stdcall GetNumCues(uint16_t* count) { return long(FACTSoundBank_GetNumCues(bank_, count)); }
    virtual long __stdcall GetCueProperties(uint16_t index, void* properties) {
        return long(FACTSoundBank_GetCueProperties(bank_, index, static_cast<FACTCueProperties*>(properties)));
    }
    virtual long __stdcall Prepare(uint16_t index, uint32_t flags, uint32_t timeOffset, re::Ptr32<Cue>* cue) {
        FACTCue* fact = nullptr;
        long result = long(FACTSoundBank_Prepare(bank_, index, flags, int32_t(timeOffset), &fact));
        if (cue != nullptr) {
            *cue = result == kOk ? AdoptCue(fact, this) : nullptr;
        }
        return result;
    }
    virtual long __stdcall Play(uint16_t index, uint32_t flags, uint32_t timeOffset, re::Ptr32<Cue>* cue) {
        FACTCue* fact = nullptr;
        long result = long(FACTSoundBank_Play(bank_, index, flags, int32_t(timeOffset), cue != nullptr ? &fact : nullptr));
        if (cue != nullptr) {
            *cue = result == kOk ? AdoptCue(fact, this) : nullptr;
        }
        return result;
    }
    virtual long __stdcall Stop(uint16_t index, uint32_t flags) { return long(FACTSoundBank_Stop(bank_, index, flags)); }
    virtual long __stdcall Destroy();
    virtual long __stdcall GetState(uint32_t* state) { return long(FACTSoundBank_GetState(bank_, state)); }

private:
    FACTSoundBank* bank_;
};

long __stdcall SoundBank::Destroy() {
    FACTSoundBank* fact = bank_;
    // (its cues go with it, each with its notification: ours of them after)
    std::unordered_set<FACTCue*> cues;
    {
        std::lock_guard<std::mutex> guard(g_lock);
        for (auto& [key, cue] : g_cues) {
            if (cue->bank() == this) {
                cues.insert(key);
            }
        }
    }
    long result = long(FACTSoundBank_Destroy(fact));
    for (FACTCue* c : cues) {
        Drop(c);
    }
    {
        std::lock_guard<std::mutex> guard(g_lock);
        g_soundBanks.erase(fact);
    }
    delete this;
    return result;
}

// ---- IXACT3WaveBank

class WaveBank {
public:
    explicit WaveBank(FACTWaveBank* bank) : bank_(bank) {}
    FACTWaveBank* fact() const { return bank_; }

    virtual long __stdcall Destroy() {
        FACTWaveBank* fact = bank_;
        long result = long(FACTWaveBank_Destroy(fact));
        {
            std::lock_guard<std::mutex> guard(g_lock);
            g_waveBanks.erase(fact);
        }
        delete this;
        return result;
    }
    virtual long __stdcall GetNumWaves(uint16_t* count) { return long(FACTWaveBank_GetNumWaves(bank_, count)); }
    virtual uint16_t __stdcall GetWaveIndex(const char* name) { return FACTWaveBank_GetWaveIndex(bank_, name); }
    virtual long __stdcall GetWaveProperties(uint16_t index, void* properties) {
        return long(FACTWaveBank_GetWaveProperties(bank_, index, static_cast<FACTWaveProperties*>(properties)));
    }
    virtual long __stdcall Prepare(uint16_t, uint32_t, uint32_t, uint8_t, re::Ptr32<void>* wave) {
        if (wave != nullptr) {
            *wave = nullptr;
        }
        return kNotImplemented;
    }
    virtual long __stdcall Play(uint16_t, uint32_t, uint32_t, uint8_t, re::Ptr32<void>* wave) {
        if (wave != nullptr) {
            *wave = nullptr;
        }
        return kNotImplemented;
    }
    virtual long __stdcall Stop(uint16_t index, uint32_t flags) { return long(FACTWaveBank_Stop(bank_, index, flags)); }
    virtual long __stdcall GetState(uint32_t* state) { return long(FACTWaveBank_GetState(bank_, state)); }

private:
    FACTWaveBank* bank_;
};

// ---- IXACT3Engine

// XACT's default file reading (FACT's would take the handle for its own stream type)
int32_t FACTCALL ReadFileCallback(void* file, void* buffer, uint32_t size, uint32_t* read, FACTOverlapped* overlapped) {
    return ReadFile(file, buffer, size, reinterpret_cast<DWORD*>(read), reinterpret_cast<OVERLAPPED*>(overlapped));
}
int32_t FACTCALL OverlappedResultCallback(void* file, FACTOverlapped* overlapped, uint32_t* transferred, int32_t wait) {
    return GetOverlappedResult(file, reinterpret_cast<OVERLAPPED*>(overlapped), reinterpret_cast<DWORD*>(transferred), wait);
}

// the game's notification callback, given FACT's notification with our objects in it
FACTNotificationCallback g_gameCallback = nullptr;

void FACTCALL NotificationCallback(const FACTNotification* fact) {
    FACTNotificationCallback callback = g_gameCallback;
    if (callback == nullptr) {
        return;
    }
    FACTNotification n = *fact;
    switch (n.type) {
    case FACTNOTIFICATIONTYPE_CUEPREPARED:
    case FACTNOTIFICATIONTYPE_CUEPLAY:
    case FACTNOTIFICATIONTYPE_CUESTOP:
    case FACTNOTIFICATIONTYPE_CUEDESTROYED:
        n.cue.pSoundBank = reinterpret_cast<FACTSoundBank*>(Find(g_soundBanks, n.cue.pSoundBank));
        n.cue.pCue = reinterpret_cast<FACTCue*>(Find(g_cues, n.cue.pCue));
        break;
    case FACTNOTIFICATIONTYPE_MARKER:
        n.marker.pSoundBank = reinterpret_cast<FACTSoundBank*>(Find(g_soundBanks, n.marker.pSoundBank));
        n.marker.pCue = reinterpret_cast<FACTCue*>(Find(g_cues, n.marker.pCue));
        break;
    case FACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED:
        n.soundBank.pSoundBank = reinterpret_cast<FACTSoundBank*>(Find(g_soundBanks, n.soundBank.pSoundBank));
        break;
    case FACTNOTIFICATIONTYPE_WAVEBANKDESTROYED:
    case FACTNOTIFICATIONTYPE_WAVEBANKPREPARED:
    case FACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT:
        n.waveBank.pWaveBank = reinterpret_cast<FACTWaveBank*>(Find(g_waveBanks, n.waveBank.pWaveBank));
        break;
    case FACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED:
        n.variable.pSoundBank = reinterpret_cast<FACTSoundBank*>(Find(g_soundBanks, n.variable.pSoundBank));
        n.variable.pCue = reinterpret_cast<FACTCue*>(Find(g_cues, n.variable.pCue));
        break;
    default:
        break;
    }
#if defined(_M_X64)
    uint8_t game[0x20] = {};
    GameNotification(n, game);
    callback(reinterpret_cast<const FACTNotification*>(game));
#else
    callback(&n);
#endif
}

SoundBank* AdoptSoundBank(FACTSoundBank* fact);
WaveBank* AdoptWaveBank(FACTWaveBank* fact);

class Engine {
public:
    explicit Engine(FACTAudioEngine* engine) : engine_(engine) {}

    // IUnknown
    virtual long __stdcall QueryInterface(const void*, re::Ptr32<void>* out) {
        if (out != nullptr) {
            *out = nullptr;
        }
        return kNoInterface;
    }
    virtual unsigned long __stdcall AddRef() { return FACTAudioEngine_AddRef(engine_); }
    virtual unsigned long __stdcall Release() {
        unsigned long left = FACTAudioEngine_Release(engine_);
        if (left == 0) {
            delete this;
        }
        return left;
    }

    // IXACT3Engine
    virtual long __stdcall GetRendererCount(uint16_t* count) { return long(FACTAudioEngine_GetRendererCount(engine_, count)); }
    virtual long __stdcall GetRendererDetails(uint16_t index, void* details) {
        return long(FACTAudioEngine_GetRendererDetails(engine_, index, static_cast<FACTRendererDetails*>(details)));
    }
    virtual long __stdcall GetFinalMixFormat(void* format) {
        return long(FACTAudioEngine_GetFinalMixFormat(engine_, static_cast<FAudioWaveFormatExtensible*>(format)));
    }
    virtual long __stdcall Initialize(const void* parameters) {
        // (the same structure: XACT_RUNTIME_PARAMETERS. Our notification callback in the game's
        // place, and XACT's file reading where the game leaves it to XACT)
        FACTRuntimeParameters p = RuntimeParameters(parameters);
        g_gameCallback = p.fnNotificationCallback;
        p.fnNotificationCallback = &NotificationCallback;
        if (p.fileIOCallbacks.readFileCallback == nullptr) {
            p.fileIOCallbacks.readFileCallback = &ReadFileCallback;
            p.fileIOCallbacks.getOverlappedResultCallback = &OverlappedResultCallback;
        }
        return long(FACTAudioEngine_Initialize(engine_, &p));
    }
    virtual long __stdcall ShutDown() { return long(FACTAudioEngine_ShutDown(engine_)); }
    virtual long __stdcall DoWork() { return long(FACTAudioEngine_DoWork(engine_)); }
    virtual long __stdcall CreateSoundBank(const void* buffer, uint32_t size, uint32_t flags, uint32_t allocation, re::Ptr32<SoundBank>* bank) {
        FACTSoundBank* fact = nullptr;
        long result = long(FACTAudioEngine_CreateSoundBank(engine_, buffer, size, flags, allocation, &fact));
        *bank = result == kOk ? AdoptSoundBank(fact) : nullptr;
        return result;
    }
    virtual long __stdcall CreateInMemoryWaveBank(const void* buffer, uint32_t size, uint32_t flags, uint32_t allocation,
                                                  re::Ptr32<WaveBank>* bank) {
        FACTWaveBank* fact = nullptr;
        long result = long(FACTAudioEngine_CreateInMemoryWaveBank(engine_, buffer, size, flags, allocation, &fact));
        *bank = result == kOk ? AdoptWaveBank(fact) : nullptr;
        return result;
    }
    virtual long __stdcall CreateStreamingWaveBank(const void* parameters, re::Ptr32<WaveBank>* bank) {
        FACTWaveBank* fact = nullptr;
        FACTStreamingParameters p = StreamingParameters(parameters);
        long result = long(FACTAudioEngine_CreateStreamingWaveBank(engine_, &p, &fact));
        *bank = result == kOk ? AdoptWaveBank(fact) : nullptr;
        return result;
    }
    virtual long __stdcall PrepareWave(uint32_t, const char*, uint16_t, uint32_t, uint32_t, uint8_t, re::Ptr32<void>* wave) {
        *wave = nullptr;
        return kNotImplemented;
    }
    // (WAVEBANKENTRY and XACT_STREAMING_PARAMETERS are passed by value: not used, not spelled out)
    virtual long __stdcall PrepareInMemoryWave() { return kNotImplemented; }
    virtual long __stdcall PrepareStreamingWave() { return kNotImplemented; }
    virtual long __stdcall RegisterNotification(const void* description) {
        FACTNotificationDescription d = Translate(description);
        return long(FACTAudioEngine_RegisterNotification(engine_, &d));
    }
    virtual long __stdcall UnRegisterNotification(const void* description) {
        FACTNotificationDescription d = Translate(description);
        return long(FACTAudioEngine_UnRegisterNotification(engine_, &d));
    }
    virtual uint16_t __stdcall GetCategory(const char* name) { return FACTAudioEngine_GetCategory(engine_, name); }
    virtual long __stdcall Stop(uint16_t category, uint32_t flags) { return long(FACTAudioEngine_Stop(engine_, category, flags)); }
    virtual long __stdcall SetVolume(uint16_t category, float volume) {
        return long(FACTAudioEngine_SetVolume(engine_, category, volume));
    }
    virtual long __stdcall Pause(uint16_t category, int32_t pause) { return long(FACTAudioEngine_Pause(engine_, category, pause)); }
    virtual uint16_t __stdcall GetGlobalVariableIndex(const char* name) {
        return FACTAudioEngine_GetGlobalVariableIndex(engine_, name);
    }
    virtual long __stdcall SetGlobalVariable(uint16_t index, float value) {
        return long(FACTAudioEngine_SetGlobalVariable(engine_, index, value));
    }
    virtual long __stdcall GetGlobalVariable(uint16_t index, float* value) {
        return long(FACTAudioEngine_GetGlobalVariable(engine_, index, value));
    }

private:
    // a notification description with FACT's objects for ours
    static FACTNotificationDescription Translate(const void* description) {
        FACTNotificationDescription d = NotificationDescription(description);
        if (d.pSoundBank != nullptr) {
            d.pSoundBank = reinterpret_cast<SoundBank*>(d.pSoundBank)->fact();
        }
        if (d.pWaveBank != nullptr) {
            d.pWaveBank = reinterpret_cast<WaveBank*>(d.pWaveBank)->fact();
        }
        if (d.pCue != nullptr) {
            d.pCue = reinterpret_cast<Cue*>(d.pCue)->fact();
        }
        return d;
    }

    FACTAudioEngine* engine_;
};

SoundBank* AdoptSoundBank(FACTSoundBank* fact) {
    if (fact == nullptr) {
        return nullptr;
    }
    SoundBank* bank = new SoundBank(fact);
    std::lock_guard<std::mutex> guard(g_lock);
    g_soundBanks[fact] = bank;
    return bank;
}

WaveBank* AdoptWaveBank(FACTWaveBank* fact) {
    if (fact == nullptr) {
        return nullptr;
    }
    WaveBank* bank = new WaveBank(fact);
    std::lock_guard<std::mutex> guard(g_lock);
    g_waveBanks[fact] = bank;
    return bank;
}

} // namespace

long __stdcall CreateEngine(re::Ptr32<void>* engine) {
    FACTAudioEngine* fact = nullptr;
    // (the C runtime's allocator, so the cue properties FACT allocates are std::free's)
    uint32_t result = FACTCreateEngineWithCustomAllocatorEXT(0, &fact, &std::malloc, &std::free, &std::realloc);
    if (result != 0 || fact == nullptr) {
        *engine = nullptr;
        return result != 0 ? long(result) : kOutOfMemory;
    }
    *engine = new Engine(fact);
    return kOk;
}

} // namespace xact
