#include "PcSpeakerEmulator.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace {
constexpr std::array<std::uint32_t,10> SOURCE_US = {
    47574,50981,77948,98631,4302,899246,39064,248070,97042,936,
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
        default: break;
    }
    return w;
}

struct Biquad {
    double b0=1,b1=0,b2=0,a1=0,a2=0;
    double x1=0,x2=0,y1=0,y2=0;
    double run(double x){
        const double y=b0*x+b1*x1+b2*x2-a1*y1-a2*y2;
        x2=x1; x1=x; y2=y1; y1=y; return y;
    }
};

Biquad makeLowPass(double fc,double q){
    const double sr=PcSpeakerEmulator::SAMPLE_RATE;
    const double w0=2.0*3.14159265358979323846*fc/sr;
    const double c=std::cos(w0),s=std::sin(w0);
    const double alpha=s/(2.0*q),a0=1.0+alpha;
    Biquad f;
    f.b0=((1.0-c)*0.5)/a0; f.b1=(1.0-c)/a0; f.b2=f.b0;
    f.a1=(-2.0*c)/a0; f.a2=(1.0-alpha)/a0; return f;
}
Biquad makeHighPass(double fc,double q){
    const double sr=PcSpeakerEmulator::SAMPLE_RATE;
    const double w0=2.0*3.14159265358979323846*fc/sr;
    const double c=std::cos(w0),s=std::sin(w0);
    const double alpha=s/(2.0*q),a0=1.0+alpha;
    Biquad f;
    f.b0=((1.0+c)*0.5)/a0; f.b1=(-(1.0+c))/a0; f.b2=f.b0;
    f.a1=(-2.0*c)/a0; f.a2=(1.0-alpha)/a0; return f;
}
}

PcSpeakerEmulator::PcSpeakerEmulator(){
    for(int i=0;i<10;++i) clips_[static_cast<std::size_t>(i)]=render(i);
}
const PcSpeakerEmulator::Clip& PcSpeakerEmulator::clip(int soundId) const{
    static const Clip empty{};
    return (soundId>=0&&soundId<10)?clips_[static_cast<std::size_t>(soundId)]:empty;
}
std::uint32_t PcSpeakerEmulator::sourceDurationUs(int soundId) const{
    return (soundId>=0&&soundId<10)?SOURCE_US[static_cast<std::size_t>(soundId)]:0u;
}
std::uint32_t PcSpeakerEmulator::sourceDurationMsCeil(int soundId) const{
    const auto us=sourceDurationUs(soundId); return (us+999u)/1000u;
}

PcSpeakerEmulator::Clip PcSpeakerEmulator::render(int soundId){
    Clip out;
    if(soundId<0||soundId>9) return out;
    const auto weights=delayWeights(soundId);
    if(weights.empty()) return out;

    const std::uint64_t sourceSamples64=
        (static_cast<std::uint64_t>(SOURCE_US[static_cast<std::size_t>(soundId)])*
         SAMPLE_RATE+999999u)/1000000u;
    out.sourceSamples=static_cast<std::uint32_t>(std::max<std::uint64_t>(1,sourceSamples64));

    constexpr std::uint32_t FILTER_TAIL_MS=32;
    const std::size_t tailSamples=static_cast<std::size_t>(FILTER_TAIL_MS)*SAMPLE_RATE/1000u;
    const std::size_t totalSamples=static_cast<std::size_t>(out.sourceSamples)+tailSamples;
    std::vector<double> target(totalSamples,0.0);

    std::uint64_t totalWeight=0;
    for(int n:weights) totalWeight+=static_cast<std::uint64_t>(n);
    std::uint64_t cumulative=0;
    const bool immediateFirstWrite=(soundId==0||soundId==1||soundId==8);
    int level=-1;
    std::size_t begin=0;
    if(immediateFirstWrite){
        for(int n:weights){
            cumulative+=static_cast<std::uint64_t>(n);
            std::size_t end=static_cast<std::size_t>(
                (cumulative*static_cast<std::uint64_t>(out.sourceSamples)+totalWeight/2u)/totalWeight);
            end=std::min<std::size_t>(end,out.sourceSamples);
            if(end<begin) end=begin;
            for(std::size_t i=begin;i<end;++i) target[i]=static_cast<double>(level);
            level=-level; begin=end;
        }
    }else{
        bool first=true;
        for(int n:weights){
            cumulative+=static_cast<std::uint64_t>(n);
            std::size_t end=static_cast<std::size_t>(
                (cumulative*static_cast<std::uint64_t>(out.sourceSamples)+totalWeight/2u)/totalWeight);
            end=std::min<std::size_t>(end,out.sourceSamples);
            if(end<begin) end=begin;
            if(!first){
                for(std::size_t i=begin;i<end;++i) target[i]=static_cast<double>(level);
                level=-level;
            }
            first=false; begin=end;
        }
    }

    const double fullTransitionSamples=0.000070*static_cast<double>(SAMPLE_RATE);
    const double maxStep=2.0/std::max(1.0,fullTransitionSamples);
    double speaker=0.0;
    auto lp=makeLowPass(4300.0,0.7071067811865476);
    auto hp=makeHighPass(120.0,0.7071067811865476);

    out.pcm.resize(totalSamples);
    constexpr double AMPLITUDE=5400.0;
    for(std::size_t i=0;i<totalSamples;++i){
        const double wanted=(i<out.sourceSamples)?target[i]:0.0;
        const double diff=wanted-speaker;
        speaker+=std::clamp(diff,-maxStep,maxStep);
        const double filtered=hp.run(lp.run(speaker));
        const auto s=static_cast<long>(std::llround(filtered*AMPLITUDE));
        out.pcm[i]=static_cast<std::int16_t>(std::clamp<long>(
            s,std::numeric_limits<std::int16_t>::min(),
              std::numeric_limits<std::int16_t>::max()));
    }
    return out;
}
