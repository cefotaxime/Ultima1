#pragma once
#include <SDL.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include "PcSpeakerEmulator.h"
#include "SfxOracle.h"

// M237: keep the M234 DOSBox-style impulse renderer and M235 source-synchronous
// sample clock, but on native Windows prefer an event-driven IAudioClient3
// shared stream at the endpoint's minimum supported engine period.  The older
// SDL pull-callback path remains the automatic fallback and the WSL/Linux path.
// There is still no application PCM queue, feeder thread, runtime WAV playback,
// or idle-silence prefill.
class SfxSystem {
public:
    SfxSystem() = default;
    ~SfxSystem();

    SfxSystem(const SfxSystem&) = delete;
    SfxSystem& operator=(const SfxSystem&) = delete;

    bool init();
    void shutdown();
    void setEnabled(bool enabled);
    bool enabled() const { return enabled_.load(std::memory_order_acquire); }
    bool available() const { return audioAvailable_; }

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

#ifdef _WIN32
    struct WinAudioState;
    bool initWindowsLowLatency();
    void shutdownWindowsLowLatency();
#endif

    static void SDLCALL audioCallback(void* userdata, Uint8* stream, int len);
    void renderCallback(std::int16_t* dst,std::size_t samples);
    void waitFallbackDurationUs(std::uint32_t us);
    void waitRenderedThrough(std::uint64_t endSample,std::uint32_t watchdogUs);

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    bool audioAvailable_ = false;
    std::atomic<bool> enabled_{false};
    bool trace_ = false;
    std::uint64_t traceEpochCounter_ = 0;
    std::atomic<std::uint64_t> renderedSamples_{0};
    std::atomic<std::uint64_t> synchronousEndSample_{0};

    PcSpeakerEmulator speaker_;
    std::mutex voicesMutex_;
    std::vector<Voice> voices_;

#ifdef _WIN32
    std::unique_ptr<WinAudioState> winAudio_;
#endif
};
