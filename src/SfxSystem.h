#pragma once
#include <SDL.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "SfxOracle.h"

// M228 sound presentation layer.
//
// M225 proved that SDL's queued-audio device works in the user's WSLg setup,
// while M227's callback-device path produced complete silence there. M228
// returns to SDL_QueueAudio, but NOT to M225's full-WAV FIFO semantics.
//
// Every queued capture WAV contains a DOSBox impulse/filter tail that may remain
// audible after the original synchronous sound() routine has already returned.
// Before adding a new source call, M228 takes the still-unplayed suffix of our
// queued PCM shadow, mixes the new capture into that suffix at the original
// logical sound() start offset, clears only SDL's replaceable queue, and queues
// the remixed suffix again.
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
    void playImmediate(int soundId);
    void playImmediate(SfxEvent event) { playImmediate(sfxId(event)); }

    std::uint32_t logicalDurationMs(int soundId) const;
    std::uint32_t logicalDurationMs(SfxEvent event) const { return logicalDurationMs(sfxId(event)); }
    bool logicalBusy() const;

private:
    void schedule(int soundId);
    void clearQueueState();
    std::uint32_t remainingLogicalMs(std::uint32_t now) const;

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    bool enabled_ = false;
    std::vector<std::int16_t> queuedMix_;
    std::uint32_t logicalBusyUntilMs_ = 0;
};
