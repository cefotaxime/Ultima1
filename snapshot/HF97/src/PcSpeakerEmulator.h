#pragma once
#include <array>
#include <cstdint>
#include <vector>

// Source-level reconstruction of OUT.EXE's ten direct port-61h speaker
// routines.  The routine topology (loop counts and toggle order) comes from
// the original bytes at file 0xB065..0xB14E.  Absolute source durations are
// calibrated to the user's DOSBox Staging 0.82.2 / cycles=3000 captures; the
// longer audible decay is produced by the speaker/filter model and is NOT part
// of the synchronous sound() return time.
class PcSpeakerEmulator {
public:
    static constexpr int SAMPLE_RATE = 48000;

    struct Clip {
        std::vector<std::int16_t> pcm;
        std::uint32_t sourceSamples = 0; // original synchronous routine span
    };

    PcSpeakerEmulator();

    const Clip& clip(int soundId) const;
    std::uint32_t sourceDurationUs(int soundId) const;
    std::uint32_t sourceDurationMsCeil(int soundId) const;

private:
    static Clip render(int soundId);
    std::array<Clip,10> clips_{};
};