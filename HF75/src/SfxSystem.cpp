#include "SfxSystem.h"
#include "Apple1986BurstTrial.h"
#include "Apple1986NormalTrial.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#if !defined(_WIN32)
#include <cerrno>
#include <csignal>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif


#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
#include <pulse/pulseaudio.h>
#endif

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
constexpr std::uint32_t PULSE_TARGET_LATENCY_US=5000;

// HF47 Apple II Front View sound oracle.
// Timestamps are exact 6502 cycle positions of the 16 STAR.OBJ $C030 reads
// for game speeds 1..8, measured by executing the original 1981 object code.
// Normal Front View sets DE = 144 - speed*16, so faster speed means shorter delay.
constexpr std::array<std::array<int,16>,8> APPLE_FRONT_STAR_TOGGLE_CYCLES={{
    {{621,2664,4707,6750,8793,10836,12879,14922,16965,19008,21051,23065,25084,27103,28820,30534}},
    {{621,2488,4355,6222,8089,9956,11823,13690,15557,17424,19291,21129,22972,24815,26356,27894}},
    {{621,2312,4003,5694,7385,9076,10767,12458,14149,15840,17531,19193,20860,22527,23892,25254}},
    {{621,2136,3651,5166,6681,8196,9711,11226,12741,14256,15771,17257,18748,20239,21428,22614}},
    {{621,1960,3299,4638,5977,7316,8655,9994,11333,12672,14011,15321,16636,17951,18964,19974}},
    {{621,1784,2947,4110,5273,6436,7599,8762,9925,11088,12251,13385,14524,15663,16500,17334}},
    {{621,1608,2595,3582,4569,5556,6543,7530,8517,9504,10491,11449,12412,13375,14036,14694}},
    {{621,1432,2243,3054,3865,4676,5487,6298,7109,7920,8731,9513,10300,11087,11572,12054}}
}};
constexpr std::array<int,8> APPLE_FRONT_STAR_TOTAL_CYCLES={
    31962,29146,26330,23514,20698,17882,15066,12250
};

// FGT3 CALL 29704 executes one extra $C030 read near the end of each BASIC loop.
// Direct execution of the original routine gives 205 cycles from FGT3 entry to
// that read and 215 cycles total. Applesoft parsing/call overhead around it is
// not represented by STAR.OBJ/FGT3, so the loop gap below is calibrated against
// the supplied Apple II longplay.
constexpr int APPLE_FRONT_FGT3_TOGGLE_AFTER_STAR_CYCLES=1000;
constexpr int APPLE_FRONT_BASIC_LOOP_GAP_CYCLES=4800;
constexpr double APPLE2_CPU_HZ=1023000.0;
// Apple II hardware is AC-coupled (C11/R24) and C12 integrates the speaker drive.
// A held +/-level therefore is not an accurate audible output. The direct longplay
// has its dominant Front View high layer at ~1.05-1.15 kHz (median ~1.09 kHz).
// This 48-kHz band-pass is centered at 1090 Hz and calibrated to the longplay, used only as
// the speaker/cabinet transfer response; toggle timing remains exact STAR.OBJ timing.
constexpr double APPLE_FRONT_BP_B0=0.008809026947197717;
constexpr double APPLE_FRONT_BP_B1=0.0;
constexpr double APPLE_FRONT_BP_B2=-0.008809026947197717;
constexpr double APPLE_FRONT_BP_A1=-1.9622377148426318;
constexpr double APPLE_FRONT_BP_A2=0.9823819461056046;
constexpr std::int32_t APPLE_FRONT_RAW_AMPLITUDE=0;
constexpr std::int32_t APPLE_FRONT_BP_AMPLITUDE=4200;
// Recorded by executing the unmodified 1981 STAR.OBJ at $0BDF with
// DE ($0BA3) = 1. CPU cycles measured at each $C030 speaker access.
constexpr std::array<int,16> APPLE_HYPER_DE1_TOGGLE_CYCLES={
    621,1267,1913,2559,3205,3851,4497,5143,
    5789,6435,7081,7698,8320,8942,9262,9579};
constexpr int APPLE_HYPER_DE1_TOTAL_CYCLES=9610;

