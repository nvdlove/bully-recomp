// FAudio's FACT (third_party/FAudio) against the game's XACT content: the global settings
// (audio/xact/ALRMCLK.xgs) start an engine, and every .sea's sound bank and wave bank are
// created from memory, each bank's cues counted and its first cue prepared and destroyed.
// Nothing is played (the engine opens the default device, silent).
//
// A .sea: int32 [0] the sound bank's offset, [1] its size; the wave bank follows it, [2] its
// size; then the game's own tables.
//
// bullyre_xacttest.exe [<game folder>]     a line per failure and a summary; exit code 1 when
//                                          any bank failed

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "FACT.h"

namespace {

std::vector<uint8_t> ReadFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {});
}

} // namespace

int main(int argc, char** argv) {
    std::filesystem::path folder = argc > 1 ? argv[1] : R"(C:\Program Files (x86)\Steam\steamapps\common\Bully Scholarship Edition)";
    std::filesystem::path xact = folder / "audio" / "xact";
    std::vector<uint8_t> settings = ReadFile(xact / "ALRMCLK.xgs");
    FACTAudioEngine* engine = nullptr;
    if (FACTCreateEngine(0, &engine) != 0) {
        std::printf("FACTCreateEngine failed\n");
        return 1;
    }
    FACTRuntimeParameters parameters = {};
    parameters.lookAheadTime = 0xFA;
    parameters.pGlobalSettingsBuffer = settings.data();
    parameters.globalSettingsBufferSize = uint32_t(settings.size());
    uint32_t hr = FACTAudioEngine_Initialize(engine, &parameters);
    if (hr != 0) {
        std::printf("Initialize with ALRMCLK.xgs failed: 0x%08X\n", hr);
        return 1;
    }
    int banks = 0, failed = 0, cues = 0, prepared = 0;
    for (const auto& entry : std::filesystem::directory_iterator(xact)) {
        std::string ext = entry.path().extension().string();
        if (_stricmp(ext.c_str(), ".sea") != 0) {
            continue;
        }
        banks++;
        std::vector<uint8_t> sea = ReadFile(entry.path());
        const int32_t* header = reinterpret_cast<const int32_t*>(sea.data());
        uint32_t sbOffset = uint32_t(header[0]), sbSize = uint32_t(header[1]), wbSize = uint32_t(header[2]);
        uint32_t wbOffset = sbOffset + sbSize;
        std::string name = entry.path().filename().string();
        if (sea.size() < 16 || wbOffset + wbSize > sea.size()) {
            std::printf("%-20s bad header\n", name.c_str());
            failed++;
            continue;
        }
        FACTWaveBank* waveBank = nullptr;
        FACTSoundBank* soundBank = nullptr;
        uint32_t wr = FACTAudioEngine_CreateInMemoryWaveBank(engine, sea.data() + wbOffset, wbSize, 0, 0, &waveBank);
        uint32_t sr = FACTAudioEngine_CreateSoundBank(engine, sea.data() + sbOffset, sbSize, 0, 0, &soundBank);
        if (wr != 0 || sr != 0) {
            std::printf("%-20s wave bank 0x%08X, sound bank 0x%08X\n", name.c_str(), wr, sr);
            failed++;
        } else {
            uint16_t count = 0;
            FACTSoundBank_GetNumCues(soundBank, &count);
            cues += count;
            FACTCue* cue = nullptr;
            if (count > 0 && FACTSoundBank_Prepare(soundBank, 0, 0, 0, &cue) == 0 && cue != nullptr) {
                prepared++;
                FACTCue_Destroy(cue);
            } else if (count > 0) {
                std::printf("%-20s cue 0 didn't prepare\n", name.c_str());
            }
        }
        if (soundBank != nullptr) {
            FACTSoundBank_Destroy(soundBank);
        }
        if (waveBank != nullptr) {
            FACTWaveBank_Destroy(waveBank);
        }
        FACTAudioEngine_DoWork(engine);
    }
    FACTAudioEngine_ShutDown(engine);
    FACTAudioEngine_Release(engine);
    std::printf("%d banks: %d failed; %d cues, the first of %d banks prepared\n", banks, failed, cues, prepared);
    return failed == 0 ? 0 : 1;
}
