#pragma once
#include <SDL.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "PcSpeakerEmulator.h"
#include "SfxOracle.h"

// M234: direct SDL pull-callback transport. There is no application PCM queue,
// feeder thread, or idle-silence prefill. The callback only renders whatever
// voice is active when SDL asks for the next device block.
class SfxSystem {
public:
    SfxSystem() = default;
    ~SfxSystem();

    SfxSystem(const SfxSystem&) = delete;
    SfxSystem& operator=(const SfxSystem&) = delete;

    bool init();
    void shutdown();
    void setEnabled(bool enabled);
    bool enabled() const { return enabled_; }
    bool available() const { return device_ != 0; }

    void play(int soundId);
    void play(SfxEvent event) { play(sfxId(event)); }
    void playImmediate(int soundId) { play(soundId); }
    void playImmediate(SfxEvent event) { play(sfxId(event)); }

    std::uint32_t logicalDurationMs(int soundId) const;
    std::uint32_t logicalDurationMs(SfxEvent event) const { return logicalDurationMs(sfxId(event)); }
    bool logicalBusy() const;

private:
    struct Voice {
        const std::int16_t* data = nullptr;
        std::size_t samples = 0;
        std::uint64_t startSample = 0;
    };

    static void SDLCALL audioCallback(void* userdata, Uint8* stream, int len);
    void renderCallback(std::int16_t* dst,std::size_t samples);
    void waitSourceDurationUs(std::uint32_t us);

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    bool enabled_ = false;
    bool trace_ = false;
    std::uint64_t traceEpochCounter_ = 0;
    std::uint64_t renderedSamples_ = 0;
    std::uint64_t logicalBusyUntilUs_ = 0;

    PcSpeakerEmulator speaker_;
    std::vector<Voice> voices_;
};