void audioDiagReset(){
    std::ofstream out("audio_runtime_m238.txt",std::ios::trunc);
    out << "M238 canonical runtime: WSL/Linux direct PulseAudio preferred; SDL is fallback\n";
}
void audioDiagAppend(const std::string& line){
    std::ofstream out("audio_runtime_m238.txt",std::ios::app);
    out << line << "\n";
}

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

#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
struct SfxSystem::HostAudioBridgeState {
    pid_t child=-1;
    int writeFd=-1;
    int readFd=-1;
    std::mutex writeMutex;
    std::string failure;
    std::string backend;
};

struct SfxSystem::PulseAudioState {
    pa_threaded_mainloop* mainloop=nullptr;
    pa_context* context=nullptr;
    pa_stream* stream=nullptr;
    pa_buffer_attr negotiated{};
    std::atomic<std::uint32_t> underflows{0};
    std::atomic<bool> latencyLogged{false};
    std::string failure;
};
#endif

SfxSystem::SfxSystem() = default;
SfxSystem::~SfxSystem(){ shutdown(); }

bool SfxSystem::init(){
    if(audioAvailable_) return true;

    audioDiagReset();
    const char* tr=std::getenv("U1_SFX_TRACE");
    trace_=(tr && *tr && *tr!='0');
    traceEpochCounter_=perfUs();
    enabled_.store(true,std::memory_order_release);
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

#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
    const char* forceSdl=std::getenv("U1_SFX_FORCE_SDL");
    const char* forcePulse=std::getenv("U1_SFX_FORCE_PULSE");
    if(!(forceSdl && *forceSdl && *forceSdl!='0') &&
       !(forcePulse && *forcePulse && *forcePulse!='0') &&
       initHostAudioBridge()){
        audioAvailable_=true;
        const std::string line=std::string("PC speaker emulator: driver=windows-host-wasapi-bridge, freq=48000, format=stereo16, transport=M246-WSL-interop-host-wasapi, pcm=M241-approved, helper=")+hostAudio_->backend;
        std::cerr << line << "\n";
        std::ofstream out("audio_runtime_m246.txt",std::ios::trunc);
        out << line << "\n";
        return true;
    }
    if(hostAudio_ && !hostAudio_->failure.empty()){
        std::cerr << "PC speaker emulator: M246 Windows-host WASAPI bridge unavailable ("
                  << hostAudio_->failure << "), falling back to M243 PulseAudio\n";
    }
    hostAudio_.reset();
    if(!(forceSdl && *forceSdl && *forceSdl!='0') && initPulseLowLatency()){
        audioAvailable_=true;
        const auto& a=pulseAudio_->negotiated;
        const pa_sample_spec pulseSpec{PA_SAMPLE_S16LE,PcSpeakerEmulator::SAMPLE_RATE,1};
        const std::uint64_t tlengthUs=pa_bytes_to_usec(a.tlength,&pulseSpec);
        const std::uint64_t minreqUs=pa_bytes_to_usec(a.minreq,&pulseSpec);
        const std::string line=std::string("PC speaker emulator: driver=pulseaudio-direct")+
            ", freq="+std::to_string(PcSpeakerEmulator::SAMPLE_RATE)+
            ", requested_latency_us="+std::to_string(PULSE_TARGET_LATENCY_US)+
            ", negotiated_tlength_us="+std::to_string(tlengthUs)+
            ", negotiated_minreq_us="+std::to_string(minreqUs)+
            ", source-synchronous=M235-callback-clock"+
            ", transport=M238-direct-pulse-write-callback"+
            ", app-queue=none"+
            ", trace="+(trace_?"on":"off");
        std::cerr << line << "\n";
        audioDiagAppend(line);
        return true;
    }
    if(pulseAudio_ && !pulseAudio_->failure.empty()){
        const std::string line=std::string("M238 direct PulseAudio unavailable: ")+pulseAudio_->failure+
            "; falling back to SDL/PulseAudio";
        std::cerr << "PC speaker emulator: " << line << "\n";
        audioDiagAppend(line);
    }
    pulseAudio_.reset();
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
    audioDiagAppend(std::string("PC speaker emulator: driver=")+
                    (SDL_GetCurrentAudioDriver()?SDL_GetCurrentAudioDriver():"unknown")+
                    ", transport=SDL-direct-callback-fallback, requested_pulse_latency_ms=5");
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
#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
    shutdownHostAudioBridge();
    shutdownPulseLowLatency();
#endif
    if(device_){
        SDL_CloseAudioDevice(device_);
        device_=0;
    }
    audioAvailable_=false;
}

