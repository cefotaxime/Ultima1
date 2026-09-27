#pragma once
#include <SDL.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "SfxOracle.h"

// M227 sound presentation layer.
//
// Presentation PCM is extracted directly from the user's DOSBox Staging
// 0.82.2 captures. The audible impulse/filter tail is intentionally retained.
// Original DOS sound() calls are synchronous, but their blocking time is
// shorter than that audible tail. M227 therefore keeps two independent clocks:
//   * a source/logical clock used to sequence original sound() calls; and
//   * the callback PCM clock used only to render overlapping audible tails.
//
// In particular, a new sound() call does NOT wait for the previous capture PCM
// tail to finish. The tails overlap in the callback mixer, while source calls
// are separated only by logicalDurationMs().
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

    static void SDLCALL audioCallback(void* userdata, Uint8* stream, int len);
    void mix(Uint8* stream, int len);
    void schedule(int soundId, bool immediate);
    void clearMixerState();
    void advanceLogicalDeadline(std::uint32_t durationMs);

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    bool enabled_ = false;
    std::vector<Voice> voices_;
    std::uint64_t renderedSamples_ = 0;
    std::uint64_t sourceCursorSample_ = 0;
    std::uint32_t logicalBusyUntilMs_ = 0;
};
