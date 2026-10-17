#include "SDL.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
// HF93 platform primitives. Relative asset path assumes game working directory.
static char dosError[192] = "";
extern "C" void hf93SetError(const char* message){snprintf(dosError,sizeof(dosError),"%s",message?message:"");}
extern "C" const char* SDL_GetError(void){return dosError;}
extern "C" void SDL_free(void* p){free(p);}
extern "C" char* SDL_GetBasePath(void){
 char* p=(char*)malloc(3);
 if(p){p[0]='.';p[1]='/';p[2]=0;}
 return p;
}
extern "C" void hf93AudioCaptureTick(void);
extern "C" void SDL_Delay(uint32_t ms){
 // A synchronous original sound waits with SDL_Delay(0). Feed ready DMA blocks here.
 if(ms==0){hf93AudioCaptureTick();return;}
 if(!ms)return;
 const clock_t start=clock();
 const clock_t target=(clock_t)((double)ms*CLOCKS_PER_SEC/1000.0);
 while((clock_t)(clock()-start)<target){}
}