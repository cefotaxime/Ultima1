#pragma once
#include <array>
#include <cstdint>
#include <vector>

class PcSpeakerEmulator {
public:
    static constexpr int SAMPLE_RATE = 48000;
    struct Clip {
        std::vector<std::int16_t> pcm;
        std::uint32_t sourceSamples = 0;
    };
    PcSpeakerEmulator();
    const Clip& clip(int soundId) const;
    std::uint32_t sourceDurationUs(int soundId) const;
    std::uint32_t sourceDurationMsCeil(int soundId) const;
private:
    static Clip render(int soundId);
    std::array<Clip,10> clips_{};
};
