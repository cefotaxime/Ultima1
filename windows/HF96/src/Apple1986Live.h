#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>
extern "C" std::uint8_t read6502(std::uint16_t);
extern "C" void write6502(std::uint16_t,std::uint8_t);
// One original Apple II 6502 virtual speaker, no independent pitch oscillators.
// Audio thread is the only owner of the global fake6502 interpreter.
class Apple1986Live {
public:
 void reset(int speed);
 float nextSample(int speed,int hyperPhase);
 std::uint64_t hyperStatus() const {return hyperStatus_.load(std::memory_order_acquire);}
 void clearHyperStatus() {hyperStatus_.store(0,std::memory_order_release);}
private:
 std::array<std::uint8_t,65536> ram_{};
 std::vector<std::uint64_t> events_;
 std::size_t eventIndex_=0;
 std::uint64_t cpuCycles_=0;
 std::uint64_t sampleIndex_=0;
 bool speakerLevel_=false;
 std::atomic<std::uint64_t> hyperStatus_{0};
 std::uint32_t phaseCalls_=0;
 int lastCpuStage_=0;
 bool hyperRunning_=false;
 int prevHyperPhase_=0;
 double dc_=0;
 unsigned dcHoldSamples_=0;
 void runFrame(int speed);
 friend std::uint8_t read6502(std::uint16_t);
 friend void write6502(std::uint16_t,std::uint8_t);
};