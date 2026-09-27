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
    want.samples=64;
    want.callback=&SfxSystem::audioCallback;
    want.userdata=this;

    device_=SDL_OpenAudioDevice(nullptr,0,&want,&spec_,0);
    if(!device_){ spec_=SDL_AudioSpec{}; return false; }

    renderedSamples_=0;
    sourceCursorSample_=0;
    voices_.clear();
    logicalBusyUntilMs_=0;
    SDL_PauseAudioDevice(device_,0);
    return true;
}

void SfxSystem::shutdown(){
    if(device_){
        SDL_PauseAudioDevice(device_,1);
        SDL_LockAudioDevice(device_);
        clearMixerState();
        SDL_UnlockAudioDevice(device_);
        SDL_CloseAudioDevice(device_);
        device_=0;
    }
    logicalBusyUntilMs_=0;
}

void SfxSystem::clearMixerState(){
    voices_.clear();
    sourceCursorSample_=renderedSamples_;
}

void SfxSystem::setEnabled(bool enabled){
    enabled_=enabled;
    if(!enabled_){
        logicalBusyUntilMs_=0;
        if(device_){
            SDL_LockAudioDevice(device_);
            clearMixerState();
            SDL_UnlockAudioDevice(device_);
        }
    }else if(device_){
        SDL_LockAudioDevice(device_);
        sourceCursorSample_=renderedSamples_;
        SDL_UnlockAudioDevice(device_);
    }
}

void SfxSystem::audioCallback(void* userdata, Uint8* stream, int len){
    static_cast<SfxSystem*>(userdata)->mix(stream,len);
}

void SfxSystem::mix(Uint8* stream, int len){
    if(len<=0) return;
    auto* out=reinterpret_cast<std::int16_t*>(stream);
    const std::size_t frames=static_cast<std::size_t>(len)/sizeof(std::int16_t);

    for(std::size_t i=0;i<frames;++i){
        const std::uint64_t absoluteSample=renderedSamples_+i;
        std::int32_t mixed=0;
        for(const auto& v:voices_){
            if(absoluteSample<v.startSample) continue;
            const std::uint64_t offset=absoluteSample-v.startSample;
            if(offset>=v.samples) continue;
            mixed+=v.data[static_cast<std::size_t>(offset)];
        }
        mixed=std::clamp<std::int32_t>(mixed,
                                      std::numeric_limits<std::int16_t>::min(),
                                      std::numeric_limits<std::int16_t>::max());
        out[i]=static_cast<std::int16_t>(mixed);
    }

    renderedSamples_+=frames;
    voices_.erase(std::remove_if(voices_.begin(),voices_.end(),[&](const Voice& v){
        return v.startSample+v.samples<=renderedSamples_;
    }),voices_.end());
    if(sourceCursorSample_<renderedSamples_) sourceCursorSample_=renderedSamples_;
}

void SfxSystem::advanceLogicalDeadline(std::uint32_t durationMs){
    const std::uint32_t now=SDL_GetTicks();
    if(!logicalBusy()) logicalBusyUntilMs_=now+durationMs;
    else logicalBusyUntilMs_+=durationMs;
}

void SfxSystem::schedule(int soundId, bool immediate){
    if(!enabled_ || soundId<0 || soundId>9) return;

    const auto durationMs=logicalDurationMs(soundId);
    advanceLogicalDeadline(durationMs);

    if(!device_) return;
    const auto clip=SfxPcmOracleM225::clip(soundId);
    if(!clip.data || clip.samples==0) return;

    SDL_LockAudioDevice(device_);
    const std::uint64_t nowSample=renderedSamples_;
    if(sourceCursorSample_<nowSample) sourceCursorSample_=nowSample;

    (void)immediate;
    const std::uint64_t startSample=sourceCursorSample_;
    voices_.push_back(Voice{clip.data,clip.samples,startSample});

    const std::uint64_t logicalSamples=
        (static_cast<std::uint64_t>(durationMs)*SfxPcmOracleM225::SAMPLE_RATE+999u)/1000u;
    sourceCursorSample_=startSample+logicalSamples;
    SDL_UnlockAudioDevice(device_);
}

void SfxSystem::play(int soundId){ schedule(soundId,false); }
void SfxSystem::playImmediate(int soundId){ schedule(soundId,true); }

bool SfxSystem::logicalBusy() const{
    if(!enabled_ || logicalBusyUntilMs_==0) return false;
    const std::uint32_t now=SDL_GetTicks();
    return static_cast<std::int32_t>(logicalBusyUntilMs_-now)>0;
}

std::uint32_t SfxSystem::logicalDurationMs(int soundId) const{
    static constexpr std::uint32_t LOGICAL_MS[10]={
        48,51,78,99,5,900,40,249,98,1,
    };
    return (soundId>=0 && soundId<10) ? LOGICAL_MS[soundId] : 0;
}