void SfxSystem::setAppleFrontView(bool active,int speed,int hyperPhase,int lowSpeed){
    speed=std::clamp(speed,1,8);
    const bool was=appleFrontActive_.exchange(active,std::memory_order_acq_rel);
    appleFrontSpeed_.store(speed,std::memory_order_release);
    const int oldHyperPhase=appleFrontHyperPhase_.exchange(hyperPhase,std::memory_order_acq_rel);
    // A completed previous jump must not instantly complete the next jump.
    if(hyperPhase==1 && oldHyperPhase==0) appleLive_.clearHyperStatus();
    appleFrontLowSpeed_.store(std::clamp(lowSpeed,0,8),std::memory_order_release);
    if(active && !was){
        std::lock_guard<std::mutex> lock(voicesMutex_);
        appleLive_.reset(speed);
        appleFrontLevel_=false;
        appleFrontEvent_=0;
        appleFrontCycleSpeed_=speed;
        appleFrontHighCycleOn_=true;
        appleFrontLowOscPhase_=0.0;
        appleFrontLowPulsePhase_=0.0;
        appleFrontPulseHzSmoothed_=0.0;
        appleFrontPreviousPhase_=0;
        appleBurstLevel_=false;
        appleBurstEvent_=0;
        appleBurstSamplesLeft_=0.0;
        appleBurstHighpass_=0.0;
        appleBurstPreviousRaw_=0.0;
        appleFrontBpX1_=appleFrontBpX2_=0.0;
        appleFrontBpY1_=appleFrontBpY2_=0.0;
        const int first=2145; // 1986 SPA+STUPH seed-specific trial trace
        appleFrontSamplesLeft_=static_cast<double>(first)*PcSpeakerEmulator::SAMPLE_RATE/APPLE2_CPU_HZ;
    }
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
                // Front View low-pulse gain only; other SFX are unchanged.
                const bool frontLow=(appleFrontActive_.load(std::memory_order_relaxed) &&
                    appleFrontHyperPhase_.load(std::memory_order_relaxed)==0 &&
                    v.data==speaker_.clip(9).pcm.data());
                mixed+=static_cast<std::int32_t>(v.data[static_cast<std::size_t>(off)]*
                    (frontLow?1.15:1.0));
            }

            if(appleFrontActive_.load(std::memory_order_acquire)){
                // HF60 direct original Apple II SPA 6502 execution.
                // The same virtual speaker continues across normal/ramp/tight-loop;
                // there are no pre-recorded toggle loops, click-period boundaries,
                // separate low oscillator or double-played sound(9).
                const int phase=appleFrontHyperPhase_.load(std::memory_order_relaxed);
                const int speed=(phase==2 || phase==3)?1:
                    appleFrontSpeed_.load(std::memory_order_relaxed);
                mixed+=static_cast<std::int32_t>(appleLive_.nextSample(speed,phase)*9500.0);
            }

            // HF73: reduce *all* Front View/HJ audio to 40% at the final mixer.
            // Unlike an EQ, this reliably changes loudness while preserving
            // the original Apple waveform, frequency balance and CPU timing.
            if(appleFrontActive_.load(std::memory_order_relaxed))
                mixed=static_cast<std::int32_t>(mixed*0.40);
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
    const auto logicalStartUs=perfUs();
    const auto us=speaker_.sourceDurationUs(soundId);
    if(!audioAvailable_){
        waitFallbackDurationUs(us);
        return;
    }

#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
    if(hostAudio_){
        if(sendHostAudioSound(soundId)){
            if(trace_){
                std::cerr << "[SFXTRACE] t_us=" << (perfUs()-traceEpochCounter_)
                          << " host_play id=" << soundId
                          << " source_us=" << us << "\n";
            }
            // OUT.EXE sound() is synchronous for the source routine itself.
            // The approved PCM tail continues independently in the Windows
            // helper, exactly as the M241 mixer allowed tails to overlap.
            waitFallbackDurationUs(us);
            synchronousEndSample_.store(renderedSamples_.load(std::memory_order_acquire),
                                        std::memory_order_release);
            return;
        }
        // Do not manufacture delayed duplicate audio if the host helper dies.
        // Preserve original source pacing and report the bridge loss.
        waitFallbackDurationUs(us);
        return;
    }
