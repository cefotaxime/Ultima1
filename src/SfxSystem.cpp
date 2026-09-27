#include "SfxSystem.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <audioclient.h>
#include <avrt.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <condition_variable>
#include <thread>
#endif

namespace {
constexpr std::uint32_t REQUESTED_SAMPLES=64;
std::uint64_t perfUs(){
    const auto f=SDL_GetPerformanceFrequency();
    const auto c=SDL_GetPerformanceCounter();
    return f?static_cast<std::uint64_t>((c*1000000ull)/f):0ull;
}

#ifdef _WIN32
template<class T> void releaseCom(T*& p){ if(p){ p->Release(); p=nullptr; } }

enum class WinSampleFormat { Unsupported, Float32, Pcm16, Pcm24, Pcm32 };

WinSampleFormat sampleFormatOf(const WAVEFORMATEX* f){
    if(!f) return WinSampleFormat::Unsupported;
    WORD tag=f->wFormatTag;
    if(tag==WAVE_FORMAT_EXTENSIBLE && f->cbSize>=22){
        const auto* ex=reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(f);
        if(IsEqualGUID(ex->SubFormat,KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) tag=WAVE_FORMAT_IEEE_FLOAT;
        else if(IsEqualGUID(ex->SubFormat,KSDATAFORMAT_SUBTYPE_PCM)) tag=WAVE_FORMAT_PCM;
        else return WinSampleFormat::Unsupported;
    }
    if(tag==WAVE_FORMAT_IEEE_FLOAT && f->wBitsPerSample==32) return WinSampleFormat::Float32;
    if(tag==WAVE_FORMAT_PCM){
        if(f->wBitsPerSample==16) return WinSampleFormat::Pcm16;
        if(f->wBitsPerSample==24) return WinSampleFormat::Pcm24;
        if(f->wBitsPerSample==32) return WinSampleFormat::Pcm32;
    }
    return WinSampleFormat::Unsupported;
}

const char* sampleFormatName(WinSampleFormat f){
    switch(f){
        case WinSampleFormat::Float32:return "float32";
        case WinSampleFormat::Pcm16:return "pcm16";
        case WinSampleFormat::Pcm24:return "pcm24";
        case WinSampleFormat::Pcm32:return "pcm32";
        default:return "unsupported";
    }
}
#endif
}

#ifdef _WIN32
struct SfxSystem::WinAudioState {
    std::thread thread;
    std::mutex initMutex;
    std::condition_variable initCv;
    bool ready=false;
    bool success=false;
    std::atomic<bool> stop{false};
    HANDLE eventHandle=nullptr;

    UINT32 defaultPeriodFrames=0;
    UINT32 fundamentalPeriodFrames=0;
    UINT32 minPeriodFrames=0;
    UINT32 maxPeriodFrames=0;
    UINT32 bufferFrames=0;
    UINT32 channels=0;
    WORD bitsPerSample=0;
    WinSampleFormat sampleFormat=WinSampleFormat::Unsupported;
    HRESULT initHr=S_OK;
    std::string failure;
};
#endif

SfxSystem::~SfxSystem(){ shutdown(); }

bool SfxSystem::init(){
    if(audioAvailable_) return true;

    const char* tr=std::getenv("U1_SFX_TRACE");
    trace_=(tr && *tr && *tr!='0');
    traceEpochCounter_=perfUs();
    enabled_.store(false,std::memory_order_release);
    renderedSamples_.store(0,std::memory_order_release);
    synchronousEndSample_.store(0,std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(voicesMutex_);
        voices_.clear();
    }

#ifdef _WIN32
    const char* forceSdl=std::getenv("U1_SFX_FORCE_SDL");
    if(!(forceSdl && *forceSdl && *forceSdl!='0') && initWindowsLowLatency()){
        audioAvailable_=true;
        std::cerr << "PC speaker emulator: driver=wasapi-iaudioclient3"
                  << ", freq=" << PcSpeakerEmulator::SAMPLE_RATE
                  << ", channels=" << winAudio_->channels
                  << ", format=" << sampleFormatName(winAudio_->sampleFormat)
                  << ", min_period_frames=" << winAudio_->minPeriodFrames
                  << ", default_period_frames=" << winAudio_->defaultPeriodFrames
                  << ", endpoint_buffer_frames=" << winAudio_->bufferFrames
                  << ", source-synchronous=M235-callback-clock"
                  << ", transport=M237-event-driven-low-latency"
                  << ", app-queue=none"
                  << ", trace=" << (trace_?"on":"off") << "\n";
        return true;
    }
    if(winAudio_ && !winAudio_->failure.empty()){
        std::cerr << "PC speaker emulator: M237 low-latency WASAPI unavailable ("
                  << winAudio_->failure << "), falling back to SDL/WASAPI\n";
    }
    winAudio_.reset();
#endif

    SDL_setenv("PULSE_LATENCY_MSEC","5",0);
    if((SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)==0){
        if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0) return false;
    }

