#include "OverworldSimulation.h"
#include "LocalSimulation.h"
#include <stdio.h>
#include <stdint.h>
static unsigned char map[13104];
int OriginalData::terrainAt(int x,int y) const {
 x=(x%168+168)%168;y=(y%156+156)%156;
 int i=y*168+x; return (i&1)?(map[i/2]&15):(map[i/2]>>4);
}
int main(){
 FILE*f=fopen("MAP.BIN","rb");if(!f)return 2;
 if(fread(map,1,13104,f)!=13104){fclose(f);return 3;}fclose(f);
 OriginalData original;
 LocalSimulation local(original);local.seed(0x1234);
 OverworldSimulation overworld(original,local);
 PlayerSave p;p.x=44;p.y=38;p.hits=100;p.food=100;
 int a=original.terrainAt(44,38);
 overworld.onPlayerMoved(p,44,38);
 overworld.maybeSpawn(p);
 auto messages=overworld.updateAfterCommand(p);
 FILE*out=fopen("SIM.OK","wb");if(!out)return 4;
 fprintf(out,"HF93 OVERWORLD LINKED TERRAIN=%d X=%u Y=%u MESSAGES=%u HP=%u\n",a,p.x,p.y,(unsigned)messages.size(),p.hits);fclose(out);
 return 0;
}