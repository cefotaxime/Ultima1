#include "PcSpeakerEmulator.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main(){
    PcSpeakerEmulator e;
    constexpr std::array<std::uint32_t,10> us={
        47574,50981,77948,98631,4302,899246,39064,248070,97042,936
    };
    for(int i=0;i<10;++i){
        const auto& c=e.clip(i);
        assert(!c.pcm.empty());
        assert(c.sourceSamples>0);
        assert(c.pcm.size()>c.sourceSamples);
        assert(e.sourceDurationUs(i)==us[static_cast<std::size_t>(i)]);
        bool nonzero=false;
        for(auto s:c.pcm) if(s!=0){nonzero=true;break;}
        assert(nonzero);
    }
    const auto& foot=e.clip(4);
    std::size_t onset=0;
    while(onset<foot.pcm.size() && (foot.pcm[onset]>=-100 && foot.pcm[onset]<=100)) ++onset;
    assert(onset>=70 && onset<=105);
    assert(foot.sourceSamples==207);
    int peak=0; for(auto v:foot.pcm) peak=std::max(peak,std::abs((int)v));
    assert(peak>=7500 && peak<=9500);
    std::cout << "M235 PC-speaker impulse renderer / footstep tone smoke OK\n";
}