#endif

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
#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
        pulseAudio_ ? static_cast<std::uint32_t>(pulseAudio_->negotiated.minreq/sizeof(std::int16_t)) :
#endif
        static_cast<std::uint32_t>(spec_.samples));

#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
    if(pulseAudio_ && !pulseAudio_->latencyLogged.exchange(true,std::memory_order_acq_rel)){
        pa_usec_t latencyUs=0;
        int negative=0;
        pa_threaded_mainloop_lock(pulseAudio_->mainloop);
        const int latencyResult=pa_stream_get_latency(pulseAudio_->stream,&latencyUs,&negative);
        pa_threaded_mainloop_unlock(pulseAudio_->mainloop);
        if(latencyResult>=0){
            audioDiagAppend(std::string("PulseAudio reported stream latency_us=")+
                            std::to_string(latencyUs)+
                            ", negative="+std::to_string(negative));
        } else {
            pulseAudio_->latencyLogged.store(false,std::memory_order_release);
        }
    }
#endif

    if(trace_){
        std::cerr << "[SFXTRACE] t_us=" << (perfUs()-traceEpochCounter_)
                  << " play_begin id=" << soundId
                  << " rendered=" << startSample
                  << " source_end=" << sourceEndSample
                  << " callback_frames=" << traceFrames
                  << " app_queue_samples=0\n";
    }

    // OUT.EXE sound() is synchronous. Tie its return to the audio render
    // consumption clock; M238 changes only the host output transport.
    waitRenderedThrough(sourceEndSample,us+1000000u);
    // A direct Pulse write callback may be asked for several milliseconds in a
    // single burst. OUT.EXE sound() still blocks for its source routine span,
    // so do not let a fast producer callback shorten that original duration.
    const auto logicalEndUs=logicalStartUs+static_cast<std::uint64_t>(us);
    while(perfUs()<logicalEndUs) SDL_Delay(0);

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

#if !defined(_WIN32) && defined(U1_HAVE_LIBPULSE)
namespace {
bool fdWriteAll(int fd,const void* data,std::size_t bytes){
    const auto* p=static_cast<const std::uint8_t*>(data);
    while(bytes){
        const auto n=::write(fd,p,bytes);
        if(n>0){ p+=n; bytes-=static_cast<std::size_t>(n); continue; }
        if(n<0 && errno==EINTR) continue;
        return false;
    }
    return true;
}

bool fdReadLineTimeout(int fd,std::string& line,int timeoutMs){
    line.clear();
    const auto deadline=perfUs()+static_cast<std::uint64_t>(timeoutMs)*1000ull;
    while(perfUs()<deadline){
        const auto remainUs=deadline-perfUs();
        pollfd pfd{fd,POLLIN,0};
        const int waitMs=static_cast<int>(std::max<std::uint64_t>(1,std::min<std::uint64_t>(100,remainUs/1000ull)));
        const int pr=::poll(&pfd,1,waitMs);
        if(pr<0 && errno==EINTR) continue;
        if(pr<=0) continue;
        if(!(pfd.revents&POLLIN)){
            if(pfd.revents&(POLLERR|POLLHUP|POLLNVAL)) return false;
            continue;
        }
        char c=0;
        const auto n=::read(fd,&c,1);
        if(n<=0) return false;
        if(c=='\n') return true;
        if(c!='\r') line.push_back(c);
        if(line.size()>4096) return false;
    }
    return false;
}
}

