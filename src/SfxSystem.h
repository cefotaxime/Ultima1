#pragma once
#include <SDL.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "PcSpeakerEmulator.h"
#include "SfxOracle.h"

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

    static int SDLCALL feederEntry(void* userdata);
    int feederLoop();
    void pumpLocked();
    std::uint64_t playedSamplesLocked() const;
    std::uint64_t scheduleLocked(int soundId);

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    SDL_mutex* mutex_ = nullptr;
    SDL_cond* progress_ = nullptr;
    SDL_Thread* feeder_ = nullptr;
    bool feederRun_ = false;
    bool enabled_ = false;

    PcSpeakerEmulator speaker_;
    std::vector<Voice> voices_;
    std::uint64_t producedSamples_ = 0;
    std::uint64_t sourceCursorSample_ = 0;
    std::uint64_t synchronousEndSample_ = 0;
    bool trace_ = false;
    std::uint64_t traceEpochCounter_ = 0;
};
