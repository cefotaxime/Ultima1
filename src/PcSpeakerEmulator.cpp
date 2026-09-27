#include "PcSpeakerEmulator.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace {
constexpr std::array<std::uint32_t,10> SOURCE_US = {
    47574, 50981, 77948, 98631, 4302,
    899246, 39064, 248070, 97042, 936,
};

constexpr std::array<std::uint16_t,8> SOUND4_INTERVALS_48K = {
    12, 21, 21, 21, 22, 22, 22, 22,
};

std::vector<int> delayWeights(int id){
    std::vector<int> w;
    switch(id){
        case 0: w.assign(16,1000); break;
        case 1: w.assign(48,355); break;
        case 2:
        case 3:
        case 4: {
            const int count=(id==2?200:(id==3?255:8));
            std::uint8_t dh=0x7e;
            w.reserve(static_cast<std::size_t>(count));
            for(int i=0;i<count;++i){
                dh=static_cast<std::uint8_t>(dh+5u);
                w.push_back(dh?static_cast<int>(dh):256);
            }
            break;
        }
        case 5:
        case 7: {
            std::uint8_t ch=static_cast<std::uint8_t>(id==5?0xc0:0x05);
            std::uint8_t cl=1;
            const int dlCount=(id==5?0x3c:0x0f);
            for(;;){
                for(int dl=0;dl<dlCount;++dl){
                    w.push_back(ch?static_cast<int>(ch):256);
                    w.push_back(cl?static_cast<int>(cl):256);
                }
                ch=static_cast<std::uint8_t>(ch-1u);
                cl=static_cast<std::uint8_t>(cl+1u);
                if(cl==0x1b) break;
            }
            break;
        }
        case 6:
            for(int bh=0xa0;bh>=1;--bh) w.push_back(bh);
            break;
        case 8:
            for(int n=5;n<=255;++n) w.push_back(n);
            break;
        case 9:
            w={5,10,15,20};
            break;
        default:
            break;
    }
    return w;
}

struct FirstOrder {
    double b0=1.0,b1=0.0,a1=0.0;
    double x1=0.0,y1=0.0;
    double run(double x){
        const double y=b0*x+b1*x1-a1*y1;
        x1=x; y1=y; return y;
    }
};

struct Biquad {
    double b0=1.0,b1=0.0,b2=0.0,a1=0.0,a2=0.0;
    double x1=0.0,x2=0.0,y1=0.0,y2=0.0;
    double run(double x){
        const double y=b0*x+b1*x1+b2*x2-a1*y1-a2*y2;
        x2=x1; x1=x; y2=y1; y1=y; return y;
    }
};

FirstOrder makeFirstOrderLowPass(double fc){
    const double k=std::tan(3.14159265358979323846*fc/PcSpeakerEmulator::SAMPLE_RATE);
    const double n=1.0/(1.0+k);
    FirstOrder f;
    f.b0=k*n; f.b1=f.b0; f.a1=(k-1.0)*n;
    return f;
}
FirstOrder makeFirstOrderHighPass(double fc){
    const double k=std::tan(3.14159265358979323846*fc/PcSpeakerEmulator::SAMPLE_RATE);
    const double n=1.0/(1.0+k);
    FirstOrder f;
    f.b0=n; f.b1=-n; f.a1=(k-1.0)*n;
    return f;
}
Biquad makeSecondOrderLowPass(double fc,double q){
    const double k=std::tan(3.14159265358979323846*fc/PcSpeakerEmulator::SAMPLE_RATE);
    const double n=1.0/(1.0+k/q+k*k);
    Biquad f;
    f.b0=k*k*n; f.b1=2.0*f.b0; f.b2=f.b0;
    f.a1=2.0*(k*k-1.0)*n; f.a2=(1.0-k/q+k*k)*n;
    return f;
}
Biquad makeSecondOrderHighPass(double fc,double q){
    const double k=std::tan(3.14159265358979323846*fc/PcSpeakerEmulator::SAMPLE_RATE);
    const double n=1.0/(1.0+k/q+k*k);
    Biquad f;
    f.b0=n; f.b1=-2.0*n; f.b2=n;
    f.a1=2.0*(k*k-1.0)*n; f.a2=(1.0-k/q+k*k)*n;
    return f;
}

constexpr int IMPULSE_OS=32;
constexpr int IMPULSE_QUALITY=150;
constexpr double REF_RATE=32000.0;
constexpr double OUTPUT_RATE=48000.0;
constexpr double CUTOFF_MARGIN=(OUTPUT_RATE/REF_RATE)*(2.0+0.2)-2.0;
constexpr double IMPULSE_GAIN=REF_RATE/OUTPUT_RATE;
constexpr double SINC_FADE=0.999;
constexpr double PWM_AMPLITUDE=16383.0;
constexpr double OUTPUT_GAIN=0.50;

inline double sinc(double x){
    return x==0.0?1.0:std::sin(x)/x;
}

std::array<double,IMPULSE_QUALITY*IMPULSE_OS> makeImpulseLut(){
    std::array<double,IMPULSE_QUALITY*IMPULSE_OS> lut{};
    constexpr double fs=OUTPUT_RATE;
    constexpr double fc=fs/(2.0+CUTOFF_MARGIN);
    constexpr double q=static_cast<double>(IMPULSE_QUALITY);
    constexpr double midpoint=q/(2.0*fs);
    for(std::size_t i=0;i<lut.size();++i){
        const double t=static_cast<double>(i)/(fs*IMPULSE_OS);
        if(t<=0.0 || t*fs>=q) continue;
        const double window=1.0+std::cos(2.0*fs*3.14159265358979323846*(midpoint-t)/q);
        lut[i]=IMPULSE_GAIN*window*sinc(2.0*fc*3.14159265358979323846*(t-midpoint))/2.0;
    }
    return lut;
}

