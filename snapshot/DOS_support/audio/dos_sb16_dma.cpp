#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pc.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
extern "C" void hf93SetError(const char*);
// Independent Sound Blaster 8-bit DMA transport. HF93 samples are supplied
// separately by the original SfxSystem, not synthesized here.
namespace {
int base=0, dmaChannel=1, segment=-1, dosSelector=-1;
unsigned long linear=0;
unsigned long lastStart=0;
int playing=0;
const unsigned RATE=22050;
const unsigned BLOCK=512;
static int outReady(int b){for(int i=0;i<20000;i++)if(!(inportb(b+0xC)&0x80))return 1;return 0;}
static int inReady(int b){for(int i=0;i<20000;i++)if(inportb(b+0xE)&0x80)return 1;return 0;}
static void command(int c){if(outReady(base))outportb(base+0xC,c);}
static int reset(int b){outportb(b+6,1);for(volatile int n=0;n<10000;n++){}outportb(b+6,0);if(!inReady(b))return 0;return inportb(b+0xA)==0xAA;}
static unsigned long microtime(){return (unsigned long)((unsigned long long)uclock()*1000000ull/UCLOCKS_PER_SEC);}
}
extern "C" int hf93SbInit(){
 if(base)return 1;
 const char* env=getenv("BLASTER");int port=0x220;int ch=1;
 if(env){const char* p=strchr(env,'A');if(p)sscanf(p+1,"%x",&port);p=strchr(env,'D');if(p)sscanf(p+1,"%d",&ch);}
 if(ch<0||ch>3){hf93SetError("DOS Sound Blaster invalid BLASTER DMA channel (expected D0..D3)");return 0;}
 base=port;dmaChannel=ch;if(!reset(base)){base=0;hf93SetError("DOS Sound Blaster DSP reset failed; check BLASTER address and card configuration");return 0;}
 int selector=0;segment=__dpmi_allocate_dos_memory((65536+BLOCK+15)/16,&selector);dosSelector=selector;
 if(segment<0){base=0;dosSelector=-1;hf93SetError("DOS Sound Blaster DMA conventional-memory allocation failed");return 0;}
 linear=(unsigned long)segment<<4;
 // 2048-byte DMA block must never cross a 64K physical boundary.
 if(((linear&0xffffu)+BLOCK)>65536u)linear=(linear+65535u)&~65535u;
 command(0xD1);playing=0;lastStart=0;return 1;
}
extern "C" int hf93SbReady(void){return base && (!playing || microtime()-lastStart>=((unsigned long)BLOCK*1000000ul/RATE)+1000ul);}
extern "C" int hf93SbSubmit(const unsigned char* pcm,unsigned count){
 if(!base||!linear||!pcm||count!=BLOCK)return 0;
 if(playing && microtime()-lastStart<((unsigned long)BLOCK*1000000ul/RATE)+1000ul)return 0;
 playing=0;dosmemput(pcm,BLOCK,linear);
 static const unsigned char mask[]={0x0a,0x0b,0x0c,0x02,0x03};
 (void)mask;
 outportb(0x0A,0x04|dmaChannel);outportb(0x0C,0);
 outportb(0x0B,0x48|dmaChannel); // single-cycle read (memory -> DSP)
 static const int pagePort[4]={0x87,0x83,0x81,0x82};
 outportb(pagePort[dmaChannel],(linear>>16)&255);
 const int addrPort=2*dmaChannel, countPort=addrPort+1;
 outportb(addrPort,linear&255);outportb(addrPort,(linear>>8)&255);
 outportb(countPort,(BLOCK-1)&255);outportb(countPort,((BLOCK-1)>>8)&255);
 outportb(0x0A,dmaChannel);
 command(0x40);command(256-(1000000u/RATE));
 command(0x14);command((BLOCK-1)&255);command((BLOCK-1)>>8);
 lastStart=microtime();playing=1;return 1;
}
extern "C" void hf93SbShutdown(){if(base){command(0xD0);command(0xD3);outportb(0x0A,0x04|dmaChannel);base=0;}if(segment>=0){__dpmi_free_dos_memory(dosSelector);segment=-1;dosSelector=-1;}linear=0;playing=0;}