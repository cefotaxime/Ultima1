#include "../src/Apple1986Live.h"
#include <fstream>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <algorithm>
static void w16(std::ofstream &f,uint16_t v){for(int i=0;i<2;i++)f.put((v>>(8*i))&255);}
static void w32(std::ofstream &f,uint32_t v){for(int i=0;i<4;i++)f.put((v>>(8*i))&255);}
int main(){
 Apple1986Live a;a.reset(8);
 constexpr int rate=44100,seconds=11;
 std::ofstream f("hf62_live_cpu_audio.wav",std::ios::binary);
 f.write("RIFF",4);w32(f,36+rate*seconds*2);f.write("WAVEfmt ",8);w32(f,16);w16(f,1);w16(f,1);w32(f,rate);w32(f,rate*2);w16(f,2);w16(f,16);f.write("data",4);w32(f,rate*seconds*2);
 for(int i=0;i<rate*seconds;i++){
  const int phase=i<rate?0:i<9*rate?1:0;
  double y=std::clamp(static_cast<double>(a.nextSample(8,phase))*9500.0,-32768.0,32767.0);
  w16(f,static_cast<uint16_t>(static_cast<int16_t>(y)));
 }
 std::cout<<"wrote 11 second raw original CPU output with captured AppleWin resident RAM: first second normal $9477; next 8s full $9811 jump\n";
}
