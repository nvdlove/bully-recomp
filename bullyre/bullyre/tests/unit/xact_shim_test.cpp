// The XACT shim (src/game/XactShim.cpp) driven the way the game's MDXact code drives XACT:
// through the vtable slots it calls (MDXactManager.cpp, MDXactObjects.cpp), with its runtime
// parameters and its cue-destroyed notification. Every .sea's banks are created, a cue of
// each prepared, looked at (state, properties freed with CoTaskMemFree, a variable set) and
// destroyed; the notifications must name our cues. Nothing is played.
//
// bullyre_xactshimtest.exe [<game folder>]

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <windows.h>
#include <objbase.h>

#include "game/XactShim.h"

namespace {

std::vector<uint8_t> ReadFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {});
}

void** Vtable(void* object) {
    return *static_cast<void***>(object);
}

template <typename F, typename... A>
auto Call(void* object, int slot, A... args) {
    return reinterpret_cast<F>(Vtable(object)[slot])(object, args...);
}

int g_destroyed = 0, g_wrongCue = 0;
void* g_expectedCue = nullptr;

// the game's callback's shape (0x5ACD90): the cue at +0xF
void __stdcall OnNotification(const uint8_t* n) {
    if (n[0] == 4) {   // cue destroyed
        g_destroyed++;
        void* cue;
        std::memcpy(&cue, n + 0xF, sizeof(cue));
        if (cue != g_expectedCue) {
            g_wrongCue++;
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    std::filesystem::path folder = argc > 1 ? argv[1] : R"(C:\Program Files (x86)\Steam\steamapps\common\Bully Scholarship Edition)";
    std::filesystem::path xact = folder / "audio" / "xact";
    CoInitializeEx(nullptr, 0);
    std::vector<uint8_t> settings = ReadFile(xact / "ALRMCLK.xgs");

    void* engine = nullptr;
    if (xact::CreateEngine(&engine) < 0 || engine == nullptr) {
        std::printf("CreateEngine failed\n");
        return 1;
    }
    alignas(4) uint8_t details[0x400] = {};
    using GetRendererDetailsFn = long(__stdcall*)(void*, uint32_t, void*);
    long hr = Call<GetRendererDetailsFn>(engine, 4, 0u, static_cast<void*>(details));
    std::printf("GetRendererDetails: 0x%08lX\n", hr);
    // (MDXactManager.cpp's Xact_Init)
    alignas(4) uint8_t parameters[0x3C] = {};
    *reinterpret_cast<uint32_t*>(parameters + 0) = 0xFA;
    *reinterpret_cast<void**>(parameters + 4) = settings.data();
    *reinterpret_cast<uint32_t*>(parameters + 8) = uint32_t(settings.size());
    *reinterpret_cast<uint32_t*>(parameters + 0x10) = 0x99040000;
    *reinterpret_cast<void**>(parameters + 0x1C) = reinterpret_cast<void*>(&OnNotification);
    *reinterpret_cast<void**>(parameters + 0x20) = details;
    using InitializeFn = long(__stdcall*)(void*, void*);
    hr = Call<InitializeFn>(engine, 6, static_cast<void*>(parameters));
    if (hr < 0) {
        std::printf("Initialize failed: 0x%08lX\n", hr);
        return 1;
    }
    alignas(4) uint8_t notification[0x1A] = {};
    notification[0] = 4;
    notification[1] = 1;
    *reinterpret_cast<uint16_t*>(notification + 0x12) = 0xFFFF;
    using RegisterFn = long(__stdcall*)(void*, void*);
    hr = Call<RegisterFn>(engine, 15, static_cast<void*>(notification));
    std::printf("RegisterNotification: 0x%08lX\n", hr);

    using CreateBankFn = long(__stdcall*)(void*, const void*, uint32_t, uint32_t, uint32_t, void**);
    using ComFn = long(__stdcall*)(void*);
    using GetCueIndexFn = uint16_t(__stdcall*)(void*, const char*);
    using GetCuePropertiesFn = long(__stdcall*)(void*, uint32_t, void*);
    using PrepareFn = long(__stdcall*)(void*, uint32_t, uint32_t, uint32_t, void**);
    using GetStateFn = long(__stdcall*)(void*, uint32_t*);
    using GetPropertiesFn = long(__stdcall*)(void*, uint8_t**);
    using SetVariableFn = long(__stdcall*)(void*, uint32_t, float);
    using StopFn = long(__stdcall*)(void*, uint32_t);
    int banks = 0, failed = 0, cues = 0;
    for (const auto& entry : std::filesystem::directory_iterator(xact)) {
        if (_stricmp(entry.path().extension().string().c_str(), ".sea") != 0) {
            continue;
        }
        banks++;
        std::vector<uint8_t> sea = ReadFile(entry.path());
        const int32_t* header = reinterpret_cast<const int32_t*>(sea.data());
        uint32_t sbOffset = uint32_t(header[0]), sbSize = uint32_t(header[1]), wbSize = uint32_t(header[2]);
        void* waveBank = nullptr;
        void* soundBank = nullptr;
        long w = Call<CreateBankFn>(engine, 10, static_cast<const void*>(sea.data() + sbOffset + sbSize), wbSize, 0u, 0u, &waveBank);
        long s = Call<CreateBankFn>(engine, 9, static_cast<const void*>(sea.data() + sbOffset), sbSize, 0u, 0u, &soundBank);
        if (w != 0 || s != 0) {
            std::printf("%s: wave bank 0x%08lX, sound bank 0x%08lX\n", entry.path().filename().string().c_str(), w, s);
            failed++;
            continue;
        }
        // the cue named in the .sea's own table (its first entry's name, after a 4-byte hash
        // at +0x18), else cue 0
        const char* name = reinterpret_cast<const char*>(sea.data() + 0x1C);
        uint16_t index = Call<GetCueIndexFn>(soundBank, 0, name);
        if (index == 0xFFFF) {
            index = 0;
        }
        alignas(4) uint8_t cueProperties[0x200] = {};
        Call<GetCuePropertiesFn>(soundBank, 2, uint32_t(index), static_cast<void*>(cueProperties));
        void* cue = nullptr;
        if (Call<PrepareFn>(soundBank, 3, uint32_t(index), 0u, 0u, &cue) == 0 && cue != nullptr) {
            cues++;
            uint32_t state = 0;
            Call<GetStateFn>(cue, 2, &state);
            uint8_t* properties = nullptr;
            if (Call<GetPropertiesFn>(cue, 9, &properties) >= 0 && properties != nullptr) {
                uint16_t tracks = *reinterpret_cast<uint16_t*>(properties + 0x12C);
                (void)tracks;
                CoTaskMemFree(properties);
            } else {
                std::printf("%s: GetProperties failed\n", entry.path().filename().string().c_str());
                failed++;
            }
            Call<SetVariableFn>(cue, 6, 0u, 0.5f);
            Call<StopFn>(cue, 1, 1u);
            g_expectedCue = cue;
            Call<ComFn>(cue, 3);   // Destroy: the notification names it
            g_expectedCue = nullptr;
        }
        uint32_t state = 0;
        Call<GetStateFn>(soundBank, 7, &state);
        Call<ComFn>(soundBank, 6);   // Destroy
        Call<ComFn>(waveBank, 0);    // Destroy
        Call<ComFn>(engine, 8);      // DoWork
    }
    Call<ComFn>(engine, 7);   // ShutDown
    Call<ComFn>(engine, 2);   // Release
    std::printf("%d banks: %d failed; %d cues prepared and destroyed, %d cue-destroyed notifications (%d naming another cue)\n",
                banks, failed, cues, g_destroyed, g_wrongCue);
    return failed == 0 && g_destroyed == cues && g_wrongCue == 0 ? 0 : 1;
}
