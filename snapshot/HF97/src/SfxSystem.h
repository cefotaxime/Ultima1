#pragma once
#include <SDL.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include "PcSpeakerEmulator.h"
#include "Apple1986Live.h"
#include "SfxOracle.h"

// M238: keep the user-approved M236 hotfix1 PC-speaker synthesis and M235
// source-synchronous sample clock.  WSL/Linux now prefers a direct asynchronous
// PulseAudio stream with explicit low-latency buffer attributes, bypassing SDL's
// PulseAudio wait/mix scheduling. Native Windows retains the M237 IAudioClient3
// path. SDL remains an automatic fallback on every platform.
class SfxSystem {
public:
    SfxSystem();
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

    // HF47: Apple II 1981 STAR.OBJ/FGT3 Front View speaker state machine.
    // Only ordinary Front View uses this; DOS SFX remain unchanged elsewhere.
    void setAppleFrontView(bool active,int speed,int hyperPhase=0,int lowSpeed=1);
    std::uint64_t appleHyperProgress() const {return appleLive_.hyperStatus();}

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

#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
    struct HostAudioBridgeState;
    struct PulseAudioState;
    bool initHostAudioBridge();
    void shutdownHostAudioBridge();
    bool sendHostAudioSound(int soundId);
    bool initPulseLowLatency();
    void shutdownPulseLowLatency();
#endif

    static void SDLCALL audioCallback(void* userdata, Uint8* stream, int len);
    void renderCallback(std::int16_t* dst,std::size_t samples);
    void waitFallbackDurationUs(std::uint32_t us);
    void waitRenderedThrough(std::uint64_t endSample,std::uint32_t watchdogUs);

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    bool audioAvailable_ = false;
    std::atomic<bool> enabled_{true};
    bool trace_ = false;
    std::uint64_t traceEpochCounter_ = 0;
    std::atomic<std::uint64_t> renderedSamples_{0};
    std::atomic<std::uint64_t> synchronousEndSample_{0};

    std::atomic<bool> appleFrontActive_{false};
    std::atomic<int> appleFrontSpeed_{1};
    std::atomic<int> appleFrontHyperPhase_{0};
    std::atomic<int> appleFrontLowSpeed_{1};
    double appleFrontLowOscPhase_=0.0;
    double appleFrontLowPulsePhase_=0.0;
    double appleFrontPulseHzSmoothed_=0.0;
    int appleFrontPreviousPhase_=0;
    bool appleBurstLevel_=false;
    int appleBurstEvent_=0;
    double appleBurstSamplesLeft_=0.0;
    double appleBurstHighpass_=0.0;
    double appleBurstPreviousRaw_=0.0;
    bool appleFrontLevel_=false;
    int appleFrontEvent_=0;
    int appleFrontCycleSpeed_=1;
    bool appleFrontHighCycleOn_=true;
    double appleFrontSamplesLeft_=0.0;
    double appleFrontBpX1_=0.0, appleFrontBpX2_=0.0;
    double appleFrontBpY1_=0.0, appleFrontBpY2_=0.0;

    Apple1986Live appleLive_;
    PcSpeakerEmulator speaker_;
    std::mutex voicesMutex_;
    std::vector<Voice> voices_;

#ifdef _WIN32
    std::unique_ptr<WinAudioState> winAudio_;
#endif
#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
    std::unique_ptr<HostAudioBridgeState> hostAudio_;
    std::unique_ptr<PulseAudioState> pulseAudio_;
#endif
};