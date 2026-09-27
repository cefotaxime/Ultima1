#include "SfxSystem.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
constexpr std::uint32_t REQUESTED_SAMPLES=64;
std::uint64_t perfUs(){
    const auto f=SDL_GetPerformanceFrequency();
    const auto c=SDL_GetPerformanceCounter();
    return f?static_cast<std::uint64_t>((c*1000000ull)/f):0ull;
}
}

SfxSystem::~SfxSystem(){ shutdown(); }

bool SfxSystem::init(){
    if(device_) return true;
    SDL_setenv("PULSE_LATENCY_MSEC","5",0);
    if((SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)==0){
        if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0) return false;
    }

    const char* tr=std::getenv("U1_SFX_TRACE");
    trace_=(tr && *tr && *tr!='0');
    traceEpochCounter_=perfUs();

    SDL_AudioSpec want{};
    want.freq=PcSpeakerEmulator::SAMPLE_RATE;
    want.format=AUDIO_S16SYS;
    want.channels=1;
    want.samples=REQUESTED_SAMPLES;
    want.callback=&SfxSystem::audioCallback;
    want.userdata=this;
    device_=SDL_OpenAudioDevice(nullptr,0,&want,&spec_,0);
    if(!device_){ spec_=SDL_AudioSpec{}; return false; }

    enabled_=false;
    renderedSamples_.store(0,std::memory_order_release);
    synchronousEndSample_.store(0,std::memory_order_release);
    voices_.clear();
    SDL_PauseAudioDevice(device_,0);

    std::cerr << "PC speaker emulator: driver="
              << (SDL_GetCurrentAudioDriver()?SDL_GetCurrentAudioDriver():"unknown")
              << ", freq=" << spec_.freq
              << ", samples=" << spec_.samples
              << ", source-synchronous=M235-callback-clock"
              << ", transport=SDL-direct-callback"
              << ", app-queue=none"
              << ", trace=" << (trace_?"on":"off") << "\n";
    return true;
}

void SfxSystem::shutdown(){
    if(device_){
        SDL_LockAudioDevice(device_);
        enabled_=false;
        voices_.clear();
        SDL_UnlockAudioDevice(device_);
        SDL_CloseAudioDevice(device_);
        device_=0;
    }
}

void SfxSystem::setEnabled(bool enabled){
    if(!device_){ enabled_=enabled; return; }
    SDL_LockAudioDevice(device_);
    enabled_=enabled;
    if(!enabled_) voices_.clear();
    SDL_UnlockAudioDevice(device_);
}

void SDLCALL SfxSystem::audioCallback(void* userdata,Uint8* stream,int len){
    auto* self=static_cast<SfxSystem*>(userdata);
    if(!self || !stream || len<=0) return;
    auto* dst=reinterpret_cast<std::int16_t*>(stream);
    self->renderCallback(dst,static_cast<std::size_t>(len)/sizeof(std::int16_t));
}

void SfxSystem::renderCallback(std::int16_t* dst,std::size_t samples){
    std::fill(dst,dst+samples,static_cast<std::int16_t>(0));
    if(enabled_){
        for(std::size_t i=0;i<samples;++i){
            const std::uint64_t sample=renderedSamples_.load(std::memory_order_relaxed)+i;
            std::int32_t mixed=0;
            for(const auto& v:voices_){
                if(sample<v.startSample) continue;
                const auto off=sample-v.startSample;
                if(off>=v.samples) continue;
                mixed+=v.data[static_cast<std::size_t>(off)];
            }
            dst[i]=static_cast<std::int16_t>(std::clamp<std::int32_t>(
                mixed,std::numeric_limits<std::int16_t>::min(),
                      std::numeric_limits<std::int16_t>::max()));
        }
    }
    const auto renderedEnd=renderedSamples_.fetch_add(samples,std::memory_order_release)+samples;
    voices_.erase(std::remove_if(voices_.begin(),voices_.end(),[&](const Voice& v){
        return v.startSample+v.samples<=renderedEnd;
    }),voices_.end());
}

void SfxSystem::waitFallbackDurationUs(std::uint32_t us){
    const auto start=perfUs();
    const auto end=start+us;
    while(true){
        const auto now=perfUs();
        if(now>=end) break;
        const auto remain=end-now;
        if(remain>1500) SDL_Delay(1);
        else SDL_Delay(0);
    }
}

void SfxSystem::waitRenderedThrough(std::uint64_t endSample,std::uint32_t watchdogUs){
    const auto watchdogEnd=perfUs()+static_cast<std::uint64_t>(watchdogUs);
    while(enabled_ && renderedSamples_.load(std::memory_order_acquire)<endSample){
        if(perfUs()>=watchdogEnd){
            std::cerr << "PC speaker emulator: callback clock stalled at "
                      << renderedSamples_.load(std::memory_order_acquire)
                      << " waiting for " << endSample << "\n";
            break;
        }
        SDL_Delay(0);
    }
}

void SfxSystem::play(int soundId){
    if(soundId<0 || soundId>9 || !enabled_) return;
    const auto us=speaker_.sourceDurationUs(soundId);
    if(!device_){
        waitFallbackDurationUs(us);
        return;
    }

    const auto& clip=speaker_.clip(soundId);
    std::uint64_t startSample=0;
    std::uint64_t sourceEndSample=0;
    SDL_LockAudioDevice(device_);
    startSample=renderedSamples_.load(std::memory_order_acquire);
    sourceEndSample=startSample+static_cast<std::uint64_t>(clip.sourceSamples);
    synchronousEndSample_.store(sourceEndSample,std::memory_order_release);
    if(!clip.pcm.empty()) voices_.push_back(Voice{clip.pcm.data(),clip.pcm.size(),startSample});
    SDL_UnlockAudioDevice(device_);

    if(trace_){
        std::cerr << "[SFXTRACE] t_us=" << (perfUs()-traceEpochCounter_)
                  << " play_begin id=" << soundId
                  << " rendered=" << startSample
                  << " source_end=" << sourceEndSample
                  << " callback_frames=" << spec_.samples
                  << " app_queue_samples=0\n";
    }

    waitRenderedThrough(sourceEndSample,us+1000000u);

    if(trace_){
        const auto rendered=renderedSamples_.load(std::memory_order_acquire);
        std::cerr << "[SFXTRACE] t_us=" << (perfUs()-traceEpochCounter_)
                  << " play_end id=" << soundId
                  << " rendered=" << rendered
                  << " source_end=" << sourceEndSample
                  << " source_us=" << us
                  << " app_queue_samples=0\n";
    }
}

bool SfxSystem::logicalBusy() const{
    return enabled_ && renderedSamples_.load(std::memory_order_acquire) <
                       synchronousEndSample_.load(std::memory_order_acquire);
}

std::uint32_t SfxSystem::logicalDurationMs(int soundId) const{
    return speaker_.sourceDurationMsCeil(soundId);
}
