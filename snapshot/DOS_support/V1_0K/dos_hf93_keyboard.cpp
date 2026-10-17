#include "SDL.h"
#include <dpmi.h>

#include <string.h>
#include <stdio.h>
// BIOS console input. One synthetic keydown, without fabricated game screens.
static SDL_Event pending[32];
static int count=0;
static SDL_Keymod biosModifiers(void){
 __dpmi_regs r={};r.h.ah=0x02;__dpmi_int(0x16,&r);
 return (SDL_Keymod)((r.h.al&3)?KMOD_SHIFT:0);
}
extern "C" SDL_Keymod SDL_GetModState(void){return biosModifiers();}
extern "C" int SDL_PushEvent(SDL_Event* e){
 if(!e||count>=32)return -1;
 pending[count++]=*e;return 1;
}
extern "C" int SDL_PeepEvents(SDL_Event* e,int n,int action,Uint32 low,Uint32 high){
 if(!e||n<=0||action!=SDL_PEEKEVENT)return 0;
 int found=0;for(int i=0;i<count&&found<n;i++)if(pending[i].type>=low&&pending[i].type<=high)e[found++]=pending[i];
 return found;
}
extern "C" int SDL_PollEvent(SDL_Event* e){
 if(!e)return 0;
 if(count){*e=pending[0];memmove(pending,pending+1,sizeof(SDL_Event)*(--count));return 1;}
 __dpmi_regs peek={};peek.h.ah=0x01;__dpmi_int(0x16,&peek);
 if(peek.x.flags&0x40)return 0; // ZF=1: BIOS keyboard buffer empty; never block
 __dpmi_regs read={};read.h.ah=0x00;__dpmi_int(0x16,&read);
 int c=read.h.al;int ext=read.h.ah;int key=c;
 if(c==0||c==224)key=ext==72?SDLK_UP:ext==80?SDLK_DOWN:ext==75?SDLK_LEFT:ext==77?SDLK_RIGHT:0;
 if(c>='A'&&c<='Z')key=c+32;
 memset(e,0,sizeof(*e));e->type=SDL_KEYDOWN;e->key.type=SDL_KEYDOWN;e->key.keysym.sym=key;e->key.keysym.mod=biosModifiers();
 if(key>='a'&&key<='z')e->key.keysym.scancode=SDL_SCANCODE_A+key-'a';
 return 1;
}