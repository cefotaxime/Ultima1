#include "Apple1986Live.h"
#include "AppleSPABytes.h"
#include "AppleSTUPHBytes.h"
#include "AppleMIBytes.h"
#include "AppleLiveSpaceRAM.h"
#include "PcSpeakerEmulator.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>
extern "C" {
#include "../third_party/fake6502/fake6502.h"
}
namespace {
 Apple1986Live* activeCpu=nullptr;
}
extern "C" std::uint8_t read6502(std::uint16_t address);
extern "C" void write6502(std::uint16_t address,std::uint8_t value);
std::uint8_t read6502(std::uint16_t address){
 if(activeCpu==nullptr)return 0;
 if(address==0xc030)activeCpu->events_.push_back(activeCpu->cpuCycles_);
 // Apple II C000 is a keyboard latch; no pending input during
 // sound-only background emulation. A RAM snapshot may contain bit7
 // set from a previously pressed key; treating it as persistent input
 // corrupts the original sound control loop.
 if(address==0xc000) return 0;
 // C010 clears the keyboard strobe (read has side effect).
 if(address==0xc010) return 0;
 // C0E8 is Disk II motor-off soft switch, not audio data.
 if(address==0xc0e8) return 0;
 return activeCpu->ram_[address];
}
void write6502(std::uint16_t address,std::uint8_t value){
 if(activeCpu)activeCpu->ram_[address]=value;
}
void Apple1986Live::reset(int speed){
 activeCpu=this;
 std::copy(std::begin(kAppleLiveSpaceRAM),std::end(kAppleLiveSpaceRAM),ram_.begin());
 // Replicate the SPA overlay's entry-point register-pointer setup.
 // Preserve original live zero-page sprite pointers.
 // No synthetic pointer initialization.
 // Reset SPA's runtime-controlled star/speaker flags, without replacing
 // unrelated resident OS/game memory with arbitrary zeroes.
 // Preserve original star/speaker state and self-modifying SPA code.
 events_.clear();eventIndex_=0;cpuCycles_=0;sampleIndex_=0;
 speakerLevel_=false;dc_=0;dcHoldSamples_=0;hyperRunning_=false;prevHyperPhase_=0;phaseCalls_=0;lastCpuStage_=0;hyperStatus_.store(0,std::memory_order_release);
 reset6502();setP(0x30);
 runFrame(speed);
}
void Apple1986Live::runFrame(int speed){
 activeCpu=this;
 // Original $93F4 is derived by SPA $9337 from $928E,X, not
 // host DOS flight speed. Preserve the authentic live value.
 PC=0x9486;SP=0xfd;ram_[0x1fe]=0xfe;ram_[0x1ff]=0xff;
 for(int i=0;i<2000000;++i){
  if(PC==0xffff)return;
  cpuCycles_+=static_cast<std::uint64_t>(step6502());
 }
 throw std::runtime_error("Apple SPA frame did not return");
}
float Apple1986Live::nextSample(int speed,int hyperPhase){
 // Launch the genuine 1986 Apple II HyperJump command at $9811.
 // Its own 6502 program governs ramp, 200+200 light-speed frames, recovery.
 if(hyperPhase==1 && prevHyperPhase_==0 && !hyperRunning_){
    activeCpu=this;
    hyperRunning_=true;
    phaseCalls_=0;lastCpuStage_=0;
    hyperStatus_.store(0,std::memory_order_release);
    // Original $93F4 is derived by SPA $9337 from $928E,X, not
 // host DOS flight speed. Preserve the authentic live value.
    PC=0x9811;
    SP=0xfd;
    ram_[0x1fe]=0xfe;
    ram_[0x1ff]=0xff;
 }
 prevHyperPhase_=hyperPhase;
 const auto target=(sampleIndex_++)*(1023000.0/PcSpeakerEmulator::SAMPLE_RATE);
 while(static_cast<double>(cpuCycles_)<=target){
    if(hyperRunning_){
       // $989B follows the sound-producing portion of the original command;
       // stop at the inlined completion-message routine.
       if(PC==0x989b){
          hyperRunning_=false;
          hyperStatus_.store(static_cast<std::uint64_t>(5)<<56,std::memory_order_release);
          continue;
       }
       const int stage=PC==0x9824?1:PC==0x9868?2:
                       PC==0x9886?3:PC==0x988d?4:0;
       if(stage!=0){
          if(stage!=lastCpuStage_){lastCpuStage_=stage;phaseCalls_=0;}
          ++phaseCalls_;
          hyperStatus_.store((static_cast<std::uint64_t>(stage)<<56) |
              (static_cast<std::uint64_t>(ram_[0x93f4])<<48) |
              phaseCalls_,std::memory_order_release);
       }
       activeCpu=this;
       cpuCycles_+=static_cast<std::uint64_t>(step6502());
    }else{
       runFrame(speed);
    }
 }
 while(eventIndex_<events_.size() && static_cast<double>(events_[eventIndex_])<=target){
  speakerLevel_=!speakerLevel_;dcHoldSamples_=42768;++eventIndex_;
 }
 // Collapse consumed events periodically to avoid unbounded long-session RAM.
 if(eventIndex_>2048){
  events_.erase(events_.begin(),events_.begin()+static_cast<std::ptrdiff_t>(eventIndex_));
  eventIndex_=0;
 }
 // AppleWin-style original speaker state reproduction:
 // toggle-based bipolar level, no additional filter synthesizing pitches.
 // Avoid persistent DC output only after speaker inactivity.
 double value=speakerLevel_?1.0:-1.0;
 if(dcHoldSamples_>=32768){--dcHoldSamples_;}
 else if(dcHoldSamples_>0){value*=static_cast<double>(dcHoldSamples_--)/32768.0;}
 else value=0.0;
 return static_cast<float>(value);

}
