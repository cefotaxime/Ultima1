#pragma once
#include <stdio.h>
#include <stdint.h>
#include <pc.h>
#include <time.h>
// 1-bit sigma-delta PC speaker output, sourced from captured 48 kHz PCM.
// This preserves source samples but cannot reproduce 16-bit PCM fidelity.
static int dosPcmReady[2]={0,0};
static int16_t dosPcmData[2][4000];
static unsigned dosPcmCount[2]={0,0};
static unsigned dosPcmEvents[2]={0,0};
static unsigned long dosPcmLastTicks[2]={0,0};
static int dosPcmLoadOne(int index,const char*name){
 FILE*f=fopen(name,"rb");if(!f)return 0;
 unsigned char h[44];
 if(fread(h,1,44,f)!=44){fclose(f);return 0;}
 if(h[0]!='R'||h[1]!='I'||h[2]!='F'||h[3]!='F'||h[8]!='W'||h[9]!='A'||h[10]!='V'||h[11]!='E'||h[20]!=1||h[22]!=1||h[24]!=0x80||h[25]!=0xbb||h[26]!=0||h[27]!=0||h[34]!=16){fclose(f);return 0;}
 unsigned n=(unsigned)(h[40]|((unsigned)h[41]<<8)|((unsigned)h[42]<<16)|((unsigned)h[43]<<24))/2;
 if(n==0||n>4000||fread(dosPcmData[index],2,n,f)!=n){fclose(f);return 0;}
 fclose(f);dosPcmCount[index]=n;dosPcmReady[index]=1;return 1;
}
static void dosPcmInit(){
 dosPcmLoadOne(0,"SND00.WAV");
 dosPcmLoadOne(1,"SND04.WAV");
}
static void dosPcmPlay(int index){
 if(index<0||index>1||!dosPcmReady[index])return;
 ++dosPcmEvents[index];
 // PIT clock-driven delta-sigma at 48kHz. No sample clock drift accumulated.
 unsigned char old=(unsigned char)inportb(0x61);
 outportb(0x61,old&~3);
 int integrator=0;
 const unsigned long base=uclock();
 for(unsigned i=0;i<dosPcmCount[index];i++){
  const long target=(long)(((unsigned long long)i*UCLOCKS_PER_SEC)/48000u);
  while((long)(uclock()-base)<target){}
  integrator+=(int)dosPcmData[index][i];
  if(integrator>=0){outportb(0x61,(old&~3)|2);integrator-=32767;}
  else{outportb(0x61,old&~3);integrator+=32767;}
 }
 outportb(0x61,old);
 dosPcmLastTicks[index]=(unsigned long)(uclock()-base);
}