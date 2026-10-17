#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <map>
#include <algorithm>
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
    PC=0x7000;
    SP=0xfd;
    std::map<unsigned,unsigned> hot;
    for(uint64_t k=0;k<80000000;k++){
      if(PC==0x9477 || PC==0x9811){
        std::cout<<"reached game flight PC "<<std::hex<<PC<<std::dec<<" steps "<<k<<" cycles "<<ticks<<"\n"; break;
      }
      if(k%5000000==0) std::cout<<"progress steps "<<k<<" PC "<<std::hex<<PC<<std::dec<<" cycles "<<ticks<<" speaker "<<toggles<<std::endl;
      if(k>20000000 && PC==0x0000){std::cout<<"PC zero at "<<k<<"\n";break;}
      ++hot[PC];
      ticks+=step6502();
    }
    std::vector<std::pair<unsigned,unsigned>> hs(hot.begin(),hot.end());
    std::sort(hs.begin(),hs.end(),[](auto a,auto b){return a.second>b.second;});
    for(int i=0;i<15 && i<(int)hs.size();i++)std::cout<<std::hex<<hs[i].first<<std::dec<<":"<<hs[i].second<<" ";
    std::cout<<"\n";
}
