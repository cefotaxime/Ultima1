#pragma once
#include <SDL.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "SfxOracle.h"

// M229 sound presentation layer.
//
// M228 still rewrote SDL's queued audio on every new sound call. On WSLg this
// can starve/restart the host queue often enough that several footsteps become
// audible only after the visible movement has already completed.
//
// M229 keeps the M225-proven SDL_QueueAudio backend but turns it into a tiny
// continuously-fed software mixer. A feeder thread keeps only a few ms of
// audio queued. New sound() calls are inserted into the source timeline at the
// original logical-duration spacing, while the longer capture/filter tails are
// mixed together when future chunks are produced.
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
    struct Voice {
        const std::int16_t* data = nullptr;
        std::size_t samples = 0;
        std::uint64_t startSample = 0;
    };

    static int SDLCALL feederEntry(void* userdata);
    int feederLoop();
    void schedule(int soundId);
    void pumpLocked();
    std::uint64_t playedSamplesLocked() const;
    std::uint32_t remainingLogicalMs(std::uint32_t now) const;
    void resetLocked(bool clearDeviceQueue);

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    SDL_mutex* mutex_ = nullptr;
    SDL_Thread* feeder_ = nullptr;
    bool feederRun_ = false;
    bool enabled_ = false;

    std::vector<Voice> voices_;
    std::uint64_t producedSamples_ = 0;
    std::uint64_t sourceCursorSample_ = 0;
    std::uint32_t logicalBusyUntilMs_ = 0;
};
