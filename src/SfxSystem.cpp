#include "SfxSystem.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>
#include <cstdlib>\n#include <chrono>\n
namespace {
constexpr std::uint32_t FEEDER_SLEEP_MS=1;
constexpr std::uint32_t REQUESTED_SAMPLES=32;
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
    SDL_AudioSpec want{};
    want.freq=PcSpeakerEmulator::SAMPLE_RATE;
    want.format=AUDIO_S16SYS;
    want.channels=1;
    want.samples=REQUESTED_SAMPLES;
    want.callback=nullptr;
    device_=SDL_OpenAudioDevice(nullptr,0,&want,&spec_,0);
    if(!device_){ spec_=SDL_AudioSpec{}; return false; }

    mutex_=SDL_CreateMutex();
    progress_=SDL_CreateCond();
    if(!mutex_ || !progress_){
        if(progress_) SDL_DestroyCond(progress_);
        if(mutex_) SDL_DestroyMutex(mutex_);
        progress_=nullptr; mutex_=nullptr;
        SDL_CloseAudioDevice(device_); device_=0;
        return false;
    }

    feederRun_=true;
    enabled_=false;
    const char* tr=std::getenv("U1_SFX_TRACE");
    trace_=(tr && *tr && *tr!='0');
    traceEpochCounter_=perfUs();
    producedSamples_=0;
    sourceCursorSample_=0;
    synchronousEndSample_=0;
    voices_.clear();
    SDL_PauseAudioDevice(device_,0);
    feeder_=SDL_CreateThread(&SfxSystem::feederEntry,"u1kr-pcspk",this);
    if(!feeder_){
        feederRun_=false;
        SDL_DestroyCond(progress_); progress_=nullptr;
        SDL_DestroyMutex(mutex_); mutex_=nullptr;
        SDL_CloseAudioDevice(device_); device_=0;
        return false;
    }

    std::cerr << "PC speaker emulator: driver="
              << (SDL_GetCurrentAudioDriver()?SDL_GetCurrentAudioDriver():"unknown")
              << ", freq=" << spec_.freq
              << ", samples=" << spec_.samples
              << ", source-synchronous=M233"
              << ", idle-silence-prefill=off, transport=M231-stable"
              << ", trace=" << (trace_?"on":"off") << "\n";
    return true;
}

void SfxSystem::shutdown(){
    if(feeder_){
        SDL_LockMutex(mutex_);
        feederRun_=false;
        SDL_CondBroadcast(progress_);
        SDL_UnlockMutex(mutex_);
        SDL_WaitThread(feeder_,nullptr);
        feeder_=nullptr;
    }
    if(device_) SDL_ClearQueuedAudio(device_);
    if(progress_){ SDL_DestroyCond(progress_); progress_=nullptr; }
    if(mutex_){ SDL_DestroyMutex(mutex_); mutex_=nullptr; }
    if(device_){ SDL_CloseAudioDevice(device_); device_=0; }
    enabled_=false;
    voices_.clear();
}

void SfxSystem::setEnabled(bool enabled){
    if(!mutex_){ enabled_=enabled; return; }
    SDL_LockMutex(mutex_);
    enabled_=enabled;
    if(!enabled_){
        voices_.clear();
        const auto played=playedSamplesLocked();
        sourceCursorSample_=std::max(producedSamples_,played);
        synchronousEndSample_=played;
    }else{
        const auto played=playedSamplesLocked();
        sourceCursorSample_=std::max(producedSamples_,played);
        synchronousEndSample_=played;
    }
    SDL_CondBroadcast(progress_);
    SDL_UnlockMutex(mutex_);
}

std::uint64_t SfxSystem::playedSamplesLocked() const{
    if(!device_) return producedSamples_;
    const std::uint64_t queued=
        static_cast<std::uint64_t>(SDL_GetQueuedAudioSize(device_))/sizeof(std::int16_t);
    return queued>=producedSamples_?0:producedSamples_-queued;
}

std::uint64_t SfxSystem::scheduleLocked(int soundId){
    const auto& clip=speaker_.clip(soundId);
    if(clip.pcm.empty() || clip.sourceSamples==0) return playedSamplesLocked();
    const std::uint64_t played=playedSamplesLocked();
    std::uint64_t start=std::max(sourceCursorSample_,producedSamples_);
    if(start<played) start=played;
    voices_.push_back(Voice{clip.pcm.data(),clip.pcm.size(),start});
    sourceCursorSample_=start+clip.sourceSamples;
    synchronousEndSample_=sourceCursorSample_;
    if(trace_){
        const auto queued=static_cast<std::uint64_t>(SDL_GetQueuedAudioSize(device_))/sizeof(std::int16_t);
        std::cerr << "[SFXTRACE] t_us=" << (perfUs()-traceEpochCounter_)
                  << " schedule id=" << soundId
                  << " played=" << played
                  << " produced=" << producedSamples_
                  << " queued=" << queued
                  << " start=" << start
                  << " source_end=" << synchronousEndSample_
                  << " clip_samples=" << clip.pcm.size() << "\n";
    }
    return synchronousEndSample_;
}

