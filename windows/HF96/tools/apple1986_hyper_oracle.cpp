#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <map>
extern "C" {
#include "../third_party/fake6502/fake6502.h"
}
static std::array<uint8_t,65536> ram{};
static uint64_t ticks=0, toggles=0;
static std::vector<uint64_t> events;
static std::map<unsigned,std::array<unsigned,3>> iosites;

extern "C" uint8_t read6502(uint16_t address) {
    if(address==0xc030){++toggles; events.push_back(ticks);}
    if(address==0xc000 || (address>=0xc0e0 && address<0xc0f0)){
      auto &stat=iosites[PC];stat[0]++;stat[1]=address;stat[2]=ram[address];
    }
    if(address==0xc000 || address==0xc010 || address==0xc0e8)return 0;
    return ram[address];
}
extern "C" void write6502(uint16_t address,uint8_t value){ram[address]=value;}
static void load(const char* path,unsigned base) {
    std::ifstream f(path,std::ios::binary);
    if(!f) throw std::runtime_error(path);
    for(unsigned i=base;f && i<65536;++i) {char v; if(!f.get(v))break;ram[i]=static_cast<uint8_t>(v);}
}
int main(int argc,char**argv){
    if(argc!=5){std::cerr<<"usage: oracle SPA.bin STUPH.bin\n";return 2;}
    load(argv[4],0x0); // full authentic running SPA RAM, including self-modification
    // preserve live state
    reset6502();setP(0x30);
    std::cout<<"live $93f4="<<unsigned(ram[0x93f4])<<"\n"; PC=0x9811;SP=0xfd;ram[0x1fe]=0xfe;ram[0x1ff]=0xff;
    std::uint64_t frameSteps=0;
    int phaseA=0,phaseB=0,phaseC=0,phaseD=0;
    std::map<unsigned,std::array<unsigned,3>> pcs;
    for(;frameSteps<16000000;++frameSteps){
      if(PC==0x989b)break;
      if(PC==0x9824)++phaseA;
      if(PC==0x9868)++phaseB;
      if(PC==0x9886)++phaseC;
      if(PC==0x988d)++phaseD;
      auto addr=PC; auto op=ram[PC]; auto cycles=step6502();ticks+=cycles;if(addr==0x81e4){static int c=0;if(c++<5)std::cerr<<"PC81E4 cycle "<<cycles<<" op "<<op<<"\n";}auto &pc=pcs[addr];++pc[0];pc[1]+=cycles;pc[2]=op;
    }
    std::cout<<"IO callsites\n";
    for(auto &site:iosites)std::cout<<std::hex<<site.first<<" IO="<<site.second[1]<<std::dec<<" n="<<site.second[0]<<" value="<<site.second[2]<<"\n";
    std::cout<<"hyper steps "<<frameSteps<<" cpu_cycles "<<ticks<<" speaker_toggles "<<toggles
       <<" phase_calls "<<phaseA<<","<<phaseB<<","<<phaseC<<","<<phaseD
       <<" lastpc "<<std::hex<<PC<<std::dec<<"\n";
    std::ofstream cyout("hf67_live_hyper_cxx_cycles_by_pc.txt");for(auto &kv:pcs){char buf[100];sprintf(buf,"%04X %u %u %02X\n",kv.first,kv.second[0],kv.second[1],kv.second[2]);cyout<<buf;}
    std::ofstream evout("hf66_hyper_cxx_toggle_cycles.txt");for(auto x:events)evout<<x<<"\n";

}