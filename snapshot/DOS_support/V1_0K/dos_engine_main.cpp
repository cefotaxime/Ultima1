#include "LocalSimulation.h"
#include <stdio.h>
int main(){
 FILE* f=fopen("ENGINE.OK","wb");
 if(!f) return 2;
 DosRng rng;
 rng.seed(0x1234);
 fprintf(f,"V1.0K HF93 DOS ENGINE\n");
 fprintf(f,"DOS RNG initial seed 0x1234\n");
 for(int i=0;i<10;i++) fprintf(f,"%d%s",rng.range(1,255), i==9?"\n":",");
 fclose(f);
 puts("HF93 LocalSimulation DosRng running under DOS.");
 puts("ENGINE.OK generated. Exit.");
 return 0;
}