bool SfxSystem::initHostAudioBridge(){
    std::signal(SIGPIPE,SIG_IGN);
    if(!std::getenv("WSL_INTEROP") && !std::getenv("WSL_DISTRO_NAME")) return false;
    hostAudio_=std::make_unique<HostAudioBridgeState>();
    auto* state=hostAudio_.get();
    auto bridgeFail=[&](const std::string& why){
        state->failure=why;
        std::cerr << "PC speaker emulator: M246 host bridge: " << why << "\n";
        std::ofstream out("audio_runtime_m246.txt",std::ios::trunc);
        out << "driver=windows-host-wasapi-bridge, status=fallback, reason=" << why << "\n";
        shutdownHostAudioBridge();
        return false;
    };

    int toChild[2]{-1,-1};
    int fromChild[2]{-1,-1};
    if(::pipe(toChild)!=0 || ::pipe(fromChild)!=0){
        if(toChild[0]>=0){::close(toChild[0]);::close(toChild[1]);}
        if(fromChild[0]>=0){::close(fromChild[0]);::close(fromChild[1]);}
        state->failure="pipe() failed";
        std::cerr << "PC speaker emulator: M246 host bridge: " << state->failure << "\n";
        return false;
    }

    const pid_t pid=::fork();
    if(pid<0){
        state->failure="fork() failed";
        ::close(toChild[0]);::close(toChild[1]);
        ::close(fromChild[0]);::close(fromChild[1]);
        std::cerr << "PC speaker emulator: M246 host bridge: " << state->failure << "\n";
        return false;
    }
    if(pid==0){
        ::dup2(toChild[0],STDIN_FILENO);
        ::dup2(fromChild[1],STDOUT_FILENO);
        ::close(toChild[0]);::close(toChild[1]);
        ::close(fromChild[0]);::close(fromChild[1]);
        const char* cmd="exec powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"$(wslpath -w ./tools/u1_audio_host.ps1)\"";
        ::execl("/bin/sh","sh","-c",cmd,static_cast<char*>(nullptr));
        _exit(127);
    }

    ::close(toChild[0]);
    ::close(fromChild[1]);
    state->child=pid;
    state->writeFd=toChild[1];
    state->readFd=fromChild[0];

    std::string line;
    if(!fdReadLineTimeout(state->readFd,line,10000) || line.rfind("READY ",0)!=0){
        const std::string why=line.empty()?"helper READY timeout":("helper startup: "+line);
        return bridgeFail(why);
    }
    state->backend=line.substr(6);

    const char magic[4]={'U','1','A','1'};
    const std::int32_t count=10;
    if(!fdWriteAll(state->writeFd,magic,sizeof(magic)) ||
       !fdWriteAll(state->writeFd,&count,sizeof(count))){
        return bridgeFail("helper clip header write failed");
    }
    for(int id=0;id<10;++id){
        const auto& clip=speaker_.clip(id);
        const auto frames=static_cast<std::int32_t>(clip.pcm.size());
        if(!fdWriteAll(state->writeFd,&frames,sizeof(frames)) ||
           (!clip.pcm.empty() && !fdWriteAll(state->writeFd,clip.pcm.data(),clip.pcm.size()*sizeof(std::int16_t)))){
            return bridgeFail("helper PCM upload failed");
        }
    }
    if(!fdReadLineTimeout(state->readFd,line,5000) || line!="LOADED"){
        const std::string why=line.empty()?"helper LOADED timeout":("helper load: "+line);
        return bridgeFail(why);
    }
    return true;
}

bool SfxSystem::sendHostAudioSound(int soundId){
    if(!hostAudio_ || hostAudio_->writeFd<0 || soundId<0 || soundId>9) return false;
    const std::uint8_t cmd[2]={0x01u,static_cast<std::uint8_t>(soundId)};
    std::lock_guard<std::mutex> lock(hostAudio_->writeMutex);
    if(fdWriteAll(hostAudio_->writeFd,cmd,sizeof(cmd))) return true;
    hostAudio_->failure="host audio command pipe closed";
    std::ofstream out("audio_runtime_m246.txt",std::ios::app);
    out << hostAudio_->failure << "\n";
    return false;
}

