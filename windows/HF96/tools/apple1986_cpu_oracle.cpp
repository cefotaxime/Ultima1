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
extern "C" uint8_t read6502(uint16_t address) {
    if(address==0xc030){++toggles; events.push_back(ticks);}
    return ram[address];
}
extern "C" void write6502(uint16_t address,uint8_t value){ram[address]=value;}
static void load(const char* path,unsigned base) {
    std::ifstream f(path,std::ios::binary);
    if(!f) throw std::runtime_error(path);
    for(unsigned i=base;f && i<65536;++i) {char v; if(!f.get(v))break;ram[i]=static_cast<uint8_t>(v);}
}
int main(int argc,char**argv){
    if(argc!=4){std::cerr<<"usage: oracle SPA.bin STUPH.bin\n";return 2;}
    load(argv[3],0x7000);load(argv[1],0x8956);load(argv[2],0x800);
    ram[0x1581]=1;ram[0x4e]=0x3d;ram[0x4f]=0x71;ram[0x93f9]=0;ram[0xba]=0x40;ram[0xbb]=0x50;
    reset6502();setP(0x30);
    std::vector<int> speeds;
    speeds.insert(speeds.end(),64,8);
    for(int s=7;s>=1;--s)speeds.push_back(s);
    speeds.insert(speeds.end(),400,1);
    for(int s=2;s<=8;++s)speeds.push_back(s);
    speeds.insert(speeds.end(),64,8);
    std::map<unsigned,std::array<unsigned,3>> pcs;
    for(size_t j=0;j<speeds.size();++j){
        ram[0x93f4]=speeds[j];PC=0x9477;SP=0xfd;ram[0x1fe]=0xfe;ram[0x1ff]=0xff;
        uint64_t start=ticks,ev=toggles;
        bool returned=false;
        for(int i=0;i<90000;++i){
            if(PC==0xffff){returned=true;break;}
            const unsigned addr=PC; const unsigned op=ram[addr];
            const auto elapsed=step6502();ticks+=elapsed;
            if(j==0){auto &r=pcs[addr]; ++r[0];r[1]+=elapsed;r[2]=op;}
        }
        if(!returned){std::cerr<<"runaway frame "<<j<<" PC="<<std::hex<<PC<<"\n";return 1;}
        if(j==0 || j==63 || j==64 || j==70 || j==71 || j==270 || j==470 || j==471 || j==477 || j==541)
            std::cout<<"frame "<<j<<" speed "<<speeds[j]<<" cycles "<<ticks-start<<" toggles "<<toggles-ev<<"\n";
    }
    std::ofstream diff("hf60_cpp_pc_cycles.txt");
    for(auto &kv:pcs){char buf[100];sprintf(buf,"%04X %u %u %02X\n",kv.first,kv.second[0],kv.second[1],kv.second[2]);diff<<buf;}
    std::ofstream evout("hf60_cxx_toggle_cycles.txt");for(auto x:events)evout<<x<<"\n";
    std::cout<<"frames "<<speeds.size()<<" cpu_cycles "<<ticks<<" speaker_toggles "<<toggles<<"\n";
}