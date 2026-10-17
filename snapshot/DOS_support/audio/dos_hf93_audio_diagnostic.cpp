#include "SDL.h"
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
// DOS 48 kHz mono callback bridge: default Sound Blaster DMA, optional PCM capture.
static SDL_AudioSpec dosAudioSpec={};
static bool dosAudioOpen=false;
static bool dosAudioPaused=true;
static FILE* dosAudioCapture=nullptr;
static unsigned long dosAudioFrames=0;
static bool sbMode=false;
static unsigned resamplePhase=0;
static Uint32 initializedSubsystems=0;
extern "C" void hf93SetError(const char*);
extern "C" int hf93SbInit(void);
extern "C" int hf93SbReady(void);
extern "C" int hf93SbSubmit(const unsigned char*,unsigned);
extern "C" void hf93SbShutdown(void);
extern "C" uint64_t SDL_GetPerformanceFrequency(void){return (uint64_t)CLOCKS_PER_SEC;}
extern "C" uint64_t SDL_GetPerformanceCounter(void){return (uint64_t)clock();}
extern "C" int SDL_setenv(const char* k,const char* v,int overwrite){return k&&v?setenv(k,v,overwrite):-1;}
extern "C" Uint32 SDL_WasInit(Uint32 flags){return flags?initializedSubsystems&flags:initializedSubsystems;}
extern "C" int SDL_InitSubSystem(Uint32 flags){initializedSubsystems|=flags;return 0;}
extern "C" SDL_AudioDeviceID SDL_OpenAudioDevice(const char*,int captureDevice,const SDL_AudioSpec* wanted,SDL_AudioSpec* obtained,int){
 hf93SetError("");
 if(dosAudioOpen){hf93SetError("DOS audio device already open");return 0;}
 if(captureDevice || !wanted || !wanted->callback || wanted->channels!=1 || wanted->format!=AUDIO_S16SYS || wanted->freq!=48000){
  hf93SetError("DOS audio requires 48000 Hz S16 mono output and a callback");return 0;
 }
 const char* capture=getenv("HF93_DOS_AUDIO_CAPTURE");
 if(capture && *capture){
  dosAudioCapture=fopen(capture,"wb");
  if(!dosAudioCapture){hf93SetError("DOS PCM capture file could not be opened");return 0;}
 }else{
  // Normal boot must not require the historical diagnostic opt-in variable.
  const char* sb=getenv("HF93_DOS_SB16");
  if(sb && strcmp(sb,"0")==0){hf93SetError("DOS Sound Blaster audio explicitly disabled (HF93_DOS_SB16=0)");return 0;}
  if(!hf93SbInit())return 0;
  sbMode=true;
 }
 dosAudioSpec=*wanted;
 if(!dosAudioSpec.samples || dosAudioSpec.samples>1024)dosAudioSpec.samples=512;
 dosAudioSpec.silence=0;dosAudioSpec.size=dosAudioSpec.samples*2;
 if(obtained)*obtained=dosAudioSpec;
 resamplePhase=0;dosAudioOpen=true;dosAudioPaused=true;dosAudioFrames=0;
 initializedSubsystems|=SDL_INIT_AUDIO;
 return 1;
}
extern "C" void SDL_PauseAudioDevice(SDL_AudioDeviceID id,int pause){if(id==1&&dosAudioOpen)dosAudioPaused=(pause!=0);}
extern "C" const char* SDL_GetCurrentAudioDriver(void){return sbMode?"DOS Sound Blaster DMA 22050Hz 8bit":(dosAudioOpen?"DOS 48kHz PCM capture diagnostic (no hardware)":"DOS audio unavailable");}
extern "C" void SDL_CloseAudioDevice(SDL_AudioDeviceID id){if(id!=1)return;dosAudioOpen=false;dosAudioPaused=true;if(sbMode){hf93SbShutdown();sbMode=false;}if(dosAudioCapture){fclose(dosAudioCapture);dosAudioCapture=nullptr;}}
extern "C" void hf93AudioCaptureTick(void){
 if(!dosAudioOpen||dosAudioPaused||!dosAudioSpec.callback)return;
 if(sbMode){
  if(!hf93SbReady())return;
  const uclock_t begin=uclock();
  // Stream-state resampler: preserve exact 48000:22050 phase across DMA blocks.
  static short input[2048];static unsigned char output[512];
  unsigned& phase=resamplePhase; // reset when opening a new stream
  const unsigned step=48000,denom=22050;
  unsigned positions[512];unsigned cursor=0;
  for(unsigned i=0;i<512;i++){
   positions[i]=cursor;
   phase+=step;
   cursor+=phase/denom;
   phase%=denom;
  }
  const unsigned required=positions[511]+1;
  int samples=dosAudioSpec.samples;if(samples<1||samples>1024)samples=512;
  for(unsigned offset=0;offset<required;){
   unsigned n=(unsigned)samples;if(n>required-offset)n=required-offset;
   dosAudioSpec.callback(dosAudioSpec.userdata,(Uint8*)(input+offset),n*2);offset+=n;
  }
  for(unsigned i=0;i<512;i++){
   int v=input[positions[i]]/256+128;if(v<0)v=0;if(v>255)v=255;
   output[i]=(unsigned char)v;
  }
  // Consume the remaining source frames, preserving alignment with the next block.
  if(cursor>required){
   unsigned skip=cursor-required;
   while(skip){unsigned n=skip>1024?1024:skip;dosAudioSpec.callback(dosAudioSpec.userdata,(Uint8*)input,n*2);skip-=n;}
  }  hf93SbSubmit(output,512);
  static unsigned blocks=0;static unsigned long maxUs=0;
  const unsigned long us=(unsigned long)(((unsigned long long)(uclock()-begin)*1000000ull)/UCLOCKS_PER_SEC);
  if(us>maxUs)maxUs=us;
  if(++blocks%128==0){FILE* f=fopen("AUDPERF.LOG","a");if(f){fprintf(f,"blocks=%u max_callback_us=%lu latest_us=%lu\\n",blocks,maxUs,us);fclose(f);}}
  return;
 }
 if(!dosAudioCapture)return;
 // Capture exactly one native callback block; bound diagnostic file growth to 5 seconds.
 if(dosAudioFrames>=240000u)return;
 short samples[1024]={};int n=dosAudioSpec.samples;if(n<1||n>1024)n=512;
 dosAudioSpec.callback(dosAudioSpec.userdata,(Uint8*)samples,n*2);
 fwrite(samples,2,n,dosAudioCapture);fflush(dosAudioCapture);dosAudioFrames+=(unsigned)n;
}