
#!/bin/bash
cd /mnt/c/Users/hojun/Downloads/Ultima1_KR_Work
for name in CommandSystem OverworldSimulation DungeonSystem VehicleSystem KingSystem ShopSystem SpaceSystem; do
if tools/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++ -std=gnu++17 -O1 -ffunction-sections -fdata-sections -Idos_port/V1_0K -Isrc/HF93/src -c src/HF93/src/${name}.cpp -o dos_port/V1_0K/${name}.dos.o 2>/tmp/porterr; then
 echo "${name}:OK"
else
 echo "${name}:FAIL"
 head -3 /tmp/porterr
fi
done