using Event=std::pair<double,double>;

std::vector<Event> transitionEvents(int soundId,std::uint32_t sourceSamples){
    std::vector<Event> ev;
    if(soundId==4){
        std::uint32_t pos=0;
        double level=-PWM_AMPLITUDE;
        for(std::size_t i=0;i<SOUND4_INTERVALS_48K.size();++i){
            pos+=SOUND4_INTERVALS_48K[i];
            if(i==0){
                ev.push_back({static_cast<double>(pos),level});
            }else if(i+1<SOUND4_INTERVALS_48K.size()){
                level=-level;
                ev.push_back({static_cast<double>(pos),level});
            }else{
                ev.push_back({static_cast<double>(pos),0.0});
            }
        }
        return ev;
    }

    const auto w=delayWeights(soundId);
    if(w.empty()) return ev;
    std::uint64_t total=0;
    for(int n:w) total+=static_cast<std::uint64_t>(n);
    if(!total) return ev;

    const bool immediate=(soundId==0 || soundId==1 || soundId==8);
    double level=-PWM_AMPLITUDE;
    std::uint64_t cumulative=0;
    if(immediate) ev.push_back({0.0,level});
    for(std::size_t i=0;i<w.size();++i){
        cumulative+=static_cast<std::uint64_t>(w[i]);
        const double pos=static_cast<double>(cumulative)*sourceSamples/static_cast<double>(total);
        if(!immediate && i==0){
            ev.push_back({pos,level});
            continue;
        }
        if(i+1<w.size()){
            level=-level;
            ev.push_back({pos,level});
        }
    }
    ev.push_back({static_cast<double>(sourceSamples),0.0});
    return ev;
}

} // namespace

PcSpeakerEmulator::PcSpeakerEmulator(){
    for(int i=0;i<10;++i) clips_[static_cast<std::size_t>(i)]=render(i);
}

const PcSpeakerEmulator::Clip& PcSpeakerEmulator::clip(int soundId) const{
    static const Clip empty{};
    return (soundId>=0 && soundId<10)?clips_[static_cast<std::size_t>(soundId)]:empty;
}

std::uint32_t PcSpeakerEmulator::sourceDurationUs(int soundId) const{
    return (soundId>=0 && soundId<10)?SOURCE_US[static_cast<std::size_t>(soundId)]:0u;
}

std::uint32_t PcSpeakerEmulator::sourceDurationMsCeil(int soundId) const{
    const auto us=sourceDurationUs(soundId);
    return (us+999u)/1000u;
}

PcSpeakerEmulator::Clip PcSpeakerEmulator::render(int soundId){
    Clip out;
    if(soundId<0 || soundId>9) return out;
    const auto sourceSamples64=(static_cast<std::uint64_t>(SOURCE_US[static_cast<std::size_t>(soundId)])*
                                SAMPLE_RATE + 999999u)/1000000u;
    out.sourceSamples=static_cast<std::uint32_t>(std::max<std::uint64_t>(1,sourceSamples64));

    constexpr std::uint32_t TAIL_MS=32;
    const std::size_t tail=static_cast<std::size_t>(TAIL_MS)*SAMPLE_RATE/1000u;
    const std::size_t total=static_cast<std::size_t>(out.sourceSamples)+tail;
    std::vector<double> impulses(total+IMPULSE_QUALITY+2,0.0);
    const auto lut=makeImpulseLut();

    double previous=0.0;
    for(const auto& [samplePos,amplitude]:transitionEvents(soundId,out.sourceSamples)){
        if(amplitude==previous) continue;
        previous=amplitude;
        const double base=std::clamp(samplePos,0.0,static_cast<double>(out.sourceSamples));
        const auto phaseRaw=static_cast<int>(base*IMPULSE_OS)%IMPULSE_OS;
        const auto offset=static_cast<std::size_t>(base)+(phaseRaw?1u:0u);
        const auto phase=phaseRaw?(IMPULSE_OS-phaseRaw):0;
        const auto kStart=phaseRaw?0:1;
        for(int k=kStart;k<IMPULSE_QUALITY;++k){
            const std::size_t dst=offset+static_cast<std::size_t>(k);
            if(dst>=impulses.size()) break;
            impulses[dst]+=amplitude*lut[static_cast<std::size_t>(phase+k*IMPULSE_OS)];
        }
    }

    auto hp1=makeFirstOrderHighPass(120.0);
    auto hp2=makeSecondOrderHighPass(120.0,1.0);
    auto lp1=makeFirstOrderLowPass(4300.0);
    auto lp2=makeSecondOrderLowPass(4300.0,1.0);

    out.pcm.resize(total);
    double accumulator=0.0;
    for(std::size_t i=0;i<total;++i){
        accumulator+=impulses[i];
        double v=accumulator;
        accumulator*=SINC_FADE;
        v=hp2.run(hp1.run(v));
        v=lp2.run(lp1.run(v));
        const auto s=static_cast<long>(std::llround(v*OUTPUT_GAIN));
        out.pcm[i]=static_cast<std::int16_t>(std::clamp<long>(
            s,std::numeric_limits<std::int16_t>::min(),std::numeric_limits<std::int16_t>::max()));
    }
    return out;
}
