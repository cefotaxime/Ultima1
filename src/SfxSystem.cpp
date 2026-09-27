#include "SfxSystem.h"
#include "SfxPcmOracleM225.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

namespace {
constexpr std::uint32_t FEEDER_SLEEP_MS = 1;
constexpr std::uint32_t MIN_LEAD_MS = 4;
constexpr std::uint32_t CHUNK_MS = 2;
}

SfxSystem::~SfxSystem(){ shutdown(); }

bool SfxSystem::init(){
    if(device_) return true;
    if((SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)==0){
        if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0) return false;
    }

    SDL_AudioSpec want{};
    want.freq=SfxPcmOracleM225::SAMPLE_RATE;
    want.format=AUDIO_S16SYS;
    want.channels=1;
    want.samples=64;
    want.callback=nullptr;

    device_=SDL_OpenAudioDevice(nullptr,0,&want,&spec_,0);
    if(!device_){ spec_=SDL_AudioSpec{}; return false; }

    mutex_=SDL_CreateMutex();
    if(!mutex_){
        SDL_CloseAudioDevice(device_);
        device_=0;
        return false;
    }

    voices_.clear();
    producedSamples_=0;
    sourceCursorSample_=0;
    logicalBusyUntilMs_=0;
    feederRun_=true;
    enabled_=false;

    SDL_PauseAudioDevice(device_,0);
    feeder_=SDL_CreateThread(&SfxSystem::feederEntry,"u1kr-sfx-feeder",this);
    if(!feeder_){
        feederRun_=false;
        SDL_DestroyMutex(mutex_); mutex_=nullptr;
        SDL_CloseAudioDevice(device_); device_=0;
        return false;
    }

    std::cerr << "SFX audio backend: "
              << (SDL_GetCurrentAudioDriver()?SDL_GetCurrentAudioDriver():"unknown")
              << ", freq=" << spec_.freq
              << ", samples=" << spec_.samples
              << ", queued-stream mixer=M229\n";
    return true;
}

void SfxSystem::resetLocked(bool clearDeviceQueue){
    voices_.clear();
    if(clearDeviceQueue && device_) SDL_ClearQueuedAudio(device_);
    producedSamples_=0;
    sourceCursorSample_=0;
}

void SfxSystem::shutdown(){
    if(feeder_){
        if(mutex_) SDL_LockMutex(mutex_);
        feederRun_=false;
        if(mutex_) SDL_UnlockMutex(mutex_);
        SDL_WaitThread(feeder_,nullptr);
        feeder_=nullptr;
    }
    if(mutex_){
        SDL_LockMutex(mutex_);
        resetLocked(true);
        SDL_UnlockMutex(mutex_);
        SDL_DestroyMutex(mutex_);
        mutex_=nullptr;
    }else if(device_){
        SDL_ClearQueuedAudio(device_);
    }
    if(device_){
        SDL_CloseAudioDevice(device_);
        device_=0;
    }
    logicalBusyUntilMs_=0;
    enabled_=false;
}

void SfxSystem::setEnabled(bool enabled){
    if(!mutex_){ enabled_=enabled; return; }
    SDL_LockMutex(mutex_);
    enabled_=enabled;
    if(!enabled_){
        logicalBusyUntilMs_=0;
        resetLocked(true);
    }else{
        logicalBusyUntilMs_=0;
        if(device_ && SDL_GetQueuedAudioSize(device_)==0){
            producedSamples_=0;
            sourceCursorSample_=0;
        }
    }
    SDL_UnlockMutex(mutex_);
}

std::uint32_t SfxSystem::remainingLogicalMs(std::uint32_t now) const{
    if(logicalBusyUntilMs_==0) return 0;
    const auto delta=static_cast<std::int32_t>(logicalBusyUntilMs_-now);
    return delta>0 ? static_cast<std::uint32_t>(delta) : 0;
}

std::uint64_t SfxSystem::playedSamplesLocked() const{
    if(!device_) return producedSamples_;
    const std::uint64_t queued=
        static_cast<std::uint64_t>(SDL_GetQueuedAudioSize(device_))/sizeof(std::int16_t);
    return queued>=producedSamples_ ? 0 : producedSamples_-queued;
}

