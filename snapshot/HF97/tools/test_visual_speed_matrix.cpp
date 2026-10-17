#include "../src/Apple1986Live.h"
#include "../src/SpaceSystem.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
int main(){
 constexpr int rate=44100;
 for(int speed=1;speed<=8;++speed){
  SpaceSystem system;SpaceState state{};PlayerSave save{};
  state.active=true;state.frontView=true;state.frontSpeed=speed;
  state.sectorX=3;state.sectorY=3;state.currentSlot=2;
  auto &craft=state.sectors[3*7+3].craft[2];
  craft.type=0;craft.fuel=1000;craft.shield=999;craft.heading=1;
  std::vector<std::string> msg;
  Apple1986Live cpu;cpu.reset(speed);
  system.hyperJump(state,save,msg);
  bool saw[6]={};int last=0;int complete=-1;
  for(int i=0;i<rate*65;++i){
   float audio=cpu.nextSample(speed,state.hyperPhase);
   (void)audio;
   if(i%735==0){
    auto packed=cpu.hyperStatus();
    int st=int((packed>>56)&255);
    if(st>=1&&st<=5)saw[st]=true;
    last=st;
    system.updateAppleHyperProgress(state,save,msg,packed);
    if(state.hyperPhase==0){complete=i;break;}
   }
  }
  bool good=(complete>=0 && state.active && craft.fuel==900 && saw[2] && saw[3] && saw[5]);
  std::cout<<"speed "<<speed<<" hyper_visual_completed_sec "<<double(complete)/rate
   <<" fuel="<<craft.fuel<<" alive="<<state.active
   <<" stages="<<saw[1]<<saw[2]<<saw[3]<<saw[4]<<saw[5]
   <<" OK="<<good<<std::endl;
  if(!good)return 2;
 }
}
