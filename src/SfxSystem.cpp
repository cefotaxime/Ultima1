#include "SfxSystem.h"
#include "SfxPcmOracleM225.h"
#include <algorithm>
#include <cstdint>
#include <limits>

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
    // Same proven low-latency queued-audio path as M225. callback remains null.
    want.samples=64;

    device_=SDL_OpenAudioDevice(nullptr,0,&want,&spec_,0);
    if(!device_){ spec_=SDL_AudioSpec{}; return false; }

    queuedMix_.clear();
    logicalBusyUntilMs_=0;
    SDL_PauseAudioDevice(device_,0);
    return true;
}

void SfxSystem::clearQueueState(){
    queuedMix_.clear();
    if(device_) SDL_ClearQueuedAudio(device_);
}

void SfxSystem::shutdown(){
    if(device_){
        clearQueueState();
        SDL_CloseAudioDevice(device_);
        device_=0;
    }
    logicalBusyUntilMs_=0;
}

void SfxSystem::setEnabled(bool enabled){
    enabled_=enabled;
    if(!enabled_){
        logicalBusyUntilMs_=0;
        clearQueueState();
    }
}

std::uint32_t SfxSystem::remainingLogicalMs(std::uint32_t now) const{
    if(logicalBusyUntilMs_==0) return 0;
    const auto delta=static_cast<std::int32_t>(logicalBusyUntilMs_-now);
    return delta>0 ? static_cast<std::uint32_t>(delta) : 0;
}

void SfxSystem::schedule(int soundId){
    if(!enabled_ || soundId<0 || soundId>9) return;

    const std::uint32_t now=SDL_GetTicks();
    const std::uint32_t startDelayMs=remainingLogicalMs(now);
    const std::uint32_t durationMs=logicalDurationMs(soundId);
    logicalBusyUntilMs_=now+startDelayMs+durationMs;

    if(!device_) return;
    const auto clip=SfxPcmOracleM225::clip(soundId);
    if(!clip.data || clip.samples==0) return;

    const std::size_t queuedSamples=
        static_cast<std::size_t>(SDL_GetQueuedAudioSize(device_))/sizeof(std::int16_t);
    const std::size_t keep=std::min(queuedSamples,queuedMix_.size());
    std::vector<std::int16_t> mixed;
    if(keep>0)
        mixed.assign(queuedMix_.end()-static_cast<std::ptrdiff_t>(keep),queuedMix_.end());

    const std::size_t startSample=static_cast<std::size_t>(
        (static_cast<std::uint64_t>(startDelayMs)*SfxPcmOracleM225::SAMPLE_RATE+999u)/1000u);
    const std::size_t need=startSample+clip.samples;
    if(mixed.size()<need) mixed.resize(need,0);

    for(std::size_t i=0;i<clip.samples;++i){
        const std::size_t dst=startSample+i;
        const std::int32_t sum=static_cast<std::int32_t>(mixed[dst])+
                               static_cast<std::int32_t>(clip.data[i]);
        mixed[dst]=static_cast<std::int16_t>(std::clamp<std::int32_t>(
            sum,std::numeric_limits<std::int16_t>::min(),
                std::numeric_limits<std::int16_t>::max()));
    }

    SDL_ClearQueuedAudio(device_);
    if(!mixed.empty()){
        const auto bytes=static_cast<Uint32>(mixed.size()*sizeof(std::int16_t));
        if(SDL_QueueAudio(device_,mixed.data(),bytes)!=0){
            queuedMix_.clear();
            return;
        }
    }
    queuedMix_.swap(mixed);
}

void SfxSystem::play(int soundId){ schedule(soundId); }
void SfxSystem::playImmediate(int soundId){ schedule(soundId); }

bool SfxSystem::logicalBusy() const{
    if(!enabled_) return false;
    return remainingLogicalMs(SDL_GetTicks())>0;
}

std::uint32_t SfxSystem::logicalDurationMs(int soundId) const{
    static constexpr std::uint32_t LOGICAL_MS[10]={
        48,51,78,99,5,900,40,249,98,1,
    };
    return (soundId>=0 && soundId<10) ? LOGICAL_MS[soundId] : 0;
}