void SfxSystem::schedule(int soundId){
    if(!enabled_ || soundId<0 || soundId>9) return;

    const std::uint32_t now=SDL_GetTicks();
    const std::uint32_t startDelayMs=remainingLogicalMs(now);
    const std::uint32_t durationMs=logicalDurationMs(soundId);
    logicalBusyUntilMs_=now+startDelayMs+durationMs;

    if(!device_ || !mutex_) return;
    const auto clip=SfxPcmOracleM225::clip(soundId);
    if(!clip.data || clip.samples==0) return;

    SDL_LockMutex(mutex_);
    const std::uint64_t played=playedSamplesLocked();
    std::uint64_t startSample=std::max(sourceCursorSample_,producedSamples_);
    if(startSample<played) startSample=played;
    voices_.push_back(Voice{clip.data,clip.samples,startSample});

    const std::uint64_t logicalSamples=
        (static_cast<std::uint64_t>(durationMs)*SfxPcmOracleM225::SAMPLE_RATE+999u)/1000u;
    sourceCursorSample_=startSample+logicalSamples;
    pumpLocked();
    SDL_UnlockMutex(mutex_);
}

void SfxSystem::play(int soundId){ schedule(soundId); }
void SfxSystem::playImmediate(int soundId){ schedule(soundId); }

bool SfxSystem::logicalBusy() const{
    if(!enabled_) return false;
    return remainingLogicalMs(SDL_GetTicks())>0;
}

int SDLCALL SfxSystem::feederEntry(void* userdata){
    return static_cast<SfxSystem*>(userdata)->feederLoop();
}

int SfxSystem::feederLoop(){
    for(;;){
        if(!mutex_) return 0;
        SDL_LockMutex(mutex_);
        const bool run=feederRun_;
        if(run && enabled_) pumpLocked();
        SDL_UnlockMutex(mutex_);
        if(!run) break;
        SDL_Delay(FEEDER_SLEEP_MS);
    }
    return 0;
}

void SfxSystem::pumpLocked(){
    if(!device_ || !enabled_) return;

    const std::uint64_t queuedSamples=
        static_cast<std::uint64_t>(SDL_GetQueuedAudioSize(device_))/sizeof(std::int16_t);
    const std::uint64_t devicePeriod=std::max<std::uint64_t>(1,spec_.samples);
    const std::uint64_t minLead=
        (static_cast<std::uint64_t>(MIN_LEAD_MS)*SfxPcmOracleM225::SAMPLE_RATE+999u)/1000u;
    const std::uint64_t targetLead=std::max(minLead,devicePeriod);
    if(queuedSamples>=targetLead) return;

    const std::uint64_t nominalChunk=
        (static_cast<std::uint64_t>(CHUNK_MS)*SfxPcmOracleM225::SAMPLE_RATE+999u)/1000u;
    const std::uint64_t chunkSamples=std::max<std::uint64_t>(nominalChunk,devicePeriod);
    const std::uint64_t need=targetLead-queuedSamples;
    const std::size_t count=static_cast<std::size_t>(std::min(chunkSamples,need));
    if(count==0) return;

    bool futureVoice=false;
    for(const auto& v:voices_){
        if(v.startSample+v.samples>producedSamples_){ futureVoice=true; break; }
    }
    if(!futureVoice) return;

    std::vector<std::int16_t> chunk(count,0);
    for(std::size_t i=0;i<count;++i){
        const std::uint64_t absoluteSample=producedSamples_+i;
        std::int32_t mixed=0;
        for(const auto& v:voices_){
            if(absoluteSample<v.startSample) continue;
            const std::uint64_t offset=absoluteSample-v.startSample;
            if(offset>=v.samples) continue;
            mixed+=v.data[static_cast<std::size_t>(offset)];
        }
        chunk[i]=static_cast<std::int16_t>(std::clamp<std::int32_t>(
            mixed,std::numeric_limits<std::int16_t>::min(),
                  std::numeric_limits<std::int16_t>::max()));
    }

    if(SDL_QueueAudio(device_,chunk.data(),
                      static_cast<Uint32>(chunk.size()*sizeof(std::int16_t)))==0){
        producedSamples_+=chunk.size();
        voices_.erase(std::remove_if(voices_.begin(),voices_.end(),[&](const Voice& v){
            return v.startSample+v.samples<=producedSamples_;
        }),voices_.end());
        if(voices_.empty() && SDL_GetQueuedAudioSize(device_)==0)
            sourceCursorSample_=producedSamples_;
    }
}

std::uint32_t SfxSystem::logicalDurationMs(int soundId) const{
    static constexpr std::uint32_t LOGICAL_MS[10]={
        48,51,78,99,5,900,40,249,98,1,
    };
    return (soundId>=0 && soundId<10) ? LOGICAL_MS[soundId] : 0;
}
