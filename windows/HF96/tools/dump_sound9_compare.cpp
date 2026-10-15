#include <fstream>
#include <cstdint>
#include "PcSpeakerEmulator.h"
#include "SfxPcmOracleM225.h"
static void wav(const char* path,const int16_t* data,size_t n){
 std::ofstream f(path,std::ios::binary);
 uint32_t dataBytes=(uint32_t)(n*2), riff=36+dataBytes, rate=48000, byteRate=96000; uint16_t one=1,bits=16,align=2;
 f.write("RIFF",4); f.write((char*)&riff,4); f.write("WAVEfmt ",8);
 uint32_t fmt=16; f.write((char*)&fmt,4); f.write((char*)&one,2); f.write((char*)&one,2);
 f.write((char*)&rate,4); f.write((char*)&byteRate,4); f.write((char*)&align,2); f.write((char*)&bits,2);
 f.write("data",4); f.write((char*)&dataBytes,4); f.write((char*)data,dataBytes);
}
int main(){
 PcSpeakerEmulator e; auto &c=e.clip(9); wav("sound9_current.wav",c.pcm.data(),c.pcm.size());
 auto o=SfxPcmOracleM225::clip(9); wav("sound9_oracle.wav",o.data,o.samples);
 return 0;
}