void SfxSystem::play(int soundId){
    if(soundId<0 || soundId>9 || !enabled_) return;
    if(!device_ || !mutex_){
        SDL_Delay(logicalDurationMs(soundId));
        return;
    }
    SDL_LockMutex(mutex_);
    if(trace_){
        std::cerr << "[SFXTRACE] t_us=" << (perfUs()-traceEpochCounter_)
                  << " play_begin id=" << soundId
                  << " queued_bytes=" << SDL_GetQueuedAudioSize(device_) << "\n";
    }
    const std::uint64_t endSample=scheduleLocked(soundId);
    SDL_CondBroadcast(progress_);
    const std::uint32_t watchdogStart=SDL_GetTicks();
    const std::uint32_t watchdogMs=logicalDurationMs(soundId)+1000u;
    while(feederRun_ && enabled_ && playedSamplesLocked()<endSample){
        SDL_CondWaitTimeout(progress_,mutex_,5);
        if(static_cast<std::uint32_t>(SDL_GetTicks()-watchdogStart)>watchdogMs){
            std::cerr << "PC speaker emulator: audio clock stalled on sound(" << soundId << ")\n";
            break;
        }
    }
    if(trace_){
        std::cerr << "[SFXTRACE] t_us=" << (perfUs()-traceEpochCounter_)
                  << " play_end id=" << soundId
                  << " played=" << playedSamplesLocked()
                  << " produced=" << producedSamples_
                  << " queued_bytes=" << SDL_GetQueuedAudioSize(device_) << "\n";
    }
    SDL_UnlockMutex(mutex_);
}

bool SfxSystem::logicalBusy() const{
    if(!enabled_ || !device_ || !mutex_) return false;
    SDL_LockMutex(mutex_);
    const bool busy=playedSamplesLocked()<synchronousEndSample_;
    SDL_UnlockMutex(mutex_);
    return busy;
}

int SDLCALL SfxSystem::feederEntry(void* userdata){
    return static_cast<SfxSystem*>(userdata)->feederLoop();
}

int SfxSystem::feederLoop(){
    for(;;){
        SDL_LockMutex(mutex_);
        const bool run=feederRun_;
        if(run) pumpLocked();
        SDL_CondBroadcast(progress_);
        SDL_UnlockMutex(mutex_);
        if(!run) break;
        SDL_Delay(FEEDER_SLEEP_MS);
    }
    return 0;
}

void SfxSystem::pumpLocked(){
    if(!device_) return;
    const std::uint64_t queued=
        static_cast<std::uint64_t>(SDL_GetQueuedAudioSize(device_))/sizeof(std::int16_t);
    const std::uint64_t period=std::max<std::uint64_t>(1,spec_.samples);
    if(queued>=period) return;

    bool hasFutureVoice=false;
    for(const auto& v:voices_){
        if(v.startSample+v.samples>producedSamples_){ hasFutureVoice=true; break; }
    }
    if(!hasFutureVoice) return;

    const std::size_t count=static_cast<std::size_t>(period-queued);
    std::vector<std::int16_t> chunk(count,0);
    for(std::size_t i=0;i<count;++i){
        const std::uint64_t sample=producedSamples_+i;
        std::int32_t mixed=0;
        if(enabled_){
            for(const auto& v:voices_){
                if(sample<v.startSample) continue;
                const std::uint64_t off=sample-v.startSample;
                if(off>=v.samples) continue;
                mixed+=v.data[static_cast<std::size_t>(off)];
            }
        }
        chunk[i]=static_cast<std::int16_t>(std::clamp<std::int32_t>(
            mixed,std::numeric_limits<std::int16_t>::min(),
                  std::numeric_limits<std::int16_t>::max()));
    }
    if(SDL_QueueAudio(device_,chunk.data(),static_cast<Uint32>(chunk.size()*sizeof(std::int16_t)))==0){
        producedSamples_+=chunk.size();
        voices_.erase(std::remove_if(voices_.begin(),voices_.end(),[&](const Voice& v){
            return v.startSample+v.samples<=producedSamples_;
        }),voices_.end());
    }
}

std::uint32_t SfxSystem::logicalDurationMs(int soundId) const{
    return speaker_.sourceDurationMsCeil(soundId);
}
