#pragma once
#include <SDL.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "PcSpeakerEmulator.h"
#include "SfxOracle.h"

// M230: source-synchronous PC-speaker emulation.
//
// OUT.EXE does not enqueue a WAV and continue.  Its sound() dispatcher enters a
// software port-61h routine and does not return until that routine has finished.
// M230 restores that contract.  PcSpeakerEmulator reconstructs the ten original
// toggle programs; a continuously-running SDL stream keeps the WSLg/PulseAudio
// path warm; play() blocks until the audio timeline has consumed the original
// source-routine span.  Filter ringing can continue after play() returns.
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

    // Both entry points are synchronous, matching OUT.EXE sound().
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
};