    SDL_AudioSpec want{};
    want.freq=PcSpeakerEmulator::SAMPLE_RATE;
    want.format=AUDIO_S16SYS;
    want.channels=1;
    want.samples=REQUESTED_SAMPLES;
    want.callback=&SfxSystem::audioCallback;
    want.userdata=this;
    device_=SDL_OpenAudioDevice(nullptr,0,&want,&spec_,0);
    if(!device_){ spec_=SDL_AudioSpec{}; return false; }

    audioAvailable_=true;
    SDL_PauseAudioDevice(device_,0);

    std::cerr << "PC speaker emulator: driver="
              << (SDL_GetCurrentAudioDriver()?SDL_GetCurrentAudioDriver():"unknown")
              << ", freq=" << spec_.freq
              << ", samples=" << spec_.samples
              << ", source-synchronous=M235-callback-clock"
              << ", transport=SDL-direct-callback-fallback"
              << ", app-queue=none"
              << ", trace=" << (trace_?"on":"off") << "\n";
    return true;
}

void SfxSystem::shutdown(){
    enabled_.store(false,std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(voicesMutex_);
        voices_.clear();
    }
#ifdef _WIN32
    shutdownWindowsLowLatency();
#endif
    if(device_){
        SDL_CloseAudioDevice(device_);
        device_=0;
    }
    audioAvailable_=false;
}

void SfxSystem::setEnabled(bool enabled){
    {
        std::lock_guard<std::mutex> lock(voicesMutex_);
        enabled_.store(enabled,std::memory_order_release);
        if(!enabled) voices_.clear();
    }
}

void SDLCALL SfxSystem::audioCallback(void* userdata,Uint8* stream,int len){
    auto* self=static_cast<SfxSystem*>(userdata);
    if(!self || !stream || len<=0) return;
    auto* dst=reinterpret_cast<std::int16_t*>(stream);
    self->renderCallback(dst,static_cast<std::size_t>(len)/sizeof(std::int16_t));
}

