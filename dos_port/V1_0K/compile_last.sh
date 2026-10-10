
#!/bin/bash
cd /mnt/c/Users/hojun/Downloads/Ultima1_KR_Work
for n in DungeonSystem SpaceSystem; do
 if tools/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++ -std=gnu++17 -O1 -ffunction-sections -fdata-sections -Idos_port/V1_0K -Isrc/HF93/src -c src/HF93/src/${n}.cpp -o dos_port/V1_0K/${n}.dos.o 2>/tmp/porterr; then echo "${n}:OK"; else echo "${n}:FAIL"; head -10 /tmp/porterr; fi
done