void SfxSystem::shutdownHostAudioBridge(){
    if(!hostAudio_) return;
    auto* state=hostAudio_.get();
    if(state->writeFd>=0){
        const std::uint8_t quit=0xffu;
        ::write(state->writeFd,&quit,1);
        ::close(state->writeFd);
        state->writeFd=-1;
    }
    if(state->readFd>=0){ ::close(state->readFd); state->readFd=-1; }
    if(state->child>0){
        int status=0;
        bool done=false;
        for(int i=0;i<50;++i){
            const pid_t r=::waitpid(state->child,&status,WNOHANG);
            if(r==state->child){ done=true; break; }
            if(r<0){ done=true; break; }
            ::usleep(10000);
        }
        if(!done){
            ::kill(state->child,SIGTERM);
            ::waitpid(state->child,&status,0);
        }
        state->child=-1;
    }
    hostAudio_.reset();
}

bool SfxSystem::initPulseLowLatency(){
    pulseAudio_=std::make_unique<PulseAudioState>();
    auto* state=pulseAudio_.get();
    bool mainloopStarted=false;

    auto cleanup=[&](){
        if(state->mainloop && mainloopStarted) pa_threaded_mainloop_lock(state->mainloop);
        if(state->stream){
            pa_stream_set_write_callback(state->stream,nullptr,nullptr);
            pa_stream_set_underflow_callback(state->stream,nullptr,nullptr);
            pa_stream_disconnect(state->stream);
            pa_stream_unref(state->stream);
            state->stream=nullptr;
        }
        if(state->context){
            pa_context_disconnect(state->context);
            pa_context_unref(state->context);
            state->context=nullptr;
        }
        if(state->mainloop){
            if(mainloopStarted){
                pa_threaded_mainloop_unlock(state->mainloop);
                pa_threaded_mainloop_stop(state->mainloop);
            }
            pa_threaded_mainloop_free(state->mainloop);
            state->mainloop=nullptr;
        }
    };
    auto fail=[&](const std::string& what){
        state->failure=what;
        cleanup();
        return false;
    };

    state->mainloop=pa_threaded_mainloop_new();
    if(!state->mainloop) return fail("pa_threaded_mainloop_new failed");
    auto* api=pa_threaded_mainloop_get_api(state->mainloop);
    state->context=pa_context_new(api,"Ultima I KR M238");
    if(!state->context) return fail("pa_context_new failed");

    pa_context_set_state_callback(state->context,[](pa_context*,void* userdata){
        auto* st=static_cast<PulseAudioState*>(userdata);
        if(st && st->mainloop) pa_threaded_mainloop_signal(st->mainloop,0);
    },state);

    if(pa_threaded_mainloop_start(state->mainloop)<0) return fail("pa_threaded_mainloop_start failed");
    mainloopStarted=true;
    pa_threaded_mainloop_lock(state->mainloop);
    if(pa_context_connect(state->context,nullptr,PA_CONTEXT_NOFLAGS,nullptr)<0){
        const std::string e=pa_strerror(pa_context_errno(state->context));
        pa_threaded_mainloop_unlock(state->mainloop);
        return fail("pa_context_connect: "+e);
    }
    while(true){
        const auto cs=pa_context_get_state(state->context);
        if(cs==PA_CONTEXT_READY) break;
        if(!PA_CONTEXT_IS_GOOD(cs)){
            const std::string e=pa_strerror(pa_context_errno(state->context));
            pa_threaded_mainloop_unlock(state->mainloop);
            return fail("PulseAudio context state: "+e);
        }
        pa_threaded_mainloop_wait(state->mainloop);
    }

    const pa_sample_spec ss{PA_SAMPLE_S16LE,PcSpeakerEmulator::SAMPLE_RATE,1};
    state->stream=pa_stream_new(state->context,"PC speaker SFX",&ss,nullptr);
    if(!state->stream){
        const std::string e=pa_strerror(pa_context_errno(state->context));
        pa_threaded_mainloop_unlock(state->mainloop);
        return fail("pa_stream_new: "+e);
    }

    pa_stream_set_state_callback(state->stream,[](pa_stream*,void* userdata){
        auto* st=static_cast<PulseAudioState*>(userdata);
        if(st && st->mainloop) pa_threaded_mainloop_signal(st->mainloop,0);
    },state);
    pa_stream_set_write_callback(state->stream,[](pa_stream* stream,std::size_t nbytes,void* userdata){
        auto* self=static_cast<SfxSystem*>(userdata);
        if(!self || !stream || nbytes<sizeof(std::int16_t)) return;

        // Render straight into PulseAudio's writable memblock. This avoids both
        // SDL's mix buffer and a per-callback heap allocation/copy. The block is
        // only the server-requested immediate write span, not a future PCM FIFO.
        void* data=nullptr;
        std::size_t writable=nbytes;
        if(pa_stream_begin_write(stream,&data,&writable)<0 || !data) return;
        writable-=writable%sizeof(std::int16_t);
        if(writable==0) return;
        const std::size_t frames=writable/sizeof(std::int16_t);
        self->renderCallback(static_cast<std::int16_t*>(data),frames);
        if(pa_stream_write(stream,data,writable,nullptr,0,PA_SEEK_RELATIVE)<0){
            // PulseAudio will transition the stream state on a hard failure;
            // the callback must not block or manufacture retry delay here.
        }
    },this);
    pa_stream_set_underflow_callback(state->stream,[](pa_stream*,void* userdata){
        auto* st=static_cast<PulseAudioState*>(userdata);
        if(st) st->underflows.fetch_add(1,std::memory_order_relaxed);
    },state);

    pa_buffer_attr attr{};
    attr.maxlength=static_cast<std::uint32_t>(-1);
    attr.tlength=pa_usec_to_bytes(PULSE_TARGET_LATENCY_US,&ss);
    attr.prebuf=static_cast<std::uint32_t>(-1);
    attr.minreq=static_cast<std::uint32_t>(-1);
    attr.fragsize=static_cast<std::uint32_t>(-1);
    const auto flags=static_cast<pa_stream_flags_t>(
        PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING);

    if(pa_stream_connect_playback(state->stream,nullptr,&attr,flags,nullptr,nullptr)<0){
        const std::string e=pa_strerror(pa_context_errno(state->context));
        pa_threaded_mainloop_unlock(state->mainloop);
        return fail("pa_stream_connect_playback: "+e);
    }
    while(true){
        const auto ssState=pa_stream_get_state(state->stream);
        if(ssState==PA_STREAM_READY) break;
        if(!PA_STREAM_IS_GOOD(ssState)){
            const std::string e=pa_strerror(pa_context_errno(state->context));
            pa_threaded_mainloop_unlock(state->mainloop);
            return fail("PulseAudio stream state: "+e);
        }
        pa_threaded_mainloop_wait(state->mainloop);
    }

    if(const auto* got=pa_stream_get_buffer_attr(state->stream)) state->negotiated=*got;
    else state->negotiated=attr;
    pa_threaded_mainloop_unlock(state->mainloop);

    spec_=SDL_AudioSpec{};
    spec_.freq=PcSpeakerEmulator::SAMPLE_RATE;
    spec_.format=AUDIO_S16SYS;
    spec_.channels=1;
    const auto frames=state->negotiated.minreq==static_cast<std::uint32_t>(-1)
        ? REQUESTED_SAMPLES
        : std::max<std::uint32_t>(1,state->negotiated.minreq/sizeof(std::int16_t));
    spec_.samples=static_cast<Uint16>(std::min<std::uint32_t>(frames,65535u));
    return true;
}

void SfxSystem::shutdownPulseLowLatency(){
    if(!pulseAudio_) return;
    auto* state=pulseAudio_.get();
    audioDiagAppend(std::string("PulseAudio underflows=")+
                    std::to_string(state->underflows.load(std::memory_order_relaxed)));
    if(state->mainloop) pa_threaded_mainloop_lock(state->mainloop);
    if(state->stream){
        pa_stream_set_write_callback(state->stream,nullptr,nullptr);
        pa_stream_set_underflow_callback(state->stream,nullptr,nullptr);
        pa_stream_disconnect(state->stream);
        pa_stream_unref(state->stream);
        state->stream=nullptr;
    }
    if(state->context){
        pa_context_disconnect(state->context);
        pa_context_unref(state->context);
        state->context=nullptr;
    }
    if(state->mainloop){
        pa_threaded_mainloop_unlock(state->mainloop);
        pa_threaded_mainloop_stop(state->mainloop);
        pa_threaded_mainloop_free(state->mainloop);
        state->mainloop=nullptr;
    }
    pulseAudio_.reset();
}
#endif

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

            // MMCSS avoids scheduler jitter without adding an application queue.
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