void SfxSystem::renderCallback(std::int16_t* dst,std::size_t samples){
    std::lock_guard<std::mutex> lock(voicesMutex_);
    std::fill(dst,dst+samples,static_cast<std::int16_t>(0));
    const auto renderedBase=renderedSamples_.load(std::memory_order_relaxed);
    if(enabled_.load(std::memory_order_acquire)){
        for(std::size_t i=0;i<samples;++i){
            const std::uint64_t sample=renderedBase+i;
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
    while(enabled_.load(std::memory_order_acquire) &&
          renderedSamples_.load(std::memory_order_acquire)<endSample){
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
    if(soundId<0 || soundId>9 || !enabled_.load(std::memory_order_acquire)) return;
    const auto us=speaker_.sourceDurationUs(soundId);
    if(!audioAvailable_){
        waitFallbackDurationUs(us);
        return;
    }

    const auto& clip=speaker_.clip(soundId);
    std::uint64_t startSample=0;
    std::uint64_t sourceEndSample=0;
    {
        std::lock_guard<std::mutex> lock(voicesMutex_);
        startSample=renderedSamples_.load(std::memory_order_acquire);
        sourceEndSample=startSample+static_cast<std::uint64_t>(clip.sourceSamples);
        synchronousEndSample_.store(sourceEndSample,std::memory_order_release);
        if(!clip.pcm.empty()) voices_.push_back(Voice{clip.pcm.data(),clip.pcm.size(),startSample});
    }

    const auto traceFrames=(
#ifdef _WIN32
        winAudio_ ? winAudio_->minPeriodFrames :
#endif
        static_cast<std::uint32_t>(spec_.samples));

    if(trace_){
        std::cerr << "[SFXTRACE] t_us=" << (perfUs()-traceEpochCounter_)
                  << " play_begin id=" << soundId
                  << " rendered=" << startSample
                  << " source_end=" << sourceEndSample
                  << " callback_frames=" << traceFrames
                  << " app_queue_samples=0\n";
    }

    // OUT.EXE sound() is synchronous. Tie its return to the audio render
    // consumption clock; M237 changes only the Windows transport period.
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
    return enabled_.load(std::memory_order_acquire) &&
           renderedSamples_.load(std::memory_order_acquire) <
           synchronousEndSample_.load(std::memory_order_acquire);
}

std::uint32_t SfxSystem::logicalDurationMs(int soundId) const{
    return speaker_.sourceDurationMsCeil(soundId);
}

#ifdef _WIN32
bool SfxSystem::initWindowsLowLatency(){
    winAudio_=std::make_unique<WinAudioState>();
    auto* state=winAudio_.get();

    state->thread=std::thread([this,state](){
        IMMDeviceEnumerator* enumerator=nullptr;
        IMMDevice* device=nullptr;
        IAudioClient3* client=nullptr;
        IAudioRenderClient* render=nullptr;
        WAVEFORMATEX* mix=nullptr;
        HANDLE mmcssHandle=nullptr;
        DWORD mmcssTaskIndex=0;
        bool comInitialized=false;
        bool started=false;

        auto fail=[&](HRESULT hr,const char* what){
            state->initHr=hr;
            state->failure=std::string(what)+" hr=0x"+[] (HRESULT x){
                char b[16]{};
                std::snprintf(b,sizeof(b),"%08lx",static_cast<unsigned long>(x));
                return std::string(b);
            }(hr);
        };
        auto signalReady=[&](bool success){
            {
                std::lock_guard<std::mutex> lock(state->initMutex);
                state->success=success;
                state->ready=true;
            }
            state->initCv.notify_one();
        };

        HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
        if(FAILED(hr)){
            fail(hr,"CoInitializeEx");
            signalReady(false);
            return;
        }
        comInitialized=true;

        do {
            hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(&enumerator));
            if(FAILED(hr)){ fail(hr,"CoCreateInstance(MMDeviceEnumerator)"); break; }
            hr=enumerator->GetDefaultAudioEndpoint(eRender,eConsole,&device);
            if(FAILED(hr)){ fail(hr,"GetDefaultAudioEndpoint"); break; }
            hr=device->Activate(__uuidof(IAudioClient3),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(&client));
            if(FAILED(hr)){ fail(hr,"Activate(IAudioClient3)"); break; }
            hr=client->GetMixFormat(&mix);
            if(FAILED(hr)){ fail(hr,"GetMixFormat"); break; }

            state->channels=mix->nChannels;
            state->bitsPerSample=mix->wBitsPerSample;
            state->sampleFormat=sampleFormatOf(mix);
            if(mix->nSamplesPerSec!=PcSpeakerEmulator::SAMPLE_RATE){
                state->failure="mix rate is not 48000 Hz; preserving pitch via SDL fallback";
                break;
            }
            if(state->channels==0 || state->sampleFormat==WinSampleFormat::Unsupported){
                state->failure="unsupported Windows mix format";
                break;
            }

            hr=client->GetSharedModeEnginePeriod(mix,
                &state->defaultPeriodFrames,
                &state->fundamentalPeriodFrames,
                &state->minPeriodFrames,
                &state->maxPeriodFrames);
            if(FAILED(hr)){ fail(hr,"GetSharedModeEnginePeriod"); break; }
            if(state->minPeriodFrames==0){
                state->failure="audio engine reported zero minimum period";
                break;
            }

            hr=client->InitializeSharedAudioStream(
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                state->minPeriodFrames,mix,nullptr);
            if(FAILED(hr)){ fail(hr,"InitializeSharedAudioStream"); break; }
            hr=client->GetBufferSize(&state->bufferFrames);
            if(FAILED(hr)){ fail(hr,"GetBufferSize"); break; }

            state->eventHandle=CreateEventW(nullptr,FALSE,FALSE,nullptr);
            if(!state->eventHandle){
                fail(HRESULT_FROM_WIN32(GetLastError()),"CreateEventW");
                break;
            }
            hr=client->SetEventHandle(state->eventHandle);
            if(FAILED(hr)){ fail(hr,"SetEventHandle"); break; }
            hr=client->GetService(__uuidof(IAudioRenderClient),reinterpret_cast<void**>(&render));
            if(FAILED(hr)){ fail(hr,"GetService(IAudioRenderClient)"); break; }

            mmcssHandle=AvSetMmThreadCharacteristicsW(L"Pro Audio",&mmcssTaskIndex);

            hr=client->Start();
            if(FAILED(hr)){ fail(hr,"IAudioClient::Start"); break; }
            started=true;

            spec_=SDL_AudioSpec{};
            spec_.freq=PcSpeakerEmulator::SAMPLE_RATE;
            spec_.format=AUDIO_S16SYS;
            spec_.channels=1;
            spec_.samples=static_cast<Uint16>(std::min<UINT32>(state->minPeriodFrames,65535u));
            signalReady(true);

            std::vector<std::int16_t> mono;
            while(!state->stop.load(std::memory_order_acquire)){
                const DWORD wr=WaitForSingleObject(state->eventHandle,2000);
                if(state->stop.load(std::memory_order_acquire)) break;
                if(wr!=WAIT_OBJECT_0) continue;

                UINT32 padding=0;
                hr=client->GetCurrentPadding(&padding);
                if(FAILED(hr) || padding>=state->bufferFrames) continue;
                const UINT32 frames=state->bufferFrames-padding;
                if(frames==0) continue;

                BYTE* dst=nullptr;
                hr=render->GetBuffer(frames,&dst);
                if(FAILED(hr) || !dst) continue;

                mono.resize(frames);
                renderCallback(mono.data(),frames);

                const UINT32 channels=state->channels;
                switch(state->sampleFormat){
                    case WinSampleFormat::Float32: {
                        auto* out=reinterpret_cast<float*>(dst);
                        for(UINT32 i=0;i<frames;++i){
                            const float v=static_cast<float>(mono[i])/32768.0f;
                            for(UINT32 c=0;c<channels;++c) *out++=v;
                        }
                        break;
                    }
                    case WinSampleFormat::Pcm16: {
                        auto* out=reinterpret_cast<std::int16_t*>(dst);
                        for(UINT32 i=0;i<frames;++i)
                            for(UINT32 c=0;c<channels;++c) *out++=mono[i];
                        break;
                    }
                    case WinSampleFormat::Pcm24: {
                        auto* out=dst;
                        for(UINT32 i=0;i<frames;++i){
                            const std::int32_t v=static_cast<std::int32_t>(mono[i])<<8;
                            for(UINT32 c=0;c<channels;++c){
                                *out++=static_cast<BYTE>(v&0xff);
                                *out++=static_cast<BYTE>((v>>8)&0xff);
                                *out++=static_cast<BYTE>((v>>16)&0xff);
                            }
                        }
                        break;
                    }
                    case WinSampleFormat::Pcm32: {
                        auto* out=reinterpret_cast<std::int32_t*>(dst);
                        for(UINT32 i=0;i<frames;++i){
                            const std::int32_t v=static_cast<std::int32_t>(mono[i])<<16;
                            for(UINT32 c=0;c<channels;++c) *out++=v;
                        }
                        break;
                    }
                    default:
                        std::memset(dst,0,static_cast<std::size_t>(frames)*mix->nBlockAlign);
                        break;
                }
                render->ReleaseBuffer(frames,0);
            }
        } while(false);

        if(!state->ready) signalReady(false);
        if(started) client->Stop();
        if(mmcssHandle) AvRevertMmThreadCharacteristics(mmcssHandle);
        releaseCom(render);
        releaseCom(client);
        if(mix) CoTaskMemFree(mix);
        releaseCom(device);
        releaseCom(enumerator);
        if(state->eventHandle){ CloseHandle(state->eventHandle); state->eventHandle=nullptr; }
        if(comInitialized) CoUninitialize();
    });

    {
        std::unique_lock<std::mutex> lock(state->initMutex);
        state->initCv.wait(lock,[&](){ return state->ready; });
    }
    if(!state->success){
        state->stop.store(true,std::memory_order_release);
        if(state->thread.joinable()) state->thread.join();
        return false;
    }
    return true;
}

void SfxSystem::shutdownWindowsLowLatency(){
    if(!winAudio_) return;
    winAudio_->stop.store(true,std::memory_order_release);
    if(winAudio_->eventHandle) SetEvent(winAudio_->eventHandle);
    if(winAudio_->thread.joinable()) winAudio_->thread.join();
    winAudio_.reset();
}
#endif
