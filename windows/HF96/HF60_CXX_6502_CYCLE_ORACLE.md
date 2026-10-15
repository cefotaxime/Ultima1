# HF60 standalone C++ 6502 audio interpreter oracle — 2026-10-09
Original Apple II 1986 binaries SPA.bin at 0x8956, STUPH.bin at 0x0800, audio speaker $C030.
C core: third_party/fake6502/fake6502.c/.h (ivop/fake6502, BSD-2-Clause) and LICENSE.
C++ harness: tools/apple1986_cpu_oracle.cpp; executes the original $9497 routine with persistent RAM and 6502 registers over 542 frames, following the same synthetic speed schedule as tools/hf60_live_6502_oracle.py.
Observed:
- C++ and py65 execute 542 updates.
- C++ and corrected py65 both accumulate exactly 9,008,174 CPU cycles.
- Both register exactly 10,577 $C030 toggles.
- Individual frame outputs match for tested frames 0,63,64,70,71,270,470,471,477,541.
- Localized discrepancy in initial comparison was Python py65 bug: DEC absolute opcode CE at PC 958E incorrectly set to 3 cycles versus real MOS6502 6 cycles. Corrected oracle script via cpu.cycletime[0xce]=6. Reference: https://www.nesdev.org/obelisk-6502-guide/reference.html
This is an important PORTABLE C++ CPU progress milestone, NOT yet runtime-integration into SDL, original HyperJump full scheduler, authentic Apple II boot memory, or a Windows EXE game package.
Example WSL build:
gcc -O2 -c third_party/fake6502/fake6502.c -o fake6502.o
g++ -O2 -std=c++17 tools/apple1986_cpu_oracle.cpp fake6502.o -o apple1986_cpu_oracle